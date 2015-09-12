/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            dem.c
 * PURPOSE:         DOS 32-bit Emulation Support Library -
 *                  This library is used by the built-in NTVDM DOS32 and by
 *                  the NT 16-bit DOS in Windows (via BOPs). It also exposes
 *                  exported functions that can be used by VDDs.
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "ntvdm.h"
#include "emulator.h"
#include <isvbop.h>

#include "utils.h"

#include "dem.h"
#include "dos/dos32krnl/device.h"
#include "dos/dos32krnl/memory.h"
#include "dos/dos32krnl/process.h"
#include "cpu/bop.h"
#include "cpu/cpu.h"

#include "bios/bios.h"
#include "mouse32.h"

#include "vddsup.h"

/*
 * EXPERIMENTAL!
 * Activate this line if you want to have COMMAND.COM completely external.
 */
// #define COMSPEC_FULLY_EXTERNAL

/* PRIVATE VARIABLES **********************************************************/

/* PRIVATE FUNCTIONS **********************************************************/

/* PUBLIC VARIABLES ***********************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/


/******************************************************************************\
|**                          DOS DEM Kernel helpers                          **|
\******************************************************************************/


VOID BiosCharPrint(CHAR Character)
{
    /* Save AX and BX */
    USHORT AX = getAX();
    USHORT BX = getBX();

    /*
     * Set the parameters:
     * AL contains the character to print,
     * BL contains the character attribute,
     * BH contains the video page to use.
     */
    setAL(Character);
    setBL(DEFAULT_ATTRIBUTE);
    setBH(Bda->VideoPage);

    /* Call the BIOS INT 10h, AH=0Eh "Teletype Output" */
    setAH(0x0E);
    Int32Call(&DosContext, BIOS_VIDEO_INTERRUPT);

    /* Restore AX and BX */
    setBX(BX);
    setAX(AX);
}

VOID DosCharPrint(CHAR Character)
{
    DosPrintCharacter(DOS_OUTPUT_HANDLE, Character);
}

/*
 * This function, derived from ntvdm.c!DisplayMessage, is used by the BIOS and
 * the DOS to display messages to an output device. A printer function is given
 * for printing the characters.
 */
static VOID
DisplayMessageAnsiV(IN CHAR_PRINT CharPrint,
                    IN LPCSTR Format,
                    IN va_list args)
{
    static CHAR CurChar = 0;
    LPSTR str;

#ifndef WIN2K_COMPLIANT
    CHAR  StaticBuffer[256];
    LPSTR Buffer = StaticBuffer; // Use the static buffer by default.
#else
    CHAR  Buffer[2048]; // Large enough. If not, increase it by hand.
#endif
    size_t MsgLen;

#ifndef WIN2K_COMPLIANT
    /*
     * Retrieve the message length and if it is too long, allocate
     * an auxiliary buffer; otherwise use the static buffer.
     * The string is built to be NULL-terminated.
     */
    MsgLen = _vscprintf(Format, args);
    if (MsgLen >= ARRAYSIZE(StaticBuffer))
    {
        Buffer = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, (MsgLen + 1) * sizeof(CHAR));
        if (Buffer == NULL)
        {
            /* Allocation failed, use the static buffer and display a suitable error message */
            Buffer = StaticBuffer;
            Format = "DisplayMessageAnsi()\nOriginal message is too long and allocating an auxiliary buffer failed.";
            MsgLen = strlen(Format);
        }
    }
#else
    MsgLen = ARRAYSIZE(Buffer) - 1;
#endif

    RtlZeroMemory(Buffer, (MsgLen + 1) * sizeof(CHAR));
    _vsnprintf(Buffer, MsgLen, Format, args);

    /* Display the message */
    DPRINT1("\n\nNTVDM DOS32\n%s\n\n", Buffer);

    MsgLen = strlen(Buffer);
    str = Buffer;
    while (MsgLen--)
    {
        if (*str == '\n' && CurChar != '\r')
            CharPrint('\r');

        CurChar = *str++;
        CharPrint(CurChar);
    }

#ifndef WIN2K_COMPLIANT
    /* Free the buffer if needed */
    if (Buffer != StaticBuffer) RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
#endif
}

VOID
DemDisplayMessage(IN CHAR_PRINT CharPrint,
                  IN LPCSTR Format, ...)
{
    va_list Parameters;

    va_start(Parameters, Format);
    DisplayMessageAnsiV(CharPrint, Format, Parameters);
    va_end(Parameters);
}


static VOID DemLoadNTDOSKernel(VOID)
{
    BOOLEAN Success = FALSE;
    LPCSTR  DosKernelFileName = "ntdos.sys";
    HANDLE  hDosKernel;
    ULONG   ulDosKernelSize = 0;

    DPRINT1("You are loading Windows NT DOS!\n");

    /* Open the DOS kernel file */
    hDosKernel = FileOpen(DosKernelFileName, &ulDosKernelSize);
    if (hDosKernel == NULL) goto Quit;

    /*
     * Attempt to load the DOS kernel into memory.
     * The segment where to load the DOS kernel is defined
     * by the DOS BIOS and is found in DI:0000 .
     */
    Success = FileLoadByHandle(hDosKernel,
                               REAL_TO_PHYS(TO_LINEAR(getDI(), 0x0000)),
                               ulDosKernelSize,
                               &ulDosKernelSize);

    DPRINT1("Windows NT DOS file '%s' loading %s at %04X:%04X, size 0x%X (Error: %u).\n",
            DosKernelFileName,
            (Success ? "succeeded" : "failed"),
            getDI(), 0x0000,
            ulDosKernelSize,
            GetLastError());

    /* Close the DOS kernel file */
    FileClose(hDosKernel);

Quit:
    if (!Success)
    {
        /* We failed everything, stop the VDM */
        BiosDisplayMessage("Windows NT DOS kernel file '%s' loading failed (Error: %u). The VDM will shut down.\n",
                           DosKernelFileName, GetLastError());
        EmulatorTerminate();
        return;
    }
}

static VOID WINAPI DosSystemBop(LPWORD Stack)
{
    /* Get the Function Number and skip it */
    BYTE FuncNum = *(PBYTE)SEG_OFF_TO_PTR(getCS(), getIP());
    setIP(getIP() + 1);

    switch (FuncNum)
    {
        /* Load the DOS kernel */
        case 0x11:
        {
            DemLoadNTDOSKernel();
            break;
        }

        /* Call 32-bit Driver Strategy Routine */
        case BOP_DRV_STRATEGY:
        {
            DeviceStrategyBop();
            break;
        }

        /* Call 32-bit Driver Interrupt Routine */
        case BOP_DRV_INTERRUPT:
        {
            DeviceInterruptBop();
            break;
        }

        default:
        {
            DPRINT1("Unknown DOS System BOP Function: 0x%02X\n", FuncNum);
            // setCF(1); // Disable, otherwise we enter an infinite loop
            break;
        }
    }
}




