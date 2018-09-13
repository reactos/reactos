/** FILE: ports.c ********** Module Header ********************************
 *
 *  Control panel applet for configuring COM ports.  This file contains
 *  the dialog and utility functions for managing the UI for setting COM
 *  port parameters
 *
 * History:
 *  12:30 on Tues  23 Apr 1991  -by-  Steve Cathcart   [stevecat]
 *        Took base code from Win 3.1 source
 *  10:30 on Tues  04 Feb 1992  -by-  Steve Cathcart   [stevecat]
 *        Updated code to latest Win 3.1 sources
 *  16:30 on Fri   27 Mar 1992  -by-  Steve Cathcart   [stevecat]
 *        Changed to allow for unlimited number of NT COM ports
 *  18:00 on Tue   06 Apr 1993  -by-  Steve Cathcart   [stevecat]
 *        Updated to work seamlessly with NT serial driver
 *  19:00 on Wed   05 Jan 1994  -by-  Steve Cathcart   [stevecat]
 *        Allow setting COM1 - COM4 advanced parameters   
 *
 *  Copyright (C) 1990-1994 Microsoft Corporation
 *
 *************************************************************************/
//==========================================================================
//                                Include files
//==========================================================================
// C Runtime
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Application specific
#include "main.h"

//==========================================================================
//                            Local Definitions
//==========================================================================
#define PORTS        4
#define MAXPORTS    32
#define KEYBZ       4096

#define DEF_BAUD   3 /* 1200 */
#define DEF_WORD   4 /* 8 bits */
#define DEF_PARITY 2 /* None */
#define DEF_STOP   0 /* 1 */
#define DEF_PORT   0 /* Null Port */
#define DEF_SHAKE  2 /* None */
#define PAR_EVEN   0
#define PAR_ODD    1
#define PAR_NONE   2
#define PAR_MARK   3
#define PAR_SPACE  4
#define STOP_1     0
#define STOP_15    1
#define STOP_2     2
#define FLOW_XON   0
#define FLOW_HARD  1
#define FLOW_NONE  2

#define CURRENT_SET 1       // TRUE if Registry key CurrentControlSet works

#define MAX_COM_PORT  256   // Maximum number of COM ports NT supports
#define MIN_COM         1   // Minimum new COM port number
#define MIN_SERIAL  10000   // Minimum new registry "Serial" value


//==========================================================================
//                            External Declarations
//==========================================================================


//==========================================================================
//                            Local Data Declarations
//==========================================================================

#ifdef  LATER
HICON  hNinePinIcon;
HICON  hiconPorts;

HWND   hPortDlg;
HFONT  hIconFont;
WORD   fSetPorts;
LPRECT lpCRects[PORTS];
RECT   CaptureRect;
RECT   rCom1, rCom2, rCom3, rCom4;
HWND   hCom1, hCom2, hCom3, hCom4;
HWND   hBaudEdit;
BOOL   bGrayBaudEdit;
BOOL   bMouseCapture;
BOOL   bCursorChanged;
short  currentBaud;

HANDLE hOldCursor;
int iSetupPort = 1;
#endif

BOOL bNewPort    = FALSE;
BOOL bResetTitle = FALSE;
BOOL bResetLB    = FALSE;

int errno;

TCHAR  sz386Enh[] = TEXT("386Enh");
TCHAR  szBase[] = TEXT("Base");
TCHAR  szIrq[] = TEXT("Irq");
TCHAR  szCOM[] = TEXT("COM");
TCHAR  szCOLON[] = TEXT(":");
TCHAR  szComPort[20];
TCHAR  szSerialKey[40];
TCHAR  szSERIAL[] = TEXT("Serial");

//  NT Registry keys to find COM port to Serial Device mapping

TCHAR szRegSerialMap[] = TEXT("Hardware\\DeviceMap\\SerialComm");

//  Registry Serial Port Advanced I/O settings key and valuenames

TCHAR szRegSerialParam[]  =
            TEXT("System\\CurrentControlSet\\Services\\Serial\\Parameters");

TCHAR szRegSerialIO[]  =
            TEXT("System\\CurrentControlSet\\Services\\Serial\\Parameters\\%s");

TCHAR szRegPortAddress[] = TEXT("PortAddress");
TCHAR szRegPortIRQ[]     = TEXT("Interrupt");
TCHAR szFIFO[]           = TEXT("ForceFifoEnable");
TCHAR szDosDev[]         = TEXT("DosDevices");


// static short nBaudRates[] = { 110, 300, 600, 1200, 2400, 4800, 9600, 14400, 19200, 0};
static int nBaudRates[] = { 75, 110, 134, 150, 300, 600, 1200, 1800, 2400,
                            4800, 7200, 9600, 14400, 19200, 38400, 57600,
                            115200, 128000, 0};

static TCHAR sz9600[] = TEXT("9600");
static TCHAR szDefParams[] = TEXT("9600,n,8,1");

static short nDataBits[] = { 4, 5, 6, 7, 8, 0};

static TCHAR *pszParityOpts[] = { TEXT("Even"), TEXT("Odd"), TEXT("None"), TEXT("Mark"), TEXT("Space"), TEXT("\0")};

static TCHAR *pszStopBits[] = { TEXT("1"), TEXT("1.5"), TEXT("2"), TEXT("\0")};

static TCHAR *pszFlowCtl[] = { TEXT("Xon / Xoff"), TEXT("Hardware"), TEXT("None"), TEXT("\0")};


TCHAR   *pszParitySuf[] = { TEXT(",e"), TEXT(",o"), TEXT(",n"), TEXT(",m"), TEXT(",s")};

TCHAR   *pszLenSuf[] = { TEXT(",4"), TEXT(",5"), TEXT(",6"), TEXT(",7"), TEXT(",8")};

TCHAR   *pszStopSuf[] = { TEXT(",1"), TEXT(",1.5"), TEXT(",2")};

TCHAR   *pszFlowSuf[] = { TEXT(",x"), TEXT(",p"), TEXT(" ")};

//==========================================================================
//                            Local Function Prototypes
//==========================================================================
BOOL AdvancedPortDlg (HWND hDlg, UINT message, DWORD wParam, LONG lParam);
BOOL CheckBaseIOSetting (LPTSTR);
BOOL CommDlg (HWND hDlg, UINT message, DWORD wParam, LONG lParam);
BOOL RestartDlg (HWND hDlg, UINT message, DWORD wParam, LONG lParam);


//==========================================================================
//                                Functions
//==========================================================================

BOOL ChkCommSettings(HWND hDlg)
{
    TCHAR    szTest[133];
    TCHAR    *pszVerify;

    SendDlgItemMessage(hDlg, PORT_BAUDRATE, WM_GETTEXT, 80, (LPARAM)szTest);
    for (pszVerify = szTest; *pszVerify != TEXT('\0'); pszVerify++)
    {
        if ((*pszVerify < TEXT('0')) || (*pszVerify > TEXT('9')))
        {
            if (!LoadString(hModule, ERRORS, szTest, CharSizeOf(szTest)))
                ErrMemDlg(hDlg);
            else
                MessageBox (hDlg, szTest, szCtlPanel,
                            MB_OK | MB_ICONINFORMATION);
            return(FALSE);                         //  ERROR EXIT
        }
    }
    return(TRUE);
}


BOOL CheckBaseIOSetting (LPTSTR lpszBaseIO)
{
    LPTSTR lpch;
    BOOL bRet = TRUE;

    CharUpper (lpszBaseIO);

    for (lpch = lpszBaseIO; *lpch; lpch++)
        if ((*lpch < TEXT('0') || *lpch > TEXT('9')) && (*lpch < TEXT('A') || *lpch > TEXT('F')))
        {
            bRet = FALSE;
            break;
        }

    return (bRet);
}


