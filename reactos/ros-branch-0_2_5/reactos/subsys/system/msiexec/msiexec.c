/*
 * msiexec.exe implementation
 *
 * Copyright 2004 Vincent Béron
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <windows.h>
#include <msi.h>
#include <objbase.h>
#include <stdio.h>

#include "msiexec.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msiexec);

static const char UsageStr[] =
"Usage:\n"
"  Install a product:\n"
"    msiexec {package|productcode} [property]\n"
"    msiexec /i {package|productcode} [property]\n"
"    msiexec /a package [property]\n"
"  Repair an installation:\n"
"    msiexec /f[p|o|e|d|c|a|u|m|s|v] {package|productcode}\n"
"  Uninstall a product:\n"
"    msiexec /x {package|productcode} [property]\n"
"  Advertise a product:\n"
"    msiexec /j[u|m] package [/t transform] [/g languageid]\n"
"    msiexec {u|m} package [/t transform] [/g languageid]\n"
"  Apply a patch:\n"
"    msiexec /p patchpackage [property]\n"
"    msiexec /p patchpackage /a package [property]\n"
"  Modifiers for above operations:\n"
"    msiexec /l[*][i|w|e|a|r|u|c|m|o|p|v|][+|!] logfile\n"
"    msiexec /q{|n|b|r|f|n+|b+|b-}\n"
"  Register a module:\n"
"    msiexec /y module\n"
"  Unregister a module:\n"
"    msiexec /z module\n"
"  Display usage and copyright:\n"
"    msiexec {/h|/?}\n"
"NOTE: Product code on commandline unimplemented as of yet\n"
"\n"
"Copyright 2004 Vincent Béron\n";

static const char ActionAdmin[] = "ACTION=ADMIN ";
static const char RemoveAll[] = "REMOVE=ALL ";

static void ShowUsage(int ExitCode)
{
	printf(UsageStr);
	ExitProcess(ExitCode);
}

static BOOL GetProductCode(LPCSTR str, LPCSTR *PackageName, LPGUID *ProductCode)
{
	BOOL ret = FALSE;
	int len = 0;
	LPWSTR wstr = NULL;

	if(strlen(str) == 38)
	{
		len = MultiByteToWideChar(CP_ACP, 0, str, -1, wstr, 0);
		wstr = HeapAlloc(GetProcessHeap(), 0, (len+1)*sizeof(WCHAR));
		ret = (CLSIDFromString(wstr, *ProductCode) == NOERROR);
		HeapFree(GetProcessHeap(), 0, wstr);
		wstr = NULL;
	}

	if(!ret)
	{
		HeapFree(GetProcessHeap(), 0, *ProductCode);
		*ProductCode = NULL;
		*PackageName = str;
	}

	return ret;
}

static VOID StringListAppend(LPSTR *StringList, LPCSTR StringAppend)
{
	LPSTR TempStr = HeapReAlloc(GetProcessHeap(), 0, *StringList, HeapSize(GetProcessHeap(), 0, *StringList)+strlen(StringAppend));
	if(!TempStr)
	{
		WINE_ERR("Out of memory!\n");
		ExitProcess(1);
	}
	*StringList = TempStr;
	strcat(*StringList, StringAppend);
}

static VOID StringCompareRemoveLast(LPSTR String, CHAR character)
{
	int len = strlen(String);
	if(len && String[len-1] == character) String[len-1] = 0;
}

static INT MSIEXEC_lstrncmpiA(LPCSTR str1, LPCSTR str2, INT size)
{
    INT ret;

    if ((str1 == NULL) && (str2 == NULL)) return 0;
    if (str1 == NULL) return -1;
    if (str2 == NULL) return 1;

    ret = CompareStringA(GetThreadLocale(), NORM_IGNORECASE, str1, size, str2, -1);
    if (ret) ret -= 2;

    return ret;
}

static VOID *LoadProc(LPCSTR DllName, LPCSTR ProcName, HMODULE* DllHandle)
{
	VOID* (*proc)(void);

	*DllHandle = LoadLibraryExA(DllName, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	if(!*DllHandle)
	{
		fprintf(stderr, "Unable to load dll %s\n", DllName);
		ExitProcess(1);
	}
	proc = (VOID *) GetProcAddress(*DllHandle, ProcName);
	if(!proc)
	{
		fprintf(stderr, "Dll %s does not implement function %s\n", DllName, ProcName);
		FreeLibrary(*DllHandle);
		ExitProcess(1);
	}

	return proc;
}

static void DllRegisterServer(LPCSTR DllName)
{
	HRESULT hr;
	DLLREGISTERSERVER pfDllRegisterServer = NULL;
	HMODULE DllHandle = NULL;

	pfDllRegisterServer = LoadProc(DllName, "DllRegisterServer", &DllHandle);

	hr = pfDllRegisterServer();
	if(FAILED(hr))
	{
		fprintf(stderr, "Failed to register dll %s\n", DllName);
		ExitProcess(1);
	}
	printf("Successfully registered dll %s\n", DllName);
	if(DllHandle)
		FreeLibrary(DllHandle);
}

static void DllUnregisterServer(LPCSTR DllName)
{
	HRESULT hr;
	DLLUNREGISTERSERVER pfDllUnregisterServer = NULL;
	HMODULE DllHandle = NULL;

	pfDllUnregisterServer = LoadProc(DllName, "DllUnregisterServer", &DllHandle);

	hr = pfDllUnregisterServer();
	if(FAILED(hr))
	{
		fprintf(stderr, "Failed to unregister dll %s\n", DllName);
		ExitProcess(1);
	}
	printf("Successfully unregistered dll %s\n", DllName);
	if(DllHandle)
		FreeLibrary(DllHandle);
}

int main(int argc, char *argv[])
{
	int i;
	BOOL FunctionInstall = FALSE;
	BOOL FunctionInstallAdmin = FALSE;
	BOOL FunctionRepair = FALSE;
	BOOL FunctionAdvertise = FALSE;
	BOOL FunctionPatch = FALSE;
	BOOL FunctionDllRegisterServer = FALSE;
	BOOL FunctionDllUnregisterServer = FALSE;
	BOOL FunctionRegServer = FALSE;
	BOOL FunctionUnregServer = FALSE;
	BOOL FunctionUnknown = FALSE;

	BOOL GotProductCode = FALSE;
	LPCSTR PackageName = NULL;
	LPGUID ProductCode = HeapAlloc(GetProcessHeap(), 0, sizeof(GUID));
	LPSTR Properties = HeapAlloc(GetProcessHeap(), 0, 1);

	DWORD RepairMode = 0;

	DWORD AdvertiseMode = 0;
	LPSTR Transforms = HeapAlloc(GetProcessHeap(), 0, 1);
	LANGID Language = 0;

	DWORD LogMode = 0;
	LPSTR LogFileName = NULL;
	DWORD LogAttributes = 0;

	LPSTR PatchFileName = NULL;
	INSTALLTYPE InstallType = INSTALLTYPE_DEFAULT;

	INSTALLUILEVEL InstallUILevel = 0, retInstallUILevel;

	LPSTR DllName = NULL;

	Properties[0] = 0;
	Transforms[0] = 0;

	for(i = 1; i < argc; i++)
	{
		WINE_TRACE("argv[%d] = %s\n", i, argv[i]);

                if (!lstrcmpiA(argv[i], "/regserver"))
                {
                    FunctionRegServer = TRUE;
                }
                else if (!lstrcmpiA(argv[i], "/unregserver") || !lstrcmpiA(argv[i], "/unregister"))
                {
                    FunctionUnregServer = TRUE;
                }
		else if(!MSIEXEC_lstrncmpiA(argv[i], "/i", 2))
		{
			char *argvi = argv[i];
			FunctionInstall = TRUE;
			if(strlen(argvi) > 2)
				argvi += 2;
			else {
				i++;
				if(i >= argc)
					ShowUsage(1);
				WINE_TRACE("argv[%d] = %s\n", i, argv[i]);
				argvi = argv[i];
			}
			GotProductCode = GetProductCode(argvi, &PackageName, &ProductCode);
		}
		else if(!lstrcmpiA(argv[i], "/a"))
		{
			FunctionInstall = TRUE;
			FunctionInstallAdmin = TRUE;
			InstallType = INSTALLTYPE_NETWORK_IMAGE;
			i++;
			if(i >= argc)
				ShowUsage(1);
			WINE_TRACE("argv[%d] = %s\n", i, argv[i]);
			PackageName = argv[i];
			StringListAppend(&Properties, ActionAdmin);
		}
		else if(!MSIEXEC_lstrncmpiA(argv[i], "/f", 2))
		{
			int j;
			int len = strlen(argv[i]);
			FunctionRepair = TRUE;
			for(j = 2; j < len; j++)
			{
				switch(argv[i][j])
				{
					case 'P':
					case 'p':
						RepairMode |= REINSTALLMODE_FILEMISSING;
						break;
					case 'O':
					case 'o':
						RepairMode |= REINSTALLMODE_FILEOLDERVERSION;
						break;
					case 'E':
					case 'e':
						RepairMode |= REINSTALLMODE_FILEEQUALVERSION;
						break;
					case 'D':
					case 'd':
						RepairMode |= REINSTALLMODE_FILEEXACT;
						break;
					case 'C':
					case 'c':
						RepairMode |= REINSTALLMODE_FILEVERIFY;
						break;
					case 'A':
					case 'a':
						RepairMode |= REINSTALLMODE_FILEREPLACE;
						break;
					case 'U':
					case 'u':
						RepairMode |= REINSTALLMODE_USERDATA;
						break;
					case 'M':
					case 'm':
						RepairMode |= REINSTALLMODE_MACHINEDATA;
						break;
					case 'S':
					case 's':
						RepairMode |= REINSTALLMODE_SHORTCUT;
						break;
					case 'V':
					case 'v':
						RepairMode |= REINSTALLMODE_PACKAGE;
						break;
					default:
						fprintf(stderr, "Unknown option \"%c\" in Repair mode\n", argv[i][j]);
						break;
				}
			}
			if(len == 2)
			{
				RepairMode = REINSTALLMODE_FILEMISSING |
					REINSTALLMODE_FILEEQUALVERSION |
					REINSTALLMODE_FILEVERIFY |
					REINSTALLMODE_MACHINEDATA |
					REINSTALLMODE_SHORTCUT;
			}
			i++;
			if(i >= argc)
				ShowUsage(1);
			WINE_TRACE("argv[%d] = %s\n", i, argv[i]);
			GotProductCode = GetProductCode(argv[i], &PackageName, &ProductCode);
		}
		else if(!lstrcmpiA(argv[i], "/x"))
		{
			FunctionInstall = TRUE;
			i++;
			if(i >= argc)
				ShowUsage(1);
			WINE_TRACE("argv[%d] = %s\n", i, argv[i]);
			GotProductCode = GetProductCode(argv[i], &PackageName, &ProductCode);
			StringListAppend(&Properties, RemoveAll);
		}
		else if(!MSIEXEC_lstrncmpiA(argv[i], "/j", 2))
		{
			int j;
			int len = strlen(argv[i]);
			FunctionAdvertise = TRUE;
			for(j = 2; j < len; j++)
			{
				switch(argv[i][j])
				{
					case 'U':
					case 'u':
						AdvertiseMode = ADVERTISEFLAGS_USERASSIGN;
						break;
					case 'M':
					case 'm':
						AdvertiseMode = ADVERTISEFLAGS_MACHINEASSIGN;
						break;
					default:
						fprintf(stderr, "Unknown option \"%c\" in Advertise mode\n", argv[i][j]);
						break;
				}
			}
			i++;
			if(i >= argc)
				ShowUsage(1);
			WINE_TRACE("argv[%d] = %s\n", i, argv[i]);
			PackageName = argv[i];
		}
		else if(!lstrcmpiA(argv[i], "u"))
		{
			FunctionAdvertise = TRUE;
			AdvertiseMode = ADVERTISEFLAGS_USERASSIGN;
			i++;
			if(i >= argc)
				ShowUsage(1);
			WINE_TRACE("argv[%d] = %s\n", i, argv[i]);
			PackageName = argv[i];
		}
		else if(!lstrcmpiA(argv[i], "m"))
		{
			FunctionAdvertise = TRUE;
			AdvertiseMode = ADVERTISEFLAGS_MACHINEASSIGN;
			i++;
			if(i >= argc)
				ShowUsage(1);
			WINE_TRACE("argv[%d] = %s\n", i, argv[i]);
			PackageName = argv[i];
		}
		else if(!lstrcmpiA(argv[i], "/t"))
		{
			i++;
			if(i >= argc)
				ShowUsage(1);
			WINE_TRACE("argv[%d] = %s\n", i, argv[i]);
			StringListAppend(&Transforms, argv[i]);
			StringListAppend(&Transforms, ";");
		}
		else if(!MSIEXEC_lstrncmpiA(argv[i], "TRANSFORMS=", 11))
		{
			StringListAppend(&Transforms, argv[i]+11);
			StringListAppend(&Transforms, ";");
		}
		else if(!lstrcmpiA(argv[i], "/g"))
		{
			i++;
			if(i >= argc)
				ShowUsage(1);
			WINE_TRACE("argv[%d] = %s\n", i, argv[i]);
			Language = strtol(argv[i], NULL, 0);
		}
		else if(!MSIEXEC_lstrncmpiA(argv[i], "/l", 2))
		{
			int j;
			int len = strlen(argv[i]);
			for(j = 2; j < len; j++)
			{
				switch(argv[i][j])
				{
					case 'I':
					case 'i':
						LogMode |= INSTALLLOGMODE_INFO;
						break;
					case 'W':
					case 'w':
						LogMode |= INSTALLLOGMODE_WARNING;
						break;
					case 'E':
					case 'e':
						LogMode |= INSTALLLOGMODE_ERROR;
						break;
					case 'A':
					case 'a':
						LogMode |= INSTALLLOGMODE_ACTIONSTART;
						break;
					case 'R':
					case 'r':
						LogMode |= INSTALLLOGMODE_ACTIONDATA;
						break;
					case 'U':
					case 'u':
						LogMode |= INSTALLLOGMODE_USER;
						break;
					case 'C':
					case 'c':
						LogMode |= INSTALLLOGMODE_COMMONDATA;
						break;
					case 'M':
					case 'm':
						LogMode |= INSTALLLOGMODE_FATALEXIT;
						break;
					case 'O':
					case 'o':
						LogMode |= INSTALLLOGMODE_OUTOFDISKSPACE;
						break;
					case 'P':
					case 'p':
						LogMode |= INSTALLLOGMODE_PROPERTYDUMP;
						break;
					case 'V':
					case 'v':
						LogMode |= INSTALLLOGMODE_VERBOSE;
						break;
					case '*':
						LogMode = INSTALLLOGMODE_FATALEXIT |
							INSTALLLOGMODE_ERROR |
							INSTALLLOGMODE_WARNING |
							INSTALLLOGMODE_USER |
							INSTALLLOGMODE_INFO |
							INSTALLLOGMODE_RESOLVESOURCE |
							INSTALLLOGMODE_OUTOFDISKSPACE |
							INSTALLLOGMODE_ACTIONSTART |
							INSTALLLOGMODE_ACTIONDATA |
							INSTALLLOGMODE_COMMONDATA |
							INSTALLLOGMODE_PROPERTYDUMP |
							INSTALLLOGMODE_PROGRESS |
							INSTALLLOGMODE_INITIALIZE |
							INSTALLLOGMODE_TERMINATE |
							INSTALLLOGMODE_SHOWDIALOG;
						break;
					case '+':
						LogAttributes |= INSTALLLOGATTRIBUTES_APPEND;
						break;
					case '!':
						LogAttributes |= INSTALLLOGATTRIBUTES_FLUSHEACHLINE;
						break;
					default:
						break;
				}
			}
			i++;
			if(i >= argc)
				ShowUsage(1);
			WINE_TRACE("argv[%d] = %s\n", i, argv[i]);
			LogFileName = argv[i];
			if(MsiEnableLogA(LogMode, LogFileName, LogAttributes) != ERROR_SUCCESS)
			{
				fprintf(stderr, "Logging in %s (0x%08lx, %lu) failed\n", LogFileName, LogMode, LogAttributes);
				ExitProcess(1);
			}
		}
		else if(!lstrcmpiA(argv[i], "/p"))
		{
			FunctionPatch = TRUE;
			i++;
			if(i >= argc)
				ShowUsage(1);
			WINE_TRACE("argv[%d] = %s\n", i, argv[i]);
			PatchFileName = argv[i];
		}
		else if(!MSIEXEC_lstrncmpiA(argv[i], "/q", 2))
		{
			if(strlen(argv[i]) == 2 || !lstrcmpiA(argv[i]+2, "n"))
			{
				InstallUILevel = INSTALLUILEVEL_NONE;
			}
			else if(!lstrcmpiA(argv[i]+2, "b"))
			{
				InstallUILevel = INSTALLUILEVEL_BASIC;
			}
			else if(!lstrcmpiA(argv[i]+2, "r"))
			{
				InstallUILevel = INSTALLUILEVEL_REDUCED;
			}
			else if(!lstrcmpiA(argv[i]+2, "f"))
			{
				InstallUILevel = INSTALLUILEVEL_FULL|INSTALLUILEVEL_ENDDIALOG;
			}
			else if(!lstrcmpiA(argv[i]+2, "n+"))
			{
				InstallUILevel = INSTALLUILEVEL_NONE|INSTALLUILEVEL_ENDDIALOG;
			}
			else if(!lstrcmpiA(argv[i]+2, "b+"))
			{
				InstallUILevel = INSTALLUILEVEL_BASIC|INSTALLUILEVEL_ENDDIALOG;
			}
			else if(!lstrcmpiA(argv[i]+2, "b-"))
			{
				InstallUILevel = INSTALLUILEVEL_BASIC|INSTALLUILEVEL_PROGRESSONLY;
			}
			else if(!lstrcmpiA(argv[i]+2, "b+!"))
            		{
				InstallUILevel = INSTALLUILEVEL_BASIC|INSTALLUILEVEL_ENDDIALOG;
				WINE_FIXME("Unknown modifier: !\n");
			}
			else
			{
				fprintf(stderr, "Unknown option \"%s\" for UI level\n", argv[i]+2);
			}
			retInstallUILevel = MsiSetInternalUI(InstallUILevel, NULL);
			if(retInstallUILevel == INSTALLUILEVEL_NOCHANGE)
			{
				fprintf(stderr, "Setting the UI level to 0x%x failed.\n", InstallUILevel);
				ExitProcess(1);
			}
		}
		else if(!lstrcmpiA(argv[i], "/y"))
		{
			FunctionDllRegisterServer = TRUE;
			i++;
			if(i >= argc)
				ShowUsage(1);
			WINE_TRACE("argv[%d] = %s\n", i, argv[i]);
			DllName = argv[i];
		}
		else if(!lstrcmpiA(argv[i], "/z"))
		{
			FunctionDllUnregisterServer = TRUE;
			i++;
			if(i >= argc)
				ShowUsage(1);
			WINE_TRACE("argv[%d] = %s\n", i, argv[i]);
			DllName = argv[i];
		}
		else if(!lstrcmpiA(argv[i], "/h") || !lstrcmpiA(argv[i], "/?"))
		{
			ShowUsage(0);
		}
		else if(!lstrcmpiA(argv[i], "/m"))
		{
			FunctionUnknown = TRUE;
			WINE_FIXME("Unknown parameter /m\n");
		}
		else if(!lstrcmpiA(argv[i], "/D"))
		{
			FunctionUnknown = TRUE;
			WINE_FIXME("Unknown parameter /D\n");
		}
		else if(strchr(argv[i], '='))
		{
			StringListAppend(&Properties, argv[i]);
			StringListAppend(&Properties, " ");
		}
		else
		{
			FunctionInstall = TRUE;
			GotProductCode = GetProductCode(argv[i], &PackageName, &ProductCode);
		}
	}

	StringCompareRemoveLast(Properties, ' ');
	StringCompareRemoveLast(Transforms, ';');

	if(FunctionInstallAdmin && FunctionPatch)
		FunctionInstall = FALSE;

	if(FunctionInstall)
	{
		if(GotProductCode)
		{
			WINE_FIXME("Product code treatment not implemented yet\n");
			ExitProcess(1);
		}
		else
		{
			if(MsiInstallProductA(PackageName, Properties) != ERROR_SUCCESS)
			{
				fprintf(stderr, "Installation of %s (%s) failed.\n", PackageName, Properties);
				ExitProcess(1);
			}
		}
	}
	else if(FunctionRepair)
	{
		if(GotProductCode)
		{
			WINE_FIXME("Product code treatment not implemented yet\n");
			ExitProcess(1);
		}
		else
		{
			if(MsiReinstallProductA(PackageName, RepairMode) != ERROR_SUCCESS)
			{
				fprintf(stderr, "Repair of %s (0x%08lx) failed.\n", PackageName, RepairMode);
				ExitProcess(1);
			}
		}
	}
	else if(FunctionAdvertise)
	{
		if(MsiAdvertiseProductA(PackageName, (LPSTR) AdvertiseMode, Transforms, Language) != ERROR_SUCCESS)
		{
			fprintf(stderr, "Advertising of %s (%lu, %s, 0x%04x) failed.\n", PackageName, AdvertiseMode, Transforms, Language);
			ExitProcess(1);
		}
	}
	else if(FunctionPatch)
	{
		if(MsiApplyPatchA(PatchFileName, PackageName, InstallType, Properties) != ERROR_SUCCESS)
		{
			fprintf(stderr, "Patching with %s (%s, %d, %s)\n", PatchFileName, PackageName, InstallType, Properties);
			ExitProcess(1);
		}
	}
	else if(FunctionDllRegisterServer)
	{
		DllRegisterServer(DllName);
	}
	else if(FunctionDllUnregisterServer)
	{
		DllUnregisterServer(DllName);
	}
	else if (FunctionRegServer)
	{
		WINE_FIXME( "/regserver not implemented yet, ignoring\n" );
	}
	else if (FunctionUnregServer)
	{
		WINE_FIXME( "/unregserver not implemented yet, ignoring\n" );
	}
	else if (FunctionUnknown)
	{
		WINE_FIXME( "Unknown function, ignoring\n" );
	}
	else
		ShowUsage(1);

	return 0;
}
