// main.c :
//
#include <windows.h>
#include <stdio.h>
#include "regdump.h"


HANDLE OutputHandle;
HANDLE InputHandle;


DWORD GetInput(char* Buffer, int buflen)
{
    DWORD Result;

    ReadConsoleA(InputHandle, Buffer, buflen, &Result, NULL);
    return Result;
}

int __cdecl main(int argc, char* argv[])
{
    //AllocConsole();
    InputHandle = GetStdHandle(STD_INPUT_HANDLE);
    OutputHandle =  GetStdHandle(STD_OUTPUT_HANDLE);
    //return regmain(argc, argv);
    return regdump(argc, argv);
}


#ifndef __GNUC__

//__declspec(dllimport) int __stdcall DllMain(void* hinstDll, unsigned long dwReason, void* reserved);

char* args[] = { "regdump.exe", "0", "ansi", "verbose"};

int __cdecl mainCRTStartup(void)
{

    //DllMain(NULL, DLL_PROCESS_ATTACH, NULL);

    main(1, args);
    return 0;
}

#endif /*__GNUC__*/
