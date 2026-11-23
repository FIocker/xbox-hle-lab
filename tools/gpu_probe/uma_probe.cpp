// uma_probe — the exact heap path xcl uses for console unified memory.
//
// Allocates a 4 GiB-aligned, fully committed 2 GB region with VirtualAlloc2
// (MEM_ADDRESS_REQUIREMENTS) and opens it as a D3D12 heap.

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <memoryapi.h>

#include <cstdio>

int main() {
    IDXGIFactory1* factory = nullptr;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) return 1;
    IDXGIAdapter1* adapter = nullptr;
    if (FAILED(factory->EnumAdapters1(0, &adapter))) return 1;

    ID3D12Device* dev = nullptr;
    if (FAILED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&dev)))) return 1;

    D3D12_FEATURE_DATA_D3D12_OPTIONS1 o1{};
    dev->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &o1, sizeof(o1));
    std::printf("adapter ok, wave %u..%u\n", o1.WaveLaneCountMin, o1.WaveLaneCountMax);

    ID3D12Device3* dev3 = nullptr;
    HRESULT hr = dev->QueryInterface(IID_PPV_ARGS(&dev3));
    std::printf("ID3D12Device3: 0x%08lx\n", static_cast<unsigned long>(hr));
    if (FAILED(hr)) return 2;

    const SIZE_T size = 2ull * 1024 * 1024 * 1024;
    MEM_ADDRESS_REQUIREMENTS req{};
    req.Alignment = 1ull << 32;
    MEM_EXTENDED_PARAMETER param{};
    param.Type = MemExtendedParameterAddressRequirements;
    param.Pointer = &req;

    using VirtualAlloc2Fn = void*(WINAPI*)(HANDLE, void*, SIZE_T, DWORD, DWORD,
                                           MEM_EXTENDED_PARAMETER*, ULONG);
    auto virtual_alloc2 = reinterpret_cast<VirtualAlloc2Fn>(
        reinterpret_cast<void*>(GetProcAddress(GetModuleHandleW(L"kernelbase.dll"), "VirtualAlloc2")));
    void* base = virtual_alloc2
        ? virtual_alloc2(GetCurrentProcess(), nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE, &param, 1)
        : nullptr;
    std::printf("VirtualAlloc2(4GiB aligned): %p err=%lu\n", base, GetLastError());
    if (!base) {
        // Fall back to the overallocate-and-align approach, committing the full extent.
        const SIZE_T big = size + (1ull << 32);
        void* raw = VirtualAlloc(nullptr, big, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        std::printf("fallback VirtualAlloc(%llu): %p\n",
                    static_cast<unsigned long long>(big), raw);
        if (!raw) return 3;
        uintptr_t addr = reinterpret_cast<uintptr_t>(raw);
        base = reinterpret_cast<void*>((addr + 0xFFFFFFFFull) & ~0xFFFFFFFFull);
        std::printf("aligned inside 6GiB block: %p\n", base);
    }

    ID3D12Heap* heap = nullptr;
    hr = dev3->OpenExistingHeapFromAddress(base, IID_PPV_ARGS(&heap));
    std::printf("OpenExistingHeapFromAddress: 0x%08lx heap=%p\n",
                static_cast<unsigned long>(hr), static_cast<void*>(heap));
    if (SUCCEEDED(hr)) {
        D3D12_HEAP_DESC d = heap->GetDesc();
        std::printf("heap size=%llu alignment=%llu flags=0x%x\n",
                    static_cast<unsigned long long>(d.SizeInBytes),
                    static_cast<unsigned long long>(d.Alignment), d.Flags);
        heap->Release();
    }
    return SUCCEEDED(hr) ? 0 : 4;
}
