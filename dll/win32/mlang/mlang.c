/*
 *    MLANG Class Factory
 *
 * Copyright 2002 Lionel Ulmer
 * Copyright 2003,2004 Mike McCormack
 * Copyright 2004,2005 Dmitry Timoshkov
 * Copyright 2009 Detlef Riekenberg
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */


#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "ole2.h"
#include "objbase.h"
#include "rpcproxy.h"
#include "mlang.h"
#include "mimeole.h"

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(mlang);

#include "initguid.h"

static INIT_ONCE font_link_global_init_once = INIT_ONCE_STATIC_INIT;
static IUnknown *font_link_global = NULL;

static HRESULT MultiLanguage_create(IUnknown *pUnkOuter, LPVOID *ppObj);
static HRESULT MLangConvertCharset_create(IUnknown *outer, void **obj);
static HRESULT EnumRfc1766_create(LANGID LangId, IEnumRfc1766 **ppEnum);

/* FIXME:
 * Under what circumstances HKEY_CLASSES_ROOT\MIME\Database\Codepage and
 * HKEY_CLASSES_ROOT\MIME\Database\Charset are used?
 */

typedef struct
{
    const WCHAR *description;
    UINT cp;
    DWORD flags;
    const WCHAR *web_charset;
    const WCHAR *header_charset;
    const WCHAR *body_charset;
    const WCHAR *alias;
} MIME_CP_INFO;

/* These data are based on the codepage info in libs/unicode/cpmap.pl */
/* FIXME: Add 28604 (Celtic), 28606 (Balkan) */

static const MIME_CP_INFO arabic_cp[] =
{
    { L"Arabic (864)",
      864, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID_NLS |
           MIMECONTF_MIME_LATEST,
      L"ibm864", L"ibm864", L"ibm864" },
    { L"Arabic (1006)",
      1006, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID_NLS |
            MIMECONTF_MIME_LATEST,
      L"ibm1006", L"ibm1006", L"ibm1006" },
    { L"Arabic (Windows)",
      1256, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_IMPORT |
            MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER |
            MIMECONTF_EXPORT | MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      L"windows-1256", L"windows-1256", L"windows-1256" },
    { L"Arabic (ISO)",
      28596, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_MINIMAL |
             MIMECONTF_IMPORT | MIMECONTF_SAVABLE_MAILNEWS |
             MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_IE4 | MIMECONTF_MIME_LATEST,
      L"iso-8859-6", L"iso-8859-6", L"iso-8859-6" }
};
static const MIME_CP_INFO baltic_cp[] =
{
    { L"Baltic (DOS)",
      775, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
           MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      L"ibm775", L"ibm775", L"ibm775" },
    { L"Baltic (Windows)",
      1257, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_MINIMAL |
            MIMECONTF_IMPORT | MIMECONTF_SAVABLE_MAILNEWS |
            MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT | MIMECONTF_VALID |
            MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      L"windows-1257", L"windows-1257", L"windows-1257" },
    { L"Baltic (ISO)",
      28594, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_IMPORT |
             MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER |
             MIMECONTF_EXPORT | MIMECONTF_VALID | MIMECONTF_VALID_NLS |
             MIMECONTF_MIME_LATEST,
      L"iso-8859-4", L"iso-8859-4", L"iso-8859-4" },
    { L"Estonian (ISO)",
      28603, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      L"iso-8859-13", L"iso-8859-13", L"iso-8859-13" }
};
static const MIME_CP_INFO chinese_simplified_cp[] =
{
    { L"Chinese Simplified (Auto-Select)",
      50936, MIMECONTF_IMPORT | MIMECONTF_VALID | MIMECONTF_VALID_NLS |
             MIMECONTF_MIME_LATEST,
      L"_autodetect_chs", L"_autodetect_chs", L"_autodetect_chs" },
    { L"Chinese Simplified (GB2312)",
      936, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_MINIMAL |
           MIMECONTF_IMPORT | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_VALID |
           MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT | MIMECONTF_VALID_NLS |
           MIMECONTF_MIME_IE4 | MIMECONTF_MIME_LATEST,
      L"gb2312", L"gb2312", L"gb2312" },
    { L"Chinese Simplified (GB2312-80)",
      20936, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      L"x-cp20936", L"x-cp20936", L"x-cp20936" },
    { L"Chinese Simplified (HZ)",
      52936, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_IMPORT |
             MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT |
             MIMECONTF_VALID | MIMECONTF_VALID_NLS | MIMECONTF_MIME_IE4 |
             MIMECONTF_MIME_LATEST,
      L"hz-gb-2312", L"hz-gb-2312", L"hz-gb-2312" },
    { L"Chinese Simplified (GB18030)",
      54936, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_MINIMAL |
             MIMECONTF_IMPORT | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER |
             MIMECONTF_EXPORT | MIMECONTF_VALID | MIMECONTF_VALID_NLS |
             MIMECONTF_MIME_LATEST,
      L"GB18030", L"GB18030", L"GB18030" },
    { L"Chinese Simplified (GBK)",
      936, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_MINIMAL |
           MIMECONTF_IMPORT | MIMECONTF_SAVABLE_MAILNEWS |
           MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT | MIMECONTF_VALID_NLS |
           MIMECONTF_MIME_IE4 | MIMECONTF_MIME_LATEST,
      L"gbk", L"gbk", L"gbk" }
};
static const MIME_CP_INFO chinese_traditional_cp[] =
{
    { L"Chinese Traditional (Auto-Select)",
      50950, MIMECONTF_IMPORT | MIMECONTF_VALID | MIMECONTF_VALID_NLS |
             MIMECONTF_MIME_LATEST,
      L"_autodetect_cht", L"_autodetect_cht", L"_autodetect_cht" },
    { L"Chinese Traditional (Big5)",
      950, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_MINIMAL |
           MIMECONTF_IMPORT | MIMECONTF_SAVABLE_MAILNEWS |
           MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT | MIMECONTF_VALID |
           MIMECONTF_VALID_NLS | MIMECONTF_MIME_IE4 | MIMECONTF_MIME_LATEST,
      L"big5", L"big5", L"big5" },
    { L"Chinese Traditional (CNS)",
      20000, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      L"x-Chinese-CNS", L"x-Chinese-CNS", L"x-Chinese-CNS" }
};
static const MIME_CP_INFO central_european_cp[] =
{
    { L"Central European (DOS)",
      852, MIMECONTF_BROWSER | MIMECONTF_IMPORT | MIMECONTF_SAVABLE_BROWSER |
           MIMECONTF_EXPORT | MIMECONTF_VALID | MIMECONTF_VALID_NLS |
           MIMECONTF_MIME_IE4 | MIMECONTF_MIME_LATEST,
      L"ibm852", L"ibm852", L"ibm852" },
    { L"Central European (Windows)",
      1250, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_IMPORT |
            MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER |
            MIMECONTF_EXPORT | MIMECONTF_VALID | MIMECONTF_VALID_NLS |
            MIMECONTF_MIME_LATEST,
      L"windows-1250", L"windows-1250", L"windows-1250" },
    { L"Central European (Mac)",
      10029, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      L"x-mac-ce", L"x-mac-ce", L"x-mac-ce" },
    { L"Central European (ISO)",
      28592, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_MINIMAL |
             MIMECONTF_IMPORT | MIMECONTF_SAVABLE_MAILNEWS |
             MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT | MIMECONTF_VALID |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_IE4 | MIMECONTF_MIME_LATEST,
      L"iso-8859-2", L"iso-8859-2", L"iso-8859-2" }
};
static const MIME_CP_INFO cyrillic_cp[] =
{
    { L"OEM Cyrillic",
      855, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
           MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      L"ibm855", L"ibm855", L"ibm855" },
    { L"Cyrillic (DOS)",
      866, MIMECONTF_BROWSER | MIMECONTF_IMPORT | MIMECONTF_SAVABLE_BROWSER |
           MIMECONTF_EXPORT | MIMECONTF_VALID_NLS | MIMECONTF_MIME_IE4 |
           MIMECONTF_MIME_LATEST,
      L"cp866", L"cp866", L"cp866" },
#if 0 /* Windows has 20866 as an official code page for KOI8-R */
    { L"Cyrillic (KOI8-R)",
      878, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
           MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      L"koi8-r", L"koi8-r", L"koi8-r" },
#endif
    { L"Cyrillic (Windows)",
      1251, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_IMPORT |
            MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER |
            MIMECONTF_EXPORT | MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      L"windows-1251", L"windows-1251", L"windows-1251" },
    { L"Cyrillic (Mac)",
      10007, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID_NLS |
             MIMECONTF_MIME_LATEST,
      L"x-mac-cyrillic", L"x-mac-cyrillic", L"x-mac-cyrillic" },
    { L"Cyrillic (KOI8-R)",
      20866, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_MINIMAL |
             MIMECONTF_IMPORT | MIMECONTF_SAVABLE_MAILNEWS |
             MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_IE4 | MIMECONTF_MIME_LATEST,
      L"koi8-r", L"koi8-r", L"koi8-r" },
    { L"Cyrillic (KOI8-U)",
      21866, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_MINIMAL |
             MIMECONTF_IMPORT | MIMECONTF_SAVABLE_MAILNEWS |
             MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_IE4 | MIMECONTF_MIME_LATEST,
      L"koi8-u", L"koi8-u", L"koi8-u" },
    { L"Cyrillic (ISO)",
      28595, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_MINIMAL |
             MIMECONTF_IMPORT | MIMECONTF_SAVABLE_MAILNEWS |
             MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_IE4 | MIMECONTF_MIME_LATEST,
      L"iso-8859-5", L"iso-8859-5", L"iso-8859-5" }
};
static const MIME_CP_INFO greek_cp[] =
{
    { L"Greek (DOS)",
      737, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID_NLS |
           MIMECONTF_MIME_LATEST,
      L"ibm737", L"ibm737", L"ibm737" },
    { L"Greek, Modern (DOS)",
      869, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID_NLS |
           MIMECONTF_MIME_LATEST,
      L"ibm869", L"ibm869", L"ibm869" },
    { L"IBM EBCDIC (Greek Modern)",
      875, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID_NLS |
           MIMECONTF_MIME_LATEST,
      L"cp875", L"cp875", L"cp875" },
    { L"Greek (Windows)",
      1253, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_IMPORT |
            MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER |
            MIMECONTF_EXPORT | MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      L"windows-1253", L"windows-1253", L"windows-1253" },
    { L"Greek (Mac)",
      10006, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID_NLS |
             MIMECONTF_MIME_LATEST,
      L"x-mac-greek", L"x-mac-greek", L"x-mac-greek" },
    { L"Greek (ISO)",
      28597, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_MINIMAL |
             MIMECONTF_IMPORT | MIMECONTF_SAVABLE_MAILNEWS |
             MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_IE4 | MIMECONTF_MIME_LATEST,
      L"iso-8859-7", L"iso-8859-7", L"iso-8859-7" }
};
static const MIME_CP_INFO hebrew_cp[] =
{
    { L"Hebrew (424)",
      424, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID_NLS |
           MIMECONTF_MIME_LATEST,
      L"ibm424", L"ibm424", L"ibm424" },
    { L"Hebrew (856)",
      856, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID_NLS |
           MIMECONTF_MIME_LATEST,
      L"cp856", L"cp856", L"cp856" },
    { L"Hebrew (DOS)",
      862, MIMECONTF_BROWSER | MIMECONTF_MINIMAL | MIMECONTF_IMPORT |
           MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT | MIMECONTF_VALID_NLS |
           MIMECONTF_MIME_LATEST,
      L"dos-862", L"dos-862", L"dos-862" },
    { L"Hebrew (Windows)",
      1255, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_IMPORT |
            MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER |
            MIMECONTF_EXPORT | MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      L"windows-1255", L"windows-1255", L"windows-1255" },
    { L"Hebrew (ISO-Visual)",
      28598, MIMECONTF_BROWSER | MIMECONTF_MINIMAL | MIMECONTF_IMPORT |
             MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_IE4 | MIMECONTF_MIME_LATEST,
      L"iso-8859-8", L"iso-8859-8", L"iso-8859-8" }
};
static const MIME_CP_INFO japanese_cp[] =
{
    { L"Japanese (Auto-Select)",
      50932, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_MINIMAL |
             MIMECONTF_IMPORT | MIMECONTF_VALID | MIMECONTF_VALID_NLS |
             MIMECONTF_MIME_IE4 | MIMECONTF_MIME_LATEST,
      L"_autodetect", L"_autodetect", L"_autodetect" },
    { L"Japanese (EUC)",
      51932, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_MINIMAL |
             MIMECONTF_IMPORT | MIMECONTF_SAVABLE_MAILNEWS |
             MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT | MIMECONTF_VALID |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_IE4 | MIMECONTF_MIME_LATEST,
      L"euc-jp", L"euc-jp", L"euc-jp" },
    { L"Japanese (JIS)",
      50220, MIMECONTF_IMPORT | MIMECONTF_MAILNEWS | MIMECONTF_EXPORT |
             MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_VALID_NLS |
             MIMECONTF_PRIVCONVERTER | MIMECONTF_MIME_LATEST |
             MIMECONTF_MIME_IE4,
      L"iso-2022-jp", L"iso-2022-jp", L"iso-2022-jp"},
    { L"Japanese (JIS 0208-1990 and 0212-1990)",
      20932, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID_NLS |
             MIMECONTF_VALID | MIMECONTF_PRIVCONVERTER | MIMECONTF_MIME_LATEST,
      L"EUC-JP", L"EUC-JP", L"EUC-JP"},
    { L"Japanese (JIS-Allow 1 byte Kana)",
      50221, MIMECONTF_MAILNEWS | MIMECONTF_EXPORT | MIMECONTF_SAVABLE_BROWSER |
             MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_VALID_NLS |
             MIMECONTF_VALID | MIMECONTF_PRIVCONVERTER | MIMECONTF_MIME_LATEST,
      L"csISO2022JP", L"iso-2022-jp", L"iso-2022-jp"},
    { L"Japanese (JIS-Allow 1 byte Kana - SO/SI)",
      50222, MIMECONTF_EXPORT | MIMECONTF_VALID_NLS | MIMECONTF_VALID |
             MIMECONTF_PRIVCONVERTER | MIMECONTF_MIME_LATEST,
      L"iso-2022-jp", L"iso-2022-jp", L"iso-2022-jp"},
    { L"Japanese (Mac)",
      10001, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID_NLS |
             MIMECONTF_VALID | MIMECONTF_PRIVCONVERTER | MIMECONTF_MIME_LATEST,
      L"x-mac-japanese", L"x-mac-japanese", L"x-mac-japanese"},
    { L"Japanese (Shift-JIS)",
      932, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_MINIMAL |
           MIMECONTF_IMPORT | MIMECONTF_SAVABLE_MAILNEWS |
           MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT | MIMECONTF_VALID |
           MIMECONTF_VALID_NLS | MIMECONTF_MIME_IE4 | MIMECONTF_MIME_LATEST,
      L"shift_jis", L"iso-2022-jp", L"iso-2022-jp" }
};
static const MIME_CP_INFO korean_cp[] =
{
    { L"Korean",
      949, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_MINIMAL |
           MIMECONTF_IMPORT | MIMECONTF_SAVABLE_MAILNEWS |
           MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT | MIMECONTF_VALID_NLS |
           MIMECONTF_MIME_LATEST,
      L"ks_c_5601-1987", L"ks_c_5601-1987", L"ks_c_5601-1987" }
};
static const MIME_CP_INFO thai_cp[] =
{
    { L"Thai (Windows)",
      874, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_MIME_LATEST,
      L"ibm-thai", L"ibm-thai", L"ibm-thai" }
};
static const MIME_CP_INFO turkish_cp[] =
{
    { L"Turkish (DOS)",
      857, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
           MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      L"ibm857", L"ibm857", L"ibm857" },
    { L"IBM EBCDIC (Turkish Latin-5)",
      1026, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
            MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      L"ibm1026", L"ibm1026", L"ibm1026" },
    { L"Turkish (Windows)",
      1254, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_MINIMAL |
            MIMECONTF_IMPORT | MIMECONTF_SAVABLE_MAILNEWS |
            MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT | MIMECONTF_VALID |
            MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      L"windows-1254", L"windows-1254", L"windows-1254" },
    { L"Turkish (Mac)",
      10081, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      L"x-mac-turkish", L"x-mac-turkish", L"x-mac-turkish" },
    { L"Latin 3 (ISO)",
      28593, MIMECONTF_MAILNEWS | MIMECONTF_IMPORT |
             MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_EXPORT | MIMECONTF_VALID |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_IE4 | MIMECONTF_MIME_LATEST,
      L"iso-8859-3", L"iso-8859-3", L"iso-8859-3" },
    { L"Turkish (ISO)",
      28599, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_MINIMAL |
             MIMECONTF_IMPORT | MIMECONTF_SAVABLE_MAILNEWS |
             MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT | MIMECONTF_VALID |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      L"iso-8859-9", L"iso-8859-9", L"iso-8859-9" }
};
static const MIME_CP_INFO vietnamese_cp[] =
{
    { L"Vietnamese (Windows)",
      1258, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_IMPORT |
            MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER |
            MIMECONTF_EXPORT | MIMECONTF_VALID_NLS | MIMECONTF_MIME_IE4 |
            MIMECONTF_MIME_LATEST,
      L"windows-1258", L"windows-1258", L"windows-1258" }
};

