///////////////////////////////////////////////////////////////////////////////
//
// fontview.cpp
//      Explorer Font Folder extension routines.
//      Implementation for the CFontView class.
//
//
// History:
//      31 May 95 SteveCat
//          Ported to Windows NT and Unicode, cleaned up
//
//
// NOTE/BUGS
//
//  Copyright (C) 1992-1995 Microsoft Corporation
//
///////////////////////////////////////////////////////////////////////////////

//==========================================================================
//                              Include files
//==========================================================================


#include "priv.h"
#include "globals.h"
#include "fonthelp.h"
#include "fontview.h"
#include "fontman.h"
#include "fontcl.h"
#include "fontlist.h"
#include "cpanel.h"
#include "resource.h"
#include "ui.h"
#include "dbutl.h"
#include "extricon.h"


#include <sys\stat.h>
#include <time.h>
#include <htmlhelp.h>

#ifdef __cplusplus
extern "C" {
#endif

//
// BUGBUG:  Hack to avoid duplicate-macro error.  platform.h (included by shlobjp.h)
//          also defines a PATH_SEPARATOR macro (as does wingdip.h).  None of the
//          code in fontview.cpp uses this macro.  Once the name conflict is
//          removed from the headers, we can remove this undefinition hack.
//          [brianau - 3/6/98]
//
#ifdef PATH_SEPARATOR
#   undef PATH_SEPARATOR
#endif
#include <wingdip.h>

#ifdef __cplusplus
}
#endif

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))
#define BOOLIFY(expr)   (!!(expr))

#define FFCOL_DEFAULT -1
#define FFCOL_NAME 0
#define FFCOL_PANOSE 1
#define FFCOL_FILENAME 1
#define FFCOL_SIZE 2
#define FFCOL_MODIFIED 3
#define FFCOL_ATTRIBUTES 4

#define WID_LISTVIEW 2

#ifdef TOOLTIP_FONTSAMPLE
const INT FONT_SAMPLE_PT_SIZE = 16;
#endif

//
// Message for shell change notifications.
// Same value as WM_DSV_FSNOTIFY.
//
#define WM_SHELL_CHANGE_NOTIFY (WM_USER + 0xA0)

UINT const cDeadMenu = MF_BYCOMMAND | MF_DISABLED | MF_GRAYED;
UINT const cLiveMenu = MF_BYCOMMAND | MF_ENABLED;

UINT const kMinPointSize = 8;

#ifdef USE_OWNERDRAW

const int kBaseViewStyle = WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_AUTOARRANGE | LVS_OWNERDRAWFIXED;

#else

const int kBaseViewStyle = WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_AUTOARRANGE;

#endif


TCHAR* g_szViewClass = TEXT( "FONTEXT_View" );

//
// List of file attribute bit values.  The order (with respect
// to meaning) must match that of the characters in g_szAttributeChars[].
//
const DWORD g_adwAttributeBits[] = {
                                    FILE_ATTRIBUTE_READONLY,
                                    FILE_ATTRIBUTE_HIDDEN,
                                    FILE_ATTRIBUTE_SYSTEM,
                                    FILE_ATTRIBUTE_ARCHIVE,
                                    FILE_ATTRIBUTE_COMPRESSED
                                   };

#define NUM_ATTRIB_CHARS  ARRAYSIZE(g_adwAttributeBits)

//
// Buffer for characters that represent attributes in Details View attributes
// column.  Must provide room for 1 character for each bit and a NUL.  The current 5
// represent Read-only, Hidden, System, Archive, and Compressed in that order.
// This can't be const because we overwrite it using LoadString.
//
TCHAR g_szAttributeChars[NUM_ATTRIB_CHARS + 1] = { 0 } ;


#ifdef WINNT
#   define ALT_TEXT_COLOR   // NT uses alternate color for compressed files.
#endif

#ifdef ALT_TEXT_COLOR
//
// Things associated with "alternate" color for compressed files in folder.
//
#include <regstr.h>

COLORREF g_crAltColor      = RGB(0,0,255);     // Color defaults to blue.
HKEY g_hkcuExplorer        = NULL;
TCHAR const c_szAltColor[] = TEXT("AltColor"); // Reg loc for setting.

#endif // ALT_TEXT_COLOR



#pragma data_seg(".text", "CODE")
const static DWORD rgOptionPropPageHelpIDs[] =
{
    IDC_TTONLY, IDH_FONTS_TRUETYPE_ON_COMPUTER,
    0,0
};

#pragma data_seg()


//***   IsBackSpace -- is key a Backspace
BOOL IsBackSpace(LPMSG pMsg)
{
    return pMsg && (pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_BACK);
}

//***   IsVK_TABCycler -- is key a TAB-equivalent
// ENTRY/EXIT
//  dir     0 if not a TAB, non-0 if a TAB
// NOTES
//  NYI: -1 for shift+tab, 1 for tab
//
int IsVK_TABCycler(MSG *pMsg)
{
    if (!pMsg)
        return 0;

    if (pMsg->message != WM_KEYDOWN)
        return 0;
    if (! (pMsg->wParam == VK_TAB || pMsg->wParam == VK_F6))
        return 0;

    return (GetKeyState(VK_SHIFT) < 0) ? -1 : 1;
}


// ***********************************************************************
// ***********************************************************************
// ***********************************************************************

extern HRESULT MyReleaseStgMedium( LPSTGMEDIUM pmedium );


//
// Call CFontClass::Release() for each font object contained in the ListView.
//
void CFontView::ReleaseFontObjects(void)
{
    int iCount = ListView_GetItemCount(m_hwndList);
    LV_ITEM lvi;
    CFontClass *pFont = NULL;

    lvi.mask       = LVIF_PARAM;
    lvi.iSubItem   = 0;
    lvi.state      = 0;
    lvi.stateMask  = 0;
    lvi.pszText    = NULL;
    lvi.cchTextMax = 0;
    lvi.iImage     = 0;

    for (int i = 0; i < iCount; i++)
    {
        lvi.iItem = i;

        if (ListView_GetItem(m_hwndList, &lvi))
        {
            pFont = (CFontClass *)lvi.lParam;
            if (NULL != pFont)
                pFont->Release();
        }
    }
}


//
//  This function emulates OleSetClipboard().
//

STDAPI FFSetClipboard( LPDATAOBJECT pdtobj )
{
    HRESULT hres = NOERROR;

    if( OpenClipboard( NULL ) )    // associate it with current task.
    {
        EmptyClipboard( );

        if( pdtobj )
        {
            //
            //  Support WIN3.1 style clipboard : Just put the file name of
            //  the first item as "FileName".
            //

            FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
            STGMEDIUM medium;

            hres = pdtobj->GetData( &fmte, &medium );

            if( SUCCEEDED( hres ) )
            {
                if( !SetClipboardData( CF_HDROP, medium.hGlobal ) )
                    MyReleaseStgMedium( &medium );

                fmte.cfFormat = CFontData::s_CFPreferredDropEffect;
                hres = pdtobj->GetData(&fmte, &medium);
                if (SUCCEEDED(hres))
                {
                    if (!SetClipboardData(CFontData::s_CFPreferredDropEffect, medium.hGlobal))
                        MyReleaseStgMedium(&medium);
                }
            }
        }
        CloseClipboard( );
    }
    else
    {
        hres = ResultFromScode( CLIPBRD_E_CANT_OPEN );
    }

    return hres;
}


// ***********************************************************************
//  Structures used to manipulate the view of the list.
//

typedef struct _COLUMN_ENTRY {
                UINT     m_iID;         //  String
                UINT     m_iWidth;      //  width of column
                UINT     m_iFormat;
} COLUMN_ENTRY;


COLUMN_ENTRY PanoseColumns[] = { { IDS_PAN_COL1, 20, LVCFMT_LEFT},
                                 { IDS_PAN_COL2, 30, LVCFMT_LEFT} };

#define PAN_COL_COUNT  (sizeof( PanoseColumns ) / sizeof( COLUMN_ENTRY ) )

COLUMN_ENTRY FileColumns[] = { { IDS_FILE_COL1, 20, LVCFMT_LEFT},
                               { IDS_FILE_COL2, 14, LVCFMT_LEFT},
                               { IDS_FILE_COL3,  6, LVCFMT_RIGHT},
                               { IDS_FILE_COL4, 15, LVCFMT_LEFT},
                               { IDS_FILE_COL5, 10, LVCFMT_RIGHT}};


const TCHAR c_szM[] = TEXT( "M" );

//
//  the width of an M
//

int g_cxM = 0;


#define FILE_COL_COUNT  (sizeof( FileColumns ) / sizeof( COLUMN_ENTRY ) )


typedef struct {
            union {  DWORD        ItemData;
                     CFontClass * poFont;
                  }; // end union
} ComboITEMDATA;


// ***********************************************************************
//  Forward Declarations.
//

UINT WINAPI MergeMenus( HMENU hmDst, HMENU hmSrc, UINT uInsert,

UINT  uIDAdjust, UINT uIDAdjustMax, ULONG uFlags );
void  MergeFileMenu( HMENU hmenu, HMENU hmenuMerge );
void  MergeEditMenu( HMENU hmenu, HMENU hmenuMerge );
void  MergeViewMenu( HMENU hmenu, HMENU hmenuMerge );
void  MergeHelpMenu( HMENU hmenu, HMENU hmenuMerge );
HMENU GetMenuFromID( HMENU hmMain, UINT uID );
void  SetListColumns( HWND hWnd, UINT iCount, COLUMN_ENTRY * lpCol );


// ***********************************************************************
//  Local Functions
//

static ::HFONT hCreateFont( HDC hDC, int iPoints, const TCHAR FAR* lpFace )
{
    int FPoints = -MulDiv( iPoints, GetDeviceCaps( hDC, LOGPIXELSY ), 72 );

    return ::CreateFont( FPoints, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, lpFace );
}


UINT WSFromViewMode( UINT uMode, HWND hWnd = 0 );

UINT WSFromViewMode( UINT uMode, HWND hWnd )
{
    UINT  ws;

    if( hWnd )
        ws = GetWindowLong( hWnd, GWL_STYLE ) & ~LVS_TYPEMASK;
    else
        ws = kBaseViewStyle;

    switch( uMode )
    {
        default: // case IDM_VIEW_ICON:
            ws |= LVS_ICON;
            break;

        case IDM_VIEW_LIST:
            ws |= LVS_LIST;
            break;

        case IDM_VIEW_PANOSE:
            ws |= LVS_REPORT;
            break;
        case IDM_VIEW_DETAILS:
            ws |= LVS_REPORT;
            break;
    }

    return ws;
}


// ***********************************************************************
// ***********************************************************************
// ***********************************************************************


class CEnumFormatEtc : public IEnumFORMATETC
    {
    private:
        ULONG           m_cRef;         //Object reference count
        LPUNKNOWN       m_pUnkRef;      //For reference counting
        ULONG           m_iCur;         //Current element.
        ULONG           m_cfe;          //Number of FORMATETCs in us
        LPFORMATETC     m_prgfe;        //Source of FORMATETCs

    public:
        CEnumFormatEtc( LPUNKNOWN, ULONG, LPFORMATETC );
        ~CEnumFormatEtc( void );

        //IUnknown members that delegate to m_pUnkRef.
        STDMETHODIMP         QueryInterface( REFIID, VOID ** );
        STDMETHODIMP_(ULONG) AddRef( void );
        STDMETHODIMP_(ULONG) Release( void );

        //IEnumFORMATETC members
        STDMETHODIMP Next( ULONG, LPFORMATETC, ULONG FAR * );
        STDMETHODIMP Skip( ULONG );
        STDMETHODIMP Reset( void );
        STDMETHODIMP Clone( IEnumFORMATETC FAR * FAR * );
    };



typedef CEnumFormatEtc FAR *LPCEnumFormatEtc;


/*
 * CEnumFormatEtc::CEnumFormatEtc
 * CEnumFormatEtc::~CEnumFormatEtc
 *
 * Parameters (Constructor):
 *  pUnkRef         LPUNKNOWN to use for reference counting.
 *  cFE             ULONG number of FORMATETCs in pFE
 *  prgFE           LPFORMATETC to the array to enumerate.
 */

CEnumFormatEtc::CEnumFormatEtc( LPUNKNOWN pUnkRef, ULONG cFE, LPFORMATETC prgFE )
{
    UINT        i;

    m_cRef    = 0;
    m_pUnkRef = pUnkRef;

    m_iCur  = 0;
    m_cfe   = cFE;
    m_prgfe = new FORMATETC[ (UINT)cFE ];

    if( NULL != m_prgfe )
    {
        for( i = 0; i < cFE; i++ )
            m_prgfe[ i ] = prgFE[ i ];
    }

    return;
}


CEnumFormatEtc::~CEnumFormatEtc( void )
{
    if( NULL != m_prgfe )
        delete [] m_prgfe;

    return;
}


/*
 * CEnumFormatEtc::QueryInterface
 * CEnumFormatEtc::AddRef
 * CEnumFormatEtc::Release
 *
 * Purpose:
 *  IUnknown members for CEnumFormatEtc object.  For QueryInterface
 *  we only return out own interfaces and not those of the data
 *  object.  However, since enumerating formats only makes sense
 *  when the data object is around, we insure that it stays as
 *  long as we stay by calling an outer IUnknown for AddRef
 *  and Release.  But since we are not controlled by the lifetime
 *  of the outer object, we still keep our own reference count in
 *  order to free ourselves.
 */

STDMETHODIMP CEnumFormatEtc::QueryInterface( REFIID riid, VOID ** ppv )
{
    *ppv = NULL;

    //
    //  Enumerators are separate objects, not the data object, so
    //  we only need to support out IUnknown and IEnumFORMATETC
    //  interfaces here with no concern for aggregation.
    //

    if( IsEqualIID( riid, IID_IUnknown )
        || IsEqualIID( riid, IID_IEnumFORMATETC ) )
        *ppv = (LPVOID)this;

    //
    //  AddRef any interface we'll return.
    //

    if( NULL!=*ppv )
    {
        ((LPUNKNOWN)*ppv)->AddRef( );

        return NOERROR;
    }

    return ResultFromScode( E_NOINTERFACE );
}


STDMETHODIMP_(ULONG) CEnumFormatEtc::AddRef( void )
{
    ++m_cRef;

    m_pUnkRef->AddRef( );

    return m_cRef;
}

STDMETHODIMP_(ULONG) CEnumFormatEtc::Release( void )
{
    ULONG       cRefT;

    cRefT=--m_cRef;

    m_pUnkRef->Release( );

    if( 0L == m_cRef )
        delete this;

    return cRefT;
}


/*
 * CEnumFormatEtc::Next
 *
 * Purpose:
 *  Returns the next element in the enumeration.
 *
 * Parameters:
 *  cFE             ULONG number of FORMATETCs to return.
 *  pFE             LPFORMATETC in which to store the returned
 *                  structures.
 *  pulFE           ULONG FAR * in which to return how many we
 *                  enumerated.
 *
 * Return Value:
 *  HRESULT         NOERROR if successful, S_FALSE otherwise,
 */

STDMETHODIMP CEnumFormatEtc::Next( ULONG cFE, LPFORMATETC pFE, ULONG FAR *pulFE )
{
    ULONG   cReturn = 0L;

    if( NULL == m_prgfe )
        return ResultFromScode( S_FALSE );

    if( NULL != pulFE )
        *pulFE = 0L;

    if( NULL == pFE || m_iCur >= m_cfe )
        return ResultFromScode( S_FALSE );

    while( m_iCur < m_cfe && cFE > 0 )
    {
        *pFE++ = m_prgfe[ m_iCur++ ];

        cReturn++;

        cFE--;
    }

    if( NULL!=pulFE )
        *pulFE = cReturn;

    return NOERROR;
}


/*
 * CEnumFormatEtc::Skip
 *
 * Purpose:
 *  Skips the next n elements in the enumeration.
 *
 * Parameters:
 *  cSkip           ULONG number of elements to skip.
 *
 * Return Value:
 *  HRESULT         NOERROR if successful, S_FALSE if we could not
 *                  skip the requested number.
 */

STDMETHODIMP CEnumFormatEtc::Skip( ULONG cSkip )
{
    if( ( (m_iCur + cSkip) >= m_cfe ) || NULL == m_prgfe )
        return ResultFromScode( S_FALSE );

    m_iCur += cSkip;
    return NOERROR;
}


/*
 * CEnumFormatEtc::Reset
 *
 * Purpose:
 *  Resets the current element index in the enumeration to zero.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  HRESULT         NOERROR
 */

STDMETHODIMP CEnumFormatEtc::Reset( void )
{
    m_iCur = 0;
    return NOERROR;
}


