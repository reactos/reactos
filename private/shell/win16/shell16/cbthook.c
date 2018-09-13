//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1991-1992
//
//---------------------------------------------------------------------------
#include "shprv.h"
#include <testing.h>

#pragma data_seg("_TEXT")
const char FAR c_szShellHook[] = "SHELLHOOK";
#pragma data_seg()

#define OUR_MAX_HOOKS 10
#define HSHELL_OVERFLOW (WM_USER+42)

// bugbug, move this to shellp.h
#define HSHELL_HIGHBIT 0x8000
#define HSHELL_FLASH (HSHELL_REDRAW|HSHELL_HIGHBIT)

static int g_chwndHooks = 0;        // count of hooks

static HWND g_ahwndHooks[OUR_MAX_HOOKS];

static HHOOK g_hhook = NULL;
static UINT g_WMShellHook = 0;
BOOL NEAR PASCAL ShellHook_Remove(HWND hwnd);


// this is for backwards compat only.
WORD       wWMOtherWindowCreated = 0;
WORD       wWMOtherWindowDestroyed = 0;
WORD       wWMActivateShellWindow=0;
HWND       hwndProgMan = NULL;

HWND       hwndTray = NULL;

// Stuff to keep from blowing users heap
int        cTrayPostMessages = 0;
BOOL        fTrayOverflowed = FALSE;
#define     MAX_POSTS_PENDING   50

void NEAR PASCAL ForceUnhook()
{
    if (g_hhook) {
        UnhookWindowsHookEx(g_hhook);
        g_hhook = NULL;
    }
}

//---------------------------------------------------------------------------
void NEAR PASCAL ShellNotifyAll(int code, HWND hwndShell, LPARAM lParam)
{
	HWND hwnd;
	int i;

#if 0 // def DEBUG
	char sz[128];
	LONG lStyle;

	lStyle = GetWindowLong(hwndShell, GWL_STYLE);
	wsprintf(sz, "s.Shellna: Hwnd %#04x Code %d Parent %#04x Style %#08x.\n\r", hwndShell, code, GetParent(hwndShell), lStyle);
	OutputDebugString(sz);
#endif

    switch (code) {

    case HSHELL_ACTIVATESHELLWINDOW:
        if (hwndProgMan) {

            // win 31, hwndProgMan is progman
            PostMessage(hwndProgMan, wWMActivateShellWindow, 0, 0L);

        } else if ((hwndShell == (HWND)1) && hwndTray) {
            // tray was registered and it faulted (hwndShell == 1 is our code)
            ShellHook_Remove(hwndTray);
            ForceUnhook();
        } else {
            goto DefaultCase;
        }
        break;

    case HSHELL_TASKMAN:
        if (hwndTray) {
            // Don't Blow users heap...
            if (cTrayPostMessages < MAX_POSTS_PENDING)
            {
                if (PostMessage(hwndTray, g_WMShellHook, (WPARAM)code, (LPARAM)hwndShell))
                    cTrayPostMessages++;
                else fTrayOverflowed = TRUE;
            }
            else
                fTrayOverflowed = TRUE;
            break;
        }
        // else fall through

    default:

DefaultCase:
        // Tell everyone about the Shell event.
        for (i = 0; i < g_chwndHooks; i++)
        {
            hwnd = g_ahwndHooks[i];
            if (IsWindow(hwnd))
            {
                if (hwnd == hwndProgMan)  {
                    // this is for backwards compatibility only
                    if ((code == HSHELL_WINDOWCREATED) ||
                        (code == HSHELL_WINDOWDESTROYED))
                        PostMessage(hwnd,  (code == HSHELL_WINDOWCREATED ?
                                            wWMOtherWindowCreated : wWMOtherWindowDestroyed),
                                    (WPARAM)hwndShell, 0L);
                } else {
                    if ((code == HSHELL_WINDOWACTIVATED) && lParam) {
                        code |= HSHELL_HIGHBIT;
                    }
                    if (hwnd == hwndTray)
                    {
                        if (cTrayPostMessages < MAX_POSTS_PENDING)
                        {
                            if (PostMessage(hwnd, g_WMShellHook, (WPARAM)code, (LPARAM)hwndShell))
                                cTrayPostMessages++;
                            else
                                fTrayOverflowed = TRUE;
                        }
                        else
                            fTrayOverflowed = TRUE;
                    }
                    else
                        PostMessage(hwnd, g_WMShellHook, (WPARAM)code, (LPARAM)hwndShell);
                }
            } else
                ShellHook_Remove(hwnd);
        }
    }
}

