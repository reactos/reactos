/*
 * msiexec.exe implementation
 *
 * Copyright 2004 Vincent Béron
 * Copyright 2005 Mike McCormack
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <msi.h>
#include <objbase.h>
#include <stdio.h>

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(msiexec);

typedef HRESULT (WINAPI *DLLREGISTERSERVER)(void);
typedef HRESULT (WINAPI *DLLUNREGISTERSERVER)(void);

DWORD DoService(void);

struct string_list
{
	struct string_list *next;
	WCHAR str[1];
};

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

static const WCHAR ActionAdmin[] = {
   'A','C','T','I','O','N','=','A','D','M','I','N',0 };
static const WCHAR RemoveAll[] = {
   'R','E','M','O','V','E','=','A','L','L',0 };

static const WCHAR InstallRunOnce[] = {
   'S','o','f','t','w','a','r','e','\\',
   'M','i','c','r','o','s','o','f','t','\\',
   'W','i','n','d','o','w','s','\\',
   'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
   'I','n','s','t','a','l','l','e','r','\\',
   'R','u','n','O','n','c','e','E','n','t','r','i','e','s',0};

static void ShowUsage(int ExitCode)
{
	printf(UsageStr);
	ExitProcess(ExitCode);
}

static BOOL IsProductCode(LPWSTR str)
{
	GUID ProductCode;

	if(lstrlenW(str) != 38)
		return FALSE;
	return ( (CLSIDFromString(str, &ProductCode) == NOERROR) );

}

static VOID StringListAppend(struct string_list **list, LPCWSTR str)
{
	struct string_list *entry;
	DWORD size;

	size = sizeof *entry + lstrlenW(str) * sizeof (WCHAR);
	entry = HeapAlloc(GetProcessHeap(), 0, size);
	if(!entry)
	{
		WINE_ERR("Out of memory!\n");
		ExitProcess(1);
	}
	lstrcpyW(entry->str, str);
	entry->next = NULL;

	/*
	 * Ignoring o(n^2) time complexity to add n strings for simplicity,
	 *  add the string to the end of the list to preserve the order.
	 */
	while( *list )
		list = &(*list)->next;
	*list = entry;
}

static LPWSTR build_properties(struct string_list *property_list)
{
	struct string_list *list;
	LPWSTR ret, p, value;
	DWORD len;
	BOOL needs_quote;

	if(!property_list)
		return NULL;

	/* count the space we need */
	len = 1;
	for(list = property_list; list; list = list->next)
		len += lstrlenW(list->str) + 3;

	ret = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );

	/* add a space before each string, and quote the value */
	p = ret;
	for(list = property_list; list; list = list->next)
	{
		value = strchrW(list->str,'=');
		if(!value)
			continue;
		len = value - list->str;
		*p++ = ' ';
		memcpy(p, list->str, len * sizeof(WCHAR));
		p += len;
		*p++ = '=';

		/* check if the value contains spaces and maybe quote it */
		value++;
		needs_quote = strchrW(value,' ') ? 1 : 0;
		if(needs_quote)
			*p++ = '"';
		len = lstrlenW(value);
		memcpy(p, value, len * sizeof(WCHAR));
		p += len;
		if(needs_quote)
			*p++ = '"';
	}
	*p = 0;

	WINE_TRACE("properties -> %s\n", wine_dbgstr_w(ret) );

	return ret;
}

static LPWSTR build_transforms(struct string_list *transform_list)
{
	struct string_list *list;
	LPWSTR ret, p;
	DWORD len;

	/* count the space we need */
	len = 1;
	for(list = transform_list; list; list = list->next)
		len += lstrlenW(list->str) + 1;

	ret = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );

	/* add all the transforms with a semicolon between each one */
	p = ret;
	for(list = transform_list; list; list = list->next)
	{
		len = lstrlenW(list->str);
		lstrcpynW(p, list->str, len );
		p += len;
		if(list->next)
			*p++ = ';';
	}
	*p = 0;

	return ret;
}

static DWORD msi_atou(LPCWSTR str)
{
	DWORD ret = 0;
	while(*str >= '0' && *str <= '9')
	{
		ret *= 10;
		ret += (*str - '0');
		str++;
	}
	return ret;
}

