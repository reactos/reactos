//****************************************************************************
//
//  Module:     RNAUI.DLL
//  File:       utils.c
//  Content:    This file contains miscellaneous utility routines.
//  History:
//      Thu 08-Apr-1993 09:43:46  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1991-1994
//
//****************************************************************************

#include "rnaui.h"

#pragma data_seg(DATASEG_READONLY)
char const c_szRunDLLProcess[] = "RunDll32.exe ";
#pragma data_seg()

//****************************************************************************
// int NEAR PASCAL RuiUserMessage (HWND, UINT, UINT)
//
// This generic function display the immediate information to the user.
//
// History:
//  Thu 25-Mar-1993 16:24:26  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

int NEAR PASCAL RuiUserMessage(HWND hWnd, UINT idMsg, UINT fuStyle)
{
  return (ShellMessageBox(ghInstance, hWnd, (LPCSTR)MAKELONG(idMsg, 0),
                          (LPCSTR)MAKELONG(IDS_CAP_REMOTE, 0), fuStyle | MB_SETFOREGROUND));
}

//****************************************************************************
// int NEAR PASCAL ShortenName (LPSTR, LPSTR, DWORD)
//
// This generic function shortens a name to fit the specified size.
//
// History:
//  Wed 06-Apr-1994 13:07:39  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************


void NEAR PASCAL ShortenName(LPSTR szLongName, LPSTR szShortName, DWORD cbShort)
{
  static BOOL    gfShortFmt  = FALSE;
  static char    g_szShortFmt[MAXNAME];
  static DWORD   gdwShortFmt = 0;

  // Get the shorten format
  if (!gfShortFmt)
  {
    gdwShortFmt  = LoadString(ghInstance, IDS_SHORT_NAME, g_szShortFmt,
                              sizeof(g_szShortFmt));
    gdwShortFmt -= 2;  // lstrlen("%s")
    gfShortFmt   = TRUE;
  };

  // Check the size of the long name
  if ((DWORD)lstrlen(szLongName)+1 <= cbShort)
  {
    // The name is shorter than the specified size, copy back the name
    lstrcpy(szShortName, szLongName);
  }
  else
  {
    LPSTR szShorten;

    // The name is longer than the specified size, adjust the name
    if ((szShorten = (LPSTR)LocalAlloc(LMEM_FIXED, cbShort)) != NULL)
    {
      lstrcpyn(szShorten, szLongName, cbShort-gdwShortFmt);
      wsprintf(szShortName, g_szShortFmt, szShorten);
      LocalFree((HLOCAL)szShorten);
    }
    else
    {
      lstrcpyn(szShortName, szLongName, cbShort);
    };
  };
}

//****************************************************************************
// BOOL NEAR PASCAL IsServerInstalled ()
//
// This function quickly checks for the server installation
//
// History:
//  Sat 29-Oct-1994 10:30:21  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

BOOL NEAR PASCAL IsServerInstalled ()
{
  OFSTRUCT    ofs;
  char        szTemp[ MAXPATHLEN ] ;

  // Get the full pathname
  //
  if (!GetSystemDirectory(szTemp, sizeof(szTemp)))
    return (FALSE);

  lstrcat(szTemp, c_szDialUpServerFile);

  // Look for the server module
  //
  if ( OpenFile( szTemp, &ofs, OF_EXIST ) == HFILE_ERROR )
      return ( FALSE ) ;
  else
      return ( TRUE ) ;

}

// Some of these are replacements for the C runtime routines.
//  This is so we don't have to link to the CRT libs.
//

/*----------------------------------------------------------
Purpose: memset

         Swiped from the C 7.0 runtime sources.

Returns: 
Cond:    
*/
LPSTR PUBLIC lmemset(
    LPSTR dst,
    char val,
    UINT count)
    {
    LPSTR start = dst;
    
    while (count--)
        *dst++ = val;
    return(start);
    }


/*----------------------------------------------------------
Purpose: memmove

         Swiped from the C 7.0 runtime sources.

Returns: 
Cond:    
*/
LPSTR PUBLIC lmemmove(
    LPSTR dst, 
    LPSTR src, 
    int count)
    {
    LPSTR ret = dst;
    
    if (dst <= src || dst >= (src + count)) {
        /*
         * Non-Overlapping Buffers
         * copy from lower addresses to higher addresses
         */
        while (count--)
            *dst++ = *src++;
        }
    else {
        /*
         * Overlapping Buffers
         * copy from higher addresses to lower addresses
         */
        dst += count - 1;
        src += count - 1;
        
        while (count--)
            *dst-- = *src--;
        }
    
    return(ret);
    }


