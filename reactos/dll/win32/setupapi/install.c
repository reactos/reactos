/*
 * Setupapi install routines
 *
 * Copyright 2002 Alexandre Julliard for CodeWeavers
 *           2005-2006 Hervé Poussineau (hpoussin@reactos.org)
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

#include "setupapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

/* Unicode constants */
static const WCHAR BackSlash[] = {'\\',0};
static const WCHAR GroupOrderListKey[] = {'S','Y','S','T','E','M','\\','C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\','C','o','n','t','r','o','l','\\','G','r','o','u','p','O','r','d','e','r','L','i','s','t',0};
static const WCHAR InfDirectory[] = {'i','n','f','\\',0};
static const WCHAR OemFileMask[] = {'o','e','m','*','.','i','n','f',0};
static const WCHAR OemFileSpecification[] = {'o','e','m','%','l','u','.','i','n','f',0};

static const WCHAR DependenciesKey[] = {'D','e','p','e','n','d','e','n','c','i','e','s',0};
static const WCHAR DescriptionKey[] = {'D','e','s','c','r','i','p','t','i','o','n',0};
static const WCHAR DisplayNameKey[] = {'D','i','s','p','l','a','y','N','a','m','e',0};
static const WCHAR ErrorControlKey[] = {'E','r','r','o','r','C','o','n','t','r','o','l',0};
static const WCHAR LoadOrderGroupKey[] = {'L','o','a','d','O','r','d','e','r','G','r','o','u','p',0};
static const WCHAR SecurityKey[] = {'S','e','c','u','r','i','t','y',0};
static const WCHAR ServiceBinaryKey[] = {'S','e','r','v','i','c','e','B','i','n','a','r','y',0};
static const WCHAR ServiceTypeKey[] = {'S','e','r','v','i','c','e','T','y','p','e',0};
static const WCHAR StartTypeKey[] = {'S','t','a','r','t','T','y','p','e',0};

static const WCHAR Name[] = {'N','a','m','e',0};
static const WCHAR CmdLine[] = {'C','m','d','L','i','n','e',0};
static const WCHAR SubDir[] = {'S','u','b','D','i','r',0};
static const WCHAR WorkingDir[] = {'W','o','r','k','i','n','g','D','i','r',0};
static const WCHAR IconPath[] = {'I','c','o','n','P','a','t','h',0};
static const WCHAR IconIndex[] = {'I','c','o','n','I','n','d','e','x',0};
static const WCHAR HotKey[] = {'H','o','t','K','e','y',0};
static const WCHAR InfoTip[] = {'I','n','f','o','T','i','p',0};
static const WCHAR DisplayResource[] = {'D','i','s','p','l','a','y','R','e','s','o','u','r','c','e',0};

/* info passed to callback functions dealing with files */
struct files_callback_info
{
    HSPFILEQ queue;
    PCWSTR   src_root;
    UINT     copy_flags;
    HINF     layout;
};

/* info passed to callback functions dealing with the registry */
struct registry_callback_info
{
    HKEY default_root;
    BOOL delete;
};

/* info passed to callback functions dealing with registering dlls */
struct register_dll_info
{
    PSP_FILE_CALLBACK_W callback;
    PVOID               callback_context;
    BOOL                unregister;
};

/* info passed to callback functions dealing with Needs directives */
struct needs_callback_info
{
    UINT type;

    HWND             owner;
    UINT             flags;
    HKEY             key_root;
    LPCWSTR          src_root;
    UINT             copy_flags;
    PVOID            callback;
    PVOID            context;
    HDEVINFO         devinfo;
    PSP_DEVINFO_DATA devinfo_data;
    PVOID            reserved1;
    PVOID            reserved2;
};

typedef BOOL (*iterate_fields_func)( HINF hinf, PCWSTR field, void *arg );
static BOOL GetLineText( HINF hinf, PCWSTR section_name, PCWSTR key_name, PWSTR *value);
typedef HRESULT WINAPI (*COINITIALIZE)(IN LPVOID pvReserved);
typedef HRESULT WINAPI (*COCREATEINSTANCE)(IN REFCLSID rclsid, IN LPUNKNOWN pUnkOuter, IN DWORD dwClsContext, IN REFIID riid, OUT LPVOID *ppv);
typedef HRESULT WINAPI (*COUNINITIALIZE)(VOID);

/* Unicode constants */
static const WCHAR AddService[] = {'A','d','d','S','e','r','v','i','c','e',0};
static const WCHAR CopyFiles[]  = {'C','o','p','y','F','i','l','e','s',0};
static const WCHAR DelFiles[]   = {'D','e','l','F','i','l','e','s',0};
static const WCHAR RenFiles[]   = {'R','e','n','F','i','l','e','s',0};
static const WCHAR Ini2Reg[]    = {'I','n','i','2','R','e','g',0};
static const WCHAR LogConf[]    = {'L','o','g','C','o','n','f',0};
static const WCHAR AddReg[]     = {'A','d','d','R','e','g',0};
static const WCHAR DelReg[]     = {'D','e','l','R','e','g',0};
static const WCHAR BitReg[]     = {'B','i','t','R','e','g',0};
static const WCHAR UpdateInis[] = {'U','p','d','a','t','e','I','n','i','s',0};
static const WCHAR CopyINF[]    = {'C','o','p','y','I','N','F',0};
static const WCHAR UpdateIniFields[] = {'U','p','d','a','t','e','I','n','i','F','i','e','l','d','s',0};
static const WCHAR RegisterDlls[]    = {'R','e','g','i','s','t','e','r','D','l','l','s',0};
static const WCHAR UnregisterDlls[]  = {'U','n','r','e','g','i','s','t','e','r','D','l','l','s',0};
static const WCHAR ProfileItems[]    = {'P','r','o','f','i','l','e','I','t','e','m','s',0};
static const WCHAR Include[]         = {'I','n','c','l','u','d','e',0};
static const WCHAR Needs[]           = {'N','e','e','d','s',0};
static const WCHAR DotSecurity[]     = {'.','S','e','c','u','r','i','t','y',0};
#ifdef __WINESRC__
static const WCHAR WineFakeDlls[]    = {'W','i','n','e','F','a','k','e','D','l','l','s',0};
#endif


/***********************************************************************
 *            get_field_string
 *
 * Retrieve the contents of a field, dynamically growing the buffer if necessary.
 */
static WCHAR *get_field_string( INFCONTEXT *context, DWORD index, WCHAR *buffer,
                                WCHAR *static_buffer, DWORD *size )
{
    DWORD required;

    if (SetupGetStringFieldW( context, index, buffer, *size, &required )) return buffer;
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        /* now grow the buffer */
        if (buffer != static_buffer) HeapFree( GetProcessHeap(), 0, buffer );
        if (!(buffer = HeapAlloc( GetProcessHeap(), 0, required*sizeof(WCHAR) ))) return NULL;
        *size = required;
        if (SetupGetStringFieldW( context, index, buffer, *size, &required )) return buffer;
    }
    if (buffer != static_buffer) HeapFree( GetProcessHeap(), 0, buffer );
    return NULL;
}


/***********************************************************************
 *            copy_files_callback
 *
 * Called once for each CopyFiles entry in a given section.
 */
static BOOL copy_files_callback( HINF hinf, PCWSTR field, void *arg )
{
    struct files_callback_info *info = arg;

    if (field[0] == '@')  /* special case: copy single file */
        SetupQueueDefaultCopyW( info->queue, info->layout ? info->layout : hinf, info->src_root, NULL, field+1, info->copy_flags );
    else
        SetupQueueCopySectionW( info->queue, info->src_root, info->layout ? info->layout : hinf, hinf, field, info->copy_flags );
    return TRUE;
}


/***********************************************************************
 *            delete_files_callback
 *
 * Called once for each DelFiles entry in a given section.
 */
static BOOL delete_files_callback( HINF hinf, PCWSTR field, void *arg )
{
    struct files_callback_info *info = arg;
    SetupQueueDeleteSectionW( info->queue, hinf, 0, field );
    return TRUE;
}


/***********************************************************************
 *            rename_files_callback
 *
 * Called once for each RenFiles entry in a given section.
 */
static BOOL rename_files_callback( HINF hinf, PCWSTR field, void *arg )
{
    struct files_callback_info *info = arg;
    SetupQueueRenameSectionW( info->queue, hinf, 0, field );
    return TRUE;
}


/***********************************************************************
 *            get_root_key
 *
 * Retrieve the registry root key from its name.
 */
static HKEY get_root_key( const WCHAR *name, HKEY def_root )
{
    static const WCHAR HKCR[] = {'H','K','C','R',0};
    static const WCHAR HKCU[] = {'H','K','C','U',0};
    static const WCHAR HKLM[] = {'H','K','L','M',0};
    static const WCHAR HKU[]  = {'H','K','U',0};
    static const WCHAR HKR[]  = {'H','K','R',0};

    if (!strcmpiW( name, HKCR )) return HKEY_CLASSES_ROOT;
    if (!strcmpiW( name, HKCU )) return HKEY_CURRENT_USER;
    if (!strcmpiW( name, HKLM )) return HKEY_LOCAL_MACHINE;
    if (!strcmpiW( name, HKU )) return HKEY_USERS;
    if (!strcmpiW( name, HKR )) return def_root;
    return 0;
}


/***********************************************************************
 *            append_multi_sz_value
 *
 * Append a multisz string to a multisz registry value.
 */
static void append_multi_sz_value( HKEY hkey, const WCHAR *value, const WCHAR *strings,
                                   DWORD str_size )
{
    DWORD size, type, total;
    WCHAR *buffer, *p;

    if (RegQueryValueExW( hkey, value, NULL, &type, NULL, &size )) return;
    if (type != REG_MULTI_SZ) return;

    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, (size + str_size) * sizeof(WCHAR) ))) return;
    if (RegQueryValueExW( hkey, value, NULL, NULL, (BYTE *)buffer, &size )) goto done;

    /* compare each string against all the existing ones */
    total = size;
    while (*strings)
    {
        int len = strlenW(strings) + 1;

        for (p = buffer; *p; p += strlenW(p) + 1)
            if (!strcmpiW( p, strings )) break;

        if (!*p)  /* not found, need to append it */
        {
            memcpy( p, strings, len * sizeof(WCHAR) );
            p[len] = 0;
            total += len;
        }
        strings += len;
    }
    if (total != size)
    {
        TRACE( "setting value %s to %s\n", debugstr_w(value), debugstr_w(buffer) );
        RegSetValueExW( hkey, value, 0, REG_MULTI_SZ, (BYTE *)buffer, total );
    }
 done:
    HeapFree( GetProcessHeap(), 0, buffer );
}


/***********************************************************************
 *            delete_multi_sz_value
 *
 * Remove a string from a multisz registry value.
 */
