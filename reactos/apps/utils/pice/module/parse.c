/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    parse.c

Abstract:

    execution of debugger commands

Environment:

    Kernel mode only

Author:

    Klaus P. Gerlicher

Revision History:

    19-Aug-1998:	created
    15-Nov-2000:    general cleanup of source files

Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/

////////////////////////////////////////////////////
// INCLUDES
////
#include "remods.h"
#include "precomp.h"
#include "pci_ids.h"

///////////////////////////////////////////////////
// GLOBALS

ULONG ValueTrue=1,ValueFalse=0;
ULONG ulLastDisassStartAddress=0,ulLastDisassEndAddress=0,ulLastInvertedAddress=0;
USHORT gCurrentSelector=0;
ULONG gCurrentOffset=0;
LONG ulCurrentlyDisplayedLineNumber=0;
USHORT usOldDisasmSegment = 0;
ULONG ulOldDisasmOffset = 0;
static ULONG ulCountForWaitKey = 0;

extern PDEBUG_MODULE pdebug_module_head;
extern PDEBUG_MODULE pdebug_module_tail;

//extern unsigned long sys_call_table[];

BOOLEAN (*DisplayMemory)(PARGS) = DisplayMemoryDword;

char szCurrentFile[256]="";
PDEBUG_MODULE pCurrentMod=NULL;
PICE_SYMBOLFILE_HEADER* pCurrentSymbols=NULL;

// suppresses passing on of function keys while stepping code
BOOLEAN bStepping = FALSE;
BOOLEAN bInt3Here = TRUE;
BOOLEAN bInt1Here = TRUE;

KEYWORDS RegKeyWords[]={
	{"eax",&CurrentEAX,sizeof(ULONG)},
	{"ebx",&CurrentEBX,sizeof(ULONG)},
	{"ecx",&CurrentECX,sizeof(ULONG)},
	{"edx",&CurrentEDX,sizeof(ULONG)},
	{"edi",&CurrentEDI,sizeof(ULONG)},
	{"esi",&CurrentESI,sizeof(ULONG)},
	{"ebp",&CurrentEBP,sizeof(ULONG)},
	{"esp",&CurrentESP,sizeof(ULONG)},
	{"eip",&CurrentEIP,sizeof(ULONG)},
	{NULL,0,0}
};

KEYWORDS SelectorRegKeyWords[]={
	{"cs",&CurrentCS,sizeof(USHORT)},
	{"ds",&CurrentDS,sizeof(USHORT)},
	{"es",&CurrentES,sizeof(USHORT)},
	{"fs",&CurrentFS,sizeof(USHORT)},
	{"gs",&CurrentGS,sizeof(USHORT)},
	{"ss",&CurrentSS,sizeof(USHORT)},
	{NULL,0,0}
};

KEYWORDS OnOffKeyWords[]={
	{"on",&ValueTrue,sizeof(ULONG)},
	{"off",&ValueFalse,sizeof(ULONG)},
	{NULL,0,0}
};

KEYWORDS SpecialKeyWords[]={
	{"process",&CurrentProcess,sizeof(ULONG)},
	{NULL,0,0}
};

LPSTR LocalVarRegs[]=
{
    "EAX",
    "ECX",
    "EDX",
    "EBX",
    "ESP",
    "EBP",
    "ESI",
    "EDI",
    "EIP",
    "EFL",
    "CS",
    "SS",
    "DS",
    "ES",
    "FS",
    "GS"
};


#define COMMAND_HAS_NO_PARAMS       (0)
#define COMMAND_HAS_PARAMS          (1<<0)
#define COMMAND_HAS_SWITCHES        (1<<1)
//
#define PARAM_CAN_BE_SYMBOLIC           (1<<0)
#define PARAM_CAN_BE_SEG_OFFSET         (1<<1)
#define PARAM_CAN_BE_MODULE             (1<<2)
#define PARAM_CAN_BE_PRNAME             (1<<3)
#define PARAM_CAN_BE_PID                (1<<4)
#define PARAM_CAN_BE_SRC_FILE           (1<<5)
#define PARAM_CAN_BE_NUMERIC            (1<<6)
#define PARAM_CAN_BE_REG_KEYWORD        (1<<7)
#define PARAM_CAN_BE_ONOFF_KEYWORD      (1<<8)
#define PARAM_CAN_BE_SPECIAL_KEYWORD    (1<<9)
#define PARAM_CAN_BE_ASTERISK           (1<<10)
#define PARAM_CAN_BE_ONOFF		        (1<<11)
#define PARAM_CAN_BE_VIRTUAL_SYMBOLIC   (1<<12)
#define PARAM_CAN_BE_SRCLINE            (1<<13)
#define PARAM_CAN_BE_PARTIAL_SYM_NAME   (1<<14)
#define PARAM_CAN_BE_ANY_STRING		    (1<<15)
#define PARAM_CAN_BE_DECIMAL            (1<<16)
#define PARAM_CAN_BE_SIZE_DESC          (1<<17)
#define PARAM_CAN_BE_LETTER             (1<<18)
//
#define COMMAND_GROUP_HELP              (0)
#define COMMAND_GROUP_FLOW              (1)
#define COMMAND_GROUP_STRUCT            (2)
#define COMMAND_GROUP_OS                (3)
#define COMMAND_GROUP_MEM               (4)
#define COMMAND_GROUP_BREAKPOINT        (5)
#define COMMAND_GROUP_WINDOW            (6)
#define COMMAND_GROUP_DEBUG             (7)
#define COMMAND_GROUP_INFO              (8)
#define COMMAND_GROUP_STATE             (9)
#define COMMAND_GROUP_HELP_ONLY         (10)
#define COMMAND_GROUP_LAST              (11)

LPSTR CommandGroups[]=
{
    "HELP",
    "FLOW CONTROL",
    "STRUCTURES",
    "OS SPECIFIC",
    "MEMORY",
    "BREAKPOINTS",
    "WINDOW",
    "DEBUGGING",
    "INFORMATION",
    "STATE",
    "EDITOR",
    NULL
};
// table of command handlers
CMDTABLE CmdTable[]={
	{"gdt",ShowGdt,"display current global descriptor table"		,0,{0,0,0,0,0},"",COMMAND_GROUP_STRUCT},
	{"idt",ShowIdt,"display current interrupt descriptor table"		,0,{0,0,0,0,0},"",COMMAND_GROUP_STRUCT},
	{"x",LeaveIce,"return to Reactos"								,0,{0,0,0,0,0},"",COMMAND_GROUP_FLOW},
	{"t",SingleStep,"single step one instruction"					,0,{0,0,0,0,0},"",COMMAND_GROUP_FLOW},
	{"vma",ShowVirtualMemory,"displays VMAs"						,0,{0,0,0,0,0},"",COMMAND_GROUP_OS},
	{"h",ShowHelp,"list help on commands"							,0,{0,0,0,0,0},"",COMMAND_GROUP_HELP},
	{"page",ShowPageDirs,"dump page directories"					,COMMAND_HAS_PARAMS,{PARAM_CAN_BE_NUMERIC|PARAM_CAN_BE_REG_KEYWORD,0,0,0,0},"",COMMAND_GROUP_STRUCT},
	{"proc",ShowProcesses,"list all processes"						,COMMAND_HAS_PARAMS,{PARAM_CAN_BE_PRNAME|PARAM_CAN_BE_PID,0,0,0,0},"",COMMAND_GROUP_OS},
	{"dd",DisplayMemoryDword,"display dword memory"         		,COMMAND_HAS_PARAMS,{PARAM_CAN_BE_NUMERIC|PARAM_CAN_BE_SYMBOLIC|PARAM_CAN_BE_REG_KEYWORD,0,0,0,0},"",COMMAND_GROUP_MEM},
	{"db",DisplayMemoryByte,"display byte memory "		            ,COMMAND_HAS_PARAMS,{PARAM_CAN_BE_NUMERIC|PARAM_CAN_BE_SYMBOLIC|PARAM_CAN_BE_REG_KEYWORD,0,0,0,0},"",COMMAND_GROUP_MEM},
	{"dpd",DisplayPhysMemDword,"display dword physical memory"      ,COMMAND_HAS_PARAMS,{PARAM_CAN_BE_NUMERIC,0,0,0,0},"",COMMAND_GROUP_MEM},
	{"u",Unassemble,"disassemble at address"						,COMMAND_HAS_PARAMS|COMMAND_HAS_SWITCHES,{PARAM_CAN_BE_NUMERIC|PARAM_CAN_BE_SYMBOLIC|PARAM_CAN_BE_REG_KEYWORD|PARAM_CAN_BE_SRCLINE,0,0,0,0},"f",COMMAND_GROUP_MEM},
	{"mod",ShowModules,"displays all modules"					    ,0,{0,0,0,0,0},"",COMMAND_GROUP_OS},
	{"bpx",SetBreakpoint,"set code breakpoint"						,COMMAND_HAS_PARAMS,{PARAM_CAN_BE_NUMERIC|PARAM_CAN_BE_VIRTUAL_SYMBOLIC|PARAM_CAN_BE_SYMBOLIC|PARAM_CAN_BE_SRCLINE|PARAM_CAN_BE_REG_KEYWORD,0,0,0,0},"",COMMAND_GROUP_BREAKPOINT},
	{"bl",ListBreakpoints,"list breakpoints"						,COMMAND_HAS_PARAMS,{PARAM_CAN_BE_NUMERIC,0,0,0,0},"",COMMAND_GROUP_BREAKPOINT},
	{"bc",ClearBreakpoints,"clear breakpoints"						,COMMAND_HAS_PARAMS,{PARAM_CAN_BE_NUMERIC|PARAM_CAN_BE_ASTERISK,0,0,0,0},"",COMMAND_GROUP_BREAKPOINT},
	{"ver",Ver,"display pICE version and state information"			,0,{0,0,0,0,0},"",COMMAND_GROUP_INFO},
	{"hboot",Hboot,"hard boot the system"							,0,{0,0,0,0,0},"",COMMAND_GROUP_FLOW},
	{"code",SetCodeDisplay,"toggle code display"					,COMMAND_HAS_PARAMS,{PARAM_CAN_BE_ONOFF,0,0,0,0},"",COMMAND_GROUP_STATE},
	{"cpu",ShowCPU,"display CPU special registers"					,0,{0,0,0,0,0},"",COMMAND_GROUP_STRUCT},
	{"stack",WalkStack,"display call stack"							,0,{0,0,0,0,0},"",COMMAND_GROUP_STRUCT},
	{"peek",PeekMemory,"peek at physical memory"    		   		,COMMAND_HAS_PARAMS,{PARAM_CAN_BE_SIZE_DESC,PARAM_CAN_BE_NUMERIC,0,0,0},"",COMMAND_GROUP_MEM},
	{"poke",PokeMemory,"poke to physical memory"            		,COMMAND_HAS_PARAMS,{PARAM_CAN_BE_SIZE_DESC,PARAM_CAN_BE_NUMERIC,PARAM_CAN_BE_NUMERIC,0,0},"",COMMAND_GROUP_MEM},
	{".",UnassembleAtCurrentEip,"unassemble at current instruction" ,0,{0,0,0,0,0},"",COMMAND_GROUP_MEM},
	{"p",StepOver,"single step over call"							,0,{0,0,0,0,0},"",COMMAND_GROUP_FLOW},
	{"i",StepInto,"single step into call"							,0,{0,0,0,0,0},"",COMMAND_GROUP_FLOW},
	{"locals",ShowLocals,"display local symbols"					,0,{0,0,0,0,0},"",COMMAND_GROUP_MEM},
	{"table",SwitchTables,"display loaded symbol tables"			,COMMAND_HAS_PARAMS,{PARAM_CAN_BE_MODULE,0,0,0,0},"",COMMAND_GROUP_DEBUG},
	{"file",SwitchFiles,"display source files in symbol table"		,COMMAND_HAS_PARAMS,{PARAM_CAN_BE_SRC_FILE,0,0,0,0},"",COMMAND_GROUP_DEBUG},
	{"sym",ShowSymbols,"list known symbol information"				,COMMAND_HAS_PARAMS,{PARAM_CAN_BE_PARTIAL_SYM_NAME,0,0,0,0},"",COMMAND_GROUP_DEBUG},
	{"?",EvaluateExpression,"evaluate an expression"				,COMMAND_HAS_PARAMS,{PARAM_CAN_BE_ANY_STRING,0,0,0,0},"",COMMAND_GROUP_DEBUG},
	{"src",SetSrcDisplay,"sets disassembly mode"					,0,{0,0,0,0,0},"",COMMAND_GROUP_DEBUG},
	{"wc",SizeCodeWindow,"change size of code window"	   			,COMMAND_HAS_PARAMS,{PARAM_CAN_BE_DECIMAL,0,0,0,0},"",COMMAND_GROUP_WINDOW},
	{"wd",SizeDataWindow,"change size of data window"	   			,COMMAND_HAS_PARAMS,{PARAM_CAN_BE_DECIMAL,0,0,0,0},"",COMMAND_GROUP_WINDOW},
	{"r",SetGetRegisters,"sets or displays registers"				,COMMAND_HAS_PARAMS,{PARAM_CAN_BE_REG_KEYWORD,PARAM_CAN_BE_NUMERIC,0,0,0},"",COMMAND_GROUP_STRUCT},
	{"cls",ClearScreen,"clear output window"     					,0,{0,0,0,0,0},"",COMMAND_GROUP_WINDOW},
	{"phys",ShowMappings,"show all mappings for linear address"		,COMMAND_HAS_PARAMS,{PARAM_CAN_BE_NUMERIC,0,0,0,0},"",COMMAND_GROUP_MEM},
	{"timers",ShowTimers,"show all active timers"					,0,{0,0,0,0,0},"",COMMAND_GROUP_OS},
	{"pci",ShowPCI,"show PCI devices"   					        ,COMMAND_HAS_PARAMS|COMMAND_HAS_SWITCHES,{PARAM_CAN_BE_DECIMAL,PARAM_CAN_BE_DECIMAL,0,0,0},"a",COMMAND_GROUP_INFO},
	{"next",NextInstr,"advance EIP to next instruction"				,0,{0,0,0,0,0},""},
	{"i3here",I3here,"catch INT 3s"									,COMMAND_HAS_PARAMS,{PARAM_CAN_BE_ONOFF,0,0,0,0},"",COMMAND_GROUP_FLOW},
	{"layout",SetKeyboardLayout,"sets keyboard layout"  			,COMMAND_HAS_PARAMS,{PARAM_CAN_BE_DECIMAL,0,0,0,0},"",COMMAND_GROUP_FLOW},
	{"syscall",ShowSysCallTable,"displays syscall (table)" 			,COMMAND_HAS_PARAMS,{PARAM_CAN_BE_DECIMAL,0,0,0,0},"",COMMAND_GROUP_FLOW},
	{"altkey",SetAltKey,"set alternate break key"        			,COMMAND_HAS_PARAMS,{PARAM_CAN_BE_LETTER,0,0,0,0},"",COMMAND_GROUP_FLOW},
	{"addr",ShowContext,"show/set address contexts"            		,COMMAND_HAS_PARAMS,{PARAM_CAN_BE_PRNAME,0,0,0,0},"",COMMAND_GROUP_FLOW},
	{"arrow up",NULL,""            		                            ,0,{0,0,0,0,0},"",COMMAND_GROUP_HELP_ONLY},
    {NULL,0,NULL}
};

char tempCmd[1024];

char HexDigit[] = { '0', '1', '2', '3', '4', '5', '6', '7',
                    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };


CPUINFO CPUInfo[]={
	{"DR0",&CurrentDR0},
	{"DR1",&CurrentDR1},
	{"DR2",&CurrentDR2},
	{"DR3",&CurrentDR3},
	{"DR6",&CurrentDR6},
	{"DR7",&CurrentDR7},
	{"EFLAGS",&CurrentEFL},
	{"CR0",&CurrentCR0},
	{"CR2",&CurrentCR2},
	{"CR3",&CurrentCR3},
	{"",NULL},
};

BP Bp[4]={
	{0,0,0,FALSE,FALSE,FALSE,"",""},
	{0,0,0,FALSE,FALSE,FALSE,"",""},
	{0,0,0,FALSE,FALSE,FALSE,"",""},
	{0,0,0,FALSE,FALSE,FALSE,"",""}
};

BOOLEAN bShowSrc = TRUE;
BOOLEAN bCodeOn = FALSE;
BOOLEAN bNeedToFillBuffer = TRUE;

char *NonSystemSegmentTypes[]=
{
	"Data RO",
	"Data RO accessed",
	"Data RW",
	"Data RW accessed",
	"Data RO expand-dwon",
	"Data RO expand-down, accessed",
	"Data RW expand-dwon",
	"Data RW expand-down, accessed",
	"Code EO",
	"Code EO accessed",
	"Code ER",
	"Code ER accessed",
	"Code EO conforming",
	"Code EO conforming, accessed",
	"Code ER conforming",
	"Code ER conforming, accessed"
};

