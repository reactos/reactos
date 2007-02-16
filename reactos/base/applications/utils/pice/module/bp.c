/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    bp.c

Abstract:

    setting, listing and removing breakpoints

Environment:

    LINUX 2.2.X
    Kernel mode only

Author:

    Klaus P. Gerlicher

Revision History:

    13-Nov-1999:	created
    15-Nov-2000:    general cleanup of source files

Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/

////////////////////////////////////////////////////
// INCLUDES
////
#include "remods.h"
#include "precomp.h"

////////////////////////////////////////////////////
// GLOBALS
////
char tempBp[1024];

ULONG OldInt3Handler=0;

SW_BP aSwBreakpoints[64]={{0,0,0,0},};

//*************************************************************************
// FindSwBp()
//
//*************************************************************************
PSW_BP FindSwBp(ULONG ulAddress)
{
    ULONG i;

    for(i=0;i<DIM(aSwBreakpoints);i++)
    {
        if(aSwBreakpoints[i].ulAddress == ulAddress && aSwBreakpoints[i].bUsed==TRUE && aSwBreakpoints[i].bVirtual==FALSE)
            return &aSwBreakpoints[i];
    }

    return NULL;
}

//*************************************************************************
// FindEmptySwBpSlot()
//
//*************************************************************************
PSW_BP FindEmptySwBpSlot(void)
{
    ULONG i;

    for(i=0;i<(sizeof(aSwBreakpoints)/sizeof(SW_BP));i++)
    {
        if(aSwBreakpoints[i].bUsed == FALSE)
        {
            return &aSwBreakpoints[i];
        }
    }

    return NULL;
}

//*************************************************************************
// FindVirtualSwBp()
//
//*************************************************************************
PSW_BP FindVirtualSwBp(LPSTR ModName,LPSTR szFunctionName)
{
    ULONG i;
    PSW_BP p;

    for(i=0;i<(sizeof(aSwBreakpoints)/sizeof(SW_BP));i++)
    {
        p = &aSwBreakpoints[i];

        if(p->bUsed == TRUE &&
           p->bVirtual == TRUE &&
           PICE_strcmpi(p->szModName,ModName)==0 &&
           PICE_strcmpi(p->szFunctionName,szFunctionName)==0)
        {
            return p;
        }
    }

    return NULL;
}

//*************************************************************************
// IsSwBpAtAddressInstalled()
//
//*************************************************************************
BOOLEAN IsSwBpAtAddressInstalled(ULONG ulAddress)
{
    ULONG i;

    for(i=0;i<DIM(aSwBreakpoints);i++)
    {
        if(aSwBreakpoints[i].ulAddress == ulAddress &&
		   aSwBreakpoints[i].bUsed == TRUE &&
		   aSwBreakpoints[i].bInstalled &&
           aSwBreakpoints[i].bVirtual == FALSE)
            return TRUE;
    }

	return FALSE;
}

//*************************************************************************
// IsSwBpAtAddress()
//
//*************************************************************************
BOOLEAN IsSwBpAtAddress(ULONG ulAddress)
{
    ULONG i;

    for(i=0;i<DIM(aSwBreakpoints);i++)
    {
        if(aSwBreakpoints[i].ulAddress == ulAddress && aSwBreakpoints[i].bUsed==TRUE && aSwBreakpoints[i].bVirtual==FALSE)
            return TRUE;
    }

	return FALSE;
}

