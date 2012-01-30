/*
 * PROJECT:         ReactOS Windows-Compatible Session Manager
 * LICENSE:         BSD 2-Clause License
 * FILE:            base/system/smss/smss.c
 * PURPOSE:         Main SMSS Code
 * PROGRAMMERS:     Alex Ionescu
 */

/* INCLUDES *******************************************************************/

#include "smss.h"
#define NDEBUG
#include "debug.h"

/* GLOBALS ********************************************************************/

/* FUNCTIONS ******************************************************************/

ULONG
NTAPI
SmpApiLoop(IN PVOID Parameter)
{
    HANDLE SmApiPort = (HANDLE)Parameter;
    NTSTATUS Status;
    PVOID ClientContext;
    PSM_API_MSG RequestMsg = NULL;
    SM_API_MSG ReplyMsg;

    DPRINT1("API Loop: %p\n", SmApiPort);
    while (TRUE)
    {
        Status = NtReplyWaitReceivePort(SmApiPort,
                                        &ClientContext,
                                        &RequestMsg->h,
                                        &ReplyMsg.h);
        DPRINT1("API loop returned: %lx\n", Status);
    }
    return STATUS_SUCCESS;
}
