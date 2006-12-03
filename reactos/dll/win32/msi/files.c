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


/*
 * Actions dealing with files These are
 *
 * InstallFiles
 * DuplicateFiles
 * MoveFiles (TODO)
 * PatchFiles (TODO)
 * RemoveDuplicateFiles(TODO)
 * RemoveFiles(TODO)
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wine/debug.h"
#include "fdi.h"
#include "msi.h"
#include "msidefs.h"
#include "msvcrt/fcntl.h"
#include "msipriv.h"
#include "winuser.h"
#include "winreg.h"
#include "shlwapi.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

extern const WCHAR szInstallFiles[];
extern const WCHAR szDuplicateFiles[];
extern const WCHAR szMoveFiles[];
extern const WCHAR szPatchFiles[];
extern const WCHAR szRemoveDuplicateFiles[];
extern const WCHAR szRemoveFiles[];

static const WCHAR cszTempFolder[]= {'T','e','m','p','F','o','l','d','e','r',0};

/*
 * This is a helper function for handling embedded cabinet media
 */
static UINT writeout_cabinet_stream(MSIPACKAGE *package, LPCWSTR stream_name,
                                    WCHAR* source)
{
    UINT rc;
    USHORT* data;
    UINT    size;
    DWORD   write;
    HANDLE  the_file;
    WCHAR tmp[MAX_PATH];

    rc = read_raw_stream_data(package->db,stream_name,&data,&size); 
    if (rc != ERROR_SUCCESS)
        return rc;

    write = MAX_PATH;
    if (MSI_GetPropertyW(package, cszTempFolder, tmp, &write))
        GetTempPathW(MAX_PATH,tmp);

    GetTempFileNameW(tmp,stream_name,0,source);

    track_tempfile(package,strrchrW(source,'\\'), source);
    the_file = CreateFileW(source, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, NULL);

    if (the_file == INVALID_HANDLE_VALUE)
    {
        ERR("Unable to create file %s\n",debugstr_w(source));
        rc = ERROR_FUNCTION_FAILED;
        goto end;
    }

    WriteFile(the_file,data,size,&write,NULL);
    CloseHandle(the_file);
    TRACE("wrote %i bytes to %s\n",write,debugstr_w(source));
end:
    msi_free(data);
    return rc;
}


/* Support functions for FDI functions */
typedef struct
{
    MSIPACKAGE* package;
    LPCSTR cab_path;
} CabData;

static void * cabinet_alloc(ULONG cb)
{
    return msi_alloc(cb);
}

static void cabinet_free(void *pv)
{
    msi_free(pv);
}

static INT_PTR cabinet_open(char *pszFile, int oflag, int pmode)
{
    HANDLE handle;
    DWORD dwAccess = 0;
    DWORD dwShareMode = 0;
    DWORD dwCreateDisposition = OPEN_EXISTING;
    switch (oflag & _O_ACCMODE)
    {
    case _O_RDONLY:
        dwAccess = GENERIC_READ;
        dwShareMode = FILE_SHARE_READ | FILE_SHARE_DELETE;
        break;
    case _O_WRONLY:
        dwAccess = GENERIC_WRITE;
        dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
        break;
    case _O_RDWR:
        dwAccess = GENERIC_READ | GENERIC_WRITE;
        dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
        break;
    }
    if ((oflag & (_O_CREAT | _O_EXCL)) == (_O_CREAT | _O_EXCL))
        dwCreateDisposition = CREATE_NEW;
    else if (oflag & _O_CREAT)
        dwCreateDisposition = CREATE_ALWAYS;
    handle = CreateFileA( pszFile, dwAccess, dwShareMode, NULL, 
                          dwCreateDisposition, 0, NULL );
    if (handle == INVALID_HANDLE_VALUE)
        return 0;
    return (INT_PTR) handle;
}

