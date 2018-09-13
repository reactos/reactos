/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    cmdline.h

Abstract:

    This file contains the internal structures and definitions used
    by command line input and editing.

Author:

    Therese Stowell (thereses) 15-Nov-1991

Revision History:

--*/

typedef struct _COMMAND {
    USHORT CommandLength;
    WCHAR Command[1];
} COMMAND, *PCOMMAND;

typedef
ULONG
(*PCLE_POPUP_INPUT_ROUTINE)(
    IN PVOID CookedReadData,
    IN PCSR_API_MSG WaitReplyMessage,
    IN PCSR_THREAD WaitingThread,
    IN BOOLEAN WaitRoutine
    );

/*
 * CLE_POPUP Flags
 */
#define CLEPF_FALSE_UNICODE 0x0001

typedef struct _CLE_POPUP {
    LIST_ENTRY ListLink;    // pointer to next popup
    SMALL_RECT Region;      // region popup occupies
    WORD  Attributes;       // text attributes
    WORD  Flags;            // CLEPF_ flags
    PCHAR_INFO OldContents; // contains data under popup
    SHORT BottomIndex;      // number of command displayed on last line of popup
    SHORT CurrentCommand;
    WCHAR NumberBuffer[6];
    SHORT NumberRead;
    PCLE_POPUP_INPUT_ROUTINE PopupInputRoutine; // routine to call when input is received
#if defined(FE_SB)
    COORD OldScreenSize;
#endif
} CLE_POPUP, *PCLE_POPUP;

#define POPUP_SIZE_X(POPUP) (SHORT)(((POPUP)->Region.Right - (POPUP)->Region.Left - 1))
#define POPUP_SIZE_Y(POPUP) (SHORT)(((POPUP)->Region.Bottom - (POPUP)->Region.Top - 1))
#define COMMAND_NUMBER_SIZE 8   // size of command number buffer


/*
 * COMMAND_HISTORY Flags
 */
#define CLE_ALLOCATED 0x00000001
#define CLE_RESET     0x00000002

typedef struct _COMMAND_HISTORY {
    DWORD Flags;
    LIST_ENTRY ListLink;
    PWCHAR AppName;
    SHORT NumberOfCommands;
    SHORT LastAdded;
    SHORT LastDisplayed;
    SHORT FirstCommand;     // circular buffer
    SHORT MaximumNumberOfCommands;
    HANDLE ProcessHandle;
    LIST_ENTRY PopupList;    // pointer to top-level popup
    PCOMMAND Commands[1];
} COMMAND_HISTORY, *PCOMMAND_HISTORY;

#define DEFAULT_NUMBER_OF_COMMANDS 25
#define DEFAULT_NUMBER_OF_BUFFERS 4

typedef struct _COOKED_READ_DATA {
    PINPUT_INFORMATION InputInfo;
    PSCREEN_INFORMATION ScreenInfo;
    PCONSOLE_INFORMATION Console;
    HANDLE_DATA TempHandle;
    ULONG UserBufferSize;   // doubled size in ansi case
    PWCHAR UserBuffer;
    ULONG BufferSize;
    ULONG BytesRead;
    ULONG CurrentPosition;  // char position, not byte position
    PWCHAR BufPtr;
    PWCHAR BackupLimit;
    COORD OriginalCursorPosition;
    DWORD NumberOfVisibleChars;
    PCOMMAND_HISTORY CommandHistory;
    BOOLEAN Echo;
    BOOLEAN Processed;
    BOOLEAN Line;
    BOOLEAN InsertMode;
    PCONSOLE_PER_PROCESS_DATA ProcessData;
    HANDLE HandleIndex;
    PWCHAR ExeName;
    USHORT ExeNameLength;   // in bytes
    ULONG CtrlWakeupMask;
    ULONG ControlKeyState;
} COOKED_READ_DATA, *PCOOKED_READ_DATA;

#define COMMAND_NUM_TO_INDEX(NUM,CMDHIST) (SHORT)(((NUM+(CMDHIST)->FirstCommand)%((CMDHIST)->MaximumNumberOfCommands)))
#define COMMAND_INDEX_TO_NUM(INDEX,CMDHIST) (SHORT)(((INDEX+((CMDHIST)->MaximumNumberOfCommands)-(CMDHIST)->FirstCommand)%((CMDHIST)->MaximumNumberOfCommands)))

