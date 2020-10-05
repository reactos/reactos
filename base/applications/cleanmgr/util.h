/*
 * PROJECT:         ReactOS Disk Cleanup
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         Definations and Functions for util.c
 * COPYRIGHT:       Copyright 2020 Arnav Bhatt (arnavbhatt288 at gmail dot com)
 */

#pragma once

#include "precomp.h"

#define KB 1024
#define MB 1024 * KB
#define GB 1024 * MB
#define TB 1024 * GB

#define ALLOW_FILE_REMOVAL 2
#define DISALLOW_FILE_REMOVAL 0

#define ICON_BIN 2
#define ICON_BLANK 0
#define ICON_FOLDER 1

/* Bitmap size */
#define CX_BITMAP 20
#define CY_BITMAP 20

#define ONLY_PHYSICAL_DRIVE 3

/* For EnableDialogTheme() function */
#define ETDT_DISABLE        0x00000001
#define ETDT_ENABLE         0x00000002
#define ETDT_USETABTEXTURE  0x00000004
#define ETDT_ENABLETAB      (ETDT_ENABLE  | ETDT_USETABTEXTURE)

static WCHAR TempDir[ARR_MAX_SIZE];

typedef enum
{
    OLD_CHKDSK_FILES = 0,
    RAPPS_FILES = 1,
    RECYCLE_BIN = 2,
    TEMPORARY_FILE = 3
} DIRECTORIES;

typedef struct
{
    BOOL TempClean;
    BOOL RecycleClean;
    BOOL ChkDskClean;
    BOOL RappsClean;
} CLEAN_DIR;

CLEAN_DIR CleanDirectories;
DIRSIZE DirectorySizes;
DLG_HANDLE DialogHandle;

typedef HRESULT (WINAPI * ETDTProc) (HWND, DWORD);

BOOL CreateImageLists(HWND hList);
BOOL DriverunProc(LPWSTR* ArgList, PWCHAR LogicalDrives);
BOOL EnableDialogTheme(HWND hwnd);
BOOL GetStageFlagVal(PWCHAR RegArg, PWCHAR SubKey);
BOOL SetStageFlagVal(PWCHAR RegArg, PWCHAR SubKey, BOOL ArgBool);

PWCHAR GetStageFlag(int nArgs, PWCHAR ArgSpecified, LPWSTR* ArgList);

uint64_t GetTargetedDirSize(PWCHAR SpecifiedDir);

void AddRequiredItem(HWND hList, UINT StringID, PWCHAR SubString, int ItemIndex);
void CleanRequiredPath(PCWSTR TempPath);
void SagesetProc(int nArgs, PWCHAR ArgSpecified, LPWSTR* ArgList);
void SagerunProc(int nArgs, PWCHAR ArgSpecified, LPWSTR* ArgList, PWCHAR LogicalDrives);
void SetDetails(UINT StringID, UINT ResourceID, HWND hwnd);
void SetTotalSize(uint64_t size, UINT ResourceID, HWND hwnd);
