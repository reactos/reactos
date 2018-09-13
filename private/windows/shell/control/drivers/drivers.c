/*  DRIVERS.C
**
**  Copyright (C) Microsoft, 1990, All Rights Reserved.
**
**  Multimedia Control Panel Applet for installing/configuring installable
**  device drivers.  See the ispec doc DRIVERS.DOC for more information.
**
**  History:
**
**      Tue Jul 31 1990 -by- MichaelE
**          Created.
**
**      Thu Oct 25 1990 -by- MichaelE
**          Added restart, horz. scroll, added SKIPDESC reading desc. strings.
**
**      Sat Oct 27 1990 -by- MichaelE
**          Added FileCopy.  Uses SULIB.LIB and LZCOPY.LIB. Finished stuff
**          for case of installing a driver with more than one type.
**
**      May 1991 -by- JohnYG
**          Added and replaced too many things to list.  Better management
**          of removed drivers, correct usage of DRV_INSTALL/DRV_REMOVE,
**          installing VxD's, replaced "Unknown" dialog with an OEMSETUP.INF
**          method, proper "Cancel" method, fixed many potential UAE's.
*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntseapi.h>
#include <windows.h>
#include <mmsystem.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <cpl.h>
#include <cphelp.h>

#include "drivers.h"
#include "sulib.h"

typedef struct
{
        int idIcon;
        int idName;
        int idInfo;
        BOOL bEnabled;
        DWORD dwContext;
        PSTR pszHelp;
} APPLET_INFO;

#define NUM_APPLETS     1
#define OBJECT_SIZE     1024

APPLET_INFO near applets[NUM_APPLETS];
BOOL     bBadOemSetup;
BOOL     bRestart = FALSE;
int      iRestartMessage = 0;
BOOL     bInstallBootLine = FALSE;
BOOL     bCopyVxD;
BOOL     bFindOEM = FALSE;
BOOL     bRelated = FALSE;
BOOL     bDriversAppInUse;
BOOL     bCopyingRelated;
BOOL     bDescFileValid;
HANDLE   myInstance;
HWND     hlistbox;
UINT     wHelpMessage;
DWORD    dwContext;
PINF     pinfOldDefault;
char     szDriversHlp[24];
char     szLastQuery[20];
char     szSetupInf[18];
char     szKnown[250];
char     szRestartDrv[80];
char     szUnlisted[150];
char     szRelatedDesc[30];
char     szAppName[26];
char     szDrivers[12];
char     szRemove[12];
char     szControlIni[20];
char     szSysIni[20];
char     szMCI[6];
char     szOutOfRemoveSpace[54];
char     szDriversDesc[38];
char     szUserDrivers[38];

// Where the source of files to copy is - user updates

char     szDirOfSrc[MAX_PATH];
char     szAddDriver[36];
char     szNoDesc[36];
char     szError[20];
char     szRemoveOrNot[250];
char     szRemoveOrNotStrict[250];
char     szStringBuf[128];
char     szMDrivers[38];
char     szMDrivers32[38];
char     szFullPath[MAXFILESPECLEN];
char     szSystem[MAX_PATH];
char     szOemInf[MAX_PATH];
char     aszClose[16];
char     szFileError[50];

static   HANDLE   hIList;
static   HANDLE   hWndMain;

/*
 *  Global flag telling us if we're allowed to write to ini files
 */

 BOOL IniFileWriteAllowed;

DWORD GetFileDateTime     (LPSTR);
PSTR  GetProfile          (PSTR,PSTR, PSTR, PSTR, int);
int   AddIDriver          (HWND, PIDRIVER);
void  AddIDrivers         (HWND, PSTR, PSTR);
BOOL  InitInstalled       (HWND, PSTR);
BOOL  InitAvailable       (HWND, int);
void  CloseDrivers        (HWND);
void  RemoveAvailable     (HWND);
BOOL  UserInstalled       (PSTR);
BOOL  RestartDlg          (HWND, unsigned, UINT, LONG);
BOOL  AddUnlistedDlg      (HWND, unsigned, UINT, LONG);
int   AvailableDriversDlg (HWND, unsigned, UINT, LONG);
BOOL  ListInstalledDlg    (HWND, unsigned, UINT, LONG);
LONG  CPlApplet           (HWND, unsigned, UINT, LONG);
void  ReBoot              (HWND);

/*
 *  CheckSectionAccess()
 *
 *  See if we can read/write to a given section
 */


 BOOL CheckSectionAccess(char *szIniFile, char *SectionName)
 {
     static char TestKey[] = "TestKey!!!";
     static char TestData[] = "TestData";
     static char ReturnData[50];

    /*
     *   Check we can write, read back and delete our key
     */

     return WritePrivateProfileString(SectionName,
                                      TestKey,
                                      TestData,
                                      szIniFile) &&

            GetPrivateProfileString(SectionName,
                                    TestKey,
                                    "",
                                    ReturnData,
                                    sizeof(ReturnData),
                                    szIniFile) == (DWORD)strlen(TestData) &&

            WritePrivateProfileString(SectionName,
                                      TestKey,
                                      NULL,
                                      szIniFile);
 }


/*
 *  CheckIniAccess()
 *
 *  Checks access to our 2 .ini file sections - DRIVERS_SECTION and
 *  MCI_SECTION by just writing and reading some junk
 *
 *  Basically if we don't have access to these sections we're not
 *  going to allow Add and Remove.  The individual MCI drivers must
 *  take care not to put their data into non-writeable storage although
 *  this completely messes up the default parameters thing so we're going
 *  to put these into a well-known key in the win.ini file (ie per user).
 *
 */

 BOOL CheckIniAccess(void)
 {
     return CheckSectionAccess(szSysIni, szDrivers) &&
            CheckSectionAccess(szSysIni, szMCI) &&
            CheckSectionAccess(szControlIni, szUserDrivers) &&
            CheckSectionAccess(szControlIni, szDriversDesc) &&
            CheckSectionAccess(szControlIni, szRelatedDesc);
 }

