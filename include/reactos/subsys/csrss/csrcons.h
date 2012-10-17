/*
 * CSRSS Console management structures.
 */

#ifndef __CSRCONS_H__
#define __CSRCONS_H__

#include <drivers/blue/ntddblue.h>

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

#endif // __CSRCONS_H__

/* EOF */
