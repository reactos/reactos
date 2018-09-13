// config.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include <eventcmd.h>
#include <eventcmt.h>

void _cdecl main(int argc, char *argv[])
{
    DWORD   dwError;

    printf("Microsoft (R) Event To Trap Translator; Configuration Tool v2.00\n");
    printf("Copyright (c) Microsoft Corporation 1998.  All rights reserved.\n");

    if (argc < 2)
        printf("\nusage: evntcmd <file.cnf>\n\n");

    EventConfigModifier eventcmt(argv[1]);
    
    dwError = eventcmt.Main();

    if (dwError == EVCMT_SUCCESS)
        printf("\nCommand succeeded.\n\n");
    else
        printf("\nCommand failed with err %u.\n\n", dwError);
}