/*----------------------------------------------------------
Purpose: My verion of atoi.  Supports hexadecimal too.
Returns: integer
Cond:    --
*/
int PUBLIC AnsiToInt(
    LPCSTR pszString)
    {
    int n;
    BOOL bNeg = FALSE;
    LPCSTR psz;
    LPCSTR pszAdj;

    // Skip leading whitespace
    //
    for (psz = pszString; *psz == ' ' || *psz == '\n' || *psz == '\t'; psz = AnsiNext(psz))
        ;
      
    // Determine possible explicit signage
    //  
    if (*psz == '+' || *psz == '-')
        {
        bNeg = (*psz == '+') ? FALSE : TRUE;
        psz = AnsiNext(psz);
        }

    // Or is this hexadecimal?
    //
    pszAdj = AnsiNext(psz);
    if (*psz == '0' && (*pszAdj == 'x' || *pszAdj == 'X'))
        {
        bNeg = FALSE;   // Never allow negative sign with hexadecimal numbers
        psz = AnsiNext(pszAdj);

        // Do the conversion
        //
        for (n = 0; ; psz = AnsiNext(psz))
            {
            if (*psz >= '0' && *psz <= '9')
                n = 0x10 * n + *psz - '0';
            else
                {
                char ch = *psz;
                int n2;

                if (ch >= 'a')
                    ch -= 'a' - 'A';

                n2 = ch - 'A' + 0xA;
                if (n2 >= 0xA && n2 <= 0xF)
                    n = 0x10 * n + n2;
                else
                    break;
                }
            }
        }
    else
        {
        for (n = 0; *psz >= '0' && *psz <= '9'; psz = AnsiNext(psz))
            n = 10 * n + *psz - '0';
        }

    return bNeg ? -n : n;
    }    


/*----------------------------------------------------------
Purpose: General front end to invoke dialog boxes
Returns: result from EndDialog
Cond:    --
*/
int PUBLIC DoModal(
    HWND hwndParent,            // owner of dialog
    DLGPROC lpfnDlgProc,        // dialog proc
    UINT uID,                   // dialog template ID
    LPARAM lParam)              // extra parm to pass to dialog (may be NULL)
    {
    int nResult = -1;

    nResult = DialogBoxParam(ghInstance, MAKEINTRESOURCE(uID), hwndParent,
                             lpfnDlgProc, lParam);

    return nResult;
    }


//---------------------------------------------------------------------------
// Menu munging code.  Taken from WINUTILS.
//---------------------------------------------------------------------------

// REVIEW: We need this function because current version of USER.EXE does
//  not support pop-up only menu.
//
HMENU PUBLIC LoadPopupMenu(UINT id, UINT uSubOffset)
    {
    HMENU hmParent, hmPopup;

    hmParent = LoadMenu(ghInstance, MAKEINTRESOURCE(id));
    if (!hmParent)
        {
        return(NULL);
        }

    hmPopup = GetSubMenu(hmParent, uSubOffset);
    RemoveMenu(hmParent, uSubOffset, MF_BYPOSITION);
    DestroyMenu(hmParent);

    return(hmPopup);
    }


UINT PUBLIC MergePopupMenu(
    HMENU FAR *phMenu, 
    UINT idResource, 
    UINT uSubOffset,
    UINT indexMenu, 
    UINT idCmdFirst, 
    UINT idCmdLast)
    {
    HMENU hmMerge;

    //
    // Create a popup menu, if it is not given.
    //
    if (*phMenu == NULL)
        {
        *phMenu = CreatePopupMenu();
        if (*phMenu == NULL)
            {
            return(0);
            }

        indexMenu = 0;    // at the bottom
        }

    hmMerge = LoadPopupMenu(idResource, uSubOffset);
    if (!hmMerge)
        {
        return(0);
        }

    idCmdLast = Shell_MergeMenus(*phMenu, hmMerge, indexMenu, 
                idCmdFirst, idCmdLast, MM_ADDSEPARATOR);
    
    DestroyMenu(hmMerge);
    return idCmdLast;
    }


HMENU PUBLIC GetMenuFromID(HMENU hmMain, UINT uID)
    {
    MENUITEMINFO miiSubMenu;

    if (!hmMain)
        {
        return(NULL);
        }

    miiSubMenu.cbSize = sizeof(MENUITEMINFO);
    miiSubMenu.fMask = MIIM_SUBMENU;
    if (!GetMenuItemInfo(hmMain, uID, FALSE, &miiSubMenu))
        {
        return(NULL);
        }

    return(miiSubMenu.hSubMenu);
    }


//---------------------------------------------------------------------------
// Thread creation routines.  (These routines were swiped from the shell.)
//
// Public API is RunDLLThread().
//---------------------------------------------------------------------------

// SHOpenPropSheet uses these messages.  It is unclear if
// we need to support this or not.  
#define STUBM_SETDATA   (WM_USER)
#define STUBM_GETDATA   (WM_USER+1)

