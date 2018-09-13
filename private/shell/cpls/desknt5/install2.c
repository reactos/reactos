/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    install2.c

Abstract:

    This file implements the display class installer.

Environment:

    WIN32 User Mode

--*/


#include "precomp.h" 
#pragma hdrstop

#include <tchar.h>
#include <initguid.h>
#include <devguid.h>

#include "migrate.h"

// temporary extern defition from newdev.c

typedef
BOOL
(*PINSTALLNEWDEVICE)(
   HWND hwndParent,
   LPGUID ClassGuid,
   PDWORD Reboot
   );

#define INSETUP         1
#define INSETUP_UPGRADE 2

DWORD bSetupFlags(VOID);

ULONG PreConfigured;
ULONG KeepEnabled;

BOOL
CALLBACK
AddPropSheetPageProc(
    IN HPROPSHEETPAGE hpage,
    IN LPARAM lParam
   )
{
    *((HPROPSHEETPAGE *)lParam) = hpage;
    return TRUE;
}

BOOL
CDECL
DeskLogError(
    LogSeverity Severity,
    UINT MsgId,
    ...
    ) 
/*++

Outputs a message to the setup log.  Prepends "desk.cpl  " to the strings and 
appends the correct newline chars (\r\n)

  --*/
{
    int cch;
    TCHAR ach[1024+40];    // Largest path plus extra
    TCHAR szMsg[1024];     // MsgId
    va_list vArgs;

    static int setupState = 0;

    if (setupState == 0) {
        if (bSetupFlags() & (INSETUP | INSETUP_UPGRADE)) {
            setupState = 1;
        } else {
            setupState = 2;
        }
    }

    if (setupState == 1) {
        
        *szMsg = 0;
        if (LoadString(hInstance,
                       MsgId,
                       szMsg,
                       sizeof(szMsg) / sizeof(TCHAR))) {

            *ach = 0;
            LoadString(hInstance,
                       IDS_SETUPLOG_MSG_000,
                       ach,
                       sizeof(ach) / sizeof(TCHAR));
                       
            cch = lstrlen(ach);
            va_start(vArgs, MsgId);
            wvsprintf(&ach[cch], szMsg, vArgs);
            lstrcat(ach, TEXT("\r\n"));
            va_end(vArgs);
    
            return SetupLogError(ach, Severity);
        } else {
            return FALSE;
        }
    }
    else {
        va_start(vArgs, MsgId);
        va_end(vArgs);
        return TRUE;
    }
}

BOOLEAN
DeskIsLegacyDeviceNode(
    const PTCHAR szRegPath
    );

BOOLEAN
DeskIsLegacyDeviceNode2(
    PSP_DEVINFO_DATA pDid
    );
 
BOOLEAN
DeskGetDeviceNodePath(
    IN PSP_DEVINFO_DATA pDid,
    IN OUT PTCHAR szPath,
    IN LONG len
    );

VOID
DeskDeleteLegacyAppletExtension(
    VOID 
    );

BOOL
DeskRegDeleteKeyAndSubkeys(
    HKEY hKey,
    LPCTSTR lpSubKey
    );

#define LEGACY_DETECT 1
#define REPORT_DEVICE 2

#define ByteCountOf(x)  ((x) * sizeof(TCHAR))

/***************************************************************************/
/***************************************************************************/
/**                                                                       **/
/**                      Service Controller stuff                         **/
/**                                                                       **/
/***************************************************************************/
/***************************************************************************/

VOID
DeskSCSetServiceStartType(
    LPTSTR ServiceName,
    DWORD  dwStartType
    )
{
    SC_HANDLE SCMHandle;
    SC_HANDLE ServiceHandle;
    ULONG     Attempts;
    SC_LOCK   SCLock = NULL;

    ULONG                  ServiceConfigSize = 0;
    LPQUERY_SERVICE_CONFIG ServiceConfig;

    TraceMsg(TF_GENERAL, "DeskSCSetServiceStartType for service %ws called with type %d",
             ServiceName, dwStartType);

    //
    // Pretty stright-forward :
    //
    // Open the service controller
    // Open the service
    // Change the service.
    //

    if (SCMHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS))
    {
        if (ServiceHandle = OpenService(SCMHandle, ServiceName, SERVICE_ALL_ACCESS))
        {
            TraceMsg(TF_GENERAL, "DeskSCSetServiceStartType SC Managr handles opened\n");

            QueryServiceConfig(ServiceHandle,
                               NULL,
                               0,
                               &ServiceConfigSize);

            ASSERT(GetLastError() == ERROR_INSUFFICIENT_BUFFER);

            TraceMsg(TF_GENERAL, "DeskSCSetServiceStartType buffer size = %d\n", ServiceConfigSize);


            if (ServiceConfig = (LPQUERY_SERVICE_CONFIG)
                                 LocalAlloc(LPTR, ServiceConfigSize))
            {
                if (QueryServiceConfig(ServiceHandle,
                                       ServiceConfig,
                                       ServiceConfigSize,
                                       &ServiceConfigSize))
                {
                    TraceMsg(TF_GENERAL, "DeskSCSetServiceStartType Queried Config info\n");

                    //
                    // The service exists.  Check that it's not disabled.
                    //

                    if (ServiceConfig->dwStartType == SERVICE_DISABLED) {
                        ASSERT(FALSE);
                    }

                    //
                    // Attempt to acquite the database lock.
                    //

                    for (Attempts = 20;
                         ((SCLock = LockServiceDatabase(SCMHandle)) == NULL) &&
                             Attempts;
                         Attempts--)
                    {
                        TraceMsg(TF_GENERAL, "Install - Lock SC database locked\n");
                        Sleep(500);
                    }

                    //
                    // Change the service to demand start
                    //

                    if (ChangeServiceConfig(ServiceHandle,
                                            SERVICE_NO_CHANGE,
                                            dwStartType,
                                            SERVICE_NO_CHANGE,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL))
                    {
                        TraceMsg(TF_GENERAL, "DeskSCSetServiceStartType SC manager succeeded\n");
                    }
                    else
                    {
                        TraceMsg(TF_GENERAL, "DeskSCSetServiceStartType SC manager failed : %d\n",
                                        GetLastError());
                    }

                    if (SCLock)
                    {
                        TraceMsg(TF_GENERAL, "DeskSCSetServiceStartType Unlock database\n");
                        UnlockServiceDatabase(SCLock);
                    }
                }

                LocalFree(ServiceConfig);
            }

            CloseServiceHandle(ServiceHandle);
        }

        CloseServiceHandle(SCMHandle);
    }

    TraceMsg(TF_GENERAL, "DeskSCSetServiceStartType Leave\n");
}



DWORD
DeskSCStartService(
    LPTSTR ServiceName
    )
{
    SC_HANDLE SCMHandle;
    SC_HANDLE ServiceHandle;
    DWORD     dwRet = ERROR_SERVICE_NEVER_STARTED;

    TraceMsg(TF_GENERAL, "DeskSCStartService for service %ws called\n",
             ServiceName);

    //
    // Pretty stright-forward :
    //
    // Open the service controller
    // Open the service
    // Change the service.
    //

    if (SCMHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS))
    {
        if (ServiceHandle = OpenService(SCMHandle, ServiceName, SERVICE_ALL_ACCESS))
        {
            if (StartService(ServiceHandle, 0, NULL))
            {
                dwRet = NO_ERROR;
            }
            else
            {
                TraceMsg(TF_GENERAL, " StartService failed %d\n", GetLastError());
            }

            CloseServiceHandle(ServiceHandle);
        }

        CloseServiceHandle(SCMHandle);
    }

    TraceMsg(TF_GENERAL, "DeskSCStartService Leave with status %ws\n",
             dwRet ? L"ERROR_SERVICE_NEVER_STARTED" : L"NO_ERROR");

    return dwRet;
}




BOOL
DeskSCStopService(
    LPTSTR ServiceName
    )
{
    SC_HANDLE      SCMHandle;
    SC_HANDLE      ServiceHandle;
    BOOL           bRet = FALSE;
    SERVICE_STATUS ServiceStatus;

    TraceMsg(TF_GENERAL, "DeskSCStopService for service %ws called\n",
             ServiceName);

    //
    // Pretty stright-forward :
    //
    // Open the service controller
    // Open the service
    // Change the service.
    //

    if (SCMHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS))
    {
        if (ServiceHandle = OpenService(SCMHandle, ServiceName, SERVICE_ALL_ACCESS))
        {
            if (ControlService(ServiceHandle,
                               SERVICE_CONTROL_STOP,
                               &ServiceStatus))
            {
                bRet= TRUE;
            }
            else
            {
                TraceMsg(TF_GENERAL, " StopService failed %d\n", GetLastError());
            }

            CloseServiceHandle(ServiceHandle);
        }

        CloseServiceHandle(SCMHandle);
    }

    TraceMsg(TF_GENERAL, "DeskSCStopService Leave with status %ws\n",
             bRet ? L"TRUE" : L"FALSE");

    return bRet;
}




BOOL
DeskSCDeleteService(
    LPTSTR ServiceName
    )
{
    SC_HANDLE      SCMHandle;
    SC_HANDLE      ServiceHandle;
    BOOL           bRet = FALSE;

    TraceMsg(TF_GENERAL, "DeskSCDeleteService for service %ws called\n",
             ServiceName);

    //
    // Pretty stright-forward :
    //
    // Open the service controller
    // Open the service
    // Change the service.
    //

    if (SCMHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS))
    {
        if (ServiceHandle = OpenService(SCMHandle, ServiceName, SERVICE_ALL_ACCESS))
        {
            if (DeleteService(ServiceHandle))
            {
                bRet= TRUE;
            }
            else
            {
                TraceMsg(TF_GENERAL, "DeleteService failed %d\n", GetLastError());
            }

            CloseServiceHandle(ServiceHandle);
        }

        CloseServiceHandle(SCMHandle);
    }

    TraceMsg(TF_GENERAL, "DeskSCDeleteService Leave with status %ws\n",
             bRet ? L"TRUE" : L"FALSE");

    return bRet;
}




/***************************************************************************/
/***************************************************************************/
/**                                                                       **/
/**                      Service Installation                             **/
/**                                                                       **/
/***************************************************************************/
/***************************************************************************/


