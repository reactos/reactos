/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/drivesup.h
 * PURPOSE:         Drive support functions
 * PROGRAMMER:      Eric Kohl
 */

#pragma once

NTSTATUS
GetSourcePaths(
    OUT PUNICODE_STRING SourcePath,
    OUT PUNICODE_STRING SourceRootPath,
    OUT PUNICODE_STRING SourceRootDir);

/* EOF */
