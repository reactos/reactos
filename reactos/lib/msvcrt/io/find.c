#include <msvcrti.h>


int _findclose(int handle)
{
  // check no wildcards or invalid handle
  if (handle == 0 || handle == -1)
    return 0;
  return FindClose((void *)handle);
}

int _findfirst(const char *_name, struct _finddata_t *result)
{
  WIN32_FIND_DATAA FindFileData;
  char dir[MAX_PATH];
  long hFindFile;
  int len = 0;

  if ( _name == NULL || _name[0] == 0 )
    {
      len = GetCurrentDirectoryA(MAX_PATH-4,dir);
      if (dir[len-1] != '\\')
	{
	  dir[len] = '\\';
	  dir[len+1] = 0;
	}
      strcat(dir,"*.*");
    }
  else
    strcpy(dir,_name);

  hFindFile = (long)FindFirstFileA( dir, &FindFileData );
  if (hFindFile == -1)
    {
      memset(result,0,sizeof(struct _finddata_t));
      return -1;
    }

  result->attrib = FindFileData.dwFileAttributes;
  result->time_create = FileTimeToUnixTime(&FindFileData.ftCreationTime,NULL);
  result->time_access = FileTimeToUnixTime(&FindFileData.ftLastAccessTime,NULL);
  result->time_write = FileTimeToUnixTime(&FindFileData.ftLastWriteTime,NULL);
  result->size = FindFileData.nFileSizeLow;
  strncpy(result->name,FindFileData.cFileName,MAX_PATH);

  // if no wildcard the find file handle can be closed right away
  // a return value of 0 can flag this.

  if (!strchr(dir,'*') && !strchr(dir,'?'))
    {
      _findclose(hFindFile);
      return 0;
    }

  return hFindFile;
}

long _findfirsti64(const char *_name, struct _finddatai64_t *result)
{
  WIN32_FIND_DATAA FindFileData;
  char dir[MAX_PATH];
  long hFindFile;
  int len = 0;

  if ( _name == NULL || _name[0] == 0 )
    {
      len = GetCurrentDirectoryA(MAX_PATH-4,dir);
      if (dir[len-1] != '\\')
	{
	  dir[len] = '\\';
	  dir[len+1] = 0;
	}
      strcat(dir, "*.*");
    }
  else
    strcpy(dir, _name);

  hFindFile = (long)FindFirstFileA(dir, &FindFileData);
  if (hFindFile == -1)
    {
      memset(result,0,sizeof(struct _finddatai64_t));
      return -1;
    }

  result->attrib = FindFileData.dwFileAttributes;
  result->time_create = FileTimeToUnixTime(&FindFileData.ftCreationTime,NULL);
  result->time_access = FileTimeToUnixTime(&FindFileData.ftLastAccessTime,NULL);
  result->time_write = FileTimeToUnixTime(&FindFileData.ftLastWriteTime,NULL);
  result->size =
    (((__int64)FindFileData.nFileSizeLow)<<32) + FindFileData.nFileSizeLow;
  strncpy(result->name,FindFileData.cFileName,MAX_PATH);

  // if no wildcard the find file handle can be closed right away
  // a return value of 0 can flag this.

  if (!strchr(dir,'*') && !strchr(dir,'?'))
    {
      _findclose(hFindFile);
      return 0;
    }

  return hFindFile;
}

int _findnext(int handle, struct _finddata_t *result)
{
  WIN32_FIND_DATAA FindFileData;

  // check no wildcards or invalid handle
  if (handle == 0 || handle == -1)
    return 0;

  if (!FindNextFileA((void *)handle, &FindFileData))
    return -1;

  result->attrib = FindFileData.dwFileAttributes;
  result->time_create = FileTimeToUnixTime(&FindFileData.ftCreationTime,NULL);
  result->time_access = FileTimeToUnixTime(&FindFileData.ftLastAccessTime,NULL);
  result->time_write = FileTimeToUnixTime(&FindFileData.ftLastWriteTime,NULL);
  result->size = FindFileData.nFileSizeLow;
  strncpy(result->name,FindFileData.cFileName, MAX_PATH);

  return 0;
}

int _findnexti64(long handle, struct _finddatai64_t *result)
{
  WIN32_FIND_DATAA FindFileData;

  // check no wildcards or invalid handle
  if (handle == 0 || handle == -1)
    return 0;

  if (!FindNextFileA((void *)handle, &FindFileData))
    return -1;

  result->attrib = FindFileData.dwFileAttributes;
  result->time_create = FileTimeToUnixTime(&FindFileData.ftCreationTime,NULL);
  result->time_access = FileTimeToUnixTime(&FindFileData.ftLastAccessTime,NULL);
  result->time_write = FileTimeToUnixTime(&FindFileData.ftLastWriteTime,NULL);
  result->size =
    (((__int64)FindFileData.nFileSizeLow)<<32) + FindFileData.nFileSizeLow;
  strncpy(result->name,FindFileData.cFileName,MAX_PATH);

  return 0;
}

