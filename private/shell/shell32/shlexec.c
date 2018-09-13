    /*
 *  shlexec.c -
 *
 *  Implements the ShellExecuteEx() function
 */
#include "shellprv.h"
#pragma  hdrstop
#include "shlexec.h"
#include "fstreex.h"
#include "uemapp.h"
#include "netview.h"
#include <badapps.h>
#include <htmlhelp.h>


// Stuff for Darwin; we LoadLibrary / GetProcAddress
// msi.dll, so we need to have this as per instance hlib.
HINSTANCE g_hInstDarwin = NULL;

#define CH_GUIDFIRST TEXT('{') // '}'

#ifdef WINNT
// Handle reg key naming differences between NT and Win9x. Bummer. Space between session manager in NT which isn't there in Win9x
//  For apps == 4.00
#define REGSTR_PATH_CHECKBADAPPSNEW    TEXT("System\\CurrentControlSet\\Control\\Session Manager\\CheckBadApps")
#define REGSTR_PATH_CHECKBADAPPS400NEW TEXT("System\\CurrentControlSet\\Control\\Session Manager\\CheckBadApps400")
#define REGSTR_TEMP_APPCOMPATPATH      TEXT("System\\CurrentControlSet\\Control\\Session Manager\\AppCompatibility\\%s")
#else
#define REGSTR_PATH_CHECKBADAPPSNEW    TEXT("System\\CurrentControlSet\\Control\\SessionManager\\CheckBadApps")
#define REGSTR_PATH_CHECKBADAPPS400NEW TEXT("System\\CurrentControlSet\\Control\\SessionManager\\CheckBadApps400")
#define REGSTR_TEMP_APPCOMPATPATH      TEXT("System\\CurrentControlSet\\Control\\SessionManager\\AppCompatibility\\%s")
#endif

// These fake ERROR_ values are used to display non-winerror.h available error
// messages. They are mapped to valid winerror.h values in _ShellExecuteError.
#define ERROR_RESTRICTED_APP ((UINT)-1)

#define SEE_MASK_CLASS (SEE_MASK_CLASSNAME|SEE_MASK_CLASSKEY)
#define _UseClassName(_mask) (((_mask)&SEE_MASK_CLASS) == SEE_MASK_CLASSNAME)
#define _UseClassKey(_mask)  (((_mask)&SEE_MASK_CLASS) == SEE_MASK_CLASSKEY)
#define _UseTitleName(_mask) (((_mask)&SEE_MASK_HASTITLE) || ((_mask)&SEE_MASK_HASLINKNAME))

#define SEE_MASK_PIDL (SEE_MASK_IDLIST|SEE_MASK_INVOKEIDLIST)
#define _UseIDList(_mask)     (((_mask)&SEE_MASK_PIDL) == SEE_MASK_IDLIST)
#define _InvokeIDList(_mask)  (((_mask)&SEE_MASK_PIDL) == SEE_MASK_INVOKEIDLIST)


#define SAFE_DEBUGSTR(str)    ((str) ? (str) : "<NULL>")

// secret kernel api to get name of missing 16 bit component
extern int WINAPI PK16FNF(TCHAR *szBuffer);

BOOL _ShellExecPidl(LPSHELLEXECUTEINFO pei, LPITEMIDLIST pidlExec);

// in exec2.c
int FindAssociatedExe(HWND hwnd, LPTSTR lpCommand, LPCTSTR pszDocument, HKEY hkeyProgID);


#ifdef DEBUG

/*----------------------------------------------------------
Purpose: Validation function for SHELLEXECUTEINFO

*/
BOOL IsValidPSHELLEXECUTEINFO(LPSHELLEXECUTEINFO pei)
{
    return (IS_VALID_WRITE_PTR(pei, SHELLEXECUTEINFO) &&
            IS_VALID_SIZE(pei->cbSize, sizeof(*pei)) &&
            (IsFlagSet(pei->fMask, SEE_MASK_FLAG_NO_UI) ||
             NULL == pei->hwnd ||
             IS_VALID_HANDLE(pei->hwnd, WND)) &&
            (NULL == pei->lpVerb || IS_VALID_STRING_PTR(pei->lpVerb, -1)) &&
            (NULL == pei->lpFile || IS_VALID_STRING_PTR(pei->lpFile, -1)) &&
            (NULL == pei->lpParameters || IS_VALID_STRING_PTR(pei->lpParameters, -1)) &&
            (NULL == pei->lpDirectory || IS_VALID_STRING_PTR(pei->lpDirectory, -1)) &&
            (NULL == pei->hInstApp || IS_VALID_HANDLE(pei->hInstApp, INSTANCE)) &&
            (IsFlagClear(pei->fMask, SEE_MASK_IDLIST) ||
             IsFlagSet(pei->fMask, SEE_MASK_INVOKEIDLIST) ||        // because SEE_MASK_IDLIST is part of SEE_MASK_INVOKEIDLIST this line will
             IS_VALID_PIDL(pei->lpIDList)) &&                       // defer to the next clause if the superset is true
            (IsFlagClear(pei->fMask, SEE_MASK_INVOKEIDLIST) ||
             NULL == pei->lpIDList ||
             IS_VALID_PIDL(pei->lpIDList)) &&
            ( !_UseClassName(pei->fMask) ||
             IS_VALID_STRING_PTR(pei->lpClass, -1)) &&
            ( !_UseTitleName(pei->fMask) ||
             NULL == pei->lpClass ||
             IS_VALID_STRING_PTR(pei->lpClass, -1)) &&
            ( !_UseClassKey(pei->fMask) ||
             IS_VALID_HANDLE(pei->hkeyClass, KEY)) &&
            (IsFlagClear(pei->fMask, SEE_MASK_ICON) ||
             IS_VALID_HANDLE(pei->hIcon, ICON)));
}

#endif // DEBUG


HINSTANCE Window_GetInstance(HWND hwnd)
{
    DWORD idProcess;

    GetWindowThreadProcessId(hwnd, &idProcess);
#ifdef WINNT
    //
    // On Windows NT HINSTANCEs are pointers valid only within
    // a single process, so 33 is returned to indicate success
    // as 0-32 are reserved for error.  (Actually 32 is supposed
    // to be a valid success return but some apps get it wrong.)
    //

    return (HINSTANCE)(idProcess ? 33 : 0);
#else
    return (HINSTANCE)GetProcessDword(idProcess, GPD_HINST);
#endif
}


/* _RoundRobinWindows:
 * A silly little enumproc to find any window (EnumWindows) which has a
 * matching EXE file path.  The desired match EXE pathname is pointed to
 * by the lParam.  The found window's handle is stored in the
 * first word of this buffer.
 */
BOOL CALLBACK _RoundRobinWindows(HWND hWnd, LPARAM lParam)
{
    TCHAR szT[MAX_PATH];
    HINSTANCE hInstance;
#ifdef DEBUG
    TCHAR szWnd[MAX_PATH];
#endif


    // Filter out invisible and disabled windows

    if (!IsWindowEnabled(hWnd) || !IsWindowVisible(hWnd))
        return TRUE;

    hInstance = Window_GetInstance(hWnd);
    // NB We are trying to get the filename from a 16bit hinst so
    // we need to thunk down.
    GetModuleFileName16(hInstance, szT, ARRAYSIZE(szT) - 1);

#ifdef DEBUG
    GetWindowText(hWnd, szWnd, ARRAYSIZE(szWnd));
    TraceMsg(TF_SHELLEXEC, "RoundRobinWindows: %x %x    %s %s", hWnd, hInstance, (LPTSTR)szWnd, (LPTSTR)szT);
#endif

    if (lstrcmpi(PathFindFileName(szT), PathFindFileName((LPTSTR)lParam)) == 0)
    {
        *(HWND *)lParam = hWnd;
        return FALSE;
    }

    return TRUE;
}

/* Find a "main" window that is the ancestor of a given window
 */
HWND _GetAncestorWindow(HWND hwnd)
{
    HWND hwndT;

    if (!hwnd)
      return NULL;

    /* First go up the parent chain to find the popup window.  Then go
    * up the owner chain to find the main window
    */
    while (NULL != (hwndT = GetWindow(hwnd, GW_OWNER)))
        hwnd = hwndT;

    return(hwnd);
}


/*
 * A helper for RealShellExecute which finds the top-level window of a
 * program which came from a particular EXE file.  Returns NULL if none
 * was found.
 * Assumes: finding ultimate parent/owner of any window which matches EXEs
 * is the top-level window desired.  If we had an EnumTopLevelWindows...
 */
HWND _FindPopupFromExe(LPTSTR lpExe)
{
    HWND hwnd;
    BOOL b;
    TCHAR szExe[MAX_PATH];

    lstrcpyn(szExe, lpExe, ARRAYSIZE(szExe));
    PathUnquoteSpaces(szExe);
    b = EnumWindows(_RoundRobinWindows, (LPARAM)szExe);
    if (b)
        return NULL;

    hwnd = *(HWND *)szExe;

    if (hwnd == NULL)
        return NULL;

    // Climbing up parents/owners not strictly necessary for two reasons:
    // * EnumWindows is usually going to find the "main" window first for
    //   each app anyway,
    // * Our usage here passes it on to ActivateHandler, which climbs for
    //   us.

    return _GetAncestorWindow(hwnd);
}


//----------------------------------------------------------------------------
// Return TRUE if the window belongs to a 32bit or a Win4.0 app.
// NB We can't just check if it's a 32bit window
// since many apps use 16bit ddeml windows to communicate with the shell
// On NT we can.
BOOL Window_IsLFNAware(HWND hwnd)
{
#ifdef WINNT
    if (LOWORD(GetWindowLongPtr(hwnd,GWLP_HINSTANCE)) == 0) {
        // 32-bit window
        return TRUE;
    }
    // BUGBUG - BobDay - Don't know about whether Win31 or Win40 yet?
    return FALSE;
#else
    DWORD idProcess;

    GetWindowThreadProcessId(hwnd, &idProcess);
    if (!(GetProcessDword(idProcess, GPD_FLAGS) & GPF_WIN16_PROCESS) ||
        (GetProcessDword(idProcess, GPD_EXP_WINVER) >= 0x0400))
    {
        TraceMsg(TF_SHELLEXEC, "Window_IsLFNAware: Win32 app (%x) handling DDE cmd.", hwnd);
        return TRUE;
    }

    TraceMsg(TF_SHELLEXEC, "Window_IsLFNAware: Win16 app (%x) handle DDE cmd.", hwnd);
    return FALSE;
#endif
}


#define COPYTODST(_szdst, _szend, _szsrc, _ulen, _ret) \
{ \
        UINT _utemp = _ulen; \
        if ((UINT)(_szend-_szdst) <= _utemp) { \
                return(_ret); \
        } \
        lstrcpyn(_szdst, _szsrc, _utemp+1); \
        _szdst += _utemp; \
}

/* Returns NULL if this is the last parm, pointer to next space otherwise
 */
LPTSTR _GetNextParm(LPCTSTR lpSrc, LPTSTR lpDst, UINT cchDst)
{
    LPCTSTR lpNextQuote, lpNextSpace;
    LPTSTR lpEnd = lpDst+cchDst-1;       // dec to account for trailing NULL
    BOOL fQuote;                        // quoted string?
    BOOL fDoubleQuote;                  // is this quote a double quote?
    VDATEINPUTBUF(lpDst, TCHAR, cchDst);

    while (*lpSrc == TEXT(' '))
        ++lpSrc;

    if (!*lpSrc)
        return(NULL);

    fQuote = (*lpSrc == TEXT('"'));
    if (fQuote)
        lpSrc++;   // skip leading quote

    for (;;)
    {
        lpNextQuote = StrChr(lpSrc, TEXT('"'));

        if (!fQuote)
        {
            // for an un-quoted string, copy all chars to first space/null

            lpNextSpace = StrChr(lpSrc, TEXT(' '));

            if (!lpNextSpace) // null before space! (end of string)
            {
                if (!lpNextQuote)
                {
                    // copy all chars to the null
                    if (lpDst)
                    {
                        COPYTODST(lpDst, lpEnd, lpSrc, lstrlen(lpSrc), NULL);
                    }
                    return NULL;
                }
                else
                {
                    // we have a quote to convert.  Fall through.
                }
            }
            else if (!lpNextQuote || lpNextSpace < lpNextQuote)
            {
                // copy all chars to the space
                if (lpDst)
                {
                    COPYTODST(lpDst, lpEnd, lpSrc, (UINT)(lpNextSpace-lpSrc), NULL);
                }
                return (LPTSTR)lpNextSpace;
            }
            else
            {
                // quote before space.  Fall through to convert quote.
            }
        }
        else if (!lpNextQuote)
        {
            // a quoted string without a terminating quote?  Illegal!
            ASSERT(0);
            return NULL;
        }

        // we have a potential quote to convert
        ASSERT(lpNextQuote);

        fDoubleQuote = *(lpNextQuote+1) == TEXT('"');
        if (fDoubleQuote)
            lpNextQuote++;      // so the quote is copied

        if (lpDst)
        {
            COPYTODST(lpDst, lpEnd, lpSrc, (UINT) (lpNextQuote-lpSrc), NULL);
        }

        lpSrc = lpNextQuote+1;

        if (!fDoubleQuote)
        {
            // we just copied the rest of this quoted string.  if this wasn't
            // quoted, it's an illegal string... treat the quote as a space.
            ASSERT(fQuote);
            return (LPTSTR)lpSrc;
        }
    }
}


//----------------------------------------------------------------------------
#define PEMAGIC         ((WORD)'P'+((WORD)'E'<<8))
//----------------------------------------------------------------------------
// Returns TRUE is app is LFN aware.
// NB This simply assumes all Win4.0 and all Win32 apps are LFN aware.
BOOL App_IsLFNAware(LPCTSTR pszFile)
{
    DWORD dw;
    BOOL fRet = FALSE;

    ASSERT(pszFile);
    ASSERT(*pszFile);

    // Assume Win 4.0 apps and Win32 apps are LFN aware.
    dw = GetExeType(pszFile);
    // TraceMsg(TF_SHELLEXEC, "s.aila: %s %s %x", lpszFile, szFile, dw);
    if ((LOWORD(dw) == PEMAGIC) || ((LOWORD(dw) == NEMAGIC) && (HIWORD(dw) >= 0x0400)))
    {
        TCHAR sz[MAX_PATH];
        PathToAppPathKey(pszFile, sz, ARRAYSIZE(sz));

        fRet = (NOERROR != SHGetValue(HKEY_LOCAL_MACHINE, sz, TEXT("UseShortName"), NULL, NULL, NULL));
    }
    
    return fRet;
}


