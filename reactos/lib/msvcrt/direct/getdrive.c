#include <windows.h>
#include <msvcrt/ctype.h>
#include <msvcrt/direct.h>


extern int cur_drive;

/*
 * @implemented
 */
int _getdrive(void)
{
    char Buffer[MAX_PATH];

    if (cur_drive == 0) {
        GetCurrentDirectoryA(MAX_PATH, Buffer);
        cur_drive = toupper(Buffer[0] - '@');
    }
    return cur_drive;
}

/*
 * @unimplemented
 */
unsigned long _getdrives(void)
{
    //fixme get logical drives
    //return GetLogicalDrives();
    return 5;  // drive A and C
}
