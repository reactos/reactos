#include <precomp.h>
#include <tchar.h>

#ifdef _UNICODE
 #define _TS S
 #define sT "S"
#else
 #define _TS s
 #define sT "s"
#endif

#define MK_STR(s) #s

/*
 * @implemented
 */
int _taccess( const _TCHAR *_path, int _amode )
{
    DWORD Attributes = GetFileAttributes(_path);
    TRACE(MK_STR(_taccess)"('%"sT"', %x)\n", _path, _amode);

    if (!_path || Attributes == INVALID_FILE_ATTRIBUTES) {
    	_dosmaperr(GetLastError());
        return -1;
    }
    if ((_amode & W_OK) == W_OK) {
        if ((Attributes & FILE_ATTRIBUTE_READONLY) == FILE_ATTRIBUTE_READONLY) {
            __set_errno(EACCES);
            return -1;
        }
    }

    return 0;
}

/*
 * INTERNAL
 */
int access_dirT(const _TCHAR *_path)
{
    DWORD Attributes = GetFileAttributes(_path);
    TRACE(MK_STR(is_dirT)"('%"sT"')\n", _path);

    if (Attributes == (DWORD)-1) {
         _dosmaperr(GetLastError());
        return -1;
    }

    if ((Attributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
    {
      __set_errno(EACCES);
      return -1;
    }

   return 0;
}


