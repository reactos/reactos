/*
 *  @doc    INTERNAL
 *
 *  @module LSCONST.CXX -- line services constants
 *
 *
 *  Owner: <nl>
 *      Chris Thrasher <nl>
 *      Sujal Parikh <nl>
 *
 *  History: <nl>
 *      1/29/97     cthrash created
 *
 *  Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */

#include "headers.hxx"

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_LINESRV_HXX_
#define X_LINESRV_HXX_
#include "linesrv.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_RUBY_H_
#define X_RUBY_H_
#include <ruby.h>
#endif

#ifndef X_HIH_H_
#define X_HIH_H_
#include <hih.h>
#endif

#ifndef X_TATENAK_H_
#define X_TATENAK_H_
#include <tatenak.h>
#endif

#ifndef X_WARICHU_H_
#define X_WARICHU_H_
#include <warichu.h>
#endif

#ifndef X_ROBJ_H_
#define X_ROBJ_H_
#include <robj.h>
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include <wchdefs.h>
#endif

#ifndef X_LSM_HXX_
#define X_LSM_HXX_
#include "lsm.hxx"
#endif

#ifdef _MAC
CLineServices::LSCBK CLineServices::s_lscbk =
#else
const CLineServices::LSCBK CLineServices::s_lscbk =
#endif
{
    CLineServices::NewPtr,
    CLineServices::DisposePtr,
    CLineServices::ReallocPtr,
    CLineServices::FetchRun,
    CLineServices::GetAutoNumberInfo,
    CLineServices::GetNumericSeparators,
    CLineServices::CheckForDigit,
    CLineServices::FetchPap,
    CLineServices::FetchTabs,
    CLineServices::GetBreakThroughTab,
    CLineServices::FGetLastLineJustification,
    CLineServices::CheckParaBoundaries,
    CLineServices::GetRunCharWidths,
    CLineServices::CheckRunKernability,
    CLineServices::GetRunCharKerning,
    CLineServices::GetRunTextMetrics,
    CLineServices::GetRunUnderlineInfo,
    CLineServices::GetRunStrikethroughInfo,
    CLineServices::GetBorderInfo,
    CLineServices::ReleaseRun,
    CLineServices::Hyphenate,
    CLineServices::GetHyphenInfo,
    CLineServices::DrawUnderline,
    CLineServices::DrawStrikethrough,
    CLineServices::DrawBorder,
    CLineServices::DrawUnderlineAsText,
    CLineServices::FInterruptUnderline,
    CLineServices::FInterruptShade,
    CLineServices::FInterruptBorder,
    CLineServices::ShadeRectangle,
    CLineServices::DrawTextRun,
    CLineServices::DrawSplatLine,
    CLineServices::FInterruptShaping,
    CLineServices::GetGlyphs,
    CLineServices::GetGlyphPositions,
    CLineServices::ResetRunContents,
    CLineServices::DrawGlyphs,
    CLineServices::GetGlyphExpansionInfo,
    CLineServices::GetGlyphExpansionInkInfo,
    CLineServices::GetEms,
    CLineServices::PunctStartLine,
    CLineServices::ModWidthOnRun,
    CLineServices::ModWidthSpace,
    CLineServices::CompOnRun,
    CLineServices::CompWidthSpace,
    CLineServices::ExpOnRun,
    CLineServices::ExpWidthSpace,
    CLineServices::GetModWidthClasses,
    CLineServices::GetBreakingClasses,
    CLineServices::FTruncateBefore,
    CLineServices::CanBreakBeforeChar,
    CLineServices::CanBreakAfterChar,
    CLineServices::FHangingPunct,
    CLineServices::GetSnapGrid,
    CLineServices::DrawEffects,
    CLineServices::FCancelHangingPunct,
    CLineServices::ModifyCompAtLastChar,
    CLineServices::EnumText,
    CLineServices::EnumTab,
    CLineServices::EnumPen,
    CLineServices::GetObjectHandlerInfo,
    CLineServices::AssertFailed
};


WHEN_DBG( int CLineServices::s_nSerialMax=0; )

