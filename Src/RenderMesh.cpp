#include "RenderMesh.h"

#include <iostream>

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/sphere.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/tokens.h"


using namespace pxr;

namespace 
{
    char const* AttrIndices = "faceVertexIndices";;
    char const* AttrPositions = "points";
    char const* AttrNormals = "normals";
    char const* AttrUVs = "primvars:st";
}

RenderMesh::RenderMesh(const USDScene* InReader)
{
    if (InReader == nullptr)
    {
        std::cerr << "Error in USDReader::RenderMesh, In Reader is nullptr" << std::endl;
        return;
    }

    Reader = InReader;
}

bool RenderMesh::ValidatePrim(pxr::UsdPrim& Mesh)
{
    bool bHasIndices = Mesh.HasAttribute(TfToken(AttrIndices));
    bool bHasPoints = Mesh.HasAttribute(TfToken(AttrPositions));
    bool bHasNormals = Mesh.HasAttribute(TfToken(AttrNormals));
    bool bHasUVs = Mesh.HasAttribute(TfToken(AttrUVs));

    return bHasIndices && bHasPoints && bHasNormals && bHasUVs;
}

void RenderMesh::Load(UsdPrim& Mesh)
{
    if (!ValidatePrim(Mesh))
    {
        std::cout << "USDReader::LoadMesh: Mesh at '" << Mesh.GetDisplayName() << "' does not have valid data!" << "\n";
        return; 
    }

    SharedMeshData = std::make_shared<MeshData>();
    
    CopyData<int, uint32_t>(Mesh, TfToken(AttrIndices), SharedMeshData->Indices);
    CopyDXFloat3Data<GfVec3f>(Mesh, TfToken(AttrPositions), SharedMeshData->Positions);
    CopyDXFloat3Data<GfVec3f>(Mesh, TfToken(AttrNormals), SharedMeshData->Normals);
    CopyDXFloat2Data<GfVec2f>(Mesh, TfToken(AttrUVs), SharedMeshData->UVs);
}

template <typename SrcT, typename DestT>
bool RenderMesh::CopyData(UsdPrim& Mesh, const TfToken& AttrName, std::vector<DestT>& DestArray)
{
    VtArray<SrcT> UsdArray;
    UsdAttribute ArrayAttr = Mesh.GetAttribute(AttrName);
    ArrayAttr.Get<VtArray<SrcT>>(&UsdArray);
    
    DestArray.reserve(UsdArray.size());
    for (size_t i = 0; i < UsdArray.size(); ++i)
    {
        DestArray.emplace_back(UsdArray[i]);           
    }

    return DestArray.size() == UsdArray.size();
}

template <typename SrcT>
bool RenderMesh::CopyDXFloat3Data(UsdPrim& Mesh, const TfToken& AttrName, std::vector<DirectX::XMFLOAT3>& DestArray)
{
    VtArray<SrcT> UsdArray;
    UsdAttribute ArrayAttr = Mesh.GetAttribute(AttrName);
    ArrayAttr.Get<VtArray<SrcT>>(&UsdArray);
    
    DestArray.reserve(UsdArray.size());
    for (size_t i = 0; i < UsdArray.size(); ++i)
    {
        DirectX::XMFLOAT3 Element;
        Element.x = UsdArray[i][0];
        Element.y = UsdArray[i][1];
        Element.z = UsdArray[i][2];
        
        DestArray.emplace_back(Element);           
    }

    return DestArray.size() == UsdArray.size();
}

template <typename SrcT>
bool RenderMesh::CopyDXFloat2Data(pxr::UsdPrim& Mesh, const pxr::TfToken& AttrName,
    std::vector<DirectX::XMFLOAT2>& DestArray)
{
    VtArray<SrcT> UsdArray;
    UsdAttribute ArrayAttr = Mesh.GetAttribute(AttrName);
    ArrayAttr.Get<VtArray<SrcT>>(&UsdArray);
    
    DestArray.reserve(UsdArray.size());
    for (size_t i = 0; i < UsdArray.size(); ++i)
    {
        DirectX::XMFLOAT2 Element;
        Element.x = UsdArray[i][0];
        Element.y = UsdArray[i][1];
        
        DestArray.emplace_back(Element);           
    }

    return DestArray.size() == UsdArray.size();
}
