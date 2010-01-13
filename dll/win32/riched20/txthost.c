/*
 * RichEdit - ITextHost implementation for windowed richedit controls
 *
 * Copyright 2009 by Dylan Smith
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#define COBJMACROS

#include "editor.h"
#include "ole2.h"
#include "richole.h"
#include "imm.h"
#include "textserv.h"
#include "wine/debug.h"
#include "editstr.h"

WINE_DEFAULT_DEBUG_CHANNEL(richedit);

typedef struct ITextHostImpl {
    const ITextHostVtbl *lpVtbl;
    LONG ref;
    HWND hWnd;
    BOOL bEmulateVersion10;
} ITextHostImpl;

static ITextHostVtbl textHostVtbl;

ITextHost *ME_CreateTextHost(HWND hwnd, BOOL bEmulateVersion10)
{
    ITextHostImpl *texthost;
    texthost = CoTaskMemAlloc(sizeof(*texthost));
    if (texthost)
    {
        ME_TextEditor *editor;

        texthost->lpVtbl = &textHostVtbl;
        texthost->ref = 1;
        texthost->hWnd = hwnd;
        texthost->bEmulateVersion10 = bEmulateVersion10;

        editor = ME_MakeEditor((ITextHost*)texthost, bEmulateVersion10);
        editor->exStyleFlags = GetWindowLongW(hwnd, GWL_EXSTYLE);
        editor->hWnd = hwnd; /* FIXME: Remove editor's dependence on hWnd */
        SetWindowLongPtrW(hwnd, 0, (LONG_PTR)editor);
    }

    return (ITextHost*)texthost;
}

