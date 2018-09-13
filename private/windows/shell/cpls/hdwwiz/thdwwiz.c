//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       thdwwiz.c
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
(*PHDWTASKSWIZARD)(
   HWND hwndParent,
   PWCHAR MachineName
   );



char
szUsage[]=
     "tHdwWiz.exe [\\MachineName]\n"
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
    PHDWTASKSWIZARD HdwTasksWizard;
    PWCHAR pwch;
    PWCHAR  MachineName = NULL;

    index = 0;
    while (++index < argc) {
        pwch = argv[index];
        if (*pwch == TEXT('\\')) {
            MachineName = argv[index];
            }
        }


    hModule = LoadLibraryW(L"HdwWiz.cpl");
    if (hModule) {
        HdwTasksWizard = (PHDWTASKSWIZARD)GetProcAddress(hModule, "HdwTasksWiz");
        if (HdwTasksWizard) {
            (*HdwTasksWizard)(NULL, MachineName);
             }
        }

    return 0;
}