//*************************************************************************
// NeedToReInstallSWBreakpoints()
//
//*************************************************************************
BOOLEAN NeedToReInstallSWBreakpoints(ULONG ulAddress,BOOLEAN bUseAddress)
{
    PSW_BP p;
    BOOLEAN bResult = FALSE;
    ULONG i;

    ENTER_FUNC();
    DPRINT((0,"NeedToReInstallSWBreakpoint() for %x (bUseAddress = %s)\n",ulAddress,bUseAddress?"TRUE":"FALSE"));

    for(i=0;i<(sizeof(aSwBreakpoints)/sizeof(SW_BP));i++)
    {
        p = &aSwBreakpoints[i];
        if(bUseAddress)
        {
            if(p->bUsed == TRUE && p->bInstalled == FALSE && p->ulAddress==ulAddress && p->bVirtual==FALSE)
            {
                if(IsAddressValid(p->ulAddress))
                {
                    DPRINT((0,"NeedToReInstallSWBreakpoint(): [1] found BP\n"));
                    bResult = TRUE;
                    break;
                }
            }
        }
        else
        {
            if(p->bUsed == TRUE && p->bInstalled == FALSE && p->bVirtual == FALSE)
            {
                if(IsAddressValid(p->ulAddress))
                {
                    DPRINT((0,"NeedToReInstallSWBreakpoint(): [2] found BP\n"));
                    bResult = TRUE;
                    break;
                }
            }
        }
    }

    LEAVE_FUNC();

    return bResult;
}

//*************************************************************************
// ReInstallSWBreakpoint()
//
//*************************************************************************
BOOLEAN ReInstallSWBreakpoint(ULONG ulAddress)
{
    PSW_BP p;
    BOOLEAN bResult = FALSE;
    ULONG i;

    ENTER_FUNC();
    DPRINT((0,"ReInstallSWBreakpoint()\n"));

    for(i=0;i<(sizeof(aSwBreakpoints)/sizeof(SW_BP));i++)
    {
        p = &aSwBreakpoints[i];
        if(p->bUsed == TRUE && p->bInstalled == FALSE && p->ulAddress == ulAddress && p->bVirtual == FALSE)
        {
            if(IsAddressValid(p->ulAddress))
            {
				BOOLEAN isWriteable;

				if( !( isWriteable = IsAddressWriteable(p->ulAddress) ) )
					SetAddressWriteable(p->ulAddress,TRUE);
				*(PUCHAR)(p->ulAddress) = 0xCC;
				if( !isWriteable )
					SetAddressWriteable(p->ulAddress,FALSE);
				p->bInstalled = TRUE;
	            bResult = TRUE;
            }
        }
    }

    LEAVE_FUNC();

    return bResult;
}


//*************************************************************************
// InstallSWBreakpoint()
//
//*************************************************************************
BOOLEAN InstallSWBreakpoint(ULONG ulAddress,BOOLEAN bPermanent,void (*SWBreakpointCallback)(void))
{
    PSW_BP p;
    BOOLEAN bResult = FALSE;

    ENTER_FUNC();
    DPRINT((0,"InstallSWBreakpoint()\n"));

    // check if page is present
    // TODO: must also check if it's a writable page
    if(IsAddressValid(ulAddress) )
    {
        DPRINT((0,"InstallSWBreakpoint(): %.8X is valid, writable? %d\n",ulAddress,IsAddressWriteable(ulAddress)));
		DPRINT((0,"pde: %x, pte: %x\n", *(ADDR_TO_PDE(ulAddress)), *(ADDR_TO_PTE(ulAddress))));
        if((p = FindSwBp(ulAddress))==NULL)
        {
            DPRINT((0,"InstallSWBreakpoint(): %.8X is free\n",ulAddress));
            if( (p=FindEmptySwBpSlot()) )
            {
				BOOLEAN isWriteable;
                DPRINT((0,"InstallSWBreakpoint(): found empty slot\n"));
				DPRINT((0,"InstallSWBreakpoint(): %x value: %x", ulAddress, *(PUCHAR)ulAddress));
                p->ucOriginalOpcode = *(PUCHAR)ulAddress;
				//allow writing to page
				if( !( isWriteable = IsAddressWriteable(ulAddress) ) )
					SetAddressWriteable(ulAddress,TRUE);
			    DPRINT((0,"writing breakpoint\n"));
				*(PUCHAR)ulAddress = 0xCC;
				DPRINT((0,"restoring page access\n"));
				if( !isWriteable )
					SetAddressWriteable(ulAddress,FALSE);
				p->bUsed = TRUE;
                p->bInstalled = TRUE;
                // find next address
                p->ulAddress = ulAddress;
                Disasm(&ulAddress,(PUCHAR)&tempBp);
                p->ulNextInstr = ulAddress;
                p->bPermanent = bPermanent;
                if(bPermanent)
                    p->Callback = SWBreakpointCallback;
				else
					p->Callback = NULL;
                bResult = TRUE;
            }
        }
        else
        {
            DPRINT((0,"InstallSWBreakpoint(): %.8X is already used\n",ulAddress));
            if(p->bPermanent)
            {
                DPRINT((0,"InstallSWBreakpoint(): %.8X is a permanent breakpoint\n",ulAddress));
            }
        }
    }

    LEAVE_FUNC();

    return bResult;
}