static HRESULT WINAPI ITextHostImpl_QueryInterface(ITextHost *iface,
                                                   REFIID riid,
                                                   LPVOID *ppvObject)
{
    ITextHostImpl *This = (ITextHostImpl *)iface;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_ITextHost)) {
        *ppvObject = This;
        ITextHost_AddRef((ITextHost *)*ppvObject);
        return S_OK;
    }

    FIXME("Unknown interface: %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ITextHostImpl_AddRef(ITextHost *iface)
{
    ITextHostImpl *This = (ITextHostImpl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);
    return ref;
}

static ULONG WINAPI ITextHostImpl_Release(ITextHost *iface)
{
    ITextHostImpl *This = (ITextHostImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    if (!ref)
    {
        SetWindowLongPtrW(This->hWnd, 0, 0);
        CoTaskMemFree(This);
    }
    return ref;
}

HDC WINAPI ITextHostImpl_TxGetDC(ITextHost *iface)
{
    ITextHostImpl *This = (ITextHostImpl *)iface;
    return GetDC(This->hWnd);
}

INT WINAPI ITextHostImpl_TxReleaseDC(ITextHost *iface,
                                     HDC hdc)
{
    ITextHostImpl *This = (ITextHostImpl *)iface;
    return ReleaseDC(This->hWnd, hdc);
}

BOOL WINAPI ITextHostImpl_TxShowScrollBar(ITextHost *iface,
                                          INT fnBar,
                                          BOOL fShow)
{
    ITextHostImpl *This = (ITextHostImpl *)iface;
    return ShowScrollBar(This->hWnd, fnBar, fShow);
}

BOOL WINAPI ITextHostImpl_TxEnableScrollBar(ITextHost *iface,
                                            INT fuSBFlags,
                                            INT fuArrowflags)
{
    ITextHostImpl *This = (ITextHostImpl *)iface;
    return EnableScrollBar(This->hWnd, fuSBFlags, fuArrowflags);
}

BOOL WINAPI ITextHostImpl_TxSetScrollRange(ITextHost *iface,
                                           INT fnBar,
                                           LONG nMinPos,
                                           INT nMaxPos,
                                           BOOL fRedraw)
{
    ITextHostImpl *This = (ITextHostImpl *)iface;
    return SetScrollRange(This->hWnd, fnBar, nMinPos, nMaxPos, fRedraw);
}

BOOL WINAPI ITextHostImpl_TxSetScrollPos(ITextHost *iface,
                                         INT fnBar,
                                         INT nPos,
                                         BOOL fRedraw)
{
    ITextHostImpl *This = (ITextHostImpl *)iface;
    int pos = SetScrollPos(This->hWnd, fnBar, nPos, fRedraw);
    return (pos ? TRUE : FALSE);
}

void WINAPI ITextHostImpl_TxInvalidateRect(ITextHost *iface,
                                           LPCRECT prc,
                                           BOOL fMode)
{
    ITextHostImpl *This = (ITextHostImpl *)iface;
    InvalidateRect(This->hWnd, prc, fMode);
}

void WINAPI ITextHostImpl_TxViewChange(ITextHost *iface,
                                       BOOL fUpdate)
{
    ITextHostImpl *This = (ITextHostImpl *)iface;
    if (fUpdate)
        UpdateWindow(This->hWnd);
}

BOOL WINAPI ITextHostImpl_TxCreateCaret(ITextHost *iface,
                                        HBITMAP hbmp,
                                        INT xWidth, INT yHeight)
{
    ITextHostImpl *This = (ITextHostImpl *)iface;
    return CreateCaret(This->hWnd, hbmp, xWidth, yHeight);
}

BOOL WINAPI ITextHostImpl_TxShowCaret(ITextHost *iface, BOOL fShow)
{
    ITextHostImpl *This = (ITextHostImpl *)iface;
    if (fShow)
        return ShowCaret(This->hWnd);
    else
        return HideCaret(This->hWnd);
}

BOOL WINAPI ITextHostImpl_TxSetCaretPos(ITextHost *iface,
                                        INT x, INT y)
{
    return SetCaretPos(x, y);
}

BOOL WINAPI ITextHostImpl_TxSetTimer(ITextHost *iface,
                                     UINT idTimer, UINT uTimeout)
{
    ITextHostImpl *This = (ITextHostImpl *)iface;
    return SetTimer(This->hWnd, idTimer, uTimeout, NULL) != 0;
}

void WINAPI ITextHostImpl_TxKillTimer(ITextHost *iface,
                                      UINT idTimer)
{
    ITextHostImpl *This = (ITextHostImpl *)iface;
    KillTimer(This->hWnd, idTimer);
}

void WINAPI ITextHostImpl_TxScrollWindowEx(ITextHost *iface,
                                           INT dx, INT dy,
                                           LPCRECT lprcScroll,
                                           LPCRECT lprcClip,
                                           HRGN hRgnUpdate,
                                           LPRECT lprcUpdate,
                                           UINT fuScroll)
{
    ITextHostImpl *This = (ITextHostImpl *)iface;
    ScrollWindowEx(This->hWnd, dx, dy, lprcScroll, lprcClip,
                   hRgnUpdate, lprcUpdate, fuScroll);
}

void WINAPI ITextHostImpl_TxSetCapture(ITextHost *iface,
                                       BOOL fCapture)
{
    ITextHostImpl *This = (ITextHostImpl *)iface;
    if (fCapture)
        SetCapture(This->hWnd);
    else
        ReleaseCapture();
}

void WINAPI ITextHostImpl_TxSetFocus(ITextHost *iface)
{
    ITextHostImpl *This = (ITextHostImpl *)iface;
    SetFocus(This->hWnd);
}

void WINAPI ITextHostImpl_TxSetCursor(ITextHost *iface,
                                      HCURSOR hcur,
                                      BOOL fText)
{
    SetCursor(hcur);
}

BOOL WINAPI ITextHostImpl_TxScreenToClient(ITextHost *iface,
                                           LPPOINT lppt)
{
    ITextHostImpl *This = (ITextHostImpl *)iface;
    return ScreenToClient(This->hWnd, lppt);
}

BOOL WINAPI ITextHostImpl_TxClientToScreen(ITextHost *iface,
                                           LPPOINT lppt)
{
    ITextHostImpl *This = (ITextHostImpl *)iface;
    return ClientToScreen(This->hWnd, lppt);
}

HRESULT WINAPI ITextHostImpl_TxActivate(ITextHost *iface,
                                        LONG *plOldState)
{
    ITextHostImpl *This = (ITextHostImpl *)iface;
    *plOldState = HandleToLong(SetActiveWindow(This->hWnd));
    return (*plOldState ? S_OK : E_FAIL);
}

HRESULT WINAPI ITextHostImpl_TxDeactivate(ITextHost *iface,
                                          LONG lNewState)
{
    HWND ret = SetActiveWindow(LongToHandle(lNewState));
    return (ret ? S_OK : E_FAIL);
}

HRESULT WINAPI ITextHostImpl_TxGetClientRect(ITextHost *iface,
                                             LPRECT prc)
{
    ITextHostImpl *This = (ITextHostImpl *)iface;
    int ret = GetClientRect(This->hWnd, prc);
    return (ret ? S_OK : E_FAIL);
}

HRESULT WINAPI ITextHostImpl_TxGetViewInset(ITextHost *iface,
                                            LPRECT prc)
{
    prc->top = 0;
    prc->left = 0;
    prc->bottom = 0;
    prc->right = 0;
    return S_OK;
}

HRESULT WINAPI ITextHostImpl_TxGetCharFormat(ITextHost *iface,
                                             const CHARFORMATW **ppCF)
{
    return E_NOTIMPL;
}

HRESULT WINAPI ITextHostImpl_TxGetParaFormat(ITextHost *iface,
                                             const PARAFORMAT **ppPF)
{
    return E_NOTIMPL;
}

COLORREF WINAPI ITextHostImpl_TxGetSysColor(ITextHost *iface,
                                            int nIndex)
{
    return GetSysColor(nIndex);
}

HRESULT WINAPI ITextHostImpl_TxGetBackStyle(ITextHost *iface,
                                            TXTBACKSTYLE *pStyle)
{
    *pStyle = TXTBACK_OPAQUE;
    return S_OK;
}

HRESULT WINAPI ITextHostImpl_TxGetMaxLength(ITextHost *iface,
                                            DWORD *pLength)
{
    *pLength = INFINITE;
    return S_OK;
}

HRESULT WINAPI ITextHostImpl_TxGetScrollBars(ITextHost *iface,
                                             DWORD *pdwScrollBar)
{
    ITextHostImpl *This = (ITextHostImpl *)iface;
    ME_TextEditor *editor = (ME_TextEditor*)GetWindowLongPtrW(This->hWnd, 0);
    const DWORD mask = WS_VSCROLL|
                       WS_HSCROLL|
                       ES_AUTOVSCROLL|
                       ES_AUTOHSCROLL|
                       ES_DISABLENOSCROLL;
    if (editor)
    {
        *pdwScrollBar = editor->styleFlags & mask;
    } else {
        DWORD style = GetWindowLongW(This->hWnd, GWL_STYLE);
        if (style & WS_VSCROLL)
            style |= ES_AUTOVSCROLL;
        if (!This->bEmulateVersion10 && (style & WS_HSCROLL))
            style |= ES_AUTOHSCROLL;
        *pdwScrollBar = style & mask;
    }
    return S_OK;
}

HRESULT WINAPI ITextHostImpl_TxGetPasswordChar(ITextHost *iface,
                                               WCHAR *pch)
{
    *pch = '*';
    return S_OK;
}

HRESULT WINAPI ITextHostImpl_TxGetAcceleratorPos(ITextHost *iface,
                                                 LONG *pch)
{
    *pch = -1;
    return S_OK;
}

HRESULT WINAPI ITextHostImpl_TxGetExtent(ITextHost *iface,
                                         LPSIZEL lpExtent)
{
    return E_NOTIMPL;
}

HRESULT WINAPI ITextHostImpl_OnTxCharFormatChange(ITextHost *iface,
                                                  const CHARFORMATW *pcf)
{
    return S_OK;
}

HRESULT WINAPI ITextHostImpl_OnTxParaFormatChange(ITextHost *iface,
                                                  const PARAFORMAT *ppf)
{
    return S_OK;
}

HRESULT WINAPI ITextHostImpl_TxGetPropertyBits(ITextHost *iface,
                                               DWORD dwMask,
                                               DWORD *pdwBits)
{
    ITextHostImpl *This = (ITextHostImpl *)iface;
    ME_TextEditor *editor = (ME_TextEditor *)GetWindowLongPtrW(This->hWnd, 0);
    DWORD style;
    DWORD dwBits = 0;

    if (editor)
    {
        style = editor->styleFlags;
        if (editor->mode & TM_RICHTEXT)
            dwBits |= TXTBIT_RICHTEXT;
        if (editor->bWordWrap)
            dwBits |= TXTBIT_WORDWRAP;
        if (style & ECO_AUTOWORDSELECTION)
            dwBits |= TXTBIT_AUTOWORDSEL;
    } else {
        DWORD dwScrollBar;

        style = GetWindowLongW(This->hWnd, GWL_STYLE);
        ITextHostImpl_TxGetScrollBars(iface, &dwScrollBar);

        dwBits |= TXTBIT_RICHTEXT|TXTBIT_AUTOWORDSEL;
        if (!(dwScrollBar & ES_AUTOHSCROLL))
            dwBits |= TXTBIT_WORDWRAP;
    }

    /* Bits that correspond to window styles. */
    if (style & ES_MULTILINE)
        dwBits |= TXTBIT_MULTILINE;
    if (style & ES_READONLY)
        dwBits |= TXTBIT_READONLY;
    if (style & ES_PASSWORD)
        dwBits |= TXTBIT_USEPASSWORD;
    if (!(style & ES_NOHIDESEL))
        dwBits |= TXTBIT_HIDESELECTION;
    if (style & ES_SAVESEL)
        dwBits |= TXTBIT_SAVESELECTION;
    if (style & ES_VERTICAL)
        dwBits |= TXTBIT_VERTICAL;
    if (style & ES_NOOLEDRAGDROP)
        dwBits |= TXTBIT_DISABLEDRAG;

    dwBits |= TXTBIT_ALLOWBEEP;

    /* The following bits are always FALSE because they are probably only
     * needed for ITextServices_OnTxPropertyBitsChange:
     *   TXTBIT_VIEWINSETCHANGE
     *   TXTBIT_BACKSTYLECHANGE
     *   TXTBIT_MAXLENGTHCHANGE
     *   TXTBIT_CHARFORMATCHANGE
     *   TXTBIT_PARAFORMATCHANGE
     *   TXTBIT_SHOWACCELERATOR
     *   TXTBIT_EXTENTCHANGE
     *   TXTBIT_SELBARCHANGE
     *   TXTBIT_SCROLLBARCHANGE
     *   TXTBIT_CLIENTRECTCHANGE
     *
     * Documented by MSDN as not supported:
     *   TXTBIT_USECURRENTBKG
     */

    *pdwBits = dwBits & dwMask;
    return S_OK;
}

HRESULT WINAPI ITextHostImpl_TxNotify(ITextHost *iface,
                                      DWORD iNotify,
                                      void *pv)
{
    ITextHostImpl *This = (ITextHostImpl *)iface;
    HWND hwnd = This->hWnd;
    HWND parent = GetParent(hwnd);
    UINT id = GetWindowLongW(hwnd, GWLP_ID);

    switch (iNotify)
    {
        case EN_DROPFILES:
        case EN_LINK:
        case EN_OLEOPFAILED:
        case EN_PROTECTED:
        case EN_REQUESTRESIZE:
        case EN_SAVECLIPBOARD:
        case EN_SELCHANGE:
        case EN_STOPNOUNDO:
        {
            /* FIXME: Verify this assumption that pv starts with NMHDR. */
            NMHDR *info = pv;
            if (!info)
                return E_FAIL;

            info->hwndFrom = hwnd;
            info->idFrom = id;
            info->code = iNotify;
            SendMessageW(parent, WM_NOTIFY, id, (LPARAM)info);
            break;
        }

        case EN_UPDATE:
            /* Only sent when the window is visible. */
            if (!IsWindowVisible(This->hWnd))
                break;
            /* Fall through */
        case EN_CHANGE:
        case EN_ERRSPACE:
        case EN_HSCROLL:
        case EN_KILLFOCUS:
        case EN_MAXTEXT:
        case EN_SETFOCUS:
        case EN_VSCROLL:
            SendMessageW(parent, WM_COMMAND, MAKEWPARAM(id, iNotify), (LPARAM)hwnd);
            break;

        case EN_MSGFILTER:
            FIXME("EN_MSGFILTER is documented as not being sent to TxNotify\n");
            /* fall through */
        default:
            return E_FAIL;
    }
    return S_OK;
}

HIMC WINAPI ITextHostImpl_TxImmGetContext(ITextHost *iface)
{
    ITextHostImpl *This = (ITextHostImpl *)iface;
    return ImmGetContext(This->hWnd);
}

void WINAPI ITextHostImpl_TxImmReleaseContext(ITextHost *iface,
                                              HIMC himc)
{
    ITextHostImpl *This = (ITextHostImpl *)iface;
    ImmReleaseContext(This->hWnd, himc);
}

HRESULT WINAPI ITextHostImpl_TxGetSelectionBarWidth(ITextHost *iface,
                                                    LONG *lSelBarWidth)
{
    ITextHostImpl *This = (ITextHostImpl *)iface;
    ME_TextEditor *editor = (ME_TextEditor *)GetWindowLongPtrW(This->hWnd, 0);

    DWORD style = editor ? editor->styleFlags
                         : GetWindowLongW(This->hWnd, GWL_STYLE);
    *lSelBarWidth = (style & ES_SELECTIONBAR) ? 225 : 0; /* in HIMETRIC */
    return S_OK;
}


#ifdef __i386__  /* thiscall functions are i386-specific */

#define THISCALL(func) __thiscall_ ## func
#define DEFINE_THISCALL_WRAPPER(func) \
   extern typeof(func) THISCALL(func); \
   __ASM_GLOBAL_FUNC(__thiscall_ ## func, \
                   "popl %eax\n\t" \
                   "pushl %ecx\n\t" \
                   "pushl %eax\n\t" \
                   "jmp " __ASM_NAME(#func) )

#else /* __i386__ */

#define THISCALL(func) func
#define DEFINE_THISCALL_WRAPPER(func) /* nothing */

#endif /* __i386__ */

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetDC);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxReleaseDC);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxShowScrollBar);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxEnableScrollBar);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxSetScrollRange);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxSetScrollPos);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxInvalidateRect);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxViewChange);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxCreateCaret);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxShowCaret);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxSetCaretPos);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxSetTimer);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxKillTimer);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxScrollWindowEx);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxSetCapture);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxSetFocus);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxSetCursor);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxScreenToClient);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxClientToScreen);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxActivate);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxDeactivate);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetClientRect);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetViewInset);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetCharFormat);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetParaFormat);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetSysColor);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetBackStyle);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetMaxLength);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetScrollBars);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetPasswordChar);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetAcceleratorPos);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetExtent);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_OnTxCharFormatChange);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_OnTxParaFormatChange);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetPropertyBits);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxNotify);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxImmGetContext);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxImmReleaseContext);
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetSelectionBarWidth);

