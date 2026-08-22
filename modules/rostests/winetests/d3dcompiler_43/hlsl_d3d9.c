/*
 * Copyright (C) 2010 Travis Athougies
 * Copyright (C) 2020 Zebediah Figura for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#define COBJMACROS
#include "wine/test.h"
#include "d3dx9.h"
#include "d3dcompiler.h"

#include <math.h>

#define D3DXERR_INVALIDDATA 0x88760b59

static HRESULT (WINAPI *pD3DAssemble)(const void *data, SIZE_T datasize, const char *filename,
        const D3D_SHADER_MACRO *defines, ID3DInclude *include, UINT flags,
        ID3DBlob **shader, ID3DBlob **error_messages);

static HRESULT (WINAPI *pD3DXGetShaderConstantTable)(const DWORD *byte_code, ID3DXConstantTable **constant_table);

struct vec2
{
    float x, y;
};

struct vec4
{
    float x, y, z, w;
};

static WCHAR temp_dir[MAX_PATH];

static BOOL create_file(const WCHAR *filename, const char *data, unsigned int size, WCHAR *out_path)
{
    WCHAR path[MAX_PATH];
    DWORD written;
    HANDLE file;

    if (!temp_dir[0])
        GetTempPathW(ARRAY_SIZE(temp_dir), temp_dir);
    lstrcpyW(path, temp_dir);
    lstrcatW(path, filename);

    file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE)
        return FALSE;

    if (WriteFile(file, data, size, &written, NULL))
    {
        CloseHandle(file);

        if (out_path)
            lstrcpyW(out_path, path);
        return TRUE;
    }

    CloseHandle(file);
    return FALSE;
}

static void delete_file(const WCHAR *filename)
{
    WCHAR path[MAX_PATH];

    lstrcpyW(path, temp_dir);
    lstrcatW(path, filename);
    DeleteFileW(path);
}

static BOOL create_directory(const WCHAR *dir)
{
    WCHAR path[MAX_PATH];

    lstrcpyW(path, temp_dir);
    lstrcatW(path, dir);
    return CreateDirectoryW(path, NULL);
}

static void delete_directory(const WCHAR *dir)
{
    WCHAR path[MAX_PATH];

    lstrcpyW(path, temp_dir);
    lstrcatW(path, dir);
    RemoveDirectoryW(path);
}

#define compile_shader(a, b, c) compile_shader_(__LINE__, a, b, c)
static ID3D10Blob *compile_shader_(unsigned int line, const char *source, const char *target,
        unsigned int flags)
{
    ID3D10Blob *blob = NULL, *errors = NULL;
    HRESULT hr;

    hr = D3DCompile(source, strlen(source), NULL, NULL, NULL, "main", target, flags, 0, &blob, &errors);
    ok_(__FILE__, line)(hr == D3D_OK, "Failed to compile shader, hr %#lx.\n", hr);
    if (errors)
    {
        if (winetest_debug > 1)
            trace_(__FILE__, line)("%s\n", (char *)ID3D10Blob_GetBufferPointer(errors));
        ID3D10Blob_Release(errors);
    }
    return blob;
}

static IDirect3DDevice9 *create_device(HWND window)
{
    D3DPRESENT_PARAMETERS present_parameters =
    {
        .Windowed = TRUE,
        .hDeviceWindow = window,
        .SwapEffect = D3DSWAPEFFECT_DISCARD,
        .BackBufferWidth = 640,
        .BackBufferHeight = 480,
        .BackBufferFormat = D3DFMT_A8R8G8B8,
    };
    IDirect3DDevice9 *device;
    IDirect3DSurface9 *rt;
    IDirect3D9 *d3d;
    D3DCAPS9 caps;
    HRESULT hr;

    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d)
    {
        skip("Failed to create a D3D object.\n");
        return NULL;
    }

    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window,
            D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_parameters, &device);
    IDirect3D9_Release(d3d);
    if (FAILED(hr))
    {
        skip("Failed to create a 3D device, hr %#lx.\n", hr);
        return NULL;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    if (caps.PixelShaderVersion < D3DPS_VERSION(2, 0) || caps.VertexShaderVersion < D3DVS_VERSION(2, 0))
    {
        skip("No shader model 2 support.\n");
        IDirect3DDevice9_Release(device);
        return NULL;
    }

    if (FAILED(hr = IDirect3DDevice9_CreateRenderTarget(device, 640, 480, D3DFMT_A32B32G32R32F,
            D3DMULTISAMPLE_NONE, 0, FALSE, &rt, NULL)))
    {
        skip("Failed to create an A32B32G32R32F surface, hr %#lx.\n", hr);
        IDirect3DDevice9_Release(device);
        return NULL;
    }
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9_SetRenderTarget(device, 0, rt);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    IDirect3DSurface9_Release(rt);

    return device;
}

struct test_context
{
    IDirect3DDevice9 *device;
    HWND window;
};

#define init_test_context(a) init_test_context_(__LINE__, a)
static BOOL init_test_context_(unsigned int line, struct test_context *context)
{
    RECT rect = {0, 0, 640, 480};

    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    context->window = CreateWindowA("static", "d3dcompiler_test", WS_OVERLAPPEDWINDOW,
            0, 0, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, NULL, NULL);
    ok(!!context->window, "Failed to create a window.\n");

    if (!(context->device = create_device(context->window)))
    {
        DestroyWindow(context->window);
        return FALSE;
    }

    return TRUE;
}

#define release_test_context(context) release_test_context_(__LINE__, context)
static void release_test_context_(unsigned int line, struct test_context *context)
{
    ULONG ref = IDirect3DDevice9_Release(context->device);
    ok_(__FILE__, line)(!ref, "Device has %lu references left.\n", ref);
    DestroyWindow(context->window);
}

#define draw_quad(device, ps_code) draw_quad_(__LINE__, device, ps_code)
static void draw_quad_(unsigned int line, IDirect3DDevice9 *device, ID3D10Blob *ps_code)
{
    IDirect3DVertexDeclaration9 *vertex_declaration;
    IDirect3DVertexShader9 *vs;
    IDirect3DPixelShader9 *ps;
    ID3D10Blob *vs_code;
    HRESULT hr;

    static const D3DVERTEXELEMENT9 decl_elements[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 8, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };

    static const struct
    {
        struct vec2 position;
        struct vec2 t0;
    }
    quad[] =
    {
        {{-1.0f, -1.0f}, {0.0f, 1.0f}},
        {{-1.0f,  1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f}, {1.0f, 1.0f}},
        {{ 1.0f,  1.0f}, {1.0f, 0.0f}},
    };

    static const char vs_source[] =
        "float4 main(float4 pos : POSITION, inout float2 texcoord : TEXCOORD0) : POSITION\n"
        "{\n"
        "   return pos;\n"
        "}";

    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements, &vertex_declaration);
    ok_(__FILE__, line)(hr == D3D_OK, "Failed to create vertex declaration, hr %#lx.\n", hr);

    hr = IDirect3DDevice9_SetVertexDeclaration(device, vertex_declaration);
    ok_(__FILE__, line)(hr == D3D_OK, "Failed to set vertex declaration, hr %#lx.\n", hr);

    vs_code = compile_shader(vs_source, "vs_2_0", 0);

    hr = IDirect3DDevice9_CreateVertexShader(device, ID3D10Blob_GetBufferPointer(vs_code), &vs);
    ok_(__FILE__, line)(hr == D3D_OK, "Failed to create vertex shader, hr %#lx.\n", hr);

    hr = IDirect3DDevice9_SetVertexShader(device, vs);
    ok_(__FILE__, line)(hr == D3D_OK, "Failed to set vertex shader, hr %#lx.\n", hr);

    hr = IDirect3DDevice9_CreatePixelShader(device, ID3D10Blob_GetBufferPointer(ps_code), &ps);
    ok_(__FILE__, line)(hr == D3D_OK, "Failed to create pixel shader, hr %#lx.\n", hr);

    hr = IDirect3DDevice9_SetPixelShader(device, ps);
    ok_(__FILE__, line)(hr == D3D_OK, "Failed to set pixel shader, hr %#lx.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok_(__FILE__, line)(hr == D3D_OK, "Failed to draw, hr %#lx.\n", hr);

    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok_(__FILE__, line)(hr == D3D_OK, "Failed to draw, hr %#lx.\n", hr);

    hr = IDirect3DDevice9_EndScene(device);
    ok_(__FILE__, line)(hr == D3D_OK, "Failed to draw, hr %#lx.\n", hr);

    IDirect3DVertexDeclaration9_Release(vertex_declaration);
    IDirect3DVertexShader9_Release(vs);
    IDirect3DPixelShader9_Release(ps);
    ID3D10Blob_Release(vs_code);
}

struct readback
{
    IDirect3DSurface9 *surface;
    D3DLOCKED_RECT rect;
};

static void init_readback(IDirect3DDevice9 *device, struct readback *rb)
{
    IDirect3DSurface9 *rt;
    D3DSURFACE_DESC desc;
    HRESULT hr;

    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &rt);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DSurface9_GetDesc(rt, &desc);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, desc.Width, desc.Height,
            desc.Format, D3DPOOL_SYSTEMMEM, &rb->surface, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DDevice9_GetRenderTargetData(device, rt, rb->surface);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DSurface9_LockRect(rb->surface, &rb->rect, NULL, D3DLOCK_READONLY);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    IDirect3DSurface9_Release(rt);
}

static const struct vec4 *get_readback_vec4(const struct readback *rb, unsigned int x, unsigned int y)
{
    return (struct vec4 *)((BYTE *)rb->rect.pBits + y * rb->rect.Pitch + x * sizeof(struct vec4));
}

static void release_readback(struct readback *rb)
{
    IDirect3DSurface9_UnlockRect(rb->surface);
    IDirect3DSurface9_Release(rb->surface);
}

static struct vec4 get_color_vec4(IDirect3DDevice9 *device, unsigned int x, unsigned int y)
{
    struct readback rb;
    struct vec4 ret;

    init_readback(device, &rb);
    ret = *get_readback_vec4(&rb, x, y);
    release_readback(&rb);

    return ret;
}

static BOOL compare_uint(unsigned int x, unsigned int y, unsigned int max_diff)
{
    unsigned int diff = x > y ? x - y : y - x;

    return diff <= max_diff;
}

static BOOL compare_float(float f, float g, unsigned int ulps)
{
    int x = *(int *)&f;
    int y = *(int *)&g;

    if (x < 0)
        x = INT_MIN - x;
    if (y < 0)
        y = INT_MIN - y;

    return compare_uint(x, y, ulps);
}

static BOOL compare_vec4(const struct vec4 *vec, float x, float y, float z, float w, unsigned int ulps)
{
    return compare_float(vec->x, x, ulps)
            && compare_float(vec->y, y, ulps)
            && compare_float(vec->z, z, ulps)
            && compare_float(vec->w, w, ulps);
}

static void test_swizzle(void)
{
    static const D3DXVECTOR4 color = {0.0303f, 0.0f, 0.0f, 0.0202f};
    struct test_context test_context;
    ID3DXConstantTable *constants;
    ID3D10Blob *ps_code = NULL;
    IDirect3DDevice9 *device;
    unsigned int i;
    struct vec4 v;
    HRESULT hr;

    static const struct
    {
        const char *source;
        struct vec4 color;
    }
    tests[] =
    {
        {
            "uniform float4 color;\n"
            "float4 main() : COLOR\n"
            "{\n"
            "    float4 ret = color;\n"
            "    ret.gb = ret.ra;\n"
            "    ret.ra = float2(0.0101, 0.0404);\n"
            "    return ret;\n"
            "}",
            {0.0101f, 0.0303f, 0.0202f, 0.0404f}
        },
        {
            "float4 main() : COLOR\n"
            "{\n"
            "    float4 ret = float4(0.1, 0.2, 0.3, 0.4);\n"
            "    ret.wyz.yx = float2(0.5, 0.6).yx;\n"
            "    return ret;\n"
            "}",
            {0.1f, 0.6f, 0.3f, 0.5f}
        },
        {
            "float4 main() : COLOR\n"
            "{\n"
            "    float4 ret;\n"
            "    ret.zwyx = float4(0.1, 0.2, 0.3, 0.4);\n"
            "    return ret;\n"
            "}",
            {0.4f, 0.3f, 0.1f, 0.2f}
        },
        {
            "float4 main() : COLOR\n"
            "{\n"
            "    float4 ret;\n"
            "    ret.yw.y = 0.1;\n"
            "    ret.xzy.yz.y.x = 0.2;\n"
            "    ret.yzwx.yzwx.wz.y = 0.3;\n"
            "    ret.zxy.xyz.zxy.xy.y = 0.4;\n"
            "    return ret;\n"
            "}",
            {0.3f, 0.2f, 0.4f, 0.1f}
        },
        {
            "float4 main() : COLOR\n"
            "{\n"
            "    float4 ret;\n"
            "    ret.yxz.yx = float2(0.1, 0.2);\n"
            "    ret.w.x = 0.3;\n"
            "    ret.wzyx.zyx.yx.x = 0.4;\n"
            "    return ret;\n"
            "}",
            {0.1f, 0.2f, 0.4f, 0.3f}
        },
        {
            "float4 main() : COLOR\n"
            "{\n"
            "    float4 ret = float4(0.1, 0.2, 0.3, 0.4).ywxz.zyyz;\n"
            "    return ret;\n"
            "}",
            {0.1f, 0.4f, 0.4f, 0.1f}
        },
        {
            "float4 main() : COLOR\n"
            "{\n"
            "    float4 ret = float4(0.1, 0.2, 0.3, 0.4);\n"
            "    ret.yxwz = ret;\n"
            "    ret = ret.wyzx;\n"
            "    return ret;\n"
            "}",
            {0.3f, 0.1f, 0.4f, 0.2f}
        },
        {
            "float4 main() : COLOR\n"
            "{\n"
            "    float4 ret;\n"
            "    ret.xyzw.xyzw = float4(0.1, 0.2, 0.3, 0.4);\n"
            "    return ret;\n"
            "}",
            {0.1f, 0.2f, 0.3f, 0.4f}
        },
    };

    if (!init_test_context(&test_context))
        return;
    device = test_context.device;

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        ps_code = compile_shader(tests[i].source, "ps_2_0", 0);
        if (i == 0)
        {
            hr = pD3DXGetShaderConstantTable(ID3D10Blob_GetBufferPointer(ps_code), &constants);
            ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
            hr = ID3DXConstantTable_SetVector(constants, device, "color", &color);
            ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
            ID3DXConstantTable_Release(constants);
        }
        draw_quad(device, ps_code);

        v = get_color_vec4(device, 0, 0);
        ok(compare_vec4(&v, tests[i].color.x, tests[i].color.y, tests[i].color.z, tests[i].color.w, 0),
                "Test %u: Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", i, v.x, v.y, v.z, v.w);

        ID3D10Blob_Release(ps_code);
    }

    release_test_context(&test_context);
}

static void test_math(void)
{
    struct test_context test_context;
    ID3DXConstantTable *constants;
    ID3D10Blob *ps_code = NULL;
    IDirect3DDevice9 *device;
    struct vec4 v;
    HRESULT hr;

    static const char ps_source[] =
        "float4 main(uniform float u, uniform float v, uniform float w, uniform float x,\n"
        "            uniform float y, uniform float z): COLOR\n"
        "{\n"
        "    return float4(x * y - z / w + --u / -v,\n"
        "            z * x / y + w / -v,\n"
        "            u + v - w,\n"
        "            x / y / w);\n"
        "}";

    if (!init_test_context(&test_context))
        return;
    device = test_context.device;

    ps_code = compile_shader(ps_source, "ps_2_0", 0);
    hr = pD3DXGetShaderConstantTable(ID3D10Blob_GetBufferPointer(ps_code), &constants);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DXConstantTable_SetFloat(constants, device, "$u", 2.5f);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DXConstantTable_SetFloat(constants, device, "$v", 0.3f);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DXConstantTable_SetFloat(constants, device, "$w", 0.2f);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DXConstantTable_SetFloat(constants, device, "$x", 0.7f);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DXConstantTable_SetFloat(constants, device, "$y", 0.1f);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DXConstantTable_SetFloat(constants, device, "$z", 1.5f);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    ID3DXConstantTable_Release(constants);

    draw_quad(device, ps_code);

    v = get_color_vec4(device, 0, 0);
    ok(compare_vec4(&v, -12.43f, 9.833333f, 1.6f, 35.0f, 1),
            "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v.x, v.y, v.z, v.w);

    ID3D10Blob_Release(ps_code);
    release_test_context(&test_context);
}

static void test_conditionals(void)
{
    struct test_context test_context;
    ID3D10Blob *ps_code = NULL;
    IDirect3DDevice9 *device;
    const struct vec4 *v;
    struct readback rb;
    unsigned int i;

    static const char ps_if_source[] =
        "float4 main(float2 pos : TEXCOORD0) : COLOR\n"
        "{\n"
        "    if((pos.x * 640.0) > 200.0)\n"
        "        return float4(0.1, 0.2, 0.3, 0.4);\n"
        "    else\n"
        "        return float4(0.9, 0.8, 0.7, 0.6);\n"
        "}";

    static const char ps_ternary_source[] =
        "float4 main(float2 pos : TEXCOORD0) : COLOR\n"
        "{\n"
        "    return (pos.x < 0.5 ? float4(0.5, 0.25, 0.5, 0.75) : float4(0.6, 0.8, 0.1, 0.2));\n"
        "}";

    if (!init_test_context(&test_context))
        return;
    device = test_context.device;

    todo_wine
    ps_code = compile_shader(ps_if_source, "ps_2_0", 0);
    if (ps_code)
    {
        draw_quad(device, ps_code);
        init_readback(device, &rb);

        for (i = 0; i < 200; i += 40)
        {
            v = get_readback_vec4(&rb, i, 0);
            todo_wine ok(compare_vec4(v, 0.9f, 0.8f, 0.7f, 0.6f, 0),
                         "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v->x, v->y, v->z, v->w);
        }

        for (i = 240; i < 640; i += 40)
        {
            v = get_readback_vec4(&rb, i, 0);
            todo_wine ok(compare_vec4(v, 0.1f, 0.2f, 0.3f, 0.4f, 0),
                         "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v->x, v->y, v->z, v->w);
        }

        release_readback(&rb);
        ID3D10Blob_Release(ps_code);
    }

    ps_code = compile_shader(ps_ternary_source, "ps_2_0", 0);
    if (ps_code)
    {
        draw_quad(device, ps_code);
        init_readback(device, &rb);

        for (i = 0; i < 320; i += 40)
        {
            v = get_readback_vec4(&rb, i, 0);
            ok(compare_vec4(v, 0.5f, 0.25f, 0.5f, 0.75f, 0),
                    "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v->x, v->y, v->z, v->w);
        }

        for (i = 360; i < 640; i += 40)
        {
            v = get_readback_vec4(&rb, i, 0);
            ok(compare_vec4(v, 0.6f, 0.8f, 0.1f, 0.2f, 0),
                    "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v->x, v->y, v->z, v->w);
        }

        release_readback(&rb);
        ID3D10Blob_Release(ps_code);
    }

    release_test_context(&test_context);
}

static void test_float_vectors(void)
{
    struct test_context test_context;
    ID3DXConstantTable *constants;
    ID3D10Blob *ps_code = NULL;
    IDirect3DDevice9 *device;
    struct vec4 v;
    HRESULT hr;

    static const char ps_indexing_source[] =
        "float4 main() : COLOR\n"
        "{\n"
        "    float4 color;\n"
        "    color[0] = 0.020;\n"
        "    color[1] = 0.245;\n"
        "    color[2] = 0.351;\n"
        "    color[3] = 1.0;\n"
        "    return color;\n"
        "}";

    /* A uniform index is used so that the compiler can't optimize. */
    static const char ps_uniform_indexing_source[] =
        "uniform int i;\n"
        "float4 main() : COLOR\n"
        "{\n"
        "    float4 color = float4(0.5, 0.4, 0.3, 0.2);\n"
        "    color.g = color[i];\n"
        "    color.b = 0.8;\n"
        "    return color;\n"
        "}";

    if (!init_test_context(&test_context))
        return;
    device = test_context.device;

    ps_code = compile_shader(ps_indexing_source, "ps_2_0", 0);
    if (ps_code)
    {
        draw_quad(device, ps_code);

        v = get_color_vec4(device, 0, 0);
        ok(compare_vec4(&v, 0.02f, 0.245f, 0.351f, 1.0f, 0),
                "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v.x, v.y, v.z, v.w);

        ID3D10Blob_Release(ps_code);
    }

    ps_code = compile_shader(ps_uniform_indexing_source, "ps_2_0", 0);
    if (ps_code)
    {
        hr = pD3DXGetShaderConstantTable(ID3D10Blob_GetBufferPointer(ps_code), &constants);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
        hr = ID3DXConstantTable_SetInt(constants, device, "i", 2);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
        ID3DXConstantTable_Release(constants);
        draw_quad(device, ps_code);

        v = get_color_vec4(device, 0, 0);
        ok(compare_vec4(&v, 0.5f, 0.3f, 0.8f, 0.2f, 0),
                "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v.x, v.y, v.z, v.w);

        ID3D10Blob_Release(ps_code);
    }

    release_test_context(&test_context);
}

static void test_trig(void)
{
    struct test_context test_context;
    ID3D10Blob *ps_code = NULL;
    IDirect3DDevice9 *device;
    const struct vec4 *v;
    struct readback rb;
    unsigned int i;

    static const char ps_source[] =
        "float4 main(float x : TEXCOORD0) : COLOR\n"
        "{\n"
        "    const float pi2 = 6.2831853;\n"
        "    float calcd_sin = (sin(x * pi2) + 1)/2;\n"
        "    float calcd_cos = (cos(x * pi2) + 1)/2;\n"
        "    return float4(calcd_sin, calcd_cos, 0, 0);\n"
        "}";

    if (!init_test_context(&test_context))
        return;
    device = test_context.device;

    ps_code = compile_shader(ps_source, "ps_2_0", 0);
    draw_quad(device, ps_code);
    init_readback(device, &rb);
    for (i = 0; i < 32; ++i)
    {
        float expect_x = (sinf(i * 2 * M_PI / 32) + 1.0f) / 2.0f;
        float expect_y = (cosf(i * 2 * M_PI / 32) + 1.0f) / 2.0f;
        v = get_readback_vec4(&rb, i * 640 / 32, 0);
        ok(compare_vec4(v, expect_x, expect_y, 0.0f, 0.0f, 4096),
                "Test %u: Got {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                i, v->x, v->y, v->z, v->w, expect_x, expect_y, 0.0f, 0.0f);
    }
    release_readback(&rb);
    ID3D10Blob_Release(ps_code);
    release_test_context(&test_context);
}

static void test_comma(void)
{
    struct test_context test_context;
    ID3D10Blob *ps_code = NULL;
    struct vec4 v;

    static const char ps_source[] =
        "float4 main(float x: TEXCOORD0): COLOR\n"
        "{\n"
        "    float4 ret;\n"
        "    return (ret = float4(0.1, 0.2, 0.3, 0.4)), ret + 0.5;\n"
        "}";

    if (!init_test_context(&test_context))
        return;

    ps_code = compile_shader(ps_source, "ps_2_0", 0);
    draw_quad(test_context.device, ps_code);

    v = get_color_vec4(test_context.device, 0, 0);
    ok(compare_vec4(&v, 0.6f, 0.7f, 0.8f, 0.9f, 0),
            "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v.x, v.y, v.z, v.w);

    ID3D10Blob_Release(ps_code);
    release_test_context(&test_context);
}

static void test_return(void)
{
    struct test_context test_context;
    ID3D10Blob *ps_code = NULL;
    struct vec4 v;

    static const char *void_source =
        "void main(float x : TEXCOORD0, out float4 ret : COLOR)\n"
        "{\n"
        "    ret = float4(0.1, 0.2, 0.3, 0.4);\n"
        "    return;\n"
        "    ret = float4(0.5, 0.6, 0.7, 0.8);\n"
        "}";

    static const char *implicit_conversion_source =
        "float4 main(float x : TEXCOORD0) : COLOR\n"
        "{\n"
        "    return float2x2(0.4, 0.3, 0.2, 0.1);\n"
        "}";

    if (!init_test_context(&test_context))
        return;

    ps_code = compile_shader(void_source, "ps_2_0", 0);
    draw_quad(test_context.device, ps_code);

    v = get_color_vec4(test_context.device, 0, 0);
    ok(compare_vec4(&v, 0.1f, 0.2f, 0.3f, 0.4f, 0),
            "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v.x, v.y, v.z, v.w);

    ID3D10Blob_Release(ps_code);

    ps_code = compile_shader(implicit_conversion_source, "ps_2_0", 0);
    if (ps_code)
    {
        draw_quad(test_context.device, ps_code);

        v = get_color_vec4(test_context.device, 0, 0);
        ok(compare_vec4(&v, 0.4f, 0.3f, 0.2f, 0.1f, 0),
                "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v.x, v.y, v.z, v.w);

        ID3D10Blob_Release(ps_code);
    }

    release_test_context(&test_context);
}

static void test_array_dimensions(void)
{
    struct test_context test_context;
    ID3D10Blob *ps_code = NULL;
    struct vec4 v;

    static const char shader[] =
        "float4 main(float x : TEXCOORD0) : COLOR\n"
        "{\n"
        "    const int dim = 4;\n"
        "    float a[2 * 2] = {0.1, 0.2, 0.3, 0.4};\n"
        "    float b[4.1] = a;\n"
        "    float c[dim] = b;\n"
        "    float d[true] = {c[0]};\n"
        "    float e[65536];\n"
        "    return float4(d[0], c[0], c[1], c[3]);\n"
        "}";

    if (!init_test_context(&test_context))
        return;

    todo_wine ps_code = compile_shader(shader, "ps_2_0", 0);
    if (ps_code)
    {
        draw_quad(test_context.device, ps_code);

        v = get_color_vec4(test_context.device, 0, 0);
        ok(compare_vec4(&v, 0.1f, 0.1f, 0.2f, 0.4f, 0),
                "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v.x, v.y, v.z, v.w);

        ID3D10Blob_Release(ps_code);
    }

    release_test_context(&test_context);
}

static void test_majority(void)
{
    static const float data[] = { 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f };
    struct test_context test_context;
    ID3D10Blob *ps_code = NULL;
    struct vec4 v;
    HRESULT hr;

    static const char ps_typedef_source[] =
        "typedef float2x2 matrix_t;\n"
        "typedef row_major matrix_t row_matrix_t;\n"
        "typedef column_major matrix_t col_matrix_t;\n"
        "uniform row_matrix_t m1;\n"
        "uniform col_matrix_t m2;\n"
        "float4 main() : COLOR\n"
        "{\n"
        "    float4 ret;\n"
        "    ret.xy = m1[0];\n"
        "    ret.zw = m2[0];\n"
        "    return ret;\n"
        "}";

    static const char ps_pragmas_source[] =
        "#pragma pack_matrix(row_major)\n"
        "uniform float2x2 m1;\n"
        "#pragma pack_matrix(column_major)\n"
        "uniform float2x2 m2;\n"
        "float4 main() : COLOR\n"
        "{\n"
        "    float4 ret;\n"
        "    ret.xy = m1[0];\n"
        "    ret.zw = m2[0];\n"
        "    return ret;\n"
        "}";

    static const char ps_row_source[] =
        "uniform row_major float2x2 m1;\n"
        "uniform row_major float2x2 m2;\n"
        "float4 main() : COLOR\n"
        "{\n"
        "    float4 ret;\n"
        "    ret.xy = m1[0];\n"
        "    ret.zw = m2[0];\n"
        "    return ret;\n"
        "}";

    static const char ps_column_source[] =
        "uniform column_major float2x2 m1;\n"
        "uniform column_major float2x2 m2;\n"
        "float4 main() : COLOR\n"
        "{\n"
        "    float4 ret;\n"
        "    ret.xy = m1[0];\n"
        "    ret.zw = m2[0];\n"
        "    return ret;\n"
        "}";

    static const char ps_no_modifiers_source[] =
        "uniform float2x2 m1;\n"
        "uniform float2x2 m2;\n"
        "float4 main() : COLOR\n"
        "{\n"
        "    float4 ret;\n"
        "    ret.xy = m1[0];\n"
        "    ret.zw = m2[0];\n"
        "    return ret;\n"
        "}";

    static const struct test
    {
        const char *code;
        struct vec4 color;
        unsigned int flags;
    }
    tests[] =
    {
        { ps_typedef_source, { 0.1f, 0.2f, 0.1f, 0.5f } },
        { ps_pragmas_source, { 0.1f, 0.2f, 0.1f, 0.5f } },
        { ps_row_source, { 0.1f, 0.2f, 0.1f, 0.2f } },
        { ps_column_source, { 0.1f, 0.5f, 0.1f, 0.5f } },
        { ps_no_modifiers_source, { 0.1f, 0.2f, 0.1f, 0.2f }, D3DCOMPILE_PACK_MATRIX_ROW_MAJOR },
        { ps_no_modifiers_source, { 0.1f, 0.5f, 0.1f, 0.5f }, D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR },
        { ps_no_modifiers_source, { 0.1f, 0.2f, 0.1f, 0.2f }, D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_PACK_MATRIX_ROW_MAJOR },
        { ps_pragmas_source, { 0.1f, 0.2f, 0.1f, 0.5f }, D3DCOMPILE_PACK_MATRIX_ROW_MAJOR },
        { ps_pragmas_source, { 0.1f, 0.2f, 0.1f, 0.5f }, D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR },
    };
    unsigned int i;

    if (!init_test_context(&test_context))
        return;

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        const struct vec4 *c = &tests[i].color;

        winetest_push_context("Test %u", i);

        ps_code = compile_shader(tests[i].code, "ps_2_0", tests[i].flags);
        if (ps_code)
        {
            ID3DXConstantTable *constants;
            D3DXCONSTANT_DESC desc;
            D3DXHANDLE h;
            UINT count;

            hr = pD3DXGetShaderConstantTable(ID3D10Blob_GetBufferPointer(ps_code), &constants);
            ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

            h = ID3DXConstantTable_GetConstantByName(constants, NULL, "m1");
            ok(!!h, "Failed to find a constant.\n");
            count = 1;
            hr = ID3DXConstantTable_GetConstantDesc(constants, h, &desc, &count);
            ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
            hr = IDirect3DDevice9_SetPixelShaderConstantF(test_context.device, desc.RegisterIndex,
                    data, desc.RegisterCount);
            ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

            h = ID3DXConstantTable_GetConstantByName(constants, NULL, "m2");
            ok(!!h, "Failed to find a constant.\n");
            count = 1;
            hr = ID3DXConstantTable_GetConstantDesc(constants, h, &desc, &count);
            ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
            hr = IDirect3DDevice9_SetPixelShaderConstantF(test_context.device, desc.RegisterIndex,
                    data, desc.RegisterCount);
            ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

            ID3DXConstantTable_Release(constants);

            draw_quad(test_context.device, ps_code);

            v = get_color_vec4(test_context.device, 0, 0);
            ok(compare_vec4(&v, c->x, c->y, c->z, c->w, 1),
                "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v.x, v.y, v.z, v.w);

            ID3D10Blob_Release(ps_code);
        }

        winetest_pop_context();
    }

    release_test_context(&test_context);
}

static void test_struct_assignment(void)
{
    struct test_context test_context;
    ID3D10Blob *ps_code = NULL;
    struct vec4 v;

    static const char ps_source[] =
        "struct apple\n"
        "{\n"
        "    struct\n"
        "    {\n"
        "        float4 a;\n"
        "    } m;\n"
        "    float4 b;\n"
        "};\n"
        "float4 main() : COLOR\n"
        "{\n"
        "    struct apple q, r, s;\n"
        "    q.m.a = float4(0.1, 0.2, 0.3, 0.4);\n"
        "    q.b = float4(0.5, 0.1, 0.4, 0.5);\n"
        "    s = r = q;\n"
        "    return s.m.a + s.b;\n"
        "}";

    if (!init_test_context(&test_context))
        return;

    ps_code = compile_shader(ps_source, "ps_2_0", 0);
    draw_quad(test_context.device, ps_code);

    v = get_color_vec4(test_context.device, 0, 0);
    ok(compare_vec4(&v, 0.6f, 0.3f, 0.7f, 0.9f, 1),
            "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v.x, v.y, v.z, v.w);

    ID3D10Blob_Release(ps_code);
    release_test_context(&test_context);
}

static void test_struct_semantics(void)
{
    struct test_context test_context;
    ID3D10Blob *ps_code = NULL;
    struct vec4 v;

    static const char ps_source[] =
        "struct input\n"
        "{\n"
        "    struct\n"
        "    {\n"
        "        float4 texcoord : TEXCOORD0;\n"
        "    } m;\n"
        "};\n"
        "struct output\n"
        "{\n"
        "    struct\n"
        "    {\n"
        "        float4 color : COLOR;\n"
        "    } m;\n"
        "};\n"
        "struct output main(struct input i)\n"
        "{\n"
        "    struct output o;\n"
        "    o.m.color = i.m.texcoord;\n"
        "    return o;\n"
        "}";

    if (!init_test_context(&test_context))
        return;

    ps_code = compile_shader(ps_source, "ps_2_0", 0);
    draw_quad(test_context.device, ps_code);

    v = get_color_vec4(test_context.device, 64, 48);
    v.z = v.w = 0.0f;
    ok(compare_vec4(&v, 0.1f, 0.1f, 0.0f, 0.0f, 4096),
            "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v.x, v.y, v.z, v.w);
    v = get_color_vec4(test_context.device, 320, 240);
    v.z = v.w = 0.0f;
    ok(compare_vec4(&v, 0.5f, 0.5f, 0.0f, 0.0f, 4096),
            "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v.x, v.y, v.z, v.w);

    ID3D10Blob_Release(ps_code);
    release_test_context(&test_context);
}

static void test_global_initializer(void)
{
    struct test_context test_context;
    ID3DXConstantTable *constants;
    ID3D10Blob *ps_code = NULL;
    struct vec4 v;
    HRESULT hr;

    static const char ps_source[] =
        "float myfunc()\n"
        "{\n"
        "    return 0.6;\n"
        "}\n"
        "static float sf = myfunc() + 0.2;\n"
        "uniform float uf = 0.2;\n"
        "float4 main() : COLOR\n"
        "{\n"
        "    return float4(sf, uf, 0, 0);\n"
        "}";

    if (!init_test_context(&test_context))
        return;

    ps_code = compile_shader(ps_source, "ps_2_0", 0);
    hr = pD3DXGetShaderConstantTable(ID3D10Blob_GetBufferPointer(ps_code), &constants);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DXConstantTable_SetDefaults(constants, test_context.device);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    ID3DXConstantTable_Release(constants);
    draw_quad(test_context.device, ps_code);

    v = get_color_vec4(test_context.device, 0, 0);
    ok(compare_vec4(&v, 0.8f, 0.2f, 0.0f, 0.0f, 4096),
            "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v.x, v.y, v.z, v.w);

    ID3D10Blob_Release(ps_code);
    release_test_context(&test_context);
}

static void test_samplers(void)
{
    struct test_context test_context;
    IDirect3DTexture9 *texture;
    ID3D10Blob *ps_code = NULL;
    D3DLOCKED_RECT map_desc;
    unsigned int i;
    struct vec4 v;
    HRESULT hr;

    static const char *tests[] =
    {
        "sampler s;\n"
        "float4 main() : COLOR\n"
        "{\n"
        "    return tex2D(s, float2(0.75, 0.25));\n"
        "}",

        "SamplerState s;\n"
        "float4 main() : COLOR\n"
        "{\n"
        "    return tex2D(s, float2(0.75, 0.25));\n"
        "}",

        "sampler2D s;\n"
        "float4 main() : COLOR\n"
        "{\n"
        "    return tex2D(s, float2(0.75, 0.25));\n"
        "}",

        "sampler s;\n"
        "Texture2D t;\n"
        "float4 main() : COLOR\n"
        "{\n"
        "    return t.Sample(s, float2(0.75, 0.25));\n"
        "}",

        "SamplerState s;\n"
        "Texture2D t;\n"
        "float4 main() : COLOR\n"
        "{\n"
        "    return t.Sample(s, float2(0.75, 0.25));\n"
        "}",
    };

    if (!init_test_context(&test_context))
        return;

    hr = IDirect3DDevice9_CreateTexture(test_context.device, 2, 2, 1, D3DUSAGE_DYNAMIC,
            D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DTexture9_LockRect(texture, 0, &map_desc, NULL, D3DLOCK_DISCARD);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    memset(map_desc.pBits, 0, 2 * map_desc.Pitch);
    ((DWORD *)map_desc.pBits)[1] = 0x00ff00ff;
    hr = IDirect3DTexture9_UnlockRect(texture, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DDevice9_SetTexture(test_context.device, 0, (IDirect3DBaseTexture9 *)texture);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(test_context.device, 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        hr = IDirect3DDevice9_Clear(test_context.device, 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(255, 0, 0), 1.0f, 0);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        todo_wine_if (i > 2)
        ps_code = compile_shader(tests[i], "ps_2_0", 0);
        if (ps_code)
        {
            draw_quad(test_context.device, ps_code);

            v = get_color_vec4(test_context.device, 0, 0);
            ok(compare_vec4(&v, 1.0f, 0.0f, 1.0f, 0.0f, 0),
                    "Test %u: Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", i, v.x, v.y, v.z, v.w);

            ID3D10Blob_Release(ps_code);
        }
    }

    IDirect3DTexture9_Release(texture);
    release_test_context(&test_context);
}

static void check_constant_desc(const char *prefix, const D3DXCONSTANT_DESC *desc,
        const D3DXCONSTANT_DESC *expect, BOOL nonzero_defaultvalue)
{
    ok(!strcmp(desc->Name, expect->Name), "%s: got Name %s.\n", prefix, debugstr_a(desc->Name));
    ok(desc->RegisterSet == expect->RegisterSet, "%s: got RegisterSet %#x.\n", prefix, desc->RegisterSet);
    if (desc->RegisterSet == D3DXRS_SAMPLER)
        ok(desc->RegisterIndex == expect->RegisterIndex, "%s: got RegisterIndex %u.\n", prefix, desc->RegisterIndex);
    ok(desc->RegisterCount == expect->RegisterCount, "%s: got RegisterCount %u.\n", prefix, desc->RegisterCount);
    ok(desc->Class == expect->Class, "%s: got Class %#x.\n", prefix, desc->Class);
    ok(desc->Type == expect->Type, "%s: got Type %#x.\n", prefix, desc->Type);
    ok(desc->Rows == expect->Rows, "%s: got Rows %u.\n", prefix, desc->Rows);
    ok(desc->Columns == expect->Columns, "%s: got Columns %u.\n", prefix, desc->Columns);
    ok(desc->Elements == expect->Elements, "%s: got Elements %u.\n", prefix, desc->Elements);
    ok(desc->StructMembers == expect->StructMembers, "%s: got StructMembers %u.\n", prefix, desc->StructMembers);
    ok(desc->Bytes == expect->Bytes, "%s: got Bytes %u.\n", prefix, desc->Bytes);
    ok(!!desc->DefaultValue == nonzero_defaultvalue, "%s: got DefaultValue %p.\n", prefix, desc->DefaultValue);
}

static void test_constant_table(void)
{
    static const char *source =
        "typedef float3x3 matrix_t;\n"
        "struct matrix_record { float3x3 a; } dummy;\n"
        "uniform float4 a;\n"
        "uniform float b;\n"
        "uniform float unused;\n"
        "uniform float3x1 c;\n"
        "uniform row_major float3x1 d;\n"
        "uniform uint e;\n"
        "uniform struct\n"
        "{\n"
        "    float2x2 a;\n"
        "    float b[2];\n"
        "    float c;\n"
        "#pragma pack_matrix(row_major)\n"
        "    float2x2 d;\n"
        "} f;\n"
        "uniform bool2 g[5];\n"
        "uniform matrix_t i;\n"
        "uniform struct matrix_record j;\n"
        "uniform matrix<float,3,1> k;\n"
        "sampler l : register(s5);\n"
        "sampler m {};\n"
        "texture dummy_texture;\n"
        "sampler n\n"
        "{\n"
        "    Texture = dummy_texture;\n"
        "    foo = bar + 2;\n"
        "};\n"
        "SamplerState o\n"
        "{\n"
        "    Texture = dummy_texture;\n"
        "    foo = bar + 2;\n"
        "};\n"
        "texture2D p;\n"
        "sampler q : register(s7);\n"
        "SamplerState r : register(s8);\n"
        "sampler2D s;\n"
        "float4 main(uniform float4 h, sampler t, uniform sampler u) : COLOR\n"
        "{\n"
        "    return b + c._31 + d._31 + f.d._22 + tex2D(l, g[e]) + tex3D(m, h.xyz) + i._33 + j.a._33 + k._31\n"
        "            + tex2D(n, a.xy) + tex2D(o, a.xy) + p.Sample(r, a.xy) + p.Sample(q, a.xy) + tex2D(s, a.xy)\n"
        "            + tex2D(t, a.xy) + tex2D(u, a.xy);\n"
        "}";

    D3DXCONSTANTTABLE_DESC table_desc;
    ID3DXConstantTable *constants;
    ID3D10Blob *ps_code = NULL;
    D3DXHANDLE handle, field;
    D3DXCONSTANT_DESC desc;
    unsigned int i, j;
    HRESULT hr;
    UINT count;

    static const D3DXCONSTANT_DESC expect_constants[] =
    {
        {"$h", D3DXRS_FLOAT4, 0, 1, D3DXPC_VECTOR, D3DXPT_FLOAT, 1, 4, 1, 0, 16},
        {"$u", D3DXRS_SAMPLER, 10, 1, D3DXPC_OBJECT, D3DXPT_SAMPLER2D, 1, 1, 1, 0, 4},
        {"a", D3DXRS_FLOAT4, 0, 1, D3DXPC_VECTOR, D3DXPT_FLOAT, 1, 4, 1, 0, 16},
        {"b", D3DXRS_FLOAT4, 0, 1, D3DXPC_SCALAR, D3DXPT_FLOAT, 1, 1, 1, 0, 4},
        {"c", D3DXRS_FLOAT4, 0, 1, D3DXPC_MATRIX_COLUMNS, D3DXPT_FLOAT, 3, 1, 1, 0, 12},
        {"d", D3DXRS_FLOAT4, 0, 3, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 3, 1, 1, 0, 12},
        {"e", D3DXRS_FLOAT4, 0, 1, D3DXPC_SCALAR, D3DXPT_INT, 1, 1, 1, 0, 4},
        {"f", D3DXRS_FLOAT4, 0, 7, D3DXPC_STRUCT, D3DXPT_VOID, 1, 11, 1, 4, 44},
        {"g", D3DXRS_FLOAT4, 0, 5, D3DXPC_VECTOR, D3DXPT_BOOL, 1, 2, 5, 0, 40},
        {"i", D3DXRS_FLOAT4, 0, 3, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 3, 3, 1, 0, 36},
        {"j", D3DXRS_FLOAT4, 0, 3, D3DXPC_STRUCT, D3DXPT_VOID, 1, 9, 1, 1, 36},
        {"k", D3DXRS_FLOAT4, 0, 3, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 3, 1, 1, 0, 12},
        {"l", D3DXRS_SAMPLER, 5, 1, D3DXPC_OBJECT, D3DXPT_SAMPLER2D, 1, 1, 1, 0, 4},
        {"m", D3DXRS_SAMPLER, 2, 1, D3DXPC_OBJECT, D3DXPT_SAMPLER3D, 1, 1, 1, 0, 4},
        {"n", D3DXRS_SAMPLER, 3, 1, D3DXPC_OBJECT, D3DXPT_SAMPLER2D, 1, 1, 1, 0, 4},
        {"o", D3DXRS_SAMPLER, 4, 1, D3DXPC_OBJECT, D3DXPT_SAMPLER2D, 1, 1, 1, 0, 4},
        {"q+p", D3DXRS_SAMPLER, 0, 1, D3DXPC_OBJECT, D3DXPT_TEXTURE2D, 1, 4, 1, 0, 16},
        {"r+p", D3DXRS_SAMPLER, 1, 1, D3DXPC_OBJECT, D3DXPT_TEXTURE2D, 1, 4, 1, 0, 16},
        {"s", D3DXRS_SAMPLER, 6, 1, D3DXPC_OBJECT, D3DXPT_SAMPLER2D, 1, 1, 1, 0, 4},
        {"t", D3DXRS_SAMPLER, 9, 1, D3DXPC_OBJECT, D3DXPT_SAMPLER2D, 1, 1, 1, 0, 4},
    };

    static const D3DXCONSTANT_DESC expect_fields_f[] =
    {
        {"a", D3DXRS_FLOAT4, 0, 2, D3DXPC_MATRIX_COLUMNS, D3DXPT_FLOAT, 2, 2, 1, 0, 16},
        {"b", D3DXRS_FLOAT4, 0, 2, D3DXPC_SCALAR, D3DXPT_FLOAT, 1, 1, 2, 0, 8},
        {"c", D3DXRS_FLOAT4, 0, 1, D3DXPC_SCALAR, D3DXPT_FLOAT, 1, 1, 1, 0, 4},
        {"d", D3DXRS_FLOAT4, 0, 2, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 2, 2, 1, 0, 16},
    };

    static const D3DXCONSTANT_DESC expect_fields_j =
        {"a", D3DXRS_FLOAT4, 0, 3, D3DXPC_MATRIX_COLUMNS, D3DXPT_FLOAT, 3, 3, 1, 0, 36};

    todo_wine ps_code = compile_shader(source, "ps_2_0", 0);
    if (!ps_code)
        return;

    hr = pD3DXGetShaderConstantTable(ID3D10Blob_GetBufferPointer(ps_code), &constants);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID3DXConstantTable_GetDesc(constants, &table_desc);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    ok(table_desc.Version == D3DPS_VERSION(2, 0), "Got unexpected version %#lx.\n", table_desc.Version);
    ok(table_desc.Constants == ARRAY_SIZE(expect_constants), "Got %u constants.\n", table_desc.Constants);

    for (i = 0; i < table_desc.Constants; ++i)
    {
        char prefix[30];

        handle = ID3DXConstantTable_GetConstant(constants, NULL, i);
        ok(!!handle, "Failed to get constant.\n");
        memset(&desc, 0xcc, sizeof(desc));
        count = 1;
        hr = ID3DXConstantTable_GetConstantDesc(constants, handle, &desc, &count);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
        ok(count == 1, "Got count %u.\n", count);
        sprintf(prefix, "Test %u", i);
        check_constant_desc(prefix, &desc, &expect_constants[i], FALSE);

        if (!strcmp(desc.Name, "f"))
        {
            for (j = 0; j < ARRAY_SIZE(expect_fields_f); ++j)
            {
                field = ID3DXConstantTable_GetConstant(constants, handle, j);
                ok(!!field, "Failed to get constant.\n");
                memset(&desc, 0xcc, sizeof(desc));
                count = 1;
                hr = ID3DXConstantTable_GetConstantDesc(constants, field, &desc, &count);
                ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
                ok(count == 1, "Got count %u.\n", count);
                sprintf(prefix, "Test %u, %u", i, j);
                check_constant_desc(prefix, &desc, &expect_fields_f[j], !!j);
            }
        }
        else if (!strcmp(desc.Name, "j"))
        {
            field = ID3DXConstantTable_GetConstant(constants, handle, 0);
            ok(!!field, "Failed to get constant.\n");
            memset(&desc, 0xcc, sizeof(desc));
            count = 1;
            hr = ID3DXConstantTable_GetConstantDesc(constants, field, &desc, &count);
            ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
            ok(count == 1, "Got count %u.\n", count);
            sprintf(prefix, "Test %u", i);
            check_constant_desc(prefix, &desc, &expect_fields_j, FALSE);
        }
    }

    ID3DXConstantTable_Release(constants);
    ID3D10Blob_Release(ps_code);
}

static void test_fail(void)
{
    static const char *tests[] =
    {
        /* 0 */
        "float4 test() : SV_TARGET\n"
        "{\n"
        "   return y;\n"
        "}",

        "float4 test() : SV_TARGET\n"
        "{\n"
        "  float4 x = float4(0, 0, 0, 0);\n"
        "  x.xzzx = float4(1, 2, 3, 4);\n"
        "  return x;\n"
        "}",

        "float4 test(float2 pos: TEXCOORD0) : SV_TARGET\n"
        "{\n"
        "  float4 x = pos;\n"
        "  return x;\n"
        "}",

        "float4 test(float2 pos, TEXCOORD0) ; SV_TARGET\n"
        "{\n"
        "  pos = float4 x;\n"
        "  mul(float4(5, 4, 3, 2), mvp) = x;\n"
        "  return float4;\n"
        "}",

        "float4 563r(float2 45s: TEXCOORD0) : SV_TARGET\n"
        "{\n"
        "  float2 x = 45s;\n"
        "  return float4(x.x, x.y, 0, 0);\n"
        "}",

        /* 5 */
        "float4 test() : SV_TARGET\n"
        "{\n"
        "   struct { int b,c; } x = {0};\n"
        "   return y;\n"
        "}",

        "float4 test() : SV_TARGET\n"
        "{\n"
        "   struct {} x = {};\n"
        "   return y;\n"
        "}",

        "float4 test(float2 pos : TEXCOORD0) : SV_TARGET\n"
        "{\n"
        "    return;\n"
        "}",

        "void test(float2 pos : TEXCOORD0)\n"
        "{\n"
        "    return pos;\n"
        "}",

        "float4 test(float2 pos : TEXCOORD0) : SV_TARGET\n"
        "{\n"
        "    return pos;\n"
        "}",

        /* 10 */
        "float4 test(float2 pos: TEXCOORD0) : SV_TARGET\n"
        "{\n"
        "    float a[0];\n"
        "    return float4(0, 0, 0, 0);\n"
        "}",

        "float4 test(float2 pos: TEXCOORD0) : SV_TARGET\n"
        "{\n"
        "    float a[65537];\n"
        "    return float4(0, 0, 0, 0);\n"
        "}",

        "float4 test(float2 pos: TEXCOORD0) : SV_TARGET\n"
        "{\n"
        "    int x;\n"
        "    float a[(x = 2)];\n"
        "    return float4(0, 0, 0, 0);\n"
        "}",

        "uniform float4 test() : SV_TARGET\n"
        "{\n"
        "    return float4(0, 0, 0, 0);\n"
        "}",

        "typedef row_major float4x4 matrix_t;\n"
        "typedef column_major matrix_t matrix2_t;\n"
        "float4 test() : SV_TARGET\n"
        "{\n"
        "    return float4(0, 0, 0, 0);\n"
        "}",

        /* 15 */
        "float4 test()\n"
        "{\n"
        "    return float4(0, 0, 0, 0);\n"
        "}",

        "float4 test(out float4 o : SV_TARGET)\n"
        "{\n"
        "    o = float4(1, 1, 1, 1);\n"
        "    return float4(0, 0, 0, 0);\n"
        "}",

        "struct {float4 a;};\n"
        "float4 test() : SV_TARGET\n"
        "{\n"
        "    return float4(0, 0, 0, 0);\n"
        "}",
    };

    static const char *targets[] = {"ps_2_0", "ps_3_0", "ps_4_0"};

    ID3D10Blob *compiled, *errors;
    unsigned int i, j;
    HRESULT hr;

    for (j = 0; j < ARRAY_SIZE(targets); ++j)
    {
        for (i = 0; i < ARRAY_SIZE(tests); ++i)
        {
            errors = NULL;
            compiled = (void *)0xdeadbeef;
            hr = D3DCompile(tests[i], strlen(tests[i]), NULL, NULL, NULL, "test", targets[j], 0, 0, &compiled, &errors);
            ok(hr == E_FAIL, "Test %u, target %s: Got unexpected hr %#lx.\n", i, targets[j], hr);
            ok(!!errors, "Test %u, target %s, expected non-NULL error blob.\n", i, targets[j]);
            ID3D10Blob_Release(errors);
            ok(!compiled, "Test %u, target %s, expected no compiled shader blob.\n", i, targets[j]);
        }
    }
}

static HRESULT WINAPI test_d3dinclude_open(ID3DInclude *iface, D3D_INCLUDE_TYPE include_type,
        const char *filename, const void *parent_data, const void **data, UINT *bytes)
{
    static const char include1[] =
        "#define LIGHT 1\n";
    static const char include2[] =
        "#include \"include1.h\"\n"
        "float4 light_color = LIGHT;\n";
    static const char include3[] =
        "#include \"include1.h\"\n"
        "def c0, LIGHT, 0, 0, 0\n";
    char *buffer;

    trace("filename %s.\n", filename);
    trace("parent_data %p: %s.\n", parent_data, parent_data ? (char *)parent_data : "(null)");

    if (!strcmp(filename, "include1.h"))
    {
        buffer = strdup(include1);
        *bytes = strlen(include1);
        buffer[*bytes] = '$'; /* everything should still work without a null terminator */
        ok(include_type == D3D_INCLUDE_LOCAL, "Unexpected include type %d.\n", include_type);
        ok(!strncmp(include2, parent_data, strlen(include2)) || !strncmp(include3, parent_data, strlen(include3)),
                "Unexpected parent_data value.\n");
    }
    else if (!strcmp(filename, "include\\include2.h"))
    {
        buffer = strdup(include2);
        *bytes = strlen(include2);
        buffer[*bytes] = '$'; /* everything should still work without a null terminator */
        ok(!parent_data, "Unexpected parent_data value.\n");
        ok(include_type == D3D_INCLUDE_LOCAL, "Unexpected include type %d.\n", include_type);
    }
    else if (!strcmp(filename, "include\\include3.h"))
    {
        buffer = strdup(include3);
        *bytes = strlen(include3);
        buffer[*bytes] = '$'; /* everything should still work without a null terminator */
        ok(!parent_data, "Unexpected parent_data value.\n");
        ok(include_type == D3D_INCLUDE_LOCAL, "Unexpected include type %d.\n", include_type);
    }
    else
    {
        ok(0, "Unexpected #include for file %s.\n", filename);
        return D3DERR_INVALIDCALL;
    }

    *data = buffer;
    return S_OK;
}

static HRESULT WINAPI test_d3dinclude_close(ID3DInclude *iface, const void *data)
{
    free((void *)data);
    return S_OK;
}

static const struct ID3DIncludeVtbl test_d3dinclude_vtbl =
{
    test_d3dinclude_open,
    test_d3dinclude_close
};

struct test_d3dinclude
{
    ID3DInclude ID3DInclude_iface;
};

static HRESULT call_D3DAssemble(const char *source_name, ID3DInclude *include, ID3D10Blob **blob, ID3D10Blob **errors)
{
    static const char ps_code[] =
        "ps_2_0\n"
        "#include \"include\\include3.h\"\n"
        "mov oC0, c0";

    return pD3DAssemble(ps_code, sizeof(ps_code), source_name, NULL, include, 0, blob, errors);
}

static HRESULT call_D3DCompile(const char *source_name, ID3DInclude *include, ID3D10Blob **blob, ID3D10Blob **errors)
{
    static const char ps_code[] =
        "#include \"include\\include2.h\"\n"
        "\n"
        "float4 main() : COLOR\n"
        "{\n"
        "    return light_color;\n"
        "}";

    return D3DCompile(ps_code, sizeof(ps_code), source_name, NULL, include, "main", "ps_2_0", 0, 0, blob, errors);
}

#if D3D_COMPILER_VERSION >= 46
static HRESULT call_D3DCompile2(const char *source_name, ID3DInclude *include, ID3D10Blob **blob, ID3D10Blob **errors)
{
    static const char ps_code[] =
        "#include \"include\\include2.h\"\n"
        "\n"
        "float4 main() : COLOR\n"
        "{\n"
        "    return light_color;\n"
        "}";

    return D3DCompile2(ps_code, sizeof(ps_code), source_name, NULL, include,
            "main", "ps_2_0", 0, 0, 0, NULL, 0, blob, errors);
}
#endif

static HRESULT call_D3DPreprocess(const char *source_name, ID3DInclude *include, ID3D10Blob **blob, ID3D10Blob **errors)
{
    static const char ps_code[] =
        "#include \"include\\include2.h\"\n"
        "#if LIGHT != 1\n"
        "#error\n"
        "#endif";

    return D3DPreprocess(ps_code, sizeof(ps_code), source_name, NULL, include, blob, errors);
}

typedef HRESULT (*include_test_cb)(const char *source_name, ID3DInclude *include, ID3D10Blob **blob, ID3D10Blob **errors);

static void test_include(void)
{
    struct test_d3dinclude include = {{&test_d3dinclude_vtbl}};
    WCHAR filename[MAX_PATH], include_filename[MAX_PATH];
    ID3D10Blob *blob = NULL, *errors = NULL;
    CHAR filename_a[MAX_PATH];
    unsigned int i;
    HRESULT hr;
    DWORD len;
    static const char ps_code[] =
        "#include \"include\\include2.h\"\n"
        "\n"
        "float4 main() : COLOR\n"
        "{\n"
        "    return light_color;\n"
        "}";
    static const char include1[] =
        "#define LIGHT 1\n";
    static const char include1_wrong[] =
        "#define LIGHT nope\n";
    static const char include2[] =
        "#include \"include1.h\"\n"
        "float4 light_color = LIGHT;\n";
    static const char include3[] =
        "#include \"include1.h\"\n"
        "def c0, LIGHT, 0, 0, 0\n";

#if D3D_COMPILER_VERSION >= 46
    WCHAR directory[MAX_PATH];
    static const char ps_absolute_template[] =
        "#include \"%ls\"\n"
        "\n"
        "float4 main() : COLOR\n"
        "{\n"
        "    return light_color;\n"
        "}";
    char ps_absolute_buffer[sizeof(ps_absolute_template) + MAX_PATH];
#endif

    static const include_test_cb tests[] =
    {
        call_D3DAssemble,
        call_D3DPreprocess,
        call_D3DCompile,
#if D3D_COMPILER_VERSION >= 46
        call_D3DCompile2,
#endif
    };

    create_file(L"source.ps", ps_code, strlen(ps_code), filename);
    create_directory(L"include");
    create_file(L"include\\include1.h", include1_wrong, strlen(include1_wrong), NULL);
    create_file(L"include1.h", include1, strlen(include1), NULL);
    create_file(L"include\\include2.h", include2, strlen(include2), include_filename);
    create_file(L"include\\include3.h", include3, strlen(include3), NULL);

    len = WideCharToMultiByte(CP_ACP, 0, filename, -1, NULL, 0, NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, filename, -1, filename_a, len, NULL, NULL);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        winetest_push_context("Test %u", i);

        hr = tests[i](filename_a, &include.ID3DInclude_iface, &blob, &errors);
        todo_wine_if (i == 1)
        {
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
            ok(!!blob, "Got unexpected blob.\n");
        }
        todo_wine_if (i == 1)
            ok(!errors, "Got unexpected errors.\n");
        if (blob)
        {
            ID3D10Blob_Release(blob);
            blob = NULL;
        }

#if D3D_COMPILER_VERSION >= 46
        hr = tests[i](NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, &blob, &errors);
        todo_wine_if (!i)
            ok(hr == (i == 0 ? D3DXERR_INVALIDDATA : E_FAIL), "Got unexpected hr %#lx.\n", hr);
        ok(!blob, "Got unexpected blob.\n");
        ok(!!errors, "Got unexpected errors.\n");
        ID3D10Blob_Release(errors);
        errors = NULL;

        /* Windows always seems to resolve includes from the initial file location
         * instead of using the immediate parent, as it would be the case for
         * standard C preprocessor includes. */
        hr = tests[i](filename_a, D3D_COMPILE_STANDARD_FILE_INCLUDE, &blob, &errors);
        todo_wine_if (i == 1)
        {
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
            ok(!!blob, "Got unexpected blob.\n");
            ok(!errors, "Got unexpected errors.\n");
        }
        if (blob)
        {
            ID3D10Blob_Release(blob);
            blob = NULL;
        }
#endif /* D3D_COMPILER_VERSION >= 46 */

        winetest_pop_context();
    }

#if D3D_COMPILER_VERSION >= 46

    hr = D3DCompileFromFile(L"nonexistent", NULL, NULL, "main", "vs_2_0", 0, 0, &blob, &errors);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "Got unexpected hr %#lx.\n", hr);
    ok(!blob, "Got unexpected blob.\n");
    ok(!errors, "Got unexpected errors.\n");

    hr = D3DCompileFromFile(filename, NULL, NULL, "main", "ps_2_0", 0, 0, &blob, &errors);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(!blob, "Got unexpected blob.\n");
    ok(!!errors, "Got unexpected errors.\n");
    trace("%s.\n", (char *)ID3D10Blob_GetBufferPointer(errors));
    ID3D10Blob_Release(errors);
    errors = NULL;

    hr = D3DCompileFromFile(filename, NULL, &include.ID3DInclude_iface, "main", "ps_2_0", 0, 0, &blob, &errors);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!!blob, "Got unexpected blob.\n");
    ok(!errors, "Got unexpected errors.\n");
    ID3D10Blob_Release(blob);
    blob = NULL;

    /* Windows always seems to resolve includes from the initial file location
     * instead of using the immediate parent, as it would be the case for
     * standard C preprocessor includes. */
    hr = D3DCompileFromFile(filename, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_2_0", 0, 0, &blob, &errors);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!!blob, "Got unexpected blob.\n");
    ok(!errors, "Got unexpected errors.\n");
    ID3D10Blob_Release(blob);
    blob = NULL;

    sprintf(ps_absolute_buffer, ps_absolute_template, include_filename);
    hr = D3DCompile(ps_absolute_buffer, sizeof(ps_absolute_buffer), filename_a, NULL,
            D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_2_0", 0, 0, &blob, &errors);
    todo_wine ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    todo_wine ok(!!blob, "Got unexpected blob.\n");
    todo_wine ok(!errors, "Got unexpected errors.\n");
    if (blob)
    {
        ID3D10Blob_Release(blob);
        blob = NULL;
    }

    GetCurrentDirectoryW(MAX_PATH, directory);
    SetCurrentDirectoryW(temp_dir);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        winetest_push_context("Test %u", i);

        hr = tests[i](NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, &blob, &errors);
        todo_wine_if (i == 1)
        {
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
            ok(!!blob, "Got unexpected blob.\n");
            ok(!errors, "Got unexpected errors.\n");
        }
        if (blob)
        {
            ID3D10Blob_Release(blob);
            blob = NULL;
        }

        winetest_pop_context();
    }

    hr = D3DCompileFromFile(L"source.ps", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_2_0", 0, 0, &blob, &errors);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!!blob, "Got unexpected blob.\n");
    ok(!errors, "Got unexpected errors.\n");
    ID3D10Blob_Release(blob);
    blob = NULL;

    SetCurrentDirectoryW(directory);
#endif /* D3D_COMPILER_VERSION >= 46 */

    delete_file(L"source.ps");
    delete_file(L"include\\include1.h");
    delete_file(L"include1.h");
    delete_file(L"include\\include2.h");
    delete_directory(L"include");
}

static void test_no_output_blob(void)
{
    static const char vs_source[] =
        "float4 main(float4 pos : POSITION, inout float2 texcoord : TEXCOORD0) : POSITION\n"
        "{\n"
        "   return pos;\n"
        "}";
    ID3D10Blob *errors;
    HRESULT hr;

    errors = (void *)0xdeadbeef;
    hr = D3DCompile(vs_source, strlen(vs_source), NULL, NULL, NULL, "main", "vs_2_0", 0, 0, NULL, &errors);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!errors, "Unexpected errors blob.\n");
}

static void test_hlsl_double(void)
{
    static const char ps_hlsl[] =
            "float func(float x){return 0.1;}\n"
            "float func(half x){return 0.2;}\n"
            "float func(double x){return 0.3;}\n"
            "\n"
            "float4 main(uniform double u) : COLOR\n"
            "{\n"
            "    return func(u);\n"
            "}\n";
    ID3DBlob *ps_bytecode, *errors;
    struct test_context context;
    struct vec4 color;
    HRESULT hr;

    if (!init_test_context(&context))
        return;

    hr = D3DCompile(ps_hlsl, sizeof(ps_hlsl), NULL, NULL, NULL, "main", "ps_2_0", 0, 0, &ps_bytecode, &errors);
#if D3D_COMPILER_VERSION >= 46
    todo_wine ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
#else
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
#endif
    if (FAILED(hr))
    {
        trace("%s\n", (char *)ID3D10Blob_GetBufferPointer(errors));
        release_test_context(&context);
        return;
    }

    draw_quad(context.device, ps_bytecode);

    color = get_color_vec4(context.device, 320, 240);
    ok(compare_vec4(&color, 0.3, 0.3, 0.3, 0.3, 0), "Unexpected color {%.8e, %.8e, %.8e, %.8e}.\n",
            color.x, color.y, color.z, color.w);
    release_test_context(&context);
}

START_TEST(hlsl_d3d9)
{
    char buffer[20];
    HMODULE mod;

    if (!(mod = LoadLibraryA("d3dx9_36.dll")))
    {
        win_skip("Failed to load d3dx9_36.dll.\n");
        return;
    }
    pD3DXGetShaderConstantTable = (void *)GetProcAddress(mod, "D3DXGetShaderConstantTable");

    sprintf(buffer, "d3dcompiler_%d", D3D_COMPILER_VERSION);
    mod = GetModuleHandleA(buffer);
    pD3DAssemble = (void *)GetProcAddress(mod, "D3DAssemble");

    test_swizzle();
    test_math();
    test_conditionals();
    test_float_vectors();
    test_trig();
    test_comma();
    test_return();
    test_array_dimensions();
    test_majority();
    test_struct_assignment();
    test_struct_semantics();
    test_global_initializer();
    test_samplers();
    test_constant_table();
    test_fail();
    test_include();
    test_no_output_blob();
    test_hlsl_double();
}
