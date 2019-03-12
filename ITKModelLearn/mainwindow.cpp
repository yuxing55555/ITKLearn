#include "mainwindow.h"
#include "ui_mainwindow.h"

#include"GlobeInclude.h"

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkGeodesicActiveContourLevelSetImageFilter.h"
#include "itkCurvatureAnisotropicDiffusionImageFilter.h"
#include "itkGradientMagnitudeRecursiveGaussianImageFilter.h"
#include "itkSigmoidImageFilter.h"
#include "itkFastMarchingImageFilter.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{


    const char * inputFileName =      argv[1];
    const char * outputFileName =     argv[2];
    const int seedPosX =              atoi( argv[3] );
    const int seedPosY =              atoi( argv[4] );

    const double initialDistance =    atof( argv[5] );
    const double sigma =              atof( argv[6] );
    const double alpha =              atof( argv[7] );
    const double beta  =              atof( argv[8] );
    const double propagationScaling = atof( argv[9] );
    const double numberOfIterations = atoi( argv[10] );
    const double seedValue =          - initialDistance;

    constexpr unsigned int Dimension = 2;

    using InputPixelType = float;
    using InputImageType = itk::Image< InputPixelType, Dimension >;
    using OutputPixelType = unsigned char;
    using OutputImageType = itk::Image< OutputPixelType, Dimension >;

    using ReaderType = itk::ImageFileReader< InputImageType >;
    using WriterType = itk::ImageFileWriter< OutputImageType >;

    ReaderType::Pointer reader = ReaderType::New();
    reader->SetFileName( inputFileName );

    using SmoothingFilterType = itk::CurvatureAnisotropicDiffusionImageFilter< InputImageType, InputImageType >;
    SmoothingFilterType::Pointer smoothing = SmoothingFilterType::New();
    smoothing->SetTimeStep( 0.125 );
    smoothing->SetNumberOfIterations( 5 );
    smoothing->SetConductanceParameter( 9.0 );
    smoothing->SetInput( reader->GetOutput() );

    using GradientFilterType = itk::GradientMagnitudeRecursiveGaussianImageFilter< InputImageType, InputImageType >;
    GradientFilterType::Pointer  gradientMagnitude = GradientFilterType::New();
    gradientMagnitude->SetSigma( sigma );
    gradientMagnitude->SetInput( smoothing->GetOutput() );

    using SigmoidFilterType = itk::SigmoidImageFilter< InputImageType, InputImageType >;
    SigmoidFilterType::Pointer sigmoid = SigmoidFilterType::New();
    sigmoid->SetOutputMinimum( 0.0 );
    sigmoid->SetOutputMaximum( 1.0 );
    sigmoid->SetAlpha( alpha );
    sigmoid->SetBeta( beta );
    sigmoid->SetInput( gradientMagnitude->GetOutput() );

    using FastMarchingFilterType = itk::FastMarchingImageFilter< InputImageType, InputImageType >;
    FastMarchingFilterType::Pointer  fastMarching = FastMarchingFilterType::New();

    using GeodesicActiveContourFilterType = itk::GeodesicActiveContourLevelSetImageFilter< InputImageType, InputImageType >;
    GeodesicActiveContourFilterType::Pointer geodesicActiveContour = GeodesicActiveContourFilterType::New();
    geodesicActiveContour->SetPropagationScaling( propagationScaling );
    geodesicActiveContour->SetCurvatureScaling( 1.0 );
    geodesicActiveContour->SetAdvectionScaling( 1.0 );
    geodesicActiveContour->SetMaximumRMSError( 0.02 );
    geodesicActiveContour->SetNumberOfIterations( numberOfIterations );
    geodesicActiveContour->SetInput( fastMarching->GetOutput() );
    geodesicActiveContour->SetFeatureImage( sigmoid->GetOutput() );

    using ThresholdingFilterType = itk::BinaryThresholdImageFilter< InputImageType, OutputImageType >;
    ThresholdingFilterType::Pointer thresholder = ThresholdingFilterType::New();
    thresholder->SetLowerThreshold( -1000.0 );
    thresholder->SetUpperThreshold( 0.0 );
    thresholder->SetOutsideValue( itk::NumericTraits< OutputPixelType >::min() );
    thresholder->SetInsideValue( itk::NumericTraits< OutputPixelType >::max() );
    thresholder->SetInput( geodesicActiveContour->GetOutput() );

    using NodeContainer = FastMarchingFilterType::NodeContainer;
    using NodeType = FastMarchingFilterType::NodeType;

    InputImageType::IndexType  seedPosition;
    seedPosition[0] = seedPosX;
    seedPosition[1] = seedPosY;

    NodeContainer::Pointer seeds = NodeContainer::New();
    NodeType node;
    node.SetValue( seedValue );
    node.SetIndex( seedPosition );

    seeds->Initialize();
    seeds->InsertElement( 0, node );

    fastMarching->SetTrialPoints( seeds );
    fastMarching->SetSpeedConstant( 1.0 );

    using CastFilterType = itk::RescaleIntensityImageFilter< InputImageType, OutputImageType >;

    CastFilterType::Pointer caster1 = CastFilterType::New();
    CastFilterType::Pointer caster2 = CastFilterType::New();
    CastFilterType::Pointer caster3 = CastFilterType::New();
    CastFilterType::Pointer caster4 = CastFilterType::New();

    WriterType::Pointer writer1 = WriterType::New();
    WriterType::Pointer writer2 = WriterType::New();
    WriterType::Pointer writer3 = WriterType::New();
    WriterType::Pointer writer4 = WriterType::New();

    caster1->SetInput( smoothing->GetOutput() );
    writer1->SetInput( caster1->GetOutput() );
    writer1->SetFileName("GeodesicActiveContourImageFilterOutput1.png");
    caster1->SetOutputMinimum( itk::NumericTraits< OutputPixelType >::min() );
    caster1->SetOutputMaximum( itk::NumericTraits< OutputPixelType >::max() );
    writer1->Update();

    caster2->SetInput( gradientMagnitude->GetOutput() );
    writer2->SetInput( caster2->GetOutput() );
    writer2->SetFileName("GeodesicActiveContourImageFilterOutput2.png");
    caster2->SetOutputMinimum( itk::NumericTraits< OutputPixelType >::min() );
    caster2->SetOutputMaximum( itk::NumericTraits< OutputPixelType >::max() );
    writer2->Update();

    caster3->SetInput( sigmoid->GetOutput() );
    writer3->SetInput( caster3->GetOutput() );
    writer3->SetFileName("GeodesicActiveContourImageFilterOutput3.png");
    caster3->SetOutputMinimum( itk::NumericTraits< OutputPixelType >::min() );
    caster3->SetOutputMaximum( itk::NumericTraits< OutputPixelType >::max() );
    writer3->Update();

    caster4->SetInput( fastMarching->GetOutput() );
    writer4->SetInput( caster4->GetOutput() );
    writer4->SetFileName("GeodesicActiveContourImageFilterOutput4.png");
    caster4->SetOutputMinimum( itk::NumericTraits< OutputPixelType >::min() );
    caster4->SetOutputMaximum( itk::NumericTraits< OutputPixelType >::max() );

    fastMarching->SetOutputSize(
             reader->GetOutput()->GetBufferedRegion().GetSize() );

    WriterType::Pointer writer = WriterType::New();
    writer->SetFileName( outputFileName );
    writer->SetInput( thresholder->GetOutput() );
    try
      {
      writer->Update();
      }
    catch( itk::ExceptionObject & error )
      {
      std::cerr << "Error: " << error << std::endl;
      return EXIT_FAILURE;
      }

    std::cout << std::endl;
    std::cout << "Max. no. iterations: " << geodesicActiveContour->GetNumberOfIterations() << std::endl;
    std::cout << "Max. RMS error: " << geodesicActiveContour->GetMaximumRMSError() << std::endl;
    std::cout << std::endl;
    std::cout << "No. elpased iterations: " << geodesicActiveContour->GetElapsedIterations() << std::endl;
    std::cout << "RMS change: " << geodesicActiveContour->GetRMSChange() << std::endl;

    try
      {
      writer4->Update();
      }
    catch( itk::ExceptionObject & error )
      {
      std::cerr << "Error: " << error << std::endl;
      return EXIT_FAILURE;
      }

    using InternalWriterType = itk::ImageFileWriter< InputImageType >;

    InternalWriterType::Pointer mapWriter = InternalWriterType::New();
    mapWriter->SetInput( fastMarching->GetOutput() );
    mapWriter->SetFileName("GeodesicActiveContourImageFilterOutput4.mha");
    try
      {
      mapWriter->Update();
      }
    catch( itk::ExceptionObject & error )
      {
      std::cerr << "Error: " << error << std::endl;
      return EXIT_FAILURE;
      }

    InternalWriterType::Pointer speedWriter = InternalWriterType::New();
    speedWriter->SetInput( sigmoid->GetOutput() );
    speedWriter->SetFileName("GeodesicActiveContourImageFilterOutput3.mha");
    try
      {
      speedWriter->Update();
      }
    catch( itk::ExceptionObject & error )
      {
      std::cerr << "Error: " << error << std::endl;
      return EXIT_FAILURE;
      }

    InternalWriterType::Pointer gradientWriter = InternalWriterType::New();
    gradientWriter->SetInput( gradientMagnitude->GetOutput() );
    gradientWriter->SetFileName("GeodesicActiveContourImageFilterOutput2.mha");
    try
      {
      gradientWriter->Update();
      }
    catch( itk::ExceptionObject & error )
      {
      std::cerr << "Error: " << error << std::endl;
      return EXIT_FAILURE;
      }

