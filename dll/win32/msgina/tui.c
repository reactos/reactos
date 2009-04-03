/*
 * PROJECT:         ReactOS msgina.dll
 * FILE:            dll/win32/msgina/tui.c
 * PURPOSE:         ReactOS Logon GINA DLL
 * PROGRAMMER:      Hervé Poussineau (hpoussin@reactos.org)
 */

#include "msgina.h"

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(msgina);

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

	UNREFERENCED_PARAMETER(pgContext);
	UNREFERENCED_PARAMETER(hDesktop);
	UNREFERENCED_PARAMETER(dwOptions);
	UNREFERENCED_PARAMETER(pTitle);

	return
		WriteConsoleW(
			GetStdHandle(STD_OUTPUT_HANDLE),
			pMessage,
			wcslen(pMessage),
			&result,
			NULL) &&
		WriteConsoleW(
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
	UNREFERENCED_PARAMETER(pgContext);

	/* Nothing to do */
	return TRUE;
}

static BOOL
DisplayResourceText(
	IN UINT uIdResourceText,
	IN BOOL AddNewLine)
{
	WCHAR Prompt[256];
	static LPCWSTR newLine = L"\n";
	DWORD count;

	if (0 == LoadStringW(hDllInstance, uIdResourceText, Prompt, 256))
		return FALSE;
	if (!WriteConsoleW(
		GetStdHandle(STD_OUTPUT_HANDLE),
		Prompt, wcslen(Prompt),
		&count, NULL))
	{
		return FALSE;
	}
	if (AddNewLine)
	{
		if (!WriteConsoleW(
			GetStdHandle(STD_OUTPUT_HANDLE),
			newLine, wcslen(newLine),
			&count, NULL))
		{
			return FALSE;
		}
	}
	return TRUE;
}

static VOID
TUIDisplaySASNotice(
	IN OUT PGINA_CONTEXT pgContext)
{
	TRACE("TUIDisplaySASNotice()\n");

	UNREFERENCED_PARAMETER(pgContext);

	DisplayResourceText(IDS_LOGGEDOUTSAS, TRUE);
	DisplayResourceText(IDS_PRESSCTRLALTDELETE, TRUE);
}

static INT
TUILoggedOnSAS(
	IN OUT PGINA_CONTEXT pgContext,
	IN DWORD dwSasType)
{
	TRACE("TUILoggedOnSAS()\n");

	UNREFERENCED_PARAMETER(pgContext);

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
	IN UINT uIdResourcePrompt,
	IN OUT PWSTR Buffer,
	IN DWORD BufferLength,
	IN BOOL ShowString)
{
	DWORD count, i;
	WCHAR charToDisplay[] = { 0, UNICODE_NULL };

	if (!SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), 0))
		return FALSE;

	if (!FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE)))
		return FALSE;

	if (!DisplayResourceText(uIdResourcePrompt, FALSE))
		return FALSE;

	i = 0;
	for (;;)
	{
		WCHAR readChar;
		if (!ReadConsoleW(GetStdHandle(STD_INPUT_HANDLE), &readChar, 1, &count, NULL))
			return FALSE;
		if (readChar == '\r' || readChar == '\n')
		{
			/* End of string */
			charToDisplay[0] = L'\n';
			WriteConsoleW(
				GetStdHandle(STD_OUTPUT_HANDLE),
				charToDisplay,
				wcslen(charToDisplay),
				&count,
				NULL);
			break;
		}
		if (ShowString)
		{
			/* Display the char */
			charToDisplay[0] = readChar;
			WriteConsoleW(
				GetStdHandle(STD_OUTPUT_HANDLE),
				charToDisplay,
				wcslen(charToDisplay),
				&count,
				NULL);
		}
		Buffer[i++] = readChar;
		/* FIXME: buffer overflow if the user writes too many chars! */
		UNREFERENCED_PARAMETER(BufferLength);
		/* FIXME: handle backspace */
	}
	Buffer[i] = UNICODE_NULL;

	if (!ShowString)
	{
		/* Still display the \n */
		static LPCWSTR newLine = L"\n";
		DWORD result;
		WriteConsoleW(
			GetStdHandle(STD_OUTPUT_HANDLE),
			newLine,
			wcslen(newLine),
			&result,
			NULL);
	}
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
	if (!ReadString(IDS_ASKFORUSER, UserName, 256, TRUE))
		return WLX_SAS_ACTION_NONE;
	if (!ReadString(IDS_ASKFORPASSWORD, Password, 256, FALSE))
		return WLX_SAS_ACTION_NONE;

	if (DoLoginTasks(pgContext, UserName, NULL, Password))
		return WLX_SAS_ACTION_LOGON;
	else
		return WLX_SAS_ACTION_NONE;
}

static INT
TUILockedSAS(
	IN OUT PGINA_CONTEXT pgContext)
{
	HANDLE hToken;
	WCHAR UserName[256];
	WCHAR Password[256];

	TRACE("TUILockedSAS()\n");

	UNREFERENCED_PARAMETER(pgContext);

	if (!DisplayResourceText(IDS_LOGGEDOUTSAS, TRUE))
		return WLX_SAS_ACTION_UNLOCK_WKSTA;

	/* Ask the user for credentials */
	if (!ReadString(IDS_ASKFORUSER, UserName, 256, TRUE))
		return WLX_SAS_ACTION_NONE;
	if (!ReadString(IDS_ASKFORPASSWORD, Password, 256, FALSE))
		return WLX_SAS_ACTION_NONE;

	if (!LogonUserW(UserName, NULL, Password,
		LOGON32_LOGON_UNLOCK,
		LOGON32_PROVIDER_DEFAULT,
		&hToken))
	{
		TRACE("LogonUserW() failed\n");
		return WLX_SAS_ACTION_NONE;
	}
	CloseHandle(hToken);
	return WLX_SAS_ACTION_UNLOCK_WKSTA;
}

GINA_UI GinaTextUI = {
	TUIInitialize,
	TUIDisplayStatusMessage,
	TUIRemoveStatusMessage,
	TUIDisplaySASNotice,
	TUILoggedOnSAS,
	TUILoggedOutSAS,
	TUILockedSAS,
};