/******************************************************************************\
|**                      DOS Command Process management                      **|
\******************************************************************************/


#ifndef STANDALONE
static ULONG SessionId = 0;

/*
 * 16-bit Command Interpreter information for DOS reentry
 */
typedef struct _COMSPEC_INFO
{
    LIST_ENTRY Entry;
    DWORD dwExitCode;
    WORD ComSpecPsp;
    BOOLEAN Terminated;
} COMSPEC_INFO, *PCOMSPEC_INFO;

static COMSPEC_INFO RootCmd;
static DWORD ReentrancyCount = 0;

// FIXME: Should we need list locking?
static LIST_ENTRY ComSpecInfoList = { &ComSpecInfoList, &ComSpecInfoList };

static PCOMSPEC_INFO
FindComSpecInfoByPsp(WORD Psp)
{
    PLIST_ENTRY Pointer;
    PCOMSPEC_INFO ComSpecInfo;

    for (Pointer = ComSpecInfoList.Flink; Pointer != &ComSpecInfoList; Pointer = Pointer->Flink)
    {
        ComSpecInfo = CONTAINING_RECORD(Pointer, COMSPEC_INFO, Entry);
        if (ComSpecInfo->ComSpecPsp == Psp) return ComSpecInfo;
    }

    return NULL;
}

static VOID
InsertComSpecInfo(PCOMSPEC_INFO ComSpecInfo)
{
    InsertHeadList(&ComSpecInfoList, &ComSpecInfo->Entry);
}

static VOID
RemoveComSpecInfo(PCOMSPEC_INFO ComSpecInfo)
{
    RemoveEntryList(&ComSpecInfo->Entry);
    if (ComSpecInfo != &RootCmd)
        RtlFreeHeap(RtlGetProcessHeap(), 0, ComSpecInfo);
}
#endif

static VOID DosProcessConsoleAttach(VOID)
{
    /* Attach to the console */
    ConsoleAttach();
    VidBiosAttachToConsole();
}

static VOID DosProcessConsoleDetach(VOID)
{
    /* Detach from the console */
    VidBiosDetachFromConsole();
    ConsoleDetach();
}

/*
 * Data for the next DOS command to run
 */
#ifndef STANDALONE
static VDM_COMMAND_INFO CommandInfo;
static BOOLEAN Repeat = FALSE;
static BOOLEAN Reentry = FALSE;
#endif
static BOOLEAN First = TRUE;
static CHAR CmdLine[MAX_PATH] = ""; // DOS_CMDLINE_LENGTH
static CHAR AppName[MAX_PATH] = "";
#ifndef STANDALONE
static CHAR PifFile[MAX_PATH] = "";
static CHAR CurDirectory[MAX_PATH] = "";
static CHAR Desktop[MAX_PATH] = "";
static CHAR Title[MAX_PATH] = "";
static ULONG EnvSize = 256;
static PVOID Env = NULL;
#endif

#pragma pack(push, 2)

/*
 * This structure is compatible with Windows NT DOS
 */
typedef struct _NEXT_CMD
{
    USHORT EnvBlockSeg;
    USHORT EnvBlockLen;
    USHORT CurDrive;
    USHORT NumDrives;
    USHORT CmdLineSeg;
    USHORT CmdLineOff;
    USHORT Unknown0;
    USHORT ExitCode;
    USHORT Unknown1;
    ULONG  Unknown2;
    USHORT CodePage;
    USHORT Unknown3;
    USHORT Unknown4;
    USHORT AppNameSeg;
    USHORT AppNameOff;
    USHORT AppNameLen;
    USHORT Flags;
} NEXT_CMD, *PNEXT_CMD;

#pragma pack(pop)

static VOID CmdStartProcess(VOID)
{
#ifndef STANDALONE
    PCOMSPEC_INFO ComSpecInfo;
#endif
    SIZE_T CmdLen;
    PNEXT_CMD DataStruct = (PNEXT_CMD)SEG_OFF_TO_PTR(getDS(), getDX());

    DPRINT1("CmdStartProcess -- DS:DX = %04X:%04X (DataStruct = 0x%p)\n",
            getDS(), getDX(), DataStruct);

    /* Pause the VM */
    EmulatorPause();

#ifndef STANDALONE
    /* Check whether we need to shell out now in case we were started by a 32-bit app */
    ComSpecInfo = FindComSpecInfoByPsp(Sda->CurrentPsp);
    if (ComSpecInfo && ComSpecInfo->Terminated)
    {
        RemoveComSpecInfo(ComSpecInfo);

        DPRINT1("Exit DOS from start-app BOP\n");
        setCF(1);
        goto Quit;
    }

    /* Clear the structure */
    RtlZeroMemory(&CommandInfo, sizeof(CommandInfo));

    /* Initialize the structure members */
    CommandInfo.TaskId = SessionId;
    CommandInfo.VDMState = VDM_FLAG_DOS;
    CommandInfo.CmdLine = CmdLine;
    CommandInfo.CmdLen = sizeof(CmdLine);
    CommandInfo.AppName = AppName;
    CommandInfo.AppLen = sizeof(AppName);
    CommandInfo.PifFile = PifFile;
    CommandInfo.PifLen = sizeof(PifFile);
    CommandInfo.CurDirectory = CurDirectory;
    CommandInfo.CurDirectoryLen = sizeof(CurDirectory);
    CommandInfo.Desktop = Desktop;
    CommandInfo.DesktopLen = sizeof(Desktop);
    CommandInfo.Title = Title;
    CommandInfo.TitleLen = sizeof(Title);
    CommandInfo.Env = Env;
    CommandInfo.EnvLen = EnvSize;

    if (First) CommandInfo.VDMState |= VDM_FLAG_FIRST_TASK;

Command:

    if (Repeat) CommandInfo.VDMState |= VDM_FLAG_RETRY;
    Repeat = FALSE;

    /* Get the VDM command information */
    DPRINT1("Calling GetNextVDMCommand in CmdStartProcess: wait for new VDM task...\n");
    if (!GetNextVDMCommand(&CommandInfo))
    {
        DPRINT1("CmdStartProcess - GetNextVDMCommand failed, retrying... last error = %d\n", GetLastError());
        if (CommandInfo.EnvLen > EnvSize)
        {
            /* Expand the environment size */
            EnvSize = CommandInfo.EnvLen;
            CommandInfo.Env = Env = RtlReAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, Env, EnvSize);

            /* Repeat the request */
            Repeat = TRUE;
            goto Command;
        }

        /* Shouldn't happen */
        DisplayMessage(L"An unrecoverable failure happened from start-app BOP; exiting DOS.");
        setCF(1);
        goto Quit;
    }

    // FIXME: What happens if some other 32-bit app is killed while we are waiting there??

    DPRINT1("CmdStartProcess - GetNextVDMCommand succeeded, start app...\n");