//----------------------------------------------------------------------------
// this function checks for the existance of a value called "useURL" under the
// App Paths key in the registry associated with the app that is passed in.

BOOL DoesAppWantUrl(LPCTSTR pszPath)
{
    TCHAR sz[MAX_PATH];
    HKEY hk;
    BOOL bRet = FALSE;

    // NOTE this assumes that this is a path to the exe
    // and not a command line
    PathToAppPathKey(pszPath, sz, ARRAYSIZE(sz));
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, sz, 0L, KEY_QUERY_VALUE, &hk) == ERROR_SUCCESS)
    {
        bRet = SHQueryValueEx(hk, TEXT("UseURL"), NULL, NULL, NULL, NULL) == ERROR_SUCCESS;
        RegCloseKey(hk);
    }

    return bRet;
}

//----------------------------------------------------------------------------
BOOL _AppIsLFNAware(LPCTSTR lpszFile)
{
    TCHAR szFile[MAX_PATH];
    LPTSTR pszArgs;

    // Does it look like a DDE command?
    if (lpszFile && *lpszFile && (*lpszFile != TEXT('[')))
    {
        // Nope - Hopefully just a regular old command %1 thing.
        lstrcpyn(szFile, lpszFile, ARRAYSIZE(szFile));
        pszArgs = PathGetArgs(szFile);
        if (*pszArgs)
            *(pszArgs - 1) = TEXT('\0');
        PathRemoveBlanks(szFile);   // remove any blanks that may be after the command
        PathUnquoteSpaces(szFile);
        return App_IsLFNAware(szFile);
    }
    return FALSE;
}

// in:
//      lpFile      exe name (used for %0 or %1 in replacement string)
//      lpFrom      string template to sub params and file into "excel.exe %1 %2 /n %3"
//      lpParams    parameter list "foo.txt bar.txt"
// out:
//      lpTo    output string with all parameters replaced
//
// supports:
//      %*      replace with all parameters
//      %0, %1  replace with file
//      %n      use nth parameter
//
// replace parameter placeholders (%1 %2 ... %n) with parameters
// BUGBUG, we need to make sure we don't do more than MAX_PATH bytes into lpTo

UINT ReplaceParameters(LPTSTR lpTo, UINT cchTo, LPCTSTR lpFile,
        LPCTSTR lpFrom, LPCTSTR lpParms, int nShow, DWORD * pdwHotKey, BOOL fLFNAware,
        LPCITEMIDLIST lpID, LPITEMIDLIST *ppidlGlobal)
{
    int i;
    TCHAR c;
    LPCTSTR lpT;
    TCHAR sz[MAX_PATH];
    BOOL fFirstParam = TRUE;
    LPTSTR lpEnd = lpTo + cchTo - 1;       // dec to allow trailing NULL
    LPTSTR pToOrig = lpTo;
#ifndef WINNT
    LPCTSTR pFromOrig = lpFrom;
#endif

    for ( ; *lpFrom; lpFrom++)
    {
        if (*lpFrom == TEXT('%'))
        {
            switch (*(++lpFrom))
            {
            case TEXT('~'): // Copy all parms starting with nth (n >= 2 and <= 9)
                  c = *(++lpFrom);
                  if (c >= TEXT('2') && c <= TEXT('9'))
                    {
                      for (i = 2, lpT = lpParms; i < c-TEXT('0') && lpT; i++)
                        {
                          lpT = _GetNextParm(lpT, NULL, 0);
                        }

                      if (lpT)
                        {
                          COPYTODST(lpTo, lpEnd, lpT, lstrlen(lpT), SE_ERR_ACCESSDENIED);
                        }
                    }
                  else
                    {
                      lpFrom -= 2;            // Backup over %~ and pass through
                      goto NormalChar;
                    }
                  break;

            case TEXT('*'): // Copy all parms
                  if (lpParms)
                  {
                      COPYTODST(lpTo, lpEnd, lpParms, lstrlen(lpParms), SE_ERR_ACCESSDENIED);
                  }
                  break;

            case TEXT('0'):
            case TEXT('1'):
                  // %0, %1, copy the file name
                  // If the filename comes first then we don't need to convert it to
                  // a shortname. If it appears anywhere else and the app is not LFN
                  // aware then we must.
                  if (!(fFirstParam || fLFNAware || _AppIsLFNAware(pToOrig)) &&
                      GetShortPathName(lpFile, sz, ARRAYSIZE(sz)) > 0)
                  {
                      TraceMsg(TF_SHELLEXEC, "ShellExecuteEx: Getting short version of path.");
                      COPYTODST(lpTo, lpEnd, sz, lstrlen(sz), SE_ERR_ACCESSDENIED);
                  }
                  else
                  {
                      TraceMsg(TF_SHELLEXEC, "ShellExecuteEx: Using long version of path.");
                      COPYTODST(lpTo, lpEnd, lpFile, lstrlen(lpFile), SE_ERR_ACCESSDENIED);
                  }
                  break;

            case TEXT('2'):
            case TEXT('3'):
            case TEXT('4'):
            case TEXT('5'):
            case TEXT('6'):
            case TEXT('7'):
            case TEXT('8'):
            case TEXT('9'):
                  for (i = *lpFrom-TEXT('2'), lpT = lpParms; lpT; --i)
                    {
                      if (i)
                          lpT = _GetNextParm(lpT, NULL, 0);
                      else
                        {
                          _GetNextParm(lpT, sz, ARRAYSIZE(sz));
                          COPYTODST(lpTo, lpEnd, sz, lstrlen(sz), SE_ERR_ACCESSDENIED);
                          break;
                        }
                    }
                  break;

                case TEXT('s'):
                case TEXT('S'):
                  wsprintf(sz, TEXT("%ld"), nShow);
                  COPYTODST(lpTo, lpEnd, sz, lstrlen(sz), SE_ERR_ACCESSDENIED);
                  break;

                case TEXT('h'):
                case TEXT('H'):
                  wsprintf(sz, TEXT("%X"), pdwHotKey ? *pdwHotKey : 0);
                  COPYTODST(lpTo, lpEnd, sz, lstrlen(sz), SE_ERR_ACCESSDENIED);
                  if (pdwHotKey)
                      *pdwHotKey = 0;
                  break;

                // Note that a new global IDList is created for each
                case TEXT('i'):
                case TEXT('I'):
                  // Note that a single global ID list is created and used over
                  // again, so that it may be easily destroyed if anything
                  // goes wrong
                  if (ppidlGlobal)
                  {
                      if (lpID && !*ppidlGlobal)
                      {
#ifndef WINNT
                          // these are the folks who figured out the %I and
                          // rely upon it being global data
                          if (StrStr(pFromOrig, "pdexplo.exe")) {
                              *ppidlGlobal = ILGlobalClone((lpID));
                              if (!*ppidlGlobal)
                              {
                                  return(SE_ERR_OOM);
                              }

                              wsprintf(sz, ":%ld", *ppidlGlobal);
                              COPYTODST(lpTo, lpEnd, sz, lstrlen(sz), SE_ERR_ACCESSDENIED);
                              break;
                          }
#endif

                          *ppidlGlobal = (LPITEMIDLIST)SHAllocShared(lpID,ILGetSize(lpID),GetCurrentProcessId());
                          if (!*ppidlGlobal)
                          {
                              return(SE_ERR_OOM);
                          }
                      }
                      wsprintf(sz, TEXT(":%ld:%ld"), *ppidlGlobal,GetCurrentProcessId());
                  }
                  else
                  {
                      lstrcpy(sz,TEXT(":0"));
                  }

                  COPYTODST(lpTo, lpEnd, sz, lstrlen(sz), SE_ERR_ACCESSDENIED);
                  break;

                case TEXT('l'):
                case TEXT('L'):
                  // Like %1 only using the long name.
                  // REVIEW UNDONE IANEL Remove the fFirstParam and fLFNAware crap as soon as this
                  // is up and running.
                  TraceMsg(TF_SHELLEXEC, "ShellExecuteEx: Using long version of path.");
                  COPYTODST(lpTo, lpEnd, lpFile, lstrlen(lpFile), SE_ERR_ACCESSDENIED);
                  break;

                case TEXT('D'):
                case TEXT('d'):
                {
                  // %D gives the display name of an object.
                  IShellFolder* pShellFolder;
                  LPITEMIDLIST pidlRight = NULL;
                  BOOL fSuccess = FALSE;
                  STRRET str;

                  if ( !lpID || FAILED(SHBindToIDListParent(lpID, &IID_IShellFolder, &pShellFolder, &pidlRight)) )
                      return SE_ERR_ACCESSDENIED;

                  if ( SUCCEEDED(pShellFolder->lpVtbl->GetDisplayNameOf(pShellFolder, pidlRight, SHGDN_FORPARSING, &str)) )
                      fSuccess = SUCCEEDED(StrRetToBuf(&str, lpID, sz, ARRAYSIZE(sz)));

                  pShellFolder->lpVtbl->Release(pShellFolder);

                  if ( fSuccess )
                      COPYTODST(lpTo, lpEnd, sz, lstrlen(sz), SE_ERR_ACCESSDENIED);

                  break;
                }

                default:
                  goto NormalChar;
              }
              // TraceMsg(TF_SHELLEXEC, "s.rp: Past first param (1).");
              fFirstParam = FALSE;
        }
        else
        {
NormalChar:
              // not a "%?" thing, just copy this to the destination

              if (lpEnd-lpTo < 2)
              {
                  // Always check for room for DBCS char
                  return(SE_ERR_ACCESSDENIED);
              }

              *lpTo++ = *lpFrom;
              // Special case for things like "%1" ie don't clear the first param flag
              // if we hit a dbl-quote.
              if (*lpFrom != TEXT('"'))
              {
                  // TraceMsg(TF_SHELLEXEC, "s.rp: Past first param (2).");
                  fFirstParam = FALSE;
              }
              else if (IsDBCSLeadByte(*lpFrom))
              {
                  *lpTo++ = *(++lpFrom);
              }

            }
    }

    // We should always have enough room since we dec'ed cchTo when determining
    // lpEnd
    *lpTo = 0;

    // This means success
    return(0);
}

HWND ThreadID_GetVisibleWindow(DWORD dwID)
{
    HWND hwnd;
    DWORD dwIDTmp;

    for (hwnd = GetWindow(GetDesktopWindow(), GW_CHILD); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT))
    {
        dwIDTmp = GetWindowThreadProcessId(hwnd, NULL);
        TraceMsg(TF_SHELLEXEC, "s.ti_gvw: Hwnd %x Thread ID %x.", hwnd, dwIDTmp);
        if (IsWindowVisible(hwnd) && (dwIDTmp == dwID))
        {
            TraceMsg(TF_SHELLEXEC, "s.ti_gvw: Found match %x.", hwnd);
            return hwnd;
        }
    }
    return NULL;
}

void ActivateHandler(HWND hwnd, DWORD_PTR dwHotKey)
{
    HWND hwndT;
    DWORD dwID;

    hwnd = _GetAncestorWindow(hwnd);

    hwndT = GetLastActivePopup(hwnd);

    if (!IsWindowVisible(hwndT))
    {
        dwID = GetWindowThreadProcessId(hwnd, NULL);
        TraceMsg(TF_SHELLEXEC, "ActivateHandler: Hwnd %x Thread ID %x.", hwnd, dwID);
        ASSERT(dwID);
        // Find the first visible top level window owned by the
        // same guy that's handling the DDE conversation.
        hwnd = ThreadID_GetVisibleWindow(dwID);
        if (hwnd)
        {
            hwndT = GetLastActivePopup(hwnd);
            if (IsIconic(hwnd))
            {
                TraceMsg(TF_SHELLEXEC, "ActivateHandler: Window is iconic, restoring.");
                ShowWindow(hwnd,SW_RESTORE);
            }
            else
            {
                TraceMsg(TF_SHELLEXEC, "ActivateHandler: Window is normal, bringing to top.");
                BringWindowToTop(hwnd);
                if (hwndT && hwnd != hwndT)
                    BringWindowToTop(hwndT);

            }

            // set the hotkey
            if (dwHotKey) {
                SendMessage(hwnd, WM_SETHOTKEY, dwHotKey, 0);
            }
        }
    }
}

// Some apps when run no-active steal the focus anyway so we
// we set it back to the previously active window.

void FixActivationStealingApps(HWND hwndOldActive, int nShow)
{
    HWND hwndNew;

    if (nShow == SW_SHOWMINNOACTIVE && (hwndNew = GetForegroundWindow()) != hwndOldActive && IsIconic(hwndNew))
        SetForegroundWindow(hwndOldActive);
}


// reg call for simpeltons

void RegGetValue(HKEY hkRoot, LPCTSTR lpKey, LPTSTR lpValue)
{
    LONG l = MAX_PATH;

    *lpValue = 0;
    SHRegQueryValue(hkRoot, lpKey, lpValue, &l);
}

BOOL FindExistingDrv(LPCTSTR pszUNCRoot, LPTSTR pszLocalName)
{
    int iDrive;

    for (iDrive = 0; iDrive < 26; iDrive++) {
        if (IsRemoteDrive(iDrive)) {
            TCHAR szDriveName[3];
            DWORD cb = MAX_PATH;
            szDriveName[0] = (TCHAR)iDrive + (TCHAR)TEXT('A');
            szDriveName[1] = TEXT(':');
            szDriveName[2] = 0;
            SHWNetGetConnection(szDriveName, pszLocalName, &cb);
            if (lstrcmpi(pszUNCRoot, pszLocalName) == 0) {
                lstrcpy(pszLocalName, szDriveName);
                return(TRUE);
            }
        }
    }
    return(FALSE);
}