DWORD
DeskInstallServiceExtensions(
    IN HWND                    hwnd,
    IN HDEVINFO                hDevInfo,
    IN PSP_DEVINFO_DATA        pDeviceInfoData,
    IN PSP_DRVINFO_DATA        DriverInfoData,
    IN PSP_DRVINFO_DETAIL_DATA DriverInfoDetailData,
    IN LPTSTR                  pServiceName,
    IN DWORD                   dwDetect
    )
{
    DWORD retError = NO_ERROR;

    HINF       InfFileHandle;
    INFCONTEXT tmpContext;

    TCHAR szSoftwareSection[LINE_LEN];

    ULONG maxmem;
    ULONG numDev;
    SP_DEVINSTALL_PARAMS   DeviceInstallParams;

    TCHAR keyName[LINE_LEN];
    DWORD disposition;
    HKEY  hkey;
    BOOL  bSetSoftwareKey;

    TraceMsg(TF_GENERAL, "DeskInstallServiceExtensions called\n");

    //
    // open the inf so we can run the sections in the inf, more or less
    // manually.
    //

    InfFileHandle = SetupOpenInfFile(DriverInfoDetailData->InfFileName,
                                     NULL,
                                     INF_STYLE_WIN4,
                                     NULL);

    if (InfFileHandle == INVALID_HANDLE_VALUE)
    {
        TraceMsg(TF_GENERAL, "SetupOpenInfFile Error %d\n", INVALID_HANDLE_VALUE);
        return ERROR_INVALID_PARAMETER;
    }


    //
    // Get any interesting configuration data for the inf file.
    //

    maxmem = 8;
    numDev = 1;
    PreConfigured = 0;
    KeepEnabled = 0;

    wsprintf(szSoftwareSection,
             TEXT("%ws.GeneralConfigData"),
             DriverInfoDetailData->SectionName);

    if (SetupFindFirstLine(InfFileHandle,
                           szSoftwareSection,
                           TEXT("MaximumNumberOfDevices"),
                           &tmpContext))
    {
        SetupGetIntField(&tmpContext,
                         1,
                         &numDev);
    }

    if (SetupFindFirstLine(InfFileHandle,
                           szSoftwareSection,
                           TEXT("MaximumDeviceMemoryConfiguration"),
                           &tmpContext))
    {
        SetupGetIntField(&tmpContext,
                         1,
                         &maxmem);
    }

    if (SetupFindFirstLine(InfFileHandle,
                           szSoftwareSection,
                           TEXT("PreConfiguredSettings"),
                           &tmpContext))
    {
        SetupGetIntField(&tmpContext,
                         1,
                         &PreConfigured);
    }

    if (SetupFindFirstLine(InfFileHandle,
                           szSoftwareSection,
                           TEXT("KeepExistingDriverEnabled"),
                           &tmpContext))
    {
        SetupGetIntField(&tmpContext,
                         1,
                         &KeepEnabled);
    }

    //
    // Write the Detect configuration information to the registry.
    //

    wsprintf(keyName,
             TEXT("System\\CurrentControlSet\\Services\\%ws"),
             pServiceName, numDev);

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                       keyName,
                       0,
                       NULL,
                       REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE,
                       NULL,
                       &hkey,
                       &disposition) == ERROR_SUCCESS)
    {
        ULONG Value = 1;

        if (dwDetect == LEGACY_DETECT)
        {
            RegSetValueEx(hkey,
                          TEXT("LegacyDetect"),
                          0,
                          REG_DWORD,
                          (LPBYTE) (&Value),
                          SIZEOF(ULONG) );
        }
        else if (dwDetect == REPORT_DEVICE)
        {
            RegSetValueEx(hkey,
                          TEXT("ReportDevice"),
                          0,
                          REG_DWORD,
                          (LPBYTE) (&Value),
                          SIZEOF(ULONG) );
        }

        RegCloseKey(hkey);
    }

    //
    // Increase the number of system PTEs if we have cards that will need
    // more than 10 MEG of PTEs
    //

    if ((maxmem = maxmem * numDev) > 10)
    {
        //
        // we need 1K PTEs to support 1 MEG
        // Then add 50% for other devices this type of machine may have.
        //
        // NOTE - in the future, we may want to be smarter and try
        // to merge with whatever someone else put in there.
        //

        maxmem = maxmem * 0x400 * 3/2 + 0x3000;

        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                           TEXT("System\\CurrentControlSet\\Control\\Session Manager\\Memory Management"),
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_READ | KEY_WRITE,
                           NULL,
                           &hkey,
                           &disposition) == ERROR_SUCCESS)
        {
            //
            // Check if we walready set maxmem in the registry.
            //

            DWORD data;
            DWORD cb = sizeof(data);

            if ( (RegQueryValueEx(hkey,
                                  TEXT("SystemPages"),
                                  NULL,
                                  NULL,
                                  (LPBYTE)(&data),
                                  &cb) != ERROR_SUCCESS) ||
                 (data < maxmem) )
            {
                //
                // Set the new value
                //

                RegSetValueEx(hkey,
                              TEXT("SystemPages"),
                              0,
                              REG_DWORD,
                              (LPBYTE) &maxmem,
                              sizeof(DWORD));

                //
                // Tell the system we must reboot before running this driver.
                //

                ZeroMemory(&DeviceInstallParams, sizeof(DeviceInstallParams));
                DeviceInstallParams.cbSize = sizeof(DeviceInstallParams);

                SetupDiGetDeviceInstallParams(hDevInfo,
                                              pDeviceInfoData,
                                              &DeviceInstallParams);

                DeviceInstallParams.Flags |= DI_NEEDREBOOT;

                SetupDiSetDeviceInstallParams(hDevInfo,
                                              pDeviceInfoData,
                                              &DeviceInstallParams);
            }

            RegCloseKey(hkey);
        }

    }

    //
    // We may have to do this for multiple adapters at this point.
    // So loop throught the number of devices, which has 1 as the default
    // value
    //

    bSetSoftwareKey = TRUE;

    do {

        if (bSetSoftwareKey && !dwDetect)
        {
            HKEY hkeyTmp;

            hkey = INVALID_HANDLE_VALUE;

            //
            // For PnP, install the display driver under the software key
            //

            hkeyTmp = SetupDiOpenDevRegKey(hDevInfo,
                                           pDeviceInfoData,
                                           DICS_FLAG_GLOBAL,
                                           0,
                                           DIREG_DRV,
                                           KEY_READ | KEY_WRITE);

            if (hkeyTmp != INVALID_HANDLE_VALUE)
            {
                RegCreateKeyEx(hkeyTmp,
                               TEXT("Settings"),
                               0,
                               NULL,
                               REG_OPTION_NON_VOLATILE,
                               KEY_READ | KEY_WRITE,
                               NULL,
                               &hkey,
                               &disposition);

                RegCloseKey(hkeyTmp);
            }
        }
        else
        {
            numDev -= 1;

            //
            // For all drivers, install the information under DeviceX
            // We do this for legacy purposes since many drivers rely on
            // information written to this key.
            //

            wsprintf(keyName,
                     TEXT("System\\CurrentControlSet\\Services\\%ws\\Device%d"),
                     pServiceName, numDev);

            RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                           keyName,
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_READ | KEY_WRITE,
                           NULL,
                           &hkey,
                           &disposition);
        }

        if (hkey != INVALID_HANDLE_VALUE)
        {
            wsprintf(szSoftwareSection,
                     TEXT("%ws.SoftwareSettings"),
                     DriverInfoDetailData->SectionName);

            if (!SetupInstallFromInfSection(hwnd,
                                            InfFileHandle,
                                            szSoftwareSection,
                                            SPINST_REGISTRY,
                                            hkey,
                                            NULL,
                                            0,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL))
            {
                TraceMsg(TF_GENERAL, "SetupInstallFromInfSection Error %d\n", INVALID_HANDLE_VALUE);
                return ERROR_INVALID_PARAMETER;
            }

            //
            // Write the description of the device if we are not detecting
            //
            // Only do this for legacy installation at this point.
            //

            if (bSetSoftwareKey == FALSE)
            {
                RegSetValueEx(hkey,
                              TEXT("Device Description"),
                              0,
                              REG_SZ,
                              (LPBYTE) DriverInfoDetailData->DrvDescription,
                              ByteCountOf(lstrlen(DriverInfoDetailData->DrvDescription) + 1));
            }
#if 0
            //
            // Invoke the resource picker for adapters that need it.
            //

            {
                LOG_CONF LogConfig;
                RES_DES ResDes;
                HMODULE hModule;
                FARPROC PropSheetExtProc;

                if (CM_Get_First_Log_Conf(&LogConfig,
                                          pDeviceInfoData->DevInst,
                                          BASIC_LOG_CONF) == CR_SUCCESS)
                {
                    //
                    // This device instance has a basic log config, so we want to
                    // give the user a resource selection dialog.
                    // (before we do that, go ahead and free the log config
                    // handle--we don't need it.)
                    //

                    CM_Free_Log_Conf_Handle(LogConfig);

                    if ((hModule = GetModuleHandle(TEXT("setupapi.dll"))) &&
                        (PropSheetExtProc = GetProcAddress(hModule,
                                                           "ExtensionPropSheetPageProc")))
                    {
                        SP_PROPSHEETPAGE_REQUEST PropSheetRequest;
                        HPROPSHEETPAGE    hPage = {0};
                        PROPSHEETPAGE     PropPage;
                        PROPSHEETHEADER   PropHeader;

                        PropSheetRequest.cbSize = sizeof(SP_PROPSHEETPAGE_REQUEST);
                        PropSheetRequest.PageRequested = SPPSR_SELECT_DEVICE_RESOURCES;
                        PropSheetRequest.DeviceInfoSet = hDevInfo;
                        PropSheetRequest.pDeviceInfoData = pDeviceInfoData;

                        //
                        // Try to get the property sheet extension from setupapi.
                        //

                        if (PropSheetExtProc(&PropSheetRequest,
                                             AddPropSheetPageProc,
                                             &hPage))
                        {

                            PropHeader.dwSize      = sizeof(PROPSHEETHEADER);
                            PropHeader.dwFlags     = PSH_NOAPPLYNOW;
                            PropHeader.hwndParent  = hwnd;
                            PropHeader.hInstance   = ghmod;
                            PropHeader.pszIcon     = NULL;
                            PropHeader.pszCaption  = TEXT(" ");
                            PropHeader.nPages      = 1;
                            PropHeader.phpage      = &hPage;
                            PropHeader.nStartPage  = 0;
                            PropHeader.pfnCallback = NULL;

                            if (PropertySheet(&PropHeader) == -1)
                            {
                                if(hPage)
                                {
                                    DestroyPropertySheetPage(hPage);
                                }
                            }
                            else
                            {
                                //
                                // BUGBUG can this return zero ?
                                //

                                //
                                // If we were successful, then we want to retrieve the user's selection.
                                //

                                CM_Get_First_Log_Conf(&LogConfig,
                                                      pDeviceInfoData->DevInst,
                                                      FORCED_LOG_CONF);

                                ResDes = LogConfig;

                                // while(

                                if (CM_Get_Next_Res_Des(&ResDes,
                                                        ResDes,
                                                        ResType_Mem,
                                                        NULL,
                                                        0) == CR_SUCCESS)
                                {
                                    MEM_RESOURCE memRes;

                                    if (CM_Get_Res_Des_Data(ResDes,
                                                            &memRes,
                                                            sizeof(memRes),
                                                            0) == CR_SUCCESS)
                                    {
                                        RegSetValueEx(hkey,
                                                      TEXT("MemBase"),
                                                      0,
                                                      REG_DWORD,
                                                      (LPBYTE) &(memRes.MEM_Header.MD_Alloc_Base),
                                                      sizeof(DWORD));
                                    }

                                    CM_Free_Res_Des_Handle(ResDes); // resdesnext
                                }
                            }
                        }
                    }
                }
            }
#endif

            RegCloseKey(hkey);
        }

        bSetSoftwareKey = FALSE;

    } while (numDev != 0);

    //
    // optinally run the OpenGl section in the inf.
    // Ignore any errors at this point since this is an optional
    // entry.
    //

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                       TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\OpenGLDrivers"),
                       0,
                       NULL,
                       REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE,
                       NULL,
                       &hkey,
                       &disposition) == ERROR_SUCCESS)
    {
        wsprintf(szSoftwareSection,
                 TEXT("%ws.OpenGLSoftwareSettings"),
                 DriverInfoDetailData->SectionName);

        SetupInstallFromInfSection(hwnd,
                                   InfFileHandle,
                                   szSoftwareSection,
                                   SPINST_REGISTRY,
                                   hkey,
                                   NULL,
                                   0,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL);

        RegCloseKey(hkey);
    }

    //
    // Get the next line for detection if we are detecting.
    //

    SetupCloseInfFile(InfFileHandle);

    TraceMsg(TF_GENERAL, "DeskInstallServiceExtensions return %d\n", retError);

    return retError;
}

#define SZ_UPGRADE_DESCRIPTION TEXT("_RealDescription")
#define SZ_UPGRADE_MFG         TEXT("_RealMfg")

void
DeskMarkUpNewDeviceNode(
    IN HDEVINFO         hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData
    )
{
    HKEY    hKey;
    PTCHAR  szProperty;
    DWORD   dwSize;
    TCHAR   szBuffer[LINE_LEN];

    //
    // Make sure the device desc is "good" 
    // don't do this to legacy devnode though
    //
    
    ZeroMemory(szBuffer, sizeof(szBuffer));
    if (DeskGetDeviceNodePath(pDeviceInfoData, szBuffer, LINE_LEN-1) &&
        DeskIsLegacyDeviceNode(szBuffer)) {
        
        DeskLogError(LogSevInformation,
                     IDS_SETUPLOG_MSG_001, 
                     szBuffer);
        return;
    }
        
    hKey = SetupDiCreateDevRegKey(hDevInfo,
                                  pDeviceInfoData,
                                  DICS_FLAG_GLOBAL,
                                  0,
                                  DIREG_DEV,
                                  NULL,
                                  NULL);

    if (hKey == INVALID_HANDLE_VALUE) {
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_002);
        return;
    }

    dwSize = 0;
    if (RegQueryValueEx(hKey,
                        SZ_UPGRADE_DESCRIPTION,
                        0,
                        NULL,
                        NULL,
                        &dwSize) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return;
    }

    DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_003, szBuffer);

    ASSERT(dwSize != 0);
    dwSize *= sizeof(TCHAR);
    szProperty = (PTCHAR) LocalAlloc(LPTR, dwSize);
    if (szProperty &&  
        RegQueryValueEx(hKey,
                        SZ_UPGRADE_DESCRIPTION,
                        0,
                        NULL,
                        (PBYTE) szProperty,
                        &dwSize) == ERROR_SUCCESS) {

        SetupDiSetDeviceRegistryProperty(hDevInfo,
                                         pDeviceInfoData,
                                         SPDRP_DEVICEDESC,
                                         (PBYTE) szProperty,
                                         ByteCountOf(lstrlen(szProperty)+1));

        RegDeleteValue(hKey, SZ_UPGRADE_DESCRIPTION);

        DeskLogError(LogSevInformation,
                     IDS_SETUPLOG_MSG_004, 
                     szProperty);
    }
    LocalFree(szProperty);
    szProperty = NULL;

    dwSize = 0;
    if (RegQueryValueEx(hKey,
                        SZ_UPGRADE_MFG,
                        0,
                        NULL,
                        NULL,
                        &dwSize) != ERROR_SUCCESS) {
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_005);
        RegCloseKey(hKey);
        return;
    }

    ASSERT(dwSize != 0);
    dwSize *= sizeof(TCHAR);
    szProperty = (PTCHAR) LocalAlloc(LPTR, dwSize);
    if (szProperty &&  
        RegQueryValueEx(hKey,
                        SZ_UPGRADE_MFG,
                        0,
                        NULL,
                        (PBYTE) szProperty,
                        &dwSize) == ERROR_SUCCESS) {

        SetupDiSetDeviceRegistryProperty(hDevInfo,
                                         pDeviceInfoData,
                                         SPDRP_MFG,
                                         (PBYTE) szProperty,
                                         ByteCountOf(lstrlen(szProperty)+1));

        RegDeleteValue(hKey, SZ_UPGRADE_MFG);

        DeskLogError(LogSevInformation,
                     IDS_SETUPLOG_MSG_006, 
                     szProperty);
    }
    LocalFree(szProperty);
    szProperty = NULL;

    RegCloseKey(hKey);
}