static LPWSTR msi_strdup(LPCWSTR str)
{
	DWORD len = lstrlenW(str)+1;
	LPWSTR ret = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR)*len);
	lstrcpyW(ret, str);
	return ret;
}

/* str1 is the same as str2, ignoring case */
static BOOL msi_strequal(LPCWSTR str1, LPCSTR str2)
{
	DWORD len, ret;
	LPWSTR strW;

	len = MultiByteToWideChar( CP_ACP, 0, str2, -1, NULL, 0);
	if( !len )
		return FALSE;
	if( lstrlenW(str1) != (len-1) )
		return FALSE;
	strW = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR)*len);
	MultiByteToWideChar( CP_ACP, 0, str2, -1, strW, len);
	ret = CompareStringW(GetThreadLocale(), NORM_IGNORECASE, str1, len, strW, len);
	HeapFree(GetProcessHeap(), 0, strW);
	return (ret == CSTR_EQUAL);
}

/* prefix is hyphen or dash, and str1 is the same as str2, ignoring case */
static BOOL msi_option_equal(LPCWSTR str1, LPCSTR str2)
{
    if (str1[0] != '/' && str1[0] != '-')
        return FALSE;

    /* skip over the hyphen or slash */
    return msi_strequal(str1 + 1, str2);
}

/* str2 is at the beginning of str1, ignoring case */
static BOOL msi_strprefix(LPCWSTR str1, LPCSTR str2)
{
	DWORD len, ret;
	LPWSTR strW;

	len = MultiByteToWideChar( CP_ACP, 0, str2, -1, NULL, 0);
	if( !len )
		return FALSE;
	if( lstrlenW(str1) < (len-1) )
		return FALSE;
	strW = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR)*len);
	MultiByteToWideChar( CP_ACP, 0, str2, -1, strW, len);
	ret = CompareStringW(GetThreadLocale(), NORM_IGNORECASE, str1, len-1, strW, len-1);
	HeapFree(GetProcessHeap(), 0, strW);
	return (ret == CSTR_EQUAL);
}

/* prefix is hyphen or dash, and str2 is at the beginning of str1, ignoring case */
static BOOL msi_option_prefix(LPCWSTR str1, LPCSTR str2)
{
    if (str1[0] != '/' && str1[0] != '-')
        return FALSE;

    /* skip over the hyphen or slash */
    return msi_strprefix(str1 + 1, str2);
}

