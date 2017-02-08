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

#include "editor.h"

WINE_DEFAULT_DEBUG_CHANNEL(richedit);

typedef struct ITextHostImpl {
    ITextHost ITextHost_iface;
    LONG ref;
    HWND hWnd;
    BOOL bEmulateVersion10;
} ITextHostImpl;

static const ITextHostVtbl textHostVtbl;

ITextHost *ME_CreateTextHost(HWND hwnd, CREATESTRUCTW *cs, BOOL bEmulateVersion10)
{
    ITextHostImpl *texthost;
    texthost = CoTaskMemAlloc(sizeof(*texthost));
    if (texthost)
    {
        ME_TextEditor *editor;

        texthost->ITextHost_iface.lpVtbl = &textHostVtbl;
        texthost->ref = 1;
        texthost->hWnd = hwnd;
        texthost->bEmulateVersion10 = bEmulateVersion10;

        editor = ME_MakeEditor(&texthost->ITextHost_iface, bEmulateVersion10, cs->style);
        editor->exStyleFlags = GetWindowLongW(hwnd, GWL_EXSTYLE);
        editor->styleFlags |= GetWindowLongW(hwnd, GWL_STYLE) & ES_WANTRETURN;
        editor->hWnd = hwnd; /* FIXME: Remove editor's dependence on hWnd */
        editor->hwndParent = cs->hwndParent;
        SetWindowLongPtrW(hwnd, 0, (LONG_PTR)editor);
    }

    return &texthost->ITextHost_iface;
}

static inline ITextHostImpl *impl_from_ITextHost(ITextHost *iface)
{
    return CONTAINING_RECORD(iface, ITextHostImpl, ITextHost_iface);
}

