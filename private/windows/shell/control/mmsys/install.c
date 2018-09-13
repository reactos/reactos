/*************************************************************************
 *
 *  INSTALL.C
 *
 *  Copyright (C) Microsoft, 1991, All Rights Reserved.
 *
 *  History:
 *
 *      Thu Oct 17 1991 -by- Sanjaya
 *      Created. Culled out of drivers.c
 *
 *************************************************************************/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <winsvc.h>
#include <memory.h>
#include <string.h>
#include <cpl.h>
#include <regstr.h>
#include <infstr.h>
#include <cphelp.h>
#include <stdlib.h>
#include "drivers.h"
#include "sulib.h"
#include "debug.h"

BOOL     GetValidAlias           (LPTSTR, LPTSTR);
BOOL     SelectInstalled         (HWND, PIDRIVER, LPTSTR, HDEVINFO, PSP_DEVINFO_DATA);
void     InitDrvConfigInfo       (LPDRVCONFIGINFO, PIDRIVER );
BOOL     InstallDrivers          (HWND, HWND, LPTSTR);
void     RemoveAlreadyInstalled  (LPTSTR, LPTSTR);
void     CheckIniDrivers         (LPTSTR, LPTSTR);
void     RemoveDriverParams      (LPTSTR, LPTSTR);

void     InsertNewIDriverNodeInList(PIDRIVER *, PIDRIVER);
void     DestroyIDriverNodeList(PIDRIVER, BOOL, BOOL);


/*
 ***************************************************************
 * Global strings
 ***************************************************************
 */
CONST TCHAR gszDriversSubkeyName[] = TEXT("Drivers");
CONST TCHAR gszSubClassesValue[]   = TEXT("SubClasses");
CONST TCHAR gszDescriptionValue[]  = TEXT("Description");
CONST TCHAR gszDriverValue[]       = TEXT("Driver");
CONST TCHAR gszAliasValue[]        = TEXT("Alias");


/**************************************************************************
 *
 *  InstallDrivers()
 *
 *  Install a driver and set of driver types.
 *
 *  Parameters :
 *      hwnd      - Window handle of the main drivers.cpl windows
 *      hwndAvail - Handle of the 'available drivers' dialog window
 *      pstrKey   - Key name of the inf section item we are installing
 *
 *  This routine calls itself recursively to install related drivers
 *  (as listed in the .inf file).
 *
 **************************************************************************/

BOOL InstallDrivers(HWND hWnd, HWND hWndAvail, LPTSTR pstrKey)
{
    IDRIVER     IDTemplate; // temporary for installing, removing, etc.
    PIDRIVER    pIDriver=NULL;
    int         n;
    TCHAR        szTypes[MAXSTR];
    TCHAR        szType[MAXSTR];
    TCHAR        szParams[MAXSTR];

    szTypes[0] = TEXT('\0');

    hMesgBoxParent = hWndAvail;

    /*
     * mmAddNewDriver needs a buffer for all types we've actually installed
     * User critical errors will pop up a task modal
     */

    IDTemplate.bRelated = FALSE;
    IDTemplate.szRemove[0] = TEXT('\0');

    /*
     *  Do the copying and extract the list of types (WAVE, MIDI, ...)
     *  and the other driver data
     */

    if (!mmAddNewDriver(pstrKey, szTypes, &IDTemplate))
        return FALSE;

    szTypes[lstrlen(szTypes)-1] = TEXT('\0');         // Remove space left at end

    RemoveAlreadyInstalled(IDTemplate.szFile, IDTemplate.szSection);

    /*
     *  At this point we assume the drivers were actually copied.
     *  Now we need to add them to the installed list.
     *  For each driver type we create an IDRIVER and add to the listbox
     */

    for (n = 1; infParseField(szTypes, n, szType); n++)
    {
        /*
         *  Find a valid alias for this device (eg Wave2).  This is
         *  used as the key in the [MCI] or [drivers] section.
         */

        if (GetValidAlias(szType, IDTemplate.szSection) == FALSE)
        {
            /*
             *  Exceeded the maximum, tell the user
             */

            LPTSTR pstrMessage;
            TCHAR szApp[MAXSTR];
            TCHAR szMessage[MAXSTR];

            LoadString(myInstance,
                       IDS_CONFIGURE_DRIVER,
                       szApp,
                       sizeof(szApp)/sizeof(TCHAR));

            LoadString(myInstance,
                       IDS_TOO_MANY_DRIVERS,
                       szMessage,
                       sizeof(szMessage)/sizeof(TCHAR));

            if (NULL !=
                (pstrMessage =
                 (LPTSTR)LocalAlloc(LPTR,
                                    sizeof(szMessage) + (lstrlen(szType)*sizeof(TCHAR)))))
            {
                wsprintf(pstrMessage, szMessage, (LPTSTR)szType);

                MessageBox(hWndAvail,
                           pstrMessage,
                           szApp,
                           MB_OK | MB_ICONEXCLAMATION|MB_TASKMODAL);

                LocalFree((HANDLE)pstrMessage);
            }
            continue;
        }

        if ( (pIDriver = (PIDRIVER)LocalAlloc(LPTR, sizeof(IDRIVER))) != NULL)
        {
            /*
             *  Copy all fields
             */

            memcpy(pIDriver, &IDTemplate, sizeof(IDRIVER));
            wcsncpy(pIDriver->szAlias, szType, sizeof(pIDriver->szAlias)/sizeof(TCHAR));
            pIDriver->szAlias[sizeof(pIDriver->szAlias)/sizeof(TCHAR) - 1] = TEXT('\0');
            wcscpy(pIDriver->wszAlias, pIDriver->szAlias);


            /*
             *  Want only one instance of each driver to show up in the list
             *  of installed drivers. Thus for the remaining drivers just
             *  place an entry in the drivers section of system.ini
             */


            if ( n > 1)
            {


                if (wcslen(szParams) != 0 && !pIDriver->KernelDriver)
                {
                    /*
                     *  Write their parameters to a section bearing their
                     *  file name with an alias reflecting their alias
                     */

                    WriteProfileString(pIDriver->szFile,
                                       pIDriver->szAlias,
                                       szParams);
                }

                WritePrivateProfileString(pIDriver->szSection,
                                          pIDriver->szAlias,
                                          pIDriver->szFile,
                                          szSysIni);
            }
            else
            {


                /*
                 *  Reduce to just the driver name
                 */

                RemoveDriverParams(pIDriver->szFile, szParams);

                wcscpy(pIDriver->wszFile, pIDriver->szFile);

                if (wcslen(szParams) != 0 && !pIDriver->KernelDriver)
                {
                    /*
                     *  Write their parameters to a section bearing their
                     *  file name with an alias reflecting their alias
                     */

                    WriteProfileString(pIDriver->szFile,
                                       pIDriver->szAlias,
                                       szParams);
                }

                WritePrivateProfileString(pIDriver->szSection,
                                          pIDriver->szAlias,
                                          pIDriver->szFile,
                                          szSysIni);

                /*
                 *  Call the driver to see if it can be configured
                 *  and configure it if it can be
                 */

                if (!SelectInstalled(hWndAvail, pIDriver, szParams, INVALID_HANDLE_VALUE, NULL))
                {

                    /*
                     *  Error talking to driver
                     */

                    WritePrivateProfileString(pIDriver->szSection,
                                              pIDriver->szAlias,
                                              NULL,
                                              szSysIni);

                    WriteProfileString(pIDriver->szFile,
                                       pIDriver->szAlias,
                                       NULL);

                    RemoveIDriver (hAdvDlgTree, pIDriver, TRUE);
                    return FALSE;
                }

                /*
                 *  for displaying the driver desc. in the restart mesg
                 */

                if (!bRelated || pIDriver->bRelated)
                {
                    wcscpy(szRestartDrv, pIDriver->szDesc);
                }

                /*
                 *  We need to write out the driver description to the
                 *  control.ini section [Userinstallable.drivers]
                 *  so we can differentiate between user and system drivers
                 *
                 *  This is tested by the function UserInstalled when
                 *  the user tries to remove a driver and merely
                 *  affects which message the user gets when being
                 *  asked to confirm removal (non user-installed drivers
                 *  are described as being necessary to the system).
                 */

                WritePrivateProfileString(szUserDrivers,
                                          pIDriver->szAlias,
                                          pIDriver->szFile,
                                          szControlIni);


                /*
                 *  Update [related.desc] section of control.ini :
                 *
                 *  ALIAS=driver name list
                 *
                 *  When the driver whose alias is ALIAS is removed
                 *  the drivers in the name list will also be removed.
                 *  These were the drivers in the related drivers list
                 *  when the driver is installed.
                 */

                WritePrivateProfileString(szRelatedDesc,
                                          pIDriver->szAlias,
                                          pIDriver->szRemove,
                                          szControlIni);


                /*
                 * Cache the description string in control.ini in the
                 * drivers description section.
                 *
                 * The key is the driver file name + extension.
                 */

                WritePrivateProfileString(szDriversDesc,
                                          pIDriver->szFile,
                                          pIDriver->szDesc,
                                          szControlIni);

#ifdef DOBOOT // We don't do the boot section on NT

                if (bInstallBootLine)
                {
                    szTemp[MAXSTR];

                    GetPrivateProfileString(szBoot,
                                            szDrivers,
                                            szTemp,
                                            szTemp,
                                            sizeof(szTemp) / sizeof(TCHAR),
                                            szSysIni);
                    wcscat(szTemp, TEXT(" "));
                    wcscat(szTemp, pIDriver->szAlias);
                    WritePrivateProfileString(szBoot,
                                              szDrivers,
                                              szTemp,
                                              szSysIni);
                    bInstallBootLine = FALSE;
                }
#endif // DOBOOT
            }
        }
        else
            return FALSE;                       //ERROR
    }


    /*
     *  If no types were added then fail
     */

    if (pIDriver == NULL)
    {
        return FALSE;
    }

    /*
     *  If there are related drivers listed in the .inf section to install
     *  then install them now by calling ourselves.  Use IDTemplate which
     *  is where mmAddNewDriver put the data.
     */

    if (IDTemplate.bRelated == TRUE)
    {

        int i;
        TCHAR szTemp[MAXSTR];

        /*
         *  Tell file copying to abort rather than put up errors
         */

        bCopyingRelated = TRUE;

        for (i = 1; infParseField(IDTemplate.szRelated, i, szTemp);i++)
        {

            InstallDrivers(hWnd, hWndAvail, szTemp);
        }
    }
    return TRUE;
}

