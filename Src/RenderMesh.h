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
    
    void ProcessVertices(bool bIsYUp);
};

class RenderMesh
{
public:
    RenderMesh(const USDScene* InReader);
    void Load(class pxr::UsdPrim& Mesh);

    std::shared_ptr<MeshData> GetMeshData() { return SharedMeshData; }

private:
    bool ValidatePrim(pxr::UsdPrim& Mesh);
    
    template<typename SrcT, typename DestT>
    bool CopyData(pxr::UsdPrim& Mesh, const pxr::TfToken& AttrName, std::vector<DestT>& DestArray);

    // Dx data accessors are different to usd types, custom implementations for copying data required.
    template<typename SrcT>
    bool CopyDXFloat3Data(pxr::UsdPrim& Mesh, const pxr::TfToken& AttrName, std::vector<DirectX::XMFLOAT3>& DestArray);

    template<typename SrcT>
    bool CopyDXFloat2Data(pxr::UsdPrim& Mesh, const pxr::TfToken& AttrName, std::vector<DirectX::XMFLOAT2>& DestArray);

    void GenerateVertexColour(std::shared_ptr<MeshData> MeshData);

private:
    const USDScene* Reader;
    std::shared_ptr<MeshData> SharedMeshData;
};


