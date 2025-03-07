/*
 * Implementation of the Microsoft Installer (msi.dll)
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

/* Actions handled in this module:
 *
 * RegisterClassInfo
 * RegisterProgIdInfo
 * RegisterExtensionInfo
 * RegisterMIMEInfo
 * UnregisterClassInfo
 * UnregisterProgIdInfo
 * UnregisterExtensionInfo
 * UnregisterMIMEInfo
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "wine/debug.h"
#include "msipriv.h"
#include "winuser.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

static MSIAPPID *load_appid( MSIPACKAGE* package, MSIRECORD *row )
{
    LPCWSTR buffer;
    MSIAPPID *appid;

    /* fill in the data */

    appid = calloc( 1, sizeof(MSIAPPID) );
    if (!appid)
        return NULL;

    appid->AppID = msi_dup_record_field( row, 1 );
    TRACE("loading appid %s\n", debugstr_w( appid->AppID ));

    buffer = MSI_RecordGetString(row,2);
    deformat_string( package, buffer, &appid->RemoteServerName );

    appid->LocalServer = msi_dup_record_field(row,3);
    appid->ServiceParameters = msi_dup_record_field(row,4);
    appid->DllSurrogate = msi_dup_record_field(row,5);

    appid->ActivateAtStorage = !MSI_RecordIsNull(row,6);
    appid->RunAsInteractiveUser = !MSI_RecordIsNull(row,7);

    list_add_tail( &package->appids, &appid->entry );

    return appid;
}

static MSIAPPID *load_given_appid( MSIPACKAGE *package, LPCWSTR name )
{
    MSIRECORD *row;
    MSIAPPID *appid;

    if (!name)
        return NULL;

    /* check for appids already loaded */
    LIST_FOR_EACH_ENTRY( appid, &package->appids, MSIAPPID, entry )
    {
        if (!wcsicmp( appid->AppID, name ))
        {
            TRACE("found appid %s %p\n", debugstr_w(name), appid);
            return appid;
        }
    }

    row = MSI_QueryGetRecord(package->db, L"SELECT * FROM `AppId` WHERE `AppId` = '%s'", name);
    if (!row)
        return NULL;

    appid = load_appid(package, row);
    msiobj_release(&row->hdr);
    return appid;
}

static MSIPROGID *load_given_progid(MSIPACKAGE *package, LPCWSTR progid);
static MSICLASS *load_given_class( MSIPACKAGE *package, LPCWSTR classid );

static MSIPROGID *load_progid( MSIPACKAGE* package, MSIRECORD *row )
{
    MSIPROGID *progid;
    LPCWSTR buffer;

    /* fill in the data */

    progid = calloc( 1, sizeof(MSIPROGID) );
    if (!progid)
        return NULL;

    list_add_tail( &package->progids, &progid->entry );

    progid->ProgID = msi_dup_record_field(row,1);
    TRACE("loading progid %s\n",debugstr_w(progid->ProgID));

    buffer = MSI_RecordGetString(row,2);
    progid->Parent = load_given_progid(package,buffer);
    if (progid->Parent == NULL && buffer)
        FIXME("Unknown parent ProgID %s\n",debugstr_w(buffer));

    buffer = MSI_RecordGetString(row,3);
    progid->Class = load_given_class(package,buffer);
    if (progid->Class == NULL && buffer)
        FIXME("Unknown class %s\n",debugstr_w(buffer));

    progid->Description = msi_dup_record_field(row,4);

    if (!MSI_RecordIsNull(row,6))
    {
        INT icon_index = MSI_RecordGetInteger(row,6);
        LPCWSTR FileName = MSI_RecordGetString(row,5);
        LPWSTR FilePath;

        FilePath = msi_build_icon_path(package, FileName);

        progid->IconPath = malloc( (wcslen(FilePath) + 10) * sizeof(WCHAR) );
        swprintf( progid->IconPath, lstrlenW(FilePath) + 10, L"%s,%d", FilePath, icon_index );
        free(FilePath);
    }
    else
    {
        buffer = MSI_RecordGetString(row,5);
        if (buffer)
            progid->IconPath = msi_build_icon_path(package, buffer);
    }

    progid->CurVer = NULL;
    progid->VersionInd = NULL;

    /* if we have a parent then we may be that parents CurVer */
    if (progid->Parent && progid->Parent != progid)
    {
        MSIPROGID *parent = progid->Parent;

        while (parent->Parent && parent->Parent != parent)
            parent = parent->Parent;

        /* FIXME: need to determine if we are really the CurVer */

        progid->CurVer = parent;
        parent->VersionInd = progid;
    }

    return progid;
}

static MSIPROGID *load_given_progid(MSIPACKAGE *package, LPCWSTR name)
{
    MSIPROGID *progid;
    MSIRECORD *row;

    if (!name)
        return NULL;

    /* check for progids already loaded */
    LIST_FOR_EACH_ENTRY( progid, &package->progids, MSIPROGID, entry )
    {
        if (!wcsicmp( progid->ProgID, name ))
        {
            TRACE("found progid %s (%p)\n",debugstr_w(name), progid );
            return progid;
        }
    }

    row = MSI_QueryGetRecord( package->db, L"SELECT * FROM `ProgId` WHERE `ProgId` = '%s'", name );
    if (!row)
        return NULL;

    progid = load_progid(package, row);
    msiobj_release(&row->hdr);
    return progid;
}

