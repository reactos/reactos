/*
 *    MLANG Class Factory
 *
 * Copyright 2002 Lionel Ulmer
 * Copyright 2003,2004 Mike McCormack
 * Copyright 2004,2005 Dmitry Timoshkov
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

#include "config.h"

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "ole2.h"
#include "mlang.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mlang);

#include "initguid.h"

#define CP_UNICODE 1200

#define ICOM_THIS_MULTI(impl,field,iface) impl* const This=(impl*)((char*)(iface) - offsetof(impl,field))

static HRESULT MultiLanguage_create(IUnknown *pUnkOuter, LPVOID *ppObj);

static DWORD MLANG_tls_index; /* to store various per thead data */

/* FIXME:
 * Under what circumstances HKEY_CLASSES_ROOT\MIME\Database\Codepage and
 * HKEY_CLASSES_ROOT\MIME\Database\Charset are used?
 */

typedef struct
{
    const char *description;
    UINT cp;
    DWORD flags;
    const char *web_charset;
    const char *header_charset;
    const char *body_charset;
} MIME_CP_INFO;

/* These data are based on the codepage info in libs/unicode/cpmap.pl */
/* FIXME: Add 28604 (Celtic), 28606 (Balkan) */

static const MIME_CP_INFO arabic_cp[] =
{
    { "Arabic (864)",
      864, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID_NLS |
           MIMECONTF_MIME_LATEST,
      "ibm864", "ibm864", "ibm864" },
    { "Arabic (1006)",
      1006, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID_NLS |
            MIMECONTF_MIME_LATEST,
      "ibm1006", "ibm1006", "ibm1006" },
    { "Arabic (Windows)",
      1256, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_IMPORT |
            MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER |
            MIMECONTF_EXPORT | MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      "windows-1256", "windows-1256", "windows-1256" },
    { "Arabic (ISO)",
      28596, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_MINIMAL |
             MIMECONTF_IMPORT | MIMECONTF_SAVABLE_MAILNEWS |
             MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_IE4 | MIMECONTF_MIME_LATEST,
      "iso-8859-6", "iso-8859-6", "iso-8859-6" }
};
static const MIME_CP_INFO baltic_cp[] =
{
    { "Baltic (DOS)",
      775, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
           MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      "ibm775", "ibm775", "ibm775" },
    { "Baltic (Windows)",
      1257, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_MINIMAL |
            MIMECONTF_IMPORT | MIMECONTF_SAVABLE_MAILNEWS |
            MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT | MIMECONTF_VALID |
            MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      "windows-1257", "windows-1257", "windows-1257" },
    { "Baltic (ISO)",
      28594, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_IMPORT |
             MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER |
             MIMECONTF_EXPORT | MIMECONTF_VALID | MIMECONTF_VALID_NLS |
             MIMECONTF_MIME_LATEST,
      "iso-8859-4", "iso-8859-4", "iso-8859-4" },
    { "Estonian (ISO)",
      28603, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      "iso-8859-13", "iso-8859-13", "iso-8859-13" }
};
static const MIME_CP_INFO chinese_simplified_cp[] =
{
    { "Chinese Simplified (GB2312)",
      936, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_MINIMAL |
           MIMECONTF_IMPORT | MIMECONTF_SAVABLE_MAILNEWS |
           MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT | MIMECONTF_VALID_NLS |
           MIMECONTF_MIME_IE4 | MIMECONTF_MIME_LATEST,
      "gb2312", "gb2312", "gb2312" }
};
static const MIME_CP_INFO chinese_traditional_cp[] =
{
    { "Chinese Traditional (Big5)",
      950, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_MINIMAL |
           MIMECONTF_IMPORT | MIMECONTF_SAVABLE_MAILNEWS |
           MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT |
           MIMECONTF_VALID_NLS | MIMECONTF_MIME_IE4 | MIMECONTF_MIME_LATEST,
      "big5", "big5", "big5" }
};
static const MIME_CP_INFO central_european_cp[] =
{
    { "Central European (DOS)",
      852, MIMECONTF_BROWSER | MIMECONTF_IMPORT | MIMECONTF_SAVABLE_BROWSER |
           MIMECONTF_EXPORT | MIMECONTF_VALID | MIMECONTF_VALID_NLS |
           MIMECONTF_MIME_IE4 | MIMECONTF_MIME_LATEST,
      "ibm852", "ibm852", "ibm852" },
    { "Central European (Windows)",
      1250, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_IMPORT |
            MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER |
            MIMECONTF_EXPORT | MIMECONTF_VALID | MIMECONTF_VALID_NLS |
            MIMECONTF_MIME_LATEST,
      "windows-1250", "windows-1250", "windows-1250" },
    { "Central European (Mac)",
      10029, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      "x-mac-ce", "x-mac-ce", "x-mac-ce" },
    { "Central European (ISO)",
      28592, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_MINIMAL |
             MIMECONTF_IMPORT | MIMECONTF_SAVABLE_MAILNEWS |
             MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT | MIMECONTF_VALID |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_IE4 | MIMECONTF_MIME_LATEST,
      "iso-8859-2", "iso-8859-2", "iso-8859-2" }
};
static const MIME_CP_INFO cyrillic_cp[] =
{
    { "OEM Cyrillic",
      855, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
           MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      "ibm855", "ibm855", "ibm855" },
    { "Cyrillic (DOS)",
      866, MIMECONTF_BROWSER | MIMECONTF_IMPORT | MIMECONTF_SAVABLE_BROWSER |
           MIMECONTF_EXPORT | MIMECONTF_VALID_NLS | MIMECONTF_MIME_IE4 |
           MIMECONTF_MIME_LATEST,
      "cp866", "cp866", "cp866" },
#if 0 /* Windows has 20866 as an official code page for KOI8-R */
    { "Cyrillic (KOI8-R)",
      878, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
           MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      "koi8-r", "koi8-r", "koi8-r" },
#endif
    { "Cyrillic (Windows)",
      1251, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_IMPORT |
            MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER |
            MIMECONTF_EXPORT | MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      "windows-1251", "windows-1251", "windows-1251" },
    { "Cyrillic (Mac)",
      10007, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID_NLS |
             MIMECONTF_MIME_LATEST,
      "x-mac-cyrillic", "x-mac-cyrillic", "x-mac-cyrillic" },
    { "Cyrillic (KOI8-R)",
      20866, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_MINIMAL |
             MIMECONTF_IMPORT | MIMECONTF_SAVABLE_MAILNEWS |
             MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_IE4 | MIMECONTF_MIME_LATEST,
      "koi8-r", "koi8-r", "koi8-r" },
    { "Cyrillic (KOI8-U)",
      21866, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_MINIMAL |
             MIMECONTF_IMPORT | MIMECONTF_SAVABLE_MAILNEWS |
             MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_IE4 | MIMECONTF_MIME_LATEST,
      "koi8-u", "koi8-u", "koi8-u" },
    { "Cyrillic (ISO)",
      28595, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_MINIMAL |
             MIMECONTF_IMPORT | MIMECONTF_SAVABLE_MAILNEWS |
             MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_IE4 | MIMECONTF_MIME_LATEST,
      "iso-8859-5", "iso-8859-5", "iso-8859-5" }
};
static const MIME_CP_INFO greek_cp[] =
{
    { "Greek (DOS)",
      737, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID_NLS |
           MIMECONTF_MIME_LATEST,
      "ibm737", "ibm737", "ibm737" },
    { "Greek, Modern (DOS)",
      869, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID_NLS |
           MIMECONTF_MIME_LATEST,
      "ibm869", "ibm869", "ibm869" },
    { "IBM EBCDIC (Greek Modern)",
      875, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID_NLS |
           MIMECONTF_MIME_LATEST,
      "cp875", "cp875", "cp875" },
    { "Greek (Windows)",
      1253, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_IMPORT |
            MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER |
            MIMECONTF_EXPORT | MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      "windows-1253", "windows-1253", "windows-1253" },
    { "Greek (Mac)",
      10006, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID_NLS |
             MIMECONTF_MIME_LATEST,
      "x-mac-greek", "x-mac-greek", "x-mac-greek" },
    { "Greek (ISO)",
      28597, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_MINIMAL |
             MIMECONTF_IMPORT | MIMECONTF_SAVABLE_MAILNEWS |
             MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_IE4 | MIMECONTF_MIME_LATEST,
      "iso-8859-7", "iso-8859-7", "iso-8859-7" }
};
static const MIME_CP_INFO hebrew_cp[] =
{
    { "Hebrew (424)",
      424, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID_NLS |
           MIMECONTF_MIME_LATEST,
      "ibm424", "ibm424", "ibm424" },
    { "Hebrew (856)",
      856, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID_NLS |
           MIMECONTF_MIME_LATEST,
      "cp856", "cp856", "cp856" },
    { "Hebrew (DOS)",
      862, MIMECONTF_BROWSER | MIMECONTF_MINIMAL | MIMECONTF_IMPORT |
           MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT | MIMECONTF_VALID_NLS |
           MIMECONTF_MIME_LATEST,
      "dos-862", "dos-862", "dos-862" },
    { "Hebrew (Windows)",
      1255, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_IMPORT |
            MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER |
            MIMECONTF_EXPORT | MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      "windows-1255", "windows-1255", "windows-1255" },
    { "Hebrew (ISO-Visual)",
      28598, MIMECONTF_BROWSER | MIMECONTF_MINIMAL | MIMECONTF_IMPORT |
             MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_IE4 | MIMECONTF_MIME_LATEST,
      "iso-8859-8", "iso-8859-8", "iso-8859-8" }
};
static const MIME_CP_INFO japanese_cp[] =
{
    { "Japanese (Shift-JIS)",
      932, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_MINIMAL |
           MIMECONTF_IMPORT | MIMECONTF_SAVABLE_MAILNEWS |
           MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT | MIMECONTF_VALID_NLS |
           MIMECONTF_MIME_IE4 | MIMECONTF_MIME_LATEST,
      "shift_jis", "iso-2022-jp", "iso-2022-jp" },
    { "Japanese (JIS 0208-1990 and 0212-1990)",
      20932, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID_NLS |
             MIMECONTF_MIME_LATEST,
      "euc-jp", "euc-jp", "euc-jp" }
};
static const MIME_CP_INFO korean_cp[] =
{
    { "Korean",
      949, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_MINIMAL |
           MIMECONTF_IMPORT | MIMECONTF_SAVABLE_MAILNEWS |
           MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT | MIMECONTF_VALID_NLS |
           MIMECONTF_MIME_LATEST,
      "ks_c_5601-1987", "ks_c_5601-1987", "ks_c_5601-1987" }
};
static const MIME_CP_INFO thai_cp[] =
{
    { "Thai (Windows)",
      874, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_MIME_LATEST,
      "ibm-thai", "ibm-thai", "ibm-thai" }
};
static const MIME_CP_INFO turkish_cp[] =
{
    { "Turkish (DOS)",
      857, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
           MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      "ibm857", "ibm857", "ibm857" },
    { "IBM EBCDIC (Turkish Latin-5)",
      1026, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
            MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      "ibm1026", "ibm1026", "ibm1026" },
    { "Turkish (Windows)",
      1254, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_MINIMAL |
            MIMECONTF_IMPORT | MIMECONTF_SAVABLE_MAILNEWS |
            MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT | MIMECONTF_VALID |
            MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      "windows-1254", "windows-1254", "windows-1254" },
    { "Turkish (Mac)",
      10081, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      "x-mac-turkish", "x-mac-turkish", "x-mac-turkish" },
    { "Latin 3 (ISO)",
      28593, MIMECONTF_MAILNEWS | MIMECONTF_IMPORT |
             MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_EXPORT | MIMECONTF_VALID |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_IE4 | MIMECONTF_MIME_LATEST,
      "iso-8859-3", "iso-8859-3", "iso-8859-3" },
    { "Turkish (ISO)",
      28599, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_MINIMAL |
             MIMECONTF_IMPORT | MIMECONTF_SAVABLE_MAILNEWS |
             MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT | MIMECONTF_VALID |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      "iso-8859-9", "iso-8859-9", "iso-8859-9" }
};
static const MIME_CP_INFO vietnamese_cp[] =
{
    { "Vietnamese (Windows)",
      1258, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_IMPORT |
            MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER |
            MIMECONTF_EXPORT | MIMECONTF_VALID_NLS | MIMECONTF_MIME_IE4 |
            MIMECONTF_MIME_LATEST,
      "windows-1258", "windows-1258", "windows-1258" }
};
static const MIME_CP_INFO western_cp[] =
{
    { "IBM EBCDIC (US-Canada)",
      37, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
          MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      "ibm037", "ibm037", "ibm037" },
    { "OEM United States",
      437, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
           MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      "ibm437", "ibm437", "ibm437" },
    { "IBM EBCDIC (International)",
      500, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
           MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      "ibm500", "ibm500", "ibm500" },
    { "Western European (DOS)",
      850, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
           MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      "ibm850", "ibm850", "ibm850" },
    { "Portuguese (DOS)",
      860, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
           MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      "ibm860", "ibm860", "ibm860" },
    { "Icelandic (DOS)",
      861, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
           MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      "ibm861", "ibm861", "ibm861" },
    { "French Canadian (DOS)",
      863, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
           MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      "ibm863", "ibm863", "ibm863" },
    { "Nordic (DOS)",
      865, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
           MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      "ibm865", "ibm865", "ibm865" },
    { "Western European (Windows)",
      1252, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_MINIMAL |
            MIMECONTF_IMPORT | MIMECONTF_SAVABLE_MAILNEWS |
            MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT | MIMECONTF_VALID |
            MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      "windows-1252", "windows-1252", "iso-8859-1" },
    { "Western European (Mac)",
      10000, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      "macintosh", "macintosh", "macintosh" },
    { "Icelandic (Mac)",
      10079, MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      "x-mac-icelandic", "x-mac-icelandic", "x-mac-icelandic" },
    { "US-ASCII",
      20127, MIMECONTF_MAILNEWS | MIMECONTF_IMPORT | MIMECONTF_EXPORT |
             MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_VALID |
             MIMECONTF_VALID_NLS | MIMECONTF_MIME_LATEST,
      "us-ascii", "us-ascii", "us-ascii" },
    { "Western European (ISO)",
      28591, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_IMPORT |
             MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER |
             MIMECONTF_EXPORT | MIMECONTF_VALID | MIMECONTF_VALID_NLS |
             MIMECONTF_MIME_LATEST,
      "iso-8859-1", "iso-8859-1", "iso-8859-1" },
    { "Latin 9 (ISO)",
      28605, MIMECONTF_MAILNEWS | MIMECONTF_IMPORT |
             MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER |
             MIMECONTF_EXPORT | MIMECONTF_VALID | MIMECONTF_VALID_NLS |
             MIMECONTF_MIME_LATEST,
      "iso-8859-15", "iso-8859-15", "iso-8859-15" }
};
static const MIME_CP_INFO unicode_cp[] =
{
    { "Unicode",
      CP_UNICODE, MIMECONTF_MINIMAL | MIMECONTF_IMPORT |
                  MIMECONTF_SAVABLE_BROWSER | MIMECONTF_EXPORT |
                  MIMECONTF_VALID | MIMECONTF_VALID_NLS | MIMECONTF_MIME_IE4 |
                  MIMECONTF_MIME_LATEST,
      "unicode", "unicode", "unicode" },
    { "Unicode (UTF-7)",
      CP_UTF7, MIMECONTF_MAILNEWS | MIMECONTF_IMPORT |
               MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_EXPORT | MIMECONTF_VALID |
               MIMECONTF_VALID_NLS | MIMECONTF_MIME_IE4 | MIMECONTF_MIME_LATEST,
      "utf-7", "utf-7", "utf-7" },
    { "Unicode (UTF-8)",
      CP_UTF8, MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_IMPORT |
               MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER |
               MIMECONTF_EXPORT | MIMECONTF_VALID | MIMECONTF_VALID_NLS |
               MIMECONTF_MIME_IE4 | MIMECONTF_MIME_LATEST,
      "utf-8", "utf-8", "utf-8" }
};

