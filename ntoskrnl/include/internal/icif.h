/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Internal header for information classes info interface
 * COPYRIGHT:   Copyright 2020-2022 George Bișoc <george.bisoc@reactos.org>
 */

#pragma once

/*
 * Implement generic information class probing code in a
 * separate header within the NT kernel header internals.
 * This makes it accessible to other sources by including
 * the header.
 */

#define ICIF_NONE                0x0
#define ICIF_QUERY               0x1
#define ICIF_SET                 0x2
#define ICIF_QUERY_SIZE_VARIABLE 0x4
#define ICIF_SET_SIZE_VARIABLE   0x8
#define ICIF_SIZE_VARIABLE (ICIF_QUERY_SIZE_VARIABLE | ICIF_SET_SIZE_VARIABLE)

#define ICIF_PROBE_READ_WRITE          0x0
#define ICIF_PROBE_READ                0x1
#define ICIF_FORCE_RETURN_LENGTH_PROBE 0x2

typedef struct _INFORMATION_CLASS_INFO
{
    USHORT RequiredSizeQUERY;
    UCHAR AlignmentQUERY;
    USHORT RequiredSizeSET;
    UCHAR AlignmentSET;
    USHORT Flags;
} INFORMATION_CLASS_INFO, *PINFORMATION_CLASS_INFO;

#define IQS_SAME(Type, Alignment, Flags) \
  { sizeof(Type), sizeof(Alignment), sizeof(Type), sizeof(Alignment), Flags }

#define IQS(TypeQuery, AlignmentQuery, TypeSet, AlignmentSet, Flags) \
  { sizeof(TypeQuery), sizeof(AlignmentQuery), sizeof(TypeSet), sizeof(AlignmentSet), Flags }

#define IQS_NO_TYPE_LENGTH(Alignment, Flags) \
  { 0, sizeof(Alignment), 0, sizeof(Alignment), Flags }

#define IQS_NONE \
  { 0, sizeof(CHAR), 0, sizeof(CHAR), ICIF_NONE }
