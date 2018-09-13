//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       htmpre.cxx
//
//  Contents:   Support for HTML preparsing, including
//
//              CAttrWatchDesc
//              CTagWatchDesc
//              CValueBuffer
//              CTagWatch
//              CHtmPre
//
//-------------------------------------------------------------------------

// KNOWN areas this tokenizer differs from NS:
// (1) embedded \0 handling. NS leaves them in until after
//     tokenizing, and we now strip out embedded \0 chars
//     before tokenizing. I've tried NS's approach, and
//     found that it adds unneeded complexity.
// (2) NS treats an initial unknown tag at the very beginning
//     of the file as an overlaped scope tag which hides text.
//     We do not treat this tag specially.
// (3) NS matches entities in a case-sensitive manner. We should
//     do the same.
// (4) NS does not resolve named or numeric entities which end
//     at the EOF without a semicolon. We do.
// TO DO:
// (1) Feed line/char count to special tags that need them (<SCRIPT>)
// (2) Saver must save entity-like chars in literals correctly.
// (dbau 12/16/96)

#include "headers.hxx"

#ifndef X_HTM_HXX_
#define X_HTM_HXX_
#include "htm.hxx"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_ASSOC_HXX_
#define X_ASSOC_HXX_
#include "assoc.hxx"
#endif

#ifndef X_ENTITY_H_
#define X_ENTITY_H_
#include "entity.h"
#endif

#ifndef X_HTMVER_HXX_
#define X_HTMVER_HXX_
#include "htmver.hxx"
#endif

#ifdef WIN16
#ifndef X_URLMKI_H_
#define X_URLMKI_H_
#include "urlmki.h"
#endif
#endif

#ifndef X_SWITCHES_HXX_
#define X_SWITCHES_HXX_
#include "switches.hxx"
#endif

#ifndef X_HTMTOK_HXX_
#define X_HTMTOK_HXX_
#include "htmtok.hxx"
#endif

#ifndef X_PRGSNK_H_
#define X_PRGSNK_H_
#include "prgsnk.h"
#endif


#define _cxx_
#include "entity.h"

#define HTMPRE_BLOCK_SIZE      4096 // assumed to be power of 2

#define CCH_LARGE_TOKEN       16384 // after text buf grows to 16K, use exponential allocation
#define CCH_OVERFLOW_TOKEN  1048576 // stop growing after a megabyte
#define MAX_ATTR_COUNT        16383 // Allow at most 16K-1 attribute-value pairs

#define CONDITIONAL_FEATURE

ExternTag(tagPalette);

DeclareTag(tagNoValues,             "Dwn", "HtmPre: Treat all values as absent");
DeclareTag(tagEmptyValues,          "Dwn", "HtmPre: Treat all values as empty strings");
DeclareTag(tagHtmPreNoAutoLoad,     "Dwn", "HtmPre: Inhibit auto-download");
DeclareTag(tagToken,                "Dwn", "HtmPre: Detailed tokenizer trace");
PerfDbgTag(tagHtmPre,               "Dwn", "Trace CHtmPre")
PerfDbgTag(tagForceSwitchToRestart, "Dwn", "! Force SwitchCodePage to restart")
PerfDbgTag(tagHtmPreOneCharText,    "Dwn", "! HtmPre slow one char per text tag")

PerfDbgExtern(tagPerfWatch)
PerfDbgExtern(tagDwnBindSlow)

MtDefine(CHtmPre, Dwn, "CHtmPre")
MtDefine(CHtmPreBuffer, CHtmPre, "CHtmPre::_pchBuffer")
MtDefine(CHtmPre_aryInsert_pv, CHtmPre, "CHtmPre::_aryInsert::_pv")
MtDefine(CHtmPre_aryCchSaved_pv, CHtmPre, "CHtmPre::_aryCchSaved::_pv")
MtDefine(CHtmPre_aryCchAsciiSaved_pv, CHtmPre, "CHtmPre::_aryCchAsciiSaved::_pv")

DWORD HashString(const TCHAR *pch, DWORD len, DWORD hash);
extern HRESULT SetUrlDefaultScheme(const TCHAR *pchHref, CStr *pStr);
extern BOOL _7csnziequal(const TCHAR *string1, DWORD cch, const TCHAR *string2);

#if DBG != 1
#pragma optimize(SPEED_OPTIMIZE_FLAGS, on)
#endif

// CHROME

// This table is a list of CLSID's used by Chrome controls. The HTML pre parser uses
// these to determine whether images imbedded in an object tag should have image
// downloading supressed at the pre parser stage. All other controls (those whose
// class id's are not contained in the table) continue to be processed as normal,
// i.e., IMG tags embeded within them will have thier bits downloaded.

static const CLSID ChromeClsidTable[] = 
{
    { 0x344C63C3, 0x8814, 0x11D1, { 0x9E, 0x24, 0x00, 0xA0, 0xC9, 0x0D, 0x61, 0x11 } },
    { 0x8A2A7F99, 0x09DA, 0x11D1, { 0xB3, 0x3A, 0x00, 0xA0, 0xC9, 0x0A, 0x8F, 0xB6 } },
};

BOOL IsChromeClsid(CLSID* pclsid)
{
    for (int i = 0; i < ARRAY_SIZE(ChromeClsidTable); i++)
    {
        if (IsEqualCLSID(*pclsid, ChromeClsidTable[i]))
            return TRUE;
    }
    return FALSE;
}
const BYTE g_charclass[64] = {
// CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH

0,                                                                                                 // 0
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 1
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 2
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 3
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 4
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 5
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 6
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 7
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 8
   CCF_TXTCH |                                                 CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 9  \t
0,                                                                                                 // 10 \n
   CCF_TXTCH |                                                 CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 11
   CCF_TXTCH |                                                 CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 12
0,                                                                                                 // 13 \r
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 14
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 15
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 16
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 17
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 18
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 19
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 20
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 21
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 22
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 23
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 24
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 25
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 26
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 27
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 28
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 29
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 30
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 31
   CCF_TXTCH |                                                 CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 32 ' '
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 33 '!'
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH |             CCF_VALCH | CCF_NDASH             | CCF_MRKCH , // 34 '"'
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 35 '#'
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 36 '$'
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 37 '%'
               CCF_NONSP | CCF_NAMCH | CCF_ATTCH |             CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 38 '&'
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH |             CCF_VALCH | CCF_NDASH             | CCF_MRKCH , // 39 '''
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 40 '('
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 41 ')'
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 42 '*'
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 43 '+'
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 44 ','
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH |             CCF_TAGCH | CCF_MRKCH , // 45 '-'
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 46 '.'
   CCF_TXTCH | CCF_NONSP |                         CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 47 '/'
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 48 '0'
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 49 '1'
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 50 '2'
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 51 '3'
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 52 '4'
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 53 '5'
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 54 '6'
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 55 '7'
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 56 '8'
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 57 '9'
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 58 ':'
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 59 ';'
               CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 60 '<'
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH |             CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 61 '='
   CCF_TXTCH | CCF_NONSP |                                     CCF_NDASH                         , // 62 '>'
   CCF_TXTCH | CCF_NONSP | CCF_NAMCH | CCF_ATTCH | CCF_VALCH | CCF_NDASH | CCF_TAGCH | CCF_MRKCH , // 63 '?'
};

