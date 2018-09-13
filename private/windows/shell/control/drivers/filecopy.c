/**************************************************************************
 *
 *  FILECOPY.C
 *
 *  Copyright (C) Microsoft, 1990, All Rights Reserved.
 *
 *  Control Panel Applet for installing installable driver.
 *
 *  This file contains hooks to SULIB, COMPRESS libraries, and the dialogs
 *  from the display applet to prompt for insert disk, error action...
 *
 *  Note SULIB.LIB, COMPRESS.LIB, SULIB.H come from the display applet
 *  and are updated here if/when updated there.
 *
 *  History:
 *
 *      Sat Oct 27 1990 -by- MichaelE
 *          Munged from display applet's DLG.C.
 *
 **************************************************************************/

#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include "drivers.h"
#include "sulib.h"
#include <cphelp.h>

// Hidden parameter between wsSingleCopyStatus and wExistDlg

static char     szErrMsg[MAXSTR];

// Hidden parameters passed from wsInsertDisk to wDiskDlg

static char     CurrentDisk[MAX_PATH];
static LPSTR    szEdit;

// Function prototypes

BOOL wsInfParseInit    (void);
int  fDialog           (int, HWND, DLGPROC);
UINT wsCopyError       (int, LPSTR);
UINT wsInsertDisk      (LPSTR, LPSTR);
BOOL wsDiskDlg         (HWND, UINT, WPARAM, LPARAM);
BOOL wsExistDlg        (HWND, UINT, WPARAM, LPARAM);

/*
 *  Load the description from the inf file or the driver file.
 *
 *  The type of file is also returned in the driver structure.
 *
 *  Parameters :
 *       pIDriver - Pointer to driver data - in particular the driver file name
 *       pstrKey  - The ini file key under which the driver should be found
 *       pstrDesc - Where to return the description
 */

 int LoadDesc(PIDRIVER pIDriver, PSTR pstrKey, PSTR pstrDesc)
 {
     PINF        pinf;
     CHAR        szFileName[MAX_INF_LINE_LEN];
     PSTR        pstrFile = pIDriver->szFile;
     CHAR        ExpandedName[MAX_PATH];
     PSTR        FilePart;

    /*
     *  See if the file can be found
     */


     if (SearchPath(NULL, pstrFile, NULL, MAX_PATH, ExpandedName, &FilePart)
         == 0) {
         return(DESC_NOFILE);
     }

    /*
     * -jyg- Let's look in the mmdriver.inf first!
     */

     for (pinf = FindInstallableDriversSection(NULL);
          pinf;
          pinf = infNextLine(pinf))
     {
         infParseField(pinf, 1, szFileName); // compare filename

        /*
         *  FileName strips of drive and path
         */

         if (lstrcmpi(FileName(pstrFile), FileName(szFileName)) == 0)
         {
             infParseField(pinf, 3, pstrDesc); // get Description Field

             return DESC_INF;
         }
     }

    /*
     *  If that failed try to get the description from the file
     */

     if (!GetFileTitle(ExpandedName, pstrDesc, MAXSTR)) {
         return DESC_EXE;
     } else {
         return DESC_NOFILE;
     }
 }

/*
 *  Find the install path from the registry if there is one there
 */

 BOOL GetInstallPath(LPSTR szDirOfSrc)
 {
     HKEY RegHandle;
     DWORD Type;
     DWORD Length = MAX_PATH - 1;
     BOOL Found = FALSE;

     if (MMSYSERR_NOERROR ==
         RegOpenKey(HKEY_LOCAL_MACHINE,
                    TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion"),
                    &RegHandle)) {

         if (MMSYSERR_NOERROR ==
             RegQueryValueEx(RegHandle,
                             TEXT("SourcePath"),
                             NULL,
                             &Type,
                             szDirOfSrc,
                             &Length) &&
             Type == REG_SZ) {

             Found = TRUE;
         }

         RegCloseKey(RegHandle);

     }

     return Found;
 }

