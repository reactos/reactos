/*
 * Copyright (C) 2010 Travis Athougies
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

static pD3DCompile ppD3DCompile;

struct vertex
{
    float x, y, z;
    float tx, ty;
};

/* Tells compute_shader_probe* which pixels should be what colors */
struct hlsl_probe_info
{
    unsigned int x, y;
    /* The expected values in this region */
    D3DXCOLOR c;
    /* The max error for any value */
    float epsilon;
    /* An error message to print if this test fails */
    const char *message;
};

static HWND create_window(void)
{
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = DefWindowProcA;
    wc.lpszClassName = "d3d9_test_wc";
    RegisterClassA(&wc);

    return CreateWindowA("d3d9_test_wc", "d3d9_test", 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

static IDirect3DDevice9 *init_d3d9(IDirect3DVertexDeclaration9 **vdeclaration,
        IDirect3DVertexBuffer9 **quad_geometry, IDirect3DVertexShader9 **vshader_passthru)
{
    static const struct vertex quad_vertices[4] =
    {
        {-1.0f, -1.0f, 0.0f, 0.0f, 1.0f},
        {-1.0f,  1.0f, 0.0f, 0.0f, 0.0f},
        { 1.0f, -1.0f, 0.0f, 1.0f, 1.0f},
        { 1.0f,  1.0f, 0.0f, 1.0f, 0.0f}
    };

    static const D3DVERTEXELEMENT9 vdeclelements[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };

    static const char *vshader_passthru_hlsl =
        "float4 vshader(float4 pos: POSITION, inout float2 texcoord: TEXCOORD0): POSITION\n"
        "{\n"
        "   return pos;\n"
        "}";

    IDirect3D9 *d3d9_ptr;
    IDirect3DDevice9 *device_ptr = NULL;
    D3DPRESENT_PARAMETERS present_parameters;

    void *temp_geometry_vertices;

    ID3D10Blob *compiled = NULL;
    ID3D10Blob *errors = NULL;

    HRESULT hr;

    d3d9_ptr = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d9_ptr)
    {
        skip("could not create D3D9\n");
        return NULL;
    }

    hr = IDirect3D9_CheckDeviceFormat(d3d9_ptr, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
            0, D3DRTYPE_SURFACE, D3DFMT_A32B32G32R32F);
    if (FAILED(hr))
    {
        skip("A32B32G32R32F format not available on this device\n");
        IDirect3D9_Release(d3d9_ptr);
        return NULL;
    }

    ZeroMemory(&present_parameters, sizeof(present_parameters));
    present_parameters.Windowed = TRUE;
    present_parameters.hDeviceWindow = create_window();
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;

    hr = IDirect3D9_CreateDevice(d3d9_ptr, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL,
            D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_parameters, &device_ptr);
    IDirect3D9_Release(d3d9_ptr);
    if (FAILED(hr))
    {
        skip("could not create Direct3D9 device\n");
        return NULL;
    }

    /* Create the quad geometry */
    hr = IDirect3DDevice9_CreateVertexBuffer(device_ptr, 4 * sizeof(struct vertex),
            D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, quad_geometry, NULL);
    ok(SUCCEEDED(hr),
            "Could not create vertex buffer, IDirect3DDevice9_CreateVertexBuffer returned: %08x\n", hr);

    hr = IDirect3DVertexBuffer9_Lock(*quad_geometry, 0, sizeof(quad_vertices), &temp_geometry_vertices, 0);
    ok(SUCCEEDED(hr), "IDirect3DVertexBuffer9_Lock returned: %08x\n", hr);
    memcpy(temp_geometry_vertices, quad_vertices, sizeof(quad_vertices));
    IDirect3DVertexBuffer9_Unlock(*quad_geometry);

    hr = IDirect3DDevice9_CreateVertexDeclaration(device_ptr, vdeclelements, vdeclaration);
    ok(SUCCEEDED(hr), "Could not create vertex declaration: "
            "IDirect3DDevice9_CreateVertexDeclaration returned: %08x\n", hr);

    hr = IDirect3DDevice9_SetVertexDeclaration(device_ptr, *vdeclaration);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration returned: %08x\n", hr);

    /* Create a simple vertex shader to just pass through the values */
    hr = ppD3DCompile(vshader_passthru_hlsl, strlen(vshader_passthru_hlsl), NULL,
            NULL, NULL, "vshader", "vs_1_1", 0, 0, &compiled, &errors);
    if (FAILED(hr))
    {
        skip("not compiling vertex shader due to lacking wine HLSL support!\n");
        if (errors)
            ID3D10Blob_Release(errors);
        return NULL;
    }

    hr = IDirect3DDevice9_CreateVertexShader(device_ptr, ID3D10Blob_GetBufferPointer(compiled),
            vshader_passthru);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_CreateVertexShader returned: %08x\n", hr);
    ID3D10Blob_Release(compiled);

    return device_ptr;
}

/* Convenience functions */
static void set_float4_d3d9(IDirect3DDevice9 *device, ID3DXConstantTable *constants, const char *name,
        float x, float y, float z, float w)
{
    D3DXVECTOR4 vector;
    vector.x = x;
    vector.y = y;
    vector.z = z;
    vector.w = w;
    ID3DXConstantTable_SetVector(constants, device, name, &vector);
}

/* Compile our pixel shader and get back the compiled version and a constant table */
static IDirect3DPixelShader9 *compile_pixel_shader9(IDirect3DDevice9 *device, const char *shader,
        const char *profile, ID3DXConstantTable **constants)
{
    ID3D10Blob *compiled = NULL;
    ID3D10Blob *errors = NULL;
    IDirect3DPixelShader9 *pshader;
    HRESULT hr;

    hr = ppD3DCompile(shader, strlen(shader), NULL, NULL,
            NULL, "test", profile, /* test is the name of the entry point of our shader */
            0, 0, &compiled, &errors);
    ok(hr == D3D_OK, "Pixel shader %s compilation failed: %s\n", shader,
            errors ? (char *)ID3D10Blob_GetBufferPointer(errors) : "");
    if (FAILED(hr)) return NULL;

    hr = D3DXGetShaderConstantTable(ID3D10Blob_GetBufferPointer(compiled), constants);
    ok(hr == D3D_OK, "Could not get constant table from compiled pixel shader\n");

    hr = IDirect3DDevice9_CreatePixelShader(device, ID3D10Blob_GetBufferPointer(compiled), &pshader);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_CreatePixelShader returned: %08x\n", hr);
    ID3D10Blob_Release(compiled);
    return pshader;
}

/* Draw a full screen quad */
static void draw_quad_with_shader9(IDirect3DDevice9 *device, IDirect3DVertexBuffer9 *quad_geometry)
{
    HRESULT hr;
    D3DXMATRIX projection_matrix;

    D3DXMatrixOrthoLH(&projection_matrix, 2.0f, 2.0f, 0.0f, 1.0f);
    IDirect3DDevice9_SetTransform(device, D3DTS_PROJECTION, &projection_matrix);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned: %08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned: %08x\n", hr);

    hr = IDirect3DDevice9_SetStreamSource(device, 0, quad_geometry, 0, sizeof(struct vertex));
    ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource returned: %08x\n", hr);
    hr = IDirect3DDevice9_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, 0, 2);
    ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitive returned: %08x\n", hr);

    hr = IDirect3DDevice9_EndScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned: %08x\n", hr);
}

