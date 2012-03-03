/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Surace Functions
 * FILE:              subsys/win32k/eng/surface.c
 * PROGRAMERS:        Jason Filby
 *                    Timo Kreuzer
 * TESTING TO BE DONE:
 * - Create a GDI bitmap with all formats, perform all drawing operations on them, render to VGA surface
 *   refer to \test\microwin\src\engine\devdraw.c for info on correct pixel plotting for various formats
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

ULONG giUniqueSurface = 0;

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


ULONG FASTCALL BitmapFormat(ULONG cBits, ULONG iCompression)
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

BOOL
NTAPI
SURFACE_Cleanup(PVOID ObjectBody)
{
    PSURFACE psurf = (PSURFACE)ObjectBody;
    PVOID pvBits = psurf->SurfObj.pvBits;
    NTSTATUS Status;

    /* Check if the surface has bits */
    if (pvBits)
    {
        /* Only bitmaps can have bits */
        ASSERT(psurf->SurfObj.iType == STYPE_BITMAP);

        /* Check if it is a DIB section */
        if (psurf->hDIBSection)
        {
            /* Unsecure the memory */
            EngUnsecureMem(psurf->hSecure);

            /* Calculate the real start of the section */
            pvBits = (PVOID)((ULONG_PTR)pvBits - psurf->dwOffset);

            /* Unmap the section */
            Status = MmUnmapViewOfSection(PsGetCurrentProcess(), pvBits);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Could not unmap section view!\n");
                // Should we BugCheck here?
                ASSERT(FALSE);
            }
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
        else if (psurf->SurfObj.fjBitmap & BMF_RLE_HACK)
        {
            /* HACK: Free RLE decompressed bits */
            EngFreeMem(pvBits);
        }
        else
        {
            /* There should be nothing to free */
            ASSERT(psurf->SurfObj.fjBitmap & BMF_DONT_FREE);
        }
    }

    /* Free palette */
    if(psurf->ppal)
    {
        PALETTE_ShareUnlockPalette(psurf->ppal);
    }

    return TRUE;
}


PSURFACE
NTAPI
SURFACE_AllocSurface(
    IN USHORT iType,
    IN ULONG cx,
    IN ULONG cy,
    IN ULONG iFormat)
{
    PSURFACE psurf;
    SURFOBJ *pso;

    /* Verify format */
    if (iFormat < BMF_1BPP || iFormat > BMF_PNG)
    {
        DPRINT1("Invalid bitmap format: %ld\n", iFormat);
        return NULL;
    }

    /* Allocate a SURFACE object */
    psurf = (PSURFACE)GDIOBJ_AllocObjWithHandle(GDI_OBJECT_TYPE_BITMAP, sizeof(SURFACE));

    if (psurf)
    {
        /* Initialize the basic fields */
        pso = &psurf->SurfObj;
        pso->hsurf = psurf->BaseObject.hHmgr;
        pso->sizlBitmap.cx = cx;
        pso->sizlBitmap.cy = cy;
        pso->iBitmapFormat = iFormat;
        pso->iType = iType;
        pso->iUniq = InterlockedIncrement((PLONG)&giUniqueSurface);

        /* Assign a default palette and increment its reference count */
        psurf->ppal = appalSurfaceDefault[iFormat];
        GDIOBJ_vReferenceObjectByPointer(&psurf->ppal->BaseObject);
    }

    return psurf;
}

BOOL
NTAPI
SURFACE_bSetBitmapBits(
    IN PSURFACE psurf,
    IN ULONG fjBitmap,
    IN ULONG ulWidth,
    IN PVOID pvBits OPTIONAL)
{
    SURFOBJ *pso = &psurf->SurfObj;
    PVOID pvSection;
    UCHAR cBitsPixel;

    /* Only bitmaps can have bits */
    ASSERT(psurf->SurfObj.iType == STYPE_BITMAP);

    /* Get bits per pixel from the format */
    cBitsPixel = gajBitsPerFormat[pso->iBitmapFormat];

    /* Is a width in bytes given? */
    if (ulWidth)
    {
        /* Align the width (Windows compatibility, drivers expect that) */
        ulWidth = WIDTH_BYTES_ALIGN32((ulWidth << 3) / cBitsPixel, cBitsPixel);
    }
	else
	{
        /* Calculate width from the bitmap width in pixels */
        ulWidth = WIDTH_BYTES_ALIGN32(pso->sizlBitmap.cx, cBitsPixel);
	}

    /* Calculate the bitmap size in bytes */
    pso->cjBits = ulWidth * pso->sizlBitmap.cy;

    /* Did the caller provide bits? */
    if (pvBits)
    {
        /* Yes, so let him free it */
        fjBitmap |= BMF_DONT_FREE;
    }
    else if (pso->cjBits)
    {
        /* We must allocate memory, check what kind */
        if (fjBitmap & BMF_USERMEM)
        {
            /* User mode memory was requested */
            pvBits = EngAllocUserMem(pso->cjBits, 0);
        }
        else
        {
            /* Use a kernel mode section */
            fjBitmap |= BMF_KMSECTION;
            pvBits = EngAllocSectionMem(&pvSection,
                                        (fjBitmap & BMF_NOZEROINIT) ?
                                                0 : FL_ZERO_MEMORY,
                                        pso->cjBits, TAG_DIB);

            /* Free the section already, but keep the mapping */
            if (pvBits) EngFreeSectionMem(pvSection, NULL);
        }

        /* Check for failure */
        if (!pvBits) return FALSE;
    }

    /* Set pvBits, pvScan0 and lDelta */
    pso->pvBits = pvBits;
    if (fjBitmap & BMF_TOPDOWN)
    {
        /* Topdown is the normal way */
        pso->pvScan0 = pso->pvBits;
        pso->lDelta = ulWidth;
    }
    else
    {
        /* Inversed bitmap (bottom up) */
        pso->pvScan0 = (PVOID)((ULONG_PTR)pso->pvBits + pso->cjBits - ulWidth);
        pso->lDelta = -(LONG)ulWidth;
    }

    pso->fjBitmap = (USHORT)fjBitmap;

    /* Success */
    return TRUE;
}

