/*
 * PROJECT:     ReactOS UI Layout Engine
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * FILE:        base/applications/rapps/include/rosui.h
 * PURPOSE:     ATL Layout engine for RAPPS
 * COPYRIGHT:   Copyright 2015 David Quintana           (gigaherz@gmail.com)
 */
#pragma once

#include <atlwin.h>

template<class T, INT GrowthRate = 10>
class CPointerArray
{
protected:
    HDPA m_hDpa;

public:
    CPointerArray()
    {
        m_hDpa = DPA_Create(GrowthRate);
    }

    ~CPointerArray()
    {
        DPA_DestroyCallback(m_hDpa, s_OnRemoveItem, this);
    }

private:
    static INT CALLBACK s_OnRemoveItem(PVOID ptr, PVOID context)
    {
        CPointerArray * self = (CPointerArray*) context;
        return (INT) self->OnRemoveItem(reinterpret_cast<T*>(ptr));
    }

    static INT CALLBACK s_OnCompareItems(PVOID p1, PVOID p2, LPARAM lParam)
    {
        CPointerArray * self = (CPointerArray*) lParam;
        return self->OnCompareItems(reinterpret_cast<T*>(p1), reinterpret_cast<T*>(p2));
    }

public:
    virtual BOOL OnRemoveItem(T * ptr)
    {
        return TRUE;
    }

    virtual INT OnCompareItems(T * p1, T * p2)
    {
        INT_PTR t = (reinterpret_cast<INT_PTR>(p2) - reinterpret_cast<INT_PTR>(p1));
        if (t > 0)
            return 1;
        if (t < 0)
            return -1;
        return 0;
    }

public:
    INT GetCount() const
    {
        return DPA_GetPtrCount(m_hDpa);
    }

    T* Get(INT i) const
    {
        return (T*) DPA_GetPtr(m_hDpa, i);
    }

    BOOL Set(INT i, T* ptr)
    {
        return DPA_SetPtr(m_hDpa, i, ptr);
    }

    INT Insert(INT at, T* ptr)
    {
        return DPA_InsertPtr(m_hDpa, at, ptr);
    }

    INT Append(T* ptr)
    {
        return DPA_InsertPtr(m_hDpa, DA_LAST, ptr);
    }

    INT IndexOf(T* ptr) const
    {
        return DPA_GetPtrIndex(m_hDpa, ptr);
    }

    BOOL Remove(T* ptr)
    {
        INT i = IndexOf(ptr);
        if (i < 0)
            return FALSE;
        return RemoveAt(i);
    }

    BOOL RemoveAt(INT i)
    {
        T* ptr = (T*) DPA_GetPtr(m_hDpa, i);
        OnRemoveItem(ptr);
        return DPA_DeletePtr(m_hDpa, i);
    }

    BOOL Clear()
    {
        DPA_EnumCallback(s_OnRemoveItem, this);
        return DPA_DeleteAllPtrs(m_hDpa);
    }

    BOOL Sort()
    {
        return DPA_Sort(m_hDpa, s_OnCompareItems, (LPARAM)this);
    }

    INT Search(T* item, INT iStart, UINT uFlags)
    {
        return DPA_Search(m_hDpa, item, 0, s_OnCompareItems, (LPARAM)this, 0);
    }
};

class CUiRect
    : public RECT
{
public:
    CUiRect()
    {
        left = right = top = bottom = 0;
    }

    CUiRect(INT l, INT t, INT r, INT b)
    {
        left = l;
        right = r;
        top = t;
        bottom = b;
    }
};

class CUiMargin
    : public CUiRect
{
public:
    CUiMargin()
    {
    }

    CUiMargin(INT all)
        : CUiRect(all, all, all, all)
    {
    }

    CUiMargin(INT horz, INT vert)
        : CUiRect(horz, vert, horz, vert)
    {
    }
};

class CUiMeasure
{
public:
    enum MeasureType
    {
        Type_FitContent = 0,
        Type_Fixed = 1,
        Type_Percent = 2,
        Type_FitParent = 3
    };

private:
    MeasureType m_Type;
    INT m_Value;

public:
    CUiMeasure()
    {
        m_Type = Type_FitContent;
        m_Value = 0;
    }