static void setup_device9(IDirect3DDevice9 *device, IDirect3DSurface9 **render_target,
        IDirect3DSurface9 **readback, D3DFORMAT format, unsigned int width, unsigned int height,
        IDirect3DVertexShader9 *vshader, IDirect3DPixelShader9 *pshader)
{
    HRESULT hr;
    hr = IDirect3DDevice9_CreateRenderTarget(device, width, height, format,
            D3DMULTISAMPLE_NONE, 0, FALSE, render_target, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateRenderTarget returned: %08x\n", hr);

    /* The Direct3D 9 docs state that we cannot lock a render target surface,
       instead we must copy the render target onto this surface to lock it */
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, width, height, format,
            D3DPOOL_SYSTEMMEM, readback, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateOffscreenPlainSurface returned: %08x\n", hr);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, *render_target);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderTarget returned: %08x\n", hr);

    hr = IDirect3DDevice9_SetVertexShader(device, vshader);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned: %08x\n", hr);
    hr = IDirect3DDevice9_SetPixelShader(device, pshader);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned: %08x\n", hr);
}

static BOOL colors_match(D3DXCOLOR a, D3DXCOLOR b, float epsilon)
{
  return (fabs(a.r - b.r) < epsilon && fabs(a.g - b.g) < epsilon && fabs(a.b - b.b) < epsilon &&
          fabs(a.a - b.a) < epsilon);
}

