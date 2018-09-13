//-------------------------------------------------------------------------//
//  linkwnd.cpp - implementation of CLinkWindow
//
//  [scotthan] - created 10/7/98

#include "shellprv.h"
#include "ids.h"
#include <oleacc.h>

//-------------------------------------------------------------------------//
#define IS_LINK(pBlock)     ((pBlock) && (pBlock)->iLink != INVALID_LINK_INDEX)

#ifndef ARRAYSIZE
#define ARRAYSIZE(a)        (sizeof(a)/sizeof(*a))
#endif

#ifndef POINTSPERRECT
#define POINTSPERRECT       (sizeof(RECT)/sizeof(POINT))
#endif

#ifndef RECTWIDTH
#define RECTWIDTH(prc)      ((prc)->right - (prc)->left)
#endif
#ifndef RECTHEIGHT
#define RECTHEIGHT(prc)     ((prc)->bottom - (prc)->top)
#endif

#define TESTKEYSTATE(vk)   ((GetKeyState(vk) & 0x8000)!=0)

#define LINKCOLOR_BKGND     COLOR_WINDOW
#define LINKCOLOR_ENABLED   GetSysColor( GetCOLOR_HOTLIGHT() )
#define LINKCOLOR_DISABLED  GetSysColor( COLOR_GRAYTEXT )

#define CF_SETCAPTURE  0x0001
#define CF_SETFOCUS    0x0002

//  KEYBOARDCUES helpes
#ifdef KEYBOARDCUES
void _InitializeUISTATE( IN HWND hwnd, IN OUT UINT* puFlags );
BOOL _HandleWM_UPDATEUISTATE( IN WPARAM wParam, IN LPARAM lParam, IN OUT UINT* puFlags );
#endif

//-------------------------------------------------------------------------//
//  class CAccessibleBase
//
//  common IAccessible implementation.
//
class CAccessibleBase : public IAccessible, public IOleWindow
//-------------------------------------------------------------------------//
{
public:
    CAccessibleBase( const HWND& hwnd )
        :   _cRef(1), _ptiAcc(NULL), _hwnd(hwnd)
    { 
        DllAddRef();
    }
    
    virtual ~CAccessibleBase()
    { 
        ATOMICRELEASE(_ptiAcc);
    }

    //  IUnknown
    STDMETHODIMP         QueryInterface( REFIID riid, void** ppvObj );
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    //  IOleWindow
    STDMETHODIMP GetWindow( HWND* phwnd );
    STDMETHODIMP ContextSensitiveHelp( BOOL fEnterMode ) { return E_NOTIMPL; }

    // IDispatch
    STDMETHODIMP GetTypeInfoCount( UINT * pctinfo );
    STDMETHODIMP GetTypeInfo( UINT itinfo, LCID lcid, ITypeInfo** pptinfo );
    STDMETHODIMP GetIDsOfNames( REFIID riid, OLECHAR** rgszNames, UINT cNames,
                                LCID lcid, DISPID * rgdispid );
    STDMETHODIMP Invoke( DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
                         DISPPARAMS * pdispparams, VARIANT * pvarResult, 
                         EXCEPINFO * pexcepinfo, UINT * puArgErr );
    //  IAccessible
    STDMETHODIMP get_accParent( IDispatch ** ppdispParent);
    STDMETHODIMP get_accChildCount( long * pcChildren );
    STDMETHODIMP get_accChild( VARIANT varChildIndex, IDispatch ** ppdispChild);
    STDMETHODIMP get_accValue( VARIANT varChild, BSTR* pbstrValue);
    STDMETHODIMP get_accDescription( VARIANT varChild, BSTR * pbstrDescription);
    STDMETHODIMP get_accRole( VARIANT varChild, VARIANT *pvarRole );
    STDMETHODIMP get_accState( VARIANT varChild, VARIANT *pvarState );
    STDMETHODIMP get_accHelp( VARIANT varChild, BSTR* pbstrHelp );
    STDMETHODIMP get_accHelpTopic( BSTR* pbstrHelpFile, VARIANT varChild, long* pidTopic );
    STDMETHODIMP get_accKeyboardShortcut( VARIANT varChild, BSTR* pbstrKeyboardShortcut );
    STDMETHODIMP get_accFocus( VARIANT FAR * pvarFocusChild );
    STDMETHODIMP get_accSelection( VARIANT FAR * pvarSelectedChildren );
    STDMETHODIMP get_accDefaultAction( VARIANT varChild, BSTR* pbstrDefaultAction );
    STDMETHODIMP accSelect( long flagsSelect, VARIANT varChild );
    STDMETHODIMP accLocation( long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varChild );
    STDMETHODIMP accNavigate( long navDir, VARIANT varStart, VARIANT * pvarEndUpAt );
    STDMETHODIMP accHitTest( long xLeft, long yTop, VARIANT * pvarChildAtPoint );
    STDMETHODIMP put_accName( VARIANT varChild, BSTR bstrName );
    STDMETHODIMP put_accValue( VARIANT varChild, BSTR bstrValue );

protected:
    virtual UINT GetDefaultActionStringID() const = 0;
    
private:
    ULONG       _cRef;
    ITypeInfo*  _ptiAcc;
    const HWND& _hwnd;

    //  Thunked OLEACC defs from winuser.h
    #ifndef OBJID_WINDOW
    #define OBJID_WINDOW        0x00000000
    #endif//OBJID_WINDOW

    #ifndef OBJID_TITLEBAR
    #define OBJID_TITLEBAR      0xFFFFFFFE
    #endif//OBJID_TITLEBAR

    #ifndef OBJID_CLIENT
    #define OBJID_CLIENT        0xFFFFFFFC
    #endif//OBJID_CLIENT

    #ifndef CHILDID_SELF
    #define CHILDID_SELF        0
    #endif//CHILDID_SELF

    #define VALIDATEACCCHILD( varChild, idChild, hrFail ) \
        if( !(VT_I4 == varChild.vt && idChild == varChild.lVal) ) {return hrFail;}

} ;

//-------------------------------------------------------------------------//
#define TEST_CAPTURE(fTest)           ((_fCapture & fTest) != 0)
#define MODIFY_CAPTURE(fSet, fRemove) {if(fSet){_fCapture |= fSet;} if(fRemove){_fCapture &= ~fRemove;}}
#define RESET_CAPTURE()               {_fCapture=0;}

//-------------------------------------------------------------------------//
//  class CLinkWindow
class CLinkWindow : public CAccessibleBase
//-------------------------------------------------------------------------//
{
public:
    CLinkWindow();
    virtual ~CLinkWindow();

    //  IAccessible specialization
    STDMETHODIMP get_accName( VARIANT varChild, BSTR* pbstrName);
    STDMETHODIMP accDoDefaultAction( VARIANT varChild );

private:
    //  CAccessibleBase overrides
    UINT GetDefaultActionStringID() const   { return IDS_LINKWINDOW_DEFAULTACTION; }

    //  Private types
    struct RECTLISTENTRY          // rect list member
    {
        RECT            rc;
        RECTLISTENTRY*  next;
    };

    struct TEXTBLOCK              // text segment data
    {
        int             iLink;   // index of link (INVALID_LINK_INDEX if static text)
        DWORD           state;   // state bits
        TCHAR           szID[MAX_LINKID_TEXT]; // link identifier.
        TEXTBLOCK*      next;    // next block
        RECTLISTENTRY*  rgrle;   // list of bounding rectangle(s)
        TCHAR*          pszText; // text
        TCHAR*          pszUrl;  // URL.

        TEXTBLOCK();
        ~TEXTBLOCK();
        void AddRect( const RECT& rc );
        void FreeRects();
    };

    //  Utility methods
    BOOL    CreateFonts( BOOL bRecreate = FALSE );
    void    DestroyFonts();
    HCURSOR GetLinkCursor();

    void        Parse( LPCTSTR pszText = NULL );
    BOOL        Add( TEXTBLOCK* pAdd );
    TEXTBLOCK*  FindLink( int iLink ) const;
    void        FreeBlocks();

    void    SetText( LPCTSTR pszText );
    int     GetText( BOOL bForParsing, LPTSTR pszText, int cchText ) const;
    int     GetTextW( BOOL bForParsing, LPWSTR pwszText, int cchText ) const;
    int     GetTextLength( BOOL bForParsing ) const;

    void    Paint( HDC hdc, IN OPTIONAL LPCRECT prcClient = NULL, LPCRECT prcClip = NULL );
    int     CalcIdealHeight( int cx );
    int     HitTest( const POINT& pt ) const;
    BOOL    WantTab( BOOL* biFocus = NULL ) const;
    void    AssignTabFocus( int nDirection );
    int     GetNextEnabledLink( int iStart, int nDir ) const;
    int     StateCount( DWORD dwStateMask, DWORD dwState ) const;
    LONG    EnableNotifications( BOOL bEnable );

    static  TEXTBLOCK* CreateBlock( LPCTSTR pszStart, LPCTSTR pszEnd, int iLink );
    
    //  Message handlers
    static  LRESULT WINAPI WndProc( HWND, UINT, WPARAM, LPARAM );
    void    OnButtonDown( WPARAM fwKeys, const POINT& pt );
    void    OnButtonUp( WPARAM fwKeys, const POINT& pt );
    void    OnCaptureLost( HWND hwndNew ) {RESET_CAPTURE();}
    LRESULT OnFocus( HWND hwndPrev );
    void    OnKeyDown( UINT virtKey );
    LRESULT SendNotify( UINT nCode, int iLink, LPCTSTR pszLinkID = NULL ) const;
    LRESULT GetItem( OUT LWITEM* pItem );
    LRESULT SetItem( IN LWITEM* pItem );

    //  Data
    TEXTBLOCK*   _rgBlocks;        // linked list of text blocks
    int          _cBlocks;         // block count
    int          _cLinks;          // link count
    int          _iFocus;          // index of focus link
    int          _cyIdeal;
    LPTSTR       _pszCaption;      
    HWND         _hwnd;
    HFONT        _hfStatic, 
                 _hfLink;
    UINT         _fCapture;
    UINT         _fKeyboardCues;                 
    POINT        _ptCapture;
    HCURSOR      _hcurHand;
    LONG         _cNotifyLocks;

    friend BOOL LinkWindow_RegisterClass();
};

//-------------------------------------------------------------------------//
LPTSTR _AllocAndCopy( LPTSTR& pszDest, LPCTSTR pszSrc )
{
    if( pszDest )
    {
        delete [] pszDest;
        pszDest = NULL;
    }

    if( pszSrc && (pszDest = new TCHAR[lstrlen(pszSrc)+1]) != NULL )
        lstrcpy( pszDest, pszSrc );

    return pszDest;
}