/*
 *  Initialize the SULIB library stuff which loads the mmdriver.inf file
 *  into RAM and parses it all over the place.
 */

 BOOL wsInfParseInit(void)
 {
     OFSTRUCT    os;
     PINF        pinf;
     char        szNoInf[MAXSTR];
     int         iDrive;
     static BOOL bChkCDROM = FALSE;

    /*
     *  put up an hour glass here
     */

     wsStartWait();

     if (OpenFile(szSetupInf, &os, OF_EXIST) == -1) {

         wsEndWait();
         LoadString(myInstance, IDS_NOINF, szNoInf, sizeof(szNoInf));
         MessageBox(hMesgBoxParent, szNoInf, szDrivers, MB_OK | MB_ICONEXCLAMATION);
         return FALSE;
     }

     pinf = infOpen(os.szPathName);

     wsEndWait();

     GetWindowsDirectory(szSetupPath, sizeof(szSetupPath));

     if (bChkCDROM == FALSE) {

         /*
          *  Use the setup path from the registry if there is one
          */

          if (!GetInstallPath(szDirOfSrc))
          {
             /*
              *  use the CD ROM drive as the default drive (if there is one)
              */

              for ( iDrive='A'; iDrive <= 'Z'; iDrive++ ) {
                  szDirOfSrc[0] = iDrive;

                  if ( GetDriveType(szDirOfSrc) == DRIVE_CDROM)
                  {
                          break;
                  }
                 /*
                  *  If we didn't find a CD ROM default to the A drive
                  */

                  if (iDrive == 'Z') {
                      szDirOfSrc[0] = 'A';
                  }
              }
          }

          bChkCDROM = TRUE;
     }

     lstrcpy(szDiskPath, szDirOfSrc);

     return TRUE;
 }


/*----------------------------------------------------------------------------*\
|   wsStartWait()                                                              |
|                                                                              |
|   Turn the WinSetup cursor to a hour glass                                   |
|                                                                              |
\*----------------------------------------------------------------------------*/
void wsStartWait()
{
    SetCursor(LoadCursor(NULL,IDC_WAIT));
}

/*----------------------------------------------------------------------------*\
|   wsEndWait()                                                                |
|                                                                              |
|   Turn the WinSetup cursor back to what it was                               |
|                                                                              |
\*----------------------------------------------------------------------------*/
void wsEndWait()
{
    SetCursor(LoadCursor(NULL,IDC_ARROW));
}


/*----------------------------------------------------------------------------*\
|   fDialog(id,hwnd,fpfn)                                                      |
|                                                                              |
|   Description:                                                               |
|       This function displays a dialog box and returns the exit code.         |
|                                                                              |
|   Arguments:                                                                 |
|       id              resource id of dialog to display                       |
|       hwnd            parent window of dialog                                |
|       fpfn            dialog message function                                |
|                                                                              |
|   Returns:                                                                   |
|       exit code of dialog (what was passed to EndDialog)                     |
|                                                                              |
\*----------------------------------------------------------------------------*/
int fDialog(int id, HWND hwnd, DLGPROC fpfn)
{
    return ( (int)DialogBox(myInstance, MAKEINTRESOURCE(id), hwnd, fpfn) );
}


/****************************************************************************
 *                                                                             |
 *wsCopyError()                                                                |
 *                                                                             |
 *  Handles errors, as the result of copying files.                            |
 *                                                                             |
 *  This may include net contention errors, in which case the user must        |
 *  retry the operation.                                                       |
 *                                                                             |
 *  Parameters :
 *
 *     n      - Copy error number
 *
 *     szFile - the fully qualified name of the file we are copying
 *
 *  Returns
 *
 *     Always returns FC_ABORT
 *
 ****************************************************************************/
 UINT wsCopyError(int n, LPSTR szFile)
 {
     char strBuf[MAXSTR];
     int i = 0;

    /*
     *  We do not want to report any errors that occur while installing
     *  related drivers to the user
     */

     if (bCopyingRelated)
          return(FC_ABORT);

    /*
     *  check for out of disk space
     */

     if (n == ERROR_DISK_FULL) {

        LoadString(myInstance, IDS_OUTOFDISK, strBuf, MAXSTR);

     } else {

       /*
        *  Check to see if a copy has been done on a file that is currently
        *  loaded by the system.
        *
        *  n is the return code from VerInstallFile after translating
        *  by ConvertFlagToValue
        */

        if (n == FC_ERROR_LOADED_DRIVER)
        {
            int iIndex;
            BOOL bFound = FALSE;
            PIDRIVER pIDriver;

            iIndex = (int)SendMessage(hlistbox, LB_GETCOUNT, 0, 0L);


           /*
            *  The driver is in use :
            *
            *  Search the list of curently installed drivers to see
            *  if this file is one of them.  If so tell the user to
            *  de-install and re-start.
            */

            while ( iIndex-- > 0  && !bFound) {

                if ( (int)(pIDriver = (PIDRIVER)SendMessage(hlistbox,
                                                            LB_GETITEMDATA,
                                                            iIndex,
                                                            0L)) != LB_ERR)
                {
                   if (!lstrcmpi(pIDriver->szFile, FileName(szFile)))
                   {
                        char sztemp[MAXSTR];

                       /*
                        *  Found the driver description.
                        *
                        *  Tell the user to un-install it and restart
                        *  windows so that it's not loaded.
                        */

                        LoadString(myInstance,
                                   IDS_FILEINUSEREM,
                                   sztemp,
                                   sizeof(sztemp));

                        wsprintf(strBuf, sztemp, (LPSTR)pIDriver->szDesc);
                        bFound = TRUE;
                   }
                }
            } // while ( iIndex-- > 0 && !bFound)


           /*
            *  If the driver is not currently installed then tell
            *  the user to re-start in the hope that it will then
            *  not be loaded (and so in use)
            *
            *  Note that there is another case not catered for that
            *  this is just a file in the driver's copy list which
            *  failed to copy because it was 'in use'.
            */

            if (!bFound)
            {
                iRestartMessage = IDS_FILEINUSEADD;
                DialogBox(myInstance,
                          MAKEINTRESOURCE(DLG_RESTART),
                          hMesgBoxParent,
                          RestartDlg);

                return(FC_ABORT);
            }

        } else {

           /*
            *  Tell the user there is a problem which we don't
            *  understand here.
            */

            LoadString(myInstance,
                       IDS_UNABLE_TOINSTALL,
                       strBuf,
                       MAXSTR);
        }
     }

    /*
     *  Put up the message box we have selected.
     */

     MessageBox(hMesgBoxParent,
                strBuf,
                szFileError,
                MB_OK | MB_ICONEXCLAMATION  | MB_TASKMODAL);

     return (FC_ABORT);

 }


