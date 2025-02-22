/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/dos/dos32krnl/process.c
 * PURPOSE:         DOS32 Processes
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "ntvdm.h"

#define NDEBUG
#include <debug.h>

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

#include "vddsup.h"

/* PRIVATE FUNCTIONS **********************************************************/

static VOID DosInitPsp(IN WORD Segment,
                       IN WORD EnvBlock,
                       IN LPCSTR CommandLine,
                       IN LPCSTR ProgramName)
{
    PDOS_PSP PspBlock = SEGMENT_TO_PSP(Segment);
    PDOS_MCB Mcb = SEGMENT_TO_MCB(Segment - 1);
    LPCSTR PspName;
    USHORT i;

    /* Link the environment block */
    PspBlock->EnvBlock = EnvBlock;

    /*
     * Copy the command line.
     * Format of the CommandLine parameter: 1 byte for size; 127 bytes for contents.
     */
    PspBlock->CommandLineSize = min(*(PBYTE)CommandLine, DOS_CMDLINE_LENGTH);
    CommandLine++;
    RtlCopyMemory(PspBlock->CommandLine, CommandLine, DOS_CMDLINE_LENGTH);

    /*
     * Initialize the owner name of the MCB of the PSP.
     */

    /* Find the start of the file name, skipping all the path elements */
    PspName = ProgramName;
    while (*ProgramName)
    {
        switch (*ProgramName++)
        {
            /* Path delimiter, skip it */
            case ':': case '\\': case '/':
                PspName = ProgramName;
                break;
        }
    }
    /* Copy the file name up to the extension... */
    for (i = 0; i < sizeof(Mcb->Name) && PspName[i] != '.' && PspName[i] != '\0'; ++i)
    {
        Mcb->Name[i] = RtlUpperChar(PspName[i]);
    }
    /* ... and NULL-terminate if needed */
    if (i < sizeof(Mcb->Name)) Mcb->Name[i] = '\0';

    // FIXME: Initialize the FCBs
}

static inline VOID DosSaveState(VOID)
{
    PDOS_REGISTER_STATE State;
    WORD StackPointer = getSP();

#ifdef ADVANCED_DEBUGGING
    DPRINT1("\n"
            "DosSaveState(before) -- SS:SP == %04X:%04X\n"
            "Original CPU State =\n"
            "DS = %04X; ES = %04X; AX = %04X; CX = %04X\n"
            "DX = %04X; BX = %04X; BP = %04X; SI = %04X; DI = %04X"
            "\n",
            getSS(), getSP(),
            getDS(), getES(), getAX(), getCX(),
            getDX(), getBX(), getBP(), getSI(), getDI());
#endif

    /*
     * Allocate stack space for the registers. Note that we
     * already have one word allocated (the interrupt number).
     */
    StackPointer -= sizeof(DOS_REGISTER_STATE) - sizeof(WORD);
    State = SEG_OFF_TO_PTR(getSS(), StackPointer);
    setSP(StackPointer);

    /* Save */
    State->DS = getDS();
    State->ES = getES();
    State->AX = getAX();
    State->CX = getCX();
    State->DX = getDX();
    State->BX = getBX();
    State->BP = getBP();
    State->SI = getSI();
    State->DI = getDI();

#ifdef ADVANCED_DEBUGGING
    DPRINT1("\n"
            "DosSaveState(after) -- SS:SP == %04X:%04X\n"
            "Saved State =\n"
            "DS = %04X; ES = %04X; AX = %04X; CX = %04X\n"
            "DX = %04X; BX = %04X; BP = %04X; SI = %04X; DI = %04X"
            "\n",
            getSS(), getSP(),
            State->DS, State->ES, State->AX, State->CX,
            State->DX, State->BX, State->BP, State->SI, State->DI);
#endif
}