/*
 * CEnumFormatEtc::Clone
 *
 * Purpose:
 *  Returns another IEnumFORMATETC with the same state as ourselves.
 *
 * Parameters:
 *  ppEnum          LPENUMFORMATETC FAR * in which to return the
 *                  new object.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CEnumFormatEtc::Clone( LPENUMFORMATETC FAR *ppEnum )
{
    LPCEnumFormatEtc    pNew;

    *ppEnum = NULL;

    //
    //  Create the clone
    //

    pNew = new CEnumFormatEtc( m_pUnkRef, m_cfe, m_prgfe );

    if( NULL == pNew )
        return ResultFromScode( E_OUTOFMEMORY );

    pNew->AddRef( );
    pNew->m_iCur = m_iCur;

    *ppEnum = pNew;
    return NOERROR;
}

// ***********************************************************************
// ***********************************************************************
// CFontData members
//

//
// NOTE:  Our preferred drop effect is ALWAYS DROPEFFECT_COPY.
//        The font folder doesn't support the "cut" operation; never has.
//        [brianau - 10/28/98]
//
CLIPFORMAT CFontData::s_CFPerformedDropEffect = 0; // Performed Drop Effect CF atom.
CLIPFORMAT CFontData::s_CFPreferredDropEffect = 0;
CLIPFORMAT CFontData::s_CFLogicalPerformedDropEffect = 0;


CFontData::CFontData( )
   :  m_cRef( 0 ),
      m_poList( 0 ),
      m_dwPerformedDropEffect(DROPEFFECT_NONE),
      m_dwPreferredDropEffect(DROPEFFECT_COPY),
      m_dwLogicalPerformedDropEffect(DROPEFFECT_NONE)
{
    //
    // Get the atom for the "performed effect" clipboard format.
    // This CF has already been added by the shell.  We're just getting the atom.
    //
    if (0 == s_CFPerformedDropEffect)
    {
        s_CFPerformedDropEffect = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_PERFORMEDDROPEFFECT);
        s_CFPreferredDropEffect = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);
        s_CFLogicalPerformedDropEffect = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_LOGICALPERFORMEDDROPEFFECT);
    }
}


CFontData::~CFontData( )
{
    if( m_poList )
    {
        m_poList->vDetachAll( );
        delete m_poList;
    }
}


CFontList * CFontData::poDetachList( )
{
    CFontList * poList = m_poList;

    m_poList = 0;
    return poList;
}


CFontList *CFontData::poCloneList(void)
{
    return m_poList ? m_poList->Clone() : NULL;
}
    


BOOL CFontData::bInit( CFontList * poList )
{
    m_poList = poList;

    return TRUE;
}


//
// *** IUnknown methods ***
//

HRESULT CFontData::QueryInterface( REFIID riid, LPVOID * ppvObj )
{
    *ppvObj = NULL;

    if( riid == IID_IUnknown )
        *ppvObj = (IUnknown*)( IDataObject* ) this;

    if( riid == IID_IDataObject )
        *ppvObj = (IDataObject* ) this;


    if( *ppvObj )
    {
        ((LPUNKNOWN)*ppvObj)->AddRef( );
        return NOERROR;
    }

    return( ResultFromScode( E_NOINTERFACE ) );
}


ULONG  CFontData::AddRef( void )
{
    return( ++m_cRef );
}

ULONG CFontData::Release( void )
{
    ULONG retval;

    retval = --m_cRef;

    if( !retval )
    {
        delete this;
    }

    return( retval );
}


//
// **** IDataObject ****
//

HRESULT CFontData::GetData( FORMATETC *pfe, STGMEDIUM *ps )
{
    if( !( DVASPECT_CONTENT & pfe->dwAspect ) )
        return ResultFromScode( DATA_E_FORMATETC );

    if( ( pfe->cfFormat == CF_HDROP ) && ( pfe->tymed & TYMED_HGLOBAL ) )
    {
        ps->hGlobal = hDropFromList( m_poList );

        if( ps->hGlobal )
        {
            ps->tymed          = TYMED_HGLOBAL;
            ps->pUnkForRelease = NULL;

            return NOERROR;
        }
    }
    else if ((pfe->cfFormat == s_CFPerformedDropEffect) && (pfe->tymed & TYMED_HGLOBAL))
    {
        //
        // The shell called SetData during a previous drag/drop and we stored
        // the performed drop effect in m_dwPerformedDropEffect.  Now, someone
        // is asking for that effect value.
        // Internally, the font folder just calls GetPerformedDropEffect().
        // This GetData code was added so that we complement SetData.
        //
        ps->hGlobal = (HGLOBAL)LocalAlloc(LPTR, sizeof(DWORD));

        if (NULL != ps->hGlobal)
        {
            *((LPDWORD)ps->hGlobal) = m_dwPerformedDropEffect;
            ps->tymed               = TYMED_HGLOBAL;
            ps->pUnkForRelease      = NULL;
            return NOERROR;
        }
        else
            return ResultFromScode(E_UNEXPECTED);
    }
    else if ((pfe->cfFormat == s_CFLogicalPerformedDropEffect) && (pfe->tymed & TYMED_HGLOBAL))
    {
        ps->hGlobal = (HGLOBAL)LocalAlloc(LPTR, sizeof(DWORD));

        if (NULL != ps->hGlobal)
        {
            *((LPDWORD)ps->hGlobal) = m_dwLogicalPerformedDropEffect;
            ps->tymed               = TYMED_HGLOBAL;
            ps->pUnkForRelease      = NULL;
            return NOERROR;
        }
        else
            return ResultFromScode(E_UNEXPECTED);
    }
    else if ((pfe->cfFormat == s_CFPreferredDropEffect) && (pfe->tymed & TYMED_HGLOBAL))
    {
        ps->hGlobal = (HGLOBAL)LocalAlloc(LPTR, sizeof(DWORD));

        if (NULL != ps->hGlobal)
        {
            *((LPDWORD)ps->hGlobal) = m_dwPreferredDropEffect;
            ps->tymed               = TYMED_HGLOBAL;
            ps->pUnkForRelease      = NULL;

            return NOERROR;
        }
        else
            return ResultFromScode(E_UNEXPECTED);
    }
    return ResultFromScode( DATA_E_FORMATETC );
}



HRESULT CFontData::GetDataHere( FORMATETC *pformatetc, STGMEDIUM *pmedium )
{
    return ResultFromScode( E_NOTIMPL );
}


HRESULT CFontData::QueryGetData( FORMATETC *pfe )
{
    HRESULT  hRet = ResultFromScode( S_FALSE );

    //
    //  Check the aspects we support.
    //

    if( !( DVASPECT_CONTENT & pfe->dwAspect ) )
        return ResultFromScode( DATA_E_FORMATETC );

    if (TYMED_HGLOBAL != pfe->tymed)
        return ResultFromScode( DV_E_TYMED );

    if( pfe->cfFormat == CF_HDROP ||
        pfe->cfFormat == s_CFPreferredDropEffect ||
        pfe->cfFormat == s_CFPerformedDropEffect ||
        pfe->cfFormat == s_CFLogicalPerformedDropEffect)
    {
        hRet = NOERROR ;
    }

    return hRet;
}


HRESULT CFontData::GetCanonicalFormatEtc( FORMATETC *pfeIn, FORMATETC *pfeOut )
{
    *pfeOut = *pfeIn;

    pfeOut->ptd = NULL;

    return ResultFromScode( DATA_S_SAMEFORMATETC );
}


HRESULT CFontData::SetData( FORMATETC *pformatetc,
                            STGMEDIUM *pmedium,
                            BOOL fRelease )
{
    HRESULT hr = DATA_E_FORMATETC;

    if ((pformatetc->cfFormat == s_CFPerformedDropEffect) &&
        (pformatetc->tymed & TYMED_HGLOBAL))
    {
        //
        // The shell has called to tell us what the performed
        // drop effect was from the drag/drop operation.  We save
        // the drop effect so we can provide it when someone asks
        // for it through GetData or GetPerformedDropEffect.
        //
        LPDWORD pdw = (LPDWORD)GlobalLock(pmedium->hGlobal);

        if (NULL != pdw)
        {
            m_dwPerformedDropEffect = *pdw;
            GlobalUnlock(pmedium->hGlobal);
            hr = NOERROR;
        }
        else
        {
            hr = E_UNEXPECTED;
        }
        if (fRelease)
            ReleaseStgMedium(pmedium);
    }
    else if ((pformatetc->cfFormat == s_CFLogicalPerformedDropEffect) &&
             (pformatetc->tymed & TYMED_HGLOBAL))
    {
        //
        // The shell has called to tell us what the logical performed
        // drop effect was from the drag/drop operation.  We save
        // the drop effect so we can provide it when someone asks
        // for it through GetData or GetPerformedDropEffect.
        //
        LPDWORD pdw = (LPDWORD)GlobalLock(pmedium->hGlobal);

        if (NULL != pdw)
        {
            m_dwLogicalPerformedDropEffect = *pdw;
            GlobalUnlock(pmedium->hGlobal);
            hr = NOERROR;
        }
        else
        {
            hr = E_UNEXPECTED;
        }
        if (fRelease)
            ReleaseStgMedium(pmedium);
    }
    return hr;
}


HRESULT CFontData::ReleaseStgMedium(LPSTGMEDIUM pmedium)
{
    if (pmedium->pUnkForRelease)
    {
        pmedium->pUnkForRelease->Release();
    }
    else
    {
        switch(pmedium->tymed)
        {
        case TYMED_HGLOBAL:
            GlobalFree(pmedium->hGlobal);
            break;

        case TYMED_ISTORAGE: // depends on pstm/pstg overlap in union
        case TYMED_ISTREAM:
            pmedium->pstm->Release();
            break;

        default:
            ASSERT(0);  // unknown type
        }
    }

    return S_OK;
}


HRESULT CFontData::EnumFormatEtc( DWORD dwDirection, IEnumFORMATETC **ppEnum )
{
    FORMATETC feGet[] = {{ CF_HDROP,                NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
                         { s_CFPreferredDropEffect, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
                         { s_CFPerformedDropEffect, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
                         { s_CFLogicalPerformedDropEffect, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL }
                        };

    FORMATETC feSet[] = {{ s_CFPerformedDropEffect, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
                         { s_CFLogicalPerformedDropEffect, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL }
                        };

    FORMATETC *pfe = NULL;
    INT cfe = 0;

    *ppEnum = NULL;

    switch(dwDirection)
    {
        case DATADIR_GET:
            pfe = feGet;
            cfe = ARRAYSIZE(feGet);
            break;

        case DATADIR_SET:
            pfe = feSet;
            cfe = ARRAYSIZE(feSet);
            break;

        default:
            break;
    }

    if (0 < cfe)
    {
        *ppEnum = new CEnumFormatEtc(this, cfe, pfe);
    }

    if( *ppEnum )
    {
        (*ppEnum)->AddRef( );

        return NOERROR;
    }
    else
        return ResultFromScode( E_FAIL );
}


HRESULT CFontData::DAdvise( FORMATETC  *pformatetc,
                            DWORD advf,
                            IAdviseSink *pAdvSink,
                            DWORD *pdwConnection )
{
    return ResultFromScode( E_NOTIMPL );
}


HRESULT CFontData::DUnadvise( DWORD dwConnection )
{
    return ResultFromScode( E_NOTIMPL );
}


HRESULT CFontData::EnumDAdvise( IEnumSTATDATA **ppenumAdvise )
{
    return ResultFromScode( E_NOTIMPL );
}


BOOL CFontData::bRFR( )
{
    if( m_poList )
    {
        int iCount = m_poList->iCount( );

        for( int i = 0; i < iCount; i++ )
            m_poList->poObjectAt( i )->bRFR( );

        return TRUE;
    }

    return FALSE;
}


BOOL CFontData::bAFR( )
{
    if( m_poList )
    {
        int iCount = m_poList->iCount( );

        for( int i = 0; i < iCount; i++ )
            m_poList->poObjectAt( i )->bAFR( );

        return TRUE;
    }

    return FALSE;
}


// ***********************************************************************
// ***********************************************************************
// CFontView members
//


CFontView::CFontView(void)
{
    DEBUGMSG( (DM_TRACE2, TEXT( "FONTEXT: CFontView object constructed" ) ) );

    m_cRef          = 0;
    m_poFontManager = NULL;
    m_bFamilyOnly   = FALSE;
    m_poPanose      = NULL;

    m_hwndView   = NULL;
    m_hwndList   = NULL;
    m_hwndText   = NULL;
    m_hwndCombo  = NULL;
    m_hwndParent = NULL;

    m_hwndNextClip = NULL;

    m_hImageList      = NULL;
    m_hImageListSmall = NULL;

    m_hmenuCur       = NULL;
    m_psb            = NULL;
    m_uState         = SVUIA_DEACTIVATE;
    m_idViewMode     = IDM_VIEW_ICON;
    m_iViewClickMode = CLICKMODE_DOUBLE;
    m_ViewModeReturn = FVM_ICON;
    m_fFolderFlags   = FWF_AUTOARRANGE;
    m_bDragSource    = FALSE;
    m_iSortColumn    = FFCOL_DEFAULT;
    m_iSortLast      = FFCOL_DEFAULT;
    m_bSortAscending = TRUE;
    m_bUIActivated   = FALSE;
    m_pdtobjHdrop    = NULL;
    m_bResizing      = FALSE;

    m_dwDateFormat   = 0;

    m_uSHChangeNotifyID = 0;

#ifdef TOOLTIP_FONTSAMPLE
    m_hwndToolTip          = NULL;
    m_bShowPreviewToolTip  = FALSE;
    m_iTTLastHit           = -1;
    m_hfontSample          = NULL;

    //
    // Load up the sample text for non-symbol fonts.
    //
    TCHAR szSample[MAX_PATH];
    LoadString(g_hInst, IDS_FONTSAMPLE_TEXT, szSample, ARRAYSIZE(szSample));
    m_pszSampleText = new TCHAR[lstrlen(szSample) + 1];
    if (NULL != m_pszSampleText)
        lstrcpy(m_pszSampleText, szSample);

    //
    // Load up the sample text for symbol fonts.
    //
    LoadString(g_hInst, IDS_FONTSAMPLE_SYMBOLS, szSample, ARRAYSIZE(szSample));
    m_pszSampleSymbols = new TCHAR[lstrlen(szSample) + 1];
    if (NULL != m_pszSampleSymbols)
        lstrcpy(m_pszSampleSymbols, szSample);

#endif

#ifdef ALT_TEXT_COLOR
    m_bShowCompColor = FALSE;
#endif // ALT_TEXT_COLOR
    m_bShowHiddenFonts = FALSE;
    m_hmodHHCtrlOcx = NULL;
    g_cRefThisDll++;
}


CFontView::~CFontView( )
{
   //
   //  These are destroyed by the ListView
   //  if( m_hImageList) ImageList_Destroy( m_hImageList );
   //  if( m_hImageListSmall ) ImageList_Destroy( m_hImageListSmall );
   //

#ifdef TOOLTIP_FONTSAMPLE
   if (NULL != m_hfontSample)
   {
       DeleteObject(m_hfontSample);
       m_hfontSample = NULL;
   }
   delete[] m_pszSampleText;
   delete[] m_pszSampleSymbols;

#endif

    if (NULL != m_hmodHHCtrlOcx)
        FreeLibrary(m_hmodHHCtrlOcx);

   if (NULL != m_poFontManager)
       ReleaseFontManager(&m_poFontManager);

   DEBUGMSG( (DM_TRACE2, TEXT( "FONTEXT: CFontView object blasted out of creation" ) ) );
   g_cRefThisDll--;
}


//
// Build a text string containing characters that represent attributes of a file.
// The attribute characters are assigned as follows:
// (R)eadonly, (H)idden, (S)ystem, (A)rchive, (C)ompressed.
//
LPTSTR CFontView::BuildAttributeString(DWORD dwAttributes, LPTSTR pszString, UINT nChars)
{
    if (NULL != pszString)
    {
        int j = 0;

        if (nChars > NUM_ATTRIB_CHARS)
        {
            int i = 0;

            for (i = 0, j = 0; i < NUM_ATTRIB_CHARS; i++)
                if (dwAttributes & g_adwAttributeBits[i])
                    *(pszString + (j++)) = g_szAttributeChars[i];

        }
        *(pszString + j) = TEXT('\0');
    }

    return pszString;
}


//
// Compare two font objects based upon the character-string representation of their
// file's attributes.
// Returns: -1 = *pFontA is "less than" *pFontB.
//           0 = *pFontA is "equal to" *pFontB.
//           1 = *pFontA is "greater than" *pFontB.
//
int CFontView::CompareByFileAttributes(CFontClass *pFontA, CFontClass *pFontB)
{
    int iResult = 0; // Assume equal

    DWORD mask = FILE_ATTRIBUTE_READONLY  |
                 FILE_ATTRIBUTE_HIDDEN    |
                 FILE_ATTRIBUTE_SYSTEM    |
                 FILE_ATTRIBUTE_ARCHIVE   |
                 FILE_ATTRIBUTE_COMPRESSED;

    if (NULL != pFontA && NULL != pFontB)
    {
        //
        // Calculate value of desired bits in attribute DWORD.
        //
        DWORD dwAttribA = pFontA->dwGetFileAttributes() & mask;
        DWORD dwAttribB = pFontB->dwGetFileAttributes() & mask;

        if (dwAttribA != dwAttribB)
        {
            //
            // If the values are not equal,
            // sort alphabetically based on string representation.
            //
            int diff = 0;
            TCHAR szAttribA[NUM_ATTRIB_CHARS + 1];
            TCHAR szAttribB[NUM_ATTRIB_CHARS + 1];

            //
            // Create attribute string for objects A and B.
            //
            BuildAttributeString(dwAttribA, szAttribA, ARRAYSIZE(szAttribA));
            BuildAttributeString(dwAttribB, szAttribB, ARRAYSIZE(szAttribB));

            //
            // Compare attribute strings and determine difference.
            //
            diff = lstrcmp(szAttribA, szAttribB);

            if (diff > 0)
               iResult = 1;
            if (diff < 0)
               iResult = -1;
        }
    }
    return iResult;
}


int CALLBACK iCompare( LPARAM p1, LPARAM p2, LPARAM p3 )
{
    return( ( (CFontView *)p3)->Compare( (CFontClass *)p1, (CFontClass *)p2) );
}

int CFontView::Compare( CFontClass * pFont1, CFontClass * pFont2 )
{
    //
    //  Sorting by name:
    //     If variations are hidden, then sort by family name.
    //     If not, sort by name. The view doesn't matter.
    //
    //  Sorting by Panose:
    //     If the current selected font doesn't have Panose information,
    //     just sort by name. If neither the compare items has PANOSE
    //     info, just sort by name. If only one has PANOSE, return it.
    //     If both have PANOSE, invoke the mapper to get a comparison.
    //

    int iRet = 0;
    bool bNoNegateResult = false;
    UINT viewMode = m_idViewMode;
    TCHAR szFont1[ MAX_NAME_LEN ];
    TCHAR szFont2[ MAX_NAME_LEN ];

    if( viewMode == IDM_VIEW_PANOSE )
    {
        if( (m_iSortColumn == FFCOL_DEFAULT ||
             m_iSortColumn == FFCOL_PANOSE ) )
        {
            BOOL bPan1 = pFont1->bHavePANOSE( );
            BOOL bPan2 = pFont2->bHavePANOSE( );

            //
            //  If neither has a PAN number, just sort by name.
            //

            if( !( bPan1 || bPan2 ) )
            {
                goto CompareNames;
            }
            else
            {
                if( !m_poPanose )
                {
                    if( bPan1 )
                        iRet = -1;
                    else
                        iRet = 1;

                    goto ReturnCompareResult;
                }

                else if(  !(bPan1 && bPan2 ) )
                {
                    //
                    //  We have m_poPanose, but do we have the others?
                    //
                    if( bPan1 )
                        iRet = -1;
                    if( bPan2 )
                        iRet = 1;
                    //
                    // This flag keeps the "No Panose Information" items
                    // always at the bottom of the view.
                    //
                    bNoNegateResult = true;
                    goto ReturnCompareResult;
                }
            }
        }
        else
        {
            goto CompareNames;
        }
    }

    switch( viewMode )
    {
    default:
        switch( m_iSortColumn )
        {
        case FFCOL_DEFAULT:
        case FFCOL_NAME:
CompareNames:
            if( m_bFamilyOnly )
            {
                pFont1->vGetFamName( szFont1 );
                pFont2->vGetFamName( szFont2 );
            }
            else
            {
                pFont1->vGetName( szFont1 );
                pFont2->vGetName( szFont2 );
            }
            iRet = lstrcmpi( szFont1, szFont2 );
            //
            // Use font type for secondary ordering.
            //
            if (0 == iRet)
            {
                iRet = pFont1->iFontType() - pFont2->iFontType();
                //
                // Force non-zero results to -1 or 1.
                //
                if (0 != iRet)
                    iRet = ((iRet < 0) ? -1 : 1);
            }
            break;

        case FFCOL_FILENAME:
            pFont1->vGetFileName( szFont1 );
            pFont2->vGetFileName( szFont2 );
            iRet = lstrcmpi( szFont1, szFont2 );
            break;

        case FFCOL_SIZE:
            iRet = pFont1->dCalcFileSize( ) - pFont2->dCalcFileSize( );
            if( iRet == 0 )
                goto CompareNames;
            break;

        case FFCOL_MODIFIED:
        {
            FILETIME ft1;
            FILETIME ft2;
            BOOL b1, b2;

            b1 = pFont1->GetFileTime( &ft1 );
            b2 = pFont2->GetFileTime( &ft2 );

            if( b1 && b2 )
            {
                if( ft1.dwHighDateTime == ft2.dwHighDateTime )
                {
                    if( ft1.dwLowDateTime > ft2.dwLowDateTime )
                        iRet = -1;
                    else if( ft1.dwLowDateTime < ft2.dwLowDateTime )
                        iRet = 1;
                    else
                        goto CompareNames;
                }
                else if( ft1.dwHighDateTime > ft2.dwHighDateTime )
                {
                    iRet = -1;
                }
                else
                    iRet = 1;
            }
            else if( !b1 && !b2 )
            {
                goto CompareNames;
            }
            else if( b1 )
            {
                iRet = -1;
            }
            else
                iRet = 1;

            break;
        }

        case FFCOL_ATTRIBUTES:
            iRet = CompareByFileAttributes(pFont1, pFont2);
            if (0 == iRet)
                goto CompareNames;
            break;

        }
        break;

    case IDM_VIEW_PANOSE:
        //
        // At this point, we are guaranteed that all three fonts
        // have PANOSE information.
        // The primary key for sorting is the "range" that the
        // difference between each font's panose number and the
        // panose number of the font currently selected in the dropdown
        // combo box falls within.  It's this range that determines the
        // text displayed next to the font in the listview
        // entry (see CFontView::OnNotify).  If two fonts fall into
        // the same range, we use the font face name as the secondary
        // sort key.  This was not done in Win9x or NT4 but I think
        // it's a helpful feature.  Otherwise, the items within a given
        // range are not sorted. [brianau - 3/24/98]
        //
        {
            //
            // Get the difference in panose number between each of the
            // comparison fonts and the font selected in the dropdown combo
            // (m_poPanose).
            //
            int nDiff1 = m_poFontManager->nDiff(m_poPanose, pFont1);
            int nDiff2 = m_poFontManager->nDiff(m_poPanose, pFont2);
            bool bCompareByName = false;

            //
            // Use a filter approach to determine if the two fonts are in
            // the same range (i.e. "similar to", "very similar to" etc.)
            // If they are, we'll sort on font face name to ensure ordering
            // within the ranges.
            //
            if (nDiff1 < 100)
            {
                if (nDiff2 < 100)
                {
                    //
                    // Both diffs < 100.
                    //
                    if (nDiff1 < 30)
                    {
                        if (nDiff2 < 30)
                        {
                            //
                            // Both diffs < 30.
                            //
                            bCompareByName = true;
                        }
                    }
                    else if (nDiff2 >= 30)
                    {
                        //
                        // Both diffs are between 30 and 99 inclusive.
                        //
                        bCompareByName = true;
                    }
                }
            }
            else if (nDiff2 >= 100)
            {
                //
                // Both diffs >= 100.
                //
                bCompareByName = true;
            }
            if (bCompareByName)
            {
                goto CompareNames;
            }
            else
            {
                //
                // Diffs aren't in the same range so sort
                // based on the difference between the two fonts.
                //
                iRet = nDiff1 - nDiff2;
            }
        }

        break;
    }


ReturnCompareResult:

    if (!m_bSortAscending && !bNoNegateResult)
    {
        //
        // Sort descending.  Invert comparison result.
        //
        iRet *= -1;
    }
    return iRet;
}



int CFontView::RegisterWindowClass( )
{
    DEBUGMSG( (DM_TRACE2, TEXT( "FONTEXT: CFontView::RegisterWindowClass" ) ) );
    WNDCLASS wndclass;

    //
    //  If the class was already regestered return success
    //

    if( GetClassInfo( g_hInst, g_szViewClass, &wndclass ) )
    {
       DEBUGMSG( (DM_TRACE2, TEXT( "FONTEXT: CFontView - class already regestered" ) ) );
       return( 1 );
    }

    wndclass.style         = CS_PARENTDC | CS_DBLCLKS;
    wndclass.lpfnWndProc   = FontViewWndProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = DLGWINDOWEXTRA;

    wndclass.hInstance     = g_hInst;
    wndclass.hIcon         = NULL;
    wndclass.hCursor       = LoadCursor( NULL, IDC_ARROW );
    wndclass.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    wndclass.lpszMenuName  = NULL;
    wndclass.lpszClassName = g_szViewClass;

    return( RegisterClass( &wndclass ) );
}


void CFontView::SortObjects( )
{
    DEBUGMSG( (DM_TRACE1, TEXT( "CFontView::SortObjects called" ) ) );
    // DEBUGBREAK;

    ListView_SortItems( m_hwndList, iCompare, ( LPARAM)this );

    DEBUGMSG( (DM_TRACE1, TEXT( "CFontView::SortObjects done" ) ) );
}


void CFontView::FillObjects( )
{
    HCURSOR hCurs;
    SHELLSTATE ss;
    DWORD fShellState = SSF_SHOWALLOBJECTS;

    hCurs = SetCursor( LoadCursor( NULL, IDC_WAIT ) );

#ifdef ALT_TEXT_COLOR
    //
    // Retrieve the "Show compressed files in alternate color" shell setting.
    // Do this here so we respond to the user changing this setting in View-Options.
    //
    fShellState |= SSF_SHOWCOMPCOLOR;
#endif // ALT_TEXT_COLOR

    SHGetSetSettings(&ss, fShellState, FALSE);
    m_bShowHiddenFonts = ss.fShowAllObjects;

#ifdef ALT_TEXT_COLOR
    m_bShowCompColor = ss.fShowCompColor;
#endif

    if( m_bFamilyOnly )
    {
        if (!m_poFontManager->bWaitOnFamilyReset( ))
        {
            //
            // Got a "terminate" signal while waiting on the
            // family reset thread.  Don't continue.
            //
            return;
        }
    }

    CFontList * poList = m_poFontManager->poLockFontList( );

    //
    //  The current selection.
    //

    CFontList * poSel;

    SendMessage( m_hwndList, WM_SETREDRAW, FALSE, 0 );

    //
    //  Save the current selection, if any, off.
    //

    if( FAILED( GetFontList( &poSel, SVGIO_SELECTION ) ) )
        poSel = 0;

    //
    // Call CFontClass::Release for all font objects contained in the ListView.
    //
    ReleaseFontObjects();

    ListView_DeleteAllItems( m_hwndList );

    m_iHidden = 0;

    if( poList )
    {
        int iCount = poList->iCount( );
        CFontClass * poFont;

        //
        //  Tell the ListView how many objects we have.
        //
        ListView_SetItemCount( m_hwndList, iCount );

        DEBUGMSG( (DM_TRACE1, TEXT( "FillObjects..." ) ) );

        for( int i = 0; i < iCount; i++)
        {
            poFont = poList->poObjectAt( i );
            if (m_bShowHiddenFonts || 0 == (FILE_ATTRIBUTE_HIDDEN & poFont->dwGetFileAttributes()))
            {
                if( !m_bFamilyOnly || poFont->bFamilyFont( ) )
                    AddObject( poFont );
                else
                    m_iHidden++;
            }
        }

        DEBUGMSG( (DM_TRACE1, TEXT( "   ...donE\n" ) ) );

        m_poFontManager->vReleaseFontList( );
    }

    SortObjects( );

    //
    //  Reselect the items that were selected before.
    //

    UINT  nState;

    if( poSel && poSel->iCount( ) )
    {
        int iCount = poSel->iCount( );
        int i;
        int idx;
        LV_FINDINFO lvf;

        lvf.flags = LVFI_PARAM;

        for( i = 0; i < iCount; i++ )
        {
            lvf.lParam = (LPARAM) poSel->poObjectAt( i );
            if( ( idx = ListView_FindItem( m_hwndList, -1, &lvf ) ) > (-1) )
            {
                nState = ListView_GetItemState( m_hwndList, idx, LVIS_SELECTED );
                nState |= LVIS_SELECTED;

                ListView_SetItemState( m_hwndList, idx, nState, LVIS_SELECTED );

                if( !i )
                    ListView_EnsureVisible( m_hwndList, idx, TRUE );
            }
        }

        poSel->vDetachAll( );
        delete poSel;
    }
    else
    {
        ListView_SetItemState( m_hwndList, 0, LVIS_FOCUSED, LVIS_FOCUSED );
    }

    SendMessage( m_hwndList, WM_SETREDRAW, TRUE, 0 );

    int ciParts[] = {-1};

    LRESULT lRet;

    m_psb->SendControlMsg( FCW_STATUS, SB_SETPARTS,
                           sizeof( ciParts )/sizeof( ciParts[ 0 ] ),
                           (LPARAM)ciParts, &lRet );

    UpdateSelectedCount( );

    SetCursor( hCurs );
}


int CFontView::AddObject( CFontClass * poFont )
{
    LV_ITEM item;

    // DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT:AddObject" ) ) );
    // DEBUGBREAK;


#ifdef _DEBUG
    if( !poFont )
    {
        DEBUGMSG( ( DM_ERROR, TEXT( "AddObject() -- poFont is NULL!" ) ) );
        DEBUGBREAK;
    }
#endif

    poFont->AddRef();  // ListView will hold a pointer.

    item.mask     = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
    item.iItem    = 0x7fff;     // add at end
    item.iSubItem = 0;

    item.iImage  = ItemImageIndex(poFont);
    item.pszText = LPSTR_TEXTCALLBACK;
    item.lParam  = (LPARAM) poFont;

    //
    //  If the file isn't in the fonts dir, make it look like a link.
    //
    item.state     = 0;
    item.stateMask = LVIS_OVERLAYMASK;

    if( !poFont->bOnSysDir( ) )
        item.state |= INDEXTOOVERLAYMASK( 1 );

    return( ListView_InsertItem( m_hwndList, &item ) );
}


HRESULT  CFontView::GetUIObjectFromItem( REFIID riid,
                                         LPVOID FAR *ppobj,
                                         UINT nItem )
{
#if 0
    return( ResultFromScode( E_NOTIMPL ) );
#else
    CFontData * poData;
    CFontList * poList;

    HRESULT  hr = GetFontList( &poList, nItem );

    *ppobj = 0;

    if( SUCCEEDED( hr ) )
    {
       poData = new CFontData;

       if( poData && poData->bInit( poList ) )
       {
          poData->AddRef( );
          hr = poData->QueryInterface( riid, ppobj );
          poData->Release( );
       }
    }

    return hr;
#endif
}

LRESULT CFontView::BeginDragDrop( NM_LISTVIEW FAR * lpn )
{
    LPDATAOBJECT   pdtobj;
    POINT          ptOffset = lpn->ptAction;
    DWORD          dwEffect = DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK;
    CFontList *    poList   = 0;


    //
    //  Store the fact that we started in our window.
    //

    m_bDragSource = TRUE;

    //
    //  Save the anchor point.
    //
       // todo

    //
    //  Get the screen point.
    //

    ClientToScreen( m_hwndList, &ptOffset );


    if(SUCCEEDED(GetUIObjectFromItem(IID_IDataObject,
                                     (void **)&pdtobj,
                                     SVGIO_SELECTION)))
    {
        IDragSourceHelper* pDragSource;
        HRESULT hr = SHCoCreateInstance(NULL,
                                        &CLSID_DragDropHelper,
                                        NULL,
                                        IID_IDragSourceHelper,
                                        (void **)&pDragSource);
        if (SUCCEEDED(hr))
        {
            hr = pDragSource->InitializeFromWindow(m_hwndList, &ptOffset, pdtobj);
            pDragSource->Release();
            
            if (SUCCEEDED(hr))
            {
                CFontData *pFontData = (CFontData *)pdtobj;
                //
                //  If we're going to allow a move, then we have to remove the font
                //  from GDI. Otherwise the move will fail 'cause GDI has the
                //  the file open/locked.
                //
                pFontData->bRFR( );
                pFontData->SetPreferredDropEffect(DROPEFFECT_MOVE);
                pFontData->SetPerformedDropEffect(DROPEFFECT_NONE);
                pFontData->SetLogicalPerformedDropEffect(DROPEFFECT_NONE);

                SHDoDragDrop( m_hwndView, pdtobj, NULL, dwEffect, &dwEffect );

                if( DROPEFFECT_MOVE == pFontData->GetLogicalPerformedDropEffect())
                {
                    m_poFontManager->vToBeRemoved(pFontData->poCloneList());
                }
                else
                {
                    pFontData->bAFR( );
                }
            }
       }

       DAD_SetDragImage( NULL, NULL );
       pdtobj->Release( );
    }

    m_bDragSource = FALSE;

    return 0;
}


int CFontView::OnActivate( UINT uState )
{
    if( m_uState != uState )
    {
        HMENU hMenu;

        OnDeactivate( );

        hMenu = CreateMenu( );

        if( hMenu )
        {
            HMENU hMergeMenu;
            OLEMENUGROUPWIDTHS mwidth = { { 0, 0, 0, 0, 0, 0 } };

            m_hmenuCur = hMenu;
            m_psb->InsertMenusSB( hMenu, &mwidth );

            if( uState == SVUIA_ACTIVATE_FOCUS )
            {
                hMergeMenu = LoadMenu( g_hInst,
                                       MAKEINTRESOURCE( MENU_DEFSHELLVIEW ) );

                if( hMergeMenu )
                {
                      MergeFileMenu( hMenu, GetSubMenu( hMergeMenu, 0 ) );
                      MergeEditMenu( hMenu, GetSubMenu( hMergeMenu, 1 ) );
                      MergeViewMenu( hMenu, GetSubMenu( hMergeMenu, 2 ) );
                      MergeHelpMenu( hMenu, GetSubMenu( hMergeMenu, 3 ) );
                      DestroyMenu( hMergeMenu );
                }
            }
            else
            {
                //
                //  SVUIA_ACTIVATE_NOFOCUS
                //

                hMergeMenu = LoadMenu( g_hInst,
                                       MAKEINTRESOURCE( MENU_DEFSHELLVIEW ) );

                if( hMergeMenu )
                {
                   MergeFileMenu( hMenu, GetSubMenu( hMergeMenu, 0 ) );
                   MergeEditMenu( hMenu, GetSubMenu( hMergeMenu, 1 ) );
                   MergeViewMenu( hMenu, GetSubMenu( hMergeMenu, 2 ) );
                   MergeHelpMenu( hMenu, GetSubMenu( hMergeMenu, 3 ) );
                   DestroyMenu( hMergeMenu );
                }
            }
            BOOL bRemovePreviewMenuItem = (CLICKMODE_DOUBLE == m_iViewClickMode);

#ifndef TOOLTIP_FONTSAMPLE
            bRemovePreviewMenuItem = TRUE;
#endif

            if (bRemovePreviewMenuItem)
            {
                //
                // If Tooltip font samples are not in the build, or if
                // the view is in double-click (std windows) mode, we need
                // to remove the "Preview" item from the "View" menu.
                //
                HMENU hmenuView = GetSubMenu(hMenu, 2);
                if (NULL != hmenuView)
                    DeleteMenu(hmenuView, IDM_VIEW_PREVIEW, MF_BYCOMMAND);
            }

            m_psb->SetMenuSB( hMenu, NULL, m_hwndView );
        }

        m_uState = uState;
    }

    return( 1 );
}


int CFontView::OnDeactivate( )
{
    if( m_uState != SVUIA_DEACTIVATE )
    {
        m_psb->SetMenuSB( NULL, NULL, NULL );
        m_psb->RemoveMenusSB( m_hmenuCur );

        DestroyMenu( m_hmenuCur );

        m_hmenuCur = NULL;
        m_uState = SVUIA_DEACTIVATE;
    }

    return( 1 );
}


int CFontView::MergeToolbar( )
{
    static TBBUTTON tbButtons[] = {
       {0,    IDM_VIEW_ICON,    TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0, 0, -1 /* IDS_TB_FONTS */ },
       {1,    IDM_VIEW_LIST,    TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0, 0, -1 /* IDS_TB_FAMILY */ },
       {2,    IDM_VIEW_PANOSE,  TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0, 0, -1 /* IDS_TB_FAMILY */ },
       {3,    IDM_VIEW_DETAILS, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0, 0, -1 /* IDS_TB_PANOSE */ },
