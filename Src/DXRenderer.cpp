
// Windows
#include <Windows.h>

// Renderer
#include "MainWindow.h"

// USD
// Useful USD References: https://github.com/LittleCoinCoin/OpenUSD-setup-vcpkg-template/blob/main/OpenUSD-setup-vcpkg/src/main.cpp

#include "pxr/usd/usd/stage.h"
#include <pxr/usd/usdGeom/xform.h>
#include "pxr/usd/usdGeom/sphere.h"
#include "pxr/usd/sdf/path.h"

#include <iostream>
#include <string>

// Force Nvidia GPU - e.g in laptop cases.
extern "C" { __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001; }

int WINAPI WinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{

	pxr::UsdStageRefPtr stage = pxr::UsdStage::CreateNew("HelloWorld.usda");
	pxr::UsdGeomXform xformPrim = pxr::UsdGeomXform::Define(stage, pxr::SdfPath("/hello"));
	pxr::UsdGeomSphere spherePrim = pxr::UsdGeomSphere::Define(stage, pxr::SdfPath("/hello/world"));

	std::string fileResult;	
	stage->GetRootLayer()->ExportToString(&fileResult);
	std::cout << "Content of file HelloWorld.usda:\n" << fileResult << std::endl;

	stage->GetRootLayer()->Save(); // Save the stage at the same location as the application.

    MainWindow App = MainWindow(hInstance);
    const int ExitCode = App.Run();

    return ExitCode;
}

