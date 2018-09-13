//--------------------------------------------------------------------------
// Init the Cabinet (ie the top level browser).
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Includes...

#include "cabinet.h"
#include "rcids.h"
#include "cabwnd.h"

#include <regstr.h>
#include "startmnu.h"
#include <shdguid.h>    // for IID_IShellService
#include <shlguid.h>
#include "..\shdocvw\winlist.h"     // BUGBUG: get rid of this
#include <desktray.h>
#include <wininet.h>
#include <dbgmem.h>

#define DM_SHUTDOWN     DM_TRACE    // shutdown

// copied from desktop.cpp
#define PEEK_NORMAL     0
#define PEEK_QUIT       1

// on NT5 we need this event to be global so that it is shared between hydra
// sessions
#define SZ_SCMCREATEDEVENT_NT5      TEXT("Global\\ScmCreatedEvent")
#define SZ_SCMCREATEDEVENT          TEXT("ScmCreatedEvent")

// exports from shdocvw.dll
STDAPI_(void) RunInstallUninstallStubs(void);

// from win32\kernel\utctime.c (private)
DWORD APIENTRY RefreshDaylightInformation(BOOL fChangeTime);

// shell32.dll exports, shelldll\binder.c
STDAPI_(void) SHFreeUnusedLibraries();

int ExplorerWinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPTSTR pszCmdLine, int nCmdShow);

//Do not change this stock5.lib use this as a BOOL not a bit.
BOOL g_bMirroredOS = FALSE;

HINSTANCE hinstCabinet = 0;

CRITICAL_SECTION g_csDll = { 0 };

HKEY g_hkeyExplorer = NULL;


BOOL g_fLogonCycle = FALSE;
BOOL g_fCleanShutdown = TRUE;
BOOL g_fExitExplorer = TRUE;            // set to FALSE on WM_ENDSESSION shutdown case
BOOL g_fEndSession = FALSE;             // set to TRUE if we rx a WM_ENDSESSION during RunOnce etc
BOOL g_fFakeShutdown = FALSE;           // set to TRUE if we do Ctrl+Alt+Shift+Cancel shutdown
BOOL g_fRuningOnTerminalServer = FALSE;	// Assume we are not running on Hydra 

BOOL Cabinet_IsExplorerWindow(HWND hwnd)
{
    TCHAR szClass[32];

    GetClassName(hwnd, szClass, ARRAYSIZE(szClass));
    return lstrcmpi(szClass, TEXT("ExploreWClass")) == 0;
}

// BUGBUG: does not account for "Browser" windows
BOOL Cabinet_IsFolderWindow(HWND hwnd)
{
    TCHAR szClass[32];

    GetClassName(hwnd, szClass, ARRAYSIZE(szClass));
    return lstrcmpi(szClass, TEXT("CabinetWClass")) == 0;
}



//---------------------------------------------------------------------------

typedef enum {
    RRA_DEFAULT             = 0x0000,
    RRA_DELETE              = 0x0001,       // delete each reg value when we're done with it
    RRA_WAIT                = 0x0002,       // Wait for current item to finish before launching next item
    RRA_SHELLSERVICEOBJECTS = 0x0004,       // treat as a shell service object instead of a command sting
    RRA_NOUI                = 0x0008,       // prevents ShellExecuteEx from displaying error dialogs
    RRA_RUNSUBKEYS          = 0x0010,       // Run items in sub keys in alphabetical order
} RRA_FLAGS;


// The following handles running an application and optionally waiting for it
// to terminate.
void ShellExecuteRegApp(LPTSTR szCmdLine, RRA_FLAGS fFlags)
{
    TCHAR szQuotedCmdLine[MAX_PATH+2];
    SHELLEXECUTEINFO ei;
    LPTSTR pszArgs;

    //
    // We used to call CreateProcess( NULL, szCmdLine, ...) here,
    // but thats not useful for people with apppaths stuff.
    //
    // Don't let empty strings through, they will endup doing something dumb
    // like opening a command prompt or the like
    if (!szCmdLine || !*szCmdLine)
        return;


    // Gross, but if the process command fails, copy the command line to let
    // shell execute report the errors
    if (PathProcessCommand(szCmdLine, szQuotedCmdLine, ARRAYSIZE(szQuotedCmdLine),
                           PPCF_ADDARGUMENTS|PPCF_FORCEQUALIFY) == -1)
        lstrcpy(szQuotedCmdLine, szCmdLine);

    pszArgs = PathGetArgs(szQuotedCmdLine);
    if (*pszArgs)
        *(pszArgs - 1) = 0; // Strip args

    PathUnquoteSpaces(szQuotedCmdLine);

    ei.cbSize          = sizeof(SHELLEXECUTEINFO);
    ei.hwnd            = NULL;
    ei.lpVerb          = NULL;
    ei.lpFile          = szQuotedCmdLine;
    ei.lpParameters    = pszArgs;
    ei.lpDirectory     = NULL;
    ei.nShow           = SW_SHOWNORMAL;
    ei.fMask           = (fFlags & RRA_NOUI)?
            (SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI) :  SEE_MASK_NOCLOSEPROCESS;

    // Hydra comment:
    //      If we were sure that this func would not be called for anything but handling entries in RunOnce, then
    //      I would have out the system in in stall-mode and back to execute-mode right here.
    //      However, although I do not see this func being used by anything other than entries in RunOnce, 
    //      there is no guaranty.
	
    if (ShellExecuteEx(&ei))
    {
        if ( NULL != ei.hProcess)
        {
            if (fFlags & RRA_WAIT)
            {
                MsgWaitForMultipleObjectsLoop(ei.hProcess, INFINITE);
            }
            CloseHandle(ei.hProcess);
        }
    }
}

// The following code manages shell service objects.  We load inproc dlls
// from the registry key and QI them for IOleCommandTarget. Note that all
// Shell Service Objects are loaded on the desktop thread.
// CGID_ShellServiceObject notifications are sent to these objects letting
// them know about shell status.
HDSA g_hdsaShellServiceObjects=NULL;

void LoadShellServiceObject(LPCTSTR szValueName, LPCTSTR szCmdLine, RRA_FLAGS fFlags)
{
    SHELLSERVICEOBJECT sso = {0};

    DebugMsg(DM_TRACE, TEXT("%s %s"), szValueName, szCmdLine);

    if (!g_hdsaShellServiceObjects &&
        !(g_hdsaShellServiceObjects = DSA_Create(sizeof(SHELLSERVICEOBJECT), 2)))
    {
        // Fail
        return;
    }

    if (SHCLSIDFromString(szCmdLine, &sso.clsid) == S_OK)
    {
        if (SUCCEEDED(CoCreateInstance(&sso.clsid, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                                &IID_IOleCommandTarget, (void **)&sso.pct)))
        {

            lstrcpyn(sso.szName, szValueName, ARRAYSIZE(sso.szName));

            if (DSA_AppendItem(g_hdsaShellServiceObjects, &sso) == -1)
            {
                DebugMsg( DM_ERROR, TEXT("Cannot add to dsa <%s>"), szValueName );
                sso.pct->lpVtbl->Release(sso.pct);
            }
        }
    }
}

//
//  While we are doing our RunOnce processing, we want to change the
//  system cursor.  We can't use SetClassLong on the desktop class
//  because that's illegal under Win32.  And we can't use SetSystemCursor
//  because on NT, CopyImage on a system cursor doesn't actually copy
//  the cursor.  It merely returns the original cursor handle back,
//  which leaves us in a bit of a fix because the original cursor is
//  about to be obliterated by SetSystemCursor!
//
//  So instead, we create a full-virtual-screen window that always ducks
//  to the bottom of the Z-order.  We can't use SetShellWindow and make
//  USER do the ducking for us, because RunOnce applets might get confused
//  if they see a GetShellWindow before the shell is initialized.
//
//  We can't use a shlwapi worker window because we need to make the
//  class background brush COLOR_DESKTOP to ensure that we don't get
//  ugly white flashes if the user quickly drags a window around our
//  fake desktop.
//
//  You'd think this would be easy, but there are a lot of subtleties.
//

LRESULT RegAppsWaitWndProc(HWND hwnd, UINT wm, WPARAM wp, LPARAM lp)
{
    switch (wm) {

    case WM_WINDOWPOSCHANGING:
        ((LPWINDOWPOS)lp)->hwndInsertAfter = HWND_BOTTOM; // force to bottom
        break;          // proceed with default action

    // Subtlety: Must erase background with PaintDesktop so user's
    // wallpaper shows through.
    //
    // Double subtlety: Don't paint directly through the HDC that comes
    // in via the wParam, because that HDC is >not clipped<.  Consequently,
    // you get horrible flickering since we repaint the entire desktop.
    //
    // Triple subtlety: You have to do this in the WM_ERASEBKGND handler,
    // not the WM_PAINT handler, because MsgWaitForMultipleObjectsLoop
    // waits only for QS_SENDMESSAGE and not QS_PAINT.

    case WM_ERASEBKGND:
        {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            PaintDesktop(ps.hdc);
            EndPaint(hwnd, &ps);
        }
        return 0;

    case WM_ENDSESSION:
        g_fEndSession = (BOOL)wp;
        break;
    }

    return DefWindowProc(hwnd, wm, wp, lp);
}

#define REGAPPSWAIT_CLASS   TEXT("RegAppsWait")

