/*
 * Copyright (C) 2009 Tony Wasserka
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
 *
 */

#include "wine/debug.h"
#include "wine/unicode.h"
#include "d3dx9_36_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);


/************************************************************
 * D3DXGetImageInfoFromFileInMemory
 *
 * Fills a D3DXIMAGE_INFO structure with info about an image
 *
 * PARAMS
 *   data     [I] pointer to the image file data
 *   datasize [I] size of the passed data
 *   info     [O] pointer to the destination structure
 *
 * RETURNS
 *   Success: D3D_OK, if info is not NULL and data and datasize make up a valid image file or
 *                    if info is NULL and data and datasize are not NULL
 *   Failure: D3DXERR_INVALIDDATA, if data is no valid image file and datasize and info are not NULL
 *            D3DERR_INVALIDCALL, if data is NULL or
 *                                if datasize is 0
 *
 * NOTES
 *   datasize may be bigger than the actual file size
 *
 */
HRESULT WINAPI D3DXGetImageInfoFromFileInMemory(LPCVOID data, UINT datasize, D3DXIMAGE_INFO *info)
{
    FIXME("stub\n");

    if(data && datasize && !info) return D3D_OK;
    if( !data || !datasize ) return D3DERR_INVALIDCALL;

    return E_NOTIMPL;
}

/************************************************************
 * D3DXGetImageInfoFromFile
 *
 * RETURNS
 *   Success: D3D_OK, if we successfully load a valid image file or
 *                    if we successfully load a file which is no valid image and info is NULL
 *   Failure: D3DXERR_INVALIDDATA, if we fail to load file or
 *                                 if file is not a valid image file and info is not NULL
 *            D3DERR_INVALIDCALL, if file is NULL
 *
 */
HRESULT WINAPI D3DXGetImageInfoFromFileA(LPCSTR file, D3DXIMAGE_INFO *info)
{
    LPWSTR widename;
    HRESULT hr;
    int strlength;
    TRACE("(void): relay\n");

    if( !file ) return D3DERR_INVALIDCALL;

    strlength = MultiByteToWideChar(CP_ACP, 0, file, -1, NULL, 0);
    widename = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, strlength * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, file, -1, widename, strlength);

    hr = D3DXGetImageInfoFromFileW(widename, info);
    HeapFree(GetProcessHeap(), 0, widename);

    return hr;
}

HRESULT WINAPI D3DXGetImageInfoFromFileW(LPCWSTR file, D3DXIMAGE_INFO *info)
{
    HRESULT hr;
    DWORD size;
    LPVOID buffer;
    TRACE("(void): relay\n");

    if( !file ) return D3DERR_INVALIDCALL;

    hr = map_view_of_file(file, &buffer, &size);
    if(FAILED(hr)) return D3DXERR_INVALIDDATA;

    hr = D3DXGetImageInfoFromFileInMemory(buffer, size, info);
    UnmapViewOfFile(buffer);

    return hr;
}

/************************************************************
 * D3DXGetImageInfoFromResource
 *
 * RETURNS
 *   Success: D3D_OK, if resource is a valid image file
 *   Failure: D3DXERR_INVALIDDATA, if resource is no valid image file or NULL or
 *                                 if we fail to load resource
 *
 */
HRESULT WINAPI D3DXGetImageInfoFromResourceA(HMODULE module, LPCSTR resource, D3DXIMAGE_INFO *info)
{
    HRSRC resinfo;
    TRACE("(void)\n");

    resinfo = FindResourceA(module, resource, (LPCSTR)RT_RCDATA);
    if(resinfo) {
        LPVOID buffer;
        HRESULT hr;
        DWORD size;

        hr = load_resource_into_memory(module, resinfo, &buffer, &size);
        if(FAILED(hr)) return D3DXERR_INVALIDDATA;
        return D3DXGetImageInfoFromFileInMemory(buffer, size, info);
    }

    resinfo = FindResourceA(module, resource, (LPCSTR)RT_BITMAP);
    if(resinfo) {
        FIXME("Implement loading bitmaps from resource type RT_BITMAP\n");
        return E_NOTIMPL;
    }
    return D3DXERR_INVALIDDATA;
}

