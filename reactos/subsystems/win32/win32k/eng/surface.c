/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Surace Functions
 * FILE:              subsys/win32k/eng/surface.c
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 3/7/1999: Created
 *                 9/11/2000: Updated to handle real pixel packed bitmaps (UPDATE TO DATE COMPLETED)
 * TESTING TO BE DONE:
 * - Create a GDI bitmap with all formats, perform all drawing operations on them, render to VGA surface
 *   refer to \test\microwin\src\engine\devdraw.c for info on correct pixel plotting for various formats
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

enum Rle_EscapeCodes
{
    RLE_EOL   = 0, /* End of line */
    RLE_END   = 1, /* End of bitmap */
    RLE_DELTA = 2  /* Delta */
};

INT FASTCALL BitsPerFormat(ULONG Format)
{
    switch (Format)
    {
        case BMF_1BPP:
            return 1;

        case BMF_4BPP:
            /* Fall through */
        case BMF_4RLE:
            return 4;

        case BMF_8BPP:
            /* Fall through */
        case BMF_8RLE:
            return 8;

        case BMF_16BPP:
            return 16;

        case BMF_24BPP:
            return 24;

        case BMF_32BPP:
            return 32;

        default:
            return 0;
    }
}

ULONG FASTCALL BitmapFormat(WORD Bits, DWORD Compression)
{
    switch (Compression)
    {
        case BI_RGB:
            /* Fall through */
        case BI_BITFIELDS:
            switch (Bits)
            {
                case 1:
                    return BMF_1BPP;
                case 4:
                    return BMF_4BPP;
                case 8:
                    return BMF_8BPP;
                case 16:
                    return BMF_16BPP;
                case 24:
                    return BMF_24BPP;
                case 32:
                    return BMF_32BPP;
            }
            return 0;

        case BI_RLE4:
            return BMF_4RLE;

        case BI_RLE8:
            return BMF_8RLE;

        default:
            return 0;
    }
}

BOOL INTERNAL_CALL
BITMAPOBJ_InitBitsLock(BITMAPOBJ *BitmapObj)
{
    BitmapObj->BitsLock = ExAllocatePoolWithTag(NonPagedPool,
                          sizeof(FAST_MUTEX),
                          TAG_BITMAPOBJ);
    if (NULL == BitmapObj->BitsLock)
    {
        return FALSE;
    }

    ExInitializeFastMutex(BitmapObj->BitsLock);

    return TRUE;
}

void INTERNAL_CALL
BITMAPOBJ_CleanupBitsLock(BITMAPOBJ *BitmapObj)
{
    if (NULL != BitmapObj->BitsLock)
    {
        ExFreePoolWithTag(BitmapObj->BitsLock, TAG_BITMAPOBJ);
        BitmapObj->BitsLock = NULL;
    }
}


/*
 * @implemented
 */
HBITMAP STDCALL
EngCreateDeviceBitmap(IN DHSURF dhsurf,
                      IN SIZEL Size,
                      IN ULONG Format)
{
    HBITMAP NewBitmap;
    SURFOBJ *SurfObj;

    NewBitmap = EngCreateBitmap(Size, DIB_GetDIBWidthBytes(Size.cx, BitsPerFormat(Format)), Format, 0, NULL);
    if (!NewBitmap)
    {
        DPRINT1("EngCreateBitmap failed\n");
        return 0;
    }

    SurfObj = EngLockSurface((HSURF)NewBitmap);
    SurfObj->dhsurf = dhsurf;
    EngUnlockSurface(SurfObj);

    return NewBitmap;
}

