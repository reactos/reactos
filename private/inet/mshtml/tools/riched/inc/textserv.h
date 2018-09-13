/*	@doc EXTERNAL
 *
 *	@module TEXTSRV.H  Text Service Interface |
 *	
 *	Define interfaces between the Text Services component and the host
 *
 *	Original Author: <nl>
 *		Christian Fortini
 *
 *	History: <nl>
 *		8/1/95	ricksa	Revised interface definition
 */

#ifndef _TEXTSERV_H
#define _TEXTSERV_H

// BUGBUG: We need to get a definition for this
interface IUndoActionManager;

EXTERN_C const IID IID_ITextServices;
EXTERN_C const IID IID_ITextHost;
EXTERN_C const IID IID_ITextHost2;

// BUGBUG: Need to figure out correct public place for this error.
// Note: error code is first outside of range reserved for OLE.
#define S_MSG_KEY_IGNORED \
	MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_ITF, 0x201)

// Enums used by property methods

/*
 *	TXTBACKSTYLE
 *
 *	@enum	Defines different background styles control
 */
enum TXTBACKSTYLE {
	TXTBACK_TRANSPARENT = 0,		//@emem	background should show through
	TXTBACK_OPAQUE,					//@emem	erase background
};


/*
 *	TXTHITRESULT
 *
 *	@enum	Defines different hitresults
 */
enum TXTHITRESULT {
	TXTHITRESULT_NOHIT	        = 0,	//@emem	no hit
	TXTHITRESULT_TRANSPARENT	= 1,	//@emem point is within the text's rectangle, but 
										//in a transparent region
	TXTHITRESULT_CLOSE	        = 2,	//@emem	point is close to the text
	TXTHITRESULT_HIT	        = 3		//@emem dead-on hit
};

/*
 *	TXTNATURALSIZE
 *
 *	@enum	useful values for TxGetNaturalSize.
 *
  *	@xref <mf CTxtEdit::TxGetNaturalSize>
 */
enum TXTNATURALSIZE {
    TXTNS_FITTOCONTENT		= 1,		//@emem	Get a size that fits the content
    TXTNS_ROUNDTOLINE		= 2			//@emem Round to the nearest whole line.
};

/*
 *	TXTVIEW
 *
 *	@enum	useful values for TxDraw lViewId parameter
 *
  *	@xref <mf CTxtEdit::TxDraw>
 */
enum TXTVIEW { 
	TXTVIEW_ACTIVE = 0,
	TXTVIEW_INACTIVE = -1
};


/*
 *	CHANGETYPE
 *
 *	@enum	used for CHANGENOTIFY.dwChangeType; indicates what happened 
 *			for a particular change.
 */
enum CHANGETYPE
{
	CN_GENERIC		= 0,				//@emem Nothing special happened
	CN_TEXTCHANGED	= 1,				//@emem the text changed
	CN_NEWUNDO		= 2,				//@emem	A new undo action was added
	CN_NEWREDO		= 4					//@emem A new redo action was added
};

/* 
 *	@struct CHANGENOTIFY  |
 *
 *	passed during an EN_CHANGE notification; contains information about
 *	what actually happened for a change.
 */
struct CHANGENOTIFY {
	DWORD	dwChangeType;				//@field TEXT changed, etc
	DWORD	dwUndoCookie; 				//@field cookie for the undo action 
										// associated with the change.
};

// The TxGetPropertyBits and OnTxPropertyBitsChange methods can pass the following bits:

// NB!!! Do NOT rely on the ordering of these bits yet; the are subject
// to change.
#define TXTBIT_RICHTEXT			1		// rich-text control
#define TXTBIT_MULTILINE		2		// single vs multi-line control
#define TXTBIT_READONLY			4		// read only text
#define TXTBIT_SHOWACCELERATOR	8		// underline accelerator character
#define TXTBIT_USEPASSWORD		0x10	// use password char to display text
#define TXTBIT_HIDESELECTION	0x20	// show selection when inactive
#define TXTBIT_SAVESELECTION	0x40	// remember selection when inactive
#define TXTBIT_AUTOWORDSEL		0x80	// auto-word selection 
#define TXTBIT_VERTICAL			0x100	// vertical 
#define TXTBIT_SELBARCHANGE 	0x200	// notification that the selection bar width 
										// has changed.
										// FUTURE: move this bit to the end to
										// maintain the division between 
										// properties and notifications.
