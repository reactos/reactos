/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            dos/dos32krnl/process.c
 * PURPOSE:         DOS32 Processes
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "ntvdm.h"
#include "emulator.h"
#include "cpu/cpu.h"

#include "dos.h"
#include "dos/dem.h"
#include "dosfiles.h"
#include "handle.h"
#include "process.h"
#include "memory.h"

#include "bios/bios.h"

#include "io.h"
#include "hardware/ps2.h"

/* PUBLIC VARIABLES ***********************************************************/

WORD CurrentPsp = SYSTEM_PSP;

/* PRIVATE FUNCTIONS **********************************************************/

static inline VOID DosSetPspCommandLine(WORD Segment, LPCSTR CommandLine)
{
    PDOS_PSP PspBlock = SEGMENT_TO_PSP(Segment);

    /*
     * Copy the command line block.
     * Format of the CommandLine parameter: 1 byte for size; 127 bytes for contents.
     */
    PspBlock->CommandLineSize = min(*(PBYTE)CommandLine, DOS_CMDLINE_LENGTH);
    CommandLine++;
    RtlCopyMemory(PspBlock->CommandLine, CommandLine, DOS_CMDLINE_LENGTH);
}

static inline VOID DosSaveState(VOID)
{
    PDOS_REGISTER_STATE State;
    WORD StackPointer = getSP();

    /* Allocate stack space for the registers */
    StackPointer -= sizeof(DOS_REGISTER_STATE);
    State = SEG_OFF_TO_PTR(getSS(), StackPointer);

    /* Save */
    State->EAX = getEAX();
    State->ECX = getECX();
    State->EDX = getEDX();
    State->EBX = getEBX();
    State->ESP = getESP();
    State->EBP = getEBP();
    State->ESI = getESI();
    State->EDI = getEDI();
    State->DS = getDS();
    State->ES = getES();
    State->FS = getFS();
    State->GS = getGS();
    State->Flags = getEFLAGS();
}

static inline VOID DosRestoreState(VOID)
{
    PDOS_REGISTER_STATE State;
    WORD StackPointer = getSP();

    /* SS:SP points to the stack on the last entry to INT 21h */
    StackPointer -= (STACK_FLAGS + 1) * 2;      /* Interrupt parameters */
    StackPointer -= sizeof(DOS_REGISTER_STATE); /* Pushed state structure */
    State = SEG_OFF_TO_PTR(getSS(), StackPointer);

    /* Restore */
    setEAX(State->EAX);
    setECX(State->ECX);
    setEDX(State->EDX);
    setEBX(State->EBX);
    setEBP(State->EBP);
    setESI(State->ESI);
    setEDI(State->EDI);
    setDS(State->DS);
    setES(State->ES);
    setFS(State->FS);
    setGS(State->GS);
    setEFLAGS(State->Flags);
}

