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

static INT load_appid(MSIPACKAGE* package, MSIRECORD *row)
{
    DWORD index = package->loaded_appids;
    DWORD sz;
    LPCWSTR buffer;

    /* fill in the data */

    package->loaded_appids++;
    if (package->loaded_appids == 1)
        package->appids = HeapAlloc(GetProcessHeap(),0,sizeof(MSIAPPID));
    else
        package->appids = HeapReAlloc(GetProcessHeap(),0,
            package->appids, package->loaded_appids * sizeof(MSIAPPID));

    memset(&package->appids[index],0,sizeof(MSIAPPID));
    
    sz = IDENTIFIER_SIZE;
    MSI_RecordGetStringW(row, 1, package->appids[index].AppID, &sz);
    TRACE("loading appid %s\n",debugstr_w(package->appids[index].AppID));

    buffer = MSI_RecordGetString(row,2);
    deformat_string(package,buffer,&package->appids[index].RemoteServerName);

    package->appids[index].LocalServer = load_dynamic_stringW(row,3);
    package->appids[index].ServiceParameters = load_dynamic_stringW(row,4);
    package->appids[index].DllSurrogate = load_dynamic_stringW(row,5);

    package->appids[index].ActivateAtStorage = !MSI_RecordIsNull(row,6);
    package->appids[index].RunAsInteractiveUser = !MSI_RecordIsNull(row,7);
    
    return index;
}

