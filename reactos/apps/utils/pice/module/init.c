/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    init.c

Abstract:

    initialisation and cleanup of debugger kernel module

Environment:

    Kernel mode only

Author:

    Klaus P. Gerlicher

Revision History:

    25-Jan-1999:	created
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
ULONG ulDoInitialBreak=1;
char szBootParams[1024]="";
char tempInit[256];

PDIRECTORY_OBJECT *pNameSpaceRoot = NULL;
PDEBUG_MODULE pdebug_module_tail = NULL;
PDEBUG_MODULE pdebug_module_head = NULL;
PMADDRESS_SPACE mm_init_mm;

ULONG KeyboardIRQL;

extern void NewInt31Handler(void);
//*************************************************************************
// InitPICE()
//
//*************************************************************************
BOOLEAN InitPICE(void)
{
    ULONG ulHandleScancode=0,ulHandleKbdEvent=0;
	ARGS Args;
    KIRQL Dirql;
    KAFFINITY Affinity;
	ULONG ulAddr;

    ENTER_FUNC();

	DPRINT((0,"InitPICE(): trace step 0.5\n"));
    KeyboardIRQL = HalGetInterruptVector(Internal,
				     0,
				     0,
				     KEYBOARD_IRQ,
				     &Dirql,
				     &Affinity);
	DPRINT((0,"KeyboardIRQL: %x\n", KeyboardIRQL));

    DPRINT((0,"InitPICE(): trace step 1\n"));
    // enable monochrome passthrough on BX type chipset
    EnablePassThrough();

    DPRINT((0,"InitPICE(): trace step 2\n"));
    // now load all symbol files described in /etc/pice.conf
    if(!LoadSymbolsFromConfig(FALSE))
    {
        DPRINT((0,"InitPICE: LoadSymbolsFromConfig() failed\n"));
        LEAVE_FUNC();
        return FALSE;
    }

    DPRINT((0,"InitPICE(): trace step 3\n"));
    // init the output console
	// this might be one of the following depending setup
	// a) monochrome card
	// b) serial terminal (TODO)
    if(!ConsoleInit())
    {
        DPRINT((0,"InitPICE: ConsoleInit() failed\n"));
        UnloadSymbols();
        LEAVE_FUNC();
        return FALSE;
    }

    DPRINT((0,"InitPICE(): trace step 4\n"));
    // print the initial screen template
    PrintTemplate();
/*
    DPRINT((0,"InitPICE(): trace step 5\n"));
	// ask the user if he wants to abort the debugger load
    if(!CheckLoadAbort())
	{
		Print(OUTPUT_WINDOW,"pICE: ABORT (abort by user)\n");
        UnloadSymbols();
		ConsoleShutdown();
        LEAVE_FUNC();
		return FALSE;
	}
*/
    DPRINT((0,"InitPICE(): trace step 6\n"));
    // load the file /boot/System.map.
    // !!! It must be consistent with the current kernel at all cost!!!
    if(!LoadExports())
    {
		Print(OUTPUT_WINDOW,"pICE: failed to load exports\n");
        Print(OUTPUT_WINDOW,"press any key to continue...\n");
        while(!GetKeyPolled());
        UnloadSymbols();
		ConsoleShutdown();
        LEAVE_FUNC();
		return FALSE;
    }

    DPRINT((0,"InitPICE(): trace step 7\n"));
	ScanExports("_KernelAddressSpace", &ulAddr);
	my_init_mm = (PMADDRESS_SPACE) ulAddr;
	DPRINT((0,"init_mm %x @ %x\n",&my_init_mm,my_init_mm));
	if(!my_init_mm)
	{
		Print(OUTPUT_WINDOW,"pICE: ABORT (initial memory map not found)\n");
		Print(OUTPUT_WINDOW,"pICE: press any key to continue...\n");
		DbgPrint("pICE: ABORT (initial memory map not found)\n");
		DbgPrint("pICE: press any key to continue...\n");
        while(!GetKeyPolled());
        UnloadSymbols();
		ConsoleShutdown();
        LEAVE_FUNC();
		return FALSE;
	}

    DPRINT((0,"InitPICE(): trace step 7.1\n"));

	ScanExports("_PsProcessListHead",&ulAddr);
	pPsProcessListHead = (LIST_ENTRY*)ulAddr;
    DPRINT((0,"pPsProcessListHead @ %X\n",pPsProcessListHead));
	if(!pPsProcessListHead)
	{
		Print(OUTPUT_WINDOW,"pICE: ABORT (PsProcessListHead not found)\n");
		Print(OUTPUT_WINDOW,"pICE: press any key to continue...\n");
        while(!GetKeyPolled());
        UnloadSymbols();
		ConsoleShutdown();
        LEAVE_FUNC();
		return FALSE;
	}

    DPRINT((0,"InitPICE(): trace step 8\n"));
    // end of the kernel
	/*
	ScanExports("_end",(PULONG)&kernel_end);
    if(!kernel_end)
	{
		Print(OUTPUT_WINDOW,"pICE: ABORT (kernel size is unknown)\n");
		Print(OUTPUT_WINDOW,"pICE: press any key to continue...\n");
        while(!GetKeyPolled());
		UnloadExports();
        UnloadSymbols();
		ConsoleShutdown();
        LEAVE_FUNC();
		return FALSE;
	}
	*/

    DPRINT((0,"InitPICE(): trace step 9\n"));

	// the loaded module list
	ScanExports("_NameSpaceRoot", &ulAddr);
	pNameSpaceRoot = (PDIRECTORY_OBJECT *)ulAddr;
	DPRINT((0,"pNameSpaceRoot @ %X\n",pNameSpaceRoot));
    if(!pNameSpaceRoot)
	{
		Print(OUTPUT_WINDOW,"pICE: ABORT (couldn't retreive name space root)\n");
		Print(OUTPUT_WINDOW,"pICE: press any key to continue...\n");
        while(!GetKeyPolled());
		UnloadExports();
        UnloadSymbols();
		ConsoleShutdown();
        LEAVE_FUNC();
		return FALSE;
	}

    DPRINT((0,"InitPICE(): trace step 10\n"));
    // setup a linked list for use in module parsing routines.
	if(!InitModuleList(&pdebug_module_head, 100))
	{
		Print(OUTPUT_WINDOW,"pICE: ABORT (couldn't initialize kernel module list)\n");
		Print(OUTPUT_WINDOW,"pICE: press any key to continue...\n");
		FreeModuleList( pdebug_module_head );
        while(!GetKeyPolled());
		UnloadExports();
        UnloadSymbols();
		ConsoleShutdown();
        LEAVE_FUNC();
		return FALSE;
	}
	pdebug_module_tail = pdebug_module_head;

    DPRINT((0,"InitPICE(): trace step 11\n"));
    // do a sanity check on exports
    if(!SanityCheckExports())
    {
		Print(OUTPUT_WINDOW,"pICE: ABORT (exports are conflicting with kernel symbols)\n");
		Print(OUTPUT_WINDOW,"pICE: press any key to continue...\n");
        while(!GetKeyPolled());
		UnloadExports();
        UnloadSymbols();
		ConsoleShutdown();
        LEAVE_FUNC();
		return FALSE;
    }

    DPRINT((0,"InitPICE(): trace step 12\n"));


    DPRINT((0,"InitPICE(): trace step 13\n"));
    // patch the keyboard driver

	if(!PatchKeyboardDriver())
	{
		Print(OUTPUT_WINDOW,"pICE: ABORT (couldn't patch keyboard driver)\n");
		Print(OUTPUT_WINDOW,"pICE: press any key to continue...\n");
        while(!GetKeyPolled());
		UnloadSymbols();
		UnloadExports();
		ConsoleShutdown();
        LEAVE_FUNC();
		return FALSE;
	}

    DPRINT((0,"InitPICE(): trace step 14\n"));
    // partial init of shadow registers
    CurrentCS = GLOBAL_CODE_SEGMENT;
    CurrentEIP = (ULONG)RealIsr;

    CurrentDS = CurrentSS = GLOBAL_DATA_SEGMENT;
    __asm__("
            mov %%esp,%%eax
            mov %%eax,_CurrentESP
            ":::"eax");


    // display version and symbol information
    Ver(NULL);

    // disable HW breakpoints
	__asm__("
		xorl %%eax,%%eax
		mov %%eax,%%dr6
		mov %%eax,%%dr7
        mov %%dr0,%%eax
        mov %%dr1,%%eax
        mov %%dr2,%%eax
        mov %%dr3,%%eax"
		:::"eax"
		);

    DPRINT((0,"InitPICE(): trace step 15\n"));
    TakeIdtSnapshot();

    DPRINT((0,"InitPICE(): trace step 16\n"));
    // install all hooks
    InstallTraceHook();
    InstallGlobalKeyboardHook();
    InstallSyscallHook();
    InstallInt3Hook();
    InstallDblFltHook();
    InstallGPFaultHook();
    InstallIntEHook();
    InstallPrintkHook();

    DPRINT((0,"InitPICE(): trace step 16\n"));
    if(ulDoInitialBreak)
    {
        DPRINT((0,"about to do initial break...\n"));

        // simulate an initial break
        __asm__("
            pushfl
            pushl %cs
            pushl $initialreturnpoint
            pushl $" STR(REASON_CTRLF) "
            jmp NewInt31Handler
initialreturnpoint:");
    }
    else
    {
        // display register contents
        DisplayRegs();

        // display data window
        Args.Value[0]=CurrentDS;
        Args.Value[1]=CurrentEIP;
        Args.Count=2;
        DisplayMemory(&Args);

        // disassembly from current address
        Args.Value[0]=CurrentCS;
        Args.Value[1]=CurrentEIP;
        Args.Count=2;
        Unassemble(&Args);
    }

    DPRINT((0,"InitPICE(): trace step 17\n"));
	InitPiceRunningTimer();

    LEAVE_FUNC();
    return TRUE;
}

//*************************************************************************
// CleanUpPICE()
//
//*************************************************************************
void CleanUpPICE(void)
{
    DPRINT((0,"CleanUpPICE(): trace step 1\n"));
	RemovePiceRunningTimer();

    DPRINT((0,"CleanUpPICE(): trace step 2\n"));
    // de-install all hooks
    DeInstallGlobalKeyboardHook();
    DeInstallSyscallHook();
    DeInstallInt3Hook();
    DeInstallPrintkHook();
    DeInstallDblFltHook();
    DeInstallGPFaultHook();
    DeInstallIntEHook();
    DeInstallTraceHook();

    DPRINT((0,"CleanUpPICE(): trace step 3\n"));
    RestoreIdt();

    DPRINT((0,"CleanUpPICE(): trace step 4\n"));
    UnloadExports(); // don't use ScanExports() after this
    UnloadSymbols();

    DPRINT((0,"CleanUpPICE(): trace step 5\n"));
    // restore patch of keyboard driver
    RestoreKeyboardDriver();

    DPRINT((0,"CleanUpPICE(): trace step 6\n"));
    Print(OUTPUT_WINDOW,"pICE: shutting down...\n");

    DPRINT((0,"CleanUpPICE(): trace step 7\n"));
    // cleanup the console
	ConsoleShutdown();
}
