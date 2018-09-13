/** FILE: system.c ********* Module Header ********************************
 *
 *  Control panel applet for System configuration.  This file holds
 *  everything to do with editing the boot.ini file, selecting an OS to
 *  start on the system by default or choice, display of System environment
 *  variables and display and editing of User environment variables.
 *
 * History:
 *  10:30 on Tues  09 Mar 1992  -by- Steve Cathcart [stevecat]
 *        Created
 *  05 Apr 1994  -by- Steve Cathcart [stevecat]
 *        Added GetBootDrive() and other fixes
 *  17:00 on Mon   18 Sep 1995  -by-  Steve Cathcart   [stevecat]
 *        Changes for product update - SUR release NT v4.0
 *
 *  Copyright (C) 1992-1995 Microsoft Corporation
 *
 *************************************************************************/
//==========================================================================
//                              Include files
//==========================================================================
// C Runtime
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Application specific
#include "system.h"


/*
 * These functions in SETUPDLL.DLL are ANSI only!!!!
 *
 * Therefore any functions working with this DLL MUST remain ANSI only.
 * The functions are GetRGSZEnvVar and UpdateNVRAM.
 * The structure CPEnvBuf MUST also remain ANSI only.
 */
typedef int (WINAPI *GETNVRAMPROC)(CHAR **, USHORT, CHAR *, USHORT);
typedef int (WINAPI *WRITENVRAMPROC)(DWORD, PSZ *, PSZ *);


// Windows sdk

//==========================================================================
//                            Local Definitions
//==========================================================================

#define FORMIN       0
#define FORMAX     999
#define FORDEF      30

#define NONE_LENGTH 16

#define NUM_SYS_ENV   12

//  Value to add to text string extent to allow for spacing between
//  text string and listbox borders.  USER should make allowances for
//  this spacing and set the scroll range appropriately.

#define ARBITRARY_USER_LB_EXTENT_ADDITION  6

#define BUFZ        4096
#define VALBZ        256

#define ADD_OS      1
#define CHOOSE_OS   2

#define MAX_VALUE_LEN     1024

#define LB_SYSVAR   1
#define LB_USERVAR  2

#ifndef REG_EXPAND_SZ
#define REG_EXPAND_SZ REG_SZ
#endif  //  REG_EXPAND_SZ

#define MAX_DEVICES  1024
#define MAX_ARC_NAME  512


//==========================================================================
//                            Typedefs and Structs
//==========================================================================

//  Environment variables structure
typedef struct
{
//    DWORD  dwLocation;
    DWORD  dwType;
    LPTSTR szValueName;
    LPTSTR szValue;
    LPTSTR szExpValue;
} ENVARS;

//  Registry valuename linked-list structure
typedef struct _regval
{
    struct _regval *prvNext;
    LPTSTR szValueName;
} REGVAL;

//==========================================================================
//                            External Declarations
//==========================================================================
/* Functions */
extern VOID SetDefButton( HWND hwndDlg, int idButton );

#ifdef _X86_
extern TCHAR x86DetermineSystemPartition( HWND hdlg );
#endif  // _X86_


//==========================================================================
//                     Local Data Declarations
//==========================================================================

char szBootIniA[]     = "c:\\boot.ini";
char szOSA[]          = "operating systems";

TCHAR szBootIni[]     = TEXT( "c:\\boot.ini" );
TCHAR szFlexBoot[]    = TEXT( "flexboot" );
TCHAR szMultiBoot[]   = TEXT( "multiboot" );
TCHAR szBootLdr[]     = TEXT( "boot loader" );
TCHAR szTimeout[]     = TEXT( "timeout" );
TCHAR szDefault[]     = TEXT( "default" );
TCHAR szOS[]          = TEXT( "operating systems" );
TCHAR szEnvironment[] = TEXT( "Environment" );     //  Win.ini section change name

TCHAR szUserEnv[]     = TEXT( "Environment" );
TCHAR szSysEnv[]      = TEXT( "System\\CurrentControlSet\\Control\\Session Manager\\Environment" );

TCHAR szControlPath[] = TEXT( "System\\CurrentControlSet\\Control" );

TCHAR szComputerName[] = TEXT( "ComputerName" );
TCHAR szCurrentUser[]  = TEXT( "CurrentUser" );

ARROWVSCROLL avsForSeconds = {1, -1, 5, -5, FORMAX, FORMIN, FORDEF, FORDEF};

HKEY    hkeyEnv;

HFONT   hfont;
HFONT   hfontBold;

DWORD dwUserWidth;
DWORD dwSysWidth;

BOOL bChangedDefaultButton;

BOOL bEditSystemVars = FALSE;

#ifdef JAPAN
TCHAR    szSystemFont[] = TEXT( "^?l^?r ^?S^?V^?b^?N" );
TCHAR    sz10[]         = TEXT( "10" );
#endif

#ifdef UNICODE
TCHAR    szFont[]    = TEXT( "Ms Shell Dlg" );
#else
TCHAR    szFont[]    = TEXT( "MS Shell Dlg" );
#endif

TCHAR    sz8[]       = TEXT( "8" );

WNDPROC lpfnSysLB;

DWORD    dwRestartSystem = 0;

//==========================================================================
//                      Local Function Prototypes
//==========================================================================
int  FindVar (HWND hwndLB, LPTSTR szVar);

void SetLBWidth (HWND hwndLB, LPTSTR szBuffer, DWORD dwListBox);
BOOL APIENTRY SysLBProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void SetGenLBWidth (HWND hwndLB, LPTSTR szBuffer, LPDWORD pdwWidth,
                    HANDLE hfontNew, DWORD cxExtra);

#if defined(_MIPS_) || defined(_ALPHA_)  || defined(_PPC_)

static HMODULE hmodSetupDll;   // hmod for setup - has api we need
static GETNVRAMPROC fpGetNVRAMvar;  // address of function for getting nvram vars
BOOL fCanUpdateNVRAM;
typedef struct tagEnvBuf
{
  int     cEntries;
  CHAR *  pszVars[10];
} CPEnvBuf;

////////////////////////////////////////////////////////////////////////////
//  CP_MAX_ENV assumes entire env. var. value < maxpath +
//  add 20 for various quotes
//  and 10 more for commas (see list description below)
////////////////////////////////////////////////////////////////////////////
#define CP_MAX_ENV   (MAX_PATH + 30)

CPEnvBuf CPEBOSLoadIdentifier;
BOOL fAutoLoad;

////////////////////////////////////////////////////////////////////////////
//
//  This routine will query the MIPS or Alpha NVRAM for an option passed
//  in szName and fill in the argv style pointer passed in.
//
////////////////////////////////////////////////////////////////////////////

BOOL GetRGSZEnvVar(CPEnvBuf * pEnvBuf, PCHAR pszName)
{
    CHAR   *pszCur, *p;
    int     cb, i;
    CHAR   *rgtmp[1];
    CHAR    rgchOut[CP_MAX_ENV];

    // GetNVRAMVar takes an argv[] style paramater as input, so crock
    // one up.
    rgtmp[0] = pszName;

    // GetNVRAMVar returns a 'list' of the form
    //   open-curly"string1","string2","string3"close-curly
    //
    // an empty environment string will be 5 bytes:
    // open-curly""close-curly[null-terminator]

    cb = fpGetNVRAMvar (rgtmp, (USHORT)1,
                rgchOut, (USHORT) CP_MAX_ENV);

    pEnvBuf->cEntries = 0;

    // if cb was equal to 5, the string was empty (see above comment)
    if (cb > 5)
    {
        // break the string up into array of separate strings that
        // can be put into a listbox.
        pszCur = rgchOut;

        // skip first open-curly brace
        pszCur++;

        // counter for array of strings
        i = 0;
        while (*pszCur != '}')
        {
            p = pEnvBuf->pszVars[i] = LocalAlloc (LPTR, MAX_PATH);
            // skip first quote
            pszCur++;
            while (*pszCur != '"')
               *p++ = *pszCur++;

            // skip the close quote
            pszCur++;

            // null terminate destination
            *p = '\0';

            // skip the comma if not at end of string
            if (*pszCur == ',')
            {
               pszCur++;
               // and go to next string
            }
            i++;
        }
        pEnvBuf->cEntries = i;
    }

    return pEnvBuf->cEntries;
}


////////////////////////////////////////////////////////////////////////////
// The user has made a choice among the entries.
// Now we have to arrange all the strings stored in NVRAM so
// that they  have the same ordering.  The selection is passed in,
// so what this  function does is if selection is M, it makes the Mth item
// appear first in each of the 5 environment strings and the other items
// follow it in the list.
//
// Then if the timeout button is checked, it updates the AUTOLOAD variable
// to "yes" and set the COUNTDOWN variable to the number of seconds in the
// edit control.
////////////////////////////////////////////////////////////////////////////