DWORD
DeskInstallService(
    IN HDEVINFO         hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData OPTIONAL,
    IN LPTSTR           pServiceName,
    IN DWORD            dwDetect
    )
{
    SP_DEVINSTALL_PARAMS   DeviceInstallParams;
    SP_DRVINFO_DATA        DriverInfoData;
    SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;
    DWORD                  cbOutputSize;
    HINF                   hInf;
    TCHAR                  ActualInfSection[LINE_LEN];
    INFCONTEXT             infContext;
    DWORD                  status = NO_ERROR;

    TraceMsg(TF_GENERAL, "DeskInstallService called\n");


    //
    // Get the params so we can get the window handle.
    //

    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    SetupDiGetDeviceInstallParams(hDevInfo,
                                  pDeviceInfoData,
                                  &DeviceInstallParams);

    //
    // Retrieve information about the driver node selected for this device.
    //

    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);

    if (!SetupDiGetSelectedDriver(hDevInfo,
                                  pDeviceInfoData,
                                  &DriverInfoData))
    {
        TraceMsg(TF_GENERAL, "SetupDiGetSelectedDriver Error %d\n", GetLastError());
        return GetLastError();
    }

    DriverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);

    if (!(SetupDiGetDriverInfoDetail(hDevInfo,
                                     pDeviceInfoData,
                                     &DriverInfoData,
                                     &DriverInfoDetailData,
                                     DriverInfoDetailData.cbSize,
                                     &cbOutputSize)) &&
        (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
    {
        TraceMsg(TF_GENERAL, "SetupDiGetDriverInfoDetail Error %d\n", GetLastError());
        return GetLastError();
    }

#if 0
    //
    // DIDD::HardwareID will be blank unless we pass a buffer large enough..
    // since we are doing nothing w/this value now, forget about it!
    //
    if (DriverInfoDetailData.HardwareID[0] == 0)
    {
        TraceMsg(TF_GENERAL, "DriverInfo HardwareID = NULL\n");
    }
    else
    {
        TraceMsg(TF_GENERAL, "DriverInfo HardwareID = %s\n", DriverInfoDetailData.HardwareID);
    }
#endif

    //
    // Open the INF that installs this driver node, so we can 'pre-run' the
    // AddService/DelService entries in its install service install section.
    //

    hInf = SetupOpenInfFile(DriverInfoDetailData.InfFileName,
                            NULL,
                            INF_STYLE_WIN4,
                            NULL);

    if(hInf == INVALID_HANDLE_VALUE)
    {
        //
        // For some reason we couldn't open the INF--this should never happen.
        //

        TraceMsg(TF_GENERAL, "SetupOpenInfFile Error\n");
        return ERROR_INVALID_HANDLE;
    }

    //
    // Now find the actual (potentially OS/platform-specific) install section name.
    //

    if (!SetupDiGetActualSectionToInstall(hInf,
                                          DriverInfoDetailData.SectionName,
                                          ActualInfSection,
                                          sizeof(ActualInfSection) / sizeof(TCHAR),
                                          NULL,
                                          NULL))
    {
        status = GetLastError();
        TraceMsg(TF_GENERAL, "SetupDiGetActualSectionToInstall Error %d\n", status);
    }
    else
    {
        //
        // Append a ".Services" to get the service install section name.
        //

        lstrcat(ActualInfSection, TEXT(".Services"));

        //
        // Now run the service modification entries in this section...
        //

        if (!SetupInstallServicesFromInfSection(hInf,
                                                ActualInfSection,
                                                0))
        {
            status = GetLastError();
            TraceMsg(TF_GENERAL, "SetupInstallServicesFromInfSection Error %d\n", status);
        }
    }

    //
    // Get the service Name if needed (detection)
    //

    if (SetupFindFirstLine(hInf,
                           ActualInfSection,
                           TEXT("AddService"),
                           &infContext))
    {
        SetupGetStringField(&infContext,
                            1,
                            pServiceName,
                            LINE_LEN,
                            NULL);
    }


    SetupCloseInfFile(hInf);

    if (status != NO_ERROR)
    {
        return status;
    }

    //
    // Now that the basic install has been performed (wihtout starting the
    // device), write the extra data to the registry.
    //

    status = DeskInstallServiceExtensions(DeviceInstallParams.hwndParent,
                                          hDevInfo,
                                          pDeviceInfoData,
                                          &DriverInfoData,
                                          &DriverInfoDetailData,
                                          pServiceName,
                                          dwDetect);

    if (status != NO_ERROR)
    {
        TraceMsg(TF_GENERAL, "DeskInstallServiceExtensions Error %d\n", status);
        return status;
    }

    if (dwDetect == 0)
    {
        //
        // We have a DEVNODE and therefore a pDeviceInfoData.
        // Do the full device install, which will also start the device
        // dynamically.
        //
        if (!SetupDiInstallDevice(hDevInfo, pDeviceInfoData))
        {
            DeskLogError(LogSevInformation,
                         IDS_SETUPLOG_MSG_007, 
                         GetLastError());
            TraceMsg(TF_GENERAL, "SetupDiInstallDevice Error %d\n", GetLastError());

            //
            // Remove the device !??
            //

            return GetLastError();
        }
        else
        {
            //
            // For a PnP Device which will never do detection, we want to mark
            // the device as DemandStart.
            //
            DeskSCSetServiceStartType(pServiceName, SERVICE_DEMAND_START);

            //
            // Make sure the device description and mfg are the original values
            // and not the marked up ones we might have made during select bext
            // compat drv
            //
            DeskMarkUpNewDeviceNode(hDevInfo, pDeviceInfoData);
        }
    }

    //
    // If some of the flags (like paged pool) needed to be changed,
    // let's ask for a reboot right now.
    //
    // Otherwise, we can actually try to start the device at this point.
    //

    TraceMsg(TF_GENERAL, "DeskInstallService return 0\n");

    return NO_ERROR;
}

DWORD
DeskRemoveService(
    IN HDEVINFO         hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData OPTIONAL,
    IN LPTSTR           pServiceName
    )
{
    CONFIGRET  crStatus = CR_FAILURE;
    ULONG      DevIdBufferLen;
    LPTSTR     DevIdBuffer;
    LPTSTR     pTmp;

    TraceMsg(TF_GENERAL, "DeskRemoveService called\n");

    //
    // Remove the service
    //

    //
    // Check if any devices are controlled by this service.
    //

    CM_Get_Device_ID_List_Size(&DevIdBufferLen,
                               pServiceName,
                               CM_GETIDLIST_FILTER_SERVICE);

    if ( (DevIdBufferLen) &&
         (DevIdBuffer = LocalAlloc(LPTR, DevIdBufferLen * sizeof(TCHAR))))
    {

        crStatus = CM_Get_Device_ID_List(pServiceName,
                                         DevIdBuffer,
                                         DevIdBufferLen,
                                         CM_GETIDLIST_FILTER_SERVICE);
    }

    if (crStatus == CR_SUCCESS)
    {
        pTmp = DevIdBuffer;

        //
        // Walk the MULTI_SZ with the devices in it.
        //

        while (*pTmp)
        {
            DEVINST DevInst;

            // ASSERT(wcsstr(TEXT("_DETECT"), pTmp));

            crStatus =
                CM_Locate_DevNode(&DevInst,
                                  pTmp,
                                  CM_LOCATE_DEVNODE_NORMAL | CM_LOCATE_DEVNODE_PHANTOM);

            if (crStatus != CR_SUCCESS)
            {
                TraceMsg(TF_GENERAL, " CM_Locate_DevNode %ws failed %d\n",
                         pTmp, crStatus);
                continue;
            }

            //
            // LATER : Make sure we only delete legacy devnodes.
            //

            crStatus = CM_Uninstall_DevNode(DevInst, 0);

            if (crStatus != CR_SUCCESS)
            {
                TraceMsg(TF_GENERAL, " CM_Locate_DevNode %ws failed %d\n",
                         pTmp, crStatus);
                continue;
            }

            TraceMsg(TF_GENERAL, "Deleted device %ws\n", pTmp);

            //
            // Next string in the MULTI_SZ
            //

            while (*pTmp++);
        }

        DeskSCDeleteService(pServiceName);
    }

    LocalFree(DevIdBuffer);
    TraceMsg(TF_GENERAL, "DeskRemoveService return 0\n");

    return NO_ERROR;
}


/***************************************************************************/
/***************************************************************************/
/**                                                                       **/
/**                              Detection                                **/
/**                                                                       **/
/***************************************************************************/
/***************************************************************************/


GUID DispDetectCLSID = { 0x92940c6e, 0xa419, 0x11d1,
                         { 0x8b, 0x32, 0x00, 0xa0, 0xc9, 0x06, 0x8f, 0xf3}
                       };

DWORD
DeskDetectDevice(
    IN  HDEVINFO                hDevInfo,
    IN  PDETECT_PROGRESS_NOTIFY ProgressFunction,
    IN  PVOID                   ProgressFunctionParam,
    OUT PSP_DRVINFO_DATA        pDrvInfoData
    )
{

    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    HINF                 InfFileHandle;
    PVOID                Context;
    INFCONTEXT           infoContext;
    ULONG                LineCount = 0;
    ULONG                LoopCount = 0;
    TCHAR                Provider[LINE_LEN];
    UCHAR                Buffer[1024];
    DWORD                detectionStatus = ERROR_DEV_NOT_EXIST;

    TraceMsg(TF_GENERAL, "DeskDetectDevice called\n");



    if ((hDevInfo = SetupDiCreateDeviceInfoList(&DispDetectCLSID,
                                                NULL)) == INVALID_HANDLE_VALUE)
    {
        TraceMsg(TF_GENERAL, "SetupDiCreateDeviceInfoList failed\n");
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Set the parameters for the device installation to only use dispdet.inf
    // Also use this to get the hwnd.
    //

    ZeroMemory(&DeviceInstallParams, sizeof(DeviceInstallParams));
    DeviceInstallParams.cbSize = sizeof(DeviceInstallParams);

    SetupDiGetDeviceInstallParams(hDevInfo,
                                  NULL,
                                  &DeviceInstallParams);

    DeviceInstallParams.Flags |= DI_ENUMSINGLEINF;

    lstrcpy(DeviceInstallParams.DriverPath, TEXT("dispdet.inf"));

    SetupDiSetDeviceInstallParams(hDevInfo,
                                  NULL,
                                  &DeviceInstallParams);

    if (!SetupDiBuildDriverInfoList(hDevInfo, NULL, SPDIT_CLASSDRIVER))
    {
        return ERROR_INVALID_PARAMETER;
    }

    TraceMsg(TF_GENERAL, "DetectDriverList DriverInfoList created\n");


    //
    // Open the main inf for us to do detection.
    // Detection is only done on legacy drivers that we provide
    //

    InfFileHandle = SetupOpenInfFile(TEXT("dispdet.inf"),
                                     NULL,
                                     INF_STYLE_WIN4,
                                     NULL);

    if (InfFileHandle == INVALID_HANDLE_VALUE)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Append layout.inx so we can do file copy.
    //

    SetupOpenAppendInfFile(NULL,
                           InfFileHandle,
                           NULL);

    Context = SetupInitDefaultQueueCallbackEx(DeviceInstallParams.hwndParent,
                                              INVALID_HANDLE_VALUE, 0, 0, 0);

    if (!SetupInstallFromInfSection(DeviceInstallParams.hwndParent,
                                    InfFileHandle,
                                    TEXT("detect.install.drivers"),
                                    SPINST_FILES,
                                    NULL,
                                    NULL,
                                    0,
                                    &SetupDefaultQueueCallback,
                                    Context,
                                    NULL,
                                    NULL))
    {
        TraceMsg(TF_GENERAL, " Install detect files failed %d\n", GetLastError());
        return GetLastError();
    }

    //
    // Get the provider name
    //

    if (SetupGetInfInformation(InfFileHandle,
                               INFINFO_INF_SPEC_IS_HINF,
                               (PSP_INF_INFORMATION) Buffer,
                               sizeof(Buffer),
                               NULL))
    {
        TraceMsg(TF_GENERAL, " Getting Provider Name\n");

        SetupQueryInfVersionInformation((PSP_INF_INFORMATION) Buffer,
                                        0,
                                        TEXT("Provider"),
                                        Provider,
                                        LINE_LEN,
                                        NULL);
    }

    //
    // Get the list of drivers we want to try to detect.
    //

    LineCount = SetupGetLineCount(InfFileHandle,
                                  TEXT("DetectDriverList"));

    TraceMsg(TF_GENERAL, "DetectDriverList size = %d\n", LineCount);

    //
    // Get the list of drivers we want to try to detect.
    //

    if (!SetupFindFirstLine(InfFileHandle,
                            TEXT("DetectDriverList"),
                            NULL,
                            &infoContext))
    {
        return ERROR_INVALID_PARAMETER;
    }

    do
    {
        TCHAR DeviceDescription[LINE_LEN];
        ULONG DeviceDescriptionSize;
        TCHAR Manufacturer[LINE_LEN];
        ULONG ManufacturerSize;
        TCHAR ServiceName[LINE_LEN];
        TCHAR DetectBroken[LINE_LEN];
        ULONG DetectBrokenSize;


        ASSERT(LoopCount < LineCount);

        //
        // For each driver in the list, install it and see if it loads
        //

        if ( (!SetupGetStringField(&infoContext,
                                   0,
                                   Manufacturer,
                                   LINE_LEN,
                                   &ManufacturerSize))
           ||
             (!SetupGetStringField(&infoContext,
                                   1,
                                   DeviceDescription,
                                   LINE_LEN,
                                   &DeviceDescriptionSize))
           )
        {
            TraceMsg(TF_GENERAL, " Get Item failed on line %d, error %d\n",
                     LineCount, GetLastError());
        }
        else
        {
            //
            // Find the appropriate entry in the list of devices.
            //

            TraceMsg(TF_GENERAL, "DeskDetectDevice on device %ws, %ws\n",
                     Manufacturer, DeviceDescription);

            ZeroMemory(pDrvInfoData, sizeof(SP_DRVINFO_DATA));
            pDrvInfoData->cbSize     = sizeof(SP_DRVINFO_DATA);
            pDrvInfoData->DriverType = SPDIT_CLASSDRIVER;
            pDrvInfoData->Reserved   = 0; // Means search the list for this driver
            lstrcpy(pDrvInfoData->Description,  DeviceDescription);
            lstrcpy(pDrvInfoData->MfgName,      Manufacturer);
            lstrcpy(pDrvInfoData->ProviderName, Provider);

            if (!SetupDiSetSelectedDriver(hDevInfo,
                                          NULL,
                                          pDrvInfoData))
            {
                TraceMsg(TF_GENERAL, "DeskDetectDevice Select device failed on %ws, error %d\n",
                         DeviceDescription, GetLastError());
            }
            else
            {
                TraceMsg(TF_GENERAL, "DeskDetectDevice Match on device %ws\n", DeviceDescription);

                //
                // Install the service in the registry.
                //

                if (DeskInstallService(hDevInfo,
                                       NULL,
                                       ServiceName,
                                       LEGACY_DETECT) == NO_ERROR)
                {
                    TraceMsg(TF_GENERAL, "DeskDetectDevice Service %ws installed\n",
                             ServiceName);

                    //
                    //  We have no DEVNODE.  Start the driver by hand.
                    //

                    if (DeskSCStartService(ServiceName) == NO_ERROR)
                    {
                        TraceMsg(TF_GENERAL, "DeskDetectDevice Service started\n");

                        //
                        // We detected a device !!!
                        //

                        detectionStatus = NO_ERROR;

                        //
                        // Stop the service
                        // BUGBUG Don't care if it stops or not for now.
                        //

                        DeskSCStopService(ServiceName);
                    }

                    TraceMsg(TF_GENERAL, "DeskDetectDevice remove Service\n");

                    DeskRemoveService(hDevInfo,
                                      NULL,
                                      ServiceName);

                    //
                    // For broken drivers that trash the screen during
                    // detection, reset the device to the default state.
                    //

                    if (SetupGetStringField(&infoContext,
                                            2,
                                            DetectBroken,
                                            LINE_LEN,
                                            &DetectBrokenSize))
                    {
                        TraceMsg(TF_GENERAL, " detect : %ws\n", DetectBroken);

                        if (lstrcmp(DetectBroken, TEXT("detect_broken")) == 0)
                        {
                            TraceMsg(TF_GENERAL, "DeskDetectDevice detect_broken - RESET_SCREEN\n");
                            ChangeDisplaySettings(NULL, CDS_RESET | CDS_RAWMODE);
                        }
                    }
                }
            }

            //
            // If we found a device, then stop detection.
            //
            // NOTE - machines with more than 1 video card should have PnP
            // drivers for those !
            //

            if (detectionStatus == NO_ERROR)
            {
                TraceMsg(TF_GENERAL, "DeskDetectDevice found device - stop detection\n");
                break;
            }
        }

        //
        // Track how far along we are in detection
        //

        TraceMsg(TF_GENERAL, "DeskDetectDevice Display LoopCount\n");

        LoopCount++;

        if (ProgressFunction)
        {
            if (ProgressFunction(ProgressFunctionParam,
                                 LoopCount * 100 / LineCount))
            {
                //
                // User cancelled - leave now.
                //

                break;
            }
        }

    } while (SetupFindNextLine(&infoContext, &infoContext));

    TraceMsg(TF_GENERAL, "DeskDetectDevice Complete\n");

    //
    // Clean out all of the extra files we copied
    //

    if (!SetupInstallFromInfSection(DeviceInstallParams.hwndParent,
                                    InfFileHandle,
                                    TEXT("detect.remove.drivers"),
                                    SPINST_FILES,
                                    NULL,
                                    NULL,
                                    0,
                                    &SetupDefaultQueueCallback,
                                    Context,
                                    NULL,
                                    NULL))
    {
        TraceMsg(TF_GENERAL, " Remove detect files failed %d\n", GetLastError());
    }


    return (detectionStatus);
}




/***************************************************************************/
/***************************************************************************/
/**                                                                       **/
/**                           Class Installers                            **/
/**                                                                       **/
/***************************************************************************/
/***************************************************************************/

//
// Migration functions
//
#define UPGRADE_FROM_351 0x1
#define UPGRADE_FROM_40  0x2
#define UPGRADE_FROM_50  0x3

BOOLEAN
DeskIsLegacyDeviceNode(
    const PTCHAR szRegPath
    )
{
    static const TCHAR root[] = TEXT("ROOT\\LEGACY_");
    return _tcsncicmp(root, szRegPath, sizeof(root)/sizeof(TCHAR)-1) == 0;
//    return lstrcmpi(root, szRegPath, sizeof(root)/sizeof(TCHAR)-1) == 0;
}

BOOLEAN
DeskIsLegacyDeviceNode2(
    PSP_DEVINFO_DATA pDid
    )
{
    TCHAR szBuf[LINE_LEN];

    if (DeskGetDeviceNodePath(pDid, szBuf, LINE_LEN-1)) {
        return DeskIsLegacyDeviceNode(szBuf);
    }

    return FALSE;
}
 
BOOLEAN
DeskGetDeviceNodePath(
    PSP_DEVINFO_DATA pDid,
    PTCHAR           szPath,
    LONG             len
    )
{
    //
    // If we have a root legacy device, then don't install any new
    // devnode (until we get better at this).
    //
    return CR_SUCCESS == CM_Get_Device_ID(pDid->DevInst, szPath, len, 0);
}

VOID
DeskNukeDeviceNode(
    LPTSTR           szService,
    HDEVINFO         hDevInfo,
    PSP_DEVINFO_DATA pDeviceInfoData
    )
{
    SP_REMOVEDEVICE_PARAMS rdParams;
    TCHAR                  szPath[LINE_LEN];

    if (szService) {
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_008, szService);
        DeskSCSetServiceStartType(szService, SERVICE_DISABLED);
    }
    else {
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_009);
    }
                             
    if (DeskGetDeviceNodePath(pDeviceInfoData, szPath, LINE_LEN)) {
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_010, szPath);
    }

    //
    // Remove the devnode
    //
    ZeroMemory(&rdParams, sizeof(SP_REMOVEDEVICE_PARAMS));
    rdParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    rdParams.ClassInstallHeader.InstallFunction = DIF_REMOVE;
    rdParams.Scope = DI_REMOVEDEVICE_GLOBAL;

    if (SetupDiSetClassInstallParams(hDevInfo,
                                     pDeviceInfoData,
                                     &rdParams.ClassInstallHeader,
                                     sizeof(SP_REMOVEDEVICE_PARAMS))) {
        if (!SetupDiCallClassInstaller(DIF_REMOVE, hDevInfo, pDeviceInfoData)) {
            DeskLogError(LogSevInformation, 
                         IDS_SETUPLOG_MSG_011, 
                         GetLastError());
        }
    }
    else {
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_012, GetLastError());
    }

}