HWND CreateRegAppsWaitWindow(void)
{
    WNDCLASS wc;

    // Should never try to do this if the main desktop is already up
    ASSERT(!GetShellWindow());

    wc.style            = 0;
    wc.lpfnWndProc      = RegAppsWaitWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = hinstCabinet;
    wc.hIcon            = 0;

    //
    //  Doesn't matter what cursor we choose because NT USER will say
    //  "Stupid app not waiting for QS_POSTMESSAGE so I will always display
    //  an hourglass cursor."  But do the right thing in case NT USER
    //  decides actually to pay attention to our class cursor.
    //
    wc.hCursor          = LoadCursor(NULL, IDC_APPSTARTING);

    // Subtlety: Don't use (HBRUSH)(COLOR_DESKTOP+1).  If the user
    // drags a window across the desktop, USER will first paint the
    // window with the class background brush and only later will
    // actually send the WM_ERASEBKGND message.  If the user's
    // desktop wallpaper has a different color from COLOR_DESKTOP,
    // you will see ugly flashing as the exposed bits are first
    // painted in COLOR_DESKTOP, and then later with the wallpaper.

    wc.hbrBackground    = (HBRUSH)GetStockObject(NULL_BRUSH);

    wc.lpszMenuName     = 0;
    wc.lpszClassName    = REGAPPSWAIT_CLASS;

    RegisterClass(&wc);

    return CreateWindow(
        REGAPPSWAIT_CLASS,                      /* Class Name */
        NULL,                                   /* Title */
        WS_POPUP | WS_CLIPCHILDREN | WS_VISIBLE,/* Style */
        GetSystemMetrics(SM_XVIRTUALSCREEN),    /* Position */
        GetSystemMetrics(SM_YVIRTUALSCREEN),    /* Position */
        GetSystemMetrics(SM_CXVIRTUALSCREEN),   /* Size */
        GetSystemMetrics(SM_CYVIRTUALSCREEN),   /* Size */
        NULL,                                   /* Parent */
        NULL,                                   /* Menu */
        hinstCabinet,                           /* Instance */
        0);                                     /* Special parameters */
}

void DestroyRegAppsWaitWindow(HWND hwnd)
{
    DestroyWindow(hwnd);
    UnregisterClass(REGAPPSWAIT_CLASS, hinstCabinet);
}

BOOL RunRegAppsAndObjects(HKEY hkeyParent, LPCTSTR szSubkey, RRA_FLAGS fFlags)
{
    HKEY hkey;
    BOOL fShellInit = FALSE;


    if (RegOpenKey(hkeyParent, szSubkey, &hkey) == ERROR_SUCCESS)
    {
        DWORD cbData, cbValue, dwType, i;
        TCHAR szValueName[80], szCmdLine[MAX_PATH];



        // BUGBUG: should we do this in retail too or is it too scary :)
#ifdef DEBUG
        //
        // we only support named values so explicitly purge default values
        //
        cbData = SIZEOF(szCmdLine);
        if (RegQueryValue(hkey, NULL, szCmdLine, &cbData) == ERROR_SUCCESS)
        {
            AssertMsg((cbData <= 2), TEXT("BOGUS default entry in <%s> '%s'"), szSubkey, szCmdLine);
            RegDeleteValue(hkey, NULL);
        }
#endif

        if ( fFlags & RRA_RUNSUBKEYS )
        {
            // run the contents of each sub key.  Keys must be run in alphabetical order.  The sub keys
            // are used to prioritize the launching of items.  We only run one level of subkey so we need
            // to turn off the subkey flag.
            RRA_FLAGS fSubkeyFlags = fFlags & ~(RRA_RUNSUBKEYS);
            for (i=0; !g_fEndSession ; i++)
            {
                LONG lEnum;

                cbValue = ARRAYSIZE(szValueName);
                
                // BUGBUG: This is a dirty assumption.  This assumes several unsafe things:
                // 1.) none of the programs being run will changes this section of the registry.
                // 2.) NT enumerates registry items in alphabetic order.
                // Since it isn't safe to make assumption #1 this needs to be made more robust.
                lEnum = RegEnumKey( hkey, i, szValueName, cbValue );

                if( ERROR_MORE_DATA == lEnum )
                {
                    // ERROR_MORE_DATA means the value name or data was too large.
                    // Skip to the next item.
                    DebugMsg( DM_ERROR, TEXT("Explorer: RunRegAppsAndObjects cannot run oversize entry in <%s> '%s'"), szSubkey, szValueName );
                    continue;
                }
                else if ( ERROR_SUCCESS != lEnum )
                {
                    // could be ERROR_NO_MORE_ENTRIES, or some kind of failure
                    // we can't recover from any other registry problem, anyway
                    break;
                }

                RunRegAppsAndObjects( hkey, szValueName, fSubkeyFlags );

                if ( fFlags & RRA_DELETE )
                {
                    // delete the sub key.
                    SHDeleteKey( hkey, szValueName );
                    i--;
                }
            }
        }
        else
        {
            //
            // now enumerate all of the values.
            //
            for (i = 0; !g_fEndSession ; i++)
            {
                LONG lEnum;

                cbValue = ARRAYSIZE(szValueName);
                cbData = SIZEOF(szCmdLine);

                lEnum = RegEnumValue( hkey, i, szValueName, &cbValue, NULL, &dwType, (LPBYTE)szCmdLine, &cbData );

                if( ERROR_MORE_DATA == lEnum )
                {
                    // ERROR_MORE_DATA means the value name or data was too large
                    // skip to the next item
                    DebugMsg( DM_ERROR, TEXT("Cannot run oversize entry in <%s>"), szSubkey );
                    continue;
                }
                else if ( lEnum != ERROR_SUCCESS )
                {
                    // could be ERROR_NO_MORE_ENTRIES, or some kind of failure
                    // we can't recover from any other registry problem, anyway
                    break;
                }

                if (dwType == REG_SZ)
                {
                    DebugMsg(DM_TRACE, TEXT("%s %s"), szSubkey, szCmdLine);
                    // only run things marked with a "*" in clean boot
                    if (g_fCleanBoot && (szValueName[0] != TEXT('*')))
                        continue;

                    // NB Things marked with a '!' mean delete after
                    // the CreateProcess not before. This is to allow
                    // certain apps (runonce.exe) to be allowed to rerun
                    // to if the machine goes down in the middle of execing
                    // them. Be very afraid of this switch.

                    if ((fFlags & RRA_DELETE) && (szValueName[0] != TEXT('!')))
                    {
                        // This delete can fail if the user doesn't have the privilege
                        if (RegDeleteValue(hkey, szValueName) == ERROR_SUCCESS)
                        {
                            // adjust for shift in value index only if delete succeeded
                            i--;
                        }
                    }

                    if (fFlags & RRA_SHELLSERVICEOBJECTS)
                        LoadShellServiceObject(szValueName, szCmdLine, fFlags);
                    else
                    {
#ifdef WINNT
                        // Hydra-Specific stuff
                        BOOL hydraInAppInstallMode = FALSE;
                
                        // In here, We only put the Hydra server in app-install-mode if RunOnce entries are 
                        // being processed 
                        if (!lstrcmpi(szSubkey, REGSTR_PATH_RUNONCE)) 
                        {
                            // See if we are on NT5, and if the terminal-services is enabled
                            if (g_fRuningOnTerminalServer) 
                            {
                                if (hydraInAppInstallMode = SetTermsrvAppInstallMode(TRUE)) 
                                {
                                    fFlags |= RRA_WAIT;  // Changing timing blows up IE 4.0, but IE5 is ok!
                                } 
                            }
                        }
#endif // WINNT
                    
                        ShellExecuteRegApp(szCmdLine, fFlags);
    
#ifdef WINNT    
                        // Hydra-Specific stuff
                        if (hydraInAppInstallMode)
                        {
                            SetTermsrvAppInstallMode(FALSE);
                        }
#endif // WINNT
                    }

                    // Post delete '!' things.
                    if ((fFlags & RRA_DELETE) && (szValueName[0] == TEXT('!'))) {
                        // This delete can fail if the user doesn't have the privilege
                        if (RegDeleteValue(hkey, szValueName) == ERROR_SUCCESS)
                        {
                            // adjust for shift in value index only if delete succeeded
                            i--;    // adjust for shift in value index
                        }
                    }
                }
            }
        }

        RegCloseKey(hkey);

    }


    // if we rx'd a WM_ENDSESSION whilst running any of these keys we must exit the 
    // process.

    if ( g_fEndSession )
        ExitProcess(0);

    return fShellInit;
}


BOOL LoadShellServiceObjects(HKEY hkeyParent, LPCTSTR szSubkey)
{
    return RunRegAppsAndObjects(hkeyParent, szSubkey, RRA_SHELLSERVICEOBJECTS);
}


