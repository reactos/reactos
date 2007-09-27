#include "precomp.h"

#define NDEBUG
#include <debug.h>

static HINSTANCE hInstance;

static VOID
InitMonitorDialog(PDESKMONITOR This)
{
    LPTSTR lpMonitorName;

    lpMonitorName = QueryDeskCplString(This->pdtobj,
                                       RegisterClipboardFormat(DESK_EXT_MONITORNAME));
    if (lpMonitorName != NULL)
    {
        SetDlgItemText(This->hwndDlg,
                       IDC_MONITORNAME,
                       lpMonitorName);

        LocalFree((HLOCAL)lpMonitorName);
    }

    if (This->DeskExtInterface != NULL)
    {
        /* FIXME */
    }
}

static INT_PTR CALLBACK
MonitorDlgProc(HWND hwndDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam)
{
    PDESKMONITOR This;
    INT_PTR Ret = 0;

    if (uMsg != WM_INITDIALOG)
    {
        This = (PDESKMONITOR)GetWindowLongPtr(hwndDlg,
                                              DWL_USER);
    }

    switch (uMsg)
    {
        case WM_INITDIALOG:
            This = (PDESKMONITOR)((LPCPROPSHEETPAGE)lParam)->lParam;
            This->hwndDlg = hwndDlg;
            SetWindowLongPtr(hwndDlg,
                             DWL_USER,
                             (LONG_PTR)This);

            InitMonitorDialog(This);
            Ret = TRUE;
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_MONITORPROPERTIES:
                    break;
            }

            break;
    }

    return Ret;
}

static VOID
IDeskMonitor_Destroy(PDESKMONITOR This)
{
    if (This->pdtobj != NULL)
    {
        IDataObject_Release(This->pdtobj);
        This->pdtobj = NULL;
    }

    if (This->DeskExtInterface != NULL)
    {
        LocalFree((HLOCAL)This->DeskExtInterface);
        This->DeskExtInterface = NULL;
    }
}

ULONG
IDeskMonitor_AddRef(PDESKMONITOR This)
{
    ULONG ret;

    ret = InterlockedIncrement((PLONG)&This->ref);
    if (ret == 1)
        InterlockedIncrement(&dll_refs);

    return ret;
}

ULONG
IDeskMonitor_Release(PDESKMONITOR This)
{
    ULONG ret;

    ret = InterlockedDecrement((PLONG)&This->ref);
    if (ret == 0)
    {
        IDeskMonitor_Destroy(This);
        InterlockedDecrement(&dll_refs);

        HeapFree(GetProcessHeap(),
                 0,
                 This);
    }

    return ret;
}

HRESULT STDMETHODCALLTYPE
IDeskMonitor_QueryInterface(PDESKMONITOR This,
                            REFIID iid,
                            PVOID *pvObject)
{
    *pvObject = NULL;

    if (IsEqualIID(iid,
                   &IID_IShellPropSheetExt) ||
        IsEqualIID(iid,
                   &IID_IUnknown))
    {
        *pvObject = impl_to_interface(This, IShellPropSheetExt);
    }
    else if (IsEqualIID(iid,
                        &IID_IShellExtInit))
    {
        *pvObject = impl_to_interface(This, IShellExtInit);
    }
    else if (IsEqualIID(iid,
                        &IID_IClassFactory))
    {
        *pvObject = impl_to_interface(This, IClassFactory);
    }
    else
    {
        DPRINT1("IDeskMonitor::QueryInterface(%p,%p): E_NOINTERFACE\n", iid, pvObject);
        return E_NOINTERFACE;
    }

    IDeskMonitor_AddRef(This);
    return S_OK;
}

HRESULT
IDeskMonitor_Initialize(PDESKMONITOR This,
                        LPCITEMIDLIST pidlFolder,
                        IDataObject *pdtobj,
                        HKEY hkeyProgID)
{
    DPRINT1("IDeskMonitor::Initialize(%p,%p,%p)\n", pidlFolder, pdtobj, hkeyProgID);

    if (pdtobj != NULL)
    {
        IDataObject_AddRef(pdtobj);
        This->pdtobj = pdtobj;

        /* Get a copy of the desk.cpl extension interface */
        This->DeskExtInterface = QueryDeskCplExtInterface(This->pdtobj);
        if (This->DeskExtInterface != NULL)
            return S_OK;
    }

    return S_FALSE;
}

HRESULT
IDeskMonitor_AddPages(PDESKMONITOR This,
                      LPFNADDPROPSHEETPAGE pfnAddPage,
                      LPARAM lParam)
{
    HPROPSHEETPAGE hpsp;
    PROPSHEETPAGE psp;

    DPRINT1("IDeskMonitor::AddPages(%p,%p)\n", pfnAddPage, lParam);

    psp.dwSize = sizeof(psp);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = hInstance;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_MONITOR);
    psp.pfnDlgProc = MonitorDlgProc;
    psp.lParam = (LPARAM)This;

    hpsp = CreatePropertySheetPage(&psp);
    if (hpsp != NULL && pfnAddPage(hpsp, lParam))
        return S_OK;

    return S_FALSE;
}

HRESULT
IDeskMonitor_ReplacePage(PDESKMONITOR This,
                         EXPPS uPageID,
                         LPFNADDPROPSHEETPAGE pfnReplacePage,
                         LPARAM lParam)
{
    DPRINT1("IDeskMonitor::ReplacePage(%u,%p,%p)\n", uPageID, pfnReplacePage, lParam);
    return E_NOTIMPL;
}

HRESULT
IDeskMonitor_Constructor(REFIID riid,
                         LPVOID *ppv)
{
    PDESKMONITOR This;
    HRESULT hRet = E_OUTOFMEMORY;

    DPRINT1("IDeskMonitor::Constructor(%p,%p)\n", riid, ppv);

    This = HeapAlloc(GetProcessHeap(),
                     0,
                     sizeof(*This));
    if (This != NULL)
    {
        ZeroMemory(This,
                   sizeof(*This));

        IDeskMonitor_InitIface(This);

        hRet = IDeskMonitor_QueryInterface(This,
                                           riid,
                                           ppv);
        if (!SUCCEEDED(hRet))
            IDeskMonitor_Release(This);
    }

    return hRet;
}

BOOL STDCALL
DllMain(HINSTANCE hinstDLL,
        DWORD dwReason,
        LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            hInstance = hinstDLL;
            DisableThreadLibraryCalls(hInstance);
            break;
    }

    return TRUE;
}
