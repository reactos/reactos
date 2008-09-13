#include <precomp.h>
#include <tchar.h>
#include <io.h>

#ifndef _USE_FIND64
#define _USE_FIND64 1
#endif

/*
 * @implemented
 */
#if _USE_FIND64
intptr_t _tfindfirst32i64(const _TCHAR* _name, struct _tfinddata32i64_t* result)
#else
intptr_t _tfindfirst32(const _TCHAR* _name, struct _tfinddata32_t* result)
#endif
{
    WIN32_FIND_DATA FindFileData;
    HANDLE hFindFile;

    hFindFile = FindFirstFile(_name, &FindFileData);
    if (hFindFile == INVALID_HANDLE_VALUE) {
        _dosmaperr(GetLastError());
        return -1;
    }

    result->attrib = FindFileData.dwFileAttributes;
    result->time_create = FileTimeToUnixTime(&FindFileData.ftCreationTime,NULL);
    result->time_access = FileTimeToUnixTime(&FindFileData.ftLastAccessTime,NULL);
    result->time_write = FileTimeToUnixTime(&FindFileData.ftLastWriteTime,NULL);
    result->size = FindFileData.nFileSizeLow;
    _tcsncpy(result->name,FindFileData.cFileName,MAX_PATH);

    return (intptr_t)hFindFile;
}

/*
 * @implemented
 */
#if _USE_FIND64
int _tfindnext32i64(intptr_t handle, struct _tfinddata32i64_t* result)
#else
int _tfindnext32(intptr_t handle, struct _tfinddata32_t* result)
#endif
{
    WIN32_FIND_DATA FindFileData;

    if (!FindNextFile((HANDLE)handle, &FindFileData)) {
    	_dosmaperr(GetLastError());
        return -1;
	}

    result->attrib = FindFileData.dwFileAttributes;
    result->time_create = FileTimeToUnixTime(&FindFileData.ftCreationTime,NULL);
    result->time_access = FileTimeToUnixTime(&FindFileData.ftLastAccessTime,NULL);
    result->time_write = FileTimeToUnixTime(&FindFileData.ftLastWriteTime,NULL);
    result->size = FindFileData.nFileSizeLow;
    _tcsncpy(result->name,FindFileData.cFileName, MAX_PATH);

    return 0;
}


/*
 * @implemented
 */
#if _USE_FIND64
intptr_t _tfindfirst64(const _TCHAR *_name, struct _tfinddata64_t *result)
#else
intptr_t _tfindfirst64i32(const _TCHAR *_name, struct _tfinddata64i32_t *result)
#endif
{
  WIN32_FIND_DATA FindFileData;
  HANDLE hFindFile;

  hFindFile = FindFirstFile(_name, &FindFileData);
  if (hFindFile == INVALID_HANDLE_VALUE) {
      _dosmaperr(GetLastError());
      return -1;
    }

  result->attrib = FindFileData.dwFileAttributes;
  result->time_create = FileTimeToUnixTime(&FindFileData.ftCreationTime,NULL);
  result->time_access = FileTimeToUnixTime(&FindFileData.ftLastAccessTime,NULL);
  result->time_write = FileTimeToUnixTime(&FindFileData.ftLastWriteTime,NULL);
  result->size =
    (((__int64)FindFileData.nFileSizeLow)<<32) + FindFileData.nFileSizeLow;
  _tcsncpy(result->name,FindFileData.cFileName,MAX_PATH);

  return (intptr_t)hFindFile;
}

/*
 * @implemented
 */
#if _USE_FIND64
int _tfindnext64(intptr_t handle, struct _tfinddata64_t *result)
#else
int _tfindnext64i32(intptr_t handle, struct _tfinddata64i32_t *result)
#endif
{
  WIN32_FIND_DATA FindFileData;

   if (!FindNextFile((HANDLE)handle, &FindFileData)) {
      _dosmaperr(GetLastError());
      return -1;
   }

  result->attrib = FindFileData.dwFileAttributes;
  result->time_create = FileTimeToUnixTime(&FindFileData.ftCreationTime,NULL);
  result->time_access = FileTimeToUnixTime(&FindFileData.ftLastAccessTime,NULL);
  result->time_write = FileTimeToUnixTime(&FindFileData.ftLastWriteTime,NULL);
  result->size =
    (((__int64)FindFileData.nFileSizeLow)<<32) + FindFileData.nFileSizeLow;
  _tcsncpy(result->name,FindFileData.cFileName,MAX_PATH);

  return 0;
}