static const MIME_CP_INFO western_cp[] =
{
    { L"IBM EBCDIC (US-Canada)",
      37, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
          MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      L"ibm037", L"ibm037", L"ibm037" },
    { L"OEM United States",
      437, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
           MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      L"ibm437", L"ibm437", L"ibm437" },
    { L"IBM EBCDIC (International)",
      500, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
           MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      L"ibm500", L"ibm500", L"ibm500" },
    { L"Western European (DOS)",
      850, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
           MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      L"ibm850", L"ibm850", L"ibm850" },
    { L"Portuguese (DOS)",
      860, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
           MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      L"ibm860", L"ibm860", L"ibm860" },
    { L"Icelandic (DOS)",
      861, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
           MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      L"ibm861", L"ibm861", L"ibm861" },
    { L"French Canadian (DOS)",
      863, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
           MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      L"ibm863", L"ibm863", L"ibm863" },
    { L"Nordic (DOS)",
      865, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
           MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      L"ibm865", L"ibm865", L"ibm865" },
    { L"Western European (Windows)",
      1252, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_MINIMAL |
            MIMECONTF_IMPORT | MIMECONTF_SAVABLE_MAILNEWS |
            MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT | MIMECONTF_VALID |
            MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      L"windows-1252", L"windows-1252", L"iso-8859-1" },
    { L"Western European (Mac)",
      10000, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      L"macintosh", L"macintosh", L"macintosh" },
    { L"Icelandic (Mac)",
      10079, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      L"x-mac-icelandic", L"x-mac-icelandic", L"x-mac-icelandic" },
    { L"US-ASCII",
      20127, MIMECONTF_MAILNEWS | MIMECONTF_IMPORT | MIMECONTF_EXPORT |
             MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_VALID |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      L"us-ascii", L"us-ascii", L"us-ascii", L"ascii" },
    { L"Western European (ISO)",
      28591, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_IMPORT |
             MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER |
             MIMECONTF_EXPORT | MIMECONTF_VALID | MIMECONTF_VALID_NLS |
             MIMECONTF_MIME_LATEST,
      L"iso-8859-1", L"iso-8859-1", L"iso-8859-1", L"iso8859-1" },
    { L"Latin 9 (ISO)",
      28605, MIMECONTF_MAILNEWS | MIMECONTF_IMPORT |
             MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER |
             MIMECONTF_EXPORT | MIMECONTF_VALID | MIMECONTF_VALID_NLS |
             MIMECONTF_MIME_LATEST,
      L"iso-8859-15", L"iso-8859-15", L"iso-8859-15" }
};
static const MIME_CP_INFO unicode_cp[] =
{
    { L"Unicode",
      CP_UNICODE, MIMECONTF_MINIMAL | MIMECONTF_IMPORT |
                  MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT |
                  MIMECONTF_VALID | MIMECONTF_VALID_NLS | MIMECONTF_MIME_IE4 |
                  MIMECONTF_MIME_LATEST,
      L"unicode", L"unicode", L"unicode" },
    { L"Unicode (UTF-7)",
      CP_UTF7, MIMECONTF_MAILNEWS | MIMECONTF_IMPORT |
               MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_EXPORT | MIMECONTF_VALID |
               MIMECONTF_VALID_NLS | MIMECONTF_MIME_IE4 | MIMECONTF_MIME_LATEST,
      L"utf-7", L"utf-7", L"utf-7" },
    { L"Unicode (UTF-8)",
      CP_UTF8, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_IMPORT |
               MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER |
               MIMECONTF_EXPORT | MIMECONTF_VALID | MIMECONTF_VALID_NLS |
               MIMECONTF_MIME_IE4 | MIMECONTF_MIME_LATEST,
      L"utf-8", L"utf-8", L"utf-8" }
};

static const struct mlang_data
{
    const WCHAR *description;
    UINT family_codepage;
    UINT number_of_cp;
    const MIME_CP_INFO *mime_cp_info;
    const WCHAR *fixed_font;
    const WCHAR *proportional_font;
    SCRIPT_ID sid;
} mlang_data[] =
{
    { L"Arabic", 1256, ARRAY_SIZE(arabic_cp), arabic_cp,
      L"Simplified Arabic Fixed", L"Simplified Arabic", sidArabic },
    { L"Baltic",  1257, ARRAY_SIZE(baltic_cp), baltic_cp,
      L"Courier New", L"Arial", sidAsciiLatin },
    { L"Chinese Simplified", 936, ARRAY_SIZE(chinese_simplified_cp), chinese_simplified_cp,
      L"Simsun", L"Simsun", sidHan },
    { L"Chinese Traditional", 950, ARRAY_SIZE(chinese_traditional_cp), chinese_traditional_cp,
      L"MingLiu", L"New MingLiu", sidBopomofo },
    { L"Central European", 1250, ARRAY_SIZE(central_european_cp), central_european_cp,
      L"Courier New", L"Arial", sidAsciiLatin },
    { L"Cyrillic", 1251, ARRAY_SIZE(cyrillic_cp), cyrillic_cp,
      L"Courier New", L"Arial", sidCyrillic },
    { L"Greek", 1253, ARRAY_SIZE(greek_cp), greek_cp,
      L"Courier New", L"Arial", sidGreek },
    { L"Hebrew", 1255, ARRAY_SIZE(hebrew_cp), hebrew_cp,
      L"Miriam Fixed", L"David", sidHebrew },
    { L"Japanese", 932, ARRAY_SIZE(japanese_cp), japanese_cp,
      L"MS Gothic", L"MS PGothic", sidKana },
    { L"Korean", 949, ARRAY_SIZE(korean_cp), korean_cp,
      L"GulimChe", L"Gulim", sidHangul },
    { L"Thai", 874, ARRAY_SIZE(thai_cp), thai_cp,
      L"Tahoma", L"Tahoma", sidThai },
    { L"Turkish", 1254, ARRAY_SIZE(turkish_cp), turkish_cp,
      L"Courier New", L"Arial", sidAsciiLatin },
    { L"Vietnamese", 1258, ARRAY_SIZE(vietnamese_cp), vietnamese_cp,
      L"Courier New", L"Arial", sidAsciiLatin },
    { L"Western European", 1252, ARRAY_SIZE(western_cp), western_cp,
      L"Courier New", L"Arial", sidAsciiLatin },
    { L"Unicode", CP_UNICODE, ARRAY_SIZE(unicode_cp), unicode_cp,
      L"Courier New", L"Arial" }
};

struct font_list
{
    struct list list_entry;
    HFONT base_font;
    HFONT font;
    UINT charset;
};

static struct list font_cache = LIST_INIT(font_cache);
static CRITICAL_SECTION font_cache_critical;
static CRITICAL_SECTION_DEBUG font_cache_critical_debug =
{
    0, 0, &font_cache_critical,
    { &font_cache_critical_debug.ProcessLocksList, &font_cache_critical_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": font_cache_critical") }
};
static CRITICAL_SECTION font_cache_critical = { &font_cache_critical_debug, -1, 0, 0, 0, 0 };

static void fill_cp_info(const struct mlang_data *ml_data, UINT index, MIMECPINFO *mime_cp_info);

static LONG dll_count;

/*
 * Japanese Detection and Conversion Functions
 */

#define HANKATA(A)  ((A >= 161) && (A <= 223))
#define ISEUC(A)    ((A >= 161) && (A <= 254))
#define NOTEUC(A,B) (((A >= 129) && (A <= 159)) && ((B >= 64) && (B <= 160)))
#define SJIS1(A)    (((A >= 129) && (A <= 159)) || ((A >= 224) && (A <= 239)))
#define SJIS2(A)    ((A >= 64) && (A <= 252))
#define ISMARU(A)   ((A >= 202) && (A <= 206))
#define ISNIGORI(A) (((A >= 182) && (A <= 196)) || ((A >= 202) && (A <= 206)))

static UINT DetectJapaneseCode(LPCSTR input, DWORD count)
{
    UINT code = 0;
    DWORD i = 0;
    unsigned char c1,c2;

    while ((code == 0 || code == 51932) && i < count)
    {
        c1 = input[i];
        if (c1 == 0x1b /* ESC */)
        {
            i++;
            if (i >= count)
                return code;
            c1 = input[i];
            if (c1 == '$')
            {
                i++;
                if (i >= count)
                    return code;
                c1 = input[i];
                if (c1 =='B' || c1 == '@')
                    code = 50220;
            }
            if (c1 == 'K')
                code = 50220;
        }
        else if (c1 >= 129)
        {
            i++;
            if (i >= count)
                return code;
            c2 = input[i];
            if NOTEUC(c1,c2)
                code = 932;
            else if (ISEUC(c1) && ISEUC(c2))
                code = 51932;
            else if (((c1 == 142)) && HANKATA(c2))
                code = 51932;
        }
        i++;
    }
    return code;
}

static inline void jis2sjis(unsigned char *p1, unsigned char *p2)
{
    unsigned char c1 = *p1;
    unsigned char c2 = *p2;
    int row = c1 < 95 ? 112 : 176;
    int cell = c1 % 2 ? 31 + (c2 > 95) : 126;

    *p1 = ((c1 + 1) >> 1) + row;
    *p2 = c2 + cell;
}

static inline void sjis2jis(unsigned char *p1, unsigned char *p2)
{
    unsigned char c1 = *p1;
    unsigned char c2 = *p2;
    int shift = c2 < 159;
    int row = c1 < 160 ? 112 : 176;
    int cell = shift ? (31 + (c2 > 127)): 126;

    *p1 = ((c1 - row) << 1) - shift;
    *p2 -= cell;
}

static int han2zen(unsigned char *p1, unsigned char *p2)
{
    BOOL maru = FALSE;
    BOOL nigori = FALSE;
    static const unsigned char char1[] = {129,129,129,129,129,131,131,131,131,
        131,131,131,131,131,131,129,131,131,131,131,131,131,131,131,131,131,
        131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,
        131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,
        131,129,129 };
    static const unsigned char char2[] = {66,117,118,65,69,146,64,66,68,70,
        72,131,133,135,98,91,65,67,69,71,73,74,76,78,80,82,84,86,88,90,92,94,
        96,99,101,103,105,106,107,108,109,110,113,116,119,122,125,126,128,
        129,130,132,134,136,137,138,139,140,141,143,147,74,75};

    if (( *p2 == 222) && ((ISNIGORI(*p1) || (*p1 == 179))))
            nigori = TRUE;
    else if ((*p2 == 223) && (ISMARU(*p1)))
            maru = TRUE;

    if (*p1 >= 161 && *p1 <= 223)
    {
        unsigned char index = *p1 - 161;
        *p1 = char1[index];
        *p2 = char2[index];
    }

    if (maru || nigori)
    {
        if (nigori)
        {
            if (((*p2 >= 74) && (*p2 <= 103)) || ((*p2 >= 110) && (*p2 <= 122)))
                (*p2)++;
            else if ((*p1 == 131) && (*p2 == 69))
                *p2 = 148;
        }
        else if ((maru) && ((*p2 >= 110) && (*p2 <= 122)))
            *p2+= 2;

        return 1;
    }

    return 0;
}


static UINT ConvertJIS2SJIS(LPCSTR input, DWORD count, LPSTR output)
{
    DWORD i = 0;
    int j = 0;
    unsigned char p2,p;
    BOOL shifted = FALSE;

    while (i < count)
    {
        p = input[i];
        if (p == 0x1b /* ESC */)
        {
            i++;
            if (i >= count)
                return 0;
            p2 = input[i];
            if (p2 == '$' || p2 =='(')
                i++;
            if (p2 == 'K' || p2 =='$')
                shifted = TRUE;
            else
                shifted = FALSE;
        }
        else
        {
            if (shifted)
            {
                i++;
                if (i >= count)
                    return 0;
                p2 = input[i];
                jis2sjis(&p,&p2);
                output[j++]=p;
                output[j++]=p2;
            }
            else
            {
                output[j++] = p;
            }
        }
        i++;
    }
    return j;
}

static inline int exit_shift(LPSTR out, int c)
{
    if (out)
    {
        out[c] = 0x1b;
        out[c+1] = '(';
        out[c+2] = 'B';
    }
    return 3;
}

static inline int enter_shift(LPSTR out, int c)
{
    if (out)
    {
        out[c] = 0x1b;
        out[c+1] = '$';
        out[c+2] = 'B';
    }
    return 3;
}


static UINT ConvertSJIS2JIS(LPCSTR input, DWORD count, LPSTR output)
{
    DWORD i = 0;
    int j = 0;
    unsigned char p2,p;
    BOOL shifted = FALSE;

    while (i < count)
    {
        p = input[i] & 0xff;
        if (p == 10 || p == 13) /* NL and CR */
        {
            if (shifted)
            {
                shifted = FALSE;
                j += exit_shift(output,j);
            }
            if (output)
                output[j++] = p;
            else
                j++;
        }
        else
        {
            if (SJIS1(p))
            {
                i++;
                if (i >= count)
                    return 0;
                p2 = input[i] & 0xff;
                if (SJIS2(p2))
                {
                    sjis2jis(&p,&p2);
                    if (!shifted)
                    {
                        shifted = TRUE;
                        j+=enter_shift(output,j);
                    }
                }

                if (output)
                {
                    output[j++]=p;
                    output[j++]=p2;
                }
                else
                    j+=2;
            }
            else
            {
                if (HANKATA(p))
                {
                    if ((i+1) >= count)
                        return 0;
                    p2 = input[i+1] & 0xff;
                    i+=han2zen(&p,&p2);
                    sjis2jis(&p,&p2);
                    if (!shifted)
                    {
                        shifted = TRUE;
                        j+=enter_shift(output,j);
                    }
                    if (output)
                    {
                        output[j++]=p;
                        output[j++]=p2;
                    }
                    else
                        j+=2;
                }
                else
                {
                    if (shifted)
                    {
                        shifted = FALSE;
                        j += exit_shift(output,j);
                    }
                    if (output)
                        output[j++]=p;
                    else
                        j++;
                }
            }
        }
        i++;
    }
    if (shifted)
        j += exit_shift(output,j);
    return j;
}

static UINT ConvertJISJapaneseToUnicode(LPCSTR input, DWORD count,
                                        LPWSTR output, DWORD out_count)
{
    CHAR *sjis_string;
    UINT rc = 0;
    sjis_string = malloc(count);
    rc = ConvertJIS2SJIS(input,count,sjis_string);
    if (rc)
    {
        TRACE("%s\n",debugstr_an(sjis_string,rc));
        if (output)
            rc = MultiByteToWideChar(932,0,sjis_string,rc,output,out_count);
        else
            rc = MultiByteToWideChar(932,0,sjis_string,rc,0,0);
    }
    free(sjis_string);
    return rc;

}

static UINT ConvertUnknownJapaneseToUnicode(LPCSTR input, DWORD count,
                                            LPWSTR output, DWORD out_count)
{
    CHAR *sjis_string;
    UINT rc = 0;
    int code = DetectJapaneseCode(input,count);
    TRACE("Japanese code %i\n",code);

    switch (code)
    {
    case 0:
        if (output)
            rc = MultiByteToWideChar(CP_ACP,0,input,count,output,out_count);
        else
            rc = MultiByteToWideChar(CP_ACP,0,input,count,0,0);
        break;

    case 932:
        if (output)
            rc = MultiByteToWideChar(932,0,input,count,output,out_count);
        else
            rc = MultiByteToWideChar(932,0,input,count,0,0);
        break;

    case 51932:
        if (output)
            rc = MultiByteToWideChar(20932,0,input,count,output,out_count);
        else
            rc = MultiByteToWideChar(20932,0,input,count,0,0);
        break;

    case 50220:
        sjis_string = malloc(count);
        rc = ConvertJIS2SJIS(input,count,sjis_string);
        if (rc)
        {
            TRACE("%s\n",debugstr_an(sjis_string,rc));
            if (output)
                rc = MultiByteToWideChar(932,0,sjis_string,rc,output,out_count);
            else
                rc = MultiByteToWideChar(932,0,sjis_string,rc,0,0);
        }
        free(sjis_string);
        break;
    }
    return rc;
}

static UINT ConvertJapaneseUnicodeToJIS(LPCWSTR input, DWORD count,
                                        LPSTR output, DWORD out_count)
{
    CHAR *sjis_string;
    INT len;
    UINT rc = 0;

    len = WideCharToMultiByte(932,0,input,count,0,0,NULL,NULL);
    sjis_string = malloc(len);
    WideCharToMultiByte(932,0,input,count,sjis_string,len,NULL,NULL);
    TRACE("%s\n",debugstr_an(sjis_string,len));

    rc = ConvertSJIS2JIS(sjis_string, len, NULL);
    if (out_count >= rc)
    {
        ConvertSJIS2JIS(sjis_string, len, output);
    }
    free(sjis_string);
    return rc;

}

/*
 * Dll lifetime tracking declaration
 */
static void LockModule(void)
{
    InterlockedIncrement(&dll_count);
}

static void UnlockModule(void)
{
    InterlockedDecrement(&dll_count);
}

