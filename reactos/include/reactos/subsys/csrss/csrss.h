#ifndef __INCLUDE_CSRSS_CSRSS_H
#define __INCLUDE_CSRSS_CSRSS_H

#include <drivers/blue/ntddblue.h>

#define CSR_NATIVE     0x0000
#define CSR_CONSOLE    0x0001
#define CSR_GUI        0x0002
#define CONSOLE_INPUT_MODE_VALID  (0x0f)
#define CONSOLE_OUTPUT_MODE_VALID (0x03)

/*
typedef union _CSR_API_NUMBER
{
    WORD Index;     // CSRSS API number
    WORD Subsystem; // 0=NTDLL;1=KERNEL32;2=KERNEL32
} CSR_API_NUMBER, *PCSR_API_NUMBER;
*/

typedef ULONG CSR_API_NUMBER;

#define MAKE_CSR_API(Number, Server) \
    ((Server) << 16) + Number

#define CSR_CSRSS_SECTION_SIZE          (65536)

typedef VOID (CALLBACK *PCONTROLDISPATCHER)(DWORD);

typedef struct
{
    ULONG Dummy;
} CSRSS_CONNECT_PROCESS, *PCSRSS_CONNECT_PROCESS;

typedef struct
{
   HANDLE NewProcessId;
   ULONG Flags;
   BOOL bInheritHandles;
} CSRSS_CREATE_PROCESS, *PCSRSS_CREATE_PROCESS;

typedef struct
{
    ULONG Dummy;
} CSRSS_TERMINATE_PROCESS, *PCSRSS_TERMINATE_PROCESS;

typedef struct
{
  ULONG nMaxIds;
   ULONG nProcessIdsCopied;
   ULONG nProcessIdsTotal;
   HANDLE ProcessId[0];
} CSRSS_GET_PROCESS_LIST, *PCSRSS_GET_PROCESS_LIST;

typedef struct
{
   HANDLE ConsoleHandle;
   BOOL Unicode;
   ULONG NrCharactersToWrite;
   ULONG NrCharactersWritten;
   BYTE Buffer[0];
} CSRSS_WRITE_CONSOLE, *PCSRSS_WRITE_CONSOLE;

typedef struct
{
   HANDLE ConsoleHandle;
   BOOL Unicode;
   WORD NrCharactersToRead;
   WORD nCharsCanBeDeleted;     /* number of chars already in buffer that can be backspaced */
   HANDLE EventHandle;
   ULONG NrCharactersRead;
   BYTE Buffer[0];
} CSRSS_READ_CONSOLE, *PCSRSS_READ_CONSOLE;

typedef struct
{
   PCONTROLDISPATCHER CtrlDispatcher;
   BOOL ConsoleNeeded;
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
	HANDLE	UniqueThread;
	CLIENT_ID	Cid;
} CSRSS_IDENTIFY_ALERTABLE_THREAD, *PCSRSS_IDENTIFY_ALERTABLE_THREAD;

typedef struct
{
  DWORD Length;
  WCHAR Title[0];
} CSRSS_SET_TITLE, *PCSRSS_SET_TITLE;

typedef struct
{
  DWORD Length;
  WCHAR Title[0];
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
}CSRSS_READ_CONSOLE_OUTPUT_CHAR, *PCSRSS_READ_CONSOLE_OUTPUT_CHAR;

typedef struct
{
  HANDLE ConsoleHandle;
  DWORD NumAttrsToRead;
  COORD ReadCoord;
  COORD EndCoord;
  WORD Attribute[0];
}CSRSS_READ_CONSOLE_OUTPUT_ATTRIB, *PCSRSS_READ_CONSOLE_OUTPUT_ATTRIB;

typedef struct
{
  HANDLE ConsoleHandle;
  DWORD NumInputEvents;
}CSRSS_GET_NUM_INPUT_EVENTS, *PCSRSS_GET_NUM_INPUT_EVENTS;

typedef struct
{
  HANDLE ProcessId;
} CSRSS_REGISTER_SERVICES_PROCESS, *PCSRSS_REGISTER_SERVICES_PROCESS;

