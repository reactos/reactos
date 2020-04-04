/*
 * Tests for the D3DX9 volume functions
 *
 * Copyright 2012 Józef Kucia
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
#include "d3dx9tex.h"

/* 4x4x2 volume map dds, 2 mipmaps */
static const unsigned char dds_volume_map[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x8a,0x00,0x04,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x03,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x44,0x58,0x54,0x33,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x40,0x00,
    0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50,
    0xff,0x00,0xff,0x00,0x00,0x00,0x00,0x00,0x2f,0x7e,0xcf,0x79,0x01,0x54,0x5c,0x5c,
    0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x84,0xef,0x7b,0xaa,0xab,0xab,0xab
};

#define check_pixel_4bpp(box, x, y, z, color) _check_pixel_4bpp(__LINE__, box, x, y, z, color)
static inline void _check_pixel_4bpp(unsigned int line, const D3DLOCKED_BOX *box, int x, int y, int z, DWORD expected_color)
{
   DWORD color = ((DWORD *)box->pBits)[x + (y * box->RowPitch + z * box->SlicePitch) / 4];
   ok_(__FILE__, line)(color == expected_color, "Got color 0x%08x, expected 0x%08x\n", color, expected_color);
}

static inline void set_box(D3DBOX *box, UINT left, UINT top, UINT right, UINT bottom, UINT front, UINT back)
{
    box->Left = left;
    box->Top = top;
    box->Right = right;
    box->Bottom = bottom;
    box->Front = front;
    box->Back = back;
}

