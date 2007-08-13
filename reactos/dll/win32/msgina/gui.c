/*
 * PROJECT:         ReactOS msgina.dll
 * FILE:            dll/win32/msgina/gui.c
 * PURPOSE:         ReactOS Logon GINA DLL
 * PROGRAMMER:      Hervé Poussineau (hpoussin@reactos.org)
 */

#include "msgina.h"

//#define YDEBUG
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(msgina);

typedef struct _DISPLAYSTATUSMSG
{
	PGINA_CONTEXT Context;
	HDESK hDesktop;
	DWORD dwOptions;
	PWSTR pTitle;
	PWSTR pMessage;
	HANDLE StartupEvent;
} DISPLAYSTATUSMSG, *PDISPLAYSTATUSMSG;

static BOOL
GUIInitialize(
	IN OUT PGINA_CONTEXT pgContext)
{
	TRACE("GUIInitialize(%p)\n", pgContext);
	return TRUE;
}

static BOOL CALLBACK
StatusMessageWindowProc(
	IN HWND hwndDlg,
	IN UINT uMsg,
	IN WPARAM wParam,
	IN LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);

	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			PDISPLAYSTATUSMSG msg = (PDISPLAYSTATUSMSG)lParam;
			if (!msg)
				return FALSE;

			msg->Context->hStatusWindow = hwndDlg;

			if (msg->pTitle)
				SetWindowTextW(hwndDlg, msg->pTitle);
			SetDlgItemTextW(hwndDlg, IDC_STATUSLABEL, msg->pMessage);
			if (!msg->Context->SignaledStatusWindowCreated)
			{
				msg->Context->SignaledStatusWindowCreated = TRUE;
				SetEvent(msg->StartupEvent);
			}
			return TRUE;
		}
	}
	return FALSE;
}

static DWORD WINAPI
StartupWindowThread(LPVOID lpParam)
{
	HDESK OldDesk;
	PDISPLAYSTATUSMSG msg = (PDISPLAYSTATUSMSG)lpParam;

	OldDesk = GetThreadDesktop(GetCurrentThreadId());

	if(!SetThreadDesktop(msg->hDesktop))
	{
		HeapFree(GetProcessHeap(), 0, lpParam);
		return FALSE;
	}
	DialogBoxParam(
		hDllInstance,
		MAKEINTRESOURCE(IDD_STATUSWINDOW_DLG),
		GetDesktopWindow(),
		StatusMessageWindowProc,
		(LPARAM)lpParam);
	SetThreadDesktop(OldDesk);

	msg->Context->hStatusWindow = 0;
	msg->Context->SignaledStatusWindowCreated = FALSE;

	HeapFree(GetProcessHeap(), 0, lpParam);
	return TRUE;
}

static BOOL
GUIDisplayStatusMessage(
	IN PGINA_CONTEXT pgContext,
	IN HDESK hDesktop,
	IN DWORD dwOptions,
	IN PWSTR pTitle,
	IN PWSTR pMessage)
{
	PDISPLAYSTATUSMSG msg;
	HANDLE Thread;
	DWORD ThreadId;

	TRACE("GUIDisplayStatusMessage(%ws)\n", pMessage);

	if (!pgContext->hStatusWindow)
	{
		msg = (PDISPLAYSTATUSMSG)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DISPLAYSTATUSMSG));
		if(!msg)
			return FALSE;

		msg->Context = pgContext;
		msg->dwOptions = dwOptions;
		msg->pTitle = pTitle;
		msg->pMessage = pMessage;
		msg->hDesktop = hDesktop;

		msg->StartupEvent = CreateEventW(
			NULL,
			TRUE,
			FALSE,
			NULL);

		if (!msg->StartupEvent)
			return FALSE;

		Thread = CreateThread(
			NULL,
			0,
			StartupWindowThread,
			(PVOID)msg,
			0,
			&ThreadId);
		if (Thread)
		{
			CloseHandle(Thread);
			WaitForSingleObject(msg->StartupEvent, INFINITE);
			CloseHandle(msg->StartupEvent);
			return TRUE;
		}

		return FALSE;
	}

	if(pTitle)
		SetWindowTextW(pgContext->hStatusWindow, pTitle);

	SetDlgItemTextW(pgContext->hStatusWindow, IDC_STATUSLABEL, pMessage);

	return TRUE;
}

