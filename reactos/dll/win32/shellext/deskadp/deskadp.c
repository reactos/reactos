#include "precomp.h"

#define NDEBUG
#include <debug.h>

static HINSTANCE hInstance;

typedef INT_PTR (WINAPI *PDEVICEPROPERTIESW)(HWND,LPCWSTR,LPCWSTR,BOOL);

static VOID
GetColorDescription(PDEVMODEW lpDevMode,
                    LPTSTR lpBuffer,
                    DWORD dwBufferLen)
{
    UINT uid = 0;

    switch (lpDevMode->dmBitsPerPel)
    {
        case 4:
            uid = IDS_4BPP;
            break;
        case 8:
            uid = IDS_8BPP;
            break;
        case 15:
            uid = IDS_15BPP;
            break;
        case 16:
            uid = IDS_16BPP;
            break;
        case 24:
            uid = IDS_24BPP;
            break;
        case 32:
            uid = IDS_32BPP;
            break;
    }

    if (uid == 0 ||
        !LoadString(hInstance,
                    uid,
                    lpBuffer,
                    dwBufferLen))
    {
        lpBuffer[0] = TEXT('\0');
    }
}

static VOID
GetRefreshRateDescription(PDEVMODEW lpDevMode,
                          LPTSTR lpBuffer,
                          DWORD dwBufferLen)
{
    TCHAR szFormat[64];

    if (lpDevMode->dmDisplayFrequency <= 1)
    {
        if (!LoadString(hInstance,
                        IDS_DEFREFRESHRATE,
                        lpBuffer,
                        dwBufferLen))
        {
            lpBuffer[0] = TEXT('\0');
        }
    }
    else
    {
        if (!LoadString(hInstance,
                        IDES_REFRESHRATEFMT,
                        szFormat,
                        sizeof(szFormat) / sizeof(szFormat[0])))
        {
            szFormat[0] = TEXT('\0');
        }

        _sntprintf(lpBuffer,
                   dwBufferLen,
                   szFormat,
                   lpDevMode->dmDisplayFrequency);
    }
}

static VOID
InitListAllModesDialog(PDESKDISPLAYADAPTER This,
                       HWND hwndListAllModesDlg)
{
    TCHAR szFormat[64], szBuffer[64], szColors[64], szRefreshRate[64];
    PDEVMODEW lpDevMode, lpCurrentDevMode;
    DWORD dwIndex = 0;
    INT i;

    if (This->DeskExtInterface != NULL)
    {
        if (!LoadString(hInstance,
                        IDS_MODEFMT,
                        szFormat,
                        sizeof(szFormat) / sizeof(szFormat[0])))
        {
            szFormat[0] = TEXT('\0');
        }

        lpCurrentDevMode = This->DeskExtInterface->GetCurrentMode(This->DeskExtInterface->Context);

        do
        {
            lpDevMode = This->DeskExtInterface->EnumAllModes(This->DeskExtInterface->Context,
                                                             dwIndex++);
            if (lpDevMode != NULL)
            {
                GetColorDescription(lpDevMode,
                                    szColors,
                                    sizeof(szColors) / sizeof(szColors[0]));

                GetRefreshRateDescription(lpDevMode,
                                          szRefreshRate,
                                          sizeof(szRefreshRate) / sizeof(szRefreshRate[0]));

                _sntprintf(szBuffer,
                           sizeof(szBuffer) / sizeof(szBuffer[0]),
                           szFormat,
                           lpDevMode->dmPelsWidth,
                           lpDevMode->dmPelsHeight,
                           szColors,
                           szRefreshRate);

                i = (INT)SendDlgItemMessage(hwndListAllModesDlg,
                                            IDC_ALLVALIDMODES,
                                            LB_ADDSTRING,
                                            0,
                                            (LPARAM)szBuffer);
                if (i >= 0)
                {
                    SendDlgItemMessage(hwndListAllModesDlg,
                                       IDC_ALLVALIDMODES,
                                       LB_SETITEMDATA,
                                       (WPARAM)i,
                                       (LPARAM)lpDevMode);

                    if (lpDevMode == lpCurrentDevMode)
                    {
                        SendDlgItemMessage(hwndListAllModesDlg,
                                           IDC_ALLVALIDMODES,
                                           LB_SETCURSEL,
                                           (WPARAM)i,
                                           0);
                    }
                }
            }

        } while (lpDevMode != NULL);
    }
}

