/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    Contains data definitions for debug code.

Author:

    Madan Appiah (madana) 15-Nov-1994

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _DEBUG_
#define _DEBUG_

#ifdef __cplusplus
extern "C" {
#endif

// Event tracking macros...
#define EVENTWRAP(API, h) {\
    BOOL ret = API(h);\
    if (ret) \
        TcpsvcsDbgPrint((DEBUG_APIS, #API "(" #h "=%d)\n", h)); \
    else { \
        TcpsvcsDbgPrint((DEBUG_APIS, #API "(" #h "=%d) failed err=%d\n",\
            h, GetLastError())); \
        TcpsvcsDbgAssert( FALSE ); \
    } \
}\

#define   SETEVENT(h)  EVENTWRAP(SetEvent,    h)
#define RESETEVENT(h)  EVENTWRAP(ResetEvent,  h)
#define CLOSEHANDLE(h) EVENTWRAP(CloseHandle, h)

//
// LOW WORD bit mask (0x0000FFFF) for low frequency debug output.
//
#define DEBUG_ERRORS            0x00000001  // hard errors.
#define DEBUG_REGISTRY          0x00000002  // debug registry calls
#define DEBUG_MISC              0x00000004  // misc info.
#define DEBUG_SCAVENGER         0x00000008  // scavenger debug info.

#define DEBUG_SORT              0x00000010  // debug B-TREE functions
#define DEBUG_CONTAINER         0x00000020  // debug container
#define DEBUG_APIS              0x00000040  // debug tcpsvcs apis
#define DEBUG_FILE_VALIDATE     0x00000080 // validate file map file
#define DEBUG_SVCLOC_MESSAGE    0x00000100  // discovery messages



//
// HIGH WORD bit mask (0x0000FFFF) for high frequency debug output.
// ie more verbose.
//

#define DEBUG_TIMESTAMP         0x00010000  // print time stamps
#define DEBUG_MEM_ALLOC         0x00020000 // memory alloc
#define DEBUG_STARTUP_BRK       0x40000000  // breakin debugger during startup.

#define ENTER_CACHE_API(paramlist) \
{ DEBUG_ONLY(LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();) \
  DEBUG_ENTER_API(paramlist); \
}

#define LEAVE_CACHE_API() \
Cleanup:                         \
    if (Error != ERROR_SUCCESS)  \
    {                            \
        SetLastError( Error );   \
        DEBUG_ERROR(INET, Error); \
    }                            \
    DEBUG_LEAVE_API (Error==ERROR_SUCCESS);      \
    return (Error==ERROR_SUCCESS);                 \

#if DBG

///#define DEBUG_PRINT OutputDebugString

//
// debug functions.
//

#define TcpsvcsDbgPrint(_x_) TcpsvcsDbgPrintRoutine _x_

VOID
TcpsvcsDbgPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    ...
    );

#define TcpsvcsDbgAssert(Predicate) INET_ASSERT(Predicate)

#else

///#define IF_DEBUG(flag) if (FALSE)

#define TcpsvcsDbgPrint(_x_)
#define TcpsvcsDbgAssert(_x_)

#endif // DBG

#if DBG
#define INLINE
#else
#define INLINE      inline
#endif

#ifdef __cplusplus
}
#endif

#endif  // _DEBUG_
