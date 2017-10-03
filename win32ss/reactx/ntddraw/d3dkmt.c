/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native DirectDraw implementation
 * FILE:             win32ss/reactx/ntddraw/d3dkmt.c
 * PROGRAMER:        Sebastian Gasiorek (sebastian.gasiorek@reactos.com)
 */

#include <win32k.h>
#include <debug.h>

DWORD
APIENTRY
NtGdiDdDDICreateDCFromMemory(D3DKMT_CREATEDCFROMMEMORY *desc)
{
    PSURFACE psurf;
    HDC hDC;

    const struct d3dddi_format_info
    {
        D3DDDIFORMAT format;
        unsigned int bit_count;
        DWORD compression;
        unsigned int palette_size;
        DWORD mask_r, mask_g, mask_b;
    } *format = NULL;
    unsigned int i;

    static const struct d3dddi_format_info format_info[] =
    {
        { D3DDDIFMT_R8G8B8,   24, BI_RGB,       0,   0x00000000, 0x00000000, 0x00000000 },
        { D3DDDIFMT_A8R8G8B8, 32, BI_RGB,       0,   0x00000000, 0x00000000, 0x00000000 },
        { D3DDDIFMT_X8R8G8B8, 32, BI_RGB,       0,   0x00000000, 0x00000000, 0x00000000 },
        { D3DDDIFMT_R5G6B5,   16, BI_BITFIELDS, 0,   0x0000f800, 0x000007e0, 0x0000001f },
        { D3DDDIFMT_X1R5G5B5, 16, BI_BITFIELDS, 0,   0x00007c00, 0x000003e0, 0x0000001f },
        { D3DDDIFMT_A1R5G5B5, 16, BI_BITFIELDS, 0,   0x00007c00, 0x000003e0, 0x0000001f },
        { D3DDDIFMT_A4R4G4B4, 16, BI_BITFIELDS, 0,   0x00000f00, 0x000000f0, 0x0000000f },
        { D3DDDIFMT_X4R4G4B4, 16, BI_BITFIELDS, 0,   0x00000f00, 0x000000f0, 0x0000000f },
        { D3DDDIFMT_P8,       8,  BI_RGB,       256, 0x00000000, 0x00000000, 0x00000000 },
    };

    if (!desc) 
        return STATUS_INVALID_PARAMETER;

    if (!desc->pMemory) 
        return STATUS_INVALID_PARAMETER;

    for (i = 0; i < sizeof(format_info) / sizeof(*format_info); ++i)
    {
        if (format_info[i].format == desc->Format)
        {
            format = &format_info[i];
            break;
        }
    }

    if (!format) 
        return STATUS_INVALID_PARAMETER;

    if (desc->Width > (UINT_MAX & ~3) / (format->bit_count / 8) ||
        !desc->Pitch || desc->Pitch < (((desc->Width * format->bit_count + 31) >> 3) & ~3) ||
        !desc->Height || desc->Height > UINT_MAX / desc->Pitch)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (!desc->hDeviceDc || !(hDC = NtGdiCreateCompatibleDC(desc->hDeviceDc)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Allocate a surface */
    psurf = SURFACE_AllocSurface(STYPE_BITMAP,
                                 desc->Width,
                                 desc->Height,
                                 BitmapFormat(format->bit_count, format->compression),
                                 BMF_TOPDOWN | BMF_NOZEROINIT,
                                 desc->Pitch,
                                 0,
                                 desc->pMemory);

    /* Mark as API bitmap */
    psurf->flags |= (DDB_SURFACE | API_BITMAP);

    desc->hDc = hDC;
    /* Get the handle for the bitmap */
    desc->hBitmap = (HBITMAP)psurf->SurfObj.hsurf;

    /* Unlock the surface and return */
    SURFACE_UnlockSurface(psurf);

    NtGdiSelectBitmap(desc->hDc, desc->hBitmap);

    return STATUS_SUCCESS;
}

DWORD
APIENTRY
NtGdiDdDDIDestroyDCFromMemory(const D3DKMT_DESTROYDCFROMMEMORY *desc)
{
    if (!desc) 
        return STATUS_INVALID_PARAMETER;

    if (GDI_HANDLE_GET_TYPE(desc->hDc)  != GDI_OBJECT_TYPE_DC ||
        GDI_HANDLE_GET_TYPE(desc->hBitmap) != GDI_OBJECT_TYPE_BITMAP) 
        return STATUS_INVALID_PARAMETER;

    NtGdiDeleteObjectApp(desc->hBitmap);
    NtGdiDeleteObjectApp(desc->hDc);

    return STATUS_SUCCESS;
}
