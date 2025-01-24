/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         GDI font driver for bitmap fonts
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#ifndef _BMFD_PCH_
#define _BMFD_PCH_

#include <stdarg.h>
#include <windef.h>
#include <wingdi.h>
#include <winddi.h>

#if defined(_M_IX86) || defined(_M_AMD64)
/* on x86 and x64, unaligned access is allowed, byteorder is LE */
#define GETVAL(x) (x)
#else
// FIXME: BE
#define GETVAL(x) \
    (sizeof(x) == 1) ? (x) : \
    (sizeof(x) == 2) ? (((PCHAR)&(x))[0] + (((PCHAR)&(x))[1] << 8)) : \
    (((PCHAR)&(x))[0] + (((PCHAR)&(x))[1] << 8) + (((PCHAR)&(x))[2] << 16) + \
     (((PCHAR)&(x))[3] << 24))

#endif

#define FDM_MASK \
    FDM_TYPE_CONST_BEARINGS | FDM_TYPE_ZERO_BEARINGS | \
    FDM_TYPE_CHAR_INC_EQUAL_BM_BASE | FDM_TYPE_MAXEXT_EQUAL_BM_SIDE | \
    FDM_TYPE_BM_SIDE_CONST

#define FM_INFO_MASK \
    FM_INFO_TECH_BITMAP | FM_INFO_1BPP | FM_INFO_INTEGER_WIDTH | \
    FM_INFO_RETURNS_BITMAPS | FM_INFO_RIGHT_HANDED | FM_INFO_INTEGRAL_SCALING |\
    FM_INFO_90DEGREE_ROTATIONS | FM_INFO_OPTICALLY_FIXED_PITCH | FM_INFO_NONNEGATIVE_AC

#define FLOATL_1 0x3f800000

#define TAG_PDEV 'veDP'
#define TAG_GLYPHSET 'GlSt'
#define TAG_IFIMETRICS 'Ifim'
#define TAG_FONTINFO 'Font'


/** FON / FNT specific types **************************************************/

#define IMAGE_DOS_MAGIC 0x594D // FIXME: hack hack hack

#include <pshpack1.h>
typedef struct
{
    WORD offset;
    WORD length;
    WORD flags;
    WORD id;
    WORD handle;
    WORD usage;
} NE_NAMEINFO, *PNE_NAMEINFO;

#define NE_RSCTYPE_FONT    0x8008
#define NE_RSCTYPE_FONTDIR 0x8007
typedef struct
{
    WORD type_id;
    WORD count;
    DWORD resloader;
    NE_NAMEINFO nameinfo[1];
} NE_TYPEINFO, *PNE_TYPEINFO;

typedef struct
{
    WORD  size_shift;
    NE_TYPEINFO typeinfo[1];
} NE_RESTABLE, *PNE_RESTABLE;

// Values of dfFlags:
#define DFF_FIXED            0x0001
#define DFF_PROPORTIONAL     0x0002
#define DFF_ABCFIXED         0x0004
#define DFF_ABCPROPORTIONAL  0x0008
#define DFF_1COLOR           0x0010
#define DFF_16COLOR          0x0020
#define DFF_256COLOR         0x0040
#define DFF_RGBCOLOR         0x0080

// see https://learn.microsoft.com/en-us/windows/win32/menurc/fontdirentry
typedef struct _FONTDIRENTRY
{
    WORD dfVersion;
    DWORD dfSize;
    char dfCopyright[60];
    WORD dfType;
    WORD dfPoints;
    WORD dfVertRes;
    WORD dfHorizRes;
    WORD dfAscent;
    WORD dfInternalLeading;
    WORD dfExternalLeading;
    BYTE dfItalic;
    BYTE dfUnderline;
    BYTE dfStrikeOut;
    WORD dfWeight;
    BYTE dfCharSet;
    WORD dfPixWidth;
    WORD dfPixHeight;
    BYTE dfPitchAndFamily;
    WORD dfAvgWidth;
    WORD dfMaxWidth;
    BYTE dfFirstChar;
    BYTE dfLastChar;
    BYTE dfDefaultChar;
    BYTE dfBreakChar;
    WORD dfWidthBytes;
    DWORD dfDevice;
    DWORD dfFace;
    DWORD dfReserved;
    char szDeviceName[1];
    char szFaceName[1];
} FONTDIRENTRY, *PFONTDIRENTRY;

typedef struct _DIRENTRY
{
    WORD fontOrdinal;
    FONTDIRENTRY fde;
} DIRENTRY, *PDIRENTRY;

typedef struct _FONTGROUPHDR
{
  WORD NumberOfFonts;
  DIRENTRY ade[1];
} FONTGROUPHDR, *PFONTGROUPHDR;

