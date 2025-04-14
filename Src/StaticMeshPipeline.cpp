#include "StaticMeshPipeline.h"

#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <d3d12.h>
#include <dxgi1_6.h>

#include "MainWindow.h"
#include "Renderer.h"
#include "pch.h"


StaticMeshPipeline::StaticMeshPipeline(Renderer* InRenderer)
{
    R = InRenderer;

    CompileShaders();
    CreatePSO();
    SetupConstantBuffer();
    SetupIndexBuffer();
    SetupVertexBuffer();

    HRESULT HR = R->Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, R->CmdAllocator.Get(), MeshPSO.Get(), IID_PPV_ARGS(&CmdList));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create 'StaticMeshPipeline' command list!", L"Error", MB_OK);
        PostQuitMessage(1);
    }
    CmdList->Close();
}

ComPtr<ID3D12GraphicsCommandList> StaticMeshPipeline::PopulateCmdList()
{
    HRESULT HR;

    HR = CmdList->Reset(R->CmdAllocator.Get(), MeshPSO.Get());
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to reset the StaticMeshPipeline command list!", L"Error", MB_OK);
        PostQuitMessage(1);
    }
    CmdList->BeginEvent(1, "SM-Pipeline", sizeof("SM-Pipeline"));
    
    CmdList->SetGraphicsRootSignature(R->RootSig.Get());
    CmdList->RSSetViewports(1, &R->Viewport);
    D3D12_RECT ScissorRect{};
    ScissorRect.right = R->Width;
    ScissorRect.bottom = R->Height;
    ScissorRect.left = 0;
    ScissorRect.top = 0;
    CmdList->RSSetScissorRects(1, &ScissorRect);
    CmdList->SetPipelineState(MeshPSO.Get());

    // Set the RT for the Output merger for this PSO.
    R->SetBackBufferOM(CmdList);
    
    // Upload const buffers
    ComPtr<ID3D12DescriptorHeap> DescriptorHeaps[] = {ConstantBufferHeap};
    CmdList->SetDescriptorHeaps(_countof(DescriptorHeaps), DescriptorHeaps->GetAddressOf());  
    D3D12_GPU_DESCRIPTOR_HANDLE CbHandle(ConstantBufferHeap->GetGPUDescriptorHandleForHeapStart());
    CmdList->SetGraphicsRootDescriptorTable(0, CbHandle);
    
    // Mesh rendering
    CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    CmdList->IASetVertexBuffers(0, 1, &VertexBufferView);
    CmdList->IASetIndexBuffer(&IndexBufferView);

    CmdList->DrawIndexedInstanced(3, 1, 0 ,0, 0);
    
    // Finalise command list and queues.
    CmdList->EndEvent();
    CmdList->Close();
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to close the StaticMeshPipeline command list!!", L"Error", MB_OK);
        PostQuitMessage(1);
    }

    return CmdList;
}

void StaticMeshPipeline::Update(const CB_WVP& WVP)
{
    D3D12_RANGE ReadRange;
    ReadRange.Begin = 0;
    ReadRange.End = 0;
    UINT8* MappedConstantBuffer;
    HRESULT HR = ConstantBuffer->Map(0, &ReadRange, reinterpret_cast<void**>(&MappedConstantBuffer));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to map constant buffer!", L"Error", MB_OK);
        PostQuitMessage(1);
        return;
    }
    memcpy(MappedConstantBuffer, &WVP, sizeof(WVP));
    ConstantBuffer->Unmap(0, &ReadRange);
}

bool StaticMeshPipeline::CompileShaders()
{
    HRESULT HR;
    bool bResult = false;

    // Load and Compile shaders...
#if defined(_DEBUG)
    UINT CompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT CompileFlags = 0;
#endif
    HR = D3DCompileFromFile(L"./Shaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", CompileFlags, 0, &VS, nullptr);
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to compile vertex shaders!", L"Error", MB_OK);
        PostQuitMessage(1);
        return bResult;
    }
    HR = D3DCompileFromFile(L"./Shaders.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", CompileFlags, 0, &PS, nullptr);
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to compile pixel shaders!", L"Error", MB_OK);
        PostQuitMessage(1);
        return bResult;
    }

    bResult = true;
    return bResult;
}