static UINT cabinet_read(INT_PTR hf, void *pv, UINT cb)
{
    HANDLE handle = (HANDLE) hf;
    DWORD dwRead;
    if (ReadFile(handle, pv, cb, &dwRead, NULL))
        return dwRead;
    return 0;
}

static UINT cabinet_write(INT_PTR hf, void *pv, UINT cb)
{
    HANDLE handle = (HANDLE) hf;
    DWORD dwWritten;
    if (WriteFile(handle, pv, cb, &dwWritten, NULL))
        return dwWritten;
    return 0;
}

static int cabinet_close(INT_PTR hf)
{
    HANDLE handle = (HANDLE) hf;
    return CloseHandle(handle) ? 0 : -1;
}

static long cabinet_seek(INT_PTR hf, long dist, int seektype)
{
    HANDLE handle = (HANDLE) hf;
    /* flags are compatible and so are passed straight through */
    return SetFilePointer(handle, dist, NULL, seektype);
}

static void msi_file_update_ui( MSIPACKAGE *package, MSIFILE *f, const WCHAR *action )
{
    MSIRECORD *uirow;
    LPWSTR uipath, p;

    /* the UI chunk */
    uirow = MSI_CreateRecord( 9 );
    MSI_RecordSetStringW( uirow, 1, f->FileName );
    uipath = strdupW( f->TargetPath );
    p = strrchrW(uipath,'\\');
    if (p)
        p[1]=0;
    MSI_RecordSetStringW( uirow, 9, uipath);
    MSI_RecordSetInteger( uirow, 6, f->FileSize );
    ui_actiondata( package, action, uirow);
    msiobj_release( &uirow->hdr );
    msi_free( uipath );
    ui_progress( package, 2, f->FileSize, 0, 0);
}

static INT_PTR cabinet_notify(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin)
{
    switch (fdint)
    {
    case fdintCOPY_FILE:
    {
        CabData *data = (CabData*) pfdin->pv;
        HANDLE handle;
        LPWSTR file;
        MSIFILE *f;
        DWORD attrs;

        file = strdupAtoW(pfdin->psz1);
        f = get_loaded_file(data->package, file);
        msi_free(file);

        if (!f)
        {
            WARN("unknown file in cabinet (%s)\n",debugstr_a(pfdin->psz1));
            return 0;
        }

        if (f->state != msifs_missing && f->state != msifs_overwrite)
        {
            TRACE("Skipping extraction of %s\n",debugstr_a(pfdin->psz1));
            return 0;
        }

        msi_file_update_ui( data->package, f, szInstallFiles );

        TRACE("extracting %s\n", debugstr_w(f->TargetPath) );

        attrs = f->Attributes & (FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM);
        if (!attrs) attrs = FILE_ATTRIBUTE_NORMAL;

        handle = CreateFileW( f->TargetPath, GENERIC_READ | GENERIC_WRITE, 0,
                              NULL, CREATE_ALWAYS, attrs, NULL );
        if ( handle == INVALID_HANDLE_VALUE )
        {
            ERR("failed to create %s (error %d)\n",
                debugstr_w( f->TargetPath ), GetLastError() );
            return 0;
        }

        f->state = msifs_installed;
        return (INT_PTR) handle;
    }
    case fdintCLOSE_FILE_INFO:
    {
        FILETIME ft;
        FILETIME ftLocal;
        HANDLE handle = (HANDLE) pfdin->hf;

        if (!DosDateTimeToFileTime(pfdin->date, pfdin->time, &ft))
            return -1;
        if (!LocalFileTimeToFileTime(&ft, &ftLocal))
            return -1;
        if (!SetFileTime(handle, &ftLocal, 0, &ftLocal))
            return -1;
        CloseHandle(handle);
        return 1;
    }
    default:
        return 0;
    }
}

/***********************************************************************
 *            extract_cabinet_file
 *
 * Extract files from a cab file.
 */
