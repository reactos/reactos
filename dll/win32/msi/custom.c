/*
 * Custom Action processing for the Microsoft Installer (msi.dll)
 *
 * Copyright 2005 Aric Stewart for CodeWeavers
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

#include "config.h"
#include "wine/port.h"

#define COBJMACROS

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "msidefs.h"
#include "winuser.h"
#include "objbase.h"
#include "oleauto.h"

#include "msipriv.h"
#include "msiserver.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/exception.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

#define CUSTOM_ACTION_TYPE_MASK 0x3F
static const WCHAR c_collen[] = {'C',':','\\',0};
static const WCHAR cszTempFolder[]= {'T','e','m','p','F','o','l','d','e','r',0};


static const WCHAR szActionData[] = {
    'C','u','s','t','o','m','A','c','t','i','o','n','D','a','t','a',0
};
static const WCHAR ProdCode[] = {
    'P','r','o','d','u','c','t','C','o','d','e',0
};
static const WCHAR UserSID[] = {'U','s','e','r','S','I','D',0};

typedef struct tagMSIRUNNINGACTION
{
    struct list entry;
    HANDLE handle;
    BOOL   process;
    LPWSTR name;
} MSIRUNNINGACTION;

static UINT HANDLE_CustomType1(MSIPACKAGE *package, LPCWSTR source,
                               LPCWSTR target, const INT type, LPCWSTR action);
static UINT HANDLE_CustomType2(MSIPACKAGE *package, LPCWSTR source,
                               LPCWSTR target, const INT type, LPCWSTR action);
static UINT HANDLE_CustomType17(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action);
static UINT HANDLE_CustomType18(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action);
static UINT HANDLE_CustomType19(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action);
static UINT HANDLE_CustomType23(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action);
static UINT HANDLE_CustomType50(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action);
static UINT HANDLE_CustomType34(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action);
static UINT HANDLE_CustomType37_38(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action);
static UINT HANDLE_CustomType5_6(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action);
static UINT HANDLE_CustomType21_22(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action);
static UINT HANDLE_CustomType53_54(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action);

typedef UINT (WINAPI *MsiCustomActionEntryPoint)( MSIHANDLE );

static CRITICAL_SECTION msi_custom_action_cs;
static CRITICAL_SECTION_DEBUG msi_custom_action_cs_debug =
{
    0, 0, &msi_custom_action_cs,
    { &msi_custom_action_cs_debug.ProcessLocksList,
      &msi_custom_action_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": msi_custom_action_cs") }
};
static CRITICAL_SECTION msi_custom_action_cs = { &msi_custom_action_cs_debug, -1, 0, 0, 0, 0 };

static struct list msi_pending_custom_actions = LIST_INIT( msi_pending_custom_actions );

static BOOL check_execution_scheduling_options(MSIPACKAGE *package, LPCWSTR action, UINT options)
{
    if (!package->script)
        return TRUE;

    if ((options & msidbCustomActionTypeClientRepeat) ==
            msidbCustomActionTypeClientRepeat)
    {
        if (!(package->script->InWhatSequence & SEQUENCE_UI &&
            package->script->InWhatSequence & SEQUENCE_EXEC))
        {
            TRACE("Skipping action due to dbCustomActionTypeClientRepeat option.\n");
            return FALSE;
        }
    }
    else if (options & msidbCustomActionTypeFirstSequence)
    {
        if (package->script->InWhatSequence & SEQUENCE_UI &&
            package->script->InWhatSequence & SEQUENCE_EXEC )
        {
            TRACE("Skipping action due to msidbCustomActionTypeFirstSequence option.\n");
            return FALSE;
        }
    }
    else if (options & msidbCustomActionTypeOncePerProcess)
    {
        if (check_unique_action(package,action))
        {
            TRACE("Skipping action due to msidbCustomActionTypeOncePerProcess option.\n");
            return FALSE;
        }
        else
            register_unique_action(package,action);
    }

    return TRUE;
}

/* stores the following properties before the action:
 *
 *    [CustomActionData<=>UserSID<=>ProductCode]Action
 */
static LPWSTR msi_get_deferred_action(LPCWSTR action, LPCWSTR actiondata,
                                      LPCWSTR usersid, LPCWSTR prodcode)
{
    LPWSTR deferred;
    DWORD len;

    static const WCHAR format[] = {
            '[','%','s','<','=','>','%','s','<','=','>','%','s',']','%','s',0
    };

    if (!actiondata)
        return strdupW(action);

    len = lstrlenW(action) + lstrlenW(actiondata) +
          lstrlenW(usersid) + lstrlenW(prodcode) +
          lstrlenW(format) - 7;
    deferred = msi_alloc(len * sizeof(WCHAR));

    sprintfW(deferred, format, actiondata, usersid, prodcode, action);
    return deferred;
}

static void set_deferred_action_props(MSIPACKAGE *package, LPWSTR deferred_data)
{
    LPWSTR end, beg = deferred_data + 1;

    static const WCHAR sep[] = {'<','=','>',0};

    end = strstrW(beg, sep);
    *end = '\0';
    MSI_SetPropertyW(package, szActionData, beg);
    beg = end + 3;

    end = strstrW(beg, sep);
    *end = '\0';
    MSI_SetPropertyW(package, UserSID, beg);
    beg = end + 3;

    end = strchrW(beg, ']');
    *end = '\0';
    MSI_SetPropertyW(package, ProdCode, beg);
}