static MSICLASS *load_class( MSIPACKAGE* package, MSIRECORD *row )
{
    MSICLASS *cls;
    DWORD i;
    LPCWSTR buffer;

    /* fill in the data */

    cls = calloc( 1, sizeof(MSICLASS) );
    if (!cls)
        return NULL;

    list_add_tail( &package->classes, &cls->entry );

    cls->clsid = msi_dup_record_field( row, 1 );
    TRACE("loading class %s\n",debugstr_w(cls->clsid));
    cls->Context = msi_dup_record_field( row, 2 );
    buffer = MSI_RecordGetString(row,3);
    cls->Component = msi_get_loaded_component( package, buffer );

    cls->ProgIDText = msi_dup_record_field(row,4);
    cls->ProgID = load_given_progid(package, cls->ProgIDText);

    cls->Description = msi_dup_record_field(row,5);

    buffer = MSI_RecordGetString(row,6);
    if (buffer)
        cls->AppID = load_given_appid(package, buffer);

    cls->FileTypeMask = msi_dup_record_field(row,7);

    if (!MSI_RecordIsNull(row,9))
    {

        INT icon_index = MSI_RecordGetInteger(row,9);
        LPCWSTR FileName = MSI_RecordGetString(row,8);
        LPWSTR FilePath;

        FilePath = msi_build_icon_path(package, FileName);

        cls->IconPath = malloc( (wcslen(FilePath) + 5) * sizeof(WCHAR) );
        swprintf( cls->IconPath, lstrlenW(FilePath) + 5, L"%s,%d", FilePath, icon_index );
        free(FilePath);
    }
    else
    {
        buffer = MSI_RecordGetString(row,8);
        if (buffer)
            cls->IconPath = msi_build_icon_path(package, buffer);
    }

    if (!MSI_RecordIsNull(row,10))
    {
        i = MSI_RecordGetInteger(row,10);
        if (i != MSI_NULL_INTEGER && i > 0 &&  i < 4)
        {
            switch(i)
            {
                case 1:
                    cls->DefInprocHandler = wcsdup(L"ole2.dll");
                    break;
                case 2:
                    cls->DefInprocHandler32 = wcsdup(L"ole32.dll");
                    break;
                case 3:
                    cls->DefInprocHandler = wcsdup(L"ole2.dll");
                    cls->DefInprocHandler32 = wcsdup(L"ole32.dll");
                    break;
            }
        }
        else
        {
            cls->DefInprocHandler32 = msi_dup_record_field( row, 10 );
            msi_reduce_to_long_filename( cls->DefInprocHandler32 );
        }
    }
    buffer = MSI_RecordGetString(row,11);
    deformat_string(package,buffer,&cls->Argument);

    buffer = MSI_RecordGetString(row,12);
    cls->Feature = msi_get_loaded_feature(package, buffer);

    cls->Attributes = MSI_RecordGetInteger(row,13);
    cls->action = INSTALLSTATE_UNKNOWN;
    return cls;
}

/*
 * the Class table has 3 primary keys. Generally it is only
 * referenced through the first CLSID key. However when loading
 * all of the classes we need to make sure we do not ignore rows
 * with other Context and ComponentIndexs
 */
static MSICLASS *load_given_class(MSIPACKAGE *package, LPCWSTR classid)
{
    MSICLASS *cls;
    MSIRECORD *row;

    if (!classid)
        return NULL;

    /* check for classes already loaded */
    LIST_FOR_EACH_ENTRY( cls, &package->classes, MSICLASS, entry )
    {
        if (!wcsicmp( cls->clsid, classid ))
        {
            TRACE("found class %s (%p)\n",debugstr_w(classid), cls);
            return cls;
        }
    }

    row = MSI_QueryGetRecord( package->db, L"SELECT * FROM `Class` WHERE `CLSID` = '%s'", classid );
    if (!row)
        return NULL;

    cls = load_class(package, row);
    msiobj_release(&row->hdr);
    return cls;
}

static MSIEXTENSION *load_given_extension( MSIPACKAGE *package, LPCWSTR extension );

static MSIMIME *load_mime( MSIPACKAGE* package, MSIRECORD *row )
{
    LPCWSTR extension;
    MSIMIME *mt;

    /* fill in the data */

    mt = calloc( 1, sizeof(MSIMIME) );
    if (!mt)
        return mt;

    mt->ContentType = msi_dup_record_field( row, 1 );
    TRACE("loading mime %s\n", debugstr_w(mt->ContentType));

    extension = MSI_RecordGetString( row, 2 );
    mt->Extension = load_given_extension( package, extension );
    mt->suffix = wcsdup( extension );

    mt->clsid = msi_dup_record_field( row, 3 );
    mt->Class = load_given_class( package, mt->clsid );

    list_add_tail( &package->mimes, &mt->entry );

    return mt;
}

static MSIMIME *load_given_mime( MSIPACKAGE *package, LPCWSTR mime )
{
    MSIRECORD *row;
    MSIMIME *mt;

    if (!mime)
        return NULL;

    /* check for mime already loaded */
    LIST_FOR_EACH_ENTRY( mt, &package->mimes, MSIMIME, entry )
    {
        if (!wcsicmp( mt->ContentType, mime ))
        {
            TRACE("found mime %s (%p)\n",debugstr_w(mime), mt);
            return mt;
        }
    }

    row = MSI_QueryGetRecord( package->db, L"SELECT * FROM `MIME` WHERE `ContentType` = '%s'", mime );
    if (!row)
        return NULL;

    mt = load_mime(package, row);
    msiobj_release(&row->hdr);
    return mt;
}

static MSIEXTENSION *load_extension( MSIPACKAGE* package, MSIRECORD *row )
{
    MSIEXTENSION *ext;
    LPCWSTR buffer;

    /* fill in the data */

    ext = calloc( 1, sizeof(MSIEXTENSION) );
    if (!ext)
        return NULL;

    list_init( &ext->verbs );

    list_add_tail( &package->extensions, &ext->entry );

    ext->Extension = msi_dup_record_field( row, 1 );
    TRACE("loading extension %s\n", debugstr_w(ext->Extension));

    buffer = MSI_RecordGetString( row, 2 );
    ext->Component = msi_get_loaded_component( package, buffer );

    ext->ProgIDText = msi_dup_record_field( row, 3 );
    ext->ProgID = load_given_progid( package, ext->ProgIDText );

    buffer = MSI_RecordGetString( row, 4 );
    ext->Mime = load_given_mime( package, buffer );

    buffer = MSI_RecordGetString(row,5);
    ext->Feature = msi_get_loaded_feature( package, buffer );
    ext->action = INSTALLSTATE_UNKNOWN;
    return ext;
}

