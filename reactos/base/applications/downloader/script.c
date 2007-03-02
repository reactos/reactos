/* PROJECT:         ReactOS Downloader
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/downloader/script.c
 * PURPOSE:         Run (un/)installscript
 * PROGRAMMERS:     Lester Kortenhoeven
 */

#include <windows.h>

#include "resources.h"
#include "structures.h"

extern BOOL getUninstaller(struct Application*, WCHAR*);
extern INT_PTR CALLBACK DownloadProc (HWND, UINT, WPARAM, LPARAM);
extern WCHAR Strings [STRING_COUNT][MAX_STRING_LENGHT];

static void DownloadScriptFunc (WCHAR* URL, WCHAR* File) {
	struct lParamDownload* lParam;
	lParam = malloc(sizeof(struct lParamDownload));
	lParam->URL = URL;
	lParam->File = File;
	DialogBoxParamW(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDD_DOWNLOAD), 0, DownloadProc, (LPARAM)lParam);
	free(lParam);
}

static void ExecScriptFunc(WCHAR* Arg) {
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;

	memset(&si, 0, sizeof(si));
	si.cb=sizeof(si);
	CreateProcessW(NULL,Arg,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi);
	CloseHandle(pi.hThread);
	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
}


static void DelScriptFunc(WCHAR* Arg) {
	DeleteFileW(Arg);
}

static BOOL UnzipScriptFunc(WCHAR* File, WCHAR* Outdir) {
	HKEY hKey;
	DWORD Type = 0;
	WCHAR ExecStr[0x100];
	DWORD currentlengt = 0x100;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,L"SOFTWARE\\7-Zip",0,KEY_READ,&hKey) == ERROR_SUCCESS) {
		if (RegQueryValueExW(hKey,L"Path",0,&Type,(LPBYTE)ExecStr,&currentlengt) == ERROR_SUCCESS) {
			if (File[0] != L'\0') {
				wcsncat(ExecStr,L"\\7z.exe x ",0x100-currentlengt);
				currentlengt = lstrlenW(ExecStr);
				wcsncat(ExecStr,File,0x100-currentlengt);
				currentlengt = lstrlenW(ExecStr);
				wcsncat(ExecStr,L" -o",0x100-currentlengt);
				currentlengt = lstrlenW(ExecStr);
				wcsncat(ExecStr,Outdir,0x100-currentlengt);
				ExecScriptFunc(ExecStr);
				RegCloseKey(hKey);
			}
			return TRUE;
		}
		RegCloseKey(hKey);
	}
	MessageBoxW(0,Strings[IDS_UNZIP_ERROR],0,0);
	return FALSE;
}

static void AddUninstallerScriptFunc(WCHAR* RegName, WCHAR* File) {
	HKEY hKey1;
	HKEY hKey2;
	LPDWORD dispos = NULL;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",0,KEY_WRITE,&hKey1) == ERROR_SUCCESS)
		if (RegCreateKeyEx(hKey1,RegName,0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey2,dispos) == ERROR_SUCCESS) {
			RegSetValueExW(hKey2,L"DisplayName",0,REG_SZ,(BYTE*)RegName,(lstrlen(RegName)+1)*sizeof(WCHAR));
			RegSetValueExW(hKey2,L"UninstallString",0,REG_SZ,(BYTE*)File,(lstrlen(File)+1)*sizeof(WCHAR));
		}
	RegCloseKey(hKey2);
	RegCloseKey(hKey1);
}

static void RemoveUninstallerScriptFunc(WCHAR* RegName) {
	HKEY hKey1;
	HKEY hKey2;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",0,KEY_WRITE,&hKey1) == ERROR_SUCCESS) {
		if (RegOpenKeyExW(hKey1,RegName,0,KEY_WRITE,&hKey2) == ERROR_SUCCESS) {
			RegDeleteValueW(hKey2,L"DisplayName");
			RegDeleteValueW(hKey2,L"UninstallString");
			RegCloseKey(hKey2);
		}
		// RegDeleteKeyW(hKey1,RegName);
	}
	RegCloseKey(hKey1);
}

static void MessageScriptFunc(WCHAR* Text) {
	MessageBoxW(0,Text,Strings[IDS_WINDOW_TITLE],0);
}

extern void LoadScriptFunc(WCHAR*, struct ScriptElement*);

