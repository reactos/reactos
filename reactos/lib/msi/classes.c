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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "wine/debug.h"
#include "msipriv.h"
#include "winuser.h"
#include "wine/unicode.h"
#include "action.h"

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
    DWORD sz;
    LPCWSTR buffer;
    MSIAPPID *appid;

    /* fill in the data */

    appid = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MSIAPPID) );
    if (!appid)
        return NULL;
    
    sz = IDENTIFIER_SIZE;
    MSI_RecordGetStringW(row, 1, appid->AppID, &sz);
    TRACE("loading appid %s\n", debugstr_w( appid->AppID ));

    buffer = MSI_RecordGetString(row,2);
    deformat_string( package, buffer, &appid->RemoteServerName );

    appid->LocalServer = load_dynamic_stringW(row,3);
    appid->ServiceParameters = load_dynamic_stringW(row,4);
    appid->DllSurrogate = load_dynamic_stringW(row,5);

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

static INT load_given_progid(MSIPACKAGE *package, LPCWSTR progid);
static MSICLASS *load_given_class( MSIPACKAGE *package, LPCWSTR classid );

static INT load_progid(MSIPACKAGE* package, MSIRECORD *row)
{
    DWORD index = package->loaded_progids;
    LPCWSTR buffer;

    /* fill in the data */

    package->loaded_progids++;
    if (package->loaded_progids == 1)
        package->progids = HeapAlloc(GetProcessHeap(),0,sizeof(MSIPROGID));
    else
        package->progids = HeapReAlloc(GetProcessHeap(),0,
            package->progids , package->loaded_progids * sizeof(MSIPROGID));

    memset(&package->progids[index],0,sizeof(MSIPROGID));

    package->progids[index].ProgID = load_dynamic_stringW(row,1);
    TRACE("loading progid %s\n",debugstr_w(package->progids[index].ProgID));

    buffer = MSI_RecordGetString(row,2);
    package->progids[index].ParentIndex = load_given_progid(package,buffer);
    if (package->progids[index].ParentIndex < 0 && buffer)
        FIXME("Unknown parent ProgID %s\n",debugstr_w(buffer));

    buffer = MSI_RecordGetString(row,3);
    package->progids[index].Class = load_given_class(package,buffer);
    if (package->progids[index].Class == NULL && buffer)
        FIXME("Unknown class %s\n",debugstr_w(buffer));

    package->progids[index].Description = load_dynamic_stringW(row,4);

    if (!MSI_RecordIsNull(row,6))
    {
        INT icon_index = MSI_RecordGetInteger(row,6); 
        LPWSTR FileName = load_dynamic_stringW(row,5);
        LPWSTR FilePath;
        static const WCHAR fmt[] = {'%','s',',','%','i',0};

        build_icon_path(package,FileName,&FilePath);
       
        package->progids[index].IconPath = 
                HeapAlloc(GetProcessHeap(),0,(strlenW(FilePath)+10)*
                                sizeof(WCHAR));

        sprintfW(package->progids[index].IconPath,fmt,FilePath,icon_index);

        HeapFree(GetProcessHeap(),0,FilePath);
        HeapFree(GetProcessHeap(),0,FileName);
    }
    else
    {
        buffer = MSI_RecordGetString(row,5);
        if (buffer)
            build_icon_path(package,buffer,&(package->progids[index].IconPath));
    }

    package->progids[index].CurVerIndex = -1;
    package->progids[index].VersionIndIndex = -1;

    /* if we have a parent then we may be that parents CurVer */
    if (package->progids[index].ParentIndex >= 0 && 
        package->progids[index].ParentIndex != index)
    {
        int pindex = package->progids[index].ParentIndex;
        while (package->progids[pindex].ParentIndex>= 0 && 
               package->progids[pindex].ParentIndex != pindex)
            pindex = package->progids[pindex].ParentIndex;

        FIXME("BAD BAD need to determing if we are really the CurVer\n");

        package->progids[index].CurVerIndex = pindex;
        package->progids[pindex].VersionIndIndex = index;
    }
    
    return index;
}

