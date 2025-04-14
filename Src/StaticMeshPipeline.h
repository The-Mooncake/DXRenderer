#pragma once

#include <wrl/client.h>

#include "pch.h"

#include <d3dcommon.h>
#include <d3d12.h>


using Microsoft::WRL::ComPtr; // Import only the ComPtr

class StaticMeshPipeline
{
public:
    StaticMeshPipeline(class Renderer* InRenderer);
    
    ComPtr<ID3D12GraphicsCommandList> PopulateCmdList();

    void Update(const CB_WVP& WVP);

private:
    bool CompileShaders();
    bool CreatePSO();
    bool SetupConstantBuffer();
    bool SetupVertexBuffer();
    bool SetupIndexBuffer();

public:
    // PSO
    ComPtr<ID3D12PipelineState> MeshPSO;

    // CmdList
    ComPtr<ID3D12GraphicsCommandList> CmdList;
    
    // Shaders and object resources.
    ComPtr<ID3DBlob> VS;
    ComPtr<ID3DBlob> PS;

    // Mesh Buffers
    D3D12_INDEX_BUFFER_VIEW IndexBufferView;
    D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
    ComPtr<ID3D12Resource> VertexBuffer;
    ComPtr<ID3D12Resource> IndexBuffer;    
    ComPtr<ID3D12Resource> ConstantBuffer;
    ComPtr<ID3D12DescriptorHeap> ConstantBufferHeap;

private:
    class Renderer* R; 
    
};
