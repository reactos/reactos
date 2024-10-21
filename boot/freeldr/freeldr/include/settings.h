/*
 * PROJECT:     FreeLoader
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Command-line parsing and global settings management
 * COPYRIGHT:   Copyright 2024 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#pragma once

typedef struct _BOOTMGRINFO
{
    PCSTR DebugString;
    PCSTR DefaultOs;
    LONG  TimeOut;
    ULONG_PTR FrLdrSection;
} BOOTMGRINFO, *PBOOTMGRINFO;

extern BOOTMGRINFO BootMgrInfo;

PBOOTMGRINFO GetBootMgrInfo(VOID);

VOID
LoadSettings(
    _In_opt_ PCSTR CmdLine);

/* EOF */