static WORD DosCopyEnvironmentBlock(LPCSTR Environment OPTIONAL,
                                    LPCSTR ProgramName)
{
    PCHAR Ptr, DestBuffer = NULL;
    ULONG TotalSize = 0;
    WORD DestSegment;

    /* If we have an environment strings list, compute its size */
    if (Environment)
    {
        /* Calculate the size of the environment block */
        Ptr = (PCHAR)Environment;
        while (*Ptr) Ptr += strlen(Ptr) + 1;
        TotalSize = (ULONG_PTR)Ptr - (ULONG_PTR)Environment;
    }
    else
    {
        /* Empty environment string */
        TotalSize = 1;
    }
    /* Add the final environment block NULL-terminator */
    TotalSize++;

    /* Add the two bytes for the program name tag */
    TotalSize += 2;

    /* Add the string buffer size */
    TotalSize += strlen(ProgramName) + 1;

    /* Allocate the memory for the environment block */
    DestSegment = DosAllocateMemory((WORD)((TotalSize + 0x0F) >> 4), NULL);
    if (!DestSegment) return 0;

    DestBuffer = (PCHAR)SEG_OFF_TO_PTR(DestSegment, 0);

    /* If we have an environment strings list, copy it */
    if (Environment)
    {
        Ptr = (PCHAR)Environment;
        while (*Ptr)
        {
            /* Copy the string and NULL-terminate it */
            strcpy(DestBuffer, Ptr);
            DestBuffer += strlen(Ptr);
            *(DestBuffer++) = '\0';

            /* Move to the next string */
            Ptr += strlen(Ptr) + 1;
        }
    }
    else
    {
        /* Empty environment string */
        *(DestBuffer++) = '\0';
    }
    /* NULL-terminate the environment block */
    *(DestBuffer++) = '\0';

    /* Store the special program name tag */
    *(DestBuffer++) = LOBYTE(DOS_PROGRAM_NAME_TAG);
    *(DestBuffer++) = HIBYTE(DOS_PROGRAM_NAME_TAG);

    /* Copy the program name after the environment block */
    strcpy(DestBuffer, ProgramName);

    return DestSegment;
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID DosClonePsp(WORD DestSegment, WORD SourceSegment)
{
    PDOS_PSP DestPsp    = SEGMENT_TO_PSP(DestSegment);
    PDOS_PSP SourcePsp  = SEGMENT_TO_PSP(SourceSegment);
    LPDWORD IntVecTable = (LPDWORD)((ULONG_PTR)BaseAddress);

    /* Literally copy the PSP first */
    RtlCopyMemory(DestPsp, SourcePsp, sizeof(DOS_PSP));

    /* Save the interrupt vectors */
    DestPsp->TerminateAddress = IntVecTable[0x22];
    DestPsp->BreakAddress     = IntVecTable[0x23];
    DestPsp->CriticalAddress  = IntVecTable[0x24];

    /* No parent PSP */
    DestPsp->ParentPsp = 0;

    /* Set the handle table pointers to the internal handle table */
    DestPsp->HandleTableSize = DEFAULT_JFT_SIZE;
    DestPsp->HandleTablePtr  = MAKELONG(0x18, DestSegment);

    /* Copy the parent handle table without referencing the SFT */
    RtlCopyMemory(FAR_POINTER(DestPsp->HandleTablePtr),
                  FAR_POINTER(SourcePsp->HandleTablePtr),
                  DEFAULT_JFT_SIZE);
}

VOID DosCreatePsp(WORD Segment, WORD ProgramSize)
{
    PDOS_PSP PspBlock   = SEGMENT_TO_PSP(Segment);
    LPDWORD IntVecTable = (LPDWORD)((ULONG_PTR)BaseAddress);

    RtlZeroMemory(PspBlock, sizeof(*PspBlock));

    /* Set the exit interrupt */
    PspBlock->Exit[0] = 0xCD; // int 0x20
    PspBlock->Exit[1] = 0x20;

    /* Set the number of the last paragraph */
    PspBlock->LastParagraph = Segment + ProgramSize - 1;

    /* Save the interrupt vectors */
    PspBlock->TerminateAddress = IntVecTable[0x22];
    PspBlock->BreakAddress     = IntVecTable[0x23];
    PspBlock->CriticalAddress  = IntVecTable[0x24];

    /* Set the parent PSP */
    PspBlock->ParentPsp = CurrentPsp;

    /* No environment block yet */
    PspBlock->EnvBlock = 0;

    /* Copy the parent handle table */
    DosCopyHandleTable(PspBlock->HandleTable);

    /* Set the handle table pointers to the internal handle table */
    PspBlock->HandleTableSize = DEFAULT_JFT_SIZE;
    PspBlock->HandleTablePtr  = MAKELONG(0x18, Segment);

    /* Set the DOS version */
    PspBlock->DosVersion = DOS_VERSION;

    /* Set the far call opcodes */
    PspBlock->FarCall[0] = 0xCD; // int 0x21
    PspBlock->FarCall[1] = 0x21;
    PspBlock->FarCall[2] = 0xCB; // retf
}

VOID DosSetProcessContext(WORD Segment)
{
    CurrentPsp = Segment;
    DiskTransferArea = MAKELONG(0x80, Segment);
}

DWORD DosLoadExecutable(IN DOS_EXEC_TYPE LoadType,
                        IN LPCSTR ExecutablePath,
                        IN PDOS_EXEC_PARAM_BLOCK Parameters,
                        IN LPCSTR CommandLine OPTIONAL,
                        IN LPCSTR Environment OPTIONAL,
                        IN DWORD ReturnAddress OPTIONAL)
{
    DWORD Result = ERROR_SUCCESS;
    HANDLE FileHandle = INVALID_HANDLE_VALUE, FileMapping = NULL;
    LPBYTE Address = NULL;
    WORD Segment = 0;
    WORD EnvBlock = 0;
    WORD LoadSegment;
    WORD MaxAllocSize;
    DWORD i, FileSize;

    /* Buffer for command line conversion: 1 byte for size; 127 bytes for contents */
    CHAR CmdLineBuffer[1 + DOS_CMDLINE_LENGTH];

    DPRINT1("DosLoadExecutable(%d, %s, 0x%08X, 0x%08X)\n",
            LoadType,
            ExecutablePath,
            Parameters,
            ReturnAddress);

    /* Open a handle to the executable */
    FileHandle = CreateFileA(ExecutablePath,
                             GENERIC_READ,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL);
    if (FileHandle == INVALID_HANDLE_VALUE)
    {
        Result = GetLastError();
        goto Cleanup;
    }

    /* Get the file size */
    FileSize = GetFileSize(FileHandle, NULL);

    /* Create a mapping object for the file */
    FileMapping = CreateFileMapping(FileHandle,
                                    NULL,
                                    PAGE_READONLY,
                                    0,
                                    0,
                                    NULL);
    if (FileMapping == NULL)
    {
        Result = GetLastError();
        goto Cleanup;
    }

    /* Map the file into memory */
    Address = (LPBYTE)MapViewOfFile(FileMapping, FILE_MAP_READ, 0, 0, 0);
    if (Address == NULL)
    {
        Result = GetLastError();
        goto Cleanup;
    }

    if (LoadType != DOS_LOAD_OVERLAY)
    {
        /* If an optional Win32 command line is given... */
        if (CommandLine)
        {
            /* ... convert it into DOS format */
            BYTE CmdLineLen;

            PBYTE CmdLineSize  = (PBYTE)CmdLineBuffer;
            LPSTR CmdLineStart = CmdLineBuffer + 1;
            LPSTR CmdLinePtr   = CmdLineStart;

            // For debugging purposes
            RtlFillMemory(CmdLineBuffer, sizeof(CmdLineBuffer), 0xFF);

            /*
             * Set the command line: it is either an empty command line or has
             * the format: " foo bar ..." (with at least one leading whitespace),
             * and is then always followed by '\r' (and optionally by '\n').
             */
            CmdLineLen = (BYTE)strlen(CommandLine);
            *CmdLineSize = 0;

            /*
             * Add the leading space if the command line is not empty
             * and doesn't already start with some whitespace...
             */
            if (*CommandLine && *CommandLine != '\r' && *CommandLine != '\n' &&
                *CommandLine != ' ' && *CommandLine != '\t')
            {
                (*CmdLineSize)++;
                *CmdLinePtr++ = ' ';
            }

            /* Compute the number of characters we need to copy from the original command line */
            CmdLineLen = min(CmdLineLen, DOS_CMDLINE_LENGTH - *CmdLineSize);

            /* The trailing '\r' or '\n' do not count in the PSP command line size parameter */
            while (CmdLineLen && (CommandLine[CmdLineLen - 1] == '\r' || CommandLine[CmdLineLen - 1] == '\n'))
            {
                CmdLineLen--;
            }

            /* Finally, set everything up */
            *CmdLineSize += CmdLineLen;
            RtlCopyMemory(CmdLinePtr, CommandLine, CmdLineLen);
            CmdLineStart[*CmdLineSize] = '\r';

            /* Finally make the pointer point to the static buffer */
            CommandLine = CmdLineBuffer;
        }
        else
        {
            /*
             * ... otherwise, get the one from the parameter block.
             * Format of the command line: 1 byte for size; 127 bytes for contents.
             */
            ASSERT(Parameters);
            CommandLine = (LPCSTR)FAR_POINTER(Parameters->CommandLine);
        }

        /* If no optional environment is given... */
        if (Environment == NULL)
        {
            /* ... get the one from the parameter block */
            ASSERT(Parameters);
            Environment = (LPCSTR)SEG_OFF_TO_PTR(Parameters->Environment, 0);
        }

        /* Copy the environment block to DOS memory */
        EnvBlock = DosCopyEnvironmentBlock(Environment, ExecutablePath);
        if (EnvBlock == 0)
        {
            Result = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    /* Check if this is an EXE file or a COM file */
    if (Address[0] == 'M' && Address[1] == 'Z')
    {
        /* EXE file */
        PIMAGE_DOS_HEADER Header;
        DWORD BaseSize;
        PDWORD RelocationTable;
        PWORD RelocWord;
        WORD RelocFactor;
        BOOLEAN LoadHigh = FALSE;

        /* Get the MZ header */
        Header = (PIMAGE_DOS_HEADER)Address;

        /* Get the base size of the file, in paragraphs (rounded up) */
        BaseSize = (((Header->e_cp - 1) * 512) + Header->e_cblp + 0x0F) >> 4;

        if (LoadType != DOS_LOAD_OVERLAY)
        {
            DWORD TotalSize = BaseSize;

            /* Add the PSP size, in paragraphs */
            TotalSize += sizeof(DOS_PSP) >> 4;

            /* Add the maximum size that should be allocated */
            TotalSize += Header->e_maxalloc;

            if (Header->e_minalloc == 0 && Header->e_maxalloc == 0)
            {
                /* This program should be loaded high */
                LoadHigh = TRUE;
                TotalSize = 0xFFFF;
            }

            /* Make sure it does not pass 0xFFFF */
            if (TotalSize > 0xFFFF) TotalSize = 0xFFFF;

            /* Try to allocate that much memory */
            Segment = DosAllocateMemory((WORD)TotalSize, &MaxAllocSize);

            if (Segment == 0)
            {
                /* Check if there's at least enough memory for the minimum size */
                if (MaxAllocSize < (BaseSize + (sizeof(DOS_PSP) >> 4) + Header->e_minalloc))
                {
                    Result = DosLastError;
                    goto Cleanup;
                }

                /* Allocate that minimum amount */
                TotalSize = MaxAllocSize;
                Segment = DosAllocateMemory((WORD)TotalSize, NULL);
                ASSERT(Segment != 0);
            }

            /* The process owns its own memory */
            DosChangeMemoryOwner(Segment, Segment);
            DosChangeMemoryOwner(EnvBlock, Segment);

            /* Set INT 22h to the return address */
            ((PULONG)BaseAddress)[0x22] = ReturnAddress;

            /* Create the PSP */
            DosCreatePsp(Segment, (WORD)TotalSize);
            DosSetPspCommandLine(Segment, CommandLine);
            SEGMENT_TO_PSP(Segment)->EnvBlock = EnvBlock;

            /* Calculate the segment where the program should be loaded */
            if (!LoadHigh) LoadSegment = Segment + (sizeof(DOS_PSP) >> 4);
            else LoadSegment = Segment + TotalSize - BaseSize;

            RelocFactor = LoadSegment;
        }
        else
        {
            ASSERT(Parameters);
            LoadSegment = Parameters->Overlay.Segment;
            RelocFactor = Parameters->Overlay.RelocationFactor;
        }

        /* Copy the program to the code segment */
        RtlCopyMemory(SEG_OFF_TO_PTR(LoadSegment, 0),
                      Address + (Header->e_cparhdr << 4),
                      min(FileSize - (Header->e_cparhdr << 4), BaseSize << 4));

        /* Get the relocation table */
        RelocationTable = (PDWORD)(Address + Header->e_lfarlc);

        /* Perform relocations */
        for (i = 0; i < Header->e_crlc; i++)
        {
            /* Get a pointer to the word that needs to be patched */
            RelocWord = (PWORD)SEG_OFF_TO_PTR(LoadSegment + HIWORD(RelocationTable[i]),
                                              LOWORD(RelocationTable[i]));

            /* Add the relocation factor to it */
            *RelocWord += RelocFactor;
        }

        if (LoadType == DOS_LOAD_AND_EXECUTE)
        {
            /* Save the program state */
            if (CurrentPsp != SYSTEM_PSP) DosSaveState();

            /* Set the initial segment registers */
            setDS(Segment);
            setES(Segment);

            /* Set the stack to the location from the header */
            setSS(LoadSegment + Header->e_ss);
            setSP(Header->e_sp);

            /* Execute */
            DosSetProcessContext(Segment);
            CpuExecute(LoadSegment + Header->e_cs, Header->e_ip);
        }
        else if (LoadType == DOS_LOAD_ONLY)
        {
            ASSERT(Parameters);
            Parameters->StackLocation = MAKELONG(Header->e_sp, LoadSegment + Header->e_ss);
            Parameters->EntryPoint    = MAKELONG(Header->e_ip, LoadSegment + Header->e_cs);
        }
    }
    else
    {
        /* COM file */

        if (LoadType != DOS_LOAD_OVERLAY)
        {
            /* Find the maximum amount of memory that can be allocated */
            DosAllocateMemory(0xFFFF, &MaxAllocSize);

            /* Make sure it's enough for the whole program and the PSP */
            if (((DWORD)MaxAllocSize << 4) < (FileSize + sizeof(DOS_PSP)))
            {
                Result = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            /* Allocate all of it */
            Segment = DosAllocateMemory(MaxAllocSize, NULL);
            if (Segment == 0)
            {
                Result = DosLastError;
                goto Cleanup;
            }

            /* The process owns its own memory */
            DosChangeMemoryOwner(Segment, Segment);
            DosChangeMemoryOwner(EnvBlock, Segment);

            /* Set INT 22h to the return address */
            ((PULONG)BaseAddress)[0x22] = ReturnAddress;

            /* Create the PSP */
            DosCreatePsp(Segment, MaxAllocSize);
            DosSetPspCommandLine(Segment, CommandLine);
            SEGMENT_TO_PSP(Segment)->EnvBlock = EnvBlock;

            /* Calculate the segment where the program should be loaded */
            LoadSegment = Segment + (sizeof(DOS_PSP) >> 4);
        }
        else
        {
            ASSERT(Parameters);
            LoadSegment = Parameters->Overlay.Segment;
        }

        /* Copy the program to the code segment */
        RtlCopyMemory(SEG_OFF_TO_PTR(LoadSegment, 0),
                      Address,
                      FileSize);

        if (LoadType == DOS_LOAD_AND_EXECUTE)
        {
            /* Set the initial segment registers */
            setDS(Segment);
            setES(Segment);

            /* Set the stack to the last word of the segment */
            setSS(Segment);
            setSP(0xFFFE);

            /*
             * Set the value on the stack to 0, so that a near return
             * jumps to PSP:0000 which has the exit code.
             */
            *((LPWORD)SEG_OFF_TO_PTR(Segment, 0xFFFE)) = 0;

            /* Execute */
            DosSetProcessContext(Segment);
            CpuExecute(Segment, 0x100);
        }
        else if (LoadType == DOS_LOAD_ONLY)
        {
            ASSERT(Parameters);
            Parameters->StackLocation = MAKELONG(0xFFFE, Segment);
            Parameters->EntryPoint    = MAKELONG(0x0100, Segment);
        }
    }

Cleanup:
    if (Result != ERROR_SUCCESS)
    {
        /* It was not successful, cleanup the DOS memory */
        if (EnvBlock) DosFreeMemory(EnvBlock);
        if (Segment) DosFreeMemory(Segment);
    }

    /* Unmap the file*/
    if (Address != NULL) UnmapViewOfFile(Address);

    /* Close the file mapping object */
    if (FileMapping != NULL) CloseHandle(FileMapping);

    /* Close the file handle */
    if (FileHandle != INVALID_HANDLE_VALUE) CloseHandle(FileHandle);

    return Result;
}

DWORD DosStartProcess(IN LPCSTR ExecutablePath,
                      IN LPCSTR CommandLine,
                      IN LPCSTR Environment OPTIONAL)
{
    DWORD Result;
    LPDWORD IntVecTable = (LPDWORD)((ULONG_PTR)BaseAddress);

    Result = DosLoadExecutable(DOS_LOAD_AND_EXECUTE,
                               ExecutablePath,
                               NULL,
                               CommandLine,
                               Environment,
                               IntVecTable[0x20]);

    if (Result != ERROR_SUCCESS) goto Quit;

    /* Attach to the console */
    ConsoleAttach();
    VidBiosAttachToConsole();

    // HACK: Simulate a ENTER key release scancode on the PS/2 port because
    // some apps expect to read a key release scancode (> 0x80) when they
    // are started.
    // (hbelusca 2 May 2015: I'm not sure it's really useful. See r65012)
    // IOWriteB(PS2_CONTROL_PORT, 0xD2);     // Next write is for the first PS/2 port
    // IOWriteB(PS2_DATA_PORT, 0x80 | 0x1C); // ENTER key release

    /* Start simulation */
    SetEvent(VdmTaskEvent);
    CpuSimulate();

    /* Detach from the console */
    VidBiosDetachFromConsole();
    ConsoleDetach();

Quit:
    return Result;
}

#ifndef STANDALONE
WORD DosCreateProcess(LPCSTR ProgramName,
                      PDOS_EXEC_PARAM_BLOCK Parameters,
                      DWORD ReturnAddress)
{
    DWORD Result;
    DWORD BinaryType;
    LPVOID Environment = NULL;
    VDM_COMMAND_INFO CommandInfo;
    CHAR CmdLine[MAX_PATH]; // DOS_CMDLINE_LENGTH + 1
    CHAR AppName[MAX_PATH];
    CHAR PifFile[MAX_PATH];
    CHAR Desktop[MAX_PATH];
    CHAR Title[MAX_PATH];
    LPSTR CmdLinePtr;
    ULONG CmdLineSize;
    ULONG EnvSize = 256;
    PVOID Env;
    STARTUPINFOA StartupInfo;
    PROCESS_INFORMATION ProcessInfo;

    /* Get the binary type */
    if (!GetBinaryTypeA(ProgramName, &BinaryType)) return GetLastError();

    /* Initialize Win32-VDM environment */
    Env = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, EnvSize);
    if (Env == NULL) return GetLastError();

    /* Did the caller specify an environment segment? */
    if (Parameters->Environment)
    {
        /* Yes, use it instead of the parent one */
        Environment = SEG_OFF_TO_PTR(Parameters->Environment, 0);
    }

    /* Set up the startup info structure */
    RtlZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);

    /*
     * Convert the DOS command line to Win32-compatible format.
     * Format of the DOS command line: 1 byte for size; 127 bytes for contents.
     */
    CmdLineSize = min(*(PBYTE)FAR_POINTER(Parameters->CommandLine), DOS_CMDLINE_LENGTH);
    RtlCopyMemory(CmdLine,
                  (LPSTR)FAR_POINTER(Parameters->CommandLine) + 1,
                  CmdLineSize);
    /* NULL-terminate it */
    CmdLine[CmdLineSize] = '\0';

    /* Remove any trailing return carriage character and NULL-terminate the command line */
    CmdLinePtr = CmdLine;
    while (*CmdLinePtr && *CmdLinePtr != '\r' && *CmdLinePtr != '\n') CmdLinePtr++;
    *CmdLinePtr = '\0';

    /* Create the process */
    if (!CreateProcessA(ProgramName,
                        CmdLine,
                        NULL,
                        NULL,
                        FALSE,
                        0,
                        Environment,
                        NULL,
                        &StartupInfo,
                        &ProcessInfo))
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, Env);
        return GetLastError();
    }

    /* Check the type of the program */
    switch (BinaryType)
    {
        /* These are handled by NTVDM */
        case SCS_DOS_BINARY:
        case SCS_WOW_BINARY:
        {
            /* Clear the structure */
            RtlZeroMemory(&CommandInfo, sizeof(CommandInfo));

            /* Initialize the structure members */
            CommandInfo.TaskId = SessionId;
            CommandInfo.VDMState = VDM_FLAG_NESTED_TASK | VDM_FLAG_DONT_WAIT;
            CommandInfo.CmdLine = CmdLine;
            CommandInfo.CmdLen = sizeof(CmdLine);
            CommandInfo.AppName = AppName;
            CommandInfo.AppLen = sizeof(AppName);
            CommandInfo.PifFile = PifFile;
            CommandInfo.PifLen = sizeof(PifFile);
            CommandInfo.Desktop = Desktop;
            CommandInfo.DesktopLen = sizeof(Desktop);
            CommandInfo.Title = Title;
            CommandInfo.TitleLen = sizeof(Title);
            CommandInfo.Env = Env;
            CommandInfo.EnvLen = EnvSize;

Command:
            /* Get the VDM command information */
            if (!GetNextVDMCommand(&CommandInfo))
            {
                if (CommandInfo.EnvLen > EnvSize)
                {
                    /* Expand the environment size */
                    EnvSize = CommandInfo.EnvLen;
                    CommandInfo.Env = Env = RtlReAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, Env, EnvSize);

                    /* Repeat the request */
                    CommandInfo.VDMState |= VDM_FLAG_RETRY;
                    goto Command;
                }

                /* Shouldn't happen */
                ASSERT(FALSE);
            }

            /* Load the executable */
            Result = DosLoadExecutable(DOS_LOAD_AND_EXECUTE,
                                       AppName,
                                       Parameters,
                                       CmdLine,
                                       Env,
                                       ReturnAddress);
            if (Result == ERROR_SUCCESS)
            {
                /* Increment the re-entry count */
                CommandInfo.VDMState = VDM_INC_REENTER_COUNT;
                GetNextVDMCommand(&CommandInfo);
            }
            else
            {
                DisplayMessage(L"Could not load '%S'. Error: %u", AppName, Result);
            }

            break;
        }

        /* Not handled by NTVDM */
        default:
        {
            /* Wait for the process to finish executing */
            WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
        }
    }

    RtlFreeHeap(RtlGetProcessHeap(), 0, Env);

    /* Close the handles */
    CloseHandle(ProcessInfo.hProcess);
    CloseHandle(ProcessInfo.hThread);

    return ERROR_SUCCESS;
}
#endif

