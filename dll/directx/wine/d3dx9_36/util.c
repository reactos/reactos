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


#include "d3dx9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

/************************************************************
 * pixel format table providing info about number of bytes per pixel,
 * number of bits per channel and format type.
 *
 * Call get_format_info to request information about a specific format.
 */
static const struct pixel_format_desc formats[] =
{
    /* format                                    bpc               shifts             bpp blocks   alpha type   rgb type     flags */
    {D3DX_PIXEL_FORMAT_B8G8R8_UNORM,             { 0,  8,  8,  8}, { 0, 16,  8,  0},  3, 1, 1,  3, CTYPE_EMPTY, CTYPE_UNORM, 0           },
    {D3DX_PIXEL_FORMAT_B8G8R8A8_UNORM,           { 8,  8,  8,  8}, {24, 16,  8,  0},  4, 1, 1,  4, CTYPE_UNORM, CTYPE_UNORM, 0           },
    {D3DX_PIXEL_FORMAT_B8G8R8X8_UNORM,           { 0,  8,  8,  8}, { 0, 16,  8,  0},  4, 1, 1,  4, CTYPE_EMPTY, CTYPE_UNORM, 0           },
    {D3DX_PIXEL_FORMAT_R8G8B8A8_UNORM,           { 8,  8,  8,  8}, {24,  0,  8, 16},  4, 1, 1,  4, CTYPE_UNORM, CTYPE_UNORM, 0           },
    {D3DX_PIXEL_FORMAT_R8G8B8X8_UNORM,           { 0,  8,  8,  8}, { 0,  0,  8, 16},  4, 1, 1,  4, CTYPE_EMPTY, CTYPE_UNORM, 0           },
    {D3DX_PIXEL_FORMAT_B5G6R5_UNORM,             { 0,  5,  6,  5}, { 0, 11,  5,  0},  2, 1, 1,  2, CTYPE_EMPTY, CTYPE_UNORM, 0           },
    {D3DX_PIXEL_FORMAT_B5G5R5X1_UNORM,           { 0,  5,  5,  5}, { 0, 10,  5,  0},  2, 1, 1,  2, CTYPE_EMPTY, CTYPE_UNORM, 0           },
    {D3DX_PIXEL_FORMAT_B5G5R5A1_UNORM,           { 1,  5,  5,  5}, {15, 10,  5,  0},  2, 1, 1,  2, CTYPE_UNORM, CTYPE_UNORM, 0           },
    {D3DX_PIXEL_FORMAT_B2G3R3_UNORM,             { 0,  3,  3,  2}, { 0,  5,  2,  0},  1, 1, 1,  1, CTYPE_EMPTY, CTYPE_UNORM, 0           },
    {D3DX_PIXEL_FORMAT_B2G3R3A8_UNORM,           { 8,  3,  3,  2}, { 8,  5,  2,  0},  2, 1, 1,  2, CTYPE_UNORM, CTYPE_UNORM, 0           },
    {D3DX_PIXEL_FORMAT_B4G4R4A4_UNORM,           { 4,  4,  4,  4}, {12,  8,  4,  0},  2, 1, 1,  2, CTYPE_UNORM, CTYPE_UNORM, 0           },
    {D3DX_PIXEL_FORMAT_B4G4R4X4_UNORM,           { 0,  4,  4,  4}, { 0,  8,  4,  0},  2, 1, 1,  2, CTYPE_EMPTY, CTYPE_UNORM, 0           },
    {D3DX_PIXEL_FORMAT_B10G10R10A2_UNORM,        { 2, 10, 10, 10}, {30, 20, 10,  0},  4, 1, 1,  4, CTYPE_UNORM, CTYPE_UNORM, 0           },
    {D3DX_PIXEL_FORMAT_R10G10B10A2_UNORM,        { 2, 10, 10, 10}, {30,  0, 10, 20},  4, 1, 1,  4, CTYPE_UNORM, CTYPE_UNORM, 0           },
    {D3DX_PIXEL_FORMAT_R16G16B16_UNORM,          { 0, 16, 16, 16}, { 0,  0, 16, 32},  6, 1, 1,  6, CTYPE_EMPTY, CTYPE_UNORM, FMT_FLAG_INTERNAL},
    {D3DX_PIXEL_FORMAT_R16G16B16A16_UNORM,       {16, 16, 16, 16}, {48,  0, 16, 32},  8, 1, 1,  8, CTYPE_UNORM, CTYPE_UNORM, 0           },
    {D3DX_PIXEL_FORMAT_R16G16_UNORM,             { 0, 16, 16,  0}, { 0,  0, 16,  0},  4, 1, 1,  4, CTYPE_EMPTY, CTYPE_UNORM, 0           },
    {D3DX_PIXEL_FORMAT_A8_UNORM,                 { 8,  0,  0,  0}, { 0,  0,  0,  0},  1, 1, 1,  1, CTYPE_UNORM, CTYPE_EMPTY, 0           },
    {D3DX_PIXEL_FORMAT_L8A8_UNORM,               { 8,  8,  0,  0}, { 8,  0,  0,  0},  2, 1, 1,  2, CTYPE_UNORM, CTYPE_LUMA,  0           },
    {D3DX_PIXEL_FORMAT_L4A4_UNORM,               { 4,  4,  0,  0}, { 4,  0,  0,  0},  1, 1, 1,  1, CTYPE_UNORM, CTYPE_LUMA,  0           },
    {D3DX_PIXEL_FORMAT_L8_UNORM,                 { 0,  8,  0,  0}, { 0,  0,  0,  0},  1, 1, 1,  1, CTYPE_EMPTY, CTYPE_LUMA,  0           },
    {D3DX_PIXEL_FORMAT_L16_UNORM,                { 0, 16,  0,  0}, { 0,  0,  0,  0},  2, 1, 1,  2, CTYPE_EMPTY, CTYPE_LUMA,  0           },
    {D3DX_PIXEL_FORMAT_DXT1_UNORM,               { 0,  0,  0,  0}, { 0,  0,  0,  0},  1, 4, 4,  8, CTYPE_UNORM, CTYPE_UNORM, FMT_FLAG_DXT},
    {D3DX_PIXEL_FORMAT_DXT2_UNORM,               { 0,  0,  0,  0}, { 0,  0,  0,  0},  1, 4, 4, 16, CTYPE_UNORM, CTYPE_UNORM, FMT_FLAG_DXT},
    {D3DX_PIXEL_FORMAT_DXT3_UNORM,               { 0,  0,  0,  0}, { 0,  0,  0,  0},  1, 4, 4, 16, CTYPE_UNORM, CTYPE_UNORM, FMT_FLAG_DXT},
    {D3DX_PIXEL_FORMAT_DXT4_UNORM,               { 0,  0,  0,  0}, { 0,  0,  0,  0},  1, 4, 4, 16, CTYPE_UNORM, CTYPE_UNORM, FMT_FLAG_DXT},
    {D3DX_PIXEL_FORMAT_DXT5_UNORM,               { 0,  0,  0,  0}, { 0,  0,  0,  0},  1, 4, 4, 16, CTYPE_UNORM, CTYPE_UNORM, FMT_FLAG_DXT},
    {D3DX_PIXEL_FORMAT_R16_FLOAT,                { 0, 16,  0,  0}, { 0,  0,  0,  0},  2, 1, 1,  2, CTYPE_EMPTY, CTYPE_FLOAT, 0           },
    {D3DX_PIXEL_FORMAT_R16G16_FLOAT,             { 0, 16, 16,  0}, { 0,  0, 16,  0},  4, 1, 1,  4, CTYPE_EMPTY, CTYPE_FLOAT, 0           },
    {D3DX_PIXEL_FORMAT_R16G16B16A16_FLOAT,       {16, 16, 16, 16}, {48,  0, 16, 32},  8, 1, 1,  8, CTYPE_FLOAT, CTYPE_FLOAT, 0           },
    {D3DX_PIXEL_FORMAT_R32_FLOAT,                { 0, 32,  0,  0}, { 0,  0,  0,  0},  4, 1, 1,  4, CTYPE_EMPTY, CTYPE_FLOAT, 0           },
    {D3DX_PIXEL_FORMAT_R32G32_FLOAT,             { 0, 32, 32,  0}, { 0,  0, 32,  0},  8, 1, 1,  8, CTYPE_EMPTY, CTYPE_FLOAT, 0           },
    {D3DX_PIXEL_FORMAT_R32G32B32A32_FLOAT,       {32, 32, 32, 32}, {96,  0, 32, 64}, 16, 1, 1, 16, CTYPE_FLOAT, CTYPE_FLOAT, 0           },
    {D3DX_PIXEL_FORMAT_P8_UINT,                  { 8,  8,  8,  8}, { 0,  0,  0,  0},  1, 1, 1,  1, CTYPE_INDEX, CTYPE_INDEX, 0           },
    {D3DX_PIXEL_FORMAT_P8_UINT_A8_UNORM,         { 8,  8,  8,  8}, { 8,  0,  0,  0},  2, 1, 1,  2, CTYPE_UNORM, CTYPE_INDEX, 0           },
    {D3DX_PIXEL_FORMAT_U8V8W8Q8_SNORM,           { 8,  8,  8,  8}, {24,  0,  8, 16},  4, 1, 1,  4, CTYPE_SNORM, CTYPE_SNORM, 0           },
    {D3DX_PIXEL_FORMAT_U16V16W16Q16_SNORM,       {16, 16, 16, 16}, {48,  0, 16, 32},  8, 1, 1,  8, CTYPE_SNORM, CTYPE_SNORM, 0           },
    {D3DX_PIXEL_FORMAT_U8V8_SNORM,               { 0,  8,  8,  0}, { 0,  0,  8,  0},  2, 1, 1,  2, CTYPE_EMPTY, CTYPE_SNORM, 0           },
    {D3DX_PIXEL_FORMAT_U16V16_SNORM,             { 0, 16, 16,  0}, { 0,  0, 16,  0},  4, 1, 1,  4, CTYPE_EMPTY, CTYPE_SNORM, 0           },
    {D3DX_PIXEL_FORMAT_U8V8_SNORM_L8X8_UNORM,    { 8,  8,  8,  0}, {16,  0,  8,  0},  4, 1, 1,  4, CTYPE_UNORM, CTYPE_SNORM, 0           },
    {D3DX_PIXEL_FORMAT_U10V10W10_SNORM_A2_UNORM, { 2, 10, 10, 10}, {30,  0, 10, 20},  4, 1, 1,  4, CTYPE_UNORM, CTYPE_SNORM, 0           },
    {D3DX_PIXEL_FORMAT_R8G8_B8G8_UNORM,          { 0,  0,  0,  0}, { 0,  0,  0,  0},  1, 2, 1,  4, CTYPE_EMPTY, CTYPE_UNORM, FMT_FLAG_PACKED},
    {D3DX_PIXEL_FORMAT_G8R8_G8B8_UNORM,          { 0,  0,  0,  0}, { 0,  0,  0,  0},  1, 2, 1,  4, CTYPE_EMPTY, CTYPE_UNORM, FMT_FLAG_PACKED},
    {D3DX_PIXEL_FORMAT_UYVY,                     { 0,  0,  0,  0}, { 0,  0,  0,  0},  1, 2, 1,  4, CTYPE_EMPTY, CTYPE_UNORM, FMT_FLAG_PACKED},
    {D3DX_PIXEL_FORMAT_YUY2,                     { 0,  0,  0,  0}, { 0,  0,  0,  0},  1, 2, 1,  4, CTYPE_EMPTY, CTYPE_UNORM, FMT_FLAG_PACKED},
    /* marks last element */
    {D3DX_PIXEL_FORMAT_COUNT,                    { 0,  0,  0,  0}, { 0,  0,  0,  0},  0, 1, 1,  0, CTYPE_EMPTY, CTYPE_EMPTY, 0           },
};

