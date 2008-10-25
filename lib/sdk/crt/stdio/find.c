#include <precomp.h>
#include <tchar.h>

/*
 * @implemented
 */
intptr_t
_tfindfirst(const _TCHAR* _name, struct _tfinddata_t* result)
{
    WIN32_FIND_DATA FindFileData;
    long hFindFile;

    hFindFile = (intptr_t)FindFirstFile(_name, &FindFileData);
    if (hFindFile == -1) {
        _dosmaperr(GetLastError());
        return -1;
    }

    result->attrib = FindFileData.dwFileAttributes;
    result->time_create = FileTimeToUnixTime(&FindFileData.ftCreationTime,NULL);
    result->time_access = FileTimeToUnixTime(&FindFileData.ftLastAccessTime,NULL);
    result->time_write = FileTimeToUnixTime(&FindFileData.ftLastWriteTime,NULL);
    result->size = FindFileData.nFileSizeLow;
    _tcsncpy(result->name,FindFileData.cFileName,MAX_PATH);

    return hFindFile;
}

/*
 * @implemented
 */
intptr_t _tfindnext(intptr_t handle, struct _tfinddata_t* result)
{
    WIN32_FIND_DATA FindFileData;

    if (!FindNextFile((void*)handle, &FindFileData)) {
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
intptr_t _tfindfirsti64(const _TCHAR *_name, struct _tfinddatai64_t *result)
{
  WIN32_FIND_DATA FindFileData;
  long hFindFile;

  hFindFile = (intptr_t)FindFirstFile(_name, &FindFileData);
  if (hFindFile == -1)
    {
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

  return hFindFile;
}

/*
 * @implemented
 */
int _tfindnexti64(intptr_t handle, struct _tfinddatai64_t *result)
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



#ifndef _UNICODE

/*
 * @implemented
 */
int _findclose(intptr_t handle)
{
  if (!FindClose((HANDLE)handle)) {
    _dosmaperr(GetLastError());
    return -1;
  }

  return 0;
}

#endif