/*
 *  QueryRemoveDrivers()
 *
 *  Ask the user if they're sure.  If the Driver is one required by the
 *  system (ie not listed in [Userinstallable.drivers] in control.ini)
 *  warn the user of that too.
 */

 BOOL NEAR PASCAL QueryRemoveDrivers(HWND hDlg, PSTR szKey, PSTR szDesc)
 {
     char bufout[MAXSTR];

     if (UserInstalled(szKey))
          wsprintf(bufout, szRemoveOrNot, (LPSTR)szDesc);
     else
          wsprintf(bufout, szRemoveOrNotStrict, (LPSTR)szDesc);

     return (MessageBox(hDlg, bufout, szRemove,
                    MB_ICONEXCLAMATION | MB_TASKMODAL | MB_YESNO) == IDYES );
 }

/*
 *  GetProfile()
 *
 *  Get private profile strings.
 */

 PSTR GetProfile(PSTR pstrAppName, PSTR pstrKeyName, PSTR pstrIniFile,
                 PSTR pstrRet, int iSize)
 {
     char szNULL[2];

     szNULL[0] = '\0';
     GetPrivateProfileString(pstrAppName, (pstrKeyName==NULL) ? NULL :
         (LPSTR)pstrKeyName, szNULL, pstrRet, iSize, pstrIniFile);
     return(pstrRet);
 }

/*********************************************************************
 *
 *  AddIDriver()
 *
 *  Add the passed driver to the Installed drivers list box, return
 *  the list box index.
 *
 *********************************************************************/

int AddIDriver(HWND hWnd, PIDRIVER pIDriver)
{
    int     iIndex;
    PIDRIVER pIDriverlocal;

    iIndex = (int)SendMessage(hWnd, LB_GETCOUNT, 0, 0L);
    while ( iIndex-- > 0)
    if ( (int)(pIDriverlocal = (PIDRIVER)SendMessage(hWnd, LB_GETITEMDATA, iIndex, 0L)) != LB_ERR)
        if (!FileNameCmp(pIDriverlocal->szFile, pIDriver->szFile))
           return(0);

    //
    // create the list box item
    //
    if ((iIndex = (int)SendMessage(hWnd, LB_ADDSTRING, 0,
        (LONG)(LPSTR)pIDriver->szDesc)) != LB_ERR)
        SendMessage(hWnd, LB_SETITEMDATA, iIndex, (LONG)pIDriver);

    return(iIndex);
}

/*********************************************************************
 *
 *  AddIDrivers()
 *
 *  Add drivers in the passed key strings list to the Installed Drivers box.
 *
 *********************************************************************/

void AddIDrivers(HWND hWnd, PSTR pstrKeys, PSTR pstrSection)
{
    PIDRIVER    pIDriver;
    HWND        hWndInstalled;
    PSTR        pstrKey;
    PSTR        pstrDesc;

    hWndInstalled = GetDlgItem(hWnd, LB_INSTALLED);
    pstrKey = pstrKeys;
    pstrDesc = (PSTR)LocalAlloc(LPTR, MAXSTR);

   /*
    *  parse key strings for profile, and make IDRIVER structs
    */

    while ( *pstrKey )
    {
        pIDriver = (PIDRIVER)LocalAlloc(LPTR, sizeof(IDRIVER));
        if ( pIDriver )
        {
            PSTR        pstr;

            if (*GetProfile(pstrSection, pstrKey, szSysIni, pIDriver->szFile,
                sizeof(pIDriver->szFile)) == '\0')
            {
                LocalFree((HANDLE)pIDriver);
                goto nextkey;
            }

            for ( pstr=pIDriver->szFile; *pstr && (*pstr!=COMMA) &&
                (*pstr!=SPACE); pstr++ )
                    ;
            *pstr = '\0';

#ifdef TRASHDRIVERDESC
            if (bDescFileValid)
#endif
              /*
               *  try to load the cached description
               */

               GetProfile(szDriversDesc,
                          pIDriver->szFile,
                          szControlIni,
                          pIDriver->szDesc,
                          sizeof(pIDriver->szDesc));

           /*
            *  if we failed, then try to get the information from
            *  mmdriver.inf or the exehdr
            */

            if (pIDriver->szDesc[0] == '\0')
            {
               if (LoadDesc(pIDriver, pstrKey, pstrDesc) != DESC_NOFILE)
               {
                   if (!*pstrDesc)
                   {
                       /*
                        *  failed to load a description.
                        *  The file isn't in setup.inf
                        *  and doesn't have exehdr information
                        */

                        lstrcpy(pIDriver->szDesc, pIDriver->szFile);
                        lstrcat(pIDriver->szDesc, szNoDesc);
                   }
                   else
                        lstrcpy(pIDriver->szDesc, pstrDesc);

                   WritePrivateProfileString(szDriversDesc, pIDriver->szFile,
                               pIDriver->szDesc, szControlIni);
               } else {
                    LocalFree((HANDLE)pIDriver);
                    goto nextkey;
               }
            }

            strncpy(pIDriver->szAlias, pstrKey, sizeof(pIDriver->szAlias));
            pIDriver->szAlias[sizeof(pIDriver->szAlias) - 1] = 0;
            mbstowcs(pIDriver->wszAlias, pIDriver->szAlias, MAX_PATH);

            strncpy(pIDriver->szSection, pstrSection,sizeof(pIDriver->szSection));
            pIDriver->szSection[sizeof(pIDriver->szSection) - 1] = 0;
            mbstowcs(pIDriver->wszSection, pIDriver->szSection, MAX_PATH);

            pIDriver->KernelDriver = IsFileKernelDriver(pIDriver->szFile);
            pIDriver->fQueryable = pIDriver->KernelDriver ? 0 : -1;

            if (AddIDriver(hWndInstalled, pIDriver) < LB_OKAY)
                LocalFree((HANDLE)pIDriver);
        }
        else
           break;  //ERROR Low Memory

nextkey: while (*pstrKey++);
    }
    LocalFree((HANDLE)pstrDesc);
}


