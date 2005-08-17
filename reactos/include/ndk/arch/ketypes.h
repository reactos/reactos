/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/arch/ketypes.h
 * PURPOSE:         Architecture-specific definitions for Kernel Types
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _ARCH_KETYPES_H
#define _ARCH_KETYPES_H

/* Shared structures needed by Arch-specific headers */
typedef struct _KDPC_DATA
{
    LIST_ENTRY  DpcListHead;
    ULONG  DpcLock;
    ULONG  DpcQueueDepth;
    ULONG  DpcCount;
} KDPC_DATA, *PKDPC_DATA;

typedef struct _PP_LOOKASIDE_LIST
{
    struct _GENERAL_LOOKASIDE *P;
    struct _GENERAL_LOOKASIDE *L;
} PP_LOOKASIDE_LIST, *PPP_LOOKASIDE_LIST;

#ifdef _M_IX86
#include <ndk/i386/ketypes.h>
#else
#error "Unknown processor"
#endif

#endif
