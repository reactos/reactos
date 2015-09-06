/*
* COPYRIGHT:       See COPYING.ARM in the top level directory
* PROJECT:         ReactOS UEFI Boot Library
* FILE:            boot/environ/lib/mm/i386/mmx86.c
* PURPOSE:         Boot Library Memory Manager x86-Specific Code
* PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include "bl.h"

/* DATA VARIABLES ************************************************************/

/* FUNCTIONS *****************************************************************/

NTSTATUS
MmArchInitialize (
    _In_ ULONG Phase,
    _In_ PBL_MEMORY_DATA MemoryData,
    _In_ BL_TRANSLATION_TYPE TranslationType,
    _In_ BL_TRANSLATION_TYPE LibraryTranslationType
    )
{
    return STATUS_NOT_IMPLEMENTED;
}