PTCHAR DeskFindMatchingId(
    PTCHAR DeviceId,    
    PTCHAR IdList               // a multi sz
    )
{
    PTCHAR currentId;

    if (!IdList) {
        return NULL;
    }

    for (currentId = IdList; *currentId; ) {
        if (lstrcmpi(currentId, DeviceId) == 0) {
            //
            // we have a match
            //
            TraceMsg(TF_GENERAL, "made a match of %s", currentId);
            return currentId;
        }
        else {
            // get to the next string in the multi sz
            while (*currentId) {
                currentId++;
            }
            // jump past the null
            currentId++;
        }
    }

    TraceMsg(TF_GENERAL, "No match was made");
    return NULL;
}

UINT
DeskDisableLegacyDeviceNodes(
    VOID 
    )
{
    DWORD           index = 0, dwSize;
    UINT            count = 0;
    HDEVINFO        hDevInfo;
    SP_DEVINFO_DATA did;
    TCHAR           szRegProperty[256];
    PTCHAR          szService;
    
    //
    // Let's find all the video drivers that are installed in the system
    //
    hDevInfo = SetupDiGetClassDevs((LPGUID) &GUID_DEVCLASS_DISPLAY,
                                   NULL,
                                   NULL,
                                   0);

    if (hDevInfo == INVALID_HANDLE_VALUE) {
        TraceMsg(TF_GENERAL, "Disable Legacy - no existing display devices");
        return 0;
    }

    do {

        ZeroMemory(&did, sizeof(SP_DEVINFO_DATA));
        did.cbSize = sizeof(SP_DEVINFO_DATA);

        if (!SetupDiEnumDeviceInfo(hDevInfo, index++, &did)) {
    
            // GetLastError == ERROR_NO_MORE_ITEMS if no more items
            // TraceMsg(TF_GENERAL, "Disable Legacy retrieving device Error %d", GetLastError());
            TraceMsg(TF_GENERAL, "Disabled %d legacy devices on %d tries",
                     count, index - 1);
            DeskLogError(LogSevInformation,
                         IDS_SETUPLOG_MSG_013,
                         count, index - 1);
            SetupDiDestroyDeviceInfoList(hDevInfo);
            return count;
        }

        //
        // If we have a root legacy device, then don't install any new
        // devnode (until we get better at this).
        //
        if (CR_SUCCESS == CM_Get_Device_ID(did.DevInst,
                                           szRegProperty,
                                           sizeof(szRegProperty),
                                           0)) {

            if (DeskIsLegacyDeviceNode(szRegProperty)) {

                TraceMsg(TF_GENERAL, "is a legacy DN!");

                //
                // We have a legacy DevNode, lets disable its service and remove
                // its devnode 
                //
                szService = NULL;
                dwSize = sizeof(szRegProperty);
                if (CM_Get_DevNode_Registry_Property(did.DevInst,
                                                     CM_DRP_SERVICE,
                                                     NULL,
                                                     szRegProperty,
                                                     &dwSize,
                                                     0) == CR_SUCCESS) {
                    //
                    // Make sure we don't disable vga, VgaSave or sglfb
                    //
                    
                    if ((lstrcmpi(szRegProperty, TEXT("vga")) == 0) ||
                        (lstrcmpi(szRegProperty, TEXT("VgaSave")) == 0) ||
                        (lstrcmpi(szRegProperty, TEXT("sglfb")) == 0)) {
                        //ASSERT(FALSE);
                        continue;
                    }

                    szService = szRegProperty;
                }

                DeskNukeDeviceNode(szService, hDevInfo, &did);
                count++;
            }
        }
    } while (1);

    // should never get here
    ASSERT(FALSE);
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return count;
}

BOOL
DeskRegDeleteKeyAndSubkeys(
    HKEY hKey,
    LPCTSTR lpSubKey
    )
{
    HKEY hkDeleteKey;
    TCHAR szChild[MAX_PATH + 1];
    BOOL bReturn = FALSE;
    
    if (RegOpenKey(hKey, lpSubKey, &hkDeleteKey) == ERROR_SUCCESS) {
        
        bReturn = TRUE;
        while (RegEnumKey(hkDeleteKey, 0, szChild, ARRAYSIZE(szChild)) == 
               ERROR_SUCCESS) {
            if (!DeskRegDeleteKeyAndSubkeys(hkDeleteKey, szChild)) {
                bReturn = FALSE;
                break;
            }    
        }        
        
        RegCloseKey(hkDeleteKey);
        
        if (bReturn)
            bReturn = (RegDeleteKey(hKey, lpSubKey) == ERROR_SUCCESS);
    }   
    
    return bReturn;
}
 
VOID
DeskDeleteLegacyAppletExtension(
    VOID 
    )
{
    HKEY  hkLocation;
    HKEY  hkHandlers;
    TCHAR szChild[MAX_PATH + 1];
    HKEY  hKeyUpdate;
    DWORD dwSize, dwPlatform = VER_PLATFORM_WIN32_NT, dwMajorVer = 5;
    
    if (!(bSetupFlags() & INSETUP_UPGRADE)) 
        return;
        
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     SZ_UPDATE_SETTINGS,
                     0,
                     KEY_READ,
                     &hKeyUpdate) != ERROR_SUCCESS) 
        return;

    dwSize = sizeof(DWORD);
    RegQueryValueEx(hKeyUpdate, SZ_UPGRADE_FROM_PLATFORM, NULL, NULL,
                    (PBYTE) &dwPlatform, &dwSize);
    dwSize = sizeof(DWORD);
    RegQueryValueEx(hKeyUpdate, SZ_UPGRADE_FROM_MAJOR_VERSION, NULL, NULL,
                    (PBYTE) &dwMajorVer, &dwSize);

    RegCloseKey(hKeyUpdate);
              
    if (!((dwPlatform == VER_PLATFORM_WIN32_WINDOWS) ||
          (dwPlatform == VER_PLATFORM_WIN32s) ||
          ((dwPlatform == VER_PLATFORM_WIN32_NT) &&
           (dwMajorVer < 5))))
        return;
        
    if (RegOpenKey(HKEY_LOCAL_MACHINE, 
                   REGSTR_PATH_CONTROLSFOLDER TEXT("\\Display"), 
                   &hkLocation ) == ERROR_SUCCESS) {

        if (RegOpenKey(hkLocation, STRREG_SHEX_PROPSHEET, &hkHandlers) ==
            ERROR_SUCCESS) {

            while (RegEnumKey(hkHandlers, 0, szChild, ARRAYSIZE(szChild)) == 
                   ERROR_SUCCESS)
                if (DeskRegDeleteKeyAndSubkeys(hkHandlers, szChild)) {
                    TraceMsg(TF_GENERAL, "Applet extension removed: %ws\n", szChild);
                    DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_098, szChild);
                } else {
                    TraceMsg(TF_GENERAL, "Could not remove all applet extensions\n");
                    DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_097);
                    break;
                }

            RegCloseKey(hkHandlers);
        }

        RegCloseKey(hkLocation);
    }
} 
 
