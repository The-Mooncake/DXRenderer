#pragma once

#include "USDScene.h"
#include "pch.h"
#include "Renderer.h"

// Has lots of useful accessors:
// https://openusd.org/dev/api/class_usd_geom_point_based.html
#include "pxr/usd/usdGeom/pointBased.h"  

// Struct of vertex vector data, e.g. vtx positions...
struct MeshData
{
public:
    // Render Data
    std::vector<uint32_t> Indices;
    std::vector<Vertex> Vertices;

    // Import Data.
    std::vector<DirectX::XMFLOAT3> Positions;
    std::vector<DirectX::XMFLOAT3> Normals;
    std::vector<DirectX::XMFLOAT2> UVs;
    std::vector<DirectX::XMFLOAT4> Colours;
    
    DirectX::XMFLOAT3 VectorToRenderSpace(bool bIsYUp, size_t Idx, std::vector<DirectX::XMFLOAT3>& Data);
    void ProcessVertices(bool bIsYUp);
};

class RenderMesh
{
public:
    RenderMesh(const USDScene* InReader);
    void Load(class pxr::UsdPrim& InMesh);

    std::shared_ptr<MeshData> GetMeshData() { return SharedMeshData; }

private:
    bool ValidatePrim(pxr::UsdPrim& Mesh);
    
    template<typename SrcT, typename DestT>
    bool CopyData(const pxr::TfToken& AttrName, std::vector<DestT>& DestArray);

    // Dx data accessors are different to usd types, custom implementations for copying data required.
    template<typename SrcT>
    bool CopyDXFloat3Data(const pxr::TfToken& AttrName, std::vector<DirectX::XMFLOAT3>& DestArray);

    template<typename SrcT>
    bool CopyDXFloat2Data(const pxr::TfToken& AttrName, std::vector<DirectX::XMFLOAT2>& DestArray);

    // Helpers
    void GenerateVertexColour(std::shared_ptr<MeshData> MeshData);

    void TriangulateIndices(std::shared_ptr<MeshData> MeshData);

    void TriangulateFaceVarying3F(std::shared_ptr<MeshData> MeshData, std::vector<DirectX::XMFLOAT3>& Data); 
    
private:
    const USDScene* Reader = nullptr;
    pxr::UsdPrim* Mesh = nullptr;
    pxr::UsdGeomPointBased MeshPointBased;
    std::shared_ptr<MeshData> SharedMeshData;
};