HRESULT WINAPI D3DXGetImageInfoFromResourceW(HMODULE module, LPCWSTR resource, D3DXIMAGE_INFO *info)
{
    HRSRC resinfo;
    TRACE("(void)\n");

    resinfo = FindResourceW(module, resource, (LPCWSTR)RT_RCDATA);
    if(resinfo) {
        LPVOID buffer;
        HRESULT hr;
        DWORD size;

        hr = load_resource_into_memory(module, resinfo, &buffer, &size);
        if(FAILED(hr)) return D3DXERR_INVALIDDATA;
        return D3DXGetImageInfoFromFileInMemory(buffer, size, info);
    }

    resinfo = FindResourceW(module, resource, (LPCWSTR)RT_BITMAP);
    if(resinfo) {
        FIXME("Implement loading bitmaps from resource type RT_BITMAP\n");
        return E_NOTIMPL;
    }
    return D3DXERR_INVALIDDATA;
}

/************************************************************
 * D3DXLoadSurfaceFromFileInMemory
 *
 * Loads data from a given buffer into a surface and fills a given
 * D3DXIMAGE_INFO structure with info about the source data.
 *
 * PARAMS
 *   pDestSurface [I] pointer to the surface
 *   pDestPalette [I] palette to use
 *   pDestRect    [I] to be filled area of the surface
 *   pSrcData     [I] pointer to the source data
 *   SrcDataSize  [I] size of the source data in bytes
 *   pSrcRect     [I] area of the source data to load
 *   dwFilter     [I] filter to apply on stretching
 *   Colorkey     [I] colorkey
 *   pSrcInfo     [O] pointer to a D3DXIMAGE_INFO structure
 *
 * RETURNS
 *   Success: D3D_OK
 *   Failure: D3DERR_INVALIDCALL, if pDestSurface or pSrcData or SrcDataSize are NULL
 *            D3DXERR_INVALIDDATA, if pSrcData is no valid image file
 *
 */
HRESULT WINAPI D3DXLoadSurfaceFromFileInMemory(LPDIRECT3DSURFACE9 pDestSurface,
                                               CONST PALETTEENTRY *pDestPalette,
                                               CONST RECT *pDestRect,
                                               LPCVOID pSrcData,
                                               UINT SrcDataSize,
                                               CONST RECT *pSrcRect,
                                               DWORD dwFilter,
                                               D3DCOLOR Colorkey,
                                               D3DXIMAGE_INFO *pSrcInfo)
{
    FIXME("stub\n");
    if( !pDestSurface || !pSrcData | !SrcDataSize ) return D3DERR_INVALIDCALL;
    return E_NOTIMPL;
}

/************************************************************
 * D3DXLoadSurfaceFromFile
 */
HRESULT WINAPI D3DXLoadSurfaceFromFileA(LPDIRECT3DSURFACE9 pDestSurface,
                                        CONST PALETTEENTRY *pDestPalette,
                                        CONST RECT *pDestRect,
                                        LPCSTR pSrcFile,
                                        CONST RECT *pSrcRect,
                                        DWORD dwFilter,
                                        D3DCOLOR Colorkey,
                                        D3DXIMAGE_INFO *pSrcInfo)
{
    LPWSTR pWidename;
    HRESULT hr;
    int strlength;
    TRACE("(void): relay\n");

    if( !pSrcFile || !pDestSurface ) return D3DERR_INVALIDCALL;

    strlength = MultiByteToWideChar(CP_ACP, 0, pSrcFile, -1, NULL, 0);
    pWidename = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, strlength * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, pSrcFile, -1, pWidename, strlength);

    hr = D3DXLoadSurfaceFromFileW(pDestSurface, pDestPalette, pDestRect, pWidename, pSrcRect, dwFilter, Colorkey, pSrcInfo);
    HeapFree(GetProcessHeap(), 0, pWidename);

    return hr;
}