char *SystemSegmentTypes[]=
{
	"reserved0",
	"16-bit TSS (available)",
	"LDT",
	"16-bit TSS (busy)",
	"16-bit call gate",
	"task gate",
	"16-bit interrupt gate",
	"16-bit trap gate",
	"reserved1",
	"32-bit TSS (available)",
	"reserved2",
	"32-bit TSS (busy)",
	"32-bit call gate",
	"reserved3",
	"32-bit interrupt gate",
	"32-bit trap gate"
};

////////////////////////////////////////////////////
// FUNCTIONS
////

//*************************************************************************
// RepaintSource()
//
//*************************************************************************
void RepaintSource(void)
{
    ARGS Args;

    ENTER_FUNC();

    // disassembly from current address
    PICE_memset(&Args,0,sizeof(ARGS));
    // make unassembler refresh all again
    ulLastDisassStartAddress=ulLastDisassEndAddress=0;
	Args.Count=0;
	Unassemble(&Args);

    LEAVE_FUNC();
}

//*************************************************************************
// RepaintDesktop()
//
//*************************************************************************
void RepaintDesktop(void)
{
    ARGS Args;

    ENTER_FUNC();

    PrintTemplate();

    DisplayRegs();

    // display data window
	Args.Value[0]=OldSelector;
	Args.Value[1]=OldOffset;
	Args.Count=2;
	DisplayMemory(&Args);

    // disassembly from current address
    PICE_memset(&Args,0,sizeof(ARGS));
    // make unassembler refresh all again
    ulLastDisassStartAddress=ulLastDisassEndAddress=0;
	Args.Count=0;
	Unassemble(&Args);

    PrintRingBuffer(wWindow[OUTPUT_WINDOW].cy-1);
    Print(OUTPUT_WINDOW,"");

    ShowStoppedMsg();
    ShowStatusLine();

    LEAVE_FUNC();
}

//*************************************************************************
// PutStatusText()
//
//*************************************************************************
void PutStatusText(LPSTR p)
{
    ENTER_FUNC();

	ClrLine(wWindow[OUTPUT_WINDOW].y-1);
	PutChar(p,1,wWindow[OUTPUT_WINDOW].y-1);

    LEAVE_FUNC();
}

//*************************************************************************
// WaitForKey()
//
//*************************************************************************
BOOLEAN WaitForKey(void)
{
    BOOLEAN result=TRUE;

    if(ulCountForWaitKey == 0)
        SuspendPrintRingBuffer(TRUE);

    ulCountForWaitKey++;

	if(ulCountForWaitKey == (wWindow[OUTPUT_WINDOW].cy-1))
	{
        SuspendPrintRingBuffer(FALSE);

    	PrintRingBuffer(wWindow[OUTPUT_WINDOW].cy-1);

        ulCountForWaitKey = 0;

		SetBackgroundColor(WHITE);
		ClrLine(wWindow[OUTPUT_WINDOW].y+wWindow[OUTPUT_WINDOW].cy);
		PutChar(" Press any key to continue listing or press ESC to stop... ",1,wWindow[OUTPUT_WINDOW].y+wWindow[OUTPUT_WINDOW].cy);
		ucKeyPressedWhileIdle=0;
        while(!(ucKeyPressedWhileIdle=GetKeyPolled()))
		{
			PrintCursor(FALSE);
		}
		SetBackgroundColor(BLACK);
		// if ESCAPE then indicate retreat
		if(ucKeyPressedWhileIdle==SCANCODE_ESC)
		{
			result=FALSE;
		}
		ucKeyPressedWhileIdle=0;
	}


    return result;
}

/////////////////////////////////////////////////////////////
// command handlers
/////////////////////////////////////////////////////////////

//*************************************************************************
// SingleStep()
//
//*************************************************************************
COMMAND_PROTOTYPE(SingleStep)
{
    ULONG ulLineNumber;
    LPSTR pSrcStart,pSrcEnd,pFilename;

    ENTER_FUNC();

    if(FindSourceLineForAddress(GetLinearAddress(CurrentCS,CurrentEIP),&ulLineNumber,&pSrcStart,&pSrcEnd,&pFilename))
    {
        DPRINT((0,"SingleStep(): stepping into source\n"));
        StepInto(NULL);
    }
    else
    {
	    // modify trace flag
	    CurrentEFL|=0x100; // set trace flag (TF)

	    bSingleStep=TRUE;
	    bNotifyToExit=TRUE;
    }

    bStepping = TRUE;

    LEAVE_FUNC();

    return TRUE;
}


//*************************************************************************
// StepOver()
//
// step over calls
//*************************************************************************
COMMAND_PROTOTYPE(StepOver)
{
	char tempDisasm[256];
	ULONG dwBreakAddress;
    ULONG ulLineNumber;
    LPSTR pSrcStart,pSrcEnd,pFilename;

    ENTER_FUNC();

    // only no arguments supplied
    // when we have source and current disassembly mod is SOURCE
    // we have to analyse the code block for the source line
    if(FindSourceLineForAddress(GetLinearAddress(CurrentCS,CurrentEIP),&ulLineNumber,&pSrcStart,&pSrcEnd,&pFilename))
    {
        DPRINT((0,"StepOver(): we have source here!\n"));
        DPRINT((0,"StepOver(): line #%u in file = %s!\n",ulLineNumber,pFilename));

        g_ulLineNumberStart = ulLineNumber;
        bStepThroughSource = TRUE;

        // deinstall the INT3 in kernel's printk()
        DeInstallPrintkHook();

        goto proceed_as_normal;
    }
    else
    {
        DPRINT((0,"StepOver(): no source here!\n"));

proceed_as_normal:
		// if there is some form of call instruction at EIP we need to find
		// the return address
		if(IsCallInstrAtEIP())
		{
			// get address of next instruction
			dwBreakAddress=GetLinearAddress(CurrentCS,CurrentEIP);

			Disasm(&dwBreakAddress,tempDisasm);

			DPRINT((0,"address of break = %.4X:%.8X\n",CurrentCS,dwBreakAddress));

			dwBreakAddress=GetLinearAddress(CurrentCS,dwBreakAddress);

			DPRINT((0,"linear address of break = %.8X\n",dwBreakAddress));

			DPRINT((0,"setting DR0=%.8X\n",dwBreakAddress));

            SetHardwareBreakPoint(dwBreakAddress,0);

			bSingleStep = FALSE;
			bNotifyToExit = TRUE;
		}
		else
		{
	        // modify trace flag
	        CurrentEFL|=0x100; // set trace flag (TF)

	        bSingleStep=TRUE;
	        bNotifyToExit=TRUE;
		}
    }

    bStepInto = FALSE;

    bStepping = TRUE;

    LEAVE_FUNC();

    return TRUE;
}

//*************************************************************************
// StepInto()
//
// step into calls
//*************************************************************************
COMMAND_PROTOTYPE(StepInto)
{
    ULONG ulLineNumber;
    LPSTR pSrcStart,pSrcEnd,pFilename;

    ENTER_FUNC();

    // only no arguments supplied
    // when we have source and current disassembly mod is SOURCE
    // we have to analyse the code block for the source line
    if(FindSourceLineForAddress(GetLinearAddress(CurrentCS,CurrentEIP),&ulLineNumber,&pSrcStart,&pSrcEnd,&pFilename))
    {
        DPRINT((0,"StepOver(): we have source here!\n"));
        DPRINT((0,"StepOver(): line #%u in file = %s!\n",ulLineNumber,pFilename));

        g_ulLineNumberStart = ulLineNumber;
        bStepThroughSource = TRUE;

        // deinstall the INT3 in kernel's printk()
        DeInstallPrintkHook();

        goto proceed_as_normal_into;
    }
    else
    {
        DPRINT((0,"StepInto(): no source here!\n"));

proceed_as_normal_into:

	    // modify trace flag
	    CurrentEFL|=0x100; // set trace flag (TF)

	    bSingleStep=TRUE;
	    bNotifyToExit=TRUE;
    }

    bStepInto = TRUE;

    bStepping = TRUE;

    LEAVE_FUNC();

    return TRUE;
}

//*************************************************************************
// SetBreakpoint()
//
//*************************************************************************
COMMAND_PROTOTYPE(SetBreakpoint)
{
    ULONG addr,addrorg;
    USHORT segment;

	if(pArgs->Count<=2)
	{
        if(pArgs->bNotTranslated[0]==FALSE)
        {
		    if(gCurrentSelector)
		    {
			    addr=pArgs->Value[0];
			    addrorg=gCurrentOffset;
			    segment=gCurrentSelector;
		    }
		    else
		    {
			    addrorg=addr=pArgs->Value[0];
			    segment=CurrentCS;
		    }

            if(InstallSWBreakpoint(GetLinearAddress(segment,addr),FALSE,NULL) )
            {
		        PICE_sprintf(tempCmd,"BP #%u set to %.4X:%.8X\n",0,segment,addr);
            }
            else
            {
		        PICE_sprintf(tempCmd,"BP #%u NOT set (either page not valid OR already used)\n",0);
            }
	        Print(OUTPUT_WINDOW,tempCmd);
        }
        else
        {
            if(InstallVirtualSWBreakpoint((LPSTR)pArgs->Value[0],(LPSTR)pArgs->Value[1]) )
            {
		        PICE_sprintf(tempCmd,"BP #%u virtually set to %s!%s\n",0,(LPSTR)pArgs->Value[0],(LPSTR)pArgs->Value[1]);
            }
            else
            {
		        PICE_sprintf(tempCmd,"BP #%u NOT set (maybe no symbols loaded)\n",0);
            }
	        Print(OUTPUT_WINDOW,tempCmd);
        }

		RepaintSource();

	}
	return TRUE;
}

//*************************************************************************
// ListBreakpoints()
//
//*************************************************************************
COMMAND_PROTOTYPE(ListBreakpoints)
{
	ULONG i;

    ListSWBreakpoints();

	for(i=0;i<4;i++)
	{
		if(Bp[i].Used)
		{
			PICE_sprintf(tempCmd,"(%u) %s %.4X:%.8X(linear %.8X)\n",i,Bp[i].Active?"*":" ",Bp[i].Segment,Bp[i].Offset,Bp[i].LinearAddress);
			Print(OUTPUT_WINDOW,tempCmd);
		}
	}
	return TRUE;
}

//*************************************************************************
// ClearBreakpoints()
//
//*************************************************************************
COMMAND_PROTOTYPE(ClearBreakpoints)
{
	if(pArgs->Count)
	{
		if(pArgs->Value[0]<4)
		{
			Bp[pArgs->Value[0]].Used=Bp[pArgs->Value[0]].Active=FALSE;
		}
		RepaintSource();
	}
	else
	{
    	ULONG i;

        RemoveAllSWBreakpoints(FALSE);

		for(i=0;i<4;i++)Bp[i].Used=Bp[i].Active=FALSE;
		RepaintSource();
	}
	return TRUE;
}

//*************************************************************************
// LeaveIce()
//
//*************************************************************************
COMMAND_PROTOTYPE(LeaveIce)
{
	//	SetHardwareBreakPoints();

	bSingleStep=FALSE;
	bNotifyToExit=TRUE;
	return TRUE;
}

//*************************************************************************
// ShowGdt()
//
//*************************************************************************
COMMAND_PROTOTYPE(ShowGdt)
{
	ULONG gdtr[2];
	USHORT i;
	PGDT pGdt;
	static ULONG addr=0;
	LPSTR pVerbose;

	// get GDT register
	__asm__ ("sgdt %0\n"
	 	:"=m" (gdtr));

    // info out
	PICE_sprintf(tempCmd,"Address=%.8X Limit=%.4X\n",(gdtr[1]<<16)|(gdtr[0]>>16),gdtr[0]&0xFFFF);
	Print(OUTPUT_WINDOW,tempCmd);
    WaitForKey();

	// make pointer to GDT
	pGdt=(PGDT)(((ULONG)(gdtr[1]<<16))|((ULONG)(gdtr[0]>>16)));
	if(pArgs->Count==1)
	{
	ULONG limit=((pGdt[addr].Limit_19_16<<16)|pGdt[addr].Limit_15_0);

		addr=pArgs->Value[0];
		addr&=(~0x7);
		if(pGdt[addr>>3].Gran)limit=(limit*4096)|0xfff;

		if(!pGdt[addr>>3].DescType)
			pVerbose = SystemSegmentTypes[pGdt[addr>>3].SegType];
		else
			pVerbose = NonSystemSegmentTypes[pGdt[addr>>3].SegType];

		PICE_sprintf(tempCmd,"%.4X %.8X %.8X %s %u %s\n",
						addr,
						(pGdt[addr>>3].Base_31_24<<24)|(pGdt[addr>>3].Base_23_16<<16)|(pGdt[addr>>3].Base_15_0),
						limit,
						pGdt[addr>>3].Present?" P":"NP",
						pGdt[addr>>3].Dpl,
						pVerbose);
		Print(OUTPUT_WINDOW,tempCmd);
	}
	else if(pArgs->Count==0)
	{
		for(i=0;i<((gdtr[0]&0xFFFF)>>3);i++)
		{
			ULONG limit=((pGdt[i].Limit_19_16<<16)|pGdt[i].Limit_15_0);

			if(!pGdt[i].DescType)
				pVerbose = SystemSegmentTypes[pGdt[i].SegType];
			else
				pVerbose = NonSystemSegmentTypes[pGdt[i].SegType];

			if(pGdt[i].Gran)limit=(limit*4096)|0xfff;

			PICE_sprintf(tempCmd,"%.4X %.8X %.8X %s %u %s\n",
							i<<3,
							(pGdt[i].Base_31_24<<24)|(pGdt[i].Base_23_16<<16)|(pGdt[i].Base_15_0),
							limit,
							pGdt[i].Present?" P":"NP",
							pGdt[i].Dpl,
							pVerbose);
			Print(OUTPUT_WINDOW,tempCmd);
			if(WaitForKey()==FALSE)break;
		}
	}
	return TRUE;
}

//*************************************************************************
// OutputIdtEntry()
//
//*************************************************************************
void OutputIdtEntry(PIDT pIdt,ULONG i)
{
    USHORT seg;
    ULONG offset;
    LPSTR pSym;

    seg = (USHORT)pIdt[i].Selector;
    offset = (pIdt[i].Offset_31_16<<16)|(pIdt[i].Offset_15_0);

    switch(pIdt[i].DescType)
	{
		// task gate
		case 0x5:
			PICE_sprintf(tempCmd,"(%0.4X) %0.4X:%0.8X %u [task]\n",i,
															seg,
															GetLinearAddress((USHORT)seg,0),
															pIdt[i].Dpl);
			break;
		// interrupt gate
		case 0x6:
		case 0xE:
			if(ScanExportsByAddress(&pSym,GetLinearAddress((USHORT)seg,offset)))
			{
				PICE_sprintf(tempCmd,"(%0.4X) %0.4X:%0.8X %u [int] (%s)\n",i,
																	seg,
																	offset,
																	pIdt[i].Dpl,
																	pSym);
			}
			else
			{
				PICE_sprintf(tempCmd,"(%0.4X) %0.4X:%0.8X %u [int]\n",i,
																	seg,
																	offset,
																	pIdt[i].Dpl);
			}
			break;
		// trap gate
		case 0x7:
		case 0xF:
			if(ScanExportsByAddress(&pSym,GetLinearAddress((USHORT)seg,offset)))
			{
				PICE_sprintf(tempCmd,"(%0.4X) %0.4X:%0.8X %u [trap] (%s)\n",i,
																	seg,
																	offset,
																	pIdt[i].Dpl,
																	pSym);
			}
			else
			{
				PICE_sprintf(tempCmd,"(%0.4X) %0.4X:%0.8X %u [trap]\n",i,
																	seg,
																	offset,
																	pIdt[i].Dpl);
			}
			break;
		default:
			PICE_sprintf(tempCmd,"(%0.4X) INVALID\n",i);
			break;
	}
	Print(OUTPUT_WINDOW,tempCmd);
}

//*************************************************************************
// ShowIdt()
//
//*************************************************************************
COMMAND_PROTOTYPE(ShowIdt)
{
	ULONG idtr[2];
	USHORT i;
	PIDT pIdt;
	ULONG addr=0;

	ENTER_FUNC();

	// get GDT register
	__asm__ ("sidt %0\n"
	 	:"=m" (idtr));
	// info out
	PICE_sprintf(tempCmd,"Address=%.8X Limit=%.4X\n",(idtr[1]<<16)|(idtr[0]>>16),idtr[0]&0xFFFF);
	Print(OUTPUT_WINDOW,tempCmd);
    WaitForKey();
	// make pointer to GDT
	pIdt=(PIDT)(((ULONG)(idtr[1]<<16))|((ULONG)(idtr[0]>>16)));
	if(pArgs->Count==1)
	{
		addr=pArgs->Value[0];
		addr&=(~0x7);

	}
	else if(pArgs->Count==0)
	{
		for(i=0;i<((idtr[0]&0xFFFF)>>3);i++)
		{
            OutputIdtEntry(pIdt,i);
			if(WaitForKey()==FALSE)break;
		}
	}
	LEAVE_FUNC();
	return TRUE;
}

