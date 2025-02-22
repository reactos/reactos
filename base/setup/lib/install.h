/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Setup Library
 * FILE:            base/setup/lib/install.c
 * PURPOSE:         Installation functions
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

typedef enum _FILE_COPY_STATUS
{
    None = 0,
    // Success = 0,
} FILE_COPY_STATUS;

typedef VOID
(__cdecl *PFILE_COPY_STATUS_ROUTINE)(IN FILE_COPY_STATUS, ...);

#if 0
BOOLEAN // ERROR_NUMBER
PrepareCopyInfFile(
    IN OUT PUSETUP_DATA pSetupData,
    IN HINF InfFile,
    IN PCWSTR SourceCabinet OPTIONAL);
#endif

BOOLEAN // ERROR_NUMBER
NTAPI
PrepareFileCopy(
    IN OUT PUSETUP_DATA pSetupData,
    IN PFILE_COPY_STATUS_ROUTINE StatusRoutine OPTIONAL);

BOOLEAN
NTAPI
DoFileCopy(
    IN OUT PUSETUP_DATA pSetupData,
    IN PSP_FILE_CALLBACK_W MsgHandler,
    IN PVOID Context OPTIONAL);

/* EOF */
