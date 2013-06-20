#include "precomp.h"

#define NDEBUG
#include <debug.h>

static HINSTANCE hInstance;

#ifdef UNICODE
typedef INT_PTR (WINAPI *PDEVICEPROPERTIES)(HWND,LPCWSTR,LPCWSTR,BOOL);
#define FUNC_DEVICEPROPERTIES "DevicePropertiesW"
#else
typedef INT_PTR (WINAPI *PDEVICEPROPERTIES)(HWND,LPCSTR,LPCSTR,BOOL);
#define FUNC_DEVICEPROPERTIES "DevicePropertiesA"
#endif

static LPTSTR
GetMonitorDevInstID(LPCTSTR lpDeviceID)
{
    /* FIXME: Implement, allocate returned string with LocalAlloc! */
    return NULL;
}

static VOID
ShowMonitorProperties(PDESKMONITOR This)
{
    HMODULE hDevMgr;
    PDEVICEPROPERTIES pDeviceProperties;
    LPTSTR lpDevInstID;

    if (This->SelMonitor != NULL)
    {
        lpDevInstID = GetMonitorDevInstID(This->SelMonitor->dd.DeviceID);
        if (lpDevInstID != NULL)
        {
            hDevMgr = LoadLibrary(TEXT("devmgr.dll"));
            if (hDevMgr != NULL)
            {
                pDeviceProperties = (PDEVICEPROPERTIES)GetProcAddress(hDevMgr,
                                                                      FUNC_DEVICEPROPERTIES);
                if (pDeviceProperties != NULL)
                {
                    pDeviceProperties(This->hwndDlg,
                                       NULL,
                                       This->SelMonitor->dd.DeviceID,
                                       FALSE);
                }

                FreeLibrary(hDevMgr);
            }

            LocalFree((HLOCAL)lpDevInstID);
        }
    }
}

static VOID
UpdateMonitorSelection(PDESKMONITOR This)
{
    INT i;

    if (This->dwMonitorCount > 1)
    {
        This->SelMonitor = NULL;

        i = (INT)SendDlgItemMessage(This->hwndDlg,
                                    IDC_MONITORLIST,
                                    LB_GETCURSEL,
                                    0,
                                    0);
        if (i >= 0)
        {
            This->SelMonitor = (PDESKMONINFO)SendDlgItemMessage(This->hwndDlg,
                                                                IDC_MONITORLIST,
                                                                LB_GETITEMDATA,
                                                                (WPARAM)i,
                                                                0);
        }
    }
    else
        This->SelMonitor = This->Monitors;

    EnableWindow(GetDlgItem(This->hwndDlg,
                            IDC_MONITORPROPERTIES),
                 This->SelMonitor != NULL);
}

static VOID
UpdatePruningControls(PDESKMONITOR This)
{
    EnableWindow(GetDlgItem(This->hwndDlg,
                            IDC_PRUNINGCHECK),
                 This->bModesPruned && !This->bKeyIsReadOnly);
    CheckDlgButton(This->hwndDlg,
                   IDC_PRUNINGCHECK,
                   (This->bModesPruned && This->bPruningOn) ? BST_CHECKED : BST_UNCHECKED);
}

static VOID
GetPruningSettings(PDESKMONITOR This)
{
    BOOL bModesPruned = FALSE, bKeyIsReadOnly = FALSE, bPruningOn = FALSE;

    if (This->DeskExtInterface != NULL)
    {
        This->DeskExtInterface->GetPruningMode(This->DeskExtInterface->Context,
                                               &bModesPruned,
                                               &bKeyIsReadOnly,
                                               &bPruningOn);
    }

    /* Check the boolean values against zero before assigning it to the bitfields! */
    This->bModesPruned = (bModesPruned != FALSE);
    This->bKeyIsReadOnly = (bKeyIsReadOnly != FALSE);
    This->bPruningOn = (bPruningOn != FALSE);

    UpdatePruningControls(This);
}

