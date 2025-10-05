
#include "ImGuiDescHeap.h"
#include <d3d12.h>
#include "imgui.h"

void ImguiDescHeapAllocator::Create(ID3D12Device* InDevice, ID3D12DescriptorHeap* InHeap)
{
    IM_ASSERT(Heap == nullptr && FreeIndices.empty());
    Heap = InHeap;
    D3D12_DESCRIPTOR_HEAP_DESC Desc = InHeap->GetDesc();
    HeapType = Desc.Type;
    HeapStartCpu = Heap->GetCPUDescriptorHandleForHeapStart();
    HeapStartGpu = Heap->GetGPUDescriptorHandleForHeapStart();
    HeapHandleIncrement = InDevice->GetDescriptorHandleIncrementSize(HeapType);
    FreeIndices.reserve(static_cast<int>(Desc.NumDescriptors));
    for (int N = static_cast<int>(Desc.NumDescriptors); N > 0; N--)
        FreeIndices.push_back(N - 1);
}

void ImguiDescHeapAllocator::Destroy()
{
    Heap = nullptr;
    FreeIndices.clear();
}

void ImguiDescHeapAllocator::Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
{
    IM_ASSERT(FreeIndices.Size > 0);
    const size_t Idx = FreeIndices.back();
    FreeIndices.pop_back();
    out_cpu_desc_handle->ptr = HeapStartCpu.ptr + (Idx * HeapHandleIncrement);
    out_gpu_desc_handle->ptr = HeapStartGpu.ptr + (Idx * HeapHandleIncrement);
}

void ImguiDescHeapAllocator::Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle)
{
    size_t cpu_idx = (out_cpu_desc_handle.ptr - HeapStartCpu.ptr) / HeapHandleIncrement;
    size_t gpu_idx = (out_gpu_desc_handle.ptr - HeapStartGpu.ptr) / HeapHandleIncrement;
    IM_ASSERT(cpu_idx == gpu_idx);
    FreeIndices.push_back(static_cast<int>(cpu_idx));
}
