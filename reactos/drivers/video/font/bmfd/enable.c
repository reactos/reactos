/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         GDI font driver for bitmap fonts
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include "bmfd.h"

static DRVFN gadrvfn[] =
{
    {INDEX_DrvEnablePDEV,		(PFN)BmfdEnablePDEV},
    {INDEX_DrvCompletePDEV,		(PFN)BmfdCompletePDEV},
    {INDEX_DrvDisablePDEV,		(PFN)BmfdDisablePDEV},
    {INDEX_DrvLoadFontFile,		(PFN)BmfdLoadFontFile},
    {INDEX_DrvUnloadFontFile,	(PFN)BmfdUnloadFontFile},
    {INDEX_DrvQueryFontFile,	(PFN)BmfdQueryFontFile},
    {INDEX_DrvQueryFontCaps,	(PFN)BmfdQueryFontCaps},
    {INDEX_DrvQueryFontTree,	(PFN)BmfdQueryFontTree},
    {INDEX_DrvQueryFont,		(PFN)BmfdQueryFont},
    {INDEX_DrvFree,				(PFN)BmfdFree},
    {INDEX_DrvQueryGlyphAttrs,	(PFN)BmfdQueryGlyphAttrs},
    {INDEX_DrvQueryFontData,	(PFN)BmfdQueryFontData},
};


ULONG
DbgPrint(IN PCHAR Format, IN ...)
{
    va_list args;

    va_start(args, Format);
    EngDebugPrint("Bmfd: ", Format, args);
    va_end(args);
    return 0;
}


BOOL
APIENTRY
BmfdEnableDriver(
    ULONG iEngineVersion,
    ULONG cj,
    PDRVENABLEDATA pded)
{
    DbgPrint("BmfdEnableDriver()\n");

    /* Check parameter */
    if (cj < sizeof(DRVENABLEDATA))
    {
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
BmfdEnablePDEV(
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
    DbgPrint("BmfdEnablePDEV(hdev=%p)\n", hdev);

    /* Return a dummy DHPDEV */
    return (PVOID)1;
}


VOID
APIENTRY
BmfdCompletePDEV(
    IN DHPDEV dhpdev,
    IN HDEV hdev)
{
    DbgPrint("BmfdCompletePDEV()\n");
    /* Nothing to do */
}


VOID
APIENTRY
BmfdDisablePDEV(
    IN DHPDEV dhpdev)
{
    DbgPrint("BmfdDisablePDEV()\n");
    /* Nothing to do */
}