static VOID *LoadProc(LPCWSTR DllName, LPCSTR ProcName, HMODULE* DllHandle)
{
	VOID* (*proc)(void);

	*DllHandle = LoadLibraryExW(DllName, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	if(!*DllHandle)
	{
		fprintf(stderr, "Unable to load dll %s\n", wine_dbgstr_w(DllName));
		ExitProcess(1);
	}
	proc = (VOID *) GetProcAddress(*DllHandle, ProcName);
	if(!proc)
	{
		fprintf(stderr, "Dll %s does not implement function %s\n",
			wine_dbgstr_w(DllName), ProcName);
		FreeLibrary(*DllHandle);
		ExitProcess(1);
	}

	return proc;
}

static DWORD DoDllRegisterServer(LPCWSTR DllName)
{
	HRESULT hr;
	DLLREGISTERSERVER pfDllRegisterServer = NULL;
	HMODULE DllHandle = NULL;

	pfDllRegisterServer = LoadProc(DllName, "DllRegisterServer", &DllHandle);

	hr = pfDllRegisterServer();
	if(FAILED(hr))
	{
		fprintf(stderr, "Failed to register dll %s\n", wine_dbgstr_w(DllName));
		return 1;
	}
	printf("Successfully registered dll %s\n", wine_dbgstr_w(DllName));
	if(DllHandle)
		FreeLibrary(DllHandle);
	return 0;
}

static DWORD DoDllUnregisterServer(LPCWSTR DllName)
{
	HRESULT hr;
	DLLUNREGISTERSERVER pfDllUnregisterServer = NULL;
	HMODULE DllHandle = NULL;

	pfDllUnregisterServer = LoadProc(DllName, "DllUnregisterServer", &DllHandle);

	hr = pfDllUnregisterServer();
	if(FAILED(hr))
	{
		fprintf(stderr, "Failed to unregister dll %s\n", wine_dbgstr_w(DllName));
		return 1;
	}
	printf("Successfully unregistered dll %s\n", wine_dbgstr_w(DllName));
	if(DllHandle)
		FreeLibrary(DllHandle);
	return 0;
}

static DWORD DoRegServer(void)
{
    SC_HANDLE scm, service;
    CHAR path[MAX_PATH+12];
    DWORD ret = 0;

    scm = OpenSCManagerA(NULL, SERVICES_ACTIVE_DATABASEA, SC_MANAGER_CREATE_SERVICE);
    if (!scm)
    {
        fprintf(stderr, "Failed to open the service control manager.\n");
        return 1;
    }

    GetSystemDirectoryA(path, MAX_PATH);
    lstrcatA(path, "\\msiexec.exe /V");

    service = CreateServiceA(scm, "MSIServer", "MSIServer", GENERIC_ALL,
                             SERVICE_WIN32_SHARE_PROCESS, SERVICE_DEMAND_START,
                             SERVICE_ERROR_NORMAL, path, NULL, NULL,
                             NULL, NULL, NULL);

    if (service) CloseServiceHandle(service);
    else if (GetLastError() != ERROR_SERVICE_EXISTS)
    {
        fprintf(stderr, "Failed to create MSI service\n");
        ret = 1;
    }
    CloseServiceHandle(scm);
    return ret;
}

static INT DoEmbedding( LPWSTR key )
{
	printf("Remote custom actions are not supported yet\n");
	return 1;
}

/*
 * state machine to break up the command line properly
 */

enum chomp_state
{
	cs_whitespace,
	cs_token,
	cs_quote
};

static int chomp( WCHAR *str )
{
	enum chomp_state state = cs_token;
	WCHAR *p, *out;
	int count = 1, ignore;

	for( p = str, out = str; *p; p++ )
	{
		ignore = 1;
		switch( state )
		{
		case cs_whitespace:
			switch( *p )
			{
			case ' ':
				break;
			case '"':
				state = cs_quote;
				count++;
				break;
			default:
				count++;
				ignore = 0;
				state = cs_token;
			}
			break;

		case cs_token:
			switch( *p )
			{
			case '"':
				state = cs_quote;
				break;
			case ' ':
				state = cs_whitespace;
				*out++ = 0;
				break;
			default:
				ignore = 0;
			}
			break;

		case cs_quote:
			switch( *p )
			{
			case '"':
				state = cs_token;
				break;
			default:
				ignore = 0;
			}
			break;
		}
		if( !ignore )
			*out++ = *p;
	}

	*out = 0;

	return count;
}

static void process_args( WCHAR *cmdline, int *pargc, WCHAR ***pargv )
{
	WCHAR **argv, *p = msi_strdup(cmdline);
	int i, n;

	n = chomp( p );
	argv = HeapAlloc(GetProcessHeap(), 0, sizeof (WCHAR*)*(n+1));
	for( i=0; i<n; i++ )
	{
		argv[i] = p;
		p += lstrlenW(p) + 1;
	}
	argv[i] = NULL;

	*pargc = n;
	*pargv = argv;
}

static BOOL process_args_from_reg( LPWSTR ident, int *pargc, WCHAR ***pargv )
{
	LONG r;
	HKEY hkey = 0, hkeyArgs = 0;
	DWORD sz = 0, type = 0;
	LPWSTR buf = NULL;
	BOOL ret = FALSE;

	r = RegOpenKeyW(HKEY_LOCAL_MACHINE, InstallRunOnce, &hkey);
	if(r != ERROR_SUCCESS)
		return FALSE;
	r = RegQueryValueExW(hkey, ident, 0, &type, 0, &sz);
	if(r == ERROR_SUCCESS && type == REG_SZ)
	{
		buf = HeapAlloc(GetProcessHeap(), 0, sz);
		r = RegQueryValueExW(hkey, ident, 0, &type, (LPBYTE)buf, &sz);
		if( r == ERROR_SUCCESS )
		{
			process_args(buf, pargc, pargv);
			ret = TRUE;
		}
		HeapFree(GetProcessHeap(), 0, buf);
	}
	RegCloseKey(hkeyArgs);
	return ret;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
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
	BOOL FunctionServer = FALSE;
	BOOL FunctionUnknown = FALSE;

	LPWSTR PackageName = NULL;
	LPWSTR Properties = NULL;
	struct string_list *property_list = NULL;

	DWORD RepairMode = 0;

	DWORD_PTR AdvertiseMode = 0;
	struct string_list *transform_list = NULL;
	LANGID Language = 0;

	DWORD LogMode = 0;
	LPWSTR LogFileName = NULL;
	DWORD LogAttributes = 0;

	LPWSTR PatchFileName = NULL;
	INSTALLTYPE InstallType = INSTALLTYPE_DEFAULT;

	INSTALLUILEVEL InstallUILevel = INSTALLUILEVEL_FULL;

	LPWSTR DllName = NULL;
	DWORD ReturnCode;
	int argc;
	LPWSTR *argvW = NULL;

	/* parse the command line */
	process_args( GetCommandLineW(), &argc, &argvW );

	/*
	 * If the args begin with /@ IDENT then we need to load the real
	 * command line out of the RunOnceEntries key in the registry.
	 *  We do that before starting to process the real commandline,
	 * then overwrite the commandline again.
	 */
	if(argc>1 && msi_option_equal(argvW[1], "@"))
	{
		if(!process_args_from_reg( argvW[2], &argc, &argvW ))
			return 1;
	}

	if (argc == 3 && msi_option_equal(argvW[1], "Embedding"))
		return DoEmbedding( argvW[2] );

	for(i = 1; i < argc; i++)
	{
		WINE_TRACE("argvW[%d] = %s\n", i, wine_dbgstr_w(argvW[i]));

		if (msi_option_equal(argvW[i], "regserver"))
		{
			FunctionRegServer = TRUE;
		}
		else if (msi_option_equal(argvW[i], "unregserver") || msi_option_equal(argvW[i], "unregister"))
		{
			FunctionUnregServer = TRUE;
		}
		else if(msi_option_prefix(argvW[i], "i"))
		{
			LPWSTR argvWi = argvW[i];
			FunctionInstall = TRUE;
			if(lstrlenW(argvWi) > 2)
				argvWi += 2;
			else
			{
				i++;
				if(i >= argc)
					ShowUsage(1);
				WINE_TRACE("argvW[%d] = %s\n", i, wine_dbgstr_w(argvW[i]));
				argvWi = argvW[i];
			}
			PackageName = argvWi;
		}
		else if(msi_option_equal(argvW[i], "a"))
		{
			FunctionInstall = TRUE;
			FunctionInstallAdmin = TRUE;
			InstallType = INSTALLTYPE_NETWORK_IMAGE;
			i++;
			if(i >= argc)
				ShowUsage(1);
			WINE_TRACE("argvW[%d] = %s\n", i, wine_dbgstr_w(argvW[i]));
			PackageName = argvW[i];
			StringListAppend(&property_list, ActionAdmin);
		}
		else if(msi_option_prefix(argvW[i], "f"))
		{
			int j;
			int len = lstrlenW(argvW[i]);
			FunctionRepair = TRUE;
			for(j = 2; j < len; j++)
			{
				switch(argvW[i][j])
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
						fprintf(stderr, "Unknown option \"%c\" in Repair mode\n", argvW[i][j]);
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
			WINE_TRACE("argvW[%d] = %s\n", i, wine_dbgstr_w(argvW[i]));
			PackageName = argvW[i];
		}
		else if(msi_option_prefix(argvW[i], "x"))
		{
			FunctionInstall = TRUE;
			PackageName = argvW[i]+2;
			if (!PackageName[0])
			{
				i++;
				if (i >= argc)
					ShowUsage(1);
				PackageName = argvW[i];
			}
			WINE_TRACE("PackageName = %s\n", wine_dbgstr_w(PackageName));
			StringListAppend(&property_list, RemoveAll);
		}
		else if(msi_option_prefix(argvW[i], "j"))
		{
			int j;
			int len = lstrlenW(argvW[i]);
			FunctionAdvertise = TRUE;
			for(j = 2; j < len; j++)
			{
				switch(argvW[i][j])
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
						fprintf(stderr, "Unknown option \"%c\" in Advertise mode\n", argvW[i][j]);
						break;
				}
			}
			i++;
			if(i >= argc)
				ShowUsage(1);
			WINE_TRACE("argvW[%d] = %s\n", i, wine_dbgstr_w(argvW[i]));
			PackageName = argvW[i];
		}
		else if(msi_strequal(argvW[i], "u"))
		{
			FunctionAdvertise = TRUE;
			AdvertiseMode = ADVERTISEFLAGS_USERASSIGN;
			i++;
			if(i >= argc)
				ShowUsage(1);
			WINE_TRACE("argvW[%d] = %s\n", i, wine_dbgstr_w(argvW[i]));
			PackageName = argvW[i];
		}
		else if(msi_strequal(argvW[i], "m"))
		{
			FunctionAdvertise = TRUE;
			AdvertiseMode = ADVERTISEFLAGS_MACHINEASSIGN;
			i++;
			if(i >= argc)
				ShowUsage(1);
			WINE_TRACE("argvW[%d] = %s\n", i, wine_dbgstr_w(argvW[i]));
			PackageName = argvW[i];
		}
		else if(msi_option_equal(argvW[i], "t"))
		{
			i++;
			if(i >= argc)
				ShowUsage(1);
			WINE_TRACE("argvW[%d] = %s\n", i, wine_dbgstr_w(argvW[i]));
			StringListAppend(&transform_list, argvW[i]);
		}
		else if(msi_option_equal(argvW[i], "g"))
		{
			i++;
			if(i >= argc)
				ShowUsage(1);
			WINE_TRACE("argvW[%d] = %s\n", i, wine_dbgstr_w(argvW[i]));
			Language = msi_atou(argvW[i]);
		}
		else if(msi_option_prefix(argvW[i], "l"))
		{
			int j;
			int len = lstrlenW(argvW[i]);
			for(j = 2; j < len; j++)
			{
				switch(argvW[i][j])
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
			WINE_TRACE("argvW[%d] = %s\n", i, wine_dbgstr_w(argvW[i]));
			LogFileName = argvW[i];
			if(MsiEnableLogW(LogMode, LogFileName, LogAttributes) != ERROR_SUCCESS)
			{
				fprintf(stderr, "Logging in %s (0x%08x, %u) failed\n",
					 wine_dbgstr_w(LogFileName), LogMode, LogAttributes);
				ExitProcess(1);
			}
		}
		else if(msi_option_equal(argvW[i], "p"))
		{
			FunctionPatch = TRUE;
			i++;
			if(i >= argc)
				ShowUsage(1);
			WINE_TRACE("argvW[%d] = %s\n", i, wine_dbgstr_w(argvW[i]));
			PatchFileName = argvW[i];
		}
		else if(msi_option_prefix(argvW[i], "q"))
		{
			if(lstrlenW(argvW[i]) == 2 || msi_strequal(argvW[i]+2, "n") ||
			   msi_strequal(argvW[i] + 2, "uiet"))
			{
				InstallUILevel = INSTALLUILEVEL_NONE;
			}
			else if(msi_strequal(argvW[i]+2, "b"))
			{
				InstallUILevel = INSTALLUILEVEL_BASIC;
			}
			else if(msi_strequal(argvW[i]+2, "r"))
			{
				InstallUILevel = INSTALLUILEVEL_REDUCED;
			}
			else if(msi_strequal(argvW[i]+2, "f"))
			{
				InstallUILevel = INSTALLUILEVEL_FULL|INSTALLUILEVEL_ENDDIALOG;
			}
			else if(msi_strequal(argvW[i]+2, "n+"))
			{
				InstallUILevel = INSTALLUILEVEL_NONE|INSTALLUILEVEL_ENDDIALOG;
			}
			else if(msi_strequal(argvW[i]+2, "b+"))
			{
				InstallUILevel = INSTALLUILEVEL_BASIC|INSTALLUILEVEL_ENDDIALOG;
			}
			else if(msi_strequal(argvW[i]+2, "b-"))
			{
				InstallUILevel = INSTALLUILEVEL_BASIC|INSTALLUILEVEL_PROGRESSONLY;
			}
			else if(msi_strequal(argvW[i]+2, "b+!"))
			{
				InstallUILevel = INSTALLUILEVEL_BASIC|INSTALLUILEVEL_ENDDIALOG;
				WINE_FIXME("Unknown modifier: !\n");
			}
			else
			{
				fprintf(stderr, "Unknown option \"%s\" for UI level\n",
					 wine_dbgstr_w(argvW[i]+2));
			}
		}
		else if(msi_option_equal(argvW[i], "y"))
		{
			FunctionDllRegisterServer = TRUE;
			i++;
			if(i >= argc)
				ShowUsage(1);
			WINE_TRACE("argvW[%d] = %s\n", i, wine_dbgstr_w(argvW[i]));
			DllName = argvW[i];
		}
		else if(msi_option_equal(argvW[i], "z"))
		{
			FunctionDllUnregisterServer = TRUE;
			i++;
			if(i >= argc)
				ShowUsage(1);
			WINE_TRACE("argvW[%d] = %s\n", i, wine_dbgstr_w(argvW[i]));
			DllName = argvW[i];
		}
		else if(msi_option_equal(argvW[i], "h") || msi_option_equal(argvW[i], "?"))
		{
			ShowUsage(0);
		}
		else if(msi_option_equal(argvW[i], "m"))
		{
			FunctionUnknown = TRUE;
			WINE_FIXME("Unknown parameter /m\n");
		}
		else if(msi_option_equal(argvW[i], "D"))
		{
			FunctionUnknown = TRUE;
			WINE_FIXME("Unknown parameter /D\n");
		}
		else if (msi_option_equal(argvW[i], "V"))
		{
		    FunctionServer = TRUE;
		}
		else
			StringListAppend(&property_list, argvW[i]);
	}

	/* start the GUI */
	MsiSetInternalUI(InstallUILevel, NULL);

	Properties = build_properties( property_list );

	if(FunctionInstallAdmin && FunctionPatch)
		FunctionInstall = FALSE;

	ReturnCode = 1;
	if(FunctionInstall)
	{
		if(IsProductCode(PackageName))
			ReturnCode = MsiConfigureProductExW(PackageName, 0, INSTALLSTATE_DEFAULT, Properties);
		else
			ReturnCode = MsiInstallProductW(PackageName, Properties);
	}
	else if(FunctionRepair)
	{
		if(IsProductCode(PackageName))
			WINE_FIXME("Product code treatment not implemented yet\n");
		else
			ReturnCode = MsiReinstallProductW(PackageName, RepairMode);
	}
	else if(FunctionAdvertise)
	{
		LPWSTR Transforms = build_transforms( property_list );
		ReturnCode = MsiAdvertiseProductW(PackageName, (LPWSTR) AdvertiseMode, Transforms, Language);
	}
	else if(FunctionPatch)
	{
		ReturnCode = MsiApplyPatchW(PatchFileName, PackageName, InstallType, Properties);
	}
	else if(FunctionDllRegisterServer)
	{
		ReturnCode = DoDllRegisterServer(DllName);
	}
	else if(FunctionDllUnregisterServer)
	{
		ReturnCode = DoDllUnregisterServer(DllName);
	}
	else if (FunctionRegServer)
	{
		ReturnCode = DoRegServer();
	}
	else if (FunctionUnregServer)
	{
		WINE_FIXME( "/unregserver not implemented yet, ignoring\n" );
	}
	else if (FunctionServer)
	{
	    ReturnCode = DoService();
	}
	else if (FunctionUnknown)
	{
		WINE_FIXME( "Unknown function, ignoring\n" );
	}
	else
		ShowUsage(1);

	return ReturnCode;
}