static VOID
UpdateRefreshFrequencyList(PDESKMONITOR This)
{
    PDEVMODEW lpCurrentMode, lpMode;
    TCHAR szBuffer[64];
    DWORD dwIndex;
    INT i;
    BOOL bHasDef = FALSE;
    BOOL bAdded = FALSE;

    /* Fill the refresh rate combo box */
    SendDlgItemMessage(This->hwndDlg,
                       IDC_REFRESHRATE,
                       CB_RESETCONTENT,
                       0,
                       0);

    lpCurrentMode = This->DeskExtInterface->GetCurrentMode(This->DeskExtInterface->Context);
    dwIndex = 0;

    do
    {
        lpMode = This->DeskExtInterface->EnumAllModes(This->DeskExtInterface->Context,
                                                      dwIndex++);
        if (lpMode != NULL &&
            lpMode->dmBitsPerPel == lpCurrentMode->dmBitsPerPel &&
            lpMode->dmPelsWidth == lpCurrentMode->dmPelsWidth &&
            lpMode->dmPelsHeight == lpCurrentMode->dmPelsHeight)
        {
            /* We're only interested in refresh rates for the current resolution and color depth */

            if (lpMode->dmDisplayFrequency <= 1)
            {
                /* Default hardware frequency */
                if (bHasDef)
                    continue;

                bHasDef = TRUE;

                if (!LoadString(hInstance,
                                IDS_USEDEFFRQUENCY,
                                szBuffer,
                                sizeof(szBuffer) / sizeof(szBuffer[0])))
                {
                    szBuffer[0] = TEXT('\0');
                }
            }
            else
            {
                TCHAR szFmt[64];

                if (!LoadString(hInstance,
                                IDS_FREQFMT,
                                szFmt,
                                sizeof(szFmt) / sizeof(szFmt[0])))
                {
                    szFmt[0] = TEXT('\0');
                }

                _sntprintf(szBuffer,
                           sizeof(szBuffer) / sizeof(szBuffer[0]),
                           szFmt,
                           lpMode->dmDisplayFrequency);
            }

            i = (INT)SendDlgItemMessage(This->hwndDlg,
                                        IDC_REFRESHRATE,
                                        CB_ADDSTRING,
                                        0,
                                        (LPARAM)szBuffer);
            if (i >= 0)
            {
                bAdded = TRUE;

                SendDlgItemMessage(This->hwndDlg,
                                   IDC_REFRESHRATE,
                                   CB_SETITEMDATA,
                                   (WPARAM)i,
                                   (LPARAM)lpMode);

                if (lpMode->dmDisplayFrequency == lpCurrentMode->dmDisplayFrequency)
                {
                    SendDlgItemMessage(This->hwndDlg,
                                       IDC_REFRESHRATE,
                                       CB_SETCURSEL,
                                       (WPARAM)i,
                                       0);
                }
            }
        }

    } while (lpMode != NULL);

    EnableWindow(GetDlgItem(This->hwndDlg,
                            IDS_MONITORSETTINGSGROUP),
                 bAdded);
    EnableWindow(GetDlgItem(This->hwndDlg,
                            IDS_REFRESHRATELABEL),
                 bAdded);
    EnableWindow(GetDlgItem(This->hwndDlg,
                            IDC_REFRESHRATE),
                 bAdded);

    GetPruningSettings(This);
}

