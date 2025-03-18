/*
 * Setupapi file queue routines
 *
 * Copyright 2002 Alexandre Julliard for CodeWeavers
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

#ifdef __WINESRC__
#include <stdlib.h>
#endif

#ifdef __REACTOS__
#include <aclapi.h>
#endif

/* context structure for the default queue callback */
struct default_callback_context
{
    DWORD     magic;
    HWND      owner;
    DWORD     unk1[4];
    DWORD_PTR unk2[7];
    HWND      progress;
    UINT      message;
    DWORD_PTR unk3[5];
};

struct source_media
{
    WCHAR root[MAX_PATH];
    WCHAR *desc, *tag;
    BOOL resolved;
    BOOL cabinet;
};

struct file_op
{
    struct file_op *next;
    UINT            style;
    WCHAR          *src_path;
    WCHAR          *src_file;
    WCHAR          *dst_path;
    WCHAR          *dst_file;
#ifdef __REACTOS__
    PSECURITY_DESCRIPTOR  dst_sd;
#endif
    struct source_media *media;
};

struct file_op_queue
{
    struct file_op *head;
    struct file_op *tail;
    unsigned int count;
};

struct file_queue
{
    DWORD magic;
    struct file_op_queue copy_queue;
    struct file_op_queue delete_queue;
    struct file_op_queue rename_queue;
    DWORD flags;
    struct source_media **sources;
    unsigned int source_count;
};

#define FILE_QUEUE_MAGIC 0x21514653

/* append a file operation to a queue */
static inline void queue_file_op( struct file_op_queue *queue, struct file_op *op )
{
    op->next = NULL;
    if (queue->tail) queue->tail->next = op;
    else queue->head = op;
    queue->tail = op;
    queue->count++;
}

/* free all the file operations on a given queue */
static void free_file_op_queue( struct file_op_queue *queue )
{
    struct file_op *t, *op = queue->head;

    while( op )
    {
        free( op->src_path );
        free( op->src_file );
        free( op->dst_path );
#ifdef __REACTOS__
        if (op->dst_sd) LocalFree(op->dst_sd);
#endif
        if (op->dst_file != op->src_file) free( op->dst_file );
        t = op;
        op = op->next;
        free( t );
    }
}

/* concat 3 strings to make a path, handling separators correctly */
static void concat_W( WCHAR *buffer, const WCHAR *src1, const WCHAR *src2, const WCHAR *src3 )
{
    *buffer = 0;
    if (src1 && *src1)
    {
        lstrcpyW( buffer, src1 );
        buffer += lstrlenW(buffer );
        if (buffer[-1] != '\\') *buffer++ = '\\';
        *buffer = 0;
        if (src2) while (*src2 == '\\') src2++;
    }

    if (src2)
    {
        lstrcpyW( buffer, src2 );
        buffer += lstrlenW(buffer );
        if (buffer[-1] != '\\') *buffer++ = '\\';
        *buffer = 0;
        if (src3) while (*src3 == '\\') src3++;
    }

    if (src3)
        lstrcpyW( buffer, src3 );
}


/***********************************************************************
 *            build_filepathsW
 *
 * Build a FILEPATHS_W structure for a given file operation.
 */
static BOOL build_filepathsW( const struct file_op *op, FILEPATHS_W *paths )
{
    unsigned int src_len = 1, dst_len = 1;
    WCHAR *source = (PWSTR)paths->Source, *target = (PWSTR)paths->Target;

#ifdef __WINESRC__
    if (!op->src_file || op->src_file[0] != '@')
#endif
    {
        if (op->media) src_len += lstrlenW(op->media->root) + 1;
        if (op->src_path) src_len += lstrlenW(op->src_path) + 1;
    }
    if (op->src_file) src_len += lstrlenW(op->src_file) + 1;
    if (op->dst_path) dst_len += lstrlenW(op->dst_path) + 1;
    if (op->dst_file) dst_len += lstrlenW(op->dst_file) + 1;
    src_len *= sizeof(WCHAR);
    dst_len *= sizeof(WCHAR);

    if (!source || HeapSize( GetProcessHeap(), 0, source ) < src_len )
    {
        HeapFree( GetProcessHeap(), 0, source );
        paths->Source = source = HeapAlloc( GetProcessHeap(), 0, src_len );
    }
    if (!target || HeapSize( GetProcessHeap(), 0, target ) < dst_len )
    {
        HeapFree( GetProcessHeap(), 0, target );
        paths->Target = target = HeapAlloc( GetProcessHeap(), 0, dst_len );
    }
    if (!source || !target) return FALSE;
#ifdef __WINESRC__
    if (!op->src_file || op->src_file[0] != '@')
#endif
        concat_W( source, op->media ? op->media->root : NULL, op->src_path, op->src_file );
#ifdef __WINESRC__
    else
        lstrcpyW( source, op->src_file );
#endif
    concat_W( target, NULL, op->dst_path, op->dst_file );
    paths->Win32Error = 0;
    paths->Flags      = 0;
    return TRUE;
}


/***********************************************************************
 *            QUEUE_callback_WtoA
 *
 * Map a file callback parameters from W to A and call the A callback.
 */
UINT CALLBACK QUEUE_callback_WtoA( void *context, UINT notification,
                                   UINT_PTR param1, UINT_PTR param2 )
{
    struct callback_WtoA_context *callback_ctx = context;
    char buffer[MAX_PATH];
    UINT ret;
    UINT_PTR old_param2 = param2;

    switch(notification)
    {
    case SPFILENOTIFY_COPYERROR:
        buffer[0] = 0;
        param2 = (UINT_PTR)buffer;
        /* fall through */
    case SPFILENOTIFY_STARTDELETE:
    case SPFILENOTIFY_ENDDELETE:
    case SPFILENOTIFY_DELETEERROR:
    case SPFILENOTIFY_STARTRENAME:
    case SPFILENOTIFY_ENDRENAME:
    case SPFILENOTIFY_RENAMEERROR:
    case SPFILENOTIFY_STARTCOPY:
    case SPFILENOTIFY_ENDCOPY:
    case SPFILENOTIFY_QUEUESCAN_EX:
        {
            FILEPATHS_W *pathsW = (FILEPATHS_W *)param1;
            FILEPATHS_A pathsA;

            pathsA.Source     = strdupWtoA( pathsW->Source );
            pathsA.Target     = strdupWtoA( pathsW->Target );
            pathsA.Win32Error = pathsW->Win32Error;
            pathsA.Flags      = pathsW->Flags;
            ret = callback_ctx->orig_handler( callback_ctx->orig_context, notification,
                                              (UINT_PTR)&pathsA, param2 );
            HeapFree( GetProcessHeap(), 0, (void *)pathsA.Source );
            HeapFree( GetProcessHeap(), 0, (void *)pathsA.Target );
        }
        if (notification == SPFILENOTIFY_COPYERROR)
            MultiByteToWideChar( CP_ACP, 0, buffer, -1, (WCHAR *)old_param2, MAX_PATH );
        break;

    case SPFILENOTIFY_STARTREGISTRATION:
    case SPFILENOTIFY_ENDREGISTRATION:
        {
            SP_REGISTER_CONTROL_STATUSW *statusW = (SP_REGISTER_CONTROL_STATUSW *)param1;
            SP_REGISTER_CONTROL_STATUSA statusA;

            statusA.cbSize = sizeof(statusA);
            statusA.FileName = strdupWtoA( statusW->FileName );
            statusA.Win32Error  = statusW->Win32Error;
            statusA.FailureCode = statusW->FailureCode;
            ret = callback_ctx->orig_handler( callback_ctx->orig_context, notification,
                                              (UINT_PTR)&statusA, param2 );
            free( (char *)statusA.FileName );
        }
        break;

    case SPFILENOTIFY_QUEUESCAN:
        {
            LPWSTR targetW = (LPWSTR)param1;
            LPSTR target = strdupWtoA( targetW );

            ret = callback_ctx->orig_handler( callback_ctx->orig_context, notification,
                                              (UINT_PTR)target, param2 );
            free( target );
        }
        break;

    case SPFILENOTIFY_NEEDMEDIA:
    {
        const SOURCE_MEDIA_W *mediaW = (const SOURCE_MEDIA_W *)param1;
        char path[MAX_PATH];
        SOURCE_MEDIA_A mediaA;

        mediaA.Tagfile = strdupWtoA(mediaW->Tagfile);
        mediaA.Description = strdupWtoA(mediaW->Description);
        mediaA.SourcePath = strdupWtoA(mediaW->SourcePath);
        mediaA.SourceFile = strdupWtoA(mediaW->SourceFile);
        mediaA.Flags = mediaW->Flags;
        path[0] = 0;

        ret = callback_ctx->orig_handler(callback_ctx->orig_context, notification,
                (UINT_PTR)&mediaA, (UINT_PTR)&path);
        MultiByteToWideChar(CP_ACP, 0, path, -1, (WCHAR *)param2, MAX_PATH);

        free((char *)mediaA.Tagfile);
        free((char *)mediaA.Description);
        free((char *)mediaA.SourcePath);
        free((char *)mediaA.SourceFile);
        break;
    }
    case SPFILENOTIFY_STARTQUEUE:
    case SPFILENOTIFY_ENDQUEUE:
    case SPFILENOTIFY_STARTSUBQUEUE:
    case SPFILENOTIFY_ENDSUBQUEUE:
    default:
        ret = callback_ctx->orig_handler( callback_ctx->orig_context, notification, param1, param2 );
        break;
    }
    return ret;
}

#ifdef __REACTOS__
/***********************************************************************
 *            get_src_file_info
 *
 * Retrieve the source file information for a given file.
 */