BOOL UpdateNVRAM(HWND hdlg, int selection, int timeout)
{
    CHAR *rgszVRAM[5] = { "SYSTEMPARTITION",
                          "OSLOADER",
                          "OSLOADPARTITION",
                          "OSLOADFILENAME",
                          "OSLOADOPTIONS"
                        };
    CPEnvBuf rgcpeb[5];


    WRITENVRAMPROC fpWriteNVRAMVar;
    int iTemp, jTemp;
    CHAR *pszSwap;
    CHAR szTemp[10];
    HMODULE hmodSetupDLL;
    BOOL bChecked;


    // args and charray are needed for call to SetNVRamVar() in SETUP
    PSZ args[2];
    CHAR chArray[CP_MAX_ENV];
    PSZ pszReturn;


    if ((hmodSetupDll = LoadLibrary (TEXT("setupdll"))) == NULL)
        return(FALSE);

    fpWriteNVRAMVar = (WRITENVRAMPROC) GetProcAddress(hmodSetupDll, "SetNVRAMVar");
    if (fpWriteNVRAMVar == NULL)
        return(FALSE);

    // 0 is always the selection when the dialog is brought up,
    // so as an optimization don't update the nvram if it's
    // not necessary.
    if (selection != 0)
    {
       // read in the strings from NVRAM.  the number of strings (other than
       // LOADIDENTIFIER is 5)
       for (iTemp = 0; iTemp < 5; iTemp++)
       {
           GetRGSZEnvVar (&rgcpeb[iTemp], rgszVRAM[iTemp]);
           // now re-order the strings to swap the 'selection-th' item
           // string with the first string.
           pszSwap = rgcpeb[iTemp].pszVars[0];
           rgcpeb[iTemp].pszVars[0] = rgcpeb[iTemp].pszVars[selection];
           rgcpeb[iTemp].pszVars[selection] = pszSwap;
       }
       // now do the same for the LOADIDENTIFIER, (this was set up earlier
       // in the processing the INITDIALOG message).
       pszSwap = CPEBOSLoadIdentifier.pszVars[0];
       CPEBOSLoadIdentifier.pszVars[0] = CPEBOSLoadIdentifier.pszVars[selection];
       CPEBOSLoadIdentifier.pszVars[selection] = pszSwap;

       // now write to NVRAM:  first write LOADIDENTIFIER, then the other 5
       // variables.
       args[0] = (PSZ)"LOADIDENTIFIER";
       args[1] = chArray;

       chArray[0] = '\0';
       for (iTemp = 0; iTemp < CPEBOSLoadIdentifier.cEntries; iTemp++)
       {
           lstrcatA (chArray, CPEBOSLoadIdentifier.pszVars[iTemp]);
           lstrcatA (chArray, ";");
       }
       // remove the last semi-colon:
       chArray[lstrlenA(chArray)-1] = '\0';

       fpWriteNVRAMVar ((DWORD)2, args, &pszReturn);

       for (iTemp = 0; iTemp < 5; iTemp++)
       {
           args[0] = rgszVRAM[iTemp];
           args[1] = chArray;
           chArray[0] = '\0';
           for (jTemp = 0; jTemp < rgcpeb[iTemp].cEntries; jTemp++)
           {
               lstrcatA (chArray, rgcpeb[iTemp].pszVars[jTemp]);
               lstrcatA (chArray, ";");
           }
           chArray[lstrlenA(chArray)-1] = '\0';

           fpWriteNVRAMVar ((DWORD)2, args, &pszReturn);
       }
    }
    args[0] = "AUTOLOAD";
    if (bChecked = IsDlgButtonChecked (hdlg, IDD_SYS_ENABLECOUNTDOWN))
       args[1] = "YES";
    else
       args[1] = "";

    fpWriteNVRAMVar ((DWORD)2, args, &pszReturn);
    if (bChecked)
    {
       args[0] = "COUNTDOWN";
       _itoa(timeout, szTemp, 10);
       args[1] = szTemp;
       fpWriteNVRAMVar ((DWORD)2, args, &pszReturn);
    }
    FreeLibrary (hmodSetupDll);

    return TRUE;
}
#endif  //  _MIPS_ || _ALPHA_ || _PPC_


////////////////////////////////////////////////////////////////////////////
//
//  Copied from winfile:
//
////////////////////////////////////////////////////////////////////////////

INT  APIENTRY GetHeightFromPointsString(LPTSTR szPoints)
{
    HDC hdc;
    INT height;

    hdc = GetDC (NULL);
    height = MulDiv(-MyAtoi(szPoints), GetDeviceCaps (hdc, LOGPIXELSY), 72);
    ReleaseDC (NULL, hdc);

    return height;
}


////////////////////////////////////////////////////////////////////////////
//
//  System Applet main dialog procedure
//
////////////////////////////////////////////////////////////////////////////