static BOOL
GUIRemoveStatusMessage(
	IN PGINA_CONTEXT pgContext)
{
	if (pgContext->hStatusWindow)
	{
		EndDialog(pgContext->hStatusWindow, 0);
		pgContext->hStatusWindow = 0;
	}

	return TRUE;
}

static INT_PTR CALLBACK
EmptyWindowProc(
	IN HWND hwndDlg,
	IN UINT uMsg,
	IN WPARAM wParam,
	IN LPARAM lParam)
{
	UNREFERENCED_PARAMETER(hwndDlg);
	UNREFERENCED_PARAMETER(uMsg);
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);

	return FALSE;
}

static VOID
GUIDisplaySASNotice(
	IN OUT PGINA_CONTEXT pgContext)
{
	INT result;

	TRACE("GUIDisplaySASNotice()\n");

	/* Display the notice window */
	result = DialogBoxParam(
		pgContext->hDllInstance,
		MAKEINTRESOURCE(IDD_NOTICE_DLG),
		GetDesktopWindow(),
		EmptyWindowProc,
		(LPARAM)NULL);
	if (result == -1)
	{
		/* Failed to display the window. Do as if the user
		 * already has pressed CTRL+ALT+DELETE */
		pgContext->pWlxFuncs->WlxSasNotify(pgContext->hWlx, WLX_SAS_TYPE_CTRL_ALT_DEL);
	}
}

/* Get the text contained in a textbox. Allocates memory in pText
 * to contain the text. Returns TRUE in case of success */
static BOOL
GetTextboxText(
	IN HWND hwndDlg,
	IN INT TextboxId,
	OUT LPWSTR *pText)
{
	LPWSTR Text;
	int Count;

	Count = GetWindowTextLength(GetDlgItem(hwndDlg, TextboxId));
	Text = HeapAlloc(GetProcessHeap(), 0, (Count + 1) * sizeof(WCHAR));
	if (!Text)
		return FALSE;
	if (Count != GetWindowTextW(GetDlgItem(hwndDlg, TextboxId), Text, Count + 1))
	{
		HeapFree(GetProcessHeap(), 0, Text);
		return FALSE;
	}
	*pText = Text;
	return TRUE;
}

static INT_PTR CALLBACK
LoggedOnWindowProc(
	IN HWND hwndDlg,
	IN UINT uMsg,
	IN WPARAM wParam,
	IN LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (uMsg)
	{
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDC_LOCK:
					EndDialog(hwndDlg, WLX_SAS_ACTION_LOCK_WKSTA);
					return TRUE;
				case IDC_LOGOFF:
					EndDialog(hwndDlg, WLX_SAS_ACTION_LOGOFF);
					return TRUE;
				case IDC_SHUTDOWN:
					EndDialog(hwndDlg, WLX_SAS_ACTION_SHUTDOWN_POWER_OFF);
					return TRUE;
				case IDC_TASKMGR:
					EndDialog(hwndDlg, WLX_SAS_ACTION_TASKLIST);
					return TRUE;
				case IDCANCEL:
					EndDialog(hwndDlg, WLX_SAS_ACTION_NONE);
					return TRUE;
			}
			break;
		}
		case WM_INITDIALOG:
		{
			SetFocus(GetDlgItem(hwndDlg, IDNO));
			return TRUE;
		}
		case WM_CLOSE:
		{
			EndDialog(hwndDlg, IDNO);
			return TRUE;
		}
	}

	return FALSE;
}

static INT
GUILoggedOnSAS(
	IN OUT PGINA_CONTEXT pgContext,
	IN DWORD dwSasType)
{
	INT result;

	TRACE("GUILoggedOnSAS()\n");

	if (dwSasType != WLX_SAS_TYPE_CTRL_ALT_DEL)
	{
		/* Nothing to do for WLX_SAS_TYPE_TIMEOUT ; the dialog will
		 * close itself thanks to the use of WlxDialogBoxParam */
		return WLX_SAS_ACTION_NONE;
	}

	result = pgContext->pWlxFuncs->WlxDialogBoxParam(
		pgContext->hWlx,
		pgContext->hDllInstance,
		MAKEINTRESOURCEW(IDD_LOGGEDON_DLG),
		GetDesktopWindow(),
		LoggedOnWindowProc,
		(LPARAM)pgContext);
	if (result >= WLX_SAS_ACTION_LOGON &&
	    result <= WLX_SAS_ACTION_SWITCH_CONSOLE)
	{
		return result;
	}
	return WLX_SAS_ACTION_NONE;
}

