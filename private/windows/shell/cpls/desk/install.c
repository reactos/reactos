#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"

#include <windows.h>
#include <initguid.h>
#include <devguid.h>

#include "tchar.h"
#include "string.h"

#include <setupapi.h>
#include <syssetup.h>
#include <regstr.h>
#include <setupbat.h>
#include <cfgmgr32.h>

#include "settings.h"
#include "setinc.h"


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


DWORD
MonitorClassInstaller(
    IN DI_FUNCTION      InstallFunction,
    IN HDEVINFO         hDevInfo,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
    )

/*++

Routine Description:

  This routine acts as the class installer for Display devices.

Arguments:

    InstallFunction - Specifies the device installer function code indicating
        the action being performed.

    DeviceInfoSet - Supplies a handle to the device information set being
        acted upon by this install action.

    DeviceInfoData - Optionally, supplies the address of a device information
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


    KdPrint(("MonitorClassInstaller function = %d\n", (DWORD) InstallFunction));

    //
    // If we did not exit from the routine by handling the call, tell the
    // setup code to handle everything the default way.
    //

    return ERROR_DI_DO_DEFAULT;
}


DWORD
DisplayClassInstaller(
    IN DI_FUNCTION      InstallFunction,
    IN HDEVINFO         hDevInfo,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
    )

/*++

Routine Description:

  This routine acts as the class installer for Display devices.

Arguments:

    InstallFunction - Specifies the device installer function code indicating
        the action being performed.

    DeviceInfoSet - Supplies a handle to the device information set being
        acted upon by this install action.

    DeviceInfoData - Optionally, supplies the address of a device information
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

    KdPrint(("DisplayClassInstaller function = %d\n", (DWORD) InstallFunction));

    switch(InstallFunction) {

    case DIF_SELECTDEVICE :

        KdPrint(("DisplayClassInstaller DELECTDEVICE \n"));

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

    //
    // we do not handle the following message because we are not
    // plug-and-play yet.
    //

    // case DIF_INSTALLDEVICE :

    default :

        break;
    }

    //
    // If we did not exit from the routine by handling the call, tell the
    // setup code to handle everything the default way.
    //

    return ERROR_DI_DO_DEFAULT;
}











VOID
SetSelectDevParams(
    HDEVINFO hDevInfo
    )

/*++

Routine Description:
    Sets the select device parameters by calling setup apis

Arguments:
    hDevInfo    : Handle to the printer class device information list

--*/
{
    SP_SELECTDEVICE_PARAMS  SelectDevParams;
    SP_DEVINSTALL_PARAMS    DeviceInstallParams;

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


    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    SetupDiGetDeviceInstallParams(hDevInfo,
                                  NULL,
                                  &DeviceInstallParams);

    DeviceInstallParams.Flags |= DI_USECI_SELECTSTRINGS | DI_SHOWOEM;
    SetupDiSetDeviceInstallParams(hDevInfo,
                                  NULL,
                                  &DeviceInstallParams);

}


VOID
PreSelectDriver(
    HDEVINFO    hDevInfo,
    LPCTSTR     pszModel
    )
/*++

Routine Description:

    Preselect a manufacturer and model for the driver dialog

    If same model is found select it, else if a match in manufacturer is
    found select first driver for the manufacturer.

--*/
{
    SP_DRVINFO_DATA DrvInfoData;
    DWORD           dwIndex = 0;

    //
    // If no model/manf given select first driver
    //

    if (pszModel && *pszModel)
    {
        DrvInfoData.cbSize = sizeof(DrvInfoData);

        while (SetupDiEnumDriverInfo(hDevInfo,
                                     NULL,
                                     SPDIT_CLASSDRIVER,
                                     dwIndex,
                                     &DrvInfoData))
        {
            if (!lstrcmp(pszModel, DrvInfoData.Description))
            {
                //
                // Found a full match.  We will return.
                //

                SetupDiSetSelectedDriver(hDevInfo,
                                         NULL,
                                         &DrvInfoData);

                return;
            }

            DrvInfoData.cbSize = sizeof(DrvInfoData);
            ++dwIndex;
        }

        //
        // There a many third-party names that we would like to match if
        // the third party driver disk is not available (old driver that
        // was not installed.
        //
        // Let do the mapping of third-party names to currently provided
        // names and restart the "selection".
        //

        // "#9 Imagine 128" = n9i128

    }

    return;
}








