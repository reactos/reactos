/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    elflpc.h

Abstract:

    This file contains defines for LPC for the eventlog service

Author:

    Rajen Shah (rajens) 10-Jul-1991

Revision History:

--*/

#ifndef _ELFLPC_
#define _ELFLPC_

// Name of the LPC port for kernel objects to communicate with the eventlog
// service
#define     ELF_PORT_NAME	    "\\ErrorLogPort"
#define     ELF_PORT_NAME_U	    L"\\ErrorLogPort"

//
// Max size of data sent to the eventlogging service through the LPC port.
//

#define     ELF_PORT_MAX_MESSAGE_LENGTH IO_ERROR_LOG_MESSAGE_LENGTH

//
// Structure that is passed in from the system thread to the LPC port
//

typedef struct _ELFIOPORTMSG {
   PORT_MESSAGE PortMessage;
   IO_ERROR_LOG_MESSAGE IoErrorLogMessage;
} ELFIOPORTMSG, *PELFIOPORTMSG;

//
// Structure for the message as a reply from the eventlogging service to
// the LPC client.
//

typedef struct _ELF_REPLY_MESSAGE {
    PORT_MESSAGE PortMessage;
    NTSTATUS Status;
} ELF_REPLY_MESSAGE, *PELF_REPLY_MESSAGE;

#endif // ifndef _ELFLPC_