//*************************************************************************
// ShowHelp()
//
//*************************************************************************
COMMAND_PROTOTYPE(ShowHelp)
{
	ULONG i,j;

    PutStatusText("COMMAND KEYWORD        DESCRIPTION");
    for(j=0;j<COMMAND_GROUP_LAST;j++)
    {
		PICE_sprintf(tempCmd,"= %-20s =====================================\n",CommandGroups[j]);
		Print(OUTPUT_WINDOW,tempCmd);
        WaitForKey();

	    for(i=0;CmdTable[i].Cmd!=NULL;i++)
	    {
            if(CmdTable[i].CommandGroup == j)
            {
		        PICE_sprintf(tempCmd,"%-20s   %s\n",CmdTable[i].Cmd,CmdTable[i].Help);
		        Print(OUTPUT_WINDOW,tempCmd);
		        if(WaitForKey()==FALSE)return TRUE;
            }
	    }
    }
	return TRUE;
}

//*************************************************************************
// ShowPageDirs()
//
//*************************************************************************
COMMAND_PROTOTYPE(ShowPageDirs)
{
	ULONG i;
	PPAGEDIR pPageDir;
	PULONG pPGD;
	PULONG pPTE;
	PEPROCESS my_current = IoGetCurrentProcess();

    ENTER_FUNC();

    DPRINT((0,"ShowPageDirs(): my_current = %.8X\n",(ULONG)my_current));

    // don't touch if not valid process
	if(my_current)
	{
        // no arguments supplied -> show all page directories
		if(!pArgs->Count)
		{
            PutStatusText("Linear            Physical Attributes");
            // there are 1024 page directories each mapping 1024*4k of address space
			for(i=0;i<1024;i++)
			{
                ULONG ulAddress = i<<22;
                // from the mm_struct get pointer to page directory for this address
                pPGD = ADDR_TO_PDE(ulAddress);
                // create a structurized pointer from PGD
                pPageDir = (PPAGEDIR)pPGD;

                if(pPageDir->PTBase)
                {

				    PICE_sprintf(tempCmd,"%.8X-%.8X %.8X %s %s %s\n",
							    ulAddress, ulAddress + 0x400000,
							    (pPageDir->PTBase<<12),
							    pPageDir->P?"P ":"NP",
							    pPageDir->RW?"RW":"R ",
							    pPageDir->US?"U":"S");
				    Print(OUTPUT_WINDOW,tempCmd);

    				if(WaitForKey()==FALSE)break;
                }
			}
		}

	  // one arg supplied -> show individual page
		else if(pArgs->Count == 1)
		{
            pPGD = ADDR_TO_PDE((ULONG)pArgs->Value[0]);

            DPRINT((0,"ShowPageDirs(): VA = %.8X\n",pArgs->Value[0]));
            DPRINT((0,"ShowPageDirs(): pPGD = %.8X\n",(ULONG)pPGD));

            if(pPGD && ((*pPGD)&_PAGE_PRESENT))
            {
                // 4M page
                if((*pPGD)&_PAGE_4M)
                {
                    PPAGEDIR pPage = (PPAGEDIR)pPGD;

                    PutStatusText("Linear    Physical   Attributes");

                    PICE_sprintf(tempCmd,"%.8X  %.8X     %s %s %s (LARGE PAGE PTE @ %.8X)\n",
						        pArgs->Value[0],
						        (pPage->PTBase<<12)|(pArgs->Value[0]&0x7FFFFF),
						        pPage->P?"P ":"NP",
						        pPage->RW?"RW":"R ",
						        pPage->US?"U":"S",
                                (ULONG)pPGD);
                }
                else
                {
                    pPTE = ADDR_TO_PTE(pArgs->Value[0]);
                    DPRINT((0,"ShowPageDirs(): pPTE = %.8X\n",(ULONG)pPTE));
                    if(pPTE)
                    {
                        PPAGEDIR pPage = (PPAGEDIR)pPTE;
                        DPRINT((0,"ShowPageDirs(): pPage->PTBase = %.8X\n",(ULONG)pPage->PTBase));

                        PutStatusText("Linear    Physical   Attributes");

                        PICE_sprintf(tempCmd,"%.8X  %.8X     %s %s %s (PTE @ %.8X)\n",
						            pArgs->Value[0],
						            (pPage->PTBase<<12)|(pArgs->Value[0]&(_PAGE_SIZE-1)),
						            (pPage->P==1)?"P ":"NP",
						            pPage->RW?"RW":"R ",
						            pPage->US?"U":"S",
                                    (ULONG)pPTE);
                    }

                }
    			Print(OUTPUT_WINDOW,tempCmd);
            }
            else
            {
                PICE_sprintf(tempCmd,"page at %.8X not present.\n",pArgs->Value[0]);
    			Print(OUTPUT_WINDOW,tempCmd);
            }

		}
	}
	return TRUE;
}

//*************************************************************************
// ShowProcesses()
//
//*************************************************************************
COMMAND_PROTOTYPE(ShowProcesses)
{
	PEPROCESS my_current = IoGetCurrentProcess();
    PLIST_ENTRY current_entry;
	PEPROCESS currentps;

	ENTER_FUNC();

	current_entry = pPsProcessListHead->Flink;

	if( current_entry ){

	    PutStatusText("NAME             TASK         PID");

		while( current_entry != pPsProcessListHead ){
				currentps = CONTAINING_RECORD(current_entry,
					  EPROCESS,
					  ProcessListEntry);
			    DPRINT((0,"currentps = %x\n",currentps));
				//ei would be nice to mark current process!
				PICE_sprintf(tempCmd,"%-16.16s %-12x %x\n",currentps->ImageFileName,
						(ULONG)currentps,currentps->UniqueProcessId);
				Print(OUTPUT_WINDOW,tempCmd);
				if(WaitForKey()==FALSE)
					break;
				current_entry = current_entry->Flink;
		}
	}
    LEAVE_FUNC();
	return TRUE;
}

//*************************************************************************
// DisplayMemoryDword()
//
//*************************************************************************
COMMAND_PROTOTYPE(DisplayMemoryDword)
{
    ULONG i,j,k;
    static ULONG addr=0,addrorg;
    static USHORT segment;
    char temp[8];
    LPSTR pSymbolName;

	ENTER_FUNC();
	DPRINT((0,"DisplayMemoryDword()\n"));
	if(pArgs->Count==2)
	{
		segment=(USHORT)pArgs->Value[0];
		if(!segment)segment=GLOBAL_DATA_SEGMENT;
		addr=pArgs->Value[1];
        OldSelector = segment;
        OldOffset = addr;
		addrorg=addr;
		addr=GetLinearAddress(segment,addr);
	}
	else if(pArgs->Count==1)
	{
		segment=CurrentDS;
		addr=pArgs->Value[0];
        OldOffset = addr;
		addrorg=addr;
		addr=GetLinearAddress(segment,addr);
	}
	else if(pArgs->Count==0)
    {
        addr += sizeof(ULONG)*4*4;
        OldOffset = addr;
    }

    if(ScanExportsByAddress(&pSymbolName,addr))
    {
		PICE_sprintf(tempCmd," %s ",pSymbolName);
		SetForegroundColor(COLOR_TEXT);
		SetBackgroundColor(COLOR_CAPTION);
		PutChar(tempCmd,GLOBAL_SCREEN_WIDTH-1-PICE_strlen(tempCmd),wWindow[DATA_WINDOW].y-1);
        ResetColor();
    }

	DisableScroll(DATA_WINDOW);

    if(DisplayMemory != DisplayMemoryDword)
    {
        Clear(DATA_WINDOW);
        DisplayMemory = DisplayMemoryDword;
    }
    else
	    Home(DATA_WINDOW);

	for(k=0;k<wWindow[DATA_WINDOW].cy;k++) // 4 lines
	{
		PICE_sprintf(tempCmd,"%.4X:%.8X: ",segment,addrorg+k*16);
		Print(1,tempCmd);
		for(i=0;i<4;i++) // 4 dwords
		{
			tempCmd[0]=0;
			Print(1," ");
			for(j=0;j<4;j++) // 1 dword = 4 bytes
			{
				if(IsAddressValid(addr+i*4+j+k*16))
				{
					PICE_sprintf(temp,"%.2x",*(PUCHAR)(addr+i*4+j+k*16));
					PICE_strrev(temp);
					PICE_strcat(tempCmd,temp);
				}
				else
				{
					PICE_strcat(tempCmd,"??");
				}
			}
			PICE_strrev(tempCmd);
			Print(1,tempCmd);
		}
		Print(1,"                     ");
		tempCmd[0]=0;
		for(j=0;j<16;j++) // 1 dword = 4 bytes
		{
            wWindow[DATA_WINDOW].usCurX = GLOBAL_SCREEN_WIDTH-17;
			if(IsAddressValid(addr+j+k*16))
			{
				PICE_sprintf(temp,"%c",PICE_isprint(*(PUCHAR)(addr+j+k*16))?(*(PUCHAR)(addr+j+k*16)):'.');
				PICE_strcat(tempCmd,temp);
			}
			else
			{
				PICE_strcat(tempCmd,"?");
			}
		}
		Print(1,tempCmd);
		Print(1,"\n");
	}
	EnableScroll(DATA_WINDOW);
	addr+=16*4;
	return TRUE;
}

//*************************************************************************
// DisplayMemoryByte()
//
//*************************************************************************
COMMAND_PROTOTYPE(DisplayMemoryByte)
{
    ULONG j,k;
    static ULONG addr=0,addrorg;
    static USHORT segment;
    char temp[8];
    LPSTR pSymbolName;

	if(pArgs->Count==2)
	{
		segment=(USHORT)pArgs->Value[0];
		if(!segment)segment=GLOBAL_DATA_SEGMENT;
		addr=pArgs->Value[1];
        OldSelector = segment;
        OldOffset = addr;
		addrorg=addr;
		addr=GetLinearAddress(segment,addr);
	}
	else if(pArgs->Count==1)
	{
		segment=CurrentDS;
		addr=pArgs->Value[0];
        OldOffset = addr;
		addrorg=addr;
		addr=GetLinearAddress(segment,addr);
	}
	else if(pArgs->Count==0)
    {
        addr += sizeof(ULONG)*4*4;
        OldOffset = addr;
    }

    if(DisplayMemory != DisplayMemoryByte)
    {
        Clear(DATA_WINDOW);
        DisplayMemory = DisplayMemoryByte;
    }
    else
	    Home(DATA_WINDOW);

    if(ScanExportsByAddress(&pSymbolName,addr))
    {
		PICE_sprintf(tempCmd," %s ",pSymbolName);
		SetForegroundColor(COLOR_TEXT);
		SetBackgroundColor(COLOR_CAPTION);
		PutChar(tempCmd,GLOBAL_SCREEN_WIDTH-1-PICE_strlen(tempCmd),wWindow[DATA_WINDOW].y-1);
        ResetColor();
    }

    DisableScroll(DATA_WINDOW);
	for(k=0;k<wWindow[DATA_WINDOW].cy;k++) // 4 lines
	{
		PICE_sprintf(tempCmd,"%.4X:%.8X: ",segment,addrorg+k*16);
		Print(1,tempCmd);
		tempCmd[0]=0;
		Print(1," ");
		for(j=0;j<16;j++) // 1 dword = 4 bytes
		{
			if(IsAddressValid(addr+j+k*16))
			{
				PICE_sprintf(temp,"%.2x ",*(PUCHAR)(addr+j+k*16));
				PICE_strcat(tempCmd,temp);
			}
			else
			{
				PICE_strcat(tempCmd,"?? ");
			}
		}
		Print(1,tempCmd);
		Print(1,"  ");
		tempCmd[0]=0;
		for(j=0;j<16;j++) // 1 dword = 4 bytes
		{
            wWindow[DATA_WINDOW].usCurX = GLOBAL_SCREEN_WIDTH-17;
			if(IsAddressValid(addr+j+k*16))
			{
				PICE_sprintf(temp,"%c",PICE_isprint(*(PUCHAR)(addr+j+k*16))?(*(PUCHAR)(addr+j+k*16)):'.');
				PICE_strcat(tempCmd,temp);
			}
			else
			{
				PICE_strcat(tempCmd,"?");
			}
		}
		Print(1,tempCmd);
		Print(1,"\n");
	}
	EnableScroll(DATA_WINDOW);
	addr+=16*4;
	return TRUE;
}

//*************************************************************************
// DisplayPhysMemDword()
//
//*************************************************************************
COMMAND_PROTOTYPE(DisplayPhysMemDword)
{
    ULONG i,j,k;
    static ULONG addr=0,addrorg;
    static USHORT segment;
    char temp[8];

	ENTER_FUNC();
	DPRINT((0,"DisplayPhysMemDword()\n"));

	if(pArgs->Count==1)
	{
		segment=CurrentDS;
		addr=pArgs->Value[0];
        OldOffset = addr;
		addrorg=addr;
		addr=GetLinearAddress(segment,addr);
	}
	else if(pArgs->Count==0)
    {
        addr += sizeof(ULONG)*4*4;
        OldOffset = addr;
    }

	DisableScroll(DATA_WINDOW);

    if(DisplayMemory != DisplayPhysMemDword)
    {
        Clear(DATA_WINDOW);
        DisplayMemory = DisplayPhysMemDword;
    }
    else
	    Home(DATA_WINDOW);

	for(k=0;k<wWindow[DATA_WINDOW].cy;k++) // 4 lines
	{
		PICE_sprintf(tempCmd,"PHYS:%.8X: ",addrorg+k*16);
		Print(1,tempCmd);
		for(i=0;i<4;i++) // 4 dwords
		{
			tempCmd[0]=0;
            PICE_sprintf(tempCmd," %.8X",ReadPhysMem(addr+i*4+k*16,sizeof(ULONG)));
			Print(1,tempCmd);
		}
		Print(1,"                     ");
		tempCmd[0]=0;
		for(j=0;j<16;j++) // 1 dword = 4 bytes
		{
            UCHAR ucData;
            wWindow[DATA_WINDOW].usCurX = GLOBAL_SCREEN_WIDTH-17;
            ucData = ReadPhysMem(addr+j+k*16,sizeof(UCHAR));
			PICE_sprintf(temp,"%c",PICE_isprint(ucData)?ucData:'.');
			PICE_strcat(tempCmd,temp);
		}
		Print(1,tempCmd);
		Print(1,"\n");
	}
	EnableScroll(DATA_WINDOW);
	addr+=16*4;
	return TRUE;
}


//*************************************************************************
// DisplaySourceFile()
//
//*************************************************************************
void DisplaySourceFile(LPSTR pSrcLine,LPSTR pSrcEnd,ULONG ulLineNumber,ULONG ulLineNumberToInvert)
{
    ULONG i;
    LPSTR pTemp;
    ULONG j = ulLineNumber-1;

    DPRINT((0,"DisplaySourceFile(%.8X,%u,%u)\n",pSrcLine,ulLineNumber,ulLineNumberToInvert));

    // go to line
    while(j--)
    {
        // goto end of current line
        while(*pSrcLine!=0x0a && *pSrcLine!=0x0d)
            pSrcLine++;

        // skip over the line end
        if(*pSrcLine == 0x0d)
            pSrcLine++;
        if(*pSrcLine == 0x0a)
            pSrcLine++;
    }

    Clear(SOURCE_WINDOW);
    DisableScroll(SOURCE_WINDOW);
    for(i=0;i<wWindow[SOURCE_WINDOW].cy;i++)
    {
        pTemp = tempCmd;

        if(pSrcLine<pSrcEnd)
        {
            PICE_sprintf(tempCmd,".%.5u ",ulLineNumber+i);
            pTemp = tempCmd + PICE_strlen(tempCmd);

            while(pSrcLine<pSrcEnd && *pSrcLine!=0x0a && *pSrcLine!=0x0d)
            {
                if(*pSrcLine==0x9) // TAB
                {
                    *pTemp++ = 0x20;
                    *pTemp++ = 0x20;
                    *pTemp++ = 0x20;
                    *pTemp++ = 0x20;
                    pSrcLine++;
                }
                else
                {
                    *pTemp++ = *pSrcLine++;
                }
            }

            if(pSrcLine<pSrcEnd)
            {
                // skip over the line end
                if(*pSrcLine == 0x0d)
                    pSrcLine++;
                if(*pSrcLine == 0x0a)
                    pSrcLine++;
            }

            *pTemp++ = '\n';
            *pTemp = 0;

            if(PICE_strlen(tempCmd)>GLOBAL_SCREEN_WIDTH-1)
            {
                tempCmd[GLOBAL_SCREEN_WIDTH-2]='\n';
                tempCmd[GLOBAL_SCREEN_WIDTH-1]=0;
            }

            if( (ulLineNumberToInvert!=-1) &&
               ((int)(ulLineNumberToInvert-ulLineNumber)>=0) &&
               ((ulLineNumberToInvert-ulLineNumber)<wWindow[SOURCE_WINDOW].cy) &&
               (i==(ulLineNumberToInvert-ulLineNumber)) )
            {
                SetForegroundColor(COLOR_BACKGROUND);
                SetBackgroundColor(COLOR_FOREGROUND);
            }

            Print(SOURCE_WINDOW,tempCmd);

            if( (ulLineNumberToInvert!=-1) &&
               ((int)(ulLineNumberToInvert-ulLineNumber)>=0) &&
               ((ulLineNumberToInvert-ulLineNumber)<wWindow[SOURCE_WINDOW].cy) &&
               (i==(ulLineNumberToInvert-ulLineNumber)) )
            {
                ResetColor();
            }

        }
        else
        {
            Print(SOURCE_WINDOW,"---- End of source file --------------\n");
            break;
        }
    }
    EnableScroll(SOURCE_WINDOW);
}

