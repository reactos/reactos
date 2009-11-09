/*
 * Copyright (C) 2008 Tony Wasserka
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

#include <d3dx9.h>

#ifndef __WINE_D3DX9TEX_H
#define __WINE_D3DX9TEX_H

/**********************************************
 ***************** Definitions ****************
 **********************************************/
#define D3DX_FILTER_NONE                 0x00000001
#define D3DX_FILTER_POINT                0x00000002
#define D3DX_FILTER_LINEAR               0x00000003
#define D3DX_FILTER_TRIANGLE             0x00000004
#define D3DX_FILTER_BOX                  0x00000005
#define D3DX_FILTER_MIRROR_U             0x00010000
#define D3DX_FILTER_MIRROR_V             0x00020000
#define D3DX_FILTER_MIRROR_W             0x00040000
#define D3DX_FILTER_MIRROR               0x00070000
#define D3DX_FILTER_DITHER               0x00080000
#define D3DX_FILTER_DITHER_DIFFUSION     0x00100000
#define D3DX_FILTER_SRGB_IN              0x00200000
#define D3DX_FILTER_SRGB_OUT             0x00400000
#define D3DX_FILTER_SRGB                 0x00600000

#define D3DX_NORMALMAP_MIRROR_U          0x00010000
#define D3DX_NORMALMAP_MIRROR_V          0x00020000
#define D3DX_NORMALMAP_MIRROR            0x00030000
#define D3DX_NORMALMAP_INVERTSIGN        0x00080000
#define D3DX_NORMALMAP_COMPUTE_OCCLUSION 0x00100000

#define D3DX_CHANNEL_RED                 0x00000001
#define D3DX_CHANNEL_BLUE                0x00000002
#define D3DX_CHANNEL_GREEN               0x00000004
#define D3DX_CHANNEL_ALPHA               0x00000008
#define D3DX_CHANNEL_LUMINANCE           0x00000010

/**********************************************
 **************** Typedefs ****************
 **********************************************/
typedef enum _D3DXIMAGE_FILEFORMAT
{
    D3DXIFF_BMP,
    D3DXIFF_JPG,
    D3DXIFF_TGA,
    D3DXIFF_PNG,
    D3DXIFF_DDS,
    D3DXIFF_PPM,
    D3DXIFF_DIB,
    D3DXIFF_FORCE_DWORD = 0x7fffffff
} D3DXIMAGE_FILEFORMAT;

typedef struct _D3DXIMAGE_INFO
{
    UINT Width;
    UINT Height;
    UINT Depth;
    UINT MipLevels;
    D3DFORMAT Format;
    D3DRESOURCETYPE ResourceType;
    D3DXIMAGE_FILEFORMAT ImageFileFormat;
} D3DXIMAGE_INFO;

/**********************************************
 ****************** Functions *****************
 **********************************************/
/* Typedefs for callback functions */
typedef VOID (WINAPI *LPD3DXFILL2D)(D3DXVECTOR4 *out, CONST D3DXVECTOR2 *texcoord, CONST D3DXVECTOR2 *texelsize, LPVOID data);
typedef VOID (WINAPI *LPD3DXFILL3D)(D3DXVECTOR4 *out, CONST D3DXVECTOR3 *texcoord, CONST D3DXVECTOR3 *texelsize, LPVOID data);