/*********************************************************************
 *
 *  FindInstallableDriversSection()
 *
 *********************************************************************/

PINF FindInstallableDriversSection(PINF pinf)
{
    PINF pinfFound;

    pinfFound = infFindSection(pinf, szMDrivers32);

    if (pinfFound == NULL) {
        pinfFound = infFindSection(pinf, szMDrivers);
    }

    return pinfFound;
}


/*********************************************************************
 *
 *  InitInstalled()
 *
 *  Add the drivers installed in [DRIVERS] and [MCI] to the Installed
 *  Drivers list box.
 *
 *********************************************************************/

BOOL InitInstalled(HWND hWnd, PSTR pstrSection)
{
    BOOL    bSuccess=FALSE;
    PSTR    pstr;

#ifdef TRASHDRIVERDESC
    UINT    wTime;
    BOOL    fForce;
    char    szOut[10];

    wTime = LOWORD(GetFileDateTime(szControlIni)) >> 1;
    if (fForce = (GetPrivateProfileInt((LPSTR)szUserDrivers,
                   (LPSTR)szLastQuery,  0, (LPSTR)szControlIni) != wTime))
    {
        wsprintf(szOut, "%d", wTime);
        WritePrivateProfileString((LPSTR)szUserDrivers, (LPSTR)szLastQuery,
                                        szOut, (LPSTR)szControlIni);
        WritePrivateProfileString((LPSTR)szDriversDesc, NULL, NULL,
                                                (LPSTR)szControlIni);
        bDescFileValid = FALSE;
    }
    else
        bDescFileValid = TRUE;
#endif


    pstr = (PSTR)LocalAlloc(LPTR, SECTION);
    if ( pstr )
    {
        if (*GetProfile(pstrSection, NULL, szSysIni, pstr, SECTION ))
        {
            AddIDrivers(hWnd,pstr,pstrSection);
            bSuccess = TRUE;
        }

        LocalFree((HANDLE)pstr);
    }
    return(bSuccess);
}

static VOID CancelToClose(HWND hwnd)
{
    char    aszText[sizeof(aszClose)];

    GetDlgItemText(hwnd, IDCANCEL, aszText, sizeof(aszText));
    if (lstrcmp(aszText, aszClose))
        SetDlgItemText(hwnd, IDCANCEL, aszClose);
}

static BOOL fRemove=FALSE;

/********************************************************************
 *
 *  ListInstalledDlg()
 *
 *  Display list of installed installable drivers.  Return TRUE/FALSE
 *  indicating if should restart windows.
 *
 ********************************************************************/