BOOL SystemDlg( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    int     nVal, nOldVal;
    BOOL    bOK;
    WORD    nCtlId;
    TCHAR   szTemp[PATHMAX];
    TCHAR   szTemp2[PATHMAX];
    HANDLE  hKey;
    TCHAR  *pszKeyName;
    TCHAR  *pszValue;
    TCHAR  *pszTemp;
    TCHAR  *bBuffer;
    LPTSTR  pszString;
    LPTSTR  pszLine;
    HWND    hwndTemp;
    int     i, n, timeout;
    int     selection;
    int     iTemp;

    ENVARS *penvar;
    REGVAL *prvFirst;
    REGVAL *prvRegVal;

static TCHAR *pszBoot = NULL;
static int nOriginalSelection;
static int nOriginalTimeout;
static BOOL bUserVars = TRUE;


    LONG    Error;
    DWORD   dwIndex;
    DWORD   dwBufz;
    DWORD   dwValz;
    DWORD   dwType;

    DWORD   dwFileAttr;

    LPARROWVSCROLL       lpAVS;

    //  ANSI string pointers

    LPSTR   pszSectionA;



    switch (message)
    {
    case WM_INITDIALOG:
        HourGlass (TRUE);

        ////////////////////////////////////////////////////////////////////
        //  Make listbox display strings in NON-BOLD font - more room
        ////////////////////////////////////////////////////////////////////

#ifdef JAPAN
        hfont = CreateFont (GetHeightFromPointsString (sz10), 0, 0, 0, 0,
                            0, 0, 0, SHIFTJIS_CHARSET, 0, 0, 0, 0,
                            szSystemFont);

        hfontBold = CreateFont (GetHeightFromPointsString (sz10), 0, 0, 0,
                                700, 0, 0, 0, SHIFTJIS_CHARSET,
                                0, 0, 0, 0, szSystemFont);
#else
        hfont = CreateFont (GetHeightFromPointsString (sz8), 0, 0, 0, 400,
                            0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                            CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                            DEFAULT_PITCH | FF_SWISS, szFont);

        hfontBold = CreateFont (GetHeightFromPointsString (sz8), 0, 0, 0,
                                700, 0, 0, 0, ANSI_CHARSET,
                                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS,
                                szFont);
#endif

        SendDlgItemMessage (hDlg, IDD_SYS_OS, WM_SETFONT,
                                   (WPARAM) hfont, MAKELPARAM(TRUE, 0));
        SendDlgItemMessage (hDlg, IDD_SYS_LB_USERVARS, WM_SETFONT,
                                   (WPARAM) hfont, MAKELPARAM(TRUE, 0));
        SendDlgItemMessage (hDlg, IDD_SYS_LB_SYSVARS, WM_SETFONT,
                                   (WPARAM) hfont, MAKELPARAM(TRUE, 0));
        SendDlgItemMessage (hDlg, IDD_SYS_VAR, WM_SETFONT,
                                   (WPARAM) hfont, MAKELPARAM(TRUE, 0));
        SendDlgItemMessage (hDlg, IDD_SYS_VALUE, WM_SETFONT,
                                   (WPARAM) hfont, MAKELPARAM(TRUE, 0));

        bChangedDefaultButton = FALSE;

        //  Reset Listbox width global vars

        dwUserWidth = dwSysWidth = 0;

        bUserVars = TRUE;

        //  Get some memory for strings

        hKey = AllocMem (BUFZ*sizeof(TCHAR));

        bBuffer = (TCHAR *) AllocMem (BUFZ*sizeof(TCHAR));
        pszString = (LPTSTR) AllocMem (BUFZ*sizeof(TCHAR));

#if defined(_MIPS_) || defined(_ALPHA_)  || defined(_PPC_)
        ////////////////////////////////////////////////////////////////////
        //  Read info from NVRAM environment variables
        ////////////////////////////////////////////////////////////////////

        fCanUpdateNVRAM = FALSE;
        fAutoLoad = FALSE;
        hwndTemp = GetDlgItem (hDlg, IDD_SYS_OS);
        if (hmodSetupDll = LoadLibrary(TEXT("setupdll")))
        {
            if (fpGetNVRAMvar = (GETNVRAMPROC)GetProcAddress(hmodSetupDll, "GetNVRAMVar"))
            {
                if (fCanUpdateNVRAM = GetRGSZEnvVar (&CPEBOSLoadIdentifier, "LOADIDENTIFIER"))
                {
                    for (iTemp = 0; iTemp < CPEBOSLoadIdentifier.cEntries; iTemp++)
                        n = SendMessageA (hwndTemp, CB_ADDSTRING, 0,
                                          (LPARAM)CPEBOSLoadIdentifier.pszVars[iTemp]);
                    // the first one is the selection we want (offset 0)
                    SendMessage (hwndTemp, CB_SETCURSEL, 0, 0L);
                    SendDlgItemMessage (hDlg, IDD_SYS_SECONDS,
                              EM_LIMITTEXT, 3, 0L);
                }
                // fCanUpdateNVRAM is a global that gets set up above
                if (fCanUpdateNVRAM)
                {
                   CPEnvBuf cpebAutoLoad, cpebTimeout;
                   // is Autoload == YES?  if so, check the checkbox
                   //    and read setting for timeouts
                   // autoload == NO? disable edit control.
                   if (GetRGSZEnvVar (&cpebAutoLoad, "AUTOLOAD"))
                   {
                      if (!lstrcmpiA (cpebAutoLoad.pszVars[0], "yes"))
                      {
                         fAutoLoad = TRUE;
                         if (GetRGSZEnvVar (&cpebTimeout, "COUNTDOWN"))
                            SetDlgItemInt (hDlg, IDD_SYS_SECONDS,
                                           atoi (cpebTimeout.pszVars[0]), FALSE);
                      }
                   }
                   CheckDlgButton (hDlg, IDD_SYS_ENABLECOUNTDOWN, fAutoLoad);
                   if (!fAutoLoad)
                   {
                       EnableWindow (GetDlgItem (hDlg, IDD_SYS_SECONDS), FALSE);
                       EnableWindow (GetDlgItem (hDlg, IDD_SYS_SECSCROLL), FALSE);
                   }
                }
                else
                {
                   // if can't set variables (no privilege), disable controls
                   EnableWindow (GetDlgItem(hDlg, IDD_SYS_SECONDS), FALSE);
                   EnableWindow (GetDlgItem(hDlg, IDD_SYS_ENABLECOUNTDOWN), FALSE);
                   EnableWindow (GetDlgItem(hDlg, IDD_SYS_SECSCROLL), FALSE);
                   EnableWindow (GetDlgItem(hDlg, IDD_SYS_HWPROFILES), FALSE);
                }
            }
            FreeLibrary (hmodSetupDll);
        }

        // default to 5 seconds for now.

#else  //  _MIPS_ || _ALPHA_ || _PPC_

        ////////////////////////////////////////////////////////////////////
        //  Read info from boot.ini file and initialize OS Group box items
        ////////////////////////////////////////////////////////////////////

        //
        //  Get correct Boot Drive - this was added because after someone
        //  boots the system, they can ghost or change the drive letter
        //  of their boot drive from "c:" to something else.
        //

//        szBootIni[0] = GetBootDrive();
        szBootIni[0] = x86DetermineSystemPartition (hDlg);

        szBootIniA[0] = (char) szBootIni[0];

        //
        //  Determine which section [boot loader]
        //                          [flexboot]
        //                       or [multiboot] is in file
        //

        n = GetPrivateProfileString (szBootLdr, NULL, NULL, szTemp2,
                                     CharSizeOf(szTemp2), szBootIni);
        if (n != 0)
            pszBoot = szBootLdr;
        else
        {
            n = GetPrivateProfileString (szFlexBoot, NULL, NULL, szTemp2,
                                         CharSizeOf(szTemp2), szBootIni);
            if (n != 0)
                pszBoot = szFlexBoot;
            else
            {
                n = GetPrivateProfileString (szMultiBoot, NULL, NULL, szTemp2,
                                             CharSizeOf(szTemp2), szBootIni);
                if (n != 0)
                {
                    pszBoot = szMultiBoot;
                }
                else
                {
                    //
                    //  This final case is here because I want to DEFAULT
                    //  to "[boot loader]" as the section name to use in
                    //  the event that we do not find any section in the
                    //  boot.ini file.
                    //

                    pszBoot = szBootLdr;
                }
            }

        }

        //  Get info under [*pszBoot] section - timeout & default OS path

        timeout = GetPrivateProfileInt (pszBoot, szTimeout, 0, szBootIni);

        SetDlgItemInt (hDlg, IDD_SYS_SECONDS, timeout, FALSE);

        nOriginalTimeout = timeout;

        //
        //  Get the "Default" os selection
        //

        szTemp2[0] = TEXT('\0');

        GetPrivateProfileString (pszBoot, szDefault, NULL, szTemp2,
                                 CharSizeOf(szTemp2), szBootIni);

        //
        //  Display all choices under [operating system] in boot.ini file
        //  in combobox for selection
        //

        hwndTemp = GetDlgItem (hDlg, IDD_SYS_OS);

        selection = -1;

        //
        //  ANSI Buffer!
        //

        pszSectionA = (LPSTR) AllocMem (BUFZ);

        //
        //  Get entire section under OS to properly show user choices
        //

        n = GetPrivateProfileSectionA (szOSA, pszSectionA, BUFZ, szBootIniA);

        if ((n >= BUFZ-2) || (n == 0))
        {
ErrorReadingSection:
            //  Error reading data
            FreeMem ((LPVOID)pszSectionA, BUFZ);
            goto ContinueSystemInit;
        }

        //
        //  Check for api errors and NoOptions
        //

        if ((pszSectionA == NULL) || ((*pszSectionA == '\0') && (*(pszSectionA+1) == '\0')))
            goto ErrorReadingSection;

        pszKeyName = (TCHAR *) hKey;

        //
        //  Convert entire buffer from OEM to UNICODE
        //

        MultiByteToWideChar (CP_OEMCP, MB_PRECOMPOSED, pszSectionA, n+2, pszKeyName, BUFZ);

        FreeMem ((LPVOID)pszSectionA, BUFZ);

        //
        //  Continue until we reach end of buffer, marked by Double '\0'
        //

        while (*(pszKeyName+1) != TEXT('\0'))
        {
            pszLine = pszKeyName;

            //
            //  Get pointer to next line in buffer.
            //

            pszKeyName += lstrlen (pszKeyName) + 1;

            //
            //  Find LHS/RHS delimiter to separate strings
            //

            pszValue = _tcsstr (pszLine, TEXT("="));

            if (pszValue && (pszValue != pszLine))
            {
                *pszValue = '\0';
                pszValue++;
            }
            else
            {
                pszValue = pszLine;
            }

            //
            //  Put it into combobox (if no descriptive name, use path)
            //

            n = SendMessage (hwndTemp, CB_ADDSTRING, 0, (LONG) (LPTSTR) pszValue);

            //
            //  Find the first selection that matches the "default" selection
            //

            if ((selection == -1)  && !lstrcmp (pszLine, szTemp2))
                selection = n;

            //
            //  Also attach pointer to KeyName (i.e. boot path) to each item
            //

            pszTemp = AllocStr (pszLine);

            SendMessage (hwndTemp, CB_SETITEMDATA, n, (LONG)pszTemp);
        }

        // If no selection was found up to this point, choose 0, because
        // that is the default value that loader would choose.

        if (selection == -1)
            selection = 0;

        OddArrowWindow (GetDlgItem (hDlg, IDD_SYS_SECSCROLL));
        SendDlgItemMessage (hDlg, IDD_SYS_SECONDS, EM_LIMITTEXT, 3, 0L);

        //  This call should force correct settings for the checkbox
        //  and "Showlist for xx seconds" controls

        nOriginalSelection = selection;

        SendMessage (hwndTemp, CB_SETCURSEL, selection, 0L);

ContinueSystemInit:

#endif  //  _MIPS_ || _ALPHA_ || _PPC_

        ////////////////////////////////////////////////////////////////////
        // Display System Variables from registry in listbox
        ////////////////////////////////////////////////////////////////////

        //  Get Computer Name from registry and display it in static text

        szTemp[0] = TEXT('\0');
        iTemp = CharSizeOf(szTemp);

        if (GetComputerName (szTemp, &iTemp))
            SetDlgItemText (hDlg, IDD_SYS_COMPUTERNAME, szTemp);
        else
            //  Error trying to read ComputerName from registry
            MyMessageBox (hDlg, SYSTEM+6, INITS+1, MB_ICONEXCLAMATION);

        hwndTemp = GetDlgItem (hDlg, IDD_SYS_LB_SYSVARS);

        //  NOTE: The System applet subclasses the SYSTEM Environment
        //        variable listbox in order to disallow the user from
        //        highlighting any item in the listbox.  This avoids
        //        confusion in the UI that might result from thinking
        //        selecting something in this listbox meant something.

        bEditSystemVars = FALSE;

        lpfnSysLB = NULL;
        hkeyEnv = NULL;

        //  Try to open the System Environment variables area with
        //  Read AND Write permission.  If successful, then we allow
        //  the User to edit them the same as their own variables

        if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                          szSysEnv,
                          0,
                          KEY_READ | KEY_WRITE,
                          &hkeyEnv)
                != ERROR_SUCCESS)
        {
            //  On failure, just try to open it for reading
            if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                              szSysEnv,
                              0,
                              KEY_READ,
                              &hkeyEnv)
                    == ERROR_SUCCESS)
            {
                lpfnSysLB = (WNDPROC) GetWindowLong (hwndTemp, GWL_WNDPROC);
                SetWindowLong (hwndTemp, GWL_WNDPROC, (LONG)SysLBProc);
            }
            else
            {
                hkeyEnv = NULL;
            }
        }
        else
        {
            bEditSystemVars = TRUE;
        }

        if (hkeyEnv)
        {
            pszValue = (TCHAR *) hKey;
            dwBufz = sizeof(szTemp);
            dwValz = ByteCountOf(BUFZ);
            dwIndex = 0;

            //  Read all values until an error is encountered

            while (!RegEnumValue(hkeyEnv,
                                 dwIndex++, // Index'th value name/data
                                 szTemp,    // Ptr to ValueName buffer
                                 &dwBufz,   // Size of ValueName buffer
                                 NULL,      // Title index return
                                 &dwType,   // Type code of entry
                        (LPBYTE) pszValue,  // Ptr to ValueData buffer
                                 &dwValz))  // Size of ValueData buffer
            {
                if ((dwType != REG_SZ) && (dwType != REG_EXPAND_SZ))
                    continue;

                //
                //  Clip length of returned Environment variable string
                //  to MAX_VALUE_LEN-1, as necessary.
                //

                pszValue[MAX_VALUE_LEN-1] = TEXT('\0');

                ExpandEnvironmentStrings (pszValue, pszString, BUFZ);

                penvar = (ENVARS *) AllocMem (sizeof(ENVARS));

                penvar->dwType      = dwType;
                penvar->szValueName = AllocStr (szTemp);
                penvar->szValue     = AllocStr (pszValue);
                penvar->szExpValue  = AllocStr (pszString);

                wsprintf (bBuffer, TEXT("%s = %s"), szTemp, pszString);
                n = SendMessage (hwndTemp, LB_ADDSTRING, 0, (LPARAM)bBuffer);
                SendMessage (hwndTemp, LB_SETITEMDATA, n, (LPARAM)penvar);
                SetLBWidth (hwndTemp, bBuffer, LB_SYSVAR);

                //  Reset vars for next iteration

                dwBufz = sizeof(szTemp);
                dwValz = ByteCountOf(BUFZ);
            }
            RegCloseKey (hkeyEnv);
        }


        ////////////////////////////////////////////////////////////////////
        //  Display USER variables from registry in listbox
        ////////////////////////////////////////////////////////////////////

        // Get CurrrentUser name from registry and display in static text

        n = LoadString (g_hInst, SYSTEM+1, szTemp, CharSizeOf(szTemp));

        bBuffer[0] = TEXT('\0');
        iTemp = CharSizeOf(szTemp2) - n;

        if (!GetUserName (bBuffer, &iTemp))
            //  Error trying to get Current User Name
            MyMessageBox (hDlg, SYSTEM+7, INITS+1, MB_ICONEXCLAMATION);

        wsprintf (szTemp2, szTemp, bBuffer);
        SetDlgItemText (hDlg, IDD_SYS_UVLABEL, szTemp2);

        Error = RegCreateKey (HKEY_CURRENT_USER, szUserEnv, &hkeyEnv);

        if (Error == ERROR_SUCCESS)
        {
            hwndTemp = GetDlgItem (hDlg, IDD_SYS_LB_USERVARS);

            pszValue = (TCHAR *) hKey;
            dwBufz = sizeof(szTemp);
            dwValz = ByteCountOf(BUFZ);
            dwIndex = 0;

            //  Read all values until an error is encountered

            while (!RegEnumValue(hkeyEnv,
                                 dwIndex++, // Index'th value name/data
                                 szTemp,    // Ptr to ValueName buffer
                                 &dwBufz,   // Size of ValueName buffer
                                 NULL,      // Title index return
                                 &dwType,   // Type code of entry
                        (LPBYTE) pszValue,  // Ptr to ValueData buffer
                                 &dwValz))  // Size of ValueData buffer
            {
                if ((dwType != REG_SZ) && (dwType != REG_EXPAND_SZ))
                    continue;

                //
                //  Clip length of returned Environment variable string
                //  to MAX_VALUE_LEN-1, as necessary.
                //

                pszValue[MAX_VALUE_LEN-1] = TEXT('\0');

                ExpandEnvironmentStrings (pszValue, pszString, BUFZ);

                penvar = (ENVARS *) AllocMem (sizeof(ENVARS));

                penvar->dwType      = dwType;
                penvar->szValueName = AllocStr (szTemp);
                penvar->szValue     = AllocStr (pszValue);
                penvar->szExpValue  = AllocStr (pszString);

                wsprintf (bBuffer, TEXT("%s = %s"), szTemp, pszString);
                n = SendMessage (hwndTemp, LB_ADDSTRING, 0, (LPARAM)bBuffer);
                SendMessage (hwndTemp, LB_SETITEMDATA, n, (LPARAM)penvar);
                SetLBWidth (hwndTemp, bBuffer, LB_USERVAR);

                // Reset vars for next call

                dwBufz = sizeof(szTemp);
                dwValz = ByteCountOf(BUFZ);
            }
            RegCloseKey (hkeyEnv);
        }
        else
        {
            //  Report opening USER Environment key
            if (MyMessageBox (hDlg, SYSTEM+8, INITS+1,
                              MB_OKCANCEL | MB_ICONEXCLAMATION) == IDCANCEL)
            {
                //  Free allocated memory since we are returning from here
                FreeMem ((LPVOID)hKey, BUFZ*sizeof(TCHAR));
                FreeMem (bBuffer, BUFZ*sizeof(TCHAR));
                FreeMem (pszString, BUFZ*sizeof(TCHAR));

                EndDialog (hDlg, 0);
                HourGlass (FALSE);
                return FALSE;
            }
        }

        // EM_LIMITTEXT of VARIABLE and VALUE editbox

        SendDlgItemMessage (hDlg, IDD_SYS_VAR, EM_LIMITTEXT, MAX_PATH-1, 0L);
        SendDlgItemMessage (hDlg, IDD_SYS_VALUE, EM_LIMITTEXT, MAX_VALUE_LEN-1, 0L);

        EnableWindow (GetDlgItem (hDlg, IDD_SYS_SETUV), FALSE);
        EnableWindow (GetDlgItem (hDlg, IDD_SYS_DELUV), FALSE);

        FreeMem ((LPVOID)hKey, BUFZ*sizeof(TCHAR));
        FreeMem (bBuffer, BUFZ*sizeof(TCHAR));
        FreeMem (pszString, BUFZ*sizeof(TCHAR));

        dwRestartSystem = RET_NO_CHANGE;

        HourGlass (FALSE);
        break;

    case WM_VSCROLL:
        nCtlId = HIWORD(wParam) - (IDD_SYS_SECSCROLL - IDD_SYS_SECONDS);

        if (LOWORD(wParam) == SB_ENDSCROLL)
        {
            SendDlgItemMessage (hDlg, nCtlId, EM_SETSEL, 0, 32767);
            break;
        }

        if (nCtlId == IDD_SYS_SECONDS)
            lpAVS = (LPARROWVSCROLL) &avsForSeconds;
        else
            return FALSE;

        nOldVal = nVal = GetDlgItemInt (hDlg, nCtlId, &bOK, FALSE);

        if (!bOK && (( nVal < lpAVS->bottom) || (nVal > lpAVS->top)))
            nVal = (int) lpAVS->thumbpos;
        else
            nVal = (int) ArrowVScrollProc (LOWORD(wParam), (short) nVal, lpAVS);

        if ((nOldVal != nVal) || !bOK)
        {
            SetDlgItemInt (hDlg, nCtlId, nVal, FALSE);
        }
        SetFocus (GetDlgItem (hDlg, nCtlId));
        break;


    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDD_SYS_VMEM:
            {
                DWORD dwSave;
                DWORD dw;

                dwSave = g_dwContext;
                g_dwContext = IDH_DLG_VIRTUALMEM;

                //  Virtual memory dialog box changes registry values on IDOK

                dw = DialogBox (g_hInst, (LPTSTR) MAKEINTRESOURCE(DLG_VIRTUALMEM), hDlg,
                               (DLGPROC) VirtualMemDlg);
                if (dw != RET_NO_CHANGE)
                {
                     SetDlgItemText( hDlg, IDCANCEL, g_szClose );
                     dwRestartSystem |= dw;
                }
                g_dwContext = dwSave;
            }
            break;

        case IDD_SYS_COREDUMP:
            {
                DWORD dwSave;
                DWORD dw;

                dwSave = g_dwContext;
                g_dwContext = IDH_DLG_COREDUMP;

                //  Recovery dialog box changes registry values on IDOK

                dw = DialogBox( g_hInst, (LPTSTR) MAKEINTRESOURCE( DLG_COREDUMP ),
                                hDlg, (DLGPROC) CoreDumpDlg);
                if (dw != RET_NO_CHANGE)
                {
                     SetDlgItemText( hDlg, IDCANCEL, g_szClose );
                     dwRestartSystem |= dw;
                }
                g_dwContext = dwSave;
            }
            break;

        case IDD_SYS_TASKING:
          {
            DWORD dwSave;

            dwSave = g_dwContext;
            g_dwContext = IDH_DLG_TASKING;

            //  Tasking memory dialog box changes registry values on IDOK

            i = DialogBox (g_hInst, (LPTSTR) MAKEINTRESOURCE(DLG_TASKING), hDlg,
                           (DLGPROC) TaskingDlg);
            if (i)
                SetDlgItemText( hDlg, IDCANCEL, g_szClose );
            g_dwContext = dwSave;
            break;
          }

          //------------------------------------------------------------
          // UNDER CONSTRUCTION - paulat
          // Next case statement added to support hardware profiles
          //------------------------------------------------------------
          case IDD_SYS_HWPROFILES:
            {
              DWORD dwSave;

              dwSave = g_dwContext;
              g_dwContext = IDH_DLG_TASKING;  // BUGBUG: add help topic for hwprof

              //  Hardware Profiles dialog box changes registry values on IDOK

              i = DialogBox (g_hInst, (LPTSTR) MAKEINTRESOURCE(DLG_HWPROFILES), hDlg,
                              (DLGPROC) HardwareProfilesDlg);
              if (i)
                  SetDlgItemText (hDlg, IDCANCEL, g_szClose);
              g_dwContext = dwSave;
              break;
            }

        case IDD_SYS_VALUE:
        case IDD_SYS_VAR:
            //  IF focus is being set to one of these controls, enable
            //  new buttons as appropriate
            //  ELSE allow "Enter" key to simply choose the IDOK button

            //  If the USER activates or clicks in either Variable or Value
            //  editbox, then change "Set" to the DefPushbutton


            if (HIWORD(wParam) == EN_SETFOCUS)
            {
                if (GetDlgItemText (hDlg, IDD_SYS_VAR, szTemp2, CharSizeOf(szTemp2)))
                {
                    EnableWindow (GetDlgItem (hDlg, IDD_SYS_SETUV), TRUE);
                    EnableWindow (GetDlgItem (hDlg, IDD_SYS_DELUV), TRUE);
                    SetDefButton (hDlg, IDD_SYS_SETUV);
                }
                else
                {
                    EnableWindow (GetDlgItem (hDlg, IDD_SYS_SETUV), FALSE);
                    EnableWindow (GetDlgItem (hDlg, IDD_SYS_DELUV), FALSE);
                    SetDefButton (hDlg, IDOK);
                }
            }
            else if (HIWORD(wParam) == EN_KILLFOCUS)
            {
                SetDefButton (hDlg, IDOK);
            }
            break;


        case IDD_SYS_LB_SYSVARS:
            if (!((HIWORD(wParam) == LBN_SELCHANGE) ||
                  (HIWORD(wParam) == LBN_SETFOCUS)))
            {
                return FALSE;
            }
            else
            {
                if (!bEditSystemVars)
                    return FALSE;

                bUserVars = FALSE;
                goto ChangeVariables;
            }


        case IDD_SYS_LB_USERVARS:
            if (!((HIWORD(wParam) == LBN_SELCHANGE) ||
                  (HIWORD(wParam) == LBN_SETFOCUS)))
                return FALSE;

            bUserVars = TRUE;

            //  Else fall thru....