//*************************************************************************
// UnassembleOneLineDown()
//
//*************************************************************************
void UnassembleOneLineDown(void)
{
    ULONG addr,addrorg;

    DPRINT((0,"UnassembleOneLineDown()\n"));

    addrorg = addr = GetLinearAddress(usOldDisasmSegment,ulOldDisasmOffset);

    DPRINT((0,"UnassembleOneLineDown(): addr = %.8X\n",addr));

    tempCmd[0]=0;
	Disasm(&addr,tempCmd);

    DPRINT((0,"UnassembleOneLineDown(): addr after = %.8X\n",addr));

    ulOldDisasmOffset += (addr - addrorg);
    RepaintSource();
}

//*************************************************************************
// UnassembleOnePageDown()
//
//*************************************************************************
void UnassembleOnePageDown(ULONG page)
{
    ULONG addr,addrorg,i;

    DPRINT((0,"UnassembleOnePageDown()\n"));

    addrorg = addr = GetLinearAddress(usOldDisasmSegment,ulOldDisasmOffset);

    DPRINT((0,"UnassembleOnePageDown(): addr = %.8X\n",addr));

    tempCmd[0]=0;
    for(i=0;i<page;i++)
	    Disasm(&addr,tempCmd);

    DPRINT((0,"UnassembleOnePageDown(): addr after = %.8X\n",addr));

    ulOldDisasmOffset += (addr - addrorg);
    RepaintSource();
}

//*************************************************************************
// UnassembleOneLineUp()
//
//*************************************************************************
void UnassembleOneLineUp(void)
{
    ULONG addr,addrorg,addrbefore,start,end,addrstart;
    LONG offset;
    LPSTR pSymbol;

    DPRINT((0,"UnassembleOneLineUp()\n"));

    addrorg = addr = GetLinearAddress(usOldDisasmSegment,ulOldDisasmOffset);

    DPRINT((0,"UnassembleOneLineUp(): addrorg = %.8X\n",addr));

    offset = 1;

    if((pSymbol = FindFunctionByAddress(addrorg-offset,&start,&end)) )
    {
        offset = addrorg - start;
        DPRINT((0,"UnassembleOneLineUp(): %s @ offset = %u\n",pSymbol,offset));
    }
    else
    {
        // max instruction length is 15 bytes
        offset = 15;
    }

    addrstart = addrorg;

    // start at current address less offset
    addr = addrorg - offset;
    do
    {
        DPRINT((0,"UnassembleOneLineUp(): offset = %u addrorg %x addr %x\n",offset,addrorg,addr));
        // disassemble while not reaching current instruction
        addrbefore = addr;
        tempCmd[0]=0;
	    Disasm(&addr,tempCmd);
        DPRINT((0,"%.8X: %s\n",addrbefore,tempCmd));
    }while((addr != addrorg) && (addrbefore < addrorg));

    if((addrorg - addrstart)<=0)
        ulOldDisasmOffset--;
    else
        ulOldDisasmOffset -= (addrorg - addrbefore);

    DPRINT((0,"UnassembleOneLineUp(): new addr = %.4X:%.8X\n",usOldDisasmSegment,ulOldDisasmOffset));

    RepaintSource();
}

//*************************************************************************
// UnassembleOneLineUp()
//
//*************************************************************************
void UnassembleOnePageUp(ULONG page)
{
    ULONG addr,addrorg,addrbefore,start,end,i,addrstart;
    LONG offset;
    LPSTR pSymbol;

    DPRINT((0,"UnassembleOnePageUp()\n"));

    for(i=0;i<page;i++)
    {
        addrorg = addr = GetLinearAddress(usOldDisasmSegment,ulOldDisasmOffset);

        DPRINT((0,"UnassembleOnePageUp(): addrorg = %.8X\n",addr));

        offset = 1;

        if((pSymbol = FindFunctionByAddress(addrorg-offset,&start,&end)) )
        {
            offset = addrorg - start;
            DPRINT((0,"UnassembleOnePageUp(): %s @ offset = %u\n",pSymbol,offset));
        }
        else
        {
            // max instruction length is 15 bytes
            offset = 15;
        }

        // start at current address less offset
        addr = addrorg - offset;
        addrstart = addrorg;
        do
        {
            DPRINT((0,"UnassembleOnePageUp(): offset = %u addrorg %x addr %x\n",offset,addrorg,addr));
            addrbefore = addr;
            // disassemble while not reaching current instruction
            tempCmd[0]=0;
	        Disasm(&addr,tempCmd);
            DPRINT((0,"%.8X: %s\n",addrbefore,tempCmd));
        }while((addr != addrorg) && (addrbefore < addrorg));

        if((addrorg - addrstart)<=0)
            ulOldDisasmOffset--;
        else
            ulOldDisasmOffset -= (addrorg - addrbefore);

    }

    DPRINT((0,"UnassembleOnePageUp(): new addr = %.4X:%.8X\n",usOldDisasmSegment,ulOldDisasmOffset));

    RepaintSource();
}

//*************************************************************************
// Unassemble()
//
//*************************************************************************
COMMAND_PROTOTYPE(Unassemble)
{
    ULONG i;
    ULONG addr=0,addrorg,addrstart,ulLineNumber;
    USHORT segment=0;
    ULONG addrbefore;
    LPSTR pSymbolName;
	BOOLEAN bSWBpAtAddr;
    LPSTR pSrc,pFilename,pSrcStart,pSrcEnd;
    BOOLEAN bForceDisassembly = FALSE;

    if(pArgs->CountSwitches>1)
        return TRUE;

    if(pArgs->CountSwitches==1)
    {
        if(pArgs->Switch[0] == 'f')
            bForceDisassembly = TRUE;
    }

	// we have args
	if(pArgs->Count==2)
	{
		addr=pArgs->Value[1];
		segment=(USHORT)pArgs->Value[0];
		addrorg=addrstart=addr;
		addr=GetLinearAddress(segment,addr);

        usOldDisasmSegment = segment;
        ulOldDisasmOffset = addr;
	}
	else if(pArgs->Count==1)
	{
		addr=pArgs->Value[0];
		segment=CurrentCS;
		addrorg=addrstart=addr;
		addr=GetLinearAddress(segment,addr);

        usOldDisasmSegment = segment;
        ulOldDisasmOffset = addr;
	}
	else if(pArgs->Count==0)
    {
        segment = usOldDisasmSegment;
		addrorg=addrstart=addr;
        addr = GetLinearAddress(usOldDisasmSegment,ulOldDisasmOffset);
    }
    else
        return TRUE;


	DPRINT((0,"Unassemble(%0.4X:%0.8X)\n",segment,addr));

    //
	// unassemble
    //
    DisableScroll(SOURCE_WINDOW);

    // if we're inside last disassembly range we only need to move to highlight
    if(addr>=ulLastDisassStartAddress &&
       addr<ulLastDisassEndAddress )
	{
		addr=ulLastDisassStartAddress;
	}
	else
	{
		ulLastDisassStartAddress=ulLastDisassEndAddress=0;
	}

    SetForegroundColor(COLOR_TEXT);
	SetBackgroundColor(COLOR_CAPTION);

	ClrLine(wWindow[SOURCE_WINDOW].y-1);

    ResetColor();

    if(ScanExportsByAddress(&pSymbolName,addr))
    {
        SetForegroundColor(COLOR_TEXT);
	    SetBackgroundColor(COLOR_CAPTION);
		PICE_sprintf(tempCmd," %s ",pSymbolName);
		PutChar(tempCmd,GLOBAL_SCREEN_WIDTH-1-PICE_strlen(tempCmd),wWindow[SOURCE_WINDOW].y-1);
        ResetColor();
    }

    pCurrentMod = FindModuleFromAddress(addr);
    if(pCurrentMod)
    {
        ULONG mod_addr;
	    DPRINT((0,"Unassemble(): pCurrentMod->name = %S\n",pCurrentMod->name));
        mod_addr = (ULONG)pCurrentMod->BaseAddress;

        pCurrentSymbols = FindModuleSymbols(mod_addr);
   	    DPRINT((0,"Unassemble(): pCurrentSymbols = %x\n",(ULONG)pCurrentSymbols));
    }
	DPRINT((0,"Unassemble(): pCurrentMod = %x, showsrc: %d\n",pCurrentMod, bShowSrc));

    ulCurrentlyDisplayedLineNumber = 0;

    if(bShowSrc && bForceDisassembly == FALSE && (pSrc = FindSourceLineForAddress(addr,&ulLineNumber,&pSrcStart,&pSrcEnd,&pFilename)) )
    {
		DPRINT((0,"FoundSourceLineForAddress: file: %s line: %d\n", pFilename, ulLineNumber));
        PICE_strcpy(szCurrentFile,pFilename);

        ulCurrentlyDisplayedLineNumber = ulLineNumber;

    	Clear(SOURCE_WINDOW);

        // display file name
        SetForegroundColor(COLOR_TEXT);
	    SetBackgroundColor(COLOR_CAPTION);

        if(PICE_strlen(pFilename)<GLOBAL_SCREEN_WIDTH/2)
        {
		    PutChar(pFilename,1,wWindow[SOURCE_WINDOW].y-1);
        }
        else
        {
            LPSTR p;

            p = strrchr(pFilename,'/');
            if(!p)
            {
                p = pFilename;
            }
            else
            {
                p++;
            }

		    PutChar(p,1,wWindow[SOURCE_WINDOW].y-1);
        }

        ResetColor();

        // display the source
        if(ulLineNumber>(wWindow[SOURCE_WINDOW].cy/2) )
        {
            DisplaySourceFile(pSrcStart,pSrcEnd,ulLineNumber-(wWindow[SOURCE_WINDOW].cy/2),ulLineNumber);
        }
        else
        {
            DisplaySourceFile(pSrcStart,pSrcEnd,ulLineNumber,ulLineNumber);
        }
    }
    else
    {
        *szCurrentFile = 0;
		DPRINT((0,"Couldn't find source for file\n"));
        Home(SOURCE_WINDOW);
        // for each line in the disassembly window
	    for(i=0;i<wWindow[SOURCE_WINDOW].cy;i++)
	    {
            extern ULONG ulWindowOffset;

		    bSWBpAtAddr = FALSE;
		    // if there is a potential SW breakpoint at address
		    // we might have to put back the original opcode
		    // in order to disassemble correctly.
		    if(IsSwBpAtAddress(addr))
		    {
			    // if INT3 is there, remove it while disassembling
			    if((bSWBpAtAddr = IsSwBpAtAddressInstalled(addr)))
			    {
				    DeInstallSWBreakpoint(addr);
			    }
		    }

		    ClrLine(wWindow[SOURCE_WINDOW].y+i);

		    // invert the line that we're about to execute
		    if(addr==CurrentEIP)
		    {
                SetForegroundColor(COLOR_BACKGROUND);
                SetBackgroundColor(COLOR_FOREGROUND);
			    ulLastInvertedAddress = CurrentEIP;
		    }

		    // output segment:offset address
		    PICE_sprintf(tempCmd,"%0.4X:%0.8X ",segment,addr);
		    Print(SOURCE_WINDOW,tempCmd);

            // disassemble a line
		    addrbefore=addr;
            if(bCodeOn)
            {
                tempCmd[30]=0;
        		Disasm(&addr,&tempCmd[30]);
            }
            else
            {
                tempCmd[0]=0;
        		Disasm(&addr,tempCmd);
            }
		    addrorg+=(addr-addrbefore);

            // want to display opcode bytes
		    if(bCodeOn)
		    {
			    ULONG j;

			    for(j=0;j<15;j++)
			    {
                    if(j<addr-addrbefore)
                    {
				        if(IsAddressValid(addrbefore+j))
				        {
					        tempCmd[j*2]=HexDigit[((*(PUCHAR)(addrbefore+j)&0xF0)>>4)];
					        tempCmd[j*2+1]=HexDigit[((*(PUCHAR)(addrbefore+j)&0xF))];
				        }
				        else
				        {
					        tempCmd[j*2]='?';
					        tempCmd[j*2+1]='?';
				        }
                    }
                    else
                    {
					        tempCmd[j*2]=' ';
					        tempCmd[j*2+1]=' ';
                    }
			    }
		    }
            PICE_strcat(tempCmd,"\n");

            if(ulWindowOffset)
            {
                LONG len = PICE_strlen(tempCmd);
                if(ulWindowOffset < len)
                    PICE_memcpy(tempCmd,&tempCmd[ulWindowOffset],len-ulWindowOffset);
                else
                    tempCmd[0]='\n';
            }

		    Print(SOURCE_WINDOW,tempCmd);

            if(addrbefore==CurrentEIP)
		    {
			    ResetColor();
		    }

		    // if potential SW breakpoint, undo marked text
		    if(IsSwBpAtAddress(addrbefore))
		    {
				HatchLine(wWindow[SOURCE_WINDOW].y+i);
		    }

		    // if breakpoint was installed before disassembly, put it back
		    if(bSWBpAtAddr)
		    {
			    ReInstallSWBreakpoint(addrbefore);
		    }

	    }

        if(ulLastDisassStartAddress==0 && ulLastDisassEndAddress==0)
        {
            ulLastDisassStartAddress=addrstart;
            ulLastDisassEndAddress=addr;
        }

	    if(!IsAddressValid(addrstart))
	    {
            ulLastDisassStartAddress=0;
            ulLastDisassEndAddress=0;
	    }

    }

	EnableScroll(SOURCE_WINDOW);

	return TRUE;
}

//*************************************************************************
// ShowModules()
//
//*************************************************************************
COMMAND_PROTOTYPE(ShowModules)
{
	PDEBUG_MODULE pMod;
	DPRINT((0,"ShowModules()\n"));

	if(BuildModuleList())
    {
        pMod = pdebug_module_head;
        do
        {
			if(pMod->size)
			{
                if(pMod == pCurrentMod)
                {
				    PICE_sprintf(tempCmd,"%.8X - %.8X *%-32S\n",
						    (unsigned int)pMod->BaseAddress,
						    (unsigned int) ((unsigned int)pMod->BaseAddress+pMod->size),pMod->name);
                }
                else
                {
				    PICE_sprintf(tempCmd,"%.8X - %.8X  %-32S\n",
						    (unsigned int)pMod->BaseAddress,
						    (unsigned int) ((unsigned int)pMod->BaseAddress+pMod->size),
							pMod->name);
                }
			}
            Print(OUTPUT_WINDOW,tempCmd);
			if(WaitForKey()==FALSE)
				break;
        }while((pMod = pMod->next)!=pdebug_module_tail);
    }
	return TRUE;
}

//*************************************************************************
// DecodeVmFlags()
//
//*************************************************************************
//ei FIX THIS!!!!!!!!!!!!!!!!!!
LPSTR DecodeVmFlags(ULONG flags)
{
    ULONG i;
/*
#define VM_READ		0x0001
#define VM_WRITE	0x0002
#define VM_EXEC		0x0004
#define VM_SHARED	0x0008

#define VM_MAYREAD	0x0010
#define VM_MAYWRITE	0x0020
#define VM_MAYEXEC	0x0040
#define VM_MAYSHARE	0x0080

#define VM_GROWSDOWN	0x0100
#define VM_GROWSUP	0x0200
#define VM_SHM		0x0400
#define VM_DENYWRITE	0x0800

#define VM_EXECUTABLE	0x1000
#define VM_LOCKED	0x2000
#define VM_IO           0x4000

#define VM_STACK_FLAGS	0x0177
*/
    static LPSTR flags_syms_on[]={"R","W","X","S","MR","MW","MX","MS","GD","GU","SHM","exe","LOCK","IO",""};
    static char temp[256];

	// terminate string
    *temp = 0;
//ei fix fix fix
#if 0

    if(flags == VM_STACK_FLAGS)
    {
        PICE_strcpy(temp," (STACK)");
    }
    else
    {
        for(i=0;i<15;i++)
        {
            if(flags&0x1)
            {
                PICE_strcat(temp," ");
                PICE_strcat(temp,flags_syms_on[i]);
            }
            flags >>= 1;
        }
    }
#endif
    return temp;
}

