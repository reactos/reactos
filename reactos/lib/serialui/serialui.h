/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS SerialUI DLL
 * FILE:        serialui.h
 * PURPOSE:     header file
 * PROGRAMMERS: Saveliy Tretiakov (saveliyt@mail.ru)
 */

#include <windows.h>
#include <shlwapi.h>
#include "resource.h"

#define UNIMPLEMENTED \
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED); \
  return FALSE;

#define DEFAULT_BAUD_INDEX 6
#define DEFAULT_BYTESIZE_INDEX 3
#define DEFAULT_PARITY_INDEX 2
#define DEFAULT_STOPBITS_INDEX 0

typedef struct _DIALOG_INFO
{
	LPCWSTR lpszDevice;
	UINT InitialFlowIndex;
	LPCOMMCONFIG lpCC;
} DIALOG_INFO, *LPDIALOG_INFO;


/************************************
 *
 *  EXPORTS
 *
 ************************************/

BOOL WINAPI drvCommConfigDialogW(LPCWSTR lpszDevice,
	HWND hWnd,
	LPCOMMCONFIG lpCommConfig);

BOOL WINAPI drvCommConfigDialogA(LPCSTR lpszDevice,
	HWND hWnd,
	LPCOMMCONFIG lpCommConfig);

BOOL WINAPI drvSetDefaultCommConfigW(LPCWSTR lpszDevice,
	LPCOMMCONFIG lpCommConfig,
	DWORD dwSize);

BOOL WINAPI drvSetDefaultCommConfigA(LPCSTR lpszDevice,
	LPCOMMCONFIG lpCommConfig,
	DWORD dwSize);

BOOL WINAPI drvGetDefaultCommConfigW(LPCWSTR lpszDevice,
	LPCOMMCONFIG lpCommConfig,
	LPDWORD lpdwSize);

BOOL WINAPI drvGetDefaultCommConfigA(LPCSTR lpszDevice,
	LPCOMMCONFIG lpCommConfig,
	LPDWORD lpdwSize);


/************************************
 *
 *  INTERNALS
 *
 ************************************/

LRESULT CommDlgProc(HWND hDlg,
	UINT Msg,
	WPARAM wParam,
	LPARAM lParam);

VOID OkButton(HWND hDlg);


