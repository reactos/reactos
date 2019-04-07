/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/winnls/string/japanese.h
 * PURPOSE:         Japanese era support
 * PROGRAMMER:      Katayama Hirofumi MZ
 */

#define JAPANESE_MAX_TWODIGITYEAR 99

typedef struct JAPANESE_ERA
{
    WORD wYear;
    WORD wMonth;
    WORD wDay;
    WCHAR szEraName[16];
    WCHAR szEraInitial[5];
    WCHAR szEnglishEraName[24];
    WCHAR szEnglishEraInitial[5];
} JAPANESE_ERA, *LPJAPANESE_ERA;
typedef const JAPANESE_ERA *LPCJAPANESE_ERA;

INT  JapaneseEra_Compare(LPCJAPANESE_ERA pEra1, LPCJAPANESE_ERA pEra2);
INT  JapaneseEra_Compare0(const void *pEra1, const void *pEra2);
BOOL JapaneseEra_ToSystemTime(LPCJAPANESE_ERA pEra, LPSYSTEMTIME pst);
LPCJAPANESE_ERA JapaneseEra_Load(DWORD *pdwCount);
LPCJAPANESE_ERA JapaneseEra_Find(const SYSTEMTIME *pst OPTIONAL);
LPCJAPANESE_ERA JapaneseEra_ConvertYear(const SYSTEMTIME *pst OPTIONAL, LPWORD pwNengoYearOut);