bool StaticMeshPipeline::CreatePSO()
{
    bool bResult = false;
    
    D3D12_INPUT_ELEMENT_DESC InElementDesc[] =  // Define the vertex input layout.
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
    
    D3D12_RASTERIZER_DESC Raster_Desc{};
    Raster_Desc.FillMode = D3D12_FILL_MODE_SOLID;
    Raster_Desc.CullMode = D3D12_CULL_MODE_NONE;
    Raster_Desc.FrontCounterClockwise = false;
    Raster_Desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    Raster_Desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    Raster_Desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    Raster_Desc.DepthClipEnable = false;
    Raster_Desc.MultisampleEnable = false;
    Raster_Desc.AntialiasedLineEnable = false;
    Raster_Desc.ForcedSampleCount = 0;
    Raster_Desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    
    D3D12_BLEND_DESC Blend_Desc{};
    Blend_Desc.AlphaToCoverageEnable = false;
    Blend_Desc.IndependentBlendEnable = false;

    const D3D12_RENDER_TARGET_BLEND_DESC DefaultRenderTargetBlendDesc =
    {
        false, false,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL,
    };
    Blend_Desc.RenderTarget[0] = DefaultRenderTargetBlendDesc;
    
    D3D12_GRAPHICS_PIPELINE_STATE_DESC PipeStateDesc = {};
    PipeStateDesc.InputLayout = { InElementDesc, _countof(InElementDesc) }; // Array of shader intrinsics
    PipeStateDesc.pRootSignature = R->RootSig.Get();
    PipeStateDesc.VS = {reinterpret_cast<UINT8*>(VS->GetBufferPointer()), VS->GetBufferSize()};
    PipeStateDesc.PS = {reinterpret_cast<UINT8*>(PS->GetBufferPointer()), PS->GetBufferSize()};
    PipeStateDesc.RasterizerState = Raster_Desc;
    PipeStateDesc.BlendState = Blend_Desc;
    PipeStateDesc.DepthStencilState.DepthEnable = FALSE;
    PipeStateDesc.DepthStencilState.StencilEnable = FALSE;
    PipeStateDesc.SampleMask = UINT_MAX;
    PipeStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    PipeStateDesc.NumRenderTargets = 1;
    PipeStateDesc.RTVFormats[0] = G_MainWindow->RendererDX->FrameBufferFormat;
    PipeStateDesc.SampleDesc.Count = 1;

    HRESULT HR = R->Device->CreateGraphicsPipelineState(&PipeStateDesc, IID_PPV_ARGS(&MeshPSO));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create graphics pipeline!", L"Error", MB_OK);
        PostQuitMessage(1);
        return bResult;
    }
    MeshPSO->SetName(L"Pipeline State (PSO) - Mesh");

    bResult = true;
    return bResult;
}

bool StaticMeshPipeline::SetupConstantBuffer()
{
    HRESULT HR;
    bool bResult = false;

    // Create the Constant Buffer
    D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
    HeapDesc.NodeMask = 0;
    HeapDesc.NumDescriptors = 1;
    HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    HR = R->Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&ConstantBufferHeap));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create constant buffer descriptor heap!", L"Error", MB_OK);
        PostQuitMessage(1);
        return bResult;
    }

    D3D12_HEAP_PROPERTIES HeapProps;
    HeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProps.CreationNodeMask = 0;
    HeapProps.VisibleNodeMask = 0;

    D3D12_RESOURCE_DESC CbResourceDesc;
    CbResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    CbResourceDesc.Alignment = 0;
    CbResourceDesc.Width = (sizeof(R->WVP) + 255) & ~255;
    CbResourceDesc.Height = 1;
    CbResourceDesc.DepthOrArraySize = 1;
    CbResourceDesc.MipLevels = 1;
    CbResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    CbResourceDesc.SampleDesc.Count = 1;
    CbResourceDesc.SampleDesc.Quality = 0;
    CbResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    CbResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    HR = R->Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &CbResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&ConstantBuffer));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create constant buffer!", L"Error", MB_OK);
        PostQuitMessage(1);
        return bResult;
    }

    // Create our Constant Buffer View
    D3D12_CONSTANT_BUFFER_VIEW_DESC CbvDesc = {};
    CbvDesc.BufferLocation = ConstantBuffer->GetGPUVirtualAddress();
    CbvDesc.SizeInBytes = (sizeof(R->WVP) + 255) & ~255; // CB size is required to be 256-byte aligned.

    D3D12_CPU_DESCRIPTOR_HANDLE CbvHandle(ConstantBufferHeap->GetCPUDescriptorHandleForHeapStart());
    CbvHandle.ptr = CbvHandle.ptr + R->Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 0;
    R->Device->CreateConstantBufferView(&CbvDesc, CbvHandle);

    // We do not intend to read from this resource on the CPU.
    D3D12_RANGE ReadRange;
    ReadRange.Begin = 0;
    ReadRange.End = 0;

    UINT8* MappedConstantBuffer;

    HR = ConstantBuffer->Map(0, &ReadRange, reinterpret_cast<void**>(&MappedConstantBuffer));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to map constant buffer!", L"Error", MB_OK);
        PostQuitMessage(1);
        return bResult;
    }
    memcpy(MappedConstantBuffer, &R->WVP, sizeof(R->WVP));
    ConstantBuffer->Unmap(0, &ReadRange);

    ConstantBufferHeap->SetName(L"Constant Buffer Upload Resource Heap");
    ConstantBuffer->SetName(L"Constant Buffer");

    bResult = true;
    return bResult;
}

