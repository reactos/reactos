
#ifndef __PREVIEWWND_H_
#define __PREVIEWWND_H_

#include "resource.h"       // main symbols
#include "ZoomWnd.h"
#include <wininet.h>

// forward declaration
class CPreview;

// messages for the Preview Window
#define IV_SETBITMAP    (WM_USER+99)
#define IV_SETIMGCTX    (WM_USER+100)   // PRIVATE
#define IV_SHOWFILEA    (WM_USER+101)
#define IV_SHOWFILEW    (WM_USER+102)
#define IV_ZOOM         (WM_USER+103)
#define IV_SCROLL       (WM_USER+104)
#define IV_BESTFIT      (WM_USER+105)
#define IV_ACTUALSIZE   (WM_USER+106)
#define IV_SETOPTIONS   (WM_USER+107)
#define IV_GETOPTIONS   (WM_USER+108)

#ifdef UNICODE
#define IV_SHOWFILE IV_SHOWFILEW
#else
#define IV_SHOWFILE IV_SHOWFILEA
#endif

// IV_SCROLL message parameters
#define IVS_LEFT        (SB_LEFT)
#define IVS_RIGHT       (SB_RIGHT)
#define IVS_LINELEFT    (SB_LINELEFT)
#define IVS_LINERIGHT   (SB_LINERIGHT)
#define IVS_PAGELEFT    (SB_PAGELEFT)
#define IVS_PAGERIGHT   (SB_PAGERIGHT)
#define IVS_UP          (SB_LEFT<<16)
#define IVS_DOWN        (SB_RIGHT<<16)
#define IVS_LINEUP      (SB_LINELEFT<<16)
#define IVS_LINEDOWN    (SB_LINERIGHT<<16)
#define IVS_PAGEUP      (SB_PAGELEFT<<16)
#define IVS_PAGEDOWN    (SB_PAGERIGHT<<16)

// IV_ZOOM messages
#define IVZ_CENTER  0
#define IVZ_POINT   1
#define IVZ_RECT    2
#define IVZ_ZOOMIN  0x00000000
#define IVZ_ZOOMOUT 0x00010000

// IV_SETOPTIONS and IV_GETOPTIONS messages
#define IVO_TOOLBAR         0
#define IVO_PRINTBTN        1
#define IVO_FULLSCREENBTN   2
#define IVO_CONTEXTMENU     3
#define IVO_PRINTABLE       4
#define IVO_ALLOWGOONLINE   5


// some typedefs for WinInet functions
typedef BOOL (* IQOFN)(HINTERNET, DWORD, LPVOID, LPDWORD);
typedef BOOL (* IGCSFN)(LPDWORD, DWORD);


/////////////////////////////////////////////////////////////////////////////
// CPreviewWnd
class CPreviewWnd :
	public CWindowImpl<CPreviewWnd>
{
public:
    CContainedWindow m_ctlToolbar;
    CZoomWnd         m_ctlPreview;

    CPreviewWnd *    m_pcwndDetachedPreview;

    CPreviewWnd(bool bShowToolbar=true);
    CPreviewWnd(CPreviewWnd & other);
    ~CPreviewWnd();

    void SetNotify( CPreview * pControl );
    BOOL GetPrintable( );
    int  TranslateAccelerator( LPMSG lpmsg );
    void StatusUpdate( int iStatus );   // used to set m_ctlPreview.m_iStrID to display correct status message

    DECLARE_WND_CLASS( TEXT("ShImgVw:CPreviewWnd") );

BEGIN_MSG_MAP(CPreviewWnd)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    COMMAND_RANGE_HANDLER(ID_ZOOMINCMD, ID_PRINTCMD, OnToolbarCommand)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
    NOTIFY_CODE_HANDLER(TTN_NEEDTEXT, OnNeedText)
    MESSAGE_HANDLER(WM_MOUSEWHEEL, OnWheelTurn)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
    MESSAGE_HANDLER(IV_SETBITMAP, IV_OnSetBitmap)
    MESSAGE_HANDLER(IV_SETIMGCTX, IV_OnSetImgCtx)
    MESSAGE_HANDLER(IV_SHOWFILEA, IV_OnShowFileA)
    MESSAGE_HANDLER(IV_SHOWFILEW, IV_OnShowFileW)
    MESSAGE_HANDLER(IV_ZOOM, IV_OnZoom)
    MESSAGE_HANDLER(IV_SCROLL, IV_OnIVScroll)
    MESSAGE_HANDLER(IV_BESTFIT, IV_OnBestFit)
    MESSAGE_HANDLER(IV_ACTUALSIZE, IV_OnActualSize)
    MESSAGE_HANDLER(IV_SETOPTIONS, IV_OnSetOptions)
ALT_MSG_MAP(1)
    // messages for the toolbar
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseToolbar)
    MESSAGE_HANDLER(WM_KEYDOWN, OnKeyEvent)
    MESSAGE_HANDLER(WM_KEYUP, OnKeyEvent)
END_MSG_MAP()

protected:
    /////////////////////////////////////////////////////////////////////////
    // Preview Window Message Handlers (PV_*)
    //
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnWheelTurn(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnNeedText(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnToolbarCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    // Image generation message handlers and functions
    LRESULT IV_OnSetBitmap(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT IV_OnShowFileA(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT IV_OnShowFileW(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT IV_OnZoom(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT IV_OnIVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT IV_OnBestFit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT IV_OnActualSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT IV_OnSetImgCtx(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
public:
    LRESULT IV_OnSetOptions(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
protected:

    // Toolbar message handlers (both toolbars)
    LRESULT OnEraseToolbar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnKeyEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    bool CreateToolbar();
    TCHAR * GetFSPTitle( TCHAR * szTitle, int cchMax );
    LRESULT OnShowFile(LPTSTR szFilename, WPARAM iCount);
    HRESULT PreviewFromFile(LPCTSTR bstrFilename, INT * iResultReturnCode);
    void FlushBitmapMessages();

    BOOL m_fHidePrintBtn;
    BOOL m_fHideFullscreen;
    BOOL m_fAllowContextMenu;
    BOOL m_fAllowGoOnline;      // if true, we'll go "online" (i.e. dial the modem) if needed, defaults to FALSE
    BOOL m_fShowToolbar;
    BOOL m_fPrintable;      // Stupid printing crap that LarryE wanted
    BOOL m_fOwnsHandles;    // if true then we own m_hbitmap and should delete it when no longer used
    
    HACCEL      m_haccel;
    CPreview *  m_pControl;  // pointer to our parent control.  NULL if we aren't running as a control.

    LPTSTR      m_pszFilename;  // The filename
    HBITMAP     m_hbitmap;      // The bitmap handle if the user set this directly
    IImgCtx *   m_pImgCtx;      // Used when decoding the interface
    IImgCtx *   m_pImgCtxReady; // Used once the interface is already decoded
    
    HPALETTE    m_hpal;         // the palette to use if in palette mode.
    HINSTANCE   m_hWininet;
    IQOFN       m_pfnInternetQueryOptionA;
    IGCSFN      m_pfnInternetGetConnectedState;

};

#endif
