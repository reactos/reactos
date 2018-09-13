/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    wininetd.h

Abstract:

    Contains the interface for the WININET diagnostic capabilities.

    This interface only exists in the debug version of WININET.DLL.
    Calling the debug functions in the retail version of the DLL will
    yield ERROR_INVALID_PARAMETER

Author:

    Richard L Firth (rfirth) 14-Jun-1995

Revision History:

    14-Jun-1995 rfirth
        Created

--*/

//
// manifests
//

//
// if the environment or registry variable "WininetLog" is set to !0 then the
// following values will be used for control, category and error level resp.
// These options generate "WININET.LOG" in the current directory
//

#define INTERNET_DEBUG_CONTROL_DEFAULT      (DBG_THREAD_INFO       \
                                            | DBG_CALL_DEPTH        \
                                            | DBG_ENTRY_TIME        \
                                            | DBG_PARAMETER_LIST    \
                                            | DBG_TO_FILE           \
                                            | DBG_INDENT_DUMP       \
                                            | DBG_SEPARATE_APIS     \
                                            | DBG_AT_ERROR_LEVEL    \
                                            | DBG_NO_ASSERT_BREAK   \
                                            | DBG_DUMP_LENGTH       \
                                            | DBG_NO_LINE_NUMBER    \
                                            | DBG_ASYNC_ID          \
                                            )
#define INTERNET_DEBUG_CATEGORY_DEFAULT     DBG_ANY
#define INTERNET_DEBUG_ERROR_LEVEL_DEFAULT  DBG_INFO

//
// options. These are the option values to use with InternetQueryOption()/
// InternetSetOption() to get/set the information described herein
//

#define INTERNET_OPTION_GET_DEBUG_INFO      1001
#define INTERNET_OPTION_SET_DEBUG_INFO      1002
#define INTERNET_OPTION_GET_HANDLE_COUNT    1003
#define INTERNET_OPTION_GET_TRIGGERS        1004
#define INTERNET_OPTION_SET_TRIGGERS        1005
#define INTERNET_OPTION_RESET_TRIGGERS      1006

#define INTERNET_FIRST_DEBUG_OPTION         INTERNET_OPTION_GET_DEBUG_INFO
#define INTERNET_LAST_DEBUG_OPTION          INTERNET_OPTION_RESET_TRIGGERS

//
// debug levels
//

#define DBG_INFO            0
#define DBG_WARNING         1
#define DBG_ERROR           2
#define DBG_FATAL           3
#define DBG_ALWAYS          99

//
// debug control flags - these flags control where the debug output goes (file,
// debugger, console) and how it is formatted
//

#define DBG_THREAD_INFO     0x00000001  // dump the thread id
#define DBG_CALL_DEPTH      0x00000002  // dump the call level
#define DBG_ENTRY_TIME      0x00000004  // dump the local time when the function is called
#define DBG_PARAMETER_LIST  0x00000008  // dump the parameter list
#define DBG_TO_DEBUGGER     0x00000010  // output via OutputDebugString()
#define DBG_TO_CONSOLE      0x00000020  // output via printf()
#define DBG_TO_FILE         0x00000040  // output via fprintf()
#define DBG_FLUSH_OUTPUT    0x00000080  // fflush() after every fprintf()
#define DBG_INDENT_DUMP     0x00000100  // indent dumped data to current level
#define DBG_SEPARATE_APIS   0x00000200  // empty line after leaving each API
#define DBG_AT_ERROR_LEVEL  0x00000400  // always output diagnostics >= InternetDebugErrorLevel
#define DBG_NO_ASSERT_BREAK 0x00000800  // don't call DebugBreak() in InternetAssert()
#define DBG_DUMP_LENGTH     0x00001000  // dump length information when dumping data
#define DBG_NO_LINE_NUMBER  0x00002000  // don't dump line number info
#define DBG_APPEND_FILE     0x00004000  // append to the log file (default is truncate)
#define DBG_LEVEL_INDICATOR 0x00008000  // dump error level indicator (E for Error, etc.)
#define DBG_DUMP_API_DATA   0x00010000  // dump data at API level (InternetReadFile(), etc.)
#define DBG_DELTA_TIME      0x00020000  // dump times as millisecond delta if DBG_ENTRY_TIME
#define DBG_CUMULATIVE_TIME 0x00040000  // dump delta time from start of trace if DBG_ENTRY_TIME
#define DBG_FIBER_INFO      0x00080000  // dump the fiber address if DBG_THREAD_INFO
#define DBG_THREAD_INFO_ADR 0x00100000  // dump INTERNET_THREAD_INFO address if DBG_THREAD_INFO
#define DBG_ARB_ADDR        0x00200000  // dump ARB address if DBG_THREAD_INFO
#define DBG_ASYNC_ID        0x00400000  // dump async ID
#define DBG_REQUEST_HANDLE  0x00800000  // dump request handle
#define DBG_TRIGGER_ON      0x10000000  // function is an enabling trigger
#define DBG_TRIGGER_OFF     0x20000000  // function is a disabling trigger
#define DBG_NO_DATA_DUMP    0x40000000  // turn off all data dumping
#define DBG_NO_DEBUG        0x80000000  // turn off all debugging