#else

    if (!First)
    {
        DPRINT1("Exit DOS from start-app BOP\n");
        setCF(1);
        goto Quit;
    }

#endif

    CmdLen = strlen(CmdLine);
    DPRINT1("Starting '%s' ('%.*s')...\n",
            AppName,
            /* Display the command line without the terminating 0d 0a (and skip the terminating NULL) */
            CmdLen >= 2 ? (CmdLine[CmdLen - 2] == '\r' ? CmdLen - 2
                                                       : CmdLen)
                        : CmdLen,
            CmdLine);

    /* Start the process */
    // FIXME: Merge 'Env' with the master environment SEG_OFF_TO_PTR(SYSTEM_ENV_BLOCK, 0)
    // FIXME: Environment
    RtlCopyMemory(SEG_OFF_TO_PTR(DataStruct->AppNameSeg, DataStruct->AppNameOff), AppName, MAX_PATH);
    *(PBYTE)(SEG_OFF_TO_PTR(DataStruct->CmdLineSeg, DataStruct->CmdLineOff)) = (BYTE)(strlen(CmdLine) - 2);
    RtlCopyMemory(SEG_OFF_TO_PTR(DataStruct->CmdLineSeg, DataStruct->CmdLineOff + 1), CmdLine, DOS_CMDLINE_LENGTH);

#ifndef STANDALONE
    /* Update console title if we run in a separate console */
    if (SessionId != 0)
        SetConsoleTitleA(AppName);
#endif

    First = FALSE;
    setCF(0);

    DPRINT1("App started!\n");

Quit:
    /* Resume the VM */
    EmulatorResume();
}

static VOID CmdStartExternalCommand(VOID)
{
    DWORD Result;

    // TODO: improve: this code has strong similarities
    // with the 'default' case of DosCreateProcess.

    LPSTR Command = (LPSTR)SEG_OFF_TO_PTR(getDS(), getSI());
    CHAR CmdLine[sizeof("cmd.exe /c ") + DOS_CMDLINE_LENGTH + 1] = "";
    LPSTR CmdLinePtr;
    ULONG CmdLineLen;

    /* Spawn a user-defined 32-bit command preprocessor */

    // FIXME: Use COMSPEC env var!!
    CmdLinePtr = CmdLine;
    strcpy(CmdLinePtr, "cmd.exe /c ");
    CmdLinePtr += strlen(CmdLinePtr);

    /* Build a Win32-compatible command-line */
    CmdLineLen = min(strlen(Command), sizeof(CmdLine) - strlen(CmdLinePtr) - 1);
    RtlCopyMemory(CmdLinePtr, Command, CmdLineLen);
    CmdLinePtr[CmdLineLen] = '\0';

    /* Remove any trailing return carriage character and NULL-terminate the command line */
    while (*CmdLinePtr && *CmdLinePtr != '\r' && *CmdLinePtr != '\n') CmdLinePtr++;
    *CmdLinePtr = '\0';

    DPRINT1("CMD Run Command '%s' ('%s')\n", Command, CmdLine);

    /*
     * No need to prepare the stack for DosStartComSpec since we won't start it.
     */
    Result = DosStartProcess32(Command, CmdLine,
                               SEG_OFF_TO_PTR(getES(), 0) /*Environment*/,
                               MAKELONG(getIP(), getCS()) /*ReturnAddress*/,
                               FALSE);
    if (Result != ERROR_SUCCESS)
    {
        DosDisplayMessage("Failed to start command '%s' ('%s'). Error: %u\n", Command, CmdLine, Result);
        setCF(0);
        setAL((UCHAR)Result);
    }
    else
    {
        DosDisplayMessage("Command '%s' ('%s') started successfully.\n", Command, CmdLine);
#ifndef STANDALONE
        setCF(Repeat); // Set CF if we need to start a 16-bit process
#else
        setCF(0);
#endif
    }
}

static VOID CmdStartComSpec32(VOID)
{
    DWORD Result;

    // TODO: improve: this code has strong similarities with the
    // 'default' case of DosCreateProcess and with the 'case 0x08'.

    CHAR CmdLine[sizeof("cmd.exe") + 1] = "";

    /* Spawn a user-defined 32-bit command preprocessor */

    // FIXME: Use COMSPEC env var!!
    strcpy(CmdLine, "cmd.exe");

    DPRINT1("CMD Run 32-bit Command Interpreter '%s'\n", CmdLine);

    /*
     * No need to prepare the stack for DosStartComSpec since we won't start it.
     */
    Result = DosStartProcess32(CmdLine, CmdLine,
                               SEG_OFF_TO_PTR(getES(), 0) /*Environment*/,
                               MAKELONG(getIP(), getCS()) /*ReturnAddress*/,
                               FALSE);
    if (Result != ERROR_SUCCESS)
    {
        DosDisplayMessage("Failed to start 32-bit Command Interpreter '%s'. Error: %u\n", CmdLine, Result);
        setCF(0);
        setAL((UCHAR)Result);
    }
    else
    {
        DosDisplayMessage("32-bit Command Interpreter '%s' started successfully.\n", CmdLine);
#ifndef STANDALONE
        setCF(Repeat); // Set CF if we need to start a 16-bit process
#else
        setCF(0);
#endif
    }
}

