/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    ctvctl.h

Abstract:

    header file for ctvctl.cpp

Author:

    William Hsieh (williamh) created

Revision History:


--*/
#include "..\inc\tvintf.h"

// CTVCtl.h : Declaration of the CTVCtrl OLE control class.


const int MAX_CONNECTIONS = 10;

/////////////////////////////////////////////////////////////////////////////
// CTVCtrl : See CTVCtl.cpp for implementation.

class CTVCtrl : public COleControl
{
        DECLARE_DYNCREATE(CTVCtrl)

// Constructor
public:
        CTVCtrl();

// Overrides
        // ClassWizard generated virtual function overrides
        //{{AFX_VIRTUAL(CTVCtrl)
        public:
        virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
        virtual void DoPropExchange(CPropExchange* pPX);
        virtual void OnResetState();
        virtual BOOL PreTranslateMessage(MSG* pMsg);
        //}}AFX_VIRTUAL

// Implementation
protected:
        ~CTVCtrl();

        DECLARE_OLECREATE_EX(CTVCtrl)    // Class factory and guid
        DECLARE_OLETYPELIB(CTVCtrl)      // GetTypeInfo
        DECLARE_PROPPAGEIDS(CTVCtrl)     // Property page IDs
        DECLARE_OLECTLTYPE(CTVCtrl)             // Type name and misc status

        // Subclassed control support
        BOOL PreCreateWindow(CREATESTRUCT& cs);
        BOOL IsSubclassedControl();
        LRESULT OnOcmCommand(WPARAM wParam, LPARAM lParam);

// Message maps
        //{{AFX_MSG(CTVCtrl)
        afx_msg void OnDestroy();
        afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
        //}}AFX_MSG
        afx_msg LRESULT OnOcmNotify(WPARAM wParam, LPARAM lParam);
        DECLARE_MESSAGE_MAP()

// Dispatch maps
        //{{AFX_DISPATCH(CTVCtrl)
        //}}AFX_DISPATCH
        DECLARE_DISPATCH_MAP()

// Event maps
        //{{AFX_EVENT(CTVCtrl)
        //}}AFX_EVENT
        DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
        enum {
        //{{AFX_DISP_ID(CTVCtrl)
        //}}AFX_DISP_ID
        };

protected:
        DECLARE_INTERFACE_MAP()
        BEGIN_INTERFACE_PART(DMTVOCX, IDMTVOCX)
            public: virtual HTREEITEM InsertItem(LPTV_INSERTSTRUCT pis);
            public: virtual HRESULT DeleteItem(HTREEITEM hItem);
            public: virtual HRESULT DeleteAllItems();
            public: virtual HIMAGELIST SetImageList(INT iImage, HIMAGELIST himl);
            public: virtual HRESULT SetItem(TV_ITEM* pitem);
            public: virtual HRESULT Expand(UINT Flags, HTREEITEM htiem);
            public: virtual HRESULT SelectItem(UINT Flags, HTREEITEM hitem);
            public: virtual HRESULT SetStyle(DWORD dwStyle);
            public: virtual HWND    GetWindowHandle();
            public: virtual HRESULT GetItem(TV_ITEM* pti);
            public: virtual HTREEITEM GetNextItem(UINT Flags, HTREEITEM htiRef);
            public: virtual HRESULT SelectItem(HTREEITEM hti);
            public: virtual UINT    GetCount();
            public: virtual HTREEITEM GetSelectedItem();
            public: virtual HRESULT Connect(IComponent* pIComponent, MMC_COOKIE);
            public: virtual HRESULT SetActiveConnection(MMC_COOKIE cookie);
            public: virtual MMC_COOKIE    GetActiveConnection();
            public: virtual HRESULT SetRedraw(BOOL Redraw);
            public: virtual BOOL    EnsureVisible(HTREEITEM hitem);
        END_INTERFACE_PART(DMTVOCX)


private:
        HTREEITEM InsertItem(LPTV_INSERTSTRUCT pis);
        HRESULT DeleteItem(HTREEITEM hItem);
        HRESULT DeleteAllItems();
        HIMAGELIST SetImageList(INT iImage, HIMAGELIST himl);
        HRESULT SetItem(TV_ITEM* pitem);
        HRESULT Expand(UINT Flags, HTREEITEM htiem);
        HRESULT SelectItem(UINT Flags, HTREEITEM hitem);
        HRESULT SetStyle(DWORD dwStyle);
        HWND    GetWindowHandle();
        HRESULT GetItem(TV_ITEM* pti);
        HTREEITEM GetNextItem(UINT Flags, HTREEITEM htiRef);
        HRESULT SelectItem(HTREEITEM hti);
        UINT    GetCount();
        HTREEITEM HitTest(LONG x, LONG y, UINT* phtFlags);
        HTREEITEM GetSelectedItem();
        HRESULT Connect(IComponent* pIComponent, MMC_COOKIE cookie);
        HRESULT SetActiveConnection(MMC_COOKIE cookie);
        MMC_COOKIE      GetActiveConnection();
        HRESULT SetRedraw(BOOL Redraw);
        BOOL    EnsureVisible(HTREEITEM hitem);
        TV_NOTIFY_CODE DoMouseNotification(UINT code, MMC_COOKIE* pcookie,LPARAM* parg, LPARAM* param);
// private data
        MMC_COOKIE      m_ActiveCookie;
        int     m_nConnections;
        BOOL    m_HasFocus;
        IComponent* m_pIComponent;
        ISnapinCallback* m_pISnapinCallback;
        BOOL    m_Destroyed;
};