BOOL ListInstalledDlg(HWND hDlg, UINT uMsg, UINT wParam, LONG lParam)
{
    DRVCONFIGINFO   DrvConfigInfo;
    HANDLE          hWndI, hWnd;
    PIDRIVER        pIDriver;
    int             iIndex;

    switch ( uMsg )
    {
        case WM_INITDIALOG:

            wsStartWait();

//   Window is redrawn twice at startup but showwindow does not fix it
//          ShowWindow(hDlg, TRUE);

            hWndI = GetDlgItem(hDlg, LB_INSTALLED);
            SendMessage(hWndI,WM_SETREDRAW, FALSE, 0L);

           /*
            *  Handle the fact that we may not be able to update our .ini
            *  sections
            *
            */

            IniFileWriteAllowed = CheckIniAccess();

            if (!IniFileWriteAllowed) {
                EnableWindow(GetDlgItem(hDlg, ID_ADD),FALSE);
                {
                    char szCantAdd[120];
                    LoadString(myInstance, IDS_CANTADD, szCantAdd,
                                                        sizeof(szCantAdd));
                    MessageBox(hDlg, szCantAdd, szError,
                                    MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
                }
            }


            if (!(InitInstalled(hDlg, szDrivers) | InitInstalled(hDlg, szMCI)))
            {
                EnableWindow(GetDlgItem(hDlg, ID_CONFIGURE),FALSE);
                EnableWindow(GetDlgItem(hDlg, ID_REMOVE),FALSE);
            }


            SendMessage(hWndI, LB_SETCURSEL, 0, 0L );

            PostMessage(hDlg, WM_COMMAND, MAKELONG(LB_INSTALLED, LBN_SELCHANGE),
                        (LONG)hWndI);
            SendMessage(hWndI,WM_SETREDRAW, TRUE, 0L);
            wsEndWait();

            break;

        case WM_COMMAND:
            hlistbox = hWndI = GetDlgItem(hDlg, LB_INSTALLED);
            hWndMain = hDlg;

            iIndex = (int)SendMessage(hWndI, LB_GETCURSEL, 0, 0L);

            pIDriver =  (PIDRIVER)SendMessage(hWndI, LB_GETITEMDATA, iIndex, 0L);

            switch ( LOWORD(wParam ))
            {
                case  IDH_CHILD_DRIVERS:
                  goto DoHelp;

                case LB_INSTALLED:
                    switch ( HIWORD(wParam) )
                    {
                        case LBN_SELCHANGE:
                        case LBN_SETFOCUS:
                        {
                            BOOL fConfigurable;

                            fConfigurable = ((iIndex >= LB_OKAY) && IsConfigurable(pIDriver, hDlg));
                            EnableWindow(GetDlgItem(hDlg, ID_CONFIGURE),
                                         fConfigurable);
                            // Only enable the button if we are not in the
                            // process of removing a driver.  Otherwise a
                            // button click can get stacked up, the line
                            // in the list box can get removed, perhaps as a
                            // related driver, and bad things start to happen.
                            EnableWindow(GetDlgItem(hDlg, ID_REMOVE),
                                         IniFileWriteAllowed && !fRemove &&
                                            iIndex != LB_ERR);
                            break;
                        }
                        case LBN_DBLCLK:
                            if ( IsWindowEnabled(hWnd = GetDlgItem(hDlg, ID_CONFIGURE)) )
                                PostMessage(hDlg, WM_COMMAND, MAKELONG(ID_CONFIGURE, BN_CLICKED),(LONG)hWnd);
                            break;
                    }
                    break;

                case ID_ADD:
                    DialogBox(myInstance, MAKEINTRESOURCE(DLG_KNOWN), hDlg,
                        AvailableDriversDlg);
                    CancelToClose(hDlg);
                    break;

                case ID_CONFIGURE:
                    {
                        HANDLE hDriver;

                        hDriver = OpenDriver(pIDriver->wszAlias,
                                             pIDriver->wszSection,
                                             0L);

                        if (hDriver)
                        {
                            InitDrvConfigInfo(&DrvConfigInfo, pIDriver);
                            if ((SendDriverMessage(
                                     hDriver,
                                     DRV_CONFIGURE,
                                     (LONG)hDlg,
                                     (LONG)(LPDRVCONFIGINFO)&DrvConfigInfo) ==
                                DRVCNF_RESTART))
                            {
                               iRestartMessage= 0;
                               DialogBox(myInstance,
                                  MAKEINTRESOURCE(DLG_RESTART), hDlg, RestartDlg);
                            }
                            CloseDriver(hDriver, 0L, 0L);
                            CancelToClose(hDlg);
                        }
                        else
                            OpenDriverError(hDlg, pIDriver->szDesc, pIDriver->szFile);
                    }
                    break;

                case ID_REMOVE:
                    // Prevent any more REMOVE button presses
                    // Otherwise one can get stacked up and cause trouble,
                    // particularly if it is assocated with a driver that
                    // is automatically removed.  We have to use a static
                    // as any focus changes cause the button to change state.
                    fRemove = TRUE;
                    EnableWindow(GetDlgItem(hDlg, ID_REMOVE),FALSE);
                    if (QueryRemoveDrivers(hDlg, pIDriver->szAlias, pIDriver->szDesc))
                    {
                       LONG Status;
                       Status = PostRemove(hWndI, pIDriver, TRUE, iIndex);
                       if (Status != DRVCNF_CANCEL)
                       {
                          // As we have removed a driver we need to reset
                          // the current selection
                          PostMessage(hWndI, LB_SETCURSEL, 0, 0L );
                          iRestartMessage= IDS_RESTART_REM;
                          if (Status == DRVCNF_RESTART) {
                              DialogBox(myInstance,
                                  MAKEINTRESOURCE(DLG_RESTART), hDlg, RestartDlg);
                          }
                       }
                    }
                    /* Reenable the Config/Remove buttons */
                    fRemove = FALSE;     // Remove can be activated again

                    // Force the button state to be updated
                    PostMessage(hWndMain, WM_COMMAND,
                                MAKELONG(LB_INSTALLED, LBN_SELCHANGE),
                                (LONG)hWndI);
                    CancelToClose(hDlg);
                    break;

                case IDCANCEL:
                    wsStartWait();

                   /*
                    *  free the driver structs added as DATAITEM
                    */

                    CloseDrivers(hWndI);
                    wsEndWait();
                    EndDialog(hDlg, FALSE);
                    break;

                default:
                    return(FALSE);
            }
            break;

        case WM_DESTROY:

            return(FALSE);

        default:
            if (uMsg == wHelpMessage)
            {
DoHelp:
                WinHelp(hDlg, szDriversHlp, HELP_CONTEXT, IDH_CHILD_DRIVERS);
                return TRUE;
            }
            else
                return FALSE;
         break;
    }
    return(TRUE);
}




/*--------------------------------------------------------------------------*
 *                                                                          *
 *                                                                          *
 *  LB_AVAILABLE Dialog Routines                                            *
 *                                                                          *
 *                                                                          *
 *--------------------------------------------------------------------------*/

/*
 *  DLG: LB_AVAILABLE
 *
 *  InitAvailable()
 *
 *  Add the available drivers from mmdriver.inf to the passed list box.
 *  The format of [Installable.drivers] in setup.inf is:
 *  profile=disk#:driverfile,"type1,type2","Installable driver Description","vxd1.386,vxd2.386","opt1,2,3"
 *
 *  for example:
 *
 *  driver1=6:sndblst.drv,"midi,wave","SoundBlaster MIDI and Waveform drivers","vdmad.386,vadmad.386","3,260"
 */

BOOL InitAvailable(HWND hWnd, int iLine)
{
    PINF    pinf;
    BOOL    bInitd=FALSE;
    PSTR    pstrKey;
    int     iIndex;
    char    szDesc[MAX_INF_LINE_LEN];

    SendMessage(hWnd,WM_SETREDRAW, FALSE, 0L);

   /*
    *  Parse the list of keywords and load their strings
    */

    for (pinf = FindInstallableDriversSection(NULL); pinf; pinf = infNextLine(pinf))
    {
        //
        // found at least one keyname!
        //
        bInitd = TRUE;
        if ( (pstrKey = (PSTR)LocalAlloc(LPTR, MAX_SYS_INF_LEN)) != NULL )
                infParseField(pinf, 0, pstrKey);
        else
            break;
       /*
        *  add the installable driver's description to listbox, and filename as data
        */

        infParseField(pinf, 3, szDesc);

        if ( (iIndex = (int)SendMessage(hWnd, LB_ADDSTRING, 0, (LONG)(LPSTR)szDesc)) != LB_ERR )

            SendMessage(hWnd, LB_SETITEMDATA, iIndex, (LONG)pstrKey);

    }

    if (iLine == UNLIST_LINE)
    {
        //
        // Add the "Install unlisted..." choice to the top of the list
        // box.
        LoadString(myInstance, IDS_UPDATED, szDesc, sizeof(szDesc));
        if ((iIndex = (int)(LONG)SendMessage(hWnd, LB_INSERTSTRING, 0, (LPARAM)(LPSTR)szDesc)) != LB_ERR)
            SendMessage(hWnd, LB_SETITEMDATA, (WPARAM)iIndex, (LPARAM)0);
     }
     if (bInitd)

         SendMessage(hWnd, LB_SETCURSEL, 0, 0L );


     SendMessage(hWnd,WM_SETREDRAW, TRUE, 0L);
     return(bInitd);
}


/*
 *  DLG: LB_AVAILABLE
 *
 *  RemoveAvailable()
 *
 *  Remove all drivers from the listbox and free all storage associated with
 *  the keyname
 */

void RemoveAvailable(HWND hWnd)
{
    int iIndex;
    HWND hWndA;
    PSTR pstrKey;

    hWndA = GetDlgItem(hWnd, LB_AVAILABLE);
    iIndex = (int)SendMessage(hWndA, LB_GETCOUNT, 0, 0L);
    while ( iIndex-- > 0)
    {
        if (( (int)(pstrKey = (PSTR)SendMessage(hWndA, LB_GETITEMDATA, iIndex,
            0L)) != LB_ERR ) && pstrKey)
            LocalFree((HLOCAL)pstrKey);
    }
}


/*
 *  DLG: LB_AVAILABLE
 *
 *  AvailableDriversDlg()
 *
 *  List the available installable drivers or return FALSE if there are none.
 */

int AvailableDriversDlg(HWND hWnd, UINT uMsg, UINT wParam, LONG lParam)
{
    PSTR    pstrKey;    //-jyg- added

    HWND    hWndA, hWndI;
    int     iIndex;

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            ShowWindow(hWnd, TRUE);
            wsStartWait();
            if (pinfOldDefault)
            {
                infSetDefault(pinfOldDefault);
                pinfOldDefault = NULL;
            }

            if ( !InitAvailable(hWndA = GetDlgItem(hWnd, LB_AVAILABLE), UNLIST_LINE))
            {
               /*
                *  We weren't able to find the [installable.drivers] section
                *  of the
                *  mmdriver.inf OR it was corrupt.  Go ahead and query the
                *  user to find an oemsetup.inf to make our default.  This
                *  is a bad state.
                */
                EndDialog(hWnd, FALSE);
                bFindOEM = TRUE;
                strcpy(szDrv, szOemInf);
                if (DialogBox(myInstance, MAKEINTRESOURCE(DLG_INSERTDISK),
                        hWnd,  AddDriversDlg) == TRUE)
                    PostMessage(hWnd, WM_INITDIALOG, 0, 0L);
                else
                    pinfOldDefault = infSetDefault(pinfOldDefault);

                bFindOEM = FALSE;
            }
            wsEndWait();
            break;

        case WM_COMMAND:

            switch ( LOWORD(wParam ))
            {
                case  IDH_DLG_ADD_DRIVERS:
                  goto DoHelp;


                case LB_AVAILABLE:

                    // Hm... We've picked it.

                    if ( HIWORD(wParam) == LBN_DBLCLK )
                        SendMessage(hWnd, WM_COMMAND, IDOK, 0L);
                    break;

                case IDOK:

                   /*
                    *  We've made our selection
                    */

                    hWndA = GetDlgItem(hWnd, LB_AVAILABLE);

                    if ( (iIndex = (int)SendMessage(hWndA, LB_GETCURSEL, 0, 0L)) != LB_ERR)
                    {
                        if (!iIndex)
                        {
                           /*
                            *  The first entry is for OEMs
                            */

                            int iFound;
                            bBadOemSetup = FALSE;

                            bFindOEM = TRUE;
                            hMesgBoxParent = hWnd;
                            while ((iFound = DialogBox(myInstance,
                                    MAKEINTRESOURCE(DLG_INSERTDISK), hWnd,
                                            AddDriversDlg)) == 2);
                            if (iFound == 1)
                            {
                                    RemoveAvailable(hWnd);
                                    SendDlgItemMessage(hWnd, LB_AVAILABLE,
                                            LB_RESETCONTENT, 0, 0L);
                                    PostMessage(hWnd, WM_INITDIALOG, 0, 0L);
                            }
                            bFindOEM = FALSE;
                        }
                        else
                        {
                           /*
                            *  The user selected an entry from our .inf
                            */

                            wsStartWait();

                           /*
                            *  The  data associated with the list item is
                            *  the driver key name (field 0 in the inf file).
                            */

                            pstrKey = (PSTR)SendMessage(hWndA, LB_GETITEMDATA, iIndex, 0L);
                            bCopyingRelated = FALSE;
                            bQueryExist = TRUE;

                            if (InstallDrivers(hWndMain, hWnd, pstrKey))
                            {
                               hWndI = GetDlgItem(hWndMain, LB_INSTALLED);
                               PostMessage(hWndI, LB_SETCURSEL, 0, 0L );
                               PostMessage(hWndMain, WM_COMMAND,
                                           MAKELONG(LB_INSTALLED, LBN_SELCHANGE),
                                           (LONG)hWndI);
                               wsEndWait();

                              /*
                               *  If bRestart is true then the system must
                               *  be restarted to activate these changes
                               */

                               if (bRestart)
                               {
                                  iRestartMessage= IDS_RESTART_ADD;
                                  DialogBox(myInstance,
                                          MAKEINTRESOURCE(DLG_RESTART), hWnd,
                                              RestartDlg);
                               }
                            }
                            else
                               wsEndWait();

                            bRestart = FALSE;
                            bRelated = FALSE;
                        }
                    }
                    EndDialog(hWnd, FALSE);
                    break;

                case IDCANCEL:
                    EndDialog(hWnd, FALSE);
                    break;

                default:
                    return(FALSE);
            }
            break;

        case WM_DESTROY:
            //
            // free the strings added as DATAITEM to the avail list

            RemoveAvailable(hWnd);
            return(FALSE);

        default:
            if (uMsg == wHelpMessage)
            {
DoHelp:
                WinHelp(hWnd, szDriversHlp, HELP_CONTEXT, IDH_DLG_ADD_DRIVERS);
                return TRUE;
            }
            else
                return FALSE;
         break;
    }
    return(TRUE);
}

