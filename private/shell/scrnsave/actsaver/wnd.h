/////////////////////////////////////////////////////////////////////////////
// WND.H
//
// Declaration of CWindow
//
// History:
//
// Author   Date        Description
// ------   ----        -----------
// jaym     08/26/96    Created
/////////////////////////////////////////////////////////////////////////////
#ifndef __WND_H__
#define __WND_H__

/////////////////////////////////////////////////////////////////////////////
// CWindow
/////////////////////////////////////////////////////////////////////////////
class CWindow
{
// Construction/destruction
public:
    CWindow();
    virtual ~CWindow();

public:
// Overridable functions
    virtual BOOL Create(LPCSTR lpszWindowName, DWORD dwStyle, const RECT & rect, HWND hwndParent, UINT nID);
    virtual BOOL CreateEx(LPCSTR lpszWindowName, DWORD dwExStyle, DWORD dwStyle, const RECT & rect, HWND hwndParent, UINT nID);

    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam)
    { return FALSE; }

    virtual BOOL OnCreate(CREATESTRUCT * pcs);
    virtual void OnDestroy();

    virtual BOOL OnEraseBkgnd(HDC hDC)
    { return FALSE; }

    virtual void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
    { }

    virtual void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
    { }

    virtual void OnMouseButtonDown(UINT nFlags, int x, int y)
    { }

    virtual void OnMouseMove(UINT nFlags, int x, int y)
    { }

    virtual void OnNCDestroy()
    { }

    virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT * plResult)
    { return FALSE; }

    virtual void OnPaint(HDC hDC, PAINTSTRUCT * pps)
    { }
    
    virtual void OnPaletteChanged(HWND hwndPalChng)
    { }

    virtual void OnParentNotify(UINT message, LPARAM lParam)
    { }

    virtual BOOL OnQueryNewPalette()
    { return FALSE; }

    virtual void OnShowWindow(BOOL bShow, int nStatus)
    { }

    virtual void OnTimer(UINT nIDEvent)
    { }

    virtual LRESULT OnUserMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    { return -1; }

    virtual LRESULT OnClose()
    {
        return DefWindowProc(m_hWnd, WM_CLOSE, 0, 0);
    }

// Message Functions
    LRESULT SendMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0)
    {
        ASSERT(::IsWindow(m_hWnd));
        return ::SendMessage(m_hWnd,message,wParam,lParam);
    }

    BOOL PostMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0)
    {
        ASSERT(::IsWindow(m_hWnd));
        return ::PostMessage(m_hWnd,message,wParam,lParam);
    }

    BOOL SendNotifyMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0)
    {
        ASSERT(::IsWindow(m_hWnd));
        return ::SendNotifyMessage(m_hWnd, message, wParam, lParam);
    }

// Window Text Functions
    BOOL SetWindowText(LPCTSTR lpszString)
    {
        ASSERT(::IsWindow(m_hWnd));
        return ::SetWindowText(m_hWnd, lpszString);
    }

    int GetWindowText(LPTSTR lpszStringBuf, int nMaxCount) const
    {
        ASSERT(::IsWindow(m_hWnd));
        return ::GetWindowText(m_hWnd, lpszStringBuf, nMaxCount);
    }

    int GetWindowTextLength() const
    {
        ASSERT(::IsWindow(m_hWnd));
        return ::GetWindowTextLength(m_hWnd);
    }

    BOOL GetWindowText(BSTR& bstrText);

// Window Size and Position Functions
    BOOL SetWindowPos(HWND hWndInsertAfter, int x, int y, int cx, int cy, UINT nFlags)
    {
        ASSERT(::IsWindow(m_hWnd));
        return ::SetWindowPos(m_hWnd, hWndInsertAfter, x, y, cx, cy, nFlags);
    }

    BOOL SetWindowPos(HWND hWndInsertAfter, LPCRECT lpRect, UINT nFlags)
    {
        ASSERT(::IsWindow(m_hWnd));
        return ::SetWindowPos(m_hWnd, hWndInsertAfter, lpRect->left, lpRect->top, lpRect->right - lpRect->left, lpRect->bottom - lpRect->top, nFlags);
    }

    BOOL GetClientRect(LPRECT lpRect) const
    {
        ASSERT(::IsWindow(m_hWnd));
        return ::GetClientRect(m_hWnd, lpRect);
    }

    BOOL GetWindowRect(LPRECT lpRect) const
    {
        ASSERT(::IsWindow(m_hWnd));
        return ::GetWindowRect(m_hWnd, lpRect);
    }

    BOOL IsWindowVisible()
    {
        ASSERT(::IsWindow(m_hWnd));
        return ::IsWindowVisible(m_hWnd);
    }

    BOOL EnableWindow(BOOL bEnable)
    {
        ASSERT(::IsWindow(m_hWnd));
        return ::EnableWindow(m_hWnd, bEnable);
    }

// Update and Painting Functions
	HDC GetDC()
	{
		ASSERT(::IsWindow(m_hWnd));
		return ::GetDC(m_hWnd);
	}

	HDC GetWindowDC()
	{
		ASSERT(::IsWindow(m_hWnd));
		return ::GetWindowDC(m_hWnd);
	}

	int ReleaseDC(HDC hDC)
	{
		ASSERT(::IsWindow(m_hWnd));
		return ::ReleaseDC(m_hWnd, hDC);
	}

    BOOL RedrawWindow(LPCRECT lpRectUpdate = NULL, HRGN hRgnUpdate = NULL, UINT flags = RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE)
    {
        ASSERT(::IsWindow(m_hWnd));
        return ::RedrawWindow(m_hWnd, lpRectUpdate, hRgnUpdate, flags);
    }

    BOOL ShowWindow(int nCmdShow)
    {
        ASSERT(::IsWindow(m_hWnd));
        return ::ShowWindow(m_hWnd, nCmdShow);
    }

// Timer Functions
    UINT_PTR SetTimer(UINT nIDEvent, UINT nElapse, void (CALLBACK* lpfnTimer)(HWND, UINT, UINT, DWORD))
    {
        ASSERT(::IsWindow(m_hWnd));
        return ::SetTimer(m_hWnd, nIDEvent, nElapse, (TIMERPROC)lpfnTimer);
    }

    BOOL KillTimer(UINT_PTR nIDEvent)
    {
        ASSERT(::IsWindow(m_hWnd));
        return ::KillTimer(m_hWnd, nIDEvent);
    }

// Class data
protected:
    HIMC    m_hPrevIMC;

public:
    static  ATOM    m_atomWndClass;

    HWND    m_hWnd;

// Class methods
protected:
    void InitWndClass();
    void SysPalChanged();
};

/////////////////////////////////////////////////////////////////////////////
// Helper functions
/////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK SysPalChangedCallback(HWND hWnd, LPARAM lParam);
LRESULT CALLBACK GenericWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif  // __WND_H__