//*************************************************************************
// ShowVirtualMemory()
//
//*************************************************************************
COMMAND_PROTOTYPE(ShowVirtualMemory)
{
	PEPROCESS my_current = IoGetCurrentProcess();
    PLIST_ENTRY current_entry;
	PMADDRESS_SPACE vma = NULL;
	MEMORY_AREA* current;
    char filename[64];

	DPRINT((0,"ShowVirtualMemory()\n"));
	if( my_current )
    	vma = &(my_current->AddressSpace);

    if(vma)
    {
        if(pArgs->Count == 0)
        {
            PutStatusText("START    END   LENGTH   VMA      TYPE   ATTR");
			current_entry = vma->MAreaListHead.Flink;
			while (current_entry != &vma->MAreaListHead)
            {
                *filename = 0;

				current = CONTAINING_RECORD(current_entry,
							    MEMORY_AREA,
							    Entry);
				// find the filename
                if(((current->Type == MEMORY_AREA_SECTION_VIEW_COMMIT) ||
					(current->Type == MEMORY_AREA_SECTION_VIEW_RESERVE) )&&
                    current->Data.SectionData.Section->FileObject)
                {
                    if(IsAddressValid((ULONG)current->Data.SectionData.Section->FileObject->FileName.Buffer) )
                        PICE_sprintf(filename,"%.64S",current->Data.SectionData.Section->FileObject->FileName.Buffer);
                }

                PICE_sprintf(tempCmd,"%.8X %.8X %.8X %.8X %x %x    %s\n",
                        (ULONG)current->BaseAddress,
                        (ULONG)current->BaseAddress+current->Length,
						current->Length,
                        (ULONG)current,
                        current->Type, current->Attributes,//DecodeVmFlags(current->Type, current->Attributes),
                        filename);
                Print(OUTPUT_WINDOW,tempCmd);

                if(WaitForKey()==FALSE)break;
				current_entry = current_entry->Flink;
            }
        }
    }

    return TRUE;
}

//*************************************************************************
// Ver()
//
//*************************************************************************
COMMAND_PROTOTYPE(Ver)
{
	//ei add kernel version info??!!
    PICE_sprintf(tempCmd,"pICE: version %u.%u (build %u) for Reactos\n",
                 PICE_MAJOR_VERSION,
                 PICE_MINOR_VERSION,
                 PICE_BUILD);

	Print(OUTPUT_WINDOW,tempCmd);

/*    PICE_sprintf(tempCmd,"pICE: loaded on %s kernel release %s\n",
		system_utsname.sysname,
		system_utsname.release);
 */
	Print(OUTPUT_WINDOW,tempCmd);
	Print(OUTPUT_WINDOW,"pICE: written by Klaus P. Gerlicher and Goran Devic.\n");
	Print(OUTPUT_WINDOW,"pICE: ported to Reactos by Eugene Ingerman.\n");

	return TRUE;
}

