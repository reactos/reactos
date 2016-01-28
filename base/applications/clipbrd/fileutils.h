/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Clipboard Viewer
 * FILE:            base/applications/clipbrd/fileutils.h
 * PURPOSE:         Clipboard file format helper functions.
 * PROGRAMMERS:     Ricardo Hanke
 */

#define CLIPBOARD_FORMAT_31 0xC350
#define CLIPBOARD_FORMAT_NT 0xC351
#define CLIPBOARD_FORMAT_BK 0xC352
#define MAX_FMT_NAME_LEN 79

typedef struct _CLIPBOARDFILEHEADER
{
    WORD wFileIdentifier;
    WORD wFormatCount;
} CLIPBOARDFILEHEADER;

typedef struct _CLIPBOARDFORMATHEADER
{
    DWORD dwFormatID;
    DWORD dwLenData;
    DWORD dwOffData;
    WCHAR szName[MAX_FMT_NAME_LEN];
} CLIPBOARDFORMATHEADER;

void ReadClipboardFile(LPCWSTR lpFileName);
void WriteClipboardFile(LPCWSTR lpFileName);
