

#define __ASM_STDCALL_FUNC(name,args,code)

#define typeof(X_) __typeof_ ## X_

struct HDC__;
struct ITextHost;
struct tagRECT;
struct _RECTL;
struct _charformatw;
struct _paraformat;
struct tagPOINT;
enum _TXTBACKSTYLE;
struct tagSIZE;
struct ITextServices;
struct tagDVTARGETDEVICE;
struct IDropTarget;

#define WINAPI __stdcall
#define HRESULT int
#define HDC struct HDC__*
#define BOOL int
#define HIMC void*
#define ITextHost struct ITextHost
#define INT int
#define UINT unsigned int
#define HBITMAP void*
#define LPCRECT const struct tagRECT *
#define LPRECT struct tagRECT *
#define LPCRECTL const struct _RECTL*
#define CHARFORMATW struct _charformatw
#define PARAFORMAT struct _paraformat
#define DWORD unsigned int /* HACK */
#define COLORREF DWORD
#define LONG int /* HACK */
#define WPARAM unsigned long
#define LPARAM long
#define HRGN void*
#define HCURSOR void*
#define LPPOINT struct tagPOINT*
#define TXTBACKSTYLE enum _TXTBACKSTYLE
#define WCHAR unsigned short
#define BSTR WCHAR*
#define LPCWSTR const WCHAR *
#define SIZEL struct tagSIZE
#define LPSIZEL struct tagSIZE*
#define ITextServices struct ITextServices
#define LRESULT long
#define DVTARGETDEVICE struct tagDVTARGETDEVICE

