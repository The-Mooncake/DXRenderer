#pragma once

#include "USDScene.h"
#include "pch.h"
#include "Renderer.h"

// Has lots of useful accessors:
// https://openusd.org/dev/api/class_usd_geom_point_based.html
#include "pxr/usd/usdGeom/pointBased.h"  
#include "pxr/usd/usdGeom/mesh.h"  

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
    
    void ProcessVertices(bool bIsYUp);

private:
    DirectX::XMFLOAT3 VectorToRenderSpace(bool bIsYUp, size_t Idx, std::vector<DirectX::XMFLOAT3>& Data);
    void GenerateVertexColour();
};

class RenderMesh
{
public:
    RenderMesh(const USDScene* InReader);
    void Load(class pxr::UsdPrim& InMesh);

    std::shared_ptr<MeshData> GetMeshData() { return SharedMeshData; }

private:
    bool ValidatePrim(pxr::UsdPrim& Mesh);
    
    // DX Helpers to copy data of various sizes to the SharedMeshData arrays.
    template <typename SrcT>
    bool CopyData_DXFloat2(const SrcT& SrcArray, std::vector<DirectX::XMFLOAT2>& DestArray);
    
    template <typename SrcT>
    bool CopyData_DXFloat3(const SrcT& SrcArray, std::vector<DirectX::XMFLOAT3>& DestArray);
    
    template <typename SrcT>
    bool CopyData_DXFloat4(const SrcT& SrcArray, std::vector<DirectX::XMFLOAT4>& DestArray);

    // Helpers
    void GenerateVertexColour(std::shared_ptr<MeshData> MeshData);

    void TriangulateUsdGeometry(); 
    
private:
    const USDScene* Reader = nullptr;
    pxr::UsdPrim Mesh;
    std::shared_ptr<MeshData> SharedMeshData;
};






