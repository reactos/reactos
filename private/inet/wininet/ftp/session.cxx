/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    session.cxx

Abstract:

    Contains functions to create and delete FTP_SESSION_INFOs

    Contents:
        FtpSessionInitialize
        FtpSessionTerminate
        AcquireFtpSessionList
        ReleaseFtpSessionList
        AcquireFtpSessionLock
        ReleaseFtpSessionLock
        CleanupFtpSessions
        TerminateFtpSession
        DereferenceFtpSession
        CreateFtpSession
        (DestroyFtpSession)
        FindFtpSession

Author:

    Richard L Firth (rfirth) 09-Jun-95

Environment:

    Win32 user-level DLL

Revision History:

    09-Jun-95 rfirth
        Created

--*/

#include <wininetp.h>
#include "ftpapih.h"

//
// private prototypes
//

PRIVATE
VOID
DestroyFtpSession(
    IN LPFTP_SESSION_INFO SessionInfo
    );

//
// public data
//

//
// FtpSessionList - a doubly-linked list of all the FTP_SESSION_INFOs owned by
// this process
//

PUBLIC SERIALIZED_LIST FtpSessionList;

//
// external data
//

extern BOOL InDllCleanup;

//
// functions
//

#if INET_DEBUG


VOID
FtpSessionInitialize(
    VOID
    )

/*++

Routine Description:

    Initializes any global data items for this module.

Arguments:

    None.

Return Value:

    None.

--*/

{
    InitializeSerializedList(&FtpSessionList);
}


VOID
FtpSessionTerminate(
    VOID
    )

/*++

Routine Description:

    Terminates any items initialized by FtpSessionInitialize()

Arguments:

    None.

Return Value:

    None.

--*/

{
    TerminateSerializedList(&FtpSessionList);
}


VOID
AcquireFtpSessionList(
    VOID
    )

/*++

Routine Description:

    Acquires the FtpSessionList lock

Arguments:

    None.

Return Value:

    None.

--*/

{
    LockSerializedList(&FtpSessionList);
}


VOID
ReleaseFtpSessionList(
    VOID
    )

/*++

Routine Description:

    Releases the FtpSessionList lock

Arguments:

    None.

Return Value:

    None.

--*/

{
    UnlockSerializedList(&FtpSessionList);
}


VOID
AcquireFtpSessionLock(
    IN LPFTP_SESSION_INFO SessionInfo
    )

/*++

Routine Description:

    Acquires an individual session info lock

Arguments:

    SessionInfo - pointer to FTP_SESSION_INFO to lock

Return Value:

    None.

--*/

{
    EnterCriticalSection(&SessionInfo->CriticalSection);
}


VOID
ReleaseFtpSessionLock(
    IN LPFTP_SESSION_INFO SessionInfo
    )

/*++

Routine Description:

    Releases an individual session info lock

Arguments:

    SessionInfo - pointer to FTP_SESSION_INFO to unlock

Return Value:

    None.

--*/

{
    LeaveCriticalSection(&SessionInfo->CriticalSection);
}

#endif // INET_DEBUG


VOID
CleanupFtpSessions(
    VOID
    )

/*++

Routine Description:

    Terminates all active FTP sessions owned by this process

Arguments:

    None.

Return Value:

    None.

--*/

{
    LPFTP_SESSION_INFO info;

    DEBUG_ENTER((DBG_FTP,
                None,
                "CleanupFtpSessions",
                NULL
                ));

    //
    // walk the session list. For each FTP_SESSION_INFO, close the data and
    // control sockets, if open, then destroy the session
    //

    do {

        DWORD error;

        AcquireFtpSessionList();

        info = (LPFTP_SESSION_INFO)HeadOfSerializedList(&FtpSessionList);
        if (info == (LPFTP_SESSION_INFO)&FtpSessionList.List.Flink) {
            ReleaseFtpSessionList();
            break;
        }

        //
        // kill the data and control sockets. Remorselessly. If the sockets are
        // set to INVALID_SOCKET, then Close() will do the right thing
        //

        error = info->socketData->Close();
        if (error != ERROR_SUCCESS) {

            DEBUG_PRINT(SESSION,
                        ERROR,
                        ("Data: Close(%x) returns %d\n",
                        info->socketData,
                        error
                        ));

        }

        error = info->socketControl->Close();
        if (error != ERROR_SUCCESS) {

            DEBUG_PRINT(SESSION,
                        ERROR,
                        ("Control: Close(%x) returns %d\n",
                        info->socketControl,
                        error
                        ));

        }

        ReleaseFtpSessionList();
        DestroyFtpSession(info);
    } while (1);

    DEBUG_LEAVE(0);
}


