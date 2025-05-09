#pragma once

#include <string>

//Usd
#include "pxr/usd/usd/stage.h"

namespace RendererAssets
{
    const std::string Tri =         std::string("C:/Users/olive/Documents/DXRenderer/Meshes/Triangle.usda");
    const std::string QuadTri =     std::string("C:/Users/olive/Documents/DXRenderer/Meshes/QuadTriangulated.usda");
    const std::string Quad =        std::string("C:/Users/olive/Documents/DXRenderer/Meshes/Quad.usda");
    const std::string CubeTri =     std::string("C:/Users/olive/Documents/DXRenderer/Meshes/CubeTriangulated.usda");
    const std::string Cube =        std::string("C:/Users/olive/Documents/DXRenderer/Meshes/Cube.usda");
    const std::string FaceAndTri =  std::string("C:/Users/olive/Documents/DXRenderer/Meshes/FaceAndTri.usda");
    
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
    void LoadExampleCubeTri() { LoadScene(RendererAssets::CubeTri); } 
    void LoadExampleCube() { LoadScene(RendererAssets::Cube); }
    void LoadFaceAndTri() { LoadScene(RendererAssets::FaceAndTri); }

private:
    pxr::UsdStageRefPtr Stage;

    std::vector<std::shared_ptr<class RenderMesh>> Meshes;

    bool bIsYUp = true;
};
