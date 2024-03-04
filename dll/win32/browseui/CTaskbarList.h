/*
 * PROJECT:     browseui
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ITaskbarList header
 * COPYRIGHT:   Copyright 2018 Mark Jansen (mark.jansen@reactos.org)
 */

#ifndef _CTASKBARLIST_H_
#define _CTASKBARLIST_H_

class CTaskbarList :
    public CComCoClass<CTaskbarList, &CLSID_TaskbarList>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public ITaskbarList2
{
    HWND m_hTaskWnd;
    UINT m_ShellHookMsg;

    HWND TaskWnd();
    void SendTaskWndShellHook(WPARAM wParam, HWND hWnd);

public:
    CTaskbarList();
    virtual ~CTaskbarList();

    /*** ITaskbarList2 methods ***/
    STDMETHOD(MarkFullscreenWindow)(HWND hwnd, BOOL fFullscreen) override;

    /*** ITaskbarList methods ***/
    STDMETHOD(HrInit)() override;
    STDMETHOD(AddTab)(HWND hwnd) override;
    STDMETHOD(DeleteTab)(HWND hwnd) override;
    STDMETHOD(ActivateTab)(HWND hwnd) override;
    STDMETHOD(SetActiveAlt)(HWND hwnd) override;

    DECLARE_REGISTRY_RESOURCEID(IDR_TASKBARLIST)
    DECLARE_NOT_AGGREGATABLE(CTaskbarList)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CTaskbarList)
        COM_INTERFACE_ENTRY_IID(IID_ITaskbarList2, ITaskbarList2)
        COM_INTERFACE_ENTRY_IID(IID_ITaskbarList, ITaskbarList)
    END_COM_MAP()
};


#endif // _CTASKBARLIST_H_