typedef void (WINAPI FAR* NEWTHREADPROC)(HWND hwndStub,                      
        HINSTANCE hAppInstance,                                           
        LPSTR lpszCmdLine, int nCmdShow);                                 

typedef struct  // dlle
    {
    HINSTANCE  hinst;
    NEWTHREADPROC lpfn;
    char       szCmd[MAXNAME+1];
    } DLLENTRY;

typedef struct _NEWTHREAD_NOTIFY                                             
    {                                                                         
    NMHDR   hdr;                                                      
    HICON   hIcon;                                                    
    LPCSTR  lpszTitle;                                                
    } NEWTHREAD_NOTIFY;                                                          


/*----------------------------------------------------------
Purpose: Parses a command line entry into a DLLENTRY struct.

Returns: TRUE on success
Cond:    --
*/
BOOL PRIVATE InitializeDLLEntry(
    LPSTR lpszCmdLine, 
    DLLENTRY * pdlle)
    {
    LPSTR lpStart, lpEnd, lpFunction;

    TRACE_MSG(TF_GENERAL, "RunDLLThread(%s)", lpszCmdLine);

    for (lpStart=lpszCmdLine; ; )
        {
        // Skip leading blanks
        //
        while (*lpStart == ' ')
            {
            ++lpStart;
            }

        // Check if there are any switches
        //
        if (*lpStart != '/')
            {
            break;
            }

        // Look at all the switches; ignore unknown ones
        //
        for (++lpStart; ; ++lpStart)
            {
            switch (*lpStart)
                {
            case ' ':
            case '\0':
                goto EndSwitches;
                break;

            // Put any switches we care about here
            //

            default:
                break;
                }
            }
EndSwitches:
            ;
        }

    // We have found the DLL,FN parameter
    //
    lpEnd = StrChr(lpStart, ' ');
    if (lpEnd)
        {
        *lpEnd++ = '\0';
        }

    // There must be a DLL name and a function name
    //
    lpFunction = StrChr(lpStart, ',');
    if (!lpFunction)
        {
        return(FALSE);
        }
    *lpFunction++ = '\0';

    // Load the library and get the procedure address
    // Note that we try to get a module handle first, so we don't need
    // to pass full file names around
    //
    pdlle->hinst = GetModuleHandle(lpStart);
    if (pdlle->hinst)
        {
        char szName[MAXPATHLEN];

        GetModuleFileName(pdlle->hinst, szName, sizeof(szName));
        LoadLibrary(szName);
        }
    else
        {
        pdlle->hinst = LoadLibrary(lpStart);
        if (!ISVALIDHINSTANCE(pdlle->hinst))
            {
            return(FALSE);
            }
        }

    lstrcpyn(pdlle->szCmd, lpFunction, sizeof(pdlle->szCmd));
    pdlle->lpfn = (NEWTHREADPROC)GetProcAddress(pdlle->hinst, lpFunction);
    if (!pdlle->lpfn)
        {
        FreeLibrary(pdlle->hinst);
        return(FALSE);
        }

    // Copy the rest of the command parameters down
    //
    if (lpEnd)
        {
        lstrcpy(lpszCmdLine, lpEnd);
        }
    else
        {
        *lpszCmdLine = '\0';
        }

    return(TRUE);
    }


/*----------------------------------------------------------
Purpose: Handles WM_NOTIFY messages for stub window
Returns: varies
Cond:    --
*/
LRESULT PRIVATE StubNotify(
    HWND hWnd, 
    WPARAM wParam, 
    NEWTHREAD_NOTIFY FAR *lpn)
    {
    switch (lpn->hdr.code)
        {
#ifdef COOLICON
    case RDN_TASKINFO:
        SetWindowText(hWnd, lpn->lpszTitle ? lpn->lpszTitle : "");
        g_hIcon = lpn->hIcon ? lpn->hIcon :
                LoadIcon(hInst, MAKEINTRESOURCE(IDI_DEFAULT));
        break;
#endif

    default:
        return(DefWindowProc(hWnd, WM_NOTIFY, wParam, (LPARAM)lpn));
        }
    }


/*----------------------------------------------------------
Purpose: Window proc for thread

Returns: varies
Cond:    --
*/
LRESULT CALLBACK WndProc(
    HWND hWnd, 
    UINT iMessage, 
    WPARAM wParam, 
    LPARAM lParam)
    {
    switch(iMessage)
        {
    case WM_CREATE:
#ifdef COOLICON
        g_hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_DEFAULT));
#endif
        break;

    case WM_DESTROY:
        break;

    case WM_NOTIFY:
        return(StubNotify(hWnd, wParam, (NEWTHREAD_NOTIFY FAR *)lParam));

#ifdef COOLICON
    case WM_QUERYDRAGICON:
        return(MAKELRESULT(g_hIcon, 0));