/* Main CPL proc.  This is for communication with CPL.EXE
*/

LONG CPlApplet(HWND hCPlWnd, UINT uMsg, UINT lParam1, LONG lParam2)
{
    int i;
    LPNEWCPLINFO   lpCPlInfo;
    char strOldDir[MAX_PATH], strSysDir[MAX_PATH];


    switch( uMsg )
    {
        case CPL_INIT:
            wHelpMessage = RegisterWindowMessage("ShellHelp");
            return(TRUE);

        case CPL_GETCOUNT:
            return(1);

        case CPL_NEWINQUIRE:
            i = 0;
            lpCPlInfo = (LPNEWCPLINFO)lParam2;
            lpCPlInfo->hIcon = LoadIcon(myInstance,
                                          MAKEINTRESOURCE(applets[i].idIcon));

            if(!LoadString(myInstance, applets[i].idName, lpCPlInfo->szName,
                                          sizeof(lpCPlInfo->szName)))
                lpCPlInfo->szName[0] = 0;

            if(!LoadString(myInstance, applets[i].idInfo, lpCPlInfo->szInfo,
                                          sizeof(lpCPlInfo->szInfo)))
                lpCPlInfo->szInfo[0] = 0;

            lpCPlInfo->dwSize = sizeof(NEWCPLINFO);
            lpCPlInfo->lData = (LONG)i;
            lpCPlInfo->dwHelpContext = applets[i].dwContext;
            lstrcpy(lpCPlInfo->szHelpFile, applets[i].pszHelp);
            break;

        case CPL_DBLCLK:
            if (bDriversAppInUse)
            {
               MessageBeep(0);
               return (FALSE);
            }

            bDriversAppInUse = TRUE;
            pinfOldDefault = NULL;

            GetSystemDirectory(strSysDir, MAX_PATH);
            GetCurrentDirectory(MAX_PATH, strOldDir);

           /*
            *  Switch to the system directory for our work
            */

            SetCurrentDirectory(strSysDir);

           /*
            *  Call initialization routine
            */

            wsInfParseInit();

            if (DialogBox(myInstance,
                          MAKEINTRESOURCE(DLG_INSTALLED),
                          hCPlWnd,
                          ListInstalledDlg)) {
               /*
                *  they changed configuration of a driver and said 'yes, restart'
                */

                ReBoot(hCPlWnd);
            }
            infClose(NULL);

           /*
            *  Restore current directory setting
            */

            SetCurrentDirectory(strOldDir);

            //SetActiveWindow(hCPlWnd);
            bDriversAppInUse = FALSE;
            return (TRUE);
            break;

        case CPL_EXIT:
            break;
    }
    return(0L);
}