//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::~CHtmPre
//
//-------------------------------------------------------------------------
CHtmPre::~CHtmPre()
{
    PerfDbgLog(tagHtmPre, this, "+CHtmPre::~CHtmPre");

    if (_pHtmLoad)
        _pHtmLoad->SubRelease();

    if (_pHtmInfo)
        _pHtmInfo->SubRelease();

    if (_pHtmTagStm)
        _pHtmTagStm->Release();

    if (_pDwnBindData)
        _pDwnBindData->Release();

    if (_pDwnDoc)
        _pDwnDoc->Release();

    if (_pVersions)
        _pVersions->Release();

    ReleaseInterface(_pInetSess);

    _aryGenericTags.Free();

    // Don't let CEncoderReader free this pointer.  It doesn't belong to it.

    _pchBuffer = NULL;

    PerfDbgLog(tagHtmPre, this, "-CHtmPre::~CHtmPre");
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::Passivate
//
//-------------------------------------------------------------------------
void
CHtmPre::Passivate()
{
    PerfDbgLog(tagHtmPre, this, "+CHtmPre::Terminate");

    _fDone = TRUE;
    _cSuspended = 0;

    super::Passivate();

    PerfDbgLog(tagHtmPre, this, "-CHtmPre::Terminate");
}

//+------------------------------------------------------------------------
//
//  Function:   CanPrefetchWithScheme
//
//  Synopsis:   Checks if the scheme is one of the "big-3"
//
//-------------------------------------------------------------------------

BOOL
CanPrefetchWithScheme(TCHAR * pchUrl)
{
    // Combining URLs requires that CoInitialize is called because
    // it might try to load the protocol handler objects for random
    // protocols.  But this thread never called CoInitialize (and
    // it doesn't want to because we don't have a message loop), we
    // punt prefetching on anything but the big-3 internal protocols.

    UINT uScheme = GetUrlScheme(pchUrl);

    return( uScheme == URL_SCHEME_FILE
        ||  uScheme == URL_SCHEME_HTTP
        ||  uScheme == URL_SCHEME_HTTPS);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::OnRedirect
//
//-------------------------------------------------------------------------
HRESULT
CHtmPre::OnRedirect(LPCTSTR pchUrl)
{
    PerfDbgLog1(tagHtmPre, this, "+CHtmPre::OnRedirect %ls",
        pchUrl ? pchUrl : g_Zero.ach);

    HRESULT hr;

    hr = THR(_cstrDocUrl.Set(pchUrl));
    if (hr)
        goto Cleanup;

    if (_pInetSess && !CanPrefetchWithScheme(_cstrDocUrl))
    {
        ClearInterface(&_pInetSess);
    }

    hr = THR(_pDwnDoc->SetSubReferer(pchUrl));
    if (hr)
        goto Cleanup;

Cleanup:
    PerfDbgLog1(tagHtmPre, this, "-CHtmPre::OnRedirect (hr=%lX)", hr);
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::Init
//
//-------------------------------------------------------------------------
#ifdef XMV_PARSE
HRESULT
CHtmPre::Init(CHtmLoad * pHtmLoad, CDwnDoc * pDwnDoc,
    IInternetSession * pInetSess, IStream *pstmLeader, CDwnBindData * pDwnBindData,
    CHtmTagStm * pHtmTagStm, HTMPASTEINFO * phpi, LPCTSTR pchUrl, CVersions *pVersions, BOOL fXML)
#else
HRESULT
CHtmPre::Init(CHtmLoad * pHtmLoad, CDwnDoc * pDwnDoc,
    IInternetSession * pInetSess, IStream *pstmLeader, CDwnBindData * pDwnBindData,
    CHtmTagStm * pHtmTagStm, HTMPASTEINFO * phpi, LPCTSTR pchUrl, CVersions *pVersions)
#endif
{
    PerfDbgLog1(tagHtmPre, this, "+CHtmPre::Init %ls",
        pchUrl ? pchUrl : g_Zero.ach);

    HRESULT hr = S_OK;
    BOOL fEof = FALSE;

    _pHtmLoad = pHtmLoad;
    _pHtmLoad->SubAddRef();

    _pHtmInfo = pHtmLoad->GetHtmInfo();
    _pHtmInfo->SubAddRef();

    _pDwnDoc = pDwnDoc;
    _pDwnDoc->AddRef();

    _fMetaCharsetOverride = (_pDwnDoc->GetLoadf() & DLCTL_NO_METACHARSET ? TRUE : FALSE);
    _pDwnDoc->SetLoadf(_pDwnDoc->GetLoadf() & ~DLCTL_NO_METACHARSET);

    ReplaceInterface(&_pInetSess, pInetSess);

    _pDwnBindData = pDwnBindData;
    _pDwnBindData->AddRef();

    _pHtmTagStm = pHtmTagStm;
    _pHtmTagStm->AddRef();

    if (pVersions)
        pVersions->AddRef();
    _pVersions = pVersions;

    Assert(!_fEndCR);

    _cbNextInsert = -1;

    _state = TS_TEXT;

    _cpNew = CP_UNDEFINED;
    _fPasting = phpi != NULL;

#ifdef XMV_PARSE
    _fXML = fXML;
#endif

    // CHROME
    // Image download is enabled until we see a Chrome object tag
    _cDownloadSupression = 0;

    if (phpi)
    {
        if (phpi->cbSelBegin >= 0)
        {
            hr = THR(AddInsert(phpi->cbSelBegin, TIC_BEGINSEL));

            if (hr)
                goto Cleanup;
        }

        if (phpi->cbSelEnd >= 0)
        {
            hr = THR(AddInsert(phpi->cbSelEnd, TIC_ENDSEL));

            if (hr)
                goto Cleanup;
        }
    }

    hr = THR(_cstrDocUrl.Set(pchUrl));
    if (hr)
        goto Cleanup;

    if (_pInetSess && (!CanPrefetchWithScheme(_cstrDocUrl) || _fPasting))
    {
        ClearInterface(&_pInetSess);
    }

    // Start buffer with contents of pstmLeader

    if (pstmLeader)
    {
        _fRestarted = TRUE; // Set so we don't restart a second time

        while (!fEof)
        {
            hr = THR(PrepareToEncode());
            if (hr)
                goto Cleanup;

            hr = THR(ReadStream(pstmLeader, &fEof));
            if (hr)
                goto Cleanup;

            if (_cbBuffer)
            {
                int cch;

                hr = THR(WideCharFromMultiByte(fEof && _pDwnBindData->IsEof(), &cch));
                if (hr)
                    goto Cleanup;

                cch -= PreprocessBuffer(cch);
                _ulCharsEnd += cch;
            }

        }
    }

Cleanup:
    PerfDbgLog1(tagHtmPre, this, "-CHtmPre::Init (hr=%lX)", hr);
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::Run, CDwnTask
//
//  Synopsis:   Runs the tokenizer and adjusts the task
//
//-------------------------------------------------------------------------

void
CHtmPre::Run()
{
    PerfDbgLog(tagPerfWatch, this, "+CHtmPre::Run");
    PerfDbgLog(tagHtmPre, this, "+CHtmPre::Run");

    SwitchesBegTimer(SWITCHES_TIMER_TOKENIZER);

    BOOL    fDataPend;
    HRESULT hr;

    if (!_fDone)
    {
        hr = THR(Exec());

        if (hr)
        {
            _fDone = TRUE;
            IGNORE_HR(OutputDocSize());
            OutputEof(hr);
        }
    }

    fDataPend = _pDwnBindData->IsPending();

    if (_fDone || _cSuspended || (_pch == _pchEnd && fDataPend))
    {
        PerfDbgLog4(tagHtmPre, this, "CHtmPre::Run blocking "
            "(_fDone: %s, _cSuspended: %d, fBufPend: %s, fDataPend: %s)",
            _fDone ? "T" : "F", _cSuspended, (_pch == _pchEnd) ? "T" : "F",
            fDataPend ? "T" : "F");

        SetBlocked(TRUE);
    }

    SwitchesEndTimer(SWITCHES_TIMER_TOKENIZER);

    PerfDbgLog(tagHtmPre, this, "-CHtmPre::Run");
    PerfDbgLog(tagPerfWatch, this, "-CHtmPre::Run");
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::Exec
//
//  Synopsis:   Runs the tokenizer
//
//-------------------------------------------------------------------------

HRESULT
CHtmPre::Exec()
{
    PerfDbgLog(tagPerfWatch, this, "+CHtmPre::Exec");
    PerfDbgLog(tagHtmPre, this, "+CHtmPre::Exec");

    BOOL fAtInsert;
    HRESULT hr = S_OK;

    if (_cSuspended)
        goto Leave;

    for (;;)
    {
        hr = THR(PrepareToEncode());
        if (hr)
            goto Cleanup;

        hr = THR(Read());
        if (hr)
            goto Cleanup;

        do
        {
            if (_cbBuffer)
            {
                int cch;

                hr = THR(WideCharFromMultiByte(_pDwnBindData->IsEof(), &cch));
                if (hr)
                    goto Cleanup;

                cch -= PreprocessBuffer(cch);

                if (cch)
                {
                    _ulCharsEnd += cch;
                }
            }

            _ulCharsUncounted = 0;
            _fCount = 1;

            fAtInsert = AtInsert();

            hr = THR_NOTRACE(Tokenize());
            if (hr)
                goto Cleanup;

            if (fAtInsert)
            {
                QueueInserts();
                
                if (_state == TS_TEXT)
                {
                    hr = THR( OutputInserts() );

                    if (hr)
                        goto Cleanup;
                }
            }

        } while (!Exhausted() && !fAtInsert);

        if (_pDwnBindData->IsEof())
        {
            _fEOF  = TRUE;

            hr = THR_NOTRACE(Tokenize());
            if (hr)
                goto Cleanup;

            Assert(_state == TS_TEXT || _state == TS_PLAINTEXT || _state == TS_ENTCLOSE); // should finish file in a clean state

            _fDone = TRUE;

            // If after reading the entire document our codepage is still
            // autodetect, we've encountered a pure-ASCII page.  We can therefore
            // treat the document as if it were in CP_ACP without risk.

            if (IsAutodetectCodePage(_cp))
            {
                DoSwitchCodePage(g_cpDefault, NULL, FALSE);
            }

            hr = THR(OutputDocSize());
            if (hr)
                goto Cleanup;

            hr = THR(OutputInserts());
            if (hr)
                goto Cleanup;

            OutputEof(S_OK);
            break;

        }

        if (_pDwnBindData->IsPending() || IsTimeout())
            break;
    }

Cleanup:

    if (hr == E_PENDING)
    {
        // Flush the constructed but not signalled tag now that the state machine is in a clean
        // state and can tolerate being re-entered.

        _pHtmTagStm->WriteTagEnd();
        _pHtmTagStm->Signal();
        hr = S_OK;
    }

Leave:
    PerfDbgLog1(tagHtmPre, this, "-CHtmPre::Exec (hr=%lX)", hr);
    PerfDbgLog(tagPerfWatch, this, "-CHtmPre::Exec");
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::PreprocessBuffer
//
//  Synopsis:   Advances _pchEnd by cch (assuming cch chars have been put
//              in place already), but first removes embedded \0 chars
//              and does NS-compatable CRLF handling.
//
//              CR   -> CR
//              LF   -> CR
//              CRLF -> CR
//              LFCR -> CRCR
//
//  Returns:    Number of chars by which the buffer was reduced.
//
//-------------------------------------------------------------------------

int
CHtmPre::PreprocessBuffer(int cch)
{
    BOOL fAscii;

    if (!cch)
        return 0;

    // Special handling for files beginning with \0
    if (!_fCheckedForLeadingNull)
    {
        if (!*_pchEnd)
            _fSuppressLeadingText = TRUE;

        _fCheckedForLeadingNull = TRUE;
    }

    TCHAR *pchEnd = _pchEnd + cch;

    int cNukedChars = NormalizerChar(_pchEnd, &pchEnd, &fAscii);

    _pchEnd = pchEnd;

    if (!fAscii)
        _pchAscii = pchEnd;

    return cNukedChars;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::MakeRoomForChars
//
//  Synopsis:   Opens up space in buffer, allocating and moving memory
//              if needed.
//
//  Memory layout:
//
//            Previous text<a href = "zee.htm">Future Text0    Saved text
//            +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//   pchBuffer^    pchStart^  pchWord^  pch^        pchEnd^    ^pchTop   ^
//                                                    pchBuffer+cchBuffer^
//
//-------------------------------------------------------------------------

HRESULT
CHtmPre::MakeRoomForChars(int cch)
{
    HRESULT hr = S_OK;

    Assert(!_cchSaved || _cSuspended);

    if (!_pchBuffer)
    {
        Assert(!_cchSaved);

        _cchBuffer = cch + 1;

        hr = THR(_pHtmTagStm->AllocTextBuffer(_cchBuffer, &_pchBuffer));
        if (hr)
            goto Cleanup;

        _pchStart = _pch = _pchEnd = _pchAscii = _pchBuffer;
        _pchTop = _pchBuffer + _cchBuffer;

        *_pchEnd = _T('\0');
    }
    else if (cch > PTR_DIFF(_pchTop, _pchEnd + 1))
    {
        long cchUsed = PTR_DIFF(_pchStart, _pchBuffer);
        long cchData = PTR_DIFF(_pchEnd + 1, _pchStart);
        long cchNeed = cchData + cch + _cchSaved;
        // Bug 9535: exponential reallocation after a certain size is reached
        long cchWant = cchNeed <= CCH_LARGE_TOKEN ? cchNeed : cchNeed + cchData / 2;
        long cchDiff;
        TCHAR * pchNewBuffer;

        if (cchUsed == 0)
            hr = THR(_pHtmTagStm->GrowTextBuffer(cchWant, &pchNewBuffer));
        else
            hr = THR(_pHtmTagStm->AllocTextBuffer(cchWant, &pchNewBuffer));
        if (hr)
            goto Cleanup;

        if (cchData && cchUsed)
        {
            memmove(pchNewBuffer, _pchStart, cchData * sizeof(TCHAR));
        }

        if (_cchSaved)
        {
            memmove(pchNewBuffer + cchWant - _cchSaved,
                    cchUsed ? _pchTop : pchNewBuffer + PTR_DIFF(_pchTop, _pchBuffer),
                    _cchSaved * sizeof(TCHAR));
        }

        if (_pchStart > _pchAscii)
            _pchAscii = _pchStart;

        cchDiff    = PTR_DIFF(pchNewBuffer, _pchStart);
        _pchTop    = pchNewBuffer + cchWant - _cchSaved;
        _pchBuffer = pchNewBuffer;
        _cchBuffer = cchWant;

        if (cchDiff)
        {
            _pchStart += cchDiff;
            _pchWord  += cchDiff;
            _pch      += cchDiff;
            _pchEnd   += cchDiff;
            _pchAscii += cchDiff;
        }

    }
    Assert((PTR_DIFF(_pchTop, _pchEnd + 1)) >= cch);

    CEncodeReader::MakeRoomForChars(cch);

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::Suspend
//
//  Synopsis:   Sets appropriate state for suspending the preparser
//
//  Threading:  Called on the preparser thread.
//
//              The _cSuspended++ closes the gate on the preparser thread
//              so that Tokenize() isn't called by Run.
//
//              After Suspend, Tokenize() can safely be called from the
//              script thread (via InsertText).
//
//-------------------------------------------------------------------------
HRESULT
CHtmPre::Suspend()
{
    PerfDbgLog(tagPerfWatch, this, "+CHtmPre::Suspend");
    PerfDbgLog(tagHtmPre, this, "+CHtmPre::Suspend");

    HRESULT hr = S_OK;
    AssertSz(!_fSuspend, "Recursive suspend without InsertText");

    // Make sure a buffer is available so that SaveBuffer works
    if (!_pchEnd)
    {
        hr = THR(MakeRoomForChars(1));
        if (hr)
            goto Error;
    }

    // exercised only when nested InsertText did not SaveBuffer
    if (_fSuspend)
    {
        hr = THR(SaveBuffer());
        if (hr)
            goto Error;
        _fSuspend = FALSE;
    }

    // optimization: don't SaveBuffer; just note that SaveBuffer is needed
    _fSuspend = TRUE;
    _cSuspended++;

Error:
    PerfDbgLog1(tagHtmPre, this, "-CHtmPre::Suspend (hr=%lX)", hr);
    PerfDbgLog(tagPerfWatch, this, "-CHtmPre::Suspend");
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::SaveBuffer
//
//  Synopsis:   Saves any chars in buffer which haven't been processed yet.
//              RestoreBuffer brings the chars back. Can be nested.
//
//              Only works when tokenizer is suspended and in TS_TEXT
//              state.
//
//-------------------------------------------------------------------------
HRESULT
CHtmPre::SaveBuffer() 
{
    PerfDbgLog(tagHtmPre, this, "+CHtmPre::SaveBuffer");

    // state needs to be pushed
    int     cch;
    int     cchAscii;
    HRESULT hr;

    Assert(_fSuspend);
    Assert(_state == TS_TEXT);
    Assert(_pchEnd + 1 <= _pchTop);
    Assert(_pch == _pchStart);
    Assert(_pchEnd >= _pchStart);

    hr = THR(_aryCchSaved.EnsureSize(_aryCchSaved.Size()+1));
    if (hr)
        goto Cleanup;

    hr = THR(_aryCchAsciiSaved.EnsureSize(_aryCchAsciiSaved.Size()+1));
    if (hr)
        goto Cleanup;

    cch = PTR_DIFF(_pchEnd, _pch);
    cchAscii = PTR_DIFF(_pchEnd, _pchAscii);
    if (cchAscii > cch)
        cchAscii = cch;

    _cchSaved += cch;
    _pchTop   -= cch;

    memmove(_pchTop, _pch, cch * sizeof(TCHAR));
    _pchEnd = _pch;
    *_pchEnd   = _T('\0');

    Verify(!_aryCchSaved.AppendIndirect(&cch));
    Verify(!_aryCchAsciiSaved.AppendIndirect(&cchAscii));

Cleanup:
    PerfDbgLog1(tagHtmPre, this, "-CHtmPre::SaveBuffer (hr=%lX)", hr);
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::RestoreBuffer
//
//  Synopsis:   Restores chars previously saved by SaveBuffer.
//              Should be called same number of times as SaveBuffer.
//
//              Works in any state.
//
//-------------------------------------------------------------------------
void
CHtmPre::RestoreBuffer()
{
    PerfDbgLog(tagHtmPre, this, "+CHtmPre::RestoreBuffer");

    int cch = _aryCchSaved[_aryCchSaved.Size()-1];
    int cchAscii = _aryCchAsciiSaved[_aryCchAsciiSaved.Size()-1];
    
    _aryCchSaved.Delete(_aryCchSaved.Size()-1);
    _aryCchAsciiSaved.Delete(_aryCchAsciiSaved.Size()-1);

    Assert(_cchSaved >= cch);

    memmove(_pchEnd, _pchTop, cch * sizeof(TCHAR));
    _pchTop   += cch;
    _cchSaved -= cch;
    _pchEnd   += cch;

    if (cchAscii < cch)
        _pchAscii = _pchEnd - cchAscii;
        
    *(_pchEnd) = _T('\0');

    PerfDbgLog(tagHtmPre, this, "-CHtmPre::RestoreBuffer");
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::Resume
//
//  Synopsis:   The inverse of Suspend
//
//  Threading:  Called on the script thread.
//
//              The _cSuspended-- at the end opens the gate for the
//              preparser thread to continue calling Tokenize().
//
//  Returns:    _cSuspended
//
//-------------------------------------------------------------------------
ULONG
CHtmPre::Resume()
{
    PerfDbgLog(tagPerfWatch, this, "+CHtmPre::Resume");
    PerfDbgLog(tagHtmPre, this, "+CHtmPre::Resume");

    AssertSz(_cSuspended, "Resume called while not suspended");

    if (_fSuspend)
    {
        // state was never pushed on the stack
        _fSuspend = FALSE;
    }
    else
    {
        // restore saved chars
        RestoreBuffer();
    }
    // now everything is done except for csuspended

    // note that _cSuspended==0 opens the gate for the preparser thread

    ULONG cSuspended = _cSuspended-1;
    _cSuspended = cSuspended;

    PerfDbgLog1(tagHtmPre, this, "-CHtmPre::Resume (cSuspended=%ld)", cSuspended);
    PerfDbgLog(tagPerfWatch, this, "-CHtmPre::Resume");
    return(cSuspended);
}

#ifdef VSTUDIO7
//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::GetIdentityBaseTag
//
//-------------------------------------------------------------------------

ELEMENT_TAG
CHtmPre::GetIdentityBaseTag(LPTSTR pch, int cch)
{
    CTagsource * ptagsrc;
    CStr         cstrTagName;

    cstrTagName.Set(pch, cch);
    ptagsrc = _pHtmInfo->GetTagsource(cstrTagName);
    return (ptagsrc) ? ptagsrc->_etagBase : ETAG_NULL;
}
#endif //VSTUDIO7

//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::GetGenericTag
//
//-------------------------------------------------------------------------

ELEMENT_TAG
CHtmPre::GetGenericTag(LPTSTR pch, int cch)
{
#if 1

    //
    // only builtin tags recognized as literal generic tags for now
    //

#ifdef VSTUDIO7
    ELEMENT_TAG etag = ETAG_NULL;
    etag = GetBuiltinLiteralGenericTag(pch, cch) ? ETAG_GENERIC_LITERAL : ETAG_NULL;
    if (etag == ETAG_NULL)
    {
        if (StrCmpNIC(pch, TAGSOURCE_TAGNAME, cch) == 0)
            etag = ETAG_RAW_TAGSOURCE;
        else if (StrCmpNIC(pch, FACTORY_TAGNAME, cch) == 0)
            etag = ETAG_RAW_FACTORY;
    }
    return etag;
#else
    return GetBuiltinLiteralGenericTag(pch, cch) ? ETAG_GENERIC_LITERAL : ETAG_NULL;
#endif //VSTUDIO7

#else
    HRESULT hr;

    if (fConfirmed)
    {
        if (GetBuiltinLiteralGenericTag(cstr))
            return ETAG_GENERIC_LITERAL;

        hr = _aryGenericTags.GetAtomFromName(cstr, NULL, FALSE);
    }
    else
    {
        hr = _aryGenericTagsUnconfirmed.GetAtomFromName(cstr, NULL, FALSE);
    }

    return (S_OK == hr) ? ETAG_GENERIC_LITERAL : ETAG_NULL;
#endif
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::InsertText
//
//  Synopsis:   Inserts text directly into the preparser, used by
//              implementation of document.write.
//              New text is inserted at _pch, which needs to be at the
//              same place as _pchStart.
//
//  Threading:  Called on the script thread.
//
//              Must be called between Suspend() and Resume().
//
//-------------------------------------------------------------------------
HRESULT
CHtmPre::InsertText(LPCTSTR pchInsert, ULONG cchInsert)
{
    PerfDbgLog1(tagHtmPre, this, "+CHtmPre::InsertText (cch=%ld)", cchInsert);

    HRESULT hr;
    BOOL fCountSrc = FALSE;

    Assert(_cSuspended); // must be called between Suspend and Resume

    // Capture the source only if this document was created with
    // document.open() and the write occurs at the top level.
    // We don't want to capture the source generated as
    // a product of running inline scripts.

    if (_pHtmInfo->IsOpened() && _cSuspended == 1)
    {
        hr = THR(_pHtmInfo->OnSource((BYTE*)pchInsert, cchInsert * sizeof(TCHAR)));
        if (hr)
            goto Cleanup;

        fCountSrc = TRUE;
    }

    // push buffer
    if (_fSuspend)
    {
        hr = THR(SaveBuffer());
        if (hr)
            goto Cleanup;
        _fSuspend = FALSE;
    }

    hr = THR(MakeRoomForChars(cchInsert));
    if (hr)
        goto Cleanup;

    memcpy(_pchEnd, pchInsert, cchInsert * sizeof(TCHAR));

    cchInsert -= PreprocessBuffer(cchInsert);

    if (fCountSrc)
    {
        _ulCharsEnd += cchInsert;
        _ulCharsUncounted = 0;
        _fCount = 1;
        if (cchInsert)
            IGNORE_HR(OutputDocSize());
    }
    else
    {
        _ulCharsUncounted += cchInsert;
        _fCount = 0;
    }

Cleanup:
    PerfDbgLog1(tagHtmPre, this, "-CHtmPre::InsertText (hr=%lX)", hr);
    RRETURN(hr);

}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::DuplicateTextAtStart
//
//  Synopsis:   Inserts text directly into the preparser.  
//              Similar in concept to InsertText, but with some
//              key differences.  This function must be called from the preparse.
//              The text it inserts into the stream is always the text
//              starting at _pchStart.  This is also where the text
//              is inserted.  e.g.
//              if _pchStart points to "<b>foo bar baz", and
//              DuplicateTextAtStart is called with cch of 3, then
//              the stream will then look like "<b><b>foo bar baz"
//              with _pchStart pointing at the second <b>, having been
//              advanced 3, as will all the other stream pointers.
//
//              ** NOTE ** that since this involves copying
//              of buffers around, _pchStart and _pch and such
//              might get changed by this command.  
//          
//-------------------------------------------------------------------------
HRESULT
CHtmPre::DuplicateTextAtStart(ULONG cchInsert)
{
    HRESULT hr;

    PerfDbgLog1(tagHtmPre, this, "+CHtmPre::InsertText (cch=%ld)", cchInsert);

    Assert( cchInsert );

    hr = THR(MakeRoomForChars(cchInsert));
    if (hr)
        goto Cleanup;

    // Move the old stuff forward to make room.  The stuff left behind
    // is exactly what we wanted to copy.
    memmove(_pchStart + cchInsert, _pchStart, (_pchEnd - _pchStart) * sizeof(TCHAR) );

    _pch        += cchInsert;
    _pchStart   += cchInsert;
    _pchAscii   += cchInsert;
    _pchWord    += cchInsert;
    _pchEnd     += cchInsert;
    *_pchEnd    = _T('\0');

Cleanup:
    PerfDbgLog1(tagHtmPre, this, "-CHtmPre::InsertText (hr=%lX)", hr);
    RRETURN(hr);

}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::TokenizeText
//
//  Synopsis:   Tokenizes text previously inserted by InsertText, used
//              by implementation of document.write
//
//  Threading:  Called on the script thread.
//
//              Must be called between Suspend() and Resume(), after
//              InsertText().
//
//-------------------------------------------------------------------------
HRESULT
CHtmPre::TokenizeText(BOOL *pfEmpty)
{
    PerfDbgLog(tagHtmPre, this, "+CHtmPre::TokenizeText");

    HRESULT hr;

    Assert(_cSuspended);// Must be called between Suspend and Resume
    Assert(!_fSuspend); // InsertText should have cleared _fSuspend

    *pfEmpty = (_pch == _pchEnd);

    if (*pfEmpty)
        return(S_OK);

    hr = THR_NOTRACE(Tokenize());

    if (hr == E_PENDING)
    {
        Assert(_fSuspend);
        hr = S_OK;

        // Flush the constructed but not signalled tag now that the state machine is in a clean
        // state and can tolerate being re-entered.

        _pHtmTagStm->WriteTagEnd();
        _pHtmTagStm->Signal();
    }
    else
    {
        Assert(hr || _pch == _pchEnd);

        if (_pch != _pchEnd)
            *pfEmpty = TRUE;    // Defensive: break infinite loop
    }

    if (hr)
        goto Cleanup;

Cleanup:
    PerfDbgLog2(tagHtmPre, this, "-CHtmPre::TokenizeText (*pfEmpty=%s,hr=%lX)",
        *pfEmpty, hr);
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Function:   ProcessValueEntities
//
//  Synopsis:   Used to process a value string in place (may shrink).
//              Turns entities into unicode chars, and truncates on \0.
//
//  Arguments:  pchValue = the beginning of the value string (not incl ")
//              pcch[in] = the initial size of the value string
//                  [out]= the final size
//
//-------------------------------------------------------------------------

#ifndef NO_UTF16
TCHAR *
WriteNonBMPXChar(TCHAR * p, XCHAR ch)
{
    *p++ = HighSurrogateCharFromUcs4(ch);
    *p++ = LowSurrogateCharFromUcs4(ch);

    return p;
}

inline TCHAR *
WriteXChar(TCHAR * p, XCHAR ch)
{
    if (ch < 0x10000)
    {
        *p++ = ch;
    }
    else
    {
        p = WriteNonBMPXChar(p,ch);
    }

    return p;
}
#else
inline TCHAR *
WriteXChar(TCHAR * p, XCHAR ch)
{
    *p++ = ch;
    return p;
}
#endif

void
ProcessValueEntities(TCHAR *pchValue, ULONG *pcch)
{
    TCHAR *pch      = pchValue;
    TCHAR *pchEnd   = pchValue + *pcch;
    TCHAR *pchTo;
    TCHAR *pchWord;
    TCHAR ch        = _T(' ');
    XCHAR chEnt;

    // fast scan to do nothing if there are no special chars
    while (pch < pchEnd)
    {
        ch = *pch;
        if (!ch || ch=='&')
            break;
        pch++;
    }

    pchTo = pch;

    // mini entity scanner (could be leaner)
    while (ch && pch < pchEnd)
    {
        // entity; copy &
        *(pchTo++) = ch;

        ch = *(++pch);

        chEnt = 0;

        // numbered or hex entity
        if (ch == '#')
        {
            *(pchTo++) = ch;
            ch = *(++pch);

            // hex entity
            if(ch == 'X' || ch == 'x')
            {
                ch = *(++pch);
                pchWord = pch;
                while (pch < pchEnd && ISHEX(ch))
                    ch = *(++pch);
                chEnt = EntityChFromHex(pchWord, PTR_DIFF(pch, pchWord));
                if (chEnt)
                {
                    pchTo = WriteXChar(pchTo-2, chEnt);
                }
                else
                {
                    ch = *(pch = pchWord);
                }
            }
            // numbered entity
            else
            {
                pchWord = pch;
                while (pch < pchEnd && ISDIGIT(ch))
                    ch = *(++pch);
                chEnt = EntityChFromNumber(pchWord, PTR_DIFF(pch, pchWord));
                if (chEnt)
                {
                    pchTo = WriteXChar(pchTo-2, chEnt);
                }
                else
                {
                    ch = *(pch = pchWord);
                }
            }
        }

        // named entity
        else
        {
            pchWord = pch;

            do
            {
                pch++;
            } while (pch <= pchEnd && PTR_DIFF(pch, pchWord) < MAXENTYLEN && ISENTYC(*pch));

            // Fix for IE5 10370: require non-alphanum or end-of-value to terminate all named entities
            if (pch == pchEnd || !ISENTYC(*pch))
            {
                chEnt = EntityChFromName(pchWord, PTR_DIFF(pch, pchWord), HashString(pchWord, PTR_DIFF(pch, pchWord), 0));

#ifndef NO_UTF16
                AssertSz(chEnt < 0x10000, "Should be no non-BMP named entities.");
#endif

                // Fix for BUG 31357: require ';' for named entities not in the HTML1 set
                if (!IS_HTML1_ENTITY_CHAR(chEnt) && (pch > pchEnd || *pch != _T(';')))
                {
                    chEnt = 0;
                }
            }

            if (chEnt)
            {
                *(pchTo-1) = chEnt;
            }
            else
            {
                pch = pchWord;
            }

            ch = *pch;
        }

        // semicolon
        if (chEnt && pch < pchEnd && ch==';')
            pch++;

        // copy chars up to next special char
        while (pch < pchEnd)
        {
            ch = *pch;
            if (!ch || ch=='&')
                break;
            pch++;
            *(pchTo++) = ch;
        }
    }

    *pcch = PTR_DIFF(pchTo, pchValue);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::GoIntoPlaintextMode
//
//  Synopsis:   Insert an XMP into the stream and go into plaintext mode.
//
//-------------------------------------------------------------------------
HRESULT
CHtmPre::GoIntoPlaintextMode()
{
    PerfDbgLog(tagHtmPre, this, "+CHtmPre::GoIntoPlaintextMode");

    HRESULT hr;

    // Dump out an <XMP> tag

    hr = THR(_pHtmTagStm->WriteTag(ETAG_XMP));
    if (hr)
        goto Cleanup;

    // Tell the tokenizer we are in a plaintext mode that will never end
    _state = TS_PLAINTEXT;

    _pHtmTagStm->Signal();

Cleanup:
    PerfDbgLog(tagHtmPre, this, "-CHtmPre::GoIntoPlaintextMode");
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::DoTokenizeOneTag (OutputTag)
//
//  Synopsis:   Tokenizes a single tag, then if successful, processes
//              name and attribute strings in place, then sends
//              then downstream.
//
//              Tag starts at _pchStart and runs to _pch.
//
//              Returns S_FALSE if tag could not be processed as markup
//              so that the tokenizer can roll back and reprocess as text.
//
//-------------------------------------------------------------------------
HRESULT
CHtmPre::DoTokenizeOneTag(TCHAR *pchStart, ULONG cch, CHtmTagStm *pHts, CHtmPre *pHtmPre, ULONG ulLine, ULONG ulOff, ULONG fCount, DWORD *potCode)
{
    //
    // ** NOTE ** if you make any local variables which point into the
    // data stream here (like pch, pchLimit, pchTagName), you'll
    // have to update them after the call to SaveSource further down.
    //

    TCHAR *pch = pchStart;
    TCHAR *pchLimit = pch + cch - 1;
    TCHAR ch;
    CHtmTag * pht;
    CHtmTag::CAttr *pAttr;
    BOOL fEnd = FALSE;
    TCHAR * pchTagName;
    UINT cchTagName;
    ELEMENT_TAG etag;
    int c;
    HRESULT hr;
    BOOL fSlowProcess;
#ifdef VSTUDIO7
    BOOL fIsIdentity = FALSE;
#endif //VSTUDIO7

#ifdef XMV_PARSE
    BOOL fNSQualified = FALSE;
    TCHAR * pchTagNameUnqualified = NULL;
    BOOL fXML = pHtmPre ? pHtmPre->_fXML : FALSE;
#endif   
    
    Assert(cch >= 2);
    Assert(*pch == _T('<'));
    Assert(*pchLimit == _T('>'));
    Assert(!!pHtmPre == !!potCode);

    // '<'
    ch = *(++pch);

    // tag name, possible /
    if (ch == _T('/'))
    {
        fEnd = TRUE;
        ch = *(++pch);
    }

    pchTagName = pch;

    // scan to end of tag name (allow ':'-s in the name)
    while (ISNAMCH(ch))
    {
        ch = *(++pch);
#ifdef XMV_PARSE
        if (fXML && ch == _T(':') && !fNSQualified)
        {
            pchTagNameUnqualified = pch + 1;
            fNSQualified = TRUE;
        }   
#endif
    }

    cchTagName = pch - pchTagName;

#ifdef XMV_PARSE
    if (fXML && !fNSQualified)
        etag = ETAG_GENERIC;
    else
    {
        if (fXML && fNSQualified && 0 == StrCmpNIC(pchTagName, _T("html:"), 5))
        {
            pchTagName = pchTagNameUnqualified;
            cchTagName = pch - pchTagNameUnqualified;
        }
        etag = EtagFromName(pchTagName, cchTagName);
    }
#else
    etag = EtagFromName(pchTagName, cchTagName);
#endif
    
    if (pHtmPre)
    {
        // check if the tag is known to be generic
        if (!etag)
        {
            if (ETAG_GENERIC_LITERAL == pHtmPre->_etagLiteral &&
                pHtmPre->_cstrLiteral.Length() == cchTagName &&
                0 == StrCmpNIC(pHtmPre->_cstrLiteral, pchTagName, cchTagName))
            {
                etag = ETAG_GENERIC_LITERAL;
            }
            else
            {
                etag = pHtmPre->GetGenericTag(pchTagName, cchTagName);

                if (etag == ETAG_NULL)
                {
#ifdef VSTUDIO7
                    etag = pHtmPre->GetIdentityBaseTag(pchTagName, cchTagName);
                    if (etag == ETAG_NULL)
                        etag = ETAG_UNKNOWN;
                    else
                        fIsIdentity = TRUE;
#else
                    etag = ETAG_UNKNOWN;
#endif //VSTUDIO7
                }
            }
        }

        // hack for literals: return with OT_REJECT without initializing if etag doesn't match
        if (pHtmPre->_etagLiteral && (!fEnd || etag != pHtmPre->_etagLiteral))
        {
            *potCode = OT_REJECT;
            return S_OK;
        }


        // hack for object/applet: echo source if needed, before outputting tag
        if (pHtmPre->_etagEchoSourceEnd)
        {
            if (fEnd ? etag == pHtmPre->_etagEchoSourceEnd : pHtmPre->_atagEchoSourceBegin && IsEtagInSet(etag, pHtmPre->_atagEchoSourceBegin))
            {
                // turn off echoing here, and continue...
                pHtmPre->_atagEchoSourceBegin = NULL;
                pHtmPre->_etagEchoSourceEnd = ETAG_NULL;
            }
            else
            {
                // Save source before overwriting it with \0's
                hr = THR(pHtmPre->SaveSource(pchStart, cch));
                if (hr)
                    goto Cleanup;

                Assert(*pchTagName); // Make sure we don't AV.
            }
        }
        *potCode = OT_NORMAL;
    }


    TraceTag((tagToken, "   OutputTag"));

    hr = THR(pHts->WriteTagBeg(etag, &pht));
    if (hr)
        goto Cleanup;

    if (fEnd)
        pht->SetEnd();

#ifdef VSTUDIO7
    if (fIsIdentity)
        pht->SetDerivedTag();
#endif //VSTUDIO7

    fSlowProcess = FALSE;

    goto VALSPACE;

    // for each attribute-value pair
    while (ch != _T('>') && pht->GetAttrCount() < MAX_ATTR_COUNT)
    {
        Assert(ISATTCH(ch) || ISQUOTE(ch) || ch == _T('='));

        hr = THR(pHts->WriteTagGrow(&pht, &pAttr));
        if (hr)
            goto Cleanup;

        pAttr->_pchName = pch;

    ATTLOOP:
        while (ISATTCH(ch))
            ch = *(++pch);

        if (ISQUOTE(ch))
        {
            // Fix for IE4 bug 44892: replace leading quotes in attribute name with '?'
            // Fix for IE5 bug 26884: replace all quotes in attribute name with '?'
            
            *pch = _T('?');
            ch = *(++pch);
            goto ATTLOOP;
        }

        pAttr->_cchName = pch - pAttr->_pchName;

        // null-terminate attribute name

        *pch = '\0';
        
        // a=b without spaces is quick

        if (ch == _T('='))
        {
            ch = *(++pch);
            if (ISNONSP(ch))
                goto VALSTART;
            else
                goto EQSPACE;
        }
        
    ATTSPACE:
        while (ISSPACR(ch))
            ch = *(++pch);

        // count line

        Assert(ch != _T('\n') || pch > pchStart);

        if (ch == _T('\r') || ch == _T('\n'))
        {
            if (ch == _T('\r') || *(pch-1) != _T('\r'))
                ulLine += fCount;
            ch = *(++pch);
            goto ATTSPACE;
        }

        // if no = look for next attr
        if (ch != _T('='))
        {
            pAttr->_pchVal  = NULL;
            pAttr->_cchVal  = 0;
            goto ENDATT;
        }

        // eat equals sign
        ch = *(++pch);

        // skip space after equals sign

    EQSPACE:
        while (ISSPACR(ch))
            ch = *(++pch);

        // count line
        Assert(ch != _T('\n') || pch > pchStart);

        if (ch == _T('\r') || ch == _T('\n'))
        {
            if (ch == _T('\r') || *(pch-1) != _T('\r'))
                ulLine += fCount;
            ch = *(++pch);
            goto EQSPACE;
        }

    VALSTART:
    
        // eat quoted value
        if (ISQUOTE(ch))
        {
            TCHAR chQuote = ch;

            // eat leading quote
            ch = *(++pch);

            pAttr->_pchVal = pch;
            pAttr->_ulOffset = ulOff + pch - pchStart;
            pAttr->_ulLine = ulLine;

            // eat quoted value, paying attention to tag limit
            while (pch < pchLimit && ch != chQuote)
            {
                if (ch == _T('&'))
                    fSlowProcess = TRUE;
                ch = *(++pch);
            }

            pAttr->_cchVal = pch - pAttr->_pchVal;

            // null-terminate
            *pch = _T('\0');

            // eat trailing quote
            if (pch < pchLimit)
                ch = *(++pch);
        }

        // eat unquoted value
        else
        {
            pAttr->_pchVal = pch;
            pAttr->_ulOffset = ulOff + (fCount ? pch - pchStart : 0);
            pAttr->_ulLine = ulLine;

        VALLOOP:
            while (ISVALCH(ch))
                ch = *(++pch);

            if (ch == '&')
            {
                fSlowProcess = TRUE;
                ch = *(++pch);
                goto VALLOOP;
            }

            pAttr->_cchVal = pch - pAttr->_pchVal;

            // null-terminate
            *pch = _T('\0');
        }

#if DBG==1
        if (IsTagEnabled(tagNoValues))
        {
            pAttr->_pchVal = NULL;
            pAttr->_cchVal = 0;
            pAttr->_ulOffset = 0;
            pAttr->_ulLine = 0;
        }
        if (IsTagEnabled(tagEmptyValues))
        {
            pAttr->_cchVal = 0;
        }
#endif

        // skip space after value
    VALSPACE:
        while (ISSPACR(ch))
            ch = *(++pch);

        // count line
        Assert(ch != _T('\n') || pch > pchStart);

        if (ch == _T('\r') || ch == _T('\n'))
        {
            if (ch == _T('\r') || *(pch-1) != _T('\r'))
                ulLine += fCount;
            ch = *(++pch);
            goto VALSPACE;
        }

    ENDATT:
        // trailing '/'
        if (ch == _T('/'))
        {
            ch = *(++pch);

            if (ch != _T('>'))
                goto VALSPACE;

            pht->SetEmpty();
        }
    }

    Assert(ch == _T('>') || ch == _T('\0') || pht->GetAttrCount() == MAX_ATTR_COUNT);

    // since we are matching a Netscape quote bug,
    // we may not have consumed all the characters, so count lines
    while (pch < pchLimit)
    {
        if (ch == _T('\r') || (ch == _T('\n') && *(pch-1) != _T('\r')))
            ulLine += fCount;
        ch = *(++pch);
    }

    if (etag == ETAG_SCRIPT && !fEnd)
    {
#ifdef VSTUDIO7
        Assert(!fIsIdentity);
#endif //VSTUDIO7
        pht->SetOffset(ulOff + cch);
        pht->SetLine(ulLine);
    }
#ifdef VSTUDIO7
    else if (!fIsIdentity && (etag > ETAG_UNKNOWN && etag < ETAG_GENERIC))
#else
    else if (etag > ETAG_UNKNOWN && etag < ETAG_GENERIC)
#endif //VSTUDIO7
    {
        if (pht->GetAttrCount() == 0)
        {
            pht->SetTiny();
        }
        else
        {
            pht->SetPch(NULL);
            pht->SetCch(0);
        }
    }
    else
    {
        pchTagName[cchTagName] = 0;
        pht->SetPch(pchTagName);
        pht->SetCch(cchTagName);
    }

    if (fSlowProcess)
    {
        // flag set means that we possibly have entities in values
        
        c = pht->GetAttrCount();
        
        Assert(c);
        
        // collapse entities and re-null-terminate strings
        for (pAttr = pht->GetAttr(0); c; pAttr++, c--)
        {
            Assert(pAttr->_pchName[pAttr->_cchName] == _T('\0'));

            if (pAttr->_pchVal)
            {
                ProcessValueEntities(pAttr->_pchVal, &(pAttr->_cchVal));
                pAttr->_pchVal[pAttr->_cchVal] = _T('\0');
            }
        }
    }

    const CTagDesc *ptd;

    // default next state is TS_TEXT; can be changed by SpecialToken
    if (pHtmPre)
    {
        pHtmPre->_state = TS_TEXT;

        ptd = TagDescFromEtag(etag);

        // Process special tags
        if (ptd)
        {
            // handle ClarisWorks header - end on a recognized tag only
            pHtmPre->_fSuppressLeadingText = FALSE;

            // literal HTML tags
            if (ptd->HasFlag(TAGDESC_LITERALTAG))
            {
                pHtmPre->_etagLiteral = fEnd ? ETAG_NULL : etag;
                if (ptd->HasFlag(TAGDESC_LITERALENT))
                {
                    pHtmPre->_fLiteralEnt = !fEnd;
                    if (fEnd)
                        pHtmPre->_cstrLiteral.Free();
                    else
                        pHtmPre->_cstrLiteral.Set(pchTagName, cchTagName);
                }
            }

            // other special tags
            if (ptd->HasFlag(TAGDESC_SPECIALTOKEN))
            {
                hr = THR(pHtmPre->SpecialToken(pht));
                if (hr)
                    goto Cleanup;
            }
        }

        if (pHtmPre->_state == TS_SUSPEND)
        {
            // We are going to be suspending and returning E_PENDING to the toplevel function
            // (either Exec() or TokenizeText()).  They will take responsibility for calling
            // WriteTagEnd() and signalling.

            goto Cleanup;
        }
    }

    pHts->WriteTagEnd();

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::OutputDocSize
//
//  Synopsis:   Outputs the number of character parsed
//
//-------------------------------------------------------------------------
HRESULT
CHtmPre::OutputDocSize( )
{
    Assert(_ulLastCharsSeen <= _ulCharsEnd);

    //
    // If we perform a document.write immediately followed by a
    // document.close, we'll end up calling OutputDocSize twice.
    // In this case, don't send the second ETAG_RAW_DOCSIZE
    // token. (t-chrisr)
    //

    if (_ulLastCharsSeen != _ulCharsEnd)
    {
        _ulLastCharsSeen = _ulCharsEnd;

        TraceTag((tagToken, "   OutputDocSize"));
        RRETURN(THR(_pHtmTagStm->WriteTag(ETAG_RAW_DOCSIZE, _ulCharsEnd, 0)));
    }

    return(S_OK);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::OutputEntity
//
//  Synopsis:   Outputs one-char entity, from _pchStart to _pch.
//
//-------------------------------------------------------------------------
HRESULT
CHtmPre::OutputEntity(TCHAR *pchStart, ULONG cch, XCHAR chEnt)
{
    HRESULT hr;
    
    TraceTag((tagToken, "   OutputEntity"));
    
    if (_etagEchoSourceEnd)
    {
        // Save entity before overwriting it with chEnt
        hr = THR(SaveSource(pchStart, cch));
        if (hr)
            goto Cleanup;
    }

#ifndef NO_UTF16
    if (chEnt < 0x10000)
    {
        // overwrite '&'
        *pchStart = chEnt;

        hr = THR(_pHtmTagStm->WriteTag(ETAG_RAW_TEXT, pchStart, 1, !!(chEnt < 0x80) ));
    }
    else
    {
        Assert(cch >= 2);

        // convert non-BMP char in to surrogate pair

        chEnt -= 0x10000;
        pchStart[0] = 0xd800 + (chEnt >> 10);
        pchStart[1] = 0xdc00 + (chEnt & 0x3ff);

        hr = THR(_pHtmTagStm->WriteTag(ETAG_RAW_TEXT, pchStart, 2, FALSE ));
    }
#else
    // overwrite '&'
    *pchStart = chEnt;

    hr = THR(_pHtmTagStm->WriteTag(ETAG_RAW_TEXT, pchStart, 1, !!(chEnt < 0x80) ));
#endif

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::EntityChFromNumber
//
//  Synopsis:   Digits->tchar, or '?' if too many digits
//
//-------------------------------------------------------------------------
XCHAR
EntityChFromNumber(TCHAR *pchWord, ULONG cch)
{
    XCHAR ch = XCHAR('?');

#ifndef NO_UTF16
    if (cch <= 7)
#else
    if (cch <= 5)
#endif
    {
        TCHAR   ach[8];
        int     i;

        // The W3C spec has declared that all numerically encoded entities
        // should be treated as Unicode.  For the range 160-255, this is
        // exactly the same as Latin-1 (Windows-1252).  Unicode, however,
        // does not define glyphs for the range 128-159 (they are control
        // characters.)  For maximum compatibility, we will treat those
        // characters also as Latin-1.

        _tcsncpy(ach, pchWord, cch);
        ach[cch]=_T('\0');
        i = StrToInt(ach);

#ifndef NO_UTF16
        if (i < 0x10000)
        {
#endif
            if (InRange(TCHAR(i), 0x80, 0x9f))
            {
                ch = g_achLatin1MappingInUnicodeControlArea[i-0x80];
            }
            else if (IsValidWideChar(TCHAR(i)))
            {
                // Exclude the Private Use Area, so as to not confuse
                // the post-parser.

                ch = XCHAR(i);
            }
#ifndef NO_UTF16
        }
        else if (i < 0x110000)
        {
            ch = XCHAR(i);
        }
#endif
    }
    return ch;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::EntityChFromHex
//
//  Synopsis:   Hex->tchar, or '?' if too many digits
//
//-------------------------------------------------------------------------
XCHAR
EntityChFromHex(TCHAR *pchWord, ULONG cch)
{
    XCHAR ch = XCHAR('?');

#ifndef NO_UTF16
    if (cch <= 6)
#else
    if (cch <= 4)
#endif
    {
        // initialize the string so hex value can be evaluated by wcstol()
        TCHAR   ach[9] = _T("0x");
        TCHAR*  pEnd;
        int     i;

        // The W3C spec has declared that all hex encoded entities
        // should be treated as Unicode.  For the range 0xA0-0xFF, this is
        // exactly the same as Latin-1 (Windows-1252).  Unicode, however,
        // does not define glyphs for the range 0x80-0x9F (they are control
        // characters.)  For maximum compatibility, we will also treat those
        // characters as Latin-1.

        _tcsncpy(ach+2, pchWord, cch);
        ach[cch+2]=_T('\0');
        // convert the hex string (base 16)
        i=(int) wcstol(ach, &pEnd, 16);

#ifndef NO_UTF16
        if (i < 0x10000)
        {
#endif
            if (InRange(TCHAR(i), 0x80, 0x9f))
            {
                ch = g_achLatin1MappingInUnicodeControlArea[i-0x80];
            }
            else if (IsValidWideChar(TCHAR(i)))
            {
                // Exclude the Private Use Area, so as to not confuse
                // the post-parser.
                ch = XCHAR(i);
            }
#ifndef NO_UTF16
        }
        else if (i < 0x110000)
        {
            ch = XCHAR(i);
        }
#endif
    }
    return ch;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::OutputComment
//
//  Synopsis:   Outputs a comment.  The comment lies between _pchStart and _pch.
//
//-------------------------------------------------------------------------
HRESULT
CHtmPre::OutputComment(TCHAR *pch, ULONG cch)
{
    HRESULT hr;
    
    // Handle ClarisWorks header
    _fSuppressLeadingText = FALSE;

    TraceTag((tagToken, "   OutputComment"));
    
    if (_etagEchoSourceEnd)
    {
        // Save comment text if needed
        hr = THR(SaveSource(pch, cch));
        if (hr)
            goto Cleanup;
    }

    hr = THR(_pHtmTagStm->WriteTag(ETAG_RAW_COMMENT, pch, cch, FALSE));

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::OutputConditional
//
//  Synopsis:   Outputs a conditional comment.  The current behavior is
//              to throw away the text and output nothing.
//
//-------------------------------------------------------------------------
HRESULT
CHtmPre::OutputConditional(TCHAR *pch, ULONG cch, CONDVAL val)
{
    HRESULT hr = S_OK;
    
    // Handle ClarisWorks header
    _fSuppressLeadingText = FALSE;

    TraceTag((tagToken, "   OutputConditional"));
    
    if (_etagEchoSourceEnd)
    {
        // Save comment text if needed
        hr = THR(SaveSource(pch, cch));
        if (hr)
            goto Cleanup;
    }

#ifdef CLIENT_SIDE_INCLUDE_FEATURE
    if (val == COND_INCLUDE) 
    {
        // the case for <![include "http://foobar/..."]>
        CHtmTag * pht;

        hr = THR(_pHtmTagStm->WriteTagBeg(ETAG_RAW_INCLUDE, &pht));

        pht->SetPch(pch);
        pht->SetCch(cch);
    }
    else 
#endif
    {
        hr = THR(_pHtmTagStm->WriteTag(ETAG_RAW_TEXTFRAG, pch, cch, FALSE));
    }

Cleanup:
    RRETURN(hr);
}
//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::OutputEof
//
//  Synopsis:   Outputs an EOF into the stmchan
//
//-------------------------------------------------------------------------
void
CHtmPre::OutputEof(HRESULT hr)
{
    TraceTag((tagToken, "   OutputEof"));
    _pHtmInfo->OnBindResult(_pDwnBindData->GetBindResult());
    _pHtmTagStm->WriteEof(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::SaveSource
//
//  Synopsis:   Sends a token containing the original source.
//              This token exists from _pchStart to _pch.
//
//              ** NOTE ** that since this involves copying
//              of buffers around, _pchStart and _pch and such
//              might get changed by this command.  Any local
//              copies of these variables will have to be re-synced
//              after this call.  pdpchShift returns the amount
//              that the buffer got moved.  You'll have to add this
//              value to all your local pointers after the call to save source.
//              pdpchShift can be NULL.
//
//-------------------------------------------------------------------------
HRESULT
CHtmPre::SaveSource(TCHAR *pch, ULONG cch)
{
    HRESULT hr = S_OK;
    CHtmTag *pht;

    hr = THR(_pHtmTagStm->WriteSource(pch, cch));
    if (hr) 
        goto Cleanup;

    hr = THR(_pHtmTagStm->WriteTagBeg(ETAG_RAW_SOURCE, &pht));
    if (hr)
        goto Cleanup;

    pht->SetHtmTagStm(_pHtmTagStm);
    pht->SetSourceCch(cch);

    _pHtmTagStm->WriteTagEnd();

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::SpecialToken
//
//  Synopsis:   Used to do special things like kick off downloads,
//              track the base tag, suspend on script tags, etc.
//
//              Called only on tags with the TAGDESC_SPECIALTOKEN flag.
//
//              Called right before CMD_ENDTOKEN is written by OutputTag.
//              _aAttr holds all the attributes and values, and
//              _fEndTag indicates an end tag.
//
//              May write hidden values to the postchan and change _state.
//
//  Arguments:  etag     = the tag
//
//-------------------------------------------------------------------------
ELEMENT_TAG s_atagAppletObject[];
ELEMENT_TAG s_atagIframe[];

// CHROME
// We need to do comparisions on an object's CLASSID attribute
extern HRESULT CLSIDFromHtmlString(TCHAR *pchClsid, CLSID *pclsid);

HRESULT
CHtmPre::SpecialToken(CHtmTag *pht)
{
    PerfDbgLog2(tagHtmPre, this, "+CHtmPre::SpecialToken <%s%ls>",
        pht->IsEnd() ? "/" : "", NameFromEtag(pht->GetTag()));

    HRESULT hr = S_OK;
    CHtmTag::CAttr *pAttr;

    if (pht->IsEnd())
    {
        switch (pht->GetTag())
        {
        case ETAG_SCRIPT:
            if (_fScriptDefer)
            {
                // if DEFER attribute present, set a bit and don't suspend the preparser
                pht->SetDefer();
            }
            else
            {
                // otherwise, suspend preparser
                _state = TS_SUSPEND;
            }
            break;

        case ETAG_OBJECT:
        case ETAG_APPLET:
        case ETAG_IFRAME:
        case ETAG_NOFRAMES:
        case ETAG_NOSCRIPT:
        case ETAG_NOEMBED:

            // When we see the end tag for any of these, we'll start speculative download again
            
            if (0 != _cDownloadSupression)
                _cDownloadSupression--;

            break;
        }
    }
    else
    {
        switch (pht->GetTag())
        {
        case ETAG_SCRIPT:

            pAttr = pht->AttrFromName(_T("DEFER"));
            _fScriptDefer = (pAttr && !pAttr->_cchVal);

            // grab SRC bits for downloaded script
            pAttr = pht->AttrFromName(_T("SRC"));
            if (!pAttr)
                break;
            hr = THR(AddDwnCtx(DWNCTX_BITS, pAttr->_pchVal, pAttr->_cchVal, NULL, PROGSINK_CLASS_CONTROL));
            if (hr)
                goto Cleanup;

            break;

        case ETAG_PLAINTEXT:
            _state = TS_PLAINTEXT;
            break;

        case ETAG_BASE:
            // deal with base href change

            pAttr = pht->AttrFromName(_T("HREF"));
            if (!pAttr)
                break;

            hr = SetUrlDefaultScheme(pAttr->_pchVal, &_cstrBase);
            if (hr)
                goto Cleanup;

            if (_pInetSess && !CanPrefetchWithScheme(_cstrBase))
            {
                ClearInterface(&_pInetSess);
            }

            break;

        case ETAG_BODY:
        case ETAG_TABLE:
            // Prevent any palette info from sneaking in once an image is found.
            // This ensures that all images for this DwnDoc will get the same palette
            TraceTag((tagPalette, "Found an image tag, palette meta will no longer be accepted"));
            _pDwnDoc->PreventAuthorPalette();

            if (!(_pDwnDoc->GetLoadf() & DLCTL_DLIMAGES))
                break;

            pAttr = pht->AttrFromName(_T("BACKGROUND"));
            if (!pAttr)
                break;
            hr = THR(AddDwnCtx(DWNCTX_IMG, pAttr->_pchVal, pAttr->_cchVal));
            if (hr)
                goto Cleanup;
            break;

        case ETAG_INPUT:

            // nothing special if type!=image
            pAttr = pht->AttrFromName(_T("TYPE"));
            if (!pAttr)
                break;
            if (_tcsnicmp(_T("IMAGE"),-1, pAttr->_pchVal, pAttr->_cchVal))
                break;

            // fallthrough to ETAG_IMG ...

        case ETAG_IMG:

            // Prevent any palette info from sneaking in once an image is found.
            // This ensures that all images for this DwnDoc will get the same palette
            TraceTag((tagPalette, "Found an image tag, palette meta will no longer be accepted"));
            _pDwnDoc->PreventAuthorPalette();

            if (!(_pDwnDoc->GetLoadf() & DLCTL_DLIMAGES))
                break;

            // Supress speculative downloads if special-context count is nonzero
            
            if (_cDownloadSupression != 0)
                break;

            pAttr = pht->AttrFromName(_T("SRC"));
            if (!pAttr)
            {
                pAttr = pht->AttrFromName(_T("LOWSRC"));
                if (!pAttr)
                    break;
            }
            hr = THR(AddDwnCtx(DWNCTX_IMG, pAttr->_pchVal, pAttr->_cchVal));
            if (hr)
                goto Cleanup;
            break;

        case ETAG_BGSOUND:

            // Supress speculative downloads if special-context count is nonzero
            
            if (_cDownloadSupression != 0)
                break;

            if (!(_pDwnDoc->GetLoadf() & DLCTL_BGSOUNDS))
                break;

            pAttr = pht->AttrFromName(_T("SRC"));
            if (!pAttr)
                break;
            hr = THR(AddDwnCtx(DWNCTX_FILE, pAttr->_pchVal, pAttr->_cchVal));
            if (hr)
                goto Cleanup;
            break;

        case ETAG_LINK:

            // Supress speculative downloads if special-context count is nonzero
            
            if (_cDownloadSupression != 0)
                break;

            pAttr = pht->AttrFromName(_T("REL"));
            if (!pAttr)
                break;

            // if stylesheet
            if (0 == _tcsnicmp(pAttr->_pchVal, pAttr->_cchVal, _T("STYLESHEET"), 10))
            {
                //
                // try to launch download
                //

                pAttr = pht->AttrFromName(_T("HREF"));
                if (!pAttr)
                    break;

                hr = THR(AddDwnCtx(DWNCTX_BITS, pAttr->_pchVal, pAttr->_cchVal));
                if (hr)
                    goto Cleanup;
            }

            break;

        case ETAG_META:

            // handle cookies
            hr = THR(HandleMETA(pht));
            if (hr)
                goto Cleanup;
            break;

        case ETAG_FRAMESET:

            // Supress all speculative downloads after the first FRAMESET tag is seen
            
            ClearInterface(&_pInetSess);
            break;

        case ETAG_OBJECT:
        case ETAG_APPLET:
        case ETAG_IFRAME:
        
            // EchoSource mode starts now, and will stop if we see a begin <OBJECT> or <APPLET>
            // (after which it will start again)
            
            if (!_etagEchoSourceEnd)
                _atagEchoSourceBegin = (pht->GetTag() == ETAG_IFRAME ? s_atagIframe : s_atagAppletObject);

            // fall through
            
        case ETAG_NOEMBED:
        case ETAG_NOFRAMES:
        case ETAG_NOSCRIPT:
        
            // EchoSource mode starts now, and will stop if we see a end tag corresponding to the begin tag
            
            if (!_etagEchoSourceEnd)
                _etagEchoSourceEnd = pht->GetTag();

            // fall through some more

        
            // Inside these tags we supress speculative download.
            // Note that even if we guess wrong and supress download when we shouldn't, the
            // download will be kicked off again on the UI thread. (And if we don't supress
            // download when we should have, the UI thread will kill the extra download.)
            
            _cDownloadSupression++;
            
            break;


#ifdef VSTUDIO7
        case ETAG_RAW_TAGSOURCE:
            HandleTagsourcePI(pht);
            break;

        case ETAG_RAW_FACTORY:
            _state = TS_SUSPEND;
            break;
#endif //VSTUDIO7

        }
    }
Cleanup:
    PerfDbgLog1(tagHtmPre, this, "-CHtmPre::SpecialToken (hr=%lX)", hr);
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::AddDwnCtx
//
//  Synopsis:   Downloads a specific type of URL, stores the load ctx
//              in a slot, and writes a hidden attribute with the slot
//              number into the stream.
//
//-------------------------------------------------------------------------
HRESULT
CHtmPre::AddDwnCtx(UINT dt, LPCTSTR pchUrl, int cchUrl,
    CDwnBindData * pDwnBindData, DWORD dwProgClass)
{
    TCHAR       ach[pdlUrlLen];
    ULONG       cch;
    TCHAR *     pchUrlCombined = ach;
    CDwnCtx *   pDwnCtx = NULL;
    BOOL        fLoad   = TRUE;
    DWNLOADINFO dli     = { 0 };
    HRESULT     hr      = S_OK;

    Assert(dt == DWNCTX_IMG || dt == DWNCTX_BITS || dt == DWNCTX_FILE);

    if (!pDwnBindData && !_pInetSess)
        goto Cleanup;

    #if DBG==1
    if (IsTagEnabled(tagHtmPreNoAutoLoad))
        goto Cleanup;
    #endif

    #if DBG==1 || defined(PERFTAGS)
    if (IsPerfDbgEnabled(tagDwnBindSlow))
        goto Cleanup;
    #endif

    #if DBG==1 || defined(PERFTAGS)
    {
        TCHAR chT = cchUrl ? pchUrl[cchUrl] : 0;
        if (cchUrl) *(TCHAR *)(pchUrl + cchUrl) = 0;
        PerfDbgLog3(tagHtmPre, this, "CHtmPre::AddDwnCtx %s %ls %s",
            dt == DWNCTX_IMG ? "DWNCTX_IMG" :
            dt == DWNCTX_BITS ? "DWNCTX_BITS" :
            dt == DWNCTX_FILE ? "DWNCTX_FILE" : "DWNCTX_???",
            cchUrl ? pchUrl : g_Zero.ach,
            pDwnBindData ? "(via CDwnBindData)" : "");
        if (cchUrl) *(TCHAR *)(pchUrl + cchUrl) = chT;
    }
    #endif

    // Temporarily terminate the URL (guaranteed to have enough space)

    if (cchUrl == 0)
    {
        ach[0] = 0;
    }
    else if (pDwnBindData)
    {
        if (cchUrl > ARRAY_SIZE(ach) - 1)
            cchUrl = ARRAY_SIZE(ach) - 1;
        memcpy(ach, pchUrl, cchUrl * sizeof(TCHAR));
        ach[cchUrl] = 0;
    }
    else
    {
        TCHAR * pchBase = _cstrBase ? _cstrBase : _cstrDocUrl;

        Assert(CanPrefetchWithScheme(pchBase));

        TCHAR ch = pchUrl[cchUrl];
        *(TCHAR *)(pchUrl + cchUrl) = 0;

        hr = THR(CoInternetCombineUrl(pchBase, pchUrl,
            URL_ESCAPE_SPACES_ONLY | URL_BROWSER_MODE,
            ach, ARRAY_SIZE(ach), &cch, 0));

        *(TCHAR *)(pchUrl + cchUrl) = ch;

        if (hr || !CanPrefetchWithScheme(ach))
            goto Cleanup;
    }

    dli.pDwnBindData    = pDwnBindData;
    dli.pDwnDoc         = _pDwnDoc;
    dli.pInetSess       = _pInetSess;
    dli.pchUrl          = pchUrlCombined;
    dli.fForceInet      = TRUE;
    dli.dwProgClass     = dwProgClass;

    hr = THR(NewDwnCtx(dt, fLoad, &dli, &pDwnCtx));
    if (hr)
        goto Cleanup;

    hr = THR(_pHtmLoad->AddDwnCtx(dt, pDwnCtx));
    if (hr)
        goto Cleanup;

    // We suppress errors because we don't want parsing to stop just
    // because we couldn't kick off a speculative download.  Also, we
    // expect some attempts to use InetSess to fail if, for example, it
    // determines that it needs to remap the namespace.  This download
    // will be retried on the UI thread later.

Cleanup:
    if (pDwnCtx)
        pDwnCtx->Release();
    return(S_OK);
}
#ifdef WIN16
#pragma code_seg("htmpre3_TEXT")
#endif

//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::HandleMETA
//
//  Synopsis:   META tag handling. This should only process values that
//              affect the current download, like "HTTP-EQUIV=Set-Cookie"
//              and "HTTP-EQUIV=Content-type".  Meta information that does
//              not affect the current download will be processed
//              after the download is complete...
//
//              Added support for the palette tag here so we have the
//              information in time for image decoding (michaelw)
//
//-------------------------------------------------------------------------
HRESULT
CHtmPre::HandleMETA(CHtmTag *pht)
{
    PerfDbgLog(tagHtmPre, this, "+CHtmPre::HandleMETA");

    HRESULT hr = S_OK;
    LPCTSTR pchActualURL;
    CHtmTag::CAttr *pAttrHttpEquiv;
    CHtmTag::CAttr *pAttrContent;

    // If the HTTP-EQUIV val is "Set-Cookie", then we want to tell
    // Wininet about the CONTENT val. If the content value contains
    // an equal-sign, it is interpreted as a name-value pair; i.e.,
    // it splits the string at that point and hands the two pieces to
    // InternetSetCookie() distinctly.

    pAttrHttpEquiv = pht->AttrFromName(_T("HTTP-EQUIV"));
    pAttrContent   = pht->AttrFromName(_T("CONTENT"));

    if (pAttrHttpEquiv && pAttrContent &&
        !_tcsnicmp(pAttrHttpEquiv->_pchVal, pAttrHttpEquiv->_cchVal, _T("Set-Cookie"), -1))
    {
        pchActualURL = _pHtmLoad->GetUrl();
        // If there's no URL for this doc, there will be no cache
        // entry to tweak (?)
        if (pchActualURL && *pchActualURL)
        {
            CStr cstrCookie;
            hr = cstrCookie.Set(pAttrContent->_pchVal, pAttrContent->_cchVal);
            if (hr)
                goto Cleanup;

            // note that "name=value" is sent as third argument, and second arg is NULL
            InternetSetCookie(pchActualURL, NULL, cstrCookie);
            goto Cleanup;  // make sure label is referenced
        }
    }
    else if (pAttrHttpEquiv && pAttrContent && !_tcsnicmp(pAttrHttpEquiv->_pchVal, pAttrHttpEquiv->_cchVal, _T("palette"), -1) && _pDwnDoc->WantAuthorPalette())
    {
        TraceTag((tagPalette, "Found a palette meta"));
        _pDwnDoc->SetAuthorColors(pAttrContent->_pchVal, pAttrContent->_cchVal);
    }
    else if (!_fMetaCharsetOverride)
    {
        // Per IE spec, only the first charset setting META tag counts.

        // BUGBUG (johnv) May need Big Endian Unicode support

        if (CP_UNDEFINED == _cpNew && _cp != CP_UCS_2)
        {
            // Per IE spec, handle the following cases:
            //
            // <META CHARSET=XXX>
            // <META HTTP-EQUIV CHARSET=XXX>
            // <META HTTP-EQUIV="Content-Type" CONTENT="text/html; charset=XXX">
            // <META HTTP-EQUIV="Charset" CONTENT="text/html; charset=XXX"> // IE5 bug 52716

            CHtmTag::CAttr *pAttrCharset = pht->AttrFromName(_T("CHARSET"));

            if (pAttrCharset ||
                (pAttrContent && pAttrHttpEquiv &&
                 (!_tcsnicmp(pAttrHttpEquiv->_pchVal, pAttrHttpEquiv->_cchVal, _T("Content-Type"), -1) ||
                  !_tcsnicmp(pAttrHttpEquiv->_pchVal, pAttrHttpEquiv->_cchVal, _T("Charset"), -1))))
            {
                CHtmTag::CAttr *pAttrArg = pAttrCharset ? pAttrCharset : pAttrContent;
                CODEPAGE cp;
                CStr cstrArg;

                hr = cstrArg.Set(pAttrArg->_pchVal, pAttrArg->_cchVal);
                if (hr)
                    goto Cleanup;

                cp = CodePageFromString( cstrArg, pAttrArg == pAttrContent );

                if (cp != CP_UNDEFINED && cp != _cp)
                {
                    // We shall reencode the buffer when we return to Tokenize().
                    _state = TS_NEWCODEPAGE;
                }

                #if DBG==1 || defined(PERFTAGS)
                if (    cp != CP_UNDEFINED
                    &&  IsPerfDbgEnabled(tagForceSwitchToRestart))
                {
                    _state = TS_NEWCODEPAGE;
                }
                #endif

                // If cp == CP_UNDEFINED, we want to process the next meta tag.
                // Otherwise, skip subsequent tags.
                _cpNew = cp;
            }
        }
    }

    // Add other HTTP-EQUIV value handling here (make sure iHttpEquiv >= 0)

Cleanup:
    PerfDbgLog(tagHtmPre, this, "-CHtmPre::HandleMETA");
    RRETURN(hr);
}

#ifdef VSTUDIO7
//+------------------------------------------------------------------------
//
//  Member:     CHtmPost::HandleTagsourcePI
//
//  Synopsis:   Process TAGSOURCE
//
//-------------------------------------------------------------------------
VOID
CHtmPre::HandleTagsourcePI(CHtmTag *pht)
{
    TCHAR *                  pchTag = NULL;
    TCHAR *                  pchBaseTag = NULL;
    TCHAR *                  pchCodebase = NULL;
    ELEMENT_TAG              etagBase = ETAG_NULL;
    INT                      cch = 0;

    if (!pht->ValFromName(_T("tagname"), &pchTag))
        return;

    pht->ValFromName(_T("basetag"), &pchBaseTag);
    if (pchBaseTag)
    {
        cch = lstrlen(pchBaseTag);
        etagBase = EtagFromName(pchBaseTag, cch);
        if (etagBase == ETAG_NULL && 0 == StrCmpNIC(pchBaseTag, _T("RAWDATA"), cch))
            etagBase = ETAG_GENERIC_LITERAL;
    }
    else
    {
        etagBase = ETAG_GENERIC;
    }

    Assert(etagBase != ETAG_NULL); 
    
    pht->ValFromName(_T("codebase"), &pchCodebase);

    _pHtmInfo->AddTagsource(pchTag, etagBase, pchCodebase);

}
#endif //VSTUDIO7

//+------------------------------------------------------------------------
//
//  Member:     SetContentTypeFromHeader
//
//  Synopsis:   Found "Content-type:" in the HTTP headers, set prelim
//              codepage for download (may be overridden by META tag in
//              doc)
//
//-------------------------------------------------------------------------

HRESULT
CHtmPre::SetContentTypeFromHeader(LPCTSTR pch)
{
    PerfDbgLog1(tagHtmPre, this, "+CHtmPre::SetContentTypeFromHeader %ls", pch);

    TCHAR       ach[128];
    CStr        cstr;
    TCHAR *     pchT;
    UINT        cch;
    CODEPAGE    cp;
    HRESULT     hr = S_OK;

    // Unfortunately, the charset code needs a writeable string.

    cch = _tcslen(pch);

    if (cch < ARRAY_SIZE(ach) - 1)
    {
        memcpy(ach, pch, (cch + 1) * sizeof(TCHAR));
        pchT = ach;
    }
    else
    {
        hr = THR(cstr.Set(pch));
        if (hr)
            goto Cleanup;

        pchT = cstr;
    }

    cp = CodePageFromString(pchT, TRUE);

    if (cp != CP_UNDEFINED && !_fMetaCharsetOverride)
    {
        // We should be able to just switch now, before the Pre
        //  gets to run.  Make sure to set _cpNew to _cp, not cp,
        //  in case we couldn't handle cp.  _cpNew needs to get set
        //  so that we ignore meta charset tags later on.

        // The FALSE argment means: don't restart, don't suspend
        DoSwitchCodePage(cp, NULL, FALSE);
        _cpNew = _cp;
    }

Cleanup:
    PerfDbgLog1(tagHtmPre, this, "-CHtmPre::SetContentTypeFromHeader (hr=%lX)", hr);
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::SwitchCodePage
//
//  Synopsis:   Switch the current codepage.  Stuffs a CMD_NEWCODEPAGE
//              token into the stream.
//
//-------------------------------------------------------------------------

BOOL
CHtmPre::DoSwitchCodePage(CODEPAGE cp, BOOL *pfNeedRestart, BOOL fRestart)
{
    PerfDbgLog1(tagHtmPre, this, "+CHtmPre::DoSwitchCodePage (cp=%d)", cp);

    // Note that this call is no longer used for really switching the code page
    // because if we switch, we will shut down and reparse. We just use the call
    // to determine if it is possible+necessary to switch (dbau)

    BOOL fSwitched;
    HRESULT hr;

    // cannot switch codepage in mid-fragment when pasting
    if (_fPasting && fRestart)
    {
        if (pfNeedRestart)
            *pfNeedRestart = FALSE;
            
        return FALSE;
    }

    fSwitched = CEncodeReader::ForceSwitchCodePage(cp, pfNeedRestart);

    #if DBG==1 || defined(PERFTAGS)
    if (fRestart && IsPerfDbgEnabled(tagForceSwitchToRestart))
        fSwitched = TRUE;
    else
    #endif
    if (pfNeedRestart)
        fRestart = fRestart && *pfNeedRestart;

    // bugfix: don't propagate switched codepage to document if pasting
    if (_fPasting)
        fSwitched = FALSE;

    // N.B. (johnv) Make sure not to throw a new codepage token into the stream if
    //  we are pasting.
    if (fSwitched)
    {
        // Convert our codepage into a token
        // and then hand it off to the post-processor.
        // N.B. (johnv) _cp can change to something other than cp if
        //  cp is not supported on the system, so be careful to use
        //  that instead.

        CHtmTag * pht;

        hr = THR(_pHtmTagStm->WriteTagBeg(ETAG_RAW_CODEPAGE, &pht));
        if (hr)
            goto Cleanup;

        if (fRestart)
            pht->SetRestart();

        pht->SetCodepage(cp);

        if (!fRestart)
        {
            // If we are not restarting, then this is informational only to the postparser.
            // Otherwise, we are going to be suspending and returning E_PENDING to the
            // toplevel function (either Exec() or TokenizeText()).  They will take responsibility
            // for calling WriteTagEnd() and signalling.

            _pHtmTagStm->WriteTagEnd();
        }

        // Codepage of CDwnDoc need to be updated if codepage of the document is being switched.
        if (_pDwnDoc)
            _pDwnDoc->SetDocCodePage(NavigatableCodePage(cp));
    }

Cleanup:

    // BUGBUG: no error handling!!

    PerfDbgLog1(tagHtmPre, this, "-CHtmPre::DoSwitchCodePage (fSwitched=%s)",
        fSwitched ? "TRUE" : "FALSE");

    return fSwitched;
}
//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::SwitchCodePage (CEncode override)
//
//  Synopsis:   This function overrides the default CEncode implementation.
//              Called from CEncode when it autodetects a codepage change
//              (e.g. when we find a unicode signature in the stream).
//              fDetected -
//              when mlang gets us detection result, unlike a unicode signature, 
//              it is not guaranteed that mlang can detect the codepage at the 
//              beginning of stream so we have to reload the document when the 
//              detected codepage is different from current.
//
//-------------------------------------------------------------------------

BOOL
CHtmPre::SwitchCodePage(CODEPAGE cp, BOOL *pfDifferentEncoding, BOOL fAutoDetected)
{
    BOOL fSwitched  = DoSwitchCodePage(cp, pfDifferentEncoding, fAutoDetected);
    if (fAutoDetected && fSwitched)
    {
       // if this switching is happening because of codepage
       // detection, we need to write out the end tag
       // so we can process codepage tag at the post parser
       _pHtmTagStm->WriteTagEnd();
    }
    return fSwitched;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::Read
//
//  Synopsis:   Read from _pDwnBindData
//
//-------------------------------------------------------------------------

HRESULT
CHtmPre::Read()
{
    PerfDbgLog(tagHtmPre, this, "+CHtmPre::Read");

    HRESULT hr;
    unsigned long cbRead;
    unsigned long cbRequest;
    BYTE *pbRead = _pbBufferPtr;

    Assert( _cbBuffer + HTMPRE_BLOCK_SIZE <= _cbBufferMax );
    Assert( _cbNextInsert < 0 || _cbNextInsert >= _cbReadTotal);

    if (_cbNextInsert >= 0)
        cbRequest = min(_cbNextInsert - _cbReadTotal, HTMPRE_BLOCK_SIZE);
    else
        cbRequest = HTMPRE_BLOCK_SIZE;

    hr = THR(_pDwnBindData->Read(pbRead, cbRequest, &cbRead));
    if (hr)
        goto Cleanup;

    hr = THR(_pHtmInfo->OnSource(pbRead, cbRead));
    if (hr)
        goto Cleanup;

    _cbBuffer += cbRead;
    _cbReadTotal += cbRead;
    _pbBufferPtr = _pbBuffer;

    Assert( _cbNextInsert < 0 || _cbNextInsert >= _cbReadTotal);

Cleanup:
    PerfDbgLog4(tagHtmPre, this, "-CHtmPre::Read (cbRead=%ld,cbTotal=%ld,cbBuffer=%ld,hr=%lX)",
        cbRead, _cbReadTotal, _cbBuffer, hr);
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::ReadStream
//
//  Synopsis:   Read from passed in stream
//
//-------------------------------------------------------------------------

HRESULT
CHtmPre::ReadStream(IStream *pstm, BOOL *pfEof)
{
    PerfDbgLog(tagHtmPre, this, "+CHtmPre::Read");

    HRESULT hr;
    unsigned long cbRead;
    unsigned long cbRequest;
    BYTE *pbRead = _pbBufferPtr;

    Assert( _cbBuffer + HTMPRE_BLOCK_SIZE <= _cbBufferMax );
    Assert( _cbNextInsert < 0 || _cbNextInsert >= _cbReadTotal);

    if (_cbNextInsert >= 0)
        cbRequest = min(_cbNextInsert - _cbReadTotal, HTMPRE_BLOCK_SIZE);
    else
        cbRequest = HTMPRE_BLOCK_SIZE;

    hr = THR(pstm->Read(pbRead, cbRequest, &cbRead));
    if (hr)
        goto Cleanup;

    *pfEof = (!cbRead);

    hr = THR(_pHtmInfo->OnSource(pbRead, cbRead));
    if (hr)
        goto Cleanup;

    _cbBuffer += cbRead;
    _cbReadTotal += cbRead;
    _pbBufferPtr = _pbBuffer;

    Assert( _cbNextInsert < 0 || _cbNextInsert >= _cbReadTotal);

Cleanup:
    PerfDbgLog4(tagHtmPre, this, "-CHtmPre::Read (cbRead=%ld,cbTotal=%ld,cbBuffer=%ld,hr=%lX)",
        cbRead, _cbReadTotal, _cbBuffer, hr);
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::AddInsert
//
//  Synopsis:   Adds a token which must be inserted into the token stream
//              before processing beyond the given cb of the input stream
//
//-------------------------------------------------------------------------
HRESULT
CHtmPre::AddInsert(int cb, TOKENINSERTCODE icode)
{
    PerfDbgLog1(tagHtmPre, this, "+CHtmPre::AddInsert %d", icode);

    HRESULT hr = S_OK;

    CInsertMap imap;
    int i;

    Assert(cb >= _cbReadTotal);

    for (i = 0; i < _aryInsert.Size(); i++)
    {
        if (_aryInsert[i]._cb > cb)
            break;
    }

    imap._cb = cb;
    imap._icode = icode;

    hr = THR(_aryInsert.InsertIndirect(i, &imap));
    if (hr)
        goto Cleanup;

    _cbNextInsert = _aryInsert[0]._cb;

Cleanup:
    PerfDbgLog1(tagHtmPre, this, "-CHtmPre::AddInsert (hr=%lX)", hr);
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::QueueInserts
//
//  Synopsis:   Notes and queues up insert codes to be inserted after
//              next token
//
//-------------------------------------------------------------------------
void
CHtmPre::QueueInserts()
{
    CInsertMap *pins;
    long c;
    long cbNext = -1;
    
    for (c = _aryInsert.Size(), pins = _aryInsert; c; c -= 1, pins += 1)
    {
        if (pins->_cb > _cbReadTotal)
        {
            cbNext = pins->_cb;
            break;
        }
    }
    
    _cOutputInsert = pins - _aryInsert;
    _cbNextInsert = cbNext;
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::OutputInserts
//
//  Synopsis:   Sends a special token into the token stream, based on
//              the insert code
//
//-------------------------------------------------------------------------
HRESULT
CHtmPre::OutputInserts()
{
    HRESULT hr = S_OK;
    ELEMENT_TAG etag;
    TOKENINSERTCODE icode;
    CHtmTag * pht;
    long i;

    Assert( _cOutputInsert >= 0 );

    if (!_cOutputInsert)
        goto Cleanup;
    
    for ( i = 0 ; i < _cOutputInsert ; i++ )
    {
        icode = _aryInsert[i]._icode;

        switch (icode)
        {
        case TIC_BEGINSEL  : etag = ETAG_RAW_BEGINSEL;  goto DoIt;
        case TIC_ENDSEL    : etag = ETAG_RAW_ENDSEL;

        DoIt:

            hr = THR( _pHtmTagStm->WriteTagBeg( etag, & pht ) );
            
            if (hr)
                goto Cleanup;

            pht->SetTiny();

            _pHtmTagStm->WriteTagEnd();

            break;

        default:
            Assert(0);
        }
    }

    _aryInsert.DeleteMultiple(0, _cOutputInsert - 1);

    _cOutputInsert = 0;

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::InsertImage, public
//
//  Synopsis:   Writes the tokens for a standalone <IMG> tag
//
//-------------------------------------------------------------------------

HRESULT
CHtmPre::InsertImage(LPCTSTR pchUrl, CDwnBindData * pDwnBindData)
{
    PerfDbgLog1(tagHtmPre, this, "+CHtmPre::InsertImage %ls", pchUrl);

    CHtmTag * pht;
    CHtmTag::CAttr * pattr;
    TCHAR * pchBuffer;
    UINT cchUrl = _tcslen(pchUrl);
    HRESULT hr;

    hr = THR(AddDwnCtx(DWNCTX_IMG, pchUrl, cchUrl, pDwnBindData));
    if (hr)
        goto Cleanup;

    hr = THR(_pHtmTagStm->AllocTextBuffer(cchUrl + 1, &pchBuffer));
    if (hr)
        goto Cleanup;

    memcpy(pchBuffer, pchUrl, (cchUrl + 1) * sizeof(TCHAR));

    hr = THR(_pHtmTagStm->WriteTagBeg(ETAG_IMG, &pht));
    if (hr)
        goto Cleanup;

    hr = THR(_pHtmTagStm->WriteTagGrow(&pht, &pattr));
    if (hr)
        goto Cleanup;

    pattr->_pchName = _T("SRC");
    pattr->_cchName = 3;
    pattr->_pchVal = pchBuffer;
    pattr->_cchVal = cchUrl;
    pattr->_ulOffset = 0;
    pattr->_ulLine = 0;

    _pHtmTagStm->WriteTagEnd();

    hr = THR(OutputDocSize());
    if (hr)
        goto Cleanup;

    OutputEof(S_OK);

Cleanup:
    PerfDbgLog1(tagHtmPre, this, "-CHtmPre::InsertImage (hr=%lX)", hr);
    RRETURN(hr);
}

#ifdef XMV_PARSE
//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::SetGenericParse, public
//
//  Synopsis:   Throws the parser into a mode where unqualified tags are considered
//              external
//
//-------------------------------------------------------------------------
void
CHtmPre::SetGenericParse(BOOL fDoGeneric)
{
    if (_pHtmTagStm->TagsWritten() == 0)
        _fXML = fDoGeneric;
}
#endif

//+------------------------------------------------------------------------
//
//  Member:
//  [in,out]    fEndCR  Caller state value to indicated if the last character
//                      of previous buffer was a carriage return. On return
//                      set to TRUE if current buffer ends in CR otherwise
//                      set to FALSE.
//  [in]        pch     Pointer to first character of buffer to normalize
//  [in,out]    pchEnd  Pointer to pointer to one past the last character in
//                      the buffer to be normalized. This MUST be a writeable
//                      memory location (for example a string's NULL terminator).
//                      *ppchEnd is assigned to the last character of the
//                      normalized buffer upon return.
//
//  Synopsis:   Advances _pchEnd by cch (assuming cch chars have been put
//              in place already), but first removes embedded \0 chars
//              and does NS-compatable CRLF handling.
//
//              CR   -> CR
//              LF   -> CR
//              CRLF -> CR
//              LFCR -> CRCR
//
//  Returns:    Number of chars by which the buffer was reduced.
//
//
//  NOTE:       **ppchEnd MUST be a writeable memory location (for example a
//              string's NULL terminator).
//
//-------------------------------------------------------------------------

#if DBG != 1
#pragma optimize(SPEED_OPTIMIZE_FLAGS, on)
#endif

int
NormalizerChar(LPTSTR pchStart, LPTSTR * ppchEnd, BOOL *pfAscii)
{
    if (pfAscii)
        *pfAscii = TRUE;

    if (!pchStart || !ppchEnd || !*ppchEnd )
        return 0;

    TCHAR *pch      = pchStart;
    TCHAR *pchLast  = *ppchEnd;
    TCHAR  ch;
    TCHAR *pchTo;

    // null terminate for speed when nothing needs to be done
    *pchLast = _T('\0');

fastscan:

    // Fast scan without copying to find first potentially invalid character.
    // Note that the NULL terminator will stop this loop.

    while (((WORD)(*pch++ - 1)) < 0x007F) ;

    if (pch > pchLast)
        return(0);

    // Nonascii character encountered.
    if (pfAscii)
        *pfAscii = FALSE;

    // Verify that the character really is invalid and not just out-of-range of
    // the fast scanner.

    ch = pch[-1];

    if (ch && IsValidWideChar(ch))
        goto fastscan;

    // Aw nuts, time to start squeezing out the invalid characters

    pchTo = --pch;

    while (pch < pchLast)
    {
        if (ch && IsValidWideChar(ch))
            *pchTo++ = ch;

        ch = *(++pch);
    }

    // null terminate for tokenizer
    *pchTo = _T('\0');

    // advance *ppchEnd
    *ppchEnd = pchTo;

    // return the number of chars by which we shrunk.
    return pchLast - pchTo;
}


//+------------------------------------------------------------------------
//
//  Member:
//  [in,out]    fEndCR  Caller state value to indicated if the last character
//                      of previous buffer was a carriage return. On return
//                      set to TRUE if current buffer ends in CR otherwise
//                      set to FALSE.
//  [in]        pch     Pointer to first character of buffer to normalize
//  [in,out]    pchEnd  Pointer to pointer to one past the last character in
//                      the buffer to be normalized. This MUST be a writeable
//                      memory location (for example a string's NULL terminator).
//                      *ppchEnd is assigned to the last character of the
//                      normalized buffer upon return.
//
//  Synopsis:   Advances _pchEnd by cch (assuming cch chars have been put
//              in place already), but first removes embedded \0 chars
//              and does NS-compatable CRLF handling.
//
//              CR   -> CR
//              LF   -> CR
//              CRLF -> CR
//              LFCR -> CRCR
//
//  Returns:    Number of chars by which the buffer was reduced.
//
//
//  NOTE:       **ppchEnd MUST be a writeable memory location (for example a
//              string's NULL terminator).
//
//-------------------------------------------------------------------------

int
NormalizerCR(BOOL * pfEndCR, LPTSTR pchStart, LPTSTR * ppchEnd)
{
    if (!pchStart || !ppchEnd || !*ppchEnd )
        return 0;

    TCHAR ch;
    TCHAR *pchTo;
    TCHAR *pch              = pchStart;
    TCHAR *const pchLast    = *ppchEnd;
    BOOL  fCR               = pfEndCR && *pfEndCR;

    // null terminate for speed
    **ppchEnd = _T('\0');

    // Assign after null termination in case pchStart == *ppchEnd
    ch = *pch;

    // Trim LF if preceded by CR from previous run
    if (fCR && ch == _T('\n'))
    {
        pchTo = pch;
        ch = *(++pch);
    }
    else
    {
        while (ch == _T('\n'))
        {
            *pch = _T('\r');
            ch = *(++pch);
        }

        while (ch && ch != _T('\n') && IsValidWideChar(ch))
            ch = *(++pch);

        pchTo = pch;
    }

    // Assert(pch > pchStart || !ch);

    // mini CRLF and internal \0 handler
    while (pch < *ppchEnd)
    {
        if (!ch || !IsValidWideChar(ch))
            ch = *(++pch);
        else
        {
            do
            {
                // Assert(ch != _T('\0'));
                if (ch == _T('\n'))
                {
                    if (*(pch-1) == _T('\r'))
                        ch = *(++pch);

                    while (ch == _T('\n'))
                    {
                        *(pchTo++) = _T('\r');
                        ch = *(++pch);
                    }

                    if (!ch)
                        break;
                }

                // Assert(ch != _T('\0') && ch !=_T('\n'))
                if (IsValidWideChar(ch))
                    *(pchTo++) = ch;
                ch = *(++pch);

            } while (ch);
        }
    }

    // null terminate for tokenizer
    *pchTo = _T('\0');

    // advance *ppchEnd
    *ppchEnd = pchTo;

    // note trailing CR
    if (pfEndCR && (pch > pchStart))
        *pfEndCR = (*(pch-1) == _T('\r'));

    // return the number of chars by which we shrunk.
    return pchLast - pchTo;
}

#if DBG != 1
#pragma optimize("", on)
#endif


//+------------------------------------------------------------------------
//
//  Section:    Entities
//
//  Synopsis:   Translates entity names to/from unicode
//
//-------------------------------------------------------------------------

CPtrBag<ELEMENT_TAG> g_bEntities(&g_entasc);

//+------------------------------------------------------------------------
//
//  Function:   LookUpErTable
//
//  Synopsis:   Unicode->Entity name string, if any.
//
//              If no match, NULL is returned.
//
//  BSearch is too lame to handle arrays of pointers to objects, hence the
//  inline routine.
//
//-------------------------------------------------------------------------
const TCHAR*
LookUpErTable(TCHAR ch, BOOL fIsCp1252)
{
    CAssoc * pEntry;
    int      iLow, iMiddle, iHigh;

    iHigh = sizeof(g_entasc_RevSearch)
           / sizeof(g_entasc_RevSearch[0]) - 1;
    iLow = 0;

#ifndef UNICODE
    // (cthrash) This is necessary for the multibyte build because entities
    // not in Windows-1252 map to '?', but we don't want to persist '?' as
    // named entities.  Yes, this means we won't roundtrip.  The Win16 folks
    // know this, and are happy to live with this limitation.
    if (ch == L'?')
    {
        return NULL;
    }
#endif

    // binary search
    while (iHigh != iLow)
    {
        Assert(iHigh > iLow);

        iMiddle = (iHigh + iLow) / 2;

        if (ch > g_entasc_RevSearch[iMiddle]->Number())
        {
            iLow = iMiddle + 1;
            if (ch < g_entasc_RevSearch[iLow]->Number())
            {
                // ch isn't in the array:
                // ch > val(iMiddle) && ch < val(iMiddle + 1)
                return NULL;
            }
        }
        else
        {
            iHigh = iMiddle;
        }
    }

    pEntry = g_entasc_RevSearch[iLow];

    if (ch == pEntry->Number() &&
        (!fIsCp1252 ||
         pEntry != &g_entasctrade &&
#ifdef UNICODE
         pEntry != &g_entascTRADE8482
#else
         pEntry != &g_entascTRADE153
#endif
        ))
    {
        return pEntry->String();
    }

    return NULL;
}

//+------------------------------------------------------------------------
//
//  Function:   EntityChFromName
//
//  Synopsis:   Entity name string->Unicode
//
//              If no match, \00 is returned.
//
//-------------------------------------------------------------------------
XCHAR
EntityChFromName(TCHAR *pchText, ULONG cchText, DWORD hash)
{
    return (XCHAR)g_bEntities.Get(pchText, cchText, hash);
}



#if DBG != 1
#pragma optimize("", on)
#endif