/* Compute a shader on a width by height buffer and probes certain locations
   to see if they are as expected. */
static void compute_shader_probe9(IDirect3DDevice9 *device, IDirect3DVertexShader9 *vshader,
        IDirect3DPixelShader9 *pshader, IDirect3DVertexBuffer9 *quad_geometry,
        const struct hlsl_probe_info *probes, unsigned int count,
        unsigned int width, unsigned int height, unsigned int line_number)
{
    IDirect3DSurface9 *render_target;
    IDirect3DSurface9 *readback;

    HRESULT hr;
    D3DLOCKED_RECT lr;
    D3DXCOLOR *pbits_data;
    unsigned int i;

    setup_device9(device, &render_target, &readback, D3DFMT_A32B32G32R32F,
            width, height, vshader, pshader);

    /* Draw the quad with the shader and read back the data */
    draw_quad_with_shader9(device, quad_geometry);
    IDirect3DDevice9_GetRenderTargetData(device, render_target, readback);
    hr = IDirect3DSurface9_LockRect(readback, &lr, NULL, D3DLOCK_READONLY);
    ok(hr == D3D_OK, "IDirect3DSurface9_LockRect returned: %08x\n", hr);
    pbits_data = lr.pBits;

    /* Now go through the probes and check each one */
    for (i = 0; i < count; i++, probes++) {
        int index = probes->x + (probes->y * lr.Pitch / sizeof(D3DXCOLOR));
        ok(colors_match(probes->c, pbits_data[index], probes->epsilon),
                "Line %d: At (%d, %d): %s: Expected (%.04f,%.04f,%.04f, %.04f), got "
                "(%.04f,%.04f,%.04f,%.04f)\n", line_number, probes->x, probes->y, probes->message,
                probes->c.r, probes->c.g, probes->c.b, probes->c.a, pbits_data[index].r,
                pbits_data[index].g, pbits_data[index].b, pbits_data[index].a);
    }

    hr = IDirect3DSurface9_UnlockRect(readback);
    ok(hr == D3D_OK, "IDirect3DSurface9_UnlockRect returned: %08x\n", hr);

    /* We now present the scene. This is mostly for debugging purposes, since GetRenderTargetData
       also waits for drawing commands to complete. The reason this call is here and not in a
       draw function is because the contents of the render target surface are invalidated after
       this call. */
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present returned: %08x\n", hr);

    IDirect3DSurface9_Release(render_target);
    IDirect3DSurface9_Release(readback);
}

/* Now the actual test functions */
static void test_swizzle(IDirect3DDevice9 *device, IDirect3DVertexBuffer9 *quad_geometry,
        IDirect3DVertexShader9 *vshader_passthru)
{
    static const struct hlsl_probe_info probes[] =
    {
       {0, 0, {0.0101f, 0.0303f, 0.0202f, 0.0404f}, 0.0001f, "swizzle_test"}
    };

    static const char *swizzle_test_shader =
        "uniform float4 color;\n"
        "float4 test(): COLOR\n"
        "{\n"
        "    float4 ret = color;\n"
        "    ret.gb = ret.ra;\n"
        "    ret.ra = float2(0.0101, 0.0404);\n"
        "    return ret;\n"
        "}";

    ID3DXConstantTable *constants;
    IDirect3DPixelShader9 *pshader;

    pshader = compile_pixel_shader9(device, swizzle_test_shader, "ps_2_0", &constants);
    if (pshader != NULL)
    {
        set_float4_d3d9(device, constants, "color", 0.0303f, 0.0f, 0.0f, 0.0202f);

        compute_shader_probe9(device, vshader_passthru, pshader, quad_geometry,
                probes, ARRAY_SIZE(probes), 1, 1, __LINE__);

        ID3DXConstantTable_Release(constants);
        IDirect3DPixelShader9_Release(pshader);
    }
}

