/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Surace Functions
 * FILE:              win32ss/gdi/eng/surface.c
 * PROGRAMERS:        Jason Filby
 *                    Timo Kreuzer
 * TESTING TO BE DONE:
 * - Create a GDI bitmap with all formats, perform all drawing operations on them, render to VGA surface
 *   refer to \test\microwin\src\engine\devdraw.c for info on correct pixel plotting for various formats
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

LONG giUniqueSurface = 0;

UCHAR
gajBitsPerFormat[11] =
{
    0, /*  0: unused */
    1, /*  1: BMF_1BPP */
    4, /*  2: BMF_4BPP */
    8, /*  3: BMF_8BPP */
   16, /*  4: BMF_16BPP */
   24, /*  5: BMF_24BPP */
   32, /*  6: BMF_32BPP */
    4, /*  7: BMF_4RLE */
    8, /*  8: BMF_8RLE */
    0, /*  9: BMF_JPEG */
    0, /* 10: BMF_PNG */
};


ULONG
FASTCALL
BitmapFormat(ULONG cBits, ULONG iCompression)
{
    switch (iCompression)
    {
        case BI_RGB:
            /* Fall through */
        case BI_BITFIELDS:
            if (cBits <= 1) return BMF_1BPP;
            if (cBits <= 4) return BMF_4BPP;
            if (cBits <= 8) return BMF_8BPP;
            if (cBits <= 16) return BMF_16BPP;
            if (cBits <= 24) return BMF_24BPP;
            if (cBits <= 32) return BMF_32BPP;
            return 0;

        case BI_RLE4:
            return BMF_4RLE;

        case BI_RLE8:
            return BMF_8RLE;

        default:
            return 0;
    }
}

VOID
NTAPI
SURFACE_vCleanup(PVOID ObjectBody)
{
    PSURFACE psurf = (PSURFACE)ObjectBody;
    PVOID pvBits = psurf->SurfObj.pvBits;

    /* Check if the surface has bits */
    if (pvBits)
    {
        /* Only bitmaps can have bits */
        ASSERT(psurf->SurfObj.iType == STYPE_BITMAP);

        /* Check if it is a DIB section */
        if (psurf->hDIBSection)
        {
            /* Unmap the section view */
            EngUnmapSectionView(pvBits, psurf->dwOffset, psurf->hSecure);
        }
        else if (psurf->SurfObj.fjBitmap & BMF_USERMEM)
        {
            /* Bitmap was allocated from usermode memory */
            EngFreeUserMem(pvBits);
        }
        else if (psurf->SurfObj.fjBitmap & BMF_KMSECTION)
        {
            /* Bitmap was allocated from a kernel section */
            if (!EngFreeSectionMem(NULL, pvBits))
            {
                DPRINT1("EngFreeSectionMem failed for %p!\n", pvBits);
                // Should we BugCheck here?
                ASSERT(FALSE);
            }
        }
        else if (psurf->SurfObj.fjBitmap & BMF_POOLALLOC)
        {
            /* Free a pool allocation */
            EngFreeMem(pvBits);
        }
    }

    /* Free palette */
    if(psurf->ppal)
    {
        PALETTE_ShareUnlockPalette(psurf->ppal);
    }
}