ChangeVariables:

            hwndTemp = GetDlgItem (hDlg, LOWORD(wParam));

            if (HIWORD(wParam) == LBN_SETFOCUS)
            {
                //
                //  Clear the selection from the other listbox
                //  so the user doens't have to figure out which
                //  one he is editing
                //

                SendDlgItemMessage (hDlg,
                                    (LOWORD(wParam) == IDD_SYS_LB_USERVARS) ?
                                    IDD_SYS_LB_SYSVARS : IDD_SYS_LB_USERVARS,
                                    LB_SETCURSEL, (WPARAM) -1, 0);
            }

            selection = SendMessage (hwndTemp, LB_GETCURSEL, 0, 0L);

            if (selection != LB_ERR)
            {
                penvar = (ENVARS *)SendMessage (hwndTemp, LB_GETITEMDATA,
                                                            selection, 0L);
                SetDlgItemText (hDlg, IDD_SYS_VAR, penvar->szValueName);
                SetDlgItemText (hDlg, IDD_SYS_VALUE, penvar->szValue);

                //  Enable DELETE button
                EnableWindow (GetDlgItem (hDlg, IDD_SYS_DELUV), TRUE);
            }
            else
            {
                //  Else  we are only deselecting an item so...
                //  simply remove all text from Editboxes
                SetDlgItemText (hDlg, IDD_SYS_VAR, g_szNull);
                SetDlgItemText (hDlg, IDD_SYS_VALUE, g_szNull);

                //  Disable buttons
                EnableWindow (GetDlgItem (hDlg, IDD_SYS_DELUV), FALSE);
                EnableWindow (GetDlgItem (hDlg, IDD_SYS_SETUV), FALSE);
            }
            break;

        case IDD_SYS_DELUV:
            // Delete listbox entry that matches value in IDD_SYS_VAR
            //  If found, delete entry else ignore

            GetDlgItemText (hDlg, IDD_SYS_VAR, szTemp2, CharSizeOf(szTemp2));

            if (szTemp2[0] == TEXT('\0'))
                break;

            //  Determine which Listbox is active (SYSTEM or USER vars)

            hwndTemp = GetDlgItem (hDlg, bUserVars ? IDD_SYS_LB_USERVARS :
                                                     IDD_SYS_LB_SYSVARS);

            n = FindVar (hwndTemp, szTemp2);

            if (n != -1)
            {
                // Free existing strings (listbox and ours)
                penvar = (ENVARS *) SendMessage (hwndTemp, LB_GETITEMDATA, n, 0L);

                FreeStr (penvar->szValueName);
                FreeStr (penvar->szValue);
                FreeStr (penvar->szExpValue);
                FreeMem ((LPVOID) penvar, sizeof(ENVARS));
                SendMessage (hwndTemp, LB_DELETESTRING, n, 0L);

                //  Remove text from Editboxes
                SetDlgItemText (hDlg, IDD_SYS_VAR, g_szNull);
                SetDlgItemText (hDlg, IDD_SYS_VALUE, g_szNull);

                //  Disable useless controls
                EnableWindow (GetDlgItem (hDlg, IDD_SYS_SETUV), FALSE);
                EnableWindow (GetDlgItem (hDlg, IDD_SYS_DELUV), FALSE);

                //  Reset OK as "DefPushbutton" and re-enable keybd input
                SetDefButton (hDlg, IDOK);
                SetFocus (GetDlgItem (hDlg, IDOK));
            }
