// gpu_probe — D3D12/DXR capability dump.
//
// Run on the host and inside the VM to separate "the GPU works" from "the VM
// can reach the GPU". `--serial COM1` also writes the report to a serial port,
// which is how results leave the isolated VM.

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>

#include <cstdarg>
#include <cstdio>
#include <string>

namespace {

std::string g_report;

void Out(const char* fmt, ...) {
    char line[1024];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);
    std::fputs(line, stdout);
    g_report += line;
}

const char* TierName(D3D12_RAYTRACING_TIER t) {
    switch (t) {
        case D3D12_RAYTRACING_TIER_NOT_SUPPORTED: return "none";
        case D3D12_RAYTRACING_TIER_1_0: return "1.0";
        case D3D12_RAYTRACING_TIER_1_1: return "1.1";
        case static_cast<D3D12_RAYTRACING_TIER>(12): return "1.2";
        default: return "?";
    }
}

const char* MeshName(UINT t) {
    if (t == 0) return "none";
    if (t == 10) return "1.0";
    return "?";
}

void ProbeAdapter(IDXGIAdapter1* adapter, unsigned index) {
    DXGI_ADAPTER_DESC1 desc{};
    adapter->GetDesc1(&desc);

    char narrow[512] = {};
    WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, narrow, sizeof(narrow) - 1, nullptr,
                        nullptr);
    Out("\n[adapter %u] %s\n", index, narrow);
    Out("  vendor=0x%04x device=0x%04x flags=0x%x vram=%llu MB\n", desc.VendorId, desc.DeviceId,
        desc.Flags,
        static_cast<unsigned long long>(desc.DedicatedVideoMemory / (1024 * 1024)));
    Out("  software=%s\n", (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) ? "yes" : "no");

    ID3D12Device* device = nullptr;
    HRESULT hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
    if (FAILED(hr)) {
        Out("  D3D12CreateDevice: FAILED 0x%08lx\n", static_cast<unsigned long>(hr));
        return;
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS1 o1{};
    D3D12_FEATURE_DATA_D3D12_OPTIONS o{};
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 o5{};
    D3D12_FEATURE_DATA_D3D12_OPTIONS7 o7{};
    device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &o1, sizeof(o1));
    device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &o, sizeof(o));
    device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &o5, sizeof(o5));
    device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &o7, sizeof(o7));

    const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1,
                                        D3D_FEATURE_LEVEL_12_0};
    const char* best = "11_0";
    for (D3D_FEATURE_LEVEL level : levels) {
        D3D12_FEATURE_DATA_FEATURE_LEVELS q{};
        q.NumFeatureLevels = 1;
        q.pFeatureLevelsRequested = &level;
        if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &q, sizeof(q))) &&
            q.MaxSupportedFeatureLevel == level) {
            best = level == D3D_FEATURE_LEVEL_12_2   ? "12_2"
                   : level == D3D_FEATURE_LEVEL_12_1 ? "12_1"
                                                     : "12_0";
            break;
        }
    }

    Out("  feature_level=%s wave_lanes=%u..%u\n", best, o1.WaveLaneCountMin,
        o1.WaveLaneCountMax);
    Out("  raytracing=%s(%d) mesh=%s(%d) binding_tier=%d\n", TierName(o5.RaytracingTier),
        static_cast<int>(o5.RaytracingTier), MeshName(o7.MeshShaderTier),
        static_cast<int>(o7.MeshShaderTier), static_cast<int>(o.ResourceBindingTier));
    device->Release();
}

void WriteSerial(const char* port_name) {
    const std::string device = std::string("\\\\.\\") + port_name;
    HANDLE port = CreateFileA(device.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (port == INVALID_HANDLE_VALUE) {
        std::fprintf(stderr, "serial open failed (error %lu)\n", GetLastError());
        return;
    }
    DCB dcb{};
    dcb.DCBlength = sizeof(dcb);
    if (GetCommState(port, &dcb)) {
        dcb.BaudRate = CBR_115200;
        dcb.ByteSize = 8;
        dcb.Parity = NOPARITY;
        dcb.StopBits = ONESTOPBIT;
        SetCommState(port, &dcb);
    }
    DWORD written = 0;
    WriteFile(port, g_report.data(), static_cast<DWORD>(g_report.size()), &written, nullptr);
    FlushFileBuffers(port);
    CloseHandle(port);
}

}  // namespace

int main(int argc, char** argv) {
    IDXGIFactory1* factory = nullptr;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
        Out("CreateDXGIFactory1 failed\n");
        return 1;
    }

    unsigned index = 0;
    IDXGIAdapter1* adapter = nullptr;
    while (factory->EnumAdapters1(index, &adapter) != DXGI_ERROR_NOT_FOUND) {
        ProbeAdapter(adapter, index);
        adapter->Release();
        ++index;
    }
    factory->Release();
    Out("\nadapters=%u\n", index);

    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--serial" && i + 1 < argc) {
            WriteSerial(argv[i + 1]);
        }
    }
    return 0;
}