static VOID CmdSetExitCode(VOID)
{
#ifndef STANDALONE
    BOOL Success;
    PCOMSPEC_INFO ComSpecInfo;
    VDM_COMMAND_INFO CommandInfo;
#endif

    /* Pause the VM */
    EmulatorPause();

#ifndef STANDALONE
    /*
     * Check whether we need to shell out now in case we were started by a 32-bit app,
     * or we were started alone along with the root 32-bit app.
     */
    ComSpecInfo = FindComSpecInfoByPsp(Sda->CurrentPsp);
    if ((ComSpecInfo && ComSpecInfo->Terminated) ||
        (ComSpecInfo == &RootCmd && SessionId != 0))
    {
        RemoveComSpecInfo(ComSpecInfo);
#endif
        DPRINT1("Exit DOS from ExitCode (prologue)!");
        setCF(0);
        goto Quit;
#ifndef STANDALONE
    }

    /* Clear the VDM structure */
    RtlZeroMemory(&CommandInfo, sizeof(CommandInfo));

Retry:
    /* Update the VDM state of the task */
    // CommandInfo.TaskId = SessionId;
    CommandInfo.ExitCode = getDX();
    CommandInfo.VDMState = VDM_FLAG_DONT_WAIT;
    DPRINT1("Calling GetNextVDMCommand 32bit end of VDM task\n");
    Success = GetNextVDMCommand(&CommandInfo);
    DPRINT1("GetNextVDMCommand 32bit end of VDM task success = %s, last error = %d\n", Success ? "true" : "false", GetLastError());

    /*
     * Check whether we were awaited because the 32-bit process was stopped,
     * or because it started a new DOS application.
     */
    if (CommandInfo.CmdLen != 0 || CommandInfo.AppLen != 0 || CommandInfo.PifLen != 0)
    {
        DPRINT1("GetNextVDMCommand end-of-app, this is for a new VDM task - CmdLen = %d, AppLen = %d, PifLen = %d\n",
                CommandInfo.CmdLen, CommandInfo.AppLen, CommandInfo.PifLen);

        /* Repeat the request */
        Repeat = TRUE;
        setCF(1);
    }
    else
    {
        DPRINT1("GetNextVDMCommand end-of-app, the app stopped\n");

        /* Check whether we need to shell out now in case we were started by a 32-bit app */
        ComSpecInfo = FindComSpecInfoByPsp(Sda->CurrentPsp);
        if (!ComSpecInfo || !ComSpecInfo->Terminated)
        {
            DPRINT1("Not our 32-bit app, retrying...\n");
            goto Retry;
        }

        ASSERT(ComSpecInfo->Terminated == TRUE);

        /* Record found, remove it and exit now */
        RemoveComSpecInfo(ComSpecInfo);

        DPRINT1("Exit DOS from ExitCode wait!\n");
        setCF(0);
    }
#endif

    // FIXME: Use the retrieved exit code as the value of our exit code
    // when COMMAND.COM will shell-out ??

Quit:
    /* Resume the VM */
    EmulatorResume();
}

static VOID WINAPI DosCmdInterpreterBop(LPWORD Stack)
{
    /* Get the Function Number and skip it */
    BYTE FuncNum = *(PBYTE)SEG_OFF_TO_PTR(getCS(), getIP());
    setIP(getIP() + 1);

    switch (FuncNum)
    {
        /* Kill the VDM */
        case 0x00:
        {
            /* Stop the VDM */
            EmulatorTerminate();
            return;
        }

        /*
         * Get a new app to start
         *
         * Input
         *     DS:DX : Data block.
         *
         * Output
         *     CF    : 0: Success; 1: Failure.
         */
        case 0x01:
        {
            CmdStartProcess();
            break;
        }

        /*
         * Check binary format
         *
         * Input
         *     DS:DX : Program to check.
         *
         * Output
         *     CF    : 0: Success; 1: Failure.
         *     AX    : Error code.
         */
        case 0x07:
        {
            DWORD BinaryType;
            LPSTR ProgramName = (LPSTR)SEG_OFF_TO_PTR(getDS(), getDX());

            if (!GetBinaryTypeA(ProgramName, &BinaryType))
            {
                /* An error happened, bail out */
                setCF(1);
                setAX(LOWORD(GetLastError()));
                break;
            }

            // FIXME: We only support DOS binaries for now...
            ASSERT(BinaryType == SCS_DOS_BINARY);
            if (BinaryType != SCS_DOS_BINARY)
            {
                /* An error happened, bail out */
                setCF(1);
                setAX(LOWORD(ERROR_BAD_EXE_FORMAT));
                break;
            }

            /* Return success: DOS application */
            setCF(0);
            break;
        }

        /*
         * Start an external command
         *
         * Input
         *     DS:SI : Command to start.
         *     ES    : Environment block segment.
         *     AL    : Current drive number.
         *     AH    : 0: Directly start the command;
         *             1: Use "cmd.exe /c" to start the command.
         *
         * Output
         *     CF    : 0: Shell-out; 1: Continue.
         *     AL    : Error/Exit code.
         */
        case 0x08:
        {
            CmdStartExternalCommand();
            break;
        }

        /*
         * Start the default 32-bit command interpreter (COMSPEC)
         *
         * Input
         *     ES    : Environment block segment.
         *     AL    : Current drive number.
         *
         * Output
         *     CF    : 0: Shell-out; 1: Continue.
         *     AL    : Error/Exit code.
         */
        case 0x0A:
        {
            CmdStartComSpec32();
            break;
        }

        /*
         * Set exit code
         *
         * Input
         *     DX    : Exit code
         *
         * Output
         *     CF    : 0: Shell-out; 1: Continue.
         */
        case 0x0B:
        {
            CmdSetExitCode();
            break;
        }

        /*
         * Get start information
         *
         * Output
         *     AL    : 0 (resp. 1): Started from (resp. without) an existing console.
         */
        case 0x10:
        {
#ifndef STANDALONE
            /*
             * When a new instance of our (internal) COMMAND.COM is started,
             * we check whether we need to run a 32-bit COMSPEC. This goes by
             * checking whether we were started in a new console (no parent
             * console process) or from an existing one.
             *
             * However COMMAND.COM can also be started in the case where a
             * 32-bit process (started by a 16-bit parent) wants to start a new
             * 16-bit process: to ensure DOS reentry we need to start a new
             * instance of COMMAND.COM. On Windows the COMMAND.COM is started
             * just before the 32-bit process (in fact, it is this COMMAND.COM
             * which starts the 32-bit process via an undocumented command-line
             * switch '/z', which syntax is:
             *     COMMAND.COM /z\bAPPNAME.EXE
             * notice the '\b' character inserted in-between. Then COMMAND.COM
             * issues a BOP_CMD 08h with AH=00h to start the process).
             *
             * Instead, we do the reverse, i.e. we start the 32-bit process,
             * and *only* if needed, i.e. if this process wants to start a
             * new 16-bit process, we start our COMMAND.COM.
             *
             * The problem we then face is that our COMMAND.COM will possibly
             * want to start a new COMSPEC, however we do not want this.
             * The chosen solution is to flag this case -- done with the 'Reentry'
             * boolean -- so that COMMAND.COM will not attempt to start COMSPEC
             * but instead will directly try to start the 16-bit process.
             */
            // setAL(SessionId != 0);
            setAL((SessionId != 0) && !Reentry);
            /* Reset 'Reentry' */
            Reentry = FALSE;
#else
            setAL(0);
#endif
            break;
        }

        default:
        {
            DPRINT1("Unknown DOS CMD Interpreter BOP Function: 0x%02X\n", FuncNum);
            // setCF(1); // Disable, otherwise we enter an infinite loop
            break;
        }
    }
}

#ifndef COMSPEC_FULLY_EXTERNAL
/*
 * Internal COMMAND.COM binary data in the CommandCom array.
 */
#include "command_com.h"
#endif

