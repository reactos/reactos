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

/* actions handled in this module
 * RegisterClassInfo
 * RegisterProgIdInfo
 * RegisterExtensionInfo
 * RegisterMIMEInfo
 * UnRegisterClassInfo (TODO)
 * UnRegisterProgIdInfo (TODO)
 * UnRegisterExtensionInfo (TODO)
 * UnRegisterMIMEInfo (TODO)
 */

#include <stdarg.h>

#include "stdio.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "wine/debug.h"
#include "msipriv.h"
#include "winuser.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);


extern const WCHAR szRegisterClassInfo[];
extern const WCHAR szRegisterProgIdInfo[];
extern const WCHAR szRegisterExtensionInfo[];
extern const WCHAR szRegisterMIMEInfo[];

extern const WCHAR szUnregisterClassInfo[];
extern const WCHAR szUnregisterExtensionInfo[];
extern const WCHAR szUnregisterMIMEInfo[];
extern const WCHAR szUnregisterProgIdInfo[];

static MSIAPPID *load_appid( MSIPACKAGE* package, MSIRECORD *row )
{
    LPCWSTR buffer;
    MSIAPPID *appid;

    /* fill in the data */

    appid = msi_alloc_zero( sizeof(MSIAPPID) );
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
    static const WCHAR ExecSeqQuery[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','A','p','p','I','d','`',' ','W','H','E','R','E',' ',
         '`','A','p','p','I','d','`',' ','=',' ','\'','%','s','\'',0};

    if (!name)
        return NULL;

    /* check for appids already loaded */
    LIST_FOR_EACH_ENTRY( appid, &package->appids, MSIAPPID, entry )
    {
        if (lstrcmpiW( appid->AppID, name )==0)
        {
            TRACE("found appid %s %p\n", debugstr_w(name), appid);
            return appid;
        }
    }
    
    row = MSI_QueryGetRecord(package->db, ExecSeqQuery, name);
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

    progid = msi_alloc_zero( sizeof(MSIPROGID) );
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
        static const WCHAR fmt[] = {'%','s',',','%','i',0};

        FilePath = build_icon_path(package,FileName);
       
        progid->IconPath = msi_alloc( (strlenW(FilePath)+10)* sizeof(WCHAR) );

        sprintfW(progid->IconPath,fmt,FilePath,icon_index);

        msi_free(FilePath);
    }
    else
    {
        buffer = MSI_RecordGetString(row,5);
        if (buffer)
            progid->IconPath = build_icon_path(package,buffer);
    }

    progid->CurVer = NULL;
    progid->VersionInd = NULL;

    /* if we have a parent then we may be that parents CurVer */
    if (progid->Parent && progid->Parent != progid)
    {
        MSIPROGID *parent = progid->Parent;

        while (parent->Parent && parent->Parent != parent)
            parent = parent->Parent;

        /* FIXME: need to determing if we are really the CurVer */

        progid->CurVer = parent;
        parent->VersionInd = progid;
    }
    
    return progid;
}

static MSIPROGID *load_given_progid(MSIPACKAGE *package, LPCWSTR name)
{
    MSIPROGID *progid;
    MSIRECORD *row;
    static const WCHAR ExecSeqQuery[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','P','r','o','g','I','d','`',' ','W','H','E','R','E',' ',
         '`','P','r','o','g','I','d','`',' ','=',' ','\'','%','s','\'',0};

    if (!name)
        return NULL;

    /* check for progids already loaded */
    LIST_FOR_EACH_ENTRY( progid, &package->progids, MSIPROGID, entry )
    {
        if (strcmpiW( progid->ProgID,name )==0)
        {
            TRACE("found progid %s (%p)\n",debugstr_w(name), progid );
            return progid;
        }
    }
    
    row = MSI_QueryGetRecord( package->db, ExecSeqQuery, name );
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

    cls = msi_alloc_zero( sizeof(MSICLASS) );
    if (!cls)
        return NULL;

    list_add_tail( &package->classes, &cls->entry );

    cls->clsid = msi_dup_record_field( row, 1 );
    TRACE("loading class %s\n",debugstr_w(cls->clsid));
    cls->Context = msi_dup_record_field( row, 2 );
    buffer = MSI_RecordGetString(row,3);
    cls->Component = get_loaded_component(package, buffer);

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
        static const WCHAR fmt[] = {'%','s',',','%','i',0};

        FilePath = build_icon_path(package,FileName);
       
        cls->IconPath = msi_alloc( (strlenW(FilePath)+5)* sizeof(WCHAR) );

        sprintfW(cls->IconPath,fmt,FilePath,icon_index);

        msi_free(FilePath);
    }
    else
    {
        buffer = MSI_RecordGetString(row,8);
        if (buffer)
            cls->IconPath = build_icon_path(package,buffer);
    }

    if (!MSI_RecordIsNull(row,10))
    {
        i = MSI_RecordGetInteger(row,10);
        if (i != MSI_NULL_INTEGER && i > 0 &&  i < 4)
        {
            static const WCHAR ole2[] = {'o','l','e','2','.','d','l','l',0};
            static const WCHAR ole32[] = {'o','l','e','3','2','.','d','l','l',0};

            switch(i)
            {
                case 1:
                    cls->DefInprocHandler = strdupW(ole2);
                    break;
                case 2:
                    cls->DefInprocHandler32 = strdupW(ole32);
                    break;
                case 3:
                    cls->DefInprocHandler = strdupW(ole2);
                    cls->DefInprocHandler32 = strdupW(ole32);
                    break;
            }
        }
        else
        {
            cls->DefInprocHandler32 = msi_dup_record_field( row, 10);
            reduce_to_longfilename(cls->DefInprocHandler32);
        }
    }
    buffer = MSI_RecordGetString(row,11);
    deformat_string(package,buffer,&cls->Argument);

    buffer = MSI_RecordGetString(row,12);
    cls->Feature = get_loaded_feature(package,buffer);

    cls->Attributes = MSI_RecordGetInteger(row,13);
    
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
    static const WCHAR ExecSeqQuery[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','C','l','a','s','s','`',' ','W','H','E','R','E',' ',
         '`','C','L','S','I','D','`',' ','=',' ','\'','%','s','\'',0};

    if (!classid)
        return NULL;
    
    /* check for classes already loaded */
    LIST_FOR_EACH_ENTRY( cls, &package->classes, MSICLASS, entry )
    {
        if (lstrcmpiW( cls->clsid, classid )==0)
        {
            TRACE("found class %s (%p)\n",debugstr_w(classid), cls);
            return cls;
        }
    }

    row = MSI_QueryGetRecord(package->db, ExecSeqQuery, classid);
    if (!row)
        return NULL;

    cls = load_class(package, row);
    msiobj_release(&row->hdr);

    return cls;
}

static MSIEXTENSION *load_given_extension( MSIPACKAGE *package, LPCWSTR extension );

static MSIMIME *load_mime( MSIPACKAGE* package, MSIRECORD *row )
{
    LPCWSTR buffer;
    MSIMIME *mt;

    /* fill in the data */

    mt = msi_alloc_zero( sizeof(MSIMIME) );
    if (!mt)
        return mt;

    mt->ContentType = msi_dup_record_field( row, 1 ); 
    TRACE("loading mime %s\n", debugstr_w(mt->ContentType));

    buffer = MSI_RecordGetString( row, 2 );
    mt->Extension = load_given_extension( package, buffer );

    mt->clsid = msi_dup_record_field( row, 3 );
    mt->Class = load_given_class( package, mt->clsid );

    list_add_tail( &package->mimes, &mt->entry );

    return mt;
}

static MSIMIME *load_given_mime( MSIPACKAGE *package, LPCWSTR mime )
{
    MSIRECORD *row;
    static const WCHAR ExecSeqQuery[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','M','I','M','E','`',' ','W','H','E','R','E',' ',
         '`','C','o','n','t','e','n','t','T','y','p','e','`',' ','=',' ',
         '\'','%','s','\'',0};
    MSIMIME *mt;

    if (!mime)
        return NULL;
    
    /* check for mime already loaded */
    LIST_FOR_EACH_ENTRY( mt, &package->mimes, MSIMIME, entry )
    {
        if (strcmpiW(mt->ContentType,mime)==0)
        {
            TRACE("found mime %s (%p)\n",debugstr_w(mime), mt);
            return mt;
        }
    }
    
    row = MSI_QueryGetRecord(package->db, ExecSeqQuery, mime);
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

    ext = msi_alloc_zero( sizeof(MSIEXTENSION) );
    if (!ext)
        return NULL;

    list_init( &ext->verbs );

    list_add_tail( &package->extensions, &ext->entry );

    ext->Extension = msi_dup_record_field( row, 1 );
    TRACE("loading extension %s\n", debugstr_w(ext->Extension));

    buffer = MSI_RecordGetString( row, 2 );
    ext->Component = get_loaded_component( package,buffer );

    ext->ProgIDText = msi_dup_record_field( row, 3 );
    ext->ProgID = load_given_progid( package, ext->ProgIDText );

    buffer = MSI_RecordGetString( row, 4 );
    ext->Mime = load_given_mime( package, buffer );

    buffer = MSI_RecordGetString(row,5);
    ext->Feature = get_loaded_feature( package, buffer );

    return ext;
}

