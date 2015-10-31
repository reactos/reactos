/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig_new/srvpage.cpp
 * PURPOSE:     Services page message handler
 * COPYRIGHT:   Copyright 2005-2006 Christoph von Wittich <Christoph@ApiViewer.de>
 *              Copyright 2011-2012 Hermes BELUSCA - MAITO <hermes.belusca@sfr.fr>
 *
 */

#include "precomp.h"
#include "utils.h"
#include "regutils.h"
#include "stringutils.h"
// #include "CmdLineParser.h"
#include "listview.h"
#include "uxthemesupp.h"

#include <winsvc.h>

// #include <atlbase.h>
#include <atlcoll.h>
#include <atlstr.h>

static HWND hServicesPage        = NULL;
static HWND hServicesListCtrl    = NULL;
static int  iSortedColumn        = 0;
static BOOL bMaskProprietarySvcs = FALSE;

DWORD GetServicesActivation(VOID)
{
    DWORD dwServices = 0;
    RegGetDWORDValue(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Shared Tools\\MSConfig\\state", L"services", &dwServices);
    return dwServices;
}

BOOL SetServicesActivation(DWORD dwState)
{
    return (RegSetDWORDValue(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Shared Tools\\MSConfig\\state", L"services", TRUE, dwState) == ERROR_SUCCESS);
}

static BOOL
RegisterNoMsgAnymore(VOID)
{
    return (RegSetDWORDValue(HKEY_CURRENT_USER /* HKEY_LOCAL_MACHINE ?? */,
                             L"SOFTWARE\\Microsoft\\Shared Tools\\MSConfig",
                             L"HideEssentialServiceWarning",
                             TRUE, 1) == ERROR_SUCCESS);
}

BOOL
HideEssentialServiceWarning(VOID)
{
    BOOL  bRetVal = FALSE;
    DWORD dwValue = 0;

    bRetVal = ( (RegGetDWORDValue(HKEY_CURRENT_USER /* HKEY_LOCAL_MACHINE ?? */,
                                  L"SOFTWARE\\Microsoft\\Shared Tools\\MSConfig",
                                  L"HideEssentialServiceWarning",
                                  &dwValue) == ERROR_SUCCESS) &&
                (dwValue == 1) );

    return bRetVal;
}

struct ServiceItem
{
    ServiceItem(const LPCWSTR lpszSvcName,
                BOOL          bIsEnabled,
                BOOL          bIsRequired) :
        m_lpszSvcName(lpszSvcName),
        m_bIsEnabled(bIsEnabled),
        m_bIsRequired(bIsRequired)
    { }

    ~ServiceItem(void)
    { }

    CAtlStringW m_lpszSvcName;
    BOOL        m_bIsEnabled;
    BOOL        m_bIsRequired;
};

struct RegistryDisabledServiceItemParams
{
    BOOL bIsPresent;
    BOOL bIsKeyed; // bIsKeyed == TRUE for a keyed-registered service ; == FALSE for a valued-registered service.
    DWORD dwStartType;
    SYSTEMTIME time;
};

static CAtlList<CAtlStringW> userModificationsList;

QUERY_REGISTRY_VALUES_ROUTINE(GetRegistryValuedDisabledServicesQueryRoutine)
{
    UNREFERENCED_PARAMETER(KeyName);
    UNREFERENCED_PARAMETER(ValueData);
    UNREFERENCED_PARAMETER(ValueLength);

    if (!EntryContext)
        return ERROR_SUCCESS;

    RegistryDisabledServiceItemParams* pContextParams = (RegistryDisabledServiceItemParams*)EntryContext;
    if (pContextParams->bIsPresent)
        return ERROR_SUCCESS;

    if ( (hRootKey == HKEY_LOCAL_MACHINE) && (ValueType == REG_DWORD) && (ValueLength == sizeof(DWORD)) &&
         (wcsicmp((LPCWSTR)Context, ValueName) == 0) )
    {
        pContextParams->bIsPresent  = TRUE;
        pContextParams->bIsKeyed    = FALSE;
        pContextParams->dwStartType = *(DWORD*)ValueData;
        // pContextParams->time        = {};
    }
    else
    {
        pContextParams->bIsPresent  = FALSE;
        pContextParams->bIsKeyed    = FALSE;
        pContextParams->dwStartType = 0;
        // pContextParams->time        = {};
    }

    return ERROR_SUCCESS;
}

QUERY_REGISTRY_KEYS_ROUTINE(GetRegistryKeyedDisabledServicesQueryRoutine)
{
    UNREFERENCED_PARAMETER(hRootKey);
    UNREFERENCED_PARAMETER(KeyName);

    if (!EntryContext)
        return ERROR_SUCCESS;

    RegistryDisabledServiceItemParams* pContextParams = (RegistryDisabledServiceItemParams*)EntryContext;
    if (pContextParams->bIsPresent)
        return ERROR_SUCCESS;

    DWORD dwType = 0, dwBufSize = 0;

    // Be careful, the order of the operations in the comparison is very important.
    if ( (wcsicmp((LPCWSTR)Context, SubKeyName) == 0) &&
         (RegQueryValueEx(hOpenedSubKey, /* ValueName == */ SubKeyName, NULL, &dwType, NULL, &dwBufSize) == ERROR_SUCCESS) &&
         (dwType == REG_DWORD) && (dwBufSize == sizeof(DWORD)) )
    {
#if 1 // DisableDate
        SYSTEMTIME disableDate = {};
        DWORD dwRegData = 0;

        dwRegData = 0;
        RegGetDWORDValue(hOpenedSubKey, NULL, L"DAY", &dwRegData);
        disableDate.wDay = LOWORD(dwRegData);

        dwRegData = 0;
        RegGetDWORDValue(hOpenedSubKey, NULL, L"HOUR", &dwRegData);
        disableDate.wHour = LOWORD(dwRegData);

        dwRegData = 0;
        RegGetDWORDValue(hOpenedSubKey, NULL, L"MINUTE", &dwRegData);
        disableDate.wMinute = LOWORD(dwRegData);

        dwRegData = 0;
        RegGetDWORDValue(hOpenedSubKey, NULL, L"MONTH", &dwRegData);
        disableDate.wMonth = LOWORD(dwRegData);

        dwRegData = 0;
        RegGetDWORDValue(hOpenedSubKey, NULL, L"SECOND", &dwRegData);
        disableDate.wSecond = LOWORD(dwRegData);

        dwRegData = 0;
        RegGetDWORDValue(hOpenedSubKey, NULL, L"YEAR", &dwRegData);
        disableDate.wYear = LOWORD(dwRegData);
#endif

        DWORD dwStartType = 0;
        RegGetDWORDValue(hOpenedSubKey, NULL, SubKeyName /* Service name */, &dwStartType);

        pContextParams->bIsPresent  = TRUE;
        pContextParams->bIsKeyed    = TRUE;
        pContextParams->dwStartType = dwStartType;
        pContextParams->time        = disableDate;
    }
    else
    {
        pContextParams->bIsPresent  = FALSE;
        pContextParams->bIsKeyed    = TRUE;
        pContextParams->dwStartType = 0;
        // pContextParams->time        = {};
    }

    return ERROR_SUCCESS;
}



static void AddService(SC_HANDLE hSCManager, LPENUM_SERVICE_STATUS_PROCESS Service, BOOL bHideOSVendorServices)
{
    //
    // Retrieve a handle to the service.
    //
    SC_HANDLE hService = OpenServiceW(hSCManager, Service->lpServiceName, SERVICE_QUERY_CONFIG);
    if (hService == NULL)
        return;

    DWORD dwBytesNeeded = 0;
    QueryServiceConfigW(hService, NULL, 0, &dwBytesNeeded);
    // if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)

    LPQUERY_SERVICE_CONFIG lpServiceConfig = (LPQUERY_SERVICE_CONFIG)MemAlloc(0, dwBytesNeeded);
    if (!lpServiceConfig)
    {
        CloseServiceHandle(hService);
        return;
    }
    QueryServiceConfigW(hService, lpServiceConfig, dwBytesNeeded, &dwBytesNeeded);

    //
    // Get the service's vendor...
    //
    LPWSTR lpszVendor = NULL;
    {
    // Isolate only the executable path, without any arguments.
    // TODO: Correct at the level of CmdLineToArgv the potential bug when lpszFilename == NULL.
#if 0 // Disabled until CmdLineToArgv is included
    unsigned int argc = 0;
    LPWSTR*      argv = NULL;
    CmdLineToArgv(lpServiceConfig->lpBinaryPathName, &argc, &argv, L" \t");
    if (argc >= 1 && argv[0])
        lpszVendor = GetExecutableVendor(argv[0]);
#else
    // Hackish solution taken from the original srvpage.c.
    // Will be removed after CmdLineToArgv is introduced.
    WCHAR FileName[MAX_PATH];
    memset(&FileName, 0, sizeof(FileName));
    if (wcscspn(lpServiceConfig->lpBinaryPathName, L"\""))
    {
        wcsncpy(FileName, lpServiceConfig->lpBinaryPathName, wcscspn(lpServiceConfig->lpBinaryPathName, L" ") );
    }
    else
    {
        wcscpy(FileName, lpServiceConfig->lpBinaryPathName);
    }
    lpszVendor = GetExecutableVendor(FileName);
#endif
    if (!lpszVendor)
        lpszVendor = LoadResourceString(hInst, IDS_UNKNOWN);
#if 0
    MemFree(argv);
#endif
    }

    // ...and display or not the Microsoft / ReactOS services.
    BOOL bContinue = TRUE;
    if (bHideOSVendorServices)
    {
        if (FindSubStrI(lpszVendor, bIsWindows ? IDS_MICROSOFT : IDS_REACTOS))
            bContinue = FALSE;
    }

    if (bContinue)
    {
        BOOL bIsServiceEnabled  = (lpServiceConfig->dwStartType != SERVICE_DISABLED);
        BOOL bAddServiceToList  = FALSE;
        BOOL bIsModifiedService = FALSE;
        RegistryDisabledServiceItemParams params = {};

        //
        // Try to look into the user modifications list...
        //
        POSITION it = userModificationsList.Find(Service->lpServiceName);
        if (it)
        {
            bAddServiceToList  = TRUE;
            bIsModifiedService = TRUE;
        }

        //
        // ...if not found, try to find if the disabled service is in the registry.
        //
        if (!bAddServiceToList)
        {
            if (!bIsServiceEnabled)
            {
                QUERY_REGISTRY_KEYS_TABLE KeysQueryTable[2] = {};
                KeysQueryTable[0].QueryRoutine = GetRegistryKeyedDisabledServicesQueryRoutine;
                KeysQueryTable[0].EntryContext = &params;
                RegQueryRegistryKeys(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Shared Tools\\MSConfig\\services", KeysQueryTable, Service->lpServiceName);

                bAddServiceToList = params.bIsPresent;

                if (bIsWindows && bIsOSVersionLessThanVista && !bAddServiceToList)
                {
                    QUERY_REGISTRY_VALUES_TABLE ValuesQueryTable[2] = {};
                    ValuesQueryTable[0].QueryRoutine = GetRegistryValuedDisabledServicesQueryRoutine;
                    ValuesQueryTable[0].EntryContext = &params;
                    RegQueryRegistryValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Shared Tools\\MSConfig\\services", ValuesQueryTable, Service->lpServiceName);

                    bAddServiceToList = params.bIsPresent;
                }
            }
            else
            {
                bAddServiceToList = TRUE;
            }
        }

        if (bAddServiceToList)
        {
            //
            // Check if service is required by the system.
            //
            BOOL bIsRequired = FALSE;

            dwBytesNeeded = 0;
            QueryServiceConfig2(hService, SERVICE_CONFIG_FAILURE_ACTIONS, NULL, 0, &dwBytesNeeded);
            // if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)

            LPSERVICE_FAILURE_ACTIONS lpServiceFailureActions = (LPSERVICE_FAILURE_ACTIONS)MemAlloc(0, dwBytesNeeded);
            if (!lpServiceFailureActions)
            {
                MemFree(lpszVendor);
                MemFree(lpServiceConfig);
                CloseServiceHandle(hService);
                return;
            }

            QueryServiceConfig2(hService, SERVICE_CONFIG_FAILURE_ACTIONS, (LPBYTE)lpServiceFailureActions, dwBytesNeeded, &dwBytesNeeded);

            // In Microsoft's MSConfig, things are done just like that!! (extracted string values from msconfig.exe)
            if ( ( wcsicmp(Service->lpServiceName, L"rpcss"     ) == 0   ||
                   wcsicmp(Service->lpServiceName, L"rpclocator") == 0   ||
                   wcsicmp(Service->lpServiceName, L"dcomlaunch") == 0 ) ||
                   ( lpServiceFailureActions &&
                     (lpServiceFailureActions->cActions >= 1) &&
                     (lpServiceFailureActions->lpsaActions[0].Type == SC_ACTION_REBOOT) ) ) // We add also this test, which corresponds to real life.
            {
                bIsRequired = TRUE;
            }
            MemFree(lpServiceFailureActions);

            //
            // Add the service into the list.
            //
            LVITEM item = {};
            item.mask = LVIF_TEXT | LVIF_PARAM;
            item.pszText = Service->lpDisplayName;
            item.lParam = reinterpret_cast<LPARAM>(new ServiceItem(Service->lpServiceName, bIsServiceEnabled, bIsRequired));
            item.iItem = ListView_InsertItem(hServicesListCtrl, &item);

            if (bIsRequired)
            {
                LPWSTR lpszYes = LoadResourceString(hInst, IDS_YES);
                ListView_SetItemText(hServicesListCtrl, item.iItem, 1, lpszYes);
                MemFree(lpszYes);
            }

            ListView_SetItemText(hServicesListCtrl, item.iItem, 2, lpszVendor);

            LPWSTR lpszStatus = LoadResourceString(hInst, ((Service->ServiceStatusProcess.dwCurrentState == SERVICE_STOPPED) ? IDS_SERVICES_STATUS_STOPPED : IDS_SERVICES_STATUS_RUNNING));
            ListView_SetItemText(hServicesListCtrl, item.iItem, 3, lpszStatus);
            MemFree(lpszStatus);

            if (!bIsServiceEnabled)
            {
                LPWSTR lpszUnknown = LoadResourceString(hInst, IDS_UNKNOWN);

                LPWSTR lpszDisableDate = FormatDateTime(&params.time);
                ListView_SetItemText(hServicesListCtrl, item.iItem, 4, (lpszDisableDate ? lpszDisableDate : lpszUnknown));
                FreeDateTime(lpszDisableDate);

                MemFree(lpszUnknown);
            }

            ListView_SetCheckState(hServicesListCtrl, item.iItem, (!bIsModifiedService ? bIsServiceEnabled : !bIsServiceEnabled));
        }
    }

    MemFree(lpszVendor);
    MemFree(lpServiceConfig);
    CloseServiceHandle(hService);

    return;
}

static void ClearServicesList(void)
{
    LVITEM lvitem = {};
    lvitem.mask  = LVIF_PARAM;
    lvitem.iItem = -1; // From the beginning.

    while ((lvitem.iItem = ListView_GetNextItem(hServicesListCtrl, lvitem.iItem, LVNI_ALL)) != -1)
    {
        ListView_GetItem(hServicesListCtrl, &lvitem);

        delete reinterpret_cast<ServiceItem*>(lvitem.lParam);
        lvitem.lParam = NULL;
    }
    ListView_DeleteAllItems(hServicesListCtrl);

    return;
}

static void GetServices(BOOL bHideOSVendorServices = FALSE)
{
    //
    // First of all, clear the list.
    //
    ClearServicesList();

    //
    // Now, we can list the services.
    //

    // Open the Service Control Manager.
    SC_HANDLE hSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
    if (hSCManager == NULL)
        return;

    // Enumerate all the Win32 services.
    DWORD dwBytesNeeded = 0;
    DWORD dwNumServices = 0;
    // DWORD dwResumeHandle = 0;
    EnumServicesStatusExW(hSCManager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL, NULL, 0, &dwBytesNeeded, &dwNumServices, NULL /* &dwResumeHandle */, NULL);
    // if (GetLastError() == ERROR_MORE_DATA)

    LPENUM_SERVICE_STATUS_PROCESS lpServices = (LPENUM_SERVICE_STATUS_PROCESS)MemAlloc(0, dwBytesNeeded);
    if (!lpServices)
    {
        CloseServiceHandle(hSCManager);
        return;
    }
    EnumServicesStatusExW(hSCManager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL, (LPBYTE)lpServices, dwBytesNeeded, &dwBytesNeeded, &dwNumServices, NULL /* &dwResumeHandle */, NULL);

    // Add them into the list.
    for (DWORD i = 0 ; i < dwNumServices ; ++i)
    {
        AddService(hSCManager, lpServices + i, bHideOSVendorServices);
    }

    // Cleaning.
    MemFree(lpServices);
    CloseServiceHandle(hSCManager);

    return;
}

INT_PTR CALLBACK
RequiredServicesDisablingDialogWndProc(HWND hDlg,
                                       UINT message,
                                       WPARAM wParam,
                                       LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
        case WM_INITDIALOG:
        {
            /* Correctly display message strings */
            LPCWSTR szOSVendor;
            size_t itemLength = 0;
            LPWSTR szItem = NULL, szNewItem = NULL;

            szOSVendor = (bIsWindows ? IDS_WINDOWS : IDS_REACTOS);

            itemLength = GetWindowTextLength(GetDlgItem(hDlg, IDC_STATIC_REQSVCSDIS_INFO)) + 1;
            szItem     = (LPWSTR)MemAlloc(0, itemLength * sizeof(WCHAR));
            GetDlgItemText(hDlg, IDC_STATIC_REQSVCSDIS_INFO, szItem, (int)itemLength);
            szNewItem  = FormatString(szItem, szOSVendor);
            SetDlgItemText(hDlg, IDC_STATIC_REQSVCSDIS_INFO, szNewItem);
            MemFree(szNewItem);
            MemFree(szItem);

            return TRUE;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    if (Button_GetCheck(GetDlgItem(hDlg, IDC_CBX_REQSVCSDIS_NO_MSG_ANYMORE)) == BST_CHECKED)
                        RegisterNoMsgAnymore();

                    EndDialog(hDlg, LOWORD(wParam));
                    return TRUE;
                }

                case IDCANCEL:
                    EndDialog(hDlg, LOWORD(wParam));
                    return TRUE;

                default:
                    //break;
                    return FALSE;
            }
        }
    }

    return FALSE;
}

static BOOL ValidateItem(int index, BOOL bNewState, BOOL bDisplayErrors)
{
    ServiceItem* pSvcItem = NULL;

    LVITEM truc = {};
    truc.mask = LVIF_PARAM;
    truc.iItem = index;
    ListView_GetItem(hServicesListCtrl, &truc);

    // The lParam member must be valid.
    pSvcItem = reinterpret_cast<ServiceItem*>(truc.lParam);
    if (!pSvcItem)
        return FALSE;

    //
    // Allow modifications only if the service is not a required service for the system,
    // or allow only the activation of a disabled required service.
    //
    BOOL bOldState = !!(ListView_GetCheckState(hServicesListCtrl, truc.iItem /* == index */) % 2);

    if ( !pSvcItem->m_bIsRequired ||
         (pSvcItem->m_bIsRequired && !pSvcItem->m_bIsEnabled && bOldState == FALSE && bNewState == TRUE) )
    {
        if (bOldState == bNewState)
            return FALSE;

        ListView_SetCheckState(hServicesListCtrl, index, bNewState);

        if (pSvcItem->m_bIsEnabled) // Enabled service.
        {
            if (bNewState == FALSE) // To be deactivated.
            {
                userModificationsList.AddTail(pSvcItem->m_lpszSvcName);
            }
            else if (bNewState == TRUE) // To be reactivated
            {
                POSITION it = userModificationsList.Find(pSvcItem->m_lpszSvcName);
                if (it)
                {
                    userModificationsList.RemoveAt(it);
                }
                else
                {
                    OutputDebugString(_T("(1) \"WTF: What The Fukhurmajalmahamadahaldeliya ?!\" (The Dictator, Sacha Baron Cohen)\n"));
                }
            }
        }
        else // Disabled service.
        {
            if (bNewState == TRUE) // To be activated.
            {
                userModificationsList.AddTail(pSvcItem->m_lpszSvcName);
            }
            else if (bNewState == FALSE) // To be redeactivated
            {
                POSITION it = userModificationsList.Find(pSvcItem->m_lpszSvcName);
                if (it)
                {
                    userModificationsList.RemoveAt(it);
                }
                else
                {
                    OutputDebugString(_T("(2) \"WTF: What The Fukhurmajalmahamadahaldeliya ?!\" (The Dictator, Sacha Baron Cohen)\n"));
                }
            }
        }

        return TRUE;
    }
    else
    {
        if (bDisplayErrors)
        {
            DialogBoxW(hInst, MAKEINTRESOURCEW(IDD_REQUIRED_SERVICES_DISABLING_DIALOG), hServicesPage /* hMainWnd */, RequiredServicesDisablingDialogWndProc);
        }

        return FALSE;
    }
}


static void
Update_Btn_States(HWND hDlg)
{
    // HWND hTree = GetDlgItem(hDlg, IDC_SYSTEM_TREE);

    //
    // "Enable all" / "Disable all" buttons.
    //
    // UINT uRootCheckState = TreeView_GetRealSubtreeState(hTree, TVI_ROOT);
    UINT uRootCheckState = ListView_GetCheckState(hServicesListCtrl, 0);
#define OP(a, b) ((a) == (b) ? (a) : 2)
    int index = 0; // -1 // From the beginning + 1.
    while ((index = ListView_GetNextItem(hServicesListCtrl, index, LVNI_ALL)) != -1)
    {
        UINT temp = ListView_GetCheckState(hServicesListCtrl, index);
        uRootCheckState = OP(uRootCheckState, temp);
    }

    if (uRootCheckState == 0)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SERVICES_ACTIVATE)  , TRUE );
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SERVICES_DEACTIVATE), FALSE);
    }
    else if (uRootCheckState == 1)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SERVICES_ACTIVATE)  , FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SERVICES_DEACTIVATE), TRUE );
    }
    else if (uRootCheckState == 2)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SERVICES_ACTIVATE)  , TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SERVICES_DEACTIVATE), TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SERVICES_ACTIVATE)  , FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SERVICES_DEACTIVATE), FALSE);
    }

    return;
}