static void get_src_file_info( HINF hinf, struct file_op *op, PWSTR* psrc_root, PWSTR* psrc_descr, PWSTR* psrc_tag)
{
    INFCONTEXT file_ctx, disk_ctx;
    INT id, diskid;
    DWORD len, len2;
    WCHAR SectionName[MAX_PATH];

    /* find the SourceDisksFiles entry */
    if(!SetupDiGetActualSectionToInstallW(hinf, L"SourceDisksFiles", SectionName, ARRAY_SIZE(SectionName), NULL, NULL))
        return;
    if (!SetupFindFirstLineW( hinf, SectionName, op->src_file, &file_ctx ))
    {
        if ((op->style & (SP_COPY_SOURCE_ABSOLUTE|SP_COPY_SOURCEPATH_ABSOLUTE))) return;
        /* no specific info, use .inf file source directory */
        if (!*psrc_root) *psrc_root = PARSER_get_src_root(hinf);
        return;
    }
    if (!SetupGetIntField( &file_ctx, 1, &diskid )) return;

    /* now find the diskid in the SourceDisksNames section */
    if(!SetupDiGetActualSectionToInstallW(hinf, L"SourceDisksNames", SectionName, ARRAY_SIZE(SectionName), NULL, NULL))
        return;
    if (!SetupFindFirstLineW( hinf, SectionName, NULL, &disk_ctx )) return;
    for (;;)
    {
        if (SetupGetIntField( &disk_ctx, 0, &id ) && (id == diskid)) break;
        if (!SetupFindNextLine( &disk_ctx, &disk_ctx )) return;
    }

    /* and fill in the missing info */

    if (!*psrc_descr)
    {
        if (SetupGetStringFieldW( &disk_ctx, 1, NULL, 0, &len ) &&
            (*psrc_descr = malloc(len*sizeof(WCHAR))))
            SetupGetStringFieldW( &disk_ctx, 1, *psrc_descr, len, NULL );
    }
    if (!*psrc_tag)
    {
        if (SetupGetStringFieldW( &disk_ctx, 2, NULL, 0, &len ) &&
            (*psrc_tag = malloc(len*sizeof(WCHAR))))
            SetupGetStringFieldW( &disk_ctx, 2, *psrc_tag, len, NULL );
    }
    if (!op->src_path && !(op->style & SP_COPY_SOURCE_ABSOLUTE))
    {
        len = len2 = 0;
        if (!(op->style & SP_COPY_SOURCEPATH_ABSOLUTE))
        {
            /* retrieve relative path for this disk */
            if (!SetupGetStringFieldW( &disk_ctx, 4, NULL, 0, &len )) len = 0;
        }
        /* retrieve relative path for this file */
        if (!SetupGetStringFieldW( &file_ctx, 2, NULL, 0, &len2 )) len2 = 0;

        if ((len || len2) &&
            (op->src_path = malloc((len+len2)*sizeof(WCHAR))))
        {
            WCHAR *ptr = op->src_path;
            if (len)
            {
                SetupGetStringFieldW( &disk_ctx, 4, op->src_path, len, NULL );
                ptr = op->src_path + lstrlenW(op->src_path);
                if (len2 && ptr > op->src_path && ptr[-1] != '\\') *ptr++ = '\\';
            }
            if (!SetupGetStringFieldW( &file_ctx, 2, ptr, len2, NULL )) *ptr = 0;
        }
    }
    if (!*psrc_root) *psrc_root = PARSER_get_src_root(hinf);
}
#endif // __REACTOS__

static void get_source_info( HINF hinf, const WCHAR *src_file, SP_FILE_COPY_PARAMS_W *params,
                             WCHAR *src_root, WCHAR *src_path)
{
    INFCONTEXT file_ctx, disk_ctx;
    INT id, diskid;
    DWORD len;

    /* find the SourceDisksFiles entry */
    if (!SetupFindFirstLineW( hinf, L"SourceDisksFiles", src_file, &file_ctx )) return;
    if (!SetupGetIntField( &file_ctx, 1, &diskid )) return;

    /* now find the diskid in the SourceDisksNames section */
    if (!SetupFindFirstLineW( hinf, L"SourceDisksNames", NULL, &disk_ctx )) return;
    for (;;)
    {
        if (SetupGetIntField( &disk_ctx, 0, &id ) && (id == diskid)) break;
        if (!SetupFindNextLine( &disk_ctx, &disk_ctx )) return;
    }

    if (SetupGetStringFieldW( &disk_ctx, 1, NULL, 0, &len ) && len > sizeof(WCHAR)
            && (params->SourceDescription = malloc( len * sizeof(WCHAR) )))
        SetupGetStringFieldW( &disk_ctx, 1, (WCHAR *)params->SourceDescription, len, NULL );

    if (SetupGetStringFieldW( &disk_ctx, 2, NULL, 0, &len ) && len > sizeof(WCHAR)
            && (params->SourceTagfile = malloc( len * sizeof(WCHAR) )))
        SetupGetStringFieldW( &disk_ctx, 2, (WCHAR *)params->SourceTagfile, len, NULL );

    if (SetupGetStringFieldW( &disk_ctx, 4, NULL, 0, &len ) && len > sizeof(WCHAR)
            && len < MAX_PATH - lstrlenW( src_root ) - 1)
    {
        lstrcatW( src_root, L"\\" );
        SetupGetStringFieldW( &disk_ctx, 4, src_root + lstrlenW( src_root ),
                              MAX_PATH - lstrlenW( src_root ), NULL );
    }

    if (SetupGetStringFieldW( &file_ctx, 2, NULL, 0, &len ) && len > sizeof(WCHAR) && len < MAX_PATH)
    {
        SetupGetStringFieldW( &file_ctx, 2, src_path, MAX_PATH, NULL );
        params->SourcePath = src_path;
    }
}

/***********************************************************************
 *            get_destination_dir
 *
 * Retrieve the destination dir for a given section.
 */
WCHAR *get_destination_dir( HINF hinf, const WCHAR *section )
{
    INFCONTEXT context;
    WCHAR systemdir[MAX_PATH], *dir;
    BOOL ret;

    if (!section || !(ret = SetupFindFirstLineW( hinf, L"DestinationDirs", section, &context )))
        ret = SetupFindFirstLineW( hinf, L"DestinationDirs", L"DefaultDestDir", &context );

    if (ret && (dir = PARSER_get_dest_dir( &context )))
        return dir;

    GetSystemDirectoryW( systemdir, MAX_PATH );
    return wcsdup( systemdir );
}

struct extract_cab_ctx
{
    const WCHAR *src;
    const WCHAR *dst;
};

static UINT WINAPI extract_cab_cb( void *arg, UINT message, UINT_PTR param1, UINT_PTR param2 )
{
    struct extract_cab_ctx *ctx = arg;

    switch (message)
    {
    case SPFILENOTIFY_FILEINCABINET:
    {
        FILE_IN_CABINET_INFO_W *info = (FILE_IN_CABINET_INFO_W *)param1;
        const WCHAR *filename;

        if ((filename = wcsrchr( info->NameInCabinet, '\\' )))
            filename++;
        else
            filename = info->NameInCabinet;

        if (lstrcmpiW( filename, ctx->src ))
            return FILEOP_SKIP;

        lstrcpyW( info->FullTargetName, ctx->dst );
        return FILEOP_DOIT;
    }
    case SPFILENOTIFY_FILEEXTRACTED:
    {
        const FILEPATHS_W *paths = (const FILEPATHS_W *)param1;
        return paths->Win32Error;
    }
    case SPFILENOTIFY_NEEDNEWCABINET:
    {
        const CABINET_INFO_W *info = (const CABINET_INFO_W *)param1;
        lstrcpyW( (WCHAR *)param2, info->CabinetPath );
        return ERROR_SUCCESS;
    }
    case SPFILENOTIFY_CABINETINFO:
        return 0;
    default:
        FIXME("Unexpected message %#x.\n", message);
        return 0;
    }
}

/***********************************************************************
 *            extract_cabinet_file
 *
 * Extract a file from a .cab file.
 */
static BOOL extract_cabinet_file( const WCHAR *cabinet, const WCHAR *root,
                                  const WCHAR *src, const WCHAR *dst )
{
    struct extract_cab_ctx ctx = {src, dst};
    WCHAR path[MAX_PATH];

#ifdef __REACTOS__
    TRACE("extract_cabinet_file(cab = '%s' ; root = '%s' ; src = '%s' ; dst = '%s')\n",
          debugstr_w(cabinet), debugstr_w(root), debugstr_w(src), debugstr_w(dst));
#else
    int len = lstrlenW( cabinet );
    /* make sure the cabinet file has a .cab extension */
    if (len <= 4 || wcsicmp( cabinet + len - 4, L".cab" )) return FALSE;
#endif
    lstrcpyW(path, root);
    lstrcatW(path, L"\\" );
    lstrcatW(path, cabinet);

    return SetupIterateCabinetW( path, 0, extract_cab_cb, &ctx );
}


/***********************************************************************
 *            SetupOpenFileQueue   (SETUPAPI.@)
 */
HSPFILEQ WINAPI SetupOpenFileQueue(void)
{
    struct file_queue *queue;

    if (!(queue = calloc( 1, sizeof(*queue) )))
        return INVALID_HANDLE_VALUE;
    queue->magic = FILE_QUEUE_MAGIC;
    return queue;
}


/***********************************************************************
 *            SetupCloseFileQueue   (SETUPAPI.@)
 */
