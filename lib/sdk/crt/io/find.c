#include <precomp.h>
#include <tchar.h>

/*
 * @implemented
 */
#if defined(_UNICODE) || !(__MINGW32_MAJOR_VERSION < 3 || __MINGW32_MINOR_VERSION < 3)
long
#else
int
#endif
_tfindfirst(const _TCHAR* _name, struct _tfinddata_t* result)
{
    WIN32_FIND_DATA FindFileData;
    long hFindFile;

    hFindFile = (long)FindFirstFile(_name, &FindFileData);
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
int _tfindnext(
#if defined(_UNICODE) || !(__MINGW32_MAJOR_VERSION < 3 || __MINGW32_MINOR_VERSION < 3)
   long handle,
#else
   int handle,
#endif
   struct _tfinddata_t* result)
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
long _tfindfirsti64(const _TCHAR *_name, struct _tfinddatai64_t *result)
{
  WIN32_FIND_DATA FindFileData;
  long hFindFile;

  hFindFile = (long)FindFirstFile(_name, &FindFileData);
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

//_CRTIMP long __cdecl _findfirsti64(const char*, struct _finddatai64_t*);
//_CRTIMP int __cdecl _findnexti64(long, struct _finddatai64_t*);


/*
 * @implemented
 */
int _tfindnexti64(long handle, struct _tfinddatai64_t *result)
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
int _findclose(
#if __MINGW32_MAJOR_VERSION < 3 || __MINGW32_MINOR_VERSION < 3
   int handle
#else
   long handle
#endif
   )
{
  if (!FindClose((void*)handle)) {
    _dosmaperr(GetLastError());
    return -1;
  }

  return 0;
}

#endif