static INT load_given_appid(MSIPACKAGE *package, LPCWSTR appid)
{
    INT rc;
    MSIRECORD *row;
    INT i;
    static const WCHAR ExecSeqQuery[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','A','p','p','I','d','`',' ','W','H','E','R','E',' ',
         '`','A','p','p','I','d','`',' ','=',' ','\'','%','s','\'',0};

    if (!appid)
        return -1;

    /* check for appids already loaded */
    for (i = 0; i < package->loaded_appids; i++)
        if (strcmpiW(package->appids[i].AppID,appid)==0)
        {
            TRACE("found appid %s at index %i\n",debugstr_w(appid),i);
            return i;
        }
    
    row = MSI_QueryGetRecord(package->db, ExecSeqQuery, appid);
    if (!row)
        return -1;

    rc = load_appid(package, row);
    msiobj_release(&row->hdr);

    return rc;
}

static INT load_given_progid(MSIPACKAGE *package, LPCWSTR progid);
static INT load_given_class(MSIPACKAGE *package, LPCWSTR classid);

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
    package->progids[index].ClassIndex = load_given_class(package,buffer);
    if (package->progids[index].ClassIndex< 0 && buffer)
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

static INT load_class(MSIPACKAGE* package, MSIRECORD *row)
{
    DWORD index = package->loaded_classes;
    DWORD sz,i;
    LPCWSTR buffer;

    /* fill in the data */

    package->loaded_classes++;
    if (package->loaded_classes== 1)
        package->classes = HeapAlloc(GetProcessHeap(),0,sizeof(MSICLASS));
    else
        package->classes = HeapReAlloc(GetProcessHeap(),0,
            package->classes, package->loaded_classes * sizeof(MSICLASS));

    memset(&package->classes[index],0,sizeof(MSICLASS));

    sz = IDENTIFIER_SIZE;
    MSI_RecordGetStringW(row, 1, package->classes[index].CLSID, &sz);
    TRACE("loading class %s\n",debugstr_w(package->classes[index].CLSID));
    sz = IDENTIFIER_SIZE;
    MSI_RecordGetStringW(row, 2, package->classes[index].Context, &sz);
    buffer = MSI_RecordGetString(row,3);
    package->classes[index].ComponentIndex = get_loaded_component(package, 
                    buffer);

    package->classes[index].ProgIDText = load_dynamic_stringW(row,4);
    package->classes[index].ProgIDIndex = 
                load_given_progid(package, package->classes[index].ProgIDText);

    package->classes[index].Description = load_dynamic_stringW(row,5);

    buffer = MSI_RecordGetString(row,6);
    if (buffer)
        package->classes[index].AppIDIndex = 
                load_given_appid(package, buffer);
    else
        package->classes[index].AppIDIndex = -1;

    package->classes[index].FileTypeMask = load_dynamic_stringW(row,7);

    if (!MSI_RecordIsNull(row,9))
    {

        INT icon_index = MSI_RecordGetInteger(row,9); 
        LPWSTR FileName = load_dynamic_stringW(row,8);
        LPWSTR FilePath;
        static const WCHAR fmt[] = {'%','s',',','%','i',0};

        build_icon_path(package,FileName,&FilePath);
       
        package->classes[index].IconPath = 
                HeapAlloc(GetProcessHeap(),0,(strlenW(FilePath)+5)*
                                sizeof(WCHAR));

        sprintfW(package->classes[index].IconPath,fmt,FilePath,icon_index);

        HeapFree(GetProcessHeap(),0,FilePath);
        HeapFree(GetProcessHeap(),0,FileName);
    }
    else
    {
        buffer = MSI_RecordGetString(row,8);
        if (buffer)
            build_icon_path(package,buffer,&(package->classes[index].IconPath));
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
                    package->classes[index].DefInprocHandler = strdupW(ole2);
                    break;
                case 2:
                    package->classes[index].DefInprocHandler32 = strdupW(ole32);
                    break;
                case 3:
                    package->classes[index].DefInprocHandler = strdupW(ole2);
                    package->classes[index].DefInprocHandler32 = strdupW(ole32);
                    break;
            }
        }
        else
        {
            package->classes[index].DefInprocHandler32 = load_dynamic_stringW(
                            row, 10);
            reduce_to_longfilename(package->classes[index].DefInprocHandler32);
        }
    }
    buffer = MSI_RecordGetString(row,11);
    deformat_string(package,buffer,&package->classes[index].Argument);

    buffer = MSI_RecordGetString(row,12);
    package->classes[index].FeatureIndex = get_loaded_feature(package,buffer);

    package->classes[index].Attributes = MSI_RecordGetInteger(row,13);
    
    return index;
}

/*
 * the Class table has 3 primary keys. Generally it is only 
 * referenced through the first CLSID key. However when loading
 * all of the classes we need to make sure we do not ignore rows
 * with other Context and ComponentIndexs 
 */
static INT load_given_class(MSIPACKAGE *package, LPCWSTR classid)
{
    INT rc;
    MSIRECORD *row;
    INT i;
    static const WCHAR ExecSeqQuery[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','C','l','a','s','s','`',' ','W','H','E','R','E',' ',
         '`','C','L','S','I','D','`',' ','=',' ','\'','%','s','\'',0};


    if (!classid)
        return -1;
    
    /* check for classes already loaded */
    for (i = 0; i < package->loaded_classes; i++)
        if (strcmpiW(package->classes[i].CLSID,classid)==0)
        {
            TRACE("found class %s at index %i\n",debugstr_w(classid), i);
            return i;
        }
    
    row = MSI_QueryGetRecord(package->db, ExecSeqQuery, classid);
    if (!row)
        return -1;

    rc = load_class(package, row);
    msiobj_release(&row->hdr);

    return rc;
}

static INT load_given_extension(MSIPACKAGE *package, LPCWSTR extension);

