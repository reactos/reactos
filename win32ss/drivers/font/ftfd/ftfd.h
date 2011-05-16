/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         GDI font driver for bitmap fonts
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#ifndef _FTFD_PCH_
#define _FTFD_PCH_

#include <stdarg.h>
#include <windef.h>
#include <wingdi.h>
#include <winddi.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_ADVANCES_H
#include FT_XFREE86_H
#include FT_TYPE1_TABLES_H
#include FT_MULTIPLE_MASTERS_H
#include FT_TRIGONOMETRY_H


extern FT_Library gftlibrary;

#define TAG_GLYPHSET 'GlSt'
#define TAG_IFIMETRICS 'Ifim'

#if 1// DBG
#define ASSERT(x) \
    if(!(x)) \
    { \
        DbgPrint("Assertion '%s' failed at %s:%i\n", #x, __FILE__, __LINE__); \
        __debugbreak(); \
    }
#define TRACE DbgPrint
#define WARN DbgPrint
#define FATAL DbgPrint
#else
#define ASSERT(x)
#define TRACE(...)
#define WARN(...)
#define FATAL(...)
#endif

/* Helper for FLOATOBJ */
#ifdef _M_IX86
#define FLOATOBJ_bIsNull(pf) (((pf)->ul1 == 0) && ((pf)->ul2 == 0))
#else
#define FLOATOBJ_bIsNull(pf) (*(pf) == 0)
#endif

/** Driver specific types *****************************************************/

typedef enum
{
    FMT_UNKNOWN,
    FMT_TRUETYPE,
    FMT_TYPE1,
    FMT_CFF,
    FMT_FNT,
    FMT_BDF,
    FMT_PCF,
    FMT_TYPE42,
    FMT_CIDTYPE1,
    FMT_PFR
} FONT_FORMAT;

typedef enum
{
    FILEFMT_TTF, /* TrueType font file */
    FILEFMT_OTF, /* OpenType font file */
    FILEFMT_FNT, /* Windows .fnt file */
} FILE_FORMAT;

//"Bold Italic Underline Strikeout"
#define MAX_STYLESIZE 35
typedef struct
{
    IFIMETRICS ifi;
    BYTE ajCharSet[16];
    FONTSIM fontsim;
    WCHAR awcFamilyName[LF_FACESIZE];
    WCHAR awcFaceName[LF_FACESIZE];
    WCHAR awcStyleName[MAX_STYLESIZE];
    WCHAR awcUniqueName[LF_FACESIZE + 11];
} FTFD_IFIMETRICS, *PFTFD_IFIMETRICS;

typedef struct
{
    struct _FTFD_FILE *pfile;
    FT_Face ftface;
    ULONG iFace;
    ULONG ulFontFormat;
    ULONG cGlyphs;
    ULONG cMappings;
    ULONG cRuns;
    PWCHAR pwcReverseTable;
    FD_GLYPHSET *pGlyphSet;
    FD_KERNINGPAIR *pKerningPairs;
    FTFD_IFIMETRICS ifiex;
} FTFD_FACE, *PFTFD_FACE;

typedef struct _FTFD_FILE
{
    PVOID pvView;
    ULONG cjView;
    ULONG_PTR iFile;
    ULONG cNumFaces;
    ULONG ulFastCheckSum;
    ULONG ulFileFormat;
    DESIGNVECTOR dv;
    PFTFD_FACE apface[1];
} FTFD_FILE, *PFTFD_FILE;

typedef union _FTFD_DEVICEMETRICS
{
    POINTL aptl[7];
    struct
    {
        POINTFIX ptfxMaxAscender;
        POINTFIX ptfxMaxDescender;
        POINTL ptlUnderline1;
        POINTL ptlStrikeout;
        POINTL ptlULThickness;
        POINTL ptlSOThickness;
        SIZEL sizlMax;
    };
} FTFD_DEVICEMETRICS;

typedef struct _POINTEF
{
    FLOATOBJ x;
    FLOATOBJ y;
} POINTEF, *PPOINTEF;

typedef struct
{
    FONTOBJ *pfo;
    PFTFD_FILE pfile;
    PFTFD_FACE pface;
    ULONG iFace;
    FT_Face ftface;
    FD_XFORM fdxQuantized;
    FTFD_DEVICEMETRICS metrics;
    POINTEF ptefBase;
    POINTEF ptefSide;
    SIZEL sizlScale;
    HGLYPH hgSelected;
    ULONG cjSelected;
    UCHAR jBpp;
} FTFD_FONT, *PFTFD_FONT;


/** Function prototypes *******************************************************/

ULONG
DbgPrint(IN PCCH Format, IN ...);

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
FtfdDestroyFont(
    FONTOBJ *pfo);

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

BOOL
APIENTRY
FtfdQueryAdvanceWidths(
    DHPDEV dhpdev,
    FONTOBJ *pfo,
    ULONG iMode,
    HGLYPH *phg,
    PVOID pvWidths,
    ULONG cGlyphs);

LONG
APIENTRY
FtfdQueryTrueTypeOutline(
    DHPDEV dhpdev,
    FONTOBJ *pfo,
    HGLYPH hglyph,
    BOOL bMetricsOnly,
    GLYPHDATA *pgldt,
    ULONG cjBuf,
    TTPOLYGONHEADER *ppoly);

LONG
APIENTRY
FtfdQueryTrueTypeTable(
    ULONG_PTR iFile,
    ULONG ulFont,
    ULONG ulTag,
    PTRDIFF dpStart,
    ULONG cjBuf,
    BYTE *pjBuf,
    PBYTE *ppjTable,
    ULONG *pcjTable);

ULONG
APIENTRY
FtfdEscape(
    SURFOBJ *pso,
    ULONG iEsc,
    ULONG cjIn,
    PVOID pvIn,
    ULONG cjOut,
    PVOID pvOut);

ULONG
APIENTRY
FtfdFontManagement(
    SURFOBJ *pso,
    FONTOBJ *pfo,
    ULONG iMode,
    ULONG cjIn,
    PVOID pvIn,
    ULONG cjOut,
    PVOID pvOut);

PVOID
APIENTRY
FtfdGetTrueTypeFile(
    ULONG_PTR iFile,
    ULONG *pcj);

/* Private interface */

PVOID
NTAPI
FtfdFindTrueTypeTable(
    PVOID pvView,
    ULONG cjView,
    ULONG iFace,
    ULONG ulTag,
    PULONG pulLength);

BOOL
NTAPI
FtfdGetWinMetrics(
    PFTFD_FACE pface,
    PIFIMETRICS pifi);

VOID
NTAPI
FtfdInitKerningPairs(
    PFTFD_FACE pface);

#endif /* _FTFD_PCH_ */
