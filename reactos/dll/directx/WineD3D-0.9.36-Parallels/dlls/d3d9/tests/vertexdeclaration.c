/*
 * Copyright (C) 2005 Henri Verbeet
 * Copyright (C) 2006 Ivan Gyurdiev
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
#include <d3d9.h>
#include "wine/test.h"

static HMODULE d3d9_handle = 0;

#define VDECL_CHECK(fcall) \
    if(fcall != S_OK) \
        trace(" Test failed on line #%d\n", __LINE__);

static HWND create_window(void)
{
    WNDCLASS wc = {0};
    wc.lpfnWndProc = &DefWindowProc;
    wc.lpszClassName = "d3d9_test_wc";
    RegisterClass(&wc);

    return CreateWindow("d3d9_test_wc", "d3d9_test",
            0, 0, 0, 0, 0, 0, 0, 0, 0);
}

static IDirect3DDevice9 *init_d3d9(void)
{
    IDirect3D9 * (__stdcall * d3d9_create)(UINT SDKVersion) = 0;
    IDirect3D9 *d3d9_ptr = 0;
    IDirect3DDevice9 *device_ptr = 0;
    D3DPRESENT_PARAMETERS present_parameters;
    HRESULT hres;

    d3d9_create = (void *)GetProcAddress(d3d9_handle, "Direct3DCreate9");
    ok(d3d9_create != NULL, "Failed to get address of Direct3DCreate9\n");
    if (!d3d9_create) return NULL;
    
    d3d9_ptr = d3d9_create(D3D_SDK_VERSION);
    ok(d3d9_ptr != NULL, "Failed to create IDirect3D9 object\n");
    if (!d3d9_ptr) return NULL;

    ZeroMemory(&present_parameters, sizeof(present_parameters));
    present_parameters.Windowed = TRUE;
    present_parameters.hDeviceWindow = create_window();
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;

    hres = IDirect3D9_CreateDevice(d3d9_ptr, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device_ptr);

    if(FAILED(hres))
    {
        trace("could not create device, IDirect3D9_CreateDevice returned %#x\n", hres);
        return NULL;
    }

    return device_ptr;
}

static int get_refcount(IUnknown *object)
{
    IUnknown_AddRef(object);
    return IUnknown_Release(object);
}

static inline void print_elements(
    D3DVERTEXELEMENT9 *elements) {

    D3DVERTEXELEMENT9 last = D3DDECL_END();
    D3DVERTEXELEMENT9 *ptr = elements;
    int count = 0;

    while (memcmp(ptr, &last, sizeof(D3DVERTEXELEMENT9))) {

        trace(
            "[Element %d] Stream = %d, Offset = %d, Type = %d, Method = %d, Usage = %d, UsageIndex = %d\n",
             count, ptr->Stream, ptr->Offset, ptr->Type, ptr->Method, ptr->Usage, ptr->UsageIndex);

        ptr++;
        count++;
    }
}

static int compare_elements(
    IDirect3DVertexDeclaration9 *decl,
    const D3DVERTEXELEMENT9 *expected_elements) {

    HRESULT hr;
    unsigned int i, size;
    D3DVERTEXELEMENT9 last = D3DDECL_END();
    D3DVERTEXELEMENT9 *elements = NULL;

    /* How many elements are there? */
    hr = IDirect3DVertexDeclaration9_GetDeclaration( decl, NULL, &size );
    ok(SUCCEEDED(hr), "GetDeclaration returned %#x, expected %#x\n", hr, D3D_OK);
    if (FAILED(hr)) goto fail;

    /* Allocate buffer */
    elements = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(D3DVERTEXELEMENT9) * size);
    ok (elements != NULL, "Out of memory, aborting test\n");
    if (elements == NULL) goto fail;

    /* Get the elements */
    hr = IDirect3DVertexDeclaration9_GetDeclaration( decl, elements, &size);
    ok(SUCCEEDED(hr), "GetDeclaration returned %#x, expected %#x\n", hr, D3D_OK);
    if (FAILED(hr)) goto fail;

    /* Compare one by one */
    for (i = 0; i < size; i++) {

        int status;

        int end1 = memcmp(&elements[i], &last, sizeof(D3DVERTEXELEMENT9));
        int end2 = memcmp(&expected_elements[i], &last, sizeof(D3DVERTEXELEMENT9));
        status = ((end1 && !end2) || (!end1 && end2));
        ok (!status, "Mismatch in size, test declaration is %s than expected\n",
            (end1 && !end2) ? "shorter" : "longer");
        if (status) { print_elements(elements); goto fail; }

        status = memcmp(&elements[i], &expected_elements[i], sizeof(D3DVERTEXELEMENT9));
        ok (!status, "Mismatch in element %d\n", i);
        if (status) { print_elements(elements); goto fail; }
    }

    HeapFree(GetProcessHeap(), 0, elements);
    return S_OK;

    fail:
    HeapFree(GetProcessHeap(), 0, elements);
    return E_FAIL;
}

