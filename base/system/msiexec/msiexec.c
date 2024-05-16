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
#include <commctrl.h>
#include <msi.h>
#include <winsvc.h>
#include <objbase.h>

#include "wine/debug.h"
#include "msiexec_internal.h"

#include "initguid.h"
DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

WINE_DEFAULT_DEBUG_CHANNEL(msiexec);

typedef HRESULT (WINAPI *DLLREGISTERSERVER)(void);
typedef HRESULT (WINAPI *DLLUNREGISTERSERVER)(void);

DWORD DoService(void);
static BOOL silent;

struct string_list
{
	struct string_list *next;
	WCHAR str[1];
};

void report_error(const char* msg, ...)
{
    char buffer[2048];
    va_list va_args;

    va_start(va_args, msg);
    vsnprintf(buffer, sizeof(buffer), msg, va_args);
    va_end(va_args);

    if (silent)
        MESSAGE("%s", buffer);
    else
        MsiMessageBoxA(NULL, buffer, "MsiExec", 0, GetUserDefaultLangID(), 0);
}

static void ShowUsage(int ExitCode)
{
    WCHAR msiexec_version[40];
    WCHAR filename[MAX_PATH];
    LPWSTR msi_res;
    LPWSTR msiexec_help;
    HMODULE hmsi = GetModuleHandleA("msi.dll");
    DWORD len;
    DWORD res;

    /* MsiGetFileVersion need the full path */
    *filename = 0;
    res = GetModuleFileNameW(hmsi, filename, ARRAY_SIZE(filename));
    if (!res)
        WINE_ERR("GetModuleFileName failed: %ld\n", GetLastError());

    len = ARRAY_SIZE(msiexec_version);
    *msiexec_version = 0;
    res = MsiGetFileVersionW(filename, msiexec_version, &len, NULL, NULL);
    if (res)
        WINE_ERR("MsiGetFileVersion failed with %ld\n", res);

    /* Return the length of the resource.
       No typo: The LPWSTR parameter must be a LPWSTR * for this mode */
    len = LoadStringW(hmsi, 10, (LPWSTR) &msi_res, 0);

    msi_res = malloc((len + 1) * sizeof(WCHAR));
    msiexec_help = malloc((len + 1) * sizeof(WCHAR) + sizeof(msiexec_version));
    if (msi_res && msiexec_help) {
        *msi_res = 0;
        LoadStringW(hmsi, 10, msi_res, len + 1);

        swprintf(msiexec_help, len + 1 + ARRAY_SIZE(msiexec_version), msi_res, msiexec_version);
        MsiMessageBoxW(0, msiexec_help, NULL, 0, GetUserDefaultLangID(), 0);
    }
    free(msi_res);
    free(msiexec_help);
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

	entry = malloc(FIELD_OFFSET(struct string_list, str[wcslen(str) + 1]));
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

	ret = malloc(len * sizeof(WCHAR));

	/* add a space before each string, and quote the value */
	p = ret;
	for(list = property_list; list; list = list->next)
	{
		value = wcschr(list->str,'=');
		if(!value)
			continue;
		len = value - list->str;
		*p++ = ' ';
		memcpy(p, list->str, len * sizeof(WCHAR));
		p += len;
		*p++ = '=';

		/* check if the value contains spaces and maybe quote it */
		value++;
		needs_quote = wcschr(value,' ') ? 1 : 0;
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

	ret = malloc(len * sizeof(WCHAR));

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
	strW = malloc(sizeof(WCHAR) * len);
	MultiByteToWideChar( CP_ACP, 0, str2, -1, strW, len);
	ret = CompareStringW(GetThreadLocale(), NORM_IGNORECASE, str1, len, strW, len);
	free(strW);
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
	strW = malloc(sizeof(WCHAR) * len);
	MultiByteToWideChar( CP_ACP, 0, str2, -1, strW, len);
	ret = CompareStringW(GetThreadLocale(), NORM_IGNORECASE, str1, len-1, strW, len-1);
	free(strW);
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
		report_error("Unable to load dll %s\n", wine_dbgstr_w(DllName));
		ExitProcess(1);
	}
	proc = (VOID *) GetProcAddress(*DllHandle, ProcName);
	if(!proc)
	{
		report_error("Dll %s does not implement function %s\n",
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
		report_error("Failed to register dll %s\n", wine_dbgstr_w(DllName));
		return 1;
	}
	MESSAGE("Successfully registered dll %s\n", wine_dbgstr_w(DllName));
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
		report_error("Failed to unregister dll %s\n", wine_dbgstr_w(DllName));
		return 1;
	}
	MESSAGE("Successfully unregistered dll %s\n", wine_dbgstr_w(DllName));
	if(DllHandle)
		FreeLibrary(DllHandle);
	return 0;
}

