/*

	compile via:
	gcc -o create-links -D_WIN32_IE=0x400 create-links.c -lole32 -luuid -lshell32 -lshlwapi

	Martin Fuchs, 27.12.2003

*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <shlobj.h>
#include <objidl.h>
#include <shlwapi.h>

#include <stdio.h>

HRESULT CreateShellLink(LPCSTR linkPath, LPCSTR cmd, LPCSTR arg, LPCSTR dir, LPCSTR iconPath, int icon_nr, LPCSTR comment)
{
	IShellLinkA* psl;
	IPersistFile* ppf;
	WCHAR buffer[MAX_PATH];

	HRESULT hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLink, (LPVOID*)&psl);

	printf("creating shortcut file '%s' to %s...\n", linkPath, cmd);

	if (SUCCEEDED(hr)) {
		hr = psl->lpVtbl->SetPath(psl, cmd);

		if (arg)
			hr = psl->lpVtbl->SetArguments(psl, arg);

		if (dir)
			hr = psl->lpVtbl->SetWorkingDirectory(psl, dir);

		if (iconPath)
			hr = psl->lpVtbl->SetIconLocation(psl, iconPath, icon_nr);

		if (comment)
			hr = psl->lpVtbl->SetDescription(psl, comment);

		hr = psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, (LPVOID*)&ppf);

		if (SUCCEEDED(hr)) {
			MultiByteToWideChar(CP_ACP, 0, linkPath, -1, buffer, MAX_PATH);

			hr = ppf->lpVtbl->Save(ppf, buffer, TRUE);

			ppf->lpVtbl->Release(ppf);
		}

		psl->lpVtbl->Release(psl);
	}

	if (SUCCEEDED(hr))
		printf("OK\n\n");
	else
		printf("error %08x\n\n", (int) hr);

	return hr;
}


int main()
{
	char path[MAX_PATH];
	LPSTR p;

	CoInitialize(NULL);

	/* create some shortcuts in the start menu "programs" folder */
	SHGetSpecialFolderPathA(0, path, CSIDL_PROGRAMS, TRUE);
	p = PathAddBackslash(path);

	strcpy(p, "start-cmd.lnk");
	CreateShellLink(path, "cmd.exe", "", NULL, NULL, 0, "open console window");

	strcpy(p, "start-winhello.lnk");
	CreateShellLink(path, "winhello.exe", "", NULL, NULL, 0, "launch winhello");


	/* create some shortcuts on the desktop */
	SHGetSpecialFolderPathA(0, path, CSIDL_DESKTOP, TRUE);
	p = PathAddBackslash(path);

	strcpy(p, "start-wcmd.lnk");
	CreateShellLink(path, "cmd.exe", "", NULL, NULL, 0, "open console window");

	strcpy(p, "start-winemine.lnk");
	CreateShellLink(path, "winemine.exe", "", NULL, NULL, 0, "launch winemine");

	CoUninitialize();

	return 0;
}