// clsid - NULL, send to everyone
//         else restrict cmd to given class.
void CTExecShellServiceObjects(const CLSID *pclsid, DWORD nCmdID, DWORD nCmdexecopt, DWORD flags)
{
    int iCount;
    int i;
    int iEnd, iStart;
    HDSA hdsaShellServiceObjects;

    if (!g_hdsaShellServiceObjects)
    {
        return;
    }

    // We use a temp variable to protect agains re-entancy (eg netshell calls PeekMessage during the 
    // Exec callback). Basically this is single threaded since the tray thread is the only guy who ever
    // calls this function, but we have to be careful that we dont re-enter ourself.
    hdsaShellServiceObjects = g_hdsaShellServiceObjects;
    g_hdsaShellServiceObjects = NULL;

    iCount = DSA_GetItemCount(hdsaShellServiceObjects);

    if (iCount)
    {
        // Loop through all shell service objects and send the command target the
        // command id.

        if (flags & CTEXECSSOF_REVERSE)
        {
            iStart = iCount-1;
            iEnd = 0-1;
        }
        else
        {
            iStart = 0;
            iEnd = iCount;
        }

        for (i=iStart; i != iEnd; (flags & CTEXECSSOF_REVERSE ? i-- : i++))
        {
            PSHELLSERVICEOBJECT psso = (PSHELLSERVICEOBJECT)DSA_GetItemPtr(hdsaShellServiceObjects, i);

            if (!pclsid || IsEqualGUID(&psso->clsid, pclsid))
            {
                psso->pct->lpVtbl->Exec(psso->pct,
                                        &CGID_ShellServiceObject,
                                        nCmdID, nCmdexecopt,
                                        NULL, NULL);

                if (nCmdID==SSOCMDID_CLOSE)
                    psso->pct->lpVtbl->Release(psso->pct);
            }
        }
    }

    g_hdsaShellServiceObjects = hdsaShellServiceObjects;
}


//---------------------------------------------------------------------------

void CreateShellDirectories()
{
    TCHAR szPath[MAX_PATH];

    //  Create the shell directories if they don't exist
    SHGetSpecialFolderPath(NULL, szPath, CSIDL_DESKTOPDIRECTORY, TRUE);
    SHGetSpecialFolderPath(NULL, szPath, CSIDL_PROGRAMS, TRUE);
    SHGetSpecialFolderPath(NULL, szPath, CSIDL_STARTMENU, TRUE);
    SHGetSpecialFolderPath(NULL, szPath, CSIDL_STARTUP, TRUE);
    SHGetSpecialFolderPath(NULL, szPath, CSIDL_RECENT, TRUE);
    SHGetSpecialFolderPath(NULL, szPath, CSIDL_FAVORITES, TRUE);
}

//----------------------------------------------------------------------------
// returns:
//      TRUE if the user wants to abort the startup sequence
//      FALSE keep going
//
// note: this is a switch, once on it will return TRUE to all
// calls so these keys don't need to be pressed the whole time
BOOL AbortStartup()
{
    static BOOL bAborted = FALSE;       // static so it sticks!

    // DebugMsg(DM_TRACE, "Abort Startup?");

    if (bAborted)
        return TRUE;    // don't do funky startup stuff
    else {
        bAborted = (g_fCleanBoot || ((GetAsyncKeyState(VK_CONTROL) < 0) || (GetAsyncKeyState(VK_SHIFT) < 0)));
        return bAborted;
    }
}

// BUGBUG: hwndOwner is no longer used (NULL is passed in) remove the hwndOwner code
// once we are certain that this isn't needed
BOOL EnumFolder_Startup(IShellFolder * psf, HWND hwndOwner, LPITEMIDLIST pidlFolder, LPITEMIDLIST pidlItem)
{
    LPCONTEXTMENU pcm;
    HRESULT hres;
//    MSG msg;

    hres = psf->lpVtbl->GetUIObjectOf(psf, hwndOwner, 1, &pidlItem, & IID_IContextMenu, NULL, &pcm);
    if (SUCCEEDED(hres))
    {
        HMENU hmenu = CreatePopupMenu();
        if (hmenu)
        {
#define CMD_ID_FIRST    1
#define CMD_ID_LAST     0x7fff
            INT idCmd;
            pcm->lpVtbl->QueryContextMenu(pcm, hmenu, 0, CMD_ID_FIRST, CMD_ID_LAST, CMF_DEFAULTONLY);
            idCmd = GetMenuDefaultItem(hmenu, MF_BYCOMMAND, 0);
            if (idCmd)
            {
                CMINVOKECOMMANDINFOEX ici;

                ZeroMemory(&ici, SIZEOF(ici));
                ici.cbSize = SIZEOF(ici);
                ici.hwnd = hwndOwner;
                ici.lpVerb = (LPSTR)MAKEINTRESOURCE(idCmd - 1);
                ici.nShow = SW_NORMAL;

                pcm->lpVtbl->InvokeCommand(pcm, (LPCMINVOKECOMMANDINFO)&ici);
            }
            DestroyMenu(hmenu);
        }
        pcm->lpVtbl->Release(pcm);
    }

    if (AbortStartup())
        return FALSE;

    return TRUE;
}

//----------------------------------------------------------------------------
// BUGBUG: hwndOwner is no longer used (NULL is passed in) remove the hwndOwner code
// once we are certain that this isn't needed
void EnumFolder(HWND hwndOwner, LPITEMIDLIST pidlFolder, DWORD grfFlags, PFNENUMFOLDERCALLBACK pfn)
{
    IShellFolder *psf = BindToFolder(pidlFolder);
    if (psf)
    {
        LPENUMIDLIST penum;
        HRESULT hres = psf->lpVtbl->EnumObjects(psf, hwndOwner, grfFlags, &penum);
        if (SUCCEEDED(hres))
        {
            LPITEMIDLIST pidl;
            UINT celt;
            while (penum->lpVtbl->Next(penum, 1, &pidl, &celt)==NOERROR && celt==1)
            {
                if (!pfn(psf, hwndOwner, pidlFolder, pidl))
                {
                    SHFree(pidl);
                    break;
                }
                SHFree(pidl);
            }
            penum->lpVtbl->Release(penum);
        }
        psf->lpVtbl->Release(psf);
    }
}

//----------------------------------------------------------------------------
// BUGBUG: hwndOwner is no longer used (NULL is passed in) remove the hwndOwner code
// once we are certain that this isn't needed
void _ExecuteStartupPrograms(HWND hwndOwner)
{
    LPITEMIDLIST pidlStartup;

    if (AbortStartup())
        return;

    pidlStartup = SHCloneSpecialIDList(NULL, CSIDL_COMMON_STARTUP, TRUE);
    if (pidlStartup)
    {
        EnumFolder(hwndOwner, pidlStartup, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, EnumFolder_Startup);
        ILFree(pidlStartup);
    }

    //
    // Execute non-localized "Common StartUp" group if exists.
    //
    pidlStartup = SHCloneSpecialIDList(NULL, CSIDL_COMMON_ALTSTARTUP, FALSE);
    if (pidlStartup)
    {
        EnumFolder(hwndOwner, pidlStartup, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, EnumFolder_Startup);
        ILFree(pidlStartup);
    }

    pidlStartup = SHCloneSpecialIDList(NULL, CSIDL_STARTUP, TRUE);
    if (pidlStartup)
    {
        EnumFolder(hwndOwner, pidlStartup, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, EnumFolder_Startup);
        ILFree(pidlStartup);
    }

    //
    // Execute non-localized "StartUp" group if exists.
    //
    pidlStartup = SHCloneSpecialIDList(NULL, CSIDL_ALTSTARTUP, FALSE);
    if (pidlStartup)
    {
        EnumFolder(hwndOwner, pidlStartup, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, EnumFolder_Startup);
        ILFree(pidlStartup);
    }
}


// BUGBUG:: A bunch of this code can get reduced down... ie boiled
const LPTSTR c_aszForceCheckWinIni[] = {TEXT("GROUPS.B$$"), NULL};

void CheckWinIniForAssocs(void);

void BoilThatDustSpec(LPTSTR pStart, int nCmdShow)
{
    BOOL bFinished;

    bFinished = FALSE;
    while (!bFinished && !AbortStartup())
    {
        SHELLEXECUTEINFO ei;
        LPTSTR pEnd;

        pEnd = pStart;
        while ((*pEnd) && (*pEnd != TEXT(' ')) && (*pEnd != TEXT(',')))
            pEnd = (LPTSTR)OFFSETOF(CharNext(pEnd));

        if (*pEnd == 0)
            bFinished = TRUE;
        else
            *pEnd = 0;

        if (lstrlen(pStart) != 0)
        {
            LPTSTR pszFile;
            TCHAR szFile[MAX_PATH];
            const LPTSTR *ppszForce;

            // Load and Run lines are done relative to windows directory.
            GetWindowsDirectory(szFile, ARRAYSIZE(szFile));
            SetCurrentDirectory(szFile);

            pszFile = PathFindFileName(pStart);
            lstrcpy(szFile, pszFile);
            PathRemoveFileSpec(pStart);

            // App hacks to get borlands Setup program to work
            for (ppszForce = c_aszForceCheckWinIni; *ppszForce; ppszForce++)
            {
                if (lstrcmpi(szFile, *ppszForce) == 0)
                {
                    DebugMsg(DM_TRACE, TEXT("c.boil: Apphack %s force winini scan"), szFile);

                    CheckWinIniForAssocs();
                    break;
                }
            }

            ei.cbSize          = sizeof(SHELLEXECUTEINFO);
            ei.hwnd            = NULL;
            ei.lpVerb          = NULL;
            ei.lpFile          = szFile;
            ei.lpParameters    = NULL;
            ei.lpDirectory     = pStart;
            ei.nShow           = nCmdShow;
            ei.fMask           = 0;

            if (!ShellExecuteEx(&ei))
            {
                ShellMessageBox(hinstCabinet, NULL, MAKEINTRESOURCE(IDS_WINININORUN),
                                MAKEINTRESOURCE(IDS_DESKTOP),
                                MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL,
                                (LPTSTR)szFile);
            }
        }
        pStart = pEnd+1;
    }
}

