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
