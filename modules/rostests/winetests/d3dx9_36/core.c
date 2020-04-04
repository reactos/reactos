/*
 * Tests for the D3DX9 core interfaces
 *
 * Copyright 2009 Tony Wasserka
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
#include <dxerr9.h>
#include "d3dx9core.h"

static inline int get_ref(IUnknown *obj)
{
    IUnknown_AddRef(obj);
    return IUnknown_Release(obj);
}

#define check_ref(obj, exp) _check_ref(__LINE__, obj, exp)
static inline void _check_ref(unsigned int line, IUnknown *obj, int exp)
{
    int ref = get_ref(obj);
    ok_(__FILE__, line)(exp == ref, "Invalid refcount. Expected %d, got %d\n", exp, ref);
}

#define check_release(obj, exp) _check_release(__LINE__, obj, exp)
static inline void _check_release(unsigned int line, IUnknown *obj, int exp)
{
    int ref = IUnknown_Release(obj);
    ok_(__FILE__, line)(ref == exp, "Invalid refcount. Expected %d, got %d\n", exp, ref);
}

#define admitted_error 0.0001f
static inline void check_mat(D3DXMATRIX got, D3DXMATRIX exp)
{
    int i, j, equal=1;
    for (i=0; i<4; i++)
        for (j=0; j<4; j++)
            if (fabs(U(exp).m[i][j]-U(got).m[i][j]) > admitted_error)
                equal=0;

    ok(equal, "Got matrix\n\t(%f,%f,%f,%f\n\t %f,%f,%f,%f\n\t %f,%f,%f,%f\n\t %f,%f,%f,%f)\n"
       "Expected matrix=\n\t(%f,%f,%f,%f\n\t %f,%f,%f,%f\n\t %f,%f,%f,%f\n\t %f,%f,%f,%f)\n",
       U(got).m[0][0],U(got).m[0][1],U(got).m[0][2],U(got).m[0][3],
       U(got).m[1][0],U(got).m[1][1],U(got).m[1][2],U(got).m[1][3],
       U(got).m[2][0],U(got).m[2][1],U(got).m[2][2],U(got).m[2][3],
       U(got).m[3][0],U(got).m[3][1],U(got).m[3][2],U(got).m[3][3],
       U(exp).m[0][0],U(exp).m[0][1],U(exp).m[0][2],U(exp).m[0][3],
       U(exp).m[1][0],U(exp).m[1][1],U(exp).m[1][2],U(exp).m[1][3],
       U(exp).m[2][0],U(exp).m[2][1],U(exp).m[2][2],U(exp).m[2][3],
       U(exp).m[3][0],U(exp).m[3][1],U(exp).m[3][2],U(exp).m[3][3]);
}

static void test_ID3DXBuffer(void)
{
    ID3DXBuffer *buffer;
    HRESULT hr;
    ULONG count;
    DWORD size;

    hr = D3DXCreateBuffer(10, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateBuffer failed, got %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateBuffer(0, &buffer);
    ok(hr == D3D_OK, "D3DXCreateBuffer failed, got %#x, expected %#x\n", hr, D3D_OK);

    size = ID3DXBuffer_GetBufferSize(buffer);
    ok(!size, "GetBufferSize failed, got %u, expected %u\n", size, 0);

    count = ID3DXBuffer_Release(buffer);
    ok(!count, "ID3DXBuffer has %u references left\n", count);

    hr = D3DXCreateBuffer(3, &buffer);
    ok(hr == D3D_OK, "D3DXCreateBuffer failed, got %#x, expected %#x\n", hr, D3D_OK);

    size = ID3DXBuffer_GetBufferSize(buffer);
    ok(size == 3, "GetBufferSize failed, got %u, expected %u\n", size, 3);

    count = ID3DXBuffer_Release(buffer);
    ok(!count, "ID3DXBuffer has %u references left\n", count);
}

static void test_ID3DXSprite(IDirect3DDevice9 *device)
{
    ID3DXSprite *sprite;
    IDirect3D9 *d3d;
    IDirect3DDevice9 *cmpdev;
    IDirect3DTexture9 *tex1, *tex2;
    D3DXMATRIX mat, cmpmat;
    D3DVIEWPORT9 vp;
    RECT rect;
    D3DXVECTOR3 pos, center;
    HRESULT hr;

    IDirect3DDevice9_GetDirect3D(device, &d3d);
    hr = IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, D3DUSAGE_DYNAMIC, D3DRTYPE_TEXTURE, D3DFMT_A8R8G8B8);
    IDirect3D9_Release(d3d);
    ok (hr == D3D_OK, "D3DFMT_A8R8G8B8 not supported\n");
    if (FAILED(hr)) return;

    hr = IDirect3DDevice9_CreateTexture(device, 64, 64, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex1, NULL);
    ok (hr == D3D_OK, "Failed to create first texture (error code: %#x)\n", hr);
    if (FAILED(hr)) return;

    hr = IDirect3DDevice9_CreateTexture(device, 32, 32, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex2, NULL);
    ok (hr == D3D_OK, "Failed to create second texture (error code: %#x)\n", hr);
    if (FAILED(hr)) {
        IDirect3DTexture9_Release(tex1);
        return;
    }

    /* Test D3DXCreateSprite */
    hr = D3DXCreateSprite(device, NULL);
    ok (hr == D3DERR_INVALIDCALL, "D3DXCreateSprite returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateSprite(NULL, &sprite);
    ok (hr == D3DERR_INVALIDCALL, "D3DXCreateSprite returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateSprite(device, &sprite);
    ok (hr == D3D_OK, "D3DXCreateSprite returned %#x, expected %#x\n", hr, D3D_OK);


    /* Test ID3DXSprite_GetDevice */
    hr = ID3DXSprite_GetDevice(sprite, NULL);
    ok (hr == D3DERR_INVALIDCALL, "GetDevice returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = ID3DXSprite_GetDevice(sprite, &cmpdev);  /* cmpdev == NULL */
    ok (hr == D3D_OK, "GetDevice returned %#x, expected %#x\n", hr, D3D_OK);

    hr = ID3DXSprite_GetDevice(sprite, &cmpdev);  /* cmpdev != NULL */
    ok (hr == D3D_OK, "GetDevice returned %#x, expected %#x\n", hr, D3D_OK);

    IDirect3DDevice9_Release(device);
    IDirect3DDevice9_Release(device);


    /* Test ID3DXSprite_GetTransform */
    hr = ID3DXSprite_GetTransform(sprite, NULL);
    ok (hr == D3DERR_INVALIDCALL, "GetTransform returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);
    hr = ID3DXSprite_GetTransform(sprite, &mat);
    ok (hr == D3D_OK, "GetTransform returned %#x, expected %#x\n", hr, D3D_OK);
    if(SUCCEEDED(hr)) {
        D3DXMATRIX identity;
        D3DXMatrixIdentity(&identity);
        check_mat(mat, identity);
    }

    /* Test ID3DXSprite_SetTransform */
    /* Set a transform and test if it gets returned correctly */
    U(mat).m[0][0]=2.1f;  U(mat).m[0][1]=6.5f;  U(mat).m[0][2]=-9.6f; U(mat).m[0][3]=1.7f;
    U(mat).m[1][0]=4.2f;  U(mat).m[1][1]=-2.5f; U(mat).m[1][2]=2.1f;  U(mat).m[1][3]=5.5f;
    U(mat).m[2][0]=-2.6f; U(mat).m[2][1]=0.3f;  U(mat).m[2][2]=8.6f;  U(mat).m[2][3]=8.4f;
    U(mat).m[3][0]=6.7f;  U(mat).m[3][1]=-5.1f; U(mat).m[3][2]=6.1f;  U(mat).m[3][3]=2.2f;

    hr = ID3DXSprite_SetTransform(sprite, NULL);
    ok (hr == D3DERR_INVALIDCALL, "SetTransform returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = ID3DXSprite_SetTransform(sprite, &mat);
    ok (hr == D3D_OK, "SetTransform returned %#x, expected %#x\n", hr, D3D_OK);
    if(SUCCEEDED(hr)) {
        hr=ID3DXSprite_GetTransform(sprite, &cmpmat);
        if(SUCCEEDED(hr)) check_mat(cmpmat, mat);
        else skip("GetTransform returned %#x\n", hr);
    }

    /* Test ID3DXSprite_SetWorldViewLH/RH */
    todo_wine {
        hr = ID3DXSprite_SetWorldViewLH(sprite, &mat, &mat);
        ok (hr == D3D_OK, "SetWorldViewLH returned %#x, expected %#x\n", hr, D3D_OK);
        hr = ID3DXSprite_SetWorldViewLH(sprite, NULL, &mat);
        ok (hr == D3D_OK, "SetWorldViewLH returned %#x, expected %#x\n", hr, D3D_OK);
        hr = ID3DXSprite_SetWorldViewLH(sprite, &mat, NULL);
        ok (hr == D3D_OK, "SetWorldViewLH returned %#x, expected %#x\n", hr, D3D_OK);
        hr = ID3DXSprite_SetWorldViewLH(sprite, NULL, NULL);
        ok (hr == D3D_OK, "SetWorldViewLH returned %#x, expected %#x\n", hr, D3D_OK);

        hr = ID3DXSprite_SetWorldViewRH(sprite, &mat, &mat);
        ok (hr == D3D_OK, "SetWorldViewRH returned %#x, expected %#x\n", hr, D3D_OK);
        hr = ID3DXSprite_SetWorldViewRH(sprite, NULL, &mat);
        ok (hr == D3D_OK, "SetWorldViewRH returned %#x, expected %#x\n", hr, D3D_OK);
        hr = ID3DXSprite_SetWorldViewRH(sprite, &mat, NULL);
        ok (hr == D3D_OK, "SetWorldViewRH returned %#x, expected %#x\n", hr, D3D_OK);
        hr = ID3DXSprite_SetWorldViewRH(sprite, NULL, NULL);
        ok (hr == D3D_OK, "SetWorldViewRH returned %#x, expected %#x\n", hr, D3D_OK);
    }
    IDirect3DDevice9_BeginScene(device);

    /* Test ID3DXSprite_Begin*/
    hr = ID3DXSprite_Begin(sprite, 0);
    ok (hr == D3D_OK, "Begin returned %#x, expected %#x\n", hr, D3D_OK);

    IDirect3DDevice9_GetTransform(device, D3DTS_WORLD, &mat);
    D3DXMatrixIdentity(&cmpmat);
    check_mat(mat, cmpmat);

    IDirect3DDevice9_GetTransform(device, D3DTS_VIEW, &mat);
    check_mat(mat, cmpmat);

    IDirect3DDevice9_GetTransform(device, D3DTS_PROJECTION, &mat);
    IDirect3DDevice9_GetViewport(device, &vp);
    D3DXMatrixOrthoOffCenterLH(&cmpmat, vp.X+0.5f, (float)vp.Width+vp.X+0.5f, (float)vp.Height+vp.Y+0.5f, vp.Y+0.5f, vp.MinZ, vp.MaxZ);
    check_mat(mat, cmpmat);

    /* Test ID3DXSprite_Flush and ID3DXSprite_End */
    hr = ID3DXSprite_Flush(sprite);
    ok (hr == D3D_OK, "Flush returned %#x, expected %#x\n", hr, D3D_OK);

    hr = ID3DXSprite_End(sprite);
    ok (hr == D3D_OK, "End returned %#x, expected %#x\n", hr, D3D_OK);

    hr = ID3DXSprite_Flush(sprite); /* May not be called before next Begin */
    ok (hr == D3DERR_INVALIDCALL, "Flush returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);
    hr = ID3DXSprite_End(sprite);
    ok (hr == D3DERR_INVALIDCALL, "End returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    /* Test ID3DXSprite_Draw */
    hr = ID3DXSprite_Begin(sprite, 0);
    ok (hr == D3D_OK, "Begin returned %#x, expected %#x\n", hr, D3D_OK);

    if(FAILED(hr)) skip("Couldn't ID3DXSprite_Begin, can't test ID3DXSprite_Draw\n");
    else { /* Feed the sprite batch */
        int texref1, texref2;

        SetRect(&rect, 53, 12, 142, 165);
        pos.x    =  2.2f; pos.y    = 4.5f; pos.z    = 5.1f;
        center.x = 11.3f; center.y = 3.4f; center.z = 1.2f;

        texref1 = get_ref((IUnknown*)tex1);
        texref2 = get_ref((IUnknown*)tex2);

        hr = ID3DXSprite_Draw(sprite, NULL, &rect, &center, &pos, D3DCOLOR_XRGB(255, 255, 255));
        ok (hr == D3DERR_INVALIDCALL, "Draw returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

        hr = ID3DXSprite_Draw(sprite, tex1, &rect, &center, &pos, D3DCOLOR_XRGB(255, 255, 255));
        ok (hr == D3D_OK, "Draw returned %#x, expected %#x\n", hr, D3D_OK);
        hr = ID3DXSprite_Draw(sprite, tex2, &rect, &center, &pos, D3DCOLOR_XRGB(  3,  45,  66));
        ok (hr == D3D_OK, "Draw returned %#x, expected %#x\n", hr, D3D_OK);
        hr = ID3DXSprite_Draw(sprite, tex1,  NULL, &center, &pos, D3DCOLOR_XRGB(255, 255, 255));
        ok (hr == D3D_OK, "Draw returned %#x, expected %#x\n", hr, D3D_OK);
        hr = ID3DXSprite_Draw(sprite, tex1, &rect,    NULL, &pos, D3DCOLOR_XRGB(255, 255, 255));
        ok (hr == D3D_OK, "Draw returned %#x, expected %#x\n", hr, D3D_OK);
        hr = ID3DXSprite_Draw(sprite, tex1, &rect, &center, NULL, D3DCOLOR_XRGB(255, 255, 255));
        ok (hr == D3D_OK, "Draw returned %#x, expected %#x\n", hr, D3D_OK);
        hr = ID3DXSprite_Draw(sprite, tex1,  NULL,    NULL, NULL,                            0);
        ok (hr == D3D_OK, "Draw returned %#x, expected %#x\n", hr, D3D_OK);

        check_ref((IUnknown*)tex1, texref1+5); check_ref((IUnknown*)tex2, texref2+1);
        hr = ID3DXSprite_Flush(sprite);
        ok (hr == D3D_OK, "Flush returned %#x, expected %#x\n", hr, D3D_OK);
        hr = ID3DXSprite_Flush(sprite);   /* Flushing twice should work */
        ok (hr == D3D_OK, "Flush returned %#x, expected %#x\n", hr, D3D_OK);
        check_ref((IUnknown*)tex1, texref1);   check_ref((IUnknown*)tex2, texref2);

        hr = ID3DXSprite_End(sprite);
        ok (hr == D3D_OK, "End returned %#x, expected %#x\n", hr, D3D_OK);
    }

    /* Test ID3DXSprite_OnLostDevice and ID3DXSprite_OnResetDevice */
    /* Both can be called twice */
    hr = ID3DXSprite_OnLostDevice(sprite);
    ok (hr == D3D_OK, "OnLostDevice returned %#x, expected %#x\n", hr, D3D_OK);
    hr = ID3DXSprite_OnLostDevice(sprite);
    ok (hr == D3D_OK, "OnLostDevice returned %#x, expected %#x\n", hr, D3D_OK);
    hr = ID3DXSprite_OnResetDevice(sprite);
    ok (hr == D3D_OK, "OnResetDevice returned %#x, expected %#x\n", hr, D3D_OK);
    hr = ID3DXSprite_OnResetDevice(sprite);
    ok (hr == D3D_OK, "OnResetDevice returned %#x, expected %#x\n", hr, D3D_OK);

    /* Make sure everything works like before */
    hr = ID3DXSprite_Begin(sprite, 0);
    ok (hr == D3D_OK, "Begin returned %#x, expected %#x\n", hr, D3D_OK);
    hr = ID3DXSprite_Draw(sprite, tex2, &rect, &center, &pos, D3DCOLOR_XRGB(255, 255, 255));
    ok (hr == D3D_OK, "Draw returned %#x, expected %#x\n", hr, D3D_OK);
    hr = ID3DXSprite_Flush(sprite);
    ok (hr == D3D_OK, "Flush returned %#x, expected %#x\n", hr, D3D_OK);
    hr = ID3DXSprite_End(sprite);
    ok (hr == D3D_OK, "End returned %#x, expected %#x\n", hr, D3D_OK);

    /* OnResetDevice makes the interface "forget" the Begin call */
    hr = ID3DXSprite_Begin(sprite, 0);
    ok (hr == D3D_OK, "Begin returned %#x, expected %#x\n", hr, D3D_OK);
    hr = ID3DXSprite_OnResetDevice(sprite);
    ok (hr == D3D_OK, "OnResetDevice returned %#x, expected %#x\n", hr, D3D_OK);
    hr = ID3DXSprite_End(sprite);
    ok (hr == D3DERR_INVALIDCALL, "End returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    IDirect3DDevice9_EndScene(device);
    check_release((IUnknown*)sprite, 0);
    check_release((IUnknown*)tex2, 0);
    check_release((IUnknown*)tex1, 0);
}

static void test_ID3DXFont(IDirect3DDevice9 *device)
{
    static const WCHAR testW[] = {'t','e','s','t',0};
    static const char testA[] = "test";
    static const struct
    {
        int font_height;
        unsigned int expected_size;
        unsigned int expected_levels;
    }
    tests[] =
    {
        {  6, 128, 4 },
        {  8, 128, 4 },
        { 10, 256, 5 },
        { 12, 256, 5 },
        { 72, 256, 8 },
    };
    const unsigned int size = ARRAY_SIZE(testW);
    D3DXFONT_DESCA desc;
    ID3DXSprite *sprite;
    int ref, i, height;
    ID3DXFont *font;
    HRESULT hr;
    RECT rect;

    /* D3DXCreateFont */
    ref = get_ref((IUnknown*)device);
    hr = D3DXCreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial", &font);
    ok(hr == D3D_OK, "D3DXCreateFont returned %#x, expected %#x\n", hr, D3D_OK);
    check_ref((IUnknown*)device, ref + 1);
    check_release((IUnknown*)font, 0);
    check_ref((IUnknown*)device, ref);

    hr = D3DXCreateFontA(device, 0, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial", &font);
    ok(hr == D3D_OK, "D3DXCreateFont returned %#x, expected %#x\n", hr, D3D_OK);
    ID3DXFont_Release(font);

    hr = D3DXCreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, NULL, &font);
    ok(hr == D3D_OK, "D3DXCreateFont returned %#x, expected %#x\n", hr, D3D_OK);
    ID3DXFont_Release(font);

    hr = D3DXCreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "", &font);
    ok(hr == D3D_OK, "D3DXCreateFont returned %#x, expected %#x\n", hr, D3D_OK);
    ID3DXFont_Release(font);

    hr = D3DXCreateFontA(NULL, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial", &font);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateFont returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial", NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateFont returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateFontA(NULL, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial", NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateFont returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);


    /* D3DXCreateFontIndirect */
    desc.Height = 12;
    desc.Width = 0;
    desc.Weight = FW_DONTCARE;
    desc.MipLevels = 0;
    desc.Italic = FALSE;
    desc.CharSet = DEFAULT_CHARSET;
    desc.OutputPrecision = OUT_DEFAULT_PRECIS;
    desc.Quality = DEFAULT_QUALITY;
    desc.PitchAndFamily = DEFAULT_PITCH;
    strcpy(desc.FaceName, "Arial");
    hr = D3DXCreateFontIndirectA(device, &desc, &font);
    ok(hr == D3D_OK, "D3DXCreateFontIndirect returned %#x, expected %#x\n", hr, D3D_OK);
    ID3DXFont_Release(font);

    hr = D3DXCreateFontIndirectA(NULL, &desc, &font);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateFontIndirect returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateFontIndirectA(device, NULL, &font);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateFontIndirect returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateFontIndirectA(device, &desc, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateFontIndirect returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);


    /* ID3DXFont_GetDevice */
    hr = D3DXCreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial", &font);
    if(SUCCEEDED(hr)) {
        IDirect3DDevice9 *bufdev;

        hr = ID3DXFont_GetDevice(font, NULL);
        ok(hr == D3DERR_INVALIDCALL, "ID3DXFont_GetDevice returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

        ref = get_ref((IUnknown*)device);
        hr = ID3DXFont_GetDevice(font, &bufdev);
        ok(hr == D3D_OK, "ID3DXFont_GetDevice returned %#x, expected %#x\n", hr, D3D_OK);
        check_release((IUnknown*)bufdev, ref);

        ID3DXFont_Release(font);
    } else skip("Failed to create a ID3DXFont object\n");


    /* ID3DXFont_GetDesc */
    hr = D3DXCreateFontA(device, 12, 8, FW_BOLD, 2, TRUE, ANSI_CHARSET, OUT_RASTER_PRECIS, ANTIALIASED_QUALITY, VARIABLE_PITCH, "Arial", &font);
    if(SUCCEEDED(hr)) {
        hr = ID3DXFont_GetDescA(font, NULL);
        ok(hr == D3DERR_INVALIDCALL, "ID3DXFont_GetDevice returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

        hr = ID3DXFont_GetDescA(font, &desc);
        ok(hr == D3D_OK, "ID3DXFont_GetDevice returned %#x, expected %#x\n", hr, D3D_OK);

        ok(desc.Height == 12, "ID3DXFont_GetDesc returned font height %d, expected %d\n", desc.Height, 12);
        ok(desc.Width == 8, "ID3DXFont_GetDesc returned font width %d, expected %d\n", desc.Width, 8);
        ok(desc.Weight == FW_BOLD, "ID3DXFont_GetDesc returned font weight %d, expected %d\n", desc.Weight, FW_BOLD);
        ok(desc.MipLevels == 2, "ID3DXFont_GetDesc returned font miplevels %d, expected %d\n", desc.MipLevels, 2);
        ok(desc.Italic == TRUE, "ID3DXFont_GetDesc says Italic was %d, but Italic should be %d\n", desc.Italic, TRUE);
        ok(desc.CharSet == ANSI_CHARSET, "ID3DXFont_GetDesc returned font charset %d, expected %d\n", desc.CharSet, ANSI_CHARSET);
        ok(desc.OutputPrecision == OUT_RASTER_PRECIS, "ID3DXFont_GetDesc returned an output precision of %d, expected %d\n", desc.OutputPrecision, OUT_RASTER_PRECIS);
        ok(desc.Quality == ANTIALIASED_QUALITY, "ID3DXFont_GetDesc returned font quality %d, expected %d\n", desc.Quality, ANTIALIASED_QUALITY);
        ok(desc.PitchAndFamily == VARIABLE_PITCH, "ID3DXFont_GetDesc returned pitch and family %d, expected %d\n", desc.PitchAndFamily, VARIABLE_PITCH);
        ok(strcmp(desc.FaceName, "Arial") == 0, "ID3DXFont_GetDesc returned facename \"%s\", expected \"%s\"\n", desc.FaceName, "Arial");

        ID3DXFont_Release(font);
    } else skip("Failed to create a ID3DXFont object\n");


    /* ID3DXFont_GetDC + ID3DXFont_GetTextMetrics */
    hr = D3DXCreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial", &font);
    if(SUCCEEDED(hr)) {
        HDC hdc;

        hdc = ID3DXFont_GetDC(font);
        ok(hdc != NULL, "ID3DXFont_GetDC returned an invalid handle\n");
        if(hdc) {
            TEXTMETRICA metrics, expmetrics;
            BOOL ret;

            ret = ID3DXFont_GetTextMetricsA(font, &metrics);
            ok(ret, "ID3DXFont_GetTextMetricsA failed\n");
            ret = GetTextMetricsA(hdc, &expmetrics);
            ok(ret, "GetTextMetricsA failed\n");

            ok(metrics.tmHeight == expmetrics.tmHeight, "Returned height %d, expected %d\n", metrics.tmHeight, expmetrics.tmHeight);
            ok(metrics.tmAscent == expmetrics.tmAscent, "Returned ascent %d, expected %d\n", metrics.tmAscent, expmetrics.tmAscent);
            ok(metrics.tmDescent == expmetrics.tmDescent, "Returned descent %d, expected %d\n", metrics.tmDescent, expmetrics.tmDescent);
            ok(metrics.tmInternalLeading == expmetrics.tmInternalLeading, "Returned internal leading %d, expected %d\n", metrics.tmInternalLeading, expmetrics.tmInternalLeading);
            ok(metrics.tmExternalLeading == expmetrics.tmExternalLeading, "Returned external leading %d, expected %d\n", metrics.tmExternalLeading, expmetrics.tmExternalLeading);
            ok(metrics.tmAveCharWidth == expmetrics.tmAveCharWidth, "Returned average char width %d, expected %d\n", metrics.tmAveCharWidth, expmetrics.tmAveCharWidth);
            ok(metrics.tmMaxCharWidth == expmetrics.tmMaxCharWidth, "Returned maximum char width %d, expected %d\n", metrics.tmMaxCharWidth, expmetrics.tmMaxCharWidth);
            ok(metrics.tmWeight == expmetrics.tmWeight, "Returned weight %d, expected %d\n", metrics.tmWeight, expmetrics.tmWeight);
            ok(metrics.tmOverhang == expmetrics.tmOverhang, "Returned overhang %d, expected %d\n", metrics.tmOverhang, expmetrics.tmOverhang);
            ok(metrics.tmDigitizedAspectX == expmetrics.tmDigitizedAspectX, "Returned digitized x aspect %d, expected %d\n", metrics.tmDigitizedAspectX, expmetrics.tmDigitizedAspectX);
            ok(metrics.tmDigitizedAspectY == expmetrics.tmDigitizedAspectY, "Returned digitized y aspect %d, expected %d\n", metrics.tmDigitizedAspectY, expmetrics.tmDigitizedAspectY);
            ok(metrics.tmFirstChar == expmetrics.tmFirstChar, "Returned first char %d, expected %d\n", metrics.tmFirstChar, expmetrics.tmFirstChar);
            ok(metrics.tmLastChar == expmetrics.tmLastChar, "Returned last char %d, expected %d\n", metrics.tmLastChar, expmetrics.tmLastChar);
            ok(metrics.tmDefaultChar == expmetrics.tmDefaultChar, "Returned default char %d, expected %d\n", metrics.tmDefaultChar, expmetrics.tmDefaultChar);
            ok(metrics.tmBreakChar == expmetrics.tmBreakChar, "Returned break char %d, expected %d\n", metrics.tmBreakChar, expmetrics.tmBreakChar);
            ok(metrics.tmItalic == expmetrics.tmItalic, "Returned italic %d, expected %d\n", metrics.tmItalic, expmetrics.tmItalic);
            ok(metrics.tmUnderlined == expmetrics.tmUnderlined, "Returned underlined %d, expected %d\n", metrics.tmUnderlined, expmetrics.tmUnderlined);
            ok(metrics.tmStruckOut == expmetrics.tmStruckOut, "Returned struck out %d, expected %d\n", metrics.tmStruckOut, expmetrics.tmStruckOut);
            ok(metrics.tmPitchAndFamily == expmetrics.tmPitchAndFamily, "Returned pitch and family %d, expected %d\n", metrics.tmPitchAndFamily, expmetrics.tmPitchAndFamily);
            ok(metrics.tmCharSet == expmetrics.tmCharSet, "Returned charset %d, expected %d\n", metrics.tmCharSet, expmetrics.tmCharSet);
        }
        ID3DXFont_Release(font);
    } else skip("Failed to create a ID3DXFont object\n");


    /* ID3DXFont_PreloadText */
    hr = D3DXCreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial", &font);
    if(SUCCEEDED(hr)) {
        todo_wine {
        hr = ID3DXFont_PreloadTextA(font, NULL, -1);
        ok(hr == D3DERR_INVALIDCALL, "ID3DXFont_PreloadTextA returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);
        hr = ID3DXFont_PreloadTextA(font, NULL, 0);
        ok(hr == D3D_OK, "ID3DXFont_PreloadTextA returned %#x, expected %#x\n", hr, D3D_OK);
        hr = ID3DXFont_PreloadTextA(font, NULL, 1);
        ok(hr == D3DERR_INVALIDCALL, "ID3DXFont_PreloadTextA returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);
        hr = ID3DXFont_PreloadTextA(font, "test", -1);
        ok(hr == D3D_OK, "ID3DXFont_PreloadTextA returned %#x, expected %#x\n", hr, D3D_OK);

        hr = ID3DXFont_PreloadTextW(font, NULL, -1);
        ok(hr == D3DERR_INVALIDCALL, "ID3DXFont_PreloadTextW returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);
        hr = ID3DXFont_PreloadTextW(font, NULL, 0);
        ok(hr == D3D_OK, "ID3DXFont_PreloadTextW returned %#x, expected %#x\n", hr, D3D_OK);
        hr = ID3DXFont_PreloadTextW(font, NULL, 1);
        ok(hr == D3DERR_INVALIDCALL, "ID3DXFont_PreloadTextW returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);
        hr = ID3DXFont_PreloadTextW(font, testW, -1);
        ok(hr == D3D_OK, "ID3DXFont_PreloadTextW returned %#x, expected %#x\n", hr, D3D_OK);
        }

        check_release((IUnknown*)font, 0);
    } else skip("Failed to create a ID3DXFont object\n");


    /* ID3DXFont_GetGlyphData, ID3DXFont_PreloadGlyphs, ID3DXFont_PreloadCharacters */
    hr = D3DXCreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial", &font);
    if(SUCCEEDED(hr)) {
        char c;
        HDC hdc;
        DWORD ret;
        HRESULT hr;
        RECT blackbox;
        POINT cellinc;
        IDirect3DTexture9 *texture;

        hdc = ID3DXFont_GetDC(font);

        todo_wine {
        hr = ID3DXFont_GetGlyphData(font, 0, NULL, &blackbox, &cellinc);
        ok(hr == D3D_OK, "ID3DXFont_GetGlyphData returned %#x, expected %#x\n", hr, D3D_OK);
        hr = ID3DXFont_GetGlyphData(font, 0, &texture, NULL, &cellinc);
        if(SUCCEEDED(hr)) check_release((IUnknown*)texture, 1);
        ok(hr == D3D_OK, "ID3DXFont_GetGlyphData returned %#x, expected %#x\n", hr, D3D_OK);
        hr = ID3DXFont_GetGlyphData(font, 0, &texture, &blackbox, NULL);
        if(SUCCEEDED(hr)) check_release((IUnknown*)texture, 1);
        ok(hr == D3D_OK, "ID3DXFont_GetGlyphData returned %#x, expected %#x\n", hr, D3D_OK);
        }
        hr = ID3DXFont_PreloadCharacters(font, 'b', 'a');
        ok(hr == D3D_OK, "ID3DXFont_PreloadCharacters returned %#x, expected %#x\n", hr, D3D_OK);
        hr = ID3DXFont_PreloadGlyphs(font, 1, 0);
        todo_wine ok(hr == D3D_OK, "ID3DXFont_PreloadGlyphs returned %#x, expected %#x\n", hr, D3D_OK);

        hr = ID3DXFont_PreloadCharacters(font, 'a', 'a');
        ok(hr == D3D_OK, "ID3DXFont_PreloadCharacters returned %#x, expected %#x\n", hr, D3D_OK);

        for(c = 'b'; c <= 'z'; c++) {
            WORD glyph;

            ret = GetGlyphIndicesA(hdc, &c, 1, &glyph, 0);
            ok(ret != GDI_ERROR, "GetGlyphIndicesA failed\n");

            hr = ID3DXFont_GetGlyphData(font, glyph, &texture, &blackbox, &cellinc);
            todo_wine ok(hr == D3D_OK, "ID3DXFont_GetGlyphData returned %#x, expected %#x\n", hr, D3D_OK);
            if(SUCCEEDED(hr)) {
                DWORD levels;
                D3DSURFACE_DESC desc;

                levels = IDirect3DTexture9_GetLevelCount(texture);
                ok(levels == 5, "Got levels %u, expected %u\n", levels, 5);
                hr = IDirect3DTexture9_GetLevelDesc(texture, 0, &desc);
                ok(hr == D3D_OK, "IDirect3DTexture9_GetLevelDesc failed\n");
                ok(desc.Format == D3DFMT_A8R8G8B8, "Got format %#x, expected %#x\n", desc.Format, D3DFMT_A8R8G8B8);
                ok(desc.Usage == 0, "Got usage %#x, expected %#x\n", desc.Usage, 0);
                ok(desc.Width == 256, "Got width %u, expected %u\n", desc.Width, 256);
                ok(desc.Height == 256, "Got height %u, expected %u\n", desc.Height, 256);
                ok(desc.Pool == D3DPOOL_MANAGED, "Got pool %u, expected %u\n", desc.Pool, D3DPOOL_MANAGED);

                check_release((IUnknown*)texture, 1);
            }
        }

        hr = ID3DXFont_PreloadCharacters(font, 'a', 'z');
        ok(hr == D3D_OK, "ID3DXFont_PreloadCharacters returned %#x, expected %#x\n", hr, D3D_OK);

        check_release((IUnknown*)font, 0);
    } else skip("Failed to create a ID3DXFont object\n");

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        HDC hdc;
        DWORD ret;
        HRESULT hr;
        WORD glyph;
        char c = 'a';
        IDirect3DTexture9 *texture;

        hr = D3DXCreateFontA(device, tests[i].font_height, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial", &font);
        if(FAILED(hr)) {
            skip("Failed to create a ID3DXFont object\n");
            continue;
        }

        hdc = ID3DXFont_GetDC(font);

        ret = GetGlyphIndicesA(hdc, &c, 1, &glyph, 0);
        ok(ret != GDI_ERROR, "GetGlyphIndicesA failed\n");

        hr = ID3DXFont_GetGlyphData(font, glyph, &texture, NULL, NULL);
        todo_wine ok(hr == D3D_OK, "ID3DXFont_GetGlyphData returned %#x, expected %#x\n", hr, D3D_OK);
        if(SUCCEEDED(hr)) {
            DWORD levels;
            D3DSURFACE_DESC desc;

            levels = IDirect3DTexture9_GetLevelCount(texture);
            ok(levels == tests[i].expected_levels, "Got levels %u, expected %u\n",
                    levels, tests[i].expected_levels);
            hr = IDirect3DTexture9_GetLevelDesc(texture, 0, &desc);
            ok(hr == D3D_OK, "IDirect3DTexture9_GetLevelDesc failed\n");
            ok(desc.Format == D3DFMT_A8R8G8B8, "Got format %#x, expected %#x\n", desc.Format, D3DFMT_A8R8G8B8);
            ok(desc.Usage == 0, "Got usage %#x, expected %#x\n", desc.Usage, 0);
            ok(desc.Width == tests[i].expected_size, "Got width %u, expected %u\n",
                    desc.Width, tests[i].expected_size);
            ok(desc.Height == tests[i].expected_size, "Got height %u, expected %u\n",
                    desc.Height, tests[i].expected_size);
            ok(desc.Pool == D3DPOOL_MANAGED, "Got pool %u, expected %u\n", desc.Pool, D3DPOOL_MANAGED);

            IDirect3DTexture9_Release(texture);
        }

        /* ID3DXFontImpl_DrawText */
        D3DXCreateSprite(device, &sprite);
        SetRect(&rect, 0, 0, 640, 480);

        IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff000000, 1.0f, 0);

        IDirect3DDevice9_BeginScene(device);
        hr = ID3DXSprite_Begin(sprite, D3DXSPRITE_ALPHABLEND);
        ok (hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

        todo_wine
        {
        height = ID3DXFont_DrawTextW(font, sprite, testW, -1, &rect, DT_TOP, 0xffffffff);
        ok(height == tests[i].font_height, "Got unexpected height %u.\n", height);
        height = ID3DXFont_DrawTextW(font, sprite, testW, size, &rect, DT_TOP, 0xffffffff);
        ok(height == tests[i].font_height, "Got unexpected height %u.\n", height);
        height = ID3DXFont_DrawTextW(font, sprite, testW, size, &rect, DT_RIGHT, 0xffffffff);
        ok(height == tests[i].font_height, "Got unexpected height %u.\n", height);
        height = ID3DXFont_DrawTextW(font, sprite, testW, size, &rect, DT_LEFT | DT_NOCLIP,
                0xffffffff);
        ok(height == tests[i].font_height, "Got unexpected height %u.\n", height);
        }

        SetRectEmpty(&rect);
        height = ID3DXFont_DrawTextW(font, sprite, testW, size, &rect,
                DT_LEFT | DT_CALCRECT, 0xffffffff);
        todo_wine ok(height == tests[i].font_height, "Got unexpected height %u.\n", height);
        ok(!rect.left, "Got unexpected rect left %d.\n", rect.left);
        ok(!rect.top, "Got unexpected rect top %d.\n", rect.top);
        todo_wine ok(rect.right, "Got unexpected rect right %d.\n", rect.right);
        todo_wine ok(rect.bottom == tests[i].font_height, "Got unexpected rect bottom %d.\n", rect.bottom);

        hr = ID3DXSprite_End(sprite);
        ok (hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
        IDirect3DDevice9_EndScene(device);
        ID3DXSprite_Release(sprite);

        ID3DXFont_Release(font);
    }

    /* ID3DXFont_DrawTextA, ID3DXFont_DrawTextW */
    hr = D3DXCreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial", &font);
    if (SUCCEEDED(hr)) {
        RECT rect;
        int height;

        SetRect(&rect, 10, 10, 200, 200);

        height = ID3DXFont_DrawTextA(font, NULL, testA, -2, &rect, 0, 0xFF00FF);
        ok(height == 12, "DrawTextA returned %d, expected 12.\n", height);

        height = ID3DXFont_DrawTextA(font, NULL, testA, -1, &rect, 0, 0xFF00FF);
        ok(height == 12, "DrawTextA returned %d, expected 12.\n", height);

        height = ID3DXFont_DrawTextA(font, NULL, testA, 0, &rect, 0, 0xFF00FF);
        ok(height == 0, "DrawTextA returned %d, expected 0.\n", height);

        height = ID3DXFont_DrawTextA(font, NULL, testA, 1, &rect, 0, 0xFF00FF);
        ok(height == 12, "DrawTextA returned %d, expected 12.\n", height);

        height = ID3DXFont_DrawTextA(font, NULL, testA, 2, &rect, 0, 0xFF00FF);
        ok(height == 12, "DrawTextA returned %d, expected 12.\n", height);

        height = ID3DXFont_DrawTextA(font, NULL, testA, -1, NULL, 0, 0xFF00FF);
        ok(height == 12, "DrawTextA returned %d, expected 12.\n", height);

        height = ID3DXFont_DrawTextA(font, NULL, testA, -1, NULL, DT_CALCRECT, 0xFF00FF);
        ok(height == 12, "DrawTextA returned %d, expected 12.\n", height);

        height = ID3DXFont_DrawTextA(font, NULL, NULL, -1, NULL, 0, 0xFF00FF);
        ok(height == 0, "DrawTextA returned %d, expected 0.\n", height);

if (0) { /* Causes a lockup on windows 7. */
        height = ID3DXFont_DrawTextW(font, NULL, testW, -2, &rect, 0, 0xFF00FF);
        ok(height == 12, "DrawTextW returned %d, expected 12.\n", height);
}

        height = ID3DXFont_DrawTextW(font, NULL, testW, -1, &rect, 0, 0xFF00FF);
        ok(height == 12, "DrawTextW returned %d, expected 12.\n", height);

        height = ID3DXFont_DrawTextW(font, NULL, testW, 0, &rect, 0, 0xFF00FF);
        ok(height == 0, "DrawTextW returned %d, expected 0.\n", height);

        height = ID3DXFont_DrawTextW(font, NULL, testW, 1, &rect, 0, 0xFF00FF);
        ok(height == 12, "DrawTextW returned %d, expected 12.\n", height);

        height = ID3DXFont_DrawTextW(font, NULL, testW, 2, &rect, 0, 0xFF00FF);
        ok(height == 12, "DrawTextW returned %d, expected 12.\n", height);

        height = ID3DXFont_DrawTextW(font, NULL, testW, -1, NULL, 0, 0xFF00FF);
        ok(height == 12, "DrawTextA returned %d, expected 12.\n", height);

        height = ID3DXFont_DrawTextW(font, NULL, testW, -1, NULL, DT_CALCRECT, 0xFF00FF);
        ok(height == 12, "DrawTextA returned %d, expected 12.\n", height);

        height = ID3DXFont_DrawTextW(font, NULL, NULL, -1, NULL, 0, 0xFF00FF);
        ok(height == 0, "DrawTextA returned %d, expected 0.\n", height);

        ID3DXFont_Release(font);
    }
}

static void test_D3DXCreateRenderToSurface(IDirect3DDevice9 *device)
{
    int i;
    HRESULT hr;
    ULONG ref_count;
    D3DXRTS_DESC desc;
    ID3DXRenderToSurface *render = (void *)0xdeadbeef;
    static const D3DXRTS_DESC tests[] =
    {
        { 0, 256, D3DFMT_A8R8G8B8, FALSE, D3DFMT_UNKNOWN },
        { 256, 0, D3DFMT_A8R8G8B8, FALSE, D3DFMT_UNKNOWN },
        { 256, 0, D3DFMT_A8R8G8B8, FALSE, D3DFMT_D24S8 },
        { 256, 256, D3DFMT_UNKNOWN, FALSE, D3DFMT_R8G8B8 },
        { 0, 0, D3DFMT_UNKNOWN, FALSE, D3DFMT_UNKNOWN },
        { -1, -1, MAKEFOURCC('B','A','D','F'), TRUE, MAKEFOURCC('B','A','D','F') }
    };

    hr = D3DXCreateRenderToSurface(NULL /* device */, 256, 256, D3DFMT_A8R8G8B8, FALSE, D3DFMT_UNKNOWN, &render);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateRenderToSurface returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);
    ok(render == (void *)0xdeadbeef, "Got %p, expected %p\n", render, (void *)0xdeadbeef);

    hr = D3DXCreateRenderToSurface(device, 256, 256, D3DFMT_A8R8G8B8, FALSE, D3DFMT_UNKNOWN, NULL /* out */);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateRenderToSurface returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        hr = D3DXCreateRenderToSurface(device, tests[i].Width, tests[i].Height, tests[i].Format, tests[i].DepthStencil,
                tests[i].DepthStencilFormat, &render);
        ok(hr == D3D_OK, "%d: D3DXCreateRenderToSurface returned %#x, expected %#x\n", i, hr, D3D_OK);
        if (SUCCEEDED(hr))
        {
            hr = ID3DXRenderToSurface_GetDesc(render, &desc);
            ok(hr == D3D_OK, "%d: GetDesc failed %#x\n", i, hr);
            if (SUCCEEDED(hr))
            {
                ok(desc.Width == tests[i].Width, "%d: Got width %u, expected %u\n", i, desc.Width, tests[i].Width);
                ok(desc.Height == tests[i].Height, "%d: Got height %u, expected %u\n", i, desc.Height, tests[i].Height);
                ok(desc.Format == tests[i].Format, "%d: Got format %#x, expected %#x\n", i, desc.Format, tests[i].Format);
                ok(desc.DepthStencil == tests[i].DepthStencil, "%d: Got depth stencil %d, expected %d\n",
                        i, desc.DepthStencil, tests[i].DepthStencil);
                ok(desc.DepthStencilFormat == tests[i].DepthStencilFormat, "%d: Got depth stencil format %#x, expected %#x\n",
                        i, desc.DepthStencilFormat, tests[i].DepthStencilFormat);
            }
            ID3DXRenderToSurface_Release(render);
        }
    }

    /* check device ref count */
    ref_count = get_ref((IUnknown *)device);
    hr = D3DXCreateRenderToSurface(device, 0, 0, D3DFMT_UNKNOWN, FALSE, D3DFMT_UNKNOWN, &render);
    check_ref((IUnknown *)device, ref_count + 1);
    if (SUCCEEDED(hr)) ID3DXRenderToSurface_Release(render);
}

/* runs a set of tests for the ID3DXRenderToSurface interface created with given parameters */
static void check_ID3DXRenderToSurface(IDirect3DDevice9 *device, UINT width, UINT height, D3DFORMAT format,
        BOOL depth_stencil, D3DFORMAT depth_stencil_format, BOOL render_target)
{
    HRESULT hr;
    D3DFORMAT fmt;
    HRESULT expected_value;
    IDirect3DSurface9 *surface;
    ID3DXRenderToSurface *render;
    D3DVIEWPORT9 viewport = { 0, 0, width, height, 0.0, 1.0 };

    hr = D3DXCreateRenderToSurface(device, width, height, format, depth_stencil, depth_stencil_format, &render);
    if (FAILED(hr))
    {
        skip("Failed to create ID3DXRenderToSurface\n");
        return;
    }

    if (render_target)
        hr = IDirect3DDevice9_CreateRenderTarget(device, width, height, format, D3DMULTISAMPLE_NONE, 0, FALSE, &surface, NULL);
    else
        hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, width, height, format, D3DPOOL_DEFAULT, &surface, NULL);
    if (FAILED(hr))
    {
        skip("Failed to create surface\n");
        ID3DXRenderToSurface_Release(render);
        return;
    }

    /* viewport */
    hr = ID3DXRenderToSurface_BeginScene(render, surface, &viewport);
    ok(hr == D3D_OK, "ID3DXRenderToSurface::BeginScene returned %#x, expected %#x\n", hr, D3D_OK);
    check_ref((IUnknown *)surface, 2);
    if (SUCCEEDED(hr)) ID3DXRenderToSurface_EndScene(render, D3DX_FILTER_NONE);

    /* invalid viewport */
    viewport.Width = 2 * width;
    hr = ID3DXRenderToSurface_BeginScene(render, surface, &viewport);
    ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToSurface::BeginScene returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    viewport.X = width / 2;
    viewport.Width = width;
    hr = ID3DXRenderToSurface_BeginScene(render, surface, &viewport);
    ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToSurface::BeginScene returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    viewport.X = width;
    viewport.Width = width;
    hr = ID3DXRenderToSurface_BeginScene(render, surface, &viewport);
    ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToSurface::BeginScene returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    /* rendering to a part of a surface is only allowed for render target surfaces */
    expected_value = render_target ? D3D_OK : D3DERR_INVALIDCALL;

    viewport.X = 0;
    viewport.Width = width / 2;
    hr = ID3DXRenderToSurface_BeginScene(render, surface, &viewport);
    ok(hr == expected_value, "ID3DXRenderToSurface::BeginScene returned %#x, expected %#x\n", hr, expected_value);
    if (SUCCEEDED(hr)) ID3DXRenderToSurface_EndScene(render, D3DX_FILTER_NONE);

    viewport.X = width / 2;
    viewport.Width = width - width / 2;
    hr = ID3DXRenderToSurface_BeginScene(render, surface, &viewport);
    ok(hr == expected_value, "ID3DXRenderToSurface::BeginScene returned %#x, expected %#x\n", hr, expected_value);
    if (SUCCEEDED(hr)) ID3DXRenderToSurface_EndScene(render, D3DX_FILTER_NONE);

    check_release((IUnknown *)surface, 0);

    /* surfaces with different sizes */
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, width / 2, width / 2, format, D3DPOOL_DEFAULT, &surface, NULL);
    if (FAILED(hr))
    {
        skip("Failed to create surface\n");
        ID3DXRenderToSurface_Release(render);
        return;
    }
    hr = ID3DXRenderToSurface_BeginScene(render, surface, NULL);
    ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToSurface::BeginScene returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);
    check_release((IUnknown *)surface, 0);

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 2 * width, 2 * height, format, D3DPOOL_DEFAULT, &surface, NULL);
    if (FAILED(hr))
    {
        skip("Failed to create surface\n");
        ID3DXRenderToSurface_Release(render);
        return;
    }
    hr = ID3DXRenderToSurface_BeginScene(render, surface, NULL);
    ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToSurface::BeginScene returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);
    viewport.X = 0;
    viewport.Y = 0;
    viewport.Width = width;
    viewport.Height = height;
    hr = ID3DXRenderToSurface_BeginScene(render, surface, &viewport);
    ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToSurface::BeginScene returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);
    check_release((IUnknown *)surface, 0);

    /* surfaces with different formats */
    for (fmt = D3DFMT_A8R8G8B8; fmt <= D3DFMT_X8R8G8B8; fmt++)
    {
        HRESULT expected_result = (fmt != format) ? D3DERR_INVALIDCALL : D3D_OK;

        hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, width, height, fmt, D3DPOOL_DEFAULT, &surface, NULL);
        if (FAILED(hr))
        {
            skip("Failed to create surface\n");
            continue;
        }

        hr = ID3DXRenderToSurface_BeginScene(render, surface, NULL);
        ok(hr == expected_result, "ID3DXRenderToSurface::BeginScene returned %#x, expected %#x\n", hr, expected_result);

        if (SUCCEEDED(hr)) ID3DXRenderToSurface_EndScene(render, D3DX_FILTER_NONE);
        check_release((IUnknown *)surface, 0);
    }

    check_release((IUnknown *)render, 0);
}

struct device_state
{
    IDirect3DSurface9 *render_target;
    IDirect3DSurface9 *depth_stencil;
    D3DVIEWPORT9 viewport;
};

static void release_device_state(struct device_state *state)
{
    if (state->render_target) IDirect3DSurface9_Release(state->render_target);
    if (state->depth_stencil) IDirect3DSurface9_Release(state->depth_stencil);
    memset(state, 0, sizeof(*state));
}

static HRESULT retrieve_device_state(IDirect3DDevice9 *device, struct device_state *state)
{
    HRESULT hr;

    memset(state, 0, sizeof(*state));

    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &state->render_target);
    if (FAILED(hr)) goto cleanup;

    hr = IDirect3DDevice9_GetDepthStencilSurface(device, &state->depth_stencil);
    if (hr == D3DERR_NOTFOUND)
        state->depth_stencil = NULL;
    else if (FAILED(hr))
        goto cleanup;

    hr = IDirect3DDevice9_GetViewport(device, &state->viewport);
    if (SUCCEEDED(hr)) return hr;

cleanup:
    release_device_state(state);
    return hr;
}

static HRESULT apply_device_state(IDirect3DDevice9 *device, struct device_state *state)
{
    HRESULT hr;
    HRESULT status = D3D_OK;

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, state->render_target);
    if (FAILED(hr)) status = hr;

    hr = IDirect3DDevice9_SetDepthStencilSurface(device, state->depth_stencil);
    if (FAILED(hr)) status = hr;

    hr = IDirect3DDevice9_SetViewport(device, &state->viewport);
    if (FAILED(hr)) status = hr;

    return status;
}

static void compare_device_state(struct device_state *state1, struct device_state *state2, BOOL equal)
{
    BOOL cmp;
    const char *message = equal ? "differs" : "is the same";

    cmp = state1->render_target == state2->render_target;
    ok(equal ? cmp : !cmp, "Render target %s %p, %p\n", message, state1->render_target, state2->render_target);

    cmp = state1->depth_stencil == state2->depth_stencil;
    ok(equal ? cmp : !cmp, "Depth stencil surface %s %p, %p\n", message, state1->depth_stencil, state2->depth_stencil);

    cmp = state1->viewport.X == state2->viewport.X && state1->viewport.Y == state2->viewport.Y
            && state1->viewport.Width == state2->viewport.Width && state1->viewport.Height == state2->viewport.Height;
    ok(equal ? cmp : !cmp, "Viewport %s (%u, %u, %u, %u), (%u, %u, %u, %u)\n", message,
            state1->viewport.X, state1->viewport.Y, state1->viewport.Width, state1->viewport.Height,
            state2->viewport.X, state2->viewport.Y, state2->viewport.Width, state2->viewport.Height);
}

static void test_ID3DXRenderToSurface_device_state(IDirect3DDevice9 *device)
{
    HRESULT hr;
    IDirect3DSurface9 *surface = NULL;
    ID3DXRenderToSurface *render = NULL;
    struct device_state pre_state;
    struct device_state current_state;
    IDirect3DSurface9 *depth_stencil_surface;

    /* make sure there is a depth stencil surface present */
    hr = IDirect3DDevice9_GetDepthStencilSurface(device, &depth_stencil_surface);
    if (SUCCEEDED(hr))
    {
        IDirect3DSurface9_Release(depth_stencil_surface);
        depth_stencil_surface = NULL;
    }
    else if (hr == D3DERR_NOTFOUND)
    {
        hr = IDirect3DDevice9_CreateDepthStencilSurface(device, 256, 256, D3DFMT_D24X8,
                D3DMULTISAMPLE_NONE, 0, TRUE, &depth_stencil_surface, NULL);
        if (SUCCEEDED(hr)) IDirect3DDevice9_SetDepthStencilSurface(device, depth_stencil_surface);
    }

    if (FAILED(hr))
    {
        skip("Failed to create depth stencil surface\n");
        return;
    }

    hr = IDirect3DDevice9_CreateRenderTarget(device, 256, 256, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0,
        FALSE, &surface, NULL);
    if (FAILED(hr))
    {
        skip("Failed to create render target\n");
        goto cleanup;
    }

    hr = retrieve_device_state(device, &pre_state);
    ok(SUCCEEDED(hr), "Failed to retrieve device state\n");

    hr = D3DXCreateRenderToSurface(device, 256, 256, D3DFMT_A8R8G8B8, TRUE, D3DFMT_D24X8, &render);
    ok(hr == D3D_OK, "D3DXCreateRenderToSurface returned %#x, expected %#x\n", hr, D3D_OK);
    if (SUCCEEDED(hr))
    {
        hr = ID3DXRenderToSurface_BeginScene(render, surface, NULL);
        ok(hr == D3D_OK, "ID3DXRenderToSurface::BeginScene returned %#x, expected %#x\n", hr, D3D_OK);

        hr = retrieve_device_state(device, &current_state);
        ok(SUCCEEDED(hr), "Failed to retrieve device state\n");
        compare_device_state(&current_state, &pre_state, FALSE);
        release_device_state(&current_state);

        hr = ID3DXRenderToSurface_EndScene(render, D3DX_FILTER_NONE);
        ok(hr == D3D_OK, "ID3DXRenderToSurface::EndScene returned %#x, expected %#x\n", hr, D3D_OK);

        hr = retrieve_device_state(device, &current_state);
        ok(SUCCEEDED(hr), "Failed to retrieve device state\n");
        compare_device_state(&current_state, &pre_state, TRUE);
        release_device_state(&current_state);

        check_release((IUnknown *)render, 0);
    }

    hr = D3DXCreateRenderToSurface(device, 256, 256, D3DFMT_A8R8G8B8, FALSE, D3DFMT_UNKNOWN, &render);
    if (SUCCEEDED(hr))
    {
        hr = ID3DXRenderToSurface_BeginScene(render, surface, NULL);
        ok(hr == D3D_OK, "ID3DXRenderToSurface::BeginScene returned %#x, expected %#x\n", hr, D3D_OK);

        hr = retrieve_device_state(device, &current_state);
        ok(SUCCEEDED(hr), "Failed to retrieve device state\n");
        compare_device_state(&current_state, &pre_state, FALSE);
        release_device_state(&current_state);

        hr = ID3DXRenderToSurface_EndScene(render, D3DX_FILTER_NONE);
        ok(hr == D3D_OK, "ID3DXRenderToSurface::EndScene returned %#x, expected %#x\n", hr, D3D_OK);

        hr = retrieve_device_state(device, &current_state);
        ok(SUCCEEDED(hr), "Failed to retrieve device state\n");
        compare_device_state(&current_state, &pre_state, TRUE);
        release_device_state(&current_state);

        hr = ID3DXRenderToSurface_BeginScene(render, surface, NULL);
        ok(hr == D3D_OK, "ID3DXRenderToSurface::BeginScene returned %#x, expected %#x\n", hr, D3D_OK);

        hr = retrieve_device_state(device, &current_state);
        ok(SUCCEEDED(hr), "Failed to retrieve device state\n");
        compare_device_state(&current_state, &pre_state, FALSE);
        release_device_state(&current_state);

        check_release((IUnknown *)render, 0);

        /* if EndScene isn't called, the device state isn't restored */
        hr = retrieve_device_state(device, &current_state);
        ok(SUCCEEDED(hr), "Failed to retrieve device state\n");
        compare_device_state(&current_state, &pre_state, FALSE);
        release_device_state(&current_state);

        hr = apply_device_state(device, &pre_state);
        ok(SUCCEEDED(hr), "Failed to restore previous device state\n");

        IDirect3DDevice9_EndScene(device);
    }

    release_device_state(&pre_state);

cleanup:
    if (depth_stencil_surface)
    {
        IDirect3DDevice9_SetDepthStencilSurface(device, NULL);
        IDirect3DSurface9_Release(depth_stencil_surface);
    }

    if (surface) check_release((IUnknown *)surface, 0);
}

static void test_ID3DXRenderToSurface(IDirect3DDevice9 *device)
{
    int i;
    HRESULT hr;
    ULONG ref_count;
    IDirect3DDevice9 *out_device;
    ID3DXRenderToSurface *render;
    IDirect3DSurface9 *surface;
    D3DVIEWPORT9 viewport = { 0, 0, 256, 256, 0.0, 1.0 };
    D3DXRTS_DESC tests[] = {
        { 256, 256, D3DFMT_A8R8G8B8, FALSE, D3DFMT_UNKNOWN },
        { 256, 256, D3DFMT_A8R8G8B8, TRUE, D3DFMT_D24S8 },
        { 256, 256, D3DFMT_A8R8G8B8, TRUE, D3DFMT_D24X8 },
        { 512, 512, D3DFMT_X8R8G8B8, FALSE, D3DFMT_X8R8G8B8 },
        { 1024, 1024, D3DFMT_X8R8G8B8, TRUE, D3DFMT_D24S8 }
    };

    hr = D3DXCreateRenderToSurface(device, 256, 256, D3DFMT_A8R8G8B8, FALSE, D3DFMT_UNKNOWN, &render);
    ok(hr == D3D_OK, "D3DXCreateRenderToSurface returned %#x, expected %#x\n", hr, D3D_OK);
    if (FAILED(hr)) return;

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 256, 256, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &surface, NULL);
    if (SUCCEEDED(hr))
    {
        ID3DXRenderToSurface *render_surface;

        /* GetDevice */
        hr = ID3DXRenderToSurface_GetDevice(render, NULL /* device */);
        ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToSurface::GetDevice returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

        ref_count = get_ref((IUnknown *)device);
        hr = ID3DXRenderToSurface_GetDevice(render, &out_device);
        ok(hr == D3D_OK, "ID3DXRenderToSurface::GetDevice returned %#x, expected %#x\n", hr, D3D_OK);
        check_release((IUnknown *)out_device, ref_count);

        /* BeginScene and EndScene */
        hr = ID3DXRenderToSurface_EndScene(render, D3DX_FILTER_NONE);
        ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToSurface::EndScene returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

        hr = ID3DXRenderToSurface_BeginScene(render, NULL /* surface */, &viewport);
        ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToSurface::BeginScene returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

        ref_count = get_ref((IUnknown *)surface);
        hr = ID3DXRenderToSurface_BeginScene(render, surface, NULL);
        ok(hr == D3D_OK, "ID3DXRenderToSurface::BeginScene returned %#x, expected %#x\n", hr, D3D_OK);
        if (SUCCEEDED(hr))
        {
            check_ref((IUnknown *)surface, ref_count + 1);

            hr = ID3DXRenderToSurface_BeginScene(render, surface, NULL);
            ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToSurface::BeginScene returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

            hr = ID3DXRenderToSurface_EndScene(render, D3DX_FILTER_NONE);
            ok(hr == D3D_OK, "ID3DXRenderToSurface::EndScene returned %#x, expected %#x\n", hr, D3D_OK);

            check_ref((IUnknown *)surface, ref_count);
        }

        /* error handling is deferred to BeginScene */
        hr = D3DXCreateRenderToSurface(device, 256, 256, D3DFMT_A8R8G8B8, TRUE, D3DFMT_UNKNOWN, &render_surface);
        ok(hr == D3D_OK, "D3DXCreateRenderToSurface returned %#x, expected %#x\n", hr, D3D_OK);
        hr = ID3DXRenderToSurface_BeginScene(render_surface, surface, NULL);
        ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToSurface::BeginScene returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);
        check_release((IUnknown *)render_surface, 0);

        check_release((IUnknown *)surface, 0);
    }
    else skip("Failed to create surface\n");

    check_release((IUnknown *)render, 0);

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        check_ID3DXRenderToSurface(device, tests[i].Width, tests[i].Height, tests[i].Format, tests[i].DepthStencil, tests[i].DepthStencilFormat, TRUE);
        check_ID3DXRenderToSurface(device, tests[i].Width, tests[i].Height, tests[i].Format, tests[i].DepthStencil, tests[i].DepthStencilFormat, FALSE);
    }

    test_ID3DXRenderToSurface_device_state(device);
}

static void test_D3DXCreateRenderToEnvMap(IDirect3DDevice9 *device)
{
    int i;
    HRESULT hr;
    ULONG ref_count;
    D3DXRTE_DESC desc;
    ID3DXRenderToEnvMap *render;
    static const struct {
        D3DXRTE_DESC parameters;
        D3DXRTE_DESC expected_values;
    } tests[] = {
        { {   0,   0, D3DFMT_A8R8G8B8, FALSE, D3DFMT_UNKNOWN }, {   1, 1, D3DFMT_A8R8G8B8, FALSE, D3DFMT_UNKNOWN } },
        { { 256,   0, D3DFMT_A8R8G8B8, FALSE, D3DFMT_UNKNOWN }, { 256, 9, D3DFMT_A8R8G8B8, FALSE, D3DFMT_UNKNOWN } },
        { { 256,   4, D3DFMT_A8R8G8B8, FALSE, D3DFMT_D24S8   }, { 256, 4, D3DFMT_A8R8G8B8, FALSE, D3DFMT_D24S8   } },
        { { 256, 256, D3DFMT_UNKNOWN,  FALSE, D3DFMT_R8G8B8  }, { 256, 9, D3DFMT_A8R8G8B8, FALSE, D3DFMT_R8G8B8  } },
        { {  -1,  -1, D3DFMT_A8R8G8B8, TRUE,  D3DFMT_DXT1    }, { 256, 9, D3DFMT_A8R8G8B8, TRUE,  D3DFMT_DXT1    } },
        { { 256,   1, D3DFMT_X8R8G8B8, TRUE,  D3DFMT_UNKNOWN }, { 256, 1, D3DFMT_X8R8G8B8, TRUE,  D3DFMT_UNKNOWN } }
    };

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        const D3DXRTE_DESC *parameters = &tests[i].parameters;
        const D3DXRTE_DESC *expected  = &tests[i].expected_values;
        hr = D3DXCreateRenderToEnvMap(device, parameters->Size, parameters->MipLevels, parameters->Format,
                parameters->DepthStencil, parameters->DepthStencilFormat, &render);
        ok(hr == D3D_OK, "%d: D3DXCreateRenderToEnvMap returned %#x, expected %#x\n", i, hr, D3D_OK);
        if (SUCCEEDED(hr))
        {
            hr = ID3DXRenderToEnvMap_GetDesc(render, &desc);
            ok(hr == D3D_OK, "%d: GetDesc failed %#x\n", i, hr);
            if (SUCCEEDED(hr))
            {
                ok(desc.Size == expected->Size, "%d: Got size %u, expected %u\n", i, desc.Size, expected->Size);
                ok(desc.MipLevels == expected->MipLevels, "%d: Got miplevels %u, expected %u\n", i, desc.MipLevels, expected->MipLevels);
                ok(desc.Format == expected->Format, "%d: Got format %#x, expected %#x\n", i, desc.Format, expected->Format);
                ok(desc.DepthStencil == expected->DepthStencil, "%d: Got depth stencil %d, expected %d\n",
                        i, expected->DepthStencil, expected->DepthStencil);
                ok(desc.DepthStencilFormat == expected->DepthStencilFormat, "%d: Got depth stencil format %#x, expected %#x\n",
                        i, expected->DepthStencilFormat, expected->DepthStencilFormat);
            }
            check_release((IUnknown *)render, 0);
        }
    }

    /* check device ref count */
    ref_count = get_ref((IUnknown *)device);
    hr = D3DXCreateRenderToEnvMap(device, 0, 0, D3DFMT_UNKNOWN, FALSE, D3DFMT_UNKNOWN, &render);
    check_ref((IUnknown *)device, ref_count + 1);
    if (SUCCEEDED(hr)) ID3DXRenderToEnvMap_Release(render);
}

static void test_ID3DXRenderToEnvMap_cube_map(IDirect3DDevice9 *device)
{
    HRESULT hr;
    IDirect3DCubeTexture9 *cube_texture = NULL;
    ID3DXRenderToEnvMap *render = NULL;
    struct device_state pre_state;
    struct device_state current_state;

    hr = IDirect3DDevice9_CreateCubeTexture(device, 256, 0, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT,
        &cube_texture, NULL);
    if (FAILED(hr))
    {
        skip("Failed to create cube texture\n");
        return;
    }

    hr = retrieve_device_state(device, &pre_state);
    ok(SUCCEEDED(hr), "Failed to retrieve device state\n");

    hr = D3DXCreateRenderToEnvMap(device, 256, 0, D3DFMT_A8R8G8B8, TRUE, D3DFMT_D24X8, &render);
    ok(hr == D3D_OK, "D3DCreateRenderToEnvMap returned %#x, expected %#x\n", hr, D3D_OK);
    if (SUCCEEDED(hr))
    {
        DWORD face;

        hr = ID3DXRenderToEnvMap_End(render, D3DX_FILTER_NONE);
        ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToEnvMap::End returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

        hr = ID3DXRenderToEnvMap_BeginCube(render, cube_texture);
        ok(hr == D3D_OK, "ID3DXRenderToEnvMap::BeginCube returned %#x, expected %#x\n", hr, D3D_OK);

        hr = retrieve_device_state(device, &current_state);
        ok(SUCCEEDED(hr), "Failed to retrieve device state\n");
        compare_device_state(&current_state, &pre_state, TRUE);
        release_device_state(&current_state);

        for (face = D3DCUBEMAP_FACE_POSITIVE_X; face <= D3DCUBEMAP_FACE_NEGATIVE_Z; face++)
        {
            hr = ID3DXRenderToEnvMap_Face(render, face, D3DX_FILTER_POINT);
            ok(hr == D3D_OK, "ID3DXRenderToEnvMap::Face returned %#x, expected %#x\n", hr, D3D_OK);

            hr = retrieve_device_state(device, &current_state);
            ok(SUCCEEDED(hr), "Failed to retrieve device state\n");
            compare_device_state(&current_state, &pre_state, FALSE);
            release_device_state(&current_state);
        }

        hr = ID3DXRenderToEnvMap_End(render, D3DX_FILTER_POINT);
        ok(hr == D3D_OK, "ID3DXRenderToEnvMap::End returned %#x, expected %#x\n", hr, D3D_OK);

        hr = retrieve_device_state(device, &current_state);
        ok(SUCCEEDED(hr), "Failed to retrieve device state\n");
        compare_device_state(&current_state, &pre_state, TRUE);
        release_device_state(&current_state);

        check_release((IUnknown *)render, 0);
    }

    release_device_state(&pre_state);

    check_release((IUnknown *)cube_texture, 0);
}

static void test_ID3DXRenderToEnvMap(IDirect3DDevice9 *device)
{
    HRESULT hr;
    ID3DXRenderToEnvMap *render;
    IDirect3DSurface9 *depth_stencil_surface;

    hr = D3DXCreateRenderToEnvMap(device, 256, 0, D3DFMT_A8R8G8B8, FALSE, D3DFMT_UNKNOWN, &render);
    if (SUCCEEDED(hr))
    {
        ULONG ref_count;
        IDirect3DDevice9 *out_device;

        hr = ID3DXRenderToEnvMap_GetDesc(render, NULL);
        ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToEnvMap::GetDesc returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

        hr = ID3DXRenderToEnvMap_GetDevice(render, NULL);
        ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToEnvMap::GetDevice returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

        ref_count = get_ref((IUnknown *)device);
        hr = ID3DXRenderToEnvMap_GetDevice(render, &out_device);
        ok(hr == D3D_OK, "ID3DXRenderToEnvMap::GetDevice returned %#x, expected %#x\n", hr, D3D_OK);
        ok(out_device == device, "ID3DXRenderToEnvMap::GetDevice returned different device\n");
        check_release((IUnknown *)device, ref_count);

        hr = ID3DXRenderToEnvMap_End(render, D3DX_FILTER_NONE);
        ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToEnvMap::End returned %#x, expected %#x\n", hr, D3D_OK);

        hr = ID3DXRenderToEnvMap_BeginCube(render, NULL);
        ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToEnvMap::BeginCube returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

        hr = ID3DXRenderToEnvMap_BeginHemisphere(render, NULL, NULL);
        todo_wine ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToEnvMap::BeginHemisphere returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

        hr = ID3DXRenderToEnvMap_BeginParabolic(render, NULL, NULL);
        todo_wine ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToEnvMap::BeginParabolic returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

        hr = ID3DXRenderToEnvMap_BeginSphere(render, NULL);
        todo_wine ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToEnvMap::BeginSphere returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

        check_release((IUnknown *)render, 0);
    } else skip("Failed to create ID3DXRenderToEnvMap\n");

    /* make sure there is a depth stencil surface present */
    hr = IDirect3DDevice9_GetDepthStencilSurface(device, &depth_stencil_surface);
    if (SUCCEEDED(hr))
    {
        IDirect3DSurface9_Release(depth_stencil_surface);
        depth_stencil_surface = NULL;
    }
    else if (hr == D3DERR_NOTFOUND)
    {
        hr = IDirect3DDevice9_CreateDepthStencilSurface(device, 256, 256, D3DFMT_D24X8,
                D3DMULTISAMPLE_NONE, 0, TRUE, &depth_stencil_surface, NULL);
        if (SUCCEEDED(hr)) IDirect3DDevice9_SetDepthStencilSurface(device, depth_stencil_surface);
    }

    if (FAILED(hr))
    {
        skip("Failed to create depth stencil surface\n");
        return;
    }

    test_ID3DXRenderToEnvMap_cube_map(device);

    if (depth_stencil_surface)
    {
        IDirect3DDevice9_SetDepthStencilSurface(device, NULL);
        IDirect3DSurface9_Release(depth_stencil_surface);
    }
}

START_TEST(core)
{
    HWND wnd;
    IDirect3D9 *d3d;
    IDirect3DDevice9 *device;
    D3DPRESENT_PARAMETERS d3dpp;
    HRESULT hr;

    if (!(wnd = CreateWindowA("static", "d3dx9_test", WS_OVERLAPPEDWINDOW, 0, 0,
            640, 480, NULL, NULL, NULL, NULL)))
    {
        skip("Couldn't create application window\n");
        return;
    }
    if (!(d3d = Direct3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Couldn't create IDirect3D9 object\n");
        DestroyWindow(wnd);
        return;
    }

    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed   = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &device);
    if(FAILED(hr)) {
        skip("Failed to create IDirect3DDevice9 object %#x\n", hr);
        IDirect3D9_Release(d3d);
        DestroyWindow(wnd);
        return;
    }

    test_ID3DXBuffer();
    test_ID3DXSprite(device);
    test_ID3DXFont(device);
    test_D3DXCreateRenderToSurface(device);
    test_ID3DXRenderToSurface(device);
    test_D3DXCreateRenderToEnvMap(device);
    test_ID3DXRenderToEnvMap(device);

    check_release((IUnknown*)device, 0);
    check_release((IUnknown*)d3d, 0);
    if (wnd) DestroyWindow(wnd);
}
