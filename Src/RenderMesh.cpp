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
    char const* AttrUVs = "primvars:st";
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

void MeshData::ProcessVertices(bool bIsYUp)
{
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
    bool bHasUVs = Mesh.HasAttribute(TfToken(AttrUVs));

    return bIsPointBased && bHasIndices && bHasPoints && bHasNormals && bHasUVs;
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

    Mesh = InMesh;
    MeshPointBased = static_cast<UsdGeomPointBased>(InMesh);
    SharedMeshData = std::make_shared<MeshData>();

    TriangulateFaceVarying3F(SharedMeshData, SharedMeshData->Normals);
    
    CopyData<int, uint32_t>(UsdGeomTokens->faceVertexIndices, SharedMeshData->Indices);
    //CopyDXFloat3Data<GfVec3f>(UsdGeomTokens->points, SharedMeshData->Positions);
    CopyDXFloat3Data<GfVec3f>(UsdGeomTokens->normals, SharedMeshData->Normals);
    CopyDXFloat2Data<GfVec2f>(TfToken(AttrUVs), SharedMeshData->UVs);

    // // Hack to test positions...
    // VtArray<GfVec3f> PointArray;
    // UsdAttribute PointAttr = MeshPointBased.GetPointsAttr(); //Mesh.GetAttribute(UsdGeomTokens->points);
    // PointAttr.Get<VtArray<GfVec3f>>(&PointArray);
    //
    // for (size_t i = 0; i < PointArray.size(); ++i)
    // {
    //     DirectX::XMFLOAT3 Element;
    //     Element.x = PointArray[i][0];
    //     Element.y = PointArray[i][1];
    //     Element.z = PointArray[i][2];
    //     
    //     SharedMeshData->Positions.emplace_back(Element);           
    // }
    // End hack to test positions.
    
    // Generate colour
    GenerateVertexColour(SharedMeshData);

    // Triangulate Mesh 
    //TriangulateIndices(SharedMeshData);

    // Triangulate data channels, these vary by interpolation type.
    if (MeshPointBased.GetNormalsInterpolation() == UsdGeomTokens->faceVarying)
    {
        //TriangulateFaceVarying3F(SharedMeshData, SharedMeshData->Normals);
    }

    // Process data to render data.
    SharedMeshData->ProcessVertices(Reader->IsYUp());
}

