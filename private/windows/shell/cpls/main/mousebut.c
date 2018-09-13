/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    mousebut.c

Abstract:

    This module contains the routines for the Mouse Buttons Property Sheet
    page.

Revision History:

--*/



//
//  Include Files.
//

#include "main.h"
#include "util.h"
#include "rc.h"
#include "mousectl.h"
#include <regstr.h>
#include <winerror.h>        // needed for ERROR_SUCCESS value
#include "mousehlp.h"




//
//  Global Variables.
//

const TCHAR szYes[]          = TEXT("yes");
const TCHAR szNo[]           = TEXT("no");
const TCHAR szDblClkSpeed[]  = TEXT("DoubleClickSpeed");
const TCHAR szRegStr_Mouse[] = REGSTR_PATH_MOUSE;




//
//  Constant Declarations.
//

//
//  SwapMouseButtons takes:
//      TRUE to make it a right mouse
//      FALSE to make it a left mouse
//
#define RIGHT       TRUE
#define LEFT        FALSE

#define CLICKMIN    100      // milliseconds
#define CLICKMAX    900
#define CLICKSUM    (CLICKMIN + CLICKMAX)
#define CLICKRANGE  (CLICKMAX - CLICKMIN)

//
// From shell\inc\shsemip.h
//
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))
#define Assert(f)


//
//  Typedef Declarations.
//

typedef struct tag_MouseGenStr
{
    BOOL bSwap;
    BOOL bOrigSwap;

    short ClickSpeed;
    short OrigDblClkSpeed;

    HWND hWndDblClkScroll;
    HWND hDlg;                    // HWND hMouseButDlg;

    RECT OrigRect;
    RECT DblClkRect;

    POINT ObjectPoint;
    POINT SelectPoint;

    int jackstate, jackhits;      // for jack-in-the-box control
    int jackignore;               // for jack-in-the-box control

    BOOL bShellSingleClick,
         bOrigShellSingleClick ;

    HICON hIconSglClick,
          hIconDblClick ;

} MOUSEBUTSTR, *PMOUSEBUTSTR, *LPMOUSEBUTSTR;

//
//  Context Help Ids.
//

const DWORD aMouseButHelpIds[] =
{
    IDC_GROUPBOX_1,     IDH_COMM_GROUPBOX,
    IDC_GROUPBOX_2,     IDH_DLGMOUSE_HANDED_PIC,
    IDC_GROUPBOX_3,     IDH_DLGMOUSE_HANDED_PIC,
    IDC_GROUPBOX_4,     IDH_COMM_GROUPBOX,
    IDC_SELECTDRAG,     IDH_DLGMOUSE_HANDED_PIC,
    IDC_OBJECTMENU,     IDH_DLGMOUSE_HANDED_PIC,
    MOUSE_LEFTHAND,     IDH_DLGMOUSE_LEFT,
    MOUSE_RIGHTHAND,    IDH_DLGMOUSE_RIGHT,
    MOUSE_SELECTBMP,    IDH_DLGMOUSE_HANDED_PIC,
    MOUSE_MOUSEBMP,     IDH_DLGMOUSE_HANDED_PIC,
    MOUSE_MENUBMP,      IDH_DLGMOUSE_HANDED_PIC,
    MOUSE_CLICKSCROLL,  IDH_DLGMOUSE_DOUBCLICK,
    IDC_GROUPBOX_5,     IDH_DLGMOUSE_DCLICK_PIC,
    MOUSE_DBLCLKBMP,    IDH_DLGMOUSE_DCLICK_PIC,
    MOUSE_SGLCLICK,     IDH_MOUSE_SGLCLICK,
    MOUSE_DBLCLICK,     IDH_MOUSE_DBLCLICK,
    0, 0
};


//
//  helper function prototypes
//
void ShellClick_UpdateUI( HWND hDlg, PMOUSEBUTSTR pMstr) ;
void ShellClick_Refresh( PMOUSEBUTSTR pMstr ) ;


//
//  Debug Info.
//

