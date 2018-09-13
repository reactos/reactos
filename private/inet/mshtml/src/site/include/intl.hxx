//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       intl.hxx
//
//  Contents:   Internationalization Support Functions
//
//----------------------------------------------------------------------------

#ifndef I_INTL_HXX_
#define I_INTL_HXX_
#pragma INCMSG("--- Beg 'intl.hxx'")

#ifndef X_CODEPAGE_H_
#define X_CODEPAGE_H_
#include "codepage.h"
#endif

#ifndef X_MLANG_H_
#define X_MLANG_H_
#pragma INCMSG("--- Beg <mlang.h>")
#include <mlang.h>
#pragma INCMSG("--- End <mlang.h>")
#endif

// define some other languages until ntdefs.h catches up
#ifndef LANG_KASHMIRI
#define LANG_KASHMIRI     0x60       
#endif
#ifndef LANG_MANIPURI
#define LANG_MANIPURI     0x58       
#endif
#ifndef LANG_NAPALI
#define LANG_NAPALI       0x61       
#endif
#ifndef LANG_SINDHI
#define LANG_SINDHI       0x59
#endif
#ifndef LANG_YIDDISH
#define LANG_YIDDISH      0x3d       
#endif

class CDoc;

HRESULT    EnsureMultiLanguage(void);
HRESULT    QuickMimeGetCodePageInfo(CODEPAGE cp, PMIMECPINFO pMimeCpInfo);
HRESULT    QuickMimeGetCSetInfo(LPCTSTR lpszCharset, PMIMECSETINFO pMimeCharsetInfo);
HRESULT    SlowMimeGetCodePageInfo(CODEPAGE cp, PMIMECPINFO pMimeCpInfo);
HMENU      CreateMimeCSetMenu(CODEPAGE cp, BOOL fDocRTL);
HMENU      CreateDocDirMenu(BOOL fDocRTL, HMENU hmenuTarget = NULL);
HMENU      GetOrAppendDocDirMenu(CODEPAGE codepage, BOOL fDocRTL,
                                 HMENU hmenuTarget = NULL);
HRESULT    ShowMimeCSetMenu(OPTIONSETTINGS *pOS, int * pnIdm, CODEPAGE codepage, LPARAM lParam, 
                            BOOL fDocRTL, BOOL fAutoMode);
HRESULT    ShowFontSizeMenu(int * pnIdm, short sFontSize, LPARAM lParam);
HMENU      GetEncodingMenu(OPTIONSETTINGS *pOS, CODEPAGE codepage, BOOL fDocRTL, BOOL fAutoMode);
HMENU      GetFontSizeMenu(short sFontSize);
HRESULT    GetCodePageFromMlangString(LPCTSTR mlangString, CODEPAGE* pCodePage);
HRESULT    GetMlangStringFromCodePage(CODEPAGE codepage, LPTSTR pMlangString,
                                      size_t cchMlangString);
CODEPAGE   GetCodePageFromMenuID(int nIdm);

UINT       DefaultCodePageFromCharSet( BYTE bCharSet, CODEPAGE cpDoc, LCID lcid );
HRESULT    DefaultFontInfoFromCodePage( CODEPAGE cp, LOGFONT * lplf );
CODEPAGE   CodePageFromString( TCHAR * pchArg, BOOL fLookForWordCharset );
LCID       LCIDFromString( TCHAR * pchArg );

UINT       WindowsCodePageFromCodePage( CODEPAGE cp );
HRESULT    MlangEnumCodePages(DWORD grfFlags, IEnumCodePage **ppEnumCodePage);
HRESULT    MlangValidateCodePage(CDoc *pDoc, CODEPAGE cp, HWND hwnd, BOOL fUserSelect);

HRESULT    MlangGetDefaultFont( SCRIPT_ID sid, SCRIPTINFO * psi );

// inlines ------------------------------------------------------------------