void _DoRunEquals()
{
    TCHAR szBuffer[255];        // max size of load= run= lines...

    if (g_fCleanBoot)
        return;

    /* "Load" apps before "Run"ning any. */
    GetProfileString(TEXT("windows"), TEXT("Load"), TEXT(""), szBuffer, ARRAYSIZE(szBuffer));
    if (*szBuffer)
        BoilThatDustSpec(szBuffer, SW_SHOWMINNOACTIVE);

    GetProfileString(TEXT("windows"), TEXT("Run"), TEXT(""), szBuffer, ARRAYSIZE(szBuffer));
    if (*szBuffer)
        BoilThatDustSpec(szBuffer, SW_SHOWNORMAL);

}

//---------------------------------------------------------------------------
// Use IERnonce.dll to process RunOnceEx key
//
typedef void (WINAPI *RUNONCEEXPROCESS)(HWND, HINSTANCE, LPSTR, int);

void ProcessRunOnceEx()
{
    HKEY hkey;


    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_RUNONCEEX,
                    0, KEY_QUERY_VALUE, &hkey))
    {
        


		DWORD dwNumSubKeys = 0;

        RegQueryInfoKey(hkey, NULL, NULL, NULL, &dwNumSubKeys, NULL,
                        NULL, NULL, NULL, NULL, NULL, NULL);
        RegCloseKey(hkey);

        if (dwNumSubKeys)
        {
            HANDLE hLib;
            TCHAR szPath[MAX_PATH];

            GetSystemDirectory(szPath, ARRAYSIZE(szPath));
            PathAppend(szPath, TEXT("iernonce.dll"));

            hLib = LoadLibrary(szPath);
            if (hLib)
            {
                RUNONCEEXPROCESS pfnRunOnceExProcess;
#ifdef WINNT    // Hydra-Specific
                BOOL    hydraInAppInstallMode = FALSE;
                
                // See if we are on NT5, and if the terminal-services is enabled
                if (g_fRuningOnTerminalServer) 
                {
                    hydraInAppInstallMode = SetTermsrvAppInstallMode(TRUE); 
                }
#endif // WINNT

                pfnRunOnceExProcess = (RUNONCEEXPROCESS)GetProcAddress(hLib, "RunOnceExProcess");
                if (pfnRunOnceExProcess)
                {
                    // the four param in the function is due to the function cab be called
                    // from RunDLL which will path in those params.  But RunOnceExProcess ignore all
                    // of them.  Therefore, I don't pass any meaningful thing here.
                    //
                    pfnRunOnceExProcess(NULL, NULL, NULL, 0);
                }
                FreeLibrary(hLib);
                
#ifdef WINNT    // Hydra-Specific
                if (hydraInAppInstallMode)
                {
                    SetTermsrvAppInstallMode(FALSE) ;
                } 
#endif // WINNT

            }
        }
    }
}

#define REGTIPS             REGSTR_PATH_EXPLORER TEXT("\\Tips")
#define SZ_REGKEY_RUNSRVWIZ TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Setup\\Welcome")

// BUGBUG: This can be removed as soon as the policy editor and setup are updated to
// place welcome under one of the prioritized HKCU\Run subkeys.
UINT _RunWelcome()
{
    HKEY hkey;
    BOOL fShow = FALSE;
    UINT uPeek = PEEK_NORMAL;


    if (RegOpenKey(HKEY_CURRENT_USER, REGTIPS, &hkey) == ERROR_SUCCESS)
    {
        DWORD cbData = SIZEOF(fShow);
        RegQueryValueEx(hkey, TEXT("Show"), NULL, NULL, (LPBYTE)&fShow, &cbData);
        RegCloseKey(hkey);
    }

    if (fShow)
    {
        DWORD dwType;
        DWORD dwData;
        DWORD cbSize = sizeof(dwData);

        TCHAR    szCmdLine[MAX_PATH * 2];
        PROCESS_INFORMATION pi;
        STARTUPINFO startup = {0};;
        startup.cb = SIZEOF(startup);
        startup.wShowWindow = SW_SHOWNORMAL;

        if ( IsOS(OS_WIN2000PRO) || IsOS(OS_WINDOWS) )
        {
            // Only run welcome.exe if we are on Professional (or win9x)
            GetWindowsDirectory(szCmdLine, ARRAYSIZE(szCmdLine));
            PathAppend(szCmdLine, TEXT("Welcome.exe"));
        }
        else if ( (IsOS(OS_WIN2000SERVER) || IsOS(OS_WIN2000ADVSERVER)) &&
                  IsUserAnAdmin() &&
                  ((ERROR_SUCCESS != SHGetValue(HKEY_CURRENT_USER, SZ_REGKEY_RUNSRVWIZ, TEXT("SrvWiz"), &dwType, (LPBYTE)&dwData, &cbSize)) || (dwData != 0)))
        {
            // launch Configure Your Server for system administrators on Win2000 Server and Advanced Server
            GetSystemDirectory(szCmdLine, ARRAYSIZE(szCmdLine));
            PathAppend(szCmdLine, TEXT("mshta.exe res://srvwiz.dll/default.hta"));
        }
        else 
        {
            // If neither or the above are true don't try to run anything.
            return 0;
        }


        if (CreateProcess(NULL, szCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &pi))
        {
            WaitForSingleObject(pi.hProcess, INFINITE);

            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }
    return uPeek;
}

#ifdef WINNT

// On NT, run the TASKMAN= line from the registry

void _AutoRunTaskMan(void)
{
    HKEY hkeyWinLogon;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"),
                     0, KEY_READ, &hkeyWinLogon) == ERROR_SUCCESS)
    {
        TCHAR szBuffer[MAX_PATH];
        DWORD cbBuffer = SIZEOF(szBuffer);
        if (RegQueryValueEx(hkeyWinLogon, TEXT("Taskman"), 0, NULL, (LPBYTE)szBuffer, &cbBuffer) == ERROR_SUCCESS)
        {
            if (szBuffer[0])
            {
                PROCESS_INFORMATION pi;
                STARTUPINFO startup = {0};
                startup.cb = SIZEOF(startup);
                startup.wShowWindow = SW_SHOWNORMAL;

                if (CreateProcess(NULL, szBuffer, NULL, NULL, FALSE, 0,
                                  NULL, NULL, &startup, &pi))
                {
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                }
            }
        }
        RegCloseKey(hkeyWinLogon);
    }
}
#else

#define _AutoRunTaskMan()       // nothing on Win95

#endif