BOOL WINAPI SetupCloseFileQueue( HSPFILEQ handle )
{
    struct file_queue *queue = handle;
    unsigned int i;

    /* Windows XP DDK installer passes the handle returned from
     * SetupInitDefaultQueueCallback() to this function. */
    if (queue->magic != FILE_QUEUE_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    free_file_op_queue( &queue->copy_queue );
    free_file_op_queue( &queue->rename_queue );
    free_file_op_queue( &queue->delete_queue );
    for (i = 0; i < queue->source_count; ++i)
    {
        free( queue->sources[i]->desc );
        free( queue->sources[i]->tag );
        free( queue->sources[i] );
    }
    free( queue->sources );
    free( queue );
    return TRUE;
}


/***********************************************************************
 *            SetupQueueCopyIndirectA   (SETUPAPI.@)
 */
BOOL WINAPI SetupQueueCopyIndirectA( SP_FILE_COPY_PARAMS_A *paramsA )
{
    SP_FILE_COPY_PARAMS_W paramsW;
    BOOL ret;

    paramsW.cbSize = sizeof(paramsW);
    paramsW.QueueHandle = paramsA->QueueHandle;
    paramsW.SourceRootPath = strdupAtoW( paramsA->SourceRootPath );
    paramsW.SourcePath = strdupAtoW( paramsA->SourcePath );
    paramsW.SourceFilename = strdupAtoW( paramsA->SourceFilename );
    paramsW.SourceDescription = strdupAtoW( paramsA->SourceDescription );
    paramsW.SourceTagfile = strdupAtoW( paramsA->SourceTagfile );
    paramsW.TargetDirectory = strdupAtoW( paramsA->TargetDirectory );
    paramsW.TargetFilename = strdupAtoW( paramsA->TargetFilename );
    paramsW.CopyStyle = paramsA->CopyStyle;
    paramsW.LayoutInf = paramsA->LayoutInf;
    paramsW.SecurityDescriptor = strdupAtoW( paramsA->SecurityDescriptor );

    ret = SetupQueueCopyIndirectW( &paramsW );

    free( (WCHAR *)paramsW.SourceRootPath );
    free( (WCHAR *)paramsW.SourcePath );
    free( (WCHAR *)paramsW.SourceFilename );
    free( (WCHAR *)paramsW.SourceDescription );
    free( (WCHAR *)paramsW.SourceTagfile );
    free( (WCHAR *)paramsW.TargetDirectory );
    free( (WCHAR *)paramsW.TargetFilename );
    free( (WCHAR *)paramsW.SecurityDescriptor );
    return ret;
}

static BOOL equal_str(const WCHAR *a, const WCHAR *b)
{
    return (!a && !b) || (a && b && !wcscmp(a, b));
}

static struct source_media *get_source_media(struct file_queue *queue,
        const WCHAR *root, const WCHAR *desc, const WCHAR *tag)
{
    unsigned int i;

    for (i = 0; i < queue->source_count; ++i)
    {
        if (!wcscmp(root, queue->sources[i]->root)
                && equal_str(desc, queue->sources[i]->desc)
                && equal_str(tag, queue->sources[i]->tag))
        {
            return queue->sources[i];
        }
    }

    queue->sources = realloc( queue->sources, ++queue->source_count * sizeof(*queue->sources) );
    queue->sources[i] = malloc( sizeof(*queue->sources[i]) );
    lstrcpyW(queue->sources[i]->root, root);
    queue->sources[i]->desc = wcsdup( desc );
    queue->sources[i]->tag = wcsdup( tag );
    queue->sources[i]->resolved = FALSE;
    queue->sources[i]->cabinet = FALSE;

    return queue->sources[i];
}

/***********************************************************************
 *            SetupQueueCopyIndirectW   (SETUPAPI.@)
 */
BOOL WINAPI SetupQueueCopyIndirectW( PSP_FILE_COPY_PARAMS_W params )
{
    struct file_queue *queue = params->QueueHandle;
    struct file_op *op;
#ifdef __REACTOS__
    PCWSTR org_src_root = NULL, org_src_descr = NULL, org_src_tag = NULL;
    PWSTR src_root = NULL, src_descr = NULL, src_tag = NULL;
#endif

    if (!(op = malloc( sizeof(*op) ))) return FALSE;
    op->style      = params->CopyStyle;
    op->src_path   = wcsdup( params->SourcePath );
    op->src_file   = wcsdup( params->SourceFilename );
    op->dst_path   = wcsdup( params->TargetDirectory );
    op->dst_file   = wcsdup( params->TargetFilename );
#ifdef __REACTOS__
    op->dst_sd     = NULL;
    if (params->SecurityDescriptor)
        ConvertStringSecurityDescriptorToSecurityDescriptorW( params->SecurityDescriptor, SDDL_REVISION_1, &op->dst_sd, NULL );
#endif

    /* some defaults */
    if (!op->dst_file) op->dst_file = op->src_file;
    if (params->LayoutInf)
#ifdef __REACTOS__
    {
        get_src_file_info( params->LayoutInf, op, &src_root, &src_descr, &src_tag );
        org_src_root = params->SourceRootPath;     params->SourceRootPath = src_root;
        org_src_descr = params->SourceDescription; params->SourceDescription = src_descr;
        org_src_tag = params->SourceTagfile;       params->SourceTagfile = src_tag;
        if (!op->dst_path) op->dst_path = get_destination_dir( params->LayoutInf, op->dst_file );
    }
#else
        FIXME("Unhandled LayoutInf %p.\n", params->LayoutInf);
#endif

    op->media = get_source_media( queue, params->SourceRootPath ? params->SourceRootPath : L"",
                                  params->SourceDescription, params->SourceTagfile );

#ifdef __REACTOS__
    if (params->LayoutInf)
    {
        params->SourceRootPath = org_src_root;
        params->SourceDescription = org_src_descr;
        params->SourceTagfile = org_src_tag;
        if (src_root) free(src_root);
        if (src_descr) free(src_descr);
        if (src_tag) free(src_tag);
    }
#endif

    TRACE( "root=%s path=%s file=%s -> dir=%s file=%s  descr=%s tag=%s\n",
           debugstr_w(op->media->root), debugstr_w(op->src_path), debugstr_w(op->src_file),
           debugstr_w(op->dst_path), debugstr_w(op->dst_file),
           debugstr_w(op->media->desc), debugstr_w(op->media->tag) );

    queue_file_op( &queue->copy_queue, op );
    return TRUE;
}


/***********************************************************************
 *            SetupQueueCopyA   (SETUPAPI.@)
 */
BOOL WINAPI SetupQueueCopyA( HSPFILEQ queue, PCSTR src_root, PCSTR src_path, PCSTR src_file,
                             PCSTR src_descr, PCSTR src_tag, PCSTR dst_dir, PCSTR dst_file,
                             DWORD style )
{
    SP_FILE_COPY_PARAMS_A params;

    params.cbSize             = sizeof(params);
    params.QueueHandle        = queue;
    params.SourceRootPath     = src_root;
    params.SourcePath         = src_path;
    params.SourceFilename     = src_file;
    params.SourceDescription  = src_descr;
    params.SourceTagfile      = src_tag;
    params.TargetDirectory    = dst_dir;
    params.TargetFilename     = dst_file;
    params.CopyStyle          = style;
    params.LayoutInf          = 0;
    params.SecurityDescriptor = NULL;
    return SetupQueueCopyIndirectA( &params );
}


/***********************************************************************
 *            SetupQueueCopyW   (SETUPAPI.@)
 */
BOOL WINAPI SetupQueueCopyW( HSPFILEQ queue, PCWSTR src_root, PCWSTR src_path, PCWSTR src_file,
                             PCWSTR src_descr, PCWSTR src_tag, PCWSTR dst_dir, PCWSTR dst_file,
                             DWORD style )
{
    SP_FILE_COPY_PARAMS_W params;

    params.cbSize             = sizeof(params);
    params.QueueHandle        = queue;
    params.SourceRootPath     = src_root;
    params.SourcePath         = src_path;
    params.SourceFilename     = src_file;
    params.SourceDescription  = src_descr;
    params.SourceTagfile      = src_tag;
    params.TargetDirectory    = dst_dir;
    params.TargetFilename     = dst_file;
    params.CopyStyle          = style;
    params.LayoutInf          = 0;
    params.SecurityDescriptor = NULL;
    return SetupQueueCopyIndirectW( &params );
}


/***********************************************************************
 *            SetupQueueDefaultCopyA   (SETUPAPI.@)
 */
BOOL WINAPI SetupQueueDefaultCopyA( HSPFILEQ queue, HINF hinf, const char *src_rootA,
                                    const char *src_fileA, const char *dst_fileA, DWORD style )
{
    WCHAR src_rootW[MAX_PATH], src_fileW[MAX_PATH], dst_fileW[MAX_PATH];

    if (!src_rootA || !src_fileA || !dst_fileA)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    MultiByteToWideChar( CP_ACP, 0, src_rootA, -1, src_rootW, ARRAY_SIZE(src_rootW) );
    MultiByteToWideChar( CP_ACP, 0, src_fileA, -1, src_fileW, ARRAY_SIZE(src_fileW) );
    MultiByteToWideChar( CP_ACP, 0, dst_fileA, -1, dst_fileW, ARRAY_SIZE(dst_fileW) );
    return SetupQueueDefaultCopyW( queue, hinf, src_rootW, src_fileW, dst_fileW, style );
}


/***********************************************************************
 *            SetupQueueDefaultCopyW   (SETUPAPI.@)
 */
BOOL WINAPI SetupQueueDefaultCopyW( HSPFILEQ queue, HINF hinf, PCWSTR src_root, PCWSTR src_file,
                                    PCWSTR dst_file, DWORD style )
{
    WCHAR src_root_buffer[MAX_PATH], src_path[MAX_PATH];
    SP_FILE_COPY_PARAMS_W params;
    BOOL ret;

    if (!src_root || !src_file || !dst_file)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    params.cbSize             = sizeof(params);
    params.QueueHandle        = queue;
    params.SourceRootPath     = src_root_buffer;
    params.SourcePath         = NULL;
    params.SourceFilename     = src_file;
    params.SourceDescription  = NULL;
    params.SourceTagfile      = NULL;
    params.TargetFilename     = dst_file;
    params.CopyStyle          = style;
#ifdef __REACTOS__
    params.LayoutInf          = hinf;
#else
    params.LayoutInf          = NULL;
#endif
    params.SecurityDescriptor = NULL;

    lstrcpyW( src_root_buffer, src_root );
    src_path[0] = 0;
    if (!(params.TargetDirectory = get_destination_dir( hinf, NULL ))) return FALSE;
    get_source_info( hinf, src_file, &params, src_root_buffer, src_path );

    ret = SetupQueueCopyIndirectW( &params );

    free( (WCHAR *)params.TargetDirectory );
    free( (WCHAR *)params.SourceDescription );
    free( (WCHAR *)params.SourceTagfile );
    return ret;
}


/***********************************************************************
 *            SetupQueueDeleteA   (SETUPAPI.@)
 */
BOOL WINAPI SetupQueueDeleteA( HSPFILEQ handle, PCSTR part1, PCSTR part2 )
{
    struct file_queue *queue = handle;
    struct file_op *op;

    if (!(op = calloc( 1, sizeof(*op) ))) return FALSE;
    op->dst_path = strdupAtoW( part1 );
    op->dst_file = strdupAtoW( part2 );
    queue_file_op( &queue->delete_queue, op );
    return TRUE;
}


/***********************************************************************
 *            SetupQueueDeleteW   (SETUPAPI.@)
 */
BOOL WINAPI SetupQueueDeleteW( HSPFILEQ handle, PCWSTR part1, PCWSTR part2 )
{
    struct file_queue *queue = handle;
    struct file_op *op;

    if (!(op = calloc( 1, sizeof(*op) ))) return FALSE;
    op->dst_path = wcsdup( part1 );
    op->dst_file = wcsdup( part2 );
    queue_file_op( &queue->delete_queue, op );
    return TRUE;
}


/***********************************************************************
 *            SetupQueueRenameA   (SETUPAPI.@)
 */
BOOL WINAPI SetupQueueRenameA( HSPFILEQ handle, PCSTR SourcePath, PCSTR SourceFilename,
                               PCSTR TargetPath, PCSTR TargetFilename )
{
    struct file_queue *queue = handle;
    struct file_op *op;

    if (!(op = calloc( 1, sizeof(*op) ))) return FALSE;
    op->src_path = strdupAtoW( SourcePath );
    op->src_file = strdupAtoW( SourceFilename );
    op->dst_path = strdupAtoW( TargetPath ? TargetPath : SourcePath );
    op->dst_file = strdupAtoW( TargetFilename );
    queue_file_op( &queue->rename_queue, op );
    return TRUE;
}


/***********************************************************************
 *            SetupQueueRenameW   (SETUPAPI.@)
 */
BOOL WINAPI SetupQueueRenameW( HSPFILEQ handle, PCWSTR SourcePath, PCWSTR SourceFilename,
                               PCWSTR TargetPath, PCWSTR TargetFilename )
{
    struct file_queue *queue = handle;
    struct file_op *op;

    if (!(op = calloc( 1, sizeof(*op) ))) return FALSE;
    op->src_path = wcsdup( SourcePath );
    op->src_file = wcsdup( SourceFilename );
    op->dst_path = wcsdup( TargetPath ? TargetPath : SourcePath );
    op->dst_file = wcsdup( TargetFilename );
    queue_file_op( &queue->rename_queue, op );
    return TRUE;
}


/***********************************************************************
 *            SetupQueueCopySectionA   (SETUPAPI.@)
 */
BOOL WINAPI SetupQueueCopySectionA( HSPFILEQ queue, PCSTR src_root, HINF hinf, HINF hlist,
                                    PCSTR section, DWORD style )
{
    UNICODE_STRING sectionW;
    BOOL ret = FALSE;

    if (!RtlCreateUnicodeStringFromAsciiz( &sectionW, section ))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }
    if (!src_root)
        ret = SetupQueueCopySectionW( queue, NULL, hinf, hlist, sectionW.Buffer, style );
    else
    {
        UNICODE_STRING srcW;
        if (RtlCreateUnicodeStringFromAsciiz( &srcW, src_root ))
        {
            ret = SetupQueueCopySectionW( queue, srcW.Buffer, hinf, hlist, sectionW.Buffer, style );
            RtlFreeUnicodeString( &srcW );
        }
        else SetLastError( ERROR_NOT_ENOUGH_MEMORY );
    }
    RtlFreeUnicodeString( &sectionW );
    return ret;
}