HRESULT _CheckExistingNet(LPCTSTR pszFile, LPCTSTR pszRoot, BOOL fPrint)
{
    //
    // This used to be a call to GetFileAttributes(), but
    // GetFileAttributes() doesn't handle net paths very well.
    // However, we need to be careful, because other shell code
    // expects SHValidateUNC to return false for paths that point
    // to print shares.
    //
    HRESULT hr = S_FALSE;

    if (!PathIsRoot(pszFile))
    {
        // if we are checking for a printshare, then it must be a Root
        if (fPrint)
            hr = E_FAIL;
        else if (PathFileExists(pszFile))
            hr = S_OK;
    }

    if (S_FALSE == hr)
    {
        DWORD dwType;
        
        if (NetPathExists(pszRoot ,&dwType))
        {
            if (fPrint ? dwType != RESOURCETYPE_PRINT : dwType == RESOURCETYPE_PRINT)
                hr = E_FAIL;
            else
                hr = S_OK;
        }
        else if (-1 != GetFileAttributes(pszRoot))
        {
            //
            // BUGBUG:  IE 4.01 SP1 QFE #104.  GetFileAttributes now called
            // as a last resort become some clients often fail when using
            // WNetGetResourceInformation.  For example, many NFS clients were
            // broken because of this.
            //
            hr = S_OK;
        }
    }

    if (hr == E_FAIL)
        SetLastError(ERROR_NOT_SUPPORTED);
        
    return hr;
}

HRESULT _CheckNetUse(HWND hwnd, LPTSTR pszShare, UINT fConnect, LPTSTR pszOut, DWORD cchOut)
{
    NETRESOURCE rc;
    DWORD dw, err;
    DWORD dwRedir = CONNECT_TEMPORARY;

    if (!(fConnect & VALIDATEUNC_NOUI))
        dwRedir |= CONNECT_INTERACTIVE;

    if (fConnect & VALIDATEUNC_CONNECT)
        dwRedir |= CONNECT_REDIRECT;

    // VALIDATE_PRINT happens only after a failed attempt to validate for
    // a file. That previous attempt will have given the option to
    // connect to other media -- don't do it here or the user will be
    // presented with the same dialog twice when the first one is cancelled.
    if (fConnect & VALIDATEUNC_PRINT)
        dwRedir |= CONNECT_CURRENT_MEDIA;

    rc.lpRemoteName = pszShare;
    rc.lpLocalName = NULL;
    rc.lpProvider = NULL;
    rc.dwType = (fConnect & VALIDATEUNC_PRINT) ? RESOURCETYPE_PRINT : RESOURCETYPE_DISK;

    err = WNetUseConnection(hwnd, &rc, NULL, NULL, dwRedir, pszOut, &cchOut, &dw);

    TraceMsg(TF_SHELLEXEC, "SHValidateUNC WNetUseConnection(%s) returned %x", pszShare, err);

    if (err)
    {
        SetLastError(err);
        return E_FAIL;
    }
    else if (fConnect & VALIDATEUNC_PRINT)        
    {
        //  just because WNetUse succeeded, doesnt mean 
        //  NetPathExists will.  if it fails then 
        //  we shouldnt succeed this call regardless
        //  because we are only interested in print shares.
        if (!NetPathExists(pszShare, &dw)
        || (dw != RESOURCETYPE_PRINT))
        {
            SetLastError(ERROR_NOT_SUPPORTED);
            return E_FAIL;
        }
    }

    return S_OK;
}

//
// SHValidateUNC
//
//  This function validates a UNC path by calling WNetAddConnection3.
//  It will make it possible for the user to type a remote (RNA) UNC
//  app/document name from Start->Run dialog.
//
//  fConnect    - flags controling what to do
//
//    VALIDATEUNC_NOUI                // dont bring up stinking UI!
//    VALIDATEUNC_CONNECT             // connect a drive letter
//    VALIDATEUNC_PRINT               // validate as print share instead of disk share
//
BOOL WINAPI SHValidateUNC(HWND hwndOwner, LPTSTR pszFile, UINT fConnect)
{
    HRESULT hr;
    TCHAR  szShare[MAX_PATH];
    BOOL fPrint = (fConnect & VALIDATEUNC_PRINT);

    ASSERT(PathIsUNC(pszFile));
    ASSERT((fConnect & ~VALIDATEUNC_VALID) == 0);
    ASSERT((fConnect & VALIDATEUNC_CONNECT) ? !fPrint : TRUE);

    lstrcpyn(szShare, pszFile, ARRAYSIZE(szShare));

    if (!PathStripToRoot(szShare))
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    if (fConnect & VALIDATEUNC_CONNECT)
        hr = S_FALSE;
    else
        hr = _CheckExistingNet(pszFile, szShare, fPrint);

    if (S_FALSE == hr)
    {
        TCHAR  szAccessName[MAX_PATH];

        if (!fPrint && FindExistingDrv(szShare, szAccessName))
        {
            hr = S_OK;
        }
        else 
            hr = _CheckNetUse(hwndOwner, szShare, fConnect, szAccessName, SIZECHARS(szAccessName));


        if (S_OK == hr && !fPrint)
        {
            StrCatBuff(szAccessName, pszFile + lstrlen(szShare), ARRAYSIZE(szAccessName));
            // The name should only get shorter, so no need to check length
            lstrcpy(pszFile, szAccessName);

            // Handle the root case
            if (pszFile[2] == TEXT('\0'))
            {
                pszFile[2] = TEXT('\\');
                pszFile[3] = TEXT('\0');
            }

            hr = _CheckExistingNet(pszFile, szShare, FALSE);
        }
    }

    return (hr == S_OK);
}

HINSTANCE WINAPI RealShellExecuteExA(HWND hwnd, LPCSTR lpOp, LPCSTR lpFile,
                                   LPCSTR lpArgs, LPCSTR lpDir, LPSTR lpResult,
                                   LPCSTR lpTitle, LPSTR lpReserved,
                                   WORD nShowCmd, LPHANDLE lphProcess,
                                   DWORD dwFlags )
{
    SHELLEXECUTEINFOA sei = { SIZEOF(SHELLEXECUTEINFOA), SEE_MASK_FLAG_NO_UI|SEE_MASK_FORCENOIDLIST, hwnd, lpOp, lpFile, lpArgs, lpDir, nShowCmd, NULL};

    TraceMsg(TF_SHELLEXEC, "RealShellExecuteExA(%04X, %s, %s, %s, %s, %s, %s, %s, %d, %08lX, %d)",
                    hwnd, lpOp, lpFile, lpArgs, lpDir, lpResult, lpTitle,
                    lpReserved, nShowCmd, lphProcess, dwFlags );

#ifdef WINNT
    //
    // Pass along the lpReserved parameter to the new process
    //
    if ( lpReserved )
    {
        sei.fMask |= SEE_MASK_RESERVED;
        sei.hInstApp = (HINSTANCE)lpReserved;
    }

    //
    // Pass along the lpTitle parameter to the new process
    //
    if ( lpTitle )
    {
        sei.fMask |= SEE_MASK_HASTITLE;
        sei.lpClass = lpTitle;
    }

    //
    // Pass along the SEPARATE_VDM flag
    //
    if ( dwFlags & EXEC_SEPARATE_VDM )
    {
        sei.fMask |= SEE_MASK_FLAG_SEPVDM;
    }
#endif

    //
    // Pass along the NO_CONSOLE flag
    //
    if ( dwFlags & EXEC_NO_CONSOLE )
    {
        sei.fMask |= SEE_MASK_NO_CONSOLE;
    }

    if ( lphProcess )
    {
        //
        // Return the process handle
        //
        sei.fMask |= SEE_MASK_NOCLOSEPROCESS;
        ShellExecuteExA(&sei);
        *lphProcess = sei.hProcess;
    }
    else
    {
        ShellExecuteExA(&sei);
    }

    return sei.hInstApp;
}

HINSTANCE WINAPI RealShellExecuteExW(HWND hwnd, LPCWSTR lpOp, LPCWSTR lpFile,
                                   LPCWSTR lpArgs, LPCWSTR lpDir, LPWSTR lpResult,
                                   LPCWSTR lpTitle, LPWSTR lpReserved,
                                   WORD nShowCmd, LPHANDLE lphProcess,
                                   DWORD dwFlags )
{
    SHELLEXECUTEINFOW sei = { SIZEOF(SHELLEXECUTEINFOW), SEE_MASK_FLAG_NO_UI|SEE_MASK_FORCENOIDLIST, hwnd, lpOp, lpFile, lpArgs, lpDir, nShowCmd, NULL};

    TraceMsg(TF_SHELLEXEC, "RealShellExecuteExW(%04X, %s, %s, %s, %s, %s, %s, %s, %d, %08lX, %d)",
                    hwnd, lpOp, lpFile, lpArgs, lpDir, lpResult, lpTitle,
                    lpReserved, nShowCmd, lphProcess, dwFlags );

#ifdef WINNT
    //
    // Pass along the lpReserved parameter to the new process
    //
    if ( lpReserved )
    {
        sei.fMask |= SEE_MASK_RESERVED;
        sei.hInstApp = (HINSTANCE)lpReserved;
    }

    //
    // Pass along the lpTitle parameter to the new process
    //
    if ( lpTitle )
    {
        sei.fMask |= SEE_MASK_HASTITLE;
        sei.lpClass = lpTitle;
    }

    //
    // Pass along the SEPARATE_VDM flag
    //
    if ( dwFlags & EXEC_SEPARATE_VDM )
    {
        sei.fMask |= SEE_MASK_FLAG_SEPVDM;
    }
#endif

    //
    // Pass along the NO_CONSOLE flag
    //
    if ( dwFlags & EXEC_NO_CONSOLE )
    {
        sei.fMask |= SEE_MASK_NO_CONSOLE;
    }

    if ( lphProcess )
    {
        //
        // Return the process handle
        //
        sei.fMask |= SEE_MASK_NOCLOSEPROCESS;
        ShellExecuteExW(&sei);
        *lphProcess = sei.hProcess;
    }
    else
    {
        ShellExecuteExW(&sei);
    }

    return sei.hInstApp;
}

HINSTANCE WINAPI RealShellExecuteA(HWND hwnd, LPCSTR lpOp, LPCSTR lpFile,
                                   LPCSTR lpArgs, LPCSTR lpDir, LPSTR lpResult,
                                   LPCSTR lpTitle, LPSTR lpReserved,
                                   WORD nShowCmd, LPHANDLE lphProcess )
{
    TraceMsg(TF_SHELLEXEC, "RealShellExecuteA(%04X, %s, %s, %s, %s, %s, %s, %s, %d, %08lX)",
                    hwnd, lpOp, lpFile, lpArgs, lpDir, lpResult, lpTitle,
                    lpReserved, nShowCmd, lphProcess );

    return RealShellExecuteExA(hwnd,lpOp,lpFile,lpArgs,lpDir,lpResult,lpTitle,lpReserved,nShowCmd,lphProcess,0);
}

HINSTANCE RealShellExecuteW(HWND hwnd, LPCWSTR lpOp, LPCWSTR lpFile,
                                   LPCWSTR lpArgs, LPCWSTR lpDir, LPWSTR lpResult,
                                   LPCWSTR lpTitle, LPWSTR lpReserved,
                                   WORD nShowCmd, LPHANDLE lphProcess)
{
    TraceMsg(TF_SHELLEXEC, "RealShellExecuteW(%04X, %s, %s, %s, %s, %s, %s, %s, %d, %08lX)",
                    hwnd, lpOp, lpFile, lpArgs, lpDir, lpResult, lpTitle,
                    lpReserved, nShowCmd, lphProcess );

    return RealShellExecuteExW(hwnd,lpOp,lpFile,lpArgs,lpDir,lpResult,lpTitle,lpReserved,nShowCmd,lphProcess,0);
}

HINSTANCE WINAPI ShellExecute(HWND hwnd, LPCTSTR lpOp, LPCTSTR lpFile, LPCTSTR lpArgs,
                               LPCTSTR lpDir, int nShowCmd)
{
    // NB The FORCENOIDLIST flag stops us from going through the ShellExecPidl()
    // code (for backwards compatability with progman).
    // DDEWAIT makes us synchronous, and gets around threads without
    // msg pumps and ones that are killed immediately after shellexec()
    SHELLEXECUTEINFO sei = { SIZEOF(SHELLEXECUTEINFO), 0, hwnd, lpOp, lpFile, lpArgs, lpDir, nShowCmd, NULL};
    ULONG fMask = SEE_MASK_FLAG_NO_UI|SEE_MASK_FORCENOIDLIST;
    if(!(SHGetAppCompatFlags(ACF_WIN95SHLEXEC) & ACF_WIN95SHLEXEC))
        fMask |= SEE_MASK_FLAG_DDEWAIT;
    sei.fMask = fMask;

    TraceMsg(TF_SHELLEXEC, "ShellExecute(%04X, %s, %s, %s, %s, %d)", hwnd, lpOp, lpFile, lpArgs, lpDir, nShowCmd);

    ShellExecuteEx(&sei);
    return sei.hInstApp;
}

#ifdef UNICODE
HINSTANCE WINAPI ShellExecuteA(HWND hwnd, LPCSTR lpOp, LPCSTR lpFile, LPCSTR lpArgs,
                               LPCSTR lpDir, int nShowCmd)
{
    // NB The FORCENOIDLIST flag stops us from going through the ShellExecPidl()
    // code (for backwards compatability with progman).
    // DDEWAIT makes us synchronous, and gets around threads without
    // msg pumps and ones that are killed immediately after shellexec()
    SHELLEXECUTEINFOA sei = { SIZEOF(SHELLEXECUTEINFOA), 0, hwnd, lpOp, lpFile, lpArgs, lpDir, nShowCmd, NULL};
    ULONG fMask = SEE_MASK_FLAG_NO_UI|SEE_MASK_FORCENOIDLIST;
    if(!(SHGetAppCompatFlags(ACF_WIN95SHLEXEC) & ACF_WIN95SHLEXEC))
        fMask |= SEE_MASK_FLAG_DDEWAIT;
    sei.fMask = fMask;

    TraceMsg(TF_SHELLEXEC, "ShellExecuteA(%04X, %S, %S, %S, %S, %d)", hwnd,
        SAFE_DEBUGSTR(lpOp), SAFE_DEBUGSTR(lpFile), SAFE_DEBUGSTR(lpArgs),
        SAFE_DEBUGSTR(lpDir), nShowCmd);

    ShellExecuteExA(&sei);
    return sei.hInstApp;
}
#else
HINSTANCE  APIENTRY ShellExecuteW(
    HWND  hwnd,
    LPCWSTR lpOp,
    LPCWSTR lpFile,
    LPCWSTR lpArgs,
    LPCWSTR lpDir,
    INT nShowCmd)
{
    // NB The FORCENOIDLIST flag stops us from going through the ShellExecPidl()
    // code (for backwards compatability with progman).
    // DDEWAIT makes us synchronous, and gets around threads without
    // msg pumps and ones that are killed immediately after shellexec()
    SHELLEXECUTEINFOW sei = { SIZEOF(SHELLEXECUTEINFOW), 0, hwnd, lpOp, lpFile, lpArgs, lpDir, nShowCmd, NULL};
    ULONG fMask = SEE_MASK_FLAG_NO_UI|SEE_MASK_FORCENOIDLIST;
    if(!(SHGetAppCompatFlags(ACF_WIN95SHLEXEC) & ACF_WIN95SHLEXEC))
        fMask |= SEE_MASK_FLAG_DDEWAIT;
    sei.fMask = fMask;

    TraceMsg(TF_SHELLEXEC, "ShellExecuteA(%04X, %S, %S, %S, %S, %d)", hwnd, lpOp, lpFile, lpArgs, lpDir, nShowCmd);

    ShellExecuteExW(&sei);
    return sei.hInstApp;
}
#endif

