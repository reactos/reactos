/*
 * This test program was copied from the former file documentation/cdrom-label
 */
#include <windows.h>
#include <stdio.h>
#include <string.h> /* for strcat() */

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpszCmdLine, int nCmdShow)
{
    char  drive, root[]="C:\\", label[1002], fsname[1002];
    DWORD serial, flags, filenamelen, labellen = 1000, fsnamelen = 1000;

    printf("Drive Serial     Flags      Filename-Length "
           "Label                 Fsname\n");
    for (drive = 'A'; drive <= 'Z'; drive++)
    {
        root[0] = drive;
        if (GetVolumeInformation(root,label,labellen,&serial,
                                  &filenamelen,&flags,fsname,fsnamelen))
        {
            strcat(label,"\""); strcat (fsname,"\"");
            printf("%c:\\   0x%08lx 0x%08lx %15ld \"%-20s \"%-20s\n",
                   drive, (long) serial, (long) flags, (long) filenamelen,
                   label, fsname);
        }
    }
    return 0;
}