static IDirect3DVertexDeclaration9 *test_create_vertex_declaration(IDirect3DDevice9 *device_ptr, D3DVERTEXELEMENT9 *vertex_decl)
{
    IDirect3DVertexDeclaration9 *decl_ptr = 0;
    HRESULT hret = 0;

    hret = IDirect3DDevice9_CreateVertexDeclaration(device_ptr, vertex_decl, &decl_ptr);
    ok(hret == D3D_OK && decl_ptr != NULL, "CreateVertexDeclaration returned: hret 0x%x, decl_ptr %p. "
        "Expected hret 0x%x, decl_ptr != %p. Aborting.\n", hret, decl_ptr, D3D_OK, NULL);

    return decl_ptr;
}

static void test_get_set_vertex_declaration(IDirect3DDevice9 *device_ptr, IDirect3DVertexDeclaration9 *decl_ptr)
{
    IDirect3DVertexDeclaration9 *current_decl_ptr = 0;
    HRESULT hret = 0;
    int decl_refcount = 0;
    int i = 0;
    
    /* SetVertexDeclaration should not touch the declaration's refcount. */
    i = get_refcount((IUnknown *)decl_ptr);
    hret = IDirect3DDevice9_SetVertexDeclaration(device_ptr, decl_ptr);
    decl_refcount = get_refcount((IUnknown *)decl_ptr);
    ok(hret == D3D_OK && decl_refcount == i, "SetVertexDeclaration returned: hret 0x%x, refcount %d. "
        "Expected hret 0x%x, refcount %d.\n", hret, decl_refcount, D3D_OK, i);
    
    /* GetVertexDeclaration should increase the declaration's refcount by one. */
    i = decl_refcount+1;
    hret = IDirect3DDevice9_GetVertexDeclaration(device_ptr, &current_decl_ptr);
    decl_refcount = get_refcount((IUnknown *)decl_ptr);
    ok(hret == D3D_OK && decl_refcount == i && current_decl_ptr == decl_ptr, 
        "GetVertexDeclaration returned: hret 0x%x, current_decl_ptr %p refcount %d. "
        "Expected hret 0x%x, current_decl_ptr %p, refcount %d.\n", hret, current_decl_ptr, decl_refcount, D3D_OK, decl_ptr, i);
}

static void test_get_declaration(IDirect3DVertexDeclaration9 *decl_ptr, D3DVERTEXELEMENT9 *vertex_decl, UINT expected_num_elements)
{
    int i;
    UINT num_elements = 0;
    D3DVERTEXELEMENT9 *decl = 0;
    HRESULT hret = 0;

    /* First test only getting the number of elements */
    num_elements = 0x1337c0de;
    hret = IDirect3DVertexDeclaration9_GetDeclaration(decl_ptr, NULL, &num_elements);
    ok(hret == D3D_OK && num_elements == expected_num_elements,
            "GetDeclaration returned: hret 0x%x, num_elements %d. "
            "Expected hret 0x%x, num_elements %d.\n", hret, num_elements, D3D_OK, expected_num_elements);

    num_elements = 0;
    hret = IDirect3DVertexDeclaration9_GetDeclaration(decl_ptr, NULL, &num_elements);
    ok(hret == D3D_OK && num_elements == expected_num_elements,
            "GetDeclaration returned: hret 0x%x, num_elements %d. "
            "Expected hret 0x%x, num_elements %d.\n", hret, num_elements, D3D_OK, expected_num_elements);

    /* Also test the returned data */
    decl = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(D3DVERTEXELEMENT9) * expected_num_elements);

    num_elements = 0x1337c0de;
    hret = IDirect3DVertexDeclaration9_GetDeclaration(decl_ptr, decl, &num_elements);
    ok(hret == D3D_OK && num_elements == expected_num_elements,
            "GetDeclaration returned: hret 0x%x, num_elements %d. "
            "Expected hret 0x%x, num_elements %d.\n", hret, num_elements, D3D_OK, expected_num_elements);
    i = memcmp(decl, vertex_decl, sizeof(vertex_decl));
    ok (!i, "Original and returned vertexdeclarations are not the same\n");
    ZeroMemory(decl, sizeof(D3DVERTEXELEMENT9) * expected_num_elements);

    num_elements = 0;
    hret = IDirect3DVertexDeclaration9_GetDeclaration(decl_ptr, decl, &num_elements);
    ok(hret == D3D_OK && num_elements == expected_num_elements,
            "GetDeclaration returned: hret 0x%x, num_elements %d. "
            "Expected hret 0x%x, num_elements %d.\n", hret, num_elements, D3D_OK, expected_num_elements);
    i = memcmp(decl, vertex_decl, sizeof(vertex_decl));
    ok (!i, "Original and returned vertexdeclarations are not the same\n");

    HeapFree(GetProcessHeap(), 0, decl);
}

