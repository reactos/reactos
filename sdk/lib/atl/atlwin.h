/*
 * ReactOS ATL
 *
 * Copyright 2009 Andrew Hill <ash77@reactos.org>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#if defined(__GNUC__) || defined(__clang__)
#define GCCU(x)    x __attribute__((unused))
#define Unused(x)
#else
#define GCCU(x) (x)
#define Unused(x)    (x);
#endif // __GNUC__

#if !defined(_WIN64)
#ifdef SetWindowLongPtr
#undef SetWindowLongPtr
inline LONG_PTR SetWindowLongPtr(HWND hWnd, int nIndex, LONG_PTR dwNewLong)
{
    return SetWindowLong(hWnd, nIndex, (LONG)dwNewLong);
}
#endif

#ifdef GetWindowLongPtr
#undef GetWindowLongPtr
inline LONG_PTR GetWindowLongPtr(HWND hWnd, int nIndex)
{
    return (LONG_PTR)GetWindowLong(hWnd, nIndex);
}
#endif
#endif // !_WIN64

#pragma push_macro("SubclassWindow")
#undef SubclassWindow

namespace ATL
{

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif


struct _ATL_WNDCLASSINFOW;
typedef _ATL_WNDCLASSINFOW CWndClassInfo;

template <DWORD t_dwStyle = 0, DWORD t_dwExStyle = 0>
class CWinTraits
{
public:
    static DWORD GetWndStyle(DWORD dwStyle)
    {
        if (dwStyle == 0)
            return t_dwStyle;
        return dwStyle;
    }

    static DWORD GetWndExStyle(DWORD dwExStyle)
    {
        if (dwExStyle == 0)
            return t_dwExStyle;
        return dwExStyle;
    }
};

typedef CWinTraits<WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0> CControlWinTraits;
typedef CWinTraits<WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE> CFrameWinTraits;
typedef CWinTraits<WS_OVERLAPPEDWINDOW | WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_MDICHILD> CMDIChildWinTraits;

template <DWORD t_dwStyle = 0, DWORD t_dwExStyle = 0, class TWinTraits = CControlWinTraits>
class CWinTraitsOR
{
public:
    static DWORD GetWndStyle(DWORD dwStyle)
    {
        return dwStyle | t_dwStyle | TWinTraits::GetWndStyle(dwStyle);
    }

    static DWORD GetWndExStyle(DWORD dwExStyle)
    {
        return dwExStyle | t_dwExStyle | TWinTraits::GetWndExStyle(dwExStyle);
    }
};

class _U_MENUorID
{
public:
    HMENU m_hMenu;
public:
    _U_MENUorID(HMENU hMenu)
    {
        m_hMenu = hMenu;
    }

    _U_MENUorID(UINT nID)
    {
        m_hMenu = (HMENU)(UINT_PTR)nID;
    }
};

class _U_RECT
{
public:
    LPRECT m_lpRect;
public:
    _U_RECT(LPRECT lpRect)
    {
        m_lpRect = lpRect;
    }

    _U_RECT(RECT &rc)
    {
        m_lpRect = &rc;
    }
};

struct _ATL_MSG : public MSG
{
public:
    BOOL bHandled;
public:
    _ATL_MSG(HWND hWnd, UINT uMsg, WPARAM wParamIn, LPARAM lParamIn, BOOL bHandledIn = TRUE)
    {
        hwnd = hWnd;
        message = uMsg;
        wParam = wParamIn;
        lParam = lParamIn;
        time = 0;
        pt.x = 0;
        pt.y = 0;
        bHandled = bHandledIn;
    }
};

#if defined(_M_IX86)

#pragma pack(push,1)
struct thunkCode
{
    DWORD  m_mov; /* mov dword ptr [esp+4], m_this */
    DWORD  m_this;
    BYTE   m_jmp; /* jmp relproc */
    DWORD  m_relproc;

    void
    Init(WNDPROC proc, void *pThis)
    {
        m_mov = 0x042444C7;
        m_this = PtrToUlong(pThis);
        m_jmp = 0xe9;
        m_relproc = DWORD(reinterpret_cast<char*>(proc) - (reinterpret_cast<char*>(this) + sizeof(thunkCode)));
        FlushInstructionCache(GetCurrentProcess(), this, sizeof(thunkCode));
    }
};
#pragma pack(pop)

#elif defined(_AMD64_)

#pragma pack(push,1)
struct thunkCode
{
    USHORT  m_mov_rcx; /* mov rcx, m_this */
    ULONG64 m_this;
    USHORT  m_mov_rax; /* mov rax, m_proc */
    ULONG64 m_proc;
    USHORT  m_jmp;    /* jmp rax */

    void
    Init(WNDPROC proc, void *pThis)
    {
        m_mov_rcx = 0xb948;
        m_this = (ULONG64)pThis;
        m_mov_rax = 0xb848;
        m_proc = (ULONG64)proc;
        m_jmp = 0xe0ff;
        FlushInstructionCache(GetCurrentProcess(), this, sizeof(thunkCode));
    }
};
#pragma pack(pop)

#elif defined(_M_ARM)

#pragma pack(push,4)
struct thunkCode
{
    DWORD m_mov_r0; /* mov r0, m_this */
    DWORD m_mov_pc; /* mov pc, m_proc */
    DWORD m_this;
    DWORD m_proc;

    void
    Init(WNDPROC proc, void *pThis)
    {
        m_mov_r0 = 0xE59F0000;
        m_mov_pc = 0xE59FF000;
        m_this = (DWORD)pThis;
        m_proc = (DWORD)proc;
        FlushInstructionCache(GetCurrentProcess(), this, sizeof(thunkCode));
    }
};
#pragma pack(pop)

#else
#error ARCH not supported
#endif

class CWndProcThunk
{
public:
    thunkCode *m_pthunk;
    _AtlCreateWndData cd;

public:
    CWndProcThunk()
    {
        m_pthunk = (thunkCode*)VirtualAlloc(NULL, sizeof(thunkCode), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    }

    ~CWndProcThunk()
    {
        if (m_pthunk != NULL)
            VirtualFree(m_pthunk, 0, MEM_RELEASE);
    }

    BOOL Init(WNDPROC proc, void *pThis)
    {
        if (m_pthunk == NULL)
            return FALSE;
        m_pthunk->Init(proc, pThis);
        return TRUE;
    }

    WNDPROC GetWNDPROC()
    {
        return reinterpret_cast<WNDPROC>(m_pthunk);
    }
};

class CMessageMap
{
public:
    virtual BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &lResult, DWORD dwMsgMapID) = 0;
};

class CWindow
{
public:
    HWND m_hWnd;
    static RECT rcDefault;

public:
    CWindow(HWND hWnd = NULL)
    {
        m_hWnd = hWnd;
    }

    operator HWND() const
    {
        return m_hWnd;
    }

    static LPCTSTR GetWndClassName()
    {
        return NULL;
    }

