/*  REMOVE.C
**
**  Copyright (C) Microsoft, 1990, All Rights Reserved.
**
**  Multimedia Control Panel Applet for removing
**  device drivers.  See the ispec doc DRIVERS.DOC for more information.
**
**  History:
**
**      Thu Oct 17 1991 -by- Sanjaya
**      Created. Originally part of drivers.c
*/

#include <windows.h>
#include <mmsystem.h>
#include <winsvc.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <cpl.h>
#include <cphelp.h>
#include <regstr.h>

#include "drivers.h"
#include "sulib.h"

BOOL     SetValidAlias      (LPTSTR, LPTSTR);

#ifdef DOBOOT
BOOL     FindBootDriver     (TCHAR *);
PSTR     strstri            (LPTSTR, LPTSTR);
#endif // DOBOOT

/*
 *  RemoveService(szFile)
 *
 *  Remove the service corresponding to the file szFile
 *
 *  returns TRUE if successful, FALSE otherwise
 */

BOOL RemoveService(LPTSTR szFile)
{
    SC_HANDLE SCManagerHandle;
    SC_HANDLE ServiceHandle;
    TCHAR ServiceName[MAX_PATH];
    BOOL Status = FALSE;

    /*
     *  Extract the service name from the file name
     */

    {
        TCHAR drive[MAX_PATH], directory[MAX_PATH], ext[MAX_PATH];
        lsplitpath(szFile, drive, directory, ServiceName, ext);
    }

    /*
     *  First try and obtain a handle to the service controller
     */

    SCManagerHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (SCManagerHandle == NULL)
    {

        TCHAR szMesg[MAXSTR];
        TCHAR szMesg2[MAXSTR];

        LoadString(myInstance, IDS_INSUFFICIENT_PRIVILEGE, szMesg, sizeof(szMesg)/sizeof(TCHAR));
        wsprintf(szMesg2, szMesg, szFile);
        MessageBox(hMesgBoxParent, szMesg2, szRemove, MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
        return FALSE;
    }


    ServiceHandle = OpenService(SCManagerHandle,
                                ServiceName,
                                SERVICE_ALL_ACCESS);
    if (ServiceHandle != NULL)
    {
        SERVICE_STATUS ServiceStatus;
        SC_LOCK ServicesDatabaseLock;

        /*
         *  Stop the service if possible.
         */

        ControlService(ServiceHandle, SERVICE_CONTROL_STOP, &ServiceStatus);

        /*
         *  Delete the service.
         *  We aren't detecting if we can just carry on.
         */

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

        Status = DeleteService(ServiceHandle);

        UnlockServiceDatabase(ServicesDatabaseLock);

        CloseServiceHandle(ServiceHandle);
    }
    else
    {

        /*
         *  It's possible there was no services entry so the driver
         *  wasn't really installed after all.
         */

        LONG Error = GetLastError();

        if (Error == ERROR_FILE_NOT_FOUND ||
            Error == ERROR_PATH_NOT_FOUND ||
            Error == ERROR_SERVICE_DOES_NOT_EXIST)
        {
            Status = TRUE;
        }
    }

    CloseServiceHandle(SCManagerHandle);

    return Status;
}

/*
**  PostRemove()
**
**  Mark an installed driver for removal later AND remove the driver's entry
**  in SYSTEM.INI to avoid conflicts when we add or remove later.
*/
LONG_PTR PostRemove(PIDRIVER pIDriver, BOOL bLookAtRelated)
{

    TCHAR *keystr;
    TCHAR allkeystr[MAXSTR];
    TCHAR szfile[MAX_PATH];
    HANDLE hDriver;
    LONG_PTR Status = DRVCNF_CANCEL;
    LPTSTR pstr;


    GetPrivateProfileString(pIDriver->szSection,
                            pIDriver->szAlias,
                            pIDriver->szFile,
                            pIDriver->szFile,
                            MAX_PATH,
                            szSysIni);


    /*
     *  Remove parameters from file name
     */


    for ( pstr=pIDriver->szFile; *pstr && (*pstr!=COMMA) &&
        (*pstr!=SPACE); pstr++ )
        ;
    *pstr = TEXT('\0');

    if (bLookAtRelated && (!bRelated || pIDriver->bRelated))
        wcscpy(szRestartDrv,  pIDriver->szDesc);

    /*
     *  If it's a kernel driver remove it from the config registry
     *  and services controller
     */

    if (pIDriver->KernelDriver)
    {

        Status = RemoveService(pIDriver->szFile) ? DRVCNF_RESTART : DRVCNF_CANCEL;

        if (Status == DRVCNF_CANCEL)
        {
            return DRVCNF_CANCEL;
        }

    }
    else
    {

        hDriver = OpenDriver(pIDriver->wszAlias, pIDriver->wszSection, 0L);

        if (hDriver)
        {

            /*
             *  Removal can fail so don't mark as deleted in this case
             */

            Status = SendDriverMessage(hDriver, DRV_REMOVE, 0L, 0L);
            CloseDriver(hDriver, 0L, 0L);

            if (Status == DRVCNF_CANCEL)
            {
                return DRVCNF_CANCEL;
            }
        }
    }

    // Remove the driver from the treeview,
    //  but don't free its structure
    //
    RemoveIDriver (hAdvDlgTree, pIDriver, FALSE);

    if (bLookAtRelated)
    {
        TCHAR allkeystr[MAXSTR];

        if (GetPrivateProfileString(szRelatedDesc, pIDriver->szAlias,
                                    allkeystr, allkeystr, sizeof(allkeystr) / sizeof(TCHAR), szControlIni))
        {
            int  i;
            TCHAR szTemp[MAXSTR];

            for (i = 1; infParseField(allkeystr, i, szTemp);i++)
            {
                PIDRIVER pid;

                if ((pid = FindIDriverByName (szTemp)) != NULL)
                {
                    if (PostRemove (pid, FALSE) == DRVCNF_RESTART)
                    {
                        Status = DRVCNF_RESTART;
                    }
                }
            }
        }
    }

    // Remove the driver entry from SYSTEM.INI so you don't
    // conflict with other drivers.

    GetPrivateProfileString(pIDriver->szSection, NULL, NULL,
                            allkeystr, sizeof(allkeystr) / sizeof(TCHAR), szSysIni);
    keystr = allkeystr;
    while (wcslen(keystr) > 0)
    {
        GetPrivateProfileString(pIDriver->szSection, keystr, NULL, szfile, sizeof(szfile) / sizeof(TCHAR), szSysIni);
        if (!FileNameCmp(pIDriver->szFile, szfile))
            RemoveDriverEntry(keystr, pIDriver->szFile, pIDriver->szSection, bLookAtRelated);
        keystr = &keystr[wcslen(keystr) + sizeof(TCHAR)];
    }
    return Status;
}



void RemoveDriverEntry (LPTSTR szKey, LPTSTR szFile, LPTSTR szSection, BOOL bLookAtRelated)
{

    /*
     *  Remove entry for loading driver
     */

    WritePrivateProfileString(szSection, szKey, NULL, szSysIni);

    /*
     *  Delete entry for parameters for this driver
     */

    WriteProfileString(szFile, szKey, NULL);

    /*
     *  Remove entry which says this is a user driver (as opposed to
     *  a pre-installed one).
     */

    WritePrivateProfileString(szUserDrivers, szKey, NULL, szControlIni);

    /*
     *  Remove description
     */

    WritePrivateProfileString(szDriversDesc, szFile, NULL, szControlIni);

    /*
     *  Remove links to related drivers
     */

    WritePrivateProfileString(szRelatedDesc, szKey, NULL, szControlIni);

#ifdef DOBOOT
    FindBootDriver(szKey);
#endif // DOBOOT

    if (bLookAtRelated)
        SetValidAlias(szKey, szSection);
}


/*
 *  SetValidAlias()
 *
 * Check to see if the alias removed would create a hole in the device
 * numbering scheme. If so switch the last device number with the deleted one.
 */
BOOL SetValidAlias(LPTSTR pstrType, LPTSTR pstrSection)
{
    TCHAR *keystr;
    static TCHAR allkeystr[MAXSTR];
    static TCHAR szExKey[MAXSTR], szExFile[MAXSTR], szExDesc[MAXSTR];
    BOOL bfound = FALSE, bExchange = FALSE;
    int val, maxval = 0, typelen, len;

    /*
     *  Getting length of alias
     */

    len = typelen = wcslen(pstrType);

    // If the last TCHAR on the type is a number don't consider it

    if (pstrType[typelen - 1] > TEXT('0') && pstrType[typelen - 1] <= TEXT('9'))
        typelen--;

    // Get all the aliases in the drivers section

    GetPrivateProfileString(pstrSection, NULL, NULL, allkeystr,
                            sizeof(allkeystr) / sizeof(TCHAR), szSysIni);
    keystr = allkeystr;
    while (*keystr != TEXT('\0'))
    {
        // Compare the root of the aliases
        if (!_wcsnicmp(keystr, pstrType, typelen) && ((keystr[typelen] <= TEXT('9') && keystr[typelen] > TEXT('0')) || keystr[typelen] == TEXT('\0')))
        {

            //We found a common alias
            bfound = TRUE;
            val = _wtoi(&keystr[typelen]);
            if (val > maxval)
            {
                maxval = val;
                wcscpy(szExKey, keystr);
            }
        }
        //Pointer to next alias
        keystr = &keystr[wcslen(keystr) + sizeof(TCHAR)];
    }
    //If we found one
    if (bfound)
    {
        if (len == typelen)
            bExchange = TRUE;
        else
            if (_wtoi(&pstrType[typelen]) < maxval)
            bExchange = TRUE;

        // We need to exchange it with the one we found
        if (bExchange)
        {
            //Exchanging the one in the drivers section in system.ini
            GetPrivateProfileString(pstrSection, szExKey, NULL, szExFile,
                                    sizeof(szExFile) / sizeof(TCHAR), szSysIni);
            WritePrivateProfileString(pstrSection, szExKey, NULL, szSysIni);
            WritePrivateProfileString(pstrSection, pstrType, szExFile, szSysIni);

#ifdef TRASHDRIVERDESC
            //Exchanging the one in the drivers description section of control.ini
            GetPrivateProfileString(szDriversDesc, szExKey, NULL, szExDesc, sizeof(szExFile) / sizeof(TCHAR), szControlIni);
            WritePrivateProfileString(szDriversDesc, szExKey, NULL, szControlIni);
            WritePrivateProfileString(szDriversDesc, pstrType, szExDesc, szControlIni);
#endif

            //If any related drivers were present under old alias switch them
            GetPrivateProfileString(szRelatedDesc, szExKey, NULL, szExDesc, sizeof(szExFile) / sizeof(TCHAR), szControlIni);

            if (wcslen(szExDesc))
            {
                WritePrivateProfileString(szRelatedDesc, szExKey, NULL, szControlIni);
                WritePrivateProfileString(szRelatedDesc, pstrType, szExDesc, szControlIni);
            }

            //If user installed driver under old alias switch them
            GetPrivateProfileString(szUserDrivers, szExKey, NULL, szExDesc, sizeof(szExFile) / sizeof(TCHAR), szControlIni);

            if (wcslen(szExDesc))
            {
                WritePrivateProfileString(szUserDrivers, szExKey, NULL, szControlIni);
                WritePrivateProfileString(szUserDrivers, pstrType, szExDesc, szControlIni);
            }

#ifdef DOBOOT
            if (FindBootDriver(szExKey))
            {
                static TCHAR szTemp[MAXSTR];

                GetPrivateProfileString(szBoot, szDrivers, szTemp, szTemp,
                                        sizeof(szTemp) / sizeof(TCHAR), szSysIni);
                strcat(szTemp, TEXT(" "));
                strcat(szTemp, pstrType);
                WritePrivateProfileString(szBoot, szDrivers, szTemp, szSysIni);
            }
#endif // DOBOOT

        }
    }
    return(bExchange);
}

int FileNameCmp(TCHAR far *pch1, TCHAR far *pch2)
{
    LPTSTR pchEOS;

    while (*pch1 == TEXT(' ')) pch1++; // eat spaces
    while (*pch2 == TEXT(' ')) pch2++; // eat spaces

    for (pchEOS = pch1; *pchEOS && *pchEOS != TEXT(' '); pchEOS++);

    return _wcsnicmp(pch1, pch2, (size_t)(pchEOS - pch1));
}

#ifdef DOBOOT

PSTR strstri(LPTSTR pszStr, LPTSTR pszKey)
{
    while (pszStr)
        if (!_strnicmp(pszStr, pszKey, lstrlen(pszKey)))
            return(pszStr);
        else
            pszStr++;
    return(NULL);
}

/*
 *   FindBootDriver()
 *  Checks to see if the driver alias is on the drivers line of the
 *  boot section. If so the alias is removed from the line.
 */

BOOL FindBootDriver(TCHAR *szKey)
{
    TCHAR *ptr;
    int wKeyLen = (int)wcslen(szKey);
    TCHAR *endkey;
    static TCHAR szDriverline[MAXSTR];

    GetPrivateProfileString(TEXT("boot"), TEXT("drivers"), szDriverline, szDriverline,
                            MAX_PATH, szSysIni);
    ptr = strstri(szDriverline, szKey);
    if (ptr)
    {

        if ((((ptr != szDriverline) && (*(ptr - 1) == TEXT(' ') )) ||
             (ptr == szDriverline)) &&
            (*(ptr + wKeyLen) == TEXT(' ') || *(ptr + wKeyLen) == NULL))
        {
            endkey = ptr + wKeyLen;
            while (*endkey)
                *ptr++ = *endkey++;
            *ptr = NULL;
            WritePrivateProfileString(TEXT("boot"), TEXT("drivers"), szDriverline,
                                      szSysIni);
            return(TRUE);
        }
    }
    return(FALSE);
}

#endif // DOBOOT

// Steal use of function in midi.c to delete a reg subtree.
LONG SHRegDeleteKey(HKEY hKey, LPCTSTR lpSubKey);

//****************************************************************************
// Function: mystrtok()
//
// Purpose: Returns a pointer to the next token in a string.
//
// Parameters:
//      SrcString   String containing token(s)
//      Seps        Set of delimiter characters
//      State       Pointer to a char* to hold state info
// Return Code:
//      Ptr to next token, or NULL if no tokens left
//
// Comments:
//      Fixes problem with standard strtok, which can't be called recursively.
//
//****************************************************************************
LPTSTR mystrtok(LPTSTR SrcString, LPCTSTR Seps, LPTSTR FAR *State)
{
    LPTSTR ThisString;
    LPTSTR NextString;

    // If Seps is NULL, use default separators
    if (!Seps)
    {
        Seps = TEXT(" ,\t");  // space, comma, tab chars
    }

    if (SrcString)
        ThisString = SrcString;
    else
        ThisString = *State;

    // Find beginning of the current string
    ThisString = ThisString + wcsspn(ThisString,Seps);
    if (ThisString[0]==TEXT('\0'))
        return NULL;

    // Find the end of the current string
    NextString = ThisString + wcscspn(ThisString,Seps);
    if (NextString[0]!=TEXT('\0'))
    {
        *NextString++=TEXT('\0');
    }

    *State = NextString;
    return ThisString;
}

BOOL RemoveDriver(IN HDEVINFO         DeviceInfoSet,
                  IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
                 )
{
    BOOL bRet = FALSE;          // Return value

    TCHAR szDriverKey[MAX_PATH]; // Value of Driver's key in Enum branch
    TCHAR *pszDrvInst;           // Driver's instance, e.g. "0000", "0001", etc.

    HKEY hkDevReg  = NULL;      // Key to Driver portion of registry (e.g. classguid\0000)
    HKEY hkDrivers32 = NULL;    // Key to Drivers32 portion of registry
    HKEY hkDrivers = NULL;      // Key to classguid\0000\Drivers

    TCHAR szSubClasses[256];     // List of subclasses to process
    TCHAR *strtok_State;         // strtok state
    TCHAR *pszClass;             // Information about e.g. classguid\0000\Drivers\wave
    HKEY hkClass;

    DWORD idxR3DriverName;      // Information about e.g. classguid\0000\Drivers\wave\foo.drv
    HKEY hkR3DriverName;
    TCHAR szR3DriverName[64];

    TCHAR szAlias[64];           // Alias in Drivers32 (e.g. wave1)
    TCHAR szDriver[64];          // Name of driver

    DWORD cbLen;

    // Get the Drivers key value under the device's Enum branch,
    // e.g. something like "{4D36E96C-E325-11CE-BFC1-08002BE10318}\0000"
    SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                     DeviceInfoData,
                                     SPDRP_DRIVER ,
                                     NULL,
                                     (LPBYTE)szDriverKey,
                                     MAX_PATH,
                                     NULL);

    // Get everything after the last \ character
    pszDrvInst = wcsrchr(szDriverKey,TEXT('\\'));
    if (!pszDrvInst)
    {
        goto RemoveDrivers32_exit;
    }
    pszDrvInst++;
    // Now pszDrvInst points to a string with the Driver Instance, e.g. "0000"

    // Open the Drivers32 section of the registry
    if (RegOpenKey(HKEY_LOCAL_MACHINE,
                   TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Drivers32"),
                   &hkDrivers32))
    {
        goto RemoveDrivers32_exit;
    }

    // Open the Driver reg key
    hkDevReg = SetupDiOpenDevRegKey(DeviceInfoSet,
                                    DeviceInfoData,
                                    DICS_FLAG_GLOBAL,
                                    0,
                                    DIREG_DRV,
                                    KEY_ALL_ACCESS);
    if (hkDevReg == INVALID_HANDLE_VALUE)
    {
        goto RemoveDrivers32_exit;
    }

    // Enumerate through supporter classes in the Drivers subkey
    if (RegOpenKey(hkDevReg, TEXT("Drivers"), &hkDrivers))
    {
        goto RemoveDrivers32_exit;
    }

    // Read the SubClasses key to determine which subclasses to process
    cbLen=sizeof(szSubClasses);
    if (RegQueryValueEx(hkDrivers, TEXT("Subclasses"), NULL, NULL, (LPBYTE)szSubClasses, &cbLen))
    {
        goto RemoveDrivers32_exit;
    }

    // Enumerate all the subclasses
    for (
        pszClass = mystrtok(szSubClasses,NULL,&strtok_State);
        pszClass;
        pszClass = mystrtok(NULL,NULL,&strtok_State)
        )
    {
        // Open up each subclass
        if (RegOpenKey(hkDrivers, pszClass, &hkClass))
        {
            continue;
        }

        // Under each class is a set of driver name subkeys.
        // For each driver (e.g. foo1.drv, foo2.drv, etc.)
        for (idxR3DriverName = 0;
            !RegEnumKey(hkClass, idxR3DriverName, szR3DriverName, sizeof(szR3DriverName)/sizeof(TCHAR));
            idxR3DriverName++)
        {

            // Open the key to the driver name
            if (RegOpenKey(hkClass, szR3DriverName, &hkR3DriverName))
            {
                continue;
            }

            // Get the value of Driver under the driver name key
            cbLen = sizeof(szDriver);
            if (!RegQueryValueEx(hkR3DriverName, TEXT("Driver"), NULL, NULL, (LPBYTE)szDriver, &cbLen))
            {
                // Send the driver a DRV_REMOVE message to the driver
                HANDLE hDriver;

                hDriver = OpenDriver(szDriver, NULL, 0L);

                if (hDriver)
                {
                    SendDriverMessage(hDriver, DRV_REMOVE, 0L, 0L);
                    CloseDriver(hDriver, 0L, 0L);
                }
            }

            // Get the value of Alias under the driver name key
            cbLen = sizeof(szAlias);
            if (!RegQueryValueEx(hkR3DriverName, TEXT("Alias"), NULL, NULL, (LPBYTE)szAlias, &cbLen))
            {
                // Delete the corresponding entry in Drivers32
                RegDeleteValue(hkDrivers32,szAlias);
            }

            // Close the Driver Name key
            RegCloseKey(hkR3DriverName);
        }
        // Close the class key
        RegCloseKey(hkClass);
    }

    bRet = TRUE;

    RemoveDrivers32_exit:

    if (hkDrivers32)    RegCloseKey(hkDrivers32);
    if (hkDevReg)       RegCloseKey(hkDevReg);
    if (hkDrivers)      RegCloseKey(hkDrivers);

    return bRet;
}

