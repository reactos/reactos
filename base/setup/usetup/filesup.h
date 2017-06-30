/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/filesup.h
 * PURPOSE:         File support functions
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

#pragma once

NTSTATUS
SetupCreateDirectory(
    PWCHAR DirectoryName);

NTSTATUS
SetupExtractFile(
    PWCHAR CabinetFileName,
    PWCHAR SourceFileName,
    PWCHAR DestinationFileName);

BOOLEAN
IsValidPath(
    IN PCWSTR InstallDir);

/* EOF */