//*************************************************************************
// InstallVirtualSWBreakpoint()
//
//*************************************************************************
BOOLEAN InstallVirtualSWBreakpoint(LPSTR ModName,LPSTR FunctionName)
{
    PSW_BP p;
    BOOLEAN bResult = FALSE;

    ENTER_FUNC();
    DPRINT((0,"InstallVirtualSWBreakpoint(%s!%s)\n",ModName,FunctionName));

    if( (p=FindEmptySwBpSlot()) )
    {
        DPRINT((0,"InstallVirtualSWBreakpoint(): found empty slot\n"));

        p->bUsed = TRUE;
        p->bInstalled = TRUE;
        p->bVirtual = TRUE;
		p->Callback = NULL;
        PICE_strcpy(p->szModName,ModName);
        PICE_strcpy(p->szFunctionName,FunctionName);

        bResult = TRUE;
    }

    LEAVE_FUNC();

    return bResult;
}

//*************************************************************************
// TryToInstallVirtualSWBreakpoints()
//
//*************************************************************************
void TryToInstallVirtualSWBreakpoints(void)
{
    ULONG i,ulAddress;
    PDEBUG_MODULE pMod;
    PSW_BP p;

    DPRINT((0,"TryToInstallVirtualSWBreakpoints()\n"));

    for(i=0;i<(sizeof(aSwBreakpoints)/sizeof(SW_BP));i++)
    {
		p = &aSwBreakpoints[i];
        if(p->bUsed == TRUE && p->bVirtual)
        {
            if((pMod = IsModuleLoaded(p->szModName)))
            {
                if((ulAddress = FindFunctionInModuleByName(p->szFunctionName,pMod)))
                {
                    if((p = FindVirtualSwBp(p->szModName,p->szFunctionName)))
                    {
						ULONG ulAddressWithOffset = ulAddress+p->ulAddress;
						DPRINT((0,"TryToInstallVirtualSWBreakpoints(): ulAddressWithOffset = %x (offset = %x)\n",ulAddressWithOffset,p->ulAddress));

                        if(IsAddressValid(ulAddressWithOffset))
                        {
							BOOLEAN isWriteable;
							DPRINT((0,"TryToInstallVirtualSWBreakpoints(): installing...\n"));
                            p->ucOriginalOpcode = *(PUCHAR)ulAddressWithOffset;
							//allow writing to page
							if( !( isWriteable = IsAddressWriteable(ulAddressWithOffset) ) )
								SetAddressWriteable(ulAddressWithOffset,TRUE);
                            *(PUCHAR)ulAddressWithOffset = 0xCC;
							if( !isWriteable )
								SetAddressWriteable(ulAddressWithOffset,FALSE);
                            p->bUsed = TRUE;
                            p->bInstalled = TRUE;
                            p->bVirtual = FALSE;
                            // find next address
                            p->ulAddress = ulAddressWithOffset;
                            Disasm(&ulAddressWithOffset,(PUCHAR)&tempBp);
                            p->ulNextInstr = ulAddressWithOffset;
                            p->bPermanent = FALSE;
					        p->Callback = NULL;
                        }
                        else
                        {
                            DPRINT((0,"TryToInstallVirtualSWBreakpoints(): not valid address\n"));
                            PICE_memset(p,0,sizeof(*p));
                        }
                    }

                }
            }
        }
    }
}

