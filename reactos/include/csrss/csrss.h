#ifndef __INCLUDE_CSRSS_CSRSS_H
#define __INCLUDE_CSRSS_CSRSS_H

#include <napi/lpc.h>

#define CSR_PRIORITY_CLASS_NORMAL	(0x10)
#define CSR_PRIORITY_CLASS_IDLE		(0x20)
#define CSR_PRIORITY_CLASS_HIGH		(0x40)
#define CSR_PRIORITY_CLASS_REALTIME	(0x80)


typedef struct
{
} CSRSS_CONNECT_PROCESS_REQUEST, PCSRSS_CONNECT_PROCESS_REQUEST;

typedef struct
{
} CSRSS_CONNECT_PROCESS_REPLY, PCSRSS_CONNECT_PROCESS_REPLY;

typedef struct
{
   ULONG NewProcessId;
   ULONG Flags;
} CSRSS_CREATE_PROCESS_REQUEST, *PCSRSS_CREATE_PROCESS_REQUEST;

typedef struct
{
   HANDLE ConsoleHandle;
} CSRSS_CREATE_PROCESS_REPLY, *PCSRSS_CREATE_PROCESS_REPLY;

typedef struct
{
   HANDLE ConsoleHandle;
   ULONG NrCharactersToWrite;
   BYTE Buffer[1];
} CSRSS_WRITE_CONSOLE_REQUEST, *PCSRSS_WRITE_CONSOLE_REQUEST;

typedef struct
{
   ULONG NrCharactersWritten;
} CSRSS_WRITE_CONSOLE_REPLY, *PCSRSS_WRITE_CONSOLE_REPLY;

typedef struct
{
   HANDLE ConsoleHandle;
   ULONG NrCharactersToRead;
} CSRSS_READ_CONSOLE_REQUEST, *PCSRSS_READ_CONSOLE_REQUEST;

typedef struct
{
   HANDLE EventHandle;
   ULONG NrCharactersRead;
   BYTE Buffer[1];
} CSRSS_READ_CONSOLE_REPLY, *PCSRSS_READ_CONSOLE_REPLY;

typedef struct
{
   HANDLE ConsoleHandle;
} CSRSS_ALLOC_CONSOLE_REPLY, *PCSRSS_ALLOC_CONSOLE_REPLY;

#define CSRSS_CREATE_PROCESS            (0x1)
#define CSRSS_TERMINATE_PROCESS         (0x2)
#define CSRSS_WRITE_CONSOLE             (0x3)
#define CSRSS_READ_CONSOLE              (0x4)
#define CSRSS_ALLOC_CONSOLE             (0x5)
#define CSRSS_FREE_CONSOLE              (0x6)
#define CSRSS_CONNECT_PROCESS           (0x7)

typedef struct
{
   LPC_MESSAGE_HEADER Header;
   ULONG Type;
   union
     {
	CSRSS_CREATE_PROCESS_REQUEST CreateProcessRequest;
	CSRSS_CONNECT_PROCESS_REQUEST ConnectRequest;
	CSRSS_WRITE_CONSOLE_REQUEST WriteConsoleRequest;
	CSRSS_READ_CONSOLE_REQUEST ReadConsoleRequest;
     } Data;
} CSRSS_API_REQUEST, *PCSRSS_API_REQUEST;

typedef struct
{
   LPC_MESSAGE_HEADER Header;
   NTSTATUS Status;
   union
     {
	CSRSS_CREATE_PROCESS_REPLY CreateProcessReply;
	CSRSS_CONNECT_PROCESS_REPLY ConnectReply;
	CSRSS_WRITE_CONSOLE_REPLY WriteConsoleReply;
	CSRSS_READ_CONSOLE_REPLY ReadConsoleReply;
	CSRSS_ALLOC_CONSOLE_REPLY AllocConsoleReply;
     } Data;
} CSRSS_API_REPLY, *PCSRSS_API_REPLY;

#endif /* __INCLUDE_CSRSS_CSRSS_H */