#define SZ_BINARY_LEN 32
typedef struct _DEVDATA {
    SP_DEVINFO_DATA did;
    TCHAR           szBinary[SZ_BINARY_LEN];
    TCHAR           szService[SZ_BINARY_LEN];
} DEVDATA, *PDEVDATA;

DWORD
DeskPerformDatabaseUpgrade(
    HINF        hInf,
    PINFCONTEXT pInfContext,
    BOOL        bUpgrade,
    PTCHAR      szDriverListSection,
    BOOL*       pbForceDeleteAppletExt)
/*--

Remarks:
    This function is called once the ID of the device in question matches an ID
    contained in the upgrade database.   We then compare the state of the system 
    with what is contained in the database.  The following algorithm is followed.
   
    If szDriverListSection is NULL or cannot be found, then bUpgrade is used
    If szDriverListSection is not NUL, then following table is used
    
    bUpgrade    match found in DL           return value
    TRUE        no                          upgrade
    TRUE        yes                         no upgrade
    FALSE       no                          no upgrade
    FALSE       yes                         upgrade
  
    essentially, a match in the DL negates bUpgrade 
    
  ++*/
{
    HKEY       hKey;
    DWORD      dwRet = ERROR_SUCCESS, dwSize;
    INFCONTEXT driverListContext;
    TCHAR      szService[32], szProperty[128];
    TCHAR      szRegPath[128];
    HDEVINFO   hDevInfo;
    PDEVDATA   rgDevData = NULL;
    PSP_DEVINFO_DATA pDid;
    UINT       iData, numData, maxData = 5, iEnum;
    BOOLEAN    foundMatch = FALSE;
    INT        ForceDeleteAppletExt = 0;

    UNREFERENCED_PARAMETER(pInfContext);

    //
    // If no Driver list is given, life is quite simple, just disable all legacy
    // drivers and succ
    //
    if (!szDriverListSection) {

        ASSERT (pbForceDeleteAppletExt == NULL);

        TraceMsg(TF_GENERAL, "straight decision, bUpgrade = %d\n", bUpgrade);
        DeskLogError(LogSevInformation, (bUpgrade ? IDS_SETUPLOG_MSG_014
                                                  : IDS_SETUPLOG_MSG_015));

        return bUpgrade ? ERROR_SUCCESS : ERROR_DI_DONT_INSTALL;
    }

    //
    // By default, do not disable applet extensions 
    //
    ASSERT (pbForceDeleteAppletExt != NULL);
    *pbForceDeleteAppletExt = FALSE;

    TraceMsg(TF_GENERAL, "using DL %s (bUpgrade = %d)", szDriverListSection, bUpgrade); 
    DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_016, szDriverListSection);
    if (!SetupFindFirstLine(hInf,
                            szDriverListSection,
                            NULL,
                            &driverListContext)) {
        //
        // The section listed in the database doesn't exist!  Behave as though
        // it wasn't there
        //
        TraceMsg(TF_GENERAL, "couldn't find it ... assuming no match");
        DeskLogError(LogSevInformation, (bUpgrade ? IDS_SETUPLOG_MSG_017 
                                                  : IDS_SETUPLOG_MSG_018));

        return bUpgrade ? ERROR_SUCCESS : ERROR_DI_DONT_INSTALL;
    }

    hDevInfo = SetupDiGetClassDevs((LPGUID) &GUID_DEVCLASS_DISPLAY,
                                   NULL,
                                   NULL,
                                   0);

    //
    // If no display devices are found, treat this as the case where no match
    // was made
    //
    if (hDevInfo == INVALID_HANDLE_VALUE) {
        TraceMsg(TF_GENERAL, "couldn't open class devs ... assuming no match");

        DeskLogError(LogSevInformation, (bUpgrade ? IDS_SETUPLOG_MSG_019 
                                                  : IDS_SETUPLOG_MSG_020));

        return bUpgrade ? ERROR_SUCCESS : ERROR_DI_DONT_INSTALL;
    }

    rgDevData = (PDEVDATA) LocalAlloc(LPTR, maxData * sizeof(DEVDATA));
    if (!rgDevData) {
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return bUpgrade ? ERROR_SUCCESS : ERROR_DI_DONT_INSTALL;
    }

    iEnum = numData = 0;
    do {
        pDid = &rgDevData[numData].did;

        pDid->cbSize = sizeof(SP_DEVINFO_DATA);
        if (!SetupDiEnumDeviceInfo(hDevInfo, ++iEnum, pDid)) {
            break;
        }

        //
        // If it isn't a legacy devnode, then ignore it
        //
        if (CM_Get_Device_ID(pDid->DevInst, szProperty, sizeof(szProperty), 0)
            == CR_SUCCESS && !DeskIsLegacyDeviceNode(szProperty)) {
            continue;
        }
                                            
        //
        // Initially grab the service name
        //
        dwSize = SZ_BINARY_LEN;
        if (CM_Get_DevNode_Registry_Property(pDid->DevInst,
                                             CM_DRP_SERVICE,
                                             NULL,
                                             rgDevData[numData].szService,
                                             &dwSize,
                                             0) != CR_SUCCESS) {
            //
            // couldn't get the service, ignore this device
            //
            continue;
        }

        szRegPath[0] = TEXT('\0');
        lstrcat(szRegPath, TEXT("System\\CurrentControlSet\\Services\\"));
        lstrcat(szRegPath, rgDevData[numData].szService);

        //
        // Try to grab the real binary name of the service
        //
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         szRegPath,
                         0,
                         KEY_READ,
                         &hKey) == ERROR_SUCCESS) {
            //
            // parse the device map and open the registry.
            //
            dwSize = sizeof(szProperty) / sizeof(TCHAR);
            if (RegQueryValueEx(hKey,
                                TEXT("ImagePath"),
                                NULL,
                                NULL,
                                (LPBYTE) szProperty,
                                &dwSize) == ERROR_SUCCESS) {
                //
                // The is a binary, extract the name, which will be of the form
                // ...\driver.sys
                //
                LPTSTR pszDriver, pszDriverEnd;

                pszDriver = szProperty;
                pszDriverEnd = szProperty + lstrlen(szProperty);

                while(pszDriverEnd != pszDriver &&
                      *pszDriverEnd != TEXT('.')) {
                    pszDriverEnd--;
                }

                *pszDriverEnd = UNICODE_NULL;

                while(pszDriverEnd != pszDriver &&
                      *pszDriverEnd != TEXT('\\')) {
                    pszDriverEnd--;
                }

                pszDriverEnd++;

                //
                // If pszDriver and pszDriverEnd are different, we now
                // have the driver name.
                //
                if (pszDriverEnd > pszDriver &&
                    lstrlen(pszDriverEnd) < SZ_BINARY_LEN) {
                    lstrcpy(rgDevData[numData].szBinary, pszDriverEnd);
                }
            }
    
            RegCloseKey(hKey);
        }
        else {
            // no service at all, consider it bogus
            continue;
        }

        if (++numData == maxData) {
            DEVDATA *tmp;
            UINT    oldMax = maxData;

            maxData <<= 1;

            TraceMsg(TF_GENERAL, "reallocating devs array to %d\n", maxData);

            //
            // Alloc twice as many, copy them over, zero out the new memory
            // and free the old list
            //
            tmp = (PDEVDATA) LocalAlloc(LPTR, maxData * sizeof(DEVDATA));
            memcpy(tmp, rgDevData, oldMax * sizeof(DEVDATA));
            ZeroMemory(tmp + oldMax, sizeof(DEVDATA) * oldMax);
            LocalFree(rgDevData);
            rgDevData = tmp;
        }
    } while (1);

    TraceMsg(TF_GENERAL, "found %d legacy devices", numData);
    DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_021, numData);

    //
    // Assume that no matches have been made
    //
    dwRet =  (bUpgrade ? ERROR_SUCCESS : ERROR_DI_DONT_INSTALL);
    if (numData != 0) {
        //
        // There are legacy devices to check against...
        //
        do {
            LPTSTR szValue;

            memset(szService, 0, sizeof(szService));
            dwSize = sizeof(szService) / sizeof(TCHAR);
            if ((SetupGetFieldCount(&driverListContext) < 1) ||
                !SetupGetStringField(&driverListContext, 
                                     1, 
                                     szService, 
                                     dwSize, 
                                     &dwSize)) {
                continue;
            }
    
            if (szService[0] == TEXT('\0')) {
                continue;
            }
    
            TraceMsg(TF_GENERAL, "Looking at %ws in DL", szService);
    
            for (iData = 0; iData < numData; iData++) {
                if (rgDevData[iData].szBinary[0] != TEXT('\0')) {
                    szValue = rgDevData[iData].szBinary;
                }
                else {
                    szValue = rgDevData[iData].szService;
                }

                if (lstrcmpi(szService, szValue) == 0) {
                    TraceMsg(TF_GENERAL, "\tMade a binary match!!!");
                    DeskLogError(LogSevInformation, 
                                 (bUpgrade ? IDS_SETUPLOG_MSG_022 
                                           : IDS_SETUPLOG_MSG_023));

                    dwRet = (bUpgrade ? ERROR_DI_DONT_INSTALL : ERROR_SUCCESS);
                    foundMatch = TRUE;
                    
                    //
                    // In case we fail upgrade, do we want to disable applet 
                    // extensions?
                    //

                    if ((dwRet == ERROR_DI_DONT_INSTALL) &&
                        (SetupGetFieldCount(&driverListContext) >= 2) &&
                        SetupGetIntField(&driverListContext,
                                         2,
                                         &ForceDeleteAppletExt)) {

                        *pbForceDeleteAppletExt = 
                            (ForceDeleteAppletExt != 0);
                    }

                    break;
                }
            }
        } while (SetupFindNextLine(&driverListContext, &driverListContext));
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);
    LocalFree(rgDevData);

    TraceMsg(TF_GENERAL, "returning 0x%x, bUpgrade = %d\n", dwRet, bUpgrade);
    if (!foundMatch) {
        DeskLogError(LogSevInformation, 
                     (bUpgrade ? IDS_SETUPLOG_MSG_024 : IDS_SETUPLOG_MSG_025),
                     szDriverListSection);
    }

    return dwRet;
}