static const struct mlang_data
{
    const char *description;
    UINT family_codepage;
    UINT number_of_cp;
    const MIME_CP_INFO *mime_cp_info;
    const char *fixed_font;
    const char *proportional_font;
} mlang_data[] =
{
    { "Arabic",1256,sizeof(arabic_cp)/sizeof(arabic_cp[0]),arabic_cp,
      "Courier","Arial" }, /* FIXME */
    { "Baltic",1257,sizeof(baltic_cp)/sizeof(baltic_cp[0]),baltic_cp,
      "Courier","Arial" }, /* FIXME */
    { "Chinese Simplified",936,sizeof(chinese_simplified_cp)/sizeof(chinese_simplified_cp[0]),chinese_simplified_cp,
      "Courier","Arial" }, /* FIXME */
    { "Chinese Traditional",950,sizeof(chinese_traditional_cp)/sizeof(chinese_traditional_cp[0]),chinese_traditional_cp,
      "Courier","Arial" }, /* FIXME */
    { "Central European",1250,sizeof(central_european_cp)/sizeof(central_european_cp[0]),central_european_cp,
      "Courier","Arial" }, /* FIXME */
    { "Cyrillic",1251,sizeof(cyrillic_cp)/sizeof(cyrillic_cp[0]),cyrillic_cp,
      "Courier","Arial" }, /* FIXME */
    { "Greek",1253,sizeof(greek_cp)/sizeof(greek_cp[0]),greek_cp,
      "Courier","Arial" }, /* FIXME */
    { "Hebrew",1255,sizeof(hebrew_cp)/sizeof(hebrew_cp[0]),hebrew_cp,
      "Courier","Arial" }, /* FIXME */
    { "Japanese",932,sizeof(japanese_cp)/sizeof(japanese_cp[0]),japanese_cp,
      "Courier","Arial" }, /* FIXME */
    { "Korean",949,sizeof(korean_cp)/sizeof(korean_cp[0]),korean_cp,
      "Courier","Arial" }, /* FIXME */
    { "Thai",874,sizeof(thai_cp)/sizeof(thai_cp[0]),thai_cp,
      "Courier","Arial" }, /* FIXME */
    { "Turkish",1254,sizeof(turkish_cp)/sizeof(turkish_cp[0]),turkish_cp,
      "Courier","Arial" }, /* FIXME */
    { "Vietnamese",1258,sizeof(vietnamese_cp)/sizeof(vietnamese_cp[0]),vietnamese_cp,
      "Courier","Arial" }, /* FIXME */
    { "Western European",1252,sizeof(western_cp)/sizeof(western_cp[0]),western_cp,
      "Courier","Arial" }, /* FIXME */
    { "Unicode",CP_UNICODE,sizeof(unicode_cp)/sizeof(unicode_cp[0]),unicode_cp,
      "Courier","Arial" } /* FIXME */
};