PSURFACE
NTAPI
SURFACE_AllocSurface(
    _In_ USHORT iType,
    _In_ ULONG cx,
    _In_ ULONG cy,
    _In_ ULONG iFormat,
    _In_ ULONG fjBitmap,
    _In_opt_ ULONG cjWidth,
    _In_opt_ ULONG cjBufSize,
    _In_opt_ PVOID pvBits)
{
    ULONG cBitsPixel, cjBits, cjObject;
    PSURFACE psurf;
    SURFOBJ *pso;
    PVOID pvSection;

    NT_ASSERT(!pvBits || (iType == STYPE_BITMAP));
    NT_ASSERT((iFormat <= BMF_32BPP) || (cjBufSize != 0));
    NT_ASSERT((LONG)cy > 0);

    /* Verify format */
    if ((iFormat < BMF_1BPP) || (iFormat > BMF_PNG))
    {
        DPRINT1("Invalid bitmap format: %lu\n", iFormat);
        return NULL;
    }

    /* Get bits per pixel from the format */
    cBitsPixel = gajBitsPerFormat[iFormat];

    /* Are bits and a width in bytes given? */
    if (pvBits && cjWidth)
    {
        /* Align the width (Windows compatibility, drivers expect that) */
        cjWidth = WIDTH_BYTES_ALIGN32((cjWidth << 3) / cBitsPixel, cBitsPixel);
    }
    else
    {
        /* Calculate width from the bitmap width in pixels */
        cjWidth = WIDTH_BYTES_ALIGN32(cx, cBitsPixel);
    }

    /* Is this an uncompressed format? */
    if (iFormat <= BMF_32BPP)
    {
        /* Calculate the correct bitmap size in bytes */
        if (!NT_SUCCESS(RtlULongMult(cjWidth, cy, &cjBits)))
        {
            DPRINT1("Overflow calculating size: cjWidth %lu, cy %lu\n",
                    cjWidth, cy);
            return NULL;
        }

        /* Did we get a buffer and size? */
        if ((pvBits != NULL) && (cjBufSize != 0))
        {
            /* Make sure the buffer is large enough */
            if (cjBufSize < cjBits)
            {
                DPRINT1("Buffer is too small, required: %lu, got %lu\n",
                        cjBits, cjBufSize);
                return NULL;
            }
        }
    }
    else
    {
        /* Compressed format, use the provided size */
        NT_ASSERT(cjBufSize != 0);
        cjBits = cjBufSize;
    }

    /* Check if we need an extra large object */
    if ((iType == STYPE_BITMAP) && (pvBits == NULL) &&
        !(fjBitmap & BMF_USERMEM) && !(fjBitmap & BMF_KMSECTION))
    {
        /* Allocate an object large enough to hold the bits */
        cjObject = sizeof(SURFACE) + cjBits;
    }
    else
    {
        /* Otherwise just allocate the SURFACE structure */
        cjObject = sizeof(SURFACE);
    }

    /* Check for arithmetic overflow */
    if (cjObject < sizeof(SURFACE))
    {
        /* Fail! */
        DPRINT1("Overflow calculating cjObject: cjBits %lu\n", cjBits);
        return NULL;
    }

    /* Allocate a SURFACE object */
    psurf = (PSURFACE)GDIOBJ_AllocObjWithHandle(GDI_OBJECT_TYPE_BITMAP, cjObject);
    if (!psurf)
    {
        return NULL;
    }

    /* Initialize the basic fields */
    pso = &psurf->SurfObj;
    pso->hsurf = psurf->BaseObject.hHmgr;
    pso->sizlBitmap.cx = cx;
    pso->sizlBitmap.cy = cy;
    pso->iBitmapFormat = iFormat;
    pso->iType = iType;
    pso->fjBitmap = (USHORT)fjBitmap;
    pso->iUniq = InterlockedIncrement(&giUniqueSurface);
    pso->cjBits = cjBits;

    /* Check if we need a bitmap buffer */
    if (iType == STYPE_BITMAP)
    {
        /* Check if we got one or if we need to allocate one */
        if (pvBits != NULL)
        {
            /* Use the caller provided buffer */
            pso->pvBits = pvBits;
        }
        else if (fjBitmap & BMF_USERMEM)
        {
            /* User mode memory was requested */
            pso->pvBits = EngAllocUserMem(cjBits, 0);

            /* Check for failure */
            if (!pso->pvBits)
            {
                GDIOBJ_vDeleteObject(&psurf->BaseObject);
                return NULL;
            }
        }
        else if (fjBitmap & BMF_KMSECTION)
        {
            /* Use a kernel mode section */
            pso->pvBits = EngAllocSectionMem(&pvSection,
                                             (fjBitmap & BMF_NOZEROINIT) ?
                                                 0 : FL_ZERO_MEMORY,
                                             cjBits, TAG_DIB);

            /* Check for failure */
            if (!pso->pvBits)
            {
                GDIOBJ_vDeleteObject(&psurf->BaseObject);
                return NULL;
            }

            /* Free the section already, but keep the mapping */
            EngFreeSectionMem(pvSection, NULL);
        }
        else
        {
            /* Buffer is after the object */
            pso->pvBits = psurf + 1;

            /* Zero the buffer, except requested otherwise */
            if (!(fjBitmap & BMF_NOZEROINIT))
            {
                RtlZeroMemory(pso->pvBits, cjBits);
            }
        }

        /* Set pvScan0 and lDelta */
        if (fjBitmap & BMF_TOPDOWN)
        {
            /* Topdown is the normal way */
            pso->pvScan0 = pso->pvBits;
            pso->lDelta = cjWidth;
        }
        else
        {
            /* Inversed bitmap (bottom up) */
            pso->pvScan0 = ((PCHAR)pso->pvBits + pso->cjBits - cjWidth);
            pso->lDelta = -(LONG)cjWidth;
        }
    }
    else
    {
        /* There are no bitmap bits */
        pso->pvScan0 = pso->pvBits = NULL;
        pso->lDelta = 0;
    }

    /* Assign a default palette and increment its reference count */
    SURFACE_vSetPalette(psurf, appalSurfaceDefault[iFormat]);

    return psurf;
}

