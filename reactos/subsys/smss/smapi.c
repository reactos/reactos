/* $Id: smapi.c,v 1.3 1999/12/28 16:25:21 ekohl Exp $
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
//                SmHandleConnectionRequest (Port, &Message);
                Reply = NULL;
            }
            else if (Message.MessageType == LPC_DEBUG_EVENT)
            {
//                DbgSsHandleKmApiMsg (&Message, 0);
                Reply = NULL;
            }
            else if (Message.MessageType == LPC_PORT_CLOSED)
            {
                Reply = NULL;
            }
            else
            {

//                Reply = &Message;
            }
        }
    }
}

/* EOF */
