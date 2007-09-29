/* Written by Krzysztof Kowalczyk (http://blog.kowalczyk.info)
   The author disclaims copyright to this source code. */
#ifndef FILE_UTILS_H_
#define FILE_UTILS_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct DirIterState {
    char *          fileName;
    char *          cleanPath;
    char *          iterPath;
    WIN32_FIND_DATAA fileInfo;
    HANDLE          dir;
} DirIterState;

typedef struct FileInfo {
    struct FileInfo *next;
    char *      path; /* full path of the file e.g. c:\foo\bar.txt */
    char *      name; /* just the name part e.g. bar.txt, points into 'path' */
    int64       size;
    FILETIME    modificationTime;
    FILETIME    accessTime;
    FILETIME    createTime;
    DWORD       attr;
    /* TODO: file attributes like file type etc. */
} FileInfo;

typedef struct FileList {
    FileInfo *  first;
    char *      dirName; /* directory where files lives e.g. c:\windows\ */
    int         filesCount;
} FileList;

DirIterState *  DirIter_New(const char *path);
BOOL            DirIter_Next(DirIterState *s);
void            DirIter_Delete(DirIterState *state);

BOOL            FileInfo_IsFile(FileInfo *fi);
BOOL            FileInfo_IsDir(FileInfo *fi);
void            FileInfo_Delete(FileInfo *fi);
FileList *      FileList_Get(char* path, int (*filter)(FileInfo *));
FileList *      FileList_GetRecursive(char* path, int (*filter)(FileInfo *));
void            FileList_Delete(FileList *fl);
int             FileList_Len(FileList *fl);
FileInfo *      FileList_GetFileInfo(FileList *fl, int file_no);

const char *    FilePath_GetBaseName(const char *path);
char *          FilePath_GetDir(const char *path);

char *          file_read_all(const char *file_path, size_t *file_size_out);
size_t          file_size_get(const char *file_path);
BOOL            write_to_file(const TCHAR *file_path, void *data, size_t data_len);

#ifdef __cplusplus
}
#endif

#endif