#define TXTBIT_WORDWRAP  		0x400	// if set, then multi-line controls
										// should wrap words to fit the available
										// display
#define	TXTBIT_ALLOWBEEP		0x800	// enable/disable beeping
#define TXTBIT_DISABLEDRAG      0x1000  // disable/enable dragging
#define TXTBIT_VIEWINSETCHANGE	0x2000	// the inset changed
#define TXTBIT_BACKSTYLECHANGE	0x4000 
#define TXTBIT_MAXLENGTHCHANGE	0x8000
#define TXTBIT_SCROLLBARCHANGE	0x10000
#define TXTBIT_CHARFORMATCHANGE 0x20000
#define TXTBIT_PARAFORMATCHANGE	0x40000
#define TXTBIT_EXTENTCHANGE		0x80000
#define TXTBIT_CLIENTRECTCHANGE	0x100000	// the client rectangle changed



/*
 *	ITextServices
 *	
 * 	@class	An interface extending Microsoft's Text Object Model to provide
 *			extra functionality for windowless operation.  In conjunction
 *			with ITextHost, ITextServices provides the means by which the
 *			the RichEdit control can be used *without* creating a window.
 *
 *	@base	public | IUnknown
 */
class ITextServices : public IUnknown
{
public:

	//@cmember Generic Send Message interface
	virtual HRESULT 	TxSendMessage(
							UINT msg, 
							WPARAM wparam, 
							LPARAM lparam,
							LRESULT *plresult) = 0;
	
	//@cmember Rendering
	virtual HRESULT		TxDraw(	
							DWORD dwDrawAspect,		
							LONG  lindex,			
							void * pvAspect,		 
							DVTARGETDEVICE * ptd,									
							HDC hdcDraw,			
							HDC hicTargetDev,		 
							LPCRECTL lprcBounds,	
							LPCRECTL lprcWBounds,	
               				LPRECT lprcUpdate,		
							BOOL (CALLBACK * pfnContinue) (DWORD), 
							DWORD dwContinue,
							LONG lViewId) = 0;	

	//@cmember Horizontal scrollbar support
	virtual HRESULT		TxGetHScroll(
							LONG *plMin, 
							LONG *plMax, 
							LONG *plPos, 
							LONG *plPage,
							BOOL * pfEnabled ) = 0;

   	//@cmember Horizontal scrollbar support
	virtual HRESULT		TxGetVScroll(
							LONG *plMin, 
							LONG *plMax, 
							LONG *plPos, 
							LONG *plPage, 
							BOOL * pfEnabled ) = 0;

	//@cmember Setcursor
	virtual HRESULT 	OnTxSetCursor(
							DWORD dwDrawAspect,		
							LONG  lindex,			
							void * pvAspect,		 
							DVTARGETDEVICE * ptd,									
							HDC hdcDraw,			
							HDC hicTargetDev,		 
							LPCRECT lprcClient, 
							INT x, 
							INT y) = 0;

	//@cmember Hit-test
	virtual HRESULT 	TxQueryHitPoint(
							DWORD dwDrawAspect,		
							LONG  lindex,			
							void * pvAspect,		 
							DVTARGETDEVICE * ptd,									
							HDC hdcDraw,			
							HDC hicTargetDev,		 
							LPCRECT lprcClient, 
							INT x, 
							INT y, 
							DWORD * pHitResult) = 0;

	//@cmember Inplace activate notification
	virtual HRESULT		OnTxInPlaceActivate(LPCRECT prcClient) = 0;

	//@cmember Inplace deactivate notification
	virtual HRESULT		OnTxInPlaceDeactivate() = 0;

	//@cmember UI activate notification
	virtual HRESULT		OnTxUIActivate() = 0;

	//@cmember UI deactivate notification
	virtual HRESULT		OnTxUIDeactivate() = 0;

