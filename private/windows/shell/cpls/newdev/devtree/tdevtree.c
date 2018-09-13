#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cpl.h>
#include <cplp.h>


typedef
BOOL
(*PDISPLAYDEVICETREE)(
   HWND hwndParent,
   PTCHAR MachineName
   );



char
szUsage[]=
     "tdevtree.exe\n"
     ;


/* main
 *
 * standard win32 base windows entry point
 * returns 0 for clean exit, otherwise nonzero for error
 *
 *
 * ExitCode:
 *  0       - Clean exit with no Errors
 *  nonzero - error ocurred
 *
 */
int __cdecl wmain(int argc, WCHAR **argv)
{
    int   index;
    HMODULE hModule;
    PWCHAR  MachineName;
    PDISPLAYDEVICETREE DisplayDeviceTree;


    index = 0;
    if (++index < argc) {
        MachineName = argv[index];
        }
    else {
        MachineName = NULL;
        }


    hModule = LoadLibraryW(L"devtree.dll");
    if (hModule) {
        DisplayDeviceTree = (PDISPLAYDEVICETREE)GetProcAddress(hModule, "DisplayDeviceTree");
        if (DisplayDeviceTree) {
            (*DisplayDeviceTree)(NULL, MachineName);
             }
        }

    return 0;
}