/* FIXME: also write a test, which shows that attempting to set
 * an invalid vertex declaration returns E_FAIL */

static HRESULT test_fvf_to_decl(
    IDirect3DDevice9* device,
    IDirect3DVertexDeclaration9* default_decl,
    DWORD test_fvf,
    const D3DVERTEXELEMENT9 expected_elements[],
    char object_should_change) 
{

    HRESULT hr;
    IDirect3DVertexDeclaration9 *result_decl = NULL;

    /* Set a default declaration to make sure it is changed */
    hr = IDirect3DDevice9_SetVertexDeclaration ( device, default_decl );
    ok (SUCCEEDED(hr), "SetVertexDeclaration returned %#x, expected %#x\n", hr, D3D_OK);
    if (FAILED(hr)) goto fail;

    /* Set an FVF */
    hr = IDirect3DDevice9_SetFVF( device, test_fvf);
    ok(SUCCEEDED(hr), "SetFVF returned %#x, expected %#x\n", hr, D3D_OK);
    if (FAILED(hr)) goto fail;

    /* Check if the declaration object changed underneath */
    hr = IDirect3DDevice9_GetVertexDeclaration ( device, &result_decl);
    ok(SUCCEEDED(hr), "GetVertexDeclaration returned %#x, expected %#x\n", hr, D3D_OK);
    if (FAILED(hr)) goto fail;
    if (object_should_change) {
       ok(result_decl != default_decl, "result declaration matches original\n");
       if (result_decl == default_decl) goto fail;
    } else {
       ok(result_decl == default_decl, "result declaration does not match original\n");
       if (result_decl != default_decl) goto fail;
    }

    /* Declaration content/size test */
    ok(result_decl != NULL, "result declaration was null\n");
    if (result_decl == NULL) 
        goto fail;
    else if (compare_elements(result_decl, expected_elements) != S_OK)
        goto fail;

    if (result_decl) IUnknown_Release( result_decl );
    return S_OK;    

    fail:
    if (result_decl) IUnknown_Release( result_decl );
    return E_FAIL;
}

static HRESULT test_decl_to_fvf(
    IDirect3DDevice9* device,
    DWORD default_fvf,
    CONST D3DVERTEXELEMENT9 test_decl[],
    DWORD test_fvf)
{

    HRESULT hr;
    IDirect3DVertexDeclaration9 *vdecl = NULL;

    DWORD result_fvf = 0xdeadbeef;

    /* Set a default FVF of SPECULAR and DIFFUSE to make sure it is changed back to 0 */
    hr = IDirect3DDevice9_SetFVF( device, default_fvf);
    ok(SUCCEEDED(hr), "SetFVF returned %#x, expected %#x\n", hr, D3D_OK);
    if (FAILED(hr)) goto fail;

    /* Create a testing declaration */
    hr = IDirect3DDevice9_CreateVertexDeclaration( device, test_decl, &vdecl );
    ok(SUCCEEDED(hr), "CreateVertexDeclaration returned %#x, expected %#x\n", hr, D3D_OK);
    if (FAILED(hr)) goto fail;

    /* Set the declaration */
    hr = IDirect3DDevice9_SetVertexDeclaration ( device, vdecl );
    ok (SUCCEEDED(hr), "SetVertexDeclaration returned %#x, expected %#x\n", hr, D3D_OK);
    if (FAILED(hr)) goto fail;

    /* Check the FVF */
    hr = IDirect3DDevice9_GetFVF( device, &result_fvf);
    ok(SUCCEEDED(hr), "GetFVF returned %#x, expected %#x\n", hr, D3D_OK);
    if (FAILED(hr)) goto fail;
    todo_wine {
       ok(test_fvf == result_fvf, "result FVF was: %#x, expected: %#x\n", result_fvf, test_fvf);
    }
    if (test_fvf != result_fvf) goto fail;

    IDirect3DDevice9_SetVertexDeclaration ( device, NULL );
    if (vdecl) IUnknown_Release( vdecl );
    return S_OK;

    fail:
    IDirect3DDevice9_SetVertexDeclaration ( device, NULL );
    if (vdecl) IUnknown_Release( vdecl );
    return E_FAIL;
}