#ifdef USE_OWNERDRAW
       {0,    0,               0,                  TBSTYLE_SEP,    0,  0 },
       {4,    IDM_VIEW_ACTUAL, TBSTATE_ENABLED,    TBSTYLE_CHECK,  0, -1 /* IDS_TB_IN_FONT */ },
       {0,    0,                0,                 TBSTYLE_SEP,    0,  0 },
       {5,    IDM_POINT_UP,    TBSTATE_ENABLED,    TBSTYLE_BUTTON, 0, -1 /* IDS_TB_POINT_UP */ },
       {6,    IDM_POINT_DOWN,  TBSTATE_ENABLED,    TBSTYLE_BUTTON, 0, -1 /* IDS_TB_POINT_DOWN */ },
#endif
     };

#define BUTTONCOUNT (sizeof( tbButtons ) / sizeof( TBBUTTON ) )


    //
    //  Add the bitmaps to the cabinet's toolbar (when do we remove them?)
    //

    TBADDBITMAP tbad;

    tbad.hInst = g_hInst;
    tbad.nID   = IDB_TOOLICONS;

    m_psb->SendControlMsg( FCW_TOOLBAR, TB_ADDBITMAP, 7,
                           (LPARAM) &tbad, (LRESULT*) &m_iFirstBitmap );

    DEBUGMSG( (DM_TRACE2, TEXT( "FONTEXT: CFontView::MergeToolbar iFirstBitmap = %d" ),
              m_iFirstBitmap ) );

    //
    //  set the buttons' bitmap indexes then add them to the toolbar
    //

    int i, iSepCount;

    for( i = 0, iSepCount = 0; i < BUTTONCOUNT; i++ )
    {
        if( tbButtons[ i ].fsStyle != TBSTYLE_SEP )
            tbButtons[ i ].iBitmap = i + m_iFirstBitmap - iSepCount;
        else
            iSepCount++;
    }

    m_psb->SetToolbarItems( tbButtons, BUTTONCOUNT, /* FCT_ADDTOEND */ FCT_MERGE );

    return( 1 );
}


#define MAX_FONTSIGNATURE         16   // length of font signature string

INT_PTR CALLBACK CFontView::FontViewDlgProc( HWND hWnd,
                                             UINT message,
                                             WPARAM wParam,
                                             LPARAM lParam )
{
    switch( message )
    {
    case WM_INITDIALOG:
        {
        DEBUGMSG( (DM_MESSAGE_TRACE1,
                  TEXT( "FONTEXT: FontViewWndProc WM_INITDIALOG" ) ) );

       CFontView* prv = (CFontView*)lParam;

#ifdef _DEBUG

        if( !prv )
        {
            DEBUGMSG( (DM_ERROR,TEXT( "FONTEXT: WM_CREATE: Invalid prv" ) ) );
            DEBUGBREAK;
        }
#endif

        SetWindowLongPtr( hWnd, DWLP_USER, (LONG_PTR) prv );

        prv->m_hwndView = hWnd;
        prv->m_hwndList = CreateWindowEx(
                                   WS_EX_CLIENTEDGE,
                                   WC_LISTVIEW, TEXT( "A List View" ),
                                   WSFromViewMode( prv->m_idViewMode ),
                                   0, 0, 50, 50,
                                   hWnd, (HMENU) WID_LISTVIEW, g_hInst, NULL );


        if( !prv->m_hwndList )
        {
           DEBUGMSG( (DM_ERROR, TEXT( "FONTEXT: ListView CreateWindowEx failed" ) ) );
           return( 0 );
        }

        prv->m_dwDateFormat = FDTF_DEFAULT;

        WORD  wLCIDFontSignature[MAX_FONTSIGNATURE];
        //
        // Let's verify this is a RTL (BiDi) locale. call GetLocaleInfo with
        // LOCALE_FONTSIGNATURE which always gives back 16 WORDs.
        //
        if( GetLocaleInfo( GetUserDefaultLCID() ,
                         LOCALE_FONTSIGNATURE ,
                         (LPTSTR) &wLCIDFontSignature ,
                         ARRAYSIZE(wLCIDFontSignature)) )
        {
            // Let's verify the bits we have a BiDi UI locale
            // see \windows\winnls\data\other\locale.txt                          ****
            // FONTSIGNATURE          \x60af\x8000\x3848\x1000\x0008\x0000\x0000\x0800\x0040\x0000\x0000\x2000\x0040\x0000\x0000\x2008
            // if this locale is BiDi UI the 8h word will be 0x0800 else it will be 0x0000 and it does not have any #define name.

            if( wLCIDFontSignature[7] & 0x0800 )
            {
                //Get the real list view windows ExStyle.
                DWORD dwLVExStyle = GetWindowLong(prv->m_hwndList, GWL_EXSTYLE);
                if ((BOOLIFY(dwLVExStyle & WS_EX_RTLREADING)) != (BOOLIFY(dwLVExStyle & WS_EX_LAYOUTRTL)))
                    prv->m_dwDateFormat |= FDTF_RTLDATE;
                else
                    prv->m_dwDateFormat |= FDTF_LTRDATE;
            }
         }

        //
        // Set the listview click mode to conform to the user's preference settings
        // in the shell.  Can be either CLICKMODE_SINGLE or CLICKMODE_DOUBLE.
        //
        prv->SetListviewClickMode();

        prv->m_hAccel = LoadAccelerators( g_hInst, MAKEINTRESOURCE( ACCEL_DEF ) );

        //
        //  Create the ComboBox
        //

        prv->m_hwndCombo = GetDlgItem( hWnd, ID_CB_PANOSE );

        //
        //  Remember the default size of the combo. We won't resize it to
        //  anything bigger than this.

        {
            RECT r;

            GetClientRect( prv->m_hwndCombo, &r );
            prv->m_nComboWid = r.right - r.left;
        }

        //
        //  Create the Text description for the combo box
        //

        prv->m_hwndText = GetDlgItem( hWnd, ID_TXT_SIM );

        int IDC_NUMITEMS (IDI_LASTFONTICON - IDI_FIRSTFONTICON + 1);

        UINT lMirrorStyle = TRUE;

        if ( GetWindowLongPtr(prv->m_hwndList , GWL_EXSTYLE) & WS_EX_LAYOUTRTL )
        {
             lMirrorStyle |= ILC_MIRROR;
        }
        
        prv->m_hImageList = ImageList_Create( 32, 32, lMirrorStyle, IDC_NUMITEMS, 0 );
        prv->m_hImageListSmall = ImageList_Create( 16, 16, lMirrorStyle, IDC_NUMITEMS, 0 );

        //
        //  Load our icons.
        //

        HICON hIcon;
        HICON hIconSmall;
        UINT  i;

        for(  i = IDI_FIRSTFONTICON; i <= IDI_LASTFONTICON; i++ )
        {
            hIcon = (HICON) LoadImage( g_hInst, MAKEINTRESOURCE( i ),
                                       IMAGE_ICON, 0, 0,
                                       LR_DEFAULTCOLOR | LR_DEFAULTSIZE );
            if( hIcon )
            {
               ImageList_AddIcon( prv->m_hImageList, hIcon );

               hIconSmall = (HICON) LoadImage( g_hInst, MAKEINTRESOURCE( i ),
                                               IMAGE_ICON, 16, 16,
                                               LR_DEFAULTCOLOR );

               if( hIconSmall )
               {
                    ImageList_AddIcon( prv->m_hImageListSmall, hIconSmall );
                    DestroyIcon( hIconSmall );
               }

               DestroyIcon( hIcon );
            }
        }

        //
        //  Extract the link icon from SHELL32.DLL
        //

        ExtractIconEx( TEXT( "SHELL32.DLL" ), IDI_X_LINK - 1, &hIcon,
                       &hIconSmall, 1 );

        if( hIcon )
        {
            ImageList_AddIcon( prv->m_hImageList, hIcon );
            DestroyIcon( hIcon );

            if( hIconSmall )
            {
                ImageList_AddIcon( prv->m_hImageListSmall, hIconSmall );
                DestroyIcon( hIconSmall );
            }
            IDC_NUMITEMS++;  // Add 1 for "link" icon.
        }

        //
        //  Specify the overlay images.
        //

        ImageList_SetOverlayImage( prv->m_hImageList, IDC_NUMITEMS - 1, 1 );
        ImageList_SetOverlayImage( prv->m_hImageListSmall, IDC_NUMITEMS - 1, 1 );

        ListView_SetImageList( prv->m_hwndList, prv->m_hImageList, LVSIL_NORMAL );
        ListView_SetImageList( prv->m_hwndList, prv->m_hImageListSmall, LVSIL_SMALL );

        ListView_SetExtendedListViewStyleEx(prv->m_hwndList, 
                                            LVS_EX_LABELTIP, 
                                            LVS_EX_LABELTIP);

#undef IDC_NUMITEMS

        DEBUGMSG( (DM_MESSAGE_TRACE1, TEXT( "FONTEXT: FontViewWndProc WM_INITDIALOG" ) ) );

        //
        //  We need to retrieve more information if we are in details or
        //  panose view.
        //

        if( prv->m_idViewMode == IDM_VIEW_PANOSE )
        {
            prv->vLoadCombo( );
            SetListColumns( prv->m_hwndList, PAN_COL_COUNT, PanoseColumns );
            prv->UpdatePanColumn( );
        }
        else if( prv->m_idViewMode == IDM_VIEW_DETAILS )
        {
            SetListColumns( prv->m_hwndList, FILE_COL_COUNT, FileColumns );
        }

#ifdef TOOLTIP_FONTSAMPLE
        //
        // Create and initialize the tooltip window.
        //
        prv->CreateToolTipWindow();

#endif // TOOLTIP_FONTSAMPLE

        //
        // Register with the shell for file-attribute change notifications.
        //
        SHChangeNotifyEntry fsne;
        fsne.pidl        = NULL;
        fsne.fRecursive  = FALSE;

        prv->m_uSHChangeNotifyID = SHChangeNotifyRegister(prv->m_hwndView,
                                                          SHCNRF_NewDelivery | SHCNRF_ShellLevel,
                                                          SHCNE_UPDATEIMAGE | SHCNE_DISKEVENTS,
                                                          WM_SHELL_CHANGE_NOTIFY,
                                                          1,
                                                          &fsne);

        SetWindowPos(prv->m_hwndCombo,
                     prv->m_hwndList,
                     0, 0, 0, 0,
                     SWP_NOSIZE |
                     SWP_NOMOVE);

        //
        //  Return 0 so SetFocus() doesn't get called.
        //

        return( 0 );
        }
        break;

    default:
        break;
    }

    return( 0 );
}


