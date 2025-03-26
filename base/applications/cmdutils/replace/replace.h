/*
 * PROJECT:     ReactOS Replace Command
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Main header
 * COPYRIGHT:   Copyright Samuel Erdtman (samuel@erdtman.se)
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <conutils.h>
#include "resource.h"

#define RC_STRING_MAX_SIZE 3072

#define PROMPT_NO    0
#define PROMPT_YES   1
#define PROMPT_ALL   2
#define PROMPT_BREAK 3

/* 16k = max buffer size */
#define BUFF_SIZE 16384

#define ConOutResPuts(uID) \
    ConResPuts(StdOut, (uID))

#define ConOutResPrintf(uID, ...) \
    ConResPrintf(StdOut, (uID), ##__VA_ARGS__)

#define ConOutFormatMessage(MessageId, ...) \
    ConFormatMessage(StdOut, (MessageId), ##__VA_ARGS__)

/* util.c */
VOID ConInString(LPTSTR lpInput, DWORD dwLength);
VOID __cdecl ConFormatMessage(PCON_STREAM Stream, DWORD MessageId, ...);
VOID ConOutChar(TCHAR c);
VOID GetPathCase(TCHAR * Path, TCHAR * OutPath);
BOOL IsExistingFile(IN LPCTSTR pszPath);
BOOL IsExistingDirectory(IN LPCTSTR pszPath);
INT FilePromptYNA(UINT resID);
VOID msg_pause(VOID);
TCHAR cgetchar(VOID);

INT
GetRootPath(
    IN LPCTSTR InPath,
    OUT LPTSTR OutPath,
    IN INT size);

extern BOOL bCtrlBreak;