static void delete_multi_sz_value( HKEY hkey, const WCHAR *value, const WCHAR *string )
{
    DWORD size, type;
    WCHAR *buffer, *src, *dst;

    if (RegQueryValueExW( hkey, value, NULL, &type, NULL, &size )) return;
    if (type != REG_MULTI_SZ) return;
    /* allocate double the size, one for value before and one for after */
    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, size * 2 * sizeof(WCHAR) ))) return;
    if (RegQueryValueExW( hkey, value, NULL, NULL, (BYTE *)buffer, &size )) goto done;
    src = buffer;
    dst = buffer + size;
    while (*src)
    {
        int len = strlenW(src) + 1;
        if (strcmpiW( src, string ))
        {
            memcpy( dst, src, len * sizeof(WCHAR) );
            dst += len;
        }
        src += len;
    }
    *dst++ = 0;
    if (dst != buffer + 2*size)  /* did we remove something? */
    {
        TRACE( "setting value %s to %s\n", debugstr_w(value), debugstr_w(buffer + size) );
        RegSetValueExW( hkey, value, 0, REG_MULTI_SZ,
                        (BYTE *)(buffer + size), dst - (buffer + size) );
    }
 done:
    HeapFree( GetProcessHeap(), 0, buffer );
}


/***********************************************************************
 *            do_reg_operation
 *
 * Perform an add/delete registry operation depending on the flags.
 */
static BOOL do_reg_operation( HKEY hkey, const WCHAR *value, INFCONTEXT *context, INT flags )
{
    DWORD type, size;

    if (flags & (FLG_ADDREG_DELREG_BIT | FLG_ADDREG_DELVAL))  /* deletion */
    {
        if (*value && !(flags & FLG_DELREG_KEYONLY_COMMON))
        {
            if ((flags & FLG_DELREG_MULTI_SZ_DELSTRING) == FLG_DELREG_MULTI_SZ_DELSTRING)
            {
                WCHAR *str;

                if (!SetupGetStringFieldW( context, 5, NULL, 0, &size ) || !size) return TRUE;
                if (!(str = HeapAlloc( GetProcessHeap(), 0, size * sizeof(WCHAR) ))) return FALSE;
                SetupGetStringFieldW( context, 5, str, size, NULL );
                delete_multi_sz_value( hkey, value, str );
                HeapFree( GetProcessHeap(), 0, str );
            }
            else RegDeleteValueW( hkey, value );
        }
        else NtDeleteKey( hkey );
        return TRUE;
    }

    if (flags & (FLG_ADDREG_KEYONLY|FLG_ADDREG_KEYONLY_COMMON)) return TRUE;

    if (flags & (FLG_ADDREG_NOCLOBBER|FLG_ADDREG_OVERWRITEONLY))
    {
        BOOL exists = !RegQueryValueExW( hkey, value, NULL, NULL, NULL, NULL );
        if (exists && (flags & FLG_ADDREG_NOCLOBBER)) return TRUE;
        if (!exists && (flags & FLG_ADDREG_OVERWRITEONLY)) return TRUE;
    }

    switch(flags & FLG_ADDREG_TYPE_MASK)
    {
    case FLG_ADDREG_TYPE_SZ:        type = REG_SZ; break;
    case FLG_ADDREG_TYPE_MULTI_SZ:  type = REG_MULTI_SZ; break;
    case FLG_ADDREG_TYPE_EXPAND_SZ: type = REG_EXPAND_SZ; break;
    case FLG_ADDREG_TYPE_BINARY:    type = REG_BINARY; break;
    case FLG_ADDREG_TYPE_DWORD:     type = REG_DWORD; break;
    case FLG_ADDREG_TYPE_NONE:      type = REG_NONE; break;
    default:                        type = flags >> 16; break;
    }

    if (!(flags & FLG_ADDREG_BINVALUETYPE) ||
        (type == REG_DWORD && SetupGetFieldCount(context) == 5))
    {
        static const WCHAR empty;
        WCHAR *str = NULL;

        if (type == REG_MULTI_SZ)
        {
            if (!SetupGetMultiSzFieldW( context, 5, NULL, 0, &size )) size = 0;
            if (size)
            {
                if (!(str = HeapAlloc( GetProcessHeap(), 0, size * sizeof(WCHAR) ))) return FALSE;
                SetupGetMultiSzFieldW( context, 5, str, size, NULL );
            }
            if (flags & FLG_ADDREG_APPEND)
            {
                if (!str) return TRUE;
                append_multi_sz_value( hkey, value, str, size );
                HeapFree( GetProcessHeap(), 0, str );
                return TRUE;
            }
            /* else fall through to normal string handling */
        }
        else
        {
            if (!SetupGetStringFieldW( context, 5, NULL, 0, &size )) size = 0;
            if (size)
            {
                if (!(str = HeapAlloc( GetProcessHeap(), 0, size * sizeof(WCHAR) ))) return FALSE;
                SetupGetStringFieldW( context, 5, str, size, NULL );
            }
        }

        if (type == REG_DWORD)
        {
            DWORD dw = str ? strtoulW( str, NULL, 0 ) : 0;
            TRACE( "setting dword %s to %x\n", debugstr_w(value), dw );
            RegSetValueExW( hkey, value, 0, type, (BYTE *)&dw, sizeof(dw) );
        }
        else
        {
            TRACE( "setting value %s to %s\n", debugstr_w(value), debugstr_w(str) );
            if (str) RegSetValueExW( hkey, value, 0, type, (BYTE *)str, size * sizeof(WCHAR) );
            else RegSetValueExW( hkey, value, 0, type, (const BYTE *)&empty, sizeof(WCHAR) );
        }
        HeapFree( GetProcessHeap(), 0, str );
        return TRUE;
    }
    else  /* get the binary data */
    {
        BYTE *data = NULL;

        if (!SetupGetBinaryField( context, 5, NULL, 0, &size )) size = 0;
        if (size)
        {
            if (!(data = HeapAlloc( GetProcessHeap(), 0, size ))) return FALSE;
            TRACE( "setting binary data %s len %d\n", debugstr_w(value), size );
            SetupGetBinaryField( context, 5, data, size, NULL );
        }
        RegSetValueExW( hkey, value, 0, type, data, size );
        HeapFree( GetProcessHeap(), 0, data );
        return TRUE;
    }
}


/***********************************************************************
 *            registry_callback
 *
 * Called once for each AddReg and DelReg entry in a given section.
 */
static BOOL registry_callback( HINF hinf, PCWSTR field, void *arg )
{
    struct registry_callback_info *info = arg;
    LPWSTR security_key, security_descriptor;
    INFCONTEXT context, security_context;
    PSECURITY_DESCRIPTOR sd = NULL;
    SECURITY_ATTRIBUTES security_attributes = { 0, };
    HKEY root_key, hkey;
    DWORD required;

    BOOL ok = SetupFindFirstLineW( hinf, field, NULL, &context );
    if (!ok)
        return TRUE;

    /* Check for .Security section */
    security_key = MyMalloc( (strlenW( field ) + strlenW( DotSecurity )) * sizeof(WCHAR) + sizeof(UNICODE_NULL) );
    if (!security_key)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    strcpyW( security_key, field );
    strcatW( security_key, DotSecurity );
    ok = SetupFindFirstLineW( hinf, security_key, NULL, &security_context );
    MyFree(security_key);
    if (ok)
    {
        if (!SetupGetLineTextW( &security_context, NULL, NULL, NULL, NULL, 0, &required ))
            return FALSE;
        security_descriptor = MyMalloc( required * sizeof(WCHAR) );
        if (!security_descriptor)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
        if (!SetupGetLineTextW( &security_context, NULL, NULL, NULL, security_descriptor, required, NULL ))
            return FALSE;
        ok = ConvertStringSecurityDescriptorToSecurityDescriptorW( security_descriptor, SDDL_REVISION_1, &sd, NULL );
        MyFree( security_descriptor );
        if (!ok)
            return FALSE;
        security_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
        security_attributes.lpSecurityDescriptor = sd;
    }

    for (ok = TRUE; ok; ok = SetupFindNextLine( &context, &context ))
    {
        WCHAR buffer[MAX_INF_STRING_LENGTH];
        INT flags;

        /* get root */
        if (!SetupGetStringFieldW( &context, 1, buffer, sizeof(buffer)/sizeof(WCHAR), NULL ))
            continue;
        if (!(root_key = get_root_key( buffer, info->default_root )))
            continue;

        /* get key */
        if (!SetupGetStringFieldW( &context, 2, buffer, sizeof(buffer)/sizeof(WCHAR), NULL ))
            *buffer = 0;

        /* get flags */
        if (!SetupGetIntField( &context, 4, &flags )) flags = 0;

        if (!info->delete)
        {
            if (flags & FLG_ADDREG_DELREG_BIT) continue;  /* ignore this entry */
        }
        else
        {
            if (!flags) flags = FLG_ADDREG_DELREG_BIT;
            else if (!(flags & FLG_ADDREG_DELREG_BIT)) continue;  /* ignore this entry */
        }

        if (info->delete || (flags & FLG_ADDREG_OVERWRITEONLY))
        {
            if (RegOpenKeyW( root_key, buffer, &hkey )) continue;  /* ignore if it doesn't exist */
        }
        else if (RegCreateKeyExW( root_key, buffer, 0, NULL, 0, MAXIMUM_ALLOWED,
            sd ? &security_attributes : NULL, &hkey, NULL ))
        {
            ERR( "could not create key %p %s\n", root_key, debugstr_w(buffer) );
            continue;
        }
        TRACE( "key %p %s\n", root_key, debugstr_w(buffer) );

        /* get value name */
        if (!SetupGetStringFieldW( &context, 3, buffer, sizeof(buffer)/sizeof(WCHAR), NULL ))
            *buffer = 0;

        /* and now do it */
        if (!do_reg_operation( hkey, buffer, &context, flags ))
        {
            if (hkey != root_key) RegCloseKey( hkey );
            if (sd) LocalFree( sd );
            return FALSE;
        }
        if (hkey != root_key) RegCloseKey( hkey );
    }
    if (sd) LocalFree( sd );
    return TRUE;
}


/***********************************************************************
 *            do_register_dll
 *
 * Register or unregister a dll.
 */