inline CODEPAGE
CodePageFromAlias( LPCTSTR lpcszAlias )
{
    CODEPAGE cp;
    IGNORE_HR(GetCodePageFromMlangString(lpcszAlias, &cp));
    return cp;
}

inline BOOL
IsAutodetectCodePage( CODEPAGE cp )
{
    return cp == CP_AUTO_JP || cp == CP_AUTO;
}

inline BOOL
IsStraightToUnicodeCodePage( CODEPAGE cp )
{
    // CP_UCS_2_BIGENDIAN is not correctly handled by MLANG, so we handle it.

    return cp == CP_UCS_2 || cp == CP_UTF_8 || cp == CP_UTF_7 || cp == CP_UCS_2_BIGENDIAN;
}

inline CODEPAGE
NavigatableCodePage( CODEPAGE cp )
{
    return (cp == CP_UCS_2 || cp == CP_UCS_2_BIGENDIAN) ? CP_UTF_8 : cp;
}

inline BOOL
IsKoreanSelectionMode()
{
#if DBG==1
    ExternTag(tagKoreanSelection);

    return g_cpDefault == CP_KOR_5601 || IsTagEnabled(tagKoreanSelection);
#else    
    return g_cpDefault == CP_KOR_5601;
#endif
}

BYTE WindowsCharsetFromCodePage( CODEPAGE cp );

inline BOOL
IsExtTextOutWBuggy( UINT codepage )
{
    // g_fExtTextOutWBuggy implies that the system call has problems.  In the
    // case of TC/PRC, many glyphs render incorrectly
    return    g_fExtTextOutWBuggy
           && (   codepage == CP_UCS_2
               || codepage == CP_TWN
               || codepage == CP_CHN_GB);
}

// **************************************************************************
// NB (cthrash) From RichEdit (_uwrap/unicwrap) start {

#ifdef MACPORT
  #if lidSerbianCyrillic != 0xc1a
    #error "lidSerbianCyrillic macro value has changed"
  #endif // lidSerbianCyrillic
#else
  #define lidSerbianCyrillic 0xc1a
#endif // MACPORT

#define IN_RANGE(n1, b, n2) ((unsigned)((b) - n1) <= n2 - n1)

// index returned by CharSetIndexFromChar()
enum CHARSETINDEX {
    ANSI_INDEX = 0,
    ARABIC_INDEX = 17,
    GREEK_INDEX = 13,
    HAN_INDEX = -2,
    HANGUL_INDEX = 10,
    HEBREW_INDEX = 6,
    RUSSIAN_INDEX = 8,
    SHIFTJIS_INDEX = 7,
    THAI_INDEX = 16,
    UNKNOWN_INDEX = -1
};

CHARSETINDEX CharSetIndexFromChar(TCHAR ch);
BOOL CheckDBCInUnicodeStr(TCHAR *ptext);

UINT ConvertLanguageIDtoCodePage(WORD lid);
BOOL IsFELCID(LCID lcid);
BOOL IsFECharset(BYTE bCharSet);
INT  In125x(WCHAR ch, BYTE bCharSet);
UINT GetKeyboardCodePage();
LCID GetKeyboardLCID();
UINT GetLocaleCodePage();
LCID GetLocaleLCID();
BOOL IsNarrowCharSet(BYTE bCharSet);

// COMPLEXSCRIPT
BOOL IsRtlLCID(LCID lcid);
BOOL IsRTLLang(LANGID lang);


/*
 *  IsInArabicBlock(LANGID lang)
 *
 *  @func
 *      Used to determine if a language is in the Arabic block
 *
 *  @rdesc
 *      TRUE if in the Arabic block.
 */
inline BOOL IsInArabicBlock(LANGID langid)
{
    BOOL fInArabicBock = FALSE;

    switch (langid)
    {
        case LANG_ARABIC:
        case LANG_URDU:
        case LANG_FARSI:
        case LANG_SINDHI:
        case LANG_KASHMIRI:
            fInArabicBock = TRUE;
            break;
    }

    return fInArabicBock;    
}