LRESULT CALLBACK CFontView::FontViewWndProc( HWND hWnd,
                                             UINT message,
                                             WPARAM wParam,
                                             LPARAM lParam )
{
    DEBUGMSG( (DM_MESSAGE_TRACE2, TEXT( "FONTEXT: FontViewWndProc Called m=0x%x wp=0x%x lp=0x%x" ),
              message, wParam, lParam) );

    CFontView* prv = (CFontView*) GetWindowLongPtr( hWnd, DWLP_USER );

    //
    //  'prv' won't be valid for any messages that comes before WM_CREATE
    //

    if( prv )
        return( prv->ProcessMessage( hWnd, message, wParam, lParam ) );
    else
        return( DefDlgProc( hWnd, message, wParam, lParam ) );

    return( 0 );
}


typedef struct {
   UINT  nMenuID;
   UINT  nStatusID;
} MENU_STATUS;


const MENU_STATUS MenuStatusMap[] = {
   {IDM_FILE_SAMPLE     , IDST_FILE_SAMPLE    },
   {IDM_FILE_PRINT      , IDST_FILE_PRINT     },
   {IDM_FILE_INSTALL    , IDST_FILE_INSTALL   },
   {IDM_FILE_LINK       , IDST_FILE_LINK      },
   {IDM_FILE_DEL        , IDST_FILE_DEL       },
   {IDM_FILE_RENAME     , IDST_FILE_RENAME    },
   {IDM_FILE_PROPERTIES , IDST_FILE_PROPERTIES},

   {IDM_EDIT_SELECTALL   , IDST_EDIT_SELECTALL    },
   {IDM_EDIT_SELECTINVERT, IDST_EDIT_SELECTINVERT },
   {IDM_EDIT_CUT         , IDST_EDIT_CUT          },
   {IDM_EDIT_COPY        , IDST_EDIT_COPY         },
   {IDM_EDIT_PASTE       , IDST_EDIT_PASTE        },
   {IDM_EDIT_UNDO        , IDST_EDIT_UNDO         },

   {IDM_VIEW_ICON       , IDST_VIEW_ICON    },
   {IDM_VIEW_LIST       , IDST_VIEW_LIST    },
   {IDM_VIEW_PANOSE     , IDST_VIEW_PANOSE  },
   {IDM_VIEW_DETAILS    , IDST_VIEW_DETAILS },

   {IDM_VIEW_VARIATIONS , IDST_VIEW_VARIATIONS },
   {IDM_VIEW_PREVIEW    , IDST_VIEW_PREVIEW },

   {IDM_HELP_TOPIC      , IDST_HELP_TOPICS },

   // THIS MUST BE LAST !!!!!!
   //
   {IDX_NULL, 0}

};


//
//  This is to put messages in the status bar
//

int CFontView::OnMenuSelect( HWND hWnd,
                             UINT nID,      // Menu item or Popup menu id
                             UINT nFlags,   // Menu flags
                             HMENU hMenu )  // HANDLE of menu clicked.
{
    UINT  nStat = IDX_NULL;

    //
    //  Is the menu closing? i.e. the user pressed escape.
    //

    if( (LOWORD( nFlags ) == 0xffff ) && (hMenu == 0 ) )
    {
            StatusPop( );
            return( 0 );
    }

    //
    //  What to do if this is a popup?
    //

    if( !(nFlags & MF_POPUP ) )
    {
        const MENU_STATUS * pms = MenuStatusMap;

        //
        //  Walk the id to status map
        //

        for(  ; pms->nMenuID != IDX_NULL; pms++ )
        {
            if( pms->nMenuID == nID )
            {
               nStat = pms->nStatusID;
               break;
            }
        }
    }

    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: OnMenuSelect: MenuID: %d   StatusID: %d" ),
               nID, nStat) );

    if( nStat == IDX_NULL )
    {
       StatusClear( );
    }
    else
    {
       StatusPush( nStat );
    }

    return( 0 );
}


inline int InRange( UINT id, UINT idFirst, UINT idLast )
{
    return( (id-idFirst) <= (idLast-idFirst) );
}


int CFontView::OnCommand( HWND hWnd,
                          UINT message,
                          WPARAM wParam,
                          LPARAM lParam )
{
    DEBUGMSG( (DM_MESSAGE_TRACE1, TEXT( "FONTEXT: CFontView::OnCommand Called m=0x%x wp=0x%x lp=0x%x" ),
             message, wParam, lParam) );

    HMENU hMenu = m_hmenuCur;
    UINT uID = GET_WM_COMMAND_ID( wParam, lParam );


    switch( uID )
    {
    case IDM_FILE_DEL:
        OnCmdDelete( );
        break;

    case IDM_EDIT_CUT:
    case IDM_EDIT_COPY:
        OnCmdCutCopy( uID );
        break;

    case IDM_EDIT_PASTE:
        OnCmdPaste( );
        break;

    case ID_CB_PANOSE:
        switch( GET_WM_COMMAND_CMD( wParam, lParam ) )
        {
        case CBN_SELCHANGE:
            {
            DEBUGMSG( (DM_TRACE1,TEXT( "CBN_SELCHANGE" ) ) );

            int iSlot = (int)::SendMessage( m_hwndCombo,  CB_GETCURSEL, 0, 0 );

            if( iSlot != CB_ERR )
                m_poPanose = (CFontClass *)::SendMessage( m_hwndCombo,
                                                          CB_GETITEMDATA,
                                                          iSlot,
                                                          0 );
            else
                m_poPanose = NULL;

            if (FFCOL_PANOSE != m_iSortColumn)
            {
                m_bSortAscending = TRUE;
                m_iSortColumn    = FFCOL_PANOSE;
            }

            SortObjects( );
            m_iSortLast = FFCOL_PANOSE;

            UpdatePanColumn( );

            }
            break;

         default:
            return 0;
         }
         break;

    case IDM_FILE_PROPERTIES:
        OnCmdProperties( );
        break;

    case IDM_EDIT_SELECTALL:
    case IDM_EDIT_SELECTINVERT:
        vToggleSelection( uID == IDM_EDIT_SELECTALL );
        break;

    case IDM_VIEW_VARIATIONS:
        m_bFamilyOnly = !m_bFamilyOnly;

        if ( m_bFamilyOnly &&
            m_poFontManager &&
            m_poFontManager->bFamiliesNeverReset() )
        {
            m_poFontManager->vResetFamilyFlags();
        }

        FillObjects( );

        break;

#ifdef TOOLTIP_FONTSAMPLE

    case IDM_VIEW_PREVIEW:
        m_bShowPreviewToolTip = !m_bShowPreviewToolTip;
        SendMessage(m_hwndToolTip, TTM_ACTIVATE, m_bShowPreviewToolTip, 0);
        break;

#endif // TOOLTIP_FONTSAMPLE

    case IDM_VIEW_ICON:
        m_ViewModeReturn = FVM_ICON;
        goto DoSetViewMode;

    case IDM_VIEW_LIST:
        m_ViewModeReturn = FVM_LIST;
        goto DoSetViewMode;

    case IDM_VIEW_DETAILS:
    case IDM_VIEW_PANOSE:
        m_ViewModeReturn = FVM_DETAILS;
        goto DoSetViewMode;

DoSetViewMode:
        SetViewMode( (UINT)wParam );
        break;

    case IDM_FILE_SAMPLE:
        OpenCurrent( );
        break;

    case IDM_FILE_PRINT:
        PrintCurrent( );
        break;

    case IDM_FILE_INSTALL:
//        if( bCPAddFonts( m_hwndParent ) )
        if( bCPAddFonts( m_hwndView ) )
        {
            vCPWinIniFontChange( );
        }
        break;

    case IDM_POPUP_MOVE:
        DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: OnCommand: IDM_POPUP_MOVE" ) ) );
        m_dwEffect = DROPEFFECT_MOVE;
        break;

    case IDM_POPUP_COPY:
        DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: OnCommand: IDM_POPUP_COPY" ) ) );
        m_dwEffect = DROPEFFECT_COPY;
        break;

    case IDM_POPUP_LINK:
        DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: OnCommand: IDM_POPUP_LINK" ) ) );
        m_dwEffect = DROPEFFECT_LINK;
        break;

    case IDCANCEL:
    case IDM_POPUP_CANCEL:
        DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: OnCommand: IDM_POPUP_CANCEL" ) ) );
        m_dwEffect = DROPEFFECT_NONE;
        break;

    case IDM_HELP_TOPIC:
        OnHelpTopics(m_hwndView);
        return 0;
    }

    return( 1L );

}

typedef HWND (WINAPI * PFNHTMLHELPA)(HWND, LPCSTR, UINT, ULONG_PTR);

void
CFontView::OnHelpTopics(
    HWND hWnd
    )
{
    if (NULL == m_hmodHHCtrlOcx)
    {
        //
        // Don't want to statically link to hhctrl.ocx so we load it
        // on demand.
        // We'll call FreeLibrary in the CFontView dtor.
        //
        m_hmodHHCtrlOcx = LoadLibrary(TEXT("hhctrl.ocx"));
    }

    if (NULL != m_hmodHHCtrlOcx)
    {
        PFNHTMLHELPA pfnHelp = (PFNHTMLHELPA)GetProcAddress(m_hmodHHCtrlOcx, "HtmlHelpA");
        if (NULL != pfnHelp)
        {
            const char szHtmlHelpFileA[]  = "FONTS.CHM > windefault";
            const char szHtmlHelpTopicA[] = "windows_fonts_overview.htm";

            (*pfnHelp)(hWnd,
                       szHtmlHelpFileA,
                       HH_DISPLAY_TOPIC,
                       (DWORD_PTR)szHtmlHelpTopicA);
        }
    }
}


//
// This code was taken from shell32's defview.
//
void
CFontView::UpdateUnderlines(
    void
    )
{
    DWORD cb;
    DWORD dwUnderline = ICON_IE;
    DWORD dwExStyle;

    //
    // Read the icon underline settings.
    //
    cb = sizeof(dwUnderline);
    SHRegGetUSValue(TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"),
                    TEXT("IconUnderline"),
                    NULL,
                    &dwUnderline,
                    &cb,
                    FALSE,
                    &dwUnderline,
                    cb);

    //
    // If it says to use the IE link settings, read them in.
    //
    if (dwUnderline == ICON_IE)
    {
        dwUnderline = ICON_YES;

        TCHAR szUnderline[8];
        cb = sizeof(szUnderline);
        SHRegGetUSValue(TEXT("Software\\Microsoft\\Internet Explorer\\Main"),
                        TEXT("Anchor Underline"),
                        NULL,
                        szUnderline,
                        &cb,
                        FALSE,
                        szUnderline,
                        cb);

        //
        // Convert the string to an ICON_ value.
        //
        if (!lstrcmpi(szUnderline, TEXT("hover")))
            dwUnderline = ICON_HOVER;
        else if (!lstrcmpi(szUnderline, TEXT("no")))
            dwUnderline = ICON_NO;
        else
            dwUnderline = ICON_YES;
    }

    //
    // Convert the ICON_ value into an LVS_EX value.
    //
    switch (dwUnderline)
    {
        case ICON_NO:
            dwExStyle = 0;
            break;

        case ICON_HOVER:
            dwExStyle = LVS_EX_UNDERLINEHOT;
            break;

        case ICON_YES:
            dwExStyle = LVS_EX_UNDERLINEHOT | LVS_EX_UNDERLINECOLD;
            break;
    }

    //
    // Set the new LVS_EX_UNDERLINE flags.
    //
    ListView_SetExtendedListViewStyleEx(m_hwndList,
                                        LVS_EX_UNDERLINEHOT |
                                        LVS_EX_UNDERLINECOLD,
                                        dwExStyle);
}


//
// Set the proper listview "click mode" depending upon the user's
// preferences in the shell.
// Returns: New click mode. CLICKMODE_SINGLE or CLICKMODE_DOUBLE.
//
CFontView::CLICKMODE
CFontView::SetListviewClickMode(
    VOID
    )
{
    SHELLSTATE ss;
    DWORD dwStyle;
    BOOL bSingleClick;

    //
    // Get the current double-click setting in the shell (user pref).
    //
    SHGetSetSettings(&ss, SSF_WIN95CLASSIC | SSF_DOUBLECLICKINWEBVIEW, FALSE);

    //
    // Get the current listview style bits.
    //
    dwStyle = ListView_GetExtendedListViewStyle(m_hwndList);

    //
    // We get single-click if user wants no web view or single-click in web view.
    // SINGLECLICK = WEBVIEW && !DBLCLICKINWEBVIEW
    //
    bSingleClick = !ss.fWin95Classic && !ss.fDoubleClickInWebView;

    if (bSingleClick)
    {
        if (0 == (dwStyle & LVS_EX_ONECLICKACTIVATE))
        {
            //
            // User wants single click but list is double click..
            // Set listview to single-click mode.
            //
            ListView_SetExtendedListViewStyleEx(m_hwndList,
                                               LVS_EX_TRACKSELECT | LVS_EX_ONECLICKACTIVATE | LVS_EX_TWOCLICKACTIVATE,
                                               LVS_EX_TRACKSELECT | LVS_EX_ONECLICKACTIVATE);
        }
    }
    else if (dwStyle & LVS_EX_ONECLICKACTIVATE)
    {
        //
        // Turn off preview tooltips.
        //
        m_bShowPreviewToolTip = FALSE;
        SendMessage(m_hwndToolTip, TTM_ACTIVATE, m_bShowPreviewToolTip, 0);

        //
        // User wants double click but list is single click.
        // Set listview to double click mode.
        //
        ListView_SetExtendedListViewStyleEx(m_hwndList,
                                           LVS_EX_TRACKSELECT | LVS_EX_ONECLICKACTIVATE | LVS_EX_TWOCLICKACTIVATE,
                                           0);
    }
    m_iViewClickMode = bSingleClick ? CLICKMODE_SINGLE : CLICKMODE_DOUBLE;

    UpdateUnderlines();

    return m_iViewClickMode;
}


void CFontView::UpdateSelectedCount( )
{
    int iCount;
    int iTemplate;
    TCHAR szText[ 128 ];
    TCHAR szStatus[ 128 ];

    iCount = (int) SendMessage( m_hwndList, LVM_GETSELECTEDCOUNT, 0, 0 );

    iTemplate = IDS_SELECTED_FONT_COUNT;

    if( !iCount )
    {
        iCount = (int) SendMessage( m_hwndList, LVM_GETITEMCOUNT, 0, 0 );
        iTemplate = IDS_TOTAL_FONT_COUNT;

        if( m_iHidden )
        {
            iTemplate = IDS_TOTAL_AND_HIDDEN_FONT_COUNT;
        }
    }

    LoadString( g_hInst, iTemplate, szText, ARRAYSIZE( szText ) );

    wsprintf( szStatus, szText, iCount, m_iHidden );

    HWND hwndStatus;

    m_psb->GetControlWindow( FCW_STATUS, &hwndStatus );

    SendMessage( hwndStatus, SB_SETTEXT, 0, (LPARAM) szStatus );
}


//
// Update a font object in the font view following a shell change notification.
// This picks up the color change (if desired by user) and the change to the
// attributes column in details view.
//
void CFontView::UpdateFontViewObject(CFontClass *pFont)
{
    if (NULL != pFont)
    {
        int i = 0;
        LV_FINDINFO lvfi;
        lvfi.flags    = LVFI_PARAM;
        lvfi.psz      = NULL;
        lvfi.lParam   = (LPARAM)pFont;

        //
        // Get the list view index for the object and redraw it.
        //
        i = ListView_FindItem(m_hwndList, -1, &lvfi);
        if (-1 != i)
        {
            ListView_RedrawItems(m_hwndList, i, i);
        }
    }
}


//
// Handles a shell change notification.
// If the path passed in the notification is the path to a font file in the folder,
// the function invalidates the font's cached file attributes and updates the
// object's visual appearance.
//
int CFontView::OnShellChangeNotify(WPARAM wParam, LPARAM lParam)
{
    LPSHChangeNotificationLock pshcnl;
    LPITEMIDLIST *ppidl = NULL;
    LONG lEvent = 0;

    pshcnl = SHChangeNotification_Lock((HANDLE)wParam, (DWORD)lParam, &ppidl, &lEvent);
        
    if (NULL != pshcnl && NULL != ppidl && NULL != *ppidl)
    {
        LPITEMIDLIST pidlPath = *ppidl;

        TCHAR szPath[MAX_PATH];
        LPTSTR pszFileName = NULL;

        if (SHGetPathFromIDList(pidlPath, szPath))
        {
            pszFileName = PathFindFileName(szPath);
            CFontClass *pFont = m_poFontManager->poSearchFontListFile(pszFileName);

            if (NULL != pFont)
            {
                //
                // This event applies to a font object.
                // Invalidate the font object's cached file attributes.
                // Update the font object's visual appearance.
                //
                pFont->InvalidateFileAttributes();
                UpdateFontViewObject(pFont);
            }
        }
        SHChangeNotification_Unlock(pshcnl);
    }
    return (int)lParam;
}


//
// Handle custom draw notification from list view.
// This is where we tell the list view to draw the item in
// normal (uncompressed) or alternate (compressed) color.
// Note this is for NT only.  Non-NT tells the listview
// control to use the default color.
//
int CFontView::OnCustomDrawNotify(LPNMHDR lpn)
{

#ifdef ALT_TEXT_COLOR

    LPNMLVCUSTOMDRAW lpCD = (LPNMLVCUSTOMDRAW)lpn;

    switch (lpCD->nmcd.dwDrawStage)
    {
        case CDDS_PREPAINT:
            return m_bShowCompColor ? CDRF_NOTIFYITEMDRAW : CDRF_DODEFAULT;

        case CDDS_ITEMPREPAINT:
            {
                LV_DISPINFO *pnmv = (LV_DISPINFO *) lpn;
                CFontClass * poFont = (CFontClass *)( pnmv->item.lParam );
                if (NULL != poFont)
                {
                    if (DWORD(-1) != poFont->dwGetFileAttributes() &&
                        poFont->dwGetFileAttributes() & FILE_ATTRIBUTE_COMPRESSED)
                    {
                        lpCD->clrText = g_crAltColor;
                    }
                }
                return CDRF_DODEFAULT;
            }
    }
#endif // ALT_TEXT_COLOR

    return CDRF_DODEFAULT;
}