/*
 * COMMAND_IND_NEXT and COMMAND_IND_PREV go to the next and prev command
 * COMMAND_IND_INC  and COMMAND_IND_DEC  go to the next and prev slots
 *
 * Don't get the two confused - it matters when the cmd history is not full!
 */

#define COMMAND_IND_PREV(IND,CMDHIST)               \
{                                                   \
    if (IND <= 0) {                                 \
        IND = (CMDHIST)->NumberOfCommands;          \
    }                                               \
    IND--;                                          \
}

#define COMMAND_IND_NEXT(IND,CMDHIST)               \
{                                                   \
    ++IND;                                          \
    if (IND >= (CMDHIST)->NumberOfCommands){        \
        IND = 0;                                    \
    }                                               \
}

#define COMMAND_IND_DEC(IND,CMDHIST)                \
{                                                   \
    if (IND <= 0) {                                 \
        IND = (CMDHIST)->MaximumNumberOfCommands;   \
    }                                               \
    IND--;                                          \
}

#define COMMAND_IND_INC(IND,CMDHIST)                \
{                                                   \
    ++IND;                                          \
    if (IND >= (CMDHIST)->MaximumNumberOfCommands){ \
        IND = 0;                                    \
    }                                               \
}

#define CLE_NO_POPUPS(COMMAND_HISTORY) (&(COMMAND_HISTORY)->PopupList == (COMMAND_HISTORY)->PopupList.Blink)

//
// aliases are grouped per console, per exe.
//

typedef struct _ALIAS {
    LIST_ENTRY ListLink;
    USHORT SourceLength; // in bytes
    USHORT TargetLength; // in bytes
    PWCHAR Source;
    PWCHAR Target;
} ALIAS, *PALIAS;

typedef struct _EXE_ALIAS_LIST {
    LIST_ENTRY ListLink;
    USHORT ExeLength;   // in bytes
    PWCHAR ExeName;
    LIST_ENTRY AliasList;
} EXE_ALIAS_LIST, *PEXE_ALIAS_LIST;

NTSTATUS
ProcessCommandLine(
    IN PCOOKED_READ_DATA CookedReadData,
    IN WCHAR Char,
    IN DWORD KeyState,
    IN PCSR_API_MSG WaitReplyMessage,
    IN PCSR_THREAD WaitingThread,
    IN BOOLEAN WaitRoutine
    );

VOID
DeleteCommandLine(
    IN OUT PCOOKED_READ_DATA CookedReadData,
    IN BOOL UpdateFields
    );

VOID
RedrawCommandLine(
    IN PCOOKED_READ_DATA CookedReadData
    );

VOID
EmptyCommandHistory(
    IN PCOMMAND_HISTORY CommandHistory
    );

PCOMMAND_HISTORY
ReallocCommandHistory(
    IN PCONSOLE_INFORMATION Console,
    IN PCOMMAND_HISTORY CurrentCommandHistory,
    IN DWORD NumCommands
    );

PCOMMAND_HISTORY
FindExeCommandHistory(
    IN PCONSOLE_INFORMATION Console,
    IN PVOID AppName,
    IN DWORD AppNameLength,
    IN BOOLEAN UnicodeExe
    );

PCOMMAND_HISTORY
FindCommandHistory(
    IN PCONSOLE_INFORMATION Console,
    IN HANDLE ProcessHandle
    );

ULONG
RetrieveNumberOfSpaces(
    IN SHORT OriginalCursorPositionX,
    IN PWCHAR Buffer,
    IN ULONG CurrentPosition
#if defined(FE_SB)
    ,
    IN PCONSOLE_INFORMATION Console,
    IN DWORD CodePage
#endif
    );

ULONG
RetrieveTotalNumberOfSpaces(
    IN SHORT OriginalCursorPositionX,
    IN PWCHAR Buffer,
    IN ULONG CurrentPosition
#if defined(FE_SB)
    ,
    IN PCONSOLE_INFORMATION Console
#endif
    );

NTSTATUS
GetChar(
    IN PINPUT_INFORMATION InputInfo,
    OUT PWCHAR Char,
    IN BOOLEAN Wait,
    IN PCONSOLE_INFORMATION Console,
    IN PHANDLE_DATA HandleData,
    IN PCSR_API_MSG Message OPTIONAL,
    IN CSR_WAIT_ROUTINE WaitRoutine OPTIONAL,
    IN PVOID WaitParameter OPTIONAL,
    IN ULONG WaitParameterLength  OPTIONAL,
    IN BOOLEAN WaitBlockExists OPTIONAL,
    OUT PBOOLEAN CommandLineEditingKeys OPTIONAL,
    OUT PBOOLEAN CommandLinePopupKeys OPTIONAL,
    OUT PBOOLEAN EnableScrollMode OPTIONAL,
    OUT PDWORD KeyState OPTIONAL
    );

