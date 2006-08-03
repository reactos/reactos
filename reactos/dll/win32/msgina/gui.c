/*
 * PROJECT:         ReactOS msgina.dll
 * FILE:            dll/win32/msgina/gui.c
 * PURPOSE:         ReactOS Logon GINA DLL
 * PROGRAMMER:      Hervé Poussineau (hpoussin@reactos.org)
 */

#include "msgina.h"

#include <debug.h>
#define TRACE DbgPrint("(%s:%d) ", __FILE__, __LINE__), DbgPrint
#define FIXME DbgPrint("(%s:%d) ", __FILE__, __LINE__), DbgPrint
#define WARN DbgPrint("(%s:%d) ", __FILE__, __LINE__), DbgPrint
#undef DPRINT
#undef DPRINT1

static HBITMAP hBitmap = NULL;
static int cxSource, cySource;

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
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			PDISPLAYSTATUSMSG msg = (PDISPLAYSTATUSMSG)lParam;
			if (!msg)
				return FALSE;

			msg->Context->hStatusWindow = hwndDlg;

			if (msg->pTitle)
				SetWindowText(hwndDlg, msg->pTitle);
			SetDlgItemText(hwndDlg, IDC_STATUSLABEL, msg->pMessage);
			if (!msg->Context->SignaledStatusWindowCreated)
			{
				msg->Context->SignaledStatusWindowCreated = TRUE;
				SetEvent(msg->StartupEvent);
			}
			break;
		}
	}
	return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
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
		0,
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

		msg->StartupEvent = CreateEvent(
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
		SetWindowText(pgContext->hStatusWindow, pTitle);

	SetDlgItemText(pgContext->hStatusWindow, IDC_STATUSLABEL, pMessage);

	return TRUE;
}

static VOID
GUIDisplaySASNotice(
	IN OUT PGINA_CONTEXT pgContext)
{
	int result;

	TRACE("GUIDisplaySASNotice()\n");

	/* Display the notice window */
	result = pgContext->pWlxFuncs->WlxDialogBoxParam(
		pgContext->hWlx,
		pgContext->hDllInstance,
		MAKEINTRESOURCE(IDD_NOTICE_DLG),
		NULL,
		(DLGPROC)DefWindowProc,
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
	if (Count != GetWindowText(GetDlgItem(hwndDlg, TextboxId), Text, Count + 1))
	{
		HeapFree(GetProcessHeap(), 0, Text);
		return FALSE;
	}
	*pText = Text;
	return TRUE;
}

static INT_PTR CALLBACK
LoggedOutWindowProc(
	IN HWND hwndDlg,
	IN UINT uMsg,
	IN WPARAM wParam,
	IN LPARAM lParam)
{
	BITMAP bitmap;

	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			/* FIXME: take care of DontDisplayLastUserName, NoDomainUI, ShutdownWithoutLogon */
			SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)lParam);
			SetFocus(GetDlgItem(hwndDlg, IDC_USERNAME));

 			hBitmap = LoadImage(hDllInstance, MAKEINTRESOURCE(IDC_ROSLOGO), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
			if (hBitmap != NULL)
			{
				GetObject(hBitmap, sizeof(BITMAP), &bitmap);
				cxSource = bitmap.bmWidth;
				cySource = bitmap.bmHeight;
			}
			break;
		}
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc, hdcMem;
			hdc = BeginPaint(hwndDlg, &ps);
			hdcMem = CreateCompatibleDC(hdc);
			SelectObject(hdcMem, hBitmap);
			BitBlt(hdc, 0, 0, cxSource, cySource, hdcMem, 0, 0, SRCCOPY);
			DeleteDC(hdcMem);
			EndPaint(hwndDlg, &ps);
			break;
		}
		case WM_DESTROY:
		{
			DeleteObject(hBitmap);
			break;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDOK:
				{
					PGINA_CONTEXT pgContext;
					LPWSTR UserName = NULL, Password = NULL;
					INT result = WLX_SAS_ACTION_NONE;
					pgContext = (PGINA_CONTEXT)GetWindowLongPtr(hwndDlg, GWL_USERDATA);

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

	return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
}

static INT_PTR CALLBACK
LoggedOnWindowProc(
	IN HWND hwndDlg,
	IN UINT uMsg,
	IN WPARAM wParam,
	IN LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDYES:
				case IDNO:
				{
					EndDialog(hwndDlg, LOWORD(wParam));
					break;
				}
			}
			return TRUE;
		}
		case WM_INITDIALOG:
		{
			SetFocus(GetDlgItem(hwndDlg, IDNO));
			break;
		}
		case WM_CLOSE:
		{
			EndDialog(hwndDlg, IDNO);
			return TRUE;
		}
	}

	return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
}

static INT
GUILoggedOnSAS(
	IN OUT PGINA_CONTEXT pgContext,
	IN DWORD dwSasType)
{
	INT SasAction = WLX_SAS_ACTION_NONE;

	TRACE("GUILoggedOnSAS()\n");

	switch (dwSasType)
	{
		case WLX_SAS_TYPE_CTRL_ALT_DEL:
		{
			INT result;
			/* Display "Are you sure you want to log off?" dialog */
			result = pgContext->pWlxFuncs->WlxDialogBoxParam(
				pgContext->hWlx,
				pgContext->hDllInstance,
				MAKEINTRESOURCE(IDD_LOGGEDON_DLG),
				NULL,
				LoggedOnWindowProc,
				(LPARAM)pgContext);
			if (result == IDOK)
				SasAction = WLX_SAS_ACTION_LOCK_WKSTA;
			break;
		}
		case WLX_SAS_TYPE_SC_INSERT:
		{
			FIXME("WlxLoggedOnSAS: SasType WLX_SAS_TYPE_SC_INSERT not supported!\n");
			break;
		}
		case WLX_SAS_TYPE_SC_REMOVE:
		{
			FIXME("WlxLoggedOnSAS: SasType WLX_SAS_TYPE_SC_REMOVE not supported!\n");
			break;
		}
		case WLX_SAS_TYPE_TIMEOUT:
		{
			FIXME("WlxLoggedOnSAS: SasType WLX_SAS_TYPE_TIMEOUT not supported!\n");
			break;
		}
		default:
		{
			WARN("WlxLoggedOnSAS: Unknown SasType: 0x%x\n", dwSasType);
			break;
		}
	}

	return SasAction;
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
		MAKEINTRESOURCE(IDD_LOGGEDOUT_DLG),
		NULL,
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

GINA_UI GinaGraphicalUI = {
	GUIInitialize,
	GUIDisplayStatusMessage,
	GUIDisplaySASNotice,
	GUILoggedOnSAS,
	GUILoggedOutSAS,
};