bool StaticMeshPipeline::SetupVertexBuffer()
{
    HRESULT HR;
    bool bResult = false;

    // Load the meshes!
    const UINT VertexBufferSize = sizeof(R->VertexBufferData);

    D3D12_HEAP_PROPERTIES HeapProps;
    HeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProps.CreationNodeMask = 1;
    HeapProps.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC VertexBufferResourceDesc;
    VertexBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    VertexBufferResourceDesc.Alignment = 0;
    VertexBufferResourceDesc.Width = VertexBufferSize;
    VertexBufferResourceDesc.Height = 1;
    VertexBufferResourceDesc.DepthOrArraySize = 1;
    VertexBufferResourceDesc.MipLevels = 1;
    VertexBufferResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    VertexBufferResourceDesc.SampleDesc.Count = 1;
    VertexBufferResourceDesc.SampleDesc.Quality = 0;
    VertexBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    VertexBufferResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
     
    HR = R->Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &VertexBufferResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&VertexBuffer));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create vertex buffer!", L"Error", MB_OK);
        PostQuitMessage(1);
        return bResult;
    }

    // Copy the triangle data to the vertex buffer.
    UINT8* VertexDataBegin;

    // We do not intend to read from this resource on the CPU.
    D3D12_RANGE ReadRange;
    ReadRange.Begin = 0;
    ReadRange.End = 0;

    HR = VertexBuffer->Map(0, &ReadRange, reinterpret_cast<void**>(&VertexDataBegin));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to map vertex buffer!", L"Error", MB_OK);
        PostQuitMessage(1);
        return bResult;
    }
    memcpy(VertexDataBegin, R->VertexBufferData, sizeof(R->VertexBufferData));
    VertexBuffer->Unmap(0, nullptr);

    // Initialize the vertex buffer view.
    VertexBufferView.BufferLocation = VertexBuffer->GetGPUVirtualAddress();
    VertexBufferView.StrideInBytes = sizeof(Vertex);
    VertexBufferView.SizeInBytes = VertexBufferSize;

    VertexBuffer->SetName(L"Vertex Buffer");

    R->Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&R->Fence));
    R->FenceValue = 1;
    R->FenceEvent = CreateEvent(nullptr, false, false, nullptr);
    R->WaitForPreviousFrame();

    bResult = true;
    return bResult;
}

bool StaticMeshPipeline::SetupIndexBuffer()
{
    HRESULT HR;
    bool bResult = false;
    
    // Declare Handles
    const UINT IndexBufferSize = sizeof(R->TriIndexBufferData);

    D3D12_HEAP_PROPERTIES HeapIndexProps;
    HeapIndexProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    HeapIndexProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapIndexProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapIndexProps.CreationNodeMask = 1;
    HeapIndexProps.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC IndexBufferResourceDesc;
    IndexBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    IndexBufferResourceDesc.Alignment = 0;
    IndexBufferResourceDesc.Width = IndexBufferSize;
    IndexBufferResourceDesc.Height = 1;
    IndexBufferResourceDesc.DepthOrArraySize = 1;
    IndexBufferResourceDesc.MipLevels = 1;
    IndexBufferResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    IndexBufferResourceDesc.SampleDesc.Count = 1;
    IndexBufferResourceDesc.SampleDesc.Quality = 0;
    IndexBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    IndexBufferResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    HR = R->Device->CreateCommittedResource(
        &HeapIndexProps, D3D12_HEAP_FLAG_NONE, &IndexBufferResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&IndexBuffer));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create index buffer!", L"Error", MB_OK);
        PostQuitMessage(1);
        return bResult;
    }

    // Copy data to DirectX 12 driver memory:
    UINT8* pVertexDataBegin;

    // We do not intend to read from this resource on the CPU.
    D3D12_RANGE IdxReadRange;
    IdxReadRange.Begin = 0;
    IdxReadRange.End = 0;
    
    HR = IndexBuffer->Map(0, &IdxReadRange, reinterpret_cast<void**>(&pVertexDataBegin));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to Map index buffer!", L"Error", MB_OK);
        PostQuitMessage(1);
        return bResult;
    }
    memcpy(pVertexDataBegin, R->TriIndexBufferData, sizeof(R->TriIndexBufferData));
    IndexBuffer->Unmap(0, nullptr);

    IndexBuffer->SetName(L"Mesh Index Buffer");

    // Initialize the index buffer view.
    IndexBufferView.BufferLocation = IndexBuffer->GetGPUVirtualAddress();
    IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
    IndexBufferView.SizeInBytes = IndexBufferSize;

    bResult = true;
    return bResult;
}
