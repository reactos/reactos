/*
 * PROJECT:     ReactOS Clipboard Viewer
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Clipboard file format helper functions.
 * COPYRIGHT:   Copyright 2015-2018 Ricardo Hanke
 *              Copyright 2015-2018 Hermes Belusca-Maito
 */

#pragma once

/* Clipboard file format signatures */
#define CLIP_FMT_31 0xC350
#define CLIP_FMT_NT 0xC351
#define CLIP_FMT_BK 0xC352

#define MAX_FMT_NAME_LEN 79

/*
 * Win3.1 Clipboard File Format (default)
 */
#pragma pack(push, 1)
typedef struct _CLIPFILEHEADER
{
    WORD wFileIdentifier;
    WORD wFormatCount;
} CLIPFILEHEADER;

typedef struct _CLIPFORMATHEADER
{
    WORD  dwFormatID;
    DWORD dwLenData;
    DWORD dwOffData;
    CHAR  szName[MAX_FMT_NAME_LEN];
} CLIPFORMATHEADER;
#pragma pack(pop)

/*
 * NT Clipboard File Format
 */
typedef struct _NTCLIPFILEHEADER
{
    WORD wFileIdentifier;
    WORD wFormatCount;
} NTCLIPFILEHEADER;

typedef struct _NTCLIPFORMATHEADER
{
    DWORD dwFormatID;
    DWORD dwLenData;
    DWORD dwOffData;
    WCHAR szName[MAX_FMT_NAME_LEN];
} NTCLIPFORMATHEADER;

void ReadClipboardFile(LPCWSTR lpFileName);
void WriteClipboardFile(LPCWSTR lpFileName, WORD wFileIdentifier);
