/*
 * CSRSS Console management structures.
 */

#ifndef __CSRCONS_H__
#define __CSRCONS_H__

#include <drivers/blue/ntddblue.h>



#define CSRSS_MAX_WRITE_CONSOLE                 (LPC_MAX_DATA_LENGTH - CSR_API_MESSAGE_HEADER_SIZE(CSRSS_WRITE_CONSOLE))
#define CSRSS_MAX_WRITE_CONSOLE_OUTPUT_CHAR     (LPC_MAX_DATA_LENGTH - CSR_API_MESSAGE_HEADER_SIZE(CSRSS_WRITE_CONSOLE_OUTPUT_CHAR))
#define CSRSS_MAX_WRITE_CONSOLE_OUTPUT_ATTRIB   (LPC_MAX_DATA_LENGTH - CSR_API_MESSAGE_HEADER_SIZE(CSRSS_WRITE_CONSOLE_OUTPUT_ATTRIB))
#define CSRSS_MAX_READ_CONSOLE_OUTPUT_CHAR      (LPC_MAX_DATA_LENGTH - CSR_API_MESSAGE_HEADER_SIZE(CSRSS_READ_CONSOLE_OUTPUT_CHAR))
#define CSRSS_MAX_READ_CONSOLE_OUTPUT_ATTRIB    (LPC_MAX_DATA_LENGTH - CSR_API_MESSAGE_HEADER_SIZE(CSRSS_READ_CONSOLE_OUTPUT_ATTRIB))

#define WRITE_CONSOLE                   (0x2)
#define READ_CONSOLE                    (0x3)
#define ALLOC_CONSOLE                   (0x4)
#define FREE_CONSOLE                    (0x5)
#define SCREEN_BUFFER_INFO              (0x7)
#define SET_CURSOR                      (0x8)
#define FILL_OUTPUT                     (0x9)
#define READ_INPUT                      (0xA)
#define WRITE_CONSOLE_OUTPUT_CHAR       (0xB)
#define WRITE_CONSOLE_OUTPUT_ATTRIB     (0xC)
#define FILL_OUTPUT_ATTRIB              (0xD)
#define GET_CURSOR_INFO                 (0xE)
#define SET_CURSOR_INFO                 (0xF)
#define SET_ATTRIB                      (0x10)
#define GET_CONSOLE_MODE                (0x11)
#define SET_CONSOLE_MODE                (0x12)
#define CREATE_SCREEN_BUFFER            (0x13)
#define SET_SCREEN_BUFFER               (0x14)
#define SET_TITLE                       (0x15)
#define GET_TITLE                       (0x16)
#define WRITE_CONSOLE_OUTPUT            (0x17)
#define FLUSH_INPUT_BUFFER              (0x18)
#define SCROLL_CONSOLE_SCREEN_BUFFER    (0x19)
#define READ_CONSOLE_OUTPUT_CHAR        (0x1A)
#define READ_CONSOLE_OUTPUT_ATTRIB      (0x1B)
#define GET_NUM_INPUT_EVENTS            (0x1C)
#define PEEK_CONSOLE_INPUT              (0x21)
#define READ_CONSOLE_OUTPUT             (0x22)
#define WRITE_CONSOLE_INPUT             (0x23)
#define GET_INPUT_HANDLE                (0x24)
#define GET_OUTPUT_HANDLE               (0x25)
#define SETGET_CONSOLE_HW_STATE         (0x29)
#define GET_CONSOLE_WINDOW              (0x2A)
#define SET_CONSOLE_ICON                (0x2E)
#define GET_CONSOLE_CP                  (0x31)
#define SET_CONSOLE_CP                  (0x32)
#define GET_CONSOLE_OUTPUT_CP           (0x33)
#define SET_CONSOLE_OUTPUT_CP           (0x34)
#define ADD_CONSOLE_ALIAS               (0x38)
#define GET_CONSOLE_ALIAS               (0x39)
#define GET_ALL_CONSOLE_ALIASES         (0x3A)
#define GET_ALL_CONSOLE_ALIASES_LENGTH  (0x3B)
#define GET_CONSOLE_ALIASES_EXES        (0x3C)
#define GET_CONSOLE_ALIASES_EXES_LENGTH (0x3D)
#define GENERATE_CTRL_EVENT             (0x3E)
#define SET_SCREEN_BUFFER_SIZE          (0x40)
#define GET_CONSOLE_SELECTION_INFO      (0x41)
#define GET_COMMAND_HISTORY_LENGTH      (0x42)
#define GET_COMMAND_HISTORY             (0x43)
#define EXPUNGE_COMMAND_HISTORY         (0x44)
#define SET_HISTORY_NUMBER_COMMANDS     (0x45)
#define GET_HISTORY_INFO                (0x46)
#define SET_HISTORY_INFO                (0x47)