//
//  Set dialog items to defaults from win.ini for given port
//  Port is zero based, 0 = com1, 1 = com2, etc.
//
//  void SetFromWin(HWND hDlg, int Port)
//

void SetFromWin (HWND hDlg)
{
    TCHAR  szParms[81];               // parms from win.ini for port
    TCHAR *pszCur, *pszNext;
    int    nIndex;
    int    baud;

//  szComPort[3] = (TCHAR) (TEXT('1') + Port);
    GetProfileString(szPorts, szComPort, MYNUL, szParms, 81);

    StripBlanks(szParms);
    pszCur = szParms;

    //
    //  baud rate
    //

    pszNext = strscan(pszCur, szComma);

    if (*pszNext)
        *pszNext++ = 0;

#ifdef OLDWAY
    if (*pszCur)
    {
        baud = myatoi (pszCur);
        for (nIndex = 0; nBaudRates[nIndex]; nIndex++)
        {
            if (baud == nBaudRates[nIndex])
                break;
        }
        if (nBaudRates[nIndex] == 0)
            nIndex = SendDlgItemMessage(hDlg, PORT_BAUDRATE, CB_ADDSTRING, 0, (LPARAM)pszCur);

        SendDlgItemMessage(hDlg, PORT_BAUDRATE, CB_SETCURSEL, nIndex, 0L);
    }
#else
    //
    // Find current Baud Rate selection
    //

    nIndex = (short) SendDlgItemMessage (hDlg, PORT_BAUDRATE,
                                         CB_FINDSTRING,
                                         (WPARAM) -1,
                                         (LPARAM) pszCur);

    nIndex = (nIndex == CB_ERR) ? 0 : nIndex;

    SendDlgItemMessage (hDlg, PORT_BAUDRATE, CB_SETCURSEL, nIndex, 0L);

#endif  //  OLDWAY

    pszCur = pszNext;

    //
    //  parity
    //

    pszNext = strscan(pszCur, szComma);

    if (*pszNext)
        *pszNext++ = 0;

    StripBlanks(pszCur);

    switch (*pszCur)
    {
    case TEXT('o'):
        nIndex = PAR_ODD;
        break;
    case TEXT('e'):
        nIndex = PAR_EVEN;
        break;
    case TEXT('n'):
        nIndex = PAR_NONE;
        break;
    case TEXT('m'):
        nIndex = PAR_MARK;
        break;
    case TEXT('s'):
        nIndex = PAR_SPACE;
        break;
    default:
        nIndex = DEF_PARITY;
        break;
    }

    SendDlgItemMessage (hDlg, PORT_PARITY, CB_SETCURSEL, nIndex, 0L);
    pszCur = pszNext;

    //
    //  word length
    //

    pszNext = strscan(pszCur, szComma);

    if (*pszNext)
        *pszNext++ = 0;

    StripBlanks(pszCur);
    nIndex = (*pszCur - TEXT('4'));

    if ( (nIndex >= 0) && (nIndex <= 4) )
        SendDlgItemMessage (hDlg, PORT_DATABITS, CB_SETCURSEL, nIndex, 0L);
    else
        SendDlgItemMessage (hDlg, PORT_DATABITS, CB_SETCURSEL, DEF_WORD, 0L);

    pszCur = pszNext;

    //
    //  stop bits
    //

    pszNext = strscan(pszCur, szComma);

    if (*pszNext)
        *pszNext++ = 0;

    StripBlanks(pszCur);

    if (!lstrcmp(pszCur, TEXT("1")))
        SendDlgItemMessage (hDlg, PORT_STOPBITS, CB_SETCURSEL, STOP_1, 0L);
    else if (!lstrcmp(pszCur, TEXT("1.5")))
        SendDlgItemMessage (hDlg, PORT_STOPBITS, CB_SETCURSEL, STOP_15, 0L);
    else if (!lstrcmp(pszCur, TEXT("2")))
        SendDlgItemMessage (hDlg, PORT_STOPBITS, CB_SETCURSEL, STOP_2, 0L);
    else
        SendDlgItemMessage (hDlg, PORT_STOPBITS, CB_SETCURSEL, DEF_STOP, 0L);

    pszCur = pszNext;

    //
    //  handshaking: Hardware, xon/xoff, or none
    //

    pszNext = strscan(pszCur, szComma);

    if (*pszNext)
        *pszNext++ = 0;

    StripBlanks(pszCur);

    if (*pszCur == TEXT('p'))
        SendDlgItemMessage (hDlg, PORT_FLOWCTL, CB_SETCURSEL, FLOW_HARD, 0L);
    else if (*pszCur == TEXT('x'))
        SendDlgItemMessage (hDlg, PORT_FLOWCTL, CB_SETCURSEL, FLOW_XON, 0L);
    else
        SendDlgItemMessage (hDlg, PORT_FLOWCTL, CB_SETCURSEL, FLOW_NONE, 0L);

    return;
}


//
//  This routine reads off the settings from the dialog and writes them
//  out to win.ini
//
//  This routine was completely overhauled to work with the new 3.1 Dialog
//  box.  Modified by C. Stevens, Oct. 90
//

void CommPortsToWin (HWND hDlg)
{
    TCHAR szBuild[PATHMAX];
    int   i;

    //
    //  Get the baud rate
    //

    i = SendDlgItemMessage (hDlg, PORT_BAUDRATE, WM_GETTEXT, 18, (LPARAM)szBuild);

    if (i == 0)
        return;

    //
    //  Get the parity setting
    //

    i = SendDlgItemMessage (hDlg, PORT_PARITY, CB_GETCURSEL, 0, 0L);

    if ((i == CB_ERR) || (i == CB_ERRSPACE))
        return;
        
    lstrcat (szBuild, pszParitySuf[i]);

    //
    //  Get the word length
    //

    i = SendDlgItemMessage (hDlg, PORT_DATABITS, CB_GETCURSEL, 0, 0L);

    if ((i == CB_ERR) || (i == CB_ERRSPACE))
        return;
        
    lstrcat (szBuild, pszLenSuf[i]);

    //
    //  Get the stop bits
    //

    i = SendDlgItemMessage (hDlg, PORT_STOPBITS, CB_GETCURSEL, 0, 0L);

    if ((i == CB_ERR) || (i == CB_ERRSPACE))
        return;
        
    lstrcat (szBuild, pszStopSuf[i]);

    //
    //  Get the flow control
    //

    i = SendDlgItemMessage (hDlg, PORT_FLOWCTL, CB_GETCURSEL, 0, 0L);

    if ((i == CB_ERR) || (i == CB_ERRSPACE))
        return;
        
    lstrcat (szBuild, pszFlowSuf[i]);

//    szComPort[3] = (char) ('0' + iSetupPort);

    //
    //  Write settings string to [ports] section in win.ini
    //

    WriteProfileString (szPorts, szComPort, szBuild);

    SendWinIniChange ((LPTSTR)szPorts);
}

#ifdef OLDWAY
// iPort is a 1 based COM port number (1 == COM1:)

int SetupCommPort(HWND hDlg, int iPort)
{
    iSetupPort = iPort;

    return(DoDialogBoxParam(DLG_PORTS2, hDlg, CommDlg, IDH_DLG_PORTS2, 0L));
//    return(DialogBox (hModule, MAKEINTRESOURCE(DLG_PORTS2), hDlg,(DLGPROC) CommDlg));
}