#ifdef LATER
            //  On a change determine which listboxes to update
            if (bUserVars)
            {
                UpdateEnvStrings (hDlg, IDD_SYS_LB_USERVARS);
            }
            else
            {
                UpdateEnvStrings (hDlg, IDD_SYS_LB_USERVARS);
                UpdateEnvStrings (hDlg, IDD_SYS_LB_SYSVARS);
            }

#endif  //  LATER
            break;


        case IDD_SYS_SETUV:
//SetEnvValues:
            //  Set the Environment variable in IDD_SYS_VAR
            //  Also add or change the registry entry

            GetDlgItemText (hDlg, IDD_SYS_VAR, szTemp2, CharSizeOf(szTemp2));

            //  Strip trailing whitespace from end of Env Variable

            i = lstrlen(szTemp2) - 1;

            while (i >= 0)
            {
                if (_istspace(szTemp2[i]))
                    szTemp2[i--] = TEXT('\0');
                else
                    break;
            }

            if (szTemp2[0] == TEXT('\0'))
                break;

            bBuffer = (TCHAR *) AllocMem (ByteCountOf(BUFZ));
            pszString = (LPTSTR) AllocMem (ByteCountOf(BUFZ));

            GetDlgItemText (hDlg, IDD_SYS_VALUE, bBuffer, BUFZ);

            //  Determine which Listbox is active (SYSTEM or USER vars)

            hwndTemp = GetDlgItem (hDlg, bUserVars ? IDD_SYS_LB_USERVARS :
                                                     IDD_SYS_LB_SYSVARS);

            n = FindVar (hwndTemp, szTemp2);

            if (n != -1)
            {
                // Free existing strings (listbox and ours)
                penvar = (ENVARS *) SendMessage (hwndTemp, LB_GETITEMDATA, n, 0L);
                SendMessage (hwndTemp, LB_DELETESTRING, n, 0L);

                FreeStr (penvar->szValueName);
                FreeStr (penvar->szValue);
                FreeStr (penvar->szExpValue);
            }
            else
            {
                //  Get some storage for new Env Var
                penvar = (ENVARS *) AllocMem (sizeof(ENVARS));
            }

            //  If there are two '%' chars in string, then this is a
            //  REG_EXPAND_SZ style environment string

            pszTemp = _tcspbrk (bBuffer, TEXT("%"));

            if (pszTemp && _tcspbrk (pszTemp, TEXT("%")))
                penvar->dwType = REG_EXPAND_SZ;
            else
                penvar->dwType = REG_SZ;

            ExpandEnvironmentStrings (bBuffer, pszString, BUFZ);

            penvar->szValueName = AllocStr (szTemp2);
            penvar->szValue     = AllocStr (bBuffer);
            penvar->szExpValue  = AllocStr (pszString);


            wsprintf (bBuffer, TEXT("%s = %s"), penvar->szValueName, pszString);
            n = SendMessage (hwndTemp, LB_ADDSTRING, n, (LPARAM)bBuffer);
            SendMessage (hwndTemp, LB_SETITEMDATA, n, (LPARAM)penvar);

            SetLBWidth (hwndTemp, bBuffer, bUserVars ? LB_USERVAR : LB_SYSVAR);

            //  Remove text from Editboxes after add
            SetDlgItemText (hDlg, IDD_SYS_VAR, g_szNull);
            SetDlgItemText (hDlg, IDD_SYS_VALUE, g_szNull);

            FreeMem (bBuffer, ByteCountOf(BUFZ));
            FreeMem (pszString, ByteCountOf(BUFZ));

            // Set user input back to VARIABLE field
            SetFocus (GetDlgItem (hDlg, IDD_SYS_VAR));