static void test_D3DXLoadVolumeFromMemory(IDirect3DDevice9 *device)
{
    int i, x, y, z;
    HRESULT hr;
    D3DBOX src_box, dst_box;
    D3DLOCKED_BOX locked_box;
    IDirect3DVolume9 *volume;
    IDirect3DVolumeTexture9 *volume_texture;
    const DWORD pixels[] = { 0xc3394cf0, 0x235ae892, 0x09b197fd, 0x8dc32bf6,
                             0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
                             0x00000000, 0x00000000, 0x00000000, 0xffffffff,
                             0xffffffff, 0x00000000, 0xffffffff, 0x00000000 };

    hr = IDirect3DDevice9_CreateVolumeTexture(device, 256, 256, 4, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT,
            &volume_texture, NULL);
    if (FAILED(hr))
    {
        skip("Failed to create volume texture.\n");
        return;
    }

    IDirect3DVolumeTexture9_GetVolumeLevel(volume_texture, 0, &volume);

    set_box(&src_box, 0, 0, 4, 1, 0, 4);
    set_box(&dst_box, 0, 0, 4, 1, 0, 4);

    hr = D3DXLoadVolumeFromMemory(volume, NULL, &dst_box, pixels, D3DFMT_A8R8G8B8, 16, 16, NULL, &src_box, D3DX_DEFAULT, 0);
    if (FAILED(hr))
    {
        win_skip("D3DXLoadVolumeFromMemory failed with error %#x, skipping some tests.\n", hr);
        return;
    }

    IDirect3DVolume9_LockBox(volume, &locked_box, &dst_box, D3DLOCK_READONLY);
    for (i = 0; i < 16; i++) check_pixel_4bpp(&locked_box, i % 4, 0, i / 4, pixels[i]);
    IDirect3DVolume9_UnlockBox(volume);

    hr = D3DXLoadVolumeFromMemory(volume, NULL, &dst_box, pixels, D3DFMT_UNKNOWN, 16, 16, NULL, &src_box, D3DX_DEFAULT, 0);
    ok(hr == E_FAIL, "D3DXLoadVolumeFromMemory returned %#x, expected %#x\n", hr, E_FAIL);

    hr = D3DXLoadVolumeFromMemory(volume, NULL, NULL, pixels, D3DFMT_A8R8G8B8, 16, 16, NULL, &src_box, D3DX_DEFAULT, 0);
    ok(hr == D3D_OK, "D3DXLoadVolumeFromMemory returned %#x, expected %#x\n", hr, D3D_OK);

    hr = D3DXLoadVolumeFromMemory(volume, NULL, &dst_box, NULL, D3DFMT_A8R8G8B8, 16, 16, NULL, &src_box, D3DX_DEFAULT, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadVolumeFromMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXLoadVolumeFromMemory(volume, NULL, &dst_box, pixels, D3DFMT_A8R8G8B8, 16, 16, NULL, NULL, D3DX_DEFAULT, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadVolumeFromMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    set_box(&src_box, 0, 0, 4, 4, 0, 1);
    set_box(&dst_box, 0, 0, 4, 4, 0, 1);
    hr = D3DXLoadVolumeFromMemory(volume, NULL, &dst_box, pixels, D3DFMT_A8R8G8B8, 16, sizeof(pixels), NULL, &src_box, D3DX_DEFAULT, 0);
    ok(hr == D3D_OK, "D3DXLoadVolumeFromMemory returned %#x, expected %#x\n", hr, D3D_OK);

    IDirect3DVolume9_LockBox(volume, &locked_box, &dst_box, D3DLOCK_READONLY);
    for (i = 0; i < 16; i++) check_pixel_4bpp(&locked_box, i % 4, i / 4, 0, pixels[i]);
    IDirect3DVolume9_UnlockBox(volume);

    hr = D3DXLoadVolumeFromMemory(volume, NULL, NULL, pixels, D3DFMT_A8R8G8B8, 16, sizeof(pixels), NULL, &src_box, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "D3DXLoadVolumeFromMemory returned %#x, expected %#x\n", hr, D3D_OK);

    IDirect3DVolume9_LockBox(volume, &locked_box, NULL, D3DLOCK_READONLY);
    for (i = 0; i < 16; i++) check_pixel_4bpp(&locked_box, i % 4, i / 4, 0, pixels[i]);
    for (z = 0; z < 4; z++)
    {
        for (y = 0; y < 256; y++)
        {
            for (x = 0; x < 256; x++)
                if (z != 0 || y >= 4 || x >= 4) check_pixel_4bpp(&locked_box, x, y, z, 0);
        }
    }
    IDirect3DVolume9_UnlockBox(volume);

    set_box(&src_box, 0, 0, 2, 2, 1, 2);
    set_box(&dst_box, 0, 0, 2, 2, 0, 1);
    hr = D3DXLoadVolumeFromMemory(volume, NULL, &dst_box, pixels, D3DFMT_A8R8G8B8, 8, 16, NULL, &src_box, D3DX_DEFAULT, 0);
    ok(hr == D3D_OK, "D3DXLoadVolumeFromMemory returned %#x, expected %#x\n", hr, D3D_OK);

    IDirect3DVolume9_LockBox(volume, &locked_box, &dst_box, D3DLOCK_READONLY);
    for (i = 0; i < 4; i++) check_pixel_4bpp(&locked_box, i % 2, i / 2, 0, pixels[i + 4]);
    IDirect3DVolume9_UnlockBox(volume);

    set_box(&src_box, 0, 0, 2, 2, 2, 3);
    set_box(&dst_box, 0, 0, 2, 2, 1, 2);
    hr = D3DXLoadVolumeFromMemory(volume, NULL, &dst_box, pixels, D3DFMT_A8R8G8B8, 8, 16, NULL, &src_box, D3DX_DEFAULT, 0);
    ok(hr == D3D_OK, "D3DXLoadVolumeFromMemory returned %#x, expected %#x\n", hr, D3D_OK);

    IDirect3DVolume9_LockBox(volume, &locked_box, &dst_box, D3DLOCK_READONLY);
    for (i = 0; i < 4; i++) check_pixel_4bpp(&locked_box, i % 2, i / 2, 0, pixels[i + 8]);
    IDirect3DVolume9_UnlockBox(volume);

    set_box(&src_box, 0, 0, 4, 1, 0, 4);

    set_box(&dst_box, -1, -1, 3, 0, 0, 4);
    hr = D3DXLoadVolumeFromMemory(volume, NULL, &dst_box, pixels, D3DFMT_A8R8G8B8, 16, 16, NULL, &src_box, D3DX_DEFAULT, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadVolumeFromMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    set_box(&dst_box, 254, 254, 258, 255, 0, 4);
    hr = D3DXLoadVolumeFromMemory(volume, NULL, &dst_box, pixels, D3DFMT_A8R8G8B8, 16, 16, NULL, &src_box, D3DX_DEFAULT, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadVolumeFromMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    set_box(&dst_box, 4, 1, 0, 0, 0, 4);
    hr = D3DXLoadVolumeFromMemory(volume, NULL, &dst_box, pixels, D3DFMT_A8R8G8B8, 16, 16, NULL, &src_box, D3DX_DEFAULT, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadVolumeFromMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    set_box(&dst_box, 0, 0, 0, 0, 0, 0);
    hr = D3DXLoadVolumeFromMemory(volume, NULL, &dst_box, pixels, D3DFMT_A8R8G8B8, 16, 16, NULL, &src_box, D3DX_DEFAULT, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadVolumeFromMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    set_box(&dst_box, 0, 0, 0, 0, 0, 1);
    hr = D3DXLoadVolumeFromMemory(volume, NULL, &dst_box, pixels, D3DFMT_A8R8G8B8, 16, 16, NULL, &src_box, D3DX_DEFAULT, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadVolumeFromMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    set_box(&dst_box, 300, 300, 300, 300, 0, 0);
    hr = D3DXLoadVolumeFromMemory(volume, NULL, &dst_box, pixels, D3DFMT_A8R8G8B8, 16, 16, NULL, &src_box, D3DX_DEFAULT, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadVolumeFromMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    IDirect3DVolume9_Release(volume);
    IDirect3DVolumeTexture9_Release(volume_texture);

    hr = IDirect3DDevice9_CreateVolumeTexture(device, 256, 256, 4, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &volume_texture, NULL);
    if (FAILED(hr))
    {
        skip("Failed to create volume texture\n");
        return;
    }

    IDirect3DVolumeTexture9_GetVolumeLevel(volume_texture, 0, &volume);

    set_box(&src_box, 0, 0, 4, 1, 0, 4);
    set_box(&dst_box, 0, 0, 4, 1, 0, 4);
    hr = D3DXLoadVolumeFromMemory(volume, NULL, &dst_box, pixels, D3DFMT_A8R8G8B8, 16, 16, NULL, &src_box, D3DX_DEFAULT, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadVolumeFromMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    IDirect3DVolume9_Release(volume);
    IDirect3DVolumeTexture9_Release(volume_texture);

    hr = IDirect3DDevice9_CreateVolumeTexture(device, 8, 8, 1, 1, D3DUSAGE_DYNAMIC, D3DFMT_DXT1, D3DPOOL_DEFAULT, &volume_texture, NULL);
    if (FAILED(hr))
    {
        skip("Failed to create volume texture\n");
        return;
    }

    IDirect3DVolumeTexture9_GetVolumeLevel(volume_texture, 0, &volume);

    set_box(&src_box, 1, 1, 7, 7, 0, 1);
    set_box(&dst_box, 1, 1, 7, 7, 0, 1);
    hr = D3DXLoadVolumeFromMemory(volume, NULL, &dst_box, pixels, D3DFMT_A8R8G8B8, 16, 32, NULL, &src_box, D3DX_DEFAULT, 0);
    todo_wine ok(hr == D3D_OK, "D3DXLoadVolumeFromMemory returned %#x, expected %#x\n", hr, D3D_OK);

    IDirect3DVolume9_Release(volume);
    IDirect3DVolumeTexture9_Release(volume_texture);
}

static void test_D3DXLoadVolumeFromFileInMemory(IDirect3DDevice9 *device)
{
    HRESULT hr;
    D3DBOX src_box;
    IDirect3DVolume9 *volume;
    IDirect3DVolumeTexture9 *volume_texture;

    hr = IDirect3DDevice9_CreateVolumeTexture(device, 4, 4, 2, 1, D3DUSAGE_DYNAMIC, D3DFMT_DXT3, D3DPOOL_DEFAULT,
            &volume_texture, NULL);
    if (FAILED(hr))
    {
        skip("Failed to create volume texture\n");
        return;
    }

    IDirect3DVolumeTexture9_GetVolumeLevel(volume_texture, 0, &volume);

    hr = D3DXLoadVolumeFromFileInMemory(volume, NULL, NULL, dds_volume_map, sizeof(dds_volume_map), NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3D_OK, "D3DXLoadVolumeFromFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);

    hr = D3DXLoadVolumeFromFileInMemory(volume, NULL, NULL, NULL, sizeof(dds_volume_map), NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadVolumeFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXLoadVolumeFromFileInMemory(volume, NULL, NULL, NULL, 0, NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadVolumeFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXLoadVolumeFromFileInMemory(volume, NULL, NULL, dds_volume_map, 0, NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadVolumeFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXLoadVolumeFromFileInMemory(NULL, NULL, NULL, dds_volume_map, sizeof(dds_volume_map), NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadVolumeFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    set_box(&src_box, 0, 0, 4, 4, 0, 2);
    hr = D3DXLoadVolumeFromFileInMemory(volume, NULL, NULL, dds_volume_map, sizeof(dds_volume_map), &src_box, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3D_OK, "D3DXLoadVolumeFromFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);

    set_box(&src_box, 0, 0, 0, 0, 0, 0);
    hr = D3DXLoadVolumeFromFileInMemory(volume, NULL, NULL, dds_volume_map, sizeof(dds_volume_map), &src_box, D3DX_DEFAULT, 0, NULL);
    ok(hr == E_FAIL, "D3DXLoadVolumeFromFileInMemory returned %#x, expected %#x\n", hr, E_FAIL);

    set_box(&src_box, 0, 0, 5, 4, 0, 2);
    hr = D3DXLoadVolumeFromFileInMemory(volume, NULL, NULL, dds_volume_map, sizeof(dds_volume_map), &src_box, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadVolumeFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    set_box(&src_box, 0, 0, 4, 4, 0, 3);
    hr = D3DXLoadVolumeFromFileInMemory(volume, NULL, NULL, dds_volume_map, sizeof(dds_volume_map), &src_box, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadVolumeFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    IDirect3DVolume9_Release(volume);
    IDirect3DVolumeTexture9_Release(volume_texture);
}

START_TEST(volume)
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
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d)
    {
        skip("Couldn't create IDirect3D9 object\n");
        DestroyWindow(wnd);
        return;
    }

    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed   = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &device);
    if(FAILED(hr))
    {
        skip("Failed to create IDirect3DDevice9 object %#x\n", hr);
        IDirect3D9_Release(d3d);
        DestroyWindow(wnd);
        return;
    }

    test_D3DXLoadVolumeFromMemory(device);
    test_D3DXLoadVolumeFromFileInMemory(device);

    IDirect3DDevice9_Release(device);
    IDirect3D9_Release(d3d);
    DestroyWindow(wnd);
}
