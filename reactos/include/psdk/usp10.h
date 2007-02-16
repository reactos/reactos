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
  SCRIPT_JUSTIFY_RESERVED4      = 15,
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
  DWORD fReserved		:8;
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

/* Function Declarations */

HRESULT WINAPI ScriptApplyDigitSubstitution(const SCRIPT_DIGITSUBSTITUTE* psds, 
                                            SCRIPT_CONTROL* psc, SCRIPT_STATE* pss);
HRESULT WINAPI ScriptApplyLogicalWidth(const int *piDx, int cChars, int cGlyphs, const WORD *pwLogClust,
                                       const SCRIPT_VISATTR *psva, const int *piAdvance,
                                       const SCRIPT_ANALYSIS *psa, ABC *pABC, int *piJustify);
HRESULT WINAPI ScriptRecordDigitSubstitution(LCID Locale, SCRIPT_DIGITSUBSTITUTE *psds);
HRESULT WINAPI ScriptItemize(const WCHAR *pwcInChars, int cInChars, int cMaxItems, 
                             const SCRIPT_CONTROL *psControl, const SCRIPT_STATE *psState, 
                             SCRIPT_ITEM *pItems, int *pcItems);
HRESULT WINAPI ScriptGetCMap(HDC hdc, SCRIPT_CACHE *psc, const WCHAR *pwcInChars, int cChars,
                             DWORD dwFlags, WORD *pwOutGlyphs);
HRESULT WINAPI ScriptGetFontProperties(HDC hdc, SCRIPT_CACHE *psc, SCRIPT_FONTPROPERTIES *sfp);
HRESULT WINAPI ScriptGetGlyphABCWidth(HDC hdc, SCRIPT_CACHE *psc, WORD wGlyph, ABC *pABC);
HRESULT WINAPI ScriptGetLogicalWidths(const SCRIPT_ANALYSIS *psa, int cChars, int cGlyphs,
                                      const int *piGlyphWidth, const WORD *pwLogClust,
                                      const SCRIPT_VISATTR *psva, int *piDx);
HRESULT WINAPI ScriptGetProperties(const SCRIPT_PROPERTIES ***ppSp, int *piNumScripts);
HRESULT WINAPI ScriptStringAnalyse(HDC hdc, 
				   const void *pString, 
				   int cString, 
				   int cGlyphs,
				   int iCharset,
				   DWORD dwFlags,
				   int iReqWidth,
				   SCRIPT_CONTROL *psControl,
				   SCRIPT_STATE *psState,
				   const int *piDx,
				   SCRIPT_TABDEF *pTabdef,
				   const BYTE *pbInClass,
				   SCRIPT_STRING_ANALYSIS *pssa);
HRESULT WINAPI ScriptStringValidate(SCRIPT_STRING_ANALYSIS ssa);
HRESULT WINAPI ScriptStringFree(SCRIPT_STRING_ANALYSIS *pssa);
HRESULT WINAPI ScriptFreeCache(SCRIPT_CACHE *psc);
HRESULT WINAPI ScriptIsComplex(const WCHAR* pwcInChars, int cInChars, DWORD dwFlags);
HRESULT WINAPI ScriptJustify(const SCRIPT_VISATTR *psva, const int *piAdvance, int cGlyphs,
                             int iDx, int iMinKashida, int *piJustify);
HRESULT WINAPI ScriptLayout(int cRuns, const BYTE *pbLevel, int *piVisualToLogical, int *piLogicalToVisual);
HRESULT WINAPI ScriptShape(HDC hdc, SCRIPT_CACHE *psc, const WCHAR *pwcChars, int cChars, int cMaxGlyphs,
                           SCRIPT_ANALYSIS *psa, WORD *pwOutGlyphs, WORD *pwLogClust, SCRIPT_VISATTR *psva, int *pcGlyphs);
HRESULT WINAPI ScriptPlace(HDC hdc, SCRIPT_CACHE *psc, const WORD *pwGlyphs, int cGlyphs, const SCRIPT_VISATTR *psva,
                           SCRIPT_ANALYSIS *psa, int *piAdvance, GOFFSET *pGoffset, ABC *pABC );
HRESULT WINAPI ScriptBreak(const WCHAR *pwcChars, int cChars, const SCRIPT_ANALYSIS *psa, SCRIPT_LOGATTR *psla);
HRESULT WINAPI ScriptCacheGetHeight(HDC hdc, SCRIPT_CACHE *psc, LONG *tmHeight);
HRESULT WINAPI ScriptCPtoX(int iCP, BOOL fTrailing, int cChars, int cGlyphs, const WORD *pwLogClust, const SCRIPT_VISATTR *psva,
                           const int *piAdvance, const SCRIPT_ANALYSIS *psa, int *piX);
HRESULT WINAPI ScriptXtoCP(int iX, int cChars, int cGlyphs, const WORD *pwLogClust, const SCRIPT_VISATTR *psva,
                           const int *piAdvance, const SCRIPT_ANALYSIS *psa, int *piCP, int *piTrailing);
HRESULT WINAPI ScriptStringCPtoX(SCRIPT_STRING_ANALYSIS ssa, int icp, BOOL fTrailing, int *pX);
HRESULT WINAPI ScriptStringXtoCP(SCRIPT_STRING_ANALYSIS ssa, int iX, int *piCh , int *piTrailing);
HRESULT WINAPI ScriptStringGetLogicalWidths(SCRIPT_STRING_ANALYSIS ssa, int *piDx);
HRESULT WINAPI ScriptStringGetOrder(SCRIPT_STRING_ANALYSIS ssa, UINT *puOrder);
HRESULT WINAPI ScriptStringOut(SCRIPT_STRING_ANALYSIS ssa, int iX, int iY, UINT uOptions, const RECT *prc,
                               int iMinSel, int iMaxSel, BOOL fDisabled);
HRESULT WINAPI ScriptTextOut(const HDC hdc, SCRIPT_CACHE *psc, int x, int y, UINT fuOptions, const RECT *lprc,
                             const SCRIPT_ANALYSIS *psa, const WCHAR *pwcReserved, int iReserved, const WORD *pwGlyphs,
                             int cGlyphs, const int *piAdvance, const int *piJustify, const GOFFSET *pGoffset);
const int* WINAPI ScriptString_pcOutChars(SCRIPT_STRING_ANALYSIS ssa);
const SCRIPT_LOGATTR* WINAPI ScriptString_pLogAttr(SCRIPT_STRING_ANALYSIS ssa);
const SIZE* WINAPI ScriptString_pSize(SCRIPT_STRING_ANALYSIS ssa);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __USP10_H */
