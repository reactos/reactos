/*
 * PROJECT:     ReactOS header
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Encoding, charsets and codepages for Chinese, Japanese and Korean (CJK)
 * COPYRIGHT:   Copyright 2017-2022 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
                Copyright 2017-2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#define CP_SHIFTJIS 932  // Japanese Shift-JIS
#define CP_HANGUL   949  // Korean Hangul/Wansung
#define CP_JOHAB    1361 // Korean Johab
#define CP_GB2312   936  // Chinese Simplified (GB2312)
#define CP_BIG5     950  // Chinese Traditional (Big5)

/*
 * "Human-understandable" names for the previous standard code pages.
 * Taken from https://github.com/microsoft/terminal/blob/main/src/inc/unicode.hpp
 */
#define CP_JAPANESE             CP_SHIFTJIS
#define CP_KOREAN               CP_HANGUL
#define CP_CHINESE_SIMPLIFIED   CP_GB2312
#define CP_CHINESE_TRADITIONAL  CP_BIG5

/* IsFarEastCP(CodePage) */
#define IsCJKCodePage(CodePage) \
    ((CodePage) == CP_SHIFTJIS || (CodePage) == CP_HANGUL || \
  /* (CodePage) == CP_JOHAB || */ \
     (CodePage) == CP_BIG5     || (CodePage) == CP_GB2312)

#if !defined(_WINGDI_) || defined(NOGDI)
#define SHIFTJIS_CHARSET    128
#define HANGEUL_CHARSET     129
#define HANGUL_CHARSET      129 // HANGEUL_CHARSET
#if (WINVER >= 0x0400)
#define JOHAB_CHARSET       130
#endif /* WINVER */
#define GB2312_CHARSET      134
#define CHINESEBIG5_CHARSET 136
#endif /* !defined(_WINGDI_) || defined(NOGDI) */

/* IsAnyDBCSCharSet(CharSet) */
#define IsCJKCharSet(CharSet)   \
    ((CharSet) == SHIFTJIS_CHARSET || (CharSet) == HANGUL_CHARSET || \
  /* (CharSet) == JOHAB_CHARSET || */ \
     (CharSet) == GB2312_CHARSET   || (CharSet) == CHINESEBIG5_CHARSET)
