/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    session.cxx

Abstract:

    Contains functions for maintaining the global session list and SESSION_INFO
    specific functions

    Contents:
        AcquireSessionLock
        ReleaseSessionLock
        CleanupSessions
        (CleanupViewList)
        FindOrCreateSession
        (CreateSession)
        (DestroySession)
        ReferenceSession
        DereferenceSession
        AcquireViewLock
        ReleaseViewLock
        GopherTransaction
        IsServerGopherPlus
        IsGopherPlusSession
        SearchSessionsForAttribute

Author:

    Richard L Firth (rfirth) 19-Oct-1994

Environment:

    Win32 DLL

Revision History:

    19-Oct-1994 rfirth
        Created

--*/

#include <wininetp.h>
#include "gfrapih.h"

//
// manifests
//

#define NULL_HANDLE ((HANDLE)0)

//
// private prototypes
//

PRIVATE
VOID
CleanupViewList(
    IN LPSESSION_INFO SessionInfo,
    IN VIEW_TYPE ViewType
    );

PRIVATE
LPSESSION_INFO
CreateSession(
    IN LPSTR Host,
    IN DWORD Port,
    OUT LPDWORD Error
    );

PRIVATE
VOID
DestroySession(
    IN LPSESSION_INFO SessionInfo
    );

//
// data
//

PUBLIC SERIALIZED_LIST SessionList;

DEBUG_DATA(LONG, NumberOfSessions, 0);

//
// functions
//


VOID
AcquireSessionLock(
    VOID
    )

/*++

Routine Description:

    Acquires the SESSION_INFO list lock

Arguments:

    None.

Return Value:

    None.

--*/

{
    LockSerializedList(&SessionList);
}


VOID
ReleaseSessionLock(
    VOID
    )

/*++

Routine Description:

    Releases the SESSION_INFO list lock

Arguments:

    None.

Return Value:

    None.

--*/

{
    UnlockSerializedList(&SessionList);
}


VOID
CleanupSessions(
    VOID
    )