template <typename SrcT, typename DestT>
bool RenderMesh::CopyData(const TfToken& AttrName, std::vector<DestT>& DestArray)
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
bool RenderMesh::CopyDXFloat3Data(const TfToken& AttrName, std::vector<DirectX::XMFLOAT3>& DestArray)
{
    VtArray<SrcT> UsdArray;
    if (!Mesh.HasAttribute(AttrName))
    {
        std::cout << "Error: Mesh does not have attribute: " << AttrName << std::endl;
        return false;
    }
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
bool RenderMesh::CopyDXFloat2Data(const pxr::TfToken& AttrName,
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

// Simple (Silly) triangulation algorithm that sets the indices for the mesh.
void RenderMesh::TriangulateIndices(std::shared_ptr<MeshData> MeshData)
{
    VtArray<int> FaceVtxCounts;
    UsdAttribute ArrayAttr = Mesh.GetAttribute(UsdGeomTokens->faceVertexCounts);
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
    TempIndices.resize(TriVtxCount);
    
    size_t SrcOffset = 0;
    size_t TempOffset = 0;
    for (size_t Idx = 0; Idx < FaceVtxCounts.size(); ++Idx)
    {
        const uint32_t VtxCount = FaceVtxCounts[Idx];

        switch (VtxCount)
        {
        case 3:
            memcpy(TempIndices.data() + TempOffset, MeshData->Indices.data() + SrcOffset, 3 * sizeof(uint32_t));
            SrcOffset += 3;
            TempOffset += 3;
            break;
        case 4:
            {
                // [0, 1, 3, 2] Quad
                // [0, 1, 3, 2, 0, 3] 2 Tris: (0, 1, 3) & (2, 0, 3)
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

void RenderMesh::TriangulateFaceVarying3F(std::shared_ptr<MeshData> MeshData, std::vector<DirectX::XMFLOAT3>& Data)
{
    // Triangulate the data to be per vertex instead of shared vertex.
    // e.g: 11 normal angles, initially only 7 shared points, 15 individual points after triangulation.
    

    // Use HdMeshUtil class which has triangulation algorithms, methods described here:
    // https://github.com/PixarAnimationStudios/OpenUSD/issues/329
    // This can handle triangulation of the mesh anyway, so should use this...


    // Copilot example
    //
    // // Load stage and mesh
    // UsdStageRefPtr stage = UsdStage::Open("your_file.usd");
    // UsdPrim meshPrim = stage->GetPrimAtPath(SdfPath("/YourMesh"));
    // UsdGeomMesh mesh(meshPrim);
    //
    // // Get topology
    // UsdImagingMeshAdapter adapter;
    // VtValue topologyValue = adapter.GetTopology(meshPrim, meshPrim.GetPath(), UsdTimeCode::Default());
    // HdMeshTopology topology = topologyValue.Get<HdMeshTopology>();
    // HdMeshUtil meshUtil(&topology, meshPrim.GetPath());
    //
    // // Get face-varying normals
    // UsdGeomPrimvar normalPrimvar = mesh.GetPrimvar(TfToken("normals"));
    // VtVec3fArray rawNormals;
    // if (!normalPrimvar || !normalPrimvar.Get(&rawNormals, UsdTimeCode::Default())) {
    //     throw std::runtime_error("Failed to retrieve face-varying normals");
    // }
    //
    // TfToken interpolation = normalPrimvar.GetInterpolation();
    // if (interpolation != UsdGeomTokens->faceVarying) {
    //     throw std::runtime_error("Normals are not face-varying");
    // }
    //
    // // Remap normals to triangulated topology
    // VtVec3fArray triangulatedNormals;
    // meshUtil.ComputeTriangulatedFaceVaryingPrimvar(rawNormals, &triangulatedNormals);
    //
    // // Optional: Get face-varying UVs
    // UsdGeomPrimvar uvPrimvar = mesh.GetPrimvar(TfToken("st"));
    // VtVec2fArray rawUVs;
    // VtVec2fArray triangulatedUVs;
    // if (uvPrimvar && uvPrimvar.GetInterpolation() == UsdGeomTokens->faceVarying) {
    //     if (uvPrimvar.Get(&rawUVs, UsdTimeCode::Default())) {
    //         meshUtil.ComputeTriangulatedFaceVaryingPrimvar(rawUVs, &triangulatedUVs);
    //     }
    // }
    //
    // // Output sizes for verification
    // std::cout << "Triangulated Normals: " << triangulatedNormals.size() << std::endl;
    // std::cout << "Triangulated UVs: " << triangulatedUVs.size() << std::endl;
    //


    // End Copilot example


    

    UsdPrim Prim = MeshPointBased.GetPrim();
    
    // Setup triangulation tools.
    UsdImagingMeshAdapter Adapter;
    VtValue Topology = Adapter.GetTopology(Mesh, Prim.GetPath(), UsdTimeCode::Default());
    HdMeshUtil MeshUtil(&Topology.Get<HdMeshTopology>(), Prim.GetPath());
    
    // Original mesh values
    VtArray<GfVec3f> InPositions;
    UsdAttribute AttrPositions = Mesh.GetAttribute(UsdGeomTokens->points);
    AttrPositions.Get<VtArray<GfVec3f>>(&InPositions);
    
    VtArray<GfVec3f> InNormals;
    UsdAttribute AttrNormals = Mesh.GetAttribute(UsdGeomTokens->normals);
    AttrNormals.Get<VtArray<GfVec3f>>(&InNormals);

    // Get new triangulation indices.
    VtVec3iArray NewIndices;
    VtIntArray NewParams; 
    MeshUtil.ComputeTriangleIndices(&NewIndices, &NewParams);

    // Triangulate the positions.
    // VtValue InPositionsVal(InPositions);
    // VtValue OutPositionsVal;
    // HdVtBufferSource PositionsBuffer(TfToken("TempP"), InPositionsVal);
    // MeshUtil.ComputeTriangulatedFaceVaryingPrimvar(PositionsBuffer.GetData(), static_cast<int>(PositionsBuffer.GetNumElements()), PositionsBuffer.GetTupleType().type, &OutPositionsVal);
    // VtArray<GfVec3f> TriedPositions = OutPositionsVal.Get<VtArray<GfVec3f>>();
    VtVec3dArray TriedPositions;
    for (const pxr::GfVec3i& tri : NewIndices) {
        TriedPositions.push_back(InPositions[tri[0]]);
        TriedPositions.push_back(InPositions[tri[1]]);
        TriedPositions.push_back(InPositions[tri[2]]);

        // DirectX::XMFLOAT3 Element0;
        // Element0.x = InPositions[tri[0]][0];
        // Element0.y = InPositions[tri[0]][1];
        // Element0.z = InPositions[tri[0]][2];
        // SharedMeshData->Positions.emplace_back(Element0);
        //
        //
    }
    for (const GfVec3d& Pos : TriedPositions)
    {
        DirectX::XMFLOAT3 Element0;
        Element0.x = Pos[0];
        Element0.y = Pos[1];
        Element0.z = Pos[2];
        SharedMeshData->Positions.emplace_back(Element0);
    }


    
    // Triangulate the normals.
    VtValue InNormalsVal(InNormals);
    VtValue OutNormalsVal;
    HdVtBufferSource NormalsBuffer(TfToken("TempN"), InNormalsVal);
    MeshUtil.ComputeTriangulatedFaceVaryingPrimvar(NormalsBuffer.GetData(), static_cast<int>(NormalsBuffer.GetNumElements()), NormalsBuffer.GetTupleType().type, &OutNormalsVal);
    VtArray<GfVec3f> TriedNrms = OutNormalsVal.Get<VtArray<GfVec3f>>();
    
    // Build new geom mesh...
    TriMesh = UsdGeomMesh(Prim);
    TriMesh.SetNormalsInterpolation(UsdGeomTokens->faceVarying);
    
    UsdAttribute NewFaceVtxCountAttr = TriMesh.CreateFaceVertexCountsAttr();
    VtArray<int> FaceVtxCount(NewIndices.size(), 3);
    if (!NewFaceVtxCountAttr.Set(FaceVtxCount))
    {
        std::cout << "TriMesh failed to set the new face vertex counts!" << std::endl;
    }
    
    UsdAttribute NewPointsAttr = TriMesh.CreatePointsAttr();
    if (NewPointsAttr.Set(TriedPositions))
    
    //VtArray<GfVec3f> NewPoints;
    //MeshPointBased.GetPointsAttr().Get<VtArray<GfVec3f>>(&NewPoints);
    //if (!NewPointsAttr.Set(NewPoints))
    {
        std::cout << "TriMesh failed to set the point positions!" << std::endl;
    }

    // Convert from VtVec3iArray to VtArray<int> and set indices.
    UsdAttribute NewIndicesAttr = TriMesh.CreateFaceVertexIndicesAttr();
    VtArray<int> NewArrayIndices(NewIndices.size() * 3);
    for (size_t i = 0; i < NewIndices.size(); i++)
    {
        // Normal indices and positions indices are not the same. Mesh ends up being incorrect.
        
        NewArrayIndices[i*3] = i*3;
        NewArrayIndices[i*3 + 1] = i*3 + 1;
        NewArrayIndices[i*3 + 2] = i*3 + 2;

        // GfVec3i Indices = NewIndices[i];
        //NewArrayIndices[i*3] = NewIndices[i][0];
        //NewArrayIndices[i*3 + 1] = NewIndices[i][1];
        //NewArrayIndices[i*3 + 2] = NewIndices[i][2];
    }
    // Set new attrs into the triangulate mesh.
    if (!NewIndicesAttr.Set(NewArrayIndices))
    {
        std::cout << "TriMesh failed to set the new indices!" << std::endl;
    }
    
    UsdAttribute NormalsAttr = TriMesh.CreateNormalsAttr();
    if (!NormalsAttr.Set(TriedNrms))
    {
        std::cout << "TriMesh failed to set the triangulated normals!" << std::endl;
    }
    
    Mesh = TriMesh.GetPrim();
    MeshPointBased = static_cast<UsdGeomPointBased>(TriMesh);
    
}