const struct lstxtcfg CLineServices::s_lstxtcfg =
{
    LS_AVG_CHARS_PER_LINE,              // cEstimatedCharsPerLine; modify as necessary
    WCH_UNDEF,
    WCH_NULL,
    WCH_SPACE,
    WCH_HYPHEN,
    WCH_TAB,
    WCH_ENDPARA1,
    WCH_ENDPARA2,
    WCH_ALTENDPARA,
    WCH_SYNTHETICLINEBREAK,
    WCH_COLUMNBREAK,
    WCH_SECTIONBREAK,
    WCH_PAGEBREAK,
    WCH_UNDEF,                          // NB (cthrash) Don't let LS default-handle NONBREAKSPACE
    WCH_NONBREAKHYPHEN,
    WCH_NONREQHYPHEN,
    WCH_EMDASH,
    WCH_ENDASH,
    WCH_EMSPACE,
    WCH_ENSPACE,
    WCH_NARROWSPACE,
    WCH_OPTBREAK,
    WCH_NOBREAK,
    WCH_FESPACE,
    WCH_ZWJ,
    WCH_ZWNJ,
    WCH_TOREPLACE,
    WCH_REPLACE,
    WCH_VISINULL,
    WCH_VISIALTENDPARA,
    WCH_VISIENDLINEINPARA,
    WCH_VISIENDPARA,
    WCH_VISISPACE,
    WCH_VISINONBREAKSPACE,
    WCH_VISINONBREAKHYPHEN,
    WCH_VISINONREQHYPHEN,
    WCH_VISITAB,
    WCH_VISIEMSPACE,
    WCH_VISIENSPACE,
    WCH_VISINARROWSPACE,
    WCH_VISIOPTBREAK,
    WCH_VISINOBREAK,
    WCH_VISIFESPACE,
    WCH_ESCANMRUN,
};