long _wfindfirst(const wchar_t *_name, struct _wfinddata_t *result)
{
  WIN32_FIND_DATAW FindFileData;
  wchar_t dir[MAX_PATH];
  long hFindFile;
  int len = 0;

  if ( _name == NULL || _name[0] == 0 )
    {
      len = GetCurrentDirectoryW(MAX_PATH-4, dir);
      if (dir[len-1] != L'\\')
	{
	  dir[len] = L'\\';
	  dir[len+1] = 0;
	}
      wcscat(dir, L"*.*");
    }
  else
    wcscpy(dir, _name);

  hFindFile = (long)FindFirstFileW(dir, &FindFileData);
  if (hFindFile == -1)
    {
      memset(result,0,sizeof(struct _wfinddata_t));
      return -1;
    }

  result->attrib = FindFileData.dwFileAttributes;
  result->time_create = FileTimeToUnixTime(&FindFileData.ftCreationTime,NULL);
  result->time_access = FileTimeToUnixTime(&FindFileData.ftLastAccessTime,NULL);
  result->time_write = FileTimeToUnixTime(&FindFileData.ftLastWriteTime,NULL);
  result->size = FindFileData.nFileSizeLow;
  wcsncpy(result->name,FindFileData.cFileName,MAX_PATH);

  // if no wildcard the find file handle can be closed right away
  // a return value of 0 can flag this.

  if (!wcschr(dir, L'*') && !wcschr(dir, L'?'))
    {
      _findclose(hFindFile);
      return 0;
    }

  return hFindFile;
}

long _wfindfirsti64(const wchar_t *_name, struct _wfinddatai64_t *result)
{
  WIN32_FIND_DATAW FindFileData;
  wchar_t dir[MAX_PATH];
  long hFindFile;
  int len = 0;

  if (_name == NULL || _name[0] == 0)
    {
      len = GetCurrentDirectoryW(MAX_PATH-4,dir);
      if (dir[len-1] != L'\\')
	{
	  dir[len] = L'\\';
	  dir[len+1] = 0;
	}
      wcscat(dir, L"*.*");
    }
  else
    wcscpy(dir, _name);

  hFindFile = (long)FindFirstFileW(dir, &FindFileData);
  if (hFindFile == -1)
    {
      memset(result,0,sizeof(struct _wfinddatai64_t));
      return -1;
    }

  result->attrib = FindFileData.dwFileAttributes;
  result->time_create = FileTimeToUnixTime(&FindFileData.ftCreationTime,NULL);
  result->time_access = FileTimeToUnixTime(&FindFileData.ftLastAccessTime,NULL);
  result->time_write = FileTimeToUnixTime(&FindFileData.ftLastWriteTime,NULL);
  result->size =
    (((__int64)FindFileData.nFileSizeLow)<<32) + FindFileData.nFileSizeLow;
  wcsncpy(result->name,FindFileData.cFileName,MAX_PATH);

  // if no wildcard the find file handle can be closed right away
  // a return value of 0 can flag this.

  if (!wcschr(dir,L'*') && !wcschr(dir,L'?'))
    {
      _findclose(hFindFile);
      return 0;
    }

  return hFindFile;
}

int _wfindnext(long handle, struct _wfinddata_t *result)
{
  WIN32_FIND_DATAW FindFileData;

  // check no wildcards or invalid handle
  if (handle == 0 || handle == -1)
    return 0;

  if (!FindNextFileW((void *)handle, &FindFileData))
    return -1;

  result->attrib = FindFileData.dwFileAttributes;
  result->time_create = FileTimeToUnixTime(&FindFileData.ftCreationTime,NULL);
  result->time_access = FileTimeToUnixTime(&FindFileData.ftLastAccessTime,NULL);
  result->time_write = FileTimeToUnixTime(&FindFileData.ftLastWriteTime,NULL);
  result->size = FindFileData.nFileSizeLow;
  wcsncpy(result->name,FindFileData.cFileName, MAX_PATH);

  return 0;
}

int _wfindnexti64(long handle, struct _wfinddatai64_t *result)
{
  WIN32_FIND_DATAW FindFileData;

  // check no wildcards or invalid handle
  if (handle == 0 || handle == -1)
    return 0;

  if (!FindNextFileW((void *)handle, &FindFileData))
    return -1;

  result->attrib = FindFileData.dwFileAttributes;
  result->time_create = FileTimeToUnixTime(&FindFileData.ftCreationTime,NULL);
  result->time_access = FileTimeToUnixTime(&FindFileData.ftLastAccessTime,NULL);
  result->time_write = FileTimeToUnixTime(&FindFileData.ftLastWriteTime,NULL);
  result->size =
    (((__int64)FindFileData.nFileSizeLow)<<32) + FindFileData.nFileSizeLow;
  wcsncpy(result->name,FindFileData.cFileName,MAX_PATH);

  return 0;
}
