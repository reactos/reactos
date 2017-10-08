/*
 * PROJECT:     ReactOS NLS to TXT Converter
 * LICENSE:     GNU General Public License Version 2.0 or any later version
 * FILE:        devutils/nlsedit/nls.c
 * COPYRIGHT:   Copyright 2016 Dmitry Chapyshev <dmitry@reactos.org>
 */

#include "precomp.h"

static VOID
NLS_InitCodePageTable(PUSHORT TableBase, PCPTABLEINFO CodePageTable)
{
    PNLS_FILE_HEADER NlsFileHeader;

    NlsFileHeader = (PNLS_FILE_HEADER)TableBase;

    /* Copy header fields first */
    CodePageTable->CodePage = NlsFileHeader->CodePage;
    CodePageTable->MaximumCharacterSize = NlsFileHeader->MaximumCharacterSize;
    CodePageTable->DefaultChar = NlsFileHeader->DefaultChar;
    CodePageTable->UniDefaultChar = NlsFileHeader->UniDefaultChar;
    CodePageTable->TransDefaultChar = NlsFileHeader->TransDefaultChar;
    CodePageTable->TransUniDefaultChar = NlsFileHeader->TransUniDefaultChar;

    CopyMemory(&CodePageTable->LeadByte, &NlsFileHeader->LeadByte, MAXIMUM_LEADBYTES);

    /* Offset to wide char table is after the header */
    CodePageTable->WideCharTable = TableBase + NlsFileHeader->HeaderSize + 1 +
                                   TableBase[NlsFileHeader->HeaderSize];

    /* Then multibyte table (256 wchars) follows */
    CodePageTable->MultiByteTable = TableBase + NlsFileHeader->HeaderSize + 1;

    /* Check the presence of glyph table (256 wchars) */
    if (!CodePageTable->MultiByteTable[256])
    {
        CodePageTable->DBCSRanges = CodePageTable->MultiByteTable + 256 + 1;
    }
    else
    {
        CodePageTable->DBCSRanges = CodePageTable->MultiByteTable + 256 + 1 + 256;
    }

    /* Is this double-byte code page? */
    if (*CodePageTable->DBCSRanges)
    {
        CodePageTable->DBCSCodePage = 1;
        CodePageTable->DBCSOffsets = CodePageTable->DBCSRanges + 1;
    }
    else
    {
        CodePageTable->DBCSCodePage = 0;
        CodePageTable->DBCSOffsets = NULL;
    }
}

PUSHORT
NLS_ReadFile(const WCHAR *pszFile, PCPTABLEINFO CodePageTable)
{
    HANDLE hFile;

    hFile = CreateFile(pszFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        PUSHORT pData;
        DWORD dwRead;
        DWORD dwFileSize;

        dwFileSize = GetFileSize(hFile, NULL);
        pData = malloc(dwFileSize);
        if (pData != NULL)
        {
            if (ReadFile(hFile, pData, dwFileSize, &dwRead, NULL) != FALSE)
            {
                NLS_InitCodePageTable(pData, CodePageTable);
                CloseHandle(hFile);

                return pData;
            }

            free(pData);
        }

        CloseHandle(hFile);
    }

    return NULL;
}

BOOL
NLS_IsDBCSCodePage(PCPTABLEINFO CodePageTable)
{
    return (BOOL)CodePageTable->DBCSCodePage;
}

BOOL
NLS_IsGlyphTablePresent(PCPTABLEINFO CodePageTable)
{
    return (CodePageTable->MultiByteTable[256]) ? TRUE : FALSE;
}

BOOL
NLS_IsDefaultCharForMB(PCPTABLEINFO CodePageTable, UCHAR Char)
{
    if (CodePageTable->MultiByteTable[Char] != CodePageTable->UniDefaultChar)
        return FALSE;

    return TRUE;
}

BOOL
NLS_IsDefaultCharForUnicode(PCPTABLEINFO CodePageTable, USHORT Char)
{
    USHORT CodePageChar;

    if (NLS_IsDBCSCodePage(CodePageTable))
    {
        PUSHORT MultiByteTable = (PUSHORT)CodePageTable->WideCharTable;
        CodePageChar = MultiByteTable[Char];
    }
    else
    {
        PUCHAR SingleByteTable = (PUCHAR)CodePageTable->WideCharTable;
        CodePageChar = SingleByteTable[Char];
    }

    if (CodePageChar != CodePageTable->DefaultChar)
        return FALSE;

    return TRUE;
}

USHORT
NLS_RecordsCountForMBTable(PCPTABLEINFO CodePageTable)
{
    USHORT CodePageChar;
    USHORT Count = 0;

    for (CodePageChar = 0; CodePageChar <= 0xFF; CodePageChar++)
    {
        if (!NLS_IsDefaultCharForMB(CodePageTable, CodePageChar))
            Count++;
    }

    return Count;
}

USHORT
NLS_RecordsCountForUnicodeTable(PCPTABLEINFO CodePageTable)
{
    ULONG UnicodeChar;
    USHORT Count = 0;

    for (UnicodeChar = 0; UnicodeChar <= 0xFFFF; UnicodeChar++)
    {
        if (!NLS_IsDefaultCharForUnicode(CodePageTable, UnicodeChar))
            Count++;
    }

    return Count;
}

USHORT
NLS_RecordsCountForGlyphTable(PCPTABLEINFO CodePageTable)
{
    USHORT Count = 0;

    if (NLS_IsGlyphTablePresent(CodePageTable))
    {
        PUSHORT GlyphTable = CodePageTable->MultiByteTable + 256 + 1;
        USHORT CodePageChar;

        for (CodePageChar = 0; CodePageChar <= 0xFF; CodePageChar++)
        {
            USHORT Char = GlyphTable[CodePageChar];

            if (Char != CodePageTable->UniDefaultChar)
                Count++;
        }
    }

    return Count;
}

USHORT
NLS_RecordsCountForDBCSTable(PCPTABLEINFO CodePageTable, UCHAR LeadByte)
{
    PUSHORT LeadByteInfo = CodePageTable->DBCSOffsets;
    USHORT CodePageChar;
    USHORT Count = 0;

    for (CodePageChar = 0; CodePageChar <= 0xFF; CodePageChar++)
    {
        USHORT Info = LeadByteInfo[LeadByte];

        if (Info && LeadByteInfo[Info + CodePageChar] != CodePageTable->UniDefaultChar)
        {
            Count++;
        }
    }

    return Count;
}