static void test_math(IDirect3DDevice9 *device, IDirect3DVertexBuffer9 *quad_geometry,
        IDirect3DVertexShader9 *vshader_passthru)
{
    /* Tests order of operations */
    static const float u = 2.5f, v = 0.3f, w = 0.2f, x = 0.7f, y = 0.1f, z = 1.5f;

    static const struct hlsl_probe_info probes[] =
    {
        {0, 0, {-12.4300f, 9.8333f, 1.6000f, 34.9999f}, 0.0001f,
                "order of operations test"}
    };

    static const char *order_of_operations_shader =
        "float4 test(uniform float u, uniform float v, uniform float w, uniform float x,\n"
        "            uniform float y, uniform float z): COLOR\n"
        "{\n"
        "    return float4(x * y - z / w + --u / -v,\n"
        "            z * x / y + w / -v,\n"
        "            u + v - w,\n"
        "            x / y / w);\n"
        "}";

    ID3DXConstantTable *constants;
    IDirect3DPixelShader9 *pshader;

    pshader = compile_pixel_shader9(device, order_of_operations_shader, "ps_2_0", &constants);
    if (pshader != NULL)
    {
        ID3DXConstantTable_SetFloat(constants, device, "$u", u);
        ID3DXConstantTable_SetFloat(constants, device, "$v", v);
        ID3DXConstantTable_SetFloat(constants, device, "$w", w);
        ID3DXConstantTable_SetFloat(constants, device, "$x", x);
        ID3DXConstantTable_SetFloat(constants, device, "$y", y);
        ID3DXConstantTable_SetFloat(constants, device, "$z", z);

        compute_shader_probe9(device, vshader_passthru, pshader, quad_geometry,
                probes, ARRAY_SIZE(probes), 1, 1, __LINE__);

        ID3DXConstantTable_Release(constants);
        IDirect3DPixelShader9_Release(pshader);
    }
}

static void test_conditionals(IDirect3DDevice9 *device, IDirect3DVertexBuffer9 *quad_geometry,
        IDirect3DVertexShader9 *vshader_passthru)
{
    static const struct hlsl_probe_info if_greater_probes[] =
    {
        { 0, 0, {0.9f, 0.8f, 0.7f, 0.6f}, 0.0001f, "if greater test"},
        { 5, 0, {0.9f, 0.8f, 0.7f, 0.6f}, 0.0001f, "if greater test"},
        {10, 0, {0.9f, 0.8f, 0.7f, 0.6f}, 0.0001f, "if greater test"},
        {15, 0, {0.9f, 0.8f, 0.7f, 0.6f}, 0.0001f, "if greater test"},
        {25, 0, {0.1f, 0.2f, 0.3f, 0.4f}, 0.0001f, "if greater test"},
        {30, 0, {0.1f, 0.2f, 0.3f, 0.4f}, 0.0001f, "if greater test"}
    };

    static const char *if_greater_shader =
        "float4 test(float2 pos: TEXCOORD0): COLOR\n"
        "{\n"
        "    if((pos.x * 32.0) > 20.0)\n"
        "        return float4(0.1, 0.2, 0.3, 0.4);\n"
        "    else\n"
        "        return float4(0.9, 0.8, 0.7, 0.6);\n"
        "}";

    static const struct hlsl_probe_info ternary_operator_probes[] =
    {
        {0, 0, {0.50f, 0.25f, 0.50f, 0.75f}, 0.00001f, "ternary operator test"},
        {1, 0, {0.50f, 0.25f, 0.50f, 0.75f}, 0.00001f, "ternary operator test"},
        {2, 0, {0.50f, 0.25f, 0.50f, 0.75f}, 0.00001f, "ternary operator test"},
        {3, 0, {0.50f, 0.25f, 0.50f, 0.75f}, 0.00001f, "ternary operator test"},
        {4, 0, {0.60f, 0.80f, 0.10f, 0.20f}, 0.00001f, "ternary operator test"},
        {5, 0, {0.60f, 0.80f, 0.10f, 0.20f}, 0.00001f, "ternary operator test"},
        {6, 0, {0.60f, 0.80f, 0.10f, 0.20f}, 0.00001f, "ternary operator test"},
        {7, 0, {0.60f, 0.80f, 0.10f, 0.20f}, 0.00001f, "ternary operator test"}
    };

    static const char *ternary_operator_shader =
        "float4 test(float2 pos: TEXCOORD0): COLOR\n"
        "{\n"
        "    return (pos.x < 0.5?float4(0.5, 0.25, 0.5, 0.75):float4(0.6, 0.8, 0.1, 0.2));\n"
        "}";

    ID3DXConstantTable *constants;
    IDirect3DPixelShader9 *pshader;

    pshader = compile_pixel_shader9(device, if_greater_shader, "ps_2_0", &constants);
    if (pshader != NULL)
    {
        compute_shader_probe9(device, vshader_passthru, pshader, quad_geometry, if_greater_probes,
                ARRAY_SIZE(if_greater_probes), 32, 1, __LINE__);

        ID3DXConstantTable_Release(constants);
        IDirect3DPixelShader9_Release(pshader);
    }

    pshader = compile_pixel_shader9(device, ternary_operator_shader, "ps_2_0", &constants);
    if (pshader != NULL)
    {
        compute_shader_probe9(device, vshader_passthru, pshader, quad_geometry, ternary_operator_probes,
                ARRAY_SIZE(ternary_operator_probes), 8, 1, __LINE__);

        ID3DXConstantTable_Release(constants);
        IDirect3DPixelShader9_Release(pshader);
    }
}