BOOL SelectInstalledKernelDriver(PIDRIVER pIDriver, LPTSTR pszParams)
{
    SC_HANDLE SCManagerHandle;
    SC_HANDLE ServiceHandle;
    TCHAR ServiceName[MAX_PATH];
    TCHAR BinaryPath[MAX_PATH];
    BOOL Success;
    SC_LOCK ServicesDatabaseLock;
    DWORD dwTagId;

    /*
     *  These drivers are not configurable
     */

    pIDriver->fQueryable = 0;

    /*
     *  The services controller will create the registry node to
     *  which we can add the device parameters value
     */

    wcscpy(BinaryPath, TEXT("\\SystemRoot\\system32\\drivers\\"));
    wcscat(BinaryPath, pIDriver->szFile);

    /*
     *  First try and obtain a handle to the service controller
     */

    SCManagerHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (SCManagerHandle == NULL)
        return FALSE;

    /*
     *  Lock the service controller database to avoid deadlocks
     *  we have to loop because we can't wait
     */


    for (ServicesDatabaseLock = NULL;
        (ServicesDatabaseLock =
         LockServiceDatabase(SCManagerHandle))
        == NULL;
        Sleep(100))
    {
    }

    {
        TCHAR drive[MAX_PATH], directory[MAX_PATH], ext[MAX_PATH];
        lsplitpath(pIDriver->szFile, drive, directory, ServiceName, ext);
    }


    ServiceHandle = CreateService(SCManagerHandle,
                                  ServiceName,
                                  NULL,
                                  SERVICE_ALL_ACCESS,
                                  SERVICE_KERNEL_DRIVER,
                                  SERVICE_DEMAND_START,
                                  SERVICE_ERROR_NORMAL,
                                  BinaryPath,
                                  TEXT("Base"),
                                  &dwTagId,
                                  TEXT("\0"),
                                  NULL,
                                  NULL);

    UnlockServiceDatabase(ServicesDatabaseLock);

    if (ServiceHandle == NULL)
    {
        CloseServiceHandle(SCManagerHandle);
        return FALSE;
    }

    /*
     *  Try to write the parameters to the registry if there
     *  are any
     */

    if (wcslen(pszParams))
    {

        HKEY ParmsKey;
        TCHAR RegPath[MAX_PATH];
        wcscpy(RegPath, TEXT("\\SYSTEM\\CurrentControlSet\\Services\\"));
        wcscat(RegPath, ServiceName);
        wcscat(RegPath, TEXT("\\Parameters"));

        Success = RegCreateKey(HKEY_LOCAL_MACHINE,
                               RegPath,
                               &ParmsKey) == ERROR_SUCCESS &&
                  RegSetValue(ParmsKey,
                              TEXT(""),
                              REG_SZ,
                              pszParams,
                              wcslen(pszParams)*sizeof(TCHAR)) == ERROR_SUCCESS &&
                  RegCloseKey(ParmsKey) == ERROR_SUCCESS;

    }
    else
    {
        Success = TRUE;
    }

    /*
     *  Service created so try and start it
     */

    if (Success)
    {
        //  We tell them to restart just in case
        bRestart = TRUE;

        /*
         *  Load the kernel driver by starting the service.
         *  If this is successful it should be safe to let
         *  the system load the driver at system start so
         *  we change the start type.
         */

        Success =
            StartService(ServiceHandle, 0, NULL) &&
            ChangeServiceConfig(ServiceHandle,
                                SERVICE_NO_CHANGE,
                                SERVICE_SYSTEM_START,
                                SERVICE_NO_CHANGE,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL);

        if (!Success)
        {
            TCHAR szMesg[MAXSTR];
            TCHAR szMesg2[MAXSTR];
            TCHAR szTitle[50];

            /*
             *  Uninstall driver if we couldn't load it
             */

            for (ServicesDatabaseLock = NULL;
                (ServicesDatabaseLock =
                 LockServiceDatabase(SCManagerHandle))
                == NULL;
                Sleep(100))
            {
            }

            DeleteService(ServiceHandle);

            UnlockServiceDatabase(ServicesDatabaseLock);

            /*
             *  Tell the user there was a configuration error
             *  (our best guess).
             */

            LoadString(myInstance, IDS_DRIVER_CONFIG_ERROR, szMesg, sizeof(szMesg)/sizeof(TCHAR));
            LoadString(myInstance, IDS_CONFIGURE_DRIVER, szTitle, sizeof(szTitle)/sizeof(TCHAR));
            wsprintf(szMesg2, szMesg, FileName(pIDriver->szFile));
            MessageBox(hMesgBoxParent, szMesg2, szTitle, MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
        }
    }

    CloseServiceHandle(ServiceHandle);
    CloseServiceHandle(SCManagerHandle);

    return Success;
}

/************************************************************************
 *
 *  SelectInstalled()
 *
 *  Check if the driver can be configured and configure it if it can be.
 *
 *  hwnd           - Our window - parent for driver to make its config window
 *  pIDriver       - info about the driver
 *  params         - the drivers parameters from the .inf file.
 *  DeviceInfoSet  - Optionally, specifies the set containing the PnP device
 *                   being installed.  Specify INVALID_HANDLE_VALUE is this
 *                   parameter is not present.
 *  DeviceInfoData - Optionally, specifies the PnP device being installed
 *                   (ignored if DeviceInfoSet is not specified).
 *
 *  Returns FALSE if an error occurred, otherwise TRUE.  GetLastError() may
 *  be called to determine the cause of the failure.
 *
 ************************************************************************/

BOOL SelectInstalled(HWND hwnd, PIDRIVER pIDriver, LPTSTR pszParams, HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData)
{
    BOOL bSuccess      = TRUE;  // Assume we succeed
    BOOL bPutUpMessage = FALSE; // Assume we don't have to put up a message
    HANDLE hDriver = 0;
    DRVCONFIGINFO DrvConfigInfo;
    DWORD_PTR DrvMsgResult;
    DWORD ConfigFlags;
    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    HKEY hkDrv = NULL;

    // Open device reg key
    hkDrv = SetupDiOpenDevRegKey(DeviceInfoSet,
                                 DeviceInfoData,
                                 DICS_FLAG_GLOBAL,
                                 0,
                                 DIREG_DRV,
                                 KEY_ALL_ACCESS);

    if (!hkDrv)
    {
        return GetLastError();
    }

    wsStartWait();

    /*
     *  If it's a kernel driver call the services controller to
     *  install the driver (unless it's a PnP device, in which case
     *  SetupDiInstallDevice would've already handled any necessary
     *  service installation).
     */

    if (pIDriver->KernelDriver)
    {
        // If DeviceInfoSet is a valid handle, then this is a PnP device
        // and there's nothing we need to do. Otherwise, config the kernel driver.
        if (DeviceInfoSet == INVALID_HANDLE_VALUE)
            bSuccess = SelectInstalledKernelDriver(pIDriver,pszParams);
        goto SelectInstalled_exit;
    }

    //  See if we can open the driver
    hDriver = OpenDriver(pIDriver->wszFile, NULL, 0L);

    if (!hDriver)
    {
        bSuccess      = FALSE;
        bPutUpMessage = TRUE;
        goto SelectInstalled_exit;
    }

    // Driver opened, prepare to send config messages to driver
    InitDrvConfigInfo(&DrvConfigInfo, pIDriver);

    // On ISAPNP devices, we need to set the CONFIGFLAG_NEEDS_FORCED_CONFIG before
    // we call into the driver to setup resources.
    if (!SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                          DeviceInfoData,
                                          SPDRP_CONFIGFLAGS,
                                          NULL,
                                          (PBYTE)&ConfigFlags,
                                          sizeof(ConfigFlags),
                                          NULL))
    {
        ConfigFlags = 0;
    }

    ConfigFlags |= CONFIGFLAG_NEEDS_FORCED_CONFIG;

    SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                     DeviceInfoData,
                                     SPDRP_CONFIGFLAGS,
                                     (PBYTE)&ConfigFlags,
                                     sizeof(ConfigFlags)
                                    );

    // See if this is a PnP device by trying to send it a PnP install message.
    // Call driver with DRV_PNPINSTALL
    DrvMsgResult = SendDriverMessage(hDriver,
                                     DRV_PNPINSTALL,
                                     (LONG_PTR)DeviceInfoSet,
                                     (LONG_PTR)DeviceInfoData);

    DeviceInstallParams.cbSize = sizeof(DeviceInstallParams);
    SetupDiGetDeviceInstallParams(DeviceInfoSet, DeviceInfoData, &DeviceInstallParams);

    // Look at result of DRV_PNPINSTALL
    switch (DrvMsgResult)
    {
    case DRVCNF_RESTART :
        // The installation was successful, but a reboot is required.
        // Ensure that the 'need reboot' flag in the device's installation parameters.
        DeviceInstallParams.Flags |= DI_NEEDREBOOT;

        // Let fall through to processing of successful installation.
    case DRVCNF_OK :
        // Remember that this is a PNPISA device driver
        RegSetValueEx(hkDrv,TEXT("DriverType"),0,REG_SZ,(LPBYTE)(TEXT("PNPISA")),14);

        break;

    default:
        // The driver did not want to install.
        // This may be because
        // 1) The user wanted to cancel
        // 2) Installation failed for some other reason
        // 3) This is not an ISAPNP driver (it's either a legacy driver or a WDM driver)
        //    and it doesn't support the DRV_PNPINSTALLis messsage.
        // Unfortunately, we don't have fine enough granularity in the return codes to
        // distinguish between these cases.

        // Assume it's a legacy or WDM driver that doesn't support DRV_PNPINSTALL.
        // Try calling DRV_INSTALL instead.

        // Remember to clear CONFIGFLAG_NEEDS_FORCED_CONFIG flag, which shouldn't be set
        // for legacy or WDM drivers.
        ConfigFlags &= ~CONFIGFLAG_NEEDS_FORCED_CONFIG;
        SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                         DeviceInfoData,
                                         SPDRP_CONFIGFLAGS,
                                         (PBYTE)&ConfigFlags,
                                         sizeof(ConfigFlags)
                                        );

        // Call driver with DRV_INSTALL
        DrvMsgResult = SendDriverMessage(hDriver,
                                         DRV_INSTALL,
                                         0L,
                                         (LONG_PTR)(LPDRVCONFIGINFO)&DrvConfigInfo);
        // Look at result of DRV_INSTALL
        switch (DrvMsgResult)
        {
        case DRVCNF_RESTART:
            // Remember to restart, then fall through to OK case
            DeviceInstallParams.Flags |= DI_NEEDREBOOT;
            bRestart = TRUE;
        case DRVCNF_OK:
            // Remember whether the driver is configurable
            // If it's a WDM driver, it will return FALSE.
            pIDriver->fQueryable =
                (int)SendDriverMessage(hDriver,
                                       DRV_QUERYCONFIGURE,
                                       0L,
                                       0L);

            // If the driver is configurable then configure it.
            // Configuring the driver may result in a need to restart
            // the system.  The user may also cancel install.
            if (pIDriver->fQueryable)
            {
                RegSetValueEx(hkDrv,TEXT("DriverType"),0,REG_SZ,(LPBYTE)(TEXT("Legacy")),14);

                switch (SendDriverMessage(hDriver,
                                          DRV_CONFIGURE,
                                          (LONG_PTR)hwnd,
                                          (LONG_PTR)(LPDRVCONFIGINFO)&DrvConfigInfo))
                {
                case DRVCNF_RESTART:
                    DeviceInstallParams.Flags |= DI_NEEDREBOOT;
                    bRestart = TRUE;
                case DRVCNF_OK:
                    break;

                case DRVCNF_CANCEL:
                    // Don't put up the error box if the user cancelled
                    bSuccess = FALSE;
                    break;
                }
            }
            break;
        case DRVCNF_CANCEL:
            // The driver did not want to install
            SetLastError(ERROR_CANCELLED);
            bPutUpMessage = TRUE;
            bSuccess = FALSE;
            break;
        }
    }

    SelectInstalled_exit:

    SetupDiSetDeviceInstallParams(DeviceInfoSet, DeviceInfoData, &DeviceInstallParams);

    if (hkDrv)
    {
        RegCloseKey(hkDrv);
    }

    if (hDriver)
    {
        CloseDriver(hDriver, 0L, 0L);
    }

    //  If dealing with the driver resulted in error then put up a message
    if (bPutUpMessage)
    {
        OpenDriverError(hwnd, pIDriver->szDesc, pIDriver->szFile);
    }

    wsEndWait();

    return bSuccess;
}

