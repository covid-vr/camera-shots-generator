#include "CameraShot.h"

#include "QmitkRenderWindow.h"
#include "QmitkSliceWidget.h"

#include "mitkProperties.h"
#include "mitkRenderingManager.h"
#include "mitkImageAccessByItk.h"
#include "mitkIOUtil.h"

#include "mitkTransferFunction.h"
#include "mitkTransferFunctionProperty.h"
#include "mitkTransferFunctionPropertySerializer.h"

#include "vtkRenderer.h"
#include "vtkImageWriter.h"
#include "vtkJPEGWriter.h"
#include "vtkPNGWriter.h"
#include "vtkRenderLargeImage.h"
#include <vtkCamera.h>
#include <QString>
#include <QStringList>
#include <QDir>

CameraShot::CameraShot(int argc, char *argv[]) {
	Load(argc, argv);
}

CameraShot::~CameraShot() {}

void CameraShot::Initialize() {
	mitk::StandaloneDataStorage::SetOfObjects::ConstPointer dataNodes = this->dataStorageOriginal->GetAll();
  
	//Reading each image and setting TransferFunction 
	for(int image_index = 0; image_index < dataNodes->Size(); image_index++) {
		mitk::Image::Pointer currentMitkImage = dynamic_cast<mitk::Image *>(dataNodes->at(image_index)->GetData());
    
		this->resultNode = mitk::DataNode::New();
		this->dataStorageModified->Add(this->resultNode);
		this->resultNode->SetData(currentMitkImage);
		this->resultNode->SetProperty("binary", mitk::BoolProperty::New(false));

		// Volume Rendering and Transfer function
		this->resultNode->SetProperty("volumerendering", mitk::BoolProperty::New(true));
		this->resultNode->SetProperty("volumerendering.usegpu", mitk::BoolProperty::New(true));
		this->resultNode->SetProperty("layer", mitk::IntProperty::New(1));

		mitk::TransferFunction::Pointer tf = mitk::TransferFunction::New();
		tf->InitializeByMitkImage(currentMitkImage);
		tf = mitk::TransferFunctionPropertySerializer::DeserializeTransferFunction(this->transferFunctionFile.toLatin1());

		this->resultNode->SetProperty("TransferFunction", mitk::TransferFunctionProperty::New(tf.GetPointer()));
		mitk::LevelWindowProperty::Pointer levWinProp = mitk::LevelWindowProperty::New();
		mitk::LevelWindow levelwindow;
		levelwindow.SetAuto(currentMitkImage);
		levWinProp->SetLevelWindow(levelwindow);
		this->resultNode->SetProperty("levelwindow", levWinProp);
	}
	this->InitializeRenderWindow();

}

QString CameraShot::GetNextFileName(QString fileDir, QString fileName, QString secondFileName) {
	int c = 1;
	while (QFile::exists(fileDir+fileName)) {
		c++;
		fileName = secondFileName; 
		fileName += QString::number(c);
		fileName += ".png";
	}
	return fileName;
}


void CameraShot::CaptureView(
				vtkCamera* vtkCam,
				vtkRenderer* renderer,
				int viewNumber,
				int shotsNumber,
				double horizontalDeltaDegrees,
				double verticalDeltaDegrees,
				unsigned int magnifierValue,
				QString filter
) {	
	
	QString fileDir = this->outputDir + "/axis" + QString::number(viewNumber);
	QString firstFileName = "/1.png";
	QString nextFileName = "/";

	this->CreateDir(fileDir);
	
	//Horizontal Movement
	double deltaDegree = horizontalDeltaDegrees;
	double currentDegree = (double)deltaDegree*(shotsNumber/2);
	vtkCam->Azimuth(-1.0 * currentDegree);

	for(int i = 0; i < shotsNumber; i++){
		QString fileName = this->GetNextFileName(fileDir, firstFileName, nextFileName);
		if(i) {
			vtkCam->Azimuth(deltaDegree);
		}
		this->TakeScreenshot(renderer, magnifierValue, fileDir+fileName, filter, this->backgroundColor);
	}
	vtkCam->Azimuth( -(double)((1+shotsNumber)/2)*deltaDegree );

	//Vertical Movement
	deltaDegree = verticalDeltaDegrees;
	currentDegree = (double)deltaDegree*(shotsNumber/2);
	vtkCam->Elevation( -1.0*currentDegree );

	for(int i = 0; i < shotsNumber; i++) {
		QString fileName = this->GetNextFileName(fileDir, firstFileName, nextFileName);
		if(i) {
			vtkCam->Elevation(deltaDegree);
		}
		this->TakeScreenshot(renderer, magnifierValue, fileDir+fileName, filter, this->backgroundColor);
	}    
	vtkCam->Elevation(-(double)((1 + shotsNumber) / 2) * deltaDegree);
}


