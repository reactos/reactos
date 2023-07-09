#ifndef I__FONTLNK_H_
#define I__FONTLNK_H_
#pragma INCMSG("--- Beg '_fontlnk.h'")

class CCcs;
class COneRun;

// HACK (cthrash) We do not ever font link for SYMBOL_CHARSET fonts.  This
// makes the FS_SYMBOL bit useless for us.  What we'd like to do, is to
// distinguish the ASCII portion from FS_LATIN1, since basically all fonts
// claim to have FS_LATIN1 when in reality many of them only contain the
// ASCII portion of Latin-1.  So here's the hack - all fonts have SBITS_ASCII
// set.  This simplifies the loop in CRenderer.

#define SBITS_LATIN1               FS_LATIN1
#define SBITS_LATIN2               FS_LATIN2
#define SBITS_CYRILLIC             FS_CYRILLIC
#define SBITS_GREEK                FS_GREEK
#define SBITS_TURKISH              FS_TURKISH
#define SBITS_HEBREW               FS_HEBREW
#define SBITS_ARABIC               FS_ARABIC
#define SBITS_BALTIC               FS_BALTIC
#define SBITS_JISJAPAN             FS_JISJAPAN
#define SBITS_CHINESESIMP          FS_CHINESESIMP
#define SBITS_WANSUNG              FS_WANSUNG
#define SBITS_CHINESETRAD          FS_CHINESETRAD
#define SBITS_ASCII                FS_SYMBOL       // <- see comment above
#define SBITS_SURROGATE_A          0x20000000L
#define SBITS_SURROGATE_B          0x40000000L

// For symbol fonts we want to assume they can handle everything.
#define SBITS_ALLLANGS DWORD(-1)

// FontLinkTextOut uMode values
#define FLTO_BOTH           0
#define FLTO_TEXTEXTONLY    1
#define FLTO_TEXTOUTONLY    2

#define CHUNKSIZE 32

DWORD GetFontScriptBits(HDC hdc, const TCHAR *szFaceName, LOGFONT *plf);
DWORD GetLangBits(IMLangFontLink *pMLangFontLink, WCHAR wc);
void DeinitFontLinking(THREADSTATE * pts);

BOOL NeedsFontLinking(HDC hdc, CCcs * pccs, LPCTSTR pch, int cch, CDoc *pDoc);
void DrawUnderlineStrikeOut(int x, int y, int iLength, HDC hDC, HFONT hFont, const GDIRECT *prc);
void VanillaTextOut(CCcs *pccs, HDC hdc, int x, int y, UINT fuOptions, const GDIRECT *prc, LPCTSTR pString, UINT cch, UINT uCodePage, int *piDx);
int FontLinkTextOut(HDC hdc, int x, int y, UINT fuOptions, const GDIRECT *prc, LPCTSTR pString, UINT cch, CDocInfo *pdci, const CCharFormat *pCF, UINT uMode);

BOOL SelectScriptAppropriateFont( SCRIPT_ID sid, BYTE bCharSet, CDoc * pDoc, CCharFormat * pcf );
BOOL ScriptAppropriateFaceNameAtom( SCRIPT_ID sid, CDoc * pDoc, BOOL fFixed, LONG * platmFontFace );

void ForceScriptIdOnUserSpecifiedFont( OPTIONSETTINGS * pOS, SCRIPT_ID sid );

#pragma INCMSG("--- End '_fontlnk.h'")
#else
#pragma INCMSG("*** Dup '_fontlnk.h'")
#endif
