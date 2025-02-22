/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for MultiByteToWideChar
 * PROGRAMMERS:     Mike "tamlin" Nordell
 *                  Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include <versionhelpers.h>

static ULONG OsVersion;

/* TODO: Russian, French, Korean etc. codepages */

#define CP932   932     /* Japanese Shift_JIS (SJIS) codepage */

/* "Japanese" in Japanese UTF-8 */
static const char UTF8_Japanese[] = "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E";
/* "Japanese" in Japanese Shift_JIS */
static const char SJIS_Japanese[] = "\x93\xFA\x96\x7B\x8C\xEA";

#define MAX_BUFFER  10

/* test entry */
typedef struct ENTRY
{
    int LineNo;
    ULONG VersionRange[2];
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
    { __LINE__, {0x000,0xA00}, 1, 0xBEAF, CP_UTF8, 0, "a", 1 },
    { __LINE__, {0x000,0xA00}, 2, 0xBEAF, CP_UTF8, 0, "a", 2 },
    { __LINE__, {0x000,0xA00}, 2, 0xBEAF, CP_UTF8, MB_ERR_INVALID_CHARS, "a", 2 },
    /* negative length */
    { __LINE__, {0x000,0xA00}, 2, 0xBEAF, CP_UTF8, 0, "a", -1 },
    { __LINE__, {0x000,0xA00}, 2, 0xBEAF, CP_UTF8, 0, "a", -2 },
    { __LINE__, {0x000,0xA00}, 2, 0xBEAF, CP_UTF8, MB_ERR_INVALID_CHARS, "a", -1 },
    { __LINE__, {0x000,0xA00}, 2, 0xBEAF, CP_UTF8, MB_ERR_INVALID_CHARS, "a", -3 },
    /* with buffer */
    { __LINE__, {0x000,0xA00}, 1, 0xBEAF, CP_UTF8, 0, "a", 1, 1, {'a', 0x7F7F}, 2 },
    { __LINE__, {0x000,0xA00}, 2, 0xBEAF, CP_UTF8, 0, "a", 2, 4, {'a', 0, 0x7F7F}, 3 },
    { __LINE__, {0x000,0xA00}, 2, 0xBEAF, CP_UTF8, MB_ERR_INVALID_CHARS, "a", 2, 4, {'a', 0, 0x7F7F}, 3 },
    /* short buffer */
    { __LINE__, {0x000,0xA00}, 0, ERROR_INSUFFICIENT_BUFFER, CP_UTF8, 0, "a", 2, 1, {'a', 0x7F7F}, 2 },
    { __LINE__, {0x000,0xA00}, 0, ERROR_INSUFFICIENT_BUFFER, CP_UTF8, 0, "a", 2, 1, {'a', 0x7F7F}, 2 },
    { __LINE__, {0x000,0xA00}, 0, ERROR_INSUFFICIENT_BUFFER, CP_UTF8, MB_ERR_INVALID_CHARS, "a", 2, 1, {'a', 0x7F7F}, 2 },
    /* same pointer */
    { __LINE__, {0x000,0xA00}, 0, ERROR_INVALID_PARAMETER, CP_UTF8, 0, "", 1, 1, { 0x7F7F }, 1, TRUE },
    { __LINE__, {0x000,0xA00}, 0, ERROR_INVALID_PARAMETER, CP_UTF8, MB_ERR_INVALID_CHARS, "", 1, 1, { 0x7F7F }, 1, TRUE },
    /* invalid UTF-8 sequences without buffer */
    { __LINE__, {0x000,0x502}, 1, 0xBEAF, CP_UTF8, 0, "\xC0", 2 },
    { __LINE__, {0x600,0xA00}, 2, 0xBEAF, CP_UTF8, 0, "\xC0", 2 },
    { __LINE__, {0x000,0x502}, 1, 0xBEAF, CP_UTF8, 0, "\xC0\xC0\x80", 4 },
    { __LINE__, {0x600,0xA00}, 4, 0xBEAF, CP_UTF8, 0, "\xC0\xC0\x80", 4 },
    { __LINE__, {0x000,0x502}, 1, 0xBEAF, CP_UTF8, 0, "\xE0\xC0", 3 },
    { __LINE__, {0x600,0xA00}, 3, 0xBEAF, CP_UTF8, 0, "\xE0\xC0", 3 },
    { __LINE__, {0x000,0x502}, 2, 0xBEAF, CP_UTF8, 0, "\xE0\x20\xC0", 4 },
    { __LINE__, {0x600,0xA00}, 4, 0xBEAF, CP_UTF8, 0, "\xE0\x20\xC0", 4 },
    { __LINE__, {0x000,0x502}, 1, 0xBEAF, CP_UTF8, 0, "\x82\xA0\x82\xA2", -1 },
    { __LINE__, {0x600,0xA00}, 5, 0xBEAF, CP_UTF8, 0, "\x82\xA0\x82\xA2", -1 },
    { __LINE__, {0x000,0x502}, 1, 0xBEAF, CP_UTF8, 0, "\x82\xA0\x82\xA2\x82", -1 },
    { __LINE__, {0x600,0xA00}, 6, 0xBEAF, CP_UTF8, 0, "\x82\xA0\x82\xA2\x82", -1 },
    { __LINE__, {0x000,0xA00}, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, "\xC0", 2 },
    { __LINE__, {0x000,0xA00}, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, "\xC0\xC0\x80", 4 },
    { __LINE__, {0x000,0xA00}, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, "\xE0\xC0", 3 },
    { __LINE__, {0x000,0xA00}, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, "\xE0\x20\xC0", 4 },
    { __LINE__, {0x000,0xA00}, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, "\x82\xA0\x82\xA2", -1 },
    { __LINE__, {0x000,0xA00}, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, "\x82\xA0\x82\xA2\x82", -1 },
    /* invalid UTF-8 sequences with buffer */
    { __LINE__, {0x000,0x502}, 1, 0xBEAF, CP_UTF8, 0, "\xC0", 2, 4, {0x00, 0x7F7F, 0x7F7F}, 3},
    { __LINE__, {0x600,0xA00}, 2, 0xBEAF, CP_UTF8, 0, "\xC0", 2, 4, {0xFFFD, 0, 0x7F7F}, 3},
    { __LINE__, {0x000,0x502}, 1, 0xBEAF, CP_UTF8, 0, "\xC0\xC0\x80", 4, 5, {0x00, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F}, 5 },
    { __LINE__, {0x600,0xA00}, 4, 0xBEAF, CP_UTF8, 0, "\xC0\xC0\x80", 4, 5, {0xFFFD, 0xFFFD, 0xFFFD, 0, 0x7F7F}, 5 },
    { __LINE__, {0x000,0x502}, 1, 0xBEAF, CP_UTF8, 0, "\xE0\xC0", 3, 4, {0, 0x7F7F, 0x7F7F, 0x7F7F}, 4 },
    { __LINE__, {0x600,0xA00}, 3, 0xBEAF, CP_UTF8, 0, "\xE0\xC0", 3, 4, {0xFFFD, 0xFFFD, 0, 0x7F7F}, 4 },
    { __LINE__, {0x000,0x502}, 2, 0xBEAF, CP_UTF8, 0, "\xE0\x20\xC0", 4, 5, {0x0020, 0, 0x7F7F, 0x7F7F, 0x7F7F}, 5 },
    { __LINE__, {0x600,0xA00}, 4, 0xBEAF, CP_UTF8, 0, "\xE0\x20\xC0", 4, 5, {0xFFFD, 0x0020, 0xFFFD, 0, 0x7F7F}, 5 },
    { __LINE__, {0x000,0x502}, 1, 0xBEAF, CP_UTF8, 0, "\x82\xA0\x82\xA2", -1, 5, {0, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F}, 5 },
    { __LINE__, {0x600,0xA00}, 5, 0xBEAF, CP_UTF8, 0, "\x82\xA0\x82\xA2", -1, 5, {0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0}, 5 },
    { __LINE__, {0x000,0x502}, 1, 0xBEAF, CP_UTF8, 0, "\x82\xA0\x82\xA2\x82", -1, 8, {0, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F}, 6 },
    { __LINE__, {0x600,0xA00}, 6, 0xBEAF, CP_UTF8, 0, "\x82\xA0\x82\xA2\x82", -1, 8, {0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0}, 6 },
    { __LINE__, {0x000,0x502}, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, "\xC0", 2, 4, {0x7F7F, 0x7F7F, 0x7F7F}, 3 },
    { __LINE__, {0x600,0xA00}, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, "\xC0", 2, 4, {0xFFFD, 0, 0x7F7F}, 3 },
    { __LINE__, {0x000,0x502}, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, "\xC0\xC0\x80", 4, 5, {0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F}, 5 },
    { __LINE__, {0x600,0xA00}, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, "\xC0\xC0\x80", 4, 5, {0xFFFD, 0xFFFD, 0xFFFD, 0, 0x7F7F}, 5 },
    { __LINE__, {0x000,0x502}, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, "\xE0\xC0", 3, 4, {0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F}, 4 },
    { __LINE__, {0x600,0xA00}, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, "\xE0\xC0", 3, 4, {0xFFFD, 0xFFFD, 0, 0x7F7F}, 4 },
    { __LINE__, {0x000,0x502}, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, "\xE0\x20\xC0", 4, 5, {0x0020, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F}, 5 },
    { __LINE__, {0x600,0xA00}, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, "\xE0\x20\xC0", 4, 5, {0xFFFD, 0x0020, 0xFFFD, 0, 0x7F7F}, 5 },
    { __LINE__, {0x000,0x502}, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, "\x82\xA0\x82\xA2", -1, 6, {0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F}, 6 },
    { __LINE__, {0x600,0xA00}, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, "\x82\xA0\x82\xA2", -1, 6, {0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0, 0x7F7F}, 6 },
    { __LINE__, {0x000,0x502}, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, "\x82\xA0\x82\xA2\x82", -1, 7, {0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F}, 7 },
    { __LINE__, {0x600,0xA00}, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, "\x82\xA0\x82\xA2\x82", -1, 7, {0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0, 0x7F7F}, 7 },
    /* invalid UTF-8 sequences with short buffer */
    { __LINE__, {0x000,0x502}, 1, 0xBEAF, CP_UTF8, 0, "\xC0", 2, 1, {0, 0x7F7F}, 2},
    { __LINE__, {0x600,0xA00}, 0, ERROR_INSUFFICIENT_BUFFER, CP_UTF8, 0, "\xC0", 2, 1, {0xFFFD, 0x7F7F}, 2},
    { __LINE__, {0x000,0x502}, 1, 0xBEAF, CP_UTF8, 0, "\xC0\xC0\x80", 4, 1, {0, 0x7F7F}, 2 },
    { __LINE__, {0x600,0xA00}, 0, ERROR_INSUFFICIENT_BUFFER, CP_UTF8, 0, "\xC0\xC0\x80", 4, 1, {0xFFFD, 0x7F7F}, 2 },
    { __LINE__, {0x000,0x502}, 1, 0xBEAF, CP_UTF8, 0, "\xE0\xC0", 3, 1, {0, 0x7F7F}, 2 },
    { __LINE__, {0x600,0xA00}, 0, ERROR_INSUFFICIENT_BUFFER, CP_UTF8, 0, "\xE0\xC0", 3, 1, {0xFFFD, 0x7F7F}, 2 },
    { __LINE__, {0x000,0x502}, 0, ERROR_INSUFFICIENT_BUFFER, CP_UTF8, 0, "\xE0\x20\xC0", 4, 1, {0x0020, 0x7F7F}, 2 },
    { __LINE__, {0x600,0xA00}, 0, ERROR_INSUFFICIENT_BUFFER, CP_UTF8, 0, "\xE0\x20\xC0", 4, 1, {0xFFFD, 0x7F7F}, 2 },
    { __LINE__, {0x000,0x502}, 1, 0xBEAF, CP_UTF8, 0, "\x82\xA0\x82\xA2", -1, 1, {0, 0x7F7F}, 2 },
    { __LINE__, {0x600,0xA00}, 0, ERROR_INSUFFICIENT_BUFFER, CP_UTF8, 0, "\x82\xA0\x82\xA2", -1, 1, {0xFFFD, 0x7F7F}, 2 },
    { __LINE__, {0x000,0x502}, 1, 0xBEAF, CP_UTF8, 0, "\x82\xA0\x82\xA2\x82", -1, 1, {0, 0x7F7F}, 2 },
    { __LINE__, {0x600,0xA00}, 0, ERROR_INSUFFICIENT_BUFFER, CP_UTF8, 0, "\x82\xA0\x82\xA2\x82", -1, 1, {0xFFFD, 0x7F7F}, 2 },
    { __LINE__, {0x000,0x502}, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, "\xC0", 2, 1, {0x7F7F, 0x7F7F}, 2 },
    { __LINE__, {0x600,0xA00}, 0, ERROR_INSUFFICIENT_BUFFER, CP_UTF8, MB_ERR_INVALID_CHARS, "\xC0", 2, 1, {0xFFFD, 0x7F7F}, 2 },
    { __LINE__, {0x000,0x502}, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, "\xC0\xC0\x80", 4, 1, {0x7F7F, 0x7F7F}, 2 },
    { __LINE__, {0x600,0xA00}, 0, ERROR_INSUFFICIENT_BUFFER, CP_UTF8, MB_ERR_INVALID_CHARS, "\xC0\xC0\x80", 4, 1, {0xFFFD, 0x7F7F}, 2 },
    { __LINE__, {0x000,0x502}, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, "\xE0\xC0", 3, 1, {0x7F7F, 0x7F7F}, 2 },
    { __LINE__, {0x600,0xA00}, 0, ERROR_INSUFFICIENT_BUFFER, CP_UTF8, MB_ERR_INVALID_CHARS, "\xE0\xC0", 3, 1, {0xFFFD, 0x7F7F}, 2 },
    { __LINE__, {0x502,0x502}, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, "\xE0\x20\xC0", 4, 1, {0x0020, 0x7F7F}, 2 },
    { __LINE__, {0x600,0xA00}, 0, ERROR_INSUFFICIENT_BUFFER, CP_UTF8, MB_ERR_INVALID_CHARS, "\xE0\x20\xC0", 4, 1, {0xFFFD, 0x7F7F}, 2 },
    { __LINE__, {0x000,0x502}, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, "\x82\xA0\x82\xA2", -1, 1, {0x7F7F, 0x7F7F}, 2 },
    { __LINE__, {0x600,0xA00}, 0, ERROR_INSUFFICIENT_BUFFER, CP_UTF8, MB_ERR_INVALID_CHARS, "\x82\xA0\x82\xA2", -1, 1, {0xFFFD, 0x7F7F}, 2 },
    { __LINE__, {0x000,0x502}, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, "\x82\xA0\x82\xA2\x82", -1, 1, {0x7F7F, 0x7F7F}, 2 },
    { __LINE__, {0x600,0xA00}, 0, ERROR_INSUFFICIENT_BUFFER, CP_UTF8, MB_ERR_INVALID_CHARS, "\x82\xA0\x82\xA2\x82", -1, 1, {0xFFFD, 0x7F7F}, 2 },
    /* Japanese UTF-8 without buffer */
    { __LINE__, {0x000,0xA00}, 4, 0xBEAF, CP_UTF8, 0, UTF8_Japanese, sizeof(UTF8_Japanese) },
    { __LINE__, {0x000,0xA00}, 4, 0xBEAF, CP_UTF8, MB_ERR_INVALID_CHARS, UTF8_Japanese, sizeof(UTF8_Japanese) },
    /* Japanese UTF-8 with buffer */
    { __LINE__, {0x000,0xA00}, 4, 0xBEAF, CP_UTF8, 0, UTF8_Japanese, sizeof(UTF8_Japanese), 4, {0x65E5, 0x672C, 0x8A9E, 0, 0x7F7F}, 5 },
    { __LINE__, {0x000,0xA00}, 4, 0xBEAF, CP_UTF8, MB_ERR_INVALID_CHARS, UTF8_Japanese, sizeof(UTF8_Japanese), 4, {0x65E5, 0x672C, 0x8A9E, 0, 0x7F7F}, 5 },
    /* Japanese UTF-8 with short buffer */
    { __LINE__, {0x000,0xA00}, 0, ERROR_INSUFFICIENT_BUFFER, CP_UTF8, 0, UTF8_Japanese, sizeof(UTF8_Japanese), 1, {0x65E5, 0x7F7F}, 2 },
    { __LINE__, {0x000,0xA00}, 0, ERROR_INSUFFICIENT_BUFFER, CP_UTF8, MB_ERR_INVALID_CHARS, UTF8_Japanese, sizeof(UTF8_Japanese), 1, {0x65E5, 0x7F7F}, 2 },
    /* Japanese UTF-8 truncated source */
    { __LINE__, {0x502,0x502}, 0, ERROR_SUCCESS, CP_UTF8, 0, UTF8_Japanese, 1, 4, {0x7F7F, 0x7F7F}, 2 },
    { __LINE__, {0x600,0xA00}, 1, 0xBEAF, CP_UTF8, 0, UTF8_Japanese, 1, 4, {0xFFFD, 0x7F7F}, 2 },
    { __LINE__, {0x502,0x502}, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, UTF8_Japanese, 1, 4, {0x7F7F, 0x7F7F}, 2 },
    { __LINE__, {0x600,0xA00}, 0, ERROR_NO_UNICODE_TRANSLATION, CP_UTF8, MB_ERR_INVALID_CHARS, UTF8_Japanese, 1, 4, {0xFFFD, 0x7F7F}, 2 },
    /* Japanese CP932 without buffer */
    { __LINE__, {0x000,0xA00}, 4, 0xBEAF, CP932, 0, SJIS_Japanese, sizeof(SJIS_Japanese) },
    { __LINE__, {0x000,0xA00}, 4, 0xBEAF, CP932, MB_ERR_INVALID_CHARS, SJIS_Japanese, sizeof(SJIS_Japanese) },
    /* Japanese CP932 with buffer */
    { __LINE__, {0x000,0xA00}, 4, 0xBEAF, CP932, 0, SJIS_Japanese, sizeof(SJIS_Japanese), 4, {0x65E5, 0x672C, 0x8A9E, 0, 0x7F7F}, 5 },
    { __LINE__, {0x000,0xA00}, 4, 0xBEAF, CP932, MB_ERR_INVALID_CHARS, SJIS_Japanese, sizeof(SJIS_Japanese), 4, {0x65E5, 0x672C, 0x8A9E, 0, 0x7F7F}, 5 },
    /* Japanese CP932 with short buffer */
    { __LINE__, {0x000,0xA00}, 0, ERROR_INSUFFICIENT_BUFFER, CP932, 0, SJIS_Japanese, sizeof(SJIS_Japanese), 1, {0x65E5, 0x7F7F}, 2 },
    { __LINE__, {0x000,0xA00}, 0, ERROR_INSUFFICIENT_BUFFER, CP932, MB_ERR_INVALID_CHARS, SJIS_Japanese, sizeof(SJIS_Japanese), 1, {0x65E5, 0x7F7F}, 2 },
    /* Japanese CP932 truncated source */
    { __LINE__, {0x000,0x502}, 1, 0xBEAF, CP932, 0, SJIS_Japanese, 1, 4, {0, 0x7F7F}, 2 },
    { __LINE__, {0x600,0xA00}, 1, 0xBEAF, CP932, 0, SJIS_Japanese, 1, 4, {0x30FB, 0x7F7F}, 2 },
    { __LINE__, {0x000,0xA00}, 0, ERROR_NO_UNICODE_TRANSLATION, CP932, MB_ERR_INVALID_CHARS, SJIS_Japanese, 1, 4, {0x7F7F, 0x7F7F}, 2 },
    /* invalid 5-byte UTF-8 sequences */
    { __LINE__, {0x502,0x502}, 0, ERROR_SUCCESS, CP_UTF8, 0, "\xF8\xA3\xA3\xA3\xA3", 1, 8, {0x7F7F, 0x7F7F}, 2 },
    { __LINE__, {0x600,0xA00}, 1, 0xBEAF, CP_UTF8, 0, "\xF8\xA3\xA3\xA3\xA3", 1, 8, {0xFFFD, 0x7F7F}, 2 },
    { __LINE__, {0x502,0x502}, 0, ERROR_SUCCESS, CP_UTF8, 0, "\xF8\xA3\xA3\xA3\xA3", 2, 8, {0x7F7F, 0x7F7F, 0x7F7F}, 3 },
    { __LINE__, {0x600,0xA00}, 2, 0xBEAF, CP_UTF8, 0, "\xF8\xA3\xA3\xA3\xA3", 2, 8, {0xFFFD, 0xFFFD, 0x7F7F}, 3 },
    { __LINE__, {0x502,0x502}, 0, ERROR_SUCCESS, CP_UTF8, 0, "\xF8\xA3\xA3\xA3\xA3", 3, 8, {0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F}, 4 },
    { __LINE__, {0x600,0xA00}, 3, 0xBEAF, CP_UTF8, 0, "\xF8\xA3\xA3\xA3\xA3", 3, 8, {0xFFFD, 0xFFFD, 0xFFFD, 0x7F7F}, 4 },
    { __LINE__, {0x502,0x502}, 0, ERROR_SUCCESS, CP_UTF8, 0, "\xF8\xA3\xA3\xA3\xA3", 4, 8, {0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F}, 5 },
    { __LINE__, {0x600,0xA00}, 4, 0xBEAF, CP_UTF8, 0, "\xF8\xA3\xA3\xA3\xA3", 4, 8, {0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x7F7F}, 5 },
    { __LINE__, {0x502,0x502}, 0, ERROR_SUCCESS, CP_UTF8, 0, "\xF8\xA3\xA3\xA3\xA3", 5, 8, {0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F}, 6 },
    { __LINE__, {0x600,0xA00}, 5, 0xBEAF, CP_UTF8, 0, "\xF8\xA3\xA3\xA3\xA3", 5, 8, {0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x7F7F}, 6 },
    { __LINE__, {0x000,0x502}, 1, 0xBEAF, CP_UTF8, 0, "\xF8\xA3\xA3\xA3\xA3", 6, 8, {0, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F}, 6 },
    { __LINE__, {0x600,0xA00}, 6, 0xBEAF, CP_UTF8, 0, "\xF8\xA3\xA3\xA3\xA3", 6, 8, {0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0}, 6 },
    /* invalid 6-byte UTF-8 sequences */
    { __LINE__, {0x502,0x502}, 0, ERROR_SUCCESS, CP_UTF8, 0, "\xFC\xA3\xA3\xA3\xA3\xA3", 1, 8, {0x7F7F, 0x7F7F}, 2 },
    { __LINE__, {0x600,0xA00}, 1, 0xBEAF, CP_UTF8, 0, "\xFC\xA3\xA3\xA3\xA3\xA3", 1, 8, {0xFFFD, 0x7F7F}, 2 },
    { __LINE__, {0x502,0x502}, 0, ERROR_SUCCESS, CP_UTF8, 0, "\xFC\xA3\xA3\xA3\xA3\xA3", 2, 8, {0x7F7F, 0x7F7F, 0x7F7F}, 3 },
    { __LINE__, {0x600,0xA00}, 2, 0xBEAF, CP_UTF8, 0, "\xFC\xA3\xA3\xA3\xA3\xA3", 2, 8, {0xFFFD, 0xFFFD, 0x7F7F}, 3 },
    { __LINE__, {0x502,0x502}, 0, ERROR_SUCCESS, CP_UTF8, 0, "\xFC\xA3\xA3\xA3\xA3\xA3", 3, 8, {0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F}, 4 },
    { __LINE__, {0x600,0xA00}, 3, 0xBEAF, CP_UTF8, 0, "\xFC\xA3\xA3\xA3\xA3\xA3", 3, 8, {0xFFFD, 0xFFFD, 0xFFFD, 0x7F7F}, 4 },
    { __LINE__, {0x502,0x502}, 0, ERROR_SUCCESS, CP_UTF8, 0, "\xFC\xA3\xA3\xA3\xA3\xA3", 4, 8, {0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F}, 5 },
    { __LINE__, {0x600,0xA00}, 4, 0xBEAF, CP_UTF8, 0, "\xFC\xA3\xA3\xA3\xA3\xA3", 4, 8, {0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x7F7F}, 5 },
    { __LINE__, {0x502,0x502}, 0, ERROR_SUCCESS, CP_UTF8, 0, "\xFC\xA3\xA3\xA3\xA3\xA3", 5, 8, {0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F}, 6 },
    { __LINE__, {0x600,0xA00}, 5, 0xBEAF, CP_UTF8, 0, "\xFC\xA3\xA3\xA3\xA3\xA3", 5, 8, {0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x7F7F}, 6 },
    { __LINE__, {0x502,0x502}, 0, ERROR_SUCCESS, CP_UTF8, 0, "\xFC\xA3\xA3\xA3\xA3\xA3", 6, 8, {0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F}, 7 },
    { __LINE__, {0x600,0xA00}, 6, 0xBEAF, CP_UTF8, 0, "\xFC\xA3\xA3\xA3\xA3\xA3", 6, 8, {0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x7F7F}, 7 },
    { __LINE__, {0x000,0x502}, 1, 0xBEAF, CP_UTF8, 0, "\xFC\xA3\xA3\xA3\xA3\xA3", 7, 8, {0, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F}, 7 },
    { __LINE__, {0x600,0xA00}, 7, 0xBEAF, CP_UTF8, 0, "\xFC\xA3\xA3\xA3\xA3\xA3", 7, 8, {0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0}, 7 },
    /* invalid 7-byte UTF-8 sequences */
    { __LINE__, {0x502,0x502}, 0, ERROR_SUCCESS, CP_UTF8, 0, "\xFE\xA3\xA3\xA3\xA3\xA3\xA3", 1, 8, {0x7F7F, 0x7F7F}, 2 },
    { __LINE__, {0x600,0xA00}, 1, 0xBEAF, CP_UTF8, 0, "\xFE\xA3\xA3\xA3\xA3\xA3\xA3", 1, 8, {0xFFFD, 0x7F7F}, 2 },
    { __LINE__, {0x502,0x502}, 0, ERROR_SUCCESS, CP_UTF8, 0, "\xFE\xA3\xA3\xA3\xA3\xA3\xA3", 2, 8, {0x7F7F, 0x7F7F, 0x7F7F}, 3 },
    { __LINE__, {0x600,0xA00}, 2, 0xBEAF, CP_UTF8, 0, "\xFE\xA3\xA3\xA3\xA3\xA3\xA3", 2, 8, {0xFFFD, 0xFFFD, 0x7F7F}, 3 },
    { __LINE__, {0x502,0x502}, 0, ERROR_SUCCESS, CP_UTF8, 0, "\xFE\xA3\xA3\xA3\xA3\xA3\xA3", 3, 8, {0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F}, 4 },
    { __LINE__, {0x600,0xA00}, 3, 0xBEAF, CP_UTF8, 0, "\xFE\xA3\xA3\xA3\xA3\xA3\xA3", 3, 8, {0xFFFD, 0xFFFD, 0xFFFD, 0x7F7F}, 4 },
    { __LINE__, {0x502,0x502}, 0, ERROR_SUCCESS, CP_UTF8, 0, "\xFE\xA3\xA3\xA3\xA3\xA3\xA3", 4, 8, {0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F}, 5 },
    { __LINE__, {0x600,0xA00}, 4, 0xBEAF, CP_UTF8, 0, "\xFE\xA3\xA3\xA3\xA3\xA3\xA3", 4, 8, {0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x7F7F}, 5 },
    { __LINE__, {0x502,0x502}, 0, ERROR_SUCCESS, CP_UTF8, 0, "\xFE\xA3\xA3\xA3\xA3\xA3\xA3", 5, 8, {0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F}, 6 },
    { __LINE__, {0x600,0xA00}, 5, 0xBEAF, CP_UTF8, 0, "\xFE\xA3\xA3\xA3\xA3\xA3\xA3", 5, 8, {0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x7F7F}, 6 },
    { __LINE__, {0x502,0x502}, 0, ERROR_SUCCESS, CP_UTF8, 0, "\xFE\xA3\xA3\xA3\xA3\xA3\xA3", 6, 8, {0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F}, 7 },
    { __LINE__, {0x600,0xA00}, 6, 0xBEAF, CP_UTF8, 0, "\xFE\xA3\xA3\xA3\xA3\xA3\xA3", 6, 8, {0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x7F7F}, 7 },
    { __LINE__, {0x502,0x502}, 0, ERROR_SUCCESS, CP_UTF8, 0, "\xFE\xA3\xA3\xA3\xA3\xA3\xA3", 7, 8, {0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F}, 8 },
    { __LINE__, {0x600,0xA00}, 7, 0xBEAF, CP_UTF8, 0, "\xFE\xA3\xA3\xA3\xA3\xA3\xA3", 7, 8, {0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x7F7F}, 8 },
    { __LINE__, {0x000,0x502}, 1, 0xBEAF, CP_UTF8, 0, "\xFE\xA3\xA3\xA3\xA3\xA3\xA3", 8, 8, {0, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F}, 8 },
    { __LINE__, {0x600,0xA00}, 8, 0xBEAF, CP_UTF8, 0, "\xFE\xA3\xA3\xA3\xA3\xA3\xA3", 8, 8, {0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0}, 8 },
    /* invalid UTF-8 sequences */
    { __LINE__, {0x502,0x502}, 0, ERROR_SUCCESS, CP_UTF8, 0, "\xFF\xA3\xA3\xA3\xA3\xA3\xA3\xA3", 1, 8, {0x7F7F, 0x7F7F}, 2 },
    { __LINE__, {0x600,0xA00}, 1, 0xBEAF, CP_UTF8, 0, "\xFF\xA3\xA3\xA3\xA3\xA3\xA3\xA3", 1, 8, {0xFFFD, 0x7F7F}, 2 },
    { __LINE__, {0x502,0x502}, 0, ERROR_SUCCESS, CP_UTF8, 0, "\xFF\xA3\xA3\xA3\xA3\xA3\xA3\xA3", 2, 8, {0x7F7F, 0x7F7F, 0x7F7F}, 3 },
    { __LINE__, {0x600,0xA00}, 2, 0xBEAF, CP_UTF8, 0, "\xFF\xA3\xA3\xA3\xA3\xA3\xA3\xA3", 2, 8, {0xFFFD, 0xFFFD, 0x7F7F}, 3 },
    { __LINE__, {0x502,0x502}, 0, ERROR_SUCCESS, CP_UTF8, 0, "\xFF\xA3\xA3\xA3\xA3\xA3\xA3\xA3", 3, 8, {0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F}, 4 },
    { __LINE__, {0x600,0xA00}, 3, 0xBEAF, CP_UTF8, 0, "\xFF\xA3\xA3\xA3\xA3\xA3\xA3\xA3", 3, 8, {0xFFFD, 0xFFFD, 0xFFFD, 0x7F7F}, 4 },
    { __LINE__, {0x502,0x502}, 0, ERROR_SUCCESS, CP_UTF8, 0, "\xFF\xA3\xA3\xA3\xA3\xA3\xA3\xA3", 4, 8, {0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F}, 5 },
    { __LINE__, {0x600,0xA00}, 4, 0xBEAF, CP_UTF8, 0, "\xFF\xA3\xA3\xA3\xA3\xA3\xA3\xA3", 4, 8, {0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x7F7F}, 5 },
    { __LINE__, {0x502,0x502}, 0, ERROR_SUCCESS, CP_UTF8, 0, "\xFF\xA3\xA3\xA3\xA3\xA3\xA3\xA3", 5, 8, {0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F}, 6 },
    { __LINE__, {0x600,0xA00}, 5, 0xBEAF, CP_UTF8, 0, "\xFF\xA3\xA3\xA3\xA3\xA3\xA3\xA3", 5, 8, {0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x7F7F}, 6 },
    { __LINE__, {0x502,0x502}, 0, ERROR_SUCCESS, CP_UTF8, 0, "\xFF\xA3\xA3\xA3\xA3\xA3\xA3\xA3", 6, 8, {0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F}, 7 },
    { __LINE__, {0x600,0xA00}, 6, 0xBEAF, CP_UTF8, 0, "\xFF\xA3\xA3\xA3\xA3\xA3\xA3\xA3", 6, 8, {0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x7F7F}, 7 },
    { __LINE__, {0x502,0x502}, 0, ERROR_SUCCESS, CP_UTF8, 0, "\xFF\xA3\xA3\xA3\xA3\xA3\xA3\xA3", 7, 8, {0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F}, 8 },
    { __LINE__, {0x600,0xA00}, 7, 0xBEAF, CP_UTF8, 0, "\xFF\xA3\xA3\xA3\xA3\xA3\xA3\xA3", 7, 8, {0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x7F7F}, 8 },
    { __LINE__, {0x502,0x502}, 0, ERROR_SUCCESS, CP_UTF8, 0, "\xFF\xA3\xA3\xA3\xA3\xA3\xA3\xA3", 8, 8, {0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F}, 9 },
    { __LINE__, {0x600,0xA00}, 8, 0xBEAF, CP_UTF8, 0, "\xFF\xA3\xA3\xA3\xA3\xA3\xA3\xA3", 8, 8, {0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x7F7F}, 9 },
    { __LINE__, {0x000,0x502}, 1, 0xBEAF, CP_UTF8, 0, "\xFF\xA3\xA3\xA3\xA3\xA3\xA3\xA3", 9, 8, {0, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F, 0x7F7F}, 9 },
    { __LINE__, {0x600,0xA00}, 0, ERROR_INSUFFICIENT_BUFFER, CP_UTF8, 0, "\xFF\xA3\xA3\xA3\xA3\xA3\xA3\xA3", 9, 8, {0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x7F7F}, 9 },
};

static void TestEntry(const ENTRY *pEntry)
{
    int ret, i;
    WCHAR Buffer[MAX_BUFFER];
    DWORD Error;

    if ((OsVersion < pEntry->VersionRange[0]) ||
        (OsVersion > pEntry->VersionRange[1]))
    {
        skip("Skipping entry from line %u (Version range %lx-%lx, current %lx)\n",
             pEntry->LineNo,
             pEntry->VersionRange[0],
             pEntry->VersionRange[1],
             OsVersion);
        return;
    }

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
    RTL_OSVERSIONINFOW vi;
    size_t i;

    vi.dwOSVersionInfoSize = sizeof(vi);
    if (RtlGetVersion(&vi) < 0)
    {
        skip("Failed to get OS version!\n");
        return;
    }

    OsVersion = (vi.dwMajorVersion << 8) | vi.dwMinorVersion;

    /* NOTE: We use Win10's MultiByteToWideChar behaviour due to security reason. */
    if (IsReactOS())
        OsVersion = 0xa00;

    for (i = 0; i < _countof(Entries); ++i)
    {
        TestEntry(&Entries[i]);
    }
}