BOOL
IsCommandLinePopupKey(
    IN OUT PKEY_EVENT_RECORD KeyEvent
    );

BOOL
IsCommandLineEditingKey(
    IN OUT PKEY_EVENT_RECORD KeyEvent
    );

VOID
CleanUpPopups(
    IN PCOOKED_READ_DATA CookedReadData
    );

BOOL
ProcessCookedReadInput(
    IN PCOOKED_READ_DATA CookedReadData,
    IN WCHAR Char,
    IN DWORD KeyState,
    OUT PNTSTATUS Status
    );

VOID
DrawCommandListBorder(
    IN PCLE_POPUP Popup,
    IN PSCREEN_INFORMATION ScreenInfo
    );

PCOMMAND
GetLastCommand(
    IN PCOMMAND_HISTORY CommandHistory
    );

SHORT
FindMatchingCommand(
    IN PCOMMAND_HISTORY CommandHistory,
    IN PWCHAR CurrentCommand,
    IN ULONG CurrentCommandLength,
    IN SHORT CurrentIndex,
    IN DWORD Flags
    );

#define FMCFL_EXACT_MATCH   1
#define FMCFL_JUST_LOOKING  2

NTSTATUS
CommandNumberPopup(
    IN PCOOKED_READ_DATA CookedReadData,
    IN PCSR_API_MSG WaitReplyMessage,
    IN PCSR_THREAD WaitingThread,
    IN BOOLEAN WaitRoutine
    );

BOOLEAN
CookedReadWaitRoutine(
    IN PLIST_ENTRY WaitQueue,
    IN PCSR_THREAD WaitingThread,
    IN PCSR_API_MSG WaitReplyMessage,
    IN PVOID WaitParameter,
    IN PVOID SatisfyParameter1,
    IN PVOID SatisfyParameter2,
    IN ULONG WaitFlags
    );

VOID
ReadRectFromScreenBuffer(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN COORD SourcePoint,
    IN PCHAR_INFO Target,
    IN COORD TargetSize,
    IN PSMALL_RECT TargetRect
    );

NTSTATUS
WriteCharsFromInput(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PWCHAR lpBufferBackupLimit,
    IN PWCHAR lpBuffer,
    IN PWCHAR lpString,
    IN OUT PDWORD NumBytes,
    OUT PLONG NumSpaces OPTIONAL,
    IN SHORT OriginalXPosition,
    IN DWORD dwFlags,
    OUT PSHORT ScrollY OPTIONAL
    );

//
// Values for WriteChars(),WriteCharsFromInput() dwFlags
//
#define WC_DESTRUCTIVE_BACKSPACE 0x01
#define WC_KEEP_CURSOR_VISIBLE   0x02
#define WC_ECHO                  0x04
#define WC_FALSIFY_UNICODE       0x08
#define WC_LIMIT_BACKSPACE       0x10


VOID
DrawCommandListPopup(
    IN PCLE_POPUP Popup,
    IN SHORT CurrentCommand,
    IN PCOMMAND_HISTORY CommandHistory,
    IN PSCREEN_INFORMATION ScreenInfo
    );

VOID
UpdateCommandListPopup(
    IN SHORT Delta,
    IN OUT PSHORT CurrentCommand,
    IN PCOMMAND_HISTORY CommandHistory,
    IN PCLE_POPUP Popup,
    IN PSCREEN_INFORMATION ScreenInfo,
    IN DWORD Flags
    );

#define UCLP_WRAP   1


//
// InitExtendedEditKey
// If lpwstr is NULL, the default value will be used.
//
VOID InitExtendedEditKeys(CONST ExtKeyDefBuf* lpbuf);

//
// IsPauseKey
// returns TRUE if pKeyEvent is pause.
// The default key is Ctrl-S if extended edit keys are not specified.
//
BOOL IsPauseKey(IN PKEY_EVENT_RECORD pKeyEvent);


//
// Word delimiters
//

#define IS_WORD_DELIM(wch)  ((wch) == L' ' || (gaWordDelimChars[0] && IsWordDelim(wch)))

extern WCHAR gaWordDelimChars[];
extern CONST WCHAR gaWordDelimCharsDefault[];
extern BOOL IsWordDelim(WCHAR);

#define WORD_DELIM_MAX  32

