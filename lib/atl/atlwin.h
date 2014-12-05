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

#ifdef __GNUC__
#define GCCU(x)	x __attribute__((unused))
#define Unused(x)
#else
#define GCCU(x) (x) 
#define Unused(x)	(x);
#endif // __GNUC__

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

namespace ATL
{

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
	HMENU									m_hMenu;
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
	LPRECT									m_lpRect;
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
	BOOL									bHandled;
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
	DWORD									m_mov;
	DWORD									m_this;
	BYTE									m_jmp;
	DWORD									m_relproc;
};
#pragma pack(pop)

class CWndProcThunk
{
public:
	thunkCode								*m_pthunk;
	_AtlCreateWndData						cd;
public:

    CWndProcThunk()
    {
        m_pthunk = (thunkCode*)VirtualAlloc(NULL, sizeof(thunkCode), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    }

    ~CWndProcThunk()
    {
        VirtualFree(m_pthunk, 0, MEM_RELEASE);
    }

	BOOL Init(WNDPROC proc, void *pThis)
	{
		m_pthunk->m_mov = 0x042444C7;
		m_pthunk->m_this = PtrToUlong(pThis);
		m_pthunk->m_jmp = 0xe9;
		m_pthunk->m_relproc = DWORD(reinterpret_cast<char *>(proc) - (reinterpret_cast<char *>(m_pthunk) + sizeof(thunkCode)));
		return TRUE;
	}

	WNDPROC GetWNDPROC()
	{
		return reinterpret_cast<WNDPROC>(m_pthunk);
	}
};

#elif _AMD64_ //WARNING: NOT VERIFIED
#pragma pack(push,1)
struct thunkCode
{
	DWORD_PTR								m_mov;
	DWORD_PTR								m_this;
	BYTE									m_jmp;
	DWORD_PTR								m_relproc;
};
#pragma pack(pop)

class CWndProcThunk
{
public:
	thunkCode								m_thunk;
	_AtlCreateWndData						cd;
public:
	BOOL Init(WNDPROC proc, void *pThis)
	{
		m_thunk.m_mov = 0xffff8000042444C7LL;
		m_thunk.m_this = (DWORD_PTR)pThis;
		m_thunk.m_jmp = 0xe9;
		m_thunk.m_relproc = DWORD_PTR(reinterpret_cast<char *>(proc) - (reinterpret_cast<char *>(this) + sizeof(thunkCode)));
		return TRUE;
	}

	WNDPROC GetWNDPROC()
	{
		return reinterpret_cast<WNDPROC>(&m_thunk);
	}
};
#else
#error ARCH not supported
#endif

class CMessageMap
{
public:
	virtual BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &lResult, DWORD dwMsgMapID) = 0;
};

class CWindow
{
public:
	HWND									m_hWnd;
	static RECT								rcDefault;
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

	HDC BeginPaint(LPPAINTSTRUCT lpPaint)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::BeginPaint(m_hWnd, lpPaint);
	}

	BOOL DestroyWindow()
	{
		ATLASSERT(::IsWindow(m_hWnd));

		if (!::DestroyWindow(m_hWnd))
			return FALSE;

		m_hWnd = NULL;
		return TRUE;
	}

	void EndPaint(LPPAINTSTRUCT lpPaint)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		::EndPaint(m_hWnd, lpPaint);
	}

	BOOL GetClientRect(LPRECT lpRect) const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::GetClientRect(m_hWnd, lpRect);
	}

	CWindow GetParent() const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return CWindow(::GetParent(m_hWnd));
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

	BOOL IsWindow() const
	{
		return ::IsWindow(m_hWnd);
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

	BOOL ScreenToClient(LPPOINT lpPoint) const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::ScreenToClient(m_hWnd, lpPoint);
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

	HWND SetCapture()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::SetCapture(m_hWnd);
	}

	HWND SetFocus()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::SetFocus(m_hWnd);
	}

	UINT_PTR SetTimer(UINT_PTR nIDEvent, UINT nElapse, void (CALLBACK *lpfnTimer)(HWND, UINT, UINT_PTR, DWORD) = NULL)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::SetTimer(m_hWnd, nIDEvent, nElapse, reinterpret_cast<TIMERPROC>(lpfnTimer));
	}

	BOOL SetWindowPos(HWND hWndInsertAfter, int x, int y, int cx, int cy, UINT nFlags)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::SetWindowPos(m_hWnd, hWndInsertAfter, x, y, cx, cy, nFlags);
	}

	BOOL SetWindowText(LPCTSTR lpszString)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::SetWindowText(m_hWnd, lpszString);
	}

	BOOL ShowWindow(int nCmdShow)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::ShowWindow(m_hWnd, nCmdShow);
	}

	BOOL UpdateWindow()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::UpdateWindow(m_hWnd);
	}
};