    CUiMeasure(MeasureType type, INT value)
    {
        m_Type = type;
        m_Value = value;
    }

    INT ComputeMeasure(INT parent, INT content)
    {
        switch (m_Type)
        {
        case Type_FitContent:
            return content;
        case Type_Fixed:
            return m_Value;
        case Type_Percent:
            return max(content, parent * m_Value / 100);
        case Type_FitParent:
            return parent;
        }

        return 0;
    }

public:
    static CUiMeasure FitContent()
    {
        return CUiMeasure(Type_FitContent, 0);
    }

    static CUiMeasure FitParent()
    {
        return CUiMeasure(Type_FitParent, 0);
    }

    static CUiMeasure Fixed(INT pixels)
    {
        return CUiMeasure(Type_Fixed, pixels);
    }

    static CUiMeasure Percent(INT percent)
    {
        return CUiMeasure(Type_Percent, percent);
    }
};

enum CUiAlignment
{
    UiAlign_LeftTop,
    UiAlign_Middle,
    UiAlign_RightBtm,
    UiAlign_Stretch
};

class CUiBox
{
public:
    CUiMargin m_Margin;

    CUiAlignment m_HorizontalAlignment;
    CUiAlignment m_VerticalAlignment;

protected:
    CUiBox()
    {
        m_HorizontalAlignment = UiAlign_LeftTop;
        m_VerticalAlignment = UiAlign_LeftTop;
    }

    virtual VOID ComputeRect(RECT parentRect, RECT currentRect, RECT* newRect)
    {
        parentRect.left += m_Margin.left;
        parentRect.right -= m_Margin.right;
        parentRect.top += m_Margin.top;
        parentRect.bottom -= m_Margin.bottom;

        if (parentRect.right < parentRect.left)
            parentRect.right = parentRect.left;

        if (parentRect.bottom < parentRect.top)
            parentRect.bottom = parentRect.top;

        SIZE szParent = {parentRect.right - parentRect.left, parentRect.bottom - parentRect.top};
        SIZE szCurrent = {currentRect.right - currentRect.left, currentRect.bottom - currentRect.top};

        currentRect = parentRect;

        switch (m_HorizontalAlignment)
        {
        case UiAlign_LeftTop:
            currentRect.right = currentRect.left + szCurrent.cx;
            break;
        case UiAlign_Middle:
            currentRect.left = parentRect.left + (szParent.cx - szCurrent.cx) / 2;
            currentRect.right = currentRect.left + szCurrent.cx;
            break;
        case UiAlign_RightBtm:
            currentRect.left = currentRect.right - szCurrent.cx;
            break;
        default:
            break;
        }

        switch (m_VerticalAlignment)
        {
        case UiAlign_LeftTop:
            currentRect.bottom = currentRect.top + szCurrent.cy;
            break;
        case UiAlign_Middle:
            currentRect.top = parentRect.top + (szParent.cy - szCurrent.cy) / 2;
            currentRect.bottom = currentRect.top + szCurrent.cy;
            break;
        case UiAlign_RightBtm:
            currentRect.top = currentRect.bottom - szCurrent.cy;
            break;
        default:
            break;
        }

        *newRect = currentRect;
    }


public:
    virtual VOID ComputeMinimalSize(SIZE* size)
    {
        // Override in subclass
        size->cx = max(size->cx, 0);
        size->cy = min(size->cy, 0);
    };

    virtual VOID ComputeContentBounds(RECT* rect)
    {
        // Override in subclass
    };

    virtual DWORD_PTR CountSizableChildren()
    {
        // Override in subclass
        return 0;
    };

    virtual HDWP OnParentSize(RECT parentRect, HDWP hDwp)
    {
        // Override in subclass
        return NULL;
    };
};

class CUiPrimitive
{
protected:
    CUiPrimitive * m_Parent;

public:
    virtual ~CUiPrimitive() {}

    virtual CUiBox * AsBox() { return NULL; }
};

