/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Precompiled header
 * COPYRIGHT:   Copyright 2019 Pierre Schweitzer <pierre@reactos.org>
 *              Copyright 2025 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#pragma once

#define WIN32_NO_STATUS
#include <apitest.h>

#define NTOS_MODE_USER
#include <ndk/iofuncs.h>
#include <ndk/rtlfuncs.h>

#include <mountmgr.h>
#include <strsafe.h>

/* utils.c */

LPCSTR wine_dbgstr_us(const UNICODE_STRING *us);

HANDLE
GetMountMgrHandle(
    _In_ ACCESS_MASK DesiredAccess);

VOID
DumpBuffer(
    _In_ PVOID Buffer,
    _In_ ULONG Length);

/* EOF */