/*
 * While the extension table has 2 primary keys, this function is only looking
 * at the Extension key which is what is referenced as a foreign key
 */
static MSIEXTENSION *load_given_extension( MSIPACKAGE *package, LPCWSTR name )
{
    MSIEXTENSION *ext;
    MSIRECORD *row;

    if (!name)
        return NULL;

    if (name[0] == '.')
        name++;

    /* check for extensions already loaded */
    LIST_FOR_EACH_ENTRY( ext, &package->extensions, MSIEXTENSION, entry )
    {
        if (!wcsicmp( ext->Extension, name ))
        {
            TRACE("extension %s already loaded %p\n", debugstr_w(name), ext);
            return ext;
        }
    }

    row = MSI_QueryGetRecord( package->db, L"SELECT * FROM `Extension` WHERE `Extension` = '%s'", name );
    if (!row)
        return NULL;

    ext = load_extension(package, row);
    msiobj_release(&row->hdr);
    return ext;
}

static UINT iterate_load_verb(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE* package = param;
    MSIVERB *verb;
    LPCWSTR buffer;
    MSIEXTENSION *extension;

    buffer = MSI_RecordGetString(row,1);
    extension = load_given_extension( package, buffer );
    if (!extension)
    {
        ERR("Verb unable to find loaded extension %s\n", debugstr_w(buffer));
        return ERROR_SUCCESS;
    }

    /* fill in the data */

    verb = calloc( 1, sizeof(MSIVERB) );
    if (!verb)
        return ERROR_OUTOFMEMORY;

    verb->Verb = msi_dup_record_field(row,2);
    TRACE("loading verb %s\n",debugstr_w(verb->Verb));
    verb->Sequence = MSI_RecordGetInteger(row,3);

    buffer = MSI_RecordGetString(row,4);
    deformat_string(package,buffer,&verb->Command);

    buffer = MSI_RecordGetString(row,5);
    deformat_string(package,buffer,&verb->Argument);

    /* associate the verb with the correct extension */
    list_add_tail( &extension->verbs, &verb->entry );

    return ERROR_SUCCESS;
}

static UINT iterate_all_classes(MSIRECORD *rec, LPVOID param)
{
    MSICOMPONENT *comp;
    LPCWSTR clsid;
    LPCWSTR context;
    LPCWSTR buffer;
    MSIPACKAGE* package = param;
    MSICLASS *cls;
    BOOL match = FALSE;

    clsid = MSI_RecordGetString(rec,1);
    context = MSI_RecordGetString(rec,2);
    buffer = MSI_RecordGetString(rec,3);
    comp = msi_get_loaded_component(package, buffer);

    LIST_FOR_EACH_ENTRY( cls, &package->classes, MSICLASS, entry )
    {
        if (wcsicmp( clsid, cls->clsid ))
            continue;
        if (wcscmp( context, cls->Context ))
            continue;
        if (comp == cls->Component)
        {
            match = TRUE;
            break;
        }
    }

    if (!match)
        load_class(package, rec);

    return ERROR_SUCCESS;
}

static UINT load_all_classes( MSIPACKAGE *package )
{
    MSIQUERY *view;
    UINT rc;

    rc = MSI_DatabaseOpenViewW( package->db, L"SELECT * FROM `Class`", &view );
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords(view, NULL, iterate_all_classes, package);
    msiobj_release(&view->hdr);
    return rc;
}

static UINT iterate_all_extensions(MSIRECORD *rec, LPVOID param)
{
    MSICOMPONENT *comp;
    LPCWSTR buffer;
    LPCWSTR extension;
    MSIPACKAGE* package = param;
    BOOL match = FALSE;
    MSIEXTENSION *ext;

    extension = MSI_RecordGetString(rec,1);
    buffer = MSI_RecordGetString(rec,2);
    comp = msi_get_loaded_component(package, buffer);

    LIST_FOR_EACH_ENTRY( ext, &package->extensions, MSIEXTENSION, entry )
    {
        if (wcsicmp(extension, ext->Extension))
            continue;
        if (comp == ext->Component)
        {
            match = TRUE;
            break;
        }
    }

    if (!match)
        load_extension(package, rec);

    return ERROR_SUCCESS;
}

static UINT load_all_extensions( MSIPACKAGE *package )
{
    MSIQUERY *view;
    UINT rc;

    rc = MSI_DatabaseOpenViewW( package->db, L"SELECT * FROM `Extension`", &view );
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords(view, NULL, iterate_all_extensions, package);
    msiobj_release(&view->hdr);
    return rc;
}

static UINT iterate_all_progids(MSIRECORD *rec, LPVOID param)
{
    LPCWSTR buffer;
    MSIPACKAGE* package = param;

    buffer = MSI_RecordGetString(rec,1);
    load_given_progid(package,buffer);
    return ERROR_SUCCESS;
}

static UINT load_all_progids( MSIPACKAGE *package )
{
    MSIQUERY *view;
    UINT rc;

    rc = MSI_DatabaseOpenViewW( package->db, L"SELECT `ProgId` FROM `ProgId`", &view );
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords(view, NULL, iterate_all_progids, package);
    msiobj_release(&view->hdr);
    return rc;
}

static UINT load_all_verbs( MSIPACKAGE *package )
{
    MSIQUERY *view;
    UINT rc;

    rc = MSI_DatabaseOpenViewW( package->db, L"SELECT * FROM `Verb`", &view );
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords(view, NULL, iterate_load_verb, package);
    msiobj_release(&view->hdr);
    return rc;
}

