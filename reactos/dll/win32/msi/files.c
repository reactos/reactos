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
#include "wine/unicode.h"
#include "action.h"

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
    TRACE("wrote %li bytes to %s\n",write,debugstr_w(source));
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

static void msi_file_update_ui( MSIPACKAGE *package, MSIFILE *f )
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
    ui_actiondata( package, szInstallFiles, uirow);
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

        file = strdupAtoW(pfdin->psz1);
        f = get_loaded_file(data->package, file);
        msi_free(file);

        if (!f)
        {
            ERR("Unknown File in Cabinet (%s)\n",debugstr_a(pfdin->psz1));
            return 0;
        }

        if (f->state != msifs_missing && f->state != msifs_overwrite)
        {
            TRACE("Skipping extraction of %s\n",debugstr_a(pfdin->psz1));
            return 0;
        }

        msi_file_update_ui( data->package, f );

        TRACE("extracting %s\n", debugstr_w(f->TargetPath) );

        handle = CreateFileW( f->TargetPath, GENERIC_READ | GENERIC_WRITE, 0,
                              NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL );
        if ( handle == INVALID_HANDLE_VALUE )
        {
            ERR("failed to create %s (error %ld)\n",
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

    ret = FDICopy(hfdi, cabinet, "", 0, cabinet_notify, NULL, &data);

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
    if (file->Attributes & msidbFileAttributesNoncompressed)
    {
        LPWSTR p;
        p = resolve_folder(package, comp->Directory, TRUE, FALSE, NULL);
        file->SourcePath = build_directory_name(2, p, file->ShortName);
        msi_free(p);
    }
    else
        file->SourcePath = build_directory_name(2, path, file->File);
}

static BOOL check_volume(LPCWSTR path, LPCWSTR want_volume, LPWSTR volume, 
        UINT *intype)
{
    WCHAR drive[4];
    WCHAR name[MAX_PATH];
    UINT type;

    if (!(path[0] && path[1] == ':'))
        return TRUE;

    drive[0] = path[0];
    drive[1] = path[1];
    drive[2] = '\\';
    drive[3] = 0;
    TRACE("Checking volume %s .. (%s)\n",debugstr_w(drive), debugstr_w(want_volume));
    type = GetDriveTypeW(drive);
    TRACE("drive is of type %x\n",type);

    if (type == DRIVE_UNKNOWN || type == DRIVE_NO_ROOT_DIR || 
            type == DRIVE_FIXED || type == DRIVE_RAMDISK)
        return TRUE;

    GetVolumeInformationW(drive, name, MAX_PATH, NULL, NULL, NULL, NULL, 0);
    TRACE("Drive contains %s\n", debugstr_w(name));
    volume = strdupW(name);
    if (*intype)
        *intype=type;
    return (strcmpiW(want_volume,name)==0);
}

static BOOL check_for_sourcefile(LPCWSTR source)
{
    DWORD attrib = GetFileAttributesW(source);
    return (!(attrib == INVALID_FILE_ATTRIBUTES));
}

static UINT ready_volume(MSIPACKAGE* package, LPCWSTR path, LPWSTR last_volume, 
                         MSIRECORD *row,UINT *type )
{
    LPWSTR volume = NULL; 
    LPCWSTR want_volume = MSI_RecordGetString(row, 5);
    BOOL ok = check_volume(path, want_volume, volume, type);

    TRACE("Readying Volume for %s (%s, %s)\n", debugstr_w(path),
          debugstr_w(want_volume), debugstr_w(last_volume));

    if (check_for_sourcefile(path) && !ok)
    {
        FIXME("Found the Sourcefile but not on the correct volume.(%s,%s,%s)\n",
                debugstr_w(path),debugstr_w(want_volume), debugstr_w(volume));
        return ERROR_SUCCESS;
    }

    while (!ok)
    {
        INT rc;
        LPCWSTR prompt;
        LPWSTR msg;
      
        prompt = MSI_RecordGetString(row,3);
        msg = generate_error_string(package, 1302, 1, prompt);
        rc = MessageBoxW(NULL,msg,NULL,MB_OKCANCEL);
        msi_free(volume);
        msi_free(msg);
        if (rc == IDOK)
            ok = check_for_sourcefile(path);
        else
            return ERROR_INSTALL_USEREXIT;
    }

    msi_free(last_volume);
    last_volume = strdupW(volume);
    return ERROR_SUCCESS;
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
    UINT type;
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

    seq = MSI_RecordGetInteger(row,2);
    mi->last_sequence = seq;

    volume = MSI_RecordGetString(row, 5);
    prompt = MSI_RecordGetString(row, 3);

    msi_free(mi->last_path);
    mi->last_path = NULL;

    if (file->Attributes & msidbFileAttributesNoncompressed)
    {
        mi->last_path = resolve_folder(package, comp->Directory, TRUE, FALSE, NULL);
        set_file_source(package,file,comp,mi->last_path);
        rc = ready_volume(package, file->SourcePath, mi->last_volume, row,&type);

        MsiSourceListAddMediaDiskW(package->ProductCode, NULL, 
            MSIINSTALLCONTEXT_USERMANAGED, MSICODE_PRODUCT, mi->count, volume,
            prompt);

        if (type == DRIVE_REMOVABLE || type == DRIVE_CDROM || 
                type == DRIVE_RAMDISK)
            MsiSourceListSetInfoW(package->ProductCode, NULL, 
                MSIINSTALLCONTEXT_USERMANAGED, 
                MSICODE_PRODUCT|MSISOURCETYPE_MEDIA,
                INSTALLPROPERTY_LASTUSEDSOURCEW, mi->last_path);
        else
            MsiSourceListSetInfoW(package->ProductCode, NULL, 
                MSIINSTALLCONTEXT_USERMANAGED, 
                MSICODE_PRODUCT|MSISOURCETYPE_NETWORK,
                INSTALLPROPERTY_LASTUSEDSOURCEW, mi->last_path);
        msiobj_release(&row->hdr);
        return rc;
    }

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

                rc = ready_volume(package, mi->source, mi->last_volume, row, &type);
                if (type == DRIVE_REMOVABLE || type == DRIVE_CDROM || 
                        type == DRIVE_RAMDISK)
                    MsiSourceListSetInfoW(package->ProductCode, NULL,
                            MSIINSTALLCONTEXT_USERMANAGED,
                            MSICODE_PRODUCT|MSISOURCETYPE_MEDIA,
                            INSTALLPROPERTY_LASTUSEDSOURCEW, mi->last_path);
                else
                    MsiSourceListSetInfoW(package->ProductCode, NULL,
                            MSIINSTALLCONTEXT_USERMANAGED,
                            MSICODE_PRODUCT|MSISOURCETYPE_NETWORK,
                            INSTALLPROPERTY_LASTUSEDSOURCEW, mi->last_path);

                /* extract the cab file into a folder in the temp folder */
                sz = MAX_PATH;
                if (MSI_GetPropertyW(package, cszTempFolder,mi->last_path, &sz) 
                                    != ERROR_SUCCESS)
                    GetTempPathW(MAX_PATH,mi->last_path);
            }
        }
        rc = !extract_cabinet_file(package, mi->source, mi->last_path);
    }
    else
    {
        sz = MAX_PATH;
        mi->last_path = msi_alloc(MAX_PATH*sizeof(WCHAR));
        MSI_GetPropertyW(package,cszSourceDir,mi->source,&sz);
        strcpyW(mi->last_path,mi->source);
        rc = ready_volume(package, mi->last_path, mi->last_volume, row, &type);

        if (type == DRIVE_REMOVABLE || type == DRIVE_CDROM || 
                type == DRIVE_RAMDISK)
            MsiSourceListSetInfoW(package->ProductCode, NULL,
                    MSIINSTALLCONTEXT_USERMANAGED,
                    MSICODE_PRODUCT|MSISOURCETYPE_MEDIA,
                    INSTALLPROPERTY_LASTUSEDSOURCEW, mi->last_path);
        else
            MsiSourceListSetInfoW(package->ProductCode, NULL,
                    MSIINSTALLCONTEXT_USERMANAGED,
                    MSICODE_PRODUCT|MSISOURCETYPE_NETWORK,
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
                                   LPWSTR* file_source)
{
    MSIFILE *file;

    LIST_FOR_EACH_ENTRY( file, &package->files, MSIFILE, entry )
    {
        if (lstrcmpW( file_key, file->File )==0)
        {
            if (file->state >= msifs_overwrite)
            {
                *file_source = strdupW( file->TargetPath );
                return ERROR_SUCCESS;
            }
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
        if (~file->Attributes & msidbFileAttributesNoncompressed)
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
    WCHAR *file_source = NULL;
    WCHAR dest_name[0x100];
    LPWSTR dest_path, dest;
    LPCWSTR file_key, component;
    DWORD sz;
    DWORD rc;
    MSICOMPONENT *comp;

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

    rc = get_file_target(package,file_key,&file_source);

    if (rc != ERROR_SUCCESS)
    {
        ERR("Original file unknown %s\n",debugstr_w(file_key));
        msi_free(file_source);
        return ERROR_SUCCESS;
    }

    if (MSI_RecordIsNull(row,4))
        strcpyW(dest_name,strrchrW(file_source,'\\')+1);
    else
    {
        sz=0x100;
        MSI_RecordGetStringW(row,4,dest_name,&sz);
        reduce_to_longfilename(dest_name);
    }

    if (MSI_RecordIsNull(row,5))
    {
        LPWSTR p;
        dest_path = strdupW(file_source);
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
                msi_free(file_source);
                return ERROR_SUCCESS;
            }
        }
    }

    dest = build_directory_name(2, dest_path, dest_name);

    TRACE("Duplicating file %s to %s\n",debugstr_w(file_source),
                    debugstr_w(dest)); 

    if (strcmpW(file_source,dest))
        rc = !CopyFileW(file_source,dest,TRUE);
    else
        rc = ERROR_SUCCESS;

    if (rc != ERROR_SUCCESS)
        ERR("Failed to copy file %s -> %s, last error %ld\n",
            debugstr_w(file_source), debugstr_w(dest_path), GetLastError());

    FIXME("We should track these duplicate files as well\n");   

    msi_free(dest_path);
    msi_free(dest);
    msi_free(file_source);

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

UINT ACTION_RemoveFiles( MSIPACKAGE *package )
{
    MSIFILE *file;

    LIST_FOR_EACH_ENTRY( file, &package->files, MSIFILE, entry )
    {
        if ( !file->Component )
            continue;
        if ( file->Component->Installed == INSTALLSTATE_LOCAL )
            continue;

        if ( file->state == msifs_installed )
            ERR("removing installed file %s\n", debugstr_w(file->TargetPath));

        if ( file->state != msifs_present )
            continue;

        TRACE("removing %s\n", debugstr_w(file->File) );
        if ( !DeleteFileW( file->TargetPath ) )
            ERR("failed to delete %s\n",  debugstr_w(file->TargetPath) );
        file->state = msifs_missing;
    }

    return ERROR_SUCCESS;
}
