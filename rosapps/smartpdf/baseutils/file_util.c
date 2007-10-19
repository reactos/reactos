/* Written by Krzysztof Kowalczyk (http://blog.kowalczyk.info)
   The author disclaims copyright to this source code. */
#include "base_util.h"
#include "file_util.h"

#include "str_util.h"
#include "strlist_util.h"

char *FilePath_ConcatA(const char *path, const char *name)
{
    assert(path && name);
    if (!path || !name) return NULL;

    if (str_endswith(path, DIR_SEP_STR))
        return str_cat(path, name);
    else
        return str_cat3(path, DIR_SEP_STR, name);
}

const char *FilePath_GetBaseName(const char *path)
{
    const char *fileBaseName = (const char*)strrchr(path, DIR_SEP_CHAR);
    if (NULL == fileBaseName)
        fileBaseName = path;
    else
        ++fileBaseName;
    return fileBaseName;
}

char *FilePath_GetDir(const char *path)
{
    char *lastSep;
    char *dir = str_dup(path);
    if (!dir) return NULL;
    lastSep = (char*)strrchr(dir, DIR_SEP_CHAR);
    if (NULL != lastSep)
        *lastSep = 0;
    return dir;
}

/* TODO: handle TCHAR (UNICODE) properly by converting to path to unicode
   if UNICODE or _UNICODE symbols are defined */
/* Start iteration of all files 'path'. 'path' should be a directory
   name but doesn't have to end with "\" (we append it if it's missing)
   Retuns NULL if there was an error.
   When no longer needed, the result should be deleted with
  'DirIter_Delete'.
*/
DirIterState *DirIter_New(const char *path)
{
    DirIterState *state;

    assert(path);

    if (!path)
        return NULL;

    state = SA(DirIterState);
    if (!state)
        return NULL;

    /* TODO: made state->cleanPath cannonical */
    state->cleanPath = str_dup(path);
    state->iterPath = FilePath_ConcatA(path, "*");
    if (!state->cleanPath || !state->iterPath) {
        DirIter_Delete(state);
        return NULL;
    }
    state->dir = INVALID_HANDLE_VALUE;
    return state;
}

/* Get information about next file in a directory.
   Returns FALSE on end of iteration (or error). */
BOOL DirIter_Next(DirIterState *s)
{
    BOOL    found;

    if (INVALID_HANDLE_VALUE == s->dir) {
        s->dir = FindFirstFileA(s->iterPath, &(s->fileInfo));
        if (INVALID_HANDLE_VALUE == s->dir)
            return FALSE;
        goto CheckFile;
    }

    for(;;) {
        found = FindNextFileA(s->dir, &(s->fileInfo));
        if (!found)
            return FALSE;
        else
CheckFile:
            return TRUE;
    }
}

/* Free memory associated with 'state' */
void DirIter_Delete(DirIterState *state)
{
    if (state) {
        free(state->cleanPath);
        free(state->iterPath);
    }
    free(state);
}

static FileList *FileList_New(char *dir)
{
    FileList *fl;

    assert(dir);

    fl = SAZ(FileList);
    if (!fl)
        return NULL;
    fl->dirName = str_dup(dir);
    if (!fl->dirName) {
        free((void*)fl);
        return NULL;
    }
    return fl;
}

static BOOL FileList_InsertFileInfo(FileList *fl, FileInfo *fi)
{
    int real_count;
    FileInfo *last_fi;

    assert(fl);
    if (!fl)
        return FALSE;
    assert(fi);
    if (!fi)
        return FALSE;
    /* TODO: use scheme where we also track of the last node, so that
       insert is O(1) and not O(n) */
    assert(!fi->next);
    fi->next = NULL;
    if (!fl->first) {
        assert(0 == fl->filesCount);
        fl->first = fi;
        fl->filesCount = 1;
        return TRUE;
    }

    last_fi = fl->first;
    assert(last_fi);
    real_count = 1;
    while (last_fi->next) {
        ++real_count;
        last_fi = last_fi->next;
    }

    assert(real_count == fl->filesCount);
    last_fi->next = fi;
    ++fl->filesCount;
    return TRUE;
}