#ifdef __cplusplus
extern "C" {
#endif


/* Image Information */
HRESULT WINAPI D3DXGetImageInfoFromFileA(LPCSTR file, D3DXIMAGE_INFO *info);
HRESULT WINAPI D3DXGetImageInfoFromFileW(LPCWSTR file, D3DXIMAGE_INFO *info);
#define        D3DXGetImageInfoFromFile WINELIB_NAME_AW(D3DXGetImageInfoFromFile)

HRESULT WINAPI D3DXGetImageInfoFromResourceA(HMODULE module, LPCSTR resource, D3DXIMAGE_INFO *info);
HRESULT WINAPI D3DXGetImageInfoFromResourceW(HMODULE module, LPCWSTR resource, D3DXIMAGE_INFO *info);
#define        D3DXGetImageInfoFromResource WINELIB_NAME_AW(D3DXGetImageInfoFromResource)

HRESULT WINAPI D3DXGetImageInfoFromFileInMemory(LPCVOID data, UINT datasize, D3DXIMAGE_INFO *info);


/* Surface Loading/Saving */
HRESULT WINAPI D3DXLoadSurfaceFromFileA(       LPDIRECT3DSURFACE9 destsurface,
                                               CONST PALETTEENTRY *destpalette,
                                               CONST RECT *destrect,
                                               LPCSTR srcfile,
                                               CONST RECT *srcrect,
                                               DWORD filter,
                                               D3DCOLOR colorkey,
                                               D3DXIMAGE_INFO *srcinfo);

HRESULT WINAPI D3DXLoadSurfaceFromFileW(       LPDIRECT3DSURFACE9 destsurface,
                                               CONST PALETTEENTRY *destpalette,
                                               CONST RECT *destrect,
                                               LPCWSTR srcfile,
                                               CONST RECT *srcrect,
                                               DWORD filter,
                                               D3DCOLOR colorkey,
                                               D3DXIMAGE_INFO *srcinfo);
#define        D3DXLoadSurfaceFromFile WINELIB_NAME_AW(D3DXLoadSurfaceFromFile)

HRESULT WINAPI D3DXLoadSurfaceFromResourceA(   LPDIRECT3DSURFACE9 destsurface,
                                               CONST PALETTEENTRY *destpalette,
                                               CONST RECT *destrect,
                                               HMODULE srcmodule,
                                               LPCSTR resource,
                                               CONST RECT *srcrect,
                                               DWORD filter,
                                               D3DCOLOR colorkey,
                                               D3DXIMAGE_INFO *srcinfo);

HRESULT WINAPI D3DXLoadSurfaceFromResourceW(   LPDIRECT3DSURFACE9 destsurface,
                                               CONST PALETTEENTRY *destpalette,
                                               CONST RECT *destrect,
                                               HMODULE srcmodule,
                                               LPCWSTR resource,
                                               CONST RECT *srcrect,
                                               DWORD filter,
                                               D3DCOLOR colorkey,
                                               D3DXIMAGE_INFO *srcinfo);
#define        D3DXLoadSurfaceFromResource WINELIB_NAME_AW(D3DXLoadSurfaceFromResource)

HRESULT WINAPI D3DXLoadSurfaceFromFileInMemory(LPDIRECT3DSURFACE9 destsurface,
                                               CONST PALETTEENTRY *destpalette,
                                               CONST RECT*destrect,
                                               LPCVOID srcdata,
                                               UINT srcdatasize,
                                               CONST RECT *srcrect,
                                               DWORD filter,
                                               D3DCOLOR colorkey,
                                               D3DXIMAGE_INFO *srcinfo);

HRESULT WINAPI D3DXLoadSurfaceFromSurface(     LPDIRECT3DSURFACE9 destsurface,
                                               CONST PALETTEENTRY *destpalette,
                                               CONST RECT *destrect,
                                               LPDIRECT3DSURFACE9 srcsurface,
                                               CONST PALETTEENTRY *srcpalette,
                                               CONST RECT *srcrect,
                                               DWORD filter,
                                               D3DCOLOR colorkey);

HRESULT WINAPI D3DXLoadSurfaceFromMemory(      LPDIRECT3DSURFACE9 destsurface,
                                               CONST PALETTEENTRY *destpalette,
                                               CONST RECT *destrect,
                                               LPCVOID srcmemory,
                                               D3DFORMAT srcformat,
                                               UINT srcpitch,
                                               CONST PALETTEENTRY *srcpalette,
                                               CONST RECT *srcrect,
                                               DWORD filter,
                                               D3DCOLOR colorkey);

HRESULT WINAPI D3DXSaveSurfaceToFileA(         LPCSTR destfile,
                                               D3DXIMAGE_FILEFORMAT destformat,
                                               LPDIRECT3DSURFACE9 srcsurface,
                                               CONST PALETTEENTRY *srcpalette,
                                               CONST RECT *srcrect);

HRESULT WINAPI D3DXSaveSurfaceToFileW(         LPCWSTR destfile,
                                               D3DXIMAGE_FILEFORMAT destformat,
                                               LPDIRECT3DSURFACE9 srcsurface,
                                               CONST PALETTEENTRY *srcpalette,
                                               CONST RECT *srcrect);
#define        D3DXSaveSurfaceToFile WINELIB_NAME_AW(D3DXSaveSurfaceToFile)


/* Volume Loading/Saving */
HRESULT WINAPI D3DXLoadVolumeFromFileA(       LPDIRECT3DVOLUME9 destvolume,
                                              CONST PALETTEENTRY *destpalette,
                                              CONST D3DBOX *destbox,
                                              LPCSTR srcfile,
                                              CONST D3DBOX *srcbox,
                                              DWORD filter,
                                              D3DCOLOR colorkey,
                                              D3DXIMAGE_INFO *srcinfo);

HRESULT WINAPI D3DXLoadVolumeFromFileW(       LPDIRECT3DVOLUME9 destVolume,
                                              CONST PALETTEENTRY *destpalette,
                                              CONST D3DBOX *destbox,
                                              LPCWSTR srcfile,
                                              CONST D3DBOX *srcbox,
                                              DWORD filter,
                                              D3DCOLOR colorkey,
                                              D3DXIMAGE_INFO *srcinfo);
#define        D3DXLoadVolumeFromFile WINELIB_NAME_AW(D3DXLoadVolumeFromFile)

HRESULT WINAPI D3DXLoadVolumeFromResourceA(   LPDIRECT3DVOLUME9 destVolume,
                                              CONST PALETTEENTRY *destpalette,
                                              CONST D3DBOX *destbox,
                                              HMODULE srcmodule,
                                              LPCSTR resource,
                                              CONST D3DBOX *srcbox,
                                              DWORD filter,
                                              D3DCOLOR colorkey,
                                              D3DXIMAGE_INFO *srcinfo);

HRESULT WINAPI D3DXLoadVolumeFromResourceW(   LPDIRECT3DVOLUME9 destVolume,
                                              CONST PALETTEENTRY *destpalette,
                                              CONST D3DBOX *destbox,
                                              HMODULE srcmodule,
                                              LPCWSTR resource,
                                              CONST D3DBOX *srcbox,
                                              DWORD filter,
                                              D3DCOLOR colorkey,
                                              D3DXIMAGE_INFO *srcinfo);
#define        D3DXLoadVolumeFromResource WINELIB_NAME_AW(D3DXLoadVolumeFromResource)

HRESULT WINAPI D3DXLoadVolumeFromFileInMemory(LPDIRECT3DVOLUME9 destvolume,
                                              CONST PALETTEENTRY *destpalette,
                                              CONST D3DBOX *destbox,
                                              LPCVOID srcdata,
                                              UINT srcdatasize,
                                              CONST D3DBOX *srcbox,
                                              DWORD filter,
                                              D3DCOLOR colorkey,
                                              D3DXIMAGE_INFO *srcinfo);

HRESULT WINAPI D3DXLoadVolumeFromVolume(      LPDIRECT3DVOLUME9 destvolume,
                                              CONST PALETTEENTRY *destpalette,
                                              CONST D3DBOX *destbox,
                                              LPDIRECT3DVOLUME9 srcvolume,
                                              CONST PALETTEENTRY *srcpalette,
                                              CONST D3DBOX *srcbox,
                                              DWORD filter,
                                              D3DCOLOR colorkey);

HRESULT WINAPI D3DXLoadVolumeFromMemory(      LPDIRECT3DVOLUME9 destvolume,
                                              CONST PALETTEENTRY *destpalette,
                                              CONST D3DBOX *destbox,
                                              LPCVOID srcmemory,
                                              D3DFORMAT srcformat,
                                              UINT srcrowpitch,
                                              UINT srcslicepitch,
                                              CONST PALETTEENTRY *srcpalette,
                                              CONST D3DBOX *srcbox,
                                              DWORD filter,
                                              D3DCOLOR colorkey);

HRESULT WINAPI D3DXSaveVolumeToFileA(         LPCSTR destfile,
                                              D3DXIMAGE_FILEFORMAT destformat,
                                              LPDIRECT3DVOLUME9 srcvolume,
                                              CONST PALETTEENTRY *srcpalette,
                                              CONST D3DBOX *srcbox);

HRESULT WINAPI D3DXSaveVolumeToFileW(         LPCWSTR destfile,
                                              D3DXIMAGE_FILEFORMAT destformat,
                                              LPDIRECT3DVOLUME9 srcvolume,
                                              CONST PALETTEENTRY *srcpalette,
                                              CONST D3DBOX *srcbox);
#define        D3DXSaveVolumeToFile WINELIB_NAME_AW(D3DXSaveVolumeToFile)


/* Texture, cube texture and volume texture creation */
HRESULT WINAPI D3DXCheckTextureRequirements(      LPDIRECT3DDEVICE9 device,
                                                  UINT *width,
                                                  UINT *height,
                                                  UINT *miplevels,
                                                  DWORD usage,
                                                  D3DFORMAT *format,
                                                  D3DPOOL pool);
HRESULT WINAPI D3DXCheckCubeTextureRequirements(  LPDIRECT3DDEVICE9 device,
                                                  UINT *size,
                                                  UINT *miplevels,
                                                  DWORD usage,
                                                  D3DFORMAT *format,
                                                  D3DPOOL pool);

HRESULT WINAPI D3DXCheckVolumeTextureRequirements(LPDIRECT3DDEVICE9 device,
                                                  UINT *width,
                                                  UINT *height,
                                                  UINT *depth,
                                                  UINT *miplevels,
                                                  DWORD usage,
                                                  D3DFORMAT *format,
                                                  D3DPOOL pool);

HRESULT WINAPI D3DXCreateTexture(      LPDIRECT3DDEVICE9 device,
                                       UINT width,
                                       UINT height,
                                       UINT miplevels,
                                       DWORD usage,
                                       D3DFORMAT format,
                                       D3DPOOL pool,
                                       LPDIRECT3DTEXTURE9 *texture);

HRESULT WINAPI D3DXCreateCubeTexture(  LPDIRECT3DDEVICE9 device,
                                       UINT size,
                                       UINT miplevels,
                                       DWORD usage,
                                       D3DFORMAT format,
                                       D3DPOOL pool,
                                       LPDIRECT3DCUBETEXTURE9 *cube);

HRESULT WINAPI D3DXCreateVolumeTexture(LPDIRECT3DDEVICE9 device,
                                       UINT width,
                                       UINT height,
                                       UINT depth,
                                       UINT miplevels,
                                       DWORD usage,
                                       D3DFORMAT format,
                                       D3DPOOL pool,
                                       LPDIRECT3DVOLUMETEXTURE9 *volume);

HRESULT WINAPI D3DXCreateTextureFromFileA(      LPDIRECT3DDEVICE9 device,
                                                LPCSTR srcfile,
                                                LPDIRECT3DTEXTURE9 *texture);

HRESULT WINAPI D3DXCreateTextureFromFileW(      LPDIRECT3DDEVICE9 device,
                                                LPCWSTR srcfile,
                                                LPDIRECT3DTEXTURE9 *texture);
#define        D3DXCreateTextureFromFile WINELIB_NAME_AW(D3DXCreateTextureFromFile)

HRESULT WINAPI D3DXCreateCubeTextureFromFileA(  LPDIRECT3DDEVICE9 device,
                                                LPCSTR srcfile,
                                                LPDIRECT3DCUBETEXTURE9 *cube);

HRESULT WINAPI D3DXCreateCubeTextureFromFileW(  LPDIRECT3DDEVICE9 device,
                                                LPCWSTR srcfile,
                                                LPDIRECT3DCUBETEXTURE9 *cube);
#define        D3DXCreateCubeTextureFromFile WINELIB_NAME_AW(D3DXCreateCubeTextureFromFile)

HRESULT WINAPI D3DXCreateVolumeTextureFromFileA(LPDIRECT3DDEVICE9 device,
                                                LPCSTR srcfile,
                                                LPDIRECT3DVOLUMETEXTURE9 *volume);

HRESULT WINAPI D3DXCreateVolumeTextureFromFileW(LPDIRECT3DDEVICE9 device,
                                                LPCWSTR srcfile,
                                                LPDIRECT3DVOLUMETEXTURE9 *volume);
#define        D3DXCreateVolumeTextureFromFile WINELIB_NAME_AW(D3DXCreateVolumeTextureFromFile)

HRESULT WINAPI D3DXCreateTextureFromResourceA(      LPDIRECT3DDEVICE9 device,
                                                    HMODULE srcmodule,
                                                    LPCSTR resource,
                                                    LPDIRECT3DTEXTURE9 *texture);

HRESULT WINAPI D3DXCreateTextureFromResourceW(      LPDIRECT3DDEVICE9 device,
                                                    HMODULE srcmodule,
                                                    LPCWSTR resource,
                                                    LPDIRECT3DTEXTURE9 *texture);
#define        D3DXCreateTextureFromResource WINELIB_NAME_AW(D3DXCreateTextureFromResource)

HRESULT WINAPI D3DXCreateCubeTextureFromResourceA(  LPDIRECT3DDEVICE9 device,
                                                    HMODULE srcmodule,
                                                    LPCSTR resource,
                                                    LPDIRECT3DCUBETEXTURE9 *cube);
HRESULT WINAPI D3DXCreateCubeTextureFromResourceW(  LPDIRECT3DDEVICE9 device,
                                                    HMODULE srcmodule,
                                                    LPCWSTR resource,
                                                    LPDIRECT3DCUBETEXTURE9 *cube);
#define        D3DXCreateCubeTextureFromResource WINELIB_NAME_AW(D3DXCreateCubeTextureFromResource)

HRESULT WINAPI D3DXCreateVolumeTextureFromResourceA(LPDIRECT3DDEVICE9 device,
                                                    HMODULE srcmodule,
                                                    LPCSTR resource,
                                                    LPDIRECT3DVOLUMETEXTURE9 *volume);
HRESULT WINAPI D3DXCreateVolumeTextureFromResourceW(LPDIRECT3DDEVICE9 device,
                                                    HMODULE srcmodule,
                                                    LPCWSTR resource,
                                                    LPDIRECT3DVOLUMETEXTURE9 *volume);
#define        D3DXCreateVolumeTextureFromResource WINELIB_NAME_AW(D3DXCreateVolumeTextureFromResource)

HRESULT WINAPI D3DXCreateTextureFromFileExA(      LPDIRECT3DDEVICE9 device,
                                                  LPCSTR srcfile,
                                                  UINT width,
                                                  UINT height,
                                                  UINT miplevels,
                                                  DWORD usage,
                                                  D3DFORMAT format,
                                                  D3DPOOL pool,
                                                  DWORD filter,
                                                  DWORD mipfilter,
                                                  D3DCOLOR colorkey,
                                                  D3DXIMAGE_INFO *srcinfo,
                                                  PALETTEENTRY *palette,
                                                  LPDIRECT3DTEXTURE9 *texture);

HRESULT WINAPI D3DXCreateTextureFromFileExW(      LPDIRECT3DDEVICE9 device,
                                                  LPCWSTR srcfile,
                                                  UINT width,
                                                  UINT height,
                                                  UINT miplevels,
                                                  DWORD usage,
                                                  D3DFORMAT format,
                                                  D3DPOOL pool,
                                                  DWORD filter,
                                                  DWORD mipfilter,
                                                  D3DCOLOR colorkey,
                                                  D3DXIMAGE_INFO *srcinfo,
                                                  PALETTEENTRY *palette,
                                                  LPDIRECT3DTEXTURE9 *texture);
#define        D3DXCreateTextureFromFileEx WINELIB_NAME_AW(D3DXCreateTextureFromFileEx)

HRESULT WINAPI D3DXCreateCubeTextureFromFileExA(  LPDIRECT3DDEVICE9 device,
                                                  LPCSTR srcfile,
                                                  UINT size,
                                                  UINT miplevels,
                                                  DWORD usage,
                                                  D3DFORMAT format,
                                                  D3DPOOL pool,
                                                  DWORD filter,
                                                  DWORD mipfilter,
                                                  D3DCOLOR colorkey,
                                                  D3DXIMAGE_INFO *srcinfo,
                                                  PALETTEENTRY *palette,
                                                  LPDIRECT3DCUBETEXTURE9 *cube);

HRESULT WINAPI D3DXCreateCubeTextureFromFileExW(  LPDIRECT3DDEVICE9 device,
                                                  LPCWSTR srcfile,
                                                  UINT size,
                                                  UINT miplevels,
                                                  DWORD usage,
                                                  D3DFORMAT format,
                                                  D3DPOOL pool,
                                                  DWORD filter,
                                                  DWORD mipfilter,
                                                  D3DCOLOR colorkey,
                                                  D3DXIMAGE_INFO *srcinfo,
                                                  PALETTEENTRY *palette,
                                                  LPDIRECT3DCUBETEXTURE9 *cube);
#define        D3DXCreateCubeTextureFromFileEx WINELIB_NAME_AW(D3DXCreateCubeTextureFromFileEx)

HRESULT WINAPI D3DXCreateVolumeTextureFromFileExA(LPDIRECT3DDEVICE9 device,
                                                  LPCSTR srcfile,
                                                  UINT width,
                                                  UINT height,
                                                  UINT depth,
                                                  UINT miplevels,
                                                  DWORD usage,
                                                  D3DFORMAT format,
                                                  D3DPOOL pool,
                                                  DWORD filter,
                                                  DWORD mipfilter,
                                                  D3DCOLOR colorkey,
                                                  D3DXIMAGE_INFO *srcinfo,
                                                  PALETTEENTRY *palette,
                                                  LPDIRECT3DVOLUMETEXTURE9 *volume);

HRESULT WINAPI D3DXCreateVolumeTextureFromFileExW(LPDIRECT3DDEVICE9 device,
                                                  LPCWSTR srcfile,
                                                  UINT width,
                                                  UINT height,
                                                  UINT depth,
                                                  UINT miplevels,
                                                  DWORD usage,
                                                  D3DFORMAT format,
                                                  D3DPOOL pool,
                                                  DWORD filter,
                                                  DWORD mipfilter,
                                                  D3DCOLOR colorkey,
                                                  D3DXIMAGE_INFO *srcinfo,
                                                  PALETTEENTRY *palette,
                                                  LPDIRECT3DVOLUMETEXTURE9 *volume);
#define        D3DXCreateVolumeTextureFromFileEx WINELIB_NAME_AW(D3DXCreateVolumeTextureFromFileEx)

HRESULT WINAPI D3DXCreateTextureFromResourceExA(      LPDIRECT3DDEVICE9 device,
                                                      HMODULE srcmodule,
                                                      LPCSTR resource,
                                                      UINT width,
                                                      UINT height,
                                                      UINT miplevels,
                                                      DWORD usage,
                                                      D3DFORMAT format,
                                                      D3DPOOL pool,
                                                      DWORD filter,
                                                      DWORD mipfilter,
                                                      D3DCOLOR colorkey,
                                                      D3DXIMAGE_INFO *srcinfo,
                                                      PALETTEENTRY *palette,
                                                      LPDIRECT3DTEXTURE9 *texture);

HRESULT WINAPI D3DXCreateTextureFromResourceExW(      LPDIRECT3DDEVICE9 device,
                                                      HMODULE srcmodule,
                                                      LPCWSTR resource,
                                                      UINT width,
                                                      UINT height,
                                                      UINT miplevels,
                                                      DWORD usage,
                                                      D3DFORMAT format,
                                                      D3DPOOL pool,
                                                      DWORD filter,
                                                      DWORD mipfilter,
                                                      D3DCOLOR colorkey,
                                                      D3DXIMAGE_INFO *srcinfo,
                                                      PALETTEENTRY *palette,
                                                      LPDIRECT3DTEXTURE9 *texture);
#define        D3DXCreateTextureFromResourceEx WINELIB_NAME_AW(D3DXCreateTextureFromResourceEx)

HRESULT WINAPI D3DXCreateCubeTextureFromResourceExA(  LPDIRECT3DDEVICE9 device,
                                                      HMODULE srcmodule,
                                                      LPCSTR resource,
                                                      UINT size,
                                                      UINT miplevels,
                                                      DWORD usage,
                                                      D3DFORMAT format,
                                                      D3DPOOL pool,
                                                      DWORD filter,
                                                      DWORD mipfilter,
                                                      D3DCOLOR colorkey,
                                                      D3DXIMAGE_INFO *srcinfo,
                                                      PALETTEENTRY *palette,
                                                      LPDIRECT3DCUBETEXTURE9 *cube);

HRESULT WINAPI D3DXCreateCubeTextureFromResourceExW(  LPDIRECT3DDEVICE9 device,
                                                      HMODULE srcmodule,
                                                      LPCWSTR resource,
                                                      UINT size,
                                                      UINT miplevels,
                                                      DWORD usage,
                                                      D3DFORMAT format,
                                                      D3DPOOL pool,
                                                      DWORD filter,
                                                      DWORD mipfilter,
                                                      D3DCOLOR colorkey,
                                                      D3DXIMAGE_INFO *srcinfo,
                                                      PALETTEENTRY *palette,
                                                      LPDIRECT3DCUBETEXTURE9 *cube);
#define        D3DXCreateCubeTextureFromResourceEx WINELIB_NAME_AW(D3DXCreateCubeTextureFromResourceEx)

HRESULT WINAPI D3DXCreateVolumeTextureFromResourceExA(LPDIRECT3DDEVICE9 device,
                                                      HMODULE srcmodule,
                                                      LPCSTR resource,
                                                      UINT width,
                                                      UINT height,
                                                      UINT depth,
                                                      UINT miplevels,
                                                      DWORD usage,
                                                      D3DFORMAT format,
                                                      D3DPOOL pool,
                                                      DWORD filter,
                                                      DWORD mipfilter,
                                                      D3DCOLOR colorkey,
                                                      D3DXIMAGE_INFO *srcinfo,
                                                      PALETTEENTRY *palette,
                                                      LPDIRECT3DVOLUMETEXTURE9 *volume);

HRESULT WINAPI D3DXCreateVolumeTextureFromResourceExW(LPDIRECT3DDEVICE9 device,
                                                      HMODULE srcmodule,
                                                      LPCWSTR resource,
                                                      UINT width,
                                                      UINT height,
                                                      UINT depth,
                                                      UINT miplevels,
                                                      DWORD usage,
                                                      D3DFORMAT format,
                                                      D3DPOOL pool,
                                                      DWORD filter,
                                                      DWORD mipfilter,
                                                      D3DCOLOR colorkey,
                                                      D3DXIMAGE_INFO *srcinfo,
                                                      PALETTEENTRY *palette,
                                                      LPDIRECT3DVOLUMETEXTURE9 *volume);
#define        D3DXCreateVolumeTextureFromResourceEx WINELIB_NAME_AW(D3DXCreateVolumeTextureFromResourceEx)

HRESULT WINAPI D3DXCreateTextureFromFileInMemory(      LPDIRECT3DDEVICE9 device,
                                                       LPCVOID srcdata,
                                                       UINT srcdatasize,
                                                       LPDIRECT3DTEXTURE9* texture);

HRESULT WINAPI D3DXCreateCubeTextureFromFileInMemory(  LPDIRECT3DDEVICE9 device,
                                                       LPCVOID srcdata,
                                                       UINT srcdatasize,
                                                       LPDIRECT3DCUBETEXTURE9* cube);

HRESULT WINAPI D3DXCreateVolumeTextureFromFileInMemory(LPDIRECT3DDEVICE9 device,
                                                       LPCVOID srcdata,
                                                       UINT srcdatasize,
                                                       LPDIRECT3DVOLUMETEXTURE9 *volume);

HRESULT WINAPI D3DXCreateTextureFromFileInMemoryEx(      LPDIRECT3DDEVICE9 device,
                                                         LPCVOID srcdata,
                                                         UINT srcdatasize,
                                                         UINT width,
                                                         UINT height,
                                                         UINT miplevels,
                                                         DWORD usage,
                                                         D3DFORMAT format,
                                                         D3DPOOL pool,
                                                         DWORD filter,
                                                         DWORD mipfilter,
                                                         D3DCOLOR colorkey,
                                                         D3DXIMAGE_INFO *srcinfo,
                                                         PALETTEENTRY *palette,
                                                         LPDIRECT3DTEXTURE9 *texture);

HRESULT WINAPI D3DXCreateCubeTextureFromFileInMemoryEx(  LPDIRECT3DDEVICE9 device,
                                                         LPCVOID srcdata,
                                                         UINT srcdatasize,
                                                         UINT size,
                                                         UINT miplevels,
                                                         DWORD usage,
                                                         D3DFORMAT format,
                                                         D3DPOOL pool,
                                                         DWORD filter,
                                                         DWORD mipfilter,
                                                         D3DCOLOR colorkey,
                                                         D3DXIMAGE_INFO *srcinfo,
                                                         PALETTEENTRY *palette,
                                                         LPDIRECT3DCUBETEXTURE9 *cube);

HRESULT WINAPI D3DXCreateVolumeTextureFromFileInMemoryEx(LPDIRECT3DDEVICE9 device,
                                                         LPCVOID srcdata,
                                                         UINT srcdatasize,
                                                         UINT width,
                                                         UINT height,
                                                         UINT depth,
                                                         UINT miplevels,
                                                         DWORD usage,
                                                         D3DFORMAT format,
                                                         D3DPOOL pool,
                                                         DWORD filter,
                                                         DWORD mipfilter,
                                                         D3DCOLOR colorkey,
                                                         D3DXIMAGE_INFO *srcinfo,
                                                         PALETTEENTRY *palette,
                                                         LPDIRECT3DVOLUMETEXTURE9 *volume);

HRESULT WINAPI D3DXSaveTextureToFileA(LPCSTR destfile,
                                      D3DXIMAGE_FILEFORMAT destformat,
                                      LPDIRECT3DBASETEXTURE9 srctexture,
                                      CONST PALETTEENTRY *srcpalette);
HRESULT WINAPI D3DXSaveTextureToFileW(LPCWSTR destfile,
                                      D3DXIMAGE_FILEFORMAT destformat,
                                      LPDIRECT3DBASETEXTURE9 srctexture,
                                      CONST PALETTEENTRY *srcpalette);
#define        D3DXSaveTextureToFile WINELIB_NAME_AW(D3DXSaveTextureToFile)


/* Other functions */
HRESULT WINAPI D3DXFilterTexture(      LPDIRECT3DBASETEXTURE9 texture,
                                       CONST PALETTEENTRY *palette,
                                       UINT srclevel,
                                       DWORD filter);
#define D3DXFilterCubeTexture D3DXFilterTexture
#define D3DXFilterVolumeTexture D3DXFilterTexture

HRESULT WINAPI D3DXFillTexture(        LPDIRECT3DTEXTURE9 texture,
                                       LPD3DXFILL2D function,
                                       LPVOID data);

HRESULT WINAPI D3DXFillCubeTexture(    LPDIRECT3DCUBETEXTURE9 cube,
                                       LPD3DXFILL3D function,
                                       LPVOID data);

HRESULT WINAPI D3DXFillVolumeTexture(  LPDIRECT3DVOLUMETEXTURE9 volume,
                                       LPD3DXFILL3D function,
                                       LPVOID data);

HRESULT WINAPI D3DXFillTextureTX(      LPDIRECT3DTEXTURE9 texture,
                                       CONST DWORD *function,
                                       CONST D3DXVECTOR4 *constants,
                                       UINT numconstants);

HRESULT WINAPI D3DXFillCubeTextureTX(  LPDIRECT3DCUBETEXTURE9 cube,
                                       CONST DWORD *function,
                                       CONST D3DXVECTOR4 *constants,
                                       UINT numconstants);

HRESULT WINAPI D3DXFillVolumeTextureTX(LPDIRECT3DVOLUMETEXTURE9 volume,
                                       CONST DWORD *function,
                                       CONST D3DXVECTOR4 *constants,
                                       UINT numconstants);

HRESULT WINAPI D3DXComputeNormalMap(   LPDIRECT3DTEXTURE9 texture,
                                       LPDIRECT3DTEXTURE9 srctexture,
                                       CONST PALETTEENTRY *srcpalette,
                                       DWORD flags,
                                       DWORD channel,
                                       FLOAT amplitude);


#ifdef __cplusplus
}
#endif

#endif /* __WINE_D3DX9TEX_H */
