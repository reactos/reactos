/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         GDI font driver for bitmap fonts
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <stdarg.h>
#include <windef.h>
#include <wingdi.h>
#include <winddi.h>

#include <ft2build.h>
#include FT_FREETYPE_H 

extern FT_Library gftlibrary;

/** Driver specific types *****************************************************/


typedef struct
{
    PVOID pvView;
    ULONG cjView;
    ULONG_PTR iFile;
    ULONG cNumFaces;
} FTFD_FILE, *PFTFD_FILE;

/** Function prototypes *******************************************************/

ULONG
DbgPrint(IN PCHAR Format, IN ...);

static __inline__
void
DbgBreakPoint(void)
{
    asm volatile ("int $3");
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
    IN HANDLE hDriver);

VOID
APIENTRY
FtfdCompletePDEV(
    IN DHPDEV dhpdev,
    IN HDEV hdev);

VOID
APIENTRY
FtfdDisablePDEV(
    IN DHPDEV dhpdev);

ULONG_PTR
APIENTRY
FtfdLoadFontFile(
    ULONG cFiles,
    ULONG_PTR *piFile,
    PVOID *ppvView,
    ULONG *pcjView,
    DESIGNVECTOR *pdv,
    ULONG ulLangID,
    ULONG ulFastCheckSum);

BOOL
APIENTRY
FtfdUnloadFontFile(
    IN ULONG_PTR iFile);

LONG
APIENTRY
FtfdQueryFontFile(
    ULONG_PTR iFile,
    ULONG ulMode,
    ULONG cjBuf,
    ULONG *pulBuf);

LONG
APIENTRY
FtfdQueryFontCaps(
    ULONG culCaps,
    ULONG *pulCaps);

PVOID
APIENTRY
FtfdQueryFontTree(
    DHPDEV dhpdev,
    ULONG_PTR iFile,
    ULONG iFace,
    ULONG iMode,
    ULONG_PTR *pid);

PIFIMETRICS
APIENTRY
FtfdQueryFont(
    IN DHPDEV dhpdev,
    IN ULONG_PTR iFile,
    IN ULONG iFace,
    IN ULONG_PTR *pid);

VOID
APIENTRY
FtfdFree(
    PVOID pv,
    ULONG_PTR id);

PFD_GLYPHATTR
APIENTRY
FtfdQueryGlyphAttrs(
	FONTOBJ *pfo,
	ULONG iMode);

LONG
APIENTRY
FtfdQueryFontData(
	DHPDEV dhpdev,
	FONTOBJ *pfo,
	ULONG iMode,
	HGLYPH hg,
	OUT GLYPHDATA *pgd,
	PVOID pv,
	ULONG cjSize);