void FileInfo_Delete(FileInfo *fi)
{
    if (!fi) return;
    free(fi->name);
    free(fi->path);
    free(fi);
}

static FileInfo *FileInfo_New(char *path, char *name, int64 size, DWORD attr, FILETIME *modificationTime)
{
    FileInfo *fi;

    assert(name);
    if (!name)
        return NULL;

    assert(modificationTime);
    if (!modificationTime)
        return NULL;

    fi = SAZ(FileInfo);
    if (!fi)
        return NULL;

    fi->name = str_dup(name);
    fi->path = str_dup(path);
    if (!fi->name || !fi->path) {
        FileInfo_Delete(fi);
        return NULL;
    }

    fi->size = size;
    fi->attr = attr;
    fi->modificationTime = *modificationTime;
    return fi;
}

static FileInfo* FileInfo_FromDirIterState(DirIterState *state)
{
    FileInfo *       fi;
    WIN32_FIND_DATAA *fd;
    char *          fileName;
    int64            size;
    char *           filePath;

    assert(state);
    if (!state) return NULL;

    fd = &state->fileInfo;
    size = fd->nFileSizeHigh;
    size = size >> 32;
    size += fd->nFileSizeLow;
    /* TODO: handle UNICODE */
    fileName = fd->cFileName;
    filePath = FilePath_ConcatA(state->cleanPath, fileName);
    fi = FileInfo_New(filePath, fileName, size, fd->dwFileAttributes, &fd->ftLastWriteTime);
    return fi;
}

BOOL FileInfo_IsFile(FileInfo *fi)
{
    DWORD attr;
    assert(fi);
    if (!fi) return
FALSE;
    attr = fi->attr;

    if (!(attr & FILE_ATTRIBUTE_DIRECTORY))
        return TRUE;
    return FALSE;
}

int FileInfo_IsDir(FileInfo *fi)
{
    DWORD attr;
    assert(fi);
    if (!fi) return
FALSE;
    attr = fi->attr;

    if (attr & FILE_ATTRIBUTE_DIRECTORY)
        return TRUE;
    return FALSE;
}

static int FileList_Append(char *path, FileList *fl, int (*filter)(FileInfo *))
{
    FileInfo *      fi;
    DirIterState *  state;
    int             shouldInsert;

    if (!path || !fl)
        return 0;

    state = DirIter_New(path);
    if (!state) {
        return 0;
    }

    /* TODO: handle errors from DirIter_Next */
    while (DirIter_Next(state)) {
        fi = FileInfo_FromDirIterState(state);
        if (!fi) {
            DirIter_Delete(state);
            return 0;
        }
        if (fi) {
            shouldInsert = 1;
            if (filter && !(*filter)(fi))
                shouldInsert = 0;
            if (shouldInsert)
                FileList_InsertFileInfo(fl, fi);
        }
    }
    DirIter_Delete(state);
    return 1;
}

/* Return a list of files/directories in a 'path'. Use filter function
   to filter out files that should not get included (return 0 from the function
   to exclude it from the list.
   Returns NULL in case of an error.
   Use FileList_Delete() to free up all memory associated with this data.
   Doesn't recurse into subdirectores, use FileList_GetRecursive for that. */
/* TODO: 'filter' needs to be implemented. */
/* TODO: add 'filterRegexp' argument that would allow filtering via regular
   expression */
FileList *FileList_Get(char* path, int (*filter)(FileInfo *))
{
    FileList *      fl;
    int             ok;

    if (!path)
        return NULL;

    /* TODO: should I expand "." ? */
    fl = FileList_New(path);
    if (!fl)
        return NULL;

    ok = FileList_Append(path, fl, filter);
    if (!ok) {
        FileList_Delete(fl);
        return NULL;
    }
    return fl;
}

/* Like FileList_Get() except recurses into sub-directories */
/* TODO: 'filter' needs to be implemented. */
/* TODO: add 'filterRegexp' argument that would allow filtering via regular
   expression */
FileList *FileList_GetRecursive(char* path, int (*filter)(FileInfo *))
{
    StrList *toVisit = NULL;
    FileList *fl = NULL;
    assert(0);
    /* TODO: clearly, implement */
    return NULL;
}