typedef struct
{
  UINT Flags;
  DWORD Reserved;
} CSRSS_EXIT_REACTOS, *PCSRSS_EXIT_REACTOS;

typedef struct
{
  DWORD Level;
  DWORD Flags;
} CSRSS_SET_SHUTDOWN_PARAMETERS, *PCSRSS_SET_SHUTDOWN_PARAMETERS;

typedef struct
{
  DWORD Level;
  DWORD Flags;
} CSRSS_GET_SHUTDOWN_PARAMETERS, *PCSRSS_GET_SHUTDOWN_PARAMETERS;

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
  HANDLE InputHandle;
} CSRSS_GET_INPUT_HANDLE, *PCSRSS_GET_INPUT_HANDLE;

typedef struct
{
  DWORD Access;
  BOOL Inheritable;
  HANDLE OutputHandle;
} CSRSS_GET_OUTPUT_HANDLE, *PCSRSS_GET_OUTPUT_HANDLE;

typedef struct
{
  HANDLE Handle;
} CSRSS_CLOSE_HANDLE, *PCSRSS_CLOSE_HANDLE;

typedef struct
{
  HANDLE Handle;
} CSRSS_VERIFY_HANDLE, *PCSRSS_VERIFY_HANDLE;

typedef struct
{
  HANDLE Handle;
  DWORD Access;
  BOOL Inheritable;
  DWORD Options;
} CSRSS_DUPLICATE_HANDLE, *PCSRSS_DUPLICATE_HANDLE;

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
  HDESK DesktopHandle;
} CSRSS_CREATE_DESKTOP, *PCSRSS_CREATE_DESKTOP;

typedef struct
{
  HWND DesktopWindow;
  ULONG Width;
  ULONG Height;
} CSRSS_SHOW_DESKTOP, *PCSRSS_SHOW_DESKTOP;

typedef struct
{
  HWND DesktopWindow;
} CSRSS_HIDE_DESKTOP, *PCSRSS_HIDE_DESKTOP;

typedef struct
{
  HWND LogonNotifyWindow;
} CSRSS_SET_LOGON_NOTIFY_WINDOW, *PCSRSS_SET_LOGON_NOTIFY_WINDOW;

typedef struct
{
  HANDLE ProcessId;
  BOOL Register;
} CSRSS_REGISTER_LOGON_PROCESS, *PCSRSS_REGISTER_LOGON_PROCESS;

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

typedef struct
{
  HANDLE InputWaitHandle;
} CSRSS_GET_INPUT_WAIT_HANDLE, *PCSRSS_GET_INPUT_WAIT_HANDLE;

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
}  CSRSS_GET_CONSOLE_ALIASES_EXES, *PCSRSS_GET_CONSOLE_ALIASES_EXES;

typedef struct
{
  DWORD Length;
} CSRSS_GET_CONSOLE_ALIASES_EXES_LENGTH, *PCSRSS_GET_CONSOLE_ALIASES_EXES_LENGTH;

typedef struct
{
  DWORD Event;
  DWORD ProcessGroup;
} CSRSS_GENERATE_CTRL_EVENT, *PCSRSS_GENERATE_CTRL_EVENT;



#define CSR_API_MESSAGE_HEADER_SIZE(Type)       (FIELD_OFFSET(CSR_API_MESSAGE, Data) + sizeof(Type))
#define CSRSS_MAX_WRITE_CONSOLE                 (LPC_MAX_DATA_LENGTH - CSR_API_MESSAGE_HEADER_SIZE(CSRSS_WRITE_CONSOLE))
#define CSRSS_MAX_WRITE_CONSOLE_OUTPUT_CHAR     (LPC_MAX_DATA_LENGTH - CSR_API_MESSAGE_HEADER_SIZE(CSRSS_WRITE_CONSOLE_OUTPUT_CHAR))
#define CSRSS_MAX_WRITE_CONSOLE_OUTPUT_ATTRIB   (LPC_MAX_DATA_LENGTH - CSR_API_MESSAGE_HEADER_SIZE(CSRSS_WRITE_CONSOLE_OUTPUT_ATTRIB))
#define CSRSS_MAX_READ_CONSOLE                  (LPC_MAX_DATA_LENGTH - CSR_API_MESSAGE_HEADER_SIZE(CSRSS_READ_CONSOLE))
#define CSRSS_MAX_READ_CONSOLE_OUTPUT_CHAR      (LPC_MAX_DATA_LENGTH - CSR_API_MESSAGE_HEADER_SIZE(CSRSS_READ_CONSOLE_OUTPUT_CHAR))
#define CSRSS_MAX_READ_CONSOLE_OUTPUT_ATTRIB    (LPC_MAX_DATA_LENGTH - CSR_API_MESSAGE_HEADER_SIZE(CSRSS_READ_CONSOLE_OUTPUT_ATTRIB))
#define CSRSS_MAX_GET_PROCESS_LIST              (LPC_MAX_DATA_LENGTH - CSR_API_MESSAGE_HEADER_SIZE(CSRSS_GET_PROCESS_LIST))