static BOOL do_register_dll( const struct register_dll_info *info, const WCHAR *path,
                             INT flags, INT timeout, const WCHAR *args )
{
    HMODULE module;
    HRESULT res;
    SP_REGISTER_CONTROL_STATUSW status;
#ifdef __WINESRC__
    IMAGE_NT_HEADERS *nt;
#endif

    status.cbSize = sizeof(status);
    status.FileName = path;
    status.FailureCode = SPREG_SUCCESS;
    status.Win32Error = ERROR_SUCCESS;

    if (info->callback)
    {
        switch(info->callback( info->callback_context, SPFILENOTIFY_STARTREGISTRATION,
                               (UINT_PTR)&status, !info->unregister ))
        {
        case FILEOP_ABORT:
            SetLastError( ERROR_OPERATION_ABORTED );
            return FALSE;
        case FILEOP_SKIP:
            return TRUE;
        case FILEOP_DOIT:
            break;
        }
    }

    if (!(module = LoadLibraryExW( path, 0, LOAD_WITH_ALTERED_SEARCH_PATH )))
    {
        WARN( "could not load %s\n", debugstr_w(path) );
        status.FailureCode = SPREG_LOADLIBRARY;
        status.Win32Error = GetLastError();
        goto done;
    }

#ifdef __WINESRC__
    if ((nt = RtlImageNtHeader( module )) && !(nt->FileHeader.Characteristics & IMAGE_FILE_DLL))
    {
        /* file is an executable, not a dll */
        STARTUPINFOW startup;
        PROCESS_INFORMATION info;
        WCHAR *cmd_line;
        BOOL res;
        static const WCHAR format[] = {'"','%','s','"',' ','%','s',0};
        static const WCHAR default_args[] = {'/','R','e','g','S','e','r','v','e','r',0};

        FreeLibrary( module );
        module = NULL;
        if (!args) args = default_args;
        cmd_line = HeapAlloc( GetProcessHeap(), 0, (strlenW(path) + strlenW(args) + 4) * sizeof(WCHAR) );
        sprintfW( cmd_line, format, path, args );
        memset( &startup, 0, sizeof(startup) );
        startup.cb = sizeof(startup);
        TRACE( "executing %s\n", debugstr_w(cmd_line) );
        res = CreateProcessW( NULL, cmd_line, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &info );
        HeapFree( GetProcessHeap(), 0, cmd_line );
        if (!res)
        {
            status.FailureCode = SPREG_LOADLIBRARY;
            status.Win32Error = GetLastError();
            goto done;
        }
        CloseHandle( info.hThread );

        if (WaitForSingleObject( info.hProcess, timeout*1000 ) == WAIT_TIMEOUT)
        {
            /* timed out, kill the process */
            TerminateProcess( info.hProcess, 1 );
            status.FailureCode = SPREG_TIMEOUT;
            status.Win32Error = ERROR_TIMEOUT;
        }
        CloseHandle( info.hProcess );
        goto done;
    }
#endif // __WINESRC__

    if (flags & FLG_REGSVR_DLLREGISTER)
    {
        const char *entry_point = info->unregister ? "DllUnregisterServer" : "DllRegisterServer";
        HRESULT (WINAPI *func)(void) = (void *)GetProcAddress( module, entry_point );

        if (!func)
        {
            status.FailureCode = SPREG_GETPROCADDR;
            status.Win32Error = GetLastError();
            goto done;
        }

        TRACE( "calling %s in %s\n", entry_point, debugstr_w(path) );
        res = func();

        if (FAILED(res))
        {
            WARN( "calling %s in %s returned error %x\n", entry_point, debugstr_w(path), res );
            status.FailureCode = SPREG_REGSVR;
            status.Win32Error = res;
            goto done;
        }
    }

    if (flags & FLG_REGSVR_DLLINSTALL)
    {
        HRESULT (WINAPI *func)(BOOL,LPCWSTR) = (void *)GetProcAddress( module, "DllInstall" );

        if (!func)
        {
            status.FailureCode = SPREG_GETPROCADDR;
            status.Win32Error = GetLastError();
            goto done;
        }

        TRACE( "calling DllInstall(%d,%s) in %s\n",
               !info->unregister, debugstr_w(args), debugstr_w(path) );
        res = func( !info->unregister, args );

        if (FAILED(res))
        {
            WARN( "calling DllInstall in %s returned error %x\n", debugstr_w(path), res );
            status.FailureCode = SPREG_REGSVR;
            status.Win32Error = res;
            goto done;
        }
    }

done:
    if (module) FreeLibrary( module );
    if (info->callback) info->callback( info->callback_context, SPFILENOTIFY_ENDREGISTRATION,
                                        (UINT_PTR)&status, !info->unregister );
    return TRUE;
}


/***********************************************************************
 *            register_dlls_callback
 *
 * Called once for each RegisterDlls entry in a given section.
 */
static BOOL register_dlls_callback( HINF hinf, PCWSTR field, void *arg )
{
    struct register_dll_info *info = arg;
    INFCONTEXT context;
    BOOL ret = TRUE;
    BOOL ok = SetupFindFirstLineW( hinf, field, NULL, &context );

    for (; ok; ok = SetupFindNextLine( &context, &context ))
    {
        WCHAR *path, *args, *p;
        WCHAR buffer[MAX_INF_STRING_LENGTH];
        INT flags, timeout;

        /* get directory */
        if (!(path = PARSER_get_dest_dir( &context ))) continue;

        /* get dll name */
        if (!SetupGetStringFieldW( &context, 3, buffer, sizeof(buffer)/sizeof(WCHAR), NULL ))
            goto done;
        if (!(p = HeapReAlloc( GetProcessHeap(), 0, path,
                               (strlenW(path) + strlenW(buffer) + 2) * sizeof(WCHAR) ))) goto done;
        path = p;
        p += strlenW(p);
        if (p == path || p[-1] != '\\') *p++ = '\\';
        strcpyW( p, buffer );

        /* get flags */
        if (!SetupGetIntField( &context, 4, &flags )) flags = 0;

        /* get timeout */
        if (!SetupGetIntField( &context, 5, &timeout )) timeout = 60;

        /* get command line */
        args = NULL;
        if (SetupGetStringFieldW( &context, 6, buffer, sizeof(buffer)/sizeof(WCHAR), NULL ))
            args = buffer;

        ret = do_register_dll( info, path, flags, timeout, args );

    done:
        HeapFree( GetProcessHeap(), 0, path );
        if (!ret) break;
    }
    return ret;
}

#ifdef __WINESRC__
/***********************************************************************
 *            fake_dlls_callback
 *
 * Called once for each WineFakeDlls entry in a given section.
 */
static BOOL fake_dlls_callback( HINF hinf, PCWSTR field, void *arg )
{
    INFCONTEXT context;
    BOOL ret = TRUE;
    BOOL ok = SetupFindFirstLineW( hinf, field, NULL, &context );

    for (; ok; ok = SetupFindNextLine( &context, &context ))
    {
        WCHAR *path, *p;
        WCHAR buffer[MAX_INF_STRING_LENGTH];

        /* get directory */
        if (!(path = PARSER_get_dest_dir( &context ))) continue;

        /* get dll name */
        if (!SetupGetStringFieldW( &context, 3, buffer, sizeof(buffer)/sizeof(WCHAR), NULL ))
            goto done;
        if (!(p = HeapReAlloc( GetProcessHeap(), 0, path,
                               (strlenW(path) + strlenW(buffer) + 2) * sizeof(WCHAR) ))) goto done;
        path = p;
        p += strlenW(p);
        if (p == path || p[-1] != '\\') *p++ = '\\';
        strcpyW( p, buffer );

        /* get source dll */
        if (SetupGetStringFieldW( &context, 4, buffer, sizeof(buffer)/sizeof(WCHAR), NULL ))
            p = buffer;  /* otherwise use target base name as default source */

        create_fake_dll( path, p );  /* ignore errors */

    done:
        HeapFree( GetProcessHeap(), 0, path );
        if (!ret) break;
    }
    return ret;
}
#endif // __WINESRC__

/***********************************************************************
 *            update_ini_callback
 *
 * Called once for each UpdateInis entry in a given section.
 */
static BOOL update_ini_callback( HINF hinf, PCWSTR field, void *arg )
{
    INFCONTEXT context;

    BOOL ok = SetupFindFirstLineW( hinf, field, NULL, &context );

    for (; ok; ok = SetupFindNextLine( &context, &context ))
    {
        WCHAR buffer[MAX_INF_STRING_LENGTH];
        WCHAR  filename[MAX_INF_STRING_LENGTH];
        WCHAR  section[MAX_INF_STRING_LENGTH];
        WCHAR  entry[MAX_INF_STRING_LENGTH];
        WCHAR  string[MAX_INF_STRING_LENGTH];
        LPWSTR divider;

        if (!SetupGetStringFieldW( &context, 1, filename,
                                   sizeof(filename)/sizeof(WCHAR), NULL ))
            continue;

        if (!SetupGetStringFieldW( &context, 2, section,
                                   sizeof(section)/sizeof(WCHAR), NULL ))
            continue;

        if (!SetupGetStringFieldW( &context, 4, buffer,
                                   sizeof(buffer)/sizeof(WCHAR), NULL ))
            continue;

        divider = strchrW(buffer,'=');
        if (divider)
        {
            *divider = 0;
            strcpyW(entry,buffer);
            divider++;
            strcpyW(string,divider);
        }
        else
        {
            strcpyW(entry,buffer);
            string[0]=0;
        }

        TRACE("Writing %s = %s in %s of file %s\n",debugstr_w(entry),
               debugstr_w(string),debugstr_w(section),debugstr_w(filename));
        WritePrivateProfileStringW(section,entry,string,filename);

    }
    return TRUE;
}

static BOOL update_ini_fields_callback( HINF hinf, PCWSTR field, void *arg )
{
    FIXME( "should update ini fields %s\n", debugstr_w(field) );
    return TRUE;
}

static BOOL ini2reg_callback( HINF hinf, PCWSTR field, void *arg )
{
    FIXME( "should do ini2reg %s\n", debugstr_w(field) );
    return TRUE;
}

static BOOL logconf_callback( HINF hinf, PCWSTR field, void *arg )
{
    FIXME( "should do logconf %s\n", debugstr_w(field) );
    return TRUE;
}

static BOOL bitreg_callback( HINF hinf, PCWSTR field, void *arg )
{
    FIXME( "should do bitreg %s\n", debugstr_w(field) );
    return TRUE;
}

static BOOL Concatenate(int DirId, LPCWSTR SubDirPart, LPCWSTR NamePart, LPWSTR *pFullName)
{
    DWORD dwRequired = 0;
    LPCWSTR Dir;
    LPWSTR FullName;

    *pFullName = NULL;

    Dir = DIRID_get_string(DirId);
    if (Dir)
        dwRequired += wcslen(Dir) + 1;
    if (SubDirPart)
        dwRequired += wcslen(SubDirPart) + 1;
    if (NamePart)
        dwRequired += wcslen(NamePart);
    dwRequired = dwRequired * sizeof(WCHAR) + sizeof(UNICODE_NULL);

    FullName = MyMalloc(dwRequired);
    if (!FullName)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    FullName[0] = UNICODE_NULL;

    if (Dir)
    {
        wcscat(FullName, Dir);
        if (FullName[wcslen(FullName) - 1] != '\\')
            wcscat(FullName, BackSlash);
    }
    if (SubDirPart)
    {
        wcscat(FullName, SubDirPart);
        if (FullName[wcslen(FullName) - 1] != '\\')
            wcscat(FullName, BackSlash);
    }
    if (NamePart)
        wcscat(FullName, NamePart);

    *pFullName = FullName;
    return TRUE;
}

/***********************************************************************
 *            profile_items_callback
 *
 * Called once for each ProfileItems entry in a given section.
 */