//*************************************************************************
// Hboot()
//
//*************************************************************************
COMMAND_PROTOTYPE(Hboot)
{
	// nudge the reset line through keyboard controller
	__asm__("
		movb $0xFE,%al
		outb %al,$0x64");
	// never gets here
	return TRUE;
}

//*************************************************************************
// SetSrcDisplay()
//
//*************************************************************************
COMMAND_PROTOTYPE(SetSrcDisplay)
{
    ARGS Args;

	if(pArgs->Count==0)
	{
		bShowSrc=bShowSrc?FALSE:TRUE;
        PICE_memset(&Args,0,sizeof(ARGS));
        // make unassembler refresh all again
        ulLastDisassStartAddress=ulLastDisassEndAddress=0;
		Args.Count=0;
		Unassemble(&Args);
	}
	return TRUE;
}

//*************************************************************************
// I3here()
//
//*************************************************************************
COMMAND_PROTOTYPE(I3here)
{
	if(pArgs->Count==1)
	{
		if(pArgs->Value[0]==1)
		{
			if(!bInt3Here)
			{
				bInt3Here=TRUE;
				Print(OUTPUT_WINDOW,"I3HERE is now ON\n");
			}
			else
				Print(OUTPUT_WINDOW,"I3HERE is already ON\n");
		}
		else if(pArgs->Value[0]==0)
		{
			if(bInt3Here)
			{
				bInt3Here=FALSE;
				Print(OUTPUT_WINDOW,"I3HERE is now OFF\n");
			}
			else
				Print(OUTPUT_WINDOW,"I3HERE is already OFF\n");
		}
	}
	else if(pArgs->Count==0)
	{
		if(bInt3Here)
		{
			Print(OUTPUT_WINDOW,"I3HERE is ON\n");
		}
		else
		{
			Print(OUTPUT_WINDOW,"I3HERE is OFF\n");
		}
	}
	// never gets here
	return TRUE;
}

COMMAND_PROTOTYPE(I1here)
{
	if(pArgs->Count==1)
	{
		if(pArgs->Value[0]==1)
		{
			if(!bInt1Here)
			{
				bInt1Here=TRUE;
				Print(OUTPUT_WINDOW,"I1HERE is now ON\n");
			}
			else
				Print(OUTPUT_WINDOW,"I1HERE is already ON\n");
		}
		else if(pArgs->Value[0]==0)
		{
			if(bInt1Here)
			{
				bInt1Here=FALSE;
				Print(OUTPUT_WINDOW,"I1HERE is now OFF\n");
			}
			else
				Print(OUTPUT_WINDOW,"I1HERE is already OFF\n");
		}
	}
	else if(pArgs->Count==0)
	{
		if(bInt1Here)
		{
			Print(OUTPUT_WINDOW,"I1HERE is ON\n");
		}
		else
		{
			Print(OUTPUT_WINDOW,"I1HERE is OFF\n");
		}
	}
	// never gets here
	return TRUE;
}

COMMAND_PROTOTYPE(NextInstr)
{
    static char tempDisasm[256];
    ULONG addr,addrbefore;

	bNeedToFillBuffer=FALSE;

	if(!pArgs->Count)
	{
		addr=addrbefore=GetLinearAddress(CurrentCS,CurrentEIP);
		DPRINT((0,"addr before %.8X\n",addrbefore));
		Disasm(&addr,tempDisasm);
		DPRINT((0,"addr after %.8X\n",addr));
		CurrentEIP=CurrentEIP+(addr-addrbefore);
		// display register contents
		DisplayRegs();
		// unassemble
		DPRINT((0,"new CS:EIP %04.x:%.8X\n",CurrentCS,CurrentEIP));
        PICE_memset(pArgs,0,sizeof(ARGS));
        // make unassembler refresh all again
        ulLastDisassStartAddress=ulLastDisassEndAddress=0;
		pArgs->Count=2;
		pArgs->Value[0]=(ULONG)CurrentCS;
		pArgs->Value[1]=CurrentEIP;
		Unassemble(pArgs);
	}
	bNeedToFillBuffer=TRUE;
	return TRUE;
}

COMMAND_PROTOTYPE(SetGetRegisters)
{
	ULONG i;

	if(pArgs->Count==0)
	{
		// display whole set
		for(i=0;RegKeyWords[i].pValue!=0;i++)
		{
			switch(RegKeyWords[i].ulSize)
			{
				case 1:
					PICE_sprintf(tempCmd,"%s = %.8X\n",RegKeyWords[i].KeyWord,*(PUCHAR)(RegKeyWords[i].pValue));
					break;
				case 2:
					PICE_sprintf(tempCmd,"%s = %.8X\n",RegKeyWords[i].KeyWord,*(PUSHORT)(RegKeyWords[i].pValue));
					break;
				case 4:
					PICE_sprintf(tempCmd,"%s = %.8X\n",RegKeyWords[i].KeyWord,*(PULONG)(RegKeyWords[i].pValue));
					break;
			}
			Print(OUTPUT_WINDOW,tempCmd);
			if(WaitForKey()==FALSE)break;
		}
	}
	else if(pArgs->Count==1)
	{
		// display selected register
		for(i=0;RegKeyWords[i].pValue!=0;i++)
		{
			if(PICE_strcmpi(pArgs->pToken[0],RegKeyWords[i].KeyWord)==0)
			{
				switch(RegKeyWords[i].ulSize)
				{
					case 1:
						PICE_sprintf(tempCmd,"%s = %.2X\n",RegKeyWords[i].KeyWord,*(PUCHAR)(RegKeyWords[i].pValue));
						break;
					case 2:
						PICE_sprintf(tempCmd,"%s = %.4X\n",RegKeyWords[i].KeyWord,*(PUSHORT)(RegKeyWords[i].pValue));
						break;
					case 4:
						PICE_sprintf(tempCmd,"%s = %.8X\n",RegKeyWords[i].KeyWord,*(PULONG)(RegKeyWords[i].pValue));
						break;
				}
				Print(OUTPUT_WINDOW,tempCmd);
				break;
			}
		}
	}
	else if(pArgs->Count==2)
	{
		// set selected register to value
		for(i=0;RegKeyWords[i].pValue!=0;i++)
		{
			if(PICE_strcmpi(pArgs->pToken[0],RegKeyWords[i].KeyWord)==0)
			{
				switch(RegKeyWords[i].ulSize)
				{
					case 1:
						*(PUCHAR)(RegKeyWords[i].pValue)=(UCHAR)pArgs->Value[1];
						break;
					case 2:
						*(PUSHORT)(RegKeyWords[i].pValue)=(USHORT)pArgs->Value[1];
						break;
					case 4:
						*(PULONG)(RegKeyWords[i].pValue)=(ULONG)pArgs->Value[1];
						break;
				}
				DisplayRegs();
                RepaintSource();
				break;
			}
		}
	}
	return TRUE;
}

//*************************************************************************
// SetCodeDisplay()
//
//*************************************************************************
COMMAND_PROTOTYPE(SetCodeDisplay)
{
    ARGS Args;

	if(pArgs->Count==0)
	{
		bCodeOn=bCodeOn?FALSE:TRUE;

        PICE_memset(&Args,0,sizeof(ARGS));
		Args.Count=0;
        // make unassembler refresh all again
        ulLastDisassStartAddress=ulLastDisassEndAddress=0;
		Unassemble(&Args);
	}
	else if(pArgs->Count==1)
	{
        bCodeOn=(pArgs->Value[0]==0)?FALSE:TRUE;

        PICE_memset(&Args,0,sizeof(ARGS));
		Args.Count=0;
        // make unassembler refresh all again
        ulLastDisassStartAddress=ulLastDisassEndAddress=0;
		Unassemble(&Args);
	}
	return TRUE;
}

//*************************************************************************
// ShowCPU()
//
//*************************************************************************
COMMAND_PROTOTYPE(ShowCPU)
{
ULONG i;

	for(i=0;CPUInfo[i].pValue!=NULL;i++)
	{
		PICE_sprintf(tempCmd,"%s = %.8X\n",CPUInfo[i].Name,*(CPUInfo[i].pValue));
		Print(OUTPUT_WINDOW,tempCmd);
		if(WaitForKey()==FALSE)break;
	}
	return TRUE;
}

//*************************************************************************
// WalkStack()
//
//*************************************************************************
COMMAND_PROTOTYPE(WalkStack)
{
	if(!pArgs->Count)
	{
		IntelStackWalk(CurrentEIP,CurrentEBP,CurrentESP);
	}

	return TRUE;
}

//*************************************************************************
// PokeDword()
//
//*************************************************************************
COMMAND_PROTOTYPE(PokeDword)
{
	ULONG ulData;

	// read old data
    ulData = ReadPhysMem(pArgs->Value[1],sizeof(ULONG));
	PICE_sprintf(tempCmd,"value @ %.8X was %.8X\n",pArgs->Value[1],ulData);
	Print(OUTPUT_WINDOW,tempCmd);

    // write new data
	WritePhysMem(pArgs->Value[1],pArgs->Value[2],sizeof(ULONG));

    // read check
	ulData = ReadPhysMem(pArgs->Value[1],sizeof(ULONG));
	PICE_sprintf(tempCmd,"value @ %.8X = %.8X\n",pArgs->Value[1],ulData);
	Print(OUTPUT_WINDOW,tempCmd);

	return TRUE;
}

//*************************************************************************
// PokeMemory()
//
//*************************************************************************
COMMAND_PROTOTYPE(PokeMemory)
{
    DPRINT((0,"PokeMemory()\n"));
	DPRINT((0,"PokeMemory(): value[0] = %.8X value[1] = %.8X value[3] = %.8X count = %.8X\n",pArgs->Value[0],pArgs->Value[1],pArgs->Value[2],pArgs->Count));

    // must be three parameters
    if(pArgs->Count == 3)
    {
        switch(pArgs->Value[0])
        {
            case 4:
                return PokeDword(pArgs);
            default:
            case 1:
            case 2:
                break;
        }
    }
	else
	{
		Print(OUTPUT_WINDOW,"you need to supply a physical address and datum to write!\n");
	}

    return TRUE;
}


//*************************************************************************
// PeekDword()
//
//*************************************************************************
COMMAND_PROTOTYPE(PeekDword)
{
	ULONG ulData;

	ulData = ReadPhysMem(pArgs->Value[1],sizeof(ULONG));
	PICE_sprintf(tempCmd,"%.8X\n",ulData);
	Print(OUTPUT_WINDOW,tempCmd);

	return TRUE;
}

//*************************************************************************
// PeekWord()
//
//*************************************************************************
COMMAND_PROTOTYPE(PeekWord)
{
	USHORT usData;

	usData = (USHORT)ReadPhysMem(pArgs->Value[1],sizeof(USHORT));
	PICE_sprintf(tempCmd,"%.4X\n",usData);
	Print(OUTPUT_WINDOW,tempCmd);

	return TRUE;
}

//*************************************************************************
// PeekByte()
//
//*************************************************************************
COMMAND_PROTOTYPE(PeekByte)
{
	UCHAR ucData;

	ucData = (UCHAR)ReadPhysMem(pArgs->Value[1],sizeof(UCHAR));
	PICE_sprintf(tempCmd,"%.2X\n",ucData);
	Print(OUTPUT_WINDOW,tempCmd);

	return TRUE;
}

//*************************************************************************
// PeekMemory()
//
//*************************************************************************
COMMAND_PROTOTYPE(PeekMemory)
{
    DPRINT((0,"PeekMemory()\n"));
	DPRINT((0,"PeekMemory(): value[0] = %.8X value[1] = %.8X count = %.8X\n",pArgs->Value[0],pArgs->Value[1],pArgs->Count));

    if(pArgs->Count == 2)
    {
        switch(pArgs->Value[0])
        {
            case 1:
                return PeekByte(pArgs);
            case 2:
                return PeekWord(pArgs);
            case 4:
                return PeekDword(pArgs);
            default:
                break;
        }
    }

    return TRUE;
}

//*************************************************************************
// UnassembleAtCurrentEip()
//
//*************************************************************************
COMMAND_PROTOTYPE(UnassembleAtCurrentEip)
{
    PICE_memset(pArgs,0,sizeof(ARGS));
    pArgs->Count = 2;
    pArgs->Value[0] = CurrentCS;
    pArgs->Value[1] = CurrentEIP;
    Unassemble(pArgs);
    return TRUE;
}

//*************************************************************************
// SwitchTables()
//
//*************************************************************************
COMMAND_PROTOTYPE(SwitchTables)
{
    ULONG i;

    DPRINT((0,"SwitchTables()\n"));

    // no arguments -> display load symbol tables
    if(!pArgs->Count)
    {
        for(i=0;i<DIM(apSymbols);i++)
        {
            if(apSymbols[i])
            {
                if(apSymbols[i] == pCurrentSymbols)
                    PICE_sprintf(tempCmd,"*%-32S @ %.8X (%5u source files)\n",apSymbols[i]->name,(ULONG)apSymbols[i],apSymbols[i]->ulNumberOfSrcFiles);
                else
                    PICE_sprintf(tempCmd," %-32S @ %.8X (%5u source files)\n",apSymbols[i]->name,(ULONG)apSymbols[i],apSymbols[i]->ulNumberOfSrcFiles);
                Print(OUTPUT_WINDOW,tempCmd);
                if(WaitForKey()==FALSE)break;
            }
        }
    }
    // 1 argument -> set new current symbols
    else if(pArgs->Count == 1)
    {
      PDEBUG_MODULE pTempMod;
      char temp[DEBUG_MODULE_NAME_LEN];
      
      pCurrentSymbols = (PICE_SYMBOLFILE_HEADER*)pArgs->Value[0];
      CopyWideToAnsi( temp, pCurrentSymbols->name );
      
      DPRINT((2,"TableSwitchSym: pCurrentSymbols: %x, Name: %s\n", pCurrentSymbols, temp));
      
      pTempMod = IsModuleLoaded(temp);
      if( pTempMod )
	pCurrentMod = pTempMod;
    }

    return TRUE;
}

//*************************************************************************
// SwitchFiles()
//
//*************************************************************************
COMMAND_PROTOTYPE(SwitchFiles)
{
    PICE_SYMBOLFILE_SOURCE* pSrc;
    ULONG i;
    LPSTR p;

    DPRINT((0,"SwitchFiles()\n"));
    // no arguments -> show files for current symbols
    if(!pArgs->Count)
    {
        if(pCurrentSymbols && pCurrentSymbols->ulNumberOfSrcFiles)
        {
            LPSTR pCurrentFile=NULL;

            // find out the current file name
            if(*szCurrentFile!=0)
            {
                if((pCurrentFile = strrchr(szCurrentFile,'/')) )
                {
                    pCurrentFile++;
                }
                else
                {
                    pCurrentFile = szCurrentFile;
                }
            }

            pSrc = (PICE_SYMBOLFILE_SOURCE*)((ULONG)pCurrentSymbols + pCurrentSymbols->ulOffsetToSrcFiles);

            for(i=0;i<pCurrentSymbols->ulNumberOfSrcFiles;i++)
            {
                if(pCurrentFile)
                {
                    if((p = strrchr(pSrc->filename,'/')) )
                    {
                        if(PICE_strcmpi(p+1,pCurrentFile)==0)
                            PICE_sprintf(tempCmd,"*%-32s @ %.8X\n",p+1,(ULONG)pSrc);
                        else
                            PICE_sprintf(tempCmd," %-32s @ %.8X\n",p+1,(ULONG)pSrc);
                    }
                    else
                    {
                        if(PICE_strcmpi(pSrc->filename,pCurrentFile)==0)
                            PICE_sprintf(tempCmd,"*%-32s @ %.8X\n",pSrc->filename,(ULONG)pSrc);
                        else
                            PICE_sprintf(tempCmd," %-32s @ %.8X\n",pSrc->filename,(ULONG)pSrc);
                    }
                }
                else
                {
                    if((p = strrchr(pSrc->filename,'/')) )
                    {
                        PICE_sprintf(tempCmd,"%-32s @ %.8X\n",p+1,(ULONG)pSrc);
                    }
                    else
                    {
                        PICE_sprintf(tempCmd,"%-32s @ %.8X\n",pSrc->filename,(ULONG)pSrc);
                    }
                }
                Print(OUTPUT_WINDOW,tempCmd);

                if(WaitForKey()==FALSE)break;
                (LPSTR)pSrc += pSrc->ulOffsetToNext;
            }
        }
        else
            Print(OUTPUT_WINDOW,"No source files available!\n");
    }
    // 1 argument -> argument is pointer PICE_SYMBOLFILE_SOURCE struct ->
    // set current file and show it
    else if(pArgs->Count == 1)
    {
        PICE_SYMBOLFILE_SOURCE* pSrc = (PICE_SYMBOLFILE_SOURCE*)pArgs->Value[0];
        LPSTR pFilename = pSrc->filename;

	    ClrLine(wWindow[SOURCE_WINDOW].y-1);

        if(PICE_strlen(pFilename)<GLOBAL_SCREEN_WIDTH/2)
        {
		    PutChar(pFilename,1,wWindow[SOURCE_WINDOW].y-1);
        }
        else
        {
            LPSTR p;

            p = strrchr(pFilename,'/');
            if(!p)
            {
                p = pFilename;
            }
            else
            {
                p++;
            }

		    PutChar(p,1,wWindow[SOURCE_WINDOW].y-1);
        }

        // set new current file
        PICE_strcpy(szCurrentFile,pFilename);

        ulCurrentlyDisplayedLineNumber = 1;

        DisplaySourceFile((LPSTR)pSrc+sizeof(PICE_SYMBOLFILE_SOURCE),
                          (LPSTR)pSrc+pSrc->ulOffsetToNext,
                          1,
                          -1);
    }

    return TRUE;
}

//*************************************************************************
// ShowLocals()
//
//*************************************************************************
COMMAND_PROTOTYPE(ShowLocals)
{
	PLOCAL_VARIABLE p;

	if(pArgs->Count==0)
	{
		p = FindLocalsByAddress(GetLinearAddress(CurrentCS,CurrentEIP));
		DPRINT((0,"ShowLocals: %x", p));
		if(p)
		{
			DPRINT((0,"ShowLocals: name %s, type_name %s\n", p->name, p->type_name));
			while(PICE_strlen(p->name))
			{
                if(!p->bRegister)
                {
    				PICE_sprintf(tempCmd,"[EBP%.4d / #%u] %x %s %s \n",p->offset,p->line,p->value,p->type_name,p->name);
                }
                else
                {
    				PICE_sprintf(tempCmd,"[%-8s / #%u] %x %s %s #%u\n",LocalVarRegs[p->offset],p->line,p->value,p->type_name,p->name);
                }
				Print(OUTPUT_WINDOW,tempCmd);
				p++;
			}
		}
	}
	return TRUE;
}

//*************************************************************************
// ShowSymbols()
//
//*************************************************************************
COMMAND_PROTOTYPE(ShowSymbols)
{
    PICE_SYMBOLFILE_HEADER* pSymbols;
    ULONG index,addr;
    LPSTR pSearchString,pName,pFind;

    // no args -> list full symbols for current module
    if(!pArgs->Count)
    {
        // have current module ?
        if(pCurrentMod)
        {
            DPRINT((0,"ShowSymbols(): full listing of symbols for %S\n",pCurrentMod->name));
            addr = (ULONG)pCurrentMod->BaseAddress;

            if((pSymbols = FindModuleSymbols(addr)) )
            {
                PICE_sprintf(tempCmd,"symbols for module \"%S\"\n",pCurrentMod->name);
                Print(OUTPUT_WINDOW,tempCmd);

                index = 0;
                while((index = ListSymbolStartingAt(pCurrentMod,pSymbols,index,tempCmd)))
                {
                    Print(OUTPUT_WINDOW,tempCmd);
                    if(WaitForKey()==FALSE)break;
                }
            }
        }
    }
    // partial name
    else if(pArgs->Count == 1)
    {
        if(pCurrentMod)
        {
            addr = (ULONG)pCurrentMod->BaseAddress;

            if((pSymbols = FindModuleSymbols(addr)))
            {
                pSearchString = (LPSTR)pArgs->Value[0];

                PICE_sprintf(tempCmd,"symbols for module \"%S\" (searching for \"%s\")\n",pCurrentMod->name,pSearchString);
                Print(OUTPUT_WINDOW,tempCmd);

                if(pSearchString)
                {
                    if(*pSearchString=='*' && pSearchString[PICE_strlen(pSearchString)-1]=='*')
                    {
                        pSearchString[PICE_strlen(pSearchString)-1] = 0;
                        pSearchString++;
                        index = 0;
                        while((index = ListSymbolStartingAt(pCurrentMod,pSymbols,index,tempCmd)))
                        {
                            pName = strrchr(tempCmd,' ');
                            pName++;
                            pFind = strstr(pName,pSearchString);
                            if(pFind)
                            {
                                Print(OUTPUT_WINDOW,tempCmd);
                                if(WaitForKey()==FALSE)break;
                            }
                        }
                        // TODO
                    }
                    else if(pSearchString[PICE_strlen(pSearchString)-1]=='*')
                    {
                        pSearchString[PICE_strlen(pSearchString)-1] = 0;
                        index = 0;

						index = ListSymbolStartingAt(pCurrentMod,pSymbols,index,tempCmd);
						if(index)
                        {
							do
							{
								pName = strrchr(tempCmd,' ');
								pName++;
								pFind = strstr(pName,pSearchString);
								if(pFind && (((ULONG)pName-(ULONG)pFind)==0) )
								{
									Print(OUTPUT_WINDOW,tempCmd);
									if(WaitForKey()==FALSE)break;
								}
							}while((index = ListSymbolStartingAt(pCurrentMod,pSymbols,index,tempCmd)));
						}
                    }
                }
            }
        }
    }

    return TRUE;
}

//*************************************************************************
// EvaluateExpression()
//
//*************************************************************************
COMMAND_PROTOTYPE(EvaluateExpression)
{
	PICE_SYMBOLFILE_HEADER* pSymbols;
    ULONG addr;

	if(pArgs->Count == 1)
	{
		if(pCurrentMod)
		{
            addr = (ULONG)pCurrentMod->BaseAddress;

			if( (pSymbols = FindModuleSymbols(addr) ) )
			{
				Evaluate(pSymbols,(LPSTR)pArgs->Value[0]);
			}
		}
	}
	return TRUE;
}

//*************************************************************************
// SizeCodeWindow()
//
//*************************************************************************
COMMAND_PROTOTYPE(SizeCodeWindow)
{
    ULONG NewHeight,TotalHeight;

    if(pArgs->Count == 1)
    {
        NewHeight = pArgs->Value[0];

        TotalHeight = wWindow[SOURCE_WINDOW].cy +
                      wWindow[OUTPUT_WINDOW].cy;

        if(NewHeight < TotalHeight)
        {
            if(wWindow[SOURCE_WINDOW].cy != NewHeight)
			{
				wWindow[SOURCE_WINDOW].cy = NewHeight;
				wWindow[OUTPUT_WINDOW].y = wWindow[SOURCE_WINDOW].y + wWindow[SOURCE_WINDOW].cy + 1;
				wWindow[OUTPUT_WINDOW].cy = TotalHeight - NewHeight;

				RepaintDesktop();
			}
        }
    }
    else
    {
        PICE_sprintf(tempCmd,"code window at position %u has %u lines \n",wWindow[SOURCE_WINDOW].y,wWindow[SOURCE_WINDOW].cy);
        Print(OUTPUT_WINDOW,tempCmd);
    }

    return TRUE;
}

//*************************************************************************
// SizeDataWindow()
//
//*************************************************************************
COMMAND_PROTOTYPE(SizeDataWindow)
{
    ULONG NewHeight,TotalHeight;

    if(pArgs->Count)
    {
        NewHeight = pArgs->Value[0];

        TotalHeight = wWindow[DATA_WINDOW].cy +
                      wWindow[SOURCE_WINDOW].cy;

        if(NewHeight < TotalHeight)
        {
            if(wWindow[DATA_WINDOW].cy != NewHeight)
			{
				wWindow[DATA_WINDOW].cy = NewHeight;
				wWindow[SOURCE_WINDOW].y = wWindow[DATA_WINDOW].y + wWindow[DATA_WINDOW].cy + 1;
				wWindow[SOURCE_WINDOW].cy = TotalHeight - NewHeight;

				RepaintDesktop();
			}
        }
    }
    else
    {
        PICE_sprintf(tempCmd,"data window has %u lines \n",wWindow[DATA_WINDOW].cy);
        Print(OUTPUT_WINDOW,tempCmd);
    }

    return TRUE;
}

//*************************************************************************
// ClearScreen()
//
//*************************************************************************
COMMAND_PROTOTYPE(ClearScreen)
{
    EmptyRingBuffer();

    Clear(OUTPUT_WINDOW);
    CheckRingBuffer();

    return TRUE;
}

//*************************************************************************
// ShowMappings()
//
//*************************************************************************
COMMAND_PROTOTYPE(ShowMappings)
{
#if 0
	ULONG ulPageDir;
	ULONG ulPageTable;
	ULONG address;
	ULONG phys_addr;
	pgd_t * pPGD;
	pmd_t * pPMD;
	pte_t * pPTE;
	struct mm_struct* p = NULL;
	struct task_struct* my_current = (struct task_struct*)0xFFFFE000;

    DPRINT((0,"ShowMappings()\n"));

	if(pArgs->Count == 1)
	{
		// We're in DebuggerShell(), so we live on a different stack
		(ULONG)my_current &= ulRealStackPtr;

        // in case we have a user context we use it's mm_struct
		if(my_current->mm)
        {
            p = my_current->mm;
        }
        // no user context -> use kernel's context
		else
        {
	        p = my_init_mm;
        }

        // get the requested address from arguments
		phys_addr = pArgs->Value[0];

        DPRINT((0,"ShowMappings(): p = %X phys_addr = %X\n",(ULONG)p,phys_addr));

        // for every page directory
		for(ulPageDir=0;ulPageDir<1024;ulPageDir++)
		{
            address = (ulPageDir<<22);

            // get the page directory for the address
			pPGD = pgd_offset(p,address);
            // if page dir present
			if(pPGD && pgd_val(*pPGD)&_PAGE_PRESENT)
			{
                DPRINT((0,"ShowMappings(): page directory present for %x\n",address));
				// not large page
    			if(!(pgd_val(*pPGD)&_PAGE_4M))
				{
                    DPRINT((0,"ShowMappings(): page directory for 4k pages\n"));
        			for(ulPageTable=0;ulPageTable<1024;ulPageTable++)
					{
        				address = (ulPageDir<<22)|(ulPageTable<<12);

						pPMD = pmd_offset(pPGD,address);
						if(pPMD)
						{
							pPTE = pte_offset(pPMD,address);
							if(pPTE)
							{
                                if(*(PULONG)pPTE & _PAGE_PRESENT)
                                {
								    ULONG ulPte = *(PULONG)pPTE & 0xFFFFF000;

								    if(ulPte == (phys_addr & 0xFFFFF000))
								    {
									    PICE_sprintf(tempCmd,"%.8X\n",address+(phys_addr&0xFFF));
									    Print(OUTPUT_WINDOW,tempCmd);
									    if(WaitForKey()==FALSE)return TRUE;
								    }
                                }
							}
						}
					}
				}
				// large page
				else
				{
       				address = (ulPageDir<<22);
					if((pgd_val(*pPGD)&0xFFC00000) == (phys_addr & 0xFFC00000) )
					{
                        if( ((address|(phys_addr&0x7FFFFF))&~TASK_SIZE) == phys_addr)
                        	PICE_sprintf(tempCmd,"%.8X (identity map %.8X+%.8X)\n",address|(phys_addr&0x7FFFFF),TASK_SIZE,phys_addr);
                        else
                        	PICE_sprintf(tempCmd,"%.8X\n",address|(phys_addr&0x7FFFFF));
						Print(OUTPUT_WINDOW,tempCmd);
						if(WaitForKey()==FALSE)return TRUE;
					}
				}
			}
		}
	}
#endif
	PICE_sprintf(tempCmd,"Not implemented yet!\n");
	Print(OUTPUT_WINDOW,tempCmd);
	return TRUE;
}

//*************************************************************************
// ShowTimers()
//
//*************************************************************************
COMMAND_PROTOTYPE(ShowTimers)
{

	return TRUE;
}

//*************************************************************************
// FindPCIVendorName()
//
//*************************************************************************
LPSTR FindPCIVendorName(USHORT vendorid)
{
    ULONG i;

    for(i=0;i<DIM(PCIVendorIDs);i++)
    {
        if(vendorid == PCIVendorIDs[i].vendorid)
            return PCIVendorIDs[i].vendor_name;
    }

    return NULL;
}

//*************************************************************************
// ShowPCI()
//
//*************************************************************************
COMMAND_PROTOTYPE(ShowPCI)
{
    ULONG oldCF8,data,bus,dev,reg,i,ulNumBaseAddresses,bus_req=0,dev_req=0;
    PCI_NUMBER pciNumber;
    PCI_COMMON_CONFIG pciConfig,*ppciConfig;
    PULONG p;
    LPSTR pVendorName;
    char temp[32];
    BOOLEAN bShowAll = FALSE,bUseDev=TRUE,bUseBus=TRUE;


    DPRINT((0,"ShowPCI()\n"));

    if(pArgs->CountSwitches>1)
        return TRUE;

    if(pArgs->CountSwitches==1)
    {
        if(pArgs->Switch[0] == 'a')
            bShowAll = TRUE;
    }

    if(pArgs->Count < 3)
    {
        if(pArgs->Count > 0)
        {
            bUseBus = FALSE;
            bus_req = pArgs->Value[0];
        }
        if(pArgs->Count > 1)
        {
            bUseDev = FALSE;
            dev_req = pArgs->Value[1];
        }

        // save old config space selector
	    oldCF8 = inl((PULONG)0xcf8);

        for(bus=0;bus<256;bus++)
        {
            for(dev=0;dev<32;dev++)
            {
                if(!((bUseDev || dev == dev_req) &&
                    (bUseBus || bus == bus_req) ))
                    continue;

                pciNumber.u.AsUlong     = 0;
                pciNumber.u.bits.dev    = dev;
                pciNumber.u.bits.bus    = bus;
                pciNumber.u.bits.func   = 0;
                pciNumber.u.bits.ce     = 1;
	            outl(pciNumber.u.AsUlong,(PULONG)0xcf8);
	            data = inl((PULONG)0xcfc);

                if(data != 0xFFFFFFFF) // valid device
                {
                    if((pVendorName = FindPCIVendorName((USHORT)data)) )
                    {
                        PICE_sprintf(tempCmd,"Bus%-8uDev%-8u === %.4X %.4X %s ====\n",bus,dev,(USHORT)data,(USHORT)(data>>16),pVendorName);
                    }
                    else
                    {
                        PICE_sprintf(tempCmd,"Bus%-8uDev%-8u === %.4X %.4X N/A ====\n",bus,dev,(USHORT)data,(USHORT)(data>>16));
                    }
                    Print(OUTPUT_WINDOW,tempCmd);
                    if(WaitForKey()==FALSE)goto CommonShowPCIExit;

                    p = (PULONG)&pciConfig;
                    for(reg=0;reg<sizeof(PCI_COMMON_CONFIG)/sizeof(ULONG);reg++)
                    {
                        pciNumber.u.AsUlong     = 0;
                        pciNumber.u.bits.dev    = dev;
                        pciNumber.u.bits.bus    = bus;
                        pciNumber.u.bits.func   = 0;
                        pciNumber.u.bits.reg    = reg;
                        pciNumber.u.bits.ce     = 1;

	                    outl(pciNumber.u.AsUlong,(PULONG)0xcf8);
	                    *p++ = inl((PULONG)0xcfc);
                    }
                    PICE_sprintf(tempCmd,"SubVendorId %.4X SubSystemId %.4X\n",pciConfig.u.type0.SubVendorID,pciConfig.u.type0.SubSystemID);
                    Print(OUTPUT_WINDOW,tempCmd);
                    if(WaitForKey()==FALSE)goto CommonShowPCIExit;

                    if(bShowAll)
                    {
                        for(ulNumBaseAddresses=0,i=0;i<6;i++)
                        {
                            if(pciConfig.u.type0.BaseAddresses[i] != 0)
                                ulNumBaseAddresses++;
                        }
                        if(ulNumBaseAddresses)
                        {
                            Print(OUTPUT_WINDOW,"BaseAddresses:");
                            tempCmd[0] = 0;
                            for(i=0;i<6;i++)
                            {
                                if(pciConfig.u.type0.BaseAddresses[i] != 0)
                                {
                                    PICE_sprintf(temp," %u:%.8X",i,pciConfig.u.type0.BaseAddresses[i]);
                                    PICE_strcat(tempCmd,temp);
                                }
                            }
                            Print(OUTPUT_WINDOW,tempCmd);
                            Print(OUTPUT_WINDOW,"\n");
                            if(WaitForKey()==FALSE)goto CommonShowPCIExit;
                        }
                        ppciConfig = &pciConfig;
                        SHOW_FIELD_WORD(ppciConfig,Status,TRUE);
                        SHOW_FIELD_WORD(ppciConfig,Command,TRUE);
                        SHOW_FIELD_BYTE(ppciConfig,RevisionID,TRUE);
                        SHOW_FIELD_BYTE(ppciConfig,ProgIf,TRUE);
                        SHOW_FIELD_BYTE(ppciConfig,BaseClass,TRUE);
                        SHOW_FIELD_BYTE(ppciConfig,SubClass,TRUE);
                        SHOW_FIELD_BYTE(ppciConfig,CacheLineSize,TRUE);
                        SHOW_FIELD_BYTE(ppciConfig,LatencyTimer,TRUE);
                    }
                }
            }
        }

CommonShowPCIExit:
        // restore old config space selector
	    outl(oldCF8,(PULONG)0xcf8);

    }
    else if(pArgs->Count == 1)
    {
    }

	return TRUE;
}

//*************************************************************************
// SetKeyboardLayout()
//
//*************************************************************************
COMMAND_PROTOTYPE(SetKeyboardLayout)
{
    ENTER_FUNC();

    switch(pArgs->Count)
    {
        case 0:
            PICE_sprintf(tempCmd,"current layout = %s\n",(ucKeyboardLayout == GERMANY)?"german":"american");
            Print(OUTPUT_WINDOW,tempCmd);
            break;
        case 1:
            if(pArgs->Value[0] < 2)
            {
                ucKeyboardLayout = pArgs->Value[0];
                PICE_sprintf(tempCmd,"current layout = %s\n",(ucKeyboardLayout == GERMANY)?"german":"american");
                Print(OUTPUT_WINDOW,tempCmd);
            }
            else
            {
                PICE_sprintf(tempCmd,"current layout = %s\n",(ucKeyboardLayout == GERMANY)?"german":"american");
                Print(OUTPUT_WINDOW,tempCmd);
            }
            break;
    }

    LEAVE_FUNC();

    return TRUE;
}

//*************************************************************************
// ShowSysCallTable()
//
//*************************************************************************
COMMAND_PROTOTYPE(ShowSysCallTable)
{
#if 0
    LPSTR pName;
    ULONG i;

    ENTER_FUNC();

    if(pArgs->Count == 0)
    {
        PICE_sprintf(tempCmd,"%u system calls\n",190);
        Print(OUTPUT_WINDOW,tempCmd);
        if(WaitForKey()!=FALSE)
            for(i=0;i<190;i++)
            {
                if((pName = FindFunctionByAddress(sys_call_table[i],NULL,NULL)) )
                {
                    PICE_sprintf(tempCmd,"%-.3u: %.8X (%s)\n",i,sys_call_table[i],pName);
                }
                else
                {
                    PICE_sprintf(tempCmd,"%-.3u: %.8X (%s)\n",i,sys_call_table[i],pName);
                }
                Print(OUTPUT_WINDOW,tempCmd);
                if(WaitForKey()==FALSE)
                    break;
            }
    }
    else if(pArgs->Count == 1)
    {
        i = pArgs->Value[0];
        if(i<190)
        {
            if((pName = FindFunctionByAddress(sys_call_table[i],NULL,NULL)) )
            {
                PICE_sprintf(tempCmd,"%-.3u: %.8X (%s)\n",i,sys_call_table[i],pName);
            }
            else
            {
                PICE_sprintf(tempCmd,"%-.3u: %.8X (%s)\n",i,sys_call_table[i],pName);
            }
            Print(OUTPUT_WINDOW,tempCmd);
        }
    }

    LEAVE_FUNC();
#endif
	PICE_sprintf(tempCmd,"Not implemented yet!\n");
	Print(OUTPUT_WINDOW,tempCmd);
	return TRUE;
}

//*************************************************************************
// SetAltKey()
//
//*************************************************************************
COMMAND_PROTOTYPE(SetAltKey)
{
    if(pArgs->Count == 1)
    {
        ucBreakKey = (UCHAR)pArgs->Value[0];

        PICE_sprintf(tempCmd,"new break key is CTRL-%c\n",ucBreakKey);
        Print(OUTPUT_WINDOW,tempCmd);
    }
    else if(pArgs->Count == 0)
    {
        PICE_sprintf(tempCmd,"current break key is CTRL-%c\n",ucBreakKey);
        Print(OUTPUT_WINDOW,tempCmd);
    }


    COMMAND_RET;
}

//*************************************************************************
// ShowContext()
//
//*************************************************************************
COMMAND_PROTOTYPE(ShowContext)
{
    COMMAND_RET;
}

//*************************************************************************
//
// utility functions for parsing
//
//*************************************************************************


//*************************************************************************
// FindCommand()
//
//*************************************************************************
LPSTR FindCommand(LPSTR p)
{
    ULONG i,j,k=0;
    LPSTR result=NULL;

	tempCmd[0]=0;
	for(j=0,i=0;CmdTable[i].Cmd!=NULL;i++)
	{
		if(PICE_strncmpi(CmdTable[i].Cmd,p,PICE_strlen(p)) == 0 &&
           CmdTable[i].CommandGroup != COMMAND_GROUP_HELP_ONLY)
		{
			if(PICE_strlen(tempCmd))
				PICE_strcat(tempCmd,", ");
			PICE_strcat(tempCmd,CmdTable[i].Cmd);
			j++;
			k=i;
		}
	}
	if(PICE_strlen(tempCmd))
	{
		SetBackgroundColor(COLOR_CAPTION);
		SetForegroundColor(COLOR_TEXT);
		ClrLine(wWindow[OUTPUT_WINDOW].y+wWindow[OUTPUT_WINDOW].cy);
		PutChar(tempCmd,1,wWindow[OUTPUT_WINDOW].y+wWindow[OUTPUT_WINDOW].cy);
		if(j==1)
		{
			PICE_sprintf(tempCmd,"%s",CmdTable[k].Help);
			PutChar(tempCmd,40,wWindow[OUTPUT_WINDOW].y+wWindow[OUTPUT_WINDOW].cy);
			result=CmdTable[k].Cmd;
		}
        ResetColor();
	}
	return result;
}


//*************************************************************************
// CompactString()
//
//*************************************************************************
void CompactString(LPSTR p)
{
ULONG i;

	for(i=1;i<PICE_strlen(p);i++)
	{
		if(p[i]==' ' && p[i-1]==' ')
		{
			PICE_strcpy(&p[i-1],&p[i]);
			i=1;
		}
	}
}

//*************************************************************************
// PICE_strtok()
//
//*************************************************************************
char* PICE_strtok(char *szInputString)
{
static char* szCurrString;
char *szTempString;
ULONG currlength;
ULONG i;

	if(szInputString)
	{
		szCurrString=szInputString;
		CompactString(szCurrString);
	}

	currlength=PICE_strlen(szCurrString);
	if(!currlength)
	{
		szCurrString=0;
		return NULL;
	}
	for(i=0;i<currlength;i++)
	{
		if(szCurrString[i]==' ')
		{
			szCurrString[i]=0;
			break;
		}
	}
	szTempString=szCurrString;
	szCurrString=szCurrString+i+1;
	return szTempString;
}

//*************************************************************************
// ConvertTokenToHex()
//
//*************************************************************************
BOOLEAN ConvertTokenToHex(LPSTR p,PULONG pValue)
{
ULONG result=0,i;

	gCurrentSelector=0;
	gCurrentOffset=0;
	for(i=0;i<8 && p[i]!=0 && p[i]!=' ' && p[i]!=':';i++)
	{
		if(p[i]>='0' && p[i]<='9')
		{
			result<<=4;
			result|=(ULONG)(UCHAR)(p[i]-'0');
		}
		else if(p[i]>='A' && p[i]<='F')
		{
			result<<=4;
			result|=(ULONG)(UCHAR)(p[i]-'A'+10);
		}
		else if(p[i]>='a' && p[i]<='f')
		{
			result<<=4;
			result|=(ULONG)(UCHAR)(p[i]-'a'+10);
		}
		else
			return FALSE;
	}
	p+=(i+1);
	if(p[i]==':')
	{
		ULONG ulSelector=result;
		if(ulSelector>0xFFFF)
			return FALSE;
		for(i=0;i<8 && p[i]!=0 && p[i]!=' ' && p[i]!=':';i++)
		{
			if(p[i]>='0' && p[i]<='9')
			{
				result<<=4;
				result|=(ULONG)(UCHAR)(p[i]-'0');
			}
			else if(p[i]>='A' && p[i]<='F')
			{
				result<<=4;
				result|=(ULONG)(UCHAR)(p[i]-'A'+10);
			}
			else if(p[i]>='a' && p[i]<='f')
			{
				result<<=4;
				result|=(ULONG)(UCHAR)(p[i]-'a'+10);
			}
			else
				return FALSE;
		}
		gCurrentSelector=(USHORT)ulSelector;
		gCurrentOffset=result;
		result = GetLinearAddress((USHORT)ulSelector,result);
	}
	*pValue=result;
	return TRUE;
}

//*************************************************************************
// ConvertTokenToDec()
//
//*************************************************************************
BOOLEAN ConvertTokenToDec(LPSTR p,PULONG pValue)
{
    ULONG result=0;
    char c;

	while((c = *p))
	{
		if(c >= '0' && c <= '9')
		{
			result *= 10;
			result += (ULONG)(c - '0');
		}
		else
			return FALSE;

        p++;
	}
    *pValue = result;
    return TRUE;
}

//*************************************************************************
// ConvertTokenToSymbol()
//
//*************************************************************************
BOOLEAN ConvertTokenToSymbol(LPSTR pToken,PULONG pValue)
{
    LPSTR pEx;
    char temp[64];
    LPSTR p;
	PDEBUG_MODULE pModFound;

    DPRINT((0,"ConvertTokenToSymbol()\n"));

    PICE_strcpy(temp,pToken);
    p = temp;

    // test for module!symbol string
    pEx = PICE_strchr(p,'!');
    if(pEx)
    {
        DPRINT((0,"ConvertTokenToSymbol(): module!symbol syntax detected\n"));
        // terminate module name
        *pEx = 0;
        // now we have two pointers
        pEx++;
        DPRINT((0,"ConvertTokenToSymbol(): module = %s symbol = %s\n",p,pEx));

		if( pModFound=IsModuleLoaded(p) )
        {
            if((*pValue = FindFunctionInModuleByName(pEx,pModFound)))
                return TRUE;
        }
    }
    else
    {
        if(pCurrentMod)
        {
            if((*pValue = FindFunctionInModuleByName(p,pCurrentMod)))
                return TRUE;
        }
    	return ScanExports(p,pValue);
    }
    return FALSE;
}

//*************************************************************************
// ConvertTokenToModuleAndName()
//
//*************************************************************************
BOOLEAN ConvertTokenToModuleAndName(LPSTR pToken,PULONG pulModuleName,PULONG pulFunctionName)
{
    LPSTR pEx;
    char temp[64];
    LPSTR p;
    static char module_name[128];
    static char function_name[128];

    // test for module!symbol string
    PICE_strcpy(temp,pToken);
    p = temp;

    DPRINT((0,"ConvertTokenToModuleAndName(%s)\n",p));

    pEx = PICE_strchr(p,'!');
    if(pEx)
    {
        DPRINT((0,"ConvertTokenToModuleAndName(): module!symbol syntax detected\n"));
        // terminate module name
        *pEx = 0;
        // now we have two pointers
        pEx++;
        DPRINT((0,"ConvertTokenToModuleAndName(): module = %s symbol = %s\n",p,pEx));
        PICE_strcpy(module_name,p);
        PICE_strcpy(function_name,pEx);
        *pulModuleName = (ULONG)module_name;
        *pulFunctionName = (ULONG)function_name;
        return TRUE;

    }
    return FALSE;
}

//*************************************************************************
// ConvertTokenToModule()
//
// convert an argument module name to a pointer to the module's symbols
//*************************************************************************
BOOLEAN ConvertTokenToModule(LPSTR p,PULONG pValue)
{
    ULONG i;
	char temp[DEBUG_MODULE_NAME_LEN];

    for(i=0;i<DIM(apSymbols);i++)
    {
        if(apSymbols[i])
        {
			CopyWideToAnsi(temp,apSymbols[i]->name);
            if(PICE_strcmpi(p,temp)==0)
            {
                *pValue = (ULONG)apSymbols[i];
                return TRUE;
            }
        }
    }

    for(i=0;i<DIM(apSymbols);i++)
    {
        if(apSymbols[i])
        {
			CopyWideToAnsi(temp,apSymbols[i]->name);
            if(PICE_strncmpi(temp,p,PICE_strlen(p))==0)
            {
                *pValue = (ULONG)apSymbols[i];
                return TRUE;
            }
        }
    }

	return FALSE;
}

//*************************************************************************
// ConvertTokenToProcess()
//
//*************************************************************************
BOOLEAN ConvertTokenToProcess(LPSTR p,PULONG pValue)
{
	return FALSE;
}

//*************************************************************************
// ReplaceKeywordWithValue()
//
//*************************************************************************
BOOLEAN ReplaceKeywordWithValue(LPSTR p,PULONG pValue,KEYWORDS* pKeyWords)
{
    ULONG i;

	for(i=0;pKeyWords[i].KeyWord!=NULL;i++)
	{
		if(PICE_strcmpi(p,pKeyWords[i].KeyWord)==0)
		{
			switch(pKeyWords[i].ulSize)
			{
				case sizeof(USHORT):
					*pValue=(ULONG)*(PUSHORT)(pKeyWords[i].pValue);
					break;
				case sizeof(ULONG):
					*pValue=*(PULONG)(pKeyWords[i].pValue);
					break;
			}
			return TRUE;
		}
	}
	return FALSE;
}

//*************************************************************************
// ConvertTokenToKeyword()
//
//*************************************************************************
BOOLEAN ConvertTokenToKeyword(LPSTR p,PULONG pValue)
{
    char Name[256];
    ULONG count;

	DPRINT((0,"ConvertTokenToKeyword()\n"));
	count=StrLenUpToWhiteChar(p," ");
	PICE_strncpy(Name,p,count);
	Name[count]=0;
	if(ReplaceKeywordWithValue(Name,pValue,RegKeyWords))
	{
		DPRINT((0,"ConvertTokenToKeyword(): success\n"));
		return TRUE;
	}
	return FALSE;
}

//*************************************************************************
// ConvertTokenToSpecialKeyword()
//
//*************************************************************************
BOOLEAN ConvertTokenToSpecialKeyword(LPSTR p,PULONG pValue)
{
    char Name[256];
    ULONG count;

	count=StrLenUpToWhiteChar(p," ");
	PICE_strncpy(Name,p,count);
	Name[count]=0;
	if(ReplaceKeywordWithValue(Name,pValue,SpecialKeyWords))
	{
		return TRUE;
	}
	return FALSE;
}

//*************************************************************************
// ConvertTokenToOnOff()
//
//*************************************************************************
BOOLEAN ConvertTokenToOnOff(LPSTR p,PULONG pValue)
{
    char Name[256];
    ULONG count;

	count=StrLenUpToWhiteChar(p," ");
	PICE_strncpy(Name,p,count);
	Name[count]=0;
	if(ReplaceKeywordWithValue(Name,pValue,OnOffKeyWords))
	{
		return TRUE;
	}
	return FALSE;
}

//*************************************************************************
// ConvertSizeToKeyword()
//
//*************************************************************************
BOOLEAN ConvertSizeToKeyword(LPSTR p,PULONG pValue)
{
    ULONG count;

	count=StrLenUpToWhiteChar(p," ");
    if(count > 1)
	    return FALSE;

    switch(*p)
    {
        // BYTE
        case 'b':
        case 'B':
            *pValue = 1;
            break;
        // WORD
        case 'w':
        case 'W':
            *pValue = 2;
            break;
        // DWORD
        case 'd':
        case 'D':
            *pValue = 4;
            break;
        // QWORD
        case 'q':
        case 'Q':
            *pValue = 4;
            break;
        default:
            return FALSE;
    }

    return TRUE;
}


//*************************************************************************
// ConvertTokenToSrcFile()
//
//*************************************************************************
BOOLEAN ConvertTokenToSrcFile(LPSTR p,PULONG pValue)
{
    PICE_SYMBOLFILE_SOURCE* pSrc;
    LPSTR pFilename,pFilenameSrc;
    ULONG i;

    DPRINT((0,"ConvertTokenToSrcFile(%s)\n",p));

    if(pCurrentSymbols && pCurrentSymbols->ulNumberOfSrcFiles)
    {
        DPRINT((0,"ConvertTokenToSrcFile(): current symbols for %S\n",pCurrentSymbols->name));

        pSrc = (PICE_SYMBOLFILE_SOURCE*)((ULONG)pCurrentSymbols + pCurrentSymbols->ulOffsetToSrcFiles);

        for(i=0;i<pCurrentSymbols->ulNumberOfSrcFiles;i++)
        {
            pFilename = strrchr(pSrc->filename,'/');
            if(!pFilename)
                pFilename = pSrc->filename;
            else
                pFilename++;

            pFilenameSrc = strrchr(p,'/');
            if(!pFilenameSrc )
                pFilenameSrc = p;
            else
                pFilenameSrc++;

            DPRINT((0,"ConvertTokenToSrcFile(): %s\n",pFilename));

            if(PICE_strcmpi(pFilename,pFilenameSrc) == 0)
            {
                DPRINT((0,"ConvertTokenToSrcFile(): found %s\n",pFilename));

                *pValue = (ULONG)pSrc;
                return TRUE;
            }

            // go to next file
            (LPSTR)pSrc += pSrc->ulOffsetToNext;
        }

        pSrc = (PICE_SYMBOLFILE_SOURCE*)((ULONG)pCurrentSymbols + pCurrentSymbols->ulOffsetToSrcFiles);

        // if not found now do a lookup for partials
        for(i=0;i<pCurrentSymbols->ulNumberOfSrcFiles;i++)
        {
            pFilename = strrchr(pSrc->filename,'/');
            if(!pFilename)
                pFilename = pSrc->filename;
            else
                pFilename++;

            DPRINT((0,"ConvertTokenToSrcFile(): %s\n",pFilename));

            if(PICE_strncmpi(pFilename,p,PICE_strlen(p)) == 0)
            {
                DPRINT((0,"ConvertTokenToSrcFile(): found %s\n",pFilename));

                *pValue = (ULONG)pSrc;
                return TRUE;
            }

            // go to next file
            (LPSTR)pSrc += pSrc->ulOffsetToNext;
        }

    }
    return FALSE;
}

//*************************************************************************
// ConvertTokenToLineNumber()
//
//*************************************************************************
BOOLEAN ConvertTokenToLineNumber(LPSTR p,PULONG pValue)
{
    ULONG ulDecimal;

    DPRINT((0,"ConvertTokenToLineNumber()\n"));
    if(*p++ == '.')
    {
        ulDecimal = ExtractNumber(p);
        DPRINT((0,"ConvertTokenToLineNumber(): ulDecimal = %u\n",ulDecimal));
        if(ulDecimal)
        {
            DPRINT((0,"ConvertTokenToLineNumber(): current file  = %s\n",szCurrentFile));
            if(pCurrentMod && PICE_strlen(szCurrentFile))
            {
                DPRINT((0,"ConvertTokenToLineNumber(): current file  %S\n",pCurrentMod->name));
                if(FindAddressForSourceLine(ulDecimal,szCurrentFile,pCurrentMod,pValue))
                {
                    DPRINT((0,"ConvertTokenToLineNumber(): value  = %x\n",*pValue));
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}


//*************************************************************************
// IsWhiteChar()
//
//*************************************************************************
BOOLEAN IsWhiteChar(char c,LPSTR WhiteChars)
{
	USHORT lenWhiteChar = PICE_strlen(WhiteChars);
	USHORT i;
	for(i=0;i<lenWhiteChar;i++)
	{
		if(c == WhiteChars[i])
			return TRUE;
	}
	return FALSE;
}

//*************************************************************************
// StrLenUpToWhiteChar()
//
//*************************************************************************
ULONG StrLenUpToWhiteChar(LPSTR p,LPSTR WhiteChars)
{
    ULONG i;

	for(i=0;p[i]!=0 && !IsWhiteChar(p[i],WhiteChars);i++);

	return i;
}

//
// command line parser
//
void Parse(LPSTR pCmdLine,BOOLEAN bInvokedByFkey)
{
    ULONG i,j;
    BOOLEAN result=FALSE;
    char *pToken;
    ARGS Arguments;
    CMDTABLE *pCurrentCommand=NULL;

    ulCountForWaitKey = 0;

    if(!bInvokedByFkey)
    {
        ClrLine(wWindow[OUTPUT_WINDOW].y+wWindow[OUTPUT_WINDOW].usCurY);
    }

    PICE_memset(&Arguments,0,sizeof(ARGS));

	for(j=0,i=0;CmdTable[i].Cmd!=NULL;i++)
	{
		if(PICE_strncmpi(CmdTable[i].Cmd,pCmdLine,PICE_strlen(CmdTable[i].Cmd))==0 &&
		   PICE_strlen(CmdTable[i].Cmd)==StrLenUpToWhiteChar(pCmdLine," ") &&
           CmdTable[i].CommandGroup != COMMAND_GROUP_HELP_ONLY)
		{
			pCurrentCommand=&CmdTable[i];
			break;
		}
	}
	if(pCurrentCommand==NULL)
	{
		Print(OUTPUT_WINDOW,"  <-- command not found\n");
		Print(OUTPUT_WINDOW,":");
		goto CommonParseReturnPoint;
	}

	pToken = PICE_strtok( pCmdLine);
	// get the args and convert them into numbers
	i=0;
    j=0;
	do
	{
		pToken=PICE_strtok( NULL);
		DPRINT((0,"pToken = %s\n",pToken));
		if(pToken)
		{
            if(*pToken == '-' && PICE_strlen(pToken)==2)
            {
        		DPRINT((0,"might be a switch\n"));
			    if(pCurrentCommand->Flags & COMMAND_HAS_SWITCHES)
                {
                    // token starts with '-' and is 2 chars long
                    // must be a switch
                    if(PICE_strchr(pCurrentCommand->pszRecognizedSwitches,*(pToken+1)) )
                    {
                		DPRINT((0,"is a switch!\n"));
                        Arguments.Switch[j++]=*(pToken+1);
                        continue;
                    }
                    // not a valid switch
                    else
                    {
					    PICE_sprintf(tempCmd,"  <-- %s is not a valid switch\n",pToken);
					    Print(OUTPUT_WINDOW,tempCmd);
                		Print(OUTPUT_WINDOW,":");
                        goto CommonParseReturnPoint;
                    }
                }
                else
                {
					PICE_sprintf(tempCmd,"  <-- %s can't have any switches\n",pCurrentCommand->Cmd);
					Print(OUTPUT_WINDOW,tempCmd);
            		Print(OUTPUT_WINDOW,":");
                    goto CommonParseReturnPoint;
                }
            }

			if(pCurrentCommand->Flags & COMMAND_HAS_PARAMS)
			{
				if(!pCurrentCommand->ParamFlags[i])
				{
				    PICE_sprintf(tempCmd,"  <-- %s can't have more than %u parameters\n",pCurrentCommand->Cmd,i);
					Print(OUTPUT_WINDOW,tempCmd);
            		Print(OUTPUT_WINDOW,":");
                    goto CommonParseReturnPoint;
				}
                DPRINT((0,"Parse(): PARAM_CAN_BE_SRCLINE\n"));
                if(pCurrentCommand->ParamFlags[i] & PARAM_CAN_BE_SRCLINE)
                {
					if(ConvertTokenToLineNumber(pToken,&Arguments.Value[i]))
                    {
                        i++;
                        continue;
                    }
                    if(*pToken == '.')
                    {
				        PICE_sprintf(tempCmd,"  <-- no line number %s found\n",pToken);
					    Print(OUTPUT_WINDOW,tempCmd);
                		Print(OUTPUT_WINDOW,":");
                        goto CommonParseReturnPoint;
                    }
                }
                DPRINT((0,"Parse(): PARAM_CAN_BE_NUMERIC\n"));
                if(pCurrentCommand->ParamFlags[i] & PARAM_CAN_BE_NUMERIC)
                {
    				if(ConvertTokenToHex(pToken,&Arguments.Value[i]))
                    {
                        i++;
                        continue;
                    }
                }
                DPRINT((0,"Parse(): PARAM_CAN_BE_DECIMAL\n"));
                if(pCurrentCommand->ParamFlags[i] & PARAM_CAN_BE_DECIMAL)
                {
    				if(ConvertTokenToDec(pToken,&Arguments.Value[i]))
                    {
                        i++;
                        continue;
                    }
                }
                DPRINT((0,"Parse(): PARAM_CAN_BE_REG_KEYWORD\n"));
                if(pCurrentCommand->ParamFlags[i] & PARAM_CAN_BE_REG_KEYWORD)
                {
					if(ConvertTokenToKeyword(pToken,&Arguments.Value[i]))
                    {
						Arguments.pToken[i] = pToken;
                        i++;
                        continue;
                    }
                }
                DPRINT((0,"Parse(): PARAM_CAN_BE_SYMBOLIC\n"));
                if(pCurrentCommand->ParamFlags[i] & PARAM_CAN_BE_SYMBOLIC)
                {
            		if(ConvertTokenToSymbol(pToken,&Arguments.Value[i]))
                    {
                        i++;
                        continue;
                    }
                }
                DPRINT((0,"Parse(): PARAM_CAN_BE_VIRTUAL_SYMBOLIC\n"));
                if(pCurrentCommand->ParamFlags[i] & PARAM_CAN_BE_VIRTUAL_SYMBOLIC)
                {
                    DPRINT((0,"might be a virtual modname!symbol syntax!\n"));
            		if(ConvertTokenToModuleAndName(pToken,&Arguments.Value[i],&Arguments.Value[i+1]))
                    {
                        Arguments.bNotTranslated[i]=TRUE;
                        Arguments.bNotTranslated[i+1]=TRUE;
                        i+=2;
                        continue;
                    }
                }
                DPRINT((0,"Parse(): PARAM_CAN_BE_MODULE\n"));
                if(pCurrentCommand->ParamFlags[i] & PARAM_CAN_BE_MODULE)
                {
                    if(ConvertTokenToModule(pToken,&Arguments.Value[i]))
                    {
                        i++;
                        continue;
                    }
                }
                DPRINT((0,"Parse(): PARAM_CAN_BE_PRNAME\n"));
                if(pCurrentCommand->ParamFlags[i] & PARAM_CAN_BE_PRNAME)
                {
                    if(ConvertTokenToProcess(pToken,&Arguments.Value[i]))
                    {
                        i++;
                        continue;
                    }
                }
                DPRINT((0,"Parse(): PARAM_CAN_BE_SRC_FILE\n"));
                if(pCurrentCommand->ParamFlags[i] & PARAM_CAN_BE_SRC_FILE)
                {
                    if(ConvertTokenToSrcFile(pToken,&Arguments.Value[i]))
                    {
                        i++;
                        continue;
                    }
                }
                DPRINT((0,"Parse(): PARAM_CAN_BE_ASTERISK\n"));
                if(pCurrentCommand->ParamFlags[i] & PARAM_CAN_BE_ASTERISK)
                {
                    if(PICE_strlen(pToken)==1 && pToken[0]=='*')
                    {
                        Arguments.Value[i]=-1;
                        i++;
                        continue;
                    }
                }
                DPRINT((0,"Parse(): PARAM_CAN_BE_LETTER\n"));
                if(pCurrentCommand->ParamFlags[i] & PARAM_CAN_BE_LETTER)
                {
                    if(PICE_strlen(pToken)==1 && PICE_isprint(pToken[0]))
                    {
                        Arguments.Value[i]=(ULONG)pToken[0];
                        i++;
                        continue;
                    }
                }
                DPRINT((0,"Parse(): PARAM_CAN_BE_ONOFF\n"));
                if(pCurrentCommand->ParamFlags[i] & PARAM_CAN_BE_ONOFF)
                {
					if(ConvertTokenToOnOff(pToken,&Arguments.Value[i]))
                    {
                        i++;
                        continue;
                    }
                }
                DPRINT((0,"Parse(): PARAM_CAN_BE_PARTIAL_SYM_NAME\n"));
                if(pCurrentCommand->ParamFlags[i] & PARAM_CAN_BE_PARTIAL_SYM_NAME)
                {
					Arguments.Value[i] = (ULONG)pToken;
                    i++;
                    continue;
                }
                DPRINT((0,"Parse(): PARAM_CAN_BE_ANY_STRING\n"));
                if(pCurrentCommand->ParamFlags[i] & PARAM_CAN_BE_ANY_STRING)
                {
					Arguments.Value[i] = (ULONG)pToken;
                    i++;
                    continue;
                }
                DPRINT((0,"Parse(): PARAM_CAN_BE_SIZE_DESC\n"));
                if(pCurrentCommand->ParamFlags[i] & PARAM_CAN_BE_SIZE_DESC)
                {
					if(ConvertSizeToKeyword(pToken,&Arguments.Value[i]))
                    {
						Arguments.pToken[i] = pToken;
                        i++;
                        continue;
                    }
                }
                PICE_sprintf(tempCmd,"  <-- syntax error in parameter %u!\n",i);
				Print(OUTPUT_WINDOW,tempCmd);
        		Print(OUTPUT_WINDOW,":");
				goto CommonParseReturnPoint;
			}
            else
            {
                PICE_sprintf(tempCmd,"  <-- %s has no parameters\n",pCurrentCommand->Cmd);
		        Print(OUTPUT_WINDOW,tempCmd);
        		Print(OUTPUT_WINDOW,":");
                goto CommonParseReturnPoint;
            }
		    // next token
		    i++;
        }
	}while(pToken && i<MAX_ARGS);

	Arguments.Count=i;
	Arguments.CountSwitches=j;

	if(pCurrentCommand)
	{
        DPRINT((0,"Parse(): command %s switches %u\n",pCurrentCommand->Cmd,Arguments.CountSwitches));

        if(!bInvokedByFkey)
        {
            DPRINT((0,"Parse(): adding new line\n"));
			Print(OUTPUT_WINDOW,"\n");
        }

        // call the command handler
		result=pCurrentCommand->Handler(&Arguments);

		if(result && !bInvokedByFkey && pCurrentCommand->Handler!=LeaveIce && pCurrentCommand->Handler!=SingleStep )
        {
            DPRINT((0,"Parse(): adding colon\n"));
			Print(OUTPUT_WINDOW,":");
        }
	}

CommonParseReturnPoint:
    SuspendPrintRingBuffer(FALSE);
	PrintRingBuffer(wWindow[OUTPUT_WINDOW].cy-1);

    ShowStatusLine();
}
