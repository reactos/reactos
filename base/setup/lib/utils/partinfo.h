/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     MBR and GPT Partition types
 * COPYRIGHT:   Copyright 2018-2020 Hermes Belusca-Maito
 */

#pragma once

/* MBR PARTITION TYPES ******************************************************/

typedef struct _MBR_PARTITION_TYPE
{
    UCHAR Type;
    PCSTR Description;
} MBR_PARTITION_TYPE, *PMBR_PARTITION_TYPE;

#define NUM_MBR_PARTITION_TYPES 153
extern SPLIBAPI const MBR_PARTITION_TYPE MbrPartitionTypes[NUM_MBR_PARTITION_TYPES];

/* GPT PARTITION TYPES ******************************************************/

typedef struct _GPT_PARTITION_TYPE
{
    GUID Guid;
    PCSTR Description;
} GPT_PARTITION_TYPE, *PGPT_PARTITION_TYPE;

#define NUM_GPT_PARTITION_TYPES 177
extern SPLIBAPI const GPT_PARTITION_TYPE GptPartitionTypes[NUM_GPT_PARTITION_TYPES];

/* EOF */
