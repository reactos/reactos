// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#include "dxlcommon.hpp"
#include "shader_compiler_t.hpp"

#include <d3dx9shader.h>
#include <memory>
#include <string>
#include <wrl/client.h>

#include "wgx_error.h"


namespace dxlayer
{
    // An implementation of ID3DXBuffer (defined in <d3dx9mesh.h>)
    // for use in shader compilation operations
    class buffer_d3dx : public buffer, public ID3DXBuffer
    {
    private:
        Microsoft::WRL::ComPtr<ID3DXBuffer> m_buf;

    public:

        // constructor
        inline buffer_d3dx(ID3DXBuffer* buf) : m_buf(buf) 
        {
        }

#pragma region Inherited via buffer
        
        inline virtual data get_buffer_data() override
        {
            return{ GetBufferPointer(), GetBufferSize() };
        }

#pragma endregion 

#pragma region Inherited via ID3DXBuffer

        inline virtual LPVOID __stdcall GetBufferPointer(void) override
        {
            return m_buf->GetBufferPointer();
        }

        inline virtual DWORD __stdcall GetBufferSize(void) override
        {
            return m_buf->GetBufferSize();
        }

        inline virtual HRESULT __stdcall QueryInterface(REFIID iid, LPVOID * ppv) override
        {
            return m_buf.Get()->QueryInterface(iid, ppv);
        }

        inline virtual ULONG __stdcall AddRef(void) override
        {
            return m_buf.Get()->AddRef();
        }

        inline virtual ULONG __stdcall Release(void) override
        {
            return m_buf.Get()->Release();
        }

#pragma endregion
    };

    // Implements shader_t in terms of shader API's exposed
    // by D3DX9.lib
    template<>
    class shader_t<dxapi::d3dx9>
    {
    public:
        // Compile a shader file using D3DXCompileShader
        static inline HRESULT compile(
            std::string src_data,
            std::string entry_point_name,
            std::string shader_profile_target,
            unsigned long flags1, unsigned long /*flags2*/,
            std::shared_ptr<buffer>& shader,
            std::shared_ptr<buffer>& err_msgs)
        {

            LPD3DXBUFFER pShader = nullptr;
            LPD3DXBUFFER pErrorMessages = nullptr;

            HRESULT hResult = 
                D3DXCompileShader(
                    src_data.c_str(), static_cast<UINT>(src_data.length()),
                    nullptr,
                    nullptr,
                    entry_point_name.c_str(),
                    shader_profile_target.c_str(),
                    flags1,
                    &pShader,
                    &pErrorMessages,
                    nullptr
                );

            if (SUCCEEDED(hResult))
            {
                shader.reset(new buffer_d3dx(pShader));

                err_msgs.reset();
                err_msgs = nullptr;
            }
            else
            {
                shader.reset();
                shader = nullptr;

                err_msgs.reset(new buffer_d3dx(pErrorMessages));
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
            return D3DXGetPixelShaderProfile(pD3DDevice);
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
            return D3DXGetVertexShaderProfile(pD3DDevice);
        }

#pragma endregion

        // Changes an error HRESULT to the more descriptive 
        // WGXERR_SHADER_COMPILE_FAILED if appropriate, and 
        // outputs the compiler errors 
        // 
        // Suppress warning C4100: 'err_msgs' : unreferenced formal parameter
        // In a release/ret build, this parameter is not used. 
#pragma warning(suppress: 4100)
        static HRESULT handle_errors_and_transform_hresult(
            HRESULT hResult, 
            const std::shared_ptr<buffer>& err_msgs)
        {
            HRESULT retval = hResult;

            if ((hResult == D3DERR_INVALIDCALL) ||
                (hResult == D3DXERR_INVALIDDATA) ||
                (hResult == E_FAIL))
            {
                retval = WGXERR_SHADER_COMPILE_FAILED;
            }

#if defined(DBG) || defined(DEBUG) || defined(_DEBUG)

            //
            // Output compiler errors
            //

            auto szErrors =
                static_cast<char const*>(err_msgs->get_buffer_data().buffer);
            TraceTag((tagError, "MIL-HW: Vertex Shader compiler errors:\n%s", szErrors));

#endif // defined(DBG) || defined(DEBUG) || defined(_DEBUG)

            return hResult;
        }

    };
}

