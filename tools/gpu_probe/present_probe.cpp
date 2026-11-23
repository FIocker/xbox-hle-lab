// present_probe — minimal desktop D3D12 swapchain: window, FLIP_DISCARD swap
// chain, clear-and-present loop for a fixed time. Prints every HRESULT and the
// present count, so a failure can be attributed to the session or the adapter.

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>

#include <cstdio>

static LRESULT CALLBACK wnd_proc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(h, m, w, l);
}

int main(int argc, char** argv) {
    setvbuf(stdout, nullptr, _IONBF, 0);
    const int seconds = argc > 1 ? atoi(argv[1]) : 12;
    const UINT sync = argc > 2 ? (UINT)atoi(argv[2]) : 0;
    const UINT W = 800, H = 600;

    WNDCLASSW wc{};
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"present_probe";
    wc.hCursor = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"present_probe - D3D12 desktop",
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE, 40, 40, W + 16, H + 39,
                                nullptr, nullptr, wc.hInstance, nullptr);
    std::printf("hwnd=%p err=%lu\n", (void*)hwnd, GetLastError());
    if (!hwnd) return 1;
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    IDXGIFactory4* factory = nullptr;
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
    std::printf("CreateDXGIFactory1 hr=0x%08lx\n", (unsigned long)hr);
    if (FAILED(hr)) return 2;

    IDXGIAdapter1* adapter = nullptr;
    hr = factory->EnumAdapters1(0, &adapter);
    DXGI_ADAPTER_DESC1 ad{};
    if (SUCCEEDED(hr)) adapter->GetDesc1(&ad);
    wprintf(L"adapter: %ls\n", ad.Description);
    std::printf("adapter hr=0x%08lx vendor=0x%04x device=0x%04x vram=%llu MB\n",
                (unsigned long)hr, ad.VendorId, ad.DeviceId,
                (unsigned long long)(ad.DedicatedVideoMemory >> 20));

    ID3D12Device* dev = nullptr;
    hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&dev));
    std::printf("D3D12CreateDevice hr=0x%08lx\n", (unsigned long)hr);
    if (FAILED(hr)) return 3;

    ID3D12CommandQueue* queue = nullptr;
    D3D12_COMMAND_QUEUE_DESC qd{};
    qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    hr = dev->CreateCommandQueue(&qd, IID_PPV_ARGS(&queue));
    std::printf("CreateCommandQueue hr=0x%08lx\n", (unsigned long)hr);
    if (FAILED(hr)) return 4;

    DXGI_SWAP_CHAIN_DESC1 sd{};
    sd.Width = W;
    sd.Height = H;
    sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.SampleDesc.Count = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    IDXGISwapChain1* swap = nullptr;
    hr = factory->CreateSwapChainForHwnd(queue, hwnd, &sd, nullptr, nullptr, &swap);
    std::printf("CreateSwapChainForHwnd hr=0x%08lx\n", (unsigned long)hr);
    if (FAILED(hr)) {
        std::printf("present_probe: swapchain failed\n");
        return 5;
    }

    IDXGISwapChain3* swap3 = nullptr;
    swap->QueryInterface(IID_PPV_ARGS(&swap3));

    ID3D12CommandAllocator* alloc = nullptr;
    hr = dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&alloc));
    std::printf("CreateCommandAllocator hr=0x%08lx\n", (unsigned long)hr);
    ID3D12GraphicsCommandList* list = nullptr;
    hr = dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, alloc, nullptr, IID_PPV_ARGS(&list));
    std::printf("CreateCommandList hr=0x%08lx\n", (unsigned long)hr);

    ID3D12DescriptorHeap* heap = nullptr;
    D3D12_DESCRIPTOR_HEAP_DESC hd{};
    hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    hd.NumDescriptors = 2;
    hr = dev->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&heap));
    const UINT rtv_size = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    LARGE_INTEGER freq, start;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
    int frames = 0;
    HRESULT last_present = S_OK;

    for (;;) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) goto done;
        }

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        if ((now.QuadPart - start.QuadPart) / (double)freq.QuadPart > seconds) break;

        UINT index = swap3 ? swap3->GetCurrentBackBufferIndex() : 0;
        ID3D12Resource* back = nullptr;
        if (FAILED(swap->GetBuffer(index, IID_PPV_ARGS(&back)))) break;

        D3D12_CPU_DESCRIPTOR_HANDLE rtv = heap->GetCPUDescriptorHandleForHeapStart();
        rtv.ptr += (SIZE_T)index * rtv_size;
        dev->CreateRenderTargetView(back, nullptr, rtv);

        alloc->Reset();
        list->Reset(alloc, nullptr);
        D3D12_RESOURCE_BARRIER b{};
        b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition.pResource = back;
        b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        b.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        b.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        list->ResourceBarrier(1, &b);
        const float colour[4] = {0.15f, 0.55f, 0.95f, 1.0f};
        list->ClearRenderTargetView(rtv, colour, 0, nullptr);
        b.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        b.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        list->ResourceBarrier(1, &b);
        list->Close();
        ID3D12CommandList* lists[] = {list};
        queue->ExecuteCommandLists(1, lists);
        last_present = swap->Present(sync, 0);
        if ((frames % 60) == 0) std::printf("frame %d present=0x%08lx\n", frames, (unsigned long)last_present);
        back->Release();
        if (FAILED(last_present)) {
            std::printf("Present failed after %d frames hr=0x%08lx\n", frames,
                        (unsigned long)last_present);
            break;
        }
        frames++;
    }

done:
    std::printf("frames presented=%d last_present=0x%08lx\n", frames, (unsigned long)last_present);
    std::printf("present_probe: %s\n", frames > 0 ? "ok" : "no frames");
    return frames > 0 ? 0 : 6;
}
