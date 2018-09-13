/*
 *  @doc    INTERNAL
 *
 *  @module _FONT.H -- Declaration of classes comprising font caching |
 *
 *  Purpose:
 *      Font cache
 *
 *  Owner: <nl>
 *      David R. Fulmer <nl>
 *      Christian Fortini <nl>
 *      Jon Matousek <nl>
 *
 *  History: <nl>
 *      8/6/95      jonmat Devised dynamic expanding cache for widths.
 *
 *  Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
 */

#ifndef I__FONT_H_
#define I__FONT_H_
#pragma INCMSG("--- Beg '_font.h'")

#ifndef X_USP_HXX_
#define X_USP_HXX_
#include "usp.hxx"
#endif

#ifndef X_ATOMTBL_HXX_
#define X_ATOMTBL_HXX_
#include "atomtbl.hxx"
#endif

#ifndef X_FONTINFO_HXX_
#define X_FONTINFO_HXX_
#include "fontinfo.hxx"
#endif

MtExtern(CWidthCacheEntry)

enum CONVERTMODE
{
    CM_UNINITED = -1,
    CM_NONE,            // Use Unicode (W) CharWidth/TextOut APIs
    CM_MULTIBYTE,       // Convert to MBCS using WCTMB and _wCodePage
    CM_SYMBOL,          // Use low byte of 16-bit chars (for SYMBOL_CHARSET
                        //  and when code page isn't installed)
    CM_FEONNONFE,       // FE on non-FE if on Win95
};


// Forwards
class CFontCache;
class CCcs;
class CBaseCcs;

// =============================  CCcs  ========================
// CCcs - caches font metrics and character size for one font

extern INT maxCacheSize[];

/*
 *  CWidthCache
 *
 *  @class  Lightweight Unicode width cache.
 *
 * We have a separate, optimized cache for the lowest 128
 * characters.  This cache just has the width, and not the character
 * since we know the cache is big enough to hold all the widths
 * in that range.  For all the higher characters, we have caches with
 * both the width and the character whose width is stored, since
 * there could be collisions.
 *
 */


#define FAST_WIDTH_CACHE_SIZE    128
// TOTALCACHES is the number of caches not counting the "fast" one.
#define TOTALCACHES         3

MtExtern(CWidthCache)

extern CFontCache & fc();           // font cache manager

class CWidthCache
{

public:
    typedef USHORT CharWidth;

    typedef struct {
        TCHAR   ch;
        CharWidth width;
    } CacheEntry;

    //@access   Private methods and data
    private:
    CharWidth * _pFastWidthCache;

    //@cmember  pointers to storage for widths.
    CacheEntry *(_pWidthCache[TOTALCACHES]);

    //@cmember  Get location where width is stored.
    CacheEntry * GetEntry( const TCHAR ch );

    //@access Public Methods
    public:

    static BOOL    IsCharFast(TCHAR ch) { return ch < FAST_WIDTH_CACHE_SIZE; }

    static TCHAR   EnsureCharIsFast(TCHAR ch) { return ch & (FAST_WIDTH_CACHE_SIZE-1); }

    BOOL    FastWidthCacheExists()  { return _pFastWidthCache != NULL; }

    // Doesn't check if this will work.  Just does it.
    CharWidth  BlindGetWidthFast(const TCHAR ch)
    {
        Assert( FastWidthCacheExists() );
        Assert( IsCharFast(ch) );
        return _pFastWidthCache[ch];
    }

    BOOL    PopulateFastWidthCache(HDC hdc, CBaseCcs* pBaseCcs);

    // Use this one if we run out of memory in GetEntry;
    CacheEntry ceLastResort;

    //@cmember  Called before GetWidth
    BOOL    CheckWidth ( const TCHAR ch, LONG &rlWidth );

    //@cmember  Fetch width if CheckWidth ret FALSE.
    BOOL    FillWidth ( HDC hdc,
                        CBaseCcs * pBaseCcs,
                        const TCHAR ch,
                        LONG &rlWidth );

    void    SetCacheEntry( TCHAR ch, CharWidth width );

    //@cmember  Fetch the width.
    INT     GetWidth ( const TCHAR ch );

    //@cmember  Recycle width cache.
    void    Free();

    //@cmember  Free dynamic mem.
    ~CWidthCache();

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CWidthCache))