static VOID
InitMonitorDialog(PDESKMONITOR This)
{
    PDESKMONINFO pmi, pminext, *pmilink;
    DISPLAY_DEVICE dd;
    BOOL bRet;
    INT i;
    DWORD dwIndex;

    /* Free all allocated monitors */
    pmi = This->Monitors;
    This->Monitors = NULL;
    while (pmi != NULL)
    {
        pminext = pmi->Next;
        LocalFree((HLOCAL)pmi);
        pmi = pminext;
    }

    This->SelMonitor = NULL;
    This->dwMonitorCount = 0;

    if (This->lpDisplayDevice != NULL)
        LocalFree((HLOCAL)This->lpDisplayDevice);

    This->lpDisplayDevice = QueryDeskCplString(This->pdtobj,
                                               RegisterClipboardFormat(DESK_EXT_DISPLAYDEVICE));

    if (This->DeskExtInterface != NULL)
    {
        if (This->lpDisplayDevice != NULL)
        {
            /* Enumerate all monitors */
            dwIndex = 0;
            pmilink = &This->Monitors;

            do
            {
                dd.cb = sizeof(dd);
                bRet = EnumDisplayDevices(This->lpDisplayDevice,
                                          dwIndex++,
                                          &dd,
                                          0);
                if (bRet)
                {
                    pmi = LocalAlloc(LMEM_FIXED,
                                     sizeof(*pmi));
                    if (pmi != NULL)
                    {
                        CopyMemory(&pmi->dd,
                                   &dd,
                                   sizeof(dd));
                        pmi->Next = NULL;
                        *pmilink = pmi;
                        pmilink = &pmi->Next;

                        This->dwMonitorCount++;
                    }
                }
            } while (bRet);
        }

        This->lpDevModeOnInit = This->DeskExtInterface->GetCurrentMode(This->DeskExtInterface->Context);
    }
    else
        This->lpDevModeOnInit = NULL;

    This->lpSelDevMode = This->lpDevModeOnInit;

    /* Setup the UI depending on how many monitors are attached */
    if (This->dwMonitorCount == 0)
    {
        LPTSTR lpMonitorName;

        /* This is a fallback, let's hope that desk.cpl can provide us with a monitor name */
        lpMonitorName = QueryDeskCplString(This->pdtobj,
                                           RegisterClipboardFormat(DESK_EXT_MONITORNAME));

        SetDlgItemText(This->hwndDlg,
                       IDC_MONITORNAME,
                       lpMonitorName);

        if (lpMonitorName != NULL)
            LocalFree((HLOCAL)lpMonitorName);
    }
    else if (This->dwMonitorCount == 1)
    {
        This->SelMonitor = This->Monitors;
        SetDlgItemText(This->hwndDlg,
                       IDC_MONITORNAME,
                       This->Monitors->dd.DeviceString);
    }
    else
    {
        SendDlgItemMessage(This->hwndDlg,
                           IDC_MONITORLIST,
                           LB_RESETCONTENT,
                           0,
                           0);

        pmi = This->Monitors;
        while (pmi != NULL)
        {
            i = (INT)SendDlgItemMessage(This->hwndDlg,
                                        IDC_MONITORLIST,
                                        LB_ADDSTRING,
                                        0,
                                        (LPARAM)pmi->dd.DeviceString);
            if (i >= 0)
            {
                SendDlgItemMessage(This->hwndDlg,
                                   IDC_MONITORLIST,
                                   LB_SETITEMDATA,
                                   (WPARAM)i,
                                   (LPARAM)pmi);

                if (This->SelMonitor == NULL)
                {
                    SendDlgItemMessage(This->hwndDlg,
                                       IDC_MONITORLIST,
                                       LB_SETCURSEL,
                                       (WPARAM)i,
                                       0);

                    This->SelMonitor = pmi;
                }
            }

            pmi = pmi->Next;
        }
    }

    /* Show/Hide controls */
    ShowWindow(GetDlgItem(This->hwndDlg,
                          IDC_MONITORNAME),
               (This->dwMonitorCount <= 1 ? SW_SHOW : SW_HIDE));
    ShowWindow(GetDlgItem(This->hwndDlg,
                          IDC_MONITORLIST),
               (This->dwMonitorCount > 1 ? SW_SHOW : SW_HIDE));

    UpdateRefreshFrequencyList(This);
    UpdateMonitorSelection(This);
}

