/*
 * PROJECT:     VerOS NT
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        sdk/include/update_shutdown.h
 * PURPOSE:     Shutdown dialog hook for a registered update client
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 */

#pragma once

#define UPDATE_SHUTDOWN_HOOK_KEY L"SOFTWARE\\SystemUpdate\\ShutdownHook"
#define UPDATE_SHUTDOWN_HOOK_VALUE L"Library"
#define UPDATE_SHUTDOWN_MAX_CHOICES 8

typedef struct _UPDATE_SHUTDOWN_CHOICE
{
    UINT Id;
    UINT FinalAction;
    WCHAR Label[64];
    WCHAR Detail[256];
} UPDATE_SHUTDOWN_CHOICE;

typedef struct _UPDATE_SHUTDOWN_QUERY
{
    UINT Count;
    UPDATE_SHUTDOWN_CHOICE Choices[UPDATE_SHUTDOWN_MAX_CHOICES];
} UPDATE_SHUTDOWN_QUERY;

typedef struct _UPDATE_SHUTDOWN_COMMIT
{
    UINT ChoiceId;
    WCHAR Notice[256];
} UPDATE_SHUTDOWN_COMMIT;

typedef BOOL (WINAPI *PFN_UPDATE_SHUTDOWN_QUERY)(UPDATE_SHUTDOWN_QUERY *Query);
typedef BOOL (WINAPI *PFN_UPDATE_SHUTDOWN_COMMIT)(UPDATE_SHUTDOWN_COMMIT *Commit);
