/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            services/winlogon/screensaver.c
 * PURPOSE:         Screen saver management
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */

#include "winlogon.h"

#define YDEBUG
#include <wine/debug.h>

static LRESULT CALLBACK
KeyboardActivityProc(
	IN INT nCode,
	IN WPARAM wParam,
	IN LPARAM lParam)
{
	InterlockedExchange((LONG*)&WLSession->LastActivity, ((PKBDLLHOOKSTRUCT)lParam)->time);
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

static LRESULT CALLBACK
MouseActivityProc(
	IN INT nCode,
	IN WPARAM wParam,
	IN LPARAM lParam)
{
	InterlockedExchange((LONG*)&WLSession->LastActivity, ((PMSLLHOOKSTRUCT)lParam)->time);
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

static VOID
LoadScreenSaverParameters(
	OUT LPDWORD Timeout)
{
	BOOL Enabled;

	if (!SystemParametersInfoW(SPI_GETSCREENSAVETIMEOUT, 0, Timeout, 0))
	{
		WARN("WL: Unable to get screen saver timeout (error %lu). Disabling it\n", GetLastError());
		*Timeout = INFINITE;
	}
	else if (!SystemParametersInfoW(SPI_GETSCREENSAVEACTIVE, 0, &Enabled, 0))
	{
		WARN("WL: Unable to check if screen saver is enabled (error %lu). Disabling it\n", GetLastError());
		*Timeout = INFINITE;
	}
	else if (!Enabled)
	{
		TRACE("WL: Screen saver is disabled\n");
		*Timeout = INFINITE;
	}
	else
	{
		TRACE("WL: Screen saver timeout: %lu seconds\n", *Timeout);
		*Timeout *= 1000;
	}
}

static DWORD WINAPI
ScreenSaverThreadMain(
	IN LPVOID lpParameter)
{
	PWLSESSION Session = (PWLSESSION)lpParameter;
	HANDLE HandleArray[3];
	DWORD LastActivity, TimeToWait;
	DWORD Timeout; /* Timeout before screen saver starts, in milliseconds */
	DWORD ret;

	if (!ImpersonateLoggedOnUser(Session->UserToken))
	{
		ERR("ImpersonateLoggedOnUser() failed with error %lu\n", GetLastError());
		return 0;
	}

	Session->hUserActivity = CreateEventW(NULL, FALSE, FALSE, NULL);
	if (!Session->hUserActivity)
	{
		ERR("WL: Unable to create event (error %lu)\n", GetLastError());
		goto cleanup;
	}

	HandleArray[0] = Session->hEndOfScreenSaverThread;
	HandleArray[1] = Session->hScreenSaverParametersChanged;
	HandleArray[2] = Session->hUserActivity;

	LoadScreenSaverParameters(&Timeout);

	InterlockedExchange((LONG*)&Session->LastActivity, GetTickCount());
	for (;;)
	{
		/* See the time of last activity and calculate a timeout */
		LastActivity = InterlockedCompareExchange((LONG*)&Session->LastActivity, 0, 0);
		TimeToWait = Timeout - (GetTickCount() - LastActivity);
		if (TimeToWait > Timeout)
		{
			/* GetTickCount() got back to 0 */
			TimeToWait = Timeout;
		}

		/* Wait for the timeout, or the end of this thread */
		ret = WaitForMultipleObjects(2, HandleArray, FALSE, TimeToWait);
		if (ret == WAIT_OBJECT_0)
			break;
		else if (ret == WAIT_OBJECT_0 + 1)
			LoadScreenSaverParameters(&Timeout);

		/* Check if we didn't had recent activity */
		LastActivity = InterlockedCompareExchange((LONG*)&Session->LastActivity, 0, 0);
		if (LastActivity + Timeout > GetTickCount())
			continue;

		/* Run screen saver */
		PostMessageW(Session->SASWindow, WLX_WM_SAS, WLX_SAS_TYPE_SCRNSVR_TIMEOUT, 0);

		/* Wait for the end of this thread or of the screen saver */
		ret = WaitForMultipleObjects(3, HandleArray, FALSE, INFINITE);
		if (ret == WAIT_OBJECT_0)
			break;
		else if (ret == WAIT_OBJECT_0 + 1)
			LoadScreenSaverParameters(&Timeout);
		else if (ret == WAIT_OBJECT_0 + 2)
			SystemParametersInfoW(SPI_SETSCREENSAVERRUNNING, FALSE, NULL, 0);
	}

cleanup:
	RevertToSelf();
	if (Session->hUserActivity)
		CloseHandle(Session->hUserActivity);
	if (Session->KeyboardHook)
		UnhookWindowsHookEx(Session->KeyboardHook);
	if (Session->MouseHook)
		UnhookWindowsHookEx(Session->MouseHook);
	CloseHandle(Session->hEndOfScreenSaverThread);
	CloseHandle(Session->hScreenSaverParametersChanged);
	return 0;
}

BOOL
InitializeScreenSaver(
	IN OUT PWLSESSION Session)
{
	HANDLE ScreenSaverThread;

	FIXME("Disabling screen saver due to numerous bugs in ReactOS (see r23540)!\n");
	return TRUE;

	/* Register hooks to detect keyboard and mouse activity */
	Session->KeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardActivityProc, hAppInstance, 0);
	if (!Session->KeyboardHook)
	{
		ERR("WL: Unable to register keyboard hook\n");
		return FALSE;
	}
	Session->MouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseActivityProc, hAppInstance, 0);
	if (!Session->MouseHook)
	{
		ERR("WL: Unable to register mouse hook\n");
		return FALSE;
	}

	if (!(Session->hScreenSaverParametersChanged = CreateEventW(NULL, FALSE, FALSE, NULL)))
	{
		WARN("WL: Unable to create screen saver event (error %lu)\n", GetLastError());
	}
	else if (!(Session->hEndOfScreenSaverThread = CreateEventW(NULL, FALSE, FALSE, NULL)))
	{
		WARN("WL: Unable to create screen saver event (error %lu)\n", GetLastError());
		CloseHandle(Session->hScreenSaverParametersChanged);
	}
	else
	{
		ScreenSaverThread = CreateThread(
			NULL,
			0,
			ScreenSaverThreadMain,
			Session,
			0,
			NULL);
		if (ScreenSaverThread)
			CloseHandle(ScreenSaverThread);
		else
			WARN("WL: Unable to start screen saver thread\n");
	}

	return TRUE;
}