static void fill_cp_info(const struct mlang_data *ml_data, UINT index, MIMECPINFO *mime_cp_info);

static LONG dll_count;

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

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    switch(fdwReason) {
        case DLL_PROCESS_ATTACH:
            MLANG_tls_index = TlsAlloc();
            DisableThreadLibraryCalls(hInstDLL);
	    break;
	case DLL_PROCESS_DETACH:
            TlsFree(MLANG_tls_index);
	    break;
    }
    return TRUE;
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

    TRACE("%p %d %s %p %p %p\n", pdwMode, dwEncoding,
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

    INT src_len = -1;

    TRACE("%p %d %s %p %p %p\n", pdwMode, dwEncoding,
          debugstr_w(pSrcStr), pcSrcSize, pDstStr, pcDstSize);

    if (!pcDstSize)
        return E_FAIL;

    if (!pcSrcSize)
        pcSrcSize = &src_len;

    if (!*pcSrcSize)
    {
        *pcDstSize = 0;
        return S_OK;
    }

    switch (dwEncoding)
    {
    case CP_UNICODE:
        if (*pcSrcSize == -1)
            *pcSrcSize = lstrlenW(pSrcStr);
        *pcDstSize = min(*pcSrcSize * sizeof(WCHAR), *pcDstSize);
        if (pDstStr)
            memmove(pDstStr, pSrcStr, *pcDstSize);
        break;

    default:
        if (*pcSrcSize == -1)
            *pcSrcSize = lstrlenW(pSrcStr);

        if (pDstStr)
            *pcDstSize = WideCharToMultiByte(dwEncoding, 0, pSrcStr, *pcSrcSize, pDstStr, *pcDstSize, NULL, NULL);
        else
            *pcDstSize = WideCharToMultiByte(dwEncoding, 0, pSrcStr, *pcSrcSize, NULL, 0, NULL, NULL);
        break;
    }


    if (!*pcDstSize)
        return E_FAIL;

    return S_OK;
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
    TRACE("%p %d %d %s %p %p %p\n", pdwMode, dwSrcEncoding, dwDstEncoding,
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

        TRACE("convert %s from %d to %d\n", debugstr_a(pSrcStr), dwSrcEncoding, dwDstEncoding);

        hr = ConvertINetMultiByteToUnicode(pdwMode, dwSrcEncoding, pSrcStr, pcSrcSize, NULL, &cDstSizeW);
        if (hr != S_OK)
            return hr;

        pDstStrW = HeapAlloc(GetProcessHeap(), 0, cDstSizeW * sizeof(WCHAR));
        hr = ConvertINetMultiByteToUnicode(pdwMode, dwSrcEncoding, pSrcStr, pcSrcSize, pDstStrW, &cDstSizeW);
        if (hr != S_OK)
            return hr;

        hr = ConvertINetUnicodeToMultiByte(pdwMode, dwDstEncoding, pDstStrW, &cDstSizeW, pDstStr, pcDstSize);
        HeapFree(GetProcessHeap(), 0, pDstStrW);
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

    for (i = 0; i < sizeof(mlang_data)/sizeof(mlang_data[0]); i++)
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

    TRACE("%d %d\n", dwSrcEncoding, dwDstEncoding);

    if (GetFamilyCodePage(dwSrcEncoding, &src_family) != S_OK ||
        GetFamilyCodePage(dwDstEncoding, &dst_family) != S_OK)
        return S_FALSE;

    if (src_family == dst_family) return S_OK;

    /* we can convert any codepage to/from unicode */
    if (src_family == CP_UNICODE || dst_family == CP_UNICODE) return S_OK;

    return S_FALSE;
}

static inline INT lcid_to_rfc1766A( LCID lcid, LPSTR rfc1766, INT len )
{
    INT n = GetLocaleInfoA( lcid, LOCALE_SISO639LANGNAME, rfc1766, len );
    if (n)
    {
        rfc1766[n - 1] = '-';
        n += GetLocaleInfoA( lcid, LOCALE_SISO3166CTRYNAME, rfc1766 + n, len - n ) + 1;
        LCMapStringA( LOCALE_USER_DEFAULT, LCMAP_LOWERCASE, rfc1766, n, rfc1766, len );
        return n;
    }
    return 0;
}

static inline INT lcid_to_rfc1766W( LCID lcid, LPWSTR rfc1766, INT len )
{
    INT n = GetLocaleInfoW( lcid, LOCALE_SISO639LANGNAME, rfc1766, len );
    if (n)
    {
        rfc1766[n - 1] = '-';
        n += GetLocaleInfoW( lcid, LOCALE_SISO3166CTRYNAME, rfc1766 + n, len - n ) + 1;
        LCMapStringW( LOCALE_USER_DEFAULT, LCMAP_LOWERCASE, rfc1766, n, rfc1766, len );
        return n;
    }
    return 0;
}

HRESULT WINAPI LcidToRfc1766A(
    LCID lcid,
    LPSTR pszRfc1766,
    INT nChar)
{
    TRACE("%04x %p %u\n", lcid, pszRfc1766, nChar);

    if (lcid_to_rfc1766A( lcid, pszRfc1766, nChar ))
        return S_OK;

    return S_FALSE;
}

HRESULT WINAPI LcidToRfc1766W(
    LCID lcid,
    LPWSTR pszRfc1766,
    INT nChar)
{
    TRACE("%04x %p %u\n", lcid, pszRfc1766, nChar);

    if (lcid_to_rfc1766W( lcid, pszRfc1766, nChar ))
        return S_OK;

    return S_FALSE;
}

static HRESULT lcid_from_rfc1766(IEnumRfc1766 *iface, LCID *lcid, LPCWSTR rfc1766)
{
    RFC1766INFO info;
    ULONG num;

    while (IEnumRfc1766_Next(iface, 1, &info, &num) == S_OK)
    {
        if (!strcmpW(info.wszRfc1766, rfc1766))
        {
            *lcid = info.lcid;
            return S_OK;
        }
        if (strlenW(rfc1766) == 2 && !memcmp(info.wszRfc1766, rfc1766, 2 * sizeof(WCHAR)))
        {
            *lcid = PRIMARYLANGID(info.lcid);
            return S_OK;
        }
    }

    return E_FAIL;
}

/******************************************************************************
 * MLANG ClassFactory
 */
typedef struct {
    IClassFactory ITF_IClassFactory;

    LONG ref;
    HRESULT (*pfnCreateInstance)(IUnknown *pUnkOuter, LPVOID *ppObj);
} IClassFactoryImpl;

struct object_creation_info
{
    const CLSID *clsid;
    LPCSTR szClassName;
    HRESULT (*pfnCreateInstance)(IUnknown *pUnkOuter, LPVOID *ppObj);
};

static const struct object_creation_info object_creation[] =
{
    { &CLSID_CMultiLanguage, "CLSID_CMultiLanguage", MultiLanguage_create },
};

static HRESULT WINAPI
MLANGCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;

    TRACE("%s\n", debugstr_guid(riid) );

    if (IsEqualGUID(riid, &IID_IUnknown)
	|| IsEqualGUID(riid, &IID_IClassFactory))
    {
	IClassFactory_AddRef(iface);
	*ppobj = This;
	return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI MLANGCF_AddRef(LPCLASSFACTORY iface)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI MLANGCF_Release(LPCLASSFACTORY iface)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;

    ULONG ref = InterlockedDecrement(&This->ref);

    if (ref == 0)
    {
        TRACE("Destroying %p\n", This);
	HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI MLANGCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter,
					  REFIID riid, LPVOID *ppobj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    HRESULT hres;
    LPUNKNOWN punk;

    TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);

    *ppobj = NULL;
    hres = This->pfnCreateInstance(pOuter, (LPVOID *) &punk);
    if (SUCCEEDED(hres)) {
        hres = IUnknown_QueryInterface(punk, riid, ppobj);
        IUnknown_Release(punk);
    }
    TRACE("returning (%p) -> %x\n", *ppobj, hres);
    return hres;
}

static HRESULT WINAPI MLANGCF_LockServer(LPCLASSFACTORY iface,BOOL dolock)
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
    int i;
    IClassFactoryImpl *factory;

    TRACE("%s %s %p\n",debugstr_guid(rclsid), debugstr_guid(iid), ppv);

    if ( !IsEqualGUID( &IID_IClassFactory, iid )
	 && ! IsEqualGUID( &IID_IUnknown, iid) )
	return E_NOINTERFACE;

    for (i=0; i < sizeof(object_creation)/sizeof(object_creation[0]); i++)
    {
	if (IsEqualGUID(object_creation[i].clsid, rclsid))
	    break;
    }

    if (i == sizeof(object_creation)/sizeof(object_creation[0]))
    {
	FIXME("%s: no class found.\n", debugstr_guid(rclsid));
	return CLASS_E_CLASSNOTAVAILABLE;
    }

    TRACE("Creating a class factory for %s\n",object_creation[i].szClassName);

    factory = HeapAlloc(GetProcessHeap(), 0, sizeof(*factory));
    if (factory == NULL) return E_OUTOFMEMORY;

    factory->ITF_IClassFactory.lpVtbl = &MLANGCF_Vtbl;
    factory->ref = 1;

    factory->pfnCreateInstance = object_creation[i].pfnCreateInstance;

    *ppv = &(factory->ITF_IClassFactory);

    TRACE("(%p) <- %p\n", ppv, &(factory->ITF_IClassFactory) );

    return S_OK;
}


