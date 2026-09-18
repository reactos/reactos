// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#include "dxlcommon.hpp"
#include "shader_compiler_t.hpp"

#include <d3d9.h>
#include <D3Dcompiler.h>
#include <memory>

#include <wrl/client.h>

#include "wgx_error.h"

namespace dxlayer
{
    // An implementation of ID3DBlob 
    // for use in shader compilation operations
    class buffer_xm : public buffer, public ID3DBlob
    {
    private:
        Microsoft::WRL::ComPtr<ID3DBlob> m_blob;

    public:
        // constructor
        inline buffer_xm(ID3DBlob* blob) : m_blob(blob) {}

        // Inherited via buffer
        virtual data get_buffer_data() override
        {
            return{ GetBufferPointer(), static_cast<int64_t>(GetBufferSize()) };
        }

#pragma region Inherited via ID3DBlob

        virtual LPVOID __stdcall GetBufferPointer() override
        {
            return m_blob->GetBufferPointer();
        }

        virtual SIZE_T __stdcall GetBufferSize() override
        {
            return m_blob->GetBufferSize();
        }

        virtual HRESULT __stdcall QueryInterface(REFIID riid, void ** ppvObject) override
        {
            return m_blob.Get()->QueryInterface(riid, ppvObject);
        }

        virtual ULONG __stdcall AddRef(void) override
        {
            return m_blob.Get()->AddRef();
        }

        virtual ULONG __stdcall Release(void) override
        {
            return m_blob.Get()->Release();
        }

#pragma endregion // Inherited via ID3DBlob
    };

    // Implements shader_t in terms of shader API's exposed 
    // by D3DCompiler_*.dll
    template<>
    class shader_t<dxapi::xmath>
    {
    public:
        // Compile a shader file using D3DCompile
        static inline HRESULT compile(
            std::string src_data,
            std::string entry_point_name,
            std::string shader_profile_target,
            unsigned long flags1, unsigned long flags2,
            std::shared_ptr<buffer>& shader,
            std::shared_ptr<buffer>& err_msgs)
        {
            ID3DBlob* pShader = nullptr;
            ID3DBlob* pErrorMsgs = nullptr;

            auto hResult =
                D3DCompile(
                    src_data.c_str(), src_data.size() * sizeof(std::string::value_type),
                    nullptr,
                    nullptr,
                    nullptr,
                    entry_point_name.c_str(),
                    shader_profile_target.c_str(),
                    flags1, flags2,
                    &pShader, &pErrorMsgs);

            if (SUCCEEDED(hResult))
            {
                shader.reset(new buffer_xm(pShader));

                err_msgs.reset();
                err_msgs = nullptr;
            }
            else
            {
                shader.reset();
                shader = nullptr;

                err_msgs.reset(new buffer_xm(pErrorMsgs));
            }

            return hResult;
        }

#pragma region get_pixel_shader_profile_name

        // Returns the name of the highest high-level shader language(HLSL) pixel-shader 
        // profile supported by a given device.
        template<typename ID3DDevice>
        static std::string get_pixel_shader_profile_name(ID3DDevice* pD3DDevice);

        // Specialization of get_pixel_shader_profile_name with 
        // ID3DDevice = IDirect3DDevice9
        template<>
        inline static std::string get_pixel_shader_profile_name<IDirect3DDevice9>(
            IDirect3DDevice9* pD3DDevice)
        {
            // We will query D3DCAPS9 to identify the best supported
            // profile name. If that query fails, then the following 
            // default name will be used as a fallback. 
            const std::string default_profile_name = "ps_3_0";

            std::string pixel_shader_profile = default_profile_name;

            if (pD3DDevice != nullptr)
            {
                D3DCAPS9 d3dCaps;
                if (SUCCEEDED(pD3DDevice->GetDeviceCaps(&d3dCaps)))
                {
                    switch (d3dCaps.PixelShaderVersion)
                    {
                    case D3DPS_VERSION(2, 0):
                        pixel_shader_profile = "ps_2_0";
                        break;
                    case D3DPS_VERSION(2, 2):
                        pixel_shader_profile = "ps_2_a";
                        break;
                    case D3DPS_VERSION(3, 0):
                        pixel_shader_profile = "ps_3_0";
                        break;
                    case D3DPS_VERSION(4, 0):
                        if ((d3dCaps.PS20Caps.NumTemps >= 32) &&
                            (d3dCaps.PS20Caps.Caps & D3DPS20CAPS_ARBITRARYSWIZZLE) &&
                            (d3dCaps.PS20Caps.Caps & D3DPS20CAPS_GRADIENTINSTRUCTIONS) &&
                            (d3dCaps.PS20Caps.Caps & D3DPS20CAPS_PREDICATION) &&
                            (d3dCaps.PS20Caps.Caps & D3DPS20CAPS_NODEPENDENTREADLIMIT) &&
                            (d3dCaps.PS20Caps.Caps & D3DPS20CAPS_NOTEXINSTRUCTIONLIMIT))
                        {
                            pixel_shader_profile = "ps_4_0_level_9_3";
                        }
                        else if (d3dCaps.PS20Caps.NumTemps >= 12)
                        {
                            pixel_shader_profile = "ps_4_0_level_9_1";
                        }
                        break;
                    case D3DPS_VERSION(1, 1):
                    case D3DPS_VERSION(1, 2):
                    case D3DPS_VERSION(1, 3):
                    case D3DPS_VERSION(1, 4):
                    default:
                        // D3DCompile* API's do not support 1.x pixel shaders. The last version 
                        // of HLSL to support these targets was D3DX9 in the Oct 2006 release of 
                        // the DirectX SDK. All versions of D3DX and the DirectX SDK (i.e., separate
                        // SDK's that shipped outside of the Windows SDK) are deprecated.
                        // 
                        // Feature levels > 4.0 correspond to DirectX 10 or above. 
                        // Those are not (yet) supported in wpfgfx.
                        break;
                    }
                }
            }

            return pixel_shader_profile;
        }

#pragma endregion

#pragma region get_vertex_shader_profile_name

