#include "precomp.h"
#pragma hdrstop

#include <tchar.h>
#include <initguid.h>
#include <devguid.h>


// temporary extern defition from newdev.c

typedef
BOOL
(*PINSTALLNEWDEVICE)(
   HWND hwndParent,
   LPGUID ClassGuid,
   PDWORD Reboot
   );


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



#define LEGACY_DETECT 1
#define REPORT_DEVICE 2



/***************************************************************************/
/***************************************************************************/
/**                                                                       **/
/**                      Service Controller stuff                         **/
/**                                                                       **/
/***************************************************************************/
/***************************************************************************/




VOID
DeskSCSetServiceDemandStart(
    LPTSTR ServiceName
    )
{
    SC_HANDLE SCMHandle;
    SC_HANDLE ServiceHandle;
    ULONG     Attempts;
    SC_LOCK   SCLock = NULL;

    ULONG                  ServiceConfigSize = 0;
    LPQUERY_SERVICE_CONFIG ServiceConfig;

    TraceMsg(TF_GENERAL, "DeskSCSetServiceDemandStart for service %ws called\n",
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
            TraceMsg(TF_GENERAL, "DeskSCSetServiceDemandStart SC Managr handles opended\n");

            QueryServiceConfig(ServiceHandle,
                               NULL,
                               0,
                               &ServiceConfigSize);

            ASSERT(GetLastError() == ERROR_INSUFFICIENT_BUFFER);

            TraceMsg(TF_GENERAL, "DeskSCSetServiceDemandStart buffer size = %d\n", ServiceConfigSize);

            if (ServiceConfig = (LPQUERY_SERVICE_CONFIG)
                                 LocalAlloc(LPTR, ServiceConfigSize))
            {
                if (QueryServiceConfig(ServiceHandle,
                                       ServiceConfig,
                                       ServiceConfigSize,
                                       &ServiceConfigSize))
                {
                    TraceMsg(TF_GENERAL, "DeskSCSetServiceDemandStart Queried Config info\n");

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
                                            SERVICE_DEMAND_START,
                                            SERVICE_NO_CHANGE,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL))
                    {
                        TraceMsg(TF_GENERAL, "DeskSCSetServiceDemandStart SC manager succeeded\n");
                    }
                    else
                    {
                        TraceMsg(TF_GENERAL, "DeskSCSetServiceDemandStart SC manager failed : %d\n",
                                        GetLastError());
                    }

                    if (SCLock)
                    {
                        TraceMsg(TF_GENERAL, "DeskSCSetServiceDemandStart Unlock database\n");
                        UnlockServiceDatabase(SCLock);
                    }
                }

                LocalFree(ServiceConfig);
            }


            CloseServiceHandle(ServiceHandle);
        }

        CloseServiceHandle(SCMHandle);
    }

    TraceMsg(TF_GENERAL, "DeskSCSetServiceDemandStart Leave\n");
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


