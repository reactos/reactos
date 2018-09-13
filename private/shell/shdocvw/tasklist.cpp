#include "priv.h"
#include "resource.h"
#include <trayp.h>

class TaskbarList : public ITaskbarList
{
public:
    TaskbarList();

    // IUnknown Methods
    STDMETHODIMP QueryInterface(REFIID,void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ITaskbarList Methods
    STDMETHODIMP HrInit(void);
    STDMETHODIMP AddTab(HWND hwnd);
    STDMETHODIMP DeleteTab(HWND hwnd);
    STDMETHODIMP ActivateTab(HWND hwnd);
    STDMETHODIMP SetActiveAlt(HWND hwnd);

protected:
    ~TaskbarList();
    HWND _HwndGetTaskbarList(void);

    UINT        _cRef;
    HWND        _hwndTaskbarList;
    int         _wm_shellhook;
};


//////////////////////////////////////////////////////////////////////////////
//
// TaskbarList Object
//
//////////////////////////////////////////////////////////////////////////////

STDAPI TaskbarList_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    // aggregation checking is handled in class factory
    TraceMsg(DM_TRACE, "TaskbarList - CreateInstance(...) called");
    
    TaskbarList *pTL = new TaskbarList();
    if (pTL) {
        *ppunk = SAFECAST(pTL, IUnknown *);
        return S_OK;
    }

    return E_OUTOFMEMORY;
}


TaskbarList::TaskbarList() 
{
    TraceMsg(DM_TRACE, "TL - TaskbarList() called.");
    _cRef = 1;
    _hwndTaskbarList = NULL;
    _wm_shellhook = RegisterWindowMessage(TEXT("SHELLHOOK"));
    DllAddRef();
}       


TaskbarList::~TaskbarList()
{
    ASSERT(_cRef == 0);                 // should always have zero
    TraceMsg(DM_TRACE, "TL - ~TaskbarList() called.");

    DllRelease();
}    

HWND TaskbarList::_HwndGetTaskbarList(void)
{
#ifndef UNIX
    if (_hwndTaskbarList && IsWindow(_hwndTaskbarList))
        return _hwndTaskbarList;

    _hwndTaskbarList = FindWindow(TEXT("Shell_TrayWnd"), NULL);
    if (_hwndTaskbarList)
        _hwndTaskbarList = (HWND)SendMessage(_hwndTaskbarList, WMTRAY_QUERY_VIEW, 0, 0);
    
    return _hwndTaskbarList;
#else
    return NULL;
#endif
}


//////////////////////////////////
//
// IUnknown Methods...
//
HRESULT TaskbarList::QueryInterface(REFIID iid, void **ppv)
{
    TraceMsg(DM_TRACE, "TaskbarList - QueryInterface() called.");
    
    if ((iid == IID_IUnknown) || (iid == IID_ITaskbarList)) {
        *ppv = (void *)(ITaskbarList *)this;
    }
    else {
        *ppv = NULL;    // null the out param
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

ULONG TaskbarList::AddRef()
{
    TraceMsg(DM_TRACE, "TaskbarList - AddRef() called.");
    
    return ++_cRef;
}

ULONG TaskbarList::Release()
{

    TraceMsg(DM_TRACE, "TaskbarList - Release() called.");
    
    if (--_cRef)
        return _cRef;

    delete this;
    return 0;   
}


//////////////////////////////////
//
// ITaskbarList Methods...
//
HRESULT TaskbarList::HrInit(void)
{
    HWND hwndTL = _HwndGetTaskbarList();

    if (hwndTL == NULL)
        return E_NOTIMPL;

    return S_OK;
}

HRESULT TaskbarList::AddTab(HWND hwnd)
{
    HWND hwndTL = _HwndGetTaskbarList();

    if (hwndTL)
        SendMessage(hwndTL, _wm_shellhook, HSHELL_WINDOWCREATED, (LPARAM) hwnd);

    return S_OK;
}

HRESULT TaskbarList::DeleteTab(HWND hwnd)
{
    HWND hwndTL = _HwndGetTaskbarList();

    if (hwndTL)
        SendMessage(hwndTL, _wm_shellhook, HSHELL_WINDOWDESTROYED, (LPARAM) hwnd);

    return S_OK;
}

HRESULT TaskbarList::ActivateTab(HWND hwnd)
{
    HWND hwndTL = _HwndGetTaskbarList();

    if (hwndTL) {
        SendMessage(hwndTL, _wm_shellhook, HSHELL_WINDOWACTIVATED, (LPARAM) hwnd);
        SendMessage(hwndTL, TBC_SETACTIVEALT , 0, (LPARAM) hwnd);
    }
    return S_OK;
}


HRESULT TaskbarList::SetActiveAlt(HWND hwnd)
{
    HWND hwndTL = _HwndGetTaskbarList();

    if (hwndTL)
        SendMessage(hwndTL, TBC_SETACTIVEALT , 0, (LPARAM) hwnd);

    return S_OK;
}