/*++

Routine Description:

    Tries to Remove all SESSION_INFOs from the session list, and terminate all
    active operations

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_GOPHER,
                None,
                "CleanupSessions",
                NULL
                ));

    while (1) {

        LPSESSION_INFO sessionInfo;

        //
        // find the next SESSION_INFO to delete. Because we may cause the entry
        // currently at the head of the list to be deleted during this loop, we
        // must walk the list each time. We may also end up with a list of items
        // that are marked for delete, but cannot be deleted until the threads
        // that own them complete their current operations
        //

        AcquireSessionLock();

        for (sessionInfo = (LPSESSION_INFO)HeadOfSerializedList(&SessionList);
            (sessionInfo != (LPSESSION_INFO)&SessionList.List.Flink)
            && !(sessionInfo->Flags & SI_CLEANUP);
            sessionInfo = (LPSESSION_INFO)sessionInfo->List.Flink) {

            //
            // empty loop
            //

        }
        if (sessionInfo == (LPSESSION_INFO)&SessionList.List.Flink) {

            DEBUG_PRINT(SESSION,
                        INFO,
                        ("end of list\n"
                        ));

            ReleaseSessionLock();
            break;
        }

        //
        // mark this SESSION_INFO as being cleaned up, in case it does not get
        // removed from the list (some other thread is accessing it)
        //

        sessionInfo->Flags |= SI_CLEANUP;

        //
        // increment the reference count so that we can release the list lock
        //

        ReferenceSession(sessionInfo);
        ReleaseSessionLock();

        //
        // now we have a pointer to a SESSION_INFO that cannot be deleted until
        // after we have dereferenced it. Dereference any items in the Find and
        // File lists. Note that had we not referenced the SESSION_INFO above,
        // it might have gotten deleted after cleaning up the Find list, and we
        // would be in danger of passing a bogus pointer to the second cleanup
        // view list call below
        //

        CleanupViewList(sessionInfo, ViewTypeFind);
        CleanupViewList(sessionInfo, ViewTypeFile);

        //
        // finally, dereference the session. This may cause it to be deleted,
        // and for the list to be changed
        //

        DereferenceSession(sessionInfo);
    }

    DEBUG_LEAVE(0);
}


PRIVATE
VOID
CleanupViewList(
    IN LPSESSION_INFO SessionInfo,
    IN VIEW_TYPE ViewType
    )

/*++

Routine Description:

    Cleans up a VIEW_INFO list on a SESSION_INFO

Arguments:

    SessionInfo - pointer to SESSION_INFO we are cleaning up

    ViewType    - identifies which VIEW_INFO list to clean up

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_GOPHER,
                None,
                "CleanupViewList",
                "%x, %x",
                SessionInfo,
                ViewType
                ));

    //
    // walk this VIEW_INFO list trying to delete everything by dereferencing
    //

    while (1) {

        LPSERIALIZED_LIST viewList;
        LPVIEW_INFO viewInfo;
        HINTERNET handle;

        AcquireViewLock(SessionInfo, ViewType);

        viewList = &SessionInfo->FindList;
        for (viewInfo = (LPVIEW_INFO)HeadOfSerializedList(viewList);
            (viewInfo != (LPVIEW_INFO)&viewList->List.Flink)
            && !(viewInfo->Flags & VI_CLEANUP);
            viewInfo = (LPVIEW_INFO)viewInfo->List.Flink) {

            //
            // empty loop
            //

        }

        if (viewInfo == (LPVIEW_INFO)&viewList->List.Flink) {

            DEBUG_PRINT(SESSION,
                        INFO,
                        ("end of list\n"
                        ));

            ReleaseViewLock(SessionInfo, ViewType);
            break;
        }

        //
        // mark this VIEW_INFO as being cleaned-up, so we don't hit it again if
        // we don't delete it from the list this time
        //

        viewInfo->Flags |= VI_CLEANUP;

        //
        // safe to release the view lock
        //

        ReleaseViewLock(SessionInfo, ViewType);

        //
        // now dereference the VIEW_INFO. This may destroy it, but cannot
        // destroy the SESSION_INFO, since we added an extra reference in
        // CleanupSessions()
        //

        DereferenceView(viewInfo);
    }

    DEBUG_LEAVE(0);
}


LPSESSION_INFO
FindOrCreateSession(
    IN LPSTR Host,
    IN DWORD Port,
    OUT LPDWORD Error
    )

/*++

Routine Description:

    Locates a SESSION_INFO that contains (Host, Port), or creates a new
    SESSION_INFO

    BUGBUG - need to do the following:

        resolve the host name
        find host by name/port or address/port

Arguments:

    Host    - pointer to host name where gopher server lives

    Port    - port at which gopher server listens

    Error   - place to return error

Return Value:

    LPSESSION_INFO
        Success - pointer to session info. Created contains TRUE if we created
                  the SESSION_INFO, else FALSE

        Failure - NULL
                    Error contains reason for failure

--*/

{
    LPSESSION_INFO sessionInfo;
    BOOL found;

    AcquireSessionLock();

    found = FALSE;
    for (sessionInfo = (LPSESSION_INFO)SessionList.List.Flink;
        sessionInfo != (LPSESSION_INFO)&SessionList.List;
        sessionInfo = (LPSESSION_INFO)(sessionInfo->List.Flink)) {

        if (!stricmp(sessionInfo->Host, Host) && (sessionInfo->Port == Port)) {
            found = TRUE;
            break;
        }
    }
    if (!found) {
        sessionInfo = CreateSession(Host, Port, Error);
        if (sessionInfo != NULL) {
            InsertAtHeadOfSerializedList(&SessionList, &sessionInfo->List);
        }
    }
    if (sessionInfo != NULL) {

        //
        // the reference count will be at least 2
        //

        ReferenceSession(sessionInfo);
    }

    ReleaseSessionLock();

    return sessionInfo;
}


PRIVATE
LPSESSION_INFO
CreateSession(
    IN LPSTR Host,
    IN DWORD Port,
    OUT LPDWORD Error
    )

/*++

Routine Description:

    Creates and initializes a SESSION_INFO 'object'

Arguments:

    Host    - pointer to host name/ip address

    Port    - host port

    Error   - place to return reason for failure

Return Value:

    LPSESSION_INFO
        Success - pointer to initialized session info

        Failure - NULL
                    Error contains reason for failure

--*/