static BOOL
profile_items_callback(
    IN HINF hInf,
    IN PCWSTR SectionName,
    IN PVOID Arg)
{
    INFCONTEXT Context;
    LPWSTR LinkSubDir = NULL, LinkName = NULL;
    INT LinkAttributes = 0;
    INT FileDirId = 0;
    LPWSTR FileSubDir = NULL;
    INT DirId = 0;
    LPWSTR SubDirPart = NULL, NamePart = NULL;
    LPWSTR FullLinkName = NULL, FullFileName = NULL, FullWorkingDir = NULL, FullIconName = NULL;
    INT IconIdx = 0;
    LPWSTR lpHotKey = NULL, lpInfoTip = NULL;
    LPWSTR DisplayName = NULL;
    INT DisplayResId = 0;
    BOOL ret = FALSE;
    DWORD Index, Required;

    IShellLinkW *psl;
    IPersistFile *ppf;
    HMODULE hOle32 = NULL;
    COINITIALIZE pCoInitialize;
    COCREATEINSTANCE pCoCreateInstance;
    COUNINITIALIZE pCoUninitialize;
    HRESULT hr;

    TRACE("hInf %p, SectionName %s, Arg %p\n",
        hInf, debugstr_w(SectionName), Arg);

    /* Read 'Name' entry */
    if (!SetupFindFirstLineW(hInf, SectionName, Name, &Context))
        goto cleanup;
    if (!GetStringField(&Context, 1, &LinkName))
        goto cleanup;
    if (SetupGetFieldCount(&Context) >= 2)
    {
        if (!SetupGetIntField(&Context, 2, &LinkAttributes))
            goto cleanup;
    }

    /* Read 'CmdLine' entry */
    if (!SetupFindFirstLineW(hInf, SectionName, CmdLine, &Context))
        goto cleanup;
    Index = 1;
    if (!SetupGetIntField(&Context, Index++, &FileDirId))
        goto cleanup;
    if (SetupGetFieldCount(&Context) >= 3)
    {
        if (!GetStringField(&Context, Index++, &FileSubDir))
            goto cleanup;
    }
    if (!GetStringField(&Context, Index++, &NamePart))
        goto cleanup;
    if (!Concatenate(FileDirId, FileSubDir, NamePart, &FullFileName))
        goto cleanup;
    MyFree(NamePart);
    NamePart = NULL;

    /* Read 'SubDir' entry */
    if ((LinkAttributes & FLG_PROFITEM_GROUP) == 0 && SetupFindFirstLineW(hInf, SectionName, SubDir, &Context))
    {
        if (!GetStringField(&Context, 1, &LinkSubDir))
            goto cleanup;
    }

    /* Read 'WorkingDir' entry */
    if (SetupFindFirstLineW(hInf, SectionName, WorkingDir, &Context))
    {
        if (!SetupGetIntField(&Context, 1, &DirId))
            goto cleanup;
        if (SetupGetFieldCount(&Context) >= 2)
        {
            if (!GetStringField(&Context, 2, &SubDirPart))
                goto cleanup;
        }
        if (!Concatenate(DirId, SubDirPart, NULL, &FullWorkingDir))
            goto cleanup;
        MyFree(SubDirPart);
        SubDirPart = NULL;
    }
    else
    {
        if (!Concatenate(FileDirId, FileSubDir, NULL, &FullWorkingDir))
            goto cleanup;
    }

    /* Read 'IconPath' entry */
    if (SetupFindFirstLineW(hInf, SectionName, IconPath, &Context))
    {
        Index = 1;
        if (!SetupGetIntField(&Context, Index++, &DirId))
            goto cleanup;
        if (SetupGetFieldCount(&Context) >= 3)
        {
            if (!GetStringField(&Context, Index++, &SubDirPart))
                goto cleanup;
        }
        if (!GetStringField(&Context, Index, &NamePart))
            goto cleanup;
        if (!Concatenate(DirId, SubDirPart, NamePart, &FullIconName))
            goto cleanup;
        MyFree(SubDirPart);
        MyFree(NamePart);
        SubDirPart = NamePart = NULL;
    }
    else
    {
        FullIconName = DuplicateString(FullFileName);
        if (!FullIconName)
            goto cleanup;
    }

    /* Read 'IconIndex' entry */
    if (SetupFindFirstLineW(hInf, SectionName, IconIndex, &Context))
    {
        if (!SetupGetIntField(&Context, 1, &IconIdx))
            goto cleanup;
    }

    /* Read 'HotKey' and 'InfoTip' entries */
    GetLineText(hInf, SectionName, HotKey, &lpHotKey);
    GetLineText(hInf, SectionName, InfoTip, &lpInfoTip);

    /* Read 'DisplayResource' entry */
    if (SetupFindFirstLineW(hInf, SectionName, DisplayResource, &Context))
    {
        if (!GetStringField(&Context, 1, &DisplayName))
            goto cleanup;
        if (!SetupGetIntField(&Context, 2, &DisplayResId))
            goto cleanup;
    }

    /* Some debug */
    TRACE("Link is %s\\%s, attributes 0x%x\n", debugstr_w(LinkSubDir), debugstr_w(LinkName), LinkAttributes);
    TRACE("File is %s\n", debugstr_w(FullFileName));
    TRACE("Working dir %s\n", debugstr_w(FullWorkingDir));
    TRACE("Icon is %s, %d\n", debugstr_w(FullIconName), IconIdx);
    TRACE("Hotkey %s\n", debugstr_w(lpHotKey));
    TRACE("InfoTip %s\n", debugstr_w(lpInfoTip));
    TRACE("Display %s, %d\n", DisplayName, DisplayResId);

    /* Load ole32.dll */
    hOle32 = LoadLibraryA("ole32.dll");
    if (!hOle32)
        goto cleanup;
    pCoInitialize = (COINITIALIZE)GetProcAddress(hOle32, "CoInitialize");
    if (!pCoInitialize)
        goto cleanup;
    pCoCreateInstance = (COCREATEINSTANCE)GetProcAddress(hOle32, "CoCreateInstance");
    if (!pCoCreateInstance)
        goto cleanup;
    pCoUninitialize = (COUNINITIALIZE)GetProcAddress(hOle32, "CoUninitialize");
    if (!pCoUninitialize)
        goto cleanup;

    /* Create shortcut */
    hr = pCoInitialize(NULL);
    if (!SUCCEEDED(hr))
    {
        if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
            SetLastError(HRESULT_CODE(hr));
        else
            SetLastError(E_FAIL);
        goto cleanup;
    }
    hr = pCoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkW, (LPVOID*)&psl);
    if (SUCCEEDED(hr))
    {
        /* Fill link properties */
        if (SUCCEEDED(hr))
            hr = IShellLinkW_SetPath(psl, FullFileName);
        if (SUCCEEDED(hr))
            hr = IShellLinkW_SetWorkingDirectory(psl, FullWorkingDir);
        if (SUCCEEDED(hr))
            hr = IShellLinkW_SetIconLocation(psl, FullIconName, IconIdx);
        if (SUCCEEDED(hr) && lpHotKey)
            FIXME("Need to store hotkey %s in shell link\n", debugstr_w(lpHotKey));
        if (SUCCEEDED(hr) && lpInfoTip)
            hr = IShellLinkW_SetDescription(psl, lpInfoTip);
        if (SUCCEEDED(hr) && DisplayName)
            FIXME("Need to store display name %s, %d in shell link\n", debugstr_w(DisplayName), DisplayResId);
        if (SUCCEEDED(hr))
        {
            hr = IShellLinkW_QueryInterface(psl, &IID_IPersistFile, (LPVOID*)&ppf);
            if (SUCCEEDED(hr))
            {
                Required = (MAX_PATH + wcslen(LinkSubDir) + 1 + wcslen(LinkName)) * sizeof(WCHAR);
                FullLinkName = MyMalloc(Required);
                if (!FullLinkName)
                    hr = E_OUTOFMEMORY;
                else
                {
                    if (LinkAttributes & (FLG_PROFITEM_DELETE | FLG_PROFITEM_GROUP))
                        FIXME("Need to handle FLG_PROFITEM_DELETE and FLG_PROFITEM_GROUP\n");
                    if (SHGetSpecialFolderPathW(
                        NULL,
                        FullLinkName,
                        LinkAttributes & FLG_PROFITEM_CURRENTUSER ? CSIDL_PROGRAMS : CSIDL_COMMON_PROGRAMS,
                        TRUE))
                    {
                        if (FullLinkName[wcslen(FullLinkName) - 1] != '\\')
                            wcscat(FullLinkName, BackSlash);
                        if (LinkSubDir)
                        {
                            wcscat(FullLinkName, LinkSubDir);
                            if (FullLinkName[wcslen(FullLinkName) - 1] != '\\')
                                wcscat(FullLinkName, BackSlash);
                        }
                        wcscat(FullLinkName, LinkName);
                        hr = IPersistFile_Save(ppf, FullLinkName, TRUE);
                    }
                    else
                        hr = HRESULT_FROM_WIN32(GetLastError());
                }
                IPersistFile_Release(ppf);
            }
        }
        IShellLinkW_Release(psl);
    }
    pCoUninitialize();
    if (SUCCEEDED(hr))
        ret = TRUE;
    else
    {
        if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
            SetLastError(HRESULT_CODE(hr));
        else
            SetLastError(E_FAIL);
    }

cleanup:
    MyFree(LinkSubDir);
    MyFree(LinkName);
    MyFree(FileSubDir);
    MyFree(SubDirPart);
    MyFree(NamePart);
    MyFree(FullFileName);
    MyFree(FullWorkingDir);
    MyFree(FullIconName);
    MyFree(FullLinkName);
    MyFree(lpHotKey);
    MyFree(lpInfoTip);
    MyFree(DisplayName);
    if (hOle32)
        FreeLibrary(hOle32);

    TRACE("Returning %d\n", ret);
    return ret;
}

static BOOL copy_inf_callback( HINF hinf, PCWSTR field, void *arg )
{
    FIXME( "should do copy inf %s\n", debugstr_w(field) );
    return TRUE;
}


/***********************************************************************
 *            iterate_section_fields
 *
 * Iterate over all fields of a certain key of a certain section
 */
static BOOL iterate_section_fields( HINF hinf, PCWSTR section, PCWSTR key,
                                    iterate_fields_func callback, void *arg )
{
    WCHAR static_buffer[200];
    WCHAR *buffer = static_buffer;
    DWORD size = sizeof(static_buffer)/sizeof(WCHAR);
    INFCONTEXT context;
    BOOL ret = FALSE;

    BOOL ok = SetupFindFirstLineW( hinf, section, key, &context );
    while (ok)
    {
        UINT i, count = SetupGetFieldCount( &context );
        for (i = 1; i <= count; i++)
        {
            if (!(buffer = get_field_string( &context, i, buffer, static_buffer, &size )))
                goto done;
            if (!callback( hinf, buffer, arg ))
            {
                WARN("callback failed for %s %s err %d\n",
                     debugstr_w(section), debugstr_w(buffer), GetLastError() );
                goto done;
            }
        }
        ok = SetupFindNextMatchLineW( &context, key, &context );
    }
    ret = TRUE;
 done:
    if (buffer != static_buffer) HeapFree( GetProcessHeap(), 0, buffer );
    return ret;
}