/*
 *  IsRTLCodepage(cp)
 *
 *  @func
 *      Used to determine if a codepage should be considered for RTL menu items.
 *
 *  @rdesc
 *      TRUE if the codepage might have RTL document layout.
 */
inline BOOL IsRTLCodepage(CODEPAGE cp)
{
    BOOL fIsRTL = FALSE;

    switch(cp)
    {
        case CP_UTF_7:           // unicode
        case CP_UTF_8:
        case CP_UCS_2:
        case CP_UCS_2_BIGENDIAN:
        case CP_UCS_4:
        case CP_UCS_4_BIGENDIAN:
        case CP_1255:            // hebrew
        case CP_HEB_862:
        // case CP_ISO_8859_8: Visual hebrew is explicitly LTR
        case CP_ISO_8859_8_I:
        case CP_1256:            // arabic
        case CP_ASMO_708: 
        case CP_ASMO_720:
        case CP_ISO_8859_6:
            fIsRTL = TRUE;
            break;
    }

    return fIsRTL;    
}

/*
 *  IsArabicCodepage(cp)
 *
 *  @func
 *      Used to determine if a codepage is Arabic.
 *
 *  @rdesc
 *      TRUE if the codepage is Arabic.
 */
inline BOOL IsArabicCodepage(CODEPAGE cp)
{
    BOOL fIsArabic = FALSE;

    switch(cp)
    {
        case CP_1256:            // arabic
        case CP_ASMO_708: 
        case CP_ASMO_720:
        case CP_ISO_8859_6:
            fIsArabic = TRUE;
            break;
    }
    return fIsArabic;    
}

/*
 *  IsRTLCharCore(ch)
 *
 *  @func
 *      Used to determine if character is a right-to-left character.
 *
 *  @rdesc
 *      TRUE if the character is used in a right-to-left language.
 *
 *
 */
inline BOOL
IsRTLCharCore(
    TCHAR ch)
{
    // Bitmask of RLM, RLE, RLO, based from RLM in the 0 bit.
    #define MASK_RTLPUNCT 0x90000001

    return ( IN_RANGE(0x0590, ch, 0x08ff) ||            // In RTL block
             ( IN_RANGE(0x200f, ch, 0x202e) &&          // Possible RTL punct char
               ((MASK_RTLPUNCT >> (ch - 0x200f)) & 1)   // Mask of RTL punct chars
             )
           );
}

/*
 *  IsRTLChar(ch)
 *
 *  @func
 *      Used to determine if character is a right-to-left character.
 *
 *  @rdesc
 *      TRUE if the character is used in a right-to-left language.
 *
 *
 */
inline BOOL
IsRTLChar(
    TCHAR ch)
{
    return ( IN_RANGE(0x0590 /* First RTL char */, ch, 0x202e /* RLO */) &&
             IsRTLCharCore(ch) );
}

/*
/*
 *  IsAlef(ch)
 *
 *  @func
 *      Used to determine if base character is a Arabic-type Alef.
 *
 *  @rdesc
 *      TRUE iff the base character is an Arabic-type Alef.
 *
 *  @comm
 *      AlefWithMaddaAbove, AlefWithHamzaAbove, AlefWithHamzaBelow,
 *      and Alef are valid matches.
 *
 */
inline BOOL IsAlef(TCHAR ch)
{
    if(InRange(ch, 0x0622, 0x0675))
    {
        return ((InRange(ch, 0x0622, 0x0627) &&
                   (ch != 0x0624 && ch != 0x0626)) ||
                (InRange(ch, 0x0671, 0x0675) &&
                   (ch != 0x0674)));
    }
    return FALSE;
}

/*
 *  IsThaiTypeChar(ch)
 *
 *  @func   Used to determine if character is a Thai type script character.
 *
 *  @rdesc  TRUE iff character is a Thai character
 *
 */
