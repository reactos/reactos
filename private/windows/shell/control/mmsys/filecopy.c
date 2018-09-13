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

static TCHAR     szErrMsg[MAXSTR];

// Hidden parameters passed from wsInsertDisk to wDiskDlg

static TCHAR     CurrentDisk[MAX_PATH];
static LPTSTR    szEdit;

// Function prototypes

BOOL wsInfParseInit    (void);
int  fDialog           (int, HWND, DLGPROC);
UINT wsCopyError       (int, LPTSTR);
UINT wsInsertDisk      (LPTSTR, LPTSTR);
INT_PTR wsDiskDlg         (HWND, UINT, WPARAM, LPARAM);
INT_PTR wsExistDlg        (HWND, UINT, WPARAM, LPARAM);

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

 int LoadDescFromFile(PIDRIVER pIDriver, LPTSTR pstrKey, LPTSTR pstrDesc)
 {
     PINF        pinf;
     TCHAR        szFileName[MAX_INF_LINE_LEN];
     LPTSTR        pstrFile = pIDriver->szFile;
     TCHAR        ExpandedName[MAX_PATH];
     LPTSTR        FilePart;

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

 BOOL GetInstallPath(LPTSTR szDirOfSrc)
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
                             (LPBYTE)szDirOfSrc,
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
     TCHAR       szPathName[MAX_PATH];
     TCHAR*      pszFilePart;
     PINF        pinf;
     TCHAR       szNoInf[MAXSTR];
     TCHAR       iDrive;
     static BOOL bChkCDROM = FALSE;
     HFILE       hFile;

    /*
     *  put up an hour glass here
     */

     wsStartWait();

     hFile = (HFILE)HandleToUlong(CreateFile(szSetupInf, GENERIC_READ, FILE_SHARE_READ,NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));

     if (hFile == -1)
     {
         wsEndWait();
         LoadString(myInstance, IDS_NOINF, szNoInf, sizeof(szNoInf)/sizeof(TCHAR));
         MessageBox(hMesgBoxParent, szNoInf, szDrivers, MB_OK | MB_ICONEXCLAMATION);
         return FALSE;
     }

     CloseHandle((HANDLE)hFile);

     GetFullPathName(szSetupInf,sizeof(szPathName)/sizeof(TCHAR),szPathName,&pszFilePart);

     pinf = infOpen(szPathName);

     wsEndWait();

     GetWindowsDirectory(szSetupPath, sizeof(szSetupPath)/sizeof(TCHAR));

     if (bChkCDROM == FALSE) {

         /*
          *  Use the setup path from the registry if there is one
          */

          if (!GetInstallPath(szDirOfSrc))
          {
             /*
              *  use the CD ROM drive as the default drive (if there is one)
              */

              for ( iDrive=TEXT('A'); iDrive <= TEXT('Z'); iDrive++ ) {
                  szDirOfSrc[0] = iDrive;

                  if ( GetDriveType(szDirOfSrc) == DRIVE_CDROM)
                  {
                          break;
                  }
                 /*
                  *  If we didn't find a CD ROM default to the A drive
                  */

                  if (iDrive == TEXT('Z')) {
                      szDirOfSrc[0] = TEXT('A');
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
 UINT wsCopyError(int n, LPTSTR szFile)
 {
     TCHAR strBuf[MAXSTR];
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
            BOOL bFound = FALSE;
            PIDRIVER pIDriver;

           /*
            *  The driver is in use :
            *
            *  Search the list of curently installed drivers to see
            *  if this file is one of them.  If so tell the user to
            *  de-install and re-start.
            *
            *  If the driver is not currently installed then tell
            *  the user to re-start in the hope that it will then
            *  not be loaded (and so in use)
            *
            *  Note that there is another case not catered for that
            *  this is just a file in the driver's copy list which
            *  failed to copy because it was 'in use'.
            *
            */

            pIDriver = FindIDriverByName (FileName(szFile));

            if (pIDriver != NULL)    // Found an already-installed driver?
            {
                TCHAR sztemp[MAXSTR];
                LoadString(myInstance,
                           IDS_FILEINUSEREM,
                           sztemp,
                           sizeof(sztemp)/sizeof(TCHAR));

                wsprintf(strBuf, sztemp, (LPTSTR)pIDriver->szDesc);
                bFound = TRUE;
            } else {
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
UINT wsInsertDisk(LPTSTR Disk, LPTSTR szSrcPath)
{
    UINT temp;
    int i;

   /*
    *  Create the real disk letter
    */
    for (i = 0; Disk[i] != TEXT('\0') && Disk[i] != TEXT(':'); i++) {
        CurrentDisk[i] = Disk[i];
    }
    CurrentDisk[i] = TEXT('\0'); // Null terminate

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

INT_PTR wsDiskDlg(HWND hDlg, UINT uiMessage, WPARAM wParam, LPARAM lParam)
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

            TCHAR DisksSection[MAXSTR];

           /*
            *  now look in the [disks] section for the disk name
            *  the disk name is the second field.
            */

            TCHAR buf[MAXSTR];
            TCHAR buf2[MAXSTR];
            TCHAR bufout[MAXSTR];

            *buf = TEXT('\0');

           /*
            *  See what the name of the section should be
            */

            LoadString(myInstance,
                       IDS_DISKS,
                       DisksSection,
                       sizeof(DisksSection)/sizeof(TCHAR));

            infGetProfileString(NULL, DisksSection, CurrentDisk, (LPTSTR)buf);

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

               infGetProfileString(NULL, TEXT("disks"), CurrentDisk, (LPTSTR)buf);
               if (!*buf)
                 infGetProfileString(NULL, TEXT("oemdisks"), CurrentDisk, (LPTSTR)buf);

               if (!*buf) {
                   return FALSE;
               }

               infParseField(buf, 2, buf2);
            }

            wsprintf(bufout, szKnown, (LPTSTR)buf2, (LPTSTR)szDrv);
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

 UINT wsCopySingleStatus(int msg, DWORD_PTR n, LPTSTR szFile)
 {
    OFSTRUCT ofs;
    TCHAR szFullPath[MAX_PATH];
    TCHAR szDriverExists[MAXSTR];

    switch (msg)
     {
         case COPY_INSERTDISK:
             return wsInsertDisk((LPTSTR)n, szFile);

         case COPY_ERROR:
             return wsCopyError((int)n, szFile);


         case COPY_QUERYCOPY:

            /*
             *  See if the file already exists in the windows system
             *  directory
             */

             GetSystemDirectory(szFullPath, MAX_PATH);

             if (IsFileKernelDriver(szFile)) {
                 lstrcat(szFullPath, TEXT("\\drivers"));
             }

             lstrcat(szFullPath, TEXT("\\"));

             lstrcat(szFullPath, RemoveDiskId(szFile));

             if ((HFILE)HandleToUlong(CreateFile(szFullPath, GENERIC_READ, FILE_SHARE_READ,NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) != HFILE_ERROR)
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
                                sizeof(szDriverExists)/sizeof(TCHAR));

                     wsprintf(szErrMsg, szDriverExists, FileName(szFile));

                    /*
                     *  Ask the user whether to copy or not ?
                     */

                     DriverCopy = (int)DialogBox(myInstance,
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

 INT_PTR wsExistDlg(HWND hDlg, UINT uiMessage, WPARAM wParam, LPARAM lParam)
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

     while (*szEdit == TEXT(' ')) {
         szEdit = CharNext(szEdit);
     }

     lstrcpy(szPath, szEdit);

     for (szLastSpaceList = NULL;
          *szPath != TEXT('\0');
          szPath = CharNext(szPath)) {

        if (*szPath == TEXT(' ')) {
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
