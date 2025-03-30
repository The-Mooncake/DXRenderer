#pragma once

#include "USDReader.h"
#include "pch.h"

struct MeshData
{
    std::vector<uint32_t> Indices;
    std::vector<DirectX::XMFLOAT3> Positions;
    std::vector<DirectX::XMFLOAT3> Normals;
    std::vector<DirectX::XMFLOAT2> UVs;
};

class RenderMesh
{
public:
    RenderMesh(const USDReader* InReader);
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

private:
    const USDReader* Reader;
    std::shared_ptr<MeshData> SharedMeshData;
};