static void RunScript(struct Application* App, struct ScriptElement* Script) {
	BOOL bRun = TRUE;
	struct ScriptElement* p = Script;
	INT SizeB = 0x100;
	INT SizeA = sizeof(p->Arg)/sizeof(*(p->Arg));
	INT i;
	int currentlengt = 0;
	WCHAR ArgBuffer[SizeA][SizeB];
	WCHAR BufferA[SizeB];
	WCHAR BufferB[SizeB];
	WCHAR BufferC[SizeB];
	WCHAR* Pos1;
	WCHAR* Pos2;
	WCHAR* Pos3 = NULL;
	BOOL bNext;
	while(bRun && (p != NULL)) {

		for(i=0; i<SizeA; i++) {
			bNext = TRUE;
			wcscpy(BufferA, p->Arg[i]);
			Pos1 = BufferA;
			Pos2 = wcschr(Pos1, L'%');
			if(!Pos2) {
				wcscpy(ArgBuffer[i], Pos1);
				break;
			}
			Pos2[0] = L'\0';
			wcscpy(BufferB, Pos1);
			Pos1 = Pos2 + 1;
			Pos2 = wcschr(Pos1, L'%');
			while (Pos2) {
				Pos2[0] = L'\0';
				if(bNext) {
					if (wcscmp(Pos1, L"name") == 0) {
						Pos3 = App->Name;
					} else if (wcscmp(Pos1, L"regname") == 0) {
						Pos3 = App->RegName;
					} else if (wcscmp(Pos1, L"version") == 0) {
						Pos3 = App->Version;
					} else if (wcscmp(Pos1, L"maintainer") == 0) {
						Pos3 = App->Maintainer;
					} else if (wcscmp(Pos1, L"licence") == 0) {
						Pos3 = App->Licence;
					} else if (wcscmp(Pos1, L"description") == 0) {
						Pos3 = App->Description;
					} else if (wcscmp(Pos1, L"location") == 0) {
						Pos3 = App->Location;
					} else if (wcscmp(Pos1, L"regname_uninstaller") == 0) {
						if (!getUninstaller(App, BufferC)) {
							BufferC[0] = '\0';
						}
						Pos3 = BufferC;
					} else if (wcscmp(Pos1, L"location_file") == 0) {
						Pos3 = wcsrchr(App->Location, L'/');
						if(Pos3 == NULL) {
							BufferC[0] = '\0';
							Pos3 = BufferC;
						} else {
							Pos3++;
						}
					} else {
						Pos3 = _wgetenv(Pos1);
					}
					bNext = !(Pos3);
					if (bNext) {
						Pos3 = Pos1;
						currentlengt = lstrlenW(BufferB);
						wcsncat(BufferB, L"%", SizeB-currentlengt);
					}
				} else {
					Pos3 = Pos1;
					bNext = TRUE;
				}
				currentlengt = lstrlenW(BufferB);
				wcsncat(BufferB, Pos3, SizeB-currentlengt);
				Pos1 = Pos2 + 1;
				Pos2 = wcschr(Pos1, L'%');
			}
			if (bNext) {
				wcsncat(BufferB, L"%", SizeB-currentlengt);
			}
			currentlengt = lstrlenW(BufferB);
			wcsncat(BufferB, Pos1, SizeB-currentlengt);
			wcscpy(ArgBuffer[i], BufferB);
		}

		if (wcscmp(p->Func, L"download") == 0) {
			DownloadScriptFunc(ArgBuffer[0], ArgBuffer[1]);
		} else if (wcscmp(p->Func, L"exec") == 0) {
			ExecScriptFunc(ArgBuffer[0]);
		} else if (wcscmp(p->Func, L"del") == 0) {
			DelScriptFunc(ArgBuffer[0]);
		} else if (wcscmp(p->Func, L"unzip") == 0) {
			bRun = UnzipScriptFunc(ArgBuffer[0], ArgBuffer[1]);
		} else if (wcscmp(p->Func, L"adduninstaller") == 0) {
			AddUninstallerScriptFunc(ArgBuffer[0], ArgBuffer[1]);
		} else if (wcscmp(p->Func, L"removeuninstaller") == 0) {
			RemoveUninstallerScriptFunc(ArgBuffer[0]);
		} else if (wcscmp(p->Func, L"message") == 0) {
			MessageScriptFunc(ArgBuffer[0]);
		} else if (wcscmp(p->Func, L"load") == 0) {
			LoadScriptFunc(ArgBuffer[0],p);
		}
		p = p->Next;
	}
}

DWORD WINAPI InstallThreadFunc(LPVOID Context) {
	struct Application* App = (struct Application*)Context;

	if(App->InstallScript == NULL){
		/* Default UninstallScript */
		struct ScriptElement* Current;
		Current = malloc(sizeof(struct ScriptElement));
		App->InstallScript = Current;
		memset(Current, 0, sizeof(struct ScriptElement));
		wcscpy(Current->Func, L"load");
		wcscpy(Current->Arg[0], L"script/default.install.xml");
	}

	RunScript(App, App->InstallScript);

	return 0;
}



DWORD WINAPI UninstallThreadFunc(LPVOID Context){
	struct Application* App = (struct Application*)Context;

	if(App->UninstallScript == NULL){
		/* Default UninstallScript */
		struct ScriptElement* Current;
		Current = malloc(sizeof(struct ScriptElement));
		App->UninstallScript = Current;
		memset(Current, 0, sizeof(struct ScriptElement));
		wcscpy(Current->Func, L"load");
		wcscpy(Current->Arg[0], L"script/default.uninstall.xml");
	}

	RunScript(App, App->UninstallScript);

	return 0;
}