//*************************************************************************
// RemoveSWBreakpoint()
//
// removes breakpoint from breakpoint list
//*************************************************************************
BOOLEAN RemoveSWBreakpoint(ULONG ulAddress)
{
    PSW_BP p;
    BOOLEAN bResult = FALSE;

    ENTER_FUNC();
    DPRINT((0,"RemoveSWBreakpoint()\n"));

    if( (p = FindSwBp(ulAddress)) )
    {
        if(IsAddressValid(ulAddress) && p->bInstalled == TRUE && p->bVirtual==FALSE)
        {
			BOOLEAN isWriteable;
			if( !( isWriteable = IsAddressWriteable(ulAddress) ) )
				SetAddressWriteable(ulAddress,TRUE);
		    // restore original opcode
            *(PUCHAR)(p->ulAddress) = p->ucOriginalOpcode;
			if( !isWriteable )
				SetAddressWriteable(ulAddress,FALSE);
        }

        PICE_memset(p,0,sizeof(*p));

        bResult = TRUE;
    }

    LEAVE_FUNC();

    return bResult;
}


//*************************************************************************
// DeInstallSWBreakpoint()
//
//*************************************************************************
BOOLEAN DeInstallSWBreakpoint(ULONG ulAddress)
{
    PSW_BP p;
    BOOLEAN bResult = FALSE;

    ENTER_FUNC();
    DPRINT((0,"DeInstallSWBreakpoint()\n"));

    if( (p = FindSwBp(ulAddress)) )
    {
        if(IsAddressValid(ulAddress) && p->bInstalled == TRUE && p->bVirtual==FALSE)
        {
			BOOLEAN isWriteable;
			if( !( isWriteable = IsAddressWriteable(ulAddress) ) )
				SetAddressWriteable(ulAddress,TRUE);
            // restore original opcode
            *(PUCHAR)(p->ulAddress) = p->ucOriginalOpcode;
			if( !isWriteable )
				SetAddressWriteable(ulAddress,FALSE);
        }

        p->bInstalled = FALSE;

        bResult = TRUE;
    }

    LEAVE_FUNC();

    return bResult;
}

//*************************************************************************
// RemoveAllSWBreakpoints()
//
//*************************************************************************
BOOLEAN RemoveAllSWBreakpoints(BOOLEAN bEvenPermanents)
{
    PSW_BP p;
    BOOLEAN bResult = FALSE;
    ULONG i;

    ENTER_FUNC();
    DPRINT((0,"RemoveAllSWBreakpoint()\n"));

    for(i=0;i<(sizeof(aSwBreakpoints)/sizeof(SW_BP));i++)
    {
        p = &aSwBreakpoints[i];
        if(p->bUsed == TRUE)
        {
            if(bEvenPermanents)
            {
                if(IsAddressValid(p->ulAddress) && p->bVirtual==FALSE)
                {
					BOOLEAN isWriteable;
					if( !( isWriteable = IsAddressWriteable(p->ulAddress) ) )
						SetAddressWriteable(p->ulAddress,TRUE);
                    *(PUCHAR)(p->ulAddress) = p->ucOriginalOpcode;
					if( !isWriteable )
						SetAddressWriteable(p->ulAddress,FALSE);
                    bResult = TRUE;
                }
                PICE_memset(p,0,sizeof(*p));
            }
            else
            {
                if(!p->bPermanent)
                {
                    if(IsAddressValid(p->ulAddress) && p->bVirtual==FALSE)
                    {
						BOOLEAN isWriteable;
						if( !( isWriteable = IsAddressWriteable(p->ulAddress) ) )
							SetAddressWriteable(p->ulAddress,TRUE);
                        *(PUCHAR)(p->ulAddress) = p->ucOriginalOpcode;
						if( !isWriteable )
							SetAddressWriteable(p->ulAddress,FALSE);
                        bResult = TRUE;
                    }
                    PICE_memset(p,0,sizeof(*p));
                }
            }
        }
    }

    LEAVE_FUNC();

    return bResult;
}

//*************************************************************************
// IsPermanentSWBreakpoint()
//
//*************************************************************************
PSW_BP IsPermanentSWBreakpoint(ULONG ulAddress)
{
    PSW_BP p;
    ULONG i;

    ENTER_FUNC();
    DPRINT((0,"IsPermanentSWBreakpoint(%.8X)\n",ulAddress));

    for(i=0;i<(sizeof(aSwBreakpoints)/sizeof(aSwBreakpoints[0]));i++)
    {
        p = &aSwBreakpoints[i];
        if(p->ulAddress == ulAddress &&
           p->bUsed == TRUE &&
           p->bPermanent == TRUE)
        {
            LEAVE_FUNC();
            return p;
        }
    }

    LEAVE_FUNC();

    return NULL;
}