static void test_float_vectors(IDirect3DDevice9 *device, IDirect3DVertexBuffer9 *quad_geometry,
        IDirect3DVertexShader9 *vshader_passthru)
{
    static const struct hlsl_probe_info vec4_indexing_test1_probes[] =
    {
        {0, 0, {0.020f, 0.245f, 0.351f, 1.000f}, 0.0001f, "vec4 indexing test 1"}
    };

    static const char *vec4_indexing_test1_shader =
        "float4 test(): COLOR\n"
        "{\n"
        "    float4 color;\n"
        "    color[0] = 0.020;\n"
        "    color[1] = 0.245;\n"
        "    color[2] = 0.351;\n"
        "    color[3] = 1.0;\n"
        "    return color;\n"
        "}";

    static const struct hlsl_probe_info vec4_indexing_test2_probes[] =
    {
        {0, 0, {0.5f, 0.3f, 0.8f, 0.2f}, 0.0001f, "vec4 indexing test 2"}
    };

    /* We have this uniform i here so the compiler can't optimize */
    static const char *vec4_indexing_test2_shader =
        "uniform int i;\n"
        "float4 test(): COLOR\n"
        "{\n"
        "    float4 color = float4(0.5, 0.4, 0.3, 0.2);\n"
        "    color.g = color[i];\n"
        "    color.b = 0.8;\n"
        "    return color;\n"
        "}";

    ID3DXConstantTable *constants;
    IDirect3DPixelShader9 *pshader;

    pshader = compile_pixel_shader9(device, vec4_indexing_test1_shader, "ps_2_0", &constants);
    if (pshader != NULL)
    {
        compute_shader_probe9(device, vshader_passthru, pshader, quad_geometry, vec4_indexing_test1_probes,
                ARRAY_SIZE(vec4_indexing_test1_probes), 1, 1, __LINE__);

        ID3DXConstantTable_Release(constants);
        IDirect3DPixelShader9_Release(pshader);
    }

    pshader = compile_pixel_shader9(device, vec4_indexing_test2_shader, "ps_2_0", &constants);
    if (pshader != NULL)
    {
        ID3DXConstantTable_SetInt(constants, device, "i", 2);

        compute_shader_probe9(device, vshader_passthru, pshader, quad_geometry, vec4_indexing_test2_probes,
                ARRAY_SIZE(vec4_indexing_test2_probes), 32, 1, __LINE__);

        ID3DXConstantTable_Release(constants);
        IDirect3DPixelShader9_Release(pshader);
    }
}