    UINT ArrangeIconicWindows()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::ArrangeIconicWindows(m_hWnd);
    }

    void Attach(HWND hWndNew)
    {
        m_hWnd = hWndNew;
    }

    HDC BeginPaint(LPPAINTSTRUCT lpPaint)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::BeginPaint(m_hWnd, lpPaint);
    }

    BOOL BringWindowToTop()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::BringWindowToTop(m_hWnd);
    }

    BOOL CenterWindow(HWND hWndCenter = NULL)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        if (hWndCenter == NULL)
            hWndCenter = ::GetParent(m_hWnd);
        if (hWndCenter == NULL)
            return FALSE;
        RECT wndCenterRect, wndRect;
        if (!::GetWindowRect(hWndCenter, &wndCenterRect) || !::GetWindowRect(m_hWnd, &wndRect))
            return FALSE;
        int wndCenterWidth = wndCenterRect.right - wndCenterRect.left;
        int wndCenterHeight = wndCenterRect.bottom - wndCenterRect.top;
        int wndWidth = wndRect.right - wndRect.left;
        int wndHeight = wndRect.bottom - wndRect.top;
        return ::MoveWindow(m_hWnd,
                            wndCenterRect.left + ((wndCenterWidth - wndWidth + 1) >> 1),
                            wndCenterRect.top + ((wndCenterHeight - wndHeight + 1) >> 1),
                            wndWidth, wndHeight, TRUE);
    }

    BOOL ChangeClipboardChain(HWND hWndNewNext)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::ChangeClipboardChain(m_hWnd, hWndNewNext);
    }

    BOOL CheckDlgButton(int nIDButton, UINT nCheck)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::CheckDlgButton(m_hWnd, nIDButton, nCheck);
    }

    BOOL CheckRadioButton(int nIDFirstButton, int nIDLastButton, int nIDCheckButton)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::CheckRadioButton(m_hWnd, nIDFirstButton, nIDLastButton, nIDCheckButton);
    }

    HWND ChildWindowFromPoint(POINT point) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::ChildWindowFromPoint(m_hWnd, point);
    }

    HWND ChildWindowFromPointEx(POINT point, UINT uFlags) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::ChildWindowFromPointEx(m_hWnd, point, uFlags);
    }

    BOOL ClientToScreen(LPPOINT lpPoint) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::ClientToScreen(m_hWnd, lpPoint);
    }

    BOOL ClientToScreen(LPRECT lpRect) const
    {
        if (lpRect == NULL)
            return FALSE;
        ATLASSERT(::IsWindow(m_hWnd));
        POINT leftTop = {lpRect->left, lpRect->top};
        POINT rightBottom = {lpRect->right, lpRect->bottom};
        BOOL success = ::ClientToScreen(m_hWnd, &leftTop) && ::ClientToScreen(m_hWnd, &rightBottom);
        if (success)
        {
            lpRect->left = leftTop.x;
            lpRect->top = leftTop.y;
            lpRect->right = rightBottom.x;
            lpRect->bottom = rightBottom.y;
        }
        return success;
    }

    HWND Create(LPCTSTR lpstrWndClass, HWND hWndParent, _U_RECT rect = NULL, LPCTSTR szWindowName = NULL, DWORD dwStyle = 0, DWORD dwExStyle = 0, _U_MENUorID MenuOrID = 0U, LPVOID lpCreateParam = NULL)
    {
        HWND hWnd;
        ATLASSERT(m_hWnd == NULL);
        hWnd = ::CreateWindowEx(dwExStyle,
                                lpstrWndClass,
                                szWindowName,
                                dwStyle,
                                rect.m_lpRect->left,
                                rect.m_lpRect->top,
                                rect.m_lpRect->right - rect.m_lpRect->left,
                                rect.m_lpRect->bottom - rect.m_lpRect->top,
                                hWndParent,
                                MenuOrID.m_hMenu,
                                _AtlBaseModule.GetModuleInstance(),
                                lpCreateParam);
        if (hWnd != NULL)
            m_hWnd = hWnd;
        return hWnd;
    }

    BOOL CreateCaret(HBITMAP pBitmap)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::CreateCaret(m_hWnd, pBitmap, 0, 0);
    }

    BOOL CreateGrayCaret(int nWidth, int nHeight)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::CreateCaret(m_hWnd, (HBITMAP)1, nWidth, nHeight);
    }

    BOOL CreateSolidCaret(int nWidth, int nHeight)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::CreateCaret(m_hWnd, (HBITMAP)0, nWidth, nHeight);
    }

    HDWP DeferWindowPos(HDWP hWinPosInfo, HWND hWndInsertAfter, int x, int y, int cx, int cy, UINT uFlags)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::DeferWindowPos(hWinPosInfo, m_hWnd, hWndInsertAfter, x, y, cx, cy, uFlags);
    }

    BOOL DestroyWindow()
    {
        ATLASSERT(::IsWindow(m_hWnd));

        if (!::DestroyWindow(m_hWnd))
            return FALSE;

        m_hWnd = NULL;
        return TRUE;
    }

    HWND Detach()
    {
        HWND hWnd = m_hWnd;
        m_hWnd = NULL;
        return hWnd;
    }

    int DlgDirList(LPTSTR lpPathSpec, int nIDListBox, int nIDStaticPath, UINT nFileType)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::DlgDirList(m_hWnd, lpPathSpec, nIDListBox, nIDStaticPath, nFileType);
    }

    int DlgDirListComboBox(LPTSTR lpPathSpec, int nIDComboBox, int nIDStaticPath, UINT nFileType)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::DlgDirListComboBox(m_hWnd, lpPathSpec, nIDComboBox, nIDStaticPath, nFileType);
    }

    BOOL DlgDirSelect(LPTSTR lpString, int nCount, int nIDListBox)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::DlgDirSelectEx(m_hWnd, lpString, nCount, nIDListBox);
    }

    BOOL DlgDirSelectComboBox(LPTSTR lpString, int nCount, int nIDComboBox)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::DlgDirSelectComboBoxEx(m_hWnd, lpString, nCount, nIDComboBox);
    }

    void DragAcceptFiles(BOOL bAccept = TRUE)
    {
        ATLASSERT(::IsWindow(m_hWnd));
// FIXME following line requires shellapi.h
//         ::DragAcceptFiles(m_hWnd, bAccept);
    }

    BOOL DrawMenuBar()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::DrawMenuBar(m_hWnd);
    }

    BOOL EnableScrollBar(UINT uSBFlags, UINT uArrowFlags = ESB_ENABLE_BOTH)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::EnableScrollBar(m_hWnd, uSBFlags, uArrowFlags);
    }

    BOOL EnableWindow(BOOL bEnable = TRUE)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::EnableWindow(m_hWnd, bEnable);
    }

    void EndPaint(LPPAINTSTRUCT lpPaint)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        ::EndPaint(m_hWnd, lpPaint);
    }

    BOOL FlashWindow(BOOL bInvert)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::FlashWindow(m_hWnd, bInvert);
    }

    BOOL GetClientRect(LPRECT lpRect) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetClientRect(m_hWnd, lpRect);
    }

    HDC GetDC()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetDC(m_hWnd);
    }

    HDC GetDCEx(HRGN hRgnClip, DWORD flags)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetDCEx(m_hWnd, hRgnClip, flags);
    }

private:
    typedef struct _IDHWNDPAIR {
        int nID;
        HWND hWnd;
    } IDHWNDPAIR, *PIDHWNDPAIR;

    static BOOL CALLBACK GetDescendantWindowCallback(HWND hWnd, LPARAM lParam)
    {
        if (::GetWindowLong(hWnd, GWL_ID) == ((PIDHWNDPAIR)lParam)->nID)
        {
            ((PIDHWNDPAIR)lParam)->hWnd = hWnd;
            return FALSE;
        }
        ::EnumChildWindows(hWnd, &GetDescendantWindowCallback, lParam);
        return (((PIDHWNDPAIR)lParam)->hWnd == NULL);
    }