static
DWORD DosStartComSpec(IN BOOLEAN Permanent,
                      IN LPCSTR Environment OPTIONAL,
                      IN DWORD ReturnAddress OPTIONAL,
                      OUT PWORD ComSpecPsp OPTIONAL)
{
    /*
     * BOOLEAN Permanent
     *   TRUE to simulate the /P switch of command.com: starts AUTOEXEC.BAT/NT
     *   and makes the interpreter permanent (cannot exit).
     */

    DWORD Result;

    if (ComSpecPsp) *ComSpecPsp = 0;

    Result =
#ifndef COMSPEC_FULLY_EXTERNAL
    DosLoadExecutableInternal(DOS_LOAD_AND_EXECUTE,
                              CommandCom,
                              sizeof(CommandCom),
                              "COMMAND.COM",
#else
            DosLoadExecutable(DOS_LOAD_AND_EXECUTE,
#ifndef STANDALONE  // FIXME: Those values are hardcoded paths on my local test machines!!
                              "C:\\CMDCMD.COM",
#else
                              "H:\\DOS_tests\\CMDCMD.COM",
#endif // STANDALONE
#endif // COMSPEC_FULLY_EXTERNAL
                              NULL,
                              Permanent ? "/P" : "",
                              Environment ? Environment : "", // FIXME: Default environment!
                              ReturnAddress);
    if (Result != ERROR_SUCCESS) return Result;

    /* TODO: Read AUTOEXEC.NT/BAT */

    /* Retrieve the PSP of the COMSPEC (current PSP set by DosLoadExecutable) */
    if (ComSpecPsp) *ComSpecPsp = Sda->CurrentPsp;

    return Result;
}

typedef struct _DOS_START_PROC32
{
    LPSTR ExecutablePath;
    LPSTR CommandLine;
    LPSTR Environment OPTIONAL;
#ifndef STANDALONE
    PCOMSPEC_INFO ComSpecInfo;
    HANDLE hEvent;
#endif
} DOS_START_PROC32, *PDOS_START_PROC32;

static DWORD
WINAPI
CommandThreadProc(LPVOID Parameter)
{
    BOOL Success;
    PROCESS_INFORMATION ProcessInfo;
    STARTUPINFOA StartupInfo;
    DWORD dwExitCode;
    PDOS_START_PROC32 DosStartProc32 = (PDOS_START_PROC32)Parameter;
#ifndef STANDALONE
    VDM_COMMAND_INFO CommandInfo;
    PCOMSPEC_INFO ComSpecInfo = DosStartProc32->ComSpecInfo;
#endif

    /* Set up the VDM, startup and process info structures */
#ifndef STANDALONE
    RtlZeroMemory(&CommandInfo, sizeof(CommandInfo));
#endif
    RtlZeroMemory(&ProcessInfo, sizeof(ProcessInfo));
    RtlZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);

    // FIXME: Build suitable 32-bit environment!!

#ifndef STANDALONE
    /*
     * Wait for signaling a new VDM task and increment the VDM re-entry count so
     * that we can handle 16-bit apps that may be possibly started by the 32-bit app.
     */
    CommandInfo.VDMState = VDM_INC_REENTER_COUNT;
    DPRINT1("Calling GetNextVDMCommand reenter++\n");
    Success = GetNextVDMCommand(&CommandInfo);
    DPRINT1("GetNextVDMCommand reenter++ success = %s, last error = %d\n", Success ? "true" : "false", GetLastError());
    ++ReentrancyCount;
#endif

    /* Start the process */
    Success = CreateProcessA(NULL, // ProgramName,
                             DosStartProc32->CommandLine,
                             NULL,
                             NULL,
                             TRUE, // Inherit handles
                             CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED,
                             DosStartProc32->Environment,
                             NULL, // lpCurrentDirectory, see "START" command in cmd.exe
                             &StartupInfo,
                             &ProcessInfo);

#ifndef STANDALONE
    /* Signal our caller the process was started */
    SetEvent(DosStartProc32->hEvent);
    // After this point, 'DosStartProc32' is not valid anymore.
#endif

    if (Success)
    {
        /* Resume the process */
        ResumeThread(ProcessInfo.hThread);

        /* Wait for the process to finish running and retrieve its exit code */
        WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
        GetExitCodeProcess(ProcessInfo.hProcess, &dwExitCode);

        /* Close the handles */
        CloseHandle(ProcessInfo.hThread);
        CloseHandle(ProcessInfo.hProcess);
    }
    else
    {
        dwExitCode = GetLastError();
    }

#ifndef STANDALONE
    ASSERT(ComSpecInfo);
    ComSpecInfo->Terminated = TRUE;
    ComSpecInfo->dwExitCode = dwExitCode;

    /* Decrement the VDM re-entry count */
    CommandInfo.VDMState = VDM_DEC_REENTER_COUNT;
    DPRINT1("Calling GetNextVDMCommand reenter--\n");
    Success = GetNextVDMCommand(&CommandInfo);
    DPRINT1("GetNextVDMCommand reenter-- success = %s, last error = %d\n", Success ? "true" : "false", GetLastError());
    --ReentrancyCount;

    return 0;
#else
    return dwExitCode;
#endif
}