static INT load_mime(MSIPACKAGE* package, MSIRECORD *row)
{
    DWORD index = package->loaded_mimes;
    DWORD sz;
    LPCWSTR buffer;

    /* fill in the data */

    package->loaded_mimes++;
    if (package->loaded_mimes== 1)
        package->mimes= HeapAlloc(GetProcessHeap(),0,sizeof(MSIMIME));
    else
        package->mimes= HeapReAlloc(GetProcessHeap(),0,
            package->mimes, package->loaded_mimes* 
            sizeof(MSIMIME));

    memset(&package->mimes[index],0,sizeof(MSIMIME));

    package->mimes[index].ContentType = load_dynamic_stringW(row,1); 
    TRACE("loading mime %s\n",debugstr_w(package->mimes[index].ContentType));

    buffer = MSI_RecordGetString(row,2);
    package->mimes[index].ExtensionIndex = load_given_extension(package,
                    buffer);

    sz = IDENTIFIER_SIZE;
    MSI_RecordGetStringW(row,3,package->mimes[index].CLSID,&sz);
    package->mimes[index].ClassIndex= load_given_class(package,
                    package->mimes[index].CLSID);
    
    return index;
}

static INT load_given_mime(MSIPACKAGE *package, LPCWSTR mime)
{
    INT rc;
    MSIRECORD *row;
    INT i;
    static const WCHAR ExecSeqQuery[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','M','I','M','E','`',' ','W','H','E','R','E',' ',
         '`','C','o','n','t','e','n','t','T','y','p','e','`',' ','=',' ',
         '\'','%','s','\'',0};

    if (!mime)
        return -1;
    
    /* check for mime already loaded */
    for (i = 0; i < package->loaded_mimes; i++)
        if (strcmpiW(package->mimes[i].ContentType,mime)==0)
        {
            TRACE("found mime %s at index %i\n",debugstr_w(mime), i);
            return i;
        }
    
    row = MSI_QueryGetRecord(package->db, ExecSeqQuery, mime);
    if (!row)
        return -1;

    rc = load_mime(package, row);
    msiobj_release(&row->hdr);

    return rc;
}

static INT load_extension(MSIPACKAGE* package, MSIRECORD *row)
{
    DWORD index = package->loaded_extensions;
    DWORD sz;
    LPCWSTR buffer;

    /* fill in the data */

    package->loaded_extensions++;
    if (package->loaded_extensions == 1)
        package->extensions = HeapAlloc(GetProcessHeap(),0,sizeof(MSIEXTENSION));
    else
        package->extensions = HeapReAlloc(GetProcessHeap(),0,
            package->extensions, package->loaded_extensions* 
            sizeof(MSIEXTENSION));

    memset(&package->extensions[index],0,sizeof(MSIEXTENSION));

    sz = 256;
    MSI_RecordGetStringW(row,1,package->extensions[index].Extension,&sz);
    TRACE("loading extension %s\n",
                    debugstr_w(package->extensions[index].Extension));

    buffer = MSI_RecordGetString(row,2);
    package->extensions[index].ComponentIndex = 
            get_loaded_component(package,buffer);

    package->extensions[index].ProgIDText = load_dynamic_stringW(row,3);
    package->extensions[index].ProgIDIndex = load_given_progid(package,
                    package->extensions[index].ProgIDText);

    buffer = MSI_RecordGetString(row,4);
    package->extensions[index].MIMEIndex = load_given_mime(package,buffer);

    buffer = MSI_RecordGetString(row,5);
    package->extensions[index].FeatureIndex = 
            get_loaded_feature(package,buffer);

    return index;
}

/*
 * While the extension table has 2 primary keys, this function is only looking
 * at the Extension key which is what is referenced as a forign key 
 */
static INT load_given_extension(MSIPACKAGE *package, LPCWSTR extension)
{
    INT rc;
    MSIRECORD *row;
    INT i;
    static const WCHAR ExecSeqQuery[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','E','x','t','e','n','s','i','o','n','`',' ',
         'W','H','E','R','E',' ',
         '`','E','x','t','e','n','s','i','o','n','`',' ','=',' ',
         '\'','%','s','\'',0};

    if (!extension)
        return -1;

    /* check for extensions already loaded */
    for (i = 0; i < package->loaded_extensions; i++)
        if (strcmpiW(package->extensions[i].Extension,extension)==0)
        {
            TRACE("extension %s already loaded at %i\n",debugstr_w(extension),
                            i);
            return i;
        }
    
    row = MSI_QueryGetRecord(package->db, ExecSeqQuery, extension);
    if (!row)
        return -1;

    rc = load_extension(package, row);
    msiobj_release(&row->hdr);

    return rc;
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
    package->verbs[index].ExtensionIndex = load_given_extension(package,buffer);
    if (package->verbs[index].ExtensionIndex < 0 && buffer)
        ERR("Verb unable to find loaded extension %s\n", debugstr_w(buffer));

    package->verbs[index].Verb = load_dynamic_stringW(row,2);
    TRACE("loading verb %s\n",debugstr_w(package->verbs[index].Verb));
    package->verbs[index].Sequence = MSI_RecordGetInteger(row,3);

    buffer = MSI_RecordGetString(row,4);
    deformat_string(package,buffer,&package->verbs[index].Command);

    buffer = MSI_RecordGetString(row,5);
    deformat_string(package,buffer,&package->verbs[index].Argument);

    /* assosiate the verb with the correct extension */
    if (package->verbs[index].ExtensionIndex >= 0)
    {
        MSIEXTENSION* extension = &package->extensions[package->verbs[index].
                ExtensionIndex];
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
    LPCWSTR clsid;
    LPCWSTR context;
    LPCWSTR buffer;
    INT    component_index;
    MSIPACKAGE* package =(MSIPACKAGE*)param;
    INT i;
    BOOL match = FALSE;

    clsid = MSI_RecordGetString(rec,1);
    context = MSI_RecordGetString(rec,2);
    buffer = MSI_RecordGetString(rec,3);
    component_index = get_loaded_component(package,buffer);

    for (i = 0; i < package->loaded_classes; i++)
    {
        if (strcmpiW(clsid,package->classes[i].CLSID))
            continue;
        if (strcmpW(context,package->classes[i].Context))
            continue;
        if (component_index == package->classes[i].ComponentIndex)
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
    LPCWSTR buffer;
    LPCWSTR extension;
    INT    component_index;
    MSIPACKAGE* package =(MSIPACKAGE*)param;
    BOOL match = FALSE;
    INT i;

    extension = MSI_RecordGetString(rec,1);
    buffer = MSI_RecordGetString(rec,2);
    component_index = get_loaded_component(package,buffer);

    for (i = 0; i < package->loaded_extensions; i++)
    {
        if (strcmpiW(extension,package->extensions[i].Extension))
            continue;
        if (component_index == package->extensions[i].ComponentIndex)
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
    if (package->classes || package->extensions || package->progids || 
        package->verbs || package->mimes)
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

static void mark_mime_for_install(MSIPACKAGE* package, INT index)
{
    MSIMIME* mime;

    if (index < 0 || index >= package->loaded_mimes)
        return;

    mime = &package->mimes[index];

    if (mime->InstallMe == TRUE)
        return;

    mime->InstallMe = TRUE;
}

static UINT register_appid(MSIPACKAGE *package, int appidIndex, LPCWSTR app )
{
    static const WCHAR szAppID[] = { 'A','p','p','I','D',0 };
    HKEY hkey2,hkey3;

    if (!package)
        return ERROR_INVALID_HANDLE;

    RegCreateKeyW(HKEY_CLASSES_ROOT,szAppID,&hkey2);
    RegCreateKeyW(hkey2,package->appids[appidIndex].AppID,&hkey3);
    RegSetValueExW(hkey3,NULL,0,REG_SZ,(LPVOID)app,
                   (strlenW(app)+1)*sizeof(WCHAR));

    if (package->appids[appidIndex].RemoteServerName)
    {
        UINT size; 
        static const WCHAR szRemoteServerName[] =
             {'R','e','m','o','t','e','S','e','r','v','e','r','N','a','m','e',
              0};

        size = (strlenW(package->appids[appidIndex].RemoteServerName)+1) * 
                sizeof(WCHAR);

        RegSetValueExW(hkey3,szRemoteServerName,0,REG_SZ,
                        (LPVOID)package->appids[appidIndex].RemoteServerName,
                        size);
    }

    if (package->appids[appidIndex].LocalServer)
    {
        static const WCHAR szLocalService[] =
             {'L','o','c','a','l','S','e','r','v','i','c','e',0};
        UINT size;
        size = (strlenW(package->appids[appidIndex].LocalServer)+1) * 
                sizeof(WCHAR);

        RegSetValueExW(hkey3,szLocalService,0,REG_SZ,
                        (LPVOID)package->appids[appidIndex].LocalServer,size);
    }

    if (package->appids[appidIndex].ServiceParameters)
    {
        static const WCHAR szService[] =
             {'S','e','r','v','i','c','e',
              'P','a','r','a','m','e','t','e','r','s',0};
        UINT size;
        size = (strlenW(package->appids[appidIndex].ServiceParameters)+1) * 
                sizeof(WCHAR);
        RegSetValueExW(hkey3,szService,0,REG_SZ,
                        (LPVOID)package->appids[appidIndex].ServiceParameters,
                        size);
    }

    if (package->appids[appidIndex].DllSurrogate)
    {
        static const WCHAR szDLL[] =
             {'D','l','l','S','u','r','r','o','g','a','t','e',0};
        UINT size;
        size = (strlenW(package->appids[appidIndex].DllSurrogate)+1) * 
                sizeof(WCHAR);
        RegSetValueExW(hkey3,szDLL,0,REG_SZ,
                        (LPVOID)package->appids[appidIndex].DllSurrogate,size);
    }

    if (package->appids[appidIndex].ActivateAtStorage)
    {
        static const WCHAR szActivate[] =
             {'A','c','t','i','v','a','t','e','A','s',
              'S','t','o','r','a','g','e',0};
        static const WCHAR szY[] = {'Y',0};

        RegSetValueExW(hkey3,szActivate,0,REG_SZ,(LPVOID)szY,4);
    }

    if (package->appids[appidIndex].RunAsInteractiveUser)
    {
        static const WCHAR szRunAs[] = {'R','u','n','A','s',0};
        static const WCHAR szUser[] = 
             {'I','n','t','e','r','a','c','t','i','v','e',' ',
              'U','s','e','r',0};

        RegSetValueExW(hkey3,szRunAs,0,REG_SZ,(LPVOID)szUser,sizeof(szUser));
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
    int i;

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
    
    for (i = 0; i < package->loaded_classes; i++)
    {
        INT index,f_index;
        DWORD size, sz;
        LPWSTR argument;

        if (package->classes[i].ComponentIndex < 0)
        {
            continue;
        }

        index = package->classes[i].ComponentIndex;
        f_index = package->classes[i].FeatureIndex;

        /* 
         * yes. MSDN says that these are based on _Feature_ not on
         * Component.  So verify the feature is to be installed
         */
        if ((!ACTION_VerifyFeatureForAction(package, f_index,
                                INSTALLSTATE_LOCAL)) &&
             !(install_on_demand && ACTION_VerifyFeatureForAction(package,
                             f_index, INSTALLSTATE_ADVERTISED)))
        {
            TRACE("Skipping class %s reg due to disabled feature %s\n", 
                            debugstr_w(package->classes[i].CLSID), 
                            debugstr_w(package->features[f_index].Feature));

            continue;
        }

        TRACE("Registering index %i  class %s\n",i,
                        debugstr_w(package->classes[i].CLSID));

        package->classes[i].Installed = TRUE;
        if (package->classes[i].ProgIDIndex >= 0)
            mark_progid_for_install(package, package->classes[i].ProgIDIndex);

        RegCreateKeyW(hkey,package->classes[i].CLSID,&hkey2);

        if (package->classes[i].Description)
            RegSetValueExW(hkey2,NULL,0,REG_SZ,(LPVOID)package->classes[i].
                            Description, (strlenW(package->classes[i].
                                     Description)+1)*sizeof(WCHAR));

        RegCreateKeyW(hkey2,package->classes[i].Context,&hkey3);
        index = get_loaded_file(package,package->components[index].KeyPath);


        /* the context server is a short path name 
         * except for if it is InprocServer32... 
         */
        if (strcmpiW(package->classes[i].Context,szInprocServer32)!=0)
        {
            sz = 0;
            sz = GetShortPathNameW(package->files[index].TargetPath, NULL, 0);
            if (sz == 0)
            {
                ERR("Unable to find short path for CLSID COM Server\n");
                argument = NULL;
            }
            else
            {
                size = sz * sizeof(WCHAR);

                if (package->classes[i].Argument)
                {
                    size += strlenW(package->classes[i].Argument) * 
                            sizeof(WCHAR);
                    size += sizeof(WCHAR);
                }

                argument = HeapAlloc(GetProcessHeap(), 0, size + sizeof(WCHAR));
                GetShortPathNameW(package->files[index].TargetPath, argument, 
                                sz);

                if (package->classes[i].Argument)
                {
                    strcatW(argument,szSpace);
                    strcatW(argument,package->classes[i].Argument);
                }
            }
        }
        else
        {
            size = lstrlenW(package->files[index].TargetPath) * sizeof(WCHAR);

            if (package->classes[i].Argument)
            {
                size += strlenW(package->classes[i].Argument) * sizeof(WCHAR);
                size += sizeof(WCHAR);
            }

            argument = HeapAlloc(GetProcessHeap(), 0, size + sizeof(WCHAR));
            strcpyW(argument, package->files[index].TargetPath);

            if (package->classes[i].Argument)
            {
                strcatW(argument,szSpace);
                strcatW(argument,package->classes[i].Argument);
            }
        }

        if (argument)
        {
            RegSetValueExW(hkey3,NULL,0,REG_SZ, (LPVOID)argument, size);
            HeapFree(GetProcessHeap(),0,argument);
        }

        RegCloseKey(hkey3);

        if (package->classes[i].ProgIDIndex >= 0 || 
            package->classes[i].ProgIDText)
        {
            LPCWSTR progid;

            if (package->classes[i].ProgIDIndex >= 0)
                progid = package->progids[
                        package->classes[i].ProgIDIndex].ProgID;
            else
                progid = package->classes[i].ProgIDText;

            RegCreateKeyW(hkey2,szProgID,&hkey3);
            RegSetValueExW(hkey3,NULL,0,REG_SZ,(LPVOID)progid,
                            (strlenW(progid)+1) *sizeof(WCHAR));
            RegCloseKey(hkey3);

            if (package->classes[i].ProgIDIndex >= 0 &&
                package->progids[package->classes[i].ProgIDIndex].
                                VersionIndIndex >= 0)
            {
                LPWSTR viprogid = strdupW(package->progids[package->progids[
                        package->classes[i].ProgIDIndex].VersionIndIndex].
                        ProgID);
                RegCreateKeyW(hkey2,szVIProgID,&hkey3);
                RegSetValueExW(hkey3,NULL,0,REG_SZ,(LPVOID)viprogid,
                            (strlenW(viprogid)+1) *sizeof(WCHAR));
                RegCloseKey(hkey3);
                HeapFree(GetProcessHeap(), 0, viprogid);
            }
        }

        if (package->classes[i].AppIDIndex >= 0)
        { 
            RegSetValueExW(hkey2,szAppID,0,REG_SZ,
             (LPVOID)package->appids[package->classes[i].AppIDIndex].AppID,
             (strlenW(package->appids[package->classes[i].AppIDIndex].AppID)+1)
             *sizeof(WCHAR));

            register_appid(package,package->classes[i].AppIDIndex,
                            package->classes[i].Description);
        }

        if (package->classes[i].IconPath)
        {
            static const WCHAR szDefaultIcon[] = 
                {'D','e','f','a','u','l','t','I','c','o','n',0};

            RegCreateKeyW(hkey2,szDefaultIcon,&hkey3);

            RegSetValueExW(hkey3,NULL,0,REG_SZ,
                           (LPVOID)package->classes[i].IconPath,
                           (strlenW(package->classes[i].IconPath)+1) * 
                           sizeof(WCHAR));

            RegCloseKey(hkey3);
        }

        if (package->classes[i].DefInprocHandler)
        {
            static const WCHAR szInproc[] =
                {'I','n','p','r','o','c','H','a','n','d','l','e','r',0};

            size = (strlenW(package->classes[i].DefInprocHandler) + 1) * 
                    sizeof(WCHAR);
            RegCreateKeyW(hkey2,szInproc,&hkey3);
            RegSetValueExW(hkey3,NULL,0,REG_SZ, 
                            (LPVOID)package->classes[i].DefInprocHandler, size);
            RegCloseKey(hkey3);
        }

        if (package->classes[i].DefInprocHandler32)
        {
            static const WCHAR szInproc32[] =
                {'I','n','p','r','o','c','H','a','n','d','l','e','r','3','2',
                 0};
            size = (strlenW(package->classes[i].DefInprocHandler32) + 1) * 
                    sizeof(WCHAR);

            RegCreateKeyW(hkey2,szInproc32,&hkey3);
            RegSetValueExW(hkey3,NULL,0,REG_SZ, 
                           (LPVOID)package->classes[i].DefInprocHandler32,size);
            RegCloseKey(hkey3);
        }
        
        RegCloseKey(hkey2);

        /* if there is a FileTypeMask, register the FileType */
        if (package->classes[i].FileTypeMask)
        {
            LPWSTR ptr, ptr2;
            LPWSTR keyname;
            INT index = 0;
            ptr = package->classes[i].FileTypeMask;
            while (ptr && *ptr)
            {
                ptr2 = strchrW(ptr,';');
                if (ptr2)
                    *ptr2 = 0;
                keyname = HeapAlloc(GetProcessHeap(),0,(strlenW(szFileType_fmt)+
                                        strlenW(package->classes[i].CLSID) + 4)
                                * sizeof(WCHAR));
                sprintfW(keyname,szFileType_fmt, package->classes[i].CLSID, 
                        index);

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

        MSI_RecordSetStringW(uirow,1,package->classes[i].CLSID);
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

    if (progid->ClassIndex >= 0)
    {   
        RegCreateKeyW(hkey,szCLSID,&hkey2);
        RegSetValueExW(hkey2,NULL,0,REG_SZ,
                        (LPVOID)package->classes[progid->ClassIndex].CLSID, 
                        (strlenW(package->classes[progid->ClassIndex].CLSID)+1)
                        * sizeof(WCHAR));

        if (clsid)
            strcpyW(clsid,package->classes[progid->ClassIndex].CLSID);

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
              (package->progids[i].ClassIndex >= 0 &&
              package->classes[package->progids[i].ClassIndex].Installed));

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
                        package->features[extension->FeatureIndex].Feature);

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
    INT i;
    MSIRECORD *uirow;
    BOOL install_on_demand = TRUE;

    if (!package)
        return ERROR_INVALID_HANDLE;

    load_classes_and_such(package);

    /* We need to set install_on_demand based on if the shell handles advertised
     * shortcuts and the like. Because Mike McCormack is working on this i am
     * going to default to TRUE
     */
    
    for (i = 0; i < package->loaded_extensions; i++)
    {
        WCHAR extension[257];
        INT index,f_index;
     
        index = package->extensions[i].ComponentIndex;
        f_index = package->extensions[i].FeatureIndex;

        if (index < 0)
            continue;

        /* 
         * yes. MSDN says that these are based on _Feature_ not on
         * Component.  So verify the feature is to be installed
         */
        if ((!ACTION_VerifyFeatureForAction(package, f_index,
                                INSTALLSTATE_LOCAL)) &&
             !(install_on_demand && ACTION_VerifyFeatureForAction(package,
                             f_index, INSTALLSTATE_ADVERTISED)))
        {
            TRACE("Skipping extension  %s reg due to disabled feature %s\n",
                            debugstr_w(package->extensions[i].Extension),
                            debugstr_w(package->features[f_index].Feature));

            continue;
        }

        TRACE("Registering extension %s index %i\n",
                        debugstr_w(package->extensions[i].Extension), i);

        package->extensions[i].Installed = TRUE;

        /* this is only registered if the extension has at least 1 verb
         * according to MSDN
         */
        if (package->extensions[i].ProgIDIndex >= 0 &&
                package->extensions[i].VerbCount > 0)
           mark_progid_for_install(package, package->extensions[i].ProgIDIndex);

        if (package->extensions[i].MIMEIndex >= 0)
           mark_mime_for_install(package, package->extensions[i].MIMEIndex);

        extension[0] = '.';
        extension[1] = 0;
        strcatW(extension,package->extensions[i].Extension);

        RegCreateKeyW(HKEY_CLASSES_ROOT,extension,&hkey);

        if (package->extensions[i].MIMEIndex >= 0)
        {
            RegSetValueExW(hkey,szContentType,0,REG_SZ,
                            (LPVOID)package->mimes[package->extensions[i].
                                MIMEIndex].ContentType,
                           (strlenW(package->mimes[package->extensions[i].
                                    MIMEIndex].ContentType)+1)*sizeof(WCHAR));
        }

        if (package->extensions[i].ProgIDIndex >= 0 || 
            package->extensions[i].ProgIDText)
        {
            static const WCHAR szSN[] = 
                {'\\','S','h','e','l','l','N','e','w',0};
            HKEY hkey2;
            LPWSTR newkey;
            LPCWSTR progid;
            INT v;
            INT Sequence = MSI_NULL_INTEGER;
            
            if (package->extensions[i].ProgIDIndex >= 0)
                progid = package->progids[package->extensions[i].
                    ProgIDIndex].ProgID;
            else
                progid = package->extensions[i].ProgIDText;

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
            for (v = 0; v < package->extensions[i].VerbCount; v++)
                register_verb(package, progid, 
                              &package->components[index],
                              &package->extensions[i],
                              &package->verbs[package->extensions[i].Verbs[v]], 
                              &Sequence);
        }
        
        RegCloseKey(hkey);

        uirow = MSI_CreateRecord(1);
        MSI_RecordSetStringW(uirow,1,package->extensions[i].Extension);
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
    INT i;
    MSIRECORD *uirow;

    if (!package)
        return ERROR_INVALID_HANDLE;

    load_classes_and_such(package);

    for (i = 0; i < package->loaded_mimes; i++)
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
        package->mimes[i].InstallMe =  ((package->mimes[i].InstallMe) ||
              (package->mimes[i].ClassIndex >= 0 &&
              package->classes[package->mimes[i].ClassIndex].Installed) ||
              (package->mimes[i].ExtensionIndex >=0 &&
              package->extensions[package->mimes[i].ExtensionIndex].Installed));

        if (!package->mimes[i].InstallMe)
        {
            TRACE("MIME %s not scheduled to be installed\n",
                             debugstr_w(package->mimes[i].ContentType));
            continue;
        }
        
        mime = package->mimes[i].ContentType;
        exten = package->extensions[package->mimes[i].ExtensionIndex].Extension;
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

        if (package->mimes[i].CLSID[0])
        {
            FIXME("Handle non null for field 3\n");
        }

        RegCloseKey(hkey);

        uirow = MSI_CreateRecord(2);
        MSI_RecordSetStringW(uirow,1,package->mimes[i].ContentType);
        MSI_RecordSetStringW(uirow,2,exten);
        ui_actiondata(package,szRegisterMIMEInfo,uirow);
        msiobj_release(&uirow->hdr);
    }

    return ERROR_SUCCESS;
}
