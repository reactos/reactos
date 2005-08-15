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

#define LPC_MESSAGE_TYPE(m) ((m).Header.u2.s2.Type)

/* DEPENDENCIES **************************************************************/

/* EXPORTED DATA *************************************************************/

/* CONSTANTS *****************************************************************/
#define LPC_MESSAGE_BASE_SIZE 24
#define MAX_MESSAGE_DATA      (0x130)
#define LPC_MAX_DATA_LENGTH 0x104
#define LPC_MAX_MESSAGE_LENGTH 0x148

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

#if defined(USE_LPC6432)
#define LPC_CLIENT_ID CLIENT_ID64
#define LPC_SIZE_T ULONGLONG
#define LPC_PVOID ULONGLONG
#define LPC_HANDLE ULONGLONG
#else
#define LPC_CLIENT_ID CLIENT_ID
#define LPC_SIZE_T SIZE_T
#define LPC_PVOID PVOID
#define LPC_HANDLE HANDLE
#endif

typedef struct _PORT_MESSAGE
{
    union
    {
        struct
        {
            CSHORT DataLength;
            CSHORT TotalLength;
        } s1;
        ULONG Length;
    } u1;
    union
    {
        struct
        {
            CSHORT Type;
            CSHORT DataInfoOffset;
        } s2;
        ULONG ZeroInit;
    } u2;
    union
    {
        LPC_CLIENT_ID ClientId;
        double DoNotUseThisField;
    };
    ULONG MessageId;
    union
    {
        LPC_SIZE_T ClientViewSize;
        ULONG CallbackId;
    };
} PORT_MESSAGE, *PPORT_MESSAGE;

typedef struct _PORT_VIEW
{
    ULONG Length;
    LPC_HANDLE SectionHandle;
    ULONG SectionOffset;
    LPC_SIZE_T ViewSize;
    LPC_PVOID ViewBase;
    LPC_PVOID ViewRemoteBase;
} PORT_VIEW, *PPORT_VIEW;

typedef struct _REMOTE_PORT_VIEW
{
    ULONG Length;
    LPC_SIZE_T ViewSize;
    LPC_PVOID ViewBase;
} REMOTE_PORT_VIEW, *PREMOTE_PORT_VIEW;

/* FIXME: USE REAL DEFINITION */
typedef struct _LPC_MAX_MESSAGE
{
    PORT_MESSAGE Header;
    BYTE Data[MAX_MESSAGE_DATA];
} LPC_MAX_MESSAGE, *PLPC_MAX_MESSAGE;

#endif