UINT ACTION_CustomAction(MSIPACKAGE *package, LPCWSTR action, UINT script, BOOL execute)
{
    UINT rc = ERROR_SUCCESS;
    MSIRECORD * row = 0;
    static const WCHAR ExecSeqQuery[] =
    {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
     '`','C','u','s','t','o' ,'m','A','c','t','i','o','n','`',
     ' ','W','H','E','R','E',' ','`','A','c','t','i' ,'o','n','`',' ',
     '=',' ','\'','%','s','\'',0};
    UINT type;
    LPCWSTR source, target;
    LPWSTR ptr, deferred_data = NULL;
    LPWSTR action_copy = strdupW(action);
    WCHAR *deformated=NULL;

    /* deferred action: [properties]Action */
    if ((ptr = strrchrW(action_copy, ']')))
    {
        deferred_data = action_copy;
        action = ptr + 1;
    }

    row = MSI_QueryGetRecord( package->db, ExecSeqQuery, action );
    if (!row)
    {
        msi_free(action_copy);
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    type = MSI_RecordGetInteger(row,2);

    source = MSI_RecordGetString(row,3);
    target = MSI_RecordGetString(row,4);

    TRACE("Handling custom action %s (%x %s %s)\n",debugstr_w(action),type,
          debugstr_w(source), debugstr_w(target));

    /* handle some of the deferred actions */
    if (type & msidbCustomActionTypeTSAware)
        FIXME("msidbCustomActionTypeTSAware not handled\n");

    if (type & msidbCustomActionTypeInScript)
    {
        if (type & msidbCustomActionTypeNoImpersonate)
            WARN("msidbCustomActionTypeNoImpersonate not handled\n");

        if (type & msidbCustomActionTypeRollback)
        {
            FIXME("Rollback only action... rollbacks not supported yet\n");
            schedule_action(package, ROLLBACK_SCRIPT, action);
            rc = ERROR_SUCCESS;
            goto end;
        }
        if (!execute)
        {
            LPWSTR actiondata = msi_dup_property(package, action);
            LPWSTR usersid = msi_dup_property(package, UserSID);
            LPWSTR prodcode = msi_dup_property(package, ProdCode);
            LPWSTR deferred = msi_get_deferred_action(action, actiondata, usersid, prodcode);

            if (type & msidbCustomActionTypeCommit)
            {
                TRACE("Deferring Commit Action!\n");
                schedule_action(package, COMMIT_SCRIPT, deferred);
            }
            else
            {
                TRACE("Deferring Action!\n");
                schedule_action(package, INSTALL_SCRIPT, deferred);
            }

            rc = ERROR_SUCCESS;
            msi_free(actiondata);
            msi_free(usersid);
            msi_free(prodcode);
            msi_free(deferred);
            goto end;
        }
        else
        {
            static const WCHAR szBlank[] = {0};

            LPWSTR actiondata = msi_dup_property( package, action );

            switch (script)
            {
            case INSTALL_SCRIPT:
                package->scheduled_action_running = TRUE;
                break;
            case COMMIT_SCRIPT:
                package->commit_action_running = TRUE;
                break;
            case ROLLBACK_SCRIPT:
                package->rollback_action_running = TRUE;
                break;
            default:
                break;
            }

            if (deferred_data)
                set_deferred_action_props(package, deferred_data);
            else if (actiondata)
                MSI_SetPropertyW(package,szActionData,actiondata);
            else
                MSI_SetPropertyW(package,szActionData,szBlank);

            msi_free(actiondata);
        }
    }
    else if (!check_execution_scheduling_options(package,action,type))
    {
        rc = ERROR_SUCCESS;
        goto end;
    }

    switch (type & CUSTOM_ACTION_TYPE_MASK)
    {
        case 1: /* DLL file stored in a Binary table stream */
            rc = HANDLE_CustomType1(package,source,target,type,action);
            break;
        case 2: /* EXE file stored in a Binary table stream */
            rc = HANDLE_CustomType2(package,source,target,type,action);
            break;
        case 18: /*EXE file installed with package */
            rc = HANDLE_CustomType18(package,source,target,type,action);
            break;
        case 19: /* Error that halts install */
            rc = HANDLE_CustomType19(package,source,target,type,action);
            break;
        case 17:
            rc = HANDLE_CustomType17(package,source,target,type,action);
            break;
        case 23: /* installs another package in the source tree */
            deformat_string(package,target,&deformated);
            rc = HANDLE_CustomType23(package,source,deformated,type,action);
            msi_free(deformated);
            break;
        case 50: /*EXE file specified by a property value */
            rc = HANDLE_CustomType50(package,source,target,type,action);
            break;
        case 34: /*EXE to be run in specified directory */
            rc = HANDLE_CustomType34(package,source,target,type,action);
            break;
        case 35: /* Directory set with formatted text. */
            deformat_string(package,target,&deformated);
            MSI_SetTargetPathW(package, source, deformated);
            msi_free(deformated);
            break;
        case 51: /* Property set with formatted text. */
            if (!source)
                break;

            deformat_string(package,target,&deformated);
            rc = MSI_SetPropertyW(package,source,deformated);
            msi_free(deformated);
            break;
	case 37: /* JScript/VBScript text stored in target column. */
	case 38:
	    rc = HANDLE_CustomType37_38(package,source,target,type,action);
	    break;
	case 5:
	case 6: /* JScript/VBScript file stored in a Binary table stream. */
	    rc = HANDLE_CustomType5_6(package,source,target,type,action);
	    break;
	case 21: /* JScript/VBScript file installed with the product. */
	case 22:
	    rc = HANDLE_CustomType21_22(package,source,target,type,action);
	    break;
	case 53: /* JScript/VBScript text specified by a property value. */
	case 54:
	    rc = HANDLE_CustomType53_54(package,source,target,type,action);
	    break;
        default:
            FIXME("UNHANDLED ACTION TYPE %i (%s %s)\n",
             type & CUSTOM_ACTION_TYPE_MASK, debugstr_w(source),
             debugstr_w(target));
    }

end:
    package->scheduled_action_running = FALSE;
    package->commit_action_running = FALSE;
    package->rollback_action_running = FALSE;
    msi_free(action_copy);
    msiobj_release(&row->hdr);
    return rc;
}


static UINT store_binary_to_temp(MSIPACKAGE *package, LPCWSTR source,
                                LPWSTR tmp_file)
{
    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
        '`','B','i' ,'n','a','r','y','`',' ','W','H','E','R','E',' ',
        '`','N','a','m','e','`',' ','=',' ','\'','%','s','\'',0};
    MSIRECORD *row = 0;
    HANDLE file;
    CHAR buffer[1024];
    static const WCHAR f1[] = {'m','s','i',0};
    WCHAR fmt[MAX_PATH];
    DWORD sz = MAX_PATH;
    UINT r;

    if (MSI_GetPropertyW(package, cszTempFolder, fmt, &sz) != ERROR_SUCCESS)
        GetTempPathW(MAX_PATH, fmt);

    if (GetTempFileNameW(fmt, f1, 0, tmp_file) == 0)
    {
        TRACE("Unable to create file\n");
        return ERROR_FUNCTION_FAILED;
    }
    track_tempfile(package, tmp_file);

    row = MSI_QueryGetRecord(package->db, query, source);
    if (!row)
        return ERROR_FUNCTION_FAILED;

    /* write out the file */
    file = CreateFileW(tmp_file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        r = ERROR_FUNCTION_FAILED;
    else
    {
        do
        {
            DWORD write;
            sz = sizeof buffer;
            r = MSI_RecordReadStream(row, 2, buffer, &sz);
            if (r != ERROR_SUCCESS)
            {
                ERR("Failed to get stream\n");
                break;
            }
            WriteFile(file, buffer, sz, &write, NULL);
        } while (sz == sizeof buffer);
        CloseHandle(file);
    }

    msiobj_release(&row->hdr);

    return r;
}

static void file_running_action(MSIPACKAGE* package, HANDLE Handle,
                                BOOL process, LPCWSTR name)
{
    MSIRUNNINGACTION *action;

    action = msi_alloc( sizeof(MSIRUNNINGACTION) );

    action->handle = Handle;
    action->process = process;
    action->name = strdupW(name);

    list_add_tail( &package->RunningActions, &action->entry );
}

static UINT custom_get_process_return( HANDLE process )
{
    DWORD rc = 0;

    GetExitCodeProcess( process, &rc );
    if (rc != 0)
        return ERROR_FUNCTION_FAILED;
    return ERROR_SUCCESS;
}

static UINT custom_get_thread_return( MSIPACKAGE *package, HANDLE thread )
{
    DWORD rc = 0;

    GetExitCodeThread( thread, &rc );

    switch (rc)
    {
    case ERROR_FUNCTION_NOT_CALLED:
    case ERROR_SUCCESS:
    case ERROR_INSTALL_USEREXIT:
    case ERROR_INSTALL_FAILURE:
        return rc;
    case ERROR_NO_MORE_ITEMS:
        return ERROR_SUCCESS;
    case ERROR_INSTALL_SUSPEND:
        ACTION_ForceReboot( package );
        return ERROR_SUCCESS;
    default:
        ERR("Invalid Return Code %d\n",rc);
        return ERROR_INSTALL_FAILURE;
    }
}

static UINT wait_process_handle(MSIPACKAGE* package, UINT type,
                           HANDLE ProcessHandle, LPCWSTR name)
{
    UINT rc = ERROR_SUCCESS;

    if (!(type & msidbCustomActionTypeAsync))
    {
        TRACE("waiting for %s\n", debugstr_w(name));

        msi_dialog_check_messages(ProcessHandle);

        if (!(type & msidbCustomActionTypeContinue))
            rc = custom_get_process_return(ProcessHandle);

        CloseHandle(ProcessHandle);
    }
    else
    {
        TRACE("%s running in background\n", debugstr_w(name));

        if (!(type & msidbCustomActionTypeContinue))
            file_running_action(package, ProcessHandle, TRUE, name);
        else
            CloseHandle(ProcessHandle);
    }

    return rc;
}

typedef struct _msi_custom_action_info {
    struct list entry;
    LONG refs;
    MSIPACKAGE *package;
    LPWSTR source;
    LPWSTR target;
    HANDLE handle;
    LPWSTR action;
    INT type;
    GUID guid;
} msi_custom_action_info;

static void release_custom_action_data( msi_custom_action_info *info )
{
    EnterCriticalSection( &msi_custom_action_cs );

    if (!--info->refs)
    {
        list_remove( &info->entry );
        if (info->handle)
            CloseHandle( info->handle );
        msi_free( info->action );
        msi_free( info->source );
        msi_free( info->target );
        msiobj_release( &info->package->hdr );
        msi_free( info );
    }

    LeaveCriticalSection( &msi_custom_action_cs );
}

/* must be called inside msi_custom_action_cs if info is in the pending custom actions list */
static void addref_custom_action_data( msi_custom_action_info *info )
{
    info->refs++;
 }

static UINT wait_thread_handle( msi_custom_action_info *info )
{
    UINT rc = ERROR_SUCCESS;

    if (!(info->type & msidbCustomActionTypeAsync))
    {
        TRACE("waiting for %s\n", debugstr_w( info->action ));

        msi_dialog_check_messages( info->handle );

        if (!(info->type & msidbCustomActionTypeContinue))
            rc = custom_get_thread_return( info->package, info->handle );

        release_custom_action_data( info );
    }
    else
    {
        TRACE("%s running in background\n", debugstr_w( info->action ));
    }

    return rc;
}

static msi_custom_action_info *find_action_by_guid( const GUID *guid )
{
    msi_custom_action_info *info;
    BOOL found = FALSE;

    EnterCriticalSection( &msi_custom_action_cs );

    LIST_FOR_EACH_ENTRY( info, &msi_pending_custom_actions, msi_custom_action_info, entry )
    {
        if (IsEqualGUID( &info->guid, guid ))
        {
            addref_custom_action_data( info );
            found = TRUE;
            break;
        }
    }

    LeaveCriticalSection( &msi_custom_action_cs );

    if (!found)
        return NULL;

    return info;
}

static void handle_msi_break( LPCWSTR target )
{
    LPWSTR msg;
    WCHAR val[MAX_PATH];

    static const WCHAR MsiBreak[] = { 'M','s','i','B','r','e','a','k',0 };
    static const WCHAR WindowsInstaller[] = {
        'W','i','n','d','o','w','s',' ','I','n','s','t','a','l','l','e','r',0
    };

    static const WCHAR format[] = {
        'T','o',' ','d','e','b','u','g',' ','y','o','u','r',' ',
        'c','u','s','t','o','m',' ','a','c','t','i','o','n',',',' ',
        'a','t','t','a','c','h',' ','y','o','u','r',' ','d','e','b','u','g','g','e','r',' ',
        't','o',' ','p','r','o','c','e','s','s',' ','%','i',' ','(','0','x','%','X',')',' ',
        'a','n','d',' ','p','r','e','s','s',' ','O','K',0
    };

    if( !GetEnvironmentVariableW( MsiBreak, val, MAX_PATH ))
        return;

    if( lstrcmpiW( val, target ))
        return;

    msg = msi_alloc( (lstrlenW(format) + 10) * sizeof(WCHAR) );
    if (!msg)
        return;

    wsprintfW( msg, format, GetCurrentProcessId(), GetCurrentProcessId());
    MessageBoxW( NULL, msg, WindowsInstaller, MB_OK);
    msi_free(msg);
    DebugBreak();
}

static UINT get_action_info( const GUID *guid, INT *type, MSIHANDLE *handle,
                             BSTR *dll, BSTR *funcname,
                             IWineMsiRemotePackage **package )
{
    IClassFactory *cf = NULL;
    IWineMsiRemoteCustomAction *rca = NULL;
    HRESULT r;

    r = DllGetClassObject( &CLSID_IWineMsiRemoteCustomAction,
                           &IID_IClassFactory, (LPVOID *)&cf );
    if (FAILED(r))
    {
        ERR("failed to get IClassFactory interface\n");
        return ERROR_FUNCTION_FAILED;
    }

    r = IClassFactory_CreateInstance( cf, NULL, &IID_IWineMsiRemoteCustomAction, (LPVOID *)&rca );
    if (FAILED(r))
    {
        ERR("failed to get IWineMsiRemoteCustomAction interface\n");
        return ERROR_FUNCTION_FAILED;
    }

    r = IWineMsiRemoteCustomAction_GetActionInfo( rca, guid, type, handle, dll, funcname, package );
    IWineMsiRemoteCustomAction_Release( rca );
    if (FAILED(r))
    {
        ERR("GetActionInfo failed\n");
        return ERROR_FUNCTION_FAILED;
    }

    return ERROR_SUCCESS;
}

static DWORD ACTION_CallDllFunction( const GUID *guid )
{
    MsiCustomActionEntryPoint fn;
    MSIHANDLE hPackage, handle;
    HANDLE hModule;
    LPSTR proc;
    UINT r = ERROR_FUNCTION_FAILED;
    BSTR dll = NULL, function = NULL;
    INT type;
    IWineMsiRemotePackage *remote_package = NULL;

    TRACE("%s\n", debugstr_guid( guid ));

    r = get_action_info( guid, &type, &handle, &dll, &function, &remote_package );
    if (r != ERROR_SUCCESS)
        return r;

    hModule = LoadLibraryW( dll );
    if (!hModule)
    {
        ERR("failed to load dll %s\n", debugstr_w( dll ) );
        return r;
    }

    proc = strdupWtoA( function );
    fn = (MsiCustomActionEntryPoint) GetProcAddress( hModule, proc );
    msi_free( proc );
    if (fn)
    {
        hPackage = alloc_msi_remote_handle( (IUnknown *)remote_package );
        if (hPackage)
        {
            IWineMsiRemotePackage_SetMsiHandle( remote_package, handle );
            TRACE("calling %s\n", debugstr_w( function ) );
            handle_msi_break( function );

            CoInitialize(NULL);

            __TRY
            {
                r = fn( hPackage );
            }
            __EXCEPT_PAGE_FAULT
            {
                ERR("Custom action (%s:%s) caused a page fault: %08x\n",
                    debugstr_w(dll), debugstr_w(function), GetExceptionCode());
                r = ERROR_SUCCESS;
            }
            __ENDTRY;

            CoUninitialize();

            MsiCloseHandle( hPackage );
        }
        else
            ERR("failed to create handle for %p\n", remote_package );
    }
    else
        ERR("GetProcAddress(%s) failed\n", debugstr_w( function ) );

    FreeLibrary(hModule);

    IWineMsiRemotePackage_Release( remote_package );
    SysFreeString( dll );
    SysFreeString( function );
    MsiCloseHandle( handle );

    return r;
}

static DWORD WINAPI DllThread( LPVOID arg )
{
    LPGUID guid = arg;
    DWORD rc = 0;

    TRACE("custom action (%x) started\n", GetCurrentThreadId() );

    rc = ACTION_CallDllFunction( guid );

    TRACE("custom action (%x) returned %i\n", GetCurrentThreadId(), rc );

    MsiCloseAllHandles();
    return rc;
}

static DWORD ACTION_CAInstallPackage(const GUID *guid)
{
    msi_custom_action_info *info;
    UINT r = ERROR_FUNCTION_FAILED;
    INSTALLUILEVEL old_level;

    info = find_action_by_guid(guid);
    if (!info)
    {
        ERR("failed to find action %s\n", debugstr_guid(guid));
        return r;
    }

    old_level = MsiSetInternalUI(INSTALLUILEVEL_BASIC, NULL);
    r = MsiInstallProductW(info->source, info->target);
    MsiSetInternalUI(old_level, NULL);

    release_custom_action_data(info);

    return r;
}

static DWORD WINAPI ConcurrentInstallThread(LPVOID arg)
{
    LPGUID guid = arg;
    DWORD rc;

    TRACE("concurrent installation (%x) started\n", GetCurrentThreadId());

    rc = ACTION_CAInstallPackage(guid);

    TRACE("concurrent installation (%x) returned %i\n", GetCurrentThreadId(), rc);

    MsiCloseAllHandles();
    return rc;
}

static msi_custom_action_info *do_msidbCustomActionTypeDll(
    MSIPACKAGE *package, INT type, LPCWSTR source, LPCWSTR target, LPCWSTR action )
{
    msi_custom_action_info *info;

    info = msi_alloc( sizeof *info );
    if (!info)
        return NULL;

    msiobj_addref( &package->hdr );
    info->refs = 2; /* 1 for our caller and 1 for thread we created */
    info->package = package;
    info->type = type;
    info->target = strdupW( target );
    info->source = strdupW( source );
    info->action = strdupW( action );
    CoCreateGuid( &info->guid );

    EnterCriticalSection( &msi_custom_action_cs );
    list_add_tail( &msi_pending_custom_actions, &info->entry );
    LeaveCriticalSection( &msi_custom_action_cs );

    info->handle = CreateThread( NULL, 0, DllThread, &info->guid, 0, NULL );
    if (!info->handle)
    {
        /* release both references */
        release_custom_action_data( info );
        release_custom_action_data( info );
        return NULL;
    }

    return info;
}

static msi_custom_action_info *do_msidbCAConcurrentInstall(
    MSIPACKAGE *package, INT type, LPCWSTR source, LPCWSTR target, LPCWSTR action)
{
    msi_custom_action_info *info;

    info = msi_alloc( sizeof *info );
    if (!info)
        return NULL;

    msiobj_addref( &package->hdr );
    info->refs = 2; /* 1 for our caller and 1 for thread we created */
    info->package = package;
    info->type = type;
    info->target = strdupW( target );
    info->source = strdupW( source );
    info->action = strdupW( action );
    CoCreateGuid( &info->guid );

    EnterCriticalSection( &msi_custom_action_cs );
    list_add_tail( &msi_pending_custom_actions, &info->entry );
    LeaveCriticalSection( &msi_custom_action_cs );

    info->handle = CreateThread( NULL, 0, ConcurrentInstallThread, &info->guid, 0, NULL );
    if (!info->handle)
    {
        /* release both references */
        release_custom_action_data( info );
        release_custom_action_data( info );
        return NULL;
    }

    return info;
}

static UINT HANDLE_CustomType23(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action)
{
    msi_custom_action_info *info;
    WCHAR package_path[MAX_PATH];
    DWORD size;

    static const WCHAR backslash[] = {'\\',0};

    size = MAX_PATH;
    MSI_GetPropertyW(package, cszSourceDir, package_path, &size);
    lstrcatW(package_path, backslash);
    lstrcatW(package_path, source);

    TRACE("Installing package %s concurrently\n", debugstr_w(package_path));

    info = do_msidbCAConcurrentInstall(package, type, package_path, target, action);

    return wait_thread_handle(info);
}

static UINT HANDLE_CustomType1(MSIPACKAGE *package, LPCWSTR source,
                               LPCWSTR target, const INT type, LPCWSTR action)
{
    msi_custom_action_info *info;
    WCHAR tmp_file[MAX_PATH];
    UINT r;

    r = store_binary_to_temp(package, source, tmp_file);
    if (r != ERROR_SUCCESS)
        return r;

    TRACE("Calling function %s from %s\n",debugstr_w(target),
          debugstr_w(tmp_file));

    if (!strchrW(tmp_file,'.'))
    {
        static const WCHAR dot[]={'.',0};
        strcatW(tmp_file,dot);
    }

    info = do_msidbCustomActionTypeDll( package, type, tmp_file, target, action );

    return wait_thread_handle( info );
}

static UINT HANDLE_CustomType2(MSIPACKAGE *package, LPCWSTR source,
                               LPCWSTR target, const INT type, LPCWSTR action)
{
    WCHAR tmp_file[MAX_PATH];
    STARTUPINFOW si;
    PROCESS_INFORMATION info;
    BOOL rc;
    INT len;
    WCHAR *deformated = NULL;
    WCHAR *cmd;
    static const WCHAR spc[] = {' ',0};
    UINT r;

    memset(&si,0,sizeof(STARTUPINFOW));

    r = store_binary_to_temp(package, source, tmp_file);
    if (r != ERROR_SUCCESS)
        return r;

    deformat_string(package,target,&deformated);

    len = strlenW(tmp_file)+2;

    if (deformated)
        len += strlenW(deformated);

    cmd = msi_alloc(sizeof(WCHAR)*len);

    strcpyW(cmd,tmp_file);
    if (deformated)
    {
        strcatW(cmd,spc);
        strcatW(cmd,deformated);

        msi_free(deformated);
    }

    TRACE("executing exe %s\n", debugstr_w(cmd));

    rc = CreateProcessW(NULL, cmd, NULL, NULL, FALSE, 0, NULL,
                  c_collen, &si, &info);
    msi_free(cmd);

    if ( !rc )
    {
        ERR("Unable to execute command %s\n", debugstr_w(cmd));
        return ERROR_SUCCESS;
    }
    CloseHandle( info.hThread );

    r = wait_process_handle(package, type, info.hProcess, action);

    return r;
}

static UINT HANDLE_CustomType17(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action)
{
    msi_custom_action_info *info;
    MSIFILE *file;

    TRACE("%s %s\n", debugstr_w(source), debugstr_w(target));

    file = get_loaded_file( package, source );
    if (!file)
    {
        ERR("invalid file key %s\n", debugstr_w( source ));
        return ERROR_FUNCTION_FAILED;
    }

    info = do_msidbCustomActionTypeDll( package, type, file->TargetPath, target, action );

    return wait_thread_handle( info );
}

static UINT HANDLE_CustomType18(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION info;
    BOOL rc;
    WCHAR *deformated;
    WCHAR *cmd;
    INT len;
    static const WCHAR spc[] = {' ',0};
    MSIFILE *file;

    memset(&si,0,sizeof(STARTUPINFOW));

    file = get_loaded_file(package,source);
    if( !file )
        return ERROR_FUNCTION_FAILED;

    len = lstrlenW( file->TargetPath );

    deformat_string(package,target,&deformated);
    if (deformated)
        len += strlenW(deformated);
    len += 2;

    cmd = msi_alloc(len * sizeof(WCHAR));

    lstrcpyW( cmd, file->TargetPath);
    if (deformated)
    {
        strcatW(cmd, spc);
        strcatW(cmd, deformated);

        msi_free(deformated);
    }

    TRACE("executing exe %s\n", debugstr_w(cmd));

    rc = CreateProcessW(NULL, cmd, NULL, NULL, FALSE, 0, NULL,
                  c_collen, &si, &info);

    if ( !rc )
    {
        ERR("Unable to execute command %s\n", debugstr_w(cmd));
        msi_free(cmd);
        return ERROR_SUCCESS;
    }
    msi_free(cmd);
    CloseHandle( info.hThread );

    return wait_process_handle(package, type, info.hProcess, action);
}

static UINT HANDLE_CustomType19(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action)
{
    static const WCHAR query[] = {
      'S','E','L','E','C','T',' ','`','M','e','s','s','a','g','e','`',' ',
      'F','R','O','M',' ','`','E','r','r','o','r','`',' ',
      'W','H','E','R','E',' ','`','E','r','r','o','r','`',' ','=',' ',
      '%','s',0
    };
    MSIRECORD *row = 0;
    LPWSTR deformated = NULL;

    deformat_string( package, target, &deformated );

    /* first try treat the error as a number */
    row = MSI_QueryGetRecord( package->db, query, deformated );
    if( row )
    {
        LPCWSTR error = MSI_RecordGetString( row, 1 );
        MessageBoxW( NULL, error, NULL, MB_OK );
        msiobj_release( &row->hdr );
    }
    else
        MessageBoxW( NULL, deformated, NULL, MB_OK );

    msi_free( deformated );

    return ERROR_FUNCTION_FAILED;
}

static UINT HANDLE_CustomType50(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION info;
    WCHAR *prop;
    BOOL rc;
    WCHAR *deformated;
    WCHAR *cmd;
    INT len;
    static const WCHAR spc[] = {' ',0};

    memset(&si,0,sizeof(STARTUPINFOW));
    memset(&info,0,sizeof(PROCESS_INFORMATION));

    prop = msi_dup_property( package, source );
    if (!prop)
        return ERROR_SUCCESS;

    deformat_string(package,target,&deformated);
    len = strlenW(prop) + 2;
    if (deformated)
         len += strlenW(deformated);

    cmd = msi_alloc(sizeof(WCHAR)*len);

    strcpyW(cmd,prop);
    if (deformated)
    {
        strcatW(cmd,spc);
        strcatW(cmd,deformated);

        msi_free(deformated);
    }
    msi_free(prop);

    TRACE("executing exe %s\n", debugstr_w(cmd));

    rc = CreateProcessW(NULL, cmd, NULL, NULL, FALSE, 0, NULL,
                  c_collen, &si, &info);

    if ( !rc )
    {
        ERR("Unable to execute command %s\n", debugstr_w(cmd));
        msi_free(cmd);
        return ERROR_SUCCESS;
    }
    msi_free(cmd);

    CloseHandle( info.hThread );

    return wait_process_handle(package, type, info.hProcess, action);
}

static UINT HANDLE_CustomType34(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action)
{
    LPWSTR filename, deformated;
    STARTUPINFOW si;
    PROCESS_INFORMATION info;
    BOOL rc;

    memset(&si,0,sizeof(STARTUPINFOW));

    filename = resolve_folder(package, source, FALSE, FALSE, TRUE, NULL);

    if (!filename)
        return ERROR_FUNCTION_FAILED;

    SetCurrentDirectoryW(filename);
    msi_free(filename);

    deformat_string(package,target,&deformated);

    if (!deformated)
        return ERROR_FUNCTION_FAILED;

    TRACE("executing exe %s\n", debugstr_w(deformated));

    rc = CreateProcessW(NULL, deformated, NULL, NULL, FALSE, 0, NULL,
                  c_collen, &si, &info);

    if ( !rc )
    {
        ERR("Unable to execute command %s\n", debugstr_w(deformated));
        msi_free(deformated);
        return ERROR_SUCCESS;
    }
    msi_free(deformated);
    CloseHandle( info.hThread );

    return wait_process_handle(package, type, info.hProcess, action);
}

static DWORD ACTION_CallScript( const GUID *guid )
{
    msi_custom_action_info *info;
    MSIHANDLE hPackage;
    UINT r = ERROR_FUNCTION_FAILED;

    info = find_action_by_guid( guid );
    if (!info)
    {
        ERR("failed to find action %s\n", debugstr_guid( guid) );
        return r;
    }

    TRACE("function %s, script %s\n", debugstr_w( info->target ), debugstr_w( info->source ) );

    hPackage = alloc_msihandle( &info->package->hdr );
    if (hPackage)
    {
        r = call_script( hPackage, info->type, info->source, info->target, info->action );
        MsiCloseHandle( hPackage );
    }
    else
        ERR("failed to create handle for %p\n", info->package );

    if (info->type & msidbCustomActionTypeAsync &&
        info->type & msidbCustomActionTypeContinue)
        release_custom_action_data( info );

    return S_OK;
}

static DWORD WINAPI ScriptThread( LPVOID arg )
{
    LPGUID guid = arg;
    DWORD rc = 0;

    TRACE("custom action (%x) started\n", GetCurrentThreadId() );

    rc = ACTION_CallScript( guid );

    TRACE("custom action (%x) returned %i\n", GetCurrentThreadId(), rc );

    MsiCloseAllHandles();
    return rc;
}

static msi_custom_action_info *do_msidbCustomActionTypeScript(
    MSIPACKAGE *package, INT type, LPCWSTR script, LPCWSTR function, LPCWSTR action )
{
    msi_custom_action_info *info;

    info = msi_alloc( sizeof *info );
    if (!info)
        return NULL;

    msiobj_addref( &package->hdr );
    info->refs = 2; /* 1 for our caller and 1 for thread we created */
    info->package = package;
    info->type = type;
    info->target = strdupW( function );
    info->source = strdupW( script );
    info->action = strdupW( action );
    CoCreateGuid( &info->guid );

    EnterCriticalSection( &msi_custom_action_cs );
    list_add_tail( &msi_pending_custom_actions, &info->entry );
    LeaveCriticalSection( &msi_custom_action_cs );

    info->handle = CreateThread( NULL, 0, ScriptThread, &info->guid, 0, NULL );
    if (!info->handle)
    {
        /* release both references */
        release_custom_action_data( info );
        release_custom_action_data( info );
        return NULL;
    }

    return info;
}

static UINT HANDLE_CustomType37_38(MSIPACKAGE *package, LPCWSTR source,
                               LPCWSTR target, const INT type, LPCWSTR action)
{
    msi_custom_action_info *info;

    TRACE("%s %s\n", debugstr_w(source), debugstr_w(target));

    info = do_msidbCustomActionTypeScript( package, type, target, NULL, action );

    return wait_thread_handle( info );
}

static UINT HANDLE_CustomType5_6(MSIPACKAGE *package, LPCWSTR source,
                               LPCWSTR target, const INT type, LPCWSTR action)
{
    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
        '`','B','i' ,'n','a','r','y','`',' ','W','H','E','R','E',' ',
        '`','N','a','m','e','`',' ','=',' ','\'','%','s','\'',0};
    MSIRECORD *row = 0;
    msi_custom_action_info *info;
    CHAR *buffer = NULL;
    WCHAR *bufferw = NULL;
    DWORD sz = 0;
    UINT r;

    TRACE("%s %s\n", debugstr_w(source), debugstr_w(target));

    row = MSI_QueryGetRecord(package->db, query, source);
    if (!row)
        return ERROR_FUNCTION_FAILED;

    r = MSI_RecordReadStream(row, 2, NULL, &sz);
    if (r != ERROR_SUCCESS)
	return r;

    buffer = msi_alloc(sizeof(CHAR)*(sz+1));
    if (!buffer)
	return ERROR_FUNCTION_FAILED;

    r = MSI_RecordReadStream(row, 2, buffer, &sz);
    if (r != ERROR_SUCCESS)
        goto done;

    buffer[sz] = 0;
    bufferw = strdupAtoW(buffer);
    if (!bufferw)
    {
        r = ERROR_FUNCTION_FAILED;
        goto done;
    }

    info = do_msidbCustomActionTypeScript( package, type, bufferw, target, action );
    r = wait_thread_handle( info );

done:
    msi_free(bufferw);
    msi_free(buffer);
    return r;
}