__declspec(selectany) RECT CWindow::rcDefault = { CW_USEDEFAULT, CW_USEDEFAULT, 0, 0 };

template <class TBase = CWindow, class TWinTraits = CControlWinTraits>
class CWindowImplBaseT : public TBase, public CMessageMap
{
public:
	enum { WINSTATE_DESTROYED = 0x00000001 };
	DWORD									m_dwState;
	const _ATL_MSG							*m_pCurrentMsg;
	CWndProcThunk							m_thunk;
	WNDPROC									m_pfnSuperWindowProc;
public:
	CWindowImplBaseT()
	{
		m_dwState = 0;
		m_pCurrentMsg = NULL;
		m_pfnSuperWindowProc = ::DefWindowProc;
	}

	virtual void OnFinalMessage(HWND /* hWnd */)
	{
	}

	BOOL SubclassWindow(HWND hWnd)
	{
		CWindowImplBaseT<TBase, TWinTraits>	*pThis;
		WNDPROC								newWindowProc;
		WNDPROC								oldWindowProc;
		BOOL								result;

		ATLASSERT(m_hWnd == NULL);
		ATLASSERT(::IsWindow(hWnd));

		pThis = reinterpret_cast<CWindowImplBaseT<TBase, TWinTraits>*>(this);

		result = m_thunk.Init(GetWindowProc(), this);
		if (result == FALSE)
			return FALSE;
		newWindowProc = m_thunk.GetWNDPROC();
		oldWindowProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(newWindowProc)));
		if (oldWindowProc == NULL)
			return FALSE;
		m_pfnSuperWindowProc = oldWindowProc;
		pThis->m_hWnd = hWnd;
		return TRUE;
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
		CWindowImplBaseT<TBase, TWinTraits>	*pThis;

		pThis = reinterpret_cast<CWindowImplBaseT<TBase, TWinTraits> *>(this);
		return ::CallWindowProc(m_pfnSuperWindowProc, pThis->m_hWnd, uMsg, wParam, lParam);
	}

	static LRESULT CALLBACK StartWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		CWindowImplBaseT<TBase, TWinTraits>	*pThis;
		WNDPROC								newWindowProc;
		WNDPROC								GCCU(pOldProc);

		pThis = reinterpret_cast<CWindowImplBaseT<TBase, TWinTraits> *>(_AtlWinModule.ExtractCreateWndData());
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
		CWindowImplBaseT<TBase, TWinTraits>	*pThis = reinterpret_cast<CWindowImplBaseT< TBase, TWinTraits> *>(hWnd);
		_ATL_MSG							msg(pThis->m_hWnd, uMsg, wParam, lParam);
		LRESULT								lResult;
		const _ATL_MSG						*previousMessage;
		BOOL								handled;
		LONG_PTR							saveWindowProc;

		ATLASSERT(pThis != NULL && (pThis->m_dwState & WINSTATE_DESTROYED) == 0 && pThis->m_hWnd != NULL);
		if (pThis == NULL || (pThis->m_dwState & WINSTATE_DESTROYED) != 0 || pThis->m_hWnd == NULL)
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
				lResult = pThis->DefWindowProc(uMsg, wParam, lParam);
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
		HWND								hWnd;

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
		hWnd = ::CreateWindowEx(dwExStyle, reinterpret_cast<LPCWSTR>(MAKEINTATOM(atom)), szWindowName, dwStyle, rect.m_lpRect->left,
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
	static LPCTSTR GetWndCaption()
	{
		return NULL;
	}

	HWND Create(HWND hWndParent, _U_RECT rect = NULL, LPCTSTR szWindowName = NULL, DWORD dwStyle = 0,
					DWORD dwExStyle = 0, _U_MENUorID MenuOrID = 0U, LPVOID lpCreateParam = NULL)
	{
		CWindowImplBaseT<TBase, TWinTraits>	*pThis;
		ATOM								atom;

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
	CWndProcThunk							m_thunk;
	LPCTSTR									m_lpszClassName;
	WNDPROC									m_pfnSuperWindowProc;
	CMessageMap								*m_pObject;
	DWORD									m_dwMsgMapID;
	const _ATL_MSG							*m_pCurrentMsg;
public:
	CContainedWindowT(CMessageMap *pObject, DWORD dwMsgMapID = 0)
	{
		m_lpszClassName = TBase::GetWndClassName();
		m_pfnSuperWindowProc = ::DefWindowProc;
		m_pObject = pObject;
		m_dwMsgMapID = dwMsgMapID;
		m_pCurrentMsg = NULL;
	}

	CContainedWindowT(LPTSTR lpszClassName, CMessageMap *pObject, DWORD dwMsgMapID = 0)
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
		CContainedWindowT<TBase>			*pThis;
		WNDPROC								newWindowProc;
		WNDPROC								oldWindowProc;
		BOOL								result;

		ATLASSERT(m_hWnd == NULL);
		ATLASSERT(::IsWindow(hWnd));

		pThis = reinterpret_cast<CContainedWindowT<TBase> *>(this);

		result = m_thunk.Init(WindowProc, pThis);
		if (result == FALSE)
			return FALSE;
		newWindowProc = m_thunk.GetWNDPROC();
		oldWindowProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(newWindowProc)));
		if (oldWindowProc == NULL)
			return FALSE;
		m_pfnSuperWindowProc = oldWindowProc;
		pThis->m_hWnd = hWnd;
		return TRUE;
	}

	static LRESULT CALLBACK StartWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		CContainedWindowT<TBase>			*pThis;
		WNDPROC								newWindowProc;
		WNDPROC								GCCU(pOldProc);

		pThis = reinterpret_cast<CContainedWindowT<TBase> *>(_AtlWinModule.ExtractCreateWndData());
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
		CContainedWindowT<TBase>			*pThis = reinterpret_cast<CContainedWindowT<TBase> *>(hWnd);
		_ATL_MSG							msg(pThis->m_hWnd, uMsg, wParam, lParam);
		LRESULT								lResult;
		const _ATL_MSG						*previousMessage;
		BOOL								handled;
		LONG_PTR							saveWindowProc;

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
typedef CContainedWindowT<CWindow>	CContainedWindow;