/***********************************************************************
 *            SetupQueueCopySectionW   (SETUPAPI.@)
 */
BOOL WINAPI SetupQueueCopySectionW( HSPFILEQ queue, PCWSTR src_root, HINF hinf, HINF hlist,
                                    PCWSTR section, DWORD style )
{
    WCHAR src_root_buffer[MAX_PATH], src_path[MAX_PATH], src_file[MAX_PATH], dst_file[MAX_PATH], *dest_dir;
#ifdef __REACTOS__
    LPWSTR security_key, security_descriptor = NULL;
    INFCONTEXT security_context;
#endif
    INFCONTEXT context;
    SP_FILE_COPY_PARAMS_W params;
    INT flags;
    BOOL ret;
    DWORD len;

    TRACE("queue %p, src_root %s, hinf %p, hlist %p, section %s, style %#lx.\n",
            queue, debugstr_w(src_root), hinf, hlist, debugstr_w(section), style);

    if (!src_root)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

#ifdef __REACTOS__
    /* Check for .Security section */
    security_key = MyMalloc( (lstrlenW( section ) + lstrlenW( L".Security" )) * sizeof(WCHAR) + sizeof(UNICODE_NULL) );
    if (!security_key)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    lstrcpyW( security_key, section );
    lstrcatW( security_key, L".Security" );
    ret = SetupFindFirstLineW( hinf, security_key, NULL, &security_context );
    MyFree(security_key);
    if (ret)
    {
        DWORD required;
        if (!SetupGetLineTextW( &security_context, NULL, NULL, NULL, NULL, 0, &required ))
            return FALSE;
        security_descriptor = MyMalloc( required * sizeof(WCHAR) );
        if (!security_descriptor)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
        if (!SetupGetLineTextW( &security_context, NULL, NULL, NULL, security_descriptor, required, NULL ))
        {
            MyFree( security_descriptor );
            return FALSE;
        }
    }
#endif // __REACTOS__
    ret = FALSE;

    params.cbSize             = sizeof(params);
    params.QueueHandle        = queue;
    params.SourceRootPath     = src_root_buffer;
    params.SourceFilename     = src_file;
    params.TargetFilename     = dst_file;
    params.CopyStyle          = style;
#ifdef __REACTOS__
    params.LayoutInf          = hinf;
#else
    params.LayoutInf          = NULL;
#endif
    params.SecurityDescriptor = security_descriptor;

    lstrcpyW( src_root_buffer, src_root );

    if (!hlist) hlist = hinf;
    if (!hinf) hinf = hlist;
    if (!SetupFindFirstLineW( hlist, section, NULL, &context )) goto done;
    if (!(params.TargetDirectory = dest_dir = get_destination_dir( hinf, section ))) goto done;
    do
    {
        params.SourcePath = NULL;
        params.SourceDescription = NULL;
        params.SourceTagfile = NULL;
        lstrcpyW( src_root_buffer, src_root );
        src_path[0] = 0;

        if (!SetupGetStringFieldW( &context, 1, dst_file, ARRAY_SIZE( dst_file ), NULL ))
            goto end;
        if (!SetupGetStringFieldW( &context, 2, src_file, ARRAY_SIZE( src_file ), &len ) || len <= sizeof(WCHAR))
            lstrcpyW( src_file, dst_file );

        if (!SetupGetIntField( &context, 4, &flags )) flags = 0;  /* FIXME */

        get_source_info( hinf, src_file, &params, src_root_buffer, src_path );

        if (!SetupQueueCopyIndirectW( &params )) goto end;

        free( (WCHAR *)params.SourceDescription );
        free( (WCHAR *)params.SourceTagfile );
    } while (SetupFindNextLine( &context, &context ));
    ret = TRUE;

end:
    free( dest_dir );
done:
#ifdef __REACTOS__
    if (security_descriptor)
        MyFree( security_descriptor );
#endif
    return ret;
}


/***********************************************************************
 *            SetupQueueDeleteSectionA   (SETUPAPI.@)
 */
BOOL WINAPI SetupQueueDeleteSectionA( HSPFILEQ queue, HINF hinf, HINF hlist, PCSTR section )
{
    UNICODE_STRING sectionW;
    BOOL ret = FALSE;

    if (RtlCreateUnicodeStringFromAsciiz( &sectionW, section ))
    {
        ret = SetupQueueDeleteSectionW( queue, hinf, hlist, sectionW.Buffer );
        RtlFreeUnicodeString( &sectionW );
    }
    else SetLastError( ERROR_NOT_ENOUGH_MEMORY );
    return ret;
}


/***********************************************************************
 *            SetupQueueDeleteSectionW   (SETUPAPI.@)
 */
BOOL WINAPI SetupQueueDeleteSectionW( HSPFILEQ queue, HINF hinf, HINF hlist, PCWSTR section )
{
    INFCONTEXT context;
    WCHAR *dest_dir;
    WCHAR buffer[MAX_PATH];
    BOOL ret = FALSE;
    INT flags;

    TRACE( "hinf=%p/%p section=%s\n", hinf, hlist, debugstr_w(section) );

    if (!hlist) hlist = hinf;
    if (!SetupFindFirstLineW( hlist, section, NULL, &context )) return FALSE;
    if (!(dest_dir = get_destination_dir( hinf, section ))) return FALSE;
    do
    {
        if (!SetupGetStringFieldW( &context, 1, buffer, ARRAY_SIZE( buffer ), NULL ))
            goto done;
        if (!SetupGetIntField( &context, 4, &flags )) flags = 0;
        if (!SetupQueueDeleteW( queue, dest_dir, buffer )) goto done;
    } while (SetupFindNextLine( &context, &context ));

    ret = TRUE;
 done:
    free( dest_dir );
    return ret;
}


/***********************************************************************
 *            SetupQueueRenameSectionA   (SETUPAPI.@)
 */
BOOL WINAPI SetupQueueRenameSectionA( HSPFILEQ queue, HINF hinf, HINF hlist, PCSTR section )
{
    UNICODE_STRING sectionW;
    BOOL ret = FALSE;

    if (RtlCreateUnicodeStringFromAsciiz( &sectionW, section ))
    {
        ret = SetupQueueRenameSectionW( queue, hinf, hlist, sectionW.Buffer );
        RtlFreeUnicodeString( &sectionW );
    }
    else SetLastError( ERROR_NOT_ENOUGH_MEMORY );
    return ret;
}


/***********************************************************************
 *            SetupQueueRenameSectionW   (SETUPAPI.@)
 */
BOOL WINAPI SetupQueueRenameSectionW( HSPFILEQ queue, HINF hinf, HINF hlist, PCWSTR section )
{
    INFCONTEXT context;
    WCHAR *dest_dir;
    WCHAR src[MAX_PATH], dst[MAX_PATH];
    BOOL ret = FALSE;

    TRACE( "hinf=%p/%p section=%s\n", hinf, hlist, debugstr_w(section) );

    if (!hlist) hlist = hinf;
    if (!SetupFindFirstLineW( hlist, section, NULL, &context )) return FALSE;
    if (!(dest_dir = get_destination_dir( hinf, section ))) return FALSE;
    do
    {
        if (!SetupGetStringFieldW( &context, 1, dst, ARRAY_SIZE( dst ), NULL ))
            goto done;
        if (!SetupGetStringFieldW( &context, 2, src, ARRAY_SIZE( src ), NULL ))
            goto done;
        if (!SetupQueueRenameW( queue, dest_dir, src, NULL, dst )) goto done;
    } while (SetupFindNextLine( &context, &context ));

    ret = TRUE;
 done:
    free( dest_dir );
    return ret;
}


/***********************************************************************
 *            SetupCommitFileQueueA   (SETUPAPI.@)
 */
BOOL WINAPI SetupCommitFileQueueA( HWND owner, HSPFILEQ queue, PSP_FILE_CALLBACK_A handler,
                                   PVOID context )
{
    struct callback_WtoA_context ctx;

    ctx.orig_context = context;
    ctx.orig_handler = handler;
    return SetupCommitFileQueueW( owner, queue, QUEUE_callback_WtoA, &ctx );
}


/***********************************************************************
 *            create_full_pathW
 *
 * Recursively create all directories in the path.
 */
static BOOL create_full_pathW(const WCHAR *path)
{
    BOOL ret = TRUE;
    int len;
    WCHAR *new_path;

    new_path = malloc((lstrlenW(path) + 1) * sizeof(WCHAR));
    lstrcpyW(new_path, path);

    while((len = lstrlenW(new_path)) && new_path[len - 1] == '\\')
        new_path[len - 1] = 0;

    while(!CreateDirectoryW(new_path, NULL))
    {
        WCHAR *slash;
        DWORD last_error = GetLastError();

        if(last_error == ERROR_ALREADY_EXISTS)
            break;

        if(last_error != ERROR_PATH_NOT_FOUND)
        {
            ret = FALSE;
            break;
        }

        if(!(slash = wcsrchr(new_path, '\\')))
        {
            ret = FALSE;
            break;
        }

        len = slash - new_path;
        new_path[len] = 0;
        if(!create_full_pathW(new_path))
        {
            ret = FALSE;
            break;
        }
        new_path[len] = '\\';
    }

    free(new_path);
    return ret;
}