VOID Decompress4bpp(SIZEL Size, BYTE *CompressedBits, BYTE *UncompressedBits, LONG Delta)
{
    int x = 0;
    int y = Size.cy - 1;
    int c;
    int length;
    int width = ((Size.cx+1)/2);
    int height = Size.cy - 1;
    BYTE *begin = CompressedBits;
    BYTE *bits = CompressedBits;
    BYTE *temp;
    while (y >= 0)
    {
        length = *bits++ / 2;
        if (length)
        {
            c = *bits++;
            while (length--)
            {
                if (x >= width) break;
                temp = UncompressedBits + (((height - y) * Delta) + x);
                x++;
                *temp = c;
            }
        }
        else
        {
            length = *bits++;
            switch (length)
            {
                case RLE_EOL:
                    x = 0;
                    y--;
                    break;
                case RLE_END:
                    return;
                case RLE_DELTA:
                    x += (*bits++)/2;
                    y -= (*bits++)/2;
                    break;
                default:
                    length /= 2;
                    while (length--)
                    {
                        c = *bits++;
                        if (x < width)
                        {
                            temp = UncompressedBits + (((height - y) * Delta) + x);
                            x++;
                            *temp = c;
                        }
                    }
                    if ((bits - begin) & 1)
                    {
                        bits++;
                    }
            }
        }
    }
}

VOID Decompress8bpp(SIZEL Size, BYTE *CompressedBits, BYTE *UncompressedBits, LONG Delta)
{
    int x = 0;
    int y = Size.cy - 1;
    int c;
    int length;
    int width = Size.cx;
    int height = Size.cy - 1;
    BYTE *begin = CompressedBits;
    BYTE *bits = CompressedBits;
    BYTE *temp;
    while (y >= 0)
    {
        length = *bits++;
        if (length)
        {
            c = *bits++;
            while (length--)
            {
                if (x >= width) break;
                temp = UncompressedBits + (((height - y) * Delta) + x);
                x++;
                *temp = c;
            }
        }
        else
        {
            length = *bits++;
            switch (length)
            {
                case RLE_EOL:
                    x = 0;
                    y--;
                    break;
                case RLE_END:
                    return;
                case RLE_DELTA:
                    x += *bits++;
                    y -= *bits++;
                    break;
                default:
                    while (length--)
                    {
                        c = *bits++;
                        if (x < width)
                        {
                            temp = UncompressedBits + (((height - y) * Delta) + x);
                            x++;
                            *temp = c;
                        }
                    }
                    if ((bits - begin) & 1)
                    {
                        bits++;
                    }
            }
        }
    }
}