#ifdef DEBUG
//---------------------------------------------------------------------------
// Copy the exception info so we can get debug info for Raised exceptions
// which don't go through the debugger.
void _CopyExceptionInfo(LPEXCEPTION_POINTERS pep)
{
    PEXCEPTION_RECORD per;

    per = pep->ExceptionRecord;
    DebugMsg(DM_ERROR, TEXT("Exception %x at %#08x."), per->ExceptionCode, per->ExceptionAddress);

    if (per->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
    {
        // If the first param is 1 then this was a write.
        // If the first param is 0 then this was a read.
        if (per->ExceptionInformation[0])
        {
            DebugMsg(DM_ERROR, TEXT("Invalid write to %#08x."), per->ExceptionInformation[1]);
        }
        else
        {
            DebugMsg(DM_ERROR, TEXT("Invalid read of %#08x."), per->ExceptionInformation[1]);
        }
    }
}
#else
#define _CopyExceptionInfo(x) TRUE
#endif



// try to create this by sending a wm_command directly to
// the desktop.
BOOL MyCreateFromDesktop(HINSTANCE hInst, LPCTSTR pszCmdLine, int nCmdShow)
{
    NEWFOLDERINFO fi = {0};
    BOOL bRet = FALSE;

    fi.nShow = nCmdShow;

    //  since we have browseui fill out the fi, 
    //  SHExplorerParseCmdLine() does a GetCommandLine()
    if (SHExplorerParseCmdLine(&fi))
        bRet = SHCreateFromDesktop(&fi);

    //  should we also have it cleanup after itself??

    //  SHExplorerParseCmdLine() can allocate this buffer...
    if (fi.uFlags & COF_PARSEPATH)
        LocalFree(fi.pszPath);
        
    ILFree(fi.pidl);
    ILFree(fi.pidlRoot);

    return bRet;
}

BOOL g_fDragFullWindows=FALSE;
int g_cxEdge=0;
int g_cyEdge=0;
int g_cySize=0;
int g_cxTabSpace=0;
int g_cyTabSpace=0;
int g_cxBorder=0;
int g_cyBorder=0;
int g_cxPrimaryDisplay=0;
int g_cyPrimaryDisplay=0;
int g_cxDlgFrame=0;
int g_cyDlgFrame=0;
int g_cxFrame=0;
int g_cyFrame=0;

int g_cxMinimized=0;
int g_fCleanBoot=0;
int g_cxVScroll=0;
int g_cyHScroll=0;

void Cabinet_InitGlobalMetrics(WPARAM wParam, LPTSTR lpszSection)
{
    BOOL fForce = (!lpszSection || !*lpszSection);

    if (fForce || wParam == SPI_SETDRAGFULLWINDOWS) {
        SystemParametersInfo(SPI_GETDRAGFULLWINDOWS, 0, &g_fDragFullWindows, 0);
    }

    if (fForce || !lstrcmpi(lpszSection, TEXT("WindowMetrics")) ||
        wParam == SPI_SETNONCLIENTMETRICS) {

        // REVIEW, before it's all over, make sure all these vars are used somewhere.
        g_cxEdge = GetSystemMetrics(SM_CXEDGE);
        g_cyEdge = GetSystemMetrics(SM_CYEDGE);

#ifdef NASHFLAT_TASKBAR
        g_cxTabSpace = (g_cxEdge * 5);
#else
        g_cxTabSpace = (g_cxEdge * 3) / 2;
#endif
        g_cyTabSpace = (g_cyEdge * 3) / 2; // cause the graphic designers really really want 3.
        g_cySize = GetSystemMetrics(SM_CYSIZE);
        g_cxBorder = GetSystemMetrics(SM_CXBORDER);
        g_cyBorder = GetSystemMetrics(SM_CYBORDER);
        g_cxVScroll = GetSystemMetrics(SM_CXVSCROLL);
        g_cyHScroll = GetSystemMetrics(SM_CYHSCROLL);
        g_cxDlgFrame = GetSystemMetrics(SM_CXDLGFRAME);
        g_cyDlgFrame = GetSystemMetrics(SM_CYDLGFRAME);
        g_cxFrame  = GetSystemMetrics(SM_CXFRAME);
        g_cyFrame  = GetSystemMetrics(SM_CYFRAME);
        g_cxMinimized = GetSystemMetrics(SM_CXMINIMIZED);

        g_cxPrimaryDisplay = GetSystemMetrics(SM_CXSCREEN);
        g_cyPrimaryDisplay = GetSystemMetrics(SM_CYSCREEN);
    }
}

//---------------------------------------------------------------------------
void _CreateAppGlobals()
{
    Cabinet_InitGlobalMetrics(0, NULL);
#ifdef WINNT
    g_bRunOnNT5 = BOOLIFY(IsOS(OS_NT5));
#else
    g_bRunOnMemphis = BOOLIFY(IsOS(OS_MEMPHIS));
#endif

    //
    // Check if the mirroring APIs exist on the current
    // platform.
    //
    g_bMirroredOS = IS_MIRRORING_ENABLED();


    // make sure we got our #defines right...
    ASSERT(BOOLIFY(g_bRunOnNT) == BOOLIFY(IsOS(OS_NT)));
    ASSERT(BOOLIFY(g_bRunOnNT5) == BOOLIFY(IsOS(OS_NT5)));
    ASSERT(BOOLIFY(g_bRunOnMemphis) == BOOLIFY(IsOS(OS_MEMPHIS)));
}

//
//  This function checks if any of the shell windows is already created by
// another instance of explorer and returns TRUE if so.
//

BOOL IsAnyShellWindowAlreadyPresent()
{
    return GetShellWindow() || FindWindow(TEXT("Proxy Desktop"), NULL);
}


// See if the Shell= line indicates that we are the shell

BOOL ExplorerIsShell()
{
    TCHAR *pszPathName, szPath[MAX_PATH];
    TCHAR *pszModuleName, szModulePath[MAX_PATH];

    ASSERT(!IsAnyShellWindowAlreadyPresent());

    GetModuleFileName(NULL, szModulePath, ARRAYSIZE(szModulePath));
    pszModuleName = PathFindFileName(szModulePath);

    GetPrivateProfileString(TEXT("boot"), TEXT("shell"), pszModuleName, szPath, ARRAYSIZE(szPath), TEXT("system.ini"));

    PathRemoveArgs(szPath);
    PathRemoveBlanks(szPath);
    pszPathName = PathFindFileName(szPath);

    // NB Special case shell=install.exe - assume we are the shell.
    // Symantec un-installers temporarily set shell=installer.exe so
    // we think we're not the shell when we are. They fail to clean up
    // a bunch of links if we don't do this.

    return StrCmpNI(pszPathName, pszModuleName, lstrlen(pszModuleName)) == 0 ||
           lstrcmpi(pszPathName, TEXT("install.exe")) == 0;
}


// Returns TRUE of this is the first time the explorer is run

BOOL ShouldStartDesktopAndTray()
{
    // We need to be careful on which window we look for.  If we look for
    // our desktop window class and Progman is running we will find the
    // progman window.  So Instead we should ask user for the shell window.

    // We can not depend on any values being set here as this is the
    // start of a new process.  This wont be called when we start new
    // threads.
    return !IsAnyShellWindowAlreadyPresent() && ExplorerIsShell();
}

void DisplayCleanBootMsg()
{
    TCHAR szMsg[1024];
    TCHAR szTitle[80];
    int ids;
    int cb;
    LPTSTR pszMsg = szMsg;

    szMsg[0] = TEXT('\0');

    for (ids=IDS_CLEANBOOTMSG1; ids <= IDS_CLEANBOOTMSG4 ; ids++)
    {
        cb = LoadString(hinstCabinet, ids, pszMsg,
                ARRAYSIZE(szMsg) - (int)(pszMsg - szMsg));
        if (cb == 0)
            break;
        pszMsg += cb;
    }
    // Make sure it is NULL terminated
    *pszMsg = TEXT('\0');

    LoadString(hinstCabinet, IDS_DESKTOP, szTitle, ARRAYSIZE(szTitle));

    // Now display the message.
    MessageBox(NULL, szMsg, szTitle,
                  MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
}

//---------------------------------------------------------------------------
const CHAR c_szTimeChangedRunDLL[] = "rundll32 shell32.dll,Control_RunDLL timedate.cpl,,/m";

void DoDaylightCheck(BOOL fStartupInit)
{
    DWORD changed;

    DebugMsg(DM_TRACE, TEXT("c.ddc(%d): calling k32.rdi"), fStartupInit);

#ifdef WINNT
    changed = FALSE;
#else
    // Win95 base does not automatically handle timezone cutover
    // we have poke it every so often...
    changed = RefreshDaylightInformation(TRUE);
#endif

    if (changed > 0)
    {
        DebugMsg(DM_TRACE, TEXT("c.ddc(%d): rdi changed - %lu"), fStartupInit, changed);

        // something actually changed, tell everbody
        if (!fStartupInit)
        {
            SendMessage((HWND)-1, WM_TIMECHANGE, 0, 0);

            // if the local time changed tell the user
            if (changed > 1)
                WinExec(c_szTimeChangedRunDLL, SW_SHOWNORMAL);
        }
        else
        {
            // there should only be "server" processes around anyway
            PostMessage((HWND)-1, WM_TIMECHANGE, 0, 0);

            // if the local time changed queue a runonce to tell the user
            if (changed > 1)
            {
                HKEY runonce;

                if (RegCreateKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_RUNONCE, &runonce) ==
                    ERROR_SUCCESS)
                {
                    RegSetValueEx(runonce, TEXT("WarnTimeChanged"), 0, REG_SZ,
                        (LPBYTE)c_szTimeChangedRunDLL, SIZEOF(c_szTimeChangedRunDLL));

                    RegCloseKey(runonce);
                }
            }
        }
    }
}

BOOL IsExecCmd(LPCTSTR pszCmd)
{
    return *pszCmd && !StrStrI(pszCmd, TEXT("-embedding"));
}

// run the cmd line passed up from win.com

void _RunWinComCmdLine(LPCTSTR pszCmdLine, UINT nCmdShow)
{
    if (IsExecCmd(pszCmdLine))
    {
        SHELLEXECUTEINFO ei = { SIZEOF(ei), 0, NULL, NULL, pszCmdLine, NULL, NULL, nCmdShow};

        ei.lpParameters = PathGetArgs(pszCmdLine);
        if (*ei.lpParameters)
            *((LPTSTR)ei.lpParameters - 1) = 0;     // const -> non const

        ShellExecuteEx(&ei);
    }
}

// stolen from the CRT, used to shirink our code

#ifdef DEBUG // For leak detection
BOOL g_fInitTable = FALSE;
LEAKDETECTFUNCS LeakDetFunctionTable;
#endif


// ccover uses runtime libs, that in turn require main()
#ifdef CCOVER

STDAPI_(int) ModuleEntry(void);

void __cdecl main()  {
    ModuleEntry();
};
#endif

STDAPI_(int) ModuleEntry(void)
{
    int i;
    STARTUPINFOA si;
    LPTSTR pszCmdLine;

#ifdef DEBUG
    // leak detection
    if(!g_fInitTable)
    {
        if(GetLeakDetectionFunctionTable(&LeakDetFunctionTable))
            g_fInitTable = TRUE;
    }
    if(g_fInitTable)
        LeakDetFunctionTable.pfnDebugMemLeak(DML_TYPE_THREAD | DML_BEGIN, TEXT(__FILE__), __LINE__);
#endif

#ifdef WINNT    
    // Hydra-Specific
    g_fRuningOnTerminalServer = IsTerminalServicesEnabled();
#endif

    pszCmdLine = GetCommandLine();

    //
    // We don't want the "No disk in drive X:" requesters, so we set
    // the critical error mask such that calls will just silently fail
    //

    SetErrorMode(SEM_FAILCRITICALERRORS);

    if ( *pszCmdLine == TEXT('\"') ) {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while ( *++pszCmdLine && (*pszCmdLine
             != TEXT('\"')) );
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *pszCmdLine == TEXT('\"') )
            pszCmdLine++;
    }
    else {
        while (*pszCmdLine > TEXT(' '))
            pszCmdLine++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*pszCmdLine && (*pszCmdLine <= TEXT(' '))) {
        pszCmdLine++;
    }

    si.dwFlags = 0;
    GetStartupInfoA(&si);

    i = ExplorerWinMain(GetModuleHandle(NULL), NULL, pszCmdLine,
                   si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);

    // Since we now have a way for an extension to tell us when it is finished,
    // we will terminate all processes when the main thread goes away.

#ifdef DEBUG
    // we stop leak detection on main shell thread here
    // we need to change this
    if (g_fInitTable)
        LeakDetFunctionTable.pfnDebugMemLeak(DML_TYPE_THREAD | DML_END, TEXT(__FILE__), __LINE__);
#endif

    if (g_fExitExplorer)    // desktop told us not to exit
    {
        TraceMsg(DM_SHUTDOWN, "c.me: call ExitProcess");
#ifdef CCOVER
        cov_write();
#endif
        ExitProcess(i);
    }

    DebugMsg(DM_TRACE, TEXT("c.me: Cabinet main thread exiting without ExitProcess."));
    return i;
}

