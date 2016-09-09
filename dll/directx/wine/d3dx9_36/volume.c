/*
 * Copyright 2010 Christian Costa
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

#include "d3dx9_36_private.h"

HRESULT WINAPI D3DXLoadVolumeFromFileA(IDirect3DVolume9 *dst_volume,
                                       const PALETTEENTRY *dst_palette,
                                       const D3DBOX *dst_box,
                                       const char *filename,
                                       const D3DBOX *src_box,
                                       DWORD filter,
                                       D3DCOLOR color_key,
                                       D3DXIMAGE_INFO *info)
{
    HRESULT hr;
    int length;
    WCHAR *filenameW;

    TRACE("(%p, %p, %p, %s, %p, %#x, %#x, %p)\n",
            dst_volume, dst_palette, dst_box, debugstr_a(filename), src_box,
            filter, color_key, info);

    if (!dst_volume || !filename) return D3DERR_INVALIDCALL;

    length = MultiByteToWideChar(CP_ACP, 0, filename, -1, NULL, 0);
    filenameW = HeapAlloc(GetProcessHeap(), 0, length * sizeof(*filenameW));
    if (!filenameW) return E_OUTOFMEMORY;

    hr = D3DXLoadVolumeFromFileW(dst_volume, dst_palette, dst_box, filenameW,
            src_box, filter, color_key, info);
    HeapFree(GetProcessHeap(), 0, filenameW);

    return hr;
}

HRESULT WINAPI D3DXLoadVolumeFromFileW(IDirect3DVolume9 *dst_volume,
                                       const PALETTEENTRY *dst_palette,
                                       const D3DBOX *dst_box,
                                       const WCHAR *filename,
                                       const D3DBOX *src_box,
                                       DWORD filter,
                                       D3DCOLOR color_key,
                                       D3DXIMAGE_INFO *info)
{
    HRESULT hr;
    void *data;
    UINT data_size;

    TRACE("(%p, %p, %p, %s, %p, %#x, %#x, %p)\n",
            dst_volume, dst_palette, dst_box, debugstr_w(filename), src_box,
            filter, color_key, info);

    if (!dst_volume || !filename) return D3DERR_INVALIDCALL;

    if (FAILED(map_view_of_file(filename, &data, &data_size)))
        return D3DXERR_INVALIDDATA;

    hr = D3DXLoadVolumeFromFileInMemory(dst_volume, dst_palette, dst_box,
            data, data_size, src_box, filter, color_key, info);
    UnmapViewOfFile(data);

    return hr;
}

HRESULT WINAPI D3DXLoadVolumeFromMemory(IDirect3DVolume9 *dst_volume,
                                        const PALETTEENTRY *dst_palette,
                                        const D3DBOX *dst_box,
                                        const void *src_memory,
                                        D3DFORMAT src_format,
                                        UINT src_row_pitch,
                                        UINT src_slice_pitch,
                                        const PALETTEENTRY *src_palette,
                                        const D3DBOX *src_box,
                                        DWORD filter,
                                        D3DCOLOR color_key)
{
    HRESULT hr;
    D3DVOLUME_DESC desc;
    D3DLOCKED_BOX locked_box;
    struct volume dst_size, src_size;
    const struct pixel_format_desc *src_format_desc, *dst_format_desc;

    TRACE("(%p, %p, %p, %p, %#x, %u, %u, %p, %p, %x, %x)\n", dst_volume, dst_palette, dst_box,
            src_memory, src_format, src_row_pitch, src_slice_pitch, src_palette, src_box,
            filter, color_key);

    if (!dst_volume || !src_memory || !src_box) return D3DERR_INVALIDCALL;

    if (src_format == D3DFMT_UNKNOWN
            || src_box->Left >= src_box->Right
            || src_box->Top >= src_box->Bottom
            || src_box->Front >= src_box->Back)
        return E_FAIL;

    if (filter == D3DX_DEFAULT)
        filter = D3DX_FILTER_TRIANGLE | D3DX_FILTER_DITHER;

    IDirect3DVolume9_GetDesc(dst_volume, &desc);

    src_size.width = src_box->Right - src_box->Left;
    src_size.height = src_box->Bottom - src_box->Top;
    src_size.depth = src_box->Back - src_box->Front;

    if (!dst_box)
    {
        dst_size.width = desc.Width;
        dst_size.height = desc.Height;
        dst_size.depth = desc.Depth;
    }
    else
    {
        if (dst_box->Left >= dst_box->Right || dst_box->Right > desc.Width)
            return D3DERR_INVALIDCALL;
        if (dst_box->Top >= dst_box->Bottom || dst_box->Bottom > desc.Height)
            return D3DERR_INVALIDCALL;
        if (dst_box->Front >= dst_box->Back || dst_box->Back > desc.Depth)
            return D3DERR_INVALIDCALL;

        dst_size.width = dst_box->Right - dst_box->Left;
        dst_size.height = dst_box->Bottom - dst_box->Top;
        dst_size.depth = dst_box->Back - dst_box->Front;
    }

    src_format_desc = get_format_info(src_format);
    if (src_format_desc->type == FORMAT_UNKNOWN)
        return E_NOTIMPL;

    dst_format_desc = get_format_info(desc.Format);
    if (dst_format_desc->type == FORMAT_UNKNOWN)
        return E_NOTIMPL;

    if (desc.Format == src_format
            && dst_size.width == src_size.width
            && dst_size.height == src_size.height
            && dst_size.depth == src_size.depth
            && color_key == 0)
    {
        const BYTE *src_addr;

        if (src_box->Left & (src_format_desc->block_width - 1)
                || src_box->Top & (src_format_desc->block_height - 1)
                || (src_box->Right & (src_format_desc->block_width - 1)
                    && src_size.width != desc.Width)
                || (src_box->Bottom & (src_format_desc->block_height - 1)
                    && src_size.height != desc.Height))
        {
            FIXME("Source box (%u, %u, %u, %u) is misaligned\n",
                    src_box->Left, src_box->Top, src_box->Right, src_box->Bottom);
            return E_NOTIMPL;
        }

        src_addr = src_memory;
        src_addr += src_box->Front * src_slice_pitch;
        src_addr += (src_box->Top / src_format_desc->block_height) * src_row_pitch;
        src_addr += (src_box->Left / src_format_desc->block_width) * src_format_desc->block_byte_count;

        hr = IDirect3DVolume9_LockBox(dst_volume, &locked_box, dst_box, 0);
        if (FAILED(hr)) return hr;

        copy_pixels(src_addr, src_row_pitch, src_slice_pitch,
                locked_box.pBits, locked_box.RowPitch, locked_box.SlicePitch,
                &dst_size, dst_format_desc);

        IDirect3DVolume9_UnlockBox(dst_volume);
    }
    else
    {
        const BYTE *src_addr;


        if (((src_format_desc->type != FORMAT_ARGB) && (src_format_desc->type != FORMAT_INDEX)) ||
            (dst_format_desc->type != FORMAT_ARGB))
        {
            FIXME("Pixel format conversion is not implemented %#x -> %#x\n",
                    src_format_desc->format, dst_format_desc->format);
            return E_NOTIMPL;
        }

        src_addr = src_memory;
        src_addr += src_box->Front * src_slice_pitch;
        src_addr += src_box->Top * src_row_pitch;
        src_addr += src_box->Left * src_format_desc->bytes_per_pixel;

        hr = IDirect3DVolume9_LockBox(dst_volume, &locked_box, dst_box, 0);
        if (FAILED(hr)) return hr;

        if ((filter & 0xf) == D3DX_FILTER_NONE)
        {
            convert_argb_pixels(src_memory, src_row_pitch, src_slice_pitch, &src_size, src_format_desc,
                    locked_box.pBits, locked_box.RowPitch, locked_box.SlicePitch, &dst_size, dst_format_desc, color_key,
                    src_palette);
        }
        else
        {
            if ((filter & 0xf) != D3DX_FILTER_POINT)
                FIXME("Unhandled filter %#x.\n", filter);

            point_filter_argb_pixels(src_addr, src_row_pitch, src_slice_pitch, &src_size, src_format_desc,
                    locked_box.pBits, locked_box.RowPitch, locked_box.SlicePitch, &dst_size, dst_format_desc, color_key,
                    src_palette);
        }

        IDirect3DVolume9_UnlockBox(dst_volume);
    }

    return D3D_OK;
}

HRESULT WINAPI D3DXLoadVolumeFromFileInMemory(IDirect3DVolume9 *dst_volume,
                                              const PALETTEENTRY *dst_palette,
                                              const D3DBOX *dst_box,
                                              const void *src_data,
                                              UINT src_data_size,
                                              const D3DBOX *src_box,
                                              DWORD filter,
                                              D3DCOLOR color_key,
                                              D3DXIMAGE_INFO *src_info)
{
    HRESULT hr;
    D3DBOX box;
    D3DXIMAGE_INFO image_info;

    TRACE("dst_volume %p, dst_palette %p, dst_box %p, src_data %p, src_data_size %u, src_box %p,\n",
            dst_volume, dst_palette, dst_box, src_data, src_data_size, src_box);
    TRACE("filter %#x, color_key %#x, src_info %p.\n", filter, color_key, src_info);

    if (!dst_volume || !src_data) return D3DERR_INVALIDCALL;

    hr = D3DXGetImageInfoFromFileInMemory(src_data, src_data_size, &image_info);
    if (FAILED(hr)) return hr;

    if (src_box)
    {
        if (src_box->Right > image_info.Width
                || src_box->Bottom > image_info.Height
                || src_box->Back > image_info.Depth)
            return D3DERR_INVALIDCALL;

        box = *src_box;
    }
    else
    {
        box.Left = 0;
        box.Top = 0;
        box.Right = image_info.Width;
        box.Bottom = image_info.Height;
        box.Front = 0;
        box.Back = image_info.Depth;

    }

    if (image_info.ImageFileFormat != D3DXIFF_DDS)
    {
        FIXME("File format %#x is not supported yet\n", image_info.ImageFileFormat);
        return E_NOTIMPL;
    }

    hr = load_volume_from_dds(dst_volume, dst_palette, dst_box, src_data, &box,
            filter, color_key, &image_info);
    if (FAILED(hr)) return hr;

    if (src_info)
        *src_info = image_info;

    return D3D_OK;
}

HRESULT WINAPI D3DXLoadVolumeFromVolume(IDirect3DVolume9 *dst_volume,
                                        const PALETTEENTRY *dst_palette,
                                        const D3DBOX *dst_box,
                                        IDirect3DVolume9 *src_volume,
                                        const PALETTEENTRY *src_palette,
                                        const D3DBOX *src_box,
                                        DWORD filter,
                                        D3DCOLOR color_key)
{
    HRESULT hr;
    D3DBOX box;
    D3DVOLUME_DESC desc;
    D3DLOCKED_BOX locked_box;

    TRACE("(%p, %p, %p, %p, %p, %p, %#x, %#x)\n",
            dst_volume, dst_palette, dst_box, src_volume, src_palette, src_box,
            filter, color_key);

    if (!dst_volume || !src_volume) return D3DERR_INVALIDCALL;

    IDirect3DVolume9_GetDesc(src_volume, &desc);

    if (!src_box)
    {
        box.Left = box.Top = 0;
        box.Right = desc.Width;
        box.Bottom = desc.Height;
        box.Front = 0;
        box.Back = desc.Depth;
    }
    else box = *src_box;

    hr = IDirect3DVolume9_LockBox(src_volume, &locked_box, NULL, D3DLOCK_READONLY);
    if (FAILED(hr)) return hr;

    hr = D3DXLoadVolumeFromMemory(dst_volume, dst_palette, dst_box,
            locked_box.pBits, desc.Format, locked_box.RowPitch, locked_box.SlicePitch,
            src_palette, &box, filter, color_key);

    IDirect3DVolume9_UnlockBox(src_volume);
    return hr;
}