DWORD DosStartProcess32(IN LPCSTR ExecutablePath,
                        IN LPCSTR CommandLine,
                        IN LPCSTR Environment OPTIONAL,
                        IN DWORD ReturnAddress OPTIONAL,
                        IN BOOLEAN StartComSpec)
{
    DWORD Result = ERROR_SUCCESS;
    HANDLE CommandThread;
    DOS_START_PROC32 DosStartProc32;
#ifndef STANDALONE
    BOOL Success;
    VDM_COMMAND_INFO CommandInfo;
#endif

    DosStartProc32.ExecutablePath = (LPSTR)ExecutablePath;
    DosStartProc32.CommandLine    = (LPSTR)CommandLine;
    DosStartProc32.Environment    = (LPSTR)Environment;

#ifndef STANDALONE
    DosStartProc32.ComSpecInfo =
        RtlAllocateHeap(RtlGetProcessHeap(),
                        HEAP_ZERO_MEMORY,
                        sizeof(*DosStartProc32.ComSpecInfo));
    ASSERT(DosStartProc32.ComSpecInfo);

    DosStartProc32.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    ASSERT(DosStartProc32.hEvent);
#endif

    /* Pause the VM and detach from the console */
    EmulatorPause();
    DosProcessConsoleDetach();

    /* Start the 32-bit process via another thread */
    CommandThread = CreateThread(NULL, 0, &CommandThreadProc, &DosStartProc32, 0, NULL);
    if (CommandThread == NULL)
    {
        DisplayMessage(L"FATAL: Failed to create the command processing thread: %d", GetLastError());
        Result = GetLastError();
        goto Quit;
    }

#ifndef STANDALONE
    /* Close the thread handle */
    CloseHandle(CommandThread);

    /* Wait for the process to be ready to start */
    WaitForSingleObject(DosStartProc32.hEvent, INFINITE);

    /* Wait for any potential new DOS app started by the 32-bit process */
    RtlZeroMemory(&CommandInfo, sizeof(CommandInfo));

Retry:
    CommandInfo.VDMState = VDM_FLAG_NESTED_TASK | VDM_FLAG_DONT_WAIT;
    DPRINT1("Calling GetNextVDMCommand 32bit for possible new VDM task...\n");
    Success = GetNextVDMCommand(&CommandInfo);
    DPRINT1("GetNextVDMCommand 32bit awaited, success = %s, last error = %d\n", Success ? "true" : "false", GetLastError());

    /*
     * Check whether we were awaited because the 32-bit process was stopped,
     * or because it started a new DOS application.
     */
    if (CommandInfo.CmdLen != 0 || CommandInfo.AppLen != 0 || CommandInfo.PifLen != 0)
    {
        DPRINT1("GetNextVDMCommand 32bit, this is for a new VDM task - CmdLen = %d, AppLen = %d, PifLen = %d\n",
                CommandInfo.CmdLen, CommandInfo.AppLen, CommandInfo.PifLen);

        /* Repeat the request */
        Repeat = TRUE;

        /*
         * Set 'Reentry' to TRUE or FALSE depending on whether we are going
         * to reenter with a new COMMAND.COM. See the comment for:
         *     BOP_CMD 0x10 'Get start information'
         * (dem.c!DosCmdInterpreterBop) for more details.
         */
        Reentry = StartComSpec;

        /* If needed, start a new command interpreter to handle the possible incoming DOS commands */
        if (StartComSpec)
        {
            //
            // DosStartProcess32 was only called by DosCreateProcess, called from INT 21h,
            // so the caller stack is already prepared for running a new DOS program
            // (Flags, CS and IP, and the extra interrupt number, are already pushed).
            //
            Result = DosStartComSpec(FALSE, Environment, ReturnAddress,
                                     &DosStartProc32.ComSpecInfo->ComSpecPsp);
            if (Result != ERROR_SUCCESS)
            {
                DosDisplayMessage("Failed to start a new Command Interpreter (Error: %u).\n", Result);
                goto Quit;
            }
        }
        else
        {
            /* Retrieve the PSP of the COMSPEC (current PSP set by DosLoadExecutable) */
            DosStartProc32.ComSpecInfo->ComSpecPsp = Sda->CurrentPsp;
            Result = ERROR_SUCCESS;
        }

        /* Insert the new entry in the list; it will be freed when needed by COMMAND.COM */
        InsertComSpecInfo(DosStartProc32.ComSpecInfo);
    }
    else
    {
        DPRINT1("GetNextVDMCommand 32bit, 32-bit app stopped\n");

        /* Check whether this was our 32-bit app which was killed */
        if (!DosStartProc32.ComSpecInfo->Terminated)
        {
            DPRINT1("Not our 32-bit app, retrying...\n");
            goto Retry;
        }

        Result = DosStartProc32.ComSpecInfo->dwExitCode;

        /* Delete the entry */
        RtlFreeHeap(RtlGetProcessHeap(), 0, DosStartProc32.ComSpecInfo);
    }
#else
    /* Wait for the thread to finish */
    WaitForSingleObject(CommandThread, INFINITE);
    GetExitCodeThread(CommandThread, &Result);

    /* Close the thread handle */
    CloseHandle(CommandThread);

    DPRINT1("32-bit app stopped\n");
#endif

Quit:
#ifndef STANDALONE
    CloseHandle(DosStartProc32.hEvent);
#endif

    /* Attach to the console and resume the VM */
    DosProcessConsoleAttach();
    EmulatorResume();

    return Result;
}




/******************************************************************************\
|**              DOS Bootloader emulation, Startup and Shutdown              **|
\******************************************************************************/


//
// This function (equivalent of the DOS bootsector) is called by the bootstrap
// loader *BEFORE* jumping at 0000:7C00. What we should do is to write at 0000:7C00
// a BOP call that calls DosInitialize back. Then the bootstrap loader jumps at
// 0000:7C00, our BOP gets called and then we can initialize the 32-bit part of the DOS.
//

/* 16-bit bootstrap code at 0000:7C00 */
/* Of course, this is not in real bootsector format, because we don't care about it for now */
static BYTE Bootsector1[] =
{
    LOBYTE(EMULATOR_BOP), HIBYTE(EMULATOR_BOP), BOP_LOAD_DOS
};
/* This portion of code is run if we failed to load the DOS */
// NOTE: This may also be done by the BIOS32.
static BYTE Bootsector2[] =
{
    LOBYTE(EMULATOR_BOP), HIBYTE(EMULATOR_BOP), BOP_UNSIMULATE
};

static VOID WINAPI DosInitialize(LPWORD Stack);

VOID DosBootsectorInitialize(VOID)
{
    /* We write the bootsector at 0000:7C00 */
    ULONG_PTR StartAddress = (ULONG_PTR)SEG_OFF_TO_PTR(0x0000, 0x7C00);
    ULONG_PTR Address = StartAddress;
    CHAR DosKernelFileName[] = ""; // No DOS BIOS file name, therefore we will load DOS32

    DPRINT("DosBootsectorInitialize\n");

    /* Write the "bootsector" */
    RtlCopyMemory((PVOID)Address, Bootsector1, sizeof(Bootsector1));
    Address += sizeof(Bootsector1);
    RtlCopyMemory((PVOID)Address, DosKernelFileName, sizeof(DosKernelFileName));
    Address += sizeof(DosKernelFileName);
    RtlCopyMemory((PVOID)Address, Bootsector2, sizeof(Bootsector2));
    Address += sizeof(Bootsector2);

    /* Initialize the callback context */
    InitializeContext(&DosContext, 0x0000,
                      (ULONG_PTR)MEM_ALIGN_UP(0x7C00 + Address - StartAddress, sizeof(WORD)));

    /* Register the DOS Loading BOP */
    RegisterBop(BOP_LOAD_DOS, DosInitialize);
}


//
// This function is called by the DOS bootsector in case we load DOS32.
// It sets up the DOS32 start code then jumps to 0070:0000.
//

/* 16-bit startup code for DOS32 at 0070:0000 */
static BYTE Startup[] =
{
    LOBYTE(EMULATOR_BOP), HIBYTE(EMULATOR_BOP), BOP_START_DOS,
    LOBYTE(EMULATOR_BOP), HIBYTE(EMULATOR_BOP), BOP_UNSIMULATE
};