D3DFORMAT d3dformat_from_d3dx_pixel_format_id(enum d3dx_pixel_format_id format)
{
    switch (format)
    {
        case D3DX_PIXEL_FORMAT_B8G8R8_UNORM:             return D3DFMT_R8G8B8;
        case D3DX_PIXEL_FORMAT_B8G8R8A8_UNORM:           return D3DFMT_A8R8G8B8;
        case D3DX_PIXEL_FORMAT_B8G8R8X8_UNORM:           return D3DFMT_X8R8G8B8;
        case D3DX_PIXEL_FORMAT_R8G8B8A8_UNORM:           return D3DFMT_A8B8G8R8;
        case D3DX_PIXEL_FORMAT_R8G8B8X8_UNORM:           return D3DFMT_X8B8G8R8;
        case D3DX_PIXEL_FORMAT_B5G6R5_UNORM:             return D3DFMT_R5G6B5;
        case D3DX_PIXEL_FORMAT_B5G5R5X1_UNORM:           return D3DFMT_X1R5G5B5;
        case D3DX_PIXEL_FORMAT_B5G5R5A1_UNORM:           return D3DFMT_A1R5G5B5;
        case D3DX_PIXEL_FORMAT_B2G3R3_UNORM:             return D3DFMT_R3G3B2;
        case D3DX_PIXEL_FORMAT_B2G3R3A8_UNORM:           return D3DFMT_A8R3G3B2;
        case D3DX_PIXEL_FORMAT_B4G4R4A4_UNORM:           return D3DFMT_A4R4G4B4;
        case D3DX_PIXEL_FORMAT_B4G4R4X4_UNORM:           return D3DFMT_X4R4G4B4;
        case D3DX_PIXEL_FORMAT_B10G10R10A2_UNORM:        return D3DFMT_A2R10G10B10;
        case D3DX_PIXEL_FORMAT_R10G10B10A2_UNORM:        return D3DFMT_A2B10G10R10;
        case D3DX_PIXEL_FORMAT_R16G16B16A16_UNORM:       return D3DFMT_A16B16G16R16;
        case D3DX_PIXEL_FORMAT_R16G16_UNORM:             return D3DFMT_G16R16;
        case D3DX_PIXEL_FORMAT_A8_UNORM:                 return D3DFMT_A8;
        case D3DX_PIXEL_FORMAT_L8A8_UNORM:               return D3DFMT_A8L8;
        case D3DX_PIXEL_FORMAT_L4A4_UNORM:               return D3DFMT_A4L4;
        case D3DX_PIXEL_FORMAT_L8_UNORM:                 return D3DFMT_L8;
        case D3DX_PIXEL_FORMAT_L16_UNORM:                return D3DFMT_L16;
        case D3DX_PIXEL_FORMAT_DXT1_UNORM:               return D3DFMT_DXT1;
        case D3DX_PIXEL_FORMAT_DXT2_UNORM:               return D3DFMT_DXT2;
        case D3DX_PIXEL_FORMAT_DXT3_UNORM:               return D3DFMT_DXT3;
        case D3DX_PIXEL_FORMAT_DXT4_UNORM:               return D3DFMT_DXT4;
        case D3DX_PIXEL_FORMAT_DXT5_UNORM:               return D3DFMT_DXT5;
        case D3DX_PIXEL_FORMAT_R16_FLOAT:                return D3DFMT_R16F;
        case D3DX_PIXEL_FORMAT_R16G16_FLOAT:             return D3DFMT_G16R16F;
        case D3DX_PIXEL_FORMAT_R16G16B16A16_FLOAT:       return D3DFMT_A16B16G16R16F;
        case D3DX_PIXEL_FORMAT_R32_FLOAT:                return D3DFMT_R32F;
        case D3DX_PIXEL_FORMAT_R32G32_FLOAT:             return D3DFMT_G32R32F;
        case D3DX_PIXEL_FORMAT_R32G32B32A32_FLOAT:       return D3DFMT_A32B32G32R32F;
        case D3DX_PIXEL_FORMAT_P8_UINT:                  return D3DFMT_P8;
        case D3DX_PIXEL_FORMAT_P8_UINT_A8_UNORM:         return D3DFMT_A8P8;
        case D3DX_PIXEL_FORMAT_U8V8W8Q8_SNORM:           return D3DFMT_Q8W8V8U8;
        case D3DX_PIXEL_FORMAT_U8V8_SNORM:               return D3DFMT_V8U8;
        case D3DX_PIXEL_FORMAT_U16V16_SNORM:             return D3DFMT_V16U16;
        case D3DX_PIXEL_FORMAT_U8V8_SNORM_L8X8_UNORM:    return D3DFMT_X8L8V8U8;
        case D3DX_PIXEL_FORMAT_U10V10W10_SNORM_A2_UNORM: return D3DFMT_A2W10V10U10;
        case D3DX_PIXEL_FORMAT_U16V16W16Q16_SNORM:       return D3DFMT_Q16W16V16U16;
        case D3DX_PIXEL_FORMAT_G8R8_G8B8_UNORM:          return D3DFMT_G8R8_G8B8;
        case D3DX_PIXEL_FORMAT_R8G8_B8G8_UNORM:          return D3DFMT_R8G8_B8G8;
        case D3DX_PIXEL_FORMAT_UYVY:                     return D3DFMT_UYVY;
        case D3DX_PIXEL_FORMAT_YUY2:                     return D3DFMT_YUY2;
        default:
            if (!is_internal_format(get_d3dx_pixel_format_info(format)))
                FIXME("Unknown d3dx_pixel_format_id %u.\n", format);
            return D3DFMT_UNKNOWN;
    }
}