int CFontView::OnNotify( LPNMHDR lpn )
{
    NM_LISTVIEW FAR * pnmv = (NM_LISTVIEW FAR *) lpn;

    // DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT - WM_NOTIFY with code: %d(%x)" ), ((NMHDR*)lParam)->code, ((NMHDR*)lParam)->code ) );

    switch( lpn->code )
    {
    case TBN_BEGINDRAG:
        OnMenuSelect( 0, ((LPTBNOTIFY) lpn)->iItem, 0, 0 );
        break;

    case TBN_ENDDRAG:
        StatusPop( );
        break;

    case NM_CUSTOMDRAW:
        return OnCustomDrawNotify(lpn);

    case LVN_ITEMACTIVATE:
        OnLVN_ItemActivate((LPNMITEMACTIVATE)lpn);
        break;

    case LVN_BEGINDRAG:
    case LVN_BEGINRDRAG:
        BeginDragDrop( pnmv );
        break;

#ifdef TOOLTIP_FONTSAMPLE

    case LVN_HOTTRACK:
        LV_OnHoverNotify((LPNMLISTVIEW)lpn);
        break;

#endif // TOOLTIP_FONTSAMPLE


    case LVN_GETINFOTIP:
        LV_OnGetInfoTip((LPNMLVGETINFOTIP)lpn);
        break;        

    case LVN_GETDISPINFO:
        {
        LV_DISPINFO FAR *pnmv = (LV_DISPINFO FAR *) lpn;
        CFontClass * poFont = (CFontClass *)( pnmv->item.lParam );
        UINT mask = pnmv->item.mask;

        //
        // DEBUGMSG( (DM_MESSAGE_TRACE1, TEXT( "FONTEXT: LVN_GETDISPINFO for item %d, subitem %d" ),
        //      pnmv->item.iItem, pnmv->item.iSubItem) );

        if( mask & LVIF_TEXT )
        {
            // DEBUGMSG( (DM_MESSAGE_TRACE1, TEXT( "   Wants text information" ) ) );

            static TCHAR szText[ 64 ];

            //
            //  The text we get depends on the view we are currently in.
            //

            switch( m_idViewMode )
            {
            default:
            // case IDM_VIEW_ICON:
            // case IDM_VIEW_LIST:
                if( m_bFamilyOnly )
                    poFont->vGetFamName( szText );
                else
                    poFont->vGetName( szText );
                break;

            case IDM_VIEW_PANOSE:
                switch( pnmv->item.iSubItem )
                {
                default:
                // case 0:
                    if( m_bFamilyOnly )
                        poFont->vGetFamName( szText );
                    else
                        poFont->vGetName( szText );
                    break;

                case 1:  // Panose match info.
                    if( !m_poPanose )
                        szText[ 0 ] =0;
                    else if( !poFont->bHavePANOSE( ) )
                    {
                        LoadString( g_hInst, IDS_NO_PAN_INFO, szText,
                                    ARRAYSIZE( szText ) );
                    }
                    else
                    {
                        USHORT nDiff = m_poFontManager->nDiff( m_poPanose,
                                                               poFont );

                        if( nDiff < 30 )
                            LoadString( g_hInst, IDS_PAN_VERY_SIMILAR, szText,
                                        ARRAYSIZE( szText ) );
                        else if( nDiff < 100 )
                            LoadString( g_hInst, IDS_PAN_SIMILAR, szText,
                                        ARRAYSIZE( szText ) );
                        else
                            LoadString( g_hInst, IDS_PAN_NOT_SIMILAR, szText,
                                        ARRAYSIZE( szText ) );
                    }
                    break;
                }
                break;

            case IDM_VIEW_DETAILS:
                switch( pnmv->item.iSubItem )
                {
                default:
                case 0:
                    poFont->vGetName( szText );
                    break;

                case 1:
                    poFont->vGetFileName( szText );
                    break;

                case 2:  // Size
                    {
                    TCHAR szFmt[ 64 ];

                    LoadString( g_hInst, IDS_FMT_FILEK, szFmt,
                                ARRAYSIZE( szFmt ) );

                    wsprintf( szText, szFmt, poFont->dCalcFileSize( ) );
                    }
                    break;

                case 3:  // Modification date and time.
                    {
                        FILETIME ft;
                        DWORD dwFormat = m_dwDateFormat;

                        szText[ 0 ] = 0;

                        if( poFont->GetFileTime( &ft ) )
                        {
                            SHFormatDateTime(&ft, &dwFormat, szText, ARRAYSIZE(szText));
                        }
                    }
                    break;

                case 4:  // File attributes.
                    BuildAttributeString(poFont->dwGetFileAttributes(),
                                         szText,
                                         ARRAYSIZE(szText));
                    break;

                }
                break;
            }  // switch

            lstrcpyn(pnmv->item.pszText, szText, pnmv->item.cchTextMax);
        } // LVIF_TEXT
        } // LVN_GETDISPINFO
        break;


    case LVN_ITEMCHANGED:
        if( pnmv->uChanged & LVIF_STATE &&
            ( (pnmv->uNewState ^ pnmv->uOldState) & LVIS_SELECTED) )
        {
            UpdateSelectedCount( );
        }
        break;


    case LVN_COLUMNCLICK:
    {
        //
        //  start and stops busy cursor
        //

        WaitCursor cWaiter;

        m_iSortColumn = pnmv->iSubItem;
        if (m_iSortColumn != m_iSortLast)
        {
            //
            // Selected new column.  Reset sort order.
            //
            m_bSortAscending = TRUE;
        }
        else
        {
            //
            // Column selected for a second time. Invert the sort order.
            //
            m_bSortAscending = !m_bSortAscending;
        }

        SortObjects( );

        m_iSortLast = m_iSortColumn;

        break;
    }

    case TTN_NEEDTEXT:
         if( lpn->idFrom >= IDM_VIEW_ICON && lpn->idFrom <= IDM_VIEW_DETAILS )
         {
            //
            //  Query for some text to display for the toolbar.
            //

            LPTOOLTIPTEXT lpt = (LPTOOLTIPTEXT)lpn;

            lpt->lpszText = (LPTSTR) MAKEINTRESOURCE( IDS_VIEW_ICON
                                                      + lpn->idFrom
                                                      - IDM_VIEW_ICON );
            lpt->hinst = g_hInst;
        }
        break;

    case NM_SETFOCUS:
        //
        //   We should call IShellBrowser::OnViewWindowActive() before
        //   calling its InsertMenus().
        //

        m_psb->OnViewWindowActive( this );

        //
        // Only call OnActivate() if UIActivate() has been called.
        // If OnActivate() is called before UIActivate(), the menus
        // are merged before IShellView is properly activated.
        // This results in missing menu items.
        //
        if (m_bUIActivated)
            OnActivate( SVUIA_ACTIVATE_FOCUS );

        break;

    } // switch( notify code )

    return( 0 );
}


void
CFontView::LV_OnGetInfoTip(
    LPNMLVGETINFOTIP lpnm
    )
{
    LVITEM item;

    item.iItem    = lpnm->iItem;
    item.iSubItem = 0;
    item.mask     = LVIF_PARAM;
    if (ListView_GetItem(m_hwndList, &item) && 0 != item.lParam)
    {
        NMLVDISPINFO lvdi;
        ZeroMemory(&lvdi, sizeof(lvdi));

        lvdi.hdr.code        = LVN_GETDISPINFO;
        lvdi.hdr.hwndFrom    = m_hwndList;
        lvdi.item.iItem      = lpnm->iItem;
        lvdi.item.iSubItem   = lpnm->iSubItem;
        lvdi.item.mask       = LVIF_TEXT;
        lvdi.item.pszText    = lpnm->pszText;
        lvdi.item.cchTextMax = lpnm->cchTextMax;
        lvdi.item.lParam     = item.lParam;
        OnNotify((LPNMHDR)&lvdi);
    }
}    



//
// Handle an item in the listview being clicked with the mouse.
//
VOID
CFontView::OnLVN_ItemActivate(
    LPNMITEMACTIVATE pnma
    )
{
    if (LVKF_ALT & pnma->uKeyFlags)
    {
        //
        // ALT+Click --> Properties page.
        // Click (dbl or single) dependent on user's settings.
        //
        OnCmdProperties();
    }
    else
    {
        //
        // Opening a font invokes the font viewer.
        //
        OpenCurrent();
    }
}


//
// Determine which icon represents a font.
//
INT
CFontView::ItemImageIndex(
    CFontClass *poFont
    )
{
    //
    // All indexes are offsets in the imagelist.
    // Default to an FON file.
    //
    INT iImage = IDI_FON - IDI_FIRSTFONTICON;

    //
    // First check for TTC because TTC is also TrueType.
    //
    if(poFont->bTTC())
    {
        iImage = IDI_TTC - IDI_FIRSTFONTICON;
    }
    else if (poFont->bOpenType() || poFont->bTrueType())
    {
        //
        // Use our IExtractIcon icon handler to determine
        // which icon to display for the TrueType or OpenType font.
        //
        // BUGBUG:  The problem with this is that it opens the
        //          font files for a second time.  Font files
        //          are opened for identification when they're
        //          first loaded into the folder.  Ideally, we would
        //          identify the appropriate icon at that point.
        //          I don't want to mess with that code right now.
        //          This code path will result in exactly the same icon
        //          used by the shell so this will ensure consistency.
        //          [brianau - 6/13/97]
        //
        TCHAR szFileName[MAX_PATH];
        poFont->bGetFQName(szFileName, ARRAYSIZE(szFileName));

        iImage = IDI_TTF;
        if (SUCCEEDED(m_IconHandler.Load(szFileName, 0)))
        {
            UINT uFlags = 0;
            m_IconHandler.GetIconLocation(GIL_FORSHELL,
                                          L"", // Force call to wide-char version.
                                          0,
                                          &iImage,
                                          &uFlags);
        }
        ASSERT(IDI_OTFp == iImage ||
               IDI_OTFt == iImage ||
               IDI_TTF  == iImage);

        iImage -= IDI_FIRSTFONTICON;
    }
    else if(poFont->bType1())
    {
        iImage = IDI_T1 - IDI_FIRSTFONTICON;
    }

    return iImage;
}



void CFontView::OnDropFiles( HDROP hDrop, DWORD dwEffect )
{
    FullPathName_t szFile;
    BOOL           bAdded = FALSE;

    //
    //  Starts and stops busy cursor
    //

    WaitCursor    cWaiter;
    UINT cnt = ::DragQueryFile( hDrop, (UINT)-1, NULL, 0 );

    for( UINT i = 0; i < cnt; )
    {
        ::DragQueryFile( hDrop, i, szFile, ARRAYSIZE( szFile ) );

        StatusPush( szFile );

        switch( CPDropInstall( m_hwndView,
                               szFile,
                               dwEffect,
                               NULL,
                               (int) (cnt - ++i) ) )
        {
        case CPDI_SUCCESS:
            bAdded = TRUE;
            break;

        case CPDI_FAIL:
            break;

        case CPDI_CANCEL:
            i = cnt; // leave the loop;
            break;
        }

        StatusPop( );
    }

    ::DragFinish( hDrop );

    if( bAdded )
    {
        vCPWinIniFontChange( );
    }

}


void CFontView::UpdateMenuItems( HMENU hMenu )
{
    CheckMenuRadioItem( hMenu,
                        IDM_VIEW_ICON,
                        IDM_VIEW_DETAILS,
                        m_idViewMode,
                        MF_BYCOMMAND );

    //
    //  Enable and disable those items based on selection.
    //

    UINT nFlag = ((iCurrentSelection( ) > 0 ) ? cLiveMenu : cDeadMenu );


    EnableMenuItem( hMenu, IDM_FILE_SAMPLE    , nFlag );
    EnableMenuItem( hMenu, IDM_FILE_PRINT     , nFlag );
    EnableMenuItem( hMenu, IDM_FILE_DEL       , nFlag );
    EnableMenuItem( hMenu, IDM_FILE_PROPERTIES, nFlag );
    EnableMenuItem( hMenu, IDM_EDIT_COPY      , nFlag );

    //
    //  If there is a file with a format on the clipboard that we can
    //  understand, then enable the menu.
    //

    nFlag = ((IsClipboardFormatAvailable( CF_HDROP ) ) ? cLiveMenu : cDeadMenu );

    EnableMenuItem( hMenu, IDM_EDIT_PASTE     , nFlag );

    //
    //  Are we hiding the variations of a font?
    //
    CheckMenuItem( hMenu, IDM_VIEW_VARIATIONS, m_bFamilyOnly ? MF_CHECKED
                                                             : MF_UNCHECKED );

#ifdef TOOLTIP_FONTSAMPLE
    CheckMenuItem(hMenu, IDM_VIEW_PREVIEW, m_bShowPreviewToolTip ? MF_CHECKED
                                                                 : MF_UNCHECKED );
#endif

}


void CFontView::UpdateToolbar( )
{
    LRESULT    lRet;

    m_psb->SendControlMsg( FCW_TOOLBAR, TB_CHECKBUTTON,
                           m_idViewMode, MAKELONG( TRUE, 0 ), &lRet );

}


HRESULT CFontView::GetFontList( CFontList **ppoList, UINT nItem )
{
    BOOL        bDeleteFam = m_bFamilyOnly;
    CFontClass* lpFontRec;
    LV_ITEM     Item;

    *ppoList = 0;

    if( nItem != SVGIO_SELECTION )
    {
        return ResultFromScode( E_FAIL );
    }

    //
    //  Build up a list of the fonts to be deleted
    //

    CFontList * poList = new CFontList( 10, 10 );

    if( !poList || !poList->bInit( ) )
    {
        return ResultFromScode( E_FAIL );
    }

    //
    //  Save it.
    //

    *ppoList = poList;

    //
    //  Start at the beginning.
    //

    int i = -1;

    UINT  nFlags = LVNI_SELECTED | LVNI_ALL;

    Item.mask = LVIF_PARAM;

    while( ( i = ListView_GetNextItem( m_hwndList, i, nFlags ) ) != -1 )
    {
        Item.iItem = i;
        Item.iSubItem = 0;

        ListView_GetItem( m_hwndList, &Item );

        lpFontRec = ( CFontClass *)Item.lParam;

        //
        //  If this font represents a whole family, then delete the whole
        //  family.
        //

        if( m_bFamilyOnly )
            m_poFontManager->vGetFamily( lpFontRec, poList );
        else
            poList->bAdd( lpFontRec );

    }  //  End of loop on selected fonts.

    return NOERROR;
}


void CFontView::OnCmdPaste( )
{
    //
    //  associate it with current task
    //

    if( OpenClipboard( NULL ) )
    {
        HGLOBAL hmem = GetClipboardData( CF_HDROP );

        if( hmem )
        {
            //
            // There is a CF_HDROP. Create CIDLData from it.
            //

            UINT  cb    = (UINT)GlobalSize( hmem );
            HDROP hdrop = (HDROP) GlobalAlloc( GPTR, cb );

            if( hdrop )
            {
                hmemcpy( (LPVOID)hdrop, GlobalLock( hmem ), cb );

                GlobalUnlock( hmem );

                OnDropFiles( hdrop, DROPEFFECT_COPY );

                //
                //  We already free this in OnDropFiles, so don't free again
                //
                //  GlobalFree( hdrop );
                //
            }
        }
        CloseClipboard( );
    }
}


void CFontView::OnCmdCutCopy( UINT nID )
{
    LPDATAOBJECT   pdtobj;


    if( SUCCEEDED( GetUIObjectFromItem( IID_IDataObject,
                                        (void **) &pdtobj,
                                        SVGIO_SELECTION ) ) )
    {
        FFSetClipboard( pdtobj );

        pdtobj->Release( );
    }
}


void CFontView::OnCmdDelete( )
{
    BOOL        bDeleteFile = FALSE;
    BOOL        bSomeDel    = FALSE;
    CFontList * poList;


    if( !SUCCEEDED( GetFontList( &poList, SVGIO_SELECTION ) ) )
    {
        iUIErrMemDlg(m_hwndList);
        return;
    }

    //
    //  Update our view and notify other apps of font changes.
    //

    if( poList->iCount( ) )
    {
        //
        //  warn first so that canceling out now won't nuke all the fonts
        //  you have selected in
        //
        if( iUIMsgBox( m_hwndList, IDSI_FMT_DELETECONFIRM, IDS_MSG_CAPTION,
                       MB_YESNO | MB_ICONEXCLAMATION, NULL ) == IDYES )
        {
           m_poFontManager->vDeleteFontList( poList );
//         vCPWinIniFontChange( );
        }
    }

    //
    //  empty the font list. They are all deleted anyway.
    //

    poList->vDetachAll( );
    delete poList;
}


void CFontView::OnCmdProperties( )
{
#if 1 // def EMR
    CFontClass*    poFont;
    BOOL           bDeleteFam  = m_bFamilyOnly;
    BOOL           bDeleteFile = FALSE;
    BOOL           bSomeDel    = FALSE;
    BOOL           bYesAll     = FALSE;

    FullPathName_t szFile;

    LV_ITEM          Item;
    SHELLEXECUTEINFO se;

    static TCHAR  szCmd[] = TEXT( "Properties" );

    //
    //  Start at the beginning.
    //

    int i = -1;

    UINT  nFlags = LVNI_SELECTED | LVNI_ALL;


    Item.mask = LVIF_PARAM;

    while( ( i = ListView_GetNextItem( m_hwndList, i, nFlags ) ) != -1 )
    {
        Item.iItem    = i;
        Item.iSubItem = 0;

        ListView_GetItem( m_hwndList, &Item );

        poFont = (CFontClass *) Item.lParam;

        poFont->vGetDirFN( szFile );

        // SHObjectProperties( m_hwndParent, SHOP_FILEPATH, szFile, NULL );

        memset( &se, 0, sizeof( se ) );

        se.cbSize = sizeof( se );
        se.fMask  = SEE_MASK_INVOKEIDLIST;
        se.hwnd   = m_hwndView;
        se.lpVerb = szCmd;
        se.lpFile = szFile;
        se.nShow  = 1;

        ShellExecuteEx( &se );
    }
#endif
}


void CFontView::OldDAD_DropTargetLeaveAndReleaseData(void)
{
    DragLeave();

    if (NULL != m_pdtobjHdrop)
    {
        m_pdtobjHdrop->Release();
        m_pdtobjHdrop = NULL;
    }
}


//
// CFontView::OldDAD_HandleMessages
//
// This function handles all messages associated with Win3.1-style drag-and-drop operations.
// The code was copied from a similar implementation in DEFVIEWX.C.  Minor changes have been
// made to create CFontView member functions and to produce the correct drag-and-drop
// behavior for the font folder.
//
LRESULT CFontView::OldDAD_HandleMessages(UINT message, WPARAM wParam, const DROPSTRUCT *lpds)
{
    DWORD dwAllowedDADEffect = DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK;

    //
    // We don't need to do this hack if NT defined POINT as typedef POINTL.
    //
    union {
        POINT ptScreen;
        POINTL ptlScreen;
    } drop;

    ASSERT(SIZEOF(drop.ptScreen)==SIZEOF(drop.ptlScreen));

    if (NULL != lpds)   // Notes: lpds is NULL, if uMsg is WM_DROPFILES.
    {
        drop.ptScreen = lpds->ptDrop;
        ClientToScreen(GetViewWindow(), &drop.ptScreen);
    }

    switch (message)
    {
        case WM_DRAGSELECT:

            //
            // WM_DRAGSELECT is sent to a sink whenever an new object is dragged inside of it.
            // wParam: TRUE if the sink is being entered, FALSE if it's being exited.
            //
            if (wParam)
            {
                if (NULL != m_pdtobjHdrop)
                {
                    //
                    // can happen if old target fails to generate drag leave properly
                    //
                    OldDAD_DropTargetLeaveAndReleaseData();
                }

                if (SUCCEEDED(CIDLData_CreateFromIDArray(NULL, 0, NULL, &m_pdtobjHdrop)))
                {
                    //
                    // promise the CF_HDROP by setting a NULL handle
                    // indicating that this dataobject will have an hdrop at Drop() time
                    //
                    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
                    STGMEDIUM medium;

                    medium.tymed          = TYMED_HGLOBAL;
                    medium.hGlobal        = (HGLOBAL)NULL;
                    medium.pUnkForRelease = NULL;

                    m_pdtobjHdrop->SetData(&fmte, &medium, TRUE);

                    DWORD dwMouseKey = MK_LBUTTON;
                    if (GetAsyncKeyState(VK_SHIFT) & 0x80000000)
                        dwMouseKey |= MK_SHIFT;

                    DragEnter(m_pdtobjHdrop, dwMouseKey, drop.ptlScreen, &dwAllowedDADEffect);
                    m_dwOldDADEffect = dwAllowedDADEffect;
                }
            }
            else
            {
                OldDAD_DropTargetLeaveAndReleaseData();
            }
            break;

        case WM_DRAGMOVE:
            //
            // WM_DRAGMOVE is sent to a sink as the object is being dragged within it.
            // wParam: Unused
            //
            if (NULL != m_pdtobjHdrop)
            {
                DWORD dwMouseKey = MK_LBUTTON;
                if (GetAsyncKeyState(VK_SHIFT) & 0x80000000)
                    dwMouseKey |= MK_SHIFT;

                DragOver(dwMouseKey, drop.ptlScreen, &dwAllowedDADEffect);
                m_dwOldDADEffect = dwAllowedDADEffect;
            }
            break;

        case WM_QUERYDROPOBJECT:

            switch (lpds->wFmt)
            {
                case DOF_SHELLDATA:
                case DOF_DIRECTORY:
                case DOF_DOCUMENT:
                case DOF_MULTIPLE:
                case DOF_EXECUTABLE:
                    //
                    // assume all targets can accept HDROP if we don't have the data object yet
                    // we will accept the drop
                    //
                    return TRUE;
            }
            return FALSE;           // don't accept

        case WM_DROPOBJECT:
            if (NULL == m_pdtobjHdrop)
                return FALSE;

            //
            // Check the format of dragged object.
            //
            switch (lpds->wFmt)
            {
                case DOF_EXECUTABLE:
                case DOF_DOCUMENT:
                case DOF_DIRECTORY:
                case DOF_MULTIPLE:
                case DOF_PROGMAN:
                case DOF_SHELLDATA:
                    //
                    // We need to unlock this window if this drag&drop is originated
                    // from the shell itself.
                    //
                    DAD_DragLeave();

                    //
                    // The source is Win 3.1 app (probably FileMan), request HDROP.
                    // Send us a WM_DROPFILES with HDROP
                    //
                    return DO_DROPFILE;
            }
            break;

        case WM_DROPFILES:
            OnDropFiles((HDROP)wParam, m_dwOldDADEffect);
            break;
    }

    return 0;   // Unknown format. Don't drop any
}