VOID
TerminateFtpSession(
    IN LPFTP_SESSION_INFO SessionInfo
    )

/*++

Routine Description:

    Initiates termination of the FTP_SESSION_INFO object

Arguments:

    SessionInfo - pointer to FTP_SESSION_INFO to terminate

Return Value:

    None.

--*/

{
    BOOL destroy;
    int i;

    INET_ASSERT(SessionInfo != NULL);
    INET_ASSERT(!(SessionInfo->Flags & FFTP_IN_DESTRUCTOR));
    INET_ASSERT(SessionInfo->ReferenceCount >= 2);

    DEBUG_ENTER((DBG_FTP,
                None,
                "TerminateFtpSession",
                "%x",
                SessionInfo
                ));

    //
    // first off, set the in-destructor flag. This will cause any other threads
    // trying to find & reference this object to fail
    //

    SessionInfo->Flags |= FFTP_IN_DESTRUCTOR;

    //
    // now we need to decrement the reference count by 2, which should cause
    // the FTP_SESSION_INFO to be destroyed
    //

    for (i = 0, destroy = FALSE; i < 2; ++i) {
        if (InterlockedDecrement(&SessionInfo->ReferenceCount) == 0) {
            destroy = TRUE;
            break;
        }
    }

    if (destroy) {

        //
        // we think the reference count has gone to zero. Acquire the session
        // lock and check again. If still zero then we can delete this object
        //

        AcquireFtpSessionLock(SessionInfo);
        if (SessionInfo->ReferenceCount == 0) {
            DestroyFtpSession(SessionInfo);
        } else {

            DEBUG_PRINT(REFCOUNT,
                        WARNING,
                        ("unexpected: SessionInfo(%x)->ReferenceCount = %d\n",
                        SessionInfo,
                        SessionInfo->ReferenceCount
                        ));

            ReleaseFtpSessionLock(SessionInfo);
        }
    } else {

        DEBUG_PRINT(REFCOUNT,
                    WARNING,
                    ("unexpected: SessionInfo(%x)->ReferenceCount = %d\n",
                    SessionInfo,
                    SessionInfo->ReferenceCount
                    ));

    }

    DEBUG_LEAVE(0);
}


VOID
DereferenceFtpSession(
    IN LPFTP_SESSION_INFO SessionInfo
    )

/*++

Routine Description:

    Reduces the reference count on an FTP_SESSION_INFO object by 1. If the
    reference count goes to 0, the FTP_SESSION_INFO is destroyed

Arguments:

    SessionInfo - pointer to FTP_SESSION_INFO to dereference

Return Value:

    None.

--*/

{
    INET_ASSERT(SessionInfo->ReferenceCount >= 1);

    DEBUG_ENTER((DBG_FTP,
                None,
                "DereferenceFtpSession",
                "%x",
                SessionInfo
                ));

    if (InterlockedDecrement(&SessionInfo->ReferenceCount) == 0) {

        //
        // we think the reference count has gone to zero. Acquire the session
        // lock and check again. If still zero then we can delete this object
        //

        AcquireFtpSessionLock(SessionInfo);
        if (SessionInfo->ReferenceCount == 0) {
            DestroyFtpSession(SessionInfo);
        } else {

            DEBUG_PRINT(REFCOUNT,
                        INFO,
                        ("SessionInfo(%x)->ReferenceCount = %d\n",
                        SessionInfo,
                        SessionInfo->ReferenceCount
                        ));

            ReleaseFtpSessionLock(SessionInfo);
        }
    } else {

        DEBUG_PRINT(REFCOUNT,
                    INFO,
                    ("SessionInfo(%x)->ReferenceCount = %d\n",
                    SessionInfo,
                    SessionInfo->ReferenceCount
                    ));

    }

    DEBUG_LEAVE(0);
}