static BOOL extract_cabinet_file(MSIPACKAGE* package, LPCWSTR source, 
                                 LPCWSTR path)
{
    HFDI hfdi;
    ERF erf;
    BOOL ret;
    char *cabinet;
    char *cab_path;
    static CHAR empty[] = "";
    CabData data;

    TRACE("Extracting %s to %s\n",debugstr_w(source), debugstr_w(path));

    hfdi = FDICreate(cabinet_alloc,
                     cabinet_free,
                     cabinet_open,
                     cabinet_read,
                     cabinet_write,
                     cabinet_close,
                     cabinet_seek,
                     0,
                     &erf);
    if (!hfdi)
    {
        ERR("FDICreate failed\n");
        return FALSE;
    }

    if (!(cabinet = strdupWtoA( source )))
    {
        FDIDestroy(hfdi);
        return FALSE;
    }
    if (!(cab_path = strdupWtoA( path )))
    {
        FDIDestroy(hfdi);
        msi_free(cabinet);
        return FALSE;
    }

    data.package = package;
    data.cab_path = cab_path;

    ret = FDICopy(hfdi, cabinet, empty, 0, cabinet_notify, NULL, &data);

    if (!ret)
        ERR("FDICopy failed\n");

    FDIDestroy(hfdi);

    msi_free(cabinet);
    msi_free(cab_path);

    return ret;
}

static VOID set_file_source(MSIPACKAGE* package, MSIFILE* file, MSICOMPONENT*
        comp, LPCWSTR path)
{
    if (!file->IsCompressed)
    {
        LPWSTR p, path;
        p = resolve_folder(package, comp->Directory, TRUE, FALSE, NULL);
        path = build_directory_name(2, p, file->ShortName);
        if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW( path ))
        {
            msi_free(path);
            path = build_directory_name(2, p, file->LongName);
        }
        file->SourcePath = path;
        msi_free(p);
    }
    else
        file->SourcePath = build_directory_name(2, path, file->File);
}

struct media_info {
    UINT last_sequence; 
    LPWSTR last_volume;
    LPWSTR last_path;
    DWORD count;
    WCHAR source[MAX_PATH];
};

static struct media_info *create_media_info( void )
{
    struct media_info *mi;

    mi = msi_alloc( sizeof *mi  );
    if (mi)
    {
        mi->last_sequence = 0; 
        mi->last_volume = NULL;
        mi->last_path = NULL;
        mi->count = 0;
        mi->source[0] = 0;
    }

    return mi;
}

static void free_media_info( struct media_info *mi )
{
    msi_free( mi->last_path );
    msi_free( mi );
}

/* downloads a remote cabinet and extracts it if it exists */
static UINT msi_extract_remote_cabinet( MSIPACKAGE *package, struct media_info *mi )
{
    FDICABINETINFO cabinfo;
    WCHAR temppath[MAX_PATH];
    WCHAR src[MAX_PATH];
    LPSTR cabpath;
    LPCWSTR file;
    LPWSTR ptr;
    HFDI hfdi;
    ERF erf;
    int hf;

    /* the URL is the path prefix of the package URL and the filename
     * of the file to download
     */
    ptr = strrchrW(package->PackagePath, '/');
    lstrcpynW(src, package->PackagePath, ptr - package->PackagePath + 2);
    ptr = strrchrW(mi->source, '\\');
    lstrcatW(src, ptr + 1);

    file = msi_download_file( src, temppath );
    lstrcpyW(mi->source, file);

    /* check if the remote cabinet still exists, ignore if it doesn't */
    hfdi = FDICreate(cabinet_alloc, cabinet_free, cabinet_open, cabinet_read,
                     cabinet_write, cabinet_close, cabinet_seek, 0, &erf);
    if (!hfdi)
    {
        ERR("FDICreate failed\n");
        return ERROR_FUNCTION_FAILED;
    }

    cabpath = strdupWtoA(mi->source);
    hf = cabinet_open(cabpath, _O_RDONLY, 0);
    if (!FDIIsCabinet(hfdi, hf, &cabinfo))
    {
        WARN("Remote cabinet %s does not exist.\n", debugstr_w(mi->source));
        msi_free(cabpath);
        return ERROR_SUCCESS;
    }

    msi_free(cabpath);
    return !extract_cabinet_file(package, mi->source, mi->last_path);
}

