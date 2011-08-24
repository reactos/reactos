#include <precomp.h>
#include <ctype.h>
#include <direct.h>


/*
 * @implemented
 *
 *    _getdrive (MSVCRT.@)
 *
 * Get the current drive number.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  Success: The drive letter number from 1 to 26 ("A:" to "Z:").
 *  Failure: 0.
 */
int _getdrive(void)
{
    WCHAR buffer[MAX_PATH];
    if (GetCurrentDirectoryW( MAX_PATH, buffer )>=2)
    {
        buffer[0]=towupper(buffer[0]);
        if (buffer[0] >= L'A' && buffer[0] <= L'Z' && buffer[1] == L':')
            return buffer[0] - L'A' + 1;
    }
    return 0;
}

/*
 * @implemented
 */
unsigned long _getdrives(void)
{
   return GetLogicalDrives();
}
