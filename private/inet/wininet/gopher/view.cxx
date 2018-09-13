/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    view.cxx

Abstract:

    Functions to manage VIEW 'object's

    Contents:
        CreateView
        (DestroyView)
        FindViewByHandle
        ReferenceView
        DereferenceView
        DereferenceViewByHandle

Author:

    Richard L Firth (rfirth) 17-Oct-1994

Environment:

    Win/32 user-mode DLL

Revision History:

    17-Oct-1994 rfirth
        Created

--*/

#include <wininetp.h>
#include "gfrapih.h"

//
// private prototypes
//

PRIVATE
VOID
DestroyView(
    IN LPVIEW_INFO ViewInfo
    );

//
// private data
//

DEBUG_DATA(LONG, NumberOfViews, 0);

//
// functions
//


LPVIEW_INFO
CreateView(
    IN LPSESSION_INFO SessionInfo,
    IN VIEW_TYPE ViewType,
    IN LPSTR Request,
    OUT LPDWORD Error,
    OUT LPBOOL Cloned
    )

/*++

Routine Description:

    Creates or clones a VIEW_INFO. There will only ever be one VIEW_INFO for a
    particular request to the same server. Other requests for the same data
    just reference the first view

Arguments:

    SessionInfo - pointer to SESSION_INFO describing gopher server

    ViewType    - type of view - ViewTypeFile or ViewTypeFind

    Request     - gopher request string

    Error       - returned error

    Cloned      - returned TRUE if we cloned the view

Return Value:

    LPVIEW_INFO
        Success - pointer to created or cloned view; check Cloned

        Failure - NULL

--*/

