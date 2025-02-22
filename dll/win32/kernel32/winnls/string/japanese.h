/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Japanese era support
 * COPYRIGHT:   Copyright 2019 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#define JAPANESE_MAX_TWODIGITYEAR 99

typedef struct JAPANESE_ERA
{
    WORD wYear;
    WORD wMonth;
    WORD wDay;
    WCHAR szEraName[16];
    WCHAR szEraAbbrev[5];
    WCHAR szEnglishEraName[24];
    WCHAR szEnglishEraAbbrev[5];
} JAPANESE_ERA, *PJAPANESE_ERA;
typedef const JAPANESE_ERA *PCJAPANESE_ERA;

BOOL JapaneseEra_IsFirstYearGannen(void);
PCJAPANESE_ERA JapaneseEra_Find(const SYSTEMTIME *pst OPTIONAL);
void JapaneseEra_ClearCache(void);