{
    LPSESSION_INFO sessionInfo;
    LPSTR hostName = NULL;
    DWORD error;

    DEBUG_ENTER((DBG_GOPHER,
                Pointer,
                "CreateSession",
                "%q, %d, %x",
                Host,
                Port,
                Error
                ));

    sessionInfo = NEW(SESSION_INFO);
    if (sessionInfo != NULL) {
        hostName = NEW_STRING(Host);
        if (hostName != NULL) {
            error = AllocateHandle((LPVOID)sessionInfo, &sessionInfo->Handle);
            if (error == ERROR_SUCCESS) {

                InitializeListHead(&sessionInfo->List);
                sessionInfo->Host = hostName;
                sessionInfo->Port = Port;
                InitializeSerializedList(&sessionInfo->FindList);
                InitializeSerializedList(&sessionInfo->FileList);

                SESSION_CREATED();

            }
        } else {
            error = ERROR_NOT_ENOUGH_MEMORY;
        }
    } else {
        error = ERROR_NOT_ENOUGH_MEMORY;
    }

    if (error != ERROR_SUCCESS) {
        if (hostName != NULL) {
            DEL_STRING(hostName);
        }
        if (sessionInfo != NULL) {
            DEL(sessionInfo);
        }
        sessionInfo = NULL;
    }

    DEBUG_ERROR(SESSION, error);

    *Error = error;

    DEBUG_LEAVE(sessionInfo);

    return sessionInfo;
}


PRIVATE
VOID
DestroySession(
    IN LPSESSION_INFO SessionInfo
    )

/*++

Routine Description:

    Opposite of CreateSession - removes a SESSION_INFO from SessionList and
    frees all resources owned by the SESSION_INFO and finally frees the memory

    Assumes:    1. SessionListLock is held
                2. SessionInfo is not on any lists
                3. The SERIALIZED_LISTs have already been created

Arguments:

    SessionInfo - pointer to SESSION_INFO to delete

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_GOPHER,
                None,
                "DestroySession",
                "%x",
                SessionInfo
                ));

    INET_DEBUG_ASSERT(SessionInfo->List.Flink == NULL);
    INET_DEBUG_ASSERT(SessionInfo->List.Blink == NULL);
    INET_ASSERT(SessionInfo->ReferenceCount == 0);
    INET_ASSERT(IsSerializedListEmpty(&SessionInfo->FindList));
    INET_DEBUG_ASSERT(!IsLockHeld(&SessionInfo->FindList));
    INET_ASSERT(IsSerializedListEmpty(&SessionInfo->FileList));
    INET_DEBUG_ASSERT(!IsLockHeld(&SessionInfo->FileList));

    if (SessionInfo->Handle) {
        FreeHandle(SessionInfo->Handle);
    }

    if (SessionInfo->Host != NULL) {
        DEL(SessionInfo->Host);
    }

    TerminateSerializedList(&SessionInfo->FindList);
    TerminateSerializedList(&SessionInfo->FileList);

    DEL(SessionInfo);

    SESSION_DESTROYED();

    DEBUG_LEAVE(0);
}


VOID
ReferenceSession(
    IN LPSESSION_INFO SessionInfo
    )

/*++

Routine Description:

    Increases the reference count of a SESSION_INFO

Arguments:

    Session - pointer to SESSION_INFO to reference

Return Value:

    None.

--*/

{
    INET_ASSERT(SessionInfo != NULL);

    InterlockedIncrement(&SessionInfo->ReferenceCount);
}


LPSESSION_INFO
DereferenceSession(
    IN LPSESSION_INFO SessionInfo
    )

/*++

Routine Description:

    Reduces the reference count of a SESSION_INFO. If it goes to zero, the
    SESSION_INFO is removed from the SessionList and is deallocated

Arguments:

    SessionInfo - pointer to SESSION_INFO to dereference

Return Value:

    LPSESSION_INFO
        NULL    - SessionInfo was deleted

        !NULL   - Reference count still >0

--*/

