/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     MBR and GPT Partition types
 * COPYRIGHT:   Copyright 2018-2025 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#pragma once

PCSTR
NTAPI
LookupPartitionTypeString(
    _In_ PARTITION_STYLE PartitionStyle,
    _In_ PVOID PartitionType);

/* EOF */