// The driver's private registry section is located in something like:
// HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\{4D36E96C-E325-11CE-BFC1-08002BE10318}\xxxx
// where xxxx is the device instance (e.g. 0000, 0001, etc.)
// These last four digits are used to index into the driver's MediaResources section.

// For example, suppose a device has a driver instance
// HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\{4D36E96C-E325-11CE-BFC1-08002BE10318}\0001
// and under that entry there is a Drivers\wave\foo.drv, meaning that the foo.drv driver supports a wave
// API.
// In this case, there would be an entry in
// HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\MediaResources\wave\foo.drv<0001>
//
// On removal, we need to delete that entry.
BOOL RemoveMediaResources(IN HDEVINFO         DeviceInfoSet,
                          IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
                         )


{
    BOOL bRet = FALSE;          // Return value

    TCHAR szDriverKey[MAX_PATH]; // Value of Driver's key in Enum branch
    TCHAR *pszDrvInst;           // Driver's instance, e.g. "0000", "0001", etc.

    HKEY hkDevReg  = NULL;      // Key to Driver portion of registry (e.g. classguid\0000)
    HKEY hkDrivers = NULL;      // Key to classguid\0000\Drivers
    HKEY hkMR      = NULL;      // Handle to MediaResources section

    TCHAR szSubClasses[256];     // List of subclasses to process
    TCHAR *strtok_State;         // strtok state
    TCHAR *pszClass;             // Information about e.g. classguid\0000\Drivers\wave
    HKEY hkClass;

    DWORD idxR3DriverName;      // Information about e.g. classguid\0000\Drivers\wave\foo.drv
    HKEY hkR3DriverName;
    TCHAR szR3DriverName[64];

    TCHAR szDriver[64];          // Driver name (e.g. foo.drv)
    DWORD cbLen;                // Size of szDriver

    TCHAR szDevNode[MAX_PATH+1];         // Path to driver's reg entry
    TCHAR szSoftwareKey[MAX_PATH+1];     // Value of SOFTWAREKEY

    // Open Media Resources section of registry
    if (RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_MEDIARESOURCES, &hkMR))
    {
        goto RemoveMediaResources_exit;
    }

    // Get the Drivers key value under the device's Enum branch,
    // e.g. something like "{4D36E96C-E325-11CE-BFC1-08002BE10318}\0000"
    SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                     DeviceInfoData,
                                     SPDRP_DRIVER ,
                                     NULL,
                                     (LPBYTE)szDriverKey,
                                     MAX_PATH,
                                     NULL);

    // Get everything after the last \ character
    pszDrvInst = wcsrchr(szDriverKey,TEXT('\\'));
    if (!pszDrvInst)
    {
        goto RemoveMediaResources_exit;
    }
    pszDrvInst++;
    // Now pszDrvInst points to a string with the Driver Instance, e.g. "0000"

    // Get full path to driver key
    wsprintf(szDevNode,
             TEXT("%s\\%s"),
             REGSTR_PATH_CLASS_NT,
             (LPTSTR)szDriverKey);

    // Open the Driver reg key
    hkDevReg = SetupDiOpenDevRegKey(DeviceInfoSet,
                                    DeviceInfoData,
                                    DICS_FLAG_GLOBAL,
                                    0,
                                    DIREG_DRV,
                                    KEY_ALL_ACCESS);
    if (hkDevReg == INVALID_HANDLE_VALUE)
    {
        goto RemoveMediaResources_exit;
    }

    // Enumerate through supporter classes in the Drivers subkey
    if (RegOpenKey(hkDevReg, TEXT("Drivers"), &hkDrivers))
    {
        goto RemoveMediaResources_exit;
    }

    // Read the SubClasses key to determine which subclasses to process
    cbLen=sizeof(szSubClasses);
    if (RegQueryValueEx(hkDrivers, TEXT("Subclasses"), NULL, NULL, (LPBYTE)szSubClasses, &cbLen))
    {
        goto RemoveMediaResources_exit;
    }

    // Enumerate all the subclasses
    for (
        pszClass = mystrtok(szSubClasses,NULL,&strtok_State);
        pszClass;
        pszClass = mystrtok(NULL,NULL,&strtok_State)
        )
    {
        if (RegOpenKey(hkDrivers, pszClass, &hkClass))
        {
            continue;
        }

        // Under each class is a set of driver name subkeys.
        // For each driver (e.g. foo1.drv, foo2.drv, etc.)
        for (idxR3DriverName = 0;
            !RegEnumKey(hkClass, idxR3DriverName, szR3DriverName, sizeof(szR3DriverName)/sizeof(TCHAR));
            idxR3DriverName++)
        {

            // Open the key to the driver name
            if (RegOpenKey(hkClass, szR3DriverName, &hkR3DriverName))
            {
                continue;
            }

            // Get the value of Driver in under the driver name key
            cbLen = sizeof(szDriver);
            if (!RegQueryValueEx(hkR3DriverName, TEXT("Driver"), NULL, NULL, (LPBYTE)szDriver, &cbLen))
            {
                TCHAR szR3Path[256];

                // Create a path to the MediaResources entry to be deleted
                wsprintf(szR3Path,
                         TEXT("%s\\%s\\%s<%s>"),
                         REGSTR_PATH_MEDIARESOURCES,
                         (LPTSTR)pszClass,
                         (LPTSTR)szDriver,
                         (LPTSTR)pszDrvInst);
                // Delete the key
                SHRegDeleteKey(HKEY_LOCAL_MACHINE, szR3Path);
            }
            // Close the Driver Name key
            RegCloseKey(hkR3DriverName);
        }

        // Close the class key in the devnode
        RegCloseKey(hkClass);

        // Backup mechanism, in case we missed something.
        // This shouldn't be necessary, but Win98 does it.

        // Open the class key in MediaResources
        if (RegOpenKey(hkMR, pszClass, &hkClass))
        {
            continue;
        }
        // Count the number of subkeys under the class key
        // We're gonna do this backwards because we'll be deleting keys later
        for (idxR3DriverName = 0;
            !RegEnumKey(hkClass, idxR3DriverName, szR3DriverName, sizeof(szR3DriverName)/sizeof(TCHAR));
            idxR3DriverName++)
        {
            ;
        }

        // For each driver subkey, working backwards.
        // Subkeys are e.g. msacm.iac2, msacm.imaadpcm, etc.
        for (idxR3DriverName--;
            ((int)idxR3DriverName >= 0) &&
            !RegEnumKey(hkClass, idxR3DriverName, szR3DriverName, sizeof(szR3DriverName)/sizeof(TCHAR));
            idxR3DriverName--)
        {
            // Open the driver key
            if (RegOpenKey(hkClass, szR3DriverName, &hkR3DriverName))
            {
                continue;
            }

            // Query the value of "SOFTWAREKEY"
            szSoftwareKey[0]=TEXT('\0');      // Init to safe value in case call fails

            cbLen = sizeof(szSoftwareKey);
            RegQueryValueEx(hkR3DriverName, TEXT("SOFTWAREKEY"), NULL, NULL, (LPBYTE)szSoftwareKey, &cbLen);

            // Close now, since we might delete in next line
            RegCloseKey(hkR3DriverName);

            // If the value of "SOFTWAREKEY" matches the path to the devnode, delete the key
            if (!lstrcmpi(szSoftwareKey, szDevNode))
            {
                SHRegDeleteKey(hkClass, szR3DriverName);
            }
        }

        // Close the class key in MediaResources
        RegCloseKey(hkClass);
    }

    bRet = TRUE;

    RemoveMediaResources_exit:
    if (hkDevReg)   RegCloseKey(hkDevReg);
    if (hkDrivers)  RegCloseKey(hkDrivers);
    if (hkMR)       RegCloseKey(hkMR);

    return bRet;
}