{
    INET_ASSERT(SessionInfo);
    INET_ASSERT(SessionInfo->ReferenceCount >= 1);

    //
    // use InterlockedDecrement to dereference the session. If it goes to zero
    // acquire the session lock, check if the reference count is still zero,
    // and if so, remove the session from the session list
    //

    if (InterlockedDecrement(&SessionInfo->ReferenceCount) == 0) {
        AcquireSessionLock();
        if (SessionInfo->ReferenceCount == 0) {
            RemoveFromSerializedList(&SessionList, (PLIST_ENTRY)SessionInfo);
            DestroySession(SessionInfo);
            SessionInfo = NULL;
        }
        ReleaseSessionLock();
    }
    return SessionInfo;
}


VOID
AcquireViewLock(
    IN LPSESSION_INFO SessionInfo,
    IN VIEW_TYPE ViewType
    )

/*++

Routine Description:

    Acquires one of the SessionInfo View locks

Arguments:

    SessionInfo - pointer to SESSION_INFO

    ViewType    - identifies which list to lock

Return Value:

    None.

--*/

{
    LPSERIALIZED_LIST list;

    INET_ASSERT(SessionInfo != NULL);
    INET_ASSERT((ViewType == ViewTypeFile) || (ViewType == ViewTypeFind));

    list = (ViewType == ViewTypeFile)
        ? &SessionInfo->FileList
        : &SessionInfo->FindList
        ;
    LockSerializedList(list);
}


VOID
ReleaseViewLock(
    IN LPSESSION_INFO SessionInfo,
    IN VIEW_TYPE ViewType
    )

/*++

Routine Description:

    Releases the SessionInfo View lock

Arguments:

    SessionInfo - pointer to SESSION_INFO

    ViewType    - identifies which list to lock

Return Value:

    None.

--*/

{
    LPSERIALIZED_LIST list;

    INET_ASSERT(SessionInfo != NULL);
    INET_ASSERT((ViewType == ViewTypeFile) || (ViewType == ViewTypeFind));

    list = (ViewType == ViewTypeFile)
        ? &SessionInfo->FileList
        : &SessionInfo->FindList
        ;
    UnlockSerializedList(list);
}


DWORD
GopherTransaction(
    IN LPVIEW_INFO ViewInfo
    )

/*++

Routine Description:

    Performs an 'atomic' gopher operation. Connects to a server (if it isn't
    already connected (in the future?)), sends a request and receives the
    entire response message. The connection is terminated

Arguments:

    ViewInfo    - pointer to VIEW_INFO describing gopher server to talk to,
                  request and buffer for response

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - Winsock error

--*/

{
    DWORD error;

    INET_ASSERT(ViewInfo != NULL);

    error = GopherConnect(ViewInfo);
    if (error == ERROR_SUCCESS) {
        error = GopherSendRequest(ViewInfo);
        if (error == ERROR_SUCCESS) {

            DWORD bytesReceived;

            //
            // receive the first part of the response. We don't care about the
            // number of bytes received at this point
            //
            // If the response is completed and the connection is not persistent
            // or an error occurs, the connection will be closed
            //

            ViewInfo->BufferInfo->Flags |= BI_FIRST_RECEIVE;
            error = GopherReceiveResponse(ViewInfo, &bytesReceived);
        }
    }
    return error;
}


DWORD
IsServerGopherPlus(
    IN LPSESSION_INFO SessionInfo,
    OUT LPBOOL Answer
    )

/*++

Routine Description:

    Tries to determine whether a gopher server identified by Session is gopher+.
    The caller should already have determined that we don't know the type of
    gopher server described by Session and should modify the flags based on a
    successful return from this function

Arguments:

    SessionInfo - pointer to SESSION_INFO describing the (unknown) gopher server

    Answer      - pointer to place to put the answer

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - WSA error

--*/