        // Returns the name of the highest high-level shader language(HLSL) vertex-shader 
        // profile supported by a given device.
        template<typename ID3DDevice>
        static std::string get_vertex_shader_profile_name(ID3DDevice* pD3DDevice);

        // Specialization of get_vertex_shader_profile_name with 
        // ID3DDevice = IDirect3DDevice9
        template<>
        static std::string get_vertex_shader_profile_name<IDirect3DDevice9>(
            IDirect3DDevice9* pD3DDevice)
        {
            // We will query D3DCAPS9 to identify the best supported
            // profile name. If that query fails, then the following 
            // default name will be used as a fallback. 
            const std::string default_profile_name = "vs_3_0";

            std::string vertex_shader_profile = default_profile_name;

            if (pD3DDevice != nullptr)
            {
                D3DCAPS9 d3dCaps;
                if (SUCCEEDED(pD3DDevice->GetDeviceCaps(&d3dCaps)))
                {
                    switch (d3dCaps.VertexShaderVersion)
                    {
                    case D3DVS_VERSION(2, 0):
                        vertex_shader_profile = "vs_2_0";
                        break;
                    case D3DVS_VERSION(2, 2):
                        vertex_shader_profile = "vs_2_a";
                        break;
                    case D3DVS_VERSION(3, 0):
                        vertex_shader_profile = "vs_3_0";
                        break;
                    case D3DVS_VERSION(4, 0):
                        if ((d3dCaps.VS20Caps.NumTemps >= 32) &&
                            (d3dCaps.VS20Caps.Caps & D3DVS20CAPS_PREDICATION))
                        {
                            vertex_shader_profile = "vs_4_0_level_9_3";
                        }
                        else if (d3dCaps.VS20Caps.NumTemps >= 12)
                        {
                            vertex_shader_profile = "vs_4_0_level_9_1";
                        }
                        break;
                    case D3DVS_VERSION(1, 1):
                    case D3DVS_VERSION(1, 2):
                    case D3DVS_VERSION(1, 3):
                    case D3DVS_VERSION(1, 4):
                    default:
                        // D3DCompile* API's do not support 1.x vertex shaders. The last version 
                        // of HLSL to support these targets was D3DX9 in the Oct 2006 release of 
                        // the DirectX SDK. All versions of D3DX and the DirectX SDK (i.e., separate
                        // SDK's that shipped outside of the Windows SDK) are deprecated.
                        // 
                        // Feature levels > 4.0 correspond to DirectX 10 or above. 
                        // Those are not (yet) supported in wpfgfx.
                        break;
                    }
                }
            }

            return vertex_shader_profile;
        }

#pragma endregion

        // Changes an error HRESULT to the more descriptive 
        // WGXERR_SHADER_COMPILE_FAILED if appropriate, and 
        // outputs the compiler errors 
#pragma warning(push)
#pragma warning(disable: 4100) // err_msgs is unreferenced in retail/fre builds.
        static HRESULT handle_errors_and_transform_hresult(
            HRESULT hResult, 
            const std::shared_ptr<buffer>& err_msgs)
#pragma warning(pop)
        {
            HRESULT retval = hResult;

            if ((hResult == D3DERR_INVALIDCALL) ||
                (hResult == DXGI_ERROR_INVALID_CALL) ||
                (hResult == E_FAIL))
            {
                retval = WGXERR_SHADER_COMPILE_FAILED;
            }

#if (defined(DBG) || defined(DEBUG) || defined(_DEBUG)) && !defined(TESTUSE_NOTRACETAG)

            //
            // Output compiler errors
            //

            auto szErrors =
                static_cast<char const*>(err_msgs->get_buffer_data().buffer);
            TraceTag((tagError, "MIL-HW: Vertex Shader compiler errors:\n%s", szErrors));

#endif // (defined(DBG) || defined(DEBUG) || defined(_DEBUG)) && !defined(TESTUSE_NOTRACETAG)
            return hResult;
            }

        };
    }

