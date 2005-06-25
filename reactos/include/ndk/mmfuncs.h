/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/halfuncs.h
 * PURPOSE:         Prototypes for exported HAL Functions not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _MMFUNCS_H
#define _MMFUNCS_H

/* DEPENDENCIES **************************************************************/


/* PROTOTYPES ****************************************************************/
NTSTATUS 
STDCALL
MmUnmapViewOfSection(
    struct _EPROCESS* Process, 
    PVOID BaseAddress
);

#endif