static inline VOID DosRestoreState(VOID)
{
    PDOS_REGISTER_STATE State;

    /*
     * Pop the state structure from the stack. Note that we
     * already have one word allocated (the interrupt number).
     */
    State = SEG_OFF_TO_PTR(getSS(), getSP());

#ifdef ADVANCED_DEBUGGING
    DPRINT1("\n"
            "DosRestoreState(before) -- SS:SP == %04X:%04X\n"
            "Saved State =\n"
            "DS = %04X; ES = %04X; AX = %04X; CX = %04X\n"
            "DX = %04X; BX = %04X; BP = %04X; SI = %04X; DI = %04X"
            "\n",
            getSS(), getSP(),
            State->DS, State->ES, State->AX, State->CX,
            State->DX, State->BX, State->BP, State->SI, State->DI);
#endif

    setSP(getSP() + sizeof(DOS_REGISTER_STATE) - sizeof(WORD));

    /* Restore */
    setDS(State->DS);
    setES(State->ES);
    setAX(State->AX);
    setCX(State->CX);
    setDX(State->DX);
    setBX(State->BX);
    setBP(State->BP);
    setSI(State->SI);
    setDI(State->DI);

#ifdef ADVANCED_DEBUGGING
    DPRINT1("\n"
            "DosRestoreState(after) -- SS:SP == %04X:%04X\n"
            "Restored CPU State =\n"
            "DS = %04X; ES = %04X; AX = %04X; CX = %04X\n"
            "DX = %04X; BX = %04X; BP = %04X; SI = %04X; DI = %04X"
            "\n",
            getSS(), getSP(),
            getDS(), getES(), getAX(), getCX(),
            getDX(), getBX(), getBP(), getSI(), getDI());
#endif
}

static WORD DosCopyEnvironmentBlock(IN LPCSTR Environment OPTIONAL,
                                    IN LPCSTR ProgramName)
{
    PCHAR Ptr, DestBuffer = NULL;
    SIZE_T TotalSize = 0;
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
    RtlCopyMemory(DestPsp, SourcePsp, sizeof(*DestPsp));

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
    PspBlock->LastParagraph = Segment + ProgramSize;

    /* Save the interrupt vectors */
    PspBlock->TerminateAddress = IntVecTable[0x22];
    PspBlock->BreakAddress     = IntVecTable[0x23];
    PspBlock->CriticalAddress  = IntVecTable[0x24];

    /* Set the parent PSP */
    PspBlock->ParentPsp = Sda->CurrentPsp;

    if (Sda->CurrentPsp != SYSTEM_PSP)
    {
        /* Link to the parent's environment block */
        PspBlock->EnvBlock = SEGMENT_TO_PSP(Sda->CurrentPsp)->EnvBlock;
    }
/*
    else
    {
        PspBlock->EnvBlock = SEG_OFF_TO_PTR(SYSTEM_ENV_BLOCK, 0);
    }
*/

    /* Copy the parent handle table */
    DosCopyHandleTable(PspBlock->HandleTable);

    /* Set the handle table pointers to the internal handle table */
    PspBlock->HandleTableSize = DEFAULT_JFT_SIZE;
    PspBlock->HandleTablePtr  = MAKELONG(0x18, Segment);

    /* Set the DOS version */
    // FIXME: This is here that SETVER stuff enters into action!
    PspBlock->DosVersion = DosData->DosVersion;

    /* Set the far call opcodes */
    PspBlock->FarCall[0] = 0xCD; // int 0x21
    PspBlock->FarCall[1] = 0x21;
    PspBlock->FarCall[2] = 0xCB; // retf
}

VOID DosSetProcessContext(WORD Segment)
{
    Sda->CurrentPsp = Segment;
    Sda->DiskTransferArea = MAKELONG(0x80, Segment);
}

