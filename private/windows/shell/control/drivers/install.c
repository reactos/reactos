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
#include <cphelp.h>
#include <stdlib.h>
#include "drivers.h"
#include "sulib.h"

BOOL     GetValidAlias           (HWND, PSTR, PSTR);
BOOL     SelectInstalled         (HWND, PIDRIVER, LPSTR);
void     InitDrvConfigInfo       (LPDRVCONFIGINFO, PIDRIVER );
BOOL     InstallDrivers          (HWND, HWND, PSTR);
void     RemoveAlreadyInstalled  (PSTR, PSTR);
void     CheckIniDrivers         (PSTR, PSTR);
void     RemoveDriverParams      (LPSTR, LPSTR);



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

BOOL InstallDrivers(HWND hWnd, HWND hWndAvail, PSTR pstrKey)
{
    IDRIVER     IDTemplate; // temporary for installing, removing, etc.
    PIDRIVER    pIDriver=NULL;
    int         n,iIndex;
    HWND        hWndI;
    char        szTypes[MAXSTR];
    char        szType[MAXSTR];
    char        szParams[MAXSTR];

    szTypes[0] = '\0';

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

    szTypes[lstrlen(szTypes)-1] = '\0';         // Remove space left at end

    RemoveAlreadyInstalled(IDTemplate.szFile, IDTemplate.szSection);

   /*
    *  At this point we assume the drivers were actually copied.
    *  Now we need to add them to the installed list.
    *  For each driver type we create an IDRIVER and add to the listbox
    */

    hWndI = GetDlgItem(hWnd, LB_INSTALLED);

    for (n = 1; infParseField(szTypes, n, szType); n++)
    {
       /*
        *  Find a valid alias for this device (eg Wave2).  This is
        *  used as the key in the [MCI] or [drivers] section.
        */

        if (GetValidAlias(hWndI, szType, IDTemplate.szSection) == FALSE)
        {
           /*
            *  Exceeded the maximum, tell the user
            */

            PSTR pstrMessage;
            char szApp[MAXSTR];
            char szMessage[MAXSTR];

            LoadString(myInstance,
                       IDS_CONFIGURE_DRIVER,
                       szApp,
                       sizeof(szApp));

            LoadString(myInstance,
                       IDS_TOO_MANY_DRIVERS,
                       szMessage,
                       sizeof(szMessage));

            if (NULL !=
                (pstrMessage =
                    (PSTR)LocalAlloc(LPTR,
                                     sizeof(szMessage) + lstrlen(szType))))
            {
                wsprintf(pstrMessage, szMessage, (LPSTR)szType);

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
            strncpy(pIDriver->szAlias, szType, sizeof(pIDriver->szAlias));
            pIDriver->szAlias[sizeof(pIDriver->szAlias) - 1] = '\0';
            mbstowcs(pIDriver->wszAlias, pIDriver->szAlias, MAX_PATH);


            /*
             *  Want only one instance of each driver to show up in the list
             *  of installed drivers. Thus for the remaining drivers just
             *  place an entry in the drivers section of system.ini
             */


            if ( n > 1) {


                 if (strlen(szParams) != 0 && !pIDriver->KernelDriver) {
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
            } else {

               /*
                *  Add the driver description to our listbox
                */

                iIndex = (int)SendMessage(hWndI,
                                          LB_ADDSTRING,
                                          0,
                                          (LONG)pIDriver->szDesc);

                if (iIndex >= LB_OKAY) {


                   /*
                    *  Our PIDRIVER data is our listbox item
                    */

                    SendMessage(hWndI, LB_SETITEMDATA, iIndex, (LONG)pIDriver);

                   /*
                    *  Reduce to just the driver name
                    */

                    RemoveDriverParams(pIDriver->szFile, szParams);

                    mbstowcs(pIDriver->wszFile, pIDriver->szFile, MAX_PATH);

                    if (strlen(szParams) != 0 && !pIDriver->KernelDriver) {
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

                    if (!SelectInstalled(hWndAvail, pIDriver, szParams))
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

                         SendMessage(hWndI, LB_DELETESTRING, iIndex, 0L);
                         return FALSE;
                    }

                   /*
                    *  for displaying the driver desc. in the restart mesg
                    */

                    if (!bRelated || pIDriver->bRelated) {
                       strcpy(szRestartDrv, pIDriver->szDesc);
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

                    if (bInstallBootLine) {
                        szTemp[MAXSTR];

                        GetPrivateProfileString(szBoot,
                                                szDrivers,
                                                szTemp,
                                                szTemp,
                                                sizeof(szTemp),
                                                szSysIni);
                        strcat(szTemp, " ");
                        strcat(szTemp, pIDriver->szAlias);
                        WritePrivateProfileString(szBoot,
                                                  szDrivers,
                                                  szTemp,
                                                  szSysIni);
                        bInstallBootLine = FALSE;
                    }
#endif // DOBOOT

                } else {
                   /*
                    * Problem getting an alias or adding a driver to the listbox
                    */

                    LocalFree((HANDLE)pIDriver);
                    pIDriver = NULL;
                    return FALSE;               //ERROR
                }
           }
        }
        else
            return FALSE;                       //ERROR
    }


   /*
    *  If no types were added then fail
    */

    if (pIDriver == NULL) {
        return FALSE;
    }

   /*
    *  If there are related drivers listed in the .inf section to install
    *  then install them now by calling ourselves.  Use IDTemplate which
    *  is where mmAddNewDriver put the data.
    */

    if (IDTemplate.bRelated == TRUE) {

        int i;
        char szTemp[MAXSTR];

       /*
        *  Tell file copying to abort rather than put up errors
        */

        bCopyingRelated = TRUE;

        for (i = 1; infParseField(IDTemplate.szRelated, i, szTemp);i++) {

            InstallDrivers(hWnd, hWndAvail, szTemp);
        }
    }
    return TRUE;
}


/************************************************************************
 *
 *  SelectInstalled()
 *
 *  Check if the driver can be configured and configure it if it can be.
 *
 *  hwnd     - Our window - parent for driver to make its config window
 *  pIDriver - info about the driver
 *  params   - the drivers parameters from the .inf file.
 *
 *  Returns FALSE if an error occurred, otherwise TRUE
 *
 ************************************************************************/

BOOL SelectInstalled(HWND hwnd, PIDRIVER pIDriver, LPSTR pszParams)
{
    DRVCONFIGINFO DrvConfigInfo;
    HANDLE hDriver;
    BOOL Success = FALSE;
    DWORD dwTagId;

    wsStartWait();

   /*
    *  If it's a kernel driver call the services controller to
    *  install the driver
    */

    if (pIDriver->KernelDriver) {

        SC_HANDLE SCManagerHandle;
        SC_HANDLE ServiceHandle;
        char ServiceName[MAX_PATH];
        char BinaryPath[MAX_PATH];

       /*
        *  These drivers are not configurable
        */

        pIDriver->fQueryable = 0;

       /*
        *  The services controller will create the registry node to
        *  which we can add the device parameters value
        */

        strcpy(BinaryPath, "\\SystemRoot\\system32\\drivers\\");
        strcat(BinaryPath, pIDriver->szFile);

       /*
        *  First try and obtain a handle to the service controller
        */

        SCManagerHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (SCManagerHandle != NULL) {

            SC_LOCK ServicesDatabaseLock;

           /*
            *  Lock the service controller database to avoid deadlocks
            *  we have to loop because we can't wait
            */


            for (ServicesDatabaseLock = NULL;
                 (ServicesDatabaseLock =
                      LockServiceDatabase(SCManagerHandle))
                    == NULL;
                 Sleep(100)) {
            }

            {
                char drive[MAX_PATH], directory[MAX_PATH], ext[MAX_PATH];
                _splitpath(pIDriver->szFile, drive, directory, ServiceName, ext);
            }


            ServiceHandle = CreateService(SCManagerHandle,
                                          ServiceName,
                                          NULL,
                                          SERVICE_ALL_ACCESS,
                                          SERVICE_KERNEL_DRIVER,
                                          SERVICE_DEMAND_START,
                                          SERVICE_ERROR_NORMAL,
                                          BinaryPath,
                                          "Base",
                                          &dwTagId,
                                          "\0",
                                          NULL,
                                          NULL);

            UnlockServiceDatabase(ServicesDatabaseLock);

            if (ServiceHandle != NULL) {
               /*
                *  Try to write the parameters to the registry if there
                *  are any
                */

                if (strlen(pszParams)) {

                    HKEY ParmsKey;
                    char RegPath[MAX_PATH];
                    strcpy(RegPath, "\\SYSTEM\\CurrentControlSet\\Services\\");
                    strcat(RegPath, ServiceName);
                    strcat(RegPath, "\\Parameters");

                    Success = RegCreateKey(HKEY_LOCAL_MACHINE,
                                           RegPath,
                                           &ParmsKey) == ERROR_SUCCESS &&
                              RegSetValue(ParmsKey,
                                          "",
                                          REG_SZ,
                                          pszParams,
                                          strlen(pszParams)) == ERROR_SUCCESS &&
                              RegCloseKey(ParmsKey) == ERROR_SUCCESS;
                } else {
                    Success = TRUE;
                }

               /*
                *  Service created so try and start it
                */

                if (Success) {
                   /*
                    *  We tell them to restart just in case
                    */

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

                    if (!Success) {
                        char szMesg[MAXSTR];
                        char szMesg2[MAXSTR];
                        char szTitle[50];

                       /*
                        *  Uninstall driver if we couldn't load it
                        */

                       for (ServicesDatabaseLock = NULL;
                            (ServicesDatabaseLock =
                                 LockServiceDatabase(SCManagerHandle))
                               == NULL;
                            Sleep(100)) {
                        }

                        DeleteService(ServiceHandle);

                        UnlockServiceDatabase(ServicesDatabaseLock);

                       /*
                        *  Tell the user there was a configuration error
                        *  (our best guess).
                        */


                        LoadString(myInstance, IDS_DRIVER_CONFIG_ERROR, szMesg, sizeof(szMesg));
                        LoadString(myInstance, IDS_CONFIGURE_DRIVER, szTitle, sizeof(szTitle));
                        wsprintf(szMesg2, szMesg, FileName(pIDriver->szFile));
                        MessageBox(hMesgBoxParent, szMesg2, szTitle, MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
                    }
                }

                CloseServiceHandle(ServiceHandle);
            }

            CloseServiceHandle(SCManagerHandle);
        }
    } else {

       /*
        *  Put up a message if the driver can't be loaded or doesn't
        *  respond favourably to the DRV_INSTALL message.
        */

        BOOL bPutUpMessage;

        bPutUpMessage = FALSE;

       /*
        *  See if we can open the driver
        */

        hDriver = OpenDriver(pIDriver->wszFile, NULL, 0L);

        if (hDriver)
        {
            Success = TRUE;

            InitDrvConfigInfo(&DrvConfigInfo, pIDriver);

           /*
            *  See if activating the driver will require restarting the
            *  system.
            *
            *  Also check the driver wants to install (it may not
            *  have the right privilege level).
            */

            switch (SendDriverMessage(hDriver,
                                  DRV_INSTALL,
                                  0L,
                                  (LONG)(LPDRVCONFIGINFO)&DrvConfigInfo))

            {

            case DRVCNF_RESTART:

                bRestart = TRUE;
                break;

            case DRVCNF_CANCEL:

               /*
                *  The driver did not want to install
                */

                bPutUpMessage = TRUE;
                Success = FALSE;
                break;

            }

           /*
            *  Remember whether the driver is configurable
            */

            pIDriver->fQueryable =
                (int)SendDriverMessage(hDriver,
                                       DRV_QUERYCONFIGURE,
                                       0L,
                                       0L);

           /*
            *  If the driver is configurable then configure it.
            *  Configuring the driver may result in a need to restart
            *  the system.  The user may also cancel install.
            */

            if (pIDriver->fQueryable) {

                switch (SendDriverMessage(
                            hDriver,
                            DRV_CONFIGURE,
                            (LONG)hwnd,
                            (LONG)(LPDRVCONFIGINFO)&DrvConfigInfo)) {

                case DRVCNF_RESTART:
                    bRestart = TRUE;
                    break;

                case DRVCNF_CANCEL:

                   /*
                    *  Don't put up the error box if the user cancelled
                    */

                    Success = FALSE;
                    break;
                }
            }
            CloseDriver(hDriver, 0L, 0L);
        } else {
            bPutUpMessage = TRUE;
            Success = FALSE;
        }

        if (bPutUpMessage) {

           /*
            *  If dealing with the driver resulted in error then put
            *  up a message
            */

            OpenDriverError(hwnd, pIDriver->szDesc, pIDriver->szFile);
        }
    }
    wsEndWait();
    return Success;
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
 *  hwnd         - Window handle - not used
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

BOOL GetValidAlias(HWND hwnd, PSTR pstrType, PSTR pstrSection)
{
    #define MAXDRVTYPES 10

    char *keystr;
    char allkeystr[MAXSTR];
    BOOL found = FALSE;
    int val, maxval = 0, typelen;

    typelen = strlen(pstrType);
    GetPrivateProfileString(pstrSection, NULL, NULL, allkeystr,
                                        sizeof(allkeystr), szSysIni);
    keystr = allkeystr;

   /*
    *  See if we have driver if this type already installed by searching
    *  our the [drivers] section.
    */

    while (*keystr != '\0')
    {
       if (!_strnicmp(keystr, pstrType, typelen) && ((keystr[typelen] > '0' &&
                                                    keystr[typelen] <= '9') ||
                                                    keystr[typelen] == TEXT('\0') ))
       {
          found = TRUE;
          val = atoi(&keystr[typelen]);
          if (val > maxval)
             maxval = val;
       }
       keystr = &keystr[strlen(keystr) + 1];
    }

    if (found)
    {
        if (maxval == MAXDRVTYPES)
            return FALSE; // too many of my type!

        pstrType[typelen] = (char)(maxval + '1');
        pstrType[typelen+1] = '\0';
    }

    return TRUE;
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

        if (pIDriver->KernelDriver) {
            pIDriver->fQueryable = 0;
        } else {

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

void RemoveAlreadyInstalled(PSTR szFile, PSTR szSection)
{
    int iIndex;
    PIDRIVER pIDriver;

    iIndex = (int)SendMessage(hlistbox, LB_GETCOUNT, 0, 0L);

    while ( iIndex-- > 0) {

        pIDriver = (PIDRIVER)SendMessage(hlistbox, LB_GETITEMDATA, iIndex, 0L);

        if ( (int)pIDriver != LB_ERR) {

            if (!FileNameCmp(pIDriver->szFile, szFile)) {
                PostRemove(hlistbox, pIDriver, FALSE, iIndex);
                return;
            }
        }
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

void CheckIniDrivers(PSTR szFile, PSTR szSection)
{
    char allkeystr[MAXSTR * 2];
    char szRemovefile[20];
    char *keystr;

    GetPrivateProfileString(szSection,
                            NULL,
                            NULL,
                            allkeystr,
                            sizeof(allkeystr),
                            szSysIni);

    keystr = allkeystr;
    while (strlen(keystr) > 0)
    {

         GetPrivateProfileString(szSection,
                                 keystr,
                                 NULL,
                                 szRemovefile,
                                 sizeof(szRemovefile),
                                 szSysIni);

         if (!FileNameCmp(szFile, szRemovefile))
               RemoveDriverEntry(keystr, szFile, szSection, FALSE);

         keystr = &keystr[strlen(keystr) + 1];
    }
}

/******************************************************************
 *
 *   RemoveDriverParams
 *
 *   Remove anything after the next token
 *
 ******************************************************************/

void RemoveDriverParams(LPSTR szFile, LPSTR Params)
{
   for(;*szFile == ' '; szFile++);
   for(;*szFile != ' ' && *szFile != '\0'; szFile++);
   if (*szFile == ' ') {
      *szFile = '\0';
      for (;*++szFile == ' ';);
      strcpy(Params, szFile);
   } else {
       *Params = '\0';
   }
}