public:
    HWND GetDescendantWindow(int nID) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        IDHWNDPAIR idHWndPair;
        idHWndPair.nID = nID;
        idHWndPair.hWnd = NULL;
        ::EnumChildWindows(m_hWnd, &GetDescendantWindowCallback, (LPARAM)&idHWndPair);
        return idHWndPair.hWnd;
    }

    HRESULT GetDlgControl(int nID, REFIID iid, void** ppCtrl)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return E_FAIL;//FIXME stub
    }

    int GetDlgCtrlID() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetDlgCtrlID(m_hWnd);
    }

    HRESULT GetDlgHost(int nID, REFIID iid, void** ppHost)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return E_FAIL;//FIXME stub
    }

    HWND GetDlgItem(_In_ int nID) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetDlgItem(m_hWnd, nID);
    }

    UINT GetDlgItemInt(
        _In_ int nID,
        _Out_opt_ BOOL* lpTrans = NULL,
        _In_ BOOL bSigned = TRUE) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetDlgItemInt(m_hWnd, nID, lpTrans, bSigned);
    }

    UINT GetDlgItemText(
        _In_ int nID,
        _Out_writes_to_(nMaxCount, return + 1) LPTSTR lpStr,
        _In_ int nMaxCount) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetDlgItemText(m_hWnd, nID, lpStr, nMaxCount);
    }

#ifdef __ATLSTR_H__
    UINT GetDlgItemText(_In_ int nID, _Inout_ CSimpleString& strText) const
    {
        HWND item = GetDlgItem(nID);
        if (!item)
        {
            strText.Empty();
            return 0;
        }
        return CWindow(item).GetWindowText(strText);
    }
#endif

    BOOL GetDlgItemText(
        _In_ int nID,
        _Inout_ _Outref_result_maybenull_ _Post_z_ BSTR& bstrText) const
    {
        HWND item = GetDlgItem(nID);
        if (!item)
            return FALSE;
        return CWindow(item).GetWindowText(bstrText);
    }

    DWORD GetExStyle() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetWindowLong(m_hWnd, GWL_EXSTYLE);
    }

    HFONT GetFont() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (HFONT)::SendMessage(m_hWnd, WM_GETFONT, 0, 0);
    }

    DWORD GetHotKey() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (DWORD)::SendMessage(m_hWnd, WM_GETHOTKEY, 0, 0);
    }

    HICON GetIcon(BOOL bBigIcon = TRUE) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (HICON)::SendMessage(m_hWnd, WM_GETICON, (WPARAM)bBigIcon, 0);
    }

    HWND GetLastActivePopup() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetLastActivePopup(m_hWnd);
    }

    HMENU GetMenu() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetMenu(m_hWnd);
    }

    HWND GetNextDlgGroupItem(HWND hWndCtl, BOOL bPrevious = FALSE) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetNextDlgGroupItem(m_hWnd, hWndCtl, bPrevious);
    }

    HWND GetNextDlgTabItem(HWND hWndCtl, BOOL bPrevious = FALSE) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetNextDlgTabItem(m_hWnd, hWndCtl, bPrevious);
    }

    CWindow GetParent() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return CWindow(::GetParent(m_hWnd));
    }

    BOOL GetScrollInfo(int nBar, LPSCROLLINFO lpScrollInfo)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetScrollInfo(m_hWnd, nBar, lpScrollInfo);
    }

    BOOL GetScrollPos(int nBar)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetScrollPos(m_hWnd, nBar);
    }

    BOOL GetScrollRange(int nBar, LPINT lpMinPos, LPINT lpMaxPos) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetScrollRange(m_hWnd, nBar, lpMinPos, lpMaxPos);
    }

    DWORD GetStyle() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetWindowLong(m_hWnd, GWL_STYLE);
    }

    HMENU GetSystemMenu(BOOL bRevert)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetSystemMenu(m_hWnd, bRevert);
    }

    HWND GetTopLevelParent() const
    {
        ATLASSERT(::IsWindow(m_hWnd));

        HWND hWndParent = m_hWnd;
        HWND hWndTmp;
        while ((hWndTmp = ::GetParent(hWndParent)) != NULL)
            hWndParent = hWndTmp;

        return hWndParent;
    }

    HWND GetTopLevelWindow() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return NULL;//FIXME stub
    }

    HWND GetTopWindow() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetTopWindow(m_hWnd);
    }

    BOOL GetUpdateRect(LPRECT lpRect, BOOL bErase = FALSE)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetUpdateRect(m_hWnd, lpRect, bErase);
    }

    int GetUpdateRgn(HRGN hRgn, BOOL bErase = FALSE)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return :: GetUpdateRgn(m_hWnd, hRgn, bErase);
    }

    HWND GetWindow(UINT nCmd) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetWindow(m_hWnd, nCmd);
    }

    DWORD GetWindowContextHelpId() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetWindowContextHelpId(m_hWnd);
    }

    HDC GetWindowDC()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetWindowDC(m_hWnd);
    }

    LONG GetWindowLong(int nIndex) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetWindowLong(m_hWnd, nIndex);
    }

    LONG_PTR GetWindowLongPtr(int nIndex) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetWindowLongPtr(m_hWnd, nIndex);
    }

    BOOL GetWindowPlacement(WINDOWPLACEMENT* lpwndpl) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetWindowPlacement(m_hWnd, lpwndpl);
    }

    DWORD GetWindowProcessID()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        DWORD processID;
        ::GetWindowThreadProcessId(m_hWnd, &processID);
        return processID;
    }

    BOOL GetWindowRect(LPRECT lpRect) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetWindowRect(m_hWnd, lpRect);
    }

    int GetWindowRgn(HRGN hRgn)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetWindowRgn(m_hWnd, hRgn);
    }

    int GetWindowText(
        _Out_writes_to_(nMaxCount, return + 1) LPTSTR lpszStringBuf,
        _In_ int nMaxCount) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetWindowText(m_hWnd, lpszStringBuf, nMaxCount);
    }

#ifdef __ATLSTR_H__
    int GetWindowText(_Inout_ CSimpleString& strText) const
    {
        int len = GetWindowTextLength();
        len = GetWindowText(strText.GetBuffer(len + 1), len + 1);
        strText.ReleaseBuffer(len);
        return len;
    }