#define BEGIN_MSG_MAP(theClass)																								\
public:																														\
	BOOL ProcessWindowMessage(HWND GCCU(hWnd), UINT GCCU(uMsg), WPARAM GCCU(wParam), LPARAM GCCU(lParam), LRESULT &GCCU(lResult), DWORD dwMsgMapID = 0)	\
	{																														\
		BOOL GCCU(bHandled) = TRUE;																							\
		Unused(hWnd);																										\
		Unused(uMsg);																										\
		Unused(wParam);																										\
		Unused(lParam);																										\
		Unused(lResult);																									\
		Unused(bHandled);																									\
		switch(dwMsgMapID)																									\
		{																													\
		case 0:

#define ALT_MSG_MAP(map)																		\
            break;																				\
        case map:

#define END_MSG_MAP()																			\
			break;																				\
		default:																				\
			ATLASSERT(FALSE);																	\
			break;																				\
		}																						\
		return FALSE;																			\
	}

#define MESSAGE_HANDLER(msg, func)																\
	if (uMsg == msg)																			\
	{																							\
		bHandled = TRUE;																		\
		lResult = func(uMsg, wParam, lParam, bHandled);											\
		if (bHandled)																			\
			return TRUE;																		\
	}

#define MESSAGE_RANGE_HANDLER(msgFirst, msgLast, func)											\
	if (uMsg >= msgFirst && uMsg <= msgLast)													\
	{																							\
		bHandled = TRUE;																		\
		lResult = func(uMsg, wParam, lParam, bHandled);											\
		if (bHandled)																			\
			return TRUE;																		\
	}