typedef struct
{
    HANDLE ConsoleHandle;
    BOOL Unicode;
    ULONG NrCharactersToWrite;
    ULONG NrCharactersWritten;
    HANDLE UnpauseEvent;
    BYTE Buffer[0];
} CSRSS_WRITE_CONSOLE, *PCSRSS_WRITE_CONSOLE;

typedef struct
{
    HANDLE ConsoleHandle;
    BOOL Unicode;
    WORD NrCharactersToRead;
    WORD NrCharactersRead;
    HANDLE EventHandle;
    PVOID Buffer;
    UNICODE_STRING ExeName;
    DWORD CtrlWakeupMask;
    DWORD ControlKeyState;
} CSRSS_READ_CONSOLE, *PCSRSS_READ_CONSOLE;

typedef struct
{
    PCONTROLDISPATCHER CtrlDispatcher;
    BOOLEAN ConsoleNeeded;
    INT ShowCmd;
    HANDLE Console;
    HANDLE InputHandle;
    HANDLE OutputHandle;
} CSRSS_ALLOC_CONSOLE, *PCSRSS_ALLOC_CONSOLE;

typedef struct
{
    ULONG Dummy;
} CSRSS_FREE_CONSOLE, *PCSRSS_FREE_CONSOLE;

typedef struct
{
    HANDLE ConsoleHandle;
    CONSOLE_SCREEN_BUFFER_INFO Info;
} CSRSS_SCREEN_BUFFER_INFO, *PCSRSS_SCREEN_BUFFER_INFO;

typedef struct
{
    HANDLE ConsoleHandle;
    COORD Position;
} CSRSS_SET_CURSOR, *PCSRSS_SET_CURSOR;

typedef struct
{
    HANDLE ConsoleHandle;
    BOOL Unicode;
    union
    {
        CHAR AsciiChar;
        WCHAR UnicodeChar;
    } Char;
    COORD Position;
    WORD Length;
    ULONG NrCharactersWritten;
} CSRSS_FILL_OUTPUT, *PCSRSS_FILL_OUTPUT;

typedef struct
{
    HANDLE ConsoleHandle;
    CHAR Attribute;
    COORD Coord;
    WORD Length;
} CSRSS_FILL_OUTPUT_ATTRIB, *PCSRSS_FILL_OUTPUT_ATTRIB;

typedef struct
{
    HANDLE ConsoleHandle;
    BOOL Unicode;
    INPUT_RECORD Input;
    BOOL MoreEvents;
    HANDLE Event;
} CSRSS_READ_INPUT, *PCSRSS_READ_INPUT;

typedef struct
{
    HANDLE ConsoleHandle;
    BOOL Unicode;
    WORD Length;
    COORD Coord;
    COORD EndCoord;
    ULONG NrCharactersWritten;
    CHAR String[0];
} CSRSS_WRITE_CONSOLE_OUTPUT_CHAR, *PCSRSS_WRITE_CONSOLE_OUTPUT_CHAR;

typedef struct
{
    HANDLE ConsoleHandle;
    WORD Length;
    COORD Coord;
    COORD EndCoord;
    WORD Attribute[0];
} CSRSS_WRITE_CONSOLE_OUTPUT_ATTRIB, *PCSRSS_WRITE_CONSOLE_OUTPUT_ATTRIB;

typedef struct
{
    HANDLE ConsoleHandle;
    CONSOLE_CURSOR_INFO Info;
} CSRSS_GET_CURSOR_INFO, *PCSRSS_GET_CURSOR_INFO;

typedef struct
{
    HANDLE ConsoleHandle;
    CONSOLE_CURSOR_INFO Info;
} CSRSS_SET_CURSOR_INFO, *PCSRSS_SET_CURSOR_INFO;

typedef struct
{
    HANDLE ConsoleHandle;
    WORD Attrib;
} CSRSS_SET_ATTRIB, *PCSRSS_SET_ATTRIB;

typedef struct
{
    HANDLE ConsoleHandle;
    DWORD Mode;
} CSRSS_SET_CONSOLE_MODE, *PCSRSS_SET_CONSOLE_MODE;

typedef struct
{
    HANDLE ConsoleHandle;
    DWORD ConsoleMode;
} CSRSS_GET_CONSOLE_MODE, *PCSRSS_GET_CONSOLE_MODE;

