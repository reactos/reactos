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
            if (fabs(exp.m[i][j]-got.m[i][j]) > admitted_error)
                equal=0;

    ok(equal, "Got matrix\n\t(%f,%f,%f,%f\n\t %f,%f,%f,%f\n\t %f,%f,%f,%f\n\t %f,%f,%f,%f)\n"
       "Expected matrix=\n\t(%f,%f,%f,%f\n\t %f,%f,%f,%f\n\t %f,%f,%f,%f\n\t %f,%f,%f,%f)\n",
       got.m[0][0],got.m[0][1],got.m[0][2],got.m[0][3],
       got.m[1][0],got.m[1][1],got.m[1][2],got.m[1][3],
       got.m[2][0],got.m[2][1],got.m[2][2],got.m[2][3],
       got.m[3][0],got.m[3][1],got.m[3][2],got.m[3][3],
       exp.m[0][0],exp.m[0][1],exp.m[0][2],exp.m[0][3],
       exp.m[1][0],exp.m[1][1],exp.m[1][2],exp.m[1][3],
       exp.m[2][0],exp.m[2][1],exp.m[2][2],exp.m[2][3],
       exp.m[3][0],exp.m[3][1],exp.m[3][2],exp.m[3][3]);
}

#define check_rect(rect, left, top, right, bottom) _check_rect(__LINE__, rect, left, top, right, bottom)
static inline void _check_rect(unsigned int line, const RECT *rect, int left, int top, int right, int bottom)
{
    ok_(__FILE__, line)(rect->left == left, "Unexpected rect.left %ld\n", rect->left);
    ok_(__FILE__, line)(rect->top == top, "Unexpected rect.top %ld\n", rect->top);
    ok_(__FILE__, line)(rect->right == right, "Unexpected rect.right %ld\n", rect->right);
    ok_(__FILE__, line)(rect->bottom == bottom, "Unexpected rect.bottom %ld\n", rect->bottom);
}