HBITMAP
APIENTRY
EngCreateBitmap(
    _In_ SIZEL sizl,
    _In_ LONG lWidth,
    _In_ ULONG iFormat,
    _In_ ULONG fl,
    _In_opt_ PVOID pvBits)
{
    PSURFACE psurf;
    HBITMAP hbmp;

    /* Allocate a surface */
    psurf = SURFACE_AllocSurface(STYPE_BITMAP,
                                 sizl.cx,
                                 sizl.cy,
                                 iFormat,
                                 fl,
                                 lWidth,
                                 0,
                                 pvBits);
    if (!psurf)
    {
        DPRINT1("SURFACE_AllocSurface failed. (STYPE_BITMAP, sizl.cx=%ld, sizl.cy=%ld, iFormat=%lu, fl=%lu, lWidth=%ld, pvBits=0x%p)\n",
                sizl.cx, sizl.cy, iFormat, fl, lWidth, pvBits);
        return NULL;
    }

    /* Get the handle for the bitmap */
    hbmp = (HBITMAP)psurf->SurfObj.hsurf;

    /* Mark as API bitmap */
    psurf->flags = API_BITMAP;

    /* Set public ownership */
    GDIOBJ_vSetObjectOwner(&psurf->BaseObject, GDI_OBJ_HMGR_PUBLIC);

    /* Unlock the surface and return */
    SURFACE_UnlockSurface(psurf);
    return hbmp;
}

/*
 * @implemented
 */
HBITMAP
APIENTRY
EngCreateDeviceBitmap(
    _In_ DHSURF dhsurf,
    _In_ SIZEL sizl,
    _In_ ULONG iFormat)
{
    PSURFACE psurf;
    HBITMAP hbmp;

    /* Allocate a surface */
    psurf = SURFACE_AllocSurface(STYPE_DEVBITMAP,
                                 sizl.cx,
                                 sizl.cy,
                                 iFormat,
                                 0,
                                 0,
                                 0,
                                 NULL);
    if (!psurf)
    {
        DPRINT1("SURFACE_AllocSurface failed. (STYPE_DEVBITMAP, sizl.cx=%ld, sizl.cy=%ld, iFormat=%lu)\n",
                sizl.cx, sizl.cy, iFormat);
        return NULL;
    }

    /* Set the device handle */
    psurf->SurfObj.dhsurf = dhsurf;

    /* Set public ownership */
    GDIOBJ_vSetObjectOwner(&psurf->BaseObject, GDI_OBJ_HMGR_PUBLIC);

    /* Get the handle for the bitmap */
    hbmp = (HBITMAP)psurf->SurfObj.hsurf;

    /* Unlock the surface and return */
    SURFACE_UnlockSurface(psurf);
    return hbmp;
}