BOOL DllInitialize( IN PVOID hInstance
                  , IN DWORD ulReason
                  , IN PCONTEXT pctx OPTIONAL
                  )
{
    if (ulReason != DLL_PROCESS_ATTACH)
        return TRUE;

    myInstance = hInstance;
    LoadString(myInstance, IDS_CLOSE,  aszClose, sizeof(aszClose));
    LoadString(myInstance, IDS_DRIVERDESC, szDriversDesc, sizeof(szDriversDesc));
    LoadString(myInstance, IDS_FILE_ERROR, szFileError, sizeof(szFileError));
    LoadString(myInstance, IDS_INSTALLDRIVERS, szMDrivers, sizeof(szMDrivers));
    LoadString(myInstance, IDS_INSTALLDRIVERS32, szMDrivers32, sizeof(szMDrivers));
    LoadString(myInstance, IDS_RELATEDDESC, szRelatedDesc, sizeof(szRelatedDesc));
    LoadString(myInstance, IDS_USERINSTALLDRIVERS, szUserDrivers, sizeof(szUserDrivers));
    LoadString(myInstance, IDS_UNLISTED, (LPSTR)szUnlisted, sizeof(szUnlisted));
    LoadString(myInstance, IDS_KNOWN, szKnown, sizeof(szKnown));
    LoadString(myInstance, IDS_OEMSETUP, szOemInf, sizeof(szOemInf));
    LoadString(myInstance, IDS_SYSTEM, szSystem, sizeof(szSystem));
    LoadString(myInstance, IDS_OUT_OF_REMOVE_SPACE, szOutOfRemoveSpace, sizeof(szOutOfRemoveSpace));
    LoadString(myInstance, IDS_NO_DESCRIPTION, szNoDesc, sizeof(szNoDesc));
    LoadString(myInstance, IDS_ERRORBOX, szError, sizeof(szError));
    LoadString(myInstance, IDS_REMOVEORNOT, szRemoveOrNot, sizeof(szRemoveOrNot));
    LoadString(myInstance, IDS_REMOVEORNOTSTRICT, szRemoveOrNotStrict, sizeof(szRemoveOrNotStrict));
    LoadString(myInstance, IDS_SETUPINF, szSetupInf, sizeof(szSetupInf));
    LoadString(myInstance, IDS_APPNAME, szAppName, sizeof(szAppName));

    LoadString(myInstance, IDS_DRIVERS, szDrivers, sizeof(szDrivers));
    LoadString(myInstance, IDS_REMOVE, szRemove, sizeof(szRemove));
    LoadString(myInstance, IDS_CONTROLINI, szControlIni, sizeof(szControlIni));
    LoadString(myInstance, IDS_SYSINI, szSysIni, sizeof(szSysIni));
    LoadString(myInstance, IDS_MCI, szMCI, sizeof(szMCI));
    LoadString(myInstance, IDS_DEFDRIVE, szDirOfSrc, sizeof(szDirOfSrc));
    LoadString(myInstance, IDS_CONTROL_HLP, szDriversHlp, sizeof(szDriversHlp));
    LoadString(myInstance, IDS_LASTQUERY, szLastQuery, sizeof(szLastQuery));


    applets[0].idIcon = DRIVERS_ICON;
    applets[0].idName = IDS_NAME;
    applets[0].idInfo = IDS_INFO;
    applets[0].bEnabled = TRUE;
    applets[0].dwContext = IDH_CHILD_DRIVERS;
    applets[0].pszHelp = szDriversHlp;
    return TRUE;
}