static UINT iterate_all_mimes(MSIRECORD *rec, LPVOID param)
{
    LPCWSTR buffer;
    MSIPACKAGE* package = param;

    buffer = MSI_RecordGetString(rec,1);
    load_given_mime(package,buffer);
    return ERROR_SUCCESS;
}

static UINT load_all_mimes( MSIPACKAGE *package )
{
    MSIQUERY *view;
    UINT rc;

    rc = MSI_DatabaseOpenViewW( package->db, L"SELECT `ContentType` FROM `MIME`", &view );
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords(view, NULL, iterate_all_mimes, package);
    msiobj_release(&view->hdr);
    return rc;
}

static UINT load_classes_and_such( MSIPACKAGE *package )
{
    UINT r;

    TRACE("Loading all the class info and related tables\n");

    /* check if already loaded */
    if (!list_empty( &package->classes ) ||
        !list_empty( &package->mimes ) ||
        !list_empty( &package->extensions ) ||
        !list_empty( &package->progids )) return ERROR_SUCCESS;

    r = load_all_classes( package );
    if (r != ERROR_SUCCESS) return r;

    r = load_all_extensions( package );
    if (r != ERROR_SUCCESS) return r;

    r = load_all_progids( package );
    if (r != ERROR_SUCCESS) return r;

    /* these loads must come after the other loads */
    r = load_all_verbs( package );
    if (r != ERROR_SUCCESS) return r;

    return load_all_mimes( package );
}

static UINT register_appid(const MSIAPPID *appid, LPCWSTR app )
{
    HKEY hkey2, hkey3;

    RegCreateKeyW( HKEY_CLASSES_ROOT, L"AppID", &hkey2 );
    RegCreateKeyW( hkey2, appid->AppID, &hkey3 );
    RegCloseKey(hkey2);
    msi_reg_set_val_str( hkey3, NULL, app );

    if (appid->RemoteServerName)
        msi_reg_set_val_str( hkey3, L"RemoteServerName", appid->RemoteServerName );

    if (appid->LocalServer)
        msi_reg_set_val_str( hkey3, L"LocalService", appid->LocalServer );

    if (appid->ServiceParameters)
        msi_reg_set_val_str( hkey3, L"ServiceParameters", appid->ServiceParameters );

    if (appid->DllSurrogate)
        msi_reg_set_val_str( hkey3, L"DllSurrogate", appid->DllSurrogate );

    if (appid->ActivateAtStorage)
        msi_reg_set_val_str( hkey3, L"ActivateAtStorage", L"Y" );

    if (appid->RunAsInteractiveUser)
        msi_reg_set_val_str( hkey3, L"RunAs", L"Interactive User" );

    RegCloseKey(hkey3);
    return ERROR_SUCCESS;
}

UINT ACTION_RegisterClassInfo(MSIPACKAGE *package)
{
    REGSAM access = KEY_ALL_ACCESS;
    MSIRECORD *uirow;
    HKEY hkey, hkey2, hkey3;
    MSICLASS *cls;
    UINT r;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action( package, SCRIPT_INSTALL, L"RegisterClassInfo" );

    r = load_classes_and_such( package );
    if (r != ERROR_SUCCESS)
        return r;

    if (package->platform == PLATFORM_INTEL)
        access |= KEY_WOW64_32KEY;
    else
        access |= KEY_WOW64_64KEY;

    if (RegCreateKeyExW( HKEY_CLASSES_ROOT, L"CLSID", 0, NULL, 0, access, NULL, &hkey, NULL ))
        return ERROR_FUNCTION_FAILED;

    LIST_FOR_EACH_ENTRY( cls, &package->classes, MSICLASS, entry )
    {
        MSICOMPONENT *comp;
        MSIFILE *file;
        DWORD size;
        LPWSTR argument;
        MSIFEATURE *feature;

        comp = cls->Component;
        if ( !comp )
            continue;

        if (!comp->Enabled)
        {
            TRACE("component is disabled\n");
            continue;
        }

        feature = cls->Feature;
        if (!feature)
            continue;

        feature->Action = msi_get_feature_action( package, feature );
        if (feature->Action != INSTALLSTATE_LOCAL &&
            feature->Action != INSTALLSTATE_ADVERTISED )
        {
            TRACE("feature %s not scheduled for installation, skipping registration of class %s\n",
                  debugstr_w(feature->Feature), debugstr_w(cls->clsid));
            continue;
        }

        if (!comp->KeyPath || !(file = msi_get_loaded_file( package, comp->KeyPath )))
        {
            TRACE("COM server not provided, skipping class %s\n", debugstr_w(cls->clsid));
            continue;
        }
        TRACE("Registering class %s (%p)\n", debugstr_w(cls->clsid), cls);

        cls->action = INSTALLSTATE_LOCAL;

        RegCreateKeyW( hkey, cls->clsid, &hkey2 );

        if (cls->Description)
            msi_reg_set_val_str( hkey2, NULL, cls->Description );

        RegCreateKeyW( hkey2, cls->Context, &hkey3 );

        /*
         * FIXME: Implement install on demand (advertised components).
         *
         * ole32.dll should call msi.MsiProvideComponentFromDescriptor()
         *  when it needs an InProcServer that doesn't exist.
         * The component advertise string should be in the "InProcServer" value.
         */
        size = lstrlenW( file->TargetPath )+1;
        if (cls->Argument)
            size += lstrlenW(cls->Argument)+1;

        argument = malloc( size * sizeof(WCHAR) );
        lstrcpyW( argument, file->TargetPath );

        if (cls->Argument)
        {
            lstrcatW( argument, L" " );
            lstrcatW( argument, cls->Argument );
        }

        msi_reg_set_val_str( hkey3, NULL, argument );
        free(argument);

        RegCloseKey(hkey3);

        if (cls->ProgID || cls->ProgIDText)
        {
            LPCWSTR progid;

            if (cls->ProgID)
                progid = cls->ProgID->ProgID;
            else
                progid = cls->ProgIDText;

            msi_reg_set_subkey_val( hkey2, L"ProgID", NULL, progid );

            if (cls->ProgID && cls->ProgID->VersionInd)
            {
                msi_reg_set_subkey_val( hkey2, L"VersionIndependentProgID", NULL,
                                        cls->ProgID->VersionInd->ProgID );
            }
        }

        if (cls->AppID)
        {
            MSIAPPID *appid = cls->AppID;
            msi_reg_set_val_str( hkey2, L"AppID", appid->AppID );
            register_appid( appid, cls->Description );
        }

        if (cls->IconPath)
            msi_reg_set_subkey_val( hkey2, L"DefaultIcon", NULL, cls->IconPath );

        if (cls->DefInprocHandler)
            msi_reg_set_subkey_val( hkey2, L"InprocHandler", NULL, cls->DefInprocHandler );

        if (cls->DefInprocHandler32)
            msi_reg_set_subkey_val( hkey2, L"InprocHandler32", NULL, cls->DefInprocHandler32 );
        RegCloseKey(hkey2);

        /* if there is a FileTypeMask, register the FileType */
        if (cls->FileTypeMask)
        {
            LPWSTR ptr, ptr2;
            LPWSTR keyname;
            INT index = 0;
            ptr = cls->FileTypeMask;
            while (ptr && *ptr)
            {
                ptr2 = wcschr(ptr,';');
                if (ptr2)
                    *ptr2 = 0;
                keyname = malloc( sizeof(L"FileType\\%s\\%d") + (wcslen(cls->clsid) + 3) * sizeof(WCHAR) );
                swprintf( keyname, lstrlenW(L"FileType\\%s\\%d") + lstrlenW(cls->clsid) + 4,
                          L"FileType\\%s\\%d", cls->clsid, index );

                msi_reg_set_subkey_val( HKEY_CLASSES_ROOT, keyname, NULL, ptr );
                free( keyname );

                if (ptr2)
                    ptr = ptr2+1;
                else
                    ptr = NULL;

                index ++;
            }
        }

        uirow = MSI_CreateRecord(1);
        MSI_RecordSetStringW( uirow, 1, cls->clsid );
        MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
        msiobj_release(&uirow->hdr);
    }
    RegCloseKey(hkey);
    return ERROR_SUCCESS;
}

