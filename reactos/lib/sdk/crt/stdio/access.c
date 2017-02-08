#include <precomp.h>
#include <tchar.h>

#ifdef _UNICODE
 #define _TS S
 #define sT "S"
 #define access_dirT access_dirW
#else
 #define _TS s
 #define sT "s"
 #define access_dirT access_dirA
#endif

#define MK_STR(s) #s

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
      _set_errno(EACCES);
      return -1;
    }

   return 0;
}


