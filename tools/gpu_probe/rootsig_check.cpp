// rootsig_check — deserialize a D3D12 serialized root signature with the PC
// runtime and print its normalised structure. Independent reference for the
// console blob captured from XboxBvhBuilder.dll through xcl.

#include <windows.h>
#include <d3d12.h>

#include <cstdio>

static const char* param_type(D3D12_ROOT_PARAMETER_TYPE t) {
    switch (t) {
    case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE: return "TABLE";
    case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS: return "32BIT";
    case D3D12_ROOT_PARAMETER_TYPE_CBV: return "CBV";
    case D3D12_ROOT_PARAMETER_TYPE_SRV: return "SRV";
    case D3D12_ROOT_PARAMETER_TYPE_UAV: return "UAV";
    }
    return "?";
}

int main(int argc, char** argv) {
    if (argc < 2) { std::printf("usage: rootsig_check <blob.bin>\n"); return 2; }
    FILE* f = fopen(argv[1], "rb");
    if (!f) { std::printf("cannot open %s\n", argv[1]); return 2; }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    void* data = malloc(n);
    if (!data || fread(data, 1, n, f) != (size_t)n) { std::printf("read failed\n"); return 2; }
    fclose(f);

    std::printf("blob bytes=%ld\n", n);
    ID3D12RootSignatureDeserializer* d = nullptr;
    HRESULT hr = D3D12CreateRootSignatureDeserializer(data, (SIZE_T)n, IID_PPV_ARGS(&d));
    std::printf("D3D12CreateRootSignatureDeserializer hr=0x%08lx\n", (unsigned long)hr);
    if (FAILED(hr)) return 3;

    const D3D12_ROOT_SIGNATURE_DESC* desc = d->GetRootSignatureDesc();
    std::printf("version=%u flags=0x%x params=%u static_samplers=%u\n",
                0u, desc->Flags,
                desc->NumParameters, desc->NumStaticSamplers);

    for (UINT i = 0; i < desc->NumParameters; i++) {
        const D3D12_ROOT_PARAMETER& p = desc->pParameters[i];
        std::printf("  param[%u] type=%s vis=%u", i, param_type(p.ParameterType), p.ShaderVisibility);
        if (p.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
            std::printf(" ranges=%u", p.DescriptorTable.NumDescriptorRanges);
            for (UINT r = 0; r < p.DescriptorTable.NumDescriptorRanges; r++) {
                const auto& g = p.DescriptorTable.pDescriptorRanges[r];
                std::printf(" {range=%u base=%u space=%u count=%u off=%u}",
                            (unsigned)g.RangeType, g.BaseShaderRegister, g.RegisterSpace,
                            g.NumDescriptors, g.OffsetInDescriptorsFromTableStart);
            }
        } else if (p.ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS) {
            std::printf(" consts base=%u space=%u count=%u",
                        p.Constants.ShaderRegister, p.Constants.RegisterSpace, p.Constants.Num32BitValues);
        } else {
            std::printf(" reg base=%u space=%u", p.Descriptor.ShaderRegister, p.Descriptor.RegisterSpace);
        }
        std::printf("\n");
    }
    for (UINT i = 0; i < desc->NumStaticSamplers; i++) {
        const D3D12_STATIC_SAMPLER_DESC& s = desc->pStaticSamplers[i];
        std::printf("  sampler[%u] filter=%u addr=%u/%u/%u reg=%u space=%u vis=%u\n",
                    i, (unsigned)s.Filter, (unsigned)s.AddressU, (unsigned)s.AddressV,
                    (unsigned)s.AddressW, s.ShaderRegister, s.RegisterSpace, (unsigned)s.ShaderVisibility);
    }
    d->Release();
    return 0;
}