void FocusOnSomething(HWND hwndLV)
{
    INT iFocus;

    iFocus = ListView_GetNextItem(hwndLV, -1, LVNI_FOCUSED);
    if (-1 == iFocus)
        iFocus = 0;

    ListView_SetItemState(hwndLV, iFocus, LVIS_FOCUSED, LVIS_FOCUSED);
}


LRESULT CFontView::ProcessMessage( HWND hWnd,
                                   UINT message,
                                   WPARAM wParam,
                                   LPARAM lParam )
{
    DEBUGMSG( (DM_MESSAGE_TRACE2, TEXT( "FONTEXT: CFontView::ProcMess Called m=0x%x wp=0x%x lp=0x%x" ),
              message, wParam, lParam ) );

    switch( message )
    {

    //
    // Desktop properties applet sends a WM_WININICHANGE when
    // the icon font is changed.  Just send the message on to the list view control.
    // This causes the list view to update using the new font.
    //
#ifdef WINNT
    case WM_WININICHANGE:
#else
    case WM_SETTINGCHANGE:
#endif
        //
        // Only handle this if the "section" is not "fonts".
        // I know this sounds weird but this is to prevent a double update
        // because vCPWinIniFontChange sends a WM_WININICHANGE and a
        // WM_FONTCHANGE whenever fonts are added or deleted to/from the folder.
        // In the WM_WININICHANGE, it sets the section string to "fonts".  We
        // use this to determine that the message came from vCPWinIniFontChange.
        //
        if (0 != lstrcmp((LPCTSTR)lParam, TEXT("fonts")))
        {
            SendMessage( m_hwndList, message, wParam, lParam );

            if (0 == lstrcmp((LPCTSTR)lParam, TEXT("intl")))
            {
                if (IDM_VIEW_DETAILS == m_idViewMode)
                {
                    //
                    // We're in "details" mode and the user might have just
                    // changed the date format.  Update the listview.  This is
                    // how the shell's defview handles this.
                    //
                    InvalidateRect(m_hwndList, NULL, TRUE);
                }
            }            
        }
        
        //
        // When changing from single to double-click mode, the shell broadcasts a WM_SETTINGCHANGE.
        //
        SetListviewClickMode();
        {
            //
            // If we received WM_SETTINGCHANGE because the user changed listview click mode,
            // we want to update the "View" menu to either add or remove the "Preview"
            // menu item.  Deactivating the view and restoring it to it's previous mode
            // does the trick.
            //
            UINT uState = m_uState;
            OnDeactivate();
            OnActivate(uState);
        }
        return 0;

    case WM_FONTCHANGE:
        //
        //  Make sure the FontManager gets refreshed before we draw the
        //  window.
        //
        {
            WaitCursor oWait;
            m_poFontManager->bRefresh( TRUE );
        }
        Refresh( );
        break;


    case WM_INITMENUPOPUP:
        UpdateMenuItems( m_hmenuCur );
        break;

    //
    // Handle drops from Win3.1 (File Manager) apps the same as the shell
    // does in defviewx.c (with a few font folder-specific mods).
    //
    case WM_DROPOBJECT:
    case WM_QUERYDROPOBJECT:
    case WM_DRAGLOOP:
    case WM_DRAGSELECT:
    case WM_DRAGMOVE:
    case WM_DROPFILES:
        return OldDAD_HandleMessages(message, wParam, (const DROPSTRUCT *)lParam);

    case WM_DESTROY:
        {
        //
        //  Remove ourself from the Clipboard chain.
        //

        ChangeClipboardChain( hWnd, m_hwndNextClip );

        //
        // Call CFontClass::Release() for each font contained in the ListView.
        //
        ReleaseFontObjects();

        return( 1 );
        }


    case WM_CHANGECBCHAIN:
        //
        //  If the next window is closing, repair the chain.
        //

        if( (HWND) wParam == m_hwndNextClip )
            m_hwndNextClip = (HWND) lParam;

        //
        //  Otherwise, pass the message to the next link.
        //

        else if( m_hwndNextClip != NULL )
            SendMessage( m_hwndNextClip, message, wParam, lParam );

        break;

    case WM_DRAWCLIPBOARD:
        //
        //  Notify the next viewer in the chain.
        //

        if( m_hwndNextClip )
            SendMessage( m_hwndNextClip, message, wParam, lParam );

        //
        //  Unhook ourself from the clipboard chain.
        //

        ChangeClipboardChain( hWnd, m_hwndNextClip );
        m_hwndNextClip = NULL;

        break;


    case WM_CONTEXTMENU:
    {
        UINT  nMenu;

        nMenu = (iCurrentSelection( ) > 0 ) ? IDM_POPUPS : IDM_POPUP_NOITEM;

        //
        //  Pop up the context menu.
        //

        HMENU hMenu = LoadMenu( g_hInst, MAKEINTRESOURCE( nMenu ) );

        if( hMenu )
        {
            HMENU hPopup = GetSubMenu( hMenu, 0 );

            if(  hPopup )
            {
                //
                //  Bold the Open menuitem.
                //

                if( nMenu == IDM_POPUPS )
                {
                    MENUITEMINFO iInfo;

                    iInfo.cbSize = sizeof( iInfo );
                    iInfo.fMask = MIIM_STATE;

                    if( GetMenuItemInfo( hMenu, IDM_FILE_SAMPLE, FALSE, &iInfo ) )
                    {
                       iInfo.fState |= MFS_DEFAULT;
                       SetMenuItemInfo( hMenu, IDM_FILE_SAMPLE, FALSE , &iInfo );
                    }
                }
                else
                {
                    UINT nFlag = ( (IsClipboardFormatAvailable( CF_HDROP ) )
                                                    ? cLiveMenu : cDeadMenu );

                    EnableMenuItem( hMenu, IDM_EDIT_PASTE, nFlag );

                    CheckMenuRadioItem(hMenu,
                                       IDM_VIEW_ICON,
                                       IDM_VIEW_DETAILS,
                                       m_idViewMode,
                                       MF_BYCOMMAND);
                }

                DWORD    dwPoint = (DWORD)lParam;

                //
                //  now get the popup positiong:
                //

                if( dwPoint == (DWORD) -1 )
                {
                    POINT pt;
                    int iItem;

                    //
                    //  keyboard...
                    //  Find the selected item
                    //

                    iItem = ListView_GetNextItem( m_hwndList, -1, LVNI_SELECTED );

                    if( iItem != -1 )
                    {
                        RECT rc;
                        int iItemFocus = ListView_GetNextItem( m_hwndList,
                                            -1, LVNI_FOCUSED | LVNI_SELECTED );

                        if( iItemFocus == -1 )
                            iItemFocus = iItem;

                        //
                        //  Note that LV_GetItemRect returns it in client
                        //  coordinate!
                        //

                        ListView_GetItemRect( m_hwndList, iItemFocus,
                                              &rc, LVIR_ICON );

                        pt.x = ( rc.left+rc.right ) / 2;
                        pt.y = ( rc.top+rc.bottom ) / 2;
                    }
                    else
                    {
                        pt.x = pt.y = 0;
                    }

                    MapWindowPoints( m_hwndList, HWND_DESKTOP, &pt, 1 );

                    dwPoint = MAKELONG( pt.x, pt.y );
                }


                TrackPopupMenuEx( hPopup,
                            TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                            GET_X_LPARAM( dwPoint ),  // x pos
                            GET_Y_LPARAM( dwPoint ),  // y pos.
                            m_hwndView,
                            NULL );
            }
            DestroyMenu( hMenu );
        }
    }
    break;

    case WM_COMMAND:
       if( !(GET_WM_COMMAND_ID( wParam, lParam ) > FCIDM_SHVIEWLAST ) )
            return( OnCommand( hWnd, message, wParam, lParam ) );
       break;

    case WM_MENUSELECT:
        return( OnMenuSelect( hWnd, (UINT)LOWORD( wParam ),
                (UINT)HIWORD( wParam ), (HMENU)lParam ) );

#ifdef USE_OWNERDRAW
    case WM_MEASUREITEM:
        SetDimensions( );
        return( OnMeasureItem( (LPMEASUREITEMSTRUCT)lParam ) );

    case WM_DRAWITEM:
        return( OnDrawItem( (LPDRAWITEMSTRUCT)lParam ) );
#endif

    case WM_SETFOCUS:
        if( !m_hwndView )
            //
            //  Ignore if we are destroying hwndView.
            //
            break;

        if( m_hwndList )
        {
            //
            //  pass this to the listview
            //
            SetFocus( m_hwndList );
            UpdateSelectedCount();
            FocusOnSomething(m_hwndList);
        }

        break;

    case WM_SIZE:
        {
        DEBUGMSG( (DM_MESSAGE_TRACE1, TEXT( "FONTEXT: CFontView::ProcessMessage WM_SIZE" ) ) );

        if( wParam != SIZEICONIC )
        {
            //
            // Resize the view.
            // Set "resizing" flag so we know to intercept WM_ERASEBKGND.
            // Helps eliminate flashing of the window during resize.
            //
            m_bResizing = TRUE;
            vShapeView( );
            m_bResizing = FALSE;
        }

        return( 1 );

        }


    case WM_ERASEBKGND:
        if (m_bResizing)
        {
            //
            // Resizing the window.
            // Eat WM_ERASEBKGND so the dialog surface behind the listview
            // isn't repainted.  This helps eliminate flashing of the window during
            // resizing.
            //
            return 1;
        }
        else
        {
            return DefDlgProc(hWnd, message, wParam, lParam);
        }

    case WM_NOTIFY:
        return OnNotify( (LPNMHDR) lParam );

    case WM_SHELL_CHANGE_NOTIFY:
        return OnShellChangeNotify( wParam, lParam );

    default:
        return( DefDlgProc( hWnd, message, wParam, lParam ) );
    } // switch( message )

    return( 0 );
}


BOOL CFontView::OpenCurrent( )
{
    LV_ITEM  Item;

    //
    //  Start at the beginning.
    //

    int  i = -1;
    UINT nFlags = LVNI_SELECTED | LVNI_ALL;


    Item.mask = LVIF_PARAM;

    GetAsyncKeyState( VK_ESCAPE );

    while( ( i = ListView_GetNextItem( m_hwndList, i, nFlags ) ) != -1 )
    {
        if( GetAsyncKeyState( VK_ESCAPE ) & 0x01 )
        {
            break;
        }

        Item.iItem = i;
        Item.iSubItem = 0;

        if( ListView_GetItem( m_hwndList, &Item ) )
        {
            CFontClass * poFont = (CFontClass *)Item.lParam;

            if( !ViewValue( poFont ) )
                return FALSE;
        }
    }
    return TRUE;;
}


BOOL CFontView::PrintCurrent( )
{
    LV_ITEM  Item;
    CFontClass * poFont;

    //
    //  Start at the beginning.
    //

    int i = -1;
    UINT  nFlags = LVNI_SELECTED | LVNI_ALL;


    Item.mask = LVIF_PARAM;

    GetAsyncKeyState( VK_ESCAPE );

    while( ( i = ListView_GetNextItem( m_hwndList, i, nFlags ) ) != -1 )
    {
        if( GetAsyncKeyState( VK_ESCAPE ) & 0x01 )
        {
            break;
        }

        Item.iItem = i;
        Item.iSubItem = 0;

        if( ListView_GetItem( m_hwndList, &Item ) )
        {
            poFont = (CFontClass *)Item.lParam;

            if( !PrintValue( poFont ) )
                return FALSE;
        }
    }

    return TRUE;
}

BOOL CFontView::ViewValue( CFontClass * poFont )
{
    FullPathName_t szFile;
    HINSTANCE      hInst;


    poFont->vGetDirFN( szFile );

    DEBUGMSG( (DM_TRACE1, TEXT( "   Browsing object: %s" ), szFile ) );

    hInst = ShellExecute( m_hwndView, NULL, szFile, szFile, NULL, 1 );

    if( ( (INT_PTR)hInst ) > 32 )
        return TRUE;

    DEBUGMSG( (DM_ERROR, TEXT( "ViewValue failed on %s, %d" ), szFile, hInst) );

    return FALSE;
}


BOOL CFontView::PrintValue( CFontClass * poFont )
{
    FullPathName_t szFile;
    HINSTANCE      hInst;


    poFont->vGetDirFN( szFile );

    DEBUGMSG( (DM_TRACE1, TEXT( "   Browsing object: %s" ), szFile) );

    hInst = ShellExecute( m_hwndView, TEXT( "print" ), szFile, szFile, NULL, 1 );

    if( ( (INT_PTR)hInst ) > 32 )
        return TRUE;

    DEBUGMSG( ( DM_ERROR, TEXT( "ViewValue failed on %s, %d" ), szFile, hInst ) );

    return FALSE;
}


void CFontView::UpdatePanColumn( )
{

    LV_COLUMN  lvc;
    TCHAR      szFmt[ MAX_NAME_LEN ];
    TCHAR      szText[ MAX_NAME_LEN ];


    if( m_idViewMode != IDM_VIEW_PANOSE )
        return;

    if( LoadString( g_hInst, IDS_PAN_COL2, szFmt, ARRAYSIZE( szFmt ) ) )
    {
        TCHAR szName[ MAX_NAME_LEN ];

        if( m_poPanose )
            m_poPanose->vGetName( szName );
        else
            szName[ 0 ] = 0;

        wsprintf( szText, szFmt, szName );
    }
    else
        szText[ 0 ] = 0;

    lvc.mask     = LVCF_TEXT | LVCF_SUBITEM;
    lvc.pszText  = szText;
    lvc.iSubItem = 1;

    ListView_SetColumn( m_hwndList, 1, &lvc );
}


void CFontView::vToggleSelection( BOOL bSelectAll )
{
    //
    //  Start at the beginning.
    //

    int i = -1;

    UINT  nFlags = LVNI_ALL;
    UINT  nState = LVIS_SELECTED;


    while( ( i = ListView_GetNextItem( m_hwndList, i, nFlags ) ) != -1 )
    {
        if( !bSelectAll )
        {
            nState = ListView_GetItemState( m_hwndList, i, LVIS_SELECTED );
            nState = nState ^ LVIS_SELECTED;
        }

        ListView_SetItemState( m_hwndList, i , nState, LVIS_SELECTED );
    }
}


int  CFontView::iCurrentSelection( )
{
    return (int) ListView_GetSelectedCount( m_hwndList );
}


void CFontView::SetViewMode( UINT uMode )
{
    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: SetViewMode %d" ), uMode) );

    if( uMode != m_idViewMode )
    {
        UINT  ws = WSFromViewMode( uMode, m_hwndList );

        m_idViewMode = uMode;

        switch( uMode )
        {
        default: // case IDM_VIEW_ICON:
            break;


        case IDM_VIEW_PANOSE:
            //
            //  Make sure the combo box is loaded with the names.
            //
            vLoadCombo( );

            SetListColumns( m_hwndList, PAN_COL_COUNT, PanoseColumns );

            //
            //  The second column contains format text that we can put the
            //  current font name into.
            //

            UpdatePanColumn( );

            break;


         case IDM_VIEW_DETAILS:
            SetListColumns( m_hwndList, FILE_COL_COUNT, FileColumns );
            break;
        }

        ULONG ulOld = SetWindowLong( m_hwndList, GWL_STYLE, ws );

        DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: SetviewMode from %x to %x" ), ulOld, ws) );

        vShapeView( );

        SortObjects( );

        InvalidateRect( m_hwndList, NULL, TRUE );

        UpdateWindow( m_hwndList );

        UpdateToolbar( );
    }
}


void CFontView::vShapeView( )
{
    RECT  rc;
    BOOL  bPanoseView = m_idViewMode == IDM_VIEW_PANOSE;
    int   nCmdShow    = bPanoseView ? SW_SHOW : SW_HIDE;

    //
    // Hide or show panose controls depending on view mode.
    //
    ShowWindow( m_hwndCombo, nCmdShow );
    ShowWindow( m_hwndText, nCmdShow );

    //
    // Get size of dialog (view) so we can size the listview control.
    //
    GetClientRect( m_hwndView, &rc );

    if (bPanoseView)
    {
        //
        // Panose view adds the "List by similarity" combo box
        // and text.  Adjust the top of the listview control to be
        // adjacent to the bottom of the combo box area.
        //
        RECT  rCombo;
        GetWindowRect( m_hwndCombo, &rCombo );
        ScreenToClient( m_hwndView, (LPPOINT)&rCombo );
        ScreenToClient( m_hwndView, ((LPPOINT)&rCombo) + 1 );
        rc.top = rCombo.bottom + 6;
    }

    //
    // Resize the list view control.
    //
    MoveWindow( m_hwndList, rc.left, rc.top, rc.right-rc.left,
                rc.bottom-rc.top, TRUE );
}


void CFontView::vLoadCombo( )
{
    int            iAdd;
    FONTNAME     szName;
    FONTNAME     szSelName;
    CFontClass*    lpFontRec;
    CFontList * poFontList = m_poFontManager->poLockFontList( );
    int            iCount = poFontList->iCount( );
    int            iOldSel;
    ComboITEMDATA cData;

    //
    //  Reset the Panose origin.
    //

    m_poPanose = NULL;

#ifdef _DEBUG
    szName[ ARRAYSIZE( szName ) - 1 ] = 0;
    szSelName[ ARRAYSIZE( szSelName ) - 1 ] = 0;
#endif

    iOldSel = (int)::SendMessage( m_hwndCombo, CB_GETCURSEL, 0, 0 );

    if( iOldSel != CB_ERR )
        ::SendMessage( m_hwndCombo, CB_GETLBTEXT, iOldSel, (LPARAM)szSelName );

        ::SendMessage( m_hwndCombo, CB_RESETCONTENT, 0, 0 );

    for( int i = 0; i < iCount; ++i )
    {
        lpFontRec = poFontList->poObjectAt( i );

        if( lpFontRec->bLTDAndPANOSE( ) )
        {
            lpFontRec->vGetName( szName );
            iAdd = (int)::SendMessage( m_hwndCombo, CB_ADDSTRING, 0,
                                       (LPARAM)szName );

            cData.poFont = lpFontRec;

            ::SendMessage( m_hwndCombo, CB_SETITEMDATA, iAdd,
                           (LPARAM)cData.ItemData );
        }
    }

    if( iOldSel == CB_ERR )
    {
        iOldSel = 0;
        ::SendMessage( m_hwndCombo, CB_GETLBTEXT, iOldSel, (LPARAM)szSelName );
    }

    if( iOldSel != CB_ERR )
    {
        i = (int)::SendMessage( m_hwndCombo, CB_FINDSTRINGEXACT,
                           (WPARAM) -1, (LPARAM) szSelName );

        if( i != CB_ERR )
        {
            m_poPanose = (CFontClass *)::SendMessage( m_hwndCombo,
                                                      CB_GETITEMDATA, i, 0 );
        }
        ::SendMessage( m_hwndCombo, CB_SETCURSEL, i, 0 );
    }

    m_poFontManager->vReleaseFontList( );

    ASSERT( szName[ ARRAYSIZE( szName ) - 1 ] == 0 );
    ASSERT( szSelName[ ARRAYSIZE( szSelName ) - 1 ] == 0 );
}


STDMETHODIMP CFontView::QueryInterface( REFIID riid, LPVOID FAR* ppvObj )
{
    *ppvObj = NULL;

    DEBUGMSG( (DM_NOEOL | DM_TRACE1,
             TEXT( "FONTEXT: CFontView::QueryInterface called for " ) ) );
    //
    //  Dump out the riid
    //

    DEBUGREFIID( (DM_TRACE1, riid) );

    if( riid == IID_IUnknown )
       *ppvObj = (IUnknown*)( IShellView*)this;

    if( riid == IID_IShellView )
       *ppvObj = (IShellView*) this;

    if( riid == IID_IDropTarget )
       *ppvObj = (IDropTarget*) this;

    if( riid == IID_IPersistFolder )
       *ppvObj = (IPersistFolder*) this;

    if( *ppvObj )
    {
       ((LPUNKNOWN)*ppvObj)->AddRef( );
       return NOERROR;
    }

    return( ResultFromScode( E_NOINTERFACE ) );
}


STDMETHODIMP_(ULONG) CFontView::AddRef( void )
{
   DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: CFontView::AddRef called: %d->%d references" ),
              m_cRef, m_cRef + 1) );

   return( ++m_cRef );
}


STDMETHODIMP_(ULONG) CFontView::Release( void )
{
    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: CFontView::Release called: %d->%d references" ),
             m_cRef, m_cRef - 1) );

    ULONG retval;


    retval = --m_cRef;

    if( !retval )
    {
        DestroyViewWindow( );
        delete this;
    }

    return( retval );
}


//
// *** IOleWindow methods ***
//

STDMETHODIMP CFontView::GetWindow( HWND FAR * lphwnd )
{
    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: CFontView::GetWindow called" ) ) );

    *lphwnd = m_hwndView;
    return S_OK;
}


STDMETHODIMP CFontView::ContextSensitiveHelp( BOOL fEnterMode )
{
    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: CFontView::ContextSensitiveHelp called" ) ) );

    return( ResultFromScode( E_NOTIMPL ) );
}


