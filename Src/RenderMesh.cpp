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
    char const* AttrFaceVtxCounts = "faceVertexCounts";
}

DirectX::XMFLOAT3 MeshData::PositionToRenderSpace(bool bIsYUp, size_t Idx)
{
    if (bIsYUp)
    {
        return Positions[Idx];
    }
    else
    {
        return DirectX::XMFLOAT3(Positions[Idx].x, Positions[Idx].z, Positions[Idx].y);
    }
}

void MeshData::ProcessVertices(bool bIsYUp)
{
    const size_t NumVerts = Positions.size();
    Vertices.reserve(NumVerts);

    for (size_t Idx = 0; Idx < NumVerts; Idx++ )
    {
        Vertex Vtx;
        Vtx.Position = PositionToRenderSpace(bIsYUp, Idx);
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

bool RenderMesh::ValidatePrim(UsdPrim& Mesh)
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

void RenderMesh::Load(UsdPrim& InMesh)
{
    if (!ValidatePrim(InMesh))
    {
        std::cout << "USDReader::LoadMesh: Mesh at '" << InMesh.GetDisplayName() << "' does not have valid data!" << "\n";
        return; 
    }

    Mesh = &InMesh;
    SharedMeshData = std::make_shared<MeshData>();
    
    CopyData<int, uint32_t>(TfToken(AttrIndices), SharedMeshData->Indices);
    CopyDXFloat3Data<GfVec3f>(TfToken(AttrPositions), SharedMeshData->Positions);
    CopyDXFloat3Data<GfVec3f>(TfToken(AttrNormals), SharedMeshData->Normals);
    CopyDXFloat2Data<GfVec2f>(TfToken(AttrUVs), SharedMeshData->UVs);

    // Generate colour
    GenerateVertexColour(SharedMeshData);

    // Triangulate Mesh
    Triangulate(SharedMeshData);

    // Process data to render data.
    SharedMeshData->ProcessVertices(Reader->IsYUp());
}

template <typename SrcT, typename DestT>
bool RenderMesh::CopyData(const TfToken& AttrName, std::vector<DestT>& DestArray)
{
    VtArray<SrcT> UsdArray;
    UsdAttribute ArrayAttr = Mesh->GetAttribute(AttrName);
    ArrayAttr.Get<VtArray<SrcT>>(&UsdArray);
    
    DestArray.reserve(UsdArray.size());
    for (size_t i = 0; i < UsdArray.size(); ++i)
    {
        DestArray.emplace_back(UsdArray[i]);           
    }

    return DestArray.size() == UsdArray.size();
}

template <typename SrcT>
bool RenderMesh::CopyDXFloat3Data(const TfToken& AttrName, std::vector<DirectX::XMFLOAT3>& DestArray)
{
    VtArray<SrcT> UsdArray;
    UsdAttribute ArrayAttr = Mesh->GetAttribute(AttrName);
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
bool RenderMesh::CopyDXFloat2Data(const pxr::TfToken& AttrName,
    std::vector<DirectX::XMFLOAT2>& DestArray)
{
    VtArray<SrcT> UsdArray;
    UsdAttribute ArrayAttr = Mesh->GetAttribute(AttrName);
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

// Simple (Silly) triangulation algorithm that sets the indices for the mesh.
void RenderMesh::Triangulate(std::shared_ptr<MeshData> MeshData)
{
    VtArray<int> FaceVtxCounts;
    UsdAttribute ArrayAttr = Mesh->GetAttribute(TfToken(AttrFaceVtxCounts));
    ArrayAttr.Get<VtArray<int>>(&FaceVtxCounts);
    
    bool bIsTriangulated = true;
    size_t TriCount = 0;
    for (size_t i = 0; i < FaceVtxCounts.size(); ++i)
    {
        const int FaceCount = FaceVtxCounts[i];
        switch (FaceCount)
        {
        case 0:
        case 1:
        case 2:
            break;
        case 3:
            TriCount++;
            break;
        default:
            const int Remainder = FaceCount % 3;
            const int TriMultiples = FaceCount / 3;
            TriCount += TriMultiples + Remainder;
            bIsTriangulated = false;
            break;
        }
    }
    if (bIsTriangulated) return; // Early out.
    
    const size_t TriVtxCount = TriCount * 3;
    std::vector<uint32_t> TempIndices;
    TempIndices.reserve(TriVtxCount);
    
    size_t SrcOffset = 0;
    size_t TempOffset = 0;
    for (size_t Idx = 0; Idx < FaceVtxCounts.size(); ++Idx)
    {
        const uint32_t VtxCount = FaceVtxCounts[Idx];

        switch (VtxCount)
        {
        case 3:
            SrcOffset += 3;
            TempOffset += 3;
            break;
        case 4:
            {
                // [0, 1, 3, 2] Quad
                // [0, 1, 3, 2, 0, 3] 2 Tris: (0, 1, 3) & (2, 0, 3)
                TempIndices.resize(TempIndices.size() + 6);
                memcpy(TempIndices.data() + TempOffset, MeshData->Indices.data() + SrcOffset, 4 * sizeof(uint32_t));
                TempIndices[TempOffset + 4] = MeshData->Indices[SrcOffset];
                TempIndices[TempOffset + 5] = MeshData->Indices[SrcOffset + 2];

                SrcOffset += 4;
                TempOffset += 6;
                break;
            }
        default:
            std::cout << "Mesh triangulation not supported with face count: " << VtxCount << std::endl;
            return;
        }
    }
    
    MeshData->Indices = TempIndices; // Probably better way of assigning.
}
