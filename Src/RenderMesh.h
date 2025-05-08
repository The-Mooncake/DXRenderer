#pragma once

#include "USDScene.h"
#include "pch.h"
#include "Renderer.h"

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

    void Triangulate(std::shared_ptr<MeshData> MeshData);
    
private:
    const USDScene* Reader = nullptr;
    pxr::UsdPrim* Mesh = nullptr;
    std::shared_ptr<MeshData> SharedMeshData;
};