{
    LPVIEW_INFO viewInfo;
    DWORD error;

    DEBUG_ENTER((DBG_GOPHER,
                Pointer,
                "CreateView",
                "%x, %x, %q, %x, %x",
                SessionInfo,
                ViewType,
                Request,
                Error,
                Cloned
                ));

    //
    // in both cases, we need a new VIEW_INFO
    //

    viewInfo = NEW(VIEW_INFO);
    if (viewInfo != NULL) {
        error = AllocateHandle((LPVOID)viewInfo, &viewInfo->Handle);
        if (error == ERROR_SUCCESS) {
            viewInfo->Request = NEW_STRING(Request);
            viewInfo->RequestLength = strlen(Request);
            if (viewInfo->Request != NULL) {
                InitializeListHead(&viewInfo->List);
                viewInfo->ViewType = ViewType;
            } else {
                error = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
    } else {
        error = ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // now search for an already existing view
    //

    if (error == ERROR_SUCCESS) {

        PLIST_ENTRY listPtr;
        PLIST_ENTRY list;
        PLIST_ENTRY listAddress;
        BOOL found;
        LPBUFFER_INFO bufferInfo;

        AcquireViewLock(SessionInfo, ViewType);

        //
        // new scheme: we now only buffer Find views: we always create a new
        // file request because file requests go straight to the user buffer
        // (keeping the connection alive until the caller finishes reading)
        //

        found = FALSE;
        if (ViewType == ViewTypeFind) {
//            list = SessionInfo->FindList.List.Flink;
            listAddress = &SessionInfo->FindList.List;
//            for (listPtr = list; listPtr != listAddress; listPtr = listPtr->Flink) {
//                if (!STRICMP(((LPVIEW_INFO)listPtr)->Request, Request)) {
//                    found = TRUE;
//                    break;
//                }
//            }
        } else {
            listAddress = &SessionInfo->FileList.List;
        }

        //
        // create a buffer info, or get a pointer to the current buffer info
        // depending on whether we located a view of the same request at the
        // same server
        //
/* N.B. found is never true because the above code is commented out. */
        if (found) {
//            bufferInfo = ((LPVIEW_INFO)listPtr)->BufferInfo;

            //
            // this is a clone. If there is a thread which is concurrently
            // requesting the data - right now - then we must wait on the
            // RequestEvent until gives us the green light. If there is no
            // RequestEvent then the data has already been received and
            // some kind soul has seen fit to close the request handle. I.e.
            // we don't have to wait
            //

//            if (bufferInfo->RequestEvent != NULL) {
//                ++bufferInfo->RequestWaiters;
//            }

            *Cloned = TRUE;
        } else {
            bufferInfo = CreateBuffer(&error);
            if (bufferInfo != NULL) {
                *Cloned = FALSE;
            }
        }

        if (error == ERROR_SUCCESS) {

            //
            // whilst holding the view lock, we add the view to the list, set
            // the view's reference count to 1, increment the session's
            // reference count, and point the view at the session
            //

            viewInfo->ReferenceCount = 1;
            viewInfo->BufferInfo = bufferInfo;
            viewInfo->SessionInfo = SessionInfo;

            //
            // also whilst holding the view lock, we increment the reference
            // count on the buffer info
            //

            ReferenceBuffer(bufferInfo);
            InsertHeadList(listAddress, &viewInfo->List);

            //
            // N.B. this will acquire the session list lock (then release it)
            //

            ReferenceSession(SessionInfo);

            VIEW_CREATED();

        }

        ReleaseViewLock(SessionInfo, ViewType);
    }

    if (error != ERROR_SUCCESS) {
        if (viewInfo != NULL) {
            viewInfo = DereferenceView(viewInfo);

            INET_ASSERT(viewInfo == NULL);

        }
    }

    DEBUG_PRINT(VIEW,
                INFO,
                ("ViewInfo %x is %scloned\n",
                viewInfo,
                (*Cloned == TRUE) ? "" : "NOT "
                ));

    DEBUG_ERROR(SESSION, error);

    *Error = error;

    DEBUG_LEAVE(viewInfo);

    return viewInfo;
}


PRIVATE
VOID
DestroyView(
    IN LPVIEW_INFO ViewInfo
    )

/*++

Routine Description:

    Destroys a VIEW_INFO after freeing all resources that it owns. ViewInfo
    must have been removed from the relevant session view list by the time
    this function is called

Arguments:

    ViewInfo    - pointer to VIEW_INFO to destroy

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_GOPHER,
                None,
                "DestroyView",
                "%x",
                ViewInfo
                ));

    INET_ASSERT(ViewInfo != NULL);
    INET_ASSERT(ViewInfo->ReferenceCount == 0);

    if (ViewInfo->Handle) {

        DWORD error;

        error = FreeHandle(ViewInfo->Handle);

        INET_ASSERT(error == ERROR_SUCCESS);

    }

    if (ViewInfo->Request) {
        DEL_STRING(ViewInfo->Request);
    }

    DEL(ViewInfo);

    VIEW_DESTROYED();

    DEBUG_LEAVE(0);
}


LPVIEW_INFO
FindViewByHandle(
    IN HANDLE Handle,
    IN VIEW_TYPE ViewType
    )

/*++

Routine Description:

    Finds a VIEW_INFO given a handle. If found, the VIEW_INFO reference count
    is incremented

Arguments:

    Handle      - handle associated with VIEW_INFO

    ViewType    - identifies which list to look on

Return Value:

    LPVIEW_INFO
        Success - pointer to VIEW_INFO; Session points at SESSION_INFO

        Failure - NULL

--*/

{
    LPVIEW_INFO viewInfo;
    LPSESSION_INFO sessionInfo;
    BOOL found = FALSE;

    DEBUG_ENTER((DBG_GOPHER,
                Pointer,
                "FindViewByHandle",
                "%x, %x",
                Handle,
                ViewType
                ));

    AcquireSessionLock();

    sessionInfo = (LPSESSION_INFO)SessionList.List.Flink;
    while (sessionInfo != (LPSESSION_INFO)&SessionList.List) {

        PLIST_ENTRY endOfList;
        PLIST_ENTRY list;

        AcquireViewLock(sessionInfo, ViewType);

        INET_ASSERT((ViewType == ViewTypeFile) || (ViewType == ViewTypeFind));

        if (ViewType == ViewTypeFile) {
            list = sessionInfo->FileList.List.Flink;
            endOfList = &sessionInfo->FileList.List;
        } else {
            list = sessionInfo->FindList.List.Flink;
            endOfList = &sessionInfo->FindList.List;
        }

        for (viewInfo = (LPVIEW_INFO)list;
            viewInfo != (LPVIEW_INFO)endOfList;
            viewInfo = (LPVIEW_INFO)(viewInfo->List.Flink)) {

            if (viewInfo->Handle == Handle) {

                //
                // we found the one we were looking for. Make sure that we
                // increase the reference count before we release the lock
                //

                INET_ASSERT(viewInfo->ViewType == ViewType);

                ReferenceView(viewInfo);
                found = TRUE;
                break;  // out of for()
            }
        }

        ReleaseViewLock(sessionInfo, ViewType);

        if (found) {
            break;  // out of while()
        } else {
            sessionInfo = (LPSESSION_INFO)sessionInfo->List.Flink;
        }
    }
    if (!found) {
        viewInfo = NULL;
    }

    ReleaseSessionLock();

    DEBUG_LEAVE(viewInfo);

    return viewInfo;
}


VOID
ReferenceView(
    IN LPVIEW_INFO ViewInfo
    )

/*++

Routine Description:

    Increments reference count on VIEW_INFO

Arguments:

    ViewInfo    - pointer to VIEW_INFO to reference

Return Value:

    None.

--*/

{
    INET_ASSERT(ViewInfo != NULL);

    InterlockedIncrement(&ViewInfo->ReferenceCount);

    DEBUG_PRINT(REFCOUNT,
                INFO,
                ("ReferenceView(): ViewInfo{%x}->ReferenceCount = %d\n",
                ViewInfo,
                ViewInfo->ReferenceCount
                ));
}


LPVIEW_INFO
DereferenceView(
    IN LPVIEW_INFO ViewInfo
    )

/*++

Routine Description:

    Reduces the reference count of a VIEW_INFO by 1. If it goes to zero, the
    VIEW_INFO is removed from its owner's list and is deallocated

Arguments:

    ViewInfo    - pointer to VIEW_INFO to dereference

Return Value:

    LPVIEW_INFO
        NULL    - ViewInfo was deleted

        !NULL   - Reference count still >0

--*/

{
    BOOL deleteViewInfo;

    DEBUG_ENTER((DBG_GOPHER,
                Pointer,
                "DereferenceView",
                "%x",
                ViewInfo
                ));

    INET_ASSERT(ViewInfo != NULL);
    INET_ASSERT(ViewInfo->ReferenceCount >= 1);

    deleteViewInfo = FALSE;
    if (InterlockedDecrement(&ViewInfo->ReferenceCount) == 0) {

        //
        // perform any reference count manipulation within the view lock
        //

        AcquireViewLock(ViewInfo->SessionInfo, ViewInfo->ViewType);

        //
        // if the reference count is still zero, then it is safe to remove it
        // from the SESSION_INFO, and delete
        //

        if (ViewInfo->ReferenceCount == 0) {

            RemoveEntryList(&ViewInfo->List);

            INET_ASSERT(ViewInfo->BufferInfo != NULL);

            //
            // perform buffer info reference count manipulation within the view
            // lock
            //

            DereferenceBuffer(ViewInfo->BufferInfo);
            deleteViewInfo = TRUE;
        }

        ReleaseViewLock(ViewInfo->SessionInfo, ViewInfo->ViewType);
    } else {

        DEBUG_PRINT(REFCOUNT,
                    INFO,
                    ("DereferenceView(): ViewInfo{%x}->ReferenceCount = %d\n",
                    ViewInfo,
                    ViewInfo->ReferenceCount
                    ));

    }

    //
    // safe to delete this view info outside of the view lock
    //

    if (deleteViewInfo) {

        LPSESSION_INFO sessionInfo;

        sessionInfo = ViewInfo->SessionInfo;

        DestroyView(ViewInfo);
        ViewInfo = NULL;
        DereferenceSession(sessionInfo);
    }

    DEBUG_LEAVE(ViewInfo);

    return ViewInfo;
}


DWORD
DereferenceViewByHandle(
    IN HINTERNET Handle,
    IN VIEW_TYPE ViewType
    )

/*++

Routine Description:

    Atomically searches for the VIEW_INFO identified by Handle and if found
    dereferences it. This may cause the VIEW_INFO to be deleted

Arguments:

    Handle      - identifying VIEW_INFO to dereference

    ViewType    - identifies which list to look on

Return Value:

    DWORD
        Success - ERROR_SUCCESS
                    Handle was dereferenced, but not necessarily deleted

        Failure - ERROR_INVALID_HANDLE
                    Couldn't find Handle

--*/

{
    LPVIEW_INFO viewInfo;
    DWORD error;

    DEBUG_ENTER((DBG_GOPHER,
                Dword,
                "DereferenceViewByHandle",
                "%x",
                Handle
                ));

    //
    // we have to perform this operation whilst holding the session lock. We
    // find the VIEW_INFO then dereference it twice - once to counteract the
    // reference added by FindViewByHandle(), and once for the reference we
    // were asked to remove by the caller.
    //
    // Since we are holding the Session lock, no other thread can dereference
    // the VIEW_INFO or SESSION_INFO. The second derereference may cause the
    // SESSION_INFO to be destroyed
    //

    AcquireSessionLock();

    viewInfo = FindViewByHandle(Handle, ViewType);
    if (viewInfo != NULL) {
        DereferenceView(viewInfo);
        DereferenceView(viewInfo);
        error = ERROR_SUCCESS;
    } else {
        error = ERROR_INVALID_HANDLE;
    }

    ReleaseSessionLock();

    DEBUG_LEAVE(error);

    return error;
}