BOOL
DeskIsPnPDriver(
    IN HDEVINFO         hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData OPTIONAL
    )
{
    SP_DRVINFO_DATA        DriverInfoData;
    SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;
    DWORD                  cbOutputSize = 0;
    HANDLE                 InfFileHandle;
    DWORD                  PnPDriver = 0;
    TCHAR                  szSoftwareSection[LINE_LEN];
    INFCONTEXT             tmpContext;

    TraceMsg(TF_GENERAL, "DeskIsPnPDriver\n");

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


    //
    // open the inf so we can run the sections in the inf, more or less
    // manually.
    //

    InfFileHandle = SetupOpenInfFile(DriverInfoDetailData.InfFileName,
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

    wsprintf(szSoftwareSection,
             TEXT("%ws.GeneralConfigData"),
             DriverInfoDetailData.SectionName);


    if (SetupFindFirstLine(InfFileHandle,
                           szSoftwareSection,
                           TEXT("PnPEnabled"),
                           &tmpContext))
    {
        SetupGetIntField(&tmpContext,
                         1,
                         &PnPDriver);
    }

    TraceMsg(TF_GENERAL, "DeskIsPnPDriver returns %d\n", (PnPDriver != 0));

    return (PnPDriver != 0);
}




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
    PVOID      Context;
    INFCONTEXT tmpContext;

    TCHAR szSoftwareSection[LINE_LEN];

    ULONG maxmem;
    ULONG numDev;
    ULONG PnPDriver;
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
    PnPDriver = 0;

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

    if (SetupFindFirstLine(InfFileHandle,
                           szSoftwareSection,
                           TEXT("PnPEnabled"),
                           &tmpContext))
    {
        SetupGetIntField(&tmpContext,
                         1,
                         &PnPDriver);
    }


    //
    // Write the PnP and Detect configuration information to the registry.
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

        if (PnPDriver)
        {
            RegSetValueEx(hkey,
                          TEXT("PnPEnabled"),
                          0,
                          REG_DWORD,
                          (LPBYTE) (&Value),
                          SIZEOF(ULONG) );
        }

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

        maxmem *= 0x400 * 3/2;

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

    //
    // Initialize the default callback so that the MsgHandler works
    // properly
    //

    Context = SetupInitDefaultQueueCallback(hwnd);

    bSetSoftwareKey = TRUE;

    do {

        if (PnPDriver && bSetSoftwareKey)
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
                                            &SetupDefaultQueueCallback,
                                            Context,
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

            if ((PnPDriver && bSetSoftwareKey) == FALSE)
            {
                RegSetValueEx(hkey,
                              TEXT("Device Description"),
                              0,
                              REG_SZ,
                              (LPBYTE) DriverInfoDetailData->DrvDescription,
                              (lstrlen(DriverInfoDetailData->DrvDescription) + 1) *
                                   sizeof(TCHAR) );
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
                                   &SetupDefaultQueueCallback,
                                   Context,
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

            DeskSCSetServiceDemandStart(pServiceName);
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

    Context = SetupInitDefaultQueueCallback(DeviceInstallParams.hwndParent);

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
            _tcscpy(pDrvInfoData->Description,  DeviceDescription);
            _tcscpy(pDrvInfoData->MfgName,      Manufacturer);
            _tcscpy(pDrvInfoData->ProviderName, Provider);

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

                        if (_tcscmp(DetectBroken, TEXT("detect_broken")) == 0)
                        {
                            TraceMsg(TF_GENERAL, "DeskDetectDevice detect_broken - RESET_SCREEN\n");
                            ChangeDisplaySettings(NULL, CDS_RESET);
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


LPTSTR UpdateDatabase[] = {
    TEXT("Root\\LEGACY_UPGRADE_THIS_DRIVER\\0000"),
    NULL,
};


BOOL
DeskCheckInstallDatabase(
    IN HDEVINFO         hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData
    )
{
    BOOL            bRet = TRUE;
    HDEVINFO        HDevInfo;
    SP_DEVINFO_DATA DeviceInfoData;
    DWORD           index = 0;
    TCHAR           RegistryProperty[256];
    ULONG           BufferSize;

#if 0
    HKEY            hKey;
    //
    // BUGBUG this does not work.
    // It appears to always succeed
    //

    //
    // First. let's check if we have a driver key.
    // If we do, then we can control the upgrade easily.
    //

    hKey = SetupDiOpenDevRegKey(hDevInfo,
                                pDeviceInfoData,
                                DICS_FLAG_GLOBAL,
                                0,
                                DIREG_DRV,
                                KEY_READ | KEY_WRITE);

    if (hKey != INVALID_HANDLE_VALUE)
    {
        //
        // If this succeeds, then we can safely upgrade the driver since
        // it is an existing, properly installed driver.
        //

        TraceMsg(TF_GENERAL, "Database - driver key exists\n");

        RegCloseKey(hKey);

        return TRUE;
    }
#endif

    //
    // Let's find all the video drivers that are installed in the system
    //

    HDevInfo = SetupDiGetClassDevs((LPGUID) &GUID_DEVCLASS_DISPLAY,
                                   NULL,
                                   NULL,
                                   0);

    if (HDevInfo == INVALID_HANDLE_VALUE)
    {
        TraceMsg(TF_GENERAL, "Database - no existing driver\n");
        return TRUE;
    }

    while (bRet)
    {
        ZeroMemory(&DeviceInfoData, sizeof(SP_DEVINFO_DATA));
        DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        if (!SetupDiEnumDeviceInfo(HDevInfo,
                                   index++,
                                   &DeviceInfoData))
        {
            TraceMsg(TF_GENERAL, "Database retrieving device Error %d\n", GetLastError());
            break;
        }

        //
        // For each driver we found, compare it to the driver we are being
        // asked to install.
        //

        //
        // If we have a root legacy device, then don't install any new
        // devnode (until we get better at this).
        //


        if (CR_SUCCESS == CM_Get_Device_ID(DeviceInfoData.DevInst,
                                           RegistryProperty,
                                           sizeof(RegistryProperty),
                                           0))
        {
            LPTSTR *pUpgrade = UpdateDatabase;

            TraceMsg(TF_GENERAL, "\t CM_Get_Device_ID = %ws\n", RegistryProperty);

            if (_tcsncicmp(TEXT("ROOT\\LEGACY_"),
                           RegistryProperty,
                           sizeof(TEXT("ROOT\\LEGACY_"))) == 0)
            {
                TraceMsg(TF_GENERAL, "\t Legacy DEVNODE = %ws\n", RegistryProperty);

                //
                // We have a legacy DEVNODE.
                // By default, we don't want to upgrade any of these devices.
                //
                // Check if it's part of our database that we should upgrade.
                //

                bRet = FALSE;

                while (*pUpgrade)
                {
                    if (_tcsicmp(*pUpgrade, RegistryProperty) == 0)
                    {
                        TraceMsg(TF_GENERAL, "\t Upgrade DEVNODE = %ws\n", RegistryProperty);

                        bRet = TRUE;
                        break;
                    }

                    pUpgrade++;
                }
            }

        }

        //
        // Print out other data for informational purposes.
        //

        BufferSize = sizeof(RegistryProperty);

        if (CR_SUCCESS ==
                CM_Get_DevNode_Registry_Property(DeviceInfoData.DevInst,
                                                 CM_DRP_DEVICEDESC,
                                                 NULL,
                                                 RegistryProperty,
                                                 &BufferSize,
                                                 0))
        {
            TraceMsg(TF_GENERAL, "\t CM_DRP_DEVICEDESC = %ws\n", RegistryProperty);
        }

        BufferSize = sizeof(RegistryProperty);

        if (CR_SUCCESS ==
                CM_Get_DevNode_Registry_Property(DeviceInfoData.DevInst,
                                                 CM_DRP_SERVICE,
                                                 NULL,
                                                 RegistryProperty,
                                                 &BufferSize,
                                                 0))
        {
            TraceMsg(TF_GENERAL, "\t CM_DRP_SERVICE = %ws\n", RegistryProperty);
        }
    }

    return bRet;
}

BOOL
bDoLegacyDetection(VOID)
{
    BOOL   bRet;
    HKEY   hkey;
    DWORD  bSetupInProgress = 0;
    DWORD  bUpgradeInProgress = 0;
    TCHAR  data[256];
    DWORD  cb;
    LPTSTR regstring;

    // ULONG  cCount;
    // cCount = GetPrivateProfileString(TEXT("data"),
    //                                  TEXT("winntupgrade"),
    //                                  TEXT("missing"),
    //                                  data,
    //                                  256,
    //                                  TEXT(".\\$winnt$.inf"));


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
            bSetupInProgress = * ((LPDWORD)(data));
        }

        cb = 256;

        if (RegQueryValueEx(hkey,
                            TEXT("UpgradeInProgress"),
                            NULL,
                            NULL,
                            (LPBYTE)(data),
                            &cb) == ERROR_SUCCESS)
        {
            bUpgradeInProgress = * ((LPDWORD)(data));
        }

        if (hkey) {
            RegCloseKey(hkey);
        }
    }

    if (bSetupInProgress)
    {
        if (bUpgradeInProgress)
        {
            regstring = TEXT("System\\Video_Upgrade");
            TraceMsg(TF_GENERAL, "/t/tUpgrade\n");
            bRet = FALSE;
        }
        else
        {
            //
            // We only do installation or detection if not BASEVIDEO
            //

            cb = 256;
            hkey = NULL;

            if ( (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                               TEXT("System\\CurrentControlSet\\Control\\Setup"),
                               0,
                               KEY_READ | KEY_WRITE,
                               &hkey) == ERROR_SUCCESS)     &&
                 (RegQueryValueEx(hkey,
                                  TEXT("video"),
                                  NULL,
                                  NULL,
                                  (LPBYTE)(data),
                                  &cb) == ERROR_SUCCESS)    &&
                 (_tcscmp(data, TEXT("forcevga")) == 0)
               )
            {
                regstring = TEXT("System\\Basevideo_Winnt");
                TraceMsg(TF_GENERAL, "/t/tbasevideo install\n");
                bRet = FALSE;
            }
            else
            {
                regstring = TEXT("System\\Video_Clean");
                TraceMsg(TF_GENERAL, "/t/tCleanInstall\n");
                bRet = TRUE;
            }

            if (hkey) {
                RegCloseKey(hkey);
            }
        }
    }
    else
    {
        regstring = TEXT("System\\Video_Missing_Winnt");
        TraceMsg(TF_GENERAL, "/t/tMissing $Winnt$.inf file\n");
        bRet = TRUE;
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

    return bRet;
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

    DWORD err;
    SP_DRVINFO_DATA DrvInfoData;
    // SP_SELECTDEVICE_PARAMS  SelectDevParams;
    // SP_DETECTDEVICE_PARAMS  DetectDevParams;
    TCHAR ServiceName[LINE_LEN];


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
                    //
                    // This is what happens when an old inf is loaded.
                    // Tell the user it is an old inf.
                    //

                    FmtMessageBox(ghwndPropSheet,   // BUGBUG
                                  MB_ICONEXCLAMATION,
                                  FALSE,
                                  ID_DSP_TXT_INSTALL_DRIVER,
                                  ID_DSP_TXT_BAD_INF);

                    return ERROR_NO_MORE_ITEMS;
                }
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

        //
        // Don't do video driver detection during upgrade.
        //

        if (bDoLegacyDetection() == FALSE)
        {
            return NO_ERROR;
        }


        err = DeskDetectDevice(hDevInfo,
                               NULL,
                               NULL,
                               &DrvInfoData);

        if (err == NO_ERROR)
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
                return err;
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
                return err;
            }

            DeviceInstallParams.Flags |= DI_ENUMSINGLEINF | DI_QUIETINSTALL;

            lstrcpy(DeviceInstallParams.DriverPath, TEXT("display.inf"));

            if (!SetupDiSetDeviceInstallParams(hDevInfo,
                                               &DeviceInfoData,
                                               &DeviceInstallParams))
            {
                err = GetLastError();
                TraceMsg(TF_GENERAL, " FIRSTTIME SetDeviceInstall : error = %d\n",err);
                return err;
            }

            //
            // Build the driver list
            //

            if (!SetupDiBuildDriverInfoList(hDevInfo,
                                            &DeviceInfoData,
                                            SPDIT_CLASSDRIVER))
            {
                err = GetLastError();
                DbgPrint("Desk.cpl: FIRSTTIME SetupDiBuildDriverInfoList Error %d\n", err);
                return err;
            }

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
                return err;
            }


            //
            // How do we finish the install ... ?
            //
            // If this is a PnP device, just install the service and
            // run it.  The video port will call IoReportDetectedDevice
            // which will later complete the installation of the deivce
            //
            // For non-PnP drivers, we can do nothing at this point, and a
            // DEVNODE will be created.
            //

            if (DeskIsPnPDriver(hDevInfo, NULL))
            {
                if (!SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES,
                                               hDevInfo,
                                               &DeviceInfoData))
                {
                    err = GetLastError();
                    TraceMsg(TF_GENERAL, " FIRSTTIME install of driver files failed = %d\n", err);
                }
                else
                {
                    err = DeskInstallService(hDevInfo,
                                             &DeviceInfoData,
                                             ServiceName,
                                             REPORT_DEVICE);

                    if (err == NO_ERROR)
                    {
                        err = DeskSCStartService(ServiceName);
                    }
                }

                //
                // Remove the device info since we don't want to install
                // it later on.
                //

                SetupDiDeleteDeviceInfo(hDevInfo,
                                        &DeviceInfoData);
            }
            else
            {
                //
                // We will get called later to register and install this
                // device if it is a TRUE legacy DEVNODE...
                //
            }
        }

        return err;


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


        return err;
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
            return err;
        }

        return NO_ERROR;


    case DIF_ALLOW_INSTALL :

        TraceMsg(TF_GENERAL, "DisplayClassInstaller DIF_ALLOW_INSTALL \n");

        //
        // Only allow installation if this is a DEVNODE we don't have a legacy
        // driver for already.
        //

        if (DeskCheckInstallDatabase(hDevInfo,
                                     pDeviceInfoData) == FALSE)
        {
            return ERROR_DI_DONT_INSTALL;
        }

        return ERROR_DI_DO_DEFAULT;


    case DIF_INSTALLDEVICE :

        TraceMsg(TF_GENERAL, "DisplayClassInstaller DIF_INSTALLDEVICE \n");

        {
            DISPLAY_DEVICE displayDevice;
            displayDevice.cb = sizeof(DISPLAY_DEVICE);

            err = DeskInstallService(hDevInfo,
                                     pDeviceInfoData,
                                     ServiceName,
                                     0);  // not a detect install

            //
            // Calling EnumDisplayDevices will rescan the devices, and if a
            // new device is detected, we will disable and reenable the main
            // device.
            // This reset of the display device will clear up any mess caused
            // by installing a new driver
            //

            EnumDisplayDevices(NULL, 0, &displayDevice, 0);
        }

        return err;


    case DIF_REMOVE :

    default :

        TraceMsg(TF_GENERAL, "DisplayClassInstaller default function = %d\n", (DWORD) InstallFunction);

        break;
    }

    //
    // If we did not exit from the routine by handling the call, tell the
    // setup code to handle everything the default way.
    //

    return ERROR_DI_DO_DEFAULT;
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
                      FALSE,
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
                          FALSE,
                          ID_DSP_TXT_INSTALL_DRIVER,
                          ID_DSP_TXT_DRIVER_INSTALLED_FAILED);
        }
    }

    return err;
}