DWORD 
DeskCheckDatabase(
    IN HDEVINFO         hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData,
    BOOL*               pbForceDeleteAppletExt
    )
{
    DWORD       dwRet = ERROR_SUCCESS, dwSize, dwValue;
    HINF        hInf;
    HKEY        hKeyUpdate;
    INFCONTEXT  infContext;
    BOOLEAN     foundMatch = FALSE;
    TCHAR       szDatabaseId[200];
    TCHAR       szDriverListSection[100];
    PTCHAR      szHardwareIds = NULL, szCompatIds = NULL,szDatabaseSection;
    CONFIGRET   cr;
    ULONG       len;
    PTCHAR      szMatchedId = NULL;
    int         upgrade = FALSE, upgradeFrom;

    TCHAR szDatabaseInf[] = TEXT("display.inf");
    TCHAR szDatabaseSection40[] = TEXT("VideoUpgradeDatabase.4.0");
    TCHAR szDatabaseSection351[] = TEXT("VideoUpgradeDatabase.3.51");

    ASSERT (pbForceDeleteAppletExt != NULL);
    *pbForceDeleteAppletExt = FALSE;

    //
    // Check to see if this is a clean install and we have valid params,
    // if not, then assume success
    //
    if (!pDeviceInfoData || !(bSetupFlags() & INSETUP_UPGRADE)) {
        TraceMsg(TF_WARNING, "not upgrading or invalid data");
        return ERROR_SUCCESS;
    }

    //
    // All of the following values were placed here by our winnt32 migration dll
    //
    // Find out what version of windows we are upgrading FROM
    //
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     SZ_UPDATE_SETTINGS,
                     0,
                     KEY_READ,
                     &hKeyUpdate) != ERROR_SUCCESS) {
        //
        // Assume a 4.0 upgrade
        //
        TraceMsg(TF_ERROR, "not upgrading or invalid data");
        DeskLogError(LogSevWarning, IDS_SETUPLOG_MSG_026);
        szDatabaseSection = szDatabaseSection40;
        upgradeFrom = UPGRADE_FROM_40;
    }


    dwSize = sizeof(DWORD);
    RegQueryValueEx(hKeyUpdate, SZ_UPGRADE_FROM_PLATFORM, NULL, NULL,
                    (PBYTE) &dwValue, &dwSize);
    TraceMsg(TF_GENERAL, "Platform ID is 0x%x", dwValue);

    switch (dwValue) {
    case VER_PLATFORM_WIN32_NT:
        //
        // At the moment, we only do something special for upgrade from NT 
        //
        break;

    case VER_PLATFORM_WIN32_WINDOWS:        // win9x
    case VER_PLATFORM_WIN32s:               // win31
    default:
        //
        // Succeed these upgrades b/c they won't have wacky legacy registries
        //
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_027);
        TraceMsg(TF_GENERAL, "do nothing for non NT upgrade");
        RegCloseKey(hKeyUpdate);
        return ERROR_SUCCESS;
    }

    //
    // Don't care about the minor version just quite yet
    //
    dwSize = sizeof(DWORD);
    RegQueryValueEx(hKeyUpdate, SZ_UPGRADE_FROM_MAJOR_VERSION, NULL, NULL,
                    (PBYTE) &dwValue, &dwSize);
    RegCloseKey(hKeyUpdate);

    switch (dwValue) {
    case 3:
        TraceMsg(TF_GENERAL, "use 3.51 DB");
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_028);
        szDatabaseSection = szDatabaseSection351;
        upgradeFrom = UPGRADE_FROM_351;
        break;

    case 4:
        TraceMsg(TF_GENERAL, "use 4.0 DB");
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_029);
        szDatabaseSection = szDatabaseSection40;
        upgradeFrom = UPGRADE_FROM_40;
        break;

    case 5:
        TraceMsg(TF_GENERAL, "BAIL ... upgrade from 5.0");
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_030);
        //
        // At the moment, we don't do anything when upgrading from 5.0 to 5.0
        //
        return ERROR_SUCCESS;
    }

    len = 0;
    cr = CM_Get_DevNode_Registry_Property(pDeviceInfoData->DevInst,
                                           CM_DRP_HARDWAREID,
                                           NULL,
                                           NULL,
                                           &len,
                                           0);

    if (cr == CR_BUFFER_SMALL) {
        szHardwareIds = LocalAlloc(LPTR, len * sizeof(TCHAR));
        if (szHardwareIds) {
            CM_Get_DevNode_Registry_Property(pDeviceInfoData->DevInst,
                                             CM_DRP_HARDWAREID,
                                             NULL,
                                             szHardwareIds,
                                             &len,
                                             0);

            if (DeskFindMatchingId(TEXT("LEGACY_UPGRADE_ID"), szHardwareIds)) {
                DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_031);
                LocalFree(szHardwareIds);
                return ERROR_SUCCESS;
            }
        }
    }

    //
    // This function will return CR_NO_SUCH_VALUE if this property doesn't exist
    //
    len = 0;
    cr = CM_Get_DevNode_Registry_Property(pDeviceInfoData->DevInst,
                                          CM_DRP_COMPATIBLEIDS,
                                          NULL,
                                          NULL,
                                          &len,
                                          0);

    if (cr == CR_BUFFER_SMALL) {
        szCompatIds = LocalAlloc(LPTR, len * sizeof(TCHAR));
        if (szCompatIds) {
            CM_Get_DevNode_Registry_Property(pDeviceInfoData->DevInst,
                                             CM_DRP_COMPATIBLEIDS,
                                             NULL,
                                             szCompatIds,
                                             &len,
                                             0);
        }
    }

    if (!szHardwareIds && !szCompatIds) {
        //
        // No IDs to look up!  Assume success.
        //
        TraceMsg(TF_ALWAYS, "No hw or compat IDs during allow install for vid card");
        DeskLogError(LogSevWarning, IDS_SETUPLOG_MSG_032);
        return ERROR_DI_DONT_INSTALL;
    }

    hInf = SetupOpenInfFile(szDatabaseInf,
                            NULL,
                            INF_STYLE_WIN4,
                            NULL);

    if (hInf == INVALID_HANDLE_VALUE) {
        //
        // ergh, couldn't open the inf.  This shouldn't happen.  Use default 
        // upgrade logic
        //
        TraceMsg(TF_GENERAL, "could not open DB");
        DeskLogError(LogSevWarning, IDS_SETUPLOG_MSG_033);
    }
    else {
        if (!SetupFindFirstLine(hInf,
                                szDatabaseSection,
                                NULL,
                                &infContext)) {
            //
            // Couldn't find the section or there are no entries in it.  Use
            // default upgrade logic
            //
            DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_034, szDatabaseSection);
        }
        else {
            do {
                dwSize = sizeof(szDatabaseId) / sizeof(TCHAR);
                if (!SetupGetStringField(&infContext, 0, szDatabaseId, dwSize, &dwSize)) {
                    continue;
                }
        
                TraceMsg(TF_GENERAL, "found DB id of %s", szDatabaseId);
        
                szMatchedId = DeskFindMatchingId(szDatabaseId, szHardwareIds);
                if (!szMatchedId) {
                    szMatchedId = DeskFindMatchingId(szDatabaseId, szCompatIds);
                }
        
                if (szMatchedId) {
                    DeskLogError(LogSevInformation,
                                 IDS_SETUPLOG_MSG_035,
                                 szMatchedId);

                    //
                    // do something here and then get out of the loop
                    //
                    SetupGetIntField(&infContext, 1, &upgrade);
                    if (SetupGetFieldCount(&infContext) >= 2) {
                        dwSize = sizeof(szDriverListSection) / sizeof(TCHAR);
                        SetupGetStringField(&infContext, 2, szDriverListSection, dwSize, &dwSize);
                        dwRet = DeskPerformDatabaseUpgrade(hInf, 
                                                           &infContext, 
                                                           upgrade, 
                                                           szDriverListSection, 
                                                           pbForceDeleteAppletExt);
                    }
                    else {
                        dwRet = DeskPerformDatabaseUpgrade(hInf, &infContext, upgrade, NULL, NULL);
                    }
        
                    break;
                }
        
            } while (SetupFindNextLine(&infContext, &infContext));
        }
    }

    if (!szMatchedId) {
        switch (upgradeFrom) {
        case UPGRADE_FROM_351:
            TraceMsg(TF_GENERAL, "no match made, succeeeding upgrade (3.51)");
            DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_036);
            dwRet = ERROR_SUCCESS;
            break;

        case UPGRADE_FROM_40:
            TraceMsg(TF_GENERAL, "no match made, failing upgrade (4.00)");
            DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_037);
            dwRet = ERROR_DI_DONT_INSTALL;
            break;

        default:
            // assume failure
            TraceMsg(TF_GENERAL, "no match made, failing upgrade (unknown)");
            DeskLogError(LogSevWarning, IDS_SETUPLOG_MSG_038);
            dwRet = ERROR_DI_DONT_INSTALL;
            break;
        }
    }

    if (szHardwareIds) {
        LocalFree(szHardwareIds);
    }
    if (szCompatIds) {
        LocalFree(szCompatIds);
    }

    if (hInf != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile(hInf);
    }

    if (dwRet == ERROR_SUCCESS) {
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_039);
    }
    else {
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_040);
    }

    return dwRet;
}

void
DisplayGetUpgradeDeviceStrings(
    PTCHAR Description,
    PTCHAR MfgName,
    PTCHAR ProviderName
    )
{
    TCHAR       szDisplay[] = TEXT("display.inf");
    TCHAR       szDeviceStrings[] = TEXT("SystemUpgradeDeviceStrings");
    TCHAR       szValue[LINE_LEN];
    HINF        hInf;
    INFCONTEXT  infContext;
    DWORD       dwSize;

    hInf = SetupOpenInfFile(szDisplay, NULL, INF_STYLE_WIN4, NULL);

    if (hInf == INVALID_HANDLE_VALUE) {
        goto GetStringsError;
    }

    if (!SetupFindFirstLine(hInf, szDeviceStrings, NULL, &infContext)) 
        goto GetStringsError;

    do {
        dwSize = sizeof(szValue) / sizeof(TCHAR);
        if (!SetupGetStringField(&infContext, 0, szValue, dwSize, &dwSize)) {
            continue;
        }

        dwSize = LINE_LEN;
        if (lstrcmp(szValue, TEXT("Mfg")) ==0) {
            SetupGetStringField(&infContext, 1, MfgName, dwSize, &dwSize);
        }
        else if (lstrcmp(szValue, TEXT("Provider")) == 0) {
            SetupGetStringField(&infContext, 1, ProviderName, dwSize, &dwSize);
        }
        else if (lstrcmp(szValue, TEXT("Description")) == 0) {
            SetupGetStringField(&infContext, 1, Description, dwSize, &dwSize);
        }
    } while (SetupFindNextLine(&infContext, &infContext));

    SetupCloseInfFile(hInf);
    return;

GetStringsError:

    if (hInf != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile(hInf);
    }

    lstrcpy(Description, TEXT("Video Upgrade Device"));
    lstrcpy(MfgName, TEXT("(Standard display types)"));
    lstrcpy(ProviderName, TEXT("Microsoft"));
}

DWORD
DeskAllowInstall(
    IN HDEVINFO         hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData
    )
{
    SP_DRVINFO_DATA        DriverInfoData;
    SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;
    DWORD                  cbOutputSize;
    HINF                   hInf;
    TCHAR                  ActualInfSection[LINE_LEN];
    INFCONTEXT             InfContext;
    ULONG                  DevStatus = 0, DevProblem = 0;
    CONFIGRET              Result;
    
    ASSERT (pDeviceInfoData != NULL);

    //
    // Do not allow install if the device is to be removed.
    //
    //! To avoid any risk, do it during upgrade for now.
    //! However, we should check DN_WILL_BE_REMOVED no matter 
    //! if we are in upgrade or not. 
    //! Remember to remove this test later ...
    // 
    //

    if (bSetupFlags() & INSETUP_UPGRADE) {
    
        Result = CM_Get_DevNode_Status(&DevStatus,
                                       &DevProblem,
                                       pDeviceInfoData->DevInst,
                                       0);

        if ((Result == CR_SUCCESS) &&
            ((DevStatus & DN_WILL_BE_REMOVED) != 0)) {
            
            DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_099);
            return ERROR_DI_DONT_INSTALL;
        }
    }

    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);

    if (!SetupDiGetSelectedDriver(hDevInfo,
                                  pDeviceInfoData,
                                  &DriverInfoData))
    {
        TraceMsg(TF_GENERAL, "SetupDiGetSelectedDriver Error %d", GetLastError());
        return GetLastError();
    }

    ///////////////////////////////////
    // Check for a Win95 Driver
    ///////////////////////////////////

    DriverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);

    if (!(SetupDiGetDriverInfoDetail(hDevInfo,
                                     pDeviceInfoData,
                                     &DriverInfoData,
                                     &DriverInfoDetailData,
                                     DriverInfoDetailData.cbSize,
                                     &cbOutputSize)) &&
        (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
    {
        TraceMsg(TF_GENERAL, "SetupDiGetDriverInfoDetail Error %d", GetLastError());
        return GetLastError();
    }

    if (DriverInfoDetailData.HardwareID[0] == 0)
    {
        TraceMsg(TF_GENERAL, "DriverInfo HardwareID = NULL");
    }
    else
    {
        TraceMsg(TF_GENERAL, "DriverInfo HardwareID = %s", DriverInfoDetailData.HardwareID);
    }

    //
    // Open the INF that installs this driver node, so we can 'pre-run' the
    // AddService/DelService entries in its install service install section.
    //

    hInf = SetupOpenInfFile(DriverInfoDetailData.InfFileName,
                            NULL,
                            INF_STYLE_WIN4,
                            NULL);

    if (hInf == INVALID_HANDLE_VALUE)
    {
        //
        // For some reason we couldn't open the INF--this should never happen.
        //

        TraceMsg(TF_GENERAL, "SetupOpenInfFile Error");
        return ERROR_INVALID_HANDLE;
    }

    //
    // Now find the actual (potentially OS/platform-specific) install section name.
    //

    if (!SetupDiGetActualSectionToInstall(hInf,
                                          DriverInfoDetailData.SectionName,
                                          ActualInfSection,
                                          sizeof(ActualInfSection) / sizeof(TCHAR),
                                          NULL,
                                          NULL))
    {
        TraceMsg(TF_GENERAL, "SetupDiGetActualSectionToInstall Error %d", GetLastError());
        return GetLastError();
    }

    //
    // Append a ".Services" to get the service install section name.
    //

    lstrcat(ActualInfSection, TEXT(".Services"));

    //
    // See if the section exists.
    //

    if (!SetupFindFirstLine(hInf,
                            ActualInfSection,
                            NULL,
                            &InfContext))
    {
        TraceMsg(TF_GENERAL, "SetupFindFirstLine Error %d\n", GetLastError());
        DeskLogError(LogSevError, 
                     IDS_SETUPLOG_MSG_041, 
                     DriverInfoDetailData.InfFileName);
        return ERROR_NON_WINDOWS_NT_DRIVER;
    }

    SetupCloseInfFile(hInf);

    return ERROR_SUCCESS;
}

DWORD
bSetupFlags(VOID)
{
    HKEY   hkey;
    DWORD  retval = 0;
    TCHAR  data[256];
    DWORD  cb;
    LPTSTR regstring;

    hkey = NULL;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     TEXT("System\\Setup"),
                     0,
                     KEY_READ | KEY_WRITE,
                     &hkey) == ERROR_SUCCESS)
    {
        cb = 256;

        if (RegQueryValueEx(hkey,
                            TEXT("SystemSetupInProgress"),
                            NULL,
                            NULL,
                            (LPBYTE)(data),
                            &cb) == ERROR_SUCCESS)
        {
            retval |= *((LPDWORD)(data)) ? INSETUP : 0;
            regstring = TEXT("System\\Video_Setup");
            TraceMsg(TF_GENERAL, "\t\tdesk.cpl - Video Setup");
        }
        else
        {
            regstring = TEXT("System\\Video_NO_Setup");
            TraceMsg(TF_GENERAL, "\t\tdesk.cpl - Video NO Setup");
        }

        cb = 256;

        if (RegQueryValueEx(hkey,
                            TEXT("UpgradeInProgress"),
                            NULL,
                            NULL,
                            (LPBYTE)(data),
                            &cb) == ERROR_SUCCESS)
        {
            retval |= *((LPDWORD)(data)) ? INSETUP_UPGRADE : 0;
            regstring = TEXT("System\\Video_Setup_Upgrade");
            TraceMsg(TF_GENERAL, "\t\tdesk.cpl - Video Setup Upgrade");
        }
        else
        {
            regstring = TEXT("System\\Video_Setup_Clean");
            TraceMsg(TF_GENERAL, "\t\tdesk.cpl - Video Setup Clean");
        }

        if (hkey) {
            RegCloseKey(hkey);
        }
    }