void FileList_Delete(FileList *fl)
{
    FileInfo *fi;
    FileInfo *fi_next;
    if (!fl)
        return;
    fi = fl->first;
    while (fi) {
        fi_next = fi->next;
        FileInfo_Delete(fi);
        fi = fi_next;
    }
    free((void*)fl->dirName);
    free((void*)fl);
}

int FileList_Len(FileList *fl)
{
    return fl->filesCount;
}

FileInfo *FileList_GetFileInfo(FileList *fl, int file_no)
{
    FileInfo *fi;
    if (!fl)
        return NULL;
    if (file_no >= fl->filesCount)
        return NULL;
    fi = fl->first;
    while (file_no > 0) {
        assert(fi->next);
        if (!fi->next)
            return NULL;
        fi = fi->next;
        --file_no;
    }
    return fi;
}

#ifdef _WIN32
size_t file_size_get(const char *file_path)
{
    int                         fOk;
    WIN32_FILE_ATTRIBUTE_DATA   fileInfo;

    if (NULL == file_path)
        return INVALID_FILE_SIZE;

    fOk = GetFileAttributesExA(file_path, GetFileExInfoStandard, (void*)&fileInfo);
    if (!fOk)
        return INVALID_FILE_SIZE;
    return (size_t)fileInfo.nFileSizeLow;
}
#else
#include <sys/types.h>
#include <sys/stat.h>

size_t file_size_get(const char *file_path)
{
    struct stat stat_buf;
    int         res;
    unsigned long size;
    if (NULL == file_path)
        return INVALID_FILE_SIZE;
    res = stat(file_path, &stat_buf);
    if (0 != res)
        return INVALID_FILE_SIZE;
    size = (size_t)stat_buf.st_size;
    return size;
}
#endif

#ifdef _WIN32
char *file_read_all(const char *file_path, size_t *file_size_out)
{
    DWORD       size, size_read;
    HANDLE      h;
    char *      data = NULL;
    int         f_ok;

    h = CreateFileA(file_path, GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,  NULL);
    if (h == INVALID_HANDLE_VALUE)
        return NULL;

    size = GetFileSize(h, NULL);
    if (-1 == size)
        goto Exit;

    /* allocate one byte more and 0-terminate just in case it's a text
       file we'll want to treat as C string. Doesn't hurt for binary
       files */
    data = (char*)malloc(size + 1);
    if (!data)
        goto Exit;
    data[size] = 0;

    f_ok = ReadFile(h, data, size, &size_read, NULL);
    if (!f_ok) {
        free(data);
        data = NULL;
    }
    *file_size_out = (size_t)size;
Exit:
    CloseHandle(h);
    return data;
}
#else
/* TODO: change unsinged long to int64 or size_t */
char *file_read_all(const char *file_path, size_t *file_size_out)
{
    FILE *fp = NULL;
    char *data = NULL;
    size_t read;

    size_t file_size = file_size_get(file_path);
    if (INVALID_FILE_SIZE == file_size)
        return NULL;

    data = (char*)malloc(file_size + 1);
    if (!data)
        goto Exit;
    data[file_size] = 0;

    fp = fopen(file_path, "rb");
    if (!fp)
        goto Error;

    read = fread((void*)data, 1, file_size, fp);
    if (ferror(fp))
        goto Error;
    assert(read == file_size);
    if (read != file_size)
        goto Error;
    fclose(fp);
    return data;
Error:
    if (fp)
        fclose(fp);
    free((void*)data);
    return NULL;
}
#endif

#ifdef _WIN32
BOOL write_to_file(const TCHAR *file_path, void *data, size_t data_len)
{
    DWORD       size;
    HANDLE      h;
    BOOL        f_ok;

    h = CreateFile(file_path, GENERIC_WRITE, FILE_SHARE_READ, NULL,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,  NULL);
    if (h == INVALID_HANDLE_VALUE)
        return FALSE;

    f_ok = WriteFile(h, data, (DWORD)data_len, &size, NULL);
    assert(!f_ok || ((DWORD)data_len == size));
    CloseHandle(h);
    return f_ok;
}
#else
// not supported
#endif