HBITMAP FASTCALL
IntCreateBitmap(IN SIZEL Size,
                IN LONG Width,
                IN ULONG Format,
                IN ULONG Flags,
                IN PVOID Bits)
{
    HBITMAP NewBitmap;
    SURFOBJ *SurfObj;
    BITMAPOBJ *BitmapObj;
    PVOID UncompressedBits;
    ULONG UncompressedFormat;

    if (Format == 0)
        return 0;

    BitmapObj = BITMAPOBJ_AllocBitmapWithHandle();
    if (BitmapObj == NULL)
    {
        return 0;
    }
    NewBitmap = BitmapObj->BaseObject.hHmgr;

    if (! BITMAPOBJ_InitBitsLock(BitmapObj))
    {
        BITMAPOBJ_UnlockBitmap(BitmapObj);
        BITMAPOBJ_FreeBitmapByHandle(NewBitmap);
        return 0;
    }
    SurfObj = &BitmapObj->SurfObj;

    if (Format == BMF_4RLE)
    {
        SurfObj->lDelta = DIB_GetDIBWidthBytes(Size.cx, BitsPerFormat(BMF_4BPP));
        SurfObj->cjBits = SurfObj->lDelta * Size.cy;
        UncompressedFormat = BMF_4BPP;
        UncompressedBits = EngAllocMem(FL_ZERO_MEMORY, SurfObj->cjBits, TAG_DIB);
        Decompress4bpp(Size, (BYTE *)Bits, (BYTE *)UncompressedBits, SurfObj->lDelta);
    }
    else if (Format == BMF_8RLE)
    {
        SurfObj->lDelta = DIB_GetDIBWidthBytes(Size.cx, BitsPerFormat(BMF_8BPP));
        SurfObj->cjBits = SurfObj->lDelta * Size.cy;
        UncompressedFormat = BMF_8BPP;
        UncompressedBits = EngAllocMem(FL_ZERO_MEMORY, SurfObj->cjBits, TAG_DIB);
        Decompress8bpp(Size, (BYTE *)Bits, (BYTE *)UncompressedBits, SurfObj->lDelta);
    }
    else
    {
        SurfObj->lDelta = abs(Width);
        SurfObj->cjBits = SurfObj->lDelta * Size.cy;
        UncompressedBits = Bits;
        UncompressedFormat = Format;
    }

    if (UncompressedBits != NULL)
    {
        SurfObj->pvBits = UncompressedBits;
    }
    else
    {
        if (SurfObj->cjBits == 0)
        {
            SurfObj->pvBits = NULL;
        }
        else
        {
            if (0 != (Flags & BMF_USERMEM))
            {
                SurfObj->pvBits = EngAllocUserMem(SurfObj->cjBits, 0);
            }
            else
            {
                SurfObj->pvBits = EngAllocMem(0 != (Flags & BMF_NOZEROINIT) ?
                                                  0 : FL_ZERO_MEMORY,
                                              SurfObj->cjBits, TAG_DIB);
            }
            if (SurfObj->pvBits == NULL)
            {
                BITMAPOBJ_UnlockBitmap(BitmapObj);
                BITMAPOBJ_FreeBitmapByHandle(NewBitmap);
                SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
                return 0;
            }
        }
    }

    if (0 == (Flags & BMF_TOPDOWN))
    {
        SurfObj->pvScan0 = (PVOID) ((ULONG_PTR) SurfObj->pvBits + SurfObj->cjBits - SurfObj->lDelta);
        SurfObj->lDelta = - SurfObj->lDelta;
    }
    else
    {
        SurfObj->pvScan0 = SurfObj->pvBits;
    }

    SurfObj->dhsurf = 0; /* device managed surface */
    SurfObj->hsurf = (HSURF)NewBitmap;
    SurfObj->dhpdev = NULL;
    SurfObj->hdev = NULL;
    SurfObj->sizlBitmap = Size;
    SurfObj->iBitmapFormat = UncompressedFormat;
    SurfObj->iType = STYPE_BITMAP;
    SurfObj->fjBitmap = Flags & (BMF_TOPDOWN | BMF_NOZEROINIT);
    SurfObj->iUniq = 0;

    BitmapObj->flHooks = 0;
    BitmapObj->flFlags = 0;
    BitmapObj->dimension.cx = 0;
    BitmapObj->dimension.cy = 0;
    BitmapObj->dib = NULL;

    BITMAPOBJ_UnlockBitmap(BitmapObj);

    return NewBitmap;
}

/*
 * @implemented
 */
HBITMAP STDCALL
EngCreateBitmap(IN SIZEL Size,
                IN LONG Width,
                IN ULONG Format,
                IN ULONG Flags,
                IN PVOID Bits)
{
    HBITMAP NewBitmap;

    NewBitmap = IntCreateBitmap(Size, Width, Format, Flags, Bits);
    if ( !NewBitmap )
        return 0;

    GDIOBJ_SetOwnership(NewBitmap, NULL);

    return NewBitmap;
}

/*
 * @unimplemented
 */
HSURF STDCALL
EngCreateDeviceSurface(IN DHSURF dhsurf,
                       IN SIZEL Size,
                       IN ULONG Format)
{
    HSURF NewSurface;
    SURFOBJ *SurfObj;
    BITMAPOBJ *BitmapObj;

    BitmapObj = BITMAPOBJ_AllocBitmapWithHandle();
    if (!BitmapObj)
    {
        return 0;
    }

    NewSurface = BitmapObj->BaseObject.hHmgr;
    GDIOBJ_SetOwnership(NewSurface, NULL);

    if (!BITMAPOBJ_InitBitsLock(BitmapObj))
    {
        BITMAPOBJ_UnlockBitmap(BitmapObj);
        BITMAPOBJ_FreeBitmapByHandle(NewSurface);
        return 0;
    }
    SurfObj = &BitmapObj->SurfObj;

    SurfObj->dhsurf = dhsurf;
    SurfObj->hsurf = NewSurface;
    SurfObj->sizlBitmap = Size;
    SurfObj->iBitmapFormat = Format;
    SurfObj->lDelta = DIB_GetDIBWidthBytes(Size.cx, BitsPerFormat(Format));
    SurfObj->iType = STYPE_DEVICE;
    SurfObj->iUniq = 0;

    BitmapObj->flHooks = 0;

    BITMAPOBJ_UnlockBitmap(BitmapObj);

    return NewSurface;
}