static void test_ID3DXBuffer(void)
{
    ID3DXBuffer *buffer;
    HRESULT hr;
    ULONG count;
    DWORD size;

    hr = D3DXCreateBuffer(10, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateBuffer failed, got %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateBuffer(0, &buffer);
    ok(hr == D3D_OK, "D3DXCreateBuffer failed, got %#lx, expected %#lx\n", hr, D3D_OK);

    size = ID3DXBuffer_GetBufferSize(buffer);
    ok(!size, "GetBufferSize failed, got %lu, expected %u\n", size, 0);

    count = ID3DXBuffer_Release(buffer);
    ok(!count, "ID3DXBuffer has %lu references left\n", count);

    hr = D3DXCreateBuffer(3, &buffer);
    ok(hr == D3D_OK, "D3DXCreateBuffer failed, got %#lx, expected %#lx\n", hr, D3D_OK);

    size = ID3DXBuffer_GetBufferSize(buffer);
    ok(size == 3, "GetBufferSize failed, got %lu, expected %u\n", size, 3);

    count = ID3DXBuffer_Release(buffer);
    ok(!count, "ID3DXBuffer has %lu references left\n", count);
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
    ok (hr == D3D_OK, "Failed to create first texture (error code: %#lx)\n", hr);
    if (FAILED(hr)) return;

    hr = IDirect3DDevice9_CreateTexture(device, 32, 32, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex2, NULL);
    ok (hr == D3D_OK, "Failed to create second texture (error code: %#lx)\n", hr);
    if (FAILED(hr)) {
        IDirect3DTexture9_Release(tex1);
        return;
    }

    /* Test D3DXCreateSprite */
    hr = D3DXCreateSprite(device, NULL);
    ok (hr == D3DERR_INVALIDCALL, "D3DXCreateSprite returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateSprite(NULL, &sprite);
    ok (hr == D3DERR_INVALIDCALL, "D3DXCreateSprite returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateSprite(device, &sprite);
    ok (hr == D3D_OK, "D3DXCreateSprite returned %#lx, expected %#lx\n", hr, D3D_OK);


    /* Test ID3DXSprite_GetDevice */
    hr = ID3DXSprite_GetDevice(sprite, NULL);
    ok (hr == D3DERR_INVALIDCALL, "GetDevice returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = ID3DXSprite_GetDevice(sprite, &cmpdev);  /* cmpdev == NULL */
    ok (hr == D3D_OK, "GetDevice returned %#lx, expected %#lx\n", hr, D3D_OK);

    hr = ID3DXSprite_GetDevice(sprite, &cmpdev);  /* cmpdev != NULL */
    ok (hr == D3D_OK, "GetDevice returned %#lx, expected %#lx\n", hr, D3D_OK);

    IDirect3DDevice9_Release(device);
    IDirect3DDevice9_Release(device);


    /* Test ID3DXSprite_GetTransform */
    hr = ID3DXSprite_GetTransform(sprite, NULL);
    ok (hr == D3DERR_INVALIDCALL, "GetTransform returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);
    hr = ID3DXSprite_GetTransform(sprite, &mat);
    ok (hr == D3D_OK, "GetTransform returned %#lx, expected %#lx\n", hr, D3D_OK);
    if(SUCCEEDED(hr)) {
        D3DXMATRIX identity;
        D3DXMatrixIdentity(&identity);
        check_mat(mat, identity);
    }

    /* Test ID3DXSprite_SetTransform */
    /* Set a transform and test if it gets returned correctly */
    mat.m[0][0]=2.1f;  mat.m[0][1]=6.5f;  mat.m[0][2]=-9.6f; mat.m[0][3]=1.7f;
    mat.m[1][0]=4.2f;  mat.m[1][1]=-2.5f; mat.m[1][2]=2.1f;  mat.m[1][3]=5.5f;
    mat.m[2][0]=-2.6f; mat.m[2][1]=0.3f;  mat.m[2][2]=8.6f;  mat.m[2][3]=8.4f;
    mat.m[3][0]=6.7f;  mat.m[3][1]=-5.1f; mat.m[3][2]=6.1f;  mat.m[3][3]=2.2f;

    hr = ID3DXSprite_SetTransform(sprite, NULL);
    ok (hr == D3DERR_INVALIDCALL, "SetTransform returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = ID3DXSprite_SetTransform(sprite, &mat);
    ok (hr == D3D_OK, "SetTransform returned %#lx, expected %#lx\n", hr, D3D_OK);
    if(SUCCEEDED(hr)) {
        hr=ID3DXSprite_GetTransform(sprite, &cmpmat);
        if(SUCCEEDED(hr)) check_mat(cmpmat, mat);
        else skip("GetTransform returned %#lx\n", hr);
    }

    /* Test ID3DXSprite_SetWorldViewLH/RH */
    todo_wine {
        hr = ID3DXSprite_SetWorldViewLH(sprite, &mat, &mat);
        ok (hr == D3D_OK, "SetWorldViewLH returned %#lx, expected %#lx\n", hr, D3D_OK);
        hr = ID3DXSprite_SetWorldViewLH(sprite, NULL, &mat);
        ok (hr == D3D_OK, "SetWorldViewLH returned %#lx, expected %#lx\n", hr, D3D_OK);
        hr = ID3DXSprite_SetWorldViewLH(sprite, &mat, NULL);
        ok (hr == D3D_OK, "SetWorldViewLH returned %#lx, expected %#lx\n", hr, D3D_OK);
        hr = ID3DXSprite_SetWorldViewLH(sprite, NULL, NULL);
        ok (hr == D3D_OK, "SetWorldViewLH returned %#lx, expected %#lx\n", hr, D3D_OK);

        hr = ID3DXSprite_SetWorldViewRH(sprite, &mat, &mat);
        ok (hr == D3D_OK, "SetWorldViewRH returned %#lx, expected %#lx\n", hr, D3D_OK);
        hr = ID3DXSprite_SetWorldViewRH(sprite, NULL, &mat);
        ok (hr == D3D_OK, "SetWorldViewRH returned %#lx, expected %#lx\n", hr, D3D_OK);
        hr = ID3DXSprite_SetWorldViewRH(sprite, &mat, NULL);
        ok (hr == D3D_OK, "SetWorldViewRH returned %#lx, expected %#lx\n", hr, D3D_OK);
        hr = ID3DXSprite_SetWorldViewRH(sprite, NULL, NULL);
        ok (hr == D3D_OK, "SetWorldViewRH returned %#lx, expected %#lx\n", hr, D3D_OK);
    }
    IDirect3DDevice9_BeginScene(device);

    /* Test ID3DXSprite_Begin*/
    hr = ID3DXSprite_Begin(sprite, 0);
    ok (hr == D3D_OK, "Begin returned %#lx, expected %#lx\n", hr, D3D_OK);

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
    ok (hr == D3D_OK, "Flush returned %#lx, expected %#lx\n", hr, D3D_OK);

    hr = ID3DXSprite_End(sprite);
    ok (hr == D3D_OK, "End returned %#lx, expected %#lx\n", hr, D3D_OK);

    hr = ID3DXSprite_Flush(sprite); /* May not be called before next Begin */
    ok (hr == D3DERR_INVALIDCALL, "Flush returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);
    hr = ID3DXSprite_End(sprite);
    ok (hr == D3DERR_INVALIDCALL, "End returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    /* Test ID3DXSprite_Draw */
    hr = ID3DXSprite_Begin(sprite, 0);
    ok (hr == D3D_OK, "Begin returned %#lx, expected %#lx\n", hr, D3D_OK);

    if(FAILED(hr)) skip("Couldn't ID3DXSprite_Begin, can't test ID3DXSprite_Draw\n");
    else { /* Feed the sprite batch */
        int texref1, texref2;

        SetRect(&rect, 53, 12, 142, 165);
        pos.x    =  2.2f; pos.y    = 4.5f; pos.z    = 5.1f;
        center.x = 11.3f; center.y = 3.4f; center.z = 1.2f;

        texref1 = get_ref((IUnknown*)tex1);
        texref2 = get_ref((IUnknown*)tex2);

        hr = ID3DXSprite_Draw(sprite, NULL, &rect, &center, &pos, D3DCOLOR_XRGB(255, 255, 255));
        ok (hr == D3DERR_INVALIDCALL, "Draw returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

        hr = ID3DXSprite_Draw(sprite, tex1, &rect, &center, &pos, D3DCOLOR_XRGB(255, 255, 255));
        ok (hr == D3D_OK, "Draw returned %#lx, expected %#lx\n", hr, D3D_OK);
        hr = ID3DXSprite_Draw(sprite, tex2, &rect, &center, &pos, D3DCOLOR_XRGB(  3,  45,  66));
        ok (hr == D3D_OK, "Draw returned %#lx, expected %#lx\n", hr, D3D_OK);
        hr = ID3DXSprite_Draw(sprite, tex1,  NULL, &center, &pos, D3DCOLOR_XRGB(255, 255, 255));
        ok (hr == D3D_OK, "Draw returned %#lx, expected %#lx\n", hr, D3D_OK);
        hr = ID3DXSprite_Draw(sprite, tex1, &rect,    NULL, &pos, D3DCOLOR_XRGB(255, 255, 255));
        ok (hr == D3D_OK, "Draw returned %#lx, expected %#lx\n", hr, D3D_OK);
        hr = ID3DXSprite_Draw(sprite, tex1, &rect, &center, NULL, D3DCOLOR_XRGB(255, 255, 255));
        ok (hr == D3D_OK, "Draw returned %#lx, expected %#lx\n", hr, D3D_OK);
        hr = ID3DXSprite_Draw(sprite, tex1,  NULL,    NULL, NULL,                            0);
        ok (hr == D3D_OK, "Draw returned %#lx, expected %#lx\n", hr, D3D_OK);

        check_ref((IUnknown*)tex1, texref1+5); check_ref((IUnknown*)tex2, texref2+1);
        hr = ID3DXSprite_Flush(sprite);
        ok (hr == D3D_OK, "Flush returned %#lx, expected %#lx\n", hr, D3D_OK);
        hr = ID3DXSprite_Flush(sprite);   /* Flushing twice should work */
        ok (hr == D3D_OK, "Flush returned %#lx, expected %#lx\n", hr, D3D_OK);
        check_ref((IUnknown*)tex1, texref1);   check_ref((IUnknown*)tex2, texref2);

        hr = ID3DXSprite_End(sprite);
        ok (hr == D3D_OK, "End returned %#lx, expected %#lx\n", hr, D3D_OK);
    }

    /* Test ID3DXSprite_OnLostDevice and ID3DXSprite_OnResetDevice */
    /* Both can be called twice */
    hr = ID3DXSprite_OnLostDevice(sprite);
    ok (hr == D3D_OK, "OnLostDevice returned %#lx, expected %#lx\n", hr, D3D_OK);
    hr = ID3DXSprite_OnLostDevice(sprite);
    ok (hr == D3D_OK, "OnLostDevice returned %#lx, expected %#lx\n", hr, D3D_OK);
    hr = ID3DXSprite_OnResetDevice(sprite);
    ok (hr == D3D_OK, "OnResetDevice returned %#lx, expected %#lx\n", hr, D3D_OK);
    hr = ID3DXSprite_OnResetDevice(sprite);
    ok (hr == D3D_OK, "OnResetDevice returned %#lx, expected %#lx\n", hr, D3D_OK);

    /* Make sure everything works like before */
    hr = ID3DXSprite_Begin(sprite, 0);
    ok (hr == D3D_OK, "Begin returned %#lx, expected %#lx\n", hr, D3D_OK);
    hr = ID3DXSprite_Draw(sprite, tex2, &rect, &center, &pos, D3DCOLOR_XRGB(255, 255, 255));
    ok (hr == D3D_OK, "Draw returned %#lx, expected %#lx\n", hr, D3D_OK);
    hr = ID3DXSprite_Flush(sprite);
    ok (hr == D3D_OK, "Flush returned %#lx, expected %#lx\n", hr, D3D_OK);
    hr = ID3DXSprite_End(sprite);
    ok (hr == D3D_OK, "End returned %#lx, expected %#lx\n", hr, D3D_OK);

    /* OnResetDevice makes the interface "forget" the Begin call */
    hr = ID3DXSprite_Begin(sprite, 0);
    ok (hr == D3D_OK, "Begin returned %#lx, expected %#lx\n", hr, D3D_OK);
    hr = ID3DXSprite_OnResetDevice(sprite);
    ok (hr == D3D_OK, "OnResetDevice returned %#lx, expected %#lx\n", hr, D3D_OK);
    hr = ID3DXSprite_End(sprite);
    ok (hr == D3DERR_INVALIDCALL, "End returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    /* Test D3DXSPRITE_DO_NOT_ADDREF_TEXTURE */
    hr = ID3DXSprite_Begin(sprite, D3DXSPRITE_DO_NOT_ADDREF_TEXTURE);
    ok (hr == D3D_OK, "Begin returned %#lx, expected %#lx\n", hr, D3D_OK);
    hr = ID3DXSprite_Draw(sprite, tex2, &rect, &center, &pos, D3DCOLOR_XRGB(255, 255, 255));
    ok (hr == D3D_OK, "Draw returned %#lx, expected %#lx\n", hr, D3D_OK);
    hr = ID3DXSprite_OnResetDevice(sprite);
    ok (hr == D3D_OK, "OnResetDevice returned %#lx, expected %#lx\n", hr, D3D_OK);
    check_ref((IUnknown*)tex2, 1);

    hr = ID3DXSprite_Begin(sprite, D3DXSPRITE_DO_NOT_ADDREF_TEXTURE);
    ok (hr == D3D_OK, "Begin returned %#lx, expected %#lx\n", hr, D3D_OK);
    hr = ID3DXSprite_Draw(sprite, tex2, &rect, &center, &pos, D3DCOLOR_XRGB(255, 255, 255));
    ok (hr == D3D_OK, "Draw returned %#lx, expected %#lx\n", hr, D3D_OK);
    hr = ID3DXSprite_End(sprite);
    ok (hr == D3D_OK, "End returned %#lx, expected %#lx\n", hr, D3D_OK);

    IDirect3DDevice9_EndScene(device);
    check_release((IUnknown*)sprite, 0);
    check_release((IUnknown*)tex2, 0);
    check_release((IUnknown*)tex1, 0);
}

static void test_ID3DXFont(IDirect3DDevice9 *device)
{
    static const WCHAR testW[] = L"test";
    static const char long_text[] = "Example text to test clipping and other related things";
    static const WCHAR long_textW[] = L"Example text to test clipping and other related things";
    static const MAT2 mat = { {0,1}, {0,0}, {0,0}, {0,1} };
    static const struct
    {
        int font_height;
        unsigned int expected_size;
        unsigned int expected_levels;
    }
    tests[] =
    {
        {   2,  32,  2 },
        {   6, 128,  4 },
        {  10, 256,  5 },
        {  12, 256,  5 },
        {  72, 256,  8 },
        { 250, 256,  9 },
        { 258, 512, 10 },
        { 512, 512, 10 },
    };
    const unsigned int size = ARRAY_SIZE(testW);
    TEXTMETRICA metrics, expmetrics;
    IDirect3DTexture9 *texture;
    D3DSURFACE_DESC surf_desc;
    IDirect3DDevice9 *bufdev;
    GLYPHMETRICS glyph_metrics;
    D3DXFONT_DESCA desc;
    ID3DXSprite *sprite;
    RECT rect, blackbox;
    DWORD count, levels;
    int ref, i, height;
    ID3DXFont *font;
    TEXTMETRICW tm;
    POINT cellinc;
    HRESULT hr;
    WORD glyph;
    BOOL ret;
    HDC hdc;
    char c;

    /* D3DXCreateFont */
    ref = get_ref((IUnknown*)device);
    hr = D3DXCreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma", &font);
    ok(hr == D3D_OK, "D3DXCreateFont returned %#lx, expected %#lx\n", hr, D3D_OK);
    check_ref((IUnknown*)device, ref + 1);
    check_release((IUnknown*)font, 0);
    check_ref((IUnknown*)device, ref);

    hr = D3DXCreateFontA(device, 0, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma", &font);
    ok(hr == D3D_OK, "D3DXCreateFont returned %#lx, expected %#lx\n", hr, D3D_OK);
    ID3DXFont_Release(font);

    hr = D3DXCreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, NULL, &font);
    ok(hr == D3D_OK, "D3DXCreateFont returned %#lx, expected %#lx\n", hr, D3D_OK);
    ID3DXFont_Release(font);

    hr = D3DXCreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "", &font);
    ok(hr == D3D_OK, "D3DXCreateFont returned %#lx, expected %#lx\n", hr, D3D_OK);
    ID3DXFont_Release(font);

    hr = D3DXCreateFontA(NULL, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma", &font);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateFont returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma", NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateFont returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateFontA(NULL, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma", NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateFont returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);


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
    strcpy(desc.FaceName, "Tahoma");
    hr = D3DXCreateFontIndirectA(device, &desc, &font);
    ok(hr == D3D_OK, "D3DXCreateFontIndirect returned %#lx, expected %#lx\n", hr, D3D_OK);
    ID3DXFont_Release(font);

    hr = D3DXCreateFontIndirectA(NULL, &desc, &font);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateFontIndirect returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateFontIndirectA(device, NULL, &font);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateFontIndirect returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateFontIndirectA(device, &desc, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateFontIndirect returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);


    /* ID3DXFont_GetDevice */
    hr = D3DXCreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma", &font);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID3DXFont_GetDevice(font, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

    ref = get_ref((IUnknown *)device);
    hr = ID3DXFont_GetDevice(font, &bufdev);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    check_release((IUnknown *)bufdev, ref);

    ID3DXFont_Release(font);


    /* ID3DXFont_GetDesc */
    hr = D3DXCreateFontA(device, 12, 8, FW_BOLD, 2, TRUE, ANSI_CHARSET, OUT_RASTER_PRECIS,
            ANTIALIASED_QUALITY, VARIABLE_PITCH, "Tahoma", &font);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID3DXFont_GetDescA(font, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

    hr = ID3DXFont_GetDescA(font, &desc);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    ok(desc.Height == 12, "Got unexpected height %d.\n", desc.Height);
    ok(desc.Width == 8, "Got unexpected width %u.\n", desc.Width);
    ok(desc.Weight == FW_BOLD, "Got unexpected weight %u.\n", desc.Weight);
    ok(desc.MipLevels == 2, "Got unexpected miplevels %u.\n", desc.MipLevels);
    ok(desc.Italic == TRUE, "Got unexpected italic %#x.\n", desc.Italic);
    ok(desc.CharSet == ANSI_CHARSET, "Got unexpected charset %u.\n", desc.CharSet);
    ok(desc.OutputPrecision == OUT_RASTER_PRECIS, "Got unexpected output precision %u.\n", desc.OutputPrecision);
    ok(desc.Quality == ANTIALIASED_QUALITY, "Got unexpected quality %u.\n", desc.Quality);
    ok(desc.PitchAndFamily == VARIABLE_PITCH, "Got unexpected pitch and family %#x.\n", desc.PitchAndFamily);
    ok(!strcmp(desc.FaceName, "Tahoma"), "Got unexpected facename %s.\n", debugstr_a(desc.FaceName));

    ID3DXFont_Release(font);


    /* ID3DXFont_GetDC + ID3DXFont_GetTextMetrics */
    hr = D3DXCreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma", &font);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hdc = ID3DXFont_GetDC(font);
    ok(!!hdc, "Got unexpected hdc %p.\n", hdc);

    ret = ID3DXFont_GetTextMetricsA(font, &metrics);
    ok(ret, "Got unexpected ret %#x.\n", ret);
    ret = GetTextMetricsA(hdc, &expmetrics);
    ok(ret, "Got unexpected ret %#x.\n", ret);

    ok(metrics.tmHeight == expmetrics.tmHeight, "Got unexpected height %ld, expected %ld.\n",
            metrics.tmHeight, expmetrics.tmHeight);
    ok(metrics.tmAscent == expmetrics.tmAscent, "Got unexpected ascent %ld, expected %ld.\n",
            metrics.tmAscent, expmetrics.tmAscent);
    ok(metrics.tmDescent == expmetrics.tmDescent, "Got unexpected descent %ld, expected %ld.\n",
            metrics.tmDescent, expmetrics.tmDescent);
    ok(metrics.tmInternalLeading == expmetrics.tmInternalLeading, "Got unexpected internal leading %ld, expected %ld.\n",
            metrics.tmInternalLeading, expmetrics.tmInternalLeading);
    ok(metrics.tmExternalLeading == expmetrics.tmExternalLeading, "Got unexpected external leading %ld, expected %ld.\n",
            metrics.tmExternalLeading, expmetrics.tmExternalLeading);
    ok(metrics.tmAveCharWidth == expmetrics.tmAveCharWidth, "Got unexpected average char width %ld, expected %ld.\n",
            metrics.tmAveCharWidth, expmetrics.tmAveCharWidth);
    ok(metrics.tmMaxCharWidth == expmetrics.tmMaxCharWidth, "Got unexpected maximum char width %ld, expected %ld.\n",
            metrics.tmMaxCharWidth, expmetrics.tmMaxCharWidth);
    ok(metrics.tmWeight == expmetrics.tmWeight, "Got unexpected weight %ld, expected %ld.\n",
            metrics.tmWeight, expmetrics.tmWeight);
    ok(metrics.tmOverhang == expmetrics.tmOverhang, "Got unexpected overhang %ld, expected %ld.\n",
            metrics.tmOverhang, expmetrics.tmOverhang);
    ok(metrics.tmDigitizedAspectX == expmetrics.tmDigitizedAspectX, "Got unexpected digitized x aspect %ld, expected %ld.\n",
            metrics.tmDigitizedAspectX, expmetrics.tmDigitizedAspectX);
    ok(metrics.tmDigitizedAspectY == expmetrics.tmDigitizedAspectY, "Got unexpected digitized y aspect %ld, expected %ld.\n",
            metrics.tmDigitizedAspectY, expmetrics.tmDigitizedAspectY);
    ok(metrics.tmFirstChar == expmetrics.tmFirstChar, "Got unexpected first char %u, expected %u.\n",
            metrics.tmFirstChar, expmetrics.tmFirstChar);
    ok(metrics.tmLastChar == expmetrics.tmLastChar, "Got unexpected last char %u, expected %u.\n",
            metrics.tmLastChar, expmetrics.tmLastChar);
    ok(metrics.tmDefaultChar == expmetrics.tmDefaultChar, "Got unexpected default char %u, expected %u.\n",
            metrics.tmDefaultChar, expmetrics.tmDefaultChar);
    ok(metrics.tmBreakChar == expmetrics.tmBreakChar, "Got unexpected break char %u, expected %u.\n",
            metrics.tmBreakChar, expmetrics.tmBreakChar);
    ok(metrics.tmItalic == expmetrics.tmItalic, "Got unexpected italic %u, expected %u.\n",
            metrics.tmItalic, expmetrics.tmItalic);
    ok(metrics.tmUnderlined == expmetrics.tmUnderlined, "Got unexpected underlined %u, expected %u.\n",
            metrics.tmUnderlined, expmetrics.tmUnderlined);
    ok(metrics.tmStruckOut == expmetrics.tmStruckOut, "Got unexpected struck out %u, expected %u.\n",
            metrics.tmStruckOut, expmetrics.tmStruckOut);
    ok(metrics.tmPitchAndFamily == expmetrics.tmPitchAndFamily, "Got unexpected pitch and family %u, expected %u.\n",
            metrics.tmPitchAndFamily, expmetrics.tmPitchAndFamily);
    ok(metrics.tmCharSet == expmetrics.tmCharSet, "Got unexpected charset %u, expected %u.\n",
            metrics.tmCharSet, expmetrics.tmCharSet);

    ID3DXFont_Release(font);


    /* ID3DXFont_PreloadText */
    hr = D3DXCreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma", &font);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID3DXFont_PreloadTextA(font, NULL, -1);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DXFont_PreloadTextA(font, NULL, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DXFont_PreloadTextA(font, NULL, 1);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DXFont_PreloadTextA(font, "test", -1);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DXFont_PreloadTextA(font, "", 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DXFont_PreloadTextA(font, "", -1);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID3DXFont_PreloadTextW(font, NULL, -1);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DXFont_PreloadTextW(font, NULL, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DXFont_PreloadTextW(font, NULL, 1);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DXFont_PreloadTextW(font, testW, -1);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DXFont_PreloadTextW(font, L"", 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DXFont_PreloadTextW(font, L"", -1);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    check_release((IUnknown*)font, 0);


    /* ID3DXFont_GetGlyphData, ID3DXFont_PreloadGlyphs, ID3DXFont_PreloadCharacters */
    hr = D3DXCreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma", &font);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hdc = ID3DXFont_GetDC(font);
    ok(!!hdc, "Got unexpected hdc %p.\n", hdc);

    hr = ID3DXFont_GetGlyphData(font, 0, NULL, &blackbox, &cellinc);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DXFont_GetGlyphData(font, 0, &texture, NULL, &cellinc);
    check_release((IUnknown *)texture, 1);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DXFont_GetGlyphData(font, 0, &texture, &blackbox, NULL);
    check_release((IUnknown *)texture, 1);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID3DXFont_PreloadCharacters(font, 'b', 'a');
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DXFont_PreloadGlyphs(font, 1, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID3DXFont_PreloadCharacters(font, 'a', 'a');
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    for (c = 'b'; c <= 'z'; ++c)
    {
        winetest_push_context("Character %c", c);
        count = GetGlyphIndicesA(hdc, &c, 1, &glyph, 0);
        ok(count != GDI_ERROR, "Unexpected count %lu.\n", count);

        hr = ID3DXFont_GetGlyphData(font, glyph, &texture, &blackbox, &cellinc);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

        levels = IDirect3DTexture9_GetLevelCount(texture);
        todo_wine ok(levels == 5, "Unexpected levels %lu.\n", levels);
        hr = IDirect3DTexture9_GetLevelDesc(texture, 0, &surf_desc);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
        ok(surf_desc.Format == D3DFMT_A8R8G8B8, "Unexpected format %#x.\n", surf_desc.Format);
        ok(surf_desc.Usage == 0, "Unexpected usage %#lx.\n", surf_desc.Usage);
        ok(surf_desc.Width == 256, "Unexpected width %u.\n", surf_desc.Width);
        ok(surf_desc.Height == 256, "Unexpected height %u.\n", surf_desc.Height);
        ok(surf_desc.Pool == D3DPOOL_MANAGED, "Unexpected pool %u.\n", surf_desc.Pool);

        count = GetGlyphOutlineW(hdc, glyph, GGO_GLYPH_INDEX | GGO_METRICS, &glyph_metrics, 0, NULL, &mat);
        ok(count != GDI_ERROR, "Unexpected count %#lx.\n", count);

        ret = ID3DXFont_GetTextMetricsW(font, &tm);
        ok(ret, "Unexpected ret %#x.\n", ret);

        todo_wine ok(blackbox.right - blackbox.left == glyph_metrics.gmBlackBoxX + 2, "Got %ld, expected %d.\n",
                blackbox.right - blackbox.left, glyph_metrics.gmBlackBoxX + 2);
        todo_wine ok(blackbox.bottom - blackbox.top == glyph_metrics.gmBlackBoxY + 2, "Got %ld, expected %d.\n",
                blackbox.bottom - blackbox.top, glyph_metrics.gmBlackBoxY + 2);
        ok(cellinc.x == glyph_metrics.gmptGlyphOrigin.x - 1, "Got %ld, expected %ld.\n",
                cellinc.x, glyph_metrics.gmptGlyphOrigin.x - 1);
        ok(cellinc.y == tm.tmAscent - glyph_metrics.gmptGlyphOrigin.y - 1, "Got %ld, expected %ld.\n",
                cellinc.y, tm.tmAscent - glyph_metrics.gmptGlyphOrigin.y - 1);

        check_release((IUnknown *)texture, 1);
        winetest_pop_context();
    }

    hr = ID3DXFont_PreloadCharacters(font, 'a', 'z');
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    /* Test multiple textures */
    hr = ID3DXFont_PreloadGlyphs(font, 0, 1000);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    /* Test glyphs that are not rendered */
    for (glyph = 1; glyph < 4; ++glyph)
    {
        texture = (IDirect3DTexture9 *)0xdeadbeef;
        hr = ID3DXFont_GetGlyphData(font, glyph, &texture, &blackbox, &cellinc);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
        ok(!texture, "Got unexpected texture %p.\n", texture);
    }

    check_release((IUnknown *)font, 0);

    c = 'a';
    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        winetest_push_context("Test %u", i);
        hr = D3DXCreateFontA(device, tests[i].font_height, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma", &font);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

        hdc = ID3DXFont_GetDC(font);
        ok(!!hdc, "Unexpected hdc %p.\n", hdc);

        count = GetGlyphIndicesA(hdc, &c, 1, &glyph, 0);
        ok(count != GDI_ERROR, "Unexpected count %lu.\n", count);

        hr = ID3DXFont_GetGlyphData(font, glyph, &texture, NULL, NULL);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

        levels = IDirect3DTexture9_GetLevelCount(texture);
        todo_wine_if(tests[i].expected_levels < 9)
        ok(levels == tests[i].expected_levels, "Unexpected levels %lu.\n", levels);

        hr = IDirect3DTexture9_GetLevelDesc(texture, 0, &surf_desc);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
        ok(surf_desc.Format == D3DFMT_A8R8G8B8, "Unexpected format %#x.\n", surf_desc.Format);
        ok(surf_desc.Usage == 0, "Unexpected usage %#lx.\n", surf_desc.Usage);
        ok(surf_desc.Width == tests[i].expected_size, "Unexpected width %u.\n", surf_desc.Width);
        ok(surf_desc.Height == tests[i].expected_size, "Unexpected height %u.\n", surf_desc.Height);
        ok(surf_desc.Pool == D3DPOOL_MANAGED, "Unexpected pool %u.\n", surf_desc.Pool);

        IDirect3DTexture9_Release(texture);

        /* ID3DXFontImpl_DrawText */
        D3DXCreateSprite(device, &sprite);
        SetRect(&rect, 0, 0, 640, 480);

        IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff000000, 1.0f, 0);

        IDirect3DDevice9_BeginScene(device);
        hr = ID3DXSprite_Begin(sprite, D3DXSPRITE_ALPHABLEND);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

        height = ID3DXFont_DrawTextW(font, sprite, testW, -1, &rect, DT_TOP, 0xffffffff);
        ok(height == tests[i].font_height, "Unexpected height %u.\n", height);
        height = ID3DXFont_DrawTextW(font, sprite, testW, size, &rect, DT_TOP, 0xffffffff);
        ok(height == tests[i].font_height, "Unexpected height %u.\n", height);
        height = ID3DXFont_DrawTextW(font, sprite, testW, size, &rect, DT_RIGHT, 0xffffffff);
        ok(height == tests[i].font_height, "Unexpected height %u.\n", height);
        height = ID3DXFont_DrawTextW(font, sprite, testW, size, &rect, DT_LEFT | DT_NOCLIP, 0xffffffff);
        ok(height == tests[i].font_height, "Unexpected height %u.\n", height);

        SetRectEmpty(&rect);
        height = ID3DXFont_DrawTextW(font, sprite, testW, size, &rect,
                DT_LEFT | DT_CALCRECT, 0xffffffff);
        ok(height == tests[i].font_height, "Unexpected height %u.\n", height);
        ok(!rect.left, "Unexpected rect left %ld.\n", rect.left);
        ok(!rect.top, "Unexpected rect top %ld.\n", rect.top);
        ok(rect.right, "Unexpected rect right %ld.\n", rect.right);
        ok(rect.bottom == tests[i].font_height, "Unexpected rect bottom %ld.\n", rect.bottom);

        hr = ID3DXSprite_End(sprite);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
        IDirect3DDevice9_EndScene(device);
        ID3DXSprite_Release(sprite);

        ID3DXFont_Release(font);
        winetest_pop_context();
    }


    /* ID3DXFont_DrawTextA, ID3DXFont_DrawTextW */
    hr = D3DXCreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma", &font);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

    SetRect(&rect, 10, 10, 200, 200);

    height = ID3DXFont_DrawTextA(font, NULL, "test", -2, &rect, 0, 0xFF00FF);
    ok(height == 12, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextA(font, NULL, "test", -1, &rect, 0, 0xFF00FF);
    ok(height == 12, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextA(font, NULL, "test", 0, &rect, 0, 0xFF00FF);
    ok(height == 0, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextA(font, NULL, "test", 1, &rect, 0, 0xFF00FF);
    ok(height == 12, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextA(font, NULL, "test", 2, &rect, 0, 0xFF00FF);
    ok(height == 12, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextA(font, NULL, "", 0, &rect, 0, 0xff00ff);
    ok(height == 0, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextA(font, NULL, "", -1, &rect, 0, 0xff00ff);
    ok(height == 0, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextA(font, NULL, "test", -1, NULL, 0, 0xFF00FF);
    ok(height == 12, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextA(font, NULL, "test", -1, NULL, DT_CALCRECT, 0xFF00FF);
    ok(height == 12, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextA(font, NULL, NULL, -1, NULL, 0, 0xFF00FF);
    ok(height == 0, "Got unexpected height %d.\n", height);

    SetRect(&rect, 10, 10, 50, 50);

    height = ID3DXFont_DrawTextA(font, NULL, long_text, -1, &rect, DT_WORDBREAK, 0xff00ff);
    todo_wine ok(height == 60, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextA(font, NULL, long_text, -1, &rect, DT_WORDBREAK | DT_NOCLIP, 0xff00ff);
    ok(height == 96, "Got unexpected height %d.\n", height);

    SetRect(&rect, 10, 10, 200, 200);

    height = ID3DXFont_DrawTextW(font, NULL, testW, -1, &rect, 0, 0xFF00FF);
    ok(height == 12, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, testW, 0, &rect, 0, 0xFF00FF);
    ok(height == 0, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, testW, 1, &rect, 0, 0xFF00FF);
    ok(height == 12, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, testW, 2, &rect, 0, 0xFF00FF);
    ok(height == 12, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, L"", 0, &rect, 0, 0xff00ff);
    ok(height == 0, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, L"", -1, &rect, 0, 0xff00ff);
    ok(height == 0, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, testW, -1, NULL, 0, 0xFF00FF);
    ok(height == 12, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, testW, -1, NULL, DT_CALCRECT, 0xFF00FF);
    ok(height == 12, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, NULL, -1, NULL, 0, 0xFF00FF);
    ok(height == 0, "Got unexpected height %d.\n", height);

    SetRect(&rect, 10, 10, 50, 50);

    height = ID3DXFont_DrawTextW(font, NULL, long_textW, -1, &rect, DT_WORDBREAK, 0xff00ff);
    todo_wine ok(height == 60, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, long_textW, -1, &rect, DT_WORDBREAK | DT_NOCLIP, 0xff00ff);
    ok(height == 96, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, L"a\na", -1, NULL, 0, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, L"a\na", -1, &rect, 0, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, L"a\r\na", -1, &rect, 0, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, L"a\ra", -1, &rect, 0, 0xff00ff);
    ok(height == 12, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, L"a\na", -1, &rect, DT_SINGLELINE, 0xff00ff);
    ok(height == 12, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, L"a\naaaaa aaaa", -1, &rect, DT_SINGLELINE, 0xff00ff);
    ok(height == 12, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, L"a\naaaaa aaaa", -1, &rect, 0, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, L"a\naaaaa aaaa", -1, &rect, DT_WORDBREAK, 0xff00ff);
    ok(height == 36, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, L"a\naaaaa aaaa", -1, &rect, DT_WORDBREAK | DT_SINGLELINE, 0xff00ff);
    ok(height == 12, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, L"1\n2\n3\n4\n5\n6", -1, &rect, 0, 0xff00ff);
    ok(height == 48, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, L"1\n2\n3\n4\n5\n6", -1, &rect, DT_NOCLIP, 0xff00ff);
    ok(height == 72, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, L"\t\t\t\t\t\t\t\t\t\t", -1, &rect, DT_WORDBREAK, 0xff00ff);
    todo_wine ok(height == 0, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, L"\t\t\t\t\t\t\t\t\t\ta", -1, &rect, DT_WORDBREAK, 0xff00ff);
    todo_wine ok(height == 12, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, L"\taaaaaaaaaa", -1, &rect, DT_WORDBREAK, 0xff00ff);
    todo_wine ok(height == 24, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, L"\taaaaaaaaaa", -1, &rect, DT_EXPANDTABS | DT_WORDBREAK, 0xff00ff);
    ok(height == 36, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, L"\taaa\taaa\taaa", -1, &rect, DT_WORDBREAK, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, L"\taaa\taaa\taaa", -1, &rect, DT_EXPANDTABS | DT_WORDBREAK, 0xff00ff);
    todo_wine ok(height == 48, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, L"\t\t\t\t\t\t\t\t\t\t", -1, &rect, DT_EXPANDTABS | DT_WORDBREAK, 0xff00ff);
    todo_wine ok(height == 60, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, L"a\ta", -1, &rect, DT_EXPANDTABS | DT_WORDBREAK, 0xff00ff);
    ok(height == 12, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, L"a\ta\ta", -1, &rect, DT_EXPANDTABS | DT_WORDBREAK, 0xff00ff);
    todo_wine ok(height == 24, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, L"aaaaaaaaaaaaaaaaaaaa", -1, &rect, DT_WORDBREAK, 0xff00ff);
    ok(height == 36, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, L"a                        a", -1, &rect, DT_WORDBREAK, 0xff00ff);
    ok(height == 36, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa              aaaa", -1, &rect, DT_WORDBREAK, 0xff00ff);
    ok(height == 36, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa              aaaa", -1, &rect, DT_WORDBREAK | DT_RIGHT, 0xff00ff);
    ok(height == 36, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa              aaaa", -1, &rect, DT_WORDBREAK | DT_CENTER, 0xff00ff);
    ok(height == 36, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_BOTTOM, 0xff00ff);
    ok(height == 40, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_VCENTER, 0xff00ff);
    ok(height == 32, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_RIGHT, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);

    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CENTER, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);

    SetRect(&rect, 10, 10, 50, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CALCRECT, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);
    check_rect(&rect, 10, 10, 30, 34);

    SetRect(&rect, -10, 10, 30, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CALCRECT, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);
    check_rect(&rect, -10, 10, 10, 34);

    SetRect(&rect, 10, -10, 50, 30);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CALCRECT, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);
    check_rect(&rect, 10, -10, 30, 14);

    SetRect(&rect, 10, 10, -30, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CALCRECT, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);
    check_rect(&rect, 10, 10, 30, 34);

    SetRect(&rect, 10, 10, 50, -30);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CALCRECT, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);
    check_rect(&rect, 10, 10, 30, 34);

    SetRect(&rect, 10, 10, 50, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_WORDBREAK | DT_CALCRECT, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);
    check_rect(&rect, 10, 10, 30, 34);

    SetRect(&rect, -10, 10, 30, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_WORDBREAK | DT_CALCRECT, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);
    check_rect(&rect, -10, 10, 10, 34);

    SetRect(&rect, 10, -10, 50, 30);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_WORDBREAK | DT_CALCRECT, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);
    check_rect(&rect, 10, -10, 30, 14);

    SetRect(&rect, 10, 10, -30, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_WORDBREAK | DT_CALCRECT, 0xff00ff);
    ok(height == 12, "Got unexpected height %d.\n", height);
    check_rect(&rect, 10, 10, 53, 22);

    SetRect(&rect, 10, 10, 50, -30);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_WORDBREAK | DT_CALCRECT, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);
    check_rect(&rect, 10, 10, 30, 34);

    SetRect(&rect, 10, 10, 50, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_BOTTOM | DT_CALCRECT, 0xff00ff);
    ok(height == 40, "Got unexpected height %d.\n", height);
    check_rect(&rect, 10, 26, 30, 50);

    SetRect(&rect, -10, 10, 30, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_BOTTOM | DT_CALCRECT, 0xff00ff);
    ok(height == 40, "Got unexpected height %d.\n", height);
    check_rect(&rect, -10, 26, 10, 50);

    SetRect(&rect, 10, -10, 50, 30);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_BOTTOM | DT_CALCRECT, 0xff00ff);
    ok(height == 40, "Got unexpected height %d.\n", height);
    check_rect(&rect, 10, 6, 30, 30);

    SetRect(&rect, 10, 10, -30, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_BOTTOM | DT_CALCRECT, 0xff00ff);
    ok(height == 40, "Got unexpected height %d.\n", height);
    check_rect(&rect, 10, 26, 30, 50);

    SetRect(&rect, 10, 10, 50, -30);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_BOTTOM | DT_CALCRECT, 0xff00ff);
    ok(height == -40, "Got unexpected height %d.\n", height);
    check_rect(&rect, 10, -54, 30, -30);

    SetRect(&rect, 10, 10, 50, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_BOTTOM | DT_WORDBREAK | DT_CALCRECT, 0xff00ff);
    ok(height == 40, "Got unexpected height %d.\n", height);
    check_rect(&rect, 10, 26, 30, 50);

    SetRect(&rect, -10, 10, 30, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_BOTTOM | DT_WORDBREAK | DT_CALCRECT, 0xff00ff);
    ok(height == 40, "Got unexpected height %d.\n", height);
    check_rect(&rect, -10, 26, 10, 50);

    SetRect(&rect, 10, -10, 50, 30);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_BOTTOM | DT_WORDBREAK | DT_CALCRECT, 0xff00ff);
    ok(height == 40, "Got unexpected height %d.\n", height);
    check_rect(&rect, 10, 6, 30, 30);

    SetRect(&rect, 10, 10, -30, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_BOTTOM | DT_WORDBREAK | DT_CALCRECT, 0xff00ff);
    ok(height == 40, "Got unexpected height %d.\n", height);
    check_rect(&rect, 10, 38, 53, 50);

    SetRect(&rect, 10, 10, 50, -30);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_BOTTOM | DT_WORDBREAK | DT_CALCRECT, 0xff00ff);
    ok(height == -40, "Got unexpected height %d.\n", height);
    check_rect(&rect, 10, -54, 30, -30);

    SetRect(&rect, 10, 10, 50, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_VCENTER | DT_CALCRECT, 0xff00ff);
    ok(height == 32, "Got unexpected height %d.\n", height);
    check_rect(&rect, 10, 18, 30, 42);

    SetRect(&rect, -10, 10, 30, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_VCENTER | DT_CALCRECT, 0xff00ff);
    ok(height == 32, "Got unexpected height %d.\n", height);
    check_rect(&rect, -10, 18, 10, 42);

    SetRect(&rect, 10, -10, 50, 30);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_VCENTER | DT_CALCRECT, 0xff00ff);
    ok(height == 32, "Got unexpected height %d.\n", height);
    check_rect(&rect, 10, -2, 30, 22);

    SetRect(&rect, 10, 10, -30, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_VCENTER | DT_CALCRECT, 0xff00ff);
    ok(height == 32, "Got unexpected height %d.\n", height);
    check_rect(&rect, 10, 18, 30, 42);

    SetRect(&rect, 10, 10, 50, -30);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_VCENTER | DT_CALCRECT, 0xff00ff);
    ok(height == -8, "Got unexpected height %d.\n", height);
    check_rect(&rect, 10, -22, 30, 2);

    SetRect(&rect, 10, 10, 50, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_VCENTER | DT_WORDBREAK | DT_CALCRECT, 0xff00ff);
    ok(height == 32, "Got unexpected height %d.\n", height);
    check_rect(&rect, 10, 18, 30, 42);

    SetRect(&rect, -10, 10, 30, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_VCENTER | DT_WORDBREAK | DT_CALCRECT, 0xff00ff);
    ok(height == 32, "Got unexpected height %d.\n", height);
    check_rect(&rect, -10, 18, 10, 42);

    SetRect(&rect, 10, -10, 50, 30);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_VCENTER | DT_WORDBREAK | DT_CALCRECT, 0xff00ff);
    ok(height == 32, "Got unexpected height %d.\n", height);
    check_rect(&rect, 10, -2, 30, 22);

    SetRect(&rect, 10, 10, -30, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_VCENTER | DT_WORDBREAK | DT_CALCRECT, 0xff00ff);
    ok(height == 26, "Got unexpected height %d.\n", height);
    check_rect(&rect, 10, 24, 53, 36);

    SetRect(&rect, 10, 10, 50, -30);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_VCENTER | DT_WORDBREAK | DT_CALCRECT, 0xff00ff);
    ok(height == -8, "Got unexpected height %d.\n", height);
    check_rect(&rect, 10, -22, 30, 2);

    SetRect(&rect, 10, 10, 50, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_RIGHT | DT_CALCRECT, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);
    check_rect(&rect, 30, 10, 50, 34);

    SetRect(&rect, -10, 10, 30, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_RIGHT | DT_CALCRECT, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);
    check_rect(&rect, 10, 10, 30, 34);

    SetRect(&rect, 10, -10, 50, 30);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_RIGHT | DT_CALCRECT, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);
    check_rect(&rect, 30, -10, 50, 14);

    SetRect(&rect, 10, 10, -30, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_RIGHT | DT_CALCRECT, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);
    check_rect(&rect, -50, 10, -30, 34);

    SetRect(&rect, 10, 10, 50, -30);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_RIGHT | DT_CALCRECT, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);
    check_rect(&rect, 30, 10, 50, 34);

    SetRect(&rect, 10, 10, 50, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_RIGHT | DT_WORDBREAK | DT_CALCRECT, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);
    check_rect(&rect, 30, 10, 50, 34);

    SetRect(&rect, -10, 10, 30, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_RIGHT | DT_WORDBREAK | DT_CALCRECT, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);
    check_rect(&rect, 10, 10, 30, 34);

    SetRect(&rect, 10, -10, 50, 30);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_RIGHT | DT_WORDBREAK | DT_CALCRECT, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);
    check_rect(&rect, 30, -10, 50, 14);

    SetRect(&rect, 10, 10, -30, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_RIGHT | DT_WORDBREAK | DT_CALCRECT, 0xff00ff);
    ok(height == 12, "Got unexpected height %d.\n", height);
    check_rect(&rect, -73, 10, -30, 22);

    SetRect(&rect, 10, 10, 50, -30);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_RIGHT | DT_WORDBREAK | DT_CALCRECT, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);
    check_rect(&rect, 30, 10, 50, 34);

    SetRect(&rect, 10, 10, 50, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CENTER | DT_CALCRECT, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);
    check_rect(&rect, 20, 10, 40, 34);

    SetRect(&rect, -10, 10, 30, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CENTER | DT_CALCRECT, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);
    check_rect(&rect, 0, 10, 20, 34);

    SetRect(&rect, 10, -10, 50, 30);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CENTER | DT_CALCRECT, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);
    check_rect(&rect, 20, -10, 40, 14);

    SetRect(&rect, 10, 10, -30, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CENTER | DT_CALCRECT, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);
    check_rect(&rect, -20, 10, 0, 34);

    SetRect(&rect, 10, 10, 50, -30);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CENTER | DT_CALCRECT, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);
    check_rect(&rect, 20, 10, 40, 34);

    SetRect(&rect, 10, 10, 50, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_CENTER | DT_WORDBREAK | DT_CALCRECT, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);
    check_rect(&rect, 20, 10, 40, 34);

    SetRect(&rect, -10, 10, 30, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_CENTER | DT_WORDBREAK | DT_CALCRECT, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);
    check_rect(&rect, 0, 10, 20, 34);

    SetRect(&rect, 10, -10, 50, 30);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_CENTER | DT_WORDBREAK | DT_CALCRECT, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);
    check_rect(&rect, 20, -10, 40, 14);

    SetRect(&rect, 10, 10, -30, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_CENTER | DT_WORDBREAK | DT_CALCRECT, 0xff00ff);
    ok(height == 12, "Got unexpected height %d.\n", height);
    check_rect(&rect, -31, 10, 12, 22);

    SetRect(&rect, 10, 10, 50, -30);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_CENTER | DT_WORDBREAK | DT_CALCRECT, 0xff00ff);
    ok(height == 24, "Got unexpected height %d.\n", height);
    check_rect(&rect, 20, 10, 40, 34);

    SetRect(&rect, 10, 10, 50, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CENTER | DT_VCENTER | DT_CALCRECT, 0xff00ff);
    ok(height == 32, "Got unexpected height %d.\n", height);
    check_rect(&rect, 20, 18, 40, 42);

    SetRect(&rect, 10, 10, 50, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CENTER | DT_VCENTER | DT_CALCRECT, 0xff00ff);
    ok(height == 32, "Got unexpected height %d.\n", height);
    check_rect(&rect, 20, 18, 40, 42);

    SetRect(&rect, -10, 10, 30, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CENTER | DT_VCENTER | DT_CALCRECT, 0xff00ff);
    ok(height == 32, "Got unexpected height %d.\n", height);
    check_rect(&rect, 0, 18, 20, 42);

    SetRect(&rect, 10, -10, 50, 30);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CENTER | DT_VCENTER | DT_CALCRECT, 0xff00ff);
    ok(height == 32, "Got unexpected height %d.\n", height);
    check_rect(&rect, 20, -2, 40, 22);

    SetRect(&rect, 10, 10, -30, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CENTER | DT_VCENTER | DT_CALCRECT, 0xff00ff);
    ok(height == 32, "Got unexpected height %d.\n", height);
    check_rect(&rect, -20, 18, 0, 42);

    SetRect(&rect, 10, 10, 50, -30);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CENTER | DT_VCENTER | DT_CALCRECT, 0xff00ff);
    ok(height == -8, "Got unexpected height %d.\n", height);
    check_rect(&rect, 20, -22, 40, 2);

    SetRect(&rect, 10, 10, 50, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_CALCRECT, 0xff00ff);
    ok(height == 32, "Got unexpected height %d.\n", height);
    check_rect(&rect, 20, 18, 40, 42);

    SetRect(&rect, 10, 10, 50, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_CALCRECT, 0xff00ff);
    ok(height == 32, "Got unexpected height %d.\n", height);
    check_rect(&rect, 20, 18, 40, 42);

    SetRect(&rect, -10, 10, 30, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_CALCRECT, 0xff00ff);
    ok(height == 32, "Got unexpected height %d.\n", height);
    check_rect(&rect, 0, 18, 20, 42);

    SetRect(&rect, 10, -10, 50, 30);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_CALCRECT, 0xff00ff);
    ok(height == 32, "Got unexpected height %d.\n", height);
    check_rect(&rect, 20, -2, 40, 22);

    SetRect(&rect, 10, 10, -30, 50);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_CALCRECT, 0xff00ff);
    ok(height == 26, "Got unexpected height %d.\n", height);
    check_rect(&rect, -31, 24, 12, 36);

    SetRect(&rect, 10, 10, 50, -30);
    height = ID3DXFont_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_CALCRECT, 0xff00ff);
    ok(height == -8, "Got unexpected height %d.\n", height);
    check_rect(&rect, 20, -22, 40, 2);

    ID3DXFont_Release(font);
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
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);
    ok(render == (void *)0xdeadbeef, "Got %p, expected %p\n", render, (void *)0xdeadbeef);

    hr = D3DXCreateRenderToSurface(device, 256, 256, D3DFMT_A8R8G8B8, FALSE, D3DFMT_UNKNOWN, NULL /* out */);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        winetest_push_context("Test %u", i);
        hr = D3DXCreateRenderToSurface(device, tests[i].Width, tests[i].Height, tests[i].Format, tests[i].DepthStencil,
                tests[i].DepthStencilFormat, &render);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
        hr = ID3DXRenderToSurface_GetDesc(render, &desc);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
        ok(desc.Width == tests[i].Width, "Width %u, expected %u.\n", desc.Width, tests[i].Width);
        ok(desc.Height == tests[i].Height, "Height %u, expected %u.\n", desc.Height, tests[i].Height);
        ok(desc.Format == tests[i].Format, "Format %#x, expected %#x.\n", desc.Format, tests[i].Format);
        ok(desc.DepthStencil == tests[i].DepthStencil, "Depth stencil %d, expected %d.\n",
                desc.DepthStencil, tests[i].DepthStencil);
        ok(desc.DepthStencilFormat == tests[i].DepthStencilFormat, "Depth stencil format %#x, expected %#x.\n",
                desc.DepthStencilFormat, tests[i].DepthStencilFormat);
        ID3DXRenderToSurface_Release(render);
        winetest_pop_context();
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
    ok(hr == D3D_OK, "ID3DXRenderToSurface::BeginScene returned %#lx, expected %#lx\n", hr, D3D_OK);
    check_ref((IUnknown *)surface, 2);
    if (SUCCEEDED(hr)) ID3DXRenderToSurface_EndScene(render, D3DX_FILTER_NONE);

    /* invalid viewport */
    viewport.Width = 2 * width;
    hr = ID3DXRenderToSurface_BeginScene(render, surface, &viewport);
    ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToSurface::BeginScene returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    viewport.X = width / 2;
    viewport.Width = width;
    hr = ID3DXRenderToSurface_BeginScene(render, surface, &viewport);
    ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToSurface::BeginScene returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    viewport.X = width;
    viewport.Width = width;
    hr = ID3DXRenderToSurface_BeginScene(render, surface, &viewport);
    ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToSurface::BeginScene returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    /* rendering to a part of a surface is only allowed for render target surfaces */
    expected_value = render_target ? D3D_OK : D3DERR_INVALIDCALL;

    viewport.X = 0;
    viewport.Width = width / 2;
    hr = ID3DXRenderToSurface_BeginScene(render, surface, &viewport);
    ok(hr == expected_value, "ID3DXRenderToSurface::BeginScene returned %#lx, expected %#lx\n", hr, expected_value);
    if (SUCCEEDED(hr)) ID3DXRenderToSurface_EndScene(render, D3DX_FILTER_NONE);

    viewport.X = width / 2;
    viewport.Width = width - width / 2;
    hr = ID3DXRenderToSurface_BeginScene(render, surface, &viewport);
    ok(hr == expected_value, "ID3DXRenderToSurface::BeginScene returned %#lx, expected %#lx\n", hr, expected_value);
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
    ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToSurface::BeginScene returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);
    check_release((IUnknown *)surface, 0);

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 2 * width, 2 * height, format, D3DPOOL_DEFAULT, &surface, NULL);
    if (FAILED(hr))
    {
        skip("Failed to create surface\n");
        ID3DXRenderToSurface_Release(render);
        return;
    }
    hr = ID3DXRenderToSurface_BeginScene(render, surface, NULL);
    ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToSurface::BeginScene returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);
    viewport.X = 0;
    viewport.Y = 0;
    viewport.Width = width;
    viewport.Height = height;
    hr = ID3DXRenderToSurface_BeginScene(render, surface, &viewport);
    ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToSurface::BeginScene returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);
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
        ok(hr == expected_result, "ID3DXRenderToSurface::BeginScene returned %#lx, expected %#lx\n", hr, expected_result);

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
    ok(equal ? cmp : !cmp, "Viewport %s (%lu, %lu, %lu, %lu), (%lu, %lu, %lu, %lu)\n", message,
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
    ok(hr == D3D_OK, "D3DXCreateRenderToSurface returned %#lx, expected %#lx\n", hr, D3D_OK);
    if (SUCCEEDED(hr))
    {
        hr = ID3DXRenderToSurface_BeginScene(render, surface, NULL);
        ok(hr == D3D_OK, "ID3DXRenderToSurface::BeginScene returned %#lx, expected %#lx\n", hr, D3D_OK);

        hr = retrieve_device_state(device, &current_state);
        ok(SUCCEEDED(hr), "Failed to retrieve device state\n");
        compare_device_state(&current_state, &pre_state, FALSE);
        release_device_state(&current_state);

        hr = ID3DXRenderToSurface_EndScene(render, D3DX_FILTER_NONE);
        ok(hr == D3D_OK, "ID3DXRenderToSurface::EndScene returned %#lx, expected %#lx\n", hr, D3D_OK);

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
        ok(hr == D3D_OK, "ID3DXRenderToSurface::BeginScene returned %#lx, expected %#lx\n", hr, D3D_OK);

        hr = retrieve_device_state(device, &current_state);
        ok(SUCCEEDED(hr), "Failed to retrieve device state\n");
        compare_device_state(&current_state, &pre_state, FALSE);
        release_device_state(&current_state);

        hr = ID3DXRenderToSurface_EndScene(render, D3DX_FILTER_NONE);
        ok(hr == D3D_OK, "ID3DXRenderToSurface::EndScene returned %#lx, expected %#lx\n", hr, D3D_OK);

        hr = retrieve_device_state(device, &current_state);
        ok(SUCCEEDED(hr), "Failed to retrieve device state\n");
        compare_device_state(&current_state, &pre_state, TRUE);
        release_device_state(&current_state);

        hr = ID3DXRenderToSurface_BeginScene(render, surface, NULL);
        ok(hr == D3D_OK, "ID3DXRenderToSurface::BeginScene returned %#lx, expected %#lx\n", hr, D3D_OK);

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
    ok(hr == D3D_OK, "D3DXCreateRenderToSurface returned %#lx, expected %#lx\n", hr, D3D_OK);
    if (FAILED(hr)) return;

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 256, 256, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &surface, NULL);
    if (SUCCEEDED(hr))
    {
        ID3DXRenderToSurface *render_surface;

        /* GetDevice */
        hr = ID3DXRenderToSurface_GetDevice(render, NULL /* device */);
        ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToSurface::GetDevice returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

        ref_count = get_ref((IUnknown *)device);
        hr = ID3DXRenderToSurface_GetDevice(render, &out_device);
        ok(hr == D3D_OK, "ID3DXRenderToSurface::GetDevice returned %#lx, expected %#lx\n", hr, D3D_OK);
        check_release((IUnknown *)out_device, ref_count);

        /* BeginScene and EndScene */
        hr = ID3DXRenderToSurface_EndScene(render, D3DX_FILTER_NONE);
        ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToSurface::EndScene returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

        hr = ID3DXRenderToSurface_BeginScene(render, NULL /* surface */, &viewport);
        ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToSurface::BeginScene returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

        ref_count = get_ref((IUnknown *)surface);
        hr = ID3DXRenderToSurface_BeginScene(render, surface, NULL);
        ok(hr == D3D_OK, "ID3DXRenderToSurface::BeginScene returned %#lx, expected %#lx\n", hr, D3D_OK);
        if (SUCCEEDED(hr))
        {
            check_ref((IUnknown *)surface, ref_count + 1);

            hr = ID3DXRenderToSurface_BeginScene(render, surface, NULL);
            ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToSurface::BeginScene returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

            hr = ID3DXRenderToSurface_EndScene(render, D3DX_FILTER_NONE);
            ok(hr == D3D_OK, "ID3DXRenderToSurface::EndScene returned %#lx, expected %#lx\n", hr, D3D_OK);

            check_ref((IUnknown *)surface, ref_count);
        }

        /* error handling is deferred to BeginScene */
        hr = D3DXCreateRenderToSurface(device, 256, 256, D3DFMT_A8R8G8B8, TRUE, D3DFMT_UNKNOWN, &render_surface);
        ok(hr == D3D_OK, "D3DXCreateRenderToSurface returned %#lx, expected %#lx\n", hr, D3D_OK);
        hr = ID3DXRenderToSurface_BeginScene(render_surface, surface, NULL);
        ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToSurface::BeginScene returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);
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

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        const D3DXRTE_DESC *parameters = &tests[i].parameters;
        const D3DXRTE_DESC *expected  = &tests[i].expected_values;

        winetest_push_context("Test %u", i);
        hr = D3DXCreateRenderToEnvMap(device, parameters->Size, parameters->MipLevels, parameters->Format,
                parameters->DepthStencil, parameters->DepthStencilFormat, &render);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
        hr = ID3DXRenderToEnvMap_GetDesc(render, &desc);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
        ok(desc.Size == expected->Size, "Got size %u, expected %u.\n", desc.Size, expected->Size);
        ok(desc.MipLevels == expected->MipLevels, "Got miplevels %u, expected %u.\n", desc.MipLevels, expected->MipLevels);
        ok(desc.Format == expected->Format, "Got format %#x, expected %#x.\n", desc.Format, expected->Format);
        ok(desc.DepthStencil == expected->DepthStencil, "Got depth stencil %d, expected %d.\n",
                expected->DepthStencil, expected->DepthStencil);
        ok(desc.DepthStencilFormat == expected->DepthStencilFormat, "Got depth stencil format %#x, expected %#x\n",
                expected->DepthStencilFormat, expected->DepthStencilFormat);
        check_release((IUnknown *)render, 0);
        winetest_pop_context();
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
    ok(hr == D3D_OK, "D3DCreateRenderToEnvMap returned %#lx, expected %#lx\n", hr, D3D_OK);
    if (SUCCEEDED(hr))
    {
        DWORD face;

        hr = ID3DXRenderToEnvMap_End(render, D3DX_FILTER_NONE);
        ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToEnvMap::End returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

        hr = ID3DXRenderToEnvMap_BeginCube(render, cube_texture);
        ok(hr == D3D_OK, "ID3DXRenderToEnvMap::BeginCube returned %#lx, expected %#lx\n", hr, D3D_OK);

        hr = retrieve_device_state(device, &current_state);
        ok(SUCCEEDED(hr), "Failed to retrieve device state\n");
        compare_device_state(&current_state, &pre_state, TRUE);
        release_device_state(&current_state);

        for (face = D3DCUBEMAP_FACE_POSITIVE_X; face <= D3DCUBEMAP_FACE_NEGATIVE_Z; face++)
        {
            hr = ID3DXRenderToEnvMap_Face(render, face, D3DX_FILTER_POINT);
            ok(hr == D3D_OK, "ID3DXRenderToEnvMap::Face returned %#lx, expected %#lx\n", hr, D3D_OK);

            hr = retrieve_device_state(device, &current_state);
            ok(SUCCEEDED(hr), "Failed to retrieve device state\n");
            compare_device_state(&current_state, &pre_state, FALSE);
            release_device_state(&current_state);
        }

        hr = ID3DXRenderToEnvMap_End(render, D3DX_FILTER_POINT);
        ok(hr == D3D_OK, "ID3DXRenderToEnvMap::End returned %#lx, expected %#lx\n", hr, D3D_OK);

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
        ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToEnvMap::GetDesc returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

        hr = ID3DXRenderToEnvMap_GetDevice(render, NULL);
        ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToEnvMap::GetDevice returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

        ref_count = get_ref((IUnknown *)device);
        hr = ID3DXRenderToEnvMap_GetDevice(render, &out_device);
        ok(hr == D3D_OK, "ID3DXRenderToEnvMap::GetDevice returned %#lx, expected %#lx\n", hr, D3D_OK);
        ok(out_device == device, "ID3DXRenderToEnvMap::GetDevice returned different device\n");
        check_release((IUnknown *)device, ref_count);

        hr = ID3DXRenderToEnvMap_End(render, D3DX_FILTER_NONE);
        ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToEnvMap::End returned %#lx, expected %#lx\n", hr, D3D_OK);

        hr = ID3DXRenderToEnvMap_BeginCube(render, NULL);
        ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToEnvMap::BeginCube returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

        hr = ID3DXRenderToEnvMap_BeginHemisphere(render, NULL, NULL);
        todo_wine ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToEnvMap::BeginHemisphere returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

        hr = ID3DXRenderToEnvMap_BeginParabolic(render, NULL, NULL);
        todo_wine ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToEnvMap::BeginParabolic returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

        hr = ID3DXRenderToEnvMap_BeginSphere(render, NULL);
        todo_wine ok(hr == D3DERR_INVALIDCALL, "ID3DXRenderToEnvMap::BeginSphere returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

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
        skip("Failed to create IDirect3DDevice9 object %#lx\n", hr);
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