//
// *** IShellView methods ***
//
STDMETHODIMP CFontView::TranslateAccelerator( LPMSG msg )
{
    DEBUGMSG( (DM_MESSAGE_TRACE2, TEXT( "FONTEXT: CFontView::TranslateAccelerator called" ) ) );
    DEBUGMSG( (DM_MESSAGE_TRACE2,
       TEXT( "FONTEXT:        hwnd=0x%x, message=0x%x, wParam=0x%x, lParam=0x%lx," ),
       msg->hwnd, msg->message, msg->wParam, msg->lParam) );
    DEBUGMSG( (DM_MESSAGE_TRACE2, TEXT( "FONTEXT:         time=0x%x, pt=?" ),
             msg->time) );


    BOOL fIsVK_TABCycler = IsVK_TABCycler(msg);
    BOOL fIsBackSpace = IsBackSpace(msg);

    if (GetFocus() == m_hwndList)
    {
        if (::TranslateAccelerator(m_hwndView, (HACCEL) m_hAccel, msg))
        {
            // we know we have a normal view, therefore this is
            // the right translate accelerator to use, otherwise the
            // common dialogs will fail to get any accelerated keys.
            return S_OK;
        }
        else if (WM_KEYDOWN == msg->message || WM_SYSKEYDOWN == msg->message)
        {
            // MSHTML eats these keys for frameset scrolling, but we
            // want to get them to our wndproc . . . translate 'em ourself
            //
            switch (msg->wParam)
            {
            case VK_LEFT:
            case VK_RIGHT:
                // only go through here if alt is not down.
                // don't intercept all alt combinations because
                // alt-enter means something (open up property sheet)
                // this is for alt-left/right compat with IE
                // remark: when debugging, if put break point at if statement, function will not work.
                if (GetAsyncKeyState(VK_MENU) < 0)
                    break;
                // fall through
                
            case VK_UP:
            case VK_DOWN:
            case VK_HOME:
            case VK_END:
            case VK_PRIOR:
            case VK_NEXT:
            case VK_RETURN:
            case VK_F10:
                TranslateMessage(msg);
                DispatchMessage(msg);
                return S_OK;
            }
        }

        // suwatch: If this message is neither VK_TABCycler nor Backspace, try send it to ShellBrowser first.
        if (!fIsVK_TABCycler && !fIsBackSpace)
        {
            if (S_OK == m_psb->TranslateAcceleratorSB(msg, 0))
                return S_OK;
        }
    }

    // suwatch: Backspace and all Alt key combination will be handled by default.
    // Backspace will navigate up the tree.
    // We assume all Alt key combo will be handled by default.
    if (fIsBackSpace || GetKeyState(VK_MENU) < 0)
    {
        return S_FALSE;
    }

    //
    //  If the view has the combo box showing, make sure it gets processed
    //  correctly.
    //
    if( m_idViewMode == IDM_VIEW_PANOSE )
    {
        if( msg->message == WM_KEYFIRST || msg->message == WM_KEYDOWN )
        {

            TCHAR ch = GET_WM_CHARTOITEM_CHAR( msg->wParam, msg->lParam );

            if( ch == VK_TAB && GetFocus( ) == m_hwndCombo )
            {
                return S_FALSE;
            }
        }

        //
        // Handle accelerator translations for Panose view mode.
        //
        if( m_hAccel &&  ::TranslateAccelerator( m_hwndView, (HACCEL) m_hAccel, msg ) )
        {
            return S_OK;
        }

        //
        //  This will handle Alt+L and TAB for all other cases.
        //
        if( IsDialogMessage( m_hwndView, msg ) )
        {
            return S_OK;
        }
    }
    //
    //  If the combo box isn't visible, the processing is easier.
    //
    else if( GetFocus( ) != m_hwndList )
    {
        if( IsDialogMessage( m_hwndView, msg ) )
            return S_OK;
    }

    if( m_hAccel &&  ::TranslateAccelerator( m_hwndView, (HACCEL) m_hAccel, msg ) )
        return S_OK;



        
    return S_FALSE;
}


STDMETHODIMP CFontView::EnableModeless( BOOL fEnable )
{
    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: CFontView::EnableModless called" ) ) );
    return( ResultFromScode( E_NOTIMPL ) );
}


STDMETHODIMP CFontView::UIActivate( UINT uState )
{
    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: CFontView::UIActivate called, uState = %d" ), uState) );

    if( uState != SVUIA_DEACTIVATE )
    {
        OnActivate( uState );
        m_bUIActivated = TRUE;
    }
    else
        OnDeactivate( );

    return NOERROR;
}


STDMETHODIMP CFontView::Refresh( )
{
    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: CFontView::Refresh start" ) ) );

    FillObjects( );

    if( m_idViewMode == IDM_VIEW_PANOSE )
        vLoadCombo( );

    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: CFontView::Refresh Done" ) ) );

    return( NOERROR );
}


STDMETHODIMP CFontView::CreateViewWindow( IShellView FAR* lpPrevView,
                                          LPCFOLDERSETTINGS lpfs,
                                          IShellBrowser FAR* psb,
                                          RECT FAR* prcView,
                                          HWND FAR* phWnd )
{
    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: CFontView::CreateViewWindow called" ) ) );

    //
    //  Should check lpPrevView for validity
    //

    if( !RegisterWindowClass( ) )
    {
        DEBUGMSG( (DM_ERROR, TEXT( "FONTEXT: CFontView - Unable to register window class" ) ) );

        // return( ResultFromScode( E_FAIL ) );
    }

    if (FAILED(GetFontManager(&m_poFontManager)))
    {
        return E_FAIL;
    }

    //
    //  Save off the browser and retrieve our settings before we draw
    //  the window.
    //

    m_psb = psb;

    GetSavedViewState( );

    psb->GetWindow( &m_hwndParent );

    //
    // Load the attribute character string.
    //
    if (TEXT('\0') == g_szAttributeChars[0])
    {
        LoadString(g_hInst,
                   IDS_ATTRIB_CHARS,
                   g_szAttributeChars,
                   ARRAYSIZE(g_szAttributeChars));
    }

#ifdef ALT_TEXT_COLOR
    //
    // Fetch the alternate color (for compression) if supplied.
    //
    {
        //
        // If not open, open key to explorer settings.
        //
        if (NULL == g_hkcuExplorer)
            RegOpenKeyEx(HKEY_CURRENT_USER,
                         REGSTR_PATH_EXPLORER,
                         0,
                         KEY_READ,
                         &g_hkcuExplorer);

        if (NULL != g_hkcuExplorer)
        {
            DWORD cbData = sizeof(COLORREF);
            DWORD dwType;
            RegQueryValueEx(g_hkcuExplorer,
                            c_szAltColor,
                            NULL,
                            &dwType,
                            (LPBYTE)&g_crAltColor,
                            &cbData);
        }
    }
#endif // ALT_TEXT_COLOR

    //
    //  Set the view mode. Never inherit the Panose view. Force the user
    //  to set it.
    //

    if( lpfs )
    {
        switch( lpfs->ViewMode )
        {
        default:
        case FVM_ICON:
            m_idViewMode = IDM_VIEW_ICON;
            break;

        case FVM_SMALLICON:
        case FVM_LIST:
            m_idViewMode = IDM_VIEW_LIST;
            break;

        case FVM_DETAILS:
            m_idViewMode = IDM_VIEW_DETAILS;
            break;
        }

        //
        //  don't save this if ViewMode is 0 (default)
        //

        if( lpfs->ViewMode )
            m_ViewModeReturn = lpfs->ViewMode;

        m_fFolderFlags = lpfs->fFlags;
    }

    DEBUGMSG( (DM_TRACE2, TEXT( "FONTEXT: CFontView::CVW view window rect=(%d,%d,%d,%d)" ),
             prcView->left, prcView->top, prcView->right, prcView->bottom) );

    if( !(m_hwndView = CreateDialogParam( g_hInst,
                                          MAKEINTRESOURCE( ID_DLG_MAIN ),
                                          m_hwndParent,
                                          FontViewDlgProc,
                                          (LPARAM) this ) ) )

    {
        DEBUGMSG( (DM_ERROR, TEXT( "FONTEXT: CFontView::CVW CreateWindow failed" ) ) );
        return( ResultFromScode( E_FAIL ) );
    }

    DragAcceptFiles( m_hwndView, TRUE );

    SHRegisterDragDrop( m_hwndView, (LPDROPTARGET)this );

    *phWnd = m_hwndView;

    //
    //  If using a dialog, we need to resize correctly.
    //

    MoveWindow( m_hwndView,
                prcView->left, prcView->top,
                prcView->right - prcView->left,
                prcView->bottom - prcView->top,
                TRUE );

    //
    // Re-read the registry in case new fonts were added while we were away.
    // If we don't do this, any fonts (without files in the fonts directory)
    // added to the registry aren't displayed in the folder list view.
    //
    m_poFontManager->bRefresh( TRUE );

    FillObjects( );

    // SortObjects( );


    ShowWindow( m_hwndView, SW_SHOW );

    // UpdateWindow( m_hwndView );

    //
    //  The BrowserWindow will invalidate us and force a redraw, so
    //  don't do it now.
    //
    //  ValidateRect( m_hwndView, NULL );

    MergeToolbar( );
    UpdateToolbar( );

    return( NOERROR );
}


STDMETHODIMP CFontView::DestroyViewWindow( )
{
    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: CFontView::DestroyViewWindow called" ) ) );

    if( m_hwndView )
    {
        if (0 != m_uSHChangeNotifyID)
            SHChangeNotifyDeregister(m_uSHChangeNotifyID);

        DragAcceptFiles( m_hwndView, FALSE );

        SHRevokeDragDrop( m_hwndView );

        DestroyWindow( m_hwndView );

#ifdef ALT_TEXT_COLOR

        if (NULL != g_hkcuExplorer)
        {
            RegCloseKey(g_hkcuExplorer);
            g_hkcuExplorer = NULL;
        }

#endif // ALT_TEXT_COLOR

        m_hwndView = NULL;
    }

#ifdef TOOLTIP_FONTSAMPLE

    if (NULL != m_hwndToolTip)
    {
        DestroyWindow(m_hwndToolTip);
        m_hwndToolTip = NULL;
    }
#endif // TOOLTIP_FONTSAMPLE

    return( NOERROR );
}



#ifdef TOOLTIP_FONTSAMPLE


VOID
CFontView::LV_OnHoverNotify(
    LPNMLISTVIEW pnmlv
    )
{
    INT iHit = pnmlv->iItem;

    if (m_bShowPreviewToolTip)
    {
        if (iHit != m_iTTLastHit)
        {
            if (-1 == iHit)
            {
                //
                // Hit nowhere.  Deactivate the tooltip.
                //
                SendMessage(m_hwndToolTip, TTM_ACTIVATE, FALSE, 0);
            }
            else
            {
                //
                // Hit on an item.  Set the tooltip font, text and activate
                // the tooltip.  The tooltip is deactivated while the new
                // font and text are loaded so we don't see an incomplete
                // sample.
                //
                SendMessage(m_hwndToolTip, TTM_ACTIVATE, FALSE, 0);
                UpdateFontSample(iHit);
                if (NULL != m_hfontSample)
                {
                    //
                    // If we were able to load the font and create a sample
                    // for display, display the sample in the tooltip window.
                    //
                    SendMessage(m_hwndToolTip, WM_SETFONT, (WPARAM)m_hfontSample, (LPARAM)FALSE);
                    SendMessage(m_hwndToolTip, TTM_ACTIVATE, TRUE, 0);
                }
            }
            //
            // Need to fake out the tooltip window into thinking that the mouse
            // is over a new tool.  Since we have only one tooltip window,
            // the entire listview object is considered a single tool.  If we don't
            // send this message, the tooltip thinks the mouse is always over
            // the "tool" so we never get a new tooltip when we navigate between
            // items in the listview.
            //
            SendMessage(m_hwndToolTip, WM_MOUSEMOVE, 0, 0);
            m_iTTLastHit = iHit;
        }
    }
}


//
// Update the sample font and text to be displayed in the tooltip window
// for a given item in the listview.
//
VOID
CFontView::UpdateFontSample(
    INT iItem
    )
{
    LV_ITEM item;

    if (NULL != m_hfontSample)
    {
        //
        // Delete a previous font.
        //
        DeleteObject(m_hfontSample);
        m_hfontSample = NULL;
    }

    ZeroMemory(&item, sizeof(item));

    item.iItem    = iItem;
    item.iSubItem = 0;
    item.mask     = LVIF_PARAM;

    //
    // Get the current listview item.
    //
    if (ListView_GetItem(m_hwndList, &item))
    {
        CFontClass *pFont = (CFontClass *)item.lParam;
        TCHAR szFontPath[(MAX_PATH * 2) + 1];           // "PathPFM|PathPFB"

        //
        // Get the font file's full path name.
        //
        if (pFont->bGetFQName(szFontPath, ARRAYSIZE(szFontPath)))
        {
            //
            // If font is a Type1 with an associated PFB,
            // create the "XXXXXX.PFM|XXXXXX.PFB" concatenation to give to
            // GDI.
            //
            LPTSTR pszFontPathPfb = (LPTSTR)szFontPath + lstrlen(szFontPath);

            if (pFont->bGetPFB(pszFontPathPfb + 1, ARRAYSIZE(szFontPath) - (UINT)(pszFontPathPfb - szFontPath) - 1))
            {
                *pszFontPathPfb = TEXT('|');
            }

            INT nFonts;
            DWORD cb = sizeof(nFonts);
            //
            // Get number of fonts in font file so we can size the LOGFONT buffer.
            //
            if (GetFontResourceInfoW(szFontPath, &cb, &nFonts, GFRI_NUMFONTS))
            {
                cb = sizeof(LOGFONT) * nFonts;
                LPLOGFONT plf = (LPLOGFONT)LocalAlloc(LPTR, cb);
                if (NULL != plf)
                {
                    //
                    // Read in the LOGFONT data for the font(s).
                    //
                    if (GetFontResourceInfoW((LPTSTR)szFontPath, &cb, plf, GFRI_LOGFONTS))
                    {
                        //
                        // Create the sample font.
                        //
                        HDC hdc = GetDC(m_hwndList);

                        plf->lfHeight = -MulDiv(FONT_SAMPLE_PT_SIZE,
                                                GetDeviceCaps(hdc, LOGPIXELSY),
                                                72);
                        plf->lfWidth = 0;

                        ReleaseDC(m_hwndList, hdc);

                        m_hfontSample = CreateFontIndirect(plf);

                        //
                        // Update the tip text.
                        // We display a different sample for symbol fonts.
                        //
                        TOOLINFO ti;
                        ti.cbSize      = sizeof(TOOLINFO);
                        ti.uFlags      = TTF_IDISHWND | TTF_SUBCLASS;
                        ti.hwnd        = m_hwndView;
                        ti.hinst       = g_hInst;
                        ti.uId         = (UINT_PTR)m_hwndList;
                        ti.lpszText    = SYMBOL_CHARSET == plf->lfCharSet ? m_pszSampleSymbols
                                                                          : m_pszSampleText;

                        SendMessage(m_hwndToolTip, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);
                    }
                    LocalFree(plf);
                }
            }
        }
    }
    //
    // Note:  m_hfontSample can be NULL on return if the font couldn't be loaded.
    //        In such cases, the caller should not display the sample tooltip.
    //
}


BOOL
CFontView::CreateToolTipWindow(
    VOID
    )
{
    BOOL bResult = FALSE;

    m_hwndToolTip = CreateWindowEx(0,
                                   TOOLTIPS_CLASS,
                                   (LPTSTR)NULL,
                                   0,
                                   CW_USEDEFAULT,
                                   CW_USEDEFAULT,
                                   CW_USEDEFAULT,
                                   CW_USEDEFAULT,
                                   m_hwndList,
                                   (HMENU)NULL,
                                   g_hInst,
                                   NULL);

    if (NULL != m_hwndToolTip)
    {
        TOOLINFO ti;

        //
        // Set tooltip timing parameter so that it pops up when the
        // item is hover selected.
        //
        SendMessage(m_hwndToolTip,
                    TTM_SETDELAYTIME,
                    TTDT_AUTOMATIC,
                    (LPARAM)ListView_GetHoverTime(m_hwndList));

        ti.cbSize      = sizeof(TOOLINFO);
        ti.uFlags      = TTF_IDISHWND | TTF_SUBCLASS;
        ti.hwnd        = m_hwndView;
        ti.hinst       = g_hInst;
        ti.uId         = (UINT_PTR)m_hwndList;
        ti.lpszText    = NULL;

        bResult = (BOOL)SendMessage(m_hwndToolTip,
                                    TTM_ADDTOOL,
                                    0,
                                    (LPARAM)&ti);

        //
        // Activate/Deactivate the tooltip based on the user's current
        // preference.
        //
        SendMessage(m_hwndToolTip, TTM_ACTIVATE, m_bShowPreviewToolTip, 0);
    }

    return bResult;
}

#endif // TOOLTIP_FONTSAMPLE



STDMETHODIMP CFontView::GetCurrentInfo( LPFOLDERSETTINGS lpfs )
{
    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: CFontView::GetCurrentInfo called" ) ) );

    // *lpfs = m_fs;

    if( lpfs )
    {
        lpfs->ViewMode = m_ViewModeReturn;
        lpfs->fFlags = m_fFolderFlags;
    }

    return( NOERROR );
}


static const TCHAR *c_szTTOnly = TEXT( "TTOnly" );

INT_PTR CALLBACK CFontView::OptionsDlgProc( HWND hDlg,
                                            UINT message,
                                            WPARAM wParam,
                                            LPARAM lParam )
{
    switch( message )
    {

    case WM_INITDIALOG:
      {
        int fWasSet = (GetProfileInt( c_szTrueType, c_szTTOnly, 0 ) ? 1 : 0 );

        CheckDlgButton( hDlg, IDC_TTONLY, fWasSet );

        SetWindowLongPtr( hDlg, DWLP_USER, 0 );

        break;
      }

    case WM_COMMAND:
      {
        UINT uID = GET_WM_COMMAND_ID( wParam, lParam );

        if( uID == IDC_TTONLY &&
            GET_WM_COMMAND_CMD( wParam, lParam ) == BN_CLICKED )
        {
            SendMessage( GetParent( hDlg ), PSM_CHANGED, (WPARAM) hDlg, 0L );
        }
        break;
      }

    case WM_NOTIFY:
        switch( ( (NMHDR*)lParam)->code )
        {
        case PSN_APPLY:
          {
            int fSet = IsDlgButtonChecked( hDlg, IDC_TTONLY ) ? 1 : 0;

            int fWasSet = (GetProfileInt( c_szTrueType, c_szTTOnly, 0 ) ? 1 : 0 );

            if( fSet != fWasSet )
            {
                WriteProfileString( c_szTrueType, c_szTTOnly,
                                    fSet ? TEXT( "1" ) : TEXT( "0" ) );
                SetWindowLongPtr( hDlg, DWLP_USER, TRUE );
            }
            break;
          }
        }

        SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 0 );
        break;

    case WM_DESTROY:
      {
        LONG_PTR fReboot = GetWindowLongPtr( hDlg, DWLP_USER );

        if( fReboot )
            RestartDialog( hDlg, NULL, EWX_REBOOT );

        break;
      }

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
                    HELP_WM_HELP, (DWORD_PTR)(LPTSTR) rgOptionPropPageHelpIDs);
        break;

    case WM_CONTEXTMENU:
        if (0 != GetDlgCtrlID((HWND)wParam))
        {
            WinHelp((HWND)wParam,
                    NULL,
                    HELP_CONTEXTMENU,
                    (DWORD_PTR)((LPTSTR)rgOptionPropPageHelpIDs));
        }
        break;
    }
    return 0;
}


STDMETHODIMP CFontView::AddPropertySheetPages( DWORD dwReserved,
                                               LPFNADDPROPSHEETPAGE lpfn,
                                               LPARAM lparam )
{
    HPROPSHEETPAGE hpage;
    PROPSHEETPAGE psp;

    psp.dwSize      = sizeof( psp );
    psp.dwFlags     = PSP_DEFAULT;
    psp.hInstance   = g_hInst;
    psp.pszTemplate = MAKEINTRESOURCE( ID_DLG_OPTIONS );
    psp.pfnDlgProc  = OptionsDlgProc;
    psp.lParam      = 0;

    hpage = CreatePropertySheetPage( &psp );

    if( hpage )
        lpfn( hpage, lparam );

    return( NOERROR );
}


STDMETHODIMP CFontView::GetSavedViewState( void )
{
    HRESULT  hr;
    LPSTREAM pstm;
    ULONG    ulLen = 0;
    ULONG    ulDataLen = sizeof(m_idViewMode);

    ULARGE_INTEGER libCurPos;
    ULARGE_INTEGER libEndPos;
    LARGE_INTEGER  dlibMove = {0, 0};

#ifdef TOOLTIP_FONTSAMPLE
    ulDataLen += sizeof(m_bShowPreviewToolTip);
#endif


    hr = m_psb->GetViewStateStream( STGM_READ, &pstm );

    if( FAILED( hr ) )
        goto backout0;

    pstm->Seek( dlibMove, STREAM_SEEK_CUR, &libCurPos );
    pstm->Seek( dlibMove, STREAM_SEEK_END, &libEndPos );

    ulLen = libEndPos.LowPart - libCurPos.LowPart;

    if(ulLen >= ulDataLen)
    {
        pstm->Seek( *(LARGE_INTEGER *)&libCurPos, STREAM_SEEK_SET, NULL );
        pstm->Read( &m_idViewMode, sizeof( m_idViewMode ), NULL );
#ifdef TOOLTIP_FONTSAMPLE
        pstm->Read( &m_bShowPreviewToolTip, sizeof(m_bShowPreviewToolTip), NULL);
#endif
    }

    pstm->Release( );

backout0:
    return hr;

}