//
// debug category flags - these control what category of information is output
//

#define DBG_NOTHING         0x00000000  // internal
#define DBG_INET            0x00000001  // e.g. InternetOpenUrl()
#define DBG_FTP             0x00000002  // e.g. FtpFindFirstFile()
#define DBG_GOPHER          0x00000004  // e.g. GopherFindFirstFile()
#define DBG_HTTP            0x00000008  // e.g. HttpOpenRequest()
#define DBG_API             0x00000010  // APIs
#define DBG_UTIL            0x00000020  // various utility functions
#define DBG_UNICODE         0x00000040  // wide character functions
#define DBG_WORKER          0x00000080  // worker functions
#define DBG_HANDLE          0x00000100  // handle creation/deletion functions
#define DBG_SESSION         0x00000200  // session/creation functions
#define DBG_SOCKETS         0x00000400  // sockets functions
#define DBG_VIEW            0x00000800  // gopher view functions
#define DBG_BUFFER          0x00001000  // gopher buffer functions
#define DBG_PARSE           0x00002000  // FTP/gopher parse functions
#define DBG_MEMALLOC        0x00004000  // Debug memory allocation/free functions
#define DBG_SERIALST        0x00008000  // Serialized List functions
#define DBG_THRDINFO        0x00010000  // INTERNET_THREAD_INFO functions
#define DBG_PROTOCOL        0x00020000  // protocol functions
#define DBG_DLL             0x00040000  // DLL functions
#define DBG_REFCOUNT        0x00080000  // logs all reference count functions
#define DBG_REGISTRY        0x00100000  // logs all registry functions
#define DBG_TRACE_SOCKETS   0x00200000  // monitors socket usage
#define DBG_ASYNC           0x00400000  // logs async functions
#define DBG_CACHE           0x00800000  // logs cache specific stuff
#define DBG_INVALID_HANDLES 0x01000000  // logs invalid handles (e.g. in InternetCloseHandle())
#define DBG_OBJECTS         0x02000000  // dump object info
#define DBG_PROXY           0x04000000  // dump proxy info
#define DBG_RESLOCK         0x08000000  // dump resource lock info
#define DBG_DIALUP          0x10000000  // dump dial-up info
#define DBG_GLOBAL          0x20000000  // dump global-scope functions
#define DBG_ANY             0xFFFFFFFF  // internal

//
// types
//

//
// INTERNET_DEBUG_INFO - structure that receives the current debugging variables
// via InternetQueryOption(), or which contains the new debugging variables to
// be set via InternetSetOption()
//

typedef struct {

    //
    // ErrorLevel - DBG_INFO, etc.
    //

    int ErrorLevel;

    //
    // ControlFlags - DBG_THREAD_INFO, etc.
    //

    DWORD ControlFlags;

    //
    // CategoryFlags - DBG_INET, etc.
    //

    DWORD CategoryFlags;

    //
    // BreakFlags - DBG_API, etc. where breakpoints will be taken
    //

    DWORD BreakFlags;

    //
    // IndentIncrement - increment to use for each depth increase
    //

    int IndentIncrement;

    //
    // Filename - name of output log being used/to use
    //

    char Filename[1];

} INTERNET_DEBUG_INFO, *LPINTERNET_DEBUG_INFO;

//
// INTERNET_TRIGGER_INFO - a diagnostic trigger. Triggers are enabled when the
// function named in this structure is executed. Triggers can enable or disable
// diagnostics
//

typedef struct {

    //
    // FunctionName - name of the function to act as trigger. ControlFlags has
    // DBG_TRIGGER_ON or DBG_TRIGGER_OFF set
    //

    LPCSTR FunctionName;

    //
    // ControlFlags - control flags to use when the trigger is enabled. If the
    // trigger disables diagnostics then the previous control flags will be
    // restored
    //

    DWORD ControlFlags;

    //
    // CategoryFlags - category flags to use when the trigger is enabled. See
    // ControlFlags
    //

    DWORD CategoryFlags;

} INTERNET_TRIGGER_INFO, *LPINTERNET_TRIGGER_INFO;

//
// INTERNET_DEBUG_TRIGGERS - 1 or more of these will be returned from/given to
// InternetQueryOption()/InternetSetOption()
//

typedef struct {

    //
    // Count - number of INTERNET_TRIGGER_INFO structures contained herein
    //

    DWORD Count;

    //
    // Triggers - an array of 0 or more INTERNET_TRIGGER_INFO structures
    //

    INTERNET_TRIGGER_INFO Triggers[1];

} INTERNET_DEBUG_TRIGGERS, *LPINTERNET_DEBUG_TRIGGERS;
