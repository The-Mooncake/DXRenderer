#pragma once

#include <string>

//Usd
#include "pxr/usd/usd/stage.h"

namespace RendererAssets
{
    const std::string Tri =         std::string("C:/Users/olive/Documents/DXRenderer/Meshes/Triangle.usda");
    const std::string QuadTri =     std::string("C:/Users/olive/Documents/DXRenderer/Meshes/QuadTriangulated.usda");
    const std::string Quad =        std::string("C:/Users/olive/Documents/DXRenderer/Meshes/Quad.usda");
    const std::string Cube =        std::string("C:/Users/olive/Documents/DXRenderer/Meshes/Cube.usda");
    
}

class USDScene
{
public:
    void LoadScene(const std::string& Path);

    const std::vector<std::shared_ptr<class RenderMesh>> GetMeshes() const { return Meshes; }
    const bool IsYUp() const { return bIsYUp; }
    
    // Example files.
    void LoadExampleTri() { LoadScene(RendererAssets::Tri); } 
    void LoadExampleQuadTri() { LoadScene(RendererAssets::QuadTri); } 
    void LoadExampleQuad() { LoadScene(RendererAssets::Quad); } 
    void LoadExampleCube() { LoadScene(RendererAssets::Cube); } 

private:
    pxr::UsdStageRefPtr Stage;

    std::vector<std::shared_ptr<class RenderMesh>> Meshes;

    bool bIsYUp = true;
};
