/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for MultiByteToWideChar
 * PROGRAMMERS:     Mike "tamlin" Nordell
 *                  Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

/* NOTE: Tested on Win10. We follow Win10 in this function.
         Win10 might alter its design in future. */

/* TODO: Russian, French, Korean etc. codepages */

#define CP932   932     /* Japanese Shift_JIS (SJIS) codepage */

/* "Japanese" in Japanese UTF-8 */
static const char UTF8_Japanese[] = "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E";
/* "Japanese" in Japanese Shift_JIS */
static const char SJIS_Japanese[] = "\x93\xFA\x96\x7B\x8C\xEA";

#define MAX_BUFFER  8

/* test entry */
typedef struct ENTRY
{
    int LineNo;
    int Return;
    DWORD Error;
    UINT CodePage;
    DWORD Flags;
    const char *Src;
    int SrcLen;
    int DestLen;
    WCHAR CheckDest[MAX_BUFFER];
    int CheckLen;
    BOOL SamePointer;
} ENTRY;

static const ENTRY Entries[] =
{
    /* without buffer */
    { __LINE__, 1, 0xBEAF, CP_UTF8, 0, "a", 1 },
    { __LINE__, 2, 0xBEAF, CP_UTF8, 0, "a", 2 },
    { __LINE__, 2, 0xBEAF, CP_UTF8, MB_ERR_INVALID_CHARS, "a", 2 },
    /* negative length */
    { __LINE__, 2, 0xBEAF, CP_UTF8, 0, "a", -1 },
    { __LINE__, 2, 0xBEAF, CP_UTF8, 0, "a", -2 },
    { __LINE__, 2, 0xBEAF, CP_UTF8, MB_ERR_INVALID_CHARS, "a", -1 },
    { __LINE__, 2, 0xBEAF, CP_UTF8, MB_ERR_INVALID_CHARS, "a", -3 },
    /* with buffer */
    { __LINE__, 1, 0xBEAF, CP_UTF8, 0, "a", 1, 1, {'a', 0x7F7F}, 2 },
    { __LINE__, 2, 0xBEAF, CP_UTF8, 0, "a", 2, 4, {'a', 0, 0x7F7F}, 3 },
    { __LINE__, 2, 0xBEAF, CP_UTF8, MB_ERR_INVALID_CHARS, "a", 2, 4, {'a', 0, 0x7F7F}, 3 },
    /* short buffer */
    { __LINE__, 0, ERROR_INSUFFICIENT_BUFFER, CP_UTF8, 0, "a", 2, 1, {'a', 0x7F7F}, 2 },
    { __LINE__, 0, ERROR_INSUFFICIENT_BUFFER, CP_UTF8, 0, "a", 2, 1, {'a', 0x7F7F}, 2 },
    { __LINE__, 0, ERROR_INSUFFICIENT_BUFFER, CP_UTF8, MB_ERR_INVALID_CHARS, "a", 2, 1, {'a', 0x7F7F}, 2 },
    /* same pointer */
    { __LINE__, 0, ERROR_INVALID_PARAMETER, CP_UTF8, 0, "", 1, 1, { 0x7F7F }, 1, TRUE },
    { __LINE__, 0, ERROR_INVALID_PARAMETER, CP_UTF8, MB_ERR_INVALID_CHARS, "", 1, 1, { 0x7F7F }, 1, TRUE },
    /* invalid UTF-8 sequences without buffer */
    { __LINE__, 2, 0xBEAF, CP_UTF8, 0, "\xC0", 2 },
    { __LINE__, 4, 0xBEAF, CP_UTF8, 0, "\xC0\xC0\x80", 4 },
    { __LINE__, 3, 0xBEAF, CP_UTF8, 0, "\xE0\xC0", 3 },
    { __LINE__, 4, 0xBEAF, CP_UTF8, 0, "\xE0\x20\xC0", 4 },
    { __LINE__, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, "\xC0", 2 },
    { __LINE__, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, "\xC0\xC0\x80", 4 },
    { __LINE__, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, "\xE0\xC0", 3 },
    { __LINE__, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, "\xE0\x20\xC0", 4 },
    /* invalid UTF-8 sequences with buffer */
    { __LINE__, 2, 0xBEAF, CP_UTF8, 0, "\xC0", 2, 4, {0xFFFD, 0, 0x7F7F}, 3},
    { __LINE__, 4, 0xBEAF, CP_UTF8, 0, "\xC0\xC0\x80", 4, 5, {0xFFFD, 0xFFFD, 0xFFFD, 0, 0x7F7F}, 5 },
    { __LINE__, 3, 0xBEAF, CP_UTF8, 0, "\xE0\xC0", 3, 4, {0xFFFD, 0xFFFD, 0, 0x7F7F}, 4 },
    { __LINE__, 4, 0xBEAF, CP_UTF8, 0, "\xE0\x20\xC0", 4, 5, {0xFFFD, 0x0020, 0xFFFD, 0, 0x7F7F}, 5 },
    { __LINE__, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, "\xC0", 2, 4, {0xFFFD, 0, 0x7F7F}, 3 },
    { __LINE__, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, "\xC0\xC0\x80", 4, 5, {0xFFFD, 0xFFFD, 0xFFFD, 0, 0x7F7F}, 5 },
    { __LINE__, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, "\xE0\xC0", 3, 4, {0xFFFD, 0xFFFD, 0, 0x7F7F}, 4 },
    { __LINE__, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, "\xE0\x20\xC0", 4, 5, {0xFFFD, 0x0020, 0xFFFD, 0, 0x7F7F}, 5 },
    /* invalid UTF-8 sequences with short buffer */
    { __LINE__, 0, ERROR_INSUFFICIENT_BUFFER, CP_UTF8, 0, "\xC0", 2, 1, {0xFFFD, 0x7F7F}, 2},
    { __LINE__, 0, ERROR_INSUFFICIENT_BUFFER, CP_UTF8, 0, "\xC0\xC0\x80", 4, 1, {0xFFFD, 0x7F7F}, 2 },
    { __LINE__, 0, ERROR_INSUFFICIENT_BUFFER, CP_UTF8, 0, "\xE0\xC0", 3, 1, {0xFFFD, 0x7F7F}, 2 },
    { __LINE__, 0, ERROR_INSUFFICIENT_BUFFER, CP_UTF8, 0, "\xE0\x20\xC0", 4, 1, {0xFFFD, 0x7F7F}, 2 },
    { __LINE__, 0, ERROR_INSUFFICIENT_BUFFER, CP_UTF8, MB_ERR_INVALID_CHARS, "\xC0", 2, 1, {0xFFFD, 0x7F7F}, 2 },
    { __LINE__, 0, ERROR_INSUFFICIENT_BUFFER, CP_UTF8, MB_ERR_INVALID_CHARS, "\xC0\xC0\x80", 4, 1, {0xFFFD, 0x7F7F}, 2 },
    { __LINE__, 0, ERROR_INSUFFICIENT_BUFFER, CP_UTF8, MB_ERR_INVALID_CHARS, "\xE0\xC0", 3, 1, {0xFFFD, 0x7F7F}, 2 },
    { __LINE__, 0, ERROR_INSUFFICIENT_BUFFER, CP_UTF8, MB_ERR_INVALID_CHARS, "\xE0\x20\xC0", 4, 1, {0xFFFD, 0x7F7F}, 2 },
    /* Japanese UTF-8 without buffer */
    { __LINE__, 4, 0xBEAF, CP_UTF8, 0, UTF8_Japanese, sizeof(UTF8_Japanese) },
    { __LINE__, 4, 0xBEAF, CP_UTF8, MB_ERR_INVALID_CHARS, UTF8_Japanese, sizeof(UTF8_Japanese) },
    /* Japanese UTF-8 with buffer */
    { __LINE__, 4, 0xBEAF, CP_UTF8, 0, UTF8_Japanese, sizeof(UTF8_Japanese), 4, {0x65E5, 0x672C, 0x8A9E, 0, 0x7F7F}, 5 },
    { __LINE__, 4, 0xBEAF, CP_UTF8, MB_ERR_INVALID_CHARS, UTF8_Japanese, sizeof(UTF8_Japanese), 4, {0x65E5, 0x672C, 0x8A9E, 0, 0x7F7F}, 5 },
    /* Japanese UTF-8 with short buffer */
    { __LINE__, 0, ERROR_INSUFFICIENT_BUFFER, CP_UTF8, 0, UTF8_Japanese, sizeof(UTF8_Japanese), 1, {0x65E5, 0x7F7F}, 2 },
    { __LINE__, 0, ERROR_INSUFFICIENT_BUFFER, CP_UTF8, MB_ERR_INVALID_CHARS, UTF8_Japanese, sizeof(UTF8_Japanese), 1, {0x65E5, 0x7F7F}, 2 },
    /* Japanese UTF-8 truncated source */
    { __LINE__, 1, 0xBEAF, CP_UTF8, 0, UTF8_Japanese, 1, 4, {0xFFFD, 0x7F7F}, 2 },
    { __LINE__, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, UTF8_Japanese, 1, 4, {0xFFFD, 0x7F7F}, 2 },
    /* Japanese CP932 without buffer */
    { __LINE__, 4, 0xBEAF, CP932, 0, SJIS_Japanese, sizeof(SJIS_Japanese) },
    { __LINE__, 4, 0xBEAF, CP932, MB_ERR_INVALID_CHARS, SJIS_Japanese, sizeof(SJIS_Japanese) },
    /* Japanese CP932 with buffer */
    { __LINE__, 4, 0xBEAF, CP932, 0, SJIS_Japanese, sizeof(SJIS_Japanese), 4, {0x65E5, 0x672C, 0x8A9E, 0, 0x7F7F}, 5 },
    { __LINE__, 4, 0xBEAF, CP932, MB_ERR_INVALID_CHARS, SJIS_Japanese, sizeof(SJIS_Japanese), 4, {0x65E5, 0x672C, 0x8A9E, 0, 0x7F7F}, 5 },
    /* Japanese CP932 with short buffer */
    { __LINE__, 0, ERROR_INSUFFICIENT_BUFFER, CP932, 0, SJIS_Japanese, sizeof(SJIS_Japanese), 1, {0x65E5, 0x7F7F}, 2 },
    { __LINE__, 0, ERROR_INSUFFICIENT_BUFFER, CP932, MB_ERR_INVALID_CHARS, SJIS_Japanese, sizeof(SJIS_Japanese), 1, {0x65E5, 0x7F7F}, 2 },
    /* Japanese CP932 truncated source */
    { __LINE__, 1, 0xBEAF, CP932, 0, SJIS_Japanese, 1, 4, {0x30FB, 0x7F7F}, 2 },
    { __LINE__, 0, ERROR_NO_UNICODE_TRANSLATION, CP932, MB_ERR_INVALID_CHARS, SJIS_Japanese, 1, 4, {0x7F7F, 0x7F7F}, 2 },
};