#endif

    BOOL GetWindowText(
        _Inout_ _Outref_result_maybenull_ _Post_z_ BSTR& bstrText) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        INT length = ::GetWindowTextLengthW(m_hWnd);
        if (!::SysReAllocStringLen(&bstrText, NULL, length))
            return FALSE;
        if (::GetWindowTextW(m_hWnd, bstrText, length + 1))
            return TRUE;
        ::SysFreeString(bstrText);
        bstrText = NULL;
        return FALSE;
    }

    int GetWindowTextLength() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetWindowTextLength(m_hWnd);
    }

    DWORD GetWindowThreadID()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetWindowThreadProcessId(m_hWnd, NULL);
    }

    WORD GetWindowWord(int nIndex) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (WORD)::GetWindowLong(m_hWnd, nIndex);
    }

    void GotoDlgCtrl(HWND hWndCtrl) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        ::SendMessage(m_hWnd, WM_NEXTDLGCTL, 0, 0);
    }

    BOOL HideCaret()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::HideCaret(m_hWnd);
    }

    BOOL HiliteMenuItem(HMENU hMenu, UINT uHiliteItem, UINT uHilite)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::HiliteMenuItem(m_hWnd, hMenu, uHiliteItem, uHilite);
    }

    BOOL Invalidate(BOOL bErase = TRUE)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::InvalidateRect(m_hWnd, NULL, bErase);
    }

    BOOL InvalidateRect(LPCRECT lpRect, BOOL bErase = TRUE)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::InvalidateRect(m_hWnd, lpRect, bErase);
    }

    void InvalidateRgn(HRGN hRgn, BOOL bErase = TRUE)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        ::InvalidateRgn(m_hWnd, hRgn, bErase);
    }

    BOOL IsChild(const HWND hWnd) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::IsChild(m_hWnd, hWnd);
    }

    BOOL IsDialogMessage(LPMSG lpMsg)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::IsDialogMessage(m_hWnd, lpMsg);
    }

    UINT IsDlgButtonChecked(int nIDButton) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::IsDlgButtonChecked(m_hWnd, nIDButton);
    }

    BOOL IsIconic() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::IsIconic(m_hWnd);
    }

    BOOL IsParentDialog()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        TCHAR pszType[10]; // Use sizeof("#32770")+3 so that extra characters can be detected.
        if (!RealGetWindowClass(::GetParent(m_hWnd), pszType, _countof(pszType)))
            return FALSE;
        return !_tcscmp(pszType, _T("#32770"));
    }

    BOOL IsWindow() const
    {
        return ::IsWindow(m_hWnd);
    }

    BOOL IsWindowEnabled() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::IsWindowEnabled(m_hWnd);
    }

    BOOL IsWindowVisible() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::IsWindowVisible(m_hWnd);
    }

    BOOL IsWindowUnicode()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::IsWindowUnicode(m_hWnd);
    }

    BOOL IsZoomed() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::IsZoomed(m_hWnd);
    }

    BOOL KillTimer(UINT_PTR nIDEvent)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::KillTimer(m_hWnd, nIDEvent);
    }

    BOOL LockWindowUpdate(BOOL bLock = TRUE)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        if (bLock)
            return ::LockWindowUpdate(m_hWnd);
        return ::LockWindowUpdate(NULL);
    }

    int MapWindowPoints(HWND hWndTo, LPPOINT lpPoint, UINT nCount) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::MapWindowPoints(m_hWnd, hWndTo, lpPoint, nCount);
    }

    int MapWindowPoints(HWND hWndTo, LPRECT lpRect) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::MapWindowPoints(m_hWnd, hWndTo, (LPPOINT)lpRect, sizeof(RECT) / sizeof(POINT));
    }

    int MessageBox(LPCTSTR lpszText, LPCTSTR lpszCaption = NULL, UINT nType = MB_OK)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::MessageBox(m_hWnd, lpszText, lpszCaption, nType);
    }

    BOOL ModifyStyle(DWORD dwRemove, DWORD dwAdd, UINT nFlags = 0)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        ::SetWindowLong(m_hWnd, GWL_STYLE, (::GetWindowLong(m_hWnd, GWL_STYLE) & ~dwRemove) | dwAdd);
        if (nFlags != 0)
            return ::SetWindowPos(m_hWnd, NULL, 0, 0, 0, 0, nFlags | SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        return TRUE;
    }

    BOOL ModifyStyleEx(DWORD dwRemove, DWORD dwAdd, UINT nFlags = 0)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        ::SetWindowLong(m_hWnd, GWL_EXSTYLE, (::GetWindowLong(m_hWnd, GWL_EXSTYLE) & ~dwRemove) | dwAdd);
        if (nFlags != 0)
            return ::SetWindowPos(m_hWnd, NULL, 0, 0, 0, 0, nFlags | SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        return TRUE;
    }

    BOOL MoveWindow(int x, int y, int nWidth, int nHeight, BOOL bRepaint = TRUE)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::MoveWindow(m_hWnd, x, y, nWidth, nHeight, bRepaint);
    }

    void NextDlgCtrl() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        ::SendMessage(m_hWnd, WM_NEXTDLGCTL, 0, 0);
    }

    BOOL OpenClipboard()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::OpenClipboard(m_hWnd);
    }

    BOOL PostMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::PostMessage(m_hWnd, message, wParam, lParam);
    }

    void PrevDlgCtrl() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        ::SendMessage(m_hWnd, WM_NEXTDLGCTL, 1, 0);
    }

    void Print(HDC hDC, DWORD dwFlags) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        ::SendMessage(m_hWnd, WM_PRINT, (WPARAM)hDC, (LPARAM)dwFlags);
    }

    void PrintClient(HDC hDC, DWORD dwFlags) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        ::SendMessage(m_hWnd, WM_PRINTCLIENT, (WPARAM)hDC, (LPARAM)dwFlags);
    }

    BOOL RedrawWindow(LPCRECT lpRectUpdate = NULL, HRGN hRgnUpdate = NULL, UINT flags = RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::RedrawWindow(m_hWnd, lpRectUpdate, hRgnUpdate, flags);
    }

    int ReleaseDC(HDC hDC)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::ReleaseDC(m_hWnd, hDC);
    }

    BOOL ResizeClient(int nWidth, int nHeight, BOOL bRedraw = FALSE)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        RECT clientRect, wndRect;
        ::GetClientRect(m_hWnd, &clientRect);
        ::GetWindowRect(m_hWnd, &wndRect);
        return ::MoveWindow(m_hWnd, wndRect.left, wndRect.top,
                            nWidth + (wndRect.right - wndRect.left) - (clientRect.right - clientRect.left),
                            nHeight + (wndRect.bottom - wndRect.top) - (clientRect.bottom - clientRect.top),
                            bRedraw);
    }

    BOOL ScreenToClient(LPPOINT lpPoint) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::ScreenToClient(m_hWnd, lpPoint);
    }

    BOOL ScrollWindow(int xAmount, int yAmount, LPCRECT lpRect = NULL, LPCRECT lpClipRect = NULL)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::ScrollWindow(m_hWnd, xAmount, yAmount, lpRect, lpClipRect);
    }

    int ScrollWindowEx(int dx, int dy, LPCRECT lpRectScroll, LPCRECT lpRectClip, HRGN hRgnUpdate, LPRECT lpRectUpdate, UINT flags)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::ScrollWindowEx(m_hWnd, dx, dy, lpRectScroll, lpRectClip, hRgnUpdate, lpRectUpdate, flags);
    }

    LRESULT SendDlgItemMessage(int nID, UINT message, WPARAM wParam = 0, LPARAM lParam = 0)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::SendDlgItemMessage(m_hWnd, nID, message, wParam, lParam);
    }

    LRESULT SendMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::SendMessage(m_hWnd, message, wParam, lParam);
    }

    static LRESULT SendMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        ATLASSERT(::IsWindow(hWnd));
        return ::SendMessage(hWnd, message, wParam, lParam);
    }

private:
    static BOOL CALLBACK SendMessageToDescendantsCallback(HWND hWnd, LPARAM lParam)
    {
        ::SendMessage(hWnd, ((LPMSG)lParam)->message, ((LPMSG)lParam)->wParam, ((LPMSG)lParam)->lParam);
        return TRUE;
    }

    static BOOL CALLBACK SendMessageToDescendantsCallbackDeep(HWND hWnd, LPARAM lParam)
    {
        ::SendMessage(hWnd, ((LPMSG)lParam)->message, ((LPMSG)lParam)->wParam, ((LPMSG)lParam)->lParam);
        ::EnumChildWindows(hWnd, &SendMessageToDescendantsCallbackDeep, lParam);
        return TRUE;
    }

