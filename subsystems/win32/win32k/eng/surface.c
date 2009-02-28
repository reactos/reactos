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
SURFACE_Cleanup(PVOID ObjectBody)
{
    PSURFACE psurf = (PSURFACE)ObjectBody;
    PVOID pvBits = psurf->SurfObj.pvBits;

    /* If this is an API bitmap, free the bits */
    if (pvBits != NULL &&
        (psurf->flFlags & BITMAPOBJ_IS_APIBITMAP))
    {
        /* Check if we have a DIB section */
        if (psurf->hSecure)
        {
            // FIXME: IMPLEMENT ME!
            // MmUnsecureVirtualMemory(psurf->hSecure);
            if (psurf->hDIBSection)
            {
                /* DIB was created from a section */
                NTSTATUS Status;

                pvBits = (PVOID)((ULONG_PTR)pvBits - psurf->dwOffset);
                Status = ZwUnmapViewOfSection(NtCurrentProcess(), pvBits);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("Could not unmap section view!\n");
                    // Should we BugCheck here?
                }
            }
            else
            {
                /* DIB was allocated */
                EngFreeUserMem(pvBits);
            }
        }
        else
        {
            // FIXME: use TAG
            ExFreePool(psurf->SurfObj.pvBits);
        }

        if (psurf->hDIBPalette != NULL)
        {
            NtGdiDeleteObject(psurf->hDIBPalette);
        }
    }

    if (NULL != psurf->BitsLock)
    {
        ExFreePoolWithTag(psurf->BitsLock, TAG_SURFACE);
        psurf->BitsLock = NULL;
    }

    return TRUE;
}

BOOL INTERNAL_CALL
SURFACE_InitBitsLock(PSURFACE psurf)
{
    psurf->BitsLock = ExAllocatePoolWithTag(NonPagedPool,
                          sizeof(FAST_MUTEX),
                          TAG_SURFACE);
    if (NULL == psurf->BitsLock)
    {
        return FALSE;
    }

    ExInitializeFastMutex(psurf->BitsLock);

    return TRUE;
}

void INTERNAL_CALL
SURFACE_CleanupBitsLock(PSURFACE psurf)
{
    if (NULL != psurf->BitsLock)
    {
        ExFreePoolWithTag(psurf->BitsLock, TAG_SURFACE);
        psurf->BitsLock = NULL;
    }
}


/*
 * @implemented
 */
HBITMAP APIENTRY
EngCreateDeviceBitmap(IN DHSURF dhsurf,
                      IN SIZEL Size,
                      IN ULONG Format)
{
    HBITMAP NewBitmap;
    SURFOBJ *pso;

    NewBitmap = EngCreateBitmap(Size, DIB_GetDIBWidthBytes(Size.cx, BitsPerFormat(Format)), Format, 0, NULL);
    if (!NewBitmap)
    {
        DPRINT1("EngCreateBitmap failed\n");
        return 0;
    }

    pso = EngLockSurface((HSURF)NewBitmap);
    pso->dhsurf = dhsurf;
    EngUnlockSurface(pso);

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
    HBITMAP hbmp;
    SURFOBJ *pso;
    PSURFACE psurf;
    PVOID UncompressedBits;
    ULONG UncompressedFormat;

    if (Format == 0)
        return 0;

    psurf = SURFACE_AllocSurfaceWithHandle();
    if (psurf == NULL)
    {
        return 0;
    }
    hbmp = psurf->BaseObject.hHmgr;

    if (! SURFACE_InitBitsLock(psurf))
    {
        SURFACE_UnlockSurface(psurf);
        SURFACE_FreeSurfaceByHandle(hbmp);
        return 0;
    }
    pso = &psurf->SurfObj;

    if (Format == BMF_4RLE)
    {
        pso->lDelta = DIB_GetDIBWidthBytes(Size.cx, BitsPerFormat(BMF_4BPP));
        pso->cjBits = pso->lDelta * Size.cy;
        UncompressedFormat = BMF_4BPP;
        UncompressedBits = EngAllocMem(FL_ZERO_MEMORY, pso->cjBits, TAG_DIB);
        Decompress4bpp(Size, (BYTE *)Bits, (BYTE *)UncompressedBits, pso->lDelta);
    }
    else if (Format == BMF_8RLE)
    {
        pso->lDelta = DIB_GetDIBWidthBytes(Size.cx, BitsPerFormat(BMF_8BPP));
        pso->cjBits = pso->lDelta * Size.cy;
        UncompressedFormat = BMF_8BPP;
        UncompressedBits = EngAllocMem(FL_ZERO_MEMORY, pso->cjBits, TAG_DIB);
        Decompress8bpp(Size, (BYTE *)Bits, (BYTE *)UncompressedBits, pso->lDelta);
    }
    else
    {
        pso->lDelta = abs(Width);
        pso->cjBits = pso->lDelta * Size.cy;
        UncompressedBits = Bits;
        UncompressedFormat = Format;
    }

    if (UncompressedBits != NULL)
    {
        pso->pvBits = UncompressedBits;
    }
    else
    {
        if (pso->cjBits == 0)
        {
            pso->pvBits = NULL;
        }
        else
        {
            if (0 != (Flags & BMF_USERMEM))
            {
                pso->pvBits = EngAllocUserMem(pso->cjBits, 0);
            }
            else
            {
                pso->pvBits = EngAllocMem(0 != (Flags & BMF_NOZEROINIT) ?
                                                  0 : FL_ZERO_MEMORY,
                                              pso->cjBits, TAG_DIB);
            }
            if (pso->pvBits == NULL)
            {
                SURFACE_UnlockSurface(psurf);
                SURFACE_FreeSurfaceByHandle(hbmp);
                SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
                return 0;
            }
        }
    }

    if (0 == (Flags & BMF_TOPDOWN))
    {
        pso->pvScan0 = (PVOID)((ULONG_PTR)pso->pvBits + pso->cjBits - pso->lDelta);
        pso->lDelta = - pso->lDelta;
    }
    else
    {
        pso->pvScan0 = pso->pvBits;
    }

    pso->dhsurf = 0; /* device managed surface */
    pso->hsurf = (HSURF)hbmp;
    pso->dhpdev = NULL;
    pso->hdev = NULL;
    pso->sizlBitmap = Size;
    pso->iBitmapFormat = UncompressedFormat;
    pso->iType = STYPE_BITMAP;
    pso->fjBitmap = Flags & (BMF_TOPDOWN | BMF_NOZEROINIT);
    pso->iUniq = 0;

    psurf->flHooks = 0;
    psurf->flFlags = 0;
    psurf->dimension.cx = 0;
    psurf->dimension.cy = 0;
    
    psurf->hSecure = NULL;
    psurf->hDIBSection = NULL;

    SURFACE_UnlockSurface(psurf);

    return hbmp;
}

