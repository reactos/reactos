//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1991-1992
//
//---------------------------------------------------------------------------

#include "shprv.h"

//---------------------------------------------------------------------------
// Global to this file only...
#define CBSZKEY	6
typedef struct _ENUMHOTKEY
	{
	LPBYTE lpb;
	LPBYTE lpbCur;
	} ENUMHOTKEY, FAR *LPENUMHOTKEY;
typedef struct _PENDINGHOTKEYS
	{
	LPSTR lpszPath;
	WORD wHKey;
	} PENDINGHOTKEYS, FAR *LPPENDINGHOTKEYS;
HOOKPROC lpNextShellHookProc = NULL;
HDSA g_hdsaKeys = NULL;
HHOOK g_hhookShell;

void FAR PASCAL Hotkey_Terminate();

//---------------------------------------------------------------------------
// Given an existing window, see if there's a pending a hotkey for it and
// if there is then set it and delete it from the list of pending hotkeys.
// Returns the count of pending hotkeys.
UINT NEAR PASCAL AssignPendingHotkey(HWND hwnd)
	{
	LPPENDINGHOTKEYS lpphk;
	int i, cItems;
	HINSTANCE hinst;
	char szPath[MAXPATHLEN];

        if (g_hdsaKeys == NULL)
            return 0;

	// Get the path of the app that created the given window.
 	hinst = GetWindowInstance(hwnd);
	GetModuleFileName(hinst, szPath, sizeof(szPath));

//	DebugMsg(DM_TRACE, "s.aph: Looking for hotkeys for 0x%x (%s).", hwnd, (LPSTR) szPath);

	// Go throught the list of pending hotkeys, if there's a match
	// then set the hotkey for that window.
	cItems = DSA_GetItemCount(g_hdsaKeys);
//	DebugMsg(DM_TRACE, "s.aph: %d pending hotkeys.", cItems);
	for (i=0; i<cItems; i++)
		{			
		lpphk = DSA_GetItemPtr(g_hdsaKeys, i);
		if (lstrcmpi(lpphk->lpszPath, szPath) == 0)
			{
			// Found a match. Set the windows hotkey.
//			DebugMsg(DM_TRACE, "s.aph: Setting hotkey 0x%x for %s", lpphk->wHKey, (LPSTR) lpphk->lpszPath);
			SendMessage(hwnd, WM_SETHOTKEY, lpphk->wHKey, 0);
			// Cleanup.
			Free(lpphk->lpszPath);
			DSA_DeleteItem(g_hdsaKeys, i);
			return cItems-1;
			}
		}
	return cItems;
	}

//---------------------------------------------------------------------------
// If there is a pending hotkey for the app lpszFrom then change it to be
// for lpszTo.
// Returns TRUE if there as was a pending hotkey for the given app.
// Returns FALSE otherwise.
BOOL WINAPI SHChangePendingHotkey(LPCSTR lpszFrom, LPCSTR lpszTo)
	{
	LPPENDINGHOTKEYS lpphk;
	LPSTR lpszNew;
	int i, cItems;

	if (!lpszFrom | !*lpszFrom || !lpszTo || !*lpszTo || !g_hdsaKeys)
		return FALSE;
	
//	DebugMsg(DM_TRACE, "s.cp: Changing pending hotkey.");

	cItems = DSA_GetItemCount(g_hdsaKeys);
	for (i=0; i<cItems; i++)
		{
		lpphk = DSA_GetItemPtr(g_hdsaKeys, i);
		// Match?
		if (lstrcmpi(lpphk->lpszPath, lpszFrom) == 0)
			{
			// Yep, change it.
//			DebugMsg(DM_TRACE, "s.hc: Changing pending hotkey from %s to %s", (LPSTR)lpszFrom, (LPSTR)lpszTo);
			lpszNew = ReAlloc(lpphk->lpszPath, lstrlen(lpszTo)+1);
			if (lpszNew)
				{
				lstrcpy(lpszNew, lpszTo);
				lpphk->lpszPath = lpszNew;
				return TRUE;
				}
			// FU.
			DebugMsg(DM_ERROR, "s.hc: Unable to change hotkey due to lack of memory");
			// This hotkey is now busted, nuke it.
			DSA_DeleteItem(g_hdsaKeys, i);
			break;
			}	
		}
	return FALSE;
	}

//---------------------------------------------------------------------------
void WINAPI SHDeleteAllPendingHotkeys(void)
	{
	LPPENDINGHOTKEYS lpphk;
	int i;

//	DebugMsg(DM_TRACE, "s.dap: Deleting all pending hotkeys");

        if (g_hdsaKeys == NULL)
            return;
	i = DSA_GetItemCount(g_hdsaKeys) - 1;
	while (i >= 0)
		{
		lpphk = DSA_GetItemPtr(g_hdsaKeys, i);
		if (lpphk->lpszPath)
			{
 			DebugMsg(DM_TRACE, "s.dap: Deleting pending hotkey for %s.", (LPSTR) lpphk->lpszPath);
			Free(lpphk->lpszPath);
			}
		DSA_DeleteItem(g_hdsaKeys, i);
		i--;
		}
	}

//---------------------------------------------------------------------------
// Deletes a pending hotkey.
// Returns TRUE if the key was succesfully deleted.
// NB This can be called before initialisation.
BOOL WINAPI SHDeletePendingHotkey(LPCSTR lpszPath)
	{
	LPPENDINGHOTKEYS lpphk;
	int i, cItems;

//	DebugMsg(DM_TRACE, "s.dp: Looking to delete pending hotkey for %s.", (LPSTR) lpszPath);

	if (!g_hdsaKeys)
		return FALSE;

	cItems = DSA_GetItemCount(g_hdsaKeys);
	for (i = 0; i < cItems; i++)
		{
		lpphk = DSA_GetItemPtr(g_hdsaKeys, i);
		if (lstrcmpi(lpszPath, lpphk->lpszPath) == 0)
			{
//			DebugMsg(DM_TRACE, "s.dp: Clearing pending hotkey for %s.", (LPSTR) lpphk->lpszPath);
			Free(lpphk->lpszPath);
			DSA_DeleteItem(g_hdsaKeys, i);

                        if (DSA_GetItemCount(g_hdsaKeys) == 0)
                            Hotkey_Terminate();

			return TRUE;
			}
		}
	return FALSE;
	}

