/*
 * PROJECT:     ReactOS Certificate Manager
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     CSortableHeader definition
 * COPYRIGHT:   Copyright 2025 Mark Jansen <mark.jansen@reactos.org>
 */

struct CSplitter
{
  private:
    CWindow m_Parent;
    CWindow m_Left;
    CWindow m_Right;

    int m_splitterPos = 280;
    const int m_splitterSize = 4;

  public:
    void
    Init(CWindow parent, CWindow left, CWindow right)
    {
        m_Parent = parent;
        m_Left = left;
        m_Right = right;
    }

    void
    ResizeWnd(int width, int height)
    {
        RECT rc;
        m_Parent.GetClientRect(&rc);

        HDWP hdwp = BeginDeferWindowPos(2);

        if (hdwp)
            hdwp = m_Left.DeferWindowPos(
                hdwp, NULL, 0, 0, m_splitterPos - m_splitterSize / 2, height, SWP_NOZORDER | SWP_NOACTIVATE);
        if (hdwp)
            hdwp = m_Right.DeferWindowPos(
                hdwp, NULL, m_splitterPos + m_splitterSize / 2, 0, width - (m_splitterPos + m_splitterSize / 2), height,
                SWP_NOZORDER | SWP_NOACTIVATE);
        if (hdwp)
            EndDeferWindowPos(hdwp);
    }

    void
    DragSplitter(INT x)
    {
        m_splitterPos = max(x, 120);
        RECT rc;
        m_Parent.GetClientRect(&rc);
        ResizeWnd(rc.right - rc.left, rc.bottom - rc.top);
    }

    LRESULT
    OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
    {
        ResizeWnd(LOWORD(lParam), HIWORD(lParam));
        return TRUE;
    }

    LRESULT
    OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
    {
        if (LOWORD(lParam) == HTCLIENT)
        {
            POINT pt;
            GetCursorPos(&pt);
            m_Parent.ScreenToClient(&pt);
            if (pt.x >= m_splitterPos - m_splitterSize / 2 && pt.x <= m_splitterPos + m_splitterSize / 2)
            {
                SetCursor(LoadCursor(NULL, IDC_SIZEWE));
                bHandled = TRUE;
                return TRUE;
            }
        }
        bHandled = FALSE;
        return FALSE;
    }

    LRESULT
    OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
    {
        INT x = GET_X_LPARAM(lParam);

        if (x >= m_splitterPos - m_splitterSize / 2 && x <= m_splitterPos + m_splitterSize / 2)
        {
            m_Parent.SetCapture();

            bHandled = TRUE;
            return TRUE;
        }

        return TRUE;
    }

    LRESULT
    OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
    {
        if (GetCapture() == m_Parent.m_hWnd)
        {
            DragSplitter(GET_X_LPARAM(lParam));
        }
        return TRUE;
    }

    LRESULT
    OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
    {
        if (GetCapture() == m_Parent.m_hWnd)
        {
            DragSplitter(GET_X_LPARAM(lParam));
            ReleaseCapture();
        }
        return TRUE;
    }
};