/*----------------------------------------------------------------------------*\
|                                                                              |
| wsInsertDisk()                                                               |
|                                                                              |
|   Handles errors, as the result of copying files.                            |
|                                                                              |
\*----------------------------------------------------------------------------*/
UINT wsInsertDisk(LPSTR Disk, LPSTR szSrcPath)
{
    UINT temp;
    int i;

   /*
    *  Create the real disk letter
    */
    for (i = 0; Disk[i] != '\0' && Disk[i] != ':'; i++) {
        CurrentDisk[i] = Disk[i];
    }
    CurrentDisk[i] = '\0'; // Null terminate

    szEdit = szSrcPath;

    bFindOEM = TRUE;
    temp =  (UINT)fDialog(DLG_INSERTDISK, GetActiveWindow(), wsDiskDlg);
    bFindOEM = FALSE;
    return(temp);
}


/*----------------------------------------------------------------------------*
|   wsDiskDlg( hDlg, uiMessage, wParam, lParam )                               |
|                                                                              |
|   Arguments:                                                                 |
|       hDlg            window handle of about dialog window                   |
|       uiMessage       message number                                         |
|       wParam          message-dependent                                      |
|       lParam          message-dependent                                      |
|                                                                              |
|   Returns:                                                                   |
|       TRUE if message has been processed, else FALSE                         |
|                                                                              |
\*----------------------------------------------------------------------------*/

BOOL wsDiskDlg(HWND hDlg, UINT uiMessage, UINT wParam, LPARAM lParam)
{

    switch (uiMessage)
    {
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDH_DLG_INSERT_DISK:
                   goto DoHelp;

                case IDS_BROWSE:

                  /*
                   *  Call the browse dialog to open drivers
                   */

                   BrowseDlg(hDlg,
                             3);    // index 3 points to no filter
                                    // - see szFilter
                   break;

                case IDOK:

                   /*
                    *  szEdit points to the path that will be retried
                    *  if the copy fails
                    */

                    GetDlgItemText(hDlg, ID_EDIT, szEdit, MAX_PATH);
                    RemoveSpaces(szDiskPath, szEdit);
                    lstrcpy(szEdit, szDiskPath);
                    EndDialog(hDlg, FC_RETRY);
                    UpdateWindow(hMesgBoxParent);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, FC_ABORT);
                    break;
            }
            return TRUE;

        case WM_INITDIALOG:
            {

            char DisksSection[MAXSTR];

           /*
            *  now look in the [disks] section for the disk name
            *  the disk name is the second field.
            */

            char buf[MAXSTR];
            char buf2[MAXSTR];
            char bufout[MAXSTR];

            *buf = '\0';

           /*
            *  See what the name of the section should be
            */

            LoadString(myInstance,
                       IDS_DISKS,
                       DisksSection,
                       sizeof(DisksSection));

            infGetProfileString(NULL, DisksSection, CurrentDisk, (LPSTR)buf);

            if (*buf) {

               /*
                * Position of description in Windows NT
                */

               infParseField(buf, 1, buf2);
            } else {

               /*
                *  Didn't find the section we were looking for so try
                *  the old names
                */

               infGetProfileString(NULL, "disks", CurrentDisk, (LPSTR)buf);
               if (!*buf)
                 infGetProfileString(NULL, "oemdisks", CurrentDisk, (LPSTR)buf);

               if (!*buf) {
                   return FALSE;
               }

               infParseField(buf, 2, buf2);
            }

            wsprintf(bufout, szKnown, (LPSTR)buf2, (LPSTR)szDrv);
            SetDlgItemText(hDlg,ID_TEXT,bufout);
            SetDlgItemText(hDlg,ID_EDIT,szEdit);

            return TRUE;
            }
        default:
            if (uiMessage == wHelpMessage) {
DoHelp:
               WinHelp(hDlg, szDriversHlp, HELP_CONTEXT, IDH_DLG_INSERT_DISK);
               return TRUE;
            }
            else
                return FALSE;
         break;
    }
}