/* WCHARs, not bytes! */
#define CSRSS_MAX_TITLE_LENGTH          80
#define CSRSS_MAX_ALIAS_TARGET_LENGTH   80

#define CREATE_PROCESS                (0x0)
#define TERMINATE_PROCESS             (0x1)
#define WRITE_CONSOLE                 (0x2)
#define READ_CONSOLE                  (0x3)
#define ALLOC_CONSOLE                 (0x4)
#define FREE_CONSOLE                  (0x5)
#define CONNECT_PROCESS               (0x6)
#define SCREEN_BUFFER_INFO            (0x7)
#define SET_CURSOR                    (0x8)
#define FILL_OUTPUT                   (0x9)
#define READ_INPUT                    (0xA)
#define WRITE_CONSOLE_OUTPUT_CHAR     (0xB)
#define WRITE_CONSOLE_OUTPUT_ATTRIB   (0xC)
#define FILL_OUTPUT_ATTRIB            (0xD)
#define GET_CURSOR_INFO               (0xE)
#define SET_CURSOR_INFO               (0xF)
#define SET_ATTRIB                    (0x10)
#define GET_CONSOLE_MODE              (0x11)
#define SET_CONSOLE_MODE              (0x12)
#define CREATE_SCREEN_BUFFER          (0x13)
#define SET_SCREEN_BUFFER             (0x14)
#define SET_TITLE                     (0x15)
#define GET_TITLE                     (0x16)
#define WRITE_CONSOLE_OUTPUT          (0x17)
#define FLUSH_INPUT_BUFFER            (0x18)
#define SCROLL_CONSOLE_SCREEN_BUFFER  (0x19)
#define READ_CONSOLE_OUTPUT_CHAR      (0x1A)
#define READ_CONSOLE_OUTPUT_ATTRIB    (0x1B)
#define GET_NUM_INPUT_EVENTS          (0x1C)
#define REGISTER_SERVICES_PROCESS     (0x1D)
#define EXIT_REACTOS                  (0x1E)
#define GET_SHUTDOWN_PARAMETERS       (0x1F)
#define SET_SHUTDOWN_PARAMETERS       (0x20)
#define PEEK_CONSOLE_INPUT            (0x21)
#define READ_CONSOLE_OUTPUT           (0x22)
#define WRITE_CONSOLE_INPUT           (0x23)
#define GET_INPUT_HANDLE              (0x24)
#define GET_OUTPUT_HANDLE             (0x25)
#define CLOSE_HANDLE                  (0x26)
#define VERIFY_HANDLE                 (0x27)
#define DUPLICATE_HANDLE	      (0x28)
#define SETGET_CONSOLE_HW_STATE       (0x29)
#define GET_CONSOLE_WINDOW            (0x2A)
#define CREATE_DESKTOP                (0x2B)
#define SHOW_DESKTOP                  (0x2C)
#define HIDE_DESKTOP                  (0x2D)
#define SET_CONSOLE_ICON              (0x2E)
#define SET_LOGON_NOTIFY_WINDOW       (0x2F)
#define REGISTER_LOGON_PROCESS        (0x30)
#define GET_CONSOLE_CP                (0x31)
#define SET_CONSOLE_CP                (0x32)
#define GET_CONSOLE_OUTPUT_CP         (0x33)
#define SET_CONSOLE_OUTPUT_CP         (0x34)
#define GET_INPUT_WAIT_HANDLE	      (0x35)
#define GET_PROCESS_LIST              (0x36)
#define START_SCREEN_SAVER            (0x37)
#define ADD_CONSOLE_ALIAS             (0x38)
#define GET_CONSOLE_ALIAS             (0x39)
#define GET_ALL_CONSOLE_ALIASES         (0x3A)
#define GET_ALL_CONSOLE_ALIASES_LENGTH (0x3B)
#define GET_CONSOLE_ALIASES_EXES      (0x3C)
#define GET_CONSOLE_ALIASES_EXES_LENGTH (0x3D)
#define GENERATE_CTRL_EVENT           (0x3E)

