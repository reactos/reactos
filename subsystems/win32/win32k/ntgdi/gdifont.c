/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntgdi/gdifont.c
 * PURPOSE:         GDI Font Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

INT
APIENTRY
NtGdiAddFontResourceW(IN WCHAR *pwszFiles,
                      IN ULONG cwc,
                      IN ULONG cFiles,
                      IN FLONG f,
                      IN DWORD dwPidTid,
                      IN OPTIONAL DESIGNVECTOR *pdv)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiRemoveFontResourceW(IN WCHAR *pwszFiles,
                         IN ULONG cwc,
                         IN ULONG cFiles,
                         IN ULONG fl,
                         IN DWORD dwPidTid,
                         IN OPTIONAL DESIGNVECTOR *pdv)
{
    UNIMPLEMENTED;
    return FALSE;
}

HANDLE
APIENTRY
NtGdiAddFontMemResourceEx(IN PVOID pvBuffer,
                          IN DWORD cjBuffer,
                          IN DESIGNVECTOR *pdv,
                          IN ULONG cjDV,
                          OUT DWORD *pNumFonts)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtGdiRemoveFontMemResourceEx(IN HANDLE hMMFont)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiAddRemoteFontToDC(IN HDC hdc,
                       IN PVOID pvBuffer,
                       IN ULONG cjBuffer,
                       IN OPTIONAL PUNIVERSAL_FONT_ID pufi)
{
    UNIMPLEMENTED;
    return FALSE;
}

ULONG
APIENTRY
NtGdiQueryFontAssocInfo(IN HDC hdc)
{
    UNIMPLEMENTED;
    return 0;
}

INT
APIENTRY
NtGdiQueryFonts(OUT PUNIVERSAL_FONT_ID pufiFontList,
                IN ULONG nBufferSize,
                OUT PLARGE_INTEGER pTimeStamp)
{
    UNIMPLEMENTED;
    return 0;
}

ULONG
APIENTRY
NtGdiGetFontData(IN HDC hdc,
                 IN DWORD dwTable,
                 IN DWORD dwOffset,
                 OUT OPTIONAL PVOID pvBuf,
                 IN ULONG cjBuf)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiGetFontResourceInfoInternalW(IN LPWSTR pwszFiles,
                                  IN ULONG cwc,
                                  IN ULONG cFiles,
                                  IN UINT cjIn,
                                  OUT LPDWORD pdwBytes,
                                  OUT LPVOID pvBuf,
                                  IN DWORD iType)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiAnyLinkedFonts(VOID)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiFontIsLinked(IN HDC hdc)
{
    UNIMPLEMENTED;
    return FALSE;
}

ULONG_PTR
APIENTRY
NtGdiEnumFontOpen(IN HDC hdc,
                  IN ULONG iEnumType,
                  IN FLONG flWin31Compat,
                  IN ULONG cwchMax,
                  IN OPTIONAL LPWSTR pwszFaceName,
                  IN ULONG lfCharSet,
                  OUT ULONG *pulCount)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiEnumFontChunk(IN HDC hdc,
                   IN ULONG_PTR idEnum,
                   IN ULONG cjEfdw,
                   OUT ULONG *pcjEfdw,
                   OUT PENUMFONTDATAW pefdw)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEnumFontClose(IN ULONG_PTR idEnum)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiRemoveMergeFont(IN HDC hdc,
                     IN UNIVERSAL_FONT_ID *pufi)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiGetUFI(IN  HDC hdc,
            OUT PUNIVERSAL_FONT_ID pufi,
            OUT OPTIONAL DESIGNVECTOR *pdv,
            OUT ULONG *pcjDV,
            OUT ULONG *pulBaseCheckSum,
            OUT FLONG *pfl)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiGetEmbUFI(IN HDC hdc,
               OUT PUNIVERSAL_FONT_ID pufi,
               OUT OPTIONAL DESIGNVECTOR *pdv,
               OUT ULONG *pcjDV,
               OUT ULONG *pulBaseCheckSum,
               OUT FLONG *pfl,
               OUT KERNEL_PVOID *embFontID)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiGetUFIPathname(IN PUNIVERSAL_FONT_ID pufi,
                    OUT OPTIONAL ULONG* pcwc,
                    OUT OPTIONAL LPWSTR pwszPathname,
                    OUT OPTIONAL ULONG* pcNumFiles,
                    IN FLONG fl,
                    OUT OPTIONAL BOOL *pbMemFont,
                    OUT OPTIONAL ULONG *pcjView,
                    OUT OPTIONAL PVOID pvView,
                    OUT OPTIONAL BOOL *pbTTC,
                    OUT OPTIONAL ULONG *piTTC)
{
    UNIMPLEMENTED;
    return FALSE;
}

