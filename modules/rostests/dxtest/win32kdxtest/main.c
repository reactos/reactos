

/* All testcase are base how windows 2000 sp4 acting */


#include <stdio.h>
/* SDK/DDK/NDK Headers. */
#include <windows.h>
#include <wingdi.h>
#include <winddi.h>
#include <d3dnthal.h>
#include <dll/directx/d3d8thk.h>
#include "test.h"

BOOL dumping_on =FALSE;
FILE *fs_file;

/* we using d3d8thk.dll it is doing the real syscall in windows 2000
 * in ReactOS and Windows XP and higher d3d8thk.dll it linking to
 * gdi32.dll instead doing syscall, gdi32.dll export DdEntry1-56
 * and doing the syscall direcly. I did forget about it, This
 * test program are now working on any Windows and ReactOS
 * that got d3d8thk.dll
 */

int main(int argc, char **argv)
{
    HANDLE hDirectDrawLocal;

    if (argc == 2)
    {
        if (stricmp(argv[1],"-dump")==0)
        {
            dumping_on = TRUE;
        }

        if ( (stricmp(argv[1],"-help")==0) ||
             (stricmp(argv[1],"-?")==0) ||
             (stricmp(argv[1],"/help")==0) ||
             (stricmp(argv[1],"/?")==0) )
        {
            printf("the %s support follow param \n",argv[0]);
            printf("-dump              : It dump all data it resvie to screen \n");
            printf("-dumpfile filename : It dump all data it resvie to file \n");
            printf("\nrember u can only use one of them at time \n");
            exit(1);
        }
    }

    if (argc == 3)
    {
        if (stricmp(argv[1],"-dumpfile")==0)
        {
            /* create or over write a file in binary mode, and redirect printf to the file */
            if ( (fs_file = freopen(argv[2], "wb", stdout)) != NULL)
            {
                dumping_on = TRUE;
            }
        }
    }

    hDirectDrawLocal = test_NtGdiDdCreateDirectDrawObject();

    test_NtGdiDdQueryDirectDrawObject(hDirectDrawLocal);

    test_NtGdiDdGetScanLine(hDirectDrawLocal);

    test_NtGdiDdWaitForVerticalBlank(hDirectDrawLocal);

    test_NtGdiDdCanCreateSurface(hDirectDrawLocal);

    test_NtGdiDdDeleteDirectDrawObject(hDirectDrawLocal);

    if (fs_file != NULL)
    {
        fclose(fs_file);
    }
    return 0;
}