HSURF
APIENTRY
EngCreateDeviceSurface(
    _In_ DHSURF dhsurf,
    _In_ SIZEL sizl,
    _In_ ULONG iFormat)
{
    PSURFACE psurf;
    HSURF hsurf;

    /* Allocate a surface */
    psurf = SURFACE_AllocSurface(STYPE_DEVICE,
                                 sizl.cx,
                                 sizl.cy,
                                 iFormat,
                                 0,
                                 0,
                                 0,
                                 NULL);
    if (!psurf)
    {
        DPRINT1("SURFACE_AllocSurface failed. (STYPE_DEVICE, sizl.cx=%ld, sizl.cy=%ld, iFormat=%lu)\n",
                sizl.cx, sizl.cy, iFormat);
        return NULL;
    }

    /* Set the device handle */
    psurf->SurfObj.dhsurf = dhsurf;

    /* Set public ownership */
    GDIOBJ_vSetObjectOwner(&psurf->BaseObject, GDI_OBJ_HMGR_PUBLIC);

    /* Get the handle for the surface */
    hsurf = psurf->SurfObj.hsurf;

    /* Unlock the surface and return */
    SURFACE_UnlockSurface(psurf);
    return hsurf;
}

BOOL
APIENTRY
EngAssociateSurface(
    _In_ HSURF hsurf,
    _In_ HDEV hdev,
    _In_ FLONG flHooks)
{
    SURFOBJ *pso;
    PSURFACE psurf;
    PDEVOBJ* ppdev;
    PPALETTE ppal;

    ppdev = (PDEVOBJ*)hdev;

    /* Lock the surface */
    psurf = SURFACE_ShareLockSurface(hsurf);
    if (!psurf)
    {
        return FALSE;
    }
    pso = &psurf->SurfObj;

    /* Associate the hdev */
    pso->hdev = hdev;
    pso->dhpdev = ppdev->dhpdev;

    /* Hook up specified functions */
    psurf->flags &= ~HOOK_FLAGS;
    psurf->flags |= (flHooks & HOOK_FLAGS);

    /* Assign the PDEV's palette */
    ppal = PALETTE_ShareLockPalette(ppdev->devinfo.hpalDefault);
    SURFACE_vSetPalette(psurf, ppal);
    PALETTE_ShareUnlockPalette(ppal);

    SURFACE_ShareUnlockSurface(psurf);

    return TRUE;
}