	//@cmember Get text in control
	virtual HRESULT		TxGetText(BSTR *pbstrText) = 0;

	//@cmember Set text in control
	virtual HRESULT		TxSetText(LPCWSTR pszText) = 0;
	
	//@cmember Get x position of 
	virtual HRESULT		TxGetCurTargetX(LONG *) = 0;
	//@cmember Get baseline position
	virtual HRESULT		TxGetBaseLinePos(LONG *) = 0;

	//@cmember Get Size to fit / Natural size
	virtual HRESULT		TxGetNaturalSize(
							DWORD dwAspect,
							HDC hdcDraw,
							HDC hicTargetDev,
							DVTARGETDEVICE *ptd,
							DWORD dwMode, 	
							const SIZEL *psizelExtent,
							LONG *pwidth, 
							LONG *pheight) = 0;

	//@cmember Drag & drop
	virtual HRESULT		TxGetDropTarget( IDropTarget **ppDropTarget ) = 0;

	//@cmember Bulk bit property change notifications
	virtual HRESULT		OnTxPropertyBitsChange(DWORD dwMask, DWORD dwBits) = 0;

	//@cmember Fetch the cached drawing size 
	virtual	HRESULT		TxGetCachedSize(DWORD *pdwWidth, DWORD *pdwHeight)=0;
};


/*
 *	ITextHost
 *	
 * 	@class	Interface to be used by text services to obtain text host services
 *
 *	@base	public | IUnknown 
 */
class ITextHost : public IUnknown
{
public:

	//@cmember Get the DC for the host
	virtual HDC 		TxGetDC() = 0;

	//@cmember Release the DC gotten from the host
	virtual INT			TxReleaseDC(HDC hdc) = 0;
	
	//@cmember Show the scroll bar
	virtual BOOL 		TxShowScrollBar(INT fnBar, BOOL fShow) = 0;

	//@cmember Enable the scroll bar
	virtual BOOL 		TxEnableScrollBar (INT fuSBFlags, INT fuArrowflags) = 0;

	//@cmember Set the scroll range
	virtual BOOL 		TxSetScrollRange(
							INT fnBar, 
							LONG nMinPos, 
							INT nMaxPos, 
							BOOL fRedraw) = 0;

	//@cmember Set the scroll position
	virtual BOOL 		TxSetScrollPos (INT fnBar, INT nPos, BOOL fRedraw) = 0;

	//@cmember InvalidateRect
	virtual void		TxInvalidateRect(LPCRECT prc, BOOL fMode) = 0;

	//@cmember Send a WM_PAINT to the window
	virtual void 		TxViewChange(BOOL fUpdate) = 0;
	
	//@cmember Create the caret
	virtual BOOL		TxCreateCaret(HBITMAP hbmp, INT xWidth, INT yHeight) = 0;

	//@cmember Show the caret
	virtual BOOL		TxShowCaret(BOOL fShow) = 0;

	//@cmember Set the caret position
	virtual BOOL		TxSetCaretPos(INT x, INT y) = 0;

	//@cmember Create a timer with the specified timeout
	virtual BOOL 		TxSetTimer(UINT idTimer, UINT uTimeout) = 0;

	//@cmember Destroy a timer
	virtual void 		TxKillTimer(UINT idTimer) = 0;

	//@cmember Scroll the content of the specified window's client area
	virtual void		TxScrollWindowEx (
							INT dx, 
							INT dy, 
							LPCRECT lprcScroll, 
							LPCRECT lprcClip,
							HRGN hrgnUpdate, 
							LPRECT lprcUpdate, 
							UINT fuScroll) = 0;
	
	//@cmember Get mouse capture
	virtual void		TxSetCapture(BOOL fCapture) = 0;

	//@cmember Set the focus to the text window
	virtual void		TxSetFocus() = 0;

	//@cmember Establish a new cursor shape
	virtual void 		TxSetCursor(HCURSOR hcur, BOOL fText) = 0;

	//@cmember Converts screen coordinates of a specified point to the client coordinates 
	virtual BOOL 		TxScreenToClient (LPPOINT lppt) = 0;

