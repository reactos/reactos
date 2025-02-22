/*
 * PROJECT:     ReactOS NLS to TXT Converter
 * LICENSE:     GNU General Public License Version 2.0 or any later version
 * FILE:        devutils/nls2txt/bestfit.c
 * COPYRIGHT:   Copyright 2016 Dmitry Chapyshev <dmitry@reactos.org>
 */

#include "precomp.h"

static HANDLE
BestFit_CreateFile(const WCHAR *pszFile)
{
    DWORD dwBytesWritten;
    HANDLE hFile;

    hFile = CreateFileW(pszFile,
                        GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        /* Write UTF-8 BOM */
        WriteFile(hFile, "\xEF\xBB\xBF", 3, &dwBytesWritten, NULL);
    }

    return hFile;
}

static VOID
BestFit_CloseFile(HANDLE hFile)
{
    CloseHandle(hFile);
}

static CHAR*
UTF8fromUNICODE(const WCHAR *pszInput, PSIZE_T Size)
{
    ULONG Length;
    CHAR *pszOutput;

    if (!pszInput || !Size) return NULL;

    Length = WideCharToMultiByte(CP_UTF8, 0, pszInput, -1, NULL, 0, NULL, NULL);

    *Size = Length * sizeof(CHAR);

    pszOutput = (CHAR *) malloc(*Size);
    if (pszOutput)
    {
        WideCharToMultiByte(CP_UTF8, 0, pszInput, -1, pszOutput, Length, NULL, NULL);
    }

    return pszOutput;
}

static VOID
BestFit_Write(HANDLE hFile, const WCHAR *pszFormat, ...)
{
    LARGE_INTEGER FileSize;
    LARGE_INTEGER MoveTo;
    LARGE_INTEGER NewPos;
    DWORD dwBytesWritten;

    if (hFile == INVALID_HANDLE_VALUE)
        return;

    MoveTo.QuadPart = 0;
    if (!SetFilePointerEx(hFile, MoveTo, &NewPos, FILE_END))
        return;

    if (!GetFileSizeEx(hFile, &FileSize))
        return;

    if (LockFile(hFile, (DWORD_PTR)NewPos.QuadPart, 0, (DWORD_PTR)FileSize.QuadPart, 0))
    {
        WCHAR *pszString;
        CHAR *pszUtf8;
        va_list Args;
        SIZE_T Size;

        va_start(Args, pszFormat);

        Size = (_vscwprintf(pszFormat, Args) + 1) * sizeof(WCHAR);
        pszString = (WCHAR*) malloc(Size);

        if (!pszString)
        {
            UnlockFile(hFile, (DWORD_PTR)NewPos.QuadPart, 0, (DWORD_PTR)FileSize.QuadPart, 0);
            va_end(Args);
            return;
        }

        StringCbVPrintfW(pszString, Size, pszFormat, Args);
        va_end(Args);

        pszUtf8 = UTF8fromUNICODE(pszString, &Size);
        if (pszUtf8)
        {
            WriteFile(hFile, pszUtf8, Size - sizeof(CHAR), &dwBytesWritten, NULL);
            free(pszUtf8);
        }

        free(pszString);

        UnlockFile(hFile, (DWORD_PTR)NewPos.QuadPart, 0, (DWORD_PTR)FileSize.QuadPart, 0);
    }
}