typedef struct
{
    DWORD Access;
    DWORD ShareMode;
    BOOL Inheritable;
    HANDLE OutputHandle;  /* handle to newly created screen buffer */
} CSRSS_CREATE_SCREEN_BUFFER, *PCSRSS_CREATE_SCREEN_BUFFER;

typedef struct
{
    HANDLE OutputHandle;  /* handle to screen buffer to switch to */
} CSRSS_SET_SCREEN_BUFFER, *PCSRSS_SET_SCREEN_BUFFER;

typedef struct
{
    DWORD Length;
    PWCHAR Title;
} CSRSS_SET_TITLE, *PCSRSS_SET_TITLE;

typedef struct
{
    DWORD Length;
    PWCHAR Title;
} CSRSS_GET_TITLE, *PCSRSS_GET_TITLE;

typedef struct
{
    HANDLE ConsoleHandle;
    BOOL Unicode;
    COORD BufferSize;
    COORD BufferCoord;
    SMALL_RECT WriteRegion;
    CHAR_INFO* CharInfo;
} CSRSS_WRITE_CONSOLE_OUTPUT, *PCSRSS_WRITE_CONSOLE_OUTPUT;

typedef struct
{
    HANDLE ConsoleInput;
} CSRSS_FLUSH_INPUT_BUFFER, *PCSRSS_FLUSH_INPUT_BUFFER;

typedef struct
{
    HANDLE ConsoleHandle;
    BOOL Unicode;
    SMALL_RECT ScrollRectangle;
    BOOLEAN UseClipRectangle;
    SMALL_RECT ClipRectangle;
    COORD DestinationOrigin;
    CHAR_INFO Fill;
} CSRSS_SCROLL_CONSOLE_SCREEN_BUFFER, *PCSRSS_SCROLL_CONSOLE_SCREEN_BUFFER;

typedef struct
{
    HANDLE ConsoleHandle;
    BOOL Unicode;
    DWORD NumCharsToRead;
    COORD ReadCoord;
    COORD EndCoord;
    DWORD CharsRead;
    CHAR String[0];
} CSRSS_READ_CONSOLE_OUTPUT_CHAR, *PCSRSS_READ_CONSOLE_OUTPUT_CHAR;

typedef struct
{
    HANDLE ConsoleHandle;
    DWORD NumAttrsToRead;
    COORD ReadCoord;
    COORD EndCoord;
    WORD Attribute[0];
} CSRSS_READ_CONSOLE_OUTPUT_ATTRIB, *PCSRSS_READ_CONSOLE_OUTPUT_ATTRIB;


typedef struct
{
    HANDLE ConsoleHandle;
    BOOL Unicode;
    DWORD Length;
    INPUT_RECORD* InputRecord;
} CSRSS_PEEK_CONSOLE_INPUT, *PCSRSS_PEEK_CONSOLE_INPUT;

typedef struct
{
    HANDLE ConsoleHandle;
    BOOL Unicode;
    COORD BufferSize;
    COORD BufferCoord;
    SMALL_RECT ReadRegion;
    CHAR_INFO* CharInfo;
} CSRSS_READ_CONSOLE_OUTPUT, *PCSRSS_READ_CONSOLE_OUTPUT;

typedef struct
{
    HANDLE ConsoleHandle;
    BOOL Unicode;
    DWORD Length;
    INPUT_RECORD* InputRecord;
} CSRSS_WRITE_CONSOLE_INPUT, *PCSRSS_WRITE_CONSOLE_INPUT;

typedef struct
{
    DWORD Access;
    BOOL Inheritable;
    HANDLE Handle;
    DWORD ShareMode;
} CSRSS_GET_INPUT_HANDLE, *PCSRSS_GET_INPUT_HANDLE,
  CSRSS_GET_OUTPUT_HANDLE, *PCSRSS_GET_OUTPUT_HANDLE;


#define CONSOLE_HARDWARE_STATE_GET 0
#define CONSOLE_HARDWARE_STATE_SET 1

#define CONSOLE_HARDWARE_STATE_GDI_MANAGED 0
#define CONSOLE_HARDWARE_STATE_DIRECT      1

typedef struct
{
    HANDLE ConsoleHandle;
    DWORD SetGet; /* 0=get; 1=set */
    DWORD State;
} CSRSS_SETGET_CONSOLE_HW_STATE, *PCSRSS_SETGET_CONSOLE_HW_STATE;

typedef struct
{
    HWND   WindowHandle;
} CSRSS_GET_CONSOLE_WINDOW, *PCSRSS_GET_CONSOLE_WINDOW;

typedef struct
{
    HICON  WindowIcon;
} CSRSS_SET_CONSOLE_ICON, *PCSRSS_SET_CONSOLE_ICON;