typedef HDC (WINAPI typeof(ITextHostImpl_TxGetDC))(ITextHost * iface);
typedef int (WINAPI typeof(ITextHostImpl_TxReleaseDC))(ITextHost *iface,HDC hdc);
typedef BOOL (WINAPI typeof(ITextHostImpl_TxShowScrollBar))(ITextHost *iface,INT fnBar,BOOL fShow);
typedef BOOL (WINAPI typeof(ITextHostImpl_TxEnableScrollBar))(ITextHost *iface,INT fuSBFlags,INT fuArrowflags);
typedef BOOL (WINAPI typeof(ITextHostImpl_TxSetScrollRange))(ITextHost *iface,INT fnBar,LONG nMinPos,INT nMaxPos,BOOL fRedraw);
typedef BOOL (WINAPI typeof(ITextHostImpl_TxSetScrollPos))(ITextHost *iface,INT fnBar,INT nPos,BOOL fRedraw);
typedef void (WINAPI typeof(ITextHostImpl_TxInvalidateRect))(ITextHost *iface,LPCRECT prc,BOOL fMode);
typedef void (WINAPI typeof(ITextHostImpl_TxViewChange))(ITextHost *iface,BOOL fUpdate);
typedef BOOL (WINAPI typeof(ITextHostImpl_TxCreateCaret))(ITextHost *iface,HBITMAP hbmp,INT xWidth, INT yHeight);
typedef BOOL (WINAPI typeof(ITextHostImpl_TxShowCaret))(ITextHost *iface, BOOL fShow);
typedef BOOL (WINAPI typeof(ITextHostImpl_TxSetCaretPos))(ITextHost *iface,INT x, INT y);
typedef BOOL (WINAPI typeof(ITextHostImpl_TxSetTimer))(ITextHost *iface,UINT idTimer, UINT uTimeout);
typedef void (WINAPI typeof(ITextHostImpl_TxKillTimer))(ITextHost *iface,UINT idTimer);
typedef void (WINAPI typeof(ITextHostImpl_TxScrollWindowEx))(ITextHost *iface,INT dx, INT dy,LPCRECT lprcScroll,LPCRECT lprcClip,HRGN hRgnUpdate,LPRECT lprcUpdate,UINT fuScroll);
typedef void (WINAPI typeof(ITextHostImpl_TxSetCapture))(ITextHost *iface,BOOL fCapture);
typedef void (WINAPI typeof(ITextHostImpl_TxSetFocus))(ITextHost *iface);
typedef void (WINAPI typeof(ITextHostImpl_TxSetCursor))(ITextHost *iface,HCURSOR hcur,BOOL fText);
typedef BOOL (WINAPI typeof(ITextHostImpl_TxScreenToClient))(ITextHost *iface,LPPOINT lppt);
typedef BOOL (WINAPI typeof(ITextHostImpl_TxClientToScreen))(ITextHost *iface,LPPOINT lppt);
typedef HRESULT (WINAPI typeof(ITextHostImpl_TxActivate))(ITextHost *iface,LONG *plOldState);
typedef HRESULT (WINAPI typeof(ITextHostImpl_TxDeactivate))(ITextHost *iface,LONG lNewState);
typedef HRESULT (WINAPI typeof(ITextHostImpl_TxGetClientRect))(ITextHost *iface,LPRECT prc);
typedef HRESULT (WINAPI typeof(ITextHostImpl_TxGetViewInset))(ITextHost *iface,LPRECT prc);
typedef HRESULT (WINAPI typeof(ITextHostImpl_TxGetCharFormat))(ITextHost *iface,const CHARFORMATW **ppCF);
typedef HRESULT (WINAPI typeof(ITextHostImpl_TxGetParaFormat))(ITextHost *iface,const PARAFORMAT **ppPF);
typedef COLORREF (WINAPI typeof(ITextHostImpl_TxGetSysColor))(ITextHost *iface,int nIndex);
typedef HRESULT (WINAPI typeof(ITextHostImpl_TxGetBackStyle))(ITextHost *iface,TXTBACKSTYLE *pStyle);
typedef HRESULT (WINAPI typeof(ITextHostImpl_TxGetMaxLength))(ITextHost *iface,DWORD *pLength);
typedef HRESULT (WINAPI typeof(ITextHostImpl_TxGetScrollBars))(ITextHost *iface,DWORD *pdwScrollBar);
typedef HRESULT (WINAPI typeof(ITextHostImpl_TxGetPasswordChar))(ITextHost *iface,WCHAR *pch);
typedef HRESULT (WINAPI typeof(ITextHostImpl_TxGetAcceleratorPos))(ITextHost *iface,LONG *pch);
typedef HRESULT (WINAPI typeof(ITextHostImpl_TxGetExtent))(ITextHost *iface,LPSIZEL lpExtent);
typedef HRESULT (WINAPI typeof(ITextHostImpl_OnTxCharFormatChange))(ITextHost *iface,const CHARFORMATW *pcf);
typedef HRESULT (WINAPI typeof(ITextHostImpl_OnTxParaFormatChange))(ITextHost *iface,const PARAFORMAT *ppf);
typedef HRESULT (WINAPI typeof(ITextHostImpl_TxGetPropertyBits))(ITextHost *iface,DWORD dwMask,DWORD *pdwBits);
typedef HRESULT (WINAPI typeof(ITextHostImpl_TxNotify))(ITextHost *iface,DWORD iNotify,void *pv);
typedef HIMC (WINAPI typeof(ITextHostImpl_TxImmGetContext))(ITextHost *iface);
typedef void (WINAPI typeof(ITextHostImpl_TxImmReleaseContext))(ITextHost *iface,HIMC himc);
typedef HRESULT (WINAPI typeof(ITextHostImpl_TxGetSelectionBarWidth))(ITextHost *iface,LONG *lSelBarWidth);

