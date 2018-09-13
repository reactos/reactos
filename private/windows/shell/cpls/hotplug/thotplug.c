//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       thotplug.c
//
//--------------------------------------------------------------------------

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
(*PHOTPLUGDEVICETREE)(
   HWND hwndParent,
   PTCHAR MachineName,
   BOOLEAN HotPlugTree
   );



char
szUsage[]=
     "tHotPlug.exe [\\\\MachineName] [T|F (HotPlugTree where default is TRUE)]\n"
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
    PHOTPLUGDEVICETREE HotPlugDeviceTree;
    PWCHAR  MachineName = NULL;
    BOOLEAN HotPlugTree = TRUE;
    PWCHAR pwch;


    index = 0;
    while (++index < argc) {
        pwch = argv[index];
        if (*pwch == TEXT('\\')) {
            MachineName = argv[index];
            }
        else if (*pwch == TEXT('T') || *pwch == TEXT('t')) {
            HotPlugTree = TRUE;
            }
        else if (*pwch == TEXT('F') || *pwch == TEXT('f')) {
            HotPlugTree = FALSE;
            }
        }

    hModule = LoadLibraryW(L"HotPlug.dll");
    if (hModule) {
        HotPlugDeviceTree = (PHOTPLUGDEVICETREE)GetProcAddress(hModule, "HotPlugDeviceTree");
        if (HotPlugDeviceTree) {
            (*HotPlugDeviceTree)(NULL, MachineName, HotPlugTree);
             }
        }

    return 0;
}
