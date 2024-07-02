/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2004,2005 Aric Stewart for CodeWeavers
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "winsvc.h"
#include "odbcinst.h"
#include "wine/debug.h"
#include "msidefs.h"
#include "winuser.h"
#include "shlobj.h"
#include "objbase.h"
#include "mscoree.h"
#include "shlwapi.h"
#include "imagehlp.h"
#include "winver.h"

#include "msipriv.h"
#include "resource.h"

#define REG_PROGRESS_VALUE 13200
#define COMPONENT_PROGRESS_VALUE 24000

WINE_DEFAULT_DEBUG_CHANNEL(msi);

struct dummy_thread
{
    HANDLE started;
    HANDLE stopped;
    HANDLE thread;
};

static INT ui_actionstart(MSIPACKAGE *package, LPCWSTR action, LPCWSTR description, LPCWSTR template)
{
    MSIRECORD *row, *textrow;
    INT rc;

    textrow = MSI_QueryGetRecord(package->db, L"SELECT * FROM `ActionText` WHERE `Action` = '%s'", action);
    if (textrow)
    {
        description = MSI_RecordGetString(textrow, 2);
        template = MSI_RecordGetString(textrow, 3);
    }

    row = MSI_CreateRecord(3);
    if (!row) return -1;
    MSI_RecordSetStringW(row, 1, action);
    MSI_RecordSetStringW(row, 2, description);
    MSI_RecordSetStringW(row, 3, template);
    rc = MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONSTART, row);
    if (textrow) msiobj_release(&textrow->hdr);
    msiobj_release(&row->hdr);
    return rc;
}

static void ui_actioninfo(MSIPACKAGE *package, const WCHAR *action, BOOL start, INT rc)
{
    MSIRECORD *row;
    WCHAR *template;

    template = msi_get_error_message(package->db, start ? MSIERR_INFO_ACTIONSTART : MSIERR_INFO_ACTIONENDED);

    row = MSI_CreateRecord(2);
    if (!row)
    {
        free(template);
        return;
    }
    MSI_RecordSetStringW(row, 0, template);
    MSI_RecordSetStringW(row, 1, action);
    MSI_RecordSetInteger(row, 2, start ? package->LastActionResult : rc);
    MSI_ProcessMessage(package, INSTALLMESSAGE_INFO, row);
    msiobj_release(&row->hdr);
    free(template);
    if (!start) package->LastActionResult = rc;
}

enum parse_state
{
    state_whitespace,
    state_token,
    state_quote
};

static int parse_prop( const WCHAR *str, WCHAR *value, int *quotes )
{
    enum parse_state state = state_quote;
    const WCHAR *p;
    WCHAR *out = value;
    BOOL ignore, in_quotes = FALSE;
    int count = 0, len = 0;

    for (p = str; *p; p++)
    {
        ignore = FALSE;
        switch (state)
        {
        case state_whitespace:
            switch (*p)
            {
            case ' ':
                in_quotes = TRUE;
                ignore = TRUE;
                len++;
                break;
            case '"':
                state = state_quote;
                if (in_quotes && p[1] != '\"') count--;
                else count++;
                break;
            default:
                state = state_token;
                in_quotes = TRUE;
                len++;
                break;
            }
            break;

        case state_token:
            switch (*p)
            {
            case '"':
                state = state_quote;
                if (in_quotes) count--;
                else count++;
                break;
            case ' ':
                state = state_whitespace;
                if (!count) goto done;
                in_quotes = TRUE;
                len++;
                break;
            default:
                if (count) in_quotes = TRUE;
                len++;
                break;
            }
            break;

        case state_quote:
            switch (*p)
            {
            case '"':
                if (in_quotes && p[1] != '\"') count--;
                else count++;
                break;
            case ' ':
                state = state_whitespace;
                if (!count || (count > 1 && !len)) goto done;
                in_quotes = TRUE;
                len++;
                break;
            default:
                state = state_token;
                if (count) in_quotes = TRUE;
                len++;
                break;
            }
            break;

        default: break;
        }
        if (!ignore && value) *out++ = *p;
        if (!count) in_quotes = FALSE;
    }

done:
    if (value)
    {
        if (!len) *value = 0;
        else *out = 0;
    }

    if(quotes) *quotes = count;
    return p - str;
}

static void remove_quotes( WCHAR *str )
{
    WCHAR *p = str;
    int len = lstrlenW( str );

    while ((p = wcschr( p, '"' )))
    {
        memmove( p, p + 1, (len - (p - str)) * sizeof(WCHAR) );
        p++;
    }
}

UINT msi_parse_command_line( MSIPACKAGE *package, LPCWSTR szCommandLine,
                             BOOL preserve_case )
{
    LPCWSTR ptr, ptr2;
    int num_quotes;
    DWORD len;
    WCHAR *prop, *val;
    UINT r;

    if (!szCommandLine)
        return ERROR_SUCCESS;

    ptr = szCommandLine;
    while (*ptr)
    {
        while (*ptr == ' ') ptr++;
        if (!*ptr) break;

        ptr2 = wcschr( ptr, '=' );
        if (!ptr2) return ERROR_INVALID_COMMAND_LINE;

        len = ptr2 - ptr;
        if (!len) return ERROR_INVALID_COMMAND_LINE;

        while (ptr[len - 1] == ' ') len--;

        prop = malloc( (len + 1) * sizeof(WCHAR) );
        memcpy( prop, ptr, len * sizeof(WCHAR) );
        prop[len] = 0;
        if (!preserve_case) wcsupr( prop );

        ptr2++;
        while (*ptr2 == ' ') ptr2++;

        num_quotes = 0;
        val = malloc( (wcslen( ptr2 ) + 1) * sizeof(WCHAR) );
        len = parse_prop( ptr2, val, &num_quotes );
        if (num_quotes % 2)
        {
            WARN("unbalanced quotes\n");
            free( val );
            free( prop );
            return ERROR_INVALID_COMMAND_LINE;
        }
        remove_quotes( val );
        TRACE("Found commandline property %s = %s\n", debugstr_w(prop), debugstr_w(val));

        r = msi_set_property( package->db, prop, val, -1 );
        if (r == ERROR_SUCCESS && !wcscmp( prop, L"SourceDir" ))
            msi_reset_source_folders( package );

        free( val );
        free( prop );

        ptr = ptr2 + len;
    }

    return ERROR_SUCCESS;
}

const WCHAR *msi_get_command_line_option(const WCHAR *cmd, const WCHAR *option, UINT *len)
{
    DWORD opt_len = lstrlenW(option);

    if (!cmd)
        return NULL;

    while (*cmd)
    {
        BOOL found = FALSE;

        while (*cmd == ' ') cmd++;
        if (!*cmd) break;

        if(!wcsnicmp(cmd, option, opt_len))
            found = TRUE;

        cmd = wcschr( cmd, '=' );
        if(!cmd) break;
        cmd++;
        while (*cmd == ' ') cmd++;
        if (!*cmd) break;

        *len = parse_prop( cmd, NULL, NULL);
        if (found) return cmd;
        cmd += *len;
    }

    return NULL;
}

WCHAR **msi_split_string( const WCHAR *str, WCHAR sep )
{
    LPCWSTR pc;
    LPWSTR p, *ret = NULL;
    UINT count = 0;

    if (!str)
        return ret;

    /* count the number of substrings */
    for ( pc = str, count = 0; pc; count++ )
    {
        pc = wcschr( pc, sep );
        if (pc)
            pc++;
    }

    /* allocate space for an array of substring pointers and the substrings */
    ret = malloc( (count + 1) * sizeof(WCHAR *) + (wcslen(str) + 1) * sizeof(WCHAR) );
    if (!ret)
        return ret;

    /* copy the string and set the pointers */
    p = (LPWSTR) &ret[count+1];
    lstrcpyW( p, str );
    for( count = 0; (ret[count] = p); count++ )
    {
        p = wcschr( p, sep );
        if (p)
            *p++ = 0;
    }

    return ret;
}

static BOOL ui_sequence_exists( MSIPACKAGE *package )
{
    MSIQUERY *view;
    DWORD count = 0;

    if (!(MSI_DatabaseOpenViewW( package->db, L"SELECT * FROM `InstallUISequence` WHERE `Sequence` > 0", &view )))
    {
        MSI_IterateRecords( view, &count, NULL, package );
        msiobj_release( &view->hdr );
    }
    return count != 0;
}

UINT msi_set_sourcedir_props(MSIPACKAGE *package, BOOL replace)
{
    WCHAR *source, *check, *p, *db;
    DWORD len;

    if (!(db = msi_dup_property( package->db, L"OriginalDatabase" )))
        return ERROR_OUTOFMEMORY;

    if (!(p = wcsrchr( db, '\\' )) && !(p = wcsrchr( db, '/' )))
    {
        free(db);
        return ERROR_SUCCESS;
    }
    len = p - db + 2;
    source = malloc( len * sizeof(WCHAR) );
    lstrcpynW( source, db, len );
    free( db );

    check = msi_dup_property( package->db, L"SourceDir" );
    if (!check || replace)
    {
        UINT r = msi_set_property( package->db, L"SourceDir", source, -1 );
        if (r == ERROR_SUCCESS)
            msi_reset_source_folders( package );
    }
    free( check );

    check = msi_dup_property( package->db, L"SOURCEDIR" );
    if (!check || replace)
        msi_set_property( package->db, L"SOURCEDIR", source, -1 );

    free( check );
    free( source );

    return ERROR_SUCCESS;
}

static BOOL needs_ui_sequence(MSIPACKAGE *package)
{
    return (package->ui_level & INSTALLUILEVEL_MASK) >= INSTALLUILEVEL_REDUCED;
}

UINT msi_set_context(MSIPACKAGE *package)
{
    UINT r = msi_locate_product( package->ProductCode, &package->Context );
    if (r != ERROR_SUCCESS)
    {
        int num = msi_get_property_int( package->db, L"ALLUSERS", 0 );
        if (num == 1 || num == 2)
            package->Context = MSIINSTALLCONTEXT_MACHINE;
        else
            package->Context = MSIINSTALLCONTEXT_USERUNMANAGED;
    }
    return ERROR_SUCCESS;
}

static UINT ITERATE_Actions(MSIRECORD *row, LPVOID param)
{
    UINT rc;
    LPCWSTR cond, action;
    MSIPACKAGE *package = param;

    action = MSI_RecordGetString(row,1);
    if (!action)
    {
        ERR("Error is retrieving action name\n");
        return ERROR_FUNCTION_FAILED;
    }

    /* check conditions */
    cond = MSI_RecordGetString(row,2);

    /* this is a hack to skip errors in the condition code */
    if (MSI_EvaluateConditionW(package, cond) == MSICONDITION_FALSE)
    {
        TRACE("Skipping action: %s (condition is false)\n", debugstr_w(action));
        return ERROR_SUCCESS;
    }

    rc = ACTION_PerformAction(package, action);

    msi_dialog_check_messages( NULL );

    if (rc == ERROR_FUNCTION_NOT_CALLED)
        rc = ERROR_SUCCESS;

    if (rc != ERROR_SUCCESS)
        ERR("Execution halted, action %s returned %i\n", debugstr_w(action), rc);

    if (package->need_reboot_now)
    {
        TRACE("action %s asked for immediate reboot, suspending installation\n",
              debugstr_w(action));
        rc = ACTION_ForceReboot( package );
    }
    return rc;
}

UINT MSI_Sequence( MSIPACKAGE *package, LPCWSTR table )
{
    MSIQUERY *view;
    UINT r;

    TRACE("%p %s\n", package, debugstr_w(table));

    r = MSI_OpenQuery( package->db, &view, L"SELECT * FROM `%s` WHERE `Sequence` > 0 ORDER BY `Sequence`", table );
    if (r == ERROR_SUCCESS)
    {
        r = MSI_IterateRecords( view, NULL, ITERATE_Actions, package );
        msiobj_release(&view->hdr);
    }
    return r;
}

static UINT ACTION_ProcessExecSequence(MSIPACKAGE *package)
{
    MSIQUERY *view;
    UINT rc;

    if (package->ExecuteSequenceRun)
    {
        TRACE("Execute Sequence already Run\n");
        return ERROR_SUCCESS;
    }

    package->ExecuteSequenceRun = TRUE;

    rc = MSI_OpenQuery(package->db, &view,
                       L"SELECT * FROM `InstallExecuteSequence` WHERE `Sequence` > 0 ORDER BY `Sequence`");
    if (rc == ERROR_SUCCESS)
    {
        TRACE("Running the actions\n");

        msi_set_property( package->db, L"SourceDir", NULL, -1 );
        rc = MSI_IterateRecords(view, NULL, ITERATE_Actions, package);
        msiobj_release(&view->hdr);
    }
    return rc;
}

static UINT ACTION_ProcessUISequence(MSIPACKAGE *package)
{
    MSIQUERY *view;
    UINT rc;

    rc = MSI_DatabaseOpenViewW(package->db,
                               L"SELECT * FROM `InstallUISequence` WHERE `Sequence` > 0 ORDER BY `Sequence`",
                               &view);
    if (rc == ERROR_SUCCESS)
    {
        TRACE("Running the actions\n");
        rc = MSI_IterateRecords(view, NULL, ITERATE_Actions, package);
        msiobj_release(&view->hdr);
    }
    return rc;
}

/********************************************************
 * ACTION helper functions and functions that perform the actions
 *******************************************************/
static UINT ACTION_HandleCustomAction(MSIPACKAGE *package, LPCWSTR action)
{
    UINT arc;
    INT uirc;

    uirc = ui_actionstart(package, action, NULL, NULL);
    if (uirc == IDCANCEL)
        return ERROR_INSTALL_USEREXIT;
    ui_actioninfo(package, action, TRUE, 0);
    arc = ACTION_CustomAction(package, action);
    uirc = !arc;

    if (arc == ERROR_FUNCTION_NOT_CALLED && needs_ui_sequence(package))
    {
        uirc = ACTION_ShowDialog(package, action);
        switch (uirc)
        {
        case -1:
            return ERROR_SUCCESS; /* stop immediately */
        case 0: arc = ERROR_FUNCTION_NOT_CALLED; break;
        case 1: arc = ERROR_SUCCESS; break;
        case 2: arc = ERROR_INSTALL_USEREXIT; break;
        case 3: arc = ERROR_INSTALL_FAILURE; break;
        case 4: arc = ERROR_INSTALL_SUSPEND; break;
        case 5: arc = ERROR_MORE_DATA; break;
        case 6: arc = ERROR_INVALID_HANDLE_STATE; break;
        case 7: arc = ERROR_INVALID_DATA; break;
        case 8: arc = ERROR_INSTALL_ALREADY_RUNNING; break;
        case 9: arc = ERROR_INSTALL_PACKAGE_REJECTED; break;
        default: arc = ERROR_FUNCTION_FAILED; break;
        }
    }

    ui_actioninfo(package, action, FALSE, uirc);

    return arc;
}

MSICOMPONENT *msi_get_loaded_component( MSIPACKAGE *package, const WCHAR *Component )
{
    MSICOMPONENT *comp;

    LIST_FOR_EACH_ENTRY( comp, &package->components, MSICOMPONENT, entry )
    {
        if (!wcscmp( Component, comp->Component )) return comp;
    }
    return NULL;
}

MSIFEATURE *msi_get_loaded_feature(MSIPACKAGE* package, const WCHAR *Feature )
{
    MSIFEATURE *feature;

    LIST_FOR_EACH_ENTRY( feature, &package->features, MSIFEATURE, entry )
    {
        if (!wcscmp( Feature, feature->Feature )) return feature;
    }
    return NULL;
}

MSIFILE *msi_get_loaded_file( MSIPACKAGE *package, const WCHAR *key )
{
    MSIFILE *file;

    LIST_FOR_EACH_ENTRY( file, &package->files, MSIFILE, entry )
    {
        if (!wcscmp( key, file->File )) return file;
    }
    return NULL;
}

MSIFOLDER *msi_get_loaded_folder( MSIPACKAGE *package, const WCHAR *dir )
{
    MSIFOLDER *folder;

    LIST_FOR_EACH_ENTRY( folder, &package->folders, MSIFOLDER, entry )
    {
        if (!wcscmp( dir, folder->Directory )) return folder;
    }
    return NULL;
}

void msi_ui_progress( MSIPACKAGE *package, int a, int b, int c, int d )
{
    MSIRECORD *row;

    row = MSI_CreateRecord( 4 );
    MSI_RecordSetInteger( row, 1, a );
    MSI_RecordSetInteger( row, 2, b );
    MSI_RecordSetInteger( row, 3, c );
    MSI_RecordSetInteger( row, 4, d );
    MSI_ProcessMessage( package, INSTALLMESSAGE_PROGRESS, row );
    msiobj_release( &row->hdr );

    msi_dialog_check_messages( NULL );
}

INSTALLSTATE msi_get_component_action( MSIPACKAGE *package, MSICOMPONENT *comp )
{
    if (!comp->Enabled)
    {
        TRACE("component is disabled: %s\n", debugstr_w(comp->Component));
        return INSTALLSTATE_UNKNOWN;
    }
    if (package->need_rollback) return comp->Installed;
    if (comp->num_clients > 0 && comp->ActionRequest == INSTALLSTATE_ABSENT)
    {
        TRACE("%s has %u clients left\n", debugstr_w(comp->Component), comp->num_clients);
        return INSTALLSTATE_UNKNOWN;
    }
    return comp->ActionRequest;
}

INSTALLSTATE msi_get_feature_action( MSIPACKAGE *package, MSIFEATURE *feature )
{
    if (package->need_rollback) return feature->Installed;
    return feature->ActionRequest;
}

static UINT ITERATE_CreateFolders(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE *package = param;
    LPCWSTR dir, component, full_path;
    MSIRECORD *uirow;
    MSIFOLDER *folder;
    MSICOMPONENT *comp;

    component = MSI_RecordGetString(row, 2);
    if (!component)
        return ERROR_SUCCESS;

    comp = msi_get_loaded_component(package, component);
    if (!comp)
        return ERROR_SUCCESS;

    comp->Action = msi_get_component_action( package, comp );
    if (comp->Action != INSTALLSTATE_LOCAL)
    {
        TRACE("component not scheduled for installation: %s\n", debugstr_w(component));
        return ERROR_SUCCESS;
    }

    dir = MSI_RecordGetString(row,1);
    if (!dir)
    {
        ERR("Unable to get folder id\n");
        return ERROR_SUCCESS;
    }

    uirow = MSI_CreateRecord(1);
    MSI_RecordSetStringW(uirow, 1, dir);
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
    msiobj_release(&uirow->hdr);

    full_path = msi_get_target_folder( package, dir );
    if (!full_path)
    {
        ERR("Unable to retrieve folder %s\n", debugstr_w(dir));
        return ERROR_SUCCESS;
    }
    TRACE("folder is %s\n", debugstr_w(full_path));

    folder = msi_get_loaded_folder( package, dir );
    if (folder->State == FOLDER_STATE_UNINITIALIZED) msi_create_full_path( package, full_path );
    folder->State = FOLDER_STATE_CREATED;

    return ERROR_SUCCESS;
}

static UINT ACTION_CreateFolders(MSIPACKAGE *package)
{
    MSIQUERY *view;
    UINT rc;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action(package, SCRIPT_INSTALL, L"CreateFolders");

    rc = MSI_DatabaseOpenViewW( package->db, L"SELECT * FROM `CreateFolder`", &view );
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords(view, NULL, ITERATE_CreateFolders, package);
    msiobj_release(&view->hdr);
    return rc;
}

static void remove_persistent_folder( MSIFOLDER *folder )
{
    FolderList *fl;

    LIST_FOR_EACH_ENTRY( fl, &folder->children, FolderList, entry )
    {
        remove_persistent_folder( fl->folder );
    }
    if (folder->persistent && folder->State != FOLDER_STATE_REMOVED)
    {
        if (RemoveDirectoryW( folder->ResolvedTarget )) folder->State = FOLDER_STATE_REMOVED;
    }
}

static UINT ITERATE_RemoveFolders( MSIRECORD *row, LPVOID param )
{
    MSIPACKAGE *package = param;
    LPCWSTR dir, component, full_path;
    MSIRECORD *uirow;
    MSIFOLDER *folder;
    MSICOMPONENT *comp;

    component = MSI_RecordGetString(row, 2);
    if (!component)
        return ERROR_SUCCESS;

    comp = msi_get_loaded_component(package, component);
    if (!comp)
        return ERROR_SUCCESS;

    comp->Action = msi_get_component_action( package, comp );
    if (comp->Action != INSTALLSTATE_ABSENT)
    {
        TRACE("component not scheduled for removal %s\n", debugstr_w(component));
        return ERROR_SUCCESS;
    }

    dir = MSI_RecordGetString( row, 1 );
    if (!dir)
    {
        ERR("Unable to get folder id\n");
        return ERROR_SUCCESS;
    }

    full_path = msi_get_target_folder( package, dir );
    if (!full_path)
    {
        ERR("Unable to resolve folder %s\n", debugstr_w(dir));
        return ERROR_SUCCESS;
    }
    TRACE("folder is %s\n", debugstr_w(full_path));

    uirow = MSI_CreateRecord( 1 );
    MSI_RecordSetStringW( uirow, 1, dir );
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
    msiobj_release( &uirow->hdr );

    folder = msi_get_loaded_folder( package, dir );
    remove_persistent_folder( folder );
    return ERROR_SUCCESS;
}

static UINT ACTION_RemoveFolders( MSIPACKAGE *package )
{
    MSIQUERY *view;
    UINT rc;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action(package, SCRIPT_INSTALL, L"RemoveFolders");

    rc = MSI_DatabaseOpenViewW( package->db, L"SELECT * FROM `CreateFolder`", &view );
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords( view, NULL, ITERATE_RemoveFolders, package );
    msiobj_release( &view->hdr );
    return rc;
}

static UINT load_component( MSIRECORD *row, LPVOID param )
{
    MSIPACKAGE *package = param;
    MSICOMPONENT *comp;

    comp = calloc( 1, sizeof(MSICOMPONENT) );
    if (!comp)
        return ERROR_FUNCTION_FAILED;

    list_add_tail( &package->components, &comp->entry );

    /* fill in the data */
    comp->Component = msi_dup_record_field( row, 1 );

    TRACE("Loading Component %s\n", debugstr_w(comp->Component));

    comp->ComponentId = msi_dup_record_field( row, 2 );
    comp->Directory = msi_dup_record_field( row, 3 );
    comp->Attributes = MSI_RecordGetInteger(row,4);
    comp->Condition = msi_dup_record_field( row, 5 );
    comp->KeyPath = msi_dup_record_field( row, 6 );

    comp->Installed = INSTALLSTATE_UNKNOWN;
    comp->Action = INSTALLSTATE_UNKNOWN;
    comp->ActionRequest = INSTALLSTATE_UNKNOWN;

    comp->assembly = msi_load_assembly( package, comp );
    return ERROR_SUCCESS;
}

UINT msi_load_all_components( MSIPACKAGE *package )
{
    MSIQUERY *view;
    UINT r;

    if (!list_empty(&package->components))
        return ERROR_SUCCESS;

    r = MSI_DatabaseOpenViewW( package->db, L"SELECT * FROM `Component`", &view );
    if (r != ERROR_SUCCESS)
        return r;

    r = MSI_IterateRecords(view, NULL, load_component, package);
    msiobj_release(&view->hdr);
    return r;
}

static UINT add_feature_component( MSIFEATURE *feature, MSICOMPONENT *comp )
{
    ComponentList *cl;

    cl = malloc( sizeof(*cl) );
    if ( !cl )
        return ERROR_NOT_ENOUGH_MEMORY;
    cl->component = comp;
    list_add_tail( &feature->Components, &cl->entry );

    return ERROR_SUCCESS;
}

static UINT add_feature_child( MSIFEATURE *parent, MSIFEATURE *child )
{
    FeatureList *fl;

    fl = malloc( sizeof(*fl) );
    if ( !fl )
        return ERROR_NOT_ENOUGH_MEMORY;
    fl->feature = child;
    list_add_tail( &parent->Children, &fl->entry );

    return ERROR_SUCCESS;
}

struct package_feature
{
    MSIPACKAGE *package;
    MSIFEATURE *feature;
};

static UINT iterate_load_featurecomponents(MSIRECORD *row, LPVOID param)
{
    struct package_feature *package_feature = param;
    LPCWSTR component;
    MSICOMPONENT *comp;

    component = MSI_RecordGetString(row,1);

    /* check to see if the component is already loaded */
    comp = msi_get_loaded_component( package_feature->package, component );
    if (!comp)
    {
        WARN("ignoring unknown component %s\n", debugstr_w(component));
        return ERROR_SUCCESS;
    }
    add_feature_component( package_feature->feature, comp );
    comp->Enabled = TRUE;

    return ERROR_SUCCESS;
}

static UINT load_feature(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE *package = param;
    MSIFEATURE *feature;
    MSIQUERY *view;
    struct package_feature package_feature;
    UINT rc;

    /* fill in the data */

    feature = calloc( 1, sizeof(MSIFEATURE) );
    if (!feature)
        return ERROR_NOT_ENOUGH_MEMORY;

    list_init( &feature->Children );
    list_init( &feature->Components );

    feature->Feature = msi_dup_record_field( row, 1 );

    TRACE("Loading feature %s\n",debugstr_w(feature->Feature));

    feature->Feature_Parent = msi_dup_record_field( row, 2 );
    feature->Title = msi_dup_record_field( row, 3 );
    feature->Description = msi_dup_record_field( row, 4 );

    if (!MSI_RecordIsNull(row,5))
        feature->Display = MSI_RecordGetInteger(row,5);

    feature->Level= MSI_RecordGetInteger(row,6);
    feature->Directory = msi_dup_record_field( row, 7 );
    feature->Attributes = MSI_RecordGetInteger(row,8);

    feature->Installed = INSTALLSTATE_UNKNOWN;
    feature->Action = INSTALLSTATE_UNKNOWN;
    feature->ActionRequest = INSTALLSTATE_UNKNOWN;

    list_add_tail( &package->features, &feature->entry );

    /* load feature components */

    rc = MSI_OpenQuery( package->db, &view, L"SELECT `Component_` FROM `FeatureComponents` WHERE `Feature_` = '%s'",
                        feature->Feature );
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    package_feature.package = package;
    package_feature.feature = feature;

    rc = MSI_IterateRecords(view, NULL, iterate_load_featurecomponents, &package_feature);
    msiobj_release(&view->hdr);
    return rc;
}

static UINT find_feature_children(MSIRECORD * row, LPVOID param)
{
    MSIPACKAGE *package = param;
    MSIFEATURE *parent, *child;

    child = msi_get_loaded_feature( package, MSI_RecordGetString( row, 1 ) );
    if (!child)
        return ERROR_FUNCTION_FAILED;

    if (!child->Feature_Parent)
        return ERROR_SUCCESS;

    parent = msi_get_loaded_feature( package, child->Feature_Parent );
    if (!parent)
        return ERROR_FUNCTION_FAILED;

    add_feature_child( parent, child );
    return ERROR_SUCCESS;
}

UINT msi_load_all_features( MSIPACKAGE *package )
{
    MSIQUERY *view;
    UINT r;

    if (!list_empty(&package->features))
        return ERROR_SUCCESS;

    r = MSI_DatabaseOpenViewW( package->db, L"SELECT * FROM `Feature` ORDER BY `Display`", &view );
    if (r != ERROR_SUCCESS)
        return r;

    r = MSI_IterateRecords( view, NULL, load_feature, package );
    if (r != ERROR_SUCCESS)
    {
        msiobj_release( &view->hdr );
        return r;
    }
    r = MSI_IterateRecords( view, NULL, find_feature_children, package );
    msiobj_release( &view->hdr );
    return r;
}

static LPWSTR folder_split_path(LPWSTR p, WCHAR ch)
{
    if (!p)
        return p;
    p = wcschr(p, ch);
    if (!p)
        return p;
    *p = 0;
    return p+1;
}

static UINT load_file_hash(MSIPACKAGE *package, MSIFILE *file)
{
    MSIQUERY *view = NULL;
    MSIRECORD *row = NULL;
    UINT r;

    TRACE("%s\n", debugstr_w(file->File));

    r = MSI_OpenQuery(package->db, &view, L"SELECT * FROM `MsiFileHash` WHERE `File_` = '%s'", file->File);
    if (r != ERROR_SUCCESS)
        goto done;

    r = MSI_ViewExecute(view, NULL);
    if (r != ERROR_SUCCESS)
        goto done;

    r = MSI_ViewFetch(view, &row);
    if (r != ERROR_SUCCESS)
        goto done;

    file->hash.dwFileHashInfoSize = sizeof(MSIFILEHASHINFO);
    file->hash.dwData[0] = MSI_RecordGetInteger(row, 3);
    file->hash.dwData[1] = MSI_RecordGetInteger(row, 4);
    file->hash.dwData[2] = MSI_RecordGetInteger(row, 5);
    file->hash.dwData[3] = MSI_RecordGetInteger(row, 6);

done:
    if (view) msiobj_release(&view->hdr);
    if (row) msiobj_release(&row->hdr);
    return r;
}

static UINT load_file_disk_id( MSIPACKAGE *package, MSIFILE *file )
{
    MSIRECORD *row = MSI_QueryGetRecord( package->db, L"SELECT `DiskId` FROM `Media` WHERE `LastSequence` >= %d",
                                         file->Sequence );
    if (!row)
    {
        WARN("query failed\n");
        return ERROR_FUNCTION_FAILED;
    }

    file->disk_id = MSI_RecordGetInteger( row, 1 );
    msiobj_release( &row->hdr );
    return ERROR_SUCCESS;
}

static UINT load_file(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE* package = param;
    LPCWSTR component;
    MSIFILE *file;

    /* fill in the data */

    file = calloc( 1, sizeof(MSIFILE) );
    if (!file)
        return ERROR_NOT_ENOUGH_MEMORY;

    file->File = msi_dup_record_field( row, 1 );

    component = MSI_RecordGetString( row, 2 );
    file->Component = msi_get_loaded_component( package, component );

    if (!file->Component)
    {
        WARN("Component not found: %s\n", debugstr_w(component));
        free(file->File);
        free(file);
        return ERROR_SUCCESS;
    }

    file->FileName = msi_dup_record_field( row, 3 );
    msi_reduce_to_long_filename( file->FileName );

    file->ShortName = msi_dup_record_field( row, 3 );
    file->LongName = wcsdup( folder_split_path(file->ShortName, '|') );

    file->FileSize = MSI_RecordGetInteger( row, 4 );
    file->Version = msi_dup_record_field( row, 5 );
    file->Language = msi_dup_record_field( row, 6 );
    file->Attributes = MSI_RecordGetInteger( row, 7 );
    file->Sequence = MSI_RecordGetInteger( row, 8 );

    file->state = msifs_invalid;

    /* if the compressed bits are not set in the file attributes,
     * then read the information from the package word count property
     */
    if (package->WordCount & msidbSumInfoSourceTypeAdminImage)
    {
        file->IsCompressed = package->WordCount & msidbSumInfoSourceTypeCompressed;
    }
    else if (file->Attributes & (msidbFileAttributesCompressed | msidbFileAttributesPatchAdded))
    {
        file->IsCompressed = TRUE;
    }
    else if (file->Attributes & msidbFileAttributesNoncompressed)
    {
        file->IsCompressed = FALSE;
    }
    else file->IsCompressed = package->WordCount & msidbSumInfoSourceTypeCompressed;

    load_file_hash(package, file);
    load_file_disk_id(package, file);

    TRACE("File loaded (file %s sequence %u)\n", debugstr_w(file->File), file->Sequence);

    list_add_tail( &package->files, &file->entry );
    return ERROR_SUCCESS;
}