#ifdef LATER
            //  On a change determine which listboxes to update
            if (bUserVars)
            {
                UpdateEnvStrings (hDlg, IDD_SYS_LB_USERVARS);
            }
            else
            {
                UpdateEnvStrings (hDlg, IDD_SYS_LB_USERVARS);
                UpdateEnvStrings (hDlg, IDD_SYS_LB_SYSVARS);
            }

#endif  //  LATER

            break;

#if defined(_MIPS_) || defined(_ALPHA_)  || defined(_PPC_)
        case IDD_SYS_ENABLECOUNTDOWN:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                BOOL bChecked;

                CheckDlgButton (hDlg, IDD_SYS_ENABLECOUNTDOWN,
                                 bChecked = (WORD) !IsDlgButtonChecked (hDlg, IDD_SYS_ENABLECOUNTDOWN));
                EnableWindow (GetDlgItem (hDlg, IDD_SYS_SECONDS), bChecked);
                EnableWindow (GetDlgItem (hDlg, IDD_SYS_SECSCROLL), bChecked);
            }
            break;
#endif

        case IDOK:
            HourGlass (TRUE);

            /////////////////////////////////////////////////////////////////
            //  Write new info to boot.ini file
            /////////////////////////////////////////////////////////////////

            hwndTemp = GetDlgItem (hDlg, IDD_SYS_OS);

            selection = SendMessage (hwndTemp, CB_GETCURSEL, 0, 0L);

            if ((selection == CB_ERR) || (selection == CB_ERRSPACE))
                selection = nOriginalSelection;

            timeout   = GetDlgItemInt (hDlg, IDD_SYS_SECONDS, NULL, FALSE);

#if defined(_MIPS_) || defined(_ALPHA_)  || defined(_PPC_)
            if (fCanUpdateNVRAM)
                UpdateNVRAM (hDlg, selection, timeout);
#else  //  _MIPS_ || _ALPHA_ || _PPC_
            if ((selection != nOriginalSelection) || (timeout != nOriginalTimeout))
            {
                bOK = TRUE;

                //  Change Read-only file attrs on Boot.ini file if necessary
                if ((dwFileAttr = GetFileAttributes (szBootIni)) != 0xFFFFFFFF)
                    if (dwFileAttr & FILE_ATTRIBUTE_READONLY)
                        if (!SetFileAttributes (szBootIni,
                                   dwFileAttr & ~FILE_ATTRIBUTE_READONLY))
                        {
BootIniWriteError:
                            bOK = FALSE;
                            MyMessageBox (hDlg, SYSTEM+19, INITS+1,
                                          MB_OK | MB_ICONINFORMATION);
                        }

                if (bOK)
                {
                    //
                    //  Write new [operating systems] section and
                    //  set "default" selection in boot.ini file.
                    //

                    if (selection != nOriginalSelection)
                    {
                        //
                        //  Allocate buffers for new section
                        //

                        hKey = AllocMem (BUFZ*sizeof(TCHAR));

                        pszKeyName = (TCHAR *) hKey;

                        //
                        //  Total profile section buffer size
                        //

                        i = dwBufz = 0;

                        pszSectionA = (LPSTR) AllocMem (BUFZ);

                        //
                        //  Get the User's selection and write it in the
                        //  section buffer first.  Then get all other items.
                        //

                        pszTemp = (LPTSTR) SendMessage (hwndTemp,
                                                        CB_GETITEMDATA,
                                                        selection, 0L);

                        SendMessage (hwndTemp, CB_GETLBTEXT, selection,
                                      (LONG) (LPTSTR) szTemp);

                        if (pszTemp != (LPTSTR) CB_ERR)
                        {
                            lstrcpy (pszKeyName, pszTemp);
                            lstrcat (pszKeyName, TEXT("="));
                            lstrcat (pszKeyName, szTemp);

                            i = lstrlen (pszKeyName) + 1;
                            pszKeyName += i;

                            //
                            //  Set "default" selection in boot.ini file
                            //

                            if (!WritePrivateProfileString (pszBoot,
                                                            szDefault,
                                                            pszTemp,
                                                            szBootIni))
                            {
                               goto BootIniWriteError;
                            }

                            FreeStr (pszTemp);
                        }

                        dwBufz = (DWORD) i;

                        //
                        //  Get the rest of the selections
                        //

                        n = SendMessage (hwndTemp, CB_GETCOUNT, 0, 0L);

                        if (n != LB_ERR)
                        {
                            for (iTemp = 0; iTemp < n; iTemp++)
                            {
                                //
                                //  Skip the User's selection since we got it
                                //  above.
                                //

                                if (iTemp == selection)
                                    continue;

                                pszTemp = (LPTSTR) SendMessage (hwndTemp,
                                                                CB_GETITEMDATA,
                                                                iTemp, 0L);

                                SendMessage (hwndTemp, CB_GETLBTEXT, iTemp,
                                             (LONG) (LPTSTR) szTemp);

                                if (pszTemp != (LPTSTR) CB_ERR)
                                {
                                    lstrcpy (pszKeyName, pszTemp);
                                    lstrcat (pszKeyName, TEXT("="));
                                    lstrcat (pszKeyName, szTemp);

                                    FreeStr (pszTemp);

                                    i = lstrlen (pszKeyName) + 1;
                                    pszKeyName += i;

                                    dwBufz += (DWORD) i;
                                }
                            }
                        }

                        //
                        //  Double-Null terminate the buffer
                        //

                        *pszKeyName = TEXT('\0');
                        dwBufz++;

                        pszKeyName = (TCHAR *) hKey;

                        //
                        //  Convert entire buffer from UNICODE to OEM
                        //

                        WideCharToMultiByte (CP_OEMCP, WC_COMPOSITECHECK,
                                             pszKeyName, dwBufz,
                                             pszSectionA, BUFZ,
                                             NULL, NULL);

                        //
                        //  Write new section under OS
                        //

                        if (!WritePrivateProfileSectionA (szOSA,
                                                          pszSectionA,
                                                          szBootIniA))
                        {
                            FreeMem ((LPVOID)pszSectionA, BUFZ);
                            FreeMem ((LPVOID)hKey, BUFZ*sizeof(TCHAR));
                            goto BootIniWriteError;
                        }

                        FreeMem ((LPVOID)pszSectionA, BUFZ);
                        FreeMem ((LPVOID)hKey, BUFZ*sizeof(TCHAR));
                    }

                    if (timeout != nOriginalTimeout)
                    {
                        GetDlgItemText (hDlg, IDD_SYS_SECONDS, szTemp2, sizeof(szTemp2));

                        if (!CheckVal (hDlg, IDD_SYS_SECONDS, FORMIN, FORMAX, SYSTEM+4))
                            break;

                        //  Write timeout value to file

                        if (!WritePrivateProfileString (pszBoot, szTimeout,
                                                       szTemp2, szBootIni))
                            goto BootIniWriteError;
                    }

                    //  Restore read-only attr, if necessary, after writes
                    if (dwFileAttr != 0xFFFFFFFF)
                        if (dwFileAttr & FILE_ATTRIBUTE_READONLY)
                            SetFileAttributes (szBootIni, dwFileAttr);
                }
            }