#if ANDREVA_DBG
    {
        DWORD  disposition;

        hkey = NULL;
        RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                       regstring,
                       0,
                       NULL,
                       REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE,
                       NULL,
                       &hkey,
                       &disposition);

        if (hkey) {
            RegCloseKey(hkey);
        }
    }
#endif

    return retval;
}

BOOLEAN
DeskAllowFirstTimeSetup()
{
    HDEVINFO        hDevInfo;
    BOOLEAN         allow = TRUE;
    SP_DEVINFO_DATA did;

    //
    // Don't do video driver detection during upgrade.
    //
    if (bSetupFlags() & INSETUP_UPGRADE)
    {
        return FALSE;
    }

    //
    // Let's find all the video drivers that are installed in the system
    //
    hDevInfo = SetupDiGetClassDevs((LPGUID) &GUID_DEVCLASS_DISPLAY,
                                   NULL,
                                   NULL,
                                   0);

    if (hDevInfo == INVALID_HANDLE_VALUE) {
        //
        // There are no devices of class display, all legacy detection
        //
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_042);
        return TRUE;
    }

    ZeroMemory(&did, sizeof(SP_DEVINFO_DATA));
    did.cbSize = sizeof(SP_DEVINFO_DATA);

    if (SetupDiEnumDeviceInfo(hDevInfo, 0, &did)) {
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_043);
        allow = FALSE;
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);
    return allow;
}

DWORD
DeskSelectBestCompatDrv(
    IN HDEVINFO         hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData
    )
{
    SP_DEVINSTALL_PARAMS dip;
    SP_DRVINFO_DATA      did;
    PTCHAR               szDesc = NULL, szMfg = NULL;
    HKEY                 hKey;
    DWORD                dwRet;
    DWORD                dwFailed = TRUE;
    BOOL                 bForceDeleteAppletExt = FALSE;

    if (DeskIsLegacyDeviceNode2(pDeviceInfoData)) {
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_044);
        return ERROR_DI_DO_DEFAULT;
    }

    //
    // Check the database to see if this is an approved driver for 5.0
    //
    dwRet = DeskCheckDatabase(hDevInfo, 
                              pDeviceInfoData,
                              &bForceDeleteAppletExt);

    TraceMsg(TF_GENERAL, "DeskCheckDatabase returned 0x%x", dwRet);

    if ((dwRet == ERROR_SUCCESS) || bForceDeleteAppletExt) {

        //
        // For now, delete everything under:
        //     REGSTR_PATH_CONTROLSFOLDER TEXT\Display,
        //
        //! TBD:we need a mechanism to link the applet extension with 
        //! the driver, so that when we delete the driver we can delete 
        //! just the corresponding extension.
        //
            
        DeskDeleteLegacyAppletExtension();

    }

    if (dwRet == ERROR_SUCCESS) {
        //
        // It is, no other work is necessary
        //
        TraceMsg(TF_GENERAL, "desk:  DB allows install, disabling legacy DNs");
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_045);
        DeskDisableLegacyDeviceNodes();
        return ERROR_DI_DO_DEFAULT;
    }

    //
    // This particular vid card is not allowed to run with the 5.0 drivers out 
    // of the box.  Note this event in the reg and save off other values.
    // Also, install a fake device onto the devnode so that the user doesn't get
    // PnP popus upon first (real) boot
    //
    TraceMsg(TF_GENERAL, "desk:  DB failed install, marking in registry");
    DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_046);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     SZ_UPDATE_SETTINGS,
                     0,
                     KEY_ALL_ACCESS,
                     &hKey) == ERROR_SUCCESS) {
        // 
        // Save off the fact that upgrade was not allowed (used in migrated 
        // display settings in the display OC
        // 
        dwFailed = TRUE;
        RegSetValueEx(hKey, 
                      SZ_UPGRADE_FAILED_ALLOW_INSTALL,
                      0,
                      REG_DWORD, 
                      (PBYTE) &dwFailed,
                      sizeof(DWORD));
        RegCloseKey(hKey);
    }

    //
    // OK, now grab the description of the device so we can give it to the devnode
    // after a succesfull install of the fake devnode
    //
    ZeroMemory(&did, sizeof(did));
    did.cbSize = sizeof(did);
    if (SetupDiEnumDriverInfo(hDevInfo,
                              pDeviceInfoData,
                              SPDIT_COMPATDRIVER,
                              0,
                              &did)) {
        if (lstrlen(did.Description)) {
            szDesc = did.Description;
        }
        if (lstrlen(did.MfgName)) {
            szMfg = did.MfgName;
        }
    }
    else {
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_047);
    }

    if (!szDesc) {
        szDesc = TEXT("Video Display Adapter");
    }
    if (!szMfg) {
        szMfg = TEXT("Microsoft");
    }

    if ((hKey = SetupDiCreateDevRegKey(hDevInfo,
                                       pDeviceInfoData,
                                       DICS_FLAG_GLOBAL,
                                       0,
                                       DIREG_DEV,
                                       NULL,
                                       NULL)) != INVALID_HANDLE_VALUE) {
        RegSetValueEx(hKey,
                      SZ_UPGRADE_DESCRIPTION,
                      0,
                      REG_SZ,
                      (PBYTE) szDesc, 
                      ByteCountOf(lstrlen(szDesc) + 1));
        RegSetValueEx(hKey,
                      SZ_UPGRADE_MFG,
                      0,
                      REG_SZ,
                      (PBYTE) szMfg, 
                      ByteCountOf(lstrlen(szMfg) + 1));
        RegCloseKey(hKey);
    }
    else {
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_048);
    }

    dip.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if (!SetupDiGetDeviceInstallParams(hDevInfo, pDeviceInfoData, &dip)) {
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_049, GetLastError());
        //
        // BUGBUG clean up?
        //
        return ERROR_DI_DO_DEFAULT;
    }

    dip.Flags |= DI_ENUMSINGLEINF;
    lstrcpy(dip.DriverPath, TEXT("display.inf"));
    if (!SetupDiSetDeviceInstallParams(hDevInfo, pDeviceInfoData, &dip)) {
        //
        // BUGBUG clean up?
        //
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_050, GetLastError());
        return ERROR_DI_DO_DEFAULT;
    }

    //
    // To be safe, make sure there isn't already a class driver list built
    // for this device information element...
    //
    if (!SetupDiDestroyDriverInfoList(hDevInfo, pDeviceInfoData, SPDIT_CLASSDRIVER)) {
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_051);
    }

    //
    // Now build a new one off of display.inf...
    //
    if (!SetupDiBuildDriverInfoList(hDevInfo, pDeviceInfoData, SPDIT_CLASSDRIVER)) {
        // 
        // BUGBUG: uh oh, clean up?
        //
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_052, GetLastError());
        return ERROR_DI_DO_DEFAULT;
    }

    //
    // Now select the fake node...
    //
    // All strings here match the inf fake device entry section.  If the INF is
    // modified in any way WRT to these strings, these to be changed as well
    //
    ZeroMemory(&did, sizeof(SP_DRVINFO_DATA));
    did.cbSize = sizeof(SP_DRVINFO_DATA);
    did.DriverType = SPDIT_CLASSDRIVER;
    DisplayGetUpgradeDeviceStrings(did.Description,
                                   did.MfgName,
                                   did.ProviderName);

    if (!SetupDiSetSelectedDriver(hDevInfo, pDeviceInfoData, &did)) {
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_053);
    }
    else
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_054);

    return NO_ERROR;
}

DWORD
DisplayClassInstaller(
    IN DI_FUNCTION      InstallFunction,
    IN HDEVINFO         hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData OPTIONAL
    )

/*++

Routine Description:

  This routine acts as the class installer for Display devices.

Arguments:

    InstallFunction - Specifies the device installer function code indicating
        the action being performed.

    DeviceInfoSet - Supplies a handle to the device information set being
        acted upon by this install action.

    pDeviceInfoData - Optionally, supplies the address of a device information
        element being acted upon by this install action.

Return Value:

    If this function successfully completed the requested action, the return
        value is NO_ERROR.

    If the default behavior is to be performed for the requested action, the
        return value is ERROR_DI_DO_DEFAULT.

    If an error occurred while attempting to perform the requested action, a
        Win32 error code is returned.

--*/

