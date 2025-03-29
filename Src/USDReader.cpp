#include "USDReader.h"

// USD
#include <iostream>

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/sphere.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/primRange.h"

// Useful USD References: https://github.com/LittleCoinCoin/OpenUSD-setup-vcpkg-template/blob/main/OpenUSD-setup-vcpkg/src/main.cpp

using namespace pxr;

void USDReader::LoadScene(const std::string& Path)
{
    //pxr::UsdStageLoadRules
    
    Stage =  UsdStage::Open(Path);
    UsdPrimRange Prims = Stage->TraverseAll();

    for (UsdPrim Prim : Prims)
    {
        std::cout << "Prim name: " << Prim.GetName() << std::endl;
        std::cout << "Path: " << Prim.GetPath() << std::endl;
        std::cout << "Type: " << Prim.GetTypeName() << std::endl;
        std ::cout << "\n";

        TfToken Type = Prim.GetTypeName();
        switch (Type)
        {
            case TfToken("Xform"):
                break;
            case TfToken("Mesh"):
                LoadMesh();
                break;

            default:
                std::cout << "Type: " << Type << " not supported." "\n";
                
        }
    }

}

void USDReader::LoadMesh()
{

    
}