DWORD DosLoadExecutableInternal(IN DOS_EXEC_TYPE LoadType,
                                IN LPBYTE ExeBuffer,
                                IN DWORD ExeBufferSize,
                                IN LPCSTR ExePath,
                                IN PDOS_EXEC_PARAM_BLOCK Parameters,
                                IN LPCSTR CommandLine OPTIONAL,
                                IN LPCSTR Environment OPTIONAL,
                                IN DWORD ReturnAddress OPTIONAL)
{
    DWORD Result = ERROR_SUCCESS;
    WORD Segment = 0;
    WORD EnvBlock = 0;
    WORD ExeSignature;
    WORD LoadSegment;
    WORD MaxAllocSize;

    WORD FinalSS, FinalSP;
    WORD FinalCS, FinalIP;

    /* Buffer for command line conversion: 1 byte for size; 127 bytes for contents */
    CHAR CmdLineBuffer[1 + DOS_CMDLINE_LENGTH];

    DPRINT1("DosLoadExecutableInternal(%d, 0x%p, '%s', 0x%p, 0x%p, 0x%p)\n",
            LoadType, ExeBuffer, ExePath, Parameters, CommandLine, Environment);

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
            ASSERT(Parameters);
            /* ... get the one from the parameter block (if not NULL)... */
            if (Parameters->Environment)
                Environment = (LPCSTR)SEG_OFF_TO_PTR(Parameters->Environment, 0);
            /* ... or the one from the parent (otherwise) */
            else
                Environment = (LPCSTR)SEG_OFF_TO_PTR(SEGMENT_TO_PSP(Sda->CurrentPsp)->EnvBlock, 0);
        }

        /* Copy the environment block to DOS memory */
        EnvBlock = DosCopyEnvironmentBlock(Environment, ExePath);
        if (EnvBlock == 0)
        {
            Result = Sda->LastErrorCode;
            goto Cleanup;
        }
    }

    /*
     * Check if this is an EXE file or a COM file by looking
     * at the MZ signature:
     * 0x4D5A 'MZ': old signature (stored as 0x5A, 0x4D)
     * 0x5A4D 'ZM': new signature (stored as 0x4D, 0x5A)
     */
    ExeSignature = *(PWORD)ExeBuffer;
    if (ExeSignature == 'MZ' || ExeSignature == 'ZM')
    {
        /* EXE file */
        PIMAGE_DOS_HEADER Header;
        DWORD BaseSize;
        PDWORD RelocationTable;
        PWORD RelocWord;
        WORD RelocFactor;
        WORD i;

        /* Get the MZ header */
        Header = (PIMAGE_DOS_HEADER)ExeBuffer;

        /* Get the base size of the file, in paragraphs (rounded up) */
#if 0   // Normally this is not needed to check for the number of bytes in the last pages.
        BaseSize = ((((Header->e_cp - (Header->e_cblp != 0)) * 512) + Header->e_cblp) >> 4)
                    - Header->e_cparhdr;
#else
        // e_cp is the number of 512-byte blocks. 512 == (1 << 9)
        // so this corresponds to (1 << 5) number of paragraphs.
        //
        // For DOS compatibility we need to truncate BaseSize to a WORD value.
        // This fact is exploited by some EXEs which are bigger than 1 Mb while
        // being able to load on DOS, the main EXE code loads the remaining data.

        BaseSize = ((Header->e_cp << 5) - Header->e_cparhdr) & 0xFFFF;
#endif

        if (LoadType != DOS_LOAD_OVERLAY)
        {
            BOOLEAN LoadHigh = FALSE;
            DWORD TotalSize;

            /* Find the maximum amount of memory that can be allocated */
            DosAllocateMemory(0xFFFF, &MaxAllocSize);

            /* Compute the total needed size, in paragraphs */
            TotalSize = BaseSize + (sizeof(DOS_PSP) >> 4);

            /* We must have the required minimum amount of memory. If not, bail out. */
            if (MaxAllocSize < TotalSize + Header->e_minalloc)
            {
                Result = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            /* Check if the program should be loaded high */
            if (Header->e_minalloc == 0 && Header->e_maxalloc == 0)
            {
                /* Yes it should. Use all the available memory. */
                LoadHigh  = TRUE;
                TotalSize = MaxAllocSize;
            }
            else
            {
                /* Compute the maximum memory size that can be allocated */
                if (Header->e_maxalloc != 0)
                    TotalSize = min(TotalSize + Header->e_maxalloc, MaxAllocSize);
                else
                    TotalSize = MaxAllocSize; // Use all the available memory
            }

            /* Try to allocate that much memory */
            Segment = DosAllocateMemory((WORD)TotalSize, NULL);
            if (Segment == 0)
            {
                Result = Sda->LastErrorCode;
                goto Cleanup;
            }

            /* The process owns its memory */
            DosChangeMemoryOwner(Segment , Segment);
            DosChangeMemoryOwner(EnvBlock, Segment);

            /* Set INT 22h to the return address */
            ((PULONG)BaseAddress)[0x22] = ReturnAddress;

            /* Create the PSP and initialize it */
            DosCreatePsp(Segment, (WORD)TotalSize);
            DosInitPsp(Segment, EnvBlock, CommandLine, ExePath);

            /* Calculate the segment where the program should be loaded */
            if (!LoadHigh)
                LoadSegment = Segment + (sizeof(DOS_PSP) >> 4);
            else
                LoadSegment = Segment + TotalSize - BaseSize;

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
                      ExeBuffer + (Header->e_cparhdr << 4),
                      min(ExeBufferSize - (Header->e_cparhdr << 4), BaseSize << 4));

        /* Get the relocation table */
        RelocationTable = (PDWORD)(ExeBuffer + Header->e_lfarlc);

        /* Perform relocations */
        for (i = 0; i < Header->e_crlc; i++)
        {
            /* Get a pointer to the word that needs to be patched */
            RelocWord = (PWORD)SEG_OFF_TO_PTR(LoadSegment + HIWORD(RelocationTable[i]),
                                              LOWORD(RelocationTable[i]));

            /* Add the relocation factor to it */
            *RelocWord += RelocFactor;
        }

        /* Set the stack to the location from the header */
        FinalSS = LoadSegment + Header->e_ss;
        FinalSP = Header->e_sp;

        /* Set the code segment/pointer */
        FinalCS = LoadSegment + Header->e_cs;
        FinalIP = Header->e_ip;
    }
    else
    {
        /* COM file */

        if (LoadType != DOS_LOAD_OVERLAY)
        {
            /* Find the maximum amount of memory that can be allocated */
            DosAllocateMemory(0xFFFF, &MaxAllocSize);

            /* Make sure it's enough for the whole program and the PSP */
            if (((DWORD)MaxAllocSize << 4) < (ExeBufferSize + sizeof(DOS_PSP)))
            {
                Result = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            /* Allocate all of it */
            Segment = DosAllocateMemory(MaxAllocSize, NULL);
            if (Segment == 0)
            {
                Result = Sda->LastErrorCode;
                goto Cleanup;
            }

            /* The process owns its memory */
            DosChangeMemoryOwner(Segment , Segment);
            DosChangeMemoryOwner(EnvBlock, Segment);

            /* Set INT 22h to the return address */
            ((PULONG)BaseAddress)[0x22] = ReturnAddress;

            /* Create the PSP and initialize it */
            DosCreatePsp(Segment, MaxAllocSize);
            DosInitPsp(Segment, EnvBlock, CommandLine, ExePath);

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
                      ExeBuffer, ExeBufferSize);

        /* Set the stack to the last word of the segment */
        FinalSS = Segment;
        FinalSP = 0xFFFE;

        /*
         * Set the value on the stack to 0x0000, so that a near return
         * jumps to PSP:0000 which has the exit code.
         */
        *((LPWORD)SEG_OFF_TO_PTR(Segment, 0xFFFE)) = 0x0000;

        /* Set the code segment/pointer */
        FinalCS = Segment;
        FinalIP = 0x0100;
    }

    if (LoadType == DOS_LOAD_AND_EXECUTE)
    {
        /* Save the program state */
        if (Sda->CurrentPsp != SYSTEM_PSP)
        {
            /* Push the task state */
            DosSaveState();

#ifdef ADVANCED_DEBUGGING
            DPRINT1("Sda->CurrentPsp = 0x%04x; Old LastStack = 0x%08x, New LastStack = 0x%08x\n",
                   Sda->CurrentPsp, SEGMENT_TO_PSP(Sda->CurrentPsp)->LastStack, MAKELONG(getSP(), getSS()));
#endif

            /* Update the last stack in the PSP */
            SEGMENT_TO_PSP(Sda->CurrentPsp)->LastStack = MAKELONG(getSP(), getSS());
        }

        /* Set the initial segment registers */
        setDS(Segment);
        setES(Segment);

        /* Set the stack */
        setSS(FinalSS);
        setSP(FinalSP);

        /*
         * Set the other registers as in real DOS: some demos expect them so!
         * See http://www.fysnet.net/yourhelp.htm
         * and http://www.beroset.com/asm/showregs.asm
         */
        setDX(Segment);
        setDI(FinalSP);
        setBP(0x091E); // DOS base stack pointer relic value. In MS-DOS 5.0 and Windows' NTVDM it's 0x091C. This is in fact the old SP value inside DosData disk stack.
        setSI(FinalIP);

        setAX(0/*0xFFFF*/); // FIXME: fcbcode
        setBX(0/*0xFFFF*/); // FIXME: fcbcode
        setCX(0x00FF);

        /*
         * Keep critical flags, clear test flags (OF, SF, ZF, AF, PF, CF)
         * and explicitly set the interrupt flag.
         */
        setEFLAGS((getEFLAGS() & ~0x08D5) | 0x0200);

        /* Notify VDDs of process execution */
        VDDCreateUserHook(Segment);

        /* Execute */
        DosSetProcessContext(Segment);
        CpuExecute(FinalCS, FinalIP);
    }
    else if (LoadType == DOS_LOAD_ONLY)
    {
        ASSERT(Parameters);
        Parameters->StackLocation = MAKELONG(FinalSP, FinalSS);
        Parameters->EntryPoint    = MAKELONG(FinalIP, FinalCS);
    }

Cleanup:
    if (Result != ERROR_SUCCESS)
    {
        /* It was not successful, cleanup the DOS memory */
        if (EnvBlock) DosFreeMemory(EnvBlock);
        if (Segment)  DosFreeMemory(Segment);
    }

    return Result;
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
    DWORD FileSize;
    LPBYTE Address = NULL;
    CHAR FullPath[MAX_PATH];
    CHAR ShortFullPath[MAX_PATH];

    DPRINT1("DosLoadExecutable(%d, '%s', 0x%p, 0x%p, 0x%p)\n",
            LoadType, ExecutablePath, Parameters, CommandLine, Environment);

    /* Try to get the full path to the executable */
    if (GetFullPathNameA(ExecutablePath, sizeof(FullPath), FullPath, NULL))
    {
        /* Get the corresponding short path */
        if (GetShortPathNameA(FullPath, ShortFullPath, sizeof(ShortFullPath)))
        {
            /* Use the shortened full path from now on */
            ExecutablePath = ShortFullPath;
        }
    }

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

    Result = DosLoadExecutableInternal(LoadType,
                                       Address,
                                       FileSize,
                                       ExecutablePath,
                                       Parameters,
                                       CommandLine,
                                       Environment,
                                       ReturnAddress);

Cleanup:
    /* Unmap the file*/
    if (Address != NULL) UnmapViewOfFile(Address);

    /* Close the file mapping object */
    if (FileMapping != NULL) CloseHandle(FileMapping);

    /* Close the file handle */
    if (FileHandle != INVALID_HANDLE_VALUE) CloseHandle(FileHandle);

    return Result;
}

WORD DosCreateProcess(IN LPCSTR ProgramName,
                      IN PDOS_EXEC_PARAM_BLOCK Parameters,
                      IN DWORD ReturnAddress OPTIONAL)
{
    DWORD Result = ERROR_SUCCESS;
    DWORD BinaryType;

    /* Get the binary type */
    if (!GetBinaryTypeA(ProgramName, &BinaryType)) return GetLastError();

    /* Check the type of the program */
    switch (BinaryType)
    {
        /* Those are handled by NTVDM */
        case SCS_WOW_BINARY:
        {
            static const PCSTR AppName = "\"%ProgramFiles%\\otvdm\\otvdmw.exe\" ";

            STARTUPINFOA si;
            PROCESS_INFORMATION pi;
            union { DWORD Size; NTSTATUS Status; } Ret;
            CHAR ExpName[MAX_PATH];

            Ret.Size = ExpandEnvironmentStringsA(AppName, ExpName, _countof(ExpName));
            if ((Ret.Size == 0) || (Ret.Size > _countof(ExpName)))
            {
                /* We failed or buffer too small, fall back to DOS execution */
                goto RunAsDOS;
            }
            Ret.Size--; // Remove NULL-terminator from count

            /* Add double-quotes before and after ProgramName */
            Ret.Status = RtlStringCchPrintfA(ExpName + Ret.Size, _countof(ExpName) - Ret.Size,
                                             "\"%s\"", ProgramName);
            if (!NT_SUCCESS(Ret.Status))
            {
                /* We failed or buffer too small, fall back to DOS execution */
                goto RunAsDOS;
            }

            ZeroMemory(&pi, sizeof(pi));
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);

            /* Create the process */
            if (CreateProcessA(NULL,    // No Application Name
                               ExpName, // Just our Command Line
                               NULL,    // Cannot inherit Process Handle
                               NULL,    // Cannot inherit Thread Handle
                               FALSE,   // No handle inheritance
                               0,       // No extra creation flags
                               NULL,    // No environment block
                               NULL,    // No starting directory 
                               &si,
                               &pi))
            {
                /* Close the handles */
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
                break;
            }
            else
            {
                /* Retrieve the actual path to the "Program Files" directory for displaying the error */
                ExpandEnvironmentStringsA("%ProgramFiles%", ExpName, _countof(ExpName));

                DisplayMessage(L"Trying to load '%S'.\n"
                               L"WOW16 applications are not supported internally by NTVDM at the moment.\n"
                               L"Consider installing WineVDM from the ReactOS Applications Manager in\n'%S'.\n\n"
                               L"Click on OK to continue.",
                               ProgramName, ExpName);
            }
            // Fall through
        }
        RunAsDOS:
        case SCS_DOS_BINARY:
        {
            /* Load the executable */
            Result = DosLoadExecutable(DOS_LOAD_AND_EXECUTE,
                                       ProgramName,
                                       Parameters,
                                       NULL,
                                       NULL,
                                       ReturnAddress);
            if (Result != ERROR_SUCCESS)
            {
                DisplayMessage(L"Could not load '%S'. Error: %u", ProgramName, Result);
            }

            break;
        }

        /* Not handled by NTVDM */
        default:
        {
            LPSTR Environment = NULL;
            CHAR CmdLine[MAX_PATH + DOS_CMDLINE_LENGTH + 1];
            LPSTR CmdLinePtr;
            ULONG CmdLineSize;

            /* Did the caller specify an environment segment? */
            if (Parameters->Environment)
            {
                /* Yes, use it instead of the parent one */
                Environment = (LPSTR)SEG_OFF_TO_PTR(Parameters->Environment, 0);
            }

            /*
             * Convert the DOS command line to Win32-compatible format, by concatenating
             * the program name with the converted command line.
             * Format of the DOS command line: 1 byte for size; 127 bytes for contents.
             */
            CmdLinePtr = CmdLine;
            strncpy(CmdLinePtr, ProgramName, MAX_PATH); // Concatenate the program name
            CmdLinePtr += strlen(CmdLinePtr);
            *CmdLinePtr++ = ' ';                        // Add separating space

            CmdLineSize = min(*(PBYTE)FAR_POINTER(Parameters->CommandLine), DOS_CMDLINE_LENGTH);
            RtlCopyMemory(CmdLinePtr,
                          (LPSTR)FAR_POINTER(Parameters->CommandLine) + 1,
                          CmdLineSize);
            /* NULL-terminate it */
            CmdLinePtr[CmdLineSize] = '\0';

            /* Remove any trailing return carriage character and NULL-terminate the command line */
            while (*CmdLinePtr && *CmdLinePtr != '\r' && *CmdLinePtr != '\n') CmdLinePtr++;
            *CmdLinePtr = '\0';

            Result = DosStartProcess32(ProgramName, CmdLine,
                                       Environment, ReturnAddress,
                                       TRUE);
            if (Result != ERROR_SUCCESS)
            {
                DisplayMessage(L"Could not load 32-bit '%S'. Error: %u", ProgramName, Result);
            }

            break;
        }
    }

    return Result;
}

