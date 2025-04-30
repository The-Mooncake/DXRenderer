#include "USDScene.h"

#include "RenderMesh.h"
#include <nvtx3/nvtx3.hpp>


// USD
#include <iostream>

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/sphere.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdGeom/xformCommonAPI.h"

// Useful USD References: https://github.com/LittleCoinCoin/OpenUSD-setup-vcpkg-template/blob/main/OpenUSD-setup-vcpkg/src/main.cpp

using namespace pxr;

void USDScene::LoadScene(const std::string& Path)
{
    nvtx3::scoped_range r{ "Load USD Scene" };

    Stage = UsdStage::Open(Path);

    TfToken UpAxis;  
    Stage->GetMetadata(UsdGeomTokens->upAxis, &UpAxis);
    bIsYUp = (UpAxis == UsdGeomTokens->y);
    
    UsdPrimRange Prims = Stage->TraverseAll();  

    for (UsdPrim Prim : Prims)
    {
        if (!Prim.IsValid()) { continue; }
        
        SdfPath PrimPath = Prim.GetPath();
        TfToken Type = Prim.GetTypeName();
        
        std::cout << "Prim name: " << Prim.GetName() << std::endl;
        std::cout << "Path: " << PrimPath << std::endl;
        std::cout << "Type: " << Type << std::endl;
        std ::cout << "\n";
        
        if (Type == TfToken("Xform"))
        {
            std::cout << "Processing Xform..." << "\n";
        }
        else if (Type == TfToken("Mesh"))
        {
            std::shared_ptr<RenderMesh> Mesh = std::make_shared<RenderMesh>(this);
            Mesh->Load(Prim);
            Meshes.emplace_back(Mesh);
        }
        else
        {
            std::cout << "Processing unsupported type: " << Prim.GetTypeName() << "\n";
        }
    }
}