//*************************************************************************
// ListSWBreakpoints()
//
//*************************************************************************
void ListSWBreakpoints(void)
{
    PSW_BP p;
    ULONG i;
    LPSTR pSymbolName;
    PDEBUG_MODULE pMod;

    ENTER_FUNC();
    DPRINT((0,"ListSWBreakpoints()\n"));

    for(i=0;i<(sizeof(aSwBreakpoints)/sizeof(SW_BP));i++)
    {
        p = &aSwBreakpoints[i];
        if(p->bUsed == TRUE && p->bVirtual == FALSE)
        {
            if((pSymbolName = FindFunctionByAddress(p->ulAddress,NULL,NULL)) )
            {
                pMod = FindModuleFromAddress(p->ulAddress);
                PICE_sprintf(tempBp,"[%u] %.8X (%S!%s) %s\n",i,p->ulAddress,pMod->name,pSymbolName,p->bPermanent?"PERMANENT":"");
            }
            else
            {
                if(ScanExportsByAddress(&pSymbolName,p->ulAddress))
                    PICE_sprintf(tempBp,"[%u] %.8X (%s) %s\n",i,p->ulAddress,pSymbolName,p->bPermanent?"PERMANENT":"");
                else
                    PICE_sprintf(tempBp,"[%u] %.8X (no symbol) %s\n",i,p->ulAddress,p->bPermanent?"PERMANENT":"");
            }
            Print(OUTPUT_WINDOW,tempBp);
        }
        else if(p->bUsed == TRUE)
        {
            PICE_sprintf(tempBp,"[%u] xxxxxxxx (%s!%s) VIRTUAL\n",i,p->szModName,p->szFunctionName);
            Print(OUTPUT_WINDOW,tempBp);
        }
    }

    LEAVE_FUNC();
}