/***********************************************************************
 *            SetupInstallFilesFromInfSectionA   (SETUPAPI.@)
 */
BOOL WINAPI SetupInstallFilesFromInfSectionA( HINF hinf, HINF hlayout, HSPFILEQ queue,
                                              PCSTR section, PCSTR src_root, UINT flags )
{
    UNICODE_STRING sectionW;
    BOOL ret = FALSE;

    if (!RtlCreateUnicodeStringFromAsciiz( &sectionW, section ))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }
    if (!src_root)
        ret = SetupInstallFilesFromInfSectionW( hinf, hlayout, queue, sectionW.Buffer,
                                                NULL, flags );
    else
    {
        UNICODE_STRING srcW;
        if (RtlCreateUnicodeStringFromAsciiz( &srcW, src_root ))
        {
            ret = SetupInstallFilesFromInfSectionW( hinf, hlayout, queue, sectionW.Buffer,
                                                    srcW.Buffer, flags );
            RtlFreeUnicodeString( &srcW );
        }
        else SetLastError( ERROR_NOT_ENOUGH_MEMORY );
    }
    RtlFreeUnicodeString( &sectionW );
    return ret;
}


/***********************************************************************
 *            SetupInstallFilesFromInfSectionW   (SETUPAPI.@)
 */
BOOL WINAPI SetupInstallFilesFromInfSectionW( HINF hinf, HINF hlayout, HSPFILEQ queue,
                                              PCWSTR section, PCWSTR src_root, UINT flags )
{
    struct files_callback_info info;

    info.queue      = queue;
    info.src_root   = src_root;
    info.copy_flags = flags;
    info.layout     = hlayout;
    return iterate_section_fields( hinf, section, CopyFiles, copy_files_callback, &info );
}


/***********************************************************************
 *            SetupInstallFromInfSectionA   (SETUPAPI.@)
 */
BOOL WINAPI SetupInstallFromInfSectionA( HWND owner, HINF hinf, PCSTR section, UINT flags,
                                         HKEY key_root, PCSTR src_root, UINT copy_flags,
                                         PSP_FILE_CALLBACK_A callback, PVOID context,
                                         HDEVINFO devinfo, PSP_DEVINFO_DATA devinfo_data )
{
    UNICODE_STRING sectionW, src_rootW;
    struct callback_WtoA_context ctx;
    BOOL ret = FALSE;

    src_rootW.Buffer = NULL;
    if (src_root && !RtlCreateUnicodeStringFromAsciiz( &src_rootW, src_root ))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    if (RtlCreateUnicodeStringFromAsciiz( &sectionW, section ))
    {
        ctx.orig_context = context;
        ctx.orig_handler = callback;
        ret = SetupInstallFromInfSectionW( owner, hinf, sectionW.Buffer, flags, key_root,
                                           src_rootW.Buffer, copy_flags, QUEUE_callback_WtoA,
                                           &ctx, devinfo, devinfo_data );
        RtlFreeUnicodeString( &sectionW );
    }
    else SetLastError( ERROR_NOT_ENOUGH_MEMORY );

    RtlFreeUnicodeString( &src_rootW );
    return ret;
}


/***********************************************************************
 *            include_callback
 *
 * Called once for each Include entry in a given section.
 */
static BOOL include_callback( HINF hinf, PCWSTR field, void *arg )
{
    return SetupOpenAppendInfFileW( field, hinf, NULL );
}


/***********************************************************************
 *            needs_callback
 *
 * Called once for each Needs entry in a given section.
 */
static BOOL needs_callback( HINF hinf, PCWSTR field, void *arg )
{
    struct needs_callback_info *info = arg;

    switch (info->type)
    {
        case 0:
            return SetupInstallFromInfSectionW(info->owner, *(HINF*)hinf, field, info->flags,
               info->key_root, info->src_root, info->copy_flags, info->callback,
               info->context, info->devinfo, info->devinfo_data);
        case 1:
            return SetupInstallServicesFromInfSectionExW(*(HINF*)hinf, field, info->flags,
                info->devinfo, info->devinfo_data, info->reserved1, info->reserved2);
        default:
            ERR("Unknown info type %u\n", info->type);
            return FALSE;
    }
}


/***********************************************************************
 *            SetupInstallFromInfSectionW   (SETUPAPI.@)
 */
BOOL WINAPI SetupInstallFromInfSectionW( HWND owner, HINF hinf, PCWSTR section, UINT flags,
                                         HKEY key_root, PCWSTR src_root, UINT copy_flags,
                                         PSP_FILE_CALLBACK_W callback, PVOID context,
                                         HDEVINFO devinfo, PSP_DEVINFO_DATA devinfo_data )
{
    struct needs_callback_info needs_info;

    /* Parse 'Include' and 'Needs' directives */
    iterate_section_fields( hinf, section, Include, include_callback, NULL);
    needs_info.type = 0;
    needs_info.owner = owner;
    needs_info.flags = flags;
    needs_info.key_root = key_root;
    needs_info.src_root = src_root;
    needs_info.copy_flags = copy_flags;
    needs_info.callback = callback;
    needs_info.context = context;
    needs_info.devinfo = devinfo;
    needs_info.devinfo_data = devinfo_data;
    iterate_section_fields( hinf, section, Needs, needs_callback, &needs_info);

    if (flags & SPINST_FILES)
    {
        SP_DEVINSTALL_PARAMS_W install_params;
        struct files_callback_info info;
        HSPFILEQ queue = NULL;
        BOOL use_custom_queue;
        BOOL ret;

        install_params.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
        use_custom_queue = SetupDiGetDeviceInstallParamsW(devinfo, devinfo_data, &install_params) && (install_params.Flags & DI_NOVCP);
        if (!use_custom_queue && ((queue = SetupOpenFileQueue()) == (HSPFILEQ)INVALID_HANDLE_VALUE ))
            return FALSE;
        info.queue      = use_custom_queue ? install_params.FileQueue : queue;
        info.src_root   = src_root;
        info.copy_flags = copy_flags;
        info.layout     = hinf;
        ret = (iterate_section_fields( hinf, section, CopyFiles, copy_files_callback, &info ) &&
               iterate_section_fields( hinf, section, DelFiles, delete_files_callback, &info ) &&
               iterate_section_fields( hinf, section, RenFiles, rename_files_callback, &info ));
        if (!use_custom_queue)
        {
            if (ret)
                ret = SetupCommitFileQueueW( owner, queue, callback, context );
            SetupCloseFileQueue( queue );
        }
        if (!ret) return FALSE;
    }
    if (flags & SPINST_INIFILES)
    {
        if (!iterate_section_fields( hinf, section, UpdateInis, update_ini_callback, NULL ) ||
            !iterate_section_fields( hinf, section, UpdateIniFields,
                                     update_ini_fields_callback, NULL ))
            return FALSE;
    }
    if (flags & SPINST_INI2REG)
    {
        if (!iterate_section_fields( hinf, section, Ini2Reg, ini2reg_callback, NULL ))
            return FALSE;
    }
    if (flags & SPINST_LOGCONFIG)
    {
        if (!iterate_section_fields( hinf, section, LogConf, logconf_callback, NULL ))
            return FALSE;
    }
    if (flags & SPINST_REGSVR)
    {
        struct register_dll_info info;

        info.unregister = FALSE;
        if (flags & SPINST_REGISTERCALLBACKAWARE)
        {
            info.callback         = callback;
            info.callback_context = context;
        }
        else info.callback = NULL;

        if (!iterate_section_fields( hinf, section, RegisterDlls, register_dlls_callback, &info ))
            return FALSE;

#ifdef __WINESRC__
        if (!iterate_section_fields( hinf, section, WineFakeDlls, fake_dlls_callback, NULL ))
            return FALSE;
#endif // __WINESRC__
    }
    if (flags & SPINST_UNREGSVR)
    {
        struct register_dll_info info;

        info.unregister = TRUE;
        if (flags & SPINST_REGISTERCALLBACKAWARE)
        {
            info.callback         = callback;
            info.callback_context = context;
        }
        else info.callback = NULL;

        if (!iterate_section_fields( hinf, section, UnregisterDlls, register_dlls_callback, &info ))
            return FALSE;
    }
    if (flags & SPINST_REGISTRY)
    {
        struct registry_callback_info info;

        info.default_root = key_root;
        info.delete = TRUE;
        if (!iterate_section_fields( hinf, section, DelReg, registry_callback, &info ))
            return FALSE;
        info.delete = FALSE;
        if (!iterate_section_fields( hinf, section, AddReg, registry_callback, &info ))
            return FALSE;
    }
    if (flags & SPINST_BITREG)
    {
        if (!iterate_section_fields( hinf, section, BitReg, bitreg_callback, NULL ))
            return FALSE;
    }
    if (flags & SPINST_PROFILEITEMS)
    {
        if (!iterate_section_fields( hinf, section, ProfileItems, profile_items_callback, NULL ))
            return FALSE;
    }
    if (flags & SPINST_COPYINF)
    {
        if (!iterate_section_fields( hinf, section, CopyINF, copy_inf_callback, NULL ))
            return FALSE;
    }

    return TRUE;
}


/***********************************************************************
 *		InstallHinfSectionW  (SETUPAPI.@)
 *
 * NOTE: 'cmdline' is <section> <mode> <path> from
 *   RUNDLL32.EXE SETUPAPI.DLL,InstallHinfSection <section> <mode> <path>
 */
void WINAPI InstallHinfSectionW( HWND hwnd, HINSTANCE handle, LPCWSTR cmdline, INT show )
{
    WCHAR *s, *path, section[MAX_PATH];
    void *callback_context;
    UINT mode;
    HINF hinf;

    TRACE("hwnd %p, handle %p, cmdline %s\n", hwnd, handle, debugstr_w(cmdline));

    lstrcpynW( section, cmdline, MAX_PATH );

    if (!(s = strchrW( section, ' ' ))) return;
    *s++ = 0;
    while (*s == ' ') s++;
    mode = atoiW( s );

    /* quoted paths are not allowed on native, the rest of the command line is taken as the path */
    if (!(s = strchrW( s, ' ' ))) return;
    while (*s == ' ') s++;
    path = s;

    hinf = SetupOpenInfFileW( path, NULL, INF_STYLE_WIN4, NULL );
    if (hinf == INVALID_HANDLE_VALUE) return;

    if (SetupDiGetActualSectionToInstallW(
        hinf, section, section, sizeof(section)/sizeof(section[0]), NULL, NULL ))
    {
        callback_context = SetupInitDefaultQueueCallback( hwnd );
        SetupInstallFromInfSectionW( hwnd, hinf, section, SPINST_ALL, NULL, NULL, SP_COPY_NEWER,
                                     SetupDefaultQueueCallbackW, callback_context,
                                     NULL, NULL );
        SetupTermDefaultQueueCallback( callback_context );
    }
    SetupCloseInfFile( hinf );

    /* FIXME: should check the mode and maybe reboot */
    /* there isn't much point in doing that since we */
    /* don't yet handle deferred file copies anyway. */
}