#ifdef __WINESRC__
static BOOL copy_file( LPCWSTR source, LPCWSTR target )
{
    WCHAR module[MAX_PATH];
    HMODULE mod = NULL;
    HRSRC res;
    HGLOBAL data;
    HANDLE handle;
    DWORD size, written;
    BOOL ret = FALSE;
    int id = 0;
    const WCHAR *p;

    TRACE( "%s -> %s\n", debugstr_w(source), debugstr_w(target) );

    if (source[0] != '@') return CopyFileW( source, target, FALSE );

    /* Wine extension: when the source of a file copy is in the format "@file.dll,-123"
     * the source data is extracted from the corresponding file.dll resource */

    source++;  /* skip '@' */
    p = wcschr( source, ',' );
    if (!p || p - source >= MAX_PATH)
    {
        SetLastError( ERROR_RESOURCE_DATA_NOT_FOUND );
        return FALSE;
    }
    memcpy( module, source, (p - source) * sizeof(WCHAR) );
    module[p - source] = 0;
    id = -wcstol( p + 1, NULL, 10 );
    if (id <= 0 || id > 0xffff ||
        !(mod = LoadLibraryExW( module, 0, LOAD_LIBRARY_AS_DATAFILE )) ||
        !(res = FindResourceW( mod, MAKEINTRESOURCEW(id), L"WINE_DATA_FILE" )) ||
        !(data = LoadResource( mod, res )))
    {
        WARN( "failed to save %s #%d to %s\n", debugstr_w(module), -id, debugstr_w(target) );
        if (mod) FreeLibrary( mod );
        SetLastError( ERROR_RESOURCE_DATA_NOT_FOUND );
        return FALSE;
    }
    size = SizeofResource( mod, res );
    if ((handle = CreateFileW( target, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                               CREATE_ALWAYS, 0, 0 )) == INVALID_HANDLE_VALUE)
    {
        WARN( "failed to save %s #%d to %s\n", debugstr_w(module), -id, debugstr_w(target) );
        if (mod) FreeLibrary( mod );
        return FALSE;
    }
    ret = WriteFile( handle, LockResource(data), size, &written, NULL ) && written == size;
    CloseHandle( handle );
    if (!ret) DeleteFileW( target );
    return ret;
}
#endif // __WINESRC__

static BOOL do_file_copyW( LPCWSTR source, LPCWSTR target, DWORD style,
                           PSP_FILE_CALLBACK_W handler, PVOID context )
{
    BOOL rc = FALSE;
    BOOL docopy = TRUE;
#ifdef __REACTOS__
    INT hSource, hTemp;
    OFSTRUCT OfStruct;
    WCHAR TempPath[MAX_PATH];
    WCHAR TempFile[MAX_PATH];
    LONG lRes;
    DWORD dwLastError;
#endif

    TRACE("copy %s to %s style 0x%lx\n",debugstr_w(source),debugstr_w(target),style);

#ifdef __REACTOS__
    /* Get a temp file name */
    if (!GetTempPathW(ARRAYSIZE(TempPath), TempPath))
    {
        ERR("GetTempPathW error\n");
        return FALSE;
    }

    /* Try to open the source file */
    hSource = LZOpenFileW((LPWSTR)source, &OfStruct, OF_READ);
    if (hSource < 0)
    {
        TRACE("LZOpenFileW(1) error %d %s\n", (int)hSource, debugstr_w(source));
        return FALSE;
    }

    if (!GetTempFileNameW(TempPath, L"", 0, TempFile))
    {
        dwLastError = GetLastError();

        ERR("GetTempFileNameW(%s) error\n", debugstr_w(TempPath));

        /* Close the source handle */
        LZClose(hSource);

        /* Restore error condition triggered by GetTempFileNameW */
        SetLastError(dwLastError);

        return FALSE;
    }

    /* Extract the compressed file to a temp location */
    hTemp = LZOpenFileW(TempFile, &OfStruct, OF_CREATE);
    if (hTemp < 0)
    {
        dwLastError = GetLastError();

        ERR("LZOpenFileW(2) error %d %s\n", (int)hTemp, debugstr_w(TempFile));

        /* Close the source handle */
        LZClose(hSource);

        /* Delete temp file if an error is signaled */
        DeleteFileW(TempFile);

        /* Restore error condition triggered by LZOpenFileW */
        SetLastError(dwLastError);

        return FALSE;
    }

    lRes = LZCopy(hSource, hTemp);

    dwLastError = GetLastError();

    LZClose(hSource);
    LZClose(hTemp);

    if (lRes < 0)
    {
        ERR("LZCopy error %d (%s, %s)\n", (int)lRes, debugstr_w(source), debugstr_w(TempFile));

        /* Delete temp file if copy was not successful */
        DeleteFileW(TempFile);

        /* Restore error condition triggered by LZCopy */
        SetLastError(dwLastError);

        return FALSE;
    }
#endif

    /* before copy processing */
    if (style & SP_COPY_REPLACEONLY)
    {
        if (GetFileAttributesW(target) == INVALID_FILE_ATTRIBUTES)
            docopy = FALSE;
    }
    if (style & (SP_COPY_NEWER_OR_SAME | SP_COPY_NEWER_ONLY | SP_COPY_FORCE_NEWER))
    {
        DWORD VersionSizeSource=0;
        DWORD VersionSizeTarget=0;
        DWORD zero=0;

        /*
         * This is sort of an interesting workaround. You see, calling
         * GetVersionInfoSize on a builtin dll loads that dll into memory
         * and we do not properly unload builtin dlls.. so we effectively
         * lock into memory all the targets we are replacing. This leads
         * to problems when we try to register the replaced dlls.
         *
         * So I will test for the existence of the files first so that
         * we just basically unconditionally replace the builtin versions.
         */
        if ((GetFileAttributesW(target) != INVALID_FILE_ATTRIBUTES) &&
            (GetFileAttributesW(TempFile) != INVALID_FILE_ATTRIBUTES))
        {
            VersionSizeSource = GetFileVersionInfoSizeW(TempFile,&zero);
            VersionSizeTarget = GetFileVersionInfoSizeW(target,&zero);
        }

        if (VersionSizeSource && VersionSizeTarget)
        {
            LPVOID VersionSource;
            LPVOID VersionTarget;
            VS_FIXEDFILEINFO *TargetInfo;
            VS_FIXEDFILEINFO *SourceInfo;
            UINT length;
            DWORD  ret;

            VersionSource = malloc(VersionSizeSource);
            VersionTarget = malloc(VersionSizeTarget);

            ret = GetFileVersionInfoW(TempFile,0,VersionSizeSource,VersionSource);
            if (ret)
              ret = GetFileVersionInfoW(target, 0, VersionSizeTarget,
                    VersionTarget);

            if (ret)
            {
                ret = VerQueryValueW(VersionSource, L"\\", (LPVOID*)&SourceInfo, &length);
                if (ret)
                    ret = VerQueryValueW(VersionTarget, L"\\", (LPVOID*)&TargetInfo, &length);

                if (ret)
                {
                    FILEPATHS_W filepaths;

                    TRACE("Versions: Source %li.%li target %li.%li\n",
                      SourceInfo->dwFileVersionMS, SourceInfo->dwFileVersionLS,
                      TargetInfo->dwFileVersionMS, TargetInfo->dwFileVersionLS);

                    /* used in case of notification */
                    filepaths.Target = target;
                    filepaths.Source = source;
                    filepaths.Win32Error = 0;
                    filepaths.Flags = 0;

                    if (TargetInfo->dwFileVersionMS > SourceInfo->dwFileVersionMS)
                    {
                        if (handler)
                            docopy = handler (context, SPFILENOTIFY_TARGETNEWER, (UINT_PTR)&filepaths, 0);
                        else
                            docopy = FALSE;
                    }
                    else if ((TargetInfo->dwFileVersionMS == SourceInfo->dwFileVersionMS)
                             && (TargetInfo->dwFileVersionLS > SourceInfo->dwFileVersionLS))
                    {
                        if (handler)
                            docopy = handler (context, SPFILENOTIFY_TARGETNEWER, (UINT_PTR)&filepaths, 0);
                        else
                            docopy = FALSE;
                    }
                    else if ((style & SP_COPY_NEWER_ONLY) &&
                        (TargetInfo->dwFileVersionMS ==
                         SourceInfo->dwFileVersionMS)
                        &&(TargetInfo->dwFileVersionLS ==
                        SourceInfo->dwFileVersionLS))
                    {
                        if (handler)
                            docopy = handler (context, SPFILENOTIFY_TARGETNEWER, (UINT_PTR)&filepaths, 0);
                        else
                            docopy = FALSE;
                    }
                }
            }
            free(VersionSource);
            free(VersionTarget);
        }
    }
    if (style & (SP_COPY_NOOVERWRITE | SP_COPY_FORCE_NOOVERWRITE))
    {
        if (GetFileAttributesW(target) != INVALID_FILE_ATTRIBUTES)
        {
            FIXME("Notify user target file exists\n");
            docopy = FALSE;
        }
    }
    if (style & (SP_COPY_NODECOMP | SP_COPY_LANGUAGEAWARE | SP_COPY_FORCE_IN_USE |
                 SP_COPY_NOSKIP | SP_COPY_WARNIFSKIP))
    {
        ERR("Unsupported style(s) 0x%lx\n",style);
    }

    if (docopy)
    {
#ifdef __REACTOS__
        rc = MoveFileExW(TempFile,target,MOVEFILE_REPLACE_EXISTING);
#else // __WINESRC__
        rc = copy_file( source, target );
#endif
        if (!rc && GetLastError() == ERROR_SHARING_VIOLATION &&
            (style & SP_COPY_IN_USE_NEEDS_REBOOT))
        {
#ifndef __REACTOS__
            WCHAR temp_file[MAX_PATH];
            WCHAR temp[MAX_PATH];

            if (GetTempPathW(MAX_PATH, temp) &&
                GetTempFileNameW(temp, L"SET", 0, temp_file))
            {
                rc = copy_file( source, temp_file );
                if (rc)
                    rc = MoveFileExW(temp_file, target, MOVEFILE_DELAY_UNTIL_REBOOT);
                else
                    DeleteFileW(temp_file);
            }
#else
            rc = MoveFileExW(TempFile, target, MOVEFILE_DELAY_UNTIL_REBOOT);
#endif
        }
        if (!rc) WARN( "failed to copy, err %lu\n", GetLastError() );
    }
    else
        SetLastError(ERROR_SUCCESS);

    /* after copy processing */
    if (style & SP_COPY_DELETESOURCE)
    {
       if (rc)
            DeleteFileW(source);
    }

    return rc;
}