HRESULT WINAPI ConvertINetMultiByteToUnicode(
    LPDWORD pdwMode,
    DWORD dwEncoding,
    LPCSTR pSrcStr,
    LPINT pcSrcSize,
    LPWSTR pDstStr,
    LPINT pcDstSize)
{
    INT src_len = -1;

    TRACE("%p %ld %s %p %p %p\n", pdwMode, dwEncoding,
          debugstr_a(pSrcStr), pcSrcSize, pDstStr, pcDstSize);

    if (!pcDstSize)
        return E_FAIL;

    if (!pcSrcSize)
        pcSrcSize = &src_len;

    if (!*pcSrcSize)
    {
        *pcDstSize = 0;
        return S_OK;
    }

    /* forwarding euc-jp to EUC-JP */
    if (dwEncoding == 51932)
        dwEncoding = 20932;

    switch (dwEncoding)
    {
    case CP_UNICODE:
        if (*pcSrcSize == -1)
            *pcSrcSize = lstrlenW((LPCWSTR)pSrcStr);
        *pcDstSize = min(*pcSrcSize, *pcDstSize);
        *pcSrcSize *= sizeof(WCHAR);
        if (pDstStr)
            memmove(pDstStr, pSrcStr, *pcDstSize * sizeof(WCHAR));
        break;

    case 50220:
    case 50221:
    case 50222:
        *pcDstSize = ConvertJISJapaneseToUnicode(pSrcStr,*pcSrcSize,pDstStr,*pcDstSize);
        break;
    case 50932:
        *pcDstSize = ConvertUnknownJapaneseToUnicode(pSrcStr,*pcSrcSize,pDstStr,*pcDstSize);
        break;

    default:
        if (*pcSrcSize == -1)
            *pcSrcSize = lstrlenA(pSrcStr);

        if (pDstStr)
            *pcDstSize = MultiByteToWideChar(dwEncoding, 0, pSrcStr, *pcSrcSize, pDstStr, *pcDstSize);
        else
            *pcDstSize = MultiByteToWideChar(dwEncoding, 0, pSrcStr, *pcSrcSize, NULL, 0);
        break;
    }

    if (!*pcDstSize)
        return E_FAIL;

    return S_OK;
}

HRESULT WINAPI ConvertINetUnicodeToMultiByte(
    LPDWORD pdwMode,
    DWORD dwEncoding,
    LPCWSTR pSrcStr,
    LPINT pcSrcSize,
    LPSTR pDstStr,
    LPINT pcDstSize)
{
    INT destsz, size;
    INT src_len = -1;

    TRACE("%p %ld %s %p %p %p\n", pdwMode, dwEncoding,
          debugstr_w(pSrcStr), pcSrcSize, pDstStr, pcDstSize);

    if (!pcDstSize)
        return S_OK;

    if (!pcSrcSize)
        pcSrcSize = &src_len;

    destsz = (pDstStr) ? *pcDstSize : 0;
    *pcDstSize = 0;

    if (!pSrcStr || !*pcSrcSize)
        return S_OK;

    if (*pcSrcSize == -1)
        *pcSrcSize = lstrlenW(pSrcStr);

    /* forwarding euc-jp to EUC-JP */
    if (dwEncoding == 51932)
        dwEncoding = 20932;

    if (dwEncoding == CP_UNICODE)
    {
        if (*pcSrcSize == -1)
            *pcSrcSize = lstrlenW(pSrcStr);

        size = min(*pcSrcSize, destsz) * sizeof(WCHAR);
        if (pDstStr)
            memmove(pDstStr, pSrcStr, size);

        if (size >= destsz)
            goto fail;
    }
    else if (dwEncoding == 50220 || dwEncoding == 50221 || dwEncoding == 50222)
    {
        size = ConvertJapaneseUnicodeToJIS(pSrcStr, *pcSrcSize, NULL, 0);
        if (!size)
            goto fail;

        if (pDstStr)
        {
            size = ConvertJapaneseUnicodeToJIS(pSrcStr, *pcSrcSize, pDstStr,
                                               destsz);
            if (!size)
                goto fail;
        }

    }
    else
    {
        size = WideCharToMultiByte(dwEncoding, 0, pSrcStr, *pcSrcSize,
                                   NULL, 0, NULL, NULL);
        if (!size)
            goto fail;

        if (pDstStr)
        {
            size = WideCharToMultiByte(dwEncoding, 0, pSrcStr, *pcSrcSize,
                                       pDstStr, destsz, NULL, NULL);
            if (!size)
                goto fail;
        }
    }

    *pcDstSize = size;
    return S_OK;

fail:
    *pcSrcSize = 0;
    *pcDstSize = 0;
    return E_FAIL;
}

HRESULT WINAPI ConvertINetString(
    LPDWORD pdwMode,
    DWORD dwSrcEncoding,
    DWORD dwDstEncoding,
    LPCSTR pSrcStr,
    LPINT pcSrcSize,
    LPSTR pDstStr,
    LPINT pcDstSize
)
{
    TRACE("%p %ld %ld %s %p %p %p\n", pdwMode, dwSrcEncoding, dwDstEncoding,
          debugstr_a(pSrcStr), pcSrcSize, pDstStr, pcDstSize);

    if (dwSrcEncoding == CP_UNICODE)
    {
        INT cSrcSizeW;
        if (pcSrcSize && *pcSrcSize != -1)
        {
            cSrcSizeW = *pcSrcSize / sizeof(WCHAR);
            pcSrcSize = &cSrcSizeW;
        }
        return ConvertINetUnicodeToMultiByte(pdwMode, dwDstEncoding, (LPCWSTR)pSrcStr, pcSrcSize, pDstStr, pcDstSize);
    }
    else if (dwDstEncoding == CP_UNICODE)
    {
        HRESULT hr = ConvertINetMultiByteToUnicode(pdwMode, dwSrcEncoding, pSrcStr, pcSrcSize, (LPWSTR)pDstStr, pcDstSize);
        *pcDstSize *= sizeof(WCHAR);
        return hr;
    }
    else
    {
        INT cDstSizeW;
        LPWSTR pDstStrW;
        HRESULT hr;

        TRACE("convert %s from %ld to %ld\n", debugstr_a(pSrcStr), dwSrcEncoding, dwDstEncoding);

        hr = ConvertINetMultiByteToUnicode(pdwMode, dwSrcEncoding, pSrcStr, pcSrcSize, NULL, &cDstSizeW);
        if (hr != S_OK)
            return hr;

        pDstStrW = malloc(cDstSizeW * sizeof(WCHAR));
        hr = ConvertINetMultiByteToUnicode(pdwMode, dwSrcEncoding, pSrcStr, pcSrcSize, pDstStrW, &cDstSizeW);
        if (hr == S_OK)
            hr = ConvertINetUnicodeToMultiByte(pdwMode, dwDstEncoding, pDstStrW, &cDstSizeW, pDstStr, pcDstSize);

        free(pDstStrW);
        return hr;
    }
}

static HRESULT GetFamilyCodePage(
    UINT uiCodePage,
    UINT* puiFamilyCodePage)
{
    UINT i, n;

    TRACE("%u %p\n", uiCodePage, puiFamilyCodePage);

    if (!puiFamilyCodePage) return S_FALSE;

    for (i = 0; i < ARRAY_SIZE(mlang_data); i++)
    {
        for (n = 0; n < mlang_data[i].number_of_cp; n++)
        {
            if (mlang_data[i].mime_cp_info[n].cp == uiCodePage)
            {
                *puiFamilyCodePage = mlang_data[i].family_codepage;
                return S_OK;
            }
        }
    }

    return S_FALSE;
}

HRESULT WINAPI IsConvertINetStringAvailable(
    DWORD dwSrcEncoding,
    DWORD dwDstEncoding)
{
    UINT src_family, dst_family;

    TRACE("%ld %ld\n", dwSrcEncoding, dwDstEncoding);

    if (GetFamilyCodePage(dwSrcEncoding, &src_family) != S_OK ||
        GetFamilyCodePage(dwDstEncoding, &dst_family) != S_OK)
        return S_FALSE;

    if (src_family == dst_family) return S_OK;

    /* we can convert any codepage to/from unicode */
    if (src_family == CP_UNICODE || dst_family == CP_UNICODE) return S_OK;

    return S_FALSE;
}

static inline HRESULT lcid_to_rfc1766A( LCID lcid, LPSTR rfc1766, INT len )
{
    CHAR buffer[MAX_RFC1766_NAME];
    INT n = GetLocaleInfoA(lcid, LOCALE_SISO639LANGNAME, buffer, MAX_RFC1766_NAME);
    INT i;

    if (n)
    {
        i = PRIMARYLANGID(lcid);
        if ((((i == LANG_ENGLISH) || (i == LANG_CHINESE) || (i == LANG_ARABIC)) &&
            (SUBLANGID(lcid) == SUBLANG_DEFAULT)) ||
            (SUBLANGID(lcid) > SUBLANG_DEFAULT)) {

            buffer[n - 1] = '-';
            i = GetLocaleInfoA(lcid, LOCALE_SISO3166CTRYNAME, buffer + n, MAX_RFC1766_NAME - n);
            if (!i)
                buffer[n - 1] = '\0';
        }
        else
            i = 0;

        LCMapStringA( LOCALE_USER_DEFAULT, LCMAP_LOWERCASE, buffer, n + i, rfc1766, len );
        return ((n + i) > len) ? E_INVALIDARG : S_OK;
    }
    return E_FAIL;
}

static inline HRESULT lcid_to_rfc1766W( LCID lcid, LPWSTR rfc1766, INT len )
{
    WCHAR buffer[MAX_RFC1766_NAME];
    INT n = GetLocaleInfoW(lcid, LOCALE_SISO639LANGNAME, buffer, MAX_RFC1766_NAME);
    INT i;

    if (n)
    {
        i = PRIMARYLANGID(lcid);
        if ((((i == LANG_ENGLISH) || (i == LANG_CHINESE) || (i == LANG_ARABIC)) &&
            (SUBLANGID(lcid) == SUBLANG_DEFAULT)) ||
            (SUBLANGID(lcid) > SUBLANG_DEFAULT)) {

            buffer[n - 1] = '-';
            i = GetLocaleInfoW(lcid, LOCALE_SISO3166CTRYNAME, buffer + n, MAX_RFC1766_NAME - n);
            if (!i)
                buffer[n - 1] = '\0';
        }
        else
            i = 0;

        LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE, buffer, n + i, rfc1766, len);
        return ((n + i) > len) ? E_INVALIDARG : S_OK;
    }
    return E_FAIL;
}

HRESULT WINAPI LcidToRfc1766A(
    LCID lcid,
    LPSTR pszRfc1766,
    INT nChar)
{
    TRACE("%04lx %p %u\n", lcid, pszRfc1766, nChar);
    if (!pszRfc1766)
        return E_INVALIDARG;

    return lcid_to_rfc1766A(lcid, pszRfc1766, nChar);
}

HRESULT WINAPI LcidToRfc1766W(
    LCID lcid,
    LPWSTR pszRfc1766,
    INT nChar)
{
    TRACE("%04lx %p %u\n", lcid, pszRfc1766, nChar);
    if (!pszRfc1766)
        return E_INVALIDARG;

    return lcid_to_rfc1766W(lcid, pszRfc1766, nChar);
}

static HRESULT lcid_from_rfc1766(IEnumRfc1766 *iface, LCID *lcid, LPCWSTR rfc1766)
{
    RFC1766INFO info;
    ULONG num;

    while (IEnumRfc1766_Next(iface, 1, &info, &num) == S_OK)
    {
        if (!wcsicmp(info.wszRfc1766, rfc1766))
        {
            *lcid = info.lcid;
            return S_OK;
        }
        if (lstrlenW(rfc1766) == 2 && !memcmp(info.wszRfc1766, rfc1766, 2 * sizeof(WCHAR)))
        {
            *lcid = PRIMARYLANGID(info.lcid);
            return S_OK;
        }
    }

    return E_FAIL;
}

HRESULT WINAPI Rfc1766ToLcidW(LCID *pLocale, LPCWSTR pszRfc1766)
{
    IEnumRfc1766 *enumrfc1766;
    HRESULT hr;

    TRACE("(%p, %s)\n", pLocale, debugstr_w(pszRfc1766));

    if (!pLocale || !pszRfc1766)
        return E_INVALIDARG;

    hr = EnumRfc1766_create(0, &enumrfc1766);
    if (FAILED(hr))
        return hr;

    hr = lcid_from_rfc1766(enumrfc1766, pLocale, pszRfc1766);
    IEnumRfc1766_Release(enumrfc1766);

    return hr;
}

HRESULT WINAPI Rfc1766ToLcidA(LCID *lcid, LPCSTR rfc1766A)
{
    WCHAR rfc1766W[MAX_RFC1766_NAME + 1];

    if (!rfc1766A)
        return E_INVALIDARG;

    MultiByteToWideChar(CP_ACP, 0, rfc1766A, -1, rfc1766W, MAX_RFC1766_NAME);
    rfc1766W[MAX_RFC1766_NAME] = 0;

    return Rfc1766ToLcidW(lcid, rfc1766W);
}

struct map_font_enum_data
{
    HDC hdc;
    LOGFONTW src_lf;
    HFONT font;
    UINT charset;
    DWORD mask;
};

static INT CALLBACK map_font_enum_proc(const LOGFONTW *lf, const TEXTMETRICW *ntm, DWORD type, LPARAM lParam)
{
    HFONT new_font, old_font;
    FONTSIGNATURE fs;
    UINT charset;
    struct map_font_enum_data *data = (struct map_font_enum_data *)lParam;

    data->src_lf.lfCharSet = lf->lfCharSet;
    wcscpy(data->src_lf.lfFaceName, lf->lfFaceName);

    new_font = CreateFontIndirectW(&data->src_lf);
    if (new_font == NULL) return 1;

    old_font = SelectObject(data->hdc, new_font);
    charset = GetTextCharsetInfo(data->hdc, &fs, 0);
    SelectObject(data->hdc, old_font);

    /* check that the font directly supports the codepage as well (not just through a child font) */
    if (charset == data->charset && fs.fsCsb[0] & data->mask)
    {
        data->font = new_font;
        return 0;
    }
    DeleteObject(new_font);
    return 1;
}

static HRESULT map_font(HDC hdc, DWORD codepages, HFONT src_font, HFONT *dst_font)
{
    struct font_list *font_list_entry;
    CHARSETINFO charset_info;
    LOGFONTW font_attr;
    DWORD mask, Csb[2];
    BOOL found_cached;
    BOOL ret;
    UINT i;
    struct map_font_enum_data enum_data;

    if (hdc == NULL || src_font == NULL) return E_FAIL;

    enum_data.hdc = hdc;
    enum_data.font = NULL;

    GetObjectW(src_font, sizeof(enum_data.src_lf), &enum_data.src_lf);
    enum_data.src_lf.lfWidth = 0;

    for (i = 0; i < 32; i++)
    {
        mask = (DWORD)(1 << i);
        if (codepages & mask)
        {
            Csb[0] = mask;
            Csb[1] = 0x0;
            ret = TranslateCharsetInfo(Csb, &charset_info, TCI_SRCFONTSIG);
            if (!ret) continue;

            /* use cached font if possible */
            found_cached = FALSE;
            EnterCriticalSection(&font_cache_critical);
            LIST_FOR_EACH_ENTRY(font_list_entry, &font_cache, struct font_list, list_entry)
            {
                if (font_list_entry->charset == charset_info.ciCharset &&
                    font_list_entry->base_font == src_font)
                {
                    if (dst_font != NULL)
                        *dst_font = font_list_entry->font;
                    found_cached = TRUE;
                }
            }
            LeaveCriticalSection(&font_cache_critical);
            if (found_cached) return S_OK;

            font_attr.lfCharSet = (BYTE)charset_info.ciCharset;
            font_attr.lfFaceName[0] = 0;
            font_attr.lfPitchAndFamily = 0;

            enum_data.charset = charset_info.ciCharset;
            enum_data.mask = mask;

            if (!EnumFontFamiliesExW(hdc, &font_attr, map_font_enum_proc, (LPARAM)&enum_data, 0))
            {
                font_list_entry = malloc(sizeof(*font_list_entry));
                if (font_list_entry == NULL) return E_OUTOFMEMORY;

                font_list_entry->base_font = src_font;
                font_list_entry->font = enum_data.font;
                font_list_entry->charset = enum_data.charset;

                EnterCriticalSection(&font_cache_critical);
                list_add_tail(&font_cache, &font_list_entry->list_entry);
                LeaveCriticalSection(&font_cache_critical);

                if (dst_font != NULL)
                    *dst_font = enum_data.font;
                return S_OK;
            }
        }
    }
    WARN("couldn't create an appropriate mapped font...\n");
    return E_FAIL;
}

static HRESULT release_font(HFONT font)
{
    struct font_list *font_list_entry;
    HRESULT hr;

    hr = E_FAIL;
    EnterCriticalSection(&font_cache_critical);
    LIST_FOR_EACH_ENTRY(font_list_entry, &font_cache, struct font_list, list_entry)
    {
        if (font_list_entry->font == font)
        {
            list_remove(&font_list_entry->list_entry);
            DeleteObject(font);
            free(font_list_entry);
            hr = S_OK;
            break;
        }
    }
    LeaveCriticalSection(&font_cache_critical);

    return hr;
}