static UINT load_all_files(MSIPACKAGE *package)
{
    MSIQUERY *view;
    UINT rc;

    if (!list_empty(&package->files))
        return ERROR_SUCCESS;

    rc = MSI_DatabaseOpenViewW(package->db, L"SELECT * FROM `File` ORDER BY `Sequence`", &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords(view, NULL, load_file, package);
    msiobj_release(&view->hdr);
    return rc;
}

static UINT load_media( MSIRECORD *row, LPVOID param )
{
    MSIPACKAGE *package = param;
    UINT disk_id = MSI_RecordGetInteger( row, 1 );
    const WCHAR *cabinet = MSI_RecordGetString( row, 4 );

    /* FIXME: load external cabinets and directory sources too */
    if (!cabinet || cabinet[0] != '#' || disk_id >= MSI_INITIAL_MEDIA_TRANSFORM_DISKID)
        return ERROR_SUCCESS;

    return msi_add_cabinet_stream( package, disk_id, package->db->storage, cabinet );
}

static UINT load_all_media( MSIPACKAGE *package )
{
    MSIQUERY *view;
    UINT r;

    r = MSI_DatabaseOpenViewW( package->db, L"SELECT * FROM `Media` ORDER BY `DiskId`", &view );
    if (r != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    r = MSI_IterateRecords( view, NULL, load_media, package );
    msiobj_release( &view->hdr );
    return r;
}

static UINT load_patch_disk_id( MSIPACKAGE *package, MSIFILEPATCH *patch )
{
    MSIRECORD *rec = MSI_QueryGetRecord( package->db, L"SELECT `DiskId` FROM `Media` WHERE `LastSequence` >= %u",
                                         patch->Sequence );
    if (!rec)
    {
        WARN("query failed\n");
        return ERROR_FUNCTION_FAILED;
    }

    patch->disk_id = MSI_RecordGetInteger( rec, 1 );
    msiobj_release( &rec->hdr );
    return ERROR_SUCCESS;
}

static UINT load_patch(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE *package = param;
    MSIFILEPATCH *patch;
    const WCHAR *file_key;

    patch = calloc( 1, sizeof(MSIFILEPATCH) );
    if (!patch)
        return ERROR_NOT_ENOUGH_MEMORY;

    file_key = MSI_RecordGetString( row, 1 );
    patch->File = msi_get_loaded_file( package, file_key );
    if (!patch->File)
    {
        ERR("Failed to find target for patch in File table\n");
        free(patch);
        return ERROR_FUNCTION_FAILED;
    }

    patch->Sequence = MSI_RecordGetInteger( row, 2 );
    patch->PatchSize = MSI_RecordGetInteger( row, 3 );
    patch->Attributes = MSI_RecordGetInteger( row, 4 );

    /* FIXME:
     * Header field - for patch validation.
     * _StreamRef   - External key into MsiPatchHeaders (instead of the header field)
     */

    load_patch_disk_id( package, patch );

    TRACE("Patch loaded (file %s sequence %u)\n", debugstr_w(patch->File->File), patch->Sequence);

    list_add_tail( &package->filepatches, &patch->entry );

    return ERROR_SUCCESS;
}

static UINT load_all_patches(MSIPACKAGE *package)
{
    MSIQUERY *view;
    UINT rc;

    if (!list_empty(&package->filepatches))
        return ERROR_SUCCESS;

    rc = MSI_DatabaseOpenViewW(package->db, L"SELECT * FROM `Patch` ORDER BY `Sequence`", &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords(view, NULL, load_patch, package);
    msiobj_release(&view->hdr);
    return rc;
}

static UINT iterate_patched_component( MSIRECORD *row, LPVOID param )
{
    MSIPACKAGE *package = param;
    const WCHAR *name;
    MSICOMPONENT *c;

    name = MSI_RecordGetString( row, 1 );
    TRACE( "found patched component: %s\n", wine_dbgstr_w(name) );
    c = msi_get_loaded_component( package, name );
    if (!c)
        return ERROR_SUCCESS;

    c->updated = 1;
    if (!wcscmp( MSI_RecordGetString( row, 2 ), L"INSERT" ))
        c->added = 1;
    return ERROR_SUCCESS;
}

static void mark_patched_components( MSIPACKAGE *package )
{
    static const WCHAR select[] = L"SELECT `Row`, `Column` FROM `_TransformView` WHERE `Table`='Component'";
    MSIQUERY *q;
    UINT r;

    r = MSI_OpenQuery( package->db, &q, select );
    if (r != ERROR_SUCCESS)
        return;

    MSI_IterateRecords( q, NULL, iterate_patched_component, package );
    msiobj_release( &q->hdr );

    while (1)
    {
        r = MSI_OpenQuery( package->db, &q, L"ALTER TABLE `_TransformView` FREE" );
        if (r != ERROR_SUCCESS)
            return;
        r = MSI_ViewExecute( q, NULL );
        msiobj_release( &q->hdr );
        if (r != ERROR_SUCCESS)
            return;
    }
}

static UINT load_folder_persistence( MSIPACKAGE *package, MSIFOLDER *folder )
{
    MSIQUERY *view;

    folder->persistent = FALSE;
    if (!MSI_OpenQuery( package->db, &view, L"SELECT * FROM `CreateFolder` WHERE `Directory_` = '%s'",
                        folder->Directory ))
    {
        if (!MSI_ViewExecute( view, NULL ))
        {
            MSIRECORD *rec;
            if (!MSI_ViewFetch( view, &rec ))
            {
                TRACE("directory %s is persistent\n", debugstr_w(folder->Directory));
                folder->persistent = TRUE;
                msiobj_release( &rec->hdr );
            }
        }
        msiobj_release( &view->hdr );
    }
    return ERROR_SUCCESS;
}

static UINT load_folder( MSIRECORD *row, LPVOID param )
{
    MSIPACKAGE *package = param;
    static WCHAR szEmpty[] = L"";
    LPWSTR p, tgt_short, tgt_long, src_short, src_long;
    MSIFOLDER *folder;

    if (!(folder = calloc( 1, sizeof(*folder) ))) return ERROR_NOT_ENOUGH_MEMORY;
    list_init( &folder->children );
    folder->Directory = msi_dup_record_field( row, 1 );
    folder->Parent = msi_dup_record_field( row, 2 );
    p = msi_dup_record_field(row, 3);

    TRACE("%s\n", debugstr_w(folder->Directory));

    /* split src and target dir */
    tgt_short = p;
    src_short = folder_split_path( p, ':' );

    /* split the long and short paths */
    tgt_long = folder_split_path( tgt_short, '|' );
    src_long = folder_split_path( src_short, '|' );

    /* check for no-op dirs */
    if (tgt_short && !wcscmp( L".", tgt_short ))
        tgt_short = szEmpty;
    if (src_short && !wcscmp( L".", src_short ))
        src_short = szEmpty;

    if (!tgt_long)
        tgt_long = tgt_short;

    if (!src_short) {
        src_short = tgt_short;
        src_long = tgt_long;
    }

    if (!src_long)
        src_long = src_short;

    /* FIXME: use the target short path too */
    folder->TargetDefault = wcsdup(tgt_long);
    folder->SourceShortPath = wcsdup(src_short);
    folder->SourceLongPath = wcsdup(src_long);
    free(p);

    TRACE("TargetDefault = %s\n",debugstr_w( folder->TargetDefault ));
    TRACE("SourceLong = %s\n", debugstr_w( folder->SourceLongPath ));
    TRACE("SourceShort = %s\n", debugstr_w( folder->SourceShortPath ));

    load_folder_persistence( package, folder );

    list_add_tail( &package->folders, &folder->entry );
    return ERROR_SUCCESS;
}

static UINT add_folder_child( MSIFOLDER *parent, MSIFOLDER *child )
{
    FolderList *fl;

    if (!(fl = malloc( sizeof(*fl) ))) return ERROR_NOT_ENOUGH_MEMORY;
    fl->folder = child;
    list_add_tail( &parent->children, &fl->entry );
    return ERROR_SUCCESS;
}

static UINT find_folder_children( MSIRECORD *row, LPVOID param )
{
    MSIPACKAGE *package = param;
    MSIFOLDER *parent, *child;

    if (!(child = msi_get_loaded_folder( package, MSI_RecordGetString( row, 1 ) )))
        return ERROR_FUNCTION_FAILED;

    if (!child->Parent) return ERROR_SUCCESS;

    if (!(parent = msi_get_loaded_folder( package, child->Parent )))
        return ERROR_FUNCTION_FAILED;

    return add_folder_child( parent, child );
}

static UINT load_all_folders( MSIPACKAGE *package )
{
    MSIQUERY *view;
    UINT r;

    if (!list_empty(&package->folders))
        return ERROR_SUCCESS;

    r = MSI_DatabaseOpenViewW( package->db, L"SELECT * FROM `Directory`", &view );
    if (r != ERROR_SUCCESS)
        return r;

    r = MSI_IterateRecords( view, NULL, load_folder, package );
    if (r != ERROR_SUCCESS)
    {
        msiobj_release( &view->hdr );
        return r;
    }
    r = MSI_IterateRecords( view, NULL, find_folder_children, package );
    msiobj_release( &view->hdr );
    return r;
}

static UINT ACTION_CostInitialize(MSIPACKAGE *package)
{
    msi_set_property( package->db, L"CostingComplete", L"0", -1 );
    msi_set_property( package->db, L"ROOTDRIVE", L"C:\\", -1 );

    load_all_folders( package );
    msi_load_all_components( package );
    msi_load_all_features( package );
    load_all_files( package );
    load_all_patches( package );
    mark_patched_components( package );
    load_all_media( package );

    return ERROR_SUCCESS;
}

static UINT execute_script( MSIPACKAGE *package, UINT script )
{
    UINT i, rc = ERROR_SUCCESS;

    TRACE("executing script %u\n", script);

    package->script = script;

    if (script == SCRIPT_ROLLBACK)
    {
        for (i = package->script_actions_count[script]; i > 0; i--)
        {
            rc = ACTION_PerformAction(package, package->script_actions[script][i-1]);
            if (rc != ERROR_SUCCESS)
            {
                ERR("Execution of script %i halted; action %s returned %u\n",
                    script, debugstr_w(package->script_actions[script][i-1]), rc);
                break;
            }
        }
    }
    else
    {
        for (i = 0; i < package->script_actions_count[script]; i++)
        {
            rc = ACTION_PerformAction(package, package->script_actions[script][i]);
            if (rc != ERROR_SUCCESS)
            {
                ERR("Execution of script %i halted; action %s returned %u\n",
                    script, debugstr_w(package->script_actions[script][i]), rc);
                break;
            }
        }
    }

    package->script = SCRIPT_NONE;

    msi_free_action_script(package, script);
    return rc;
}

static UINT ACTION_FileCost(MSIPACKAGE *package)
{
    return ERROR_SUCCESS;
}

static void get_client_counts( MSIPACKAGE *package )
{
    MSICOMPONENT *comp;
    HKEY hkey;

    LIST_FOR_EACH_ENTRY( comp, &package->components, MSICOMPONENT, entry )
    {
        if (!comp->ComponentId) continue;

        if (MSIREG_OpenUserDataComponentKey( comp->ComponentId, L"S-1-5-18", &hkey, FALSE ) &&
            MSIREG_OpenUserDataComponentKey( comp->ComponentId, NULL, &hkey, FALSE ))
        {
            comp->num_clients = 0;
            continue;
        }
        RegQueryInfoKeyW( hkey, NULL, NULL, NULL, NULL, NULL, NULL, (DWORD *)&comp->num_clients,
                          NULL, NULL, NULL, NULL );
        RegCloseKey( hkey );
    }
}

static void ACTION_GetComponentInstallStates(MSIPACKAGE *package)
{
    MSICOMPONENT *comp;
    UINT r;

    LIST_FOR_EACH_ENTRY(comp, &package->components, MSICOMPONENT, entry)
    {
        if (!comp->ComponentId) continue;

        r = MsiQueryComponentStateW( package->ProductCode, NULL,
                                     MSIINSTALLCONTEXT_USERMANAGED, comp->ComponentId,
                                     &comp->Installed );
        if (r == ERROR_SUCCESS) continue;

        r = MsiQueryComponentStateW( package->ProductCode, NULL,
                                     MSIINSTALLCONTEXT_USERUNMANAGED, comp->ComponentId,
                                     &comp->Installed );
        if (r == ERROR_SUCCESS) continue;

        r = MsiQueryComponentStateW( package->ProductCode, NULL,
                                     MSIINSTALLCONTEXT_MACHINE, comp->ComponentId,
                                     &comp->Installed );
        if (r == ERROR_SUCCESS) continue;

        comp->Installed = INSTALLSTATE_ABSENT;
    }
}

static void ACTION_GetFeatureInstallStates(MSIPACKAGE *package)
{
    MSIFEATURE *feature;

    LIST_FOR_EACH_ENTRY( feature, &package->features, MSIFEATURE, entry )
    {
        INSTALLSTATE state = MsiQueryFeatureStateW( package->ProductCode, feature->Feature );

        if (state == INSTALLSTATE_UNKNOWN || state == INSTALLSTATE_INVALIDARG)
            feature->Installed = INSTALLSTATE_ABSENT;
        else
            feature->Installed = state;
    }
}

static inline BOOL is_feature_selected( MSIFEATURE *feature, INT level )
{
    return (feature->Level > 0 && feature->Level <= level);
}

static BOOL process_state_property(MSIPACKAGE* package, int level,
                                   LPCWSTR property, INSTALLSTATE state)
{
    LPWSTR override;
    MSIFEATURE *feature;
    BOOL remove = !wcscmp(property, L"REMOVE");
    BOOL reinstall = !wcscmp(property, L"REINSTALL");

    override = msi_dup_property( package->db, property );
    if (!override)
        return FALSE;

    LIST_FOR_EACH_ENTRY( feature, &package->features, MSIFEATURE, entry )
    {
        if (feature->Level <= 0)
            continue;

        if (reinstall)
            state = (feature->Installed == INSTALLSTATE_ABSENT ? INSTALLSTATE_UNKNOWN : feature->Installed);
        else if (remove)
            state = (feature->Installed == INSTALLSTATE_ABSENT ? INSTALLSTATE_UNKNOWN : INSTALLSTATE_ABSENT);

        if (!wcsicmp( override, L"ALL" ))
        {
            feature->Action = state;
            feature->ActionRequest = state;
        }
        else
        {
            LPWSTR ptr = override;
            LPWSTR ptr2 = wcschr(override,',');

            while (ptr)
            {
                int len = ptr2 - ptr;

                if ((ptr2 && lstrlenW(feature->Feature) == len && !wcsncmp(ptr, feature->Feature, len))
                    || (!ptr2 && !wcscmp(ptr, feature->Feature)))
                {
                    feature->Action = state;
                    feature->ActionRequest = state;
                    break;
                }
                if (ptr2)
                {
                    ptr=ptr2+1;
                    ptr2 = wcschr(ptr,',');
                }
                else
                    break;
            }
        }
    }
    free(override);
    return TRUE;
}

static BOOL process_overrides( MSIPACKAGE *package, int level )
{
    BOOL ret = FALSE;

    /* all these activation/deactivation things happen in order and things
     * later on the list override things earlier on the list.
     *
     *  0  INSTALLLEVEL processing
     *  1  ADDLOCAL
     *  2  REMOVE
     *  3  ADDSOURCE
     *  4  ADDDEFAULT
     *  5  REINSTALL
     *  6  ADVERTISE
     *  7  COMPADDLOCAL
     *  8  COMPADDSOURCE
     *  9  FILEADDLOCAL
     * 10  FILEADDSOURCE
     * 11  FILEADDDEFAULT
     */
    ret |= process_state_property( package, level, L"ADDLOCAL", INSTALLSTATE_LOCAL );
    ret |= process_state_property( package, level, L"REMOVE", INSTALLSTATE_ABSENT );
    ret |= process_state_property( package, level, L"ADDSOURCE", INSTALLSTATE_SOURCE );
    ret |= process_state_property( package, level, L"REINSTALL", INSTALLSTATE_UNKNOWN );
    ret |= process_state_property( package, level, L"ADVERTISE", INSTALLSTATE_ADVERTISED );

    if (ret)
        msi_set_property( package->db, L"Preselected", L"1", -1 );

    return ret;
}

static void disable_children( MSIFEATURE *feature, int level )
{
    FeatureList *fl;

    LIST_FOR_EACH_ENTRY( fl, &feature->Children, FeatureList, entry )
    {
        if (!is_feature_selected( feature, level ))
        {
            TRACE("child %s (level %d request %d) follows disabled parent %s (level %d request %d)\n",
                  debugstr_w(fl->feature->Feature), fl->feature->Level, fl->feature->ActionRequest,
                  debugstr_w(feature->Feature), feature->Level, feature->ActionRequest);

            fl->feature->Level = feature->Level;
            fl->feature->Action = INSTALLSTATE_UNKNOWN;
            fl->feature->ActionRequest = INSTALLSTATE_UNKNOWN;
        }
        disable_children( fl->feature, level );
    }
}

static void follow_parent( MSIFEATURE *feature )
{
    FeatureList *fl;

    LIST_FOR_EACH_ENTRY( fl, &feature->Children, FeatureList, entry )
    {
        if (fl->feature->Attributes & msidbFeatureAttributesFollowParent)
        {
            TRACE("child %s (level %d request %d) follows parent %s (level %d request %d)\n",
                  debugstr_w(fl->feature->Feature), fl->feature->Level, fl->feature->ActionRequest,
                  debugstr_w(feature->Feature), feature->Level, feature->ActionRequest);

            fl->feature->Action = feature->Action;
            fl->feature->ActionRequest = feature->ActionRequest;
        }
        follow_parent( fl->feature );
    }
}

UINT MSI_SetFeatureStates(MSIPACKAGE *package)
{
    int level;
    MSICOMPONENT* component;
    MSIFEATURE *feature;

    TRACE("Checking Install Level\n");

    level = msi_get_property_int(package->db, L"INSTALLLEVEL", 1);

    if (msi_get_property_int( package->db, L"Preselected", 0 ))
    {
        LIST_FOR_EACH_ENTRY( feature, &package->features, MSIFEATURE, entry )
        {
            if (!is_feature_selected( feature, level )) continue;

            if (feature->ActionRequest == INSTALLSTATE_UNKNOWN)
            {
                if (feature->Installed == INSTALLSTATE_ABSENT)
                {
                    feature->Action = INSTALLSTATE_UNKNOWN;
                    feature->ActionRequest = INSTALLSTATE_UNKNOWN;
                }
                else
                {
                    feature->Action = feature->Installed;
                    feature->ActionRequest = feature->Installed;
                }
            }
        }
    }
    else if (!msi_get_property_int( package->db, L"Installed", 0 ))
    {
        LIST_FOR_EACH_ENTRY( feature, &package->features, MSIFEATURE, entry )
        {
            if (!is_feature_selected( feature, level )) continue;

            if (feature->ActionRequest == INSTALLSTATE_UNKNOWN)
            {
                if (feature->Attributes & msidbFeatureAttributesFavorSource)
                {
                    feature->Action = INSTALLSTATE_SOURCE;
                    feature->ActionRequest = INSTALLSTATE_SOURCE;
                }
                else if (feature->Attributes & msidbFeatureAttributesFavorAdvertise)
                {
                    feature->Action = INSTALLSTATE_ADVERTISED;
                    feature->ActionRequest = INSTALLSTATE_ADVERTISED;
                }
                else
                {
                    feature->Action = INSTALLSTATE_LOCAL;
                    feature->ActionRequest = INSTALLSTATE_LOCAL;
                }
            }
        }
    }
    else
    {
        LIST_FOR_EACH_ENTRY( feature, &package->features, MSIFEATURE, entry )
        {
            ComponentList *cl;
            MSIFEATURE *cur;

            if (!is_feature_selected( feature, level )) continue;
            if (feature->ActionRequest != INSTALLSTATE_UNKNOWN) continue;

            LIST_FOR_EACH_ENTRY( cl, &feature->Components, ComponentList, entry )
            {
                if (!cl->component->updated && !cl->component->added)
                    continue;

                cur = feature;
                while (cur)
                {
                    if (cur->ActionRequest != INSTALLSTATE_UNKNOWN)
                        break;

                    if (cur->Installed != INSTALLSTATE_ABSENT)
                    {
                        cur->Action = cur->Installed;
                        cur->ActionRequest = cur->Installed;
                    }
                    else if (!cl->component->added)
                    {
                        break;
                    }
                    else if (cur->Attributes & msidbFeatureAttributesFavorSource)
                    {
                        cur->Action = INSTALLSTATE_SOURCE;
                        cur->ActionRequest = INSTALLSTATE_SOURCE;
                    }
                    else if (cur->Attributes & msidbFeatureAttributesFavorAdvertise)
                    {
                        cur->Action = INSTALLSTATE_ADVERTISED;
                        cur->ActionRequest = INSTALLSTATE_ADVERTISED;
                    }
                    else
                    {
                        cur->Action = INSTALLSTATE_LOCAL;
                        cur->ActionRequest = INSTALLSTATE_LOCAL;
                    }

                    if (!cur->Feature_Parent)
                        break;
                    cur = msi_get_loaded_feature(package, cur->Feature_Parent);
                }
            }
        }
    }

    /* disable child features of unselected parent or follow parent */
    LIST_FOR_EACH_ENTRY( feature, &package->features, MSIFEATURE, entry )
    {
        if (feature->Feature_Parent) continue;
        disable_children( feature, level );
        follow_parent( feature );
    }

    /* now we want to set component state based based on feature state */
    LIST_FOR_EACH_ENTRY( feature, &package->features, MSIFEATURE, entry )
    {
        ComponentList *cl;

        TRACE("examining feature %s (level %d installed %d request %d action %d)\n",
              debugstr_w(feature->Feature), feature->Level, feature->Installed,
              feature->ActionRequest, feature->Action);

        /* features with components that have compressed files are made local */
        LIST_FOR_EACH_ENTRY( cl, &feature->Components, ComponentList, entry )
        {
            if (cl->component->ForceLocalState &&
                feature->ActionRequest == INSTALLSTATE_SOURCE)
            {
                feature->Action = INSTALLSTATE_LOCAL;
                feature->ActionRequest = INSTALLSTATE_LOCAL;
                break;
            }
        }

        LIST_FOR_EACH_ENTRY( cl, &feature->Components, ComponentList, entry )
        {
            component = cl->component;

            switch (feature->ActionRequest)
            {
            case INSTALLSTATE_ABSENT:
                component->anyAbsent = 1;
                break;
            case INSTALLSTATE_ADVERTISED:
                component->hasAdvertisedFeature = 1;
                break;
            case INSTALLSTATE_SOURCE:
                component->hasSourceFeature = 1;
                break;
            case INSTALLSTATE_LOCAL:
                component->hasLocalFeature = 1;
                break;
            case INSTALLSTATE_DEFAULT:
                if (feature->Attributes & msidbFeatureAttributesFavorAdvertise)
                    component->hasAdvertisedFeature = 1;
                else if (feature->Attributes & msidbFeatureAttributesFavorSource)
                    component->hasSourceFeature = 1;
                else
                    component->hasLocalFeature = 1;
                break;
            case INSTALLSTATE_UNKNOWN:
                if (feature->Installed == INSTALLSTATE_ADVERTISED)
                    component->hasAdvertisedFeature = 1;
                if (feature->Installed == INSTALLSTATE_SOURCE)
                    component->hasSourceFeature = 1;
                if (feature->Installed == INSTALLSTATE_LOCAL)
                    component->hasLocalFeature = 1;
                break;
            default:
                break;
            }
        }
    }

    LIST_FOR_EACH_ENTRY( component, &package->components, MSICOMPONENT, entry )
    {
        /* check if it's local or source */
        if (!(component->Attributes & msidbComponentAttributesOptional) &&
             (component->hasLocalFeature || component->hasSourceFeature))
        {
            if ((component->Attributes & msidbComponentAttributesSourceOnly) &&
                 !component->ForceLocalState)
            {
                component->Action = INSTALLSTATE_SOURCE;
                component->ActionRequest = INSTALLSTATE_SOURCE;
            }
            else
            {
                component->Action = INSTALLSTATE_LOCAL;
                component->ActionRequest = INSTALLSTATE_LOCAL;
            }
            continue;
        }

        /* if any feature is local, the component must be local too */
        if (component->hasLocalFeature)
        {
            component->Action = INSTALLSTATE_LOCAL;
            component->ActionRequest = INSTALLSTATE_LOCAL;
            continue;
        }
        if (component->hasSourceFeature)
        {
            component->Action = INSTALLSTATE_SOURCE;
            component->ActionRequest = INSTALLSTATE_SOURCE;
            continue;
        }
        if (component->hasAdvertisedFeature)
        {
            component->Action = INSTALLSTATE_ADVERTISED;
            component->ActionRequest = INSTALLSTATE_ADVERTISED;
            continue;
        }
        TRACE("nobody wants component %s\n", debugstr_w(component->Component));
        if (component->anyAbsent && component->ComponentId)
        {
            component->Action = INSTALLSTATE_ABSENT;
            component->ActionRequest = INSTALLSTATE_ABSENT;
        }
    }

    LIST_FOR_EACH_ENTRY( component, &package->components, MSICOMPONENT, entry )
    {
        if (component->ActionRequest == INSTALLSTATE_DEFAULT)
        {
            TRACE("%s was default, setting to local\n", debugstr_w(component->Component));
            component->Action = INSTALLSTATE_LOCAL;
            component->ActionRequest = INSTALLSTATE_LOCAL;
        }

        if (component->ActionRequest == INSTALLSTATE_SOURCE &&
            component->Installed == INSTALLSTATE_SOURCE &&
            component->hasSourceFeature)
        {
            component->Action = INSTALLSTATE_UNKNOWN;
            component->ActionRequest = INSTALLSTATE_UNKNOWN;
        }

        if (component->Action == INSTALLSTATE_LOCAL || component->Action == INSTALLSTATE_SOURCE)
            component->num_clients++;
        else if (component->Action == INSTALLSTATE_ABSENT)
        {
            component->num_clients--;

            if (component->num_clients > 0)
            {
                TRACE("multiple clients uses %s - disallowing uninstallation\n", debugstr_w(component->Component));
                component->Action = INSTALLSTATE_UNKNOWN;
            }
        }

        TRACE("component %s (installed %d request %d action %d)\n",
              debugstr_w(component->Component), component->Installed, component->ActionRequest, component->Action);
    }

    return ERROR_SUCCESS;
}

static UINT ITERATE_CostFinalizeConditions(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE *package = param;
    LPCWSTR name;
    MSIFEATURE *feature;

    name = MSI_RecordGetString( row, 1 );

    feature = msi_get_loaded_feature( package, name );
    if (!feature)
        ERR("FAILED to find loaded feature %s\n",debugstr_w(name));
    else
    {
        LPCWSTR Condition;
        Condition = MSI_RecordGetString(row,3);

        if (MSI_EvaluateConditionW(package,Condition) == MSICONDITION_TRUE)
        {
            int level = MSI_RecordGetInteger(row,2);
            TRACE("Resetting feature %s to level %i\n", debugstr_w(name), level);
            feature->Level = level;
        }
    }
    return ERROR_SUCCESS;
}

int msi_compare_file_versions( VS_FIXEDFILEINFO *fi, const WCHAR *version )
{
    DWORD ms, ls;

    msi_parse_version_string( version, &ms, &ls );

    if (fi->dwFileVersionMS > ms) return 1;
    else if (fi->dwFileVersionMS < ms) return -1;
    else if (fi->dwFileVersionLS > ls) return 1;
    else if (fi->dwFileVersionLS < ls) return -1;
    return 0;
}

int msi_compare_font_versions( const WCHAR *ver1, const WCHAR *ver2 )
{
    DWORD ms1, ms2;

    msi_parse_version_string( ver1, &ms1, NULL );
    msi_parse_version_string( ver2, &ms2, NULL );

    if (ms1 > ms2) return 1;
    else if (ms1 < ms2) return -1;
    return 0;
}

static WCHAR *create_temp_dir( MSIDATABASE *db )
{
    static UINT id;
    WCHAR *ret;

    if (!db->tempfolder)
    {
        WCHAR tmp[MAX_PATH];
        DWORD len = ARRAY_SIZE( tmp );

        if (msi_get_property( db, L"TempFolder", tmp, &len ) ||
            GetFileAttributesW( tmp ) != FILE_ATTRIBUTE_DIRECTORY)
        {
            GetTempPathW( MAX_PATH, tmp );
        }
        if (!(db->tempfolder = wcsdup( tmp ))) return NULL;
    }

    if ((ret = malloc( (wcslen( db->tempfolder ) + 20) * sizeof(WCHAR) )))
    {
        for (;;)
        {
            if (!GetTempFileNameW( db->tempfolder, L"msi", ++id, ret ))
            {
                free( ret );
                return NULL;
            }
            if (CreateDirectoryW( ret, NULL )) break;
        }
    }

    return ret;
}

/*
 *  msi_build_directory_name()
 *
 *  This function is to save messing round with directory names
 *  It handles adding backslashes between path segments,
 *  and can add \ at the end of the directory name if told to.
 *
 *  It takes a variable number of arguments.
 *  It always allocates a new string for the result, so make sure
 *  to free the return value when finished with it.
 *
 *  The first arg is the number of path segments that follow.
 *  The arguments following count are a list of path segments.
 *  A path segment may be NULL.
 *
 *  Path segments will be added with a \ separating them.
 *  A \ will not be added after the last segment, however if the
 *  last segment is NULL, then the last character will be a \
 */
WCHAR * WINAPIV msi_build_directory_name( DWORD count, ... )
{
    DWORD sz = 1, i;
    WCHAR *dir;
    va_list va;

    va_start( va, count );
    for (i = 0; i < count; i++)
    {
        const WCHAR *str = va_arg( va, const WCHAR * );
        if (str) sz += lstrlenW( str ) + 1;
    }
    va_end( va );

    dir = malloc( sz * sizeof(WCHAR) );
    dir[0] = 0;

    va_start( va, count );
    for (i = 0; i < count; i++)
    {
        const WCHAR *str = va_arg( va, const WCHAR * );
        if (!str) continue;
        lstrcatW( dir, str );
        if ( i + 1 != count && dir[0] && dir[lstrlenW( dir ) - 1] != '\\') lstrcatW( dir, L"\\" );
    }
    va_end( va );
    return dir;
}

BOOL msi_is_global_assembly( MSICOMPONENT *comp )
{
    return comp->assembly && !comp->assembly->application;
}

static void set_target_path( MSIPACKAGE *package, MSIFILE *file )
{
    free( file->TargetPath );
    if (msi_is_global_assembly( file->Component ))
    {
        MSIASSEMBLY *assembly = file->Component->assembly;

        if (!assembly->tempdir) assembly->tempdir = create_temp_dir( package->db );
        file->TargetPath = msi_build_directory_name( 2, assembly->tempdir, file->FileName );
    }
    else
    {
        const WCHAR *dir = msi_get_target_folder( package, file->Component->Directory );
        file->TargetPath = msi_build_directory_name( 2, dir, file->FileName );
    }

    TRACE("file %s resolves to %s\n", debugstr_w(file->File), debugstr_w(file->TargetPath));
}

static UINT calculate_file_cost( MSIPACKAGE *package )
{
    VS_FIXEDFILEINFO *file_version;
    WCHAR *font_version;
    MSIFILE *file;

    LIST_FOR_EACH_ENTRY( file, &package->files, MSIFILE, entry )
    {
        MSICOMPONENT *comp = file->Component;
        DWORD file_size;

        if (!comp->Enabled) continue;

        if (file->IsCompressed)
            comp->ForceLocalState = TRUE;

        set_target_path( package, file );

        if (msi_get_file_attributes( package, file->TargetPath ) == INVALID_FILE_ATTRIBUTES)
        {
            comp->cost += cost_from_size( file->FileSize );
            continue;
        }
        file_size = msi_get_disk_file_size( package, file->TargetPath );
        TRACE("%s (size %lu)\n", debugstr_w(file->TargetPath), file_size);

        if (file->Version)
        {
            if ((file_version = msi_get_disk_file_version( package, file->TargetPath )))
            {
                if (msi_compare_file_versions( file_version, file->Version ) < 0)
                {
                    comp->cost += cost_from_size( file->FileSize - file_size );
                }
                free( file_version );
                continue;
            }
            else if ((font_version = msi_get_font_file_version( package, file->TargetPath )))
            {
                if (msi_compare_font_versions( font_version, file->Version ) < 0)
                {
                    comp->cost += cost_from_size( file->FileSize - file_size );
                }
                free( font_version );
                continue;
            }
        }
        if (file_size != file->FileSize)
        {
            comp->cost += cost_from_size( file->FileSize - file_size );
        }
    }

    return ERROR_SUCCESS;
}

WCHAR *msi_normalize_path( const WCHAR *in )
{
    const WCHAR *p = in;
    WCHAR *q, *ret;
    int n, len = lstrlenW( in ) + 2;

    if (!(q = ret = malloc( len * sizeof(WCHAR) ))) return NULL;

    len = 0;
    while (1)
    {
        /* copy until the end of the string or a space */
        while (*p != ' ' && (*q = *p))
        {
            p++, len++;
            /* reduce many backslashes to one */
            if (*p != '\\' || *q != '\\')
                q++;
        }

        /* quit at the end of the string */
        if (!*p)
            break;

        /* count the number of spaces */
        n = 0;
        while (p[n] == ' ')
            n++;

        /* if it's leading or trailing space, skip it */
        if ( len == 0 || p[-1] == '\\' || p[n] == '\\' )
            p += n;
        else  /* copy n spaces */
            while (n && (*q++ = *p++)) n--;
    }
    while (q - ret > 0 && q[-1] == ' ') q--;
    if (q - ret > 0 && q[-1] != '\\')
    {
        q[0] = '\\';
        q[1] = 0;
    }
    return ret;
}

static WCHAR *get_install_location( MSIPACKAGE *package )
{
    HKEY hkey;
    WCHAR *path;

    if (!package->ProductCode) return NULL;
    if (MSIREG_OpenInstallProps( package->ProductCode, package->Context, NULL, &hkey, FALSE )) return NULL;
    if ((path = msi_reg_get_val_str( hkey, L"InstallLocation" )) && !path[0])
    {
        free( path );
        path = NULL;
    }
    RegCloseKey( hkey );
    return path;
}

void msi_resolve_target_folder( MSIPACKAGE *package, const WCHAR *name, BOOL load_prop )
{
    FolderList *fl;
    MSIFOLDER *folder, *parent, *child;
    WCHAR *path, *normalized_path;

    TRACE("resolving %s\n", debugstr_w(name));

    if (!(folder = msi_get_loaded_folder( package, name ))) return;

    if (!wcscmp( folder->Directory, L"TARGETDIR" )) /* special resolving for target root dir */
    {
        if (!(path = get_install_location( package )) &&
            (!load_prop || !(path = msi_dup_property( package->db, L"TARGETDIR" ))))
        {
            path = msi_dup_property( package->db, L"ROOTDRIVE" );
        }
    }
    else if (!load_prop || !(path = msi_dup_property( package->db, folder->Directory )))
    {
        if (folder->Parent && wcscmp( folder->Directory, folder->Parent ))
        {
            parent = msi_get_loaded_folder( package, folder->Parent );
            path = msi_build_directory_name( 3, parent->ResolvedTarget, folder->TargetDefault, NULL );
        }
        else
            path = msi_build_directory_name( 2, folder->TargetDefault, NULL );
    }

    normalized_path = msi_normalize_path( path );
    msi_set_property( package->db, folder->Directory, normalized_path, -1 );
    free( path );

    free( folder->ResolvedTarget );
    folder->ResolvedTarget = normalized_path;

    LIST_FOR_EACH_ENTRY( fl, &folder->children, FolderList, entry )
    {
        child = fl->folder;
        msi_resolve_target_folder( package, child->Directory, load_prop );
    }
    TRACE("%s resolves to %s\n", debugstr_w(name), debugstr_w(folder->ResolvedTarget));
}

static ULONGLONG get_volume_space_required( MSIPACKAGE *package )
{
    MSICOMPONENT *comp;
    ULONGLONG ret = 0;

    LIST_FOR_EACH_ENTRY( comp, &package->components, MSICOMPONENT, entry )
    {
        if (comp->Action == INSTALLSTATE_LOCAL) ret += comp->cost;
    }
    return ret;
}

static UINT ACTION_CostFinalize(MSIPACKAGE *package)
{
    MSICOMPONENT *comp;
    MSIQUERY *view;
    WCHAR *level, *primary_key, *primary_folder;
    UINT rc;

    TRACE("Building directory properties\n");
    msi_resolve_target_folder( package, L"TARGETDIR", TRUE );

    TRACE("Evaluating component conditions\n");
    LIST_FOR_EACH_ENTRY( comp, &package->components, MSICOMPONENT, entry )
    {
        if (MSI_EvaluateConditionW( package, comp->Condition ) == MSICONDITION_FALSE)
        {
            TRACE("Disabling component %s\n", debugstr_w(comp->Component));
            comp->Enabled = FALSE;
        }
        else
            comp->Enabled = TRUE;
    }
    get_client_counts( package );

    /* read components states from the registry */
    ACTION_GetComponentInstallStates(package);
    ACTION_GetFeatureInstallStates(package);

    if (!process_overrides( package, msi_get_property_int( package->db, L"INSTALLLEVEL", 1 ) ))
    {
        TRACE("Evaluating feature conditions\n");

        rc = MSI_DatabaseOpenViewW( package->db, L"SELECT * FROM `Condition`", &view );
        if (rc == ERROR_SUCCESS)
        {
            rc = MSI_IterateRecords( view, NULL, ITERATE_CostFinalizeConditions, package );
            msiobj_release( &view->hdr );
            if (rc != ERROR_SUCCESS)
                return rc;
        }
    }

    TRACE("Calculating file cost\n");
    calculate_file_cost( package );

    msi_set_property( package->db, L"CostingComplete", L"1", -1 );
    /* set default run level if not set */
    level = msi_dup_property( package->db, L"INSTALLLEVEL" );
    if (!level) msi_set_property( package->db, L"INSTALLLEVEL", L"1", -1 );
    free(level);

    if ((rc = MSI_SetFeatureStates( package ))) return rc;

    if ((primary_key = msi_dup_property( package->db, L"PRIMARYFOLDER" )))
    {
        if ((primary_folder = msi_dup_property( package->db, primary_key )))
        {
            if (((primary_folder[0] >= 'A' && primary_folder[0] <= 'Z') ||
                 (primary_folder[0] >= 'a' && primary_folder[0] <= 'z')) && primary_folder[1] == ':')
            {
                ULARGE_INTEGER free;
                ULONGLONG required;
                WCHAR buf[21];

                primary_folder[2] = 0;
                if (GetDiskFreeSpaceExW( primary_folder, &free, NULL, NULL ))
                {
#ifdef __REACTOS__
                    swprintf(buf, ARRAY_SIZE(buf), L"%I64u", free.QuadPart / 512);
#else
                    swprintf( buf, ARRAY_SIZE(buf), L"%lu", free.QuadPart / 512 );
#endif
                    msi_set_property( package->db, L"PrimaryVolumeSpaceAvailable", buf, -1 );
                }
                required = get_volume_space_required( package );
#ifdef __REACTOS__
                swprintf( buf, ARRAY_SIZE(buf), L"%I64u", required );
#else
                swprintf( buf, ARRAY_SIZE(buf), L"%lu", required );
#endif
                msi_set_property( package->db, L"PrimaryVolumeSpaceRequired", buf, -1 );

#ifdef __REACTOS__
                swprintf( buf, ARRAY_SIZE(buf), L"%I64u", (free.QuadPart / 512) - required );
#else
                swprintf( buf, ARRAY_SIZE(buf), L"%lu", (free.QuadPart / 512) - required );
#endif
                msi_set_property( package->db, L"PrimaryVolumeSpaceRemaining", buf, -1 );
                msi_set_property( package->db, L"PrimaryVolumePath", primary_folder, 2 );
            }
            free( primary_folder );
        }
        free( primary_key );
    }

    /* FIXME: check volume disk space */
    msi_set_property( package->db, L"OutOfDiskSpace", L"0", -1 );
    msi_set_property( package->db, L"OutOfNoRbDiskSpace", L"0", -1 );

    return ERROR_SUCCESS;
}

static BYTE *parse_value( MSIPACKAGE *package, const WCHAR *value, DWORD len, DWORD *type, DWORD *size )
{
    BYTE *data;

    if (!value)
    {
        *size = sizeof(WCHAR);
        *type = REG_SZ;
        if ((data = malloc( *size ))) *(WCHAR *)data = 0;
        return data;
    }
    if (value[0]=='#' && value[1]!='#' && value[1]!='%')
    {
        if (value[1]=='x')
        {
            LPWSTR ptr;
            CHAR byte[5];
            LPWSTR deformated = NULL;
            int count;

            deformat_string(package, &value[2], &deformated);

            /* binary value type */
            ptr = deformated;
            *type = REG_BINARY;
            if (lstrlenW(ptr)%2)
                *size = (lstrlenW(ptr)/2)+1;
            else
                *size = lstrlenW(ptr)/2;

            data = malloc(*size);

            byte[0] = '0'; 
            byte[1] = 'x'; 
            byte[4] = 0; 
            count = 0;
            /* if uneven pad with a zero in front */
            if (lstrlenW(ptr)%2)
            {
                byte[2]= '0';
                byte[3]= *ptr;
                ptr++;
                data[count] = (BYTE)strtol(byte,NULL,0);
                count ++;
                TRACE("Uneven byte count\n");
            }
            while (*ptr)
            {
                byte[2]= *ptr;
                ptr++;
                byte[3]= *ptr;
                ptr++;
                data[count] = (BYTE)strtol(byte,NULL,0);
                count ++;
            }
            free(deformated);

            TRACE( "data %lu bytes(%u)\n", *size, count );
        }
        else
        {
            LPWSTR deformated;
            LPWSTR p;
            DWORD d = 0;
            deformat_string(package, &value[1], &deformated);

            *type=REG_DWORD; 
            *size = sizeof(DWORD);
            data = malloc(*size);
            p = deformated;
            if (*p == '-')
                p++;
            while (*p)
            {
                if ( (*p < '0') || (*p > '9') )
                    break;
                d *= 10;
                d += (*p - '0');
                p++;
            }
            if (deformated[0] == '-')
                d = -d;
            *(DWORD *)data = d;
            TRACE( "DWORD %lu\n", *(DWORD *)data);

            free(deformated);
        }
    }
    else
    {
        const WCHAR *ptr = value;

        *type = REG_SZ;
        if (value[0] == '#')
        {
            ptr++; len--;
            if (value[1] == '%')
            {
                ptr++; len--;
                *type = REG_EXPAND_SZ;
            }
        }
        data = (BYTE *)msi_strdupW( ptr, len );
        if (len > lstrlenW( (const WCHAR *)data )) *type = REG_MULTI_SZ;
        *size = (len + 1) * sizeof(WCHAR);
    }
    return data;
}

static const WCHAR *get_root_key( MSIPACKAGE *package, INT root, HKEY *root_key )
{
    const WCHAR *ret;

    switch (root)
    {
    case -1:
        if (msi_get_property_int( package->db, L"ALLUSERS", 0 ))
        {
            *root_key = HKEY_LOCAL_MACHINE;
            ret = L"HKEY_LOCAL_MACHINE\\";
        }
        else
        {
            *root_key = HKEY_CURRENT_USER;
            ret = L"HKEY_CURRENT_USER\\";
        }
        break;
    case 0:
        *root_key = HKEY_CLASSES_ROOT;
        ret = L"HKEY_CLASSES_ROOT\\";
        break;
    case 1:
        *root_key = HKEY_CURRENT_USER;
        ret = L"HKEY_CURRENT_USER\\";
        break;
    case 2:
        *root_key = HKEY_LOCAL_MACHINE;
        ret = L"HKEY_LOCAL_MACHINE\\";
        break;
    case 3:
        *root_key = HKEY_USERS;
        ret = L"HKEY_USERS\\";
        break;
    default:
        ERR("Unknown root %i\n", root);
        return NULL;
    }

    return ret;
}

static inline REGSAM get_registry_view( const MSICOMPONENT *comp )
{
    REGSAM view = 0;
    if (is_wow64 || is_64bit)
        view |= (comp->Attributes & msidbComponentAttributes64bit) ? KEY_WOW64_64KEY : KEY_WOW64_32KEY;
    return view;
}

static HKEY open_key( const MSICOMPONENT *comp, HKEY root, const WCHAR *path, BOOL create, REGSAM access )
{
    WCHAR *subkey, *p, *q;
    HKEY hkey, ret = NULL;
    LONG res;

    access |= get_registry_view( comp );

    if (!(subkey = wcsdup( path ))) return NULL;
    p = subkey;
    if ((q = wcschr( p, '\\' ))) *q = 0;
    if (create)
        res = RegCreateKeyExW( root, subkey, 0, NULL, 0, access, NULL, &hkey, NULL );
    else
        res = RegOpenKeyExW( root, subkey, 0, access, &hkey );
    if (res)
    {
        TRACE( "failed to open key %s (%ld)\n", debugstr_w(subkey), res );
        free( subkey );
        return NULL;
    }
    if (q && q[1])
    {
        ret = open_key( comp, hkey, q + 1, create, access );
        RegCloseKey( hkey );
    }
    else ret = hkey;
    free( subkey );
    return ret;
}

static BOOL is_special_entry( const WCHAR *name )
{
     return (name && (name[0] == '*' || name[0] == '+') && !name[1]);
}

static WCHAR **split_multi_string_values( const WCHAR *str, DWORD len, DWORD *count )
{
    const WCHAR *p = str;
    WCHAR **ret;
    int i = 0;

    *count = 0;
    if (!str) return NULL;
    while ((p - str) < len)
    {
        p += lstrlenW( p ) + 1;
        (*count)++;
    }
    if (!(ret = malloc( *count * sizeof(WCHAR *) ))) return NULL;
    p = str;
    while ((p - str) < len)
    {
        if (!(ret[i] = wcsdup( p )))
        {
            for (; i >= 0; i--) free( ret[i] );
            free( ret );
            return NULL;
        }
        p += lstrlenW( p ) + 1;
        i++;
    }
    return ret;
}

static WCHAR *flatten_multi_string_values( WCHAR **left, DWORD left_count,
                                           WCHAR **right, DWORD right_count, DWORD *size )
{
    WCHAR *ret, *p;
    unsigned int i;

    *size = sizeof(WCHAR);
    for (i = 0; i < left_count; i++) *size += (lstrlenW( left[i] ) + 1) * sizeof(WCHAR);
    for (i = 0; i < right_count; i++) *size += (lstrlenW( right[i] ) + 1) * sizeof(WCHAR);

    if (!(ret = p = malloc( *size ))) return NULL;

    for (i = 0; i < left_count; i++)
    {
        lstrcpyW( p, left[i] );
        p += lstrlenW( p ) + 1;
    }
    for (i = 0; i < right_count; i++)
    {
        lstrcpyW( p, right[i] );
        p += lstrlenW( p ) + 1;
    }
    *p = 0;
    return ret;
}

static DWORD remove_duplicate_values( WCHAR **old, DWORD old_count,
                                      WCHAR **new, DWORD new_count )
{
    DWORD ret = old_count;
    unsigned int i, j, k;

    for (i = 0; i < new_count; i++)
    {
        for (j = 0; j < old_count; j++)
        {
            if (old[j] && !wcscmp( new[i], old[j] ))
            {
                free( old[j] );
                for (k = j; k < old_count - 1; k++) { old[k] = old[k + 1]; }
                old[k] = NULL;
                ret--;
            }
        }
    }
    return ret;
}

enum join_op
{
    JOIN_OP_APPEND,
    JOIN_OP_PREPEND,
    JOIN_OP_REPLACE
};

static WCHAR *join_multi_string_values( enum join_op op, WCHAR **old, DWORD old_count,
                                        WCHAR **new, DWORD new_count, DWORD *size )
{
    switch (op)
    {
    case JOIN_OP_APPEND:
        old_count = remove_duplicate_values( old, old_count, new, new_count );
        return flatten_multi_string_values( old, old_count, new, new_count, size );

    case JOIN_OP_PREPEND:
        old_count = remove_duplicate_values( old, old_count, new, new_count );
        return flatten_multi_string_values( new, new_count, old, old_count, size );

    case JOIN_OP_REPLACE:
        return flatten_multi_string_values( new, new_count, NULL, 0, size );

    default:
        ERR("unhandled join op %u\n", op);
        return NULL;
    }
}

static BYTE *build_multi_string_value( BYTE *old_value, DWORD old_size,
                                       BYTE *new_value, DWORD new_size, DWORD *size )
{
    DWORD i, old_len = 0, new_len = 0, old_count = 0, new_count = 0;
    const WCHAR *new_ptr = NULL, *old_ptr = NULL;
    enum join_op op = JOIN_OP_REPLACE;
    WCHAR **old = NULL, **new = NULL;
    BYTE *ret;

    if (new_size / sizeof(WCHAR) - 1 > 1)
    {
        new_ptr = (const WCHAR *)new_value;
        new_len = new_size / sizeof(WCHAR) - 1;

        if (!new_ptr[0] && new_ptr[new_len - 1])
        {
            op = JOIN_OP_APPEND;
            new_len--;
            new_ptr++;
        }
        else if (new_ptr[0] && !new_ptr[new_len - 1])
        {
            op = JOIN_OP_PREPEND;
            new_len--;
        }
        else if (new_len > 2 && !new_ptr[0] && !new_ptr[new_len - 1])
        {
            op = JOIN_OP_REPLACE;
            new_len -= 2;
            new_ptr++;
        }
        new = split_multi_string_values( new_ptr, new_len, &new_count );
    }
    if (old_size / sizeof(WCHAR) - 1 > 1)
    {
        old_ptr = (const WCHAR *)old_value;
        old_len = old_size / sizeof(WCHAR) - 1;
        old = split_multi_string_values( old_ptr, old_len, &old_count );
    }
    ret = (BYTE *)join_multi_string_values( op, old, old_count, new, new_count, size );
    for (i = 0; i < old_count; i++) free( old[i] );
    for (i = 0; i < new_count; i++) free( new[i] );
    free( old );
    free( new );
    return ret;
}

static BYTE *reg_get_value( HKEY hkey, const WCHAR *name, DWORD *type, DWORD *size )
{
    BYTE *ret;
    if (RegQueryValueExW( hkey, name, NULL, NULL, NULL, size )) return NULL;
    if (!(ret = malloc( *size ))) return NULL;
    RegQueryValueExW( hkey, name, NULL, type, ret, size );
    return ret;
}

static UINT ITERATE_WriteRegistryValues(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE *package = param;
    BYTE *new_value, *old_value = NULL;
    HKEY  root_key, hkey;
    DWORD type, old_type, new_size, old_size = 0;
    LPWSTR deformated, uikey;
    const WCHAR *szRoot, *component, *name, *key, *str;
    MSICOMPONENT *comp;
    MSIRECORD * uirow;
    INT   root;
    BOOL check_first = FALSE;
    int len;

    msi_ui_progress( package, 2, REG_PROGRESS_VALUE, 0, 0 );

    component = MSI_RecordGetString(row, 6);
    comp = msi_get_loaded_component(package,component);
    if (!comp)
        return ERROR_SUCCESS;

    comp->Action = msi_get_component_action( package, comp );
    if (comp->Action != INSTALLSTATE_LOCAL && comp->Action != INSTALLSTATE_SOURCE)
    {
        TRACE("component not scheduled for installation %s\n", debugstr_w(component));
        return ERROR_SUCCESS;
    }

    name = MSI_RecordGetString(row, 4);
    if( MSI_RecordIsNull(row,5) && name )
    {
        /* null values can have special meanings */
        if (name[0]=='-' && name[1] == 0)
                return ERROR_SUCCESS;
        if ((name[0] == '+' || name[0] == '*') && !name[1])
            check_first = TRUE;
    }

    root = MSI_RecordGetInteger(row,2);
    key = MSI_RecordGetString(row, 3);

    szRoot = get_root_key( package, root, &root_key );
    if (!szRoot)
        return ERROR_SUCCESS;

    deformat_string(package, key , &deformated);
    uikey = malloc( (wcslen(deformated) + wcslen(szRoot) + 1) * sizeof(WCHAR) );
    lstrcpyW(uikey,szRoot);
    lstrcatW(uikey,deformated);

    if (!(hkey = open_key( comp, root_key, deformated, TRUE, KEY_QUERY_VALUE | KEY_SET_VALUE )))
    {
        ERR("Could not create key %s\n", debugstr_w(deformated));
        free(uikey);
        free(deformated);
        return ERROR_FUNCTION_FAILED;
    }
    free( deformated );
    str = msi_record_get_string( row, 5, NULL );
    len = deformat_string( package, str, &deformated );
    new_value = parse_value( package, deformated, len, &type, &new_size );

    free( deformated );
    deformat_string(package, name, &deformated);

    if (!is_special_entry( name ))
    {
        old_value = reg_get_value( hkey, deformated, &old_type, &old_size );
        if (type == REG_MULTI_SZ)
        {
            BYTE *new;
            if (old_value && old_type != REG_MULTI_SZ)
            {
                free( old_value );
                old_value = NULL;
                old_size = 0;
            }
            new = build_multi_string_value( old_value, old_size, new_value, new_size, &new_size );
            free( new_value );
            new_value = new;
        }
        if (!check_first)
        {
            TRACE( "setting value %s of %s type %lu\n", debugstr_w(deformated), debugstr_w(uikey), type );
            RegSetValueExW( hkey, deformated, 0, type, new_value, new_size );
        }
        else if (!old_value)
        {
            if (deformated || new_size)
            {
                TRACE( "setting value %s of %s type %lu\n", debugstr_w(deformated), debugstr_w(uikey), type );
                RegSetValueExW( hkey, deformated, 0, type, new_value, new_size );
            }
        }
        else TRACE("not overwriting existing value %s of %s\n", debugstr_w(deformated), debugstr_w(uikey));
    }
    RegCloseKey(hkey);

    uirow = MSI_CreateRecord(3);
    MSI_RecordSetStringW(uirow,2,deformated);
    MSI_RecordSetStringW(uirow,1,uikey);
    if (type == REG_SZ || type == REG_EXPAND_SZ)
        MSI_RecordSetStringW(uirow, 3, (LPWSTR)new_value);
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
    msiobj_release( &uirow->hdr );

    free(new_value);
    free(old_value);
    free(deformated);
    free(uikey);

    return ERROR_SUCCESS;
}

static UINT ACTION_WriteRegistryValues(MSIPACKAGE *package)
{
    MSIQUERY *view;
    UINT rc;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action(package, SCRIPT_INSTALL, L"WriteRegistryValues");

    rc = MSI_DatabaseOpenViewW(package->db, L"SELECT * FROM `Registry`", &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords(view, NULL, ITERATE_WriteRegistryValues, package);
    msiobj_release(&view->hdr);
    return rc;
}

static int is_key_empty(const MSICOMPONENT *comp, HKEY root, const WCHAR *path)
{
    DWORD subkeys, values;
    HKEY key;
    LONG res;

    key = open_key(comp, root, path, FALSE, KEY_READ);
    if (!key) return 0;

    res = RegQueryInfoKeyW(key, 0, 0, 0, &subkeys, 0, 0, &values, 0, 0, 0, 0);
    RegCloseKey(key);

    return !res && !subkeys && !values;
}

static void delete_key( const MSICOMPONENT *comp, HKEY root, const WCHAR *path )
{
    LONG res = ERROR_SUCCESS;
    REGSAM access = get_registry_view( comp );
    WCHAR *subkey, *p;
    HKEY hkey;

    if (!(subkey = wcsdup( path ))) return;
    do
    {
        if ((p = wcsrchr( subkey, '\\' )))
        {
            *p = 0;
            if (!p[1]) continue; /* trailing backslash */
            hkey = open_key( comp, root, subkey, FALSE, READ_CONTROL );
            if (!hkey) break;
            if (!is_key_empty(comp, hkey, p + 1))
            {
                RegCloseKey(hkey);
                break;
            }
            res = RegDeleteKeyExW( hkey, p + 1, access, 0 );
            RegCloseKey( hkey );
        }
        else if (is_key_empty(comp, root, subkey))
            res = RegDeleteKeyExW( root, subkey, access, 0 );
        if (res)
        {
            TRACE( "failed to delete key %s (%ld)\n", debugstr_w(subkey), res );
            break;
        }
    } while (p);
    free( subkey );
}

static void delete_value( const MSICOMPONENT *comp, HKEY root, const WCHAR *path, const WCHAR *value )
{
    LONG res;
    HKEY hkey;

    if ((hkey = open_key( comp, root, path, FALSE, KEY_SET_VALUE | KEY_QUERY_VALUE )))
    {
        if ((res = RegDeleteValueW( hkey, value )))
            TRACE( "failed to delete value %s (%ld)\n", debugstr_w(value), res );

        RegCloseKey( hkey );
        if (is_key_empty(comp, root, path))
        {
            TRACE("removing empty key %s\n", debugstr_w(path));
            delete_key( comp, root, path );
        }
    }
}

static void delete_tree( const MSICOMPONENT *comp, HKEY root, const WCHAR *path )
{
    LONG res;
    HKEY hkey;

    if (!(hkey = open_key( comp, root, path, FALSE, KEY_ALL_ACCESS ))) return;
    res = RegDeleteTreeW( hkey, NULL );
    if (res) TRACE( "failed to delete subtree of %s (%ld)\n", debugstr_w(path), res );
    delete_key( comp, root, path );
    RegCloseKey( hkey );
}

static UINT ITERATE_RemoveRegistryValuesOnUninstall( MSIRECORD *row, LPVOID param )
{
    MSIPACKAGE *package = param;
    LPCWSTR component, name, key_str, root_key_str;
    LPWSTR deformated_key, deformated_name, ui_key_str;
    MSICOMPONENT *comp;
    MSIRECORD *uirow;
    BOOL delete_key = FALSE;
    HKEY hkey_root;
    UINT size;
    INT root;

    msi_ui_progress( package, 2, REG_PROGRESS_VALUE, 0, 0 );

    component = MSI_RecordGetString( row, 6 );
    comp = msi_get_loaded_component( package, component );
    if (!comp)
        return ERROR_SUCCESS;

    comp->Action = msi_get_component_action( package, comp );
    if (comp->Action != INSTALLSTATE_ABSENT)
    {
        TRACE("component not scheduled for removal %s\n", debugstr_w(component));
        return ERROR_SUCCESS;
    }

    name = MSI_RecordGetString( row, 4 );
    if (MSI_RecordIsNull( row, 5 ) && name )
    {
        if (name[0] == '+' && !name[1])
            return ERROR_SUCCESS;
        if ((name[0] == '-' || name[0] == '*') && !name[1])
        {
            delete_key = TRUE;
            name = NULL;
        }
    }

    root = MSI_RecordGetInteger( row, 2 );
    key_str = MSI_RecordGetString( row, 3 );

    root_key_str = get_root_key( package, root, &hkey_root );
    if (!root_key_str)
        return ERROR_SUCCESS;

    deformat_string( package, key_str, &deformated_key );
    size = lstrlenW( deformated_key ) + lstrlenW( root_key_str ) + 1;
    ui_key_str = malloc( size * sizeof(WCHAR) );
    lstrcpyW( ui_key_str, root_key_str );
    lstrcatW( ui_key_str, deformated_key );

    deformat_string( package, name, &deformated_name );

    if (delete_key) delete_tree( comp, hkey_root, deformated_key );
    else delete_value( comp, hkey_root, deformated_key, deformated_name );
    free( deformated_key );

    uirow = MSI_CreateRecord( 2 );
    MSI_RecordSetStringW( uirow, 1, ui_key_str );
    MSI_RecordSetStringW( uirow, 2, deformated_name );
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
    msiobj_release( &uirow->hdr );

    free( ui_key_str );
    free( deformated_name );
    return ERROR_SUCCESS;
}

static UINT ITERATE_RemoveRegistryValuesOnInstall( MSIRECORD *row, LPVOID param )
{
    MSIPACKAGE *package = param;
    LPCWSTR component, name, key_str, root_key_str;
    LPWSTR deformated_key, deformated_name, ui_key_str;
    MSICOMPONENT *comp;
    MSIRECORD *uirow;
    BOOL delete_key = FALSE;
    HKEY hkey_root;
    UINT size;
    INT root;

    component = MSI_RecordGetString( row, 5 );
    comp = msi_get_loaded_component( package, component );
    if (!comp)
        return ERROR_SUCCESS;

    comp->Action = msi_get_component_action( package, comp );
    if (comp->Action != INSTALLSTATE_LOCAL)
    {
        TRACE("component not scheduled for installation %s\n", debugstr_w(component));
        return ERROR_SUCCESS;
    }

    if ((name = MSI_RecordGetString( row, 4 )))
    {
        if (name[0] == '-' && !name[1])
        {
            delete_key = TRUE;
            name = NULL;
        }
    }

    root = MSI_RecordGetInteger( row, 2 );
    key_str = MSI_RecordGetString( row, 3 );

    root_key_str = get_root_key( package, root, &hkey_root );
    if (!root_key_str)
        return ERROR_SUCCESS;

    deformat_string( package, key_str, &deformated_key );
    size = lstrlenW( deformated_key ) + lstrlenW( root_key_str ) + 1;
    ui_key_str = malloc( size * sizeof(WCHAR) );
    lstrcpyW( ui_key_str, root_key_str );
    lstrcatW( ui_key_str, deformated_key );

    deformat_string( package, name, &deformated_name );

    if (delete_key) delete_tree( comp, hkey_root, deformated_key );
    else delete_value( comp, hkey_root, deformated_key, deformated_name );
    free( deformated_key );

    uirow = MSI_CreateRecord( 2 );
    MSI_RecordSetStringW( uirow, 1, ui_key_str );
    MSI_RecordSetStringW( uirow, 2, deformated_name );
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
    msiobj_release( &uirow->hdr );

    free( ui_key_str );
    free( deformated_name );
    return ERROR_SUCCESS;
}

static UINT ACTION_RemoveRegistryValues( MSIPACKAGE *package )
{
    MSIQUERY *view;
    UINT rc;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action(package, SCRIPT_INSTALL, L"RemoveRegistryValues");

    rc = MSI_DatabaseOpenViewW( package->db, L"SELECT * FROM `Registry`", &view );
    if (rc == ERROR_SUCCESS)
    {
        rc = MSI_IterateRecords( view, NULL, ITERATE_RemoveRegistryValuesOnUninstall, package );
        msiobj_release( &view->hdr );
        if (rc != ERROR_SUCCESS)
            return rc;
    }
    rc = MSI_DatabaseOpenViewW( package->db, L"SELECT * FROM `RemoveRegistry`", &view );
    if (rc == ERROR_SUCCESS)
    {
        rc = MSI_IterateRecords( view, NULL, ITERATE_RemoveRegistryValuesOnInstall, package );
        msiobj_release( &view->hdr );
        if (rc != ERROR_SUCCESS)
            return rc;
    }
    return ERROR_SUCCESS;
}

static UINT ACTION_InstallInitialize(MSIPACKAGE *package)
{
    return ERROR_SUCCESS;
}


static UINT ACTION_InstallValidate(MSIPACKAGE *package)
{
    MSICOMPONENT *comp;
    DWORD total = 0, count = 0;
    MSIQUERY *view;
    MSIFEATURE *feature;
    MSIFILE *file;
    UINT rc;

    TRACE("InstallValidate\n");

    rc = MSI_DatabaseOpenViewW( package->db, L"SELECT * FROM `Registry`", &view );
    if (rc == ERROR_SUCCESS)
    {
        rc = MSI_IterateRecords( view, &count, NULL, package );
        msiobj_release( &view->hdr );
        if (rc != ERROR_SUCCESS)
            return rc;
        total += count * REG_PROGRESS_VALUE;
    }
    LIST_FOR_EACH_ENTRY( comp, &package->components, MSICOMPONENT, entry )
        total += COMPONENT_PROGRESS_VALUE;

    LIST_FOR_EACH_ENTRY( file, &package->files, MSIFILE, entry )
        total += file->FileSize;

    msi_ui_progress( package, 0, total, 0, 0 );

    LIST_FOR_EACH_ENTRY( feature, &package->features, MSIFEATURE, entry )
    {
        TRACE("Feature: %s Installed %d Request %d Action %d\n",
              debugstr_w(feature->Feature), feature->Installed,
              feature->ActionRequest, feature->Action);
    }
    return ERROR_SUCCESS;
}

static UINT ITERATE_LaunchConditions(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE* package = param;
    const WCHAR *cond, *message;
    UINT r;

    cond = MSI_RecordGetString(row, 1);
    r = MSI_EvaluateConditionW(package, cond);
    if (r == MSICONDITION_FALSE)
    {
        if ((package->ui_level & INSTALLUILEVEL_MASK) != INSTALLUILEVEL_NONE)
        {
            WCHAR *deformated;
            message = MSI_RecordGetString(row, 2);
            deformat_string(package, message, &deformated);
            MessageBoxW(NULL, deformated, L"Install Failed", MB_OK);
            free(deformated);
        }

        return ERROR_INSTALL_FAILURE;
    }

    return ERROR_SUCCESS;
}

static UINT ACTION_LaunchConditions(MSIPACKAGE *package)
{
    MSIQUERY *view;
    UINT rc;

    TRACE("Checking launch conditions\n");

    rc = MSI_DatabaseOpenViewW(package->db, L"SELECT * FROM `LaunchCondition`", &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords(view, NULL, ITERATE_LaunchConditions, package);
    msiobj_release(&view->hdr);
    return rc;
}

static LPWSTR resolve_keypath( MSIPACKAGE* package, MSICOMPONENT *cmp )
{

    if (!cmp->KeyPath)
        return wcsdup( msi_get_target_folder( package, cmp->Directory ) );

    if (cmp->Attributes & msidbComponentAttributesRegistryKeyPath)
    {
        MSIRECORD *row;
        UINT root, len;
        LPWSTR deformated, buffer, deformated_name;
        LPCWSTR key, name;

        row = MSI_QueryGetRecord(package->db, L"SELECT * FROM `Registry` WHERE `Registry` = '%s'", cmp->KeyPath);
        if (!row)
            return NULL;

        root = MSI_RecordGetInteger(row,2);
        key = MSI_RecordGetString(row, 3);
        name = MSI_RecordGetString(row, 4);
        deformat_string(package, key , &deformated);
        deformat_string(package, name, &deformated_name);

        len = lstrlenW(deformated) + 6;
        if (deformated_name)
            len+=lstrlenW(deformated_name);

        buffer = malloc(len * sizeof(WCHAR));

        if (deformated_name)
            swprintf(buffer, len, L"%02d:\\%s\\%s", root, deformated, deformated_name);
        else
            swprintf(buffer, len, L"%02d:\\%s\\", root, deformated);

        free(deformated);
        free(deformated_name);
        msiobj_release(&row->hdr);

        return buffer;
    }
    else if (cmp->Attributes & msidbComponentAttributesODBCDataSource)
    {
        FIXME("UNIMPLEMENTED keypath as ODBC Source\n");
        return NULL;
    }
    else
    {
        MSIFILE *file = msi_get_loaded_file( package, cmp->KeyPath );

        if (file)
            return wcsdup( file->TargetPath );
    }
    return NULL;
}

static HKEY open_shared_dlls_key( MSICOMPONENT *comp, BOOL create, REGSAM access )
{
    return open_key( comp, HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\SharedDLLs",
                     create, access );
}

static UINT get_shared_dlls_count( MSICOMPONENT *comp )
{
    DWORD count, type, sz = sizeof(count);
    HKEY hkey = open_shared_dlls_key( comp, FALSE, KEY_READ );
    if (RegQueryValueExW( hkey, comp->FullKeypath, NULL, &type, (BYTE *)&count, &sz )) count = 0;
    RegCloseKey( hkey );
    return count;
}

static void write_shared_dlls_count( MSICOMPONENT *comp, const WCHAR *path, INT count )
{
    HKEY hkey = open_shared_dlls_key( comp, TRUE, KEY_SET_VALUE );
    if (count > 0)
        msi_reg_set_val_dword( hkey, path, count );
    else
        RegDeleteValueW( hkey, path );
    RegCloseKey(hkey);
}

static void refcount_component( MSIPACKAGE *package, MSICOMPONENT *comp )
{
    MSIFEATURE *feature;
    INT count = 0;
    BOOL write = FALSE;

    /* only refcount DLLs */
    if (!comp->KeyPath || comp->assembly || comp->Attributes & msidbComponentAttributesRegistryKeyPath ||
        comp->Attributes & msidbComponentAttributesODBCDataSource)
        write = FALSE;
    else
    {
        count = get_shared_dlls_count( comp );
        write = (count > 0);
        if (comp->Attributes & msidbComponentAttributesSharedDllRefCount)
            write = TRUE;
    }

    /* increment counts */
    LIST_FOR_EACH_ENTRY( feature, &package->features, MSIFEATURE, entry )
    {
        ComponentList *cl;

        if (msi_get_feature_action( package, feature ) != INSTALLSTATE_LOCAL)
            continue;

        LIST_FOR_EACH_ENTRY( cl, &feature->Components, ComponentList, entry )
        {
            if ( cl->component == comp )
                count++;
        }
    }

    /* decrement counts */
    LIST_FOR_EACH_ENTRY( feature, &package->features, MSIFEATURE, entry )
    {
        ComponentList *cl;

        if (msi_get_feature_action( package, feature ) != INSTALLSTATE_ABSENT)
            continue;

        LIST_FOR_EACH_ENTRY( cl, &feature->Components, ComponentList, entry )
        {
            if ( cl->component == comp )
                count--;
        }
    }

    /* ref count all the files in the component */
    if (write)
    {
        MSIFILE *file;

        LIST_FOR_EACH_ENTRY( file, &package->files, MSIFILE, entry )
        {
            if (file->Component == comp)
                write_shared_dlls_count( comp, file->TargetPath, count );
        }
    }

    /* add a count for permanent */
    if (comp->Attributes & msidbComponentAttributesPermanent)
        count ++;

    comp->RefCount = count;

    if (write)
        write_shared_dlls_count( comp, comp->FullKeypath, comp->RefCount );
}

static WCHAR *build_full_keypath( MSIPACKAGE *package, MSICOMPONENT *comp )
{
    if (comp->assembly)
    {
        DWORD len = lstrlenW( L"<\\" ) + lstrlenW( comp->assembly->display_name );
        WCHAR *keypath = malloc( (len + 1) * sizeof(WCHAR) );

        if (keypath)
        {
            lstrcpyW( keypath, L"<\\" );
            lstrcatW( keypath, comp->assembly->display_name );
        }
        return keypath;
    }
    return resolve_keypath( package, comp );
}

static UINT ACTION_ProcessComponents(MSIPACKAGE *package)
{
    WCHAR squashed_pc[SQUASHED_GUID_SIZE], squashed_cc[SQUASHED_GUID_SIZE];
    UINT rc;
    MSICOMPONENT *comp;
    HKEY hkey;

    TRACE("\n");

    msi_set_sourcedir_props(package, FALSE);

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action(package, SCRIPT_INSTALL, L"ProcessComponents");

    squash_guid( package->ProductCode, squashed_pc );

    LIST_FOR_EACH_ENTRY( comp, &package->components, MSICOMPONENT, entry )
    {
        MSIRECORD *uirow;
        INSTALLSTATE action;

        msi_ui_progress( package, 2, COMPONENT_PROGRESS_VALUE, 0, 0 );
        if (!comp->ComponentId)
            continue;

        squash_guid( comp->ComponentId, squashed_cc );
        free( comp->FullKeypath );
        comp->FullKeypath = build_full_keypath( package, comp );

        refcount_component( package, comp );

        if (package->need_rollback) action = comp->Installed;
        else action = comp->ActionRequest;

        TRACE("Component %s (%s) Keypath=%s RefCount=%u Clients=%u Action=%u\n",
                            debugstr_w(comp->Component), debugstr_w(squashed_cc),
                            debugstr_w(comp->FullKeypath), comp->RefCount, comp->num_clients, action);

        if (action == INSTALLSTATE_LOCAL || action == INSTALLSTATE_SOURCE)
        {
            if (package->Context == MSIINSTALLCONTEXT_MACHINE)
                rc = MSIREG_OpenUserDataComponentKey(comp->ComponentId, L"S-1-5-18", &hkey, TRUE);
            else
                rc = MSIREG_OpenUserDataComponentKey(comp->ComponentId, NULL, &hkey, TRUE);

            if (rc != ERROR_SUCCESS)
                continue;

            if (comp->Attributes & msidbComponentAttributesPermanent)
            {
                msi_reg_set_val_str(hkey, L"00000000000000000000000000000000", comp->FullKeypath);
            }
            if (action == INSTALLSTATE_LOCAL)
                msi_reg_set_val_str( hkey, squashed_pc, comp->FullKeypath );
            else
            {
                MSIFILE *file;
                MSIRECORD *row;
                LPWSTR ptr, ptr2;
                WCHAR source[MAX_PATH];
                WCHAR base[MAX_PATH];
                LPWSTR sourcepath;

                if (!comp->KeyPath || !(file = msi_get_loaded_file(package, comp->KeyPath)))
                    continue;

                if (!(row = MSI_QueryGetRecord(package->db, L"SELECT * FROM `Media` WHERE `LastSequence` >= %d "
                                                             L"ORDER BY `DiskId`", file->Sequence)))
                    return ERROR_FUNCTION_FAILED;

                swprintf(source, ARRAY_SIZE(source), L"%02d\\", MSI_RecordGetInteger(row, 1));
                ptr2 = wcsrchr(source, '\\') + 1;
                msiobj_release(&row->hdr);

                lstrcpyW(base, package->PackagePath);
                ptr = wcsrchr(base, '\\');
                *(ptr + 1) = '\0';

                sourcepath = msi_resolve_file_source(package, file);
                ptr = sourcepath + lstrlenW(base);
                lstrcpyW(ptr2, ptr);
                free(sourcepath);

                msi_reg_set_val_str( hkey, squashed_pc, source );
            }
            RegCloseKey(hkey);
        }
        else if (action == INSTALLSTATE_ABSENT)
        {
            if (comp->num_clients <= 0)
            {
                if (package->Context == MSIINSTALLCONTEXT_MACHINE)
                    rc = MSIREG_DeleteUserDataComponentKey( comp->ComponentId, L"S-1-5-18" );
                else
                    rc = MSIREG_DeleteUserDataComponentKey( comp->ComponentId, NULL );

                if (rc != ERROR_SUCCESS) WARN( "failed to delete component key %u\n", rc );
            }
            else
            {
                LONG res;

                if (package->Context == MSIINSTALLCONTEXT_MACHINE)
                    rc = MSIREG_OpenUserDataComponentKey( comp->ComponentId, L"S-1-5-18", &hkey, FALSE );
                else
                    rc = MSIREG_OpenUserDataComponentKey( comp->ComponentId, NULL, &hkey, FALSE );

                if (rc != ERROR_SUCCESS)
                {
                    WARN( "failed to open component key %u\n", rc );
                    continue;
                }
                res = RegDeleteValueW( hkey, squashed_pc );
                RegCloseKey(hkey);
                if (res) WARN( "failed to delete component value %ld\n", res );
            }
        }

        /* UI stuff */
        uirow = MSI_CreateRecord(3);
        MSI_RecordSetStringW(uirow,1,package->ProductCode);
        MSI_RecordSetStringW(uirow,2,comp->ComponentId);
        MSI_RecordSetStringW(uirow,3,comp->FullKeypath);
        MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
        msiobj_release( &uirow->hdr );
    }
    return ERROR_SUCCESS;
}

struct typelib
{
    CLSID       clsid;
    LPWSTR      source;
    LPWSTR      path;
    ITypeLib    *ptLib;
};

static BOOL CALLBACK Typelib_EnumResNameProc( HMODULE hModule, LPCWSTR lpszType, 
                                       LPWSTR lpszName, LONG_PTR lParam)
{
    TLIBATTR *attr;
    struct typelib *tl_struct = (struct typelib *)lParam;
    int sz;
    HRESULT res;

    if (!IS_INTRESOURCE(lpszName))
    {
        ERR("Not Int Resource Name %s\n",debugstr_w(lpszName));
        return TRUE;
    }

    sz = lstrlenW(tl_struct->source)+4;

    if ((INT_PTR)lpszName == 1)
        tl_struct->path = wcsdup(tl_struct->source);
    else
    {
        tl_struct->path = malloc(sz * sizeof(WCHAR));
#ifdef __REACTOS__
        swprintf(tl_struct->path, sz, L"%s\\%d", tl_struct->source, (WORD)(INT_PTR)lpszName);
#else
        swprintf(tl_struct->path, sz, L"%s\\%d", tl_struct->source, lpszName);
#endif
    }

    TRACE("trying %s\n", debugstr_w(tl_struct->path));
    res = LoadTypeLib(tl_struct->path,&tl_struct->ptLib);
    if (FAILED(res))
    {
        free(tl_struct->path);
        tl_struct->path = NULL;

        return TRUE;
    }

    ITypeLib_GetLibAttr(tl_struct->ptLib, &attr);
    if (IsEqualGUID(&(tl_struct->clsid),&(attr->guid)))
    {
        ITypeLib_ReleaseTLibAttr(tl_struct->ptLib, attr);
        return FALSE;
    }

    free(tl_struct->path);
    tl_struct->path = NULL;

    ITypeLib_ReleaseTLibAttr(tl_struct->ptLib, attr);
    ITypeLib_Release(tl_struct->ptLib);

    return TRUE;
}

static HMODULE load_library( MSIPACKAGE *package, const WCHAR *filename, DWORD flags )
{
    HMODULE module;
    msi_disable_fs_redirection( package );
    module = LoadLibraryExW( filename, NULL, flags );
    msi_revert_fs_redirection( package );
    return module;
}

static HRESULT load_typelib( MSIPACKAGE *package, const WCHAR *filename, REGKIND kind, ITypeLib **lib )
{
    HRESULT hr;
    msi_disable_fs_redirection( package );
    hr = LoadTypeLibEx( filename, kind, lib );
    msi_revert_fs_redirection( package );
    return hr;
}

static UINT ITERATE_RegisterTypeLibraries(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE* package = param;
    LPCWSTR component;
    MSICOMPONENT *comp;
    MSIFILE *file;
    struct typelib tl_struct;
    ITypeLib *tlib;
    HMODULE module;
    HRESULT hr;

    component = MSI_RecordGetString(row,3);
    comp = msi_get_loaded_component(package,component);
    if (!comp)
        return ERROR_SUCCESS;

    comp->Action = msi_get_component_action( package, comp );
    if (comp->Action != INSTALLSTATE_LOCAL)
    {
        TRACE("component not scheduled for installation %s\n", debugstr_w(component));
        return ERROR_SUCCESS;
    }

    if (!comp->KeyPath || !(file = msi_get_loaded_file( package, comp->KeyPath )))
    {
        TRACE("component has no key path\n");
        return ERROR_SUCCESS;
    }
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, row);

    module = load_library( package, file->TargetPath, LOAD_LIBRARY_AS_DATAFILE );
    if (module)
    {
        LPCWSTR guid;
        guid = MSI_RecordGetString(row,1);
        CLSIDFromString( guid, &tl_struct.clsid);
        tl_struct.source = wcsdup( file->TargetPath );
        tl_struct.path = NULL;

        EnumResourceNamesW(module, L"TYPELIB", Typelib_EnumResNameProc,
                        (LONG_PTR)&tl_struct);

        if (tl_struct.path)
        {
            LPCWSTR helpid, help_path = NULL;
            HRESULT res;

            helpid = MSI_RecordGetString(row,6);

            if (helpid) help_path = msi_get_target_folder( package, helpid );
            res = RegisterTypeLib( tl_struct.ptLib, tl_struct.path, (OLECHAR *)help_path );

            if (FAILED(res))
                ERR("Failed to register type library %s\n", debugstr_w(tl_struct.path));
            else
                TRACE("Registered %s\n", debugstr_w(tl_struct.path));

            ITypeLib_Release(tl_struct.ptLib);
            free(tl_struct.path);
        }
        else ERR("Failed to load type library %s\n", debugstr_w(tl_struct.source));

        FreeLibrary(module);
        free(tl_struct.source);
    }
    else
    {
        hr = load_typelib( package, file->TargetPath, REGKIND_REGISTER, &tlib );
        if (FAILED(hr))
        {
            ERR( "failed to load type library: %#lx\n", hr );
            return ERROR_INSTALL_FAILURE;
        }

        ITypeLib_Release(tlib);
    }

    return ERROR_SUCCESS;
}

static UINT ACTION_RegisterTypeLibraries(MSIPACKAGE *package)
{
    MSIQUERY *view;
    UINT rc;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action(package, SCRIPT_INSTALL, L"RegisterTypeLibraries");

    rc = MSI_DatabaseOpenViewW(package->db, L"SELECT * FROM `TypeLib`", &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords(view, NULL, ITERATE_RegisterTypeLibraries, package);
    msiobj_release(&view->hdr);
    return rc;
}

static UINT ITERATE_UnregisterTypeLibraries( MSIRECORD *row, LPVOID param )
{
    MSIPACKAGE *package = param;
    LPCWSTR component, guid;
    MSICOMPONENT *comp;
    GUID libid;
    UINT version;
    LCID language;
    SYSKIND syskind;
    HRESULT hr;

    component = MSI_RecordGetString( row, 3 );
    comp = msi_get_loaded_component( package, component );
    if (!comp)
        return ERROR_SUCCESS;

    comp->Action = msi_get_component_action( package, comp );
    if (comp->Action != INSTALLSTATE_ABSENT)
    {
        TRACE("component not scheduled for removal %s\n", debugstr_w(component));
        return ERROR_SUCCESS;
    }
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, row);

    guid = MSI_RecordGetString( row, 1 );
    CLSIDFromString( guid, &libid );
    version = MSI_RecordGetInteger( row, 4 );
    language = MSI_RecordGetInteger( row, 2 );

#ifdef _WIN64
    syskind = SYS_WIN64;
#else
    syskind = SYS_WIN32;
#endif

    hr = UnRegisterTypeLib( &libid, (version >> 8) & 0xffff, version & 0xff, language, syskind );
    if (FAILED(hr))
    {
        WARN( "failed to unregister typelib: %#lx\n", hr );
    }

    return ERROR_SUCCESS;
}

static UINT ACTION_UnregisterTypeLibraries( MSIPACKAGE *package )
{
    MSIQUERY *view;
    UINT rc;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action(package, SCRIPT_INSTALL, L"UnregisterTypeLibraries");

    rc = MSI_DatabaseOpenViewW( package->db, L"SELECT * FROM `TypeLib`", &view );
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords( view, NULL, ITERATE_UnregisterTypeLibraries, package );
    msiobj_release( &view->hdr );
    return rc;
}

static WCHAR *get_link_file( MSIPACKAGE *package, MSIRECORD *row )
{
    LPCWSTR directory, extension, link_folder;
    WCHAR *link_file = NULL, *filename, *new_filename;

    directory = MSI_RecordGetString( row, 2 );
    link_folder = msi_get_target_folder( package, directory );
    if (!link_folder)
    {
        ERR("unable to resolve folder %s\n", debugstr_w(directory));
        return NULL;
    }
    /* may be needed because of a bug somewhere else */
    msi_create_full_path( package, link_folder );

    filename = msi_dup_record_field( row, 3 );
    if (!filename) return NULL;
    msi_reduce_to_long_filename( filename );

    extension = wcsrchr( filename, '.' );
    if (!extension || wcsicmp( extension, L".lnk" ))
    {
        int len = lstrlenW( filename );
        new_filename = realloc( filename, len * sizeof(WCHAR) + sizeof(L".lnk") );
        if (!new_filename) goto done;
        filename = new_filename;
        memcpy( filename + len, L".lnk", sizeof(L".lnk") );
    }
    link_file = msi_build_directory_name( 2, link_folder, filename );

done:
    free( filename );
    return link_file;
}

WCHAR *msi_build_icon_path( MSIPACKAGE *package, const WCHAR *icon_name )
{
    WCHAR *folder, *dest, *path;

    if (package->Context == MSIINSTALLCONTEXT_MACHINE)
        folder = msi_dup_property( package->db, L"WindowsFolder" );
    else
    {
        WCHAR *appdata = msi_dup_property( package->db, L"AppDataFolder" );
        folder = msi_build_directory_name( 2, appdata, L"Microsoft\\" );
        free( appdata );
    }
    dest = msi_build_directory_name( 3, folder, L"Installer\\", package->ProductCode );
    msi_create_full_path( package, dest );
    path = msi_build_directory_name( 2, dest, icon_name );
    free( folder );
    free( dest );
    return path;
}

static UINT ITERATE_CreateShortcuts(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE *package = param;
    LPWSTR link_file, deformated, path;
    LPCWSTR component, target;
    MSICOMPONENT *comp;
    IShellLinkW *sl = NULL;
    IPersistFile *pf = NULL;
    HRESULT res;

    component = MSI_RecordGetString(row, 4);
    comp = msi_get_loaded_component(package, component);
    if (!comp)
        return ERROR_SUCCESS;

    comp->Action = msi_get_component_action( package, comp );
    if (comp->Action != INSTALLSTATE_LOCAL)
    {
        TRACE("component not scheduled for installation %s\n", debugstr_w(component));
        return ERROR_SUCCESS;
    }
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, row);

    res = CoCreateInstance( &CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                    &IID_IShellLinkW, (LPVOID *) &sl );

    if (FAILED( res ))
    {
        ERR("CLSID_ShellLink not available\n");
        goto err;
    }

    res = IShellLinkW_QueryInterface( sl, &IID_IPersistFile,(LPVOID*) &pf );
    if (FAILED( res ))
    {
        ERR("QueryInterface(IID_IPersistFile) failed\n");
        goto err;
    }

    target = MSI_RecordGetString(row, 5);
    if (wcschr(target, '['))
    {
        deformat_string( package, target, &path );
        TRACE("target path is %s\n", debugstr_w(path));
        IShellLinkW_SetPath( sl, path );
        free( path );
    }
    else
    {
        FIXME("poorly handled shortcut format, advertised shortcut\n");
        path = resolve_keypath( package, comp );
        IShellLinkW_SetPath( sl, path );
        free( path );
    }

    if (!MSI_RecordIsNull(row,6))
    {
        LPCWSTR arguments = MSI_RecordGetString(row, 6);
        deformat_string(package, arguments, &deformated);
        IShellLinkW_SetArguments(sl,deformated);
        free(deformated);
    }

    if (!MSI_RecordIsNull(row,7))
    {
        LPCWSTR description = MSI_RecordGetString(row, 7);
        IShellLinkW_SetDescription(sl, description);
    }

    if (!MSI_RecordIsNull(row,8))
        IShellLinkW_SetHotkey(sl,MSI_RecordGetInteger(row,8));

    if (!MSI_RecordIsNull(row,9))
    {
        INT index; 
        LPCWSTR icon = MSI_RecordGetString(row, 9);

        path = msi_build_icon_path(package, icon);
        index = MSI_RecordGetInteger(row,10);

        /* no value means 0 */
        if (index == MSI_NULL_INTEGER)
            index = 0;

        IShellLinkW_SetIconLocation(sl, path, index);
        free(path);
    }

    if (!MSI_RecordIsNull(row,11))
        IShellLinkW_SetShowCmd(sl,MSI_RecordGetInteger(row,11));

    if (!MSI_RecordIsNull(row,12))
    {
        LPCWSTR full_path, wkdir = MSI_RecordGetString( row, 12 );
        full_path = msi_get_target_folder( package, wkdir );
        if (full_path) IShellLinkW_SetWorkingDirectory( sl, full_path );
    }

    link_file = get_link_file(package, row);
    TRACE("Writing shortcut to %s\n", debugstr_w(link_file));

    msi_disable_fs_redirection( package );
    IPersistFile_Save(pf, link_file, FALSE);
    msi_revert_fs_redirection( package );

    free(link_file);

err:
    if (pf)
        IPersistFile_Release( pf );
    if (sl)
        IShellLinkW_Release( sl );

    return ERROR_SUCCESS;
}

static UINT ACTION_CreateShortcuts(MSIPACKAGE *package)
{
    MSIQUERY *view;
    HRESULT res;
    UINT rc;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action(package, SCRIPT_INSTALL, L"CreateShortcuts");

    rc = MSI_DatabaseOpenViewW(package->db, L"SELECT * FROM `Shortcut`", &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    res = CoInitialize( NULL );

    rc = MSI_IterateRecords(view, NULL, ITERATE_CreateShortcuts, package);
    msiobj_release(&view->hdr);

    if (SUCCEEDED(res)) CoUninitialize();
    return rc;
}

static UINT ITERATE_RemoveShortcuts( MSIRECORD *row, LPVOID param )
{
    MSIPACKAGE *package = param;
    LPWSTR link_file;
    LPCWSTR component;
    MSICOMPONENT *comp;

    component = MSI_RecordGetString( row, 4 );
    comp = msi_get_loaded_component( package, component );
    if (!comp)
        return ERROR_SUCCESS;

    comp->Action = msi_get_component_action( package, comp );
    if (comp->Action != INSTALLSTATE_ABSENT)
    {
        TRACE("component not scheduled for removal %s\n", debugstr_w(component));
        return ERROR_SUCCESS;
    }
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, row);

    link_file = get_link_file( package, row );
    TRACE("Removing shortcut file %s\n", debugstr_w( link_file ));
    if (!msi_delete_file( package, link_file )) WARN( "failed to remove shortcut file %lu\n", GetLastError() );
    free( link_file );

    return ERROR_SUCCESS;
}

static UINT ACTION_RemoveShortcuts( MSIPACKAGE *package )
{
    MSIQUERY *view;
    UINT rc;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action(package, SCRIPT_INSTALL, L"RemoveShortcuts");

    rc = MSI_DatabaseOpenViewW( package->db, L"SELECT * FROM `Shortcut`", &view );
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords( view, NULL, ITERATE_RemoveShortcuts, package );
    msiobj_release( &view->hdr );
    return rc;
}

static UINT ITERATE_PublishIcon(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE *package = param;
    HANDLE handle;
    WCHAR *icon_path;
    const WCHAR *filename;
    char buffer[1024];
    DWORD sz;
    UINT rc;

    filename = MSI_RecordGetString( row, 1 );
    if (!filename)
    {
        ERR("Unable to get filename\n");
        return ERROR_SUCCESS;
    }

    icon_path = msi_build_icon_path( package, filename );

    TRACE("Creating icon file at %s\n", debugstr_w(icon_path));

    handle = msi_create_file( package, icon_path, GENERIC_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL );
    if (handle == INVALID_HANDLE_VALUE)
    {
        ERR("Unable to create file %s\n", debugstr_w(icon_path));
        free( icon_path );
        return ERROR_SUCCESS;
    }

    do
    {
        DWORD count;
        sz = 1024;
        rc = MSI_RecordReadStream( row, 2, buffer, &sz );
        if (rc != ERROR_SUCCESS)
        {
            ERR("Failed to get stream\n");
            msi_delete_file( package, icon_path );
            break;
        }
        WriteFile( handle, buffer, sz, &count, NULL );
    } while (sz == 1024);

    free( icon_path );
    CloseHandle( handle );

    return ERROR_SUCCESS;
}

static UINT publish_icons(MSIPACKAGE *package)
{
    MSIQUERY *view;
    UINT r;

    r = MSI_DatabaseOpenViewW(package->db, L"SELECT * FROM `Icon`", &view);
    if (r == ERROR_SUCCESS)
    {
        r = MSI_IterateRecords(view, NULL, ITERATE_PublishIcon, package);
        msiobj_release(&view->hdr);
        if (r != ERROR_SUCCESS)
            return r;
    }
    return ERROR_SUCCESS;
}

static UINT publish_sourcelist(MSIPACKAGE *package, HKEY hkey)
{
    UINT r;
    HKEY source;
    LPWSTR buffer;
    MSIMEDIADISK *disk;
    MSISOURCELISTINFO *info;

    r = RegCreateKeyW(hkey, L"SourceList", &source);
    if (r != ERROR_SUCCESS)
        return r;

    RegCloseKey(source);

    buffer = wcsrchr(package->PackagePath, '\\') + 1;
    r = MsiSourceListSetInfoW(package->ProductCode, NULL,
                              package->Context, MSICODE_PRODUCT,
                              INSTALLPROPERTY_PACKAGENAMEW, buffer);
    if (r != ERROR_SUCCESS)
        return r;

    r = MsiSourceListSetInfoW(package->ProductCode, NULL,
                              package->Context, MSICODE_PRODUCT,
                              INSTALLPROPERTY_MEDIAPACKAGEPATHW, L"");
    if (r != ERROR_SUCCESS)
        return r;

    r = MsiSourceListSetInfoW(package->ProductCode, NULL,
                              package->Context, MSICODE_PRODUCT,
                              INSTALLPROPERTY_DISKPROMPTW, L"");
    if (r != ERROR_SUCCESS)
        return r;

    LIST_FOR_EACH_ENTRY(info, &package->sourcelist_info, MSISOURCELISTINFO, entry)
    {
        if (!wcscmp( info->property, INSTALLPROPERTY_LASTUSEDSOURCEW ))
            msi_set_last_used_source(package->ProductCode, NULL, info->context,
                                     info->options, info->value);
        else
            MsiSourceListSetInfoW(package->ProductCode, NULL,
                                  info->context, info->options,
                                  info->property, info->value);
    }

    LIST_FOR_EACH_ENTRY(disk, &package->sourcelist_media, MSIMEDIADISK, entry)
    {
        MsiSourceListAddMediaDiskW(package->ProductCode, NULL,
                                   disk->context, disk->options,
                                   disk->disk_id, disk->volume_label, disk->disk_prompt);
    }

    return ERROR_SUCCESS;
}

static UINT publish_product_properties(MSIPACKAGE *package, HKEY hkey)
{
    WCHAR *buffer, *ptr, *guids, packcode[SQUASHED_GUID_SIZE];
    DWORD langid;

    buffer = msi_dup_property(package->db, INSTALLPROPERTY_PRODUCTNAMEW);
    msi_reg_set_val_str(hkey, INSTALLPROPERTY_PRODUCTNAMEW, buffer);
    free(buffer);

    langid = msi_get_property_int(package->db, L"ProductLanguage", 0);
    msi_reg_set_val_dword(hkey, INSTALLPROPERTY_LANGUAGEW, langid);

    /* FIXME */
    msi_reg_set_val_dword(hkey, INSTALLPROPERTY_AUTHORIZED_LUA_APPW, 0);

    buffer = msi_dup_property(package->db, L"ARPPRODUCTICON");
    if (buffer)
    {
        LPWSTR path = msi_build_icon_path(package, buffer);
        msi_reg_set_val_str(hkey, INSTALLPROPERTY_PRODUCTICONW, path);
        free(path);
        free(buffer);
    }

    buffer = msi_dup_property(package->db, L"ProductVersion");
    if (buffer)
    {
        DWORD verdword = msi_version_str_to_dword(buffer);
        msi_reg_set_val_dword(hkey, INSTALLPROPERTY_VERSIONW, verdword);
        free(buffer);
    }

    msi_reg_set_val_dword(hkey, L"Assignment", 0);
    msi_reg_set_val_dword(hkey, L"AdvertiseFlags", 0x184);
    msi_reg_set_val_dword(hkey, INSTALLPROPERTY_INSTANCETYPEW, 0);
    msi_reg_set_val_multi_str(hkey, L"Clients", L":\0");

    if (!(guids = msi_get_package_code(package->db))) return ERROR_OUTOFMEMORY;
    if ((ptr = wcschr(guids, ';'))) *ptr = 0;
    squash_guid(guids, packcode);
    free(guids);
    msi_reg_set_val_str(hkey, INSTALLPROPERTY_PACKAGECODEW, packcode);

    return ERROR_SUCCESS;
}

static UINT publish_upgrade_code(MSIPACKAGE *package)
{
    UINT r;
    HKEY hkey;
    WCHAR *upgrade, squashed_pc[SQUASHED_GUID_SIZE];

    upgrade = msi_dup_property(package->db, L"UpgradeCode");
    if (!upgrade)
        return ERROR_SUCCESS;

    if (package->Context == MSIINSTALLCONTEXT_MACHINE)
        r = MSIREG_OpenClassesUpgradeCodesKey(upgrade, &hkey, TRUE);
    else
        r = MSIREG_OpenUserUpgradeCodesKey(upgrade, &hkey, TRUE);

    if (r != ERROR_SUCCESS)
    {
        WARN("failed to open upgrade code key\n");
        free(upgrade);
        return ERROR_SUCCESS;
    }
    squash_guid(package->ProductCode, squashed_pc);
    msi_reg_set_val_str(hkey, squashed_pc, NULL);
    RegCloseKey(hkey);
    free(upgrade);
    return ERROR_SUCCESS;
}

static BOOL check_publish(MSIPACKAGE *package)
{
    MSIFEATURE *feature;

    LIST_FOR_EACH_ENTRY(feature, &package->features, MSIFEATURE, entry)
    {
        feature->Action = msi_get_feature_action( package, feature );
        if (feature->Action == INSTALLSTATE_LOCAL || feature->Action == INSTALLSTATE_SOURCE)
            return TRUE;
    }

    return FALSE;
}

static BOOL check_unpublish(MSIPACKAGE *package)
{
    MSIFEATURE *feature;

    LIST_FOR_EACH_ENTRY(feature, &package->features, MSIFEATURE, entry)
    {
        feature->Action = msi_get_feature_action( package, feature );
        if (feature->Action != INSTALLSTATE_ABSENT)
            return FALSE;
    }

    return TRUE;
}

static UINT publish_patches( MSIPACKAGE *package )
{
    WCHAR patch_squashed[GUID_SIZE];
    HKEY patches_key = NULL, product_patches_key = NULL, product_key;
    LONG res;
    MSIPATCHINFO *patch;
    UINT r;
    WCHAR *p, *all_patches = NULL;
    DWORD len = 0;

    r = MSIREG_OpenProductKey( package->ProductCode, NULL, package->Context, &product_key, TRUE );
    if (r != ERROR_SUCCESS)
        return ERROR_FUNCTION_FAILED;

    res = RegCreateKeyExW( product_key, L"Patches", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &patches_key, NULL );
    if (res != ERROR_SUCCESS)
    {
        r = ERROR_FUNCTION_FAILED;
        goto done;
    }

    r = MSIREG_OpenUserDataProductPatchesKey( package->ProductCode, package->Context, &product_patches_key, TRUE );
    if (r != ERROR_SUCCESS)
        goto done;

    LIST_FOR_EACH_ENTRY( patch, &package->patches, MSIPATCHINFO, entry )
    {
        squash_guid( patch->patchcode, patch_squashed );
        len += lstrlenW( patch_squashed ) + 1;
    }

    p = all_patches = malloc( (len + 1) * sizeof(WCHAR) );
    if (!all_patches)
        goto done;

    LIST_FOR_EACH_ENTRY( patch, &package->patches, MSIPATCHINFO, entry )
    {
        HKEY patch_key;

        squash_guid( patch->patchcode, p );
        p += lstrlenW( p ) + 1;

        res = RegSetValueExW( patches_key, patch_squashed, 0, REG_SZ,
                              (const BYTE *)patch->transforms,
                              (lstrlenW(patch->transforms) + 1) * sizeof(WCHAR) );
        if (res != ERROR_SUCCESS)
            goto done;

        r = MSIREG_OpenUserDataPatchKey( patch->patchcode, package->Context, &patch_key, TRUE );
        if (r != ERROR_SUCCESS)
            goto done;

        res = RegSetValueExW( patch_key, L"LocalPackage", 0, REG_SZ, (const BYTE *)patch->localfile,
                              (lstrlenW( patch->localfile ) + 1) * sizeof(WCHAR) );
        RegCloseKey( patch_key );
        if (res != ERROR_SUCCESS)
            goto done;

        if (patch->filename && !CopyFileW( patch->filename, patch->localfile, FALSE ))
        {
            res = GetLastError();
            ERR( "unable to copy patch package %lu\n", res );
            goto done;
        }
        res = RegCreateKeyExW( product_patches_key, patch_squashed, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &patch_key, NULL );
        if (res != ERROR_SUCCESS)
            goto done;

        res = RegSetValueExW( patch_key, L"State", 0, REG_DWORD, (const BYTE *)&patch->state,
                              sizeof(patch->state) );
        if (res != ERROR_SUCCESS)
        {
            RegCloseKey( patch_key );
            goto done;
        }

        res = RegSetValueExW( patch_key, L"Uninstallable", 0, REG_DWORD, (const BYTE *)&patch->uninstallable,
                              sizeof(patch->uninstallable) );
        RegCloseKey( patch_key );
        if (res != ERROR_SUCCESS)
            goto done;
    }

    all_patches[len] = 0;
    res = RegSetValueExW( patches_key, L"Patches", 0, REG_MULTI_SZ,
                          (const BYTE *)all_patches, (len + 1) * sizeof(WCHAR) );
    if (res != ERROR_SUCCESS)
        goto done;

    res = RegSetValueExW( product_patches_key, L"AllPatches", 0, REG_MULTI_SZ,
                          (const BYTE *)all_patches, (len + 1) * sizeof(WCHAR) );
    if (res != ERROR_SUCCESS)
        r = ERROR_FUNCTION_FAILED;

done:
    RegCloseKey( product_patches_key );
    RegCloseKey( patches_key );
    RegCloseKey( product_key );
    free( all_patches );
    return r;
}

static UINT ACTION_PublishProduct(MSIPACKAGE *package)
{
    UINT rc;
    HKEY hukey = NULL, hudkey = NULL;
    MSIRECORD *uirow;
    BOOL republish = FALSE;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action(package, SCRIPT_INSTALL, L"PublishProduct");

    if (!list_empty(&package->patches))
    {
        rc = publish_patches(package);
        if (rc != ERROR_SUCCESS)
            goto end;
    }

    rc = MSIREG_OpenProductKey(package->ProductCode, NULL, package->Context,
                               &hukey, FALSE);
    if (rc == ERROR_SUCCESS)
    {
        WCHAR *package_code;

        package_code = msi_reg_get_val_str(hukey, INSTALLPROPERTY_PACKAGECODEW);
        if (package_code)
        {
            WCHAR *guid;

            guid = msi_get_package_code(package->db);
            if (guid)
            {
                WCHAR packed[SQUASHED_GUID_SIZE];

                squash_guid(guid, packed);
                free(guid);
                if (!wcscmp(packed, package_code))
                {
                    TRACE("re-publishing product - new package\n");
                    republish = TRUE;
                }
            }
            free(package_code);
        }
    }

    /* FIXME: also need to publish if the product is in advertise mode */
    if (!republish && !check_publish(package))
    {
        if (hukey)
            RegCloseKey(hukey);
        return ERROR_SUCCESS;
    }

    if (!hukey)
    {
        rc = MSIREG_OpenProductKey(package->ProductCode, NULL, package->Context,
                                   &hukey, TRUE);
        if (rc != ERROR_SUCCESS)
            goto end;
    }

    rc = MSIREG_OpenUserDataProductKey(package->ProductCode, package->Context,
                                       NULL, &hudkey, TRUE);
    if (rc != ERROR_SUCCESS)
        goto end;

    rc = publish_upgrade_code(package);
    if (rc != ERROR_SUCCESS)
        goto end;

    rc = publish_product_properties(package, hukey);
    if (rc != ERROR_SUCCESS)
        goto end;

    rc = publish_sourcelist(package, hukey);
    if (rc != ERROR_SUCCESS)
        goto end;

    rc = publish_icons(package);

end:
    uirow = MSI_CreateRecord( 1 );
    MSI_RecordSetStringW( uirow, 1, package->ProductCode );
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
    msiobj_release( &uirow->hdr );

    RegCloseKey(hukey);
    RegCloseKey(hudkey);
    return rc;
}

static WCHAR *get_ini_file_name( MSIPACKAGE *package, MSIRECORD *row )
{
    WCHAR *filename, *ptr, *folder, *ret;
    const WCHAR *dirprop;

    filename = msi_dup_record_field( row, 2 );
    if (filename && (ptr = wcschr( filename, '|' )))
        ptr++;
    else
        ptr = filename;

    dirprop = MSI_RecordGetString( row, 3 );
    if (dirprop)
    {
        folder = wcsdup( msi_get_target_folder( package, dirprop ) );
        if (!folder) folder = msi_dup_property( package->db, dirprop );
    }
    else
        folder = msi_dup_property( package->db, L"WindowsFolder" );

    if (!folder)
    {
        ERR("Unable to resolve folder %s\n", debugstr_w(dirprop));
        free( filename );
        return NULL;
    }

    ret = msi_build_directory_name( 2, folder, ptr );

    free( filename );
    free( folder );
    return ret;
}

static UINT ITERATE_WriteIniValues(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE *package = param;
    LPCWSTR component, section, key, value, identifier;
    LPWSTR deformated_section, deformated_key, deformated_value, fullname;
    MSIRECORD * uirow;
    INT action;
    MSICOMPONENT *comp;

    component = MSI_RecordGetString(row, 8);
    comp = msi_get_loaded_component(package,component);
    if (!comp)
        return ERROR_SUCCESS;

    comp->Action = msi_get_component_action( package, comp );
    if (comp->Action != INSTALLSTATE_LOCAL)
    {
        TRACE("component not scheduled for installation %s\n", debugstr_w(component));
        return ERROR_SUCCESS;
    }

    identifier = MSI_RecordGetString(row,1); 
    section = MSI_RecordGetString(row,4);
    key = MSI_RecordGetString(row,5);
    value = MSI_RecordGetString(row,6);
    action = MSI_RecordGetInteger(row,7);

    deformat_string(package,section,&deformated_section);
    deformat_string(package,key,&deformated_key);
    deformat_string(package,value,&deformated_value);

    fullname = get_ini_file_name(package, row);

    if (action == 0)
    {
        TRACE("Adding value %s to section %s in %s\n",
                debugstr_w(deformated_key), debugstr_w(deformated_section),
                debugstr_w(fullname));
        WritePrivateProfileStringW(deformated_section, deformated_key,
                                   deformated_value, fullname);
    }
    else if (action == 1)
    {
        WCHAR returned[10];
        GetPrivateProfileStringW(deformated_section, deformated_key, NULL,
                                 returned, 10, fullname);
        if (returned[0] == 0)
        {
            TRACE("Adding value %s to section %s in %s\n",
                    debugstr_w(deformated_key), debugstr_w(deformated_section),
                    debugstr_w(fullname));

            WritePrivateProfileStringW(deformated_section, deformated_key,
                                       deformated_value, fullname);
        }
    }
    else if (action == 3)
        FIXME("Append to existing section not yet implemented\n");

    uirow = MSI_CreateRecord(4);
    MSI_RecordSetStringW(uirow,1,identifier);
    MSI_RecordSetStringW(uirow,2,deformated_section);
    MSI_RecordSetStringW(uirow,3,deformated_key);
    MSI_RecordSetStringW(uirow,4,deformated_value);
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
    msiobj_release( &uirow->hdr );

    free(fullname);
    free(deformated_key);
    free(deformated_value);
    free(deformated_section);
    return ERROR_SUCCESS;
}

static UINT ACTION_WriteIniValues(MSIPACKAGE *package)
{
    MSIQUERY *view;
    UINT rc;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action(package, SCRIPT_INSTALL, L"WriteIniValues");

    rc = MSI_DatabaseOpenViewW(package->db, L"SELECT * FROM `IniFile`", &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords(view, NULL, ITERATE_WriteIniValues, package);
    msiobj_release(&view->hdr);
    return rc;
}

static UINT ITERATE_RemoveIniValuesOnUninstall( MSIRECORD *row, LPVOID param )
{
    MSIPACKAGE *package = param;
    LPCWSTR component, section, key, value, identifier;
    LPWSTR deformated_section, deformated_key, deformated_value, filename;
    MSICOMPONENT *comp;
    MSIRECORD *uirow;
    INT action;

    component = MSI_RecordGetString( row, 8 );
    comp = msi_get_loaded_component( package, component );
    if (!comp)
        return ERROR_SUCCESS;

    comp->Action = msi_get_component_action( package, comp );
    if (comp->Action != INSTALLSTATE_ABSENT)
    {
        TRACE("component not scheduled for removal %s\n", debugstr_w(component));
        return ERROR_SUCCESS;
    }

    identifier = MSI_RecordGetString( row, 1 );
    section = MSI_RecordGetString( row, 4 );
    key = MSI_RecordGetString( row, 5 );
    value = MSI_RecordGetString( row, 6 );
    action = MSI_RecordGetInteger( row, 7 );

    deformat_string( package, section, &deformated_section );
    deformat_string( package, key, &deformated_key );
    deformat_string( package, value, &deformated_value );

    if (action == msidbIniFileActionAddLine || action == msidbIniFileActionCreateLine)
    {
        filename = get_ini_file_name( package, row );

        TRACE("Removing key %s from section %s in %s\n",
               debugstr_w(deformated_key), debugstr_w(deformated_section), debugstr_w(filename));

        if (!WritePrivateProfileStringW( deformated_section, deformated_key, NULL, filename ))
        {
            WARN( "unable to remove key %lu\n", GetLastError() );
        }
        free( filename );
    }
    else
        FIXME("Unsupported action %d\n", action);


    uirow = MSI_CreateRecord( 4 );
    MSI_RecordSetStringW( uirow, 1, identifier );
    MSI_RecordSetStringW( uirow, 2, deformated_section );
    MSI_RecordSetStringW( uirow, 3, deformated_key );
    MSI_RecordSetStringW( uirow, 4, deformated_value );
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
    msiobj_release( &uirow->hdr );

    free( deformated_key );
    free( deformated_value );
    free( deformated_section );
    return ERROR_SUCCESS;
}

static UINT ITERATE_RemoveIniValuesOnInstall( MSIRECORD *row, LPVOID param )
{
    MSIPACKAGE *package = param;
    LPCWSTR component, section, key, value, identifier;
    LPWSTR deformated_section, deformated_key, deformated_value, filename;
    MSICOMPONENT *comp;
    MSIRECORD *uirow;
    INT action;

    component = MSI_RecordGetString( row, 8 );
    comp = msi_get_loaded_component( package, component );
    if (!comp)
        return ERROR_SUCCESS;

    comp->Action = msi_get_component_action( package, comp );
    if (comp->Action != INSTALLSTATE_LOCAL)
    {
        TRACE("component not scheduled for installation %s\n", debugstr_w(component));
        return ERROR_SUCCESS;
    }

    identifier = MSI_RecordGetString( row, 1 );
    section = MSI_RecordGetString( row, 4 );
    key = MSI_RecordGetString( row, 5 );
    value = MSI_RecordGetString( row, 6 );
    action = MSI_RecordGetInteger( row, 7 );

    deformat_string( package, section, &deformated_section );
    deformat_string( package, key, &deformated_key );
    deformat_string( package, value, &deformated_value );

    if (action == msidbIniFileActionRemoveLine)
    {
        filename = get_ini_file_name( package, row );

        TRACE("Removing key %s from section %s in %s\n",
               debugstr_w(deformated_key), debugstr_w(deformated_section), debugstr_w(filename));

        if (!WritePrivateProfileStringW( deformated_section, deformated_key, NULL, filename ))
        {
            WARN( "unable to remove key %lu\n", GetLastError() );
        }
        free( filename );
    }
    else
        FIXME("Unsupported action %d\n", action);

    uirow = MSI_CreateRecord( 4 );
    MSI_RecordSetStringW( uirow, 1, identifier );
    MSI_RecordSetStringW( uirow, 2, deformated_section );
    MSI_RecordSetStringW( uirow, 3, deformated_key );
    MSI_RecordSetStringW( uirow, 4, deformated_value );
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
    msiobj_release( &uirow->hdr );

    free( deformated_key );
    free( deformated_value );
    free( deformated_section );
    return ERROR_SUCCESS;
}

static UINT ACTION_RemoveIniValues( MSIPACKAGE *package )
{
    MSIQUERY *view;
    UINT rc;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action(package, SCRIPT_INSTALL, L"RemoveIniValues");

    rc = MSI_DatabaseOpenViewW( package->db, L"SELECT * FROM `IniFile`", &view );
    if (rc == ERROR_SUCCESS)
    {
        rc = MSI_IterateRecords( view, NULL, ITERATE_RemoveIniValuesOnUninstall, package );
        msiobj_release( &view->hdr );
        if (rc != ERROR_SUCCESS)
            return rc;
    }
    rc = MSI_DatabaseOpenViewW( package->db, L"SELECT * FROM `RemoveIniFile`", &view );
    if (rc == ERROR_SUCCESS)
    {
        rc = MSI_IterateRecords( view, NULL, ITERATE_RemoveIniValuesOnInstall, package );
        msiobj_release( &view->hdr );
        if (rc != ERROR_SUCCESS)
            return rc;
    }
    return ERROR_SUCCESS;
}

static void register_dll( const WCHAR *dll, BOOL unregister )
{
    static const WCHAR regW[] = L"regsvr32.exe /s \"%s\"";
    static const WCHAR unregW[] = L"regsvr32.exe /s /u \"%s\"";
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    WCHAR *cmd;

    if (!(cmd = malloc( wcslen(dll) * sizeof(WCHAR) + sizeof(unregW) ))) return;

    if (unregister) swprintf( cmd, lstrlenW(dll) + ARRAY_SIZE(unregW), unregW, dll );
    else swprintf( cmd, lstrlenW(dll) + ARRAY_SIZE(unregW), regW, dll );

    memset( &si, 0, sizeof(STARTUPINFOW) );
    if (CreateProcessW( NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi ))
    {
        CloseHandle( pi.hThread );
        msi_dialog_check_messages( pi.hProcess );
        CloseHandle( pi.hProcess );
    }
    free( cmd );
}

static UINT ITERATE_SelfRegModules(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE *package = param;
    LPCWSTR filename;
    MSIFILE *file;
    MSIRECORD *uirow;

    filename = MSI_RecordGetString( row, 1 );
    file = msi_get_loaded_file( package, filename );
    if (!file)
    {
        WARN("unable to find file %s\n", debugstr_w(filename));
        return ERROR_SUCCESS;
    }
    file->Component->Action = msi_get_component_action( package, file->Component );
    if (file->Component->Action != INSTALLSTATE_LOCAL)
    {
        TRACE("component not scheduled for installation %s\n", debugstr_w(file->Component->Component));
        return ERROR_SUCCESS;
    }

    TRACE("Registering %s\n", debugstr_w( file->TargetPath ));
    register_dll( file->TargetPath, FALSE );

    uirow = MSI_CreateRecord( 2 );
    MSI_RecordSetStringW( uirow, 1, file->File );
    MSI_RecordSetStringW( uirow, 2, file->Component->Directory );
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
    msiobj_release( &uirow->hdr );

    return ERROR_SUCCESS;
}

static UINT ACTION_SelfRegModules(MSIPACKAGE *package)
{
    MSIQUERY *view;
    UINT rc;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action(package, SCRIPT_INSTALL, L"SelfRegModules");

    rc = MSI_DatabaseOpenViewW(package->db, L"SELECT * FROM `SelfReg`", &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords(view, NULL, ITERATE_SelfRegModules, package);
    msiobj_release(&view->hdr);
    return rc;
}

static UINT ITERATE_SelfUnregModules( MSIRECORD *row, LPVOID param )
{
    MSIPACKAGE *package = param;
    LPCWSTR filename;
    MSIFILE *file;
    MSIRECORD *uirow;

    filename = MSI_RecordGetString( row, 1 );
    file = msi_get_loaded_file( package, filename );
    if (!file)
    {
        WARN("unable to find file %s\n", debugstr_w(filename));
        return ERROR_SUCCESS;
    }
    file->Component->Action = msi_get_component_action( package, file->Component );
    if (file->Component->Action != INSTALLSTATE_ABSENT)
    {
        TRACE("component not scheduled for removal %s\n", debugstr_w(file->Component->Component));
        return ERROR_SUCCESS;
    }

    TRACE("Unregistering %s\n", debugstr_w( file->TargetPath ));
    register_dll( file->TargetPath, TRUE );

    uirow = MSI_CreateRecord( 2 );
    MSI_RecordSetStringW( uirow, 1, file->File );
    MSI_RecordSetStringW( uirow, 2, file->Component->Directory );
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
    msiobj_release( &uirow->hdr );

    return ERROR_SUCCESS;
}

static UINT ACTION_SelfUnregModules( MSIPACKAGE *package )
{
    MSIQUERY *view;
    UINT rc;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action(package, SCRIPT_INSTALL, L"SelfUnregModules");

    rc = MSI_DatabaseOpenViewW( package->db, L"SELECT * FROM `SelfReg`", &view );
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords( view, NULL, ITERATE_SelfUnregModules, package );
    msiobj_release( &view->hdr );
    return rc;
}

static UINT ACTION_PublishFeatures(MSIPACKAGE *package)
{
    MSIFEATURE *feature;
    UINT rc;
    HKEY hkey = NULL, userdata = NULL;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action(package, SCRIPT_INSTALL, L"PublishFeatures");

    if (!check_publish(package))
        return ERROR_SUCCESS;

    rc = MSIREG_OpenFeaturesKey(package->ProductCode, NULL, package->Context,
                                &hkey, TRUE);
    if (rc != ERROR_SUCCESS)
        goto end;

    rc = MSIREG_OpenUserDataFeaturesKey(package->ProductCode, NULL, package->Context,
                                        &userdata, TRUE);
    if (rc != ERROR_SUCCESS)
        goto end;

    /* here the guids are base 85 encoded */
    LIST_FOR_EACH_ENTRY( feature, &package->features, MSIFEATURE, entry )
    {
        ComponentList *cl;
        LPWSTR data = NULL;
        GUID clsid;
        INT size;
        BOOL absent = FALSE;
        MSIRECORD *uirow;

        if (feature->Level <= 0) continue;
        if (feature->Action == INSTALLSTATE_UNKNOWN &&
                feature->Installed != INSTALLSTATE_ABSENT) continue;

        if (feature->Action != INSTALLSTATE_LOCAL &&
            feature->Action != INSTALLSTATE_SOURCE &&
            feature->Action != INSTALLSTATE_ADVERTISED) absent = TRUE;

        size = 1;
        LIST_FOR_EACH_ENTRY( cl, &feature->Components, ComponentList, entry )
        {
            size += 21;
        }
        if (feature->Feature_Parent)
            size += lstrlenW( feature->Feature_Parent )+2;

        data = malloc(size * sizeof(WCHAR));

        data[0] = 0;
        LIST_FOR_EACH_ENTRY( cl, &feature->Components, ComponentList, entry )
        {
            MSICOMPONENT* component = cl->component;
            WCHAR buf[21];

            buf[0] = 0;
            if (component->ComponentId)
            {
                TRACE("From %s\n",debugstr_w(component->ComponentId));
                CLSIDFromString(component->ComponentId, &clsid);
                encode_base85_guid(&clsid,buf);
                TRACE("to %s\n",debugstr_w(buf));
                lstrcatW(data,buf);
            }
        }

        if (feature->Feature_Parent)
        {
            lstrcatW(data, L"\2");
            lstrcatW(data, feature->Feature_Parent);
        }

        msi_reg_set_val_str( userdata, feature->Feature, data );
        free(data);

        size = 0;
        if (feature->Feature_Parent)
            size = lstrlenW(feature->Feature_Parent)*sizeof(WCHAR);
        if (!absent)
        {
            size += sizeof(WCHAR);
            RegSetValueExW(hkey, feature->Feature, 0 ,REG_SZ,
                           (const BYTE*)(feature->Feature_Parent ? feature->Feature_Parent : L""), size);
        }
        else
        {
            size += 2*sizeof(WCHAR);
            data = malloc(size);
            data[0] = 0x6;
            data[1] = 0;
            if (feature->Feature_Parent)
                lstrcpyW( &data[1], feature->Feature_Parent );
            RegSetValueExW(hkey,feature->Feature,0,REG_SZ,
                       (LPBYTE)data,size);
            free(data);
        }

        /* the UI chunk */
        uirow = MSI_CreateRecord( 1 );
        MSI_RecordSetStringW( uirow, 1, feature->Feature );
        MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
        msiobj_release( &uirow->hdr );
        /* FIXME: call msi_ui_progress? */
    }

end:
    RegCloseKey(hkey);
    RegCloseKey(userdata);
    return rc;
}

static UINT unpublish_feature(MSIPACKAGE *package, MSIFEATURE *feature)
{
    UINT r;
    HKEY hkey;
    MSIRECORD *uirow;

    TRACE("unpublishing feature %s\n", debugstr_w(feature->Feature));

    r = MSIREG_OpenFeaturesKey(package->ProductCode, NULL, package->Context,
                               &hkey, FALSE);
    if (r == ERROR_SUCCESS)
    {
        RegDeleteValueW(hkey, feature->Feature);
        RegCloseKey(hkey);
    }

    r = MSIREG_OpenUserDataFeaturesKey(package->ProductCode, NULL, package->Context,
                                       &hkey, FALSE);
    if (r == ERROR_SUCCESS)
    {
        RegDeleteValueW(hkey, feature->Feature);
        RegCloseKey(hkey);
    }

    uirow = MSI_CreateRecord( 1 );
    MSI_RecordSetStringW( uirow, 1, feature->Feature );
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
    msiobj_release( &uirow->hdr );

    return ERROR_SUCCESS;
}

static UINT ACTION_UnpublishFeatures(MSIPACKAGE *package)
{
    MSIFEATURE *feature;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action(package, SCRIPT_INSTALL, L"UnpublishFeatures");

    if (!check_unpublish(package))
        return ERROR_SUCCESS;

    LIST_FOR_EACH_ENTRY(feature, &package->features, MSIFEATURE, entry)
    {
        unpublish_feature(package, feature);
    }

    return ERROR_SUCCESS;
}

static UINT publish_install_properties(MSIPACKAGE *package, HKEY hkey)
{
    static const WCHAR *propval[] =
    {
        L"ARPAUTHORIZEDCDFPREFIX", L"AuthorizedCDFPrefix",
        L"ARPCONTACT",             L"Contact",
        L"ARPCOMMENTS",            L"Comments",
        L"ProductName",            L"DisplayName",
        L"ARPHELPLINK",            L"HelpLink",
        L"ARPHELPTELEPHONE",       L"HelpTelephone",
        L"ARPINSTALLLOCATION",     L"InstallLocation",
        L"SourceDir",              L"InstallSource",
        L"Manufacturer",           L"Publisher",
        L"ARPREADME",              L"ReadMe",
        L"ARPSIZE",                L"Size",
        L"ARPURLINFOABOUT",        L"URLInfoAbout",
        L"ARPURLUPDATEINFO",       L"URLUpdateInfo",
        NULL
    };
    const WCHAR **p = propval;
    SYSTEMTIME systime;
    DWORD size, langid;
    WCHAR date[9], *val, *buffer;
    const WCHAR *prop, *key;

    while (*p)
    {
        prop = *p++;
        key = *p++;
        val = msi_dup_property(package->db, prop);
        msi_reg_set_val_str(hkey, key, val);
        free(val);
    }

    msi_reg_set_val_dword(hkey, L"WindowsInstaller", 1);
    if (msi_get_property_int( package->db, L"ARPSYSTEMCOMPONENT", 0 ))
    {
        msi_reg_set_val_dword( hkey, L"SystemComponent", 1 );
    }

    if (msi_get_property_int( package->db, L"ARPNOREMOVE", 0 ))
        msi_reg_set_val_dword( hkey, L"NoRemove", 1 );
    else
    {
        static const WCHAR fmt_install[] = L"MsiExec.exe /I[ProductCode]";
        static const WCHAR fmt_uninstall[] = L"MsiExec.exe /X[ProductCode]";
        const WCHAR *fmt = fmt_install;

        if (msi_get_property_int( package->db, L"ARPNOREPAIR", 0 ))
            msi_reg_set_val_dword( hkey, L"NoRepair", 1 );

        if (msi_get_property_int( package->db, L"ARPNOMODIFY", 0 ))
        {
            msi_reg_set_val_dword( hkey, L"NoModify", 1 );
            fmt = fmt_uninstall;
        }

        size = deformat_string(package, fmt, &buffer) * sizeof(WCHAR);
        RegSetValueExW(hkey, L"ModifyPath", 0, REG_EXPAND_SZ, (LPBYTE)buffer, size);
        RegSetValueExW(hkey, L"UninstallString", 0, REG_EXPAND_SZ, (LPBYTE)buffer, size);
        free(buffer);
    }

    /* FIXME: Write real Estimated Size when we have it */
    msi_reg_set_val_dword(hkey, L"EstimatedSize", 0);

    GetLocalTime(&systime);
    swprintf(date, ARRAY_SIZE(date), L"%d%02d%02d", systime.wYear, systime.wMonth, systime.wDay);
    msi_reg_set_val_str(hkey, INSTALLPROPERTY_INSTALLDATEW, date);

    langid = msi_get_property_int(package->db, L"ProductLanguage", 0);
    msi_reg_set_val_dword(hkey, INSTALLPROPERTY_LANGUAGEW, langid);

    buffer = msi_dup_property(package->db, L"ProductVersion");
    msi_reg_set_val_str(hkey, L"DisplayVersion", buffer);
    if (buffer)
    {
        DWORD verdword = msi_version_str_to_dword(buffer);

        msi_reg_set_val_dword(hkey, INSTALLPROPERTY_VERSIONW, verdword);
        msi_reg_set_val_dword(hkey, INSTALLPROPERTY_VERSIONMAJORW, verdword >> 24);
        msi_reg_set_val_dword(hkey, INSTALLPROPERTY_VERSIONMINORW, (verdword >> 16) & 0xFF);
        free(buffer);
    }

    return ERROR_SUCCESS;
}

static UINT ACTION_RegisterProduct(MSIPACKAGE *package)
{
    WCHAR *upgrade_code, squashed_pc[SQUASHED_GUID_SIZE];
    MSIRECORD *uirow;
    HKEY hkey, props, upgrade_key;
    UINT rc;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action(package, SCRIPT_INSTALL, L"RegisterProduct");

    /* FIXME: also need to publish if the product is in advertise mode */
    if (!msi_get_property_int( package->db, L"ProductToBeRegistered", 0 ) && !check_publish(package))
        return ERROR_SUCCESS;

    rc = MSIREG_OpenUninstallKey(package->ProductCode, package->platform, &hkey, TRUE);
    if (rc != ERROR_SUCCESS)
        return rc;

    rc = MSIREG_OpenInstallProps(package->ProductCode, package->Context, NULL, &props, TRUE);
    if (rc != ERROR_SUCCESS)
        goto done;

    rc = publish_install_properties(package, hkey);
    if (rc != ERROR_SUCCESS)
        goto done;

    rc = publish_install_properties(package, props);
    if (rc != ERROR_SUCCESS)
        goto done;

    upgrade_code = msi_dup_property(package->db, L"UpgradeCode");
    if (upgrade_code)
    {
        rc = MSIREG_OpenUpgradeCodesKey( upgrade_code, &upgrade_key, TRUE );
        if (rc == ERROR_SUCCESS)
        {
            squash_guid( package->ProductCode, squashed_pc );
            msi_reg_set_val_str( upgrade_key, squashed_pc, NULL );
            RegCloseKey( upgrade_key );
        }
        free( upgrade_code );
    }
    msi_reg_set_val_str( props, INSTALLPROPERTY_LOCALPACKAGEW, package->localfile );
    package->delete_on_close = FALSE;

done:
    uirow = MSI_CreateRecord( 1 );
    MSI_RecordSetStringW( uirow, 1, package->ProductCode );
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
    msiobj_release( &uirow->hdr );

    RegCloseKey(hkey);
    return ERROR_SUCCESS;
}

static UINT ACTION_InstallExecute(MSIPACKAGE *package)
{
    return execute_script(package, SCRIPT_INSTALL);
}

static UINT ITERATE_UnpublishIcon( MSIRECORD *row, LPVOID param )
{
    MSIPACKAGE *package = param;
    const WCHAR *icon = MSI_RecordGetString( row, 1 );
    WCHAR *p, *icon_path;

    if (!icon) return ERROR_SUCCESS;
    if ((icon_path = msi_build_icon_path( package, icon )))
    {
        TRACE("removing icon file %s\n", debugstr_w(icon_path));
        msi_delete_file( package, icon_path );
        if ((p = wcsrchr( icon_path, '\\' )))
        {
            *p = 0;
            msi_remove_directory( package, icon_path );
        }
        free( icon_path );
    }
    return ERROR_SUCCESS;
}

static UINT unpublish_icons( MSIPACKAGE *package )
{
    MSIQUERY *view;
    UINT r;

    r = MSI_DatabaseOpenViewW( package->db, L"SELECT * FROM `Icon`", &view );
    if (r == ERROR_SUCCESS)
    {
        r = MSI_IterateRecords( view, NULL, ITERATE_UnpublishIcon, package );
        msiobj_release( &view->hdr );
        if (r != ERROR_SUCCESS)
            return r;
    }
    return ERROR_SUCCESS;
}

static void remove_product_upgrade_code( MSIPACKAGE *package )
{
    WCHAR *code, product[SQUASHED_GUID_SIZE];
    HKEY hkey;
    LONG res;
    DWORD count;

    squash_guid( package->ProductCode, product );
    if (!(code = msi_dup_property( package->db, L"UpgradeCode" )))
    {
        WARN( "upgrade code not found\n" );
        return;
    }
    if (!MSIREG_OpenUpgradeCodesKey( code, &hkey, FALSE ))
    {
        RegDeleteValueW( hkey, product );
        res = RegQueryInfoKeyW( hkey, NULL, NULL, NULL, NULL, NULL, &count, NULL, NULL, NULL, NULL, NULL );
        RegCloseKey( hkey );
        if (!res && !count) MSIREG_DeleteUpgradeCodesKey( code );
    }
    if (!MSIREG_OpenUserUpgradeCodesKey( code, &hkey, FALSE ))
    {
        RegDeleteValueW( hkey, product );
        res = RegQueryInfoKeyW( hkey, NULL, NULL, NULL, NULL, NULL, &count, NULL, NULL, NULL, NULL, NULL );
        RegCloseKey( hkey );
        if (!res && !count) MSIREG_DeleteUserUpgradeCodesKey( code );
    }
    if (!MSIREG_OpenClassesUpgradeCodesKey( code, &hkey, FALSE ))
    {
        RegDeleteValueW( hkey, product );
        res = RegQueryInfoKeyW( hkey, NULL, NULL, NULL, NULL, NULL, &count, NULL, NULL, NULL, NULL, NULL );
        RegCloseKey( hkey );
        if (!res && !count) MSIREG_DeleteClassesUpgradeCodesKey( code );
    }

    free( code );
}

static UINT ACTION_UnpublishProduct(MSIPACKAGE *package)
{
    MSIPATCHINFO *patch;

    MSIREG_DeleteProductKey(package->ProductCode);
    MSIREG_DeleteUserDataProductKey(package->ProductCode, package->Context);
    MSIREG_DeleteUninstallKey(package->ProductCode, package->platform);

    MSIREG_DeleteLocalClassesProductKey(package->ProductCode);
    MSIREG_DeleteLocalClassesFeaturesKey(package->ProductCode);
    MSIREG_DeleteUserProductKey(package->ProductCode);
    MSIREG_DeleteUserFeaturesKey(package->ProductCode);

    remove_product_upgrade_code( package );

    LIST_FOR_EACH_ENTRY(patch, &package->patches, MSIPATCHINFO, entry)
    {
        MSIREG_DeleteUserDataPatchKey(patch->patchcode, package->Context);
        if (!wcscmp( package->ProductCode, patch->products ))
        {
            TRACE("removing local patch package %s\n", debugstr_w(patch->localfile));
            patch->delete_on_close = TRUE;
        }
        /* FIXME: remove local patch package if this is the last product */
    }
    TRACE("removing local package %s\n", debugstr_w(package->localfile));
    package->delete_on_close = TRUE;

    unpublish_icons( package );
    return ERROR_SUCCESS;
}

static BOOL is_full_uninstall( MSIPACKAGE *package )
{
    MSIFEATURE *feature;

    LIST_FOR_EACH_ENTRY( feature, &package->features, MSIFEATURE, entry )
    {
        if (feature->Action != INSTALLSTATE_ABSENT &&
                (feature->Installed != INSTALLSTATE_ABSENT || feature->Action != INSTALLSTATE_UNKNOWN))
            return FALSE;
    }

    return TRUE;
}

static UINT ACTION_InstallFinalize(MSIPACKAGE *package)
{
    UINT rc;
    MSIFILE *file;
    MSIFILEPATCH *patch;

    /* first do the same as an InstallExecute */
    rc = execute_script(package, SCRIPT_INSTALL);
    if (rc != ERROR_SUCCESS)
        return rc;

    /* install global assemblies */
    LIST_FOR_EACH_ENTRY( file, &package->files, MSIFILE, entry )
    {
        MSICOMPONENT *comp = file->Component;

        if (!msi_is_global_assembly( comp ) || (file->state != msifs_missing && file->state != msifs_overwrite))
            continue;

        rc = msi_install_assembly( package, comp );
        if (rc != ERROR_SUCCESS)
        {
            ERR("Failed to install assembly\n");
            return ERROR_INSTALL_FAILURE;
        }
        file->state = msifs_installed;
    }

    /* patch global assemblies */
    LIST_FOR_EACH_ENTRY( patch, &package->filepatches, MSIFILEPATCH, entry )
    {
        MSICOMPONENT *comp = patch->File->Component;

        if (!msi_is_global_assembly( comp ) || !patch->path) continue;

        rc = msi_patch_assembly( package, comp->assembly, patch );
        if (rc && !(patch->Attributes & msidbPatchAttributesNonVital))
        {
            ERR("Failed to apply patch to file: %s\n", debugstr_w(patch->File->File));
            return rc;
        }

        if ((rc = msi_install_assembly( package, comp )))
        {
            ERR("Failed to install patched assembly\n");
            return rc;
        }
    }

    /* then handle commit actions */
    rc = execute_script(package, SCRIPT_COMMIT);
    if (rc != ERROR_SUCCESS)
        return rc;

    if (is_full_uninstall(package))
        rc = ACTION_UnpublishProduct(package);

    return rc;
}

UINT ACTION_ForceReboot(MSIPACKAGE *package)
{
    WCHAR buffer[256], sysdir[MAX_PATH], squashed_pc[SQUASHED_GUID_SIZE];
    HKEY hkey;

    squash_guid( package->ProductCode, squashed_pc );

    GetSystemDirectoryW(sysdir, ARRAY_SIZE(sysdir));
    RegCreateKeyW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce", &hkey);
    swprintf(buffer, ARRAY_SIZE(buffer), L"%s\\MsiExec.exe /@ \"%s\"", sysdir, squashed_pc);

    msi_reg_set_val_str( hkey, squashed_pc, buffer );
    RegCloseKey(hkey);

    TRACE("Reboot command %s\n",debugstr_w(buffer));

    RegCreateKeyW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\RunOnceEntries",
                  &hkey);
    swprintf( buffer, ARRAY_SIZE(buffer), L"/I \"%s\" AFTERREBOOT=1 RUNONCEENTRY=\"%s\"", package->ProductCode,
              squashed_pc );

    msi_reg_set_val_str( hkey, squashed_pc, buffer );
    RegCloseKey(hkey);

    return ERROR_INSTALL_SUSPEND;
}

static UINT ACTION_ResolveSource(MSIPACKAGE* package)
{
    DWORD attrib;
    UINT rc;

    /*
     * We are currently doing what should be done here in the top level Install
     * however for Administrative and uninstalls this step will be needed
     */
    if (!package->PackagePath)
        return ERROR_SUCCESS;

    msi_set_sourcedir_props(package, TRUE);

    attrib = GetFileAttributesW(package->db->path);
    if (attrib == INVALID_FILE_ATTRIBUTES)
    {
        MSIRECORD *record;
        LPWSTR prompt;
        DWORD size = 0;

        rc = MsiSourceListGetInfoW(package->ProductCode, NULL, 
                package->Context, MSICODE_PRODUCT,
                INSTALLPROPERTY_DISKPROMPTW,NULL,&size);
        if (rc == ERROR_MORE_DATA)
        {
            prompt = malloc(size * sizeof(WCHAR));
            MsiSourceListGetInfoW(package->ProductCode, NULL, 
                    package->Context, MSICODE_PRODUCT,
                    INSTALLPROPERTY_DISKPROMPTW,prompt,&size);
        }
        else
            prompt = wcsdup(package->db->path);

        record = MSI_CreateRecord(2);
        MSI_RecordSetInteger(record, 1, MSIERR_INSERTDISK);
        MSI_RecordSetStringW(record, 2, prompt);
        free(prompt);
        while(attrib == INVALID_FILE_ATTRIBUTES)
        {
            MSI_RecordSetStringW(record, 0, NULL);
            rc = MSI_ProcessMessage(package, INSTALLMESSAGE_ERROR, record);
            if (rc == IDCANCEL)
            {
                msiobj_release(&record->hdr);
                return ERROR_INSTALL_USEREXIT;
            }
            attrib = GetFileAttributesW(package->db->path);
        }
        msiobj_release(&record->hdr);
        rc = ERROR_SUCCESS;
    }
    else
        return ERROR_SUCCESS;

    return rc;
}

static UINT ACTION_RegisterUser(MSIPACKAGE *package)
{
    static const WCHAR szPropKeys[][80] =
    {
        L"ProductID",
        L"USERNAME",
        L"COMPANYNAME",
        L"",
    };
    static const WCHAR szRegKeys[][80] =
    {
        L"ProductID",
        L"RegOwner",
        L"RegCompany",
        L"",
    };
    HKEY hkey = 0;
    LPWSTR buffer, productid = NULL;
    UINT i, rc = ERROR_SUCCESS;
    MSIRECORD *uirow;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action(package, SCRIPT_INSTALL, L"RegisterUser");

    if (check_unpublish(package))
    {
        MSIREG_DeleteUserDataProductKey(package->ProductCode, package->Context);
        goto end;
    }

    productid = msi_dup_property( package->db, INSTALLPROPERTY_PRODUCTIDW );
    if (!productid)
        goto end;

    rc = MSIREG_OpenInstallProps(package->ProductCode, package->Context,
                                 NULL, &hkey, TRUE);
    if (rc != ERROR_SUCCESS)
        goto end;

    for( i = 0; szPropKeys[i][0]; i++ )
    {
        buffer = msi_dup_property( package->db, szPropKeys[i] );
        msi_reg_set_val_str( hkey, szRegKeys[i], buffer );
        free( buffer );
    }

end:
    uirow = MSI_CreateRecord( 1 );
    MSI_RecordSetStringW( uirow, 1, productid );
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
    msiobj_release( &uirow->hdr );

    free(productid);
    RegCloseKey(hkey);
    return rc;
}

static UINT iterate_properties(MSIRECORD *record, void *param)
{
    MSIRECORD *uirow;

    uirow = MSI_CloneRecord(record);
    if (!uirow) return ERROR_OUTOFMEMORY;
    MSI_RecordSetStringW(uirow, 0, L"Property(S): [1] = [2]");
    MSI_ProcessMessage(param, INSTALLMESSAGE_INFO|MB_ICONHAND, uirow);
    msiobj_release(&uirow->hdr);

    return ERROR_SUCCESS;
}


static UINT ACTION_ExecuteAction(MSIPACKAGE *package)
{
    WCHAR *productname;
    WCHAR *action;
    WCHAR *info_template;
    MSIQUERY *view;
    MSIRECORD *uirow, *uirow_info;
    UINT rc;

    /* Send COMMONDATA and INFO messages. */
    /* FIXME: when should these messages be sent? [see also MsiOpenPackage()] */
    uirow = MSI_CreateRecord(3);
    if (!uirow) return ERROR_OUTOFMEMORY;
    MSI_RecordSetStringW(uirow, 0, NULL);
    MSI_RecordSetInteger(uirow, 1, 0);
    MSI_RecordSetInteger(uirow, 2, package->num_langids ? package->langids[0] : 0);
    MSI_RecordSetInteger(uirow, 3, msi_get_string_table_codepage(package->db->strings));
    MSI_ProcessMessageVerbatim(package, INSTALLMESSAGE_COMMONDATA, uirow);
    /* FIXME: send INSTALLMESSAGE_PROGRESS */
    MSI_ProcessMessageVerbatim(package, INSTALLMESSAGE_COMMONDATA, uirow);

    if (!(needs_ui_sequence(package) && ui_sequence_exists(package)))
    {
        uirow_info = MSI_CreateRecord(0);
        if (!uirow_info)
        {
            msiobj_release(&uirow->hdr);
            return ERROR_OUTOFMEMORY;
        }
        info_template = msi_get_error_message(package->db, MSIERR_INFO_LOGGINGSTART);
        MSI_RecordSetStringW(uirow_info, 0, info_template);
        free(info_template);
        MSI_ProcessMessage(package, INSTALLMESSAGE_INFO|MB_ICONHAND, uirow_info);
        msiobj_release(&uirow_info->hdr);
    }

    MSI_ProcessMessage(package, INSTALLMESSAGE_COMMONDATA, uirow);

    productname = msi_dup_property(package->db, INSTALLPROPERTY_PRODUCTNAMEW);
    MSI_RecordSetInteger(uirow, 1, 1);
    MSI_RecordSetStringW(uirow, 2, productname);
    MSI_RecordSetStringW(uirow, 3, NULL);
    MSI_ProcessMessage(package, INSTALLMESSAGE_COMMONDATA, uirow);
    msiobj_release(&uirow->hdr);

    package->LastActionResult = MSI_NULL_INTEGER;

    action = msi_dup_property(package->db, L"EXECUTEACTION");
    if (!action) action = msi_strdupW(L"INSTALL", ARRAY_SIZE(L"INSTALL") - 1);

    /* Perform the action. Top-level actions trigger a sequence. */
    if (!wcscmp(action, L"INSTALL"))
    {
        /* Send ACTIONSTART/INFO and INSTALLSTART. */
        ui_actionstart(package, L"INSTALL", NULL, NULL);
        ui_actioninfo(package, L"INSTALL", TRUE, 0);
        uirow = MSI_CreateRecord(2);
        if (!uirow)
        {
            rc = ERROR_OUTOFMEMORY;
            goto end;
        }
        MSI_RecordSetStringW(uirow, 0, NULL);
        MSI_RecordSetStringW(uirow, 1, productname);
        MSI_RecordSetStringW(uirow, 2, package->ProductCode);
        MSI_ProcessMessage(package, INSTALLMESSAGE_INSTALLSTART, uirow);
        msiobj_release(&uirow->hdr);

        /* Perform the installation. Always use the ExecuteSequence. */
        package->InWhatSequence |= SEQUENCE_EXEC;
        rc = ACTION_ProcessExecSequence(package);

        /* Send return value and INSTALLEND. */
        ui_actioninfo(package, L"INSTALL", FALSE, !rc);
        uirow = MSI_CreateRecord(3);
        if (!uirow)
        {
            rc = ERROR_OUTOFMEMORY;
            goto end;
        }
        MSI_RecordSetStringW(uirow, 0, NULL);
        MSI_RecordSetStringW(uirow, 1, productname);
        MSI_RecordSetStringW(uirow, 2, package->ProductCode);
        MSI_RecordSetInteger(uirow, 3, !rc);
        MSI_ProcessMessage(package, INSTALLMESSAGE_INSTALLEND, uirow);
        msiobj_release(&uirow->hdr);
    }
    else
        rc = ACTION_PerformAction(package, action);

    /* Send all set properties. */
    if (!MSI_OpenQuery(package->db, &view, L"SELECT * FROM `_Property`"))
    {
        MSI_IterateRecords(view, NULL, iterate_properties, package);
        msiobj_release(&view->hdr);
    }

    /* And finally, toggle the cancel off and on. */
    uirow = MSI_CreateRecord(2);
    if (!uirow)
    {
        rc = ERROR_OUTOFMEMORY;
        goto end;
    }
    MSI_RecordSetStringW(uirow, 0, NULL);
    MSI_RecordSetInteger(uirow, 1, 2);
    MSI_RecordSetInteger(uirow, 2, 0);
    MSI_ProcessMessageVerbatim(package, INSTALLMESSAGE_COMMONDATA, uirow);
    MSI_RecordSetInteger(uirow, 2, 1);
    MSI_ProcessMessageVerbatim(package, INSTALLMESSAGE_COMMONDATA, uirow);
    msiobj_release(&uirow->hdr);

end:
    free(productname);
    free(action);
    return rc;
}

static UINT ACTION_INSTALL(MSIPACKAGE *package)
{
    msi_set_property(package->db, L"EXECUTEACTION", L"INSTALL", -1);
    if (needs_ui_sequence(package) && ui_sequence_exists(package))
    {
        package->InWhatSequence |= SEQUENCE_UI;
        return ACTION_ProcessUISequence(package);
    }
    else
        return ACTION_ExecuteAction(package);
}

WCHAR *msi_create_component_advertise_string( MSIPACKAGE *package, MSICOMPONENT *component, const WCHAR *feature )
{
    WCHAR productid_85[21], component_85[21], *ret;
    GUID clsid;
    DWORD sz;

    /* > is used if there is a component GUID and < if not.  */

    productid_85[0] = 0;
    component_85[0] = 0;
    CLSIDFromString( package->ProductCode, &clsid );

    encode_base85_guid( &clsid, productid_85 );
    if (component)
    {
        CLSIDFromString( component->ComponentId, &clsid );
        encode_base85_guid( &clsid, component_85 );
    }

    TRACE("product=%s feature=%s component=%s\n", debugstr_w(productid_85), debugstr_w(feature),
          debugstr_w(component_85));

    sz = 20 + lstrlenW( feature ) + 20 + 3;
    ret = calloc( 1, sz * sizeof(WCHAR) );
    if (ret) swprintf( ret, sz, L"%s%s%c%s", productid_85, feature, component ? '>' : '<', component_85 );
    return ret;
}

static UINT ITERATE_PublishComponent(MSIRECORD *rec, LPVOID param)
{
    MSIPACKAGE *package = param;
    LPCWSTR compgroupid, component, feature, qualifier, text;
    LPWSTR advertise = NULL, output = NULL, existing = NULL, p, q;
    HKEY hkey = NULL;
    UINT rc;
    MSICOMPONENT *comp;
    MSIFEATURE *feat;
    DWORD sz;
    MSIRECORD *uirow;
    int len;

    feature = MSI_RecordGetString(rec, 5);
    feat = msi_get_loaded_feature(package, feature);
    if (!feat)
        return ERROR_SUCCESS;

    feat->Action = msi_get_feature_action( package, feat );
    if (feat->Action != INSTALLSTATE_LOCAL &&
        feat->Action != INSTALLSTATE_SOURCE &&
        feat->Action != INSTALLSTATE_ADVERTISED)
    {
        TRACE("feature not scheduled for installation %s\n", debugstr_w(feature));
        return ERROR_SUCCESS;
    }

    component = MSI_RecordGetString(rec, 3);
    comp = msi_get_loaded_component(package, component);
    if (!comp)
        return ERROR_SUCCESS;

    compgroupid = MSI_RecordGetString(rec,1);
    qualifier = MSI_RecordGetString(rec,2);

    rc = MSIREG_OpenUserComponentsKey(compgroupid, &hkey, TRUE);
    if (rc != ERROR_SUCCESS)
        goto end;

    advertise = msi_create_component_advertise_string( package, comp, feature );
    text = MSI_RecordGetString( rec, 4 );
    if (text)
    {
        p = malloc( (wcslen( advertise ) + wcslen( text ) + 1) * sizeof(WCHAR) );
        lstrcpyW( p, advertise );
        lstrcatW( p, text );
        free( advertise );
        advertise = p;
    }
    existing = msi_reg_get_val_str( hkey, qualifier );

    sz = lstrlenW( advertise ) + 1;
    if (existing)
    {
        for (p = existing; *p; p += len)
        {
            len = lstrlenW( p ) + 1;
            if (wcscmp( advertise, p )) sz += len;
        }
    }
    if (!(output = malloc( (sz + 1) * sizeof(WCHAR) )))
    {
        rc = ERROR_OUTOFMEMORY;
        goto end;
    }
    q = output;
    if (existing)
    {
        for (p = existing; *p; p += len)
        {
            len = lstrlenW( p ) + 1;
            if (wcscmp( advertise, p ))
            {
                memcpy( q, p, len * sizeof(WCHAR) );
                q += len;
            }
        }
    }
    lstrcpyW( q, advertise );
    q[lstrlenW( q ) + 1] = 0;

    msi_reg_set_val_multi_str( hkey, qualifier, output );
    
end:
    RegCloseKey(hkey);
    free( output );
    free( advertise );
    free( existing );

    /* the UI chunk */
    uirow = MSI_CreateRecord( 2 );
    MSI_RecordSetStringW( uirow, 1, compgroupid );
    MSI_RecordSetStringW( uirow, 2, qualifier);
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
    msiobj_release( &uirow->hdr );
    /* FIXME: call ui_progress? */

    return rc;
}

/*
 * At present I am ignoring the advertised components part of this and only
 * focusing on the qualified component sets
 */
static UINT ACTION_PublishComponents(MSIPACKAGE *package)
{
    MSIQUERY *view;
    UINT rc;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action(package, SCRIPT_INSTALL, L"PublishComponents");

    rc = MSI_DatabaseOpenViewW(package->db, L"SELECT * FROM `PublishComponent`", &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords(view, NULL, ITERATE_PublishComponent, package);
    msiobj_release(&view->hdr);
    return rc;
}

static UINT ITERATE_UnpublishComponent( MSIRECORD *rec, LPVOID param )
{
    MSIPACKAGE *package = param;
    LPCWSTR compgroupid, component, feature, qualifier;
    MSICOMPONENT *comp;
    MSIFEATURE *feat;
    MSIRECORD *uirow;
    WCHAR squashed[GUID_SIZE], keypath[MAX_PATH];
    LONG res;

    feature = MSI_RecordGetString( rec, 5 );
    feat = msi_get_loaded_feature( package, feature );
    if (!feat)
        return ERROR_SUCCESS;

    feat->Action = msi_get_feature_action( package, feat );
    if (feat->Action != INSTALLSTATE_ABSENT)
    {
        TRACE("feature not scheduled for removal %s\n", debugstr_w(feature));
        return ERROR_SUCCESS;
    }

    component = MSI_RecordGetString( rec, 3 );
    comp = msi_get_loaded_component( package, component );
    if (!comp)
        return ERROR_SUCCESS;

    compgroupid = MSI_RecordGetString( rec, 1 );
    qualifier = MSI_RecordGetString( rec, 2 );

    squash_guid( compgroupid, squashed );
    lstrcpyW( keypath, L"Software\\Microsoft\\Installer\\Components\\" );
    lstrcatW( keypath, squashed );

    res = RegDeleteKeyW( HKEY_CURRENT_USER, keypath );
    if (res != ERROR_SUCCESS)
    {
        WARN( "unable to delete component key %ld\n", res );
    }

    uirow = MSI_CreateRecord( 2 );
    MSI_RecordSetStringW( uirow, 1, compgroupid );
    MSI_RecordSetStringW( uirow, 2, qualifier );
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
    msiobj_release( &uirow->hdr );

    return ERROR_SUCCESS;
}

static UINT ACTION_UnpublishComponents( MSIPACKAGE *package )
{
    MSIQUERY *view;
    UINT rc;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action(package, SCRIPT_INSTALL, L"UnpublishComponents");

    rc = MSI_DatabaseOpenViewW( package->db, L"SELECT * FROM `PublishComponent`", &view );
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords( view, NULL, ITERATE_UnpublishComponent, package );
    msiobj_release( &view->hdr );
    return rc;
}

static UINT ITERATE_InstallService(MSIRECORD *rec, LPVOID param)
{
    MSIPACKAGE *package = param;
    MSICOMPONENT *component;
    MSIRECORD *row;
    MSIFILE *file;
    SC_HANDLE hscm = NULL, service = NULL;
    LPCWSTR comp, key;
    LPWSTR name = NULL, disp = NULL, load_order = NULL, serv_name = NULL;
    LPWSTR depends = NULL, pass = NULL, args = NULL, image_path = NULL;
    DWORD serv_type, start_type, err_control;
    BOOL is_vital;
    SERVICE_DESCRIPTIONW sd = {NULL};
    UINT ret = ERROR_SUCCESS;

    comp = MSI_RecordGetString( rec, 12 );
    component = msi_get_loaded_component( package, comp );
    if (!component)
    {
        WARN("service component not found\n");
        goto done;
    }
    component->Action = msi_get_component_action( package, component );
    if (component->Action != INSTALLSTATE_LOCAL)
    {
        TRACE("component not scheduled for installation %s\n", debugstr_w(comp));
        goto done;
    }
    hscm = OpenSCManagerW(NULL, SERVICES_ACTIVE_DATABASEW, GENERIC_WRITE);
    if (!hscm)
    {
        ERR("Failed to open the SC Manager!\n");
        goto done;
    }

    start_type = MSI_RecordGetInteger(rec, 5);
    if (start_type == SERVICE_BOOT_START || start_type == SERVICE_SYSTEM_START)
        goto done;

    deformat_string(package, MSI_RecordGetString(rec, 2), &name);
    deformat_string(package, MSI_RecordGetString(rec, 3), &disp);
    serv_type = MSI_RecordGetInteger(rec, 4);
    err_control = MSI_RecordGetInteger(rec, 6);
    deformat_string(package, MSI_RecordGetString(rec, 7), &load_order);
    deformat_string(package, MSI_RecordGetString(rec, 8), &depends);
    deformat_string(package, MSI_RecordGetString(rec, 9), &serv_name);
    deformat_string(package, MSI_RecordGetString(rec, 10), &pass);
    deformat_string(package, MSI_RecordGetString(rec, 11), &args);
    deformat_string(package, MSI_RecordGetString(rec, 13), &sd.lpDescription);

    /* Should the complete install fail if CreateService fails? */
    is_vital = (err_control & msidbServiceInstallErrorControlVital);

    /* Remove the msidbServiceInstallErrorControlVital-flag from err_control.
       CreateService (under Windows) would fail if not. */
    err_control &= ~msidbServiceInstallErrorControlVital;

    /* fetch the service path */
    row = MSI_QueryGetRecord(package->db, L"SELECT * FROM `Component` WHERE `Component` = '%s'", comp);
    if (!row)
    {
        ERR("Query failed\n");
        goto done;
    }
    if (!(key = MSI_RecordGetString(row, 6)))
    {
        msiobj_release(&row->hdr);
        goto done;
    }
    file = msi_get_loaded_file(package, key);
    msiobj_release(&row->hdr);
    if (!file)
    {
        ERR("Failed to load the service file\n");
        goto done;
    }

    if (!args || !args[0]) image_path = file->TargetPath;
    else
    {
        int len = lstrlenW(file->TargetPath) + lstrlenW(args) + 2;
        if (!(image_path = malloc(len * sizeof(WCHAR))))
        {
            ret = ERROR_OUTOFMEMORY;
            goto done;
        }

        lstrcpyW(image_path, file->TargetPath);
        lstrcatW(image_path, L" ");
        lstrcatW(image_path, args);
    }
    service = CreateServiceW(hscm, name, disp, GENERIC_ALL, serv_type,
                             start_type, err_control, image_path, load_order,
                             NULL, depends, serv_name, pass);

    if (!service)
    {
        if (GetLastError() != ERROR_SERVICE_EXISTS)
        {
            WARN( "failed to create service %s (%lu)\n", debugstr_w(name), GetLastError() );
            if (is_vital)
                ret = ERROR_INSTALL_FAILURE;

        }
    }
    else if (sd.lpDescription)
    {
        if (!ChangeServiceConfig2W(service, SERVICE_CONFIG_DESCRIPTION, &sd))
            WARN( "failed to set service description %lu\n", GetLastError() );
    }

    if (image_path != file->TargetPath) free(image_path);
done:
    if (service) CloseServiceHandle(service);
    if (hscm) CloseServiceHandle(hscm);
    free(name);
    free(disp);
    free(sd.lpDescription);
    free(load_order);
    free(serv_name);
    free(pass);
    free(depends);
    free(args);

    return ret;
}

static UINT ACTION_InstallServices( MSIPACKAGE *package )
{
    MSIQUERY *view;
    UINT rc;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action(package, SCRIPT_INSTALL, L"InstallServices");

    rc = MSI_DatabaseOpenViewW(package->db, L"SELECT * FROM `ServiceInstall`", &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords(view, NULL, ITERATE_InstallService, package);
    msiobj_release(&view->hdr);
    return rc;
}

/* converts arg1[~]arg2[~]arg3 to a list of ptrs to the strings */
static const WCHAR **service_args_to_vector(WCHAR *args, DWORD *numargs)
{
    LPCWSTR *vector, *temp_vector;
    LPWSTR p, q;
    DWORD sep_len;

    *numargs = 0;
    sep_len = ARRAY_SIZE(L"[~]") - 1;

    if (!args)
        return NULL;

    vector = malloc(sizeof(WCHAR *));
    if (!vector)
        return NULL;

    p = args;
    do
    {
        (*numargs)++;
        vector[*numargs - 1] = p;

        if ((q = wcsstr(p, L"[~]")))
        {
            *q = '\0';

            temp_vector = realloc(vector, (*numargs + 1) * sizeof(WCHAR *));
            if (!temp_vector)
            {
                free(vector);
                return NULL;
            }
            vector = temp_vector;

            p = q + sep_len;
        }
    } while (q);

    return vector;
}

static UINT ITERATE_StartService(MSIRECORD *rec, LPVOID param)
{
    MSIPACKAGE *package = param;
    MSICOMPONENT *comp;
    MSIRECORD *uirow;
    SC_HANDLE scm = NULL, service = NULL;
    LPCWSTR component, *vector = NULL;
    LPWSTR name, args, display_name = NULL;
    DWORD event, numargs, len, wait, dummy;
    UINT r = ERROR_FUNCTION_FAILED;
    SERVICE_STATUS_PROCESS status;
    ULONGLONG start_time;

    component = MSI_RecordGetString(rec, 6);
    comp = msi_get_loaded_component(package, component);
    if (!comp)
        return ERROR_SUCCESS;

    event = MSI_RecordGetInteger( rec, 3 );
    deformat_string( package, MSI_RecordGetString( rec, 2 ), &name );

    comp->Action = msi_get_component_action( package, comp );
    if (!(comp->Action == INSTALLSTATE_LOCAL && (event & msidbServiceControlEventStart)) &&
        !(comp->Action == INSTALLSTATE_ABSENT && (event & msidbServiceControlEventUninstallStart)))
    {
        TRACE("not starting %s\n", debugstr_w(name));
        free(name);
        return ERROR_SUCCESS;
    }

    deformat_string(package, MSI_RecordGetString(rec, 4), &args);
    wait = MSI_RecordGetInteger(rec, 5);

    scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm)
    {
        ERR("Failed to open the service control manager\n");
        goto done;
    }

    len = 0;
    if (!GetServiceDisplayNameW( scm, name, NULL, &len ) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        if ((display_name = malloc(++len * sizeof(WCHAR))))
            GetServiceDisplayNameW( scm, name, display_name, &len );
    }

    service = OpenServiceW(scm, name, SERVICE_START|SERVICE_QUERY_STATUS);
    if (!service)
    {
        ERR( "failed to open service %s (%lu)\n", debugstr_w(name), GetLastError() );
        goto done;
    }

    vector = service_args_to_vector(args, &numargs);

    if (!StartServiceW(service, numargs, vector) &&
        GetLastError() != ERROR_SERVICE_ALREADY_RUNNING)
    {
        ERR( "failed to start service %s (%lu)\n", debugstr_w(name), GetLastError() );
        goto done;
    }

    r = ERROR_SUCCESS;
    if (wait)
    {
        /* wait for at most 30 seconds for the service to be up and running */
        if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO,
            (BYTE *)&status, sizeof(SERVICE_STATUS_PROCESS), &dummy))
        {
            TRACE( "failed to query service status (%lu)\n", GetLastError() );
            goto done;
        }
        start_time = GetTickCount64();
        while (status.dwCurrentState == SERVICE_START_PENDING)
        {
            if (GetTickCount64() - start_time > 30000) break;
            Sleep(1000);
            if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO,
                (BYTE *)&status, sizeof(SERVICE_STATUS_PROCESS), &dummy))
            {
                TRACE( "failed to query service status (%lu)\n", GetLastError() );
                goto done;
            }
        }
        if (status.dwCurrentState != SERVICE_RUNNING)
        {
            WARN( "service failed to start %lu\n", status.dwCurrentState );
            r = ERROR_FUNCTION_FAILED;
        }
    }

done:
    uirow = MSI_CreateRecord( 2 );
    MSI_RecordSetStringW( uirow, 1, display_name );
    MSI_RecordSetStringW( uirow, 2, name );
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
    msiobj_release( &uirow->hdr );

    if (service) CloseServiceHandle(service);
    if (scm) CloseServiceHandle(scm);

    free(name);
    free(args);
    free(vector);
    free(display_name);
    return r;
}

static UINT ACTION_StartServices( MSIPACKAGE *package )
{
    MSIQUERY *view;
    UINT rc;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action(package, SCRIPT_INSTALL, L"StartServices");

    rc = MSI_DatabaseOpenViewW(package->db, L"SELECT * FROM `ServiceControl`", &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords(view, NULL, ITERATE_StartService, package);
    msiobj_release(&view->hdr);
    return rc;
}

static BOOL stop_service_dependents(SC_HANDLE scm, SC_HANDLE service)
{
    DWORD i, needed, count;
    ENUM_SERVICE_STATUSW *dependencies;
    SERVICE_STATUS ss;
    SC_HANDLE depserv;
    BOOL stopped, ret = FALSE;

    if (EnumDependentServicesW(service, SERVICE_ACTIVE, NULL,
                               0, &needed, &count))
        return TRUE;

    if (GetLastError() != ERROR_MORE_DATA)
        return FALSE;

    dependencies = malloc(needed);
    if (!dependencies)
        return FALSE;

    if (!EnumDependentServicesW(service, SERVICE_ACTIVE, dependencies,
                                needed, &needed, &count))
        goto done;

    for (i = 0; i < count; i++)
    {
        depserv = OpenServiceW(scm, dependencies[i].lpServiceName,
                               SERVICE_STOP | SERVICE_QUERY_STATUS);
        if (!depserv)
            goto done;

        stopped = ControlService(depserv, SERVICE_CONTROL_STOP, &ss);
        CloseServiceHandle(depserv);
        if (!stopped)
            goto done;
    }

    ret = TRUE;

done:
    free(dependencies);
    return ret;
}

static UINT stop_service( LPCWSTR name )
{
    SC_HANDLE scm = NULL, service = NULL;
    SERVICE_STATUS status;
    SERVICE_STATUS_PROCESS ssp;
    DWORD needed;

    scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!scm)
    {
        WARN( "failed to open the SCM (%lu)\n", GetLastError() );
        goto done;
    }

    service = OpenServiceW(scm, name, SERVICE_STOP | SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS);
    if (!service)
    {
        WARN( "failed to open service %s (%lu)\n", debugstr_w(name), GetLastError() );
        goto done;
    }

    if (!QueryServiceStatusEx( service, SC_STATUS_PROCESS_INFO, (BYTE *)&ssp, sizeof(ssp), &needed) )
    {
        WARN( "failed to query service status %s (%lu)\n", debugstr_w(name), GetLastError() );
        goto done;
    }

    if (ssp.dwCurrentState == SERVICE_STOPPED)
        goto done;

    stop_service_dependents(scm, service);

    if (!ControlService(service, SERVICE_CONTROL_STOP, &status))
        WARN( "failed to stop service %s (%lu)\n", debugstr_w(name), GetLastError() );

done:
    if (service) CloseServiceHandle(service);
    if (scm) CloseServiceHandle(scm);

    return ERROR_SUCCESS;
}

static UINT ITERATE_StopService( MSIRECORD *rec, LPVOID param )
{
    MSIPACKAGE *package = param;
    MSICOMPONENT *comp;
    MSIRECORD *uirow;
    LPCWSTR component;
    WCHAR *name, *display_name = NULL;
    DWORD event, len;
    SC_HANDLE scm;

    component = MSI_RecordGetString( rec, 6 );
    comp = msi_get_loaded_component( package, component );
    if (!comp)
        return ERROR_SUCCESS;

    event = MSI_RecordGetInteger( rec, 3 );
    deformat_string( package, MSI_RecordGetString( rec, 2 ), &name );

    comp->Action = msi_get_component_action( package, comp );
    if (!(comp->Action == INSTALLSTATE_LOCAL && (event & msidbServiceControlEventStop)) &&
        !(comp->Action == INSTALLSTATE_ABSENT && (event & msidbServiceControlEventUninstallStop)))
    {
        TRACE("not stopping %s\n", debugstr_w(name));
        free( name );
        return ERROR_SUCCESS;
    }

    scm = OpenSCManagerW( NULL, NULL, SC_MANAGER_CONNECT );
    if (!scm)
    {
        ERR("Failed to open the service control manager\n");
        goto done;
    }

    len = 0;
    if (!GetServiceDisplayNameW( scm, name, NULL, &len ) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        if ((display_name = malloc( ++len * sizeof(WCHAR ))))
            GetServiceDisplayNameW( scm, name, display_name, &len );
    }
    CloseServiceHandle( scm );

    stop_service( name );

done:
    uirow = MSI_CreateRecord( 2 );
    MSI_RecordSetStringW( uirow, 1, display_name );
    MSI_RecordSetStringW( uirow, 2, name );
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
    msiobj_release( &uirow->hdr );

    free( name );
    free( display_name );
    return ERROR_SUCCESS;
}

static UINT ACTION_StopServices( MSIPACKAGE *package )
{
    MSIQUERY *view;
    UINT rc;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action(package, SCRIPT_INSTALL, L"StopServices");

    rc = MSI_DatabaseOpenViewW(package->db, L"SELECT * FROM `ServiceControl`", &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords(view, NULL, ITERATE_StopService, package);
    msiobj_release(&view->hdr);
    return rc;
}

static UINT ITERATE_DeleteService( MSIRECORD *rec, LPVOID param )
{
    MSIPACKAGE *package = param;
    MSICOMPONENT *comp;
    MSIRECORD *uirow;
    LPWSTR name = NULL, display_name = NULL;
    DWORD event, len;
    SC_HANDLE scm = NULL, service = NULL;

    comp = msi_get_loaded_component( package, MSI_RecordGetString(rec, 6) );
    if (!comp)
        return ERROR_SUCCESS;

    event = MSI_RecordGetInteger( rec, 3 );
    deformat_string( package, MSI_RecordGetString(rec, 2), &name );

    comp->Action = msi_get_component_action( package, comp );
    if (!(comp->Action == INSTALLSTATE_LOCAL && (event & msidbServiceControlEventDelete)) &&
        !(comp->Action == INSTALLSTATE_ABSENT && (event & msidbServiceControlEventUninstallDelete)))
    {
        TRACE("service %s not scheduled for removal\n", debugstr_w(name));
        free( name );
        return ERROR_SUCCESS;
    }
    stop_service( name );

    scm = OpenSCManagerW( NULL, NULL, SC_MANAGER_ALL_ACCESS );
    if (!scm)
    {
        WARN( "failed to open the SCM (%lu)\n", GetLastError() );
        goto done;
    }

    len = 0;
    if (!GetServiceDisplayNameW( scm, name, NULL, &len ) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        if ((display_name = malloc( ++len * sizeof(WCHAR ))))
            GetServiceDisplayNameW( scm, name, display_name, &len );
    }

    service = OpenServiceW( scm, name, DELETE );
    if (!service)
    {
        WARN( "failed to open service %s (%lu)\n", debugstr_w(name), GetLastError() );
        goto done;
    }

    if (!DeleteService( service ))
        WARN( "failed to delete service %s (%lu)\n", debugstr_w(name), GetLastError() );

done:
    uirow = MSI_CreateRecord( 2 );
    MSI_RecordSetStringW( uirow, 1, display_name );
    MSI_RecordSetStringW( uirow, 2, name );
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
    msiobj_release( &uirow->hdr );

    if (service) CloseServiceHandle( service );
    if (scm) CloseServiceHandle( scm );
    free( name );
    free( display_name );

    return ERROR_SUCCESS;
}

static UINT ACTION_DeleteServices( MSIPACKAGE *package )
{
    MSIQUERY *view;
    UINT rc;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action(package, SCRIPT_INSTALL, L"DeleteServices");

    rc = MSI_DatabaseOpenViewW( package->db, L"SELECT * FROM `ServiceControl`", &view );
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords( view, NULL, ITERATE_DeleteService, package );
    msiobj_release( &view->hdr );
    return rc;
}

static UINT ITERATE_InstallODBCDriver( MSIRECORD *rec, LPVOID param )
{
    MSIPACKAGE *package = param;
    LPWSTR driver, driver_path, ptr;
    WCHAR outpath[MAX_PATH];
    MSIFILE *driver_file = NULL, *setup_file = NULL;
    MSICOMPONENT *comp;
    MSIRECORD *uirow;
    LPCWSTR desc, file_key, component;
    DWORD len, usage;
    UINT r = ERROR_SUCCESS;

    component = MSI_RecordGetString( rec, 2 );
    comp = msi_get_loaded_component( package, component );
    if (!comp)
        return ERROR_SUCCESS;

    comp->Action = msi_get_component_action( package, comp );
    if (comp->Action != INSTALLSTATE_LOCAL)
    {
        TRACE("component not scheduled for installation %s\n", debugstr_w(component));
        return ERROR_SUCCESS;
    }
    desc = MSI_RecordGetString(rec, 3);

    file_key = MSI_RecordGetString( rec, 4 );
    if (file_key) driver_file = msi_get_loaded_file( package, file_key );

    file_key = MSI_RecordGetString( rec, 5 );
    if (file_key) setup_file = msi_get_loaded_file( package, file_key );

    if (!driver_file)
    {
        ERR("ODBC Driver entry not found!\n");
        return ERROR_FUNCTION_FAILED;
    }

    len = lstrlenW(desc) + lstrlenW(L"Driver=%s") + lstrlenW(driver_file->FileName);
    if (setup_file)
        len += lstrlenW(L"Setup=%s") + lstrlenW(setup_file->FileName);
    len += lstrlenW(L"FileUsage=1") + 2; /* \0\0 */

    driver = malloc(len * sizeof(WCHAR));
    if (!driver)
        return ERROR_OUTOFMEMORY;

    ptr = driver;
    lstrcpyW(ptr, desc);
    ptr += lstrlenW(ptr) + 1;

    len = swprintf(ptr, len - (ptr - driver), L"Driver=%s", driver_file->FileName);
    ptr += len + 1;

    if (setup_file)
    {
        len = swprintf(ptr, len - (ptr - driver), L"Setup=%s", setup_file->FileName);
        ptr += len + 1;
    }

    lstrcpyW(ptr, L"FileUsage=1");
    ptr += lstrlenW(ptr) + 1;
    *ptr = '\0';

    if (!driver_file->TargetPath)
    {
        const WCHAR *dir = msi_get_target_folder( package, driver_file->Component->Directory );
        driver_file->TargetPath = msi_build_directory_name( 2, dir, driver_file->FileName );
    }
    driver_path = wcsdup(driver_file->TargetPath);
    ptr = wcsrchr(driver_path, '\\');
    if (ptr) *ptr = '\0';

    if (!SQLInstallDriverExW(driver, driver_path, outpath, MAX_PATH,
                             NULL, ODBC_INSTALL_COMPLETE, &usage))
    {
        ERR("Failed to install SQL driver!\n");
        r = ERROR_FUNCTION_FAILED;
    }

    uirow = MSI_CreateRecord( 5 );
    MSI_RecordSetStringW( uirow, 1, desc );
    MSI_RecordSetStringW( uirow, 2, MSI_RecordGetString(rec, 2) );
    MSI_RecordSetStringW( uirow, 3, driver_file->Component->Directory );
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
    msiobj_release( &uirow->hdr );

    free(driver);
    free(driver_path);

    return r;
}

static UINT ITERATE_InstallODBCTranslator( MSIRECORD *rec, LPVOID param )
{
    MSIPACKAGE *package = param;
    LPWSTR translator, translator_path, ptr;
    WCHAR outpath[MAX_PATH];
    MSIFILE *translator_file = NULL, *setup_file = NULL;
    MSICOMPONENT *comp;
    MSIRECORD *uirow;
    LPCWSTR desc, file_key, component;
    DWORD len, usage;
    UINT r = ERROR_SUCCESS;

    component = MSI_RecordGetString( rec, 2 );
    comp = msi_get_loaded_component( package, component );
    if (!comp)
        return ERROR_SUCCESS;

    comp->Action = msi_get_component_action( package, comp );
    if (comp->Action != INSTALLSTATE_LOCAL)
    {
        TRACE("component not scheduled for installation %s\n", debugstr_w(component));
        return ERROR_SUCCESS;
    }
    desc = MSI_RecordGetString(rec, 3);

    file_key = MSI_RecordGetString( rec, 4 );
    if (file_key) translator_file = msi_get_loaded_file( package, file_key );

    file_key = MSI_RecordGetString( rec, 5 );
    if (file_key) setup_file = msi_get_loaded_file( package, file_key );

    if (!translator_file)
    {
        ERR("ODBC Translator entry not found!\n");
        return ERROR_FUNCTION_FAILED;
    }

    len = lstrlenW(desc) + lstrlenW(L"Translator=%s") + lstrlenW(translator_file->FileName) + 2; /* \0\0 */
    if (setup_file)
        len += lstrlenW(L"Setup=%s") + lstrlenW(setup_file->FileName);

    translator = malloc(len * sizeof(WCHAR));
    if (!translator)
        return ERROR_OUTOFMEMORY;

    ptr = translator;
    lstrcpyW(ptr, desc);
    ptr += lstrlenW(ptr) + 1;

    len = swprintf(ptr, len - (ptr - translator), L"Translator=%s", translator_file->FileName);
    ptr += len + 1;

    if (setup_file)
    {
        len = swprintf(ptr, len - (ptr - translator), L"Setup=%s", setup_file->FileName);
        ptr += len + 1;
    }
    *ptr = '\0';

    translator_path = wcsdup(translator_file->TargetPath);
    ptr = wcsrchr(translator_path, '\\');
    if (ptr) *ptr = '\0';

    if (!SQLInstallTranslatorExW(translator, translator_path, outpath, MAX_PATH,
                                 NULL, ODBC_INSTALL_COMPLETE, &usage))
    {
        ERR("Failed to install SQL translator!\n");
        r = ERROR_FUNCTION_FAILED;
    }

    uirow = MSI_CreateRecord( 5 );
    MSI_RecordSetStringW( uirow, 1, desc );
    MSI_RecordSetStringW( uirow, 2, MSI_RecordGetString(rec, 2) );
    MSI_RecordSetStringW( uirow, 3, translator_file->Component->Directory );
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
    msiobj_release( &uirow->hdr );

    free(translator);
    free(translator_path);

    return r;
}

static UINT ITERATE_InstallODBCDataSource( MSIRECORD *rec, LPVOID param )
{
    MSIPACKAGE *package = param;
    MSICOMPONENT *comp;
    LPWSTR attrs;
    LPCWSTR desc, driver, component;
    WORD request = ODBC_ADD_SYS_DSN;
    INT registration;
    DWORD len;
    UINT r = ERROR_SUCCESS;
    MSIRECORD *uirow;

    component = MSI_RecordGetString( rec, 2 );
    comp = msi_get_loaded_component( package, component );
    if (!comp)
        return ERROR_SUCCESS;

    comp->Action = msi_get_component_action( package, comp );
    if (comp->Action != INSTALLSTATE_LOCAL)
    {
        TRACE("component not scheduled for installation %s\n", debugstr_w(component));
        return ERROR_SUCCESS;
    }

    desc = MSI_RecordGetString(rec, 3);
    driver = MSI_RecordGetString(rec, 4);
    registration = MSI_RecordGetInteger(rec, 5);

    if (registration == msidbODBCDataSourceRegistrationPerMachine) request = ODBC_ADD_SYS_DSN;
    else if (registration == msidbODBCDataSourceRegistrationPerUser) request = ODBC_ADD_DSN;

    len = lstrlenW(L"DSN=%s") + lstrlenW(desc) + 2; /* \0\0 */
    attrs = malloc(len * sizeof(WCHAR));
    if (!attrs)
        return ERROR_OUTOFMEMORY;

    len = swprintf(attrs, len, L"DSN=%s", desc);
    attrs[len + 1] = 0;

    if (!SQLConfigDataSourceW(NULL, request, driver, attrs))
        WARN("Failed to install SQL data source!\n");

    uirow = MSI_CreateRecord( 5 );
    MSI_RecordSetStringW( uirow, 1, desc );
    MSI_RecordSetStringW( uirow, 2, MSI_RecordGetString(rec, 2) );
    MSI_RecordSetInteger( uirow, 3, request );
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
    msiobj_release( &uirow->hdr );

    free(attrs);

    return r;
}

static UINT ACTION_InstallODBC( MSIPACKAGE *package )
{
    MSIQUERY *view;
    UINT rc;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action(package, SCRIPT_INSTALL, L"InstallODBC");

    rc = MSI_DatabaseOpenViewW(package->db, L"SELECT * FROM `ODBCDriver`", &view);
    if (rc == ERROR_SUCCESS)
    {
        rc = MSI_IterateRecords(view, NULL, ITERATE_InstallODBCDriver, package);
        msiobj_release(&view->hdr);
        if (rc != ERROR_SUCCESS)
            return rc;
    }
    rc = MSI_DatabaseOpenViewW(package->db, L"SELECT * FROM `ODBCTranslator`", &view);
    if (rc == ERROR_SUCCESS)
    {
        rc = MSI_IterateRecords(view, NULL, ITERATE_InstallODBCTranslator, package);
        msiobj_release(&view->hdr);
        if (rc != ERROR_SUCCESS)
            return rc;
    }
    rc = MSI_DatabaseOpenViewW(package->db, L"SELECT * FROM `ODBCDataSource`", &view);
    if (rc == ERROR_SUCCESS)
    {
        rc = MSI_IterateRecords(view, NULL, ITERATE_InstallODBCDataSource, package);
        msiobj_release(&view->hdr);
        if (rc != ERROR_SUCCESS)
            return rc;
    }
    return ERROR_SUCCESS;
}

static UINT ITERATE_RemoveODBCDriver( MSIRECORD *rec, LPVOID param )
{
    MSIPACKAGE *package = param;
    MSICOMPONENT *comp;
    MSIRECORD *uirow;
    DWORD usage;
    LPCWSTR desc, component;

    component = MSI_RecordGetString( rec, 2 );
    comp = msi_get_loaded_component( package, component );
    if (!comp)
        return ERROR_SUCCESS;

    comp->Action = msi_get_component_action( package, comp );
    if (comp->Action != INSTALLSTATE_ABSENT)
    {
        TRACE("component not scheduled for removal %s\n", debugstr_w(component));
        return ERROR_SUCCESS;
    }

    desc = MSI_RecordGetString( rec, 3 );
    if (!SQLRemoveDriverW( desc, FALSE, &usage ))
    {
        WARN("Failed to remove ODBC driver\n");
    }
    else if (!usage)
    {
        FIXME("Usage count reached 0\n");
    }

    uirow = MSI_CreateRecord( 2 );
    MSI_RecordSetStringW( uirow, 1, desc );
    MSI_RecordSetStringW( uirow, 2, MSI_RecordGetString(rec, 2) );
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
    msiobj_release( &uirow->hdr );

    return ERROR_SUCCESS;
}

static UINT ITERATE_RemoveODBCTranslator( MSIRECORD *rec, LPVOID param )
{
    MSIPACKAGE *package = param;
    MSICOMPONENT *comp;
    MSIRECORD *uirow;
    DWORD usage;
    LPCWSTR desc, component;

    component = MSI_RecordGetString( rec, 2 );
    comp = msi_get_loaded_component( package, component );
    if (!comp)
        return ERROR_SUCCESS;

    comp->Action = msi_get_component_action( package, comp );
    if (comp->Action != INSTALLSTATE_ABSENT)
    {
        TRACE("component not scheduled for removal %s\n", debugstr_w(component));
        return ERROR_SUCCESS;
    }

    desc = MSI_RecordGetString( rec, 3 );
    if (!SQLRemoveTranslatorW( desc, &usage ))
    {
        WARN("Failed to remove ODBC translator\n");
    }
    else if (!usage)
    {
        FIXME("Usage count reached 0\n");
    }

    uirow = MSI_CreateRecord( 2 );
    MSI_RecordSetStringW( uirow, 1, desc );
    MSI_RecordSetStringW( uirow, 2, MSI_RecordGetString(rec, 2) );
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
    msiobj_release( &uirow->hdr );

    return ERROR_SUCCESS;
}

static UINT ITERATE_RemoveODBCDataSource( MSIRECORD *rec, LPVOID param )
{
    MSIPACKAGE *package = param;
    MSICOMPONENT *comp;
    MSIRECORD *uirow;
    LPWSTR attrs;
    LPCWSTR desc, driver, component;
    WORD request = ODBC_REMOVE_SYS_DSN;
    INT registration;
    DWORD len;

    component = MSI_RecordGetString( rec, 2 );
    comp = msi_get_loaded_component( package, component );
    if (!comp)
        return ERROR_SUCCESS;

    comp->Action = msi_get_component_action( package, comp );
    if (comp->Action != INSTALLSTATE_ABSENT)
    {
        TRACE("component not scheduled for removal %s\n", debugstr_w(component));
        return ERROR_SUCCESS;
    }

    desc = MSI_RecordGetString( rec, 3 );
    driver = MSI_RecordGetString( rec, 4 );
    registration = MSI_RecordGetInteger( rec, 5 );

    if (registration == msidbODBCDataSourceRegistrationPerMachine) request = ODBC_REMOVE_SYS_DSN;
    else if (registration == msidbODBCDataSourceRegistrationPerUser) request = ODBC_REMOVE_DSN;

    len = lstrlenW( L"DSN=%s" ) + lstrlenW( desc ) + 2; /* \0\0 */
    attrs = malloc( len * sizeof(WCHAR) );
    if (!attrs)
        return ERROR_OUTOFMEMORY;

    FIXME("Use ODBCSourceAttribute table\n");

    len = swprintf( attrs, len, L"DSN=%s", desc );
    attrs[len + 1] = 0;

    if (!SQLConfigDataSourceW( NULL, request, driver, attrs ))
    {
        WARN("Failed to remove ODBC data source\n");
    }
    free( attrs );

    uirow = MSI_CreateRecord( 3 );
    MSI_RecordSetStringW( uirow, 1, desc );
    MSI_RecordSetStringW( uirow, 2, MSI_RecordGetString(rec, 2) );
    MSI_RecordSetInteger( uirow, 3, request );
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
    msiobj_release( &uirow->hdr );

    return ERROR_SUCCESS;
}

static UINT ACTION_RemoveODBC( MSIPACKAGE *package )
{
    MSIQUERY *view;
    UINT rc;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action(package, SCRIPT_INSTALL, L"RemoveODBC");

    rc = MSI_DatabaseOpenViewW( package->db, L"SELECT * FROM `ODBCDriver`", &view );
    if (rc == ERROR_SUCCESS)
    {
        rc = MSI_IterateRecords( view, NULL, ITERATE_RemoveODBCDriver, package );
        msiobj_release( &view->hdr );
        if (rc != ERROR_SUCCESS)
            return rc;
    }
    rc = MSI_DatabaseOpenViewW( package->db, L"SELECT * FROM `ODBCTranslator`", &view );
    if (rc == ERROR_SUCCESS)
    {
        rc = MSI_IterateRecords( view, NULL, ITERATE_RemoveODBCTranslator, package );
        msiobj_release( &view->hdr );
        if (rc != ERROR_SUCCESS)
            return rc;
    }
    rc = MSI_DatabaseOpenViewW( package->db, L"SELECT * FROM `ODBCDataSource`", &view );
    if (rc == ERROR_SUCCESS)
    {
        rc = MSI_IterateRecords( view, NULL, ITERATE_RemoveODBCDataSource, package );
        msiobj_release( &view->hdr );
        if (rc != ERROR_SUCCESS)
            return rc;
    }
    return ERROR_SUCCESS;
}

#define ENV_ACT_SETALWAYS   0x1
#define ENV_ACT_SETABSENT   0x2
#define ENV_ACT_REMOVE      0x4
#define ENV_ACT_REMOVEMATCH 0x8

#define ENV_MOD_MACHINE     0x20000000
#define ENV_MOD_APPEND      0x40000000
#define ENV_MOD_PREFIX      0x80000000
#define ENV_MOD_MASK        0xC0000000

#define check_flag_combo(x, y) ((x) & ~(y)) == (y)

static UINT env_parse_flags( LPCWSTR *name, LPCWSTR *value, DWORD *flags )
{
    const WCHAR *cptr = *name;

    *flags = 0;
    while (*cptr)
    {
        if (*cptr == '=')
            *flags |= ENV_ACT_SETALWAYS;
        else if (*cptr == '+')
            *flags |= ENV_ACT_SETABSENT;
        else if (*cptr == '-')
            *flags |= ENV_ACT_REMOVE;
        else if (*cptr == '!')
            *flags |= ENV_ACT_REMOVEMATCH;
        else if (*cptr == '*')
            *flags |= ENV_MOD_MACHINE | ENV_ACT_REMOVE;
        else
            break;

        cptr++;
        (*name)++;
    }

    if (!*cptr)
    {
        ERR("Missing environment variable\n");
        return ERROR_FUNCTION_FAILED;
    }

    if (*value)
    {
        LPCWSTR ptr = *value;
        if (!wcsncmp(ptr, L"[~]", 3))
        {
            if (ptr[3] == ';')
            {
                *flags |= ENV_MOD_APPEND;
                *value += 3;
            }
            else
            {
                *value = NULL;
            }
        }
        else if (lstrlenW(*value) >= 3)
        {
            ptr += lstrlenW(ptr) - 3;
            if (!wcscmp( ptr, L"[~]" ))
            {
                if ((ptr-1) > *value && *(ptr-1) == ';')
                {
                    *flags |= ENV_MOD_PREFIX;
                    /* the "[~]" will be removed by deformat_string */;
                }
                else
                {
                    *value = NULL;
                }
            }
        }
    }

    if (check_flag_combo(*flags, ENV_ACT_SETALWAYS | ENV_ACT_SETABSENT) ||
        check_flag_combo(*flags, ENV_ACT_REMOVEMATCH | ENV_ACT_SETABSENT) ||
        check_flag_combo(*flags, ENV_ACT_REMOVEMATCH | ENV_ACT_SETALWAYS) ||
        check_flag_combo(*flags, ENV_ACT_SETABSENT | ENV_MOD_MASK))
    {
        ERR( "invalid flags: %#lx\n", *flags );
        return ERROR_FUNCTION_FAILED;
    }

    if (!*flags)
        *flags = ENV_ACT_SETALWAYS | ENV_ACT_REMOVE;

    return ERROR_SUCCESS;
}

static UINT open_env_key( DWORD flags, HKEY *key )
{
    const WCHAR *env;
    HKEY root;
    LONG res;

    if (flags & ENV_MOD_MACHINE)
    {
        env = L"System\\CurrentControlSet\\Control\\Session Manager\\Environment";
        root = HKEY_LOCAL_MACHINE;
    }
    else
    {
        env = L"Environment";
        root = HKEY_CURRENT_USER;
    }

    res = RegOpenKeyExW( root, env, 0, KEY_ALL_ACCESS, key );
    if (res != ERROR_SUCCESS)
    {
        WARN( "failed to open key %s (%ld)\n", debugstr_w(env), res );
        return ERROR_FUNCTION_FAILED;
    }

    return ERROR_SUCCESS;
}

static UINT ITERATE_WriteEnvironmentString( MSIRECORD *rec, LPVOID param )
{
    MSIPACKAGE *package = param;
    LPCWSTR name, value, component;
    WCHAR *data = NULL, *newval = NULL, *deformatted = NULL, *p, *q;
    DWORD flags, type, size, len, len_value = 0;
    UINT res;
    HKEY env = NULL;
    MSICOMPONENT *comp;
    MSIRECORD *uirow;
    int action = 0, found = 0;

    component = MSI_RecordGetString(rec, 4);
    comp = msi_get_loaded_component(package, component);
    if (!comp)
        return ERROR_SUCCESS;

    comp->Action = msi_get_component_action( package, comp );
    if (comp->Action != INSTALLSTATE_LOCAL)
    {
        TRACE("component not scheduled for installation %s\n", debugstr_w(component));
        return ERROR_SUCCESS;
    }
    name = MSI_RecordGetString(rec, 2);
    value = MSI_RecordGetString(rec, 3);

    TRACE("name %s value %s\n", debugstr_w(name), debugstr_w(value));

    res = env_parse_flags(&name, &value, &flags);
    if (res != ERROR_SUCCESS || !value)
       goto done;

    if (value)
    {
        DWORD len = deformat_string( package, value, &deformatted );
        if (!deformatted)
        {
            res = ERROR_OUTOFMEMORY;
            goto done;
        }

        if (len)
        {
            value = deformatted;
            if (flags & ENV_MOD_PREFIX)
            {
                p = wcsrchr( value, ';' );
                len_value = p - value;
            }
            else if (flags & ENV_MOD_APPEND)
            {
                value = wcschr( value, ';' ) + 1;
                len_value = lstrlenW( value );
            }
            else len_value = lstrlenW( value );
        }
        else
        {
            value = NULL;
        }
    }

    res = open_env_key( flags, &env );
    if (res != ERROR_SUCCESS)
        goto done;

    if (flags & ENV_MOD_MACHINE)
        action |= 0x20000000;

    size = 0;
    type = REG_SZ;
    res = RegQueryValueExW(env, name, NULL, &type, NULL, &size);
    if ((res != ERROR_SUCCESS && res != ERROR_FILE_NOT_FOUND) ||
        (res == ERROR_SUCCESS && type != REG_SZ && type != REG_EXPAND_SZ))
        goto done;

    if ((res == ERROR_FILE_NOT_FOUND || !(flags & ENV_MOD_MASK)))
    {
        action = 0x2;

        /* Nothing to do. */
        if (!value)
        {
            res = ERROR_SUCCESS;
            goto done;
        }
        size = (lstrlenW(value) + 1) * sizeof(WCHAR);
        newval = wcsdup(value);
        if (!newval)
        {
            res = ERROR_OUTOFMEMORY;
            goto done;
        }
    }
    else
    {
        action = 0x1;

        /* Contrary to MSDN, +-variable to [~];path works */
        if (flags & ENV_ACT_SETABSENT && !(flags & ENV_MOD_MASK))
        {
            res = ERROR_SUCCESS;
            goto done;
        }

        if (!(p = q = data = malloc( size )))
        {
            free(deformatted);
            RegCloseKey(env);
            return ERROR_OUTOFMEMORY;
        }

        res = RegQueryValueExW( env, name, NULL, &type, (BYTE *)data, &size );
        if (res != ERROR_SUCCESS)
            goto done;

        if (flags & ENV_ACT_REMOVEMATCH && (!value || !wcscmp( data, value )))
        {
            action = 0x4;
            res = RegDeleteValueW(env, name);
            if (res != ERROR_SUCCESS)
                WARN("Failed to remove value %s (%d)\n", debugstr_w(name), res);
            goto done;
        }

        for (;;)
        {
            while (*q && *q != ';') q++;
            len = q - p;
            if (value && len == len_value && !memcmp( value, p, len * sizeof(WCHAR) ) &&
                (!p[len] || p[len] == ';'))
            {
                found = 1;
                break;
            }
            if (!*q) break;
            p = ++q;
        }

        if (found)
        {
            TRACE("string already set\n");
            goto done;
        }

        size = (len_value + 1 + lstrlenW( data ) + 1) * sizeof(WCHAR);
        if (!(p = newval = malloc( size )))
        {
            res = ERROR_OUTOFMEMORY;
            goto done;
        }

        if (flags & ENV_MOD_PREFIX)
        {
            memcpy( newval, value, len_value * sizeof(WCHAR) );
            newval[len_value] = ';';
            p = newval + len_value + 1;
            action |= 0x80000000;
        }

        lstrcpyW( p, data );

        if (flags & ENV_MOD_APPEND)
        {
            p += lstrlenW( data );
            *p++ = ';';
            memcpy( p, value, (len_value + 1) * sizeof(WCHAR) );
            action |= 0x40000000;
        }
    }
    TRACE("setting %s to %s\n", debugstr_w(name), debugstr_w(newval));
    res = RegSetValueExW( env, name, 0, type, (BYTE *)newval, size );
    if (res)
    {
        WARN("Failed to set %s to %s (%d)\n",  debugstr_w(name), debugstr_w(newval), res);
    }

done:
    uirow = MSI_CreateRecord( 3 );
    MSI_RecordSetStringW( uirow, 1, name );
    MSI_RecordSetStringW( uirow, 2, newval );
    MSI_RecordSetInteger( uirow, 3, action );
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
    msiobj_release( &uirow->hdr );

    if (env) RegCloseKey(env);
    free(deformatted);
    free(data);
    free(newval);
    return res;
}

static UINT ACTION_WriteEnvironmentStrings( MSIPACKAGE *package )
{
    MSIQUERY *view;
    UINT rc;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action(package, SCRIPT_INSTALL, L"WriteEnvironmentStrings");

    rc = MSI_DatabaseOpenViewW(package->db, L"SELECT * FROM `Environment`", &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords(view, NULL, ITERATE_WriteEnvironmentString, package);
    msiobj_release(&view->hdr);
    return rc;
}

static UINT ITERATE_RemoveEnvironmentString( MSIRECORD *rec, LPVOID param )
{
    MSIPACKAGE *package = param;
    LPCWSTR name, value, component;
    WCHAR *p, *q, *deformatted = NULL, *new_value = NULL;
    DWORD flags, type, size, len, len_value = 0, len_new_value;
    HKEY env = NULL;
    MSICOMPONENT *comp;
    MSIRECORD *uirow;
    int action = 0;
    LONG res;
    UINT r;

    component = MSI_RecordGetString( rec, 4 );
    comp = msi_get_loaded_component( package, component );
    if (!comp)
        return ERROR_SUCCESS;

    comp->Action = msi_get_component_action( package, comp );
    if (comp->Action != INSTALLSTATE_ABSENT)
    {
        TRACE("component not scheduled for removal %s\n", debugstr_w(component));
        return ERROR_SUCCESS;
    }
    name = MSI_RecordGetString( rec, 2 );
    value = MSI_RecordGetString( rec, 3 );

    TRACE("name %s value %s\n", debugstr_w(name), debugstr_w(value));

    r = env_parse_flags( &name, &value, &flags );
    if (r != ERROR_SUCCESS)
       return r;

    if (!(flags & ENV_ACT_REMOVE))
    {
        TRACE("Environment variable %s not marked for removal\n", debugstr_w(name));
        return ERROR_SUCCESS;
    }

    if (value)
    {
        DWORD len = deformat_string( package, value, &deformatted );
        if (!deformatted)
        {
            res = ERROR_OUTOFMEMORY;
            goto done;
        }

        if (len)
        {
            value = deformatted;
            if (flags & ENV_MOD_PREFIX)
            {
                p = wcsrchr( value, ';' );
                len_value = p - value;
            }
            else if (flags & ENV_MOD_APPEND)
            {
                value = wcschr( value, ';' ) + 1;
                len_value = lstrlenW( value );
            }
            else len_value = lstrlenW( value );
        }
        else
        {
            value = NULL;
        }
    }

    r = open_env_key( flags, &env );
    if (r != ERROR_SUCCESS)
    {
        r = ERROR_SUCCESS;
        goto done;
    }

    if (flags & ENV_MOD_MACHINE)
        action |= 0x20000000;

    size = 0;
    type = REG_SZ;
    res = RegQueryValueExW( env, name, NULL, &type, NULL, &size );
    if (res != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ))
        goto done;

    if (!(new_value = malloc( size ))) goto done;

    res = RegQueryValueExW( env, name, NULL, &type, (BYTE *)new_value, &size );
    if (res != ERROR_SUCCESS)
        goto done;

    len_new_value = size / sizeof(WCHAR) - 1;
    p = q = new_value;
    for (;;)
    {
        while (*q && *q != ';') q++;
        len = q - p;
        if (value && len == len_value && !memcmp( value, p, len * sizeof(WCHAR) ))
        {
            if (*q == ';') q++;
            memmove( p, q, (len_new_value - (q - new_value) + 1) * sizeof(WCHAR) );
            break;
        }
        if (!*q) break;
        p = ++q;
    }

    if (!new_value[0] || !value)
    {
        TRACE("removing %s\n", debugstr_w(name));
        res = RegDeleteValueW( env, name );
        if (res != ERROR_SUCCESS)
            WARN( "failed to delete value %s (%ld)\n", debugstr_w(name), res );
    }
    else
    {
        TRACE("setting %s to %s\n", debugstr_w(name), debugstr_w(new_value));
        size = (lstrlenW( new_value ) + 1) * sizeof(WCHAR);
        res = RegSetValueExW( env, name, 0, type, (BYTE *)new_value, size );
        if (res != ERROR_SUCCESS)
            WARN( "failed to set %s to %s (%ld)\n", debugstr_w(name), debugstr_w(new_value), res );
    }

done:
    uirow = MSI_CreateRecord( 3 );
    MSI_RecordSetStringW( uirow, 1, name );
    MSI_RecordSetStringW( uirow, 2, value );
    MSI_RecordSetInteger( uirow, 3, action );
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
    msiobj_release( &uirow->hdr );

    if (env) RegCloseKey( env );
    free( deformatted );
    free( new_value );
    return r;
}

static UINT ACTION_RemoveEnvironmentStrings( MSIPACKAGE *package )
{
    MSIQUERY *view;
    UINT rc;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action(package, SCRIPT_INSTALL, L"RemoveEnvironmentStrings");

    rc = MSI_DatabaseOpenViewW( package->db, L"SELECT * FROM `Environment`", &view );
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords( view, NULL, ITERATE_RemoveEnvironmentString, package );
    msiobj_release( &view->hdr );
    return rc;
}

UINT msi_validate_product_id( MSIPACKAGE *package )
{
    LPWSTR key, template, id;
    UINT r = ERROR_SUCCESS;

    id = msi_dup_property( package->db, L"ProductID" );
    if (id)
    {
        free( id );
        return ERROR_SUCCESS;
    }
    template = msi_dup_property( package->db, L"PIDTemplate" );
    key = msi_dup_property( package->db, L"PIDKEY" );
    if (key && template)
    {
        FIXME( "partial stub: template %s key %s\n", debugstr_w(template), debugstr_w(key) );
#ifdef __REACTOS__
        WARN("Product key validation HACK, see CORE-14710\n");
#else
        r = msi_set_property( package->db, L"ProductID", key, -1 );
#endif
    }
    free( template );
    free( key );
    return r;
}

static UINT ACTION_ValidateProductID( MSIPACKAGE *package )
{
    return msi_validate_product_id( package );
}

static UINT ACTION_ScheduleReboot( MSIPACKAGE *package )
{
    TRACE("\n");
    package->need_reboot_at_end = 1;
    return ERROR_SUCCESS;
}

static UINT ACTION_AllocateRegistrySpace( MSIPACKAGE *package )
{
    MSIRECORD *uirow;
    int space = msi_get_property_int( package->db, L"AVAILABLEFREEREG", 0 );

    TRACE("%p %d kilobytes\n", package, space);

    uirow = MSI_CreateRecord( 1 );
    MSI_RecordSetInteger( uirow, 1, space );
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
    msiobj_release( &uirow->hdr );

    return ERROR_SUCCESS;
}

static UINT ACTION_DisableRollback( MSIPACKAGE *package )
{
    TRACE("%p\n", package);

    msi_set_property( package->db, L"RollbackDisabled", L"1", -1 );
    return ERROR_SUCCESS;
}

static UINT ACTION_InstallAdminPackage( MSIPACKAGE *package )
{
    FIXME("%p\n", package);
    return ERROR_SUCCESS;
}

static UINT ACTION_SetODBCFolders( MSIPACKAGE *package )
{
    MSIQUERY *view;
    UINT r;
    DWORD count;

    r = MSI_DatabaseOpenViewW( package->db, L"SELECT * FROM `ODBCDriver`", &view );
    if (r == ERROR_SUCCESS)
    {
        count = 0;
        r = MSI_IterateRecords( view, &count, NULL, package );
        msiobj_release( &view->hdr );
        if (r != ERROR_SUCCESS)
            return r;
        if (count) FIXME( "ignored %lu rows in ODBCDriver table\n", count );
    }
    r = MSI_DatabaseOpenViewW( package->db, L"SELECT * FROM `ODBCTranslator`", &view );
    if (r == ERROR_SUCCESS)
    {
        count = 0;
        r = MSI_IterateRecords( view, &count, NULL, package );
        msiobj_release( &view->hdr );
        if (r != ERROR_SUCCESS)
            return r;
        if (count) FIXME( "ignored %lu rows in ODBCTranslator table\n", count );
    }
    return ERROR_SUCCESS;
}

static UINT ITERATE_RemoveExistingProducts( MSIRECORD *rec, LPVOID param )
{
    MSIPACKAGE *package = param;
    const WCHAR *property = MSI_RecordGetString( rec, 7 );
    int attrs = MSI_RecordGetInteger( rec, 5 );
    UINT len = ARRAY_SIZE( L"msiexec /qn /i %s REMOVE=%s" );
    WCHAR *product, *features, *cmd;
    STARTUPINFOW si;
    PROCESS_INFORMATION info;
    BOOL ret;

    if (attrs & msidbUpgradeAttributesOnlyDetect) return ERROR_SUCCESS;
    if (!(product = msi_dup_property( package->db, property ))) return ERROR_SUCCESS;

    deformat_string( package, MSI_RecordGetString( rec, 6 ), &features );

    len += lstrlenW( product );
    if (features)
        len += lstrlenW( features );
    else
        len += ARRAY_SIZE( L"ALL" );

    if (!(cmd = malloc( len * sizeof(WCHAR) )))
    {
        free( product );
        free( features );
        return ERROR_OUTOFMEMORY;
    }
    swprintf( cmd, len, L"msiexec /qn /i %s REMOVE=%s", product, features ? features : L"ALL" );
    free( product );
    free( features );

    memset( &si, 0, sizeof(STARTUPINFOW) );
    ret = CreateProcessW( NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &info );
    free( cmd );
    if (!ret) return GetLastError();
    CloseHandle( info.hThread );

    WaitForSingleObject( info.hProcess, INFINITE );
    CloseHandle( info.hProcess );
    return ERROR_SUCCESS;
}

static UINT ACTION_RemoveExistingProducts( MSIPACKAGE *package )
{
    MSIQUERY *view;
    UINT r;

    r = MSI_DatabaseOpenViewW( package->db, L"SELECT * FROM `Upgrade`", &view );
    if (r == ERROR_SUCCESS)
    {
        r = MSI_IterateRecords( view, NULL, ITERATE_RemoveExistingProducts, package );
        msiobj_release( &view->hdr );
        if (r != ERROR_SUCCESS)
            return r;
    }
    return ERROR_SUCCESS;
}

static UINT ITERATE_MigrateFeatureStates( MSIRECORD *rec, LPVOID param )
{
    MSIPACKAGE *package = param;
    int attributes = MSI_RecordGetInteger( rec, 5 );

    if (attributes & msidbUpgradeAttributesMigrateFeatures)
    {
        const WCHAR *upgrade_code = MSI_RecordGetString( rec, 1 );
        const WCHAR *version_min = MSI_RecordGetString( rec, 2 );
        const WCHAR *version_max = MSI_RecordGetString( rec, 3 );
        const WCHAR *language = MSI_RecordGetString( rec, 4 );
        HKEY hkey;
        UINT r;

        if (package->Context == MSIINSTALLCONTEXT_MACHINE)
        {
            r = MSIREG_OpenClassesUpgradeCodesKey( upgrade_code, &hkey, FALSE );
            if (r != ERROR_SUCCESS)
                return ERROR_SUCCESS;
        }
        else
        {
            r = MSIREG_OpenUserUpgradeCodesKey( upgrade_code, &hkey, FALSE );
            if (r != ERROR_SUCCESS)
                return ERROR_SUCCESS;
        }
        RegCloseKey( hkey );

        FIXME("migrate feature states from %s version min %s version max %s language %s\n",
              debugstr_w(upgrade_code), debugstr_w(version_min),
              debugstr_w(version_max), debugstr_w(language));
    }
    return ERROR_SUCCESS;
}

static UINT ACTION_MigrateFeatureStates( MSIPACKAGE *package )
{
    MSIQUERY *view;
    UINT r;

    if (msi_get_property_int( package->db, L"Installed", 0 ))
    {
        TRACE("product is installed, skipping action\n");
        return ERROR_SUCCESS;
    }
    if (msi_get_property_int( package->db, L"Preselected", 0 ))
    {
        TRACE("Preselected property is set, not migrating feature states\n");
        return ERROR_SUCCESS;
    }
    r = MSI_DatabaseOpenViewW( package->db, L"SELECT * FROM `Upgrade`", &view );
    if (r == ERROR_SUCCESS)
    {
        r = MSI_IterateRecords( view, NULL, ITERATE_MigrateFeatureStates, package );
        msiobj_release( &view->hdr );
        if (r != ERROR_SUCCESS)
            return r;
    }
    return ERROR_SUCCESS;
}

static void bind_image( MSIPACKAGE *package, const char *filename, const char *path )
{
    msi_disable_fs_redirection( package );
    if (!BindImage( filename, path, NULL )) WARN( "failed to bind image %lu\n", GetLastError() );
    msi_revert_fs_redirection( package );
}

static UINT ITERATE_BindImage( MSIRECORD *rec, LPVOID param )
{
    UINT i;
    MSIFILE *file;
    MSIPACKAGE *package = param;
    const WCHAR *key = MSI_RecordGetString( rec, 1 );
    const WCHAR *paths = MSI_RecordGetString( rec, 2 );
    char *filenameA, *pathA;
    WCHAR *pathW, **path_list;

    if (!(file = msi_get_loaded_file( package, key )))
    {
        WARN("file %s not found\n", debugstr_w(key));
        return ERROR_SUCCESS;
    }
    if (!(filenameA = strdupWtoA( file->TargetPath ))) return ERROR_SUCCESS;

    path_list = msi_split_string( paths, ';' );
    if (!path_list) bind_image( package, filenameA, NULL );
    else
    {
        for (i = 0; path_list[i] && path_list[i][0]; i++)
        {
            deformat_string( package, path_list[i], &pathW );
            if ((pathA = strdupWtoA( pathW )))
            {
                bind_image( package, filenameA, pathA );
                free( pathA );
            }
            free( pathW );
        }
    }
    free( path_list );
    free( filenameA );

    return ERROR_SUCCESS;
}

static UINT ACTION_BindImage( MSIPACKAGE *package )
{
    MSIQUERY *view;
    UINT r;

    r = MSI_DatabaseOpenViewW( package->db, L"SELECT * FROM `BindImage`", &view );
    if (r == ERROR_SUCCESS)
    {
        MSI_IterateRecords( view, NULL, ITERATE_BindImage, package );
        msiobj_release( &view->hdr );
    }
    return ERROR_SUCCESS;
}

static UINT unimplemented_action_stub( MSIPACKAGE *package, LPCSTR action, LPCWSTR table )
{
    MSIQUERY *view;
    DWORD count = 0;
    UINT r;

    r = MSI_OpenQuery( package->db, &view, L"SELECT * FROM `%s`", table );
    if (r == ERROR_SUCCESS)
    {
        r = MSI_IterateRecords(view, &count, NULL, package);
        msiobj_release(&view->hdr);
        if (r != ERROR_SUCCESS)
            return r;
    }
    if (count) FIXME( "%s: ignored %lu rows from %s\n", action, count, debugstr_w(table) );
    return ERROR_SUCCESS;
}

static UINT ACTION_IsolateComponents( MSIPACKAGE *package )
{
    return unimplemented_action_stub( package, "IsolateComponents", L"IsolateComponent" );
}

static UINT ACTION_RMCCPSearch( MSIPACKAGE *package )
{
    return unimplemented_action_stub( package, "RMCCPSearch", L"CCPSearch" );
}

static UINT ACTION_RegisterComPlus( MSIPACKAGE *package )
{
    return unimplemented_action_stub( package, "RegisterComPlus", L"Complus" );
}

static UINT ACTION_UnregisterComPlus( MSIPACKAGE *package )
{
    return unimplemented_action_stub( package, "UnregisterComPlus", L"Complus" );
}

static UINT ACTION_InstallSFPCatalogFile( MSIPACKAGE *package )
{
    return unimplemented_action_stub( package, "InstallSFPCatalogFile", L"SFPCatalog" );
}

static const struct
{
    const WCHAR *action;
    const UINT description;
    const UINT template;
    UINT (*handler)(MSIPACKAGE *);
    const WCHAR *action_rollback;
}
StandardActions[] =
{
    { L"AllocateRegistrySpace", IDS_DESC_ALLOCATEREGISTRYSPACE, IDS_TEMP_ALLOCATEREGISTRYSPACE, ACTION_AllocateRegistrySpace, NULL },
    { L"AppSearch", IDS_DESC_APPSEARCH, IDS_TEMP_APPSEARCH, ACTION_AppSearch, NULL },
    { L"BindImage", IDS_DESC_BINDIMAGE, IDS_TEMP_BINDIMAGE, ACTION_BindImage, NULL },
    { L"CCPSearch", IDS_DESC_CCPSEARCH, 0, ACTION_CCPSearch, NULL },
    { L"CostFinalize", IDS_DESC_COSTFINALIZE, 0, ACTION_CostFinalize, NULL },
    { L"CostInitialize", IDS_DESC_COSTINITIALIZE, 0, ACTION_CostInitialize, NULL },
    { L"CreateFolders", IDS_DESC_CREATEFOLDERS, IDS_TEMP_CREATEFOLDERS, ACTION_CreateFolders, L"RemoveFolders" },
    { L"CreateShortcuts", IDS_DESC_CREATESHORTCUTS, IDS_TEMP_CREATESHORTCUTS, ACTION_CreateShortcuts, L"RemoveShortcuts" },
    { L"DeleteServices", IDS_DESC_DELETESERVICES, IDS_TEMP_DELETESERVICES, ACTION_DeleteServices, L"InstallServices" },
    { L"DisableRollback", 0, 0, ACTION_DisableRollback, NULL },
    { L"DuplicateFiles", IDS_DESC_DUPLICATEFILES, IDS_TEMP_DUPLICATEFILES, ACTION_DuplicateFiles, L"RemoveDuplicateFiles" },
    { L"ExecuteAction", 0, 0, ACTION_ExecuteAction, NULL },
    { L"FileCost", IDS_DESC_FILECOST, 0, ACTION_FileCost, NULL },
    { L"FindRelatedProducts", IDS_DESC_FINDRELATEDPRODUCTS, IDS_TEMP_FINDRELATEDPRODUCTS, ACTION_FindRelatedProducts, NULL },
    { L"ForceReboot", 0, 0, ACTION_ForceReboot, NULL },
    { L"InstallAdminPackage", IDS_DESC_INSTALLADMINPACKAGE, IDS_TEMP_INSTALLADMINPACKAGE, ACTION_InstallAdminPackage, NULL },
    { L"InstallExecute", 0, 0, ACTION_InstallExecute, NULL },
    { L"InstallExecuteAgain", 0, 0, ACTION_InstallExecute, NULL },
    { L"InstallFiles", IDS_DESC_INSTALLFILES, IDS_TEMP_INSTALLFILES, ACTION_InstallFiles, L"RemoveFiles" },
    { L"InstallFinalize", 0, 0, ACTION_InstallFinalize, NULL },
    { L"InstallInitialize", 0, 0, ACTION_InstallInitialize, NULL },
    { L"InstallODBC", IDS_DESC_INSTALLODBC, 0, ACTION_InstallODBC, L"RemoveODBC" },
    { L"InstallServices", IDS_DESC_INSTALLSERVICES, IDS_TEMP_INSTALLSERVICES, ACTION_InstallServices, L"DeleteServices" },
    { L"InstallSFPCatalogFile", IDS_DESC_INSTALLSFPCATALOGFILE, IDS_TEMP_INSTALLSFPCATALOGFILE, ACTION_InstallSFPCatalogFile, NULL },
    { L"InstallValidate", IDS_DESC_INSTALLVALIDATE, 0, ACTION_InstallValidate, NULL },
    { L"IsolateComponents", 0, 0, ACTION_IsolateComponents, NULL },
    { L"LaunchConditions", IDS_DESC_LAUNCHCONDITIONS, 0, ACTION_LaunchConditions, NULL },
    { L"MigrateFeutureStates", IDS_DESC_MIGRATEFEATURESTATES, IDS_TEMP_MIGRATEFEATURESTATES, ACTION_MigrateFeatureStates, NULL },
    { L"MoveFiles", IDS_DESC_MOVEFILES, IDS_TEMP_MOVEFILES, ACTION_MoveFiles, NULL },
    { L"MsiPublishAssemblies", IDS_DESC_MSIPUBLISHASSEMBLIES, IDS_TEMP_MSIPUBLISHASSEMBLIES, ACTION_MsiPublishAssemblies, L"MsiUnpublishAssemblies" },
    { L"MsiUnpublishAssemblies", IDS_DESC_MSIUNPUBLISHASSEMBLIES, IDS_TEMP_MSIUNPUBLISHASSEMBLIES, ACTION_MsiUnpublishAssemblies, L"MsiPublishAssemblies" },
    { L"PatchFiles", IDS_DESC_PATCHFILES, IDS_TEMP_PATCHFILES, ACTION_PatchFiles, NULL },
    { L"ProcessComponents", IDS_DESC_PROCESSCOMPONENTS, 0, ACTION_ProcessComponents, L"ProcessComponents" },
    { L"PublishComponents", IDS_DESC_PUBLISHCOMPONENTS, IDS_TEMP_PUBLISHCOMPONENTS, ACTION_PublishComponents, L"UnpublishComponents" },
    { L"PublishFeatures", IDS_DESC_PUBLISHFEATURES, IDS_TEMP_PUBLISHFEATURES, ACTION_PublishFeatures, L"UnpublishFeatures" },
    { L"PublishProduct", IDS_DESC_PUBLISHPRODUCT, 0, ACTION_PublishProduct, L"UnpublishProduct" },
    { L"RegisterClassInfo", IDS_DESC_REGISTERCLASSINFO, IDS_TEMP_REGISTERCLASSINFO, ACTION_RegisterClassInfo, L"UnregisterClassInfo" },
    { L"RegisterComPlus", IDS_DESC_REGISTERCOMPLUS, IDS_TEMP_REGISTERCOMPLUS, ACTION_RegisterComPlus, L"UnregisterComPlus" },
    { L"RegisterExtensionInfo", IDS_DESC_REGISTEREXTENSIONINFO, 0, ACTION_RegisterExtensionInfo, L"UnregisterExtensionInfo" },
    { L"RegisterFonts", IDS_DESC_REGISTERFONTS, IDS_TEMP_REGISTERFONTS, ACTION_RegisterFonts, L"UnregisterFonts" },
    { L"RegisterMIMEInfo", IDS_DESC_REGISTERMIMEINFO, IDS_TEMP_REGISTERMIMEINFO, ACTION_RegisterMIMEInfo, L"UnregisterMIMEInfo" },
    { L"RegisterProduct", IDS_DESC_REGISTERPRODUCT, 0, ACTION_RegisterProduct, NULL },
    { L"RegisterProgIdInfo", IDS_DESC_REGISTERPROGIDINFO, IDS_TEMP_REGISTERPROGIDINFO, ACTION_RegisterProgIdInfo, L"UnregisterProgIdInfo" },
    { L"RegisterTypeLibraries", IDS_DESC_REGISTERTYPELIBRARIES, IDS_TEMP_REGISTERTYPELIBRARIES, ACTION_RegisterTypeLibraries, L"UnregisterTypeLibraries" },
    { L"RegisterUser", IDS_DESC_REGISTERUSER, 0, ACTION_RegisterUser, NULL },
    { L"RemoveDuplicateFiles", IDS_DESC_REMOVEDUPLICATEFILES, IDS_TEMP_REMOVEDUPLICATEFILES, ACTION_RemoveDuplicateFiles, L"DuplicateFiles" },
    { L"RemoveEnvironmentStrings", IDS_DESC_REMOVEENVIRONMENTSTRINGS, IDS_TEMP_REMOVEENVIRONMENTSTRINGS, ACTION_RemoveEnvironmentStrings, L"WriteEnvironmentStrings" },
    { L"RemoveExistingProducts", IDS_DESC_REMOVEEXISTINGPRODUCTS, IDS_TEMP_REMOVEEXISTINGPRODUCTS, ACTION_RemoveExistingProducts, NULL },
    { L"RemoveFiles", IDS_DESC_REMOVEFILES, IDS_TEMP_REMOVEFILES, ACTION_RemoveFiles, L"InstallFiles" },
    { L"RemoveFolders", IDS_DESC_REMOVEFOLDERS, IDS_TEMP_REMOVEFOLDERS, ACTION_RemoveFolders, L"CreateFolders" },
    { L"RemoveIniValues", IDS_DESC_REMOVEINIVALUES, IDS_TEMP_REMOVEINIVALUES, ACTION_RemoveIniValues, L"WriteIniValues" },
    { L"RemoveODBC", IDS_DESC_REMOVEODBC, 0, ACTION_RemoveODBC, L"InstallODBC" },
    { L"RemoveRegistryValues", IDS_DESC_REMOVEREGISTRYVALUES, IDS_TEMP_REMOVEREGISTRYVALUES, ACTION_RemoveRegistryValues, L"WriteRegistryValues" },
    { L"RemoveShortcuts", IDS_DESC_REMOVESHORTCUTS, IDS_TEMP_REMOVESHORTCUTS, ACTION_RemoveShortcuts, L"CreateShortcuts" },
    { L"ResolveSource", 0, 0, ACTION_ResolveSource, NULL },
    { L"RMCCPSearch", IDS_DESC_RMCCPSEARCH, 0, ACTION_RMCCPSearch, NULL },
    { L"ScheduleReboot", 0, 0, ACTION_ScheduleReboot, NULL },
    { L"SelfRegModules", IDS_DESC_SELFREGMODULES, IDS_TEMP_SELFREGMODULES, ACTION_SelfRegModules, L"SelfUnregModules" },
    { L"SelfUnregModules", IDS_DESC_SELFUNREGMODULES, IDS_TEMP_SELFUNREGMODULES, ACTION_SelfUnregModules, L"SelfRegModules" },
    { L"SetODBCFolders", IDS_DESC_SETODBCFOLDERS, 0, ACTION_SetODBCFolders, NULL },
    { L"StartServices", IDS_DESC_STARTSERVICES, IDS_TEMP_STARTSERVICES, ACTION_StartServices, L"StopServices" },
    { L"StopServices", IDS_DESC_STOPSERVICES, IDS_TEMP_STOPSERVICES, ACTION_StopServices, L"StartServices" },
    { L"UnpublishComponents", IDS_DESC_UNPUBLISHCOMPONENTS, IDS_TEMP_UNPUBLISHCOMPONENTS, ACTION_UnpublishComponents, L"PublishComponents" },
    { L"UnpublishFeatures", IDS_DESC_UNPUBLISHFEATURES, IDS_TEMP_UNPUBLISHFEATURES, ACTION_UnpublishFeatures, L"PublishFeatures" },
    { L"UnpublishProduct", IDS_DESC_UNPUBLISHPRODUCT, 0, ACTION_UnpublishProduct, NULL }, /* for rollback only */
    { L"UnregisterClassInfo", IDS_DESC_UNREGISTERCLASSINFO, IDS_TEMP_UNREGISTERCLASSINFO, ACTION_UnregisterClassInfo, L"RegisterClassInfo" },
    { L"UnregisterComPlus", IDS_DESC_UNREGISTERCOMPLUS, IDS_TEMP_UNREGISTERCOMPLUS, ACTION_UnregisterComPlus, L"RegisterComPlus" },
    { L"UnregisterExtensionInfo", IDS_DESC_UNREGISTEREXTENSIONINFO, IDS_TEMP_UNREGISTEREXTENSIONINFO, ACTION_UnregisterExtensionInfo, L"RegisterExtensionInfo" },
    { L"UnregisterFonts", IDS_DESC_UNREGISTERFONTS, IDS_TEMP_UNREGISTERFONTS, ACTION_UnregisterFonts, L"RegisterFonts" },
    { L"UnregisterMIMEInfo", IDS_DESC_UNREGISTERMIMEINFO, IDS_TEMP_UNREGISTERMIMEINFO, ACTION_UnregisterMIMEInfo, L"RegisterMIMEInfo" },
    { L"UnregisterProgIdInfo", IDS_DESC_UNREGISTERPROGIDINFO, IDS_TEMP_UNREGISTERPROGIDINFO, ACTION_UnregisterProgIdInfo, L"RegisterProgIdInfo" },
    { L"UnregisterTypeLibraries", IDS_DESC_UNREGISTERTYPELIBRARIES, IDS_TEMP_UNREGISTERTYPELIBRARIES, ACTION_UnregisterTypeLibraries, L"RegisterTypeLibraries" },
    { L"ValidateProductID", 0, 0, ACTION_ValidateProductID, NULL },
    { L"WriteEnvironmentStrings", IDS_DESC_WRITEENVIRONMENTSTRINGS, IDS_TEMP_WRITEENVIRONMENTSTRINGS, ACTION_WriteEnvironmentStrings, L"RemoveEnvironmentStrings" },
    { L"WriteIniValues", IDS_DESC_WRITEINIVALUES, IDS_TEMP_WRITEINIVALUES, ACTION_WriteIniValues, L"RemoveIniValues" },
    { L"WriteRegistryValues", IDS_DESC_WRITEREGISTRYVALUES, IDS_TEMP_WRITEREGISTRYVALUES, ACTION_WriteRegistryValues, L"RemoveRegistryValues" },
    { L"INSTALL", 0, 0, ACTION_INSTALL, NULL },
    { 0 }
};

static UINT ACTION_HandleStandardAction(MSIPACKAGE *package, LPCWSTR action)
{
    UINT rc = ERROR_FUNCTION_NOT_CALLED;
    UINT i;

    i = 0;
    while (StandardActions[i].action != NULL)
    {
        if (!wcscmp( StandardActions[i].action, action ))
        {
            WCHAR description[100] = {0}, template[100] = {0};

            if (StandardActions[i].description != 0)
                LoadStringW(msi_hInstance, StandardActions[i].description, (LPWSTR)&description, 100);
            if (StandardActions[i].template != 0)
                LoadStringW(msi_hInstance, StandardActions[i].template, (LPWSTR)&template, 100);

            ui_actionstart(package, action, description, template);
            if (StandardActions[i].handler)
            {
                ui_actioninfo( package, action, TRUE, 0 );
                rc = StandardActions[i].handler( package );
                ui_actioninfo( package, action, FALSE, !rc );

                if (StandardActions[i].action_rollback && !package->need_rollback)
                {
                    TRACE("scheduling rollback action\n");
                    msi_schedule_action( package, SCRIPT_ROLLBACK, StandardActions[i].action_rollback );
                }
            }
            else
            {
                FIXME("unhandled standard action %s\n", debugstr_w(action));
                rc = ERROR_SUCCESS;
            }
            break;
        }
        i++;
    }

    return rc;
}

UINT ACTION_PerformAction(MSIPACKAGE *package, const WCHAR *action)
{
    UINT rc;

    TRACE("Performing action (%s)\n", debugstr_w(action));

    package->action_progress_increment = 0;
    rc = ACTION_HandleStandardAction(package, action);

    if (rc == ERROR_FUNCTION_NOT_CALLED)
        rc = ACTION_HandleCustomAction(package, action);

    if (rc == ERROR_FUNCTION_NOT_CALLED)
        WARN("unhandled msi action %s\n", debugstr_w(action));

    return rc;
}

static UINT ACTION_PerformActionSequence(MSIPACKAGE *package, UINT seq)
{
    UINT rc = ERROR_SUCCESS;
    MSIRECORD *row;

    if (needs_ui_sequence(package))
        row = MSI_QueryGetRecord(package->db, L"SELECT * FROM `InstallUISequence` WHERE `Sequence` = %d", seq);
    else
        row = MSI_QueryGetRecord(package->db, L"SELECT * FROM `InstallExecuteSequence` WHERE `Sequence` = %d", seq);

    if (row)
    {
        LPCWSTR action, cond;

        TRACE("Running the actions\n");

        /* check conditions */
        cond = MSI_RecordGetString(row, 2);

        /* this is a hack to skip errors in the condition code */
        if (MSI_EvaluateConditionW(package, cond) == MSICONDITION_FALSE)
        {
            msiobj_release(&row->hdr);
            return ERROR_SUCCESS;
        }

        action = MSI_RecordGetString(row, 1);
        if (!action)
        {
            ERR("failed to fetch action\n");
            msiobj_release(&row->hdr);
            return ERROR_FUNCTION_FAILED;
        }

        rc = ACTION_PerformAction(package, action);

        msiobj_release(&row->hdr);
    }

    return rc;
}

DWORD WINAPI dummy_thread_proc(void *arg)
{
    struct dummy_thread *info = arg;
    HRESULT hr;

    hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hr)) ERR("CoInitializeEx failed %08x\n", hr);

    SetEvent(info->started);
    WaitForSingleObject(info->stopped, INFINITE);

    CoUninitialize();
    return 0;
}