DWORD
InstallDriver(
    HWND         hwnd,
    INSTALL_TYPE InstallType,
    LPCTSTR      pszModel,
    LPCTSTR      pszInf,
    LPTSTR       serviceName
    )
{
    HCURSOR hCursorOrg;
    HCURSOR hCursor;
    HDEVINFO hDevInfo;
    DWORD err;
    DWORD bEntryFound;
    DWORD retError = NO_ERROR;
    SP_DRVINFO_DATA DriverInfoData;
    SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;
    BOOL bThirdParty = FALSE;
    DWORD cbOutputSize;
    HINF InfFileHandle;
    PVOID Context;
    INFCONTEXT infoContext;

    LPTSTR pSourceLists[6];
    LPTSTR *pSourceList = pSourceLists;
    ULONG  pSourceListCount;
    TCHAR  szSourceLocation[LINE_LEN];

    LPTSTR InfName;
    LPTSTR copyFileSection;
    LPTSTR installSection;
    TCHAR installSectionData[LINE_LEN];
    TCHAR szServiceSection[LINE_LEN];
    TCHAR szServiceName[LINE_LEN];
    INFCONTEXT serviceContext;

    TCHAR DeviceInst[LINE_LEN];
    SP_DEVINFO_DATA DeviceInfoData;
    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    TCHAR szSoftwareSection[LINE_LEN];
    INFCONTEXT tmpContext;
    ULONG maxmem;
    ULONG numDev;
    ULONG configFlags;

    TCHAR keyName[LINE_LEN];
    HKEY hkey;
    DWORD disposition;

    //
    // instantiate setup
    //

    hDevInfo = SetupDiCreateDeviceInfoList((LPGUID) &GUID_DEVCLASS_DISPLAY,
                                           hwnd);

    if (hDevInfo == INVALID_HANDLE_VALUE)
        return ERROR_INVALID_PARAMETER;

    if (InstallType != DETECT)
    {
        //
        // Turn on the hourglass since it will take a long time to process
        // the inf file
        //

        hCursorOrg = GetCursor();

        if (hCursor = LoadCursor(NULL, (LPCTSTR) IDC_WAIT))
        {
            SetCursor(hCursor);
        }

        //
        // Build the list of drivers, and select the approrpiate entry within
        // it, if possible.
        // If we are preinstalling, specify the appropriate inf
        //

        if (InstallType == INSTALL)
        {
            if (!SetupDiBuildDriverInfoList(hDevInfo,
                                            NULL,
                                            SPDIT_CLASSDRIVER))
            {
                SetupDiDestroyDeviceInfoList(hDevInfo);
                return ERROR_INVALID_PARAMETER;
            }
        }
        else
        {
            DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

            SetupDiGetDeviceInstallParams(hDevInfo,
                                          NULL,
                                          &DeviceInstallParams);

            DeviceInstallParams.Flags |= DI_NOFILECOPY |
                                         DI_DONOTCALLCONFIGMG |
                                         DI_ENUMSINGLEINF;

            if (SetupQuerySourceList(SRCLIST_SYSTEM,
                                     &pSourceList,
                                     &pSourceListCount))
            {
                pSourceListCount = 0;

                wsprintf(DeviceInstallParams.DriverPath,
                         TEXT("%ws\\%ws\\%ws"),
                         pSourceList[0], WINNT_OEM_DISPLAY_DIR, pszInf);

                SetupFreeSourceList(&pSourceList, pSourceListCount);
            }

            SetupDiSetDeviceInstallParams(hDevInfo,
                                          NULL,
                                          &DeviceInstallParams);

            if (!SetupDiBuildDriverInfoList(hDevInfo,
                                            NULL,
                                            SPDIT_CLASSDRIVER))

            {
                SetupDiDestroyDeviceInfoList(hDevInfo);
                return ERROR_INVALID_PARAMETER;
            }
        }

        PreSelectDriver(hDevInfo, pszModel);

        //
        // Set the parameters for the window to show appriate text, buttons.
        // Then tell setup to use these parameters
        //

        SetSelectDevParams(hDevInfo);

        //
        // Restore the pointer
        //

        if (hCursorOrg)
        {
            SetCursor(hCursorOrg);
        }


        //
        // Call setup to run the window
        //

reselect:

        bEntryFound = FALSE;

        //
        // Ask the user to pick the device, unless we are preinstalling, in
        // which we just use the preselection.
        //

        if ((InstallType == PREINSTALL) ||
            SetupDiSelectDevice(hDevInfo, NULL))
        {
            DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);

            if (SetupDiGetSelectedDriver(hDevInfo,
                                         NULL,
                                         &DriverInfoData))
            {
                //
                // If this is thrid party driver, put up a popup.
                //

                if (_tcsicmp(DriverInfoData.ProviderName, TEXT("Microsoft")))
                {
                    bThirdParty = TRUE;

                    if (InstallType != PREINSTALL)
                    {
                        if (FmtMessageBox(hwnd,
                                          MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION,
                                          FALSE,
                                          ID_DSP_TXT_THIRD_PARTY,
                                          ID_DSP_TXT_THIRD_PARTY_DRIVER) != IDYES)
                        {
                            goto reselect;
                        }
                    }
                }

                DriverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);

                if ((SetupDiGetDriverInfoDetail(hDevInfo,
                                                NULL,
                                                &DriverInfoData,
                                                &DriverInfoDetailData,
                                                DriverInfoDetailData.cbSize,
                                                &cbOutputSize)) ||
                    (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
                {
                    bEntryFound = TRUE;
                }
            }
        }

        if (bEntryFound == FALSE)
        {
            retError = GetLastError();
            SetupDiDestroyDriverInfoList(hDevInfo, NULL, SPDIT_CLASSDRIVER);
            SetupDiDestroyDeviceInfoList(hDevInfo);

            return retError;
        }
    }

    //
    // open the inf so we can run the sections in the inf, more or less
    // manually.
    //

    if (InstallType == DETECT)
    {
        InfName = TEXT("display.inf");
        copyFileSection = TEXT("detect");
        installSection = installSectionData;
    }
    else
    {
        InfName = DriverInfoDetailData.InfFileName;
        copyFileSection = DriverInfoDetailData.SectionName;
        installSection = DriverInfoDetailData.SectionName;
    }

    //
    // Open the inf so we can copy the files from it.
    //

    InfFileHandle = SetupOpenInfFile(InfName,
                                     NULL,
                                     INF_STYLE_WIN4,
                                     NULL);

    if (InfFileHandle == INVALID_HANDLE_VALUE)
    {
        retError = ERROR_INVALID_PARAMETER;
    }
    else
    {


        //
        // For drivers we ship, the file information is lcoated in layout.inf.
        // So append that file if we need it.
        //

        if (bThirdParty == FALSE)
        {
            SetupOpenAppendInfFile(NULL,
                                   InfFileHandle,
                                   NULL);
        }

        //
        // Initialize the default callback so that the MsgHandler works
        // properly
        //

        Context = SetupInitDefaultQueueCallback(hwnd);

        //
        // Copy all the files to the disk
        //

        if (InstallType == PREINSTALL)
        {
            pSourceListCount = 0;

            if (SetupQuerySourceList(SRCLIST_SYSTEM,
                                     &pSourceList,
                                     &pSourceListCount))
            {
                wsprintf(szSourceLocation,
                         TEXT("%ws\\%ws"),
                         pSourceList[0], WINNT_OEM_DISPLAY_DIR);

                SetupFreeSourceList(&pSourceList, pSourceListCount);
            }
        }

        if (!SetupInstallFromInfSection(hwnd,
                                        InfFileHandle,
                                        copyFileSection,
                                        SPINST_FILES,
                                        NULL,
                                        (InstallType == PREINSTALL) ?
                                            szSourceLocation :
                                            NULL,
                                        0,
                                        &SetupDefaultQueueCallback,
                                        Context,
                                        NULL,
                                        NULL))
        {
            err = GetLastError();
            DbgPrint("Install files Error %d\n", err);

            //
            // User cancelled.  give them a chance to reselect.
            //
            // If they were doing a detect, just exit with the cancel message.
            //

            if ( (err == ERROR_CANCELLED) &&
                 (InstallType == INSTALL) )
            {
                goto reselect;
            }

            return err;
        }

        //
        // If this is the detect key, then it's a list of all the sections
        // we need to run.  So we will have a loop.
        //

        if (InstallType == DETECT)
        {
            if (!SetupFindFirstLine(InfFileHandle,
                                    TEXT("detect.Services"),
                                    NULL,
                                    &infoContext))
            {
                return ERROR_INVALID_PARAMETER;
            }

            //
            // Get the next driver we have to install in a detect case
            //

detect_next:

            if (!SetupGetStringField(&infoContext,
                                     0,
                                     installSection,
                                     sizeof(installSectionData),
                                     NULL))
            {
                return ERROR_INVALID_PARAMETER;
            }
        }

        wsprintf(szServiceSection,
                 TEXT("%ws.Services"),
                 installSection);

        //
        // Formulate a service install section name based on the
        // install name (add .services at the end).
        // Go in this section and get the name of the service that
        // will be installed.
        //

        if (SetupFindFirstLine(InfFileHandle,
                               szServiceSection,
                               NULL,
                               &serviceContext))
        {
            SetupGetStringField(&serviceContext,
                                1,
                                szServiceName,
                                sizeof(szServiceName),
                                NULL);
        }

        if (serviceName)
        {
            wsprintf(serviceName,
                     TEXT("\\Registry\\Machine\\System\\CurrentControlSet\\Services\\%ws"),
                     szServiceName);
        }

        wsprintf(DeviceInst,
                 TEXT("Root\\LEGACY_%ws\\0000"),
                 szServiceName);

        ZeroMemory(&DeviceInfoData, sizeof(SP_DEVINFO_DATA));
        DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        if (SetupDiOpenDeviceInfo(hDevInfo,
                                  DeviceInst,
                                  hwnd,
                                  0,
                                  &DeviceInfoData) ||
            (SetupDiCreateDeviceInfo(hDevInfo,
                                     DeviceInst,
                                     (LPGUID) &GUID_DEVCLASS_DISPLAY,
                                     NULL,
                                     hwnd,
                                     0,
                                     &DeviceInfoData) &&
             SetupDiRegisterDeviceInfo(hDevInfo,
                                       &DeviceInfoData,
                                       0,
                                       NULL,
                                       NULL,
                                       NULL)) )
        {

            //
            // Set the parameters for the installation to do no
            // file copies
            //

            ZeroMemory(&DeviceInstallParams, sizeof(SP_DEVINSTALL_PARAMS));
            DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

            SetupDiGetDeviceInstallParams(hDevInfo,
                                          &DeviceInfoData,
                                          &DeviceInstallParams);

            //
            // Don't copy the files since we already copied them.
            //
            // Don't configure device automatically otherwise the driver
            // can load, and end up generating a Device0 volatile key which
            // which break the next part of install.
            // This can be removed when we get rid of these Device0 instance
            // mechanism.
            //
            // We only need to enumerate the current inf.
            //

            DeviceInstallParams.Flags |= DI_NOFILECOPY |
                                         DI_DONOTCALLCONFIGMG |
                                         DI_ENUMSINGLEINF;

            lstrcpy(DeviceInstallParams.DriverPath,
                    InfName);

            SetupDiSetDeviceInstallParams(hDevInfo,
                                          &DeviceInfoData,
                                          &DeviceInstallParams);

            if (InstallType == DETECT)
            {
                TCHAR DeviceInstId[LINE_LEN];
                ULONG DeviceInstIdLength;

                //
                // Assume we get the hardware id from the detect driver node
                // line in our INF
                //

                if (SetupGetStringField(&infoContext,
                                        1,
                                        DeviceInstId,
                                        sizeof(DeviceInstId),
                                        NULL))
                {
                    //
                    // Make the DeviceInstId a MULTI_SZ.
                    //

                    DeviceInstIdLength = lstrlen(DeviceInstId);
                    DeviceInstId[DeviceInstIdLength] = (TCHAR) 0;
                    DeviceInstId[DeviceInstIdLength + 1] = (TCHAR) 0;

                    SetupDiSetDeviceRegistryProperty(hDevInfo,
                                                     &DeviceInfoData,
                                                     SPDRP_HARDWAREID,
                                                     (PBYTE) DeviceInstId,
                                                     (DeviceInstIdLength + 2) *
                                                         sizeof(TCHAR));
                }

                if (!SetupDiBuildDriverInfoList(hDevInfo,
                                                &DeviceInfoData,
                                                SPDIT_COMPATDRIVER))
                {
                    err = GetLastError();
                    DbgPrint("SetupDiBuildDriverInfoList Detail Error %d\n", err);

                    return ERROR_INVALID_PARAMETER;
                }

                //
                // Now you're guaranteed to have a non-empty list, and the first
                // (i.e., most-compatible) driver node is the one you want, so
                // so select it.
                //

                DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);

                if (!SetupDiEnumDriverInfo(hDevInfo,
                                           &DeviceInfoData,
                                           SPDIT_COMPATDRIVER,
                                           0,
                                           &DriverInfoData))
                {
                    err = GetLastError();
                    DbgPrint("SetupDiEnumDriverInfo Detail Error %d\n", err);

                    return ERROR_INVALID_PARAMETER;
                }
            }
            else
            {
                SetupDiBuildDriverInfoList(hDevInfo,
                                           &DeviceInfoData,
                                           SPDIT_CLASSDRIVER);

                //
                // Special feature that lets us use the original driver
                // description to find the equivalent one in the new
                // list.
                //

                DriverInfoData.Reserved = 0;
            }

            if (!SetupDiSetSelectedDriver(hDevInfo,
                                          &DeviceInfoData,
                                          &DriverInfoData))
            {
                return ERROR_INVALID_PARAMETER;
            }

            if (!SetupDiInstallDevice(hDevInfo,
                                      &DeviceInfoData))
            {
                return ERROR_INVALID_PARAMETER;
            }

            //
            // Make sure the device is enabled in case the setupapis
            // diabled the device for some reason.
            // See LonnyM for possible fix.
            //

            if (SetupDiGetDeviceRegistryProperty(hDevInfo,
                                                 &DeviceInfoData,
                                                 SPDRP_CONFIGFLAGS,
                                                 NULL,
                                                 (LPBYTE) &configFlags,
                                                 sizeof(DWORD),
                                                 NULL) &&
                (configFlags & CONFIGFLAG_DISABLED))
            {
                configFlags &= ~CONFIGFLAG_DISABLED;

                SetupDiSetDeviceRegistryProperty(hDevInfo,
                                                 &DeviceInfoData,
                                                 SPDRP_CONFIGFLAGS,
                                                 (LPBYTE) &configFlags,
                                                 sizeof(DWORD));
            }

            //
            // Destroy these structures
            //

            if (InstallType == DETECT)
            {
                SetupDiDestroyDriverInfoList(hDevInfo,
                                             &DeviceInfoData,
                                             SPDIT_COMPATDRIVER);
            }
            else
            {
                SetupDiDestroyDriverInfoList(hDevInfo,
                                             &DeviceInfoData,
                                             SPDIT_CLASSDRIVER);
            }
        }
        else
        {
            err = GetLastError();
            DbgPrint("Device Install Detail Error %d\n", err);

            return ERROR_INVALID_PARAMETER;
        }

        //
        // Get any interesteing configuration data for the inf file.
        //

        maxmem = 4;
        numDev = 1;
        PreConfigured = 0;
        KeepEnabled = 0;

        wsprintf(szSoftwareSection,
                 TEXT("%ws.GeneralConfigData"),
                 installSection);

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
        // Write the configuration information to the registry.
        //

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
                RegSetValueEx(hkey,
                              TEXT("SystemPages"),
                              0,
                              REG_DWORD,
                              (LPBYTE) &maxmem,
                              sizeof(DWORD));

                RegCloseKey(hkey);
            }
        }

        //
        // We may have to do this for multiple adapters at this point.
        // So loop throught the number of devices, which has 1 as the default
        // value
        //

        do {

            numDev -= 1;

            wsprintf(keyName,
                     TEXT("System\\CurrentControlSet\\Services\\%ws\\Device%d"),
                     szServiceName, numDev);

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
                wsprintf(szSoftwareSection,
                         TEXT("%ws.SoftwareSettings"),
                         installSection);

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
                    return ERROR_INVALID_PARAMETER;
                }

                //
                // Write the description of the device if we are not detecting
                //

                if (InstallType != DETECT)
                {
                    RegSetValueEx(hkey,
                                  TEXT("Device Description"),
                                  0,
                                  REG_SZ,
                                  (LPBYTE) DriverInfoDetailData.DrvDescription,
                                  (lstrlen(DriverInfoDetailData.DrvDescription) + 1) *
                                       sizeof(TCHAR) );
                }

                //
                // Invoke the resource picker for adapters that need it.
                //

                {
                    LOG_CONF LogConfig;
                    RES_DES ResDes;
                    HMODULE hModule;
                    FARPROC PropSheetExtProc;

                    if (CM_Get_First_Log_Conf(&LogConfig,
                                              DeviceInfoData.DevInst,
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
                            PropSheetRequest.DeviceInfoData = &DeviceInfoData;

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
                                                          DeviceInfoData.DevInst,
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

                RegCloseKey(hkey);
            }

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
                     installSection);

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

        if ((InstallType == DETECT) &&
            SetupFindNextLine(&infoContext, &infoContext))
        {
            goto detect_next;
        }

        SetupCloseInfFile(InfFileHandle);
    }

    SetupDiDestroyDriverInfoList(hDevInfo, NULL, SPDIT_CLASSDRIVER);
    SetupDiDestroyDeviceInfoList(hDevInfo);

    return retError;
}