HBITMAP
APIENTRY
EngCreateBitmap(
    IN SIZEL sizl,
    IN LONG lWidth,
    IN ULONG iFormat,
    IN ULONG fl,
    IN PVOID pvBits)
{
    PSURFACE psurf;
    HBITMAP hbmp;

    /* Allocate a surface */
    psurf = SURFACE_AllocSurface(STYPE_BITMAP, sizl.cx, sizl.cy, iFormat);
    if (!psurf)
    {
        DPRINT1("SURFACE_AllocSurface failed.\n");
        return NULL;
    }

    /* Get the handle for the bitmap */
    hbmp = (HBITMAP)psurf->SurfObj.hsurf;

    /* Set the bitmap bits */
    if (!SURFACE_bSetBitmapBits(psurf, fl, lWidth, pvBits))
    {
        /* Bail out if that failed */
        DPRINT1("SURFACE_bSetBitmapBits failed.\n");
        GDIOBJ_vDeleteObject(&psurf->BaseObject);
        return NULL;
    }

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
    IN DHSURF dhsurf,
    IN SIZEL sizl,
    IN ULONG iFormat)
{
    PSURFACE psurf;
    HBITMAP hbmp;

    /* Allocate a surface */
    psurf = SURFACE_AllocSurface(STYPE_DEVBITMAP, sizl.cx, sizl.cy, iFormat);
    if (!psurf)
    {
        return 0;
    }

    /* Set the device handle */
    psurf->SurfObj.dhsurf = dhsurf;

    /* Get the handle for the bitmap */
    hbmp = (HBITMAP)psurf->SurfObj.hsurf;

    /* Set public ownership */
    GDIOBJ_vSetObjectOwner(&psurf->BaseObject, GDI_OBJ_HMGR_PUBLIC);

    /* Unlock the surface and return */
    SURFACE_UnlockSurface(psurf);
    return hbmp;
}

HSURF
APIENTRY
EngCreateDeviceSurface(
    IN DHSURF dhsurf,
    IN SIZEL sizl,
    IN ULONG iFormat)
{
    PSURFACE psurf;
    HSURF hsurf;

    /* Allocate a surface */
    psurf = SURFACE_AllocSurface(STYPE_DEVICE, sizl.cx, sizl.cy, iFormat);
    if (!psurf)
    {
        return 0;
    }

    /* Set the device handle */
    psurf->SurfObj.dhsurf = dhsurf;

    /* Get the handle for the surface */
    hsurf = psurf->SurfObj.hsurf;

    /* Set public ownership */
    GDIOBJ_vSetObjectOwner(&psurf->BaseObject, GDI_OBJ_HMGR_PUBLIC);

    /* Unlock the surface and return */
    SURFACE_UnlockSurface(psurf);
    return hsurf;
}

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

    /* Get palette */
    psurf->ppal = PALETTE_ShareLockPalette(ppdev->devinfo.hpalDefault);

    SURFACE_ShareUnlockSurface(psurf);

    return TRUE;
}

BOOL
APIENTRY
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

    psurf = SURFACE_ShareLockSurface(hsurf);
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
    psurf->flags &= ~HOOK_FLAGS;
    psurf->flags |= (flHooks & HOOK_FLAGS);

    /* Get palette */
    psurf->ppal = PALETTE_ShareLockPalette(ppdev->devinfo.hpalDefault);

    SURFACE_ShareUnlockSurface(psurf);

    return TRUE;
}


BOOL
APIENTRY
EngDeleteSurface(IN HSURF hsurf)
{
    PSURFACE psurf;

    psurf = SURFACE_ShareLockSurface(hsurf);
    if (!psurf)
    {
        DPRINT1("Could not reference surface to delete\n");
        return FALSE;
    }

    GDIOBJ_vDeleteObject(&psurf->BaseObject);
    return TRUE;
}

BOOL
APIENTRY
EngEraseSurface(
    SURFOBJ *pso,
    RECTL *prcl,
    ULONG iColor)
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
EngLockSurface(IN HSURF hsurf)
{
    SURFACE *psurf = SURFACE_ShareLockSurface(hsurf);

    return psurf ? &psurf->SurfObj : NULL;
}

VOID
APIENTRY
NtGdiEngUnlockSurface(IN SURFOBJ *pso)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
}

VOID
APIENTRY
EngUnlockSurface(IN SURFOBJ *pso)
{
    if (pso != NULL)
    {
        SURFACE *psurf = CONTAINING_RECORD(pso, SURFACE, SurfObj);
        SURFACE_ShareUnlockSurface(psurf);
    }
}

/* EOF */