void CameraShot::InitializeRenderWindow() {
	mitk::StandaloneDataStorage::Pointer ds = mitk::StandaloneDataStorage::New();
	QmitkRenderWindow* renderWindow = new QmitkRenderWindow();

	// Tell the renderwindow which (part of) the tree to render
	renderWindow->GetRenderer()->SetDataStorage(dataStorageModified);

	// Use it as a 3D view
	renderWindow->GetRenderer()->SetMapperID(mitk::BaseRenderer::Standard3D);
	renderWindow->resize(this->widthSize, this->heightSize);

	renderWindow->setAttribute(Qt::WA_DontShowOnScreen);
	renderWindow->show();

	vtkRenderWindow* renderWindow2 = renderWindow->GetVtkRenderWindow();
	mitk::BaseRenderer* baserenderer = mitk::BaseRenderer::GetInstance(renderWindow2);
	auto renderer = baserenderer->GetVtkRenderer();

	// Reposition the camera to include all visible actors
	renderer->ResetCamera();
	vtkCamera* vtkCam = renderer->GetActiveCamera();
	vtkCam->Zoom(2.0);
	int magnifierValue = 1;
	if(nullptr == renderer) {
		return;
	}
	
	QString filter = PNGExtension;
	int shotsNumber = this->slicesCount / 2; 
	double horizontalDeltaDegrees = 1.2;
	double verticalDeltaDegrees =  1.2;

	vtkCam->Roll(90);
	vtkCam->Azimuth(-7.5);
	if(this->axisBool[1]) {
		this->CaptureView(vtkCam, renderer, 1, shotsNumber, horizontalDeltaDegrees, verticalDeltaDegrees, magnifierValue, filter);
	}

	vtkCam->Roll(4);
	vtkCam->Azimuth(90);
	vtkCam->Elevation(4);
	vtkCam->Roll(-90);	
	if(this->axisBool[2]) {
		this->CaptureView(vtkCam, renderer, 2, shotsNumber, horizontalDeltaDegrees, verticalDeltaDegrees, magnifierValue, filter);
	}

	vtkCam->Roll(90);
	vtkCam->Azimuth(90);
	vtkCam->Roll(-2.5);
	vtkCam->Roll(90);
	if(this->axisBool[3]) {
		this->CaptureView(vtkCam, renderer, 3, shotsNumber, horizontalDeltaDegrees, verticalDeltaDegrees, magnifierValue, filter);
	}

	vtkCam->Roll(90);
	vtkCam->Azimuth(-90);
	vtkCam->Roll(-90);
	if(this->axisBool[4]) {
		this->CaptureView(vtkCam, renderer, 4, shotsNumber, horizontalDeltaDegrees, verticalDeltaDegrees, magnifierValue, filter);
	}

	vtkCam->Azimuth(-90);
	if(this->axisBool[5]) {
		this->CaptureView(vtkCam, renderer, 5, shotsNumber, horizontalDeltaDegrees, verticalDeltaDegrees, magnifierValue, filter);
	}
	
	vtkCam->Azimuth(180);
	if(this->axisBool[6]) {
		this->CaptureView(vtkCam, renderer, 6, shotsNumber, horizontalDeltaDegrees, verticalDeltaDegrees, magnifierValue, filter);
	}
}

void CameraShot::CreateDir(QString fileDir){
	if(!QDir(fileDir).exists()) {
		QDir().mkdir(fileDir);
	}
}