// Clear out entries in the Driver's branch of the registry, e.g. in {4D36E96C-E325-11CE-BFC1-08002BE10318}\0000
BOOL RemoveDriverInfo(IN HDEVINFO         DeviceInfoSet,
                      IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
                     )
{
    HKEY hkDevReg;      // Key to Driver portion of registry (e.g. classguid\0000)

    // Remove entries in the driver's reg section
    hkDevReg = SetupDiOpenDevRegKey(DeviceInfoSet,
                                    DeviceInfoData,
                                    DICS_FLAG_GLOBAL,
                                    0,
                                    DIREG_DRV,
                                    KEY_ALL_ACCESS);
    if (hkDevReg == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    // Delete any entries that might cause trouble
    RegDeleteValue(hkDevReg,REGSTR_VAL_DEVLOADER);
    RegDeleteValue(hkDevReg,REGSTR_VAL_DRIVER);
    RegDeleteValue(hkDevReg,REGSTR_VAL_ENUMPROPPAGES);
    RegDeleteValue(hkDevReg,TEXT("NTMPDriver"));
    RegDeleteValue(hkDevReg,TEXT("AssociatedFilters"));
    RegDeleteValue(hkDevReg,TEXT("FDMA"));
    RegDeleteValue(hkDevReg,TEXT("DriverType"));

    // Blow away the Drivers subtree
    SHRegDeleteKey(hkDevReg,TEXT("Drivers"));

    // For future use, allow a key under which everything gets blown away
    SHRegDeleteKey(hkDevReg,TEXT("UnretainedSettings"));

    RegCloseKey(hkDevReg);

    return TRUE;
}

// Clear out entries in the Device's Enum branch of the registry:
BOOL RemoveDeviceInfo(IN HDEVINFO         DeviceInfoSet,
                      IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
                     )
{
    // Remove the Driver key. It looks something like "Driver = {4D36E96C-E325-11CE-BFC1-08002BE10318}\0000"
    // !!NO don't remove driver key, or else on driver upgrade system loses track of node & creates a new one
    //    SetupDiSetDeviceRegistryProperty(DeviceInfoSet, DeviceInfoData, SPDRP_DRIVER , NULL, 0);

    // Remove the Service key.
    // Make sure before doing anything else that there is no service property for
    // this device instance.  This allows us to know whether we should clean up the
    // device instance if we boot and find that it's no longer present.
    SetupDiSetDeviceRegistryProperty(DeviceInfoSet, DeviceInfoData, SPDRP_SERVICE, NULL, 0);

    return TRUE;
}


/* 5/14/98 andyraf for NT5 */
/* Media_RemoveDevice
 *
 * This function gets called on driver removal (DIF_REMOVE) and driver installation (DIF_INSTALL).
 * It cleans up all the registry entries associated with the driver.
 */
DWORD Media_RemoveDevice(IN HDEVINFO         DeviceInfoSet,
                         IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
                        )
{
#if 0
    // Don't know how to do this on NT5
    if ((diFunction == DIF_REMOVE) &&
        (lpdi->Flags & DI_CLASSINSTALLPARAMS) &&
        (((LPREMOVEDEVICE_PARAMS)lpdi->lpClassInstallParams)->dwFlags & DI_REMOVEDEVICE_CONFIGSPECIFIC))
    {
        return ERROR_DI_DO_DEFAULT;
    }

    // Not needed on NT5??
    CleanupDummySysIniDevs();           //remove the wave=*.drv and midi=*.drv dummy devices.
#endif

#if 0   // We'll allow people to remove these drivers for now
    if (IsSpecialDriver(DeviceInfoSet, DeviceInfoData))
    {
        return NO_ERROR;
    }
#endif

    // Send DRV_REMOVE to each driver and clean out Drivers32 section of registry
    RemoveDriver        (DeviceInfoSet, DeviceInfoData);

    // Clean out MediaResources section of registry
    RemoveMediaResources(DeviceInfoSet, DeviceInfoData);

    // Clean out driver's classguid\instance section of registry
    RemoveDriverInfo    (DeviceInfoSet, DeviceInfoData);

    // Clean out device's enum section of registry
    RemoveDeviceInfo    (DeviceInfoSet, DeviceInfoData);

    return ERROR_DI_DO_DEFAULT;
}

#if 0   // Unused at present
BOOL AddDrivers32(IN HDEVINFO         DeviceInfoSet,
                  IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
                 )
{
    BOOL bRet = FALSE;          // Return value

    TCHAR szDriverKey[MAX_PATH]; // Value of Driver's key in Enum branch
    TCHAR *pszDrvInst;           // Driver's instance, e.g. "0000", "0001", etc.

    HKEY hkDevReg  = NULL;      // Key to Driver portion of registry (e.g. classguid\0000)
    HKEY hkDrivers32 = NULL;    // Key to Drivers32 portion of registry
    HKEY hkDrivers = NULL;      // Key to classguid\0000\Drivers
    HKEY hkDriversDesc = NULL;  // Key to drivers.desc portion of registry

    TCHAR szSubClasses[256];     // List of subclasses to process
    TCHAR *strtok_State;         // strtok state
    TCHAR *pszClass;             // Information about e.g. classguid\0000\Drivers\wave
    HKEY hkClass;

    DWORD idxR3DriverName;      // Information about e.g. classguid\0000\Drivers\wave\foo.drv
    HKEY hkR3DriverName;
    TCHAR szR3DriverName[64];

    TCHAR szAlias[64];           // Alias in Drivers32 (e.g. wave1)

    TCHAR szDriver[64];          // Name of driver
    TCHAR szDescription[MAX_PATH];

    DWORD cbLen;

    // Get the Drivers key value under the device's Enum branch,
    // e.g. something like "{4D36E96C-E325-11CE-BFC1-08002BE10318}\0000"
    SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                     DeviceInfoData,
                                     SPDRP_DRIVER ,
                                     NULL,
                                     (LPBYTE)szDriverKey,
                                     MAX_PATH,
                                     NULL);

    // Get everything after the last \ character
    pszDrvInst = strrchr(szDriverKey,TEXT('\\'));
    if (!pszDrvInst)
    {
        goto RemoveDrivers32_exit;
    }
    pszDrvInst++;
    // Now pszDrvInst points to a string with the Driver Instance, e.g. "0000"

    // Open the Drivers32 section of the registry
    if (RegOpenKey(HKEY_LOCAL_MACHINE,
                   TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Drivers32"),
                   &hkDrivers32))
    {
        goto RemoveDrivers32_exit;
    }

    // If we're adding a driver, need to open key to drivers.desc also
    if (RegOpenKey(HKEY_LOCAL_MACHINE,
                   TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\drivers.desc"),
                   &hkDriversDesc))
    {
        goto RemoveDrivers32_exit;
    }

    // Open the Driver reg key
    hkDevReg = SetupDiOpenDevRegKey(DeviceInfoSet,
                                    DeviceInfoData,
                                    DICS_FLAG_GLOBAL,
                                    0,
                                    DIREG_DRV,
                                    KEY_ALL_ACCESS);
    if (hkDevReg == INVALID_HANDLE_VALUE)
    {
        goto RemoveDrivers32_exit;
    }

    // Enumerate through supporter classes in the Drivers subkey
    if (RegOpenKey(hkDevReg, TEXT("Drivers"), &hkDrivers))
    {
        goto RemoveDrivers32_exit;
    }

    // Read the SubClasses key to determine which subclasses to process
    cbLen=sizeof(szSubClasses);
    if (RegQueryValueEx(hkDrivers, TEXT("Subclasses"), NULL, NULL, (LPBYTE)szSubClasses, &cbLen))
    {
        goto RemoveDrivers32_exit;
    }

    // Enumerate all the subclasses
    for (
        pszClass = mystrtok(szSubClasses,NULL,&strtok_State);
        pszClass;
        pszClass = mystrtok(NULL,NULL,&strtok_State)
        )
    {
        // Open up each subclass
        if (RegOpenKey(hkDrivers, pszClass, &hkClass))
        {
            continue;
        }

        // Under each class is a set of driver name subkeys.
        // For each driver (e.g. foo1.drv, foo2.drv, etc.)
        for (idxR3DriverName = 0;
            !RegEnumKey(hkClass, idxR3DriverName, szR3DriverName, sizeof(szR3DriverName)/sizeof(TCHAR));
            idxR3DriverName++)
        {

            // Open the key to the driver name
            if (RegOpenKey(hkClass, szR3DriverName, &hkR3DriverName))
            {
                continue;
            }

            // Get driver name
            cbLen = sizeof(szDriver);
            if (!RegQueryValueEx(hkR3DriverName, TEXT("Driver"), NULL, NULL, (LPBYTE)szDriver, &cbLen))
            {
                // Create the alias
                wsprintf(szAlias,TEXT("%s.%s<%s>"),(LPTSTR)pszClass,(LPTSTR)szDriver,(LPTSTR)pszDrvInst);

                // Write into Drivers32
                RegSetValueExA(hkDrivers32,szAlias,0,REG_SZ,(PBYTE)szDriver,(wcslen(szDriver)*sizeof(TCHAR)) + sizeof(TCHAR));

                // Write alias back into driver's reg area
                RegSetValueExA(hkR3DriverName,TEXT("Alias"),0,REG_SZ,(PBYTE)szAlias,(wcslen(szAlias)*sizeof(TCHAR)) + sizeof(TCHAR));
            }

            // Write out Description
            // Get driver description
            cbLen = sizeof(szDescription);
            if (!RegQueryValueEx(hkR3DriverName, TEXT("Description"), NULL, NULL, (LPBYTE)szDescription, &cbLen))
            {
                RegSetValueExA(hkDriversDesc,szDriver,0,REG_SZ,(PBYTE)szDescription,(wcslen(szDescription)*sizeof(TCHAR)) + sizeof(TCHAR));
            }

            // Close the Driver Name key
            RegCloseKey(hkR3DriverName);
        }
        // Close the class key
        RegCloseKey(hkClass);
    }

    bRet = TRUE;

    RemoveDrivers32_exit:

    if (hkDrivers32)    RegCloseKey(hkDrivers32);
    if (hkDevReg)       RegCloseKey(hkDevReg);
    if (hkDrivers)      RegCloseKey(hkDrivers);
    if (hkDriversDesc)  RegCloseKey(hkDriversDesc);

    return bRet;
}
#endif

