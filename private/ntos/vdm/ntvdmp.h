/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ntvdmp.h

Abstract:

    Vdm Private include file for platform independent vdm support functions


Author:

    20-May-1994 Jonle

Revision History:

--*/


NTSTATUS
VdmQueryDirectoryFile(
    PVDMQUERYDIRINFO VdmQueryDir
    );

NTSTATUS
VdmIsLegalFatName(
    POEM_STRING pOemFileNameString
    );