BOOL
BestFit_FromNLS(const WCHAR *pszNLSFile, const WCHAR *pszBestFitFile)
{
    CPTABLEINFO CodePageTable;
    PUSHORT CodePage;
    HANDLE hFile;
    USHORT CodePageChar;
    ULONG UnicodeChar;

    CodePage = NLS_ReadFile(pszNLSFile, &CodePageTable);
    if (CodePage == NULL)
        return FALSE;

    hFile = BestFit_CreateFile(pszBestFitFile);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        free(CodePage);
        return FALSE;
    }

    /* The only field is the decimal windows code page number for this code page. */
    BestFit_Write(hFile, L"CODEPAGE %u\r\n\r\n", CodePageTable.CodePage);

    BestFit_Write(hFile,
                  L"CPINFO %u 0x%02X 0x%04X\r\n\r\n",
                  /* "1" for a single byte code page, "2" for a double byte code page */
                  CodePageTable.MaximumCharacterSize,
                  /* Replacement characters for unassigned Unicode code points when
                     written to this code page */
                  CodePageTable.DefaultChar,
                  /* Replacement characters for illegal or unassigned code page values
                     when converting to Unicode. */
                  CodePageTable.UniDefaultChar);

    /* This field contains the number of following records of code page to Unicode mappings. */
    BestFit_Write(hFile, L"MBTABLE %u\r\n\r\n", NLS_RecordsCountForMBTable(&CodePageTable));

    for (CodePageChar = 0; CodePageChar <= 0xFF; CodePageChar++)
    {
        if (!NLS_IsDefaultCharForMB(&CodePageTable, CodePageChar))
        {
            WCHAR szCharName[MAX_STR_LEN] = { 0 };

            GetUName(CodePageTable.MultiByteTable[CodePageChar], szCharName);

            BestFit_Write(hFile,
                          L"0x%02X 0x%04X ;%s\r\n",
                          CodePageChar,
                          CodePageTable.MultiByteTable[CodePageChar],
                          szCharName);
        }
    }

    BestFit_Write(hFile, L"\r\n");

    if (NLS_IsGlyphTablePresent(&CodePageTable))
    {
        PUSHORT GlyphTable = CodePageTable.MultiByteTable + 256 + 1;

        BestFit_Write(hFile, L"GLYPHTABLE %u\r\n\r\n", NLS_RecordsCountForGlyphTable(&CodePageTable));

        for (CodePageChar = 0; CodePageChar <= 0xFF; CodePageChar++)
        {
            if (CodePageChar != CodePageTable.UniDefaultChar)
            {
                WCHAR szCharName[MAX_STR_LEN] = { 0 };

                GetUName(GlyphTable[CodePageChar], szCharName);

                BestFit_Write(hFile,
                              L"0x%02X 0x%04X ;%s\r\n",
                              CodePageChar,
                              GlyphTable[CodePageChar],
                              szCharName);
            }
        }

        BestFit_Write(hFile, L"\r\n");
    }

    if (NLS_IsDBCSCodePage(&CodePageTable))
    {
        PUSHORT LeadByteRanges = (PUSHORT)&CodePageTable.LeadByte[0];
        USHORT Index;
        USHORT LeadByte;

        BestFit_Write(hFile,
                      L"DBCSRANGE %u ;%u DBCS Lead Byte Ranges\r\n\r\n",
                      CodePageTable.DBCSRanges[0],
                      CodePageTable.DBCSRanges[0]);

        for (Index = 0; Index < MAXIMUM_LEADBYTES / 2; Index++)
        {
            if (!LeadByteRanges[Index])
                continue;

            BestFit_Write(hFile,
                          L"0x%X 0x%X ;Lead Byte Range %u\r\n\r\n",
                          LOBYTE(LeadByteRanges[Index]),
                          HIBYTE(LeadByteRanges[Index]),
                          Index + 1);

            for (LeadByte = LOBYTE(LeadByteRanges[Index]);
                 LeadByte <= HIBYTE(LeadByteRanges[Index]);
                 LeadByte++)
            {
                PUSHORT LeadByteInfo = CodePageTable.DBCSOffsets;

                BestFit_Write(hFile,
                              L"DBCSTABLE %u ;Range = %u, LeadByte = 0x%02X\r\n\r\n",
                              NLS_RecordsCountForDBCSTable(&CodePageTable, LeadByte),
                              Index + 1,
                              LeadByte);

                for (CodePageChar = 0; CodePageChar <= 0xFF; CodePageChar++)
                {
                    USHORT Info = LeadByteInfo[LeadByte];

                    if (Info && LeadByteInfo[Info + CodePageChar] != CodePageTable.UniDefaultChar)
                    {
                        BestFit_Write(hFile,
                                      L"0x%02X 0x%04X\r\n",
                                      CodePageChar,
                                      LeadByteInfo[Info + CodePageChar]);
                    }
                }

                BestFit_Write(hFile, L"\r\n");
            }
        }
    }

    /* This field contains the number of records of Unicode to byte mappings. */
    BestFit_Write(hFile, L"WCTABLE %u\r\n\r\n", NLS_RecordsCountForUnicodeTable(&CodePageTable));

    for (UnicodeChar = 0; UnicodeChar <= 0xFFFF; UnicodeChar++)
    {
        if (!NLS_IsDefaultCharForUnicode(&CodePageTable, UnicodeChar))
        {
            WCHAR szCharName[MAX_STR_LEN] = { 0 };

            GetUName(UnicodeChar, szCharName);

            if (NLS_IsDBCSCodePage(&CodePageTable))
            {
                PUSHORT MultiByteTable = (PUSHORT)CodePageTable.WideCharTable;

                BestFit_Write(hFile,
                              L"0x%04X 0x%04X ;%s\r\n",
                              UnicodeChar,
                              MultiByteTable[UnicodeChar],
                              szCharName);
            }
            else
            {
                PUCHAR SingleByteTable = (PUCHAR)CodePageTable.WideCharTable;

                BestFit_Write(hFile,
                              L"0x%04X 0x%02X ;%s\r\n",
                              UnicodeChar,
                              SingleByteTable[UnicodeChar],
                              szCharName);
            }
        }
    }

    /* This tag marks the end of the code page data. Anything after this marker is ignored. */
    BestFit_Write(hFile, L"\r\nENDCODEPAGE\r\n");

    BestFit_CloseFile(hFile);
    free(CodePage);

    return TRUE;
}