static void start_dummy_thread(struct dummy_thread *info)
{
    if (!(info->started = CreateEventA(NULL, TRUE, FALSE, NULL))) return;
    if (!(info->stopped = CreateEventA(NULL, TRUE, FALSE, NULL))) return;
    if (!(info->thread  = CreateThread(NULL, 0, dummy_thread_proc, info, 0, NULL))) return;

    WaitForSingleObject(info->started, INFINITE);
}

static void stop_dummy_thread(struct dummy_thread *info)
{
    if (info->thread)
    {
        SetEvent(info->stopped);
        WaitForSingleObject(info->thread, INFINITE);
        CloseHandle(info->thread);
    }
    if (info->started) CloseHandle(info->started);
    if (info->stopped) CloseHandle(info->stopped);
}

/****************************************************
 * TOP level entry points
 *****************************************************/

UINT MSI_InstallPackage( MSIPACKAGE *package, LPCWSTR szPackagePath,
                         LPCWSTR szCommandLine )
{
    WCHAR *reinstall = NULL, *productcode, *action;
    struct dummy_thread thread_info = {NULL, NULL, NULL};
    UINT rc;
    DWORD len = 0;

    if (szPackagePath)
    {
        LPWSTR p, dir;
        LPCWSTR file;

        dir = wcsdup(szPackagePath);
        p = wcsrchr(dir, '\\');
        if (p)
        {
            *(++p) = 0;
            file = szPackagePath + (p - dir);
        }
        else
        {
            free(dir);
            dir = malloc(MAX_PATH * sizeof(WCHAR));
            GetCurrentDirectoryW(MAX_PATH, dir);
            lstrcatW(dir, L"\\");
            file = szPackagePath;
        }

        free(package->PackagePath);
        package->PackagePath = malloc((wcslen(dir) + wcslen(file) + 1) * sizeof(WCHAR));
        if (!package->PackagePath)
        {
            free(dir);
            return ERROR_OUTOFMEMORY;
        }

        lstrcpyW(package->PackagePath, dir);
        lstrcatW(package->PackagePath, file);
        free(dir);

        msi_set_sourcedir_props(package, FALSE);
    }

    rc = msi_parse_command_line( package, szCommandLine, FALSE );
    if (rc != ERROR_SUCCESS)
        return rc;

    msi_apply_transforms( package );
    msi_apply_patches( package );

    if (msi_get_property( package->db, L"ACTION", NULL, &len ))
        msi_set_property( package->db, L"ACTION", L"INSTALL", -1 );
    action = msi_dup_property( package->db, L"ACTION" );
    CharUpperW(action);

    msi_set_original_database_property( package->db, szPackagePath );
    msi_parse_command_line( package, szCommandLine, FALSE );
    msi_adjust_privilege_properties( package );
    msi_set_context( package );

    start_dummy_thread(&thread_info);

    productcode = msi_dup_property( package->db, L"ProductCode" );
    if (wcsicmp( productcode, package->ProductCode ))
    {
        TRACE( "product code changed %s -> %s\n", debugstr_w(package->ProductCode), debugstr_w(productcode) );
        free( package->ProductCode );
        package->ProductCode = productcode;
    }
    else free( productcode );

    if (msi_get_property_int( package->db, L"DISABLEROLLBACK", 0 ))
    {
        TRACE("disabling rollback\n");
        msi_set_property( package->db, L"RollbackDisabled", L"1", -1 );
    }

    rc = ACTION_PerformAction(package, action);

    /* process the ending type action */
    if (rc == ERROR_SUCCESS)
        ACTION_PerformActionSequence(package, -1);
    else if (rc == ERROR_INSTALL_USEREXIT)
        ACTION_PerformActionSequence(package, -2);
    else if (rc == ERROR_INSTALL_SUSPEND)
        ACTION_PerformActionSequence(package, -4);
    else  /* failed */
    {
        ACTION_PerformActionSequence(package, -3);
        if (!msi_get_property_int( package->db, L"RollbackDisabled", 0 ))
        {
            package->need_rollback = TRUE;
        }
    }

    /* finish up running custom actions */
    ACTION_FinishCustomActions(package);

    stop_dummy_thread(&thread_info);

    if (package->need_rollback && !(reinstall = msi_dup_property( package->db, L"REINSTALL" )))
    {
        WARN("installation failed, running rollback script\n");
        execute_script( package, SCRIPT_ROLLBACK );
    }
    free( reinstall );
    free( action );

    if (rc == ERROR_SUCCESS && package->need_reboot_at_end)
        return ERROR_SUCCESS_REBOOT_REQUIRED;

    return rc;
}
