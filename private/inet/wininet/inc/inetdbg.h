/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    inetdbg.h

Abstract:

    Manifests, macros, types and prototypes for Windows Internet client DLL
    debugging functions

Author:

    Richard L Firth (rfirth) 11-Oct-1994

Revision History:

    11-Oct-1994 rfirth
        Created

--*/

#if defined(__cplusplus)
extern "C" {
#endif

//
// misc. debug manifests
//

#define DEBUG_WAIT_TIME     (2 * 60 * 1000)

//
// Checked builds get INET_DEBUG set by default; retail builds get no debugging
// by default
//

#if DBG

#if !defined(INET_DEBUG)

#define INET_DEBUG          1

#endif // INET_DEBUG

#else

#if !defined(INET_DEBUG)

#define INET_DEBUG          0

#endif // INET_DEBUG

#endif // DBG

//
// types
//

//
// DEBUG_FUNCTION_RETURN_TYPE - Type of result (scalar) that a function returns
//

#ifdef ENABLE_DEBUG

typedef enum {
    None,
    Bool,
    Int,
    Dword,
    String,
    Handle,
    Pointer
} DEBUG_FUNCTION_RETURN_TYPE;

//
// INTERNET_DEBUG_RECORD - for each thread, we maintain a LIFO stack of these,
// describing the functions we have visited
//

typedef struct _INTERNET_DEBUG_RECORD {

    //
    // Stack - a LIFO stack of debug records is maintained in the debug version
    // of the INTERNET_THREAD_INFO
    //

    struct _INTERNET_DEBUG_RECORD* Stack;

    //
    // Category - the function's category flag(s)
    //

    DWORD Category;

    //
    // ReturnType - type of value returned by function
    //

    DEBUG_FUNCTION_RETURN_TYPE ReturnType;

    //
    // Function - name of the function
    //

    LPCSTR Function;

    //
    // LastTime - if we are dumping times as deltas, keeps the last tick count
    //

    DWORD LastTime;

} INTERNET_DEBUG_RECORD, *LPINTERNET_DEBUG_RECORD;

//
// INTERNET_FUNCTION_TRIGGER - if we are required to trigger on a function, this
// structure maintains the debugging flags
//

typedef struct _INTERNET_FUNCTION_TRIGGER {

    //
    // Next - we maintain a singly-linked list of INTERNET_FUNCTION_TRIGGERs
    //

    struct _INTERNET_FUNCTION_TRIGGER* Next;

    //
    // Hash - hash value for the function name, to cut-down strcmp's to 1
    //

    DWORD Hash;

    //
    // Function - name of the function - must match exactly
    //

    LPCSTR Function;

    //
    // Category - category debug flags to use when this function triggers
    //

    DWORD MajorCategory;

} INTERNET_FUNCTION_TRIGGER, *LPINTERNET_FUNCTION_TRIGGER;

//
// data
//

extern DWORD InternetDebugErrorLevel;
extern DWORD InternetDebugControlFlags;
extern DWORD InternetDebugCategoryFlags;
extern DWORD InternetDebugBreakFlags;

//
// prototypes
//

//
// inetdbg.cxx
//

VOID
InternetDebugInitialize(
    VOID
    );

VOID
InternetDebugTerminate(
    VOID
    );

DWORD
InternetGetDebugInfo(
    OUT LPINTERNET_DEBUG_INFO lpBuffer,
    IN OUT LPDWORD lpdwBufferLength
    );

DWORD
InternetSetDebugInfo(
    IN LPINTERNET_DEBUG_INFO lpBuffer,
    IN DWORD dwBufferLength
    );

BOOL
InternetOpenDebugFile(
    VOID
    );

BOOL
InternetReopenDebugFile(
    IN LPSTR Filename
    );

VOID
InternetCloseDebugFile(
    VOID
    );

VOID
InternetFlushDebugFile(
    VOID
    );

VOID
InternetDebugSetControlFlags(
    IN DWORD dwFlags
    );

VOID
InternetDebugResetControlFlags(
    IN DWORD dwFlags
    );

VOID
InternetDebugEnter(
    IN DWORD Category,
    IN DEBUG_FUNCTION_RETURN_TYPE ReturnType,
    IN LPCSTR Function,
    IN LPCSTR ParameterList,
    IN ...
    );

VOID
InternetDebugLeave(
    IN DWORD_PTR Variable,
    IN LPCSTR Filename,
    IN DWORD LineNumber
    );

VOID
InternetDebugError(
    IN DWORD Error
    );

VOID
InternetDebugPrint(
    IN LPSTR Format,
    ...
    );

VOID
InternetDebugPrintValist(
    IN LPSTR Format,
    IN va_list valist
    );

VOID
InternetDebugPrintf(
    IN LPSTR Format,
    IN ...
    );

VOID
InternetDebugOut(
    IN LPSTR Buffer,
    IN BOOL Assert
    );

VOID
InternetDebugDump(
    IN LPSTR Text,
    IN LPBYTE Address,
    IN DWORD Size
    );

DWORD
InternetDebugDumpFormat(
    IN LPBYTE Address,
    IN DWORD Size,
    IN DWORD ElementSize,
    OUT LPSTR Buffer
    );

VOID
InternetAssert(
    IN LPSTR Condition,
    IN LPSTR Filename,
    IN DWORD LineNumber
    );

VOID
InternetGetDebugVariable(
    IN LPSTR lpszVariableName,
    OUT LPDWORD lpdwVariable
    );

LPSTR
InternetMapError(
    IN DWORD Error
    );

LPSTR
InternetMapStatus(
    IN DWORD Status
    );

LPSTR
InternetMapOption(
    IN DWORD Option
    );

LPSTR
InternetMapSSPIError(
    IN DWORD Status
    );

LPSTR
InternetMapHttpOption(
    IN DWORD Option
    );

LPSTR
InternetMapHttpState(
    IN DWORD State
    );

LPSTR
InternetMapHttpStateFlag(
    IN DWORD Flag
    );

LPSTR
InternetMapAsyncRequest(
    IN AR_TYPE Type
    );

LPSTR
InternetMapHandleType(
    IN DWORD HandleType
    );

LPSTR
InternetMapScheme(
    IN INTERNET_SCHEME Scheme
    );

LPSTR
InternetMapOpenType(
    IN DWORD OpenType
    );

LPSTR
InternetMapService(
    IN DWORD Service
    );         

LPSTR
InternetMapWinsockCallbackType(
    IN DWORD CallbackType
    );

LPSTR
InternetMapChunkToken(
    IN CHUNK_TOKEN ctToken
    );

LPSTR
InternetMapChunkState(
    IN CHUNK_STATE csState
    );

DWORD
InternetHandleCount(
    VOID
    );

int dprintf(char *, ...);

LPSTR
SourceFilename(
    LPSTR Filespec
    );

VOID
InitSymLib(
    VOID
    );

VOID
TermSymLib(
    VOID
    );

LPSTR
GetDebugSymbol(
    DWORD Address,
    LPDWORD Offset
    );

VOID
x86SleazeCallStack(
    OUT LPVOID * lplpvStack,
    IN DWORD dwStackCount,
    IN LPVOID * Ebp
    );

VOID
x86SleazeCallersAddress(
    LPVOID* pCaller,
    LPVOID* pCallersCaller
    );

//
// exceptn.cxx
//

VOID
SetExceptionHandler(
    VOID
    );

#else

#define dprintf (VOID)

#endif // #ifdef ENABLE_DEBUG

//
// macros
//

#ifdef ENABLE_DEBUG

//
// INET_DEBUG_START - initialize debugging support
//

#define INET_DEBUG_START() \
    InternetDebugInitialize()

//
// INET_DEBUG_FINISH - terminate debugging support
//

#define INET_DEBUG_FINISH() \
    InternetDebugTerminate()

//
// INET_ASSERT - The standard assert, redefined here because Win95 doesn't have
// RtlAssert
//

#if defined(DISABLE_ASSERTS)

#define INET_ASSERT(test) \
    /* NOTHING */

#else

#define INET_ASSERT(test) \
    do if (!(test)) { \
        InternetAssert(#test, __FILE__, __LINE__); \
    } while (0)

#endif // defined(RETAIL_LOGGING)

#else // end #ifdef ENABLE_DEBUG

#define INET_DEBUG_START() \
    /* NOTHING */

#define INET_DEBUG_FINISH() \
    /* NOTHING */

#define INET_ASSERT(test) \
    do { } while(0) /* NOTHING */

#endif // end #ifndef ENABLE_DEBUG

//
// INET_DEBUG_ASSERT - assert only if INET_DEBUG is set
//

#if INET_DEBUG
#define INET_DEBUG_ASSERT(cond) INET_ASSERT(cond)
#else
#define INET_DEBUG_ASSERT(cond) /* NOTHING */
#endif

#if INET_DEBUG

//
// IF_DEBUG_CODE - always on if INET_DEBUG is set
//

#define IF_DEBUG_CODE() \
    if (1)

//
// IF_DEBUG - only execute following code if the specific flag is set
//

#define IF_DEBUG(x) \
    if (InternetDebugCategoryFlags & DBG_ ## x)

//
// IF_DEBUG_CONTROL - only execute if control flag is set
//

#define IF_DEBUG_CONTROL(x) \
    if (InternetDebugControlFlags & DBG_ ## x)

//
// DEBUG_ENTER - creates an INTERNET_DEBUG_RECORD for this function
//

#if defined(RETAIL_LOGGING)

#define DEBUG_ENTER(ParameterList) \
    /* NOTHING */

#define DEBUG_ENTER_API(ParameterList) \
    InternetDebugEnter ParameterList

#else

#define DEBUG_ENTER_API DEBUG_ENTER
#define DEBUG_ENTER(ParameterList) \
    InternetDebugEnter ParameterList

#endif // defined(RETAIL_LOGGING)

//
// DEBUG_LEAVE - destroys this function's INTERNET_DEBUG_RECORD
//

#if defined(RETAIL_LOGGING)

#define DEBUG_LEAVE(Variable) \
    /* NOTHING */

#define DEBUG_LEAVE_API(Variable) \
    InternetDebugLeave((DWORD_PTR)Variable, __FILE__, __LINE__)

#else

#define DEBUG_LEAVE_API DEBUG_LEAVE
#define DEBUG_LEAVE(Variable) \
    InternetDebugLeave((DWORD_PTR)Variable, __FILE__, __LINE__)

#endif // defined(RETAIL_LOGGING)

//
// DEBUG_ERROR - displays an error and its symbolic name
//

#define DEBUG_ERROR(Category, Error) \
    if (InternetDebugCategoryFlags & DBG_ ## Category) { \
        InternetDebugError(Error); \
    }

//
// DEBUG_PRINT - print debug info if we are at the correct level or we are
// requested to always dump information at, or above, InternetDebugErrorLevel
//

#if defined(RETAIL_LOGGING)

#define DEBUG_PRINT(Category, ErrorLevel, Args) \
    /* NOTHING */

#define DEBUG_PRINT_API(Category, ErrorLevel, Args) \
    if (((InternetDebugCategoryFlags & DBG_ ## Category) \
        && (DBG_ ## ErrorLevel >= InternetDebugErrorLevel)) \
    || ((InternetDebugControlFlags & DBG_AT_ERROR_LEVEL) \
        && (DBG_ ## ErrorLevel >= InternetDebugErrorLevel))) { \
        InternetDebugPrint Args; \
    }

#else

#define DEBUG_PRINT_API DEBUG_PRINT
#define DEBUG_PRINT(Category, ErrorLevel, Args) \
    if (((InternetDebugCategoryFlags & DBG_ ## Category) \
        && (DBG_ ## ErrorLevel >= InternetDebugErrorLevel)) \
    || ((InternetDebugControlFlags & DBG_AT_ERROR_LEVEL) \
        && (DBG_ ## ErrorLevel >= InternetDebugErrorLevel))) { \
        InternetDebugPrint Args; \
    }

#endif // defined(RETAIL_LOGGING)

//
// DEBUG_PUT - prints formatted string to debug output stream
//

#if defined(RETAIL_LOGGING)

#define DEBUG_PUT(Args) \
    /* NOTHING */

#else

#define DEBUG_PUT(Args) \
    InternetDebugPrintf Args

#endif // defined(RETAIL_LOGGING)

//
// DEBUG_DUMP - dump data
//

#if defined(RETAIL_LOGGING)

#define DEBUG_DUMP(Category, Text, Address, Length) \
    /* NOTHING */

#define DEBUG_DUMP_API(Category, Text, Address, Length) \
    if (InternetDebugCategoryFlags & DBG_ ## Category) { \
        InternetDebugDump(Text, (LPBYTE)Address, Length); \
    }

#else

#define DEBUG_DUMP_API DEBUG_DUMP
#define DEBUG_DUMP(Category, Text, Address, Length) \
    if (InternetDebugCategoryFlags & DBG_ ## Category) { \
        InternetDebugDump(Text, (LPBYTE)Address, Length); \
    }

#endif // defined(RETAIL_LOGGING)

//
// DEBUG_BREAK - break into debugger if break flag is set for this module
//

#define DEBUG_BREAK(Module) \
    if (InternetDebugBreakFlags & DBG_ ## Module) { \
        InternetDebugPrintf("Breakpoint. File %s Line %d\n", \
                            __FILE__, \
                            __LINE__ \
                            ); \
        DebugBreak(); \
    }

//
// WAIT_FOR_SINGLE_OBJECT - perform WaitForSingleObject and check we didn't
// get a timeout
//

#define WAIT_FOR_SINGLE_OBJECT(Object, Error) \
    Error = WaitForSingleObject((Object), DEBUG_WAIT_TIME); \
    if (Error == WAIT_TIMEOUT) { \
        InternetDebugPrintf("single object timeout\n"); \
        DebugBreak(); \
    }

//
// DEBUG_WAIT_TIMER - create DWORD variable for holding time
//

#define DEBUG_WAIT_TIMER(TimerVar) \
    DWORD TimerVar

//
// DEBUG_START_WAIT_TIMER - get current tick count
//

#define DEBUG_START_WAIT_TIMER(TimerVar) \
    TimerVar = GetTickCount()

//
// DEBUG_CHECK_WAIT_TIMER - get the current number of ticks, subtract from the
// previous value recorded by DEBUG_START_WAIT_TIMER and break to debugger if
// outside the predefined range
//

#define DEBUG_CHECK_WAIT_TIMER(TimerVar, MilliSeconds) \
    TimerVar = GetTickCount() - TimerVar; \
    if (TimerVar > MilliSeconds) { \
        InternetDebugPrintf("Wait time (%d mSecs) exceeds acceptable value (%d mSecs)\n", \
                            TimerVar, \
                            MilliSeconds \
                            ); \
        DebugBreak(); \
    }

#define DEBUG_DATA(Type, Name, InitialValue) \
    Type Name = InitialValue

#define DEBUG_DATA_EXTERN(Type, Name) \
    extern Type Name

#define DEBUG_LABEL(label) \
    label:

#define DEBUG_GOTO(label) \
    goto label

#define DEBUG_ONLY(x) \
    x

#if defined(i386)

#define GET_CALLERS_ADDRESS(p, pp)  x86SleazeCallersAddress(p, pp)
#define GET_CALL_STACK(p)           x86SleazeCallStack((LPVOID *)&p, ARRAY_ELEMENTS(p), 0)

#else

#define GET_CALLERS_ADDRESS(p, pp)
#define GET_CALL_STACK(p)

#endif // defined(i386)

#else // end #if INET_DEBUG

#define IF_DEBUG_CODE() \
    if (0)

#define IF_DEBUG(x) \
    if (0)

#define IF_DEBUG_CONTROL(x) \
    if (0)

#define DEBUG_ENTER(ParameterList) \
    /* NOTHING */

#define DEBUG_ENTER_API(ParameterList) \
    /* NOTHING */

#define DEBUG_LEAVE(Variable) \
    /* NOTHING */

#define DEBUG_LEAVE_API(Variable) \
    /* NOTHING */

#define DEBUG_ERROR(Category, Error) \
    /* NOTHING */

#define DEBUG_PRINT(Category, ErrorLevel, Args) \
    /* NOTHING */

#define DEBUG_PRINT_API(Category, ErrorLevel, Args) \
    /* NOTHING */

#define DEBUG_PUT(Args) \
    /* NOTHING */

#define DEBUG_DUMP(Category, Text, Address, Length) \
    /* NOTHING */

#define DEBUG_DUMP_API(Category, Text, Address, Length) \
    /* NOTHING */

#define DEBUG_BREAK(module) \
    /* NOTHING */

#define WAIT_FOR_SINGLE_OBJECT(Object, Error) \
    Error = WaitForSingleObject((Object), INFINITE)

#define DEBUG_WAIT_TIMER(TimerVar) \
    /* NOTHING */

#define DEBUG_START_WAIT_TIMER(TimerVar) \
    /* NOTHING */

#define DEBUG_CHECK_WAIT_TIMER(TimerVar, MilliSeconds) \
    /* NOTHING */

#define DEBUG_DATA(Type, Name, InitialValue) \
    /* NOTHING */

#define DEBUG_DATA_EXTERN(Type, Name) \
    /* NOTHING */

#define DEBUG_LABEL(label) \
    /* NOTHING */

#define DEBUG_GOTO(label) \
    /* NOTHING */

#define DEBUG_ONLY(x) \
    /* NOTHING */

#endif // INET_DEBUG

#if defined(__cplusplus)
}
#endif
