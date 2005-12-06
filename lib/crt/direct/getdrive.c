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
    if (GetCurrentDirectoryW( MAX_PATH, buffer ) &&
        buffer[0] >= 'A' && buffer[0] <= 'z' && buffer[1] == ':')
        return towupper(buffer[0]) - 'A' + 1;
    return 0;
}

/*
 * @implemented
 */
unsigned long _getdrives(void)
{
   return GetLogicalDrives();
}