public:
    void SendMessageToDescendants(UINT message, WPARAM wParam = 0, LPARAM lParam = 0, BOOL bDeep = TRUE)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        MSG msg;
        msg.message = message;
        msg.wParam = wParam;
        msg.lParam = lParam;
        if (bDeep)
            ::EnumChildWindows(m_hWnd, &SendMessageToDescendantsCallback, (LPARAM)&msg);
        else
            ::EnumChildWindows(m_hWnd, &SendMessageToDescendantsCallbackDeep, (LPARAM)&msg);
    }

    BOOL SendNotifyMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::SendNotifyMessage(m_hWnd, message, wParam, lParam);
    }

    HWND SetActiveWindow()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::SetActiveWindow(m_hWnd);
    }

    HWND SetCapture()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::SetCapture(m_hWnd);
    }

    HWND SetClipboardViewer()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::SetClipboardViewer(m_hWnd);
    }

    int SetDlgCtrlID(int nID)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::SetWindowLong(m_hWnd, GWL_ID, nID);
    }

    BOOL SetDlgItemInt(int nID, UINT nValue, BOOL bSigned = TRUE)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::SetDlgItemInt(m_hWnd, nID, nValue, bSigned);
    }

    BOOL SetDlgItemText(int nID, LPCTSTR lpszString)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::SetDlgItemText(m_hWnd, nID, lpszString);
    }

    HWND SetFocus()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::SetFocus(m_hWnd);
    }

    void SetFont(HFONT hFont, BOOL bRedraw = TRUE)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        ::SendMessage(m_hWnd, WM_SETFONT, (WPARAM)hFont, (LPARAM)bRedraw);
    }

    int SetHotKey(WORD wVirtualKeyCode, WORD wModifiers)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::SendMessage(m_hWnd, WM_SETHOTKEY, MAKEWPARAM(wVirtualKeyCode, wModifiers), 0);
    }

    HICON SetIcon(HICON hIcon, BOOL bBigIcon = TRUE)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (HICON)::SendMessage(m_hWnd, WM_SETICON, (WPARAM)bBigIcon, (LPARAM)hIcon);
    }

    BOOL SetMenu(HMENU hMenu)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::SetMenu(m_hWnd, hMenu);
    }

    HWND SetParent(HWND hWndNewParent)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::SetParent(m_hWnd, hWndNewParent);
    }

    void SetRedraw(BOOL bRedraw = TRUE)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        ::SendMessage(m_hWnd, WM_SETREDRAW, (WPARAM)bRedraw, 0);
    }

    int SetScrollInfo(int nBar, LPSCROLLINFO lpScrollInfo, BOOL bRedraw = TRUE)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::SetScrollInfo(m_hWnd, nBar, lpScrollInfo, bRedraw);
    }

    int SetScrollPos(int nBar, int nPos, BOOL bRedraw = TRUE)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::SetScrollPos(m_hWnd, nBar, nPos, bRedraw);
    }

    BOOL SetScrollRange(int nBar, int nMinPos, int nMaxPos, BOOL bRedraw = TRUE)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::SetScrollRange(m_hWnd, nBar, nMinPos, nMaxPos, bRedraw);
    }

    UINT_PTR SetTimer(UINT_PTR nIDEvent, UINT nElapse, void (CALLBACK *lpfnTimer)(HWND, UINT, UINT_PTR, DWORD) = NULL)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::SetTimer(m_hWnd, nIDEvent, nElapse, reinterpret_cast<TIMERPROC>(lpfnTimer));
    }

    BOOL SetWindowContextHelpId(DWORD dwContextHelpId)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::SetWindowContextHelpId(m_hWnd, dwContextHelpId);
    }

    LONG SetWindowLong(int nIndex, LONG dwNewLong)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::SetWindowLong(m_hWnd, nIndex, dwNewLong);
    }

    LONG_PTR SetWindowLongPtr(int nIndex, LONG_PTR dwNewLong)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::SetWindowLongPtr(m_hWnd, nIndex, dwNewLong);
    }

    BOOL SetWindowPlacement(const WINDOWPLACEMENT* lpwndpl)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::SetWindowPlacement(m_hWnd, lpwndpl);
    }

    BOOL SetWindowPos(HWND hWndInsertAfter, int x, int y, int cx, int cy, UINT nFlags)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::SetWindowPos(m_hWnd, hWndInsertAfter, x, y, cx, cy, nFlags);
    }

    int SetWindowRgn(HRGN hRgn, BOOL bRedraw = FALSE)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::SetWindowRgn(m_hWnd, hRgn, bRedraw);
    }

    BOOL SetWindowText(LPCTSTR lpszString)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::SetWindowText(m_hWnd, lpszString);
    }

    WORD SetWindowWord(int nIndex, WORD wNewWord)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        if (nIndex >= -4)
            return ::SetWindowLong(m_hWnd, nIndex - 2, MAKELONG(LOWORD(::GetWindowLong(m_hWnd, nIndex - 2)), wNewWord));
        else
            return ::SetWindowLong(m_hWnd, nIndex, MAKELONG(wNewWord, HIWORD(::GetWindowLong(m_hWnd, nIndex))));
    }

    BOOL ShowCaret()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::ShowCaret(m_hWnd);
    }

    BOOL ShowOwnedPopups(BOOL bShow = TRUE)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::ShowOwnedPopups(m_hWnd, bShow);
    }

    BOOL ShowScrollBar(UINT nBar, BOOL bShow = TRUE)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::ShowScrollBar(m_hWnd, nBar, bShow);
    }

    BOOL ShowWindow(int nCmdShow)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::ShowWindow(m_hWnd, nCmdShow);
    }

    BOOL ShowWindowAsync(int nCmdShow)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::ShowWindowAsync(m_hWnd, nCmdShow);
    }

    BOOL UpdateWindow()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::UpdateWindow(m_hWnd);
    }

    BOOL ValidateRect(LPCRECT lpRect)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::ValidateRect(m_hWnd, lpRect);
    }

    BOOL ValidateRgn(HRGN hRgn)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::ValidateRgn(m_hWnd, hRgn);
    }

    BOOL WinHelp(LPCTSTR lpszHelp, UINT nCmd = HELP_CONTEXT, DWORD dwData = 0)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::WinHelp(m_hWnd, lpszHelp, nCmd, dwData);
    }
};

__declspec(selectany) RECT CWindow::rcDefault = { CW_USEDEFAULT, CW_USEDEFAULT, 0, 0 };

template <class TBase = CWindow>
class CWindowImplRoot : public TBase, public CMessageMap
{
public:
    enum { WINSTATE_DESTROYED = 0x00000001 };

public:
    CWndProcThunk m_thunk;
    const _ATL_MSG *m_pCurrentMsg;
    DWORD m_dwState;

    CWindowImplRoot()
        : m_pCurrentMsg(NULL)
        , m_dwState(0)
    {
    }

    virtual ~CWindowImplRoot()
    {
    }
};


template <class TBase = CWindow>
class CDialogImplBaseT : public CWindowImplRoot<TBase>
{
public:
    // + Hacks for gcc
    using CWindowImplRoot<TBase>::WINSTATE_DESTROYED;
    // - Hacks for gcc

    virtual ~CDialogImplBaseT()
    {
    }
    virtual DLGPROC GetDialogProc()
    {
        return DialogProc;
    }

    static INT_PTR CALLBACK StartDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        CDialogImplBaseT<TBase>* pThis;
        DLGPROC newDlgProc;
        DLGPROC GCCU(pOldProc);

        pThis = reinterpret_cast<CDialogImplBaseT<TBase>*>(_AtlWinModule.ExtractCreateWndData());
        ATLASSERT(pThis != NULL);
        if (pThis == NULL)
            return 0;

        pThis->m_thunk.Init((WNDPROC)pThis->GetDialogProc(), pThis);
        newDlgProc = reinterpret_cast<DLGPROC>(pThis->m_thunk.GetWNDPROC());
        pOldProc = reinterpret_cast<DLGPROC>(::SetWindowLongPtr(hWnd, DWLP_DLGPROC, reinterpret_cast<LONG_PTR>(newDlgProc)));
        Unused(pOldProc); // TODO: should generate trace message if overwriting another subclass
        pThis->m_hWnd = hWnd;
        return newDlgProc(hWnd, uMsg, wParam, lParam);
    }

    static INT_PTR CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        CDialogImplBaseT<TBase>* pThis = reinterpret_cast<CDialogImplBaseT<TBase>*>(hWnd);
        _ATL_MSG msg(pThis->m_hWnd, uMsg, wParam, lParam);
        LRESULT lResult = 0;
        const _ATL_MSG *previousMessage;
        BOOL handled;

        hWnd = pThis->m_hWnd;
        previousMessage = pThis->m_pCurrentMsg;
        pThis->m_pCurrentMsg = &msg;

        handled = pThis->ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult, 0);
        ATLASSERT(pThis->m_pCurrentMsg == &msg);

        if (handled)
        {
            if ((pThis->m_dwState & WINSTATE_DESTROYED) == 0)
            {
                ::SetWindowLongPtr(pThis->m_hWnd, DWLP_MSGRESULT, lResult);
            }
        }
        else
        {
            if (uMsg == WM_NCDESTROY)
            {
                pThis->m_dwState |= WINSTATE_DESTROYED;
            }
        }

        ATLASSERT(pThis->m_pCurrentMsg == &msg);
        pThis->m_pCurrentMsg = previousMessage;

        if (previousMessage == NULL && (pThis->m_dwState & WINSTATE_DESTROYED) != 0)
        {
            pThis->m_dwState &= ~WINSTATE_DESTROYED;
            pThis->m_hWnd = NULL;
            pThis->OnFinalMessage(hWnd);
        }
        return lResult;
    }

    virtual void OnFinalMessage(HWND)
    {
    }
};