static VOID
UpdatePruningSelection(PDESKMONITOR This)
{
    BOOL bPruningOn;

    if (This->DeskExtInterface != NULL && This->bModesPruned && !This->bKeyIsReadOnly)
    {
        bPruningOn = IsDlgButtonChecked(This->hwndDlg,
                                        IDC_PRUNINGCHECK) != BST_UNCHECKED;

        if (bPruningOn != This->bPruningOn)
        {
            /* Tell desk.cpl to turn on/off pruning mode */
            This->bPruningOn = bPruningOn;
            This->DeskExtInterface->SetPruningMode(This->DeskExtInterface->Context,
                                                   bPruningOn);

            /* Fill the refresh rate combobox again, we now receive a filtered
               or unfiltered device mode list from desk.cpl (depending on whether
               pruning is active or not) */
            UpdateRefreshFrequencyList(This);

            (void)PropSheet_Changed(GetParent(This->hwndDlg),
                                    This->hwndDlg);
        }
    }
}

static VOID
UpdateRefreshRateSelection(PDESKMONITOR This)
{
    PDEVMODEW lpCurrentDevMode;
    INT i;

    if (This->DeskExtInterface != NULL)
    {
        i = (INT)SendDlgItemMessage(This->hwndDlg,
                                    IDC_REFRESHRATE,
                                    CB_GETCURSEL,
                                    0,
                                    0);
        if (i >= 0)
        {
            lpCurrentDevMode = This->lpSelDevMode;
            This->lpSelDevMode = (PDEVMODEW)SendDlgItemMessage(This->hwndDlg,
                                                               IDC_REFRESHRATE,
                                                               CB_GETITEMDATA,
                                                               (WPARAM)i,
                                                               0);

            if (This->lpSelDevMode != NULL && This->lpSelDevMode != lpCurrentDevMode)
            {
                This->DeskExtInterface->SetCurrentMode(This->DeskExtInterface->Context,
                                                       This->lpSelDevMode);

                UpdateRefreshFrequencyList(This);

                (void)PropSheet_Changed(GetParent(This->hwndDlg),
                                        This->hwndDlg);
            }
        }
    }
}

static LONG
ApplyMonitorChanges(PDESKMONITOR This)
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
            This->lpSelDevMode = This->lpDevModeOnInit;
            return PSNRET_NOERROR;
        }
        else if (lChangeRet == DISP_CHANGE_RESTART)
        {
            /* Notify desk.cpl that the user needs to reboot */
            PropSheet_RestartWindows(GetParent(This->hwndDlg));
            return PSNRET_NOERROR;
        }
    }

    InitMonitorDialog(This);

    return PSNRET_INVALID_NOCHANGEPAGE;
}

static VOID
ResetMonitorChanges(PDESKMONITOR This)
{
    if (This->DeskExtInterface != NULL && This->lpDevModeOnInit != NULL)
    {
        This->DeskExtInterface->SetCurrentMode(This->DeskExtInterface->Context,
                                               This->lpDevModeOnInit);
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
                    ShowMonitorProperties(This);
                    break;

                case IDC_MONITORLIST:
                    if (HIWORD(wParam) == LBN_SELCHANGE)
                        UpdateMonitorSelection(This);
                    break;

                case IDC_PRUNINGCHECK:
                    if (HIWORD(wParam) == BN_CLICKED)
                        UpdatePruningSelection(This);
                    break;

                case IDC_REFRESHRATE:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                        UpdateRefreshRateSelection(This);
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
                    SetWindowLongPtr(hwndDlg,
                                     DWL_MSGRESULT,
                                     ApplyMonitorChanges(This));
                    break;
                }

                case PSN_RESET:
                    ResetMonitorChanges(This);
                    break;

                case PSN_SETACTIVE:
                    UpdateRefreshFrequencyList(This);
                    break;
            }
            break;
        }
    }

    return Ret;
}

static VOID
IDeskMonitor_Destroy(PDESKMONITOR This)
{
    PDESKMONINFO pmi, pminext;

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

    if (This->lpDisplayDevice != NULL)
    {
        LocalFree((HLOCAL)This->lpDisplayDevice);
        This->lpDisplayDevice = NULL;
    }

    /* Free all monitors */
    pmi = This->Monitors;
    This->Monitors = NULL;
    while (pmi != NULL)
    {
        pminext = pmi->Next;
        LocalFree((HLOCAL)pmi);
        pmi = pminext;
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

BOOL WINAPI
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