typedef HRESULT (WINAPI typeof(fnTextSrv_TxSendMessage))(ITextServices *iface,UINT msg,WPARAM wparam,LPARAM lparam,LRESULT* plresult);
typedef HRESULT (WINAPI typeof(fnTextSrv_TxDraw))(ITextServices *iface,DWORD dwDrawAspect,LONG lindex,void* pvAspect,DVTARGETDEVICE* ptd,HDC hdcDraw,HDC hdcTargetDev,LPCRECTL lprcBounds,LPCRECTL lprcWBounds,LPRECT lprcUpdate,BOOL (__stdcall * pfnContinue)(DWORD),DWORD dwContinue,LONG lViewId);
typedef HRESULT (WINAPI typeof(fnTextSrv_TxGetHScroll))(ITextServices *iface,LONG* plMin,LONG* plMax,LONG* plPos,LONG* plPage,BOOL* pfEnabled);
typedef HRESULT (WINAPI typeof(fnTextSrv_TxGetVScroll))(ITextServices *iface,LONG* plMin,LONG* plMax,LONG* plPos,LONG* plPage,BOOL* pfEnabled);
typedef HRESULT (WINAPI typeof(fnTextSrv_OnTxSetCursor))(ITextServices *iface,DWORD dwDrawAspect,LONG lindex,void* pvAspect,DVTARGETDEVICE* ptd,HDC hdcDraw,HDC hicTargetDev,LPCRECT lprcClient,INT x, INT y);
typedef HRESULT (WINAPI typeof(fnTextSrv_TxQueryHitPoint))(ITextServices *iface,DWORD dwDrawAspect,LONG lindex,void* pvAspect,DVTARGETDEVICE* ptd,HDC hdcDraw,HDC hicTargetDev,LPCRECT lprcClient,INT x, INT y,DWORD* pHitResult);
typedef HRESULT (WINAPI typeof(fnTextSrv_OnTxInplaceActivate))(ITextServices *iface,LPCRECT prcClient);
typedef HRESULT (WINAPI typeof(fnTextSrv_OnTxInplaceDeactivate))(ITextServices *iface);
typedef HRESULT (WINAPI typeof(fnTextSrv_OnTxUIActivate))(ITextServices *iface);
typedef HRESULT (WINAPI typeof(fnTextSrv_OnTxUIDeactivate))(ITextServices *iface);
typedef HRESULT (WINAPI typeof(fnTextSrv_TxGetText))(ITextServices *iface,BSTR* pbstrText);
typedef HRESULT (WINAPI typeof(fnTextSrv_TxSetText))(ITextServices *iface,LPCWSTR pszText);
typedef HRESULT (WINAPI typeof(fnTextSrv_TxGetCurTargetX))(ITextServices *iface,LONG* x);
typedef HRESULT (WINAPI typeof(fnTextSrv_TxGetBaseLinePos))(ITextServices *iface,LONG* x);
typedef HRESULT (WINAPI typeof(fnTextSrv_TxGetNaturalSize))(ITextServices *iface,DWORD dwAspect,HDC hdcDraw,HDC hicTargetDev,DVTARGETDEVICE* ptd,DWORD dwMode,const SIZEL* psizelExtent,LONG* pwidth,LONG* pheight);
typedef HRESULT (WINAPI typeof(fnTextSrv_TxGetDropTarget))(ITextServices *iface,struct IDropTarget** ppDropTarget);
typedef HRESULT (WINAPI typeof(fnTextSrv_OnTxPropertyBitsChange))(ITextServices *iface,DWORD dwMask,DWORD dwBits);
typedef HRESULT (WINAPI typeof(fnTextSrv_TxGetCachedSize))(ITextServices *iface,DWORD* pdwWidth,DWORD* pdwHeight);

#undef WINAPI
#undef HRESULT
#undef HDC
#undef BOOL
#undef COLORREF
#undef HIMC
#undef ITextHost
#undef INT
#undef UINT
#undef HBITMAP
#undef LPCRECT
#undef LPRECT
#undef LPCRECTL
#undef CHARFORMATW
#undef PARAFORMAT
#undef DWORD
#undef LONG
#undef WPARAM
#undef LPARAM
#undef HRGN
#undef HCURSOR
#undef LPPOINT
#undef TXTBACKSTYLE
#undef WCHAR
#undef BSTR
#undef LPCWSTR
#undef SIZEL
#undef LPSIZEL
#undef ITextServices
#undef LRESULT
#undef DVTARGETDEVICE

//#undef typeof