void DeleteCPLCache(void)
{
    HKEY hKeyCache;

    if (ERROR_SUCCESS ==
        RegOpenKey(HKEY_CURRENT_USER,
                   TEXT("Control Panel\\Cache\\multimed.cpl"),
                   &hKeyCache)) {
        for ( ; ; ) {
            TCHAR Name[50];

            if (ERROR_SUCCESS ==
                RegEnumKey(hKeyCache,
                           0,
                           Name,
                           sizeof(Name) / sizeof(Name[0]))) {
                HKEY hSubKey;

                RegDeleteKey(hKeyCache, Name);
            } else {
                break;    // leave loop
            }
        }

        RegDeleteKey(hKeyCache, NULL);
        RegCloseKey(hKeyCache);
    }
}


/*
** RestartDlg()
**
** Offer user the choice to (not) restart windows.
*/
BOOL RestartDlg(HWND hDlg, unsigned uiMessage, UINT wParam, LONG lParam)
{
    switch (uiMessage)
    {
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
               case IDCANCEL:
                    //
                    // don't restart windows
                    //
                    EndDialog(hDlg, FALSE);
                    break;

                case IDOK:
                    //
                    // do restart windows, *dont* dismiss dialog incase
                    // the user canceled it.
                    //
                    ReBoot(hDlg);
                    SetActiveWindow(hDlg);
                    //EndDialog(hDlg, TRUE);
                    break;

                default:
                    return FALSE;
            }
            return TRUE;

        case WM_INITDIALOG:
              /*
              **  Delete the control panel's cache so it will get it
              **  right!
              */

              DeleteCPLCache();


              if (iRestartMessage)
              {
                char szMesg1[300];
                char szMesg2[300];

                LoadString(myInstance, iRestartMessage, szMesg1, sizeof(szMesg1));
                wsprintf(szMesg2, szMesg1, (LPSTR)szRestartDrv);
                SetDlgItemText(hDlg, IDS_RESTARTTEXT, (LPSTR)szMesg2);
              }
              return TRUE;

        case WM_KEYUP:
            if (wParam == VK_F3)
                //
                // don't restart windows
                //
                EndDialog(hDlg, FALSE);
            break;

        default:
            break;
    }
    return FALSE;
}

/*
 * UserInstalled()
 *
 *
 */

BOOL UserInstalled(PSTR szKey)
{
        char buf[MAXSTR];

        if (*GetProfile(szUserDrivers, (LPSTR)szKey, szControlIni, buf, MAXSTR) != '\0')
            return(TRUE);
        else
            return(FALSE);
}

/*
 *   AddUnlistedDlg()
 *
 *   The following function processes requests by the user to install unlisted
 *   or updated drivers.
 *
 *   PARAMETERS:  The normal Dialog box parameters
 *   RETURN VALUE:  The usual Dialog box return value
 */