DWORD
CreateFtpSession(
    IN LPSTR lpszHost,
    IN INTERNET_PORT Port,
    IN DWORD dwFlags,
    OUT LPFTP_SESSION_INFO* lpSessionInfo
    )

/*++

Routine Description:

    Creates a new FTP_SESSION_INFO object. If successful, adds the new object
    to the FtpSessionList; the reference count of the new object will be 1 (i.e.
    it is unowned)

Arguments:

    lpszHost        - pointer to name of the host we are connecting to. Mainly
                      for diagnostic purposes (can remove it later)

    Port            - Port at which the FTP server listens. Only in anomalous
                      circumstances will this not be 21

    dwFlags         - flags controlling session creation. Can be:
                        - FFTP_PASSIVE_MODE

    lpSessionInfo   - pointer to returned session info

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY
                    Couldn't allocate memory

                  ERROR_INTERNET_OUT_OF_HANDLES
                    Couldn't allocate a handle

                  ERROR_INTERNET_SHUTDOWN
                    The DLL is being unloaded

--*/

{
    LPFTP_SESSION_INFO sessionInfo;
    DWORD error;

    DEBUG_ENTER((DBG_FTP,
                Dword,
                "CreateFtpSession",
                "%q, %d, %x",
                lpszHost,
                Port,
                lpSessionInfo
                ));

    //
    // if the DLL is being unloaded then return the shutdown error
    //

    if (InDllCleanup) {
        error = ERROR_INTERNET_SHUTDOWN;
        goto quit;
    }

    //
    // allocate the FTP_SESSION_INFO 'object' and its various sub-assemblies
    //

    sessionInfo = NEW(FTP_SESSION_INFO);
    if (sessionInfo != NULL) {
        sessionInfo->Host = NEW_STRING(lpszHost);
        if (sessionInfo->Host != NULL) {
            error = AllocateHandle((LPVOID)sessionInfo, &sessionInfo->Handle);
            if (error == ERROR_SUCCESS) {

                INET_ASSERT(sessionInfo->ServerType == FTP_SERVER_TYPE_UNKNOWN);

                //
                // initialize any non-zero fields
                //

                InitializeListHead(&sessionInfo->List);
                sessionInfo->Port = Port;
                sessionInfo->Flags = dwFlags;
                sessionInfo->ReferenceCount = 1;
                InitializeListHead(&sessionInfo->FindFileList);
                InitializeCriticalSection(&sessionInfo->CriticalSection);
                SetSessionSignature(sessionInfo);

                //
                // Allocate Our Socket Objects.
                //

                //
                // Isn't this kinda of messy?  I've copied the format of
                // surrounding block, since there is no clean quit case in this
                // function... arhh.
                //

                sessionInfo->socketControl = new ICSocket();
                if ( sessionInfo->socketControl )
                {
                    sessionInfo->socketData = new ICSocket();
                    if ( sessionInfo->socketData )
                    {
                        sessionInfo->socketListener = new ICSocket();
                        if ( sessionInfo->socketListener  == NULL )
                        {
                            sessionInfo->socketControl->Dereference();
                            sessionInfo->socketData->Dereference();
                            error = ERROR_NOT_ENOUGH_MEMORY;
                        }
                    }
                    else
                    {
                        sessionInfo->socketControl->Dereference();
                        error = ERROR_NOT_ENOUGH_MEMORY;
                    }

                }
                else
                {
                    error = ERROR_NOT_ENOUGH_MEMORY;
                }
            }
        } else {
            error = ERROR_NOT_ENOUGH_MEMORY;
        }
    } else {
        error = ERROR_NOT_ENOUGH_MEMORY;
    }
    if (error == ERROR_SUCCESS) {

        //
        // add this FTP_SESSION_INFO to the object list
        //

        InsertAtHeadOfSerializedList(&FtpSessionList, &sessionInfo->List);
        *lpSessionInfo = sessionInfo;

        DEBUG_PRINT(SESSION,
                    INFO,
                    ("handle = %x, SessionInfo = %x\n",
                    sessionInfo->Handle,
                    sessionInfo
                    ));

    } else if (sessionInfo != NULL) {
        if (sessionInfo->Host != NULL) {
            DEL_STRING(sessionInfo->Host);
        }
        DEL(sessionInfo);
    }

quit:

    DEBUG_LEAVE(error);

    return error;
}