/***********************************************************************
 *            SetupInstallFileExA   (SETUPAPI.@)
 */
BOOL WINAPI SetupInstallFileExA( HINF hinf, PINFCONTEXT inf_context, PCSTR source, PCSTR root,
                                 PCSTR dest, DWORD style, PSP_FILE_CALLBACK_A handler, PVOID context, PBOOL in_use )
{
    BOOL ret = FALSE;
    struct callback_WtoA_context ctx;
    UNICODE_STRING sourceW, rootW, destW;

    TRACE("%p %p %s %s %s %lx %p %p %p\n", hinf, inf_context, debugstr_a(source), debugstr_a(root),
          debugstr_a(dest), style, handler, context, in_use);

    sourceW.Buffer = rootW.Buffer = destW.Buffer = NULL;
    if (source && !RtlCreateUnicodeStringFromAsciiz( &sourceW, source ))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }
    if (root && !RtlCreateUnicodeStringFromAsciiz( &rootW, root ))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        goto exit;
    }
    if (dest && !RtlCreateUnicodeStringFromAsciiz( &destW, dest ))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        goto exit;
    }

    ctx.orig_context = context;
    ctx.orig_handler = handler;

    ret = SetupInstallFileExW( hinf, inf_context, sourceW.Buffer, rootW.Buffer, destW.Buffer, style, QUEUE_callback_WtoA, &ctx, in_use );

exit:
    RtlFreeUnicodeString( &sourceW );
    RtlFreeUnicodeString( &rootW );
    RtlFreeUnicodeString( &destW );
    return ret;
}

/***********************************************************************
 *            SetupInstallFileA   (SETUPAPI.@)
 */
BOOL WINAPI SetupInstallFileA( HINF hinf, PINFCONTEXT inf_context, PCSTR source, PCSTR root,
                               PCSTR dest, DWORD style, PSP_FILE_CALLBACK_A handler, PVOID context )
{
    return SetupInstallFileExA( hinf, inf_context, source, root, dest, style, handler, context, NULL );
}

/***********************************************************************
 *            SetupInstallFileExW   (SETUPAPI.@)
 */
BOOL WINAPI SetupInstallFileExW( HINF hinf, PINFCONTEXT inf_context, PCWSTR source, PCWSTR root,
                                 PCWSTR dest, DWORD style, PSP_FILE_CALLBACK_W handler, PVOID context, PBOOL in_use )
{
    BOOL ret, absolute = (root && *root && !(style & SP_COPY_SOURCE_ABSOLUTE));
    WCHAR *buffer, *p, *inf_source = NULL, dest_path[MAX_PATH];
    DWORD len;

    TRACE("%p %p %s %s %s %lx %p %p %p\n", hinf, inf_context, debugstr_w(source), debugstr_w(root),
          debugstr_w(dest), style, handler, context, in_use);

    if (in_use) FIXME("no file in use support\n");

    dest_path[0] = 0;

    if (hinf)
    {
        WCHAR *dest_dir;
        INFCONTEXT ctx;

        if (!inf_context)
        {
            inf_context = &ctx;
            if (!SetupFindFirstLineW( hinf, L"CopyFiles", NULL, inf_context )) return FALSE;
        }
        if (!SetupGetStringFieldW( inf_context, 1, NULL, 0, (PDWORD) &len )) return FALSE;
        if (!(inf_source = malloc( len * sizeof(WCHAR) )))
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return FALSE;
        }
        if (!SetupGetStringFieldW( inf_context, 1, inf_source, len, NULL ))
        {
            free( inf_source );
            return FALSE;
        }
        source = inf_source;

        if ((dest_dir = get_destination_dir( hinf, NULL )))
        {
            lstrcpyW( dest_path, dest_dir );
            lstrcatW( dest_path, L"\\" );
            free( dest_dir );
        }
    }
    else if (!source)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    len = lstrlenW( source ) + 1;
    if (absolute) len += lstrlenW( root ) + 1;

    if (!(p = buffer = malloc( len * sizeof(WCHAR) )))
    {
        free( inf_source );
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    if (absolute)
    {
        lstrcpyW( buffer, root );
        p += lstrlenW( buffer );
        if (p[-1] != '\\') *p++ = '\\';
    }
    while (*source == '\\') source++;
    lstrcpyW( p, source );

    lstrcatW( dest_path, dest );

    ret = do_file_copyW( buffer, dest_path, style, handler, context );

    free( inf_source );
    free( buffer );
    return ret;
}

/***********************************************************************
 *            SetupInstallFileW   (SETUPAPI.@)
 */
BOOL WINAPI SetupInstallFileW( HINF hinf, PINFCONTEXT inf_context, PCWSTR source, PCWSTR root,
                               PCWSTR dest, DWORD style, PSP_FILE_CALLBACK_W handler, PVOID context )
{
    return SetupInstallFileExW( hinf, inf_context, source, root, dest, style, handler, context, NULL );
}

static BOOL queue_copy_file( const WCHAR *source, const WCHAR *dest,
        const struct file_op *op, PSP_FILE_CALLBACK_W handler, void *context )
{
    TRACE("copying file %s -> %s\n", debugstr_w(source), debugstr_w(dest));

    if (op->dst_path && !create_full_pathW(op->dst_path))
        return FALSE;

    if (do_file_copyW(source, dest, op->style, handler, context) || GetLastError() == ERROR_SUCCESS)
        return TRUE;

    /* try to extract it from the cabinet file */
    if (op->media->tag && extract_cabinet_file(op->media->tag, op->media->root, op->src_file, dest))
    {
        op->media->cabinet = TRUE;
        return TRUE;
    }

    return FALSE;
}

/***********************************************************************
 *            SetupCommitFileQueueW   (SETUPAPI.@)
 */