void CameraShot::TakeScreenshot(vtkRenderer* renderer, unsigned int magnificationFactor, QString fileName, QString filter, QColor backgroundColor){
	if ((renderer == nullptr) ||(magnificationFactor < 1) || fileName.isEmpty()){
		return;
	}

	bool doubleBuffering(renderer->GetRenderWindow()->GetDoubleBuffer());
	renderer->GetRenderWindow()->DoubleBufferOff();

	vtkImageWriter* fileWriter = nullptr;

	QFileInfo fi(fileName);
	QString suffix = fi.suffix().toLower();

	if (suffix.isEmpty() || (suffix != "png" && suffix != "jpg" && suffix != "jpeg")) {
		if (filter == PNGExtension) {
			suffix = "png";
		} else if (filter == JPGExtension){
			suffix = "jpg";
		}
		fileName += "." + suffix;
	}

	if (suffix.compare("jpg", Qt::CaseInsensitive) == 0 || suffix.compare("jpeg", Qt::CaseInsensitive) == 0){
		vtkJPEGWriter* w = vtkJPEGWriter::New();
		w->SetQuality(100);
		w->ProgressiveOff();
		fileWriter = w;
	} else{ 
		fileWriter = vtkPNGWriter::New(); //default is png
	}

	vtkRenderLargeImage* magnifier = vtkRenderLargeImage::New();
	magnifier->SetInput(renderer);
	magnifier->SetMagnification(magnificationFactor);
	fileWriter->SetInputConnection(magnifier->GetOutputPort());
	fileWriter->SetFileName(fileName.toLatin1());

	double oldBackground[3];
	renderer->GetBackground(oldBackground);

	backgroundColor = backgroundColor;

	double bgcolor[] = {backgroundColor.red()/255.0, backgroundColor.green()/255.0, backgroundColor.blue()/255.0};
	renderer->SetBackground(bgcolor);

	fileWriter->Write();
	fileWriter->Delete();
	renderer->GetRenderWindow()->SetDoubleBuffer(doubleBuffering);
}

void CameraShot::Load(int argc, char *argv[]) {
	//*************************************************************************
	// Part I: Basic initialization
	//*************************************************************************

	this->dataStorageOriginal = mitk::StandaloneDataStorage::New();
	this->dataStorageModified = mitk::StandaloneDataStorage::New();

	int i = 1;
	while (i < argc){	
		if(strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--image") == 0) {
			mitk::StandaloneDataStorage::SetOfObjects::Pointer dataNodes = mitk::IOUtil::Load(argv[i + 1], *this->dataStorageOriginal);
			if (dataNodes->empty()){
				fprintf(stderr, "Could not open file %s \n\n", argv[i + 1]);
				exit(2);
			}
		}

		if(strcmp(argv[i], "-tf") == 0 || strcmp(argv[i], "--transfer_function") == 0){
			this->transferFunctionFile=argv[i + 1];
		}

		if(strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output_dir") == 0){
			this->outputDir=argv[i + 1];
    		this->CreateDir(this->outputDir);
		}

		if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--width") == 0){
			this->widthSize=atoi(argv[i + 1]);
		}

		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--height") == 0){
			this->heightSize=atoi(argv[i + 1]);
		}

		if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--slices_count") == 0){
			this->slicesCount=atoi(argv[i + 1]);
		}
		
		if(strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--color") == 0){	
			QString color = argv[i + 1];
			QStringList colorList = color.split(",");

			int rgb[3] = {255};
			for(int c = 0; c < colorList.length(); c++) {
				if(c > 2) {
					break;
				}
				int value = colorList.value(c).toInt();
				if (value > -1 && value < 256) {
					rgb[c] = value;			
				}
			}
			this->backgroundColor = QColor(rgb[0], rgb[1], rgb[2]);
		}

		if(strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--axis") == 0){
			for(int k = 0; k < 7; k++) {
				this->axisBool[k] = false;
			}
			QString axis = argv[i + 1];
			QStringList axisList = axis.split(",");

			for(int a = 0; a < axisList.length(); a++) {
				this->axisBool[axisList.value(a).toInt()] = true;
			}
		}

		i += 2;
	}
}