void ChangePortSelection(HWND hDlg, int iNewPort, BOOL bOldOnly)
{
    HWND hwnd;

    hwnd = GetDlgItem(hDlg, iSetupPort + PORT_COM1RECT - 1);
    iSetupPort = iNewPort;
    InvalidateRect(hwnd, NULL, TRUE);
    if (!bOldOnly)
        InvalidateRect(GetDlgItem(hDlg, iNewPort + PORT_COM1RECT - 1), NULL, TRUE);
}


void DrawPortButton(HWND hDlg, LPDRAWITEMSTRUCT lpdis)
{
    int    x, y;
    RECT   rc;
    HBRUSH hbr;

    x = (lpdis->rcItem.left + (lpdis->rcItem.right -  (int)GSM(SM_CXICON))) / 2;
    y = (lpdis->rcItem.top  + (lpdis->rcItem.bottom - (int)GSM(SM_CYICON))) / 2;
    rc.left   = x;
    rc.right  = x + (int)GSM(SM_CXICON);
    rc.top    = y;
    rc.bottom = y + (int)GSM(SM_CYICON);

    DrawIcon(lpdis->hDC, x, y, hiconPorts);

    if (lpdis->itemAction == ODA_FOCUS)
        ChangePortSelection(hDlg, lpdis->CtlID - PORT_COM1RECT + 1, TRUE);

    if (lpdis->CtlID == (WORD)(PORT_COM1RECT + iSetupPort - 1))
        hbr = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
    else
        hbr = CreateSolidBrush(GetSysColor(COLOR_WINDOW));

    if (hbr)
    {
        InflateRect(&rc, 2, 2);
        FrameRect(lpdis->hDC, &rc, hbr);
        DeleteObject(hbr);
    }
}
#endif  //  OLDWAY