class CUiCollection :
    public CPointerArray < CUiPrimitive >
{
    virtual BOOL OnRemoveItem(CUiPrimitive * ptr)
    {
        delete ptr;
        return TRUE;
    }
};

class CUiContainer
{
protected:
    CUiCollection m_Children;

public:
    CUiCollection& Children() { return m_Children; }
};

class CUiPanel :
    public CUiPrimitive,
    public CUiBox,
    public CUiContainer
{
public:
    CUiMeasure m_Width;
    CUiMeasure m_Height;

    CUiPanel()
    {
        m_Width = CUiMeasure::FitParent();
        m_Height = CUiMeasure::FitParent();
    }

    virtual ~CUiPanel()
    {
    }

    virtual CUiBox * AsBox() { return this; }

    virtual VOID ComputeMinimalSize(SIZE* size)
    {
        for (INT i = 0; i < m_Children.GetCount(); i++)
        {
            CUiBox * box = m_Children.Get(i)->AsBox();
            if (box)
            {
                box->ComputeMinimalSize(size);
            }
        }
    };

    virtual VOID ComputeContentBounds(RECT* rect)
    {
        for (INT i = 0; i < m_Children.GetCount(); i++)
        {
            CUiBox * box = m_Children.Get(i)->AsBox();
            if (box)
            {
                box->ComputeContentBounds(rect);
            }
        }
    };

    virtual DWORD_PTR CountSizableChildren()
    {
        INT count = 0;
        for (INT i = 0; i < m_Children.GetCount(); i++)
        {
            CUiBox * box = m_Children.Get(i)->AsBox();
            if (box)
            {
                count += box->CountSizableChildren();
            }
        }
        return count;
    }

    virtual HDWP OnParentSize(RECT parentRect, HDWP hDwp)
    {
        RECT rect = {0};

        SIZE content = {0};
        ComputeMinimalSize(&content);

        INT preferredWidth = m_Width.ComputeMeasure(parentRect.right - parentRect.left, content.cx);
        INT preferredHeight = m_Height.ComputeMeasure(parentRect.bottom - parentRect.top, content.cy);

        rect.right = preferredWidth;
        rect.bottom = preferredHeight;

        ComputeRect(parentRect, rect, &rect);

        for (INT i = 0; i < m_Children.GetCount(); i++)
        {
            CUiBox * box = m_Children.Get(i)->AsBox();
            if (box)
            {
                hDwp = box->OnParentSize(rect, hDwp);
            }
        }

        return hDwp;
    }
};

template<class T = CWindow>
class CUiWindow :
    public CUiPrimitive,
    public CUiBox,
    public T
{
public:
    virtual CUiBox * AsBox() { return this; }

    HWND GetWindow() { return T::m_hWnd; }

    virtual VOID ComputeMinimalSize(SIZE* size)
    {
        // TODO: Maybe use WM_GETMINMAXINFO?
        return CUiBox::ComputeMinimalSize(size);
    };

    virtual VOID ComputeContentBounds(RECT* rect)
    {
        RECT r;
        ::GetWindowRect(T::m_hWnd, &r);
        rect->left = min(rect->left, r.left);
        rect->top = min(rect->top, r.top);
        rect->right = max(rect->right, r.right);
        rect->bottom = max(rect->bottom, r.bottom);
    };

    virtual DWORD_PTR CountSizableChildren()
    {
        return 1;
    };

    virtual HDWP OnParentSize(RECT parentRect, HDWP hDwp)
    {
        RECT rect;

        ::GetWindowRect(T::m_hWnd, &rect);

        ComputeRect(parentRect, rect, &rect);

        if (hDwp)
        {
            return ::DeferWindowPos(hDwp, T::m_hWnd, NULL, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOACTIVATE | SWP_NOZORDER);
        }
        else
        {
            T::SetWindowPos(NULL, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOACTIVATE | SWP_NOZORDER | SWP_DEFERERASE);
            return NULL;
        }
    };

    virtual ~CUiWindow()
    {
        T::DestroyWindow();
    }

    VOID GetWindowTextW(ATL::CStringW& szText)
    {
        INT length = CWindow::GetWindowTextLengthW() + 1;
        CWindow::GetWindowTextW(szText.GetBuffer(length), length);
        szText.ReleaseBuffer();
    }
};

