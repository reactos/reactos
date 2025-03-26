/*
 * PROJECT:     ReactOS SM Helper Library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ReactOS-specific SM Debug Utility Function:
 *              Querying subsystem information.
 * COPYRIGHT:   Copyright 2005 Emanuele Aliberti <ea@reactos.com>
 *              Copyright 2022 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#ifndef _SMROSDBG_H_
#define _SMROSDBG_H_

#pragma once

/* Note: this is not in NT */
/* Ask SM to send back some data */
#define SM_API_QUERY_INFORMATION    SmpMaxApiNumber

#define SM_QRYINFO_MAX_SS_COUNT     8
#define SM_QRYINFO_MAX_ROOT_NODE    30

typedef enum {
    SmBasicInformation = 0,
    SmSubSystemInformation
} SM_INFORMATION_CLASS;

typedef struct _SM_BASIC_INFORMATION
{
    USHORT SubSystemCount;
    USHORT Unused;
    struct
    {
        USHORT Id;
        USHORT Flags;
        ULONG ProcessId;
    } SubSystem[SM_QRYINFO_MAX_SS_COUNT];
} SM_BASIC_INFORMATION, *PSM_BASIC_INFORMATION;

typedef struct _SM_SUBSYSTEM_INFORMATION
{
    USHORT SubSystemId;
    USHORT Flags;
    ULONG ProcessId;
    WCHAR NameSpaceRootNode[SM_QRYINFO_MAX_ROOT_NODE];
} SM_SUBSYSTEM_INFORMATION, *PSM_SUBSYSTEM_INFORMATION;

typedef struct _SM_QUERYINFO_MSG
{
    SM_INFORMATION_CLASS SmInformationClass;
    ULONG DataLength;
    union
    {
        SM_BASIC_INFORMATION BasicInformation;
        SM_SUBSYSTEM_INFORMATION SubSystemInformation;
    };
} SM_QUERYINFO_MSG, *PSM_QUERYINFO_MSG;

#endif // _SMROSDBG_H_