enum d3dx_pixel_format_id d3dx_pixel_format_id_from_d3dformat(D3DFORMAT format)
{
    switch (format)
    {
        case D3DFMT_R8G8B8:        return D3DX_PIXEL_FORMAT_B8G8R8_UNORM;
        case D3DFMT_A8R8G8B8:      return D3DX_PIXEL_FORMAT_B8G8R8A8_UNORM;
        case D3DFMT_X8R8G8B8:      return D3DX_PIXEL_FORMAT_B8G8R8X8_UNORM;
        case D3DFMT_A8B8G8R8:      return D3DX_PIXEL_FORMAT_R8G8B8A8_UNORM;
        case D3DFMT_X8B8G8R8:      return D3DX_PIXEL_FORMAT_R8G8B8X8_UNORM;
        case D3DFMT_R5G6B5:        return D3DX_PIXEL_FORMAT_B5G6R5_UNORM;
        case D3DFMT_X1R5G5B5:      return D3DX_PIXEL_FORMAT_B5G5R5X1_UNORM;
        case D3DFMT_A1R5G5B5:      return D3DX_PIXEL_FORMAT_B5G5R5A1_UNORM;
        case D3DFMT_R3G3B2:        return D3DX_PIXEL_FORMAT_B2G3R3_UNORM;
        case D3DFMT_A8R3G3B2:      return D3DX_PIXEL_FORMAT_B2G3R3A8_UNORM;
        case D3DFMT_A4R4G4B4:      return D3DX_PIXEL_FORMAT_B4G4R4A4_UNORM;
        case D3DFMT_X4R4G4B4:      return D3DX_PIXEL_FORMAT_B4G4R4X4_UNORM;
        case D3DFMT_A2R10G10B10:   return D3DX_PIXEL_FORMAT_B10G10R10A2_UNORM;
        case D3DFMT_A2B10G10R10:   return D3DX_PIXEL_FORMAT_R10G10B10A2_UNORM;
        case D3DFMT_A16B16G16R16:  return D3DX_PIXEL_FORMAT_R16G16B16A16_UNORM;
        case D3DFMT_G16R16:        return D3DX_PIXEL_FORMAT_R16G16_UNORM;
        case D3DFMT_A8:            return D3DX_PIXEL_FORMAT_A8_UNORM;
        case D3DFMT_A8L8:          return D3DX_PIXEL_FORMAT_L8A8_UNORM;
        case D3DFMT_A4L4:          return D3DX_PIXEL_FORMAT_L4A4_UNORM;
        case D3DFMT_L8:            return D3DX_PIXEL_FORMAT_L8_UNORM;
        case D3DFMT_L16:           return D3DX_PIXEL_FORMAT_L16_UNORM;
        case D3DFMT_DXT1:          return D3DX_PIXEL_FORMAT_DXT1_UNORM;
        case D3DFMT_DXT2:          return D3DX_PIXEL_FORMAT_DXT2_UNORM;
        case D3DFMT_DXT3:          return D3DX_PIXEL_FORMAT_DXT3_UNORM;
        case D3DFMT_DXT4:          return D3DX_PIXEL_FORMAT_DXT4_UNORM;
        case D3DFMT_DXT5:          return D3DX_PIXEL_FORMAT_DXT5_UNORM;
        case D3DFMT_R16F:          return D3DX_PIXEL_FORMAT_R16_FLOAT;
        case D3DFMT_G16R16F:       return D3DX_PIXEL_FORMAT_R16G16_FLOAT;
        case D3DFMT_A16B16G16R16F: return D3DX_PIXEL_FORMAT_R16G16B16A16_FLOAT;
        case D3DFMT_R32F:          return D3DX_PIXEL_FORMAT_R32_FLOAT;
        case D3DFMT_G32R32F:       return D3DX_PIXEL_FORMAT_R32G32_FLOAT;
        case D3DFMT_A32B32G32R32F: return D3DX_PIXEL_FORMAT_R32G32B32A32_FLOAT;
        case D3DFMT_P8:            return D3DX_PIXEL_FORMAT_P8_UINT;
        case D3DFMT_A8P8:          return D3DX_PIXEL_FORMAT_P8_UINT_A8_UNORM;
        case D3DFMT_Q8W8V8U8:      return D3DX_PIXEL_FORMAT_U8V8W8Q8_SNORM;
        case D3DFMT_V8U8:          return D3DX_PIXEL_FORMAT_U8V8_SNORM;
        case D3DFMT_V16U16:        return D3DX_PIXEL_FORMAT_U16V16_SNORM;
        case D3DFMT_X8L8V8U8:      return D3DX_PIXEL_FORMAT_U8V8_SNORM_L8X8_UNORM;
        case D3DFMT_A2W10V10U10:   return D3DX_PIXEL_FORMAT_U10V10W10_SNORM_A2_UNORM;
        case D3DFMT_Q16W16V16U16:  return D3DX_PIXEL_FORMAT_U16V16W16Q16_SNORM;
        case D3DFMT_R8G8_B8G8:     return D3DX_PIXEL_FORMAT_R8G8_B8G8_UNORM;
        case D3DFMT_G8R8_G8B8:     return D3DX_PIXEL_FORMAT_G8R8_G8B8_UNORM;
        case D3DFMT_UYVY:          return D3DX_PIXEL_FORMAT_UYVY;
        case D3DFMT_YUY2:          return D3DX_PIXEL_FORMAT_YUY2;
        default:
            FIXME("No d3dx_pixel_format_id for D3DFORMAT %s.\n", debugstr_fourcc(format));
            return D3DX_PIXEL_FORMAT_COUNT;
    }
}