{

    DWORD err, retVal = ERROR_DI_DO_DEFAULT;
    SP_DRVINFO_DATA DrvInfoData;
    TCHAR szBuffer[LINE_LEN];

    DeskOpenLog();

#if ANDREVA_DBG
    g_dwTraceFlags = 0xFFFFFFFF;
#endif

    switch(InstallFunction) {

    case DIF_SELECTDEVICE : 

        TraceMsg(TF_GENERAL, "DisplayClassInstaller DIF_SELECTDEVICE \n");

        //
        // Check for old display driver infs that we no longer support.
        //

        if (SetupDiBuildDriverInfoList(hDevInfo, NULL, SPDIT_CLASSDRIVER))
        {
            if (!SetupDiEnumDriverInfo(hDevInfo,
                                       NULL,
                                       SPDIT_CLASSDRIVER,
                                       0,
                                       &DrvInfoData))
            {

                if (GetLastError() == ERROR_NO_MORE_ITEMS)
                {
                    HWND hwnd = ghwndPropSheet;  // fallback
                    SP_DEVINSTALL_PARAMS dip;

                    dip.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
                    if (SetupDiGetDeviceInstallParams(hDevInfo, NULL, &dip)) 
                        hwnd = dip.hwndParent;

                    //
                    // This is what happens when an old inf is loaded.
                    // Tell the user it is an old inf.
                    //
                    FmtMessageBox(hwnd,
                                  MB_ICONEXCLAMATION,
                                  ID_DSP_TXT_INSTALL_DRIVER,
                                  ID_DSP_TXT_BAD_INF);

                    retVal = ERROR_NO_MORE_ITEMS;
                    break;
                }
            }
        }

        if (SetupDiBuildDriverInfoList(hDevInfo, pDeviceInfoData, SPDIT_CLASSDRIVER))
        {
            SP_DRVINSTALL_PARAMS DrvInstallParams;
            PSP_DRVINFO_DETAIL_DATA pDrvInfoDetailData = NULL;
            DWORD index = 0, size;
        
            ZeroMemory(&DrvInfoData, sizeof(SP_DRVINFO_DATA));
            DrvInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
        
            size = sizeof(SP_DRVINFO_DETAIL_DATA) + sizeof(TCHAR) * 512;
            pDrvInfoDetailData = (PSP_DRVINFO_DETAIL_DATA) LocalAlloc(LPTR, size);
            if (!pDrvInfoDetailData)
            {
                break; 
            }
            pDrvInfoDetailData->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
        
            SetupDiBuildDriverInfoList(hDevInfo, pDeviceInfoData, SPDIT_CLASSDRIVER);
            
            while (SetupDiEnumDriverInfo(hDevInfo,
                                         pDeviceInfoData,
                                         SPDIT_CLASSDRIVER,
                                         index++,
                                         &DrvInfoData))
            {
        
                if (SetupDiGetDriverInfoDetail(hDevInfo,
                                               pDeviceInfoData,
                                               &DrvInfoData,
                                               pDrvInfoDetailData,
                                               size,
                                               NULL))
                {
                    if (lstrcmpi(pDrvInfoDetailData->HardwareID, TEXT("LEGACY_UPGRADE_ID")) == 0) {
                        ZeroMemory(&DrvInstallParams, sizeof(SP_DRVINSTALL_PARAMS));
                        DrvInstallParams.cbSize = sizeof(SP_DRVINSTALL_PARAMS);
                        if (SetupDiGetDriverInstallParams(hDevInfo,
                                                          pDeviceInfoData,
                                                          &DrvInfoData,
                                                          &DrvInstallParams))
                        {
                            DrvInstallParams.Flags |=  DNF_BAD_DRIVER;
                            
                            SetupDiSetDriverInstallParams(hDevInfo,
                                                          pDeviceInfoData,
                                                          &DrvInfoData,
                                                          &DrvInstallParams);
                        }
                    }
                }

            ZeroMemory(&DrvInfoData, sizeof(SP_DRVINFO_DATA));
            DrvInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
            }
        
            if (pDrvInfoDetailData)
            {
                LocalFree(pDrvInfoDetailData);
            }

        }

        break;

#if 0
        //
        // Set the test parameters for the display class installer.
        //

        ZeroMemory(&SelectDevParams, sizeof(SelectDevParams));

        SelectDevParams.ClassInstallHeader.cbSize
                                     = sizeof(SelectDevParams.ClassInstallHeader);
        SelectDevParams.ClassInstallHeader.InstallFunction
                                     = DIF_SELECTDEVICE;

        //
        // Get current SelectDevice parameters, and then set the fields
        // we want to be different from default
        //

        SetupDiGetClassInstallParams(hDevInfo,
                                     NULL,
                                     &SelectDevParams.ClassInstallHeader,
                                     sizeof(SelectDevParams),
                                     NULL);

        SelectDevParams.ClassInstallHeader.cbSize
                                 = sizeof(SelectDevParams.ClassInstallHeader);
        SelectDevParams.ClassInstallHeader.InstallFunction
                                 = DIF_SELECTDEVICE;

        //
        // Set the strings to use on the select driver page .
        //

        LoadString(ghmod,
                   IDS_DISPLAYINST,
                   SelectDevParams.Title,
                   sizeof(SelectDevParams.Title));

        LoadString(ghmod,
                   IDS_WINNTDEV_INSTRUCT,
                   SelectDevParams.Instructions,
                   sizeof(SelectDevParams.Instructions));

        LoadString(ghmod,
                   IDS_SELECTDEV_LABEL,
                   SelectDevParams.ListLabel,
                   sizeof(SelectDevParams.ListLabel));

        SetupDiSetClassInstallParams(hDevInfo,
                                     NULL,
                                     &SelectDevParams.ClassInstallHeader,
                                     sizeof(SelectDevParams));

        //
        // Let the wizard finish it's thing
        //

        break;

#endif



    case DIF_FIRSTTIMESETUP :

        TraceMsg(TF_GENERAL, "DisplayClassInstaller DIF_FIRSTTIMESETUP \n");

        ASSERT(pDeviceInfoData == NULL);

        if (!DeskAllowFirstTimeSetup()) {
            retVal = NO_ERROR;
            break;
        }

        err = DeskDetectDevice(hDevInfo,
                               NULL,
                               NULL,
                               &DrvInfoData);

        if (err == ERROR_DEV_NOT_EXIST)
        {
            retVal = NO_ERROR;
            break;
        }
        else if (err == NO_ERROR)
        {
            SP_DEVINFO_DATA      DeviceInfoData;
            SP_DEVINSTALL_PARAMS DeviceInstallParams;
            SP_DRVINFO_DATA      NewDrvInfoData = DrvInfoData;

            //
            // Create the DeviceInfoData for this new device.
            //

            TraceMsg(TF_GENERAL, " FIRSTTIME Creating DeviceInfo\n");

            ZeroMemory(&DeviceInfoData, sizeof(DeviceInfoData));
            DeviceInfoData.cbSize = sizeof(DeviceInfoData);

            if (!SetupDiCreateDeviceInfo(hDevInfo,
                                         TEXT("Display"),
                                         (LPGUID) &GUID_DEVCLASS_DISPLAY,
                                         NULL,
                                         NULL,
                                         DICD_GENERATE_ID,
                                         &DeviceInfoData))
            {
                err = GetLastError();
                TraceMsg(TF_GENERAL, " FIRSTTIME CreateDeviceInfo failed : error = %d\n",err);
                ASSERT(FALSE);
                retVal = err;
                break;
            }

            //
            // Set the inf name to display.inf
            //
            // Also copy the files silently since we are in setup.
            //

            ZeroMemory(&DeviceInstallParams, sizeof(DeviceInstallParams));
            DeviceInstallParams.cbSize = sizeof(DeviceInstallParams);

            if (!SetupDiGetDeviceInstallParams(hDevInfo,
                                               &DeviceInfoData,
                                               &DeviceInstallParams))
            {
                err = GetLastError();
                TraceMsg(TF_GENERAL, " FIRSTTIME GetDeviceInstall : error = %d\n",err);
                ASSERT(FALSE);
            }
            else
            {
                DeviceInstallParams.Flags |= DI_ENUMSINGLEINF | DI_QUIETINSTALL;

                lstrcpy(DeviceInstallParams.DriverPath, TEXT("display.inf"));

                if (!SetupDiSetDeviceInstallParams(hDevInfo,
                                                   &DeviceInfoData,
                                                   &DeviceInstallParams))
                {
                    err = GetLastError();
                    TraceMsg(TF_GENERAL, " FIRSTTIME SetDeviceInstall : error = %d\n",err);
                    ASSERT(FALSE);
                }
                else
                {
                    //
                    // Build the driver list
                    //

                    if (!SetupDiBuildDriverInfoList(hDevInfo,
                                                    &DeviceInfoData,
                                                    SPDIT_CLASSDRIVER))
                    {
                        err = GetLastError();
                        TraceMsg(TF_GENERAL, " FIRSTTIME SetupDiBuildDriverInfoList Error %d\n", err);
                        ASSERT(FALSE);
                    }
                    else
                    {
                        //
                        // Find the same driver in display.inf
                        //

                        NewDrvInfoData.Reserved = 0;

                        if (!SetupDiSetSelectedDriver(hDevInfo,
                                                      &DeviceInfoData,
                                                      &NewDrvInfoData))
                        {
                            err = GetLastError();
                            TraceMsg(TF_GENERAL, " FIRSTTIME display.inf dispdet.inf inconsistent = %d\n", err);
                            ASSERT(FALSE);
                        }
                        else
                        {
                            //
                            // How do we finish the install ... ?
                            //
                            // Install the service and run it.  The video port will call
                            // IoReportDetectedDevice, if the miniport is a PnP miniport.
                            //
                            // BUGBUG this should really be done by just passing a parameter
                            // in the device infoset, so that we just get called with
                            // DIF_INSTALLDEVICE.
                            //

                            if (!SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES,
                                                           hDevInfo,
                                                           &DeviceInfoData))
                            {
                                err = GetLastError();
                                TraceMsg(TF_GENERAL, " FIRSTTIME install of driver files failed = %d\n", err);
                                ASSERT(FALSE);
                            }
                            else
                            {
                                err = DeskInstallService(hDevInfo,
                                                         &DeviceInfoData,
                                                         szBuffer,
                                                         REPORT_DEVICE);

                                if (err == NO_ERROR)
                                {
                                    err = DeskSCStartService(szBuffer);
                                }
                            }
                        }
                    }
                }
            }

            //
            // Remove the device info since we don't want to install
            // it later on.
            //

            SetupDiDeleteDeviceInfo(hDevInfo,
                                    &DeviceInfoData);
        }

        retVal = err;
        break;


#if LATER

    case DIF_DETECT :

        TraceMsg(TF_GENERAL, "DisplayClassInstaller DIF_DETECT \n");

        ASSERT(pDeviceInfoData == NULL);

        //
        // Get the progress bar parameter for detection
        //

        ZeroMemory(&DetectDevParams, sizeof(DetectDevParams));

        DetectDevParams.ClassInstallHeader.cbSize
                                     = sizeof(DetectDevParams.ClassInstallHeader);
        DetectDevParams.ClassInstallHeader.InstallFunction
                                     = DIF_DETECT;

        SetupDiGetClassInstallParams(hDevInfo,
                                     NULL,
                                     &DetectDevParams.ClassInstallHeader,
                                     sizeof(DetectDevParams),
                                     NULL);

        err = DeskDetectDevice(hDevInfo,
                               DetectDevParams.DetectProgressNotify,
                               DetectDevParams.ProgressNotifyParam,
                               &DrvInfoData);


        retVal = err;
        break;
#endif


    case DIF_REGISTERDEVICE :


        TraceMsg(TF_GENERAL, "DisplayClassInstaller DIF_REGISTERDEVICE \n");

        if (!SetupDiRegisterDeviceInfo(hDevInfo,
                                       pDeviceInfoData,
                                       0,
                                       NULL,
                                       NULL,
                                       NULL))
        {
            err = GetLastError();
            TraceMsg(TF_GENERAL, "DIF_REGISTERDEVICE failed : error = %d\n",err);
            retVal = err;
            break;
        }

        retVal = NO_ERROR;
        break;

    case DIF_SELECTBESTCOMPATDRV :

        if (DeskGetDeviceNodePath(pDeviceInfoData,
                                  szBuffer,
                                  sizeof(szBuffer)/sizeof(TCHAR))) {
            DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_055, szBuffer);
        }
        retVal = DeskSelectBestCompatDrv(hDevInfo, pDeviceInfoData);
        break;

    case DIF_ALLOW_INSTALL :

        TraceMsg(TF_GENERAL, "DisplayClassInstaller DIF_ALLOW_INSTALL \n");
        if (DeskGetDeviceNodePath(pDeviceInfoData,
                                  szBuffer,
                                  sizeof(szBuffer)/sizeof(TCHAR))) {
            DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_056, szBuffer);
        }

        //
        // Only allow installation if this is a DEVNODE we don't have a legacy
        // driver for already.
        //

        retVal =  DeskAllowInstall(hDevInfo, pDeviceInfoData);
        break;

    case DIF_INSTALLDEVICE :

        TraceMsg(TF_GENERAL, "DisplayClassInstaller DIF_INSTALLDEVICE \n");

        {
            DISPLAY_DEVICE displayDevice;
            displayDevice.cb = sizeof(DISPLAY_DEVICE);

            err = DeskInstallService(hDevInfo,
                                     pDeviceInfoData,
                                     szBuffer,
                                     0);  // not a detect install

            if ((err == ERROR_NO_DRIVER_SELECTED) &&
                (bSetupFlags() & INSETUP_UPGRADE) &&
                pDeviceInfoData &&
                DeskIsLegacyDeviceNode2(pDeviceInfoData)) {
                
                //
                // If this is a legacy device and no driver is selected,
                // let the default handler install a NULL driver.
                //
                
                err = ERROR_DI_DO_DEFAULT;
            }
                
            //
            // Calling EnumDisplayDevices will rescan the devices, and if a
            // new device is detected, we will disable and reenable the main
            // device.
            // This reset of the display device will clear up any mess caused
            // by installing a new driver
            //

            EnumDisplayDevices(NULL, 0, &displayDevice, 0);
        }

        retVal = err;
        break;


    case DIF_REMOVE :
        TraceMsg(TF_GENERAL, "desk:  DIF_REMOVE called\n");

    default :

            TraceMsg(TF_GENERAL, "DisplayClassInstaller default function = 0x%08lx\n", (DWORD) InstallFunction);

        break;
    }

    DeskLogError(LogSevInformation, 
                 IDS_SETUPLOG_MSG_057, 
                 retVal, 
                 InstallFunction);

    //
    // If we did not exit from the routine by handling the call, tell the
    // setup code to handle everything the default way.
    //
    DeskCloseLog();
    return retVal;
}


DWORD
MonitorClassInstaller(
    IN DI_FUNCTION      InstallFunction,
    IN HDEVINFO         hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData OPTIONAL
    )

/*++

Routine Description:

  This routine acts as the class installer for Display devices.

Arguments:

    InstallFunction - Specifies the device installer function code indicating
        the action being performed.

    DeviceInfoSet - Supplies a handle to the device information set being
        acted upon by this install action.

    pDeviceInfoData - Optionally, supplies the address of a device information
        element being acted upon by this install action.

Return Value:

    If this function successfully completed the requested action, the return
        value is NO_ERROR.

    If the default behavior is to be performed for the requested action, the
        return value is ERROR_DI_DO_DEFAULT.

    If an error occurred while attempting to perform the requested action, a
        Win32 error code is returned.

--*/

{


    TraceMsg(TF_GENERAL, " MonitorClassInstaller function = %d\n", (DWORD) InstallFunction);

    //
    // If we did not exit from the routine by handling the call, tell the
    // setup code to handle everything the default way.
    //

    return ERROR_DI_DO_DEFAULT;
}




/***************************************************************************/
/***************************************************************************/
/**                                                                       **/
/**                           Internal setup call                         **/
/**                                                                       **/
/***************************************************************************/
/***************************************************************************/




DWORD
InstallNewDriver(
    HWND    hwnd,
    LPCTSTR pszModel,
    PBOOL   pbKeepEnabled
    )
{
    DWORD err;
    BOOL bStatus;
    LPTSTR keyName;
    DWORD reboot;

    HMODULE hModule;
    PINSTALLNEWDEVICE InstallNewDevice;

    hModule = LoadLibrary(TEXT("newdev.dll"));

    if (hModule)
    {
        InstallNewDevice = (PINSTALLNEWDEVICE)GetProcAddress(hModule, "InstallNewDevice");

        if (InstallNewDevice)
        {
            bStatus = (*InstallNewDevice)(hwnd, (LPGUID) &GUID_DEVCLASS_DISPLAY, &reboot);

        }
    }

    if (bStatus)
    {
        DWORD disposition;
        HKEY hkey;

        *pbKeepEnabled = KeepEnabled;

        //
        // Tell the user the driver was installed.
        //

        FmtMessageBox(hwnd,
                      MB_ICONINFORMATION | MB_OK,
                      ID_DSP_TXT_INSTALL_DRIVER,
                      ID_DSP_TXT_DRIVER_INSTALLED);

        if (PreConfigured == 0)
        {
            //
            // Create a registry key that indicates a new display was
            // installed.
            //

            keyName = SZ_NEW_DISPLAY;

            if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                               keyName,
                               0,
                               NULL,
                               REG_OPTION_NON_VOLATILE,
                               KEY_READ | KEY_WRITE,
                               NULL,
                               &hkey,
                               &disposition) == ERROR_SUCCESS)
            {
                RegCloseKey(hkey);
            }
        }


        //
        // Save into the registry the fact that it will take a reboot
        // for these changes to take effect
        //

        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                           SZ_REBOOT_NECESSARY,
                           0,
                           NULL,
                           REG_OPTION_VOLATILE,
                           KEY_READ | KEY_WRITE,
                           NULL,
                           &hkey,
                           &disposition) == ERROR_SUCCESS)
        {
            RegCloseKey(hkey);
        }

        //
        // If everything succeeded properly, then we need to reboot the
        // machine
        //
        // Ask the user to reboot the machine.
        //

        PropSheet_RestartWindows(ghwndPropSheet);
        PropSheet_CancelToClose(ghwndPropSheet);

    }
    else
    {
        err = GetLastError();

        if (err != ERROR_CANCELLED)
        {
            //
            // Tell the user the driver was not installed properly.
            //

            FmtMessageBox(hwnd,
                          MB_ICONSTOP | MB_OK,
                          ID_DSP_TXT_INSTALL_DRIVER,
                          ID_DSP_TXT_DRIVER_INSTALLED_FAILED);
        }
    }

    return err;
}