	//@cmember Converts the client coordinates of a specified point to screen coordinates
	virtual BOOL		TxClientToScreen (LPPOINT lppt) = 0;

	//@cmember Request host to activate text services
	virtual HRESULT		TxActivate( LONG * plOldState ) = 0;

	//@cmember Request host to deactivate text services
   	virtual HRESULT		TxDeactivate( LONG lNewState ) = 0;

	//@cmember Retrieves the coordinates of a window's client area
	virtual HRESULT		TxGetClientRect(LPRECT prc) = 0;

	//@cmember Get the view rectangle relative to the inset
	virtual HRESULT		TxGetViewInset(LPRECT prc) = 0;

	//@cmember Get the default character format for the text
	virtual HRESULT 	TxGetCharFormat(const CHARFORMATW **ppCF ) = 0;

	//@cmember Get the default paragraph format for the text
	virtual HRESULT		TxGetParaFormat(const PARAFORMAT **ppPF) = 0;

	//@cmember Get the background color for the window
	virtual COLORREF	TxGetSysColor(int nIndex) = 0;

	//@cmember Get the background (either opaque or transparent)
	virtual HRESULT		TxGetBackStyle(TXTBACKSTYLE *pstyle) = 0;

	//@cmember Get the maximum length for the text
	virtual HRESULT		TxGetMaxLength(DWORD *plength) = 0;

	//@cmember Get the bits representing requested scroll bars for the window
	virtual HRESULT		TxGetScrollBars(DWORD *pdwScrollBar) = 0;

	//@cmember Get the character to display for password input
	virtual HRESULT		TxGetPasswordChar(TCHAR *pch) = 0;

	//@cmember Get the accelerator character
	virtual HRESULT		TxGetAcceleratorPos(LONG *pcp) = 0;

	//@cmember Get the native size
    virtual HRESULT		TxGetExtent(LPSIZEL lpExtent) = 0;
 
	//@cmember Notify host that default character format has changed
	virtual HRESULT 	OnTxCharFormatChange (const CHARFORMATW * pcf) = 0;

	//@cmember Notify host that default paragraph format has changed
	virtual HRESULT		OnTxParaFormatChange (const PARAFORMAT * ppf) = 0;

	//@cmember Bulk access to bit properties
	virtual HRESULT		TxGetPropertyBits(DWORD dwMask, DWORD *pdwBits) = 0;

	//@cmember Notify host of events
	virtual HRESULT		TxNotify(DWORD iNotify, void *pv) = 0;

	// Far East Methods for getting the Input Context
//#ifdef WIN95_IME
	virtual HIMC		TxImmGetContext() = 0;
	virtual void		TxImmReleaseContext( HIMC himc ) = 0;
//#endif

	//@cmember Returns HIMETRIC size of the control bar.
	virtual HRESULT		TxGetSelectionBarWidth (LONG *lSelBarWidth) = 0;

};

/*
 *	class ITextHost2
 *
 *	@class	An optional extension to ITextHost which provides functionality
 *			necessary to allow TextServices to embed OLE objects
 */
class ITextHost2 : public ITextHost
{
public:					//@cmember Is a double click in the message queue?
	virtual BOOL		TxIsDoubleClickPending() = 0; 
						//@cmember Get the overall window for this control	 
	virtual HRESULT		TxGetWindow(HWND *phwnd) = 0;
						//@cmember Set control window to foreground
	virtual HRESULT		TxSetForegroundWindow() = 0;
						//@cmember Set control window to foreground
	virtual HPALETTE	TxGetPalette() = 0;
};

	
//+-----------------------------------------------------------------------
// 	Factories
//------------------------------------------------------------------------

// Text Services factory
STDAPI CreateTextServices(
	IUnknown *punkOuter,
	ITextHost *pITextHost, 
	IUnknown **ppUnk);

typedef HRESULT (STDAPICALLTYPE * PCreateTextServices)(
	IUnknown *punkOuter,
	ITextHost *pITextHost, 
	IUnknown **ppUnk);

#endif // _TEXTSERV_H