static UINT ready_media_for_file( MSIPACKAGE *package, struct media_info *mi,
                                  MSIFILE *file )
{
    UINT rc = ERROR_SUCCESS;
    MSIRECORD * row = 0;
    static const WCHAR ExecSeqQuery[] =
        {'S','E','L','E','C','T',' ','*',' ', 'F','R','O','M',' ',
         '`','M','e','d','i','a','`',' ','W','H','E','R','E',' ',
         '`','L','a','s','t','S','e','q','u','e','n','c','e','`',' ','>','=',
         ' ','%', 'i',' ','O','R','D','E','R',' ','B','Y',' ',
         '`','L','a','s','t','S','e','q','u','e','n','c','e','`',0};
    LPCWSTR cab, volume;
    DWORD sz;
    INT seq;
    LPCWSTR prompt;
    MSICOMPONENT *comp = file->Component;

    if (file->Sequence <= mi->last_sequence)
    {
        set_file_source(package,file,comp,mi->last_path);
        TRACE("Media already ready (%u, %u)\n",file->Sequence,mi->last_sequence);
        return ERROR_SUCCESS;
    }

    mi->count ++;
    row = MSI_QueryGetRecord(package->db, ExecSeqQuery, file->Sequence);
    if (!row)
    {
        TRACE("Unable to query row\n");
        return ERROR_FUNCTION_FAILED;
    }

    volume = MSI_RecordGetString(row, 5);
    prompt = MSI_RecordGetString(row, 3);

    msi_free(mi->last_path);
    mi->last_path = NULL;

    if (!file->IsCompressed)
    {
        mi->last_path = resolve_folder(package, comp->Directory, TRUE, FALSE, NULL);
        set_file_source(package,file,comp,mi->last_path);

        MsiSourceListAddMediaDiskW(package->ProductCode, NULL, 
            MSIINSTALLCONTEXT_USERMANAGED, MSICODE_PRODUCT, mi->count, volume,
            prompt);

        MsiSourceListSetInfoW(package->ProductCode, NULL, 
                MSIINSTALLCONTEXT_USERMANAGED, 
                MSICODE_PRODUCT|MSISOURCETYPE_MEDIA,
                INSTALLPROPERTY_LASTUSEDSOURCEW, mi->last_path);
        msiobj_release(&row->hdr);
        return rc;
    }

    seq = MSI_RecordGetInteger(row,2);
    mi->last_sequence = seq;

    cab = MSI_RecordGetString(row,4);
    if (cab)
    {
        TRACE("Source is CAB %s\n",debugstr_w(cab));
        /* the stream does not contain the # character */
        if (cab[0]=='#')
        {
            LPWSTR path;

            writeout_cabinet_stream(package,&cab[1],mi->source);
            mi->last_path = strdupW(mi->source);
            *(strrchrW(mi->last_path,'\\')+1)=0;

            path = msi_dup_property( package, cszSourceDir );

            MsiSourceListAddMediaDiskW(package->ProductCode, NULL, 
                MSIINSTALLCONTEXT_USERMANAGED, MSICODE_PRODUCT, mi->count,
                volume, prompt);

            MsiSourceListSetInfoW(package->ProductCode, NULL,
                MSIINSTALLCONTEXT_USERMANAGED,
                MSICODE_PRODUCT|MSISOURCETYPE_NETWORK,
                INSTALLPROPERTY_LASTUSEDSOURCEW, path);

            msi_free(path);
        }
        else
        {
            sz = MAX_PATH;
            mi->last_path = msi_alloc(MAX_PATH*sizeof(WCHAR));
            if (MSI_GetPropertyW(package, cszSourceDir, mi->source, &sz))
            {
                ERR("No Source dir defined\n");
                rc = ERROR_FUNCTION_FAILED;
            }
            else
            {
                strcpyW(mi->last_path,mi->source);
                strcatW(mi->source,cab);

                MsiSourceListSetInfoW(package->ProductCode, NULL,
                            MSIINSTALLCONTEXT_USERMANAGED,
                            MSICODE_PRODUCT|MSISOURCETYPE_MEDIA,
                            INSTALLPROPERTY_LASTUSEDSOURCEW, mi->last_path);

                /* extract the cab file into a folder in the temp folder */
                sz = MAX_PATH;
                if (MSI_GetPropertyW(package, cszTempFolder,mi->last_path, &sz) 
                                    != ERROR_SUCCESS)
                    GetTempPathW(MAX_PATH,mi->last_path);
            }
        }

        /* only download the remote cabinet file if a local copy does not exist */
        if (GetFileAttributesW(mi->source) == INVALID_FILE_ATTRIBUTES &&
            UrlIsW(package->PackagePath, URLIS_URL))
        {
            rc = msi_extract_remote_cabinet(package, mi);
        }
        else
        {
            rc = !extract_cabinet_file(package, mi->source, mi->last_path);
        }
    }
    else
    {
        sz = MAX_PATH;
        mi->last_path = msi_alloc(MAX_PATH*sizeof(WCHAR));
        MSI_GetPropertyW(package,cszSourceDir,mi->source,&sz);
        strcpyW(mi->last_path,mi->source);

        MsiSourceListSetInfoW(package->ProductCode, NULL,
                    MSIINSTALLCONTEXT_USERMANAGED,
                    MSICODE_PRODUCT|MSISOURCETYPE_MEDIA,
                    INSTALLPROPERTY_LASTUSEDSOURCEW, mi->last_path);
    }
    set_file_source(package, file, comp, mi->last_path);

    MsiSourceListAddMediaDiskW(package->ProductCode, NULL,
            MSIINSTALLCONTEXT_USERMANAGED, MSICODE_PRODUCT, mi->count, volume,
            prompt);

    msiobj_release(&row->hdr);

    return rc;
}

