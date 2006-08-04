/*
 * PROJECT:         ReactOS msgina.dll
 * FILE:            dll/win32/msgina/tui.c
 * PURPOSE:         ReactOS Logon GINA DLL
 * PROGRAMMER:      Hervé Poussineau (hpoussin@reactos.org)
 */

#include "msgina.h"

#define YDEBUG
#include <wine/debug.h>

static BOOL
TUIInitialize(
	IN OUT PGINA_CONTEXT pgContext)
{
	TRACE("TUIInitialize(%p)\n", pgContext);

	return AllocConsole();
}

static BOOL
TUIDisplayStatusMessage(
	IN PGINA_CONTEXT pgContext,
	IN HDESK hDesktop,
	IN DWORD dwOptions,
	IN PWSTR pTitle,
	IN PWSTR pMessage)
{
	static LPCWSTR newLine = L"\n";
	DWORD result;

	TRACE("TUIDisplayStatusMessage(%ws)\n", pMessage);

	return
		WriteConsole(
			GetStdHandle(STD_OUTPUT_HANDLE),
			pMessage,
			wcslen(pMessage),
			&result,
			NULL) &&
		WriteConsole(
			GetStdHandle(STD_OUTPUT_HANDLE),
			newLine,
			wcslen(newLine),
			&result,
			NULL);
}

static BOOL
TUIRemoveStatusMessage(
	IN PGINA_CONTEXT pgContext)
{
	/* Nothing to do */
	return TRUE;
}

static VOID
TUIDisplaySASNotice(
	IN OUT PGINA_CONTEXT pgContext)
{
	WCHAR CtrlAltDelPrompt[256];
	DWORD count;

	TRACE("TUIDisplaySASNotice()\n");

	if (LoadString(hDllInstance, IDS_PRESSCTRLALTDELETE, CtrlAltDelPrompt, 256))
	{
		WriteConsole(
			GetStdHandle(STD_OUTPUT_HANDLE),
			CtrlAltDelPrompt,
			wcslen(CtrlAltDelPrompt),
			&count,
			NULL);
	}
}

static INT
TUILoggedOnSAS(
	IN OUT PGINA_CONTEXT pgContext,
	IN DWORD dwSasType)
{
	TRACE("TUILoggedOnSAS()\n");

	if (dwSasType != WLX_SAS_TYPE_CTRL_ALT_DEL)
	{
		/* Nothing to do for WLX_SAS_TYPE_TIMEOUT */
		return WLX_SAS_ACTION_NONE;
	}

	FIXME("FIXME: TUILoggedOnSAS(): Let's suppose the user wants to log off...\n");
	return WLX_SAS_ACTION_LOGOFF;
}

static BOOL
ReadString(
	IN PGINA_CONTEXT pgContext,
	IN UINT uIdResourcePrompt,
	IN OUT PWSTR Buffer,
	IN DWORD BufferLength,
	IN BOOL ShowString)
{
	WCHAR Prompt[256];
	DWORD count, i;

	if (!SetConsoleMode(
		GetStdHandle(STD_INPUT_HANDLE),
		ShowString ? ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT: ENABLE_LINE_INPUT))
	{
		return FALSE;
	}

	if (!LoadString(hDllInstance, uIdResourcePrompt, Prompt, 256))
		return FALSE;

	if (!WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), Prompt, wcslen(Prompt), &count, NULL))
		return FALSE;

	i = 0;
	do
	{
		if (!ReadConsole(GetStdHandle(STD_INPUT_HANDLE), &Buffer[i], 1, &count, NULL))
			return FALSE;
		i++;
		/* FIXME: buffer overflow if the user writes too many chars! */
		/* FIXME: handle backspace */
	} while (Buffer[i - 1] != '\n');
	Buffer[i - 1] = 0;
	return TRUE;
}

static INT
TUILoggedOutSAS(
	IN OUT PGINA_CONTEXT pgContext)
{
	WCHAR UserName[256];
	WCHAR Password[256];

	TRACE("TUILoggedOutSAS()\n");

	/* Ask the user for credentials */
	if (!ReadString(pgContext, IDS_ASKFORUSER, UserName, 256, TRUE))
		return WLX_SAS_ACTION_NONE;
	if (!ReadString(pgContext, IDS_ASKFORPASSWORD, Password, 256, FALSE))
		return WLX_SAS_ACTION_NONE;

	if (DoLoginTasks(pgContext, UserName, NULL, Password))
		return WLX_SAS_ACTION_LOGON;
	else
		return WLX_SAS_ACTION_NONE;
}

GINA_UI GinaTextUI = {
	TUIInitialize,
	TUIDisplayStatusMessage,
	TUIRemoveStatusMessage,
	TUIDisplaySASNotice,
	TUILoggedOnSAS,
	TUILoggedOutSAS,
};