CLineServices::LSIMETHODS
CLineServices::s_rgLsiMethods[CLineServices::LSOBJID_COUNT] =
{
    //
    // The order of these is determined by the order of 
    // enum LSOBJID
    //

    // LSOBJID_EMBEDDED
    {
        CLineServices::CreateILSObj,
        CEmbeddedILSObj::DestroyILSObj,
        CEmbeddedILSObj::SetDoc,
        CEmbeddedILSObj::CreateLNObj,
        CEmbeddedILSObj::DestroyLNObj,
        (LSERR (WINAPI CILSObjBase::*)(PCFMTIN, FMTRES*))CEmbeddedILSObj::Fmt,
        (LSERR (WINAPI CILSObjBase::*)(BREAKREC*, DWORD, PCFMTIN, FMTRES*))CEmbeddedILSObj::FmtResume,
        CEmbeddedDobj::GetModWidthPrecedingChar,
        CEmbeddedDobj::GetModWidthFollowingChar,
        CLineServices::TruncateChunk,
        CEmbeddedDobj::FindPrevBreakChunk,
        CEmbeddedDobj::FindNextBreakChunk,
        CLineServices::ForceBreakChunk,
        CEmbeddedDobj::SetBreak,  
        CEmbeddedDobj::GetSpecialEffectsInside,
        CEmbeddedDobj::FExpandWithPrecedingChar,
        CEmbeddedDobj::FExpandWithFollowingChar,
        CEmbeddedDobj::CalcPresentation,  
        CEmbeddedDobj::QueryPointPcp,
        CEmbeddedDobj::QueryCpPpoint,
        CEmbeddedDobj::Enum,
        CEmbeddedDobj::Display,
        CEmbeddedDobj::DestroyDObj
    },

    // LSOBJID_NOBR
    {
        CLineServices::CreateILSObj,
        CNobrILSObj::DestroyILSObj,
        CNobrILSObj::SetDoc,
        CNobrILSObj::CreateLNObj,
        CNobrILSObj::DestroyLNObj,
        (LSERR (WINAPI CILSObjBase::*)(PCFMTIN, FMTRES*))CNobrILSObj::Fmt,
        (LSERR (WINAPI CILSObjBase::*)(BREAKREC*, DWORD, PCFMTIN, FMTRES*))CNobrILSObj::FmtResume,
        CNobrDobj::GetModWidthPrecedingChar,
        CNobrDobj::GetModWidthFollowingChar,
        CLineServices::TruncateChunk,
        CNobrDobj::FindPrevBreakChunk,
        CNobrDobj::FindNextBreakChunk,
        CLineServices::ForceBreakChunk,
        CNobrDobj::SetBreak,  
        CNobrDobj::GetSpecialEffectsInside,
        CNobrDobj::FExpandWithPrecedingChar,
        CNobrDobj::FExpandWithFollowingChar,
        CNobrDobj::CalcPresentation,  
        CNobrDobj::QueryPointPcp,  
        CNobrDobj::QueryCpPpoint,
        CNobrDobj::Enum,
        CNobrDobj::Display,
        CNobrDobj::DestroyDObj
    },

    // LSOBJID_GLYPH
    {
        CLineServices::CreateILSObj,
        CGlyphILSObj::DestroyILSObj,
        CGlyphILSObj::SetDoc,
        CGlyphILSObj::CreateLNObj,
        CGlyphILSObj::DestroyLNObj,
        (LSERR (WINAPI CILSObjBase::*)(PCFMTIN, FMTRES*))CGlyphILSObj::Fmt,
        (LSERR (WINAPI CILSObjBase::*)(BREAKREC*, DWORD, PCFMTIN, FMTRES*))CGlyphILSObj::FmtResume,
        CGlyphDobj::GetModWidthPrecedingChar,
        CGlyphDobj::GetModWidthFollowingChar,
        CLineServices::TruncateChunk,
        CGlyphDobj::FindPrevBreakChunk,
        CGlyphDobj::FindNextBreakChunk,
        CLineServices::ForceBreakChunk,
        CGlyphDobj::SetBreak,  
        CGlyphDobj::GetSpecialEffectsInside,
        CGlyphDobj::FExpandWithPrecedingChar,
        CGlyphDobj::FExpandWithFollowingChar,
        CGlyphDobj::CalcPresentation,  
        CGlyphDobj::QueryPointPcp,  
        CGlyphDobj::QueryCpPpoint,
        CGlyphDobj::Enum,
        CGlyphDobj::Display,
        CGlyphDobj::DestroyDObj
    },

    // The remainder is populated by LineServices.
};

#if defined(UNIX) || defined(_MAC)
// UNIX uses s_unix_rgLsiMethods to replace s_rgLsiMethods
::LSIMETHODS
CLineServices::s_unix_rgLsiMethods[CLineServices::LSOBJID_COUNT] =
{
    // Will be filled later.
};

// It takes only 4 bytes of each Method pointer in s_rgLsiMethods
#if defined(SPARC) || defined(_MAC) || (defined(_HPUX_SOURCE) && defined(__APOGEE__))
void CLineServices::InitLsiMethodStruct()
{
    int i, j;
    DWORD *pdest = (DWORD*)s_unix_rgLsiMethods;
    DWORD *psrc = (DWORD*)s_rgLsiMethods;
    // The # of bytes of each member of LSIMETHODS
    // Macintosh uses 12 byte entries not 8
    int s_LSIMETHODS_Sizes[] = {8,8,8,8,8,8,8,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4};

    for (i=0; i< 3; i++) // 3 sub-structs were initialized in s_rgLsiMethods
        for (j=0; j< 23; j++) // Each sub-struct has 23 members
        {
            if (s_LSIMETHODS_Sizes[j] == 8)
            {
#ifdef _MAC
                psrc++; // skip an extra 4 bytes
#endif
                psrc++; // skip the first 4 bytes
            }
            *pdest++ = *psrc++;
        }
}
#else // HP
#error "HP uses 12 bytes method ptrs, so it needs to implement differently"
#endif //SPARC
#endif //UNIX