UINT ACTION_UnregisterClassInfo( MSIPACKAGE *package )
{
    REGSAM access = KEY_ALL_ACCESS;
    MSIRECORD *uirow;
    MSICLASS *cls;
    HKEY hkey, hkey2;
    UINT r;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action( package, SCRIPT_INSTALL, L"UnregisterClassInfo" );

    r = load_classes_and_such( package );
    if (r != ERROR_SUCCESS)
        return r;

    if (package->platform == PLATFORM_INTEL)
        access |= KEY_WOW64_32KEY;
    else
        access |= KEY_WOW64_64KEY;

    if (RegCreateKeyExW( HKEY_CLASSES_ROOT, L"CLSID", 0, NULL, 0, access, NULL, &hkey, NULL ))
        return ERROR_FUNCTION_FAILED;

    LIST_FOR_EACH_ENTRY( cls, &package->classes, MSICLASS, entry )
    {
        MSIFEATURE *feature;
        MSICOMPONENT *comp;
        LPWSTR filetype;
        LONG res;

        comp = cls->Component;
        if (!comp)
            continue;

        if (!comp->Enabled)
        {
            TRACE("component is disabled\n");
            continue;
        }

        feature = cls->Feature;
        if (!feature)
            continue;

        feature->Action = msi_get_feature_action( package, feature );
        if (feature->Action != INSTALLSTATE_ABSENT)
        {
            TRACE("feature %s not scheduled for removal, skipping unregistration of class %s\n",
                  debugstr_w(feature->Feature), debugstr_w(cls->clsid));
            continue;
        }
        TRACE("Unregistering class %s (%p)\n", debugstr_w(cls->clsid), cls);

        cls->action = INSTALLSTATE_ABSENT;

        res = RegDeleteTreeW( hkey, cls->clsid );
        if (res != ERROR_SUCCESS)
            WARN("failed to delete class key %ld\n", res);

        if (cls->AppID)
        {
            res = RegOpenKeyW( HKEY_CLASSES_ROOT, L"AppID", &hkey2 );
            if (res == ERROR_SUCCESS)
            {
                res = RegDeleteKeyW( hkey2, cls->AppID->AppID );
                if (res != ERROR_SUCCESS)
                    WARN("failed to delete appid key %ld\n", res);
                RegCloseKey( hkey2 );
            }
        }
        if (cls->FileTypeMask)
        {
            filetype = malloc( sizeof( L"FileType\\" ) + wcslen( cls->clsid ) * sizeof(WCHAR) );
            if (filetype)
            {
                lstrcpyW( filetype, L"FileType\\" );
                lstrcatW( filetype, cls->clsid );
                res = RegDeleteTreeW( HKEY_CLASSES_ROOT, filetype );
                free( filetype );

                if (res != ERROR_SUCCESS)
                    WARN("failed to delete file type %ld\n", res);
            }
        }

        uirow = MSI_CreateRecord( 1 );
        MSI_RecordSetStringW( uirow, 1, cls->clsid );
        MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
        msiobj_release( &uirow->hdr );
    }
    RegCloseKey( hkey );
    return ERROR_SUCCESS;
}

static LPCWSTR get_clsid_of_progid( const MSIPROGID *progid )
{
    while (progid)
    {
        if (progid->Class)
            return progid->Class->clsid;
        if (progid->Parent == progid)
            break;
        progid = progid->Parent;
    }
    return NULL;
}