static VOID WINAPI DosStart(LPWORD Stack);

static VOID WINAPI DosInitialize(LPWORD Stack)
{
    /* Get the DOS BIOS file name (NULL-terminated) */
    // FIXME: Isn't it possible to use some DS:SI instead??
    LPCSTR DosBiosFileName = (LPCSTR)SEG_OFF_TO_PTR(getCS(), getIP());
    setIP(getIP() + strlen(DosBiosFileName) + 1); // Skip it

    DPRINT("DosInitialize('%s')\n", DosBiosFileName);

    /*
     * We succeeded, deregister the DOS Loading BOP
     * so that no app will be able to call us back.
     */
    RegisterBop(BOP_LOAD_DOS, NULL);

    /* Register the DOS BOPs */
    RegisterBop(BOP_DOS, DosSystemBop        );
    RegisterBop(BOP_CMD, DosCmdInterpreterBop);

    if (DosBiosFileName[0] != '\0')
    {
        BOOLEAN Success = FALSE;
        HANDLE  hDosBios;
        ULONG   ulDosBiosSize = 0;

        /* Open the DOS BIOS file */
        hDosBios = FileOpen(DosBiosFileName, &ulDosBiosSize);
        if (hDosBios == NULL) goto Quit;

        /* Attempt to load the DOS BIOS into memory */
        Success = FileLoadByHandle(hDosBios,
                                   REAL_TO_PHYS(TO_LINEAR(0x0070, 0x0000)),
                                   ulDosBiosSize,
                                   &ulDosBiosSize);

        DPRINT1("DOS BIOS file '%s' loading %s at %04X:%04X, size 0x%X (Error: %u).\n",
                DosBiosFileName,
                (Success ? "succeeded" : "failed"),
                0x0070, 0x0000,
                ulDosBiosSize,
                GetLastError());

        /* Close the DOS BIOS file */
        FileClose(hDosBios);

Quit:
        if (!Success)
        {
            BiosDisplayMessage("DOS BIOS file '%s' loading failed (Error: %u). The VDM will shut down.\n",
                               DosBiosFileName, GetLastError());
            return;
        }
    }
    else
    {
        /* Load the 16-bit startup code for DOS32 and register its Starting BOP */
        RtlCopyMemory(SEG_OFF_TO_PTR(0x0070, 0x0000), Startup, sizeof(Startup));

        // This is the equivalent of BOP_LOAD_DOS, function 0x11 "Load the DOS kernel"
        // for the Windows NT DOS.
        RegisterBop(BOP_START_DOS, DosStart);
    }

    /* Position execution pointers for DOS startup and return */
    setCS(0x0070);
    setIP(0x0000);
}

static VOID WINAPI DosStart(LPWORD Stack)
{
    BOOLEAN Success;
    DWORD Result;
#ifndef STANDALONE
    INT i;
#endif

    DPRINT("DosStart\n");

    /*
     * We succeeded, deregister the DOS Starting BOP
     * so that no app will be able to call us back.
     */
    RegisterBop(BOP_START_DOS, NULL);

    /* Initialize the callback context */
    InitializeContext(&DosContext, BIOS_CODE_SEGMENT, 0x0010);

    Success  = DosBIOSInitialize();
//  Success &= DosKRNLInitialize();
    if (!Success)
    {
        BiosDisplayMessage("DOS32 loading failed (Error: %u). The VDM will shut down.\n", GetLastError());
        EmulatorTerminate();
        return;
    }

    /* Load the mouse driver */
    // DosMouseInitialize();

#ifndef STANDALONE

    /* Parse the command line arguments */
    for (i = 1; i < NtVdmArgc; i++)
    {
        if (wcsncmp(NtVdmArgv[i], L"-i", 2) == 0)
        {
            /* This is the session ID (hex format) */
            SessionId = wcstoul(NtVdmArgv[i] + 2, NULL, 16);
        }
    }

    /* Initialize Win32-VDM environment */
    Env = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, EnvSize);
    if (Env == NULL)
    {
        DosDisplayMessage("Failed to initialize the global environment (Error: %u). The VDM will shut down.\n", GetLastError());
        EmulatorTerminate();
        return;
    }

    /* Clear the structure */
    RtlZeroMemory(&CommandInfo, sizeof(CommandInfo));

    /* Get the initial information */
    CommandInfo.TaskId = SessionId;
    CommandInfo.VDMState = VDM_GET_FIRST_COMMAND | VDM_FLAG_DOS;
    GetNextVDMCommand(&CommandInfo);

#else

    /* Retrieve the command to start */
    if (NtVdmArgc >= 2)
    {
        WideCharToMultiByte(CP_ACP, 0, NtVdmArgv[1], -1, AppName, sizeof(AppName), NULL, NULL);

        if (NtVdmArgc >= 3)
            WideCharToMultiByte(CP_ACP, 0, NtVdmArgv[2], -1, CmdLine, sizeof(CmdLine), NULL, NULL);
        else
            strcpy(CmdLine, "");
    }
    else
    {
        DosDisplayMessage("Invalid DOS command line\n");
        EmulatorTerminate();
        return;
    }

#endif

    /*
     * At this point, CS:IP points to the DOS BIOS exit code. If the
     * root command interpreter fails to start (or if it exits), DOS
     * exits and the VDM terminates.
     */

    /* Start the root command interpreter */
    // TODO: Really interpret the 'SHELL=' line of CONFIG.NT, and use it!

    /*
     * Prepare the stack for DosStartComSpec:
     * push Flags, CS and IP, and an extra WORD.
     */
    setSP(getSP() - sizeof(WORD));
    *((LPWORD)SEG_OFF_TO_PTR(getSS(), getSP())) = (WORD)getEFLAGS();
    setSP(getSP() - sizeof(WORD));
    *((LPWORD)SEG_OFF_TO_PTR(getSS(), getSP())) = getCS();
    setSP(getSP() - sizeof(WORD));
    *((LPWORD)SEG_OFF_TO_PTR(getSS(), getSP())) = getIP();
    setSP(getSP() - sizeof(WORD));

    Result = DosStartComSpec(TRUE, SEG_OFF_TO_PTR(SYSTEM_ENV_BLOCK, 0),
                             MAKELONG(getIP(), getCS()),
#ifndef STANDALONE
                             &RootCmd.ComSpecPsp
#else
                             NULL
#endif
                             );
    if (Result != ERROR_SUCCESS)
    {
        /* Unprepare the stack for DosStartComSpec */
        setSP(getSP() + 4*sizeof(WORD));

        DosDisplayMessage("Failed to start the Command Interpreter (Error: %u). The VDM will shut down.\n", Result);
        EmulatorTerminate();
        return;
    }