typedef struct
{
    WORD dfVersion;
    DWORD dfSize;
    CHAR dfCopyright[60];
    WORD dfType;
    WORD dfPoints;
    WORD dfVertRes;
    WORD dfHorizRes;
    WORD dfAscent;
    WORD dfInternalLeading;
    WORD dfExternalLeading;
    BYTE dfItalic;
    BYTE dfUnderline;
    BYTE dfStrikeOut;
    WORD dfWeight;
    BYTE dfCharSet;
    WORD dfPixWidth;
    WORD dfPixHeight;
    BYTE dfPitchAndFamily;
    WORD dfAvgWidth;
    WORD dfMaxWidth;
    BYTE dfFirstChar;
    BYTE dfLastChar;
    BYTE dfDefaultChar;
    BYTE dfBreakChar;
    WORD dfWidthBytes;
    DWORD dfDevice;
    DWORD dfFace;
    DWORD dfBitsPointer;
    DWORD dfBitsOffset;
    BYTE dfReserved;
    /* Version 3.00: */
    DWORD dfFlags;
    WORD dfAspace;
    WORD dfBspace;
    WORD dfCspace;
    DWORD dfColorPointer;
    DWORD dfReserved1[4];
    BYTE dfCharTable[1];
} FONTINFO16, *LPFONTINFO16, *PFONTINFO16;

typedef struct
{
     WORD geWidth;
     WORD geOffset;
} GLYPHENTRY20, *PGLYPHENTRY20;

typedef struct
{
     WORD geWidth;
     DWORD geOffset;
} GLYPHENTRY30, *PGLYPHENTRY30;

typedef union
{
    GLYPHENTRY20 ge20;
    GLYPHENTRY30 ge30;
} GLYPHENTRY, *PGLYPHENTRY;

#include <poppack.h>


/** Driver specific types *****************************************************/

typedef enum
{
    FONTTYPE_FON,
    FONTTYPE_FNT,
} FONTTYPE;

typedef struct
{
    PFONTDIRENTRY pFontDirEntry;
    PFONTINFO16 pFontInfo;
    PBYTE pCharTable;
    ULONG cjEntrySize;
    ULONG ulVersion;
    PCHAR pszFaceName;
    PCHAR pszCopyright;
    ULONG cGlyphs;
    CHAR chFirstChar;
    CHAR chLastChar;
    WCHAR wcFirstChar;
    WCHAR wcLastChar;
    WCHAR wcDefaultChar;
    WCHAR wcBreakChar;
    WORD wPixHeight;
    WORD wPixWidth;
    WORD wWidthBytes;
    WORD wA;
    WORD wB;
    WORD wC;
    WORD wAscent;
    WORD wDescent;
    FLONG flInfo;
} BMFD_FACE, *PBMFD_FACE;

typedef struct
{
    PVOID pvView;
    ULONG_PTR iFile;
    PFONTGROUPHDR pFontDir;
    FONTTYPE ulFontType;
    ULONG cNumFaces;
    BMFD_FACE aface[1];
} BMFD_FILE, *PBMFD_FILE;

typedef struct
{
    FONTOBJ *pfo;
    PBMFD_FACE pface;
    LONG xScale;
    LONG yScale;
    ULONG ulAngle;
} BMFD_FONT, *PBMFD_FONT;

//"Bold Italic Underline Strikeout"
#define MAX_STYLESIZE 35
typedef struct
{
    IFIMETRICS ifim;
    BYTE ajCharSet[16];
    WCHAR wszFamilyName[LF_FACESIZE];
    WCHAR wszFaceName[LF_FACESIZE];
    WCHAR wszStyleName[MAX_STYLESIZE];
} BMFD_IFIMETRICS, *PBMFD_IFIMETRICS;


/** Function prototypes *******************************************************/

ULONG
DbgPrint(IN PCHAR Format, IN ...);

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
    IN HANDLE hDriver);

VOID
APIENTRY
BmfdCompletePDEV(
    IN DHPDEV dhpdev,
    IN HDEV hdev);

VOID
APIENTRY
BmfdDisablePDEV(
    IN DHPDEV dhpdev);

ULONG_PTR
APIENTRY
BmfdLoadFontFile(
    ULONG cFiles,
    ULONG_PTR *piFile,
    PVOID *ppvView,
    ULONG *pcjView,
    DESIGNVECTOR *pdv,
    ULONG ulLangID,
    ULONG ulFastCheckSum);

BOOL
APIENTRY
BmfdUnloadFontFile(
    IN ULONG_PTR iFile);

LONG
APIENTRY
BmfdQueryFontFile(
    ULONG_PTR iFile,
    ULONG ulMode,
    ULONG cjBuf,
    ULONG *pulBuf);

LONG
APIENTRY
BmfdQueryFontCaps(
    ULONG culCaps,
    ULONG *pulCaps);

PVOID
APIENTRY
BmfdQueryFontTree(
    DHPDEV dhpdev,
    ULONG_PTR iFile,
    ULONG iFace,
    ULONG iMode,
    ULONG_PTR *pid);

PIFIMETRICS
APIENTRY
BmfdQueryFont(
    IN DHPDEV dhpdev,
    IN ULONG_PTR iFile,
    IN ULONG iFace,
    IN ULONG_PTR *pid);

VOID
APIENTRY
BmfdFree(
    PVOID pv,
    ULONG_PTR id);

PFD_GLYPHATTR
APIENTRY
BmfdQueryGlyphAttrs(
	FONTOBJ *pfo,
	ULONG iMode);

LONG
APIENTRY
BmfdQueryFontData(
	DHPDEV dhpdev,
	FONTOBJ *pfo,
	ULONG iMode,
	HGLYPH hg,
	OUT GLYPHDATA *pgd,
	PVOID pv,
	ULONG cjSize);

VOID
APIENTRY
BmfdDestroyFont(
    IN FONTOBJ *pfo);

#endif /* _BMFD_PCH_ */