//    const char * inputImageName   = "../ITKModelLearn/Data/HeadMRVolume.mha";
//    const char * inputMeshName    = "../ITKModelLearn/Data/torus.vtk";
//    const char * outputImageName  = "../ITKModelLearn/Data/OutputBaseline.mha";

//    constexpr unsigned int Dimension = 3;
//    using MeshPixelType = double;

//    using MeshType = itk::Mesh< MeshPixelType, Dimension >;

//    using MeshReaderType = itk::MeshFileReader< MeshType >;
//    MeshReaderType::Pointer meshReader = MeshReaderType::New();
//    meshReader->SetFileName( inputMeshName );

    /*using InputPixelType = unsigned char;
    using InputImageType = itk::Image< InputPixelType, Dimension >;
    using ImageReaderType = itk::ImageFileReader< InputImageType >;

    ImageReaderType::Pointer imageReader = ImageReaderType::New();
    imageReader->SetFileName( inputImageName );

    using OutputPixelType = unsigned char;
    using OutputImageType = itk::Image< OutputPixelType, Dimension >;

    using CastFilterType = itk::CastImageFilter< InputImageType, OutputImageType >;
    CastFilterType::Pointer cast = CastFilterType::New();
    cast->SetInput( imageReader->GetOutput() );

    using FilterType = itk::TriangleMeshToBinaryImageFilter< MeshType, OutputImageType >;
    FilterType::Pointer filter = FilterType::New();
    filter->SetInput( meshReader->GetOutput() );
    filter->SetInfoImage( cast->GetOutput() );
    filter->SetInsideValue( itk::NumericTraits< OutputPixelType >::max() );
    try
      {
      filter->Update();
      }
    catch( itk::ExceptionObject & error )
      {
      std::cerr << "Error: " << error << std::endl;
//      return EXIT_FAILURE;
      }

    using WriterType = itk::ImageFileWriter< OutputImageType >;
    WriterType::Pointer writer = WriterType::New();
    writer->SetFileName( outputImageName );
    writer->SetInput( filter->GetOutput() );
    try
      {
      writer->Update();
      }
    catch( itk::ExceptionObject & error )
      {
      std::cerr << "Error: " << error << std::endl;
//      return EXIT_FAILURE;
      }*/

}
//#include <vtkImageData.h>
//#include <vtkMetaImageReader.h>
//#include <vtkSmartPointer.h>
//#include <vtkInteractorStyleImage.h>
//#include <vtkRenderer.h>
//#include <vtkImageActor.h>
//#include <vtkImageMapper3D.h>
//#include <vtkRenderWindow.h>
//#include <vtkRenderWindowInteractor.h>
void MainWindow::on_actionMha_triggered()
{
//    std::string inputFilename = "../ITKModelLearn/Data/HeadMRVolume.mha";

//    vtkSmartPointer<vtkMetaImageReader> reader =
//      vtkSmartPointer<vtkMetaImageReader>::New();
//    reader->SetFileName(inputFilename.c_str());
//    reader->Update();

//    // Visualize
//    vtkSmartPointer<vtkImageActor> actor =
//      vtkSmartPointer<vtkImageActor>::New();
//    actor->GetMapper()->SetInputConnection(reader->GetOutputPort());

//    vtkSmartPointer<vtkRenderer> renderer =
//      vtkSmartPointer<vtkRenderer>::New();
//    renderer->AddActor(actor);
//    renderer->ResetCamera();

//    vtkSmartPointer<vtkRenderWindow> renderWindow =
//      vtkSmartPointer<vtkRenderWindow>::New();
//    renderWindow->AddRenderer(renderer);

//    vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
//      vtkSmartPointer<vtkRenderWindowInteractor>::New();
//    vtkSmartPointer<vtkInteractorStyleImage> style =
//      vtkSmartPointer<vtkInteractorStyleImage>::New();

//    renderWindowInteractor->SetInteractorStyle(style);

//    renderWindowInteractor->SetRenderWindow(renderWindow);
//    renderWindowInteractor->Initialize();

//    renderWindowInteractor->Start();

}
