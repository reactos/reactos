/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         GDI font driver for bitmap fonts
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include "ftfd.h"

PVOID
HackFixup(
    PVOID pvView,
    ULONG cjView)
{
    CHAR *pc;
    CHAR c;

    pc = EngAllocMem(0, cjView, 'tmp ');
    memcpy(pc, pvView, cjView);

    c = *pc;
    *pc = 0;

    return pc;
}

/** Public Interface **********************************************************/

ULONG_PTR
APIENTRY
FtfdLoadFontFile(
    ULONG cFiles,
    ULONG_PTR *piFile,
    PVOID *ppvView,
    ULONG *pcjView,
    DESIGNVECTOR *pdv,
    ULONG ulLangID,
    ULONG ulFastCheckSum)
{
    PVOID pvView;
    ULONG cjView, i;
    FT_Error fterror;
    FT_Face ftface;
    PFTFD_FILE pfile;

    DbgPrint("FtfdLoadFontFile()\n");

    /* Check parameters */
    if (cFiles != 1)
    {
        DbgPrint("Only 1 File is allowed, got %ld!\n", cFiles);
        return HFF_INVALID;
    }

    /* Map the font file */
    if (!EngMapFontFileFD(*piFile, (PULONG*)&pvView, &cjView))
    {
        DbgPrint("Could not map font file!\n", cFiles);
        return HFF_INVALID;
    }

    // HACK!!!
    pvView = HackFixup(pvView, cjView);

    /* Look for faces in the file */
    for (i = 0; i < 100; i++)
    {
        fterror = FT_New_Memory_Face(gftlibrary, pvView, cjView, i, &ftface);
        if (fterror)
        {
            DbgPrint("Error reading font file (error code: %u)\n", fterror);
            break;
        }
        FT_Done_Face(ftface);
    }

    /* Check whether we succeeded finding a face */
    if (i > 0)
    {
        pfile = EngAllocMem(0, sizeof(FTFD_FILE), 'dftF');
        if (pfile)
        {
            pfile->cNumFaces = i;
            pfile->iFile = *piFile;
            pfile->pvView = pvView;
            pfile->cjView = cjView;

            DbgPrint("Success! Returning %ld faces\n", i);

            return (ULONG_PTR)pfile;
        }
    }

    DbgPrint("No faces found in file\n");

    /* Unmap the file */
    EngUnmapFontFileFD(*piFile);

    /* Failure! */
    return HFF_INVALID;

}

BOOL
APIENTRY
FtfdUnloadFontFile(
    IN ULONG_PTR iFile)
{
    PFTFD_FILE pfile = (PFTFD_FILE)iFile;

    DbgPrint("FtfdUnloadFontFile()\n");

    // HACK!!!
    EngFreeMem(pfile->pvView);

    /* Free the memory that was allocated for the font */
    EngFreeMem(pfile);

    /* Unmap the font file */
    EngUnmapFontFileFD(pfile->iFile);

    return TRUE;
}


LONG
APIENTRY
FtfdQueryFontFile(
    ULONG_PTR iFile,
    ULONG ulMode,
    ULONG cjBuf,
    ULONG *pulBuf)
{
    PFTFD_FILE pfile = (PFTFD_FILE)iFile;

    DbgPrint("FtfdQueryFontFile(ulMode=%ld)\n", ulMode);
//    DbgBreakPoint();

    switch (ulMode)
    {
        case QFF_DESCRIPTION:
        {
            return 0;
        }

        case QFF_NUMFACES:
            /* return the number of faces in the file */
            return pfile->cNumFaces;

    }

    return FD_ERROR;
}

LONG
APIENTRY
FtfdQueryFontCaps(
    ULONG culCaps,
    ULONG *pulCaps)
{
    DbgPrint("BmfdQueryFontCaps()\n");

    /* We need room for 2 ULONGs */
    if (culCaps < 2)
    {
        return FD_ERROR;
    }

    /* We only support 1 bpp */
    pulCaps[0] = 2;
    pulCaps[1] = QC_1BIT;

    return 2;
}


PVOID
APIENTRY
FtfdQueryFontTree(
    DHPDEV dhpdev,
    ULONG_PTR iFile,
    ULONG iFace,
    ULONG iMode,
    ULONG_PTR *pid)
{
    return NULL;
}

PIFIMETRICS
APIENTRY
FtfdQueryFont(
    IN DHPDEV dhpdev,
    IN ULONG_PTR iFile,
    IN ULONG iFace,
    IN ULONG_PTR *pid)
{
    return 0;
}


VOID
APIENTRY
FtfdFree(
    PVOID pv,
    ULONG_PTR id)
{
    DbgPrint("FtfdFree()\n");
    if (id)
    {
        EngFreeMem((PVOID)id);
    }
}