VOID DosTerminateProcess(WORD Psp, BYTE ReturnCode, WORD KeepResident)
{
    WORD McbSegment = SysVars->FirstMcb;
    PDOS_MCB CurrentMcb;
    LPDWORD IntVecTable = (LPDWORD)((ULONG_PTR)BaseAddress);
    PDOS_PSP PspBlock = SEGMENT_TO_PSP(Psp);
    LPWORD Stack;
    BYTE TerminationType;

    DPRINT("DosTerminateProcess: Psp 0x%04X, ReturnCode 0x%02X, KeepResident 0x%04X\n",
           Psp, ReturnCode, KeepResident);

    /* Notify VDDs of process termination */
    VDDTerminateUserHook(Psp);

    /* Check if this PSP is its own parent */
    if (PspBlock->ParentPsp == Psp) goto Done;

    if (KeepResident == 0)
    {
        WORD i;
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

    /* Update the current PSP with the parent's one */
    if (Psp == Sda->CurrentPsp)
    {
        DosSetProcessContext(PspBlock->ParentPsp);
        if (Sda->CurrentPsp == SYSTEM_PSP)
        {
            // NOTE: we can also use the DOS BIOS exit code.
            CpuUnsimulate();
            return;
        }
    }

    /* Save the return code - Normal termination or TSR */
    TerminationType = (KeepResident != 0 ? 0x03 : 0x00);
    Sda->ErrorLevel = MAKEWORD(ReturnCode, TerminationType);

#ifdef ADVANCED_DEBUGGING
    DPRINT1("PspBlock->ParentPsp = 0x%04x; Sda->CurrentPsp = 0x%04x\n",
           PspBlock->ParentPsp, Sda->CurrentPsp);
#endif

    if (Sda->CurrentPsp != SYSTEM_PSP)
    {
#ifdef ADVANCED_DEBUGGING
        DPRINT1("Sda->CurrentPsp = 0x%04x; Old SS:SP = %04X:%04X going to be LastStack = 0x%08x\n",
               Sda->CurrentPsp, getSS(), getSP(), SEGMENT_TO_PSP(Sda->CurrentPsp)->LastStack);
#endif

        /* Restore the parent's stack */
        setSS(HIWORD(SEGMENT_TO_PSP(Sda->CurrentPsp)->LastStack));
        setSP(LOWORD(SEGMENT_TO_PSP(Sda->CurrentPsp)->LastStack));

        /* Pop the task state */
        DosRestoreState();
    }

    /* Return control to the parent process */
    Stack = (LPWORD)SEG_OFF_TO_PTR(getSS(), getSP());
    Stack[STACK_CS] = HIWORD(PspBlock->TerminateAddress);
    Stack[STACK_IP] = LOWORD(PspBlock->TerminateAddress);
}

/* EOF */