//----------------------------------------------------------------------------
// ShellExecuteEx: Extended Shell execute.


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Returns TRUE if the specified app is listed under the specified key
extern BOOL IsNameListedUnderKey(LPCTSTR pszFileName, LPCTSTR pszKey)
{
    HKEY hkey;
    int iValue = 0;
    TCHAR szValue[MAX_PATH];
    TCHAR szData[MAX_PATH];
    DWORD dwType;
    DWORD cbData;
    DWORD cchValue;

    // Enum through the list of apps.

    if (RegOpenKey(HKEY_CURRENT_USER, pszKey, &hkey) == ERROR_SUCCESS)
    {
            cbData = SIZEOF(szData);
            cchValue = ARRAYSIZE(szValue);
            while (RegEnumValue(hkey, iValue, szValue, &cchValue, NULL, &dwType,
                    (LPBYTE)szData, &cbData) == ERROR_SUCCESS)
            {
                    if (lstrcmpi(szData, pszFileName) == 0)
                    {
                            RegCloseKey(hkey);
                            return TRUE;
                    }
                    cbData = SIZEOF(szData);
                    cchValue = ARRAYSIZE(szValue);
                    iValue++;
            }
            RegCloseKey(hkey);
    }

    // End of the list...
    return FALSE;
}




#define REGSTR_PATH_POLICIES_EXPLORER REGSTR_PATH_POLICIES TEXT("\\Explorer\\RestrictRun")
#define REGSTR_PATH_POLICIES_EXPLORER_DISALLOW REGSTR_PATH_POLICIES TEXT("\\Explorer\\DisallowRun")

//----------------------------------------------------------------------------
// Returns TRUE if the specified app is not on the list of unrestricted apps.
BOOL RestrictedApp(LPCTSTR pszApp)
{
    LPTSTR pszFileName;

    pszFileName = PathFindFileName(pszApp);

    TraceMsg(TF_SHELLEXEC, "RestrictedApp: %s ", pszFileName);

    // Special cases:
    //     Apps you can always run.
    if (lstrcmpi(pszFileName, c_szRunDll) == 0)
        return FALSE;

    if (lstrcmpi(pszFileName, TEXT("systray.exe")) == 0)
        return FALSE;

    return !IsNameListedUnderKey(pszFileName, REGSTR_PATH_POLICIES_EXPLORER);
}

//----------------------------------------------------------------------------
// Returns TRUE if the specified app is on the list of disallowed apps.
BOOL DisallowedApp(LPCTSTR pszApp)
{
    LPTSTR pszFileName;

    pszFileName = PathFindFileName(pszApp);

    TraceMsg(TF_SHELLEXEC, "DisallowedApp: %s ", pszFileName);

    return IsNameListedUnderKey(pszFileName, REGSTR_PATH_POLICIES_EXPLORER_DISALLOW);
}


//----------------------------------------------------------------------------
// Returns TRUE if the system has FAT32 drives.

BOOL HasFat32Drives()
{
    static BOOL fHasFat32Drives = -1; // -1 means unverified.
    int         iDrive;

    if (fHasFat32Drives != -1)
        return fHasFat32Drives;

    // Assume false
    fHasFat32Drives = FALSE;

    for (iDrive = 0; iDrive < 26; iDrive++)
    {
        TCHAR szDriveName[4];

        // BUGBUG (scotth): lets make PathBuildRoot return aligned strings
        if (GetDriveType((LPTSTR)PathBuildRoot(szDriveName, iDrive)) == DRIVE_FIXED)
        {
            TCHAR szFileSystemName[12];

            if (GetVolumeInformation(szDriveName, NULL, 0, NULL, NULL, NULL,
                                     szFileSystemName, sizeof(szFileSystemName)))
            {
                if (lstrcmpi(szFileSystemName, TEXT("FAT32"))==0)
                {
                    fHasFat32Drives = TRUE;
                    return fHasFat32Drives;
                }
            }
        }
    }

    return fHasFat32Drives;
}


typedef struct {
    // local data
    HWND          hDlg;
    // parameters
    DWORD         dwHelpId;
    LPCTSTR       lpszTitle;
    DWORD         dwResString;
    BOOL          fHardBlock;
    BOOL          fDone;
} APPCOMPATDLG_DATA, *PAPPCOMPATDLG_DATA;


BOOL_PTR CALLBACK AppCompat_DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PAPPCOMPATDLG_DATA lpdata = (PAPPCOMPATDLG_DATA)GetWindowLongPtr(hDlg, DWLP_USER);
    DWORD aHelpIDs[4];

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            TCHAR szMsgText[2048];

            /* The title will be in the lParam. */
            lpdata = (PAPPCOMPATDLG_DATA)lParam;
            lpdata->hDlg = hDlg;
            if (lpdata->fHardBlock)
            {
                // Disable the "Run" button.
                EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
            }
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)lpdata);
            SetWindowText(hDlg, lpdata->lpszTitle);

            LoadString(HINST_THISDLL, lpdata->dwResString, szMsgText, ARRAYSIZE(szMsgText));
            SetDlgItemText(hDlg, IDD_LINE_1, szMsgText);
            return TRUE;
        }

    case WM_DESTROY:
        break;

    case WM_HELP:
//        WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, TEXT("apps.chm>Proc4"), HELP_CONTEXT, 0);
        HtmlHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, TEXT("apps.chm>Proc4"), HELP_CONTEXT, 0);        
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDHELP:
            aHelpIDs[0]=IDHELP;
            aHelpIDs[1]=lpdata->dwHelpId;
            aHelpIDs[2]=0;
            aHelpIDs[3]=0;

//            WinHelp(hDlg, TEXT("apps.chm>Proc4"), HELP_CONTEXT, (DWORD)lpdata->dwHelpId);
            HtmlHelp(hDlg, TEXT("apps.chm>Proc4"), HH_HELP_CONTEXT, (DWORD)lpdata->dwHelpId);            
            break;

        case IDD_COMMAND:
            case IDOK:
                if (IsDlgButtonChecked(hDlg, IDD_STATE))
                    EndDialog(hDlg, 0x8000 | IDOK);
                else
                    EndDialog(hDlg, IDOK);
                break;

            case IDCANCEL:
                EndDialog(hDlg, IDCANCEL);
                break;

            default:
                return FALSE;
        }
        break;

        default:
            return FALSE;
    }
    return TRUE;
}


 
BOOL _GetAppCompatData(LPCTSTR pszAppPath, LPCTSTR pszAppName, LPCTSTR *ppszNewEnvString, HKEY hkApp, APPCOMPATDLG_DATA *pdata, LPTSTR pszValue, DWORD cchValue)
{
    BOOL fRet = FALSE;
    BOOL fBreakOutOfTheLoop=FALSE;
    int iValue;


    // Enum keys under this app name and check for dependant files.
    for (iValue = 0; !fBreakOutOfTheLoop; iValue++)
    {
        DWORD cch = cchValue;
        DWORD dwType;
        void *pvData;
        DWORD cbData;
        LONG lResult;

        lResult = RegEnumValue(hkApp, iValue, pszValue, &cch, NULL, &dwType, NULL, &cbData);
        if ((lResult != NOERROR) && (lResult != ERROR_MORE_DATA))
        {
            //  no more values
            break;
        }

        //  insure this is our kind of data
        if (dwType != REG_BINARY)
            continue;          

        pvData = GlobalAlloc(GPTR, cbData);

        if (pvData)
        {
            cch = cchValue;
            if (NOERROR == RegEnumValue(hkApp, iValue, pszValue, &cch, NULL, 
                                &dwType, pvData, &cbData))
            {
                BADAPP_DATA badAppData;
                BADAPP_PROP badAppProp;
                badAppProp.Size = sizeof(BADAPP_PROP);
                badAppData.Size = sizeof(BADAPP_DATA);
                badAppData.FilePath = pszAppPath;
                badAppData.Blob = pvData;
                badAppData.BlobSize = cbData;

                if (SHIsBadApp(&badAppData, &badAppProp))
                {
                    //
                    // we found a bad app
                    //
                    pdata->dwHelpId = badAppProp.MsgId;
                    pdata->lpszTitle = pszAppName;

                    fRet=TRUE;
                    
                    // Map ids to message strings for the various platforms we run on
                    switch (badAppProp.AppType & APPTYPE_TYPE_MASK) 
                    {
                        case APPTYPE_MINORPROBLEM:
                            pdata->dwResString = IDS_APPCOMPATWIN95L;
                            break;
                        case APPTYPE_INC_HARDBLOCK:
                            pdata->fHardBlock = TRUE;
                            pdata->dwResString = IDS_APPCOMPATWIN95H;
                            break;
                        case APPTYPE_INC_NOBLOCK:
                            pdata->dwResString = IDS_APPCOMPATWIN95;
                            break;

#ifdef WINNT
                        case APPTYPE_VERSIONSUB:
                        {
                            static LPCTSTR VersionFlavors[] = {
                                    TEXT("_COMPAT_VER_NNN=4,0,1381,3,0,2,Service Pack 3"),
                                    TEXT("_COMPAT_VER_NNN=4,0,1381,4,0,2,Service Pack 4"),
                                    TEXT("_COMPAT_VER_NNN=4,0,1381,5,0,2,Service Pack 5"),
                                    TEXT("_COMPAT_VER_NNN=4,0,950,0,0,1"),
                                    0};

                            //
                            // Is the ID within the number of strings we have?
                            //
                            if ( badAppProp.MsgId <= (sizeof(VersionFlavors) / sizeof(LPTSTR) - 1) )
                            {
                                *ppszNewEnvString = VersionFlavors[badAppProp.MsgId];
                            }

                            fRet = FALSE;
                        }
                        break;
                    
                        case APPTYPE_SHIM:
                            
                            //
                            // If there is a shim for this app do not display
                            // any message
                            //
                            fRet = FALSE;
                            break;
#endif

                            
                        default:
                            continue;
                    }

                    //  this will break us out
                    fBreakOutOfTheLoop = TRUE;
                }
            }

            GlobalFree((HANDLE)pvData);
        }
    }
    return fRet;
}


typedef enum {
    SEV_DEFAULT = 0,
    SEV_LOW,
    SEV_HARD,
} SEVERITY;

BOOL _GetBadAppData(LPCTSTR pszAppPath, LPCTSTR pszAppName, HKEY hkApp, APPCOMPATDLG_DATA *pdata, LPTSTR pszValue, DWORD cchValue)
{
    BOOL fRet = FALSE;
    int iValue;
    TCHAR szPath[MAX_PATH];
    DWORD cchPath;
    LPTSTR pchCopyToPath;

    // Get directory of this app so that we can check for dependant files.
    StrCpyN(szPath, pszAppPath, ARRAYSIZE(szPath));
    PathRemoveFileSpec(szPath);
    PathAddBackslash(szPath);
    cchPath = lstrlen(szPath);
    pchCopyToPath = &szPath[cchPath];
    cchPath = ARRAYSIZE(szPath) - cchPath;
        
    for (iValue = 0; !fRet; iValue++)
    {
        DWORD cch = cchValue;
        TCHAR szData[MAX_PATH];
        DWORD cbData = SIZEOF(szData);
        DWORD dwType;
        if (NOERROR == RegEnumValue(hkApp, iValue, pszValue, &cch, NULL, &dwType,
                            (LPBYTE)szData, &cbData))
        {
            // Fully qualified path to dependant file
            StrCpyN(pchCopyToPath, pszValue, cchPath);
            
            // * means match any file.
            if (pszValue[0] == TEXT('*') || PathFileExistsAndAttributes(szPath, NULL))
            {
                DWORD rgData[2];
                DWORD dwHelpId = StrToInt(szData);
                SEVERITY sev = SEV_DEFAULT;

                // Get the flags...
                lstrcpy(szData, TEXT("Flags"));
                StrCatBuff(szData, pszValue, ARRAYSIZE(szData));
                
                cbData = SIZEOF(szData);
                if (SHQueryValueEx(hkApp, szData, NULL, &dwType, (LPBYTE)szData, &cbData) == ERROR_SUCCESS && cbData >= 1)
                {
                    if (StrChr(szData, TEXT('L')))
                        sev = SEV_LOW;

                    if (StrChr(szData, TEXT('Y')))
                        sev = SEV_HARD;

                    if ((StrChr(szData, TEXT('N')) && !(GetSystemMetrics(SM_NETWORK) | RNC_NETWORKS))
                    ||  (StrChr(szData, TEXT('F')) && !HasFat32Drives()))
                    {
                        continue;
                    }
                }

                // Check the version if any...
                lstrcpy(szData, TEXT("Version"));
                StrCatBuff(szData, pszValue, ARRAYSIZE(szData));
                cbData = SIZEOF(rgData);
                if (SHQueryValueEx(hkApp, szData, NULL, &dwType, (LPBYTE)rgData, &cbData) == ERROR_SUCCESS 
                && (cbData == 8))
                {
                    DWORD dwVerLen, dwVerHandle;
                    DWORD dwMajorVer, dwMinorVer;
                    DWORD dwBadMajorVer, dwBadMinorVer;
                    LPTSTR lpVerBuffer;
                    BOOL  fBadApp = FALSE;

                    // What is a bad version according to the registry key?
                    dwBadMajorVer = rgData[0];
                    dwBadMinorVer = rgData[1];

                    // If no version resource can be found, assume 0.
                    dwMajorVer = 0;
                    dwMinorVer = 0;

                    // Version data in inf file should be of the form 8 bytes
                    // Major Minor
                    // 3.10  10.10
                    // 40 30 20 10 is 10 20 30 40 in registry
                    // cast const -> non const
                    if (0 != (dwVerLen = GetFileVersionInfoSize((LPTSTR)pszAppPath, &dwVerHandle)))
                    {
                        lpVerBuffer = (LPTSTR)GlobalAlloc(GPTR, dwVerLen);
                        if (lpVerBuffer)
                        {
                            VS_FIXEDFILEINFO *pffi = NULL;
                            UINT             cb;

                            if (GetFileVersionInfo((LPTSTR)pszAppPath, dwVerHandle, dwVerLen, lpVerBuffer) &&
                                VerQueryValue(lpVerBuffer, TEXT("\\"), &pffi, &cb))
                            {
                                dwMajorVer = pffi->dwProductVersionMS;
                                dwMinorVer = pffi->dwProductVersionLS;
                            }

                            GlobalFree((HANDLE)lpVerBuffer);
                        }
                    }

                    if (dwMajorVer < dwBadMajorVer)
                        fBadApp = TRUE;
                    else if ((dwMajorVer == dwBadMajorVer) && (dwMinorVer <= dwBadMinorVer))
                        fBadApp = TRUE;

                    if (!fBadApp)
                    {
                        // This dude is ok
                        continue;
                    }
                }

                pdata->dwHelpId = dwHelpId;
                pdata->lpszTitle = pszAppName;

                // Map ids to message strings for the various platforms we run on
                switch (sev)
                {
                case SEV_LOW:
                    pdata->dwResString = IDS_APPCOMPATWIN95L;
                    break;

                case SEV_HARD:
                    pdata->fHardBlock = TRUE;
                    pdata->dwResString = IDS_APPCOMPATWIN95H;
                    break;

                default:
                    pdata->dwResString = IDS_APPCOMPATWIN95;
                }

                // this will break us out
                fRet = TRUE;
            }
        }
        else
            break;
    }

    return fRet;
}