BOOL AddUnlistedDlg(HWND hDlg, unsigned nMsg, UINT wParam, LONG lParam)
{
  switch (nMsg)
  {
      case WM_INITDIALOG:
      {
          HWND hListDrivers;
          BOOL bFoundDrivers;

          wsStartWait();
          hListDrivers = GetDlgItem(hDlg, LB_UNLISTED);

          /* Search for drivers */
          bFoundDrivers = InitAvailable(hListDrivers, NO_UNLIST_LINE);
          if (!bFoundDrivers)
          {
                //
                // We weren't able to find the MMDRIVERS section of the
                // setup.inf OR it was corrupt.  Go ahead and query the
                // user to find an oemsetup.inf to make our default.  This
                // is a bad state.
                //

                int iFound;

                bFindOEM = TRUE;
                bBadOemSetup = TRUE;
                while ((iFound = DialogBox(myInstance,
                        MAKEINTRESOURCE(DLG_INSERTDISK), hMesgBoxParent,
                                AddDriversDlg)) == 2);
                bFindOEM = FALSE;
                if (iFound == 1)
                {
                        SendDlgItemMessage(hDlg, LB_AVAILABLE,
                                LB_RESETCONTENT, 0, 0L);
                        PostMessage(hDlg, WM_INITDIALOG, 0, 0L);
                }
                EndDialog(hDlg, FALSE);
          }
          SendMessage(hListDrivers, LB_SETCURSEL, 0, 0L);
          wsEndWait();

          break;
        }

      case WM_COMMAND:
        switch (LOWORD(wParam))
        {
            case IDH_DLG_ADD_UNKNOWN:
              goto DoHelp;

            case LB_UNLISTED:
              if (HIWORD(wParam) != LBN_DBLCLK)
                  break;

              // else Fall through here
            case IDOK:
            {
             HWND hWndI, hWndA;
             int iIndex;
             PSTR pstrKey;

             hWndA = GetDlgItem(hDlg, LB_UNLISTED);
             if ( (iIndex = (int)SendMessage(hWndA, LB_GETCURSEL, 0, 0L))
                                                             != LB_ERR)
             {
                wsStartWait();
                pstrKey = (PSTR)SendMessage(hWndA, LB_GETITEMDATA, iIndex, 0L);
                bCopyingRelated = FALSE;
                bQueryExist = TRUE;
                if (InstallDrivers(hWndMain, hDlg, pstrKey))
                {
                   hWndI = GetDlgItem(hWndMain, LB_INSTALLED);
                   PostMessage(hWndI, LB_SETCURSEL, 0, 0L );
                   PostMessage(hWndMain, WM_COMMAND,
                               MAKELONG(LB_INSTALLED, LBN_SELCHANGE),
                               (LONG)hWndI);

                   wsEndWait();
                   if (bRestart)
                   {
                      iRestartMessage= IDS_RESTART_ADD;
                      DialogBox(myInstance,   MAKEINTRESOURCE(DLG_RESTART),
                                                      hDlg, RestartDlg);
                   }
                 }
                 else
                   wsEndWait();
                 bRelated = FALSE;
                 bRestart = FALSE;
              }
              EndDialog(hDlg, FALSE);
            }
            break;

            case IDCANCEL:
              EndDialog(hDlg, wParam);
              break;

            default:
              return FALSE;
          }

      default:
        if (nMsg == wHelpMessage)
        {
DoHelp:
            WinHelp(hDlg, szDriversHlp, HELP_CONTEXT, IDH_DLG_ADD_UNKNOWN);
            break;
        }
        else
            return FALSE;
   }
   return TRUE;
}
/*
 *  ReBoot()
 *
 *  Restart the system.  If this fails we put up a message box
 */

 void ReBoot(HWND hDlg)
 {
     DWORD Error;
     BOOLEAN WasEnabled;

    /*
     *  We must adjust our privilege level to be allowed to restart the
     *  system
     */

     RtlAdjustPrivilege( SE_SHUTDOWN_PRIVILEGE,
                         TRUE,
                         FALSE,
                         &WasEnabled
                       );
    /*
     *  Try to reboot the system
     */

     if (!ExitWindowsEx(EWX_REBOOT, 0xFFFFFFFF)) {

         Error = GetLastError();

        /*
         *  Put up a message box if we failed
         */

         if (Error != NO_ERROR) {
            char szCantRestart[80];
            LoadString(myInstance,
                       Error == ERROR_PRIVILEGE_NOT_HELD  ||
                       Error == ERROR_NOT_ALL_ASSIGNED  ||
                       Error == ERROR_ACCESS_DENIED ?
                           IDS_CANNOT_RESTART_PRIVILEGE :
                           IDS_CANNOT_RESTART_UNKNOWN,
                       szCantRestart,
                       sizeof(szCantRestart));

            MessageBox(hDlg, szCantRestart, szError,
                       MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
         }
     }
 }

/*
 *  CloseDrivers()
 *
 *  Make free memory for drivers.
 */
void CloseDrivers(HWND hWnd)
{
    PIDRIVER    pIDriver;
    int         iIndex;

   /*
    *  Go through the drivers remaining in the Installed list
    */
    iIndex = (int)SendMessage(hWnd, LB_GETCOUNT, 0, 0L);
    while ( iIndex-- > 0 )
        if ( (int)(pIDriver = (PIDRIVER)SendMessage(hWnd, LB_GETITEMDATA,
                                                   iIndex, 0L)) != LB_ERR)
        {
            LocalFree((HLOCAL)pIDriver);
        }

}



void OpenDriverError(HWND hDlg, LPSTR szDriver, LPSTR szFile)
{
        char szMesg[MAXSTR];
        char szMesg2[MAXSTR];

        LoadString(myInstance, IDS_INSTALLING_DRIVERS, szMesg, sizeof(szMesg));
        wsprintf(szMesg2, szMesg, szDriver, szFile);
        MessageBox(hDlg, szMesg2, szError, MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);

}