PRIVATE
VOID
DestroyFtpSession(
    IN LPFTP_SESSION_INFO SessionInfo
    )

/*++

Routine Description:

    Removes an FTP_SESSION_INFO object from the session list and destroys it.

    Note: SessionInfo MUST be owned by the current thread (critical section held
    but reference count == 0)

Arguments:

    SessionInfo - pointer to FTP_SESSION_INFO to delete

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_FTP,
                None,
                "DestroyFtpSession",
                "%x",
                SessionInfo
                ));

    INET_DEBUG_ASSERT(SessionInfo->Signature == FTP_SESSION_SIGNATURE);
    INET_ASSERT(!SessionInfo->socketControl->IsValid());
    INET_ASSERT(!SessionInfo->socketData->IsValid());
    INET_ASSERT(SessionInfo->ReferenceCount == 0);

    RemoveFromSerializedList(&FtpSessionList, (PLIST_ENTRY)&SessionInfo->List);

    INET_DEBUG_ASSERT(SessionInfo->List.Flink == NULL);
    INET_DEBUG_ASSERT(SessionInfo->List.Blink == NULL);

    ReleaseFtpSessionLock(SessionInfo);

    if (SessionInfo->Handle) {
        FreeHandle(SessionInfo->Handle);
    }

    if (SessionInfo->Host != NULL) {
        DEL_STRING(SessionInfo->Host);
    }

    if ( SessionInfo->socketControl )
        SessionInfo->socketControl->Dereference();

    if ( SessionInfo->socketData )
        SessionInfo->socketData->Dereference();

    if ( SessionInfo->socketListener )
        SessionInfo->socketListener->Dereference();

    ClearFindList(&SessionInfo->FindFileList);

    DeleteCriticalSection(&SessionInfo->CriticalSection);

    DEL(SessionInfo);

    DEBUG_LEAVE(0);
}


BOOL
FindFtpSession(
    IN HANDLE Handle,
    OUT LPFTP_SESSION_INFO *lpSessionInfo
    )

/*++

Routine Description:

    Searches the session list for an FTP_SESSION_INFO object. If found, the
    FTP_SESSION_INFO's reference count is incremented by 1

Arguments:

    Handle          - handle to search for

    lpSessionInfo   - pointer to returned FTP_SESSION_INFO if found

Return Value:

    BOOL
        Success - TRUE, *lppSessionInfo is pointer to FTP_SESSION_INFO

        Failure - FALSE

--*/

{
    BOOL found;
    LPFTP_SESSION_INFO sessionInfo;

    DEBUG_ENTER((DBG_FTP,
                Bool,
                "FindFtpSession",
                "%x",
                Handle
                ));

    //
    // lock the list, to prevent the element from moving under us
    //

    AcquireFtpSessionList();

    found = FALSE;
    for (sessionInfo = (LPFTP_SESSION_INFO)FtpSessionList.List.Flink;
        sessionInfo != (LPFTP_SESSION_INFO)&FtpSessionList.List;
        sessionInfo = (LPFTP_SESSION_INFO)(sessionInfo->List.Flink)) {

        if (sessionInfo->Handle == Handle) {
            AcquireFtpSessionLock(sessionInfo);

            //
            // if the destructor flag is set then another thread is already
            // destroying this session - we cannot return it. Treat as not
            // found
            //

            if (!(sessionInfo->Flags & FFTP_IN_DESTRUCTOR)) {

                //
                // although we're holding the session info lock (a critical
                // section), we still need to perform interlocked increment
                // on the reference count
                //

                InterlockedIncrement(&sessionInfo->ReferenceCount);
                found = TRUE;
            }
            ReleaseFtpSessionLock(sessionInfo);
            break;
        }
    }

    ReleaseFtpSessionList();

    if (found) {
        *lpSessionInfo = sessionInfo;

        DEBUG_PRINT(SESSION,
                    INFO,
                    ("handle = %x, FTP_SESSION_INFO = %x\n",
                    Handle,
                    sessionInfo
                    ));

    } else {

        DEBUG_PRINT(SESSION,
                    ERROR,
                    ("handle %x: not found\n",
                    Handle
                    ));

    }

    DEBUG_LEAVE(found);

    return found;
}
