#pragma once

#include <string>

//Usd
#include "pxr/usd/usd/stage.h"

namespace RendererAssets
{
    const std::string Cube = std::string("C:/Users/olive/Documents/DXRenderer/Meshes/Cube.usda");
}

class USDReader
{
public:
    void LoadScene(const std::string& Path);

    void LoadExampleCube() { LoadScene(RendererAssets::Cube); } 

private:

    void LoadMesh();

private:
    pxr::UsdStageRefPtr Stage;
    
};