{
/*
    DWORD error;
    BYTE buffer[GOPHER_PLUS_INFO_TOKEN_LENGTH + 2]; // "+INFO"
                                                    // + 1 for possible ':'
                                                    // + 1 for ' '
    BOOL receiveComplete;

    //
    // in order to find out the type of gopher server, we send a request for
    // gopher+ info of the root directory. We will get back either the gopher+
    // information or a gopher0 server will return the directory list (or we
    // will get an error). Therefore, if the transaction doesn't result in an
    // error, we can say that if the buffer starts with "+INFO" then the
    // server is gopher+ else plain gopher
    //

    error = GopherTransaction(SessionInfo,
                              GOPHER_PLUS_INFO_REQUEST,
                              TRUE,
                              sizeof(buffer),
                              buffer,
                              NULL,
                              &receiveComplete
                              );

    //
    // in both gopher+ and gopher server cases, the server should want to
    // return more data than we've supplied buffer for (7 bytes!). In the
    // case of gopher+, it will be trying to return the +INFO block for
    // the directory entry. In the case of gopher, it will be trying to
    // return the entire default directory list. Either way, we don't care
    // to take the data: "+INFO[:] " being present or not at the start of
    // the buffer is good enough for us, so just close the session and
    // examine what we have
    //

    DisconnectFromServer(SessionInfo, );
    if (error == ERROR_SUCCESS) {

        register DWORD matchLength;

        matchLength = IsGopherPlusToken(GOPHER_PLUS_INFO_TOKEN,
                                        GOPHER_PLUS_INFO_TOKEN_LENGTH,
                                        buffer,
                                        sizeof(buffer)
                                        );
        *Answer = (BOOL)(matchLength != 0);

        IF_DEBUG(SESSION) {
            DBGPRINT(DBG_INFO,
                     "IsServerGopherPlus",
                     ("Server \"%s\" %s gopher+\n",
                     SessionInfo->Host,
                     (matchLength == 0) ? "NOT" : "IS"
                     ));
        }
    } else {
        IF_DEBUG(SESSION) {
            DBGPRINT(DBG_ERROR,
                     "IsServerGopherPlus",
                     ("GopherTransaction() returns %d\n",
                     error
                     ));
        }
    }
    return error;
*/
    *Answer = FALSE;
    return ERROR_SUCCESS;
}


BOOL
IsGopherPlusSession(
    IN LPSESSION_INFO SessionInfo
    )

/*++

Routine Description:

    Returns TRUE if Session is a session with a gopher+ server

Arguments:

    SessionInfo - pointer to SESSION_INFO describing gopher[+] server

Return Value:

    BOOL

--*/

{
    return (BOOL)SessionInfo->Flags & SI_GOPHER_PLUS;
}


DWORD
SearchSessionsForAttribute(
    IN LPSTR Locator,
    IN LPSTR Attribute,
    IN LPBYTE Buffer,
    IN OUT LPDWORD BufferLength
    )

/*++

Routine Description:

    Searches all VIEW_INFO buffers for a requested Locator and extracts the
    attributes if found

Arguments:

    Locator         - pointer to locator describing item to get attributes for

    Attribute       - pointer to string describing attribute(s) to get

    Buffer          - pointer to buffer in which to return attribute strings

    BufferLength    - IN: length of Buffer in bytes
                      OUT: length of returned attribute strings in bytes,
                           excluding any terminating NUL

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_GOPHER_ATTRIBUTE_NOT_FOUND
                  ERROR_INSUFFICIENT_BUFFER

--*/

{
    LPSESSION_INFO session;
    BOOL found = FALSE;
    DWORD error = ERROR_SUCCESS;

    AcquireSessionLock();
/*
    for (session = (LPSESSION_INFO)SessionList.Flink;
        session != (LPSESSION_INFO)&SessionList.Flink;
        session = (LPSESSION_INFO)session->List.Flink) {

        LPVIEW_INFO viewInfo;

        AcquireFindLock(session);
        for (findInfo = (LPVIEW_INFO)session->FindList.Flink;
            findInfo != (LPVIEW_INFO)&session->FindList;
            findInfo = (LPVIEW_INFO)findInfo->List.Flink) {

            ReferenceFind(findInfo);
            if (findInfo->Handle) {
                found = TRUE;
                break;  // out of for()
            }
        }
        if (found) {
            break;  // out of while()
        }
        ReleaseFindLock(session);
    }
    if (found) {
        error = ERROR_SUCCESS;
    } else {
        error = ERROR_GOPHER_ATTRIBUTE_NOT_FOUND;
    }
*/
    ReleaseSessionLock();

    error = ERROR_GOPHER_ATTRIBUTE_NOT_FOUND;

    return error;
}