private:
    void    ThreadSafeCacheAlloc(void** ppCache, size_t iSize);

};


// =============================  CFontCache  =====================================================
// CFontCache - maintains up to ccsMost font caches

#define cccsMost 16
#define QUICKCRCSEARCHSIZE  31  // Must be 2^n - 1 for quick MOD
                                //  operation, it is a simple hash.

// These definitions are for the face cache held within the font cache.
// This is used to speed up the ApplyFontFace() function used during
// format calculation.
#define CACHEMAX 3
typedef struct _FACECACHE
{
    LONG _latmFaceName;

    // Pack it tight.
    union
    {
        BYTE _bFlagsVar;
        struct
        {
            BYTE _fExplicitFace : 1;
            BYTE _fNarrow : 1;
        };
    };
    BYTE _bCharSet;
    BYTE _bPitchAndFamily;
} FACECACHE;

MtExtern(CBaseCcs)

class CBaseCcs
{
    friend class CFontCache;
    friend class CCcs;
    friend CWidthCache::FillWidth( HDC hdc, class CBaseCcs *, const TCHAR, LONG & );
    friend CWidthCache::PopulateFastWidthCache( HDC hdc, CBaseCcs * );

private:

    DWORD   _dwRefCount;    // ref. count
    DWORD   _dwAge;         // for LRU algorithm

    BYTE    _bCrc;          // check sum for quick comparison with charformats
    BYTE    _bPitchAndFamily; // For CBaseCcs::Compare; identical to _lf.lfPitchAndFamily except in PRC hack

    class CWidthCache _widths;

    SCRIPT_CACHE _sc;       // handle for Uniscribe (USP.DLL) script cache

public:
    LONG    _yCfHeight;     // Height of font in TWIPs.
    LONG    _yOffset;       // baseline offset for super/subscript
    SHORT   _yHeight;       // total height of the character cell in logical units.
    SHORT   _yDescent;      // distance from baseline to bottom of character cell in logical units.
    SHORT   _xAveCharWidth; // average character width in logical units.
    SHORT   _xMaxCharWidth; // max character width in logical units.
    USHORT  _sCodePage;     // code page for font.
    SHORT   _yTextMetricDescent; // Original descent from GetTextMetrics
    SHORT   _xOverhangAdjust;// overhang for synthesized fonts in logical units.
    SHORT   _xOverhang;     // font's overhang.
    SHORT   _xUnderhang;    // font's underhang.
    SHORT   _xDefDBCWidth;  // default width for DB Character
    SHORT   _sPitchAndFamily;    // For getting the right widths.
    BYTE    _bCharSet;
    BYTE    _bConvertMode;  // CONVERTMODE casted down to a byte
    HFONT   _hfont;         // Windows font handle
    SCRIPT_IDS _sids;       // Font script ids.  Cached value from CFontInfo.
    DWORD   _dwLangBits;    // For old-style fontlinking.  BUGBUG (cthrash) retire this.

    // NOTE (paulpark): The LOGFONT structure includes a font name.  We keep _latmLFFaceName in sync with
    // this font name.  It always points into the atom table in the global font-cache to the same thing.
    // For this reason you must never directly change _latmLFFaceName or _lf.lfFaceName without changing
    // the other.  In fact you should just use the two mutator methods: SetLFFaceName and SetLFFaceNameAtm.
    LOGFONT _lf;            // the log font as returned from GetObject().
    LONG    _latmLFFaceName;// For faster string-name comparisons.  The atom table is in the FontCache.
    LONG    _latmBaseFaceName;  // base facename -- for fontlinking
    LONG    _yOriginalHeight;   // pre-adjusted height -- for fontlinking

    ULONG   _fLatin1CoverageSuspicious:1;     // font probably does not adequately cover Latin1
    ULONG   _fHasABC:1;     // font has ABC widths
    ULONG   _fFixPitchFont:1;           // font with fix character width
    ULONG   _fSymbolFont:1;             // TRUE iff symbol font
    ULONG   _fFEFontOnNonFEWin95:1;
    ULONG   _fHeightAdjustedForFontlinking:1;
#if DBG == 1
    static LONG s_cTotalCccs;
    static LONG s_cMaxCccs;
#endif

private:
    ULONG   _fConvertNBSPsSet:1;        // _fConvertNBSPs and _fConvertNBSPsIfA have been set
    ULONG   _fConvertNBSPs:1;           // Font requires us to convert NBSPs to spaces
    ULONG   _fConvertNBSPsIfA:1;        // If calling ExtTextOutA, must convert NBSPs to spaces
    ULONG   _fPrinting:1;