/***********************************************************************
 *
 *  InitDrvConfigInfo()
 *
 *  Initialize Driver Configuration Information.
 *
 ***********************************************************************/

void InitDrvConfigInfo( LPDRVCONFIGINFO lpDrvConfigInfo, PIDRIVER pIDriver )
{
    lpDrvConfigInfo->dwDCISize          = sizeof(DRVCONFIGINFO);
    lpDrvConfigInfo->lpszDCISectionName = pIDriver->wszSection;
    lpDrvConfigInfo->lpszDCIAliasName   = pIDriver->wszAlias;
}

/***********************************************************************
 *
 *  GetValidAlias()
 *
 *  pstrType     - Input  - the type
 *                 Output - New alias for that type
 *
 *  pstrSection  - The system.ini section we're dealing with
 *
 *  Create a valid alias name for a type.  Searches the system.ini file
 *  in the drivers section for aliases of the type already defined and
 *  returns a new alias (eg WAVE1).
 *
 ***********************************************************************/
BOOL GetValidAlias(LPTSTR pstrType, LPTSTR pstrSection)
{
    TCHAR keystr[32];
    TCHAR *pstrTypeEnd;
    int AppendVal;
    DWORD CharsFound;

    pstrTypeEnd = pstrType + wcslen(pstrType);
    for (AppendVal=0; AppendVal<=9; AppendVal++)
    {
        if (AppendVal!=0)
        {
            _itow(AppendVal,pstrTypeEnd,10);
        }

        CharsFound = GetPrivateProfileString( pstrSection,
                                            pstrType,
                                            TEXT(""),
                                            keystr,
                                            sizeof(keystr) / sizeof(TCHAR),
                                            szSysIni);

        if (!CharsFound)
        {
            return TRUE;
        }
    }

    return FALSE;
}