ULONG
APIENTRY
NtGdiGetEmbedFonts(VOID)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiForceUFIMapping(IN HDC hdc,
                     IN PUNIVERSAL_FONT_ID pufi)
{
    UNIMPLEMENTED;
    return FALSE;
}

INT
APIENTRY
NtGdiGetLinkedUFIs(IN HDC hdc,
                   OUT OPTIONAL PUNIVERSAL_FONT_ID pufiLinkedUFIs,
                   IN INT BufferSize)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiSetLinkedUFIs(IN HDC hdc,
                   IN PUNIVERSAL_FONT_ID pufiLinks,
                   IN ULONG uNumUFIs)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiChangeGhostFont(IN KERNEL_PVOID *pfontID,
                     IN BOOL bLoad)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiAddEmbFontToDC(IN HDC hdc,
                    IN VOID **pFontID)
{
    UNIMPLEMENTED;
    return FALSE;
}

DWORD
/* Missing APIENTRY! */
NtGdiGetFontUnicodeRanges(IN HDC hdc,
                          OUT OPTIONAL LPGLYPHSET pgs)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiUnmapMemFont(IN PVOID pvView)
{
    UNIMPLEMENTED;
    return FALSE;
}

HFONT
APIENTRY
NtGdiSelectFont(IN HDC hdc,
                IN HFONT hf)
{
    UNIMPLEMENTED;
	return NULL;
}

HFONT
APIENTRY
NtGdiHfontCreate(IN ENUMLOGFONTEXDVW *pelfw,
                 IN ULONG cjElfw,
                 IN LFTYPE lft,
                 IN FLONG fl,
                 IN PVOID pvCliData)
{
    UNIMPLEMENTED;
    return NULL;
}

ULONG
APIENTRY
NtGdiSetFontEnumeration(IN ULONG ulType)
{
    UNIMPLEMENTED;
    return 0;
}

ULONG
APIENTRY
NtGdiMakeFontDir(IN FLONG flEmbed,
                 OUT PBYTE pjFontDir,
                 IN unsigned cjFontDir,
                 IN LPWSTR pwszPathname,
                 IN unsigned cjPathname)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiSetFontXform(IN HDC hdc,
                  IN DWORD dwxScale,
                  IN DWORD dwyScale)
{
    UNIMPLEMENTED;
    return FALSE;
}

INT
APIENTRY
NtGdiSetupPublicCFONT(IN HDC hdc,
                      IN OPTIONAL HFONT hf,
                      IN ULONG ulAve)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiEnableEudc(IN BOOL Param1)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEudcLoadUnloadLink(IN OPTIONAL LPCWSTR pBaseFaceName,
                        IN UINT cwcBaseFaceName,
                        IN LPCWSTR pEudcFontPath,
                        IN UINT cwcEudcFontPath,
                        IN INT iPriority,
                        IN INT iFontLinkType,
                        IN BOOL bLoadLin)
{
    UNIMPLEMENTED;
    return FALSE;
}

ULONG
APIENTRY
NtGdiGetEudcTimeStampEx(IN OPTIONAL LPWSTR lpBaseFaceName,
                        IN ULONG cwcBaseFaceName,
                        IN BOOL bSystemTimeStamp)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtGdiGetGlyphIndicesW(IN HDC hdc,
                      IN OPTIONAL LPWSTR pwc,
                      IN INT cwc,
                      OUT OPTIONAL LPWORD pgi,
                      IN DWORD iMode)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtGdiGetGlyphIndicesWInternal(IN HDC hdc,
                              IN OPTIONAL LPWSTR pwc,
                              IN INT cwc,
                              OUT OPTIONAL LPWORD pgi,
                              IN DWORD iMode,
                              IN BOOL bSubset)
{
    UNIMPLEMENTED;
    return 0;
}

ULONG
APIENTRY
NtGdiGetGlyphOutline(IN HDC hdc,
                     IN WCHAR wch,
                     IN UINT iFormat,
                     OUT LPGLYPHMETRICS pgm,
                     IN ULONG cjBuf,
                     OUT OPTIONAL PVOID pvBuf,
                     IN LPMAT2 pmat2,
                     IN BOOL bIgnoreRotation)
{
    UNIMPLEMENTED;
    return 0;
}

ULONG
APIENTRY
NtGdiGetKerningPairs(IN HDC hdc,
                     IN ULONG cPairs,
                     OUT OPTIONAL KERNINGPAIR *pkpDst)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiGetRasterizerCaps(OUT LPRASTERIZER_STATUS praststat,
                       IN ULONG cjBytes)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
/* Missing APIENTRY! */
NtGdiGetRealizationInfo(IN HDC hdc,
                        OUT PREALIZATION_INFO pri,
                        IN HFONT hf)
{
    UNIMPLEMENTED;
    return FALSE;
}