HKEY _OpenBadAppKey(LPCTSTR pszApp, LPCTSTR pszName)
{
    HKEY hkBad = NULL;
    DWORD dwAppVersion = GetExeType(pszApp);

    ASSERT(pszApp && *pszApp && pszName && *pszName);

    if (HIWORD(dwAppVersion) < 0x0400)
    {
        // Check the reg key for apps older than 4.00
        RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_CHECKBADAPPSNEW, &hkBad);
    }
    else if (HIWORD(dwAppVersion) == 0x0400)
    {
        // Check the reg key for apps == 4.00
        RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_CHECKBADAPPS400NEW, &hkBad);
    }
    //  else
        // Newer than 4.0 so all should be fine.

    if (hkBad)
    {
        // Check for the app name
        HKEY hkRet = NULL;
        RegOpenKey(hkBad, pszName, &hkRet);
        RegCloseKey(hkBad);
        return hkRet;
    }

    return NULL;
}


HKEY _CheckBadApps(LPCTSTR pszAppPath, LPCTSTR pszAppName, APPCOMPATDLG_DATA *pdata, LPTSTR pszValue, DWORD cchValue)
{
    HKEY hkApp = _OpenBadAppKey(pszAppPath, pszAppName);

    if (hkApp)
    {
        TraceMsg(TF_SHELLEXEC, "CheckBadApps() maybe is bad %s", pszAppName);
        if (_GetBadAppData(pszAppPath, pszAppName, hkApp, pdata, pszValue, cchValue))
            return hkApp;
            
        RegCloseKey(hkApp);
    }

    return NULL;
}

HKEY _OpenAppCompatKey(LPCTSTR pszAppName)
{
    TCHAR sz[MAX_PATH];
    HKEY hkRet = NULL;
    wnsprintf(sz, SIZECHARS(sz), REGSTR_TEMP_APPCOMPATPATH, pszAppName);

    RegOpenKey(HKEY_LOCAL_MACHINE, sz, &hkRet);

    return hkRet;
}

HKEY _CheckAppCompat(LPCTSTR pszAppPath, LPCTSTR pszAppName, LPCTSTR *ppszNewEnvString, APPCOMPATDLG_DATA *pdata, LPTSTR pszValue, DWORD cchValue)
{
    HKEY hkApp = _OpenAppCompatKey(pszAppName);

    if (hkApp)
    {
        TraceMsg(TF_SHELLEXEC, "CheckAppCompat() maybe is bad %s", pszAppName);
        if (_GetAppCompatData(pszAppPath, pszAppName, ppszNewEnvString, hkApp, pdata, pszValue, cchValue))
            return hkApp;
            
        RegCloseKey(hkApp);
    }

    return NULL;
}
        
// Returns FALSE if app is fatally incompatible

BOOL CheckAppCompatibility(LPCTSTR pszApp, LPCTSTR *ppszNewEnvString, BOOL fNoUI, HWND hwnd)
{
    BOOL fRet = TRUE;
    // If no app name, then nothing to check, so pretend it's a good app.
    // Must check now or RegOpenKey will get a null string and behave
    // "nonintuitively".  (If you give RegOpenKey a null string, it
    // returns the same key back and does *not* bump the refcount.)

    if (pszApp && *pszApp)
    {
        LPCTSTR pszFileName = PathFindFileName(pszApp);

        if (pszFileName && *pszFileName)
        {
            APPCOMPATDLG_DATA data = {0};
            TCHAR szValue[MAX_PATH];
            HKEY hkBad = _CheckAppCompat(pszApp, pszFileName, ppszNewEnvString, &data, szValue, ARRAYSIZE(szValue));

            if (!hkBad)
                hkBad = _CheckBadApps(pszApp, pszFileName, &data, szValue, ARRAYSIZE(szValue));


            if (hkBad)
            {
                TraceMsg(TF_SHELLEXEC, "BADAPP %s", pszFileName);
                
                if (fNoUI && !hwnd)
                {
                    //
                    //  LEGACY - we just let soft blocks right on through - ZekeL - 27-MAY-99
                    //  the NOUI flag is usually passed by apps when they 
                    //  have very specific behavior they are looking for.
                    //  if that is the case we should probably just defer to them
                    //  unless we know it is really bad.
                    //
                    if (data.fHardBlock)
                        fRet = FALSE;
                    else
                        fRet = TRUE;
                }
                else
                {
                    int iRet = (int)DialogBoxParam(HINST_THISDLL,
                                            MAKEINTRESOURCE(DLG_APPCOMPAT),
                                            hwnd, AppCompat_DlgProc, (LPARAM)&data);

                    if (iRet & 0x8000)
                    {
                        // Delete so we don't warn again.
                        RegDeleteValue(hkBad, szValue);
                    }

                    if ((iRet & 0x0FFF) != IDOK)
                        fRet = FALSE;
                }

                RegCloseKey(hkBad);
            }
        }

    }

    return fRet;
}


//----------------------------------------------------------------------------
HINSTANCE MapWin32ErrToHINST(UINT errWin32)
{
    HINSTANCE hinst;

    switch (errWin32) {
    case ERROR_SHARING_VIOLATION:
        hinst = (HINSTANCE)SE_ERR_SHARE;
        break;

    case ERROR_OUTOFMEMORY:             // 14
        hinst = (HINSTANCE)SE_ERR_OOM;  // 8
        break;

    case ERROR_BAD_PATHNAME:
    case ERROR_BAD_NETPATH:
    case ERROR_PATH_BUSY:
    case ERROR_NO_NET_OR_BAD_PATH:
        hinst = (HINSTANCE)SE_ERR_PNF;
        break;

    case ERROR_OLD_WIN_VERSION:
        hinst = (HINSTANCE)10;
        break;

    case ERROR_APP_WRONG_OS:
        hinst = (HINSTANCE)12;
        break;

    case ERROR_RMODE_APP:
        hinst = (HINSTANCE)15;
        break;

    case ERROR_SINGLE_INSTANCE_APP:
        hinst = (HINSTANCE)16;
        break;

    case ERROR_INVALID_DLL:
        hinst = (HINSTANCE)20;
        break;

    case ERROR_NO_ASSOCIATION:
        hinst = (HINSTANCE)SE_ERR_NOASSOC;
        break;

    case ERROR_DDE_FAIL:
        hinst = (HINSTANCE)SE_ERR_DDEFAIL;
        break;

    case ERROR_DLL_NOT_FOUND:
        hinst = (HINSTANCE)SE_ERR_DLLNOTFOUND;
        break;

    default:
        hinst = (HINSTANCE)errWin32;
        if (errWin32 >= SE_ERR_SHARE)
            hinst = (HINSTANCE)ERROR_ACCESS_DENIED;

        break;
    }

    return hinst;
}

UINT MapHINSTToWin32Err(HINSTANCE hinst)
{
    UINT errWin32;

    switch (PtrToUlong(hinst)) {
    case SE_ERR_SHARE:
        errWin32 = ERROR_SHARING_VIOLATION;
        break;

    case 10:
        errWin32 = ERROR_OLD_WIN_VERSION;
        break;

    case 12:
        errWin32 = ERROR_APP_WRONG_OS;
        break;

    case 15:
        errWin32 = ERROR_RMODE_APP;
        break;

    case 16:
        errWin32 = ERROR_SINGLE_INSTANCE_APP;
        break;

    case 20:
        errWin32 = ERROR_INVALID_DLL;
        break;

    case SE_ERR_NOASSOC:
        errWin32 = ERROR_NO_ASSOCIATION;
        break;

    case SE_ERR_DDEFAIL:
        errWin32 = ERROR_DDE_FAIL;
        break;

    case SE_ERR_DLLNOTFOUND:
        errWin32 = ERROR_DLL_NOT_FOUND;
        break;

    default:
        errWin32 = PtrToUlong(hinst);
        break;
    }

    return errWin32;
}


#ifndef NO_SHELLEXECUTE_HOOK

/*
 * Returns:
 *    S_OK or error.
 *    *phrHook is hook result if S_OK is returned, otherwise it is S_FALSE.
 */
HRESULT
InvokeShellExecuteHook(
    LPCLSID pclsidHook,
    LPSHELLEXECUTEINFO pei,
    HRESULT *phrHook)
{
    HRESULT hr;
    IUnknown *punk;

    *phrHook = S_FALSE;

    hr = SHExtCoCreateInstance(NULL, pclsidHook, NULL, &IID_IUnknown, &punk);

    if (hr == S_OK)
    {
        IShellExecuteHook *pshexhk;

        hr = punk->lpVtbl->QueryInterface(punk, &IID_IShellExecuteHook, &pshexhk);

        if (hr == S_OK)
        {
            *phrHook = pshexhk->lpVtbl->Execute(pshexhk, pei);

            pshexhk->lpVtbl->Release(pshexhk);
        }
#ifdef UNICODE
        else
        {
            IShellExecuteHookA *pshexhkA;

            hr = punk->lpVtbl->QueryInterface(punk, &IID_IShellExecuteHookA,
                                              &pshexhkA);

            if (SUCCEEDED(hr))
            {
                SHELLEXECUTEINFOA seia;
                UINT cchVerb = 0;
                UINT cchFile = 0;
                UINT cchParameters = 0;
                UINT cchDirectory  = 0;
                UINT cchClass = 0;
                LPSTR lpszBuffer;

                seia = *(SHELLEXECUTEINFOA*)pei;    // Copy all of the binary data

                if (pei->lpVerb)
                {
                    cchVerb = WideCharToMultiByte(CP_ACP,0,
                                                  pei->lpVerb, -1,
                                                  NULL, 0,
                                                  NULL, NULL) + 1;
                }

                if (pei->lpFile)
                    cchFile = WideCharToMultiByte(CP_ACP,0,
                                                  pei->lpFile, -1,
                                                  NULL, 0,
                                                  NULL, NULL)+1;

                if (pei->lpParameters)
                    cchParameters = WideCharToMultiByte(CP_ACP,0,
                                                        pei->lpParameters, -1,
                                                        NULL, 0,
                                                        NULL, NULL)+1;

                if (pei->lpDirectory)
                    cchDirectory = WideCharToMultiByte(CP_ACP,0,
                                                       pei->lpDirectory, -1,
                                                       NULL, 0,
                                                       NULL, NULL)+1;
                if (_UseClassName(pei->fMask) && pei->lpClass)
                    cchClass = WideCharToMultiByte(CP_ACP,0,
                                                   pei->lpClass, -1,
                                                   NULL, 0,
                                                   NULL, NULL)+1;

                lpszBuffer = alloca(cchVerb+cchFile+cchParameters+cchDirectory+cchClass);

                seia.lpVerb = NULL;
                seia.lpFile = NULL;
                seia.lpParameters = NULL;
                seia.lpDirectory = NULL;
                seia.lpClass = NULL;

                //
                // Convert all of the strings to ANSI
                //
                if (pei->lpVerb)
                {
                    WideCharToMultiByte(CP_ACP, 0, pei->lpVerb, -1,
                                        lpszBuffer, cchVerb, NULL, NULL);
                    seia.lpVerb = lpszBuffer;
                    lpszBuffer += cchVerb;
                }
                if (pei->lpFile)
                {
                    WideCharToMultiByte(CP_ACP, 0, pei->lpFile, -1,
                                        lpszBuffer, cchFile, NULL, NULL);
                    seia.lpFile = lpszBuffer;
                    lpszBuffer += cchFile;
                }
                if (pei->lpParameters)
                {
                    WideCharToMultiByte(CP_ACP, 0,
                                        pei->lpParameters, -1,
                                        lpszBuffer, cchParameters, NULL, NULL);
                    seia.lpParameters = lpszBuffer;
                    lpszBuffer += cchParameters;
                }
                if (pei->lpDirectory)
                {
                    WideCharToMultiByte(CP_ACP, 0,
                                        pei->lpDirectory, -1,
                                        lpszBuffer, cchDirectory, NULL, NULL);
                    seia.lpDirectory = lpszBuffer;
                    lpszBuffer += cchDirectory;
                }
                if (_UseClassName(pei->fMask) && pei->lpClass)
                {
                    WideCharToMultiByte(CP_ACP, 0,
                                        pei->lpClass, -1,
                                        lpszBuffer, cchClass, NULL, NULL);
                    seia.lpClass = lpszBuffer;
                }

                *phrHook = pshexhkA->lpVtbl->Execute(pshexhkA, &seia );

                pei->hInstApp = seia.hInstApp;
                // hook may set hProcess (e.g. CURLExec creates dummy process
                // to signal IEAK that IE setup failed -- in browser only mode)
                pei->hProcess = seia.hProcess;

                pshexhkA->lpVtbl->Release(pshexhkA);
            }
        }
#endif
        punk->lpVtbl->Release(punk);
    }

    return(hr);
}

