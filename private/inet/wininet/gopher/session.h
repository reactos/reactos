/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    session.h

Abstract:

    Manifests, macros, types, prototypes for session.c

Author:

    Richard L Firth (rfirth) 25-Oct-1994

Revision History:

    25-Oct-1994 rfirth
        Created

--*/

#if defined(__cplusplus)
extern "C" {
#endif

//
// types
//

//
// SESSION_INFO - describes a session with a gopher server. We will keep a cache
// of these. Only one 'conversation' can be active at any one time with a gopher
// server. Threads must wait on the mutex handle
//

typedef struct _SESSION_INFO {

    //
    // List - SESSION_INFOs are maintained on double-linked list
    //

    LIST_ENTRY List;

    //
    // ReferenceCount - used to keep this session alive when there are
    // concurrent creates/deletes on different threads
    //

    LONG ReferenceCount;

    //
    // Handle - identifies this session to the application
    //

    HANDLE Handle;

    //
    // Flags - various control flags. See below
    //

    DWORD Flags;

    //
    // Host - name of host with which we have a connection
    //

    LPSTR Host;

    //
    // Port - port number at which gopher server listens at Host
    //

    DWORD Port;

    //
    // FindList - protected, doubly-linked list of VIEW_INFO 'object's generated
    // by gopher directory requests
    //

    SERIALIZED_LIST FindList;

    //
    // FileList - protected, doubly-linked list of VIEW_INFO 'object's generated
    // by gopher document (file) requests
    //

    SERIALIZED_LIST FileList;

} SESSION_INFO, *LPSESSION_INFO;

//
// SESSION_INFO flags
//

#define SI_GOPHER_PLUS      0x00000001  // gopher server at Host is gopher+
#define SI_CLEANUP          0x00000002  // set by CleanupSession()
#define SI_PERSISTENT       0x80000000  // connection to gopher server kept alive

//
// macros
//

#define UNKNOWN_GOPHER(session) ((session)->Flags & (SI_GOPHER_ZERO | SI_GOPHER_PLUS) == 0)

//
// public data
//

extern SERIALIZED_LIST SessionList;

DEBUG_DATA_EXTERN(LONG, NumberOfSessions);

//
// prototypes
//

VOID
AcquireSessionLock(
    VOID
    );

VOID
ReleaseSessionLock(
    VOID
    );

VOID
CleanupSessions(
    VOID
    );

LPSESSION_INFO
FindOrCreateSession(
    IN LPSTR Host,
    IN DWORD Port,
    OUT LPDWORD Error
    );

VOID
ReferenceSession(
    IN LPSESSION_INFO SessionInfo
    );

LPSESSION_INFO
DereferenceSession(
    IN LPSESSION_INFO SessionInfo
    );

VOID
AcquireViewLock(
    IN LPSESSION_INFO SessionInfo,
    IN VIEW_TYPE ViewType
    );

VOID
ReleaseViewLock(
    IN LPSESSION_INFO SessionInfo,
    IN VIEW_TYPE ViewType
    );

DWORD
GopherTransaction(
    IN LPVIEW_INFO ViewInfo
    );

BOOL
IsGopherPlusSession(
    IN LPSESSION_INFO SessionInfo
    );

DWORD
SearchSessionsForAttribute(
    IN LPSTR Locator,
    IN LPSTR Attribute,
    IN LPBYTE Buffer,
    IN OUT LPDWORD BufferLength
    );

//
// macros
//

#if INET_DEBUG

#define SESSION_CREATED()   ++NumberOfBuffers
#define SESSION_DESTROYED() --NumberOfBuffers
#define ASSERT_NO_SESSIONS() \
    if (NumberOfSessions != 0) { \
        INET_ASSERT(FALSE); \
    }

#else

#define SESSION_CREATED()       /* NOTHING */
#define SESSION_DESTROYED()     /* NOTHING */
#define ASSERT_NO_SESSIONS()    /* NOTHING */

#endif

#if defined(__cplusplus)
}
#endif