static UINT register_progid( const MSIPROGID* progid )
{
    HKEY hkey = 0;
    UINT rc;

    rc = RegCreateKeyW( HKEY_CLASSES_ROOT, progid->ProgID, &hkey );
    if (rc == ERROR_SUCCESS)
    {
        LPCWSTR clsid = get_clsid_of_progid( progid );

        if (clsid)
            msi_reg_set_subkey_val( hkey, L"CLSID", NULL, clsid );
        else
            TRACE("%s has no class\n", debugstr_w( progid->ProgID ) );

        if (progid->Description)
            msi_reg_set_val_str( hkey, NULL, progid->Description );

        if (progid->IconPath)
            msi_reg_set_subkey_val( hkey, L"DefaultIcon", NULL, progid->IconPath );

        /* write out the current version */
        if (progid->CurVer)
            msi_reg_set_subkey_val( hkey, L"CurVer", NULL, progid->CurVer->ProgID );

        RegCloseKey(hkey);
    }
    else
        ERR("failed to create key %s\n", debugstr_w( progid->ProgID ) );

    return rc;
}

static const MSICLASS *get_progid_class( const MSIPROGID *progid )
{
    while (progid)
    {
        if (progid->Parent) progid = progid->Parent;
        if (progid->Class) return progid->Class;
        if (!progid->Parent || progid->Parent == progid) break;
    }
    return NULL;
}

static BOOL has_class_installed( const MSIPROGID *progid )
{
    const MSICLASS *class = get_progid_class( progid );
    if (!class || !class->ProgID) return FALSE;
    return (class->action == INSTALLSTATE_LOCAL);
}

static BOOL has_one_extension_installed( const MSIPACKAGE *package, const MSIPROGID *progid )
{
    const MSIEXTENSION *extension;
    LIST_FOR_EACH_ENTRY( extension, &package->extensions, MSIEXTENSION, entry )
    {
        if (extension->ProgID == progid && !list_empty( &extension->verbs ) &&
            extension->action == INSTALLSTATE_LOCAL) return TRUE;
    }
    return FALSE;
}

UINT ACTION_RegisterProgIdInfo(MSIPACKAGE *package)
{
    MSIPROGID *progid;
    MSIRECORD *uirow;
    UINT r;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action( package, SCRIPT_INSTALL, L"RegisterProgIdInfo" );

    r = load_classes_and_such( package );
    if (r != ERROR_SUCCESS)
        return r;

    LIST_FOR_EACH_ENTRY( progid, &package->progids, MSIPROGID, entry )
    {
        if (!has_class_installed( progid ) && !has_one_extension_installed( package, progid ))
        {
            TRACE("progid %s not scheduled to be installed\n", debugstr_w(progid->ProgID));
            continue;
        }
        TRACE("Registering progid %s\n", debugstr_w(progid->ProgID));

        register_progid( progid );

        uirow = MSI_CreateRecord( 1 );
        MSI_RecordSetStringW( uirow, 1, progid->ProgID );
        MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
        msiobj_release( &uirow->hdr );
    }
    return ERROR_SUCCESS;
}

static BOOL has_class_removed( const MSIPROGID *progid )
{
    const MSICLASS *class = get_progid_class( progid );
    if (!class || !class->ProgID) return FALSE;
    return (class->action == INSTALLSTATE_ABSENT);
}

static BOOL has_extensions( const MSIPACKAGE *package, const MSIPROGID *progid )
{
    const MSIEXTENSION *extension;
    LIST_FOR_EACH_ENTRY( extension, &package->extensions, MSIEXTENSION, entry )
    {
        if (extension->ProgID == progid && !list_empty( &extension->verbs )) return TRUE;
    }
    return FALSE;
}

static BOOL has_all_extensions_removed( const MSIPACKAGE *package, const MSIPROGID *progid )
{
    BOOL ret = FALSE;
    const MSIEXTENSION *extension;
    LIST_FOR_EACH_ENTRY( extension, &package->extensions, MSIEXTENSION, entry )
    {
        if (extension->ProgID == progid && !list_empty( &extension->verbs ) &&
            extension->action == INSTALLSTATE_ABSENT) ret = TRUE;
        else ret = FALSE;
    }
    return ret;
}

UINT ACTION_UnregisterProgIdInfo( MSIPACKAGE *package )
{
    MSIPROGID *progid;
    MSIRECORD *uirow;
    LONG res;
    UINT r;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action( package, SCRIPT_INSTALL, L"UnregisterProgIdInfo" );

    r = load_classes_and_such( package );
    if (r != ERROR_SUCCESS)
        return r;

    LIST_FOR_EACH_ENTRY( progid, &package->progids, MSIPROGID, entry )
    {
        if (!has_class_removed( progid ) ||
            (has_extensions( package, progid ) && !has_all_extensions_removed( package, progid )))
        {
            TRACE("progid %s not scheduled to be removed\n", debugstr_w(progid->ProgID));
            continue;
        }
        TRACE("Unregistering progid %s\n", debugstr_w(progid->ProgID));

        res = RegDeleteTreeW( HKEY_CLASSES_ROOT, progid->ProgID );
        if (res != ERROR_SUCCESS)
            TRACE("failed to delete progid key %ld\n", res);

        uirow = MSI_CreateRecord( 1 );
        MSI_RecordSetStringW( uirow, 1, progid->ProgID );
        MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
        msiobj_release( &uirow->hdr );
    }
    return ERROR_SUCCESS;
}

