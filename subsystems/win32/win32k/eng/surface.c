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
            GreDeleteObject(psurf->hDIBPalette);
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
    if (!pso)
    {
        DPRINT1("EngLockSurface failed on newly created bitmap!\n");
        GreDeleteObject(NewBitmap);
        return NULL;
    }

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

/* Name gleaned from C++ symbol information for SURFMEM::bInitDIB */
typedef struct _DEVBITMAPINFO
{
    ULONG Format;
    ULONG Width;
    ULONG Height;
    ULONG Flags;
    ULONG Size;
} DEVBITMAPINFO, *PDEVBITMAPINFO;

SURFOBJ*
FASTCALL
SURFMEM_bCreateDib(IN PDEVBITMAPINFO BitmapInfo,
                   IN PVOID Bits)
{
    BOOLEAN Compressed = FALSE;
    ULONG ScanLine = 0; // Compiler is dumb
    ULONG Size;
    SURFOBJ *pso;
    PSURFACE psurf;
    SIZEL LocalSize;
    BOOLEAN AllocatedLocally = FALSE;

    /*
     * First, check the format so we can get the aligned scanline width.
     * RLE and the newer fancy-smanshy JPG/PNG support do NOT have scanlines
     * since they are compressed surfaces!
     */
    switch (BitmapInfo->Format)
    {
        case BMF_1BPP:
            ScanLine = ((BitmapInfo->Width + 31) & ~31) >> 3;
            break;

        case BMF_4BPP:
            ScanLine = ((BitmapInfo->Width + 7) & ~7) >> 1;
            break;

        case BMF_8BPP:
            ScanLine = (BitmapInfo->Width + 3) & ~3;
            break;

        case BMF_16BPP:
            ScanLine = ((BitmapInfo->Width + 1) & ~1) << 1;
            break;

        case BMF_24BPP:
            ScanLine = ((BitmapInfo->Width * 3) + 3) & ~3;
            break;

        case BMF_32BPP:
            ScanLine = BitmapInfo->Width << 2;
            break;

        case BMF_8RLE:
        case BMF_4RLE:
        case BMF_JPEG:
        case BMF_PNG:
            Compressed = TRUE;
            break;

        default:
            DPRINT1("Invalid bitmap format\n");
            return NULL;
    }

    /* Does the device manage its own surface? */
    if (!Bits)
    {
        /* We need to allocate bits for the caller, figure out the size */
        if (Compressed)
        {
            /* Note: we should not be seeing this scenario from ENGDDI */
            ASSERT(FALSE);
            Size = BitmapInfo->Size;
        }
        else
        {
            /* The height times the bytes for each scanline */
            Size = BitmapInfo->Height * ScanLine;
        }
        
        if (Size)
        {
            /* Check for allocation flag */
            if (BitmapInfo->Flags & BMF_USERMEM)
            {
                /* Get the bits from user-mode memory */
                Bits = EngAllocUserMem(Size, 'mbuG');
            }
            else
            {
                /* Get kernel bits (zeroed out if requested) */
                Bits = EngAllocMem((BitmapInfo->Flags & BMF_NOZEROINIT) ? 0 : FL_ZERO_MEMORY,
                                   Size,
                                   TAG_DIB);
            }
            AllocatedLocally = TRUE;
            /* Bail out if that failed */
            if (!Bits) return NULL;
        }
    }
    else
    {
        /* Should not have asked for user memory */
        ASSERT((BitmapInfo->Flags & BMF_USERMEM) == 0);
    }

    /* Allocate the actual surface object structure */
    psurf = SURFACE_AllocSurfaceWithHandle();
    if (!psurf)
    {
        if(Bits && AllocatedLocally)
        {
            if(BitmapInfo->Flags & BMF_USERMEM)
                EngFreeUserMem(Bits);
            else
                EngFreeMem(Bits);
        }
        return NULL;
    }

    /* Lock down the surface */
    if (!SURFACE_InitBitsLock(psurf))
    {
        /* Bail out if that failed */
        SURFACE_UnlockSurface(psurf);
        SURFACE_FreeSurfaceByHandle(psurf->BaseObject.hHmgr);
        return NULL;
    }

    /* We should now have our surface object */
    pso = &psurf->SurfObj;

    /* Save format and flags */
    pso->iBitmapFormat = BitmapInfo->Format;
    pso->fjBitmap = BitmapInfo->Flags & (BMF_TOPDOWN | BMF_UMPDMEM | BMF_USERMEM);

    /* Save size and type */
    LocalSize.cy = BitmapInfo->Height;
    LocalSize.cx = BitmapInfo->Width;
    pso->sizlBitmap = LocalSize;
    pso->iType = STYPE_BITMAP;
    
    /* Device-managed surface, no flags or dimension */
    pso->dhsurf = 0;
    pso->dhpdev = NULL;
    pso->hdev = NULL;
    psurf->flFlags = 0;
    psurf->dimension.cx = 0;
    psurf->dimension.cy = 0;
    psurf->hSecure = NULL;
    psurf->hDIBSection = NULL;
    psurf->flHooks = 0;
    
    /* Set bits */
    pso->pvBits = Bits;
    
    /* Check for bitmap type */
    if (!Compressed)
    {
        /* Number of bits is based on the height times the scanline */
        pso->cjBits = BitmapInfo->Height * ScanLine;
        if (BitmapInfo->Flags & BMF_TOPDOWN)
        {
            /* For topdown, the base address starts with the bits */
            pso->pvScan0 = pso->pvBits;
            pso->lDelta = ScanLine;
        }
        else
        {
            /* Otherwise we start with the end and go up */
            pso->pvScan0 = (PVOID)((ULONG_PTR)pso->pvBits + pso->cjBits - ScanLine);
            pso->lDelta = -ScanLine;
        }
    }
    else
    {
        /* Compressed surfaces don't have scanlines! */
        ASSERT(FALSE); // Should not get here on ENGDDI
        pso->lDelta = 0;
        pso->cjBits = BitmapInfo->Size;
        
        /* Check for JPG or PNG */
        if ((BitmapInfo->Format != BMF_JPEG) && (BitmapInfo->Format != BMF_PNG))
        {
            /* Wherever the bit data is */
            pso->pvScan0 = pso->pvBits;
        }
        else
        {
            /* Fancy formats don't use a base address */
            pso->pvScan0 = NULL;
            ASSERT(FALSE); // ENGDDI shouldn't be creating PNGs for drivers ;-)
        }
    }
    
    /* Finally set the handle and uniq */
    pso->hsurf = (HSURF)psurf->BaseObject.hHmgr;
    pso->iUniq = 0;
    
    /* Unlock and return the surface */
    SURFACE_UnlockSurface(psurf);
    return pso;
}