static HRESULT clear_font_cache(void)
{
    struct font_list *font_list_entry;
    struct font_list *font_list_entry2;

    EnterCriticalSection(&font_cache_critical);
    LIST_FOR_EACH_ENTRY_SAFE(font_list_entry, font_list_entry2, &font_cache, struct font_list, list_entry)
    {
        list_remove(&font_list_entry->list_entry);
        DeleteObject(font_list_entry->font);
        free(font_list_entry);
    }
    LeaveCriticalSection(&font_cache_critical);

    return S_OK;
}

/******************************************************************************
 * MLANG ClassFactory
 */
typedef struct {
    IClassFactory IClassFactory_iface;
    LONG ref;
    HRESULT (*pfnCreateInstance)(IUnknown *pUnkOuter, LPVOID *ppObj);
} IClassFactoryImpl;

static inline IClassFactoryImpl *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, IClassFactoryImpl, IClassFactory_iface);
}

struct object_creation_info
{
    const CLSID *clsid;
    LPCSTR szClassName;
    HRESULT (*pfnCreateInstance)(IUnknown *pUnkOuter, LPVOID *ppObj);
};

static const struct object_creation_info object_creation[] =
{
    { &CLSID_CMultiLanguage, "CLSID_CMultiLanguage", MultiLanguage_create },
    { &CLSID_CMLangConvertCharset, "CLSID_CMLangConvertCharset", MLangConvertCharset_create }
};

