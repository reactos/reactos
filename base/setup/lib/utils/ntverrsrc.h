/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     NT Version Resource Management API
 * COPYRIGHT:   Copyright 2017-2018 Hermes Belusca-Maito
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