#if 0 // Unused at present
// The driver's private registry section is located in something like:
// HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\{4D36E96C-E325-11CE-BFC1-08002BE10318}\xxxx
// where xxxx is the device instance (e.g. 0000, 0001, etc.)
// These last four digits are used to index into the driver's MediaResources section.

// For example, suppose a device has a driver instance
// HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\{4D36E96C-E325-11CE-BFC1-08002BE10318}\0001
// and under that entry there is a Drivers\wave\foo.drv, meaning that the foo.drv driver supports a wave
// API.
// In this case, there would be an entry in
// HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\MediaResources\wave\foo.drv<0001>
//
// On removal, we need to delete that entry.
BOOL AddMediaResources(IN HDEVINFO         DeviceInfoSet,
                       IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
                      )


{
    BOOL bRet = FALSE;          // Return value

    TCHAR szDriverKey[MAX_PATH]; // Value of Driver's key in Enum branch
    TCHAR *pszDrvInst;           // Driver's instance, e.g. "0000", "0001", etc.

    HKEY hkDevReg  = NULL;      // Key to Driver portion of registry (e.g. classguid\0000)
    HKEY hkDrivers = NULL;      // Key to classguid\0000\Drivers
    HKEY hkMR      = NULL;      // Handle to MediaResources section

    TCHAR szSubClasses[256];     // List of subclasses to process
    TCHAR *strtok_State;         // strtok state
    TCHAR *pszClass;             // Information about e.g. classguid\0000\Drivers\wave
    HKEY hkClass;

    DWORD idxR3DriverName;      // Information about e.g. classguid\0000\Drivers\wave\foo.drv
    HKEY hkR3DriverName;
    TCHAR szR3DriverName[64];

    TCHAR szDriver[64];          // Driver name (e.g. foo.drv)
    DWORD cbLen;                // Size of szDriver

    TCHAR szDevNode[MAX_PATH+1];         // Path to driver's reg entry
    TCHAR szSoftwareKey[MAX_PATH+1];     // Value of SOFTWAREKEY

    // Open Media Resources section of registry
    if (RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_MEDIARESOURCES, &hkMR))
    {
        goto RemoveMediaResources_exit;
    }

    // Get the Drivers key value under the device's Enum branch,
    // e.g. something like "{4D36E96C-E325-11CE-BFC1-08002BE10318}\0000"
    SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                     DeviceInfoData,
                                     SPDRP_DRIVER ,
                                     NULL,
                                     (LPBYTE)szDriverKey,
                                     MAX_PATH,
                                     NULL);

    // Get everything after the last \ character
    pszDrvInst = strrchr(szDriverKey,TEXT('\\'));
    if (!pszDrvInst)
    {
        goto RemoveMediaResources_exit;
    }
    pszDrvInst++;
    // Now pszDrvInst points to a string with the Driver Instance, e.g. "0000"

    // Get full path to driver key
    wsprintf(szDevNode,
             TEXT("%s\\%s"),
             REGSTR_PATH_CLASS_NT,
             (LPTSTR)szDriverKey);

    // Open the Driver reg key
    hkDevReg = SetupDiOpenDevRegKey(DeviceInfoSet,
                                    DeviceInfoData,
                                    DICS_FLAG_GLOBAL,
                                    0,
                                    DIREG_DRV,
                                    KEY_ALL_ACCESS);
    if (hkDevReg == INVALID_HANDLE_VALUE)
    {
        goto RemoveMediaResources_exit;
    }

    // Enumerate through supporter classes in the Drivers subkey
    if (RegOpenKey(hkDevReg, TEXT("Drivers"), &hkDrivers))
    {
        goto RemoveMediaResources_exit;
    }

    // Read the SubClasses key to determine which subclasses to process
    cbLen=sizeof(szSubClasses);
    if (RegQueryValueEx(hkDrivers, TEXT("Subclasses"), NULL, NULL, (LPBYTE)szSubClasses, &cbLen))
    {
        goto RemoveMediaResources_exit;
    }

    // Enumerate all the subclasses
    for (
        pszClass = mystrtok(szSubClasses,NULL,&strtok_State);
        pszClass;
        pszClass = mystrtok(NULL,NULL,&strtok_State)
        )
    {
        if (RegOpenKey(hkDrivers, pszClass, &hkClass))
        {
            continue;
        }

        // Under each class is a set of driver name subkeys.
        // For each driver (e.g. foo1.drv, foo2.drv, etc.)
        for (idxR3DriverName = 0;
            !RegEnumKey(hkClass, idxR3DriverName, szR3DriverName, sizeof(szR3DriverName)/sizeof(TCHAR));
            idxR3DriverName++)
        {

            // Open the key to the driver name
            if (RegOpenKey(hkClass, szR3DriverName, &hkR3DriverName))
            {
                continue;
            }

            // Get the value of Driver in under the driver name key
            cbLen = sizeof(szDriver);
            if (!RegQueryValueEx(hkR3DriverName, TEXT("Driver"), NULL, NULL, (LPBYTE)szDriver, &cbLen))
            {
                HKEY hkMRClass;
                HKEY hkMRDriver;
                TCHAR szMRDriver[256];

                // Create the class key if it doesn't already exist
                if (!RegCreateKey(hkMR,pszClass,&hkMRClass))
                {
                    continue;
                }

                // Create the driver key if it doesn't already exist
                wsprintf(szMRDriver,
                         TEXT("%s<%s>"),
                         (LPTSTR)szDriver,
                         (LPTSTR)pszDrvInst);

                if (!RegCreateKey(hkMRClass,szMRDriver,&hkMRDriver))
                {
                    RegCloseKey(hkMRClass);
                    continue;
                }

                // Migrate the values from the driver into the MediaResources key
                // First write out driver name
                // BUGBUG Not implemented yet.

                RegCloseKey(hkMRClass);
                RegCloseKey(hkMRDriver);
            }
            // Close the Driver Name key
            RegCloseKey(hkR3DriverName);
        }

        // Close the class key in the devnode
        RegCloseKey(hkClass);
    }

    bRet = TRUE;

    RemoveMediaResources_exit:
    if (hkDevReg)   RegCloseKey(hkDevReg);
    if (hkDrivers)  RegCloseKey(hkDrivers);
    if (hkMR)       RegCloseKey(hkMR);

    return bRet;
}
#endif