typedef struct
{
    ULONG SourceLength;
    ULONG ExeLength;
    ULONG TargetLength;
} CSRSS_ADD_CONSOLE_ALIAS, *PCSRSS_ADD_CONSOLE_ALIAS;

typedef struct
{
    ULONG SourceLength;
    ULONG ExeLength;
    ULONG BytesWritten;
    ULONG TargetBufferLength;
    PVOID TargetBuffer;
} CSRSS_GET_CONSOLE_ALIAS, *PCSRSS_GET_CONSOLE_ALIAS;

typedef struct
{
    LPWSTR lpExeName;
    DWORD BytesWritten;
    DWORD AliasBufferLength;
    LPWSTR AliasBuffer;
} CSRSS_GET_ALL_CONSOLE_ALIASES, *PCSRSS_GET_ALL_CONSOLE_ALIAS;

typedef struct
{
    LPWSTR lpExeName;
    DWORD Length;
} CSRSS_GET_ALL_CONSOLE_ALIASES_LENGTH, *PCSRSS_GET_ALL_CONSOLE_ALIASES_LENGTH;

typedef struct
{
    DWORD BytesWritten;
    DWORD Length;
    LPWSTR ExeNames;
} CSRSS_GET_CONSOLE_ALIASES_EXES, *PCSRSS_GET_CONSOLE_ALIASES_EXES;

typedef struct
{
    DWORD Length;
} CSRSS_GET_CONSOLE_ALIASES_EXES_LENGTH, *PCSRSS_GET_CONSOLE_ALIASES_EXES_LENGTH;

typedef struct
{
    DWORD Event;
    DWORD ProcessGroup;
} CSRSS_GENERATE_CTRL_EVENT, *PCSRSS_GENERATE_CTRL_EVENT;

typedef struct
{
    HANDLE ConsoleHandle;
    DWORD NumInputEvents;
} CSRSS_GET_NUM_INPUT_EVENTS, *PCSRSS_GET_NUM_INPUT_EVENTS;

typedef struct
{
    HANDLE OutputHandle;
    COORD Size;
} CSRSS_SET_SCREEN_BUFFER_SIZE, *PCSRSS_SET_SCREEN_BUFFER_SIZE;

typedef struct
{
    CONSOLE_SELECTION_INFO Info;
} CSRSS_GET_CONSOLE_SELECTION_INFO, *PCSRSS_GET_CONSOLE_SELECTION_INFO;

typedef struct
{
    UNICODE_STRING ExeName;
    DWORD Length;
} CSRSS_GET_COMMAND_HISTORY_LENGTH, *PCSRSS_GET_COMMAND_HISTORY_LENGTH;

typedef struct
{
    UNICODE_STRING ExeName;
    PWCHAR History;
    DWORD Length;
} CSRSS_GET_COMMAND_HISTORY, *PCSRSS_GET_COMMAND_HISTORY;

typedef struct
{
  UNICODE_STRING ExeName;
} CSRSS_EXPUNGE_COMMAND_HISTORY, *PCSRSS_EXPUNGE_COMMAND_HISTORY;

typedef struct
{
    UNICODE_STRING ExeName;
    DWORD NumCommands;
} CSRSS_SET_HISTORY_NUMBER_COMMANDS, *PCSRSS_SET_HISTORY_NUMBER_COMMANDS;

typedef struct
{
    DWORD HistoryBufferSize;
    DWORD NumberOfHistoryBuffers;
    DWORD dwFlags;
} CSRSS_GET_HISTORY_INFO, *PCSRSS_GET_HISTORY_INFO,
  CSRSS_SET_HISTORY_INFO, *PCSRSS_SET_HISTORY_INFO;;

  
typedef struct
{
    UINT CodePage;
} CSRSS_GET_CONSOLE_CP, *PCSRSS_GET_CONSOLE_CP;

typedef struct
{
    UINT CodePage;
} CSRSS_SET_CONSOLE_CP, *PCSRSS_SET_CONSOLE_CP;

typedef struct
{
    UINT CodePage;
} CSRSS_GET_CONSOLE_OUTPUT_CP, *PCSRSS_GET_CONSOLE_OUTPUT_CP;

typedef struct
{
    UINT CodePage;
} CSRSS_SET_CONSOLE_OUTPUT_CP, *PCSRSS_SET_CONSOLE_OUTPUT_CP;












