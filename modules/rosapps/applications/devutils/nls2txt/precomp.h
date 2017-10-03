/*
 * PROJECT:     ReactOS NLS to TXT Converter
 * LICENSE:     GNU General Public License Version 2.0 or any later version
 * FILE:        devutils/nls2txt/precomp.h
 * COPYRIGHT:   Copyright 2016 Dmitry Chapyshev <dmitry@reactos.org>
 */

#ifndef __PRECOMP_H
#define __PRECOMP_H

#include <windows.h>
#include <strsafe.h>

#define MAX_STR_LEN   256

#define MAXIMUM_LEADBYTES   12

typedef struct _NLS_FILE_HEADER
{
    USHORT HeaderSize;
    USHORT CodePage;
    USHORT MaximumCharacterSize;
    USHORT DefaultChar;
    USHORT UniDefaultChar;
    USHORT TransDefaultChar;
    USHORT TransUniDefaultChar;
    UCHAR LeadByte[MAXIMUM_LEADBYTES];
} NLS_FILE_HEADER, *PNLS_FILE_HEADER;

typedef struct _CPTABLEINFO
{
    USHORT CodePage;
    USHORT MaximumCharacterSize;       /* 1 = SBCS, 2 = DBCS */
    USHORT DefaultChar;                /* Default MultiByte Character for the CP->Unicode conversion */
    USHORT UniDefaultChar;             /* Default Unicode Character for the CP->Unicode conversion */
    USHORT TransDefaultChar;           /* Default MultiByte Character for the Unicode->CP conversion */
    USHORT TransUniDefaultChar;        /* Default Unicode Character for the Unicode->CP conversion */
    USHORT DBCSCodePage;
    UCHAR LeadByte[MAXIMUM_LEADBYTES];
    PUSHORT MultiByteTable;             /* Table for CP->Unicode conversion */
    PVOID WideCharTable;                /* Table for Unicode->CP conversion */
    PUSHORT DBCSRanges;
    PUSHORT DBCSOffsets;
} CPTABLEINFO, *PCPTABLEINFO;

int WINAPI
GetUName(IN WORD wCharCode, OUT LPWSTR lpBuf);

/* nls.c */
PUSHORT
NLS_ReadFile(const WCHAR *pszFile, PCPTABLEINFO CodePageTable);

BOOL
NLS_IsDBCSCodePage(PCPTABLEINFO CodePageTable);

BOOL
NLS_IsGlyphTablePresent(PCPTABLEINFO CodePageTable);

BOOL
NLS_IsDefaultCharForMB(PCPTABLEINFO CodePageTable, UCHAR Char);

BOOL
NLS_IsDefaultCharForUnicode(PCPTABLEINFO CodePageTable, USHORT Char);

USHORT
NLS_RecordsCountForMBTable(PCPTABLEINFO CodePageTable);

USHORT
NLS_RecordsCountForUnicodeTable(PCPTABLEINFO CodePageTable);

USHORT
NLS_RecordsCountForGlyphTable(PCPTABLEINFO CodePageTable);

USHORT
NLS_RecordsCountForDBCSTable(PCPTABLEINFO CodePageTable, UCHAR LeadByte);

/* bestfit.c */
BOOL
BestFit_FromNLS(const WCHAR *pszNLSFile, const WCHAR *pszBestFitFile);

#endif
