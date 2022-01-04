/*
 * Copyright (C) 2005 Steven Edwards
 * Copyright (C) 2005 Vijay Kiran Kamuju
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __USP10_H
#define __USP10_H

#ifndef __WINESRC__
# include <windows.h>
#endif
/* FIXME: #include <specstrings.h> */

#ifdef __cplusplus
extern "C" {
#endif

/** ScriptStringAnalyse */
#define  SSA_PASSWORD         0x00000001
#define  SSA_TAB              0x00000002
#define  SSA_CLIP             0x00000004
#define  SSA_FIT              0x00000008
#define  SSA_DZWG             0x00000010
#define  SSA_FALLBACK         0x00000020
#define  SSA_BREAK            0x00000040
#define  SSA_GLYPHS           0x00000080
#define  SSA_RTL              0x00000100
#define  SSA_GCP              0x00000200
#define  SSA_HOTKEY           0x00000400
#define  SSA_METAFILE         0x00000800
#define  SSA_LINK             0x00001000
#define  SSA_HIDEHOTKEY       0x00002000
#define  SSA_HOTKEYONLY       0x00002400
#define  SSA_FULLMEASURE      0x04000000
#define  SSA_LPKANSIFALLBACK  0x08000000
#define  SSA_PIDX             0x10000000
#define  SSA_LAYOUTRTL        0x20000000
#define  SSA_DONTGLYPH        0x40000000
#define  SSA_NOKASHIDA        0x80000000

/** StringIsComplex */
#define  SIC_COMPLEX     1
#define  SIC_ASCIIDIGIT  2
#define  SIC_NEUTRAL     4

/** ScriptGetCMap */
#define SGCM_RTL  0x00000001

/** ScriptApplyDigitSubstitution */
#define SCRIPT_DIGITSUBSTITUTE_CONTEXT      0
#define SCRIPT_DIGITSUBSTITUTE_NONE         1
#define SCRIPT_DIGITSUBSTITUTE_NATIONAL     2
#define SCRIPT_DIGITSUBSTITUTE_TRADITIONAL  3

#define SCRIPT_UNDEFINED  0

#define USP_E_SCRIPT_NOT_IN_FONT MAKE_HRESULT(SEVERITY_ERROR,FACILITY_ITF,0x200)

typedef enum tag_SCRIPT_JUSTIFY {
  SCRIPT_JUSTIFY_NONE           = 0,
  SCRIPT_JUSTIFY_ARABIC_BLANK   = 1,
  SCRIPT_JUSTIFY_CHARACTER      = 2,
  SCRIPT_JUSTIFY_RESERVED1      = 3,
  SCRIPT_JUSTIFY_BLANK          = 4,
  SCRIPT_JUSTIFY_RESERVED2      = 5,
  SCRIPT_JUSTIFY_RESERVED3      = 6,
  SCRIPT_JUSTIFY_ARABIC_NORMAL  = 7,
  SCRIPT_JUSTIFY_ARABIC_KASHIDA = 8,
  SCRIPT_JUSTIFY_ARABIC_ALEF    = 9,
  SCRIPT_JUSTIFY_ARABIC_HA      = 10,
  SCRIPT_JUSTIFY_ARABIC_RA      = 11,
  SCRIPT_JUSTIFY_ARABIC_BA      = 12,
  SCRIPT_JUSTIFY_ARABIC_BARA    = 13,
  SCRIPT_JUSTIFY_ARABIC_SEEN    = 14,
  SCRIPT_JUSTIFY_ARABIC_SEEN_M  = 15,
} SCRIPT_JUSTIFY;

typedef struct tag_SCRIPT_CONTROL {
  DWORD uDefaultLanguage	:16;
  DWORD fContextDigits		:1;
  DWORD fInvertPreBoundDir	:1;
  DWORD fInvertPostBoundDir	:1;
  DWORD fLinkStringBefore	:1;
  DWORD fLinkStringAfter	:1;
  DWORD fNeutralOverride	:1;
  DWORD fNumericOverride	:1;
  DWORD fLegacyBidiClass	:1;
  DWORD fMergeNeutralItems	:1;
  DWORD fReserved		:7;
} SCRIPT_CONTROL;

typedef struct {
  DWORD langid			:16;
  DWORD fNumeric		:1;
  DWORD fComplex		:1;
  DWORD fNeedsWordBreaking	:1;
  DWORD fNeedsCaretInfo		:1;
  DWORD bCharSet		:8;
  DWORD fControl		:1;
  DWORD fPrivateUseArea		:1;
  DWORD fNeedsCharacterJustify	:1;
  DWORD fInvalidGlyph		:1;
  DWORD fInvalidLogAttr		:1;
  DWORD fCDM			:1;
  DWORD fAmbiguousCharSet	:1;
  DWORD fClusterSizeVaries	:1;
  DWORD fRejectInvalid		:1;
} SCRIPT_PROPERTIES;

typedef struct tag_SCRIPT_STATE {
  WORD uBidiLevel		:5;
  WORD fOverrideDirection	:1;
  WORD fInhibitSymSwap		:1;
  WORD fCharShape		:1;
  WORD fDigitSubstitute		:1;
  WORD fInhibitLigate		:1;
  WORD fDisplayZWG		:1;
  WORD fArabicNumContext	:1;
  WORD fGcpClusters		:1;
  WORD fReserved		:1;
  WORD fEngineReserved		:2;
} SCRIPT_STATE;

typedef struct tag_SCRIPT_ANALYSIS {
  WORD eScript			:10;
  WORD fRTL			:1;
  WORD fLayoutRTL		:1;
  WORD fLinkBefore		:1;
  WORD fLinkAfter		:1;
  WORD fLogicalOrder		:1;
  WORD fNoGlyphIndex		:1;
  SCRIPT_STATE 	s;
} SCRIPT_ANALYSIS;

typedef struct tag_SCRIPT_ITEM {
  int iCharPos;
  SCRIPT_ANALYSIS a;
} SCRIPT_ITEM;

typedef struct tag_SCRIPT_DIGITSUBSTITUTE {
  DWORD NationalDigitLanguage		:16;
  DWORD TraditionalDigitLanguage	:16;
  DWORD DigitSubstitute			:8;
  DWORD dwReserved;
} SCRIPT_DIGITSUBSTITUTE;

typedef struct tag_SCRIPT_FONTPROPERTIES {
  int   cBytes;
  WORD wgBlank;
  WORD wgDefault;
  WORD wgInvalid;
  WORD wgKashida;
  int iKashidaWidth;
} SCRIPT_FONTPROPERTIES;

typedef struct tag_SCRIPT_TABDEF {
  int cTabStops;
  int iScale;
  int *pTabStops;
  int iTabOrigin;
} SCRIPT_TABDEF;

typedef struct tag_SCRIPT_VISATTR {
  WORD uJustification   :4;
  WORD fClusterStart    :1;
  WORD fDiacritic       :1;
  WORD fZeroWidth       :1;
  WORD fReserved        :1;
  WORD fShapeReserved   :8;
} SCRIPT_VISATTR;

typedef struct tag_SCRIPT_LOGATTR {
  BYTE    fSoftBreak      :1;
  BYTE    fWhiteSpace     :1;
  BYTE    fCharStop       :1;
  BYTE    fWordStop       :1;
  BYTE    fInvalid        :1;
  BYTE    fReserved       :3;
} SCRIPT_LOGATTR;

typedef void *SCRIPT_CACHE;
typedef void *SCRIPT_STRING_ANALYSIS;

#ifndef LSDEFS_DEFINED
typedef struct tagGOFFSET {
  LONG  du;
  LONG  dv;
} GOFFSET;
#endif

typedef ULONG OPENTYPE_TAG;

typedef struct tagOPENTYPE_FEATURE_RECORD
{
    OPENTYPE_TAG tagFeature;
    LONG         lParameter;
} OPENTYPE_FEATURE_RECORD;

typedef struct tagSCRIPT_GLYPHPROP
{
    SCRIPT_VISATTR sva;
    WORD           reserved;
} SCRIPT_GLYPHPROP;

typedef struct tagSCRIPT_CHARPROP
{
    WORD fCanGlyphAlone  :1;
    WORD reserved        :15;
} SCRIPT_CHARPROP;

typedef struct tagTEXTRANGE_PROPERTIES
{
    OPENTYPE_FEATURE_RECORD *potfRecords;
    INT                     cotfRecords;
} TEXTRANGE_PROPERTIES;

/* Function Declarations */

_Check_return_
HRESULT
WINAPI
ScriptApplyDigitSubstitution(
  _In_reads_(1) const SCRIPT_DIGITSUBSTITUTE* psds,
  _Out_writes_(1) SCRIPT_CONTROL* psc,
  _Out_writes_(1) SCRIPT_STATE* pss);

_Check_return_
HRESULT
WINAPI
ScriptApplyLogicalWidth(
  _In_reads_(cChars) const int *piDx,
  _In_ int cChars,
  _In_ int cGlyphs,
  _In_reads_(cChars) const WORD *pwLogClust,
  _In_reads_(cGlyphs) const SCRIPT_VISATTR *psva,
  _In_reads_(cGlyphs) const int *piAdvance,
  _In_reads_(1) const SCRIPT_ANALYSIS *psa,
  _Inout_updates_opt_(1) ABC *pABC,
  _Out_writes_all_(cGlyphs) int *piJustify);

_Check_return_
HRESULT
WINAPI
ScriptRecordDigitSubstitution(
  _In_ LCID Locale,
  _Out_writes_(1) SCRIPT_DIGITSUBSTITUTE *psds);

_Check_return_
HRESULT
WINAPI
ScriptItemize(
  _In_reads_(cInChars) const WCHAR *pwcInChars,
  _In_ int cInChars,
  _In_ int cMaxItems,
  _In_reads_opt_(1) const SCRIPT_CONTROL *psControl,
  _In_reads_opt_(1) const SCRIPT_STATE *psState,
  _Out_writes_to_(cMaxItems, *pcItems) SCRIPT_ITEM *pItems,
  _Out_writes_(1) int *pcItems);

_Check_return_
HRESULT
WINAPI
ScriptGetCMap(
  _In_ HDC hdc,
  _Inout_updates_(1) SCRIPT_CACHE *psc,
  _In_reads_(cChars) const WCHAR *pwcInChars,
  _In_ int cChars,
  _In_ DWORD dwFlags,
  _Out_writes_(cChars) WORD *pwOutGlyphs);

_Check_return_
HRESULT
WINAPI
ScriptGetFontProperties(
  _In_ HDC hdc,
  _Inout_updates_(1) SCRIPT_CACHE *psc,
  _Out_writes_(1) SCRIPT_FONTPROPERTIES *sfp);

_Check_return_
HRESULT
WINAPI
ScriptGetGlyphABCWidth(
  _In_ HDC hdc,
  _Inout_updates_(1) SCRIPT_CACHE *psc,
  _In_ WORD wGlyph,
  _Out_writes_(1) ABC *pABC);

_Check_return_
HRESULT
WINAPI
ScriptGetLogicalWidths(
  _In_reads_(1) const SCRIPT_ANALYSIS *psa,
  _In_ int cChars,
  _In_ int cGlyphs,
  _In_reads_(cGlyphs) const int *piGlyphWidth,
  _In_reads_(cChars) const WORD *pwLogClust,
  _In_reads_(cGlyphs) const SCRIPT_VISATTR *psva,
  _In_reads_(cChars) int *piDx);

_Check_return_
HRESULT
WINAPI
ScriptGetProperties(
  _Outptr_result_buffer_(*piNumScripts) const SCRIPT_PROPERTIES ***ppSp,
  _Out_ int *piNumScripts);

_Check_return_
HRESULT
WINAPI
ScriptStringAnalyse(
  _In_ HDC hdc,
  _In_ const void *pString,
  _In_ int cString,
  _In_ int cGlyphs,
  _In_ int iCharset,
  _In_ DWORD dwFlags,
  _In_ int iReqWidth,
  _In_reads_opt_(1) SCRIPT_CONTROL *psControl,
  _In_reads_opt_(1) SCRIPT_STATE *psState,
  _In_reads_opt_(cString) const int *piDx,
  _In_reads_opt_(1) SCRIPT_TABDEF *pTabdef,
  _In_ const BYTE *pbInClass,
  _Outptr_result_buffer_(1) SCRIPT_STRING_ANALYSIS *pssa);

_Check_return_
HRESULT
WINAPI
ScriptStringValidate(
  _In_reads_(1) SCRIPT_STRING_ANALYSIS ssa);

_Check_return_
HRESULT
WINAPI
ScriptStringFree(
  _Inout_updates_(1) SCRIPT_STRING_ANALYSIS *pssa);

_Check_return_
HRESULT
WINAPI
ScriptFreeCache(
  _Inout_updates_(1) _At_(*psc, _Post_null_) SCRIPT_CACHE *psc);

_Check_return_
HRESULT
WINAPI
ScriptIsComplex(
  _In_reads_(cInChars) const WCHAR *pwcInChars,
  _In_ int cInChars,
  _In_ DWORD dwFlags);

_Check_return_
HRESULT
WINAPI
ScriptJustify(
  _In_reads_(cGlyphs) const SCRIPT_VISATTR *psva,
  _In_reads_(cGlyphs) const int *piAdvance,
  _In_ int cGlyphs,
  _In_ int iDx,
  _In_ int iMinKashida,
  _Out_writes_all_(cGlyphs) int *piJustify);

_Check_return_
HRESULT
WINAPI
ScriptLayout(
  int cRuns,
  _In_reads_(cRuns) const BYTE *pbLevel,
  _Out_writes_all_opt_(cRuns) int *piVisualToLogical,
  _Out_writes_all_opt_(cRuns) int *piLogicalToVisual);

_Check_return_
HRESULT
WINAPI
ScriptShape(
  _In_ HDC hdc,
  _Inout_updates_(1) SCRIPT_CACHE *psc,
  _In_reads_(cChars) const WCHAR *pwcChars,
  _In_ int cChars,
  _In_ int cMaxGlyphs,
  _Inout_updates_(1) SCRIPT_ANALYSIS *psa,
  _Out_writes_to_(cMaxGlyphs, *pcGlyphs) WORD *pwOutGlyphs,
  _Out_writes_all_(cChars) WORD *pwLogClust,
  _Out_writes_to_(cMaxGlyphs, *pcGlyphs) SCRIPT_VISATTR *psva,
  _Out_writes_(1) int *pcGlyphs);

_Check_return_
HRESULT
WINAPI
ScriptPlace(
  _In_ HDC hdc,
  _Inout_updates_(1) SCRIPT_CACHE *psc,
  _In_reads_(cGlyphs) const WORD *pwGlyphs,
  _In_ int cGlyphs,
  _In_reads_(cGlyphs) const SCRIPT_VISATTR *psva,
  _Inout_updates_(1) SCRIPT_ANALYSIS *psa,
  _Out_writes_all_(cGlyphs) int *piAdvance,
  _Out_writes_all_opt_(cGlyphs) GOFFSET *pGoffset,
  _Out_writes_(1) ABC *pABC);

_Check_return_
HRESULT
WINAPI
ScriptBreak(
  _In_reads_(cChars) const WCHAR *pwcChars,
  _In_ int cChars,
  _In_reads_(1) const SCRIPT_ANALYSIS *psa,
  _Out_writes_all_(cChars) SCRIPT_LOGATTR *psla);

_Check_return_
HRESULT
WINAPI
ScriptCacheGetHeight(
  _In_ HDC hdc,
  _Inout_updates_(1) SCRIPT_CACHE *psc,
  _Out_writes_(1) LONG *tmHeight);

_Check_return_
HRESULT
WINAPI
ScriptCPtoX(
  _In_ int iCP,
  _In_ BOOL fTrailing,
  _In_ int cChars,
  _In_ int cGlyphs,
  _In_reads_(cChars) const WORD *pwLogClust,
  _In_reads_(cGlyphs) const SCRIPT_VISATTR *psva,
  _In_reads_(cGlyphs) const int *piAdvance,
  _In_reads_(1) const SCRIPT_ANALYSIS *psa,
  _Out_ int *piX);

_Check_return_
HRESULT
WINAPI
ScriptXtoCP(
  _In_ int iX,
  _In_ int cChars,
  _In_ int cGlyphs,
  _In_reads_(cChars) const WORD *pwLogClust,
  _In_reads_(cGlyphs) const SCRIPT_VISATTR *psva,
  _In_reads_(cGlyphs) const int *piAdvance,
  _In_reads_(1) const SCRIPT_ANALYSIS *psa,
  _Out_writes_(1) int *piCP,
  _Out_writes_(1) int *piTrailing);

_Check_return_
HRESULT
WINAPI
ScriptStringCPtoX(
  _In_reads_(1) SCRIPT_STRING_ANALYSIS ssa,
  _In_ int icp,
  _In_ BOOL fTrailing,
  _Out_writes_(1) int *pX);

_Check_return_
HRESULT
WINAPI
ScriptStringXtoCP(
  _In_reads_(1) SCRIPT_STRING_ANALYSIS ssa,
  _In_ int iX,
  _Out_writes_(1) int *piCh,
  _Out_writes_(1) int *piTrailing);

_Check_return_
HRESULT
WINAPI
ScriptStringGetLogicalWidths(
  _In_reads_(1) SCRIPT_STRING_ANALYSIS ssa,
  _Out_ int *piDx);

_Check_return_
HRESULT
WINAPI
ScriptStringGetOrder(
  _In_reads_(1) SCRIPT_STRING_ANALYSIS ssa,
  _Out_ UINT *puOrder);

_Check_return_
HRESULT
WINAPI
ScriptStringOut(
  _In_reads_(1) SCRIPT_STRING_ANALYSIS ssa,
  _In_ int iX,
  _In_ int iY,
  _In_ UINT uOptions,
  _In_reads_opt_(1) const RECT *prc,
  _In_ int iMinSel,
  _In_ int iMaxSel,
  _In_ BOOL fDisabled);

_Check_return_
HRESULT
WINAPI
ScriptTextOut(
  _In_ const HDC hdc,
  _Inout_updates_(1) SCRIPT_CACHE *psc,
  _In_ int x,
  _In_ int y,
  _In_ UINT fuOptions,
  _In_reads_opt_(1) const RECT *lprc,
  _In_reads_(1) const SCRIPT_ANALYSIS *psa,
  _Reserved_ const WCHAR *pwcReserved,
  _Reserved_ int iReserved,
  _In_reads_(cGlyphs) const WORD *pwGlyphs,
  _In_ int cGlyphs,
  _In_reads_(cGlyphs) const int *piAdvance,
  _In_reads_opt_(cGlyphs) const int *piJustify,
  _In_reads_(cGlyphs) const GOFFSET *pGoffset);

const int*
WINAPI
ScriptString_pcOutChars(
  _In_reads_(1) SCRIPT_STRING_ANALYSIS ssa);

const SCRIPT_LOGATTR*
WINAPI
ScriptString_pLogAttr(
  _In_reads_(1) SCRIPT_STRING_ANALYSIS ssa);

const SIZE*
WINAPI
ScriptString_pSize(
  _In_reads_(1) SCRIPT_STRING_ANALYSIS ssa);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __USP10_H */