/******************************************************************************/

typedef struct tagMLang_impl
{
    const IMLangFontLinkVtbl *vtbl_IMLangFontLink;
    const IMultiLanguageVtbl *vtbl_IMultiLanguage;
    const IMultiLanguage3Vtbl *vtbl_IMultiLanguage3;
    LONG ref;
    DWORD total_cp, total_scripts;
} MLang_impl;

static ULONG WINAPI MLang_AddRef( MLang_impl* This)
{
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI MLang_Release( MLang_impl* This )
{
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p ref = %d\n", This, ref);
    if (ref == 0)
    {
        TRACE("Destroying %p\n", This);
	HeapFree(GetProcessHeap(), 0, This);
        UnlockModule();
    }

    return ref;
}

static HRESULT WINAPI MLang_QueryInterface(
        MLang_impl* This,
        REFIID riid,
        void** ppvObject)
{
    TRACE("%p -> %s\n", This, debugstr_guid(riid) );

    if (IsEqualGUID(riid, &IID_IUnknown)
	|| IsEqualGUID(riid, &IID_IMLangCodePages)
	|| IsEqualGUID(riid, &IID_IMLangFontLink))
    {
	MLang_AddRef(This);
        TRACE("Returning IID_IMLangFontLink %p ref = %d\n", This, This->ref);
	*ppvObject = &(This->vtbl_IMLangFontLink);
	return S_OK;
    }

    if (IsEqualGUID(riid, &IID_IMultiLanguage) )
    {
	MLang_AddRef(This);
        TRACE("Returning IID_IMultiLanguage %p ref = %d\n", This, This->ref);
	*ppvObject = &(This->vtbl_IMultiLanguage);
	return S_OK;
    }

    if (IsEqualGUID(riid, &IID_IMultiLanguage2) )
    {
	MLang_AddRef(This);
	*ppvObject = &(This->vtbl_IMultiLanguage3);
        TRACE("Returning IID_IMultiLanguage2 %p ref = %d\n", This, This->ref);
	return S_OK;
    }

    if (IsEqualGUID(riid, &IID_IMultiLanguage3) )
    {
	MLang_AddRef(This);
	*ppvObject = &(This->vtbl_IMultiLanguage3);
        TRACE("Returning IID_IMultiLanguage3 %p ref = %d\n", This, This->ref);
	return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);
    return E_NOINTERFACE;
}

/******************************************************************************/

typedef struct tagEnumCodePage_impl
{
    const IEnumCodePageVtbl *vtbl_IEnumCodePage;
    LONG ref;
    MIMECPINFO *cpinfo;
    DWORD total, pos;
} EnumCodePage_impl;

static HRESULT WINAPI fnIEnumCodePage_QueryInterface(
        IEnumCodePage* iface,
        REFIID riid,
        void** ppvObject)
{
    ICOM_THIS_MULTI(EnumCodePage_impl, vtbl_IEnumCodePage, iface);

    TRACE("%p -> %s\n", This, debugstr_guid(riid) );

    if (IsEqualGUID(riid, &IID_IUnknown)
	|| IsEqualGUID(riid, &IID_IEnumCodePage))
    {
	IEnumCodePage_AddRef(iface);
        TRACE("Returning IID_IEnumCodePage %p ref = %d\n", This, This->ref);
	*ppvObject = &(This->vtbl_IEnumCodePage);
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);
    return E_NOINTERFACE;
}

static ULONG WINAPI fnIEnumCodePage_AddRef(
        IEnumCodePage* iface)
{
    ICOM_THIS_MULTI(EnumCodePage_impl, vtbl_IEnumCodePage, iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI fnIEnumCodePage_Release(
        IEnumCodePage* iface)
{
    ICOM_THIS_MULTI(EnumCodePage_impl, vtbl_IEnumCodePage, iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p ref = %d\n", This, ref);
    if (ref == 0)
    {
        TRACE("Destroying %p\n", This);
        HeapFree(GetProcessHeap(), 0, This->cpinfo);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI fnIEnumCodePage_Clone(
        IEnumCodePage* iface,
        IEnumCodePage** ppEnum)
{
    ICOM_THIS_MULTI(EnumCodePage_impl, vtbl_IEnumCodePage, iface);
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

    ICOM_THIS_MULTI(EnumCodePage_impl, vtbl_IEnumCodePage, iface);
    TRACE("%p %u %p %p\n", This, celt, rgelt, pceltFetched);

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
        TRACE("#%u: %08x %u %u %s %s %s %s %s %s %d\n",
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
    ICOM_THIS_MULTI(EnumCodePage_impl, vtbl_IEnumCodePage, iface);
    TRACE("%p\n", This);

    This->pos = 0;
    return S_OK;
}

static  HRESULT WINAPI fnIEnumCodePage_Skip(
        IEnumCodePage* iface,
        ULONG celt)
{
    ICOM_THIS_MULTI(EnumCodePage_impl, vtbl_IEnumCodePage, iface);
    TRACE("%p %u\n", This, celt);

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

    TRACE("%p, %08x, %04x, %p\n", mlang, grfFlags, LangId, ppEnumCodePage);

    if (!grfFlags) /* enumerate internal data base of encodings */
        grfFlags = MIMECONTF_MIME_LATEST;

    ecp = HeapAlloc( GetProcessHeap(), 0, sizeof (EnumCodePage_impl) );
    ecp->vtbl_IEnumCodePage = &IEnumCodePage_vtbl;
    ecp->ref = 1;
    ecp->pos = 0;
    ecp->total = 0;
    for (i = 0; i < sizeof(mlang_data)/sizeof(mlang_data[0]); i++)
    {
        for (n = 0; n < mlang_data[i].number_of_cp; n++)
        {
            if (mlang_data[i].mime_cp_info[n].flags & grfFlags)
                ecp->total++;
        }
    }

    ecp->cpinfo = HeapAlloc(GetProcessHeap(), 0,
                            sizeof(MIMECPINFO) * ecp->total);
    cpinfo = ecp->cpinfo;

    for (i = 0; i < sizeof(mlang_data)/sizeof(mlang_data[0]); i++)
    {
        for (n = 0; n < mlang_data[i].number_of_cp; n++)
        {
            if (mlang_data[i].mime_cp_info[n].flags & grfFlags)
                fill_cp_info(&mlang_data[i], n, cpinfo++);
        }
    }

    TRACE("enumerated %d codepages with flags %08x\n", ecp->total, grfFlags);

    *ppEnumCodePage = (IEnumCodePage*) ecp;

    return S_OK;
}

/******************************************************************************/

typedef struct tagEnumScript_impl
{
    const IEnumScriptVtbl *vtbl_IEnumScript;
    LONG ref;
    SCRIPTINFO *script_info;
    DWORD total, pos;
} EnumScript_impl;

static HRESULT WINAPI fnIEnumScript_QueryInterface(
        IEnumScript* iface,
        REFIID riid,
        void** ppvObject)
{
    ICOM_THIS_MULTI(EnumScript_impl, vtbl_IEnumScript, iface);

    TRACE("%p -> %s\n", This, debugstr_guid(riid) );

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IEnumScript))
    {
        IEnumScript_AddRef(iface);
        TRACE("Returning IID_IEnumScript %p ref = %d\n", This, This->ref);
        *ppvObject = &(This->vtbl_IEnumScript);
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);
    return E_NOINTERFACE;
}

static ULONG WINAPI fnIEnumScript_AddRef(
        IEnumScript* iface)
{
    ICOM_THIS_MULTI(EnumScript_impl, vtbl_IEnumScript, iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI fnIEnumScript_Release(
        IEnumScript* iface)
{
    ICOM_THIS_MULTI(EnumScript_impl, vtbl_IEnumScript, iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p ref = %d\n", This, ref);
    if (ref == 0)
    {
        TRACE("Destroying %p\n", This);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI fnIEnumScript_Clone(
        IEnumScript* iface,
        IEnumScript** ppEnum)
{
    ICOM_THIS_MULTI(EnumScript_impl, vtbl_IEnumScript, iface);
    FIXME("%p %p: stub!\n", This, ppEnum);
    return E_NOTIMPL;
}

static  HRESULT WINAPI fnIEnumScript_Next(
        IEnumScript* iface,
        ULONG celt,
        PSCRIPTINFO rgelt,
        ULONG* pceltFetched)
{
    ICOM_THIS_MULTI(EnumScript_impl, vtbl_IEnumScript, iface);
    TRACE("%p %u %p %p\n", This, celt, rgelt, pceltFetched);

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
    ICOM_THIS_MULTI(EnumScript_impl, vtbl_IEnumScript, iface);
    TRACE("%p\n", This);

    This->pos = 0;
    return S_OK;
}

static  HRESULT WINAPI fnIEnumScript_Skip(
        IEnumScript* iface,
        ULONG celt)
{
    ICOM_THIS_MULTI(EnumScript_impl, vtbl_IEnumScript, iface);
    TRACE("%p %u\n", This, celt);

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

    TRACE("%p, %08x, %04x, %p: stub!\n", mlang, dwFlags, LangId, ppEnumScript);

    if (!dwFlags) /* enumerate all available scripts */
        dwFlags = SCRIPTCONTF_SCRIPT_USER | SCRIPTCONTF_SCRIPT_HIDE | SCRIPTCONTF_SCRIPT_SYSTEM;

    es = HeapAlloc( GetProcessHeap(), 0, sizeof (EnumScript_impl) );
    es->vtbl_IEnumScript = &IEnumScript_vtbl;
    es->ref = 1;
    es->pos = 0;
    /* do not enumerate unicode flavours */
    es->total = sizeof(mlang_data)/sizeof(mlang_data[0]) - 1;
    es->script_info = HeapAlloc(GetProcessHeap(), 0, sizeof(SCRIPTINFO) * es->total);

    for (i = 0; i < es->total; i++)
    {
        es->script_info[i].ScriptId = i;
        es->script_info[i].uiCodePage = mlang_data[i].family_codepage;
        MultiByteToWideChar(CP_ACP, 0, mlang_data[i].description, -1,
            es->script_info[i].wszDescription, MAX_SCRIPT_NAME);
        MultiByteToWideChar(CP_ACP, 0, mlang_data[i].fixed_font, -1,
            es->script_info[i].wszFixedWidthFont, MAX_MIMEFACE_NAME);
        MultiByteToWideChar(CP_ACP, 0, mlang_data[i].proportional_font, -1,
            es->script_info[i].wszProportionalFont, MAX_MIMEFACE_NAME);
    }

    TRACE("enumerated %d scripts with flags %08x\n", es->total, dwFlags);

    *ppEnumScript = (IEnumScript *)es;

    return S_OK;
}

/******************************************************************************/

static HRESULT WINAPI fnIMLangFontLink_QueryInterface(
        IMLangFontLink* iface,
        REFIID riid,
        void** ppvObject)
{
    ICOM_THIS_MULTI(MLang_impl, vtbl_IMLangFontLink, iface);
    return MLang_QueryInterface( This, riid, ppvObject );
}

static ULONG WINAPI fnIMLangFontLink_AddRef(
        IMLangFontLink* iface)
{
    ICOM_THIS_MULTI(MLang_impl, vtbl_IMLangFontLink, iface);
    return MLang_AddRef( This );
}

static ULONG WINAPI fnIMLangFontLink_Release(
        IMLangFontLink* iface)
{
    ICOM_THIS_MULTI(MLang_impl, vtbl_IMLangFontLink, iface);
    return MLang_Release( This );
}

static HRESULT WINAPI fnIMLangFontLink_GetCharCodePages(
        IMLangFontLink* iface,
        WCHAR chSrc,
        DWORD* pdwCodePages)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIMLangFontLink_GetStrCodePages(
        IMLangFontLink* iface,
        const WCHAR* pszSrc,
        long cchSrc,
        DWORD dwPriorityCodePages,
        DWORD* pdwCodePages,
        long* pcchCodePages)
{
    FIXME("(pszSrc=%s, cchSrc=%ld, dwPriorityCodePages=%d) stub\n", debugstr_w(pszSrc), cchSrc, dwPriorityCodePages);
    *pdwCodePages = 0;
    *pcchCodePages = 1;
    return S_OK;
}

static HRESULT WINAPI fnIMLangFontLink_CodePageToCodePages(
        IMLangFontLink* iface,
        UINT uCodePage,
        DWORD* pdwCodePages)
{
    ICOM_THIS_MULTI(MLang_impl, vtbl_IMLangFontLink, iface);
    CHARSETINFO cs;
    BOOL rc;

    TRACE("(%p) Seeking %u\n",This, uCodePage);
    memset(&cs, 0, sizeof(cs));

    rc = TranslateCharsetInfo((DWORD*)uCodePage, &cs, TCI_SRCCODEPAGE);

    if (rc)
    {
        *pdwCodePages = cs.fs.fsCsb[0];
        TRACE("resulting CodePages 0x%x\n",*pdwCodePages);
    }
    else
        TRACE("CodePage Not Found\n");

    return S_OK;
}

static HRESULT WINAPI fnIMLangFontLink_CodePagesToCodePage(
        IMLangFontLink* iface,
        DWORD dwCodePages,
        UINT uDefaultCodePage,
        UINT* puCodePage)
{
    ICOM_THIS_MULTI(MLang_impl, vtbl_IMLangFontLink, iface);
    DWORD mask = 0x00000000;
    UINT i;
    CHARSETINFO cs;
    BOOL rc;

    TRACE("(%p) scanning  0x%x  default page %u\n",This, dwCodePages,
            uDefaultCodePage);

    *puCodePage = 0x00000000;

    rc = TranslateCharsetInfo((DWORD*)uDefaultCodePage, &cs, TCI_SRCCODEPAGE);

    if (rc && (dwCodePages & cs.fs.fsCsb[0]))
    {
        TRACE("Found Default Codepage\n");
        *puCodePage = uDefaultCodePage;
        return S_OK;
    }


    for (i = 0; i < 32; i++)
    {

        mask = 1 << i;
        if (dwCodePages & mask)
        {
            DWORD Csb[2];
            Csb[0] = mask;
            Csb[1] = 0x0;
            rc = TranslateCharsetInfo((DWORD*)Csb, &cs, TCI_SRCFONTSIG);
            if (!rc)
                continue;

            TRACE("Falling back to least significant found CodePage %u\n",
                    cs.ciACP);
            *puCodePage = cs.ciACP;
            return S_OK;
        }
    }

    TRACE("no codepage found\n");
    return E_FAIL;
}

static HRESULT WINAPI fnIMLangFontLink_GetFontCodePages(
        IMLangFontLink* iface,
        HDC hDC,
        HFONT hFont,
        DWORD* pdwCodePages)
{
    HFONT old_font;
    FONTSIGNATURE fontsig;
    ICOM_THIS_MULTI(MLang_impl, vtbl_IMLangFontLink, iface);

    TRACE("(%p)\n",This);

    old_font = SelectObject(hDC,hFont);
    GetTextCharsetInfo(hDC,&fontsig, 0);
    SelectObject(hDC,old_font);

    *pdwCodePages = fontsig.fsCsb[0];
    TRACE("CodePages is 0x%x\n",fontsig.fsCsb[0]);

    return S_OK;
}

static HRESULT WINAPI fnIMLangFontLink_MapFont(
        IMLangFontLink* iface,
        HDC hDC,
        DWORD dwCodePages,
        HFONT hSrcFont,
        HFONT* phDestFont)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIMLangFontLink_ReleaseFont(
        IMLangFontLink* iface,
        HFONT hFont)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIMLangFontLink_ResetFontMapping(
        IMLangFontLink* iface)
{
    FIXME("\n");
    return E_NOTIMPL;
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

static HRESULT WINAPI fnIMultiLanguage_QueryInterface(
    IMultiLanguage* iface,
    REFIID riid,
    void** ppvObject)
{
    ICOM_THIS_MULTI(MLang_impl, vtbl_IMultiLanguage, iface);
    return MLang_QueryInterface( This, riid, ppvObject );
}

static ULONG WINAPI fnIMultiLanguage_AddRef( IMultiLanguage* iface )
{
    ICOM_THIS_MULTI(MLang_impl, vtbl_IMultiLanguage, iface);
    return IMLangFontLink_AddRef( ((IMLangFontLink*)This) );
}

static ULONG WINAPI fnIMultiLanguage_Release( IMultiLanguage* iface )
{
    ICOM_THIS_MULTI(MLang_impl, vtbl_IMultiLanguage, iface);
    return IMLangFontLink_Release( ((IMLangFontLink*)This) );
}

static HRESULT WINAPI fnIMultiLanguage_GetNumberOfCodePageInfo(
    IMultiLanguage* iface,
    UINT* pcCodePage)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIMultiLanguage_GetCodePageInfo(
    IMultiLanguage* iface,
    UINT uiCodePage,
    PMIMECPINFO pCodePageInfo)
{
    UINT i, n;

    ICOM_THIS_MULTI(MLang_impl, vtbl_IMultiLanguage, iface);
    TRACE("%p, %u, %p\n", This, uiCodePage, pCodePageInfo);

    for (i = 0; i < sizeof(mlang_data)/sizeof(mlang_data[0]); i++)
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
    UINT uiCodePage,
    UINT* puiFamilyCodePage)
{
    return GetFamilyCodePage(uiCodePage, puiFamilyCodePage);
}

static HRESULT WINAPI fnIMultiLanguage_EnumCodePages(
    IMultiLanguage* iface,
    DWORD grfFlags,
    IEnumCodePage** ppEnumCodePage)
{
    ICOM_THIS_MULTI(MLang_impl, vtbl_IMultiLanguage, iface);
    TRACE("%p %08x %p\n", This, grfFlags, ppEnumCodePage);

    return EnumCodePage_create( This, grfFlags, 0, ppEnumCodePage );
}

static HRESULT WINAPI fnIMultiLanguage_GetCharsetInfo(
    IMultiLanguage* iface,
    BSTR Charset,
    PMIMECSETINFO pCharsetInfo)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIMultiLanguage_IsConvertible(
    IMultiLanguage* iface,
    DWORD dwSrcEncoding,
    DWORD dwDstEncoding)
{
    return IsConvertINetStringAvailable(dwSrcEncoding, dwDstEncoding);
}

static HRESULT WINAPI fnIMultiLanguage_ConvertString(
    IMultiLanguage* iface,
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

static HRESULT WINAPI fnIMultiLanguage_ConvertStringToUnicode(
    IMultiLanguage* iface,
    DWORD* pdwMode,
    DWORD dwEncoding,
    CHAR* pSrcStr,
    UINT* pcSrcSize,
    WCHAR* pDstStr,
    UINT* pcDstSize)
{
    return ConvertINetMultiByteToUnicode(pdwMode, dwEncoding,
        (LPCSTR)pSrcStr, (LPINT)pcSrcSize, pDstStr, (LPINT)pcDstSize);
}

static HRESULT WINAPI fnIMultiLanguage_ConvertStringFromUnicode(
    IMultiLanguage* iface,
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

static HRESULT WINAPI fnIMultiLanguage_ConvertStringReset(
    IMultiLanguage* iface)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIMultiLanguage_GetRfc1766FromLcid(
    IMultiLanguage* iface,
    LCID lcid,
    BSTR* pbstrRfc1766)
{
    WCHAR buf[MAX_RFC1766_NAME];

    TRACE("%p %04x %p\n", iface, lcid, pbstrRfc1766);

    if (lcid_to_rfc1766W( lcid, buf, MAX_RFC1766_NAME ))
    {
        *pbstrRfc1766 = SysAllocString( buf );
        return S_OK;
    }
    return E_FAIL;
}

static HRESULT WINAPI fnIMultiLanguage_GetLcidFromRfc1766(
    IMultiLanguage* iface,
    LCID* pLocale,
    BSTR bstrRfc1766)
{
    HRESULT hr;
    IEnumRfc1766 *rfc1766;

    TRACE("%p %p %s\n", iface, pLocale, debugstr_w(bstrRfc1766));

    if (!pLocale || !bstrRfc1766)
        return E_INVALIDARG;

    hr = IMultiLanguage_EnumRfc1766(iface, &rfc1766);
    if (FAILED(hr))
        return hr;

    hr = lcid_from_rfc1766(rfc1766, pLocale, bstrRfc1766);

    IEnumRfc1766_Release(rfc1766);
    return hr;
}

/******************************************************************************/

typedef struct tagEnumRfc1766_impl
{
    const IEnumRfc1766Vtbl *vtbl_IEnumRfc1766;
    LONG ref;
    RFC1766INFO *info;
    DWORD total, pos;
} EnumRfc1766_impl;

static HRESULT WINAPI fnIEnumRfc1766_QueryInterface(
        IEnumRfc1766 *iface,
        REFIID riid,
        void** ppvObject)
{
    ICOM_THIS_MULTI(EnumRfc1766_impl, vtbl_IEnumRfc1766, iface);

    TRACE("%p -> %s\n", This, debugstr_guid(riid) );

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IEnumRfc1766))
    {
        IEnumRfc1766_AddRef(iface);
        TRACE("Returning IID_IEnumRfc1766 %p ref = %d\n", This, This->ref);
        *ppvObject = &(This->vtbl_IEnumRfc1766);
        return S_OK;
    }

    WARN("(%p) -> (%s,%p), not found\n",This,debugstr_guid(riid),ppvObject);
    return E_NOINTERFACE;
}

static ULONG WINAPI fnIEnumRfc1766_AddRef(
        IEnumRfc1766 *iface)
{
    ICOM_THIS_MULTI(EnumRfc1766_impl, vtbl_IEnumRfc1766, iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI fnIEnumRfc1766_Release(
        IEnumRfc1766 *iface)
{
    ICOM_THIS_MULTI(EnumRfc1766_impl, vtbl_IEnumRfc1766, iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p ref = %d\n", This, ref);
    if (ref == 0)
    {
        TRACE("Destroying %p\n", This);
        HeapFree(GetProcessHeap(), 0, This->info);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

static HRESULT WINAPI fnIEnumRfc1766_Clone(
        IEnumRfc1766 *iface,
        IEnumRfc1766 **ppEnum)
{
    ICOM_THIS_MULTI(EnumRfc1766_impl, vtbl_IEnumRfc1766, iface);
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

    ICOM_THIS_MULTI(EnumRfc1766_impl, vtbl_IEnumRfc1766, iface);
    TRACE("%p %u %p %p\n", This, celt, rgelt, pceltFetched);

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
        TRACE("#%u: %08x %s %s\n",
              i, rgelt[i].lcid,
              wine_dbgstr_w(rgelt[i].wszRfc1766),
              wine_dbgstr_w(rgelt[i].wszLocaleName));
    }
    return S_OK;
}

static HRESULT WINAPI fnIEnumRfc1766_Reset(
        IEnumRfc1766 *iface)
{
    ICOM_THIS_MULTI(EnumRfc1766_impl, vtbl_IEnumRfc1766, iface);
    TRACE("%p\n", This);

    This->pos = 0;
    return S_OK;
}

static  HRESULT WINAPI fnIEnumRfc1766_Skip(
        IEnumRfc1766 *iface,
        ULONG celt)
{
    ICOM_THIS_MULTI(EnumRfc1766_impl, vtbl_IEnumRfc1766, iface);
    TRACE("%p %u\n", This, celt);

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

static BOOL CALLBACK enum_locales_proc(LPWSTR locale)
{
    WCHAR *end;
    struct enum_locales_data *data = TlsGetValue(MLANG_tls_index);
    RFC1766INFO *info;

    TRACE("%s\n", debugstr_w(locale));

    if (data->total >= data->allocated)
    {
        data->allocated += 32;
        data->info = HeapReAlloc(GetProcessHeap(), 0, data->info, data->allocated * sizeof(RFC1766INFO));
        if (!data->info) return FALSE;
    }

    info = &data->info[data->total];

    info->lcid = strtolW(locale, &end, 16);
    if (*end) /* invalid number */
        return FALSE;

    info->wszRfc1766[0] = 0;
    lcid_to_rfc1766W( info->lcid, info->wszRfc1766, MAX_RFC1766_NAME );

    info->wszLocaleName[0] = 0;
    GetLocaleInfoW(info->lcid, LOCALE_SLANGUAGE, info->wszLocaleName, MAX_LOCALE_NAME);
    TRACE("ISO639: %s SLANGUAGE: %s\n", wine_dbgstr_w(info->wszRfc1766), wine_dbgstr_w(info->wszLocaleName));

    data->total++;

    return TRUE;
}

static HRESULT EnumRfc1766_create(MLang_impl* mlang, LANGID LangId,
                                  IEnumRfc1766 **ppEnum)
{
    EnumRfc1766_impl *rfc;
    struct enum_locales_data data;

    TRACE("%p, %04x, %p\n", mlang, LangId, ppEnum);

    rfc = HeapAlloc( GetProcessHeap(), 0, sizeof(EnumRfc1766_impl) );
    rfc->vtbl_IEnumRfc1766 = &IEnumRfc1766_vtbl;
    rfc->ref = 1;
    rfc->pos = 0;
    rfc->total = 0;

    data.total = 0;
    data.allocated = 32;
    data.info = HeapAlloc(GetProcessHeap(), 0, data.allocated * sizeof(RFC1766INFO));
    if (!data.info) return S_FALSE;

    TlsSetValue(MLANG_tls_index, &data);
    EnumSystemLocalesW(enum_locales_proc, 0/*LOCALE_SUPPORTED*/);
    TlsSetValue(MLANG_tls_index, NULL);

    TRACE("enumerated %d rfc1766 structures\n", data.total);

    if (!data.total) return FALSE;

    rfc->info = data.info;
    rfc->total = data.total;

    *ppEnum = (IEnumRfc1766 *)rfc;
    return S_OK;
}

static HRESULT WINAPI fnIMultiLanguage_EnumRfc1766(
    IMultiLanguage *iface,
    IEnumRfc1766 **ppEnumRfc1766)
{
    ICOM_THIS_MULTI(MLang_impl, vtbl_IMultiLanguage, iface);
    TRACE("%p %p\n", This, ppEnumRfc1766);

    return EnumRfc1766_create(This, 0, ppEnumRfc1766);
}

/******************************************************************************/

static HRESULT WINAPI fnIMultiLanguage_GetRfc1766Info(
    IMultiLanguage* iface,
    LCID Locale,
    PRFC1766INFO pRfc1766Info)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIMultiLanguage_CreateConvertCharset(
    IMultiLanguage* iface,
    UINT uiSrcCodePage,
    UINT uiDstCodePage,
    DWORD dwProperty,
    IMLangConvertCharset** ppMLangConvertCharset)
{
    FIXME("\n");
    return E_NOTIMPL;
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

static HRESULT WINAPI fnIMultiLanguage2_QueryInterface(
    IMultiLanguage3* iface,
    REFIID riid,
    void** ppvObject)
{
    ICOM_THIS_MULTI(MLang_impl, vtbl_IMultiLanguage3, iface);
    return MLang_QueryInterface( This, riid, ppvObject );
}

static ULONG WINAPI fnIMultiLanguage2_AddRef( IMultiLanguage3* iface )
{
    ICOM_THIS_MULTI(MLang_impl, vtbl_IMultiLanguage3, iface);
    return MLang_AddRef( This );
}

static ULONG WINAPI fnIMultiLanguage2_Release( IMultiLanguage3* iface )
{
    ICOM_THIS_MULTI(MLang_impl, vtbl_IMultiLanguage3, iface);
    return MLang_Release( This );
}

static HRESULT WINAPI fnIMultiLanguage2_GetNumberOfCodePageInfo(
    IMultiLanguage3* iface,
    UINT* pcCodePage)
{
    ICOM_THIS_MULTI(MLang_impl, vtbl_IMultiLanguage3, iface);
    TRACE("%p, %p\n", This, pcCodePage);

    if (!pcCodePage) return S_FALSE;

    *pcCodePage = This->total_cp;
    return S_OK;
}

static void fill_cp_info(const struct mlang_data *ml_data, UINT index, MIMECPINFO *mime_cp_info)
{
    CHARSETINFO csi;

    if (TranslateCharsetInfo((DWORD *)ml_data->family_codepage, &csi, TCI_SRCCODEPAGE))
        mime_cp_info->bGDICharset = csi.ciCharset;
    else
        mime_cp_info->bGDICharset = DEFAULT_CHARSET;

    mime_cp_info->dwFlags = ml_data->mime_cp_info[index].flags;
    mime_cp_info->uiCodePage = ml_data->mime_cp_info[index].cp;
    mime_cp_info->uiFamilyCodePage = ml_data->family_codepage;
    MultiByteToWideChar(CP_ACP, 0, ml_data->mime_cp_info[index].description, -1,
                        mime_cp_info->wszDescription, sizeof(mime_cp_info->wszDescription)/sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, ml_data->mime_cp_info[index].web_charset, -1,
                        mime_cp_info->wszWebCharset, sizeof(mime_cp_info->wszWebCharset)/sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, ml_data->mime_cp_info[index].header_charset, -1,
                        mime_cp_info->wszHeaderCharset, sizeof(mime_cp_info->wszHeaderCharset)/sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, ml_data->mime_cp_info[index].body_charset, -1,
                        mime_cp_info->wszBodyCharset, sizeof(mime_cp_info->wszBodyCharset)/sizeof(WCHAR));

    MultiByteToWideChar(CP_ACP, 0, ml_data->fixed_font, -1,
        mime_cp_info->wszFixedWidthFont, sizeof(mime_cp_info->wszFixedWidthFont)/sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, ml_data->proportional_font, -1,
        mime_cp_info->wszProportionalFont, sizeof(mime_cp_info->wszProportionalFont)/sizeof(WCHAR));

    TRACE("%08x %u %u %s %s %s %s %s %s %d\n",
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

static HRESULT WINAPI fnIMultiLanguage2_GetCodePageInfo(
    IMultiLanguage3* iface,
    UINT uiCodePage,
    LANGID LangId,
    PMIMECPINFO pCodePageInfo)
{
    UINT i, n;

    ICOM_THIS_MULTI(MLang_impl, vtbl_IMultiLanguage3, iface);
    TRACE("%p, %u, %04x, %p\n", This, uiCodePage, LangId, pCodePageInfo);

    for (i = 0; i < sizeof(mlang_data)/sizeof(mlang_data[0]); i++)
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

static HRESULT WINAPI fnIMultiLanguage2_GetFamilyCodePage(
    IMultiLanguage3* iface,
    UINT uiCodePage,
    UINT* puiFamilyCodePage)
{
    return GetFamilyCodePage(uiCodePage, puiFamilyCodePage);
}

static HRESULT WINAPI fnIMultiLanguage2_EnumCodePages(
    IMultiLanguage3* iface,
    DWORD grfFlags,
    LANGID LangId,
    IEnumCodePage** ppEnumCodePage)
{
    ICOM_THIS_MULTI(MLang_impl, vtbl_IMultiLanguage3, iface);
    TRACE("%p %08x %04x %p\n", This, grfFlags, LangId, ppEnumCodePage);

    return EnumCodePage_create( This, grfFlags, LangId, ppEnumCodePage );
}

static HRESULT WINAPI fnIMultiLanguage2_GetCharsetInfo(
    IMultiLanguage3* iface,
    BSTR Charset,
    PMIMECSETINFO pCharsetInfo)
{
    UINT i, n;

    ICOM_THIS_MULTI(MLang_impl, vtbl_IMultiLanguage3, iface);
    TRACE("%p %s %p\n", This, debugstr_w(Charset), pCharsetInfo);

    if (!pCharsetInfo) return E_FAIL;

    for (i = 0; i < sizeof(mlang_data)/sizeof(mlang_data[0]); i++)
    {
        for (n = 0; n < mlang_data[i].number_of_cp; n++)
        {
            WCHAR csetW[MAX_MIMECSET_NAME];

            MultiByteToWideChar(CP_ACP, 0, mlang_data[i].mime_cp_info[n].web_charset, -1, csetW, MAX_MIMECSET_NAME);
            if (!lstrcmpiW(Charset, csetW))
            {
                pCharsetInfo->uiCodePage = mlang_data[i].family_codepage;
                pCharsetInfo->uiInternetEncoding = mlang_data[i].mime_cp_info[n].cp;
                strcpyW(pCharsetInfo->wszCharset, csetW);
                return S_OK;
            }
        }
    }

    /* FIXME:
     * Since we do not support charsets like iso-2022-jp and do not have
     * them in our database as a primary (web_charset) encoding this loop
     * does an attempt to 'approximate' charset name by header_charset.
     */
    for (i = 0; i < sizeof(mlang_data)/sizeof(mlang_data[0]); i++)
    {
        for (n = 0; n < mlang_data[i].number_of_cp; n++)
        {
            WCHAR csetW[MAX_MIMECSET_NAME];

            MultiByteToWideChar(CP_ACP, 0, mlang_data[i].mime_cp_info[n].header_charset, -1, csetW, MAX_MIMECSET_NAME);
            if (!lstrcmpiW(Charset, csetW))
            {
                pCharsetInfo->uiCodePage = mlang_data[i].family_codepage;
                pCharsetInfo->uiInternetEncoding = mlang_data[i].mime_cp_info[n].cp;
                strcpyW(pCharsetInfo->wszCharset, csetW);
                return S_OK;
            }
        }
    }

    return E_FAIL;
}

static HRESULT WINAPI fnIMultiLanguage2_IsConvertible(
    IMultiLanguage3* iface,
    DWORD dwSrcEncoding,
    DWORD dwDstEncoding)
{
    return IsConvertINetStringAvailable(dwSrcEncoding, dwDstEncoding);
}

static HRESULT WINAPI fnIMultiLanguage2_ConvertString(
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

static HRESULT WINAPI fnIMultiLanguage2_ConvertStringToUnicode(
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

static HRESULT WINAPI fnIMultiLanguage2_ConvertStringFromUnicode(
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

static HRESULT WINAPI fnIMultiLanguage2_ConvertStringReset(
    IMultiLanguage3* iface)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIMultiLanguage2_GetRfc1766FromLcid(
    IMultiLanguage3* iface,
    LCID lcid,
    BSTR* pbstrRfc1766)
{
    WCHAR buf[MAX_RFC1766_NAME];

    TRACE("%p %04x %p\n", iface, lcid, pbstrRfc1766);

    if (lcid_to_rfc1766W( lcid, buf, MAX_RFC1766_NAME ))
    {
        *pbstrRfc1766 = SysAllocString( buf );
        return S_OK;
    }
    return E_FAIL;
}

static HRESULT WINAPI fnIMultiLanguage2_GetLcidFromRfc1766(
    IMultiLanguage3* iface,
    LCID* pLocale,
    BSTR bstrRfc1766)
{
    HRESULT hr;
    IEnumRfc1766 *rfc1766;

    TRACE("%p %p %s\n", iface, pLocale, debugstr_w(bstrRfc1766));

    if (!pLocale || !bstrRfc1766)
        return E_INVALIDARG;

    hr = IMultiLanguage2_EnumRfc1766(iface, 0, &rfc1766);
    if (FAILED(hr))
        return hr;

    hr = lcid_from_rfc1766(rfc1766, pLocale, bstrRfc1766);

    IEnumRfc1766_Release(rfc1766);
    return hr;
}

static HRESULT WINAPI fnIMultiLanguage2_EnumRfc1766(
    IMultiLanguage3* iface,
    LANGID LangId,
    IEnumRfc1766** ppEnumRfc1766)
{
    ICOM_THIS_MULTI(MLang_impl, vtbl_IMultiLanguage, iface);
    TRACE("%p %p\n", This, ppEnumRfc1766);

    return EnumRfc1766_create(This, LangId, ppEnumRfc1766);
}

static HRESULT WINAPI fnIMultiLanguage2_GetRfc1766Info(
    IMultiLanguage3* iface,
    LCID Locale,
    LANGID LangId,
    PRFC1766INFO pRfc1766Info)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIMultiLanguage2_CreateConvertCharset(
    IMultiLanguage3* iface,
    UINT uiSrcCodePage,
    UINT uiDstCodePage,
    DWORD dwProperty,
    IMLangConvertCharset** ppMLangConvertCharset)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIMultiLanguage2_ConvertStringInIStream(
    IMultiLanguage3* iface,
    DWORD* pdwMode,
    DWORD dwFlag,
    WCHAR* lpFallBack,
    DWORD dwSrcEncoding,
    DWORD dwDstEncoding,
    IStream* pstmIn,
    IStream* pstmOut)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/*
 * TODO: handle dwFlag and lpFallBack
*/
static HRESULT WINAPI fnIMultiLanguage2_ConvertStringToUnicodeEx(
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
    FIXME("\n");
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
static HRESULT WINAPI fnIMultiLanguage2_ConvertStringFromUnicodeEx(
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

static HRESULT WINAPI fnIMultiLanguage2_DetectCodepageInIStream(
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

static HRESULT WINAPI fnIMultiLanguage2_DetectInputCodepage(
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

static HRESULT WINAPI fnIMultiLanguage2_ValidateCodePage(
    IMultiLanguage3* iface,
    UINT uiCodePage,
    HWND hwnd)
{
    FIXME("%u, %p\n", uiCodePage, hwnd);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIMultiLanguage2_GetCodePageDescription(
    IMultiLanguage3* iface,
    UINT uiCodePage,
    LCID lcid,
    LPWSTR lpWideCharStr,
    int cchWideChar)
{
    FIXME("%u, %04x, %p, %d\n", uiCodePage, lcid, lpWideCharStr, cchWideChar);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIMultiLanguage2_IsCodePageInstallable(
    IMultiLanguage3* iface,
    UINT uiCodePage)
{
    FIXME("%u\n", uiCodePage);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIMultiLanguage2_SetMimeDBSource(
    IMultiLanguage3* iface,
    MIMECONTF dwSource)
{
    FIXME("0x%08x\n", dwSource);
    return S_OK;
}

static HRESULT WINAPI fnIMultiLanguage2_GetNumberOfScripts(
    IMultiLanguage3* iface,
    UINT* pnScripts)
{
    ICOM_THIS_MULTI(MLang_impl, vtbl_IMultiLanguage3, iface);
    TRACE("%p %p\n", This, pnScripts);

    if (!pnScripts) return S_FALSE;

    *pnScripts = This->total_scripts;
    return S_OK;
}

static HRESULT WINAPI fnIMultiLanguage2_EnumScripts(
    IMultiLanguage3* iface,
    DWORD dwFlags,
    LANGID LangId,
    IEnumScript** ppEnumScript)
{
    ICOM_THIS_MULTI(MLang_impl, vtbl_IMultiLanguage3, iface);
    TRACE("%p %08x %04x %p\n", This, dwFlags, LangId, ppEnumScript);

    return EnumScript_create( This, dwFlags, LangId, ppEnumScript );
}

static HRESULT WINAPI fnIMultiLanguage2_ValidateCodePageEx(
    IMultiLanguage3* iface,
    UINT uiCodePage,
    HWND hwnd,
    DWORD dwfIODControl)
{
    ICOM_THIS_MULTI(MLang_impl, vtbl_IMultiLanguage3, iface);
    FIXME("%p %u %p %08x: stub!\n", This, uiCodePage, hwnd, dwfIODControl);

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
    ICOM_THIS_MULTI(MLang_impl, vtbl_IMultiLanguage3, iface);
    FIXME("(%p)->(%08x %s %u %p %u %p %p %p)\n", This, dwFlags, debugstr_w(lpWideCharStr),
          cchWideChar, puiPreferredCodePages, nPreferredCodePages, puiDetectedCodePages,
          pnDetectedCodePages, lpSpecialChar);
    return E_NOTIMPL;
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
    ICOM_THIS_MULTI(MLang_impl, vtbl_IMultiLanguage3, iface);
    FIXME("(%p)->(%08x %p %p %u %p %p %p)\n", This, dwFlags, pStrIn,
          puiPreferredCodePages, nPreferredCodePages, puiDetectedCodePages,
          pnDetectedCodePages, lpSpecialChar);
    return E_NOTIMPL;
}

static const IMultiLanguage3Vtbl IMultiLanguage3_vtbl =
{
    fnIMultiLanguage2_QueryInterface,
    fnIMultiLanguage2_AddRef,
    fnIMultiLanguage2_Release,
    fnIMultiLanguage2_GetNumberOfCodePageInfo,
    fnIMultiLanguage2_GetCodePageInfo,
    fnIMultiLanguage2_GetFamilyCodePage,
    fnIMultiLanguage2_EnumCodePages,
    fnIMultiLanguage2_GetCharsetInfo,
    fnIMultiLanguage2_IsConvertible,
    fnIMultiLanguage2_ConvertString,
    fnIMultiLanguage2_ConvertStringToUnicode,
    fnIMultiLanguage2_ConvertStringFromUnicode,
    fnIMultiLanguage2_ConvertStringReset,
    fnIMultiLanguage2_GetRfc1766FromLcid,
    fnIMultiLanguage2_GetLcidFromRfc1766,
    fnIMultiLanguage2_EnumRfc1766,
    fnIMultiLanguage2_GetRfc1766Info,
    fnIMultiLanguage2_CreateConvertCharset,
    fnIMultiLanguage2_ConvertStringInIStream,
    fnIMultiLanguage2_ConvertStringToUnicodeEx,
    fnIMultiLanguage2_ConvertStringFromUnicodeEx,
    fnIMultiLanguage2_DetectCodepageInIStream,
    fnIMultiLanguage2_DetectInputCodepage,
    fnIMultiLanguage2_ValidateCodePage,
    fnIMultiLanguage2_GetCodePageDescription,
    fnIMultiLanguage2_IsCodePageInstallable,
    fnIMultiLanguage2_SetMimeDBSource,
    fnIMultiLanguage2_GetNumberOfScripts,
    fnIMultiLanguage2_EnumScripts,
    fnIMultiLanguage2_ValidateCodePageEx,
    fnIMultiLanguage3_DetectOutboundCodePage,
    fnIMultiLanguage3_DetectOutboundCodePageInIStream
};

static HRESULT MultiLanguage_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    MLang_impl *mlang;
    UINT i;

    TRACE("Creating MultiLanguage object\n");

    if( pUnkOuter )
        return CLASS_E_NOAGGREGATION;

    mlang = HeapAlloc( GetProcessHeap(), 0, sizeof (MLang_impl) );
    mlang->vtbl_IMLangFontLink = &IMLangFontLink_vtbl;
    mlang->vtbl_IMultiLanguage = &IMultiLanguage_vtbl;
    mlang->vtbl_IMultiLanguage3 = &IMultiLanguage3_vtbl;

    mlang->total_cp = 0;
    for (i = 0; i < sizeof(mlang_data)/sizeof(mlang_data[0]); i++)
        mlang->total_cp += mlang_data[i].number_of_cp;

    /* do not enumerate unicode flavours */
    mlang->total_scripts = sizeof(mlang_data)/sizeof(mlang_data[0]) - 1;

    mlang->ref = 1;
    *ppObj = (LPVOID) mlang;
    TRACE("returning %p\n", mlang);

    LockModule();

    return S_OK;
}

/******************************************************************************/

HRESULT WINAPI DllCanUnloadNow(void)
{
    return dll_count == 0 ? S_OK : S_FALSE;
}

HRESULT WINAPI GetGlobalFontLinkObject(void)
{
    FIXME("\n");
    return S_FALSE;
}