void _InitComctl32()
{
    INITCOMMONCONTROLSEX icce;

    // init the controls
    icce.dwICC = ICC_COOL_CLASSES | ICC_WIN95_CLASSES | ICC_PAGESCROLLER_CLASS;
    icce.dwSize = sizeof(icce);
    InitCommonControlsEx(&icce);
}



// cached "handle" to the desktop
HANDLE g_hDesktop = NULL;
extern IDeskTray* const c_pdtray;

BOOL CreateDesktopAndTray()
{
    BOOL fRet = TRUE;

    if (g_dwProfileCAP & 0x00008000)
        StartCAPAll();

    if (!v_hwndTray)
    {
        InitTrayClass(hinstCabinet);
        if (!InitTray(hinstCabinet))
            return FALSE;
    }

    ASSERT(v_hwndTray);

    if (!v_hwndDesktop)
    {
        ASSERT(!g_hDesktop);

        // cache the handle to the desktop...
        g_hDesktop = SHCreateDesktop(c_pdtray);
        if (g_hDesktop == NULL)
            fRet = FALSE;
    }

    if (g_dwProfileCAP & 0x80000000)
        StopCAPAll();

    return fRet;
}

//
//  The "Session key" is a volatile registry key unique to this session.
//  A session is a single continuous logon.  If Explorer crashes and is
//  auto-restarted, the two Explorers share the same session.  But if you
//  log off and back on, that new Explorer is a new session.
//
//  Note that Win9x doesn't support volatile registry keys, so we have to
//  fake it.  IsFirstInstanceAfterLogon() answers the question *and*
//  initializes the session key.
//

//
//  The s_SessionKeyName is the name of the session key relative to
//  REGSTR_PATH_EXPLORER\SessionInfo.  On NT, this is normally the
//  Authentication ID, but we pre-initialize it to something safe so
//  we don't fault if for some reason we can't get to it.  Since
//  Win95 supports only one session at a time, it just stays at the
//  default value.
//
//  Sometimes we want to talk about the full path (SessionInfo\BlahBlah)
//  and sometimes just the partial path (BlahBlah) so we wrap it inside
//  this goofy structure.
//

union SESSIONKEYNAME {
    TCHAR szPath[12+16+1];
    struct {
        TCHAR szSessionInfo[12];    // strlen("SessionInfo\\")
        TCHAR szName[16+1];         // 16 = two DWORDs converted to hex
    };
} s_SessionKeyName = {
    { TEXT("SessionInfo\\.Default") }
};

HKEY GetSessionKey(REGSAM samDesired)
{
    HKEY hkExp;
    HKEY hkSession = NULL;
    DWORD dwDisposition;
    LONG lRes;

    //
    // Must create this key in multiple steps, because we want
    // SessionInfo to be volatile, but REGSTR_PATH_EXPLORER to be
    // nonvolatile.
    //

    lRes = RegCreateKeyEx(HKEY_CURRENT_USER,
                          REGSTR_PATH_EXPLORER, 0,
                          NULL, REG_OPTION_NON_VOLATILE, MAXIMUM_ALLOWED,
                          NULL, &hkExp, &dwDisposition);
    if (lRes == ERROR_SUCCESS)
    {
        lRes = RegCreateKeyEx(hkExp, s_SessionKeyName.szPath, 0,
                       NULL,
                       REG_OPTION_VOLATILE,
                       samDesired,
                       NULL,
                       &hkSession,
                       &dwDisposition );
        RegCloseKey(hkExp);
    }

    return hkSession;
}

// Removes the session key from the registry.
void NukeSessionKey(void)
{
    HKEY hkExp;
    LONG lRes;

    lRes = RegOpenKey(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER, &hkExp);
    if (lRes == ERROR_SUCCESS)
    {
        SHDeleteKey(hkExp, s_SessionKeyName.szPath);
        RegCloseKey(hkExp);
    }
}

BOOL IsFirstInstanceAfterLogon()
{
    BOOL fResult = FALSE;

#ifdef WINNT
    LONG lRes;
    HANDLE hToken;
    //
    //  Build the name of the session key.  We use the authentication ID
    //  which is guaranteed to be unique forever.  We can't use the
    //  Hydra session ID since that can be recycled.
    //
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        TOKEN_STATISTICS stats;
        DWORD cbOut;

        if (GetTokenInformation(hToken, TokenStatistics, &stats, sizeof(stats), &cbOut))
        {
            HKEY hkSession;
            wsprintf(s_SessionKeyName.szName, TEXT("%08x%08x"),
                     stats.AuthenticationId.HighPart,
                     stats.AuthenticationId.LowPart);
            hkSession = GetSessionKey(KEY_WRITE);
            if (hkSession) {
                HKEY hkStartup;
                DWORD dwDisposition;
                lRes = RegCreateKeyEx(hkSession, TEXT("StartupHasBeenRun"), 0,
                               NULL,
                               REG_OPTION_VOLATILE,
                               KEY_WRITE,
                               NULL,
                               &hkStartup,
                               &dwDisposition );
                if (lRes == ERROR_SUCCESS)
                {
                    RegCloseKey(hkStartup);
                    if (dwDisposition == REG_CREATED_NEW_KEY)
                        fResult = TRUE;
                }
                RegCloseKey(hkSession);
            }
        }
        CloseHandle(hToken);
    }
#else
    // on win95, we use the overloaded RegisterShellHook to thunk to the 16 bit side to do 
    // the work for us. this gets fixed when the tray shuts down properly...
    fResult = RegisterShellHook(NULL,(BOOL) 4 );
    if (fResult) {
        // Clean out the "volatile" session info since Win9x doesn't support
        // volatile regkeys.
        NukeSessionKey();
    }
#endif
    return fResult;
}

//
//  dwValue is FALSE if this is startup, TRUE if this is shutdown,
//
void WriteCleanShutdown(DWORD dwValue)
{
    RegSetValueEx(g_hkeyExplorer, TEXT("CleanShutdown"), 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(dwValue));

    // If we are shutting down for real (i.e., not fake), then clean up the
    // session key so we don't leak a bazillion volatile keys into the
    // registry on a Hydra system when people log on and off and on and off...
    if (dwValue && !g_fFakeShutdown) {
        NukeSessionKey();
    }
}

BOOL ReadCleanShutdown()
{
    DWORD dwValue = 1;  // default: it was clean
    DWORD dwSize = sizeof(dwValue);

    RegQueryValueEx(g_hkeyExplorer, TEXT("CleanShutdown"), NULL, NULL, (LPBYTE)&dwValue, &dwSize);
    return (BOOL)dwValue;
}

// APP HACK APP HACK
// installing SP3 over ie4 on NT4 trashes the RSA key.
// same code is in mainloop.c of iexplore.exe
#ifdef WINNT

#define RSA_PATH_TO_KEY    TEXT("Software\\Microsoft\\Cryptography\\Defaults\\Provider\\Microsoft Base Cryptographic Provider v1.0")
#define CSD_REG_PATH       TEXT("System\\CurrentControlSet\\Control\\Windows")
#define CSD_REG_VALUE      TEXT("CSDVersion")

// the signatures we are looking for in the regsitry so that we can patch up