HRESULT WINAPI D3DXLoadSurfaceFromFileW(LPDIRECT3DSURFACE9 pDestSurface,
                                        CONST PALETTEENTRY *pDestPalette,
                                        CONST RECT *pDestRect,
                                        LPCWSTR pSrcFile,
                                        CONST RECT *pSrcRect,
                                        DWORD Filter,
                                        D3DCOLOR Colorkey,
                                        D3DXIMAGE_INFO *pSrcInfo)
{
    HRESULT hr;
    DWORD dwSize;
    LPVOID pBuffer;
    TRACE("(void): relay\n");

    if( !pSrcFile || !pDestSurface ) return D3DERR_INVALIDCALL;

    hr = map_view_of_file(pSrcFile, &pBuffer, &dwSize);
    if(FAILED(hr)) return D3DXERR_INVALIDDATA;

    hr = D3DXLoadSurfaceFromFileInMemory(pDestSurface, pDestPalette, pDestRect, pBuffer, dwSize, pSrcRect, Filter, Colorkey, pSrcInfo);
    UnmapViewOfFile(pBuffer);

    return hr;
}

/************************************************************
 * D3DXLoadSurfaceFromResource
 */
HRESULT WINAPI D3DXLoadSurfaceFromResourceA(LPDIRECT3DSURFACE9 pDestSurface,
                                            CONST PALETTEENTRY *pDestPalette,
                                            CONST RECT *pDestRect,
                                            HMODULE hSrcModule,
                                            LPCSTR pResource,
                                            CONST RECT *pSrcRect,
                                            DWORD dwFilter,
                                            D3DCOLOR Colorkey,
                                            D3DXIMAGE_INFO *pSrcInfo)
{
    HRSRC hResInfo;
    TRACE("(void): relay\n");

    if( !pDestSurface ) return D3DERR_INVALIDCALL;

    hResInfo = FindResourceA(hSrcModule, pResource, (LPCSTR)RT_RCDATA);
    if(hResInfo) {
        LPVOID pBuffer;
        HRESULT hr;
        DWORD dwSize;

        hr = load_resource_into_memory(hSrcModule, hResInfo, &pBuffer, &dwSize);
        if(FAILED(hr)) return D3DXERR_INVALIDDATA;
        return D3DXLoadSurfaceFromFileInMemory(pDestSurface, pDestPalette, pDestRect, pBuffer, dwSize, pSrcRect, dwFilter, Colorkey, pSrcInfo);
    }

    hResInfo = FindResourceA(hSrcModule, pResource, (LPCSTR)RT_BITMAP);
    if(hResInfo) {
        FIXME("Implement loading bitmaps from resource type RT_BITMAP\n");
        return E_NOTIMPL;
    }
    return D3DXERR_INVALIDDATA;
}

HRESULT WINAPI D3DXLoadSurfaceFromResourceW(LPDIRECT3DSURFACE9 pDestSurface,
                                            CONST PALETTEENTRY *pDestPalette,
                                            CONST RECT *pDestRect,
                                            HMODULE hSrcModule,
                                            LPCWSTR pResource,
                                            CONST RECT *pSrcRect,
                                            DWORD dwFilter,
                                            D3DCOLOR Colorkey,
                                            D3DXIMAGE_INFO *pSrcInfo)
{
    HRSRC hResInfo;
    TRACE("(void): relay\n");

    if( !pDestSurface ) return D3DERR_INVALIDCALL;

    hResInfo = FindResourceW(hSrcModule, pResource, (LPCWSTR)RT_RCDATA);
    if(hResInfo) {
        LPVOID pBuffer;
        HRESULT hr;
        DWORD dwSize;

        hr = load_resource_into_memory(hSrcModule, hResInfo, &pBuffer, &dwSize);
        if(FAILED(hr)) return D3DXERR_INVALIDDATA;
        return D3DXLoadSurfaceFromFileInMemory(pDestSurface, pDestPalette, pDestRect, pBuffer, dwSize, pSrcRect, dwFilter, Colorkey, pSrcInfo);
    }

    hResInfo = FindResourceW(hSrcModule, pResource, (LPCWSTR)RT_BITMAP);
    if(hResInfo) {
        FIXME("Implement loading bitmaps from resource type RT_BITMAP\n");
        return E_NOTIMPL;
    }
    return D3DXERR_INVALIDDATA;
}