class CUiSplitPanel :
    public CUiPrimitive,
    public CUiBox,
    public CWindowImpl<CUiSplitPanel>
{
    static const INT THICKNESS = 4;

protected:

    HCURSOR m_hCursor;

    CUiPanel m_First;
    CUiPanel m_Second;

    RECT m_LastRect;

    BOOL m_HasOldRect;

public:
    INT m_Pos;
    BOOL m_Horizontal;
    BOOL m_DynamicFirst;
    INT m_MinFirst;
    INT m_MinSecond;

    CUiMeasure m_Width;
    CUiMeasure m_Height;

    CUiSplitPanel()
    {
        m_Width = CUiMeasure::FitParent();
        m_Height = CUiMeasure::FitParent();
        m_Pos = 100;
        m_MinFirst = 100;
        m_MinSecond = 100;
        m_DynamicFirst = FALSE;
        m_HasOldRect = FALSE;
    }

    virtual ~CUiSplitPanel()
    {
    }

    virtual CUiBox * AsBox() { return this; }

    CUiCollection& First() { return m_First.Children(); }
    CUiCollection& Second() { return m_Second.Children(); }

    virtual VOID ComputeMinimalSize(SIZE* size)
    {
        if (m_Horizontal)
            size->cx = max(size->cx, THICKNESS);
        else
            size->cy = max(size->cy, THICKNESS);
        m_First.ComputeMinimalSize(size);
        m_Second.ComputeMinimalSize(size);
    };

    virtual VOID ComputeContentBounds(RECT* rect)
    {
        RECT r;

        m_First.ComputeContentBounds(rect);
        m_Second.ComputeContentBounds(rect);

        ::GetWindowRect(m_hWnd, &r);

        rect->left = min(rect->left, r.left);
        rect->top = min(rect->top, r.top);
        rect->right = max(rect->right, r.right);
        rect->bottom = max(rect->bottom, r.bottom);
    };

    virtual DWORD_PTR CountSizableChildren()
    {
        INT count = 1;
        count += m_First.CountSizableChildren();
        count += m_Second.CountSizableChildren();
        return count;
    };

    virtual HDWP OnParentSize(RECT parentRect, HDWP hDwp)
    {
        RECT rect = {0};

        SIZE content = {0};
        ComputeMinimalSize(&content);

        INT preferredWidth = m_Width.ComputeMeasure(parentRect.right - parentRect.left, content.cx);
        INT preferredHeight = m_Width.ComputeMeasure(parentRect.bottom - parentRect.top, content.cy);

        rect.right = preferredWidth;
        rect.bottom = preferredHeight;

        ComputeRect(parentRect, rect, &rect);

        SIZE growth = {0};
        if (m_HasOldRect)
        {
            RECT oldRect = m_LastRect;

            growth.cx = (parentRect.right - parentRect.left) - (oldRect.right - oldRect.left);
            growth.cy = (parentRect.bottom - parentRect.top) - (oldRect.bottom - oldRect.top);
        }

        RECT splitter = rect;
        RECT first = rect;
        RECT second = rect;

        if (m_Horizontal)
        {
            rect.top += m_MinFirst;
            rect.bottom -= THICKNESS + m_MinSecond;
            if (m_DynamicFirst)
            {
                if (growth.cy > 0)
                {
                    m_Pos += min(growth.cy, rect.bottom - (m_Pos + THICKNESS));
                }
                else if (growth.cy < 0)
                {
                    m_Pos += max(growth.cy, rect.top - m_Pos);
                }
            }

            if (m_Pos > rect.bottom)
                m_Pos = rect.bottom;

            if (m_Pos < rect.top)
                m_Pos = rect.top;

            splitter.top = m_Pos;
            splitter.bottom = m_Pos + THICKNESS;
            first.bottom = splitter.top;
            second.top = splitter.bottom;
        }
        else
        {
            rect.left += m_MinFirst;
            rect.right -= THICKNESS + m_MinSecond;
            if (m_DynamicFirst)
            {
                if (growth.cx > 0)
                {
                    m_Pos += min(growth.cx, rect.right - (m_Pos + THICKNESS));
                }
                else if (growth.cx < 0)
                {
                    m_Pos += max(growth.cy, rect.left - m_Pos);
                }
            }

            if (m_Pos > rect.right)
                m_Pos = rect.right;

            if (m_Pos < rect.left)
                m_Pos = rect.left;

            splitter.left = m_Pos;
            splitter.right = m_Pos + THICKNESS;
            first.right = splitter.left;
            second.left = splitter.right;
        }

        m_LastRect = parentRect;
        m_HasOldRect = TRUE;

        hDwp = m_First.OnParentSize(first, hDwp);
        hDwp = m_Second.OnParentSize(second, hDwp);

        if (hDwp)
        {
            return DeferWindowPos(hDwp, NULL,
                                  splitter.left, splitter.top,
                                  splitter.right - splitter.left,
                                  splitter.bottom - splitter.top,
                                  SWP_NOACTIVATE | SWP_NOZORDER);
        }
        else
        {
            SetWindowPos(NULL,
                         splitter.left, splitter.top,
                         splitter.right - splitter.left,
                         splitter.bottom - splitter.top,
                         SWP_NOACTIVATE | SWP_NOZORDER);
            return NULL;
        }
    };

private:
    BOOL ProcessWindowMessage(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT& theResult, DWORD dwMapId)
    {
        theResult = 0;
        switch (Msg)
        {
        case WM_SETCURSOR:
            SetCursor(m_hCursor);
            theResult = TRUE;
            break;

        case WM_LBUTTONDOWN:
            SetCapture();
            break;

        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
            if (GetCapture() == m_hWnd)
            {
                ReleaseCapture();
            }
            break;

        case WM_MOUSEMOVE:
            if (GetCapture() == m_hWnd)
            {
                POINT Point;
                GetCursorPos(&Point);
                ::ScreenToClient(GetParent(), &Point);
                if (m_Horizontal)
                    SetPos(Point.y);
                else
                    SetPos(Point.x);
            }
            break;

        default:
            return FALSE;
        }

        return TRUE;
    }

public:
    INT GetPos()
    {
        return m_Pos;
    }

    VOID SetPos(INT NewPos)
    {
        RECT rcParent;

        rcParent = m_LastRect;

        if (m_Horizontal)
        {
            rcParent.bottom -= THICKNESS;

            m_Pos = NewPos;

            if (m_Pos < rcParent.top)
                m_Pos = rcParent.top;

            if (m_Pos > rcParent.bottom)
                m_Pos = rcParent.bottom;
        }
        else
        {
            rcParent.right -= THICKNESS;

            m_Pos = NewPos;

            if (m_Pos < rcParent.left)
                m_Pos = rcParent.left;

            if (m_Pos > rcParent.right)
                m_Pos = rcParent.right;
        }

        INT count = CountSizableChildren();

        HDWP hdwp = NULL;
        hdwp = BeginDeferWindowPos(count);
        if (hdwp) hdwp = OnParentSize(m_LastRect, hdwp);
        if (hdwp) EndDeferWindowPos(hdwp);
    }

public:
    DECLARE_WND_CLASS_EX(_T("SplitterWindowClass"), CS_HREDRAW | CS_VREDRAW, COLOR_BTNFACE)

    /* Create splitter bar */
    HWND Create(HWND hwndParent)
    {
        if (m_Horizontal)
            m_hCursor = LoadCursor(0, IDC_SIZENS);
        else
            m_hCursor = LoadCursor(0, IDC_SIZEWE);

        DWORD style = WS_CHILD | WS_VISIBLE;
        DWORD exStyle = WS_EX_TRANSPARENT;

        RECT size = {205, 180, 465, THICKNESS};
        size.right += size.left;
        size.bottom += size.top;

        return CWindowImpl::Create(hwndParent, size, NULL, style, exStyle);
    }
};