PFN FASTCALL DriverFunction(DRVENABLEDATA *DED, ULONG DriverFunc)
{
    ULONG i;

    for (i=0; i<DED->c; i++)
    {
        if (DED->pdrvfn[i].iFunc == DriverFunc)
        {
            return DED->pdrvfn[i].pfn;
        }
    }
    return NULL;
}

/*
 * @implemented
 */
BOOL STDCALL
EngAssociateSurface(IN HSURF Surface,
                    IN HDEV Dev,
                    IN ULONG Hooks)
{
    SURFOBJ *SurfObj;
    BITMAPOBJ *BitmapObj;
    GDIDEVICE* Device;

    Device = (GDIDEVICE*)Dev;

    BitmapObj = BITMAPOBJ_LockBitmap(Surface);
    ASSERT(BitmapObj);
    SurfObj = &BitmapObj->SurfObj;

    /* Associate the hdev */
    SurfObj->hdev = Dev;
    SurfObj->dhpdev = Device->hPDev;

    /* Hook up specified functions */
    BitmapObj->flHooks = Hooks;

    BITMAPOBJ_UnlockBitmap(BitmapObj);

    return TRUE;
}

/*
 * @implemented
 */
BOOL STDCALL
EngModifySurface(
    IN HSURF hsurf,
    IN HDEV hdev,
    IN FLONG flHooks,
    IN FLONG flSurface,
    IN DHSURF dhsurf,
    OUT VOID *pvScan0,
    IN LONG lDelta,
    IN VOID *pvReserved)
{
    SURFOBJ *pso;

    pso = EngLockSurface(hsurf);
    if (pso == NULL)
    {
        return FALSE;
    }

    if (!EngAssociateSurface(hsurf, hdev, flHooks))
    {
        EngUnlockSurface(pso);

        return FALSE;
    }

    pso->dhsurf = dhsurf;
    pso->lDelta = lDelta;
    pso->pvScan0 = pvScan0;

    EngUnlockSurface(pso);

    return TRUE;
}

/*
 * @implemented
 */
BOOL STDCALL
EngDeleteSurface(IN HSURF Surface)
{
    GDIOBJ_SetOwnership(Surface, PsGetCurrentProcess());
    BITMAPOBJ_FreeBitmapByHandle(Surface);
    return TRUE;
}

/*
 * @implemented
 */
BOOL STDCALL
EngEraseSurface(SURFOBJ *Surface,
                RECTL *Rect,
                ULONG iColor)
{
    ASSERT(Surface);
    ASSERT(Rect);
    return FillSolid(Surface, Rect, iColor);
}

#define GDIBdyToHdr(body)                                                      \
  ((PGDIOBJHDR)(body) - 1)


/*
 * @implemented
 */
SURFOBJ * STDCALL
NtGdiEngLockSurface(IN HSURF Surface)
{
    return EngLockSurface(Surface);
}


/*
 * @implemented
 */
SURFOBJ * STDCALL
EngLockSurface(IN HSURF Surface)
{
    BITMAPOBJ *bmp = GDIOBJ_ShareLockObj(Surface, GDI_OBJECT_TYPE_BITMAP);

    if (bmp != NULL)
        return &bmp->SurfObj;

    return NULL;
}


/*
 * @implemented
 */
VOID STDCALL
NtGdiEngUnlockSurface(IN SURFOBJ *Surface)
{
    EngUnlockSurface(Surface);
}

/*
 * @implemented
 */
VOID STDCALL
EngUnlockSurface(IN SURFOBJ *Surface)
{
    if (Surface != NULL)
    {
        BITMAPOBJ *bmp = CONTAINING_RECORD(Surface, BITMAPOBJ, SurfObj);
        GDIOBJ_ShareUnlockObjByPtr((POBJ)bmp);
    }
}


/* EOF */