BOOL _AssignBit( const DWORD dwBit, DWORD& dwDest, const DWORD dwSrc )    // returns TRUE if changed
{
    if( ((dwSrc & dwBit) != 0) != ((dwDest & dwBit) != 0) )
    {
        if( ((dwSrc & dwBit) != 0) )
            dwDest |= dwBit;
        else
            dwDest &= ~dwBit;
        return TRUE;
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
BOOL WINAPI LinkWindow_RegisterClass()
{
    WNDCLASSEX wc;
    ZeroMemory( &wc, sizeof(wc) );
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_GLOBALCLASS;
    wc.lpfnWndProc   = CLinkWindow::WndProc;
    wc.hInstance     = HINST_THISDLL;
    wc.hIcon         = NULL;
    wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground = (HBRUSH)(LINKCOLOR_BKGND+1);
    wc.lpszClassName = LINKWINDOW_CLASS;

    return ::RegisterClassEx( &wc ) != 0 || 
             GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

//-------------------------------------------------------------------------//
BOOL WINAPI LinkWindow_UnregisterClass( HINSTANCE hInst )
{
    return ::UnregisterClass( LINKWINDOW_CLASS, hInst );
}

//-------------------------------------------------------------------------//
CLinkWindow::CLinkWindow()
    :   CAccessibleBase( _hwnd ),
        _rgBlocks(NULL),
        _cBlocks(0),
        _cLinks(0),
        _cyIdeal(0),
        _hwnd(NULL),
        _hfStatic(NULL),
        _hfLink(NULL),
        _iFocus(INVALID_LINK_INDEX),
        _fCapture(0),
        _pszCaption(NULL),
        _fKeyboardCues(0),
        _hcurHand(NULL),
        _cNotifyLocks(0)
{
    _ptCapture.x = _ptCapture.y = 0;
}

//-------------------------------------------------------------------------//
CLinkWindow::~CLinkWindow()
{
    FreeBlocks();
    DestroyFonts();
    SetText( NULL );
}

//-------------------------------------------------------------------------//
//  CLinkWindow IAccessible impl
//
//  Note: Currently, this IAccessible implementation does not supports only
//  single links; multiple links are not supported.   All child delegation
//  is to/from self.  This allows us to blow off the IEnumVARIANT and IDispatch
//  implementations.
//
//  To shore this up the implementation, we need to implement each link
//  as a child IAccessible object and delegate accordingly.
//
STDMETHODIMP CLinkWindow::get_accName( VARIANT varChild, BSTR* pbstrName)
{
    VALIDATEACCCHILD( varChild, CHILDID_SELF, E_INVALIDARG );

    if( NULL == pbstrName )
        return E_POINTER;
    *pbstrName = 0;

    int cch = GetTextLength( FALSE );
    if( (*pbstrName = SysAllocStringLen(NULL, cch + 1)) != NULL )
    {
        GetTextW( FALSE, *pbstrName, cch + 1 );
        return S_OK;
    }
    return E_OUTOFMEMORY;
}

STDMETHODIMP CLinkWindow::accDoDefaultAction( VARIANT varChild )
{
    VALIDATEACCCHILD( varChild, CHILDID_SELF, E_INVALIDARG );
    SendNotify( NM_RETURN, _iFocus );
    return S_OK;
}

//-------------------------------------------------------------------------//
//  CLinkWindow window implementation
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
void CLinkWindow::FreeBlocks()
{
    for( TEXTBLOCK* pBlock = _rgBlocks; pBlock;  )
    {
        TEXTBLOCK* pNext = pBlock->next;
        delete pBlock;
        pBlock = pNext;
    }
    _rgBlocks = NULL;
    _cBlocks = _cLinks = 0;
}

//-------------------------------------------------------------------------//
CLinkWindow::TEXTBLOCK* CLinkWindow::CreateBlock( LPCTSTR pszStart, LPCTSTR pszEnd, int iLink )
{
    TEXTBLOCK* pBlock = NULL;
    int cch = (int)(pszEnd - pszStart) + 1;
    if( cch > 0 )
    {
        if( (pBlock = new TEXTBLOCK) != NULL )
        {
            if( (pBlock->pszText = new TCHAR[cch]) == NULL )
            {
                delete pBlock;
                pBlock = NULL;
            }
            else
            {
                lstrcpyn( pBlock->pszText, pszStart, cch );
                pBlock->iLink = iLink;
            }
        }
    }
    return pBlock;
}

//-------------------------------------------------------------------------//
BOOL CLinkWindow::CreateFonts( BOOL bRecreate )
{
    if( _hfStatic && _hfLink && !bRecreate )
        return TRUE;
    
    BOOL  bRet = FALSE;
    HFONT hfStatic = NULL;

    for( HWND hwnd = _hwnd; NULL == hfStatic && hwnd != NULL; hwnd = GetParent(hwnd) )
        hfStatic = (HFONT)::SendMessage( hwnd, WM_GETFONT, 0, 0L );
    
    if( hfStatic )
    {
        DestroyFonts();

        LOGFONT lf;
        if( GetObject( hfStatic, sizeof(lf), &lf ) )
        {
            //  static text has no underline
            lf.lfUnderline = FALSE;
            _hfStatic = CreateFontIndirect( &lf );

            //  link text has underline
            lf.lfUnderline = TRUE;
            _hfLink = CreateFontIndirect( &lf );

            bRet = _hfLink != NULL && _hfStatic != NULL;
        }
    }

    return bRet;
}

//-------------------------------------------------------------------------//
HCURSOR CLinkWindow::GetLinkCursor()
{
    if( !_hcurHand )
        _hcurHand = LoadHandCursor(0);

    return _hcurHand;
}

//-------------------------------------------------------------------------//
void CLinkWindow::DestroyFonts()
{
    if( _hfStatic )
    {
        DeleteObject( _hfStatic );
        _hfStatic = NULL;
    }
    if( _hfLink )
    {
        DeleteObject( _hfLink );
        _hfLink = NULL;
    }
}

//-------------------------------------------------------------------------//
void CLinkWindow::Parse( LPCTSTR pszText )
{
    TEXTBLOCK*  pBlock;
    int         cBlocks = 0, cLinks  = 0;
    LPCTSTR     psz1, psz2, pszBlock;
    LPTSTR      pszBuf = NULL;

    FreeBlocks(); // free existing blocks
    
    if( !pszText )
    {
        int cch = GetWindowTextLength( _hwnd )+1;
        if( cch <= 0 ||
            (pszBuf = new TCHAR[cch+1]) == NULL )
            goto exit;
        GetWindowText( _hwnd, pszBuf, cch );
    }
    else
        pszBuf = (LPTSTR)pszText;
    
    if( !(pszBuf && *pszBuf) )
        goto exit;

    #define LINKTAG1L    TEXT("<a>")
    #define LINKTAG1U    TEXT("<A>")
    #define cchLINKTAG1  3
    #define LINKTAG2L    TEXT("</a>")
    #define LINKTAG2U    TEXT("</A>")
    #define cchLINKTAG2  4

    for( pszBlock = pszBuf; pszBlock && *pszBlock; )
    {
        //  Search for "<a>" tag
        if( ((psz1 = StrStrI( pszBlock, LINKTAG1L )) != NULL ||
             (psz1 = StrStrI( pszBlock, LINKTAG1U )) != NULL) )
        {
            //  Add run between psz1 and pszBlock as static text
            if( psz1 > pszBlock )
            {
                if( (pBlock = CreateBlock( pszBlock, psz1, INVALID_LINK_INDEX )) != NULL )
                {
                    Add( pBlock );
                    cBlocks++;
                }
            }
        
            //  safe-skip over tag
            for( int i = 0; i < cchLINKTAG1 && psz1 && *psz1; 
                 i++, psz1 = CharNext(psz1) );

            pszBlock = psz1;

            if( psz1 && *psz1 )
            {
                if( (psz2 = StrStrI( pszBlock, LINKTAG2L )) != NULL ||
                    (psz2 = StrStrI( pszBlock, LINKTAG2U )) != NULL )
                {
                    if( (pBlock = CreateBlock( psz1, psz2, cLinks )) != NULL )
                    {
                        Add( pBlock );
                        cBlocks++;
                        cLinks++;
                    }

                    //  safe-skip over tag
                    for( int i = 0; 
                         i < cchLINKTAG2 && psz2 && *psz2; 
                         i++, psz2 = CharNext(psz2) );

                    pszBlock = psz2;
                }
                else // syntax error; mark trailing run is static text.
                {
                    psz2 = pszBlock + lstrlen( pszBlock );
                    if( (pBlock = CreateBlock( psz1, psz2, INVALID_LINK_INDEX )) != NULL )
                    {
                        Add( pBlock );
                        cBlocks++;
                    }
                    pszBlock = psz2;
                }
            }
        }
        else // no more tags.  Mark the last run of static text
        {
            psz2 = pszBlock + lstrlen( pszBlock );
            if( (pBlock = CreateBlock( pszBlock, psz2, INVALID_LINK_INDEX )) != NULL )
            {
                Add( pBlock );
                cBlocks++;
            }
            pszBlock = psz2;
        }
    }

    ASSERT( cBlocks == _cBlocks );
    ASSERT( cLinks  == _cLinks );

exit:
    if( !pszText && pszBuf ) // delete text buffer if we had alloc'd it.
        delete [] pszBuf;
}

//-------------------------------------------------------------------------//
BOOL CLinkWindow::Add( TEXTBLOCK* pAdd )
{
    BOOL bAdded = FALSE;
    pAdd->next = NULL;

    if( !_rgBlocks )    {
        _rgBlocks = pAdd;
        bAdded = TRUE;
    }
    else    {    
        for( TEXTBLOCK* pBlock = _rgBlocks; pBlock && !bAdded; pBlock = pBlock->next )   {
            if( !pBlock->next ) {
                pBlock->next = pAdd;
                bAdded = TRUE;
            }
        }
    }

    if( bAdded )    {
        _cBlocks++;
        if( IS_LINK( pAdd ) )
            _cLinks++;
    }

    return bAdded;
}

//-------------------------------------------------------------------------//
CLinkWindow::TEXTBLOCK*  CLinkWindow::FindLink( int iLink ) const
{
    if( iLink == INVALID_LINK_INDEX )
        return NULL;

    for( TEXTBLOCK* pBlock = _rgBlocks; pBlock; pBlock = pBlock->next )
    {
        if( IS_LINK( pBlock ) && pBlock->iLink == iLink )
            return pBlock;
    }
    return NULL;
}

//-------------------------------------------------------------------------//
int _IsLineBreakChar( LPCTSTR psz, int ich, TCHAR chBreak, OUT BOOL* pbRemove )
{
    LPTSTR pch;
    *pbRemove = FALSE;

    ASSERT( psz != NULL )
    ASSERT( psz[ich] != 0 );
    
    //  Try caller-provided break character (assumed a 'remove' break char).
    if( psz[ich] == chBreak )
    {
        *pbRemove = TRUE;
        return ich;
    }

    #define MAX_LINEBREAK_RESOURCE   128
    static TCHAR _szBreakRemove   [MAX_LINEBREAK_RESOURCE] = {0};
    static TCHAR _szBreakPreserve [MAX_LINEBREAK_RESOURCE] = {0};
    #define LOAD_BREAKCHAR_RESOURCE( nIDS, buff ) \
        if(0==*buff) { LoadString(HINST_THISDLL, nIDS, buff, ARRAYSIZE(buff)); }

    //  Try 'remove' break chars
    LOAD_BREAKCHAR_RESOURCE( IDS_LINEBREAK_REMOVE, _szBreakRemove );
    for( pch = _szBreakRemove; *pch; pch = CharNext(pch) )
    {
        if( psz[ich] == *pch )
        {
            *pbRemove = TRUE;
            return ich;
        }
    }

    //  Try 'preserve prior' break chars:
    LOAD_BREAKCHAR_RESOURCE( IDS_LINEBREAK_PRESERVE, _szBreakPreserve );
    for( pch = _szBreakPreserve; *pch; pch = CharNext(pch) )
    {
        if( psz[ich] == *pch )
            return ++ich;
    }

    return -1;
}

//-------------------------------------------------------------------------//
BOOL _FindLastBreakChar( 
    IN LPCTSTR pszText, 
    IN int cchText, 
    IN TCHAR chBreak,   // official break char (from TEXTMETRIC).
    OUT int* piLast, 
    OUT BOOL* pbRemove )
{
    *piLast   = 0;
    *pbRemove = FALSE;

    for( int i = cchText-1; i >= 0; i-- )
    {
#ifndef UNICODE
        if( IsDBCSLeadByte( pszText[i] ) )
            continue;
#endif
        int ich = _IsLineBreakChar( pszText, i, chBreak, pbRemove );
        if( ich >= 0 )
        {
            *piLast = ich;
            return TRUE;
        }
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
int CLinkWindow::CalcIdealHeight( int cx )
{
    int   cyRet = -1;
    HDC   hdc;
    RECT  rc;
    SIZE  sizeDC;

    if( NULL == _rgBlocks || 0 == _cBlocks )
        return -1;
    
    GetClientRect( _hwnd, &rc );
    if( cx <= 0 )
        cx = RECTWIDTH( &rc );
    else
        rc.right = cx;

    if( cx <= 0 )
        return -1;

    //  Come up with a conservative estimate for the new height.
    sizeDC.cy = MulDiv( RECTHEIGHT( &rc ), cx, RECTWIDTH( &rc ) ) * 2;
    sizeDC.cx = cx;

    if( (hdc = GetDC( _hwnd )) != NULL )
    {
        // prepare memory DC
        HDC hdcMem;
        if( (hdcMem = CreateCompatibleDC( hdc )) )
        {
            // [scotthan]: BUGBUG - Probably don't need anything but a monochrome DC
            // but should test before removing the code.
            HBITMAP hbm = CreateCompatibleBitmap( hdc, sizeDC.cx, sizeDC.cy );
            HBITMAP hbmPrev = (HBITMAP)SelectObject( hdcMem, hbm );
            
            int cyPrev = _cyIdeal;   // push ideal

            //  paint into memory DC to determine height
            SetRect( &rc, 0, 0, sizeDC.cx, sizeDC.cy );
            Paint( hdcMem, &rc );
            cyRet = _cyIdeal;

            _cyIdeal = cyPrev;       // pop ideal

            SelectObject( hdcMem, hbmPrev );
            DeleteObject( hbm );
            DeleteDC( hdcMem );
        }

        ReleaseDC( _hwnd, hdc );
    }
    return cyRet;
}


//-------------------------------------------------------------------------//
void CLinkWindow::Paint( HDC hdcClient, LPCRECT prcClient, LPCRECT prcClip )
{
    RECT rcClient;
    if( !prcClient )
    {
        GetClientRect( _hwnd, &rcClient );
        prcClient = &rcClient;
    }
    if( RECTWIDTH( prcClient )<=0 ||RECTHEIGHT( prcClient )<=0 )
        return;

    HDC             hdc = hdcClient ? hdcClient : GetDC( _hwnd );
    TEXTBLOCK*      pBlock;
    TEXTMETRIC      tm;
    int             iLine = 0,  // current line index         
                    cyLine = 0, // line height.
                    cyLeading = 0; // internal leading
    COLORREF        rgbOld = GetTextColor( hdc );  // save text color
    RECT            rcFill, 
                    rcDraw = *prcClient;             // initialize line rect
    const ULONG     dwFlags = DT_TOP|DT_LEFT;
    BOOL            fFocus = GetFocus()==_hwnd;

    //  initialize background
    SendMessage( GetParent( _hwnd ), WM_CTLCOLORSTATIC, 
                 (WPARAM)hdc, (LPARAM)_hwnd );
    SetBkMode( hdc, OPAQUE ); 
    _cyIdeal = 0;

    //  For each block of text...
    for( pBlock = _rgBlocks; pBlock; pBlock = pBlock->next )
    {
        BOOL bLink = IS_LINK(pBlock);
        SelectObject( hdc, bLink ? _hfLink : _hfStatic );
        int  cchDraw = lstrlen( pBlock->pszText ), // chars to draw, this block
             cchDrawn = 0;  // chars to draw, this block
        LPTSTR pszText = &pBlock->pszText[cchDrawn];
        
        pBlock->FreeRects();   // free hit/focus rects; we're going to recompute.
        
        //  Get font metrics
        GetTextMetrics( hdc, &tm );
        if( tm.tmExternalLeading > cyLeading )
            cyLeading = tm.tmExternalLeading;

        //  initialize foreground color
        if( bLink )
        {
            BOOL bEnabled = pBlock->state & LWIS_ENABLED;
            SetTextColor( hdc, bEnabled ? LINKCOLOR_ENABLED : LINKCOLOR_DISABLED );
        }
        else
            SetTextColor( hdc, GetSysColor( COLOR_WINDOWTEXT ) );

        //  while text remains...
        while( cchDraw > 0 )
        {
            //  compute line height and maximum text width to rcBlock
            RECT rcBlock;
            BOOL bShy = FALSE;
            int  cchTry = cchDraw;
            int  cchFit = 0;
            SIZE sizeFit;
            BOOL bRemoveBreak = FALSE;
            
            for(;;)
            {
                if( !GetTextExtentExPoint( hdc, pszText, cchTry, RECTWIDTH(&rcDraw),
                                           &cchFit, NULL, &sizeFit ) )
                {
                    cchTry--;
                    continue;
                }
                else if( cchFit < cchTry )
                {
                    BOOL fBreak = _FindLastBreakChar( pszText, cchFit, tm.tmBreakChar, &cchTry, &bRemoveBreak );

                    if( 0 == cchTry )
                    {
                        if( !fBreak && (0 == rcDraw.left /* nothing drawn on this line*/) )
                            cchTry = cchFit; // no break character found, so force a break.
                    }
                    else
                    {
                        GetTextExtentExPoint( hdc, pszText, cchTry, RECTWIDTH(&rcDraw),
                                              &cchFit, NULL, &sizeFit );
                    }
                    break;
                }
                else
                    break;
            }
            
            cyLine = sizeFit.cy;
            SetRect( &rcBlock, 0, 0, sizeFit.cx, sizeFit.cy );
            OffsetRect( &rcBlock, rcDraw.left - rcBlock.left, 0 );
                
            //  initialize drawing rectangle
            rcDraw.right  = min( rcDraw.left + RECTWIDTH(&rcBlock), prcClient->right );
            rcDraw.bottom = rcDraw.top + cyLine;
            
            //  draw the text
            ExtTextOut( hdc, rcDraw.left, rcDraw.top, ETO_OPAQUE|ETO_CLIPPED,
                        &rcDraw, pszText, cchTry, NULL );

            //  Add rectangle to block's list
            if( bLink && cchTry )
                pBlock->AddRect( rcDraw );

            cchDrawn = cchTry;

            if( cchTry < cchDraw ) // we got clipped
            {
                //  fill line to right boundary
                SetRect( &rcFill, rcDraw.right, rcDraw.top, prcClient->right, rcDraw.bottom );
                ExtTextOut( hdc, rcFill.left, rcFill.top, ETO_OPAQUE,
                            &rcFill, NULL, 0, NULL );

                //  adjust text
                if( bRemoveBreak )
                    cchDrawn++;

                pszText += cchDrawn;

                //  advance to next line
                iLine++;
                rcDraw.left = 0;
                rcDraw.top  = iLine * cyLine;
                rcDraw.bottom = rcDraw.top + cyLine + cyLeading;
                rcDraw.right = prcClient->right;
            }
            else //  we were able to draw the entire text
            {
                //  adjust drawing rectangle
                rcDraw.left += RECTWIDTH(&rcBlock);
                rcDraw.right = prcClient->right;

                //  if this is the last block of text, fill line to right boundary
                if( pBlock->next == NULL )
                {
                    rcFill = rcDraw;
                    rcFill.right = prcClient->right;
                    ExtTextOut( hdc, rcFill.left, rcFill.top, ETO_OPAQUE,
                                &rcFill, NULL, 0, NULL );
                }
            }
            _cyIdeal = rcDraw.bottom;
            cchDraw -= cchDrawn;
        }

        //  Draw focus rect(s)
#ifdef KEYBOARDCUES
        if( 0 == (_fKeyboardCues & UISF_HIDEFOCUS) )
#endif
        {
            if( fFocus && pBlock->iLink == _iFocus )
            {
                HBRUSH hbr, hbrOld;
                COLORREF rgbBkgnd = GetBkColor( hdc );
                if( (hbr = CreateSolidBrush( rgbBkgnd )) )
                {
                    hbrOld = (HBRUSH)SelectObject( hdc, hbr );
                
                    for( RECTLISTENTRY* prce = pBlock->rgrle; prce && hbr; prce = prce->next )
                    {
                        RECT rc = prce->rc;
                        FrameRect( hdc, &rc, hbr );
                        SetTextColor( hdc, rgbOld );
                        DrawFocusRect( hdc, &rc );
                    }

                    SelectObject( hdc, hbrOld );
                    DeleteObject( hbr );
                }
            }
        }
    }

    //  Fill remainder of client rect.
    RECT rcX;
    rcFill = *prcClient;
    rcFill.top = rcDraw.top + cyLine;
    if( prcClip == NULL )
    {
        ExtTextOut( hdc, rcFill.left, rcFill.right, ETO_OPAQUE,
                    &rcFill, NULL, 0, NULL );
    }
    else if( IntersectRect( &rcX, prcClip, &rcFill ) )
    {
        ExtTextOut( hdc, rcX.left, rcX.right, ETO_OPAQUE,
                    &rcX, NULL, 0, NULL );
    }

    SetTextColor( hdc, rgbOld );   // restore text color

    if( NULL == hdcClient && hdc )  // release DC if we acquired it.
        ReleaseDC( _hwnd, hdc );
}

//-------------------------------------------------------------------------//
int CLinkWindow::HitTest( const POINT& pt ) const
{
    //  Walk blocks until we find a link rect that contains the point
    TEXTBLOCK* pBlock;
    for( pBlock = _rgBlocks; pBlock; pBlock = pBlock->next )
    {
        if( IS_LINK(pBlock) && (pBlock->state & LWIS_ENABLED)!=0 )
        {
            RECTLISTENTRY* prce;
            for( prce = pBlock->rgrle; prce; prce = prce->next )
            {
                if( PtInRect( &prce->rc, pt ) )
                {
                    return pBlock->iLink;
                }
            }
        }
    }
    return INVALID_LINK_INDEX;
}

//-------------------------------------------------------------------------//
LRESULT CLinkWindow::SetItem( IN LWITEM* pItem )
{
    TEXTBLOCK*  pBlock;
    BOOL        bRedraw = FALSE;
    LRESULT     lRet = 0L;

    if( NULL == pItem || 0 == (pItem->mask & LWIF_ITEMINDEX) )
        return lRet; //BUGBUG: need to open up search keys to LWIF_ITEMID and LWIF_URL.

    if( (pBlock = FindLink( pItem->iLink )) != NULL )
    {
        if( pItem->mask & LWIF_STATE )
        {
            if( pItem->stateMask & LWIS_ENABLED )
            {
                bRedraw |= _AssignBit( LWIS_ENABLED, pBlock->state, pItem->state );
                BOOL bEnabled = IsWindowEnabled( _hwnd );
                int  cEnabledLinks = StateCount( LWIS_ENABLED, LWIS_ENABLED );

                if( bEnabled )
                {
                    if( bEnabled && 0 == cEnabledLinks )
                        EnableWindow( _hwnd, FALSE );
                    else if( !bEnabled && cEnabledLinks!=0 )
                        EnableWindow( _hwnd, TRUE );
                }
            }

            if( pItem->stateMask & LWIS_VISITED )
                bRedraw |= _AssignBit( LWIS_VISITED, pBlock->state, pItem->state );

            if( pItem->stateMask & LWIS_FOCUSED )
            {
                //  Focus assignment is handled differently;
                //  one and only one link can have focus...
                if( pItem->state & LWIS_FOCUSED )
                {
                    bRedraw |= (_iFocus != pItem->iLink);
                    _iFocus = pItem->iLink;
                }
                else
                {
                    bRedraw |= (_iFocus == pItem->iLink);
                    _iFocus = INVALID_LINK_INDEX;
                }
            }
        }

        if( pItem->mask & LWIF_ITEMID )
        {
            lstrcpyn( pBlock->szID, pItem->szID, sizeof(pBlock->szID) );
            lRet = 1L;        
        }

        if( pItem->mask & LWIF_URL )
        {
            _AllocAndCopy( pBlock->pszUrl, pItem->szUrl );
            lRet = 1L;
        }
    }

    if( bRedraw )
    {
        InvalidateRect( _hwnd, NULL, TRUE );
        UpdateWindow( _hwnd );
    }

    return lRet;
}



//-------------------------------------------------------------------------//
LRESULT CLinkWindow::GetItem( OUT LWITEM* pItem )
{
    TEXTBLOCK*  pBlock;
    LRESULT     lRet = 0L;

    if( NULL == pItem || 0 == (pItem->mask & LWIF_ITEMINDEX) )
        return lRet; //BUGBUG: need to open up search keys to LWIF_ITEMID and LWIF_URL.

    if( (pBlock = FindLink( pItem->iLink )) != NULL )
    {
        if( pItem->mask & LWIF_STATE )
        {
            pItem->state = 0L;
            if( pItem->stateMask & LWIS_FOCUSED )
            {
                if( _iFocus == pItem->iLink )
                    pItem->state |= LWIS_FOCUSED;
            }

            if( pItem->stateMask & LWIS_ENABLED )
            {
                if( pBlock->state & LWIS_ENABLED )
                    pItem->state |= LWIS_ENABLED;
            }

            if( pItem->stateMask & LWIS_VISITED )
            {
                if( pBlock->state & LWIS_VISITED )
                    pItem->state |= LWIS_VISITED;
            }
        }

        if( pItem->mask & LWIF_ITEMID )
        {
            lstrcpyn( pItem->szID, pBlock->szID, sizeof(pBlock->szID) );
            lRet = 1L;        
        }

        if( pItem->mask & LWIF_URL )
        {
            *pItem->szUrl = 0;
            if( pBlock->pszUrl )
                lstrcpyn( pItem->szUrl, pBlock->pszUrl, sizeof(pItem->szUrl) );
            lRet = 1L;
        }
    }

    return lRet;
}

//-------------------------------------------------------------------------//
void CLinkWindow::OnButtonDown( WPARAM fwKeys, const POINT& pt )
{
    int iLink;

    if( (iLink = HitTest( pt )) != INVALID_LINK_INDEX )
    {
        SetCursor( GetLinkCursor() );

        _iFocus = iLink;
        MODIFY_CAPTURE( CF_SETCAPTURE, 0 );
        _ptCapture = pt;

        if( GetFocus() != _hwnd )
        {
            MODIFY_CAPTURE( CF_SETFOCUS, 0 );
            EnableNotifications( FALSE ); // so the host doesn't reposition the link.
            SetFocus( _hwnd );
            EnableNotifications( TRUE );
        }

        InvalidateRect( _hwnd, NULL, FALSE );
        SetCapture( _hwnd );
    }
}

//-------------------------------------------------------------------------//
void CLinkWindow::OnButtonUp( WPARAM fwKeys, const POINT& pt )
{
    if( TEST_CAPTURE(CF_SETCAPTURE) )
    {
        ReleaseCapture();
        MODIFY_CAPTURE( 0, CF_SETCAPTURE );
        //  if the focus link contains the point, we can 
        //  notify the parent window of a click event.
        TEXTBLOCK* pBlock;
        if( (pBlock = FindLink( _iFocus )) != NULL && 
            (pBlock->state & LWIS_ENABLED) != 0 &&
            _iFocus == HitTest( pt ) )
        {
            SendNotify( NM_CLICK, _iFocus );        
        }
    }

    if( TEST_CAPTURE(CF_SETFOCUS) )
    {
        MODIFY_CAPTURE( 0, CF_SETFOCUS );
        if( GetFocus() == _hwnd ) // if we still have the focus...
            SendNotify( NM_SETFOCUS, _iFocus );
    }
}

//-------------------------------------------------------------------------//
//  WM_SETTEXT handler
void CLinkWindow::SetText( LPCTSTR pszText )
{
    if( pszText && _pszCaption && 0 == lstrcmp( pszText, _pszCaption ) )
        return; // nothing to do.
    
    if( _pszCaption )
    {
        delete [] _pszCaption;
        _pszCaption = NULL;
    }

    if( pszText && *pszText )
    {
        int cch = lstrlen( pszText );
        if( (_pszCaption = new TCHAR[cch+1]) != NULL )
            lstrcpy( _pszCaption, pszText );
    }
}

//-------------------------------------------------------------------------//
//  WM_GETTEXT handler
int CLinkWindow::GetText( BOOL bForParsing, LPTSTR pszText, int cchText ) const
{
    int cchRet = 0;
    if( pszText && cchText )
    {
        *pszText = 0;

        if( bForParsing )
        {
            if( _pszCaption && *_pszCaption &&
                lstrcpyn( pszText, _pszCaption, cchText ) )
                return lstrlen( pszText ) + 1;
        }
        else
        {
            TEXTBLOCK* pBlock;
            for( pBlock = _rgBlocks; cchText > 0 && pBlock; pBlock = pBlock->next )
            {
                if( pBlock->pszText )
                {
                    int cchBlock = lstrlen( pBlock->pszText );
                    StrNCat( pszText, pBlock->pszText, min( cchBlock + 1, cchText ) );
                    cchRet  += min( cchBlock, cchText );
                    cchText -= min( cchBlock, cchText );
                }
            }
            if( cchRet )
                cchRet++; // terminating NULL
        }
    }
    return cchRet;
}

//-------------------------------------------------------------------------//
//  WM_GETTEXT handler
int CLinkWindow::GetTextW( BOOL bForParsing, LPWSTR pwszText, int cchText ) const
{
#ifdef UNICODE
    return GetText( bForParsing, pwszText, cchText );

#else  UNICODE

    int   cchRet = 0;
    LPSTR pszText = new CHAR[cchText];
    
    if( pszText )
    {
        cchRet = GetText( bForParsing, pszText, cchText );
        if( cchRet )
        {
            SHAnsiToUnicode( pszText, pwszText, cchText );
        }
        delete [] pszText;
    }
    return cchRet;

#endif UNICODE
}

//-------------------------------------------------------------------------//
//  WM_GETTEXTLENGTH handler
int CLinkWindow::GetTextLength( BOOL bForParsing ) const
{
    int cnt = 0;

    if( bForParsing )
        cnt = (_pszCaption && *_pszCaption) ? lstrlen(_pszCaption) : 0;
    else
    {
        TEXTBLOCK* pBlock;
        for( pBlock = _rgBlocks; pBlock; pBlock = pBlock->next )
        {
            cnt += (pBlock->pszText && *pBlock->pszText) ? lstrlen(pBlock->pszText) : 0;
        }
    }

    return cnt;
}

//-------------------------------------------------------------------------//
LONG CLinkWindow::EnableNotifications( BOOL bEnable )
{
    if( bEnable )
    {
        if( _cNotifyLocks > 0 )
            _cNotifyLocks--;
    }
    else
        _cNotifyLocks++;
    
    return _cNotifyLocks;
}

//-------------------------------------------------------------------------//
LRESULT CLinkWindow::SendNotify( UINT nCode, int iLink, LPCTSTR pszLinkID ) const
{
    if( 0 == _cNotifyLocks )
    {
        NMLINKWND nm;
        ZeroMemory( &nm, sizeof(nm) );

        nm.hdr.hwndFrom = _hwnd;
        nm.hdr.idFrom   = (UINT_PTR)GetWindowLong( _hwnd, GWL_ID );
        nm.hdr.code     = nCode;
        nm.item.iLink   = iLink;

        if( pszLinkID && *pszLinkID )
            lstrcpyn( nm.item.szID, pszLinkID, ARRAYSIZE(nm.item.szID) );

        return SendMessage( GetParent( _hwnd ), WM_NOTIFY, nm.hdr.idFrom, (LPARAM)&nm );
    }
    return 0L;
}

//-------------------------------------------------------------------------//
inline void MakePoint( LPARAM lParam, OUT LPPOINT ppt )
{
    POINTS pts = MAKEPOINTS( lParam );
    ppt->x = pts.x;
    ppt->y = pts.y;
}

//-------------------------------------------------------------------------//
int CLinkWindow::GetNextEnabledLink( int iStart, int nDir ) const
{
    ASSERT( -1 == nDir || 1 == nDir );

    if( _cLinks > 0 )
    {
        if( INVALID_LINK_INDEX == iStart )
            iStart = nDir > 0 ? -1 : _cLinks;

        for( iStart += nDir; iStart >= 0; iStart += nDir )
        {
            TEXTBLOCK* pBlock;
            if( NULL == (pBlock = FindLink( iStart )) )
                return INVALID_LINK_INDEX;

            if( pBlock->state & LWIS_ENABLED )
            {
                ASSERT( iStart == pBlock->iLink );
                return iStart;
            }
        }
    }
    return INVALID_LINK_INDEX;
}

//-------------------------------------------------------------------------//
int CLinkWindow::StateCount( DWORD dwStateMask, DWORD dwState ) const
{
    TEXTBLOCK* pBlock;
    int cnt = 0;

    for( pBlock = _rgBlocks; pBlock; pBlock = pBlock->next )
    {
        if( IS_LINK(pBlock) && 
            (pBlock->state & dwStateMask) == dwState )
            cnt++;
    }
    return cnt;
}

//-------------------------------------------------------------------------//
BOOL CLinkWindow::WantTab( BOOL* biFocus ) const
{
    int nDir  = TESTKEYSTATE( VK_SHIFT ) ? -1 : 1;
    int iFocus = GetNextEnabledLink( _iFocus, nDir );

    if( INVALID_LINK_INDEX != iFocus )
    {
        if( biFocus )
            *biFocus = iFocus;
        return TRUE;
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
//  WM_SETFOCUS handler
LRESULT CLinkWindow::OnFocus( HWND hwndPrev )
{
    if( !TEST_CAPTURE(CF_SETCAPTURE) ) // if we got focus by something other than a mouse click
    {
        int nDir = 0;
        if( TESTKEYSTATE( VK_TAB ) )
            nDir = TESTKEYSTATE( VK_SHIFT ) ? -1 : 1;
        AssignTabFocus( nDir ); // move internal focus
    }
    InvalidateRect( _hwnd, NULL, FALSE );
    SendNotify( NM_SETFOCUS, _iFocus ); // notify parent
    return 0L;
}

//-------------------------------------------------------------------------//
void CLinkWindow::AssignTabFocus( int nDirection )
{
    if( _cLinks )
    {
        if( 0 == nDirection )
        {
            if( INVALID_LINK_INDEX != _iFocus )
                return;
            nDirection = 1;
        }
        _iFocus = GetNextEnabledLink( _iFocus, nDirection );
    }
}

//-------------------------------------------------------------------------//
//  WM_KEYDOWN handler
void CLinkWindow::OnKeyDown( UINT virtKey )
{
    switch( virtKey )
    {
        case VK_TAB:
            if( WantTab( &_iFocus ) )
                InvalidateRect( _hwnd, NULL, FALSE );
            break;
        
        case VK_RETURN:
        case VK_SPACE:
            if( FindLink( _iFocus ) )
                SendNotify( NM_RETURN, _iFocus );
            break;
    }
}

//-------------------------------------------------------------------------//
LRESULT WINAPI CLinkWindow::WndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    LRESULT lRet = 0L;
    CLinkWindow* pThis = NULL;

    if( uMsg == WM_NCCREATE )
    {
        pThis = new CLinkWindow;
        if( NULL == pThis )
        {
            ASSERT( pThis );
            return FALSE;
        }
        pThis->_hwnd = hwnd;
        SetWindowPtr( hwnd, GWLP_USERDATA, pThis );
        return TRUE;
    }
    else
    {
        pThis = (CLinkWindow*)GetWindowPtr( hwnd, GWLP_USERDATA );
        ASSERT( pThis );
        ASSERT( pThis->_hwnd == hwnd );
    }

    switch( uMsg )
    {
        case WM_NCHITTEST:
        {
            POINT pt;
            MakePoint( lParam, &pt );
            MapWindowPoints( HWND_DESKTOP, hwnd, &pt, 1 );
            if( pThis->HitTest( pt ) != INVALID_LINK_INDEX )
                return HTCLIENT;
            return HTTRANSPARENT;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC         hdc;

            if( (hdc = BeginPaint( pThis->_hwnd, &ps )) != NULL )
            {
                pThis->Paint( hdc );
                EndPaint( pThis->_hwnd, &ps );
            }
            return lRet;
        }

        case WM_WINDOWPOSCHANGING:
        {
            WINDOWPOS* pwp = (WINDOWPOS*)lParam;
            RECT rc;
            GetClientRect( pThis->_hwnd, &rc );
            if( 0 == (pwp->flags & SWP_NOSIZE) &&
                !( pwp->cx == RECTWIDTH(&rc) &&
                   pwp->cy == RECTHEIGHT(&rc) ) )
            {
                //  BUGBUG: implement LWS_AUTOHEIGHT style by
                //  calling CalcIdealHeight() to compute the height for
                //  the given width.
            }
            break;
        }

        case WM_SIZE:
        {
            pThis->Paint( NULL );
            break;
        }

        case WM_CREATE:
        {
            if( (lRet = DefWindowProc( hwnd, uMsg, wParam, lParam )) == 0 )
            {
                CREATESTRUCT* pcs = (CREATESTRUCT*)lParam;
#ifdef KEYBOARDCUES
                _InitializeUISTATE( hwnd, &pThis->_fKeyboardCues );
#endif//KEYBOARDCUES
                pThis->CreateFonts();
                pThis->SetText( pcs->lpszName );
                pThis->Parse( pThis->_pszCaption );
            }
            return lRet;
        }

        case WM_SETTEXT:
            pThis->SetText( (LPCTSTR)lParam );
            pThis->Parse( pThis->_pszCaption );
            InvalidateRect( pThis->_hwnd, NULL, FALSE );
            break;

        case WM_GETTEXT:
            return pThis->GetText( TRUE, (LPTSTR)lParam, (int)wParam );

        case WM_GETTEXTLENGTH:
            return pThis->GetTextLength( TRUE );

        case WM_SETFOCUS:
            return pThis->OnFocus( (HWND)wParam );

        case WM_KILLFOCUS:
            pThis->SendNotify( NM_KILLFOCUS, pThis->_iFocus );
            pThis->_iFocus = INVALID_LINK_INDEX;
            InvalidateRect( pThis->_hwnd, NULL, FALSE );
            return lRet;

        case WM_LBUTTONDOWN:
        {
            POINT pt;
            MakePoint( lParam, &pt );
            pThis->OnButtonDown( wParam, pt );
            break;
        }

        case WM_LBUTTONUP:
        {
            POINT pt;
            MakePoint( lParam, &pt );
            pThis->OnButtonUp( wParam, pt );
            break;
        }

        case WM_MOUSEMOVE:
        {
            POINT pt;
            MakePoint( lParam, &pt );
            if( pThis->HitTest( pt ) != INVALID_LINK_INDEX ) 
                SetCursor( pThis->GetLinkCursor() );
            break;
        }
    
        case WM_CAPTURECHANGED:
            if( lParam /* NULL if we called ReleaseCapture() */ )
                pThis->OnCaptureLost( (HWND)lParam );
            break;

        case LWM_HITTEST:  // wParam: n/a, lparam: LPLWITEM, ret: BOOL
        {
            LWHITTESTINFO* phti = (LWHITTESTINFO*)lParam;
            if( phti )
            {
                TEXTBLOCK* pBlock;
                *phti->item.szID = 0;
                if( (phti->item.iLink = pThis->HitTest( phti->pt )) != INVALID_LINK_INDEX &&
                    (pBlock = pThis->FindLink( phti->item.iLink )) != NULL )
                {
                    lstrcpyn( phti->item.szID, pBlock->szID, ARRAYSIZE(phti->item.szID) );
                    return TRUE;
                }
            }
            return lRet;
        }

        case LWM_SETITEM:
            return pThis->SetItem( (LWITEM*)lParam );

        case LWM_GETITEM:
            return pThis->GetItem( (LWITEM*)lParam );

        case LWM_GETIDEALHEIGHT:  // wParam: cx, lparam: n/a, ret: cy
            //  force a recalc if we've never done so
            return pThis->CalcIdealHeight( (int)wParam );

        case WM_NCDESTROY:
        {
            lRet = DefWindowProc( hwnd, uMsg, wParam, lParam );
            SetWindowPtr( hwnd, GWLP_USERDATA, 0 );
            pThis->_hwnd = NULL;
            pThis->Release();
            return lRet;
        }

        case WM_GETDLGCODE:
        {
            MSG* pmsg;
            lRet = DLGC_BUTTON|DLGC_UNDEFPUSHBUTTON;

            if( (pmsg = (MSG*)lParam) )
            {
                if( (WM_KEYDOWN == pmsg->message || WM_KEYUP == pmsg->message) )
                {
                    switch( pmsg->wParam )
                    {
                    case VK_TAB:
                        if( pThis->WantTab() )
                            lRet |= DLGC_WANTTAB;
                        break;

                    case VK_RETURN:
                    case VK_SPACE:
                        lRet |= DLGC_WANTALLKEYS;
                        break;
                    }
                }
                else if( WM_CHAR == pmsg->message && VK_RETURN == pmsg->wParam )
                {
                    //  Eat VK_RETURN WM_CHARs; we don't want
                    //  Dialog manager to beep when IsDialogMessage gets it.
                    return lRet |= DLGC_WANTMESSAGE;
                }
            }

            return lRet;
        }

        case WM_KEYDOWN:
            pThis->OnKeyDown( (UINT)wParam );
        case WM_KEYUP:
        case WM_CHAR:
            return lRet;

#ifdef KEYBOARDCUES
        case WM_UPDATEUISTATE:
            if( _HandleWM_UPDATEUISTATE( wParam, lParam, &pThis->_fKeyboardCues ) )
                RedrawWindow( hwnd, NULL, NULL, 
                              RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW );
            break;
#endif

        default:
            // oleacc defs thunked for WINVER < 0x0500
            if( IsWM_GETOBJECT( uMsg ) && OBJID_CLIENT == lParam )
                return LresultFromObject( IID_IAccessible, wParam, SAFECAST(pThis, IAccessible*) );

            break;
    }

    return DefWindowProc( hwnd, uMsg, wParam, lParam );
}


//-------------------------------------------------------------------------//
CLinkWindow::TEXTBLOCK::TEXTBLOCK()
    :   iLink(INVALID_LINK_INDEX), 
        next(NULL), 
        state(LWIS_ENABLED),
        pszText(NULL),
        pszUrl(NULL),
        rgrle(NULL)
{
    *szID = 0;
}

//-------------------------------------------------------------------------//
CLinkWindow::TEXTBLOCK::~TEXTBLOCK()
{
    //  free block text
    _AllocAndCopy( pszText, NULL );
    _AllocAndCopy( pszUrl, NULL );

    //  free rectangle(s)
    FreeRects();
}

//-------------------------------------------------------------------------//
void CLinkWindow::TEXTBLOCK::AddRect( const RECT& rc )
{
    RECTLISTENTRY* prce;
    if( (prce = new RECTLISTENTRY) != NULL )
    {
        prce->rc = rc;
        prce->next = NULL;
    }

    if( rgrle == NULL )
        rgrle = prce;
    else
    {
        for( RECTLISTENTRY* p = rgrle; p; p = p->next )
        {
            if( p->next == NULL )
            {
                p->next = prce;
                break;
            }
        }
    }
}

//-------------------------------------------------------------------------//
void CLinkWindow::TEXTBLOCK::FreeRects()
{
    for( RECTLISTENTRY* p = rgrle; p; )
    {
        RECTLISTENTRY* next = p->next;
        delete p;
        p = next;
    }
    rgrle = NULL;
}

//-------------------------------------------------------------------------//
#define GROUPBTN_BKCOLOR    COLOR_WINDOW
#define CAPTION_VPADDING    3
#define CAPTION_HPADDING    2
#define GBM_SENDNOTIFY      (GBM_LAST + 1)

//-------------------------------------------------------------------------//
//  class CGroupBtn
class CGroupBtn : public CAccessibleBase
//-------------------------------------------------------------------------//
{ // all members private:

    CGroupBtn( HWND hwnd );
    ~CGroupBtn();

    //  IAccessible specialization
    STDMETHODIMP get_accName( VARIANT varChild, BSTR* pbstrName);
    STDMETHODIMP accDoDefaultAction( VARIANT varChild );

    //  CAccessibleBase overrides
    UINT GetDefaultActionStringID() const   { return IDS_GROUPBTN_DEFAULTACTION; }

    //  window procedures
    static LRESULT WINAPI WndProc( HWND, UINT, WPARAM, LPARAM );
    static LRESULT WINAPI BuddyProc( HWND, UINT, WPARAM, LPARAM );

    //  message handlers
    BOOL    NcCreate( LPCREATESTRUCT lpcs );
    LRESULT NcCalcSize( BOOL, LPNCCALCSIZE_PARAMS );
    void    NcPaint( HRGN );
    LRESULT NcMouseMove( WPARAM, LONG, LONG );
    LRESULT NcHitTest( LONG, LONG );
    LRESULT NcButtonDown( UINT nMsg, WPARAM nHittest, const POINTS& pts );
    LRESULT NcDblClick( UINT nMsg, WPARAM nHittest, LPARAM lParam );
    LRESULT ButtonUp( UINT nMsg, WPARAM nHittest, const POINTS& pts );
    void    OnCaptureLost( HWND hwndNew ) {RESET_CAPTURE();}
    LRESULT WindowPosChanging( LPWINDOWPOS );
    LRESULT OnSize( WPARAM, LONG, LONG);
    BOOL    SetPlacement( PGBPLACEMENT );
    BOOL    SetBuddy( HWND, ULONG );
    BOOL    SetDropState( BOOL );
    void    SetText( LPCTSTR );
    int     GetText( LPTSTR, int );
    int     GetTextW( LPWSTR, int );
    int     GetTextLength();
    void    SetFont( HFONT );
    HFONT   GetFont();

    //  utility methods
    static void _MapWindowRect( HWND hwnd, HWND hwndRelative, OUT LPRECT prcWindow );
    void        _MapWindowRect( HWND hwndRelative, OUT LPRECT prcWindow );
    HCURSOR     GetHandCursor();
    void        CalcCaptionSize();
    BOOL        CalcClientRect( IN OPTIONAL LPCRECT prcWindow, OUT LPRECT prcClient );
    BOOL        CalcWindowSizeForClient( IN OPTIONAL LPCRECT prcClient, 
                                         IN OPTIONAL LPCRECT prcWindow, 
                                         IN LPCRECT prcNewClient, 
                                         OUT LPSIZE psizeWindow );

    void    DoLayout( BOOL bNewBuddy = FALSE );

    LONG    EnableNotifications( BOOL bEnable );
    LRESULT SendNotify( int nCode, IN OPTIONAL NMHDR* pnmh = NULL );
    void    PostNotify( int nCode );
    
    
    //  instance and static data
    HWND        _hwnd;
    HWND        _hwndBuddy;
    WNDPROC     _pfnBuddy;
    ULONG       _dwBuddyFlags;
    SIZE        _sizeBuddyMargin;
    HFONT       _hf;
    static ATOM _atom;
    LPTSTR      _pszCaption;
    SIZE        _sizeCaption;
    int         _yDrop;
    BOOL        _fDropped : 1,
                _fInLayout : 1;
    UINT        _fCapture;
    UINT        _fKeyboardCues;
    HCURSOR     _hcurHand;
    LONG        _cNotifyLocks;

    friend ATOM GroupButton_RegisterClass();
    friend HWND CreateGroupBtn( DWORD, LPCTSTR, DWORD, 
					            int x, int y, HWND hwndParent, UINT nID );
};

//-------------------------------------------------------------------------//
ATOM GroupButton_RegisterClass()
{
    if( CGroupBtn::_atom != 0 )
        return CGroupBtn::_atom;

    WNDCLASSEX wc;
    ZeroMemory( &wc, sizeof(wc) );
    
    wc.cbSize         = sizeof(wc);
    wc.style          = CS_GLOBALCLASS;
    wc.lpfnWndProc    = CGroupBtn::WndProc;
    wc.hInstance      = HINST_THISDLL;
    wc.hCursor        = LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground  = (HBRUSH)(GROUPBTN_BKCOLOR+1);
    wc.lpszClassName  = GROUPBUTTON_CLASS;
    //wc.lpszMenuName 
    //wc.hIcon
    //wc.hIconSm

    return (CGroupBtn::_atom = RegisterClassEx( &wc ));
}

//-------------------------------------------------------------------------//
BOOL GroupButton_UnregisterClass()
{
    return UnregisterClass( GROUPBUTTON_CLASS, HINST_THISDLL );
}

//-------------------------------------------------------------------------//
CGroupBtn::CGroupBtn( HWND hwnd ) 
    :   CAccessibleBase( _hwnd ),
        _hwnd(hwnd), 
        _hwndBuddy(NULL), 
        _pfnBuddy(NULL),
        _dwBuddyFlags(GBBF_HRESIZE|GBBF_VRESIZE),
        _fInLayout(FALSE),
        _hf(NULL), 
        _pszCaption(NULL),
        _fDropped(TRUE),
        _fKeyboardCues(0),
        _yDrop(0),
        _fCapture(0),
        _hcurHand(NULL),
        _cNotifyLocks(0)
{
    _sizeCaption.cx = _sizeCaption.cy = 0;
    _sizeBuddyMargin.cx = _sizeBuddyMargin.cy = 0;
}

ATOM    CGroupBtn::_atom = 0;

//-------------------------------------------------------------------------//
CGroupBtn::~CGroupBtn() 
{
    SetFont( NULL );
    SetText( NULL );
}

//-------------------------------------------------------------------------//
//  CGroupBtn IAccessible impl
STDMETHODIMP CGroupBtn::get_accName( VARIANT varChild, BSTR* pbstrName)
{
    VALIDATEACCCHILD( varChild, CHILDID_SELF, E_INVALIDARG );

    if( NULL == pbstrName )
        return E_POINTER;
    *pbstrName = 0;

    int cch = GetTextLength();
    if( (*pbstrName = SysAllocStringLen(NULL, cch + 1)) != NULL )
    {
        GetTextW( *pbstrName, cch + 1 );
        return S_OK;
    }
    return E_OUTOFMEMORY;
}

STDMETHODIMP CGroupBtn::accDoDefaultAction( VARIANT varChild )
{
    VALIDATEACCCHILD( varChild, CHILDID_SELF, E_INVALIDARG );
    SendNotify( NM_RETURN );
    return S_OK;
}


//-------------------------------------------------------------------------//
//  CGroupBtn window impl
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
//  WM_SETTEXT handler
void CGroupBtn::SetText( LPCTSTR pszText )
{
    if( _pszCaption )
    {
        if( pszText && 0==lstrcmp( _pszCaption, pszText ) )
            return;
        delete [] _pszCaption;
        _pszCaption = NULL;
    }

    if( pszText && *pszText )
    {
        if( (_pszCaption = new TCHAR[lstrlen(pszText)+1]) != NULL )
            lstrcpy( _pszCaption, pszText );
    }
    
    if( IsWindow( _hwnd ) )
        CalcCaptionSize();
}

//-------------------------------------------------------------------------//
//  WM_GETTEXT handler
int CGroupBtn::GetText( LPTSTR pszText, int cchText )
{
    int cch = 0;
    if( pszText && cchText > 0 )
    {
        *pszText = 0;
        if( _pszCaption && lstrcpyn( pszText, _pszCaption, cchText ) )
            cch = min( lstrlen( _pszCaption ), cchText );
    }
    return cch;
}

//-------------------------------------------------------------------------//
int CGroupBtn::GetTextW( LPWSTR pwszText, int cchText )
{
#ifdef UNICODE
    return GetText( pwszText, cchText );
#else //UNICODE

    int   cchRet = 0;
    LPSTR pszText = new CHAR[cchText];
    
    if( pszText )
    {
        cchRet = GetText( pszText, cchText );
        if( cchRet )
        {
            SHAnsiToUnicode( pszText, pwszText, cchText );
        }
        delete [] pszText;
    }
    return cchRet;

#endif //UNICODE
}

//-------------------------------------------------------------------------//
//  WM_GETTEXTLENGTH handler
int CGroupBtn::GetTextLength()
{
    return (_pszCaption && *_pszCaption) ? lstrlen( _pszCaption ) : 0 ;
}

//-------------------------------------------------------------------------//
//  WM_SETFONT handler
void CGroupBtn::SetFont( HFONT hf )
{
    if( _hf )
    {
        DeleteObject( _hf );
        _hf = NULL;
    }
    _hf = hf;
}

//-------------------------------------------------------------------------//
//  WM_GETFONT handler
HFONT CGroupBtn::GetFont()
{
    if( _hf == NULL )
    {
        //  if we don't have a font, use the parent's font
        HFONT hfParent = (HFONT)SendMessage( GetParent( _hwnd ), WM_GETFONT, 0, 0L );
        if( hfParent )
        {
            LOGFONT lf;
            if( GetObject( hfParent, sizeof(LOGFONT), &lf ) >0 )
                _hf = CreateFontIndirect( &lf );
        }
    }
    return _hf;
}

//-------------------------------------------------------------------------//
//  Hand cursor load
HCURSOR CGroupBtn::GetHandCursor()
{
    if( !_hcurHand )
        _hcurHand = LoadHandCursor(0);

    return _hcurHand;
}

//-------------------------------------------------------------------------//
//  Retrieves the window rect in relative coords.
void CGroupBtn::_MapWindowRect( HWND hwnd, HWND hwndRelative, OUT LPRECT prcWindow )
{
    ASSERT( IsWindow( hwnd ) );
    GetWindowRect( hwnd, prcWindow );
    MapWindowPoints( HWND_DESKTOP, hwndRelative, (LPPOINT)prcWindow, 2 );
}

//-------------------------------------------------------------------------//
//  Retrieves the window rect in relative coords.
inline void CGroupBtn::_MapWindowRect( HWND hwndRelative, OUT LPRECT prcWindow )
{
    _MapWindowRect( _hwnd, hwndRelative, prcWindow );
}

//-------------------------------------------------------------------------//
//  Caches the size of the caption 'bar'.
void CGroupBtn::CalcCaptionSize()
{
    SIZE    sizeCaption = {0,0};
    LPCTSTR pszCaption = (_pszCaption && *_pszCaption) ? _pszCaption : TEXT("|");
    HDC     hdc;

    //  compute caption size based on window text:
    if( (hdc = GetDC( _hwnd )) )
    {
        HFONT hf = GetFont(),
              hfPrev = (HFONT)SelectObject( hdc, hf );
        
        if( GetTextExtentPoint32( hdc, pszCaption, lstrlen( pszCaption ),
                                  &sizeCaption ) )
            sizeCaption.cy += CAPTION_VPADDING; // add some vertical padding

        SelectObject( hdc, hfPrev );
        ReleaseDC( _hwnd, hdc );
    }

    _sizeCaption = sizeCaption;
}

//-------------------------------------------------------------------------//
//  Computes the size and position of the client area
BOOL CGroupBtn::CalcClientRect( IN OPTIONAL LPCRECT prcWindow, OUT LPRECT prcClient )
{
    DWORD dwStyle = GetWindowLong( _hwnd, GWL_STYLE );
    RECT  rcWindow;

    if( !prcWindow )
    {
        //  Get parent-relative coords
        _MapWindowRect( GetParent( _hwnd ), &rcWindow );
        prcWindow = &rcWindow;
    }

    *prcClient = *prcWindow;

    //  compute client rectangle:

    //  allow for border
    if( dwStyle & WS_BORDER )
        InflateRect( prcClient, -1, -1 );

    //  allow for caption 'bar'
    prcClient->top += _sizeCaption.cy;

    //  Normalize for NULL rect.
    if( RECTWIDTH(prcWindow) <=0 )
        prcClient->left = prcClient->right = prcWindow->left;
    if( RECTHEIGHT(prcWindow) <=0 )
        prcClient->bottom = prcClient->top = prcWindow->top;

    return TRUE;
}

//-------------------------------------------------------------------------//
BOOL CGroupBtn::CalcWindowSizeForClient( 
    IN OPTIONAL LPCRECT prcClient, 
    IN OPTIONAL LPCRECT prcWindow, 
    IN LPCRECT prcNewClient, 
    OUT LPSIZE psizeWindow )
{
    if( !(prcNewClient && psizeWindow ) )
    {
        ASSERT(FALSE);
        return FALSE;
    }

    RECT rcWindow, rcClient;
    if( NULL == prcWindow )
    {
        GetWindowRect( _hwnd, &rcWindow );
        prcWindow = &rcWindow;
    }

    if( NULL == prcClient )
    {
        GetClientRect( _hwnd, &rcClient );
        prcClient = &rcClient;
    }

    SIZE sizeDelta;
    sizeDelta.cx = RECTWIDTH(prcWindow)   - RECTWIDTH(prcClient);
    sizeDelta.cy = RECTHEIGHT(prcWindow)  - RECTHEIGHT(prcClient);

    psizeWindow->cx = RECTWIDTH(prcNewClient)  + sizeDelta.cx;
    psizeWindow->cy = RECTHEIGHT(prcNewClient) + sizeDelta.cy;
    
    return TRUE;
}

//-------------------------------------------------------------------------//
//  WM_WINDOWPOSCHANGING handler
LRESULT CGroupBtn::WindowPosChanging( LPWINDOWPOS pwp )
{
    if( pwp->flags & SWP_NOSIZE )
        return DefWindowProc( _hwnd, WM_WINDOWPOSCHANGING, 0, (LPARAM)pwp );

    //  disallow sizing in buddy slave dimension(s).
    if( IsWindow( _hwndBuddy ) && _dwBuddyFlags & (GBBF_HSLAVE|GBBF_VSLAVE) && !_fInLayout )
    {
        RECT rcWindow, rcClient;
        BOOL fResizeBuddy = FALSE;

        GetWindowRect( _hwnd, &rcWindow );
        GetClientRect( _hwnd, &rcClient );

        //  Prepare a buddy size data block
        GBNQUERYBUDDYSIZE qbs;
        qbs.cy = pwp->cy - (RECTHEIGHT(&rcWindow) - RECTHEIGHT(&rcClient));
        qbs.cx = pwp->cx - (RECTWIDTH(&rcWindow) - RECTWIDTH(&rcClient));
        
        if( _dwBuddyFlags & GBBF_HSLAVE ) // prevent external horz resizing
        {
            pwp->cx = RECTWIDTH( &rcWindow );
            //  If we're being resized in the vert dir, query for
            //  optimal buddy width for this height and adjust
            if( _dwBuddyFlags & GBBF_VRESIZE && RECTHEIGHT( &rcWindow ) != pwp->cy )
            {
                if( SendNotify( GBN_QUERYBUDDYWIDTH, (NMHDR*)&qbs ) && qbs.cx >= 0 )
                {
                    //  if the owner wants the buddy width to change, do it now.
                    LONG cxNew = qbs.cx + (RECTWIDTH( &rcWindow ) - RECTWIDTH( &rcClient ));
                    fResizeBuddy = cxNew != pwp->cx;
                    pwp->cx = cxNew;
                }
            }
        }
        
        if( _dwBuddyFlags & GBBF_VSLAVE ) // prevent external vert resizing
        {
            pwp->cy = RECTHEIGHT( &rcWindow );
            //  If we're being resized in the horz dir, query for
            //  optimal buddy height for this horizontal and adjust
            if( _dwBuddyFlags & GBBF_HRESIZE && RECTWIDTH( &rcWindow ) != pwp->cx )
            {
                if( SendNotify( GBN_QUERYBUDDYHEIGHT, (NMHDR*)&qbs ) && qbs.cy >= 0 )
                {
                    LONG cyNew = qbs.cy + (RECTHEIGHT( &rcWindow ) - RECTHEIGHT( &rcClient ));
                    fResizeBuddy = cyNew != pwp->cy;
                    pwp->cy = cyNew;
                }
            }
        }

        if( fResizeBuddy )
        {
            _fInLayout = TRUE;
            SetWindowPos( _hwndBuddy, NULL, 0, 0, qbs.cx, qbs.cy,
                          SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE );
            _fInLayout = FALSE;
        }
    }

    //  enforce minimum height:
    if( pwp->cy < _sizeCaption.cy )
        pwp->cy = _sizeCaption.cy;

    return 0L;
}

//-------------------------------------------------------------------------//
LRESULT CGroupBtn::OnSize( WPARAM flags, LONG cx, LONG cy )
{
    DoLayout();
    return 0L;
} 

//-------------------------------------------------------------------------//
void CGroupBtn::DoLayout( BOOL bNewBuddy )
{
    if( !_fInLayout && IsWindow( _hwndBuddy ) )
    {
        RECT  rcWindow, rcThis, rcBuddy;
        DWORD dwSwpBuddy = SWP_NOACTIVATE,
              dwSwpThis  = SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE;
        BOOL  fReposThis = FALSE;
        SIZE  sizeNew;

        GetClientRect( _hwnd, &rcThis );
        GetWindowRect( _hwnd, &rcWindow );

        //  get rectangles in parent coords
        MapWindowPoints( _hwnd, GetParent( _hwnd ), (LPPOINT)&rcThis,    POINTSPERRECT ); 
        MapWindowPoints( HWND_DESKTOP, GetParent( _hwnd ), (LPPOINT)&rcWindow,  POINTSPERRECT ); 
        _MapWindowRect( _hwndBuddy, GetParent( _hwnd ), &rcBuddy );

        //  If we need to reposition ourself to the buddy, 
        //  calculate the new size now.
        if( _dwBuddyFlags & (GBBF_HSLAVE|GBBF_VSLAVE) )
            CalcWindowSizeForClient( &rcThis, &rcWindow, &rcBuddy, &sizeNew );

        //  Resize buddy according to size.
        if( _dwBuddyFlags & GBBF_HRESIZE )
        {
            rcBuddy.right = rcBuddy.left + RECTWIDTH(&rcThis);

            if( bNewBuddy && 0 == (_dwBuddyFlags & GBBF_VRESIZE) ) 
            {
                // query height
                GBNQUERYBUDDYSIZE qbs;
                qbs.cx = RECTWIDTH( &rcThis );
                qbs.cy = -1;
                if( SendNotify( GBN_QUERYBUDDYHEIGHT, (NMHDR*)&qbs ) && qbs.cy >= 0 )
                    rcBuddy.bottom = rcBuddy.top + qbs.cy;
            }
        }
        else if( _dwBuddyFlags & GBBF_HSLAVE )
        { 
            rcWindow.right = rcWindow.left + sizeNew.cx;
            fReposThis = TRUE;
        }
        if( _dwBuddyFlags & GBBF_VRESIZE )
        {
            rcBuddy.bottom = rcBuddy.top + RECTHEIGHT(&rcThis);

            if( bNewBuddy && 0 == (_dwBuddyFlags & GBBF_HRESIZE) ) 
            {
                // query width
                GBNQUERYBUDDYSIZE qbs;
                qbs.cx = -1;
                qbs.cy = RECTHEIGHT( &rcThis );
                if( SendNotify( GBN_QUERYBUDDYWIDTH, (NMHDR*)&qbs ) && qbs.cx >= 0 )
                    rcBuddy.right = rcBuddy.left + qbs.cx;
            }
        }
        else if( _dwBuddyFlags & GBBF_VSLAVE )
        { 
            rcWindow.bottom = rcWindow.top + sizeNew.cy;
            fReposThis = TRUE;
        }
        
        if( _dwBuddyFlags & GBBF_HSCROLL ) 
        { 
            /* not implemented */
        }

        if( _dwBuddyFlags & GBBF_VSCROLL )
        { 
            /* not implemented */
        }

        //  reposition ourself and update our client rect.
        if( fReposThis )
         {
            _fInLayout = TRUE;
            SetWindowPos( _hwnd, NULL, 0, 0, 
                          RECTWIDTH( &rcWindow ), RECTHEIGHT( &rcWindow ), dwSwpThis );
            _fInLayout = FALSE;
            GetClientRect( _hwnd, &rcThis );
            MapWindowPoints( _hwnd, GetParent( _hwnd ), (LPPOINT)&rcThis,  POINTSPERRECT ); 
        }
        
        //  slide buddy into client area and reposition
        OffsetRect( &rcBuddy, rcThis.left - rcBuddy.left, rcThis.top - rcBuddy.top );
        
        _fInLayout = TRUE;
        SetWindowPos( _hwndBuddy, _hwnd, rcBuddy.left, rcBuddy.top,
                      RECTWIDTH( &rcBuddy ), RECTHEIGHT( &rcBuddy ), dwSwpBuddy );
        _fInLayout = FALSE;
    }
}

//-------------------------------------------------------------------------//
//  GBM_SETPLACEMENT handler
BOOL CGroupBtn::SetPlacement( PGBPLACEMENT pgbp )
{
    RECT  rcWindow, rcClient;
    SIZE  sizeDelta;
    DWORD dwFlags = SWP_NOZORDER|SWP_NOACTIVATE;

    ZeroMemory( &sizeDelta, sizeof(sizeDelta) );
    _MapWindowRect( GetParent( _hwnd ), &rcWindow );
    CalcClientRect( &rcWindow, &rcClient );

    //  establish whether we need to resize
    if( (pgbp->x < 0 || pgbp->x == rcWindow.left) && 
        (pgbp->y < 0 || pgbp->y == rcWindow.top) )
        dwFlags |= SWP_NOMOVE;

    //  compute horizontal placement
    if( pgbp->x >= 0 )  // fixed horz origin requested
        OffsetRect( &rcWindow, pgbp->x - rcWindow.left, 0 );

    if( pgbp->cx >= 0 ) // fixed width requested
        rcWindow.right = rcWindow.left + pgbp->cx;
    else
    {
        if( pgbp->cxBuddy >= 0 ) // client width requested
            sizeDelta.cx = pgbp->cxBuddy - RECTWIDTH(&rcClient);
        rcWindow.right  += sizeDelta.cx;
    }
                          
    //  compute vertical placement
    if( pgbp->y >= 0 )  // fixed vert origin requested
        OffsetRect( &rcWindow, 0, pgbp->y - rcWindow.top );

    if( pgbp->cy >= 0 ) // fixed height requested
        rcWindow.bottom = rcWindow.top + pgbp->cy;
    else
    {
        if( pgbp->cyBuddy >= 0 ) // client height requested
            sizeDelta.cy = pgbp->cyBuddy - RECTHEIGHT(&rcClient);
        rcWindow.bottom += sizeDelta.cy;
    }

    if( pgbp->hdwp && (-1 != (LONG_PTR)pgbp->hdwp) )
        DeferWindowPos( pgbp->hdwp, _hwnd, NULL, rcWindow.left, rcWindow.top, 
                        RECTWIDTH( &rcWindow ), RECTHEIGHT( &rcWindow ),
                        dwFlags );
    else
        SetWindowPos( _hwnd, NULL, rcWindow.left, rcWindow.top, 
                      RECTWIDTH( &rcWindow ), RECTHEIGHT( &rcWindow ),
                      dwFlags );

    //  stuff resulting rects
    pgbp->rcWindow = rcWindow;
    return CalcClientRect( &rcWindow, &pgbp->rcBuddy );
}

//-------------------------------------------------------------------------//
BOOL CGroupBtn::SetBuddy( HWND hwnd, ULONG dwFlags )
{
    if( !IsWindow( hwnd ) )
        hwnd = NULL;

    if( hwnd && (dwFlags & (GBBF_HSLAVE|GBBF_VSLAVE)) )
    {
        //  subclass the buddy 
        _pfnBuddy = (WNDPROC)SetWindowLongPtr( hwnd, GWLP_WNDPROC, (LONG_PTR)BuddyProc );
        SetWindowLongPtr( hwnd, GWLP_USERDATA, (LONG_PTR)this );
    }
    else if( IsWindow( _hwndBuddy ) && _pfnBuddy )
    {
        SetWindowLongPtr( hwnd, GWLP_USERDATA, (LONG_PTR)NULL );
        SetWindowLongPtr( hwnd, GWLP_WNDPROC, (LONG_PTR)_pfnBuddy );
        _pfnBuddy = NULL;
    }

    _hwndBuddy = hwnd;
    _dwBuddyFlags = dwFlags;
    DoLayout( TRUE );

    return TRUE;
}

//-------------------------------------------------------------------------//
BOOL CGroupBtn::SetDropState( BOOL bDropped )
{
    _fDropped = bDropped;
    return TRUE;
}

//-------------------------------------------------------------------------//
//  WM_NCCREATE handler
BOOL CGroupBtn::NcCreate( LPCREATESTRUCT lpcs )
{
    //  assign user data
    SetWindowLongPtr( _hwnd, GWLP_USERDATA, (LONG_PTR)this );
    
    //  enforce window style bits
    lpcs->style     |= WS_CLIPCHILDREN|WS_CLIPSIBLINGS;
    lpcs->dwExStyle |= WS_EX_TRANSPARENT;
    SetWindowLong( _hwnd, GWL_STYLE, lpcs->style );
    SetWindowLong( _hwnd, GWL_EXSTYLE, lpcs->dwExStyle );

    //  enforce min height
    SetText( lpcs->lpszName );
    if( lpcs->cy < _sizeCaption.cy )
    { 
        lpcs->cy = _sizeCaption.cy;
        SetWindowPos( _hwnd, NULL, 0,0, lpcs->cx, lpcs->cy,
                      SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE );
    }
    return TRUE;
}

//-------------------------------------------------------------------------//
//  WM_NCCALCSIZE handler
LRESULT CGroupBtn::NcCalcSize( BOOL fCalcValidRects, LPNCCALCSIZE_PARAMS pnccs )
{
    LRESULT lRet = FALSE;
    RECT   rcClient;

    if( fCalcValidRects && CalcClientRect( &pnccs->rgrc[0], &rcClient ) )
    {
        pnccs->rgrc[1] = pnccs->rgrc[2];
        pnccs->rgrc[0] = pnccs->rgrc[2] = rcClient;
        return WVR_VALIDRECTS;
    }
    
    return lRet;
}

//-------------------------------------------------------------------------//
//  WM_NCPAINT handler
void CGroupBtn::NcPaint( HRGN hrgn )
{
    RECT    rcWindow;
    DWORD   dwStyle = GetWindowLong( _hwnd, GWL_STYLE );
    HDC     hdc;

    GetWindowRect( _hwnd, &rcWindow );
    OffsetRect( &rcWindow, -rcWindow.left, -rcWindow.top );

    if( (hdc = GetWindowDC( _hwnd )) != NULL )
    {
        if( dwStyle & WS_BORDER )
        {
            HBRUSH hbr = CreateSolidBrush( COLOR_WINDOWFRAME );
            FrameRect( hdc, &rcWindow, hbr );
            DeleteObject( hbr );
        }

        rcWindow.bottom = rcWindow.top + _sizeCaption.cy;

        SetBkColor( hdc, GetSysColor( COLOR_HIGHLIGHT ) );
        SetTextColor( hdc, GetSysColor( COLOR_HIGHLIGHTTEXT ) );

        ExtTextOut( hdc, rcWindow.left, rcWindow.top, 
                    ETO_OPAQUE, &rcWindow, NULL, 0, NULL );

        InflateRect( &rcWindow, -CAPTION_HPADDING, -(CAPTION_VPADDING/2) );
        HFONT hfPrev = (HFONT)SelectObject( hdc, GetFont() );
        ExtTextOut( hdc, rcWindow.left, rcWindow.top, 
                    ETO_OPAQUE, &rcWindow, _pszCaption, 
                    _pszCaption ? lstrlen(_pszCaption) : 0, NULL );
        SelectObject( hdc, hfPrev );

#ifdef KEYBOARDCUES
        if( 0 == (_fKeyboardCues & UISF_HIDEFOCUS) )
#endif
        {
            if( GetFocus() == _hwnd )
            {
                rcWindow.right = rcWindow.left + _sizeCaption.cx + 1;
                InflateRect( &rcWindow, 1, 0 );
                DrawFocusRect( hdc, &rcWindow );
            }
        }

        ReleaseDC( _hwnd, hdc );
    }
}

//-------------------------------------------------------------------------//
//  WM_NCMOUSEMOVE handler
//-------------------------------------------------------------------------//
LRESULT CGroupBtn::NcMouseMove( WPARAM nHittest, LONG x, LONG y )
{
    if( HTCAPTION == nHittest )
    {
        RECT  rc;
        POINT pt;
        GetWindowRect( _hwnd, &rc );
        rc.bottom = rc.top + _sizeCaption.cy;
        rc.right  = rc.left + _sizeCaption.cx;
        InflateRect( &rc, 0, -(CAPTION_VPADDING/2) );
        pt.x = x;
        pt.y = y;

        if( PtInRect( &rc, pt ) )
        {
            HCURSOR hc = GetHandCursor();
            if( hc != NULL )
            {
                SetCursor( hc );
                return 0L;
            }
        }
    }
    return DefWindowProc( _hwnd, WM_NCMOUSEMOVE, nHittest, MAKELPARAM( x, y ) );
}

//-------------------------------------------------------------------------//
//  WM_NCHITTEST handler
LRESULT CGroupBtn::NcHitTest( LONG x, LONG y )
{
    POINT pt;
    RECT  rc, rcClient;
    DWORD dwStyle = GetWindowLong( _hwnd, GWL_STYLE );
    
    pt.x = x;
    pt.y = y;

    GetWindowRect( _hwnd, &rc );
    CalcClientRect( &rc, &rcClient );

    if( PtInRect( &rcClient, pt ) )
        return HTTRANSPARENT;

    if( PtInRect( &rc, pt ) )
    {
        if( dwStyle & WS_BORDER )
        {
            if( pt.x == rc.left ||
                pt.x == rc.right ||
                pt.y == rc.bottom )
                return HTBORDER;
        }
        return HTCAPTION;
    }
    return HTNOWHERE;
}

//-------------------------------------------------------------------------//
LRESULT CGroupBtn::NcButtonDown( UINT nMsg, WPARAM nHittest, const POINTS& pts )
{
    LRESULT lRet = 0L;

    if( HTCAPTION == nHittest )
    {
        SetCursor( GetHandCursor() );
        MODIFY_CAPTURE( CF_SETCAPTURE, 0 );

        if( GetFocus() != _hwnd )
        {
            MODIFY_CAPTURE( CF_SETFOCUS, 0 );
            EnableNotifications( FALSE ); // so the host doesn't reposition the link.
            SetFocus( _hwnd );
            EnableNotifications( TRUE );
        }

        SetCapture( _hwnd );
    }
    else
        lRet = DefWindowProc( _hwnd, nMsg, nHittest, MAKELONG(pts.x, pts.y) );
        
    return lRet;
}

//-------------------------------------------------------------------------//
LRESULT CGroupBtn::ButtonUp( UINT nMsg, WPARAM nHittest, const POINTS& pts )
{
    if( TEST_CAPTURE(CF_SETCAPTURE) )
    {
        ReleaseCapture();
        MODIFY_CAPTURE( 0, CF_SETCAPTURE );

        POINT ptScrn;
        ptScrn.x = pts.x;
        ptScrn.y = pts.y;
        MapWindowPoints( _hwnd, HWND_DESKTOP, &ptScrn, 1 );

        LRESULT nHittest = SendMessage( _hwnd, WM_NCHITTEST, 0, MAKELONG( ptScrn.x, ptScrn.y ) );

        if( HTCAPTION == nHittest )
        {
            switch( nMsg )
            {
            case WM_LBUTTONUP:
                SendNotify( NM_CLICK );
                break;
            case WM_RBUTTONUP:
                SendNotify( NM_RCLICK );
                break;
            }
        }
    }

    if( TEST_CAPTURE(CF_SETFOCUS) )
    {
        MODIFY_CAPTURE( 0, CF_SETFOCUS );
        if( GetFocus() == _hwnd ) // if we still have the focus...
            SendNotify( NM_SETFOCUS );
    }
    return 0L;
}

//-------------------------------------------------------------------------//
//  Non-client mouse click/dblclk handler
LRESULT CGroupBtn::NcDblClick( UINT nMsg, WPARAM nHittest, LPARAM lParam )
{
    LRESULT lRet = 0L;
    
    if( HTCAPTION == nHittest )
    {
        SetFocus( _hwnd );

        lRet = DefWindowProc( _hwnd, nMsg, HTCLIENT, lParam );

        switch( nMsg )
        {
            case WM_NCLBUTTONDBLCLK:
                SendNotify( NM_DBLCLK );
                break;
            case WM_NCRBUTTONDBLCLK:
                SendNotify( NM_RDBLCLK );
                break;
        }
    }
    else
        lRet = DefWindowProc( _hwnd, nMsg, nHittest, lParam );

    return lRet;
}

//-------------------------------------------------------------------------//
LONG CGroupBtn::EnableNotifications( BOOL bEnable )
{
    if( bEnable )
    {
        if( _cNotifyLocks > 0 )
            _cNotifyLocks--;
    }
    else
        _cNotifyLocks++;
    
    return _cNotifyLocks;
}

//-------------------------------------------------------------------------//
//  WM_NOTIFY transmit helper
LRESULT CGroupBtn::SendNotify( int nCode, IN OPTIONAL NMHDR* pnmh )
{
    if( 0 == _cNotifyLocks )
    {
        NMHDR hdr;
        if( NULL == pnmh )
            pnmh = &hdr; 

        pnmh->hwndFrom = _hwnd;
        pnmh->idFrom   = GetDlgCtrlID( _hwnd );
        pnmh->code     = nCode;
        return SendMessage( GetParent( _hwnd ), WM_NOTIFY, hdr.idFrom, (LPARAM)pnmh );
    }
    return 0;
}

//-------------------------------------------------------------------------//
//  WM_NOTIFY transmit helper
void CGroupBtn::PostNotify( int nCode )
{
    if( 0 == _cNotifyLocks )
        PostMessage( _hwnd, GBM_SENDNOTIFY, nCode, 0L );
}

//-------------------------------------------------------------------------//
LRESULT CGroupBtn::WndProc( HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam )
{
    CGroupBtn *pThis = (CGroupBtn*)GetWindowLongPtr( hwnd, GWLP_USERDATA );
    LRESULT lRet = 0;
    
    switch( nMsg )
    {
        case WM_NCHITTEST:
        {
            POINTS pts = MAKEPOINTS( lParam );
            return pThis->NcHitTest( pts.x, pts.y );
        }

        case WM_NCMOUSEMOVE:
        {
            POINTS pts = MAKEPOINTS( lParam );
            return pThis->NcMouseMove( wParam, pts.x, pts.y );
        }

        case WM_NCCALCSIZE:
            return pThis->NcCalcSize( (BOOL)wParam, (LPNCCALCSIZE_PARAMS)lParam );

        case WM_NCPAINT:
            pThis->NcPaint( (HRGN)wParam );
            return 0;

        case WM_WINDOWPOSCHANGING:
            return pThis->WindowPosChanging( (LPWINDOWPOS)lParam );

        case WM_SIZE:
        {
            POINTS pts = MAKEPOINTS( lParam );
            return pThis->OnSize( wParam, pts.x, pts.y );
        }

        case WM_DESTROY:
            if( IsWindow( pThis->_hwndBuddy ) )
                DestroyWindow( pThis->_hwndBuddy );
            break;

        case WM_ERASEBKGND:
            return TRUE; // transparent: no erase bkgnd

        case WM_NCLBUTTONDOWN:
        case WM_NCRBUTTONDOWN:
        {
            POINTS pts = MAKEPOINTS(lParam);
            return pThis->NcButtonDown( nMsg, wParam, pts );
        }

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        {
            POINTS pts = MAKEPOINTS(lParam);
            return pThis->ButtonUp( nMsg, wParam, pts );
        }

        case WM_NCLBUTTONDBLCLK:
        case WM_NCRBUTTONDBLCLK:
            return pThis->NcDblClick( nMsg, wParam, lParam );

        case WM_SHOWWINDOW:
            if( IsWindow( pThis->_hwndBuddy ) )
                ShowWindow( pThis->_hwndBuddy, wParam ? SW_SHOW : SW_HIDE );
            break;

        case WM_SETTEXT:
            pThis->SetText( (LPCTSTR)lParam );
            return TRUE;

        case WM_GETTEXT:
            return pThis->GetText( (LPTSTR)lParam, (int)wParam );

        case WM_SETFONT:
            pThis->SetFont( (HFONT)wParam );
            if( lParam /* fRedraw */)
                InvalidateRect( hwnd, NULL, TRUE );
            break;

        case WM_CAPTURECHANGED:
            if( lParam /* NULL if we called ReleaseCapture() */)
                pThis->OnCaptureLost( (HWND)lParam );
            break;

        case WM_SETFOCUS:
            pThis->NcPaint( (HRGN)1 );
            pThis->SendNotify( NM_SETFOCUS );
            break;
             
        case WM_KILLFOCUS:
            pThis->NcPaint( (HRGN)1 );
            pThis->SendNotify( NM_KILLFOCUS );
            break;

        case WM_GETDLGCODE:
        {
            MSG* pmsg;
            lRet = DLGC_BUTTON|DLGC_UNDEFPUSHBUTTON;

            if( (pmsg = (MSG*)lParam) )
            {
                if( (WM_KEYDOWN == pmsg->message || WM_KEYUP == pmsg->message) )
                {
                    switch( pmsg->wParam )
                    {
                    case VK_RETURN:
                    case VK_SPACE:
                        lRet |= DLGC_WANTALLKEYS;
                        break;
                    }
                }
                else if( WM_CHAR == pmsg->message && VK_RETURN == pmsg->wParam )
                {
                    //  Eat VK_RETURN WM_CHARs; we don't want
                    //  Dialog manager to beep when IsDialogMessage gets it.
                    return lRet |= DLGC_WANTMESSAGE;
                }
            }

            return lRet;
        }

        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_CHAR:
            switch( wParam )
            {
                case VK_RETURN:
                case VK_SPACE:
                    if( WM_KEYDOWN == nMsg )
                        pThis->SendNotify( NM_RETURN );
                    return 0L;
            }
            break;

#ifdef KEYBOARDCUES
        case WM_UPDATEUISTATE:
            if( _HandleWM_UPDATEUISTATE( wParam, lParam, &pThis->_fKeyboardCues ) )
                SendMessage( hwnd, WM_NCPAINT, 1, 0L );
            break;
#endif

        case GBM_SETPLACEMENT:
            if( lParam )
                return pThis->SetPlacement( (PGBPLACEMENT)lParam );
            return 0L;

        case GBM_SETDROPSTATE: // WPARAM: BOOL fDropped, LPARAM: n/a, return: BOOL
            return 0L;

        case GBM_GETDROPSTATE: // WPARAM: n/a, LPARAM: n/a, return: BOOL fDropped
            return 0L;

        case GBM_SENDNOTIFY:
            pThis->SendNotify( (int)wParam );
            break;

        case WM_NCCREATE:
        {
            LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
            if( (pThis = new CGroupBtn(hwnd) ) == NULL )
                break;
            
            if( !pThis->NcCreate( (LPCREATESTRUCT)lParam ) )
                return FALSE;
            break;
        }

        case WM_NCDESTROY:
            lRet = DefWindowProc( hwnd, nMsg, wParam, lParam );
            SetWindowPtr( hwnd, GWLP_USERDATA, NULL );
            pThis->_hwnd = NULL;
            pThis->Release();
            return lRet;

        case WM_CREATE:
#ifdef KEYBOARDCUES
            _InitializeUISTATE( hwnd, &pThis->_fKeyboardCues );
#endif//KEYBOARDCUES
            pThis->SetText( ((LPCREATESTRUCT)lParam)->lpszName );
            break;

        case GBM_SETBUDDY:     // WPARAM: HWND hwndBuddy, LPARAM: MAKELPARAM(cxMargin, cyMargin), return: BOOL
            return pThis->SetBuddy( (HWND)wParam, (ULONG)lParam );

        case GBM_GETBUDDY:     // WPARAM: n/a, LPARAM: n/a, return: HWND
            return (LRESULT)pThis->_hwndBuddy;

        default:
            // oleacc defs thunked for WINVER < 0x0500
            if( IsWM_GETOBJECT( nMsg ) && (OBJID_CLIENT == lParam || OBJID_TITLEBAR == lParam) ) 
                return LresultFromObject( IID_IAccessible, wParam, SAFECAST(pThis, IAccessible*) );

            break;
    }

    return DefWindowProc( hwnd, nMsg, wParam, lParam );
}
                                                    
//-------------------------------------------------------------------------//
LRESULT CGroupBtn::BuddyProc( HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam )
{
    CGroupBtn *pBtn = (CGroupBtn*)GetWindowLongPtr( hwnd, GWLP_USERDATA );
    ASSERT( pBtn );

    switch( nMsg )
    {
        case WM_SIZE:
        {
            LRESULT lRet = CallWindowProc( pBtn->_pfnBuddy, hwnd, nMsg, wParam, lParam );
            if( !pBtn->_fInLayout )
                pBtn->DoLayout();
            return lRet;
        }

        case WM_DESTROY:
        {
            WNDPROC pfn = pBtn->_pfnBuddy;
            pBtn->SetBuddy( NULL, 0 );
            return CallWindowProc( pfn, hwnd, nMsg, wParam, lParam );
        }
        break;

    }
    return pBtn->_pfnBuddy ? CallWindowProc( pBtn->_pfnBuddy, hwnd, nMsg, wParam, lParam ) :
           0L;
}

//-------------------------------------------------------------------------//
//  CAccessibleBase IUnknown impl
STDMETHODIMP CAccessibleBase::QueryInterface( REFIID riid, void** ppvObj )
{
    HRESULT hres;
    static const QITAB qit[] = 
    {
        QITABENT(CAccessibleBase, IDispatch),
        QITABENT(CAccessibleBase, IAccessible),
        QITABENT(CAccessibleBase, IOleWindow),
        { 0 },
    };
    hres = QISearch(this, (LPCQITAB)qit, riid, ppvObj);
    return hres;
}

STDMETHODIMP_(ULONG) CAccessibleBase::AddRef()
{
    return InterlockedIncrement( (LONG*)&_cRef );
}

STDMETHODIMP_(ULONG) CAccessibleBase::Release()
{
    ULONG cRef = InterlockedDecrement( (LONG*)&_cRef );
    if( cRef <= 0 )
    {
        DllRelease();
        delete this;
    }
    return cRef;
}

//-------------------------------------------------------------------------//
//  CAccessibleBase IOleWindow impl
STDMETHODIMP CAccessibleBase::GetWindow( HWND* phwnd )
{
    *phwnd = _hwnd;
    return IsWindow( _hwnd ) ? S_OK : S_FALSE;
}

//-------------------------------------------------------------------------//
//  CAccessibleBase IDispatch impl
//-------------------------------------------------------------------------//

static BOOL _accLoadTypeInfo( ITypeInfo** ppti )
{
    ITypeLib* ptl;
    HRESULT hr = LoadTypeLib(L"oleacc.dll", &ptl);

    if( SUCCEEDED( hr ) )
    {
        hr = ptl->GetTypeInfoOfGuid( IID_IAccessible, ppti );
        ATOMICRELEASE( ptl );
    }
    return hr;
}

STDMETHODIMP CAccessibleBase::GetTypeInfoCount( UINT * pctinfo ) 
{ 
    *pctinfo = 1;
    return S_OK;
}

STDMETHODIMP CAccessibleBase::GetTypeInfo( UINT itinfo, LCID lcid, ITypeInfo** pptinfo )
{ 
    HRESULT hr = E_FAIL;
    if( NULL == _ptiAcc && FAILED( (hr = _accLoadTypeInfo( &_ptiAcc )) ) )
        return hr;

    *pptinfo = _ptiAcc;
    (*pptinfo)->AddRef();
    return S_OK;
}

STDMETHODIMP CAccessibleBase::GetIDsOfNames( 
    REFIID riid, 
    OLECHAR** rgszNames, 
    UINT cNames,
    LCID lcid, DISPID * rgdispid )
{
    HRESULT hr = E_FAIL;

    if( IID_NULL != riid && IID_IAccessible != riid )
        return DISP_E_UNKNOWNINTERFACE;

    if( NULL == _ptiAcc && FAILED( (hr = _accLoadTypeInfo( &_ptiAcc )) ) )
        return hr;

    return _ptiAcc->GetIDsOfNames( rgszNames, cNames, rgdispid );
}

STDMETHODIMP CAccessibleBase::Invoke( 
    DISPID  dispidMember, 
    REFIID  riid, 
    LCID    lcid, 
    WORD    wFlags,
    DISPPARAMS * pdispparams, 
    VARIANT * pvarResult, 
    EXCEPINFO * pexcepinfo, 
    UINT * puArgErr )
{
    HRESULT hr = E_FAIL;
    if( IID_NULL != riid && IID_IAccessible != riid )
        return DISP_E_UNKNOWNINTERFACE;

    if( NULL == _ptiAcc && FAILED( (hr = _accLoadTypeInfo( &_ptiAcc )) ) )
        return hr;

    return _ptiAcc->Invoke( this, dispidMember, wFlags, pdispparams, 
                            pvarResult, pexcepinfo, puArgErr );
}

STDMETHODIMP CAccessibleBase::get_accParent( IDispatch ** ppdispParent )
{
    *ppdispParent = NULL;
    if( IsWindow(_hwnd) )
        return AccessibleObjectFromWindow( _hwnd, OBJID_WINDOW,
                                           IID_IDispatch, (void **)ppdispParent );
    return S_FALSE;
}

STDMETHODIMP CAccessibleBase::get_accChildCount( long * pcChildren )
{
    *pcChildren = 0;
    return S_OK;
}

STDMETHODIMP CAccessibleBase::get_accChild( VARIANT varChildIndex, IDispatch ** ppdispChild)
{
    *ppdispChild = NULL;
    return S_FALSE;
}

STDMETHODIMP CAccessibleBase::get_accValue( VARIANT varChild, BSTR* pbstrValue)
{
    VALIDATEACCCHILD( varChild, CHILDID_SELF, E_INVALIDARG );
    *pbstrValue = NULL;
    return S_FALSE;
}

STDMETHODIMP CAccessibleBase::get_accDescription( VARIANT varChild, BSTR * pbstrDescription)
{
    VALIDATEACCCHILD( varChild, CHILDID_SELF, E_INVALIDARG );
    *pbstrDescription = NULL;
    return S_FALSE;
}

STDMETHODIMP CAccessibleBase::get_accRole( VARIANT varChild, VARIANT *pvarRole )
{
    VALIDATEACCCHILD( varChild, CHILDID_SELF, E_INVALIDARG );
    pvarRole->vt    = VT_I4;
    pvarRole->lVal  = ROLE_SYSTEM_LINK;
    return S_OK;
}

STDMETHODIMP CAccessibleBase::get_accState( VARIANT varChild, VARIANT *pvarState )
{
    VALIDATEACCCHILD( varChild, CHILDID_SELF, E_INVALIDARG );

    pvarState->vt = VT_I4;
    pvarState->lVal = STATE_SYSTEM_DEFAULT ;

    if( GetFocus() == _hwnd )
        pvarState->lVal |= STATE_SYSTEM_FOCUSED;
    else if( IsWindowEnabled( _hwnd ) )
        pvarState->lVal |= STATE_SYSTEM_FOCUSABLE;

    if( !IsWindowVisible( _hwnd ) )
        pvarState->lVal |= STATE_SYSTEM_INVISIBLE;

    return S_OK;
}

STDMETHODIMP CAccessibleBase::get_accHelp( VARIANT varChild, BSTR* pbstrHelp )
{
    VALIDATEACCCHILD( varChild, CHILDID_SELF, E_INVALIDARG );
    *pbstrHelp = NULL;
    return S_FALSE;
}

STDMETHODIMP CAccessibleBase::get_accHelpTopic( BSTR* pbstrHelpFile, VARIANT varChild, long* pidTopic )
{
    VALIDATEACCCHILD( varChild, CHILDID_SELF, E_INVALIDARG );
    *pbstrHelpFile = NULL;
    *pidTopic    = -1;
    return S_FALSE;
}

STDMETHODIMP CAccessibleBase::get_accKeyboardShortcut( VARIANT varChild, BSTR* pbstrKeyboardShortcut )
{
    VALIDATEACCCHILD( varChild, CHILDID_SELF, E_INVALIDARG );
    *pbstrKeyboardShortcut = NULL;
    return S_FALSE;
}

STDMETHODIMP CAccessibleBase::get_accFocus( VARIANT FAR * pvarFocusChild )
{
    HWND hwndFocus;
    if( (hwndFocus = GetFocus()) == _hwnd || IsChild( _hwnd, hwndFocus ) )
    {
        pvarFocusChild->vt = VT_I4;
        pvarFocusChild->lVal = CHILDID_SELF;
        return S_OK;
    }
    return S_FALSE;
}

STDMETHODIMP CAccessibleBase::get_accSelection( VARIANT FAR * pvarSelectedChildren )
{
    return get_accFocus( pvarSelectedChildren );  // implemented same as focus.
}

STDMETHODIMP CAccessibleBase::get_accDefaultAction( VARIANT varChild, BSTR* pbstrDefaultAction )
{
    VALIDATEACCCHILD( varChild, CHILDID_SELF, E_INVALIDARG );

    WCHAR wsz[128];
    if( LoadStringW( HINST_THISDLL, GetDefaultActionStringID(), wsz, ARRAYSIZE(wsz) ) )
    {
        if( NULL == (*pbstrDefaultAction = SysAllocString( wsz )) )
            return E_OUTOFMEMORY;
        return S_OK;
    }
    return E_FAIL;
}

STDMETHODIMP CAccessibleBase::accSelect( long flagsSelect, VARIANT varChild )
{
    VALIDATEACCCHILD( varChild, CHILDID_SELF, E_INVALIDARG );

    if( flagsSelect & SELFLAG_TAKEFOCUS )
    {
        SetFocus( _hwnd );
        return S_OK;
    }
    return S_FALSE;
}

STDMETHODIMP CAccessibleBase::accLocation( long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varChild )
{
    RECT rc;
    GetWindowRect( _hwnd, &rc );
    *pxLeft = rc.left;
    *pyTop  = rc.top;
    *pcxWidth  = RECTWIDTH(&rc);
    *pcyHeight = RECTHEIGHT(&rc);

    varChild.vt = VT_I4;
    varChild.lVal = CHILDID_SELF;
    return S_OK;
}

STDMETHODIMP CAccessibleBase::accNavigate( long navDir, VARIANT varStart, VARIANT * pvarEndUpAt )
{
    return S_FALSE;
}

STDMETHODIMP CAccessibleBase::accHitTest( long xLeft, long yTop, VARIANT * pvarChildAtPoint )
{
    pvarChildAtPoint->vt   = VT_I4;
    pvarChildAtPoint->lVal = CHILDID_SELF;
    return S_OK;
}

STDMETHODIMP CAccessibleBase::put_accName( VARIANT varChild, BSTR bstrName )
{
    VALIDATEACCCHILD( varChild, CHILDID_SELF, E_INVALIDARG );
    return S_FALSE;
}

STDMETHODIMP CAccessibleBase::put_accValue( VARIANT varChild, BSTR bstrValue )
{
    VALIDATEACCCHILD( varChild, CHILDID_SELF, E_INVALIDARG );
    return S_FALSE;
}

#ifdef KEYBOARDCUES

//-------------------------------------------------------------------------//
//  KEYBOARDCUES helpes
BOOL _HandleWM_UPDATEUISTATE( 
    IN WPARAM wParam, 
    IN LPARAM lParam, 
    IN OUT UINT* puFlags )
{
    UINT uFlags = *puFlags;

    switch( LOWORD(wParam) )
    {
    case UIS_CLEAR:
        *puFlags &= ~(HIWORD(wParam));
        break;

    case UIS_SET:
        *puFlags |= HIWORD(wParam);
        break;
    }

    return uFlags != *puFlags;
}

void _InitializeUISTATE( IN HWND hwnd, IN OUT UINT* puFlags )
{
    HWND hwndParent = GetParent( hwnd );
    *puFlags = (UINT)SendMessage( hwndParent, WM_QUERYUISTATE, 0, 0L );
}

#endif//KEYBOARDCUES