static void test_trig(IDirect3DDevice9 *device, IDirect3DVertexBuffer9 *quad_geometry,
        IDirect3DVertexShader9 *vshader_passthru)
{
    static const struct hlsl_probe_info sincos_probes[] =
    {
        {0, 0, {0.5000f, 1.0000f, 0.0f, 0.0f}, 0.001f, "sin/cos test"},
        {1, 0, {0.5975f, 0.9904f, 0.0f, 0.0f}, 0.001f, "sin/cos test"},
        {2, 0, {0.6913f, 0.9620f, 0.0f, 0.0f}, 0.001f, "sin/cos test"},
        {3, 0, {0.7778f, 0.9160f, 0.0f, 0.0f}, 0.001f, "sin/cos test"},
        {4, 0, {0.8536f, 0.8536f, 0.0f, 0.0f}, 0.001f, "sin/cos test"},
        {5, 0, {0.9157f, 0.7778f, 0.0f, 0.0f}, 0.001f, "sin/cos test"},
        {6, 0, {0.9620f, 0.6913f, 0.0f, 0.0f}, 0.001f, "sin/cos test"},
        {7, 0, {0.9904f, 0.5975f, 0.0f, 0.0f}, 0.001f, "sin/cos test"},
        {8, 0, {1.0000f, 0.5000f, 0.0f, 0.0f}, 0.001f, "sin/cos test"},
        {9, 0, {0.9904f, 0.4025f, 0.0f, 0.0f}, 0.001f, "sin/cos test"},
        {10, 0, {0.9619f, 0.3087f, 0.0f, 0.0f}, 0.001f, "sin/cos test"},
        {11, 0, {0.9157f, 0.2222f, 0.0f, 0.0f}, 0.001f, "sin/cos test"},
        {12, 0, {0.8536f, 0.1464f, 0.0f, 0.0f}, 0.001f, "sin/cos test"},
        {13, 0, {0.7778f, 0.0843f, 0.0f, 0.0f}, 0.001f, "sin/cos test"},
        {14, 0, {0.6913f, 0.0381f, 0.0f, 0.0f}, 0.001f, "sin/cos test"},
        {15, 0, {0.5975f, 0.0096f, 0.0f, 0.0f}, 0.001f, "sin/cos test"},
        {16, 0, {0.5000f, 0.0000f, 0.0f, 0.0f}, 0.001f, "sin/cos test"},
        {17, 0, {0.4025f, 0.0096f, 0.0f, 0.0f}, 0.001f, "sin/cos test"},
        {18, 0, {0.3087f, 0.0381f, 0.0f, 0.0f}, 0.001f, "sin/cos test"},
        {19, 0, {0.2222f, 0.0843f, 0.0f, 0.0f}, 0.001f, "sin/cos test"},
        {20, 0, {0.1464f, 0.1464f, 0.0f, 0.0f}, 0.001f, "sin/cos test"},
        {21, 0, {0.0843f, 0.2222f, 0.0f, 0.0f}, 0.001f, "sin/cos test"},
        {22, 0, {0.0381f, 0.3087f, 0.0f, 0.0f}, 0.001f, "sin/cos test"},
        {23, 0, {0.0096f, 0.4025f, 0.0f, 0.0f}, 0.001f, "sin/cos test"},
        {24, 0, {0.0000f, 0.5000f, 0.0f, 0.0f}, 0.001f, "sin/cos test"},
        {25, 0, {0.0096f, 0.5975f, 0.0f, 0.0f}, 0.001f, "sin/cos test"},
        {26, 0, {0.0381f, 0.6913f, 0.0f, 0.0f}, 0.001f, "sin/cos test"},
        {27, 0, {0.0843f, 0.7778f, 0.0f, 0.0f}, 0.001f, "sin/cos test"},
        {28, 0, {0.1464f, 0.8536f, 0.0f, 0.0f}, 0.001f, "sin/cos test"},
        {29, 0, {0.2222f, 0.9157f, 0.0f, 0.0f}, 0.001f, "sin/cos test"},
        {30, 0, {0.3087f, 0.9619f, 0.0f, 0.0f}, 0.001f, "sin/cos test"},
        {31, 0, {0.4025f, 0.9904f, 0.0f, 0.0f}, 0.001f, "sin/cos test"},
    };

    static const char *sincos_shader =
        "float4 test(float x: TEXCOORD0): COLOR\n"
        "{\n"
        "    const float pi2 = 6.2831853;\n"
        "    float calcd_sin = (sin(x * pi2) + 1)/2;\n"
        "    float calcd_cos = (cos(x * pi2) + 1)/2;\n"
        "    return float4(calcd_sin, calcd_cos, 0, 0);\n"
        "}";

    ID3DXConstantTable *constants;
    IDirect3DPixelShader9 *pshader;

    pshader = compile_pixel_shader9(device, sincos_shader, "ps_2_0", &constants);
    if (pshader != NULL)
    {
        compute_shader_probe9(device, vshader_passthru, pshader, quad_geometry, sincos_probes,
                ARRAY_SIZE(sincos_probes), 32, 1, __LINE__);

        ID3DXConstantTable_Release(constants);
        IDirect3DPixelShader9_Release(pshader);
    }
}