extern "C" {

INT_PTR CALLBACK
ServicesPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);

    switch (message)
    {
        case WM_INITDIALOG:
        {
            hServicesPage     = hDlg;
            hServicesListCtrl = GetDlgItem(hServicesPage, IDC_SERVICES_LIST);

            //
            // Correctly display message strings.
            //
            LPCWSTR szOSVendor = (bIsWindows ? IDS_MICROSOFT : IDS_REACTOS);

            size_t itemLength = 0;
            LPWSTR szItem = NULL, szNewItem = NULL;

            itemLength = GetWindowTextLength(GetDlgItem(hServicesPage, IDC_STATIC_SERVICES_WARNING)) + 1;
            szItem     = (LPWSTR)MemAlloc(0, itemLength * sizeof(WCHAR));
            GetDlgItemText(hServicesPage, IDC_STATIC_SERVICES_WARNING, szItem, (int)itemLength);
            szNewItem  = FormatString(szItem, szOSVendor);
            SetDlgItemText(hServicesPage, IDC_STATIC_SERVICES_WARNING, szNewItem);
            MemFree(szNewItem);
            MemFree(szItem);

            itemLength = GetWindowTextLength(GetDlgItem(hServicesPage, IDC_CBX_SERVICES_MASK_PROPRIETARY_SVCS)) + 1;
            szItem     = (LPWSTR)MemAlloc(0, itemLength * sizeof(WCHAR));
            GetDlgItemText(hServicesPage, IDC_CBX_SERVICES_MASK_PROPRIETARY_SVCS, szItem, (int)itemLength);
            szNewItem  = FormatString(szItem, szOSVendor);
            SetDlgItemText(hServicesPage, IDC_CBX_SERVICES_MASK_PROPRIETARY_SVCS, szNewItem);
            MemFree(szNewItem);
            MemFree(szItem);

            //
            // Initialize the styles.
            //
            DWORD dwStyle = ListView_GetExtendedListViewStyle(hServicesListCtrl);
            ListView_SetExtendedListViewStyle(hServicesListCtrl, dwStyle | LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);
            SetWindowTheme(hServicesListCtrl, L"Explorer", NULL);

            //
            // Initialize the application page's controls.
            //
            LVCOLUMN column = {};

            // First column : Service's name.
            column.mask = LVCF_TEXT | LVCF_WIDTH;
            column.pszText = LoadResourceString(hInst, IDS_SERVICES_COLUMN_SERVICE);
            column.cx = 150;
            ListView_InsertColumn(hServicesListCtrl, 0, &column);
            MemFree(column.pszText);

            // Second column : Whether the service is required or not.
            column.mask = LVCF_TEXT | LVCF_WIDTH;
            column.pszText = LoadResourceString(hInst, IDS_SERVICES_COLUMN_REQ);
            column.cx = 60;
            ListView_InsertColumn(hServicesListCtrl, 1, &column);
            MemFree(column.pszText);

            // Third column : Service's vendor.
            column.mask = LVCF_TEXT | LVCF_WIDTH;
            column.pszText = LoadResourceString(hInst, IDS_SERVICES_COLUMN_VENDOR);
            column.cx = 150;
            ListView_InsertColumn(hServicesListCtrl, 2, &column);
            MemFree(column.pszText);

            // Fourth column : Service's status.
            column.mask = LVCF_TEXT | LVCF_WIDTH;
            column.pszText = LoadResourceString(hInst, IDS_SERVICES_COLUMN_STATUS);
            column.cx = 60;
            ListView_InsertColumn(hServicesListCtrl, 3, &column);
            MemFree(column.pszText);

            // Fifth column : Service's disabled date.
            column.mask = LVCF_TEXT | LVCF_WIDTH;
            column.pszText = LoadResourceString(hInst, IDS_SERVICES_COLUMN_DATEDISABLED);
            column.cx = 120;
            ListView_InsertColumn(hServicesListCtrl, 4, &column);
            MemFree(column.pszText);

            //
            // Populate and sort the list.
            //
            GetServices();
            ListView_Sort(hServicesListCtrl, 0);
            Update_Btn_States(hDlg);

            // Select the first item.
            ListView_SetItemState(hServicesListCtrl, 0, LVIS_SELECTED, LVIS_SELECTED);

            return TRUE;
        }

        case WM_DESTROY:
        {
            ClearServicesList();
            userModificationsList.RemoveAll();
            return 0;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_BTN_SERVICES_ACTIVATE:
                {
                    BOOL bAreThereModifs = FALSE;

                    int index = -1; // From the beginning.
                    while ((index = ListView_GetNextItem(hServicesListCtrl, index, LVNI_ALL)) != -1)
                    {
                        bAreThereModifs = ValidateItem(index, TRUE, FALSE) || bAreThereModifs; // The order is verrrrrry important !!!!
                    }

                    if (bAreThereModifs)
                    {
                        Update_Btn_States(hDlg);
                        PropSheet_Changed(GetParent(hServicesPage), hServicesPage);
                    }

                    return TRUE;
                }

                case IDC_BTN_SERVICES_DEACTIVATE:
                {
                    BOOL bAreThereModifs = FALSE;

                    int index = -1; // From the beginning.
                    while ((index = ListView_GetNextItem(hServicesListCtrl, index, LVNI_ALL)) != -1)
                    {
                        bAreThereModifs = ValidateItem(index, FALSE, FALSE) || bAreThereModifs; // The order is verrrrrry important !!!!
                    }

                    if (bAreThereModifs)
                    {
                        Update_Btn_States(hDlg);
                        PropSheet_Changed(GetParent(hServicesPage), hServicesPage);
                    }

                    return TRUE;
                }

                case IDC_CBX_SERVICES_MASK_PROPRIETARY_SVCS:
                {
                    bMaskProprietarySvcs = !bMaskProprietarySvcs;
                    GetServices(bMaskProprietarySvcs);
                    Update_Btn_States(hDlg);

                    return TRUE;
                }

                default:
                    return FALSE;
            }
            return FALSE;
        }

        case UM_CHECKSTATECHANGE:
        {
            BOOL bNewCheckState = !!((ListView_GetCheckState(hServicesListCtrl, int(lParam)) + 1) % 2);

            if (ValidateItem(/*reinterpret_cast<int>*/ int(lParam), bNewCheckState, !HideEssentialServiceWarning()))
            {
                Update_Btn_States(hDlg);
                PropSheet_Changed(GetParent(hServicesPage), hServicesPage);
            }

            return TRUE;
        }

        case WM_NOTIFY:
        {
            if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == hServicesListCtrl)
            {
                switch (reinterpret_cast<LPNMHDR>(lParam)->code)
                {
                    case NM_CLICK:
                    case NM_RCLICK:
                    {
                        DWORD         dwpos = GetMessagePos();
                        LVHITTESTINFO ht    = {};
                        ht.pt.x = GET_X_LPARAM(dwpos);
                        ht.pt.y = GET_Y_LPARAM(dwpos);
                        MapWindowPoints(HWND_DESKTOP /*NULL*/, hServicesListCtrl, &ht.pt, 1);

                        /*
                         * We use ListView_SubItemHitTest(...) and not ListView_HitTest(...)
                         * because ListView_HitTest(...) returns bad flags when one clicks
                         * on a sub-item different from 0. The flags then contain LVHT_ONITEMSTATEICON
                         * which must not be obviously present in this case.
                         */
                        ListView_SubItemHitTest(hServicesListCtrl, &ht);

                        if (LVHT_ONITEMSTATEICON & ht.flags)
                        {
                            PostMessage(hDlg, UM_CHECKSTATECHANGE, 0, (LPARAM)ht.iItem);

                            // Disable default behaviour. Needed for the UM_CHECKSTATECHANGE
                            // custom notification to work as expected.
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
                        }

                        return TRUE;
                    }

                    case NM_DBLCLK:
                    case NM_RDBLCLK:
                    {
                        // We deactivate double-clicks.
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
                        return TRUE;
                    }

                    case LVN_KEYDOWN:
                    {
                        if (reinterpret_cast<LPNMLVKEYDOWN>(lParam)->wVKey == VK_SPACE)
                        {
                            int iItem = ListView_GetSelectionMark(hServicesListCtrl);
                            PostMessage(hDlg, UM_CHECKSTATECHANGE, 0, (LPARAM)iItem);

                            // Disable default behaviour. Needed for the UM_CHECKSTATECHANGE
                            // custom notification to work as expected.
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
                        }

                        return TRUE;
                    }

                    case LVN_COLUMNCLICK:
                    {
                        int iSortingColumn = reinterpret_cast<LPNMLISTVIEW>(lParam)->iSubItem;

                        ListView_SortEx(hServicesListCtrl, iSortingColumn, iSortedColumn);
                        iSortedColumn = iSortingColumn;

                        return TRUE;
                    }
                }
            }
            else
            {
                switch (reinterpret_cast<LPNMHDR>(lParam)->code)
                {
                    case PSN_APPLY:
                    {
                        // Try to apply the modifications to the system.
                        MessageBox(NULL, _T("In Services page: PSN_APPLY"), _T("Info"), MB_ICONINFORMATION);

                        /*
                        //
                        // Move this away...
                        //
                        int iRetVal = MessageBox(NULL, _T("Would you really want to modify the configuration of your system ?"), _T("Warning"), MB_ICONWARNING | MB_YESNOCANCEL);

                        if (iRetVal == IDYES /\* modifications are OK *\/)
                            SetWindowLongPtr(hServicesPage, DWLP_MSGRESULT, PSNRET_NOERROR);
                        else if (iRetVal == IDNO /\* modifications are not OK *\/)
                            SetWindowLongPtr(hServicesPage, DWLP_MSGRESULT, PSNRET_NOERROR);
                        else // if (iRetVal == IDCANCEL) // There was an error...
                            SetWindowLongPtr(hServicesPage, DWLP_MSGRESULT, PSNRET_INVALID);
                        */

                        //
                        // We modify the services which are stored in the user modification list.
                        //

                        // 1- Open the Service Control Manager for modifications.
                        SC_HANDLE hSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
                        if (hSCManager != NULL)
                        {
                            LPCWSTR svcName;

                            for (POSITION it = userModificationsList.GetHeadPosition(); it; userModificationsList.GetNext(it))
                            {
                                svcName = userModificationsList.GetAt(it);

                                // 2- Retrieve a handle to the service.
                                SC_HANDLE hService = OpenServiceW(hSCManager, svcName, SERVICE_QUERY_CONFIG | SERVICE_CHANGE_CONFIG);
                                if (hService == NULL)
                                {
                                    // TODO : Show a message box.
                                    continue;
                                }

                                DWORD dwBytesNeeded = 0;
                                QueryServiceConfigW(hService, NULL, 0, &dwBytesNeeded);
                                // if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)

                                LPQUERY_SERVICE_CONFIG lpServiceConfig = (LPQUERY_SERVICE_CONFIG)MemAlloc(0, dwBytesNeeded);
                                if (!lpServiceConfig)
                                {
                                    CloseServiceHandle(hService);
                                    continue; // TODO ? Show a message box...
                                }
                                QueryServiceConfigW(hService, lpServiceConfig, dwBytesNeeded, &dwBytesNeeded);

                                if (lpServiceConfig->dwStartType == SERVICE_DISABLED) // We have a disabled service which is becoming to be enabled.
                                {
                                    // 3a- Retrive the properties of the disabled service from the registry.
                                    RegistryDisabledServiceItemParams params = {};

                                    QUERY_REGISTRY_KEYS_TABLE KeysQueryTable[2] = {};
                                    KeysQueryTable[0].QueryRoutine = GetRegistryKeyedDisabledServicesQueryRoutine;
                                    KeysQueryTable[0].EntryContext = &params;
                                    RegQueryRegistryKeys(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Shared Tools\\MSConfig\\services", KeysQueryTable, (PVOID)svcName);

                                    if (bIsWindows && bIsOSVersionLessThanVista && !params.bIsPresent)
                                    {
                                        QUERY_REGISTRY_VALUES_TABLE ValuesQueryTable[2] = {};
                                        ValuesQueryTable[0].QueryRoutine = GetRegistryValuedDisabledServicesQueryRoutine;
                                        ValuesQueryTable[0].EntryContext = &params;
                                        RegQueryRegistryValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Shared Tools\\MSConfig\\services", ValuesQueryTable, (PVOID)svcName);
                                    }

                                    if (params.bIsPresent)
                                    {
                                        // 4a- Modify the service.
                                        ChangeServiceConfigW(hService, SERVICE_NO_CHANGE, params.dwStartType, SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

                                        // 5a- Remove the registry entry of the service.
                                        if (params.bIsKeyed)
                                        {
                                            CAtlStringW serviceRegKey(L"SOFTWARE\\Microsoft\\Shared Tools\\MSConfig\\services\\");
                                            serviceRegKey += svcName;
                                            RegDeleteKeyW(HKEY_LOCAL_MACHINE, serviceRegKey);

                                            /***** HACK for Windows < Vista (e.g. 2000, Xp, 2003...) *****/
                                            //
                                            // Delete also the valued-entry of the service.
                                            //
                                            if (bIsWindows && bIsOSVersionLessThanVista)
                                            {
                                                HKEY hSubKey = NULL;
                                                if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Shared Tools\\MSConfig\\services", 0, KEY_SET_VALUE /*KEY_READ*/, &hSubKey) == ERROR_SUCCESS)
                                                {
                                                    RegDeleteValue(hSubKey, svcName);
                                                    RegCloseKey(hSubKey);
                                                }
                                            }
                                            /*************************************************************/
                                        }
                                        else
                                        {
                                            HKEY hSubKey = NULL;
                                            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Shared Tools\\MSConfig\\services", 0, KEY_SET_VALUE /*KEY_READ*/, &hSubKey) == ERROR_SUCCESS)
                                            {
                                                RegDeleteValue(hSubKey, svcName);
                                                RegCloseKey(hSubKey);
                                            }
                                        }

                                        ////////// HACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACK ///////////
                                        // userModificationsList.RemoveAt(it);
                                    }
                                    else
                                    {
                                        // Ohoh !! We have a very big problem.
                                        MessageBox(NULL, _T("WTF ??"), _T("FATAL ERROR !!!!"), MB_ICONERROR);
                                    }
                                }
                                else // We have an enabled service which is becoming to be disabled.
                                {
                                    // 3b- Retrieve the local time of disabling.
                                    SYSTEMTIME disableDate = {};
                                    GetLocalTime(&disableDate);

                                    // 4b- Modify the service.
                                    ChangeServiceConfigW(hService, SERVICE_NO_CHANGE, SERVICE_DISABLED, SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

                                    // 5b- Register the service into the registry.
                                    CAtlStringW serviceRegKey(L"SOFTWARE\\Microsoft\\Shared Tools\\MSConfig\\services\\");
                                    serviceRegKey += svcName;
                                    HKEY hSubKey = NULL;
                                    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, serviceRegKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hSubKey, NULL) == ERROR_SUCCESS)
                                    {
                                        RegSetDWORDValue(hSubKey, NULL, svcName, FALSE, lpServiceConfig->dwStartType);

                                    #if 1 // DisableDate
                                        RegSetDWORDValue(hSubKey, NULL, L"DAY"   , FALSE, disableDate.wDay   );
                                        RegSetDWORDValue(hSubKey, NULL, L"HOUR"  , FALSE, disableDate.wHour  );
                                        RegSetDWORDValue(hSubKey, NULL, L"MINUTE", FALSE, disableDate.wMinute);
                                        RegSetDWORDValue(hSubKey, NULL, L"MONTH" , FALSE, disableDate.wMonth );
                                        RegSetDWORDValue(hSubKey, NULL, L"SECOND", FALSE, disableDate.wSecond);
                                        RegSetDWORDValue(hSubKey, NULL, L"YEAR"  , FALSE, disableDate.wYear  );
                                    #endif

                                        RegCloseKey(hSubKey);
                                    }

                                    /***** HACK for Windows < Vista (e.g. 2000, Xp, 2003...) *****/
                                    //
                                    // Save also a valued-entry for the service.
                                    //
                                    if (bIsWindows && bIsOSVersionLessThanVista)
                                    {
                                        RegSetDWORDValue(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Shared Tools\\MSConfig\\services", svcName, TRUE, lpServiceConfig->dwStartType);
                                    }
                                    /*************************************************************/

                                    ////////// HACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACK ///////////
                                    // userModificationsList.RemoveAt(it);
                                }

                                MemFree(lpServiceConfig);
                                CloseServiceHandle(hService);
                            }

                            //////////// HACK HACK !!!! ////////////
                            userModificationsList.RemoveAll();
                            ////////////////////////////////////////

                            CloseServiceHandle(hSCManager);


                            //// PropSheet_UnChanged(GetParent(hServicesPage), hServicesPage); ////
                            PropSheet_CancelToClose(GetParent(hDlg));

                            /* Modifications are OK */
                            SetWindowLongPtr(hServicesPage, DWLP_MSGRESULT, PSNRET_NOERROR);
                        }
                        else
                        {
                            MessageBox(hDlg, _T("Impossible to open the SC manager..."), _T("Error"), MB_ICONERROR);

                            // There was an error...
                            SetWindowLongPtr(hServicesPage, DWLP_MSGRESULT, PSNRET_INVALID);
                        }

                        GetServices(bMaskProprietarySvcs);
                        Update_Btn_States(hDlg);

                        return TRUE;
                    }

                    case PSN_HELP:
                    {
                        MessageBox(hServicesPage, _T("Help not implemented yet!"), _T("Help"), MB_ICONINFORMATION | MB_OK);
                        return TRUE;
                    }

                    case PSN_KILLACTIVE: // Is going to lose activation.
                    {
                        // Changes are always valid of course.
                        SetWindowLongPtr(hServicesPage, DWLP_MSGRESULT, FALSE);
                        return TRUE;
                    }

                    case PSN_QUERYCANCEL:
                    {
                        // RefreshStartupList();

                        // Allows cancellation.
                        SetWindowLongPtr(hServicesPage, DWLP_MSGRESULT, FALSE);

                        return TRUE;
                    }

                    case PSN_QUERYINITIALFOCUS:
                    {
                        // Give the focus on and select the first item.
                        ListView_SetItemState(hServicesListCtrl, 0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);

                        SetWindowLongPtr(hServicesPage, DWLP_MSGRESULT, (LONG_PTR)hServicesListCtrl);
                        return TRUE;
                    }

                    //
                    // DO NOT TOUCH THESE NEXT MESSAGES, THEY ARE OK LIKE THIS...
                    //
                    case PSN_RESET: // Perform final cleaning, called before WM_DESTROY.
                        return TRUE;

                    case PSN_SETACTIVE: // Is going to gain activation.
                    {
                        SetWindowLongPtr(hServicesPage, DWLP_MSGRESULT, 0);
                        return TRUE;
                    }

                    default:
                        break;
                }
            }
        }

        default:
            return FALSE;
    }

    return FALSE;
}

}