/*
 * While the extension table has 2 primary keys, this function is only looking
 * at the Extension key which is what is referenced as a forign key 
 */
static MSIEXTENSION *load_given_extension( MSIPACKAGE *package, LPCWSTR name )
{
    MSIRECORD *row;
    MSIEXTENSION *ext;
    static const WCHAR ExecSeqQuery[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','E','x','t','e','n','s','i','o','n','`',' ',
         'W','H','E','R','E',' ',
         '`','E','x','t','e','n','s','i','o','n','`',' ','=',' ',
         '\'','%','s','\'',0};

    if (!name)
        return NULL;

    /* check for extensions already loaded */
    LIST_FOR_EACH_ENTRY( ext, &package->extensions, MSIEXTENSION, entry )
    {
        if (strcmpiW( ext->Extension, name )==0)
        {
            TRACE("extension %s already loaded %p\n", debugstr_w(name), ext);
            return ext;
        }
    }
    
    row = MSI_QueryGetRecord( package->db, ExecSeqQuery, name );
    if (!row)
        return NULL;

    ext = load_extension(package, row);
    msiobj_release(&row->hdr);

    return ext;
}

static UINT iterate_load_verb(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE* package = (MSIPACKAGE*)param;
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

    verb = msi_alloc_zero( sizeof(MSIVERB) );
    if (!verb)
        return ERROR_OUTOFMEMORY;

    verb->Verb = msi_dup_record_field(row,2);
    TRACE("loading verb %s\n",debugstr_w(verb->Verb));
    verb->Sequence = MSI_RecordGetInteger(row,3);

    buffer = MSI_RecordGetString(row,4);
    deformat_string(package,buffer,&verb->Command);

    buffer = MSI_RecordGetString(row,5);
    deformat_string(package,buffer,&verb->Argument);

    /* assosiate the verb with the correct extension */
    list_add_tail( &extension->verbs, &verb->entry );
    
    return ERROR_SUCCESS;
}

static UINT iterate_all_classes(MSIRECORD *rec, LPVOID param)
{
    MSICOMPONENT *comp;
    LPCWSTR clsid;
    LPCWSTR context;
    LPCWSTR buffer;
    MSIPACKAGE* package =(MSIPACKAGE*)param;
    MSICLASS *cls;
    BOOL match = FALSE;

    clsid = MSI_RecordGetString(rec,1);
    context = MSI_RecordGetString(rec,2);
    buffer = MSI_RecordGetString(rec,3);
    comp = get_loaded_component(package,buffer);

    LIST_FOR_EACH_ENTRY( cls, &package->classes, MSICLASS, entry )
    {
        if (strcmpiW( clsid, cls->clsid ))
            continue;
        if (strcmpW( context, cls->Context ))
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

static VOID load_all_classes(MSIPACKAGE *package)
{
    UINT rc = ERROR_SUCCESS;
    MSIQUERY *view;

    static const WCHAR ExecSeqQuery[] =
        {'S','E','L','E','C','T',' ','*',' ', 'F','R','O','M',' ',
         '`','C','l','a','s','s','`',0};

    rc = MSI_DatabaseOpenViewW(package->db, ExecSeqQuery, &view);
    if (rc != ERROR_SUCCESS)
        return;

    rc = MSI_IterateRecords(view, NULL, iterate_all_classes, package);
    msiobj_release(&view->hdr);
}

static UINT iterate_all_extensions(MSIRECORD *rec, LPVOID param)
{
    MSICOMPONENT *comp;
    LPCWSTR buffer;
    LPCWSTR extension;
    MSIPACKAGE* package =(MSIPACKAGE*)param;
    BOOL match = FALSE;
    MSIEXTENSION *ext;

    extension = MSI_RecordGetString(rec,1);
    buffer = MSI_RecordGetString(rec,2);
    comp = get_loaded_component(package,buffer);

    LIST_FOR_EACH_ENTRY( ext, &package->extensions, MSIEXTENSION, entry )
    {
        if (strcmpiW(extension,ext->Extension))
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

static VOID load_all_extensions(MSIPACKAGE *package)
{
    UINT rc = ERROR_SUCCESS;
    MSIQUERY *view;

    static const WCHAR ExecSeqQuery[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','E','x','t','e','n','s','i','o','n','`',0};

    rc = MSI_DatabaseOpenViewW(package->db, ExecSeqQuery, &view);
    if (rc != ERROR_SUCCESS)
        return;

    rc = MSI_IterateRecords(view, NULL, iterate_all_extensions, package);
    msiobj_release(&view->hdr);
}

static UINT iterate_all_progids(MSIRECORD *rec, LPVOID param)
{
    LPCWSTR buffer;
    MSIPACKAGE* package =(MSIPACKAGE*)param;

    buffer = MSI_RecordGetString(rec,1);
    load_given_progid(package,buffer);
    return ERROR_SUCCESS;
}

static VOID load_all_progids(MSIPACKAGE *package)
{
    UINT rc = ERROR_SUCCESS;
    MSIQUERY *view;

    static const WCHAR ExecSeqQuery[] =
        {'S','E','L','E','C','T',' ','`','P','r','o','g','I','d','`',' ',
         'F','R','O','M',' ', '`','P','r','o','g','I','d','`',0};

    rc = MSI_DatabaseOpenViewW(package->db, ExecSeqQuery, &view);
    if (rc != ERROR_SUCCESS)
        return;

    rc = MSI_IterateRecords(view, NULL, iterate_all_progids, package);
    msiobj_release(&view->hdr);
}

static VOID load_all_verbs(MSIPACKAGE *package)
{
    UINT rc = ERROR_SUCCESS;
    MSIQUERY *view;

    static const WCHAR ExecSeqQuery[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','V','e','r','b','`',0};

    rc = MSI_DatabaseOpenViewW(package->db, ExecSeqQuery, &view);
    if (rc != ERROR_SUCCESS)
        return;

    rc = MSI_IterateRecords(view, NULL, iterate_load_verb, package);
    msiobj_release(&view->hdr);
}

static UINT iterate_all_mimes(MSIRECORD *rec, LPVOID param)
{
    LPCWSTR buffer;
    MSIPACKAGE* package =(MSIPACKAGE*)param;

    buffer = MSI_RecordGetString(rec,1);
    load_given_mime(package,buffer);
    return ERROR_SUCCESS;
}

static VOID load_all_mimes(MSIPACKAGE *package)
{
    UINT rc = ERROR_SUCCESS;
    MSIQUERY *view;

    static const WCHAR ExecSeqQuery[] =
        {'S','E','L','E','C','T',' ',
         '`','C','o','n','t','e','n','t','T','y','p','e','`',
         ' ','F','R','O','M',' ',
         '`','M','I','M','E','`',0};

    rc = MSI_DatabaseOpenViewW(package->db, ExecSeqQuery, &view);
    if (rc != ERROR_SUCCESS)
        return;

    rc = MSI_IterateRecords(view, NULL, iterate_all_mimes, package);
    msiobj_release(&view->hdr);
}

static void load_classes_and_such(MSIPACKAGE *package)
{
    TRACE("Loading all the class info and related tables\n");

    /* check if already loaded */
    if (!list_empty( &package->classes ) ||
        !list_empty( &package->mimes ) ||
        !list_empty( &package->extensions ) ||
        !list_empty( &package->progids ) )
        return;

    load_all_classes(package);
    load_all_extensions(package);
    load_all_progids(package);
    /* these loads must come after the other loads */
    load_all_verbs(package);
    load_all_mimes(package);
}

static void mark_progid_for_install( MSIPACKAGE* package, MSIPROGID *progid )
{
    MSIPROGID *child;

    if (!progid)
        return;

    if (progid->InstallMe)
        return;

    progid->InstallMe = TRUE;

    /* all children if this is a parent also install */
    LIST_FOR_EACH_ENTRY( child, &package->progids, MSIPROGID, entry )
    {
        if (child->Parent == progid)
            mark_progid_for_install( package, child );
    }
}

static void mark_mime_for_install( MSIMIME *mime )
{
    if (!mime)
        return;
    mime->InstallMe = TRUE;
}

static UINT register_appid(MSIAPPID *appid, LPCWSTR app )
{
    static const WCHAR szAppID[] = { 'A','p','p','I','D',0 };
    static const WCHAR szRemoteServerName[] =
         {'R','e','m','o','t','e','S','e','r','v','e','r','N','a','m','e',0};
    static const WCHAR szLocalService[] =
         {'L','o','c','a','l','S','e','r','v','i','c','e',0};
    static const WCHAR szService[] =
         {'S','e','r','v','i','c','e','P','a','r','a','m','e','t','e','r','s',0};
    static const WCHAR szDLL[] =
         {'D','l','l','S','u','r','r','o','g','a','t','e',0};
    static const WCHAR szActivate[] =
         {'A','c','t','i','v','a','t','e','A','s','S','t','o','r','a','g','e',0};
    static const WCHAR szY[] = {'Y',0};
    static const WCHAR szRunAs[] = {'R','u','n','A','s',0};
    static const WCHAR szUser[] = 
         {'I','n','t','e','r','a','c','t','i','v','e',' ','U','s','e','r',0};

    HKEY hkey2,hkey3;

    RegCreateKeyW(HKEY_CLASSES_ROOT,szAppID,&hkey2);
    RegCreateKeyW( hkey2, appid->AppID, &hkey3 );
    RegCloseKey(hkey2);
    msi_reg_set_val_str( hkey3, NULL, app );

    if (appid->RemoteServerName)
        msi_reg_set_val_str( hkey3, szRemoteServerName, appid->RemoteServerName );

    if (appid->LocalServer)
        msi_reg_set_val_str( hkey3, szLocalService, appid->LocalServer );

    if (appid->ServiceParameters)
        msi_reg_set_val_str( hkey3, szService, appid->ServiceParameters );

    if (appid->DllSurrogate)
        msi_reg_set_val_str( hkey3, szDLL, appid->DllSurrogate );

    if (appid->ActivateAtStorage)
        msi_reg_set_val_str( hkey3, szActivate, szY );

    if (appid->RunAsInteractiveUser)
        msi_reg_set_val_str( hkey3, szRunAs, szUser );

    RegCloseKey(hkey3);
    return ERROR_SUCCESS;
}

UINT ACTION_RegisterClassInfo(MSIPACKAGE *package)
{
    /* 
     * Again I am assuming the words, "Whose key file represents" when referring
     * to a Component as to meaning that Components KeyPath file
     */
    
    UINT rc;
    MSIRECORD *uirow;
    static const WCHAR szCLSID[] = { 'C','L','S','I','D',0 };
    static const WCHAR szProgID[] = { 'P','r','o','g','I','D',0 };
    static const WCHAR szVIProgID[] = { 'V','e','r','s','i','o','n','I','n','d','e','p','e','n','d','e','n','t','P','r','o','g','I','D',0 };
    static const WCHAR szAppID[] = { 'A','p','p','I','D',0 };
    static const WCHAR szSpace[] = {' ',0};
    static const WCHAR szFileType_fmt[] = {'F','i','l','e','T','y','p','e','\\','%','s','\\','%','i',0};
    HKEY hkey,hkey2,hkey3;
    MSICLASS *cls;

    load_classes_and_such(package);
    rc = RegCreateKeyW(HKEY_CLASSES_ROOT,szCLSID,&hkey);
    if (rc != ERROR_SUCCESS)
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

        feature = cls->Feature;

        /*
         * MSDN says that these are based on Feature not on Component.
         */
        if (!ACTION_VerifyFeatureForAction( feature, INSTALLSTATE_LOCAL ) &&
            !ACTION_VerifyFeatureForAction( feature, INSTALLSTATE_ADVERTISED ))
        {
            TRACE("Skipping class %s reg due to disabled feature %s\n",
                  debugstr_w(cls->clsid), debugstr_w(feature->Feature));

            continue;
        }

        TRACE("Registering class %s (%p)\n", debugstr_w(cls->clsid), cls);

        cls->Installed = TRUE;
        mark_progid_for_install( package, cls->ProgID );

        RegCreateKeyW( hkey, cls->clsid, &hkey2 );

        if (cls->Description)
            msi_reg_set_val_str( hkey2, NULL, cls->Description );

        RegCreateKeyW( hkey2, cls->Context, &hkey3 );
        file = get_loaded_file( package, comp->KeyPath );
        if (!file)
        {
            TRACE("COM server not provided, skipping class %s\n", debugstr_w(cls->clsid));
            continue;
        }

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

        argument = msi_alloc( size * sizeof(WCHAR) );
        lstrcpyW( argument, file->TargetPath );

        if (cls->Argument)
        {
            lstrcatW( argument, szSpace );
            lstrcatW( argument, cls->Argument );
        }

        msi_reg_set_val_str( hkey3, NULL, argument );
        msi_free(argument);

        RegCloseKey(hkey3);

        if (cls->ProgID || cls->ProgIDText)
        {
            LPCWSTR progid;

            if (cls->ProgID)
                progid = cls->ProgID->ProgID;
            else
                progid = cls->ProgIDText;

            msi_reg_set_subkey_val( hkey2, szProgID, NULL, progid );

            if (cls->ProgID && cls->ProgID->VersionInd)
            {
                msi_reg_set_subkey_val( hkey2, szVIProgID, NULL, 
                                        cls->ProgID->VersionInd->ProgID );
            }
        }

        if (cls->AppID)
        {
            MSIAPPID *appid = cls->AppID;

            msi_reg_set_val_str( hkey2, szAppID, appid->AppID );

            register_appid( appid, cls->Description );
        }

        if (cls->IconPath)
        {
            static const WCHAR szDefaultIcon[] = 
                {'D','e','f','a','u','l','t','I','c','o','n',0};

            msi_reg_set_subkey_val( hkey2, szDefaultIcon, NULL, cls->IconPath );
        }

        if (cls->DefInprocHandler)
        {
            static const WCHAR szInproc[] =
                {'I','n','p','r','o','c','H','a','n','d','l','e','r',0};

            msi_reg_set_subkey_val( hkey2, szInproc, NULL, cls->DefInprocHandler );
        }

        if (cls->DefInprocHandler32)
        {
            static const WCHAR szInproc32[] =
                {'I','n','p','r','o','c','H','a','n','d','l','e','r','3','2',0};

            msi_reg_set_subkey_val( hkey2, szInproc32, NULL, cls->DefInprocHandler32 );
        }
        
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
                ptr2 = strchrW(ptr,';');
                if (ptr2)
                    *ptr2 = 0;
                keyname = msi_alloc( (strlenW(szFileType_fmt) + strlenW(cls->clsid) + 4) * sizeof(WCHAR));
                sprintfW( keyname, szFileType_fmt, cls->clsid, index );

                msi_reg_set_subkey_val( HKEY_CLASSES_ROOT, keyname, NULL, ptr );
                msi_free(keyname);

                if (ptr2)
                    ptr = ptr2+1;
                else
                    ptr = NULL;

                index ++;
            }
        }
        
        uirow = MSI_CreateRecord(1);

        MSI_RecordSetStringW( uirow, 1, cls->clsid );
        ui_actiondata(package,szRegisterClassInfo,uirow);
        msiobj_release(&uirow->hdr);
    }

    RegCloseKey(hkey);
    return rc;
}

static LPCWSTR get_clsid_of_progid( MSIPROGID *progid )
{
    while (progid)
    {
        if (progid->Class)
            return progid->Class->clsid;
        progid = progid->Parent;
    }
    return NULL;
}

static UINT register_progid( MSIPROGID* progid )
{
    static const WCHAR szCLSID[] = { 'C','L','S','I','D',0 };
    static const WCHAR szDefaultIcon[] =
        {'D','e','f','a','u','l','t','I','c','o','n',0};
    static const WCHAR szCurVer[] =
        {'C','u','r','V','e','r',0};
    HKEY hkey = 0;
    UINT rc;

    rc = RegCreateKeyW( HKEY_CLASSES_ROOT, progid->ProgID, &hkey );
    if (rc == ERROR_SUCCESS)
    {
        LPCWSTR clsid = get_clsid_of_progid( progid );

        if (clsid)
            msi_reg_set_subkey_val( hkey, szCLSID, NULL, clsid );
        else
            ERR("%s has no class\n", debugstr_w( progid->ProgID ) );

        if (progid->Description)
            msi_reg_set_val_str( hkey, NULL, progid->Description );

        if (progid->IconPath)
            msi_reg_set_subkey_val( hkey, szDefaultIcon, NULL, progid->IconPath );

        /* write out the current version */
        if (progid->CurVer)
            msi_reg_set_subkey_val( hkey, szCurVer, NULL, progid->CurVer->ProgID );

        RegCloseKey(hkey);
    }
    else
        ERR("failed to create key %s\n", debugstr_w( progid->ProgID ) );

    return rc;
}

UINT ACTION_RegisterProgIdInfo(MSIPACKAGE *package)
{
    MSIPROGID *progid;
    MSIRECORD *uirow;

    load_classes_and_such(package);

    LIST_FOR_EACH_ENTRY( progid, &package->progids, MSIPROGID, entry )
    {
        /* check if this progid is to be installed */
        if (progid->Class && progid->Class->Installed)
            progid->InstallMe = TRUE;

        if (!progid->InstallMe)
        {
            TRACE("progid %s not scheduled to be installed\n",
                             debugstr_w(progid->ProgID));
            continue;
        }
       
        TRACE("Registering progid %s\n", debugstr_w(progid->ProgID));

        register_progid( progid );

        uirow = MSI_CreateRecord( 1 );
        MSI_RecordSetStringW( uirow, 1, progid->ProgID );
        ui_actiondata( package, szRegisterProgIdInfo, uirow );
        msiobj_release( &uirow->hdr );
    }

    return ERROR_SUCCESS;
}

static UINT register_verb(MSIPACKAGE *package, LPCWSTR progid, 
                MSICOMPONENT* component, MSIEXTENSION* extension,
                MSIVERB* verb, INT* Sequence )
{
    LPWSTR keyname;
    HKEY key;
    static const WCHAR szShell[] = {'s','h','e','l','l',0};
    static const WCHAR szCommand[] = {'c','o','m','m','a','n','d',0};
    static const WCHAR fmt[] = {'\"','%','s','\"',' ','%','s',0};
    static const WCHAR fmt2[] = {'\"','%','s','\"',0};
    LPWSTR command;
    DWORD size;
    LPWSTR advertise;

    keyname = build_directory_name(4, progid, szShell, verb->Verb, szCommand);

    TRACE("Making Key %s\n",debugstr_w(keyname));
    RegCreateKeyW(HKEY_CLASSES_ROOT, keyname, &key);
    size = strlenW(component->FullKeypath);
    if (verb->Argument)
        size += strlenW(verb->Argument);
     size += 4;

     command = msi_alloc(size * sizeof (WCHAR));
     if (verb->Argument)
        sprintfW(command, fmt, component->FullKeypath, verb->Argument);
     else
        sprintfW(command, fmt2, component->FullKeypath);

     msi_reg_set_val_str( key, NULL, command );
     msi_free(command);

     advertise = create_component_advertise_string(package, component, 
                                                   extension->Feature->Feature);

     size = strlenW(advertise);

     if (verb->Argument)
         size += strlenW(verb->Argument);
     size += 4;

     command = msi_alloc_zero(size * sizeof (WCHAR));

     strcpyW(command,advertise);
     if (verb->Argument)
     {
         static const WCHAR szSpace[] = {' ',0};
         strcatW(command,szSpace);
         strcatW(command,verb->Argument);
     }

     msi_reg_set_val_multi_str( key, szCommand, command );
     
     RegCloseKey(key);
     msi_free(keyname);
     msi_free(advertise);
     msi_free(command);

     if (verb->Command)
     {
        keyname = build_directory_name(3, progid, szShell, verb->Verb);
        msi_reg_set_subkey_val( HKEY_CLASSES_ROOT, keyname, NULL, verb->Command );
        msi_free(keyname);
     }

     if (verb->Sequence != MSI_NULL_INTEGER)
     {
        if (*Sequence == MSI_NULL_INTEGER || verb->Sequence < *Sequence)
        {
            *Sequence = verb->Sequence;
            keyname = build_directory_name(2, progid, szShell);
            msi_reg_set_subkey_val( HKEY_CLASSES_ROOT, keyname, NULL, verb->Verb );
            msi_free(keyname);
        }
    }
    return ERROR_SUCCESS;
}

UINT ACTION_RegisterExtensionInfo(MSIPACKAGE *package)
{
    static const WCHAR szContentType[] = 
        {'C','o','n','t','e','n','t',' ','T','y','p','e',0 };
    HKEY hkey;
    MSIEXTENSION *ext;
    MSIRECORD *uirow;
    BOOL install_on_demand = TRUE;

    load_classes_and_such(package);

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

        feature = ext->Feature;

        /* 
         * yes. MSDN says that these are based on _Feature_ not on
         * Component.  So verify the feature is to be installed
         */
        if ((!ACTION_VerifyFeatureForAction( feature, INSTALLSTATE_LOCAL )) &&
             !(install_on_demand &&
               ACTION_VerifyFeatureForAction( feature, INSTALLSTATE_ADVERTISED )))
        {
            TRACE("Skipping extension %s reg due to disabled feature %s\n",
                   debugstr_w(ext->Extension), debugstr_w(feature->Feature));

            continue;
        }

        TRACE("Registering extension %s (%p)\n", debugstr_w(ext->Extension), ext);

        ext->Installed = TRUE;

        /* this is only registered if the extension has at least 1 verb
         * according to MSDN
         */
        if (ext->ProgID && !list_empty( &ext->verbs ) )
            mark_progid_for_install( package, ext->ProgID );

        mark_mime_for_install(ext->Mime);

        extension = msi_alloc( (lstrlenW( ext->Extension ) + 2)*sizeof(WCHAR) );
        extension[0] = '.';
        lstrcpyW(extension+1,ext->Extension);

        RegCreateKeyW(HKEY_CLASSES_ROOT,extension,&hkey);
        msi_free( extension );

        if (ext->Mime)
            msi_reg_set_val_str( hkey, szContentType, ext->Mime->ContentType );

        if (ext->ProgID || ext->ProgIDText)
        {
            static const WCHAR szSN[] = 
                {'\\','S','h','e','l','l','N','e','w',0};
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

            newkey = msi_alloc( (strlenW(progid)+strlenW(szSN)+1) * sizeof(WCHAR)); 

            strcpyW(newkey,progid);
            strcatW(newkey,szSN);
            RegCreateKeyW(hkey,newkey,&hkey2);
            RegCloseKey(hkey2);

            msi_free(newkey);

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
        ui_actiondata(package,szRegisterExtensionInfo,uirow);
        msiobj_release(&uirow->hdr);
    }

    return ERROR_SUCCESS;
}

UINT ACTION_RegisterMIMEInfo(MSIPACKAGE *package)
{
    static const WCHAR szExten[] = 
        {'E','x','t','e','n','s','i','o','n',0 };
    MSIRECORD *uirow;
    MSIMIME *mt;

    load_classes_and_such(package);

    LIST_FOR_EACH_ENTRY( mt, &package->mimes, MSIMIME, entry )
    {
        LPWSTR extension;
        LPCWSTR exten;
        LPCWSTR mime;
        static const WCHAR fmt[] = 
            {'M','I','M','E','\\','D','a','t','a','b','a','s','e','\\',
             'C','o','n','t','e','n','t',' ','T','y','p','e','\\', '%','s',0};
        LPWSTR key;

        /* 
         * check if the MIME is to be installed. Either as requesed by an
         * extension or Class
         */
        mt->InstallMe = (mt->InstallMe ||
              (mt->Class && mt->Class->Installed) ||
              (mt->Extension && mt->Extension->Installed));

        if (!mt->InstallMe)
        {
            TRACE("MIME %s not scheduled to be installed\n",
                             debugstr_w(mt->ContentType));
            continue;
        }
        
        mime = mt->ContentType;
        exten = mt->Extension->Extension;

        extension = msi_alloc( (lstrlenW( exten ) + 2)*sizeof(WCHAR) );
        extension[0] = '.';
        lstrcpyW(extension+1,exten);

        key = msi_alloc( (strlenW(mime)+strlenW(fmt)+1) * sizeof(WCHAR) );
        sprintfW(key,fmt,mime);
        msi_reg_set_subkey_val( HKEY_CLASSES_ROOT, key, szExten, extension );

        msi_free(extension);
        msi_free(key);

        if (mt->clsid)
            FIXME("Handle non null for field 3\n");

        uirow = MSI_CreateRecord(2);
        MSI_RecordSetStringW(uirow,1,mt->ContentType);
        MSI_RecordSetStringW(uirow,2,exten);
        ui_actiondata(package,szRegisterMIMEInfo,uirow);
        msiobj_release(&uirow->hdr);
    }

    return ERROR_SUCCESS;
}
