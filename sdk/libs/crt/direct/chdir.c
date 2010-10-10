#include <precomp.h>
#include <ctype.h>
#include <direct.h>
#include <tchar.h>

/*
 * @implemented
 */
int _tchdir(const _TCHAR* _path)
{
    WCHAR newdir[MAX_PATH];

    if (!SetCurrentDirectory(_path))
    {
        _dosmaperr(_path ? GetLastError() : 0);
        return -1;
    }

    /* Update the drive-specific current directory variable */
    if (GetCurrentDirectoryW(MAX_PATH, newdir) && newdir[1] == L':')
    {
        WCHAR envvar[4] = { L'=', towupper(newdir[0]), L':', L'\0' };
        SetEnvironmentVariableW(envvar, newdir);
    }

    return 0;
}
