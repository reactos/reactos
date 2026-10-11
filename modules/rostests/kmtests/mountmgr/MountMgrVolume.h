/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Shared definitions for the MountMgr persisted-link regression test
 * COPYRIGHT:   Copyright 2026 Alejandro Sánchez <alesangreat@gmail.com>
 */

#pragma once

#define MOUNTMGR_VOLUME_QUERY_INFO 1

#define MOUNTMGR_TEST_DEVICE_NAME_CCH   96
#define MOUNTMGR_TEST_VOLUME_NAME_CCH   64
#define MOUNTMGR_TEST_SENTINEL_NAME_CCH 64

typedef struct _MOUNTMGR_VOLUME_TEST_INFO
{
    GUID UniqueId;
    WCHAR DeviceName[MOUNTMGR_TEST_DEVICE_NAME_CCH];
    WCHAR VolumeName[MOUNTMGR_TEST_VOLUME_NAME_CCH];
    WCHAR SentinelName[MOUNTMGR_TEST_SENTINEL_NAME_CCH];
} MOUNTMGR_VOLUME_TEST_INFO, *PMOUNTMGR_VOLUME_TEST_INFO;