    SHORT   _sAdjustFor95Hack;          // Compute discrepancy between GetCharWidthA and W once.

    BOOL    NeedConvertNBSPs(HDC hdc, CDoc *pDoc);  // Set _fConvertNBSPs/_fConvertNBSPsIfA flags

    BOOL    MakeFont(
                HDC hdc,
                const CCharFormat * const pcf,
                CDocInfo * pdci );

    void    DestroyFont();
    BOOL    GetTextMetrics(HDC hdc, CODEPAGE codepage, LCID lcid);
    BOOL    GetFontWithMetrics( HDC hdc,
                                TCHAR* szNewFaceName,
                                CODEPAGE codepage,
                                LCID lcid );

    BOOL    FillWidths ( HDC hdc, TCHAR ch, LONG &rlWidth );
    void    PrivateRelease();

public:
    CBaseCcs ()
    {
        _dwRefCount = 1;
#if DBG == 1
        s_cMaxCccs = max(s_cMaxCccs, ++s_cTotalCccs);
#endif
    }
    ~CBaseCcs ()
    {
        if(_hfont)
            DestroyFont();

        // make sure script cache is freed
        ReleaseScriptCache();

        WHEN_DBG(s_cTotalCccs--);

    }

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CBaseCcs))

    BOOL    Init(HDC hdc, const CCharFormat * const pcf, CDocInfo * pdci, LONG latmBaseFaceName);
    void    AddRef()    { InterlockedIncrement((LONG *)&_dwRefCount); }
    void    NullOutScriptCache();
    void    ReleaseScriptCache();

    HFONT   PushFont(HDC hdc);
    void    PopFont(HDC hdc, HFONT hfontOld);

    BOOL    ConvertNBSPs(HDC hdc, CDoc *pDoc)
    {
        return ((_fConvertNBSPsSet || NeedConvertNBSPs(hdc, pDoc)) && _fConvertNBSPs);
    }

    typedef struct tagCompareArgs
    {
        CCharFormat * pcf;
        LONG lfHeight;
        LONG latmBaseFaceName;
    } CompareArgs;
    
    BOOL Compare( CompareArgs * pCompareArgs );
    BOOL CompareForFontLink( CompareArgs * pCompareArgs );

    void    GetAscentDescent(SHORT *pyAscent, SHORT *pyDescent)
    {
        *pyAscent  = _yHeight + _yOffset - _yDescent;
        *pyDescent = _yDescent - _yOffset;
    }

    CONVERTMODE GetConvertMode( BOOL fEnhancedMetafile, BOOL fMetafile );

    SCRIPT_CACHE* GetUniscribeCache() { return &_sc; }

    //
    // Width Cache Functions Exposed
    //
    BOOL    Include( HDC hdc, TCHAR ch, LONG &rlWidth );  // Slow, reliable.

    // Assumes ascii.  No checking.  Will crash if > 128.
    USHORT  BlindFastInclude( TCHAR ch ) { return _widths.BlindGetWidthFast(ch); }

    // Returns true if we can use blindfastinclude on this char
    BOOL    CanFastIncludeChar( TCHAR ch ) { return _widths.IsCharFast(ch); }

    // Returns true if it succeeded.  False on out of memory.
    BOOL    EnsureFastCacheExists(HDC hdc)
    {
        if( !_widths.FastWidthCacheExists() )
        {
            _widths.PopulateFastWidthCache(hdc, this);
        }
        return _widths.FastWidthCacheExists();
    }

    // Mutators for _lf.szFaceName
    void SetLFFaceNameAtm(LONG latmFaceName);
    void SetLFFaceName(const TCHAR * szFaceName);

    void VerifyLFAtom();

    void EnsureLangBits(HDC hdc);

    void FixupForFontLink( HDC hdc, CBaseCcs * pBaseBaseCcs );    
};