static void TestEntry(const ENTRY *pEntry)
{
    int ret, i;
    WCHAR Buffer[MAX_BUFFER];
    DWORD Error;

    FillMemory(Buffer, sizeof(Buffer), 0x7F);
    SetLastError(0xBEAF);

    if (pEntry->DestLen == 0)
    {
        /* dest is NULL */
        ret = MultiByteToWideChar(pEntry->CodePage, pEntry->Flags, pEntry->Src,
                                  pEntry->SrcLen, NULL, 0);
    }
    else
    {
        ok(pEntry->DestLen >= pEntry->CheckLen - 1,
           "Line %d: DestLen was shorter than (CheckLen - 1)\n", pEntry->LineNo);

        if (pEntry->SamePointer)
        {
            /* src ptr == dest ptr */
            ret = MultiByteToWideChar(pEntry->CodePage, pEntry->Flags,
                                      (const char *)Buffer, pEntry->SrcLen,
                                      Buffer, pEntry->DestLen);
        }
        else
        {
            /* src ptr != dest ptr */
            ret = MultiByteToWideChar(pEntry->CodePage, pEntry->Flags,
                                      pEntry->Src, pEntry->SrcLen,
                                      Buffer, pEntry->DestLen);
        }
    }

    Error = GetLastError();

    /* check ret */
    ok(ret == pEntry->Return, "Line %d: ret expected %d, got %d\n",
       pEntry->LineNo, pEntry->Return, ret);

    /* check error code */
    ok(Error == pEntry->Error,
       "Line %d: Wrong last error. Expected %lu, got %lu\n",
       pEntry->LineNo, pEntry->Error, Error);

    if (pEntry->DestLen)
    {
        /* check buffer */
        for (i = 0; i < pEntry->CheckLen; ++i)
        {
            ok(Buffer[i] == pEntry->CheckDest[i], "Line %d: Buffer[%d] expected %d, got %d\n",
               pEntry->LineNo, i, pEntry->CheckDest[i], Buffer[i]);
        }
    }
}

START_TEST(MultiByteToWideChar)
{
    size_t i;
    for (i = 0; i < _countof(Entries); ++i)
    {
        TestEntry(&Entries[i]);
    }
}
