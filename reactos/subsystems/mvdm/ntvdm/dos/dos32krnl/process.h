/*
 * COPYRIGHT:       GPLv2 - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            dos/dos32krnl/process.h
 * PURPOSE:         DOS32 Processes
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* DEFINITIONS ****************************************************************/

#define DOS_CMDLINE_LENGTH   127
#define DOS_PROGRAM_NAME_TAG 0x0001

#define SEGMENT_TO_PSP(seg) ((PDOS_PSP)SEG_OFF_TO_PTR((seg), 0))

typedef enum
{
    DOS_LOAD_AND_EXECUTE = 0x00,
    DOS_LOAD_ONLY        = 0x01,
    DOS_LOAD_OVERLAY     = 0x03
} DOS_EXEC_TYPE;

#pragma pack(push, 1)

typedef struct _DOS_PSP
{
    BYTE Exit[2];
    WORD LastParagraph;
    BYTE Reserved0[6];
    DWORD TerminateAddress;
    DWORD BreakAddress;
    DWORD CriticalAddress;
    WORD ParentPsp;
    BYTE HandleTable[20];
    WORD EnvBlock;
    DWORD LastStack;
    WORD HandleTableSize;
    DWORD HandleTablePtr;
    DWORD PreviousPsp;
    DWORD Reserved1;
    WORD DosVersion;
    BYTE Reserved2[14];
    BYTE FarCall[3];
    BYTE Reserved3[9];
    DOS_FCB Fcb;
    BYTE CommandLineSize;
    CHAR CommandLine[DOS_CMDLINE_LENGTH];
} DOS_PSP, *PDOS_PSP;

typedef struct _DOS_EXEC_PARAM_BLOCK
{
    union
    {
        struct
        {
            /* Input variables */
            WORD Environment;
            DWORD CommandLine;
            DWORD FirstFcb;
            DWORD SecondFcb;

            /* Output variables */
            DWORD StackLocation;
            DWORD EntryPoint;
        };

        struct
        {
            WORD Segment;
            WORD RelocationFactor;
        } Overlay;
    };
} DOS_EXEC_PARAM_BLOCK, *PDOS_EXEC_PARAM_BLOCK;

typedef struct _DOS_REGISTER_STATE
{
    WORD AX, BX, CX, DX, SI, DI, BP, DS, ES;
//  WORD IP, CS, Flags; // They are supposed to be already
                        // pushed on stack by the DOS caller.
} DOS_REGISTER_STATE, *PDOS_REGISTER_STATE;

#pragma pack(pop)

/* FUNCTIONS ******************************************************************/

VOID DosClonePsp(WORD DestSegment, WORD SourceSegment);
VOID DosCreatePsp(WORD Segment, WORD ProgramSize);
VOID DosSetProcessContext(WORD Segment);

DWORD DosLoadExecutable
(
    IN DOS_EXEC_TYPE LoadType,
    IN LPCSTR ExecutablePath,
    IN PDOS_EXEC_PARAM_BLOCK Parameters,
    IN LPCSTR CommandLine OPTIONAL,
    IN LPCSTR Environment OPTIONAL,
    IN DWORD ReturnAddress OPTIONAL
);

DWORD DosStartProcess(
    IN LPCSTR ExecutablePath,
    IN LPCSTR CommandLine,
    IN LPCSTR Environment OPTIONAL,
    IN DWORD ReturnAddress OPTIONAL
);

WORD DosCreateProcess
(
    LPCSTR ProgramName,
    PDOS_EXEC_PARAM_BLOCK Parameters,
    IN DWORD ReturnAddress OPTIONAL
);

VOID DosTerminateProcess(WORD Psp, BYTE ReturnCode, WORD KeepResident);