/************************************************************
 * D3DXLoadSurfaceFromMemory
 *
 * Loads data from a given memory chunk into a surface,
 * applying any of the specified filters.
 *
 * PARAMS
 *   pDestSurface [I] pointer to the surface
 *   pDestPalette [I] palette to use
 *   pDestRect    [I] to be filled area of the surface
 *   pSrcMemory   [I] pointer to the source data
 *   SrcFormat    [I] format of the source pixel data
 *   SrcPitch     [I] number of bytes in a row
 *   pSrcPalette  [I] palette used in the source image
 *   pSrcRect     [I] area of the source data to load
 *   dwFilter     [I] filter to apply on stretching
 *   Colorkey     [I] colorkey
 *
 * RETURNS
 *   Success: D3D_OK, if we successfully load the pixel data into our surface or
 *                    if pSrcMemory is NULL but the other parameters are valid
 *   Failure: D3DERR_INVALIDCALL, if pDestSurface, SrcPitch or pSrcRect are NULL or
 *                                if SrcFormat is an invalid format (other than D3DFMT_UNKNOWN)
 *            D3DXERR_INVALIDDATA, if we fail to lock pDestSurface
 *            E_FAIL, if SrcFormat is D3DFMT_UNKNOWN or the dimensions of pSrcRect are invalid
 *
 * NOTES
 *   pSrcRect specifies the dimensions of the source data
 *
 */
HRESULT WINAPI D3DXLoadSurfaceFromMemory(LPDIRECT3DSURFACE9 pDestSurface,
                                         CONST PALETTEENTRY *pDestPalette,
                                         CONST RECT *pDestRect,
                                         LPCVOID pSrcMemory,
                                         D3DFORMAT SrcFormat,
                                         UINT SrcPitch,
                                         CONST PALETTEENTRY *pSrcPalette,
                                         CONST RECT *pSrcRect,
                                         DWORD dwFilter,
                                         D3DCOLOR Colorkey)
{
    TRACE("stub\n");

    if( !pDestSurface || !pSrcMemory || !pSrcRect ) return D3DERR_INVALIDCALL;
    if(SrcFormat == D3DFMT_UNKNOWN || pSrcRect->left >= pSrcRect->right || pSrcRect->top >= pSrcRect->bottom) return E_FAIL;
    return E_NOTIMPL;
}

/************************************************************
 * D3DXLoadSurfaceFromSurface
 *
 * Copies the contents from one surface to another, performing any required
 * format conversion, resizing or filtering.
 *
 * PARAMS
 *   pDestSurface [I] pointer to the destination surface
 *   pDestPalette [I] palette to use
 *   pDestRect    [I] to be filled area of the surface
 *   pSrcSurface  [I] pointer to the source surface
 *   pSrcPalette  [I] palette used for the source surface
 *   pSrcRect     [I] area of the source data to load
 *   dwFilter     [I] filter to apply on resizing
 *   Colorkey     [I] any ARGB value or 0 to disable color-keying
 *
 * RETURNS
 *   Success: D3D_OK
 *   Failure: D3DERR_INVALIDCALL, if pDestSurface or pSrcSurface are NULL
 *            D3DXERR_INVALIDDATA, if one of the surfaces is not lockable
 *
 */
HRESULT WINAPI D3DXLoadSurfaceFromSurface(LPDIRECT3DSURFACE9 pDestSurface,
                                          CONST PALETTEENTRY *pDestPalette,
                                          CONST RECT *pDestRect,
                                          LPDIRECT3DSURFACE9 pSrcSurface,
                                          CONST PALETTEENTRY *pSrcPalette,
                                          CONST RECT *pSrcRect,
                                          DWORD dwFilter,
                                          D3DCOLOR Colorkey)
{
    RECT rect;
    D3DLOCKED_RECT lock;
    D3DSURFACE_DESC SrcDesc;
    HRESULT hr;
    TRACE("(void): relay\n");

    if( !pDestSurface || !pSrcSurface ) return D3DERR_INVALIDCALL;

    IDirect3DSurface9_GetDesc(pSrcSurface, &SrcDesc);

    if( !pSrcRect ) SetRect(&rect, 0, 0, SrcDesc.Width, SrcDesc.Height);
    else rect = *pSrcRect;

    hr = IDirect3DSurface9_LockRect(pSrcSurface, &lock, NULL, D3DLOCK_READONLY);
    if(FAILED(hr)) return D3DXERR_INVALIDDATA;

    hr = D3DXLoadSurfaceFromMemory(pDestSurface, pDestPalette, pDestRect,
                                   lock.pBits, SrcDesc.Format, lock.Pitch,
                                   pSrcPalette, &rect, dwFilter, Colorkey);

    IDirect3DSurface9_UnlockRect(pSrcSurface);
    return hr;
}