/***********************************************************************
 *		InstallHinfSectionA  (SETUPAPI.@)
 */
void WINAPI InstallHinfSectionA( HWND hwnd, HINSTANCE handle, LPCSTR cmdline, INT show )
{
    UNICODE_STRING cmdlineW;

    if (RtlCreateUnicodeStringFromAsciiz( &cmdlineW, cmdline ))
    {
        InstallHinfSectionW( hwnd, handle, cmdlineW.Buffer, show );
        RtlFreeUnicodeString( &cmdlineW );
    }
}

/***********************************************************************
 *              SetupInstallServicesFromInfSectionW  (SETUPAPI.@)
 */
BOOL WINAPI SetupInstallServicesFromInfSectionW( HINF Inf, PCWSTR SectionName, DWORD Flags)
{
    return SetupInstallServicesFromInfSectionExW( Inf, SectionName, Flags,
                                                  NULL, NULL, NULL, NULL );
}

/***********************************************************************
 *              SetupInstallServicesFromInfSectionA  (SETUPAPI.@)
 */
BOOL WINAPI SetupInstallServicesFromInfSectionA( HINF Inf, PCSTR SectionName, DWORD Flags)
{
    return SetupInstallServicesFromInfSectionExA( Inf, SectionName, Flags,
                                                  NULL, NULL, NULL, NULL );
}

/***********************************************************************
 *		SetupInstallServicesFromInfSectionExA  (SETUPAPI.@)
 */
BOOL WINAPI SetupInstallServicesFromInfSectionExA( HINF hinf, PCSTR sectionname, DWORD flags, HDEVINFO devinfo, PSP_DEVINFO_DATA devinfo_data, PVOID reserved1, PVOID reserved2 )
{
    UNICODE_STRING sectionnameW;
    BOOL ret = FALSE;

    if (RtlCreateUnicodeStringFromAsciiz( &sectionnameW, sectionname ))
    {
        ret = SetupInstallServicesFromInfSectionExW( hinf, sectionnameW.Buffer, flags, devinfo, devinfo_data, reserved1, reserved2 );
        RtlFreeUnicodeString( &sectionnameW );
    }
    else
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );

    return ret;
}


static BOOL GetLineText( HINF hinf, PCWSTR section_name, PCWSTR key_name, PWSTR *value)
{
    DWORD required;
    PWSTR buf = NULL;

    *value = NULL;

    if (! SetupGetLineTextW( NULL, hinf, section_name, key_name, NULL, 0, &required )
        && GetLastError() != ERROR_INSUFFICIENT_BUFFER )
        return FALSE;

    buf = HeapAlloc( GetProcessHeap(), 0, required * sizeof(WCHAR) );
    if ( ! buf )
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    if (! SetupGetLineTextW( NULL, hinf, section_name, key_name, buf, required, &required ) )
    {
        HeapFree( GetProcessHeap(), 0, buf );
        return FALSE;
    }

    *value = buf;
    return TRUE;
}


static BOOL GetIntField( HINF hinf, PCWSTR section_name, PCWSTR key_name, INT *value)
{
    LPWSTR buffer, end;
    INT res;

    if (! GetLineText( hinf, section_name, key_name, &buffer ) )
        return FALSE;

    res = wcstol( buffer, &end, 0 );
    if (end != buffer && !*end)
    {
        HeapFree(GetProcessHeap(), 0, buffer);
        *value = res;
        return TRUE;
    }
    else
    {
        HeapFree(GetProcessHeap(), 0, buffer);
        SetLastError( ERROR_INVALID_DATA );
        return FALSE;
    }
}


BOOL GetStringField( PINFCONTEXT context, DWORD index, PWSTR *value)
{
    DWORD RequiredSize;
    BOOL ret;

    ret = SetupGetStringFieldW(
        context,
        index,
        NULL, 0,
        &RequiredSize);
    if (!ret)
        return FALSE;
    else if (RequiredSize == 0)
    {
        *value = NULL;
        return TRUE;
    }

    /* We got the needed size for the buffer */
    *value = MyMalloc(RequiredSize * sizeof(WCHAR));
    if (!*value)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    ret = SetupGetStringFieldW(
        context,
        index,
        *value, RequiredSize, NULL);
    if (!ret)
        MyFree(*value);

    return ret;
}


static BOOL InstallOneService(
    struct DeviceInfoSet *list,
    IN HINF hInf,
    IN LPCWSTR ServiceSection,
    IN LPCWSTR ServiceName,
    IN UINT ServiceFlags)
{
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE hService = NULL;
    LPDWORD GroupOrder = NULL;
    LPQUERY_SERVICE_CONFIGW ServiceConfig = NULL;
    HKEY hServicesKey, hServiceKey;
    LONG rc;
    BOOL ret = FALSE;

    HKEY hGroupOrderListKey = NULL;
    LPWSTR ServiceBinary = NULL;
    LPWSTR LoadOrderGroup = NULL;
    LPWSTR DisplayName = NULL;
    LPWSTR Description = NULL;
    LPWSTR Dependencies = NULL;
    LPWSTR SecurityDescriptor = NULL;
    PSECURITY_DESCRIPTOR sd = NULL;
    INT ServiceType, StartType, ErrorControl;
    DWORD dwRegType;
    DWORD tagId = (DWORD)-1;
    BOOL useTag;

    if (!GetIntField(hInf, ServiceSection, ServiceTypeKey, &ServiceType))
        goto cleanup;
    if (!GetIntField(hInf, ServiceSection, StartTypeKey, &StartType))
        goto cleanup;
    if (!GetIntField(hInf, ServiceSection, ErrorControlKey, &ErrorControl))
        goto cleanup;
    useTag = (ServiceType == SERVICE_BOOT_START || ServiceType == SERVICE_SYSTEM_START);

    hSCManager = OpenSCManagerW(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CREATE_SERVICE);
    if (hSCManager == NULL)
        goto cleanup;

    if (!GetLineText(hInf, ServiceSection, ServiceBinaryKey, &ServiceBinary))
        goto cleanup;

    /* Don't check return value, as these fields are optional and
     * GetLineText initialize output parameter even on failure */
    GetLineText(hInf, ServiceSection, LoadOrderGroupKey, &LoadOrderGroup);
    GetLineText(hInf, ServiceSection, DisplayNameKey, &DisplayName);
    GetLineText(hInf, ServiceSection, DescriptionKey, &Description);
    GetLineText(hInf, ServiceSection, DependenciesKey, &Dependencies);

    hService = OpenServiceW(
        hSCManager,
        ServiceName,
        DELETE | SERVICE_QUERY_CONFIG | SERVICE_CHANGE_CONFIG | WRITE_DAC);
    if (hService == NULL && GetLastError() != ERROR_SERVICE_DOES_NOT_EXIST)
        goto cleanup;

    if (hService && (ServiceFlags & SPSVCINST_DELETEEVENTLOGENTRY))
    {
        ret = DeleteService(hService);
        if (!ret && GetLastError() != ERROR_SERVICE_MARKED_FOR_DELETE)
            goto cleanup;
    }

    if (hService == NULL)
    {
        /* Create new service */
        hService = CreateServiceW(
            hSCManager,
            ServiceName,
            DisplayName,
            WRITE_DAC,
            ServiceType,
            StartType,
            ErrorControl,
            ServiceBinary,
            LoadOrderGroup,
            useTag ? &tagId : NULL,
            Dependencies,
            NULL, NULL);
        if (hService == NULL)
            goto cleanup;
    }
    else
    {
        DWORD bufferSize;
        /* Read current configuration */
        if (!QueryServiceConfigW(hService, NULL, 0, &bufferSize))
        {
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                goto cleanup;
            ServiceConfig = MyMalloc(bufferSize);
            if (!ServiceConfig)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto cleanup;
            }
            if (!QueryServiceConfigW(hService, ServiceConfig, bufferSize, &bufferSize))
                goto cleanup;
        }
        tagId = ServiceConfig->dwTagId;

        /* Update configuration */
        ret = ChangeServiceConfigW(
            hService,
            ServiceType,
            (ServiceFlags & SPSVCINST_NOCLOBBER_STARTTYPE) ? SERVICE_NO_CHANGE : StartType,
            (ServiceFlags & SPSVCINST_NOCLOBBER_ERRORCONTROL) ? SERVICE_NO_CHANGE : ErrorControl,
            ServiceBinary,
            (ServiceFlags & SPSVCINST_NOCLOBBER_LOADORDERGROUP && ServiceConfig->lpLoadOrderGroup) ? NULL : LoadOrderGroup,
            useTag ? &tagId : NULL,
            (ServiceFlags & SPSVCINST_NOCLOBBER_DEPENDENCIES && ServiceConfig->lpDependencies) ? NULL : Dependencies,
            NULL, NULL,
            (ServiceFlags & SPSVCINST_NOCLOBBER_DISPLAYNAME && ServiceConfig->lpDisplayName) ? NULL : DisplayName);
        if (!ret)
            goto cleanup;
    }

    /* Set security */
    if (GetLineText(hInf, ServiceSection, SecurityKey, &SecurityDescriptor))
    {
        ret = ConvertStringSecurityDescriptorToSecurityDescriptorW(SecurityDescriptor, SDDL_REVISION_1, &sd, NULL);
        if (!ret)
            goto cleanup;
        ret = SetServiceObjectSecurity(hService, DACL_SECURITY_INFORMATION, sd);
        if (!ret)
            goto cleanup;
    }

    /* FIXME: use Description and SPSVCINST_NOCLOBBER_DESCRIPTION */

    if (useTag)
    {
        /* Add the tag to SYSTEM\CurrentControlSet\Control\GroupOrderList key */
        LPCWSTR lpLoadOrderGroup;
        DWORD bufferSize;

        lpLoadOrderGroup = LoadOrderGroup;
        if ((ServiceFlags & SPSVCINST_NOCLOBBER_LOADORDERGROUP) && ServiceConfig && ServiceConfig->lpLoadOrderGroup)
            lpLoadOrderGroup = ServiceConfig->lpLoadOrderGroup;

        rc = RegOpenKeyW(
            list ? list->HKLM : HKEY_LOCAL_MACHINE,
            GroupOrderListKey,
            &hGroupOrderListKey);
        if (rc != ERROR_SUCCESS)
        {
            SetLastError(rc);
            goto cleanup;
        }
        rc = RegQueryValueExW(hGroupOrderListKey, lpLoadOrderGroup, NULL, &dwRegType, NULL, &bufferSize);
        if (rc == ERROR_FILE_NOT_FOUND)
            bufferSize = sizeof(DWORD);
        else if (rc != ERROR_SUCCESS)
        {
            SetLastError(rc);
            goto cleanup;
        }
        else if (dwRegType != REG_BINARY || bufferSize == 0 || bufferSize % sizeof(DWORD) != 0)
        {
            SetLastError(ERROR_GEN_FAILURE);
            goto cleanup;
        }
        /* Allocate buffer to store existing data + the new tag */
        GroupOrder = MyMalloc(bufferSize + sizeof(DWORD));
        if (!GroupOrder)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        if (rc == ERROR_SUCCESS)
        {
            /* Read existing data */
            rc = RegQueryValueExW(
                hGroupOrderListKey,
                lpLoadOrderGroup,
                NULL,
                NULL,
                (BYTE*)GroupOrder,
                &bufferSize);
            if (rc != ERROR_SUCCESS)
            {
                SetLastError(rc);
                goto cleanup;
            }
            if (ServiceFlags & SPSVCINST_TAGTOFRONT)
                memmove(&GroupOrder[2], &GroupOrder[1], bufferSize - sizeof(DWORD));
        }
        else
        {
            GroupOrder[0] = 0;
        }
        GroupOrder[0]++;
        if (ServiceFlags & SPSVCINST_TAGTOFRONT)
            GroupOrder[1] = tagId;
        else
            GroupOrder[bufferSize / sizeof(DWORD)] = tagId;

        rc = RegSetValueExW(
            hGroupOrderListKey,
            lpLoadOrderGroup,
            0,
            REG_BINARY,
            (BYTE*)GroupOrder,
            bufferSize + sizeof(DWORD));
        if (rc != ERROR_SUCCESS)
        {
            SetLastError(rc);
            goto cleanup;
        }
    }

    /* Handle AddReg and DelReg */
    rc = RegOpenKeyExW(
        list ? list->HKLM : HKEY_LOCAL_MACHINE,
        REGSTR_PATH_SERVICES,
        0,
        0,
        &hServicesKey);
    if (rc != ERROR_SUCCESS)
    {
        SetLastError(rc);
        goto cleanup;
    }
    rc = RegOpenKeyExW(
        hServicesKey,
        ServiceName,
        0,
        KEY_READ | KEY_WRITE,
        &hServiceKey);
    RegCloseKey(hServicesKey);
    if (rc != ERROR_SUCCESS)
    {
        SetLastError(rc);
        goto cleanup;
    }

    ret = SetupInstallFromInfSectionW(
        NULL,
        hInf,
        ServiceSection,
        SPINST_REGISTRY,
        hServiceKey,
        NULL,
        0,
        NULL,
        NULL,
        NULL,
        NULL);
    RegCloseKey(hServiceKey);

