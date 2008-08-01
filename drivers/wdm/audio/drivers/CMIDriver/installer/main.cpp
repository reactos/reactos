/*
Copyright (c) 2006-2007 dogbert <dogber1@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "main.h"


void PrintLastError(LPCSTR function)
{
	LPVOID	lpMsgBuf;
	DWORD   errorid = GetLastError();

	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, errorid, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL );
	MessageBox(NULL, (LPCSTR)lpMsgBuf, function, MB_ICONEXCLAMATION | MB_OK);
	LocalFree(lpMsgBuf);
}

BOOL deleteCMIKeys()
{
	HKEY key;
	unsigned int i;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), NULL, KEY_SET_VALUE, &key) != ERROR_SUCCESS) {
		PrintLastError("RegOpenKeyEx()");
		return FALSE;
	}
	for (i=0;i<NumberOfCMIKeys;i++) {
		RegDeleteValue(key, CMIKeys[i]);
	}
	RegCloseKey(key);
	return TRUE;
}

BOOL CMIKeysExist()
{
	HKEY key;
	unsigned int i;
	BOOL result = FALSE;
	LONG size;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), NULL, KEY_SET_VALUE, &key) != ERROR_SUCCESS) {
		PrintLastError("RegOpenKeyEx()");
		return FALSE;
	}
	for (i=0;i<NumberOfCMIKeys;i++) {
		result |= (RegQueryValue(key, CMIKeys[i], NULL, &size) == ERROR_SUCCESS);
	}
	RegCloseKey(key);

	return result;
}

void writeUninstallerKeys()
{
	TCHAR SysDir[MAX_PATH];
	unsigned int len;
	HKEY key;

	if (GetSystemDirectory(SysDir, sizeof(SysDir))==0) {
		PrintLastError("GetSystemDirectory()");
		return;
	}
	len = strlen(SysDir);
	strcat(SysDir, Uninstaller);

	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\CMIDriver"), NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &key, NULL) != ERROR_SUCCESS) {
		PrintLastError("RegCreateKeyEx()");
		return;
	}
	RegSetValueEx(key, "DisplayName", NULL, REG_SZ, (BYTE*)&DisplayName, sizeof(DisplayName));
	RegSetValueEx(key, "URLInfoAbout", NULL, REG_SZ, (BYTE*)&URLInfoAbout, sizeof(URLInfoAbout));
	RegSetValueEx(key, "Publisher", NULL, REG_SZ, (BYTE*)&Publisher, sizeof(Publisher));
	RegSetValueEx(key, "UninstallString", NULL, REG_SZ, (BYTE*)&SysDir, strlen(SysDir));

	SysDir[len] = 0;
	strcat(SysDir, DisplayIcon);
	RegSetValueEx(key, "DisplayIcon", NULL, REG_SZ, (BYTE*)&SysDir, strlen(SysDir));

	RegCloseKey(key);
}

BOOL installDriver(HWND hWnd)
{
	TCHAR DriverPath[MAX_PATH];
	unsigned int i;

	EnableWindow(GetDlgItem(hWnd, IDB_INSTALL), FALSE);

	if (GetModuleFileName(NULL, DriverPath, MAX_PATH) == 0) {
		PrintLastError("DriverPackageInstall()");
		EnableWindow(GetDlgItem(hWnd, IDB_INSTALL), TRUE);
		return FALSE;
	}
	*_tcsrchr(DriverPath, _T('\\')) = _T('\0');
	*_tcscat(DriverPath, _T("\\CM8738.inf"));

	for (i=0;i<devArraySize;i++) {
		if (UpdateDriverForPlugAndPlayDevices(hWnd, devIDs[i], DriverPath, INSTALLFLAG_FORCE, NULL)) {
			EnableWindow(GetDlgItem(hWnd, IDB_INSTALL), TRUE);
			return TRUE;
		}
	}

	PrintLastError("DriverPackageInstall()");
	EnableWindow(GetDlgItem(hWnd, IDB_INSTALL), TRUE);
	return FALSE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg) {
		case WM_CLOSE:
			DestroyWindow(hWnd);
			return TRUE;
		case WM_DESTROY:
			PostQuitMessage(0);
			return TRUE;
		case WM_COMMAND:
			if (LOWORD(wParam) == IDB_CLOSE) {
				PostQuitMessage(0);
				return TRUE;
			}
			if (LOWORD(wParam) == IDB_INSTALL) {
				if (installDriver(hWnd)) {
					if (CMIKeysExist()) {
						if (MessageBox(hWnd, "The driver has been successfully installed! Do you want to remove the remains of the official C-Media driver?", "Driver Installer", MB_ICONINFORMATION | MB_YESNO) == IDYES) {
							deleteCMIKeys();
						}
					} else {
						MessageBox(hWnd, "The driver has been successfully installed!", "Driver Installer", MB_ICONINFORMATION);
					}
					writeUninstallerKeys();
					PostQuitMessage(0);
				}

				return TRUE;
			}
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	WNDCLASSEX wce;
	HWND       hWnd;
	MSG        msg;

	if (hWnd = FindWindow("cmiDriverInstaller", NULL)) {
		SetForegroundWindow(hWnd);
		return FALSE;
	}

	hInst = hInstance;
	ZeroMemory(&wce, sizeof(WNDCLASSEX));
	wce.cbSize        = sizeof(WNDCLASSEX);
	wce.lpfnWndProc   = DefDlgProc;
	wce.style         = 0;
	wce.cbWndExtra    = DLGWINDOWEXTRA;
	wce.hInstance     = hInstance;
	wce.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wce.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
	wce.lpszClassName = "cmiDriverInstaller";
	wce.lpszMenuName  = NULL;
	wce.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
	wce.hIconSm       = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
    if(!RegisterClassEx(&wce)) {
		PrintLastError("RegisterClassEx()");
		return -1;
	}

	hWnd = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_MAIN), NULL, (DLGPROC)WndProc, NULL);
	if (!hWnd) {
		PrintLastError("CreateDialogParam()");
		return -1;
	}

	while (GetMessage(&msg, (HWND) NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
    return 0;
}
