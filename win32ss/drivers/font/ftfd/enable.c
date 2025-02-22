/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         GDI font driver for bitmap fonts
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include "ftfd.h"

static DRVFN gadrvfn[] =
{
    {INDEX_DrvEnablePDEV,		(PFN)FtfdEnablePDEV},
    {INDEX_DrvCompletePDEV,		(PFN)FtfdCompletePDEV},
    {INDEX_DrvDisablePDEV,		(PFN)FtfdDisablePDEV},
    {INDEX_DrvLoadFontFile,		(PFN)FtfdLoadFontFile},
    {INDEX_DrvUnloadFontFile,	(PFN)FtfdUnloadFontFile},
    {INDEX_DrvQueryFontFile,	(PFN)FtfdQueryFontFile},
    {INDEX_DrvQueryFontCaps,	(PFN)FtfdQueryFontCaps},
    {INDEX_DrvQueryFontTree,	(PFN)FtfdQueryFontTree},
    {INDEX_DrvQueryFont,		(PFN)FtfdQueryFont},
    {INDEX_DrvFree,				(PFN)FtfdFree},
    {INDEX_DrvQueryGlyphAttrs,	(PFN)FtfdQueryGlyphAttrs},
    {INDEX_DrvQueryFontData,	(PFN)FtfdQueryFontData},
};

FT_Library gftlibrary;


BOOL
APIENTRY
FtfdEnableDriver(
    ULONG iEngineVersion,
    ULONG cj,
    PDRVENABLEDATA pded)
{
    FT_Error fterror;

    DbgPrint("FtfdEnableDriver()\n");

    /* Check parameter */
    if (cj < sizeof(DRVENABLEDATA))
    {
        return FALSE;
    }

    /* Initialize freetype library */
    fterror = FT_Init_FreeType(&gftlibrary);
    if (fterror)
    {
        DbgPrint("an error occurred during library initialization: %ld.\n", fterror);
        return FALSE;
    }

    /* Fill DRVENABLEDATA */
    pded->c = sizeof(gadrvfn) / sizeof(DRVFN);
    pded->pdrvfn = gadrvfn;
    pded->iDriverVersion = DDI_DRIVER_VERSION_NT5;

    /* Success */
    return TRUE;
}


DHPDEV
APIENTRY
FtfdEnablePDEV(
    IN DEVMODEW *pdm,
    IN LPWSTR pwszLogAddress,
    IN ULONG cPat,
    OUT HSURF *phsurfPatterns,
    IN ULONG cjCaps,
    OUT ULONG *pdevcaps,
    IN ULONG cjDevInfo,
    OUT DEVINFO *pdi,
    IN HDEV hdev,
    IN LPWSTR pwszDeviceName,
    IN HANDLE hDriver)
{
    DbgPrint("FtfdEnablePDEV(hdev=%p)\n", hdev);
    __debugbreak();


    /* Return a dummy DHPDEV */
    return (PVOID)1;
}


VOID
APIENTRY
FtfdCompletePDEV(
    IN DHPDEV dhpdev,
    IN HDEV hdev)
{
    DbgPrint("FtfdCompletePDEV()\n");
    /* Nothing to do */
}


VOID
APIENTRY
FtfdDisablePDEV(
    IN DHPDEV dhpdev)
{
    DbgPrint("FtfdDisablePDEV()\n");
    /* Nothing to do */
}