BOOL
APIENTRY
EngModifySurface(
    _In_ HSURF hsurf,
    _In_ HDEV hdev,
    _In_ FLONG flHooks,
    _In_ FLONG flSurface,
    _In_ DHSURF dhsurf,
    _In_ PVOID pvScan0,
    _In_ LONG lDelta,
    _Reserved_ PVOID pvReserved)
{
    SURFOBJ *pso;
    PSURFACE psurf;
    PDEVOBJ* ppdev;
    PPALETTE ppal;

    /* Lock the surface */
    psurf = SURFACE_ShareLockSurface(hsurf);
    if (psurf == NULL)
    {
        DPRINT1("Failed to reference surface %p\n", hsurf);
        return FALSE;
    }

    ppdev = (PDEVOBJ*)hdev;
    pso = &psurf->SurfObj;
    pso->dhsurf = dhsurf;

    /* Associate the hdev */
    pso->hdev = hdev;
    pso->dhpdev = ppdev->dhpdev;

    /* Hook up specified functions */
    psurf->flags &= ~HOOK_FLAGS;
    psurf->flags |= (flHooks & HOOK_FLAGS);

    /* Assign the PDEV's palette */
    ppal = PALETTE_ShareLockPalette(ppdev->devinfo.hpalDefault);
    SURFACE_vSetPalette(psurf, ppal);
    PALETTE_ShareUnlockPalette(ppal);

    /* Update surface flags */
    if (flSurface & MS_NOTSYSTEMMEMORY)
         pso->fjBitmap |= BMF_NOTSYSMEM;
    else
        pso->fjBitmap &= ~BMF_NOTSYSMEM;
    if (flSurface & MS_SHAREDACCESS)
         psurf->flags |= SHAREACCESS_SURFACE;
    else
        psurf->flags &= ~SHAREACCESS_SURFACE;

    /* Check if the caller passed bitmap bits */
    if ((pvScan0 != NULL) && (lDelta != 0))
    {
        /* Update the fields */
        pso->pvScan0 = pvScan0;
        pso->lDelta = lDelta;

        /* This is a bitmap now! */
        pso->iType = STYPE_BITMAP;

        /* Check memory layout */
        if (lDelta > 0)
        {
            /* Topdown is the normal way */
            pso->cjBits = lDelta * pso->sizlBitmap.cy;
            pso->pvBits = pso->pvScan0;
            pso->fjBitmap |= BMF_TOPDOWN;
        }
        else
        {
            /* Inversed bitmap (bottom up) */
            pso->cjBits = (-lDelta) * pso->sizlBitmap.cy;
            pso->pvBits = (PCHAR)pso->pvScan0 - pso->cjBits - lDelta;
            pso->fjBitmap &= ~BMF_TOPDOWN;
        }
    }
    else
    {
        /* Set bits to NULL */
        pso->pvBits = NULL;
        pso->pvScan0 = NULL;
        pso->lDelta = 0;

        /* Set appropriate surface type */
        if (pso->iType != STYPE_DEVICE)
            pso->iType = STYPE_DEVBITMAP;
    }

    SURFACE_ShareUnlockSurface(psurf);

    return TRUE;
}


BOOL
APIENTRY
EngDeleteSurface(
    _In_ _Post_ptr_invalid_ HSURF hsurf)
{
    PSURFACE psurf;

    psurf = SURFACE_ShareLockSurface(hsurf);
    if (!psurf)
    {
        DPRINT1("Could not reference surface %p to delete\n", hsurf);
        return FALSE;
    }

    GDIOBJ_vDeleteObject(&psurf->BaseObject);
    return TRUE;
}

BOOL
APIENTRY
EngEraseSurface(
    _In_ SURFOBJ *pso,
    _In_ RECTL *prcl,
    _In_ ULONG iColor)
{
    ASSERT(pso);
    ASSERT(prcl);
    return FillSolid(pso, prcl, iColor);
}

/*
 * @implemented
 */
SURFOBJ * APIENTRY
NtGdiEngLockSurface(IN HSURF hsurf)
{
    return EngLockSurface(hsurf);
}


SURFOBJ *
APIENTRY
EngLockSurface(
    _In_ HSURF hsurf)
{
    SURFACE *psurf = SURFACE_ShareLockSurface(hsurf);

    return psurf ? &psurf->SurfObj : NULL;
}

__kernel_entry
NTSTATUS
APIENTRY
NtGdiEngUnlockSurface(
    _In_ SURFOBJ *pso)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

VOID
APIENTRY
EngUnlockSurface(
    _In_ _Post_ptr_invalid_ SURFOBJ *pso)
{
    if (pso != NULL)
    {
        SURFACE *psurf = CONTAINING_RECORD(pso, SURFACE, SurfObj);
        SURFACE_ShareUnlockSurface(psurf);
    }
}

/* EOF */
