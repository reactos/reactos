/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    bp.h

Abstract:

    HEADER for bp.c

Environment:

    LINUX 2.2.X
    Kernel mode only

Author: 

    Klaus P. Gerlicher

Revision History:

    15-Nov-2000:    general cleanup of source files

Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/
typedef struct _SW_BP
{
    ULONG ulAddress;
    ULONG ulNextInstr;
    UCHAR ucOriginalOpcode;
    BOOLEAN bUsed;
    BOOLEAN bInstalled;
    BOOLEAN bPermanent;
    BOOLEAN bVirtual;
    char szModName[128];
    char szFunctionName[128];
    void (*Callback)(void);
}SW_BP,*PSW_BP;

BOOLEAN InstallSWBreakpoint(ULONG ulAddress,BOOLEAN bPermanent,void (*SWBreakpointCallback)(void));
BOOLEAN InstallVirtualSWBreakpoint(LPSTR ModName,LPSTR Function);
void TryToInstallVirtualSWBreakpoints(void);
BOOLEAN DeInstallSWBreakpoint(ULONG ulAddress);
BOOLEAN RemoveSWBreakpoint(ULONG ulAddress);
BOOLEAN NeedToReInstallSWBreakpoints(ULONG ulAddress,BOOLEAN bUseAddress);
BOOLEAN ReInstallSWBreakpoint(ULONG ulAddress);
BOOLEAN RemoveAllSWBreakpoints(BOOLEAN bEvenPermanents);
PSW_BP IsPermanentSWBreakpoint(ULONG ulAddress);
void ListSWBreakpoints(void);
PSW_BP FindSwBp(ULONG ulAddress);
BOOLEAN IsSwBpAtAddress(ULONG ulAddress);
BOOLEAN IsSwBpAtAddressInstalled(ULONG ulAddress);
void RevirtualizeBreakpointsForModule(struct module* pMod);

void InstallInt3Hook(void);
void DeInstallInt3Hook(void);

