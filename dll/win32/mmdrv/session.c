/*
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS Multimedia
 * FILE:                 dll/win32/mmdrv/session.c
 * PURPOSE:              Multimedia User Mode Driver (session management)
 * PROGRAMMER:           Andrew Greenwood
 * UPDATE HISTORY:
 *                       Jan 14, 2007: Created
 */

#include <mmdrv.h>

/* Each session is tracked, but the list must be locked when in use  */

SessionInfo* session_list = NULL;
CRITICAL_SECTION session_lock;


/*
    Obtains a pointer to the session associated with a device type and ID.
    If no session exists, returns NULL. This is mainly used to see if a
    session already exists prior to creating a new one.
*/

SessionInfo*
GetSession(
    DeviceType device_type,
    DWORD device_id)
{
    SessionInfo* session_info;

    EnterCriticalSection(&session_lock);
    session_info = session_list;

    while ( session_info )
    {
        if ( ( session_info->device_type == device_type ) &&
             ( session_info->device_id == device_id ) )
        {
            LeaveCriticalSection(&session_lock);
            return session_info;
        }

        session_info = session_info->next;
    }

    LeaveCriticalSection(&session_lock);
    return NULL;
}


/*
    Creates a new session, associated with the specified device type and ID.
    Whilst the session list is locked, this also checks to see if an existing
    session is associated with the device.
*/

MMRESULT
CreateSession(
    DeviceType device_type,
    DWORD device_id,
    SessionInfo** session_info)
{
    HANDLE heap = GetProcessHeap();

    ASSERT(session_info);

    EnterCriticalSection(&session_lock);

    /* Ensure we're not creating a duplicate session */

    if ( GetSession(device_type, device_id) )
    {
        DPRINT("Already allocated session\n");
        LeaveCriticalSection(&session_lock);
        return MMSYSERR_ALLOCATED;
    }

    *session_info = HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(SessionInfo));

    if ( ! *session_info )
    {
        DPRINT("Failed to allocate mem for session info\n");
        LeaveCriticalSection(&session_lock);
        return MMSYSERR_NOMEM;
    }

    (*session_info)->device_type = device_type;
    (*session_info)->device_id = device_id;

    /* Add to the list */

    (*session_info)->next = session_list;
    session_list = *session_info;

    LeaveCriticalSection(&session_lock);

    return MMSYSERR_NOERROR;
}


/*
    Removes a session from the list and destroys it. This function does NOT
    perform any additional cleanup. Think of it as a slightly more advanced
    free()
*/

VOID
DestroySession(SessionInfo* session)
{
    HANDLE heap = GetProcessHeap();
    SessionInfo* session_node;
    SessionInfo* session_prev;

    /* TODO: More cleanup stuff */

    /* Remove from the list */

    EnterCriticalSection(&session_lock);

    session_node = session_list;
    session_prev = NULL;

    while ( session_node )
    {
        if ( session_node == session )
        {
            /* Bridge the gap for when we go */
            session_prev->next = session->next;
            break;
        }

        /* Save the previous node, fetch the next */
        session_prev = session_node;
        session_node = session_node->next;
    }

    LeaveCriticalSection(&session_lock);

    HeapFree(heap, 0, session);
}


/*
    Allocates events and other resources for the session thread, starts it,
    and waits for it to announce that it is ready to work for us.
*/

MMRESULT
StartSessionThread(SessionInfo* session_info)
{
    LPTASKCALLBACK task;
    MMRESULT result;

    ASSERT(session_info);

    /* This is our "ready" event, sent when the thread is idle */

    session_info->thread.ready_event = CreateEvent(NULL, FALSE, FALSE, NULL);

    if ( ! session_info->thread.ready_event )
    {
        DPRINT("Couldn't create thread_ready event\n");
        return MMSYSERR_NOMEM;
    }

    /* This is our "go" event, sent when we want the thread to do something */

    session_info->thread.go_event = CreateEvent(NULL, FALSE, FALSE, NULL);

    if ( ! session_info->thread.go_event )
    {
        DPRINT("Couldn't create thread_go event\n");
        CloseHandle(session_info->thread.ready_event);
        return MMSYSERR_NOMEM;
    }

    /* TODO - other kinds of devices need attention, too */
    task = ( session_info->device_type == WaveOutDevice )
           ? (LPTASKCALLBACK) WaveThread : NULL;

    ASSERT(task);

    /* Effectively, this is a beefed-up CreateThread */

    result = mmTaskCreate(task,
                          &session_info->thread.handle,
                          (DWORD) session_info);

    if ( result != MMSYSERR_NOERROR )
    {
        DPRINT("Task creation failed\n");
        CloseHandle(session_info->thread.ready_event);
        CloseHandle(session_info->thread.go_event);
        return result;
    }

    /* Wait for the thread to be ready before completing */

    WaitForSingleObject(session_info->thread.ready_event, INFINITE);

    return MMSYSERR_NOERROR;
}


/*
    The session thread is pretty simple. Upon creation, it announces that it
    is ready to do stuff for us. When we want it to perform an action, we use
    CallSessionThread with an appropriate function and parameter, then tell
    the thread we want it to do something. When it's finished, it announces
    that it is ready once again.
*/

MMRESULT
CallSessionThread(
    SessionInfo* session_info,
    ThreadFunction function,
    PVOID thread_parameter)
{
    ASSERT(session_info);

    session_info->thread.function = function;
    session_info->thread.parameter = thread_parameter;

    DPRINT("Calling session thread\n");
    SetEvent(session_info->thread.go_event);

    DPRINT("Waiting for thread response\n");
    WaitForSingleObject(session_info->thread.ready_event, INFINITE);

    return session_info->thread.result;
}


DWORD
HandleBySessionThread(
    DWORD private_handle,
    DWORD message,
    DWORD parameter)
{
    return CallSessionThread((SessionInfo*) private_handle,
                             message,
                             (PVOID) parameter);
}