template <class T, class TBase = CWindow>
class CDialogImpl : public CDialogImplBaseT<TBase>
{
public:
    // + Hacks for gcc
    using CWindowImplRoot<TBase>::m_thunk;
    using CWindowImplRoot<TBase>::m_hWnd;
    // - Hacks for gcc

    HWND Create(HWND hWndParent, LPARAM dwInitParam = NULL)
    {
        BOOL result;
        HWND hWnd;
        T* pImpl;

        result = m_thunk.Init(NULL, NULL);
        if (result == FALSE)
            return NULL;

        _AtlWinModule.AddCreateWndData(&m_thunk.cd, this);

        pImpl = static_cast<T*>(this);
        hWnd = ::CreateDialogParam(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(pImpl->IDD), hWndParent, T::StartDialogProc, dwInitParam);
        return hWnd;
    }

    INT_PTR DoModal(HWND hWndParent = ::GetActiveWindow(), LPARAM dwInitParam = NULL)
    {
        BOOL result;
        T* pImpl;

        result = m_thunk.Init(NULL, NULL);
        if (result == FALSE)
            return -1;

        _AtlWinModule.AddCreateWndData(&m_thunk.cd, this);

        pImpl = static_cast<T*>(this);
        return ::DialogBoxParam(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(pImpl->IDD), hWndParent, T::StartDialogProc, dwInitParam);
    }

    BOOL EndDialog(_In_ int nRetCode)
    {
        return ::EndDialog(m_hWnd, nRetCode);
    }

    BOOL DestroyWindow()
    {
        return ::DestroyWindow(m_hWnd);
    }
};

template <class TBase = CWindow, class TWinTraits = CControlWinTraits>
class CWindowImplBaseT : public CWindowImplRoot<TBase>
{
public:
    // + Hacks for gcc
    using CWindowImplRoot<TBase>::WINSTATE_DESTROYED;
    using CWindowImplRoot<TBase>::m_thunk;
    using CWindowImplRoot<TBase>::m_hWnd;
    // - Hacks for gcc

    WNDPROC m_pfnSuperWindowProc;

public:
    CWindowImplBaseT()
    {
        m_pfnSuperWindowProc = ::DefWindowProc;
    }

    virtual void OnFinalMessage(HWND /* hWnd */)
    {
    }

    BOOL SubclassWindow(HWND hWnd)
    {
        ATLASSERT(m_hWnd == NULL);
        ATLASSERT(::IsWindow(hWnd));

        CWindowImplBaseT<TBase, TWinTraits>* pThis;
        pThis = reinterpret_cast<CWindowImplBaseT<TBase, TWinTraits>*>(this);

        BOOL result = m_thunk.Init(GetWindowProc(), this);
        if (result == FALSE)
            return FALSE;

        WNDPROC newWindowProc = m_thunk.GetWNDPROC();
        WNDPROC oldWindowProc = reinterpret_cast<WNDPROC>(
            ::SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(newWindowProc)));
        if (oldWindowProc == NULL)
            return FALSE;

        pThis->m_pfnSuperWindowProc = oldWindowProc;
        pThis->m_hWnd = hWnd;
        return TRUE;
    }

    HWND UnsubclassWindow(BOOL bForce = FALSE)
    {
        ATLASSERT(m_hWnd != NULL);
        ATLASSERT(::IsWindow(m_hWnd));

        CWindowImplBaseT<TBase, TWinTraits>* pThis;
        pThis = reinterpret_cast<CWindowImplBaseT<TBase, TWinTraits>*>(this);

        HWND hwndOld = pThis->m_hWnd;
        WNDPROC oldWindowProc = m_thunk.GetWNDPROC();
        WNDPROC subclassedProc = reinterpret_cast<WNDPROC>(
            ::GetWindowLongPtr(hwndOld, GWLP_WNDPROC));
        if (!bForce && oldWindowProc != subclassedProc)
            return NULL;

        ::SetWindowLongPtr(hwndOld, GWLP_WNDPROC,
                           (LONG_PTR)pThis->m_pfnSuperWindowProc);
        pThis->m_pfnSuperWindowProc = ::DefWindowProc;
        pThis->m_hWnd = NULL;
        return hwndOld;
    }

    virtual WNDPROC GetWindowProc()
    {
        return WindowProc;
    }

    static DWORD GetWndStyle(DWORD dwStyle)
    {
        return TWinTraits::GetWndStyle(dwStyle);
    }

    static DWORD GetWndExStyle(DWORD dwExStyle)
    {
        return TWinTraits::GetWndExStyle(dwExStyle);
    }

    LRESULT DefWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        CWindowImplBaseT<TBase, TWinTraits>* pThis;
        pThis = reinterpret_cast<CWindowImplBaseT<TBase, TWinTraits>*>(this);
        return ::CallWindowProc(m_pfnSuperWindowProc, pThis->m_hWnd, uMsg, wParam, lParam);
    }

    static LRESULT CALLBACK StartWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        CWindowImplBaseT<TBase, TWinTraits>* pThis;
        WNDPROC newWindowProc;
        WNDPROC GCCU(pOldProc);

        pThis = reinterpret_cast<CWindowImplBaseT<TBase, TWinTraits>*>(_AtlWinModule.ExtractCreateWndData());
        ATLASSERT(pThis != NULL);
        if (pThis == NULL)
            return 0;

        pThis->m_thunk.Init(pThis->GetWindowProc(), pThis);
        newWindowProc = pThis->m_thunk.GetWNDPROC();
        pOldProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(newWindowProc)));
        Unused(pOldProc); // TODO: should generate trace message if overwriting another subclass
        pThis->m_hWnd = hWnd;
        return newWindowProc(hWnd, uMsg, wParam, lParam);
    }

    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        CWindowImplBaseT<TBase, TWinTraits>* pThis = reinterpret_cast<CWindowImplBaseT< TBase, TWinTraits>*>(hWnd);
        _ATL_MSG msg(pThis->m_hWnd, uMsg, wParam, lParam);
        LRESULT lResult;
        const _ATL_MSG *previousMessage;
        BOOL handled;
        LONG_PTR saveWindowProc;

        ATLASSERT(pThis != NULL);
        if (pThis == NULL)
            return 0;

        ATLASSERT((pThis->m_dwState & WINSTATE_DESTROYED) == 0);
        ATLASSERT(pThis->m_hWnd != NULL);
        if ((pThis->m_dwState & WINSTATE_DESTROYED) != 0 || pThis->m_hWnd == NULL)
            return 0;

        hWnd = pThis->m_hWnd;
        previousMessage = pThis->m_pCurrentMsg;
        pThis->m_pCurrentMsg = &msg;

        handled = pThis->ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult, 0);
        ATLASSERT(pThis->m_pCurrentMsg == &msg);

        if (handled == FALSE)
        {
            if (uMsg == WM_NCDESTROY)
            {
                saveWindowProc = ::GetWindowLongPtr(hWnd, GWLP_WNDPROC);
                lResult = pThis->DefWindowProc(uMsg, wParam, lParam);
                if (pThis->m_pfnSuperWindowProc != ::DefWindowProc && saveWindowProc == ::GetWindowLongPtr(hWnd, GWLP_WNDPROC))
                    ::SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(pThis->m_pfnSuperWindowProc));
                pThis->m_dwState |= WINSTATE_DESTROYED;
            }
            else
            {
                lResult = pThis->DefWindowProc(uMsg, wParam, lParam);
            }
        }
        ATLASSERT(pThis->m_pCurrentMsg == &msg);
        pThis->m_pCurrentMsg = previousMessage;
        if (previousMessage == NULL && (pThis->m_dwState & WINSTATE_DESTROYED) != 0)
        {
            pThis->m_dwState &= ~WINSTATE_DESTROYED;
            pThis->m_hWnd = NULL;
            pThis->OnFinalMessage(hWnd);
        }
        return lResult;
    }

    HWND Create(HWND hWndParent, _U_RECT rect, LPCTSTR szWindowName, DWORD dwStyle, DWORD dwExStyle,
                _U_MENUorID MenuOrID, ATOM atom, LPVOID lpCreateParam)
    {
        HWND hWnd;

        ATLASSERT(m_hWnd == NULL);
        ATLASSERT(atom != 0);
        if (atom == 0)
            return NULL;
        if (m_thunk.Init(NULL, NULL) == FALSE)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return NULL;
        }

        _AtlWinModule.AddCreateWndData(&m_thunk.cd, this);
        if (MenuOrID.m_hMenu == NULL && (dwStyle & WS_CHILD) != 0)
            MenuOrID.m_hMenu = (HMENU)(UINT_PTR)this;
        if (rect.m_lpRect == NULL)
            rect.m_lpRect = &TBase::rcDefault;

        hWnd = ::CreateWindowEx(dwExStyle, MAKEINTATOM(atom), szWindowName, dwStyle, rect.m_lpRect->left,
                    rect.m_lpRect->top, rect.m_lpRect->right - rect.m_lpRect->left, rect.m_lpRect->bottom - rect.m_lpRect->top,
                    hWndParent, MenuOrID.m_hMenu, _AtlBaseModule.GetModuleInstance(), lpCreateParam);

        ATLASSERT(m_hWnd == hWnd);

        return hWnd;
    }
};