static DWORD DoRegServer(void)
{
    SC_HANDLE scm, service;
    WCHAR path[MAX_PATH+12];
    DWORD len, ret = 0;

    if (!(scm = OpenSCManagerW(NULL, SERVICES_ACTIVE_DATABASEW, SC_MANAGER_CREATE_SERVICE)))
    {
        report_error("Failed to open the service control manager.\n");
        return 1;
    }
    len = GetSystemDirectoryW(path, MAX_PATH);
    lstrcpyW(path + len, L"\\msiexec /V");
    if ((service = CreateServiceW(scm, L"MSIServer", L"MSIServer", GENERIC_ALL,
                                  SERVICE_WIN32_SHARE_PROCESS, SERVICE_DEMAND_START,
                                  SERVICE_ERROR_NORMAL, path, NULL, NULL, NULL, NULL, NULL)))
    {
        CloseServiceHandle(service);
    }
    else if (GetLastError() != ERROR_SERVICE_EXISTS)
    {
        report_error("Failed to create MSI service\n");
        ret = 1;
    }
    CloseServiceHandle(scm);
    return ret;
}

static DWORD DoUnregServer(void)
{
    SC_HANDLE scm, service;
    DWORD ret = 0;

    if (!(scm = OpenSCManagerW(NULL, SERVICES_ACTIVE_DATABASEW, SC_MANAGER_CONNECT)))
    {
        report_error("Failed to open service control manager\n");
        return 1;
    }
    if ((service = OpenServiceW(scm, L"MSIServer", DELETE)))
    {
        if (!DeleteService(service))
        {
            report_error("Failed to delete MSI service\n");
            ret = 1;
        }
        CloseServiceHandle(service);
    }
    else if (GetLastError() != ERROR_SERVICE_DOES_NOT_EXIST)
    {
        report_error("Failed to open MSI service\n");
        ret = 1;
    }
    CloseServiceHandle(scm);
    return ret;
}

extern UINT CDECL __wine_msi_call_dll_function(DWORD client_pid, const GUID *guid);

static DWORD client_pid;

static DWORD CALLBACK custom_action_thread(void *arg)
{
    GUID guid = *(GUID *)arg;
    free(arg);
    return __wine_msi_call_dll_function(client_pid, &guid);
}

