/*
 * Copyright 2010, 2012 Christian Costa
 * Copyright 2012 André Hentschel
 * Copyright 2011-2014 Henri Verbeet for CodeWeavers
 * Copyright 2014-2015 Aaryaman Vasishta
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

#include <limits.h>

#define COBJMACROS
#define _USE_MATH_DEFINES
#include <d3d.h>
#include <initguid.h>
#include <d3drm.h>
#include <d3drmwin.h>
#include <math.h>

#include "wine/test.h"

#define CHECK_REFCOUNT(obj,rc) \
    { \
        int rc_new = rc; \
        int count = get_refcount( (IUnknown *)obj ); \
        ok(count == rc_new, "Invalid refcount. Expected %d got %d\n", rc_new, count); \
    }

static ULONG get_refcount(IUnknown *object)
{
    IUnknown_AddRef( object );
    return IUnknown_Release( object );
}

static BOOL compare_float(float f, float g, unsigned int ulps)
{
    int x = *(int *)&f;
    int y = *(int *)&g;

    if (x < 0)
        x = INT_MIN - x;
    if (y < 0)
        y = INT_MIN - y;

    if (abs(x - y) > ulps)
        return FALSE;

    return TRUE;
}

#define expect_matrix(m, m11, m12, m13, m14, m21, m22, m23, m24, m31, m32, m33, m34, m41, m42, m43, m44, u) \
        expect_matrix_(__LINE__, m, m11, m12, m13, m14, m21, m22, m23, m24, m31, m32, m33, m34, m41, m42, m43, m44, u)
static void expect_matrix_(unsigned int line, D3DRMMATRIX4D m,
        float m11, float m12, float m13, float m14, float m21, float m22, float m23, float m24,
        float m31, float m32, float m33, float m34, float m41, float m42, float m43, float m44,
        unsigned int ulps)
{
    BOOL equal = compare_float(m[0][0], m11, ulps) && compare_float(m[0][1], m12, ulps)
            && compare_float(m[0][2], m13, ulps) && compare_float(m[0][3], m14, ulps)
            && compare_float(m[1][0], m21, ulps) && compare_float(m[1][1], m22, ulps)
            && compare_float(m[1][2], m23, ulps) && compare_float(m[1][3], m24, ulps)
            && compare_float(m[2][0], m31, ulps) && compare_float(m[2][1], m32, ulps)
            && compare_float(m[2][2], m33, ulps) && compare_float(m[2][3], m34, ulps)
            && compare_float(m[3][0], m41, ulps) && compare_float(m[3][1], m42, ulps)
            && compare_float(m[3][2], m43, ulps) && compare_float(m[3][3], m44, ulps);

    ok_(__FILE__, line)(equal,
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e, %.8e, %.8e, "
            "%.8e, %.8e, %.8e, %.8e, %.8e, %.8e, %.8e, %.8e}, "
            "expected {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e, %.8e, %.8e, "
            "%.8e, %.8e, %.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            m[0][0], m[0][1], m[0][2], m[0][3], m[1][0], m[1][1], m[1][2], m[1][3],
            m[2][0], m[2][1], m[2][2], m[2][3], m[3][0], m[3][1], m[3][2], m[3][3],
            m11, m12, m13, m14, m21, m22, m23, m24, m31, m32, m33, m34, m41, m42, m43, m44);
}

#define expect_vector(v, x, y, z, u) expect_vector_(__LINE__, v, x, y, z, u)
static void expect_vector_(unsigned int line, const D3DVECTOR *v, float x, float y, float z, unsigned int ulps)
{
    BOOL equal = compare_float(U1(*v).x, x, ulps)
            && compare_float(U2(*v).y, y, ulps)
            && compare_float(U3(*v).z, z, ulps);

    ok_(__FILE__, line)(equal, "Got unexpected vector {%.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e}.\n",
            U1(*v).x, U2(*v).y, U3(*v).z, x, y, z);
}

#define vector_eq(a, b) vector_eq_(__LINE__, a, b)
static void vector_eq_(unsigned int line, const D3DVECTOR *left, const D3DVECTOR *right)
{
    expect_vector_(line, left, U1(*right).x, U2(*right).y, U3(*right).z, 0);
}

static D3DRMMATRIX4D identity = {
    { 1.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f }
};

static void frame_set_transform(IDirect3DRMFrame *frame,
        float m11, float m12, float m13, float m14, float m21, float m22, float m23, float m24,
        float m31, float m32, float m33, float m34, float m41, float m42, float m43, float m44)
{
    D3DRMMATRIX4D matrix =
    {
        {m11, m12, m13, m14},
        {m21, m22, m23, m24},
        {m31, m32, m33, m34},
        {m41, m42, m43, m44},
    };

    IDirect3DRMFrame_AddTransform(frame, D3DRMCOMBINE_REPLACE, matrix);
}

static void set_vector(D3DVECTOR *v, float x, float y, float z)
{
    U1(*v).x = x;
    U2(*v).y = y;
    U3(*v).z = z;
}

static void matrix_sanitise(D3DRMMATRIX4D m)
{
    unsigned int i, j;

    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            if (m[i][j] > -1e-7f && m[i][j] < 1e-7f)
                m[i][j] = 0.0f;
        }
    }
}

static HWND create_window(void)
{
    RECT r = {0, 0, 640, 480};

    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW | WS_VISIBLE, FALSE);

    return CreateWindowA("static", "d3drm_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top, NULL, NULL, NULL, NULL);
}

#define test_class_name(a, b) test_class_name_(__LINE__, a, b)
static void test_class_name_(unsigned int line, IDirect3DRMObject *object, const char *name)
{
    char cname[64] = {0};
    DWORD size, size2;
    HRESULT hr;

    hr = IDirect3DRMObject_GetClassName(object, NULL, cname);
    ok_(__FILE__, line)(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMViewport_GetClassName(object, NULL, NULL);
    ok_(__FILE__, line)(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    size = 0;
    hr = IDirect3DRMObject_GetClassName(object, &size, NULL);
    ok_(__FILE__, line)(hr == D3DRM_OK, "Failed to get classname size, hr %#x.\n", hr);
    ok_(__FILE__, line)(size == strlen(name) + 1, "wrong size: %u\n", size);

    size = size2 = !!*name;
    hr = IDirect3DRMObject_GetClassName(object, &size, cname);
    ok_(__FILE__, line)(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    ok_(__FILE__, line)(size == size2, "Got size %u.\n", size);

    size = sizeof(cname);
    hr = IDirect3DRMObject_GetClassName(object, &size, cname);
    ok_(__FILE__, line)(hr == D3DRM_OK, "Failed to get classname, hr %#x.\n", hr);
    ok_(__FILE__, line)(size == strlen(name) + 1, "wrong size: %u\n", size);
    ok_(__FILE__, line)(!strcmp(cname, name), "Expected cname to be \"%s\", but got \"%s\".\n", name, cname);

    size = strlen(name) + 1;
    hr = IDirect3DRMObject_GetClassName(object, &size, cname);
    ok_(__FILE__, line)(hr == D3DRM_OK, "Failed to get classname, hr %#x.\n", hr);
    ok_(__FILE__, line)(size == strlen(name) + 1, "wrong size: %u\n", size);
    ok_(__FILE__, line)(!strcmp(cname, name), "Expected cname to be \"%s\", but got \"%s\".\n", name, cname);

    size = strlen(name);
    strcpy(cname, "XXX");
    hr = IDirect3DRMObject_GetClassName(object, &size, cname);
    ok_(__FILE__, line)(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    ok_(__FILE__, line)(size == strlen(name), "Wrong classname size: %u.\n", size);
    ok_(__FILE__, line)(!strcmp(cname, "XXX"), "Expected unchanged buffer, but got \"%s\".\n", cname);
}

#define test_object_name(a) test_object_name_(__LINE__, a)
static void test_object_name_(unsigned int line, IDirect3DRMObject *object)
{
    char name[64] = {0};
    HRESULT hr;
    DWORD size;

    hr = IDirect3DRMObject_GetName(object, NULL, NULL);
    ok_(__FILE__, line)(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    name[0] = 0x1f;
    hr = IDirect3DRMObject_GetName(object, NULL, name);
    ok_(__FILE__, line)(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    ok_(__FILE__, line)(name[0] == 0x1f, "Unexpected buffer contents, %#x.\n", name[0]);

    /* Name is not set yet. */
    size = 100;
    hr = IDirect3DRMObject_GetName(object, &size, NULL);
    ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to get name size, hr %#x.\n", hr);
    ok_(__FILE__, line)(size == 0, "Unexpected size %u.\n", size);

    size = sizeof(name);
    name[0] = 0x1f;
    hr = IDirect3DRMObject_GetName(object, &size, name);
    ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to get name size, hr %#x.\n", hr);
    ok_(__FILE__, line)(size == 0, "Unexpected size %u.\n", size);
    ok_(__FILE__, line)(name[0] == 0, "Unexpected name \"%s\".\n", name);

    size = 0;
    name[0] = 0x1f;
    hr = IDirect3DRMObject_GetName(object, &size, name);
    ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to get name size, hr %#x.\n", hr);
    ok_(__FILE__, line)(size == 0, "Unexpected size %u.\n", size);
    ok_(__FILE__, line)(name[0] == 0x1f, "Unexpected name \"%s\".\n", name);

    hr = IDirect3DRMObject_SetName(object, NULL);
    ok_(__FILE__, line)(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DRMObject_SetName(object, "name");
    ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to set a name, hr %#x.\n", hr);

    size = 0;
    hr = IDirect3DRMObject_GetName(object, &size, NULL);
    ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to get name size, hr %#x.\n", hr);
    ok_(__FILE__, line)(size == strlen("name") + 1, "Unexpected size %u.\n", size);

    size = strlen("name") + 1;
    hr = IDirect3DRMObject_GetName(object, &size, name);
    ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to get name size, hr %#x.\n", hr);
    ok_(__FILE__, line)(size == strlen("name") + 1, "Unexpected size %u.\n", size);
    ok_(__FILE__, line)(!strcmp(name, "name"), "Unexpected name \"%s\".\n", name);

    size = 2;
    name[0] = 0x1f;
    hr = IDirect3DRMObject_GetName(object, &size, name);
    ok_(__FILE__, line)(hr == E_INVALIDARG, "Failed to get object name, hr %#x.\n", hr);
    ok_(__FILE__, line)(size == 2, "Unexpected size %u.\n", size);
    ok_(__FILE__, line)(name[0] == 0x1f, "Got unexpected name \"%s\".\n", name);

    hr = IDirect3DRMObject_SetName(object, NULL);
    ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to set object name, hr %#x.\n", hr);

    size = 1;
    hr = IDirect3DRMObject_GetName(object, &size, NULL);
    ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to get name size, hr %#x.\n", hr);
    ok_(__FILE__, line)(size == 0, "Unexpected size %u.\n", size);

    size = 1;
    name[0] = 0x1f;
    hr = IDirect3DRMObject_GetName(object, &size, name);
    ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to get name size, hr %#x.\n", hr);
    ok_(__FILE__, line)(size == 0, "Unexpected size %u.\n", size);
    ok_(__FILE__, line)(name[0] == 0, "Got unexpected name \"%s\".\n", name);
}

static char data_bad_version[] =
"xof 0302txt 0064\n"
"Header Object\n"
"{\n"
"1; 2; 3;\n"
"}\n";

static char data_no_mesh[] =
"xof 0302txt 0064\n"
"Header Object\n"
"{\n"
"1; 0; 1;\n"
"}\n";

static char data_ok[] =
"xof 0302txt 0064\n"
"Header Object\n"
"{\n"
"1; 0; 1;\n"
"}\n"
"Mesh Object\n"
"{\n"
"4;\n"
"1.0; 0.0; 0.0;,\n"
"0.0; 1.0; 0.0;,\n"
"0.0; 0.0; 1.0;,\n"
"1.0; 1.0; 1.0;;\n"
"3;\n"
"3; 0, 1, 2;,\n"
"3; 1, 2, 3;,\n"
"3; 3, 1, 2;;\n"
"}\n";

static char data_full[] =
"xof 0302txt 0064\n"
"Header { 1; 0; 1; }\n"
"Mesh {\n"
" 3;\n"
" 0.1; 0.2; 0.3;,\n"
" 0.4; 0.5; 0.6;,\n"
" 0.7; 0.8; 0.9;;\n"
" 1;\n"
" 3; 0, 1, 2;;\n"
" MeshMaterialList {\n"
"  1; 1; 0;\n"
"  Material {\n"
"   0.0; 1.0; 0.0; 1.0;;\n"
"   30.0;\n"
"   1.0; 0.0; 0.0;;\n"
"   0.5; 0.5; 0.5;;\n"
"   TextureFileName {\n"
"    \"Texture.bmp\";\n"
"   }\n"
"  }\n"
" }\n"
" MeshNormals {\n"
"  3;\n"
"  1.1; 1.2; 1.3;,\n"
"  1.4; 1.5; 1.6;,\n"
"  1.7; 1.8; 1.9;;\n"
"  1;"
"  3; 0, 1, 2;;\n"
" }\n"
" MeshTextureCoords {\n"
"  3;\n"
"  0.13; 0.17;,\n"
"  0.23; 0.27;,\n"
"  0.33; 0.37;;\n"
" }\n"
"}\n";

static char data_d3drm_load[] =
"xof 0302txt 0064\n"
"Header Object\n"
"{\n"
"1; 0; 1;\n"
"}\n"
"Mesh Object1\n"
"{\n"
" 1;\n"
" 0.1; 0.2; 0.3;,\n"
" 1;\n"
" 3; 0, 1, 2;;\n"
"}\n"
"Mesh Object2\n"
"{\n"
" 1;\n"
" 0.1; 0.2; 0.3;,\n"
" 1;\n"
" 3; 0, 1, 2;;\n"
"}\n"
"Frame Scene\n"
"{\n"
" {Object1}\n"
" {Object2}\n"
"}\n"
"Material\n"
"{\n"
" 0.1, 0.2, 0.3, 0.4;;\n"
" 0.5;\n"
" 0.6, 0.7, 0.8;;\n"
" 0.9, 1.0, 1.1;;\n"
"}\n";

static char data_frame_mesh_materials[] =
"xof 0302txt 0064\n"
"Header { 1; 0; 1; }\n"
"Frame {\n"
" Mesh mesh1 {\n"
"  5;\n"
"  0.1; 0.2; 0.3;,\n"
"  0.4; 0.5; 0.6;,\n"
"  0.7; 0.8; 0.9;,\n"
"  1.1; 1.2; 1.3;,\n"
"  1.4; 1.5; 1.6;;\n"
"  6;\n"
"  3; 0, 1, 2;,\n"
"  3; 0, 2, 1;,\n"
"  3; 1, 2, 3;,\n"
"  3; 1, 3, 2;,\n"
"  3; 2, 3, 4;,\n"
"  3; 2, 4, 3;;\n"
"  MeshMaterialList {\n"
"   3; 6; 0, 1, 1, 2, 2, 2;\n"
"   Material mat1 {\n"
"    1.0; 0.0; 0.0; 0.1;;\n"
"    10.0;\n"
"    0.11; 0.12; 0.13;;\n"
"    0.14; 0.15; 0.16;;\n"
"   }\n"
"   Material mat2 {\n"
"    0.0; 1.0; 0.0; 0.2;;\n"
"    20.0;\n"
"    0.21; 0.22; 0.23;;\n"
"    0.24; 0.25; 0.26;;\n"
"   }\n"
"   Material mat3 {\n"
"    0.0; 0.0; 1.0; 0.3;;\n"
"    30.0;\n"
"    0.31; 0.32; 0.33;;\n"
"    0.34; 0.35; 0.36;;\n"
"   }\n"
"  }\n"
" }\n"
"}\n";

static void test_MeshBuilder(void)
{
    HRESULT hr;
    IDirect3DRM *d3drm;
    IDirect3DRMMeshBuilder *pMeshBuilder;
    IDirect3DRMMeshBuilder3 *meshbuilder3;
    IDirect3DRMMesh *mesh;
    D3DRMLOADMEMORY info;
    int val;
    DWORD val1, val2, val3;
    D3DVALUE valu, valv;
    D3DVECTOR v[3];
    D3DVECTOR n[4];
    DWORD f[8];
    char name[10];
    DWORD size;
    D3DCOLOR color;
    IUnknown *unk;

    hr = Direct3DRMCreate(&d3drm);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);

    hr = IDirect3DRM_CreateMeshBuilder(d3drm, &pMeshBuilder);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMMeshBuilder interface (hr = %x)\n", hr);

    hr = IDirect3DRMMeshBuilder_QueryInterface(pMeshBuilder, &IID_IDirect3DRMObject, (void **)&unk);
    ok(SUCCEEDED(hr), "Failed to get IDirect3DRMObject, %#x.\n", hr);
    ok(unk == (IUnknown *)pMeshBuilder, "Unexpected interface pointer.\n");
    IUnknown_Release(unk);

    hr = IDirect3DRMMeshBuilder_QueryInterface(pMeshBuilder, &IID_IDirect3DRMVisual, (void **)&unk);
    ok(SUCCEEDED(hr), "Failed to get IDirect3DRMVisual, %#x.\n", hr);
    ok(unk == (IUnknown *)pMeshBuilder, "Unexpected interface pointer.\n");
    IUnknown_Release(unk);

    hr = IDirect3DRMMeshBuilder_QueryInterface(pMeshBuilder, &IID_IDirect3DRMMeshBuilder3, (void **)&meshbuilder3);
    ok(SUCCEEDED(hr), "Failed to get IDirect3DRMMeshBuilder3, %#x.\n", hr);

    hr = IDirect3DRMMeshBuilder3_QueryInterface(meshbuilder3, &IID_IDirect3DRMObject, (void **)&unk);
    ok(SUCCEEDED(hr), "Failed to get IDirect3DRMObject, %#x.\n", hr);
    ok(unk == (IUnknown *)pMeshBuilder, "Unexpected interface pointer.\n");
    IUnknown_Release(unk);

    hr = IDirect3DRMMeshBuilder3_QueryInterface(meshbuilder3, &IID_IDirect3DRMVisual, (void **)&unk);
    ok(SUCCEEDED(hr), "Failed to get IDirect3DRMVisual, %#x.\n", hr);
    ok(unk == (IUnknown *)pMeshBuilder, "Unexpected interface pointer.\n");
    IUnknown_Release(unk);

    IDirect3DRMMeshBuilder3_Release(meshbuilder3);

    test_class_name((IDirect3DRMObject *)pMeshBuilder, "Builder");
    test_object_name((IDirect3DRMObject *)pMeshBuilder);

    info.lpMemory = data_bad_version;
    info.dSize = strlen(data_bad_version);
    hr = IDirect3DRMMeshBuilder_Load(pMeshBuilder, &info, NULL, D3DRMLOAD_FROMMEMORY, NULL, NULL);
    ok(hr == D3DRMERR_BADFILE, "Should have returned D3DRMERR_BADFILE (hr = %x)\n", hr);

    info.lpMemory = data_no_mesh;
    info.dSize = strlen(data_no_mesh);
    hr = IDirect3DRMMeshBuilder_Load(pMeshBuilder, &info, NULL, D3DRMLOAD_FROMMEMORY, NULL, NULL);
    ok(hr == D3DRMERR_NOTFOUND, "Should have returned D3DRMERR_NOTFOUND (hr = %x)\n", hr);

    info.lpMemory = data_ok;
    info.dSize = strlen(data_ok);
    hr = IDirect3DRMMeshBuilder_Load(pMeshBuilder, &info, NULL, D3DRMLOAD_FROMMEMORY, NULL, NULL);
    ok(hr == D3DRM_OK, "Cannot load mesh data (hr = %x)\n", hr);

    size = sizeof(name);
    hr = IDirect3DRMMeshBuilder_GetName(pMeshBuilder, &size, name);
    ok(hr == D3DRM_OK, "IDirect3DRMMeshBuilder_GetName returned hr = %x\n", hr);
    ok(!strcmp(name, "Object"), "Retrieved name '%s' instead of 'Object'\n", name);
    size = strlen("Object"); /* No space for null character */
    hr = IDirect3DRMMeshBuilder_GetName(pMeshBuilder, &size, name);
    ok(hr == E_INVALIDARG, "IDirect3DRMMeshBuilder_GetName returned hr = %x\n", hr);
    hr = IDirect3DRMMeshBuilder_SetName(pMeshBuilder, NULL);
    ok(hr == D3DRM_OK, "IDirect3DRMMeshBuilder_SetName returned hr = %x\n", hr);
    size = sizeof(name);
    hr = IDirect3DRMMeshBuilder_GetName(pMeshBuilder, &size, name);
    ok(hr == D3DRM_OK, "IDirect3DRMMeshBuilder_GetName returned hr = %x\n", hr);
    ok(size == 0, "Size should be 0 instead of %u\n", size);
    hr = IDirect3DRMMeshBuilder_SetName(pMeshBuilder, "");
    ok(hr == D3DRM_OK, "IDirect3DRMMeshBuilder_SetName returned hr = %x\n", hr);
    size = sizeof(name);
    hr = IDirect3DRMMeshBuilder_GetName(pMeshBuilder, &size, name);
    ok(hr == D3DRM_OK, "IDirect3DRMMeshBuilder_GetName returned hr = %x\n", hr);
    ok(!strcmp(name, ""), "Retrieved name '%s' instead of ''\n", name);

    val = IDirect3DRMMeshBuilder_GetVertexCount(pMeshBuilder);
    ok(val == 4, "Wrong number of vertices %d (must be 4)\n", val);

    val = IDirect3DRMMeshBuilder_GetFaceCount(pMeshBuilder);
    ok(val == 3, "Wrong number of faces %d (must be 3)\n", val);

    hr = IDirect3DRMMeshBuilder_GetVertices(pMeshBuilder, &val1, NULL, &val2, NULL, &val3, NULL);
    ok(hr == D3DRM_OK, "Cannot get vertices information (hr = %x)\n", hr);
    ok(val1 == 4, "Wrong number of vertices %d (must be 4)\n", val1);
    ok(val2 == 4, "Wrong number of normals %d (must be 4)\n", val2);
    ok(val3 == 22, "Wrong number of face data bytes %d (must be 22)\n", val3);

    /* Check that Load method generated default normals */
    hr = IDirect3DRMMeshBuilder_GetVertices(pMeshBuilder, NULL, NULL, &val2, n, NULL, NULL);
    ok(hr == D3DRM_OK, "Cannot get vertices information (hr = %x)\n", hr);
    expect_vector(&n[0],  0.577350f, 0.577350f, 0.577350f, 32);
    expect_vector(&n[1], -0.229416f, 0.688247f, 0.688247f, 32);
    expect_vector(&n[2], -0.229416f, 0.688247f, 0.688247f, 32);
    expect_vector(&n[3], -0.577350f, 0.577350f, 0.577350f, 32);

    /* Check that Load method generated default texture coordinates (0.0f, 0.0f) for each vertex */
    valu = 1.23f;
    valv = 3.21f;
    hr = IDirect3DRMMeshBuilder_GetTextureCoordinates(pMeshBuilder, 0, &valu, &valv);
    ok(hr == D3DRM_OK, "Cannot get texture coordinates (hr = %x)\n", hr);
    ok(valu == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valu);
    ok(valv == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valv);
    valu = 1.23f;
    valv = 3.21f;
    hr = IDirect3DRMMeshBuilder_GetTextureCoordinates(pMeshBuilder, 1, &valu, &valv);
    ok(hr == D3DRM_OK, "Cannot get texture coordinates (hr = %x)\n", hr);
    ok(valu == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valu);
    ok(valv == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valv);
    valu = 1.23f;
    valv = 3.21f;
    hr = IDirect3DRMMeshBuilder_GetTextureCoordinates(pMeshBuilder, 2, &valu, &valv);
    ok(hr == D3DRM_OK, "Cannot get texture coordinates (hr = %x)\n", hr);
    ok(valu == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valu);
    ok(valv == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valv);
    valu = 1.23f;
    valv = 3.21f;
    hr = IDirect3DRMMeshBuilder_GetTextureCoordinates(pMeshBuilder, 3, &valu, &valv);
    ok(hr == D3DRM_OK, "Cannot get texture coordinates (hr = %x)\n", hr);
    ok(valu == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valu);
    ok(valv == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valv);
    hr = IDirect3DRMMeshBuilder_GetTextureCoordinates(pMeshBuilder, 4, &valu, &valv);
    ok(hr == D3DRMERR_BADVALUE, "Should fail and return D3DRM_BADVALUE (hr = %x)\n", hr);

    valu = 1.23f;
    valv = 3.21f;
    hr = IDirect3DRMMeshBuilder_SetTextureCoordinates(pMeshBuilder, 0, valu, valv);
    ok(hr == D3DRM_OK, "Cannot set texture coordinates (hr = %x)\n", hr);
    hr = IDirect3DRMMeshBuilder_SetTextureCoordinates(pMeshBuilder, 4, valu, valv);
    ok(hr == D3DRMERR_BADVALUE, "Should fail and return D3DRM_BADVALUE (hr = %x)\n", hr);

    valu = 0.0f;
    valv = 0.0f;
    hr = IDirect3DRMMeshBuilder_GetTextureCoordinates(pMeshBuilder, 0, &valu, &valv);
    ok(hr == D3DRM_OK, "Cannot get texture coordinates (hr = %x)\n", hr);
    ok(valu == 1.23f, "Wrong coordinate %f (must be 1.23)\n", valu);
    ok(valv == 3.21f, "Wrong coordinate %f (must be 3.21)\n", valv);

    IDirect3DRMMeshBuilder_Release(pMeshBuilder);

    hr = IDirect3DRM_CreateMeshBuilder(d3drm, &pMeshBuilder);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMMeshBuilder interface (hr = %x)\n", hr);

    /* No group in mesh when mesh builder is not loaded */
    hr = IDirect3DRMMeshBuilder_CreateMesh(pMeshBuilder, &mesh);
    ok(hr == D3DRM_OK, "CreateMesh failed returning hr = %x\n", hr);
    if (hr == D3DRM_OK)
    {
        DWORD nb_groups;

        nb_groups = IDirect3DRMMesh_GetGroupCount(mesh);
        ok(nb_groups == 0, "GetCroupCount returned %u\n", nb_groups);

        IDirect3DRMMesh_Release(mesh);
    }

    info.lpMemory = data_full;
    info.dSize = strlen(data_full);
    hr = IDirect3DRMMeshBuilder_Load(pMeshBuilder, &info, NULL, D3DRMLOAD_FROMMEMORY, NULL, NULL);
    ok(hr == D3DRM_OK, "Cannot load mesh data (hr = %x)\n", hr);

    val = IDirect3DRMMeshBuilder_GetVertexCount(pMeshBuilder);
    ok(val == 3, "Wrong number of vertices %d (must be 3)\n", val);

    val = IDirect3DRMMeshBuilder_GetFaceCount(pMeshBuilder);
    ok(val == 1, "Wrong number of faces %d (must be 1)\n", val);

    /* Check no buffer size and too small buffer size errors */
    val1 = 1; val2 = 3; val3 = 8;
    hr = IDirect3DRMMeshBuilder_GetVertices(pMeshBuilder, &val1, v, &val2, n, &val3, f);
    ok(hr == D3DRMERR_BADVALUE, "IDirect3DRMMeshBuilder_GetVertices returned %#x\n", hr);
    hr = IDirect3DRMMeshBuilder_GetVertices(pMeshBuilder, NULL, v, &val2, n, &val3, f);
    ok(hr == D3DRMERR_BADVALUE, "IDirect3DRMMeshBuilder_GetVertices returned %#x\n", hr);
    val1 = 3; val2 = 1; val3 = 8;
    hr = IDirect3DRMMeshBuilder_GetVertices(pMeshBuilder, &val1, v, &val2, n, &val3, f);
    ok(hr == D3DRMERR_BADVALUE, "IDirect3DRMMeshBuilder_GetVertices returned %#x\n", hr);
    hr = IDirect3DRMMeshBuilder_GetVertices(pMeshBuilder, &val1, v, NULL, n, &val3, f);
    ok(hr == D3DRMERR_BADVALUE, "IDirect3DRMMeshBuilder_GetVertices returned %#x\n", hr);
    val1 = 3; val2 = 3; val3 = 1;
    hr = IDirect3DRMMeshBuilder_GetVertices(pMeshBuilder, &val1, v, &val2, n, &val3, f);
    ok(hr == D3DRMERR_BADVALUE, "IDirect3DRMMeshBuilder_GetVertices returned %#x\n", hr);
    hr = IDirect3DRMMeshBuilder_GetVertices(pMeshBuilder, &val1, v, &val2, n, NULL, f);
    ok(hr == D3DRMERR_BADVALUE, "IDirect3DRMMeshBuilder_GetVertices returned %#x\n", hr);

    val1 = 3; val2 = 3; val3 = 8;
    hr = IDirect3DRMMeshBuilder_GetVertices(pMeshBuilder, &val1, v, &val2, n, &val3, f);
    ok(hr == D3DRM_OK, "Cannot get vertices information (hr = %x)\n", hr);
    ok(val1 == 3, "Wrong number of vertices %d (must be 3)\n", val1);
    ok(val2 == 3, "Wrong number of normals %d (must be 3)\n", val2);
    ok(val3 == 8, "Wrong number of face data bytes %d (must be 8)\n", val3);
    expect_vector(&v[0], 0.1f, 0.2f, 0.3f, 32);
    expect_vector(&v[1], 0.4f, 0.5f, 0.6f, 32);
    expect_vector(&v[2], 0.7f, 0.8f, 0.9f, 32);
    expect_vector(&n[0], 1.1f, 1.2f, 1.3f, 32);
    expect_vector(&n[1], 1.4f, 1.5f, 1.6f, 32);
    expect_vector(&n[2], 1.7f, 1.8f, 1.9f, 32);
    ok(f[0] == 3 , "Wrong component f[0] = %d (expected 3)\n", f[0]);
    ok(f[1] == 0 , "Wrong component f[1] = %d (expected 0)\n", f[1]);
    ok(f[2] == 0 , "Wrong component f[2] = %d (expected 0)\n", f[2]);
    ok(f[3] == 1 , "Wrong component f[3] = %d (expected 1)\n", f[3]);
    ok(f[4] == 1 , "Wrong component f[4] = %d (expected 1)\n", f[4]);
    ok(f[5] == 2 , "Wrong component f[5] = %d (expected 2)\n", f[5]);
    ok(f[6] == 2 , "Wrong component f[6] = %d (expected 2)\n", f[6]);
    ok(f[7] == 0 , "Wrong component f[7] = %d (expected 0)\n", f[7]);

    hr = IDirect3DRMMeshBuilder_CreateMesh(pMeshBuilder, &mesh);
    ok(hr == D3DRM_OK, "CreateMesh failed returning hr = %x\n", hr);
    if (hr == D3DRM_OK)
    {
        DWORD nb_groups;
        unsigned nb_vertices, nb_faces, nb_face_vertices;
        DWORD data_size;
        IDirect3DRMMaterial *material = (IDirect3DRMMaterial *)0xdeadbeef;
        IDirect3DRMTexture *texture = (IDirect3DRMTexture *)0xdeadbeef;
        D3DVALUE values[3];

        nb_groups = IDirect3DRMMesh_GetGroupCount(mesh);
        ok(nb_groups == 1, "GetCroupCount returned %u\n", nb_groups);
        hr = IDirect3DRMMesh_GetGroup(mesh, 1, &nb_vertices, &nb_faces, &nb_face_vertices, &data_size, NULL);
        ok(hr == D3DRMERR_BADVALUE, "GetCroup returned hr = %x\n", hr);
        hr = IDirect3DRMMesh_GetGroup(mesh, 0, &nb_vertices, &nb_faces, &nb_face_vertices, &data_size, NULL);
        ok(hr == D3DRM_OK, "GetCroup failed returning hr = %x\n", hr);
        ok(nb_vertices == 3, "Wrong number of vertices %u (must be 3)\n", nb_vertices);
        ok(nb_faces == 1, "Wrong number of faces %u (must be 1)\n", nb_faces);
        ok(nb_face_vertices == 3, "Wrong number of vertices per face %u (must be 3)\n", nb_face_vertices);
        ok(data_size == 3, "Wrong number of face data bytes %u (must be 3)\n", data_size);
        color = IDirect3DRMMesh_GetGroupColor(mesh, 0);
        ok(color == 0xff00ff00, "Wrong color returned %#x instead of %#x\n", color, 0xff00ff00);
        hr = IDirect3DRMMesh_GetGroupTexture(mesh, 0, &texture);
        ok(hr == D3DRM_OK, "GetCroupTexture failed returning hr = %x\n", hr);
        ok(texture == NULL, "No texture should be present\n");
        hr = IDirect3DRMMesh_GetGroupMaterial(mesh, 0, &material);
        ok(hr == D3DRM_OK, "GetCroupMaterial failed returning hr = %x\n", hr);
        ok(material != NULL, "No material present\n");
        hr = IDirect3DRMMaterial_GetEmissive(material, &values[0], &values[1], &values[2]);
        ok(hr == D3DRM_OK, "Failed to get emissive color, hr %#x.\n", hr);
        ok(values[0] == 0.5f, "Got unexpected red component %.8e.\n", values[0]);
        ok(values[1] == 0.5f, "Got unexpected green component %.8e.\n", values[1]);
        ok(values[2] == 0.5f, "Got unexpected blue component %.8e.\n", values[2]);
        hr = IDirect3DRMMaterial_GetSpecular(material, &values[0], &values[1], &values[2]);
        ok(hr == D3DRM_OK, "Failed to get specular color, hr %#x.\n", hr);
        ok(values[0] == 1.0f, "Got unexpected red component %.8e.\n", values[0]);
        ok(values[1] == 0.0f, "Got unexpected green component %.8e.\n", values[1]);
        ok(values[2] == 0.0f, "Got unexpected blue component %.8e.\n", values[2]);
        values[0] = IDirect3DRMMaterial_GetPower(material);
        ok(values[0] == 30.0f, "Got unexpected power value %.8e.\n", values[0]);
        IDirect3DRMMaterial_Release(material);

        IDirect3DRMMesh_Release(mesh);
    }

    hr = IDirect3DRMMeshBuilder_Scale(pMeshBuilder, 2, 3 ,4);
    ok(hr == D3DRM_OK, "Scale failed returning hr = %x\n", hr);

    hr = IDirect3DRMMeshBuilder_GetVertices(pMeshBuilder, &val1, v, &val2, n, &val3, f);
    ok(hr == D3DRM_OK, "Cannot get vertices information (hr = %x)\n", hr);
    ok(val2 == 3, "Wrong number of normals %d (must be 3)\n", val2);
    ok(val1 == 3, "Wrong number of vertices %d (must be 3)\n", val1);

    expect_vector(&v[0], 0.1f * 2, 0.2f * 3, 0.3f * 4, 32);
    expect_vector(&v[1], 0.4f * 2, 0.5f * 3, 0.6f * 4, 32);
    expect_vector(&v[2], 0.7f * 2, 0.8f * 3, 0.9f * 4, 32);
    /* Normals are not affected by Scale */
    expect_vector(&n[0], 1.1f, 1.2f, 1.3f, 32);
    expect_vector(&n[1], 1.4f, 1.5f, 1.6f, 32);
    expect_vector(&n[2], 1.7f, 1.8f, 1.9f, 32);

    IDirect3DRMMeshBuilder_Release(pMeshBuilder);

    IDirect3DRM_Release(d3drm);
}

static void test_MeshBuilder3(void)
{
    HRESULT hr;
    IDirect3DRM *d3drm;
    IDirect3DRM3 *d3drm3;
    IDirect3DRMMeshBuilder3 *pMeshBuilder3;
    D3DRMLOADMEMORY info;
    int val;
    DWORD val1;
    D3DVALUE valu, valv;

    hr = Direct3DRMCreate(&d3drm);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);

    if (FAILED(hr = IDirect3DRM_QueryInterface(d3drm, &IID_IDirect3DRM3, (void **)&d3drm3)))
    {
        win_skip("Cannot get IDirect3DRM3 interface (hr = %x), skipping tests\n", hr);
        IDirect3DRM_Release(d3drm);
        return;
    }

    hr = IDirect3DRM3_CreateMeshBuilder(d3drm3, &pMeshBuilder3);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMMeshBuilder3 interface (hr = %x)\n", hr);

    test_class_name((IDirect3DRMObject *)pMeshBuilder3, "Builder");
    test_object_name((IDirect3DRMObject *)pMeshBuilder3);

    info.lpMemory = data_bad_version;
    info.dSize = strlen(data_bad_version);
    hr = IDirect3DRMMeshBuilder3_Load(pMeshBuilder3, &info, NULL, D3DRMLOAD_FROMMEMORY, NULL, NULL);
    ok(hr == D3DRMERR_BADFILE, "Should have returned D3DRMERR_BADFILE (hr = %x)\n", hr);

    info.lpMemory = data_no_mesh;
    info.dSize = strlen(data_no_mesh);
    hr = IDirect3DRMMeshBuilder3_Load(pMeshBuilder3, &info, NULL, D3DRMLOAD_FROMMEMORY, NULL, NULL);
    ok(hr == D3DRMERR_NOTFOUND, "Should have returned D3DRMERR_NOTFOUND (hr = %x)\n", hr);

    info.lpMemory = data_ok;
    info.dSize = strlen(data_ok);
    hr = IDirect3DRMMeshBuilder3_Load(pMeshBuilder3, &info, NULL, D3DRMLOAD_FROMMEMORY, NULL, NULL);
    ok(hr == D3DRM_OK, "Cannot load mesh data (hr = %x)\n", hr);

    val = IDirect3DRMMeshBuilder3_GetVertexCount(pMeshBuilder3);
    ok(val == 4, "Wrong number of vertices %d (must be 4)\n", val);

    val = IDirect3DRMMeshBuilder3_GetFaceCount(pMeshBuilder3);
    ok(val == 3, "Wrong number of faces %d (must be 3)\n", val);

    hr = IDirect3DRMMeshBuilder3_GetVertices(pMeshBuilder3, 0, &val1, NULL);
    ok(hr == D3DRM_OK, "Cannot get vertices information (hr = %x)\n", hr);
    ok(val1 == 4, "Wrong number of vertices %d (must be 4)\n", val1);

    /* Check that Load method generated default texture coordinates (0.0f, 0.0f) for each vertex */
    valu = 1.23f;
    valv = 3.21f;
    hr = IDirect3DRMMeshBuilder3_GetTextureCoordinates(pMeshBuilder3, 0, &valu, &valv);
    ok(hr == D3DRM_OK, "Cannot get texture coordinates (hr = %x)\n", hr);
    ok(valu == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valu);
    ok(valv == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valv);
    valu = 1.23f;
    valv = 3.21f;
    hr = IDirect3DRMMeshBuilder3_GetTextureCoordinates(pMeshBuilder3, 1, &valu, &valv);
    ok(hr == D3DRM_OK, "Cannot get texture coordinates (hr = %x)\n", hr);
    ok(valu == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valu);
    ok(valv == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valv);
    valu = 1.23f;
    valv = 3.21f;
    hr = IDirect3DRMMeshBuilder3_GetTextureCoordinates(pMeshBuilder3, 2, &valu, &valv);
    ok(hr == D3DRM_OK, "Cannot get texture coordinates (hr = %x)\n", hr);
    ok(valu == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valu);
    ok(valv == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valv);
    valu = 1.23f;
    valv = 3.21f;
    hr = IDirect3DRMMeshBuilder3_GetTextureCoordinates(pMeshBuilder3, 3, &valu, &valv);
    ok(hr == D3DRM_OK, "Cannot get texture coordinates (hr = %x)\n", hr);
    ok(valu == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valu);
    ok(valv == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valv);
    hr = IDirect3DRMMeshBuilder3_GetTextureCoordinates(pMeshBuilder3, 4, &valu, &valv);
    ok(hr == D3DRMERR_BADVALUE, "Should fail and return D3DRM_BADVALUE (hr = %x)\n", hr);

    valu = 1.23f;
    valv = 3.21f;
    hr = IDirect3DRMMeshBuilder3_SetTextureCoordinates(pMeshBuilder3, 0, valu, valv);
    ok(hr == D3DRM_OK, "Cannot set texture coordinates (hr = %x)\n", hr);
    hr = IDirect3DRMMeshBuilder3_SetTextureCoordinates(pMeshBuilder3, 4, valu, valv);
    ok(hr == D3DRMERR_BADVALUE, "Should fail and return D3DRM_BADVALUE (hr = %x)\n", hr);

    valu = 0.0f;
    valv = 0.0f;
    hr = IDirect3DRMMeshBuilder3_GetTextureCoordinates(pMeshBuilder3, 0, &valu, &valv);
    ok(hr == D3DRM_OK, "Cannot get texture coordinates (hr = %x)\n", hr);
    ok(valu == 1.23f, "Wrong coordinate %f (must be 1.23)\n", valu);
    ok(valv == 3.21f, "Wrong coordinate %f (must be 3.21)\n", valv);

    IDirect3DRMMeshBuilder3_Release(pMeshBuilder3);
    IDirect3DRM3_Release(d3drm3);
    IDirect3DRM_Release(d3drm);
}

static void test_Mesh(void)
{
    HRESULT hr;
    IDirect3DRM *d3drm;
    IDirect3DRMMesh *mesh;
    IUnknown *unk;

    hr = Direct3DRMCreate(&d3drm);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);

    hr = IDirect3DRM_CreateMesh(d3drm, &mesh);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMMesh interface (hr = %x)\n", hr);

    test_class_name((IDirect3DRMObject *)mesh, "Mesh");
    test_object_name((IDirect3DRMObject *)mesh);

    hr = IDirect3DRMMesh_QueryInterface(mesh, &IID_IDirect3DRMObject, (void **)&unk);
    ok(SUCCEEDED(hr), "Failed to get IDirect3DRMObject, %#x.\n", hr);
    IUnknown_Release(unk);

    hr = IDirect3DRMMesh_QueryInterface(mesh, &IID_IDirect3DRMVisual, (void **)&unk);
    ok(SUCCEEDED(hr), "Failed to get IDirect3DRMVisual, %#x.\n", hr);
    IUnknown_Release(unk);

    IDirect3DRMMesh_Release(mesh);

    IDirect3DRM_Release(d3drm);
}

static void test_Face(void)
{
    HRESULT hr;
    IDirect3DRM *d3drm;
    IDirect3DRM2 *d3drm2;
    IDirect3DRM3 *d3drm3;
    IDirect3DRMMeshBuilder2 *MeshBuilder2;
    IDirect3DRMMeshBuilder3 *MeshBuilder3;
    IDirect3DRMFace *face1;
    IDirect3DRMObject *obj;
    IDirect3DRMFace2 *face2;
    IDirect3DRMFaceArray *array1;
    D3DRMLOADMEMORY info;
    D3DVECTOR v1[4], n1[4], v2[4], n2[4];
    D3DCOLOR color;
    DWORD count;
    int icount;

    hr = Direct3DRMCreate(&d3drm);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);

    hr = IDirect3DRM_CreateFace(d3drm, &face1);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMFace interface (hr = %x)\n", hr);
    if (FAILED(hr))
    {
        skip("Cannot get IDirect3DRMFace interface (hr = %x), skipping tests\n", hr);
        IDirect3DRM_Release(d3drm);
        return;
    }

    hr = IDirect3DRMFace_QueryInterface(face1, &IID_IDirect3DRMObject, (void **)&obj);
    ok(SUCCEEDED(hr), "Failed to get IDirect3DRMObject, %#x.\n", hr);
    ok(obj == (IDirect3DRMObject *)face1, "Unexpected interface pointer.\n");
    IDirect3DRMObject_Release(obj);

    test_class_name((IDirect3DRMObject *)face1, "Face");
    test_object_name((IDirect3DRMObject *)face1);

    icount = IDirect3DRMFace_GetVertexCount(face1);
    ok(!icount, "wrong VertexCount: %i\n", icount);

    IDirect3DRMFace_Release(face1);

    if (FAILED(hr = IDirect3DRM_QueryInterface(d3drm, &IID_IDirect3DRM2, (void **)&d3drm2)))
    {
        win_skip("Cannot get IDirect3DRM2 interface (hr = %x), skipping tests\n", hr);
        IDirect3DRM_Release(d3drm);
        return;
    }

    hr = IDirect3DRM2_CreateMeshBuilder(d3drm2, &MeshBuilder2);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMMeshBuilder2 interface (hr = %x)\n", hr);

    icount = IDirect3DRMMeshBuilder2_GetFaceCount(MeshBuilder2);
    ok(!icount, "wrong FaceCount: %i\n", icount);

    array1 = NULL;
    hr = IDirect3DRMMeshBuilder2_GetFaces(MeshBuilder2, &array1);
    todo_wine
    ok(hr == D3DRM_OK, "Cannot get FaceArray (hr = %x)\n", hr);

    hr = IDirect3DRMMeshBuilder2_CreateFace(MeshBuilder2, &face1);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMFace interface (hr = %x)\n", hr);

    icount = IDirect3DRMMeshBuilder2_GetFaceCount(MeshBuilder2);
    todo_wine
    ok(icount == 1, "wrong FaceCount: %i\n", icount);

    array1 = NULL;
    hr = IDirect3DRMMeshBuilder2_GetFaces(MeshBuilder2, &array1);
    todo_wine
    ok(hr == D3DRM_OK, "Cannot get FaceArray (hr = %x)\n", hr);
    todo_wine
    ok(array1 != NULL, "pArray = %p\n", array1);
    if (array1)
    {
        IDirect3DRMFace *face;
        count = IDirect3DRMFaceArray_GetSize(array1);
        ok(count == 1, "count = %u\n", count);
        hr = IDirect3DRMFaceArray_GetElement(array1, 0, &face);
        ok(hr == D3DRM_OK, "Cannot get face (hr = %x)\n", hr);
        IDirect3DRMFace_Release(face);
        IDirect3DRMFaceArray_Release(array1);
    }

    icount = IDirect3DRMFace_GetVertexCount(face1);
    ok(!icount, "wrong VertexCount: %i\n", icount);

    IDirect3DRMFace_Release(face1);
    IDirect3DRMMeshBuilder2_Release(MeshBuilder2);

    if (FAILED(hr = IDirect3DRM_QueryInterface(d3drm, &IID_IDirect3DRM3, (void **)&d3drm3)))
    {
        win_skip("Cannot get IDirect3DRM3 interface (hr = %x), skipping tests\n", hr);
        IDirect3DRM_Release(d3drm);
        return;
    }

    hr = IDirect3DRM3_CreateMeshBuilder(d3drm3, &MeshBuilder3);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMMeshBuilder3 interface (hr = %x)\n", hr);

    icount = IDirect3DRMMeshBuilder3_GetFaceCount(MeshBuilder3);
    ok(!icount, "wrong FaceCount: %i\n", icount);

    hr = IDirect3DRMMeshBuilder3_CreateFace(MeshBuilder3, &face2);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMFace2 interface (hr = %x)\n", hr);

    hr = IDirect3DRMFace2_QueryInterface(face2, &IID_IDirect3DRMObject, (void **)&obj);
    ok(SUCCEEDED(hr), "Failed to get IDirect3DRMObject, %#x.\n", hr);

    hr = IDirect3DRMFace2_QueryInterface(face2, &IID_IDirect3DRMFace, (void **)&face1);
    ok(SUCCEEDED(hr), "Failed to get IDirect3DRMObject, %#x.\n", hr);
    ok(obj == (IDirect3DRMObject *)face1, "Unexpected interface pointer.\n");

    IDirect3DRMFace_Release(face1);
    IDirect3DRMObject_Release(obj);

    test_class_name((IDirect3DRMObject *)face2, "Face");
    test_object_name((IDirect3DRMObject *)face2);

    icount = IDirect3DRMMeshBuilder3_GetFaceCount(MeshBuilder3);
    todo_wine
    ok(icount == 1, "wrong FaceCount: %i\n", icount);

    array1 = NULL;
    hr = IDirect3DRMMeshBuilder3_GetFaces(MeshBuilder3, &array1);
    todo_wine
    ok(hr == D3DRM_OK, "Cannot get FaceArray (hr = %x)\n", hr);
    todo_wine
    ok(array1 != NULL, "pArray = %p\n", array1);
    if (array1)
    {
        IDirect3DRMFace *face;
        count = IDirect3DRMFaceArray_GetSize(array1);
        ok(count == 1, "count = %u\n", count);
        hr = IDirect3DRMFaceArray_GetElement(array1, 0, &face);
        ok(hr == D3DRM_OK, "Cannot get face (hr = %x)\n", hr);
        IDirect3DRMFace_Release(face);
        IDirect3DRMFaceArray_Release(array1);
    }

    icount = IDirect3DRMFace2_GetVertexCount(face2);
    ok(!icount, "wrong VertexCount: %i\n", icount);

    info.lpMemory = data_ok;
    info.dSize = strlen(data_ok);
    hr = IDirect3DRMMeshBuilder3_Load(MeshBuilder3, &info, NULL, D3DRMLOAD_FROMMEMORY, NULL, NULL);
    ok(hr == D3DRM_OK, "Cannot load mesh data (hr = %x)\n", hr);

    icount = IDirect3DRMMeshBuilder3_GetVertexCount(MeshBuilder3);
    ok(icount == 4, "Wrong number of vertices %d (must be 4)\n", icount);

    icount = IDirect3DRMMeshBuilder3_GetNormalCount(MeshBuilder3);
    ok(icount == 4, "Wrong number of normals %d (must be 4)\n", icount);

    icount = IDirect3DRMMeshBuilder3_GetFaceCount(MeshBuilder3);
    todo_wine
    ok(icount == 4, "Wrong number of faces %d (must be 4)\n", icount);

    count = 4;
    hr = IDirect3DRMMeshBuilder3_GetVertices(MeshBuilder3, 0, &count, v1);
    ok(hr == D3DRM_OK, "Cannot get vertices information (hr = %x)\n", hr);
    ok(count == 4, "Wrong number of vertices %d (must be 4)\n", count);

    hr = IDirect3DRMMeshBuilder3_GetNormals(MeshBuilder3, 0, &count, n1);
    ok(hr == D3DRM_OK, "Cannot get normals information (hr = %x)\n", hr);
    ok(count == 4, "Wrong number of normals %d (must be 4)\n", count);

    array1 = NULL;
    hr = IDirect3DRMMeshBuilder3_GetFaces(MeshBuilder3, &array1);
    todo_wine
    ok(hr == D3DRM_OK, "Cannot get FaceArray (hr = %x)\n", hr);
    todo_wine
    ok(array1 != NULL, "pArray = %p\n", array1);
    if (array1)
    {
        IDirect3DRMFace *face;
        count = IDirect3DRMFaceArray_GetSize(array1);
        ok(count == 4, "count = %u\n", count);
        hr = IDirect3DRMFaceArray_GetElement(array1, 1, &face);
        ok(hr == D3DRM_OK, "Cannot get face (hr = %x)\n", hr);
        hr = IDirect3DRMFace_GetVertices(face, &count, v2, n2);
        ok(hr == D3DRM_OK, "Cannot get vertices information (hr = %x)\n", hr);
        ok(count == 3, "Wrong number of vertices %d (must be 3)\n", count);

        vector_eq(&v1[0], &v2[0]);
        vector_eq(&v1[1], &v2[1]);
        vector_eq(&v1[2], &v2[2]);

        vector_eq(&n1[0], &n2[0]);
        vector_eq(&n1[1], &n2[1]);
        vector_eq(&n1[2], &n2[2]);

        IDirect3DRMFace_Release(face);
        IDirect3DRMFaceArray_Release(array1);
    }

    /* Setting face color. */
    hr = IDirect3DRMFace2_SetColor(face2, 0x1f180587);
    ok(SUCCEEDED(hr), "Failed to set face color, hr %#x.\n", hr);
    color = IDirect3DRMFace2_GetColor(face2);
    ok(color == 0x1f180587, "Unexpected color %8x.\n", color);

    hr = IDirect3DRMFace2_SetColorRGB(face2, 0.5f, 0.5f, 0.5f);
    ok(SUCCEEDED(hr), "Failed to set color, hr %#x.\n", hr);
    color = IDirect3DRMFace2_GetColor(face2);
    ok(color == 0xff7f7f7f, "Unexpected color %8x.\n", color);

    IDirect3DRMFace2_Release(face2);
    IDirect3DRMMeshBuilder3_Release(MeshBuilder3);
    IDirect3DRM3_Release(d3drm3);
    IDirect3DRM2_Release(d3drm2);
    IDirect3DRM_Release(d3drm);
}

static void test_Frame(void)
{
    HRESULT hr;
    IDirect3DRM *d3drm;
    IDirect3DRMFrame *pFrameC;
    IDirect3DRMFrame *pFrameP1;
    IDirect3DRMFrame *pFrameP2;
    IDirect3DRMFrame *pFrameTmp;
    IDirect3DRMFrame *scene_frame;
    IDirect3DRMFrameArray *frame_array;
    IDirect3DRMMeshBuilder *mesh_builder;
    IDirect3DRMVisual *visual1;
    IDirect3DRMVisual *visual_tmp;
    IDirect3DRMVisualArray *visual_array;
    IDirect3DRMLight *light1;
    IDirect3DRMLight *light_tmp;
    IDirect3DRMLightArray *light_array;
    IDirect3DRMFrame3 *frame3;
    DWORD count, options;
    ULONG ref, ref2;
    D3DCOLOR color;

    hr = Direct3DRMCreate(&d3drm);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);

    ref = get_refcount((IUnknown *)d3drm);
    hr = IDirect3DRM_CreateFrame(d3drm, NULL, &pFrameC);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMFrame interface (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameC, 1);
    ref2 = get_refcount((IUnknown *)d3drm);
    ok(ref2 > ref, "Expected d3drm object to be referenced.\n");

    test_class_name((IDirect3DRMObject *)pFrameC, "Frame");
    test_object_name((IDirect3DRMObject *)pFrameC);

    hr = IDirect3DRMFrame_GetParent(pFrameC, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Should fail and return D3DRM_BADVALUE (hr = %x)\n", hr);
    pFrameTmp = (void*)0xdeadbeef;
    hr = IDirect3DRMFrame_GetParent(pFrameC, &pFrameTmp);
    ok(hr == D3DRM_OK, "Cannot get parent frame (hr = %x)\n", hr);
    ok(pFrameTmp == NULL, "pFrameTmp = %p\n", pFrameTmp);
    CHECK_REFCOUNT(pFrameC, 1);

    frame_array = NULL;
    hr = IDirect3DRMFrame_GetChildren(pFrameC, &frame_array);
    ok(hr == D3DRM_OK, "Cannot get children (hr = %x)\n", hr);
    ok(!!frame_array, "frame_array = %p\n", frame_array);
    if (frame_array)
    {
        count = IDirect3DRMFrameArray_GetSize(frame_array);
        ok(count == 0, "count = %u\n", count);
        hr = IDirect3DRMFrameArray_GetElement(frame_array, 0, &pFrameTmp);
        ok(hr == D3DRMERR_BADVALUE, "Should have returned D3DRMERR_BADVALUE (hr = %x)\n", hr);
        ok(pFrameTmp == NULL, "pFrameTmp = %p\n", pFrameTmp);
        IDirect3DRMFrameArray_Release(frame_array);
    }

    hr = IDirect3DRM_CreateFrame(d3drm, NULL, &pFrameP1);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMFrame interface (hr = %x)\n", hr);

    /* GetParent with NULL pointer */
    hr = IDirect3DRMFrame_GetParent(pFrameP1, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Should have returned D3DRMERR_BADVALUE (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 1);

    /* [Add/Delete]Child with NULL pointer */
    hr = IDirect3DRMFrame_AddChild(pFrameP1, NULL);
    ok(hr == D3DRMERR_BADOBJECT, "Should have returned D3DRMERR_BADOBJECT (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 1);

    hr = IDirect3DRMFrame_DeleteChild(pFrameP1, NULL);
    ok(hr == D3DRMERR_BADOBJECT, "Should have returned D3DRMERR_BADOBJECT (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 1);

    /* Add child to first parent */
    pFrameTmp = (void*)0xdeadbeef;
    hr = IDirect3DRMFrame_GetParent(pFrameP1, &pFrameTmp);
    ok(hr == D3DRM_OK, "Cannot get parent frame (hr = %x)\n", hr);
    ok(pFrameTmp == NULL, "pFrameTmp = %p\n", pFrameTmp);

    hr = IDirect3DRMFrame_AddChild(pFrameP1, pFrameC);
    ok(hr == D3DRM_OK, "Cannot add child frame (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 1);
    CHECK_REFCOUNT(pFrameC, 2);

    hr = IDirect3DRMFrame_GetScene(pFrameC, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    hr = IDirect3DRMFrame_GetScene(pFrameC, &scene_frame);
    ok(SUCCEEDED(hr), "Cannot get scene (hr == %#x).\n", hr);
    ok(scene_frame == pFrameP1, "Expected scene frame == %p, got %p.\n", pFrameP1, scene_frame);
    CHECK_REFCOUNT(pFrameP1, 2);
    IDirect3DRMFrame_Release(scene_frame);
    hr = IDirect3DRMFrame_GetScene(pFrameP1, &scene_frame);
    ok(SUCCEEDED(hr), "Cannot get scene (hr == %#x).\n", hr);
    ok(scene_frame == pFrameP1, "Expected scene frame == %p, got %p.\n", pFrameP1, scene_frame);
    CHECK_REFCOUNT(pFrameP1, 2);
    IDirect3DRMFrame_Release(scene_frame);

    frame_array = NULL;
    hr = IDirect3DRMFrame_GetChildren(pFrameP1, &frame_array);
    ok(hr == D3DRM_OK, "Cannot get children (hr = %x)\n", hr);
    /* In some older version of d3drm, creating IDirect3DRMFrameArray object with GetChildren does not increment refcount of children frames */
    ok((get_refcount((IUnknown*)pFrameC) == 3) || broken(get_refcount((IUnknown*)pFrameC) == 2),
            "Invalid refcount. Expected 3 (or 2) got %d\n", get_refcount((IUnknown*)pFrameC));
    if (frame_array)
    {
        count = IDirect3DRMFrameArray_GetSize(frame_array);
        ok(count == 1, "count = %u\n", count);
        hr = IDirect3DRMFrameArray_GetElement(frame_array, 0, &pFrameTmp);
        ok(hr == D3DRM_OK, "Cannot get element (hr = %x)\n", hr);
        ok(pFrameTmp == pFrameC, "pFrameTmp = %p\n", pFrameTmp);
        ok((get_refcount((IUnknown*)pFrameC) == 4) || broken(get_refcount((IUnknown*)pFrameC) == 3),
                "Invalid refcount. Expected 4 (or 3) got %d\n", get_refcount((IUnknown*)pFrameC));
        IDirect3DRMFrame_Release(pFrameTmp);
        ok((get_refcount((IUnknown*)pFrameC) == 3) || broken(get_refcount((IUnknown*)pFrameC) == 2),
                "Invalid refcount. Expected 3 (or 2) got %d\n", get_refcount((IUnknown*)pFrameC));
        IDirect3DRMFrameArray_Release(frame_array);
        CHECK_REFCOUNT(pFrameC, 2);
    }

    pFrameTmp = (void*)0xdeadbeef;
    hr = IDirect3DRMFrame_GetParent(pFrameC, &pFrameTmp);
    ok(hr == D3DRM_OK, "Cannot get parent frame (hr = %x)\n", hr);
    ok(pFrameTmp == pFrameP1, "pFrameTmp = %p\n", pFrameTmp);
    CHECK_REFCOUNT(pFrameP1, 2);
    IDirect3DRMFrame_Release(pFrameTmp);
    CHECK_REFCOUNT(pFrameP1, 1);

    /* Add child to second parent */
    hr = IDirect3DRM_CreateFrame(d3drm, NULL, &pFrameP2);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMFrame interface (hr = %x)\n", hr);

    hr = IDirect3DRMFrame_AddChild(pFrameP2, pFrameC);
    ok(hr == D3DRM_OK, "Cannot add child frame (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameC, 2);

    frame_array = NULL;
    hr = IDirect3DRMFrame_GetChildren(pFrameP2, &frame_array);
    ok(hr == D3DRM_OK, "Cannot get children (hr = %x)\n", hr);
    if (frame_array)
    {
        count = IDirect3DRMFrameArray_GetSize(frame_array);
        ok(count == 1, "count = %u\n", count);
        hr = IDirect3DRMFrameArray_GetElement(frame_array, 0, &pFrameTmp);
        ok(hr == D3DRM_OK, "Cannot get element (hr = %x)\n", hr);
        ok(pFrameTmp == pFrameC, "pFrameTmp = %p\n", pFrameTmp);
        IDirect3DRMFrame_Release(pFrameTmp);
        IDirect3DRMFrameArray_Release(frame_array);
    }

    frame_array = NULL;
    hr = IDirect3DRMFrame_GetChildren(pFrameP1, &frame_array);
    ok(hr == D3DRM_OK, "Cannot get children (hr = %x)\n", hr);
    if (frame_array)
    {
        count = IDirect3DRMFrameArray_GetSize(frame_array);
        ok(count == 0, "count = %u\n", count);
        pFrameTmp = (void*)0xdeadbeef;
        hr = IDirect3DRMFrameArray_GetElement(frame_array, 0, &pFrameTmp);
        ok(hr == D3DRMERR_BADVALUE, "Should have returned D3DRMERR_BADVALUE (hr = %x)\n", hr);
        ok(pFrameTmp == NULL, "pFrameTmp = %p\n", pFrameTmp);
        IDirect3DRMFrameArray_Release(frame_array);
    }
    hr = IDirect3DRMFrame_GetScene(pFrameC, &scene_frame);
    ok(SUCCEEDED(hr), "Cannot get scene (hr == %#x).\n", hr);
    ok(scene_frame == pFrameP2, "Expected scene frame == %p, got %p.\n", pFrameP2, scene_frame);
    CHECK_REFCOUNT(pFrameP2, 2);
    IDirect3DRMFrame_Release(scene_frame);
    hr = IDirect3DRMFrame_GetScene(pFrameP2, &scene_frame);
    ok(SUCCEEDED(hr), "Cannot get scene (hr == %#x).\n", hr);
    ok(scene_frame == pFrameP2, "Expected scene frame == %p, got %p.\n", pFrameP2, scene_frame);
    CHECK_REFCOUNT(pFrameP2, 2);
    IDirect3DRMFrame_Release(scene_frame);

    pFrameTmp = (void*)0xdeadbeef;
    hr = IDirect3DRMFrame_GetParent(pFrameC, &pFrameTmp);
    ok(hr == D3DRM_OK, "Cannot get parent frame (hr = %x)\n", hr);
    ok(pFrameTmp == pFrameP2, "pFrameTmp = %p\n", pFrameTmp);
    CHECK_REFCOUNT(pFrameP2, 2);
    CHECK_REFCOUNT(pFrameC, 2);
    IDirect3DRMFrame_Release(pFrameTmp);
    CHECK_REFCOUNT(pFrameP2, 1);

    /* Add child again */
    hr = IDirect3DRMFrame_AddChild(pFrameP2, pFrameC);
    ok(hr == D3DRM_OK, "Cannot add child frame (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameC, 2);

    frame_array = NULL;
    hr = IDirect3DRMFrame_GetChildren(pFrameP2, &frame_array);
    ok(hr == D3DRM_OK, "Cannot get children (hr = %x)\n", hr);
    if (frame_array)
    {
        count = IDirect3DRMFrameArray_GetSize(frame_array);
        ok(count == 1, "count = %u\n", count);
        hr = IDirect3DRMFrameArray_GetElement(frame_array, 0, &pFrameTmp);
        ok(hr == D3DRM_OK, "Cannot get element (hr = %x)\n", hr);
        ok(pFrameTmp == pFrameC, "pFrameTmp = %p\n", pFrameTmp);
        IDirect3DRMFrame_Release(pFrameTmp);
        IDirect3DRMFrameArray_Release(frame_array);
    }

    /* Delete child */
    hr = IDirect3DRMFrame_DeleteChild(pFrameP2, pFrameC);
    ok(hr == D3DRM_OK, "Cannot delete child frame (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameC, 1);

    frame_array = NULL;
    hr = IDirect3DRMFrame_GetChildren(pFrameP2, &frame_array);
    ok(hr == D3DRM_OK, "Cannot get children (hr = %x)\n", hr);
    if (frame_array)
    {
        count = IDirect3DRMFrameArray_GetSize(frame_array);
        ok(count == 0, "count = %u\n", count);
        pFrameTmp = (void*)0xdeadbeef;
        hr = IDirect3DRMFrameArray_GetElement(frame_array, 0, &pFrameTmp);
        ok(hr == D3DRMERR_BADVALUE, "Should have returned D3DRMERR_BADVALUE (hr = %x)\n", hr);
        ok(pFrameTmp == NULL, "pFrameTmp = %p\n", pFrameTmp);
        IDirect3DRMFrameArray_Release(frame_array);
    }

    pFrameTmp = (void*)0xdeadbeef;
    hr = IDirect3DRMFrame_GetParent(pFrameC, &pFrameTmp);
    ok(hr == D3DRM_OK, "Cannot get parent frame (hr = %x)\n", hr);
    ok(pFrameTmp == NULL, "pFrameTmp = %p\n", pFrameTmp);

    /* Add two children */
    hr = IDirect3DRMFrame_AddChild(pFrameP2, pFrameC);
    ok(hr == D3DRM_OK, "Cannot add child frame (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameC, 2);

    hr = IDirect3DRMFrame_AddChild(pFrameP2, pFrameP1);
    ok(hr == D3DRM_OK, "Cannot add child frame (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 2);

    frame_array = NULL;
    hr = IDirect3DRMFrame_GetChildren(pFrameP2, &frame_array);
    ok(hr == D3DRM_OK, "Cannot get children (hr = %x)\n", hr);
    if (frame_array)
    {
        count = IDirect3DRMFrameArray_GetSize(frame_array);
        ok(count == 2, "count = %u\n", count);
        hr = IDirect3DRMFrameArray_GetElement(frame_array, 0, &pFrameTmp);
        ok(hr == D3DRM_OK, "Cannot get element (hr = %x)\n", hr);
        ok(pFrameTmp == pFrameC, "pFrameTmp = %p\n", pFrameTmp);
        IDirect3DRMFrame_Release(pFrameTmp);
        hr = IDirect3DRMFrameArray_GetElement(frame_array, 1, &pFrameTmp);
        ok(hr == D3DRM_OK, "Cannot get element (hr = %x)\n", hr);
        ok(pFrameTmp == pFrameP1, "pFrameTmp = %p\n", pFrameTmp);
        IDirect3DRMFrame_Release(pFrameTmp);
        IDirect3DRMFrameArray_Release(frame_array);
    }

    /* [Add/Delete]Visual with NULL pointer */
    hr = IDirect3DRMFrame_AddVisual(pFrameP1, NULL);
    ok(hr == D3DRMERR_BADOBJECT, "Should have returned D3DRMERR_BADOBJECT (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 2);

    hr = IDirect3DRMFrame_DeleteVisual(pFrameP1, NULL);
    ok(hr == D3DRMERR_BADOBJECT, "Should have returned D3DRMERR_BADOBJECT (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 2);

    /* Create Visual */
    hr = IDirect3DRM_CreateMeshBuilder(d3drm, &mesh_builder);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMMeshBuilder interface (hr = %x)\n", hr);
    visual1 = (IDirect3DRMVisual *)mesh_builder;

    /* Add Visual to first parent */
    hr = IDirect3DRMFrame_AddVisual(pFrameP1, visual1);
    ok(hr == D3DRM_OK, "Cannot add visual (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 2);
    CHECK_REFCOUNT(visual1, 2);

    visual_array = NULL;
    hr = IDirect3DRMFrame_GetVisuals(pFrameP1, &visual_array);
    ok(hr == D3DRM_OK, "Cannot get visuals (hr = %x)\n", hr);
    if (visual_array)
    {
        count = IDirect3DRMVisualArray_GetSize(visual_array);
        ok(count == 1, "count = %u\n", count);
        hr = IDirect3DRMVisualArray_GetElement(visual_array, 0, &visual_tmp);
        ok(hr == D3DRM_OK, "Cannot get element (hr = %x)\n", hr);
        ok(visual_tmp == visual1, "visual_tmp = %p\n", visual_tmp);
        IDirect3DRMVisual_Release(visual_tmp);
        IDirect3DRMVisualArray_Release(visual_array);
    }

    /* Delete Visual */
    hr = IDirect3DRMFrame_DeleteVisual(pFrameP1, visual1);
    ok(hr == D3DRM_OK, "Cannot delete visual (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 2);
    IDirect3DRMMeshBuilder_Release(mesh_builder);

    /* [Add/Delete]Light with NULL pointer */
    hr = IDirect3DRMFrame_AddLight(pFrameP1, NULL);
    ok(hr == D3DRMERR_BADOBJECT, "Should have returned D3DRMERR_BADOBJECT (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 2);

    hr = IDirect3DRMFrame_DeleteLight(pFrameP1, NULL);
    ok(hr == D3DRMERR_BADOBJECT, "Should have returned D3DRMERR_BADOBJECT (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 2);

    /* Create Light */
    hr = IDirect3DRM_CreateLightRGB(d3drm, D3DRMLIGHT_SPOT, 0.1, 0.2, 0.3, &light1);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMLight interface (hr = %x)\n", hr);

    /* Add Light to first parent */
    hr = IDirect3DRMFrame_AddLight(pFrameP1, light1);
    ok(hr == D3DRM_OK, "Cannot add light (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 2);
    CHECK_REFCOUNT(light1, 2);

    light_array = NULL;
    hr = IDirect3DRMFrame_GetLights(pFrameP1, &light_array);
    ok(hr == D3DRM_OK, "Cannot get lights (hr = %x)\n", hr);
    if (light_array)
    {
        count = IDirect3DRMLightArray_GetSize(light_array);
        ok(count == 1, "count = %u\n", count);
        hr = IDirect3DRMLightArray_GetElement(light_array, 0, &light_tmp);
        ok(hr == D3DRM_OK, "Cannot get element (hr = %x)\n", hr);
        ok(light_tmp == light1, "light_tmp = %p\n", light_tmp);
        IDirect3DRMLight_Release(light_tmp);
        IDirect3DRMLightArray_Release(light_array);
    }

    /* Delete Light */
    hr = IDirect3DRMFrame_DeleteLight(pFrameP1, light1);
    ok(hr == D3DRM_OK, "Cannot delete light (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 2);
    IDirect3DRMLight_Release(light1);

    /* Test SceneBackground on first parent */
    color = IDirect3DRMFrame_GetSceneBackground(pFrameP1);
    ok(color == 0xff000000, "wrong color (%x)\n", color);

    hr = IDirect3DRMFrame_SetSceneBackground(pFrameP1, 0xff180587);
    ok(hr == D3DRM_OK, "Cannot set color (hr = %x)\n", hr);
    color = IDirect3DRMFrame_GetSceneBackground(pFrameP1);
    ok(color == 0xff180587, "wrong color (%x)\n", color);

    hr = IDirect3DRMFrame_SetSceneBackgroundRGB(pFrameP1, 0.5, 0.5, 0.5);
    ok(hr == D3DRM_OK, "Cannot set color (hr = %x)\n", hr);
    color = IDirect3DRMFrame_GetSceneBackground(pFrameP1);
    ok(color == 0xff7f7f7f, "wrong color (%x)\n", color);

    /* Traversal options. */
    hr = IDirect3DRMFrame_QueryInterface(pFrameP2, &IID_IDirect3DRMFrame3, (void **)&frame3);
    ok(SUCCEEDED(hr), "Failed to get IDirect3DRMFrame3 interface, hr %#x.\n", hr);

    hr = IDirect3DRMFrame3_GetTraversalOptions(frame3, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Unexpected hr %#x.\n", hr);

    options = 0;
    hr = IDirect3DRMFrame3_GetTraversalOptions(frame3, &options);
    ok(SUCCEEDED(hr), "Failed to get traversal options, hr %#x.\n", hr);
    ok(options == (D3DRMFRAME_RENDERENABLE | D3DRMFRAME_PICKENABLE), "Unexpected default options %#x.\n", options);

    hr = IDirect3DRMFrame3_SetTraversalOptions(frame3, 0);
    ok(SUCCEEDED(hr), "Unexpected hr %#x.\n", hr);

    hr = IDirect3DRMFrame3_SetTraversalOptions(frame3, 0xf0000000);
    ok(hr == D3DRMERR_BADVALUE, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DRMFrame3_SetTraversalOptions(frame3, 0xf0000000 | D3DRMFRAME_PICKENABLE);
    ok(hr == D3DRMERR_BADVALUE, "Unexpected hr %#x.\n", hr);

    options = 0xf;
    hr = IDirect3DRMFrame3_GetTraversalOptions(frame3, &options);
    ok(SUCCEEDED(hr), "Failed to get traversal options, hr %#x.\n", hr);
    ok(options == 0, "Unexpected traversal options %#x.\n", options);

    hr = IDirect3DRMFrame3_SetTraversalOptions(frame3, D3DRMFRAME_PICKENABLE);
    ok(SUCCEEDED(hr), "Failed to set traversal options, hr %#x.\n", hr);

    options = 0;
    hr = IDirect3DRMFrame3_GetTraversalOptions(frame3, &options);
    ok(SUCCEEDED(hr), "Failed to get traversal options, hr %#x.\n", hr);
    ok(options == D3DRMFRAME_PICKENABLE, "Unexpected traversal options %#x.\n", options);

    IDirect3DRMFrame3_Release(frame3);

    /* Cleanup */
    IDirect3DRMFrame_Release(pFrameP2);
    CHECK_REFCOUNT(pFrameC, 1);
    CHECK_REFCOUNT(pFrameP1, 1);

    IDirect3DRMFrame_Release(pFrameC);
    IDirect3DRMFrame_Release(pFrameP1);

    IDirect3DRM_Release(d3drm);
}

struct destroy_context
{
    IDirect3DRMObject *obj;
    unsigned int test_idx;
    int called;
};

struct callback_order
{
    void *callback;
    void *context;
} corder[3], d3drm_corder[3];

static void CDECL destroy_callback(IDirect3DRMObject *obj, void *arg)
{
    struct destroy_context *ctxt = arg;
    ok(ctxt->called == 1 || ctxt->called == 2, "got called counter %d\n", ctxt->called);
    ok(obj == ctxt->obj, "called with %p, expected %p\n", obj, ctxt->obj);
    d3drm_corder[ctxt->called].callback = &destroy_callback;
    d3drm_corder[ctxt->called++].context = ctxt;
}

static void CDECL destroy_callback1(IDirect3DRMObject *obj, void *arg)
{
    struct destroy_context *ctxt = (struct destroy_context*)arg;
    ok(ctxt->called == 0, "got called counter %d\n", ctxt->called);
    ok(obj == ctxt->obj, "called with %p, expected %p\n", obj, ctxt->obj);
    d3drm_corder[ctxt->called].callback = &destroy_callback1;
    d3drm_corder[ctxt->called++].context = ctxt;
}

static void test_destroy_callback(unsigned int test_idx, REFCLSID clsid, REFIID iid)
{
    struct destroy_context context;
    IDirect3DRMObject *obj;
    IUnknown *unknown;
    IDirect3DRM *d3drm;
    HRESULT hr;
    int i;

    hr = Direct3DRMCreate(&d3drm);
    ok(SUCCEEDED(hr), "Test %u: Cannot get IDirect3DRM interface (hr = %x).\n", test_idx, hr);

    hr = IDirect3DRM_CreateObject(d3drm, clsid, NULL, iid, (void **)&unknown);
    ok(hr == D3DRM_OK, "Test %u: Cannot get IDirect3DRMObject interface (hr = %x).\n", test_idx, hr);
    hr = IUnknown_QueryInterface(unknown, &IID_IDirect3DRMObject, (void**)&obj);
    ok(hr == D3DRM_OK, "Test %u: expected D3DRM_OK (hr = %x).\n", test_idx, hr);
    IUnknown_Release(unknown);

    context.called = 0;
    context.test_idx = test_idx;
    context.obj = obj;

    hr = IDirect3DRMObject_AddDestroyCallback(obj, NULL, &context);
    ok(hr == D3DRMERR_BADVALUE, "Test %u: expected D3DRMERR_BADVALUE (hr = %x).\n", test_idx, hr);

    hr = IDirect3DRMObject_AddDestroyCallback(obj, destroy_callback, &context);
    ok(hr == D3DRM_OK, "Test %u: expected D3DRM_OK (hr = %x).\n", test_idx, hr);
    corder[2].callback = &destroy_callback;
    corder[2].context = &context;

    /* same callback added twice */
    hr = IDirect3DRMObject_AddDestroyCallback(obj, destroy_callback, &context);
    ok(hr == D3DRM_OK, "Test %u: expected D3DRM_OK (hr = %x).\n", test_idx, hr);
    corder[1].callback = &destroy_callback;
    corder[1].context = &context;

    hr = IDirect3DRMObject_DeleteDestroyCallback(obj, destroy_callback1, NULL);
    ok(hr == D3DRM_OK, "Test %u: expected D3DRM_OK (hr = %x).\n", test_idx, hr);

    hr = IDirect3DRMObject_DeleteDestroyCallback(obj, destroy_callback1, &context);
    ok(hr == D3DRM_OK, "Test %u: expected D3DRM_OK (hr = %x).\n", test_idx, hr);

    /* add one more */
    hr = IDirect3DRMObject_AddDestroyCallback(obj, destroy_callback1, &context);
    ok(hr == D3DRM_OK, "Test %u: expected D3DRM_OK (hr = %x).\n", test_idx, hr);
    corder[0].callback = &destroy_callback1;
    corder[0].context = &context;

    hr = IDirect3DRMObject_DeleteDestroyCallback(obj, NULL, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Test %u: expected D3DRM_BADVALUE (hr = %x).\n", test_idx, hr);

    context.called = 0;
    IDirect3DRMObject_Release(obj);
    ok(context.called == 3, "Test %u: got %d, expected 3.\n", test_idx, context.called);
    for (i = 0; i < context.called; i++)
    {
        ok(corder[i].callback == d3drm_corder[i].callback
                && corder[i].context == d3drm_corder[i].context,
                "Expected callback = %p, context = %p. Got callback = %p, context = %p.\n", d3drm_corder[i].callback,
                d3drm_corder[i].context, corder[i].callback, corder[i].context);
    }

    /* test this pattern - add cb1, add cb2, add cb1, delete cb1 */
    hr = IDirect3DRM_CreateObject(d3drm, clsid, NULL, iid, (void **)&unknown);
    ok(hr == D3DRM_OK, "Test %u: Cannot get IDirect3DRMObject interface (hr = %x).\n", test_idx, hr);
    hr = IUnknown_QueryInterface(unknown, &IID_IDirect3DRMObject, (void**)&obj);
    ok(hr == D3DRM_OK, "Test %u: expected D3DRM_OK (hr = %x).\n", test_idx, hr);
    IUnknown_Release(unknown);

    hr = IDirect3DRMObject_AddDestroyCallback(obj, destroy_callback, &context);
    ok(hr == D3DRM_OK, "Test %u: expected D3DRM_OK (hr = %x).\n", test_idx, hr);
    corder[1].callback = &destroy_callback;
    corder[1].context = &context;

    hr = IDirect3DRMObject_AddDestroyCallback(obj, destroy_callback1, &context);
    ok(hr == D3DRM_OK, "Test %u: expected D3DRM_OK (hr = %x).\n", test_idx, hr);
    corder[0].callback = &destroy_callback1;
    corder[0].context = &context;

    hr = IDirect3DRMObject_AddDestroyCallback(obj, destroy_callback, &context);
    ok(hr == D3DRM_OK, "Test %u: expected D3DRM_OK (hr = %x).\n", test_idx, hr);

    hr = IDirect3DRMObject_DeleteDestroyCallback(obj, destroy_callback, &context);
    ok(hr == D3DRM_OK, "Test %u: expected D3DRM_OK (hr = %x).\n", test_idx, hr);

    context.called = 0;
    hr = IDirect3DRMObject_QueryInterface(obj, &IID_IDirect3DRMObject, (void**)&context.obj);
    ok(hr == D3DRM_OK, "Test %u: expected D3DRM_OK (hr = %x).\n", test_idx, hr);
    IDirect3DRMObject_Release(context.obj);
    IUnknown_Release(unknown);
    ok(context.called == 2, "Test %u: got %d, expected 2.\n", test_idx, context.called);
    for (i = 0; i < context.called; i++)
    {
        ok(corder[i].callback == d3drm_corder[i].callback
                && corder[i].context == d3drm_corder[i].context,
                "Expected callback = %p, context = %p. Got callback = %p, context = %p.\n", d3drm_corder[i].callback,
                d3drm_corder[i].context, corder[i].callback, corder[i].context);
    }

    IDirect3DRM_Release(d3drm);
}

static void test_object(void)
{
    static const struct
    {
        REFCLSID clsid;
        REFIID iid;
        BOOL takes_d3drm_ref;
    }
    tests[] =
    {
        { &CLSID_CDirect3DRMDevice,      &IID_IDirect3DRMDevice },
        { &CLSID_CDirect3DRMDevice,      &IID_IDirect3DRMDevice2 },
        { &CLSID_CDirect3DRMDevice,      &IID_IDirect3DRMDevice3 },
        { &CLSID_CDirect3DRMDevice,      &IID_IDirect3DRMWinDevice },
        { &CLSID_CDirect3DRMTexture,     &IID_IDirect3DRMTexture },
        { &CLSID_CDirect3DRMTexture,     &IID_IDirect3DRMTexture2 },
        { &CLSID_CDirect3DRMTexture,     &IID_IDirect3DRMTexture3 },
        { &CLSID_CDirect3DRMViewport,    &IID_IDirect3DRMViewport },
        { &CLSID_CDirect3DRMViewport,    &IID_IDirect3DRMViewport2 },
        { &CLSID_CDirect3DRMFace,        &IID_IDirect3DRMFace },
        { &CLSID_CDirect3DRMFace,        &IID_IDirect3DRMFace2 },
        { &CLSID_CDirect3DRMMeshBuilder, &IID_IDirect3DRMMeshBuilder,  TRUE },
        { &CLSID_CDirect3DRMMeshBuilder, &IID_IDirect3DRMMeshBuilder2, TRUE },
        { &CLSID_CDirect3DRMMeshBuilder, &IID_IDirect3DRMMeshBuilder3, TRUE },
        { &CLSID_CDirect3DRMFrame,       &IID_IDirect3DRMFrame,        TRUE },
        { &CLSID_CDirect3DRMFrame,       &IID_IDirect3DRMFrame2,       TRUE },
        { &CLSID_CDirect3DRMFrame,       &IID_IDirect3DRMFrame3,       TRUE },
        { &CLSID_CDirect3DRMLight,       &IID_IDirect3DRMLight,        TRUE },
        { &CLSID_CDirect3DRMMaterial,    &IID_IDirect3DRMMaterial,     TRUE },
        { &CLSID_CDirect3DRMMaterial,    &IID_IDirect3DRMMaterial2,    TRUE },
        { &CLSID_CDirect3DRMMesh,        &IID_IDirect3DRMMesh,         TRUE },
        { &CLSID_CDirect3DRMAnimation,   &IID_IDirect3DRMAnimation,    TRUE },
        { &CLSID_CDirect3DRMAnimation,   &IID_IDirect3DRMAnimation2,   TRUE },
        { &CLSID_CDirect3DRMWrap,        &IID_IDirect3DRMWrap },
    };
    IDirect3DRM *d3drm1;
    IDirect3DRM2 *d3drm2;
    IDirect3DRM3 *d3drm3;
    IUnknown *unknown = (IUnknown *)0xdeadbeef;
    HRESULT hr;
    ULONG ref1, ref2, ref3, ref4;
    int i;

    hr = Direct3DRMCreate(&d3drm1);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRM interface (hr = %#x).\n", hr);
    ref1 = get_refcount((IUnknown *)d3drm1);
    hr = IDirect3DRM_QueryInterface(d3drm1, &IID_IDirect3DRM2, (void **)&d3drm2);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRM2 interface (hr = %#x).\n", hr);
    hr = IDirect3DRM_QueryInterface(d3drm1, &IID_IDirect3DRM3, (void **)&d3drm3);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRM3 interface (hr = %#x).\n", hr);

    hr = IDirect3DRM_CreateObject(d3drm1, &CLSID_DirectDraw, NULL, &IID_IDirectDraw, (void **)&unknown);
    ok(hr == CLASSFACTORY_E_FIRST, "Expected hr == CLASSFACTORY_E_FIRST, got %#x.\n", hr);
    ok(!unknown, "Expected object returned == NULL, got %p.\n", unknown);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        unknown = (IUnknown *)0xdeadbeef;
        hr = IDirect3DRM_CreateObject(d3drm1, NULL, NULL, tests[i].iid, (void **)&unknown);
        ok(hr == D3DRMERR_BADVALUE, "Test %u: expected hr == D3DRMERR_BADVALUE, got %#x.\n", i, hr);
        ok(!unknown, "Expected object returned == NULL, got %p.\n", unknown);
        unknown = (IUnknown *)0xdeadbeef;
        hr = IDirect3DRM_CreateObject(d3drm1, tests[i].clsid, NULL, NULL, (void **)&unknown);
        ok(hr == D3DRMERR_BADVALUE, "Test %u: expected hr == D3DRMERR_BADVALUE, got %#x.\n", i, hr);
        ok(!unknown, "Expected object returned == NULL, got %p.\n", unknown);
        hr = IDirect3DRM_CreateObject(d3drm1, tests[i].clsid, NULL, NULL, NULL);
        ok(hr == D3DRMERR_BADVALUE, "Test %u: expected hr == D3DRMERR_BADVALUE, got %#x.\n", i, hr);

        hr = IDirect3DRM_CreateObject(d3drm1, tests[i].clsid, NULL, tests[i].iid, (void **)&unknown);
        ok(SUCCEEDED(hr), "Test %u: expected hr == D3DRM_OK, got %#x.\n", i, hr);
        if (SUCCEEDED(hr))
        {
            ref2 = get_refcount((IUnknown *)d3drm1);
            if (tests[i].takes_d3drm_ref)
                ok(ref2 > ref1, "Test %u: expected ref2 > ref1, got ref1 = %u, ref2 = %u.\n", i, ref1, ref2);
            else
                ok(ref2 == ref1, "Test %u: expected ref2 == ref1, got ref1 = %u, ref2 = %u.\n", i, ref1, ref2);

            ref3 = get_refcount((IUnknown *)d3drm2);
            ok(ref3 == ref1, "Test %u: expected ref3 == ref1, got ref1 = %u, ref3 = %u.\n", i, ref1, ref3);
            ref4 = get_refcount((IUnknown *)d3drm3);
            ok(ref4 == ref1, "Test %u: expected ref4 == ref1, got ref1 = %u, ref4 = %u.\n", i, ref1, ref4);
            IUnknown_Release(unknown);
            ref2 = get_refcount((IUnknown *)d3drm1);
            ok(ref2 == ref1, "Test %u: expected ref2 == ref1, got ref1 = %u, ref2 = %u.\n", i, ref1, ref2);
            ref3 = get_refcount((IUnknown *)d3drm2);
            ok(ref3 == ref1, "Test %u: expected ref3 == ref1, got ref1 = %u, ref3 = %u.\n", i, ref1, ref3);
            ref4 = get_refcount((IUnknown *)d3drm3);
            ok(ref4 == ref1, "Test %u: expected ref4 == ref1, got ref1 = %u, ref4 = %u.\n", i, ref1, ref4);

            /* test Add/Destroy callbacks */
            test_destroy_callback(i, tests[i].clsid, tests[i].iid);

            hr = IDirect3DRM2_CreateObject(d3drm2, tests[i].clsid, NULL, tests[i].iid, (void **)&unknown);
            ok(SUCCEEDED(hr), "Test %u: expected hr == D3DRM_OK, got %#x.\n", i, hr);
            ref2 = get_refcount((IUnknown *)d3drm1);
            if (tests[i].takes_d3drm_ref)
                ok(ref2 > ref1, "Test %u: expected ref2 > ref1, got ref1 = %u, ref2 = %u.\n", i, ref1, ref2);
            else
                ok(ref2 == ref1, "Test %u: expected ref2 == ref1, got ref1 = %u, ref2 = %u.\n", i, ref1, ref2);
            ref3 = get_refcount((IUnknown *)d3drm2);
            ok(ref3 == ref1, "Test %u: expected ref3 == ref1, got ref1 = %u, ref3 = %u.\n", i, ref1, ref3);
            ref4 = get_refcount((IUnknown *)d3drm3);
            ok(ref4 == ref1, "Test %u: expected ref4 == ref1, got ref1 = %u, ref4 = %u.\n", i, ref1, ref4);
            IUnknown_Release(unknown);
            ref2 = get_refcount((IUnknown *)d3drm1);
            ok(ref2 == ref1, "Test %u: expected ref2 == ref1, got ref1 = %u, ref2 = %u.\n", i, ref1, ref2);
            ref3 = get_refcount((IUnknown *)d3drm2);
            ok(ref3 == ref1, "Test %u: expected ref3 == ref1, got ref1 = %u, ref3 = %u.\n", i, ref1, ref3);
            ref4 = get_refcount((IUnknown *)d3drm3);
            ok(ref4 == ref1, "Test %u: expected ref4 == ref1, got ref1 = %u, ref4 = %u.\n", i, ref1, ref4);

            hr = IDirect3DRM3_CreateObject(d3drm3, tests[i].clsid, NULL, tests[i].iid, (void **)&unknown);
            ok(SUCCEEDED(hr), "Test %u: expected hr == D3DRM_OK, got %#x.\n", i, hr);
            ref2 = get_refcount((IUnknown *)d3drm1);
            if (tests[i].takes_d3drm_ref)
                ok(ref2 > ref1, "Test %u: expected ref2 > ref1, got ref1 = %u, ref2 = %u.\n", i, ref1, ref2);
            else
                ok(ref2 == ref1, "Test %u: expected ref2 == ref1, got ref1 = %u, ref2 = %u.\n", i, ref1, ref2);
            ref3 = get_refcount((IUnknown *)d3drm2);
            ok(ref3 == ref1, "Test %u: expected ref3 == ref1, got ref1 = %u, ref3 = %u.\n", i, ref1, ref3);
            ref4 = get_refcount((IUnknown *)d3drm3);
            ok(ref4 == ref1, "Test %u: expected ref4 == ref1, got ref1 = %u, ref4 = %u.\n", i, ref1, ref4);
            IUnknown_Release(unknown);
            ref2 = get_refcount((IUnknown *)d3drm1);
            ok(ref2 == ref1, "Test %u: expected ref2 == ref1, got ref1 = %u, ref2 = %u.\n", i, ref1, ref2);
            ref3 = get_refcount((IUnknown *)d3drm2);
            ok(ref3 == ref1, "Test %u: expected ref3 == ref1, got ref1 = %u, ref3 = %u.\n", i, ref1, ref3);
            ref4 = get_refcount((IUnknown *)d3drm3);
            ok(ref4 == ref1, "Test %u: expected ref4 == ref1, got ref1 = %u, ref4 = %u.\n", i, ref1, ref4);
        }
    }

    IDirect3DRM_Release(d3drm1);
    IDirect3DRM2_Release(d3drm2);
    IDirect3DRM3_Release(d3drm3);
}

static void test_Viewport(void)
{
    IDirect3DRMFrame3 *frame3, *d3drm_frame3, *tmp_frame3;
    IDirect3DRMFrame *frame, *d3drm_frame, *tmp_frame1;
    float field, left, top, right, bottom, front, back;
    IDirectDrawClipper *clipper;
    HRESULT hr;
    IDirect3DRM *d3drm1;
    IDirect3DRM2 *d3drm2;
    IDirect3DRM3 *d3drm3;
    IDirect3DRMDevice *device1, *d3drm_device1;
    IDirect3DRMDevice3 *device3, *d3drm_device3;
    IDirect3DRMViewport *viewport;
    IDirect3DRMViewport2 *viewport2;
    IDirect3DViewport *d3d_viewport;
    D3DVIEWPORT vp;
    D3DVALUE expected_val;
    IDirect3DRMObject *obj, *obj2;
    GUID driver;
    HWND window;
    RECT rc;
    DWORD data, ref1, ref2, ref3, ref4;
    DWORD initial_ref1, initial_ref2, initial_ref3, device_ref, frame_ref, frame_ref2, viewport_ref;

    window = create_window();
    GetClientRect(window, &rc);

    hr = Direct3DRMCreate(&d3drm1);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);
    hr = IDirect3DRM_QueryInterface(d3drm1, &IID_IDirect3DRM2, (void **)&d3drm2);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRM2 interface (hr = %#x).\n", hr);
    hr = IDirect3DRM_QueryInterface(d3drm1, &IID_IDirect3DRM3, (void **)&d3drm3);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRM3 interface (hr = %#x).\n", hr);
    initial_ref1 = get_refcount((IUnknown *)d3drm1);
    initial_ref2 = get_refcount((IUnknown *)d3drm2);
    initial_ref3 = get_refcount((IUnknown *)d3drm3);

    hr = DirectDrawCreateClipper(0, &clipper, NULL);
    ok(hr == DD_OK, "Cannot get IDirectDrawClipper interface (hr = %x)\n", hr);

    hr = IDirectDrawClipper_SetHWnd(clipper, 0, window);
    ok(hr == DD_OK, "Cannot set HWnd to Clipper (hr = %x)\n", hr);

    memcpy(&driver, &IID_IDirect3DRGBDevice, sizeof(GUID));
    hr = IDirect3DRM3_CreateDeviceFromClipper(d3drm3, clipper, &driver, rc.right, rc.bottom, &device3);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMDevice interface (hr = %x)\n", hr);
    hr = IDirect3DRMDevice3_QueryInterface(device3, &IID_IDirect3DRMDevice, (void **)&device1);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMDevice interface (hr = %#x).\n", hr);

    hr = IDirect3DRM_CreateFrame(d3drm1, NULL, &frame);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMFrame interface (hr = %x)\n", hr);
    hr = IDirect3DRM_CreateFrame(d3drm1, NULL, &tmp_frame1);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRM3_CreateFrame(d3drm3, NULL, &frame3);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMFrame3 interface (hr = %x).\n", hr);
    hr = IDirect3DRM3_CreateFrame(d3drm3, NULL, &tmp_frame3);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);

    ref1 = get_refcount((IUnknown *)d3drm1);
    ref2 = get_refcount((IUnknown *)d3drm2);
    ref3 = get_refcount((IUnknown *)d3drm3);
    device_ref = get_refcount((IUnknown *)device1);
    frame_ref = get_refcount((IUnknown *)frame);

    hr = IDirect3DRM_CreateViewport(d3drm1, device1, frame, 0, 0, 0, 0, &viewport);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMViewport interface (hr = %#x)\n", hr);
    ref4 = get_refcount((IUnknown *)d3drm1);
    ok(ref4 > ref1, "Expected ref4 > ref1, got ref1 = %u, ref4 = %u.\n", ref1, ref4);
    ref4 = get_refcount((IUnknown *)d3drm2);
    ok(ref4 == ref2, "Expected ref4 == ref2, got ref2 = %u, ref4 = %u.\n", ref2, ref4);
    ref4 = get_refcount((IUnknown *)d3drm3);
    ok(ref4 == ref3, "Expected ref4 == ref3, got ref3 = %u, ref4 = %u.\n", ref3, ref4);
    ref4 = get_refcount((IUnknown *)device1);
    ok(ref4 == device_ref, "Expected ref4 == device_ref, got device_ref = %u, ref4 = %u.\n", device_ref, ref4);
    ref4 = get_refcount((IUnknown *)frame);
    ok(ref4 > frame_ref, "Expected ref4 > frame_ref, got frame_ref = %u, ref4 = %u.\n", frame_ref, ref4);

    hr = IDirect3DRMViewport_GetDevice(viewport, &d3drm_device1);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMDevice interface (hr = %x)\n", hr);
    ok(device1 == d3drm_device1, "Expected device returned = %p, got %p.\n", device1, d3drm_device1);
    IDirect3DRMDevice_Release(d3drm_device1);

    hr = IDirect3DRMViewport_SetCamera(viewport, NULL);
    ok(hr == D3DRMERR_BADOBJECT, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMViewport_GetCamera(viewport, &d3drm_frame);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    ok(frame == d3drm_frame, "Expected frame returned = %p, got %p.\n", frame, d3drm_frame);
    IDirect3DRMFrame_Release(d3drm_frame);

    hr = IDirect3DRMViewport_SetCamera(viewport, tmp_frame1);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMViewport_GetCamera(viewport, &d3drm_frame);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    ok(d3drm_frame == tmp_frame1, "Got unexpected frame %p, expected %p.\n", d3drm_frame, tmp_frame1);
    IDirect3DRMFrame_Release(d3drm_frame);

    IDirect3DRMViewport_Release(viewport);
    ref4 = get_refcount((IUnknown *)d3drm1);
    ok(ref4 == ref1, "Expected ref4 == ref1, got ref1 = %u, ref4 = %u.\n", ref1, ref4);
    ref4 = get_refcount((IUnknown *)d3drm2);
    ok(ref4 == ref2, "Expected ref4 == ref2, got ref2 = %u, ref4 = %u.\n", ref2, ref4);
    ref4 = get_refcount((IUnknown *)d3drm3);
    ok(ref4 == ref3, "Expected ref4 == ref3, got ref3 = %u, ref4 = %u.\n", ref3, ref4);
    ref4 = get_refcount((IUnknown *)device1);
    ok(ref4 == device_ref, "Expected ref4 == device_ref, got device_ref = %u, ref4 = %u.\n", device_ref, ref4);
    ref4 = get_refcount((IUnknown *)frame);
    ok(ref4 == frame_ref, "Expected ref4 == frame_ref, got frame_ref = %u, ref4 = %u.\n", frame_ref, ref4);

    hr = IDirect3DRM2_CreateViewport(d3drm2, device1, frame, 0, 0, 0, 0, &viewport);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMViewport interface (hr = %#x)\n", hr);
    ref4 = get_refcount((IUnknown *)d3drm1);
    ok(ref4 > ref1, "Expected ref4 > ref1, got ref1 = %u, ref4 = %u.\n", ref1, ref4);
    ref4 = get_refcount((IUnknown *)d3drm2);
    ok(ref4 == ref2, "Expected ref4 == ref2, got ref2 = %u, ref4 = %u.\n", ref2, ref4);
    ref4 = get_refcount((IUnknown *)d3drm3);
    ok(ref4 == ref3, "Expected ref4 == ref3, got ref3 = %u, ref4 = %u.\n", ref3, ref4);
    ref4 = get_refcount((IUnknown *)device1);
    ok(ref4 == device_ref, "Expected ref4 == device_ref, got device_ref = %u, ref4 = %u.\n", device_ref, ref4);
    ref4 = get_refcount((IUnknown *)frame);
    ok(ref4 > frame_ref, "Expected ref4 > frame_ref, got frame_ref = %u, ref4 = %u.\n", frame_ref, ref4);

    hr = IDirect3DRMViewport_GetDevice(viewport, &d3drm_device1);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMDevice interface (hr = %x)\n", hr);
    ok(device1 == d3drm_device1, "Expected device returned = %p, got %p.\n", device1, d3drm_device1);
    IDirect3DRMDevice_Release(d3drm_device1);

    hr = IDirect3DRMViewport_SetCamera(viewport, NULL);
    ok(hr == D3DRMERR_BADOBJECT, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMViewport_GetCamera(viewport, &d3drm_frame);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    ok(frame == d3drm_frame, "Expected frame returned = %p, got %p.\n", frame, d3drm_frame);
    IDirect3DRMFrame_Release(d3drm_frame);

    hr = IDirect3DRMViewport_SetCamera(viewport, tmp_frame1);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMViewport_GetCamera(viewport, &d3drm_frame);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    ok(d3drm_frame == tmp_frame1, "Got unexpected frame %p, expected %p.\n", d3drm_frame, tmp_frame1);
    IDirect3DRMFrame_Release(d3drm_frame);

    IDirect3DRMViewport_Release(viewport);
    ref4 = get_refcount((IUnknown *)d3drm1);
    ok(ref4 == ref1, "Expected ref4 == ref1, got ref1 = %u, ref4 = %u.\n", ref1, ref4);
    ref4 = get_refcount((IUnknown *)d3drm2);
    ok(ref4 == ref2, "Expected ref4 == ref2, got ref2 = %u, ref4 = %u.\n", ref2, ref4);
    ref4 = get_refcount((IUnknown *)d3drm3);
    ok(ref4 == ref3, "Expected ref4 == ref3, got ref3 = %u, ref4 = %u.\n", ref3, ref4);
    ref4 = get_refcount((IUnknown *)device1);
    ok(ref4 == device_ref, "Expected ref4 == device_ref, got device_ref = %u, ref4 = %u.\n", device_ref, ref4);
    ref4 = get_refcount((IUnknown *)frame);
    ok(ref4 == frame_ref, "Expected ref4 == frame_ref, got frame_ref = %u, ref4 = %u.\n", frame_ref, ref4);

    device_ref = get_refcount((IUnknown *)device3);
    frame_ref2 = get_refcount((IUnknown *)frame3);

    hr = IDirect3DRM3_CreateViewport(d3drm3, device3, frame3, 0, 0, 0, 0, &viewport2);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMViewport2 interface (hr = %#x)\n", hr);
    ref4 = get_refcount((IUnknown *)d3drm1);
    ok(ref4 > ref1, "Expected ref4 > ref1, got ref1 = %u, ref4 = %u.\n", ref1, ref4);
    ref4 = get_refcount((IUnknown *)d3drm2);
    ok(ref4 == ref2, "Expected ref4 == ref2, got ref2 = %u, ref4 = %u.\n", ref2, ref4);
    ref4 = get_refcount((IUnknown *)d3drm3);
    ok(ref4 == ref3, "Expected ref4 == ref3, got ref3 = %u, ref4 = %u.\n", ref3, ref4);
    ref4 = get_refcount((IUnknown *)device3);
    ok(ref4 == device_ref, "Expected ref4 == device_ref, got device_ref = %u, ref4 = %u.\n", device_ref, ref4);
    ref4 = get_refcount((IUnknown *)frame3);
    ok(ref4 > frame_ref2, "Expected ref4 > frame_ref2, got frame_ref2 = %u, ref4 = %u.\n", frame_ref2, ref4);

    hr = IDirect3DRMViewport2_GetDevice(viewport2, &d3drm_device3);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMDevice3 interface (hr = %x)\n", hr);
    ok(device3 == d3drm_device3, "Expected device returned = %p, got %p.\n", device3, d3drm_device3);
    IDirect3DRMDevice3_Release(d3drm_device3);

    hr = IDirect3DRMViewport2_SetCamera(viewport2, NULL);
    ok(hr == D3DRMERR_BADOBJECT, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMViewport2_GetCamera(viewport2, &d3drm_frame3);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    ok(frame3 == d3drm_frame3, "Expected frame returned = %p, got %p.\n", frame3, d3drm_frame3);
    IDirect3DRMFrame3_Release(d3drm_frame3);

    hr = IDirect3DRMViewport2_SetCamera(viewport2, tmp_frame3);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMViewport2_GetCamera(viewport2, &d3drm_frame3);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    ok(d3drm_frame3 == tmp_frame3, "Got unexpected frame %p, expected %p.\n", d3drm_frame3, tmp_frame3);
    IDirect3DRMFrame3_Release(d3drm_frame3);

    IDirect3DRMViewport2_Release(viewport2);
    ref4 = get_refcount((IUnknown *)d3drm1);
    ok(ref4 == ref1, "Expected ref4 == ref1, got ref1 = %u, ref4 = %u.\n", ref1, ref4);
    ref4 = get_refcount((IUnknown *)d3drm2);
    ok(ref4 == ref2, "Expected ref4 == ref2, got ref2 = %u, ref4 = %u.\n", ref2, ref4);
    ref4 = get_refcount((IUnknown *)d3drm3);
    ok(ref4 == ref3, "Expected ref4 == ref3, got ref3 = %u, ref4 = %u.\n", ref3, ref4);
    ref4 = get_refcount((IUnknown *)device3);
    ok(ref4 == device_ref, "Expected ref4 == device_ref, got device_ref = %u, ref4 = %u.\n", device_ref, ref4);
    ref4 = get_refcount((IUnknown *)frame3);
    ok(ref4 == frame_ref2, "Expected ref4 == frame_ref2, got frame_ref2 = %u, ref4 = %u.\n", frame_ref2, ref4);

    /* Test all failures together */
    viewport = (IDirect3DRMViewport *)0xdeadbeef;
    hr = IDirect3DRM_CreateViewport(d3drm1, NULL, frame, rc.left, rc.top, rc.right, rc.bottom, &viewport);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    ok(!viewport, "Expected viewport returned == NULL, got %p.\n", viewport);
    viewport = (IDirect3DRMViewport *)0xdeadbeef;
    hr = IDirect3DRM_CreateViewport(d3drm1, device1, NULL, rc.left, rc.top, rc.right, rc.bottom, &viewport);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    ok(!viewport, "Expected viewport returned == NULL, got %p.\n", viewport);
    viewport = (IDirect3DRMViewport *)0xdeadbeef;
    hr = IDirect3DRM_CreateViewport(d3drm1, device1, frame, rc.left, rc.top, rc.right + 1, rc.bottom + 1, &viewport);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    ok(!viewport, "Expected viewport returned == NULL, got %p.\n", viewport);
    viewport = (IDirect3DRMViewport *)0xdeadbeef;
    hr = IDirect3DRM_CreateViewport(d3drm1, device1, frame, rc.left, rc.top, rc.right + 1, rc.bottom, &viewport);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    ok(!viewport, "Expected viewport returned == NULL, got %p.\n", viewport);
    viewport = (IDirect3DRMViewport *)0xdeadbeef;
    hr = IDirect3DRM_CreateViewport(d3drm1, device1, frame, rc.left, rc.top, rc.right, rc.bottom + 1, &viewport);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    ok(!viewport, "Expected viewport returned == NULL, got %p.\n", viewport);
    hr = IDirect3DRM_CreateViewport(d3drm1, device1, frame, rc.left, rc.top, rc.right, rc.bottom, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);

    viewport = (IDirect3DRMViewport *)0xdeadbeef;
    hr = IDirect3DRM2_CreateViewport(d3drm2, NULL, frame, rc.left, rc.top, rc.right, rc.bottom, &viewport);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    ok(!viewport, "Expected viewport returned == NULL, got %p.\n", viewport);
    viewport = (IDirect3DRMViewport *)0xdeadbeef;
    hr = IDirect3DRM2_CreateViewport(d3drm2, device1, NULL, rc.left, rc.top, rc.right, rc.bottom, &viewport);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    ok(!viewport, "Expected viewport returned == NULL, got %p.\n", viewport);
    viewport = (IDirect3DRMViewport *)0xdeadbeef;
    hr = IDirect3DRM2_CreateViewport(d3drm2, device1, frame, rc.left, rc.top, rc.right + 1, rc.bottom + 1, &viewport);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    ok(!viewport, "Expected viewport returned == NULL, got %p.\n", viewport);
    viewport = (IDirect3DRMViewport *)0xdeadbeef;
    hr = IDirect3DRM2_CreateViewport(d3drm2, device1, frame, rc.left, rc.top, rc.right + 1, rc.bottom, &viewport);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    ok(!viewport, "Expected viewport returned == NULL, got %p.\n", viewport);
    viewport = (IDirect3DRMViewport *)0xdeadbeef;
    hr = IDirect3DRM2_CreateViewport(d3drm2, device1, frame, rc.left, rc.top, rc.right, rc.bottom + 1, &viewport);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    ok(!viewport, "Expected viewport returned == NULL, got %p.\n", viewport);
    hr = IDirect3DRM2_CreateViewport(d3drm2, device1, frame, rc.left, rc.top, rc.right, rc.bottom, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);

    viewport2 = (IDirect3DRMViewport2 *)0xdeadbeef;
    hr = IDirect3DRM3_CreateViewport(d3drm3, NULL, frame3, rc.left, rc.top, rc.right, rc.bottom, &viewport2);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    ok(!viewport2, "Expected viewport returned == NULL, got %p.\n", viewport2);
    viewport2 = (IDirect3DRMViewport2 *)0xdeadbeef;
    hr = IDirect3DRM3_CreateViewport(d3drm3, device3, NULL, rc.left, rc.top, rc.right, rc.bottom, &viewport2);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    ok(!viewport2, "Expected viewport returned == NULL, got %p.\n", viewport2);
    viewport2 = (IDirect3DRMViewport2 *)0xdeadbeef;
    hr = IDirect3DRM3_CreateViewport(d3drm3, device3, frame3, rc.left, rc.top, rc.right + 1, rc.bottom + 1, &viewport2);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    ok(!viewport2, "Expected viewport returned == NULL, got %p.\n", viewport2);
    viewport2 = (IDirect3DRMViewport2 *)0xdeadbeef;
    hr = IDirect3DRM3_CreateViewport(d3drm3, device3, frame3, rc.left, rc.top, rc.right + 1, rc.bottom, &viewport2);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    ok(!viewport2, "Expected viewport returned == NULL, got %p.\n", viewport2);
    viewport2 = (IDirect3DRMViewport2 *)0xdeadbeef;
    hr = IDirect3DRM3_CreateViewport(d3drm3, device3, frame3, rc.left, rc.top, rc.right, rc.bottom + 1, &viewport2);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    ok(!viewport2, "Expected viewport returned == NULL, got %p.\n", viewport2);
    hr = IDirect3DRM3_CreateViewport(d3drm3, device3, frame3, rc.left, rc.top, rc.right, rc.bottom, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);

    hr = IDirect3DRM2_CreateViewport(d3drm2, device1, frame, rc.left, rc.top, rc.right, rc.bottom, &viewport);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMViewport interface (hr = %#x)\n", hr);
    hr = IDirect3DRMViewport_GetDirect3DViewport(viewport, &d3d_viewport);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DViewport interface (hr = %#x).\n", hr);
    viewport_ref = get_refcount((IUnknown *)d3d_viewport);
    hr = IDirect3DRMViewport_GetDirect3DViewport(viewport, &d3d_viewport);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DViewport interface (hr = %#x).\n", hr);
    ref4 = get_refcount((IUnknown *)d3d_viewport);
    ok(ref4 > viewport_ref, "Expected ref4 > viewport_ref, got ref4 = %u, viewport_ref = %u.\n", ref4, viewport_ref);
    IDirect3DViewport_Release(d3d_viewport);
    ref4 = get_refcount((IUnknown *)d3d_viewport);
    ok(ref4 == viewport_ref, "Expected ref4 == viewport_ref, got ref4 = %u, viewport_ref = %u.\n", ref4, viewport_ref);
    IDirect3DViewport_Release(d3d_viewport);

    hr = IDirect3DRMViewport_GetDirect3DViewport(viewport, &d3d_viewport);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DViewport interface (hr = %#x).\n", hr);
    vp.dwSize = sizeof(vp);
    hr = IDirect3DViewport_GetViewport(d3d_viewport, &vp);
    ok(SUCCEEDED(hr), "Cannot get D3DVIEWPORT struct (hr = %#x).\n", hr);
    ok(vp.dwWidth == rc.right, "Expected viewport width = %u, got %u.\n", rc.right, vp.dwWidth);
    ok(vp.dwHeight == rc.bottom, "Expected viewport height = %u, got %u.\n", rc.bottom, vp.dwHeight);
    ok(vp.dwX == rc.left, "Expected viewport X position = %u, got %u.\n", rc.left, vp.dwX);
    ok(vp.dwY == rc.top, "Expected viewport Y position = %u, got %u.\n", rc.top, vp.dwY);
    expected_val = (rc.right > rc.bottom) ? (rc.right / 2.0f) : (rc.bottom / 2.0f);
    ok(vp.dvScaleX == expected_val, "Expected dvScaleX = %f, got %f.\n", expected_val, vp.dvScaleX);
    ok(vp.dvScaleY == expected_val, "Expected dvScaleY = %f, got %f.\n", expected_val, vp.dvScaleY);
    expected_val = vp.dwWidth / (2.0f * vp.dvScaleX);
    ok(vp.dvMaxX == expected_val, "Expected dvMaxX = %f, got %f.\n", expected_val, vp.dvMaxX);
    expected_val = vp.dwHeight / (2.0f * vp.dvScaleY);
    ok(vp.dvMaxY == expected_val, "Expected dvMaxY = %f, got %f.\n", expected_val, vp.dvMaxY);
    IDirect3DViewport_Release(d3d_viewport);
    IDirect3DRMViewport_Release(viewport);

    hr = IDirect3DRM3_CreateViewport(d3drm3, device3, frame3, rc.left, rc.top, rc.right, rc.bottom, &viewport2);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMViewport2 interface (hr = %#x)\n", hr);
    hr = IDirect3DRMViewport2_GetDirect3DViewport(viewport2, &d3d_viewport);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DViewport interface (hr = %#x).\n", hr);
    viewport_ref = get_refcount((IUnknown *)d3d_viewport);
    hr = IDirect3DRMViewport2_GetDirect3DViewport(viewport2, &d3d_viewport);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DViewport interface (hr = %#x).\n", hr);
    ref4 = get_refcount((IUnknown *)d3d_viewport);
    ok(ref4 > viewport_ref, "Expected ref4 > viewport_ref, got ref4 = %u, viewport_ref = %u.\n", ref4, viewport_ref);
    IDirect3DViewport_Release(d3d_viewport);
    ref4 = get_refcount((IUnknown *)d3d_viewport);
    ok(ref4 == viewport_ref, "Expected ref4 == viewport_ref, got ref4 = %u, viewport_ref = %u.\n", ref4, viewport_ref);
    IDirect3DViewport_Release(d3d_viewport);

    hr = IDirect3DRMViewport2_GetDirect3DViewport(viewport2, &d3d_viewport);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DViewport interface (hr = %#x).\n", hr);
    vp.dwSize = sizeof(vp);
    hr = IDirect3DViewport_GetViewport(d3d_viewport, &vp);
    ok(SUCCEEDED(hr), "Cannot get D3DVIEWPORT struct (hr = %#x).\n", hr);
    ok(vp.dwWidth == rc.right, "Expected viewport width = %u, got %u.\n", rc.right, vp.dwWidth);
    ok(vp.dwHeight == rc.bottom, "Expected viewport height = %u, got %u.\n", rc.bottom, vp.dwHeight);
    ok(vp.dwX == rc.left, "Expected viewport X position = %u, got %u.\n", rc.left, vp.dwX);
    ok(vp.dwY == rc.top, "Expected viewport Y position = %u, got %u.\n", rc.top, vp.dwY);
    expected_val = (rc.right > rc.bottom) ? (rc.right / 2.0f) : (rc.bottom / 2.0f);
    ok(vp.dvScaleX == expected_val, "Expected dvScaleX = %f, got %f.\n", expected_val, vp.dvScaleX);
    ok(vp.dvScaleY == expected_val, "Expected dvScaleY = %f, got %f.\n", expected_val, vp.dvScaleY);
    expected_val = vp.dwWidth / (2.0f * vp.dvScaleX);
    ok(vp.dvMaxX == expected_val, "Expected dvMaxX = %f, got %f.\n", expected_val, vp.dvMaxX);
    expected_val = vp.dwHeight / (2.0f * vp.dvScaleY);
    ok(vp.dvMaxY == expected_val, "Expected dvMaxY = %f, got %f.\n", expected_val, vp.dvMaxY);
    IDirect3DViewport_Release(d3d_viewport);
    IDirect3DRMViewport2_Release(viewport2);

    hr = IDirect3DRM_CreateViewport(d3drm1, device1, frame, rc.left, rc.top, rc.right, rc.bottom, &viewport);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMViewport interface (hr = %x)\n", hr);
    hr = IDirect3DRMViewport_GetDirect3DViewport(viewport, &d3d_viewport);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DViewport interface (hr = %#x).\n", hr);
    viewport_ref = get_refcount((IUnknown *)d3d_viewport);
    hr = IDirect3DRMViewport_GetDirect3DViewport(viewport, &d3d_viewport);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DViewport interface (hr = %#x).\n", hr);
    ref4 = get_refcount((IUnknown *)d3d_viewport);
    ok(ref4 > viewport_ref, "Expected ref4 > viewport_ref, got ref4 = %u, viewport_ref = %u.\n", ref4, viewport_ref);
    IDirect3DViewport_Release(d3d_viewport);
    ref4 = get_refcount((IUnknown *)d3d_viewport);
    ok(ref4 == viewport_ref, "Expected ref4 == viewport_ref, got ref4 = %u, viewport_ref = %u.\n", ref4, viewport_ref);
    IDirect3DViewport_Release(d3d_viewport);

    hr = IDirect3DRMViewport_GetDirect3DViewport(viewport, &d3d_viewport);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DViewport interface (hr = %#x).\n", hr);
    vp.dwSize = sizeof(vp);
    hr = IDirect3DViewport_GetViewport(d3d_viewport, &vp);
    ok(SUCCEEDED(hr), "Cannot get D3DVIEWPORT struct (hr = %#x).\n", hr);
    ok(vp.dwWidth == rc.right, "Expected viewport width = %u, got %u.\n", rc.right, vp.dwWidth);
    ok(vp.dwHeight == rc.bottom, "Expected viewport height = %u, got %u.\n", rc.bottom, vp.dwHeight);
    ok(vp.dwX == rc.left, "Expected viewport X position = %u, got %u.\n", rc.left, vp.dwX);
    ok(vp.dwY == rc.top, "Expected viewport Y position = %u, got %u.\n", rc.top, vp.dwY);
    expected_val = (rc.right > rc.bottom) ? (rc.right / 2.0f) : (rc.bottom / 2.0f);
    ok(vp.dvScaleX == expected_val, "Expected dvScaleX = %f, got %f.\n", expected_val, vp.dvScaleX);
    ok(vp.dvScaleY == expected_val, "Expected dvScaleY = %f, got %f.\n", expected_val, vp.dvScaleY);
    expected_val = vp.dwWidth / (2.0f * vp.dvScaleX);
    ok(vp.dvMaxX == expected_val, "Expected dvMaxX = %f, got %f.\n", expected_val, vp.dvMaxX);
    expected_val = vp.dwHeight / (2.0f * vp.dvScaleY);
    ok(vp.dvMaxY == expected_val, "Expected dvMaxY = %f, got %f.\n", expected_val, vp.dvMaxY);
    IDirect3DViewport_Release(d3d_viewport);

    hr = IDirect3DRMViewport_QueryInterface(viewport, &IID_IDirect3DRMObject, (void**)&obj);
    ok(hr == D3DRM_OK, "expected D3DRM_OK (hr = %x)\n", hr);
    ok((IDirect3DRMObject*)viewport == obj, "got object pointer %p, expected %p\n", obj, viewport);

    hr = IDirect3DRMViewport_QueryInterface(viewport, &IID_IDirect3DRMViewport2, (void**)&viewport2);
    ok(hr == D3DRM_OK, "expected D3DRM_OK (hr = %x)\n", hr);

    hr = IDirect3DRMViewport2_QueryInterface(viewport2, &IID_IDirect3DRMObject, (void**)&obj2);
    ok(hr == D3DRM_OK, "expected D3DRM_OK (hr = %x)\n", hr);
    ok(obj == obj2, "got object pointer %p, expected %p\n", obj2, obj);
    ok((IUnknown*)viewport != (IUnknown*)viewport2, "got viewport1 %p, viewport2 %p\n", viewport, viewport2);

    IDirect3DRMViewport2_Release(viewport2);
    IDirect3DRMObject_Release(obj);
    IDirect3DRMObject_Release(obj2);

    test_class_name((IDirect3DRMObject *)viewport, "Viewport");
    test_object_name((IDirect3DRMObject *)viewport);

    /* AppData */
    hr = IDirect3DRMViewport_SetAppData(viewport, 0);
    ok(hr == D3DRM_OK, "expected D3DRM_OK (hr = %x)\n", hr);

    hr = IDirect3DRMViewport_SetAppData(viewport, 0);
    ok(hr == D3DRM_OK, "expected D3DRM_OK (hr = %x)\n", hr);

    hr = IDirect3DRMViewport_SetAppData(viewport, 1);
    ok(hr == D3DRM_OK, "expected D3DRM_OK (hr = %x)\n", hr);

    hr = IDirect3DRMViewport_SetAppData(viewport, 1);
    ok(hr == D3DRM_OK, "expected D3DRM_OK (hr = %x)\n", hr);

    hr = IDirect3DRMViewport_QueryInterface(viewport, &IID_IDirect3DRMViewport2, (void**)&viewport2);
    ok(hr == D3DRM_OK, "expected D3DRM_OK (hr = %x)\n", hr);

    data = IDirect3DRMViewport2_GetAppData(viewport2);
    ok(data == 1, "got %x\n", data);
    IDirect3DRMViewport2_Release(viewport2);
    IDirect3DRMViewport_Release(viewport);

    /* IDirect3DRMViewport*::Init tests */
    ref1 = get_refcount((IUnknown *)d3drm1);
    ref2 = get_refcount((IUnknown *)d3drm2);
    ref3 = get_refcount((IUnknown *)d3drm3);
    hr = IDirect3DRM_CreateObject(d3drm1, &CLSID_CDirect3DRMViewport, NULL, &IID_IDirect3DRMViewport,
            (void **)&viewport);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMViewport interface (hr = %#x).\n", hr);
    ref4 = get_refcount((IUnknown *)d3drm1);
    ok(ref4 == ref1, "Expected ref4 == ref1, got ref1 = %u, ref4 = %u.\n", ref1, ref4);
    ref4 = get_refcount((IUnknown *)d3drm2);
    ok(ref4 == ref2, "Expected ref4 == ref2, got ref2 = %u, ref4 = %u.\n", ref2, ref4);
    ref4 = get_refcount((IUnknown *)d3drm3);
    ok(ref4 == ref3, "Expected ref4 == ref3, got ref3 = %u, ref4 = %u.\n", ref3, ref4);

    hr = IDirect3DRMViewport_GetDirect3DViewport(viewport, &d3d_viewport);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    hr = IDirect3DRMViewport_GetDevice(viewport, &d3drm_device1);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    hr = IDirect3DRMViewport_GetCamera(viewport, &d3drm_frame);
    ok(hr == D3DRMERR_BADOBJECT, "Got unexpected hr %#x.\n", hr);
    field = IDirect3DRMViewport_GetField(viewport);
    ok(field == -1.0f, "Got unexpected field %.8e.\n", field);
    left = right = bottom = top = 10.0f;
    hr = IDirect3DRMViewport_GetPlane(viewport, &left, &right, &bottom, &top);
    ok(hr == D3DRMERR_BADOBJECT, "Got unexpected hr %#x.\n", hr);
    ok(left == 10.0f, "Got unexpected left %.8e.\n", left);
    ok(right == 10.0f, "Got unexpected right %.8e.\n", right);
    ok(bottom == 10.0f, "Got unexpected bottom %.8e.\n", bottom);
    ok(top == 10.0f, "Got unexpected top %.8e.\n", top);
    front = IDirect3DRMViewport_GetFront(viewport);
    ok(front == -1.0f, "Got unexpected front %.8e\n", front);
    back = IDirect3DRMViewport_GetBack(viewport);
    ok(back == -1.0f, "Got unexpected back %.8e\n", back);

    hr = IDirect3DRMViewport_SetCamera(viewport, frame);
    ok(hr == D3DRMERR_BADOBJECT, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMViewport_SetField(viewport, 0.5f);
    ok(hr == D3DRMERR_BADOBJECT, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMViewport_SetPlane(viewport, -0.5f, 0.5f, -0.5f, 0.5f);
    ok(hr == D3DRMERR_BADOBJECT, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMViewport_SetFront(viewport, 1.0f);
    ok(hr == D3DRMERR_BADOBJECT, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMViewport_SetBack(viewport, 100.0f);
    ok(hr == D3DRMERR_BADOBJECT, "Got unexpected hr %#x.\n", hr);

    /* Test all failures together */
    hr = IDirect3DRMViewport_Init(viewport, NULL, frame, rc.left, rc.top, rc.right, rc.bottom);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    hr = IDirect3DRMViewport_Init(viewport, device1, NULL, rc.left, rc.top, rc.right, rc.bottom);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    hr = IDirect3DRMViewport_Init(viewport, device1, frame, rc.left, rc.top, rc.right + 1, rc.bottom + 1);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    hr = IDirect3DRMViewport_Init(viewport, device1, frame, rc.left, rc.top, rc.right + 1, rc.bottom);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    hr = IDirect3DRMViewport_Init(viewport, device1, frame, rc.left, rc.top, rc.right, rc.bottom + 1);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);

    device_ref = get_refcount((IUnknown *)device1);
    frame_ref = get_refcount((IUnknown *)frame);
    hr = IDirect3DRMViewport_Init(viewport, device1, frame, rc.left, rc.top, rc.right, rc.bottom);
    ok(SUCCEEDED(hr), "Cannot initialize IDirect3DRMViewport interface (hr = %#x).\n", hr);
    ref4 = get_refcount((IUnknown *)d3drm1);
    ok(ref4 > ref1, "Expected ref4 > ref1, got ref1 = %u, ref4 = %u.\n", ref1, ref4);
    ref4 = get_refcount((IUnknown *)d3drm2);
    ok(ref4 == ref2, "Expected ref4 == ref2, got ref2 = %u, ref4 = %u.\n", ref2, ref4);
    ref4 = get_refcount((IUnknown *)d3drm3);
    ok(ref4 == ref3, "Expected ref4 == ref3, got ref3 = %u, ref4 = %u.\n", ref3, ref4);
    ref4 = get_refcount((IUnknown *)device1);
    ok(ref4 == device_ref, "Expected ref4 == device_ref, got device_ref = %u, ref4 = %u.\n", device_ref, ref4);
    ref4 = get_refcount((IUnknown *)frame);
    ok(ref4 > frame_ref, "Expected ref4 > frame_ref, got frame_ref = %u, ref4 = %u.\n", frame_ref, ref4);

    hr = IDirect3DRMViewport_GetDevice(viewport, &d3drm_device1);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMDevice3 interface (hr = %x)\n", hr);
    ok(device1 == d3drm_device1, "Expected device returned = %p, got %p.\n", device3, d3drm_device3);
    IDirect3DRMDevice_Release(d3drm_device1);

    hr = IDirect3DRMViewport_GetDirect3DViewport(viewport, &d3d_viewport);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DViewport interface (hr = %#x).\n", hr);
    viewport_ref = get_refcount((IUnknown *)d3d_viewport);
    hr = IDirect3DRMViewport_GetDirect3DViewport(viewport, &d3d_viewport);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DViewport interface (hr = %#x).\n", hr);
    ref4 = get_refcount((IUnknown *)d3d_viewport);
    ok(ref4 > viewport_ref, "Expected ref4 > viewport_ref, got ref4 = %u, viewport_ref = %u.\n", ref4, viewport_ref);
    IDirect3DViewport_Release(d3d_viewport);
    ref4 = get_refcount((IUnknown *)d3d_viewport);
    ok(ref4 == viewport_ref, "Expected ref4 == viewport_ref, got ref4 = %u, viewport_ref = %u.\n", ref4, viewport_ref);
    IDirect3DViewport_Release(d3d_viewport);

    hr = IDirect3DRMViewport_GetDirect3DViewport(viewport, &d3d_viewport);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DViewport interface (hr = %#x).\n", hr);
    vp.dwSize = sizeof(vp);
    hr = IDirect3DViewport_GetViewport(d3d_viewport, &vp);
    ok(SUCCEEDED(hr), "Cannot get D3DVIEWPORT struct (hr = %#x).\n", hr);
    ok(vp.dwWidth == rc.right, "Expected viewport width = %u, got %u.\n", rc.right, vp.dwWidth);
    ok(vp.dwHeight == rc.bottom, "Expected viewport height = %u, got %u.\n", rc.bottom, vp.dwHeight);
    ok(vp.dwX == rc.left, "Expected viewport X position = %u, got %u.\n", rc.left, vp.dwX);
    ok(vp.dwY == rc.top, "Expected viewport Y position = %u, got %u.\n", rc.top, vp.dwY);
    expected_val = (rc.right > rc.bottom) ? (rc.right / 2.0f) : (rc.bottom / 2.0f);
    ok(vp.dvScaleX == expected_val, "Expected dvScaleX = %f, got %f.\n", expected_val, vp.dvScaleX);
    ok(vp.dvScaleY == expected_val, "Expected dvScaleY = %f, got %f.\n", expected_val, vp.dvScaleY);
    expected_val = vp.dwWidth / (2.0f * vp.dvScaleX);
    ok(vp.dvMaxX == expected_val, "Expected dvMaxX = %f, got %f.\n", expected_val, vp.dvMaxX);
    expected_val = vp.dwHeight / (2.0f * vp.dvScaleY);
    ok(vp.dvMaxY == expected_val, "Expected dvMaxY = %f, got %f.\n", expected_val, vp.dvMaxY);
    IDirect3DViewport_Release(d3d_viewport);

    field = IDirect3DRMViewport_GetField(viewport);
    ok(field == 0.5f, "Got unexpected field %.8e.\n", field);
    hr = IDirect3DRMViewport_GetPlane(viewport, &left, &right, &bottom, &top);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    ok(left == -0.5f, "Got unexpected left %.8e.\n", left);
    ok(right == 0.5f, "Got unexpected right %.8e.\n", right);
    ok(bottom == -0.5f, "Got unexpected bottom %.8e.\n", bottom);
    ok(top == 0.5f, "Got unexpected top %.8e.\n", top);
    front = IDirect3DRMViewport_GetFront(viewport);
    ok(front == 1.0f, "Got unexpected front %.8e.\n", front);
    back = IDirect3DRMViewport_GetBack(viewport);
    ok(back == 100.0f, "Got unexpected back %.8e.\n", back);

    hr = IDirect3DRMViewport_SetField(viewport, 1.0f);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    field = IDirect3DRMViewport_GetField(viewport);
    ok(field == 1.0f, "Got unexpected field %.8e.\n", field);
    hr = IDirect3DRMViewport_GetPlane(viewport, &left, &right, &bottom, &top);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    ok(left == -1.0f, "Got unexpected left %.8e.\n", left);
    ok(right == 1.0f, "Got unexpected right %.8e.\n", right);
    ok(bottom == -1.0f, "Got unexpected bottom %.8e.\n", bottom);
    ok(top == 1.0f, "Got unexpected top %.8e.\n", top);

    hr = IDirect3DRMViewport_SetPlane(viewport, 5.0f, 3.0f, 2.0f, 0.0f);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    field = IDirect3DRMViewport_GetField(viewport);
    ok(field == -1.0f, "Got unexpected field %.8e.\n", field);
    hr = IDirect3DRMViewport_GetPlane(viewport, &left, &right, &bottom, &top);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    ok(left == 5.0f, "Got unexpected left %.8e.\n", left);
    ok(right == 3.0f, "Got unexpected right %.8e.\n", right);
    ok(bottom == 2.0f, "Got unexpected bottom %.8e.\n", bottom);
    ok(top == 0.0f, "Got unexpected top %.8e.\n", top);
    hr = IDirect3DRMViewport_SetFront(viewport, 2.0f);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    front = IDirect3DRMViewport_GetFront(viewport);
    ok(front == 2.0f, "Got unexpected front %.8e.\n", front);
    hr = IDirect3DRMViewport_SetBack(viewport, 200.0f);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    back = IDirect3DRMViewport_GetBack(viewport);
    ok(back == 200.0f, "Got unexpected back %.8e.\n", back);

    hr = IDirect3DRMViewport_Init(viewport, device1, frame, rc.left, rc.top, rc.right, rc.bottom);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    hr = IDirect3DRMViewport_GetDevice(viewport, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    hr = IDirect3DRMViewport_GetDirect3DViewport(viewport, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    hr = IDirect3DRMViewport_GetCamera(viewport, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMViewport_SetField(viewport, 0.0f);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    hr = IDirect3DRMViewport_SetField(viewport, -1.0f);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    hr = IDirect3DRMViewport_SetFront(viewport, 0.0f);
    ok(hr == D3DRMERR_BADVALUE, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMViewport_SetFront(viewport, -1.0f);
    ok(hr == D3DRMERR_BADVALUE, "Got unexpected hr %#x.\n", hr);
    front = IDirect3DRMViewport_GetFront(viewport);
    hr = IDirect3DRMViewport_SetBack(viewport, front);
    ok(hr == D3DRMERR_BADVALUE, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMViewport_SetBack(viewport, front / 2.0f);
    ok(hr == D3DRMERR_BADVALUE, "Got unexpected hr %#x.\n", hr);

    IDirect3DRMViewport_Release(viewport);
    ref4 = get_refcount((IUnknown *)d3drm1);
    todo_wine ok(ref4 > ref1, "Expected ref4 > ref1, got ref1 = %u, ref4 = %u.\n", ref1, ref4);
    ref4 = get_refcount((IUnknown *)d3drm2);
    ok(ref4 == ref2, "Expected ref4 == ref2, got ref2 = %u, ref4 = %u.\n", ref2, ref4);
    ref4 = get_refcount((IUnknown *)d3drm3);
    ok(ref4 == ref3, "Expected ref4 == ref3, got ref3 = %u, ref4 = %u.\n", ref3, ref4);
    ref4 = get_refcount((IUnknown *)device1);
    ok(ref4 == device_ref, "Expected ref4 == device_ref, got device_ref = %u, ref4 = %u.\n", device_ref, ref4);
    ref4 = get_refcount((IUnknown *)frame);
    todo_wine ok(ref4 > frame_ref, "Expected ref4 > frame_ref, got frame_ref = %u, ref4 = %u.\n", frame_ref, ref4);

    ref1 = get_refcount((IUnknown *)d3drm1);
    ref2 = get_refcount((IUnknown *)d3drm2);
    ref3 = get_refcount((IUnknown *)d3drm3);
    hr = IDirect3DRM3_CreateObject(d3drm2, &CLSID_CDirect3DRMViewport, NULL, &IID_IDirect3DRMViewport2,
            (void **)&viewport2);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMViewport2 interface (hr = %#x).\n", hr);
    ref4 = get_refcount((IUnknown *)d3drm1);
    ok(ref4 == ref1, "Expected ref4 == ref1, got ref1 = %u, ref4 = %u.\n", ref1, ref4);
    ref4 = get_refcount((IUnknown *)d3drm2);
    ok(ref4 == ref2, "Expected ref4 == ref2, got ref2 = %u, ref4 = %u.\n", ref2, ref4);
    ref4 = get_refcount((IUnknown *)d3drm3);
    ok(ref4 == ref3, "Expected ref4 == ref3, got ref3 = %u, ref4 = %u.\n", ref3, ref4);

    hr = IDirect3DRMViewport2_GetDirect3DViewport(viewport2, &d3d_viewport);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    hr = IDirect3DRMViewport2_GetDevice(viewport2, &d3drm_device3);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    hr = IDirect3DRMViewport2_GetCamera(viewport2, &d3drm_frame3);
    ok(hr == D3DRMERR_BADOBJECT, "Got unexpected hr %#x.\n", hr);
    field = IDirect3DRMViewport2_GetField(viewport2);
    ok(field == -1.0f, "Got unexpected field %.8e.\n", field);
    left = right = bottom = top = 10.0f;
    hr = IDirect3DRMViewport2_GetPlane(viewport2, &left, &right, &bottom, &top);
    ok(hr == D3DRMERR_BADOBJECT, "Got unexpected hr %#x.\n", hr);
    ok(left == 10.0f, "Got unexpected left %.8e.\n", left);
    ok(right == 10.0f, "Got unexpected right %.8e.\n", right);
    ok(bottom == 10.0f, "Got unexpected bottom %.8e.\n", bottom);
    ok(top == 10.0f, "Got unexpected top %.8e.\n", top);
    front = IDirect3DRMViewport2_GetFront(viewport2);
    ok(front == -1.0f, "Got unexpected front %.8e\n", front);
    back = IDirect3DRMViewport2_GetBack(viewport2);
    ok(back == -1.0f, "Got unexpected back %.8e\n", back);

    hr = IDirect3DRMViewport2_SetCamera(viewport2, frame3);
    ok(hr == D3DRMERR_BADOBJECT, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMViewport2_SetField(viewport2, 0.5f);
    ok(hr == D3DRMERR_BADOBJECT, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMViewport2_SetPlane(viewport2, -0.5f, 0.5f, -0.5f, 0.5f);
    ok(hr == D3DRMERR_BADOBJECT, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMViewport2_SetFront(viewport2, 1.0f);
    ok(hr == D3DRMERR_BADOBJECT, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMViewport2_SetBack(viewport2, 100.0f);
    ok(hr == D3DRMERR_BADOBJECT, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DRMViewport2_Init(viewport2, NULL, frame3, rc.left, rc.top, rc.right, rc.bottom);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    hr = IDirect3DRMViewport2_Init(viewport2, device3, NULL, rc.left, rc.top, rc.right, rc.bottom);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    hr = IDirect3DRMViewport2_Init(viewport2, device3, frame3, rc.left, rc.top, rc.right + 1, rc.bottom + 1);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    hr = IDirect3DRMViewport2_Init(viewport2, device3, frame3, rc.left, rc.top, rc.right + 1, rc.bottom);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    hr = IDirect3DRMViewport2_Init(viewport2, device3, frame3, rc.left, rc.top, rc.right, rc.bottom + 1);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);

    device_ref = get_refcount((IUnknown *)device3);
    frame_ref2 = get_refcount((IUnknown *)frame3);
    hr = IDirect3DRMViewport2_Init(viewport2, device3, frame3, rc.left, rc.top, rc.right, rc.bottom);
    ok(SUCCEEDED(hr), "Cannot initialize IDirect3DRMViewport2 interface (hr = %#x).\n", hr);
    ref4 = get_refcount((IUnknown *)device3);
    ok(ref4 == device_ref, "Expected ref4 == device_ref, got device_ref = %u, ref4 = %u.\n", device_ref, ref4);
    ref4 = get_refcount((IUnknown *)frame3);
    ok(ref4 > frame_ref2, "Expected ref4 > frame_ref2, got frame_ref2 = %u, ref4 = %u.\n", frame_ref2, ref4);

    hr = IDirect3DRMViewport2_GetDevice(viewport2, &d3drm_device3);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMDevice3 interface (hr = %x)\n", hr);
    ok(device3 == d3drm_device3, "Expected device returned = %p, got %p.\n", device3, d3drm_device3);
    IDirect3DRMDevice3_Release(d3drm_device3);

    hr = IDirect3DRMViewport2_GetDirect3DViewport(viewport2, &d3d_viewport);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DViewport interface (hr = %#x).\n", hr);
    viewport_ref = get_refcount((IUnknown *)d3d_viewport);
    hr = IDirect3DRMViewport2_GetDirect3DViewport(viewport2, &d3d_viewport);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DViewport interface (hr = %#x).\n", hr);
    ref4 = get_refcount((IUnknown *)d3d_viewport);
    ok(ref4 > viewport_ref, "Expected ref4 > viewport_ref, got ref4 = %u, viewport_ref = %u.\n", ref4, viewport_ref);
    IDirect3DViewport_Release(d3d_viewport);
    ref4 = get_refcount((IUnknown *)d3d_viewport);
    ok(ref4 == viewport_ref, "Expected ref4 == viewport_ref, got ref4 = %u, viewport_ref = %u.\n", ref4, viewport_ref);
    IDirect3DViewport_Release(d3d_viewport);

    hr = IDirect3DRMViewport2_GetDirect3DViewport(viewport2, &d3d_viewport);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DViewport interface (hr = %#x).\n", hr);
    vp.dwSize = sizeof(vp);
    hr = IDirect3DViewport_GetViewport(d3d_viewport, &vp);
    ok(SUCCEEDED(hr), "Cannot get D3DVIEWPORT struct (hr = %#x).\n", hr);
    ok(vp.dwWidth == rc.right, "Expected viewport width = %u, got %u.\n", rc.right, vp.dwWidth);
    ok(vp.dwHeight == rc.bottom, "Expected viewport height = %u, got %u.\n", rc.bottom, vp.dwHeight);
    ok(vp.dwX == rc.left, "Expected viewport X position = %u, got %u.\n", rc.left, vp.dwX);
    ok(vp.dwY == rc.top, "Expected viewport Y position = %u, got %u.\n", rc.top, vp.dwY);
    expected_val = (rc.right > rc.bottom) ? (rc.right / 2.0f) : (rc.bottom / 2.0f);
    ok(vp.dvScaleX == expected_val, "Expected dvScaleX = %f, got %f.\n", expected_val, vp.dvScaleX);
    ok(vp.dvScaleY == expected_val, "Expected dvScaleY = %f, got %f.\n", expected_val, vp.dvScaleY);
    expected_val = vp.dwWidth / (2.0f * vp.dvScaleX);
    ok(vp.dvMaxX == expected_val, "Expected dvMaxX = %f, got %f.\n", expected_val, vp.dvMaxX);
    expected_val = vp.dwHeight / (2.0f * vp.dvScaleY);
    ok(vp.dvMaxY == expected_val, "Expected dvMaxY = %f, got %f.\n", expected_val, vp.dvMaxY);
    IDirect3DViewport_Release(d3d_viewport);

    field = IDirect3DRMViewport2_GetField(viewport2);
    ok(field == 0.5f, "Got unexpected field %.8e.\n", field);
    hr = IDirect3DRMViewport2_GetPlane(viewport2, &left, &right, &bottom, &top);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    ok(left == -0.5f, "Got unexpected left %.8e.\n", left);
    ok(right == 0.5f, "Got unexpected right %.8e.\n", right);
    ok(bottom == -0.5f, "Got unexpected bottom %.8e.\n", bottom);
    ok(top == 0.5f, "Got unexpected top %.8e.\n", top);
    front = IDirect3DRMViewport2_GetFront(viewport2);
    ok(front == 1.0f, "Got unexpected front %.8e.\n", front);
    back = IDirect3DRMViewport2_GetBack(viewport2);
    ok(back == 100.0f, "Got unexpected back %.8e.\n", back);

    hr = IDirect3DRMViewport2_SetField(viewport2, 1.0f);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    field = IDirect3DRMViewport2_GetField(viewport2);
    ok(field == 1.0f, "Got unexpected field %.8e.\n", field);
    hr = IDirect3DRMViewport2_GetPlane(viewport2, &left, &right, &bottom, &top);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    ok(left == -1.0f, "Got unexpected left %.8e.\n", left);
    ok(right == 1.0f, "Got unexpected right %.8e.\n", right);
    ok(bottom == -1.0f, "Got unexpected bottom %.8e.\n", bottom);
    ok(top == 1.0f, "Got unexpected top %.8e.\n", top);

    hr = IDirect3DRMViewport2_SetPlane(viewport2, 5.0f, 3.0f, 2.0f, 0.0f);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    field = IDirect3DRMViewport2_GetField(viewport2);
    ok(field == -1.0f, "Got unexpected field %.8e.\n", field);
    hr = IDirect3DRMViewport2_GetPlane(viewport2, &left, &right, &bottom, &top);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    ok(left == 5.0f, "Got unexpected left %.8e.\n", left);
    ok(right == 3.0f, "Got unexpected right %.8e.\n", right);
    ok(bottom == 2.0f, "Got unexpected bottom %.8e.\n", bottom);
    ok(top == 0.0f, "Got unexpected top %.8e.\n", top);
    hr = IDirect3DRMViewport2_SetFront(viewport2, 2.0f);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    front = IDirect3DRMViewport2_GetFront(viewport2);
    ok(front == 2.0f, "Got unexpected front %.8e.\n", front);
    hr = IDirect3DRMViewport2_SetBack(viewport2, 200.0f);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    back = IDirect3DRMViewport2_GetBack(viewport2);
    ok(back == 200.0f, "Got unexpected back %.8e.\n", back);

    hr = IDirect3DRMViewport2_Init(viewport2, device3, frame3, rc.left, rc.top, rc.right, rc.bottom);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    hr = IDirect3DRMViewport2_GetDevice(viewport2, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    hr = IDirect3DRMViewport2_GetDirect3DViewport(viewport2, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    hr = IDirect3DRMViewport2_GetCamera(viewport2, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMViewport2_SetField(viewport2, 0.0f);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    hr = IDirect3DRMViewport2_SetField(viewport2, -1.0f);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    hr = IDirect3DRMViewport2_SetFront(viewport2, 0.0f);
    ok(hr == D3DRMERR_BADVALUE, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMViewport2_SetFront(viewport2, -1.0f);
    ok(hr == D3DRMERR_BADVALUE, "Got unexpected hr %#x.\n", hr);
    front = IDirect3DRMViewport2_GetFront(viewport2);
    hr = IDirect3DRMViewport2_SetBack(viewport2, front);
    ok(hr == D3DRMERR_BADVALUE, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMViewport2_SetBack(viewport2, front / 2.0f);
    ok(hr == D3DRMERR_BADVALUE, "Got unexpected hr %#x.\n", hr);

    IDirect3DRMViewport2_Release(viewport2);
    ref4 = get_refcount((IUnknown *)d3drm1);
    todo_wine ok(ref4 > ref1, "Expected ref4 > ref1, got ref1 = %u, ref4 = %u.\n", ref1, ref4);
    ref4 = get_refcount((IUnknown *)d3drm2);
    ok(ref4 == ref2, "Expected ref4 == ref2, got ref2 = %u, ref4 = %u.\n", ref2, ref4);
    ref4 = get_refcount((IUnknown *)d3drm3);
    ok(ref4 == ref3, "Expected ref4 == ref3, got ref3 = %u, ref4 = %u.\n", ref3, ref4);
    ref4 = get_refcount((IUnknown *)device3);
    ok(ref4 == device_ref, "Expected ref4 == device_ref, got device_ref = %u, ref4 = %u.\n", device_ref, ref4);
    ref4 = get_refcount((IUnknown *)frame3);
    todo_wine ok(ref4 > frame_ref2, "Expected ref4 > frame_ref2, got frame_ref2 = %u, ref4 = %u.\n", frame_ref2, ref4);

    IDirect3DRMDevice3_Release(device3);
    IDirect3DRMDevice_Release(device1);
    ref4 = get_refcount((IUnknown *)d3drm1);
    ok(ref4 > initial_ref1, "Expected ref4 > initial_ref1, got initial_ref1 = %u, ref4 = %u.\n", initial_ref1, ref4);
    ref4 = get_refcount((IUnknown *)d3drm2);
    ok(ref4 == initial_ref2, "Expected ref4 == initial_ref2, got initial_ref2 = %u, ref4 = %u.\n", initial_ref2, ref4);
    ref4 = get_refcount((IUnknown *)d3drm3);
    ok(ref4 == initial_ref3, "Expected ref4 == initial_ref3, got initial_ref3 = %u, ref4 = %u.\n", initial_ref3, ref4);
    ref4 = get_refcount((IUnknown *)frame);
    ok(ref4 == frame_ref, "Expected ref4 == frame_ref, got frame_ref = %u, ref4 = %u.\n", frame_ref, ref4);
    ref4 = get_refcount((IUnknown *)frame3);
    ok(ref4 == frame_ref2, "Expected ref4 == frame_ref2, got frame_ref2 = %u, ref4 = %u.\n", frame_ref2, ref4);

    IDirect3DRMFrame3_Release(tmp_frame3);
    IDirect3DRMFrame3_Release(frame3);
    ref4 = get_refcount((IUnknown *)d3drm1);
    ok(ref4 > initial_ref1, "Expected ref4 > initial_ref1, got initial_ref1 = %u, ref4 = %u.\n", initial_ref1, ref4);
    ref4 = get_refcount((IUnknown *)d3drm2);
    ok(ref4 == initial_ref2, "Expected ref4 == initial_ref2, got initial_ref2 = %u, ref4 = %u.\n", initial_ref2, ref4);
    ref4 = get_refcount((IUnknown *)d3drm3);
    ok(ref4 == initial_ref3, "Expected ref4 == initial_ref3, got initial_ref3 = %u, ref4 = %u.\n", initial_ref3, ref4);

    IDirect3DRMFrame3_Release(tmp_frame1);
    IDirect3DRMFrame_Release(frame);
    ref4 = get_refcount((IUnknown *)d3drm1);
    ok(ref4 == initial_ref1, "Expected ref4 == initial_ref1, got initial_ref1 = %u, ref4 = %u.\n", initial_ref1, ref4);
    ref4 = get_refcount((IUnknown *)d3drm2);
    ok(ref4 == initial_ref2, "Expected ref4 == initial_ref2, got initial_ref2 = %u, ref4 = %u.\n", initial_ref2, ref4);
    ref4 = get_refcount((IUnknown *)d3drm3);
    ok(ref4 == initial_ref3, "Expected ref4 == initial_ref3, got initial_ref3 = %u, ref4 = %u.\n", initial_ref3, ref4);
    IDirectDrawClipper_Release(clipper);

    IDirect3DRM3_Release(d3drm3);
    IDirect3DRM2_Release(d3drm2);
    IDirect3DRM_Release(d3drm1);
    DestroyWindow(window);
}

static void test_Light(void)
{
    IDirect3DRMObject *object;
    HRESULT hr;
    IDirect3DRM *d3drm;
    IDirect3DRMLight *light;
    D3DRMLIGHTTYPE type;
    D3DCOLOR color;

    hr = Direct3DRMCreate(&d3drm);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);

    hr = IDirect3DRM_CreateLightRGB(d3drm, D3DRMLIGHT_SPOT, 0.5, 0.5, 0.5, &light);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMLight interface (hr = %x)\n", hr);

    hr = IDirect3DRMLight_QueryInterface(light, &IID_IDirect3DRMObject, (void **)&object);
    ok(SUCCEEDED(hr), "Failed to get IDirect3DRMObject, hr %#x.\n", hr);
    IDirect3DRMObject_Release(object);

    test_class_name((IDirect3DRMObject *)light, "Light");
    test_object_name((IDirect3DRMObject *)light);

    type = IDirect3DRMLight_GetType(light);
    ok(type == D3DRMLIGHT_SPOT, "wrong type (%u)\n", type);

    color = IDirect3DRMLight_GetColor(light);
    ok(color == 0xff7f7f7f, "wrong color (%x)\n", color);

    hr = IDirect3DRMLight_SetType(light, D3DRMLIGHT_POINT);
    ok(hr == D3DRM_OK, "Cannot set type (hr = %x)\n", hr);
    type = IDirect3DRMLight_GetType(light);
    ok(type == D3DRMLIGHT_POINT, "wrong type (%u)\n", type);

    hr = IDirect3DRMLight_SetColor(light, 0xff180587);
    ok(hr == D3DRM_OK, "Cannot set color (hr = %x)\n", hr);
    color = IDirect3DRMLight_GetColor(light);
    ok(color == 0xff180587, "wrong color (%x)\n", color);

    hr = IDirect3DRMLight_SetColorRGB(light, 0.5, 0.5, 0.5);
    ok(hr == D3DRM_OK, "Cannot set color (hr = %x)\n", hr);
    color = IDirect3DRMLight_GetColor(light);
    ok(color == 0xff7f7f7f, "wrong color (%x)\n", color);

    IDirect3DRMLight_Release(light);

    IDirect3DRM_Release(d3drm);
}

static void test_Material2(void)
{
    HRESULT hr;
    IDirect3DRM *d3drm;
    IDirect3DRM3 *d3drm3;
    IDirect3DRMMaterial2 *material2;
    D3DVALUE r, g, b;

    hr = Direct3DRMCreate(&d3drm);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);

    if (FAILED(hr = IDirect3DRM_QueryInterface(d3drm, &IID_IDirect3DRM3, (void **)&d3drm3)))
    {
        win_skip("Cannot get IDirect3DRM3 interface (hr = %x), skipping tests\n", hr);
        IDirect3DRM_Release(d3drm);
        return;
    }

    hr = IDirect3DRM3_CreateMaterial(d3drm3, 18.5f, &material2);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMMaterial2 interface (hr = %x)\n", hr);

    test_class_name((IDirect3DRMObject *)material2, "Material");
    test_object_name((IDirect3DRMObject *)material2);

    r = IDirect3DRMMaterial2_GetPower(material2);
    ok(r == 18.5f, "wrong power (%f)\n", r);

    hr = IDirect3DRMMaterial2_GetEmissive(material2, &r, &g, &b);
    ok(hr == D3DRM_OK, "Cannot get emissive (hr = %x)\n", hr);
    ok(r == 0.0f && g == 0.0f && b == 0.0f, "wrong emissive r=%f g=%f b=%f, expected r=0.0 g=0.0 b=0.0\n", r, g, b);

    hr = IDirect3DRMMaterial2_GetSpecular(material2, &r, &g, &b);
    ok(hr == D3DRM_OK, "Cannot get emissive (hr = %x)\n", hr);
    ok(r == 1.0f && g == 1.0f && b == 1.0f, "wrong specular r=%f g=%f b=%f, expected r=1.0 g=1.0 b=1.0\n", r, g, b);

    hr = IDirect3DRMMaterial2_GetAmbient(material2, &r, &g, &b);
    ok(hr == D3DRM_OK, "Cannot get emissive (hr = %x)\n", hr);
    ok(r == 0.0f && g == 0.0f && b == 0.0f, "wrong ambient r=%f g=%f b=%f, expected r=0.0 g=0.0 b=0.0\n", r, g, b);

    hr = IDirect3DRMMaterial2_SetPower(material2, 5.87f);
    ok(hr == D3DRM_OK, "Cannot set power (hr = %x)\n", hr);
    r = IDirect3DRMMaterial2_GetPower(material2);
    ok(r == 5.87f, "wrong power (%f)\n", r);

    hr = IDirect3DRMMaterial2_SetEmissive(material2, 0.5f, 0.5f, 0.5f);
    ok(hr == D3DRM_OK, "Cannot set emissive (hr = %x)\n", hr);
    hr = IDirect3DRMMaterial2_GetEmissive(material2, &r, &g, &b);
    ok(hr == D3DRM_OK, "Cannot get emissive (hr = %x)\n", hr);
    ok(r == 0.5f && g == 0.5f && b == 0.5f, "wrong emissive r=%f g=%f b=%f, expected r=0.5 g=0.5 b=0.5\n", r, g, b);

    hr = IDirect3DRMMaterial2_SetSpecular(material2, 0.6f, 0.6f, 0.6f);
    ok(hr == D3DRM_OK, "Cannot set specular (hr = %x)\n", hr);
    hr = IDirect3DRMMaterial2_GetSpecular(material2, &r, &g, &b);
    ok(hr == D3DRM_OK, "Cannot get specular (hr = %x)\n", hr);
    ok(r == 0.6f && g == 0.6f && b == 0.6f, "wrong specular r=%f g=%f b=%f, expected r=0.6 g=0.6 b=0.6\n", r, g, b);

    hr = IDirect3DRMMaterial2_SetAmbient(material2, 0.7f, 0.7f, 0.7f);
    ok(hr == D3DRM_OK, "Cannot set ambient (hr = %x)\n", hr);
    hr = IDirect3DRMMaterial2_GetAmbient(material2, &r, &g, &b);
    ok(hr == D3DRM_OK, "Cannot get ambient (hr = %x)\n", hr);
    ok(r == 0.7f && g == 0.7f && b == 0.7f, "wrong ambient r=%f g=%f b=%f, expected r=0.7 g=0.7 b=0.7\n", r, g, b);

    IDirect3DRMMaterial2_Release(material2);

    IDirect3DRM3_Release(d3drm3);
    IDirect3DRM_Release(d3drm);
}

static void test_Texture(void)
{
    HRESULT hr;
    IDirect3DRM *d3drm1;
    IDirect3DRM2 *d3drm2;
    IDirect3DRM3 *d3drm3;
    IDirect3DRMTexture *texture1;
    IDirect3DRMTexture2 *texture2;
    IDirect3DRMTexture3 *texture3;
    IDirectDrawSurface *surface;

    D3DRMIMAGE initimg =
    {
        2, 2, 1, 1, 32,
        TRUE, 2 * sizeof(DWORD), NULL, NULL,
        0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000, 0, NULL
    },
    testimg =
    {
        0, 0, 0, 0, 0,
        TRUE, 0, (void *)0xcafebabe, NULL,
        0x000000ff, 0x0000ff00, 0x00ff0000, 0, 0, NULL
    },
    *d3drm_img = NULL;

    DWORD pixel[4] = { 20000, 30000, 10000, 0 };
    ULONG ref1, ref2, ref3, ref4;

    hr = Direct3DRMCreate(&d3drm1);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRM interface (hr = %x)\n", hr);
    ref1 = get_refcount((IUnknown *)d3drm1);

    hr = IDirect3DRM_QueryInterface(d3drm1, &IID_IDirect3DRM2, (void **)&d3drm2);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRM2 interface (hr = %x).\n", hr);

    hr = IDirect3DRM_QueryInterface(d3drm1, &IID_IDirect3DRM3, (void **)&d3drm3);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRM3 interface (hr = %x).\n", hr);

    /* Test NULL params */
    texture1 = (IDirect3DRMTexture *)0xdeadbeef;
    hr = IDirect3DRM_CreateTexture(d3drm1, NULL, &texture1);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    ok(!texture1, "Expected texture returned == NULL, got %p.\n", texture1);
    hr = IDirect3DRM_CreateTexture(d3drm1, NULL, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);

    texture2 = (IDirect3DRMTexture2 *)0xdeadbeef;
    hr = IDirect3DRM2_CreateTexture(d3drm2, NULL, &texture2);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    ok(!texture2, "Expected texture returned == NULL, got %p.\n", texture2);
    hr = IDirect3DRM2_CreateTexture(d3drm2, NULL, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);

    texture3 = (IDirect3DRMTexture3 *)0xdeadbeef;
    hr = IDirect3DRM3_CreateTexture(d3drm3, NULL, &texture3);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    ok(!texture3, "Expected texture returned == NULL, got %p.\n", texture3);
    hr = IDirect3DRM3_CreateTexture(d3drm3, NULL, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);

    /* Tests for validation of D3DRMIMAGE struct */
    hr = IDirect3DRM_CreateTexture(d3drm1, &testimg, &texture1);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMTexture interface (hr = %#x)\n", hr);
    hr = IDirect3DRM2_CreateTexture(d3drm2, &testimg, &texture2);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMTexture2 interface (hr = %#x)\n", hr);
    hr = IDirect3DRM3_CreateTexture(d3drm3, &testimg, &texture3);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMTexture3 interface (hr = %#x)\n", hr);
    IDirect3DRMTexture_Release(texture1);
    IDirect3DRMTexture2_Release(texture2);
    IDirect3DRMTexture3_Release(texture3);

    testimg.rgb = 0;
    testimg.palette = (void *)0xdeadbeef;
    testimg.palette_size = 0x39;
    hr = IDirect3DRM_CreateTexture(d3drm1, &testimg, &texture1);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMTexture interface (hr = %#x)\n", hr);
    hr = IDirect3DRM2_CreateTexture(d3drm2, &testimg, &texture2);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMTexture2 interface (hr = %#x)\n", hr);
    hr = IDirect3DRM3_CreateTexture(d3drm3, &testimg, &texture3);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMTexture3 interface (hr = %#x)\n", hr);
    IDirect3DRMTexture_Release(texture1);
    IDirect3DRMTexture2_Release(texture2);
    IDirect3DRMTexture3_Release(texture3);

    initimg.rgb = 0;
    texture1 = (IDirect3DRMTexture *)0xdeadbeef;
    hr = IDirect3DRM_CreateTexture(d3drm1, &initimg, &texture1);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    ok(!texture1, "Expected texture == NULL, got %p.\n", texture1);
    texture2 = (IDirect3DRMTexture2 *)0xdeadbeef;
    hr = IDirect3DRM2_CreateTexture(d3drm2, &initimg, &texture2);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    ok(!texture2, "Expected texture == NULL, got %p.\n", texture2);
    texture3 = (IDirect3DRMTexture3 *)0xdeadbeef;
    hr = IDirect3DRM3_CreateTexture(d3drm3, &initimg, &texture3);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    ok(!texture3, "Expected texture == NULL, got %p.\n", texture3);
    initimg.rgb = 1;
    initimg.red_mask = 0;
    hr = IDirect3DRM_CreateTexture(d3drm1, &initimg, &texture1);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    hr = IDirect3DRM2_CreateTexture(d3drm2, &initimg, &texture2);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    hr = IDirect3DRM3_CreateTexture(d3drm3, &initimg, &texture3);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    initimg.red_mask = 0x000000ff;
    initimg.green_mask = 0;
    hr = IDirect3DRM_CreateTexture(d3drm1, &initimg, &texture1);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    hr = IDirect3DRM2_CreateTexture(d3drm2, &initimg, &texture2);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    hr = IDirect3DRM3_CreateTexture(d3drm3, &initimg, &texture3);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    initimg.green_mask = 0x0000ff00;
    initimg.blue_mask = 0;
    hr = IDirect3DRM_CreateTexture(d3drm1, &initimg, &texture1);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    hr = IDirect3DRM2_CreateTexture(d3drm2, &initimg, &texture2);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    hr = IDirect3DRM3_CreateTexture(d3drm3, &initimg, &texture3);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    initimg.blue_mask = 0x00ff0000;
    initimg.buffer1 = NULL;
    hr = IDirect3DRM_CreateTexture(d3drm1, &initimg, &texture1);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    hr = IDirect3DRM2_CreateTexture(d3drm2, &initimg, &texture2);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    hr = IDirect3DRM3_CreateTexture(d3drm3, &initimg, &texture3);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);

    initimg.buffer1 = &pixel;
    hr = IDirect3DRM_CreateTexture(d3drm1, &initimg, &texture1);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMTexture interface (hr = %#x)\n", hr);
    ref2 = get_refcount((IUnknown *)d3drm1);
    ok(ref2 > ref1, "expected ref2 > ref1, got ref1 = %u , ref2 = %u.\n", ref1, ref2);
    ref3 = get_refcount((IUnknown *)d3drm2);
    ok(ref3 == ref1, "expected ref3 == ref1, got ref1 = %u , ref3 = %u.\n", ref1, ref3);
    ref4 = get_refcount((IUnknown *)d3drm3);
    ok(ref4 == ref1, "expected ref4 == ref1, got ref1 = %u , ref4 = %u.\n", ref1, ref4);
    hr = IDirect3DRM2_CreateTexture(d3drm2, &initimg, &texture2);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMTexture2 interface (hr = %#x)\n", hr);
    ref2 = get_refcount((IUnknown *)d3drm1);
    ok(ref2 > ref1 + 1, "expected ref2 > (ref1 + 1), got ref1 = %u , ref2 = %u.\n", ref1, ref2);
    ref3 = get_refcount((IUnknown *)d3drm2);
    ok(ref3 == ref1, "expected ref3 == ref1, got ref1 = %u , ref3 = %u.\n", ref1, ref3);
    ref4 = get_refcount((IUnknown *)d3drm3);
    ok(ref4 == ref1, "expected ref4 == ref1, got ref1 = %u , ref4 = %u.\n", ref1, ref4);
    hr = IDirect3DRM3_CreateTexture(d3drm3, &initimg, &texture3);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMTexture3 interface (hr = %#x)\n", hr);
    ref2 = get_refcount((IUnknown *)d3drm1);
    ok(ref2 > ref1 + 2, "expected ref2 > (ref1 + 2), got ref1 = %u , ref2 = %u.\n", ref1, ref2);
    ref3 = get_refcount((IUnknown *)d3drm2);
    ok(ref3 == ref1, "expected ref3 == ref1, got ref1 = %u , ref3 = %u.\n", ref1, ref3);
    ref4 = get_refcount((IUnknown *)d3drm3);
    ok(ref4 == ref1, "expected ref4 == ref1, got ref1 = %u , ref4 = %u.\n", ref1, ref4);

    /* Created from image, GetSurface() does not work. */
    hr = IDirect3DRMTexture3_GetSurface(texture3, 0, NULL);
    ok(hr == D3DRMERR_BADVALUE, "GetSurface() expected to fail, %#x\n", hr);

    hr = IDirect3DRMTexture3_GetSurface(texture3, 0, &surface);
    ok(hr == D3DRMERR_NOTCREATEDFROMDDS, "GetSurface() expected to fail, %#x\n", hr);

    /* Test all failures together */
    test_class_name((IDirect3DRMObject *)texture1, "Texture");
    test_class_name((IDirect3DRMObject *)texture2, "Texture");
    test_class_name((IDirect3DRMObject *)texture3, "Texture");
    test_object_name((IDirect3DRMObject *)texture1);
    test_object_name((IDirect3DRMObject *)texture2);
    test_object_name((IDirect3DRMObject *)texture3);

    d3drm_img = IDirect3DRMTexture_GetImage(texture1);
    ok(!!d3drm_img, "Failed to get image.\n");
    ok(d3drm_img == &initimg, "Expected image returned == %p, got %p.\n", &initimg, d3drm_img);

    IDirect3DRMTexture_Release(texture1);
    ref2 = get_refcount((IUnknown *)d3drm1);
    ok(ref2 - 2 == ref1, "expected (ref2 - 2) == ref1, got ref1 = %u, ref2 = %u.\n", ref1, ref2);
    ref3 = get_refcount((IUnknown *)d3drm2);
    ok(ref3 == ref1, "expected ref3 == ref1, got ref1 = %u, ref3 = %u.\n", ref1, ref3);
    ref4 = get_refcount((IUnknown *)d3drm3);
    ok(ref4 == ref1, "expected ref4 == ref1, got ref1 = %u, ref4 = %u.\n", ref1, ref4);

    d3drm_img = NULL;
    d3drm_img = IDirect3DRMTexture2_GetImage(texture2);
    ok(!!d3drm_img, "Failed to get image.\n");
    ok(d3drm_img == &initimg, "Expected image returned == %p, got %p.\n", &initimg, d3drm_img);

    IDirect3DRMTexture2_Release(texture2);
    ref2 = get_refcount((IUnknown *)d3drm1);
    ok(ref2 - 1 == ref1, "expected (ref2 - 1) == ref1, got ref1 = %u, ref2 = %u.\n", ref1, ref2);
    ref3 = get_refcount((IUnknown *)d3drm2);
    ok(ref3 == ref1, "expected ref3 == ref1, got ref1 = %u, ref3 = %u.\n", ref1, ref3);
    ref4 = get_refcount((IUnknown *)d3drm3);
    ok(ref4 == ref1, "expected ref4 == ref1, got ref1 = %u, ref4 = %u.\n", ref1, ref4);

    d3drm_img = NULL;
    d3drm_img = IDirect3DRMTexture3_GetImage(texture3);
    ok(!!d3drm_img, "Failed to get image.\n");
    ok(d3drm_img == &initimg, "Expected image returned == %p, got %p.\n", &initimg, d3drm_img);

    IDirect3DRMTexture3_Release(texture3);
    ref2 = get_refcount((IUnknown *)d3drm1);
    ok(ref2 == ref1, "expected ref2 == ref1, got ref1 = %u, ref2 = %u.\n", ref1, ref2);
    ref3 = get_refcount((IUnknown *)d3drm2);
    ok(ref3 == ref1, "expected ref3 == ref1, got ref1 = %u, ref3 = %u.\n", ref1, ref3);
    ref4 = get_refcount((IUnknown *)d3drm3);
    ok(ref4 == ref1, "expected ref4 == ref1, got ref1 = %u, ref4 = %u.\n", ref1, ref4);

    /* InitFromImage tests */
    /* Tests for validation of D3DRMIMAGE struct */
    testimg.rgb = 1;
    testimg.palette = NULL;
    testimg.palette_size = 0;
    hr = IDirect3DRM2_CreateObject(d3drm2, &CLSID_CDirect3DRMTexture, NULL, &IID_IDirect3DRMTexture2,
            (void **)&texture2);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMTexture2 interface (hr = %#x).\n", hr);
    hr = IDirect3DRM3_CreateObject(d3drm3, &CLSID_CDirect3DRMTexture, NULL, &IID_IDirect3DRMTexture3,
            (void **)&texture3);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMTexture3 interface (hr = %#x).\n", hr);
    hr = IDirect3DRMTexture2_InitFromImage(texture2, &testimg);
    ok(SUCCEEDED(hr), "Cannot initialize IDirect3DRMTexture2 interface (hr = %#x)\n", hr);
    hr = IDirect3DRMTexture3_InitFromImage(texture3, &testimg);
    ok(SUCCEEDED(hr), "Cannot initialize IDirect3DRMTexture3 interface (hr = %#x)\n", hr);
    IDirect3DRMTexture2_Release(texture2);
    IDirect3DRMTexture3_Release(texture3);

    testimg.rgb = 0;
    testimg.palette = (void *)0xdeadbeef;
    testimg.palette_size = 0x39;
    hr = IDirect3DRM2_CreateObject(d3drm2, &CLSID_CDirect3DRMTexture, NULL, &IID_IDirect3DRMTexture2,
            (void **)&texture2);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMTexture2 interface (hr = %#x).\n", hr);
    hr = IDirect3DRM3_CreateObject(d3drm3, &CLSID_CDirect3DRMTexture, NULL, &IID_IDirect3DRMTexture3,
            (void **)&texture3);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMTexture3 interface (hr = %#x).\n", hr);
    hr = IDirect3DRMTexture2_InitFromImage(texture2, &testimg);
    ok(SUCCEEDED(hr), "Cannot initialize IDirect3DRMTexture2 interface (hr = %#x)\n", hr);
    hr = IDirect3DRMTexture3_InitFromImage(texture3, &testimg);
    ok(SUCCEEDED(hr), "Cannot initialize IDirect3DRMTexture3 interface (hr = %#x)\n", hr);
    IDirect3DRMTexture2_Release(texture2);
    IDirect3DRMTexture3_Release(texture3);

    hr = IDirect3DRM2_CreateObject(d3drm2, &CLSID_CDirect3DRMTexture, NULL, &IID_IDirect3DRMTexture2,
            (void **)&texture2);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMTexture2 interface (hr = %#x).\n", hr);
    ref2 = get_refcount((IUnknown *)texture2);
    hr = IDirect3DRMTexture2_InitFromImage(texture2, NULL);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    ref3 = get_refcount((IUnknown *)texture2);
    ok(ref3 == ref2, "expected ref3 == ref2, got ref2 = %u , ref3 = %u.\n", ref2, ref3);

    hr = IDirect3DRM3_CreateObject(d3drm3, &CLSID_CDirect3DRMTexture, NULL, &IID_IDirect3DRMTexture3,
            (void **)&texture3);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMTexture3 interface (hr = %#x).\n", hr);
    ref2 = get_refcount((IUnknown *)texture3);
    hr = IDirect3DRMTexture3_InitFromImage(texture3, NULL);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    ref3 = get_refcount((IUnknown *)texture3);
    ok(ref3 == ref2, "expected ref3 == ref2, got ref2 = %u , ref3 = %u.\n", ref2, ref3);

    initimg.rgb = 0;
    hr = IDirect3DRMTexture2_InitFromImage(texture2, &initimg);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    hr = IDirect3DRMTexture3_InitFromImage(texture3, &initimg);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    initimg.rgb = 1;
    initimg.red_mask = 0;
    hr = IDirect3DRMTexture2_InitFromImage(texture2, &initimg);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    hr = IDirect3DRMTexture3_InitFromImage(texture3, &initimg);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    initimg.red_mask = 0x000000ff;
    initimg.green_mask = 0;
    hr = IDirect3DRMTexture2_InitFromImage(texture2, &initimg);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    hr = IDirect3DRMTexture3_InitFromImage(texture3, &initimg);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    initimg.green_mask = 0x0000ff00;
    initimg.blue_mask = 0;
    hr = IDirect3DRMTexture2_InitFromImage(texture2, &initimg);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    hr = IDirect3DRMTexture3_InitFromImage(texture3, &initimg);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    initimg.blue_mask = 0x00ff0000;
    initimg.buffer1 = NULL;
    hr = IDirect3DRMTexture2_InitFromImage(texture2, &initimg);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    hr = IDirect3DRMTexture3_InitFromImage(texture3, &initimg);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    initimg.buffer1 = &pixel;

    d3drm_img = NULL;
    hr = IDirect3DRMTexture2_InitFromImage(texture2, &initimg);
    ok(SUCCEEDED(hr), "Cannot initialize IDirect3DRMTexture2 from image (hr = %#x).\n", hr);
    ref2 = get_refcount((IUnknown *)d3drm1);
    ok(ref2 > ref1, "expected ref2 > ref1, got ref1 = %u , ref2 = %u.\n", ref1, ref2);
    ref3 = get_refcount((IUnknown *)d3drm2);
    ok(ref3 == ref1, "expected ref3 == ref1, got ref1 = %u , ref3 = %u.\n", ref1, ref3);
    ref4 = get_refcount((IUnknown *)d3drm3);
    ok(ref4 == ref1, "expected ref4 == ref1, got ref1 = %u , ref4 = %u.\n", ref1, ref4);

    hr = IDirect3DRMTexture2_InitFromImage(texture2, &initimg);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    /* Release leaked reference to d3drm1 */
    IDirect3DRM_Release(d3drm1);

    d3drm_img = IDirect3DRMTexture2_GetImage(texture2);
    ok(!!d3drm_img, "Failed to get image.\n");
    ok(d3drm_img == &initimg, "Expected image returned == %p, got %p.\n", &initimg, d3drm_img);
    IDirect3DRMTexture2_Release(texture2);
    ref2 = get_refcount((IUnknown *)d3drm1);
    ok(ref2 == ref1, "expected ref2 == ref1, got ref1 = %u, ref2 = %u.\n", ref1, ref2);
    ref3 = get_refcount((IUnknown *)d3drm2);
    ok(ref3 == ref1, "expected ref3 == ref1, got ref1 = %u, ref3 = %u.\n", ref1, ref3);
    ref4 = get_refcount((IUnknown *)d3drm3);
    ok(ref4 == ref1, "expected ref4 == ref1, got ref1 = %u, ref4 = %u.\n", ref1, ref4);

    d3drm_img = NULL;
    hr = IDirect3DRMTexture3_InitFromImage(texture3, &initimg);
    ok(SUCCEEDED(hr), "Cannot initialize IDirect3DRMTexture3 from image (hr = %#x).\n", hr);
    ref2 = get_refcount((IUnknown *)d3drm1);
    ok(ref2 > ref1, "expected ref2 > ref1, got ref1 = %u , ref2 = %u.\n", ref1, ref2);
    ref3 = get_refcount((IUnknown *)d3drm2);
    ok(ref3 == ref1, "expected ref3 == ref1, got ref1 = %u , ref3 = %u.\n", ref1, ref3);
    ref4 = get_refcount((IUnknown *)d3drm3);
    ok(ref4 == ref1, "expected ref4 == ref1, got ref1 = %u , ref4 = %u.\n", ref1, ref4);

    hr = IDirect3DRMTexture3_InitFromImage(texture3, &initimg);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    IDirect3DRM_Release(d3drm1);

    d3drm_img = IDirect3DRMTexture3_GetImage(texture3);
    ok(!!d3drm_img, "Failed to get image.\n");
    ok(d3drm_img == &initimg, "Expected image returned == %p, got %p.\n", &initimg, d3drm_img);
    IDirect3DRMTexture3_Release(texture3);
    ref2 = get_refcount((IUnknown *)d3drm1);
    ok(ref2 == ref1, "expected ref2 == ref1, got ref1 = %u, ref2 = %u.\n", ref1, ref2);
    ref3 = get_refcount((IUnknown *)d3drm2);
    ok(ref3 == ref1, "expected ref3 == ref1, got ref1 = %u, ref3 = %u.\n", ref1, ref3);
    ref4 = get_refcount((IUnknown *)d3drm3);
    ok(ref4 == ref1, "expected ref4 == ref1, got ref1 = %u, ref4 = %u.\n", ref1, ref4);

    IDirect3DRM3_Release(d3drm3);
    IDirect3DRM2_Release(d3drm2);
    IDirect3DRM_Release(d3drm1);
}

static void test_Device(void)
{
    IDirectDrawClipper *pClipper;
    HRESULT hr;
    IDirect3DRM *d3drm;
    IDirect3DRMDevice *device;
    IDirect3DRMWinDevice *win_device;
    GUID driver;
    HWND window;
    RECT rc;

    window = create_window();
    GetClientRect(window, &rc);

    hr = Direct3DRMCreate(&d3drm);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);

    hr = DirectDrawCreateClipper(0, &pClipper, NULL);
    ok(hr == DD_OK, "Cannot get IDirectDrawClipper interface (hr = %x)\n", hr);

    hr = IDirectDrawClipper_SetHWnd(pClipper, 0, window);
    ok(hr == DD_OK, "Cannot set HWnd to Clipper (hr = %x)\n", hr);

    memcpy(&driver, &IID_IDirect3DRGBDevice, sizeof(GUID));
    hr = IDirect3DRM3_CreateDeviceFromClipper(d3drm, pClipper, &driver, rc.right, rc.bottom, &device);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMDevice interface (hr = %x)\n", hr);

    test_class_name((IDirect3DRMObject *)device, "Device");
    test_object_name((IDirect3DRMObject *)device);

    /* WinDevice */
    if (FAILED(hr = IDirect3DRMDevice_QueryInterface(device, &IID_IDirect3DRMWinDevice, (void **)&win_device)))
    {
        win_skip("Cannot get IDirect3DRMWinDevice interface (hr = %x), skipping tests\n", hr);
        goto cleanup;
    }

    test_class_name((IDirect3DRMObject *)win_device, "Device");
    test_object_name((IDirect3DRMObject *)win_device);
    IDirect3DRMWinDevice_Release(win_device);

cleanup:
    IDirect3DRMDevice_Release(device);
    IDirectDrawClipper_Release(pClipper);

    IDirect3DRM_Release(d3drm);
    DestroyWindow(window);
}

static void test_frame_transform(void)
{
    IDirect3DRMFrame *frame, *subframe;
    D3DRMMATRIX4D matrix, add_matrix;
    IDirect3DRM *d3drm;
    D3DVECTOR v1, v2;
    HRESULT hr;

    hr = Direct3DRMCreate(&d3drm);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DRM_CreateFrame(d3drm, NULL, &frame);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DRMFrame_GetTransform(frame, matrix);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    expect_matrix(matrix,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f, 0);

    memcpy(add_matrix, identity, sizeof(add_matrix));
    add_matrix[3][0] = 3.0f;
    add_matrix[3][1] = 3.0f;
    add_matrix[3][2] = 3.0f;

    frame_set_transform(frame,
            2.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 2.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 2.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    hr = IDirect3DRMFrame_AddTransform(frame, D3DRMCOMBINE_REPLACE, add_matrix);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMFrame_GetTransform(frame, matrix);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    expect_matrix(matrix,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            3.0f, 3.0f, 3.0f, 1.0f, 1);

    frame_set_transform(frame,
            2.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 2.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 2.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    hr = IDirect3DRMFrame_AddTransform(frame, D3DRMCOMBINE_BEFORE, add_matrix);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMFrame_GetTransform(frame, matrix);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    expect_matrix(matrix,
            2.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 2.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 2.0f, 0.0f,
            6.0f, 6.0f, 6.0f, 1.0f, 1);

    frame_set_transform(frame,
            2.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 2.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 2.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    hr = IDirect3DRMFrame_AddTransform(frame, D3DRMCOMBINE_AFTER, add_matrix);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMFrame_GetTransform(frame, matrix);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    expect_matrix(matrix,
            2.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 2.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 2.0f, 0.0f,
            3.0f, 3.0f, 3.0f, 1.0f, 1);

    add_matrix[3][3] = 2.0f;
    hr = IDirect3DRMFrame_AddTransform(frame, D3DRMCOMBINE_REPLACE, add_matrix);
    ok(hr == D3DRMERR_BADVALUE, "Got unexpected hr %#x.\n", hr);

    frame_set_transform(frame,
            2.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 2.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 2.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    hr = IDirect3DRMFrame_AddTranslation(frame, D3DRMCOMBINE_REPLACE, 3.0f, 3.0f, 3.0f);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMFrame_GetTransform(frame, matrix);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    expect_matrix(matrix,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            3.0f, 3.0f, 3.0f, 1.0f, 1);

    frame_set_transform(frame,
            2.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 2.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 2.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    hr = IDirect3DRMFrame_AddTranslation(frame, D3DRMCOMBINE_BEFORE, 3.0f, 3.0f, 3.0f);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMFrame_GetTransform(frame, matrix);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    expect_matrix(matrix,
            2.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 2.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 2.0f, 0.0f,
            6.0f, 6.0f, 6.0f, 1.0f, 1);

    frame_set_transform(frame,
            2.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 2.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 2.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    hr = IDirect3DRMFrame_AddTranslation(frame, D3DRMCOMBINE_AFTER, 3.0f, 3.0f, 3.0f);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMFrame_GetTransform(frame, matrix);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    expect_matrix(matrix,
            2.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 2.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 2.0f, 0.0f,
            3.0f, 3.0f, 3.0f, 1.0f, 1);

    frame_set_transform(frame,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            3.0f, 3.0f, 3.0f, 1.0f);
    hr = IDirect3DRMFrame_AddScale(frame, D3DRMCOMBINE_REPLACE, 2.0f, 2.0f, 2.0f);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMFrame_GetTransform(frame, matrix);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    expect_matrix(matrix,
            2.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 2.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 2.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f, 1);

    frame_set_transform(frame,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            3.0f, 3.0f, 3.0f, 1.0f);
    hr = IDirect3DRMFrame_AddScale(frame, D3DRMCOMBINE_BEFORE, 2.0f, 2.0f, 2.0f);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMFrame_GetTransform(frame, matrix);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    expect_matrix(matrix,
            2.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 2.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 2.0f, 0.0f,
            3.0f, 3.0f, 3.0f, 1.0f, 1);

    frame_set_transform(frame,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            3.0f, 3.0f, 3.0f, 1.0f);
    hr = IDirect3DRMFrame_AddScale(frame, D3DRMCOMBINE_AFTER, 2.0f, 2.0f, 2.0f);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMFrame_GetTransform(frame, matrix);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    expect_matrix(matrix,
            2.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 2.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 2.0f, 0.0f,
            6.0f, 6.0f, 6.0f, 1.0f, 1);

    frame_set_transform(frame,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            3.0f, 3.0f, 3.0f, 1.0f);
    hr = IDirect3DRMFrame_AddRotation(frame, D3DRMCOMBINE_REPLACE, 1.0f, 0.0f, 0.0f, M_PI_2);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMFrame_GetTransform(frame, matrix);
    matrix_sanitise(matrix);
    expect_matrix(matrix,
            1.0f,  0.0f, 0.0f, 0.0f,
            0.0f,  0.0f, 1.0f, 0.0f,
            0.0f, -1.0f, 0.0f, 0.0f,
            0.0f,  0.0f, 0.0f, 1.0f, 1);

    frame_set_transform(frame,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            3.0f, 3.0f, 3.0f, 1.0f);
    hr = IDirect3DRMFrame_AddRotation(frame, D3DRMCOMBINE_BEFORE, 1.0f, 0.0f, 0.0f, M_PI_2);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMFrame_GetTransform(frame, matrix);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    matrix_sanitise(matrix);
    expect_matrix(matrix,
            1.0f,  0.0f, 0.0f, 0.0f,
            0.0f,  0.0f, 1.0f, 0.0f,
            0.0f, -1.0f, 0.0f, 0.0f,
            3.0f,  3.0f, 3.0f, 1.0f, 1);

    frame_set_transform(frame,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            3.0f, 3.0f, 3.0f, 1.0f);
    hr = IDirect3DRMFrame_AddRotation(frame, D3DRMCOMBINE_AFTER, 1.0f, 0.0f, 0.0f, M_PI_2);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMFrame_GetTransform(frame, matrix);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    matrix_sanitise(matrix);
    expect_matrix(matrix,
            1.0f,  0.0f, 0.0f, 0.0f,
            0.0f,  0.0f, 1.0f, 0.0f,
            0.0f, -1.0f, 0.0f, 0.0f,
            3.0f, -3.0f, 3.0f, 1.0f, 1);

    hr = IDirect3DRMFrame_AddRotation(frame, D3DRMCOMBINE_REPLACE, 0.0f, 0.0f, 1.0f, M_PI_2);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMFrame_GetTransform(frame, matrix);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    matrix_sanitise(matrix);
    expect_matrix(matrix,
             0.0f, 1.0f, 0.0f, 0.0f,
            -1.0f, 0.0f, 0.0f, 0.0f,
             0.0f, 0.0f, 1.0f, 0.0f,
             0.0f, 0.0f, 0.0f, 1.0f, 1);

    hr = IDirect3DRMFrame_AddRotation(frame, D3DRMCOMBINE_REPLACE, 0.0f, 0.0f, 0.0f, M_PI_2);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DRMFrame_GetTransform(frame, matrix);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    matrix_sanitise(matrix);
    expect_matrix(matrix,
            1.0f,  0.0f, 0.0f, 0.0f,
            0.0f,  0.0f, 1.0f, 0.0f,
            0.0f, -1.0f, 0.0f, 0.0f,
            0.0f,  0.0f, 0.0f, 1.0f, 1);

    frame_set_transform(frame,
             2.0f,  0.0f,  0.0f, 0.0f,
             0.0f,  4.0f,  0.0f, 0.0f,
             0.0f,  0.0f,  8.0f, 0.0f,
            64.0f, 64.0f, 64.0f, 1.0f);
    hr = IDirect3DRM_CreateFrame(d3drm, frame, &subframe);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    frame_set_transform(subframe,
             1.0f,  0.0f,  0.0f, 0.0f,
             0.0f,  1.0f,  0.0f, 0.0f,
             0.0f,  0.0f,  1.0f, 0.0f,
            11.0f, 11.0f, 11.0f, 1.0f);
    set_vector(&v1, 3.0f, 5.0f, 7.0f);

    hr = IDirect3DRMFrame_Transform(frame, &v2, &v1);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    expect_vector(&v2, 70.0f, 84.0f, 120.0f, 1);

    hr = IDirect3DRMFrame_Transform(subframe, &v2, &v1);
    ok(hr == D3DRM_OK, "Got unexpected hr %#x.\n", hr);
    expect_vector(&v2, 92.0f, 128.0f, 208.0f, 1);

    IDirect3DRMFrame_Release(subframe);
    IDirect3DRMFrame_Release(frame);
    IDirect3DRM_Release(d3drm);
}

static int nb_objects = 0;
static const GUID* refiids[] =
{
    &IID_IDirect3DRMMeshBuilder,
    &IID_IDirect3DRMMeshBuilder,
    &IID_IDirect3DRMFrame,
    &IID_IDirect3DRMMaterial /* Not taken into account and not notified */
};

static void __cdecl object_load_callback(IDirect3DRMObject *object, REFIID objectguid, void *arg)
{
    ok(object != NULL, "Arg 1 should not be null\n");
    ok(IsEqualGUID(objectguid, refiids[nb_objects]), "Arg 2 is incorrect\n");
    ok(arg == (void *)0xdeadbeef, "Arg 3 should be 0xdeadbeef (got %p)\n", arg);
    nb_objects++;
}

static void test_d3drm_load(void)
{
    HRESULT hr;
    IDirect3DRM *d3drm;
    D3DRMLOADMEMORY info;
    const GUID* req_refiids[] = { &IID_IDirect3DRMMeshBuilder, &IID_IDirect3DRMFrame, &IID_IDirect3DRMMaterial };

    hr = Direct3DRMCreate(&d3drm);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);

    info.lpMemory = data_d3drm_load;
    info.dSize = strlen(data_d3drm_load);
    hr = IDirect3DRM_Load(d3drm, &info, NULL, (GUID **)req_refiids, 3, D3DRMLOAD_FROMMEMORY,
            object_load_callback, (void *)0xdeadbeef, NULL, NULL, NULL);
    ok(hr == D3DRM_OK, "Cannot load data (hr = %x)\n", hr);
    ok(nb_objects == 3, "Should have loaded 3 objects (got %d)\n", nb_objects);

    IDirect3DRM_Release(d3drm);
}

IDirect3DRMMeshBuilder *mesh_builder = NULL;

static void __cdecl object_load_callback_frame(IDirect3DRMObject *object, REFIID object_guid, void *arg)
{
    HRESULT hr;
    IDirect3DRMFrame *frame;
    IDirect3DRMVisualArray *array;
    IDirect3DRMVisual *visual;
    ULONG size;
    char name[128];

    hr = IDirect3DRMObject_QueryInterface(object, &IID_IDirect3DRMFrame, (void**)&frame);
    ok(hr == D3DRM_OK, "IDirect3DRMObject_QueryInterface returned %x\n", hr);

    hr = IDirect3DRMFrame_GetVisuals(frame, &array);
    ok(hr == D3DRM_OK, "IDirect3DRMFrame_GetVisuals returned %x\n", hr);

    size = IDirect3DRMVisualArray_GetSize(array);
    ok(size == 1, "Wrong size %u returned, expected 1\n", size);

    hr = IDirect3DRMVisualArray_GetElement(array, 0, &visual);
    ok(hr == D3DRM_OK, "IDirect3DRMVisualArray_GetElement returned %x\n", hr);

    hr = IDirect3DRMVisual_QueryInterface(visual, &IID_IDirect3DRMMeshBuilder, (void**)&mesh_builder);
    ok(hr == D3DRM_OK, "IDirect3DRMVisualArray_GetSize returned %x\n", hr);

    size = sizeof(name);
    hr = IDirect3DRMMeshBuilder_GetName(mesh_builder, &size, name);
    ok(hr == D3DRM_OK, "IDirect3DRMMeshBuilder_GetName returned %x\n", hr);
    ok(!strcmp(name, "mesh1"), "Wrong name %s, expected mesh1\n", name);

    IDirect3DRMVisual_Release(visual);
    IDirect3DRMVisualArray_Release(array);
    IDirect3DRMFrame_Release(frame);
}

struct {
    int vertex_count;
    int face_count;
    int vertex_per_face;
    int face_data_size;
    DWORD color;
    float power;
    float specular[3];
    float emissive[3];
} groups[3] = {
    { 4, 3, 3, 9, 0x4c0000ff, 30.0f, { 0.31f, 0.32f, 0.33f }, { 0.34f, 0.35f, 0.36f } },
    { 4, 2, 3, 6, 0x3300ff00, 20.0f, { 0.21f, 0.22f, 0.23f }, { 0.24f, 0.25f, 0.26f } },
    { 3, 1, 3, 3, 0x19ff0000, 10.0f, { 0.11f, 0.12f, 0.13f }, { 0.14f, 0.15f, 0.16f } }
};

static void test_frame_mesh_materials(void)
{
    HRESULT hr;
    IDirect3DRM *d3drm;
    D3DRMLOADMEMORY info;
    const GUID *req_refiids[] = { &IID_IDirect3DRMFrame };
    IDirect3DRMMesh *mesh;
    ULONG size;
    IDirect3DRMMaterial *material;
    IDirect3DRMTexture *texture;
    int i;

    hr = Direct3DRMCreate(&d3drm);
    ok(hr == D3DRM_OK, "Direct3DRMCreate returned %x\n", hr);

    info.lpMemory = data_frame_mesh_materials;
    info.dSize = strlen(data_frame_mesh_materials);
    hr = IDirect3DRM_Load(d3drm, &info, NULL, (GUID**)req_refiids, 1, D3DRMLOAD_FROMMEMORY, object_load_callback_frame, (void*)0xdeadbeef, NULL, NULL, NULL);
    ok(hr == D3DRM_OK, "Cannot load data (hr = %x)\n", hr);

    hr = IDirect3DRMMeshBuilder_CreateMesh(mesh_builder, &mesh);
    ok(hr == D3DRM_OK, "IDirect3DRMMeshBuilder_CreateMesh returned %x\n", hr);

    size = IDirect3DRMMesh_GetGroupCount(mesh);
    ok(size == 3, "Wrong size %u returned, expected 3\n", size);

    for (i = 0; i < size; i++)
    {
        D3DVALUE red, green, blue, power;
        D3DCOLOR color;
        unsigned vertex_count, face_count, vertex_per_face;
        DWORD face_data_size;

        hr = IDirect3DRMMesh_GetGroup(mesh, i, &vertex_count, &face_count, &vertex_per_face, &face_data_size, NULL);
        ok(hr == D3DRM_OK, "Group %d: IDirect3DRMMesh_GetGroup returned %x\n", i, hr);
        ok(vertex_count == groups[i].vertex_count, "Group %d: Wrong vertex count %d, expected %d\n", i, vertex_count, groups[i].vertex_count);
        ok(face_count == groups[i].face_count, "Group %d: Wrong face count %d; expected %d\n", i, face_count, groups[i].face_count);
        ok(vertex_per_face == groups[i].vertex_per_face, "Group %d: Wrong vertex per face %d, expected %d\n", i, vertex_per_face, groups[i].vertex_per_face);
        ok(face_data_size == groups[i].face_data_size, "Group %d: Wrong face data size %d, expected %d\n", i, face_data_size, groups[i].face_data_size);

        color = IDirect3DRMMesh_GetGroupColor(mesh, i);
        ok(color == groups[i].color, "Group %d: Wrong color %x, expected %x\n", i, color, groups[i].color);

        hr = IDirect3DRMMesh_GetGroupMaterial(mesh, i, &material);
        ok(hr == D3DRM_OK, "Group %d: IDirect3DRMMesh_GetGroupMaterial returned %x\n", i, hr);
        ok(material != NULL, "Group %d: No material\n", i);
        power = IDirect3DRMMaterial_GetPower(material);
        ok(power == groups[i].power, "Group %d: Wrong power %f, expected %f\n", i, power,  groups[i].power);
        hr = IDirect3DRMMaterial_GetSpecular(material, &red, &green, &blue);
        ok(hr == D3DRM_OK, "Group %d: IDirect3DRMMaterial_GetSpecular returned %x\n", i, hr);
        ok(red == groups[i].specular[0], "Group %d: Wrong specular red %f, expected %f\n", i, red, groups[i].specular[0]);
        ok(green == groups[i].specular[1], "Group %d: Wrong specular green %f, pD3DRMexpected %f\n", i, green, groups[i].specular[1]);
        ok(blue == groups[i].specular[2], "Group %d: Wrong specular blue %f, expected %f\n", i, blue, groups[i].specular[2]);
        hr = IDirect3DRMMaterial_GetEmissive(material, &red, &green, &blue);
        ok(hr == D3DRM_OK, "Group %d: IDirect3DRMMaterial_GetEmissive returned %x\n", i, hr);
        ok(red == groups[i].emissive[0], "Group %d: Wrong emissive red %f, expected %f\n", i, red, groups[i].emissive[0]);
        ok(green == groups[i].emissive[1], "Group %d: Wrong emissive green %f, expected %f\n", i, green, groups[i].emissive[1]);
        ok(blue == groups[i].emissive[2], "Group %d: Wrong emissive blue %f, expected %f\n", i, blue, groups[i].emissive[2]);

        hr = IDirect3DRMMesh_GetGroupTexture(mesh, i, &texture);
        ok(hr == D3DRM_OK, "Group %d: IDirect3DRMMesh_GetGroupTexture returned %x\n", i, hr);
        ok(!texture, "Group %d: Unexpected texture\n", i);

        if (material)
            IDirect3DRMMaterial_Release(material);
        if (texture)
            IDirect3DRMTexture_Release(texture);
    }

    IDirect3DRMMesh_Release(mesh);
    IDirect3DRMMeshBuilder_Release(mesh_builder);
    IDirect3DRM_Release(d3drm);
}

struct qi_test
{
    REFIID iid;
    REFIID refcount_iid;
    REFIID vtable_iid;
    HRESULT hr;
};

static void test_qi(const char *test_name, IUnknown *base_iface,
                    REFIID refcount_iid, const struct qi_test *tests, UINT entry_count)
{
    ULONG refcount, expected_refcount;
    IUnknown *iface1, *iface2;
    HRESULT hr;
    UINT i, j;

    for (i = 0; i < entry_count; ++i)
    {
        hr = IUnknown_QueryInterface(base_iface, tests[i].iid, (void **)&iface1);
        ok(hr == tests[i].hr, "Got hr %#x for test \"%s\" %u.\n", hr, test_name, i);
        if (SUCCEEDED(hr))
        {
            for (j = 0; j < entry_count; ++j)
            {
                hr = IUnknown_QueryInterface(iface1, tests[j].iid, (void **)&iface2);
                ok(hr == tests[j].hr, "Got hr %#x for test \"%s\" %u, %u.\n", hr, test_name, i, j);
                if (SUCCEEDED(hr))
                {
                    expected_refcount = 0;
                    if (IsEqualGUID(refcount_iid, tests[j].refcount_iid))
                        ++expected_refcount;
                    if (IsEqualGUID(tests[i].refcount_iid, tests[j].refcount_iid))
                        ++expected_refcount;
                    refcount = IUnknown_Release(iface2);
                    ok(refcount == expected_refcount, "Got refcount %u for test \"%s\" %u, %u, expected %u.\n",
                                refcount, test_name, i, j, expected_refcount);
                    if (tests[i].vtable_iid && tests[j].vtable_iid && IsEqualGUID(tests[i].vtable_iid, tests[j].vtable_iid))
                        ok(iface1 == iface2,
                                "Expected iface1 == iface2 for test \"%s\" %u, %u. Got iface1 = %p, iface 2 = %p.\n",
                                test_name, i, j, iface1, iface2);
                    else if (tests[i].vtable_iid && tests[j].vtable_iid)
                        ok(iface1 != iface2,
                                "Expected iface1 != iface2 for test \"%s\" %u, %u. Got iface1 == iface2 == %p.\n",
                                test_name, i, j, iface1);
                }
            }

            expected_refcount = 0;
            if (IsEqualGUID(refcount_iid, tests[i].refcount_iid))
                ++expected_refcount;
            refcount = IUnknown_Release(iface1);
            ok(refcount == expected_refcount, "Got refcount %u for test \"%s\" %u, expected %u.\n",
                        refcount, test_name, i, expected_refcount);
        }
    }
}

static void test_d3drm_qi(void)
{
    static const struct qi_test tests[] =
    {
        { &IID_IDirect3DRM3,               &IID_IDirect3DRM3, &IID_IDirect3DRM3, S_OK                      },
        { &IID_IDirect3DRM2,               &IID_IDirect3DRM2, &IID_IDirect3DRM2, S_OK                      },
        { &IID_IDirect3DRM,                &IID_IDirect3DRM,  &IID_IDirect3DRM,  S_OK                      },
        { &IID_IDirect3DRMDevice,          NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMObject,          NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMObject2,         NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMDevice2,         NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMDevice3,         NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMViewport,        NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMViewport2,       NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFrame,           NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFrame2,          NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFrame3,          NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMVisual,          NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMesh,            NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMeshBuilder,     NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMeshBuilder2,    NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMeshBuilder3,    NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFace,            NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFace2,           NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMLight,           NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMTexture,         NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMTexture2,        NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMTexture3,        NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMWrap,            NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMaterial,        NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMaterial2,       NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimation,       NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimation2,      NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimationSet,    NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimationSet2,   NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMObjectArray,     NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMDeviceArray,     NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMViewportArray,   NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFrameArray,      NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMVisualArray,     NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMLightArray,      NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMPickedArray,     NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFaceArray,       NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimationArray,  NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMUserVisual,      NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMShadow,          NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMShadow2,         NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMInterpolator,    NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMProgressiveMesh, NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMPicked2Array,    NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMClippedVisual,   NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawClipper,         NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface7,        NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface4,        NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface3,        NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface2,        NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface,         NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice7,           NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice3,           NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice2,           NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice,            NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D7,                 NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D3,                 NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D2,                 NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D,                  NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw7,               NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw4,               NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw3,               NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw2,               NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw,                NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DLight,             NULL,              NULL,              CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IUnknown,                   &IID_IDirect3DRM,  &IID_IDirect3DRM,  S_OK                      },
    };
    HRESULT hr;
    IDirect3DRM *d3drm;

    hr = Direct3DRMCreate(&d3drm);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);

    test_qi("d3drm_qi", (IUnknown *)d3drm, &IID_IDirect3DRM, tests, ARRAY_SIZE(tests));

    IDirect3DRM_Release(d3drm);
}

static void test_frame_qi(void)
{
    static const struct qi_test tests[] =
    {
        { &IID_IDirect3DRMFrame3,          &IID_IUnknown, &IID_IDirect3DRMFrame3, S_OK                      },
        { &IID_IDirect3DRMFrame2,          &IID_IUnknown, &IID_IDirect3DRMFrame2, S_OK                      },
        { &IID_IDirect3DRMFrame,           &IID_IUnknown, &IID_IDirect3DRMFrame,  S_OK                      },
        { &IID_IDirect3DRM,                NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMDevice,          NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMObject,          &IID_IUnknown, &IID_IDirect3DRMFrame,  S_OK                      },
        { &IID_IDirect3DRMDevice2,         NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMDevice3,         NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMViewport,        NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMViewport2,       NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRM3,               NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRM2,               NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMVisual,          &IID_IUnknown, &IID_IDirect3DRMFrame,  S_OK                      },
        { &IID_IDirect3DRMMesh,            NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMeshBuilder,     NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMeshBuilder2,    NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMeshBuilder3,    NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFace,            NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFace2,           NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMLight,           NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMTexture,         NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMTexture2,        NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMTexture3,        NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMWrap,            NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMaterial,        NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMaterial2,       NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimation,       NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimation2,      NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimationSet,    NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimationSet2,   NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMObjectArray,     NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMDeviceArray,     NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMViewportArray,   NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFrameArray,      NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMVisualArray,     NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMLightArray,      NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMPickedArray,     NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFaceArray,       NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimationArray,  NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMUserVisual,      NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMShadow,          NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMShadow2,         NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMInterpolator,    NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMProgressiveMesh, NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMPicked2Array,    NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMClippedVisual,   NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawClipper,         NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface7,        NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface4,        NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface3,        NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface2,        NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface,         NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice7,           NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice3,           NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice2,           NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice,            NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D7,                 NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D3,                 NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D2,                 NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D,                  NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw7,               NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw4,               NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw3,               NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw2,               NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw,                NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DLight,             NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IUnknown,                   &IID_IUnknown, NULL,                   S_OK                      },
    };
    HRESULT hr;
    IDirect3DRM *d3drm1;
    IDirect3DRM2 *d3drm2;
    IDirect3DRM3 *d3drm3;
    IDirect3DRMFrame *frame1;
    IDirect3DRMFrame2 *frame2;
    IDirect3DRMFrame3 *frame3;
    IUnknown *unknown;

    hr = Direct3DRMCreate(&d3drm1);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);

    hr = IDirect3DRM_CreateFrame(d3drm1, NULL, &frame1);
    ok(hr == D3DRM_OK, "Failed to create frame1 (hr = %x)\n", hr);
    hr = IDirect3DRMFrame_QueryInterface(frame1, &IID_IUnknown, (void **)&unknown);
    ok(hr == D3DRM_OK, "Failed to create IUnknown from frame1 (hr = %x)\n", hr);
    IDirect3DRMFrame_Release(frame1);
    test_qi("frame1_qi", unknown, &IID_IUnknown, tests, ARRAY_SIZE(tests));
    IUnknown_Release(unknown);

    hr = IDirect3DRM_QueryInterface(d3drm1, &IID_IDirect3DRM2, (void **)&d3drm2);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM2 interface (hr = %x).\n", hr);
    hr = IDirect3DRM2_CreateFrame(d3drm2, NULL, &frame2);
    ok(hr == D3DRM_OK, "Failed to create frame2 (hr = %x)\n", hr);
    hr = IDirect3DRMFrame2_QueryInterface(frame2, &IID_IUnknown, (void **)&unknown);
    ok(hr == D3DRM_OK, "Failed to create IUnknown from frame2 (hr = %x)\n", hr);
    IDirect3DRMFrame2_Release(frame2);
    test_qi("frame2_qi", unknown, &IID_IUnknown, tests, ARRAY_SIZE(tests));
    IUnknown_Release(unknown);

    hr = IDirect3DRM_QueryInterface(d3drm1, &IID_IDirect3DRM3, (void **)&d3drm3);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM3 interface (hr = %x).\n", hr);
    hr = IDirect3DRM3_CreateFrame(d3drm3, NULL, &frame3);
    ok(hr == D3DRM_OK, "Failed to create frame3 (hr = %x)\n", hr);
    hr = IDirect3DRMFrame3_QueryInterface(frame3, &IID_IUnknown, (void **)&unknown);
    ok(hr == D3DRM_OK, "Failed to create IUnknown from frame3 (hr = %x)\n", hr);
    IDirect3DRMFrame3_Release(frame3);
    test_qi("frame3_qi", unknown, &IID_IUnknown, tests, ARRAY_SIZE(tests));
    IUnknown_Release(unknown);

    IDirect3DRM3_Release(d3drm3);
    IDirect3DRM2_Release(d3drm2);
    IDirect3DRM_Release(d3drm1);
}

static void test_device_qi(void)
{
    static const struct qi_test tests[] =
    {
        { &IID_IDirect3DRM3,               NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRM2,               NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRM,                NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMDevice,          &IID_IUnknown, &IID_IDirect3DRMDevice,    S_OK,                     },
        { &IID_IDirect3DRMDevice2,         &IID_IUnknown, &IID_IDirect3DRMDevice2,   S_OK,                     },
        { &IID_IDirect3DRMDevice3,         &IID_IUnknown, &IID_IDirect3DRMDevice3,   S_OK,                     },
        { &IID_IDirect3DRMWinDevice,       &IID_IUnknown, &IID_IDirect3DRMWinDevice, S_OK,                     },
        { &IID_IDirect3DRMObject,          &IID_IUnknown, &IID_IDirect3DRMDevice,    S_OK,                     },
        { &IID_IDirect3DRMViewport,        NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMViewport2,       NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFrame,           NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFrame2,          NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFrame3,          NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMVisual,          NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMesh,            NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMeshBuilder,     NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMeshBuilder2,    NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMeshBuilder3,    NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFace,            NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFace2,           NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMLight,           NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMTexture,         NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMTexture2,        NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMTexture3,        NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMWrap,            NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMaterial,        NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMaterial2,       NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimation,       NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimation2,      NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimationSet,    NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimationSet2,   NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMObjectArray,     NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMDeviceArray,     NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMViewportArray,   NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFrameArray,      NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMVisualArray,     NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMLightArray,      NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMPickedArray,     NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFaceArray,       NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimationArray,  NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMUserVisual,      NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMShadow,          NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMShadow2,         NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMInterpolator,    NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMProgressiveMesh, NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMPicked2Array,    NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMClippedVisual,   NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawClipper,         NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface7,        NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface4,        NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface3,        NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface2,        NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface,         NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice7,           NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice3,           NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice2,           NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice,            NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D7,                 NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D3,                 NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D2,                 NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D,                  NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw7,               NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw4,               NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw3,               NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw2,               NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw,                NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DLight,             NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IUnknown,                   &IID_IUnknown, NULL,                      S_OK,                     },
    };
    HRESULT hr;
    IDirect3DRM *d3drm1;
    IDirect3DRM2 *d3drm2;
    IDirect3DRM3 *d3drm3;
    IDirectDrawClipper *clipper;
    IDirect3DRMDevice *device1;
    IDirect3DRMDevice2 *device2;
    IDirect3DRMDevice3 *device3;
    IUnknown *unknown;
    HWND window;
    GUID driver;
    RECT rc;

    window = create_window();
    GetClientRect(window, &rc);
    hr = DirectDrawCreateClipper(0, &clipper, NULL);
    ok(hr == DD_OK, "Cannot get IDirectDrawClipper interface (hr = %x)\n", hr);
    hr = IDirectDrawClipper_SetHWnd(clipper, 0, window);
    ok(hr == DD_OK, "Cannot set HWnd to Clipper (hr = %x)\n", hr);

    hr = Direct3DRMCreate(&d3drm1);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRM interface (hr = %x)\n", hr);
    memcpy(&driver, &IID_IDirect3DRGBDevice, sizeof(GUID));
    hr = IDirect3DRM_CreateDeviceFromClipper(d3drm1, clipper, &driver, rc.right, rc.bottom, &device1);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMDevice interface (hr = %x)\n", hr);
    hr = IDirect3DRMDevice_QueryInterface(device1, &IID_IUnknown, (void **)&unknown);
    ok(SUCCEEDED(hr), "Cannot get IUnknown interface from IDirect3DRMDevice (hr = %x)\n", hr);
    IDirect3DRMDevice_Release(device1);
    test_qi("device1_qi", unknown, &IID_IUnknown, tests, ARRAY_SIZE(tests));
    IUnknown_Release(unknown);

    hr = IDirect3DRM_QueryInterface(d3drm1, &IID_IDirect3DRM2, (void **)&d3drm2);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRM2 interface (hr = %x).\n", hr);
    hr = IDirect3DRM2_CreateDeviceFromClipper(d3drm2, clipper, &driver, rc.right, rc.bottom, &device2);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMDevice2 interface (hr = %x)\n", hr);
    hr = IDirect3DRMDevice2_QueryInterface(device2, &IID_IUnknown, (void **)&unknown);
    ok(SUCCEEDED(hr), "Cannot get IUnknown interface from IDirect3DRMDevice2 (hr = %x)\n", hr);
    IDirect3DRMDevice2_Release(device2);
    test_qi("device2_qi", unknown, &IID_IUnknown, tests, ARRAY_SIZE(tests));
    IUnknown_Release(unknown);

    hr = IDirect3DRM_QueryInterface(d3drm1, &IID_IDirect3DRM3, (void **)&d3drm3);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRM3 interface (hr = %x).\n", hr);
    hr = IDirect3DRM3_CreateDeviceFromClipper(d3drm3, clipper, &driver, rc.right, rc.bottom, &device3);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMDevice3 interface (hr = %x)\n", hr);
    hr = IDirect3DRMDevice3_QueryInterface(device3, &IID_IUnknown, (void **)&unknown);
    ok(SUCCEEDED(hr), "Cannot get IUnknown interface from IDirect3DRMDevice3 (hr = %x)\n", hr);
    IDirect3DRMDevice3_Release(device3);
    test_qi("device3_qi", unknown, &IID_IUnknown, tests, ARRAY_SIZE(tests));
    IUnknown_Release(unknown);

    IDirectDrawClipper_Release(clipper);
    IDirect3DRM3_Release(d3drm3);
    IDirect3DRM2_Release(d3drm2);
    IDirect3DRM_Release(d3drm1);
    DestroyWindow(window);
}


static HRESULT CALLBACK surface_callback(IDirectDrawSurface *surface, DDSURFACEDESC *desc, void *context)
{
    IDirectDrawSurface **primary = context;

    if (desc->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {
        *primary = surface;
        return DDENUMRET_CANCEL;
    }
    IDirectDrawSurface_Release(surface);

    return DDENUMRET_OK;
}

static void test_create_device_from_clipper1(void)
{
    DDSCAPS caps = { DDSCAPS_ZBUFFER };
    IDirect3DRM *d3drm1 = NULL;
    IDirectDraw *ddraw = NULL;
    IUnknown *unknown = NULL;
    IDirect3DRMDevice *device1 = (IDirect3DRMDevice *)0xdeadbeef;
    IDirect3DDevice *d3ddevice1 = NULL;
    IDirectDrawClipper *clipper = NULL, *d3drm_clipper = NULL;
    IDirectDrawSurface *surface = NULL, *ds = NULL, *d3drm_primary = NULL;
    IDirectDrawSurface7 *surface7 = NULL;
    DDSURFACEDESC desc, surface_desc;
    DWORD expected_flags, ret_val;
    HWND window;
    GUID driver = IID_IDirect3DRGBDevice;
    HRESULT hr;
    ULONG ref1, ref2, cref1, cref2;
    RECT rc;

    window = create_window();
    GetClientRect(window, &rc);
    hr = DirectDrawCreateClipper(0, &clipper, NULL);
    ok(hr == DD_OK, "Cannot get IDirectDrawClipper interface (hr = %x).\n", hr);
    hr = IDirectDrawClipper_SetHWnd(clipper, 0, window);
    ok(hr == DD_OK, "Cannot set HWnd to Clipper (hr = %x).\n", hr);

    hr = Direct3DRMCreate(&d3drm1);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x).\n", hr);
    ref1 = get_refcount((IUnknown *)d3drm1);
    cref1 = get_refcount((IUnknown *)clipper);

    hr = IDirect3DRM_CreateDeviceFromClipper(d3drm1, clipper, &driver, 0, 0, &device1);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %x.\n", hr);
    ok(device1 == NULL, "Expected device returned == NULL, got %p.\n", device1);

    /* If NULL is passed for clipper, CreateDeviceFromClipper returns D3DRMERR_BADVALUE */
    hr = IDirect3DRM_CreateDeviceFromClipper(d3drm1, NULL, &driver, 300, 200, &device1);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %x.\n", hr);

    hr = IDirect3DRM_CreateDeviceFromClipper(d3drm1, clipper, &driver, 300, 200, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %x.\n", hr);

    hr = IDirect3DRM_CreateDeviceFromClipper(d3drm1, clipper, &driver, 300, 200, &device1);
    ok(hr == D3DRM_OK, "Cannot create IDirect3DRMDevice interface (hr = %x).\n", hr);
    ref2 = get_refcount((IUnknown *)d3drm1);
    ok(ref2 > ref1, "expected ref2 > ref1, got ref1 = %u , ref2 = %u.\n", ref1, ref2);
    cref2 = get_refcount((IUnknown *)clipper);
    ok(cref2 > cref1, "expected cref2 > cref1, got cref1 = %u , cref2 = %u.\n", cref1, cref2);
    ret_val = IDirect3DRMDevice_GetWidth(device1);
    ok(ret_val == 300, "Expected device width = 300, got %u.\n", ret_val);
    ret_val = IDirect3DRMDevice_GetHeight(device1);
    ok(ret_val == 200, "Expected device height == 200, got %u.\n", ret_val);

    /* Fetch immediate mode device in order to access render target */
    hr = IDirect3DRMDevice_GetDirect3DDevice(device1, &d3ddevice1);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DDevice interface (hr = %x).\n", hr);

    hr = IDirect3DDevice_QueryInterface(d3ddevice1, &IID_IDirectDrawSurface, (void **)&surface);
    ok(hr == DD_OK, "Cannot get surface to the render target (hr = %x).\n", hr);

    hr = IDirectDrawSurface_GetClipper(surface, &d3drm_clipper);
    ok(hr == DDERR_NOCLIPPERATTACHED, "Expected hr == DDERR_NOCLIPPERATTACHED, got %x.\n", hr);

    /* Check if CreateDeviceFromClipper creates a primary surface and attaches the clipper to it */
    hr = IDirectDrawSurface_QueryInterface(surface, &IID_IDirectDrawSurface7, (void **)&surface7);
    ok(hr == DD_OK, "Cannot get IDirectDrawSurface7 interface (hr = %x).\n", hr);
    IDirectDrawSurface7_GetDDInterface(surface7, (void **)&unknown);
    hr = IUnknown_QueryInterface(unknown, &IID_IDirectDraw, (void **)&ddraw);
    ok(hr == DD_OK, "Cannot get IDirectDraw interface (hr = %x).\n", hr);
    IUnknown_Release(unknown);
    hr = IDirectDraw_EnumSurfaces(ddraw, DDENUMSURFACES_ALL | DDENUMSURFACES_DOESEXIST,
            NULL, &d3drm_primary, surface_callback);
    ok(hr == DD_OK, "Failed to enumerate surfaces (hr = %x).\n", hr);
    ok(d3drm_primary != NULL, "No primary surface was enumerated.\n");
    hr = IDirectDrawSurface_GetClipper(d3drm_primary, &d3drm_clipper);
    ok(hr == DD_OK, "Cannot get attached clipper from primary surface (hr = %x).\n", hr);
    ok(d3drm_clipper == clipper, "Expected clipper returned == %p, got %p.\n", clipper , d3drm_clipper);

    IDirectDrawClipper_Release(d3drm_clipper);
    IDirectDrawSurface_Release(d3drm_primary);
    IDirectDrawSurface7_Release(surface7);
    IDirectDraw_Release(ddraw);

    /* Check properties of render target and depth surface */
    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &surface_desc);
    ok(hr == DD_OK, "Cannot get surface desc structure (hr = %x).\n", hr);

    ok((surface_desc.dwWidth == 300) && (surface_desc.dwHeight == 200), "Expected surface dimensions = 300, 200, got %u, %u.\n",
            surface_desc.dwWidth, surface_desc.dwHeight);
    ok((surface_desc.ddsCaps.dwCaps & (DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE)) == (DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE),
            "Expected caps containing %x, got %x.\n", DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE, surface_desc.ddsCaps.dwCaps);
    expected_flags = DDSD_PIXELFORMAT | DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH;
    ok(surface_desc.dwFlags == expected_flags, "Expected %x for flags, got %x.\n", expected_flags, surface_desc.dwFlags);

    hr = DirectDrawCreate(NULL, &ddraw, NULL);
    ok(hr == DD_OK, "Cannot get IDirectDraw interface (hr = %x).\n", hr);
    desc.dwSize = sizeof(desc);
    hr = IDirectDraw_GetDisplayMode(ddraw, &desc);
    ok(hr == DD_OK, "Cannot get IDirectDraw display mode (hr = %x)\n", hr);
    ok(desc.ddpfPixelFormat.dwRGBBitCount == surface_desc.ddpfPixelFormat.dwRGBBitCount, "Expected %u bpp, got %u bpp.\n",
            surface_desc.ddpfPixelFormat.dwRGBBitCount, desc.ddpfPixelFormat.dwRGBBitCount);

    hr = IDirectDrawSurface_GetAttachedSurface(surface, &caps, &ds);
    ok(hr == DD_OK, "Cannot get attached depth surface (hr = %x).\n", hr);

    desc.dwSize = sizeof(desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(ds, &desc);
    ok(hr == DD_OK, "Cannot get z surface desc structure (hr = %x).\n", hr);

    ok((desc.dwWidth == 300) && (desc.dwHeight == 200), "Expected surface dimensions = 300, 200, got %u, %u.\n",
            desc.dwWidth, desc.dwHeight);
    ok((desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER) == DDSCAPS_ZBUFFER, "Expected caps containing %x, got %x.\n", DDSCAPS_ZBUFFER, desc.ddsCaps.dwCaps);
    expected_flags = DDSD_ZBUFFERBITDEPTH | DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH;
    ok(desc.dwFlags == expected_flags, "Expected %x for flags, got %x.\n", expected_flags, desc.dwFlags);
    ok(desc.dwZBufferBitDepth == 16, "Expected 16 for Z buffer bit depth, got %u.\n", desc.dwZBufferBitDepth);
    ok(desc.ddpfPixelFormat.dwStencilBitMask == 0, "Expected 0 stencil bits, got %x.\n", desc.ddpfPixelFormat.dwStencilBitMask);

    /* Release old objects and check refcount of device and clipper */
    IDirectDrawSurface_Release(ds);
    ds = NULL;
    IDirectDrawSurface_Release(surface);
    surface = NULL;
    IDirect3DDevice_Release(d3ddevice1);
    d3ddevice1 = NULL;
    IDirect3DRMDevice_Release(device1);
    ref2 = get_refcount((IUnknown *)d3drm1);
    ok(ref1 == ref2, "expected ref1 == ref2, got ref1 = %u, ref2 = %u.\n", ref1, ref2);
    cref2 = get_refcount((IUnknown *)clipper);
    ok(cref1 == cref2, "expected cref1 == cref2, got cref1 = %u, cref2 = %u.\n", cref1, cref2);

    /* Test if render target format follows the screen format */
    hr = IDirectDraw_GetDisplayMode(ddraw, &desc);
    ok(hr == DD_OK, "Cannot get IDirectDraw display mode (hr = %x)\n", hr);
    hr = IDirectDraw_SetDisplayMode(ddraw, desc.dwWidth, desc.dwHeight, 16);
    ok(hr == DD_OK, "Cannot set display mode to 16bpp (hr = %x).\n", hr);

    hr = IDirectDraw_GetDisplayMode(ddraw, &desc);
    ok(hr == DD_OK, "Cannot get IDirectDraw display mode (hr = %x)\n", hr);
    ok(desc.ddpfPixelFormat.dwRGBBitCount == 16, "Expected 16 bpp, got %u.\n", desc.ddpfPixelFormat.dwRGBBitCount);

    hr = IDirect3DRM_CreateDeviceFromClipper(d3drm1, clipper, &driver, rc.right, rc.bottom, &device1);
    ok(hr == D3DRM_OK, "Cannot create IDirect3DRMDevice interface (hr = %x).\n", hr);

    hr = IDirect3DRMDevice_GetDirect3DDevice(device1, &d3ddevice1);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DDevice interface (hr = %x).\n", hr);

    hr = IDirect3DDevice_QueryInterface(d3ddevice1, &IID_IDirectDrawSurface, (void **)&surface);
    ok(hr == DD_OK, "Cannot get surface to the render target (hr = %x).\n", hr);

    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &surface_desc);
    ok(hr == DD_OK, "Cannot get surface desc structure (hr = %x).\n", hr);
    todo_wine ok(surface_desc.ddpfPixelFormat.dwRGBBitCount == 16, "Expected 16bpp, got %ubpp.\n",
            surface_desc.ddpfPixelFormat.dwRGBBitCount);

    hr = IDirectDraw2_RestoreDisplayMode(ddraw);
    ok(SUCCEEDED(hr), "RestoreDisplayMode failed, hr %#x.\n", hr);

    if (ds)
        IDirectDrawSurface_Release(ds);
    IDirectDrawSurface_Release(surface);
    IDirect3DDevice_Release(d3ddevice1);
    IDirect3DRMDevice_Release(device1);
    IDirect3DRM_Release(d3drm1);
    IDirectDrawClipper_Release(clipper);
    IDirectDraw_Release(ddraw);
    DestroyWindow(window);
}

static void test_create_device_from_clipper2(void)
{
    DDSCAPS caps = { DDSCAPS_ZBUFFER };
    IDirect3DRM *d3drm1 = NULL;
    IDirect3DRM2 *d3drm2 = NULL;
    IDirectDraw *ddraw = NULL;
    IUnknown *unknown = NULL;
    IDirect3DRMDevice2 *device2 = (IDirect3DRMDevice2 *)0xdeadbeef;
    IDirect3DDevice2 *d3ddevice2 = NULL;
    IDirectDrawClipper *clipper = NULL, *d3drm_clipper = NULL;
    IDirectDrawSurface *surface = NULL, *ds = NULL, *d3drm_primary = NULL;
    IDirectDrawSurface7 *surface7 = NULL;
    DDSURFACEDESC desc, surface_desc;
    DWORD expected_flags, ret_val;
    HWND window;
    GUID driver = IID_IDirect3DRGBDevice;
    HRESULT hr;
    ULONG ref1, ref2, ref3, cref1, cref2;
    RECT rc;

    window = create_window();
    GetClientRect(window, &rc);
    hr = DirectDrawCreateClipper(0, &clipper, NULL);
    ok(hr == DD_OK, "Cannot get IDirectDrawClipper interface (hr = %x).\n", hr);
    hr = IDirectDrawClipper_SetHWnd(clipper, 0, window);
    ok(hr == DD_OK, "Cannot set HWnd to Clipper (hr = %x).\n", hr);

    hr = Direct3DRMCreate(&d3drm1);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x).\n", hr);
    ref1 = get_refcount((IUnknown *)d3drm1);
    cref1 = get_refcount((IUnknown *)clipper);

    hr = IDirect3DRM_QueryInterface(d3drm1, &IID_IDirect3DRM2, (void **)&d3drm2);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM2 interface (hr = %x).\n", hr);
    ref2 = get_refcount((IUnknown *)d3drm2);

    hr = IDirect3DRM2_CreateDeviceFromClipper(d3drm2, clipper, &driver, 0, 0, &device2);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %x.\n", hr);
    ok(device2 == NULL, "Expected device returned == NULL, got %p.\n", device2);

    /* If NULL is passed for clipper, CreateDeviceFromClipper returns D3DRMERR_BADVALUE */
    hr = IDirect3DRM2_CreateDeviceFromClipper(d3drm2, NULL, &driver, 300, 200, &device2);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %x.\n", hr);

    hr = IDirect3DRM2_CreateDeviceFromClipper(d3drm2, clipper, &driver, 300, 200, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %x.\n", hr);

    hr = IDirect3DRM2_CreateDeviceFromClipper(d3drm2, clipper, &driver, 300, 200, &device2);
    ok(hr == D3DRM_OK, "Cannot create IDirect3DRMDevice2 interface (hr = %x).\n", hr);
    ref3 = get_refcount((IUnknown *)d3drm1);
    ok(ref3 > ref1, "expected ref3 > ref1, got ref1 = %u , ref3 = %u.\n", ref1, ref3);
    ref3 = get_refcount((IUnknown *)d3drm2);
    ok(ref3 == ref2, "expected ref3 == ref2, got ref2 = %u , ref3 = %u.\n", ref2, ref3);
    cref2 = get_refcount((IUnknown *)clipper);
    ok(cref2 > cref1, "expected cref2 > cref1, got cref1 = %u , cref2 = %u.\n", cref1, cref2);
    ret_val = IDirect3DRMDevice2_GetWidth(device2);
    ok(ret_val == 300, "Expected device width = 300, got %u.\n", ret_val);
    ret_val = IDirect3DRMDevice2_GetHeight(device2);
    ok(ret_val == 200, "Expected device height == 200, got %u.\n", ret_val);

    /* Fetch immediate mode device in order to access render target */
    hr = IDirect3DRMDevice2_GetDirect3DDevice2(device2, &d3ddevice2);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DDevice2 interface (hr = %x).\n", hr);

    hr = IDirect3DDevice2_GetRenderTarget(d3ddevice2, &surface);
    ok(hr == DD_OK, "Cannot get surface to the render target (hr = %x).\n", hr);

    hr = IDirectDrawSurface_GetClipper(surface, &d3drm_clipper);
    ok(hr == DDERR_NOCLIPPERATTACHED, "Expected hr == DDERR_NOCLIPPERATTACHED, got %x.\n", hr);

    /* Check if CreateDeviceFromClipper creates a primary surface and attaches the clipper to it */
    hr = IDirectDrawSurface_QueryInterface(surface, &IID_IDirectDrawSurface7, (void **)&surface7);
    ok(hr == DD_OK, "Cannot get IDirectDrawSurface7 interface (hr = %x).\n", hr);
    IDirectDrawSurface7_GetDDInterface(surface7, (void **)&unknown);
    hr = IUnknown_QueryInterface(unknown, &IID_IDirectDraw, (void **)&ddraw);
    ok(hr == DD_OK, "Cannot get IDirectDraw interface (hr = %x).\n", hr);
    IUnknown_Release(unknown);
    hr = IDirectDraw_EnumSurfaces(ddraw, DDENUMSURFACES_ALL | DDENUMSURFACES_DOESEXIST,
            NULL, &d3drm_primary, surface_callback);
    ok(hr == DD_OK, "Failed to enumerate surfaces (hr = %x).\n", hr);
    ok(d3drm_primary != NULL, "No primary surface was enumerated.\n");
    hr = IDirectDrawSurface_GetClipper(d3drm_primary, &d3drm_clipper);
    ok(hr == DD_OK, "Cannot get attached clipper from primary surface (hr = %x).\n", hr);
    ok(d3drm_clipper == clipper, "Expected clipper returned == %p, got %p.\n", clipper , d3drm_clipper);

    IDirectDrawClipper_Release(d3drm_clipper);
    IDirectDrawSurface_Release(d3drm_primary);
    IDirectDrawSurface7_Release(surface7);
    IDirectDraw_Release(ddraw);

    /* Check properties of render target and depth surface */
    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &surface_desc);
    ok(hr == DD_OK, "Cannot get surface desc structure (hr = %x).\n", hr);

    ok((surface_desc.dwWidth == 300) && (surface_desc.dwHeight == 200), "Expected surface dimensions = 300, 200, got %u, %u.\n",
            surface_desc.dwWidth, surface_desc.dwHeight);
    ok((surface_desc.ddsCaps.dwCaps & (DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE)) == (DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE),
            "Expected caps containing %x, got %x.\n", DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE, surface_desc.ddsCaps.dwCaps);
    expected_flags = DDSD_PIXELFORMAT | DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH;
    ok(surface_desc.dwFlags == expected_flags, "Expected %x for flags, got %x.\n", expected_flags, surface_desc.dwFlags);

    hr = DirectDrawCreate(NULL, &ddraw, NULL);
    ok(hr == DD_OK, "Cannot get IDirectDraw interface (hr = %x).\n", hr);
    desc.dwSize = sizeof(desc);
    hr = IDirectDraw_GetDisplayMode(ddraw, &desc);
    ok(hr == DD_OK, "Cannot get IDirectDraw display mode (hr = %x)\n", hr);
    ok(desc.ddpfPixelFormat.dwRGBBitCount == surface_desc.ddpfPixelFormat.dwRGBBitCount, "Expected %u bpp, got %u bpp.\n",
            surface_desc.ddpfPixelFormat.dwRGBBitCount, desc.ddpfPixelFormat.dwRGBBitCount);

    hr = IDirectDrawSurface_GetAttachedSurface(surface, &caps, &ds);
    ok(hr == DD_OK, "Cannot get attached depth surface (hr = %x).\n", hr);

    desc.dwSize = sizeof(desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(ds, &desc);
    ok(hr == DD_OK, "Cannot get z surface desc structure (hr = %x).\n", hr);

    ok((desc.dwWidth == 300) && (desc.dwHeight == 200), "Expected surface dimensions = 300, 200, got %u, %u.\n",
            desc.dwWidth, desc.dwHeight);
    ok((desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER) == DDSCAPS_ZBUFFER, "Expected caps containing %x, got %x.\n", DDSCAPS_ZBUFFER, desc.ddsCaps.dwCaps);
    expected_flags = DDSD_ZBUFFERBITDEPTH | DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH;
    ok(desc.dwFlags == expected_flags, "Expected %x for flags, got %x.\n", expected_flags, desc.dwFlags);
    ok(desc.dwZBufferBitDepth == 16, "Expected 16 for Z buffer bit depth, got %u.\n", desc.dwZBufferBitDepth);
    ok(desc.ddpfPixelFormat.dwStencilBitMask == 0, "Expected 0 stencil bits, got %x.\n", desc.ddpfPixelFormat.dwStencilBitMask);

    /* Release old objects and check refcount of device and clipper */
    IDirectDrawSurface_Release(ds);
    ds = NULL;
    IDirectDrawSurface_Release(surface);
    surface = NULL;
    IDirect3DDevice2_Release(d3ddevice2);
    d3ddevice2 = NULL;
    IDirect3DRMDevice2_Release(device2);
    ref3 = get_refcount((IUnknown *)d3drm1);
    ok(ref1 == ref3, "expected ref1 == ref3, got ref1 = %u, ref3 = %u.\n", ref1, ref3);
    ref3 = get_refcount((IUnknown *)d3drm2);
    ok(ref3 == ref2, "expected ref3 == ref2, got ref2 = %u , ref3 = %u.\n", ref2, ref3);
    cref2 = get_refcount((IUnknown *)clipper);
    ok(cref1 == cref2, "expected cref1 == cref2, got cref1 = %u, cref2 = %u.\n", cref1, cref2);

    /* Test if render target format follows the screen format */
    hr = IDirectDraw_GetDisplayMode(ddraw, &desc);
    ok(hr == DD_OK, "Cannot get IDirectDraw display mode (hr = %x)\n", hr);
    hr = IDirectDraw_SetDisplayMode(ddraw, desc.dwWidth, desc.dwHeight, 16);
    ok(hr == DD_OK, "Cannot set display mode to 16bpp (hr = %x).\n", hr);

    hr = IDirectDraw_GetDisplayMode(ddraw, &desc);
    ok(hr == DD_OK, "Cannot get IDirectDraw display mode (hr = %x)\n", hr);
    ok(desc.ddpfPixelFormat.dwRGBBitCount == 16, "Expected 16 bpp, got %u.\n", desc.ddpfPixelFormat.dwRGBBitCount);

    hr = IDirect3DRM2_CreateDeviceFromClipper(d3drm2, clipper, &driver, rc.right, rc.bottom, &device2);
    ok(hr == D3DRM_OK, "Cannot create IDirect3DRMDevice2 interface (hr = %x).\n", hr);

    hr = IDirect3DRMDevice2_GetDirect3DDevice2(device2, &d3ddevice2);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DDevice2 interface (hr = %x).\n", hr);

    hr = IDirect3DDevice2_GetRenderTarget(d3ddevice2, &surface);
    ok(hr == DD_OK, "Cannot get surface to the render target (hr = %x).\n", hr);

    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &surface_desc);
    ok(hr == DD_OK, "Cannot get surface desc structure (hr = %x).\n", hr);
    todo_wine ok(surface_desc.ddpfPixelFormat.dwRGBBitCount == 16, "Expected 16bpp, got %ubpp.\n",
            surface_desc.ddpfPixelFormat.dwRGBBitCount);

    hr = IDirectDraw2_RestoreDisplayMode(ddraw);
    ok(SUCCEEDED(hr), "RestoreDisplayMode failed, hr %#x.\n", hr);

    IDirectDrawSurface_Release(surface);
    IDirect3DDevice2_Release(d3ddevice2);
    IDirect3DRMDevice2_Release(device2);
    IDirect3DRM2_Release(d3drm2);
    IDirect3DRM_Release(d3drm1);
    IDirectDrawClipper_Release(clipper);
    IDirectDraw_Release(ddraw);
    DestroyWindow(window);
}

static void test_create_device_from_clipper3(void)
{
    DDSCAPS caps = { DDSCAPS_ZBUFFER };
    IDirect3DRM *d3drm1 = NULL;
    IDirect3DRM3 *d3drm3 = NULL;
    IDirectDraw *ddraw = NULL;
    IUnknown *unknown = NULL;
    IDirect3DRMDevice3 *device3 = (IDirect3DRMDevice3 *)0xdeadbeef;
    IDirect3DDevice2 *d3ddevice2 = NULL;
    IDirectDrawClipper *clipper = NULL, *d3drm_clipper = NULL;
    IDirectDrawSurface *surface = NULL, *ds = NULL, *d3drm_primary = NULL;
    IDirectDrawSurface7 *surface7 = NULL;
    DDSURFACEDESC desc, surface_desc;
    DWORD expected_flags, ret_val;
    HWND window;
    GUID driver = IID_IDirect3DRGBDevice;
    HRESULT hr;
    ULONG ref1, ref2, ref3, cref1, cref2;
    RECT rc;

    window = create_window();
    GetClientRect(window, &rc);
    hr = DirectDrawCreateClipper(0, &clipper, NULL);
    ok(hr == DD_OK, "Cannot get IDirectDrawClipper interface (hr = %x).\n", hr);
    hr = IDirectDrawClipper_SetHWnd(clipper, 0, window);
    ok(hr == DD_OK, "Cannot set HWnd to Clipper (hr = %x).\n", hr);

    hr = Direct3DRMCreate(&d3drm1);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x).\n", hr);
    ref1 = get_refcount((IUnknown *)d3drm1);
    cref1 = get_refcount((IUnknown *)clipper);

    hr = IDirect3DRM_QueryInterface(d3drm1, &IID_IDirect3DRM3, (void **)&d3drm3);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM3 interface (hr = %x).\n", hr);
    ref2 = get_refcount((IUnknown *)d3drm3);

    hr = IDirect3DRM3_CreateDeviceFromClipper(d3drm3, clipper, &driver, 0, 0, &device3);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %x.\n", hr);
    ok(device3 == NULL, "Expected device returned == NULL, got %p.\n", device3);

    /* If NULL is passed for clipper, CreateDeviceFromClipper returns D3DRMERR_BADVALUE */
    hr = IDirect3DRM3_CreateDeviceFromClipper(d3drm3, NULL, &driver, 300, 200, &device3);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %x.\n", hr);

    hr = IDirect3DRM3_CreateDeviceFromClipper(d3drm3, clipper, &driver, 300, 200, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %x.\n", hr);

    hr = IDirect3DRM3_CreateDeviceFromClipper(d3drm3, clipper, &driver, 300, 200, &device3);
    ok(hr == D3DRM_OK, "Cannot create IDirect3DRMDevice3 interface (hr = %x).\n", hr);
    ref3 = get_refcount((IUnknown *)d3drm1);
    ok(ref3 > ref1, "expected ref3 > ref1, got ref1 = %u , ref3 = %u.\n", ref1, ref3);
    ref3 = get_refcount((IUnknown *)d3drm3);
    ok(ref3 == ref2, "expected ref3 == ref2, got ref2 = %u , ref3 = %u.\n", ref2, ref3);
    cref2 = get_refcount((IUnknown *)clipper);
    ok(cref2 > cref1, "expected cref2 > cref1, got cref1 = %u , cref2 = %u.\n", cref1, cref2);
    ret_val = IDirect3DRMDevice3_GetWidth(device3);
    ok(ret_val == 300, "Expected device width = 300, got %u.\n", ret_val);
    ret_val = IDirect3DRMDevice3_GetHeight(device3);
    ok(ret_val == 200, "Expected device height == 200, got %u.\n", ret_val);

    /* Fetch immediate mode device in order to access render target */
    hr = IDirect3DRMDevice3_GetDirect3DDevice2(device3, &d3ddevice2);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DDevice2 interface (hr = %x).\n", hr);

    hr = IDirect3DDevice2_GetRenderTarget(d3ddevice2, &surface);
    ok(hr == DD_OK, "Cannot get surface to the render target (hr = %x).\n", hr);

    hr = IDirectDrawSurface_GetClipper(surface, &d3drm_clipper);
    ok(hr == DDERR_NOCLIPPERATTACHED, "Expected hr == DDERR_NOCLIPPERATTACHED, got %x.\n", hr);

    /* Check if CreateDeviceFromClipper creates a primary surface and attaches the clipper to it */
    hr = IDirectDrawSurface_QueryInterface(surface, &IID_IDirectDrawSurface7, (void **)&surface7);
    ok(hr == DD_OK, "Cannot get IDirectDrawSurface7 interface (hr = %x).\n", hr);
    IDirectDrawSurface7_GetDDInterface(surface7, (void **)&unknown);
    hr = IUnknown_QueryInterface(unknown, &IID_IDirectDraw, (void **)&ddraw);
    ok(hr == DD_OK, "Cannot get IDirectDraw interface (hr = %x).\n", hr);
    IUnknown_Release(unknown);
    hr = IDirectDraw_EnumSurfaces(ddraw, DDENUMSURFACES_ALL | DDENUMSURFACES_DOESEXIST,
            NULL, &d3drm_primary, surface_callback);
    ok(hr == DD_OK, "Failed to enumerate surfaces (hr = %x).\n", hr);
    ok(d3drm_primary != NULL, "No primary surface was enumerated.\n");
    hr = IDirectDrawSurface_GetClipper(d3drm_primary, &d3drm_clipper);
    ok(hr == DD_OK, "Cannot get attached clipper from primary surface (hr = %x).\n", hr);
    ok(d3drm_clipper == clipper, "Expected clipper returned == %p, got %p.\n", clipper , d3drm_clipper);

    IDirectDrawClipper_Release(d3drm_clipper);
    IDirectDrawSurface_Release(d3drm_primary);
    IDirectDrawSurface7_Release(surface7);
    IDirectDraw_Release(ddraw);

    /* Check properties of render target and depth surface */
    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &surface_desc);
    ok(hr == DD_OK, "Cannot get surface desc structure (hr = %x).\n", hr);

    ok((surface_desc.dwWidth == 300) && (surface_desc.dwHeight == 200), "Expected surface dimensions = 300, 200, got %u, %u.\n",
            surface_desc.dwWidth, surface_desc.dwHeight);
    ok((surface_desc.ddsCaps.dwCaps & (DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE)) == (DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE),
            "Expected caps containing %x, got %x.\n", DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE, surface_desc.ddsCaps.dwCaps);
    expected_flags = DDSD_PIXELFORMAT | DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH;
    ok(surface_desc.dwFlags == expected_flags, "Expected %x for flags, got %x.\n", expected_flags, surface_desc.dwFlags);

    hr = DirectDrawCreate(NULL, &ddraw, NULL);
    ok(hr == DD_OK, "Cannot get IDirectDraw interface (hr = %x).\n", hr);
    desc.dwSize = sizeof(desc);
    hr = IDirectDraw_GetDisplayMode(ddraw, &desc);
    ok(hr == DD_OK, "Cannot get IDirectDraw display mode (hr = %x)\n", hr);
    ok(desc.ddpfPixelFormat.dwRGBBitCount == surface_desc.ddpfPixelFormat.dwRGBBitCount, "Expected %u bpp, got %u bpp.\n",
            surface_desc.ddpfPixelFormat.dwRGBBitCount, desc.ddpfPixelFormat.dwRGBBitCount);

    hr = IDirectDrawSurface_GetAttachedSurface(surface, &caps, &ds);
    ok(hr == DD_OK, "Cannot get attached depth surface (hr = %x).\n", hr);

    desc.dwSize = sizeof(desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(ds, &desc);
    ok(hr == DD_OK, "Cannot get z surface desc structure (hr = %x).\n", hr);

    ok((desc.dwWidth == 300) && (desc.dwHeight == 200), "Expected surface dimensions = 300, 200, got %u, %u.\n",
            desc.dwWidth, desc.dwHeight);
    ok((desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER) == DDSCAPS_ZBUFFER, "Expected caps containing %x, got %x.\n", DDSCAPS_ZBUFFER, desc.ddsCaps.dwCaps);
    expected_flags = DDSD_ZBUFFERBITDEPTH | DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH;
    ok(desc.dwFlags == expected_flags, "Expected %x for flags, got %x.\n", expected_flags, desc.dwFlags);
    ok(desc.dwZBufferBitDepth == 16, "Expected 16 for Z buffer bit depth, got %u.\n", desc.dwZBufferBitDepth);
    ok(desc.ddpfPixelFormat.dwStencilBitMask == 0, "Expected 0 stencil bits, got %x.\n", desc.ddpfPixelFormat.dwStencilBitMask);

    /* Release old objects and check refcount of device and clipper */
    IDirectDrawSurface_Release(ds);
    ds = NULL;
    IDirectDrawSurface_Release(surface);
    surface = NULL;
    IDirect3DDevice2_Release(d3ddevice2);
    d3ddevice2 = NULL;
    IDirect3DRMDevice3_Release(device3);
    ref3 = get_refcount((IUnknown *)d3drm1);
    ok(ref1 == ref3, "expected ref1 == ref3, got ref1 = %u, ref3 = %u.\n", ref1, ref3);
    ref3 = get_refcount((IUnknown *)d3drm3);
    ok(ref3 == ref2, "expected ref3 == ref2, got ref2 = %u , ref3 = %u.\n", ref2, ref3);
    cref2 = get_refcount((IUnknown *)clipper);
    ok(cref1 == cref2, "expected cref1 == cref2, got cref1 = %u, cref2 = %u.\n", cref1, cref2);

    /* Test if render target format follows the screen format */
    hr = IDirectDraw_GetDisplayMode(ddraw, &desc);
    ok(hr == DD_OK, "Cannot get IDirectDraw display mode (hr = %x)\n", hr);
    hr = IDirectDraw_SetDisplayMode(ddraw, desc.dwWidth, desc.dwHeight, 16);
    ok(hr == DD_OK, "Cannot set display mode to 16bpp (hr = %x).\n", hr);

    hr = IDirectDraw_GetDisplayMode(ddraw, &desc);
    ok(hr == DD_OK, "Cannot get IDirectDraw display mode (hr = %x)\n", hr);
    ok(desc.ddpfPixelFormat.dwRGBBitCount == 16, "Expected 16 bpp, got %u.\n", desc.ddpfPixelFormat.dwRGBBitCount);

    hr = IDirect3DRM3_CreateDeviceFromClipper(d3drm3, clipper, &driver, rc.right, rc.bottom, &device3);
    ok(hr == D3DRM_OK, "Cannot create IDirect3DRMDevice3 interface (hr = %x).\n", hr);

    hr = IDirect3DRMDevice3_GetDirect3DDevice2(device3, &d3ddevice2);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DDevice2 interface (hr = %x).\n", hr);

    hr = IDirect3DDevice2_GetRenderTarget(d3ddevice2, &surface);
    ok(hr == DD_OK, "Cannot get surface to the render target (hr = %x).\n", hr);

    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &surface_desc);
    ok(hr == DD_OK, "Cannot get surface desc structure (hr = %x).\n", hr);
    todo_wine ok(surface_desc.ddpfPixelFormat.dwRGBBitCount == 16, "Expected 16bpp, got %ubpp.\n",
            surface_desc.ddpfPixelFormat.dwRGBBitCount);

    hr = IDirectDraw2_RestoreDisplayMode(ddraw);
    ok(SUCCEEDED(hr), "RestoreDisplayMode failed, hr %#x.\n", hr);

    IDirectDrawSurface_Release(surface);
    IDirect3DDevice2_Release(d3ddevice2);
    IDirect3DRMDevice3_Release(device3);
    IDirect3DRM3_Release(d3drm3);
    IDirect3DRM_Release(d3drm1);
    IDirectDrawClipper_Release(clipper);
    IDirectDraw_Release(ddraw);
    DestroyWindow(window);
}

static void test_create_device_from_surface1(void)
{
    DDSCAPS caps = { DDSCAPS_ZBUFFER };
    DDSURFACEDESC desc;
    IDirectDraw *ddraw = NULL;
    IDirect3DRM *d3drm1 = NULL;
    IDirect3DRMDevice *device1 = (IDirect3DRMDevice *)0xdeadbeef;
    IDirect3DDevice *d3ddevice1 = NULL;
    IDirectDrawSurface *surface = NULL, *ds = NULL, *d3drm_surface = NULL, *d3drm_ds = NULL;
    DWORD expected_flags, ret_val;
    HWND window;
    GUID driver = IID_IDirect3DRGBDevice;
    ULONG ref1, ref2, surface_ref1, surface_ref2;
    RECT rc;
    BOOL use_sysmem_zbuffer = FALSE;
    HRESULT hr;

    hr = DirectDrawCreate(NULL, &ddraw, NULL);
    ok(hr == DD_OK, "Cannot get IDirectDraw interface (hr = %x).\n", hr);

    window = create_window();
    GetClientRect(window, &rc);

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    hr = Direct3DRMCreate(&d3drm1);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x).\n", hr);
    ref1 = get_refcount((IUnknown *)d3drm1);

    /* Create a surface and use it to create the retained mode device. */
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    desc.dwWidth = rc.right;
    desc.dwHeight = rc.bottom;

    hr = IDirectDraw_CreateSurface(ddraw, &desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirect3DRM_CreateDeviceFromSurface(d3drm1, &driver, ddraw, surface, &device1);
    ok(hr == DDERR_INVALIDCAPS, "Expected hr == DDERR_INVALIDCAPS, got %x.\n", hr);
    ok(device1 == NULL, "Expected device returned == NULL, got %p.\n", device1);
    IDirectDrawSurface_Release(surface);

    desc.ddsCaps.dwCaps |= DDSCAPS_3DDEVICE;
    hr = IDirectDraw_CreateSurface(ddraw, &desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    surface_ref1 = get_refcount((IUnknown *)surface);

    hr = IDirect3DRM_CreateDeviceFromSurface(d3drm1, &driver, ddraw, surface, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == DDERR_BADVALUE, got %x.\n", hr);
    hr = IDirect3DRM_CreateDeviceFromSurface(d3drm1, &driver, ddraw, NULL, &device1);
    ok(hr == D3DRMERR_BADDEVICE, "Expected hr == DDERR_BADDEVICE, got %x.\n", hr);
    hr = IDirect3DRM_CreateDeviceFromSurface(d3drm1, &driver, NULL, surface, &device1);
    ok(hr == D3DRMERR_BADDEVICE, "Expected hr == DDERR_BADDEVICE, got %x.\n", hr);

    hr = IDirect3DRM_CreateDeviceFromSurface(d3drm1, &driver, ddraw, surface, &device1);
    ok(SUCCEEDED(hr), "Cannot create IDirect3DRMDevice interface (hr = %x).\n", hr);
    ref2 = get_refcount((IUnknown *)d3drm1);
    ok(ref2 > ref1, "expected ref2 > ref1, got ref1 = %u , ref2 = %u.\n", ref1, ref2);
    surface_ref2 = get_refcount((IUnknown *)surface);
    ok(surface_ref2 > surface_ref1, "Expected surface_ref2 > surface_ref1, got surface_ref1 = %u, surface_ref2 = %u.\n", surface_ref1, surface_ref2);
    ret_val = IDirect3DRMDevice_GetWidth(device1);
    ok(ret_val == rc.right, "Expected device width = 300, got %u.\n", ret_val);
    ret_val = IDirect3DRMDevice_GetHeight(device1);
    ok(ret_val == rc.bottom, "Expected device height == 200, got %u.\n", ret_val);

    /* Check if CreateDeviceFromSurface creates a primary surface */
    hr = IDirectDraw_EnumSurfaces(ddraw, DDENUMSURFACES_ALL | DDENUMSURFACES_DOESEXIST,
            NULL, &d3drm_surface, surface_callback);
    ok(hr == DD_OK, "Failed to enumerate surfaces (hr = %x).\n", hr);
    ok(d3drm_surface == NULL, "No primary surface should have enumerated (%p).\n", d3drm_surface);

    hr = IDirect3DRMDevice_GetDirect3DDevice(device1, &d3ddevice1);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DDevice interface (hr = %x).\n", hr);

    hr = IDirect3DDevice_QueryInterface(d3ddevice1, &IID_IDirectDrawSurface, (void **)&d3drm_surface);
    ok(hr == DD_OK, "Cannot get surface to the render target (hr = %x).\n", hr);
    ok(surface == d3drm_surface, "Expected surface returned == %p, got %p.\n", surface, d3drm_surface);

    /* Check properties of attached depth surface */
    hr = IDirectDrawSurface_GetAttachedSurface(d3drm_surface, &caps, &ds);
    ok(hr == DD_OK, "Cannot get attached depth surface (hr = %x).\n", hr);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(ds, &desc);
    ok(hr == DD_OK, "Cannot get z surface desc structure (hr = %x).\n", hr);

    use_sysmem_zbuffer = desc.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY;
    ok((desc.dwWidth == rc.right) && (desc.dwHeight == rc.bottom), "Expected surface dimensions = %u, %u, got %u, %u.\n",
            rc.right, rc.bottom, desc.dwWidth, desc.dwHeight);
    ok(desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER, "Expected caps containing %x, got %x.\n", DDSCAPS_ZBUFFER, desc.ddsCaps.dwCaps);
    expected_flags = DDSD_ZBUFFERBITDEPTH | DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH;
    ok(desc.dwFlags == expected_flags, "Expected %x for flags, got %x.\n", expected_flags, desc.dwFlags);

    IDirectDrawSurface_Release(ds);
    IDirect3DDevice_Release(d3ddevice1);
    IDirectDrawSurface_Release(d3drm_surface);

    IDirect3DRMDevice_Release(device1);
    ref2 = get_refcount((IUnknown *)d3drm1);
    ok(ref1 == ref2, "expected ref1 == ref2, got ref1 = %u, ref2 = %u.\n", ref1, ref2);
    surface_ref2 = get_refcount((IUnknown *)surface);
    ok(surface_ref2 == surface_ref1, "Expected surface_ref2 == surface_ref1, got surface_ref1 = %u, surface_ref2 = %u.\n",
            surface_ref1, surface_ref2);
    hr = IDirectDrawSurface_GetAttachedSurface(surface, &caps, &ds);
    ok(hr == DD_OK, "Cannot get attached depth surface (hr = %x).\n", hr);
    /*The render target still holds a reference to ds as the depth surface remains attached to it, so refcount will be 1*/
    ref1 = IDirectDrawSurface_Release(ds);
    ok(ref1 == 1, "Expected ref1 == 1, got %u.\n", ref1);
    ref1 = IDirectDrawSurface_Release(surface);
    ok(ref1 == 0, "Expected Render target refcount == 0, got %u.\n", ref1);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    desc.dwWidth = rc.right;
    desc.dwHeight = rc.bottom;

    hr = IDirectDraw_CreateSurface(ddraw, &desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_ZBUFFERBITDEPTH | DDSD_WIDTH | DDSD_HEIGHT;
    desc.ddsCaps.dwCaps = DDSCAPS_ZBUFFER | (use_sysmem_zbuffer ? DDSCAPS_SYSTEMMEMORY : 0);
    desc.dwZBufferBitDepth = 16;
    desc.dwWidth = rc.right;
    desc.dwHeight = rc.bottom;
    hr = IDirectDraw_CreateSurface(ddraw, &desc, &ds, NULL);
    ok(hr == DD_OK, "Cannot create depth surface (hr = %x).\n", hr);
    hr = IDirectDrawSurface_AddAttachedSurface(surface, ds);
    ok(SUCCEEDED(hr), "Failed to attach depth buffer, hr %#x.\n", hr);

    hr = IDirect3DRM_CreateDeviceFromSurface(d3drm1, &driver, ddraw, surface, &device1);
    ok(SUCCEEDED(hr), "Cannot create IDirect3DRMDevice interface (hr = %x).\n", hr);

    hr = IDirect3DRMDevice2_GetDirect3DDevice(device1, &d3ddevice1);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DDevice interface (hr = %x).\n", hr);

    hr = IDirect3DDevice_QueryInterface(d3ddevice1, &IID_IDirectDrawSurface, (void **)&d3drm_surface);
    ok(hr == DD_OK, "Cannot get surface to the render target (hr = %x).\n", hr);
    ok(surface == d3drm_surface, "Expected surface returned == %p, got %p.\n", surface, d3drm_surface);

    /* Check if depth surface matches the one we created  */
    hr = IDirectDrawSurface_GetAttachedSurface(d3drm_surface, &caps, &d3drm_ds);
    ok(hr == DD_OK, "Cannot get attached depth surface (hr = %x).\n", hr);
    ok(ds == d3drm_ds, "Expected depth surface (%p) == surface created internally (%p).\n", ds, d3drm_ds);

    IDirectDrawSurface_Release(d3drm_ds);
    IDirectDrawSurface_Release(d3drm_surface);
    IDirectDrawSurface_Release(ds);

    IDirect3DDevice_Release(d3ddevice1);
    IDirect3DRMDevice_Release(device1);
    hr = IDirectDrawSurface_GetAttachedSurface(surface, &caps, &ds);
    ok(hr == DD_OK, "Cannot get attached depth surface (hr = %x).\n", hr);
    /*The render target still holds a reference to ds as the depth surface remains attached to it, so refcount will be 1*/
    ref1 = IDirectDrawSurface_Release(ds);
    ok(ref1 == 1, "Expected ref1 == 1, got %u.\n", ref1);
    ref1 = IDirectDrawSurface_Release(surface);
    ok(ref1 == 0, "Expected Render target refcount == 0, got %u.\n", ref1);
    IDirect3DRM_Release(d3drm1);
    IDirectDraw_Release(ddraw);
    DestroyWindow(window);
}

static void test_create_device_from_surface2(void)
{
    DDSCAPS caps = { DDSCAPS_ZBUFFER };
    DDSURFACEDESC desc;
    IDirectDraw *ddraw = NULL;
    IDirect3DRM *d3drm1 = NULL;
    IDirect3DRM2 *d3drm2 = NULL;
    IDirect3DRMDevice2 *device2 = (IDirect3DRMDevice2 *)0xdeadbeef;
    IDirect3DDevice2 *d3ddevice2 = NULL;
    IDirectDrawSurface *surface = NULL, *ds = NULL, *d3drm_surface = NULL, *d3drm_ds = NULL;
    DWORD expected_flags, ret_val;
    HWND window;
    GUID driver = IID_IDirect3DRGBDevice;
    ULONG ref1, ref2, ref3, surface_ref1, surface_ref2;
    RECT rc;
    BOOL use_sysmem_zbuffer = FALSE;
    HRESULT hr;

    hr = DirectDrawCreate(NULL, &ddraw, NULL);
    ok(hr == DD_OK, "Cannot get IDirectDraw interface (hr = %x).\n", hr);

    window = create_window();
    GetClientRect(window, &rc);

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    hr = Direct3DRMCreate(&d3drm1);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x).\n", hr);
    ref1 = get_refcount((IUnknown *)d3drm1);

    hr = IDirect3DRM_QueryInterface(d3drm1, &IID_IDirect3DRM2, (void **)&d3drm2);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM2 interface (hr = %x).\n", hr);
    ref2 = get_refcount((IUnknown *)d3drm2);

    /* Create a surface and use it to create the retained mode device. */
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    desc.dwWidth = rc.right;
    desc.dwHeight = rc.bottom;

    hr = IDirectDraw_CreateSurface(ddraw, &desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirect3DRM2_CreateDeviceFromSurface(d3drm2, &driver, ddraw, surface, &device2);
    ok(hr == DDERR_INVALIDCAPS, "Expected hr == DDERR_INVALIDCAPS, got %x.\n", hr);
    ok(device2 == NULL, "Expected device returned == NULL, got %p.\n", device2);
    IDirectDrawSurface_Release(surface);

    desc.ddsCaps.dwCaps |= DDSCAPS_3DDEVICE;
    hr = IDirectDraw_CreateSurface(ddraw, &desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    surface_ref1 = get_refcount((IUnknown *)surface);

    hr = IDirect3DRM2_CreateDeviceFromSurface(d3drm2, &driver, ddraw, surface, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == DDERR_BADVALUE, got %x.\n", hr);
    hr = IDirect3DRM2_CreateDeviceFromSurface(d3drm2, &driver, ddraw, NULL, &device2);
    ok(hr == D3DRMERR_BADDEVICE, "Expected hr == DDERR_BADDEVICE, got %x.\n", hr);
    hr = IDirect3DRM2_CreateDeviceFromSurface(d3drm2, &driver, NULL, surface, &device2);
    ok(hr == D3DRMERR_BADDEVICE, "Expected hr == DDERR_BADDEVICE, got %x.\n", hr);

    hr = IDirect3DRM2_CreateDeviceFromSurface(d3drm2, &driver, ddraw, surface, &device2);
    ok(SUCCEEDED(hr), "Cannot create IDirect3DRMDevice2 interface (hr = %x).\n", hr);
    ref3 = get_refcount((IUnknown *)d3drm1);
    ok(ref3 > ref1, "expected ref3 > ref1, got ref1 = %u , ref3 = %u.\n", ref1, ref3);
    ref3 = get_refcount((IUnknown *)d3drm2);
    ok(ref3 == ref2, "expected ref3 == ref2, got ref2 = %u , ref3 = %u.\n", ref2, ref3);
    surface_ref2 = get_refcount((IUnknown *)surface);
    ok(surface_ref2 > surface_ref1, "Expected surface_ref2 > surface_ref1, got surface_ref1 = %u, surface_ref2 = %u.\n", surface_ref1, surface_ref2);
    ret_val = IDirect3DRMDevice2_GetWidth(device2);
    ok(ret_val == rc.right, "Expected device width = 300, got %u.\n", ret_val);
    ret_val = IDirect3DRMDevice2_GetHeight(device2);
    ok(ret_val == rc.bottom, "Expected device height == 200, got %u.\n", ret_val);

    /* Check if CreateDeviceFromSurface creates a primary surface */
    hr = IDirectDraw_EnumSurfaces(ddraw, DDENUMSURFACES_ALL | DDENUMSURFACES_DOESEXIST,
            NULL, &d3drm_surface, surface_callback);
    ok(hr == DD_OK, "Failed to enumerate surfaces (hr = %x).\n", hr);
    ok(d3drm_surface == NULL, "No primary surface should have enumerated (%p).\n", d3drm_surface);

    hr = IDirect3DRMDevice2_GetDirect3DDevice2(device2, &d3ddevice2);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DDevice2 interface (hr = %x).\n", hr);

    hr = IDirect3DDevice2_GetRenderTarget(d3ddevice2, &d3drm_surface);
    ok(hr == DD_OK, "Cannot get surface to the render target (hr = %x).\n", hr);
    ok(surface == d3drm_surface, "Expected surface returned == %p, got %p.\n", surface, d3drm_surface);

    /* Check properties of attached depth surface */
    hr = IDirectDrawSurface_GetAttachedSurface(d3drm_surface, &caps, &ds);
    ok(hr == DD_OK, "Cannot get attached depth surface (hr = %x).\n", hr);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(ds, &desc);
    ok(hr == DD_OK, "Cannot get z surface desc structure (hr = %x).\n", hr);

    use_sysmem_zbuffer = desc.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY;
    ok((desc.dwWidth == rc.right) && (desc.dwHeight == rc.bottom), "Expected surface dimensions = %u, %u, got %u, %u.\n",
            rc.right, rc.bottom, desc.dwWidth, desc.dwHeight);
    ok(desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER, "Expected caps containing %x, got %x.\n", DDSCAPS_ZBUFFER, desc.ddsCaps.dwCaps);
    expected_flags = DDSD_ZBUFFERBITDEPTH | DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH;
    ok(desc.dwFlags == expected_flags, "Expected %x for flags, got %x.\n", expected_flags, desc.dwFlags);

    IDirectDrawSurface_Release(ds);
    IDirect3DDevice2_Release(d3ddevice2);
    IDirectDrawSurface_Release(d3drm_surface);

    IDirect3DRMDevice2_Release(device2);
    ref3 = get_refcount((IUnknown *)d3drm1);
    ok(ref1 == ref3, "expected ref1 == ref3, got ref1 = %u, ref3 = %u.\n", ref1, ref3);
    ref3 = get_refcount((IUnknown *)d3drm2);
    ok(ref3 == ref2, "expected ref3 == ref2, got ref2 = %u , ref3 = %u.\n", ref2, ref3);
    surface_ref2 = get_refcount((IUnknown *)surface);
    ok(surface_ref2 == surface_ref1, "Expected surface_ref2 == surface_ref1, got surface_ref1 = %u, surface_ref2 = %u.\n",
            surface_ref1, surface_ref2);
    hr = IDirectDrawSurface_GetAttachedSurface(surface, &caps, &ds);
    ok(hr == DD_OK, "Cannot get attached depth surface (hr = %x).\n", hr);
    /*The render target still holds a reference to ds as the depth surface remains attached to it, so refcount will be 1*/
    ref1 = IDirectDrawSurface_Release(ds);
    ok(ref1 == 1, "Expected ref1 == 1, got %u.\n", ref1);

    ref1 = IDirectDrawSurface_Release(surface);
    ok(ref1 == 0, "Expected Render target refcount == 0, got %u.\n", ref1);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    desc.dwWidth = rc.right;
    desc.dwHeight = rc.bottom;

    hr = IDirectDraw_CreateSurface(ddraw, &desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_ZBUFFERBITDEPTH | DDSD_WIDTH | DDSD_HEIGHT;
    desc.ddsCaps.dwCaps = DDSCAPS_ZBUFFER | (use_sysmem_zbuffer ? DDSCAPS_SYSTEMMEMORY : 0);
    desc.dwZBufferBitDepth = 16;
    desc.dwWidth = rc.right;
    desc.dwHeight = rc.bottom;
    hr = IDirectDraw_CreateSurface(ddraw, &desc, &ds, NULL);
    ok(hr == DD_OK, "Cannot create depth surface (hr = %x).\n", hr);
    hr = IDirectDrawSurface_AddAttachedSurface(surface, ds);
    ok(SUCCEEDED(hr), "Failed to attach depth buffer, hr %#x.\n", hr);

    hr = IDirect3DRM2_CreateDeviceFromSurface(d3drm2, &driver, ddraw, surface, &device2);
    ok(SUCCEEDED(hr), "Cannot create IDirect3DRMDevice2 interface (hr = %x).\n", hr);

    hr = IDirect3DRMDevice2_GetDirect3DDevice2(device2, &d3ddevice2);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DDevice2 interface (hr = %x).\n", hr);

    hr = IDirect3DDevice2_GetRenderTarget(d3ddevice2, &d3drm_surface);
    ok(hr == DD_OK, "Cannot get surface to the render target (hr = %x).\n", hr);
    ok(surface == d3drm_surface, "Expected surface returned == %p, got %p.\n", surface, d3drm_surface);

    /* Check if depth surface matches the one we created  */
    hr = IDirectDrawSurface_GetAttachedSurface(d3drm_surface, &caps, &d3drm_ds);
    ok(hr == DD_OK, "Cannot get attached depth surface (hr = %x).\n", hr);
    ok(ds == d3drm_ds, "Expected depth surface (%p) == surface created internally (%p).\n", ds, d3drm_ds);

    IDirectDrawSurface_Release(d3drm_ds);
    IDirectDrawSurface_Release(d3drm_surface);
    IDirectDrawSurface_Release(ds);

    IDirect3DDevice2_Release(d3ddevice2);
    IDirect3DRMDevice2_Release(device2);
    hr = IDirectDrawSurface_GetAttachedSurface(surface, &caps, &ds);
    ok(hr == DD_OK, "Cannot get attached depth surface (hr = %x).\n", hr);
    /*The render target still holds a reference to ds as the depth surface remains attached to it, so refcount will be 1*/
    ref1 = IDirectDrawSurface_Release(ds);
    ok(ref1 == 1, "Expected ref1 == 1, got %u.\n", ref1);
    ref1 = IDirectDrawSurface_Release(surface);
    ok(ref1 == 0, "Expected Render target refcount == 0, got %u.\n", ref1);
    IDirect3DRM2_Release(d3drm2);
    IDirect3DRM_Release(d3drm1);
    IDirectDraw_Release(ddraw);
    DestroyWindow(window);
}

static void test_create_device_from_surface3(void)
{
    DDSCAPS caps = { DDSCAPS_ZBUFFER };
    DDSURFACEDESC desc;
    IDirectDraw *ddraw = NULL;
    IDirect3DRM *d3drm1 = NULL;
    IDirect3DRM3 *d3drm3 = NULL;
    IDirect3DRMDevice3 *device3 = (IDirect3DRMDevice3 *)0xdeadbeef;
    IDirect3DDevice2 *d3ddevice2 = NULL;
    IDirectDrawSurface *surface = NULL, *ds = NULL, *d3drm_surface = NULL, *d3drm_ds = NULL;
    DWORD expected_flags, ret_val;
    HWND window;
    GUID driver = IID_IDirect3DRGBDevice;
    ULONG ref1, ref2, ref3, surface_ref1, surface_ref2;
    RECT rc;
    BOOL use_sysmem_zbuffer = FALSE;
    HRESULT hr;

    hr = DirectDrawCreate(NULL, &ddraw, NULL);
    ok(hr == DD_OK, "Cannot get IDirectDraw interface (hr = %x).\n", hr);

    window = create_window();
    GetClientRect(window, &rc);

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    hr = Direct3DRMCreate(&d3drm1);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x).\n", hr);
    ref1 = get_refcount((IUnknown *)d3drm1);

    hr = IDirect3DRM_QueryInterface(d3drm1, &IID_IDirect3DRM3, (void **)&d3drm3);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM3 interface (hr = %x).\n", hr);
    ref2 = get_refcount((IUnknown *)d3drm3);

    /* Create a surface and use it to create the retained mode device. */
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    desc.dwWidth = rc.right;
    desc.dwHeight = rc.bottom;

    hr = IDirectDraw_CreateSurface(ddraw, &desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirect3DRM3_CreateDeviceFromSurface(d3drm3, &driver, ddraw, surface, 0, &device3);
    ok(hr == DDERR_INVALIDCAPS, "Expected hr == DDERR_INVALIDCAPS, got %x.\n", hr);
    ok(device3 == NULL, "Expected device returned == NULL, got %p.\n", device3);
    IDirectDrawSurface_Release(surface);

    desc.ddsCaps.dwCaps |= DDSCAPS_3DDEVICE;
    hr = IDirectDraw_CreateSurface(ddraw, &desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    surface_ref1 = get_refcount((IUnknown *)surface);

    hr = IDirect3DRM3_CreateDeviceFromSurface(d3drm3, &driver, ddraw, surface, 0, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == DDERR_BADVALUE, got %x.\n", hr);
    hr = IDirect3DRM3_CreateDeviceFromSurface(d3drm3, &driver, ddraw, NULL, 0, &device3);
    ok(hr == D3DRMERR_BADDEVICE, "Expected hr == DDERR_BADDEVICE, got %x.\n", hr);
    hr = IDirect3DRM3_CreateDeviceFromSurface(d3drm3, &driver, NULL, surface, 0, &device3);
    ok(hr == D3DRMERR_BADDEVICE, "Expected hr == DDERR_BADDEVICE, got %x.\n", hr);

    hr = IDirect3DRM3_CreateDeviceFromSurface(d3drm3, &driver, ddraw, surface, 0, &device3);
    ok(SUCCEEDED(hr), "Cannot create IDirect3DRMDevice3 interface (hr = %x).\n", hr);
    ref3 = get_refcount((IUnknown *)d3drm1);
    ok(ref3 > ref1, "expected ref3 > ref1, got ref1 = %u , ref3 = %u.\n", ref1, ref3);
    ref3 = get_refcount((IUnknown *)d3drm3);
    ok(ref3 == ref2, "expected ref3 == ref2, got ref2 = %u , ref3 = %u.\n", ref2, ref3);
    surface_ref2 = get_refcount((IUnknown *)surface);
    ok(surface_ref2 > surface_ref1, "Expected surface_ref2 > surface_ref1, got surface_ref1 = %u, surface_ref2 = %u.\n", surface_ref1, surface_ref2);
    ret_val = IDirect3DRMDevice3_GetWidth(device3);
    ok(ret_val == rc.right, "Expected device width = 300, got %u.\n", ret_val);
    ret_val = IDirect3DRMDevice3_GetHeight(device3);
    ok(ret_val == rc.bottom, "Expected device height == 200, got %u.\n", ret_val);

    /* Check if CreateDeviceFromSurface creates a primary surface */
    hr = IDirectDraw_EnumSurfaces(ddraw, DDENUMSURFACES_ALL | DDENUMSURFACES_DOESEXIST,
            NULL, &d3drm_surface, surface_callback);
    ok(hr == DD_OK, "Failed to enumerate surfaces (hr = %x).\n", hr);
    ok(d3drm_surface == NULL, "No primary surface should have enumerated (%p).\n", d3drm_surface);

    hr = IDirect3DRMDevice3_GetDirect3DDevice2(device3, &d3ddevice2);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DDevice2 interface (hr = %x).\n", hr);

    hr = IDirect3DDevice2_GetRenderTarget(d3ddevice2, &d3drm_surface);
    ok(hr == DD_OK, "Cannot get surface to the render target (hr = %x).\n", hr);
    ok(surface == d3drm_surface, "Expected surface returned == %p, got %p.\n", surface, d3drm_surface);

    /* Check properties of attached depth surface */
    hr = IDirectDrawSurface_GetAttachedSurface(d3drm_surface, &caps, &ds);
    ok(hr == DD_OK, "Cannot get attached depth surface (hr = %x).\n", hr);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(ds, &desc);
    ok(hr == DD_OK, "Cannot get z surface desc structure (hr = %x).\n", hr);

    use_sysmem_zbuffer = desc.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY;
    ok((desc.dwWidth == rc.right) && (desc.dwHeight == rc.bottom), "Expected surface dimensions = %u, %u, got %u, %u.\n",
            rc.right, rc.bottom, desc.dwWidth, desc.dwHeight);
    ok(desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER, "Expected caps containing %x, got %x.\n", DDSCAPS_ZBUFFER, desc.ddsCaps.dwCaps);
    expected_flags = DDSD_ZBUFFERBITDEPTH | DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH;
    ok(desc.dwFlags == expected_flags, "Expected %x for flags, got %x.\n", expected_flags, desc.dwFlags);

    IDirectDrawSurface_Release(ds);
    IDirect3DDevice2_Release(d3ddevice2);
    IDirectDrawSurface_Release(d3drm_surface);
    IDirect3DRMDevice3_Release(device3);

    ref3 = get_refcount((IUnknown *)d3drm1);
    ok(ref1 == ref3, "expected ref1 == ref3, got ref1 = %u, ref3 = %u.\n", ref1, ref3);
    ref3 = get_refcount((IUnknown *)d3drm3);
    ok(ref3 == ref2, "expected ref3 == ref2, got ref2 = %u , ref3 = %u.\n", ref2, ref3);
    surface_ref2 = get_refcount((IUnknown *)surface);
    ok(surface_ref2 == surface_ref1, "Expected surface_ref2 == surface_ref1, got surface_ref1 = %u, surface_ref2 = %u.\n",
            surface_ref1, surface_ref2);
    /* In version 3, d3drm will destroy all references of the depth surface it created internally. */
    hr = IDirectDrawSurface_GetAttachedSurface(surface, &caps, &ds);
    todo_wine ok(hr == DDERR_NOTFOUND, "Expected hr == DDERR_NOTFOUND, got %x.\n", hr);
    if (SUCCEEDED(hr))
        IDirectDrawSurface_Release(ds);
    ref1 = IDirectDrawSurface_Release(surface);
    ok(ref1 == 0, "Expected Render target refcount == 0, got %u.\n", ref1);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    desc.dwWidth = rc.right;
    desc.dwHeight = rc.bottom;

    hr = IDirectDraw_CreateSurface(ddraw, &desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_ZBUFFERBITDEPTH | DDSD_WIDTH | DDSD_HEIGHT;
    desc.ddsCaps.dwCaps = DDSCAPS_ZBUFFER | (use_sysmem_zbuffer ? DDSCAPS_SYSTEMMEMORY : 0);
    desc.dwZBufferBitDepth = 16;
    desc.dwWidth = rc.right;
    desc.dwHeight = rc.bottom;
    hr = IDirectDraw_CreateSurface(ddraw, &desc, &ds, NULL);
    ok(hr == DD_OK, "Cannot create depth surface (hr = %x).\n", hr);
    hr = IDirectDrawSurface_AddAttachedSurface(surface, ds);
    ok(SUCCEEDED(hr), "Failed to attach depth buffer, hr %#x.\n", hr);

    hr = IDirect3DRM3_CreateDeviceFromSurface(d3drm3, &driver, ddraw, surface, D3DRMDEVICE_NOZBUFFER, &device3);
    ok(SUCCEEDED(hr), "Cannot create IDirect3DRMDevice3 interface (hr = %x).\n", hr);

    hr = IDirect3DRMDevice3_GetDirect3DDevice2(device3, &d3ddevice2);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DDevice2 interface (hr = %x).\n", hr);

    hr = IDirect3DDevice2_GetRenderTarget(d3ddevice2, &d3drm_surface);
    ok(hr == DD_OK, "Cannot get surface to the render target (hr = %x).\n", hr);
    ok(surface == d3drm_surface, "Expected surface returned == %p, got %p.\n", surface, d3drm_surface);

    /* Check if depth surface matches the one we created  */
    hr = IDirectDrawSurface_GetAttachedSurface(d3drm_surface, &caps, &d3drm_ds);
    ok(hr == DD_OK, "Cannot get attached depth surface (hr = %x).\n", hr);
    ok(ds == d3drm_ds, "Expected depth surface (%p) == surface created internally (%p).\n", ds, d3drm_ds);

    IDirectDrawSurface_Release(d3drm_ds);
    IDirectDrawSurface_Release(d3drm_surface);
    IDirectDrawSurface_Release(ds);
    IDirect3DDevice2_Release(d3ddevice2);
    IDirect3DRMDevice3_Release(device3);
    hr = IDirectDrawSurface_GetAttachedSurface(surface, &caps, &ds);
    ok(hr == DD_OK, "Cannot get attached depth surface (hr = %x).\n", hr);
    /* The render target still holds a reference to ds as the depth surface remains attached to it, so refcount will be 1*/
    ref1 = IDirectDrawSurface_Release(ds);
    ok(ref1 == 1, "Expected ref1 == 1, got %u.\n", ref1);

    /* What happens if we pass no flags and still attach our own depth surface? */
    hr = IDirect3DRM3_CreateDeviceFromSurface(d3drm3, &driver, ddraw, surface, 0, &device3);
    ok(SUCCEEDED(hr), "Cannot create IDirect3DRMDevice3 interface (hr = %x).\n", hr);

    hr = IDirect3DRMDevice3_GetDirect3DDevice2(device3, &d3ddevice2);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DDevice2 interface (hr = %x).\n", hr);

    hr = IDirect3DDevice2_GetRenderTarget(d3ddevice2, &d3drm_surface);
    ok(hr == DD_OK, "Cannot get surface to the render target (hr = %x).\n", hr);
    ok(surface == d3drm_surface, "Expected surface returned == %p, got %p.\n", surface, d3drm_surface);

    /* Check if depth surface matches the one we created  */
    hr = IDirectDrawSurface_GetAttachedSurface(d3drm_surface, &caps, &d3drm_ds);
    ok(hr == DD_OK, "Cannot get attached depth surface (hr = %x).\n", hr);
    ok(ds == d3drm_ds, "Expected depth surface (%p) == surface created internally (%p).\n", ds, d3drm_ds);

    IDirectDrawSurface_Release(d3drm_ds);
    IDirectDrawSurface_Release(d3drm_surface);
    IDirect3DDevice2_Release(d3ddevice2);
    IDirect3DRMDevice3_Release(device3);
    hr = IDirectDrawSurface_GetAttachedSurface(surface, &caps, &ds);
    ok(hr == DD_OK, "Cannot get attached depth surface (hr = %x).\n", hr);
    /*The render target still holds a reference to ds as the depth surface remains attached to it, so refcount will be 1*/
    ref1 = IDirectDrawSurface_Release(ds);
    ok(ref1 == 1, "Expected ref1 == 1, got %u.\n", ref1);
    ref1 = IDirectDrawSurface_Release(surface);
    ok(ref1 == 0, "Expected Render target refcount == 0, got %u.\n", ref1);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    desc.dwWidth = rc.right;
    desc.dwHeight = rc.bottom;

    hr = IDirectDraw_CreateSurface(ddraw, &desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    /* What happens if we don't pass D3DRMDEVICE_NOZBUFFER and still not attach our own depth surface? */
    hr = IDirect3DRM3_CreateDeviceFromSurface(d3drm3, &driver, ddraw, surface, D3DRMDEVICE_NOZBUFFER, &device3);
    ok(SUCCEEDED(hr), "Cannot create IDirect3DRMDevice3 interface (hr = %x).\n", hr);

    hr = IDirect3DRMDevice3_GetDirect3DDevice2(device3, &d3ddevice2);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DDevice2 interface (hr = %x).\n", hr);

    hr = IDirect3DDevice2_GetRenderTarget(d3ddevice2, &d3drm_surface);
    ok(hr == DD_OK, "Cannot get surface to the render target (hr = %x).\n", hr);
    ok(surface == d3drm_surface, "Expected surface returned == %p, got %p.\n", surface, d3drm_surface);

    /* Check if depth surface matches the one we created  */
    hr = IDirectDrawSurface_GetAttachedSurface(d3drm_surface, &caps, &d3drm_ds);
    ok(hr == DDERR_NOTFOUND, "Expected hr == DDERR_NOTFOUND, got %x).\n", hr);
    IDirectDrawSurface_Release(d3drm_surface);

    IDirect3DDevice2_Release(d3ddevice2);
    IDirect3DRMDevice3_Release(device3);
    ref1 = IDirectDrawSurface_Release(surface);
    ok(ref1 == 0, "Expected Render target refcount == 0, got %u.\n", ref1);
    IDirect3DRM3_Release(d3drm3);
    IDirect3DRM_Release(d3drm1);
    IDirectDraw_Release(ddraw);
    DestroyWindow(window);
}

static IDirect3DDevice *create_device1(IDirectDraw *ddraw, HWND window, IDirectDrawSurface **ds)
{
    static const DWORD z_depths[] = { 32, 24, 16 };
    IDirectDrawSurface *surface;
    IDirect3DDevice *device = NULL;
    DDSURFACEDESC surface_desc;
    unsigned int i;
    HRESULT hr;
    RECT rc;

    GetClientRect(window, &rc);
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    surface_desc.dwWidth = rc.right;
    surface_desc.dwHeight = rc.bottom;

    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    /* We used to use EnumDevices() for this, but it seems
    * D3DDEVICEDESC.dwDeviceZBufferBitDepth only has a very casual
    * relationship with reality. */
    for (i = 0; i < ARRAY_SIZE(z_depths); ++i)
    {
        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags = DDSD_CAPS | DDSD_ZBUFFERBITDEPTH | DDSD_WIDTH | DDSD_HEIGHT;
        surface_desc.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
        U2(surface_desc).dwZBufferBitDepth = z_depths[i];
        surface_desc.dwWidth = rc.right;
        surface_desc.dwHeight = rc.bottom;
        if (FAILED(IDirectDraw_CreateSurface(ddraw, &surface_desc, ds, NULL)))
            continue;

        hr = IDirectDrawSurface_AddAttachedSurface(surface, *ds);
        ok(SUCCEEDED(hr), "Failed to attach depth buffer, hr %#x.\n", hr);
        if (FAILED(hr))
        {
            IDirectDrawSurface_Release(*ds);
            continue;
        }

        if (SUCCEEDED(IDirectDrawSurface_QueryInterface(surface, &IID_IDirect3DHALDevice, (void **)&device)))
            break;

        IDirectDrawSurface_DeleteAttachedSurface(surface, 0, *ds);
        IDirectDrawSurface_Release(*ds);
        *ds = NULL;
    }

    IDirectDrawSurface_Release(surface);
    return device;
}

static void test_create_device_from_d3d1(void)
{
    IDirectDraw *ddraw1 = NULL, *temp_ddraw1;
    IDirect3D *d3d1 = NULL, *temp_d3d1;
    IDirect3DRM *d3drm1 = NULL;
    IDirect3DRMDevice *device1 = (IDirect3DRMDevice *)0xdeadbeef;
    IDirect3DRMDevice2 *device2;
    IDirect3DRMDevice3 *device3;
    IDirect3DDevice *d3ddevice1 = NULL, *d3drm_d3ddevice1 = NULL, *temp_d3ddevice1;
    IDirect3DDevice2 *d3ddevice2 = (IDirect3DDevice2 *)0xdeadbeef;
    IDirectDrawSurface *surface = NULL, *ds = NULL, *d3drm_ds = NULL;
    DWORD expected_flags, ret_val;
    DDSCAPS caps = { DDSCAPS_ZBUFFER };
    DDSURFACEDESC desc;
    RECT rc;
    HWND window;
    ULONG ref1, ref2, ref3, ref4, device_ref1, device_ref2, d3d_ref1, d3d_ref2;
    HRESULT hr;

    hr = DirectDrawCreate(NULL, &ddraw1, NULL);
    ok(hr == DD_OK, "Cannot get IDirectDraw interface (hr = %x).\n", hr);

    window = create_window();
    GetClientRect(window, &rc);

    hr = IDirectDraw_QueryInterface(ddraw1, &IID_IDirect3D, (void **)&d3d1);
    ok(hr == DD_OK, "Cannot get IDirect3D2 interface (hr = %x).\n", hr);
    d3d_ref1 = get_refcount((IUnknown *)d3d1);

    /* Create the immediate mode device */
    d3ddevice1 = create_device1(ddraw1, window, &ds);
    if (d3ddevice1 == NULL)
    {
        win_skip("Cannot create IM device, skipping tests.\n");
        IDirect3D_Release(d3d1);
        IDirectDraw_Release(ddraw1);
        return;
    }
    device_ref1 = get_refcount((IUnknown *)d3ddevice1);

    hr = Direct3DRMCreate(&d3drm1);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x).\n", hr);
    ref1 = get_refcount((IUnknown *)d3drm1);

    hr = IDirect3DRM_CreateDeviceFromD3D(d3drm1, NULL, d3ddevice1, &device1);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %x.\n", hr);
    ok(device1 == NULL, "Expected device returned == NULL, got %p.\n", device1);
    hr = IDirect3DRM_CreateDeviceFromD3D(d3drm1, d3d1, NULL, &device1);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %x.\n", hr);
    hr = IDirect3DRM_CreateDeviceFromD3D(d3drm1, d3d1, d3ddevice1, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %x.\n", hr);

    hr = IDirect3DRM_CreateDeviceFromD3D(d3drm1, d3d1, d3ddevice1, &device1);
    ok(hr == DD_OK, "Failed to create IDirect3DRMDevice interface (hr = %x)\n", hr);
    ref2 = get_refcount((IUnknown *)d3drm1);
    ok(ref2 > ref1, "expected ref2 > ref1, got ref1 = %u , ref2 = %u.\n", ref1, ref2);
    device_ref2 = get_refcount((IUnknown *)d3ddevice1);
    ok(device_ref2 > device_ref1, "Expected device_ref2 > device_ref1, got device_ref1 = %u, device_ref2 = %u.\n", device_ref1, device_ref2);
    d3d_ref2 = get_refcount((IUnknown *)d3d1);
    ok(d3d_ref2 > d3d_ref1, "Expected d3d_ref2 > d3d_ref1, got d3d_ref1 = %u, d3d_ref2 = %u.\n", d3d_ref1, d3d_ref2);
    ret_val = IDirect3DRMDevice_GetWidth(device1);
    ok(ret_val == rc.right, "Expected device width = 300, got %u.\n", ret_val);
    ret_val = IDirect3DRMDevice_GetHeight(device1);
    ok(ret_val == rc.bottom, "Expected device height == 200, got %u.\n", ret_val);

    hr = IDirect3DRMDevice_QueryInterface(device1, &IID_IDirect3DRMDevice2, (void **)&device2);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMDevice2 Interface (hr = %x).\n", hr);
    hr = IDirect3DRMDevice2_GetDirect3DDevice2(device2, &d3ddevice2);
    ok(SUCCEEDED(hr), "Expected hr == D3DRM_OK, got %#x.\n", hr);
    ok(d3ddevice2 == NULL, "Expected d3ddevice2 == NULL, got %p.\n", d3ddevice2);
    IDirect3DRMDevice2_Release(device2);

    d3ddevice2 = (IDirect3DDevice2 *)0xdeadbeef;
    hr = IDirect3DRMDevice_QueryInterface(device1, &IID_IDirect3DRMDevice3, (void **)&device3);
    ok(hr == DD_OK, "Cannot get IDirect3DRMDevice3 Interface (hr = %x).\n", hr);
    hr = IDirect3DRMDevice3_GetDirect3DDevice2(device3, &d3ddevice2);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    ok(d3ddevice2 == NULL, "Expected d3ddevice2 == NULL, got %p.\n", d3ddevice2);
    IDirect3DRMDevice3_Release(device3);

    hr = IDirectDraw_EnumSurfaces(ddraw1, DDENUMSURFACES_ALL | DDENUMSURFACES_DOESEXIST,
            NULL, &surface, surface_callback);
    ok(hr == DD_OK, "Failed to enumerate surfaces (hr = %x).\n", hr);
    ok(surface == NULL, "No primary surface should have enumerated (%p).\n", surface);

    hr = IDirect3DRMDevice_GetDirect3DDevice(device1, &d3drm_d3ddevice1);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DDevice interface (hr = %x).\n", hr);
    ok(d3ddevice1 == d3drm_d3ddevice1, "Expected Immediate Mode device created == %p, got %p.\n", d3ddevice1, d3drm_d3ddevice1);

    /* Check properties of render target and depth surfaces */
    hr = IDirect3DDevice_QueryInterface(d3drm_d3ddevice1, &IID_IDirectDrawSurface, (void **)&surface);
    ok(hr == DD_OK, "Cannot get surface to the render target (hr = %x).\n", hr);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &desc);
    ok(hr == DD_OK, "Cannot get surface desc structure (hr = %x).\n", hr);

    ok((desc.dwWidth == rc.right) && (desc.dwHeight == rc.bottom), "Expected surface dimensions = %u, %u, got %u, %u.\n",
            rc.right, rc.bottom, desc.dwWidth, desc.dwHeight);
    ok((desc.ddsCaps.dwCaps & (DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE)) == (DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE),
            "Expected caps containing %x, got %x.\n", DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE, desc.ddsCaps.dwCaps);
    expected_flags = DDSD_PIXELFORMAT | DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH;
    ok(desc.dwFlags == expected_flags, "Expected %x for flags, got %x.\n", expected_flags, desc.dwFlags);

    hr = IDirectDrawSurface_GetAttachedSurface(surface, &caps, &d3drm_ds);
    ok(hr == DD_OK, "Cannot get attached depth surface (hr = %x).\n", hr);
    ok(ds == d3drm_ds, "Expected depth surface (%p) == surface created internally (%p).\n", ds, d3drm_ds);

    desc.dwSize = sizeof(desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(ds, &desc);
    ok(hr == DD_OK, "Cannot get z surface desc structure (hr = %x).\n", hr);

    ok((desc.dwWidth == rc.right) && (desc.dwHeight == rc.bottom), "Expected surface dimensions = %u, %u, got %u, %u.\n",
            rc.right, rc.bottom, desc.dwWidth, desc.dwHeight);
    ok((desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER) == DDSCAPS_ZBUFFER, "Expected caps containing %x, got %x.\n", DDSCAPS_ZBUFFER, desc.ddsCaps.dwCaps);
    expected_flags = DDSD_ZBUFFERBITDEPTH | DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH;
    ok(desc.dwFlags == expected_flags, "Expected %x for flags, got %x.\n", expected_flags, desc.dwFlags);

    IDirectDrawSurface_Release(d3drm_ds);
    IDirectDrawSurface_Release(ds);
    IDirectDrawSurface_Release(surface);
    IDirect3DDevice_Release(d3drm_d3ddevice1);
    IDirect3DRMDevice_Release(device1);
    ref2 = get_refcount((IUnknown *)d3drm1);
    ok(ref1 == ref2, "expected ref1 == ref2, got ref1 = %u, ref2 = %u.\n", ref1, ref2);
    device_ref2 = get_refcount((IUnknown *)d3ddevice1);
    ok(device_ref2 == device_ref1, "Expected device_ref2 == device_ref1, got device_ref1 = %u, device_ref2 = %u.\n", device_ref1, device_ref2);

    /* InitFromD3D tests */
    hr = IDirect3DRM_CreateObject(d3drm1, &CLSID_CDirect3DRMDevice, NULL, &IID_IDirect3DRMDevice, (void **)&device1);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMDevice interface (hr = %#x).\n", hr);

    hr = IDirect3DRMDevice_InitFromD3D(device1, NULL, d3ddevice1);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    hr = IDirect3DRMDevice_InitFromD3D(device1, d3d1, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);

    hr = IDirect3DRMDevice_InitFromD3D(device1, d3d1, d3ddevice1);
    ok(SUCCEEDED(hr), "Failed to initialise IDirect3DRMDevice interface (hr = %#x)\n", hr);
    ref2 = get_refcount((IUnknown *)d3drm1);
    ok(ref2 > ref1, "expected ref2 > ref1, got ref1 = %u , ref2 = %u.\n", ref1, ref2);
    device_ref2 = get_refcount((IUnknown *)d3ddevice1);
    ok(device_ref2 > device_ref1, "Expected device_ref2 > device_ref1, got device_ref1 = %u, device_ref2 = %u.\n",
            device_ref1, device_ref2);
    d3d_ref2 = get_refcount((IUnknown *)d3d1);
    ok(d3d_ref2 > d3d_ref1, "Expected d3d_ref2 > d3d_ref1, got d3d_ref1 = %u, d3d_ref2 = %u.\n", d3d_ref1, d3d_ref2);
    ret_val = IDirect3DRMDevice_GetWidth(device1);
    ok(ret_val == rc.right, "Expected device width = 300, got %u.\n", ret_val);
    ret_val = IDirect3DRMDevice_GetHeight(device1);
    ok(ret_val == rc.bottom, "Expected device height == 200, got %u.\n", ret_val);

    hr = IDirect3DRMDevice_InitFromD3D(device1, d3d1, d3ddevice1);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    ref3 = get_refcount((IUnknown *)d3drm1);
    ok(ref3 > ref1, "expected ref3 > ref1, got ref1 = %u , ref3 = %u.\n", ref1, ref3);
    ref3 = get_refcount((IUnknown *)d3ddevice1);
    ok(ref3 > device_ref2, "Expected ref3 > device_ref2, got ref3 = %u, device_ref2 = %u.\n", ref3, device_ref2);
    ref3 = get_refcount((IUnknown *)d3d1);
    ok(ref3 > d3d_ref2, "Expected ref3 > d3d_ref2, got ref3 = %u, d3d_ref2 = %u.\n", ref3, d3d_ref2);
    /* Release leaked references */
    while (IDirect3DRM_Release(d3drm1) > ref2);
    while (IDirect3DDevice_Release(d3ddevice1) > device_ref2);
    while (IDirect3D_Release(d3d1) > d3d_ref2);

    hr = DirectDrawCreate(NULL, &temp_ddraw1, NULL);
    ok(SUCCEEDED(hr), "Cannot get IDirectDraw interface (hr = %#x).\n", hr);
    ref4 = get_refcount((IUnknown *)temp_ddraw1);

    hr = IDirectDraw_QueryInterface(temp_ddraw1, &IID_IDirect3D, (void **)&temp_d3d1);
    ok(SUCCEEDED(hr), "Cannot get IDirect3D2 interface (hr = %#x).\n", hr);
    temp_d3ddevice1 = create_device1(temp_ddraw1, window, &surface);
    hr = IDirect3DRMDevice_InitFromD3D(device1, temp_d3d1, temp_d3ddevice1);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    ref3 = get_refcount((IUnknown *)d3drm1);
    ok(ref3 > ref2, "expected ref3 > ref1, got ref1 = %u , ref3 = %u.\n", ref1, ref3);
    ref3 = get_refcount((IUnknown *)temp_d3ddevice1);
    ok(ref3 == device_ref2, "Expected ref3 == device_ref2, got ref3 = %u, device_ref2 = %u.\n", ref3, device_ref2);
    ref3 = get_refcount((IUnknown *)temp_d3d1);
    todo_wine ok(ref3 < d3d_ref2, "Expected ref3 < d3d_ref2, got ref3 = %u, d3d_ref2 = %u.\n", ref3, d3d_ref2);
    /* Release leaked references */
    while (IDirect3DRM_Release(d3drm1) > ref2);
    while (IDirect3DDevice_Release(temp_d3ddevice1) > 0);
    while (IDirect3D_Release(temp_d3d1) > ref4);
    IDirectDrawSurface_Release(surface);
    IDirectDraw_Release(temp_ddraw1);

    d3ddevice2 = (IDirect3DDevice2 *)0xdeadbeef;
    hr = IDirect3DRMDevice_QueryInterface(device1, &IID_IDirect3DRMDevice2, (void **)&device2);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMDevice2 Interface (hr = %x).\n", hr);
    hr = IDirect3DRMDevice2_GetDirect3DDevice2(device2, &d3ddevice2);
    ok(SUCCEEDED(hr), "Expected hr == D3DRM_OK, got %#x.\n", hr);
    ok(d3ddevice2 == NULL, "Expected d3ddevice2 == NULL, got %p.\n", d3ddevice2);
    IDirect3DRMDevice2_Release(device2);

    d3ddevice2 = (IDirect3DDevice2 *)0xdeadbeef;
    hr = IDirect3DRMDevice_QueryInterface(device1, &IID_IDirect3DRMDevice3, (void **)&device3);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMDevice3 Interface (hr = %#x).\n", hr);
    hr = IDirect3DRMDevice3_GetDirect3DDevice2(device3, &d3ddevice2);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    ok(d3ddevice2 == NULL, "Expected d3ddevice2 == NULL, got %p.\n", d3ddevice2);
    IDirect3DRMDevice3_Release(device3);

    surface = NULL;
    hr = IDirectDraw_EnumSurfaces(ddraw1, DDENUMSURFACES_ALL | DDENUMSURFACES_DOESEXIST,
            NULL, &surface, surface_callback);
    ok(SUCCEEDED(hr), "Failed to enumerate surfaces (hr = %#x).\n", hr);
    ok(surface == NULL, "No primary surface should have enumerated (%p).\n", surface);

    hr = IDirect3DRMDevice_GetDirect3DDevice(device1, &d3drm_d3ddevice1);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DDevice interface (hr = %#x).\n", hr);
    ok(d3ddevice1 == d3drm_d3ddevice1, "Expected Immediate Mode device created == %p, got %p.\n",
            d3ddevice1, d3drm_d3ddevice1);

    /* Check properties of render target and depth surfaces */
    hr = IDirect3DDevice_QueryInterface(d3drm_d3ddevice1, &IID_IDirectDrawSurface, (void **)&surface);
    ok(SUCCEEDED(hr), "Cannot get surface to the render target (hr = %#x).\n", hr);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &desc);
    ok(SUCCEEDED(hr), "Cannot get surface desc structure (hr = %#x).\n", hr);

    ok((desc.dwWidth == rc.right) && (desc.dwHeight == rc.bottom), "Expected surface dimensions = %u, %u, got %u, %u.\n",
            rc.right, rc.bottom, desc.dwWidth, desc.dwHeight);
    ok((desc.ddsCaps.dwCaps & (DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE)) == (DDSCAPS_OFFSCREENPLAIN|DDSCAPS_3DDEVICE),
            "Expected caps containing %x, got %x.\n", DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE, desc.ddsCaps.dwCaps);
    expected_flags = DDSD_PIXELFORMAT | DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH;
    ok(desc.dwFlags == expected_flags, "Expected %x for flags, got %x.\n", expected_flags, desc.dwFlags);

    hr = IDirectDrawSurface_GetAttachedSurface(surface, &caps, &d3drm_ds);
    ok(SUCCEEDED(hr), "Cannot get attached depth surface (hr = %x).\n", hr);
    ok(ds == d3drm_ds, "Expected depth surface (%p) == surface created internally (%p).\n", ds, d3drm_ds);

    desc.dwSize = sizeof(desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(ds, &desc);
    ok(SUCCEEDED(hr), "Cannot get z surface desc structure (hr = %#x).\n", hr);

    ok((desc.dwWidth == rc.right) && (desc.dwHeight == rc.bottom), "Expected surface dimensions = %u, %u, got %u, %u.\n",
            rc.right, rc.bottom, desc.dwWidth, desc.dwHeight);
    ok((desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER) == DDSCAPS_ZBUFFER, "Expected caps containing %#x, got %#x.\n",
            DDSCAPS_ZBUFFER, desc.ddsCaps.dwCaps);
    expected_flags = DDSD_ZBUFFERBITDEPTH | DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH;
    ok(desc.dwFlags == expected_flags, "Expected %#x for flags, got %#x.\n", expected_flags, desc.dwFlags);

    IDirectDrawSurface_Release(d3drm_ds);
    IDirectDrawSurface_Release(ds);
    IDirectDrawSurface_Release(surface);
    IDirect3DDevice_Release(d3drm_d3ddevice1);
    IDirect3DRMDevice_Release(device1);
    ref2 = get_refcount((IUnknown *)d3drm1);
    ok(ref1 == ref2, "expected ref1 == ref2, got ref1 = %u, ref2 = %u.\n", ref1, ref2);
    device_ref2 = get_refcount((IUnknown *)d3ddevice1);
    ok(device_ref2 == device_ref1, "Expected device_ref2 == device_ref1, got device_ref1 = %u, device_ref2 = %u.\n",
            device_ref1, device_ref2);
    d3d_ref2 = get_refcount((IUnknown *)d3d1);
    todo_wine ok(d3d_ref2 > d3d_ref1, "Expected d3d_ref2 > d3d_ref1, got d3d_ref1 = %u, d3d_ref2 = %u.\n", d3d_ref1,
            d3d_ref2);

    IDirect3DRM_Release(d3drm1);
    IDirect3DDevice_Release(d3ddevice1);
    IDirect3D_Release(d3d1);
    IDirectDraw_Release(ddraw1);
    DestroyWindow(window);
}

static IDirect3DDevice2 *create_device2(IDirectDraw2 *ddraw, HWND window, IDirectDrawSurface **ds)
{
    static const DWORD z_depths[] = { 32, 24, 16 };
    IDirectDrawSurface *surface;
    IDirect3DDevice2 *device = NULL;
    DDSURFACEDESC surface_desc;
    IDirect3D2 *d3d;
    unsigned int i;
    HRESULT hr;
    RECT rc;

    GetClientRect(window, &rc);
    hr = IDirectDraw2_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    surface_desc.dwWidth = rc.right;
    surface_desc.dwHeight = rc.bottom;

    hr = IDirectDraw2_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDraw2_QueryInterface(ddraw, &IID_IDirect3D2, (void **)&d3d);
    if (FAILED(hr))
    {
        IDirectDrawSurface_Release(surface);
        *ds = NULL;
        return NULL;
    }

    /* We used to use EnumDevices() for this, but it seems
    * D3DDEVICEDESC.dwDeviceZBufferBitDepth only has a very casual
    * relationship with reality. */
    for (i = 0; i < ARRAY_SIZE(z_depths); ++i)
    {
        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags = DDSD_CAPS | DDSD_ZBUFFERBITDEPTH | DDSD_WIDTH | DDSD_HEIGHT;
        surface_desc.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
        U2(surface_desc).dwZBufferBitDepth = z_depths[i];
        surface_desc.dwWidth = rc.right;
        surface_desc.dwHeight = rc.bottom;
        if (FAILED(IDirectDraw2_CreateSurface(ddraw, &surface_desc, ds, NULL)))
            continue;

        hr = IDirectDrawSurface_AddAttachedSurface(surface, *ds);
        ok(SUCCEEDED(hr), "Failed to attach depth buffer, hr %#x.\n", hr);
        if (FAILED(hr))
        {
            IDirectDrawSurface_Release(*ds);
            continue;
        }

        if (SUCCEEDED(IDirect3D2_CreateDevice(d3d, &IID_IDirect3DHALDevice, surface, &device)))
            break;

        IDirectDrawSurface_DeleteAttachedSurface(surface, 0, *ds);
        IDirectDrawSurface_Release(*ds);
        *ds = NULL;
    }

    IDirect3D2_Release(d3d);
    IDirectDrawSurface_Release(surface);
    return device;
}

static void test_create_device_from_d3d2(void)
{
    IDirectDraw *ddraw1 = NULL, *temp_ddraw1;
    IDirectDraw2 *ddraw2 = NULL, *temp_ddraw2;
    IDirect3D* d3d1;
    IDirect3D2 *d3d2 = NULL, *temp_d3d2;
    IDirect3DRM *d3drm1 = NULL;
    IDirect3DRM2 *d3drm2 = NULL;
    IDirect3DRMDevice *device1;
    IDirect3DRMDevice2 *device2 = (IDirect3DRMDevice2 *)0xdeadbeef;
    IDirect3DDevice *d3ddevice1;
    IDirect3DDevice2 *d3ddevice2 = NULL, *d3drm_d3ddevice2 = NULL, *temp_d3ddevice2;
    IDirectDrawSurface *surface = NULL, *ds = NULL, *d3drm_ds = NULL;
    DWORD expected_flags, ret_val;
    DDSCAPS caps = { DDSCAPS_ZBUFFER };
    DDSURFACEDESC desc;
    RECT rc;
    HWND window;
    ULONG ref1, ref2, ref3, ref4, ref5, device_ref1, device_ref2, d3d_ref1, d3d_ref2;
    HRESULT hr;

    hr = DirectDrawCreate(NULL, &ddraw1, NULL);
    ok(hr == DD_OK, "Cannot get IDirectDraw interface (hr = %x).\n", hr);

    window = create_window();
    GetClientRect(window, &rc);

    hr = IDirectDraw_QueryInterface(ddraw1, &IID_IDirect3D2, (void **)&d3d2);
    ok(hr == DD_OK, "Cannot get IDirect3D2 interface (hr = %x).\n", hr);
    hr = IDirectDraw_QueryInterface(ddraw1, &IID_IDirectDraw2, (void **)&ddraw2);
    ok(hr == DD_OK, "Cannot get IDirectDraw2 interface (hr = %x).\n", hr);
    d3d_ref1 = get_refcount((IUnknown *)d3d2);

    /* Create the immediate mode device */
    d3ddevice2 = create_device2(ddraw2, window, &ds);
    if (d3ddevice2 == NULL)
    {
        win_skip("Cannot create IM device, skipping tests.\n");
        IDirect3D2_Release(d3d2);
        IDirectDraw2_Release(ddraw2);
        IDirectDraw_Release(ddraw1);
        return;
    }
    device_ref1 = get_refcount((IUnknown *)d3ddevice2);

    hr = Direct3DRMCreate(&d3drm1);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x).\n", hr);
    ref1 = get_refcount((IUnknown *)d3drm1);

    hr = IDirect3DRM_QueryInterface(d3drm1, &IID_IDirect3DRM2, (void **)&d3drm2);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM2 interface (hr = %x).\n", hr);
    ref2 = get_refcount((IUnknown *)d3drm2);

    hr = IDirect3DRM2_CreateDeviceFromD3D(d3drm2, NULL, d3ddevice2, &device2);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %x.\n", hr);
    ok(device2 == NULL, "Expected device returned == NULL, got %p.\n", device2);
    hr = IDirect3DRM2_CreateDeviceFromD3D(d3drm2, d3d2, NULL, &device2);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %x.\n", hr);
    hr = IDirect3DRM2_CreateDeviceFromD3D(d3drm2, d3d2, d3ddevice2, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %x.\n", hr);

    hr = IDirect3DRM2_CreateDeviceFromD3D(d3drm2, d3d2, d3ddevice2, &device2);
    ok(hr == DD_OK, "Failed to create IDirect3DRMDevice2 interface (hr = %x)\n", hr);
    ref3 = get_refcount((IUnknown *)d3drm1);
    ok(ref3 > ref1, "expected ref3 > ref1, got ref1 = %u , ref3 = %u.\n", ref1, ref3);
    ref3 = get_refcount((IUnknown *)d3drm2);
    ok(ref3 == ref2, "expected ref3 == ref2, got ref2 = %u , ref3 = %u.\n", ref2, ref3);
    device_ref2 = get_refcount((IUnknown *)d3ddevice2);
    ok(device_ref2 > device_ref1, "Expected device_ref2 > device_ref1, got device_ref1 = %u, device_ref2 = %u.\n", device_ref1, device_ref2);
    d3d_ref2 = get_refcount((IUnknown *)d3d2);
    ok(d3d_ref2 > d3d_ref1, "Expected d3d_ref2 > d3d_ref1, got d3d_ref1 = %u, d3d_ref2 = %u.\n", d3d_ref1, d3d_ref2);
    ret_val = IDirect3DRMDevice2_GetWidth(device2);
    ok(ret_val == rc.right, "Expected device width = 300, got %u.\n", ret_val);
    ret_val = IDirect3DRMDevice2_GetHeight(device2);
    ok(ret_val == rc.bottom, "Expected device height == 200, got %u.\n", ret_val);

    hr = IDirectDraw_EnumSurfaces(ddraw1, DDENUMSURFACES_ALL | DDENUMSURFACES_DOESEXIST,
            NULL, &surface, surface_callback);
    ok(hr == DD_OK, "Failed to enumerate surfaces (hr = %x).\n", hr);
    ok(surface == NULL, "No primary surface should have enumerated (%p).\n", surface);

    hr = IDirect3DRMDevice2_GetDirect3DDevice2(device2, &d3drm_d3ddevice2);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DDevice2 interface (hr = %x).\n", hr);
    ok(d3ddevice2 == d3drm_d3ddevice2, "Expected Immediate Mode device created == %p, got %p.\n", d3ddevice2, d3drm_d3ddevice2);

    /* Check properties of render target and depth surfaces */
    hr = IDirect3DDevice2_GetRenderTarget(d3drm_d3ddevice2, &surface);
    ok(hr == DD_OK, "Cannot get surface to the render target (hr = %x).\n", hr);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &desc);
    ok(hr == DD_OK, "Cannot get surface desc structure (hr = %x).\n", hr);

    ok((desc.dwWidth == rc.right) && (desc.dwHeight == rc.bottom), "Expected surface dimensions = %u, %u, got %u, %u.\n",
            rc.right, rc.bottom, desc.dwWidth, desc.dwHeight);
    ok((desc.ddsCaps.dwCaps & (DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE)) == (DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE),
            "Expected caps containing %x, got %x.\n", DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE, desc.ddsCaps.dwCaps);
    expected_flags = DDSD_PIXELFORMAT | DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH;
    ok(desc.dwFlags == expected_flags, "Expected %x for flags, got %x.\n", expected_flags, desc.dwFlags);

    hr = IDirectDrawSurface_GetAttachedSurface(surface, &caps, &d3drm_ds);
    ok(hr == DD_OK, "Cannot get attached depth surface (hr = %x).\n", hr);
    ok(ds == d3drm_ds, "Expected depth surface (%p) == surface created internally (%p).\n", ds, d3drm_ds);

    desc.dwSize = sizeof(desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(ds, &desc);
    ok(hr == DD_OK, "Cannot get z surface desc structure (hr = %x).\n", hr);

    ok((desc.dwWidth == rc.right) && (desc.dwHeight == rc.bottom), "Expected surface dimensions = %u, %u, got %u, %u.\n",
            rc.right, rc.bottom, desc.dwWidth, desc.dwHeight);
    ok((desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER) == DDSCAPS_ZBUFFER, "Expected caps containing %x, got %x.\n", DDSCAPS_ZBUFFER, desc.ddsCaps.dwCaps);
    expected_flags = DDSD_ZBUFFERBITDEPTH | DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH;
    ok(desc.dwFlags == expected_flags, "Expected %x for flags, got %x.\n", expected_flags, desc.dwFlags);

    IDirectDrawSurface_Release(d3drm_ds);
    IDirectDrawSurface_Release(ds);
    IDirectDrawSurface_Release(surface);
    IDirect3DDevice2_Release(d3drm_d3ddevice2);
    IDirect3DRMDevice2_Release(device2);
    ref3 = get_refcount((IUnknown *)d3drm1);
    ok(ref1 == ref3, "expected ref1 == ref3, got ref1 = %u, ref3 = %u.\n", ref1, ref3);
    ref3 = get_refcount((IUnknown *)d3drm2);
    ok(ref3 == ref2, "expected ref3 == ref2, got ref2 = %u , ref3 = %u.\n", ref2, ref3);
    device_ref2 = get_refcount((IUnknown *)d3ddevice2);
    ok(device_ref2 == device_ref1, "Expected device_ref2 == device_ref1, got device_ref1 = %u, device_ref2 = %u.\n", device_ref1, device_ref2);
    d3d_ref2 = get_refcount((IUnknown *)d3d2);
    ok(d3d_ref2 == d3d_ref1, "Expected d3d_ref2 == d3d_ref1, got d3d_ref1 = %u, d3d_ref2 = %u.\n", d3d_ref1, d3d_ref2);

    /* InitFromD3D tests */
    hr = IDirect3DRM2_CreateObject(d3drm2, &CLSID_CDirect3DRMDevice, NULL, &IID_IDirect3DRMDevice2, (void **)&device2);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMDevice2 interface (hr = %#x).\n", hr);

    hr = IDirectDraw_QueryInterface(ddraw1, &IID_IDirect3D, (void **)&d3d1);
    ok(SUCCEEDED(hr), "Cannot get IDirect3D interface (hr = %x).\n", hr);
    if (SUCCEEDED(hr = IDirect3DDevice2_QueryInterface(d3ddevice2, &IID_IDirect3DDevice, (void **)&d3ddevice1)))
    {
        hr = IDirect3DRMDevice2_InitFromD3D(device2, d3d1, d3ddevice1);
        ok(hr == E_NOINTERFACE, "Expected hr == E_NOINTERFACE, got %#x.\n", hr);
        hr = IDirect3DRMDevice2_InitFromD3D(device2, NULL, d3ddevice1);
        ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
        hr = IDirect3DRMDevice2_InitFromD3D(device2, d3d1, NULL);
        ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
        hr = IDirect3DRMDevice2_QueryInterface(device2, &IID_IDirect3DRMDevice, (void **)&device1);
        ok(SUCCEEDED(hr), "Cannot obtain IDirect3DRMDevice interface (hr = %#x).\n", hr);
        hr = IDirect3DRMDevice_InitFromD3D(device1, d3d1, d3ddevice1);
        todo_wine ok(hr == E_NOINTERFACE, "Expected hr == E_NOINTERFACE, got %#x.\n", hr);
        IDirect3DRMDevice_Release(device1);
        if (SUCCEEDED(hr))
        {
            IDirect3DRMDevice_Release(device1);
            hr = IDirect3DRM2_CreateObject(d3drm2, &CLSID_CDirect3DRMDevice, NULL, &IID_IDirect3DRMDevice2,
                    (void **)&device2);
            ok(SUCCEEDED(hr), "Cannot get IDirect3DRMDevice2 interface (hr = %#x).\n", hr);
        }
    }
    IDirect3D_Release(d3d1);
    IDirect3DDevice_Release(d3ddevice1);

    hr = IDirect3DRMDevice2_InitFromD3D2(device2, NULL, d3ddevice2);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    hr = IDirect3DRMDevice2_InitFromD3D2(device2, d3d2, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);

    hr = IDirect3DRMDevice2_InitFromD3D2(device2, d3d2, d3ddevice2);
    ok(SUCCEEDED(hr), "Failed to initialise IDirect3DRMDevice2 interface (hr = %#x)\n", hr);
    ref4 = get_refcount((IUnknown *)d3drm1);
    ok(ref4 > ref1, "Expected ref4 > ref1, got ref1 = %u , ref4 = %u.\n", ref1, ref4);
    device_ref2 = get_refcount((IUnknown *)d3ddevice2);
    ok(device_ref2 > device_ref1, "Expected device_ref2 > device_ref1, got device_ref1 = %u, device_ref2 = %u.\n",
            device_ref1, device_ref2);
    d3d_ref2 = get_refcount((IUnknown *)d3d2);
    ok(d3d_ref2 > d3d_ref1, "Expected d3d_ref2 > d3d_ref1, got d3d_ref1 = %u, d3d_ref2 = %u.\n", d3d_ref1, d3d_ref2);
    ret_val = IDirect3DRMDevice2_GetWidth(device2);
    ok(ret_val == rc.right, "Expected device width = 300, got %u.\n", ret_val);
    ret_val = IDirect3DRMDevice2_GetHeight(device2);
    ok(ret_val == rc.bottom, "Expected device height == 200, got %u.\n", ret_val);

    hr = IDirect3DRMDevice2_InitFromD3D2(device2, d3d2, d3ddevice2);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    ref3 = get_refcount((IUnknown *)d3drm1);
    ok(ref3 > ref1, "expected ref3 > ref1, got ref1 = %u , ref3 = %u.\n", ref1, ref3);
    ref3 = get_refcount((IUnknown *)d3ddevice2);
    ok(ref3 > device_ref2, "Expected ref3 > device_ref2, got ref3 = %u, device_ref2 = %u.\n", ref3, device_ref2);
    ref3 = get_refcount((IUnknown *)d3d2);
    ok(ref3 > d3d_ref2, "Expected ref3 > d3d_ref2, got ref3 = %u, d3d_ref2 = %u.\n", ref3, d3d_ref2);
    /* Release leaked references */
    while (IDirect3DRM_Release(d3drm1) > ref4);
    while (IDirect3DDevice2_Release(d3ddevice2) > device_ref2);
    while (IDirect3D2_Release(d3d2) > d3d_ref2);

    hr = DirectDrawCreate(NULL, &temp_ddraw1, NULL);
    ok(SUCCEEDED(hr), "Cannot get IDirectDraw interface (hr = %#x).\n", hr);
    hr = IDirectDraw_QueryInterface(temp_ddraw1, &IID_IDirect3D2, (void **)&temp_d3d2);
    ok(SUCCEEDED(hr), "Cannot get IDirect3D2 interface (hr = %#x).\n", hr);
    ref5 = get_refcount((IUnknown *)temp_d3d2);

    hr = IDirectDraw_QueryInterface(temp_ddraw1, &IID_IDirectDraw2, (void **)&temp_ddraw2);
    ok(SUCCEEDED(hr), "Cannot get IDirectDraw2 interface (hr = %#x).\n", hr);

    temp_d3ddevice2 = create_device2(temp_ddraw2, window, &surface);
    hr = IDirect3DRMDevice2_InitFromD3D2(device2, temp_d3d2, temp_d3ddevice2);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    ref3 = get_refcount((IUnknown *)d3drm1);
    ok(ref3 > ref4, "expected ref3 > ref4, got ref3 = %u , ref4 = %u.\n", ref3, ref4);
    ref3 = get_refcount((IUnknown *)temp_d3ddevice2);
    ok(ref3 == device_ref2, "Expected ref3 == device_ref2, got ref3 = %u, device_ref2 = %u.\n", ref3, device_ref2);
    ref3 = get_refcount((IUnknown *)temp_d3d2);
    ok(ref3 == d3d_ref2, "Expected ref3 == d3d_ref2, got ref3 = %u, d3d_ref2 = %u.\n", ref3, d3d_ref2);
    /* Release leaked references */
    while (IDirect3DRM_Release(d3drm1) > ref4);
    while (IDirect3DDevice2_Release(temp_d3ddevice2) > 0);
    while (IDirect3D2_Release(temp_d3d2) >= ref5);
    IDirectDrawSurface_Release(surface);
    IDirectDraw2_Release(temp_ddraw2);
    IDirectDraw_Release(temp_ddraw1);

    surface = NULL;
    hr = IDirectDraw_EnumSurfaces(ddraw1, DDENUMSURFACES_ALL | DDENUMSURFACES_DOESEXIST,
            NULL, &surface, surface_callback);
    ok(SUCCEEDED(hr), "Failed to enumerate surfaces (hr = %#x).\n", hr);
    ok(surface == NULL, "No primary surface should have enumerated (%p).\n", surface);

    hr = IDirect3DRMDevice2_GetDirect3DDevice2(device2, &d3drm_d3ddevice2);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DDevice2 interface (hr = %#x).\n", hr);
    ok(d3ddevice2 == d3drm_d3ddevice2, "Expected Immediate Mode device created == %p, got %p.\n", d3ddevice2,
            d3drm_d3ddevice2);

    /* Check properties of render target and depth surfaces */
    hr = IDirect3DDevice2_GetRenderTarget(d3drm_d3ddevice2, &surface);
    ok(SUCCEEDED(hr), "Cannot get surface to the render target (hr = %#x).\n", hr);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &desc);
    ok(SUCCEEDED(hr), "Cannot get surface desc structure (hr = %#x).\n", hr);

    ok((desc.dwWidth == rc.right) && (desc.dwHeight == rc.bottom), "Expected surface dimensions = %u, %u, got %u, %u.\n",
            rc.right, rc.bottom, desc.dwWidth, desc.dwHeight);
    ok((desc.ddsCaps.dwCaps & (DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE)) == (DDSCAPS_OFFSCREENPLAIN|DDSCAPS_3DDEVICE),
            "Expected caps containing %#x, got %#x.\n", DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE, desc.ddsCaps.dwCaps);
    expected_flags = DDSD_PIXELFORMAT | DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH;
    ok(desc.dwFlags == expected_flags, "Expected %#x for flags, got %#x.\n", expected_flags, desc.dwFlags);

    hr = IDirectDrawSurface_GetAttachedSurface(surface, &caps, &d3drm_ds);
    ok(SUCCEEDED(hr), "Cannot get attached depth surface (hr = %x).\n", hr);
    ok(ds == d3drm_ds, "Expected depth surface (%p) == surface created internally (%p).\n", ds, d3drm_ds);

    desc.dwSize = sizeof(desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(ds, &desc);
    ok(SUCCEEDED(hr), "Cannot get z surface desc structure (hr = %x).\n", hr);

    ok((desc.dwWidth == rc.right) && (desc.dwHeight == rc.bottom), "Expected surface dimensions = %u, %u, got %u, %u.\n",
            rc.right, rc.bottom, desc.dwWidth, desc.dwHeight);
    ok((desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER) == DDSCAPS_ZBUFFER, "Expected caps containing %#x, got %#x.\n",
            DDSCAPS_ZBUFFER, desc.ddsCaps.dwCaps);
    expected_flags = DDSD_ZBUFFERBITDEPTH | DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH;
    ok(desc.dwFlags == expected_flags, "Expected %#x for flags, got %#x.\n", expected_flags, desc.dwFlags);

    IDirectDrawSurface_Release(d3drm_ds);
    IDirectDrawSurface_Release(ds);
    IDirectDrawSurface_Release(surface);
    IDirect3DDevice2_Release(d3drm_d3ddevice2);
    IDirect3DRMDevice2_Release(device2);
    ref3 = get_refcount((IUnknown *)d3drm1);
    ok(ref1 == ref3, "Expected ref1 == ref3, got ref1 = %u, ref3 = %u.\n", ref1, ref3);
    ref3 = get_refcount((IUnknown *)d3drm2);
    ok(ref3 == ref2, "Expected ref3 == ref2, got ref2 = %u , ref3 = %u.\n", ref2, ref3);
    device_ref2 = get_refcount((IUnknown *)d3ddevice2);
    ok(device_ref2 == device_ref1, "Expected device_ref2 == device_ref1, got device_ref1 = %u, device_ref2 = %u.\n",
            device_ref1, device_ref2);
    d3d_ref2 = get_refcount((IUnknown *)d3d2);
    ok(d3d_ref2 == d3d_ref1, "Expected d3d_ref2 == d3d_ref1, got d3d_ref1 = %u, d3d_ref2 = %u.\n", d3d_ref1, d3d_ref2);

    IDirect3DRM2_Release(d3drm2);
    IDirect3DRM_Release(d3drm1);
    IDirect3DDevice2_Release(d3ddevice2);
    IDirect3D2_Release(d3d2);
    IDirectDraw2_Release(ddraw2);
    IDirectDraw_Release(ddraw1);
    DestroyWindow(window);
}

static void test_create_device_from_d3d3(void)
{
    IDirectDraw *ddraw1 = NULL, *temp_ddraw1;
    IDirectDraw2 *ddraw2 = NULL, *temp_ddraw2;
    IDirect3D *d3d1;
    IDirect3D2 *d3d2 = NULL, *temp_d3d2;
    IDirect3DRM *d3drm1 = NULL;
    IDirect3DRM3 *d3drm3 = NULL;
    IDirect3DRMDevice *device1;
    IDirect3DRMDevice3 *device3 = (IDirect3DRMDevice3 *)0xdeadbeef;
    IDirect3DDevice *d3ddevice1;
    IDirect3DDevice2 *d3ddevice2 = NULL, *d3drm_d3ddevice2 = NULL, *temp_d3ddevice2;
    IDirectDrawSurface *surface = NULL, *ds = NULL, *d3drm_ds = NULL;
    DWORD expected_flags, ret_val;
    DDSCAPS caps = { DDSCAPS_ZBUFFER };
    DDSURFACEDESC desc;
    RECT rc;
    HWND window;
    ULONG ref1, ref2, ref3, ref4, ref5, device_ref1, device_ref2, d3d_ref1, d3d_ref2;
    HRESULT hr;

    hr = DirectDrawCreate(NULL, &ddraw1, NULL);
    ok(hr == DD_OK, "Cannot get IDirectDraw interface (hr = %x).\n", hr);

    window = create_window();
    GetClientRect(window, &rc);

    hr = IDirectDraw_QueryInterface(ddraw1, &IID_IDirect3D2, (void **)&d3d2);
    ok(hr == DD_OK, "Cannot get IDirect3D2 interface (hr = %x).\n", hr);
    hr = IDirectDraw_QueryInterface(ddraw1, &IID_IDirectDraw2, (void **)&ddraw2);
    ok(hr == DD_OK, "Cannot get IDirectDraw2 interface (hr = %x).\n", hr);
    d3d_ref1 = get_refcount((IUnknown *)d3d2);

    /* Create the immediate mode device */
    d3ddevice2 = create_device2(ddraw2, window, &ds);
    if (d3ddevice2 == NULL)
    {
        win_skip("Cannot create IM device, skipping tests.\n");
        IDirect3D2_Release(d3d2);
        IDirectDraw2_Release(ddraw2);
        IDirectDraw_Release(ddraw1);
        return;
    }
    device_ref1 = get_refcount((IUnknown *)d3ddevice2);

    hr = Direct3DRMCreate(&d3drm1);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x).\n", hr);
    ref1 = get_refcount((IUnknown *)d3drm1);

    hr = IDirect3DRM_QueryInterface(d3drm1, &IID_IDirect3DRM3, (void **)&d3drm3);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM3 interface (hr = %x).\n", hr);
    ref2 = get_refcount((IUnknown *)d3drm3);

    hr = IDirect3DRM3_CreateDeviceFromD3D(d3drm3, NULL, d3ddevice2, &device3);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %x.\n", hr);
    ok(device3 == NULL, "Expected device returned == NULL, got %p.\n", device3);
    hr = IDirect3DRM3_CreateDeviceFromD3D(d3drm3, d3d2, NULL, &device3);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %x.\n", hr);
    hr = IDirect3DRM3_CreateDeviceFromD3D(d3drm3, d3d2, d3ddevice2, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %x.\n", hr);

    hr = IDirect3DRM3_CreateDeviceFromD3D(d3drm3, d3d2, d3ddevice2, &device3);
    ok(hr == DD_OK, "Failed to create IDirect3DRMDevice3 interface (hr = %x)\n", hr);
    ref3 = get_refcount((IUnknown *)d3drm1);
    ok(ref3 > ref1, "expected ref3 > ref1, got ref1 = %u , ref3 = %u.\n", ref1, ref3);
    ref3 = get_refcount((IUnknown *)d3drm3);
    ok(ref3 == ref2, "expected ref3 == ref2, got ref2 = %u , ref3 = %u.\n", ref2, ref3);
    device_ref2 = get_refcount((IUnknown *)d3ddevice2);
    ok(device_ref2 > device_ref1, "Expected device_ref2 > device_ref1, got device_ref1 = %u, device_ref2 = %u.\n", device_ref1, device_ref2);
    ret_val = IDirect3DRMDevice3_GetWidth(device3);
    ok(ret_val == rc.right, "Expected device width = 300, got %u.\n", ret_val);
    ret_val = IDirect3DRMDevice3_GetHeight(device3);
    ok(ret_val == rc.bottom, "Expected device height == 200, got %u.\n", ret_val);

    hr = IDirectDraw_EnumSurfaces(ddraw1, DDENUMSURFACES_ALL | DDENUMSURFACES_DOESEXIST,
            NULL, &surface, surface_callback);
    ok(hr == DD_OK, "Failed to enumerate surfaces (hr = %x).\n", hr);
    ok(surface == NULL, "No primary surface should have enumerated (%p).\n", surface);

    hr = IDirect3DRMDevice3_GetDirect3DDevice2(device3, &d3drm_d3ddevice2);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DDevice2 interface (hr = %x).\n", hr);
    ok(d3ddevice2 == d3drm_d3ddevice2, "Expected Immediate Mode device created == %p, got %p.\n", d3ddevice2, d3drm_d3ddevice2);

    /* Check properties of render target and depth surfaces */
    hr = IDirect3DDevice2_GetRenderTarget(d3drm_d3ddevice2, &surface);
    ok(hr == DD_OK, "Cannot get surface to the render target (hr = %x).\n", hr);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &desc);
    ok(hr == DD_OK, "Cannot get surface desc structure (hr = %x).\n", hr);

    ok((desc.dwWidth == rc.right) && (desc.dwHeight == rc.bottom), "Expected surface dimensions = %u, %u, got %u, %u.\n",
            rc.right, rc.bottom, desc.dwWidth, desc.dwHeight);
    ok((desc.ddsCaps.dwCaps & (DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE)) == (DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE),
            "Expected caps containing %x, got %x.\n", DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE, desc.ddsCaps.dwCaps);
    expected_flags = DDSD_PIXELFORMAT | DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH;
    ok(desc.dwFlags == expected_flags, "Expected %x for flags, got %x.\n", expected_flags, desc.dwFlags);

    hr = IDirectDrawSurface_GetAttachedSurface(surface, &caps, &d3drm_ds);
    ok(hr == DD_OK, "Cannot get attached depth surface (hr = %x).\n", hr);
    ok(ds == d3drm_ds, "Expected depth surface (%p) == surface created internally (%p).\n", ds, d3drm_ds);

    desc.dwSize = sizeof(desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(ds, &desc);
    ok(hr == DD_OK, "Cannot get z surface desc structure (hr = %x).\n", hr);

    ok((desc.dwWidth == rc.right) && (desc.dwHeight == rc.bottom), "Expected surface dimensions = %u, %u, got %u, %u.\n",
            rc.right, rc.bottom, desc.dwWidth, desc.dwHeight);
    ok((desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER) == DDSCAPS_ZBUFFER, "Expected caps containing %x, got %x.\n", DDSCAPS_ZBUFFER, desc.ddsCaps.dwCaps);
    expected_flags = DDSD_ZBUFFERBITDEPTH | DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH;
    ok(desc.dwFlags == expected_flags, "Expected %x for flags, got %x.\n", expected_flags, desc.dwFlags);

    IDirectDrawSurface_Release(d3drm_ds);
    IDirectDrawSurface_Release(ds);
    IDirectDrawSurface_Release(surface);
    IDirect3DDevice2_Release(d3drm_d3ddevice2);
    IDirect3DRMDevice3_Release(device3);
    ref3 = get_refcount((IUnknown *)d3drm1);
    ok(ref1 == ref3, "expected ref1 == ref3, got ref1 = %u, ref3 = %u.\n", ref1, ref3);
    ref3 = get_refcount((IUnknown *)d3drm3);
    ok(ref3 == ref2, "expected ref3 == ref2, got ref2 = %u , ref3 = %u.\n", ref2, ref3);
    device_ref2 = get_refcount((IUnknown *)d3ddevice2);
    ok(device_ref2 == device_ref1, "Expected device_ref2 == device_ref1, got device_ref1 = %u, device_ref2 = %u.\n", device_ref1, device_ref2);
    d3d_ref2 = get_refcount((IUnknown *)d3d2);
    ok(d3d_ref2 == d3d_ref1, "Expected d3d_ref2 == d3d_ref1, got d3d_ref1 = %u, d3d_ref2 = %u.\n", d3d_ref1, d3d_ref2);

    /* InitFromD3D tests */
    hr = IDirect3DRM3_CreateObject(d3drm3, &CLSID_CDirect3DRMDevice, NULL, &IID_IDirect3DRMDevice3, (void **)&device3);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMDevice3 interface (hr = %#x).\n", hr);

    hr = IDirectDraw_QueryInterface(ddraw1, &IID_IDirect3D, (void **)&d3d1);
    ok(SUCCEEDED(hr), "Cannot get IDirect3D interface (hr = %#x).\n", hr);
    if (SUCCEEDED(hr = IDirect3DDevice2_QueryInterface(d3ddevice2, &IID_IDirect3DDevice, (void **)&d3ddevice1)))
    {
        hr = IDirect3DRMDevice3_InitFromD3D(device3, d3d1, d3ddevice1);
        ok(hr == E_NOINTERFACE, "Expected hr == E_NOINTERFACE, got %#x.\n", hr);
        hr = IDirect3DRMDevice3_InitFromD3D(device3, NULL, d3ddevice1);
        ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
        hr = IDirect3DRMDevice3_InitFromD3D(device3, d3d1, NULL);
        ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
        hr = IDirect3DRMDevice3_QueryInterface(device3, &IID_IDirect3DRMDevice, (void **)&device1);
        ok(SUCCEEDED(hr), "Cannot obtain IDirect3DRMDevice interface (hr = %#x).\n", hr);
        hr = IDirect3DRMDevice_InitFromD3D(device1, d3d1, d3ddevice1);
        todo_wine ok(hr == E_NOINTERFACE, "Expected hr == E_NOINTERFACE, got %#x.\n", hr);
        IDirect3DRMDevice_Release(device1);
        if (SUCCEEDED(hr))
        {
            IDirect3DRMDevice_Release(device1);
            hr = IDirect3DRM3_CreateObject(d3drm3, &CLSID_CDirect3DRMDevice, NULL, &IID_IDirect3DRMDevice3,
                    (void **)&device3);
            ok(SUCCEEDED(hr), "Cannot get IDirect3DRMDevice3 interface (hr = %#x).\n", hr);
        }
    }
    IDirect3D_Release(d3d1);
    IDirect3DDevice_Release(d3ddevice1);

    hr = IDirect3DRMDevice3_InitFromD3D2(device3, NULL, d3ddevice2);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    hr = IDirect3DRMDevice3_InitFromD3D2(device3, d3d2, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);

    hr = IDirect3DRMDevice3_InitFromD3D2(device3, d3d2, d3ddevice2);
    ok(SUCCEEDED(hr), "Failed to initialise IDirect3DRMDevice2 interface (hr = %#x)\n", hr);
    ref4 = get_refcount((IUnknown *)d3drm1);
    ok(ref4 > ref1, "Expected ref4 > ref1, got ref1 = %u , ref4 = %u.\n", ref1, ref4);
    device_ref2 = get_refcount((IUnknown *)d3ddevice2);
    ok(device_ref2 > device_ref1, "Expected device_ref2 > device_ref1, got device_ref1 = %u, device_ref2 = %u.\n",
            device_ref1, device_ref2);
    d3d_ref2 = get_refcount((IUnknown *)d3d2);
    ok(d3d_ref2 > d3d_ref1, "Expected d3d_ref2 > d3d_ref1, got d3d_ref1 = %u, d3d_ref2 = %u.\n", d3d_ref1, d3d_ref2);
    ret_val = IDirect3DRMDevice3_GetWidth(device3);
    ok(ret_val == rc.right, "Expected device width = 300, got %u.\n", ret_val);
    ret_val = IDirect3DRMDevice3_GetHeight(device3);
    ok(ret_val == rc.bottom, "Expected device height == 200, got %u.\n", ret_val);

    hr = IDirect3DRMDevice3_InitFromD3D2(device3, d3d2, d3ddevice2);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    ref3 = get_refcount((IUnknown *)d3drm1);
    ok(ref3 > ref1, "expected ref3 > ref1, got ref1 = %u , ref3 = %u.\n", ref1, ref3);
    ref3 = get_refcount((IUnknown *)d3ddevice2);
    ok(ref3 > device_ref2, "Expected ref3 > device_ref2, got ref3 = %u, device_ref2 = %u.\n", ref3, device_ref2);
    ref3 = get_refcount((IUnknown *)d3d2);
    ok(ref3 > d3d_ref2, "Expected ref3 > d3d_ref2, got ref3 = %u, d3d_ref2 = %u.\n", ref3, d3d_ref2);
    /* Release leaked references */
    while (IDirect3DRM_Release(d3drm1) > ref4);
    while (IDirect3DDevice2_Release(d3ddevice2) > device_ref2);
    while (IDirect3D2_Release(d3d2) > d3d_ref2);

    hr = DirectDrawCreate(NULL, &temp_ddraw1, NULL);
    ok(SUCCEEDED(hr), "Cannot get IDirectDraw interface (hr = %#x).\n", hr);
    hr = IDirectDraw_QueryInterface(temp_ddraw1, &IID_IDirect3D2, (void **)&temp_d3d2);
    ok(SUCCEEDED(hr), "Cannot get IDirect3D2 interface (hr = %#x).\n", hr);
    ref5 = get_refcount((IUnknown *)temp_d3d2);

    hr = IDirectDraw_QueryInterface(temp_ddraw1, &IID_IDirectDraw2, (void **)&temp_ddraw2);
    ok(SUCCEEDED(hr), "Cannot get IDirectDraw2 interface (hr = %#x).\n", hr);

    temp_d3ddevice2 = create_device2(temp_ddraw2, window, &surface);
    hr = IDirect3DRMDevice3_InitFromD3D2(device3, temp_d3d2, temp_d3ddevice2);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    ref3 = get_refcount((IUnknown *)d3drm1);
    ok(ref3 > ref4, "expected ref3 > ref4, got ref3 = %u , ref4 = %u.\n", ref3, ref4);
    ref3 = get_refcount((IUnknown *)temp_d3ddevice2);
    ok(ref3 == device_ref2, "Expected ref3 == device_ref2, got ref3 = %u, device_ref2 = %u.\n", ref3, device_ref2);
    ref3 = get_refcount((IUnknown *)temp_d3d2);
    ok(ref3 == d3d_ref2, "Expected ref3 == d3d_ref2, got ref3 = %u, d3d_ref2 = %u.\n", ref3, d3d_ref2);
    /* Release leaked references */
    while (IDirect3DRM_Release(d3drm1) > ref4);
    while (IDirect3DDevice2_Release(temp_d3ddevice2) > 0);
    while (IDirect3D2_Release(temp_d3d2) >= ref5);
    IDirectDrawSurface_Release(surface);
    IDirectDraw2_Release(temp_ddraw2);
    IDirectDraw_Release(temp_ddraw1);

    surface = NULL;
    hr = IDirectDraw_EnumSurfaces(ddraw1, DDENUMSURFACES_ALL | DDENUMSURFACES_DOESEXIST,
            NULL, &surface, surface_callback);
    ok(SUCCEEDED(hr), "Failed to enumerate surfaces (hr = %#x).\n", hr);
    ok(surface == NULL, "No primary surface should have enumerated (%p).\n", surface);

    hr = IDirect3DRMDevice3_GetDirect3DDevice2(device3, &d3drm_d3ddevice2);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DDevice2 interface (hr = %#x).\n", hr);
    ok(d3ddevice2 == d3drm_d3ddevice2, "Expected Immediate Mode device created == %p, got %p.\n", d3ddevice2,
            d3drm_d3ddevice2);

    /* Check properties of render target and depth surfaces */
    hr = IDirect3DDevice2_GetRenderTarget(d3drm_d3ddevice2, &surface);
    ok(SUCCEEDED(hr), "Cannot get surface to the render target (hr = %#x).\n", hr);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &desc);
    ok(SUCCEEDED(hr), "Cannot get surface desc structure (hr = %x).\n", hr);

    ok((desc.dwWidth == rc.right) && (desc.dwHeight == rc.bottom), "Expected surface dimensions = %u, %u, got %u, %u.\n",
            rc.right, rc.bottom, desc.dwWidth, desc.dwHeight);
    ok((desc.ddsCaps.dwCaps & (DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE)) == (DDSCAPS_OFFSCREENPLAIN|DDSCAPS_3DDEVICE),
            "Expected caps containing %#x, got %#x.\n", DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE, desc.ddsCaps.dwCaps);
    expected_flags = DDSD_PIXELFORMAT | DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH;
    ok(desc.dwFlags == expected_flags, "Expected %#x for flags, got %#x.\n", expected_flags, desc.dwFlags);

    hr = IDirectDrawSurface_GetAttachedSurface(surface, &caps, &d3drm_ds);
    ok(SUCCEEDED(hr), "Cannot get attached depth surface (hr = %x).\n", hr);
    ok(ds == d3drm_ds, "Expected depth surface (%p) == surface created internally (%p).\n", ds, d3drm_ds);

    desc.dwSize = sizeof(desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(ds, &desc);
    ok(SUCCEEDED(hr), "Cannot get z surface desc structure (hr = %x).\n", hr);

    ok((desc.dwWidth == rc.right) && (desc.dwHeight == rc.bottom), "Expected surface dimensions = %u, %u, got %u, %u.\n",
            rc.right, rc.bottom, desc.dwWidth, desc.dwHeight);
    ok((desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER) == DDSCAPS_ZBUFFER, "Expected caps containing %x, got %#x.\n",
            DDSCAPS_ZBUFFER, desc.ddsCaps.dwCaps);
    expected_flags = DDSD_ZBUFFERBITDEPTH | DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH;
    ok(desc.dwFlags == expected_flags, "Expected %#x for flags, got %#x.\n", expected_flags, desc.dwFlags);

    IDirectDrawSurface_Release(d3drm_ds);
    IDirectDrawSurface_Release(ds);
    IDirectDrawSurface_Release(surface);
    IDirect3DDevice2_Release(d3drm_d3ddevice2);
    IDirect3DRMDevice3_Release(device3);
    ref3 = get_refcount((IUnknown *)d3drm1);
    ok(ref1 == ref3, "expected ref1 == ref3, got ref1 = %u, ref3 = %u.\n", ref1, ref3);
    ref3 = get_refcount((IUnknown *)d3drm3);
    ok(ref3 == ref2, "expected ref3 == ref2, got ref2 = %u , ref3 = %u.\n", ref2, ref3);
    device_ref2 = get_refcount((IUnknown *)d3ddevice2);
    ok(device_ref2 == device_ref1, "Expected device_ref2 == device_ref1, got device_ref1 = %u, device_ref2 = %u.\n",
            device_ref1, device_ref2);
    d3d_ref2 = get_refcount((IUnknown *)d3d2);
    ok(d3d_ref2 == d3d_ref1, "Expected d3d_ref2 == d3d_ref1, got d3d_ref1 = %u, d3d_ref2 = %u.\n", d3d_ref1, d3d_ref2);

    IDirect3DRM3_Release(d3drm3);
    IDirect3DRM_Release(d3drm1);
    IDirect3DDevice2_Release(d3ddevice2);
    IDirect3D2_Release(d3d2);
    IDirectDraw2_Release(ddraw2);
    IDirectDraw_Release(ddraw1);
    DestroyWindow(window);
}

static void test_create_device_1(void)
{
    IDirect3DRM *d3drm = NULL;
    IDirect3DRMDevice *device = (IDirect3DRMDevice *)0xdeadbeef;
    HRESULT hr;

    hr = Direct3DRMCreate(&d3drm);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x).\n", hr);

    hr = IDirect3DRM_CreateDevice(d3drm, 640, 480, &device);
    ok(hr == D3DRMERR_BADDEVICE, "Expected hr == D3DRMERR_BADDEVICE, got %x.\n", hr);
    ok(device == NULL, "Expected device returned == NULL, got %p.\n", device);
    hr = IDirect3DRM_CreateDevice(d3drm, 640, 480, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %x.\n", hr);

    IDirect3DRM_Release(d3drm);
}

static void test_create_device_2(void)
{
    IDirect3DRM *d3drm = NULL;
    IDirect3DRM2 *d3drm2 = NULL;
    IDirect3DRMDevice2 *device2 = (IDirect3DRMDevice2 *)0xdeadbeef;
    HRESULT hr;

    hr = Direct3DRMCreate(&d3drm);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x).\n", hr);
    hr = IDirect3DRM_QueryInterface(d3drm, &IID_IDirect3DRM2, (void **)&d3drm2);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM3 interface (hr = %x).\n", hr);

    hr = IDirect3DRM2_CreateDevice(d3drm2, 640, 480, &device2);
    ok(hr == D3DRMERR_BADDEVICE, "Expected hr == D3DRMERR_BADDEVICE, got %x.\n", hr);
    ok(device2 == NULL, "Expected device returned == NULL, got %p.\n", device2);
    hr = IDirect3DRM2_CreateDevice(d3drm2, 640, 480, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %x.\n", hr);

    IDirect3DRM2_Release(d3drm2);
    IDirect3DRM_Release(d3drm);
}

static void test_create_device_3(void)
{
    IDirect3DRM *d3drm = NULL;
    IDirect3DRM3 *d3drm3 = NULL;
    IDirect3DRMDevice3 *device3 = (IDirect3DRMDevice3 *)0xdeadbeef;
    HRESULT hr;

    hr = Direct3DRMCreate(&d3drm);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x).\n", hr);
    hr = IDirect3DRM_QueryInterface(d3drm, &IID_IDirect3DRM3, (void **)&d3drm3);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM3 interface (hr = %x).\n", hr);

    hr = IDirect3DRM3_CreateDevice(d3drm3, 640, 480, &device3);
    ok(hr == D3DRMERR_BADDEVICE, "Expected hr == D3DRMERR_BADDEVICE, got %x.\n", hr);
    ok(device3 == NULL, "Expected device returned == NULL, got %p.\n", device3);
    hr = IDirect3DRM3_CreateDevice(d3drm3, 640, 480, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %x.\n", hr);

    IDirect3DRM3_Release(d3drm3);
    IDirect3DRM_Release(d3drm);
}

static char *create_bitmap(unsigned int w, unsigned int h, BOOL palettized)
{
    unsigned int bpp = palettized ? 8 : 24;
    BITMAPFILEHEADER file_header;
    DWORD written, size, ret;
    unsigned char *buffer;
    char path[MAX_PATH];
    unsigned int i, j;
    BITMAPINFO *info;
    char *filename;
    HANDLE file;

    ret = GetTempPathA(MAX_PATH, path);
    ok(ret, "Failed to get temporary file path.\n");
    filename = HeapAlloc(GetProcessHeap(), 0, MAX_PATH);
    ret = GetTempFileNameA(path, "d3d", 0, filename);
    ok(ret, "Failed to get filename.\n");
    file = CreateFileA(filename, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "Failed to open temporary file \"%s\".\n", filename);

    size = FIELD_OFFSET(BITMAPINFO, bmiColors[palettized ? 256 : 0]);

    memset(&file_header, 0, sizeof(file_header));
    file_header.bfType = 0x4d42; /* BM */
    file_header.bfOffBits = sizeof(file_header) + size;
    file_header.bfSize = file_header.bfOffBits + w * h * (bpp / 8);
    ret = WriteFile(file, &file_header, sizeof(file_header), &written, NULL);
    ok(ret && written == sizeof(file_header), "Failed to write file header.\n");

    info = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
    info->bmiHeader.biSize = sizeof(info->bmiHeader);
    info->bmiHeader.biBitCount = bpp;
    info->bmiHeader.biPlanes = 1;
    info->bmiHeader.biWidth = w;
    info->bmiHeader.biHeight = h;
    info->bmiHeader.biCompression = BI_RGB;
    if (palettized)
    {
        for (i = 0; i < 256; ++i)
        {
            info->bmiColors[i].rgbBlue = i;
            info->bmiColors[i].rgbGreen = i;
            info->bmiColors[i].rgbRed = i;
        }
    }
    ret = WriteFile(file, info, size, &written, NULL);
    ok(ret && written == size, "Failed to write bitmap info.\n");
    HeapFree(GetProcessHeap(), 0, info);

    size = w * h * (bpp / 8);
    buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
    for (i = 0, j = 0; i < size;)
    {
        if (palettized)
        {
            buffer[i++] = j++;
            j %= 256;
        }
        else
        {
            buffer[i++] = j % 251;
            buffer[i++] = j % 239;
            buffer[i++] = j++ % 247;
        }
    }
    ret = WriteFile(file, buffer, size, &written, NULL);
    ok(ret && written == size, "Failed to write bitmap data.\n");
    HeapFree(GetProcessHeap(), 0, buffer);

    CloseHandle(file);

    return filename;
}

static void test_bitmap_data(unsigned int test_idx, const D3DRMIMAGE *img,
        BOOL upside_down, unsigned int w, unsigned int h, BOOL palettized)
{
    const unsigned char *data = img->buffer1;
    unsigned int i, j;

    ok(img->width == w, "Test %u: Got unexpected image width %u, expected %u.\n", test_idx, img->width, w);
    ok(img->height == h, "Test %u: Got unexpected image height %u, expected %u.\n", test_idx, img->height, h);
    ok(img->aspectx == 1, "Test %u: Got unexpected image aspectx %u.\n", test_idx, img->aspectx);
    ok(img->aspecty == 1, "Test %u: Got unexpected image aspecty %u.\n", test_idx, img->aspecty);
    ok(!img->buffer2, "Test %u: Got unexpected image buffer2 %p.\n", test_idx, img->buffer2);

    /* The image is palettized if the total number of colors used is <= 256. */
    if (w * h > 256 && !palettized)
    {
        /* D3drm aligns the 24bpp texture to 4 bytes in the buffer, with one
         * byte padding from 24bpp texture. */
        ok(img->depth == 32, "Test %u: Got unexpected image depth %u.\n", test_idx, img->depth);
        ok(img->rgb == TRUE, "Test %u: Got unexpected image rgb %#x.\n", test_idx, img->rgb);
        ok(img->bytes_per_line == w * 4, "Test %u: Got unexpected image bytes per line %u, expected %u.\n",
                test_idx, img->bytes_per_line, w * 4);
        ok(img->red_mask == 0xff0000, "Test %u: Got unexpected image red mask %#x.\n", test_idx, img->red_mask);
        ok(img->green_mask == 0x00ff00, "Test %u: Got unexpected image green mask %#x.\n", test_idx, img->green_mask);
        ok(img->blue_mask == 0x0000ff, "Test %u: Got unexpected image blue mask %#x.\n", test_idx, img->blue_mask);
        ok(!img->alpha_mask, "Test %u: Got unexpected image alpha mask %#x.\n", test_idx, img->alpha_mask);
        ok(!img->palette_size, "Test %u: Got unexpected palette size %u.\n", test_idx, img->palette_size);
        ok(!img->palette, "Test %u: Got unexpected image palette %p.\n", test_idx, img->palette);
        for (i = 0; i < h; ++i)
        {
            for (j = 0; j < w; ++j)
            {
                const unsigned char *ptr = &data[i * img->bytes_per_line + j * 4];
                unsigned int idx = upside_down ? (h - 1 - i) * w + j : i * w + j;

                if (ptr[0] != idx % 251 || ptr[1] != idx % 239 || ptr[2] != idx % 247 || ptr[3] != 0xff)
                {
                    ok(0, "Test %u: Got unexpected color 0x%02x%02x%02x%02x at position %u, %u, "
                            "expected 0x%02x%02x%02x%02x.\n", test_idx, ptr[0], ptr[1], ptr[2], ptr[3],
                            j, i, idx % 251, idx % 239, idx % 247, 0xff);
                    return;
                }
            }
        }
        return;
    }

    ok(img->depth == 8, "Test %u: Got unexpected image depth %u.\n", test_idx, img->depth);
    ok(!img->rgb, "Test %u: Got unexpected image rgb %#x.\n", test_idx, img->rgb);
    ok(img->red_mask == 0xff, "Test %u: Got unexpected image red mask %#x.\n", test_idx, img->red_mask);
    ok(img->green_mask == 0xff, "Test %u: Got unexpected image green mask %#x.\n", test_idx, img->green_mask);
    ok(img->blue_mask == 0xff, "Test %u: Got unexpected image blue mask %#x.\n", test_idx, img->blue_mask);
    ok(!img->alpha_mask, "Test %u: Got unexpected image alpha mask %#x.\n", test_idx, img->alpha_mask);
    ok(!!img->palette, "Test %u: Got unexpected image palette %p.\n", test_idx, img->palette);
    if (!palettized)
    {
        /* In this case, bytes_per_line is aligned to the next multiple of
         * 4 from width. */
        ok(img->bytes_per_line == ((w + 3) & ~3), "Test %u: Got unexpected image bytes per line %u, expected %u.\n",
                test_idx, img->bytes_per_line, (w + 3) & ~3);
        ok(img->palette_size == w * h, "Test %u: Got unexpected palette size %u, expected %u.\n",
                test_idx, img->palette_size, w * h);
        for (i = 0; i < img->palette_size; ++i)
        {
            unsigned int idx = upside_down ? (h - 1) * w - i + (i % w) * 2 : i;
            ok(img->palette[i].red == idx % 251
                    && img->palette[i].green == idx % 239 && img->palette[i].blue == idx % 247,
                    "Test %u: Got unexpected palette entry (%u) color 0x%02x%02x%02x.\n",
                    test_idx, i, img->palette[i].red, img->palette[i].green, img->palette[i].blue);
            ok(img->palette[i].flags == D3DRMPALETTE_READONLY,
                    "Test %u: Got unexpected palette entry (%u) flags %#x.\n",
                    test_idx, i, img->palette[i].flags);
        }
        for (i = 0; i < h; ++i)
        {
            for (j = 0; j < w; ++j)
            {
                if (data[i * img->bytes_per_line + j] != i * w + j)
                {
                    ok(0, "Test %u: Got unexpected color 0x%02x at position %u, %u, expected 0x%02x.\n",
                            test_idx, data[i * img->bytes_per_line + j], j, i, i * w + j);
                    return;
                }
            }
        }
        return;
    }

    /* bytes_per_line is not always aligned by d3drm depending on the
     * format. */
    ok(img->bytes_per_line == w, "Test %u: Got unexpected image bytes per line %u, expected %u.\n",
            test_idx, img->bytes_per_line, w);
    ok(img->palette_size == 256, "Test %u: Got unexpected palette size %u.\n", test_idx, img->palette_size);
    for (i = 0; i < 256; ++i)
    {
        ok(img->palette[i].red == i && img->palette[i].green == i && img->palette[i].blue == i,
                "Test %u: Got unexpected palette entry (%u) color 0x%02x%02x%02x.\n",
                test_idx, i, img->palette[i].red, img->palette[i].green, img->palette[i].blue);
        ok(img->palette[i].flags == D3DRMPALETTE_READONLY,
                "Test %u: Got unexpected palette entry (%u) flags %#x.\n",
                test_idx, i, img->palette[i].flags);
    }
    for (i = 0; i < h; ++i)
    {
        for (j = 0; j < w; ++j)
        {
            unsigned int idx = upside_down ? (h - 1 - i) * w + j : i * w + j;
            if (data[i * img->bytes_per_line + j] != idx % 256)
            {
                ok(0, "Test %u: Got unexpected color 0x%02x at position %u, %u, expected 0x%02x.\n",
                        test_idx, data[i * img->bytes_per_line + j], j, i, idx % 256);
                return;
            }
        }
    }
}

static void test_load_texture(void)
{
    IDirect3DRMTexture3 *texture3;
    IDirect3DRMTexture2 *texture2;
    IDirect3DRMTexture *texture1;
    D3DRMIMAGE *d3drm_img;
    IDirect3DRM3 *d3drm3;
    IDirect3DRM2 *d3drm2;
    IDirect3DRM *d3drm1;
    ULONG ref1, ref2;
    unsigned int i;
    char *filename;
    HRESULT hr;
    BOOL ret;

    static const struct
    {
        unsigned int w;
        unsigned int h;
        BOOL palettized;
    }
    tests[] =
    {
        {100, 100, TRUE },
        {99,  100, TRUE },
        {100, 100, FALSE},
        {99,  100, FALSE},
        {3,   39,  FALSE},
    };

    hr = Direct3DRMCreate(&d3drm1);
    ok(hr == D3DRM_OK, "Failed to create IDirect3DRM object, hr %#x.\n", hr);
    hr = IDirect3DRM_QueryInterface(d3drm1, &IID_IDirect3DRM2, (void **)&d3drm2);
    ok(SUCCEEDED(hr), "Failed to get IDirect3DRM2 interface, hr %#x.\n", hr);
    hr = IDirect3DRM_QueryInterface(d3drm1, &IID_IDirect3DRM3, (void **)&d3drm3);
    ok(SUCCEEDED(hr), "Failed to get IDirect3DRM3 interface, hr %#x.\n", hr);
    ref1 = get_refcount((IUnknown *)d3drm1);

    /* Test all failures together. */
    texture1 = (IDirect3DRMTexture *)0xdeadbeef;
    hr = IDirect3DRM_LoadTexture(d3drm1, NULL, &texture1);
    ok(hr == D3DRMERR_BADVALUE, "Got unexpected hr %#x.\n", hr);
    ok(!texture1, "Got unexpected texture %p.\n", texture1);
    texture1 = (IDirect3DRMTexture *)0xdeadbeef;
    hr = IDirect3DRM_LoadTexture(d3drm1, "", &texture1);
    ok(hr == D3DRMERR_FILENOTFOUND, "Got unexpected hr %#x.\n", hr);
    ok(!texture1, "Got unexpected texture %p.\n", texture1);
    hr = IDirect3DRM_LoadTexture(d3drm1, NULL, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Got unexpected hr %#x.\n", hr);

    texture2 = (IDirect3DRMTexture2 *)0xdeadbeef;
    hr = IDirect3DRM2_LoadTexture(d3drm2, NULL, &texture2);
    ok(hr == D3DRMERR_FILENOTFOUND, "Got unexpected hr %#x.\n", hr);
    ok(!texture2, "Got unexpected texture %p.\n", texture2);
    texture2 = (IDirect3DRMTexture2 *)0xdeadbeef;
    hr = IDirect3DRM2_LoadTexture(d3drm2, "", &texture2);
    ok(hr == D3DRMERR_FILENOTFOUND, "Got unexpected hr %#x.\n", hr);
    ok(!texture2, "Got unexpected texture %p.\n", texture2);
    hr = IDirect3DRM2_LoadTexture(d3drm2, NULL, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Got unexpected hr %#x.\n", hr);

    texture3 = (IDirect3DRMTexture3 *)0xdeadbeef;
    hr = IDirect3DRM3_LoadTexture(d3drm3, NULL, &texture3);
    ok(hr == D3DRMERR_FILENOTFOUND, "Got unexpected hr %#x.\n", hr);
    ok(!texture3, "Got unexpected texture %p.\n", texture3);
    texture3 = (IDirect3DRMTexture3 *)0xdeadbeef;
    hr = IDirect3DRM_LoadTexture(d3drm3, "", &texture3);
    ok(hr == D3DRMERR_FILENOTFOUND, "Got unexpected hr %#x.\n", hr);
    ok(!texture3, "Got unexpected texture %p.\n", texture3);
    hr = IDirect3DRM3_LoadTexture(d3drm3, NULL, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Got unexpected hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        filename = create_bitmap(tests[i].w, tests[i].h, tests[i].palettized);

        hr = IDirect3DRM_LoadTexture(d3drm1, filename, &texture1);
        ok(SUCCEEDED(hr), "Test %u: Failed to load texture, hr %#x.\n", i, hr);
        ref2 = get_refcount((IUnknown *)d3drm1);
        ok(ref2 > ref1, "Test %u: expected ref2 > ref1, got ref1 %u, ref2 %u.\n", i, ref1, ref2);

        hr = IDirect3DRMTexture_InitFromFile(texture1, filename);
        ok(hr == D3DRMERR_BADOBJECT, "Test %u: Got unexpected hr %#x.\n", i, hr);
        /* InitFromFile() seems to AddRef() IDirect3DRM even if it fails. */
        IDirect3DRM_Release(d3drm1);
        d3drm_img = IDirect3DRMTexture_GetImage(texture1);
        ok(!!d3drm_img, "Test %u: Failed to get image.\n", i);
        test_bitmap_data(i * 7, d3drm_img, FALSE, tests[i].w, tests[i].h, tests[i].palettized);
        IDirect3DRMTexture_Release(texture1);
        ref2 = get_refcount((IUnknown *)d3drm1);
        ok(ref1 == ref2, "Test %u: expected ref1 == ref2, got ref1 = %u, ref2 = %u.\n", i, ref1, ref2);
        hr = IDirect3DRM_CreateObject(d3drm1, &CLSID_CDirect3DRMTexture,
                NULL, &IID_IDirect3DRMTexture, (void **)&texture1);
        ok(SUCCEEDED(hr), "Test %u: Failed to create texture, hr %#x.\n", i, hr);
        hr = IDirect3DRMTexture_InitFromFile(texture1, NULL);
        ok(hr == D3DRMERR_BADOBJECT, "Test %u: Got unexpected hr %#x.\n", i, hr);
        hr = IDirect3DRMTexture_InitFromFile(texture1, "");
        ok(hr == D3DRMERR_BADOBJECT, "Test %u: Got unexpected hr %#x.\n", i, hr);
        hr = IDirect3DRMTexture_InitFromFile(texture1, filename);
        ok(SUCCEEDED(hr), "Test %u: Failed to initialise texture from file, hr %#x.\n", i, hr);
        d3drm_img = IDirect3DRMTexture_GetImage(texture1);
        ok(!!d3drm_img, "Test %u: Failed to get image.\n", i);
        test_bitmap_data(i * 7 + 1, d3drm_img, FALSE, tests[i].w, tests[i].h, tests[i].palettized);
        IDirect3DRMTexture_Release(texture1);

        hr = IDirect3DRM2_LoadTexture(d3drm2, filename, &texture2);
        ok(SUCCEEDED(hr), "Test %u: Failed to load texture, hr %#x.\n", i, hr);
        ref2 = get_refcount((IUnknown *)d3drm1);
        ok(ref2 > ref1, "Test %u: expected ref2 > ref1, got ref1 %u, ref2 %u.\n", i, ref1, ref2);

        hr = IDirect3DRMTexture2_InitFromFile(texture2, filename);
        ok(hr == D3DRMERR_BADOBJECT, "Test %u: Got unexpected hr %#x.\n", i, hr);
        IDirect3DRM_Release(d3drm1);
        d3drm_img = IDirect3DRMTexture2_GetImage(texture2);
        ok(!!d3drm_img, "Test %u: Failed to get image.\n", i);
        test_bitmap_data(i * 7 + 2, d3drm_img, TRUE, tests[i].w, tests[i].h, tests[i].palettized);
        IDirect3DRMTexture2_Release(texture2);
        ref2 = get_refcount((IUnknown *)d3drm1);
        ok(ref1 == ref2, "Test %u: expected ref1 == ref2, got ref1 = %u, ref2 = %u.\n", i, ref1, ref2);
        hr = IDirect3DRM2_CreateObject(d3drm2, &CLSID_CDirect3DRMTexture,
                NULL, &IID_IDirect3DRMTexture2, (void **)&texture2);
        ok(SUCCEEDED(hr), "Test %u: Failed to create texture, hr %#x.\n", i, hr);
        hr = IDirect3DRMTexture2_InitFromFile(texture2, NULL);
        ok(hr == D3DRMERR_BADOBJECT, "Test %u: Got unexpected hr %#x.\n", i, hr);
        hr = IDirect3DRMTexture2_InitFromFile(texture2, "");
        ok(hr == D3DRMERR_BADOBJECT, "Test %u: Got unexpected hr %#x.\n", i, hr);
        hr = IDirect3DRMTexture2_InitFromFile(texture2, filename);
        ok(SUCCEEDED(hr), "Test %u: Failed to initialise texture from file, hr %#x.\n", i, hr);
        d3drm_img = IDirect3DRMTexture2_GetImage(texture2);
        ok(!!d3drm_img, "Test %u: Failed to get image.\n", i);
        test_bitmap_data(i * 7 + 3, d3drm_img, TRUE, tests[i].w, tests[i].h, tests[i].palettized);
        IDirect3DRMTexture2_Release(texture2);

        hr = IDirect3DRM3_LoadTexture(d3drm3, filename, &texture3);
        ok(SUCCEEDED(hr), "Test %u: Failed to load texture, hr %#x.\n", i, hr);
        ref2 = get_refcount((IUnknown *)d3drm1);
        ok(ref2 > ref1, "Test %u: expected ref2 > ref1, got ref1 %u, ref2 %u.\n", i, ref1, ref2);

        hr = IDirect3DRMTexture3_InitFromFile(texture3, filename);
        ok(hr == D3DRMERR_BADOBJECT, "Test %u: Got unexpected hr %#x.\n", i, hr);
        IDirect3DRM_Release(d3drm1);
        d3drm_img = IDirect3DRMTexture3_GetImage(texture3);
        ok(!!d3drm_img, "Test %u: Failed to get image.\n", i);
        test_bitmap_data(i * 7 + 4, d3drm_img, TRUE, tests[i].w, tests[i].h, tests[i].palettized);
        /* Test whether querying a version 1 texture from version 3 causes a
         * change in the loading behavior. */
        hr = IDirect3DRMTexture3_QueryInterface(texture3, &IID_IDirect3DRMTexture, (void **)&texture1);
        ok(SUCCEEDED(hr), "Failed to get IDirect3DRMTexture interface, hr %#x.\n", hr);
        d3drm_img = IDirect3DRMTexture_GetImage(texture1);
        ok(!!d3drm_img, "Test %u: Failed to get image.\n", i);
        test_bitmap_data(i * 7 + 5, d3drm_img, TRUE, tests[i].w, tests[i].h, tests[i].palettized);
        IDirect3DRMTexture_Release(texture1);
        IDirect3DRMTexture3_Release(texture3);
        ref2 = get_refcount((IUnknown *)d3drm1);
        ok(ref1 == ref2, "Test %u: expected ref1 == ref2, got ref1 = %u, ref2 = %u.\n", i, ref1, ref2);

        hr = IDirect3DRM3_CreateObject(d3drm3, &CLSID_CDirect3DRMTexture,
                NULL, &IID_IDirect3DRMTexture3, (void **)&texture3);
        ok(SUCCEEDED(hr), "Test %u: Failed to create texture, hr %#x.\n", i, hr);
        hr = IDirect3DRMTexture3_InitFromFile(texture3, NULL);
        ok(hr == D3DRMERR_BADOBJECT, "Test %u: Got unexpected hr %#x.\n", i, hr);
        hr = IDirect3DRMTexture3_InitFromFile(texture3, "");
        ok(hr == D3DRMERR_BADOBJECT, "Test %u: Got unexpected hr %#x.\n", i, hr);
        hr = IDirect3DRMTexture3_InitFromFile(texture3, filename);
        ok(SUCCEEDED(hr), "Test %u: Failed to initialize texture from file, hr %#x.\n", i, hr);
        d3drm_img = IDirect3DRMTexture3_GetImage(texture3);
        ok(!!d3drm_img, "Test %u: Failed to get image.\n", i);
        test_bitmap_data(i * 7 + 6, d3drm_img, TRUE, tests[i].w, tests[i].h, tests[i].palettized);
        IDirect3DRMTexture3_Release(texture3);

        ret = DeleteFileA(filename);
        ok(ret, "Test %u: Failed to delete bitmap \"%s\".\n", i, filename);
        HeapFree(GetProcessHeap(), 0, filename);
    }

    IDirect3DRM3_Release(d3drm3);
    IDirect3DRM2_Release(d3drm2);
    IDirect3DRM_Release(d3drm1);
}

static void test_texture_qi(void)
{
    static const struct qi_test tests[] =
    {
        { &IID_IDirect3DRM3,               NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRM2,               NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRM,                NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMDevice,          NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMDevice2,         NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMDevice3,         NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMWinDevice,       NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMObject,          &IID_IUnknown, &IID_IDirect3DRMTexture,  S_OK                      },
        { &IID_IDirect3DRMViewport,        NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMViewport2,       NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFrame,           NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFrame2,          NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFrame3,          NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMVisual,          &IID_IUnknown, &IID_IDirect3DRMTexture,  S_OK                      },
        { &IID_IDirect3DRMMesh,            NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMeshBuilder,     NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMeshBuilder2,    NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMeshBuilder3,    NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFace,            NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFace2,           NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMLight,           NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMTexture,         &IID_IUnknown, &IID_IDirect3DRMTexture,  S_OK                      },
        { &IID_IDirect3DRMTexture2,        &IID_IUnknown, &IID_IDirect3DRMTexture2, S_OK                      },
        { &IID_IDirect3DRMTexture3,        &IID_IUnknown, &IID_IDirect3DRMTexture3, S_OK                      },
        { &IID_IDirect3DRMWrap,            NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMaterial,        NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMaterial2,       NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimation,       NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimation2,      NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimationSet,    NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimationSet2,   NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMObjectArray,     NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMDeviceArray,     NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMViewportArray,   NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFrameArray,      NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMVisualArray,     NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMLightArray,      NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMPickedArray,     NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFaceArray,       NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimationArray,  NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMUserVisual,      NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMShadow,          NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMShadow2,         NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMInterpolator,    NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMProgressiveMesh, NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMPicked2Array,    NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMClippedVisual,   NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawClipper,         NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface7,        NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface4,        NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface3,        NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface2,        NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface,         NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice7,           NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice3,           NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice2,           NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice,            NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D7,                 NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D3,                 NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D2,                 NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D,                  NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw7,               NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw4,               NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw3,               NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw2,               NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw,                NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DLight,             NULL,          NULL,                     CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IUnknown,                   &IID_IUnknown, NULL,                     S_OK,                     },
    };
    HRESULT hr;
    IDirect3DRM *d3drm1;
    IDirect3DRM2 *d3drm2;
    IDirect3DRM3 *d3drm3;
    IDirect3DRMTexture *texture1;
    IDirect3DRMTexture2 *texture2;
    IDirect3DRMTexture3 *texture3;
    IUnknown *unknown;
    char *filename;
    BOOL check;

    hr = Direct3DRMCreate(&d3drm1);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRM interface (hr = %#x)\n", hr);
    filename = create_bitmap(1, 1, TRUE);
    hr = IDirect3DRM_LoadTexture(d3drm1, filename, &texture1);
    ok(SUCCEEDED(hr), "Failed to load texture (hr = %#x).\n", hr);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMTexture interface (hr = %#x)\n", hr);
    hr = IDirect3DRMTexture_QueryInterface(texture1, &IID_IUnknown, (void **)&unknown);
    ok(SUCCEEDED(hr), "Cannot get IUnknown interface from IDirect3DRMTexture (hr = %#x)\n", hr);
    IDirect3DRMTexture_Release(texture1);
    test_qi("texture1_qi", unknown, &IID_IUnknown, tests, ARRAY_SIZE(tests));
    IUnknown_Release(unknown);

    hr = IDirect3DRM_QueryInterface(d3drm1, &IID_IDirect3DRM2, (void **)&d3drm2);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRM2 interface (hr = %#x).\n", hr);
    hr = IDirect3DRM2_LoadTexture(d3drm2, filename, &texture2);
    ok(SUCCEEDED(hr), "Failed to load texture (hr = %#x).\n", hr);
    hr = IDirect3DRMTexture2_QueryInterface(texture2, &IID_IUnknown, (void **)&unknown);
    ok(SUCCEEDED(hr), "Cannot get IUnknown interface from IDirect3DRMTexture2 (hr = %#x)\n", hr);
    IDirect3DRMTexture2_Release(texture2);
    test_qi("texture2_qi", unknown, &IID_IUnknown, tests, ARRAY_SIZE(tests));
    IUnknown_Release(unknown);

    hr = IDirect3DRM_QueryInterface(d3drm1, &IID_IDirect3DRM3, (void **)&d3drm3);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRM3 interface (hr = %#x).\n", hr);
    hr = IDirect3DRM3_LoadTexture(d3drm3, filename, &texture3);
    ok(SUCCEEDED(hr), "Failed to load texture (hr = %#x).\n", hr);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMTexture3 interface (hr = %#x)\n", hr);
    hr = IDirect3DRMTexture3_QueryInterface(texture3, &IID_IUnknown, (void **)&unknown);
    ok(SUCCEEDED(hr), "Cannot get IUnknown interface from IDirect3DRMTexture3 (hr = %#x)\n", hr);
    IDirect3DRMTexture3_Release(texture3);
    test_qi("texture3_qi", unknown, &IID_IUnknown, tests, ARRAY_SIZE(tests));
    IUnknown_Release(unknown);

    IDirect3DRM3_Release(d3drm3);
    IDirect3DRM2_Release(d3drm2);
    IDirect3DRM_Release(d3drm1);
    check = DeleteFileA(filename);
    ok(check, "Cannot delete image stored in %s (error = %d).\n", filename, GetLastError());
    HeapFree(GetProcessHeap(), 0, filename);
}

static void test_viewport_qi(void)
{
    IDirect3DRM *d3drm1;
    IDirect3DRM2 *d3drm2;
    IDirect3DRM3 *d3drm3;
    IDirect3DRMFrame *frame1, *camera1;
    IDirect3DRMFrame3 *frame3, *camera3;
    IDirect3DRMDevice *device1;
    IDirect3DRMDevice3 *device3;
    IDirectDrawClipper *clipper;
    IDirect3DRMViewport *viewport1;
    IDirect3DRMViewport2 *viewport2;
    IUnknown *unknown;
    GUID driver = IID_IDirect3DRGBDevice;
    HRESULT hr;

    static const struct qi_test tests[] =
    {
        { &IID_IDirect3DRM3,               NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRM2,               NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRM,                NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMDevice,          NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMDevice2,         NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMDevice3,         NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMWinDevice,       NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMObject,          &IID_IUnknown, &IID_IDirect3DRMViewport,  S_OK                      },
        { &IID_IDirect3DRMViewport,        &IID_IUnknown, &IID_IDirect3DRMViewport,  S_OK                      },
        { &IID_IDirect3DRMViewport2,       &IID_IUnknown, &IID_IDirect3DRMViewport2, S_OK                      },
        { &IID_IDirect3DRMFrame,           NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFrame2,          NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFrame3,          NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMVisual,          NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMesh,            NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMeshBuilder,     NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMeshBuilder2,    NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMeshBuilder3,    NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFace,            NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFace2,           NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMLight,           NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMTexture,         NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMTexture2,        NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMTexture3,        NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMWrap,            NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMaterial,        NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMaterial2,       NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimation,       NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimation2,      NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimationSet,    NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimationSet2,   NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMObjectArray,     NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMDeviceArray,     NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMViewportArray,   NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFrameArray,      NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMVisualArray,     NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMLightArray,      NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMPickedArray,     NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFaceArray,       NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimationArray,  NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMUserVisual,      NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMShadow,          NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMShadow2,         NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMInterpolator,    NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMProgressiveMesh, NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMPicked2Array,    NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMClippedVisual,   NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawClipper,         NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface7,        NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface4,        NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface3,        NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface2,        NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface,         NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice7,           NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice3,           NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice2,           NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice,            NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D7,                 NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D3,                 NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D2,                 NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D,                  NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw7,               NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw4,               NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw3,               NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw2,               NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw,                NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DLight,             NULL,          NULL,                      CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IUnknown,                   &IID_IUnknown, NULL,                      S_OK,                     },
    };

    hr = DirectDrawCreateClipper(0, &clipper, NULL);
    ok(SUCCEEDED(hr), "Cannot get IDirectDrawClipper interface (hr = %#x).\n", hr);

    hr = Direct3DRMCreate(&d3drm1);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRM interface (hr = %#x).\n", hr);

    hr = IDirect3DRM_CreateDeviceFromClipper(d3drm1, clipper, &driver, 640, 480, &device1);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMDevice interface (hr = %#x).\n", hr);
    hr = IDirect3DRM_CreateFrame(d3drm1, NULL, &frame1);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMFrame interface (hr = %#x)\n", hr);
    hr = IDirect3DRM_CreateFrame(d3drm1, frame1, &camera1);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMFrame interface (hr = %#x)\n", hr);
    hr = IDirect3DRM_CreateViewport(d3drm1, device1, camera1, 0, 0, 640, 480, &viewport1);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMViewport interface (hr = %#x)\n", hr);
    hr = IDirect3DRMViewport_QueryInterface(viewport1, &IID_IUnknown, (void **)&unknown);
    ok(SUCCEEDED(hr), "Cannot get IUnknown interface (hr = %#x).\n", hr);
    IDirect3DRMViewport_Release(viewport1);
    test_qi("viewport1_qi", unknown, &IID_IUnknown, tests, ARRAY_SIZE(tests));
    IUnknown_Release(unknown);

    hr = IDirect3DRM_QueryInterface(d3drm1, &IID_IDirect3DRM2, (void **)&d3drm2);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRM2 interface (hr = %#x).\n", hr);
    hr = IDirect3DRM2_CreateViewport(d3drm2, device1, camera1, 0, 0, 640, 480, &viewport1);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMViewport interface (hr = %#x)\n", hr);
    hr = IDirect3DRMViewport_QueryInterface(viewport1, &IID_IUnknown, (void **)&unknown);
    ok(SUCCEEDED(hr), "Cannot get IUnknown interface (hr = %#x).\n", hr);
    IDirect3DRMViewport_Release(viewport1);
    test_qi("viewport1_qi", unknown, &IID_IUnknown, tests, ARRAY_SIZE(tests));
    IUnknown_Release(unknown);
    IDirect3DRMDevice_Release(device1);
    IDirect3DRMFrame_Release(camera1);
    IDirect3DRMFrame_Release(frame1);

    hr = IDirect3DRM_QueryInterface(d3drm1, &IID_IDirect3DRM3, (void **)&d3drm3);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRM3 interface (hr = %#x).\n", hr);
    hr = IDirect3DRM3_CreateDeviceFromClipper(d3drm3, clipper, &driver, 640, 480, &device3);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMDevice3 interface (hr = %#x).\n", hr);
    hr = IDirect3DRM3_CreateFrame(d3drm3, NULL, &frame3);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMFrame3 interface (hr = %#x)\n", hr);
    hr = IDirect3DRM3_CreateFrame(d3drm3, frame3, &camera3);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMFrame3 interface (hr = %#x)\n", hr);
    hr = IDirect3DRM3_CreateViewport(d3drm3, device3, camera3, 0, 0, 640, 480, &viewport2);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMViewport2 interface (hr = %#x)\n", hr);
    hr = IDirect3DRMViewport2_QueryInterface(viewport2, &IID_IUnknown, (void **)&unknown);
    ok(SUCCEEDED(hr), "Cannot get IUnknown interface (hr = %#x).\n", hr);
    IDirect3DRMViewport_Release(viewport2);
    test_qi("viewport2_qi", unknown, &IID_IUnknown, tests, ARRAY_SIZE(tests));
    IUnknown_Release(unknown);
    IDirect3DRMDevice3_Release(device3);
    IDirect3DRMFrame3_Release(camera3);
    IDirect3DRMFrame3_Release(frame3);

    IDirectDrawClipper_Release(clipper);
    IDirect3DRM3_Release(d3drm3);
    IDirect3DRM2_Release(d3drm2);
    IDirect3DRM_Release(d3drm1);
}

static D3DCOLOR get_surface_color(IDirectDrawSurface *surface, UINT x, UINT y)
{
    RECT rect = { x, y, x + 1, y + 1 };
    DDSURFACEDESC surface_desc;
    D3DCOLOR color;
    HRESULT hr;

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);

    hr = IDirectDrawSurface_Lock(surface, &rect, &surface_desc, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    if (FAILED(hr))
        return 0xdeadbeef;

    color = *((DWORD *)surface_desc.lpSurface) & 0x00ffffff;

    hr = IDirectDrawSurface_Unlock(surface, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    return color;
}

static IDirect3DDevice2 *create_device2_without_ds(IDirectDraw2 *ddraw, HWND window)
{
    IDirectDrawSurface *surface;
    IDirect3DDevice2 *device = NULL;
    DDSURFACEDESC surface_desc;
    IDirect3D2 *d3d;
    HRESULT hr;
    RECT rc;

    GetClientRect(window, &rc);
    hr = IDirectDraw2_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    surface_desc.dwWidth = rc.right;
    surface_desc.dwHeight = rc.bottom;

    hr = IDirectDraw2_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDraw2_QueryInterface(ddraw, &IID_IDirect3D2, (void **)&d3d);
    if (FAILED(hr))
    {
        IDirectDrawSurface_Release(surface);
        return NULL;
    }

    IDirect3D2_CreateDevice(d3d, &IID_IDirect3DHALDevice, surface, &device);

    IDirect3D2_Release(d3d);
    IDirectDrawSurface_Release(surface);
    return device;
}

static BOOL compare_color(D3DCOLOR c1, D3DCOLOR c2, BYTE max_diff)
{
    if ((c1 & 0xff) - (c2 & 0xff) > max_diff) return FALSE;
    c1 >>= 8; c2 >>= 8;
    if ((c1 & 0xff) - (c2 & 0xff) > max_diff) return FALSE;
    c1 >>= 8; c2 >>= 8;
    if ((c1 & 0xff) - (c2 & 0xff) > max_diff) return FALSE;
    c1 >>= 8; c2 >>= 8;
    if ((c1 & 0xff) - (c2 & 0xff) > max_diff) return FALSE;
    return TRUE;
}

static void clear_depth_surface(IDirectDrawSurface *surface, DWORD value)
{
    HRESULT hr;
    DDBLTFX fx;

    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    U5(fx).dwFillDepth = value;

    hr = IDirectDrawSurface_Blt(surface, NULL, NULL, NULL, DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
}

static void set_execute_data(IDirect3DExecuteBuffer *execute_buffer, UINT vertex_count, UINT offset, UINT len)
{
    D3DEXECUTEDATA exec_data;
    HRESULT hr;

    memset(&exec_data, 0, sizeof(exec_data));
    exec_data.dwSize = sizeof(exec_data);
    exec_data.dwVertexCount = vertex_count;
    exec_data.dwInstructionOffset = offset;
    exec_data.dwInstructionLength = len;
    hr = IDirect3DExecuteBuffer_SetExecuteData(execute_buffer, &exec_data);
    ok(SUCCEEDED(hr), "Failed to set execute data, hr %#x.\n", hr);
}

static void emit_set_ts(void **ptr, D3DTRANSFORMSTATETYPE state, DWORD value)
{
    D3DINSTRUCTION *inst = *ptr;
    D3DSTATE *ts = (D3DSTATE *)(inst + 1);

    inst->bOpcode = D3DOP_STATETRANSFORM;
    inst->bSize = sizeof(*ts);
    inst->wCount = 1;

    U1(*ts).dtstTransformStateType = state;
    U2(*ts).dwArg[0] = value;

    *ptr = ts + 1;
}

static void emit_set_rs(void **ptr, D3DRENDERSTATETYPE state, DWORD value)
{
    D3DINSTRUCTION *inst = *ptr;
    D3DSTATE *rs = (D3DSTATE *)(inst + 1);

    inst->bOpcode = D3DOP_STATERENDER;
    inst->bSize = sizeof(*rs);
    inst->wCount = 1;

    U1(*rs).drstRenderStateType = state;
    U2(*rs).dwArg[0] = value;

    *ptr = rs + 1;
}

static void emit_process_vertices(void **ptr, DWORD flags, WORD base_idx, DWORD vertex_count)
{
    D3DINSTRUCTION *inst = *ptr;
    D3DPROCESSVERTICES *pv = (D3DPROCESSVERTICES *)(inst + 1);

    inst->bOpcode = D3DOP_PROCESSVERTICES;
    inst->bSize = sizeof(*pv);
    inst->wCount = 1;

    pv->dwFlags = flags;
    pv->wStart = base_idx;
    pv->wDest = 0;
    pv->dwCount = vertex_count;
    pv->dwReserved = 0;

    *ptr = pv + 1;
}

static void emit_tquad(void **ptr, WORD base_idx)
{
    D3DINSTRUCTION *inst = *ptr;
    D3DTRIANGLE *tri = (D3DTRIANGLE *)(inst + 1);

    inst->bOpcode = D3DOP_TRIANGLE;
    inst->bSize = sizeof(*tri);
    inst->wCount = 2;

    U1(*tri).v1 = base_idx;
    U2(*tri).v2 = base_idx + 1;
    U3(*tri).v3 = base_idx + 2;
    tri->wFlags = D3DTRIFLAG_START;
    ++tri;

    U1(*tri).v1 = base_idx + 2;
    U2(*tri).v2 = base_idx + 1;
    U3(*tri).v3 = base_idx + 3;
    tri->wFlags = D3DTRIFLAG_ODD;
    ++tri;

    *ptr = tri;
}

static void emit_end(void **ptr)
{
    D3DINSTRUCTION *inst = *ptr;

    inst->bOpcode = D3DOP_EXIT;
    inst->bSize = 0;
    inst->wCount = 0;

    *ptr = inst + 1;
}

static void d3d_draw_quad1(IDirect3DDevice *device, IDirect3DViewport *viewport)
{
    IDirect3DExecuteBuffer *execute_buffer;
    D3DEXECUTEBUFFERDESC exec_desc;
    HRESULT hr;
    void *ptr;
    UINT inst_length;
    D3DMATRIXHANDLE world_handle, view_handle, proj_handle;
    static D3DMATRIX mat =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    static const D3DLVERTEX quad_strip[] =
    {
        {{-1.0f}, {-1.0f}, {0.00f}, 0, {0xffbada55}, {0}, {0.0f}, {0.0f}},
        {{-1.0f}, { 1.0f}, {0.00f}, 0, {0xffbada55}, {0}, {0.0f}, {0.0f}},
        {{ 1.0f}, {-1.0f}, {1.00f}, 0, {0xffbada55}, {0}, {0.0f}, {0.0f}},
        {{ 1.0f}, { 1.0f}, {1.00f}, 0, {0xffbada55}, {0}, {0.0f}, {0.0f}},
    };

    hr = IDirect3DDevice_CreateMatrix(device, &world_handle);
    ok(hr == D3D_OK, "Creating a matrix object failed, hr %#x.\n", hr);
    hr = IDirect3DDevice_SetMatrix(device, world_handle, &mat);
    ok(hr == D3D_OK, "Setting a matrix object failed, hr %#x.\n", hr);
    hr = IDirect3DDevice_CreateMatrix(device, &view_handle);
    ok(hr == D3D_OK, "Creating a matrix object failed, hr %#x.\n", hr);
    hr = IDirect3DDevice_SetMatrix(device, view_handle, &mat);
    ok(hr == D3D_OK, "Setting a matrix object failed, hr %#x.\n", hr);
    hr = IDirect3DDevice_CreateMatrix(device, &proj_handle);
    ok(hr == D3D_OK, "Creating a matrix object failed, hr %#x.\n", hr);
    hr = IDirect3DDevice_SetMatrix(device, proj_handle, &mat);
    ok(hr == D3D_OK, "Setting a matrix object failed, hr %#x.\n", hr);

    memset(&exec_desc, 0, sizeof(exec_desc));
    exec_desc.dwSize = sizeof(exec_desc);
    exec_desc.dwFlags = D3DDEB_BUFSIZE | D3DDEB_CAPS;
    exec_desc.dwBufferSize = 1024;
    exec_desc.dwCaps = D3DDEBCAPS_SYSTEMMEMORY;

    hr = IDirect3DDevice_CreateExecuteBuffer(device, &exec_desc, &execute_buffer, NULL);
    ok(SUCCEEDED(hr), "Failed to create execute buffer, hr %#x.\n", hr);

    hr = IDirect3DExecuteBuffer_Lock(execute_buffer, &exec_desc);
    ok(SUCCEEDED(hr), "Failed to lock execute buffer, hr %#x.\n", hr);

    memcpy(exec_desc.lpData, quad_strip, sizeof(quad_strip));
    ptr = ((BYTE *)exec_desc.lpData) + sizeof(quad_strip);
    emit_set_ts(&ptr, D3DTRANSFORMSTATE_WORLD, world_handle);
    emit_set_ts(&ptr, D3DTRANSFORMSTATE_VIEW, view_handle);
    emit_set_ts(&ptr, D3DTRANSFORMSTATE_PROJECTION, proj_handle);
    emit_set_rs(&ptr, D3DRENDERSTATE_CLIPPING, FALSE);
    emit_set_rs(&ptr, D3DRENDERSTATE_ZENABLE, TRUE);
    emit_set_rs(&ptr, D3DRENDERSTATE_FOGENABLE, FALSE);
    emit_set_rs(&ptr, D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
    emit_set_rs(&ptr, D3DRENDERSTATE_SHADEMODE, D3DSHADE_FLAT);

    emit_process_vertices(&ptr, D3DPROCESSVERTICES_TRANSFORM, 0, 4);
    emit_tquad(&ptr, 0);

    emit_end(&ptr);
    inst_length = (BYTE *)ptr - (BYTE *)exec_desc.lpData;
    inst_length -= sizeof(quad_strip);

    hr = IDirect3DExecuteBuffer_Unlock(execute_buffer);
    ok(SUCCEEDED(hr), "Failed to unlock execute buffer, hr %#x.\n", hr);

    hr = IDirect3DDevice_BeginScene(device);
    set_execute_data(execute_buffer, 4, sizeof(quad_strip), inst_length);
    hr = IDirect3DDevice_Execute(device, execute_buffer, viewport, D3DEXECUTE_CLIPPED);
    ok(SUCCEEDED(hr), "Failed to execute exec buffer, hr %#x.\n", hr);
    hr = IDirect3DDevice_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    IDirect3DExecuteBuffer_Release(execute_buffer);
}

static void test_viewport_clear1(void)
{
    DDSCAPS caps = { DDSCAPS_ZBUFFER };
    IDirectDraw *ddraw;
    IDirectDrawClipper *clipper;
    IDirect3DRM *d3drm1;
    IDirect3DRMFrame *frame1, *camera1;
    IDirect3DRMDevice *device1;
    IDirect3DViewport *d3d_viewport;
    IDirect3DRMViewport *viewport1;
    IDirect3DDevice *d3d_device1;
    IDirectDrawSurface *surface, *ds, *d3drm_ds;
    HWND window;
    GUID driver = IID_IDirect3DRGBDevice;
    HRESULT hr;
    D3DCOLOR ret_color;
    RECT rc;

    window = create_window();
    GetClientRect(window, &rc);

    hr = DirectDrawCreate(NULL, &ddraw, NULL);
    ok(SUCCEEDED(hr), "Cannot create IDirectDraw interface (hr = %#x).\n", hr);

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level (hr = %#x).\n", hr);

    hr = IDirectDraw_CreateClipper(ddraw, 0, &clipper, NULL);
    ok(SUCCEEDED(hr), "Cannot create clipper (hr = %#x).\n", hr);

    hr = IDirectDrawClipper_SetHWnd(clipper, 0, window);
    ok(SUCCEEDED(hr), "Cannot set HWnd to Clipper (hr = %#x)\n", hr);

    hr = Direct3DRMCreate(&d3drm1);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRM interface (hr = %#x).\n", hr);

    hr = IDirect3DRM_CreateDeviceFromClipper(d3drm1, clipper, &driver, rc.right, rc.bottom, &device1);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMDevice interface (hr = %#x)\n", hr);

    hr = IDirect3DRM_CreateFrame(d3drm1, NULL, &frame1);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMFrame interface (hr = %#x)\n", hr);
    hr = IDirect3DRM_CreateFrame(d3drm1, frame1, &camera1);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMFrame interface (hr = %#x)\n", hr);

    hr = IDirect3DRM_CreateViewport(d3drm1, device1, camera1, 0, 0, rc.right,
            rc.bottom, &viewport1);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMViewport2 interface (hr = %#x)\n", hr);

    /* Fetch immediate mode device and viewport */
    hr = IDirect3DRMDevice_GetDirect3DDevice(device1, &d3d_device1);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DDevice interface (hr = %#x).\n", hr);
    hr = IDirect3DRMViewport_GetDirect3DViewport(viewport1, &d3d_viewport);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DViewport interface (hr = %#x).\n", hr);

    hr = IDirect3DDevice_QueryInterface(d3d_device1, &IID_IDirectDrawSurface, (void **)&surface);
    ok(SUCCEEDED(hr), "Cannot get surface to the render target (hr = %#x).\n", hr);

    ret_color = get_surface_color(surface, 320, 240);
    ok(compare_color(ret_color, 0, 1), "Got unexpected color 0x%08x.\n", ret_color);

    /* Clear uses the scene frame's background color. */
    hr = IDirect3DRMFrame_SetSceneBackgroundRGB(frame1, 1.0f, 1.0f, 1.0f);
    ok(SUCCEEDED(hr), "Cannot set scene background RGB (hr = %#x)\n", hr);
    ret_color = IDirect3DRMFrame_GetSceneBackground(frame1);
    ok(ret_color == 0xffffffff, "Expected scene color returned == 0xffffffff, got %#x.\n", ret_color);
    hr = IDirect3DRMFrame_SetSceneBackgroundRGB(camera1, 0.0f, 1.0f, 0.0f);
    ok(SUCCEEDED(hr), "Cannot set scene background RGB (hr = %#x)\n", hr);
    ret_color = IDirect3DRMFrame_GetSceneBackground(camera1);
    ok(ret_color == 0xff00ff00, "Expected scene color returned == 0xff00ff00, got %#x.\n", ret_color);

    CHECK_REFCOUNT(frame1, 1);
    hr = IDirect3DRMViewport_Clear(viewport1);
    ok(SUCCEEDED(hr), "Cannot clear viewport (hr = %#x).\n", hr);
    ret_color = get_surface_color(surface, 320, 240);
    ok(compare_color(ret_color, 0x00ffffff, 1), "Got unexpected color 0x%08x.\n", ret_color);
    CHECK_REFCOUNT(frame1, 1);

    hr = IDirect3DRMFrame_SetSceneBackgroundRGB(frame1, 0.0f, 0.0f, 1.0f);
    ok(SUCCEEDED(hr), "Cannot set scene background RGB (hr = %#x)\n", hr);
    ret_color = IDirect3DRMFrame_GetSceneBackground(frame1);
    ok(ret_color == 0xff0000ff, "Expected scene color returned == 0xff00ff00, got %#x.\n", ret_color);

    hr = IDirect3DRMViewport_Configure(viewport1, 0, 0, rc.right, rc.bottom);
    todo_wine ok(SUCCEEDED(hr), "Cannot configure viewport (hr = %#x).\n", hr);
    hr = IDirect3DRMViewport_Clear(viewport1);
    ok(SUCCEEDED(hr), "Cannot clear viewport (hr = %#x).\n", hr);
    ret_color = get_surface_color(surface, 100, 200);
    ok(compare_color(ret_color, 0x000000ff, 1), "Got unexpected color 0x%08x.\n", ret_color);

    d3d_draw_quad1(d3d_device1, d3d_viewport);

    ret_color = get_surface_color(surface, 100, 200);
    ok(compare_color(ret_color, 0x00bada55, 1), "Got unexpected color 0x%08x.\n", ret_color);

    hr = IDirectDrawSurface_GetAttachedSurface(surface, &caps, &ds);
    ok(SUCCEEDED(hr), "Cannot get attached depth surface (hr = %x).\n", hr);

    hr = IDirect3DRMViewport_Configure(viewport1, 0, 0, rc.right, rc.bottom);
    todo_wine ok(SUCCEEDED(hr), "Cannot configure viewport (hr = %#x).\n", hr);
    hr = IDirect3DRMViewport_Clear(viewport1);
    ok(SUCCEEDED(hr), "Cannot clear viewport (hr = %#x).\n", hr);
    ret_color = get_surface_color(surface, 100, 200);
    ok(compare_color(ret_color, 0x000000ff, 1), "Got unexpected color 0x%08x.\n", ret_color);

    /* Fill the depth surface with a value lower than the quad's depth value. */
    clear_depth_surface(ds, 0x7fff);

    /* Depth test passes here */
    d3d_draw_quad1(d3d_device1, d3d_viewport);
    ret_color = get_surface_color(surface, 100, 200);
    ok(compare_color(ret_color, 0x00bada55, 1), "Got unexpected color 0x%08x.\n", ret_color);
    /* Depth test fails here */
    ret_color = get_surface_color(surface, 500, 400);
    ok(compare_color(ret_color, 0x000000ff, 1), "Got unexpected color 0x%08x.\n", ret_color);

    /* Check what happens if we release the depth surface that d3drm created, and clear the viewport */
    hr = IDirectDrawSurface_DeleteAttachedSurface(surface, 0, ds);
    ok(SUCCEEDED(hr), "Cannot delete attached surface (hr = %#x).\n", hr);
    d3drm_ds = (IDirectDrawSurface *)0xdeadbeef;
    hr = IDirectDrawSurface_GetAttachedSurface(surface, &caps, &d3drm_ds);
    ok(hr == DDERR_NOTFOUND, "Expected hr == DDERR_NOTFOUND, got %#x.\n", hr);
    ok(d3drm_ds == NULL, "Expected NULL z-surface, got %p.\n", d3drm_ds);

    clear_depth_surface(ds, 0x7fff);
    hr = IDirect3DRMViewport_Configure(viewport1, 0, 0, rc.right, rc.bottom);
    todo_wine ok(SUCCEEDED(hr), "Cannot configure viewport (hr = %#x).\n", hr);
    hr = IDirect3DRMViewport_Clear(viewport1);
    ok(SUCCEEDED(hr), "Cannot clear viewport (hr = %#x).\n", hr);

    ret_color = get_surface_color(surface, 100, 200);
    ok(compare_color(ret_color, 0x000000ff, 1), "Got unexpected color 0x%08x.\n", ret_color);

    hr = IDirectDrawSurface_AddAttachedSurface(surface, ds);
    ok(SUCCEEDED(hr), "Failed to attach depth buffer, hr %#x.\n", hr);
    IDirectDrawSurface_Release(ds);

    d3d_draw_quad1(d3d_device1, d3d_viewport);

    ret_color = get_surface_color(surface, 100, 200);
    ok(compare_color(ret_color, 0x00bada55, 1), "Got unexpected color 0x%08x.\n", ret_color);
    ret_color = get_surface_color(surface, 500, 400);
    ok(compare_color(ret_color, 0x000000ff, 1), "Got unexpected color 0x%08x.\n", ret_color);

    IDirect3DViewport_Release(d3d_viewport);
    IDirectDrawSurface_Release(surface);
    IDirect3DDevice_Release(d3d_device1);
    IDirect3DRMViewport_Release(viewport1);
    IDirect3DRMFrame_Release(frame1);
    IDirect3DRMFrame_Release(camera1);
    IDirect3DRMDevice_Release(device1);
    IDirect3DRM_Release(d3drm1);
    IDirectDrawClipper_Release(clipper);
    IDirectDraw_Release(ddraw);
    DestroyWindow(window);
}

static void draw_quad2(IDirect3DDevice2 *device, IDirect3DViewport *viewport)
{
    static D3DLVERTEX tquad[] =
    {
        {{-1.0f}, {-1.0f}, {0.0f}, 0, {0xffbada55}, {0}, {0.0f}, {0.0f}},
        {{-1.0f}, { 1.0f}, {0.0f}, 0, {0xffbada55}, {0}, {0.0f}, {1.0f}},
        {{ 1.0f}, {-1.0f}, {1.0f}, 0, {0xffbada55}, {0}, {1.0f}, {0.0f}},
        {{ 1.0f}, { 1.0f}, {1.0f}, 0, {0xffbada55}, {0}, {1.0f}, {1.0f}},
    };
    static D3DMATRIX mat =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    IDirect3DViewport2 *viewport2;
    HRESULT hr;

    hr = IDirect3DDevice2_SetTransform(device, D3DTRANSFORMSTATE_WORLD, &mat);
    ok(SUCCEEDED(hr), "Failed to set world transform, hr %#x.\n", hr);
    hr = IDirect3DDevice2_SetTransform(device, D3DTRANSFORMSTATE_VIEW, &mat);
    ok(SUCCEEDED(hr), "Failed to set view transform, hr %#x.\n", hr);
    hr = IDirect3DDevice2_SetTransform(device, D3DTRANSFORMSTATE_PROJECTION, &mat);
    ok(SUCCEEDED(hr), "Failed to set projection transform, hr %#x.\n", hr);

    hr = IDirect3DViewport_QueryInterface(viewport, &IID_IDirect3DViewport2, (void **)&viewport2);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DViewport2 interface (hr = %#x).\n", hr);
    hr = IDirect3DDevice2_SetCurrentViewport(device, viewport2);
    ok(SUCCEEDED(hr), "Failed to activate the viewport, hr %#x.\n", hr);
    IDirect3DViewport2_Release(viewport2);

    hr = IDirect3DDevice2_SetRenderState(device, D3DRENDERSTATE_ZENABLE, D3DZB_TRUE);
    ok(SUCCEEDED(hr), "Failed to enable z testing, hr %#x.\n", hr);
    hr = IDirect3DDevice2_SetRenderState(device, D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL);
    ok(SUCCEEDED(hr), "Failed to set the z function, hr %#x.\n", hr);

    hr = IDirect3DDevice2_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice2_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DVT_LVERTEX, tquad, 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice2_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);
}

static void test_viewport_clear2(void)
{
    DDSCAPS caps = { DDSCAPS_ZBUFFER };
    IDirect3D2 *d3d2;
    IDirectDraw *ddraw1;
    IDirectDraw2 *ddraw2;
    IDirectDrawClipper *clipper;
    IDirect3DRM *d3drm1;
    IDirect3DRM3 *d3drm3;
    IDirect3DRMFrame3 *frame3, *camera3;
    IDirect3DRMDevice3 *device3;
    IDirect3DViewport *d3d_viewport;
    IDirect3DRMViewport2 *viewport2;
    IDirect3DDevice2 *d3d_device2;
    IDirectDrawSurface *surface, *ds, *d3drm_ds;
    HWND window;
    GUID driver = IID_IDirect3DRGBDevice;
    HRESULT hr;
    D3DCOLOR ret_color;
    RECT rc;

    window = create_window();
    GetClientRect(window, &rc);

    hr = DirectDrawCreate(NULL, &ddraw1, NULL);
    ok(SUCCEEDED(hr), "Cannot create IDirectDraw interface (hr = %#x).\n", hr);

    hr = IDirectDraw_SetCooperativeLevel(ddraw1, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level (hr = %#x).\n", hr);

    hr = IDirectDraw_CreateClipper(ddraw1, 0, &clipper, NULL);
    ok(SUCCEEDED(hr), "Cannot create clipper (hr = %#x).\n", hr);

    hr = IDirectDrawClipper_SetHWnd(clipper, 0, window);
    ok(SUCCEEDED(hr), "Cannot set HWnd to Clipper (hr = %#x)\n", hr);

    hr = Direct3DRMCreate(&d3drm1);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRM interface (hr = %#x).\n", hr);

    hr = IDirect3DRM_QueryInterface(d3drm1, &IID_IDirect3DRM3, (void **)&d3drm3);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRM3 interface (hr = %#x).\n", hr);

    hr = IDirect3DRM3_CreateDeviceFromClipper(d3drm3, clipper, &driver, rc.right, rc.bottom, &device3);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMDevice3 interface (hr = %#x)\n", hr);

    hr = IDirect3DRM3_CreateFrame(d3drm3, NULL, &frame3);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMFrame3 interface (hr = %#x)\n", hr);
    hr = IDirect3DRM3_CreateFrame(d3drm3, frame3, &camera3);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMFrame3 interface (hr = %#x)\n", hr);

    hr = IDirect3DRM3_CreateViewport(d3drm3, device3, camera3, 0, 0, rc.right,
            rc.bottom, &viewport2);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMViewport2 interface (hr = %#x)\n", hr);

    /* Fetch immediate mode device in order to access render target and test its color. */
    hr = IDirect3DRMDevice3_GetDirect3DDevice2(device3, &d3d_device2);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DDevice2 interface (hr = %#x).\n", hr);

    hr = IDirect3DDevice2_GetRenderTarget(d3d_device2, &surface);
    ok(SUCCEEDED(hr), "Cannot get surface to the render target (hr = %#x).\n", hr);

    ret_color = get_surface_color(surface, 320, 240);
    ok(compare_color(ret_color, 0, 1), "Got unexpected color 0x%08x.\n", ret_color);

    /* Clear uses the scene frame's background color. */
    hr = IDirect3DRMFrame3_SetSceneBackgroundRGB(frame3, 1.0f, 1.0f, 1.0f);
    ok(SUCCEEDED(hr), "Cannot set scene background RGB (hr = %#x)\n", hr);
    ret_color = IDirect3DRMFrame3_GetSceneBackground(frame3);
    ok(ret_color == 0xffffffff, "Expected scene color returned == 0xffffffff, got %#x.\n", ret_color);
    hr = IDirect3DRMFrame3_SetSceneBackgroundRGB(camera3, 0.0f, 1.0f, 0.0f);
    ok(SUCCEEDED(hr), "Cannot set scene background RGB (hr = %#x)\n", hr);
    ret_color = IDirect3DRMFrame3_GetSceneBackground(camera3);
    ok(ret_color == 0xff00ff00, "Expected scene color returned == 0xff00ff00, got %#x.\n", ret_color);

    CHECK_REFCOUNT(frame3, 1);
    hr = IDirect3DRMViewport2_Clear(viewport2, D3DRMCLEAR_ALL);
    ok(SUCCEEDED(hr), "Cannot clear viewport (hr = %#x).\n", hr);
    ret_color = get_surface_color(surface, 320, 240);
    ok(compare_color(ret_color, 0x00ffffff, 1), "Got unexpected color 0x%08x.\n", ret_color);
    CHECK_REFCOUNT(frame3, 1);

    hr = IDirect3DRMViewport2_GetDirect3DViewport(viewport2, &d3d_viewport);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DViewport interface (hr = %#x).\n", hr);

    hr = IDirect3DRMViewport2_Clear(viewport2, D3DRMCLEAR_ALL);
    ok(SUCCEEDED(hr), "Cannot clear viewport (hr = %#x).\n", hr);

    /* d3drm seems to be calling BeginScene when Clear is called. */
    hr = IDirect3DDevice2_BeginScene(d3d_device2);
    todo_wine ok(hr == D3DERR_SCENE_IN_SCENE, "Expected hr == D3DERR_SCENE_IN_SCENE, got %#x.\n", hr);
    hr = IDirect3DDevice2_EndScene(d3d_device2);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    ret_color = get_surface_color(surface, 320, 240);
    ok(compare_color(ret_color, 0x00ffffff, 1), "Got unexpected color 0x%08x.\n", ret_color);

    /* We're using d3d to draw using IDirect3DDevice2 created from d3drm. */
    draw_quad2(d3d_device2, d3d_viewport);
    ret_color = get_surface_color(surface, 320, 240);
    ok(compare_color(ret_color, 0x00bada55, 1), "Got unexpected color 0x%08x.\n", ret_color);

    /* Without calling Configure, Clear doesn't work. */
    hr = IDirect3DRMViewport2_Clear(viewport2, D3DRMCLEAR_ALL);
    ok(SUCCEEDED(hr), "Cannot clear viewport (hr = %#x).\n", hr);
    ret_color = get_surface_color(surface, 320, 240);
    todo_wine ok(compare_color(ret_color, 0x00bada55, 1), "Got unexpected color 0x%08x.\n", ret_color);

    hr = IDirect3DRMViewport2_Configure(viewport2, 0, 0, rc.right, rc.bottom);
    todo_wine ok(SUCCEEDED(hr), "Cannot configure viewport (hr = %#x).\n", hr);
    hr = IDirect3DRMViewport2_Clear(viewport2, D3DRMCLEAR_ALL);
    ok(SUCCEEDED(hr), "Cannot clear viewport (hr = %#x).\n", hr);

    ret_color = get_surface_color(surface, 320, 240);
    ok(compare_color(ret_color, 0x00ffffff, 1), "Got unexpected color 0x%08x.\n", ret_color);

    /* Fetch attached depth surface and see if viewport clears it if it's detached from the render target. */
    hr = IDirectDrawSurface_GetAttachedSurface(surface, &caps, &ds);
    ok(SUCCEEDED(hr), "Cannot get attached depth surface (hr = %x).\n", hr);

    clear_depth_surface(ds, 0x39);
    draw_quad2(d3d_device2, d3d_viewport);

    ret_color = get_surface_color(surface, 320, 240);
    ok(compare_color(ret_color, 0x00ffffff, 1), "Got unexpected color 0x%08x.\n", ret_color);

    hr = IDirectDrawSurface_DeleteAttachedSurface(surface, 0, ds);
    ok(SUCCEEDED(hr), "Cannot delete attached surface (hr = %#x).\n", hr);
    d3drm_ds = (IDirectDrawSurface *)0xdeadbeef;
    hr = IDirectDrawSurface_GetAttachedSurface(surface, &caps, &d3drm_ds);
    ok(hr == DDERR_NOTFOUND, "Expected hr == DDERR_NOTFOUND, got %#x.\n", hr);
    ok(d3drm_ds == NULL, "Expected NULL z-surface, got %p.\n", d3drm_ds);

    clear_depth_surface(ds, 0x7fff);

    /* This version of Clear still clears the depth surface even if it's deleted from the render target. */
    hr = IDirect3DRMViewport2_Configure(viewport2, 0, 0, rc.right, rc.bottom);
    todo_wine ok(SUCCEEDED(hr), "Cannot configure viewport (hr = %#x).\n", hr);
    hr = IDirect3DRMViewport2_Clear(viewport2, D3DRMCLEAR_ALL);
    ok(SUCCEEDED(hr), "Cannot clear viewport (hr = %#x).\n", hr);

    hr = IDirectDrawSurface_AddAttachedSurface(surface, ds);
    ok(SUCCEEDED(hr), "Failed to attach depth buffer, hr %#x.\n", hr);
    ret_color = get_surface_color(surface, 320, 240);
    ok(compare_color(ret_color, 0x00ffffff, 1), "Got unexpected color 0x%08x.\n", ret_color);

    draw_quad2(d3d_device2, d3d_viewport);
    ret_color = get_surface_color(surface, 100, 200);
    ok(compare_color(ret_color, 0x00bada55, 1), "Got unexpected color 0x%08x.\n", ret_color);
    ret_color = get_surface_color(surface, 500, 400);
    todo_wine ok(compare_color(ret_color, 0x00bada55, 1), "Got unexpected color 0x%08x.\n", ret_color);

    /* Clear with no flags */
    hr = IDirect3DRMViewport2_Configure(viewport2, 0, 0, rc.right, rc.bottom);
    todo_wine ok(SUCCEEDED(hr), "Cannot configure viewport (hr = %#x).\n", hr);
    hr = IDirect3DRMViewport2_Clear(viewport2, 0);
    ok(SUCCEEDED(hr), "Cannot clear viewport (hr = %#x).\n", hr);
    ret_color = get_surface_color(surface, 320, 240);
    todo_wine ok(compare_color(ret_color, 0x00bada55, 1), "Got unexpected color 0x%08x.\n", ret_color);

    hr = IDirect3DRMViewport2_Configure(viewport2, 0, 0, rc.right, rc.bottom);
    todo_wine ok(SUCCEEDED(hr), "Cannot configure viewport (hr = %#x).\n", hr);
    hr = IDirect3DRMViewport2_Clear(viewport2, D3DRMCLEAR_ALL);
    ok(SUCCEEDED(hr), "Cannot clear viewport (hr = %#x).\n", hr);
    ret_color = get_surface_color(surface, 320, 240);
    ok(compare_color(ret_color, 0x00ffffff, 1), "Got unexpected color 0x%08x.\n", ret_color);

    IDirect3DViewport_Release(d3d_viewport);
    IDirectDrawSurface_Release(surface);
    IDirectDrawSurface_Release(ds);
    IDirect3DDevice2_Release(d3d_device2);
    IDirect3DRMViewport2_Release(viewport2);
    IDirect3DRMDevice3_Release(device3);

    /* Create device without depth surface attached */
    hr = IDirectDraw_QueryInterface(ddraw1, &IID_IDirectDraw2, (void **)&ddraw2);
    ok(SUCCEEDED(hr), "Cannot get IDirectDraw2 interface (hr = %#x).\n", hr);
    hr = IDirectDraw_QueryInterface(ddraw1, &IID_IDirect3D2, (void **)&d3d2);
    ok(SUCCEEDED(hr), "Cannot get IDirect3D2 interface (hr = %x).\n", hr);
    d3d_device2 = create_device2_without_ds(ddraw2, window);
    if (!d3d_device2)
        goto cleanup;

    hr = IDirect3DRM3_CreateDeviceFromD3D(d3drm3, d3d2, d3d_device2, &device3);
    ok(SUCCEEDED(hr), "Failed to create IDirect3DRMDevice interface (hr = %#x)\n", hr);
    hr = IDirect3DRM3_CreateViewport(d3drm3, device3, camera3, 0, 0, rc.right,
            rc.bottom, &viewport2);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMViewport2 interface (hr = %#x)\n", hr);
    hr = IDirect3DDevice2_GetRenderTarget(d3d_device2, &surface);
    ok(SUCCEEDED(hr), "Cannot get surface to the render target (hr = %#x).\n", hr);

    hr = IDirect3DRMViewport2_Clear(viewport2, D3DRMCLEAR_ALL);
    ok(SUCCEEDED(hr), "Cannot clear viewport (hr = %#x).\n", hr);
    ret_color = get_surface_color(surface, 320, 240);
    ok(compare_color(ret_color, 0x00ffffff, 1), "Got unexpected color 0x%08x.\n", ret_color);

    hr = IDirect3DRMViewport2_Clear(viewport2, D3DRMCLEAR_ZBUFFER);
    ok(SUCCEEDED(hr), "Cannot clear viewport (hr = %#x).\n", hr);

    IDirectDrawSurface_Release(surface);
    IDirect3DRMViewport2_Release(viewport2);
    IDirect3DRMDevice3_Release(device3);
    IDirect3DDevice2_Release(d3d_device2);

cleanup:
    IDirect3DRMFrame3_Release(camera3);
    IDirect3DRMFrame3_Release(frame3);
    IDirect3DRM3_Release(d3drm3);
    IDirect3DRM_Release(d3drm1);
    IDirectDrawClipper_Release(clipper);
    IDirect3D2_Release(d3d2);
    IDirectDraw2_Release(ddraw2);
    IDirectDraw_Release(ddraw1);
    DestroyWindow(window);
}

static void test_create_texture_from_surface(void)
{
    D3DRMIMAGE testimg =
    {
        0, 0, 0, 0, 0,
        TRUE, 0, (void *)0xcafebabe, NULL,
        0x000000ff, 0x0000ff00, 0x00ff0000, 0, 0, NULL
    };
    IDirectDrawSurface *surface = NULL, *surface2 = NULL, *ds = NULL;
    IDirect3DRMTexture *texture1;
    IDirect3DRMTexture2 *texture2;
    IDirect3DRMTexture3 *texture3;
    IDirectDraw *ddraw = NULL;
    IDirect3DRM *d3drm1 = NULL;
    IDirect3DRM2 *d3drm2 = NULL;
    IDirect3DRM3 *d3drm3 = NULL;
    ULONG ref1, ref2, ref3;
    D3DRMIMAGE *image;
    DDSURFACEDESC desc;
    HWND window;
    HRESULT hr;
    RECT rc;

    hr = DirectDrawCreate(NULL, &ddraw, NULL);
    ok(hr == DD_OK, "Cannot get IDirectDraw interface (hr = %x).\n", hr);

    window = create_window();
    GetClientRect(window, &rc);

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    hr = Direct3DRMCreate(&d3drm1);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x).\n", hr);

    hr = IDirect3DRM_QueryInterface(d3drm1, &IID_IDirect3DRM2, (void **)&d3drm2);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRM2 interface (hr = %x).\n", hr);

    hr = IDirect3DRM_QueryInterface(d3drm1, &IID_IDirect3DRM3, (void **)&d3drm3);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRM3 interface (hr = %x).\n", hr);

    /* Create a surface and use it to create a texture. */
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    desc.dwWidth = rc.right;
    desc.dwHeight = rc.bottom;

    hr = IDirectDraw_CreateSurface(ddraw, &desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDraw_CreateSurface(ddraw, &desc, &surface2, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    /* Test NULL params */
    texture1 = (IDirect3DRMTexture *)0xdeadbeef;
    hr = IDirect3DRM_CreateTextureFromSurface(d3drm1, NULL, &texture1);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    ok(!texture1, "Expected texture returned == NULL, got %p.\n", texture1);

    hr = IDirect3DRM_CreateTextureFromSurface(d3drm1, NULL, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);

    texture2 = (IDirect3DRMTexture2 *)0xdeadbeef;
    hr = IDirect3DRM2_CreateTextureFromSurface(d3drm2, NULL, &texture2);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    ok(!texture2, "Expected texture returned == NULL, got %p.\n", texture2);

    hr = IDirect3DRM2_CreateTextureFromSurface(d3drm2, NULL, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);

    texture3 = (IDirect3DRMTexture3 *)0xdeadbeef;
    hr = IDirect3DRM3_CreateTextureFromSurface(d3drm3, NULL, &texture3);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    ok(!texture3, "Expected texture returned == NULL, got %p.\n", texture3);

    hr = IDirect3DRM3_CreateTextureFromSurface(d3drm3, NULL, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);

    ok(get_refcount((IUnknown *)surface) == 1, "Unexpected surface refcount.\n");
    hr = IDirect3DRM_CreateTextureFromSurface(d3drm1, surface, &texture1);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);

    ok(get_refcount((IUnknown *)surface) == 2, "Unexpected surface refcount.\n");
    image = IDirect3DRMTexture_GetImage(texture1);
    ok(image == NULL, "Unexpected image, %p.\n", image);
    hr = IDirect3DRMTexture_InitFromSurface(texture1, NULL);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    IDirect3DRMTexture_Release(texture1);

    ok(get_refcount((IUnknown *)surface) == 1, "Unexpected surface refcount.\n");
    hr = IDirect3DRM2_CreateTextureFromSurface(d3drm2, surface, &texture2);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    ok(get_refcount((IUnknown *)surface) == 2, "Unexpected surface refcount.\n");
    image = IDirect3DRMTexture2_GetImage(texture2);
    ok(image == NULL, "Unexpected image, %p.\n", image);
    hr = IDirect3DRMTexture2_InitFromSurface(texture2, NULL);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    IDirect3DRMTexture_Release(texture2);

    ok(get_refcount((IUnknown *)surface) == 1, "Unexpected surface refcount.\n");
    hr = IDirect3DRM3_CreateTextureFromSurface(d3drm3, surface, &texture3);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    ok(get_refcount((IUnknown *)surface) == 2, "Unexpected surface refcount.\n");
    image = IDirect3DRMTexture3_GetImage(texture3);
    ok(image == NULL, "Unexpected image, %p.\n", image);
    hr = IDirect3DRMTexture3_InitFromSurface(texture3, NULL);
    ok(hr == D3DRMERR_BADOBJECT, "Expected hr == D3DRMERR_BADOBJECT, got %#x.\n", hr);
    hr = IDirect3DRMTexture3_GetSurface(texture3, 0, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %#x.\n", hr);
    hr = IDirect3DRMTexture3_GetSurface(texture3, 0, &ds);
    ok(SUCCEEDED(hr), "Failed to get surface, hr %#x.\n", hr);
    ok(ds == surface, "Expected same surface back.\n");
    IDirectDrawSurface_Release(ds);

    /* Init already initialized texture with same surface. */
    hr = IDirect3DRMTexture3_InitFromSurface(texture3, surface);
    ok(hr == D3DRMERR_BADOBJECT, "Expected a failure, hr %#x.\n", hr);

    /* Init already initialized texture with different surface. */
    hr = IDirect3DRMTexture3_InitFromSurface(texture3, surface2);
    ok(hr == D3DRMERR_BADOBJECT, "Expected a failure, hr %#x.\n", hr);

    hr = IDirect3DRMTexture3_GetSurface(texture3, 0, &ds);
    ok(SUCCEEDED(hr), "Failed to get surface, hr %#x.\n", hr);
    ok(ds == surface, "Expected same surface back.\n");
    IDirectDrawSurface_Release(ds);

    ref1 = get_refcount((IUnknown *)d3drm1);
    ref2 = get_refcount((IUnknown *)d3drm2);
    ref3 = get_refcount((IUnknown *)d3drm3);
    hr = IDirect3DRMTexture3_InitFromImage(texture3, &testimg);
    ok(hr == D3DRMERR_BADOBJECT, "Expected a failure, hr %#x.\n", hr);
    ok(ref1 < get_refcount((IUnknown *)d3drm1), "Expected d3drm1 reference taken.\n");
    ok(ref2 == get_refcount((IUnknown *)d3drm2), "Expected d3drm2 reference unchanged.\n");
    ok(ref3 == get_refcount((IUnknown *)d3drm3), "Expected d3drm3 reference unchanged.\n");
    /* Release leaked reference to d3drm1 */
    IDirect3DRM_Release(d3drm1);

    IDirect3DRMTexture_Release(texture3);

    /* Create from image, initialize from surface. */
    hr = IDirect3DRM3_CreateTexture(d3drm3, &testimg, &texture3);
    ok(SUCCEEDED(hr), "Cannot get IDirect3DRMTexture3 interface (hr = %#x)\n", hr);

    ref1 = get_refcount((IUnknown *)d3drm1);
    ref2 = get_refcount((IUnknown *)d3drm2);
    ref3 = get_refcount((IUnknown *)d3drm3);
    hr = IDirect3DRMTexture3_InitFromSurface(texture3, surface);
    ok(hr == D3DRMERR_BADOBJECT, "Expected a failure, hr %#x.\n", hr);
    ok(ref1 < get_refcount((IUnknown *)d3drm1), "Expected d3drm1 reference taken.\n");
    ok(ref2 == get_refcount((IUnknown *)d3drm2), "Expected d3drm2 reference unchanged.\n");
    ok(ref3 == get_refcount((IUnknown *)d3drm3), "Expected d3drm3 reference unchanged.\n");
    /* Release leaked reference to d3drm1 */
    IDirect3DRM_Release(d3drm1);
    IDirect3DRMTexture3_Release(texture3);

    IDirectDrawSurface_Release(surface2);
    IDirectDrawSurface_Release(surface);
    IDirect3DRM3_Release(d3drm3);
    IDirect3DRM2_Release(d3drm2);
    IDirect3DRM_Release(d3drm1);
    IDirectDraw_Release(ddraw);
}

static void test_animation(void)
{
    IDirect3DRMAnimation2 *animation2;
    IDirect3DRMAnimation *animation;
    D3DRMANIMATIONOPTIONS options;
    IDirect3DRMObject *obj, *obj2;
    D3DRMANIMATIONKEY keys[10];
    IDirect3DRMFrame3 *frame3;
    IDirect3DRMFrame *frame;
    D3DRMANIMATIONKEY key;
    IDirect3DRM *d3drm1;
    D3DRMQUATERNION q;
    DWORD count, i;
    HRESULT hr;
    D3DVECTOR v;

    hr = Direct3DRMCreate(&d3drm1);
    ok(SUCCEEDED(hr), "Failed to create IDirect3DRM instance, hr 0x%08x.\n", hr);

    hr = IDirect3DRM_CreateAnimation(d3drm1, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Unexpected hr 0x%08x.\n", hr);

    CHECK_REFCOUNT(d3drm1, 1);
    hr = IDirect3DRM_CreateAnimation(d3drm1, &animation);
    ok(SUCCEEDED(hr), "Failed to create animation hr 0x%08x.\n", hr);
    CHECK_REFCOUNT(d3drm1, 2);

    test_class_name((IDirect3DRMObject *)animation, "Animation");

    hr = IDirect3DRMAnimation_QueryInterface(animation, &IID_IDirect3DRMAnimation2, (void **)&animation2);
    ok(SUCCEEDED(hr), "Failed to get IDirect3DRMAnimation2, hr 0x%08x.\n", hr);
    ok(animation != (void *)animation2, "Expected different interface pointer.\n");

    hr = IDirect3DRMAnimation_QueryInterface(animation, &IID_IDirect3DRMObject, (void **)&obj);
    ok(SUCCEEDED(hr), "Failed to get IDirect3DRMObject, hr 0x%08x.\n", hr);

    hr = IDirect3DRMAnimation2_QueryInterface(animation2, &IID_IDirect3DRMObject, (void **)&obj2);
    ok(SUCCEEDED(hr), "Failed to get IDirect3DRMObject, hr 0x%08x.\n", hr);

    ok(obj == obj2 && obj == (IDirect3DRMObject *)animation, "Unexpected object pointer.\n");

    IDirect3DRMObject_Release(obj);
    IDirect3DRMObject_Release(obj2);

    /* Set animated frame, get it back. */
    hr = IDirect3DRM_CreateFrame(d3drm1, NULL, &frame);
    ok(SUCCEEDED(hr), "Failed to create a frame, hr %#x.\n", hr);

    hr = IDirect3DRMAnimation_SetFrame(animation, NULL);
    ok(SUCCEEDED(hr), "Failed to reset frame, hr %#x.\n", hr);

    CHECK_REFCOUNT(frame, 1);
    hr = IDirect3DRMAnimation_SetFrame(animation, frame);
    ok(SUCCEEDED(hr), "Failed to set a frame, hr %#x.\n", hr);
    CHECK_REFCOUNT(frame, 1);

    hr = IDirect3DRMAnimation2_GetFrame(animation2, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DRMAnimation2_GetFrame(animation2, &frame3);
    ok(SUCCEEDED(hr), "Failed to get the frame, %#x.\n", hr);
    ok(frame3 != (void *)frame, "Unexpected interface pointer.\n");
    CHECK_REFCOUNT(frame, 2);

    IDirect3DRMFrame3_Release(frame3);

    hr = IDirect3DRMAnimation_SetFrame(animation, NULL);
    ok(SUCCEEDED(hr), "Failed to reset frame, hr %#x.\n", hr);

    hr = IDirect3DRMFrame_QueryInterface(frame, &IID_IDirect3DRMFrame3, (void **)&frame3);
    ok(SUCCEEDED(hr), "Failed to get IDirect3DRMFrame3, hr %#x.\n", hr);

    CHECK_REFCOUNT(frame3, 2);
    hr = IDirect3DRMAnimation2_SetFrame(animation2, frame3);
    ok(SUCCEEDED(hr), "Failed to set a frame, hr %#x.\n", hr);
    CHECK_REFCOUNT(frame3, 2);

    IDirect3DRMFrame3_Release(frame3);
    IDirect3DRMFrame_Release(frame);

    /* Animation options. */
    options = IDirect3DRMAnimation_GetOptions(animation);
    ok(options == (D3DRMANIMATION_CLOSED | D3DRMANIMATION_LINEARPOSITION),
            "Unexpected default options %#x.\n", options);

    /* Undefined mask value */
    hr = IDirect3DRMAnimation_SetOptions(animation, 0xf0000000);
    ok(hr == D3DRMERR_BADVALUE, "Unexpected hr %#x.\n", hr);

    options = IDirect3DRMAnimation_GetOptions(animation);
    ok(options == (D3DRMANIMATION_CLOSED | D3DRMANIMATION_LINEARPOSITION),
            "Unexpected default options %#x.\n", options);

    /* Ambiguous mask */
    hr = IDirect3DRMAnimation_SetOptions(animation, D3DRMANIMATION_OPEN | D3DRMANIMATION_CLOSED);
    ok(hr == D3DRMERR_BADVALUE, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DRMAnimation_SetOptions(animation, D3DRMANIMATION_LINEARPOSITION | D3DRMANIMATION_SPLINEPOSITION);
    ok(hr == D3DRMERR_BADVALUE, "Unexpected hr %#x.\n", hr);

    hr = IDirect3DRMAnimation_SetOptions(animation, D3DRMANIMATION_SCALEANDROTATION | D3DRMANIMATION_POSITION);
    ok(hr == D3DRMERR_BADVALUE, "Unexpected hr %#x.\n", hr);

    options = IDirect3DRMAnimation_GetOptions(animation);
    ok(options == (D3DRMANIMATION_CLOSED | D3DRMANIMATION_LINEARPOSITION),
            "Unexpected default options %#x.\n", options);

    /* Mask contains undefined bits together with valid one. */
    hr = IDirect3DRMAnimation_SetOptions(animation, 0xf0000000 | D3DRMANIMATION_OPEN);
    ok(SUCCEEDED(hr), "Failed to set animation options, hr %#x.\n", hr);

    options = IDirect3DRMAnimation_GetOptions(animation);
    ok(options == (0xf0000000 | D3DRMANIMATION_OPEN), "Unexpected animation options %#x.\n", options);

    hr = IDirect3DRMAnimation_SetOptions(animation, D3DRMANIMATION_SCALEANDROTATION);
    ok(SUCCEEDED(hr), "Failed to set animation options, hr %#x.\n", hr);

    options = IDirect3DRMAnimation_GetOptions(animation);
    ok(options == D3DRMANIMATION_SCALEANDROTATION, "Unexpected options %#x.\n", options);

    hr = IDirect3DRMAnimation_SetOptions(animation, D3DRMANIMATION_OPEN);
    ok(SUCCEEDED(hr), "Failed to set animation options, hr %#x.\n", hr);

    options = IDirect3DRMAnimation_GetOptions(animation);
    ok(options == D3DRMANIMATION_OPEN, "Unexpected options %#x.\n", options);

    hr = IDirect3DRMAnimation_SetOptions(animation, 0);
    ok(hr == D3DRMERR_BADVALUE, "Unexpected hr %#x.\n", hr);

    options = IDirect3DRMAnimation_GetOptions(animation);
    ok(options == D3DRMANIMATION_OPEN, "Unexpected options %#x.\n", options);

    /* Key management. */
    hr = IDirect3DRMAnimation_AddPositionKey(animation, 0.0f, 1.0f, 0.0f, 0.0f);
    ok(SUCCEEDED(hr), "Failed to add position key, hr %#x.\n", hr);

    hr = IDirect3DRMAnimation_AddScaleKey(animation, 0.0f, 1.0f, 2.0f, 1.0f);
    ok(SUCCEEDED(hr), "Failed to add scale key, hr %#x.\n", hr);

    hr = IDirect3DRMAnimation_AddPositionKey(animation, 0.0f, 2.0f, 0.0f, 0.0f);
    ok(SUCCEEDED(hr), "Failed to add position key, hr %#x.\n", hr);

    hr = IDirect3DRMAnimation_AddPositionKey(animation, 99.0f, 3.0f, 1.0f, 0.0f);
    ok(SUCCEEDED(hr), "Failed to add position key, hr %#x.\n", hr);

    hr = IDirect3DRMAnimation_AddPositionKey(animation, 80.0f, 4.0f, 1.0f, 0.0f);
    ok(SUCCEEDED(hr), "Failed to add position key, hr %#x.\n", hr);

    v.x = 1.0f;
    v.y = 0.0f;
    v.z = 0.0f;
    D3DRMQuaternionFromRotation(&q, &v, 1.0f);

    /* NULL quaternion pointer leads to a crash on Windows. */
    hr = IDirect3DRMAnimation_AddRotateKey(animation, 0.0f, &q);
    ok(SUCCEEDED(hr), "Failed to add rotation key, hr %#.x\n", hr);

    count = 0;
    memset(keys, 0, sizeof(keys));
    hr = IDirect3DRMAnimation2_GetKeys(animation2, 0.0f, 99.0f, &count, keys);
    ok(SUCCEEDED(hr), "Failed to get animation keys, hr %#x.\n", hr);
    ok(count == 6, "Unexpected key count %u.\n", count);

    ok(keys[0].dwKeyType == D3DRMANIMATION_ROTATEKEY, "Unexpected key type %u.\n", keys[0].dwKeyType);
    ok(keys[1].dwKeyType == D3DRMANIMATION_POSITIONKEY, "Unexpected key type %u.\n", keys[1].dwKeyType);
    ok(keys[2].dwKeyType == D3DRMANIMATION_POSITIONKEY, "Unexpected key type %u.\n", keys[2].dwKeyType);
    ok(keys[3].dwKeyType == D3DRMANIMATION_POSITIONKEY, "Unexpected key type %u.\n", keys[3].dwKeyType);
    ok(keys[4].dwKeyType == D3DRMANIMATION_POSITIONKEY, "Unexpected key type %u.\n", keys[4].dwKeyType);
    ok(keys[5].dwKeyType == D3DRMANIMATION_SCALEKEY, "Unexpected key type %u.\n", keys[5].dwKeyType);

    /* Relative order, keys are returned sorted by time. */
    ok(keys[1].dvTime == 0.0f, "Unexpected key time %.8e.\n", keys[1].dvTime);
    ok(keys[2].dvTime == 0.0f, "Unexpected key time %.8e.\n", keys[2].dvTime);
    ok(keys[3].dvTime == 80.0f, "Unexpected key time %.8e.\n", keys[3].dvTime);
    ok(keys[4].dvTime == 99.0f, "Unexpected key time %.8e.\n", keys[4].dvTime);

    /* For keys with same time, order they were added in is kept. */
    ok(keys[1].dvPositionKey.x == 1.0f, "Unexpected key position x %.8e.\n", keys[1].dvPositionKey.x);
    ok(keys[2].dvPositionKey.x == 2.0f, "Unexpected key position x %.8e.\n", keys[2].dvPositionKey.x);
    ok(keys[3].dvPositionKey.x == 4.0f, "Unexpected key position x %.8e.\n", keys[3].dvPositionKey.x);
    ok(keys[4].dvPositionKey.x == 3.0f, "Unexpected key position x %.8e.\n", keys[4].dvPositionKey.x);

    for (i = 0; i < count; i++)
    {
        ok(keys[i].dwSize == sizeof(*keys), "%u: unexpected dwSize value %u.\n", i, keys[i].dwSize);

    todo_wine
    {
        switch (keys[i].dwKeyType)
        {
        case D3DRMANIMATION_ROTATEKEY:
            ok((keys[i].dwID & 0xf0000000) == 0x40000000, "%u: unexpected id mask %#x.\n", i, keys[i].dwID);
            break;
        case D3DRMANIMATION_POSITIONKEY:
            ok((keys[i].dwID & 0xf0000000) == 0x80000000, "%u: unexpected id mask %#x.\n", i, keys[i].dwID);
            break;
        case D3DRMANIMATION_SCALEKEY:
            ok((keys[i].dwID & 0xf0000000) == 0xc0000000, "%u: unexpected id mask %#x.\n", i, keys[i].dwID);
            break;
        default:
            ok(0, "%u: unknown key type %d.\n", i, keys[i].dwKeyType);
        }
    }
    }

    /* No keys in this range. */
    count = 10;
    hr = IDirect3DRMAnimation2_GetKeys(animation2, 100.0f, 200.0f, &count, NULL);
    ok(hr == D3DRMERR_NOSUCHKEY, "Unexpected hr %#x.\n", hr);
    ok(count == 0, "Unexpected key count %u.\n", count);

    count = 10;
    hr = IDirect3DRMAnimation2_GetKeys(animation2, 100.0f, 200.0f, &count, keys);
    ok(hr == D3DRMERR_NOSUCHKEY, "Unexpected hr %#x.\n", hr);
    ok(count == 0, "Unexpected key count %u.\n", count);

    count = 10;
    hr = IDirect3DRMAnimation2_GetKeys(animation2, 0.0f, 0.0f, &count, NULL);
    ok(SUCCEEDED(hr), "Failed to get animation keys, hr %#x.\n", hr);
    ok(count == 4, "Unexpected key count %u.\n", count);

    hr = IDirect3DRMAnimation2_GetKeys(animation2, 0.0f, 100.0f, NULL, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Unexpected hr %#x.\n", hr);

    /* Time is 0-based. */
    hr = IDirect3DRMAnimation2_GetKeys(animation2, -100.0f, -50.0f, NULL, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Unexpected hr %#x.\n", hr);

    count = 10;
    hr = IDirect3DRMAnimation2_GetKeys(animation2, -100.0f, -50.0f, &count, NULL);
    ok(hr == D3DRMERR_NOSUCHKEY, "Unexpected hr %#x.\n", hr);
    ok(count == 0, "Unexpected key count %u.\n", count);

    count = 10;
    hr = IDirect3DRMAnimation2_GetKeys(animation2, -100.0f, 100.0f, &count, NULL);
    ok(SUCCEEDED(hr), "Failed to get animation keys, hr %#x.\n", hr);
    ok(count == 6, "Unexpected key count %u.\n", count);

    /* AddKey() tests. */
    hr = IDirect3DRMAnimation2_AddKey(animation2, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    memset(&key, 0, sizeof(key));
    key.dwKeyType = D3DRMANIMATION_POSITIONKEY;
    hr = IDirect3DRMAnimation2_AddKey(animation2, &key);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    memset(&key, 0, sizeof(key));
    key.dwSize = sizeof(key) - 1;
    key.dwKeyType = D3DRMANIMATION_POSITIONKEY;
    hr = IDirect3DRMAnimation2_AddKey(animation2, &key);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    memset(&key, 0, sizeof(key));
    key.dwSize = sizeof(key) + 1;
    key.dwKeyType = D3DRMANIMATION_POSITIONKEY;
    hr = IDirect3DRMAnimation2_AddKey(animation2, &key);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    memset(&key, 0, sizeof(key));
    key.dwSize = sizeof(key);
    key.dwKeyType = D3DRMANIMATION_POSITIONKEY;
    key.dvPositionKey.x = 8.0f;
    hr = IDirect3DRMAnimation2_AddKey(animation2, &key);
    ok(SUCCEEDED(hr), "Failed to add key, hr %#x.\n", hr);

    /* Delete tests. */
    hr = IDirect3DRMAnimation_AddRotateKey(animation, 0.0f, &q);
    ok(SUCCEEDED(hr), "Failed to add rotation key, hr %#.x\n", hr);

    hr = IDirect3DRMAnimation_AddScaleKey(animation, 0.0f, 1.0f, 2.0f, 1.0f);
    ok(SUCCEEDED(hr), "Failed to add scale key, hr %#x.\n", hr);

    count = 0;
    memset(keys, 0, sizeof(keys));
    hr = IDirect3DRMAnimation2_GetKeys(animation2, -1000.0f, 1000.0f, &count, keys);
    ok(SUCCEEDED(hr), "Failed to get key count, hr %#x.\n", hr);
    ok(count == 9, "Unexpected key count %u.\n", count);

    ok(keys[0].dwKeyType == D3DRMANIMATION_ROTATEKEY, "Unexpected key type %u.\n", keys[0].dwKeyType);
    ok(keys[1].dwKeyType == D3DRMANIMATION_ROTATEKEY, "Unexpected key type %u.\n", keys[1].dwKeyType);
    ok(keys[2].dwKeyType == D3DRMANIMATION_POSITIONKEY, "Unexpected key type %u.\n", keys[2].dwKeyType);
    ok(keys[3].dwKeyType == D3DRMANIMATION_POSITIONKEY, "Unexpected key type %u.\n", keys[3].dwKeyType);
    ok(keys[4].dwKeyType == D3DRMANIMATION_POSITIONKEY, "Unexpected key type %u.\n", keys[4].dwKeyType);
    ok(keys[5].dwKeyType == D3DRMANIMATION_POSITIONKEY, "Unexpected key type %u.\n", keys[5].dwKeyType);
    ok(keys[6].dwKeyType == D3DRMANIMATION_POSITIONKEY, "Unexpected key type %u.\n", keys[6].dwKeyType);
    ok(keys[7].dwKeyType == D3DRMANIMATION_SCALEKEY, "Unexpected key type %u.\n", keys[7].dwKeyType);
    ok(keys[8].dwKeyType == D3DRMANIMATION_SCALEKEY, "Unexpected key type %u.\n", keys[8].dwKeyType);

    ok(keys[0].dvTime == 0.0f, "Unexpected key time %.8e.\n", keys[0].dvTime);
    ok(keys[1].dvTime == 0.0f, "Unexpected key time %.8e.\n", keys[1].dvTime);
    ok(keys[2].dvTime == 0.0f, "Unexpected key time %.8e.\n", keys[2].dvTime);
    ok(keys[3].dvTime == 0.0f, "Unexpected key time %.8e.\n", keys[3].dvTime);
    ok(keys[4].dvTime == 0.0f, "Unexpected key time %.8e.\n", keys[4].dvTime);
    ok(keys[5].dvTime == 80.0f, "Unexpected key time %.8e.\n", keys[5].dvTime);
    ok(keys[6].dvTime == 99.0f, "Unexpected key time %.8e.\n", keys[6].dvTime);
    ok(keys[7].dvTime == 0.0f, "Unexpected key time %.8e.\n", keys[7].dvTime);
    ok(keys[8].dvTime == 0.0f, "Unexpected key time %.8e.\n", keys[8].dvTime);

    hr = IDirect3DRMAnimation_DeleteKey(animation, -100.0f);
    ok(SUCCEEDED(hr), "Failed to delete keys, hr %#x.\n", hr);

    hr = IDirect3DRMAnimation_DeleteKey(animation, 100.0f);
    ok(SUCCEEDED(hr), "Failed to delete keys, hr %#x.\n", hr);

    /* Only first Position keys are not removed. */
    hr = IDirect3DRMAnimation_DeleteKey(animation, 0.0f);
    ok(SUCCEEDED(hr), "Failed to delete keys, hr %#x.\n", hr);

    count = 0;
    memset(keys, 0, sizeof(keys));
    hr = IDirect3DRMAnimation2_GetKeys(animation2, 0.0f, 100.0f, &count, keys);
    ok(SUCCEEDED(hr), "Failed to get key count, hr %#x.\n", hr);
    ok(count == 6, "Unexpected key count %u.\n", count);

    ok(keys[0].dwKeyType == D3DRMANIMATION_ROTATEKEY, "Unexpected key type %u.\n", keys[0].dwKeyType);
    ok(keys[1].dwKeyType == D3DRMANIMATION_POSITIONKEY, "Unexpected key type %u.\n", keys[1].dwKeyType);
    ok(keys[2].dwKeyType == D3DRMANIMATION_POSITIONKEY, "Unexpected key type %u.\n", keys[2].dwKeyType);
    ok(keys[3].dwKeyType == D3DRMANIMATION_POSITIONKEY, "Unexpected key type %u.\n", keys[3].dwKeyType);
    ok(keys[4].dwKeyType == D3DRMANIMATION_POSITIONKEY, "Unexpected key type %u.\n", keys[4].dwKeyType);
    ok(keys[5].dwKeyType == D3DRMANIMATION_SCALEKEY, "Unexpected key type %u.\n", keys[5].dwKeyType);

    ok(keys[0].dvTime == 0.0f, "Unexpected key time %.8e.\n", keys[0].dvTime);
    ok(keys[1].dvTime == 0.0f, "Unexpected key time %.8e.\n", keys[1].dvTime);
    ok(keys[2].dvTime == 0.0f, "Unexpected key time %.8e.\n", keys[2].dvTime);
    ok(keys[3].dvTime == 80.0f, "Unexpected key time %.8e.\n", keys[3].dvTime);
    ok(keys[4].dvTime == 99.0f, "Unexpected key time %.8e.\n", keys[4].dvTime);
    ok(keys[5].dvTime == 0.0f, "Unexpected key time %.8e.\n", keys[5].dvTime);

    hr = IDirect3DRMAnimation_DeleteKey(animation, 0.0f);
    ok(SUCCEEDED(hr), "Failed to delete keys, hr %#x.\n", hr);

    count = 0;
    hr = IDirect3DRMAnimation2_GetKeys(animation2, 0.0f, 100.0f, &count, NULL);
    ok(SUCCEEDED(hr), "Failed to get key count, hr %#x.\n", hr);
    ok(count == 3, "Unexpected key count %u.\n", count);

    IDirect3DRMAnimation2_Release(animation2);
    IDirect3DRMAnimation_Release(animation);

    IDirect3DRM_Release(d3drm1);
}

static void test_animation_qi(void)
{
    static const struct qi_test tests[] =
    {
        { &IID_IDirect3DRMAnimation2,      &IID_IUnknown, &IID_IDirect3DRMAnimation2, S_OK                      },
        { &IID_IDirect3DRMAnimation,       &IID_IUnknown, &IID_IDirect3DRMAnimation,  S_OK                      },
        { &IID_IDirect3DRM,                NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMDevice,          NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMObject,          &IID_IUnknown, &IID_IDirect3DRMAnimation,  S_OK                      },
        { &IID_IDirect3DRMDevice2,         NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMDevice3,         NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMViewport,        NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMViewport2,       NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRM3,               NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRM2,               NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMVisual,          NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMesh,            NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMeshBuilder,     NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMeshBuilder2,    NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMeshBuilder3,    NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFace,            NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFace2,           NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMLight,           NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMTexture,         NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMTexture2,        NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMTexture3,        NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMaterial,        NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMaterial2,       NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimationSet,    NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimationSet2,   NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMObjectArray,     NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMDeviceArray,     NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMViewportArray,   NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFrameArray,      NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMVisualArray,     NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMLightArray,      NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMPickedArray,     NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFaceArray,       NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimationArray,  NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMUserVisual,      NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMShadow,          NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMShadow2,         NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMInterpolator,    NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMProgressiveMesh, NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMPicked2Array,    NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMClippedVisual,   NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawClipper,         NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface7,        NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface4,        NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface3,        NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface2,        NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface,         NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice7,           NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice3,           NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice2,           NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice,            NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D7,                 NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D3,                 NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D2,                 NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D,                  NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw7,               NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw4,               NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw3,               NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw2,               NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw,                NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DLight,             NULL,          NULL,                       CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IUnknown,                   &IID_IUnknown, NULL,                       S_OK                      },
    };
    IDirect3DRMAnimation2 *animation2;
    IDirect3DRMAnimation *animation;
    IDirect3DRM3 *d3drm3;
    IDirect3DRM *d3drm1;
    IUnknown *unknown;
    HRESULT hr;

    hr = Direct3DRMCreate(&d3drm1);
    ok(SUCCEEDED(hr), "Failed to create d3drm instance, hr %#x.\n", hr);

    hr = IDirect3DRM_CreateAnimation(d3drm1, &animation);
    ok(SUCCEEDED(hr), "Failed to create animation hr %#x.\n", hr);

    hr = IDirect3DRMAnimation_QueryInterface(animation, &IID_IUnknown, (void **)&unknown);
    ok(SUCCEEDED(hr), "Failed to get IUnknown from animation, hr %#x.\n", hr);
    IDirect3DRMAnimation_Release(animation);

    test_qi("animation_qi", unknown, &IID_IUnknown, tests, ARRAY_SIZE(tests));
    IUnknown_Release(unknown);

    hr = IDirect3DRM_QueryInterface(d3drm1, &IID_IDirect3DRM3, (void **)&d3drm3);
    ok(SUCCEEDED(hr), "Failed to get IDirect3DRM3, hr %#x.\n", hr);

    hr = IDirect3DRM3_CreateAnimation(d3drm3, &animation2);
    ok(SUCCEEDED(hr), "Failed to create animation hr %#x.\n", hr);

    hr = IDirect3DRMAnimation2_QueryInterface(animation2, &IID_IUnknown, (void **)&unknown);
    ok(SUCCEEDED(hr), "Failed to get IUnknown from animation, hr %#x.\n", hr);
    IDirect3DRMAnimation2_Release(animation2);

    test_qi("animation2_qi", unknown, &IID_IUnknown, tests, ARRAY_SIZE(tests));
    IUnknown_Release(unknown);

    IDirect3DRM3_Release(d3drm3);
    IDirect3DRM_Release(d3drm1);
}

static void test_wrap(void)
{
    IDirect3DRMWrap *wrap;
    IDirect3DRM *d3drm1;
    HRESULT hr;

    hr = Direct3DRMCreate(&d3drm1);
    ok(SUCCEEDED(hr), "Failed to create IDirect3DRM instance, hr %#x.\n", hr);

    hr = IDirect3DRM_CreateObject(d3drm1, &CLSID_CDirect3DRMWrap, NULL, &IID_IDirect3DRMWrap, (void **)&wrap);
    ok(SUCCEEDED(hr), "Failed to create wrap instance, hr %#x.\n", hr);

    test_class_name((IDirect3DRMObject *)wrap, "");

    IDirect3DRMWrap_Release(wrap);
    IDirect3DRM_Release(d3drm1);
}

static void test_wrap_qi(void)
{
    static const struct qi_test tests[] =
    {
        { &IID_IDirect3DRMWrap,            &IID_IUnknown, &IID_IDirect3DRMWrap,   S_OK                      },
        { &IID_IDirect3DRM,                NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMDevice,          NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMObject,          &IID_IUnknown, &IID_IDirect3DRMWrap,   S_OK                      },
        { &IID_IDirect3DRMDevice2,         NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMDevice3,         NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMViewport,        NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMViewport2,       NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRM3,               NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRM2,               NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMVisual,          NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMesh,            NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMeshBuilder,     NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMeshBuilder2,    NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMeshBuilder3,    NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFace,            NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFace2,           NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMLight,           NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMTexture,         NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMTexture2,        NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMTexture3,        NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMaterial,        NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMaterial2,       NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimation,       NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimation2,      NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimationSet,    NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimationSet2,   NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMObjectArray,     NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMDeviceArray,     NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMViewportArray,   NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFrameArray,      NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMVisualArray,     NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMLightArray,      NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMPickedArray,     NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFaceArray,       NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimationArray,  NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMUserVisual,      NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMShadow,          NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMShadow2,         NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMInterpolator,    NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMProgressiveMesh, NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMPicked2Array,    NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMClippedVisual,   NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawClipper,         NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface7,        NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface4,        NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface3,        NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface2,        NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface,         NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice7,           NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice3,           NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice2,           NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice,            NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D7,                 NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D3,                 NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D2,                 NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D,                  NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw7,               NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw4,               NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw3,               NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw2,               NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw,                NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DLight,             NULL,          NULL,                   CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IUnknown,                   &IID_IUnknown, NULL,                   S_OK                      },
    };
    IDirect3DRMWrap *wrap;
    IDirect3DRM *d3drm1;
    IUnknown *unknown;
    HRESULT hr;

    hr = Direct3DRMCreate(&d3drm1);
    ok(SUCCEEDED(hr), "Failed to create d3drm instance, hr %#x.\n", hr);

    hr = IDirect3DRM_CreateObject(d3drm1, &CLSID_CDirect3DRMWrap, NULL, &IID_IDirect3DRMWrap, (void **)&wrap);
    ok(SUCCEEDED(hr), "Failed to create wrap instance, hr %#x.\n", hr);

    hr = IDirect3DRMWrap_QueryInterface(wrap, &IID_IUnknown, (void **)&unknown);
    ok(SUCCEEDED(hr), "Failed to get IUnknown from wrap (hr = %#x)\n", hr);
    IDirect3DRMWrap_Release(wrap);
    test_qi("wrap_qi", unknown, &IID_IUnknown, tests, ARRAY_SIZE(tests));
    IUnknown_Release(unknown);

    IDirect3DRM_Release(d3drm1);
}
START_TEST(d3drm)
{
    test_MeshBuilder();
    test_MeshBuilder3();
    test_Mesh();
    test_Face();
    test_Frame();
    test_Device();
    test_object();
    test_Viewport();
    test_Light();
    test_Material2();
    test_Texture();
    test_frame_transform();
    test_d3drm_load();
    test_frame_mesh_materials();
    test_d3drm_qi();
    test_frame_qi();
    test_device_qi();
    test_create_device_from_clipper1();
    test_create_device_from_clipper2();
    test_create_device_from_clipper3();
    test_create_device_from_surface1();
    test_create_device_from_surface2();
    test_create_device_from_surface3();
    test_create_device_from_d3d1();
    test_create_device_from_d3d2();
    test_create_device_from_d3d3();
    test_create_device_1();
    test_create_device_2();
    test_create_device_3();
    test_load_texture();
    test_texture_qi();
    test_viewport_qi();
    test_viewport_clear1();
    test_viewport_clear2();
    test_create_texture_from_surface();
    test_animation();
    test_animation_qi();
    test_wrap();
    test_wrap_qi();
}
