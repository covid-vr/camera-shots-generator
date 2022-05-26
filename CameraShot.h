#ifndef CAMERASHOT_H
#define CAMERASHOT_H

#include <mitkImage.h>
#include <mitkPointSet.h>
#include <mitkStandaloneDataStorage.h>

#include "vtkRenderer.h"
#include <mitkTransferFunction.h>

#include <itkImage.h>
#include <QString>
#include <QColor>

#ifndef DOXYGEN_IGNORE

class QLineEdit;

class CameraShot
{
	public:
    	CameraShot(int argc, char *argv[]);
    	~CameraShot();
    	virtual void Initialize();

	protected:
		void Load(int argc, char *argv[]);
		virtual void InitializeRenderWindow();

		mitk::StandaloneDataStorage::Pointer dataStorageOriginal;
		mitk::StandaloneDataStorage::Pointer dataStorageModified;

		mitk::DataNode::Pointer resultNode;

		QColor backgroundColor;

		QString transferFunctionFile = "../tf/tf6.xml";
		QString outputDir;   
		QString lastFile;
		QString PNGExtension = "PNG File (*.png)";
		QString JPGExtension = "JPEG File (*.jpg)";
		int widthSize = 512;
		int heightSize = 512;
		int slicesCount = 82;
		bool axisBool[7] = {false};

	private:
		void SetDefaultTransferFunction(mitk::TransferFunction::Pointer tf);
		void CaptureView(vtkCamera* vtkCam, vtkRenderer* renderer, int viewNumber, int shotsNumber, double horizontalDeltaDegrees, double verticalDeltaDegrees, unsigned int magnifierValue, QString filter);
		void TakeScreenshot(vtkRenderer* renderer, unsigned int magnificationFactor, QString fileName, QString filter = "", QColor backgroundColor = QColor(255,255,255));
		QString GetNextFileName(QString filePath, QString firstFileName, QString nextFileName);
		void CreateDir(QString dirPath);
    
};

#endif
#endif