//---------------------------------------------------------------------------
// Returns the hotkey for the first pending hotkey with the given path.
// NB This can be called before initialisation.
WORD WINAPI SHGetPendingHotkey(LPCSTR lpszPath)
	{
	LPPENDINGHOTKEYS lpphk;
	int i, cItems;


	if (!g_hdsaKeys)
		return 0;

//	DebugMsg(DM_TRACE, "s.gp:...");

	cItems = DSA_GetItemCount(g_hdsaKeys);
	for (i = 0; i < cItems; i++)
		{
		lpphk = DSA_GetItemPtr(g_hdsaKeys, i);
		if (lstrcmpi(lpszPath, lpphk->lpszPath) == 0)
			{
//			DebugMsg(DM_TRACE, "s.gp: Getting pending hotkey 0x%x for %s.", lpphk->wHKey, (LPSTR) lpszPath);
			return lpphk->wHKey;
			}
		}

//	DebugMsg(DM_TRACE, "s.gp: No pending hotkey for %s.", (LPSTR) lpszPath);
	return 0;
	}


//---------------------------------------------------------------------------
// Search for the first top level window belonging to the app with the
// given instance and set the hotkey for it.
// Returns TRUE if a window was found.
BOOL WINAPI SHSetHotkeyByInstance(HINSTANCE hinst, WORD wHotkey)
	{
	HWND hwnd;

//	DebugMsg(DM_TRACE, "s.hsbi: Setting hotkey for instance 0x%x.", hinst);

	// Go through the list of all top level windows looking for one with
	// the given instance.
	hwnd = GetWindow(GetDesktopWindow(), GW_CHILD);
	while (hwnd)
		{	
	 	if (GetWindowInstance(hwnd) == hinst)
			{
			// Found a match...
//			DebugMsg(DM_TRACE, "s.sfi: Setting hotkey 0x%x for window 0x%x", wHotkey, hwnd);
			SendMessage(hwnd, WM_SETHOTKEY, wHotkey, 0);
			return TRUE;
			}
		hwnd = GetWindow(hwnd, GW_HWNDNEXT);
		}
	return FALSE;
	}
	

//---------------------------------------------------------------------------
// Deals with setting hotkeys for things.
int CALLBACK ShellHookProc(int code, WPARAM wparam, LPARAM lparam)
{
    if (code == HSHELL_WINDOWCREATED)
        AssignPendingHotkey((HWND)wparam);

    return CallNextHookEx(g_hhookShell, code, wparam, lparam);
}

//---------------------------------------------------------------------------
BOOL FAR PASCAL Hotkey_Init(HINSTANCE hinst)
{
    if (g_hdsaKeys)
    	return TRUE;

    g_hdsaKeys =  DSA_Create(sizeof(PENDINGHOTKEYS), 8);
    if (g_hdsaKeys)
    {
        g_hhookShell = SetWindowsHookEx(WH_SHELL, ShellHookProc, hinst, 0);
        if (g_hhookShell)
        {
            // Hook installed.
            return TRUE;
        }
        DSA_Destroy(g_hdsaKeys);
        g_hdsaKeys = NULL;
    }
    DebugMsg(DM_ERROR, "Hotkey: failed to init");
    return FALSE;
}

//---------------------------------------------------------------------------
void FAR PASCAL Hotkey_Terminate()
{
    // Unhook ourselves.
    if (g_hhookShell) {
        UnhookWindowsHookEx(g_hhookShell);
        g_hhookShell = NULL;
    }

    // clean up remaining hotkeys that never got used.
    if (g_hdsaKeys)
    {
        SHDeleteAllPendingHotkeys();

        DSA_Destroy(g_hdsaKeys);

        g_hdsaKeys = NULL;
    }
}


//---------------------------------------------------------------------------
// Convert a hotkey and modifiers into a string number.
#define HotkeyToText(wHotkey, lpszText) wsprintf(lpszText, "%u", wHotkey)

//---------------------------------------------------------------------------
// Assign a hotkey for the next app to be run.
// Returns TRUE if the hotkey was set OK.
// FALSE otherwise.
BOOL WINAPI SHSetPendingHotkey(LPCSTR lpszPath, WORD wHotkey)
{
	PENDINGHOTKEYS phk;
	LPSTR lpsz;
	int i;

        if (!g_hdsaKeys)
            Hotkey_Init(HINST_THISDLL);
	
	if (!lpszPath || !*lpszPath || !wHotkey || !g_hdsaKeys)
		return FALSE;

//	DebugMsg(DM_TRACE, "s.hs: Pending hotkey 0x%x for %s", wHotkey, (LPSTR)lpszPath);

	i = DSA_GetItemCount(g_hdsaKeys);
	lpsz = Alloc(lstrlen(lpszPath)+1);
	if (lpsz)
		{
		lstrcpy(lpsz, lpszPath);
		phk.lpszPath = lpsz;
		phk.wHKey = wHotkey;
		DSA_InsertItem(g_hdsaKeys, i, &phk);
		return TRUE;
		}
	// Busted.
	DebugMsg(DM_ERROR, "s.hs: Not enough memory set a hotkey.");
	return FALSE;	
}