#ifdef _M_IX86
static  BYTE  SP3Sig[] = {0xbd, 0x9f, 0x13, 0xc5, 0x92, 0x12, 0x2b, 0x72,
                          0x4a, 0xba, 0xb6, 0x2a, 0xf9, 0xfc, 0x54, 0x46,
                          0x6f, 0xa1, 0xb4, 0xbb, 0x43, 0xa8, 0xfe, 0xf8,
                          0xa8, 0x23, 0x7d, 0xd1, 0x85, 0x84, 0x22, 0x6e,
                          0xb4, 0x58, 0x00, 0x3e, 0x0b, 0x19, 0x83, 0x88,
                          0x6a, 0x8d, 0x64, 0x02, 0xdf, 0x5f, 0x65, 0x7e,
                          0x3b, 0x4d, 0xd4, 0x10, 0x44, 0xb9, 0x46, 0x34,
                          0xf3, 0x40, 0xf4, 0xbc, 0x9f, 0x4b, 0x82, 0x1e,
                          0xcc, 0xa7, 0xd0, 0x2d, 0x22, 0xd7, 0xb1, 0xf0,
                          0x2e, 0xcd, 0x0e, 0x21, 0x52, 0xbc, 0x3e, 0x81,
                          0xb1, 0x1a, 0x86, 0x52, 0x4d, 0x3f, 0xfb, 0xa2,
                          0x9d, 0xae, 0xc6, 0x3d, 0xaa, 0x13, 0x4d, 0x18,
                          0x7c, 0xd2, 0x28, 0xce, 0x72, 0xb1, 0x26, 0x3f,
                          0xba, 0xf8, 0xa6, 0x4b, 0x01, 0xb9, 0xa4, 0x5c,
                          0x43, 0x68, 0xd3, 0x46, 0x81, 0x00, 0x7f, 0x6a,
                          0xd7, 0xd1, 0x69, 0x51, 0x47, 0x25, 0x14, 0x40,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#else // other than _M_IX86
static  BYTE  SP3Sig[] = {0x8a, 0x06, 0x01, 0x6d, 0xc2, 0xb5, 0xa2, 0x66,
                          0x12, 0x1b, 0x9c, 0xe4, 0x58, 0xb1, 0xf8, 0x7d,
                          0xad, 0x17, 0xc1, 0xf9, 0x3f, 0x87, 0xe3, 0x9c,
                          0xdd, 0xeb, 0xcc, 0xa8, 0x6b, 0x62, 0xd0, 0x72,
                          0xe7, 0xf2, 0xec, 0xd6, 0xd6, 0x36, 0xab, 0x2d,
                          0x28, 0xea, 0x74, 0x07, 0x0e, 0x6c, 0x6d, 0xe1,
                          0xf8, 0x17, 0x97, 0x13, 0x8d, 0xb1, 0x8b, 0x0b,
                          0x33, 0x97, 0xc5, 0x46, 0x66, 0x96, 0xb4, 0xf7,
                          0x03, 0xc5, 0x03, 0x98, 0xf7, 0x91, 0xae, 0x9d,
                          0x00, 0x1a, 0xc6, 0x86, 0x30, 0x5c, 0xc8, 0xc7,
                          0x05, 0x47, 0xed, 0x2d, 0xc2, 0x0b, 0x61, 0x4b,
                          0xce, 0xe5, 0xb7, 0xd7, 0x27, 0x0c, 0x9e, 0x2f,
                          0xc5, 0x25, 0xe3, 0x81, 0x13, 0x9d, 0xa2, 0x67,
                          0xb2, 0x26, 0xfc, 0x99, 0x9d, 0xce, 0x0e, 0xaf,
                          0x30, 0xf3, 0x30, 0xec, 0xa3, 0x0a, 0xfe, 0x16,
                          0xb6, 0xda, 0x16, 0x90, 0x9a, 0x9a, 0x74, 0x7a,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#endif      // _M_IX86

void CheckForSP3RSAOverwrite( void )
{
    // check for them having installed NTSP3 over the top of IE4, it nukes
    // the RSABASE reg stuff, so we have to re-do it. (our default platform is NT + SP3, but this
    // problem doesn't occur on NT5, so ignore it.
    OSVERSIONINFO osVer;

    ZeroMemory(&osVer, sizeof(osVer));
    osVer.dwOSVersionInfoSize = sizeof(osVer);

    if( GetVersionEx((OSVERSIONINFO *)&osVer) && (osVer.dwPlatformId == VER_PLATFORM_WIN32_NT)
        && (osVer.dwMajorVersion == 4))
    {
        // now check to see we are on SP3 ...
        DWORD dwValue = 0;
        DWORD dwSize = sizeof( dwValue );

        if ( ERROR_SUCCESS == SHGetValue( HKEY_LOCAL_MACHINE, CSD_REG_PATH, CSD_REG_VALUE, NULL,
             &dwValue, &dwSize) && LOWORD( dwValue ) == 0x300 )
        {
            BYTE rgbSig[136];
            dwSize = sizeof(rgbSig);

            if (ERROR_SUCCESS == SHGetValue ( HKEY_LOCAL_MACHINE, RSA_PATH_TO_KEY, TEXT("Signature"), NULL,
                rgbSig, &dwSize))
            {

                if ((dwSize == sizeof(SP3Sig)) &&
                    (0 == memcmp(SP3Sig, rgbSig, sizeof(SP3Sig))))
                {
                    // need to do a DLLRegisterServer on RSABase
                    HINSTANCE hInst = LoadLibrary(TEXT("rsabase.dll"));
                    if ( hInst )
                    {
                        FARPROC pfnDllReg = GetProcAddress( hInst, "DllRegisterServer");
                        if ( pfnDllReg )
                        {
                            __try
                            {
                                pfnDllReg();
                            }
                            __except( EXCEPTION_EXECUTE_HANDLER)
                            {
                            }
                        }

                        FreeLibrary( hInst );
                    }
                }
            }
        }
    }
}
#else
#define CheckForSP3RSAOverwrite()
#endif

#ifdef WINNT

//
//  Synopsis:   Waits for the OLE SCM process to finish its initialization.
//              This is called before the first call to OleInitialize since
//              the SHELL runs early in the boot process.
//
//  Arguments:  None.
//
//  Returns:    S_OK - SCM is running. OK to call OleInitialize.
//              CO_E_INIT_SCM_EXEC_FAILURE - timed out waiting for SCM
//              other - create event failed
//
//  History:    26-Oct-95   Rickhi  Extracted from CheckAndStartSCM so
//                                  that only the SHELL need call it.
//
HRESULT WaitForSCMToInitialize()
{
    static BOOL s_fScmStarted = FALSE;
    HANDLE hEvent;
    SECURITY_ATTRIBUTES sa;
    SECURITY_DESCRIPTOR sd;
    SECURITY_ATTRIBUTES *psa;


    if (s_fScmStarted)
        return S_OK;

    psa =  CreateAllAccessSecurityAttributes(&sa, &sd);

    if (g_bRunOnNT5)
    {
        // on NT5 we need a global event that is shared between hydra sessions
        hEvent = CreateEvent(psa, TRUE, FALSE, SZ_SCMCREATEDEVENT_NT5);
    } 
    else
    {
        hEvent = CreateEvent(psa, TRUE, FALSE, SZ_SCMCREATEDEVENT);
    }

    if (hEvent)
    {
        // wait for the SCM to signal the event, then close the handle
        // and return a code based on the WaitEvent result.
        int rc = WaitForSingleObject(hEvent, 60000);

        CloseHandle(hEvent);

        if (rc == WAIT_OBJECT_0)
        {
            s_fScmStarted = TRUE;
            return S_OK;
        }
        else if (rc == WAIT_TIMEOUT)
        {
            return CO_E_INIT_SCM_EXEC_FAILURE;
        }
    }
    return HRESULT_FROM_WIN32(GetLastError());  // event creation failed or WFSO failed.
}
#endif // WINNT


// OleInitialize()

STDAPI OleInitializeWaitForSCM()
{
#ifdef WINNT
    HRESULT hres = WaitForSCMToInitialize();
    if (FAILED(hres))
        return hres;
#endif
    return OleInitialize(NULL);
}


// we need to figure out the fFirstShellBoot on a per-user
// basis rather than once per machine.  We want the welcome
// splash screen to come up for every new user.

BOOL IsFirstShellBoot()
{
    DWORD dwDisp;
    HKEY hkey;
    BOOL fFirstShellBoot = TRUE;  // default value

    if (RegCreateKeyEx(HKEY_CURRENT_USER, REGTIPS, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                     NULL, &hkey, &dwDisp) == ERROR_SUCCESS)
    {
        DWORD dwSize = sizeof(fFirstShellBoot);

        RegQueryValueEx(hkey, TEXT("DisplayInitialTipWindow"), NULL, NULL, (LPBYTE)&fFirstShellBoot, &dwSize);

        if (fFirstShellBoot)
        {
            // Turn off the initial tip window for future shell starts.
            BOOL bTemp = FALSE;
            RegSetValueEx(hkey, TEXT("DisplayInitialTipWindow"), 0, REG_DWORD, (LPBYTE) &bTemp, sizeof(bTemp));
        }
        RegCloseKey(hkey);
    }
    return fFirstShellBoot;
}

//
// Post a message to the MTTF window if it is present
// The MTTF tool tracks this process being up
//
#define STR_MTTF_WINDOW_CLASS TEXT("HARVEYCAT")
#define MTTF_STARTUP  40006  
#define MTTF_SHUTDOWN 40007

void PostMTTFMessage(WPARAM mttfMsg)
{
    HWND hwndVerFind = FindWindow(STR_MTTF_WINDOW_CLASS, NULL);
    if (hwndVerFind)
    {
        PostMessage(hwndVerFind, WM_COMMAND, 
                    (WPARAM) mttfMsg, 
                    (LPARAM) GetCurrentProcessId());
    }
}


int ExplorerWinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPTSTR pszCmdLine, int nCmdShow)
{
    DWORD dwShellStartTime;   // used for perf times

    CcshellGetDebugFlags();

    if (g_dwProfileCAP & 0x00000001)
        StartCAP();

    hinstCabinet = hInstance;

    g_fCleanBoot = GetSystemMetrics(SM_CLEANBOOT);      // also known as "Safe Mode"


    // Run IEAK via Wininet initialization if the autoconfig url is present.
    // No need to unload wininet in this case. Also only do this first time
    // Explorer loads (GetShellWindow() returns NULL).
    if (!GetShellWindow() && !g_fCleanBoot && SHRegGetUSValue(TEXT("Software\\Microsoft\\Windows\\Internet Settings"),
                                         TEXT("AutoConfigURL"),
                                         NULL, NULL, NULL, FALSE, NULL, 0) == ERROR_SUCCESS)
    {
        LoadLibrary(TEXT("WININET.DLL"));
    }


    // Very Important: Make sure to init dde prior to any Get/Peek/Wait().
    InitializeCriticalSection(&g_csDll);

    _InitComctl32();

    if (g_dwPrototype & 0x80000000)
    {
        // Turn off GDI batching so that paints are performed immediately
        GdiSetBatchLimit(1);
    }

    RegCreateKey(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER, &g_hkeyExplorer);
    ASSERT(g_hkeyExplorer); // Really really bad..  unable to create reg explorer key

    if (!ShouldStartDesktopAndTray())
    {
        MyCreateFromDesktop(hInstance, pszCmdLine, nCmdShow);
    }
    else
    {
        MSG msg;

        /* In case shell32 was kept in memory by a service process, tell him to
         * refresh all restrictions, before we call SHRestricted or any other
         * shell API that might depend on restrictions (SHGetSpecialFolderPath,
         * for example). Otherwise, the current user will inherit some of the
         * previous user's restrictions.
         */
        SHSettingsChanged(0, 0);

        /*
         *  BUGBUG - Win9x - we need to nuke the drives list so the incoming
         *  user doesn't inherit drive icons from the previous user
         */

        dwShellStartTime = GetTickCount();    // Compute shell startup time for perf automation

        ShellDDEInit(TRUE);        // use shdocvw shell DDE code.

        //  Specify the shutdown order of the shell process.  2 means
        //  the explorer should shutdown after everything but ntsd/windbg
        //  (level 0).  (Taskman used to use 1, but is no more.)

        SetProcessShutdownParameters(2, 0);

        _AutoRunTaskMan();

        // NB Make this the primary thread by calling peek message
        // for a message we know we're not going to get.
        // If we don't do it really soon, the notify thread can sometimes
        // become the primary thread by accident. There's a bunch of
        // special code in user to implement DDE hacks by assuming that
        // the primary thread is handling DDE.
        // Also, the PeekMsg() will cause us to set the WaitForInputIdle()
        // event so we better be ready to do all dde.

        PeekMessage(&msg, NULL, WM_QUIT, WM_QUIT, PM_NOREMOVE);

        // Make sure we are the first one to call the FileIconInit...
        FileIconInit(TRUE); // Tell the shell we want to play with a full deck

        g_fLogonCycle = IsFirstInstanceAfterLogon();
        g_fCleanShutdown = ReadCleanShutdown();

        if (g_fLogonCycle)
        {
            HWND hwndWait;

            // force kernel32 to update the timezone before running any apps
            DoDaylightCheck(TRUE);

            ProcessRunOnceEx();

            // We are about to do something that might take a long time so we want to display the
            // wait cursor, however we haven't created the desktop window yet.  To get around this
            // we create a temporary window which we use to display the wait cursor.
            // REVIEW: Maybe we should move this up to show the wait cursor sooner and longer?
            hwndWait = CreateRegAppsWaitWindow();
            RunRegAppsAndObjects(HKEY_LOCAL_MACHINE, REGSTR_PATH_RUNONCE, RRA_DELETE | RRA_WAIT);
            DestroyRegAppsWaitWindow(hwndWait);
        }

        _CreateAppGlobals();

        CheckForSP3RSAOverwrite();

        if (g_fCleanBoot)
            DisplayCleanBootMsg();  // let users know we are in safe mode

        CreateShellDirectories();   // Create the other special folders.

        // Run install stubs for the current user, mostly to propagate
        // shortcuts to apps installed by another user.
        if (!g_fCleanBoot)
            RunInstallUninstallStubs();

        OleInitializeWaitForSCM();

        if (!g_fCleanShutdown)
        {

            HWND hwndVerFind;
            IActiveDesktopP *piadp;

            //
            // Post a message to the MTTF window if it is present
            //
            // BUGBUG Get LOR to use RegisterWindowMessage()
            // 
            hwndVerFind = FindWindow(TEXT("BFG2000"), NULL);
            if (hwndVerFind)
            {
                PostMessage(hwndVerFind, WM_COMMAND, 40005, 0);
            }

            // Put the active desktop in safe mode if we faulted previously and this is a subsequent instance
            if (SUCCEEDED(CoCreateInstance(&CLSID_ActiveDesktop, NULL, CLSCTX_INPROC, &IID_IActiveDesktopP, (void **)&piadp)))
            {
                piadp->lpVtbl->SetSafeMode(piadp, SSM_SET | SSM_UPDATE);
                piadp->lpVtbl->Release(piadp);
            }
        }

        PostMTTFMessage(MTTF_STARTUP);
        WriteCleanShutdown(FALSE);    // assume we will have a bad shutdown

        WinList_Init();

        // If any of the shellwindows are already present, then we want to bail out.
        //
        // NOTE: Compaq shell changes the "shell=" line during RunOnce time and
        // that will make ShouldStartDesktopAndTray() return FALSE

        if (!IsAnyShellWindowAlreadyPresent() && CreateDesktopAndTray())
        {
            _RunWinComCmdLine(pszCmdLine, nCmdShow);

            if (StopWatchMode())
            {
                // We used to save these off into global vars, and then write them at
                // WM_ENDSESSION, but that seems too unreliable
                DWORD dwShellStopTime = GetTickCount();
                StopWatch_StartTimed(SWID_STARTUP, TEXT("Shell Startup: Start"), SPMODE_SHELL | SPMODE_DEBUGOUT, dwShellStartTime);
                StopWatch_StopTimed(SWID_STARTUP, TEXT("Shell Startup: Stop"), SPMODE_SHELL | SPMODE_DEBUGOUT, dwShellStopTime);
            }

            if (g_dwProfileCAP & 0x00010000)
                StopCAP();

            // this must be whomever is the window on this thread
            SHDesktopMessageLoop(g_hDesktop);

            WriteCleanShutdown(TRUE);    // we made it out ok, record that fact
            PostMTTFMessage(MTTF_SHUTDOWN);
        }

        WinList_Terminate();    // Turn off our window list processing
        OleUninitialize();

        ShellDDEInit(FALSE);    // use shdocvw shell DDE code
    }

    DebugMsg(DM_TRACE, TEXT("c.App Exit."));

    return TRUE;
}

DWORD WINAPI RunStartupAppsThread(void *pvVoid)
{
    // Some of the items we launch during startup assume that com is initialized.  Make this
    // assumption true.
    CoInitialize(0);

    // These global flags are set once long before our thread starts and are then only
    // read so we don't need to worry about timing issues.
    if (g_fLogonCycle && !g_fCleanBoot)
    {
        // We only run these startup items if g_fLogonCycle is TRUE. This prevents
        // them from running again if the shell crashes and restarts.

        _DoRunEquals();     // Process the Load= and Run= lines...
        RunRegAppsAndObjects(HKEY_LOCAL_MACHINE, REGSTR_PATH_RUN, RRA_NOUI);
        RunRegAppsAndObjects(HKEY_CURRENT_USER, REGSTR_PATH_RUN, RRA_NOUI);
        _ExecuteStartupPrograms(NULL);
    }

    // As a best guess, the CU\RunOnce key is executed regardless of the g_fLogonCycle
    // becuase it was once hoped that we could install newer versions of IE without
    // requiring a reboot.  They would place something in the CU\RunOnce key and then
    // shutdown and restart the shell to continue their setup process.  I believe this
    // idea was later abandoned but the code change is still here.  Since that could
    // some day be a useful feature I'm leaving it the same.
    RunRegAppsAndObjects(HKEY_CURRENT_USER, REGSTR_PATH_RUNONCE, RRA_DELETE|RRA_NOUI);

    // we need to run all the non-blocking items first.  Then we spend the rest of this threads life
    // runing the synchronized objects one after another.
    if (g_fLogonCycle && !g_fCleanBoot)
    {
        _RunWelcome();
        RunRegAppsAndObjects(HKEY_LOCAL_MACHINE, REGSTR_PATH_RUNONCE, RRA_NOUI|RRA_RUNSUBKEYS|RRA_WAIT|RRA_DELETE);
        RunRegAppsAndObjects(HKEY_CURRENT_USER,  REGSTR_PATH_RUNONCE, RRA_NOUI|RRA_RUNSUBKEYS|RRA_WAIT|RRA_DELETE);
        RunRegAppsAndObjects(HKEY_LOCAL_MACHINE, REGSTR_PATH_RUN,     RRA_NOUI|RRA_RUNSUBKEYS|RRA_WAIT);
        RunRegAppsAndObjects(HKEY_CURRENT_USER,  REGSTR_PATH_RUN,     RRA_NOUI|RRA_RUNSUBKEYS|RRA_WAIT);
    }

    CoUninitialize();

    return TRUE;
}

void RunStartupApps()
{
    DWORD dwThreadID;
    HANDLE handle;

    handle = CreateThread( NULL, 0, RunStartupAppsThread, 0, 0, &dwThreadID );
    if ( handle )
    {
        CloseHandle(handle);
    }
    else
    {
        // we couldn't create the thread so just call the thread proc directly.
        // The RunStartupAppsThread function takes a long time to complete and
        // the UI thread will be non-responsive.  We call this from the tray.
        // REVIEW: Is this really safe?
        RunStartupAppsThread(0);
    }
}
