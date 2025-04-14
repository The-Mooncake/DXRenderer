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

void MeshData::ProcessVertices(bool bIsYUp)
{
    const size_t NumVerts = Positions.size();
    Vertices.reserve(NumVerts);

    for (size_t Idx = 0; Idx < NumVerts; Idx++ )
    {
        Vertex Vtx;
        if (bIsYUp)
        {
            Vtx.Position = Positions[Idx];
        }
        else
        {
            // This is wrong, need the correct solution for swizzling input vectors.
            //Vtx.Position = DirectX::XMFLOAT3(Positions[Idx].y, -Positions[Idx].x, Positions[Idx].z); // Works for tri.
            Vtx.Position = DirectX::XMFLOAT3(Positions[Idx].x, Positions[Idx].z, Positions[Idx].y);  // Works for quad.
        }
        Vtx.Colour = Colours[Idx];
        Vertices.emplace_back(Vtx);
    }
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

void RenderMesh::GenerateVertexColour(std::shared_ptr<MeshData> MeshData)
{
    const size_t NumVerts = MeshData->Positions.size();
    MeshData->Colours.reserve(NumVerts);
    
    for (size_t Idx = 0; Idx < NumVerts; Idx++)
    {
        uint8_t Remainder = Idx % 3; 
        switch (Remainder)
        {
        case 0:
            MeshData->Colours.push_back(DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f));
            break;
        case 1:
            MeshData->Colours.push_back(DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));
            break;
        case 2:
            MeshData->Colours.push_back(DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f));
            break;
        default:
            MeshData->Colours.push_back(DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
            break;
        }
    }
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

    // Generate colour
    GenerateVertexColour(SharedMeshData);

    // Process data to render data.
    SharedMeshData->ProcessVertices(Reader->IsYUp());
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