const CLineServices::RUBYINIT CLineServices::s_rubyinit =
{
    RUBY_VERSION,
    RubyMainLineFirst,
    WCH_ESCRUBY,
    WCH_ESCMAIN,
    WCH_NULL,
    WCH_NULL,
    CLineServices::FetchRubyPosition,
    CLineServices::FetchRubyWidthAdjust,
    CLineServices::RubyEnum,
};

#if defined(UNIX) || defined(_MAC)
::RUBYINIT CLineServices::s_unix_rubyinit =
{
    // will be filled out later
};
#if defined(SPARC) || defined(_MAC) || (defined(_HPUX_SOURCE) && defined(__APOGEE__))
int CLineServices::InitRubyinit()
{
    static BOOL fInit = FALSE;
    int cMethodPtrs = 3;
    if (!fInit)
    {
        BYTE* pdest = (BYTE*)&s_unix_rubyinit;
        BYTE* psrc  = (BYTE*)&s_rubyinit;

#ifdef _MAC
        int iFirst = sizeof(RUBYINIT) - cMethodPtrs * 12; // 3 12-bytes member;
#else
        int iFirst = sizeof(RUBYINIT) - cMethodPtrs * 8; // 3 8-bytes member;
#endif
        memcpy(pdest, psrc, iFirst);
        pdest += iFirst;
        psrc += iFirst;

        for(int i=0; i< cMethodPtrs; i++)
        {
#ifdef _MAC
            psrc += sizeof(DWORD);
#endif
            psrc += sizeof(DWORD);
            *(DWORD*)pdest = *(DWORD*)psrc;
            pdest += sizeof(DWORD);
            psrc  += sizeof(DWORD);
        }
        fInit = TRUE;
    }
    return cMethodPtrs * sizeof(DWORD);
}
#else
#error "HP needs to implement differently"
#endif
#endif // UNIX || _MAC

const CLineServices::TATENAKAYOKOINIT CLineServices::s_tatenakayokoinit =
{
    TATENAKAYOKO_VERSION,
    WCH_ENDTATENAKAYOKO,
    WCH_NULL,
    WCH_NULL,
    WCH_NULL,
    CLineServices::GetTatenakayokoLinePosition,
    CLineServices::TatenakayokoEnum,
};

#if defined(UNIX) || defined(_MAC)
::TATENAKAYOKOINIT CLineServices::s_unix_tatenakayokoinit =
{
    // will be filled later
};

int CLineServices::InitTatenakayokoinit()
{
    static BOOL fInit = FALSE;
    int cMethodPtrs = 2;
    if (!fInit)
    {
        BYTE* pdest = (BYTE*)&s_unix_tatenakayokoinit;
        BYTE* psrc  = (BYTE*)&s_tatenakayokoinit;

#ifdef _MAC
        int iFirst = sizeof(TATENAKAYOKOINIT) - cMethodPtrs * 12; // 2 12-bytes member;
#else
        int iFirst = sizeof(TATENAKAYOKOINIT) - cMethodPtrs * 8; // 2 8-bytes member;
#endif
        memcpy(pdest, psrc, iFirst);
        pdest += iFirst;
        psrc += iFirst;

        for(int i=0; i< cMethodPtrs; i++)
        {
#ifdef _MAC
            psrc += sizeof(DWORD);
#endif
            psrc += sizeof(DWORD);
            *(DWORD*)pdest = *(DWORD*)psrc;
            pdest += sizeof(DWORD);
            psrc  += sizeof(DWORD);
        }
        fInit = TRUE;
    }
    return cMethodPtrs * sizeof(DWORD);
}
#endif

const CLineServices::HIHINIT CLineServices::s_hihinit =
{
    HIH_VERSION,
    WCH_ENDHIH,
    WCH_NULL,
    WCH_NULL,
    WCH_NULL,
    CLineServices::HihEnum,
};