static UINT HANDLE_CustomType21_22(MSIPACKAGE *package, LPCWSTR source,
                               LPCWSTR target, const INT type, LPCWSTR action)
{
    msi_custom_action_info *info;
    MSIFILE *file;
    HANDLE hFile;
    DWORD sz, szHighWord = 0, read;
    CHAR *buffer=NULL;
    WCHAR *bufferw=NULL;
    BOOL bRet;
    UINT r;

    TRACE("%s %s\n", debugstr_w(source), debugstr_w(target));

    file = get_loaded_file(package,source);
    if (!file)
    {
	ERR("invalid file key %s\n", debugstr_w(source));
	return ERROR_FUNCTION_FAILED;
    }

    hFile = CreateFileW(file->TargetPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
	return ERROR_FUNCTION_FAILED;

    sz = GetFileSize(hFile, &szHighWord);
    if (sz == INVALID_FILE_SIZE || szHighWord != 0)
    {
	CloseHandle(hFile);
	return ERROR_FUNCTION_FAILED;
    }

    buffer = msi_alloc(sizeof(CHAR)*(sz+1));
    if (!buffer)
    {
	CloseHandle(hFile);
	return ERROR_FUNCTION_FAILED;
    }

    bRet = ReadFile(hFile, buffer, sz, &read, NULL);
    CloseHandle(hFile);
    if (!bRet)
    {
	r = ERROR_FUNCTION_FAILED;
        goto done;
    }

    buffer[read] = 0;
    bufferw = strdupAtoW(buffer);
    if (!bufferw)
    {
        r = ERROR_FUNCTION_FAILED;
        goto done;
    }

    info = do_msidbCustomActionTypeScript( package, type, bufferw, target, action );
    r = wait_thread_handle( info );

done:
    msi_free(bufferw);
    msi_free(buffer);
    return r;
}

static UINT HANDLE_CustomType53_54(MSIPACKAGE *package, LPCWSTR source,
                               LPCWSTR target, const INT type, LPCWSTR action)
{
    msi_custom_action_info *info;
    WCHAR *prop;

    TRACE("%s %s\n", debugstr_w(source), debugstr_w(target));

    prop = msi_dup_property(package,source);
    if (!prop)
	return ERROR_SUCCESS;

    info = do_msidbCustomActionTypeScript( package, type, prop, NULL, action );
    msi_free(prop);
    return wait_thread_handle( info );
}

void ACTION_FinishCustomActions(const MSIPACKAGE* package)
{
    struct list *item;
    HANDLE *wait_handles;
    unsigned int handle_count, i;
    msi_custom_action_info *info, *cursor;

    while ((item = list_head( &package->RunningActions )))
    {
        MSIRUNNINGACTION *action = LIST_ENTRY( item, MSIRUNNINGACTION, entry );

        list_remove( &action->entry );

        TRACE("waiting for %s\n", debugstr_w( action->name ) );
        msi_dialog_check_messages( action->handle );

        CloseHandle( action->handle );
        msi_free( action->name );
        msi_free( action );
    }

    EnterCriticalSection( &msi_custom_action_cs );

    handle_count = list_count( &msi_pending_custom_actions );
    wait_handles = HeapAlloc( GetProcessHeap(), 0, handle_count * sizeof(HANDLE) );

    handle_count = 0;
    LIST_FOR_EACH_ENTRY_SAFE( info, cursor, &msi_pending_custom_actions, msi_custom_action_info, entry )
    {
        if (info->package == package )
        {
            if (DuplicateHandle(GetCurrentProcess(), info->handle, GetCurrentProcess(), &wait_handles[handle_count], SYNCHRONIZE, FALSE, 0))
                handle_count++;
        }
    }

    LeaveCriticalSection( &msi_custom_action_cs );

    for (i = 0; i < handle_count; i++)
    {
        msi_dialog_check_messages( wait_handles[i] );
        CloseHandle( wait_handles[i] );
    }

    HeapFree( GetProcessHeap(), 0, wait_handles );
}

typedef struct _msi_custom_remote_impl {
    const IWineMsiRemoteCustomActionVtbl *lpVtbl;
    LONG refs;
} msi_custom_remote_impl;

static inline msi_custom_remote_impl* mcr_from_IWineMsiRemoteCustomAction( IWineMsiRemoteCustomAction* iface )
{
    return (msi_custom_remote_impl*) iface;
}

static HRESULT WINAPI mcr_QueryInterface( IWineMsiRemoteCustomAction *iface,
                REFIID riid,LPVOID *ppobj)
{
    if( IsEqualCLSID( riid, &IID_IUnknown ) ||
        IsEqualCLSID( riid, &IID_IWineMsiRemoteCustomAction ) )
    {
        IUnknown_AddRef( iface );
        *ppobj = iface;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI mcr_AddRef( IWineMsiRemoteCustomAction *iface )
{
    msi_custom_remote_impl* This = mcr_from_IWineMsiRemoteCustomAction( iface );

    return InterlockedIncrement( &This->refs );
}

static ULONG WINAPI mcr_Release( IWineMsiRemoteCustomAction *iface )
{
    msi_custom_remote_impl* This = mcr_from_IWineMsiRemoteCustomAction( iface );
    ULONG r;

    r = InterlockedDecrement( &This->refs );
    if (r == 0)
        msi_free( This );
    return r;
}

static HRESULT WINAPI mcr_GetActionInfo( IWineMsiRemoteCustomAction *iface, LPCGUID custom_action_guid,
         INT *type, MSIHANDLE *handle, BSTR *dll, BSTR *func, IWineMsiRemotePackage **remote_package )
{
    msi_custom_action_info *info;

    info = find_action_by_guid( custom_action_guid );
    if (!info)
        return E_FAIL;

    *type = info->type;
    *handle = alloc_msihandle( &info->package->hdr );
    *dll = SysAllocString( info->source );
    *func = SysAllocString( info->target );

    release_custom_action_data( info );
    return create_msi_remote_package( NULL, (LPVOID *)remote_package );
}

static const IWineMsiRemoteCustomActionVtbl msi_custom_remote_vtbl =
{
    mcr_QueryInterface,
    mcr_AddRef,
    mcr_Release,
    mcr_GetActionInfo,
};

HRESULT create_msi_custom_remote( IUnknown *pOuter, LPVOID *ppObj )
{
    msi_custom_remote_impl* This;

    This = msi_alloc( sizeof *This );
    if (!This)
        return E_OUTOFMEMORY;

    This->lpVtbl = &msi_custom_remote_vtbl;
    This->refs = 1;

    *ppObj = This;

    return S_OK;
}