static ITextHostVtbl textHostVtbl = {
    ITextHostImpl_QueryInterface,
    ITextHostImpl_AddRef,
    ITextHostImpl_Release,
    THISCALL(ITextHostImpl_TxGetDC),
    THISCALL(ITextHostImpl_TxReleaseDC),
    THISCALL(ITextHostImpl_TxShowScrollBar),
    THISCALL(ITextHostImpl_TxEnableScrollBar),
    THISCALL(ITextHostImpl_TxSetScrollRange),
    THISCALL(ITextHostImpl_TxSetScrollPos),
    THISCALL(ITextHostImpl_TxInvalidateRect),
    THISCALL(ITextHostImpl_TxViewChange),
    THISCALL(ITextHostImpl_TxCreateCaret),
    THISCALL(ITextHostImpl_TxShowCaret),
    THISCALL(ITextHostImpl_TxSetCaretPos),
    THISCALL(ITextHostImpl_TxSetTimer),
    THISCALL(ITextHostImpl_TxKillTimer),
    THISCALL(ITextHostImpl_TxScrollWindowEx),
    THISCALL(ITextHostImpl_TxSetCapture),
    THISCALL(ITextHostImpl_TxSetFocus),
    THISCALL(ITextHostImpl_TxSetCursor),
    THISCALL(ITextHostImpl_TxScreenToClient),
    THISCALL(ITextHostImpl_TxClientToScreen),
    THISCALL(ITextHostImpl_TxActivate),
    THISCALL(ITextHostImpl_TxDeactivate),
    THISCALL(ITextHostImpl_TxGetClientRect),
    THISCALL(ITextHostImpl_TxGetViewInset),
    THISCALL(ITextHostImpl_TxGetCharFormat),
    THISCALL(ITextHostImpl_TxGetParaFormat),
    THISCALL(ITextHostImpl_TxGetSysColor),
    THISCALL(ITextHostImpl_TxGetBackStyle),
    THISCALL(ITextHostImpl_TxGetMaxLength),
    THISCALL(ITextHostImpl_TxGetScrollBars),
    THISCALL(ITextHostImpl_TxGetPasswordChar),
    THISCALL(ITextHostImpl_TxGetAcceleratorPos),
    THISCALL(ITextHostImpl_TxGetExtent),
    THISCALL(ITextHostImpl_OnTxCharFormatChange),
    THISCALL(ITextHostImpl_OnTxParaFormatChange),
    THISCALL(ITextHostImpl_TxGetPropertyBits),
    THISCALL(ITextHostImpl_TxNotify),
    THISCALL(ITextHostImpl_TxImmGetContext),
    THISCALL(ITextHostImpl_TxImmReleaseContext),
    THISCALL(ITextHostImpl_TxGetSelectionBarWidth),
};