inline BOOL IsThaiTypeChar(TCHAR ch)
{
    // NOTE: This includes Thai, Lao and Khmer 
    //       -- no wordbreak characters between words
    return (InRange(ch, 0x0E00, 0x0EFF) || 
            InRange(ch, 0x1400, 0x147F));
}

/*
 *  IsNotThaiTypeChar(ch)
 *
 *  @func   Used to determine if character is a Thai type script character.
 *
 *  @rdesc  TRUE iff character is not a Thai character used to optimize ASCII cases
 *
 */
inline BOOL IsNotThaiTypeChar(TCHAR ch)
{
    // NOTE: This excludes Thai, Lao and Khmer 
    //       -- no wordbreak characters between words
    return (ch < 0x0E00 ||
            ch > 0x147F || 
            IN_RANGE(0x0EFF, ch, 0x1400));
}

/*
 *  IsClusterTypeChar(ch)
 *
 *  @func   Used to determine if character is a cluster type character.
 *
 *  @rdesc  TRUE iff character is a cluster type character
 *
 *  @comm   Requires special handling to navigate from cluster to cluster
 */
inline BOOL IsClusterTypeChar(TCHAR ch)
{
    // NOTE: This includes Indic, Thai, Lao; Other langauges normally increment partially
    //       across clusters. We want to include space types of characters here.
    return (InRange(ch, 0x0900, 0x0EFF) || 
            InRange(ch, 0x1400, 0x147F));
}

/*
 *  IsNotClusterTypeChar(ch)
 *
 *  @func   Used to determine if character is NOT a cluster type character.
 *
 *  @rdesc  TRUE iff character is not a cluster type character
 *
 *  @comm   Required for special handling to navigate from cluster to cluster
 */
inline BOOL IsNotClusterTypeChar(TCHAR ch)
{
    // NOTE: This excludes Indic, Thai, Lao; 
    //       Other langauges normally increment partially across clusters
    return (ch < 0x0900 ||
            ch > 0x147F || 
            InRange(ch, 0x0EFF, 0x1400));
}

// (_uwrap) end }
// **************************************************************************

class CIntlFont
{
public:
    CIntlFont( HDC hdc, CODEPAGE codepage, LCID lcid, SHORT sBaselineFont, const TCHAR * pch );
    ~CIntlFont();

private:
    HDC     _hdc;
    HFONT   _hFont;
    HFONT   _hOldFont;
    BOOL    _fIsStock;
};

BOOL CommCtrlNativeFontSupport();

// class CCachedCPInfo - used as a codepage cache for 'encoding' menu
    
// CPCACHE typedef has to be outside of CCachedCPInfo, because
// we initialize the cache outside the class
typedef struct {
    UINT cp;
    ULONG  ulIdx;
    int  cUsed;
} CPCACHE;

class CCachedCPInfo 
{
public:
    static void InitCpCache (OPTIONSETTINGS *pOS, PMIMECPINFO pcp, ULONG ccp);
    static void SaveCodePage (UINT codepage, PMIMECPINFO pcp, ULONG ccp);

    static UINT GetCodePage(int idx)
    {
        return idx < ARRAY_SIZE(_CpCache) ? _CpCache[idx].cp: 0;
    }
    static ULONG GetCcp()
    {
        return _ccpInfo;
    }

    static ULONG GetMenuIdx(int idx)
    {
        return idx < ARRAY_SIZE(_CpCache) ? _CpCache[idx].ulIdx: 0;
    } 
    static void SaveSetting();
    static void LoadSetting(OPTIONSETTINGS *pOS);

 private:
    static void RemoveInvalidCp(void);
    static ULONG _ccpInfo;
    static CPCACHE _CpCache[5];
    static BOOL _fCacheLoaded;
    static BOOL _fCacheChanged;
    static LPTSTR _pchRegKey;
};

extern const TCHAR s_szPathInternational[];
#pragma INCMSG("--- End 'intl.hxx'")
#else
#pragma INCMSG("*** Dup 'intl.hxx'")
#endif
