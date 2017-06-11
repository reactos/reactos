/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/filesup.h
 * PURPOSE:         File support functions
 * PROGRAMMER:      Eric Kohl
 */

#pragma once

NTSTATUS
SetupCreateDirectory(
    PWCHAR DirectoryName);

NTSTATUS
SetupDeleteFile(
    IN PCWSTR FileName,
    IN BOOLEAN ForceDelete); // ForceDelete can be used to delete read-only files

NTSTATUS
SetupCopyFile(
    IN PCWSTR SourceFileName,
    IN PCWSTR DestinationFileName,
    IN BOOLEAN FailIfExists);

#ifndef _WINBASE_

#define MOVEFILE_REPLACE_EXISTING   1
#define MOVEFILE_COPY_ALLOWED       2
#define MOVEFILE_WRITE_THROUGH      8

#endif

// ACHTUNG! HAXX FIXME!!
#define _SEH2_TRY
#define _SEH2_LEAVE     goto __SEH2_FINALLY__label;
#define _SEH2_FINALLY   __SEH2_FINALLY__label:
#define _SEH2_END


NTSTATUS
SetupMoveFile(
    IN PCWSTR ExistingFileName,
    IN PCWSTR NewFileName,
    IN ULONG Flags);

NTSTATUS
SetupExtractFile(
    PWCHAR CabinetFileName,
    PWCHAR SourceFileName,
    PWCHAR DestinationFileName);


BOOLEAN
IsValidPath(
    IN PWCHAR InstallDir,
    IN ULONG Length);

/* EOF */