BOOL ShortCommDlg (HWND hDlg, UINT message, DWORD wParam, LONG lParam)
{
    int    i, j, nEntries;
    DWORD  dwSize, dwBufz;
    DWORD  dwType;
    HKEY   hkey, hkeySub;
    TCHAR  szSerial[40];
    TCHAR  szCom[40];
    BOOL   bPorts[MAX_COM_PORT+1];   // 1 based array of all allowable COM ports

    switch (message)
    {

    case WM_INITDIALOG:
        HourGlass (TRUE);

        // Init globals
        bResetTitle = FALSE;
        bResetLB    = FALSE;

        bNewPort = FALSE;

        //////////////////////////////////////////////////////////////////////
        //  Create a MERGED listing of COM ports between DeviceMap and
        //  Services node.
        //////////////////////////////////////////////////////////////////////

        //  Always clear BOOL array
        for (i = 0; i <= MAX_COM_PORT; bPorts[i++] = FALSE)
            ;

        //////////////////////////////////////////////////////////////////////
        //  Get list of valid COM ports from DEVICEMAP in registry
        //////////////////////////////////////////////////////////////////////

        // Reset list box before display.
        SendDlgItemMessage (hDlg, PORT_LB, LB_RESETCONTENT, 0, 0L);

        if (!RegOpenKeyEx (HKEY_LOCAL_MACHINE, szRegSerialMap,
                                                    0L, KEY_READ, &hkey))
        {
            dwBufz = sizeof(szSerial);
            dwSize = sizeof(szCom);
            nEntries = i = 0;

            while (!RegEnumValue (hkey, i++, szSerial, &dwBufz,
                                  NULL, &dwType, (LPBYTE) szCom, &dwSize))
            {
                if (dwType != REG_SZ)
                    continue;

                // Append ":" char to end of szCom string

                lstrcat (szCom, TEXT(":"));

                // Get number of COM port (go past "COM" in string)
                j = myatoi (&szCom[3]);

                if (j <= MAX_COM_PORT)
                    bPorts[j] = TRUE;
                else
                    continue;

                // Put Port name string in ListBox

                if ((j = (int)SendDlgItemMessage (hDlg, PORT_LB, LB_INSERTSTRING,
                                  (WPARAM)(LONG)-1, (LPARAM) szCom)) >= 0)
                {
                    SendDlgItemMessage (hDlg, SERIAL_DBASE, LB_INSERTSTRING,
                                        j, (LPARAM) szSerial);
                    ++nEntries;
                }

                dwSize = sizeof(szCom);
                dwBufz = sizeof(szSerial);
            }
            RegCloseKey (hkey);
        }

        //////////////////////////////////////////////////////////////////////
        //  Get list of valid COM ports from Services Node in registry
        //////////////////////////////////////////////////////////////////////

        hkey = NULL;

        //  Read Serial keys at Services Node
        if (!RegOpenKeyEx (HKEY_LOCAL_MACHINE,        // Root key
                           szRegSerialParam,          // Subkey to open
                           0L,                        // Reserved
                           KEY_READ,                  // SAM
                           &hkey))                    // return handle
        {
            //  Enumerate all keys under Serial\Parameters
            i = 0;

            while (RegEnumKey (hkey, i++, szSerial, CharSizeOf(szSerial))
                                                     != ERROR_NO_MORE_ITEMS)
            {
                hkeySub = NULL;

                if (!RegOpenKeyEx (hkey,                      // Root key
                                   szSerial,                  // Subkey to open
                                   0L,                        // Reserved
                                   KEY_READ,                  // SAM
                                   &hkeySub))                 // return handle
                {
                    //  Get DosDevices value for this Serial key

                    dwSize = sizeof(szCom);

                    if (!RegQueryValueEx (hkeySub, szDosDev, NULL, &dwType,
                                          (LPBYTE) szCom, &dwSize))
                    {
                        if (dwType != REG_SZ)
                            goto TryNextSubKey;

                        // Append ":" char to end of szCom string

                        lstrcat (szCom, TEXT(":"));

                        // Get number of COM port (go past "COM" in string)
                        j = myatoi (&szCom[3]);

                        //  Ignore if this port already found in DeviceMap
                        if ((j <= MAX_COM_PORT) && (!bPorts[j]))
                            bPorts[j] = TRUE;
                        else
                            goto TryNextSubKey;

                        // Put Port name string in ListBox
                        if ((j = (int)SendDlgItemMessage (hDlg,
                                                          PORT_LB,
                                                          LB_INSERTSTRING,
                                                          (WPARAM) -1,
                                                          (LPARAM) szCom)) >= 0)
                        {
                            SendDlgItemMessage (hDlg,
                                                SERIAL_DBASE,
                                                LB_INSERTSTRING,
                                                j,
                                                (LPARAM) szSerial);
                            ++nEntries;
                        }
                    }
TryNextSubKey:
                    RegCloseKey (hkeySub);
                }
            }
            RegCloseKey (hkey);
        }

        // By default, select the first item in the Listbox
        SendDlgItemMessage (hDlg, PORT_LB, LB_SETCURSEL, 0, 0L);

        /////////////////////////////////////////////////////////////////
        //  Check for Greying out ADD and DELETE buttons
        /////////////////////////////////////////////////////////////////
#ifdef OLD
        //  Check for registry access to both the DeviceMap and
        //  Services nodes

        if ((RegOpenKeyEx (HKEY_LOCAL_MACHINE,        // Root key      
                           szRegSerialMap,            // Subkey to open
                           0L,                        // Reserved      
                           KEY_READ | KEY_WRITE,      // SAM           
                           &hkeySub)                  // return handle
                != ERROR_SUCCESS)
            ||

            RegCloseKey (hkeySub);

#endif  //  OLD

        //
        //  Check for registry access to the Services nodes for
        //  adding new ports.
        //

        if ((RegOpenKeyEx (HKEY_LOCAL_MACHINE,        // Root key
                           szRegSerialParam,          // Subkey to open
                           0L,                        // Reserved
                           KEY_READ | KEY_WRITE,      // SAM
                           &hkey)                     // return handle
                != ERROR_SUCCESS))
        {
            EnableWindow (GetDlgItem (hDlg, PORT_ADD), FALSE);
            EnableWindow (GetDlgItem (hDlg, PORT_DELETE), FALSE);
            RegCloseKey (hkey);
        }

        HourGlass (FALSE);
        return TRUE;


#ifdef OLDWAY
    case WM_DRAWITEM:
        DrawPortButton(hDlg, (LPDRAWITEMSTRUCT)lParam);
        break;
#endif  //  OLDWAY

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDD_HELP:
            goto DoHelp;

        case IDOK:
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            break;

#ifdef OLDWAY
        case PORT_COM1RECT:
        case PORT_COM2RECT:
        case PORT_COM3RECT:
        case PORT_COM4RECT:
            ChangePortSelection(hDlg, LOWORD(wParam) - PORT_COM1RECT + 1, FALSE);
            SendMessage(hDlg, WM_NEXTDLGCTL, (WPARAM) GetDlgItem(hDlg, PORT_SETTING), 1L);

            if (HIWORD(wParam) != BN_DOUBLECLICKED)
                break;

            // fall through...
#endif  //  OLDWAY


        case PORT_LB:
            // If user Double Clicks on listbox item, react as though they
            // clicked on "Settings" button
            if (HIWORD(wParam) != LBN_DBLCLK)
                break;

            // fall through...

        case PORT_SETTING:
#ifdef OLDWAY
            if (SetupCommPort(hDlg, iSetupPort) == IDOK)
                SetDlgItemText(hDlg, IDOK, pszClose);
#endif //OLDWAY

            i = SendDlgItemMessage (hDlg, PORT_LB, LB_GETCURSEL, 0, 0L);

            if (i != LB_ERR)
            {
                //
                // Set the global COM PORT string for later use
                //

                SendDlgItemMessage (hDlg, PORT_LB, LB_GETTEXT, i, (LPARAM)szComPort);
                SendDlgItemMessage (hDlg, SERIAL_DBASE, LB_GETTEXT, i, (LPARAM)szSerialKey);

                if (DoDialogBoxParam(DLG_PORTS2, hDlg, (DLGPROC)CommDlg,
                                                IDH_DLG_PORTS2, 0L) == IDOK)
                {
                    SetDlgItemText(hDlg, IDOK, pszClose);
                }

                if (bResetLB)
                {
                    //
                    // User changed COM Port Number, delete old, add new
                    //

                    SendDlgItemMessage (hDlg, PORT_LB, LB_DELETESTRING, i, 0L);
                    SendDlgItemMessage (hDlg, SERIAL_DBASE, LB_DELETESTRING, i, 0L);

                    //
                    // Put New Port and Serial name strings in ListBoxes
                    //

                    if ((j = (int)SendDlgItemMessage (hDlg, PORT_LB, LB_INSERTSTRING,
                              (WPARAM)(LONG)-1, (LPARAM) szComPort)) >= 0)
                    {
                        SendDlgItemMessage (hDlg, SERIAL_DBASE, LB_INSERTSTRING,
                                            j, (LPARAM) szSerialKey);
                    }

                    j = (j > 0) ? j : 0;

                    //
                    // Select an item in the Listbox
                    //

                    SendDlgItemMessage (hDlg, PORT_LB, LB_SETCURSEL, j, 0L);

                    //
                    // Reset globals
                    //

                    bResetTitle = FALSE;
                    bResetLB    = FALSE;
                }
            }
            break;

        case PORT_ADD:
        {
            DWORD dwSave;

            //
            // Set global flag for Advanced Dialog
            //

            bNewPort = TRUE;

            dwSave = dwContext;
            dwContext = IDH_DLG_PORTS3;

            //
            //  Setup some Advanced parameters for this port now
            //

            if (DialogBox (hModule, (LPTSTR) MAKEINTRESOURCE(DLG_PORTS3), hDlg,
                  (DLGPROC) AdvancedPortDlg) == 1)
            {
                SetDlgItemText (hDlg, IDOK, pszClose);

                //
                // Add new port name to listbox
                // Add new serial value to SERIAL_DBASE listbox
                //
                
                if ((j = (int)SendDlgItemMessage (hDlg, PORT_LB, LB_ADDSTRING,
                                  (WPARAM)(LONG)-1, (LPARAM) szComPort)) >= 0)
                {
                    SendDlgItemMessage (hDlg, SERIAL_DBASE, LB_INSERTSTRING,
                                        j, (LPARAM) szSerialKey);

                    //
                    // Select an item in the Listbox
                    //

                    SendDlgItemMessage (hDlg, PORT_LB, LB_SETCURSEL, j, 0L);

                    SendWinIniChange (szPorts);
                }
            }

            bNewPort = FALSE;
            dwContext = dwSave;

            break;
        }

        case PORT_DELETE:
        {
            /////////////////////////////////////////////////////////////////
            //  Check for registry access to both the DeviceMap and
            //  Services nodes
            /////////////////////////////////////////////////////////////////

            if ((RegOpenKeyEx (HKEY_LOCAL_MACHINE,        // Root key      
                               szRegSerialMap,            // Subkey to open
                               0L,                        // Reserved      
                               KEY_READ | KEY_WRITE,      // SAM           
                               &hkey)                     // return handle
                    != ERROR_SUCCESS)
                ||
                (RegOpenKeyEx (HKEY_LOCAL_MACHINE,        // Root key
                               szRegSerialParam,          // Subkey to open
                               0L,                        // Reserved
                               KEY_READ | KEY_WRITE,      // SAM
                               &hkeySub)                  // return handle
                    != ERROR_SUCCESS))
            {
                MyMessageBox (hDlg, MYPORT+7, INITS+1,
                                        MB_OKCANCEL | MB_ICONEXCLAMATION);
                break;
            }


            /////////////////////////////////////////////////////////////////
            //  Confirm deletion
            /////////////////////////////////////////////////////////////////

            i = MyMessageBox (hDlg, MYPORT+6, INITS+1,
                                        MB_OKCANCEL | MB_ICONEXCLAMATION);

            /////////////////////////////////////////////////////////////////
            //  Delete everything associated with COM port
            /////////////////////////////////////////////////////////////////

            if (i == IDOK)
            {
                SetDlgItemText (hDlg, IDOK, pszClose);

                //
                //  Get current selection
                //

                i = SendDlgItemMessage (hDlg, PORT_LB, LB_GETCURSEL, 0, 0L);
                SendDlgItemMessage (hDlg, SERIAL_DBASE, LB_GETTEXT, i, (LPARAM)szSerial);
                SendDlgItemMessage (hDlg, PORT_LB, LB_GETTEXT, i, (LPARAM)szCom);

                //
                //  Delete Registry entries for this COM port
                //
                //  Delete Value entry in DeviceMap\SERIALCOMM key
                //  Delete entire key under Services\Serial\Parameters node
                //

                RegDeleteValue (hkey, szSerial);
                RegDeleteKey (hkeySub, szSerial);

                RegCloseKey (hkey);
                RegCloseKey (hkeySub);

                //
                //  Delete WIN.INI entry - write NULL string to [ports] section
                //

                WriteProfileString (szPorts, szCom, (LPTSTR) NULL);

                //
                //  Delete ListBox items
                //

                SendDlgItemMessage (hDlg, PORT_LB, LB_DELETESTRING, i, 0L);
                SendDlgItemMessage (hDlg, SERIAL_DBASE, LB_DELETESTRING, i, 0L);

                //
                //  Selection next item in ListBox
                //

                j = SendDlgItemMessage (hDlg, PORT_LB, LB_GETCOUNT, 0, 0L);

                j = (j > i) ? i : j;

                //
                // Select an item in the Listbox
                //

                SendDlgItemMessage (hDlg, PORT_LB, LB_SETCURSEL, j, 0L);

                //
                // Let the rest of the world know also
                //

                SendWinIniChange (szPorts);
            }

            break;
        }

        }
        break;

    default:

        if (message == wHelpMessage)
        {
DoHelp:
            CPHelp(hDlg);
            return TRUE;
        }
        else
            return FALSE;
        break;
    }
  return TRUE;

}