template <class T, class TBase = CWindow, class TWinTraits = CControlWinTraits>
class CWindowImpl : public CWindowImplBaseT<TBase, TWinTraits>
{
public:
    // + Hacks for gcc
    using CWindowImplRoot<TBase>::m_hWnd;
    // - Hacks for gcc

    static LPCTSTR GetWndCaption()
    {
        return NULL;
    }

    HWND Create(HWND hWndParent, _U_RECT rect = NULL, LPCTSTR szWindowName = NULL, DWORD dwStyle = 0,
                DWORD dwExStyle = 0, _U_MENUorID MenuOrID = 0U, LPVOID lpCreateParam = NULL)
    {
        CWindowImplBaseT<TBase, TWinTraits>* pThis;
        ATOM atom;

        ATLASSERT(m_hWnd == NULL);
        pThis = reinterpret_cast<CWindowImplBaseT<TBase, TWinTraits>*>(this);

        if (T::GetWndClassInfo().m_lpszOrigName == NULL)
            T::GetWndClassInfo().m_lpszOrigName = pThis->GetWndClassName();
        atom = T::GetWndClassInfo().Register(&pThis->m_pfnSuperWindowProc);

        if (szWindowName == NULL)
            szWindowName = T::GetWndCaption();
        dwStyle = T::GetWndStyle(dwStyle);
        dwExStyle = T::GetWndExStyle(dwExStyle);

        return CWindowImplBaseT<TBase, TWinTraits>::Create(hWndParent, rect, szWindowName, dwStyle,
                        dwExStyle, MenuOrID, atom, lpCreateParam);
    }
};

template <class TBase = CWindow, class TWinTraits = CControlWinTraits>
class CContainedWindowT : public TBase
{
public:
    // + Hacks for gcc
    using TBase::m_hWnd;
    // - Hacks for gcc

    CWndProcThunk m_thunk;
    LPCTSTR m_lpszClassName;
    WNDPROC m_pfnSuperWindowProc;
    CMessageMap *m_pObject;
    DWORD m_dwMsgMapID;
    const _ATL_MSG *m_pCurrentMsg;

public:
    CContainedWindowT(CMessageMap *pObject, DWORD dwMsgMapID = 0)
    {
        m_lpszClassName = TBase::GetWndClassName();
        m_pfnSuperWindowProc = ::DefWindowProc;
        m_pObject = pObject;
        m_dwMsgMapID = dwMsgMapID;
        m_pCurrentMsg = NULL;
    }

    CContainedWindowT(LPCTSTR lpszClassName, CMessageMap *pObject, DWORD dwMsgMapID = 0)
    {
        m_lpszClassName = lpszClassName;
        m_pfnSuperWindowProc = ::DefWindowProc;
        m_pObject = pObject;
        m_dwMsgMapID = dwMsgMapID;
        m_pCurrentMsg = NULL;
    }

    LRESULT DefWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        return ::CallWindowProc(m_pfnSuperWindowProc, this->m_hWnd, uMsg, wParam, lParam);
    }

    BOOL SubclassWindow(HWND hWnd)
    {
        ATLASSERT(m_hWnd == NULL);
        ATLASSERT(::IsWindow(hWnd));

        CContainedWindowT<TBase>* pThis;
        pThis = reinterpret_cast<CContainedWindowT<TBase>*>(this);

        BOOL result = m_thunk.Init(WindowProc, pThis);
        if (result == FALSE)
            return FALSE;

        WNDPROC newWindowProc = m_thunk.GetWNDPROC();
        WNDPROC oldWindowProc = reinterpret_cast<WNDPROC>(
            ::SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(newWindowProc)));
        if (oldWindowProc == NULL)
            return FALSE;

        pThis->m_pfnSuperWindowProc = oldWindowProc;
        pThis->m_hWnd = hWnd;
        return TRUE;
    }

    HWND UnsubclassWindow(BOOL bForce = FALSE)
    {
        ATLASSERT(m_hWnd != NULL);
        ATLASSERT(::IsWindow(m_hWnd));

        CContainedWindowT<TBase>* pThis;
        pThis = reinterpret_cast<CContainedWindowT<TBase>*>(this);
        HWND hwndOld = pThis->m_hWnd;

        WNDPROC subclassedProc = reinterpret_cast<WNDPROC>(
            ::GetWindowLongPtr(hwndOld, GWLP_WNDPROC));
        if (!bForce && m_thunk.GetWNDPROC() != subclassedProc)
            return NULL;

        ::SetWindowLongPtr(hwndOld, GWLP_WNDPROC,
                           (LONG_PTR)pThis->m_pfnSuperWindowProc);
        pThis->m_pfnSuperWindowProc = ::DefWindowProc;
        pThis->m_hWnd = NULL;
        return hwndOld;
    }

    static LRESULT CALLBACK StartWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        CContainedWindowT<TBase>* pThis;
        WNDPROC newWindowProc;
        WNDPROC GCCU(pOldProc);

        pThis = reinterpret_cast<CContainedWindowT<TBase>*>(_AtlWinModule.ExtractCreateWndData());
        ATLASSERT(pThis != NULL);
        if (pThis == NULL)
            return 0;
        pThis->m_thunk.Init(WindowProc, pThis);
        newWindowProc = pThis->m_thunk.GetWNDPROC();
        pOldProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(newWindowProc)));
        Unused(pOldProc); // TODO: should generate trace message if overwriting another subclass
        pThis->m_hWnd = hWnd;
        return newWindowProc(hWnd, uMsg, wParam, lParam);
    }

    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        CContainedWindowT<TBase>* pThis = reinterpret_cast<CContainedWindowT<TBase>*>(hWnd);
        _ATL_MSG msg(pThis->m_hWnd, uMsg, wParam, lParam);
        LRESULT lResult;
        const _ATL_MSG *previousMessage;
        BOOL handled;
        LONG_PTR saveWindowProc;

        ATLASSERT(pThis != NULL && pThis->m_hWnd != NULL && pThis->m_pObject != NULL);
        if (pThis == NULL || pThis->m_hWnd == NULL || pThis->m_pObject == NULL)
            return 0;

        hWnd = pThis->m_hWnd;
        previousMessage = pThis->m_pCurrentMsg;
        pThis->m_pCurrentMsg = &msg;

        handled = pThis->m_pObject->ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult, pThis->m_dwMsgMapID);
        ATLASSERT(pThis->m_pCurrentMsg == &msg);

        pThis->m_pCurrentMsg = previousMessage;
        if (handled == FALSE)
        {
            if (uMsg == WM_NCDESTROY)
            {
                saveWindowProc = ::GetWindowLongPtr(hWnd, GWLP_WNDPROC);
                lResult = pThis->DefWindowProc(uMsg, wParam, lParam);
                if (pThis->m_pfnSuperWindowProc != ::DefWindowProc && saveWindowProc == ::GetWindowLongPtr(hWnd, GWLP_WNDPROC))
                    ::SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(pThis->m_pfnSuperWindowProc));
                pThis->m_hWnd = NULL;
            }
            else
                lResult = pThis->DefWindowProc(uMsg, wParam, lParam);
        }
        return lResult;
    }

};
typedef CContainedWindowT<CWindow> CContainedWindow;

