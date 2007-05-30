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
    _TCHAR dir[MAX_PATH];
    long hFindFile;
    int len = 0;

    if (_name == NULL || _name[0] == 0) {
        len = GetCurrentDirectory(MAX_PATH-4,dir);
        if (dir[len-1] != '\\') {
            dir[len] = '\\';
            dir[len+1] = 0;
        }
        _tcscat(dir,_T("*.*"));
    } else {
        _tcscpy(dir,_name);
    }

    hFindFile = (long)FindFirstFile(dir, &FindFileData);
    if (hFindFile == -1) {
        memset(result,0,sizeof(struct _tfinddata_t));
        _dosmaperr(GetLastError());
        return -1;
    }

    result->attrib = FindFileData.dwFileAttributes;
    result->time_create = FileTimeToUnixTime(&FindFileData.ftCreationTime,NULL);
    result->time_access = FileTimeToUnixTime(&FindFileData.ftLastAccessTime,NULL);
    result->time_write = FileTimeToUnixTime(&FindFileData.ftLastWriteTime,NULL);
    result->size = FindFileData.nFileSizeLow;
    _tcsncpy(result->name,FindFileData.cFileName,MAX_PATH);

    // if no wildcard the find file handle can be closed right away
    // a return value of 0 can flag this.

    if (!_tcschr(dir,'*') && !_tcschr(dir,'?')) {
        _findclose(hFindFile);
        return 0;
    }

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

    // check no wildcards or invalid handle
    if (handle == 0 || handle == -1)
        return 0;

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
  _TCHAR dir[MAX_PATH];
  long hFindFile;
  int len = 0;

  if ( _name == NULL || _name[0] == 0 )
    {
      len = GetCurrentDirectory(MAX_PATH-4,dir);
      if (dir[len-1] != '\\')
    {
      dir[len] = '\\';
      dir[len+1] = 0;
    }
      _tcscat(dir, _T("*.*"));
    }
  else
    _tcscpy(dir, _name);

  hFindFile = (long)FindFirstFile(dir, &FindFileData);
  if (hFindFile == -1)
    {
      memset(result,0,sizeof(struct _tfinddatai64_t));
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

  // if no wildcard the find file handle can be closed right away
  // a return value of 0 can flag this.

  if (!_tcschr(dir,'*') && !_tcschr(dir,'?')) {
      _findclose(hFindFile);
      return 0;
    }
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

  // check no wildcards or invalid handle
  if (handle == 0 || handle == -1)
    return 0;

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
    // check no wildcards or invalid handle
    if (handle == 0 || handle == -1)
       return 0;
    return FindClose((void*)handle);
}

#endif