/*******************************************************************
 *
 *  IsConfigurable
 *
 *  Find if a driver supports configuration
 *
 *******************************************************************/

BOOL IsConfigurable(PIDRIVER pIDriver, HWND hwnd)
{
    HANDLE hDriver;

    wsStartWait();

    /*
     *  have we ever checked if this driver is queryable?
     */

    if ( pIDriver->fQueryable == -1 )
    {

        /*
         *  Check it's not a kernel driver
         */

        if (pIDriver->KernelDriver)
        {
            pIDriver->fQueryable = 0;
        }
        else
        {

            /*
             *  Open the driver and ask it if it is configurable
             */

            hDriver = OpenDriver(pIDriver->wszAlias, pIDriver->wszSection, 0L);

            if (hDriver)
            {
                pIDriver->fQueryable =
                    (int)SendDriverMessage(hDriver,
                                           DRV_QUERYCONFIGURE,
                                           0L,
                                           0L);

                CloseDriver(hDriver, 0L, 0L);
            }
            else
            {
                pIDriver->fQueryable = 0;
                OpenDriverError(hwnd, pIDriver->szDesc, pIDriver->szFile);
                wsEndWait();
                return(FALSE);
            }
        }
    }
    wsEndWait();
    return((BOOL)pIDriver->fQueryable);
}

/******************************************************************
 *
 *  Find any driver with the same name currently installed and
 *  remove it
 *
 *  szFile     - File name of driver
 *  szSection  - system.ini section ([MCI] or [drivers]).
 *
 ******************************************************************/

void RemoveAlreadyInstalled(LPTSTR szFile, LPTSTR szSection)
{
    PIDRIVER pIDriver;

    pIDriver = FindIDriverByName (szFile);

    if (pIDriver != NULL)
    {
        PostRemove(pIDriver, FALSE);
        return;
    }

    CheckIniDrivers(szFile, szSection);
}

/******************************************************************
 *
 *  Remove system.ini file entries for our driver
 *
 *  szFile    - driver file name
 *  szSection - [drivers] or [MCI]
 *
 ******************************************************************/

void CheckIniDrivers(LPTSTR szFile, LPTSTR szSection)
{
    TCHAR allkeystr[MAXSTR * 2];
    TCHAR szRemovefile[20];
    TCHAR *keystr;

    GetPrivateProfileString(szSection,
                            NULL,
                            NULL,
                            allkeystr,
                            sizeof(allkeystr) / sizeof(TCHAR),
                            szSysIni);

    keystr = allkeystr;
    while (wcslen(keystr) > 0)
    {

        GetPrivateProfileString(szSection,
                                keystr,
                                NULL,
                                szRemovefile,
                                sizeof(szRemovefile) / sizeof(TCHAR),
                                szSysIni);

        if (!FileNameCmp(szFile, szRemovefile))
            RemoveDriverEntry(keystr, szFile, szSection, FALSE);

        keystr = &keystr[wcslen(keystr) + 1];
    }
}

/******************************************************************
 *
 *   RemoveDriverParams
 *
 *   Remove anything after the next token
 *
 ******************************************************************/