typedef struct
{
    DWORD   hwnd;
    RECT    rc;
} SHELLHOOKINFO, FAR *LPSHELLHOOKINFO;

DWORD FAR PASCAL MapSL(void FAR *);


// We do all this global fix, mapsl stuff because we're
// sending private messages, so we don't get thunked.
// do it ourselves.
LRESULT NEAR PASCAL ShellNotifyAllNow(int code, HWND hwndShell, LPARAM lParamOrg)
{
    HGLOBAL hshi;
    LPSHELLHOOKINFO lpshi;
    int i;
    HWND hwnd;
    LRESULT lres = 0;
    LPARAM lParam;

    if (code == HSHELL_GETMINRECT)
    {
        // GPTR is GMEM_FIXED, so no need to GlobalFix it.
        hshi = GlobalAlloc(GPTR, sizeof(SHELLHOOKINFO));
        if (!hshi)
            return(0L);

        // No need to copy rect up
        lpshi = MAKELP(hshi, 0);
        lpshi->hwnd = hwndShell;

        lParam = MapSL(lpshi);
    }
    else
        lParam = hwndShell;


    // Tell everyone about the Shell event.
    for (i = 0; i < g_chwndHooks; i++)
    {
        hwnd = g_ahwndHooks[i];
        if (IsWindow(hwnd))
        {
            lres |= SendMessage(hwnd, g_WMShellHook, (WPARAM)code,
                        lParam);
        } else {
            ShellHook_Remove(hwnd);
            i--;
        }
    }

    if (code == HSHELL_GETMINRECT)
    {
        // Copy rect back down
        CopyRect((LPRECT)lParamOrg, &lpshi->rc);
        GlobalFree(hshi);
    }
    return lres;
}


//---------------------------------------------------------------------------
LRESULT CALLBACK _ShellHookProc(int code, WPARAM wparam, LPARAM lparam)
{
	// Filter the events.
	switch (code)
	{	
            case HSHELL_GETMINRECT:
                return ShellNotifyAllNow(code, (HWND)wparam, lparam);

            case HSHELL_REDRAW:
                if (lparam) {
                    code = HSHELL_FLASH;
                }
            case HSHELL_TASKMAN:
                ShellNotifyAll(code, (HWND)wparam, lparam);
                return TRUE;

            default:
                ShellNotifyAll(code, (HWND)wparam, lparam);
                break;
        }

        // ShellNotifyAll may unhook us
        if (g_hhook != NULL)
            return CallNextHookEx(g_hhook, code, wparam, lparam);
}