static void test_fvf_decl_conversion(IDirect3DDevice9 *pDevice)
{

    HRESULT hr;
    unsigned int i;

    IDirect3DVertexDeclaration9* default_decl = NULL;
    DWORD default_fvf = D3DFVF_SPECULAR | D3DFVF_DIFFUSE;
    D3DVERTEXELEMENT9 default_elements[] =
        { { 0, 0, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 0 },
          { 0, 4, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 1 }, D3DDECL_END() };

    /* Create a default declaration and FVF that does not match any of the tests */
    hr = IDirect3DDevice9_CreateVertexDeclaration( pDevice, default_elements, &default_decl );
    ok(SUCCEEDED(hr), "CreateVertexDeclaration returned %#x, expected %#x\n", hr, D3D_OK);
    if (FAILED(hr)) goto cleanup;

    /* Test conversions from vertex declaration to an FVF.
     * For some reason those seem to occur only for POSITION/POSITIONT,
     * Otherwise the FVF is forced to 0 - maybe this is configuration specific */
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0 }, D3DDECL_END() };
        VDECL_CHECK(test_decl_to_fvf(pDevice, default_fvf, test_buffer, D3DFVF_XYZ));
    }
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_POSITIONT, 0 }, D3DDECL_END() };
        VDECL_CHECK(test_decl_to_fvf(pDevice, default_fvf, test_buffer, D3DFVF_XYZRHW));
    }
    for (i = 0; i < 4; i++) {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT1+i, 0, D3DDECLUSAGE_BLENDWEIGHT, 0}, D3DDECL_END() };
        VDECL_CHECK(test_decl_to_fvf(pDevice, default_fvf, test_buffer, 0));
    }
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] = 
            { { 0, 0, D3DDECLTYPE_UBYTE4, 0, D3DDECLUSAGE_BLENDINDICES, 0}, D3DDECL_END() };
        VDECL_CHECK(test_decl_to_fvf(pDevice, default_fvf, test_buffer, 0));
    }
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_NORMAL, 0 }, D3DDECL_END() };
        VDECL_CHECK(test_decl_to_fvf(pDevice, default_fvf, test_buffer, 0));
    }
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_PSIZE, 0 }, D3DDECL_END() };
        VDECL_CHECK(test_decl_to_fvf(pDevice, default_fvf, test_buffer, 0));
    }
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 0 }, D3DDECL_END() };
        VDECL_CHECK(test_decl_to_fvf(pDevice, default_fvf, test_buffer, 0));
    }
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 1 }, D3DDECL_END() };
        VDECL_CHECK(test_decl_to_fvf(pDevice, default_fvf, test_buffer, 0));
    }

    /* Make sure textures of different sizes work */
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_TEXCOORD, 0 }, D3DDECL_END() };
        VDECL_CHECK(test_decl_to_fvf(pDevice, default_fvf, test_buffer, 0));
    }
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_TEXCOORD, 0 }, D3DDECL_END() };
        VDECL_CHECK(test_decl_to_fvf(pDevice, default_fvf, test_buffer, 0));
    }
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_TEXCOORD, 0 }, D3DDECL_END() };
        VDECL_CHECK(test_decl_to_fvf(pDevice, default_fvf, test_buffer, 0));
    }
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_TEXCOORD, 0 }, D3DDECL_END() };
        VDECL_CHECK(test_decl_to_fvf(pDevice, default_fvf, test_buffer, 0));
    }

    /* Make sure the TEXCOORD index works correctly - try several textures */
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_TEXCOORD, 0 },
              { 0, 4, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_TEXCOORD, 1 },
              { 0, 16, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_TEXCOORD, 2 },
              { 0, 24, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_TEXCOORD, 3 }, D3DDECL_END() };
        VDECL_CHECK(test_decl_to_fvf(pDevice, default_fvf, test_buffer, 0));
    }

    /* No FVF mapping available */
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 1 }, D3DDECL_END() };
        VDECL_CHECK(test_decl_to_fvf(pDevice, default_fvf, test_buffer, 0));
    }
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_NORMAL, 1 }, D3DDECL_END() };
        VDECL_CHECK(test_decl_to_fvf(pDevice, default_fvf, test_buffer, 0));
    }

    /* Try empty declaration */
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] = { D3DDECL_END() };
        VDECL_CHECK(test_decl_to_fvf(pDevice, default_fvf, test_buffer, 0));
    }

    /* Now try a combination test */
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITIONT, 0 },
              { 0, 12, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_NORMAL, 0 },
              { 0, 24, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_PSIZE, 0 },
              { 0, 28, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 1 },
              { 0, 32, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_TEXCOORD, 0 },
              { 0, 44, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_TEXCOORD, 1 }, D3DDECL_END() };
        VDECL_CHECK(test_decl_to_fvf(pDevice, default_fvf, test_buffer, 0));
    }

    /* Test conversions from FVF to a vertex declaration 
     * These seem to always occur internally. A new declaration object is created if necessary */

    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0 }, D3DDECL_END() };
        VDECL_CHECK(test_fvf_to_decl(pDevice, default_decl, D3DFVF_XYZ, test_buffer, 1));
    }
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
          { { 0, 0, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_POSITIONT, 0 }, D3DDECL_END() };
        VDECL_CHECK(test_fvf_to_decl(pDevice, default_decl, D3DFVF_XYZRHW, test_buffer, 1));
    }
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0 },
              { 0, 12, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_BLENDWEIGHT, 0 },
              { 0, 28, D3DDECLTYPE_UBYTE4, 0, D3DDECLUSAGE_BLENDINDICES, 0 }, D3DDECL_END() };
        VDECL_CHECK(test_fvf_to_decl(pDevice, default_decl,
            D3DFVF_XYZB5 | D3DFVF_LASTBETA_UBYTE4, test_buffer, 1));
    }
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0 },
              { 0, 12, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_BLENDWEIGHT, 0 },
              { 0, 28, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_BLENDINDICES, 0 }, D3DDECL_END() };
        VDECL_CHECK(test_fvf_to_decl(pDevice, default_decl,
           D3DFVF_XYZB5 | D3DFVF_LASTBETA_D3DCOLOR, test_buffer, 1));
    }
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0 },
              { 0, 12, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_BLENDWEIGHT, 0 },
              { 0, 28, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_BLENDINDICES, 0 }, D3DDECL_END() };
        VDECL_CHECK(test_fvf_to_decl(pDevice, default_decl, D3DFVF_XYZB5, test_buffer, 1));
    }
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0 },
              { 0, 12, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_BLENDWEIGHT, 0 }, D3DDECL_END() };
        VDECL_CHECK(test_fvf_to_decl(pDevice, default_decl, D3DFVF_XYZB1, test_buffer, 1));
    }
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0 },
              { 0, 12, D3DDECLTYPE_UBYTE4, 0, D3DDECLUSAGE_BLENDINDICES, 0 }, D3DDECL_END() };
        VDECL_CHECK(test_fvf_to_decl(pDevice, default_decl,
            D3DFVF_XYZB1 | D3DFVF_LASTBETA_UBYTE4, test_buffer, 1));
    }
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0 },
              { 0, 12, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_BLENDINDICES, 0 }, D3DDECL_END() };
        VDECL_CHECK(test_fvf_to_decl(pDevice, default_decl,
            D3DFVF_XYZB1 | D3DFVF_LASTBETA_D3DCOLOR, test_buffer, 1));
    }
    {
         CONST D3DVERTEXELEMENT9 test_buffer[] =
             { { 0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0 },
               { 0, 12, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_BLENDWEIGHT, 0 }, D3DDECL_END() };
         VDECL_CHECK(test_fvf_to_decl(pDevice, default_decl, D3DFVF_XYZB2, test_buffer, 1));
    }
    {
         CONST D3DVERTEXELEMENT9 test_buffer[] =
             { { 0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0 },
               { 0, 12, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_BLENDWEIGHT, 0 },
               { 0, 16, D3DDECLTYPE_UBYTE4, 0, D3DDECLUSAGE_BLENDINDICES, 0 }, D3DDECL_END() };
         VDECL_CHECK(test_fvf_to_decl(pDevice, default_decl,
             D3DFVF_XYZB2 | D3DFVF_LASTBETA_UBYTE4, test_buffer, 1));
     }
     {
         CONST D3DVERTEXELEMENT9 test_buffer[] =
             { { 0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0 },
               { 0, 12, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_BLENDWEIGHT, 0 },
               { 0, 16, D3DDECLTYPE_UBYTE4, 0, D3DDECLUSAGE_BLENDINDICES, 0 }, D3DDECL_END() };
         VDECL_CHECK(test_fvf_to_decl(pDevice, default_decl,
             D3DFVF_XYZB2 | D3DFVF_LASTBETA_D3DCOLOR, test_buffer, 1));
     }
     {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0 },
              { 0, 12, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_BLENDWEIGHT, 0 }, D3DDECL_END() };
        VDECL_CHECK(test_fvf_to_decl(pDevice, default_decl, D3DFVF_XYZB3, test_buffer, 1));
    }
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0 },
              { 0, 12, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_BLENDWEIGHT, 0 },
              { 0, 20, D3DDECLTYPE_UBYTE4, 0, D3DDECLUSAGE_BLENDINDICES, 0 }, D3DDECL_END() };
        VDECL_CHECK(test_fvf_to_decl(pDevice, default_decl,
            D3DFVF_XYZB3 | D3DFVF_LASTBETA_UBYTE4, test_buffer, 1));
    }
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0 },
              { 0, 12, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_BLENDWEIGHT, 0 },
              { 0, 20, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_BLENDINDICES, 0 }, D3DDECL_END() };
        VDECL_CHECK(test_fvf_to_decl(pDevice, default_decl,
            D3DFVF_XYZB3 | D3DFVF_LASTBETA_D3DCOLOR, test_buffer, 1));
    }
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0 },
              { 0, 12, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_BLENDWEIGHT, 0 }, D3DDECL_END() };
        VDECL_CHECK(test_fvf_to_decl(pDevice, default_decl, D3DFVF_XYZB4, test_buffer, 1));
    }
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0 },
              { 0, 12, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_BLENDWEIGHT, 0 },
              { 0, 24, D3DDECLTYPE_UBYTE4, 0, D3DDECLUSAGE_BLENDINDICES, 0 }, D3DDECL_END() };
        VDECL_CHECK(test_fvf_to_decl(pDevice, default_decl,
            D3DFVF_XYZB4 | D3DFVF_LASTBETA_UBYTE4, test_buffer, 1));
    }
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0 },
              { 0, 12, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_BLENDWEIGHT, 0 },
              { 0, 24, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_BLENDINDICES, 0 }, D3DDECL_END() };
        VDECL_CHECK(test_fvf_to_decl(pDevice, default_decl,
            D3DFVF_XYZB4 | D3DFVF_LASTBETA_D3DCOLOR, test_buffer, 1));
    }
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_NORMAL, 0 }, D3DDECL_END() };
        VDECL_CHECK(test_fvf_to_decl(pDevice, default_decl, D3DFVF_NORMAL, test_buffer, 1));
    }
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_PSIZE, 0 }, D3DDECL_END() };
        VDECL_CHECK(test_fvf_to_decl(pDevice, default_decl, D3DFVF_PSIZE, test_buffer, 1));
    }
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 0 }, D3DDECL_END() };
        VDECL_CHECK(test_fvf_to_decl(pDevice, default_decl, D3DFVF_DIFFUSE, test_buffer, 1));
    }
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 1 }, D3DDECL_END() };
        VDECL_CHECK(test_fvf_to_decl(pDevice, default_decl, D3DFVF_SPECULAR, test_buffer, 1));
    }

    /* Make sure textures of different sizes work */
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_TEXCOORD, 0 }, D3DDECL_END() };
        VDECL_CHECK(test_fvf_to_decl(pDevice, default_decl,
            D3DFVF_TEXCOORDSIZE1(0) | D3DFVF_TEX1, test_buffer, 1));
    }
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_TEXCOORD, 0 }, D3DDECL_END() };
        VDECL_CHECK(test_fvf_to_decl(pDevice, default_decl,
            D3DFVF_TEXCOORDSIZE2(0) | D3DFVF_TEX1, test_buffer, 1));
    }
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_TEXCOORD, 0 }, D3DDECL_END() };
        VDECL_CHECK(test_fvf_to_decl(pDevice, default_decl,
            D3DFVF_TEXCOORDSIZE3(0) | D3DFVF_TEX1, test_buffer, 1));
    }
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_TEXCOORD, 0 }, D3DDECL_END() };
        VDECL_CHECK(test_fvf_to_decl(pDevice, default_decl,
            D3DFVF_TEXCOORDSIZE4(0) | D3DFVF_TEX1, test_buffer, 1));
    }

    /* Make sure the TEXCOORD index works correctly - try several textures */
    {
        CONST D3DVERTEXELEMENT9 test_buffer[] =
            { { 0, 0, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_TEXCOORD, 0 },
              { 0, 4, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_TEXCOORD, 1 },
              { 0, 16, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_TEXCOORD, 2 },
              { 0, 24, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_TEXCOORD, 3 }, D3DDECL_END() };
        VDECL_CHECK(test_fvf_to_decl(pDevice, default_decl,
            D3DFVF_TEXCOORDSIZE1(0) | D3DFVF_TEXCOORDSIZE3(1) | D3DFVF_TEXCOORDSIZE2(2) |
            D3DFVF_TEXCOORDSIZE4(3) | D3DFVF_TEX4, test_buffer, 1));
    }

    /* Now try a combination test  */
    {
       CONST D3DVERTEXELEMENT9 test_buffer[] =
                { { 0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0 },
                  { 0, 12, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_BLENDWEIGHT, 0 },
                  { 0, 28, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 0 },
                  { 0, 32, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 1 },
                  { 0, 36, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_TEXCOORD, 0 },
                  { 0, 44, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_TEXCOORD, 1 }, D3DDECL_END() };
       VDECL_CHECK(test_fvf_to_decl(pDevice, default_decl, D3DFVF_XYZB4 | D3DFVF_SPECULAR | D3DFVF_DIFFUSE |
           D3DFVF_TEXCOORDSIZE2(0) | D3DFVF_TEXCOORDSIZE3(1) | D3DFVF_TEX2, test_buffer, 1));
    }

    /* Setting the FVF to 0 should result in no change to the default decl */
    VDECL_CHECK(test_fvf_to_decl(pDevice, default_decl, 0, default_elements, 0));

    cleanup:
    IDirect3DDevice9_SetVertexDeclaration ( pDevice, NULL );
    if ( default_decl ) IUnknown_Release (default_decl);
}

