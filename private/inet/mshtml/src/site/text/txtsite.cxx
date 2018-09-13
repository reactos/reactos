//+---------------------------------------------------------------------
//
//   File:      txtsite.cxx
//
//  Contents:   Text site class
//
//  Classes:    CTxtSite
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_CGUID_H_
#define X_CGUID_H_
#include <cguid.h>
#endif

#ifndef X_TXTSITE_HXX_
#define X_TXTSITE_HXX_
#include "txtsite.hxx"
#endif

#ifndef X__DISP_H_
#define X__DISP_H_
#include "_disp.h"
#endif

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

#ifndef X__DXFROBJ_H_
#define X__DXFROBJ_H_
#include "_dxfrobj.h"
#endif

#ifndef X__RUNPTR_H_
#define X__RUNPTR_H_
#include "_runptr.h"
#endif

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X__IME_H_
#define X__IME_H_
#include "_ime.h"
#endif

#ifndef X_MISCPROT_H_
#define X_MISCPROT_H_
#include "miscprot.h"
#endif

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#ifndef X__TXTSAVE_H_
#define X__TXTSAVE_H_
#include "_txtsave.h"
#endif

#ifndef X_IRANGE_HXX_
#define X_IRANGE_HXX_
#include "irange.hxx"
#endif

#ifndef X_HYPLNK_HXX_
#define X_HYPLNK_HXX_
#include "hyplnk.hxx"
#endif

#ifndef X_EANCHOR_HXX_
#define X_EANCHOR_HXX_
#include "eanchor.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_CUTIL_HXX_
#define X_CUTIL_HXX_
#include "cutil.hxx"
#endif

#ifndef X_FPRINT_HXX_
#define X_FPRINT_HXX_
#include "fprint.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_DOCPRINT_HXX_
#define X_DOCPRINT_HXX_
#include "docprint.hxx"
#endif

#ifndef X_COLLECT_HXX_
#define X_COLLECT_HXX_
#include "collect.hxx"
#endif

#ifndef X_EAREA_HXX_
#define X_EAREA_HXX_
#include "earea.hxx"
#endif

#ifndef X_SBBASE_HXX_
#define X_SBBASE_HXX_
#include "sbbase.hxx"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_LTCELL_HXX_
#define X_LTCELL_HXX_
#include "ltcell.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#define _cxx_
#include "txtedit.hdl"

DeclareTag(tagPaginate, "Print", "Pagination output");

// force functions to load through dynamic wrappers
#ifndef WIN16
#ifndef X_COMCTRLP_H_
#define X_COMCTRLP_H_
#ifdef WINCOMMCTRLAPI
#undef WINCOMMCTRLAPI
#define WINCOMMCTRLAPI
#endif
#include "comctrlp.h"
#endif
#endif // ndef WIN16

ExternTag(tagMsoCommandTarget);
ExternTag(tagFormatCaches);
ExternTag(tagLayout);

MtDefine(CTxtSite, Elements, "CTxtSite")
MtDefine(CTxtSite_pDropTargetSelInfo, CTxtSite, "CTxtSite::_pDropTargetSelInfo")
MtDefine(CTxtSiteScrollRangeIntoView_aryRects_pv, Locals, "CTxtSite::ScrollRangeIntoView aryRects::_pv")
MtDefine(CTxtSiteBranchFromPointEx_aryRects_pv, Locals, "CTxtSite::SiteBranchFromPointEx aryRects::_pv")
MtDefine(CTxtSiteDrop_arySites_pv, Locals, "CTxtSite::Drop arySites::_pv")
MtDefine(CTxtSiteGetChildElementTopLeft_aryRects_pv, Locals, "CTxtSite::GetChildElementTopLeft aryRects::_pv")
MtDefine(CTxtSitePaginate_aryValues_pv, Locals, "CTxtSite::Paginate aryValues::_pv")


#if DBG==1
extern void TestMarkupServices(CElement *);
#endif

// Commented out the following 4 lines because unused due to #ifdef NEVER in code:
// #pragma BEGIN_CODESPACE_DATA
// TCHAR szCRLF[]  = TEXT("\r\n");
// TCHAR szCR[]    = TEXT("\r");
// #pragma END_CODESPACE_DATA

WORD  ConvVKey (WORD vKey);
WORD        wConvScroll(WORD wparam);


#ifdef NEVER
// not used
#ifndef NO_PROPERTY_PAGE
const CLSID * CTxtSite::s_apclsidPages[] =
{
    // Browse-time pages
    NULL,
    // Edit-time pages
#if DBG==1    
    &CLSID_CCDGenericPropertyPage,
#endif // DBG==1    
    NULL
};
#endif // NO_PROPERTY_PAGE
#endif // NEVER