#if defined(UNIX) || defined(_MAC)
::HIHINIT CLineServices::s_unix_hihinit =
{
    // will be filled later
};
#if defined(SPARC) || (defined(_HPUX_SOURCE) && defined(__APOGEE__))
int CLineServices::InitHihinit()
{
    static BOOL fInit = FALSE;
    int cMethodPtrs = 1;
    if (!fInit)
    {
        BYTE* pdest = (BYTE*)&s_unix_hihinit;
        BYTE* psrc  = (BYTE*)&s_hihinit;

#ifdef _MAC
        int iFirst = sizeof(HIHINIT) - 12; // 1 12-bytes member;
#else
        int iFirst = sizeof(HIHINIT) - 8; // 1 8-bytes member;
#endif
        memcpy(pdest, psrc, iFirst);
        pdest += iFirst;
        psrc += iFirst;

#ifdef _MAC
        psrc += sizeof(DWORD);
#endif
        psrc += sizeof(DWORD);
        *(DWORD*)pdest = *(DWORD*)psrc;
        fInit = TRUE;
    }
    return cMethodPtrs * sizeof(DWORD);
}
#else
#error "HP needs to implement this function"
#endif
#endif // UNIX

const CLineServices::WARICHUINIT CLineServices::s_warichuinit =
{
    WARICHU_VERSION,
    WCH_ENDFIRSTBRACKET,
    WCH_ENDTEXT,
    WCH_ENDWARICHU,
    WCH_NULL,
    CLineServices::GetWarichuInfo,
    CLineServices::FetchWarichuWidthAdjust,
    CLineServices::WarichuEnum,
    FALSE
};

#if defined(UNIX) || defined(_MAC)
::WARICHUINIT CLineServices::s_unix_warichuinit =
{
    // will be filled later
};

#if defined(SPARC) || (defined(_HPUX_SOURCE) && defined(__APOGEE__))
int CLineServices::InitWarichuinit()
{
    static BOOL fInit = FALSE;
    int cMethodPtrs = 3; 
    if (!fInit)
    {
        BYTE* pdest = (BYTE*)&s_unix_warichuinit;
        BYTE* psrc  = (BYTE*)&s_warichuinit;

#ifdef _MAC
        int iFirst = sizeof(WARICHUINIT) - cMethodPtrs * 12; // 3 12-bytes member;
#else
        int iFirst = sizeof(WARICHUINIT) - cMethodPtrs * 8; // 3 8-bytes member;
#endif
        memcpy(pdest, psrc, iFirst);
        pdest += iFirst;
        psrc += iFirst;

        for(int i=0; i< cMethodPtrs; i++)
        {
#ifdef _MAC
            psrc += sizeof(DWORD);
#endif
            psrc += sizeof(DWORD);
            *(DWORD*)pdest = *(DWORD*)psrc;
            pdest += sizeof(DWORD);
            psrc  += sizeof(DWORD);
        }
        fInit = TRUE;
    }
    return cMethodPtrs * sizeof(DWORD);
}
#else
#error "HP needs to implement this function"
#endif
#endif // UNIX

const CLineServices::REVERSEINIT CLineServices::s_reverseinit =
{
    REVERSE_VERSION,
    WCH_ENDREVERSE,
    WCH_NULL,
    WCH_NULL,
    WCH_NULL,
    CLineServices::ReverseEnum,
};

#if defined(UNIX) || defined(_MAC)
::REVERSEINIT CLineServices::s_unix_reverseinit =
{
    // will be filled later
};

#if defined(SPARC) || (defined(_HPUX_SOURCE) && defined(__APOGEE__))
int CLineServices::InitReverseinit()
{
    static BOOL fInit = FALSE;
    int cMethodPtrs = 1;
    if (!fInit)
    {
        BYTE* pdest = (BYTE*)&s_unix_reverseinit;
        BYTE* psrc  = (BYTE*)&s_reverseinit;

#ifdef _MAC
        int iFirst = sizeof(REVERSEINIT) - 8; // 1 8-bytes member;
#else
        int iFirst = sizeof(REVERSEINIT) - 8; // 1 8-bytes member;
#endif
        memcpy(pdest, psrc, iFirst);
        pdest += iFirst;
        psrc += iFirst;

#ifdef _MAC
        psrc += sizeof(DWORD);
#endif
        psrc += sizeof(DWORD);
        *(DWORD*)pdest = *(DWORD*)psrc;
        fInit = TRUE;
    }
    return cMethodPtrs * sizeof(DWORD);
}
#else
#error "HP needs to implement this function"
#endif
#endif // UNIX

