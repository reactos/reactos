#include "precomp.h"

#define NDEBUG
#include <debug.h>

static HINSTANCE hInstance;

typedef INT_PTR (WINAPI *PDEVICEPROPERTIESEXW)(HWND,LPCWSTR,LPCWSTR,DWORD,BOOL);

static VOID
ShowAdapterProperties(PDESKDISPLAYADAPTER This)
{
    HMODULE hDevMgr;
    PDEVICEPROPERTIESEXW pDevicePropertiesExW;

    hDevMgr = LoadLibrary(TEXT("devmgr.dll"));
    if (hDevMgr != NULL)
    {
        pDevicePropertiesExW = (PDEVICEPROPERTIESEXW)GetProcAddress(hDevMgr,
                                                                    "DevicePropertiesExW");
        if (pDevicePropertiesExW != NULL)
        {
            pDevicePropertiesExW(This->hwndDlg,
                                 NULL,
                                 This->lpDeviceId,
                                 0,
                                 FALSE);
        }

        FreeLibrary(hDevMgr);
    }
}

static VOID
InitDisplayAdapterDialog(PDESKDISPLAYADAPTER This)
{
    LPTSTR lpAdapterName;

    This->lpDeviceId = QueryDeskCplString(This->pdtobj,
                                          RegisterClipboardFormat(DESK_EXT_DISPLAYID));
    EnableWindow(GetDlgItem(This->hwndDlg,
                            IDC_ADAPTERPROPERTIES),
                 This->lpDeviceId != NULL && This->lpDeviceId[0] != TEXT('\0'));
    lpAdapterName = QueryDeskCplString(This->pdtobj,
                                       RegisterClipboardFormat(DESK_EXT_DISPLAYNAME));
    if (lpAdapterName != NULL)
    {
        SetDlgItemText(This->hwndDlg,
                       IDC_ADAPTERNAME,
                       lpAdapterName);

        LocalFree((HLOCAL)lpAdapterName);
    }

    if (This->DeskExtInterface != NULL)
    {
        SetDlgItemTextW(This->hwndDlg,
                        IDC_CHIPTYPE,
                        This->DeskExtInterface->ChipType);
        SetDlgItemTextW(This->hwndDlg,
                        IDC_DACTYPE,
                        This->DeskExtInterface->DacType);
        SetDlgItemTextW(This->hwndDlg,
                        IDC_MEMORYSIZE,
                        This->DeskExtInterface->MemorySize);
        SetDlgItemTextW(This->hwndDlg,
                        IDC_ADAPTERSTRING,
                        This->DeskExtInterface->AdapterString);
        SetDlgItemTextW(This->hwndDlg,
                        IDC_BIOSINFORMATION,
                        This->DeskExtInterface->BiosString);
    }
}

static INT_PTR CALLBACK
DisplayAdapterDlgProc(HWND hwndDlg,
                      UINT uMsg,
                      WPARAM wParam,
                      LPARAM lParam)
{
    PDESKDISPLAYADAPTER This;
    INT_PTR Ret = 0;

    if (uMsg != WM_INITDIALOG)
    {
        This = (PDESKDISPLAYADAPTER)GetWindowLongPtr(hwndDlg,
                                                     DWL_USER);
    }

    switch (uMsg)
    {
        case WM_INITDIALOG:
            This = (PDESKDISPLAYADAPTER)((LPCPROPSHEETPAGE)lParam)->lParam;
            This->hwndDlg = hwndDlg;
            SetWindowLongPtr(hwndDlg,
                             DWL_USER,
                             (LONG_PTR)This);

            InitDisplayAdapterDialog(This);
            Ret = TRUE;
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_ADAPTERPROPERTIES:
                    ShowAdapterProperties(This);
                    break;
            }

            break;
    }

    return Ret;
}

static VOID
IDeskDisplayAdapter_Destroy(PDESKDISPLAYADAPTER This)
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

    if (This->lpDeviceId != NULL)
    {
        LocalFree((HLOCAL)This->lpDeviceId);
        This->lpDeviceId = NULL;
    }
}

ULONG
IDeskDisplayAdapter_AddRef(PDESKDISPLAYADAPTER This)
{
    ULONG ret;

    ret = InterlockedIncrement((PLONG)&This->ref);
    if (ret == 1)
        InterlockedIncrement(&dll_refs);

    return ret;
}

ULONG
IDeskDisplayAdapter_Release(PDESKDISPLAYADAPTER This)
{
    ULONG ret;

    ret = InterlockedDecrement((PLONG)&This->ref);
    if (ret == 0)
    {
        IDeskDisplayAdapter_Destroy(This);
        InterlockedDecrement(&dll_refs);

        HeapFree(GetProcessHeap(),
                 0,
                 This);
    }

    return ret;
}

HRESULT STDMETHODCALLTYPE
IDeskDisplayAdapter_QueryInterface(PDESKDISPLAYADAPTER This,
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
        DPRINT1("IDeskDisplayAdapter::QueryInterface(%p,%p): E_NOINTERFACE\n", iid, pvObject);
        return E_NOINTERFACE;
    }

    IDeskDisplayAdapter_AddRef(This);
    return S_OK;
}

HRESULT
IDeskDisplayAdapter_Initialize(PDESKDISPLAYADAPTER This,
                               LPCITEMIDLIST pidlFolder,
                               IDataObject *pdtobj,
                               HKEY hkeyProgID)
{
    DPRINT1("IDeskDisplayAdapter::Initialize(%p,%p,%p)\n", pidlFolder, pdtobj, hkeyProgID);

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
IDeskDisplayAdapter_AddPages(PDESKDISPLAYADAPTER This,
                             LPFNADDPROPSHEETPAGE pfnAddPage,
                             LPARAM lParam)
{
    HPROPSHEETPAGE hpsp;
    PROPSHEETPAGE psp;

    DPRINT1("IDeskDisplayAdapter::AddPages(%p,%p)\n", pfnAddPage, lParam);

    psp.dwSize = sizeof(psp);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = hInstance;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_DISPLAYADAPTER);
    psp.pfnDlgProc = DisplayAdapterDlgProc;
    psp.lParam = (LPARAM)This;

    hpsp = CreatePropertySheetPage(&psp);
    if (hpsp != NULL && pfnAddPage(hpsp, lParam))
        return S_OK;

    return S_FALSE;
}

HRESULT
IDeskDisplayAdapter_ReplacePage(PDESKDISPLAYADAPTER This,
                                EXPPS uPageID,
                                LPFNADDPROPSHEETPAGE pfnReplacePage,
                                LPARAM lParam)
{
    DPRINT1("IDeskDisplayAdapter::ReplacePage(%u,%p,%p)\n", uPageID, pfnReplacePage, lParam);
    return E_NOTIMPL;
}

HRESULT
IDeskDisplayAdapter_Constructor(REFIID riid,
                                LPVOID *ppv)
{
    PDESKDISPLAYADAPTER This;
    HRESULT hRet = E_OUTOFMEMORY;

    DPRINT1("IDeskDisplayAdapter::Constructor(%p,%p)\n", riid, ppv);

    This = HeapAlloc(GetProcessHeap(),
                     0,
                     sizeof(*This));
    if (This != NULL)
    {
        ZeroMemory(This,
                   sizeof(*This));

        IDeskDisplayAdapter_InitIface(This);

        hRet = IDeskDisplayAdapter_QueryInterface(This,
                                                  riid,
                                                  ppv);
        if (!SUCCEEDED(hRet))
            IDeskDisplayAdapter_Release(This);
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
