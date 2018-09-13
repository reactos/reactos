/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    buffer.h

Abstract:

    Contains manifests, macros, types and typedefs for buffer.c

Author:

    Richard L Firth (rfirth) 31-Oct-1994

Revision History:

    31-Oct-1994 rfirth
        Created

--*/

#if defined(__cplusplus)
extern "C" {
#endif

//
// manifests
//

//
// LOOK_AHEAD_LENGTH - read enough characters to comprise the maximum (32-bit)
// length returned by gopher+
//

#define LOOK_AHEAD_LENGTH   sizeof("+4294967296\r\n")   // 12

//
// BUFFER_INFO - structure maintains information about responses from gopher
// server. The same data can be shared amongst many requests
//

typedef struct {

    //
    // ReferenceCount - number of VIEW_INFO structures referencing this buffer
    //

    LONG ReferenceCount;

    //
    // Flags - information about the buffer
    //

    DWORD Flags;

    //
    // RequestEvent - the owner of this event is the only thread which can
    // make the gopher request that creates this buffer info. All other
    // requesters for the same information wait for the first thread to signal
    // the event, then just read the data returned by the first thread
    //

//    HANDLE RequestEvent;

    //
    // RequestWaiters - used in conjunction with RequestEvent. If this field
    // is 0 by the time the initial requester thread comes to signal the
    // event, it can do away with the event altogether, since it was only
    // required to stop those other threads from making a redundant request
    //

//    DWORD RequestWaiters;

    //
    // ConnectedSocket - contains socket we are using to receive the data in
    // the buffer, and the index into the parent SESSION_INFO's
    // ADDRESS_INFO_LIST of the address used to connect the socket
    //

//    CONNECTED_SOCKET ConnectedSocket;
    ICSocket * Socket;

    //
    // ResponseLength - the response length as told to us by the gopher+ server
    //

    int ResponseLength;

    //
    // BufferLength - length of Buffer
    //

    DWORD BufferLength;

    //
    // Buffer - containing response
    //

    LPBYTE Buffer;

    //
    // ResponseInfo - we read the gopher+ header information (i.e. length) here.
    // The main reason is so that we can determine the length of a gopher+ file
    // even though we were given a zero-length user buffer
    //

    char ResponseInfo[LOOK_AHEAD_LENGTH];

    //
    // BytesRemaining - number of bytes left in ResponseInfo that are data
    //

    int BytesRemaining;

    //
    // DataBytes - pointer into ResponseInfo where data bytes start
    //

    LPBYTE DataBytes;

    //
    // CriticalSection - used to serialize readers
    //

//    CRITICAL_SECTION CriticalSection;

} BUFFER_INFO, *LPBUFFER_INFO;

//
// BUFFER_INFO flags
//

#define BI_RECEIVE_COMPLETE 0x00000001  // receiver thread has finished
#define BI_DOT_AT_END       0x00000002  // buffer terminated with ".\r\n"
#define BI_BUFFER_RESPONSE  0x00000004  // response is buffered internally
#define BI_ERROR_RESPONSE   0x00000008  // the server responded with an error
#define BI_MOVEABLE         0x00000010  // buffer is moveable memory
#define BI_FIRST_RECEIVE    0x00000020  // this is the first receive
#define BI_OWN_BUFFER       0x00000040  // set if we own the buffer (directory)

//
// external data
//

DEBUG_DATA_EXTERN(LONG, NumberOfBuffers);

//
// prototypes
//

LPBUFFER_INFO
CreateBuffer(
    OUT LPDWORD Error
    );

VOID
DestroyBuffer(
    IN LPBUFFER_INFO BufferInfo
    );

VOID
AcquireBufferLock(
    IN LPBUFFER_INFO BufferInfo
    );

VOID
ReleaseBufferLock(
    IN LPBUFFER_INFO BufferInfo
    );

VOID
ReferenceBuffer(
    IN LPBUFFER_INFO BufferInfo
    );

LPBUFFER_INFO
DereferenceBuffer(
    IN LPBUFFER_INFO BufferInfo
    );

//
// macros
//

#if INET_DEBUG

#define BUFFER_CREATED()    ++NumberOfBuffers
#define BUFFER_DESTROYED()  --NumberOfBuffers
#define ASSERT_NO_BUFFERS() \
    if (NumberOfBuffers != 0) { \
        INET_ASSERT(FALSE); \
    }

#else

#define BUFFER_CREATED()    /* NOTHING */
#define BUFFER_DESTROYED()  /* NOTHING */
#define ASSERT_NO_BUFFERS() /* NOTHING */

#endif

#if defined(__cplusplus)
}
#endif