static UINT get_file_target(MSIPACKAGE *package, LPCWSTR file_key, 
                            MSIFILE** file)
{
    LIST_FOR_EACH_ENTRY( *file, &package->files, MSIFILE, entry )
    {
        if (lstrcmpW( file_key, (*file)->File )==0)
        {
            if ((*file)->state >= msifs_overwrite)
                return ERROR_SUCCESS;
            else
                return ERROR_FILE_NOT_FOUND;
        }
    }

    return ERROR_FUNCTION_FAILED;
}

/*
 * ACTION_InstallFiles()
 * 
 * For efficiency, this is done in two passes:
 * 1) Correct all the TargetPaths and determine what files are to be installed.
 * 2) Extract Cabinets and copy files.
 */
UINT ACTION_InstallFiles(MSIPACKAGE *package)
{
    struct media_info *mi;
    UINT rc = ERROR_SUCCESS;
    LPWSTR ptr;
    MSIFILE *file;

    /* increment progress bar each time action data is sent */
    ui_progress(package,1,1,0,0);

    /* handle the keys for the SourceList */
    ptr = strrchrW(package->PackagePath,'\\');
    if (ptr)
    {
        ptr ++;
        MsiSourceListSetInfoW(package->ProductCode, NULL,
                MSIINSTALLCONTEXT_USERMANAGED,
                MSICODE_PRODUCT,
                INSTALLPROPERTY_PACKAGENAMEW, ptr);
    }
    /* FIXME("Write DiskPrompt\n"); */
    
    /* Pass 1 */
    LIST_FOR_EACH_ENTRY( file, &package->files, MSIFILE, entry )
    {
        if (!ACTION_VerifyComponentForAction( file->Component, INSTALLSTATE_LOCAL ))
        {
            ui_progress(package,2,file->FileSize,0,0);
            TRACE("File %s is not scheduled for install\n",
                   debugstr_w(file->File));

            file->state = msifs_skipped;
        }
    }

    /*
     * Despite MSDN specifying that the CreateFolders action
     * should be called before InstallFiles, some installers don't
     * do that, and they seem to work correctly.  We need to create
     * directories here to make sure that the files can be copied.
     */
    msi_create_component_directories( package );

    mi = create_media_info();

    /* Pass 2 */
    LIST_FOR_EACH_ENTRY( file, &package->files, MSIFILE, entry )
    {
        if (file->state != msifs_missing && file->state != msifs_overwrite)
            continue;

        TRACE("Pass 2: %s\n",debugstr_w(file->File));

        rc = ready_media_for_file( package, mi, file );
        if (rc != ERROR_SUCCESS)
        {
            ERR("Unable to ready media\n");
            rc = ERROR_FUNCTION_FAILED;
            break;
        }

        TRACE("file paths %s to %s\n",debugstr_w(file->SourcePath),
              debugstr_w(file->TargetPath));

        if (file->state != msifs_missing && file->state != msifs_overwrite)
            continue;

        /* compressed files are extracted in ready_media_for_file */
        if (file->IsCompressed)
        {
            if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(file->TargetPath))
                ERR("compressed file wasn't extracted (%s)\n",
                    debugstr_w(file->TargetPath));
            continue;
        }

        rc = CopyFileW(file->SourcePath,file->TargetPath,FALSE);
        if (!rc)
        {
            rc = GetLastError();
            ERR("Unable to copy file (%s -> %s) (error %d)\n",
                debugstr_w(file->SourcePath), debugstr_w(file->TargetPath), rc);
            if (rc == ERROR_ALREADY_EXISTS && file->state == msifs_overwrite)
            {
                rc = 0;
            }
            else if (rc == ERROR_FILE_NOT_FOUND)
            {
                ERR("Source File Not Found!  Continuing\n");
                rc = 0;
            }
            else if (file->Attributes & msidbFileAttributesVital)
            {
                ERR("Ignoring Error and continuing (nonvital file)...\n");
                rc = 0;
            }
        }
        else
        {
            file->state = msifs_installed;
            rc = ERROR_SUCCESS;
        }
    }

    /* cleanup */
    free_media_info( mi );
    return rc;
}

