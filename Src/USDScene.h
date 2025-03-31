#pragma once

#include <string>

//Usd
#include "pxr/usd/usd/stage.h"

namespace RendererAssets
{
    const std::string Cube = std::string("C:/Users/olive/Documents/DXRenderer/Meshes/Cube.usda");
}

class USDScene
{
public:
    void LoadScene(const std::string& Path);

    void LoadExampleCube() { LoadScene(RendererAssets::Cube); } 

private:
    pxr::UsdStageRefPtr Stage;

    std::vector<std::shared_ptr<class RenderMesh>> Meshes;
    
};