VOID SetCBFromRes(HWND hCB, DWORD wRes, DWORD wDef)
{
    TCHAR  szTemp[258], cSep;
    LPTSTR pThis, pThat;

    if (!LoadString(hModule, wRes, szTemp, CharSizeOf(szTemp)))
        return;

    for (pThis = szTemp, cSep = *pThis++; pThis; pThis = pThat)
    {
        if (pThat = _tcschr(pThis, cSep))
            *pThat++ = TEXT('\0');
        SendMessage(hCB, CB_ADDSTRING, 0, (LPARAM)pThis);
    }
    SendMessage(hCB, CB_SETCURSEL, wDef, 0L);
}

/* CommDlg
   This is the communications ports setup dialog.  It just reads all the
   items, munges them all together into a string, and then writes it out
   to win.ini.  This routine does not interact with the comm driver.  It
   is the responsibility of each app that uses the comm ports to go first
   to win.ini, read the appropriate line, and then set up the comm port
   accordingly
*/

BOOL CommDlg(HWND hDlg, UINT message, DWORD wParam, LONG lParam)
{
    TCHAR   szTitle[81];
    TCHAR   szTitleFormat[81];
    short   nIndex;
    HWND    hParent;
    DWORD    dwMask;
    HANDLE   hComm;
    COMMPROP cpComm;

    switch (message)
    {
    case WM_INITDIALOG:
        /* init to defaults */

        // Get info about COM port from Serial driver
        hComm = CreateFile (szComPort,
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

        if ((hComm != INVALID_HANDLE_VALUE) &&
             GetCommProperties (hComm, &cpComm))
        {
            // Check baud rate bitmask and only display settable rates
            for (dwMask = 1, nIndex = 0; nBaudRates[nIndex]; nIndex++)
            {
                // BAUD_128K comes before 115200 in bitmask, special checks
                if ((cpComm.dwSettableBaud & dwMask) == BAUD_128K)
                {
                    // Check for BAUD_115200 first and put it in before this one
                    if (cpComm.dwSettableBaud & BAUD_115200)
                    {
                        MyItoa(nBaudRates[nIndex], szTitle, 10);
                        SendDlgItemMessage (hDlg, PORT_BAUDRATE, CB_ADDSTRING,
                                                  0, (LPARAM) szTitle);
                    }
                    //  Now increment to 128K baud rate value
                    nIndex++;

                    MyItoa(nBaudRates[nIndex], szTitle, 10);
                    SendDlgItemMessage (hDlg, PORT_BAUDRATE, CB_ADDSTRING,
                                              0, (LPARAM) szTitle);

                    // Move mask over another bit since we have checked
                    // for the 115200 value
                    dwMask <<= 1;
                }
                else if ((cpComm.dwSettableBaud & dwMask) == BAUD_115200)
                {
                    MyItoa(nBaudRates[nIndex-1], szTitle, 10);
                    SendDlgItemMessage (hDlg, PORT_BAUDRATE, CB_ADDSTRING,
                                              0, (LPARAM) szTitle);
                }
                else if (cpComm.dwSettableBaud & dwMask)
                {
                    MyItoa(nBaudRates[nIndex], szTitle, 10);
                    SendDlgItemMessage (hDlg, PORT_BAUDRATE, CB_ADDSTRING,
                                              0, (LPARAM) szTitle);
                }

                dwMask <<= 1;
            }
            CloseHandle (hComm);
        }
        else // Open failure, just list all of the baud rates
        {
            for (nIndex = 0; nBaudRates[nIndex]; nIndex++)
            {
                MyItoa(nBaudRates[nIndex], szTitle, 10);
                SendDlgItemMessage (hDlg, PORT_BAUDRATE, CB_ADDSTRING, 0,
                                                        (LPARAM) szTitle);
            }
        }

        // Set 9600 as default baud selection
        nIndex = (short) SendDlgItemMessage (hDlg, PORT_BAUDRATE,
                                             CB_FINDSTRING,
                                             (WPARAM) -1,
                                             (LPARAM) sz9600);

        nIndex = (nIndex == CB_ERR) ? 0 : nIndex;

        SendDlgItemMessage (hDlg, PORT_BAUDRATE, CB_SETCURSEL, nIndex, 0L);

        for (nIndex = 0; nDataBits[nIndex]; nIndex++)
        {
            MyItoa(nDataBits[nIndex], szTitle, 10);
            SendDlgItemMessage (hDlg, PORT_DATABITS, CB_ADDSTRING, 0,
                (LPARAM) szTitle);
        }
        SendDlgItemMessage (hDlg, PORT_DATABITS, CB_SETCURSEL, DEF_WORD, 0L);

        SetCBFromRes(GetDlgItem(hDlg, PORT_PARITY), MYPORT+1, DEF_PARITY);
        SetCBFromRes(GetDlgItem(hDlg, PORT_STOPBITS), MYPORT+2, DEF_STOP);
        SetCBFromRes(GetDlgItem(hDlg, PORT_FLOWCTL), MYPORT+3, DEF_SHAKE);

        LoadString(hModule, MYPORT + 5, szTitleFormat, CharSizeOf(szTitleFormat));
        wsprintf(szTitle, szTitleFormat, szComPort);
        SetWindowText(hDlg, szTitle);

        SetFromWin(hDlg);

////////////////////////////////////////////////////////////////////////////////
//  Test code to determine if this is a "Serial" port or "Digiboard" port
////////////////////////////////////////////////////////////////////////////////
        //
        //  Test global szSerialKey string to determine if this is a Serial
        //  driver port or a 3rd party (i.e OEM) port.  If it is a 3rd party
        //  port, we do not attempt to Setup Advanced parameters for it.
        //

        if (!_tcsstr (szSerialKey, szSERIAL))
            EnableWindow (GetDlgItem (hDlg, PORT_ADVANCED), FALSE);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

        //
        // center and make visible
        //

        ShowWindow(hDlg, SHOW_OPENWINDOW);

        return(TRUE);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDD_HELP:
            goto DoHelp;

        case PORT_ADVANCED:
        {
            DWORD dwSave;

            dwSave = dwContext;
            dwContext = IDH_DLG_PORTS3;

            if (DialogBox (hModule, (LPTSTR) MAKEINTRESOURCE(DLG_PORTS3), hDlg,
                  (DLGPROC) AdvancedPortDlg) == 1)
            {
                SetDlgItemText (hDlg, PUSH_CANCEL, pszClose);
                hParent = GetParent (hDlg);
                SetDlgItemText (hParent, IDOK, pszClose);

                // COM Port Number changed, change our title, tell parent also
                if (bResetTitle)
                {
                    LoadString(hModule, MYPORT + 5, szTitleFormat,
                                                CharSizeOf(szTitleFormat));
                    wsprintf(szTitle, szTitleFormat, szComPort);
                    SetWindowText(hDlg, szTitle);
                    bResetTitle = FALSE;

                    // Tell parent dlg to reset when we get there
                    bResetLB = TRUE;
                }
            }

            dwContext = dwSave;

            break;
        }

        case PUSH_OK:
            if (ChkCommSettings(hDlg))
            {
                /* change cursor to hourglass */
                HourGlass(TRUE);

                /* store changes to win.ini; broadcast changes to apps */
                CommPortsToWin(hDlg);

                /* change cursor back to arrow */
                HourGlass(FALSE);
            }
            else
            {
                SetFocus(GetDlgItem(hDlg, PORT_BAUDRATE));
                SendDlgItemMessage(hDlg, PORT_BAUDRATE, EM_SETSEL, 0, 32767);
                break;
            }
            /* fall through */

        case PUSH_CANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            break;

        default:
            break;
        }
        break;

    default:
        if (message == wHelpMessage)
        {
DoHelp:
            CPHelp(hDlg);
            return TRUE;
        }
        else
            return FALSE;
        break;
    }
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// AdvancedPortDlg
//
// Note: This dlg uses the global strings szComPort and szSerialKey.
//       and sets the global BOOL bResetTitle.
//
//       The global BOOL bNewPort is used to modify behavoir of this
//       dialog slightly when creating new COM ports.
//
//       DEFAULT values in Base I/O and IRQ controls are only valid for
//       COM1 - COM4 if nothing currently exists for these ports.  For all
//       other COM ports, the DEFAULT selection basically means just leaving
//       the current configured value for that port as it is.
//
/////////////////////////////////////////////////////////////////////////////

#define IRQMIN 2
#define IRQMAX 15
#define IRQDEF 2
#define BASEIODEF 1 /* A000 */

TCHAR *pszBaseIO[] = { TEXT("03F8"), TEXT("02F8"), TEXT("03E8"), TEXT("02E8"), TEXT("02E0"), TEXT("\0")};

BOOL AdvancedPortDlg (HWND hDlg, UINT nMsg, DWORD wParam, LONG lParam)
{
    TCHAR  szFormat[200];
    TCHAR  szBaseIO[8];
    TCHAR  szTitle[120];
    TCHAR  szDef[20];
    TCHAR  szCom[40];
    TCHAR *pszTemp;
    int    nVal;
    int    nIndex;
    DWORD  dwSize, dwBaseIO, dwFIFO;
    DWORD  dwType, dwBaseIRQ;
    BOOL   bNone, bWroteToSerialKey;
    HWND   hParent;
    int    LastCom, NewCom;
    int    i, nEntries;
    HKEY   hkey;

    static int   nStartIndex;
    static DWORD nFifo;
    static int   nPortNumber;
    static TCHAR szStartString[10];
    static HKEY  hkeySerial = NULL;
    static DWORD dwDisposition;


    switch (nMsg)
    {

    case WM_INITDIALOG:
    {
        TCHAR  szSerial[40];
        int    LastSerial, NewSerial;

        HourGlass (TRUE);

        dwDisposition = 0;

        if (bNewPort)
        {
            //////////////////////////////////////////////////////////////////
            //  Determine new "Serial" and "Com" values
            //////////////////////////////////////////////////////////////////

            hParent = GetParent (hDlg);

            //
            // Determine the last "Serial" and "Com" values
            //

            nEntries = SendDlgItemMessage (hParent, PORT_LB, LB_GETCOUNT, 0, 0);

            //
            //  Since BIOS usually detects COM1 - COM2
            //  New "COMx" port value cannot be less than COM3
            //  [stevecat] - for Win NT 3.11 we allow this to be COM1 & COM2
            //

            NewCom = MIN_COM;

            //
            //  New "Serialxx" value cannot be less than 10,000
            //

            NewSerial = MIN_SERIAL;

            //
            // Find last (largest)  numbers for COM and SERIAL port numbers
            //

            for (i = 0; i < nEntries; i++)
            {
                SendDlgItemMessage (hParent, PORT_LB, LB_GETTEXT, i, (LPARAM)szCom);

                //
                // Get number of Last COM port (go past "COM" in string)
                //

                LastCom = myatoi (&szCom[3]);

                NewCom = max(LastCom+1, NewCom);

                SendDlgItemMessage (hParent, SERIAL_DBASE, LB_GETTEXT, i,
                                                              (LPARAM)szSerial);

                //
                // Get number of Last Serial entry (go past "Serial" in string)
                //

                LastSerial = myatoi (&szSerial[6]);

                NewSerial = max(LastSerial+1, NewSerial);
            }

            //
            // Use NewCom as the recommendation (only) for new COM port number
            //

            NewCom = (NewCom > MAX_COM_PORT) ? MIN_COM : NewCom;

            //
            // Setup new global registry SerialKey value
            //

            wsprintf (szSerialKey, TEXT("%s%d"), szSERIAL, NewSerial);

            //
            //  Set up window title
            //

            LoadString(hModule, MYPORT + 9, szFormat, CharSizeOf(szFormat));
            LoadString(hModule, MYPORT + 4, szCom, CharSizeOf(szCom));

            wsprintf(szTitle, szFormat, szCom);
            SetWindowText(hDlg, szTitle);
        }
        else
        {
            //
            // Determine current COM port number
            //
            // NOTE: szSerialKey and szComPort global vlaues are already set
            //
            // Get number of Last COM port (go past "COM" in string)
            //

            NewCom = myatoi (&szComPort[3]);

            //
            //  Set up window title
            //

            LoadString(hModule, MYPORT + 9, szFormat, CharSizeOf(szFormat));
            wsprintf(szTitle, szFormat, szComPort);
            SetWindowText(hDlg, szTitle);
        }

        //
        // Init values for "COM port number" combobox
        //

        for (i = MIN_COM; i <= MAX_COM_PORT; i++)
        {
            wsprintf (szCom, TEXT("%d"), i);
            SendDlgItemMessage (hDlg, PORT_NUMBER, CB_ADDSTRING, 0,
                                                        (LPARAM)szCom);
        }

        SendDlgItemMessage (hDlg, PORT_NUMBER, CB_SETCURSEL,
                                                    NewCom-MIN_COM, 0L);


        //////////////////////////////////////////////////////////////////////
        //  Put selections for Port Base I/O address in combobox
        //////////////////////////////////////////////////////////////////////

        //
        //  Limit length of edit boxes
        //

        SendDlgItemMessage (hDlg,PORT_BASEIO, CB_LIMITTEXT,4,0L);

        //
        // Fill combo box; add "(None)" and numbers
        //

        if (!bNewPort)
        {
            //
            // "Default" not allowed for new ports
            //

            LoadString (hModule, MYPORT, szDef, CharSizeOf(szDef));

            SendDlgItemMessage (hDlg, PORT_BASEIO, CB_ADDSTRING, 0, (LPARAM)szDef);
        }

        for (nIndex = 0; *pszBaseIO[nIndex]; nIndex++)
            SendDlgItemMessage (hDlg, PORT_BASEIO, CB_ADDSTRING, 0,
                                (LPARAM) pszBaseIO[nIndex]);

        //////////////////////////////////////////////////////////////////////
        //  Put selections for Port IRQ level in combobox
        //////////////////////////////////////////////////////////////////////

        if (!bNewPort)
        {
            //
            //  "Default" not allowed for new ports
            //

            SendDlgItemMessage (hDlg, PORT_IRQ, CB_ADDSTRING, 0, (LPARAM)szDef);
        }

        for (nIndex = IRQMAX; nIndex >= IRQMIN; --nIndex)
        {
            wsprintf (szTitle, TEXT("%d"), nIndex);
            SendDlgItemMessage (hDlg, PORT_IRQ, CB_ADDSTRING, 0, (LPARAM)szTitle);
        }

        //////////////////////////////////////////////////////////////////////
        //  Get configured values for options from Registry
        //////////////////////////////////////////////////////////////////////

        //
        //  Put initial values in edit boxes; note that szDef still
        //  contains "Default"
        //
        //  NT: The NT serial driver doesn't support the EscapeCommFunction
        //      api.  We try to get this value from the Registry.
        //
        //  Make registry key for this Serial Port to get Advanced I/O
        //  settings.  And save hkey around for later use on exit
        //

        hkeySerial = NULL;
        wsprintf (szFormat, szRegSerialIO, szSerialKey);

        //
        //  For New Ports this will create a new serial key entry
        //

        if (!RegCreateKeyEx (HKEY_LOCAL_MACHINE,        // Root key
                             szFormat,                  // Subkey to open/create
                             0L,                        // Reserved
                             NULL,                      // Class string
                             0L,                        // Options
                             KEY_ALL_ACCESS,            // SAM
                             NULL,                      // ptr to Security struct
                             &hkeySerial,               // return handle
                             &dwDisposition))           // return disposition
        {
            //
            //  Try to get the configured Port Base I/O address from Registry
            //

            dwSize = sizeof(DWORD);

            if (!RegQueryValueEx (hkeySerial, szRegPortAddress, NULL, &dwType,
                                              (LPBYTE) &dwBaseIO, &dwSize))
            {
                if (dwType == REG_DWORD)
                {
                    MyUltoa (dwBaseIO, szBaseIO, 16);
                    SetDlgItemText (hDlg, PORT_BASEIO, szBaseIO);
                }
                else
                    goto SetDefaultIO;
            }
            else
            {
SetDefaultIO:
                if (bNewPort)
                {
                    //
                    // Choose first item in combobox for new ports
                    //

                    SendDlgItemMessage (hDlg, PORT_BASEIO, CB_SETCURSEL, 0, 0);
                }
                else
                {
                    //
                    //  If this is port COM1, 2, 3 or 4 allow open to continue
                    //  and set all controls for with default values.
                    //

                    if (NewCom <= 4)
                    {
                        SetDlgItemText(hDlg, PORT_BASEIO, szDef);
                        goto SpecialCom1to4;
                    }

                    //
                    //  Put up a message box error stating that the USER does
                    //  not have the Advanced Serial I/O parameters configured
                    //  in the registry and just continue for now.
                    //

                    RegCloseKey (hkeySerial);

                    if (dwDisposition == REG_CREATED_NEW_KEY)
                    {
                        RegDeleteKey (HKEY_LOCAL_MACHINE, szFormat);
                    }

                    MyMessageBox (hDlg, MYPORT+13, INITS+1,
                                        MB_OK | MB_ICONINFORMATION);

                    HourGlass (FALSE);
                    EndDialog (hDlg, 0);
                    return FALSE;
                }
            }

SpecialCom1to4:

            //
            //  Try to get the configured Port IRQ Level from Registry
            //

            dwSize = sizeof(DWORD);
            dwBaseIRQ = 0;

            if (!RegQueryValueEx (hkeySerial, szRegPortIRQ, NULL, &dwType,
                                               (LPBYTE) &dwBaseIRQ, &dwSize))
            {
                if (dwType != REG_DWORD)
                    dwBaseIRQ = 0;
            }

            //
            //  Try to get the configured FIFO state from Registry
            //

            dwSize = sizeof(DWORD);
            dwFIFO = 0;

            if (!RegQueryValueEx (hkeySerial, szFIFO, NULL, &dwType,
                                               (LPBYTE) &dwFIFO, &dwSize))
            {
                if (dwType != REG_DWORD)
                    dwFIFO = 0;
            }
        }
        else
        {
            MyMessageBox (hDlg, (bNewPort) ? MYPORT+15 : MYPORT+14, INITS+1,
                                                    MB_OK | MB_ICONINFORMATION);
            HourGlass (FALSE);
            EndDialog (hDlg, 0);
            return FALSE;
        }

        //
        //  Make a PORT_IRQ and PORT_FIFO selection
        //

        SendDlgItemMessage(hDlg, PORT_IRQ, CB_SETCURSEL,
                            (dwBaseIRQ >= IRQMIN && dwBaseIRQ <= IRQMAX) ?
                            IRQMAX+1 - dwBaseIRQ : 0, 0L);

        CheckDlgButton (hDlg, PORT_FIFO, dwFIFO);

        //////////////////////////////////////////////////////////////////////
        //  Store away the initial settings so we can warn the user if
        //  they need to reboot
        //////////////////////////////////////////////////////////////////////

        nFifo = dwFIFO;
        nPortNumber = NewCom;
        nStartIndex = SendDlgItemMessage (hDlg,PORT_IRQ,CB_GETCURSEL, 0, 0L);
        GetDlgItemText (hDlg, PORT_BASEIO, szStartString, CharSizeOf(szStartString));

        //////////////////////////////////////////////////////////////////////
        //  Show the window and reset the cursor
        //////////////////////////////////////////////////////////////////////

        ShowWindow (hDlg, SW_SHOW);
        UpdateWindow (hDlg);
        HourGlass (FALSE);
        break;
    }

    case WM_COMMAND:

        switch (LOWORD(wParam))
        {

        case IDD_HELP:
            goto DoHelp;

        case IDOK:
            HourGlass (TRUE);

            bWroteToSerialKey = FALSE;

            //
            //  Check IRQ line is valid
            //

            nVal = (int)SendDlgItemMessage(hDlg, PORT_IRQ, CB_GETCURSEL, 0, 0L);

            if ((nVal == CB_ERR) || (nVal == CB_ERRSPACE))
                nVal = nStartIndex;

            //
            //  Check Base I/O Address is valid
            //

            i = GetDlgItemText (hDlg, PORT_BASEIO, szBaseIO, CharSizeOf(szBaseIO));
            LoadString (hModule, MYPORT, szDef, CharSizeOf(szDef));

            if (i == 0)
                lstrcpy (szBaseIO, szStartString);

            bNone = !_tcsnicmp (szDef, szBaseIO, CharSizeOf(szBaseIO)-1);

            if (!bNone && !CheckBaseIOSetting (szBaseIO))
            {
                LoadString (hModule, MYPORT + 11, szFormat, CharSizeOf(szFormat));
                GetWindowText (hDlg, szTitle, CharSizeOf(szTitle));
                MessageBox (hDlg, szFormat, szTitle, MB_OK | MB_ICONINFORMATION);
                break;
            }

            //
            //  Get FIFO selection
            //

            dwFIFO = IsDlgButtonChecked (hDlg, PORT_FIFO);

            //
            //  Get the COM port number from the edit box
            //

            i = GetDlgItemText (hDlg, PORT_NUMBER, szCom, CharSizeOf(szCom));

            NewCom = (i == 0) ? nPortNumber : myatoi (szCom);

            //////////////////////////////////////////////////////////////////
            // Check to see if we've changed anything.  If so,
            //      - Output new values to registry
            //      - check and save the stuff in the parent dialog,
            //      - and issue a restart warning.
            //

            // ASSERT(hkeySerial != NULL);

            if (bNewPort ||
                nVal != nStartIndex ||
                nPortNumber != NewCom ||
                nFifo != dwFIFO ||
                _tcsnicmp (szStartString, szBaseIO, CharSizeOf(szBaseIO)-1))
            {
                //
                //  Output Base I/O to Registry
                //
                //  If I write anything out to registry for this port
                //  I must also, always write the Port Address, IRQ and
                //  DosDevices name.  For COM1-COM4 this means converting
                //  from the "default" value to the real default Base I/O
                //  address.
                //

                if (bNone)
                {
                    //
                    //  Convert "Default" value to a real I/O address for
                    //  storage in registry - failsafe case uses COM1 I/O
                    //

                    if ((NewCom > 0) && (NewCom < 5))
                        dwBaseIO = _tcstoul (pszBaseIO[NewCom-1], &pszTemp, 16);
                    else 
                        dwBaseIO = _tcstoul (pszBaseIO[4], &pszTemp, 16);
                }
                else
                {
                    dwBaseIO = _tcstoul (szBaseIO, &pszTemp, 16);
                }

                dwSize   = sizeof(DWORD);

                if (RegSetValueEx (hkeySerial, szRegPortAddress, 0L,
                                   REG_DWORD, (LPBYTE) &dwBaseIO, dwSize))
                    goto RegistryError;

                bWroteToSerialKey = TRUE;

                //
                //  Set FIFO value
                //

                dwSize = sizeof(DWORD);

                if (RegSetValueEx (hkeySerial, szFIFO, 0L, REG_DWORD,
                                                (LPBYTE) &dwFIFO, dwSize))
                    goto RegistryError;

                //
                //  Create global string from new port (without ":")
                //

                wsprintf (szComPort, TEXT("%s%d"), szCOM, NewCom);

                //
                //  Set "DosDevices" entry while we are here
                //

                if (RegSetValueEx (hkeySerial, szDosDev, 0L, REG_SZ,
                       (LPBYTE) szComPort, ByteCountOf(lstrlen(szComPort)+1)))
                {
                    //
                    //  Append ":" char to end of szComPort string
                    //

                    lstrcat (szComPort, TEXT(":"));

                    goto RegistryError;
                }

                //
                //  Try to create registry Devicemap entry
                //

                if (RegCreateKey (HKEY_LOCAL_MACHINE, szRegSerialMap,
                                  &hkey) == ERROR_SUCCESS)
                {
                    if (RegSetValueEx (hkey, szSerialKey, 0L, REG_SZ,
                           (LPBYTE) szComPort, ByteCountOf(lstrlen (szComPort)+1)) != ERROR_SUCCESS)
                    {
                        RegCloseKey (hkey);
                        goto ComCreateError;
                    }
                }
                else
                {
ComCreateError:
                    //
                    //  Append ":" char to end of szComPort string
                    //

                    lstrcat (szComPort, TEXT(":"));
                    MyMessageBox (hDlg, MYPORT+17, INITS+1,
                                               MB_OK | MB_ICONINFORMATION);
                    break;
                }

                //
                //  Append ":" char to end of szComPort string
                //

                lstrcat (szComPort, TEXT(":"));
                RegCloseKey (hkey);

                //
                //  Output IRQ line to Registry
                //

                if (bNewPort || nVal > 0)
                {
                    //
                    //  Since we don't display "Default" string at NewPort time
                    //  the equation for IRQ is different
                    //

                    dwBaseIRQ = (bNewPort ? IRQMAX : IRQMAX+1) - nVal;
                }
                else if (nVal == 0)
                {
                    //
                    //  Replace the Default setting with correct IRQ for port
                    //

                    switch (NewCom)
                    {
                    case 1:
                    case 3:
                        dwBaseIRQ = 4;
                        break;

                    case 2:
                    case 4:
                        dwBaseIRQ = 3;
                        break;

                    default:
                        //
                        //  Failsafe
                        //

                        dwBaseIRQ = 15;
                        break;
                    }
                }

                dwSize   = sizeof(DWORD);

                if (RegSetValueEx (hkeySerial, szRegPortIRQ, 0L,
                                   REG_DWORD, (LPBYTE) &dwBaseIRQ, dwSize))
                {
RegistryError:
                    //
                    //  ERROR - Cannot set Registry value
                    //

                    MyMessageBox (hDlg, MYPORT+18, INITS+1,
                                            MB_OK | MB_ICONINFORMATION);
                    goto AdvancedFailure;
                }

                if (bNewPort)
                {
                    //
                    //  Write default Baud rate string to registry "9600,n,8,1"
                    //

                    WriteProfileString (szPorts, szComPort, szDefParams);
                }
                else
                {
                    //
                    //  Check params in "Baud rate" dlg
                    //

                    hParent = GetParent (hDlg);

                    if (!ChkCommSettings (hParent))
                    {
                        SetFocus (hDlg);
                        break;
                    }

                    //
                    //  Output normal comm settings to WIN.INI
                    //

                    CommPortsToWin (hParent);

                    //
                    //  Change titlebar in parent dlg if COM Port Number changed
                    //

                    if (nPortNumber != NewCom)
                        bResetTitle = TRUE;
                }

                //
                //  Issue a restart warning
                //

                DialogBoxParam (hModule,(LPTSTR) MAKEINTRESOURCE(DLG_RESTART),hDlg,
                              (DLGPROC) RestartDlg, MAKELONG(IDS_COMCHANGE, 0));
            }

            RegCloseKey (hkeySerial);

            //
            //  Delete the Serial Key entry I created at InitDlg time in the
            //  Services\Parameters\Serial registry node only if I really
            //  created it AND I did not write anything to it at ID_OK time.
            //
            //  Note:  This is for the brain dead serial driver that doesn't
            //         know how to ignore a key that has no value entries
            //         and instead reports bogus EventLog ERRORS.
            //

            if ((dwDisposition == REG_CREATED_NEW_KEY) && !bWroteToSerialKey)
            {
                if (!RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                                   szRegSerialParam,
                                   0L,
                                   KEY_ALL_ACCESS,
                                   &hkey))
                {
                    RegDeleteKey (hkey, szSerialKey);
                    RegCloseKey (hkey);
                }
            }


            HourGlass (FALSE);

            EndDialog (hDlg, 1);
            break;

        case IDCANCEL:
AdvancedFailure:
            if (bNewPort)
            {
                //
                // "Cancel" btn selected, delete Parameters entry
                //

                if (hkeySerial != NULL)
                {
                    RegCloseKey (hkeySerial);

                    if (!RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                                       szRegSerialParam,
                                       0L,
                                       KEY_ALL_ACCESS,
                                       &hkey))
                    {
                        RegDeleteKey (hkey, szSerialKey);
                        RegCloseKey (hkey);
                    }
                }
            }
            else if (hkeySerial != NULL)
            {
                RegCloseKey (hkeySerial);

                //
                //  Delete Parameters entry in other cases only if I initially
                //  created the key at InitDlg time.
                //

                if (dwDisposition == REG_CREATED_NEW_KEY)
                {
                    if (!RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                                       szRegSerialParam,
                                       0L,
                                       KEY_ALL_ACCESS,
                                       &hkey))
                    {
                        RegDeleteKey (hkey, szSerialKey);
                        RegCloseKey (hkey);
                    }
                }
            }


            HourGlass (FALSE);

            EndDialog (hDlg, 0);
            break;

        default:
            return FALSE;
        }
        break;

    default:
        if (nMsg == wHelpMessage)
        {
DoHelp:
            CPHelp(hDlg);
            return TRUE;
        }
        else
            return FALSE;
    }

    return TRUE;
}