static BOOL
ChangeSelectedMode(PDESKDISPLAYADAPTER This,
                   HWND hwndListAllModesDlg)
{
    INT i;
    PDEVMODEW lpSelDevMode = NULL;
    BOOL bRet = FALSE;

    i = (INT)SendDlgItemMessage(hwndListAllModesDlg,
                                IDC_ALLVALIDMODES,
                                LB_GETCURSEL,
                                0,
                                0);

    if (i >= 0)
    {
        lpSelDevMode = (PDEVMODEW)SendDlgItemMessage(hwndListAllModesDlg,
                                                     IDC_ALLVALIDMODES,
                                                     LB_GETITEMDATA,
                                                     (WPARAM)i,
                                                     0);
    }

    if (lpSelDevMode != NULL)
    {
        This->lpSelDevMode = lpSelDevMode;
        bRet = TRUE;
    }

    return bRet;
}

static INT_PTR CALLBACK
ListAllModesDlgProc(HWND hwndDlg,
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
            This = (PDESKDISPLAYADAPTER)lParam;
            SetWindowLongPtr(hwndDlg,
                             DWL_USER,
                             (LONG_PTR)This);

            InitListAllModesDialog(This,
                                   hwndDlg);
            Ret = TRUE;
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    if (ChangeSelectedMode(This,
                                           hwndDlg))
                    {
                        EndDialog(hwndDlg,
                                  IDOK);
                    }
                    break;

                case IDCANCEL:
                    EndDialog(hwndDlg,
                              IDCANCEL);
                    break;
            }
            break;

        case WM_CLOSE:
            EndDialog(hwndDlg,
                      IDCANCEL);
            break;
    }

    return Ret;
}

static VOID
ShowListAllModes(PDESKDISPLAYADAPTER This)
{
    PDEVMODEW lpPrevSel;

    lpPrevSel = This->lpSelDevMode;

    if (This->DeskExtInterface != NULL &&
        DialogBoxParam(hInstance,
                       MAKEINTRESOURCE(IDD_LISTALLMODES),
                       This->hwndDlg,
                       ListAllModesDlgProc,
                       (LPARAM)This) == IDOK)
    {
        if (lpPrevSel != This->lpSelDevMode)
        {
            (void)PropSheet_Changed(GetParent(This->hwndDlg),
                                    This->hwndDlg);
            This->DeskExtInterface->SetCurrentMode(This->DeskExtInterface->Context,
                                                   This->lpSelDevMode);
        }
    }
}

static VOID
ShowAdapterProperties(PDESKDISPLAYADAPTER This)
{
    HMODULE hDevMgr;
    PDEVICEPROPERTIESW pDevicePropertiesW;

    hDevMgr = LoadLibrary(TEXT("devmgr.dll"));
    if (hDevMgr != NULL)
    {
        pDevicePropertiesW = (PDEVICEPROPERTIESW)GetProcAddress(hDevMgr,
                                                                "DevicePropertiesW");
        if (pDevicePropertiesW != NULL)
        {
            pDevicePropertiesW(This->hwndDlg,
                               NULL,
                               This->lpDeviceId,
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

        This->lpDevModeOnInit = This->DeskExtInterface->GetCurrentMode(This->DeskExtInterface->Context);
    }
    else
        This->lpDevModeOnInit = NULL;

    This->lpSelDevMode = This->lpDevModeOnInit;
}

static LONG
ApplyDisplayAdapterChanges(PDESKDISPLAYADAPTER This)
{
    LONG lChangeRet;

    if (This->DeskExtInterface != NULL)
    {
        /* Change the display settings through desk.cpl */
        lChangeRet = DeskCplExtDisplaySaveSettings(This->DeskExtInterface,
                                                   This->hwndDlg);
        if (lChangeRet == DISP_CHANGE_SUCCESSFUL)
        {
            /* Save the new mode */
            This->lpDevModeOnInit = This->DeskExtInterface->GetCurrentMode(This->DeskExtInterface->Context);
            return PSNRET_NOERROR;
        }
        else if (lChangeRet == DISP_CHANGE_RESTART)
        {
            /* Notify desk.cpl that the user needs to reboot */
            PropSheet_RestartWindows(GetParent(This->hwndDlg));
            return PSNRET_NOERROR;
        }
    }

    return PSNRET_INVALID_NOCHANGEPAGE;
}

static VOID
ResetDisplayAdapterChanges(PDESKDISPLAYADAPTER This)
{
    if (This->DeskExtInterface != NULL && This->lpDevModeOnInit != NULL)
    {
        This->DeskExtInterface->SetCurrentMode(This->DeskExtInterface->Context,
                                               This->lpDevModeOnInit);
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

                case IDC_LISTALLMODES:
                    ShowListAllModes(This);
                    break;
            }

            break;

        case WM_NOTIFY:
        {
            NMHDR *nmh = (NMHDR *)lParam;

            switch (nmh->code)
            {
                case PSN_APPLY:
                {
                    SetWindowLong(hwndDlg,
                                  DWL_MSGRESULT,
                                  ApplyDisplayAdapterChanges(This));
                    break;
                }

                case PSN_RESET:
                    ResetDisplayAdapterChanges(This);
                    break;
            }
            break;
        }
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
