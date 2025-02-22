/*
 * PROJECT:     ReactOS CabView Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Precompiled header file
 * COPYRIGHT:   Copyright 2024 Whindmar Saksit <whindsaks@proton.me>
 */

#pragma once
#define NTOS_MODE_USER
#include <windows.h>
#include <atlbase.h>
#include <atlcom.h>
#include <strsafe.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <shlguid_undoc.h>
#define NTSTATUS LONG // for debug.h
#include <reactos/debug.h>
#include <shellutils.h>
#include <ntquery.h>
#include <fdi.h>

#ifndef SFGAO_SYSTEM
#define SFGAO_SYSTEM 0x00001000
#endif

#ifndef SHGSI_ICONLOCATION
#define SIID_FOLDER 3
#define SIID_FOLDEROPEN 4
#endif

EXTERN_C INT WINAPI SHFormatDateTimeA(const FILETIME UNALIGNED *fileTime, DWORD *flags, LPSTR buf, UINT size);