MtExtern(CCcs)
class CCcs
{
public:
    HDC       _hdc;
    CBaseCcs *_pBaseCcs;

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CCcs))

    CCcs() { _hdc = NULL; _pBaseCcs = NULL; }
    CBaseCcs *GetBaseCcs() { Assert(_pBaseCcs); return _pBaseCcs; }
    void Release() { _pBaseCcs->PrivateRelease(); delete this; }
    HDC  GetHDC() { return _hdc; }

    BOOL Include(TCHAR ch, LONG &rlWidth) { return _pBaseCcs->Include(_hdc, ch, rlWidth); }
    void EnsureLangBits() { _pBaseCcs->EnsureLangBits(_hdc); }
};

MtExtern(CFontCache)

class CFontCache
{
    friend class CCcs;
    friend class CBaseCcs;

private:
    CBaseCcs *   _rpBaseCcs[cccsMost];
    DWORD   _dwAgeNext;
    struct {
        BYTE      bCrc;
        CBaseCcs *pBaseCcs;
    } quickCrcSearch[QUICKCRCSEARCHSIZE+1];

    CRITICAL_SECTION _cs;

public:
#if DBG == 1
    LONG _cCccsReplaced;
#endif


    // For the face cache.
    CRITICAL_SECTION _csFaceCache;  // critical section used for quick face name resolution
    int _iCacheLen; // increments until equal to CACHEMAX
    int _iCacheNext; // The next available spot.
#if DBG == 1
    int _iCacheHitCount;    // For debugging.
#endif
    FACECACHE _fcFaceCache[CACHEMAX];

private:

    CBaseCcs*   GrabInitNewBaseCcs(
                HDC hdc,
                const CCharFormat * const pcf,
                CDocInfo * pdci,
                LONG latmBaseFace );
    DWORD       SetSupportedCodePageInfo(HDC hdc);

    CFontInfoCache  _atFontInfo;
    CRITICAL_SECTION _csFaceNames;


public:

    void Init()
    {
        _dwAgeNext = 0;
        ClearSupportedCodePageInfo();
        InitializeCriticalSection(&_cs);
        InitializeCriticalSection(&_csOther);
        InitializeCriticalSection(&_csFaceCache);
        InitializeCriticalSection(&_csFaceNames);
    }
    void DeInit();

    void FreeScriptCaches();

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CFontCache))

    CCcs*       GetCcs(HDC hdc,
                       CDocInfo * pdci,
                       const CCharFormat * const pcf);
    CCcs*       GetFontLinkCcs(HDC hdc,
                               CDocInfo * pdci,
                               CCcs * pccsBase,
                               const CCharFormat * const pcf);
    CBaseCcs*   GetBaseCcs(HDC hdc,
                           CDocInfo * pdci,
                           const CCharFormat * const pcf,
                           CBaseCcs * pBaseBaseCcs);

    LONG          GetAtomFromFaceName(const TCHAR* szFaceName);
    LONG          FindAtomFromFaceName(const TCHAR* szFaceName);
    const TCHAR * GetFaceNameFromAtom(LONG latmFontInfo);
    HRESULT       AddScriptIdToAtom(LONG latmFontInfo, SCRIPT_ID sid);
    SCRIPT_IDS    EnsureScriptIDsForFont(HDC hdc, CBaseCcs * pccs, BOOL fDownloadedFont);
    SCRIPT_IDS    ScriptIDsForAtom(LONG latmFontInfo);

    LONG          GetAtomWingdings();

    void          ClearSupportedCodePageInfo() { _dwSupportedCodePageInfo = DWORD(-1); }
    DWORD         GetSupportedCodePageInfo(HDC hdc) { return (_dwSupportedCodePageInfo != DWORD(-1))
                                                       ? _dwSupportedCodePageInfo
                                                       : SetSupportedCodePageInfo(hdc); }

    WHEN_DBG(void DumpFontInfo() { _atFontInfo.Dump(); });

    CRITICAL_SECTION _csOther;      // critical section used by cccs stored in rpccs

private:
    //
    // Cached atom values for global font atoms.
    // These are put in the table lazilly
    //
    LONG _latmWingdings;

    //
    // Flag indicating what codepages we support.  Used for Han fontlinking.
    //

    DWORD _dwSupportedCodePageInfo;

};

inline void
CWidthCache::SetCacheEntry( TCHAR ch, CharWidth width )
{
    if( ! IsCharFast(ch) )
    {
        CacheEntry* pce= GetEntry(ch);
        pce->ch= ch;
        pce->width= width;
    }
    else
    {
        Assert( _pFastWidthCache );
        _pFastWidthCache[ch]= width;
    }
}

