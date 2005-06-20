/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/lpctypes.h
 * PURPOSE:         Definitions for Local Procedure Call Types not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _LPCTYPES_H
#define _LPCTYPES_H

/* DEPENDENCIES **************************************************************/

/* EXPORTED DATA *************************************************************/

/* CONSTANTS *****************************************************************/
#define LPC_MESSAGE_BASE_SIZE 24
#define MAX_MESSAGE_DATA      (0x130)

/* ENUMERATIONS **************************************************************/

typedef enum _LPC_TYPE 
{
    LPC_NEW_MESSAGE,
    LPC_REQUEST,
    LPC_REPLY,
    LPC_DATAGRAM,
    LPC_LOST_REPLY,
    LPC_PORT_CLOSED,
    LPC_CLIENT_DIED,
    LPC_EXCEPTION,
    LPC_DEBUG_EVENT,
    LPC_ERROR_EVENT,
    LPC_CONNECTION_REQUEST,
    LPC_CONNECTION_REFUSED,
    LPC_MAXIMUM
} LPC_TYPE;

/* TYPES *********************************************************************/

/* FIXME: USE REAL DEFINITION */
typedef struct _LPC_MESSAGE {
    USHORT  DataSize;
    USHORT  MessageSize;
    USHORT  MessageType;
    USHORT  VirtualRangesOffset;
    CLIENT_ID  ClientId;
    ULONG  MessageId;
    ULONG  SectionSize;
} LPC_MESSAGE, *PLPC_MESSAGE;

/* FIXME: USE REAL DEFINITION */
typedef struct _LPC_SECTION_WRITE 
{
    ULONG  Length;
    HANDLE  SectionHandle;
    ULONG  SectionOffset;
    ULONG  ViewSize;
    PVOID  ViewBase;
    PVOID  TargetViewBase;
} LPC_SECTION_WRITE, *PLPC_SECTION_WRITE;

/* FIXME: USE REAL DEFINITION */
typedef struct _LPC_SECTION_READ 
{
    ULONG  Length;
    ULONG  ViewSize;
    PVOID  ViewBase;
} LPC_SECTION_READ, *PLPC_SECTION_READ; 

/* FIXME: USE REAL DEFINITION */
typedef struct _LPC_MAX_MESSAGE 
{
    LPC_MESSAGE Header;
    BYTE Data[MAX_MESSAGE_DATA];
} LPC_MAX_MESSAGE, *PLPC_MAX_MESSAGE;

#endif
