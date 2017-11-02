/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Setup Library
 * FILE:            base/setup/lib/ntverrsrc.h
 * PURPOSE:         NT Version Resource Management API
 * PROGRAMMER:      Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

NTSTATUS
NtGetVersionResource(
    IN PVOID BaseAddress,
    OUT PVOID* Resource,
    OUT PULONG ResourceSize OPTIONAL);

NTSTATUS
NtVerQueryValue(
    IN const VOID* pBlock,
    IN PCWSTR lpSubBlock,
    OUT PVOID* lplpBuffer,
    OUT PUINT puLen);

/* EOF */