/*--------------------------------------------------------------------------
 *
 * Function : wsCopySingleStatus
 *     File copying callback routine
 *
 * Parameters :
 *     msg - Which callback function
 *     n   - various
 *     szFile - which file
 *
 * this call back only copies it's file if it does not exist in the
 * path.
 *
 *--------------------------------------------------------------------------*/

 UINT wsCopySingleStatus(int msg, DWORD n, LPSTR szFile)
 {
    OFSTRUCT ofs;
    char szFullPath[MAX_PATH];
    char szDriverExists[MAXSTR];

    switch (msg)
     {
         case COPY_INSERTDISK:
             return wsInsertDisk((LPSTR)n, szFile);

         case COPY_ERROR:
             return wsCopyError((int)n, szFile);


         case COPY_QUERYCOPY:

            /*
             *  See if the file already exists in the windows system
             *  directory
             */

             GetSystemDirectory(szFullPath, MAX_PATH);

             if (IsFileKernelDriver(szFile)) {
                 lstrcat(szFullPath, "\\drivers");
             }

             lstrcat(szFullPath, "\\");

             lstrcat(szFullPath, RemoveDiskId(szFile));

             if (OpenFile(szFullPath, &ofs, OF_EXIST|OF_SHARE_DENY_NONE) >= 0)
             {
                /*
                 *  DriverCopy remembers whether to copy from
                 *  current or new after we have queried the user
                 *  once
                 */

                 static int DriverCopy;

                 if (bQueryExist)
                 {
                     bQueryExist = FALSE;

                     LoadString(myInstance,
                                IDS_DRIVER_EXISTS,
                                szDriverExists,
                                sizeof(szDriverExists));

                     wsprintf(szErrMsg, szDriverExists, FileName(szFile));

                    /*
                     *  Ask the user whether to copy or not ?
                     */

                     DriverCopy = DialogBox(myInstance,
                                            MAKEINTRESOURCE(DLG_EXISTS),
                                            hMesgBoxParent,
                                            wsExistDlg);
                 }

                 return DriverCopy;
             } else {

                 return CopyNew;
             }

         case COPY_START:
         case COPY_END:
             SetErrorMode(msg == COPY_START);    // don't crit error on us
             break;
     }
     return FC_IGNORE;
 }

/*
 *  Function : wsExistDlg - 'File exists' dialog
 */

 BOOL wsExistDlg(HWND hDlg, UINT uiMessage, UINT wParam, LPARAM lParam)
 {
     switch (uiMessage)
     {
         case WM_COMMAND:
             switch (LOWORD(wParam))
             {
                 case ID_CURRENT:

                     EndDialog(hDlg, CopyCurrent);
                     break;

                 case ID_NEW:

                    /*
                     *  User selected to copy the new files over the
                     *  existing ones
                     */

                     EndDialog(hDlg, CopyNew);
                     break;

                 case IDCANCEL:
                     EndDialog(hDlg, CopyNeither);  // Cancel
                     break;
             }
             return TRUE;

         case WM_INITDIALOG:
             SetDlgItemText(hDlg, ID_STATUS2, szErrMsg);
             return TRUE;

         default:
          break;
     }
     return FALSE;
 }

/*
 *  Function : RemoveSpaces
 *     Copies a string removing leading and trailing spaces but allowing
 *     for long file names with internal spaces.
 *
 *  Parameters :
 *     szPath - The output result
 *     szEdit - The input path
 */

 VOID RemoveSpaces(LPTSTR szPath, LPTSTR szEdit)
 {
     LPTSTR szLastSpaceList;

     while (*szEdit == ' ') {
         szEdit = CharNext(szEdit);
     }

     lstrcpy(szPath, szEdit);

     for (szLastSpaceList = NULL;
          *szPath != TEXT('\0');
          szPath = CharNext(szPath)) {

        if (*szPath == ' ') {
            if (szLastSpaceList == NULL) {
                szLastSpaceList = szPath;
            }
        } else {
            szLastSpaceList = NULL;
        }

     }

     if (szLastSpaceList != NULL) {
         *szLastSpaceList = TEXT('\0');
     }
 }