#if 0

                CSRSS_WRITE_CONSOLE WriteConsoleRequest;
                CSRSS_READ_CONSOLE ReadConsoleRequest;
                CSRSS_ALLOC_CONSOLE AllocConsoleRequest;
                CSRSS_FREE_CONSOLE FreeConsoleRequest;
                CSRSS_SCREEN_BUFFER_INFO ScreenBufferInfoRequest;
                CSRSS_SET_CURSOR SetCursorRequest;
                CSRSS_FILL_OUTPUT FillOutputRequest;
                CSRSS_FILL_OUTPUT_ATTRIB FillOutputAttribRequest;
                CSRSS_READ_INPUT ReadInputRequest;
                CSRSS_WRITE_CONSOLE_OUTPUT_CHAR WriteConsoleOutputCharRequest;
                CSRSS_WRITE_CONSOLE_OUTPUT_ATTRIB WriteConsoleOutputAttribRequest;
                CSRSS_GET_CURSOR_INFO GetCursorInfoRequest;
                CSRSS_SET_CURSOR_INFO SetCursorInfoRequest;
                CSRSS_SET_ATTRIB SetAttribRequest;
                CSRSS_SET_CONSOLE_MODE SetConsoleModeRequest;
                CSRSS_GET_CONSOLE_MODE GetConsoleModeRequest;
                CSRSS_CREATE_SCREEN_BUFFER CreateScreenBufferRequest;
                CSRSS_SET_SCREEN_BUFFER SetScreenBufferRequest;
                CSRSS_SET_TITLE SetTitleRequest;
                CSRSS_GET_TITLE GetTitleRequest;
                CSRSS_WRITE_CONSOLE_OUTPUT WriteConsoleOutputRequest;
                CSRSS_FLUSH_INPUT_BUFFER FlushInputBufferRequest;
                CSRSS_SCROLL_CONSOLE_SCREEN_BUFFER ScrollConsoleScreenBufferRequest;
                CSRSS_READ_CONSOLE_OUTPUT_CHAR ReadConsoleOutputCharRequest;
                CSRSS_READ_CONSOLE_OUTPUT_ATTRIB ReadConsoleOutputAttribRequest;
                CSRSS_PEEK_CONSOLE_INPUT PeekConsoleInputRequest;
                CSRSS_READ_CONSOLE_OUTPUT ReadConsoleOutputRequest;
                CSRSS_WRITE_CONSOLE_INPUT WriteConsoleInputRequest;
                CSRSS_GET_INPUT_HANDLE GetInputHandleRequest;
                CSRSS_GET_OUTPUT_HANDLE GetOutputHandleRequest;
                CSRSS_SETGET_CONSOLE_HW_STATE ConsoleHardwareStateRequest;
                CSRSS_GET_CONSOLE_WINDOW GetConsoleWindowRequest;
                CSRSS_SET_CONSOLE_ICON SetConsoleIconRequest;
                CSRSS_ADD_CONSOLE_ALIAS AddConsoleAlias;
                CSRSS_GET_CONSOLE_ALIAS GetConsoleAlias;
                CSRSS_GET_ALL_CONSOLE_ALIASES GetAllConsoleAlias;
                CSRSS_GET_ALL_CONSOLE_ALIASES_LENGTH GetAllConsoleAliasesLength;
                CSRSS_GET_CONSOLE_ALIASES_EXES GetConsoleAliasesExes;
                CSRSS_GET_CONSOLE_ALIASES_EXES_LENGTH GetConsoleAliasesExesLength;
                CSRSS_GENERATE_CTRL_EVENT GenerateCtrlEvent;
                CSRSS_GET_NUM_INPUT_EVENTS GetNumInputEventsRequest;
                CSRSS_SET_SCREEN_BUFFER_SIZE SetScreenBufferSize;
                CSRSS_GET_CONSOLE_SELECTION_INFO GetConsoleSelectionInfo;
                CSRSS_GET_COMMAND_HISTORY_LENGTH GetCommandHistoryLength;
                CSRSS_GET_COMMAND_HISTORY GetCommandHistory;
                CSRSS_EXPUNGE_COMMAND_HISTORY ExpungeCommandHistory;
                CSRSS_SET_HISTORY_NUMBER_COMMANDS SetHistoryNumberCommands;
                CSRSS_GET_HISTORY_INFO GetHistoryInfo;
                CSRSS_SET_HISTORY_INFO SetHistoryInfo;
                CSRSS_GET_CONSOLE_CP GetConsoleCodePage;
                CSRSS_SET_CONSOLE_CP SetConsoleCodePage;
                CSRSS_GET_CONSOLE_OUTPUT_CP GetConsoleOutputCodePage;
                CSRSS_SET_CONSOLE_OUTPUT_CP SetConsoleOutputCodePage;

#endif



#endif // __CSRCONS_H__

/* EOF */