/*
 * @implemented
 */
HBITMAP APIENTRY
EngCreateBitmap(IN SIZEL Size,
                IN LONG Width,
                IN ULONG Format,
                IN ULONG Flags,
                IN PVOID Bits)
{
    HBITMAP hNewBitmap;

    hNewBitmap = IntCreateBitmap(Size, Width, Format, Flags, Bits);
    if ( !hNewBitmap )
        return 0;

    GDIOBJ_SetOwnership(hNewBitmap, NULL);

    return hNewBitmap;
}

/*
 * @unimplemented
 */
HSURF APIENTRY
EngCreateDeviceSurface(IN DHSURF dhsurf,
                       IN SIZEL Size,
                       IN ULONG Format)
{
    HSURF hsurf;
    SURFOBJ *pso;
    PSURFACE psurf;

    psurf = SURFACE_AllocSurfaceWithHandle();
    if (!psurf)
    {
        return 0;
    }

    hsurf = psurf->BaseObject.hHmgr;
    GDIOBJ_SetOwnership(hsurf, NULL);

    if (!SURFACE_InitBitsLock(psurf))
    {
        SURFACE_UnlockSurface(psurf);
        SURFACE_FreeSurfaceByHandle(hsurf);
        return 0;
    }
    pso = &psurf->SurfObj;

    pso->dhsurf = dhsurf;
    pso->hsurf = hsurf;
    pso->sizlBitmap = Size;
    pso->iBitmapFormat = Format;
    pso->lDelta = DIB_GetDIBWidthBytes(Size.cx, BitsPerFormat(Format));
    pso->iType = STYPE_DEVICE;
    pso->iUniq = 0;

    psurf->flHooks = 0;

    SURFACE_UnlockSurface(psurf);

    return hsurf;
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
BOOL APIENTRY
EngAssociateSurface(IN HSURF hsurf,
                    IN HDEV Dev,
                    IN ULONG Hooks)
{
    SURFOBJ *pso;
    PSURFACE psurf;
    GDIDEVICE* Device;

    Device = (GDIDEVICE*)Dev;

    psurf = SURFACE_LockSurface(hsurf);
    ASSERT(psurf);
    pso = &psurf->SurfObj;

    /* Associate the hdev */
    pso->hdev = Dev;
    pso->dhpdev = Device->hPDev;

    /* Hook up specified functions */
    psurf->flHooks = Hooks;

    SURFACE_UnlockSurface(psurf);

    return TRUE;
}

/*
 * @implemented
 */
BOOL APIENTRY
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
BOOL APIENTRY
EngDeleteSurface(IN HSURF hsurf)
{
    GDIOBJ_SetOwnership(hsurf, PsGetCurrentProcess());
    SURFACE_FreeSurfaceByHandle(hsurf);
    return TRUE;
}

/*
 * @implemented
 */
BOOL APIENTRY
EngEraseSurface(SURFOBJ *pso,
                RECTL *Rect,
                ULONG iColor)
{
    ASSERT(pso);
    ASSERT(Rect);
    return FillSolid(pso, Rect, iColor);
}

#define GDIBdyToHdr(body)                                                      \
  ((PGDIOBJHDR)(body) - 1)


/*
 * @implemented
 */
SURFOBJ * APIENTRY
NtGdiEngLockSurface(IN HSURF hsurf)
{
    return EngLockSurface(hsurf);
}


/*
 * @implemented
 */
SURFOBJ * APIENTRY
EngLockSurface(IN HSURF hsurf)
{
    SURFACE *psurf = GDIOBJ_ShareLockObj(hsurf, GDI_OBJECT_TYPE_BITMAP);

    if (psurf != NULL)
        return &psurf->SurfObj;

    return NULL;
}


/*
 * @implemented
 */
VOID APIENTRY
NtGdiEngUnlockSurface(IN SURFOBJ *pso)
{
    EngUnlockSurface(pso);
}

/*
 * @implemented
 */
VOID APIENTRY
EngUnlockSurface(IN SURFOBJ *pso)
{
    if (pso != NULL)
    {
        SURFACE *psurf = CONTAINING_RECORD(pso, SURFACE, SurfObj);
        GDIOBJ_ShareUnlockObjByPtr((POBJ)psurf);
    }
}


/* EOF */