#endif  //  _MIPS_ || _ALPHA_ || _PPC_

            /////////////////////////////////////////////////////////////////
            //  Set all new USER environment variables to current values
            //  but delete all old environment variables first
            /////////////////////////////////////////////////////////////////

            if (RegOpenKeyEx (HKEY_CURRENT_USER, szUserEnv, 0,
                             KEY_READ | KEY_WRITE, &hkeyEnv)
                    == ERROR_SUCCESS)
            {
                dwBufz = sizeof(szTemp);
                dwIndex = 0;

                //  Delete all values of type REG_SZ & REG_EXPAND_SZ under key

                //  First: Make a linked list of all USER Env string vars

                prvFirst = (REGVAL *) NULL;

                while (!RegEnumValue(hkeyEnv,
                                     dwIndex++, // Index'th value name/data
                                     szTemp,    // Ptr to ValueName buffer
                                     &dwBufz,   // Size of ValueName buffer
                                     NULL,      // Title index return
                                     &dwType,   // Type code of entry
                                     NULL,      // Ptr to ValueData buffer
                                     NULL))     // Size of ValueData buffer
                {
                    if ((dwType != REG_SZ) && (dwType != REG_EXPAND_SZ))
                        continue;

                    if (prvFirst)
                    {
                        prvRegVal->prvNext = (REGVAL *) AllocMem (sizeof(REGVAL));
                        prvRegVal = prvRegVal->prvNext;
                    }
                    else        // First time thru
                    {
                        prvFirst  =
                        prvRegVal = (REGVAL *) AllocMem (sizeof(REGVAL));
                    }

                    prvRegVal->prvNext = NULL;
                    prvRegVal->szValueName = AllocStr (szTemp);

                    // Reset vars for next call

                    dwBufz = sizeof(szTemp);
                }

                //  Now traverse the list, deleting them all

                prvRegVal = prvFirst;

                while (prvRegVal)
                {
                    RegDeleteValue (hkeyEnv, prvRegVal->szValueName);

                    FreeStr (prvRegVal->szValueName);

                    prvFirst  = prvRegVal;
                    prvRegVal = prvRegVal->prvNext;

                    FreeMem ((LPVOID) prvFirst, sizeof(REGVAL));
                }

                ///////////////////////////////////////////////////////////////
                //  Set all new USER environment variables to current values
                ///////////////////////////////////////////////////////////////

                hwndTemp = GetDlgItem (hDlg, IDD_SYS_LB_USERVARS);

                if ((n = SendMessage (hwndTemp, LB_GETCOUNT, 0, 0L)) != LB_ERR)
                {
                    for (i = 0; i < n; i++)
                    {
                        penvar = (ENVARS *) SendMessage (hwndTemp,
                                                         LB_GETITEMDATA,
                                                         i, 0L);
                        if (RegSetValueEx (hkeyEnv,
                                           penvar->szValueName,
                                           0L,
                                           penvar->dwType,
                                  (LPBYTE) penvar->szValue,
                                           ByteCountOf(lstrlen (penvar->szValue)+1)))
                        {
                            //  Report error trying to set registry values

                            if (MyMessageBox (hDlg, SYSTEM+9, INITS+1,
                                MB_OKCANCEL | MB_ICONEXCLAMATION) == IDCANCEL)
                                break;
                        }
                    }
                }

                RegFlushKey (hkeyEnv);
                RegCloseKey (hkeyEnv);
            }
            else
            {
                //  Report opening USER Environment key
                if (MyMessageBox (hDlg, SYSTEM+8, INITS+1,
                               MB_OKCANCEL | MB_ICONEXCLAMATION) == IDCANCEL)
                    goto SystemFreeAndExit;
            }

            /////////////////////////////////////////////////////////////////
            //  Set all new SYSTEM environment variables to current values
            //  but delete all old environment variables first
            /////////////////////////////////////////////////////////////////

            if (!bEditSystemVars)
                goto SkipSystemVars;

            if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                               szSysEnv,
                               0,
                               KEY_READ | KEY_WRITE,
                               &hkeyEnv)
                    == ERROR_SUCCESS)
            {
                dwBufz = sizeof(szTemp);
                dwIndex = 0;

                //  Delete all values of type REG_SZ & REG_EXPAND_SZ under key

                //  First: Make a linked list of all Env string vars

                prvFirst = (REGVAL *) NULL;

                while (!RegEnumValue(hkeyEnv,
                                     dwIndex++, // Index'th value name/data
                                     szTemp,    // Ptr to ValueName buffer
                                     &dwBufz,   // Size of ValueName buffer
                                     NULL,      // Title index return
                                     &dwType,   // Type code of entry
                                     NULL,      // Ptr to ValueData buffer
                                     NULL))     // Size of ValueData buffer
                {
                    if ((dwType != REG_SZ) && (dwType != REG_EXPAND_SZ))
                        continue;

                    if (prvFirst)
                    {
                        prvRegVal->prvNext = (REGVAL *) AllocMem (sizeof(REGVAL));
                        prvRegVal = prvRegVal->prvNext;
                    }
                    else        // First time thru
                    {
                        prvFirst  =
                        prvRegVal = (REGVAL *) AllocMem (sizeof(REGVAL));
                    }

                    prvRegVal->prvNext = NULL;
                    prvRegVal->szValueName = AllocStr (szTemp);

                    // Reset vars for next call

                    dwBufz = sizeof(szTemp);
                }

                //  Now traverse the list, deleting them all

                prvRegVal = prvFirst;

                while (prvRegVal)
                {
                    RegDeleteValue (hkeyEnv, prvRegVal->szValueName);

                    FreeStr (prvRegVal->szValueName);

                    prvFirst  = prvRegVal;
                    prvRegVal = prvRegVal->prvNext;

                    FreeMem ((LPVOID) prvFirst, sizeof(REGVAL));
                }

                ///////////////////////////////////////////////////////////////
                //  Set all new SYSTEM environment variables to current values
                ///////////////////////////////////////////////////////////////

                hwndTemp = GetDlgItem (hDlg, IDD_SYS_LB_SYSVARS);

                if ((n = SendMessage (hwndTemp, LB_GETCOUNT, 0, 0L)) != LB_ERR)
                {
                    for (i = 0; i < n; i++)
                    {
                        penvar = (ENVARS *) SendMessage (hwndTemp,
                                                         LB_GETITEMDATA,
                                                         i, 0L);
                        if (RegSetValueEx (hkeyEnv,
                                           penvar->szValueName,
                                           0L,
                                           penvar->dwType,
                                  (LPBYTE) penvar->szValue,
                                           ByteCountOf(lstrlen (penvar->szValue)+1)))
                        {
                            //  Report error trying to set registry values

                            if (MyMessageBox (hDlg, SYSTEM+9, INITS+1,
                                MB_OKCANCEL | MB_ICONEXCLAMATION) == IDCANCEL)
                                break;
                        }
                    }
                }

                RegFlushKey (hkeyEnv);
                RegCloseKey (hkeyEnv);
            }
            else
            {
                //  Report opening SYSTEM Environment key
                if (MyMessageBox (hDlg, SYSTEM+21, INITS+1,
                               MB_OKCANCEL | MB_ICONEXCLAMATION) == IDCANCEL)
                    goto SystemFreeAndExit;
            }