static void test_fail(IDirect3DDevice9 *device, IDirect3DVertexBuffer9 *qquad_geometry,
        IDirect3DVertexShader9 *vshader_passthru)
{
    static const char *tests[] =
    {
        "float4 test(float2 pos: TEXCOORD0) : COLOR\n"
        "{\n"
        "   return y;\n"
        "}",

        "float4 test(float2 pos: TEXCOORD0) : COLOR\n"
        "{\n"
        "  float4 x = float4(0, 0, 0, 0);\n"
        "  x.xzzx = float4(1, 2, 3, 4);\n"
        "  return x;\n"
        "}",

        "float4 test(float2 pos: TEXCOORD0) : COLOR\n"
        "{\n"
        "  float4 x = pos;\n"
        "  return x;\n"
        "}",

        "float4 test(float2 pos, TEXCOORD0) ; COLOR\n"
        "{\n"
        "  pos = float4 x;\n"
        "  mul(float4(5, 4, 3, 2), mvp) = x;\n"
        "  return float4;\n"
        "}",

        "float4 563r(float2 45s: TEXCOORD0) : COLOR\n"
        "{\n"
        "  float2 x = 45s;\n"
        "  return float4(x.x, x.y, 0, 0);\n"
        "}",

        "float4 test(float2 pos: TEXCOORD0) : COLOR\n"
        "{\n"
        "   struct { int b,c; } x = {0};\n"
        "   return y;\n"
        "}",

        "float4 test(float2 pos: TEXCOORD0) : COLOR\n"
        "{\n"
        "   struct {} x = {};\n"
        "   return y;\n"
        "}",
    };

    ID3D10Blob *compiled, *errors;
    unsigned int i;
    HRESULT hr;

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        compiled = errors = NULL;
        hr = ppD3DCompile(tests[i], strlen(tests[i]), NULL, NULL, NULL, "test", "ps_2_0", 0, 0, &compiled, &errors);
        ok(hr == E_FAIL, "Test %u, got unexpected hr %#x.\n", i, hr);
        ok(!!errors, "Test %u, expected non-NULL error blob.\n", i);
        ok(!compiled, "Test %u, expected no compiled shader blob.\n", i);
        ID3D10Blob_Release(errors);
    }
}

static BOOL load_d3dcompiler(void)
{
    HMODULE module;

    if (!(module = LoadLibraryA("d3dcompiler_43.dll"))) return FALSE;

    ppD3DCompile = (void*)GetProcAddress(module, "D3DCompile");
    return TRUE;
}

START_TEST(hlsl)
{
    D3DCAPS9 caps;
    ULONG refcount;
    IDirect3DDevice9 *device;
    IDirect3DVertexDeclaration9 *vdeclaration;
    IDirect3DVertexBuffer9 *quad_geometry;
    IDirect3DVertexShader9 *vshader_passthru;

    if (!load_d3dcompiler())
    {
        win_skip("Could not load d3dcompiler_43.dll\n");
        return;
    }

    device = init_d3d9(&vdeclaration, &quad_geometry, &vshader_passthru);
    if (!device) return;

    /* Make sure we support pixel shaders, before trying to compile them! */
    /* Direct3D 9 (Shader model 1-3 tests) */
    IDirect3DDevice9_GetDeviceCaps(device, &caps);
    if (caps.PixelShaderVersion >= D3DPS_VERSION(2, 0))
    {
        todo_wine
        {
            test_swizzle(device, quad_geometry, vshader_passthru);
            test_math(device, quad_geometry, vshader_passthru);
            test_conditionals(device, quad_geometry, vshader_passthru);
            test_float_vectors(device, quad_geometry, vshader_passthru);
            test_trig(device, quad_geometry, vshader_passthru);
            test_fail(device, quad_geometry, vshader_passthru);
        }
    } else skip("no pixel shader support\n");

    /* Reference counting sanity checks */
    if (vshader_passthru)
    {
        refcount = IDirect3DVertexShader9_Release(vshader_passthru);
        ok(!refcount, "Pass-through vertex shader has %u references left\n", refcount);
    }

    refcount = IDirect3DVertexBuffer9_Release(quad_geometry);
    ok(!refcount, "Vertex buffer has %u references left\n", refcount);

    refcount = IDirect3DVertexDeclaration9_Release(vdeclaration);
    ok(!refcount, "Vertex declaration has %u references left\n", refcount);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left\n", refcount);
}