cleanup:
    if (hSCManager != NULL)
        CloseServiceHandle(hSCManager);
    if (hService != NULL)
        CloseServiceHandle(hService);
    if (hGroupOrderListKey != NULL)
        RegCloseKey(hGroupOrderListKey);
    if (sd != NULL)
        LocalFree(sd);
    MyFree(ServiceConfig);
    MyFree(ServiceBinary);
    MyFree(LoadOrderGroup);
    MyFree(DisplayName);
    MyFree(Description);
    MyFree(Dependencies);
    MyFree(SecurityDescriptor);
    MyFree(GroupOrder);

    TRACE("Returning %d\n", ret);
    return ret;
}


/***********************************************************************
 *		SetupInstallServicesFromInfSectionExW  (SETUPAPI.@)
 */
BOOL WINAPI SetupInstallServicesFromInfSectionExW( HINF hinf, PCWSTR sectionname, DWORD flags, HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData, PVOID reserved1, PVOID reserved2 )
{
    struct DeviceInfoSet *list = NULL;
    BOOL ret = FALSE;

    TRACE("%p, %s, 0x%lx, %p, %p, %p, %p\n", hinf, debugstr_w(sectionname),
        flags, DeviceInfoSet, DeviceInfoData, reserved1, reserved2);

    if (!sectionname)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (flags & ~(SPSVCINST_TAGTOFRONT | SPSVCINST_DELETEEVENTLOGENTRY | SPSVCINST_NOCLOBBER_DISPLAYNAME | SPSVCINST_NOCLOBBER_STARTTYPE | SPSVCINST_NOCLOBBER_ERRORCONTROL | SPSVCINST_NOCLOBBER_LOADORDERGROUP | SPSVCINST_NOCLOBBER_DEPENDENCIES | SPSVCINST_STOPSERVICE))
    {
        TRACE("Unknown flags: 0x%08lx\n", flags & ~(SPSVCINST_TAGTOFRONT | SPSVCINST_DELETEEVENTLOGENTRY | SPSVCINST_NOCLOBBER_DISPLAYNAME | SPSVCINST_NOCLOBBER_STARTTYPE | SPSVCINST_NOCLOBBER_ERRORCONTROL | SPSVCINST_NOCLOBBER_LOADORDERGROUP | SPSVCINST_NOCLOBBER_DEPENDENCIES | SPSVCINST_STOPSERVICE));
        SetLastError(ERROR_INVALID_FLAGS);
    }
    else if (DeviceInfoSet == (HDEVINFO)INVALID_HANDLE_VALUE)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (DeviceInfoSet && (list = (struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEVICE_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (DeviceInfoData && DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if (reserved1 != NULL || reserved2 != NULL)
        SetLastError(ERROR_INVALID_PARAMETER);
    else
    {
        struct needs_callback_info needs_info;
        LPWSTR ServiceName = NULL;
        LPWSTR ServiceSection = NULL;
        INT ServiceFlags;
        INFCONTEXT ContextService;
        BOOL bNeedReboot = FALSE;

        /* Parse 'Include' and 'Needs' directives */
        iterate_section_fields( hinf, sectionname, Include, include_callback, NULL);
        needs_info.type = 1;
        needs_info.flags = flags;
        needs_info.devinfo = DeviceInfoSet;
        needs_info.devinfo_data = DeviceInfoData;
        needs_info.reserved1 = reserved1;
        needs_info.reserved2 = reserved2;
        iterate_section_fields( hinf, sectionname, Needs, needs_callback, &needs_info);

        if (flags & SPSVCINST_STOPSERVICE)
        {
            FIXME("Stopping the device not implemented\n");
            /* This may lead to require a reboot */
            /* bNeedReboot = TRUE; */
#if 0
            SERVICE_STATUS ServiceStatus;
            ret = ControlService(hService, SERVICE_CONTROL_STOP, &ServiceStatus);
            if (!ret && GetLastError() != ERROR_SERVICE_NOT_ACTIVE)
                goto cleanup;
            if (ServiceStatus.dwCurrentState != SERVICE_STOP_PENDING && ServiceStatus.dwCurrentState != SERVICE_STOPPED)
            {
                SetLastError(ERROR_INSTALL_SERVICE_FAILURE);
                goto cleanup;
            }
#endif
            flags &= ~SPSVCINST_STOPSERVICE;
        }

        ret = SetupFindFirstLineW(hinf, sectionname, AddService, &ContextService);
        while (ret)
        {
            if (!GetStringField(&ContextService, 1, &ServiceName))
                goto nextservice;

            ret = SetupGetIntField(
                &ContextService,
                2, /* Field index */
                &ServiceFlags);
            if (!ret)
            {
                /* The field may be empty. Ignore the error */
                ServiceFlags = 0;
            }

            if (!GetStringField(&ContextService, 3, &ServiceSection))
                goto nextservice;

            ret = InstallOneService(list, hinf, ServiceSection, ServiceName, (ServiceFlags & ~SPSVCINST_ASSOCSERVICE) | flags);
            if (!ret)
                goto nextservice;

            if (ServiceFlags & SPSVCINST_ASSOCSERVICE)
            {
                ret = SetupDiSetDeviceRegistryPropertyW(DeviceInfoSet, DeviceInfoData, SPDRP_SERVICE, (LPBYTE)ServiceName, (strlenW(ServiceName) + 1) * sizeof(WCHAR));
                if (!ret)
                    goto nextservice;
            }

nextservice:
            HeapFree(GetProcessHeap(), 0, ServiceName);
            HeapFree(GetProcessHeap(), 0, ServiceSection);
            ServiceName = ServiceSection = NULL;
            ret = SetupFindNextMatchLineW(&ContextService, AddService, &ContextService);
        }

        if (bNeedReboot)
            SetLastError(ERROR_SUCCESS_REBOOT_REQUIRED);
        else
            SetLastError(ERROR_SUCCESS);
        ret = TRUE;
    }

    TRACE("Returning %d\n", ret);
    return ret;
}


/***********************************************************************
 *		SetupCopyOEMInfA  (SETUPAPI.@)
 */
BOOL WINAPI SetupCopyOEMInfA(
        IN PCSTR SourceInfFileName,
        IN PCSTR OEMSourceMediaLocation,
        IN DWORD OEMSourceMediaType,
        IN DWORD CopyStyle,
        OUT PSTR DestinationInfFileName OPTIONAL,
        IN DWORD DestinationInfFileNameSize,
        OUT PDWORD RequiredSize OPTIONAL,
        OUT PSTR* DestinationInfFileNameComponent OPTIONAL)
{
    PWSTR SourceInfFileNameW = NULL;
    PWSTR OEMSourceMediaLocationW = NULL;
    PWSTR DestinationInfFileNameW = NULL;
    PWSTR DestinationInfFileNameComponentW = NULL;
    BOOL ret = FALSE;

    TRACE("%s %s 0x%lx 0x%lx %p 0%lu %p %p\n",
        SourceInfFileName, OEMSourceMediaLocation, OEMSourceMediaType,
        CopyStyle, DestinationInfFileName, DestinationInfFileNameSize,
        RequiredSize, DestinationInfFileNameComponent);

    if (!DestinationInfFileName && DestinationInfFileNameSize > 0)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (!(SourceInfFileNameW = MultiByteToUnicode(SourceInfFileName, CP_ACP)))
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (OEMSourceMediaType != SPOST_NONE && !(OEMSourceMediaLocationW = MultiByteToUnicode(OEMSourceMediaLocation, CP_ACP)))
        SetLastError(ERROR_INVALID_PARAMETER);
    else
    {
        if (DestinationInfFileNameSize != 0)
        {
            DestinationInfFileNameW = MyMalloc(DestinationInfFileNameSize * sizeof(WCHAR));
            if (!DestinationInfFileNameW)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto cleanup;
            }
        }

        ret = SetupCopyOEMInfW(
            SourceInfFileNameW,
            OEMSourceMediaLocationW,
            OEMSourceMediaType,
            CopyStyle,
            DestinationInfFileNameW,
            DestinationInfFileNameSize,
            RequiredSize,
            DestinationInfFileNameComponent ? &DestinationInfFileNameComponentW : NULL);
        if (!ret)
            goto cleanup;

        if (DestinationInfFileNameSize != 0)
        {
            if (WideCharToMultiByte(CP_ACP, 0, DestinationInfFileNameW, -1,
                DestinationInfFileName, DestinationInfFileNameSize, NULL, NULL) == 0)
            {
                DestinationInfFileName[0] = '\0';
                goto cleanup;
            }
        }
        if (DestinationInfFileNameComponent)
        {
            if (DestinationInfFileNameComponentW)
                *DestinationInfFileNameComponent = &DestinationInfFileName[DestinationInfFileNameComponentW - DestinationInfFileNameW];
            else
                *DestinationInfFileNameComponent = NULL;
        }
        ret = TRUE;
    }

cleanup:
    MyFree(SourceInfFileNameW);
    MyFree(OEMSourceMediaLocationW);
    MyFree(DestinationInfFileNameW);

    TRACE("Returning %d\n", ret);
    return ret;
}

static int compare_files( HANDLE file1, HANDLE file2 )
{
    char buffer1[2048];
    char buffer2[2048];
    DWORD size1;
    DWORD size2;

    while( ReadFile(file1, buffer1, sizeof(buffer1), &size1, NULL) &&
           ReadFile(file2, buffer2, sizeof(buffer2), &size2, NULL) )
    {
        int ret;
        if (size1 != size2)
            return size1 > size2 ? 1 : -1;
        if (!size1)
            return 0;
        ret = memcmp( buffer1, buffer2, size1 );
        if (ret)
            return ret;
    }

    return 0;
}

/***********************************************************************
 *		SetupCopyOEMInfW  (SETUPAPI.@)
 */
BOOL WINAPI SetupCopyOEMInfW(
        IN PCWSTR SourceInfFileName,
        IN PCWSTR OEMSourceMediaLocation,
        IN DWORD OEMSourceMediaType,
        IN DWORD CopyStyle,
        OUT PWSTR DestinationInfFileName OPTIONAL,
        IN DWORD DestinationInfFileNameSize,
        OUT PDWORD RequiredSize OPTIONAL,
        OUT PWSTR* DestinationInfFileNameComponent OPTIONAL)
{
    BOOL ret = FALSE;

    TRACE("%s %s 0x%lx 0x%lx %p 0%lu %p %p\n",
        debugstr_w(SourceInfFileName), debugstr_w(OEMSourceMediaLocation), OEMSourceMediaType,
        CopyStyle, DestinationInfFileName, DestinationInfFileNameSize,
        RequiredSize, DestinationInfFileNameComponent);

    if (!SourceInfFileName)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (OEMSourceMediaType != SPOST_NONE && OEMSourceMediaType != SPOST_PATH && OEMSourceMediaType != SPOST_URL)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (OEMSourceMediaType != SPOST_NONE && !OEMSourceMediaLocation)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (CopyStyle & ~(SP_COPY_DELETESOURCE | SP_COPY_REPLACEONLY | SP_COPY_NOOVERWRITE | SP_COPY_OEMINF_CATALOG_ONLY))
    {
        TRACE("Unknown flags: 0x%08lx\n", CopyStyle & ~(SP_COPY_DELETESOURCE | SP_COPY_REPLACEONLY | SP_COPY_NOOVERWRITE | SP_COPY_OEMINF_CATALOG_ONLY));
        SetLastError(ERROR_INVALID_FLAGS);
    }
    else if (!DestinationInfFileName && DestinationInfFileNameSize > 0)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (CopyStyle & SP_COPY_OEMINF_CATALOG_ONLY)
    {
        FIXME("CopyStyle 0x%x not supported\n", SP_COPY_OEMINF_CATALOG_ONLY);
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    }
    else
    {
        HANDLE hSearch = INVALID_HANDLE_VALUE;
        WIN32_FIND_DATAW FindFileData;
        BOOL AlreadyExists;
        DWORD NextFreeNumber = 0;
        SIZE_T len;
        LPWSTR pFullFileName = NULL;
        LPWSTR pFileName; /* Pointer into pFullFileName buffer */
        HANDLE hSourceFile = INVALID_HANDLE_VALUE;

        if (OEMSourceMediaType == SPOST_PATH || OEMSourceMediaType == SPOST_URL)
            FIXME("OEMSourceMediaType 0x%lx ignored\n", OEMSourceMediaType);

        /* Check if source file exists, and open it */
        if (strchrW(SourceInfFileName, '\\' ) || strchrW(SourceInfFileName, '/' ))
        {
            WCHAR *path;

            if (!(len = GetFullPathNameW(SourceInfFileName, 0, NULL, NULL)))
                return FALSE;
            if (!(path = MyMalloc(len * sizeof(WCHAR))))
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
            }
            GetFullPathNameW(SourceInfFileName, len, path, NULL);
            hSourceFile = CreateFileW(
                path, FILE_READ_DATA | FILE_READ_ATTRIBUTES,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                NULL, OPEN_EXISTING, 0, NULL);
            MyFree(path);
        }
        else  /* try Windows directory */
        {
            WCHAR *path, *p;
            static const WCHAR Inf[]      = {'\\','i','n','f','\\',0};
            static const WCHAR System32[] = {'\\','s','y','s','t','e','m','3','2','\\',0};

            len = GetWindowsDirectoryW(NULL, 0) + strlenW(SourceInfFileName) + 12;
            if (!(path = MyMalloc(len * sizeof(WCHAR))))
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
            }
            GetWindowsDirectoryW(path, len);
            p = path + strlenW(path);
            strcpyW(p, Inf);
            strcatW(p, SourceInfFileName);
            hSourceFile = CreateFileW(
                path, FILE_READ_DATA | FILE_READ_ATTRIBUTES,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                NULL, OPEN_EXISTING, 0, NULL);
            if (hSourceFile == INVALID_HANDLE_VALUE)
            {
                strcpyW(p, System32);
                strcatW(p, SourceInfFileName);
                hSourceFile = CreateFileW(
                    path, FILE_READ_DATA | FILE_READ_ATTRIBUTES,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    NULL, OPEN_EXISTING, 0, NULL);
            }
            MyFree(path);
        }
        if (hSourceFile == INVALID_HANDLE_VALUE)
        {
            SetLastError(ERROR_FILE_NOT_FOUND);
            goto cleanup;
        }

        /* Prepare .inf file specification */
        len = MAX_PATH + 1 + strlenW(InfDirectory) + 13;
        pFullFileName = MyMalloc(len * sizeof(WCHAR));
        if (!pFullFileName)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        len = GetSystemWindowsDirectoryW(pFullFileName, MAX_PATH);
        if (len == 0 || len > MAX_PATH)
            goto cleanup;
        if (pFullFileName[strlenW(pFullFileName) - 1] != '\\')
            strcatW(pFullFileName, BackSlash);
        strcatW(pFullFileName, InfDirectory);
        pFileName = &pFullFileName[strlenW(pFullFileName)];

        /* Search if the specified .inf file already exists in %WINDIR%\Inf */
        AlreadyExists = FALSE;
        strcpyW(pFileName, OemFileMask);
        hSearch = FindFirstFileW(pFullFileName, &FindFileData);
        if (hSearch != INVALID_HANDLE_VALUE)
        {
            LARGE_INTEGER SourceFileSize;

            if (GetFileSizeEx(hSourceFile, &SourceFileSize))
            {
                do
                {
                    LARGE_INTEGER DestFileSize;
                    HANDLE hDestFile;

                    strcpyW(pFileName, FindFileData.cFileName);
                    hDestFile = CreateFileW(
                        pFullFileName, FILE_READ_DATA | FILE_READ_ATTRIBUTES,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL, OPEN_EXISTING, 0, NULL);
                    if (hDestFile != INVALID_HANDLE_VALUE)
                    {
                        if (GetFileSizeEx(hDestFile, &DestFileSize)
                         && DestFileSize.QuadPart == SourceFileSize.QuadPart
                         && compare_files(hSourceFile, hDestFile))
                        {
                            TRACE("%s already exists as %s\n",
                                debugstr_w(SourceInfFileName), debugstr_w(pFileName));
                            AlreadyExists = TRUE;
                        }
                    }
                } while (!AlreadyExists && FindNextFileW(hSearch, &FindFileData));
            }
            FindClose(hSearch);
            hSearch = INVALID_HANDLE_VALUE;
        }

        if (!AlreadyExists && CopyStyle & SP_COPY_REPLACEONLY)
        {
            /* FIXME: set DestinationInfFileName, RequiredSize, DestinationInfFileNameComponent */
            SetLastError(ERROR_FILE_NOT_FOUND);
            goto cleanup;
        }
        else if (AlreadyExists && (CopyStyle & SP_COPY_NOOVERWRITE))
        {
            DWORD Size = strlenW(pFileName) + 1;

            if (RequiredSize)
                *RequiredSize = Size;
            if (DestinationInfFileNameSize == 0)
                SetLastError(ERROR_FILE_EXISTS);
            else if (DestinationInfFileNameSize < Size)
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
            else
            {
                SetLastError(ERROR_FILE_EXISTS);
                strcpyW(DestinationInfFileName, pFileName);
            }
            goto cleanup;
        }

        /* Search the number to give to OEM??.INF */
        strcpyW(pFileName, OemFileMask);
        hSearch = FindFirstFileW(pFullFileName, &FindFileData);
        if (hSearch == INVALID_HANDLE_VALUE)
        {
            if (GetLastError() != ERROR_FILE_NOT_FOUND)
                goto cleanup;
        }
        else
        {
            do
            {
                DWORD CurrentNumber;
                if (swscanf(FindFileData.cFileName, OemFileSpecification, &CurrentNumber) == 1
                    && CurrentNumber <= 99999)
                {
                    if (CurrentNumber >= NextFreeNumber)
                        NextFreeNumber = CurrentNumber + 1;
                }
            } while (FindNextFileW(hSearch, &FindFileData));
        }

        if (NextFreeNumber > 99999)
        {
            ERR("Too much custom .inf files\n");
            SetLastError(ERROR_GEN_FAILURE);
            goto cleanup;
        }

        /* Create the full path: %WINDIR%\Inf\OEM{XXXXX}.inf */
        sprintfW(pFileName, OemFileSpecification, NextFreeNumber);
        TRACE("Next available file is %s\n", debugstr_w(pFileName));

        if (!CopyFileW(SourceInfFileName, pFullFileName, TRUE))
        {
            TRACE("CopyFileW() failed with error 0x%lx\n", GetLastError());
            goto cleanup;
        }

        len = strlenW(pFullFileName) + 1;
        if (RequiredSize)
            *RequiredSize = len;
        if (DestinationInfFileName)
        {
            if (DestinationInfFileNameSize >= len)
            {
                strcpyW(DestinationInfFileName, pFullFileName);
                if (DestinationInfFileNameComponent)
                    *DestinationInfFileNameComponent = &DestinationInfFileName[pFileName - pFullFileName];
            }
            else
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                goto cleanup;
            }
        }

        if (CopyStyle & SP_COPY_DELETESOURCE)
        {
            if (!DeleteFileW(SourceInfFileName))
            {
                TRACE("DeleteFileW() failed with error 0x%lx\n", GetLastError());
                goto cleanup;
            }
        }

        ret = TRUE;

cleanup:
        if (hSourceFile != INVALID_HANDLE_VALUE)
            CloseHandle(hSourceFile);
        if (hSearch != INVALID_HANDLE_VALUE)
            FindClose(hSearch);
        MyFree(pFullFileName);
    }

    TRACE("Returning %d\n", ret);
    return ret;
}