#endif
        
    case STUBM_SETDATA:
        SetWindowLong(hWnd, 0, wParam);
        SetWindowLong(hWnd, 4, lParam);
        break;
        
    case  STUBM_GETDATA:
        *((LONG *)lParam) = GetWindowLong(hWnd, 4);
        return GetWindowLong(hWnd, 0);

    default:
        return DefWindowProc(hWnd, iMessage, wParam, lParam) ;
        break;
        }

    return 0L;
    }


/*----------------------------------------------------------
Purpose: Creates a stub window to handle the thread

Returns: handle to window
Cond:    --
*/
HWND PRIVATE CreateStubWindow(LPSTR szClassName)
    {
    WNDCLASS wndclass;

    if (!GetClassInfo(ghInstance, szClassName, &wndclass))
        {
        wndclass.style         = 0 ;
        wndclass.lpfnWndProc   = (WNDPROC)WndProc ;
        wndclass.cbClsExtra    = 0 ;
        wndclass.cbWndExtra    = sizeof(DWORD) * 2 ; // This is not a Win64 bug; app really wants two DWORDs
        wndclass.hInstance     = ghInstance;
        wndclass.hIcon         = NULL ;
        wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
        wndclass.hbrBackground = GetStockObject (WHITE_BRUSH) ;
        wndclass.lpszMenuName  = NULL ;
        wndclass.lpszClassName = szClassName ;

        if (!RegisterClass(&wndclass))
            {
            return NULL;
            }
        }

    return CreateWindowEx(WS_EX_TOOLWINDOW, szClassName, c_szNull,
                        0, 0, 0, 0, 0, NULL, NULL, ghInstance, NULL);
    }


/*----------------------------------------------------------
Purpose: Initializes a new thread in our DLL

Returns: 0
Cond:    --
*/
DWORD PRIVATE ThreadInitDLL(
    LPVOID pszCmdLine)
    {
    DLLENTRY dlle;

    // Increment the reference count so that this function will always
    // be present.
    //
    g_cRef++;
    if (InitializeDLLEntry((LPSTR)pszCmdLine, &dlle))
        {
        HWND hwndStub = CreateStubWindow(dlle.szCmd);
        if (hwndStub)
            {
            SetForegroundWindow(hwndStub);
            dlle.lpfn(hwndStub, ghInstance, pszCmdLine, SW_NORMAL);
            DestroyWindow(hwndStub);
            }
        FreeLibrary(dlle.hinst);
        }

    GlobalFreePtr((LPSTR)pszCmdLine);
    g_cRef--;

    return 0;
    }


/*----------------------------------------------------------
Purpose: Start a new thread

         This code was swiped from the shell.

Returns: TRUE if the thread was created successfully
Cond:    --
*/
BOOL PUBLIC RunDLLThread(
    HWND hwnd, 
    LPCSTR pszCmdLine, 
    int nCmdShow)
    {
    BOOL fSuccess=FALSE; // assume error
    LPSTR pszCopy = GlobalAllocPtr(GPTR, lstrlen(pszCmdLine)+1);

    // _asm int 3;

    if (pszCopy)
        {
        DWORD idThread;
        HANDLE hthread;
        lstrcpy(pszCopy, pszCmdLine);

        if (hthread=CreateThread(NULL, 0, ThreadInitDLL, pszCopy, 0, &idThread))
            {
            // We don't need to communicate with this thread any more.
            // Close the handle and let it run and terminate itself.
            //
            // Notes: In this case, pszCopy will be freed by the thread.
            //
            CloseHandle(hthread);
            fSuccess=TRUE;
            }
        else
            {
            // Thread creation failed, we should free the buffer.
            GlobalFreePtr(pszCopy);
            }
        }

    return fSuccess;
    }

/****************************************************************************
* @doc INTERNAL
*
* @func BOOL PUBLIC | RunDLLProcess | This function starts a new process.
*
* @rdesc Returns TRUEE if successful.
*
****************************************************************************/

BOOL PUBLIC RunDLLProcess (LPSTR pszCmdLine)
{
  STARTUPINFO         sti;
  PROCESS_INFORMATION pi;
  BOOL                bRet;
  LPSTR               pszCmd;

  // Build the command line string
  //
  if ((pszCmd = (LPSTR)LocalAlloc(LMEM_FIXED, sizeof(c_szRunDLLProcess)+
                                              lstrlen(pszCmdLine)+1)) == NULL)
    return FALSE;

  lstrcpy(pszCmd, c_szRunDLLProcess);
  lstrcat(pszCmd, pszCmdLine);

  ZeroMemory(&sti, sizeof(sti));
  sti.cb = sizeof(sti);

  // Start the modem installation process
  //
  if (bRet = CreateProcess(NULL, pszCmd,
                           NULL, NULL, FALSE, 0, NULL, NULL,
                           &sti, &pi))
  {
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
  };

  LocalFree(pszCmd);

  return bRet;
}

