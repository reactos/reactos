/* $Id: lpcproto.h,v 1.1 2002/03/17 22:19:37 ea Exp $
 *
 * ReactOS POSIX+ Environment Subsystem
 * LPC protocol spoken by PSXSS.EXE, PSXDLL.DLL, CSRTERM.EXE.
 */
#ifndef _PSX_LPCPROTO_H
#define _PSX_LPCPROTO_H
#include <napi/lpc.h>
/* Protocol version */
#define PSX_LPC_PROTOCOL_VERSION 1
/* POSIX+ system name space */
#define PSX_NS_SUBSYSTEM_DIRECTORY_NAME L"POSIX+"
#define PSX_NS_SESSION_DIRECTORY_NAME   L"Sessions"
#define PSX_NS_SYSTEM_DIRECTORY_NAME    L"System"
#define PSX_NS_API_PORT_NAME            L"ApiPort"
#define PSX_NS_SBAPI_PORT_NAME          L"SbApiPort"
#define PSX_NS_SESSIONAPI_PORT_NAME     L"SessionPort"
#define PSX_NS_API_PORT_TEMPLATE        L"\\%s\\%s"
#define PSX_NS_SESSION_PORT_TEMPLATE    L"\\%s\\%s\\P%d"
#define PSX_NS_SESSION_DATA_TEMPLATE    L"\\%s\\%s\\D%d"
/* ConnectData protocol */
typedef enum {
	PSX_CONNECTION_TYPE_PROCESS,
	PSX_CONNECTION_TYPE_TERMINAL
} PSX_CONNECTION_TYPE;
typedef struct _PSX_CONNECT_PORT_DATA
{
	PSX_CONNECTION_TYPE	ConnectionType; /* IN OUT */
	ULONG			Version;	/* IN OUT */
	ULONG			PortIdentifier; /* OUT */
} PSX_CONNECT_PORT_DATA;
/* LPC message subsystem-specific header */
typedef struct _PSX_MESSAGE_HEADER
{
	WORD			Context;
	WORD			Procedure;
	NTSTATUS		Status;
} PSX_MESSAGE_HEADER, * PPSX_MESSAGE_HEADER;
typedef PSX_MESSAGE_HEADER PSX_MESSAGE, * PPSX_MESSAGE;
#define PSX_MAX_LPC_DATA_SIZE 128 /* compute it*/
typedef struct _PSX_MAX_MESSAGE
{
    LPC_MESSAGE_HEADER Header;
    PSX_MESSAGE_HEADER PsxHeader;
    BYTE               Data [PSX_MAX_LPC_DATA_SIZE];
} PSX_MAX_MESSAGE, * PPSX_MAX_MESSAGE;
/* Terminal I/O */
/* \POSIX+\SessionPort API */
typedef enum {
	PSX_TERMINAL_INTERRUPT,
	PSX_TERMINAL_SESSION_STATUS_REQUEST
} PSX_TERMINAL_API;
typedef struct _PSX_TERMINAL_READ
{
    LPC_MESSAGE_HEADER Header;
    PSX_MESSAGE_HEADER PsxHeader;
    ULONG              Size;
    CHAR               Buffer [80]; /* FIXME! */
} PSX_TERMINAL_READ, * PPSX_TERMINAL_READ;
/* System I/O (system calls) */
/* #include <psx/syscall.h> */
#endif /* ndef _PSX_LPCPROTO_H */