#ifndef STANDALONE
    RootCmd.Terminated = FALSE;
    InsertComSpecInfo(&RootCmd);
#endif

    /**/
    /* Attach to the console and resume the VM */
    DosProcessConsoleAttach();
    EmulatorResume();
    /**/

    return;
}

BOOLEAN DosShutdown(BOOLEAN Immediate)
{
    /*
     * Immediate = TRUE:  Immediate shutdown;
     *             FALSE: Delayed shutdown (notification).
     */

#ifndef STANDALONE
    if (Immediate)
    {
        ExitVDM(FALSE, 0);
        return TRUE;
    }
    else
    {
// HACK!
extern HANDLE VdmTaskEvent; // see emulator.c

        /*
         * Signal the root COMMAND.COM that it should terminate
         * when it checks for a new command.
         */
        RootCmd.Terminated = TRUE;

        /* If the list is already empty, or just contains only one element, bail out */
        // FIXME: Question: if the list has only one element, is it ALWAYS RootCmd ??
        // FIXME: The following is hackish.
        if (IsListEmpty(&ComSpecInfoList) ||
            (ComSpecInfoList.Flink == &RootCmd.Entry  &&
             ComSpecInfoList.Blink == &RootCmd.Entry) &&
            ReentrancyCount == 0 &&
            (WaitForSingleObject(VdmTaskEvent, 0) == WAIT_TIMEOUT))
        {
            /* Nothing runs, so exit immediately */
            ExitVDM(FALSE, 0);
            return TRUE;
        }

        return FALSE;
    }
#else
    UNREFERENCED_PARAMETER(Immediate);
    return TRUE;
#endif
}


/* PUBLIC EXPORTED APIS *******************************************************/

// demLFNCleanup
// demLFNGetCurrentDirectory

// demGetFileTimeByHandle_WOW
// demWOWLFNAllocateSearchHandle
// demWOWLFNCloseSearchHandle
// demWOWLFNEntry
// demWOWLFNGetSearchHandle
// demWOWLFNInit

DWORD
WINAPI
demClientErrorEx(IN HANDLE FileHandle,
                 IN CHAR   Unknown,
                 IN BOOL   Flag)
{
    UNIMPLEMENTED;
    return GetLastError();
}

DWORD
WINAPI
demFileDelete(IN LPCSTR FileName)
{
    if (DeleteFileA(FileName)) SetLastError(ERROR_SUCCESS);

    return GetLastError();
}

DWORD
WINAPI
demFileFindFirst(OUT PVOID  lpFindFileData,
                 IN  LPCSTR FileName,
                 IN  WORD   AttribMask)
{
    BOOLEAN Success = TRUE;
    WIN32_FIND_DATAA FindData;
    HANDLE SearchHandle;
    PDOS_FIND_FILE_BLOCK FindFileBlock = (PDOS_FIND_FILE_BLOCK)lpFindFileData;

    /* Start a search */
    SearchHandle = FindFirstFileA(FileName, &FindData);
    if (SearchHandle == INVALID_HANDLE_VALUE) return GetLastError();

    do
    {
        /* Check the attributes and retry as long as we haven't found a matching file */
        if (!((FindData.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN |
                                            FILE_ATTRIBUTE_SYSTEM |
                                            FILE_ATTRIBUTE_DIRECTORY))
             & ~AttribMask))
        {
            break;
        }
    }
    while ((Success = FindNextFileA(SearchHandle, &FindData)));

    /* If we failed at some point, close the search and return an error */
    if (!Success)
    {
        FindClose(SearchHandle);
        return GetLastError();
    }

    /* Fill the block */
    FindFileBlock->DriveLetter  = DosData->Sda.CurrentDrive + 'A';
    FindFileBlock->AttribMask   = AttribMask;
    FindFileBlock->SearchHandle = SearchHandle;
    FindFileBlock->Attributes   = LOBYTE(FindData.dwFileAttributes);
    FileTimeToDosDateTime(&FindData.ftLastWriteTime,
                          &FindFileBlock->FileDate,
                          &FindFileBlock->FileTime);
    FindFileBlock->FileSize = FindData.nFileSizeHigh ? 0xFFFFFFFF
                                                     : FindData.nFileSizeLow;
    /* Build a short path name */
    if (*FindData.cAlternateFileName)
        strncpy(FindFileBlock->FileName, FindData.cAlternateFileName, sizeof(FindFileBlock->FileName));
    else
        GetShortPathNameA(FindData.cFileName, FindFileBlock->FileName, sizeof(FindFileBlock->FileName));

    return ERROR_SUCCESS;
}

DWORD
WINAPI
demFileFindNext(OUT PVOID lpFindFileData)
{
    WIN32_FIND_DATAA FindData;
    PDOS_FIND_FILE_BLOCK FindFileBlock = (PDOS_FIND_FILE_BLOCK)lpFindFileData;

    do
    {
        /* Continue searching as long as we haven't found a matching file */

        /* If we failed at some point, close the search and return an error */
        if (!FindNextFileA(FindFileBlock->SearchHandle, &FindData))
        {
            FindClose(FindFileBlock->SearchHandle);
            return GetLastError();
        }
    }
    while ((FindData.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN |
                                         FILE_ATTRIBUTE_SYSTEM |
                                         FILE_ATTRIBUTE_DIRECTORY))
           & ~FindFileBlock->AttribMask);

    /* Update the block */
    FindFileBlock->Attributes = LOBYTE(FindData.dwFileAttributes);
    FileTimeToDosDateTime(&FindData.ftLastWriteTime,
                          &FindFileBlock->FileDate,
                          &FindFileBlock->FileTime);
    FindFileBlock->FileSize = FindData.nFileSizeHigh ? 0xFFFFFFFF
                                                     : FindData.nFileSizeLow;
    /* Build a short path name */
    if (*FindData.cAlternateFileName)
        strncpy(FindFileBlock->FileName, FindData.cAlternateFileName, sizeof(FindFileBlock->FileName));
    else
        GetShortPathNameA(FindData.cFileName, FindFileBlock->FileName, sizeof(FindFileBlock->FileName));

    return ERROR_SUCCESS;
}

UCHAR
WINAPI
demGetPhysicalDriveType(IN UCHAR DriveNumber)
{
    UNIMPLEMENTED;
    return DOSDEVICE_DRIVE_UNKNOWN;
}

BOOL
WINAPI
demIsShortPathName(IN LPCSTR Path,
                   IN BOOL Unknown)
{
    UNIMPLEMENTED;
    return FALSE;
}

DWORD
WINAPI
demSetCurrentDirectoryGetDrive(IN  LPCSTR CurrentDirectory,
                               OUT PUCHAR DriveNumber)
{
    UNIMPLEMENTED;
    return ERROR_SUCCESS;
}

/* EOF */
