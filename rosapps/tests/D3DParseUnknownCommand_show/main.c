
#include <windows.h>
#include <stdio.h>

#include <ddrawi.h>
#include <d3dhal.h>

/* Black Box program for D3DParseUnknownCommand in ddraw.dll
 *
 * This program scaning all return valu that D3DParseUnknownCommand have in ddraw.dll 
 * This command is not 100% document in MSDN so I using ddk kit doc and WinCE document 
 * and ms ddk / ms sdk to figout the basic. and MSDN was unclare what this command support for 
 * D3DHAL_DP2COMMAND dp2command->bCommand so I wrote this small scanner
 *
 * it will show D3DParseUnknownCommand  dp2command->bCommand support follow command
 *  command                 hex      dec 
 *  D3DDP2OP_VIEWPORTINFO (aka 0x1c) 28
 *  D3DDP2OP_WINFO        (aka 0x1d) 29
 *  Unknown               (aka 0x0d) 13
 *
 *  no doc in msdn about command 13 (dec) I will exaime it later 
 *
 */

INT main(INT argc, CHAR argv[]);
VOID BuildReturnCode(DWORD * ReturnCode);

INT main(INT argc, TCHAR argv[])
{
    DWORD ReturnCode[256];

    BuildReturnCode(ReturnCode);

    return 0;
}

VOID BuildReturnCode(DWORD * ReturnCode)
{
    INT t;
    D3DHAL_DP2COMMAND dp2command;
    DWORD lplpvReturnedCommand[2];
    
    for (t=0;t<256;t++)
    {
       dp2command.bCommand = t;
       ReturnCode[t] = D3DParseUnknownCommand ( (LPVOID) &dp2command, (LPVOID *) lplpvReturnedCommand) ;   
       printf("D3DParseUnknownCommand return code = %x command %d \n", ReturnCode[t], t);
    }
}