BOOL WINAPI SetupCommitFileQueueW( HWND owner, HSPFILEQ handle, PSP_FILE_CALLBACK_W handler,
                                   PVOID context )
{
    struct file_queue *queue = handle;
    struct file_op *op;
    BOOL result = FALSE;
    FILEPATHS_W paths;
    UINT op_result;

    paths.Source = paths.Target = NULL;

    if (!queue->copy_queue.count && !queue->delete_queue.count && !queue->rename_queue.count)
        return TRUE;  /* nothing to do */

    if (!handler( context, SPFILENOTIFY_STARTQUEUE, (UINT_PTR)owner, 0 )) return FALSE;

    /* perform deletes */

    if (queue->delete_queue.count)
    {
        if (!(handler( context, SPFILENOTIFY_STARTSUBQUEUE, FILEOP_DELETE,
                       queue->delete_queue.count ))) goto done;
        for (op = queue->delete_queue.head; op; op = op->next)
        {
            build_filepathsW( op, &paths );
            op_result = handler( context, SPFILENOTIFY_STARTDELETE, (UINT_PTR)&paths, FILEOP_DELETE);
            if (op_result == FILEOP_ABORT) goto done;
            while (op_result == FILEOP_DOIT)
            {
                TRACE( "deleting file %s\n", debugstr_w(paths.Target) );
                if (DeleteFileW( paths.Target )) break;  /* success */
                paths.Win32Error = GetLastError();
                op_result = handler( context, SPFILENOTIFY_DELETEERROR, (UINT_PTR)&paths, 0 );
                if (op_result == FILEOP_ABORT) goto done;
            }
            handler( context, SPFILENOTIFY_ENDDELETE, (UINT_PTR)&paths, 0 );
        }
        handler( context, SPFILENOTIFY_ENDSUBQUEUE, FILEOP_DELETE, 0 );
    }

    /* perform renames */

    if (queue->rename_queue.count)
    {
        if (!(handler( context, SPFILENOTIFY_STARTSUBQUEUE, FILEOP_RENAME,
                       queue->rename_queue.count ))) goto done;
        for (op = queue->rename_queue.head; op; op = op->next)
        {
            build_filepathsW( op, &paths );
            op_result = handler( context, SPFILENOTIFY_STARTRENAME, (UINT_PTR)&paths, FILEOP_RENAME);
            if (op_result == FILEOP_ABORT) goto done;
            while (op_result == FILEOP_DOIT)
            {
                TRACE( "renaming file %s -> %s\n",
                       debugstr_w(paths.Source), debugstr_w(paths.Target) );
                if (MoveFileW( paths.Source, paths.Target )) break;  /* success */
                paths.Win32Error = GetLastError();
                op_result = handler( context, SPFILENOTIFY_RENAMEERROR, (UINT_PTR)&paths, 0 );
                if (op_result == FILEOP_ABORT) goto done;
            }
            handler( context, SPFILENOTIFY_ENDRENAME, (UINT_PTR)&paths, 0 );
        }
        handler( context, SPFILENOTIFY_ENDSUBQUEUE, FILEOP_RENAME, 0 );
    }

    /* perform copies */

    if (queue->copy_queue.count)
    {
        if (!(handler( context, SPFILENOTIFY_STARTSUBQUEUE, FILEOP_COPY,
                       queue->copy_queue.count ))) goto done;
        for (op = queue->copy_queue.head; op; op = op->next)
        {
            WCHAR newpath[MAX_PATH];

            if (!op->media->resolved)
            {
                /* The NEEDMEDIA callback asks for the folder containing the
                 * first file, but that might be in a subdir of the source
                 * disk's root directory. We have to do some contortions to
                 * correct for this. Pretend that the file we're using
                 * actually isn't in a subdirectory, but keep track of what it
                 * was, and then later strip it from the root path that we
                 * ultimately resolve the source disk to. */
                WCHAR src_path[MAX_PATH];
                size_t path_len = 0;

                src_path[0] = 0;
                if (op->src_path)
                {
                    lstrcpyW(src_path, op->src_path);
                    path_len = lstrlenW(src_path);

                    lstrcatW(op->media->root, L"\\");
                    lstrcatW(op->media->root, op->src_path);

                    free(op->src_path);
                    op->src_path = NULL;
                }

                for (;;)
                {
                    SOURCE_MEDIA_W media;
                    media.Reserved = NULL;
                    media.Tagfile = op->media->tag;
                    media.Description = op->media->desc;
                    media.SourcePath = op->media->root;
                    media.SourceFile = op->src_file;
                    media.Flags = op->style & (SP_COPY_WARNIFSKIP | SP_COPY_NOSKIP | SP_FLAG_CABINETCONTINUATION | SP_COPY_NOBROWSE);

                    newpath[0] = 0;
                    op_result = handler( context, SPFILENOTIFY_NEEDMEDIA, (UINT_PTR)&media, (UINT_PTR)newpath );

                    if (op_result == FILEOP_ABORT)
                        goto done;
                    else if (op_result == FILEOP_SKIP)
                        break;
                    else if (op_result == FILEOP_NEWPATH)
                        lstrcpyW(op->media->root, newpath);
                    else if (op_result != FILEOP_DOIT)
                        FIXME("Unhandled return value %#x.\n", op_result);

                    build_filepathsW( op, &paths );
                    op_result = handler( context, SPFILENOTIFY_STARTCOPY, (UINT_PTR)&paths, FILEOP_COPY );
                    if (op_result == FILEOP_ABORT)
                        goto done;
                    else if (op_result == FILEOP_SKIP)
                        break;
                    else if (op_result != FILEOP_DOIT)
                        FIXME("Unhandled return value %#x.\n", op_result);

                    if (queue_copy_file( paths.Source, paths.Target, op, handler, context ))
                    {
                        if (path_len > 0 && !op->media->cabinet)
                        {
                            size_t root_len = lstrlenW(op->media->root);
                            if (path_len <= root_len && !wcsnicmp(op->media->root + root_len - path_len, src_path, path_len))
                                op->media->root[root_len - path_len - 1] = 0;
                        }
                        op->media->resolved = TRUE;
#ifdef __REACTOS__
                        goto setDestSD;
#else
                        handler( context, SPFILENOTIFY_ENDCOPY, (UINT_PTR)&paths, 0 );
                        break;
#endif
                    }
                    paths.Win32Error = GetLastError();
                    if (paths.Win32Error == ERROR_PATH_NOT_FOUND ||
                        paths.Win32Error == ERROR_FILE_NOT_FOUND)
#if defined(__REACTOS__) // TEMP HACK!
                    {
                        ERR( "** Could not find source install media! ** copy error %d %s -> %s\n",
                            paths.Win32Error, debugstr_w(paths.Source), debugstr_w(paths.Target) );
                        break;
                    }
#else
                        continue;
#endif

                    newpath[0] = 0;
                    op_result = handler( context, SPFILENOTIFY_COPYERROR, (UINT_PTR)&paths, (UINT_PTR)newpath );
                    if (op_result == FILEOP_ABORT)
                        goto done;
                    else if (op_result == FILEOP_SKIP)
                        break;
                    else if (op_result == FILEOP_NEWPATH)
                    {
                        lstrcpyW(op->media->root, newpath);
                        build_filepathsW(op, &paths);
                    }
                    else if (op_result != FILEOP_DOIT)
                        FIXME("Unhandled return value %#x.\n", op_result);
                }
            }
            else
            {
                build_filepathsW( op, &paths );
                op_result = handler( context, SPFILENOTIFY_STARTCOPY, (UINT_PTR)&paths, FILEOP_COPY );
                if (op_result == FILEOP_ABORT)
                    goto done;
                else if (op_result == FILEOP_SKIP)
                    continue;
                else if (op_result != FILEOP_DOIT)
                    FIXME("Unhandled return value %#x.\n", op_result);

                while (op_result == FILEOP_DOIT || op_result == FILEOP_NEWPATH)
                {
                    if (queue_copy_file( paths.Source, paths.Target, op, handler, context ))
                        break;

                    paths.Win32Error = GetLastError();
                    newpath[0] = 0;
                    op_result = handler( context, SPFILENOTIFY_COPYERROR, (UINT_PTR)&paths, (UINT_PTR)newpath );
                    if (op_result == FILEOP_ABORT)
                        goto done;
                    else if (op_result == FILEOP_NEWPATH)
                    {
                        lstrcpyW(op->media->root, newpath);
                        build_filepathsW(op, &paths);
                    }
                    else if (op_result != FILEOP_SKIP && op_result != FILEOP_DOIT)
                        FIXME("Unhandled return value %#x.\n", op_result);
                }
#ifdef __REACTOS__
setDestSD:
                if (op->dst_sd)
                {
                    PSID psidOwner = NULL, psidGroup = NULL;
                    PACL pDacl = NULL, pSacl = NULL;
                    SECURITY_INFORMATION security_info = 0;
                    BOOL present, dummy;

                    if (GetSecurityDescriptorOwner( op->dst_sd, &psidOwner, &dummy ) && psidOwner)
                        security_info |= OWNER_SECURITY_INFORMATION;
                    if (GetSecurityDescriptorGroup( op->dst_sd, &psidGroup, &dummy ) && psidGroup)
                        security_info |= GROUP_SECURITY_INFORMATION;
                    if (GetSecurityDescriptorDacl( op->dst_sd, &present, &pDacl, &dummy ))
                        security_info |= DACL_SECURITY_INFORMATION;
                    if (GetSecurityDescriptorSacl( op->dst_sd, &present, &pSacl, &dummy ))
                        security_info |= DACL_SECURITY_INFORMATION;
                    SetNamedSecurityInfoW( (LPWSTR)paths.Target, SE_FILE_OBJECT, security_info,
                        psidOwner, psidGroup, pDacl, pSacl );
                    /* Yes, ignore the return code... */
                }
#endif // __REACTOS__
                handler( context, SPFILENOTIFY_ENDCOPY, (UINT_PTR)&paths, 0 );
            }
        }
        handler( context, SPFILENOTIFY_ENDSUBQUEUE, FILEOP_COPY, 0 );
    }


    result = TRUE;

 done:
    handler( context, SPFILENOTIFY_ENDQUEUE, result, 0 );
    HeapFree( GetProcessHeap(), 0, (void *)paths.Source );
    HeapFree( GetProcessHeap(), 0, (void *)paths.Target );
    return result;
}


/***********************************************************************
 *            SetupScanFileQueueA   (SETUPAPI.@)
 */
BOOL WINAPI SetupScanFileQueueA( HSPFILEQ handle, DWORD flags, HWND window,
                                 PSP_FILE_CALLBACK_A handler, PVOID context, PDWORD result )
{
    struct callback_WtoA_context ctx;

    TRACE("%p %lx %p %p %p %p\n", handle, flags, window, handler, context, result);

    ctx.orig_context = context;
    ctx.orig_handler = handler;

    return SetupScanFileQueueW( handle, flags, window, QUEUE_callback_WtoA, &ctx, result );
}


/***********************************************************************
 *            SetupScanFileQueueW   (SETUPAPI.@)
 */
BOOL WINAPI SetupScanFileQueueW( HSPFILEQ handle, DWORD flags, HWND window,
                                 PSP_FILE_CALLBACK_W handler, PVOID context, PDWORD result )
{
    struct file_queue *queue = handle;
    struct file_op *op;
    FILEPATHS_W paths;
    UINT notification = 0;
    BOOL ret = FALSE;

    TRACE("%p %lx %p %p %p %p\n", handle, flags, window, handler, context, result);

    *result = FALSE;

    if (!queue->copy_queue.count) return TRUE;

    if (flags & SPQ_SCAN_USE_CALLBACK)        notification = SPFILENOTIFY_QUEUESCAN;
    else if (flags & SPQ_SCAN_USE_CALLBACKEX) notification = SPFILENOTIFY_QUEUESCAN_EX;

    if (flags & ~(SPQ_SCAN_USE_CALLBACK | SPQ_SCAN_USE_CALLBACKEX))
    {
        FIXME("flags %lx not fully implemented\n", flags);
    }

    paths.Source = paths.Target = NULL;

    for (op = queue->copy_queue.head; op; op = op->next)
    {
        build_filepathsW( op, &paths );
        switch (notification)
        {
        case SPFILENOTIFY_QUEUESCAN:
            /* FIXME: handle delay flag */
            if (handler( context,  notification, (UINT_PTR)paths.Target, 0 )) goto done;
            break;
        case SPFILENOTIFY_QUEUESCAN_EX:
            if (handler( context, notification, (UINT_PTR)&paths, 0 )) goto done;
            break;
        default:
            ret = TRUE; goto done;
        }
    }

    *result = TRUE;

 done:
    HeapFree( GetProcessHeap(), 0, (void *)paths.Source );
    HeapFree( GetProcessHeap(), 0, (void *)paths.Target );
    return ret;
}


/***********************************************************************
 *            SetupGetFileQueueCount   (SETUPAPI.@)
 */
BOOL WINAPI SetupGetFileQueueCount( HSPFILEQ handle, UINT op, PUINT result )
{
    struct file_queue *queue = handle;

    switch(op)
    {
    case FILEOP_COPY:
        *result = queue->copy_queue.count;
        return TRUE;
    case FILEOP_RENAME:
        *result = queue->rename_queue.count;
        return TRUE;
    case FILEOP_DELETE:
        *result = queue->delete_queue.count;
        return TRUE;
    }
    return FALSE;
}


/***********************************************************************
 *            SetupGetFileQueueFlags   (SETUPAPI.@)
 */
BOOL WINAPI SetupGetFileQueueFlags( HSPFILEQ handle, PDWORD flags )
{
    struct file_queue *queue = handle;
    *flags = queue->flags;
    return TRUE;
}


/***********************************************************************
 *            SetupSetFileQueueFlags   (SETUPAPI.@)
 */
BOOL WINAPI SetupSetFileQueueFlags( HSPFILEQ handle, DWORD mask, DWORD flags )
{
    struct file_queue *queue = handle;
    queue->flags = (queue->flags & ~mask) | flags;
    return TRUE;
}


/***********************************************************************
 *   SetupSetFileQueueAlternatePlatformA  (SETUPAPI.@)
 */
BOOL WINAPI SetupSetFileQueueAlternatePlatformA(HSPFILEQ handle, PSP_ALTPLATFORM_INFO platform, PCSTR catalogfile)
{
    FIXME("(%p, %p, %s) stub!\n", handle, platform, debugstr_a(catalogfile));
    return FALSE;
}


/***********************************************************************
 *   SetupSetFileQueueAlternatePlatformW  (SETUPAPI.@)
 */
BOOL WINAPI SetupSetFileQueueAlternatePlatformW(HSPFILEQ handle, PSP_ALTPLATFORM_INFO platform, PCWSTR catalogfile)
{
    FIXME("(%p, %p, %s) stub!\n", handle, platform, debugstr_w(catalogfile));
    return FALSE;
}


/***********************************************************************
 *            SetupInitDefaultQueueCallback   (SETUPAPI.@)
 */
PVOID WINAPI SetupInitDefaultQueueCallback( HWND owner )
{
    return SetupInitDefaultQueueCallbackEx( owner, 0, 0, 0, NULL );
}


/***********************************************************************
 *            SetupInitDefaultQueueCallbackEx   (SETUPAPI.@)
 */
PVOID WINAPI SetupInitDefaultQueueCallbackEx( HWND owner, HWND progress, UINT msg,
                                              DWORD reserved1, PVOID reserved2 )
{
    struct default_callback_context *context;

    if ((context = calloc( 1, sizeof(*context) )))
    {
        context->magic    = 0x43515053; /* "SPQC" */
        context->owner    = owner;
        context->progress = progress;
        context->message  = msg;
    }
    return context;
}


