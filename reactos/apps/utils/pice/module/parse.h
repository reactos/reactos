/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    parse.h

Abstract:

    HEADER for parse.c

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
typedef struct TagArgs
{
	ULONG Value[16];
	ULONG Count;
	UCHAR Switch[16];
	ULONG CountSwitches;
    BOOLEAN bNotTranslated[16];
	LPSTR pToken[16];
}ARGS,*PARGS;

typedef struct tagCPUInfo
{
	char *Name;
	PULONG pValue;
}CPUINFO;

typedef BOOLEAN (*PFN)(PARGS);

#define MAX_ARGS (5)

typedef struct _CMDTABLE
{
	char * Cmd;
	PFN Handler;
	char * Help;
    ULONG Flags;
    ULONG ParamFlags[MAX_ARGS];
    LPSTR pszRecognizedSwitches;
    ULONG CommandGroup;
}CMDTABLE,*PCMDTABLE;

typedef struct tagKeyWords
{
	char* KeyWord;
	PVOID pValue;
	ULONG ulSize;
}KEYWORDS;

typedef struct tagSETGETREGS
{
	char *RegName;
	PULONG RegAddr;
}SETGETREGS;

typedef struct tag_BP
{
	ULONG LinearAddress;
	ULONG Segment,Offset;
	BOOLEAN Used;
	BOOLEAN Active;
	BOOLEAN Virtual;
	char ModName[256];
	char SymName[256];
}BP;

extern BOOLEAN bNeedToFillBuffer;

extern BOOLEAN bCodeOn;
extern BOOLEAN bShowSrc;
extern BP Bp[];
extern BOOLEAN bInt3Here;

extern BOOLEAN bStepping;

extern char szCurrentFile[256];
extern struct module* pCurrentMod;
extern PICE_SYMBOLFILE_HEADER* pCurrentSymbols;
extern LONG ulCurrentlyDisplayedLineNumber;

BOOLEAN AsciiToHex(LPSTR p,PULONG pValue);
void Parse(LPSTR pCmdLine,BOOLEAN bInvokedByFkey);
LPSTR FindCommand(LPSTR p);
ULONG StrLenUpToWhiteChar(LPSTR p,LPSTR lpszWhiteChars);
BOOLEAN WaitForKey(void);
BOOLEAN ConvertTokenToHex(LPSTR p,PULONG pValue);
void DisplaySourceFile(LPSTR pSrcLine,LPSTR pSrcEnd,ULONG ulLineNumber,ULONG ulLineNumberToInvert);
BOOLEAN ConvertTokenToSrcFile(LPSTR p,PULONG pValue);
void RepaintDesktop(void);
void PutStatusText(LPSTR p);
void UnassembleOneLineDown(void);
void UnassembleOnePageDown(ULONG page);
void UnassembleOneLineUp(void);
void UnassembleOnePageUp(ULONG page);

extern BOOLEAN (*DisplayMemory)(PARGS pArgs);

#define COMMAND_PROTOTYPE(arg)  BOOLEAN arg(PARGS pArgs)
#define COMMAND_RET             return TRUE

// available commands
COMMAND_PROTOTYPE(ShowGdt);
COMMAND_PROTOTYPE(LeaveIce);
COMMAND_PROTOTYPE(SingleStep);
COMMAND_PROTOTYPE(ShowHelp);
COMMAND_PROTOTYPE(ShowPageDirs);
COMMAND_PROTOTYPE(ShowProcesses);
COMMAND_PROTOTYPE(DisplayMemoryDword);
COMMAND_PROTOTYPE(DisplayMemoryByte);
COMMAND_PROTOTYPE(DisplayPhysMemDword);
COMMAND_PROTOTYPE(Unassemble);
COMMAND_PROTOTYPE(ShowSymbols);
COMMAND_PROTOTYPE(ShowModules);
COMMAND_PROTOTYPE(SetBreakpoint);
COMMAND_PROTOTYPE(ListBreakpoints);
COMMAND_PROTOTYPE(ClearBreakpoints);
COMMAND_PROTOTYPE(Ver);
COMMAND_PROTOTYPE(Hboot);
COMMAND_PROTOTYPE(I3here);
COMMAND_PROTOTYPE(I1here);
COMMAND_PROTOTYPE(SetSrcDisplay);
COMMAND_PROTOTYPE(ShowIdt);
COMMAND_PROTOTYPE(StepOver);
COMMAND_PROTOTYPE(StepInto);
COMMAND_PROTOTYPE(SetGetRegisters);
COMMAND_PROTOTYPE(SetCodeDisplay);
COMMAND_PROTOTYPE(NextInstr);
COMMAND_PROTOTYPE(ShowCPU);
COMMAND_PROTOTYPE(ShowTables);
COMMAND_PROTOTYPE(WalkStack);
COMMAND_PROTOTYPE(ShowVirtualMemory);
COMMAND_PROTOTYPE(UnassembleAtCurrentEip);
COMMAND_PROTOTYPE(PokeMemory);
COMMAND_PROTOTYPE(PeekMemory);
COMMAND_PROTOTYPE(ShowLocals);
COMMAND_PROTOTYPE(SwitchTables);
COMMAND_PROTOTYPE(SwitchFiles);
COMMAND_PROTOTYPE(EvaluateExpression);
COMMAND_PROTOTYPE(SizeCodeWindow);
COMMAND_PROTOTYPE(SizeDataWindow);
COMMAND_PROTOTYPE(ClearScreen);
COMMAND_PROTOTYPE(ShowMappings);
COMMAND_PROTOTYPE(ShowTimers);
COMMAND_PROTOTYPE(ShowPCI);
COMMAND_PROTOTYPE(SetKeyboardLayout);
COMMAND_PROTOTYPE(ShowSysCallTable);
COMMAND_PROTOTYPE(SetAltKey);
COMMAND_PROTOTYPE(ShowContext);