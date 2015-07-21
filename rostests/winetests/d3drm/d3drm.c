/*
 * Copyright 2010, 2012 Christian Costa
 * Copyright 2012 André Hentschel
 * Copyright 2011-2014 Henri Verbeet for CodeWeavers
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
#include <d3d.h>
#include <initguid.h>
#include <d3drm.h>
#include <d3drmwin.h>

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

static BOOL match_float(float a, float b)
{
    return (a - b) < 0.000001f;
}

static D3DRMMATRIX4D identity = {
    { 1.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f }
};

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
    CHAR cname[64] = {0};

    hr = Direct3DRMCreate(&d3drm);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);

    hr = IDirect3DRM_CreateMeshBuilder(d3drm, &pMeshBuilder);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMMeshBuilder interface (hr = %x)\n", hr);

    hr = IDirect3DRMMeshBuilder_GetClassName(pMeshBuilder, NULL, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    hr = IDirect3DRMMeshBuilder_GetClassName(pMeshBuilder, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = 1;
    hr = IDirect3DRMMeshBuilder_GetClassName(pMeshBuilder, &size, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = sizeof(cname);
    hr = IDirect3DRMMeshBuilder_GetClassName(pMeshBuilder, &size, cname);
    ok(hr == D3DRM_OK, "Cannot get classname (hr = %x)\n", hr);
    ok(size == sizeof("Builder"), "wrong size: %u\n", size);
    ok(!strcmp(cname, "Builder"), "Expected cname to be \"Builder\", but got \"%s\"\n", cname);

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
    ok(match_float(U1(n[0]).x, 0.577350f),  "Wrong component n[0].x = %f (expected %f)\n", U1(n[0]).x, 0.577350f);
    ok(match_float(U2(n[0]).y, 0.577350f),  "Wrong component n[0].y = %f (expected %f)\n", U2(n[0]).y, 0.577350f);
    ok(match_float(U3(n[0]).z, 0.577350f),  "Wrong component n[0].z = %f (expected %f)\n", U3(n[0]).z, 0.577350f);
    ok(match_float(U1(n[1]).x, -0.229416f), "Wrong component n[1].x = %f (expected %f)\n", U1(n[1]).x, -0.229416f);
    ok(match_float(U2(n[1]).y, 0.688247f),  "Wrong component n[1].y = %f (expected %f)\n", U2(n[1]).y, 0.688247f);
    ok(match_float(U3(n[1]).z, 0.688247f),  "Wrong component n[1].z = %f (expected %f)\n", U3(n[1]).z, 0.688247f);
    ok(match_float(U1(n[2]).x, -0.229416f), "Wrong component n[2].x = %f (expected %f)\n", U1(n[2]).x, -0.229416f);
    ok(match_float(U2(n[2]).y, 0.688247f),  "Wrong component n[2].y = %f (expected %f)\n", U2(n[2]).y, 0.688247f);
    ok(match_float(U3(n[2]).z, 0.688247f),  "Wrong component n[2].z = %f (expected %f)\n", U3(n[2]).z, 0.688247f);
    ok(match_float(U1(n[3]).x, -0.577350f), "Wrong component n[3].x = %f (expected %f)\n", U1(n[3]).x, -0.577350f);
    ok(match_float(U2(n[3]).y, 0.577350f),  "Wrong component n[3].y = %f (expected %f)\n", U2(n[3]).y, 0.577350f);
    ok(match_float(U3(n[3]).z, 0.577350f),  "Wrong component n[3].z = %f (expected %f)\n", U3(n[3]).z, 0.577350f);

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
    ok(U1(v[0]).x == 0.1f, "Wrong component v[0].x = %f (expected 0.1)\n", U1(v[0]).x);
    ok(U2(v[0]).y == 0.2f, "Wrong component v[0].y = %f (expected 0.2)\n", U2(v[0]).y);
    ok(U3(v[0]).z == 0.3f, "Wrong component v[0].z = %f (expected 0.3)\n", U3(v[0]).z);
    ok(U1(v[1]).x == 0.4f, "Wrong component v[1].x = %f (expected 0.4)\n", U1(v[1]).x);
    ok(U2(v[1]).y == 0.5f, "Wrong component v[1].y = %f (expected 0.5)\n", U2(v[1]).y);
    ok(U3(v[1]).z == 0.6f, "Wrong component v[1].z = %f (expected 0.6)\n", U3(v[1]).z);
    ok(U1(v[2]).x == 0.7f, "Wrong component v[2].x = %f (expected 0.7)\n", U1(v[2]).x);
    ok(U2(v[2]).y == 0.8f, "Wrong component v[2].y = %f (expected 0.8)\n", U2(v[2]).y);
    ok(U3(v[2]).z == 0.9f, "Wrong component v[2].z = %f (expected 0.9)\n", U3(v[2]).z);
    ok(U1(n[0]).x == 1.1f, "Wrong component n[0].x = %f (expected 1.1)\n", U1(n[0]).x);
    ok(U2(n[0]).y == 1.2f, "Wrong component n[0].y = %f (expected 1.2)\n", U2(n[0]).y);
    ok(U3(n[0]).z == 1.3f, "Wrong component n[0].z = %f (expected 1.3)\n", U3(n[0]).z);
    ok(U1(n[1]).x == 1.4f, "Wrong component n[1].x = %f (expected 1.4)\n", U1(n[1]).x);
    ok(U2(n[1]).y == 1.5f, "Wrong component n[1].y = %f (expected 1.5)\n", U2(n[1]).y);
    ok(U3(n[1]).z == 1.6f, "Wrong component n[1].z = %f (expected 1.6)\n", U3(n[1]).z);
    ok(U1(n[2]).x == 1.7f, "Wrong component n[2].x = %f (expected 1.7)\n", U1(n[2]).x);
    ok(U2(n[2]).y == 1.8f, "Wrong component n[2].y = %f (expected 1.8)\n", U2(n[2]).y);
    ok(U3(n[2]).z == 1.9f, "Wrong component n[2].z = %f (expected 1.9)\n", U3(n[2]).z);
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
    ok(U1(v[0]).x == 0.1f*2, "Wrong component v[0].x = %f (expected %f)\n", U1(v[0]).x, 0.1f*2);
    ok(U2(v[0]).y == 0.2f*3, "Wrong component v[0].y = %f (expected %f)\n", U2(v[0]).y, 0.2f*3);
    ok(U3(v[0]).z == 0.3f*4, "Wrong component v[0].z = %f (expected %f)\n", U3(v[0]).z, 0.3f*4);
    ok(U1(v[1]).x == 0.4f*2, "Wrong component v[1].x = %f (expected %f)\n", U1(v[1]).x, 0.4f*2);
    ok(U2(v[1]).y == 0.5f*3, "Wrong component v[1].y = %f (expected %f)\n", U2(v[1]).y, 0.5f*3);
    ok(U3(v[1]).z == 0.6f*4, "Wrong component v[1].z = %f (expected %f)\n", U3(v[1]).z, 0.6f*4);
    ok(U1(v[2]).x == 0.7f*2, "Wrong component v[2].x = %f (expected %f)\n", U1(v[2]).x, 0.7f*2);
    ok(U2(v[2]).y == 0.8f*3, "Wrong component v[2].y = %f (expected %f)\n", U2(v[2]).y, 0.8f*3);
    ok(U3(v[2]).z == 0.9f*4, "Wrong component v[2].z = %f (expected %f)\n", U3(v[2]).z, 0.9f*4);
    /* Normals are not affected by Scale */
    ok(U1(n[0]).x == 1.1f, "Wrong component n[0].x = %f (expected %f)\n", U1(n[0]).x, 1.1f);
    ok(U2(n[0]).y == 1.2f, "Wrong component n[0].y = %f (expected %f)\n", U2(n[0]).y, 1.2f);
    ok(U3(n[0]).z == 1.3f, "Wrong component n[0].z = %f (expected %f)\n", U3(n[0]).z, 1.3f);
    ok(U1(n[1]).x == 1.4f, "Wrong component n[1].x = %f (expected %f)\n", U1(n[1]).x, 1.4f);
    ok(U2(n[1]).y == 1.5f, "Wrong component n[1].y = %f (expected %f)\n", U2(n[1]).y, 1.5f);
    ok(U3(n[1]).z == 1.6f, "Wrong component n[1].z = %f (expected %f)\n", U3(n[1]).z, 1.6f);
    ok(U1(n[2]).x == 1.7f, "Wrong component n[2].x = %f (expected %f)\n", U1(n[2]).x, 1.7f);
    ok(U2(n[2]).y == 1.8f, "Wrong component n[2].y = %f (expected %f)\n", U2(n[2]).y, 1.8f);
    ok(U3(n[2]).z == 1.9f, "Wrong component n[2].z = %f (expected %f)\n", U3(n[2]).z, 1.9f);

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
    DWORD size;
    CHAR cname[64] = {0};

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

    hr = IDirect3DRMMeshBuilder3_GetClassName(pMeshBuilder3, NULL, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    hr = IDirect3DRMMeshBuilder3_GetClassName(pMeshBuilder3, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = 1;
    hr = IDirect3DRMMeshBuilder3_GetClassName(pMeshBuilder3, &size, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = sizeof(cname);
    hr = IDirect3DRMMeshBuilder3_GetClassName(pMeshBuilder3, &size, cname);
    ok(hr == D3DRM_OK, "Cannot get classname (hr = %x)\n", hr);
    ok(size == sizeof("Builder"), "wrong size: %u\n", size);
    ok(!strcmp(cname, "Builder"), "Expected cname to be \"Builder\", but got \"%s\"\n", cname);

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
    DWORD size;
    CHAR cname[64] = {0};

    hr = Direct3DRMCreate(&d3drm);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);

    hr = IDirect3DRM_CreateMesh(d3drm, &mesh);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMMesh interface (hr = %x)\n", hr);

    hr = IDirect3DRMMesh_GetClassName(mesh, NULL, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    hr = IDirect3DRMMesh_GetClassName(mesh, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = 1;
    hr = IDirect3DRMMesh_GetClassName(mesh, &size, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = sizeof(cname);
    hr = IDirect3DRMMesh_GetClassName(mesh, &size, cname);
    ok(hr == D3DRM_OK, "Cannot get classname (hr = %x)\n", hr);
    ok(size == sizeof("Mesh"), "wrong size: %u\n", size);
    ok(!strcmp(cname, "Mesh"), "Expected cname to be \"Mesh\", but got \"%s\"\n", cname);

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
    IDirect3DRMFace2 *face2;
    IDirect3DRMFaceArray *array1;
    D3DRMLOADMEMORY info;
    D3DVECTOR v1[4], n1[4], v2[4], n2[4];
    DWORD count;
    CHAR cname[64] = {0};
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

    hr = IDirect3DRMFace_GetClassName(face1, NULL, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    hr = IDirect3DRMFace_GetClassName(face1, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    count = 1;
    hr = IDirect3DRMFace_GetClassName(face1, &count, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    count = sizeof(cname);
    hr = IDirect3DRMFace_GetClassName(face1, &count, cname);
    ok(hr == D3DRM_OK, "Cannot get classname (hr = %x)\n", hr);
    ok(count == sizeof("Face"), "wrong size: %u\n", count);
    ok(!strcmp(cname, "Face"), "Expected cname to be \"Face\", but got \"%s\"\n", cname);

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

    hr = IDirect3DRMFace2_GetClassName(face2, NULL, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    hr = IDirect3DRMFace2_GetClassName(face2, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    count = 1;
    hr = IDirect3DRMFace2_GetClassName(face2, &count, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    count = sizeof(cname);
    hr = IDirect3DRMFace2_GetClassName(face2, &count, cname);
    ok(hr == D3DRM_OK, "Cannot get classname (hr = %x)\n", hr);
    ok(count == sizeof("Face"), "wrong size: %u\n", count);
    ok(!strcmp(cname, "Face"), "Expected cname to be \"Face\", but got \"%s\"\n", cname);

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
        ok(U1(v2[0]).x == U1(v1[0]).x, "Wrong component v2[0].x = %f (expected %f)\n",
           U1(v2[0]).x, U1(v1[0]).x);
        ok(U2(v2[0]).y == U2(v1[0]).y, "Wrong component v2[0].y = %f (expected %f)\n",
           U2(v2[0]).y, U2(v1[0]).y);
        ok(U3(v2[0]).z == U3(v1[0]).z, "Wrong component v2[0].z = %f (expected %f)\n",
           U3(v2[0]).z, U3(v1[0]).z);
        ok(U1(v2[1]).x == U1(v1[1]).x, "Wrong component v2[1].x = %f (expected %f)\n",
           U1(v2[1]).x, U1(v1[1]).x);
        ok(U2(v2[1]).y == U2(v1[1]).y, "Wrong component v2[1].y = %f (expected %f)\n",
           U2(v2[1]).y, U2(v1[1]).y);
        ok(U3(v2[1]).z == U3(v1[1]).z, "Wrong component v2[1].z = %f (expected %f)\n",
           U3(v2[1]).z, U3(v1[1]).z);
        ok(U1(v2[2]).x == U1(v1[2]).x, "Wrong component v2[2].x = %f (expected %f)\n",
           U1(v2[2]).x, U1(v1[2]).x);
        ok(U2(v2[2]).y == U2(v1[2]).y, "Wrong component v2[2].y = %f (expected %f)\n",
           U2(v2[2]).y, U2(v1[2]).y);
        ok(U3(v2[2]).z == U3(v1[2]).z, "Wrong component v2[2].z = %f (expected %f)\n",
           U3(v2[2]).z, U3(v1[2]).z);

        ok(U1(n2[0]).x == U1(n1[0]).x, "Wrong component n2[0].x = %f (expected %f)\n",
           U1(n2[0]).x, U1(n1[0]).x);
        ok(U2(n2[0]).y == U2(n1[0]).y, "Wrong component n2[0].y = %f (expected %f)\n",
           U2(n2[0]).y, U2(n1[0]).y);
        ok(U3(n2[0]).z == U3(n1[0]).z, "Wrong component n2[0].z = %f (expected %f)\n",
           U3(n2[0]).z, U3(n1[0]).z);
        ok(U1(n2[1]).x == U1(n1[1]).x, "Wrong component n2[1].x = %f (expected %f)\n",
           U1(n2[1]).x, U1(n1[1]).x);
        ok(U2(n2[1]).y == U2(n1[1]).y, "Wrong component n2[1].y = %f (expected %f)\n",
           U2(n2[1]).y, U2(n1[1]).y);
        ok(U3(n2[1]).z == U3(n1[1]).z, "Wrong component n2[1].z = %f (expected %f)\n",
           U3(n2[1]).z, U3(n1[1]).z);
        ok(U1(n2[2]).x == U1(n1[2]).x, "Wrong component n2[2].x = %f (expected %f)\n",
           U1(n2[2]).x, U1(n1[2]).x);
        ok(U2(n2[2]).y == U2(n1[2]).y, "Wrong component n2[2].y = %f (expected %f)\n",
           U2(n2[2]).y, U2(n1[2]).y);
        ok(U3(n2[2]).z == U3(n1[2]).z, "Wrong component n2[2].z = %f (expected %f)\n",
           U3(n2[2]).z, U3(n1[2]).z);

        IDirect3DRMFace_Release(face);
        IDirect3DRMFaceArray_Release(array1);
    }

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
    IDirect3DRMFrameArray *frame_array;
    IDirect3DRMMeshBuilder *mesh_builder;
    IDirect3DRMVisual *visual1;
    IDirect3DRMVisual *visual_tmp;
    IDirect3DRMVisualArray *visual_array;
    IDirect3DRMLight *light1;
    IDirect3DRMLight *light_tmp;
    IDirect3DRMLightArray *light_array;
    D3DCOLOR color;
    DWORD count;
    CHAR cname[64] = {0};

    hr = Direct3DRMCreate(&d3drm);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);

    hr = IDirect3DRM_CreateFrame(d3drm, NULL, &pFrameC);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMFrame interface (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameC, 1);

    hr = IDirect3DRMFrame_GetClassName(pFrameC, NULL, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    hr = IDirect3DRMFrame_GetClassName(pFrameC, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    count = 1;
    hr = IDirect3DRMFrame_GetClassName(pFrameC, &count, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    count = sizeof(cname);
    hr = IDirect3DRMFrame_GetClassName(pFrameC, &count, cname);
    ok(hr == D3DRM_OK, "Cannot get classname (hr = %x)\n", hr);
    ok(count == sizeof("Frame"), "wrong size: %u\n", count);
    ok(!strcmp(cname, "Frame"), "Expected cname to be \"Frame\", but got \"%s\"\n", cname);

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

    pFrameTmp = (void*)0xdeadbeef;
    hr = IDirect3DRMFrame_GetParent(pFrameC, &pFrameTmp);
    ok(hr == D3DRM_OK, "Cannot get parent frame (hr = %x)\n", hr);
    ok(pFrameTmp == pFrameP2, "pFrameTmp = %p\n", pFrameTmp);
    CHECK_REFCOUNT(pFrameP2, 2);
    CHECK_REFCOUNT(pFrameC, 2);

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
    CHECK_REFCOUNT(pFrameP1, 3);

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
    CHECK_REFCOUNT(pFrameP1, 3);

    hr = IDirect3DRMFrame_DeleteVisual(pFrameP1, NULL);
    ok(hr == D3DRMERR_BADOBJECT, "Should have returned D3DRMERR_BADOBJECT (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 3);

    /* Create Visual */
    hr = IDirect3DRM_CreateMeshBuilder(d3drm, &mesh_builder);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMMeshBuilder interface (hr = %x)\n", hr);
    visual1 = (IDirect3DRMVisual *)mesh_builder;

    /* Add Visual to first parent */
    hr = IDirect3DRMFrame_AddVisual(pFrameP1, visual1);
    ok(hr == D3DRM_OK, "Cannot add visual (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 3);
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
    CHECK_REFCOUNT(pFrameP1, 3);
    IDirect3DRMMeshBuilder_Release(mesh_builder);

    /* [Add/Delete]Light with NULL pointer */
    hr = IDirect3DRMFrame_AddLight(pFrameP1, NULL);
    ok(hr == D3DRMERR_BADOBJECT, "Should have returned D3DRMERR_BADOBJECT (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 3);

    hr = IDirect3DRMFrame_DeleteLight(pFrameP1, NULL);
    ok(hr == D3DRMERR_BADOBJECT, "Should have returned D3DRMERR_BADOBJECT (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 3);

    /* Create Light */
    hr = IDirect3DRM_CreateLightRGB(d3drm, D3DRMLIGHT_SPOT, 0.1, 0.2, 0.3, &light1);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMLight interface (hr = %x)\n", hr);

    /* Add Light to first parent */
    hr = IDirect3DRMFrame_AddLight(pFrameP1, light1);
    ok(hr == D3DRM_OK, "Cannot add light (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 3);
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
    CHECK_REFCOUNT(pFrameP1, 3);
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

    /* Cleanup */
    IDirect3DRMFrame_Release(pFrameP2);
    CHECK_REFCOUNT(pFrameC, 2);
    CHECK_REFCOUNT(pFrameP1, 3);

    IDirect3DRMFrame_Release(pFrameC);
    IDirect3DRMFrame_Release(pFrameP1);

    IDirect3DRM_Release(d3drm);
}

static void test_Viewport(void)
{
    IDirectDrawClipper *pClipper;
    HRESULT hr;
    IDirect3DRM *d3drm;
    IDirect3DRMDevice *device;
    IDirect3DRMFrame *frame;
    IDirect3DRMViewport *viewport;
    GUID driver;
    HWND window;
    RECT rc;
    DWORD size;
    CHAR cname[64] = {0};

    window = CreateWindowA("static", "d3drm_test", WS_OVERLAPPEDWINDOW, 0, 0, 300, 200, 0, 0, 0, 0);
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

    hr = IDirect3DRM_CreateFrame(d3drm, NULL, &frame);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMFrame interface (hr = %x)\n", hr);

    hr = IDirect3DRM_CreateViewport(d3drm, device, frame, rc.left, rc.top, rc.right, rc.bottom, &viewport);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMViewport interface (hr = %x)\n", hr);

    hr = IDirect3DRMViewport_GetClassName(viewport, NULL, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    hr = IDirect3DRMViewport_GetClassName(viewport, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = 1;
    hr = IDirect3DRMViewport_GetClassName(viewport, &size, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = sizeof(cname);
    hr = IDirect3DRMViewport_GetClassName(viewport, &size, cname);
    ok(hr == D3DRM_OK, "Cannot get classname (hr = %x)\n", hr);
    ok(size == sizeof("Viewport"), "wrong size: %u\n", size);
    ok(!strcmp(cname, "Viewport"), "Expected cname to be \"Viewport\", but got \"%s\"\n", cname);

    IDirect3DRMViewport_Release(viewport);
    IDirect3DRMFrame_Release(frame);
    IDirect3DRMDevice_Release(device);
    IDirectDrawClipper_Release(pClipper);

    IDirect3DRM_Release(d3drm);
    DestroyWindow(window);
}

static void test_Light(void)
{
    HRESULT hr;
    IDirect3DRM *d3drm;
    IDirect3DRMLight *light;
    D3DRMLIGHTTYPE type;
    D3DCOLOR color;
    DWORD size;
    CHAR cname[64] = {0};

    hr = Direct3DRMCreate(&d3drm);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);

    hr = IDirect3DRM_CreateLightRGB(d3drm, D3DRMLIGHT_SPOT, 0.5, 0.5, 0.5, &light);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMLight interface (hr = %x)\n", hr);

    hr = IDirect3DRMLight_GetClassName(light, NULL, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    hr = IDirect3DRMLight_GetClassName(light, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = 1;
    hr = IDirect3DRMLight_GetClassName(light, &size, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = sizeof(cname);
    hr = IDirect3DRMLight_GetClassName(light, &size, cname);
    ok(hr == D3DRM_OK, "Cannot get classname (hr = %x)\n", hr);
    ok(size == sizeof("Light"), "wrong size: %u\n", size);
    ok(!strcmp(cname, "Light"), "Expected cname to be \"Light\", but got \"%s\"\n", cname);

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
    DWORD size;
    CHAR cname[64] = {0};

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

    hr = IDirect3DRMMaterial2_GetClassName(material2, NULL, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    hr = IDirect3DRMMaterial2_GetClassName(material2, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = 1;
    hr = IDirect3DRMMaterial2_GetClassName(material2, &size, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = sizeof(cname);
    hr = IDirect3DRMMaterial2_GetClassName(material2, &size, cname);
    ok(hr == D3DRM_OK, "Cannot get classname (hr = %x)\n", hr);
    ok(size == sizeof("Material"), "wrong size: %u\n", size);
    ok(!strcmp(cname, "Material"), "Expected cname to be \"Material\", but got \"%s\"\n", cname);

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
    IDirect3DRM *d3drm;
    IDirect3DRMTexture *texture;
    D3DRMIMAGE initimg = {
        2, 2, 1, 1, 32,
        TRUE, 2 * sizeof(DWORD), NULL, NULL,
        0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000, 0, NULL
    };
    DWORD pixel[4] = { 20000, 30000, 10000, 0 };
    DWORD size;
    CHAR cname[64] = {0};

    hr = Direct3DRMCreate(&d3drm);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);

    initimg.buffer1 = &pixel;
    hr = IDirect3DRM_CreateTexture(d3drm, &initimg, &texture);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMTexture interface (hr = %x)\n", hr);

    hr = IDirect3DRMTexture_GetClassName(texture, NULL, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    hr = IDirect3DRMTexture_GetClassName(texture, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = 1;
    hr = IDirect3DRMTexture_GetClassName(texture, &size, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = sizeof(cname);
    hr = IDirect3DRMTexture_GetClassName(texture, &size, cname);
    ok(hr == D3DRM_OK, "Cannot get classname (hr = %x)\n", hr);
    ok(size == sizeof("Texture"), "wrong size: %u\n", size);
    ok(!strcmp(cname, "Texture"), "Expected cname to be \"Texture\", but got \"%s\"\n", cname);

    IDirect3DRMTexture_Release(texture);

    IDirect3DRM_Release(d3drm);
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
    DWORD size;
    CHAR cname[64] = {0};

    window = CreateWindowA("static", "d3drm_test", WS_OVERLAPPEDWINDOW, 0, 0, 300, 200, 0, 0, 0, 0);
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

    hr = IDirect3DRMDevice_GetClassName(device, NULL, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    hr = IDirect3DRMDevice_GetClassName(device, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = 1;
    hr = IDirect3DRMDevice_GetClassName(device, &size, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = sizeof(cname);
    hr = IDirect3DRMDevice_GetClassName(device, &size, cname);
    ok(hr == D3DRM_OK, "Cannot get classname (hr = %x)\n", hr);
    ok(size == sizeof("Device"), "wrong size: %u\n", size);
    ok(!strcmp(cname, "Device"), "Expected cname to be \"Device\", but got \"%s\"\n", cname);

    /* WinDevice */
    if (FAILED(hr = IDirect3DRMDevice_QueryInterface(device, &IID_IDirect3DRMWinDevice, (void **)&win_device)))
    {
        win_skip("Cannot get IDirect3DRMWinDevice interface (hr = %x), skipping tests\n", hr);
        goto cleanup;
    }

    hr = IDirect3DRMWinDevice_GetClassName(win_device, NULL, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    hr = IDirect3DRMWinDevice_GetClassName(win_device, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = 1;
    hr = IDirect3DRMWinDevice_GetClassName(win_device, &size, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = sizeof(cname);
    hr = IDirect3DRMWinDevice_GetClassName(win_device, &size, cname);
    ok(hr == D3DRM_OK, "Cannot get classname (hr = %x)\n", hr);
    ok(size == sizeof("Device"), "wrong size: %u\n", size);
    ok(!strcmp(cname, "Device"), "Expected cname to be \"Device\", but got \"%s\"\n", cname);

    IDirect3DRMWinDevice_Release(win_device);

cleanup:
    IDirect3DRMDevice_Release(device);
    IDirectDrawClipper_Release(pClipper);

    IDirect3DRM_Release(d3drm);
    DestroyWindow(window);
}

static void test_frame_transform(void)
{
    HRESULT hr;
    IDirect3DRM *d3drm;
    IDirect3DRMFrame *frame;
    D3DRMMATRIX4D matrix;

    hr = Direct3DRMCreate(&d3drm);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);

    hr = IDirect3DRM_CreateFrame(d3drm, NULL, &frame);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMFrame interface (hr = %x)\n", hr);

    hr = IDirect3DRMFrame_GetTransform(frame, matrix);
    ok(hr == D3DRM_OK, "IDirect3DRMFrame_GetTransform returned hr = %x\n", hr);
    ok(!memcmp(matrix, identity, sizeof(D3DRMMATRIX4D)), "Returned matrix is not identity\n");

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
        { &IID_IDirect3DRM3,               &IID_IDirect3DRM3,    S_OK,                     },
        { &IID_IDirect3DRM2,               &IID_IDirect3DRM2,    S_OK,                     },
        { &IID_IDirect3DRM,                &IID_IDirect3DRM,     S_OK,                     },
        { &IID_IDirect3DRMDevice,          NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMObject,          NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMObject2,         NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMDevice2,         NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMDevice3,         NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMViewport,        NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMViewport2,       NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFrame,           NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFrame2,          NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFrame3,          NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMVisual,          NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMesh,            NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMeshBuilder,     NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMeshBuilder2,    NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMeshBuilder3,    NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFace,            NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFace2,           NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMLight,           NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMTexture,         NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMTexture2,        NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMTexture3,        NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMWrap,            NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMaterial,        NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMaterial2,       NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimation,       NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimation2,      NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimationSet,    NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimationSet2,   NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMObjectArray,     NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMDeviceArray,     NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMViewportArray,   NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFrameArray,      NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMVisualArray,     NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMLightArray,      NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMPickedArray,     NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFaceArray,       NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimationArray,  NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMUserVisual,      NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMShadow,          NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMShadow2,         NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMInterpolator,    NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMProgressiveMesh, NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMPicked2Array,    NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMClippedVisual,   NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawClipper,         NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface7,        NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface4,        NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface3,        NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface2,        NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface,         NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice7,           NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice3,           NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice2,           NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice,            NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D7,                 NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D3,                 NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D2,                 NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D,                  NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw7,               NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw4,               NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw3,               NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw2,               NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw,                NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DLight,             NULL,                 CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IUnknown,                   &IID_IDirect3DRM,     S_OK                      },
    };
    HRESULT hr;
    IDirect3DRM *d3drm;

    hr = Direct3DRMCreate(&d3drm);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);

    test_qi("d3drm_qi", (IUnknown *)d3drm, &IID_IDirect3DRM, tests, sizeof(tests) / sizeof(*tests));

    IDirect3DRM_Release(d3drm);
}

static void test_frame_qi(void)
{
    static const struct qi_test tests[] =
    {
        { &IID_IDirect3DRMFrame3,             &IID_IUnknown,    S_OK                      },
        { &IID_IDirect3DRMFrame2,             &IID_IUnknown,    S_OK                      },
        { &IID_IDirect3DRMFrame,              &IID_IUnknown,    S_OK                      },
        { &IID_IDirect3DRM,                   NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMDevice,             NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMObject,             &IID_IUnknown,    S_OK                      },
        { &IID_IDirect3DRMDevice2,            NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMDevice3,            NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMViewport,           NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMViewport2,          NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRM3,                  NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRM2,                  NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMVisual,             &IID_IUnknown,    S_OK                      },
        { &IID_IDirect3DRMMesh,               NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMeshBuilder,        NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMeshBuilder2,       NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMeshBuilder3,       NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFace,               NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFace2,              NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMLight,              NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMTexture,            NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMTexture2,           NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMTexture3,           NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMWrap,               NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMaterial,           NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMMaterial2,          NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimation,          NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimation2,         NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimationSet,       NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimationSet2,      NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMObjectArray,        NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMDeviceArray,        NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMViewportArray,      NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFrameArray,         NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMVisualArray,        NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMLightArray,         NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMPickedArray,        NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMFaceArray,          NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMAnimationArray,     NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMUserVisual,         NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMShadow,             NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMShadow2,            NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMInterpolator,       NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMProgressiveMesh,    NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMPicked2Array,       NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DRMClippedVisual,      NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawClipper,            NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface7,           NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface4,           NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface3,           NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface2,           NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDrawSurface,            NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice7,              NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice3,              NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice2,              NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DDevice,               NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D7,                    NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D3,                    NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D2,                    NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3D,                     NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw7,                  NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw4,                  NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw3,                  NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw2,                  NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirectDraw,                   NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IDirect3DLight,                NULL,             CLASS_E_CLASSNOTAVAILABLE },
        { &IID_IUnknown,                      &IID_IUnknown,    S_OK                      },
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
    IDirect3DRMFrame_QueryInterface(frame1, &IID_IUnknown, (void **)&unknown);
    ok(hr == D3DRM_OK, "Failed to create IUnknown from frame1 (hr = %x)\n", hr);
    IDirect3DRMFrame_Release(frame1);
    test_qi("frame1_qi", unknown, &IID_IUnknown, tests, sizeof(tests) / sizeof(*tests));
    IUnknown_Release(unknown);

    hr = IDirect3DRM_QueryInterface(d3drm1, &IID_IDirect3DRM2, (void **)&d3drm2);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM2 interface (hr = %x).\n", hr);
    hr = IDirect3DRM2_CreateFrame(d3drm2, NULL, &frame2);
    ok(hr == D3DRM_OK, "Failed to create frame2 (hr = %x)\n", hr);
    IDirect3DRMFrame2_QueryInterface(frame2, &IID_IUnknown, (void **)&unknown);
    ok(hr == D3DRM_OK, "Failed to create IUnknown from frame2 (hr = %x)\n", hr);
    IDirect3DRMFrame2_Release(frame2);
    test_qi("frame2_qi", unknown, &IID_IUnknown, tests, sizeof(tests) / sizeof(*tests));
    IUnknown_Release(unknown);

    hr = IDirect3DRM_QueryInterface(d3drm1, &IID_IDirect3DRM3, (void **)&d3drm3);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM3 interface (hr = %x).\n", hr);
    hr = IDirect3DRM3_CreateFrame(d3drm3, NULL, &frame3);
    ok(hr == D3DRM_OK, "Failed to create frame3 (hr = %x)\n", hr);
    IDirect3DRMFrame3_QueryInterface(frame3, &IID_IUnknown, (void **)&unknown);
    ok(hr == D3DRM_OK, "Failed to create IUnknown from frame3 (hr = %x)\n", hr);
    IDirect3DRMFrame3_Release(frame3);
    test_qi("frame3_qi", unknown, &IID_IUnknown, tests, sizeof(tests) / sizeof(*tests));
    IUnknown_Release(unknown);

    IDirect3DRM3_Release(d3drm3);
    IDirect3DRM2_Release(d3drm2);
    IDirect3DRM_Release(d3drm1);
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

static void test_create_device_from_clipper(void)
{
    DDSCAPS caps = { DDSCAPS_ZBUFFER };
    IDirect3DRM *d3drm1 = NULL;
    IDirect3DRM2 *d3drm2 = NULL;
    IDirectDraw *ddraw = NULL;
    IUnknown *unknown = NULL;
    IDirect3DRMDevice2 *device2 = NULL;
    IDirect3DDevice2 *d3ddevice2 = NULL;
    IDirectDrawClipper *clipper = NULL, *d3drm_clipper = NULL;
    IDirectDrawSurface *surface = NULL, *ds = NULL, *d3drm_primary = NULL;
    IDirectDrawSurface7 *surface7 = NULL;
    DDSURFACEDESC desc, surface_desc;
    DWORD expected_flags;
    HWND window;
    GUID driver = IID_IDirect3DRGBDevice;
    HRESULT hr;
    ULONG ref1, ref2, ref3, cref1, cref2;
    RECT rc;

    window = CreateWindowA("static", "d3drm_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, 500, 400, 0, 0, 0, 0);
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
    todo_wine ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %x.\n", hr);

    /* If NULL is passed for clipper, CreateDeviceFromClipper returns D3DRMERR_BADVALUE */
    hr = IDirect3DRM2_CreateDeviceFromClipper(d3drm2, NULL, &driver, 0, 0, &device2);
    todo_wine ok(hr == D3DRMERR_BADVALUE, "Expected hr == D3DRMERR_BADVALUE, got %x.\n", hr);

    hr = IDirect3DRM2_CreateDeviceFromClipper(d3drm2, clipper, &driver, 300, 200, &device2);
    ok(hr == D3DRM_OK, "Cannot create IDirect3DRMDevice2 interface (hr = %x).\n", hr);
    ref3 = get_refcount((IUnknown *)d3drm1);
    todo_wine ok(ref3 > ref1, "expected ref3 > ref1, got ref1 = %u , ref3 = %u.\n", ref1, ref3);
    ref3 = get_refcount((IUnknown *)d3drm2);
    ok(ref3 == ref2, "expected ref3 == ref2, got ref2 = %u , ref3 = %u.\n", ref2, ref3);
    cref2 = get_refcount((IUnknown *)clipper);
    todo_wine ok(cref2 > cref1, "expected cref2 > cref1, got cref1 = %u , cref2 = %u.\n", cref1, cref2);

    /* Fetch immediate mode device in order to access render target */
    hr = IDirect3DRMDevice2_GetDirect3DDevice2(device2, &d3ddevice2);
    todo_wine ok(hr == D3DRM_OK, "Cannot get IDirect3DDevice2 interface (hr = %x).\n", hr);
    if (FAILED(hr))
        goto cleanup;

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
    if (SUCCEEDED(hr))
    {
        ok(d3drm_clipper == clipper, "Expected clipper returned == %p, got %p.\n", clipper , d3drm_clipper);
        IDirectDrawClipper_Release(d3drm_clipper);
    }
    if (d3drm_primary)
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
    todo_wine ok(cref1 == cref2, "expected cref1 == cref2, got cref1 = %u, cref2 = %u.\n", cref1, cref2);

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
    todo_wine ok(hr == D3DRM_OK, "Cannot get IDirect3DDevice2 interface (hr = %x).\n", hr);
    if (FAILED(hr))
        goto cleanup;

    hr = IDirect3DDevice2_GetRenderTarget(d3ddevice2, &surface);
    ok(hr == DD_OK, "Cannot get surface to the render target (hr = %x).\n", hr);

    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &surface_desc);
    ok(hr == DD_OK, "Cannot get surface desc structure (hr = %x).\n", hr);
    todo_wine ok(surface_desc.ddpfPixelFormat.dwRGBBitCount == 16, "Expected 16bpp, got %ubpp.\n",
            surface_desc.ddpfPixelFormat.dwRGBBitCount);

    hr = IDirectDraw2_RestoreDisplayMode(ddraw);
    ok(SUCCEEDED(hr), "RestoreDisplayMode failed, hr %#x.\n", hr);

cleanup:
    if (ds)
        IDirectDrawSurface_Release(ds);
    if (surface)
        IDirectDrawSurface_Release(surface);
    if (d3ddevice2)
        IDirect3DDevice2_Release(d3ddevice2);
    if (device2)
        IDirect3DRMDevice2_Release(device2);
    if (d3drm2)
        IDirect3DRM2_Release(d3drm2);
    if (d3drm1)
        IDirect3DRM_Release(d3drm1);
    if (clipper)
        IDirectDrawClipper_Release(clipper);
    if (ddraw)
        IDirectDraw_Release(ddraw);
    DestroyWindow(window);
}

static void test_create_device_from_surface(void)
{
    DDSCAPS caps = { DDSCAPS_ZBUFFER };
    DDSURFACEDESC desc;
    IDirectDraw *ddraw = NULL;
    IDirect3DRM *d3drm1 = NULL;
    IDirect3DRM2 *d3drm2 = NULL;
    IDirect3DRMDevice2 *device2 = NULL;
    IDirect3DDevice2 *d3ddevice2 = NULL;
    IDirectDrawSurface *surface = NULL, *ds = NULL, *d3drm_surface = NULL, *d3drm_ds = NULL;
    DWORD expected_flags;
    HWND window;
    GUID driver = IID_IDirect3DRGBDevice;
    ULONG ref1, ref2, ref3, surface_ref1, surface_ref2;
    RECT rc;
    BOOL use_sysmem_zbuffer = FALSE;
    HRESULT hr;

    hr = DirectDrawCreate(NULL, &ddraw, NULL);
    ok(hr == DD_OK, "Cannot get IDirectDraw interface (hr = %x).\n", hr);

    window = CreateWindowA("static", "d3drm_test", WS_OVERLAPPEDWINDOW, 0, 0, 300, 200, 0, 0, 0, 0);
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
    todo_wine ok(hr == DDERR_INVALIDCAPS, "Expected hr == DDERR_INVALIDCAPS, got %x.\n", hr);
    IDirectDrawSurface_Release(surface);

    desc.ddsCaps.dwCaps |= DDSCAPS_3DDEVICE;
    hr = IDirectDraw_CreateSurface(ddraw, &desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    surface_ref1 = get_refcount((IUnknown *)surface);
    hr = IDirect3DRM2_CreateDeviceFromSurface(d3drm2, &driver, ddraw, surface, &device2);
    ok(SUCCEEDED(hr), "Cannot create IDirect3DRMDevice2 interface (hr = %x).\n", hr);
    ref3 = get_refcount((IUnknown *)d3drm1);
    todo_wine ok(ref3 > ref1, "expected ref3 > ref1, got ref1 = %u , ref3 = %u.\n", ref1, ref3);
    ref3 = get_refcount((IUnknown *)d3drm2);
    ok(ref3 == ref2, "expected ref3 == ref2, got ref2 = %u , ref3 = %u.\n", ref2, ref3);
    surface_ref2 = get_refcount((IUnknown *)surface);
    todo_wine ok(surface_ref2 > surface_ref1, "Expected surface_ref2 > surface_ref1, got surface_ref1 = %u, surface_ref2 = %u.\n",
            surface_ref1, surface_ref2);

    /* Check if CreateDeviceFromSurface creates a primary surface */
    hr = IDirectDraw_EnumSurfaces(ddraw, DDENUMSURFACES_ALL | DDENUMSURFACES_DOESEXIST,
        NULL, &d3drm_surface, surface_callback);
    ok(hr == DD_OK, "Failed to enumerate surfaces (hr = %x).\n", hr);
    ok(d3drm_surface == NULL, "No primary surface should have enumerated (%p).\n", d3drm_surface);

    hr = IDirect3DRMDevice2_GetDirect3DDevice2(device2, &d3ddevice2);
    todo_wine ok(hr == D3DRM_OK, "Cannot get IDirect3DDevice2 interface (hr = %x).\n", hr);
    if (FAILED(hr))
        goto cleanup;

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
    ok((desc.dwWidth == rc.right) && (desc.dwHeight == rc.bottom), "Expected surface dimentions = %u, %u, got %u, %u.\n",
            rc.right, rc.bottom, desc.dwWidth, desc.dwHeight);
    ok(desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER, "Expected caps containing %x, got %x.\n", DDSCAPS_ZBUFFER, desc.ddsCaps.dwCaps);
    expected_flags = DDSD_ZBUFFERBITDEPTH | DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH;
    ok(desc.dwFlags == expected_flags, "Expected %x for flags, got %x.\n", expected_flags, desc.dwFlags);

    IDirectDrawSurface_Release(ds);
    IDirect3DDevice2_Release(d3ddevice2);
    IDirectDrawSurface_Release(d3drm_surface);
    if (device2)
    {
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
    }
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
    todo_wine ok(hr == D3DRM_OK, "Cannot get IDirect3DDevice2 interface (hr = %x).\n", hr);
    if (FAILED(hr))
        goto cleanup;

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

cleanup:
    if (d3ddevice2)
        IDirect3DDevice2_Release(d3ddevice2);
    if (device2)
    {
        IDirect3DRMDevice2_Release(device2);
        hr = IDirectDrawSurface_GetAttachedSurface(surface, &caps, &ds);
        todo_wine ok(hr == DD_OK, "Cannot get attached depth surface (hr = %x).\n", hr);
        if (SUCCEEDED(hr))
        {
            /*The render target still holds a reference to ds as the depth surface remains attached to it, so refcount will be 1*/
            ref1 = IDirectDrawSurface_Release(ds);
            ok(ref1 == 1, "Expected ref1 == 1, got %u.\n", ref1);
        }
    }
    if (surface)
    {
        ref1 = IDirectDrawSurface_Release(surface);
        ok(ref1 == 0, "Expected Render target refcount == 0, got %u.\n", ref1);
    }
    if (d3drm2)
        IDirect3DRM2_Release(d3drm2);
    if (d3drm1)
        IDirect3DRM_Release(d3drm1);
    if (ddraw)
        IDirectDraw_Release(ddraw);
    DestroyWindow(window);
}

static IDirect3DDevice2 *create_device(IDirectDraw2 *ddraw, HWND window, IDirectDrawSurface **ds)
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
    for (i = 0; i < sizeof(z_depths) / sizeof(*z_depths); ++i)
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

        if (SUCCEEDED(IDirect3D2_CreateDevice(d3d, &IID_IDirect3DRGBDevice, surface, &device)))
            break;

        IDirectDrawSurface_DeleteAttachedSurface(surface, 0, *ds);
        IDirectDrawSurface_Release(*ds);
        *ds = NULL;
    }

    IDirect3D2_Release(d3d);
    IDirectDrawSurface_Release(surface);
    return device;
}

static void test_create_device_from_d3d(void)
{
    IDirectDraw *ddraw1 = NULL;
    IDirectDraw2 *ddraw2 = NULL;
    IDirect3D2 *d3d2 = NULL;
    IDirect3DRM *d3drm1 = NULL;
    IDirect3DRM2 *d3drm2 = NULL;
    IDirect3DRMDevice2 *device2 = NULL;
    IDirect3DDevice2 *d3ddevice2 = NULL, *d3drm_d3ddevice2 = NULL;
    IDirectDrawSurface *surface = NULL, *ds = NULL, *d3drm_ds = NULL;
    DWORD expected_flags;
    DDSCAPS caps = { DDSCAPS_ZBUFFER };
    DDSURFACEDESC desc;
    RECT rc;
    HWND window;
    ULONG ref1, ref2, ref3, device_ref1, device_ref2;
    HRESULT hr;

    hr = DirectDrawCreate(NULL, &ddraw1, NULL);
    ok(hr == DD_OK, "Cannot get IDirectDraw interface (hr = %x).\n", hr);

    window = CreateWindowA("static", "d3drm_test", WS_OVERLAPPEDWINDOW, 0, 0, 300, 200, 0, 0, 0, 0);
    GetClientRect(window, &rc);

    hr = IDirectDraw_QueryInterface(ddraw1, &IID_IDirect3D2, (void **)&d3d2);
    ok(hr == DD_OK, "Cannot get IDirect3D2 interface (hr = %x).\n", hr);
    hr = IDirectDraw_QueryInterface(ddraw1, &IID_IDirectDraw2, (void **)&ddraw2);
    ok(hr == DD_OK, "Cannot get IDirectDraw2 interface (hr = %x).\n", hr);

    /* Create the immediate mode device */
    d3ddevice2 = create_device(ddraw2, window, &ds);
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

    hr = IDirect3DRM2_CreateDeviceFromD3D(d3drm2, d3d2, d3ddevice2, &device2);
    ok(hr == DD_OK, "Failed to create IDirect3DRMDevice2 interface (hr = %x)\n", hr);
    ref3 = get_refcount((IUnknown *)d3drm1);
    todo_wine ok(ref3 > ref1, "expected ref3 > ref1, got ref1 = %u , ref3 = %u.\n", ref1, ref3);
    ref3 = get_refcount((IUnknown *)d3drm2);
    ok(ref3 == ref2, "expected ref3 == ref2, got ref2 = %u , ref3 = %u.\n", ref2, ref3);
    device_ref2 = get_refcount((IUnknown *)d3ddevice2);
    todo_wine ok(device_ref2 > device_ref1, "Expected device_ref2 > device_ref1, got device_ref1 = %u, device_ref2 = %u.\n", device_ref1, device_ref2);

    hr = IDirectDraw_EnumSurfaces(ddraw1, DDENUMSURFACES_ALL | DDENUMSURFACES_DOESEXIST,
        NULL, &surface, surface_callback);
    ok(hr == DD_OK, "Failed to enumerate surfaces (hr = %x).\n", hr);
    ok(surface == NULL, "No primary surface should have enumerated (%p).\n", surface);

    hr = IDirect3DRMDevice2_GetDirect3DDevice2(device2, &d3drm_d3ddevice2);
    todo_wine ok(hr == D3DRM_OK, "Cannot get IDirect3DDevice2 interface (hr = %x).\n", hr);
    if (FAILED(hr))
        goto cleanup;
    ok(d3ddevice2 == d3drm_d3ddevice2, "Expected Immediate Mode deivce created == %p, got %p.\n", d3ddevice2, d3drm_d3ddevice2);

    /* Check properties of render target and depth surfaces */
    hr = IDirect3DDevice2_GetRenderTarget(d3drm_d3ddevice2, &surface);
    ok(hr == DD_OK, "Cannot get surface to the render target (hr = %x).\n", hr);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &desc);
    ok(hr == DD_OK, "Cannot get surface desc structure (hr = %x).\n", hr);

    ok((desc.dwWidth == rc.right) && (desc.dwHeight == rc.bottom), "Expected surface dimentions = %u, %u, got %u, %u.\n",
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

    ok((desc.dwWidth == rc.right) && (desc.dwHeight == rc.bottom), "Expected surface dimentions = %u, %u, got %u, %u.\n",
            rc.right, rc.bottom, desc.dwWidth, desc.dwHeight);
    ok((desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER) == DDSCAPS_ZBUFFER, "Expected caps containing %x, got %x.\n", DDSCAPS_ZBUFFER, desc.ddsCaps.dwCaps);
    expected_flags = DDSD_ZBUFFERBITDEPTH | DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH;
    ok(desc.dwFlags == expected_flags, "Expected %x for flags, got %x.\n", expected_flags, desc.dwFlags);

    IDirectDrawSurface_Release(d3drm_ds);
    IDirectDrawSurface_Release(ds);
    ds = NULL;
    IDirectDrawSurface_Release(surface);
    IDirect3DDevice2_Release(d3drm_d3ddevice2);
cleanup:
    if (ds)
        IDirectDrawSurface_Release(ds);

    IDirect3DRMDevice2_Release(device2);
    ref3 = get_refcount((IUnknown *)d3drm1);
    ok(ref1 == ref3, "expected ref1 == ref3, got ref1 = %u, ref3 = %u.\n", ref1, ref3);
    ref3 = get_refcount((IUnknown *)d3drm2);
    ok(ref3 == ref2, "expected ref3 == ref2, got ref2 = %u , ref3 = %u.\n", ref2, ref3);
    device_ref2 = get_refcount((IUnknown *)d3ddevice2);
    ok(device_ref2 == device_ref1, "Expected device_ref2 == device_ref1, got device_ref1 = %u, device_ref2 = %u.\n", device_ref1, device_ref2);

    IDirect3DRM2_Release(d3drm2);
    IDirect3DRM_Release(d3drm1);
    IDirect3DDevice2_Release(d3ddevice2);
    IDirect3D2_Release(d3d2);
    IDirectDraw2_Release(ddraw2);
    IDirectDraw_Release(ddraw1);
    DestroyWindow(window);
}

START_TEST(d3drm)
{
    test_MeshBuilder();
    test_MeshBuilder3();
    test_Mesh();
    test_Face();
    test_Frame();
    test_Device();
    test_Viewport();
    test_Light();
    test_Material2();
    test_Texture();
    test_frame_transform();
    test_d3drm_load();
    test_frame_mesh_materials();
    test_d3drm_qi();
    test_frame_qi();
    test_create_device_from_clipper();
    test_create_device_from_surface();
    test_create_device_from_d3d();
}