inline int
CACHE_SWITCH ( const TCHAR ch )
{
    if (ch < 0x4E00)
    {
        Assert( !CWidthCache::IsCharFast(ch) );
        return 0;
    }
    else if (ch < 0xAC00)
        return 1;
    else
        return 2;
}


inline
CWidthCache::CacheEntry *
CWidthCache::GetEntry(const TCHAR ch)
{
    // Figure out which cache we're in.
    Assert( !IsCharFast(ch) );

    int i= CACHE_SWITCH( ch );
    Assert( i>=0 && i < TOTALCACHES );

    CacheEntry ** ppEntry = &_pWidthCache[i];

    if (!*ppEntry)
    {
        ThreadSafeCacheAlloc( (void **)ppEntry, sizeof(CacheEntry) * (maxCacheSize[i] + 1) );

        // Assert that maxCacheSize[i] is of the form 2^n-1
        Assert( ((maxCacheSize[i] + 1) & maxCacheSize[i]) == 0 );

        // Failed, need to return a pointer to something,
        // just to avoid crashing. Layout will look like crap.
        if (!*ppEntry)
            return &ceLastResort;
    }


    // logical & is really a MOD, as all of the bits
    // of cacheSize are turned on; the value of cacheSize is
    // required to be of the form 2^n-1.
    return &(*ppEntry)[ ch & maxCacheSize[i] ];
}



// This function tries to get the width of this character,
// returning TRUE if it can.
// It's called "Include" just to confuse people.
// GetCharWidth would be a better name.
#if DBG != 1
#pragma optimize(SPEED_OPTIMIZE_FLAGS, on)
#endif

inline
BOOL
CBaseCcs::Include ( HDC hdc, TCHAR ch, LONG &rlWidth )
{
    if (   _widths.IsCharFast(ch)
        && _widths.FastWidthCacheExists())
    {
        // ASCII case -- really optimized.
        rlWidth= _widths.BlindGetWidthFast(ch);
        return TRUE;
    }
    else if (_widths.CheckWidth( ch, rlWidth ))
    {
        return TRUE;
    }
    else
    {
        return FillWidths( hdc, ch, rlWidth );
    }
}


/*
 *  CWidthCache::CheckWidth(ch, rlWidth)
 *
 *  @mfunc
 *      check to see if we have a width for a TCHAR character.
 *
 *  @comm
 *      Used prior to calling FillWidth(). Since FillWidth
 *      may require selecting the map mode and font in the HDC,
 *      checking here first saves time.
 *
 *  @rdesc
 *      returns TRUE if we have the width of the given TCHAR.
 *
 *  Note: This should not be called for ascii characters --
 *    a faster codepath should be taken.  This asserts against it.
 */
inline BOOL
CWidthCache::CheckWidth (
    const TCHAR ch,  //@parm char, can be Unicode, to check width for.
    LONG &rlWidth ) //@parm the width of the character
{
    Assert( !IsCharFast(ch) );

    CacheEntry widthData = *GetEntry ( ch );

    if( ch == widthData.ch )
    {
        rlWidth = widthData.width;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

inline void
CBaseCcs::VerifyLFAtom()
{
#if DBG==1
    const TCHAR * szFaceName = fc().GetFaceNameFromAtom(_latmLFFaceName);
        // If this assert fires that means somebody is directly modifying either _latmLFFaceName
        // or _lf.lffacename.  You should never modify these directly, but instead use the
        // SetLFFaceName or SetLFFaceNameAtm mutator methods, as these are sure to keep the
        // actual string and the atomized value in sync.
#ifdef UNIX
    Assert( !StrCmpC( _lf.lfFaceName, szFaceName ) );
#else
    Assert( !StrCmpIC( _lf.lfFaceName, szFaceName ) );
#endif
#endif
}

#if DBG==1
    HFONT SelectFontEx(HDC hdc, HFONT hfont);
    BOOL DeleteFontEx(HFONT hfont);
#else
    #define SelectFontEx(hdc, hfont) SelectFont(hdc, hfont)
    #define DeleteFontEx(hfont) DeleteObject(hfont)
#endif

#if DBG != 1
#pragma optimize("", on)
#endif

#pragma INCMSG("--- End '_font.h'")
#else
#pragma INCMSG("*** Dup '_font.h'")
#endif