static INT_PTR CALLBACK
LoggedOutWindowProc(
	IN HWND hwndDlg,
	IN UINT uMsg,
	IN WPARAM wParam,
	IN LPARAM lParam)
{
	PGINA_CONTEXT pgContext;

	pgContext = (PGINA_CONTEXT)GetWindowLongPtr(hwndDlg, GWL_USERDATA);

	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			/* FIXME: take care of DontDisplayLastUserName, NoDomainUI, ShutdownWithoutLogon */
			pgContext = (PGINA_CONTEXT)lParam;
			SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)pgContext);
			SetFocus(GetDlgItem(hwndDlg, IDC_USERNAME));

			pgContext->hBitmap = LoadImage(hDllInstance, MAKEINTRESOURCE(IDI_ROSLOGO), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
			return TRUE;
		}
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc;
			if (pgContext->hBitmap)
			{
				hdc = BeginPaint(hwndDlg, &ps);
				DrawStateW(hdc, NULL, NULL, (LPARAM)pgContext->hBitmap, (WPARAM)0, 0, 0, 0, 0, DST_BITMAP);
				EndPaint(hwndDlg, &ps);
			}
			return TRUE;
		}
		case WM_DESTROY:
		{
			DeleteObject(pgContext->hBitmap);
			return TRUE;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDOK:
				{
					LPWSTR UserName = NULL, Password = NULL;
					INT result = WLX_SAS_ACTION_NONE;

					if (GetTextboxText(hwndDlg, IDC_USERNAME, &UserName) && *UserName == '\0')
						break;
					if (GetTextboxText(hwndDlg, IDC_PASSWORD, &Password) &&
					    DoLoginTasks(pgContext, UserName, NULL, Password))
					{
						result = WLX_SAS_ACTION_LOGON;
					}
					HeapFree(GetProcessHeap(), 0, UserName);
					HeapFree(GetProcessHeap(), 0, Password);
					EndDialog(hwndDlg, result);
					return TRUE;
				}
				case IDCANCEL:
				{
					EndDialog(hwndDlg, WLX_SAS_ACTION_NONE);
					return TRUE;
				}
				case IDC_SHUTDOWN:
				{
					EndDialog(hwndDlg, WLX_SAS_ACTION_SHUTDOWN);
					return TRUE;
				}
			}
			break;
		}
	}

	return FALSE;
}

static INT
GUILoggedOutSAS(
	IN OUT PGINA_CONTEXT pgContext)
{
	int result;

	TRACE("GUILoggedOutSAS()\n");

	result = pgContext->pWlxFuncs->WlxDialogBoxParam(
		pgContext->hWlx,
		pgContext->hDllInstance,
		MAKEINTRESOURCEW(IDD_LOGGEDOUT_DLG),
		GetDesktopWindow(),
		LoggedOutWindowProc,
		(LPARAM)pgContext);
	if (result >= WLX_SAS_ACTION_LOGON &&
	    result <= WLX_SAS_ACTION_SWITCH_CONSOLE)
	{
		WARN("WlxLoggedOutSAS() returns 0x%x\n", result);
		return result;
	}

	WARN("WlxDialogBoxParam() failed (0x%x)\n", result);
	return WLX_SAS_ACTION_NONE;
}

static INT
GUILockedSAS(
	IN OUT PGINA_CONTEXT pgContext)
{
	TRACE("GUILockedSAS()\n");

	UNREFERENCED_PARAMETER(pgContext);

	UNIMPLEMENTED;
	return WLX_SAS_ACTION_UNLOCK_WKSTA;
}

GINA_UI GinaGraphicalUI = {
	GUIInitialize,
	GUIDisplayStatusMessage,
	GUIRemoveStatusMessage,
	GUIDisplaySASNotice,
	GUILoggedOnSAS,
	GUILoggedOutSAS,
	GUILockedSAS,
};
