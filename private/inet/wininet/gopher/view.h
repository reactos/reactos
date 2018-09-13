/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    view.h

Abstract:

    Manifests, macros, types prototypes for view.c

Author:

    Richard L Firth (rfirth) 17-Oct-1994

Revision History:

    17-Oct-1994 rfirth
        Created

--*/

#if defined(__cplusplus)
extern "C" {
#endif

//
// have to forward-define LPSESSION_INFO
//

typedef struct _SESSION_INFO * LPSESSION_INFO;

//
// VIEW_TYPE - which type of view are we talking about, FIND or FILE?
//

typedef enum {
    ViewTypeFile = 0xff010101,  // arbitrary values always good for a few laughs
    ViewTypeFind
} VIEW_TYPE;

//
// VIEW_INFO - describes a data view, either the results of a FindFirst or a
// GetFile
//

typedef struct {

    //
    // List - the list of VIEW_INFO structures owned by the parent SESSION_INFO
    //

    LIST_ENTRY List;

    //
    // ViewType - lets us know which of the Session view lists this is on
    //

    VIEW_TYPE ViewType;

    //
    // Handle - the handle returned by GopherFindFirst/GopherGetFile
    //

    HINTERNET Handle;

    //
    // Request - the request string which generated Buffer
    //

    LPSTR Request;

    //
    // RequestLength - number of bytes in Request (excluding terminating \0)
    //

    DWORD RequestLength;

    //
    // Set to 1 when this 'object' is created. Any time it is used thereafter
    // this field must be incremented and decremented when no longer being
    // used. Closing the handle that corresponds to this view will dereference
    // it a final time and cause the view to be deleted
    //

    LONG ReferenceCount;

    //
    // Flags - various control flags, see below
    //

    DWORD Flags;

    //
    // ViewOffset - offset in buffer described by BufferInfo->Buffer which will
    // be used to generate the results of the next request on this view
    //

    DWORD ViewOffset;

    //
    // Buffer - pointer to BUFFER_INFO containing data returned from gopher
    // server
    //

    LPBUFFER_INFO BufferInfo;

    //
    // SessionInfo - back-pointer to the owning SESSION_INFO. Used when we
    // create or destroy this view - the owning session must be referenced
    // or dereferenced accordingly
    //

    LPSESSION_INFO SessionInfo;

} VIEW_INFO, *LPVIEW_INFO;

//
// VIEW_INFO flags
//

#define VI_GOPHER_PLUS      0x00000001  // the data buffer contains gopher+ data
#define VI_CLEANUP          0x00000002  // set by CleanupSessions()

//
// external data
//

DEBUG_DATA_EXTERN(LONG, NumberOfViews);

//
// prototypes
//

LPVIEW_INFO
CreateView(
    IN LPSESSION_INFO SessionInfo,
    IN VIEW_TYPE ViewType,
    IN LPSTR Request,
    OUT LPDWORD Error,
    OUT LPBOOL Cloned
    );

LPVIEW_INFO
FindViewByHandle(
    IN HINTERNET Handle,
    IN VIEW_TYPE ViewType
    );

VOID
ReferenceView(
    IN LPVIEW_INFO ViewInfo
    );

LPVIEW_INFO
DereferenceView(
    IN LPVIEW_INFO ViewInfo
    );

DWORD
DereferenceViewByHandle(
    IN HINTERNET Handle,
    IN VIEW_TYPE ViewType
    );

//
// macros
//

#if INET_DEBUG

#define VIEW_CREATED()      ++NumberOfViews
#define VIEW_DESTROYED()    --NumberOfViews
#define ASSERT_NO_VIEWS() \
    if (NumberOfViews != 0) { \
        INET_ASSERT(FALSE); \
    }

#else

#define VIEW_CREATED()      /* NOTHING */
#define VIEW_DESTROYED()    /* NOTHING */
#define ASSERT_NO_VIEWS()   /* NOTHING */

#endif // INET_DEBUG

#if defined(__cplusplus)
}
#endif