void RemoveDriverParams(LPTSTR szFile, LPTSTR Params)
{
    for (;*szFile == TEXT(' '); szFile++);
    for (;*szFile != TEXT(' ') && *szFile != TEXT('\0'); szFile++);
    if (*szFile == TEXT(' '))
    {
        *szFile = TEXT('\0');
        for (;*++szFile == TEXT(' '););
        wcscpy(Params, szFile);
    }
    else
    {
        *Params = TEXT('\0');
    }
}


DWORD
    InstallDriversForPnPDevice(
                              IN HWND             hWnd,
                              IN HDEVINFO         DeviceInfoSet,
                              IN PSP_DEVINFO_DATA DeviceInfoData
                              )
/*++

Routine Description:

    This routine traverses the "Drivers" tree under the specified device's software
    key, adding each multimedia type entry present to the Drivers32 key of the registry.
    The driver is then invoked to perform any configuration necessary for that type.

Arguments:

    hWnd - Supplies the handle of the window to be used as the parent for any UI.

    DeviceInfoSet - Supplies a handle to the device information set containing the
        multimedia device being installed.

    DeviceInfoData - Supplies the address of the SP_DEVINFO_DATA structure representing
        the multimedia device being installed.

Return Value:

    If successful, the return value is NO_ERROR, otherwise it is a Win32 error code.

--*/
{
    HKEY hKey, hDriversKey, hTypeInstanceKey;
    TCHAR szTypes[MAXSTR];
    TCHAR szType[MAXSTR];
    DWORD Err;
    DWORD RegDataType, cbRegDataSize, RegKeyIndex;
    int i;
    PIDRIVER pIDriver, pPrevIDriver;
    PIDRIVER IDriverList = NULL, IDriverListToCleanUp = NULL;
    TCHAR CharBuffer[MAX_PATH + sizeof(TCHAR)];
    LPCTSTR CurrentFilename;
    BOOL bNoMoreAliases = FALSE;

    if ((hKey = SetupDiOpenDevRegKey(DeviceInfoSet,
                                     DeviceInfoData,
                                     DICS_FLAG_GLOBAL,
                                     0,
                                     DIREG_DRV,
                                     KEY_ALL_ACCESS)) == INVALID_HANDLE_VALUE)
    {
        return GetLastError();
    }

    //
    // What we're really interested in is the "Drivers" subkey.
    //
    Err = (DWORD)RegOpenKeyEx(hKey, gszDriversSubkeyName, 0, KEY_ALL_ACCESS, &hDriversKey);

    RegCloseKey(hKey);      // don't need this key anymore.

    if (Err != ERROR_SUCCESS)
    {
        //
        // If the key is not present, then there is no work to do.
        //
        return NO_ERROR;
    }

    //
    // Retrieve the "SubClasses" value from this key.  This contains a comma-delimited
    // list of all multimedia type entries associated with this device.
    //
    cbRegDataSize = sizeof(szTypes);
    if ((Err = RegQueryValueEx(hDriversKey,
                               gszSubClassesValue,
                               NULL,
                               &RegDataType,
                               (PBYTE)szTypes,
                               &cbRegDataSize)) != ERROR_SUCCESS)
    {
        goto clean0;
    }

    if ((RegDataType != REG_SZ) || !cbRegDataSize)
    {
        Err = ERROR_INVALID_DATA;
        goto clean0;
    }

    //
    // OK, we have the list of types, now process each one.
    //
    for (i = 1; ((Err == NO_ERROR) && infParseField(szTypes, i, szType)); i++)
    {

        if (RegOpenKeyEx(hDriversKey, szType, 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS)
        {
            //
            // Couldn't find a subkey for this entry--move on to the next one.
            //
            continue;
        }

        for (RegKeyIndex = 0;
            ((Err == NO_ERROR) &&
             (RegEnumKey(hKey, RegKeyIndex, CharBuffer, sizeof(CharBuffer)/sizeof(TCHAR)) == ERROR_SUCCESS));
            RegKeyIndex++)
        {
            if (RegOpenKeyEx(hKey, CharBuffer, 0, KEY_ALL_ACCESS, &hTypeInstanceKey) != ERROR_SUCCESS)
            {
                //
                // For some reason, we couldn't open the key we just enumerated.  Oh well, move on
                // to the next one.
                //
                continue;
            }

            if (!(pIDriver = (PIDRIVER)LocalAlloc(LPTR, sizeof(IDRIVER))))
            {
                //
                // Not enough memory!  Abort the whole thing.
                //
                Err = ERROR_NOT_ENOUGH_MEMORY;
                goto CloseInstanceAndContinue;
            }

            //
            // Retrieve the description and driver filename from this key.
            //
            cbRegDataSize = sizeof(pIDriver->szDesc);
            if ((RegQueryValueEx(hTypeInstanceKey,
                                 gszDescriptionValue,
                                 NULL,
                                 &RegDataType,
                                 (LPBYTE)pIDriver->szDesc,
                                 &cbRegDataSize) != ERROR_SUCCESS)
                || (RegDataType != REG_SZ) || !cbRegDataSize)
            {
                LocalFree((HANDLE)pIDriver);
                goto CloseInstanceAndContinue;
            }

            wcsncpy(pIDriver->szSection,
                    wcsstr(pIDriver->szDesc, TEXT("MCI")) ? szMCI : szDrivers,
                    sizeof(pIDriver->szSection) / sizeof(TCHAR)
                   );

            cbRegDataSize = sizeof(pIDriver->szFile);
            if ((RegQueryValueEx(hTypeInstanceKey,
                                 gszDriverValue,
                                 NULL,
                                 &RegDataType,
                                 (LPBYTE)pIDriver->szFile,
                                 &cbRegDataSize) != ERROR_SUCCESS)
                || (RegDataType != REG_SZ) || !cbRegDataSize)
            {
                LocalFree((HANDLE)pIDriver);
                goto CloseInstanceAndContinue;
            }

            pIDriver->KernelDriver = IsFileKernelDriver(pIDriver->szFile);

            //
            // Find a valid alias for this device (eg Wave2).  This is
            // used as the key in the [MCI] or [Drivers32] section.
            //
            wcsncpy(pIDriver->szAlias, szType, sizeof(pIDriver->szAlias) / sizeof(TCHAR));

            if (!GetValidAlias(pIDriver->szAlias, pIDriver->szSection))
            {
                //
                // Exceeded the maximum--but can't tell the user.  We can't bring up a dialog
                // in the services.exe process
                //
                bNoMoreAliases = TRUE;
                LocalFree((HANDLE)pIDriver);
                goto CloseInstanceAndContinue;
            }

            //
            // Fill in the Unicode fields from the ANSI ones.
            //
            wcscpy(pIDriver->wszSection, pIDriver->szSection);
            wcscpy(pIDriver->wszAlias,   pIDriver->szAlias);
            wcscpy(pIDriver->wszFile,    pIDriver->szFile);

            //
            // We must write the alias out now, because we may need to generate
            // other aliases for this same type, and we can't generate a unique
            // alias unless all existing aliases are present in the relevant
            // registry key.
            //
            WritePrivateProfileString(pIDriver->szSection,
                                      pIDriver->szAlias,
                                      pIDriver->szFile,
                                      szSysIni
                                     );

            //
            // We also must write the alias out to the key we're currently in (under
            // the device's software key), because during uninstall, we need to be
            // able to figure out what devices get removed.
            //
            RegSetValueEx(hTypeInstanceKey,
                          gszAliasValue,
                          0,
                          REG_SZ,
                          (PBYTE)(pIDriver->szAlias),
                          (wcslen(pIDriver->szAlias)*sizeof(TCHAR)) + sizeof(TCHAR)
                         );

            //
            // Add this new IDriver node to our linked list.  The list is sorted by
            // driver filename, and this node should be inserted at the end of the
            // the group of nodes that have the same driver filename.
            //
            InsertNewIDriverNodeInList(&IDriverList, pIDriver);

            CloseInstanceAndContinue:

            RegCloseKey(hTypeInstanceKey);
        }

        RegCloseKey(hKey);
    }

    if ((Err == NO_ERROR) && !IDriverList)
    {
        //  We actually don't want to present the ugly "Data is invalid" error if we ran out of aliases
        if (bNoMoreAliases)
        {
            DestroyIDriverNodeList(IDriverList, TRUE, FALSE);
            goto clean0;
        }
        else
        {
            //
            // We didn't find anything to install!
            //
            Err = ERROR_INVALID_DATA;
        }
    }

    if (Err != NO_ERROR)
    {
        //
        // Clean up anything we put in the multimedia sections of the registry.
        //
        DestroyIDriverNodeList(IDriverList, TRUE, FALSE);
        goto clean0;
    }

    //
    // If we get to here, then we've successfully built up a list of all driver entries
    // we need to install.  Now, traverse the list, and install each one.
    //
    CurrentFilename = NULL;
    *CharBuffer = TEXT('\0');        // use this character buffer to contain (empty) parameter string.
    pIDriver = IDriverList;
    pPrevIDriver = NULL;

    while (pIDriver)
    {
        if (!CurrentFilename || _wcsicmp(CurrentFilename, pIDriver->szFile))
        {
            //
            // This is the first entry we've encountered for this driver.  We need
            // to call the driver to see if it can be configured, and configure it
            // if it can be.
            //
            if (SelectInstalled(hWnd, pIDriver, CharBuffer, DeviceInfoSet, DeviceInfoData))
            {
                //
                // Move this IDriver node to our list of clean-up items.  This is used in
                // case we hit an error with some other driver, and we need to notify this
                // driver that even though it was successful, someone else screwed up and
                // complete removal of the device must occur.
                //
                if (pPrevIDriver)
                {
                    pPrevIDriver->related = pIDriver->related;
                }
                else
                {
                    IDriverList = pIDriver->related;
                }
                pIDriver->related = IDriverListToCleanUp;
                IDriverListToCleanUp = pIDriver;
            }
            else
            {
                //
                // Error talking to driver
                //
                Err = GetLastError();
                goto clean1;
            }

#if 0       // We don't need this piece of code in the Plug&Play install case.

            /*
             *  for displaying the driver desc. in the restart mesg
             */
            if (!bRelated || pIDriver->bRelated)
            {
                wcscpy(szRestartDrv, pIDriver->szDesc);
            }
#endif

            //
            // We need to write out the driver description to the
            // control.ini section [Userinstallable.drivers]
            // so we can differentiate between user and system drivers
            //
            // This is tested by the function UserInstalled when
            // the user tries to remove a driver and merely
            // affects which message the user gets when being
            // asked to confirm removal (non user-installed drivers
            // are described as being necessary to the system).
            //
            WritePrivateProfileString(szUserDrivers,
                                      pIDriver->szAlias,
                                      pIDriver->szFile,
                                      szControlIni
                                     );

            //
            // Update [related.desc] section of control.ini :
            //
            // ALIAS=driver name list
            //
            // When the driver whose alias is ALIAS is removed
            // the drivers in the name list will also be removed.
            // These were the drivers in the related drivers list
            // when the driver is installed.
            //
            WritePrivateProfileString(szRelatedDesc,
                                      pIDriver->szAlias,
                                      pIDriver->szRemove,
                                      szControlIni
                                     );

            //
            // Cache the description string in control.ini in the
            // drivers description section.
            //
            // The key is the driver file name + extension.
            //
            WritePrivateProfileString(szDriversDesc,
                                      pIDriver->szFile,
                                      pIDriver->szDesc,
                                      szControlIni
                                     );

#ifdef DOBOOT // We don't do the boot section on NT

            if (bInstallBootLine)
            {
                szTemp[MAXSTR];

                GetPrivateProfileString(szBoot,
                                        szDrivers,
                                        szTemp,
                                        szTemp,
                                        sizeof(szTemp) / sizeof(TCHAR),
                                        szSysIni);
                wcscat(szTemp, TEXT(" "));
                wcscat(szTemp, pIDriver->szAlias);
                WritePrivateProfileString(szBoot,
                                          szDrivers,
                                          szTemp,
                                          szSysIni);
                bInstallBootLine = FALSE;
            }
#endif // DOBOOT

            //
            // Update our "CurrentFilename" pointer, so that we'll know when we
            // move from one driver filename to another.
            //
            CurrentFilename = pIDriver->szFile;

            //
            // Move on to the next IDriver node IN THE ORIGINAL LIST.  We can't simply
            // move on the 'related' pointer in our node anymore, since we moved it
            // into our clean-up list.
            //
            if (pPrevIDriver)
            {
                pIDriver = pPrevIDriver->related;
            }
            else
            {
                pIDriver = IDriverList;
            }

        }
        else
        {
            //
            // We've already configured this driver.  Leave it in its original list,
            // and move on to the next node.
            //
            pPrevIDriver = pIDriver;
            pIDriver = pIDriver->related;
        }
    }

    clean1:

    DestroyIDriverNodeList(IDriverListToCleanUp, (Err != NO_ERROR), TRUE);
    DestroyIDriverNodeList(IDriverList, (Err != NO_ERROR), FALSE);

    clean0:

    RegCloseKey(hDriversKey);

    return Err;
}


void
    InsertNewIDriverNodeInList(
                              IN OUT PIDRIVER *IDriverList,
                              IN     PIDRIVER  NewIDriverNode
                              )
/*++

Routine Description:

    This routine inserts a new IDriver node into the specified linked list of IDriver
    nodes.  The list is sorted by driver filename, and this node will be placed after
    any existing nodes having this same driver filename.

Arguments:

    IDriverList - Supplies the address of the variable that points to the head of the
        linked list.  If the new node is inserted at the head of the list, this variable
        will be updated upon return to reflect the new head of the list.

    NewIDriverNode - Supplies the address of the new driver node to be inserted into the
        list.

Return Value:

    None.

--*/
{
    PIDRIVER CurNode, PrevNode;

    for (CurNode = *IDriverList, PrevNode = NULL;
        CurNode;
        PrevNode = CurNode, CurNode = CurNode->related)
    {
        if (_wcsicmp(CurNode->szFile, NewIDriverNode->szFile) > 0)
        {
            break;
        }
    }

    //
    // Insert the new IDriver node in front of the current one.
    //
    NewIDriverNode->related = CurNode;
    if (PrevNode)
    {
        PrevNode->related = NewIDriverNode;
    }
    else
    {
        *IDriverList = NewIDriverNode;
    }
}


void
    DestroyIDriverNodeList(
                          IN PIDRIVER IDriverList,
                          IN BOOL     CleanRegistryValues,
                          IN BOOL     NotifyDriverOfCleanUp
                          )
/*++

Routine Description:

    This routine frees all memory associated with the nodes in the specified IDriver
    linked list.  It also optionally cleans up any modifications that were previously
    made as a result of an attempted install.

Arguments:

    IDriverList - Points to the head of the linked list of IDriver nodes.

    CleanRegistryValues - If TRUE, then the multimedia registry values previously
        created (e.g., Drivers32 aliases) will be deleted.

    NotifyDriverOfCleanUp - If TRUE, then the driver will be notified of its removal.
        This only applies to non-kernel (i.e., installable) drivers, and this flag is
        ignored if CleanRegistryValues is FALSE.

Return Value:

    None.

--*/
{
    PIDRIVER NextNode;
    HANDLE hDriver;

    while (IDriverList)
    {
        NextNode = IDriverList->related;
        if (CleanRegistryValues)
        {
            if (NotifyDriverOfCleanUp && !IDriverList->KernelDriver)
            {
                if (hDriver = OpenDriver(IDriverList->wszAlias, IDriverList->wszSection, 0L))
                {
                    SendDriverMessage(hDriver, DRV_REMOVE, 0L, 0L);
                    CloseDriver(hDriver, 0L, 0L);
                }
            }
            WritePrivateProfileString(IDriverList->szSection,
                                      IDriverList->szAlias,
                                      NULL,
                                      szSysIni
                                     );

            WriteProfileString(IDriverList->szFile, IDriverList->szAlias, NULL);
        }
        LocalFree((HANDLE)IDriverList);
        IDriverList = NextNode;
    }
}


// Adds mmseRunOnce to Registry for next reboot
BOOL WINAPI SetRunOnceSchemeInit (void)
{
    static const TCHAR aszRunOnce[] = REGSTR_PATH_RUNONCE;
    static const TCHAR aszName[]    = TEXT ("MigrateMMDrivers");
    static const TCHAR aszCommand[] = TEXT ("rundll32.exe mmsys.cpl,mmseRunOnce");

    HKEY  hKey;
    DWORD cbSize;

    if (ERROR_SUCCESS != RegCreateKeyEx (HKEY_LOCAL_MACHINE, aszRunOnce, 0, NULL, 0,
                                         KEY_ALL_ACCESS, NULL, &hKey, NULL))
    {
        return FALSE;
    }

    cbSize = (lstrlen(aszCommand)+1) * sizeof(TCHAR);
    if (ERROR_SUCCESS != RegSetValueEx (hKey, aszName, 0, REG_SZ,
                                        (LPBYTE)(LPVOID)aszCommand, cbSize))
    {
        RegCloseKey (hKey);
        return FALSE;
    }

    RegCloseKey (hKey);
    return TRUE;
} // End SetRunOnce


BOOL DriverNodeSupportsNt(IN HDEVINFO         DeviceInfoSet,
                          IN PSP_DEVINFO_DATA DeviceInfoData,
                          IN PSP_DRVINFO_DATA DriverInfoData
                         )
/*++

Routine Description:

    This routine determines whether the driver node specified is capable of
    installing on Windows NT (as opposed to being a Win95-only driver node).
    This determination is made based upon whether or not there is a corresponding
    service install section for this device install section.

Return Value:

    If the driver node supports Windows NT, the return value is TRUE, otherwise
    it is FALSE.

--*/
{
    SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;
    HINF hInf;
    DWORD Err;
    TCHAR ActualSectionName[255];
    DWORD ActualSectionNameLen;
    LONG LineCount;
    CONST TCHAR szServiceInstallSuffix[] = TEXT(".") INFSTR_SUBKEY_SERVICES;

    // Get name and section to install from
    DriverInfoDetailData.cbSize = sizeof(DriverInfoDetailData);
    if (!SetupDiGetDriverInfoDetail(DeviceInfoSet,
                                    DeviceInfoData,
                                    DriverInfoData,
                                    &DriverInfoDetailData,
                                    sizeof(DriverInfoDetailData),
                                    NULL) &&
        ((Err = GetLastError()) != ERROR_INSUFFICIENT_BUFFER))
    {
        return FALSE;
    }

    //
    // Open the associated INF file.
    //
    if ((hInf = SetupOpenInfFile(DriverInfoDetailData.InfFileName, NULL, INF_STYLE_WIN4, NULL)) == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    //
    // Retrieve the actual name of the install section to be used for this
    // driver node.
    //
    SetupDiGetActualSectionToInstall(hInf,
                                     DriverInfoDetailData.SectionName,
                                     ActualSectionName,
                                     sizeof(ActualSectionName) / sizeof(TCHAR),
                                     &ActualSectionNameLen,
                                     NULL
                                    );

    //
    // Generate the service install section name, and see if it exists.
    //
    CopyMemory(&(ActualSectionName[ActualSectionNameLen - 1]),
               szServiceInstallSuffix,
               sizeof(szServiceInstallSuffix)
              );

    LineCount = SetupGetLineCount(hInf, ActualSectionName);

    SetupCloseInfFile(hInf);

    return (LineCount != -1);
}

// Go through the list of drivers and try to keep from installing or displaying any non-NT drivers
// Warning: If you call this function with DeviceInfoData NULL, it will have to enumerate and open
// every media inf there is, which may take awhile.
BOOL FilterOutNonNTInfs(IN HDEVINFO         DeviceInfoSet,
                        IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
                        DWORD DriverType
                       )
{
    DWORD MemberIndex;
    SP_DRVINFO_DATA DriverInfoData;
    SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;
    SP_DRVINSTALL_PARAMS DriverInstallParams;

    MemberIndex = 0;
    DriverInfoData.cbSize = sizeof(DriverInfoData);
    while (SetupDiEnumDriverInfo(DeviceInfoSet,DeviceInfoData,DriverType,MemberIndex,&DriverInfoData))
    {
        if (!DriverNodeSupportsNt(DeviceInfoSet, DeviceInfoData, &DriverInfoData))
        {
            // If driver doesn't support NT, try to exclude from list & max out rank
            DriverInstallParams.cbSize=sizeof(DriverInstallParams);
            if (SetupDiGetDriverInstallParams(DeviceInfoSet, DeviceInfoData, &DriverInfoData, &DriverInstallParams))
            {
                DriverInstallParams.Flags |= DNF_EXCLUDEFROMLIST | DNF_BAD_DRIVER;
                DriverInstallParams.Rank = 10000;
                SetupDiSetDriverInstallParams(DeviceInfoSet, DeviceInfoData, &DriverInfoData, &DriverInstallParams);
            }
        }
        MemberIndex++;
    }

    return TRUE;
}

DWORD Media_SelectBestCompatDrv(IN HDEVINFO         DeviceInfoSet,
                                IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
                               )
{
    DWORD DriverType = (DeviceInfoData ? SPDIT_COMPATDRIVER : SPDIT_CLASSDRIVER);

    FilterOutNonNTInfs(DeviceInfoSet, DeviceInfoData, DriverType);
    return ERROR_DI_DO_DEFAULT;
}

DWORD Media_AllowInstall(IN HDEVINFO         DeviceInfoSet,
                         IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
                        )
{
    DWORD Err;
    SP_DRVINFO_DATA DriverInfoData;

    // Verify that the driver node selected for this device supports NT.
    // It will probably be a pretty common scenario for users to try to
    // give us their Win95 INFs.
    DriverInfoData.cbSize = sizeof(DriverInfoData);
    if (!SetupDiGetSelectedDriver(DeviceInfoSet, DeviceInfoData, &DriverInfoData))
    {
        // NULL driver?
        return ERROR_DI_DO_DEFAULT;
    }

    if (!DriverNodeSupportsNt(DeviceInfoSet,
                              DeviceInfoData,
                              &DriverInfoData))
    {
        dlog("Media_AllowInstall: Not an NT driver");
        return ERROR_DI_DONT_INSTALL;
    }

    return ERROR_DI_DO_DEFAULT;
}

DWORD Media_InstallDevice(IN HDEVINFO         DeviceInfoSet,
                          IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
                         )
{
    DWORD Err, ConfigFlags;
    SP_DRVINFO_DATA DriverInfoData;
    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    HWND hWnd;

    // First remove any driver that was already installed
    Media_RemoveDevice(DeviceInfoSet,DeviceInfoData);

    DriverInfoData.cbSize = sizeof(DriverInfoData);
    if (!SetupDiGetSelectedDriver(DeviceInfoSet, DeviceInfoData, &DriverInfoData))
    {
        //
        // The NULL driver is to be installed for this device.  We don't need to
        // do anything special in that case.
        //
        dlog("Media_InstallDevice: Null driver");
        return ERROR_DI_DO_DEFAULT;
    }

    dlog("Media_InstallDevice: Calling SetupDiInstallDevice");
    if (!SetupDiInstallDevice(DeviceInfoSet, DeviceInfoData))
    {

        Err = GetLastError();

        dlog("Media_InstallDevice: SetupDiInstallDevice failed");
        //
        // In certain circumstances, we have INFs that control some of the functions on the
        // card, but not all (e.g., our sndblst driver controls wave, midi, aux, mixer but
        // not the fancy 3D stuff).  In order to give the user a descriptive name that lets
        // them know what we're trying to install, the INF contains driver nodes for devices
        // it can't support.  If this is the case, then SetupDiInstallDevice will fail with
        // ERROR_NO_ASSOCIATED_SERVICE.  If this happens, we want to clear the
        // CONFIGFLAG_REINSTALL that got set, so we don't keep hounding the user about this.
        // While we're at it, we go ahead and store the driver node's device description as
        // the device instance's description, so that we know what the device instances are
        // later on (for diagnostic purposes, mainly).
        //
        if (Err == ERROR_NO_ASSOCIATED_SERVICE)
        {

            // Clear reinstall flag
            if (SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                                 DeviceInfoData,
                                                 SPDRP_CONFIGFLAGS,
                                                 NULL,
                                                 (PBYTE)&ConfigFlags,
                                                 sizeof(ConfigFlags),
                                                 NULL))
            {
                ConfigFlags &= ~CONFIGFLAG_REINSTALL;
                SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                                 DeviceInfoData,
                                                 SPDRP_CONFIGFLAGS,
                                                 (PBYTE)&ConfigFlags,
                                                 sizeof(ConfigFlags)
                                                );
            }

            // Save description of device
            SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                             DeviceInfoData,
                                             SPDRP_DEVICEDESC,
                                             (PBYTE)DriverInfoData.Description,
                                             (lstrlen(DriverInfoData.Description) + 1) * sizeof(TCHAR)
                                            );
        }

        goto Media_InstallDevice_exit;
    }

    //
    // Get the device install parameters, so we'll know what parent window to use for any
    // UI that occurs during configuration of this device.
    //
    DeviceInstallParams.cbSize = sizeof(DeviceInstallParams);
    if (SetupDiGetDeviceInstallParams(DeviceInfoSet, DeviceInfoData, &DeviceInstallParams))
    {
        hWnd = DeviceInstallParams.hwndParent;
    }
    else
    {
        hWnd = NULL;
    }

    //
    // The INF will have created a "Drivers" subkey under the device's software key.
    // This tree, in turn, contains subtrees for each type of driver (aux, midi, etc.)
    // applicable for this device.  We must now traverse this tree, and create entries
    // in Drivers32 for each function alias.
    //
    dlog("Media_InstallDevice: Calling InstallDriversForPnPDevice");
    if ((Err = InstallDriversForPnPDevice(hWnd, DeviceInfoSet, DeviceInfoData)) != NO_ERROR)
    {
        //
        // The device is in an unknown state.  Disable it by setting the
        // CONFIGFLAG_DISABLED config flag, and mark it as needing a reinstall.
        //
        if (!SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                              DeviceInfoData,
                                              SPDRP_CONFIGFLAGS,
                                              NULL,
                                              (PBYTE)&ConfigFlags,
                                              sizeof(ConfigFlags),
                                              NULL))
        {
            ConfigFlags = 0;
        }

        ConfigFlags |= (CONFIGFLAG_DISABLED | CONFIGFLAG_REINSTALL);

        SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                         DeviceInfoData,
                                         SPDRP_CONFIGFLAGS,
                                         (PBYTE)&ConfigFlags,
                                         sizeof(ConfigFlags)
                                        );

        //
        // Delete the Driver= entry from the Dev Reg Key and delete the
        // DrvRegKey.
        //
        SetupDiDeleteDevRegKey(DeviceInfoSet,
                               DeviceInfoData,
                               DICS_FLAG_GLOBAL | DICS_FLAG_CONFIGGENERAL,
                               0,
                               DIREG_DRV
                              );

        SetupDiSetDeviceRegistryProperty(DeviceInfoSet, DeviceInfoData, SPDRP_DRIVER, NULL, 0);

        //
        // Also, delete the service property, so we'll know this device instance needs to be
        // cleaned up if we later reboot and don't find the device.
        //
        SetupDiSetDeviceRegistryProperty(DeviceInfoSet, DeviceInfoData, SPDRP_SERVICE, NULL, 0);

        goto Media_InstallDevice_exit;
    }

    Err = NO_ERROR;

    SetRunOnceSchemeInit();

    Media_InstallDevice_exit:

    dlog("Media_InstallDevice: Returning");
    return Err;
}