/***********************************************************************
 *            SetupTermDefaultQueueCallback   (SETUPAPI.@)
 */
void WINAPI SetupTermDefaultQueueCallback( PVOID context )
{
    free( context );
}


/***********************************************************************
 *            SetupDefaultQueueCallbackA   (SETUPAPI.@)
 */
UINT WINAPI SetupDefaultQueueCallbackA( PVOID context, UINT notification,
                                        UINT_PTR param1, UINT_PTR param2 )
{
    FILEPATHS_A *paths = (FILEPATHS_A *)param1;
    struct default_callback_context *ctx = context;

    switch(notification)
    {
    case SPFILENOTIFY_STARTQUEUE:
        TRACE( "start queue\n" );
        return TRUE;
    case SPFILENOTIFY_ENDQUEUE:
        TRACE( "end queue\n" );
        return 0;
    case SPFILENOTIFY_STARTSUBQUEUE:
        TRACE( "start subqueue %Id count %Id\n", param1, param2 );
        return TRUE;
    case SPFILENOTIFY_ENDSUBQUEUE:
        TRACE( "end subqueue %Id\n", param1 );
        return 0;
    case SPFILENOTIFY_STARTDELETE:
        TRACE( "start delete %s\n", debugstr_a(paths->Target) );
        return FILEOP_DOIT;
    case SPFILENOTIFY_ENDDELETE:
        TRACE( "end delete %s\n", debugstr_a(paths->Target) );
        return 0;
    case SPFILENOTIFY_DELETEERROR:
        /*Windows Ignores attempts to delete files / folders which do not exist*/
        if ((paths->Win32Error != ERROR_FILE_NOT_FOUND) && (paths->Win32Error != ERROR_PATH_NOT_FOUND))
        SetupDeleteErrorA(ctx->owner, NULL, paths->Target, paths->Win32Error, 0);
        return FILEOP_SKIP;
    case SPFILENOTIFY_STARTRENAME:
        TRACE( "start rename %s -> %s\n", debugstr_a(paths->Source), debugstr_a(paths->Target) );
        return FILEOP_DOIT;
    case SPFILENOTIFY_ENDRENAME:
        TRACE( "end rename %s -> %s\n", debugstr_a(paths->Source), debugstr_a(paths->Target) );
        return 0;
    case SPFILENOTIFY_RENAMEERROR:
        SetupRenameErrorA(ctx->owner, NULL, paths->Source, paths->Target, paths->Win32Error, 0);
        return FILEOP_SKIP;
    case SPFILENOTIFY_STARTCOPY:
        TRACE( "start copy %s -> %s\n", debugstr_a(paths->Source), debugstr_a(paths->Target) );
        return FILEOP_DOIT;
    case SPFILENOTIFY_ENDCOPY:
        TRACE( "end copy %s -> %s\n", debugstr_a(paths->Source), debugstr_a(paths->Target) );
        return 0;
    case SPFILENOTIFY_COPYERROR:
        ERR( "copy error %d %s -> %s\n", paths->Win32Error,
             debugstr_a(paths->Source), debugstr_a(paths->Target) );
        return FILEOP_SKIP;
    case SPFILENOTIFY_NEEDMEDIA:
    {
        const SOURCE_MEDIA_A *media = (const SOURCE_MEDIA_A *)param1;
        TRACE( "need media %s %s\n", debugstr_a(media->SourcePath), debugstr_a(media->SourceFile) );
        strcpy( (char *)param2, media->SourcePath );
        return FILEOP_DOIT;
    }
    default:
        FIXME( "notification %d params %Ix,%Ix\n", notification, param1, param2 );
        break;
    }
    return 0;
}


/***********************************************************************
 *            SetupDefaultQueueCallbackW   (SETUPAPI.@)
 */
UINT WINAPI SetupDefaultQueueCallbackW( PVOID context, UINT notification,
                                        UINT_PTR param1, UINT_PTR param2 )
{
    FILEPATHS_W *paths = (FILEPATHS_W *)param1;
    struct default_callback_context *ctx = context;

    switch(notification)
    {
    case SPFILENOTIFY_STARTQUEUE:
        TRACE( "start queue\n" );
        return TRUE;
    case SPFILENOTIFY_ENDQUEUE:
        TRACE( "end queue\n" );
        return 0;
    case SPFILENOTIFY_STARTSUBQUEUE:
        TRACE( "start subqueue %Id count %Id\n", param1, param2 );
        return TRUE;
    case SPFILENOTIFY_ENDSUBQUEUE:
        TRACE( "end subqueue %Id\n", param1 );
        return 0;
    case SPFILENOTIFY_STARTDELETE:
        TRACE( "start delete %s\n", debugstr_w(paths->Target) );
        return FILEOP_DOIT;
    case SPFILENOTIFY_ENDDELETE:
        TRACE( "end delete %s\n", debugstr_w(paths->Target) );
        return 0;
    case SPFILENOTIFY_DELETEERROR:
        /*Windows Ignores attempts to delete files / folders which do not exist*/
        if ((paths->Win32Error != ERROR_FILE_NOT_FOUND) && (paths->Win32Error != ERROR_PATH_NOT_FOUND))
            SetupDeleteErrorW(ctx->owner, NULL, paths->Target, paths->Win32Error, 0);
        return FILEOP_SKIP;
    case SPFILENOTIFY_STARTRENAME:
        SetupRenameErrorW(ctx->owner, NULL, paths->Source, paths->Target, paths->Win32Error, 0);
        return FILEOP_DOIT;
    case SPFILENOTIFY_ENDRENAME:
        TRACE( "end rename %s -> %s\n", debugstr_w(paths->Source), debugstr_w(paths->Target) );
        return 0;
    case SPFILENOTIFY_RENAMEERROR:
        ERR( "rename error %d %s -> %s\n", paths->Win32Error,
             debugstr_w(paths->Source), debugstr_w(paths->Target) );
        return FILEOP_SKIP;
    case SPFILENOTIFY_STARTCOPY:
        TRACE( "start copy %s -> %s\n", debugstr_w(paths->Source), debugstr_w(paths->Target) );
        return FILEOP_DOIT;
    case SPFILENOTIFY_ENDCOPY:
        TRACE( "end copy %s -> %s\n", debugstr_w(paths->Source), debugstr_w(paths->Target) );
        return 0;
    case SPFILENOTIFY_COPYERROR:
        TRACE( "copy error %d %s -> %s\n", paths->Win32Error,
             debugstr_w(paths->Source), debugstr_w(paths->Target) );
        return FILEOP_SKIP;
    case SPFILENOTIFY_NEEDMEDIA:
    {
        const SOURCE_MEDIA_W *media = (const SOURCE_MEDIA_W *)param1;
        TRACE( "need media %s %s\n", debugstr_w(media->SourcePath), debugstr_w(media->SourceFile) );
        lstrcpyW( (WCHAR *)param2, media->SourcePath );
        return FILEOP_DOIT;
    }
    default:
        FIXME( "notification %d params %Ix,%Ix\n", notification, param1, param2 );
        break;
    }
    return 0;
}

/***********************************************************************
 *            SetupDeleteErrorA   (SETUPAPI.@)
 */

UINT WINAPI SetupDeleteErrorA( HWND parent, PCSTR dialogTitle, PCSTR file,
                               UINT w32error, DWORD style)
{
    FIXME( "stub: (Error Number %d when attempting to delete %s)\n",
           w32error, debugstr_a(file) );
    return DPROMPT_SKIPFILE;
}

/***********************************************************************
 *            SetupDeleteErrorW   (SETUPAPI.@)
 */

UINT WINAPI SetupDeleteErrorW( HWND parent, PCWSTR dialogTitle, PCWSTR file,
                               UINT w32error, DWORD style)
{
    FIXME( "stub: (Error Number %d when attempting to delete %s)\n",
           w32error, debugstr_w(file) );
    return DPROMPT_SKIPFILE;
}

/***********************************************************************
 *            SetupRenameErrorA   (SETUPAPI.@)
 */

UINT WINAPI SetupRenameErrorA( HWND parent, PCSTR dialogTitle, PCSTR source,
                               PCSTR target, UINT w32error, DWORD style)
{
    FIXME( "stub: (Error Number %d when attempting to rename %s to %s)\n",
           w32error, debugstr_a(source), debugstr_a(target));
    return DPROMPT_SKIPFILE;
}

/***********************************************************************
 *            SetupRenameErrorW   (SETUPAPI.@)
 */

UINT WINAPI SetupRenameErrorW( HWND parent, PCWSTR dialogTitle, PCWSTR source,
                               PCWSTR target, UINT w32error, DWORD style)
{
    FIXME( "stub: (Error Number %d when attempting to rename %s to %s)\n",
           w32error, debugstr_w(source), debugstr_w(target));
    return DPROMPT_SKIPFILE;
}


/***********************************************************************
 *            SetupCopyErrorA   (SETUPAPI.@)
 */

UINT WINAPI SetupCopyErrorA( HWND parent, PCSTR dialogTitle, PCSTR diskname,
                             PCSTR sourcepath, PCSTR sourcefile, PCSTR targetpath,
                             UINT w32error, DWORD style, PSTR pathbuffer,
                             DWORD buffersize, PDWORD requiredsize)
{
    FIXME( "stub: (Error Number %d when attempting to copy file %s from %s to %s)\n",
           w32error, debugstr_a(sourcefile), debugstr_a(sourcepath) ,debugstr_a(targetpath));
    return DPROMPT_SKIPFILE;
}

/***********************************************************************
 *            SetupCopyErrorW   (SETUPAPI.@)
 */

UINT WINAPI SetupCopyErrorW( HWND parent, PCWSTR dialogTitle, PCWSTR diskname,
                             PCWSTR sourcepath, PCWSTR sourcefile, PCWSTR targetpath,
                             UINT w32error, DWORD style, PWSTR pathbuffer,
                             DWORD buffersize, PDWORD requiredsize)
{
    FIXME( "stub: (Error Number %d when attempting to copy file %s from %s to %s)\n",
           w32error, debugstr_w(sourcefile), debugstr_w(sourcepath) ,debugstr_w(targetpath));
    return DPROMPT_SKIPFILE;
}

/***********************************************************************
 *            pSetupGetQueueFlags   (SETUPAPI.@)
 */
DWORD WINAPI pSetupGetQueueFlags( HSPFILEQ handle )
{
    struct file_queue *queue = handle;
    return queue->flags;
}

/***********************************************************************
 *            pSetupSetQueueFlags   (SETUPAPI.@)
 */
BOOL WINAPI pSetupSetQueueFlags( HSPFILEQ handle, DWORD flags )
{
    struct file_queue *queue = handle;
    queue->flags = flags;
    return TRUE;
}
