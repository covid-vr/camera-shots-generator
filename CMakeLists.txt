cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

set (CMAKE_AUTOMOC ON)

project(camera-shots)

set(MITK_SUPERBUILD_DIR "$ENV{MITK_BUILD_PATH}")
set(QT_DIR "$ENV{QT_ROOT_PATH}")

set(CMAKE_PREFIX_PATH
		"${MITK_SUPERBUILD_DIR}/MITK-build"
		"${MITK_SUPERBUILD_DIR}/ep/src/VTK/CMake/"
		"${QT_DIR}/gcc_64/lib/cmake/Qt5"
	)

find_package(MITK CONFIG REQUIRED)
find_package(OpenMP REQUIRED)
find_package(VTK COMPONENTS
	vtkCommonCore
	vtkIOExport
	QUIET
)

set(cpp_files
	CameraShot.cpp
	CameraShotMain.cpp
)

set(header_files
	CameraShot.h
)

add_executable(${PROJECT_NAME} ${cpp_files})
target_link_libraries(${PROJECT_NAME}
	MitkCore 
	MitkSceneSerializationBase
	MitkQtWidgetsExt
	EGL
	vtkCommonCore
	vtkIOExport
)
