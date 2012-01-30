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

LIST_ENTRY SmpPagingFileDescriptorList, SmpVolumeDescriptorList;

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
SmpPagingFileInitialize(VOID)
{
    /* Initialize the two lists */
    InitializeListHead(&SmpPagingFileDescriptorList);
    InitializeListHead(&SmpVolumeDescriptorList);
}

NTSTATUS
NTAPI
SmpCreatePagingFileDescriptor(IN PUNICODE_STRING PageFileToken)
{
    DPRINT1("New pagefile data: %wZ\n", PageFileToken);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
SmpCreatePagingFiles(VOID)
{
    return STATUS_SUCCESS;
}