static HRESULT WINAPI ITextHostImpl_QueryInterface(ITextHost *iface, REFIID riid, void **ppvObject)
{
    ITextHostImpl *This = impl_from_ITextHost(iface);

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
    ITextHostImpl *This = impl_from_ITextHost(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    return ref;
}

static ULONG WINAPI ITextHostImpl_Release(ITextHost *iface)
{
    ITextHostImpl *This = impl_from_ITextHost(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    if (!ref)
    {
        SetWindowLongPtrW(This->hWnd, 0, 0);
        CoTaskMemFree(This);
    }
    return ref;
}

DECLSPEC_HIDDEN HDC WINAPI ITextHostImpl_TxGetDC(ITextHost *iface)
{
    ITextHostImpl *This = impl_from_ITextHost(iface);
    return GetDC(This->hWnd);
}

DECLSPEC_HIDDEN INT WINAPI ITextHostImpl_TxReleaseDC(ITextHost *iface, HDC hdc)
{
    ITextHostImpl *This = impl_from_ITextHost(iface);
    return ReleaseDC(This->hWnd, hdc);
}

DECLSPEC_HIDDEN BOOL WINAPI ITextHostImpl_TxShowScrollBar(ITextHost *iface, INT fnBar, BOOL fShow)
{
    ITextHostImpl *This = impl_from_ITextHost(iface);
    return ShowScrollBar(This->hWnd, fnBar, fShow);
}

DECLSPEC_HIDDEN BOOL WINAPI ITextHostImpl_TxEnableScrollBar(ITextHost *iface, INT fuSBFlags, INT fuArrowflags)
{
    ITextHostImpl *This = impl_from_ITextHost(iface);
    return EnableScrollBar(This->hWnd, fuSBFlags, fuArrowflags);
}

DECLSPEC_HIDDEN BOOL WINAPI ITextHostImpl_TxSetScrollRange(ITextHost *iface, INT fnBar, LONG nMinPos, INT nMaxPos,
                                           BOOL fRedraw)
{
    ITextHostImpl *This = impl_from_ITextHost(iface);
    return SetScrollRange(This->hWnd, fnBar, nMinPos, nMaxPos, fRedraw);
}

DECLSPEC_HIDDEN BOOL WINAPI ITextHostImpl_TxSetScrollPos(ITextHost *iface, INT fnBar, INT nPos, BOOL fRedraw)
{
    ITextHostImpl *This = impl_from_ITextHost(iface);
    return SetScrollPos(This->hWnd, fnBar, nPos, fRedraw) != 0;
}

DECLSPEC_HIDDEN void WINAPI ITextHostImpl_TxInvalidateRect(ITextHost *iface, LPCRECT prc, BOOL fMode)
{
    ITextHostImpl *This = impl_from_ITextHost(iface);
    InvalidateRect(This->hWnd, prc, fMode);
}

DECLSPEC_HIDDEN void WINAPI ITextHostImpl_TxViewChange(ITextHost *iface, BOOL fUpdate)
{
    ITextHostImpl *This = impl_from_ITextHost(iface);
    if (fUpdate)
        UpdateWindow(This->hWnd);
}

DECLSPEC_HIDDEN BOOL WINAPI ITextHostImpl_TxCreateCaret(ITextHost *iface, HBITMAP hbmp, INT xWidth, INT yHeight)
{
    ITextHostImpl *This = impl_from_ITextHost(iface);
    return CreateCaret(This->hWnd, hbmp, xWidth, yHeight);
}

DECLSPEC_HIDDEN BOOL WINAPI ITextHostImpl_TxShowCaret(ITextHost *iface, BOOL fShow)
{
    ITextHostImpl *This = impl_from_ITextHost(iface);
    if (fShow)
        return ShowCaret(This->hWnd);
    else
        return HideCaret(This->hWnd);
}

DECLSPEC_HIDDEN BOOL WINAPI ITextHostImpl_TxSetCaretPos(ITextHost *iface,
                                        INT x, INT y)
{
    return SetCaretPos(x, y);
}

DECLSPEC_HIDDEN BOOL WINAPI ITextHostImpl_TxSetTimer(ITextHost *iface, UINT idTimer, UINT uTimeout)
{
    ITextHostImpl *This = impl_from_ITextHost(iface);
    return SetTimer(This->hWnd, idTimer, uTimeout, NULL) != 0;
}

DECLSPEC_HIDDEN void WINAPI ITextHostImpl_TxKillTimer(ITextHost *iface, UINT idTimer)
{
    ITextHostImpl *This = impl_from_ITextHost(iface);
    KillTimer(This->hWnd, idTimer);
}

DECLSPEC_HIDDEN void WINAPI ITextHostImpl_TxScrollWindowEx(ITextHost *iface, INT dx, INT dy, LPCRECT lprcScroll,
                                           LPCRECT lprcClip, HRGN hRgnUpdate, LPRECT lprcUpdate,
                                           UINT fuScroll)
{
    ITextHostImpl *This = impl_from_ITextHost(iface);
    ScrollWindowEx(This->hWnd, dx, dy, lprcScroll, lprcClip,
                   hRgnUpdate, lprcUpdate, fuScroll);
}

DECLSPEC_HIDDEN void WINAPI ITextHostImpl_TxSetCapture(ITextHost *iface, BOOL fCapture)
{
    ITextHostImpl *This = impl_from_ITextHost(iface);
    if (fCapture)
        SetCapture(This->hWnd);
    else
        ReleaseCapture();
}

DECLSPEC_HIDDEN void WINAPI ITextHostImpl_TxSetFocus(ITextHost *iface)
{
    ITextHostImpl *This = impl_from_ITextHost(iface);
    SetFocus(This->hWnd);
}

DECLSPEC_HIDDEN void WINAPI ITextHostImpl_TxSetCursor(ITextHost *iface,
                                      HCURSOR hcur,
                                      BOOL fText)
{
    SetCursor(hcur);
}

DECLSPEC_HIDDEN BOOL WINAPI ITextHostImpl_TxScreenToClient(ITextHost *iface, LPPOINT lppt)
{
    ITextHostImpl *This = impl_from_ITextHost(iface);
    return ScreenToClient(This->hWnd, lppt);
}

DECLSPEC_HIDDEN BOOL WINAPI ITextHostImpl_TxClientToScreen(ITextHost *iface, LPPOINT lppt)
{
    ITextHostImpl *This = impl_from_ITextHost(iface);
    return ClientToScreen(This->hWnd, lppt);
}

DECLSPEC_HIDDEN HRESULT WINAPI ITextHostImpl_TxActivate(ITextHost *iface, LONG *plOldState)
{
    ITextHostImpl *This = impl_from_ITextHost(iface);
    *plOldState = HandleToLong(SetActiveWindow(This->hWnd));
    return (*plOldState ? S_OK : E_FAIL);
}

DECLSPEC_HIDDEN HRESULT WINAPI ITextHostImpl_TxDeactivate(ITextHost *iface,
                                          LONG lNewState)
{
    HWND ret = SetActiveWindow(LongToHandle(lNewState));
    return (ret ? S_OK : E_FAIL);
}

DECLSPEC_HIDDEN HRESULT WINAPI ITextHostImpl_TxGetClientRect(ITextHost *iface, LPRECT prc)
{
    ITextHostImpl *This = impl_from_ITextHost(iface);
    int ret = GetClientRect(This->hWnd, prc);
    return (ret ? S_OK : E_FAIL);
}

DECLSPEC_HIDDEN HRESULT WINAPI ITextHostImpl_TxGetViewInset(ITextHost *iface,
                                            LPRECT prc)
{
    prc->top = 0;
    prc->left = 0;
    prc->bottom = 0;
    prc->right = 0;
    return S_OK;
}

DECLSPEC_HIDDEN HRESULT WINAPI ITextHostImpl_TxGetCharFormat(ITextHost *iface,
                                             const CHARFORMATW **ppCF)
{
    return E_NOTIMPL;
}

DECLSPEC_HIDDEN HRESULT WINAPI ITextHostImpl_TxGetParaFormat(ITextHost *iface,
                                             const PARAFORMAT **ppPF)
{
    return E_NOTIMPL;
}

DECLSPEC_HIDDEN COLORREF WINAPI ITextHostImpl_TxGetSysColor(ITextHost *iface,
                                            int nIndex)
{
    return GetSysColor(nIndex);
}

DECLSPEC_HIDDEN HRESULT WINAPI ITextHostImpl_TxGetBackStyle(ITextHost *iface,
                                            TXTBACKSTYLE *pStyle)
{
    *pStyle = TXTBACK_OPAQUE;
    return S_OK;
}

DECLSPEC_HIDDEN HRESULT WINAPI ITextHostImpl_TxGetMaxLength(ITextHost *iface,
                                            DWORD *pLength)
{
    *pLength = INFINITE;
    return S_OK;
}

DECLSPEC_HIDDEN HRESULT WINAPI ITextHostImpl_TxGetScrollBars(ITextHost *iface, DWORD *pdwScrollBar)
{
    ITextHostImpl *This = impl_from_ITextHost(iface);
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

DECLSPEC_HIDDEN HRESULT WINAPI ITextHostImpl_TxGetPasswordChar(ITextHost *iface,
                                               WCHAR *pch)
{
    *pch = '*';
    return S_OK;
}

DECLSPEC_HIDDEN HRESULT WINAPI ITextHostImpl_TxGetAcceleratorPos(ITextHost *iface,
                                                 LONG *pch)
{
    *pch = -1;
    return S_OK;
}

DECLSPEC_HIDDEN HRESULT WINAPI ITextHostImpl_TxGetExtent(ITextHost *iface,
                                         LPSIZEL lpExtent)
{
    return E_NOTIMPL;
}

DECLSPEC_HIDDEN HRESULT WINAPI ITextHostImpl_OnTxCharFormatChange(ITextHost *iface,
                                                  const CHARFORMATW *pcf)
{
    return S_OK;
}

DECLSPEC_HIDDEN HRESULT WINAPI ITextHostImpl_OnTxParaFormatChange(ITextHost *iface,
                                                  const PARAFORMAT *ppf)
{
    return S_OK;
}

DECLSPEC_HIDDEN HRESULT WINAPI ITextHostImpl_TxGetPropertyBits(ITextHost *iface, DWORD dwMask, DWORD *pdwBits)
{
    ITextHostImpl *This = impl_from_ITextHost(iface);
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

DECLSPEC_HIDDEN HRESULT WINAPI ITextHostImpl_TxNotify(ITextHost *iface, DWORD iNotify, void *pv)
{
    ITextHostImpl *This = impl_from_ITextHost(iface);
    ME_TextEditor *editor = (ME_TextEditor*)GetWindowLongPtrW(This->hWnd, 0);
    HWND hwnd = This->hWnd;
    UINT id;

    if (!editor || !editor->hwndParent) return S_OK;

    id = GetWindowLongW(hwnd, GWLP_ID);

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
            SendMessageW(editor->hwndParent, WM_NOTIFY, id, (LPARAM)info);
            break;
        }

        case EN_UPDATE:
            /* Only sent when the window is visible. */
            if (!IsWindowVisible(hwnd))
                break;
            /* Fall through */
        case EN_CHANGE:
        case EN_ERRSPACE:
        case EN_HSCROLL:
        case EN_KILLFOCUS:
        case EN_MAXTEXT:
        case EN_SETFOCUS:
        case EN_VSCROLL:
            SendMessageW(editor->hwndParent, WM_COMMAND, MAKEWPARAM(id, iNotify), (LPARAM)hwnd);
            break;

        case EN_MSGFILTER:
            FIXME("EN_MSGFILTER is documented as not being sent to TxNotify\n");
            /* fall through */
        default:
            return E_FAIL;
    }
    return S_OK;
}

DECLSPEC_HIDDEN HIMC WINAPI ITextHostImpl_TxImmGetContext(ITextHost *iface)
{
    ITextHostImpl *This = impl_from_ITextHost(iface);
    return ImmGetContext(This->hWnd);
}

DECLSPEC_HIDDEN void WINAPI ITextHostImpl_TxImmReleaseContext(ITextHost *iface, HIMC himc)
{
    ITextHostImpl *This = impl_from_ITextHost(iface);
    ImmReleaseContext(This->hWnd, himc);
}

DECLSPEC_HIDDEN HRESULT WINAPI ITextHostImpl_TxGetSelectionBarWidth(ITextHost *iface, LONG *lSelBarWidth)
{
    ITextHostImpl *This = impl_from_ITextHost(iface);
    ME_TextEditor *editor = (ME_TextEditor *)GetWindowLongPtrW(This->hWnd, 0);

    DWORD style = editor ? editor->styleFlags
                         : GetWindowLongW(This->hWnd, GWL_STYLE);
    *lSelBarWidth = (style & ES_SELECTIONBAR) ? 225 : 0; /* in HIMETRIC */
    return S_OK;
}


#ifdef __i386__  /* thiscall functions are i386-specific */

#define THISCALL(func) __thiscall_ ## func
#define DEFINE_THISCALL_WRAPPER(func,args) \
   extern typeof(func) THISCALL(func); \
   __ASM_STDCALL_FUNC(__thiscall_ ## func, args, \
                   "popl %eax\n\t" \
                   "pushl %ecx\n\t" \
                   "pushl %eax\n\t" \
                   "jmp " __ASM_NAME(#func) __ASM_STDCALL(args) )

#else /* __i386__ */

#define THISCALL(func) func
#define DEFINE_THISCALL_WRAPPER(func,args) /* nothing */

#endif /* __i386__ */

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetDC,4)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxReleaseDC,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxShowScrollBar,12)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxEnableScrollBar,12)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxSetScrollRange,20)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxSetScrollPos,16)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxInvalidateRect,12)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxViewChange,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxCreateCaret,16)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxShowCaret,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxSetCaretPos,12)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxSetTimer,12)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxKillTimer,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxScrollWindowEx,32)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxSetCapture,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxSetFocus,4)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxSetCursor,12)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxScreenToClient,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxClientToScreen,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxActivate,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxDeactivate,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetClientRect,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetViewInset,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetCharFormat,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetParaFormat,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetSysColor,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetBackStyle,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetMaxLength,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetScrollBars,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetPasswordChar,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetAcceleratorPos,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetExtent,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_OnTxCharFormatChange,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_OnTxParaFormatChange,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetPropertyBits,12)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxNotify,12)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxImmGetContext,4)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxImmReleaseContext,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetSelectionBarWidth,8)

