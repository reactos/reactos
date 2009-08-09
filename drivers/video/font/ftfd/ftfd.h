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

#define TAG_GLYPHSET 'GlSt'
#define TAG_IFIMETRICS 'Ifim'

/** Driver specific types *****************************************************/

typedef struct
{
    FT_UInt index;
    FT_ULong charcode;
} FTFD_CHARPAIR;

typedef struct
{
    PVOID pvView;
    ULONG cjView;
    ULONG_PTR iFile;
    ULONG cNumFaces;
    FT_Face aftface[1];
} FTFD_FILE, *PFTFD_FILE;

//"Bold Italic Underline Strikeout"
#define MAX_STYLESIZE 35
typedef struct
{
    IFIMETRICS ifim;
    BYTE ajCharSet[16];
    WCHAR wszFamilyName[LF_FACESIZE];
    WCHAR wszFaceName[LF_FACESIZE];
    WCHAR wszStyleName[MAX_STYLESIZE];
} FTFD_IFIMETRICS, *PFTFD_IFIMETRICS;

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