static UINT register_verb(MSIPACKAGE *package, LPCWSTR progid,
                MSICOMPONENT* component, const MSIEXTENSION* extension,
                MSIVERB* verb, INT* Sequence )
{
    LPWSTR keyname;
    HKEY key;
    LPWSTR command;
    DWORD size;
    LPWSTR advertise;

    keyname = msi_build_directory_name(4, progid, L"shell", verb->Verb, L"command");

    TRACE("Making Key %s\n",debugstr_w(keyname));
    RegCreateKeyW(HKEY_CLASSES_ROOT, keyname, &key);
    size = lstrlenW(component->FullKeypath);
    if (verb->Argument)
        size += lstrlenW(verb->Argument);
     size += 4;

     command = malloc(size * sizeof(WCHAR));
     if (verb->Argument)
         swprintf(command, size, L"\"%s\" %s", component->FullKeypath, verb->Argument);
     else
         swprintf(command, size, L"\"%s\"", component->FullKeypath);

     msi_reg_set_val_str( key, NULL, command );
     free(command);

     advertise = msi_create_component_advertise_string(package, component,
                                                       extension->Feature->Feature);
     size = lstrlenW(advertise);

     if (verb->Argument)
         size += lstrlenW(verb->Argument);
     size += 4;

     command = calloc(size, sizeof(WCHAR));

     lstrcpyW(command,advertise);
     if (verb->Argument)
     {
         lstrcatW(command, L" ");
         lstrcatW(command, verb->Argument);
     }

     msi_reg_set_val_multi_str( key, L"command", command );

     RegCloseKey(key);
     free(keyname);
     free(advertise);
     free(command);

     if (verb->Command)
     {
        keyname = msi_build_directory_name( 3, progid, L"shell", verb->Verb );
        msi_reg_set_subkey_val( HKEY_CLASSES_ROOT, keyname, NULL, verb->Command );
        free(keyname);
     }

     if (verb->Sequence != MSI_NULL_INTEGER)
     {
        if (*Sequence == MSI_NULL_INTEGER || verb->Sequence < *Sequence)
        {
            *Sequence = verb->Sequence;
            keyname = msi_build_directory_name( 2, progid, L"shell" );
            msi_reg_set_subkey_val( HKEY_CLASSES_ROOT, keyname, NULL, verb->Verb );
            free(keyname);
        }
    }
    return ERROR_SUCCESS;
}

UINT ACTION_RegisterExtensionInfo(MSIPACKAGE *package)
{
    HKEY hkey = NULL;
    MSIEXTENSION *ext;
    MSIRECORD *uirow;
    BOOL install_on_demand = TRUE;
    LONG res;
    UINT r;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action( package, SCRIPT_INSTALL, L"RegisterExtensionInfo" );

    r = load_classes_and_such( package );
    if (r != ERROR_SUCCESS)
        return r;

    /* We need to set install_on_demand based on if the shell handles advertised
     * shortcuts and the like. Because Mike McCormack is working on this i am
     * going to default to TRUE
     */

    LIST_FOR_EACH_ENTRY( ext, &package->extensions, MSIEXTENSION, entry )
    {
        LPWSTR extension;
        MSIFEATURE *feature;

        if (!ext->Component)
            continue;

        if (!ext->Component->Enabled)
        {
            TRACE("component is disabled\n");
            continue;
        }

        feature = ext->Feature;
        if (!feature)
            continue;

        /*
         * yes. MSDN says that these are based on _Feature_ not on
         * Component.  So verify the feature is to be installed
         */
        feature->Action = msi_get_feature_action( package, feature );
        if (feature->Action != INSTALLSTATE_LOCAL &&
            !(install_on_demand && feature->Action == INSTALLSTATE_ADVERTISED))
        {
            TRACE("feature %s not scheduled for installation, skipping registration of extension %s\n",
                  debugstr_w(feature->Feature), debugstr_w(ext->Extension));
            continue;
        }
        TRACE("Registering extension %s (%p)\n", debugstr_w(ext->Extension), ext);

        ext->action = INSTALLSTATE_LOCAL;

        extension = malloc( (wcslen( ext->Extension ) + 2) * sizeof(WCHAR) );
        if (extension)
        {
            extension[0] = '.';
            lstrcpyW( extension + 1, ext->Extension );
            res = RegCreateKeyW( HKEY_CLASSES_ROOT, extension, &hkey );
            free( extension );
            if (res != ERROR_SUCCESS)
                WARN("failed to create extension key %ld\n", res);
        }

        if (ext->Mime)
            msi_reg_set_val_str( hkey, L"Content Type", ext->Mime->ContentType );

        if (ext->ProgID || ext->ProgIDText)
        {
            HKEY hkey2;
            LPWSTR newkey;
            LPCWSTR progid;
            MSIVERB *verb;
            INT Sequence = MSI_NULL_INTEGER;

            if (ext->ProgID)
                progid = ext->ProgID->ProgID;
            else
                progid = ext->ProgIDText;

            msi_reg_set_val_str( hkey, NULL, progid );

            newkey = malloc( wcslen(progid) * sizeof(WCHAR) + sizeof(L"\\ShellNew") );

            lstrcpyW(newkey, progid);
            lstrcatW(newkey, L"\\ShellNew");
            RegCreateKeyW(hkey, newkey, &hkey2);
            RegCloseKey(hkey2);

            free(newkey);

            /* do all the verbs */
            LIST_FOR_EACH_ENTRY( verb, &ext->verbs, MSIVERB, entry )
            {
                register_verb( package, progid, ext->Component,
                               ext, verb, &Sequence);
            }
        }

        RegCloseKey(hkey);

        uirow = MSI_CreateRecord(1);
        MSI_RecordSetStringW( uirow, 1, ext->Extension );
        MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
        msiobj_release(&uirow->hdr);
    }
    return ERROR_SUCCESS;
}

