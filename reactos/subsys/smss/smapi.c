/* $Id: smapi.c,v 1.2 1999/12/11 01:42:44 ekohl Exp $
 *
 * Reactos Session Manager
 *
 *
 */

#include <ddk/ntddk.h>


#include "smss.h"

//#define NDEBUG


VOID STDCALL
SmApiThread (HANDLE Port)
{
    NTSTATUS Status;
    ULONG Unknown;
    PLPCMESSAGE Reply = NULL;
    LPCMESSAGE Message;

#ifndef NDEBUG
    DisplayString (L"SmApiThread: running\n");
#endif

    for (;;)
    {
#ifndef NDEBUG
        DisplayString (L"SmApiThread: waiting for message\n");
#endif

        Status = NtReplyWaitReceivePort (Port,
                                         &Unknown,
                                         Reply,
                                         &Message);
        if (NT_SUCCESS(Status))
        {
#ifndef NDEBUG
            DisplayString (L"SmApiThread: message received\n");
#endif

            if (Message.MessageType == LPC_CONNECTION_REQUEST)
            {

            }
            else
            {

            }

        }
    }

#ifndef NDEBUG
    DisplayString (L"SmApiThread: terminating\n");
#endif
    NtTerminateThread (NtCurrentThread (), STATUS_UNSUCCESSFUL);
}

/* EOF */