CElement::ACCELS CTxtSite::s_AccelsTxtSiteDesign = CElement::ACCELS (&CElement::s_AccelsElementDesign, IDR_ACCELS_TXTSITE_DESIGN);
CElement::ACCELS CTxtSite::s_AccelsTxtSiteRun    = CElement::ACCELS (&CElement::s_AccelsElementRun,    IDR_ACCELS_TXTSITE_RUN);

//+------------------------------------------------------------------------
//
//  Member:     CTxtSite, ~CTxtSite
//
//  Synopsis:   Constructor/Destructor
//
//-------------------------------------------------------------------------

CTxtSite::CTxtSite (ELEMENT_TAG etag, CDoc *pDoc)
  : super(etag, pDoc)
{
#ifdef WIN16
    m_baseOffset = ((BYTE *) (void *) (CBase *)this) - ((BYTE *) this);
    m_ElementOffset = ((BYTE *) (void *) (CElement *)this) - ((BYTE *) this);
#endif
    _fOwnsRuns = TRUE;

}

//+------------------------------------------------------------------------
//
//  Member:     CTxtSite::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-------------------------------------------------------------------------

HRESULT
CTxtSite::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    HRESULT     hr;

    *ppv = NULL;

    if (iid == CLSID_CTextSite)
    {
        *ppv = this;        // weak ref.
        return S_OK;
    }
    else if IID_TEAROFF(this, IHTMLTextContainer, NULL)
    else
    {
        RRETURN(THR_NOTRACE(super::PrivateQueryInterface(iid, ppv)));
    }

    (*(IUnknown **)ppv)->AddRef();
    return hr;
}


/*
 *  CTxtSite::Init ()
 *
 *  @mfunc
 *      Initializes this CTxtSite. Called by CreateTextServices()
 */

#ifndef X_TXTELEMS_HXX_
#define X_TXTELEMS_HXX_
#include "txtelems.hxx"
#endif

//+-------------------------------------------------------------------------
//
//  Method:     CTxtSite::Detach
//
//--------------------------------------------------------------------------


BOOL PointInRectAry(POINT pt, CStackDataAry<RECT, 1> &aryRects)
{
    for (int i = 0; i < aryRects.Size(); i++)
    {
        RECT rc = aryRects[i];
        if (PtInRect(&rc, pt))
            return TRUE;
    }
    return FALSE;
}

//+------------------------------------------------------------------------
//
//  Member:     wConvScroll
//
//  Synopsis:   Interchange horizontal and vertical commands for vertical
//              text site.
//
//-------------------------------------------------------------------------

//
WORD wConvScroll(WORD wparam)
{
    switch(wparam)
    {
        case SB_BOTTOM:
            return SB_TOP;

        case SB_LINEDOWN:
            return SB_LINEUP;

        case SB_LINEUP:
            return SB_LINEDOWN;

        case SB_PAGEDOWN:
            return SB_PAGEUP;

        case SB_PAGEUP:
            return SB_PAGEDOWN;

        case SB_TOP:
            return SB_BOTTOM;

        default:
            return wparam;
    }
}


WORD ConvVKey (WORD vKey)
{
    switch(vKey)
    {
        case VK_LEFT:
            return VK_DOWN;

        case VK_RIGHT:
            return VK_UP;

        case VK_UP:
            return VK_LEFT;

        case VK_DOWN:
            return VK_RIGHT;

        default:
            return vKey;
    }
}

//+------------------------------------------------------------------
//
//  Members: [get/put]_scroll[top/left] and get_scroll[height/width]
//
//  Synopsis : IHTMLTextContainer members. _dp is in pixels.
//
//------------------------------------------------------------------

HRESULT
CTxtSite::get_scrollHeight(long *plValue)
{
    RRETURN(super::get_scrollHeight(plValue));
}

HRESULT
CTxtSite::get_scrollWidth(long *plValue)
{
    RRETURN(super::get_scrollWidth(plValue));
}

HRESULT
CTxtSite::get_scrollTop(long *plValue)
{
    RRETURN(super::get_scrollTop(plValue));
}

HRESULT
CTxtSite::get_scrollLeft(long *plValue)
{
    RRETURN(super::get_scrollLeft(plValue));
}

HRESULT
CTxtSite::put_scrollTop(long lPixels)
{
    RRETURN(super::put_scrollTop(lPixels));
}


HRESULT
CTxtSite::put_scrollLeft(long lPixels)
{
    RRETURN(super::put_scrollLeft(lPixels));
}


//+-----------------------------------------------------------------
//
//  member [put_|get_]onscroll
//
//  synopsis - just delegate to CElement. these are here because this
//      property migrated from here to elemetn.
//+-----------------------------------------------------------------
STDMETHODIMP
CTxtSite::put_onscroll(VARIANT v)
{
    RRETURN(super::put_onscroll(v));
}
STDMETHODIMP
CTxtSite::get_onscroll(VARIANT * p)
{
    RRETURN(super::get_onscroll(p));
}