static INT load_given_progid(MSIPACKAGE *package, LPCWSTR progid)
{
    INT rc;
    MSIRECORD *row;
    INT i;
    static const WCHAR ExecSeqQuery[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','P','r','o','g','I','d','`',' ','W','H','E','R','E',' ',
         '`','P','r','o','g','I','d','`',' ','=',' ','\'','%','s','\'',0};

    if (!progid)
        return -1;

    /* check for progids already loaded */
    for (i = 0; i < package->loaded_progids; i++)
        if (strcmpiW(package->progids[i].ProgID,progid)==0)
        {
            TRACE("found progid %s at index %i\n",debugstr_w(progid), i);
            return i;
        }
    
    row = MSI_QueryGetRecord(package->db, ExecSeqQuery, progid);
    if(!row)
        return -1;

    rc = load_progid(package, row);
    msiobj_release(&row->hdr);

    return rc;
}

static MSICLASS *load_class( MSIPACKAGE* package, MSIRECORD *row )
{
    MSICLASS *cls;
    DWORD sz,i;
    LPCWSTR buffer;

    /* fill in the data */

    cls = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MSICLASS) );
    if (!cls)
        return NULL;

    sz = IDENTIFIER_SIZE;
    MSI_RecordGetStringW(row, 1, cls->CLSID, &sz);
    TRACE("loading class %s\n",debugstr_w(cls->CLSID));
    sz = IDENTIFIER_SIZE;
    MSI_RecordGetStringW(row, 2, cls->Context, &sz);
    buffer = MSI_RecordGetString(row,3);
    cls->Component = get_loaded_component(package, buffer);

    cls->ProgIDText = load_dynamic_stringW(row,4);
    cls->ProgIDIndex = 
                load_given_progid(package, cls->ProgIDText);

    cls->Description = load_dynamic_stringW(row,5);

    buffer = MSI_RecordGetString(row,6);
    if (buffer)
        cls->AppID = load_given_appid(package, buffer);

    cls->FileTypeMask = load_dynamic_stringW(row,7);

    if (!MSI_RecordIsNull(row,9))
    {

        INT icon_index = MSI_RecordGetInteger(row,9); 
        LPWSTR FileName = load_dynamic_stringW(row,8);
        LPWSTR FilePath;
        static const WCHAR fmt[] = {'%','s',',','%','i',0};

        build_icon_path(package,FileName,&FilePath);
       
        cls->IconPath = 
                HeapAlloc(GetProcessHeap(),0,(strlenW(FilePath)+5)*
                                sizeof(WCHAR));

        sprintfW(cls->IconPath,fmt,FilePath,icon_index);

        HeapFree(GetProcessHeap(),0,FilePath);
        HeapFree(GetProcessHeap(),0,FileName);
    }
    else
    {
        buffer = MSI_RecordGetString(row,8);
        if (buffer)
            build_icon_path(package,buffer,&(cls->IconPath));
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
            cls->DefInprocHandler32 = load_dynamic_stringW(
                            row, 10);
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
        if (lstrcmpiW( cls->CLSID, classid )==0)
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
    DWORD sz;
    LPCWSTR buffer;
    MSIMIME *mt;

    /* fill in the data */

    mt = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MSIMIME) );
    if (!mt)
        return mt;

    mt->ContentType = load_dynamic_stringW( row, 1 ); 
    TRACE("loading mime %s\n", debugstr_w(mt->ContentType));

    buffer = MSI_RecordGetString( row, 2 );
    mt->Extension = load_given_extension( package, buffer );

    sz = IDENTIFIER_SIZE;
    MSI_RecordGetStringW( row, 3, mt->CLSID, &sz );
    mt->Class = load_given_class( package, mt->CLSID );

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
    DWORD sz;
    LPCWSTR buffer;

    /* fill in the data */

    ext = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MSIEXTENSION) );

    sz = 256;
    MSI_RecordGetStringW( row, 1, ext->Extension, &sz );
    TRACE("loading extension %s\n", debugstr_w(ext->Extension));

    buffer = MSI_RecordGetString( row, 2 );
    ext->Component = get_loaded_component( package,buffer );

    ext->ProgIDText = load_dynamic_stringW( row, 3 );
    ext->ProgIDIndex = load_given_progid( package, ext->ProgIDText );

    buffer = MSI_RecordGetString( row, 4 );
    ext->Mime = load_given_mime( package, buffer );

    buffer = MSI_RecordGetString(row,5);
    ext->Feature = get_loaded_feature( package, buffer );

    list_add_tail( &package->extensions, &ext->entry );

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
    DWORD index = package->loaded_verbs;
    LPCWSTR buffer;

    /* fill in the data */

    package->loaded_verbs++;
    if (package->loaded_verbs == 1)
        package->verbs = HeapAlloc(GetProcessHeap(),0,sizeof(MSIVERB));
    else
        package->verbs = HeapReAlloc(GetProcessHeap(),0,
            package->verbs , package->loaded_verbs * sizeof(MSIVERB));

    memset(&package->verbs[index],0,sizeof(MSIVERB));

    buffer = MSI_RecordGetString(row,1);
    package->verbs[index].Extension = load_given_extension(package,buffer);
    if (package->verbs[index].Extension == NULL && buffer)
        ERR("Verb unable to find loaded extension %s\n", debugstr_w(buffer));

    package->verbs[index].Verb = load_dynamic_stringW(row,2);
    TRACE("loading verb %s\n",debugstr_w(package->verbs[index].Verb));
    package->verbs[index].Sequence = MSI_RecordGetInteger(row,3);

    buffer = MSI_RecordGetString(row,4);
    deformat_string(package,buffer,&package->verbs[index].Command);

    buffer = MSI_RecordGetString(row,5);
    deformat_string(package,buffer,&package->verbs[index].Argument);

    /* assosiate the verb with the correct extension */
    if (package->verbs[index].Extension)
    {
        MSIEXTENSION* extension = package->verbs[index].Extension;
        int count = extension->VerbCount;

        if (count >= 99)
            FIXME("Exceeding max verb count! Increase that limit!!!\n");
        else
        {
            extension->VerbCount++;
            extension->Verbs[count] = index;
        }
    }
    
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
        if (strcmpiW( clsid, cls->CLSID ))
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
        package->progids || package->verbs )
        return;

    load_all_classes(package);
    load_all_extensions(package);
    load_all_progids(package);
    /* these loads must come after the other loads */
    load_all_verbs(package);
    load_all_mimes(package);
}

static void mark_progid_for_install(MSIPACKAGE* package, INT index)
{
    MSIPROGID* progid;
    int i;

    if (index < 0 || index >= package->loaded_progids)
        return;

    progid = &package->progids[index];

    if (progid->InstallMe == TRUE)
        return;

    progid->InstallMe = TRUE;

    /* all children if this is a parent also install */
   for (i = 0; i < package->loaded_progids; i++)
        if (package->progids[i].ParentIndex == index)
            mark_progid_for_install(package,i);
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
    HKEY hkey2,hkey3;
    UINT size; 

    RegCreateKeyW(HKEY_CLASSES_ROOT,szAppID,&hkey2);
    RegCreateKeyW( hkey2, appid->AppID, &hkey3 );
    RegSetValueExW( hkey3, NULL, 0, REG_SZ,
                    (LPBYTE)app, (strlenW(app)+1)*sizeof(WCHAR) );

    if (appid->RemoteServerName)
    {
        static const WCHAR szRemoteServerName[] =
             {'R','e','m','o','t','e','S','e','r','v','e','r','N','a','m','e',0};

        size = (lstrlenW(appid->RemoteServerName)+1) * sizeof(WCHAR);

        RegSetValueExW( hkey3, szRemoteServerName, 0, REG_SZ,
                        (LPBYTE)appid->RemoteServerName, size);
    }

    if (appid->LocalServer)
    {
        static const WCHAR szLocalService[] =
             {'L','o','c','a','l','S','e','r','v','i','c','e',0};

        size = (lstrlenW(appid->LocalServer)+1) * sizeof(WCHAR);

        RegSetValueExW( hkey3, szLocalService, 0, REG_SZ,
                        (LPBYTE)appid->LocalServer, size );
    }

    if (appid->ServiceParameters)
    {
        static const WCHAR szService[] =
             {'S','e','r','v','i','c','e',
              'P','a','r','a','m','e','t','e','r','s',0};

        size = (lstrlenW(appid->ServiceParameters)+1) * sizeof(WCHAR);
        RegSetValueExW( hkey3, szService, 0, REG_SZ,
                        (LPBYTE)appid->ServiceParameters, size );
    }

    if (appid->DllSurrogate)
    {
        static const WCHAR szDLL[] =
             {'D','l','l','S','u','r','r','o','g','a','t','e',0};

        size = (lstrlenW(appid->DllSurrogate)+1) * sizeof(WCHAR);
        RegSetValueExW( hkey3, szDLL, 0, REG_SZ,
                        (LPBYTE)appid->DllSurrogate, size );
    }

    if (appid->ActivateAtStorage)
    {
        static const WCHAR szActivate[] =
             {'A','c','t','i','v','a','t','e','A','s',
              'S','t','o','r','a','g','e',0};
        static const WCHAR szY[] = {'Y',0};

        RegSetValueExW( hkey3, szActivate, 0, REG_SZ,
                        (LPBYTE)szY, sizeof szY );
    }

    if (appid->RunAsInteractiveUser)
    {
        static const WCHAR szRunAs[] = {'R','u','n','A','s',0};
        static const WCHAR szUser[] = 
             {'I','n','t','e','r','a','c','t','i','v','e',' ',
              'U','s','e','r',0};

        RegSetValueExW( hkey3, szRunAs, 0, REG_SZ,
                        (LPBYTE)szUser, sizeof szUser );
    }

    RegCloseKey(hkey3);
    RegCloseKey(hkey2);
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
    static const WCHAR szInprocServer32[] = {'I','n','p','r','o','c','S','e','r','v','e','r','3','2',0};
    static const WCHAR szFileType_fmt[] = {'F','i','l','e','T','y','p','e','\\','%','s','\\','%','i',0};
    HKEY hkey,hkey2,hkey3;
    BOOL install_on_demand = FALSE;
    MSICLASS *cls;

    if (!package)
        return ERROR_INVALID_HANDLE;

    load_classes_and_such(package);
    rc = RegCreateKeyW(HKEY_CLASSES_ROOT,szCLSID,&hkey);
    if (rc != ERROR_SUCCESS)
        return ERROR_FUNCTION_FAILED;

    /* install_on_demand should be set if OLE supports install on demand OLE
     * servers. For now i am defaulting to FALSE because i do not know how to
     * check, and i am told our builtin OLE does not support it
     */
    
    LIST_FOR_EACH_ENTRY( cls, &package->classes, MSICLASS, entry )
    {
        MSICOMPONENT *comp;
        MSIFILE *file;
        DWORD size, sz;
        LPWSTR argument;
        MSIFEATURE *feature;

        comp = cls->Component;
        if ( !comp )
            continue;

        feature = cls->Feature;

        /* 
         * yes. MSDN says that these are based on _Feature_ not on
         * Component.  So verify the feature is to be installed
         */
        if ((!ACTION_VerifyFeatureForAction( feature, INSTALLSTATE_LOCAL )) &&
             !(install_on_demand &&
               ACTION_VerifyFeatureForAction( feature, INSTALLSTATE_ADVERTISED )))
        {
            TRACE("Skipping class %s reg due to disabled feature %s\n", 
                            debugstr_w(cls->CLSID), 
                            debugstr_w(feature->Feature));

            continue;
        }

        TRACE("Registering class %s (%p)\n", debugstr_w(cls->CLSID), cls);

        cls->Installed = TRUE;
        if (cls->ProgIDIndex >= 0)
            mark_progid_for_install( package, cls->ProgIDIndex );

        RegCreateKeyW( hkey, cls->CLSID, &hkey2 );

        if (cls->Description)
            RegSetValueExW( hkey2, NULL, 0, REG_SZ, (LPBYTE)cls->Description,
                            (strlenW(cls->Description)+1)*sizeof(WCHAR));

        RegCreateKeyW( hkey2, cls->Context, &hkey3 );
        file = get_loaded_file( package, comp->KeyPath );


        /* the context server is a short path name 
         * except for if it is InprocServer32... 
         */
        if (strcmpiW( cls->Context, szInprocServer32 )!=0)
        {
            sz = 0;
            sz = GetShortPathNameW( file->TargetPath, NULL, 0 );
            if (sz == 0)
            {
                ERR("Unable to find short path for CLSID COM Server\n");
                argument = NULL;
            }
            else
            {
                size = sz * sizeof(WCHAR);

                if (cls->Argument)
                {
                    size += strlenW(cls->Argument) * sizeof(WCHAR);
                    size += sizeof(WCHAR);
                }

                argument = HeapAlloc(GetProcessHeap(), 0, size + sizeof(WCHAR));
                GetShortPathNameW( file->TargetPath, argument, sz );

                if (cls->Argument)
                {
                    strcatW(argument,szSpace);
                    strcatW( argument, cls->Argument );
                }
            }
        }
        else
        {
            size = lstrlenW( file->TargetPath ) * sizeof(WCHAR);

            if (cls->Argument)
            {
                size += strlenW(cls->Argument) * sizeof(WCHAR);
                size += sizeof(WCHAR);
            }

            argument = HeapAlloc(GetProcessHeap(), 0, size + sizeof(WCHAR));
            strcpyW( argument, file->TargetPath );

            if (cls->Argument)
            {
                strcatW(argument,szSpace);
                strcatW( argument, cls->Argument );
            }
        }

        if (argument)
        {
            RegSetValueExW(hkey3,NULL,0,REG_SZ, (LPBYTE)argument, size);
            HeapFree(GetProcessHeap(),0,argument);
        }

        RegCloseKey(hkey3);

        if (cls->ProgIDIndex >= 0 || cls->ProgIDText)
        {
            LPCWSTR progid;

            if (cls->ProgIDIndex >= 0)
                progid = package->progids[cls->ProgIDIndex].ProgID;
            else
                progid = cls->ProgIDText;

            RegCreateKeyW(hkey2,szProgID,&hkey3);
            RegSetValueExW(hkey3,NULL,0,REG_SZ,(LPBYTE)progid,
                            (strlenW(progid)+1) *sizeof(WCHAR));
            RegCloseKey(hkey3);

            if (cls->ProgIDIndex >= 0 &&
                package->progids[cls->ProgIDIndex].VersionIndIndex >= 0)
            {
                LPWSTR viprogid = strdupW(package->progids[package->progids[
                        cls->ProgIDIndex].VersionIndIndex].ProgID);
                RegCreateKeyW(hkey2,szVIProgID,&hkey3);
                RegSetValueExW(hkey3,NULL,0,REG_SZ,(LPBYTE)viprogid,
                            (strlenW(viprogid)+1) *sizeof(WCHAR));
                RegCloseKey(hkey3);
                HeapFree(GetProcessHeap(), 0, viprogid);
            }
        }

        if (cls->AppID)
        { 
            MSIAPPID *appid = cls->AppID;

            RegSetValueExW( hkey2, szAppID, 0, REG_SZ, (LPBYTE)appid->AppID,
                           (lstrlenW(appid->AppID)+1)*sizeof(WCHAR) );

            register_appid( appid, cls->Description );
        }

        if (cls->IconPath)
        {
            static const WCHAR szDefaultIcon[] = 
                {'D','e','f','a','u','l','t','I','c','o','n',0};

            RegCreateKeyW(hkey2,szDefaultIcon,&hkey3);

            RegSetValueExW( hkey3, NULL, 0, REG_SZ, (LPVOID)cls->IconPath,
                           (strlenW(cls->IconPath)+1) * sizeof(WCHAR));

            RegCloseKey(hkey3);
        }

        if (cls->DefInprocHandler)
        {
            static const WCHAR szInproc[] =
                {'I','n','p','r','o','c','H','a','n','d','l','e','r',0};

            size = (strlenW(cls->DefInprocHandler) + 1) * sizeof(WCHAR);
            RegCreateKeyW(hkey2,szInproc,&hkey3);
            RegSetValueExW(hkey3,NULL,0,REG_SZ, 
                            (LPBYTE)cls->DefInprocHandler, size);
            RegCloseKey(hkey3);
        }

        if (cls->DefInprocHandler32)
        {
            static const WCHAR szInproc32[] =
                {'I','n','p','r','o','c','H','a','n','d','l','e','r','3','2',
                 0};
            size = (strlenW(cls->DefInprocHandler32) + 1) * sizeof(WCHAR);

            RegCreateKeyW(hkey2,szInproc32,&hkey3);
            RegSetValueExW(hkey3,NULL,0,REG_SZ, 
                           (LPBYTE)cls->DefInprocHandler32,size);
            RegCloseKey(hkey3);
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
                keyname = HeapAlloc(GetProcessHeap(),0,(strlenW(szFileType_fmt)+
                                        strlenW(cls->CLSID) + 4) * sizeof(WCHAR));
                sprintfW( keyname, szFileType_fmt, cls->CLSID, index );

                RegCreateKeyW(HKEY_CLASSES_ROOT,keyname,&hkey2);
                RegSetValueExW(hkey2,NULL,0,REG_SZ, (LPVOID)ptr,
                        strlenW(ptr)*sizeof(WCHAR));
                RegCloseKey(hkey2);
                HeapFree(GetProcessHeap(), 0, keyname);

                if (ptr2)
                    ptr = ptr2+1;
                else
                    ptr = NULL;

                index ++;
            }
        }
        
        uirow = MSI_CreateRecord(1);

        MSI_RecordSetStringW( uirow, 1, cls->CLSID );
        ui_actiondata(package,szRegisterClassInfo,uirow);
        msiobj_release(&uirow->hdr);
    }

    RegCloseKey(hkey);
    return rc;
}

static UINT register_progid_base(MSIPACKAGE* package, MSIPROGID* progid,
                                 LPWSTR clsid)
{
    static const WCHAR szCLSID[] = { 'C','L','S','I','D',0 };
    static const WCHAR szDefaultIcon[] =
        {'D','e','f','a','u','l','t','I','c','o','n',0};
    HKEY hkey,hkey2;

    RegCreateKeyW(HKEY_CLASSES_ROOT,progid->ProgID,&hkey);

    if (progid->Description)
    {
        RegSetValueExW(hkey,NULL,0,REG_SZ,
                        (LPVOID)progid->Description, 
                        (strlenW(progid->Description)+1) *
                       sizeof(WCHAR));
    }

    if (progid->Class)
    {   
        RegCreateKeyW(hkey,szCLSID,&hkey2);
        RegSetValueExW( hkey2, NULL, 0, REG_SZ, (LPBYTE)progid->Class->CLSID, 
                        (strlenW(progid->Class->CLSID)+1) * sizeof(WCHAR) );

        if (clsid)
            strcpyW( clsid, progid->Class->CLSID );

        RegCloseKey(hkey2);
    }
    else
    {
        FIXME("UNHANDLED case, Parent progid but classid is NULL\n");
    }

    if (progid->IconPath)
    {
        RegCreateKeyW(hkey,szDefaultIcon,&hkey2);

        RegSetValueExW(hkey2,NULL,0,REG_SZ,(LPVOID)progid->IconPath,
                           (strlenW(progid->IconPath)+1) * sizeof(WCHAR));
        RegCloseKey(hkey2);
    }
    return ERROR_SUCCESS;
}

static UINT register_progid(MSIPACKAGE *package, MSIPROGID* progid, 
                LPWSTR clsid)
{
    UINT rc = ERROR_SUCCESS; 

    if (progid->ParentIndex < 0)
        rc = register_progid_base(package, progid, clsid);
    else
    {
        DWORD disp;
        HKEY hkey,hkey2;
        static const WCHAR szCLSID[] = { 'C','L','S','I','D',0 };
        static const WCHAR szDefaultIcon[] =
            {'D','e','f','a','u','l','t','I','c','o','n',0};
        static const WCHAR szCurVer[] =
            {'C','u','r','V','e','r',0};

        /* check if already registered */
        RegCreateKeyExW(HKEY_CLASSES_ROOT, progid->ProgID, 0, NULL, 0,
                        KEY_ALL_ACCESS, NULL, &hkey, &disp );
        if (disp == REG_OPENED_EXISTING_KEY)
        {
            TRACE("Key already registered\n");
            RegCloseKey(hkey);
            return rc;
        }

        TRACE("Registering Parent %s index %i\n",
                    debugstr_w(package->progids[progid->ParentIndex].ProgID), 
                    progid->ParentIndex);
        rc = register_progid(package,&package->progids[progid->ParentIndex],
                        clsid);

        /* clsid is same as parent */
        RegCreateKeyW(hkey,szCLSID,&hkey2);
        RegSetValueExW(hkey2,NULL,0,REG_SZ,(LPVOID)clsid, (strlenW(clsid)+1) *
                       sizeof(WCHAR));

        RegCloseKey(hkey2);


        if (progid->Description)
        {
            RegSetValueExW(hkey,NULL,0,REG_SZ,(LPVOID)progid->Description,
                           (strlenW(progid->Description)+1) * sizeof(WCHAR));
        }

        if (progid->IconPath)
        {
            RegCreateKeyW(hkey,szDefaultIcon,&hkey2);
            RegSetValueExW(hkey2,NULL,0,REG_SZ,(LPVOID)progid->IconPath,
                           (strlenW(progid->IconPath)+1) * sizeof(WCHAR));
            RegCloseKey(hkey2);
        }

        /* write out the current version */
        if (progid->CurVerIndex >= 0)
        {
            RegCreateKeyW(hkey,szCurVer,&hkey2);
            RegSetValueExW(hkey2,NULL,0,REG_SZ,
                (LPVOID)package->progids[progid->CurVerIndex].ProgID,
                (strlenW(package->progids[progid->CurVerIndex].ProgID)+1) * 
                sizeof(WCHAR));
            RegCloseKey(hkey2);
        }

        RegCloseKey(hkey);
    }
    return rc;
}

UINT ACTION_RegisterProgIdInfo(MSIPACKAGE *package)
{
    INT i;
    MSIRECORD *uirow;

    if (!package)
        return ERROR_INVALID_HANDLE;

    load_classes_and_such(package);

    for (i = 0; i < package->loaded_progids; i++)
    {
        WCHAR clsid[0x1000];

        /* check if this progid is to be installed */
        package->progids[i].InstallMe =  ((package->progids[i].InstallMe) ||
              (package->progids[i].Class &&
               package->progids[i].Class->Installed));

        if (!package->progids[i].InstallMe)
        {
            TRACE("progid %s not scheduled to be installed\n",
                             debugstr_w(package->progids[i].ProgID));
            continue;
        }
       
        TRACE("Registering progid %s index %i\n",
                        debugstr_w(package->progids[i].ProgID), i);

        register_progid(package,&package->progids[i],clsid);

        uirow = MSI_CreateRecord(1);
        MSI_RecordSetStringW(uirow,1,package->progids[i].ProgID);
        ui_actiondata(package,szRegisterProgIdInfo,uirow);
        msiobj_release(&uirow->hdr);
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

     command = HeapAlloc(GetProcessHeap(),0, size * sizeof (WCHAR));
     if (verb->Argument)
        sprintfW(command, fmt, component->FullKeypath, verb->Argument);
     else
        sprintfW(command, fmt2, component->FullKeypath);

     RegSetValueExW(key,NULL,0,REG_SZ, (LPVOID)command, (strlenW(command)+1)*
                     sizeof(WCHAR));
     HeapFree(GetProcessHeap(),0,command);

     advertise = create_component_advertise_string(package, component, 
                                                   extension->Feature->Feature);

     size = strlenW(advertise);

     if (verb->Argument)
         size += strlenW(verb->Argument);
     size += 4;

     command = HeapAlloc(GetProcessHeap(),0, size * sizeof (WCHAR));
     memset(command,0,size*sizeof(WCHAR));

     strcpyW(command,advertise);
     if (verb->Argument)
     {
         static const WCHAR szSpace[] = {' ',0};
         strcatW(command,szSpace);
         strcatW(command,verb->Argument);
     }

     RegSetValueExW(key, szCommand, 0, REG_MULTI_SZ, (LPBYTE)command,
                        (strlenW(command)+2)*sizeof(WCHAR));
     
     RegCloseKey(key);
     HeapFree(GetProcessHeap(),0,keyname);
     HeapFree(GetProcessHeap(),0,advertise);
     HeapFree(GetProcessHeap(),0,command);

     if (verb->Command)
     {
        keyname = build_directory_name(3, progid, szShell, verb->Verb);
        RegCreateKeyW(HKEY_CLASSES_ROOT, keyname, &key);
        RegSetValueExW(key,NULL,0,REG_SZ, (LPVOID)verb->Command,
                                    (strlenW(verb->Command)+1) *sizeof(WCHAR));
        RegCloseKey(key);
        HeapFree(GetProcessHeap(),0,keyname);
     }

     if (verb->Sequence != MSI_NULL_INTEGER)
     {
        if (*Sequence == MSI_NULL_INTEGER || verb->Sequence < *Sequence)
        {
            *Sequence = verb->Sequence;
            keyname = build_directory_name(2, progid, szShell);
            RegCreateKeyW(HKEY_CLASSES_ROOT, keyname, &key);
            RegSetValueExW(key,NULL,0,REG_SZ, (LPVOID)verb->Verb,
                            (strlenW(verb->Verb)+1) *sizeof(WCHAR));
            RegCloseKey(key);
            HeapFree(GetProcessHeap(),0,keyname);
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

    if (!package)
        return ERROR_INVALID_HANDLE;

    load_classes_and_such(package);

    /* We need to set install_on_demand based on if the shell handles advertised
     * shortcuts and the like. Because Mike McCormack is working on this i am
     * going to default to TRUE
     */
    
    LIST_FOR_EACH_ENTRY( ext, &package->extensions, MSIEXTENSION, entry )
    {
        WCHAR extension[257];
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
        if (ext->ProgIDIndex >= 0 && ext->VerbCount > 0)
           mark_progid_for_install(package, ext->ProgIDIndex);

        mark_mime_for_install(ext->Mime);

        extension[0] = '.';
        extension[1] = 0;
        strcatW(extension,ext->Extension);

        RegCreateKeyW(HKEY_CLASSES_ROOT,extension,&hkey);

        if (ext->Mime)
        {
            RegSetValueExW(hkey,szContentType,0,REG_SZ,
                            (LPBYTE)ext->Mime->ContentType,
                     (strlenW(ext->Mime->ContentType)+1)*sizeof(WCHAR));
        }

        if (ext->ProgIDIndex >= 0 || ext->ProgIDText)
        {
            static const WCHAR szSN[] = 
                {'\\','S','h','e','l','l','N','e','w',0};
            HKEY hkey2;
            LPWSTR newkey;
            LPCWSTR progid;
            INT v;
            INT Sequence = MSI_NULL_INTEGER;
            
            if (ext->ProgIDIndex >= 0)
                progid = package->progids[ext->ProgIDIndex].ProgID;
            else
                progid = ext->ProgIDText;

            RegSetValueExW(hkey,NULL,0,REG_SZ,(LPVOID)progid,
                           (strlenW(progid)+1)*sizeof(WCHAR));

            newkey = HeapAlloc(GetProcessHeap(),0,
                           (strlenW(progid)+strlenW(szSN)+1) * sizeof(WCHAR)); 

            strcpyW(newkey,progid);
            strcatW(newkey,szSN);
            RegCreateKeyW(hkey,newkey,&hkey2);
            RegCloseKey(hkey2);

            HeapFree(GetProcessHeap(),0,newkey);

            /* do all the verbs */
            for (v = 0; v < ext->VerbCount; v++)
                register_verb( package, progid, ext->Component,
                               ext, &package->verbs[ext->Verbs[v]], &Sequence);
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
    HKEY hkey;
    MSIRECORD *uirow;
    MSIMIME *mt;

    if (!package)
        return ERROR_INVALID_HANDLE;

    load_classes_and_such(package);

    LIST_FOR_EACH_ENTRY( mt, &package->mimes, MSIMIME, entry )
    {
        WCHAR extension[257];
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
        extension[0] = '.';
        extension[1] = 0;
        strcatW(extension,exten);

        key = HeapAlloc(GetProcessHeap(),0,(strlenW(mime)+strlenW(fmt)+1) *
                                            sizeof(WCHAR));
        sprintfW(key,fmt,mime);
        RegCreateKeyW(HKEY_CLASSES_ROOT,key,&hkey);
        RegSetValueExW(hkey,szExten,0,REG_SZ,(LPVOID)extension,
                           (strlenW(extension)+1)*sizeof(WCHAR));

        HeapFree(GetProcessHeap(),0,key);

        if (mt->CLSID[0])
        {
            FIXME("Handle non null for field 3\n");
        }

        RegCloseKey(hkey);

        uirow = MSI_CreateRecord(2);
        MSI_RecordSetStringW(uirow,1,mt->ContentType);
        MSI_RecordSetStringW(uirow,2,exten);
        ui_actiondata(package,szRegisterMIMEInfo,uirow);
        msiobj_release(&uirow->hdr);
    }

    return ERROR_SUCCESS;
}