static UINT ITERATE_DuplicateFiles(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE *package = (MSIPACKAGE*)param;
    WCHAR dest_name[0x100];
    LPWSTR dest_path, dest;
    LPCWSTR file_key, component;
    DWORD sz;
    DWORD rc;
    MSICOMPONENT *comp;
    MSIFILE *file;

    component = MSI_RecordGetString(row,2);
    comp = get_loaded_component(package,component);

    if (!ACTION_VerifyComponentForAction( comp, INSTALLSTATE_LOCAL ))
    {
        TRACE("Skipping copy due to disabled component %s\n",
                        debugstr_w(component));

        /* the action taken was the same as the current install state */        
        comp->Action = comp->Installed;

        return ERROR_SUCCESS;
    }

    comp->Action = INSTALLSTATE_LOCAL;

    file_key = MSI_RecordGetString(row,3);
    if (!file_key)
    {
        ERR("Unable to get file key\n");
        return ERROR_FUNCTION_FAILED;
    }

    rc = get_file_target(package,file_key,&file);

    if (rc != ERROR_SUCCESS)
    {
        ERR("Original file unknown %s\n",debugstr_w(file_key));
        return ERROR_SUCCESS;
    }

    if (MSI_RecordIsNull(row,4))
        strcpyW(dest_name,strrchrW(file->TargetPath,'\\')+1);
    else
    {
        sz=0x100;
        MSI_RecordGetStringW(row,4,dest_name,&sz);
        reduce_to_longfilename(dest_name);
    }

    if (MSI_RecordIsNull(row,5))
    {
        LPWSTR p;
        dest_path = strdupW(file->TargetPath);
        p = strrchrW(dest_path,'\\');
        if (p)
            *p=0;
    }
    else
    {
        LPCWSTR destkey;
        destkey = MSI_RecordGetString(row,5);
        dest_path = resolve_folder(package, destkey, FALSE,FALSE,NULL);
        if (!dest_path)
        {
            /* try a Property */
            dest_path = msi_dup_property( package, destkey );
            if (!dest_path)
            {
                FIXME("Unable to get destination folder, try AppSearch properties\n");
                return ERROR_SUCCESS;
            }
        }
    }

    dest = build_directory_name(2, dest_path, dest_name);

    TRACE("Duplicating file %s to %s\n",debugstr_w(file->TargetPath),
                    debugstr_w(dest)); 

    if (strcmpW(file->TargetPath,dest))
        rc = !CopyFileW(file->TargetPath,dest,TRUE);
    else
        rc = ERROR_SUCCESS;

    if (rc != ERROR_SUCCESS)
        ERR("Failed to copy file %s -> %s, last error %d\n",
            debugstr_w(file->TargetPath), debugstr_w(dest_path), GetLastError());

    FIXME("We should track these duplicate files as well\n");   

    msi_free(dest_path);
    msi_free(dest);

    msi_file_update_ui(package, file, szDuplicateFiles);

    return ERROR_SUCCESS;
}