SkipSystemVars:
            // Send public message announcing change to Environment
            SendWinIniChange (szEnvironment);

            /* fall through.... */

        case IDCANCEL:
SystemFreeAndExit:
            HourGlass (TRUE);

            /////////////////////////////////////////////////////////////////
            //  Free memory alloc'd for list and combo boxes
            /////////////////////////////////////////////////////////////////

            hwndTemp = GetDlgItem (hDlg, IDD_SYS_OS);
            n = SendMessage (hwndTemp, CB_GETCOUNT, 0, 0L);

            if (n != LB_ERR)
            {
                for (i = 0; i < n; i++)
                {
                    iTemp = SendMessage (hwndTemp, CB_GETITEMDATA, i, 0L);

                    if ((LPTSTR) iTemp != NULL)
                        FreeStr ((LPTSTR) iTemp);
                }
            }

            //  Free alloc'd strings and memory for UserEnvVars list box items

            hwndTemp = GetDlgItem (hDlg, IDD_SYS_LB_USERVARS);
            n = SendMessage (hwndTemp, LB_GETCOUNT, 0, 0L);

            if (n != LB_ERR)
            {
                for (i = 0; i < n; i++)
                {
                    penvar = (ENVARS *) SendMessage (hwndTemp, LB_GETITEMDATA,
                                                        i, 0L);
                    if (penvar)
                    {
                        FreeStr (penvar->szValueName);
                        FreeStr (penvar->szValue);
                        FreeStr (penvar->szExpValue);
                        FreeMem ((LPVOID) penvar, sizeof(ENVARS));
                    }
                }
            }

            //  Free alloc'd strings and memory for SysEnvVars list box items

            hwndTemp = GetDlgItem (hDlg, IDD_SYS_LB_SYSVARS);
            n = SendMessage (hwndTemp, LB_GETCOUNT, 0, 0L);

            if (n != LB_ERR)
            {
                for (i = 0; i < n; i++)
                {
                    penvar = (ENVARS *) SendMessage (hwndTemp, LB_GETITEMDATA,
                                                        i, 0L);
                    if (penvar)
                    {
                        FreeStr (penvar->szValueName);
                        FreeStr (penvar->szValue);
                        FreeStr (penvar->szExpValue);
                        FreeMem ((LPVOID) penvar, sizeof(ENVARS));
                    }
                }
            }

            if (hfont);
                DeleteObject (hfont);

            if (hfontBold);
                DeleteObject (hfontBold);

            if (dwRestartSystem & ~RET_CHANGE_NO_REBOOT)
            {
                DWORD dwMsg;

                //
                // Prompt for reboot if something changed in Virtual
                // Memory settings
                //
                switch(dwRestartSystem & ~RET_CHANGE_NO_REBOOT) {
                case RET_VIRTUAL_CHANGE:
                    dwMsg = MAKELONG(IDS_VIRTUALMEMCHANGE, 0);
                    break;

                case RET_RECOVER_CHANGE:
                    dwMsg = MAKELONG(IDS_RECOVERDLGCHANGE, 0);
                    break;

                case RET_VIRT_AND_RECOVER:
                    dwMsg = MAKELONG(IDS_VIRTANDRECCHANGE, 0);
                    break;
                }

                DialogBoxParam(g_hInst, (LPTSTR) MAKEINTRESOURCE(DLG_RESTART),
                    hDlg, (DLGPROC)RestartDlg, dwMsg);
            }
            EndDialog (hDlg, 0L);
            HourGlass (FALSE);
            break;

        case IDD_HELP:
            goto DoHelp;

        default:
            break;
        }
        break;

    default:
        if (message == g_wHelpMessage)
        {
DoHelp:
            SysHelp(hDlg);
        }
        else
            return FALSE;
        break;
    }

    return(TRUE);
}


////////////////////////////////////////////////////////////////////////////
//  FindVar
//
//  Find the USER Environment variable that matches passed string
//  and return its listbox index or -1
//
////////////////////////////////////////////////////////////////////////////

int FindVar (HWND hwndLB, LPTSTR szVar)
{
    int     sel, control;
    ENVARS *penvar;

    //  Try for a quick out, check szVar against itemdata->szValueName
    //  of current selection

    if ((sel = SendMessage (hwndLB, LB_GETCURSEL, 0, 0L)) != LB_ERR)
    {
        penvar = (ENVARS *) SendMessage (hwndLB, LB_GETITEMDATA, sel, 0L);

        if (!lstrcmpi (penvar->szValueName, szVar))
            return (sel);
    }

    //  Only check those items which at least start with the same chars
    //  and start search at beginning of listbox

    control = sel = -1;

    while ((sel = SendMessage (hwndLB, LB_FINDSTRING, sel, (LONG)szVar)) != LB_ERR)
    {
        //  If we make it into the loop, then there is at least one
        //  possible match.  Save that in 'control' var.  If sel ever
        //  equals the first match again, then we have tried all
        //  potential listbox entries and none of them match; therefore
        //  there is no match and we just exit.

        if (control == sel)
        {
            sel = -1;
            break;
        }
        else if (control == -1)
            control = sel;

        penvar = (ENVARS *) SendMessage (hwndLB, LB_GETITEMDATA, sel, 0L);

        if (!lstrcmpi (penvar->szValueName, szVar))
            break;
    }

    return sel;
}


////////////////////////////////////////////////////////////////////////////
//  SetLBWidth
//
//  Set the width of the listbox, in pixels, acording to the size of the
//  string passed and based on which LB (USER VARS or SYS VARS).
//
//  History:
//  05-Mar-1993 JonPa   changed to call SetGenLBWidth
////////////////////////////////////////////////////////////////////////////

void SetLBWidth (HWND hwndLB, LPTSTR szBuffer, DWORD dwListBox)
{

    SetGenLBWidth (hwndLB, szBuffer,
                   (dwListBox == LB_SYSVAR) ? &dwSysWidth : &dwUserWidth,
                   hfont, ARBITRARY_USER_LB_EXTENT_ADDITION);
}


////////////////////////////////////////////////////////////////////////////
//  SetGenLBWidth
//
//  Set the width of the listbox, in pixels, acording to the size of the
//  string passed.
//
//  History:
//  05-Mar-1993 JonPa   Created from SetLBWidth
////////////////////////////////////////////////////////////////////////////

void SetGenLBWidth (HWND hwndLB, LPTSTR szBuffer, LPDWORD pdwWidth,
                    HANDLE hfontNew, DWORD cxExtra)
{
    HFONT   hFont;
    HDC     hDC;
    SIZE    Size;

    hDC = GetDC(NULL);
    hFont = SelectObject(hDC, hfontNew);

    // Set scroll width of listbox

    GetTextExtentPoint(hDC, szBuffer, lstrlen(szBuffer), &Size);

    Size.cx += cxExtra;

    // Get the name length and adjust the longest name

    if ((DWORD) Size.cx > *pdwWidth)
    {
        *pdwWidth = Size.cx;
        SendMessage (hwndLB, LB_SETHORIZONTALEXTENT, (DWORD)Size.cx, 0L);
    }

    SelectObject(hDC, hFont);
    ReleaseDC(NULL, hDC);

}


////////////////////////////////////////////////////////////////////////////
//
//  SysLBProc
//
//  Subclassing Window Procedure for the System Environment Variables
//  display listbox.
//
//  I am using this WndProc to subclass a ListBox window procedure.  The
//  main thing I am doing is simply disallowing any MOUSE messages that
//  would seem to indicate that a User had selected an item.  By capturing
//  those messages here, and doing nothing with them, I keep them from
//  reaching the true ListBox WndProc.
//
//
//
//  Returns: TRUE or FALSE
//
////////////////////////////////////////////////////////////////////////////

BOOL APIENTRY SysLBProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
       case WM_MOUSEMOVE:
       case WM_LBUTTONDOWN:
       case WM_LBUTTONUP:
       case WM_LBUTTONDBLCLK:
           break;

       default:
           return (CallWindowProc (lpfnSysLB, hWnd, message, wParam, lParam));
   }
}