VOID
StartScreenSaver(
	IN PWLSESSION Session)
{
	HKEY hKey = NULL;
	WCHAR szApplicationName[MAX_PATH];
	WCHAR szCommandLine[MAX_PATH + 3];
	DWORD bufferSize = sizeof(szApplicationName)- 1;
	DWORD dwType;
	STARTUPINFOW StartupInfo;
	PROCESS_INFORMATION ProcessInformation;
	LONG rc;
	BOOL ret = FALSE;

	if (!ImpersonateLoggedOnUser(Session->UserToken))
	{
		ERR("ImpersonateLoggedOnUser() failed with error %lu\n", GetLastError());
		goto cleanup;
	}

	rc = RegOpenKeyExW(
		HKEY_CURRENT_USER,
		L"Control Panel\\Desktop",
		0,
		KEY_QUERY_VALUE,
		&hKey);
	if (rc != ERROR_SUCCESS)
		goto cleanup;

	szApplicationName[bufferSize] = 0; /* Terminate the string */
	rc = RegQueryValueExW(
		hKey,
		L"SCRNSAVE.EXE",
		0,
		&dwType,
		(LPBYTE)szApplicationName,
		&bufferSize);
	if (rc != ERROR_SUCCESS || dwType != REG_SZ)
		goto cleanup;

	wsprintfW(szCommandLine, L"%s /s", szApplicationName);
	TRACE("WL: Executing %S\n", szCommandLine);

	ZeroMemory(&StartupInfo, sizeof(STARTUPINFOW));
	ZeroMemory(&ProcessInformation, sizeof(PROCESS_INFORMATION));
	StartupInfo.cb = sizeof(STARTUPINFOW);
	/* FIXME: run the screen saver on the screen saver desktop */
	ret = CreateProcessW(
		szApplicationName,
		szCommandLine,
		NULL,
		NULL,
		FALSE,
		0,
		NULL,
		NULL,
		&StartupInfo,
		&ProcessInformation);
	if (!ret)
	{
		WARN("WL: Unable to start %S, error %lu\n", szApplicationName, GetLastError());
		goto cleanup;
	}

	CloseHandle(ProcessInformation.hProcess);
	CloseHandle(ProcessInformation.hThread);

cleanup:
	RevertToSelf();
	if (hKey)
		RegCloseKey(hKey);
	if (ret)
		SystemParametersInfoW(SPI_SETSCREENSAVERRUNNING, TRUE, NULL, 0);
	else
	{
		PostMessageW(Session->SASWindow, WLX_WM_SAS, WLX_SAS_TYPE_SCRNSVR_ACTIVITY, 0);
		InterlockedExchange((LONG*)&Session->LastActivity, GetTickCount());
	}
}