VOID DosTerminateProcess(WORD Psp, BYTE ReturnCode, WORD KeepResident)
{
    WORD i;
    WORD McbSegment = FIRST_MCB_SEGMENT;
    PDOS_MCB CurrentMcb;
    LPDWORD IntVecTable = (LPDWORD)((ULONG_PTR)BaseAddress);
    PDOS_PSP PspBlock = SEGMENT_TO_PSP(Psp);
#ifndef STANDALONE
    VDM_COMMAND_INFO CommandInfo;
#endif

    DPRINT("DosTerminateProcess: Psp 0x%04X, ReturnCode 0x%02X, KeepResident 0x%04X\n",
           Psp,
           ReturnCode,
           KeepResident);

    /* Check if this PSP is it's own parent */
    if (PspBlock->ParentPsp == Psp) goto Done;

    if (KeepResident == 0)
    {
        for (i = 0; i < PspBlock->HandleTableSize; i++)
        {
            /* Close the handle */
            DosCloseHandle(i);
        }
    }

    /* Free the memory used by the process */
    while (TRUE)
    {
        /* Get a pointer to the MCB */
        CurrentMcb = SEGMENT_TO_MCB(McbSegment);

        /* Make sure the MCB is valid */
        if (CurrentMcb->BlockType != 'M' && CurrentMcb->BlockType != 'Z') break;

        /* Check if this block was allocated by the process */
        if (CurrentMcb->OwnerPsp == Psp)
        {
            if (KeepResident)
            {
                /* Check if this is the PSP block and we should reduce its size */
                if ((McbSegment + 1) == Psp && KeepResident < CurrentMcb->Size)
                {
                    /* Reduce the size of the block */
                    DosResizeMemory(McbSegment + 1, KeepResident, NULL);
                    break;
                }
            }
            else
            {
                /* Free this entire block */
                DosFreeMemory(McbSegment + 1);
            }
        }

        /* If this was the last block, quit */
        if (CurrentMcb->BlockType == 'Z') break;

        /* Update the segment and continue */
        McbSegment += CurrentMcb->Size + 1;
    }

Done:
    /* Restore the interrupt vectors */
    IntVecTable[0x22] = PspBlock->TerminateAddress;
    IntVecTable[0x23] = PspBlock->BreakAddress;
    IntVecTable[0x24] = PspBlock->CriticalAddress;

    /* Update the current PSP */
    if (Psp == CurrentPsp)
    {
        CurrentPsp = PspBlock->ParentPsp;
        if (CurrentPsp == SYSTEM_PSP)
        {
            ResetEvent(VdmTaskEvent);
            CpuUnsimulate();
            return;
        }
    }

#ifndef STANDALONE

    /* Decrement the re-entry count */
    CommandInfo.TaskId = SessionId;
    CommandInfo.VDMState = VDM_DEC_REENTER_COUNT;
    GetNextVDMCommand(&CommandInfo);

    /* Clear the structure */
    RtlZeroMemory(&CommandInfo, sizeof(CommandInfo));

    /* Update the VDM state of the task */
    CommandInfo.TaskId = SessionId;
    CommandInfo.VDMState = VDM_FLAG_DONT_WAIT;
    GetNextVDMCommand(&CommandInfo);

#endif

    /* Save the return code - Normal termination */
    DosErrorLevel = MAKEWORD(ReturnCode, 0x00);

    /* Restore the old stack */
    setSS(HIWORD(SEGMENT_TO_PSP(CurrentPsp)->LastStack));
    setSP(LOWORD(SEGMENT_TO_PSP(CurrentPsp)->LastStack));

    /* Restore the program state */
    DosRestoreState();

    /* Return control to the parent process */
    CpuExecute(HIWORD(PspBlock->TerminateAddress),
               LOWORD(PspBlock->TerminateAddress));
}