#ifdef __i386__  /* thiscall functions are i386-specific */

#define STDCALL(func) __stdcall_ ## func
#define DEFINE_STDCALL_WRAPPER(num,func) \
   extern typeof(func) __stdcall_ ## func; \
   __ASM_GLOBAL_FUNC(__stdcall_ ## func, \
                   "popl %eax\n\t" \
                   "popl %ecx\n\t" \
                   "pushl %eax\n\t" \
                   "movl (%ecx), %eax\n\t" \
                   "jmp *(4*(" #num "))(%eax)" )

DEFINE_STDCALL_WRAPPER(3,ITextHostImpl_TxGetDC);
DEFINE_STDCALL_WRAPPER(4,ITextHostImpl_TxReleaseDC);
DEFINE_STDCALL_WRAPPER(5,ITextHostImpl_TxShowScrollBar);
DEFINE_STDCALL_WRAPPER(6,ITextHostImpl_TxEnableScrollBar);
DEFINE_STDCALL_WRAPPER(7,ITextHostImpl_TxSetScrollRange);
DEFINE_STDCALL_WRAPPER(8,ITextHostImpl_TxSetScrollPos);
DEFINE_STDCALL_WRAPPER(9,ITextHostImpl_TxInvalidateRect);
DEFINE_STDCALL_WRAPPER(10,ITextHostImpl_TxViewChange);
DEFINE_STDCALL_WRAPPER(11,ITextHostImpl_TxCreateCaret);
DEFINE_STDCALL_WRAPPER(12,ITextHostImpl_TxShowCaret);
DEFINE_STDCALL_WRAPPER(13,ITextHostImpl_TxSetCaretPos);
DEFINE_STDCALL_WRAPPER(14,ITextHostImpl_TxSetTimer);
DEFINE_STDCALL_WRAPPER(15,ITextHostImpl_TxKillTimer);
DEFINE_STDCALL_WRAPPER(16,ITextHostImpl_TxScrollWindowEx);
DEFINE_STDCALL_WRAPPER(17,ITextHostImpl_TxSetCapture);
DEFINE_STDCALL_WRAPPER(18,ITextHostImpl_TxSetFocus);
DEFINE_STDCALL_WRAPPER(19,ITextHostImpl_TxSetCursor);
DEFINE_STDCALL_WRAPPER(20,ITextHostImpl_TxScreenToClient);
DEFINE_STDCALL_WRAPPER(21,ITextHostImpl_TxClientToScreen);
DEFINE_STDCALL_WRAPPER(22,ITextHostImpl_TxActivate);
DEFINE_STDCALL_WRAPPER(23,ITextHostImpl_TxDeactivate);
DEFINE_STDCALL_WRAPPER(24,ITextHostImpl_TxGetClientRect);
DEFINE_STDCALL_WRAPPER(25,ITextHostImpl_TxGetViewInset);
DEFINE_STDCALL_WRAPPER(26,ITextHostImpl_TxGetCharFormat);
DEFINE_STDCALL_WRAPPER(27,ITextHostImpl_TxGetParaFormat);
DEFINE_STDCALL_WRAPPER(28,ITextHostImpl_TxGetSysColor);
DEFINE_STDCALL_WRAPPER(29,ITextHostImpl_TxGetBackStyle);
DEFINE_STDCALL_WRAPPER(30,ITextHostImpl_TxGetMaxLength);
DEFINE_STDCALL_WRAPPER(31,ITextHostImpl_TxGetScrollBars);
DEFINE_STDCALL_WRAPPER(32,ITextHostImpl_TxGetPasswordChar);
DEFINE_STDCALL_WRAPPER(33,ITextHostImpl_TxGetAcceleratorPos);
DEFINE_STDCALL_WRAPPER(34,ITextHostImpl_TxGetExtent);
DEFINE_STDCALL_WRAPPER(35,ITextHostImpl_OnTxCharFormatChange);
DEFINE_STDCALL_WRAPPER(36,ITextHostImpl_OnTxParaFormatChange);
DEFINE_STDCALL_WRAPPER(37,ITextHostImpl_TxGetPropertyBits);
DEFINE_STDCALL_WRAPPER(38,ITextHostImpl_TxNotify);
DEFINE_STDCALL_WRAPPER(39,ITextHostImpl_TxImmGetContext);
DEFINE_STDCALL_WRAPPER(40,ITextHostImpl_TxImmReleaseContext);
DEFINE_STDCALL_WRAPPER(41,ITextHostImpl_TxGetSelectionBarWidth);

ITextHostVtbl itextHostStdcallVtbl = {
    NULL,
    NULL,
    NULL,
    __stdcall_ITextHostImpl_TxGetDC,
    __stdcall_ITextHostImpl_TxReleaseDC,
    __stdcall_ITextHostImpl_TxShowScrollBar,
    __stdcall_ITextHostImpl_TxEnableScrollBar,
    __stdcall_ITextHostImpl_TxSetScrollRange,
    __stdcall_ITextHostImpl_TxSetScrollPos,
    __stdcall_ITextHostImpl_TxInvalidateRect,
    __stdcall_ITextHostImpl_TxViewChange,
    __stdcall_ITextHostImpl_TxCreateCaret,
    __stdcall_ITextHostImpl_TxShowCaret,
    __stdcall_ITextHostImpl_TxSetCaretPos,
    __stdcall_ITextHostImpl_TxSetTimer,
    __stdcall_ITextHostImpl_TxKillTimer,
    __stdcall_ITextHostImpl_TxScrollWindowEx,
    __stdcall_ITextHostImpl_TxSetCapture,
    __stdcall_ITextHostImpl_TxSetFocus,
    __stdcall_ITextHostImpl_TxSetCursor,
    __stdcall_ITextHostImpl_TxScreenToClient,
    __stdcall_ITextHostImpl_TxClientToScreen,
    __stdcall_ITextHostImpl_TxActivate,
    __stdcall_ITextHostImpl_TxDeactivate,
    __stdcall_ITextHostImpl_TxGetClientRect,
    __stdcall_ITextHostImpl_TxGetViewInset,
    __stdcall_ITextHostImpl_TxGetCharFormat,
    __stdcall_ITextHostImpl_TxGetParaFormat,
    __stdcall_ITextHostImpl_TxGetSysColor,
    __stdcall_ITextHostImpl_TxGetBackStyle,
    __stdcall_ITextHostImpl_TxGetMaxLength,
    __stdcall_ITextHostImpl_TxGetScrollBars,
    __stdcall_ITextHostImpl_TxGetPasswordChar,
    __stdcall_ITextHostImpl_TxGetAcceleratorPos,
    __stdcall_ITextHostImpl_TxGetExtent,
    __stdcall_ITextHostImpl_OnTxCharFormatChange,
    __stdcall_ITextHostImpl_OnTxParaFormatChange,
    __stdcall_ITextHostImpl_TxGetPropertyBits,
    __stdcall_ITextHostImpl_TxNotify,
    __stdcall_ITextHostImpl_TxImmGetContext,
    __stdcall_ITextHostImpl_TxImmReleaseContext,
    __stdcall_ITextHostImpl_TxGetSelectionBarWidth,
};

#endif /* __i386__ */