//+-------------------------------------------------------------------------------
//
//  Member:     createControlRange
//
//  Synopsis:   Implementation of the automation interface method.
//              This creates a default structure range (CAutoTxtSiteRange) and
//              passes it back.
//
//+-------------------------------------------------------------------------------
HRESULT
CTxtSite::createControlRange(IDispatch ** ppDisp)
{
    RRETURN(SetErrorInfo(THR(CElement::createControlRange(ppDisp))));
}

//////////////////////////////  Line Dump  ///////////////////////////////////////

#if DBG == 1 || defined(DUMPTREE)


// Use this inside asserts checking to see that the document is inplace.
// This code is DBG==1 only, so if you obviously can't use it anywhere
// in the ship build.  The key here is the CPrintDoc's lie.


// Made this static so I would not have to declare FILE in _edit.h
HANDLE g_f;

void WriteFileAnsi (
    HANDLE hFile, LPCVOID lpBuffer,
    DWORD nNumberOfBytesToWrite )
{
    char buffer [ 1024 ];
    long _cchLen;
    DWORD nbw;

    _cchLen = WideCharToMultiByte(
        CP_ACP, 0, LPWSTR( lpBuffer ), nNumberOfBytesToWrite,
        buffer, ARRAY_SIZE(buffer), NULL, NULL );

    WriteFile( hFile, buffer, _cchLen, & nbw, NULL );
}

void WriteString ( HANDLE hFile, TCHAR * pszStr )
{
    WriteFileAnsi( hFile, pszStr, _tcslen(pszStr) );
}

void WriteChar ( HANDLE hFile, TCHAR ch, int nRepeat )
{
    Assert( nRepeat >= 0 );

    while (nRepeat--)
        WriteFileAnsi( hFile, & ch, 1 );
}

void WriteHelp ( HANDLE hFile, TCHAR * format, ... )
{
    va_list arg;

    va_start( arg, format );

    WriteHelpV( hFile, format, &arg );
}

void WriteHelpV( HANDLE hFile, TCHAR * format, va_list * parg )
{
    TCHAR achBuf [ 1024 ];

    VFormat( 0, achBuf, ARRAY_SIZE(achBuf), format, parg );

    WriteFileAnsi( hFile, achBuf, _tcslen(achBuf) );
}

void WriteFormattedString( HANDLE hFile, TCHAR * pch, long cch )
{
    if (!pch)
        return;

    for ( int i = 0 ; i < cch ; i++ )
    {
        TCHAR ch = pch[i];

        if (ch >= 1 && ch <= 26)
        {
            if (ch == _T('\r'))
                WriteString( hFile,  _T("\\r"));
            else if (ch == _T('\n'))
                WriteString( hFile, _T("\\n"));
            else
            {
                WriteHelp( hFile, _T("[<0d>]"), (long)int(ch) );
            }
        }
        else
        {
            switch ( ch )
            {
            case 0 :
                WriteString( hFile, _T("[NULL]"));
                break;

            case WCH_NODE:
                WriteString( hFile, _T("[Node]"));
                break;

            case WCH_NBSP :
                WriteString( hFile, _T("[NBSP]"));
                break;

            default :
                if (ch < 256 && _istprint(ch))
                {
                    WriteChar(hFile, ch);
                }
                else
                {
                    TCHAR achHex[9];

                    Format( 0, achHex, ARRAY_SIZE(achHex), _T("<0x>"), ch);

                    StrCpy( achHex, TEXT("[U+") );
                    StrCpy( achHex + 3, achHex + 4 );
                    StrCpy( achHex + 7, TEXT("]") );

                    WriteString( hFile, achHex );
                }

                break;
            }
        }
    }
}

BOOL InitDumpFile ( BOOL fOverwrite /* = FALSE */ )
{
    Assert( g_f == 0 );

    g_f = CreateFile(
#ifdef UNIX
            _T("tree.dump"),
#else
            fOverwrite ? _T("c:\\ff.txt") : _T("c:\\ee."),
#endif
            GENERIC_WRITE | GENERIC_READ,
            FILE_SHARE_WRITE | FILE_SHARE_READ,
            NULL,
            fOverwrite ? CREATE_ALWAYS : OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

    if (g_f == INVALID_HANDLE_VALUE)
    {
        g_f = 0;
        return FALSE;
    }

    SetFilePointer( g_f, GetFileSize( g_f, 0 ), 0, 0 );

    static int cDumps = 0;

    WriteHelp( g_f,
        _T("------------- Dump <0d> ------------------------------- \r\n\r\n" ),
        (long)cDumps++ );

    return TRUE;
}

void CloseDumpFile ( )
{
    CloseHandle( g_f );

    g_f = 0;
}

#endif