//*************************************************************************
// RevirtualizeBreakpointsForModule()
//
//*************************************************************************
void RevirtualizeBreakpointsForModule(PDEBUG_MODULE pMod)
{
    ULONG i,start,end;
    PSW_BP p;
	char temp[DEBUG_MODULE_NAME_LEN];

    DPRINT((0,"RevirtualizeBreakpointsForModule(%x)\n",(ULONG)pMod));

	if(IsRangeValid((ULONG)pMod,sizeof(DEBUG_MODULE)) )
    {
        start = (ULONG)pMod->BaseAddress;
        end = (ULONG)pMod->BaseAddress+pMod->size;

		DPRINT((0,"RevirtualizeBreakpointsForModule(): module %x (%x-%x)\n",(ULONG)pMod,start,end));
		// go through all breakpoints
        for(i=0;i<(sizeof(aSwBreakpoints)/sizeof(SW_BP));i++)
        {
            p = &aSwBreakpoints[i];
			// if it's used and installed and not virtual
            if(p->bUsed && p->bInstalled && p->bVirtual == FALSE)
            {
				// make sure we're in module's bound
                if(p->ulAddress>=start && p->ulAddress<end)
                {
                    LPSTR pFind;
					ULONG ulFunctionAddress;

					DPRINT((0,"RevirtualizeBreakpointsForModule(): module breakpoint %u\n",i));
					// find the function in which this breakpoint resides
                    if(ScanExportsByAddress(&pFind,p->ulAddress))
                    {
						// from now on it's virtual again
                        p->bVirtual = TRUE;
						if(IsAddressValid(p->ulAddress) )
						{
							BOOLEAN isWriteable;
							if( !( isWriteable = IsAddressWriteable(p->ulAddress) ) )
								SetAddressWriteable(p->ulAddress,TRUE);
						    DPRINT((0,"RevirtualizeBreakpointsForModule(): restoring original opcode @ %x\n",p->ulAddress));
							*(PUCHAR)(p->ulAddress) = p->ucOriginalOpcode;
							if( !isWriteable )
								SetAddressWriteable(p->ulAddress,FALSE);
						}
						else
						{
						    DPRINT((0,"RevirtualizeBreakpointsForModule(): could not restore original opcode @ %x\n",p->ulAddress));
						}
						// skip past the module separator
                        while(*pFind!='!')pFind++;
                        pFind++;
						// remember the function and the module for reinstallation
						CopyWideToAnsi(temp,pMod->name);
						PICE_strcpy(p->szModName,temp);
                        PICE_strcpy(p->szFunctionName,pFind);
					    DPRINT((0,"RevirtualizeBreakpointsForModule(): %s!%s\n",p->szModName,p->szFunctionName));
						// if function name contains a '+' it's an offset
						pFind = p->szFunctionName;
						while(*pFind!=0)
						{
						    DPRINT((0,"RevirtualizeBreakpointsForModule(): [1] %s\n",pFind));
							// found any offset to function
							if(*pFind=='+')
							{
								*pFind=0;
								break;
							}
							pFind++;
						}

						DPRINT((0,"RevirtualizeBreakpointsForModule(): [2] %s\n",p->szFunctionName));
						if(ScanExports(p->szFunctionName,&ulFunctionAddress))
						{
							p->ulAddress -= ulFunctionAddress;
							DPRINT((0,"RevirtualizeBreakpointsForModule(): [1] function @ %x offset = %x\n",ulFunctionAddress,p->ulAddress));
						}
						else
						{
							if((ulFunctionAddress = FindFunctionInModuleByName(p->szFunctionName,pMod)) )
							{
								p->ulAddress -= ulFunctionAddress;
								DPRINT((0,"RevirtualizeBreakpointsForModule(): [2] function @ %x offset = %x\n",ulFunctionAddress,p->ulAddress));
							}
							else
							{
   								DPRINT((0,"RevirtualizeBreakpointsForModule(): Breakpoint %u could not be virtualized properly!\n",i));
								PICE_sprintf(tempBp,"Breakpoint %u could not be virtualized properly!\n",i);
								Print(OUTPUT_WINDOW,tempBp);
							}
						}
                    }
                    else
                    {
						DPRINT((0,"RevirtualizeBreakpointsForModule(): function for %x not found!\n",p->ulAddress));
                        PICE_memset(p,0,sizeof(*p));
                    }
                }
            }
        }
    }
}

//*************************************************************************
// NewInt3Handler()
//
//*************************************************************************
__asm__ ("\n\t \
NewInt3Handler:\n\t \
        pushl $" STR(REASON_INT3) "\n\t \
		// call debugger loop\n\t \
		jmp NewInt31Handler\n\t \
");


//*************************************************************************
// InstallInt3Hook()
//
//*************************************************************************
void InstallInt3Hook(void)
{
	ULONG LocalInt3Handler;

	ENTER_FUNC();
	DPRINT((0,"enter InstallInt3Hook()...\n"));

	MaskIrqs();
	if(!OldInt3Handler)
	{
		PICE_memset(aSwBreakpoints,0,sizeof(aSwBreakpoints));
		__asm__("mov $NewInt3Handler,%0"
			:"=r" (LocalInt3Handler)
			:
			:"eax");
		OldInt3Handler=SetGlobalInt(0x03,(ULONG)LocalInt3Handler);
	}
	UnmaskIrqs();

	DPRINT((0,"leave InstallInt3Hook()...\n"));
    LEAVE_FUNC();
}

//*************************************************************************
// DeInstallInt3Hook()
//
//*************************************************************************
void DeInstallInt3Hook(void)
{
	ENTER_FUNC();
	DPRINT((0,"enter DeInstallInt3Hook()...\n"));

	MaskIrqs();
	if(OldInt3Handler)
	{
        RemoveAllSWBreakpoints(TRUE);
		SetGlobalInt(0x03,(ULONG)OldInt3Handler);
        OldInt3Handler=0;
	}
	UnmaskIrqs();

	DPRINT((0,"leave DeInstallInt3Hook()...\n"));
    LEAVE_FUNC();
}