const TCHAR c_szShellExecuteHooks[] = REGSTR_PATH_EXPLORER TEXT("\\ShellExecuteHooks");

/*
 * Returns:
 *    S_OK     Execution handled by hook.  pei->hInstApp filled in.
 *    S_FALSE  Execution not handled by hook.  pei->hInstApp not filled in.
 *    E_...    Error during execution by hook.  pei->hInstApp filled in.
 */
HRESULT TryShellExecuteHooks(LPSHELLEXECUTEINFO pei)
{
    HRESULT hr = S_FALSE;
    HKEY hkeyHooks;

    // Enumerate the list of hooks.  A hook is registered as a GUID value of the
    // c_szShellExecuteHooks key.

    if (RegOpenKey(HKEY_LOCAL_MACHINE, c_szShellExecuteHooks, &hkeyHooks)
        == ERROR_SUCCESS)
    {
        DWORD dwiValue;
        TCHAR szCLSID[GUIDSTR_MAX];
        DWORD dwcbCLSIDLen;

        // Invoke each hook.  A hook returns S_FALSE if it does not handle the
        // exec.  Stop when a hook returns S_OK (handled) or an error.

        for (dwcbCLSIDLen = SIZEOF(szCLSID), dwiValue = 0;
             RegEnumValue(hkeyHooks, dwiValue, szCLSID, &dwcbCLSIDLen, NULL,
                          NULL, NULL, NULL) == ERROR_SUCCESS;
             dwcbCLSIDLen = SIZEOF(szCLSID), dwiValue++)
        {
            CLSID clsidHook;

            if (SUCCEEDED(SHCLSIDFromString(szCLSID, &clsidHook)))
            {
                HRESULT hrHook;

                if (InvokeShellExecuteHook(&clsidHook, pei, &hrHook) == S_OK &&
                    hrHook != S_FALSE)
                {
                    hr = hrHook;
                    break;
                }
            }
        }

        RegCloseKey(hkeyHooks);
    }

    ASSERT(hr == S_FALSE ||
           (hr == S_OK && ISSHELLEXECSUCCEEDED(pei->hInstApp)) ||
           (FAILED(hr) && ! ISSHELLEXECSUCCEEDED(pei->hInstApp)));

    return(hr);
}

#endif   // ! NO_SHELLEXECUTE_HOOK


