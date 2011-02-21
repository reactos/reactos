
/*
 * @implemented
 */
intptr_t _tfindfirst(const _TCHAR* _name, struct _tfinddata_t* result)
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
    result->size = (((__int64)FindFileData.nFileSizeHigh)<<32) + FindFileData.nFileSizeLow;
    _tcsncpy(result->name,FindFileData.cFileName,MAX_PATH);

    return (intptr_t)hFindFile;
}

/*
 * @implemented
 */
int _tfindnext(intptr_t handle, struct _tfinddata_t* result)
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
    result->size = (((__int64)FindFileData.nFileSizeHigh)<<32) + FindFileData.nFileSizeLow;
    _tcsncpy(result->name,FindFileData.cFileName, MAX_PATH);

    return 0;
}
