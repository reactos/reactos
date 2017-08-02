#pragma once

#include <windef.h>
#include <atlstr.h>

int GetWindowWidth(HWND hwnd);
int GetWindowHeight(HWND hwnd);
int GetClientWindowWidth(HWND hwnd);
int GetClientWindowHeight(HWND hwnd);

VOID CopyTextToClipboard(LPCWSTR lpszText);
VOID SetWelcomeText(VOID);
VOID ShowPopupMenu(HWND hwnd, UINT MenuID, UINT DefaultItem);
BOOL StartProcess(ATL::CStringW &Path, BOOL Wait);
BOOL StartProcess(LPWSTR lpPath, BOOL Wait);
BOOL GetStorageDirectory(ATL::CStringW &lpDirectory);
BOOL ExtractFilesFromCab(LPCWSTR lpCabName, LPCWSTR lpOutputPath);
VOID InitLogs(VOID);
VOID FreeLogs(VOID);
BOOL WriteLogMessage(WORD wType, DWORD dwEventID, LPCWSTR lpMsg);
BOOL GetInstalledVersion(ATL::CStringW *pszVersion, const ATL::CStringW &szRegName);