UINT ACTION_DuplicateFiles(MSIPACKAGE *package)
{
    UINT rc;
    MSIQUERY * view;
    static const WCHAR ExecSeqQuery[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','D','u','p','l','i','c','a','t','e','F','i','l','e','`',0};

    rc = MSI_DatabaseOpenViewW(package->db, ExecSeqQuery, &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords(view, NULL, ITERATE_DuplicateFiles, package);
    msiobj_release(&view->hdr);

    return rc;
}

/* compares the version of a file read from the filesystem and
 * the version specified in the File table
 */
static int msi_compare_file_version( MSIFILE *file )
{
    WCHAR version[MAX_PATH];
    DWORD size;
    UINT r;

    size = MAX_PATH;
    version[0] = '\0';
    r = MsiGetFileVersionW( file->TargetPath, version, &size, NULL, NULL );
    if ( r != ERROR_SUCCESS )
        return 0;

    return lstrcmpW( version, file->Version );
}

UINT ACTION_RemoveFiles( MSIPACKAGE *package )
{
    MSIFILE *file;

    LIST_FOR_EACH_ENTRY( file, &package->files, MSIFILE, entry )
    {
        MSIRECORD *uirow;
        LPWSTR uipath, p;

        if ( !file->Component )
            continue;
        if ( file->Component->Installed == INSTALLSTATE_LOCAL )
            continue;

        if ( file->state == msifs_installed )
            ERR("removing installed file %s\n", debugstr_w(file->TargetPath));

        if ( file->state != msifs_present )
            continue;

        /* only remove a file if the version to be installed
         * is strictly newer than the old file
         */
        if ( msi_compare_file_version( file ) >= 0 )
            continue;

        TRACE("removing %s\n", debugstr_w(file->File) );
        if ( !DeleteFileW( file->TargetPath ) )
            ERR("failed to delete %s\n",  debugstr_w(file->TargetPath) );
        file->state = msifs_missing;

        /* the UI chunk */
        uirow = MSI_CreateRecord( 9 );
        MSI_RecordSetStringW( uirow, 1, file->FileName );
        uipath = strdupW( file->TargetPath );
        p = strrchrW(uipath,'\\');
        if (p)
            p[1]=0;
        MSI_RecordSetStringW( uirow, 9, uipath);
        ui_actiondata( package, szRemoveFiles, uirow);
        msiobj_release( &uirow->hdr );
        msi_free( uipath );
        /* FIXME: call ui_progress here? */
    }

    return ERROR_SUCCESS;
}