/* Keep in sync with definition below. */
#define CSRSS_HEADER_SIZE (sizeof(PORT_MESSAGE) + sizeof(ULONG) + sizeof(NTSTATUS))

typedef struct _CSR_API_MESSAGE
{
    PORT_MESSAGE Header;
    PVOID CsrCaptureData;
    ULONG Type;
    NTSTATUS Status;
    union
    {
        CSRSS_CREATE_PROCESS CreateProcessRequest;
        CSRSS_CONNECT_PROCESS ConnectRequest;
        CSRSS_WRITE_CONSOLE WriteConsoleRequest;
        CSRSS_READ_CONSOLE ReadConsoleRequest;
        CSRSS_ALLOC_CONSOLE AllocConsoleRequest;
        CSRSS_SCREEN_BUFFER_INFO ScreenBufferInfoRequest;
        CSRSS_SET_CURSOR SetCursorRequest;
        CSRSS_FILL_OUTPUT FillOutputRequest;
        CSRSS_READ_INPUT ReadInputRequest;
        CSRSS_WRITE_CONSOLE_OUTPUT_CHAR WriteConsoleOutputCharRequest;
        CSRSS_WRITE_CONSOLE_OUTPUT_ATTRIB WriteConsoleOutputAttribRequest;
        CSRSS_FILL_OUTPUT_ATTRIB FillOutputAttribRequest;
        CSRSS_SET_CURSOR_INFO SetCursorInfoRequest;
        CSRSS_GET_CURSOR_INFO GetCursorInfoRequest;
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
        CSRSS_GET_NUM_INPUT_EVENTS GetNumInputEventsRequest;
        CSRSS_REGISTER_SERVICES_PROCESS RegisterServicesProcessRequest;
        CSRSS_EXIT_REACTOS ExitReactosRequest;
        CSRSS_SET_SHUTDOWN_PARAMETERS SetShutdownParametersRequest;
        CSRSS_GET_SHUTDOWN_PARAMETERS GetShutdownParametersRequest;
        CSRSS_PEEK_CONSOLE_INPUT PeekConsoleInputRequest;
        CSRSS_READ_CONSOLE_OUTPUT ReadConsoleOutputRequest;
        CSRSS_WRITE_CONSOLE_INPUT WriteConsoleInputRequest;
        CSRSS_GET_INPUT_HANDLE GetInputHandleRequest;
        CSRSS_GET_OUTPUT_HANDLE GetOutputHandleRequest;
        CSRSS_CLOSE_HANDLE CloseHandleRequest;
        CSRSS_VERIFY_HANDLE VerifyHandleRequest;
        CSRSS_DUPLICATE_HANDLE DuplicateHandleRequest;
        CSRSS_SETGET_CONSOLE_HW_STATE ConsoleHardwareStateRequest;
        CSRSS_GET_CONSOLE_WINDOW GetConsoleWindowRequest;
        CSRSS_CREATE_DESKTOP CreateDesktopRequest;
        CSRSS_SHOW_DESKTOP ShowDesktopRequest;
        CSRSS_HIDE_DESKTOP HideDesktopRequest;
        CSRSS_SET_CONSOLE_ICON SetConsoleIconRequest;
        CSRSS_SET_LOGON_NOTIFY_WINDOW SetLogonNotifyWindowRequest;
        CSRSS_REGISTER_LOGON_PROCESS RegisterLogonProcessRequest;
        CSRSS_GET_CONSOLE_CP GetConsoleCodePage;
        CSRSS_SET_CONSOLE_CP SetConsoleCodePage;
        CSRSS_GET_CONSOLE_OUTPUT_CP GetConsoleOutputCodePage;
        CSRSS_SET_CONSOLE_OUTPUT_CP SetConsoleOutputCodePage;
        CSRSS_GET_INPUT_WAIT_HANDLE GetConsoleInputWaitHandle;
        CSRSS_GET_PROCESS_LIST GetProcessListRequest;
        CSRSS_ADD_CONSOLE_ALIAS AddConsoleAlias;
        CSRSS_GET_CONSOLE_ALIAS GetConsoleAlias;
        CSRSS_GET_ALL_CONSOLE_ALIASES GetAllConsoleAlias;
        CSRSS_GET_ALL_CONSOLE_ALIASES_LENGTH GetAllConsoleAliasesLength;
        CSRSS_GET_CONSOLE_ALIASES_EXES GetConsoleAliasesExes;
        CSRSS_GET_CONSOLE_ALIASES_EXES_LENGTH GetConsoleAliasesExesLength;
        CSRSS_GENERATE_CTRL_EVENT GenerateCtrlEvent;
    } Data;
} CSR_API_MESSAGE, *PCSR_API_MESSAGE;