#ifdef DEBUG

#define REG_INTEGER  1000

int fTraceRegAccess = 0;

void RegDetails(
    int     iWrite,
    HKEY    hk,
    LPCTSTR lpszSubKey,
    LPCTSTR lpszValueName,
    DWORD   dwType,
    LPTSTR  lpszString,
    int     iValue)
{
    TCHAR Buff[256];
    TCHAR *lpszReadWrite[] = { TEXT("DESK.CPL:Read"), TEXT("DESK.CPL:Write") };

    if (!fTraceRegAccess)
    {
        return;
    }

    switch (dwType)
    {
        case ( REG_SZ ) :
        {
            wsprintf( Buff,
                      TEXT("%s String:hk=%#08lx, %s:%s=%s\n\r"),
                      lpszReadWrite[iWrite],
                      hk,
                      lpszSubKey,
                      lpszValueName,
                      lpszString );
            break;
        }
        case ( REG_INTEGER ) :
        {
            wsprintf( Buff,
                      TEXT("%s int:hk=%#08lx, %s:%s=%d\n\r"),
                      lpszReadWrite[iWrite],
                      hk,
                      lpszSubKey,
                      lpszValueName,
                      iValue );
            break;
        }
        case ( REG_BINARY ) :
        {
            wsprintf( Buff,
                      TEXT("%s Binary:hk=%#08lx, %s:%s=%#0lx;DataSize:%d\r\n"),
                      lpszReadWrite[iWrite],
                      hk,
                      lpszSubKey,
                      lpszValueName,
                      lpszString,
                      iValue );
            break;
        }
    }
    OutputDebugString(Buff);
}

#endif  // DEBUG





////////////////////////////////////////////////////////////////////////////
//
//  GetIntFromSubKey
//
//  hKey is the handle to the subkey (already pointing to the proper
//  location.
//
////////////////////////////////////////////////////////////////////////////