/*
 * @implemented
 */
HBITMAP
APIENTRY
EngCreateBitmap(IN SIZEL Size,
                IN LONG Width,
                IN ULONG Format,
                IN ULONG Flags,
                IN PVOID Bits)
{
    SURFOBJ* Surface;
    DEVBITMAPINFO BitmapInfo;
    
    /* Capture the parameters */
    BitmapInfo.Format = Format;
    BitmapInfo.Width = Size.cx;
    BitmapInfo.Height = Size.cy;
    BitmapInfo.Flags = Flags;

    /*
     * If the display driver supports framebuffer access, use the scanline width
     * to determine the actual width of the bitmap, and convert it to pels instead
     * of bytes.
     */
    if ((Bits) && (Width))
    {
        switch (BitmapInfo.Format)
        {
            /* Do the conversion for each bit depth we support */
            case BMF_1BPP:
                BitmapInfo.Width = Width * 8;
                break;
            case BMF_4BPP:
                BitmapInfo.Width = Width * 2;
                break;
            case BMF_8BPP:
                BitmapInfo.Width = Width;
                break;
            case BMF_16BPP:
                BitmapInfo.Width = Width / 2;
                break;
            case BMF_24BPP:
                BitmapInfo.Width = Width / 3;
                break;
            case BMF_32BPP:
                BitmapInfo.Width = Width / 4;
                break;
        }
    }
    
    /* Now create the surface */
    Surface = SURFMEM_bCreateDib(&BitmapInfo, Bits);
    if (!Surface) return 0;

    /* Set public ownership and reutrn the handle */
    GDIOBJ_SetOwnership(Surface->hsurf, NULL);
    return Surface->hsurf;
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

/*
 * @implemented
 */
BOOL
APIENTRY
EngAssociateSurface(
    IN HSURF hsurf,
    IN HDEV hdev,
    IN FLONG flHooks)
{
    SURFOBJ *pso;
    PSURFACE psurf;
    PDEVOBJ* ppdev;

    ppdev = (PDEVOBJ*)hdev;

    /* Lock the surface */
    psurf = SURFACE_LockSurface(hsurf);
    if (!psurf)
    {
        return FALSE;
    }
    pso = &psurf->SurfObj;

    /* Associate the hdev */
    pso->hdev = hdev;
    pso->dhpdev = ppdev->dhpdev;

    /* Hook up specified functions */
    psurf->flHooks = flHooks;

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
    PSURFACE psurf;
    PDEVOBJ* ppdev;

    psurf = SURFACE_LockSurface(hsurf);
    if (psurf == NULL)
    {
        return FALSE;
    }

    ppdev = (PDEVOBJ*)hdev;
    pso = &psurf->SurfObj;
    pso->dhsurf = dhsurf;
    pso->lDelta = lDelta;
    pso->pvScan0 = pvScan0;

    /* Associate the hdev */
    pso->hdev = hdev;
    pso->dhpdev = ppdev->dhpdev;

    /* Hook up specified functions */
    psurf->flHooks = flHooks;

    SURFACE_UnlockSurface(psurf);

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
