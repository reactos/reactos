/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Setup Library
 * FILE:            base/setup/lib/arcname.h
 * PURPOSE:         ARC path to-and-from NT path resolver.
 * PROGRAMMER:      Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

BOOLEAN
ArcPathNormalize(
    OUT PUNICODE_STRING NormalizedArcPath,
    IN  PCWSTR ArcPath);

BOOLEAN
ArcPathToNtPath(
    OUT PUNICODE_STRING NtPath,
    IN  PCWSTR ArcPath,
    IN  PPARTLIST PartList OPTIONAL);

/* EOF */