/************************************************************
 * map_view_of_file
 *
 * Loads a file into buffer and stores the number of read bytes in length.
 *
 * PARAMS
 *   filename [I] name of the file to be loaded
 *   buffer   [O] pointer to destination buffer
 *   length   [O] size of the obtained data
 *
 * RETURNS
 *   Success: D3D_OK
 *   Failure:
 *     see error codes for CreateFileW, GetFileSize, CreateFileMapping and MapViewOfFile
 *
 * NOTES
 *   The caller must UnmapViewOfFile when it doesn't need the data anymore
 *
 */
HRESULT map_view_of_file(const WCHAR *filename, void **buffer, DWORD *length)
{
    HANDLE hfile, hmapping = NULL;

    hfile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(hfile == INVALID_HANDLE_VALUE) goto error;

    *length = GetFileSize(hfile, NULL);
    if(*length == INVALID_FILE_SIZE) goto error;

    hmapping = CreateFileMappingW(hfile, NULL, PAGE_READONLY, 0, 0, NULL);
    if(!hmapping) goto error;

    *buffer = MapViewOfFile(hmapping, FILE_MAP_READ, 0, 0, 0);
    if(*buffer == NULL) goto error;

    CloseHandle(hmapping);
    CloseHandle(hfile);

    return S_OK;

error:
    if (hmapping)
        CloseHandle(hmapping);
    if (hfile != INVALID_HANDLE_VALUE)
        CloseHandle(hfile);
    return HRESULT_FROM_WIN32(GetLastError());
}

