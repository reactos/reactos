/*  TXTSITE.HXX
 *
 *  Purpose:
 *      Text site class
 *
 *  Copyright (c) 1995-1996, Microsoft Corporation. All rights reserved.
 */

#ifndef I_TXTSITE_HXX_
#define I_TXTSITE_HXX_
#pragma INCMSG("--- Beg 'txtsite.hxx'")

#ifndef X_CSITE_HXX_
#define X_CSITE_HXX_
#include "csite.hxx"
#endif

#ifndef X_DRAGDROP_HXX_
#define X_DRAGDROP_HXX_
#include "dragdrop.hxx"
#endif

class CBaseBag;

#define _hxx_
#include "txtedit.hdl"

#define FONTMIN          1
#define FONTMAX          7

// used by CTxtSite::HandleMouseWheel. The "magic" number is similar to
// the one used in CDisplay::VScroll(SB_LINEDOWN, fFixed) and
// CDisplay::VScroll(SB_PAGEDOWN, fFixed).
//
#define PERCENT_PER_LINE 50
#define PERCENT_PER_PAGE 875

// Forward declarations
class CMeasurer;
class CRenderer;
class CDisplay;
class CDrawInfoRE;
class CChan;
class CFormDrawInfo;

#ifdef _MAC // we need the full enum definition
#include "flowlyt.hxx"
#else
enum TXTBACKSTYLE;
#endif



// ==================================  CTxtSite  ============================================

MtExtern(CTxtSite)

class CParser;
class CImgCtx;

#if defined( DYNAMICARRAY_NOTSUPPORTED)
#define CTXTSITE_ACCELLIST_SIZE         35
#endif

class CTxtSite : public CSite
{
    DECLARE_CLASS_TYPES(CTxtSite, CSite)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTxtSite))

    CTxtSite (ELEMENT_TAG etag, CDoc *pDoc);

    // Automation interfaces
    #define _CTxtSite_
    #include "txtedit.hdl"

    NV_DECLARE_TEAROFF_METHOD(get_onscroll, GET_onscroll, (VARIANT*));
    NV_DECLARE_TEAROFF_METHOD(put_onscroll, PUT_onscroll, (VARIANT));

    // createControlRange helper
    HRESULT     CreateControlRange(int cnt, CLayout ** ppLayout, IDispatch ** ppDisp);

    // Misc helpers

    //--------------------------------------------------------------
    // IPrivateUnknown members
    //--------------------------------------------------------------

    DECLARE_PLAIN_IUNKNOWN(CTxtSite);

    DECLARE_PRIVATE_QI_FUNCS(CBase)

    // static const CLSID *                s_apclsidPages[];  // not used
    static CElement::ACCELS                s_AccelsTxtSiteDesign;
    static CElement::ACCELS                s_AccelsTxtSiteRun;
};

// Debug services for dumping tree and line array

#if DBG == 1 || defined(DUMPTREE)

extern HANDLE g_f;

void WriteFileAnsi( HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite );
void WriteString( HANDLE hFile, TCHAR * pszStr );
void WriteChar( HANDLE hFile, TCHAR ch, int nRepeat = 1 );
void WriteHelp( HANDLE hFile, TCHAR * format, ... );
void WriteHelpV( HANDLE hFile, TCHAR * format, va_list * parg );
void WriteFormattedString( HANDLE hFile, TCHAR * pch, long cch );

BOOL InitDumpFile( BOOL fOverwrite = FALSE );
void CloseDumpFile( );

#endif

#pragma INCMSG("--- End 'txtsite.hxx'")
#else
#pragma INCMSG("*** Dup 'txtsite.hxx'")
#endif
