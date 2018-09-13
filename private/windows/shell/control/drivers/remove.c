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

#include "drivers.h"
#include "sulib.h"

BOOL     SetValidAlias      (PSTR, PSTR);

#ifdef DOBOOT
BOOL     FindBootDriver     (char *);
PSTR     strstri            (PSTR, PSTR);
#endif // DOBOOT

/*
 *  RemoveService(szFile)
 *
 *  Remove the service corresponding to the file szFile
 *
 *  returns TRUE if successful, FALSE otherwise
 */

 BOOL RemoveService(LPSTR szFile)
 {
     SC_HANDLE SCManagerHandle;
     SC_HANDLE ServiceHandle;
     char ServiceName[MAX_PATH];
     BOOL Status = FALSE;

    /*
     *  Extract the service name from the file name
     */

     {
         char drive[MAX_PATH], directory[MAX_PATH], ext[MAX_PATH];
         _splitpath(szFile, drive, directory, ServiceName, ext);
     }

    /*
     *  First try and obtain a handle to the service controller
     */

     SCManagerHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
     if (SCManagerHandle == NULL) {

         char szMesg[MAXSTR];
         char szMesg2[MAXSTR];

         LoadString(myInstance, IDS_INSUFFICIENT_PRIVILEGE, szMesg, sizeof(szMesg));
         wsprintf(szMesg2, szMesg, szFile);
         MessageBox(hMesgBoxParent, szMesg2, szRemove, MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
         return FALSE;
     }


     ServiceHandle = OpenService(SCManagerHandle,
                                 ServiceName,
                                 SERVICE_ALL_ACCESS);
     if (ServiceHandle != NULL) {
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
              Sleep(100)) {
         }

         Status = DeleteService(ServiceHandle);

         UnlockServiceDatabase(ServicesDatabaseLock);

         CloseServiceHandle(ServiceHandle);
     } else {

        /*
         *  It's possible there was no services entry so the driver
         *  wasn't really installed after all.
         */

         LONG Error = GetLastError();

         if (Error == ERROR_FILE_NOT_FOUND ||
             Error == ERROR_PATH_NOT_FOUND ||
             Error == ERROR_SERVICE_DOES_NOT_EXIST) {
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
LONG PostRemove(HWND hWnd, PIDRIVER pIDriver, BOOL bLookAtRelated,
                int iIndexMain)
{

    char *keystr;
    char allkeystr[MAXSTR];
    char szfile[MAX_PATH];
    HANDLE hDriver;
    LONG Status = DRVCNF_CANCEL;
    PSTR pstr;


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
    *pstr = '\0';

    if (bLookAtRelated && (!bRelated || pIDriver->bRelated))
        strcpy(szRestartDrv,  pIDriver->szDesc);

   /*
    *  If it's a kernel driver remove it from the config registry
    *  and services controller
    */

    if (pIDriver->KernelDriver) {

        Status = RemoveService(pIDriver->szFile) ? DRVCNF_RESTART : DRVCNF_CANCEL;

        if (Status == DRVCNF_CANCEL) {
            return DRVCNF_CANCEL;
        }

    } else {

        hDriver = OpenDriver(pIDriver->wszAlias, pIDriver->wszSection, 0L);

        if (hDriver)
        {

           /*
            *  Removal can fail so don't mark as deleted in this case
            */

            Status = SendDriverMessage(hDriver, DRV_REMOVE, 0L, 0L);
            CloseDriver(hDriver, 0L, 0L);

            if (Status == DRVCNF_CANCEL) {
                return DRVCNF_CANCEL;
            }
        }
    }

    SendMessage(hWnd, LB_DELETESTRING, iIndexMain, 0L);
    if (bLookAtRelated)
    {
        char allkeystr[MAXSTR];


        if (GetPrivateProfileString(szRelatedDesc, pIDriver->szAlias,
              allkeystr, allkeystr, sizeof(allkeystr), szControlIni))
        {
            int i, iIndex;
            BOOL bFound;
            PIDRIVER pIDriver;
            char szTemp[MAXSTR];

            for (i = 1; infParseField(allkeystr, i, szTemp);i++)
            {
                bFound = FALSE;

                iIndex = (int)SendMessage(hWnd, LB_GETCOUNT, 0, 0L);
                while ( iIndex-- > 0  && !bFound)
                  if ( (int)(pIDriver = (PIDRIVER)SendMessage(hWnd, LB_GETITEMDATA, iIndex, 0L)) != LB_ERR) {
                      if (!FileNameCmp(pIDriver->szFile, szTemp))
                      {
                          if (PostRemove(hWnd, pIDriver, FALSE, iIndex)
                                 == DRVCNF_RESTART) {
                                 Status = DRVCNF_RESTART;
                             }
                          bFound = TRUE;
                      }

                  }
            }
        }
    }

    // Remove the driver entry from SYSTEM.INI so you don't
    // conflict with other drivers.

    GetPrivateProfileString(pIDriver->szSection, NULL, NULL,
                            allkeystr, sizeof(allkeystr), szSysIni);
    keystr = allkeystr;
    while (strlen(keystr) > 0)
    {
         GetPrivateProfileString(pIDriver->szSection, keystr, NULL, szfile, sizeof(szfile), szSysIni);
         if (!FileNameCmp(pIDriver->szFile, szfile))
               RemoveDriverEntry(keystr, pIDriver->szFile, pIDriver->szSection, bLookAtRelated);
         keystr = &keystr[strlen(keystr) + 1];
    }
    return Status;
}



void RemoveDriverEntry (PSTR szKey, PSTR szFile, PSTR szSection, BOOL bLookAtRelated)
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
 BOOL SetValidAlias(PSTR pstrType, PSTR pstrSection)
 {
     char *keystr;
     static char allkeystr[MAXSTR];
     static char szExKey[MAXSTR], szExFile[MAXSTR], szExDesc[MAXSTR];
     BOOL bfound = FALSE, bExchange = FALSE;
     int val, maxval = 0, typelen, len;

    /*
     *  Getting length of alias
     */

     len = typelen = strlen(pstrType);

     // If the last char on the type is a number don't consider it

     if (pstrType[typelen -1] > '0' && pstrType[typelen - 1] <= '9')
        typelen--;

     // Get all the aliases in the drivers section

     GetPrivateProfileString(pstrSection, NULL, NULL, allkeystr,
                    sizeof(allkeystr), szSysIni);
     keystr = allkeystr;
     while (*keystr != TEXT('\0'))
     {
        // Compare the root of the aliases
        if (!_strnicmp(keystr, pstrType, typelen) && ((keystr[typelen] <= '9' && keystr[typelen] > '0') || keystr[typelen] == TEXT('\0')))
        {

           //We found a common alias
      bfound = TRUE;
           val = atoi(&keystr[typelen]);
           if (val > maxval)
      {
         maxval = val;
         strcpy(szExKey, keystr);
           }
        }
        //Pointer to next alias
        keystr = &keystr[strlen(keystr) + 1];
     }
     //If we found one
     if (bfound)
     {
    if (len == typelen)
        bExchange = TRUE;
         else
             if (atoi(&pstrType[typelen]) < maxval)
            bExchange = TRUE;

        // We need to exchange it with the one we found
        if (bExchange)
        {
            //Exchanging the one in the drivers section in system.ini
           GetPrivateProfileString(pstrSection, szExKey, NULL, szExFile,
                                    sizeof(szExFile), szSysIni);
            WritePrivateProfileString(pstrSection, szExKey, NULL, szSysIni);
            WritePrivateProfileString(pstrSection, pstrType, szExFile, szSysIni);

 #ifdef TRASHDRIVERDESC
       //Exchanging the one in the drivers description section of control.ini
        GetPrivateProfileString(szDriversDesc, szExKey, NULL, szExDesc, sizeof(szExFile), szControlIni);
        WritePrivateProfileString(szDriversDesc, szExKey, NULL, szControlIni);
        WritePrivateProfileString(szDriversDesc, pstrType, szExDesc, szControlIni);
 #endif

            //If any related drivers were present under old alias switch them
            GetPrivateProfileString(szRelatedDesc, szExKey, NULL, szExDesc, sizeof(szExFile), szControlIni);

            if (strlen(szExDesc))
            {
               WritePrivateProfileString(szRelatedDesc, szExKey, NULL, szControlIni);
               WritePrivateProfileString(szRelatedDesc, pstrType, szExDesc, szControlIni);
            }

            //If user installed driver under old alias switch them
           GetPrivateProfileString(szUserDrivers, szExKey, NULL, szExDesc, sizeof(szExFile), szControlIni);

           if (strlen(szExDesc))
           {
              WritePrivateProfileString(szUserDrivers, szExKey, NULL, szControlIni);
              WritePrivateProfileString(szUserDrivers, pstrType, szExDesc, szControlIni);
            }

 #ifdef DOBOOT
            if (FindBootDriver(szExKey))
           {
              static char szTemp[MAXSTR];

               GetPrivateProfileString(szBoot, szDrivers, szTemp, szTemp,
                                 sizeof(szTemp), szSysIni);
               strcat(szTemp, " ");
              strcat(szTemp, pstrType);
               WritePrivateProfileString(szBoot, szDrivers, szTemp, szSysIni);
            }
 #endif // DOBOOT

        }
     }
     return(bExchange);
 }

#if 0 // Dead code !

/*
 *  IsOnlyInstance()
 *
 *  Check to see if this is the only occurance of an open driver, so we can
 *  tell if a DRV_INSTALL message has been sent to the driver yet OR
 *  if it is the last instance of the driver so we can send a
 *  DRV_REMOVE message.
 */

 BOOL IsOnlyInstance( HWND hwndLB, PIDRIVER pIDriver )
 {
     int iEntries;
     PIDRIVER    pIDTest;

     if (pIDriver == NULL)
         return FALSE;

    /*
     *  Check Installed ListBox
     */

     if ((iEntries = (int)SendMessage(hwndLB, LB_GETCOUNT, 0, 0L)) == LB_ERR)
         iEntries = 0;
     else
         while (iEntries-- > 0)
         {
             pIDTest = (PIDRIVER)SendMessage(hwndLB,
                                             LB_GETITEMDATA,
                                             iEntries,
                                             0L);
             if (pIDTest == NULL)
                 continue;

             if (pIDriver == pIDTest)
                 continue;

             if (FileNameCmp(pIDTest->szFile,pIDriver->szFile) == 0)
             {
                 return FALSE;
             }
         }

     return TRUE;
 }
#endif

int FileNameCmp(char far *pch1, char far *pch2)
{
    LPSTR pchEOS;

    while (*pch1 == ' ') pch1++; // eat spaces
    while (*pch2 == ' ') pch2++; // eat spaces

    for (pchEOS = pch1; *pchEOS && *pchEOS != ' '; pchEOS++);

    return _strnicmp(pch1, pch2, pchEOS - pch1);
}

#ifdef DOBOOT

PSTR strstri(PSTR pszStr, PSTR pszKey)
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

BOOL FindBootDriver(char *szKey)
{
    char *ptr;
    int wKeyLen = (int)strlen(szKey);
    char *endkey;
    static char szDriverline[MAXSTR];

    GetPrivateProfileString("boot", "drivers", szDriverline, szDriverline,
                    MAX_PATH, szSysIni);
    ptr = strstri(szDriverline, szKey);
    if (ptr)
    {

    if ((((ptr != szDriverline) && (*(ptr - 1) == ' ' )) ||
          (ptr == szDriverline)) &&
        (*(ptr + wKeyLen) == ' ' || *(ptr + wKeyLen) == NULL))
    {
        endkey = ptr + wKeyLen;
        while (*endkey)
          *ptr++ = *endkey++;
        *ptr = NULL;
        WritePrivateProfileString("boot", "drivers", szDriverline,
                            szSysIni);
        return(TRUE);
    }
    }
    return(FALSE);
}

#endif // DOBOOT