STDMETHODIMP CFontView::SaveViewState( void )
{
    HRESULT  hr;
    LPSTREAM pstm;
    ULARGE_INTEGER libMove = {0,0};
    ULONG ulWrite;
    ULONG ulDataLen = sizeof(m_idViewMode);

    //
    //  Get a stream to work with
    //
    hr = m_psb->GetViewStateStream( STGM_WRITE, &pstm );

    if( FAILED( hr ) )
       goto backout0;

#ifdef TOOLTIP_FONTSAMPLE
    ulDataLen += sizeof(m_bShowPreviewToolTip);
#endif

    //
    //  The stream is at the begining of our data, I think. So write it in
    //  order.
    //
    hr = pstm->Write( &m_idViewMode, sizeof( m_idViewMode ), &ulWrite );
#ifdef TOOLTIP_FONTSAMPLE
    hr = pstm->Write( &m_bShowPreviewToolTip, sizeof(m_bShowPreviewToolTip), &ulWrite);
#endif
    if( FAILED( hr ) )
       goto backout1;

    libMove.LowPart = ulDataLen;

    pstm->SetSize( libMove );

    //
    //  Release the stream.
    //

backout1:

    pstm->Release( );

backout0:

    return hr;
}


STDMETHODIMP CFontView::SelectItem( LPCITEMIDLIST lpvID, UINT uFlags )
{
    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: CFontView::SelectItem called" ) ) );

    return( ResultFromScode( E_NOTIMPL ) );
}

STDMETHODIMP CFontView::GetItemObject( UINT uItem, REFIID riid, LPVOID *ppv )
{
    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: CFontView::SelectItem called" ) ) );

    return( ResultFromScode( E_NOTIMPL ) );
}


//------------------------------------------------------------------------
// IDropTarget Methods.
//
//

STDMETHODIMP CFontView::DragEnter( IDataObject __RPC_FAR *pDataObj,
                                   DWORD grfKeyState,
                                   POINTL pt,
                                   DWORD __RPC_FAR *pdwEffect )
{
    m_grfKeyState = grfKeyState;
    m_dwEffect = DROPEFFECT_NONE;

    //
    //  TODO: We need to know the type of file and where it's coming from
    //  to determine what kind of operation can be done. Replace the TRUE
    //  with something more accurate.
    //

    if( TRUE )
    {
        m_dwEffect = DROPEFFECT_COPY;

        if( grfKeyState & MK_SHIFT )
            m_dwEffect = DROPEFFECT_MOVE;

        DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: CFontView::DragEnter called" ) ) );

        if( m_hwndView )
        {
            RECT rc;
            POINT pti;

            GetWindowRect( m_hwndParent, &rc );


            //
            // If the font folder window is RTL mirrored, then the client
            // coordinate are measured from the visual right edge. [samera]
            //
            if (GetWindowLong(m_hwndParent, GWL_EXSTYLE) & WS_EX_LAYOUTRTL)
                pti.x = rc.right-pt.x;
            else
                pti.x = pt.x-rc.left;
            pti.y = pt.y-rc.top;
            DAD_DragEnterEx2( m_hwndParent, pti, pDataObj );
        }
    }

    *pdwEffect &= m_dwEffect;

    return NOERROR;
}


STDMETHODIMP CFontView::DragOver( DWORD grfKeyState,
                                  POINTL pt,
                                  DWORD __RPC_FAR *pdwEffect )
{
    m_grfKeyState = grfKeyState;

    if( m_dwEffect != DROPEFFECT_NONE )
    {
       m_dwEffect = DROPEFFECT_COPY;

       if( grfKeyState & MK_SHIFT )
          m_dwEffect = DROPEFFECT_MOVE;

        POINT ptt;
        RECT rc;

        GetWindowRect( m_hwndParent, &rc );

        //
        // If the font folder window is RTL mirrored, then the client
        // coordinate are measured from the visual right edge. [samera]
        //
        if (GetWindowLong(m_hwndParent, GWL_EXSTYLE) & WS_EX_LAYOUTRTL)
            ptt.x = rc.right-pt.x;
        else
            ptt.x = pt.x-rc.left;
        ptt.y = pt.y-rc.top;

        DAD_DragMove( ptt );
    }

    *pdwEffect &= m_dwEffect;

    return NOERROR;
}


STDMETHODIMP CFontView::DragLeave( void )
{
    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: CFontView::DragLeave called" ) ) );

    if( m_dwEffect != DROPEFFECT_NONE && m_hwndView )
    {
        DAD_DragLeave( );
    }

    return NOERROR;
}


//
//  BUGBUG: The TrackPopupMenu does not work, if the hwnd does not have
//   the input focus. We believe this is a bug in USER, but ...
//

BOOL _TrackPopupMenuEx( HMENU hmenu,
                        UINT wFlags,
                        int x,
                        int y,
                     //    int wReserved,
                        HWND hwndOwner,
                        LPCRECT lprc )
{
    int iRet = FALSE;

    HWND hwndDummy = CreateWindow( TEXT( "Static" ), NULL,
                                   0, x, y, 1, 1, HWND_DESKTOP,
                                   NULL, g_hInst, NULL );

    if( hwndDummy )
    {
        //
        //  to restore
        //

        HWND hwndPrev = GetForegroundWindow( );

        SetForegroundWindow( hwndDummy );
        SetFocus( hwndDummy );

        iRet = TrackPopupMenu( hmenu, wFlags, x, y, 0, hwndDummy, lprc );

        //
        //  We MUST unlock the destination window before changing its Z-order.
        //
        //  DAD_DragLeave( );

        if( iRet && ( iRet != IDCANCEL ) )
        {
            //
            //  non-cancel item is selected. Make the hwndOwner foreground.
            //

            SetForegroundWindow( hwndOwner );
            SetFocus( hwndOwner );
        }
        else
        {
            //
            //  The user canceled the menu.
            //  Restore the previous foreground window
            //   (before destroying hwndDummy).
            //

            if( hwndPrev )
            {
                SetForegroundWindow( hwndPrev );
            }
        }

        DestroyWindow( hwndDummy );
    }

    return iRet;
}


STDMETHODIMP CFontView::Drop( IDataObject __RPC_FAR *pDataObj,
                              DWORD grfKeyState,
                              POINTL pt,
                              DWORD __RPC_FAR *pdwEffect )
{
    HRESULT  hr = NOERROR;


    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: CFontView::DragEnter called" ) ) );

    if( m_dwEffect != DROPEFFECT_NONE )
    {
       DAD_DragLeave( );

       //
       //  If this is us sourcing the drag, just bail. We may want to save the
       //  points of the icons.
       //

       if( m_bDragSource )
            goto done;

       //
       //  If this is a right-mouse drag, then ask the user what we should
       //  do. Otherwise, just do what is in m_dwEffect.
       //

       if( m_grfKeyState & MK_RBUTTON )
       {
            //
            //  Pop up the context menu.
            //

            HMENU hMenu = LoadMenu( g_hInst,
                                    MAKEINTRESOURCE( IDM_POPUP_DRAGDROP ) );

            if( hMenu )
            {
                HMENU hPopup = GetSubMenu( hMenu, 0 );

                if( hPopup )
                {
                    //
                    //  Bold the Open menuitem.
                    //

                    MENUITEMINFO iInfo;

                    iInfo.cbSize = sizeof( iInfo );
                    iInfo.fMask  = MIIM_STATE;

                    if( GetMenuItemInfo( hMenu, IDM_POPUP_COPY, FALSE, &iInfo ) )
                    {
                        iInfo.fState |= MFS_DEFAULT;
                        SetMenuItemInfo( hMenu, IDM_POPUP_COPY, FALSE , &iInfo );
                    }

                    UINT idCmd = _TrackPopupMenuEx( hPopup,
                                                    TPM_RETURNCMD
                                                    | TPM_LEFTALIGN
                                                    | TPM_LEFTBUTTON
                                                    | TPM_RIGHTBUTTON,
                                                    pt.x,              // x pos
                                                    pt.y,              // y pos.
                                                    m_hwndView,
                                                    NULL );

                    switch( idCmd )
                    {
                    case IDM_POPUP_MOVE:
                       DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: IDM_POPUP_MOVE" ) ) );

                       m_dwEffect = DROPEFFECT_MOVE;
                       break;

                    case IDM_POPUP_COPY:
                       DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: IDM_POPUP_COPY" ) ) );

                       m_dwEffect = DROPEFFECT_COPY;
                       break;

                    case IDM_POPUP_LINK:
                       DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: IDM_POPUP_LINK" ) ) );

                       m_dwEffect = DROPEFFECT_LINK;
                       break;

                    default:
                    // case IDM_POPUP_CANCEL:
                       DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: IDM_POPUP_CANCEL" ) ) );

                       m_dwEffect = DROPEFFECT_NONE;
                       break;
                    }
                }
                DestroyMenu( hMenu );
            }

            //
            //  The right mouse context menu may have cancelled.
            //

            if( m_dwEffect == DROPEFFECT_NONE )
                goto done;
        }

        //
        //  Do the operation. What we do with the source depends on
        //  m_dwEffect.
        //

        InstallDataObject( pDataObj, m_dwEffect, m_hwndView, this );
    }


done:
   return hr;
}



//
//  Copy a menu onto the beginning or end of another menu
//  Adds uIDAdjust to each menu ID (pass in 0 for no adjustment)
//  Will not add any item whose adjusted ID is greater than uMaxIDAdjust
//  (pass in 0xffff to allow everything)
//  Returns one more than the maximum adjusted ID that is used
//

inline int IsMenuSeparator( HMENU hm,UINT i )
{
    return( GetMenuItemID( hm, i ) == 0 );
}


UINT WINAPI MergeMenus( HMENU hmDst, HMENU hmSrc, UINT uInsert,
                        UINT uIDAdjust, UINT uIDAdjustMax, ULONG uFlags )
{
    int   nItem;
    HMENU hmSubMenu;
    BOOL  bAlreadySeparated;

    MENUITEMINFO miiSrc;

    TCHAR szName[ 256 ];
    UINT  uTemp, uIDMax = uIDAdjust;


    if( !hmDst || !hmSrc )
        goto MM_Exit;

    nItem = GetMenuItemCount( hmDst );

    if( uInsert >= (UINT)nItem )
    {
        //
        //  We are inserting an additional popup on the menu bar (I think)
        //  So it is already separated
        //

        uInsert = (UINT)nItem;

        bAlreadySeparated = TRUE;
    }
    else
    {
        //
        //  otherwise check to see if there is a separator between the items
        //  already in the destination menu and the menu being merged in
        //

        bAlreadySeparated = IsMenuSeparator( hmDst, uInsert );
    }

    if( (uFlags & MM_ADDSEPARATOR) && !bAlreadySeparated )
    {
        //
        //  Add a separator between the menus if requested by caller
        //

        InsertMenu( hmDst, uInsert, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
        bAlreadySeparated = TRUE;
    }

    //
    //  Go through the menu items and clone them
    //

    for( nItem = GetMenuItemCount( hmSrc ) - 1; nItem >= 0; nItem-- )
    {
        miiSrc.cbSize = sizeof( MENUITEMINFO );
        miiSrc.fMask = MIIM_STATE | MIIM_ID | MIIM_SUBMENU
                       | MIIM_CHECKMARKS | MIIM_TYPE | MIIM_DATA;

        //
        //  We need to reset this every time through the loop in case
        //  menus DON'T have IDs
        //

        miiSrc.fType = MFT_STRING;
        miiSrc.dwTypeData = szName;
        miiSrc.dwItemData = 0;
        miiSrc.cch = ARRAYSIZE( szName );  // szName character count.

        if(!GetMenuItemInfo( hmSrc, nItem, TRUE, &miiSrc ) )
           continue;

        if( miiSrc.fType & MFT_SEPARATOR )
        {
            //
            //  This is a separator; don't put two of them in a row
            //

            if( bAlreadySeparated )
               continue;

            if( !InsertMenuItem( hmDst, uInsert, TRUE, &miiSrc ) )
                goto MM_Exit;

            bAlreadySeparated = TRUE;
        }
        else if( miiSrc.hSubMenu )
        {
            //
            //  this item has a submenu
            //

            if( uFlags & MM_SUBMENUSHAVEIDS )
            {
                //
                //  Adjust the ID and check it
                //

                miiSrc.wID += uIDAdjust;

                if( miiSrc.wID > uIDAdjustMax )
                    continue;

                if( uIDMax <= miiSrc.wID )
                {
                    uIDMax = miiSrc.wID + 1;
                }
            }
            else
            {
                //
                //  Don't set IDs for submenus that didn't have
                //  them already
                //

                miiSrc.fMask &= ~MIIM_ID;
            }

            hmSubMenu = miiSrc.hSubMenu;

            miiSrc.hSubMenu = CreatePopupMenu( );

            if( !miiSrc.hSubMenu )
                goto MM_Exit;

            uTemp = MergeMenus( miiSrc.hSubMenu, hmSubMenu, 0, uIDAdjust,
                                uIDAdjustMax, uFlags & MM_SUBMENUSHAVEIDS );

            if( uIDMax <= uTemp )
                uIDMax = uTemp;

            if( !InsertMenuItem( hmDst, uInsert, TRUE, &miiSrc ) )
                goto MM_Exit;

            bAlreadySeparated = FALSE;
        }
        else
        {
            //
            //  This is just a regular old item
            //  Adjust the ID and check it
            //

            miiSrc.wID += uIDAdjust;

            if( miiSrc.wID > uIDAdjustMax )
                continue;

            if( uIDMax <= miiSrc.wID )
                uIDMax = miiSrc.wID + 1;

            bAlreadySeparated = FALSE;

            if( !InsertMenuItem( hmDst, uInsert, TRUE, &miiSrc ) )
                goto MM_Exit;
        }
    }

    //
    //  Ensure the correct number of separators at the beginning of the
    //  inserted menu items
    //

    if( uInsert == 0 )
    {
        if( bAlreadySeparated )
        {
            DeleteMenu( hmDst, uInsert, MF_BYPOSITION );
        }
    }
    else
    {
        if( IsMenuSeparator( hmDst, uInsert-1 ) )
        {
            if( bAlreadySeparated )
            {
                DeleteMenu( hmDst, uInsert, MF_BYPOSITION );
            }
        }
        else
        {
            if( (uFlags & MM_ADDSEPARATOR ) && !bAlreadySeparated )
            {
                //
                //  Add a separator between the menus
                //

                InsertMenu( hmDst, uInsert, MF_BYPOSITION | MF_SEPARATOR,
                            0, NULL );
            }
        }
    }

MM_Exit:
    return( uIDMax );
}


void MergeHelpMenu( HMENU hmenu, HMENU hmenuMerge )
{
    HMENU hmenuHelp = GetMenuFromID( hmenu, FCIDM_MENU_HELP );

    if ( hmenuHelp )
        MergeMenus( hmenuHelp, hmenuMerge, 0, 0, (UINT) -1, MM_ADDSEPARATOR );
}


void MergeFileMenu( HMENU hmenu, HMENU hmenuMerge )
{
    HMENU hmenuView = GetMenuFromID( hmenu, FCIDM_MENU_FILE );

    if( hmenuView )
        MergeMenus( hmenuView, hmenuMerge, 0, 0, (UINT) -1, MM_ADDSEPARATOR );
}


void MergeEditMenu( HMENU hmenu, HMENU hmenuMerge )
{
    HMENU hmenuView = GetMenuFromID( hmenu, FCIDM_MENU_EDIT );

    if( hmenuView )
        MergeMenus( hmenuView, hmenuMerge, 0, 0, (UINT) -1, 0 );
}


void MergeViewMenu( HMENU hmenu, HMENU hmenuMerge )
{
    HMENU hmenuView = GetMenuFromID( hmenu, FCIDM_MENU_VIEW );


    if( hmenuView )
    {
        int index;

        //
        //  Find the last separator in the view menu.
        //

        for( index = GetMenuItemCount( hmenuView ) - 1; index >= 0; index-- )
        {
            UINT mf = GetMenuState( hmenuView, (UINT)index, MF_BYPOSITION );

            if( mf & MF_SEPARATOR )
            {
                //
                //  merge it right above the separator.
                //
                break;
            }
        }

        //
        //  Add the separator above (in addition to existing one if any).
        //

        InsertMenu( hmenuView, index, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );

        //
        //  Then merge our menu between two separators
        //  (or right below if only one).
        //

        if( index != -1 ) index++;

        MergeMenus( hmenuView, hmenuMerge, (UINT) index, 0, (UINT) -1, 0 );
    }
}


HMENU GetMenuFromID( HMENU hmMain, UINT uID )
{
    MENUITEMINFO miiSubMenu;


    miiSubMenu.cbSize = sizeof( MENUITEMINFO );
    miiSubMenu.fMask  = MIIM_SUBMENU;

    if( !GetMenuItemInfo( hmMain, uID, FALSE, &miiSubMenu ) )
        return NULL;

    return( miiSubMenu.hSubMenu );
}


void SetListColumns( HWND hWnd, UINT iCount, COLUMN_ENTRY * lpCol )
{
    LV_COLUMN lvc;
    UINT      iCol;
    TCHAR     szText[ MAX_NAME_LEN ];     // 64


    //
    //  Delete the current columns.
    //

    while( ListView_DeleteColumn( hWnd, 0 ) )
        ;

    //
    //  Initialize the LV_COLUMN structure.
    //

    lvc.mask    = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.pszText = szText;

    if( !g_cxM )
    {
        SIZE siz;
        HDC  hdc = GetDC( HWND_DESKTOP );

        SelectFont( hdc, FORWARD_WM_GETFONT( hWnd, SendMessage ) );

        GetTextExtentPoint( hdc, c_szM, 1, &siz );

        ReleaseDC( HWND_DESKTOP, hdc );

        g_cxM = siz.cx;
    }

    for( iCol = 0; iCol < iCount; iCol++, lpCol++ )
    {
        lvc.iSubItem = iCol;

        lvc.cx  = lpCol->m_iWidth * g_cxM;
        lvc.fmt = lpCol->m_iFormat;

        LoadString( g_hInst,
                    lpCol->m_iID,
                    szText,
                    ARRAYSIZE( szText ) );

        if( !ListView_InsertColumn( hWnd, iCol, &lvc ) )
        {
          DEBUGMSG( (DM_TRACE1,TEXT( "SetListColumns: Couldn't add column %d" ), iCol ) );
//          DEBUGBREAK;
       }
    }

}


void CFontView::StatusPush( UINT nStatus )
{
    TCHAR szText[ 128 ];

    if( LoadString( g_hInst, nStatus, szText, ARRAYSIZE( szText ) ) )
        StatusPush( szText );
}


void CFontView::StatusPush( LPTSTR lpsz )
{
    OLECHAR szOle[ 256 ];


    if( m_psb )
    {
#ifdef UNICODE
        m_psb->SetStatusTextSB( lpsz );
#else
        MultiByteToWideChar( CP_ACP, 0, lpsz, -1, szOle, ARRAYSIZE( szOle ) );

        m_psb->SetStatusTextSB( szOle );
#endif

    }
}


void CFontView::StatusPop( )
{
    //
    //  For now, just clear the thing.
    //

    StatusClear( );
}


void CFontView::StatusClear( )
{
    if( m_psb )
    {
        m_psb->SetStatusTextSB( (LPCOLESTR) TEXT( "" ) );
        m_psb->SendControlMsg( FCW_STATUS, SB_SIMPLE, 0, 0L, NULL );
    }
}


// *** IPersist methods ***

STDMETHODIMP CFontView::GetClassID( LPCLSID lpClassID )
{
    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: CFontView/IPersistFolder ::GetClassID called" ) ) );

    return ResultFromScode( E_NOTIMPL );
}


// *** IPersistFolder methods ***

STDMETHODIMP CFontView::Initialize( LPCITEMIDLIST pidl )
{
    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: CFontView/IPersistFolder ::Initialize called" ) ) );

    HRESULT hResult = E_FAIL;
    TCHAR szPersistDataPath[MAX_PATH];

    if (SHGetPathFromIDList(pidl, szPersistDataPath))
    {
        TCHAR szFontsPath[MAX_PATH];

        ::GetFontsDirectory(szFontsPath, ARRAYSIZE(szFontsPath));

        if (0 == lstrcmpi(szFontsPath, szPersistDataPath))
        {
            //
            // Only treat this persistent storage as the font folder if
            // it is the "fonts" directory on the local machine.  Otherwise,
            // the shell should just browse it as a normal shell folder.
            //
            hResult = NO_ERROR;
        }
    }

    return hResult;
}