/************************************************************
 * load_resource_into_memory
 *
 * Loads a resource into buffer and stores the number of
 * read bytes in length.
 *
 * PARAMS
 *   module  [I] handle to the module
 *   resinfo [I] handle to the resource's information block
 *   buffer  [O] pointer to destination buffer
 *   length  [O] size of the obtained data
 *
 * RETURNS
 *   Success: D3D_OK
 *   Failure:
 *     See error codes for SizeofResource, LoadResource and LockResource
 *
 * NOTES
 *   The memory doesn't need to be freed by the caller manually
 *
 */
HRESULT load_resource_into_memory(HMODULE module, HRSRC resinfo, void **buffer, DWORD *length)
{
    HGLOBAL resource;

    *length = SizeofResource(module, resinfo);
    if(*length == 0) return HRESULT_FROM_WIN32(GetLastError());

    resource = LoadResource(module, resinfo);
    if( !resource ) return HRESULT_FROM_WIN32(GetLastError());

    *buffer = LockResource(resource);
    if(*buffer == NULL) return HRESULT_FROM_WIN32(GetLastError());

    return S_OK;
}

HRESULT write_buffer_to_file(const WCHAR *dst_filename, ID3DXBuffer *buffer)
{
    HRESULT hr = S_OK;
    void *buffer_pointer;
    DWORD buffer_size;
    DWORD bytes_written;
    HANDLE file = CreateFileW(dst_filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32(GetLastError());

    buffer_pointer = ID3DXBuffer_GetBufferPointer(buffer);
    buffer_size = ID3DXBuffer_GetBufferSize(buffer);

    if (!WriteFile(file, buffer_pointer, buffer_size, &bytes_written, NULL))
        hr = HRESULT_FROM_WIN32(GetLastError());

    CloseHandle(file);
    return hr;
}

const struct pixel_format_desc *get_d3dx_pixel_format_info(enum d3dx_pixel_format_id format)
{
    return &formats[min(format, D3DX_PIXEL_FORMAT_COUNT)];
}

/************************************************************
 * get_format_info
 *
 * Returns information about the specified format.
 * If the format is unsupported, it's filled with the D3DX_PIXEL_FORMAT_COUNT desc.
 *
 * PARAMS
 *   format [I] format whose description is queried
 *
 */
const struct pixel_format_desc *get_format_info(D3DFORMAT format)
{
    const struct pixel_format_desc *fmt_desc = &formats[d3dx_pixel_format_id_from_d3dformat(format)];

    if (is_unknown_format(fmt_desc))
        FIXME("Unknown format %s.\n", debugstr_fourcc(format));
    return fmt_desc;
}

const struct pixel_format_desc *get_format_info_idx(int idx)
{
    return idx < D3DX_PIXEL_FORMAT_COUNT ? &formats[idx] : NULL;
}

#define WINE_D3DX_TO_STR(x) case x: return #x

const char *debug_d3dxparameter_class(D3DXPARAMETER_CLASS c)
{
    switch (c)
    {
        WINE_D3DX_TO_STR(D3DXPC_SCALAR);
        WINE_D3DX_TO_STR(D3DXPC_VECTOR);
        WINE_D3DX_TO_STR(D3DXPC_MATRIX_ROWS);
        WINE_D3DX_TO_STR(D3DXPC_MATRIX_COLUMNS);
        WINE_D3DX_TO_STR(D3DXPC_OBJECT);
        WINE_D3DX_TO_STR(D3DXPC_STRUCT);
        default:
            FIXME("Unrecognized D3DXPARAMETER_CLASS %#x.\n", c);
            return "unrecognized";
    }
}

const char *debug_d3dxparameter_type(D3DXPARAMETER_TYPE t)
{
    switch (t)
    {
        WINE_D3DX_TO_STR(D3DXPT_VOID);
        WINE_D3DX_TO_STR(D3DXPT_BOOL);
        WINE_D3DX_TO_STR(D3DXPT_INT);
        WINE_D3DX_TO_STR(D3DXPT_FLOAT);
        WINE_D3DX_TO_STR(D3DXPT_STRING);
        WINE_D3DX_TO_STR(D3DXPT_TEXTURE);
        WINE_D3DX_TO_STR(D3DXPT_TEXTURE1D);
        WINE_D3DX_TO_STR(D3DXPT_TEXTURE2D);
        WINE_D3DX_TO_STR(D3DXPT_TEXTURE3D);
        WINE_D3DX_TO_STR(D3DXPT_TEXTURECUBE);
        WINE_D3DX_TO_STR(D3DXPT_SAMPLER);
        WINE_D3DX_TO_STR(D3DXPT_SAMPLER1D);
        WINE_D3DX_TO_STR(D3DXPT_SAMPLER2D);
        WINE_D3DX_TO_STR(D3DXPT_SAMPLER3D);
        WINE_D3DX_TO_STR(D3DXPT_SAMPLERCUBE);
        WINE_D3DX_TO_STR(D3DXPT_PIXELSHADER);
        WINE_D3DX_TO_STR(D3DXPT_VERTEXSHADER);
        WINE_D3DX_TO_STR(D3DXPT_PIXELFRAGMENT);
        WINE_D3DX_TO_STR(D3DXPT_VERTEXFRAGMENT);
        WINE_D3DX_TO_STR(D3DXPT_UNSUPPORTED);
        default:
            FIXME("Unrecognized D3DXPARAMETER_TYP %#x.\n", t);
            return "unrecognized";
    }
}

const char *debug_d3dxparameter_registerset(D3DXREGISTER_SET r)
{
    switch (r)
    {
        WINE_D3DX_TO_STR(D3DXRS_BOOL);
        WINE_D3DX_TO_STR(D3DXRS_INT4);
        WINE_D3DX_TO_STR(D3DXRS_FLOAT4);
        WINE_D3DX_TO_STR(D3DXRS_SAMPLER);
        default:
            FIXME("Unrecognized D3DXREGISTER_SET %#x.\n", r);
            return "unrecognized";
    }
}

#undef WINE_D3DX_TO_STR

/***********************************************************************
 * D3DXDebugMute
 * Returns always FALSE for us.
 */
BOOL WINAPI D3DXDebugMute(BOOL mute)
{
    return FALSE;
}

/***********************************************************************
 * D3DXGetDriverLevel.
 * Returns always 900 (DX 9) for us
 */
UINT WINAPI D3DXGetDriverLevel(struct IDirect3DDevice9 *device)
{
    return 900;
}
