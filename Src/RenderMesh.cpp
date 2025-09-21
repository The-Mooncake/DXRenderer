#include "RenderMesh.h"

#include <iostream>

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/sphere.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/tokens.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"
#include "pxr/usdImaging/usdImaging/meshAdapter.h"
#include "pxr/imaging/hd/meshUtil.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/bufferArray.h"
#include "pxr/imaging/hd/vtBufferSource.h"

// Test - Might need to move to cmakefile.
#define _CXX20_DEPRECATE_OLD_SHARED_PTR_ATOMIC_SUPPORT

using namespace pxr;

namespace 
{
    char const* TokenAttrUVs = "primvars:st";
}

DirectX::XMFLOAT3 MeshData::VectorToRenderSpace(bool bIsYUp, size_t Idx, std::vector<DirectX::XMFLOAT3>& Data)
{
    if (bIsYUp)
    {
        return Data[Idx];
    }
    else
    {
        return DirectX::XMFLOAT3(Data[Idx].x, Data[Idx].z, Data[Idx].y);
    }
}

void MeshData::GenerateVertexColour()
{
    const size_t NumVerts = Positions.size();
    Colours.reserve(NumVerts);
    
    for (size_t Idx = 0; Idx < NumVerts; Idx++)
    {
        const uint8_t Remainder = Idx % 3; 
        switch (Remainder)
        {
        case 0:
            Colours.push_back(DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f));
            break;
        case 1:
            Colours.push_back(DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));
            break;
        case 2:
            Colours.push_back(DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f));
            break;
        default:
            Colours.push_back(DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
            break;
        }
    }
}