DWORD
InstallNewDriver(
    HWND    hwnd,
    LPCTSTR pszModel,
    BOOL    bDetect,
    PBOOL   pbKeepEnabled
    )
{
    DWORD err;
    LPTSTR keyName;

    if (bDetect)
    {
        LPTSTR psz1;
        int  iRet;

        psz1 = FmtSprint(ID_DSP_TXT_AUTODETECT_PROCEED);

        iRet = FmtMessageBox(hwnd,
                             MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION,
                             FALSE,
                             ID_DSP_TXT_INSTALL_DRIVER,
                             ID_DSP_TXT_AUTODETECT,
                             psz1);

        LocalFree(psz1);

        if (iRet != IDYES)
        {
            return ERROR_CANCELLED;
        }
    }

    err = InstallDriver(hwnd,
                        bDetect ? DETECT : INSTALL,
                        pszModel,
                        NULL,
                        NULL);

    if (err == NO_ERROR)
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
            if (bDetect)
            {
                //
                // Write a key to the registry to tell the display applet to
                // cleanup all the drivers after the autodetect.
                //

                keyName = SZ_DETECT_DISPLAY;
            }
            else
            {
                //
                // Create a registry key that indicates a new display was
                // installed.
                //

                keyName = SZ_NEW_DISPLAY;
            }

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
    else if (err != ERROR_CANCELLED)
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

    return err;
}


DWORD
PreInstallDriver(
    HWND    hwnd,
    LPCTSTR pszModel,
    LPCTSTR pszInf,
    LPTSTR  ServiceName
    )
{
    DWORD err;

    err = InstallDriver(hwnd,
                        PREINSTALL,
                        pszModel,
                        pszInf,
                        ServiceName);

    //
    // Tell the user the driver was not installed properly.
    //

    if (err != NO_ERROR)
    {
        FmtMessageBox(hwnd,
                      MB_ICONSTOP | MB_OK,
                      FALSE,
                      ID_DSP_TXT_INSTALL_DRIVER,
                      ID_DSP_TXT_DRIVER_PREINSTALLED_FAILED);
    }

    return err;
}
