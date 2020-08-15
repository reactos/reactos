/*
 * PROJECT:         ReactOS Disk Cleanup
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Main file
 * COPYRIGHT:       Copyright 2020 Arnav Bhatt (arnavbhatt288 at gmail dot com)
 */
 
#include "resource.h"
#include "precomp.h"

DIRSIZE sz;
DLG_VAR dv;
WCHAR_VAR wcv;
BOOL_VAR bv;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPWSTR lpCmdLine, int nCmdShow)
{
	INITCOMMONCONTROLSEX InitControls;

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(nCmdShow);

	WCHAR err[256] = { 0 };
	HANDLE obj;

	INT_PTR dialogbuttonSelect;

	WCHAR sysDrive[MAX_PATH] = { 0 };

	LPWSTR* argList = NULL;
	int nArgs = 0;

	InitControls.dwSize = sizeof(INITCOMMONCONTROLSEX);
	InitControls.dwICC = ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES;
	if (!InitCommonControlsEx(&InitControls))
	{
		FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), err, 255, NULL);
		MessageBoxW(NULL, err, L"Ok", MB_OK);
		return FALSE;
	}	
	dv.hInst = hInstance;
	
	obj = CreateMutexW(NULL, FALSE, L"pMutex");

	if (obj)
	{
		DWORD err = GetLastError();

		if (err == ERROR_ALREADY_EXISTS)
		{
			MessageBoxW(NULL, L"Program is already running!", L"Error", MB_OK | MB_ICONSTOP);
			CloseHandle(obj);
			return TRUE;
		}
	}

	argList = CommandLineToArgvW(GetCommandLineW(), &nArgs);

	if (argList == NULL)
	{
		return FALSE;
	}

	else if (nArgs > 1)
	{
		if(!argCheck(argList, nArgs))
		{
			return FALSE;
		}
	}

	else
	{
		dialogbuttonSelect = DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_START), NULL, StartDlgProc, 0);

		if (dialogbuttonSelect == IDCANCEL)
		{
			return TRUE;
		}
	}

	GetEnvironmentVariableW(L"SystemDrive", sysDrive, _countof(sysDrive));
	
	if(wcscmp(sysDrive, wcv.driveLetter) == 0)
	{
		bv.sysDrive = TRUE;
	}
	
	Sleep(150);
	
	dialogbuttonSelect = DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_PROGRESS), NULL, ProgressDlgProc, 0);

	if(dialogbuttonSelect == IDCANCEL)
	{
		return TRUE;
	}

	dialogbuttonSelect = DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_CHOICE), NULL, ChoiceDlgProc, 0);

	if(dialogbuttonSelect == IDCANCEL)
	{
		return TRUE;
	}

	DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_PROGRESS_END), NULL, ProgressEndDlgProc, 0);
	
	if(obj)
	{
		CloseHandle(obj);
	}

	return TRUE;
}