const WCHAR CLineServices::s_achTabLeader[tomLines] =
{
    WCH_NULL,
    WCH_DOT,
    WCH_HYPHEN
};

const CLineServices::SYNTHDATA
CLineServices::s_aSynthData[SYNTHTYPE_COUNT] =
{
//
//    wch               idObj               typeEndObj                          fObjStart   fObjEnd,    fHidden     fLSCPStop   pszSynthName
//
    { WCH_UNDEF,        idObjTextChp,       SYNTHTYPE_NONE,                     FALSE,      FALSE,      TRUE,       FALSE,      WHEN_DBG(_T("[none]")) },
    { WCH_SECTIONBREAK, idObjTextChp,       SYNTHTYPE_NONE,                     FALSE,      FALSE,      FALSE,      FALSE,      WHEN_DBG(_T("[sectionbreak]")) },
    { WCH_REVERSE,      LSOBJID_REVERSE,    SYNTHTYPE_ENDREVERSE,               TRUE,       FALSE,      FALSE,      FALSE,      WHEN_DBG(_T("[reverse]")) },
    { WCH_ENDREVERSE,   LSOBJID_REVERSE,    SYNTHTYPE_NONE,                     FALSE,      TRUE,       FALSE,      FALSE,      WHEN_DBG(_T("[endreverse]")) },
    { WCH_NOBRBLOCK,    LSOBJID_NOBR,       SYNTHTYPE_ENDNOBR,                  TRUE,       FALSE,      FALSE,      FALSE,      WHEN_DBG(_T("[nobr]")) },
    { WCH_NOBRBLOCK,    LSOBJID_NOBR,       SYNTHTYPE_NONE,                     FALSE,      TRUE,       FALSE,      FALSE,      WHEN_DBG(_T("[endnobr]")) },
    { WCH_ENDPARA1,     idObjTextChp,       SYNTHTYPE_NONE,                     FALSE,      FALSE,      FALSE,      TRUE,       WHEN_DBG(_T("[endpara1]")) },
    { WCH_ALTENDPARA,   idObjTextChp,       SYNTHTYPE_NONE,                     FALSE,      FALSE,      FALSE,      FALSE,      WHEN_DBG(_T("[altendpara]")) },
    { WCH_UNDEF,     	LSOBJID_RUBY,       SYNTHTYPE_ENDRUBYTEXT,              TRUE,       FALSE,      FALSE,      FALSE,      WHEN_DBG(_T("[rubymain]")) },
    { WCH_ESCMAIN,     	LSOBJID_RUBY,       SYNTHTYPE_NONE,                  	TRUE,       TRUE,       FALSE,      FALSE,      WHEN_DBG(_T("[endrubymain]")) },
    { WCH_ESCRUBY,     	LSOBJID_RUBY,       SYNTHTYPE_NONE,                  	FALSE,      TRUE,       FALSE,      FALSE,      WHEN_DBG(_T("[endrubytext]")) },
    { WCH_SYNTHETICLINEBREAK, idObjTextChp,       SYNTHTYPE_NONE,                     FALSE,      FALSE,      FALSE,      TRUE,       WHEN_DBG(_T("[linebreak]")) },
    { WCH_UNDEF,        LSOBJID_GLYPH,      SYNTHTYPE_NONE,                     FALSE,      FALSE,      FALSE,      TRUE,       WHEN_DBG(_T("[glyph]")) },
};


// This is a line-services structure defining an escape
// sequence.  In this case, it is the escape sequence ending
// a nobr block.  It is one character long, and that character
// must be in the range wch_nobrblock through wch_nobrblock
// (i.e. must be exactly wch_nobrblock).
const LSESC CNobrILSObj::s_lsescEndNOBR[NBREAKCHARS] = 
{
    {WCH_NOBRBLOCK,  WCH_NOBRBLOCK},
};