#define COMMAND_ID_HANDLER(id, func)															\
	if (uMsg == WM_COMMAND && id == LOWORD(wParam))												\
	{																							\
		bHandled = TRUE;																		\
		lResult = func(HIWORD(wParam), LOWORD(wParam), (HWND)lParam, bHandled);					\
		if (bHandled)																			\
			return TRUE;																		\
	}

#define COMMAND_RANGE_HANDLER(idFirst, idLast, func)											\
	if (uMsg == WM_COMMAND && LOWORD(wParam) >= idFirst  && LOWORD(wParam) <= idLast)			\
	{																							\
		bHandled = TRUE;																		\
		lResult = func(HIWORD(wParam), LOWORD(wParam), (HWND)lParam, bHandled);					\
		if (bHandled)																			\
			return TRUE;																		\
	}

#define NOTIFY_CODE_HANDLER(cd, func)															\
	if(uMsg == WM_NOTIFY && cd == ((LPNMHDR)lParam)->code)										\
	{																							\
		bHandled = TRUE;																		\
		lResult = func((int)wParam, (LPNMHDR)lParam, bHandled);									\
		if (bHandled)																			\
			return TRUE;																		\
	}

#define NOTIFY_HANDLER(id, cd, func)															\
	if(uMsg == WM_NOTIFY && id == ((LPNMHDR)lParam)->idFrom && cd == ((LPNMHDR)lParam)->code)	\
	{																							\
		bHandled = TRUE;																		\
		lResult = func((int)wParam, (LPNMHDR)lParam, bHandled);									\
		if (bHandled)																			\
			return TRUE;																		\
	}

#define DECLARE_WND_CLASS_EX(WndClassName, style, bkgnd)										\
static ATL::CWndClassInfo& GetWndClassInfo()													\
{																								\
	static ATL::CWndClassInfo wc =																\
	{																							\
		{ sizeof(WNDCLASSEX), style, StartWindowProc,											\
		  0, 0, NULL, NULL, NULL, (HBRUSH)(bkgnd + 1), NULL, WndClassName, NULL },				\
		NULL, NULL, IDC_ARROW, TRUE, 0, _T("")													\
	};																							\
	return wc;																					\
}

struct _ATL_WNDCLASSINFOW
{
	WNDCLASSEXW								m_wc;
	LPCWSTR									m_lpszOrigName;
	WNDPROC									pWndProc;
	LPCWSTR									m_lpszCursorID;
	BOOL									m_bSystemCursor;
	ATOM									m_atom;
	WCHAR									m_szAutoName[5 + sizeof(void *)];

	ATOM Register(WNDPROC *p)
	{
		if (m_wc.hInstance == NULL)
			m_wc.hInstance = _AtlBaseModule.GetModuleInstance();
		if (m_atom == 0)
			m_atom = RegisterClassEx(&m_wc);
		return m_atom;
	}
};

}; // namespace ATL