#define BEGIN_MSG_MAP(theClass)     \
public:                             \
    BOOL ProcessWindowMessage(HWND GCCU(hWnd), UINT GCCU(uMsg), WPARAM GCCU(wParam), LPARAM GCCU(lParam), LRESULT &GCCU(lResult), DWORD dwMsgMapID = 0) \
    {                               \
        BOOL GCCU(bHandled) = TRUE; \
        Unused(hWnd);               \
        Unused(uMsg);               \
        Unused(wParam);             \
        Unused(lParam);             \
        Unused(lResult);            \
        Unused(bHandled);           \
        switch(dwMsgMapID)          \
        {                           \
        case 0:

#define ALT_MSG_MAP(map) \
            break;       \
        case map:

#define END_MSG_MAP()   \
            break;      \
        default:        \
            ATLASSERT(FALSE); \
            break;      \
        }               \
        return FALSE;   \
    }

#define MESSAGE_HANDLER(msg, func)  \
    if (uMsg == msg)                \
    {                               \
        bHandled = TRUE;            \
        lResult = func(uMsg, wParam, lParam, bHandled); \
        if (bHandled)               \
            return TRUE;            \
    }

#define MESSAGE_RANGE_HANDLER(msgFirst, msgLast, func)  \
    if (uMsg >= msgFirst && uMsg <= msgLast)            \
    {                                                   \
        bHandled = TRUE;                                \
        lResult = func(uMsg, wParam, lParam, bHandled); \
        if (bHandled)                                   \
            return TRUE;                                \
    }

#define COMMAND_HANDLER(id, code, func) \
    if (uMsg == WM_COMMAND && id == LOWORD(wParam) && code == HIWORD(wParam)) \
    {                                   \
        bHandled = TRUE;                \
        lResult = func(HIWORD(wParam), LOWORD(wParam), (HWND)lParam, bHandled); \
        if (bHandled)                   \
            return TRUE;                \
    }

#define COMMAND_ID_HANDLER(id, func)    \
    if (uMsg == WM_COMMAND && id == LOWORD(wParam)) \
    {                                   \
        bHandled = TRUE;                \
        lResult = func(HIWORD(wParam), LOWORD(wParam), (HWND)lParam, bHandled); \
        if (bHandled)                   \
            return TRUE;                \
    }

#define COMMAND_CODE_HANDLER(code, func)    \
    if (uMsg == WM_COMMAND && code == HIWORD(wParam)) \
    {                                       \
        bHandled = TRUE;                    \
        lResult = func(HIWORD(wParam), LOWORD(wParam), (HWND)lParam, bHandled); \
        if (bHandled)                       \
            return TRUE;                    \
    }

#define COMMAND_RANGE_HANDLER(idFirst, idLast, func)    \
    if (uMsg == WM_COMMAND && LOWORD(wParam) >= idFirst && LOWORD(wParam) <= idLast) \
    {                                                   \
        bHandled = TRUE;                                \
        lResult = func(HIWORD(wParam), LOWORD(wParam), (HWND)lParam, bHandled); \
        if (bHandled)                                   \
            return TRUE;                                \
    }

#define NOTIFY_CODE_HANDLER(cd, func)   \
    if (uMsg == WM_NOTIFY && cd == ((LPNMHDR)lParam)->code) \
    {                                   \
        bHandled = TRUE;                \
        lResult = func((int)wParam, (LPNMHDR)lParam, bHandled); \
        if (bHandled)                   \
            return TRUE;                \
    }

#define NOTIFY_HANDLER(id, cd, func)    \
    if (uMsg == WM_NOTIFY && id == ((LPNMHDR)lParam)->idFrom && cd == ((LPNMHDR)lParam)->code) \
    {                                   \
        bHandled = TRUE;                \
        lResult = func((int)wParam, (LPNMHDR)lParam, bHandled); \
        if (bHandled)                   \
            return TRUE;                \
    }

#define CHAIN_MSG_MAP(theChainClass)    \
    {                                   \
        if (theChainClass::ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult)) \
            return TRUE;                \
    }

#define DECLARE_WND_CLASS_EX(WndClassName, style, bkgnd)    \
static ATL::CWndClassInfo& GetWndClassInfo()                \
{                                                           \
    static ATL::CWndClassInfo wc =                          \
    {                                                       \
        { sizeof(WNDCLASSEX), style, StartWindowProc,       \
          0, 0, NULL, NULL, NULL, (HBRUSH)(bkgnd + 1), NULL, WndClassName, NULL }, \
        NULL, NULL, IDC_ARROW, TRUE, 0, _T("")              \
    };                                                      \
    return wc;                                              \
}

struct _ATL_WNDCLASSINFOW
{
    WNDCLASSEX m_wc;
    LPCTSTR m_lpszOrigName;
    WNDPROC pWndProc;
    LPCTSTR m_lpszCursorID;
    BOOL m_bSystemCursor;
    ATOM m_atom;
    TCHAR m_szAutoName[sizeof("ATL:") + sizeof(void *) * 2]; // == 4 characters + NULL + number of hexadecimal digits describing a pointer.

    ATOM Register(WNDPROC *p)
    {
        if (m_wc.hInstance == NULL)
            m_wc.hInstance = _AtlBaseModule.GetModuleInstance();
        if (m_atom == 0)
        {
            if (m_bSystemCursor)
                m_wc.hCursor = ::LoadCursor(NULL, m_lpszCursorID);
            else
                m_wc.hCursor = ::LoadCursor(_AtlBaseModule.GetResourceInstance(), m_lpszCursorID);

            m_atom = RegisterClassEx(&m_wc);
        }

        return m_atom;
    }
};

}; // namespace ATL

#pragma pop_macro("SubclassWindow")