int GetIntFromSubkey(
    HKEY hKey,
    LPCTSTR lpszValueName,
    int iDefault)
{
    TCHAR szValue[20];
    DWORD dwSizeofValueBuff = sizeof(szValue);
    DWORD dwType;
    int iRetValue = iDefault;

    if ((RegQueryValueEx( hKey,
                          (LPTSTR)lpszValueName,
                          NULL,
                          &dwType,
                          (LPBYTE)szValue,
                          &dwSizeofValueBuff ) == ERROR_SUCCESS) &&
        (dwSizeofValueBuff))
    {
        //
        //  BOGUS: This handles only the string type entries now!
        //
        if (dwType == REG_SZ)
        {
            iRetValue = (int)StrToLong(szValue);
        }
#ifdef DEBUG
        else
        {
            OutputDebugString(TEXT("String type expected from Registry\n\r"));
        }
#endif
    }

#ifdef DEBUG
    RegDetails(0, hKey, TEXT(""), lpszValueName, REG_INTEGER, NULL, iRetValue);
#endif

    return (iRetValue);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetIntFromReg
//
//  Opens the given subkey and gets the int value.
//
////////////////////////////////////////////////////////////////////////////

int GetIntFromReg(
    HKEY hKey,
    LPCTSTR lpszSubkey,
    LPCTSTR lpszNameValue,
    int iDefault)
{
    HKEY hk;
    int iRetValue = iDefault;

    //
    //  See if the key is present.
    //
    if (RegOpenKey(hKey, lpszSubkey, &hk) == ERROR_SUCCESS)
    {
        iRetValue = GetIntFromSubkey(hk, lpszNameValue, iDefault);
        RegCloseKey(hk);
    }

    return (iRetValue);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetStringFromReg
//
//  Opens the given subkey and gets the string value.
//
////////////////////////////////////////////////////////////////////////////

BOOL GetStringFromReg(
    HKEY hKey,
    LPCTSTR lpszSubkey,
    LPCTSTR lpszValueName,
    LPCTSTR lpszDefault,
    LPTSTR lpszValue,
    DWORD dwSizeofValueBuff)
{
    HKEY hk;
    DWORD dwType;
    BOOL fSuccess = FALSE;

    //
    //  See if the key is present.
    //
    if (RegOpenKey(hKey, lpszSubkey, &hk) == ERROR_SUCCESS)
    {
        if ((RegQueryValueEx( hk,
                              (LPTSTR)lpszValueName,
                              NULL,
                              &dwType,
                              (LPBYTE)lpszValue,
                              &dwSizeofValueBuff ) == ERROR_SUCCESS) &&
            (dwSizeofValueBuff))
        {
            //
            //  BOGUS: This handles only the string type entries now!
            //
#ifdef DEBUG
            if (dwType != REG_SZ)
            {
                OutputDebugString(TEXT("String type expected from Registry\n\r"));
            }
            else
#endif
            fSuccess = TRUE;
        }
        RegCloseKey(hk);
    }

    //
    //  If failure, use the default string.
    //
    if (!fSuccess)
    {
        lstrcpy(lpszValue, lpszDefault);
    }

#ifdef DEBUG
    RegDetails(0, hKey, lpszSubkey, lpszValueName, REG_SZ, lpszValue, 0);
#endif

    return (fSuccess);
}


////////////////////////////////////////////////////////////////////////////
//
//  UpdateRegistry
//
//  This updates a given value of any data type at a given location in
//  the registry.
//
//  The value name is passed in as an Id to a string in USER's String
//  table.
//
////////////////////////////////////////////////////////////////////////////

BOOL UpdateRegistry(
    HKEY hKey,
    LPCTSTR lpszSubkey,
    LPCTSTR lpszValueName,
    DWORD dwDataType,
    LPVOID lpvData,
    DWORD dwDataSize)
{
    HKEY hk;

    if (RegCreateKey(hKey, lpszSubkey, &hk) == ERROR_SUCCESS)
    {
        RegSetValueEx( hk,
                       (LPTSTR)lpszValueName,
                       0,
                       dwDataType,
                       (LPBYTE)lpvData,
                       dwDataSize );
#ifdef DEBUG
        RegDetails(1, hKey, lpszSubkey, lpszValueName, dwDataType, lpvData, (int)dwDataSize);
#endif

        RegCloseKey(hk);
        return (TRUE);
    }

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  ShowButtonState
//
//  Swaps the menu and selection bitmaps.
//
////////////////////////////////////////////////////////////////////////////

void ShowButtonState(
    PMOUSEBUTSTR pMstr)
{
    HWND hDlg;

    Assert(pMstr);

    hDlg = pMstr->hDlg;

    MouseControlSetSwap(GetDlgItem(hDlg, MOUSE_MOUSEBMP), pMstr->bSwap);

    SetWindowPos( GetDlgItem(hDlg, pMstr->bSwap ? IDC_SELECTDRAG : IDC_OBJECTMENU),
                  NULL,
                  pMstr->ObjectPoint.x,
                  pMstr->ObjectPoint.y,
                  0,
                  0,
                  SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER );

    SetWindowPos( GetDlgItem(hDlg, !pMstr->bSwap ? IDC_SELECTDRAG : IDC_OBJECTMENU),
                  NULL,
                  pMstr->SelectPoint.x,
                  pMstr->SelectPoint.y,
                  0,
                  0,
                  SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER );

    CheckDlgButton(hDlg, MOUSE_RIGHTHAND, !pMstr->bSwap);
    CheckDlgButton(hDlg, MOUSE_LEFTHAND, pMstr->bSwap);

    CheckDlgButton(hDlg, MOUSE_SGLCLICK, pMstr->bShellSingleClick);
    CheckDlgButton(hDlg, MOUSE_DBLCLICK, !pMstr->bShellSingleClick);
}


////////////////////////////////////////////////////////////////////////////
//
//  DestroyMouseButDlg
//
////////////////////////////////////////////////////////////////////////////
#define SAFE_DESTROYICON(hicon)   if (hicon) { DestroyIcon(hicon); hicon=NULL; }

void DestroyMouseButDlg(
    PMOUSEBUTSTR pMstr)
{
    if (pMstr)
    {
        SAFE_DESTROYICON( pMstr->hIconSglClick ) ;
        SAFE_DESTROYICON( pMstr->hIconDblClick )

        SetWindowLongPtr(pMstr->hDlg, DWLP_USER, 0);
        LocalFree((HGLOBAL)pMstr);
    }
}



////////////////////////////////////////////////////////////////////////////
//
//  Jack stuff -- code to manage the jack-in-the-box double-click testbox.
//
////////////////////////////////////////////////////////////////////////////

typedef struct
{
    int first;          // first frame of state
    int last;           // last frame of state
    int repeat;         // state's repeat count
} JACKSTATE;

#define JACK_OPPOSITE_STATE     (-2)
#define JACK_NEXT_STATE         (-1)

#define JACK_CRANKING           ( 0)
#define JACK_OPENING            ( 1)
#define JACK_BOUNCING           ( 2)
#define JACK_CLOSING            ( 3)

//
//  NOTE: Every state starts with a key frame in the AVI file.
//

JACKSTATE JackFSA[] =
{
    {  0,  7, -1 },     // cranking
    {  8, 12,  1 },     // opening
    { 16, 23, -1 },     // bouncing
    { 24, 27,  1 },     // closing
};

#define JACK_NUM_STATES     (sizeof(JackFSA) / sizeof(JACKSTATE))


////////////////////////////////////////////////////////////////////////////
//
//  JackSetState
//
////////////////////////////////////////////////////////////////////////////

void JackSetState(
    HWND dlg,
    PMOUSEBUTSTR mstr,
    int index)
{
    JACKSTATE *state;

    switch (index)
    {
        case ( JACK_OPPOSITE_STATE ) :
        {
            mstr->jackstate++;

            // fall thru...
        }
        case ( JACK_NEXT_STATE ) :
        {
            index = mstr->jackstate + 1;
        }
    }

    index %= JACK_NUM_STATES;

    state = JackFSA + index;

    mstr->jackstate = index;

    mstr->jackignore++;

    Animate_Play( GetDlgItem(dlg, MOUSE_DBLCLKBMP),
                  state->first,
                  state->last,
                  state->repeat );
}


////////////////////////////////////////////////////////////////////////////
//
//  JackClicked
//
////////////////////////////////////////////////////////////////////////////

void JackClicked(
    HWND dlg,
    PMOUSEBUTSTR mstr)
{
    JACKSTATE *state = JackFSA + mstr->jackstate;

    if (state->repeat < 0)
    {
        JackSetState(dlg, mstr, JACK_NEXT_STATE);
    }
    else
    {
        mstr->jackhits++;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  JackStopped
//
////////////////////////////////////////////////////////////////////////////

void JackStopped(
    HWND dlg,
    PMOUSEBUTSTR mstr)
{
    if (--mstr->jackignore < 0)
    {
        JACKSTATE *state = JackFSA + mstr->jackstate;

        if (state->repeat > 0)
        {
            int index;

            if (mstr->jackhits)
            {
                mstr->jackhits--;
                index = JACK_OPPOSITE_STATE;
            }
            else
            {
                index = JACK_NEXT_STATE;
            }

            JackSetState(dlg, mstr, index);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  JackStart
//
////////////////////////////////////////////////////////////////////////////

void JackStart(
    HWND dlg,
    PMOUSEBUTSTR mstr)
{
    mstr->jackhits = 0;
    mstr->jackstate = 0;
    mstr->jackignore = -1;

    Animate_Open( GetDlgItem(dlg, MOUSE_DBLCLKBMP),
                  MAKEINTRESOURCE(IDA_JACKNBOX) );

    JackSetState(dlg, mstr, JACK_CRANKING);
}

////////////////////////////////////////////////////////////////////////////
//
//  InitMouseButDlg
//
////////////////////////////////////////////////////////////////////////////

BOOL InitMouseButDlg(
    HWND hDlg)
{
    POINT        Origin;
    SHELLSTATE   shellstate = {0} ;
    PMOUSEBUTSTR pMstr;
    RECT         TmpRect;
    HINSTANCE    hInstDlg = (HINSTANCE)GetWindowLongPtr( hDlg, GWLP_HINSTANCE ) ;

    pMstr = (PMOUSEBUTSTR)LocalAlloc(LPTR , sizeof(MOUSEBUTSTR));

    if (pMstr == NULL)
    {
        return (TRUE);
    }

    SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pMstr);

    pMstr->hDlg = hDlg;

    Origin.x = Origin.y = 0;
    ClientToScreen(hDlg, (LPPOINT)&Origin);

    GetWindowRect(hDlg, &pMstr->OrigRect);

    pMstr->OrigRect.left -= Origin.x;
    pMstr->OrigRect.right -= Origin.x;

    GetWindowRect(GetDlgItem(hDlg, MOUSE_DBLCLKBMP), &pMstr->DblClkRect);

    pMstr->DblClkRect.top    -= Origin.y;
    pMstr->DblClkRect.bottom -= Origin.y;
    pMstr->DblClkRect.left   -= Origin.x;
    pMstr->DblClkRect.right  -= Origin.x;

    //
    //  Find position of Select and Object windows.
    //
    GetWindowRect(GetDlgItem(hDlg, IDC_SELECTDRAG), &TmpRect);

    ScreenToClient(hDlg, (POINT *)&TmpRect);

    pMstr->SelectPoint.x = TmpRect.left;
    pMstr->SelectPoint.y = TmpRect.top;

    GetWindowRect(GetDlgItem(hDlg, IDC_OBJECTMENU), &TmpRect);

    ScreenToClient(hDlg, (POINT *)&TmpRect);

    pMstr->ObjectPoint.x = TmpRect.left;
    pMstr->ObjectPoint.y = TmpRect.top;

    //
    //  Set (and get), then restore the state of the mouse buttons.
    //
    (pMstr->bOrigSwap) = (pMstr->bSwap) = SwapMouseButton(TRUE);

    SwapMouseButton(pMstr->bOrigSwap);

    //
    //  Get shell single-click behavior:
    //
    SHGetSetSettings( &shellstate, SSF_DOUBLECLICKINWEBVIEW | SSF_WIN95CLASSIC, FALSE /*get*/ ) ;
    pMstr->bShellSingleClick =
    pMstr->bOrigShellSingleClick =  shellstate.fWin95Classic ? FALSE :
                                    shellstate.fDoubleClickInWebView ? FALSE :
                                    TRUE ;
    pMstr->hIconSglClick = LoadIcon( hInstDlg, MAKEINTRESOURCE( IDI_SGLCLICK ) ) ;
    pMstr->hIconDblClick = LoadIcon( hInstDlg, MAKEINTRESOURCE( IDI_DBLCLICK ) ) ;
    ShellClick_UpdateUI( hDlg, pMstr ) ;

    //
    //  Initialize check/radio button state
    //
    ShowButtonState(pMstr);

    pMstr->OrigDblClkSpeed =
    pMstr->ClickSpeed = (SHORT) GetIntFromReg( HKEY_CURRENT_USER,
                                       szRegStr_Mouse,
                                       szDblClkSpeed,
                                       CLICKMIN + (CLICKRANGE / 2) );

    pMstr->hWndDblClkScroll = GetDlgItem(hDlg, MOUSE_CLICKSCROLL);

    SendMessage( pMstr->hWndDblClkScroll,
                 TBM_SETRANGE,
                 0,
                 MAKELONG(CLICKMIN, CLICKMAX) );
    SendMessage( pMstr->hWndDblClkScroll,
                 TBM_SETPOS,
                 TRUE,
                 (LONG)(CLICKMIN + CLICKMAX - pMstr->ClickSpeed) );

    SetDoubleClickTime(pMstr->ClickSpeed);

    JackStart(hDlg, pMstr);

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  MouseButDlg
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK MouseButDlg(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    PMOUSEBUTSTR pMstr = (PMOUSEBUTSTR)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (message)
    {
        case ( WM_INITDIALOG ) :
        {
            return (InitMouseButDlg(hDlg));
        }
        case ( WM_DESTROY ) :
        {
            DestroyMouseButDlg(pMstr);
            break;
        }
        case ( WM_HSCROLL ) :
        {
            if ((HWND)lParam == pMstr->hWndDblClkScroll)
            {
                short temp = CLICKMIN + CLICKMAX -
                             (short)SendMessage( (HWND)lParam,
                                                 TBM_GETPOS,
                                                 0,
                                                 0L );

                if (temp != pMstr->ClickSpeed)
                {
                    pMstr->ClickSpeed = temp;

                    SetDoubleClickTime(pMstr->ClickSpeed);

                    SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
                }
            }
            break;
        }
        case ( WM_RBUTTONDBLCLK ) :
        case ( WM_LBUTTONDBLCLK ) :
        {
            POINT point = { (int)MAKEPOINTS(lParam).x,
                            (int)MAKEPOINTS(lParam).y };

            if (PtInRect(&pMstr->DblClkRect, point))
            {
                JackClicked(hDlg, pMstr);
            }
            break;
        }
        case ( WM_COMMAND ) :
        {
            switch (LOWORD(wParam))
            {
                case ( MOUSE_LEFTHAND ) :
                {
                    if (pMstr->bSwap != RIGHT)
                    {
                        SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);

                        pMstr->bSwap = RIGHT;

                        ShowButtonState(pMstr);
                    }
                    break;
                }
                case ( MOUSE_RIGHTHAND ) :
                {
                    if (pMstr->bSwap != LEFT)
                    {
                        SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);

                        pMstr->bSwap = LEFT;

                        ShowButtonState(pMstr);
                    }
                    break;
                }
                case ( MOUSE_DBLCLKBMP ) :
                {
                    if (HIWORD(wParam) == ACN_STOP)
                    {
                        JackStopped(hDlg, pMstr);
                    }
                    break;
                }
                case ( MOUSE_SGLCLICK ) :
                case ( MOUSE_DBLCLICK ) :
                {
                    if( pMstr->bShellSingleClick != (MOUSE_SGLCLICK == LOWORD(wParam)) )
                    {
                        pMstr->bShellSingleClick = (MOUSE_SGLCLICK == LOWORD(wParam)) ;
                        ShellClick_UpdateUI( hDlg, pMstr ) ;
                        SendMessage( GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L ) ;
                    }
                    break ;
                }


            }
            break;
        }
        case ( WM_NOTIFY ) :
        {
            switch (((NMHDR *)lParam)->code)
            {
                case ( PSN_APPLY ) :
                {
                    HourGlass(TRUE);

                    if (pMstr->bSwap != pMstr->bOrigSwap)
                    {
                        SystemParametersInfo( SPI_SETMOUSEBUTTONSWAP,
                                              pMstr->bSwap,
                                              NULL,
                                              SPIF_UPDATEINIFILE |
                                                SPIF_SENDWININICHANGE );

                        pMstr->bOrigSwap = pMstr->bSwap;
                    }

                    if (pMstr->ClickSpeed != pMstr->OrigDblClkSpeed)
                    {
                        SystemParametersInfo( SPI_SETDOUBLECLICKTIME,
                                              pMstr->ClickSpeed,
                                              NULL,
                                              SPIF_UPDATEINIFILE |
                                                SPIF_SENDWININICHANGE );

                        pMstr->OrigDblClkSpeed = pMstr->ClickSpeed;
                    }


                    if( pMstr->bOrigShellSingleClick != pMstr->bShellSingleClick )
                    {
                        SHELLSTATE shellstate = {0} ;
                        ULONG      dwFlags = SSF_DOUBLECLICKINWEBVIEW ;

                        shellstate.fWin95Classic =
                        shellstate.fDoubleClickInWebView = !pMstr->bShellSingleClick ;

                        // update the WIN95CLASSIC member only if we've chosen single-click.
                        if( pMstr->bShellSingleClick )
                            dwFlags |= SSF_WIN95CLASSIC ;

                        SHGetSetSettings( &shellstate, dwFlags, TRUE ) ;
                        ShellClick_Refresh( pMstr ) ;

                        pMstr->bOrigShellSingleClick = pMstr->bShellSingleClick ;
                    }

                    HourGlass(FALSE);
                    break;
                }
                case ( PSN_RESET ) :
                {
                    SetDoubleClickTime(pMstr->OrigDblClkSpeed);
                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        case ( WM_HELP ) :             // F1
        {
            WinHelp( ((LPHELPINFO)lParam)->hItemHandle,
                     HELP_FILE,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aMouseButHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :      // right mouse click
        {
            WinHelp( (HWND) wParam,
                     HELP_FILE,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aMouseButHelpIds );
            break;
        }

        case ( WM_DISPLAYCHANGE ) :
        case ( WM_WININICHANGE ) :
        case ( WM_SYSCOLORCHANGE ) :
        {
            SHPropagateMessage(hDlg, message, wParam, lParam, TRUE);
            return TRUE;
        }

        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  ShellClick_UpdateUI
//
//  Assigns the appropriate icon for shell single/double click
//
////////////////////////////////////////////////////////////////////////////
void ShellClick_UpdateUI(
    HWND hDlg,
    PMOUSEBUTSTR pMstr)
{
    HICON hicon = pMstr->bShellSingleClick ? pMstr->hIconSglClick :
                                             pMstr->hIconDblClick ;

    SendMessage( GetDlgItem( hDlg, MOUSE_CLICKICON ), STM_SETICON,
                 (WPARAM)hicon, 0L ) ;
}

////////////////////////////////////////////////////////////////////////////
//
//  IsShellWindow
//
//  Determines whether the specified window is a shell folder window.
//
////////////////////////////////////////////////////////////////////////////
#define c_szExploreClass TEXT("ExploreWClass")
#define c_szIExploreClass TEXT("IEFrame")
#ifdef IE3CLASSNAME
#define c_szCabinetClass TEXT("IEFrame")
#else
#define c_szCabinetClass TEXT("CabinetWClass")
#endif

BOOL IsShellWindow( HWND hwnd )
{
    TCHAR szClass[32];

    GetClassName(hwnd, szClass, ARRAYSIZE(szClass));
    return (lstrcmp(szClass, c_szCabinetClass) == 0) ||
           (lstrcmp(szClass, c_szExploreClass) == 0) ||
           (lstrcmp(szClass, c_szIExploreClass) == 0) ;
}

//The following value is taken from shdocvw\rcids.h
#ifndef FCIDM_REFRESH
#define FCIDM_REFRESH  0xA220
#endif // FCIDM_REFRESH

////////////////////////////////////////////////////////////////////////////
//
//  ShellClick_RefreshEnumProc
//
//  EnumWindow callback for shell refresh.
//
////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK ShellClick_RefreshEnumProc( HWND hwnd, LPARAM lParam )
{
    if( IsShellWindow(hwnd) )
        PostMessage(hwnd, WM_COMMAND, FCIDM_REFRESH, 0L);

    return(TRUE);
}

////////////////////////////////////////////////////////////////////////////
//
//  ShellClick_Refresh
//
//  Re-renders the contents of all shell folder windows.
//
////////////////////////////////////////////////////////////////////////////
void ShellClick_Refresh( PMOUSEBUTSTR pMstr )
{
    HWND hwndDesktop = FindWindowEx(NULL, NULL, TEXT(STR_DESKTOPCLASS), NULL);

    if( NULL != hwndDesktop )
       PostMessage( hwndDesktop, WM_COMMAND, FCIDM_REFRESH, 0L );

    EnumWindows( ShellClick_RefreshEnumProc, 0L ) ;
}