static int custom_action_server(const WCHAR *arg)
{
    GUID guid, *thread_guid;
    DWORD64 thread64;
    WCHAR buffer[24];
    HANDLE thread;
    HANDLE pipe;
    DWORD size;

    TRACE("%s\n", debugstr_w(arg));

    if (!(client_pid = wcstol(arg, NULL, 10)))
    {
        ERR("Invalid parameter %s\n", debugstr_w(arg));
        return 1;
    }

    swprintf(buffer, ARRAY_SIZE(buffer), L"\\\\.\\pipe\\msica_%x_%d", client_pid, sizeof(void *) * 8);
    pipe = CreateFileW(buffer, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (pipe == INVALID_HANDLE_VALUE)
    {
        ERR("Failed to create custom action server pipe: %lu\n", GetLastError());
        return GetLastError();
    }

    /* We need this to unmarshal streams, and some apps expect it to be present. */
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    while (ReadFile(pipe, &guid, sizeof(guid), &size, NULL) && size == sizeof(guid))
    {
        if (IsEqualGUID(&guid, &GUID_NULL))
        {
            /* package closed; time to shut down */
            CoUninitialize();
            return 0;
        }

        thread_guid = malloc(sizeof(GUID));
        memcpy(thread_guid, &guid, sizeof(GUID));
        thread = CreateThread(NULL, 0, custom_action_thread, thread_guid, 0, NULL);

        /* give the thread handle to the client to wait on, since we might have
         * to run a nested action and can't block during this one */
        thread64 = (DWORD_PTR)thread;
        if (!WriteFile(pipe, &thread64, sizeof(thread64), &size, NULL) || size != sizeof(thread64))
        {
            ERR("Failed to write to custom action server pipe: %lu\n", GetLastError());
            CoUninitialize();
            return GetLastError();
        }
    }
    ERR("Failed to read from custom action server pipe: %lu\n", GetLastError());
    CoUninitialize();
    return GetLastError();
}

/*
 * state machine to break up the command line properly
 */

enum chomp_state
{
    CS_WHITESPACE,
    CS_TOKEN,
    CS_QUOTE
};

static int chomp( const WCHAR *in, WCHAR *out )
{
    enum chomp_state state = CS_TOKEN;
    const WCHAR *p;
    int count = 1;
    BOOL ignore;

    for (p = in; *p; p++)
    {
        ignore = TRUE;
        switch (state)
        {
        case CS_WHITESPACE:
            switch (*p)
            {
            case ' ':
                break;
            case '"':
                state = CS_QUOTE;
                count++;
                break;
            default:
                count++;
                ignore = FALSE;
                state = CS_TOKEN;
            }
            break;

        case CS_TOKEN:
            switch (*p)
            {
            case '"':
                state = CS_QUOTE;
                break;
            case ' ':
                state = CS_WHITESPACE;
                if (out) *out++ = 0;
                break;
            default:
                if (p > in && p[-1] == '"')
                {
                    if (out) *out++ = 0;
                    count++;
                }
                ignore = FALSE;
            }
            break;

        case CS_QUOTE:
            switch (*p)
            {
            case '"':
                state = CS_TOKEN;
                break;
            default:
                ignore = FALSE;
            }
            break;
        }
        if (!ignore && out) *out++ = *p;
    }
    if (out) *out = 0;
    return count;
}

static void process_args( WCHAR *cmdline, int *pargc, WCHAR ***pargv )
{
    WCHAR **argv, *p;
    int i, count;

    *pargc = 0;
    *pargv = NULL;

    count = chomp( cmdline, NULL );
    if (!(p = malloc( (wcslen(cmdline) + count + 1) * sizeof(WCHAR) )))
        return;

    count = chomp( cmdline, p );
    if (!(argv = malloc( (count + 1) * sizeof(WCHAR *) )))
    {
        free( p );
        return;
    }
    for (i = 0; i < count; i++)
    {
        argv[i] = p;
        p += lstrlenW( p ) + 1;
    }
    argv[i] = NULL;

    *pargc = count;
    *pargv = argv;
}

static BOOL process_args_from_reg( const WCHAR *ident, int *pargc, WCHAR ***pargv )
{
	LONG r;
	HKEY hkey;
	DWORD sz = 0, type = 0;
	WCHAR *buf;
	BOOL ret = FALSE;

	r = RegOpenKeyW(HKEY_LOCAL_MACHINE,
			L"Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\RunOnceEntries", &hkey);
	if(r != ERROR_SUCCESS)
		return FALSE;
	r = RegQueryValueExW(hkey, ident, 0, &type, 0, &sz);
	if(r == ERROR_SUCCESS && type == REG_SZ)
	{
		int len = lstrlenW( *pargv[0] );
		if (!(buf = malloc( (len + 1) * sizeof(WCHAR) )))
		{
			RegCloseKey( hkey );
			return FALSE;
		}
		memcpy( buf, *pargv[0], len * sizeof(WCHAR) );
		buf[len++] = ' ';
		r = RegQueryValueExW(hkey, ident, 0, &type, (LPBYTE)(buf + len), &sz);
		if( r == ERROR_SUCCESS )
		{
			process_args(buf, pargc, pargv);
			ret = TRUE;
		}
		free(buf);
	}
	RegCloseKey(hkey);
	return ret;
}

static WCHAR *get_path_with_extension(const WCHAR *package_name)
{
    static const WCHAR ext[] = L".msi";
    unsigned int p;
    WCHAR *path;

    if (!(path = malloc(wcslen(package_name) * sizeof(WCHAR) + sizeof(ext))))
    {
        WINE_ERR("No memory.\n");
        return NULL;
    }

    lstrcpyW(path, package_name);
    p = lstrlenW(path);
    while (p && path[p] != '.' && path[p] != L'\\' && path[p] != '/')
        --p;
    if (path[p] == '.')
    {
        free(path);
        return NULL;
    }
    lstrcatW(path, ext);
    return path;
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
	WCHAR *path;

        InitCommonControls();

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
        return custom_action_server(argvW[2]);

	for(i = 1; i < argc; i++)
	{
		WINE_TRACE("argvW[%d] = %s\n", i, wine_dbgstr_w(argvW[i]));

		if (msi_option_equal(argvW[i], "regserver"))
		{
			FunctionRegServer = TRUE;
		}
		else if (msi_option_equal(argvW[i], "unregserver") || msi_option_equal(argvW[i], "unregister")
			||  msi_option_equal(argvW[i], "unreg"))
		{
			FunctionUnregServer = TRUE;
		}
		else if(msi_option_prefix(argvW[i], "i") || msi_option_prefix(argvW[i], "package"))
		{
			LPWSTR argvWi = argvW[i];
			int argLen = (msi_option_prefix(argvW[i], "i") ? 2 : 8);
			FunctionInstall = TRUE;
			if(lstrlenW(argvW[i]) > argLen)
				argvWi += argLen;
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
			StringListAppend(&property_list, L"ACTION=ADMIN");
			WINE_FIXME("Administrative installs are not currently supported\n");
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
						report_error("Unknown option \"%c\" in Repair mode\n", argvW[i][j]);
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
		else if(msi_option_prefix(argvW[i], "x") || msi_option_equal(argvW[i], "uninstall"))
		{
			FunctionInstall = TRUE;
			if(msi_option_prefix(argvW[i], "x")) PackageName = argvW[i]+2;
			if(!PackageName || !PackageName[0])
			{
				i++;
				if (i >= argc)
					ShowUsage(1);
				PackageName = argvW[i];
			}
			WINE_TRACE("PackageName = %s\n", wine_dbgstr_w(PackageName));
			StringListAppend(&property_list, L"REMOVE=ALL");
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
						report_error("Unknown option \"%c\" in Advertise mode\n", argvW[i][j]);
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
				report_error("Logging in %s (0x%08lx, %lu) failed\n",
					     wine_dbgstr_w(LogFileName), LogMode, LogAttributes);
				ExitProcess(1);
			}
		}
		else if(msi_option_equal(argvW[i], "p") || msi_option_equal(argvW[i], "update"))
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
				silent = TRUE;
				InstallUILevel = INSTALLUILEVEL_NONE;
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
			else if(msi_strprefix(argvW[i]+2, "b"))
			{
                const WCHAR *ptr = argvW[i] + 3;

                InstallUILevel = INSTALLUILEVEL_BASIC;

                while (*ptr)
                {
                    if (msi_strprefix(ptr, "+"))
                        InstallUILevel |= INSTALLUILEVEL_ENDDIALOG;
                    if (msi_strprefix(ptr, "-"))
                        InstallUILevel |= INSTALLUILEVEL_PROGRESSONLY;
                    if (msi_strprefix(ptr, "!"))
                    {
                        WINE_FIXME("Unhandled modifier: !\n");
                        InstallUILevel |= INSTALLUILEVEL_HIDECANCEL;
                    }
                    ptr++;
                }
			}
			else
			{
				report_error("Unknown option \"%s\" for UI level\n",
					     wine_dbgstr_w(argvW[i]+2));
			}
		}
                else if(msi_option_equal(argvW[i], "passive"))
                {
                    InstallUILevel = INSTALLUILEVEL_BASIC|INSTALLUILEVEL_PROGRESSONLY|INSTALLUILEVEL_HIDECANCEL;
                    StringListAppend(&property_list, L"REBOOTPROMPT=\"S\"");
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
		else if(msi_option_equal(argvW[i], "help") || msi_option_equal(argvW[i], "?"))
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
		{
			if ((ReturnCode = MsiInstallProductW(PackageName, Properties)) == ERROR_FILE_NOT_FOUND
					&& (path = get_path_with_extension(PackageName)))
			{
				ReturnCode = MsiInstallProductW(path, Properties);
				free(path);
			}
		}
	}
	else if(FunctionRepair)
	{
		if(IsProductCode(PackageName))
			WINE_FIXME("Product code treatment not implemented yet\n");
		else
		{
			if ((ReturnCode = MsiReinstallProductW(PackageName, RepairMode)) == ERROR_FILE_NOT_FOUND
					&& (path = get_path_with_extension(PackageName)))
			{
				ReturnCode = MsiReinstallProductW(path, RepairMode);
				free(path);
			}
		}
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
		ReturnCode = DoUnregServer();
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