static HRESULT WINAPI MLANGCF_QueryInterface(IClassFactory *iface, REFIID riid, void **ppobj)
{
    TRACE("%s\n", debugstr_guid(riid) );

    if (IsEqualGUID(riid, &IID_IUnknown)
	|| IsEqualGUID(riid, &IID_IClassFactory))
    {
	IClassFactory_AddRef(iface);
        *ppobj = iface;
	return S_OK;
    }

    *ppobj = NULL;
    WARN("(%p)->(%s,%p), not found\n", iface, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI MLANGCF_AddRef(IClassFactory *iface)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI MLANGCF_Release(IClassFactory *iface)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    if (ref == 0)
    {
        TRACE("Destroying %p\n", This);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI MLANGCF_CreateInstance(IClassFactory *iface, IUnknown *pOuter,
        REFIID riid, void **ppobj)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    HRESULT hres;
    LPUNKNOWN punk;

    TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);

    *ppobj = NULL;
    hres = This->pfnCreateInstance(pOuter, (LPVOID *) &punk);
    if (SUCCEEDED(hres)) {
        hres = IUnknown_QueryInterface(punk, riid, ppobj);
        IUnknown_Release(punk);
    }
    TRACE("returning (%p) -> %lx\n", *ppobj, hres);
    return hres;
}

static HRESULT WINAPI MLANGCF_LockServer(IClassFactory *iface, BOOL dolock)
{
    if (dolock)
        LockModule();
    else
        UnlockModule();

    return S_OK;
}

static const IClassFactoryVtbl MLANGCF_Vtbl =
{
    MLANGCF_QueryInterface,
    MLANGCF_AddRef,
    MLANGCF_Release,
    MLANGCF_CreateInstance,
    MLANGCF_LockServer
};

/******************************************************************
 *		DllGetClassObject (MLANG.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    unsigned int i;
    IClassFactoryImpl *factory;

    TRACE("%s %s %p\n",debugstr_guid(rclsid), debugstr_guid(iid), ppv);

    if ( !IsEqualGUID( &IID_IClassFactory, iid )
	 && ! IsEqualGUID( &IID_IUnknown, iid) )
	return E_NOINTERFACE;

    for (i = 0; i < ARRAY_SIZE(object_creation); i++)
    {
	if (IsEqualGUID(object_creation[i].clsid, rclsid))
	    break;
    }

    if (i == ARRAY_SIZE(object_creation))
    {
	FIXME("%s: no class found.\n", debugstr_guid(rclsid));
	return CLASS_E_CLASSNOTAVAILABLE;
    }

    TRACE("Creating a class factory for %s\n",object_creation[i].szClassName);

    factory = malloc(sizeof(*factory));
    if (factory == NULL) return E_OUTOFMEMORY;

    factory->IClassFactory_iface.lpVtbl = &MLANGCF_Vtbl;
    factory->ref = 1;

    factory->pfnCreateInstance = object_creation[i].pfnCreateInstance;

    *ppv = &factory->IClassFactory_iface;

    TRACE("(%p) <- %p\n", ppv, &factory->IClassFactory_iface);

    return S_OK;
}


/******************************************************************************/

typedef struct tagMLang_impl
{
    IMLangFontLink IMLangFontLink_iface;
    IMultiLanguage IMultiLanguage_iface;
    IMultiLanguage3 IMultiLanguage3_iface;
    IMLangFontLink2 IMLangFontLink2_iface;
    IMLangLineBreakConsole IMLangLineBreakConsole_iface;
    LONG ref;
    DWORD total_cp, total_scripts;
} MLang_impl;

/******************************************************************************/

typedef struct tagEnumCodePage_impl
{
    IEnumCodePage IEnumCodePage_iface;
    LONG ref;
    MIMECPINFO *cpinfo;
    DWORD total, pos;
} EnumCodePage_impl;

static inline EnumCodePage_impl *impl_from_IEnumCodePage( IEnumCodePage *iface )
{
    return CONTAINING_RECORD( iface, EnumCodePage_impl, IEnumCodePage_iface );
}

static HRESULT WINAPI fnIEnumCodePage_QueryInterface(
        IEnumCodePage* iface,
        REFIID riid,
        void** ppvObject)
{
    EnumCodePage_impl *This = impl_from_IEnumCodePage( iface );

    TRACE("%p -> %s\n", This, debugstr_guid(riid) );

    if (IsEqualGUID(riid, &IID_IUnknown)
	|| IsEqualGUID(riid, &IID_IEnumCodePage))
    {
	IEnumCodePage_AddRef(iface);
        TRACE("Returning IID_IEnumCodePage %p ref = %ld\n", This, This->ref);
	*ppvObject = &This->IEnumCodePage_iface;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);
    return E_NOINTERFACE;
}

static ULONG WINAPI fnIEnumCodePage_AddRef(
        IEnumCodePage* iface)
{
    EnumCodePage_impl *This = impl_from_IEnumCodePage( iface );
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI fnIEnumCodePage_Release(
        IEnumCodePage* iface)
{
    EnumCodePage_impl *This = impl_from_IEnumCodePage( iface );
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p ref = %ld\n", This, ref);
    if (ref == 0)
    {
        TRACE("Destroying %p\n", This);
        free(This->cpinfo);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI fnIEnumCodePage_Clone(
        IEnumCodePage* iface,
        IEnumCodePage** ppEnum)
{
    EnumCodePage_impl *This = impl_from_IEnumCodePage( iface );
    FIXME("%p %p\n", This, ppEnum);
    return E_NOTIMPL;
}

static  HRESULT WINAPI fnIEnumCodePage_Next(
        IEnumCodePage* iface,
        ULONG celt,
        PMIMECPINFO rgelt,
        ULONG* pceltFetched)
{
    ULONG i;
    EnumCodePage_impl *This = impl_from_IEnumCodePage( iface );

    TRACE("%p %lu %p %p\n", This, celt, rgelt, pceltFetched);

    if (!pceltFetched) return S_FALSE;
    *pceltFetched = 0;

    if (!rgelt) return S_FALSE;

    if (This->pos + celt > This->total)
        celt = This->total - This->pos;

    if (!celt) return S_FALSE;

    memcpy(rgelt, This->cpinfo + This->pos, celt * sizeof(MIMECPINFO));
    *pceltFetched = celt;
    This->pos += celt;

    for (i = 0; i < celt; i++)
    {
        TRACE("#%lu: %08lx %u %u %s %s %s %s %s %s %d\n",
              i, rgelt[i].dwFlags, rgelt[i].uiCodePage,
              rgelt[i].uiFamilyCodePage,
              wine_dbgstr_w(rgelt[i].wszDescription),
              wine_dbgstr_w(rgelt[i].wszWebCharset),
              wine_dbgstr_w(rgelt[i].wszHeaderCharset),
              wine_dbgstr_w(rgelt[i].wszBodyCharset),
              wine_dbgstr_w(rgelt[i].wszFixedWidthFont),
              wine_dbgstr_w(rgelt[i].wszProportionalFont),
              rgelt[i].bGDICharset);
    }
    return S_OK;
}

static HRESULT WINAPI fnIEnumCodePage_Reset(
        IEnumCodePage* iface)
{
    EnumCodePage_impl *This = impl_from_IEnumCodePage( iface );

    TRACE("%p\n", This);

    This->pos = 0;
    return S_OK;
}

static  HRESULT WINAPI fnIEnumCodePage_Skip(
        IEnumCodePage* iface,
        ULONG celt)
{
    EnumCodePage_impl *This = impl_from_IEnumCodePage( iface );

    TRACE("%p %lu\n", This, celt);

    if (celt >= This->total) return S_FALSE;

    This->pos += celt;
    return S_OK;
}

static const IEnumCodePageVtbl IEnumCodePage_vtbl =
{
    fnIEnumCodePage_QueryInterface,
    fnIEnumCodePage_AddRef,
    fnIEnumCodePage_Release,
    fnIEnumCodePage_Clone,
    fnIEnumCodePage_Next,
    fnIEnumCodePage_Reset,
    fnIEnumCodePage_Skip
};

static HRESULT EnumCodePage_create( MLang_impl* mlang, DWORD grfFlags,
                     LANGID LangId, IEnumCodePage** ppEnumCodePage )
{
    EnumCodePage_impl *ecp;
    MIMECPINFO *cpinfo;
    UINT i, n;

    TRACE("%p, %08lx, %04x, %p\n", mlang, grfFlags, LangId, ppEnumCodePage);

    if (!grfFlags) /* enumerate internal data base of encodings */
        grfFlags = MIMECONTF_MIME_LATEST;

    ecp = malloc(sizeof(EnumCodePage_impl));
    ecp->IEnumCodePage_iface.lpVtbl = &IEnumCodePage_vtbl;
    ecp->ref = 1;
    ecp->pos = 0;
    ecp->total = 0;
    for (i = 0; i < ARRAY_SIZE(mlang_data); i++)
    {
        for (n = 0; n < mlang_data[i].number_of_cp; n++)
        {
            if (mlang_data[i].mime_cp_info[n].flags & grfFlags)
                ecp->total++;
        }
    }

    ecp->cpinfo = malloc(sizeof(MIMECPINFO) * ecp->total);
    cpinfo = ecp->cpinfo;

    for (i = 0; i < ARRAY_SIZE(mlang_data); i++)
    {
        for (n = 0; n < mlang_data[i].number_of_cp; n++)
        {
            if (mlang_data[i].mime_cp_info[n].flags & grfFlags)
                fill_cp_info(&mlang_data[i], n, cpinfo++);
        }
    }

    TRACE("enumerated %ld codepages with flags %08lx\n", ecp->total, grfFlags);

    *ppEnumCodePage = &ecp->IEnumCodePage_iface;

    return S_OK;
}

/******************************************************************************/

typedef struct tagEnumScript_impl
{
    IEnumScript IEnumScript_iface;
    LONG ref;
    SCRIPTINFO *script_info;
    DWORD total, pos;
} EnumScript_impl;

static inline EnumScript_impl *impl_from_IEnumScript( IEnumScript *iface )
{
    return CONTAINING_RECORD( iface, EnumScript_impl, IEnumScript_iface );
}

static HRESULT WINAPI fnIEnumScript_QueryInterface(
        IEnumScript* iface,
        REFIID riid,
        void** ppvObject)
{
    EnumScript_impl *This = impl_from_IEnumScript( iface );

    TRACE("%p -> %s\n", This, debugstr_guid(riid) );

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IEnumScript))
    {
        IEnumScript_AddRef(iface);
        TRACE("Returning IID_IEnumScript %p ref = %ld\n", This, This->ref);
        *ppvObject = &This->IEnumScript_iface;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);
    return E_NOINTERFACE;
}

static ULONG WINAPI fnIEnumScript_AddRef(
        IEnumScript* iface)
{
    EnumScript_impl *This = impl_from_IEnumScript( iface );
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI fnIEnumScript_Release(
        IEnumScript* iface)
{
    EnumScript_impl *This = impl_from_IEnumScript( iface );
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p ref = %ld\n", This, ref);
    if (ref == 0)
    {
        TRACE("Destroying %p\n", This);
        free(This->script_info);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI fnIEnumScript_Clone(
        IEnumScript* iface,
        IEnumScript** ppEnum)
{
    EnumScript_impl *This = impl_from_IEnumScript( iface );
    FIXME("%p %p: stub!\n", This, ppEnum);
    return E_NOTIMPL;
}

static  HRESULT WINAPI fnIEnumScript_Next(
        IEnumScript* iface,
        ULONG celt,
        PSCRIPTINFO rgelt,
        ULONG* pceltFetched)
{
    EnumScript_impl *This = impl_from_IEnumScript( iface );

    TRACE("%p %lu %p %p\n", This, celt, rgelt, pceltFetched);

    if (!pceltFetched || !rgelt) return E_FAIL;

    *pceltFetched = 0;

    if (This->pos + celt > This->total)
        celt = This->total - This->pos;

    if (!celt) return S_FALSE;

    memcpy(rgelt, This->script_info + This->pos, celt * sizeof(SCRIPTINFO));
    *pceltFetched = celt;
    This->pos += celt;

    return S_OK;
}

static HRESULT WINAPI fnIEnumScript_Reset(
        IEnumScript* iface)
{
    EnumScript_impl *This = impl_from_IEnumScript( iface );

    TRACE("%p\n", This);

    This->pos = 0;
    return S_OK;
}

static  HRESULT WINAPI fnIEnumScript_Skip(
        IEnumScript* iface,
        ULONG celt)
{
    EnumScript_impl *This = impl_from_IEnumScript( iface );

    TRACE("%p %lu\n", This, celt);

    if (celt >= This->total) return S_FALSE;

    This->pos += celt;
    return S_OK;
}

static const IEnumScriptVtbl IEnumScript_vtbl =
{
    fnIEnumScript_QueryInterface,
    fnIEnumScript_AddRef,
    fnIEnumScript_Release,
    fnIEnumScript_Clone,
    fnIEnumScript_Next,
    fnIEnumScript_Reset,
    fnIEnumScript_Skip
};

static HRESULT EnumScript_create( MLang_impl* mlang, DWORD dwFlags,
                     LANGID LangId, IEnumScript** ppEnumScript )
{
    EnumScript_impl *es;
    UINT i;

    TRACE("%p, %08lx, %04x, %p\n", mlang, dwFlags, LangId, ppEnumScript);

    if (!dwFlags) /* enumerate all available scripts */
        dwFlags = SCRIPTCONTF_SCRIPT_USER | SCRIPTCONTF_SCRIPT_HIDE | SCRIPTCONTF_SCRIPT_SYSTEM;

    es = malloc(sizeof(EnumScript_impl));
    es->IEnumScript_iface.lpVtbl = &IEnumScript_vtbl;
    es->ref = 1;
    es->pos = 0;
    /* do not enumerate unicode flavours */
    es->total = ARRAY_SIZE(mlang_data) - 1;
    es->script_info = malloc(sizeof(SCRIPTINFO) * es->total);

    for (i = 0; i < es->total; i++)
    {
        es->script_info[i].ScriptId = i;
        es->script_info[i].uiCodePage = mlang_data[i].family_codepage;
        wcscpy( es->script_info[i].wszDescription, mlang_data[i].description );
        wcscpy( es->script_info[i].wszFixedWidthFont, mlang_data[i].fixed_font );
        wcscpy( es->script_info[i].wszProportionalFont, mlang_data[i].proportional_font );
    }

    TRACE("enumerated %ld scripts with flags %08lx\n", es->total, dwFlags);

    *ppEnumScript = &es->IEnumScript_iface;

    return S_OK;
}

/******************************************************************************/

static inline MLang_impl *impl_from_IMLangFontLink( IMLangFontLink *iface )
{
    return CONTAINING_RECORD( iface, MLang_impl, IMLangFontLink_iface );
}

static HRESULT WINAPI fnIMLangFontLink_QueryInterface(
        IMLangFontLink* iface,
        REFIID riid,
        void** ppvObject)
{
    MLang_impl *This = impl_from_IMLangFontLink( iface );
    return IMultiLanguage3_QueryInterface( &This->IMultiLanguage3_iface, riid, ppvObject );
}

static ULONG WINAPI fnIMLangFontLink_AddRef(
        IMLangFontLink* iface)
{
    MLang_impl *This = impl_from_IMLangFontLink( iface );
    return IMultiLanguage3_AddRef( &This->IMultiLanguage3_iface );
}

static ULONG WINAPI fnIMLangFontLink_Release(
        IMLangFontLink* iface)
{
    MLang_impl *This = impl_from_IMLangFontLink( iface );
    return IMultiLanguage3_Release( &This->IMultiLanguage3_iface );
}

static HRESULT WINAPI fnIMLangFontLink_GetCharCodePages(
        IMLangFontLink* iface,
        WCHAR ch_src,
        DWORD* codepages)
{
    MLang_impl *This = impl_from_IMLangFontLink( iface );
    return IMLangFontLink2_GetCharCodePages(&This->IMLangFontLink2_iface, ch_src, codepages);
}

static HRESULT WINAPI fnIMLangFontLink_GetStrCodePages(
        IMLangFontLink* iface,
        const WCHAR* src,
        LONG src_len,
        DWORD priority_cp,
        DWORD* codepages,
        LONG* ret_len)
{
    MLang_impl *This = impl_from_IMLangFontLink( iface );
    return IMLangFontLink2_GetStrCodePages(&This->IMLangFontLink2_iface, src, src_len, priority_cp,
        codepages, ret_len);
}

static HRESULT WINAPI fnIMLangFontLink_CodePageToCodePages(
        IMLangFontLink* iface,
        UINT codepage,
        DWORD* codepages)
{
    MLang_impl *This = impl_from_IMLangFontLink( iface );
    return IMLangFontLink2_CodePageToCodePages(&This->IMLangFontLink2_iface, codepage, codepages);
}

static HRESULT WINAPI fnIMLangFontLink_CodePagesToCodePage(
        IMLangFontLink* iface,
        DWORD codepages,
        UINT def_codepage,
        UINT* codepage)
{
    MLang_impl *This = impl_from_IMLangFontLink(iface);
    return IMLangFontLink2_CodePagesToCodePage(&This->IMLangFontLink2_iface, codepages,
        def_codepage, codepage);
}

static HRESULT WINAPI fnIMLangFontLink_GetFontCodePages(
        IMLangFontLink* iface,
        HDC hdc,
        HFONT hfont,
        DWORD* codepages)
{
    MLang_impl *This = impl_from_IMLangFontLink(iface);
    return IMLangFontLink2_GetFontCodePages(&This->IMLangFontLink2_iface, hdc, hfont, codepages);
}

static HRESULT WINAPI fnIMLangFontLink_MapFont(
        IMLangFontLink* iface,
        HDC hDC,
        DWORD dwCodePages,
        HFONT hSrcFont,
        HFONT* phDestFont)
{
    TRACE("(%p)->%p %08lx %p %p\n",iface, hDC, dwCodePages, hSrcFont, phDestFont);

    return map_font(hDC, dwCodePages, hSrcFont, phDestFont);
}

static HRESULT WINAPI fnIMLangFontLink_ReleaseFont(
        IMLangFontLink* iface,
        HFONT hFont)
{
    TRACE("(%p)->%p\n",iface, hFont);

    return release_font(hFont);
}

static HRESULT WINAPI fnIMLangFontLink_ResetFontMapping(
        IMLangFontLink* iface)
{
    TRACE("(%p)\n",iface);

    return clear_font_cache();
}


static const IMLangFontLinkVtbl IMLangFontLink_vtbl =
{
    fnIMLangFontLink_QueryInterface,
    fnIMLangFontLink_AddRef,
    fnIMLangFontLink_Release,
    fnIMLangFontLink_GetCharCodePages,
    fnIMLangFontLink_GetStrCodePages,
    fnIMLangFontLink_CodePageToCodePages,
    fnIMLangFontLink_CodePagesToCodePage,
    fnIMLangFontLink_GetFontCodePages,
    fnIMLangFontLink_MapFont,
    fnIMLangFontLink_ReleaseFont,
    fnIMLangFontLink_ResetFontMapping,
};

/******************************************************************************/

static inline MLang_impl *impl_from_IMultiLanguage( IMultiLanguage *iface )
{
    return CONTAINING_RECORD( iface, MLang_impl, IMultiLanguage_iface );
}

static HRESULT WINAPI fnIMultiLanguage_QueryInterface(
    IMultiLanguage* iface,
    REFIID riid,
    void** obj)
{
    MLang_impl *This = impl_from_IMultiLanguage( iface );
    return IMultiLanguage3_QueryInterface(&This->IMultiLanguage3_iface, riid, obj);
}

static ULONG WINAPI fnIMultiLanguage_AddRef( IMultiLanguage* iface )
{
    MLang_impl *This = impl_from_IMultiLanguage( iface );
    return IMultiLanguage3_AddRef(&This->IMultiLanguage3_iface);
}

static ULONG WINAPI fnIMultiLanguage_Release( IMultiLanguage* iface )
{
    MLang_impl *This = impl_from_IMultiLanguage( iface );
    return IMultiLanguage3_Release(&This->IMultiLanguage3_iface);
}

static HRESULT WINAPI fnIMultiLanguage_GetNumberOfCodePageInfo(
    IMultiLanguage* iface,
    UINT* cp)
{
    MLang_impl *This = impl_from_IMultiLanguage( iface );
    TRACE("(%p, %p)\n", This, cp);
    return IMultiLanguage3_GetNumberOfCodePageInfo(&This->IMultiLanguage3_iface, cp);
}

static HRESULT WINAPI fnIMultiLanguage_GetCodePageInfo(
    IMultiLanguage* iface,
    UINT uiCodePage,
    PMIMECPINFO pCodePageInfo)
{
    UINT i, n;
    MLang_impl *This = impl_from_IMultiLanguage( iface );

    TRACE("%p, %u, %p\n", This, uiCodePage, pCodePageInfo);

    for (i = 0; i < ARRAY_SIZE(mlang_data); i++)
    {
        for (n = 0; n < mlang_data[i].number_of_cp; n++)
        {
            if (mlang_data[i].mime_cp_info[n].cp == uiCodePage)
            {
                fill_cp_info(&mlang_data[i], n, pCodePageInfo);
                return S_OK;
            }
        }
    }

    return S_FALSE;
}

static HRESULT WINAPI fnIMultiLanguage_GetFamilyCodePage(
    IMultiLanguage* iface,
    UINT cp,
    UINT* family_cp)
{
    MLang_impl *This = impl_from_IMultiLanguage( iface );
    return IMultiLanguage3_GetFamilyCodePage(&This->IMultiLanguage3_iface, cp, family_cp);
}

static HRESULT WINAPI fnIMultiLanguage_EnumCodePages(
    IMultiLanguage* iface,
    DWORD grfFlags,
    IEnumCodePage** ppEnumCodePage)
{
    MLang_impl *This = impl_from_IMultiLanguage( iface );

    TRACE("%p %08lx %p\n", This, grfFlags, ppEnumCodePage);

    return EnumCodePage_create( This, grfFlags, 0, ppEnumCodePage );
}

static HRESULT WINAPI fnIMultiLanguage_GetCharsetInfo(
    IMultiLanguage* iface,
    BSTR Charset,
    PMIMECSETINFO pCharsetInfo)
{
    MLang_impl *This = impl_from_IMultiLanguage( iface );
    return IMultiLanguage3_GetCharsetInfo( &This->IMultiLanguage3_iface, Charset, pCharsetInfo );
}

static HRESULT WINAPI fnIMultiLanguage_IsConvertible(
    IMultiLanguage* iface,
    DWORD src_enc,
    DWORD dst_enc)
{
    MLang_impl *This = impl_from_IMultiLanguage( iface );
    return IMultiLanguage3_IsConvertible(&This->IMultiLanguage3_iface, src_enc, dst_enc);
}

static HRESULT WINAPI fnIMultiLanguage_ConvertString(
    IMultiLanguage* iface,
    DWORD* mode,
    DWORD src_enc,
    DWORD dst_enc,
    BYTE* src,
    UINT* src_size,
    BYTE* dest,
    UINT* dest_size)
{
    MLang_impl *This = impl_from_IMultiLanguage( iface );
    return IMultiLanguage3_ConvertString(&This->IMultiLanguage3_iface, mode, src_enc,
        dst_enc, src, src_size, dest, dest_size);
}

static HRESULT WINAPI fnIMultiLanguage_ConvertStringToUnicode(
    IMultiLanguage* iface,
    DWORD* mode,
    DWORD src_enc,
    CHAR* src,
    UINT* src_size,
    WCHAR* dest,
    UINT* dest_size)
{
    MLang_impl *This = impl_from_IMultiLanguage( iface );
    return IMultiLanguage3_ConvertStringToUnicode(&This->IMultiLanguage3_iface,
        mode, src_enc, src, src_size, dest, dest_size);
}

static HRESULT WINAPI fnIMultiLanguage_ConvertStringFromUnicode(
    IMultiLanguage* iface,
    DWORD* mode,
    DWORD encoding,
    WCHAR* src,
    UINT* src_size,
    CHAR* dest,
    UINT* dest_size)
{
    MLang_impl *This = impl_from_IMultiLanguage(iface);
    return IMultiLanguage3_ConvertStringFromUnicode(&This->IMultiLanguage3_iface,
        mode, encoding, src, src_size, dest, dest_size);
}

static HRESULT WINAPI fnIMultiLanguage_ConvertStringReset(
    IMultiLanguage* iface)
{
    MLang_impl *This = impl_from_IMultiLanguage( iface );
    return IMultiLanguage3_ConvertStringReset(&This->IMultiLanguage3_iface);
}

static HRESULT WINAPI fnIMultiLanguage_GetRfc1766FromLcid(
    IMultiLanguage* iface,
    LCID lcid,
    BSTR* pbstrRfc1766)
{
    MLang_impl *This = impl_from_IMultiLanguage(iface);
    return IMultiLanguage3_GetRfc1766FromLcid(&This->IMultiLanguage3_iface, lcid, pbstrRfc1766);
}

static HRESULT WINAPI fnIMultiLanguage_GetLcidFromRfc1766(
    IMultiLanguage* iface,
    LCID* locale,
    BSTR rfc1766)
{
    MLang_impl *This = impl_from_IMultiLanguage(iface);
    return IMultiLanguage3_GetLcidFromRfc1766(&This->IMultiLanguage3_iface, locale, rfc1766);
}

/******************************************************************************/

typedef struct tagEnumRfc1766_impl
{
    IEnumRfc1766 IEnumRfc1766_iface;
    LONG ref;
    RFC1766INFO *info;
    DWORD total, pos;
} EnumRfc1766_impl;

static inline EnumRfc1766_impl *impl_from_IEnumRfc1766( IEnumRfc1766 *iface )
{
    return CONTAINING_RECORD( iface, EnumRfc1766_impl, IEnumRfc1766_iface );
}

static HRESULT WINAPI fnIEnumRfc1766_QueryInterface(
        IEnumRfc1766 *iface,
        REFIID riid,
        void** ppvObject)
{
    EnumRfc1766_impl *This = impl_from_IEnumRfc1766( iface );

    TRACE("%p -> %s\n", This, debugstr_guid(riid) );

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IEnumRfc1766))
    {
        IEnumRfc1766_AddRef(iface);
        TRACE("Returning IID_IEnumRfc1766 %p ref = %ld\n", This, This->ref);
        *ppvObject = &This->IEnumRfc1766_iface;
        return S_OK;
    }

    WARN("(%p) -> (%s,%p), not found\n",This,debugstr_guid(riid),ppvObject);
    return E_NOINTERFACE;
}

static ULONG WINAPI fnIEnumRfc1766_AddRef(
        IEnumRfc1766 *iface)
{
    EnumRfc1766_impl *This = impl_from_IEnumRfc1766( iface );
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI fnIEnumRfc1766_Release(
        IEnumRfc1766 *iface)
{
    EnumRfc1766_impl *This = impl_from_IEnumRfc1766( iface );
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p ref = %ld\n", This, ref);
    if (ref == 0)
    {
        TRACE("Destroying %p\n", This);
        free(This->info);
        free(This);
    }
    return ref;
}

static HRESULT WINAPI fnIEnumRfc1766_Clone(
        IEnumRfc1766 *iface,
        IEnumRfc1766 **ppEnum)
{
    EnumRfc1766_impl *This = impl_from_IEnumRfc1766( iface );

    FIXME("%p %p\n", This, ppEnum);
    return E_NOTIMPL;
}

static  HRESULT WINAPI fnIEnumRfc1766_Next(
        IEnumRfc1766 *iface,
        ULONG celt,
        PRFC1766INFO rgelt,
        ULONG *pceltFetched)
{
    ULONG i;
    EnumRfc1766_impl *This = impl_from_IEnumRfc1766( iface );

    TRACE("%p %lu %p %p\n", This, celt, rgelt, pceltFetched);

    if (!pceltFetched) return S_FALSE;
    *pceltFetched = 0;

    if (!rgelt) return S_FALSE;

    if (This->pos + celt > This->total)
        celt = This->total - This->pos;

    if (!celt) return S_FALSE;

    memcpy(rgelt, This->info + This->pos, celt * sizeof(RFC1766INFO));
    *pceltFetched = celt;
    This->pos += celt;

    for (i = 0; i < celt; i++)
    {
        TRACE("#%lu: %08lx %s %s\n",
              i, rgelt[i].lcid,
              wine_dbgstr_w(rgelt[i].wszRfc1766),
              wine_dbgstr_w(rgelt[i].wszLocaleName));
    }
    return S_OK;
}

static HRESULT WINAPI fnIEnumRfc1766_Reset(
        IEnumRfc1766 *iface)
{
    EnumRfc1766_impl *This = impl_from_IEnumRfc1766( iface );

    TRACE("%p\n", This);

    This->pos = 0;
    return S_OK;
}

static  HRESULT WINAPI fnIEnumRfc1766_Skip(
        IEnumRfc1766 *iface,
        ULONG celt)
{
    EnumRfc1766_impl *This = impl_from_IEnumRfc1766( iface );

    TRACE("%p %lu\n", This, celt);

    if (celt >= This->total) return S_FALSE;

    This->pos += celt;
    return S_OK;
}

static const IEnumRfc1766Vtbl IEnumRfc1766_vtbl =
{
    fnIEnumRfc1766_QueryInterface,
    fnIEnumRfc1766_AddRef,
    fnIEnumRfc1766_Release,
    fnIEnumRfc1766_Clone,
    fnIEnumRfc1766_Next,
    fnIEnumRfc1766_Reset,
    fnIEnumRfc1766_Skip
};

struct enum_locales_data
{
    RFC1766INFO *info;
    DWORD total, allocated;
};

static BOOL CALLBACK enum_locales_proc(LPWSTR locale, DWORD flags, LPARAM lparam)
{
    struct enum_locales_data *data = (struct enum_locales_data *)lparam;
    RFC1766INFO *info;

    TRACE("%s\n", debugstr_w(locale));

    if (data->total >= data->allocated)
    {
        data->allocated *= 2;
        data->info = realloc(data->info, data->allocated * sizeof(RFC1766INFO));
        if (!data->info) return FALSE;
    }

    info = &data->info[data->total];

    info->lcid = LocaleNameToLCID( locale, 0 );
    if (info->lcid == LOCALE_CUSTOM_UNSPECIFIED) return TRUE;

    info->wszRfc1766[0] = 0;
    if (FAILED( lcid_to_rfc1766W( info->lcid, info->wszRfc1766, MAX_RFC1766_NAME ))) return TRUE;

    info->wszLocaleName[0] = 0;
    GetLocaleInfoW(info->lcid, LOCALE_SLANGUAGE, info->wszLocaleName, MAX_LOCALE_NAME);
    TRACE("ISO639: %s SLANGUAGE: %s\n", wine_dbgstr_w(info->wszRfc1766), wine_dbgstr_w(info->wszLocaleName));

    data->total++;

    return TRUE;
}

static HRESULT EnumRfc1766_create(LANGID LangId, IEnumRfc1766 **ppEnum)
{
    EnumRfc1766_impl *rfc;
    struct enum_locales_data data;

    TRACE("%04x, %p\n", LangId, ppEnum);

    rfc = malloc(sizeof(EnumRfc1766_impl));
    rfc->IEnumRfc1766_iface.lpVtbl = &IEnumRfc1766_vtbl;
    rfc->ref = 1;
    rfc->pos = 0;
    rfc->total = 0;

    data.total = 0;
    data.allocated = 160;
    data.info = malloc(data.allocated * sizeof(RFC1766INFO));
    if (!data.info)
    {
        free(rfc);
        return E_OUTOFMEMORY;
    }

    EnumSystemLocalesEx(enum_locales_proc, LOCALE_WINDOWS, (LPARAM)&data, NULL);

    TRACE("enumerated %ld rfc1766 structures\n", data.total);

    if (!data.total)
    {
        free(data.info);
        free(rfc);
        return E_FAIL;
    }

    rfc->info = data.info;
    rfc->total = data.total;

    *ppEnum = &rfc->IEnumRfc1766_iface;
    return S_OK;
}

static HRESULT WINAPI fnIMultiLanguage_EnumRfc1766(
    IMultiLanguage *iface,
    IEnumRfc1766 **ppEnumRfc1766)
{
    MLang_impl *This = impl_from_IMultiLanguage( iface );

    TRACE("%p %p\n", This, ppEnumRfc1766);

    return EnumRfc1766_create(0, ppEnumRfc1766);
}

/******************************************************************************/

static HRESULT WINAPI fnIMultiLanguage_GetRfc1766Info(
    IMultiLanguage* iface,
    LCID Locale,
    PRFC1766INFO pRfc1766Info)
{
    LCTYPE type = LOCALE_SLANGUAGE;

    TRACE("(%p, 0x%04lx, %p)\n", iface, Locale, pRfc1766Info);

    if (!pRfc1766Info)
        return E_INVALIDARG;

    if ((PRIMARYLANGID(Locale) == LANG_ENGLISH) ||
        (PRIMARYLANGID(Locale) == LANG_CHINESE) ||
        (PRIMARYLANGID(Locale) == LANG_ARABIC)) {

        if (!SUBLANGID(Locale))
            type = LOCALE_SENGLANGUAGE; /* suppress country */
    }
    else
    {
        if (!SUBLANGID(Locale)) {
            TRACE("SUBLANGID missing in 0x%04lx\n", Locale);
            return E_FAIL;
        }
    }

    pRfc1766Info->lcid = Locale;
    pRfc1766Info->wszRfc1766[0] = 0;
    pRfc1766Info->wszLocaleName[0] = 0;

    if ((!lcid_to_rfc1766W(Locale, pRfc1766Info->wszRfc1766, MAX_RFC1766_NAME)) &&
        (GetLocaleInfoW(Locale, type, pRfc1766Info->wszLocaleName, MAX_LOCALE_NAME) > 0))
            return S_OK;

    /* Locale not supported */
    return E_INVALIDARG;
}

static HRESULT WINAPI fnIMultiLanguage_CreateConvertCharset(
    IMultiLanguage* iface,
    UINT src_cp,
    UINT dst_cp,
    DWORD prop,
    IMLangConvertCharset** convert_charset)
{
    MLang_impl *This = impl_from_IMultiLanguage(iface);
    return IMultiLanguage3_CreateConvertCharset(&This->IMultiLanguage3_iface, src_cp, dst_cp, prop, convert_charset);
}

static const IMultiLanguageVtbl IMultiLanguage_vtbl =
{
    fnIMultiLanguage_QueryInterface,
    fnIMultiLanguage_AddRef,
    fnIMultiLanguage_Release,
    fnIMultiLanguage_GetNumberOfCodePageInfo,
    fnIMultiLanguage_GetCodePageInfo,
    fnIMultiLanguage_GetFamilyCodePage,
    fnIMultiLanguage_EnumCodePages,
    fnIMultiLanguage_GetCharsetInfo,
    fnIMultiLanguage_IsConvertible,
    fnIMultiLanguage_ConvertString,
    fnIMultiLanguage_ConvertStringToUnicode,
    fnIMultiLanguage_ConvertStringFromUnicode,
    fnIMultiLanguage_ConvertStringReset,
    fnIMultiLanguage_GetRfc1766FromLcid,
    fnIMultiLanguage_GetLcidFromRfc1766,
    fnIMultiLanguage_EnumRfc1766,
    fnIMultiLanguage_GetRfc1766Info,
    fnIMultiLanguage_CreateConvertCharset,
};


/******************************************************************************/

static inline MLang_impl *impl_from_IMultiLanguage3( IMultiLanguage3 *iface )
{
    return CONTAINING_RECORD( iface, MLang_impl, IMultiLanguage3_iface );
}

static HRESULT WINAPI fnIMultiLanguage3_QueryInterface(
    IMultiLanguage3* iface,
    REFIID riid,
    void** obj)
{
    MLang_impl *This = impl_from_IMultiLanguage3( iface );

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IMultiLanguage))
    {
        *obj = &This->IMultiLanguage_iface;
    }
    else if (IsEqualGUID(riid, &IID_IMLangCodePages) ||
             IsEqualGUID(riid, &IID_IMLangFontLink))
    {
        *obj = &This->IMLangFontLink_iface;
    }
    else if (IsEqualGUID(riid, &IID_IMLangFontLink2))
    {
        *obj = &This->IMLangFontLink2_iface;
    }
    else if (IsEqualGUID(riid, &IID_IMultiLanguage2) ||
             IsEqualGUID(riid, &IID_IMultiLanguage3))
    {
        *obj = &This->IMultiLanguage3_iface;
    }
    else if (IsEqualGUID(riid, &IID_IMLangLineBreakConsole))
    {
        *obj = &This->IMLangLineBreakConsole_iface;
    }
    else
    {
        WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), obj);
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IMultiLanguage3_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI fnIMultiLanguage3_AddRef( IMultiLanguage3* iface )
{
    MLang_impl *This = impl_from_IMultiLanguage3( iface );
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI fnIMultiLanguage3_Release( IMultiLanguage3* iface )
{
    MLang_impl *This = impl_from_IMultiLanguage3( iface );
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%ld)\n", This, ref);
    if (ref == 0)
    {
        free(This);
        UnlockModule();
    }

    return ref;
}

static HRESULT WINAPI fnIMultiLanguage3_GetNumberOfCodePageInfo(
    IMultiLanguage3* iface,
    UINT* pcCodePage)
{
    MLang_impl *This = impl_from_IMultiLanguage3( iface );

    TRACE("%p, %p\n", This, pcCodePage);

    if (!pcCodePage) return E_INVALIDARG;

    *pcCodePage = This->total_cp;
    return S_OK;
}

static void fill_cp_info(const struct mlang_data *ml_data, UINT index, MIMECPINFO *mime_cp_info)
{
    CHARSETINFO csi;

    if (TranslateCharsetInfo((DWORD*)(DWORD_PTR)ml_data->family_codepage, &csi,
                             TCI_SRCCODEPAGE))
        mime_cp_info->bGDICharset = csi.ciCharset;
    else
        mime_cp_info->bGDICharset = DEFAULT_CHARSET;

    mime_cp_info->dwFlags = ml_data->mime_cp_info[index].flags;
    mime_cp_info->uiCodePage = ml_data->mime_cp_info[index].cp;
    mime_cp_info->uiFamilyCodePage = ml_data->family_codepage;
    wcscpy( mime_cp_info->wszDescription, ml_data->mime_cp_info[index].description );
    wcscpy( mime_cp_info->wszWebCharset, ml_data->mime_cp_info[index].web_charset );
    wcscpy( mime_cp_info->wszHeaderCharset, ml_data->mime_cp_info[index].header_charset );
    wcscpy( mime_cp_info->wszBodyCharset, ml_data->mime_cp_info[index].body_charset );
    wcscpy( mime_cp_info->wszFixedWidthFont, ml_data->fixed_font );
    wcscpy( mime_cp_info->wszProportionalFont, ml_data->proportional_font );

    TRACE("%08lx %u %u %s %s %s %s %s %s %d\n",
          mime_cp_info->dwFlags, mime_cp_info->uiCodePage,
          mime_cp_info->uiFamilyCodePage,
          wine_dbgstr_w(mime_cp_info->wszDescription),
          wine_dbgstr_w(mime_cp_info->wszWebCharset),
          wine_dbgstr_w(mime_cp_info->wszHeaderCharset),
          wine_dbgstr_w(mime_cp_info->wszBodyCharset),
          wine_dbgstr_w(mime_cp_info->wszFixedWidthFont),
          wine_dbgstr_w(mime_cp_info->wszProportionalFont),
          mime_cp_info->bGDICharset);
}

static HRESULT WINAPI fnIMultiLanguage3_GetCodePageInfo(
    IMultiLanguage3* iface,
    UINT uiCodePage,
    LANGID LangId,
    PMIMECPINFO pCodePageInfo)
{
    UINT i, n;
    MLang_impl *This = impl_from_IMultiLanguage3( iface );

    TRACE("%p, %u, %04x, %p\n", This, uiCodePage, LangId, pCodePageInfo);

    for (i = 0; i < ARRAY_SIZE(mlang_data); i++)
    {
        for (n = 0; n < mlang_data[i].number_of_cp; n++)
        {
            if (mlang_data[i].mime_cp_info[n].cp == uiCodePage)
            {
                fill_cp_info(&mlang_data[i], n, pCodePageInfo);
                return S_OK;
            }
        }
    }

    return S_FALSE;
}

static HRESULT WINAPI fnIMultiLanguage3_GetFamilyCodePage(
    IMultiLanguage3* iface,
    UINT uiCodePage,
    UINT* puiFamilyCodePage)
{
    return GetFamilyCodePage(uiCodePage, puiFamilyCodePage);
}

static HRESULT WINAPI fnIMultiLanguage3_EnumCodePages(
    IMultiLanguage3* iface,
    DWORD grfFlags,
    LANGID LangId,
    IEnumCodePage** ppEnumCodePage)
{
    MLang_impl *This = impl_from_IMultiLanguage3( iface );

    TRACE("%p %08lx %04x %p\n", This, grfFlags, LangId, ppEnumCodePage);

    return EnumCodePage_create( This, grfFlags, LangId, ppEnumCodePage );
}

static HRESULT WINAPI fnIMultiLanguage3_GetCharsetInfo(
    IMultiLanguage3* iface,
    BSTR Charset,
    PMIMECSETINFO pCharsetInfo)
{
    UINT i, n;
    MLang_impl *This = impl_from_IMultiLanguage3( iface );

    TRACE("%p %s %p\n", This, debugstr_w(Charset), pCharsetInfo);

    if (!pCharsetInfo) return E_FAIL;

    for (i = 0; i < ARRAY_SIZE(mlang_data); i++)
    {
        for (n = 0; n < mlang_data[i].number_of_cp; n++)
        {
            if (!lstrcmpiW(Charset, mlang_data[i].mime_cp_info[n].web_charset))
            {
                pCharsetInfo->uiCodePage = mlang_data[i].family_codepage;
                pCharsetInfo->uiInternetEncoding = mlang_data[i].mime_cp_info[n].cp;
                lstrcpyW(pCharsetInfo->wszCharset, mlang_data[i].mime_cp_info[n].web_charset);
                return S_OK;
            }
            if (mlang_data[i].mime_cp_info[n].alias && !lstrcmpiW(Charset, mlang_data[i].mime_cp_info[n].alias))
            {
                pCharsetInfo->uiCodePage = mlang_data[i].family_codepage;
                pCharsetInfo->uiInternetEncoding = mlang_data[i].mime_cp_info[n].cp;
                lstrcpyW(pCharsetInfo->wszCharset, mlang_data[i].mime_cp_info[n].alias);
                return S_OK;
            }
        }
    }

    /* FIXME:
     * Since we do not support charsets like iso-2022-jp and do not have
     * them in our database as a primary (web_charset) encoding this loop
     * does an attempt to 'approximate' charset name by header_charset.
     */
    for (i = 0; i < ARRAY_SIZE(mlang_data); i++)
    {
        for (n = 0; n < mlang_data[i].number_of_cp; n++)
        {
            if (!lstrcmpiW(Charset, mlang_data[i].mime_cp_info[n].header_charset))
            {
                pCharsetInfo->uiCodePage = mlang_data[i].family_codepage;
                pCharsetInfo->uiInternetEncoding = mlang_data[i].mime_cp_info[n].cp;
                lstrcpyW(pCharsetInfo->wszCharset, mlang_data[i].mime_cp_info[n].header_charset);
                return S_OK;
            }
        }
    }

    return E_FAIL;
}

static HRESULT WINAPI fnIMultiLanguage3_IsConvertible(
    IMultiLanguage3* iface,
    DWORD dwSrcEncoding,
    DWORD dwDstEncoding)
{
    return IsConvertINetStringAvailable(dwSrcEncoding, dwDstEncoding);
}

static HRESULT WINAPI fnIMultiLanguage3_ConvertString(
    IMultiLanguage3* iface,
    DWORD* pdwMode,
    DWORD dwSrcEncoding,
    DWORD dwDstEncoding,
    BYTE* pSrcStr,
    UINT* pcSrcSize,
    BYTE* pDstStr,
    UINT* pcDstSize)
{
    return ConvertINetString(pdwMode, dwSrcEncoding, dwDstEncoding,
        (LPCSTR)pSrcStr, (LPINT)pcSrcSize, (LPSTR)pDstStr, (LPINT)pcDstSize);
}

static HRESULT WINAPI fnIMultiLanguage3_ConvertStringToUnicode(
    IMultiLanguage3* iface,
    DWORD* pdwMode,
    DWORD dwEncoding,
    CHAR* pSrcStr,
    UINT* pcSrcSize,
    WCHAR* pDstStr,
    UINT* pcDstSize)
{
    return ConvertINetMultiByteToUnicode(pdwMode, dwEncoding,
        pSrcStr, (LPINT)pcSrcSize, pDstStr, (LPINT)pcDstSize);
}

static HRESULT WINAPI fnIMultiLanguage3_ConvertStringFromUnicode(
    IMultiLanguage3* iface,
    DWORD* pdwMode,
    DWORD dwEncoding,
    WCHAR* pSrcStr,
    UINT* pcSrcSize,
    CHAR* pDstStr,
    UINT* pcDstSize)
{
    return ConvertINetUnicodeToMultiByte(pdwMode, dwEncoding,
        pSrcStr, (LPINT)pcSrcSize, pDstStr, (LPINT)pcDstSize);
}

static HRESULT WINAPI fnIMultiLanguage3_ConvertStringReset(
    IMultiLanguage3* iface)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIMultiLanguage3_GetRfc1766FromLcid(
    IMultiLanguage3* iface,
    LCID lcid,
    BSTR* pbstrRfc1766)
{
    WCHAR buf[MAX_RFC1766_NAME];

    TRACE("%p %04lx %p\n", iface, lcid, pbstrRfc1766);
    if (!pbstrRfc1766)
        return E_INVALIDARG;

    if (!lcid_to_rfc1766W( lcid, buf, MAX_RFC1766_NAME ))
    {
        *pbstrRfc1766 = SysAllocString( buf );
        return S_OK;
    }
    return E_FAIL;
}

static HRESULT WINAPI fnIMultiLanguage3_GetLcidFromRfc1766(
    IMultiLanguage3* iface,
    LCID* pLocale,
    BSTR bstrRfc1766)
{
    HRESULT hr;
    IEnumRfc1766 *rfc1766;

    TRACE("%p %p %s\n", iface, pLocale, debugstr_w(bstrRfc1766));

    if (!pLocale || !bstrRfc1766)
        return E_INVALIDARG;

    hr = IMultiLanguage3_EnumRfc1766(iface, 0, &rfc1766);
    if (FAILED(hr))
        return hr;

    hr = lcid_from_rfc1766(rfc1766, pLocale, bstrRfc1766);

    IEnumRfc1766_Release(rfc1766);
    return hr;
}

static HRESULT WINAPI fnIMultiLanguage3_EnumRfc1766(
    IMultiLanguage3* iface,
    LANGID LangId,
    IEnumRfc1766** ppEnumRfc1766)
{
    MLang_impl *This = impl_from_IMultiLanguage3( iface );

    TRACE("%p %p\n", This, ppEnumRfc1766);

    return EnumRfc1766_create(LangId, ppEnumRfc1766);
}

static HRESULT WINAPI fnIMultiLanguage3_GetRfc1766Info(
    IMultiLanguage3* iface,
    LCID Locale,
    LANGID LangId,
    PRFC1766INFO pRfc1766Info)
{
    static LANGID last_lang = -1;
    LCTYPE type = LOCALE_SLANGUAGE;

    TRACE("(%p, 0x%04lx, 0x%04x, %p)\n", iface, Locale, LangId, pRfc1766Info);

    if (!pRfc1766Info)
        return E_INVALIDARG;

    if ((PRIMARYLANGID(Locale) == LANG_ENGLISH) ||
        (PRIMARYLANGID(Locale) == LANG_CHINESE) ||
        (PRIMARYLANGID(Locale) == LANG_ARABIC)) {

        if (!SUBLANGID(Locale))
            type = LOCALE_SENGLANGUAGE; /* suppress country */
    }
    else
    {
        if (!SUBLANGID(Locale)) {
            TRACE("SUBLANGID missing in 0x%04lx\n", Locale);
            return E_FAIL;
        }
    }

    pRfc1766Info->lcid = Locale;
    pRfc1766Info->wszRfc1766[0] = 0;
    pRfc1766Info->wszLocaleName[0] = 0;

    if ((PRIMARYLANGID(LangId) != LANG_ENGLISH) &&
        (last_lang != LangId)) {
        FIXME("Only English names supported (requested: 0x%04x)\n", LangId);
        last_lang = LangId;
    }

    if ((!lcid_to_rfc1766W(Locale, pRfc1766Info->wszRfc1766, MAX_RFC1766_NAME)) &&
        (GetLocaleInfoW(Locale, type, pRfc1766Info->wszLocaleName, MAX_LOCALE_NAME) > 0))
            return S_OK;

    /* Locale not supported */
    return E_INVALIDARG;
}

static HRESULT WINAPI fnIMultiLanguage3_CreateConvertCharset(
    IMultiLanguage3* iface,
    UINT src_cp,
    UINT dst_cp,
    DWORD prop,
    IMLangConvertCharset** convert_charset)
{
    HRESULT hr;

    TRACE("(%u %u 0x%08lx %p)\n", src_cp, dst_cp, prop, convert_charset);

    hr = MLangConvertCharset_create(NULL, (void**)convert_charset);
    if (FAILED(hr)) return hr;

    return IMLangConvertCharset_Initialize(*convert_charset, src_cp, dst_cp, prop);
}

static HRESULT WINAPI fnIMultiLanguage3_ConvertStringInIStream(
    IMultiLanguage3* iface,
    DWORD* pdwMode,
    DWORD dwFlag,
    WCHAR* lpFallBack,
    DWORD dwSrcEncoding,
    DWORD dwDstEncoding,
    IStream* pstmIn,
    IStream* pstmOut)
{
    char *src, *dst = NULL;
    INT srclen, dstlen;
    STATSTG stat;
    HRESULT hr;

    TRACE("%p %0lx8 %s %lu %lu %p %p\n",
          pdwMode, dwFlag, debugstr_w(lpFallBack), dwSrcEncoding, dwDstEncoding, pstmIn, pstmOut);

    FIXME("dwFlag and lpFallBack not handled\n");

    hr = IStream_Stat(pstmIn, &stat, STATFLAG_NONAME);
    if (FAILED(hr)) return hr;

    if (stat.cbSize.QuadPart > MAXLONG) return E_INVALIDARG;
    if (!(src = malloc(stat.cbSize.QuadPart))) return E_OUTOFMEMORY;

    hr = IStream_Read(pstmIn, src, stat.cbSize.QuadPart, (ULONG *)&srclen);
    if (FAILED(hr)) goto exit;

    hr = ConvertINetString(pdwMode, dwSrcEncoding, dwDstEncoding, src, &srclen, NULL, &dstlen);
    if (FAILED(hr)) goto exit;

    if (!(dst = malloc(dstlen)))
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    hr = ConvertINetString(pdwMode, dwSrcEncoding, dwDstEncoding, src, &srclen, dst, &dstlen);
    if (FAILED(hr)) goto exit;

    hr = IStream_Write(pstmOut, dst, dstlen, NULL);

exit:
    free(src);
    free(dst);
    return hr;
}

static HRESULT WINAPI fnIMultiLanguage3_ConvertStringToUnicodeEx(
    IMultiLanguage3* iface,
    DWORD* pdwMode,
    DWORD dwEncoding,
    CHAR* pSrcStr,
    UINT* pcSrcSize,
    WCHAR* pDstStr,
    UINT* pcDstSize,
    DWORD dwFlag,
    WCHAR* lpFallBack)
{
    if (dwFlag || lpFallBack)
        FIXME("Ignoring dwFlag (0x%lx/%ld) and lpFallBack (%p)\n",
                dwFlag, dwFlag, lpFallBack);

    return ConvertINetMultiByteToUnicode(pdwMode, dwEncoding,
        pSrcStr, (LPINT)pcSrcSize, pDstStr, (LPINT)pcDstSize);
}

/*****************************************************************************
 * MultiLanguage2::ConvertStringToUnicodeEx
 *
 * Translates the multibyte string from the specified code page to Unicode.
 *
 * PARAMS
 *   see ConvertStringToUnicode
 *   dwFlag
 *   lpFallBack if dwFlag contains MLCONVCHARF_USEDEFCHAR, lpFallBack string used
 *              instead unconvertible characters.
 *
 * RETURNS
 *   S_OK     Success.
 *   S_FALSE  The conversion is not supported.
 *   E_FAIL   Some error has occurred.
 *
 * TODO: handle dwFlag and lpFallBack
*/
static HRESULT WINAPI fnIMultiLanguage3_ConvertStringFromUnicodeEx(
    IMultiLanguage3* This,
    DWORD* pdwMode,
    DWORD dwEncoding,
    WCHAR* pSrcStr,
    UINT* pcSrcSize,
    CHAR* pDstStr,
    UINT* pcDstSize,
    DWORD dwFlag,
    WCHAR* lpFallBack)
{
    FIXME("\n");
    return ConvertINetUnicodeToMultiByte(pdwMode, dwEncoding,
        pSrcStr, (LPINT)pcSrcSize, pDstStr, (LPINT)pcDstSize);
}

static HRESULT WINAPI fnIMultiLanguage3_DetectCodepageInIStream(
    IMultiLanguage3* iface,
    DWORD dwFlag,
    DWORD dwPrefWinCodePage,
    IStream* pstmIn,
    DetectEncodingInfo* lpEncoding,
    INT* pnScores)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIMultiLanguage3_DetectInputCodepage(
    IMultiLanguage3* iface,
    DWORD dwFlag,
    DWORD dwPrefWinCodePage,
    CHAR* pSrcStr,
    INT* pcSrcSize,
    DetectEncodingInfo* lpEncoding,
    INT* pnScores)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIMultiLanguage3_ValidateCodePage(
    IMultiLanguage3* iface,
    UINT uiCodePage,
    HWND hwnd)
{
    return IMultiLanguage3_ValidateCodePageEx(iface,uiCodePage,hwnd,0);
}

static HRESULT WINAPI fnIMultiLanguage3_GetCodePageDescription(
    IMultiLanguage3* iface,
    UINT uiCodePage,
    LCID lcid,
    LPWSTR lpWideCharStr,
    int cchWideChar)
{
    /* Find first instance */
    unsigned int i,n;

    TRACE ("%u, %04lx, %p, %d\n", uiCodePage, lcid, lpWideCharStr, cchWideChar);
    for (i = 0; i < ARRAY_SIZE(mlang_data); i++)
    {
        for (n = 0; n < mlang_data[i].number_of_cp; n++)
        {
            if (mlang_data[i].mime_cp_info[n].cp == uiCodePage)
            {
                lstrcpynW( lpWideCharStr, mlang_data[i].mime_cp_info[n].description, cchWideChar);
                return S_OK;
            }
        }
    }

    return S_FALSE;
}

static HRESULT WINAPI fnIMultiLanguage3_IsCodePageInstallable(
    IMultiLanguage3* iface,
    UINT uiCodePage)
{
    TRACE("%u\n", uiCodePage);

    /* FIXME: the installable set is usually larger than the set of valid codepages */
    return IMultiLanguage3_ValidateCodePageEx(iface, uiCodePage, NULL, CPIOD_PEEK);
}

static HRESULT WINAPI fnIMultiLanguage3_SetMimeDBSource(
    IMultiLanguage3* iface,
    MIMECONTF dwSource)
{
    FIXME("0x%08x\n", dwSource);
    return S_OK;
}

static HRESULT WINAPI fnIMultiLanguage3_GetNumberOfScripts(
    IMultiLanguage3* iface,
    UINT* pnScripts)
{
    MLang_impl *This = impl_from_IMultiLanguage3( iface );

    TRACE("%p %p\n", This, pnScripts);

    if (!pnScripts) return S_FALSE;

    *pnScripts = This->total_scripts;
    return S_OK;
}

static HRESULT WINAPI fnIMultiLanguage3_EnumScripts(
    IMultiLanguage3* iface,
    DWORD dwFlags,
    LANGID LangId,
    IEnumScript** ppEnumScript)
{
    MLang_impl *This = impl_from_IMultiLanguage3( iface );

    TRACE("%p %08lx %04x %p\n", This, dwFlags, LangId, ppEnumScript);

    return EnumScript_create( This, dwFlags, LangId, ppEnumScript );
}

static HRESULT WINAPI fnIMultiLanguage3_ValidateCodePageEx(
    IMultiLanguage3* iface,
    UINT uiCodePage,
    HWND hwnd,
    DWORD dwfIODControl)
{
    unsigned int i;
    MLang_impl *This = impl_from_IMultiLanguage3( iface );

    TRACE("%p %u %p %08lx\n", This, uiCodePage, hwnd, dwfIODControl);

    /* quick check for kernel32 supported code pages */
    if (IsValidCodePage(uiCodePage))
        return S_OK;

    /* check for mlang supported code pages */
    for (i = 0; i < ARRAY_SIZE(mlang_data); i++)
    {
        UINT n;
        for (n = 0; n < mlang_data[i].number_of_cp; n++)
        {
            if (mlang_data[i].mime_cp_info[n].cp == uiCodePage)
                return S_OK;
        }
    }

    if (dwfIODControl != CPIOD_PEEK)
        FIXME("Request to install codepage language pack not handled\n");

    return S_FALSE;
}

static HRESULT WINAPI fnIMultiLanguage3_DetectOutboundCodePage(
    IMultiLanguage3 *iface,
    DWORD dwFlags,
    LPCWSTR lpWideCharStr,
    UINT cchWideChar,
    UINT *puiPreferredCodePages,
    UINT nPreferredCodePages,
    UINT *puiDetectedCodePages,
    UINT *pnDetectedCodePages,
    WCHAR *lpSpecialChar)
{
    MLang_impl *This = impl_from_IMultiLanguage3( iface );

    FIXME("(%p)->(%08lx %s %p %u %p %p(%u) %s)\n", This, dwFlags, debugstr_w(lpWideCharStr),
          puiPreferredCodePages, nPreferredCodePages, puiDetectedCodePages,
          pnDetectedCodePages, pnDetectedCodePages ? *pnDetectedCodePages : 0,
          debugstr_w(lpSpecialChar));

    if (!puiDetectedCodePages || !pnDetectedCodePages || !*pnDetectedCodePages)
        return E_INVALIDARG;

    puiDetectedCodePages[0] = CP_UTF8;
    *pnDetectedCodePages = 1;
    return S_OK;
}

static HRESULT WINAPI fnIMultiLanguage3_DetectOutboundCodePageInIStream(
    IMultiLanguage3 *iface,
    DWORD dwFlags,
    IStream *pStrIn,
    UINT *puiPreferredCodePages,
    UINT nPreferredCodePages,
    UINT *puiDetectedCodePages,
    UINT *pnDetectedCodePages,
    WCHAR *lpSpecialChar)
{
    MLang_impl *This = impl_from_IMultiLanguage3( iface );

    FIXME("(%p)->(%08lx %p %p %u %p %p(%u) %s)\n", This, dwFlags, pStrIn,
          puiPreferredCodePages, nPreferredCodePages, puiDetectedCodePages,
          pnDetectedCodePages, pnDetectedCodePages ? *pnDetectedCodePages : 0,
          debugstr_w(lpSpecialChar));

    if (!puiDetectedCodePages || !pnDetectedCodePages || !*pnDetectedCodePages)
        return E_INVALIDARG;

    puiDetectedCodePages[0] = CP_UTF8;
    *pnDetectedCodePages = 1;
    return S_OK;
}

static const IMultiLanguage3Vtbl IMultiLanguage3_vtbl =
{
    fnIMultiLanguage3_QueryInterface,
    fnIMultiLanguage3_AddRef,
    fnIMultiLanguage3_Release,
    fnIMultiLanguage3_GetNumberOfCodePageInfo,
    fnIMultiLanguage3_GetCodePageInfo,
    fnIMultiLanguage3_GetFamilyCodePage,
    fnIMultiLanguage3_EnumCodePages,
    fnIMultiLanguage3_GetCharsetInfo,
    fnIMultiLanguage3_IsConvertible,
    fnIMultiLanguage3_ConvertString,
    fnIMultiLanguage3_ConvertStringToUnicode,
    fnIMultiLanguage3_ConvertStringFromUnicode,
    fnIMultiLanguage3_ConvertStringReset,
    fnIMultiLanguage3_GetRfc1766FromLcid,
    fnIMultiLanguage3_GetLcidFromRfc1766,
    fnIMultiLanguage3_EnumRfc1766,
    fnIMultiLanguage3_GetRfc1766Info,
    fnIMultiLanguage3_CreateConvertCharset,
    fnIMultiLanguage3_ConvertStringInIStream,
    fnIMultiLanguage3_ConvertStringToUnicodeEx,
    fnIMultiLanguage3_ConvertStringFromUnicodeEx,
    fnIMultiLanguage3_DetectCodepageInIStream,
    fnIMultiLanguage3_DetectInputCodepage,
    fnIMultiLanguage3_ValidateCodePage,
    fnIMultiLanguage3_GetCodePageDescription,
    fnIMultiLanguage3_IsCodePageInstallable,
    fnIMultiLanguage3_SetMimeDBSource,
    fnIMultiLanguage3_GetNumberOfScripts,
    fnIMultiLanguage3_EnumScripts,
    fnIMultiLanguage3_ValidateCodePageEx,
    fnIMultiLanguage3_DetectOutboundCodePage,
    fnIMultiLanguage3_DetectOutboundCodePageInIStream
};

/******************************************************************************/

static inline MLang_impl *impl_from_IMLangFontLink2( IMLangFontLink2 *iface )
{
    return CONTAINING_RECORD( iface, MLang_impl, IMLangFontLink2_iface );
}

static HRESULT WINAPI fnIMLangFontLink2_QueryInterface(
    IMLangFontLink2 * iface,
    REFIID riid,
    void** ppvObject)
{
    MLang_impl *This = impl_from_IMLangFontLink2( iface );
    return IMultiLanguage3_QueryInterface( &This->IMultiLanguage3_iface, riid, ppvObject );
}

static ULONG WINAPI fnIMLangFontLink2_AddRef( IMLangFontLink2* iface )
{
    MLang_impl *This = impl_from_IMLangFontLink2( iface );
    return IMultiLanguage3_AddRef( &This->IMultiLanguage3_iface );
}

static ULONG WINAPI fnIMLangFontLink2_Release( IMLangFontLink2* iface )
{
    MLang_impl *This = impl_from_IMLangFontLink2( iface );
    return IMultiLanguage3_Release( &This->IMultiLanguage3_iface );
}

static HRESULT WINAPI fnIMLangFontLink2_GetCharCodePages( IMLangFontLink2* iface,
        WCHAR ch_src, DWORD *ret_codepages)
{
    MLang_impl *This = impl_from_IMLangFontLink2(iface);
    unsigned int i;

    TRACE("(%p)->(%s %p)\n", This, debugstr_wn(&ch_src, 1), ret_codepages);

    *ret_codepages = 0;

    for (i = 0; i < ARRAY_SIZE(mlang_data) - 1 /* skip unicode codepages */; i++)
    {
        BOOL used_dc;
        CHAR buf[2];

        WideCharToMultiByte(mlang_data[i].family_codepage, WC_NO_BEST_FIT_CHARS,
            &ch_src, 1, buf, 2, NULL, &used_dc);

        /* If default char is not used, current codepage include the given symbol */
        if (!used_dc)
        {
            DWORD codepages;

            IMLangFontLink2_CodePageToCodePages(iface,
                mlang_data[i].family_codepage, &codepages);
            *ret_codepages |= codepages;
        }
    }
    return S_OK;
}

static HRESULT WINAPI fnIMLangFontLink2_GetStrCodePages( IMLangFontLink2* iface,
        const WCHAR *src, LONG src_len, DWORD priority_cp,
        DWORD *codepages, LONG *ret_len)
{
    MLang_impl *This = impl_from_IMLangFontLink2(iface);
    LONG i;
    DWORD cps = 0;

    TRACE("(%p)->(%s:%ld %lx %p %p)\n", This, debugstr_wn(src, src_len), src_len, priority_cp,
        codepages, ret_len);

    if (codepages) *codepages = 0;
    if (ret_len) *ret_len = 0;

    if (!src || !src_len || src_len < 0)
        return E_INVALIDARG;

    for (i = 0; i < src_len; i++)
    {
        DWORD cp = 0;
        HRESULT ret;

        ret = IMLangFontLink2_GetCharCodePages(iface, src[i], &cp);
        if (ret != S_OK) return E_FAIL;

        if (!cps) cps = cp;
        else if ((cps & cp) != 0 &&
                 !((priority_cp & cps) ^ (priority_cp & cp))) cps &= cp;
        else
        {
            i--;
            break;
        }
    }

    if (codepages) *codepages = cps;
    if (ret_len) *ret_len = min( i + 1, src_len );
    return S_OK;
}

static HRESULT WINAPI fnIMLangFontLink2_CodePageToCodePages(IMLangFontLink2* iface,
        UINT codepage,
        DWORD *codepages)
{
    MLang_impl *This = impl_from_IMLangFontLink2(iface);
    CHARSETINFO cs;
    BOOL rc;

    TRACE("(%p)->(%u %p)\n", This, codepage, codepages);

    rc = TranslateCharsetInfo((DWORD*)(DWORD_PTR)codepage, &cs, TCI_SRCCODEPAGE);
    if (rc)
    {
        *codepages = cs.fs.fsCsb[0];
        TRACE("resulting codepages 0x%lx\n", *codepages);
        return S_OK;
    }

    TRACE("codepage not found\n");
    *codepages = 0;
    return E_FAIL;
}

static HRESULT WINAPI fnIMLangFontLink2_CodePagesToCodePage(IMLangFontLink2* iface,
        DWORD codepages, UINT def_codepage, UINT *codepage)
{
    MLang_impl *This = impl_from_IMLangFontLink2(iface);
    DWORD mask = 0;
    CHARSETINFO cs;
    BOOL rc;
    UINT i;

    TRACE("(%p)->(0x%lx %u %p)\n", This, codepages, def_codepage, codepage);

    *codepage = 0;

    rc = TranslateCharsetInfo((DWORD*)(DWORD_PTR)def_codepage, &cs, TCI_SRCCODEPAGE);
    if (rc && (codepages & cs.fs.fsCsb[0]))
    {
        TRACE("Found Default Codepage\n");
        *codepage = def_codepage;
        return S_OK;
    }

    for (i = 0; i < 32; i++)
    {
        mask = 1 << i;
        if (codepages & mask)
        {
            DWORD Csb[2];
            Csb[0] = mask;
            Csb[1] = 0x0;
            rc = TranslateCharsetInfo(Csb, &cs, TCI_SRCFONTSIG);
            if (!rc)
                continue;

            TRACE("Falling back to least significant found CodePage %u\n",
                    cs.ciACP);
            *codepage = cs.ciACP;
            return S_OK;
        }
    }

    TRACE("no codepage found\n");
    return E_FAIL;
}

static HRESULT WINAPI fnIMLangFontLink2_GetFontCodePages(IMLangFontLink2 *iface,
        HDC hdc, HFONT hfont, DWORD *codepages)
{
    MLang_impl *This = impl_from_IMLangFontLink2(iface);
    FONTSIGNATURE fontsig;
    HFONT old_font;

    TRACE("(%p)->(%p %p %p)\n", This, hdc, hfont, codepages);

    if (codepages) *codepages = 0;

    old_font = SelectObject(hdc, hfont);
    if (!old_font) return E_FAIL;
    GetTextCharsetInfo(hdc, &fontsig, 0);
    SelectObject(hdc, old_font);

    if (codepages) *codepages = fontsig.fsCsb[0];

    TRACE("ret 0x%lx\n", fontsig.fsCsb[0]);

    return S_OK;
}

static HRESULT WINAPI fnIMLangFontLink2_ReleaseFont(IMLangFontLink2* This,
        HFONT hFont)
{
    TRACE("(%p)->%p\n",This, hFont);

    return release_font(hFont);
}

static HRESULT WINAPI fnIMLangFontLink2_ResetFontMapping(IMLangFontLink2* This)
{
    TRACE("(%p)\n",This);

    return clear_font_cache();
}

static HRESULT WINAPI fnIMLangFontLink2_MapFont(IMLangFontLink2* This,
        HDC hDC, DWORD dwCodePages, WCHAR chSrc, HFONT *pFont)
{
    HFONT old_font;

    TRACE("(%p)->%p %08lx %04x %p\n",This, hDC, dwCodePages, chSrc, pFont);

    if (!hDC) return E_FAIL;

    if (dwCodePages != 0)
    {
        old_font = GetCurrentObject(hDC, OBJ_FONT);
        return map_font(hDC, dwCodePages, old_font, pFont);
    }
    else
    {
        if (pFont == NULL) return E_INVALIDARG;
        FIXME("the situation where dwCodepages is set to zero is not implemented\n");
        return E_FAIL;
    }
}

static HRESULT WINAPI fnIMLangFontLink2_GetFontUnicodeRanges(IMLangFontLink2* This,
        HDC hDC, UINT *puiRanges, UNICODERANGE *pUranges)
{
    DWORD size;
    GLYPHSET *gs;

    TRACE("(%p)->%p %p %p\n", This, hDC, puiRanges, pUranges);

    if (!puiRanges) return E_INVALIDARG;
    if (!(size = GetFontUnicodeRanges(hDC, NULL))) return E_FAIL;
    if (!(gs = malloc(size))) return E_OUTOFMEMORY;

    GetFontUnicodeRanges(hDC, gs);
    *puiRanges = gs->cRanges;
    if (pUranges)
    {
        UINT i;
        for (i = 0; i < gs->cRanges; i++)
        {
            if (i >= *puiRanges) break;
            pUranges[i].wcFrom = gs->ranges[i].wcLow;
            pUranges[i].wcTo   = gs->ranges[i].wcLow + gs->ranges[i].cGlyphs;
        }
        *puiRanges = i;
    }
    free(gs);
    return S_OK;
}

static HRESULT WINAPI fnIMLangFontLink2_GetScriptFontInfo(IMLangFontLink2* This,
        SCRIPT_ID sid, DWORD dwFlags, UINT *puiFonts,
        SCRIPTFONTINFO *pScriptFont)
{
    UINT i, j;

    TRACE("(%p)->%u %lx %p %p\n", This, sid, dwFlags, puiFonts, pScriptFont);

    if (!dwFlags) dwFlags = SCRIPTCONTF_PROPORTIONAL_FONT;

    for (i = 0, j = 0; i < ARRAY_SIZE(mlang_data); i++)
    {
        if (sid == mlang_data[i].sid)
        {
            if (pScriptFont)
            {
                if (j >= *puiFonts) break;

                pScriptFont[j].scripts = (SCRIPT_IDS)1 << mlang_data[i].sid;
                if (dwFlags == SCRIPTCONTF_FIXED_FONT)
                    wcscpy( pScriptFont[j].wszFont, mlang_data[i].fixed_font );
                else if (dwFlags == SCRIPTCONTF_PROPORTIONAL_FONT)
                    wcscpy( pScriptFont[j].wszFont, mlang_data[i].proportional_font );
            }
            j++;
        }
    }
    *puiFonts = j;
    return S_OK;
}

static HRESULT WINAPI fnIMLangFontLink2_CodePageToScriptID(IMLangFontLink2* This,
        UINT uiCodePage, SCRIPT_ID *pSid)
{
    UINT i;

    TRACE("(%p)->%i %p\n", This, uiCodePage, pSid);

    if (uiCodePage == CP_UNICODE) return E_FAIL;

    for (i = 0; i < ARRAY_SIZE(mlang_data); i++)
    {
        if (uiCodePage == mlang_data[i].family_codepage)
        {
            if (pSid) *pSid = mlang_data[i].sid;
            return S_OK;
        }
    }
    return E_FAIL;
}

static const IMLangFontLink2Vtbl IMLangFontLink2_vtbl =
{
    fnIMLangFontLink2_QueryInterface,
    fnIMLangFontLink2_AddRef,
    fnIMLangFontLink2_Release,
    fnIMLangFontLink2_GetCharCodePages,
    fnIMLangFontLink2_GetStrCodePages,
    fnIMLangFontLink2_CodePageToCodePages,
    fnIMLangFontLink2_CodePagesToCodePage,
    fnIMLangFontLink2_GetFontCodePages,
    fnIMLangFontLink2_ReleaseFont,
    fnIMLangFontLink2_ResetFontMapping,
    fnIMLangFontLink2_MapFont,
    fnIMLangFontLink2_GetFontUnicodeRanges,
    fnIMLangFontLink2_GetScriptFontInfo,
    fnIMLangFontLink2_CodePageToScriptID
};

/******************************************************************************/

static inline MLang_impl *impl_from_IMLangLineBreakConsole( IMLangLineBreakConsole *iface )
{
    return CONTAINING_RECORD( iface, MLang_impl, IMLangLineBreakConsole_iface );
}

static HRESULT WINAPI fnIMLangLineBreakConsole_QueryInterface(
    IMLangLineBreakConsole* iface,
    REFIID riid,
    void** ppvObject)
{
    MLang_impl *This = impl_from_IMLangLineBreakConsole( iface );
    return IMultiLanguage3_QueryInterface( &This->IMultiLanguage3_iface, riid, ppvObject );
}

static ULONG WINAPI fnIMLangLineBreakConsole_AddRef(
    IMLangLineBreakConsole* iface )
{
    MLang_impl *This = impl_from_IMLangLineBreakConsole( iface );
    return IMultiLanguage3_AddRef( &This->IMultiLanguage3_iface );
}

static ULONG WINAPI fnIMLangLineBreakConsole_Release(
    IMLangLineBreakConsole* iface )
{
    MLang_impl *This = impl_from_IMLangLineBreakConsole( iface );
    return IMultiLanguage3_Release( &This->IMultiLanguage3_iface );
}

static HRESULT WINAPI fnIMLangLineBreakConsole_BreakLineML(
    IMLangLineBreakConsole* iface,
    IMLangString* pSrcMLStr,
    LONG lSrcPos,
    LONG lSrcLen,
    LONG cMinColumns,
    LONG cMaxColumns,
    LONG* plLineLen,
    LONG* plSkipLen)
{
    FIXME("(%p)->%p %li %li %li %li %p %p\n", iface, pSrcMLStr, lSrcPos, lSrcLen, cMinColumns, cMaxColumns, plLineLen, plSkipLen);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIMLangLineBreakConsole_BreakLineW(
    IMLangLineBreakConsole* iface,
    LCID locale,
    const WCHAR* pszSrc,
    LONG cchSrc,
    LONG cMaxColumns,
    LONG* pcchLine,
    LONG* pcchSkip )
{
    FIXME("(%p)->%li %s %li %li %p %p\n", iface, locale, debugstr_wn(pszSrc,cchSrc), cchSrc, cMaxColumns, pcchLine, pcchSkip);

    *pcchLine = cchSrc;
    *pcchSkip = 0;
    return S_OK;
}

static HRESULT WINAPI fnIMLangLineBreakConsole_BreakLineA(
    IMLangLineBreakConsole* iface,
    LCID locale,
    UINT uCodePage,
    const CHAR* pszSrc,
    LONG cchSrc,
    LONG cMaxColumns,
    LONG* pcchLine,
    LONG* pcchSkip)
{
    LONG i, line = cchSrc, skip = 0;

    FIXME("(%p)->%li %i %s %li %li %p %p\n", iface, locale, uCodePage, debugstr_an(pszSrc,cchSrc), cchSrc, cMaxColumns, pcchLine, pcchSkip);

    if (uCodePage == CP_USASCII && cchSrc > cMaxColumns)
    {
        for (line = cMaxColumns, i = cMaxColumns - 1; i >= 0; i--)
        {
            if (pszSrc[i] == ' ')
            {
                while (i >= 0 && pszSrc[i] == ' ')
                {
                    i--;
                    line--;
                    skip++;
                }
                break;
            }
        }
    }
    *pcchLine = line;
    *pcchSkip = skip;
    return S_OK;
}

static const IMLangLineBreakConsoleVtbl IMLangLineBreakConsole_vtbl =
{
    fnIMLangLineBreakConsole_QueryInterface,
    fnIMLangLineBreakConsole_AddRef,
    fnIMLangLineBreakConsole_Release,
    fnIMLangLineBreakConsole_BreakLineML,
    fnIMLangLineBreakConsole_BreakLineW,
    fnIMLangLineBreakConsole_BreakLineA
};

struct convert_charset {
    IMLangConvertCharset IMLangConvertCharset_iface;
    LONG ref;

    UINT src_cp;
    UINT dst_cp;
};

static inline struct convert_charset *impl_from_IMLangConvertCharset(IMLangConvertCharset *iface)
{
    return CONTAINING_RECORD(iface, struct convert_charset, IMLangConvertCharset_iface);
}

static HRESULT WINAPI MLangConvertCharset_QueryInterface(IMLangConvertCharset *iface, REFIID riid, void **obj)
{
    struct convert_charset *This = impl_from_IMLangConvertCharset(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IMLangConvertCharset))
    {
        *obj = &This->IMLangConvertCharset_iface;
        IMLangConvertCharset_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI MLangConvertCharset_AddRef(IMLangConvertCharset *iface)
{
    struct convert_charset *This = impl_from_IMLangConvertCharset(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%lu)\n", This, ref);
    return ref;
}

static ULONG WINAPI MLangConvertCharset_Release(IMLangConvertCharset *iface)
{
    struct convert_charset *This = impl_from_IMLangConvertCharset(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%lu)\n", This, ref);
    if (!ref)
    {
        free(This);
        UnlockModule();
    }

    return ref;
}

static HRESULT WINAPI MLangConvertCharset_Initialize(IMLangConvertCharset *iface,
    UINT src_cp, UINT dst_cp, DWORD prop)
{
    struct convert_charset *This = impl_from_IMLangConvertCharset(iface);

    TRACE("(%p)->(%u %u 0x%08lx)\n", This, src_cp, dst_cp, prop);

    prop &= ~MLCONVCHARF_USEDEFCHAR;
    if (prop)
        FIXME("property 0x%08lx not supported\n", prop);

    This->src_cp = src_cp;
    This->dst_cp = dst_cp;

    return S_OK;
}

static HRESULT WINAPI MLangConvertCharset_GetSourceCodePage(IMLangConvertCharset *iface, UINT *src_cp)
{
    struct convert_charset *This = impl_from_IMLangConvertCharset(iface);

    TRACE("(%p)->(%p)\n", This, src_cp);

    if (!src_cp) return E_INVALIDARG;
    *src_cp = This->src_cp;
    return S_OK;
}

static HRESULT WINAPI MLangConvertCharset_GetDestinationCodePage(IMLangConvertCharset *iface, UINT *dst_cp)
{
    struct convert_charset *This = impl_from_IMLangConvertCharset(iface);

    TRACE("(%p)->(%p)\n", This, dst_cp);

    if (!dst_cp) return E_INVALIDARG;
    *dst_cp = This->dst_cp;
    return S_OK;
}

static HRESULT WINAPI MLangConvertCharset_GetProperty(IMLangConvertCharset *iface, DWORD *prop)
{
    struct convert_charset *This = impl_from_IMLangConvertCharset(iface);
    FIXME("(%p)->(%p): stub\n", This, prop);
    return E_NOTIMPL;
}

static HRESULT WINAPI MLangConvertCharset_DoConversion(IMLangConvertCharset *iface, BYTE *src,
    UINT *src_size, BYTE *dest, UINT *dest_size)
{
    struct convert_charset *This = impl_from_IMLangConvertCharset(iface);
    FIXME("(%p)->(%p %p %p %p): stub\n", This, src, src_size, dest, dest_size);
    return E_NOTIMPL;
}

static HRESULT WINAPI MLangConvertCharset_DoConversionToUnicode(IMLangConvertCharset *iface, CHAR *src,
    UINT *src_size, WCHAR *dest, UINT *dest_size)
{
    struct convert_charset *This = impl_from_IMLangConvertCharset(iface);
    TRACE("(%p)->(%p %p %p %p)\n", This, src, src_size, dest, dest_size);
    return ConvertINetMultiByteToUnicode(NULL, This->src_cp, src, (INT*)src_size, dest, (INT*)dest_size);
}

static HRESULT WINAPI MLangConvertCharset_DoConversionFromUnicode(IMLangConvertCharset *iface,
    WCHAR *src, UINT *src_size, CHAR *dest, UINT *dest_size)
{
    struct convert_charset *This = impl_from_IMLangConvertCharset(iface);
    TRACE("(%p)->(%p %p %p %p)\n", This, src, src_size, dest, dest_size);
    return ConvertINetUnicodeToMultiByte(NULL, This->dst_cp, src, (INT*)src_size, dest, (INT*)dest_size);
}

static const IMLangConvertCharsetVtbl MLangConvertCharsetVtbl =
{
    MLangConvertCharset_QueryInterface,
    MLangConvertCharset_AddRef,
    MLangConvertCharset_Release,
    MLangConvertCharset_Initialize,
    MLangConvertCharset_GetSourceCodePage,
    MLangConvertCharset_GetDestinationCodePage,
    MLangConvertCharset_GetProperty,
    MLangConvertCharset_DoConversion,
    MLangConvertCharset_DoConversionToUnicode,
    MLangConvertCharset_DoConversionFromUnicode
};

static HRESULT MultiLanguage_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    MLang_impl *mlang;
    UINT i;

    TRACE("Creating MultiLanguage object\n");

    if( pUnkOuter )
        return CLASS_E_NOAGGREGATION;

    mlang = malloc(sizeof(MLang_impl));
    mlang->IMLangFontLink_iface.lpVtbl = &IMLangFontLink_vtbl;
    mlang->IMultiLanguage_iface.lpVtbl = &IMultiLanguage_vtbl;
    mlang->IMultiLanguage3_iface.lpVtbl = &IMultiLanguage3_vtbl;
    mlang->IMLangFontLink2_iface.lpVtbl = &IMLangFontLink2_vtbl;
    mlang->IMLangLineBreakConsole_iface.lpVtbl = &IMLangLineBreakConsole_vtbl;

    mlang->total_cp = 0;
    for (i = 0; i < ARRAY_SIZE(mlang_data); i++)
        mlang->total_cp += mlang_data[i].number_of_cp;

    /* do not enumerate unicode flavours */
    mlang->total_scripts = ARRAY_SIZE(mlang_data) - 1;

    mlang->ref = 1;
    *ppObj = &mlang->IMultiLanguage_iface;
    TRACE("returning %p\n", mlang);

    LockModule();

    return S_OK;
}

static HRESULT MLangConvertCharset_create(IUnknown *outer, void **obj)
{
    struct convert_charset *convert;

    if (outer)
        return CLASS_E_NOAGGREGATION;

    *obj = NULL;

    convert = malloc(sizeof(struct convert_charset));
    if (!convert) return E_OUTOFMEMORY;

    convert->IMLangConvertCharset_iface.lpVtbl = &MLangConvertCharsetVtbl;
    convert->ref = 1;

    *obj = &convert->IMLangConvertCharset_iface;

    LockModule();

    return S_OK;
}

/******************************************************************************/

HRESULT WINAPI DllCanUnloadNow(void)
{
    return dll_count == 0 ? S_OK : S_FALSE;
}

static BOOL WINAPI allocate_font_link_cb(PINIT_ONCE init_once, PVOID args, PVOID *context)
{
    return SUCCEEDED(MultiLanguage_create(NULL, (void**)&font_link_global));
}

HRESULT WINAPI GetGlobalFontLinkObject(IMLangFontLink **obj)
{
    TRACE("%p\n", obj);

    if (!obj) return E_INVALIDARG;

    if (!InitOnceExecuteOnce(&font_link_global_init_once, allocate_font_link_cb, NULL, NULL))
    {
        ERR("Failed to create global font link object.\n");
        return E_FAIL;
    }

    return IUnknown_QueryInterface(font_link_global, &IID_IMLangFontLink, (void**)obj);
}