void MeshData::ProcessVertices(bool bIsYUp)
{
    if (Colours.empty()){ GenerateVertexColour(); }
    
    const size_t NumVerts = Positions.size();
    Vertices.reserve(NumVerts);

    for (size_t Idx = 0; Idx < NumVerts; Idx++ )
    {
        Vertex Vtx;
        Vtx.Position = VectorToRenderSpace(bIsYUp, Idx, Positions);
        Vtx.Normals = VectorToRenderSpace(bIsYUp, Idx, Normals);
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
    bool bIsPointBased = Mesh.IsA(UsdGeomTokens->PointBased);
    
    bool bHasIndices = Mesh.HasAttribute(UsdGeomTokens->faceVertexIndices);
    bool bHasPoints = Mesh.HasAttribute(UsdGeomTokens->points);
    bool bHasNormals = Mesh.HasAttribute(UsdGeomTokens->normals);
    bool bHasUVs = Mesh.HasAttribute(TfToken(TokenAttrUVs));

    return bIsPointBased && bHasIndices && bHasPoints && bHasNormals && bHasUVs;
}

void RenderMesh::Load(UsdPrim& InMesh)
{
    if (!ValidatePrim(InMesh))
    {
        std::cout << "USDReader::LoadMesh: Mesh at '" << InMesh.GetDisplayName() << "' does not have valid data!" << "\n";
        return; 
    }

    Mesh = InMesh;
    SharedMeshData = std::make_shared<MeshData>();

    TriangulateUsdGeometry();
    
    // Process data to render data.
    SharedMeshData->ProcessVertices(Reader->IsYUp());
}

template <typename SrcT>
bool RenderMesh::CopyData_DXFloat2(const SrcT& SrcArray, std::vector<DirectX::XMFLOAT2>& DestArray)
{
    using ElementType = std::remove_reference<decltype( *SrcArray.data() )>::type;
    DestArray.reserve(SrcArray.size());
    for (const ElementType& Element : SrcArray)
    {
        DirectX::XMFLOAT2 OutElem;
        OutElem.x = Element[0];
        OutElem.y = Element[1];
        DestArray.emplace_back(OutElem);           
    }

    return DestArray.size() == SrcArray.size();
}

template <typename SrcT>
bool RenderMesh::CopyData_DXFloat3(const SrcT& SrcArray, std::vector<DirectX::XMFLOAT3>& DestArray)
{
    using ElementType = std::remove_reference<decltype( *SrcArray.data() )>::type;
    DestArray.reserve(SrcArray.size());
    for (const ElementType& Element : SrcArray)
    {
        DirectX::XMFLOAT3 OutElem;
        OutElem.x = Element[0];
        OutElem.y = Element[1];
        OutElem.z = Element[2];
        DestArray.emplace_back(OutElem);           
    }

    return DestArray.size() == SrcArray.size();
}

template <typename SrcT>
bool RenderMesh::CopyData_DXFloat4(const SrcT& SrcArray, std::vector<DirectX::XMFLOAT4>& DestArray)
{
    using ElementType = std::remove_reference<decltype( *SrcArray.data() )>::type;
    DestArray.reserve(SrcArray.size());
    for (const ElementType& Element : SrcArray)
    {
        DirectX::XMFLOAT4 OutElem;
        OutElem.x = Element[0];
        OutElem.y = Element[1];
        OutElem.z = Element[2];
        OutElem.w = Element[3];
        DestArray.emplace_back(OutElem);           
    }

    return DestArray.size() == SrcArray.size();
}

void RenderMesh::TriangulateUsdGeometry()
{
    // Use HdMeshUtil class which has triangulation algorithms, methods described here:
    // https://github.com/PixarAnimationStudios/OpenUSD/issues/329
    // This can handle triangulation of the mesh anyway, so should use this...

    // Setup triangulation tools.
    UsdImagingMeshAdapter Adapter;
    VtValue Topology = Adapter.GetTopology(Mesh, Mesh.GetPath(), UsdTimeCode::Default());
    HdMeshUtil MeshUtil(&Topology.Get<HdMeshTopology>(), Mesh.GetPath());

    // Calculate new triangulation indices.
    VtVec3iArray NewIndices;
    VtIntArray NewParams; 
    MeshUtil.ComputeTriangleIndices(&NewIndices, &NewParams);
    
    // In mesh positions
    VtArray<GfVec3f> InPositions;
    UsdAttribute AttrPositions = Mesh.GetAttribute(UsdGeomTokens->points);
    AttrPositions.Get<VtArray<GfVec3f>>(&InPositions);
    
    // Unroll the positions, Our Renderer does not currently support indexed rendering. Due to USD indexing.
    for (const GfVec3i& Tri : NewIndices)
    {
        for (size_t Idx = 0; Idx < 3; Idx++)
        {
            DirectX::XMFLOAT3 OutPosition;
            const GfVec3d& Pos = InPositions[Tri[Idx]];
            OutPosition.x = static_cast<float>(Pos[0]);
            OutPosition.y = static_cast<float>(Pos[1]);
            OutPosition.z = static_cast<float>(Pos[2]);
            SharedMeshData->Positions.push_back(OutPosition);
        }
    }
    
    // Create linear indices, 0...VertNum incrementally: 0, 1, 2...
    SharedMeshData->Indices.resize(NewIndices.size() * 3);
    std::iota(SharedMeshData->Indices.begin(), SharedMeshData->Indices.end(), 0);
    
    // Triangulate the normals.
    VtArray<GfVec3f> InNormals;
    UsdAttribute AttrNormals = Mesh.GetAttribute(UsdGeomTokens->normals);
    AttrNormals.Get<VtArray<GfVec3f>>(&InNormals);
    
    VtValue InNormalsVal(InNormals);
    VtValue OutNormalsVal;
    HdVtBufferSource NormalsBuffer(TfToken("TempN"), InNormalsVal);
    MeshUtil.ComputeTriangulatedFaceVaryingPrimvar(NormalsBuffer.GetData(), static_cast<int>(NormalsBuffer.GetNumElements()), NormalsBuffer.GetTupleType().type, &OutNormalsVal);
    VtArray<GfVec3f> TriedNrms = OutNormalsVal.Get<VtArray<GfVec3f>>();
    CopyData_DXFloat3<VtArray<GfVec3f>>(TriedNrms, SharedMeshData->Normals);

    // Triangulate Uvs
    const TfToken UVToken = TfToken(TokenAttrUVs);
    if (Mesh.HasAttribute(UVToken))
    {
        VtArray<GfVec2f> InUvs;
        UsdAttribute AttrUvs = Mesh.GetAttribute(UVToken);
        AttrUvs.Get<VtArray<GfVec2f>>(&InUvs);
    
        VtValue InUvsVal(InUvs);
        VtValue OutUvsVal;
        HdVtBufferSource UvsBuffer(TfToken("TempUvs"), InUvsVal);
        MeshUtil.ComputeTriangulatedFaceVaryingPrimvar(UvsBuffer.GetData(), static_cast<int>(UvsBuffer.GetNumElements()), UvsBuffer.GetTupleType().type, &OutUvsVal);
        VtArray<GfVec2f> TriedUvs = OutUvsVal.Get<VtArray<GfVec2f>>();
        CopyData_DXFloat2<VtArray<GfVec2f>>(TriedUvs, SharedMeshData->UVs);
    }
}