STDAPI InvokeInProcExec(IContextMenu *pcm, LPSHELLEXECUTEINFO pei, LPCTSTR pszDefVerb)
{
    HRESULT hres = E_OUTOFMEMORY;

    HMENU hmenu = CreatePopupMenu();
    if (hmenu)
    {
        CMINVOKECOMMANDINFOEX ici;
        void * pvFree;
        
        if (SUCCEEDED(SEI2ICIX(pei, &ici, &pvFree)))
        {
            UINT uFlags;
            BOOL fDefVerb;
            CHAR szDefVerbAnsi[128];

            ici.fMask |= CMIC_MASK_FLAG_NO_UI;
        
#define CMD_ID_FIRST    1
#define CMD_ID_LAST     0x7fff

            fDefVerb = (ici.lpVerb == NULL || *ici.lpVerb == 0);

            // This optimization eliminate creating handlers that
            // will not change the default verb

            if (fDefVerb)
                uFlags = CMF_VERBSONLY | CMF_DEFAULTONLY;
            else
                uFlags = 0;

            pcm->lpVtbl->QueryContextMenu(pcm, hmenu, 0, CMD_ID_FIRST, CMD_ID_LAST, uFlags);

            if (fDefVerb)
            {
                UINT idCmd = GetMenuDefaultItem(hmenu, MF_BYCOMMAND, 0);
                if (-1 == idCmd)
                {
                    if (pszDefVerb)
                    {
                        SHTCharToAnsi(pszDefVerb, szDefVerbAnsi, ARRAYSIZE(szDefVerbAnsi));
                        ici.lpVerb = szDefVerbAnsi;
                    }
                    else
                        ici.lpVerb = (LPSTR)MAKEINTRESOURCE(0);  // best guess
                }
                else
                    ici.lpVerb = (LPSTR)MAKEINTRESOURCE(idCmd - CMD_ID_FIRST);
            }
            
            SetLastError(ERROR_SUCCESS);

            hres = pcm->lpVtbl->InvokeCommand(pcm, (LPCMINVOKECOMMANDINFO)&ici);

            // Assume success
            pei->hInstApp = (HINSTANCE)42;
            if (FAILED(hres))
            {
                UINT errWin32 = GetLastError();
                if (errWin32 != ERROR_SUCCESS)
                {
                    // Assume that the InvokeCommand set the last
                    // error properly. (Such as when we wind up
                    // calling back into ShellExecuteEx.)

                    pei->hInstApp = MapWin32ErrToHINST(errWin32);
                }
            }

            if (pvFree)
                LocalFree(pvFree);
        }
        
        DestroyMenu(hmenu);
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: Bind to the specified CLSID's IContextMenu handler.

Returns:
Cond:    --
*/
HRESULT
BindToInProcHandler(
    IN  LPSHELLEXECUTEINFO pei,
    IN  LPCLSID            pclsid,
    IN  HKEY               hkeyClass,
    OUT IContextMenu **    ppcm)
{
    HRESULT hres = E_OUTOFMEMORY;
    LPITEMIDLIST pidl;

    ASSERT(pei);
    ASSERT(pclsid);
    ASSERT(ppcm);

    *ppcm = NULL;

    pidl = ILCreateFromPath(pei->lpFile);
    if (pidl)
    {
        IShellExtInit * psei;

        hres = SHExtCoCreateInstance(NULL, pclsid, NULL, &IID_IShellExtInit, &psei);
        if (SUCCEEDED(hres))
        {
            IShellFolder * psf;
            LPITEMIDLIST pidlLast;

            hres = SHBindToIDListParent(pidl, &IID_IShellFolder, &psf,
                                        &pidlLast);
            if (SUCCEEDED(hres))
            {
                IDataObject * pdtobj;

                // Get the data object
                hres = psf->lpVtbl->GetUIObjectOf(psf, pei->hwnd, 1, &pidlLast,
                                                  &IID_IDataObject, NULL, &pdtobj);
                if (SUCCEEDED(hres))
                {
                    ILRemoveLastID(pidl);

                    hres = psei->lpVtbl->Initialize(psei, pidl, pdtobj, hkeyClass);
                    if (SUCCEEDED(hres))
                        hres = psei->lpVtbl->QueryInterface(psei, &IID_IContextMenu, ppcm);

                    pdtobj->lpVtbl->Release(pdtobj);
                }

                psf->lpVtbl->Release(psf);
            }
            psei->lpVtbl->Release(psei);
        }
        ILFree(pidl);
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: Look in class\shell\verb for the CLSID value
         to see if we can execute this command in-process.
         If we can, we will bind to IContextMenu and invoke
         the command.

Returns: S_FALSE if there is no in-proc handler
         S_OK on success of handler, or a failure code
Cond:    --
*/
STDAPI TryInProcess(
    IN LPSHELLEXECUTEINFO pei,
    IN HKEY               hkeyClass,
    IN LPCTSTR            pszClassVerb,     // "shell/open"
    IN LPCTSTR            pszVerb)          // "open"
{
    HRESULT hres = S_FALSE;
    HKEY hkey;

    ASSERT(pei);
    ASSERT(hkeyClass);
    ASSERT(pszClassVerb);

    if (NO_ERROR == RegOpenKeyEx(hkeyClass, pszClassVerb, 0, KEY_QUERY_VALUE, &hkey))
    {
        TCHAR szCLSID[GUIDSTR_MAX];
        DWORD cbData = SIZEOF(szCLSID);

        if (NO_ERROR == SHQueryValueEx(hkey, TEXT("CLSID"), NULL,
                                        NULL, (LPBYTE)szCLSID, &cbData))
        {
            CLSID clsid;

            if (SUCCEEDED(SHCLSIDFromString(szCLSID, &clsid)))
            {
                IContextMenu *pcm;

                hres = BindToInProcHandler(pei, &clsid, hkeyClass, &pcm);
                if (SUCCEEDED(hres))
                {
                    hres = InvokeInProcExec(pcm, pei, pszVerb);

                    // (we only return S_FALSE if there was no handler)
                    if (S_FALSE == hres)
                        hres = S_OK;

                    pcm->lpVtbl->Release(pcm);
                }
                else
                    hres = S_FALSE;
            }
        }

        RegCloseKey(hkey);
    }

    ASSERT(hres == S_FALSE ||
          (hres == S_OK && ISSHELLEXECSUCCEEDED(pei->hInstApp)) ||
          (FAILED(hres) && ! ISSHELLEXECSUCCEEDED(pei->hInstApp)));

    return hres;
}

// ShellExec for a pidl

BOOL _ShellExecPidl(LPSHELLEXECUTEINFO pei, LPITEMIDLIST pidlExec)
{
    HRESULT hres;

    LPITEMIDLIST pidlLast;
    IShellFolder *psf;

    hres = SHBindToIDListParent(pidlExec, &IID_IShellFolder, &psf, &pidlLast);
    if (SUCCEEDED(hres))
    {
        IContextMenu *pcm;

        hres = psf->lpVtbl->GetUIObjectOf(psf, pei->hwnd, 1, &pidlLast, &IID_IContextMenu, NULL, &pcm);
        if (SUCCEEDED(hres))
        {
            hres = InvokeInProcExec(pcm, pei, NULL);
            pcm->lpVtbl->Release(pcm);
        }
        psf->lpVtbl->Release(psf);
    }

    if (FAILED(hres))
    {
        UINT errWin32 = ERROR_SUCCESS;
        switch (hres) 
        {
        case E_OUTOFMEMORY:
            pei->hInstApp = (HINSTANCE)SE_ERR_OOM;
            errWin32 = ERROR_NOT_ENOUGH_MEMORY;
            break;

        default:
            // HACKHACK (lamadio): A canceled dialog is not an error. Just deal.
            // This would not have been a hack if we propogated the error messages out and
            // were consistant in our use of win32, ole and shell errors. Why so many? Who knows.
            if (GetLastError() == ERROR_CANCELLED)
            {
                hres = S_OK;
            }
            else
            {
                pei->hInstApp = (HINSTANCE)SE_ERR_ACCESSDENIED;
                errWin32 = ERROR_ACCESS_DENIED;
            }
            break;
        }

        SetLastError(errWin32);
    }
    return SUCCEEDED(hres);
}

BOOL InRunDllProcess(void)
{
    static BOOL s_fInRunDll = -1;

    if (-1 == s_fInRunDll)
    {
        TCHAR sz[MAX_PATH];
        s_fInRunDll = FALSE;
        if (GetModuleFileName(NULL, sz, SIZECHARS(sz)))
        {
            //  
            //  WARNING - rundll often seems to fail to add the DDEWAIT flag, and
            //  it often needs to since it is common to use rundll as a fire
            //  and forget process, and it exits too early.
            //
            
            if (StrStrI(sz, TEXT("rundll")))
                s_fInRunDll = TRUE;
        }
    }

    return s_fInRunDll;
}

//
// ShellExecuteEx
//
// returns TRUE if the execute succeeded, in which case
//   hInstApp should be the hinstance of the app executed (>32)
//   NOTE: in some cases the HINSTANCE cannot (currently) be determined.
//   In these cases, hInstApp is set to 42.
//
// returns FALSE if the execute did not succeed, in which case
//   GetLastError will contain error information
//   For backwards compatibility, hInstApp will contain the
//     best SE_ERR_ error information (<=32) possible.
//

extern BOOL CheckResourcesBeforeExec(void);

BOOL WINAPI ShellExecuteEx(LPSHELLEXECUTEINFO pei)
{
    BOOL fRet;
    DWORD errLast = NOERROR;

    // Don't freak out if CoInitializeEx fails; it just means we
    // can't do our shell hooks.
    HRESULT hrInit = SHCoInitialize();

    if (IS_VALID_STRUCT_PTR(pei, SHELLEXECUTEINFO) &&
        sizeof(*pei) == pei->cbSize)
    {
        // This internal bit prevents error message box reporting
        // when we recurse back into ShellExecuteEx
        ULONG ulOriginalMask = pei->fMask;
        pei->fMask |= SEE_MASK_FLAG_SHELLEXEC;

        if (!(pei->fMask & SEE_MASK_FLAG_DDEWAIT) && InRunDllProcess())
        {
            //  
            //  WARNING - rundll often seems to fail to add the DDEWAIT flag, and
            //  it often needs to since it is common to use rundll as a fire
            //  and forget process, and it exits too early.
            //
            pei->fMask |= (SEE_MASK_FLAG_DDEWAIT | SEE_MASK_WAITFORINPUTIDLE);
        }
    
        // This is semi-bogus, but before we exec something we should make sure that the
        // user heap has memory left.
        if (!CheckResourcesBeforeExec())
        {
            TraceMsg(TF_ERROR, "ShellExecuteEx - User said Low memory so return out of memory");
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            fRet= FALSE;
        }

        else if (_InvokeIDList(pei->fMask) && pei->lpIDList)
        {
            // _ShellExecPidl does its own SetLastError
            fRet = _ShellExecPidl(pei, pei->lpIDList);
        }
        else
        {
            // if _InvokeIDList, ShellExecuteNormal will create a pidl
            // and call _ShellExecPidl on that.

            // ShellExecuteNormal does its own SetLastError
            fRet = ShellExecuteNormal(pei);
        }

        // Mike's attempt to be consistent in error reporting:
        if (!fRet)
        {
            errLast = GetLastError();
            // we shouldn't put up errors on dll's not found.
            // this is handled WITHIN shellexecuteNormal because
            // sometimes kernel will put up the message for us, and sometimes
            // we need to.  we've put the curtion at ShellExecuteNormal
            if (errLast != ERROR_DLL_NOT_FOUND &&
                errLast != ERROR_CANCELLED)
            {
                _ShellExecuteError(pei, NULL, errLast);
            }
        }

        pei->fMask = ulOriginalMask;
    }
    else
    {
        // Failed parameter validation
        pei->hInstApp = (HINSTANCE)SE_ERR_ACCESSDENIED;
        errLast =  ERROR_ACCESS_DENIED;
        fRet = FALSE;
    }

    SHCoUninitialize(hrInit);

    if (errLast != NOERROR)
        SetLastError(errLast);
        
    return fRet;
}

#ifdef UNICODE

//+-------------------------------------------------------------------------
//
//  Function:   ShellExecuteExA
//
//  Synopsis:   Thunks ANSI call to ShellExecuteA to ShellExecuteW
//
//  Arguments:  [pei] -- pointer to an ANSI SHELLEXECUTINFO struct
//
//  Returns:    BOOL success value
//
//  History:    2-04-95   bobday   Created
//              2-06-95   davepl   Changed to ConvertStrings
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL WINAPI ShellExecuteExA(LPSHELLEXECUTEINFOA pei)
{
    BOOL    b;
    SHELLEXECUTEINFOW seiw;
    ThunkText * pThunkText;
    LPWSTR pwszFileAndUrl = NULL;

    memset( &seiw, 0, SIZEOF(SHELLEXECUTEINFOW) );

    // BUGBUG: We need many robustness checks here
    if (pei->cbSize != SIZEOF(SHELLEXECUTEINFOA))
    {
        pei->hInstApp = (HINSTANCE)SE_ERR_ACCESSDENIED;
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    seiw.cbSize = SIZEOF(SHELLEXECUTEINFOW);
    seiw.fMask = pei->fMask;
    seiw.hwnd  = pei->hwnd;
    seiw.nShow = pei->nShow;

    if ( pei->fMask & SEE_MASK_IDLIST )
        seiw.lpIDList = pei->lpIDList;

    //
    // CLASSNAME has a boolean value of (3) and CLASSKEY has a value of (1).  Since
    // name thus includes key, but does not imply it, we only copy when the classname
    // is not set.
    //

    if ( pei->fMask & SEE_MASK_CLASSKEY & !(pei->fMask & SEE_MASK_CLASSNAME))
        seiw.hkeyClass = pei->hkeyClass;

    if ( pei->fMask & SEE_MASK_HOTKEY )
        seiw.dwHotKey = pei->dwHotKey;
    if ( pei->fMask & SEE_MASK_ICON )
        seiw.hIcon = pei->hIcon;

    //
    // Thunk the text fields as appropriate
    //
    pThunkText =
      ConvertStrings( 6,
                      pei->lpVerb,
                      pei->lpFile,
                      pei->lpParameters,
                      pei->lpDirectory,
                      ((pei->fMask & SEE_MASK_HASLINKNAME) ||
                       (pei->fMask & SEE_MASK_HASTITLE) ||
                       (pei->fMask & SEE_MASK_CLASSNAME)) ? pei->lpClass : NULL,
                      (pei->fMask & SEE_MASK_RESERVED)  ? pei->hInstApp : NULL);

    if (NULL == pThunkText)
    {
        pei->hInstApp = (HINSTANCE)SE_ERR_OOM;  // BUGBUG (DavePl) More appropriate error code
        return FALSE;
    }

    //
    // Set our UNICODE text fields to point to the thunked strings
    //
    seiw.lpVerb         = pThunkText->m_pStr[0];
    seiw.lpFile         = pThunkText->m_pStr[1];
    seiw.lpParameters   = pThunkText->m_pStr[2];
    seiw.lpDirectory    = pThunkText->m_pStr[3];
    seiw.lpClass        = pThunkText->m_pStr[4];
    seiw.hInstApp       = (HINSTANCE)pThunkText->m_pStr[5];

    //
    // If we are passed the SEE_MASK_FILEANDURL flag, this means that
    // we have a lpFile parameter that has both the CacheFilename and the URL
    // (seperated by a single NULL, eg. "CacheFileName\0UrlName). We therefore
    // need to special case the thunking of pei->lpFile.
    //
    if (pei->fMask & SEE_MASK_FILEANDURL)
    {
        int iUrlLength;
        int iCacheFileLength = lstrlenW(pThunkText->m_pStr[1]);
        WCHAR wszURL[INTERNET_MAX_URL_LENGTH];
        LPSTR pszUrlPart = (LPSTR)&pei->lpFile[iCacheFileLength + 1];


        if (IsBadStringPtrA(pszUrlPart, INTERNET_MAX_URL_LENGTH) || !PathIsURLA(pszUrlPart))
        {
            ASSERT(FALSE);
        }
        else
        {
            // we have a vaild URL, so thunk it
            iUrlLength = lstrlenA(pszUrlPart);

            pwszFileAndUrl = LocalAlloc(LPTR, (iUrlLength + iCacheFileLength + 2) * SIZEOF(WCHAR));
            if (!pwszFileAndUrl)
            {
                pei->hInstApp = (HINSTANCE)SE_ERR_OOM;
                return FALSE;
            }

            SHAnsiToUnicode(pszUrlPart, wszURL, INTERNET_MAX_URL_LENGTH);

            // construct the wide multi-string
            lstrcpyW(pwszFileAndUrl, pThunkText->m_pStr[1]);
            lstrcpyW(&pwszFileAndUrl[iCacheFileLength + 1], wszURL);
            seiw.lpFile = pwszFileAndUrl;
        }
    }

    //
    // Call the real UNICODE ShellExecuteEx

    b = ShellExecuteEx(&seiw);

    pei->hInstApp = seiw.hInstApp;

    if (pei->fMask & SEE_MASK_NOCLOSEPROCESS)
        pei->hProcess = seiw.hProcess;

    LocalFree(pThunkText);
    if (pwszFileAndUrl)
        LocalFree(pwszFileAndUrl);

    return b;
}
#else
BOOL WINAPI ShellExecuteExW(LPSHELLEXECUTEINFOW pei)
{
    return FALSE;       // BUGBUG - BobDay - We should move this into SHUNIMP.C
}
#endif

// To display an error message appropriately, call this if ShellExecuteEx fails.
void _DisplayShellExecError(ULONG fMask, HWND hwnd, LPCTSTR pszFile, LPCTSTR pszTitle, DWORD dwErr)
{

    if (!(fMask & SEE_MASK_FLAG_NO_UI))
    {
        if (dwErr != ERROR_CANCELLED)
        {
            LPCTSTR pszHeader;
            UINT ids;

            // don't display "user cancelled", the user knows that already

            // make sure parent window is the foreground window
            if (hwnd)
                SetForegroundWindow(hwnd);

            if (pszTitle)
                pszHeader = pszTitle;
            else
                pszHeader = pszFile;

            // Use our messages when we can -- they're more descriptive
            switch (dwErr)
            {
            case 0:
            case ERROR_NOT_ENOUGH_MEMORY:
            case ERROR_OUTOFMEMORY:
                ids = IDS_LowMemError;
                break;

            case ERROR_FILE_NOT_FOUND:
                ids = IDS_RunFileNotFound;
                break;

            case ERROR_PATH_NOT_FOUND:
            case ERROR_BAD_PATHNAME:
                ids = IDS_PathNotFound;
                break;

            case ERROR_TOO_MANY_OPEN_FILES:
                ids = IDS_TooManyOpenFiles;
                break;

            case ERROR_ACCESS_DENIED:
                ids = IDS_RunAccessDenied;
                break;

            case ERROR_BAD_FORMAT:
                // NB CreateProcess, when execing a Win16 apps maps just about all of
                // these errors to BadFormat. Not very useful but there it is.
                ids = IDS_BadFormat;
                break;

            case ERROR_SHARING_VIOLATION:
                ids = IDS_ShareError;
                break;

            case ERROR_OLD_WIN_VERSION:
                ids = IDS_OldWindowsVer;
                break;

            case ERROR_APP_WRONG_OS:
                ids = IDS_OS2AppError;
                break;

            case ERROR_SINGLE_INSTANCE_APP:
                ids = IDS_MultipleDS;
                break;

            case ERROR_RMODE_APP:
                ids = IDS_RModeApp;
                break;

            case ERROR_INVALID_DLL:
                ids = IDS_InvalidDLL;
                break;

            case ERROR_NO_ASSOCIATION:
                ids = IDS_NoAssocError;
                break;

            case ERROR_DDE_FAIL:
                ids = IDS_DDEFailError;
                break;

            // THESE ARE FAKE ERROR_ VALUES DEFINED AT TOP OF THIS FILE.
            // THEY ARE FOR ERROR MESSAGE PURPOSES ONLY AND ARE MAPPED
            // TO VALID WINERROR.H ERROR MESSAGES BELOW.

            case ERROR_RESTRICTED_APP:
                ids = IDS_RESTRICTIONS;
                // restrictions like to use IDS_RESTRICTIONSTITLE
                if (!pszTitle)
                    pszHeader = MAKEINTRESOURCE(IDS_RESTRICTIONSTITLE);
                break;

            // If we don't get a match, let the system handle it for us
            default:
                ids = 0;
                SHSysErrorMessageBox(
                    hwnd,
                    pszHeader,
                    IDS_SHLEXEC_ERROR,
                    dwErr,
                    pszFile,
                    MB_OK | MB_ICONSTOP);
                break;
            }

            if (ids)
            {
                ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(ids),
                        pszHeader, (ids == IDS_LowMemError)?
                        (MB_OK | MB_ICONSTOP | MB_SYSTEMMODAL):(MB_OK | MB_ICONSTOP),
                        pszFile);
            }
        }
    }

    if (!(fMask & SEE_MASK_FLAG_SHELLEXEC))
    {
        UINT err = 0;

        switch (dwErr)
        {
        case ERROR_RESTRICTED_APP:
            dwErr = ERROR_ACCESS_DENIED;
            break;
        }

    }

    SetLastError(dwErr); // The message box may have clobbered.

}

void _ShellExecuteError(LPSHELLEXECUTEINFO pei, LPCTSTR lpTitle, DWORD dwErr)
{
    ASSERT(!ISSHELLEXECSUCCEEDED(pei->hInstApp));

    // if dwErr not passed in, get it
    if (dwErr == 0)
        dwErr = GetLastError();

    _DisplayShellExecError(pei->fMask, pei->hwnd, pei->lpFile, lpTitle, dwErr);
}




//----------------------------------------------------------------------------
// Given a file name and directory, get the path to the execuatable that
// would be exec'd if you tried to ShellExecute this thing.
HINSTANCE WINAPI FindExecutable(LPCTSTR lpFile, LPCTSTR lpDirectory, LPTSTR lpResult)
{
    HINSTANCE hInstance = (HINSTANCE)42;    // assume success must be > 32
    TCHAR szOldDir[MAX_PATH];
    TCHAR szFile[MAX_PATH];
    LPTSTR dirs[2];

    // Progman relies on lpResult being a ptr to an null string on error.
    *lpResult = TEXT('\0');
    GetCurrentDirectory(ARRAYSIZE(szOldDir), szOldDir);
    if (lpDirectory && *lpDirectory)
        SetCurrentDirectory(lpDirectory);
    else
        lpDirectory = szOldDir;     // needed for PathResolve()

    if (!GetShortPathName(lpFile, szFile, ARRAYSIZE(szFile))) {
        // if the lpFile is unqualified or bogus, let's use it down
        // in PathResolve.
        lstrcpyn(szFile, lpFile, ARRAYSIZE(szFile));
    }

    // get fully qualified path and add .exe extension if needed
    dirs[0] = (LPTSTR)lpDirectory;
    dirs[1] = NULL;
    if (!PathResolve(szFile, dirs, PRF_VERIFYEXISTS | PRF_TRYPROGRAMEXTENSIONS | PRF_FIRSTDIRDEF))
    {
        // File doesn't exist, return file not found.
        hInstance = (HINSTANCE)SE_ERR_FNF;
        goto Exit;
    }

    TraceMsg(TF_SHELLEXEC, "FindExecutable: PathResolve -> %s", (LPCSTR)szFile);

    if (PathIsExe(szFile))
    {
        lstrcpy(lpResult, szFile);
        goto Exit;
    }

    if (SUCCEEDED(AssocQueryString(0, ASSOCSTR_EXECUTABLE, szFile, NULL, szFile, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(szFile)))))
    {
        lstrcpy(lpResult, szFile);
    }
    else
    {
        hInstance = (HINSTANCE)SE_ERR_NOASSOC;
    }

Exit:
    TraceMsg(TF_SHELLEXEC, "FindExec(%s) ==> %s", (LPTSTR)lpFile, (LPTSTR)lpResult);
    SetCurrentDirectory(szOldDir);
    return hInstance;
}

#ifdef UNICODE
HINSTANCE WINAPI FindExecutableA(LPCSTR lpFile, LPCSTR lpDirectory, LPSTR lpResult)
{
    HINSTANCE   hResult;
    WCHAR       wszResult[MAX_PATH];
    ThunkText * pThunkText = ConvertStrings(2, lpFile, lpDirectory);

    *lpResult = '\0';
    if (NULL == pThunkText)
    {
        return (HINSTANCE)SE_ERR_OOM;   // BUGBUG (DavePl) More appropriate error code
    }

    hResult = FindExecutableW(pThunkText->m_pStr[0], pThunkText->m_pStr[1], wszResult);
    LocalFree(pThunkText);

    // FindExecutableW terminates wszResult for us, so this is safe
    // even if the above call fails

    // Thunk the output result string back to ANSI.  If the conversion fails,
    // or if the default char is used, we fail the API call.

    if (0 == WideCharToMultiByte(CP_ACP, 0, wszResult, -1, lpResult, MAX_PATH, NULL, NULL))
    {
        SetLastError((DWORD)E_FAIL);    // BUGBUG Need better error value
        return (HINSTANCE) SE_ERR_FNF;  // BUGBUG (DavePl) More appropriate error code
    }

    return hResult;

}
#else
HINSTANCE WINAPI FindExecutableW(LPCWSTR lpFile, LPCWSTR lpDirectory, LPWSTR lpResult)
{
    return 0;   // BUGBUG - BobDay - We should move this into SHUNIMP.C
}
#endif


//----------------------------------------------------------------------------
// Data structures for our wait for file open functions
//
typedef struct _WaitForItem * PWAITFORITEM;

typedef struct _WaitForItem
{
    DWORD           dwSize;
    DWORD           fOperation;    // Operation to perform
    PWAITFORITEM    pwfiNext;
    HANDLE          hEvent;         // Handle to event that was registered.
    UINT            iWaiting;       // Number of clients that are waiting.
    ITEMIDLIST      idlItem;        // pidl to wait for
} WAITFORITEM;

//
//  This is the form of the structure that is shoved into the shared memory
//  block.  It must be the 32-bit version for interoperability reasons.
//
typedef struct _WaitForItem32
{
    DWORD           dwSize;
    DWORD           fOperation;    // Operation to perform
    DWORD           NotUsed1;
    LONG            hEvent;        // Truncated event handle
    UINT            NotUsed2;
    ITEMIDLIST      idlItem;       // pidl to wait for
} WAITFORITEM32, *PWAITFORITEM32;

//
//  These macros enforce type safety so people are forced to use the
//  WAITFORITEM32 structure when accessing the shared memory block.
//
#define SHLockWaitForItem(h, pid) ((PWAITFORITEM32)SHLockShared(h, pid))

__inline void SHUnlockWaitForItem(PWAITFORITEM32 pwfi)
{
    SHUnlockShared(pwfi);
}

#pragma data_seg(DATASEG_SHARED)
PWAITFORITEM g_pwfiHead = NULL;
#pragma data_seg()

HANDLE SHWaitOp_OperateInternal( DWORD fOperation, LPCITEMIDLIST pidlItem)
{
    PWAITFORITEM    pwfi;
    HANDLE  hEvent = (HANDLE)NULL;

    for (pwfi = g_pwfiHead; pwfi != NULL; pwfi = pwfi->pwfiNext)
    {
        if (ILIsEqual(&(pwfi->idlItem), pidlItem))
        {
            hEvent = pwfi->hEvent;
            break;
        }
    }

    if (fOperation & WFFO_ADD)
    {
        if (!pwfi)
        {
            UINT uSize;
            UINT uSizeIDList = 0;

            if (pidlItem)
                uSizeIDList = ILGetSize(pidlItem);

            uSize = SIZEOF(WAITFORITEM) + uSizeIDList;

            // Create an event to wait for
            hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

            if (hEvent)
                pwfi = (PWAITFORITEM)SHAlloc(uSize);

            if (pwfi)
            {
                pwfi->dwSize = uSize;
                // pwfi->fOperation = 0;       // Meaningless
                pwfi->hEvent = hEvent;
                pwfi->iWaiting = ((fOperation & WFFO_WAIT) != 0);

                memcpy( &(pwfi->idlItem), pidlItem, uSizeIDList);

                // now link it in
                pwfi->pwfiNext = g_pwfiHead;
                g_pwfiHead = pwfi;
            }
        }
    }

    if (pwfi)
    {
        if (fOperation & WFFO_WAIT)
            pwfi->iWaiting++;

        if (fOperation & WFFO_SIGNAL)
            SetEvent(hEvent);

        if (fOperation & WFFO_REMOVE)
            pwfi->iWaiting--;       // decrement in use count;

        // Only check removal case if not adding
        if ((fOperation & WFFO_ADD) == 0)
        {
            // Remove it if nobody waiting on it
            if (pwfi->iWaiting == 0)
            {
                if (g_pwfiHead == pwfi)
                    g_pwfiHead = pwfi->pwfiNext;
                else
                {
                    PWAITFORITEM pwfiT = g_pwfiHead;
                    while ((pwfiT != NULL) && (pwfiT->pwfiNext != pwfi))
                        pwfiT = pwfiT->pwfiNext;
                    ASSERT(pwfiT != NULL);
                    if (pwfiT != NULL)
                        pwfiT->pwfiNext = pwfi->pwfiNext;
                }

                // Close the handle
                CloseHandle(pwfi->hEvent);

                // Free the memory
                SHFree(pwfi);

                hEvent = NULL;          // NULL indicates nobody waiting... (for remove case)
            }
        }
    }

    return hEvent;
}

void SHWaitOp_Operate( HANDLE hWait, DWORD dwProcId)
{
    PWAITFORITEM32 pwfiFind;

    pwfiFind = SHLockWaitForItem(hWait, dwProcId);
    if (!pwfiFind)
        return;

    pwfiFind->hEvent = HandleToLong(SHWaitOp_OperateInternal(pwfiFind->fOperation, &(pwfiFind->idlItem)));

    SHUnlockWaitForItem(pwfiFind);
}

HANDLE SHWaitOp_Create( DWORD fOperation, LPCITEMIDLIST pidlItem, DWORD dwProcId)
{
    UINT    uSizeIDList = 0;
    UINT    uSize;
    HANDLE  hWaitOp;
    PWAITFORITEM32 pwfi;

    if (pidlItem)
        uSizeIDList = ILGetSize(pidlItem);

    uSize = SIZEOF(WAITFORITEM32) + uSizeIDList;

    hWaitOp = SHAllocShared(NULL, uSize, dwProcId);
    if (!hWaitOp)
        goto Punt;

    pwfi = SHLockWaitForItem(hWaitOp,dwProcId);
    if (!pwfi)
        goto Punt;

    pwfi->dwSize = uSize;
    pwfi->fOperation = fOperation;
    pwfi->NotUsed1 = 0;
    pwfi->hEvent = HandleToLong((HANDLE)NULL);
    pwfi->NotUsed2 = 0;

    if (pidlItem)
        memcpy(&(pwfi->idlItem), pidlItem, uSizeIDList);

    SHUnlockWaitForItem(pwfi);

    return hWaitOp;

Punt:
    if (hWaitOp)
        SHFreeShared(hWaitOp, dwProcId);
    return (HANDLE)NULL;
}

//----------------------------------------------------------------------------
// SHWaitForFileToOpen - This function allows the cabinet to wait for a
// file (in particular folders) to signal us that they are in an open state.
// This should take care of several synchronazation problems with the shell
// not knowing when a folder is in the process of being opened or not
//
DWORD WINAPI SHWaitForFileToOpen(LPCITEMIDLIST pidl, UINT uOptions, DWORD dwTimeout)
{
    HWND    hwndShell;
    HANDLE  hWaitOp;
    HANDLE  hEvent;
    DWORD   dwProcIdSrc;
    DWORD   dwReturn = WAIT_OBJECT_0; // we need a default

    hwndShell = GetShellWindow();

    if ( (uOptions & (WFFO_WAIT | WFFO_ADD)) != 0)
    {
        if (hwndShell)
        {
            PWAITFORITEM32 pwfi;
            DWORD dwProcIdDst;

            dwProcIdSrc = GetCurrentProcessId();
            GetWindowThreadProcessId(hwndShell, &dwProcIdDst);

            // Do just the add and/or wait portions
            hWaitOp = SHWaitOp_Create( uOptions & (WFFO_WAIT | WFFO_ADD), pidl, dwProcIdSrc);

            SendMessage(hwndShell, CWM_WAITOP, (WPARAM)hWaitOp, (LPARAM)dwProcIdSrc);

            //
            // Now get the hEvent and convert to a local handle
            //
            pwfi = SHLockWaitForItem(hWaitOp, dwProcIdSrc);
            if (pwfi)
            {
                hEvent = SHMapHandle(LongToHandle(pwfi->hEvent),dwProcIdDst, dwProcIdSrc, EVENT_ALL_ACCESS, 0);
            }
            SHUnlockWaitForItem(pwfi);
            SHFreeShared(hWaitOp,dwProcIdSrc);
        }
        else
        {
            // Do just the add and/or wait portions
            hEvent = SHWaitOp_OperateInternal(uOptions & (WFFO_WAIT | WFFO_ADD), pidl);
        }

        if ((uOptions & WFFO_WAIT) && hEvent != (HANDLE)NULL)
        {
            dwReturn = SHProcessMessagesUntilEvent(NULL, hEvent, dwTimeout);
        }

        // BUGBUGBC hEvent can be NULL at this point, closing is bad

        if (hwndShell)          // Close the duplicated handle.
            CloseHandle(hEvent);
    }

    if (uOptions & WFFO_REMOVE)
    {
        if (hwndShell)
        {
            dwProcIdSrc = GetCurrentProcessId();

            hWaitOp = SHWaitOp_Create( WFFO_REMOVE, pidl, dwProcIdSrc);

            SendMessage(hwndShell, CWM_WAITOP, (WPARAM)hWaitOp, (LPARAM)dwProcIdSrc);
            SHFreeShared(hWaitOp,dwProcIdSrc);
        }
        else
        {
            SHWaitOp_OperateInternal(WFFO_REMOVE, pidl);
        }
    }
    return dwReturn;
}


//----------------------------------------------------------------------------
// SignalFileOpen - Signals that the file is open
//
BOOL WINAPI SignalFileOpen(LPCITEMIDLIST pidl)
{
    HWND    hwndShell;
    BOOL    fResult;
    PWAITFORITEM32 pwfi;

    hwndShell = GetShellWindow();

    if (hwndShell)
    {
        HANDLE  hWaitOp;
        DWORD dwProcId;

        dwProcId = GetCurrentProcessId();

        hWaitOp = SHWaitOp_Create( WFFO_SIGNAL, pidl, dwProcId);

        SendMessage(hwndShell, CWM_WAITOP, (WPARAM)hWaitOp, (LPARAM)dwProcId);

        //
        // Now get the hEvent to determine return value...
        //
        pwfi = SHLockWaitForItem(hWaitOp, dwProcId);
        if (pwfi)
        {
            fResult = (LongToHandle(pwfi->hEvent) != (HANDLE)NULL);
        }
        SHUnlockWaitForItem(pwfi);
        SHFreeShared(hWaitOp,dwProcId);
    }
    else
    {
        fResult = (SHWaitOp_OperateInternal(WFFO_SIGNAL, pidl) == (HANDLE)NULL);
    }
    return fResult;
}

//
// Checks to see if darwin is enabled.
//
BOOL IsDarwinEnabled()
{
    static BOOL fInit = FALSE;
    if (!fInit)
    {
        BOOL bDarwinDisabled = FALSE;
        HKEY hkeyPolicy = 0;
        if (RegOpenKey(HKEY_CURRENT_USER, REGSTR_PATH_POLICIES_EXPLORER, &hkeyPolicy) == ERROR_SUCCESS) 
        {
            bDarwinDisabled = (SHQueryValueEx(hkeyPolicy, TEXT("DisableMSI"), NULL, NULL, NULL, NULL) == ERROR_SUCCESS);
            RegCloseKey(hkeyPolicy);
        }

        if (!bDarwinDisabled)
            g_hInstDarwin = LoadLibrary(TEXT("MSI.DLL"));

        fInit = TRUE;
    }
    return BOOLFROMPTR(g_hInstDarwin);
}

//
// PFNMSIPROVIDECOMPONENTFROMDESCRIPTOR is specifically TCHAR since we delayload the xxxA on win95, 
// and the xxxW on NT4
//
typedef UINT (__stdcall * PFNMSIPROVIDECOMPONENTFROMDESCRIPTOR)(LPCTSTR, LPTSTR, DWORD*, DWORD*);

//
// PFNCOMMANDLINEFROMMSIDESCRIPTOR is specifically WCHAR since this api is NT5 only
//
typedef DWORD (__stdcall * PFNCOMMANDLINEFROMMSIDESCRIPTOR)(LPWSTR, LPWSTR, DWORD*);


//----------------------------------------------------------------------------
// takes the darwin ID string from the registry, and calls darwin to get the
// full path to the application.
//
// IN:  pszDarwinDescriptor - this has the contents of the darwin key read out of the registry.
//                            it should contain a string like "[Darwin-ID-for-App] /switches".
//
// OUT: pszDarwinComand - the full path to the application to this buffer w/ switches.
//
STDAPI ParseDarwinID(LPTSTR pszDarwinDescriptor, LPTSTR pszDarwinCommand, DWORD cchDarwinCommand)
{
#ifdef WINNT
    // On NT5 we call the CommandLineFromMsiDescriptor in ADVAPI instead of MSI directly. Don't ask
    // me why; I just work here.
    if (IsOS(OS_NT5))
    {
        static PFNCOMMANDLINEFROMMSIDESCRIPTOR s_pfnCommandLineFromDescriptor = (LPVOID)-1;

        if ((LPVOID)-1 == s_pfnCommandLineFromDescriptor)
        {
            HINSTANCE hinst = GetModuleHandle(TEXT("ADVAPI32.DLL"));

            if (hinst)
                s_pfnCommandLineFromDescriptor = (PFNCOMMANDLINEFROMMSIDESCRIPTOR)GetProcAddress(hinst, "CommandLineFromMsiDescriptor");
            else
                s_pfnCommandLineFromDescriptor = NULL;
        }

        if (s_pfnCommandLineFromDescriptor)
        {
            DWORD dwError = s_pfnCommandLineFromDescriptor(pszDarwinDescriptor, pszDarwinCommand, &cchDarwinCommand);

            return HRESULT_FROM_WIN32(dwError);
        }

        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

        return HRESULT_FROM_WIN32(ERROR_CALL_NOT_IMPLEMENTED);;
    }
    else
#endif WINNT
    {
        static PFNMSIPROVIDECOMPONENTFROMDESCRIPTOR s_pfnMsiProvideComponentFromDescriptor = (LPVOID)-1;
        DWORD dwArgsOffset = 0;

        if ((LPVOID)-1 == s_pfnMsiProvideComponentFromDescriptor)
        {
            HINSTANCE hinst = LoadLibrary(TEXT("MSI.DLL"));

            if (hinst)
            {
#ifdef UNICODE
                // on NT4 call the UNICODE function
                s_pfnMsiProvideComponentFromDescriptor = (PFNMSIPROVIDECOMPONENTFROMDESCRIPTOR)GetProcAddress(hinst, "MsiProvideComponentFromDescriptorW");
#else
                // on win95 call the ANSI function
                s_pfnMsiProvideComponentFromDescriptor = (PFNMSIPROVIDECOMPONENTFROMDESCRIPTOR)GetProcAddress(hinst, "MsiProvideComponentFromDescriptorA");
#endif
            }
            else
            {
                s_pfnMsiProvideComponentFromDescriptor = NULL;
            }
        }

        if (s_pfnMsiProvideComponentFromDescriptor)
        {
            UINT uError = s_pfnMsiProvideComponentFromDescriptor(pszDarwinDescriptor, pszDarwinCommand, &cchDarwinCommand, &dwArgsOffset);

            // The darwin guys used to pass us back the dwArgsOffset and we would have to strcat it onto the pszDarwinCommand.
            // Nowdays that is done for us so it should always be null.
            ASSERT(dwArgsOffset == 0);

            return HRESULT_FROM_WIN32(uError);
        }
        else
        {
            SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

            return HRESULT_FROM_WIN32(ERROR_CALL_NOT_IMPLEMENTED);
        }
    }
}