/* Check whether a declaration converted from FVF is shared.
 * Check whether refcounts behave as expected */
static void test_fvf_decl_management(
    IDirect3DDevice9* device) {

    HRESULT hr;
    IDirect3DVertexDeclaration9* result_decl1 = NULL;
    IDirect3DVertexDeclaration9* result_decl2 = NULL;
    IDirect3DVertexDeclaration9* result_decl3 = NULL;
    IDirect3DVertexDeclaration9* result_decl4 = NULL;
    int ref1, ref2, ref3, ref4;

    DWORD test_fvf1 = D3DFVF_XYZRHW;
    DWORD test_fvf2 = D3DFVF_NORMAL;
    CONST D3DVERTEXELEMENT9 test_elements1[] =
        { { 0, 0, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_POSITIONT, 0 }, D3DDECL_END() };
    CONST D3DVERTEXELEMENT9 test_elements2[] =
        { { 0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_NORMAL, 0 }, D3DDECL_END() };

    /* Clear down any current vertex declaration */
    hr = IDirect3DDevice9_SetVertexDeclaration ( device, NULL );
    ok (SUCCEEDED(hr), "SetVertexDeclaration returned %#x, expected %#x\n", hr, D3D_OK);
    if (FAILED(hr)) return;

    /* Conversion */
    hr = IDirect3DDevice9_SetFVF( device, test_fvf1);
    ok(SUCCEEDED(hr), "SetFVF returned %#x, expected %#x\n", hr, D3D_OK);
    if (FAILED(hr)) return;

    /* Get converted decl (#1) */
    hr = IDirect3DDevice9_GetVertexDeclaration ( device, &result_decl1);
    ok(SUCCEEDED(hr), "GetVertexDeclaration returned %#x, expected %#x\n", hr, D3D_OK);
    if (FAILED(hr)) return;

    /* Get converted decl again (#2) */
    hr = IDirect3DDevice9_GetVertexDeclaration ( device, &result_decl2);
    ok(SUCCEEDED(hr), "GetVertexDeclaration returned %#x, expected %#x\n", hr, D3D_OK);
    if (FAILED(hr)) return;

    /* Conversion */
    hr = IDirect3DDevice9_SetFVF( device, test_fvf2);
    ok(SUCCEEDED(hr), "SetFVF returned %#x, expected %#x\n", hr, D3D_OK);
    if (FAILED(hr)) return;

    /* The contents should correspond to the first conversion */
    VDECL_CHECK(compare_elements(result_decl1, test_elements1));

    /* Get converted decl (#3) */
    hr = IDirect3DDevice9_GetVertexDeclaration ( device, &result_decl3);
    ok(SUCCEEDED(hr), "GetVertexDeclaration returned %#x, expected %#x\n", hr, D3D_OK);
    if (FAILED(hr)) return;

    /* The object should be the same */
    ok (result_decl1 == result_decl2, "Declaration object changes on the second Get() call\n");
    ok (result_decl2 != result_decl3, "Declaration object did not change during conversion\n");

    /* The contents should correspond to the second conversion */
    VDECL_CHECK(compare_elements(result_decl3, test_elements2));
    /* Re-Check if the first decl was overwritten by the new Get() */
    VDECL_CHECK(compare_elements(result_decl1, test_elements1));

    hr = IDirect3DDevice9_SetFVF( device, test_fvf1);
    ok(SUCCEEDED(hr), "SetFVF returned %#x, expected %#x\n", hr, D3D_OK);
    if (FAILED(hr)) return;

    hr = IDirect3DDevice9_GetVertexDeclaration ( device, &result_decl4);
    ok(SUCCEEDED(hr), "GetVertexDeclaration returned %#x, expected %#x\n", hr, D3D_OK);
    if (FAILED(hr)) return;

    ok(result_decl4 == result_decl1, "Setting an already used FVF over results in a different vertexdeclaration\n");

    ref1 = get_refcount((IUnknown*) result_decl1);
    ref2 = get_refcount((IUnknown*) result_decl2);
    ref3 = get_refcount((IUnknown*) result_decl3);
    ref4 = get_refcount((IUnknown*) result_decl4);
    ok (ref1 == 3, "Refcount #1 is %d, expected 3\n", ref1);
    ok (ref2 == 3, "Refcount #2 is %d, expected 3\n", ref2);
    ok (ref3 == 1, "Refcount #3 is %d, expected 1\n", ref3);
    ok (ref4 == 3, "Refcount #4 is %d, expected 3\n", ref4);

    /* Clear down any current vertex declaration */
    hr = IDirect3DDevice9_SetVertexDeclaration ( device, NULL );
    ok (SUCCEEDED(hr), "SetVertexDeclaration returned %#x, expected %#x\n", hr, D3D_OK);
    if (FAILED(hr)) return;

    IDirect3DVertexDeclaration9_Release(result_decl1);
    IDirect3DVertexDeclaration9_Release(result_decl2);
    IDirect3DVertexDeclaration9_Release(result_decl3);
    IDirect3DVertexDeclaration9_Release(result_decl4);

    return;
}

START_TEST(vertexdeclaration)
{
    static D3DVERTEXELEMENT9 simple_decl[] = {
        { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        D3DDECL_END()};
    UINT simple_decl_num_elements = sizeof(simple_decl) / sizeof(*simple_decl);
    IDirect3DDevice9 *device_ptr = 0;
    IDirect3DVertexDeclaration9 *decl_ptr = 0;

    d3d9_handle = LoadLibraryA("d3d9.dll");
    if (!d3d9_handle)
    {
        skip("Could not load d3d9.dll\n");
        return;
    }

    device_ptr = init_d3d9();
    if (!device_ptr)
    {
        skip("Failed to initialise d3d9\n");
        return;
    }

    decl_ptr = test_create_vertex_declaration(device_ptr, simple_decl);
    if (!decl_ptr)
    {
        skip("Failed to create a vertex declaration\n");
        return;
    }

    test_get_set_vertex_declaration(device_ptr, decl_ptr);
    test_get_declaration(decl_ptr, simple_decl, simple_decl_num_elements);
    test_fvf_decl_conversion(device_ptr);
    test_fvf_decl_management(device_ptr);
}