UINT ACTION_UnregisterExtensionInfo( MSIPACKAGE *package )
{
    MSIEXTENSION *ext;
    MSIRECORD *uirow;
    LONG res;
    UINT r;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action( package, SCRIPT_INSTALL, L"UnregisterExtensionInfo" );

    r = load_classes_and_such( package );
    if (r != ERROR_SUCCESS)
        return r;

    LIST_FOR_EACH_ENTRY( ext, &package->extensions, MSIEXTENSION, entry )
    {
        LPWSTR extension;
        MSIFEATURE *feature;

        if (!ext->Component)
            continue;

        if (!ext->Component->Enabled)
        {
            TRACE("component is disabled\n");
            continue;
        }

        feature = ext->Feature;
        if (!feature)
            continue;

        feature->Action = msi_get_feature_action( package, feature );
        if (feature->Action != INSTALLSTATE_ABSENT)
        {
            TRACE("feature %s not scheduled for removal, skipping unregistration of extension %s\n",
                  debugstr_w(feature->Feature), debugstr_w(ext->Extension));
            continue;
        }
        TRACE("Unregistering extension %s\n", debugstr_w(ext->Extension));

        ext->action = INSTALLSTATE_ABSENT;

        extension = malloc( (wcslen( ext->Extension ) + 2) * sizeof(WCHAR) );
        if (extension)
        {
            extension[0] = '.';
            lstrcpyW( extension + 1, ext->Extension );
            res = RegDeleteTreeW( HKEY_CLASSES_ROOT, extension );
            free( extension );
            if (res != ERROR_SUCCESS)
                WARN("failed to delete extension key %ld\n", res);
        }

        if (ext->ProgID || ext->ProgIDText)
        {
            LPCWSTR progid;
            LPWSTR progid_shell;

            if (ext->ProgID)
                progid = ext->ProgID->ProgID;
            else
                progid = ext->ProgIDText;

            progid_shell = malloc( wcslen( progid ) * sizeof(WCHAR) + sizeof( L"\\shell" ) );
            if (progid_shell)
            {
                lstrcpyW( progid_shell, progid );
                lstrcatW( progid_shell, L"\\shell" );
                res = RegDeleteTreeW( HKEY_CLASSES_ROOT, progid_shell );
                free( progid_shell );
                if (res != ERROR_SUCCESS)
                    WARN("failed to delete shell key %ld\n", res);
                RegDeleteKeyW( HKEY_CLASSES_ROOT, progid );
            }
        }

        uirow = MSI_CreateRecord( 1 );
        MSI_RecordSetStringW( uirow, 1, ext->Extension );
        MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
        msiobj_release( &uirow->hdr );
    }
    return ERROR_SUCCESS;
}

UINT ACTION_RegisterMIMEInfo(MSIPACKAGE *package)
{
    MSIRECORD *uirow;
    MSIMIME *mt;
    UINT r;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action( package, SCRIPT_INSTALL, L"RegisterMIMEInfo" );

    r = load_classes_and_such( package );
    if (r != ERROR_SUCCESS)
        return r;

    LIST_FOR_EACH_ENTRY( mt, &package->mimes, MSIMIME, entry )
    {
        LPWSTR extension = NULL, key;

        /*
         * check if the MIME is to be installed. Either as requested by an
         * extension or Class
         */
        if ((!mt->Class || mt->Class->action != INSTALLSTATE_LOCAL) &&
            (!mt->Extension || mt->Extension->action != INSTALLSTATE_LOCAL))
        {
            TRACE("MIME %s not scheduled to be installed\n", debugstr_w(mt->ContentType));
            continue;
        }

        TRACE("Registering MIME type %s\n", debugstr_w(mt->ContentType));

        if (mt->Extension) extension = malloc( (wcslen( mt->Extension->Extension ) + 2) * sizeof(WCHAR) );
        key = malloc( sizeof( L"MIME\\Database\\Content Type\\" ) +
                      wcslen( mt->ContentType ) * sizeof(WCHAR) );

        if (extension && key)
        {
            extension[0] = '.';
            lstrcpyW( extension + 1, mt->Extension->Extension );

            lstrcpyW( key, L"MIME\\Database\\Content Type\\" );
            lstrcatW( key, mt->ContentType );
            msi_reg_set_subkey_val( HKEY_CLASSES_ROOT, key, L"Extension", extension );

            if (mt->clsid)
                msi_reg_set_subkey_val( HKEY_CLASSES_ROOT, key, L"CLSID", mt->clsid );
        }
        free( extension );
        free( key );

        uirow = MSI_CreateRecord( 2 );
        MSI_RecordSetStringW( uirow, 1, mt->ContentType );
        MSI_RecordSetStringW( uirow, 2, mt->suffix );
        MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
        msiobj_release( &uirow->hdr );
    }
    return ERROR_SUCCESS;
}

UINT ACTION_UnregisterMIMEInfo( MSIPACKAGE *package )
{
    MSIRECORD *uirow;
    MSIMIME *mime;
    UINT r;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action( package, SCRIPT_INSTALL, L"UnregisterMIMEInfo" );

    r = load_classes_and_such( package );
    if (r != ERROR_SUCCESS)
        return r;

    LIST_FOR_EACH_ENTRY( mime, &package->mimes, MSIMIME, entry )
    {
        LONG res;
        LPWSTR mime_key;

        if ((!mime->Class || mime->Class->action != INSTALLSTATE_ABSENT) &&
            (!mime->Extension || mime->Extension->action != INSTALLSTATE_ABSENT))
        {
            TRACE("MIME %s not scheduled to be removed\n", debugstr_w(mime->ContentType));
            continue;
        }

        TRACE("Unregistering MIME type %s\n", debugstr_w(mime->ContentType));

        mime_key = malloc( sizeof( L"MIME\\Database\\Content Type\\" ) +
                           wcslen( mime->ContentType ) * sizeof(WCHAR) );
        if (mime_key)
        {
            lstrcpyW( mime_key, L"MIME\\Database\\Content Type\\" );
            lstrcatW( mime_key, mime->ContentType );
            res = RegDeleteKeyW( HKEY_CLASSES_ROOT, mime_key );
            if (res != ERROR_SUCCESS)
                WARN("failed to delete MIME key %ld\n", res);
            free( mime_key );
        }

        uirow = MSI_CreateRecord( 2 );
        MSI_RecordSetStringW( uirow, 1, mime->ContentType );
        MSI_RecordSetStringW( uirow, 2, mime->suffix );
        MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
        msiobj_release( &uirow->hdr );
    }
    return ERROR_SUCCESS;
}
