
#ifndef UNICODE
	#define UNICODE
#endif


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <stdio.h>
#include "logging.h"
extern HRESULT PAWaitForSamService();


HRESULT
PAWaitForSamService()
/*++

Routine Description:

    This procedure waits for the SAM service to start and to complete
    all its initialization.

Arguments:


Return Value:


--*/

{
    NTSTATUS Status;
    DWORD WaitStatus;
    UNICODE_STRING EventName;
    HANDLE EventHandle;
    OBJECT_ATTRIBUTES EventAttributes;
	int waits=0;

    //
    // open SAM event
    //

    RtlInitUnicodeString( &EventName, L"\\SAM_SERVICE_STARTED");
    InitializeObjectAttributes( &EventAttributes, &EventName, 0, 0, NULL );

    Status = NtOpenEvent( &EventHandle,
                            SYNCHRONIZE|EVENT_MODIFY_STATE,
                            &EventAttributes );
    if ( !NT_SUCCESS(Status))
	{

        if( Status == STATUS_OBJECT_NAME_NOT_FOUND )
		{
            //
            // SAM hasn't created this event yet, let us create it now.
            // SAM opens this event to set it.
            //

            Status = NtCreateEvent(
                           &EventHandle,
                           SYNCHRONIZE|EVENT_MODIFY_STATE,
                           &EventAttributes,
                           NotificationEvent,
                           FALSE // The event is initially not signaled
                           );

            if( Status == STATUS_OBJECT_NAME_EXISTS ||
                Status == STATUS_OBJECT_NAME_COLLISION )
			{

                //
                // second change, if the SAM created the event before we
                // do.
                //

                Status = NtOpenEvent( &EventHandle,
                                        SYNCHRONIZE|EVENT_MODIFY_STATE,
                                        &EventAttributes );

            }
        }

        if ( !NT_SUCCESS(Status))
		{
            //
            // could not make the event handle
            //
            return( Status );
        }
    }

    //
    // Loop waiting.
    //
    for (;;)
	{
        WaitStatus = WaitForSingleObject( EventHandle,
                                          5*1000 );  // 5 Seconds

        if ( WaitStatus == WAIT_TIMEOUT )
		{
			waits++;
			if (waits<4)
			{
				// TODO: should keep SCM notified with START_PENDING...
				continue;
			}
			else
			{
				return WaitStatus;
			}
        }
		else if ( WaitStatus == WAIT_OBJECT_0 )
		{
            break;
        }
		else
		{
            (VOID) NtClose( EventHandle );
            return WaitStatus;
        }
    }

    (VOID) NtClose( EventHandle );
    return S_OK;

}