#ifdef __i386__  /* thiscall functions are i386-specific */

#define STDCALL(func) __stdcall_ ## func
#define DEFINE_STDCALL_WRAPPER(num,func,args) \
   extern typeof(func) __stdcall_ ## func; \
   __ASM_STDCALL_FUNC(__stdcall_ ## func, args, \
                   "popl %eax\n\t" \
                   "popl %ecx\n\t" \
                   "pushl %eax\n\t" \
                   "movl (%ecx), %eax\n\t" \
                   "jmp *(4*(" #num "))(%eax)" )

DEFINE_STDCALL_WRAPPER(3,ITextHostImpl_TxGetDC,4)
DEFINE_STDCALL_WRAPPER(4,ITextHostImpl_TxReleaseDC,8)
DEFINE_STDCALL_WRAPPER(5,ITextHostImpl_TxShowScrollBar,12)
DEFINE_STDCALL_WRAPPER(6,ITextHostImpl_TxEnableScrollBar,12)
DEFINE_STDCALL_WRAPPER(7,ITextHostImpl_TxSetScrollRange,20)
DEFINE_STDCALL_WRAPPER(8,ITextHostImpl_TxSetScrollPos,16)
DEFINE_STDCALL_WRAPPER(9,ITextHostImpl_TxInvalidateRect,12)
DEFINE_STDCALL_WRAPPER(10,ITextHostImpl_TxViewChange,8)
DEFINE_STDCALL_WRAPPER(11,ITextHostImpl_TxCreateCaret,16)
DEFINE_STDCALL_WRAPPER(12,ITextHostImpl_TxShowCaret,8)
DEFINE_STDCALL_WRAPPER(13,ITextHostImpl_TxSetCaretPos,12)
DEFINE_STDCALL_WRAPPER(14,ITextHostImpl_TxSetTimer,12)
DEFINE_STDCALL_WRAPPER(15,ITextHostImpl_TxKillTimer,8)
DEFINE_STDCALL_WRAPPER(16,ITextHostImpl_TxScrollWindowEx,32)
DEFINE_STDCALL_WRAPPER(17,ITextHostImpl_TxSetCapture,8)
DEFINE_STDCALL_WRAPPER(18,ITextHostImpl_TxSetFocus,4)
DEFINE_STDCALL_WRAPPER(19,ITextHostImpl_TxSetCursor,12)
DEFINE_STDCALL_WRAPPER(20,ITextHostImpl_TxScreenToClient,8)
DEFINE_STDCALL_WRAPPER(21,ITextHostImpl_TxClientToScreen,8)
DEFINE_STDCALL_WRAPPER(22,ITextHostImpl_TxActivate,8)
DEFINE_STDCALL_WRAPPER(23,ITextHostImpl_TxDeactivate,8)
DEFINE_STDCALL_WRAPPER(24,ITextHostImpl_TxGetClientRect,8)
DEFINE_STDCALL_WRAPPER(25,ITextHostImpl_TxGetViewInset,8)
DEFINE_STDCALL_WRAPPER(26,ITextHostImpl_TxGetCharFormat,8)
DEFINE_STDCALL_WRAPPER(27,ITextHostImpl_TxGetParaFormat,8)
DEFINE_STDCALL_WRAPPER(28,ITextHostImpl_TxGetSysColor,8)
DEFINE_STDCALL_WRAPPER(29,ITextHostImpl_TxGetBackStyle,8)
DEFINE_STDCALL_WRAPPER(30,ITextHostImpl_TxGetMaxLength,8)
DEFINE_STDCALL_WRAPPER(31,ITextHostImpl_TxGetScrollBars,8)
DEFINE_STDCALL_WRAPPER(32,ITextHostImpl_TxGetPasswordChar,8)
DEFINE_STDCALL_WRAPPER(33,ITextHostImpl_TxGetAcceleratorPos,8)
DEFINE_STDCALL_WRAPPER(34,ITextHostImpl_TxGetExtent,8)
DEFINE_STDCALL_WRAPPER(35,ITextHostImpl_OnTxCharFormatChange,8)
DEFINE_STDCALL_WRAPPER(36,ITextHostImpl_OnTxParaFormatChange,8)
DEFINE_STDCALL_WRAPPER(37,ITextHostImpl_TxGetPropertyBits,12)
DEFINE_STDCALL_WRAPPER(38,ITextHostImpl_TxNotify,12)
DEFINE_STDCALL_WRAPPER(39,ITextHostImpl_TxImmGetContext,4)
DEFINE_STDCALL_WRAPPER(40,ITextHostImpl_TxImmReleaseContext,8)
DEFINE_STDCALL_WRAPPER(41,ITextHostImpl_TxGetSelectionBarWidth,8)

const ITextHostVtbl itextHostStdcallVtbl = {
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

static const ITextHostVtbl textHostVtbl = {
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