//---------------------------------------------------------------------------
// Make sure the given window gets notified about Shell style things.
BOOL NEAR PASCAL ShellHook_Add(HWND hwnd)
{
	// No hooks yet, initialise everything.

        if (g_chwndHooks >= OUR_MAX_HOOKS)
        {
            DebugMsg(DM_TRACE, "Shellh_a: Installing Shell hook fail, table full");
            return FALSE;       // Max number of hooks
        }

        // If the current count is zero, we need to actually set the hook
        // or if the g_hhook got cleared (from gpf) and we're now re-registering the shell..
        // don't do it whenever the g_hhook is NULL because we want to be
        // registered on behalf of the shell main process
        if (g_chwndHooks == 0 || (hwndTray == hwnd))
        {

            // if we were previously hooked (like by taskman running before tray does)
            // unhook... we always want to be hooked onbehalf of tray
            ForceUnhook();

      	    DebugMsg(DM_TRACE, "Shellh_a: Installing Shell hook");
      	    g_WMShellHook = RegisterWindowMessage(c_szShellHook);
      	    g_hhook = SetWindowsHookEx(WH_SHELL, (HOOKPROC)_ShellHookProc, HINST_THISDLL, 0);
      	    if (!g_hhook)
      	        return FALSE;
        }

        if (hwnd == hwndProgMan && wWMOtherWindowCreated == 0) {
            wWMOtherWindowCreated   = RegisterWindowMessage("OTHERWINDOWCREATED");
            wWMOtherWindowDestroyed = RegisterWindowMessage("OTHERWINDOWDESTROYED");
            wWMActivateShellWindow  = RegisterWindowMessage("ACTIVATESHELLWINDOW");
        }

        g_ahwndHooks[g_chwndHooks++] = hwnd;
        //DebugMsg(DM_TRACE, "Shellh_a: Shell hook installed for %x, Count=%d",hwnd, g_chwndHooks);
}

//---------------------------------------------------------------------------
BOOL NEAR PASCAL ShellHook_Remove(HWND hwnd)
{
	int i;

	if (g_chwndHooks == 0)
	    return FALSE;

	// Find the given window in the list.
	for (i = 0; i < g_chwndHooks; i++)
	{
            if (hwnd == g_ahwndHooks[i])
            {
		// Found it. So remove it from the list
                g_chwndHooks--; // dec count
                if (i != g_chwndHooks)
                    g_ahwndHooks[i] = g_ahwndHooks[g_chwndHooks];
                //DebugMsg(DM_TRACE, "Shellh_a: Shell hook removed for %x, Count=%d", hwnd, g_chwndHooks);
		break;
            }
	}
	
	// Is anyone left?
	if (g_chwndHooks == 0)
	{
	    // Nope, clean up.
	    DebugMsg(DM_TRACE, "Shellh_r: Removing Shell hook");
	    UnhookWindowsHookEx(g_hhook);
	    g_hhook = NULL;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
// Make it look like the other hook installer.
BOOL WINAPI RegisterShellHook(HWND hwnd, BOOL fInstall)
{
    // gross hacks galore...

    switch (fInstall) {
    case 2:
        // from win 3.1 to know what to activate on the fault
        // and because they use special registered messages
        hwndProgMan = hwnd;
        break;

    case 3:
        // because we can't change api's and we need to do a similar thing for
        // the tray
        hwndTray = hwnd;
        cTrayPostMessages = 0;
        break;

    case 4:
        // cheap trick to see if tray is already registered
        return (hwndTray == NULL);

    case 5:
        // Another hack to decrement count of messages pending
        if (cTrayPostMessages > 0) {
            cTrayPostMessages--;

            if (fTrayOverflowed && (cTrayPostMessages < 5 ))
            {
                if (PostMessage(hwndTray, g_WMShellHook, (WPARAM)HSHELL_OVERFLOW, (LPARAM)-1))
                    fTrayOverflowed = FALSE;
            }
        }
        return TRUE;

    case 0:
        if (hwnd == hwndProgMan)
            hwndProgMan = 0;
        else if (hwnd == hwndTray)
            hwndTray = NULL;

        return ShellHook_Remove(hwnd);
    }

    return ShellHook_Add(hwnd);
}

//
// Simple function that we can call from the 32 bit side that can ask user
// and if necessary GDI if they still have some memory left.  If not we should
// probably bail out of things that spawn more windows/processes...
//

#define USER_ALLOC_SIZE     512
BOOL WINAPI CheckResourcesBeforeExec()
{
    WORD hAlloc;
    hAlloc = (WORD)UserSeeUserDo(SD_LOCALALLOC, LMEM_FIXED, USER_ALLOC_SIZE);
    if (hAlloc)
    {
        UserSeeUserDo(SD_LOCALFREE, (WPARAM)hAlloc, 0);
        return TRUE;
    }

    return FALSE;
}