/* Types used in the new CSR. Temporarly here for proper compile of NTDLL */
#define CSR_SRV_SERVER 0

#define CsrSrvClientConnect             0
#define CsrSrvIdentifyAlertableThread   3
#define CsrSrvSetPriorityClass          4

#define CSR_MAKE_OPCODE(s,m) ((s) << 16) | (m)

typedef struct _CSR_CONNECTION_INFO
{
    ULONG Version;
    ULONG Unknown;
    HANDLE ObjectDirectory;
    PVOID SharedSectionBase;
    PVOID SharedSectionHeap;
    PVOID SharedSectionData;
    ULONG DebugFlags;
    ULONG Unknown2[3];
    HANDLE ProcessId;
} CSR_CONNECTION_INFO, *PCSR_CONNECTION_INFO;

typedef struct _CSR_CLIENT_CONNECT
{
    ULONG ServerId;
    PVOID ConnectionInfo;
    ULONG ConnectionInfoSize;
} CSR_CLIENT_CONNECT, *PCSR_CLIENT_CONNECT;

typedef struct _CSR_IDENTIFY_ALTERTABLE_THREAD
{
    CLIENT_ID Cid;
} CSR_IDENTIFY_ALTERTABLE_THREAD, *PCSR_IDENTIFY_ALTERTABLE_THREAD;

typedef struct _CSR_SET_PRIORITY_CLASS
{
    HANDLE hProcess;
    ULONG PriorityClass;
} CSR_SET_PRIORITY_CLASS, *PCSR_SET_PRIORITY_CLASS;

typedef struct _CSR_API_MESSAGE2
{
    PORT_MESSAGE Header;
    union
    {
        CSR_CONNECTION_INFO ConnectionInfo;
        struct
        {
            PVOID CsrCaptureData;
            CSR_API_NUMBER Opcode;
            ULONG Status;
            ULONG Reserved;
            union
            {
                CSR_CLIENT_CONNECT ClientConnect;
                CSR_SET_PRIORITY_CLASS SetPriorityClass;
                CSR_IDENTIFY_ALTERTABLE_THREAD IdentifyAlertableThread;
            };
        };
    };
} CSR_API_MESSAGE2, *PCSR_API_MESSAGE2;

typedef struct _CSR_CAPTURE_BUFFER
{
    ULONG Size;
    struct _CSR_CAPTURE_BUFFER *PreviousCaptureBuffer;
    ULONG PointerCount;
    ULONG_PTR BufferEnd;
    ULONG_PTR PointerArray[1];
} CSR_CAPTURE_BUFFER, *PCSR_CAPTURE_BUFFER;

#endif /* __INCLUDE_CSRSS_CSRSS_H */
