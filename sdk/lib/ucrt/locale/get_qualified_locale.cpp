/***
*getqloc.c - get qualified locale
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines __acrt_get_qualified_locale - get complete locale information
*
*******************************************************************************/
#include <corecrt_internal.h>
#include <locale.h>
#include <stdlib.h>

extern "C" {



//  local defines
#define __LOC_DEFAULT  0x1     //  default language locale for country
#define __LOC_PRIMARY  0x2     //  primary language locale for country
#define __LOC_FULL     0x4     //  fully matched language locale for country
#define __LOC_LANGUAGE 0x100   //  language default seen
#define __LOC_EXISTS   0x200   //  language is installed



//  non-NLS language string table
//  The three letter Windows names are non-standard and very limited and should not be used.
extern __crt_locale_string_table const __acrt_rg_language[]
{
    { L"american",                    L"ENU" },
    { L"american english",            L"ENU" },
    { L"american-english",            L"ENU" },
    { L"australian",                  L"ENA" },
    { L"belgian",                     L"NLB" },
    { L"canadian",                    L"ENC" },
    { L"chh",                         L"ZHH" },
    { L"chi",                         L"ZHI" },
    { L"chinese",                     L"CHS" },
    { L"chinese-hongkong",            L"ZHH" },
    { L"chinese-simplified",          L"CHS" },
    { L"chinese-singapore",           L"ZHI" },
    { L"chinese-traditional",         L"CHT" },
    { L"dutch-belgian",               L"NLB" },
    { L"english-american",            L"ENU" },
    { L"english-aus",                 L"ENA" },
    { L"english-belize",              L"ENL" },
    { L"english-can",                 L"ENC" },
    { L"english-caribbean",           L"ENB" },
    { L"english-ire",                 L"ENI" },
    { L"english-jamaica",             L"ENJ" },
    { L"english-nz",                  L"ENZ" },
    { L"english-south africa",        L"ENS" },
    { L"english-trinidad y tobago",   L"ENT" },
    { L"english-uk",                  L"ENG" },
    { L"english-us",                  L"ENU" },
    { L"english-usa",                 L"ENU" },
    { L"french-belgian",              L"FRB" },
    { L"french-canadian",             L"FRC" },
    { L"french-luxembourg",           L"FRL" },
    { L"french-swiss",                L"FRS" },
    { L"german-austrian",             L"DEA" },
    { L"german-lichtenstein",         L"DEC" },
    { L"german-luxembourg",           L"DEL" },
    { L"german-swiss",                L"DES" },
    { L"irish-english",               L"ENI" },
    { L"italian-swiss",               L"ITS" },
    { L"norwegian",                   L"NOR" },
    { L"norwegian-bokmal",            L"NOR" },
    { L"norwegian-nynorsk",           L"NON" },
    { L"portuguese-brazilian",        L"PTB" },
    { L"spanish-argentina",           L"ESS" },
    { L"spanish-bolivia",             L"ESB" },
    { L"spanish-chile",               L"ESL" },
    { L"spanish-colombia",            L"ESO" },
    { L"spanish-costa rica",          L"ESC" },
    { L"spanish-dominican republic",  L"ESD" },
    { L"spanish-ecuador",             L"ESF" },
    { L"spanish-el salvador",         L"ESE" },
    { L"spanish-guatemala",           L"ESG" },
    { L"spanish-honduras",            L"ESH" },
    { L"spanish-mexican",             L"ESM" },
    { L"spanish-modern",              L"ESN" },
    { L"spanish-nicaragua",           L"ESI" },
    { L"spanish-panama",              L"ESA" },
    { L"spanish-paraguay",            L"ESZ" },
    { L"spanish-peru",                L"ESR" },
    { L"spanish-puerto rico",         L"ESU" },
    { L"spanish-uruguay",             L"ESY" },
    { L"spanish-venezuela",           L"ESV" },
    { L"swedish-finland",             L"SVF" },
    { L"swiss",                       L"DES" },
    { L"uk",                          L"ENG" },
    { L"us",                          L"ENU" },
    { L"usa",                         L"ENU" }
};

//  non-NLS country/region string table
//  The three letter Windows names are non-standard and very limited and should not be used.
extern __crt_locale_string_table const __acrt_rg_country[]
{
    { L"america",                     L"USA" },
    { L"britain",                     L"GBR" },
    { L"china",                       L"CHN" },
    { L"czech",                       L"CZE" },
    { L"england",                     L"GBR" },
    { L"great britain",               L"GBR" },
    { L"holland",                     L"NLD" },
    { L"hong-kong",                   L"HKG" },
    { L"new-zealand",                 L"NZL" },
    { L"nz",                          L"NZL" },
    { L"pr china",                    L"CHN" },
    { L"pr-china",                    L"CHN" },
    { L"puerto-rico",                 L"PRI" },
    { L"slovak",                      L"SVK" },
    { L"south africa",                L"ZAF" },
    { L"south korea",                 L"KOR" },
    { L"south-africa",                L"ZAF" },
    { L"south-korea",                 L"KOR" },
    { L"trinidad & tobago",           L"TTO" },
    { L"uk",                          L"GBR" },
    { L"united-kingdom",              L"GBR" },
    { L"united-states",               L"USA" },
    { L"us",                          L"USA" },
};

// Number of entries in the language and country tables
extern size_t const __acrt_rg_language_count{_countof(__acrt_rg_language)};
extern size_t const __acrt_rg_country_count {_countof(__acrt_rg_country )};




//  function prototypes
BOOL __cdecl __acrt_get_qualified_locale(const __crt_locale_strings*, UINT*, __crt_locale_strings*);
static BOOL TranslateName(const __crt_locale_string_table *, int, const wchar_t **);

static void GetLocaleNameFromLangCountry (__crt_qualified_locale_data* _psetloc_data);
static BOOL CALLBACK LangCountryEnumProcEx(_In_z_ LPWSTR, DWORD, LPARAM);

static void GetLocaleNameFromLanguage (__crt_qualified_locale_data* _psetloc_data);
static BOOL CALLBACK LanguageEnumProcEx(_In_z_ LPWSTR, DWORD, LPARAM);

static void GetLocaleNameFromDefault (__crt_qualified_locale_data* _psetloc_data);

static int ProcessCodePage (LPCWSTR lpCodePageStr, __crt_qualified_locale_data* _psetloc_data);
static BOOL TestDefaultCountry(LPCWSTR localeName);
static BOOL TestDefaultLanguage (LPCWSTR localeName, BOOL bTestPrimary, __crt_qualified_locale_data* _psetloc_data);

static int GetPrimaryLen(LPCWSTR);


/***
*BOOL __acrt_get_qualified_locale - return fully qualified locale
*
*Purpose:
*       get default locale, qualify partially complete locales
*
*Entry:
*       lpInStr - input strings to be qualified
*       lpOutStr - pointer to string locale names and codepage output
*
*Exit:
*       TRUE if success, qualified locale is valid
*       FALSE if failure
*
*Exceptions:
*
*******************************************************************************/
BOOL __cdecl __acrt_get_qualified_locale(const __crt_locale_strings* lpInStr, UINT* lpOutCodePage, __crt_locale_strings* lpOutStr)
{
    int iCodePage;
    __crt_qualified_locale_data* _psetloc_data = &__acrt_getptd()->_setloc_data;
    _psetloc_data->_cacheLocaleName[0] = L'\x0'; // Initialize to invariant localename

    //  initialize pointer to call locale info routine based on operating system

    _psetloc_data->iLocState = 0;
    _psetloc_data->pchLanguage = lpInStr->szLanguage;
    _psetloc_data->pchCountry = lpInStr->szCountry;

    //  if country defined
    //  convert non-NLS country strings to three-letter abbreviations
    if (*_psetloc_data->pchCountry)
        TranslateName(__acrt_rg_country, static_cast<int>(__acrt_rg_country_count - 1),
                      &_psetloc_data->pchCountry);


    //  if language defined ...
    if (*_psetloc_data->pchLanguage)
    {
        //  and country defined
        if (*_psetloc_data->pchCountry)
        {
            //  both language and country strings defined
            //  get locale info using language and country
            GetLocaleNameFromLangCountry(_psetloc_data);
        }
        else
        {
            //  language string defined, but country string undefined
            //  get locale info using language only
            GetLocaleNameFromLanguage(_psetloc_data);
        }

        // still not done?
        if (_psetloc_data->iLocState == 0)
        {
            //  first attempt failed, try substituting the language name
            //  convert non-NLS language strings to three-letter abbrevs
            if (TranslateName(__acrt_rg_language, static_cast<int>(__acrt_rg_language_count - 1),
                              &_psetloc_data->pchLanguage))
            {
                if (*_psetloc_data->pchCountry)
                {
                    //  get locale info using language and country
                    GetLocaleNameFromLangCountry(_psetloc_data);
                }
                else
                {
                    //  get locale info using language only
                    GetLocaleNameFromLanguage(_psetloc_data);
                }
            }
        }
    }
    else
    {
        //  language is an empty string, use the User Default locale name
        GetLocaleNameFromDefault(_psetloc_data);
    }

    //  test for error in locale processing
    if (_psetloc_data->iLocState == 0)
        return FALSE;

    //  process codepage value
    if (lpInStr == nullptr || *lpInStr->szLanguage || *lpInStr->szCodePage )
    {
        // If there's no input, then get the current codepage
        // If they explicitly chose a language, use that default codepage
        // If they explicilty set a codepage, then use that
        iCodePage = ProcessCodePage(lpInStr ? lpInStr->szCodePage : nullptr, _psetloc_data);
    }
    else
    {
        // No language or codepage means that they want to set to the
        // user default settings, get that codepage (could be UTF-8)
        iCodePage = GetACP();
    }

    //  verify codepage validity
    //  CP_UTF7 is unexpected and has never been previously permitted.
    //  CP_UTF8 is the current preferred codepage
    if (!iCodePage || iCodePage == CP_UTF7 || !IsValidCodePage((WORD)iCodePage))
        return FALSE;

    //  set codepage
    if (lpOutCodePage)
    {
        *lpOutCodePage = (UINT)iCodePage;
    }

    //  set locale name and codepage results
    if (lpOutStr)
    {
        lpOutStr->szLocaleName[0] = L'\x0'; // Init the locale name to empty string

        _ERRCHECK(wcsncpy_s(lpOutStr->szLocaleName, _countof(lpOutStr->szLocaleName), _psetloc_data->_cacheLocaleName, wcslen(_psetloc_data->_cacheLocaleName) + 1));

        // Get and store the English language lang name, to be returned to user
        if (__acrt_GetLocaleInfoEx(lpOutStr->szLocaleName, LOCALE_SENGLISHLANGUAGENAME,
                                    lpOutStr->szLanguage, MAX_LANG_LEN) == 0)
            return FALSE;

        // Get and store the English language country name, to be returned to user
        if (__acrt_GetLocaleInfoEx(lpOutStr->szLocaleName, LOCALE_SENGLISHCOUNTRYNAME,
                                    lpOutStr->szCountry, MAX_CTRY_LEN) == 0)
            return FALSE;

        // Special case: Both '.' and '_' are separators in string passed to setlocale,
        // so if found in Country we use abbreviated name instead.
        if (wcschr(lpOutStr->szCountry, L'_') || wcschr(lpOutStr->szCountry, L'.'))
            if (__acrt_GetLocaleInfoEx(lpOutStr->szLocaleName, LOCALE_SABBREVCTRYNAME,
                                    lpOutStr->szCountry, MAX_CTRY_LEN) == 0)
                return FALSE;

        if (iCodePage == CP_UTF8)
        {
            // We want UTF-8 to look like utf8, not 65001
            _ERRCHECK(wcsncpy_s(lpOutStr->szCodePage, _countof(lpOutStr->szCodePage), L"utf8", 5));
        }
        else
        {
            _itow_s((int)iCodePage, (wchar_t *)lpOutStr->szCodePage, MAX_CP_LEN, 10);
        }
    }

    return TRUE;
}

/***
*BOOL TranslateName - convert known non-NLS string to NLS equivalent
*
*Purpose:
*   Provide compatibility with existing code for non-NLS strings
*
*Entry:
*   lpTable  - pointer to __crt_locale_string_table used for translation
*   high     - maximum index of table (size - 1)
*   ppchName - pointer to pointer of string to translate
*
*Exit:
*   ppchName - pointer to pointer of string possibly translated
*   TRUE if string translated, FALSE if unchanged
*
*Exceptions:
*
*******************************************************************************/
static BOOL TranslateName (
    const __crt_locale_string_table * lpTable,
    int               high,
    const wchar_t**   ppchName)
{
    int     i;
    int     cmp = 1;
    int     low = 0;

    //  typical binary search - do until no more to search or match
    while (low <= high && cmp != 0)
    {
        i = (low + high) / 2;
        cmp = _wcsicmp(*ppchName, (const wchar_t *)(*(lpTable + i)).szName);

        if (cmp == 0)
            *ppchName = (*(lpTable + i)).chAbbrev;
        else if (cmp < 0)
            high = i - 1;
        else
            low = i + 1;
    }

    return !cmp;
}

/***
*void GetLocaleNameFromLangCountry - get locale names from language and country strings
*
*Purpose:
*   Match the best locale names to the language and country string given.
*   After global variables are initialized, the LangCountryEnumProcEx
*   routine is registered as an EnumSystemLocalesEx callback to actually
*   perform the matching as the locale names are enumerated.
*
*
*WARNING:
*   This depends on an exact match with a localized string that can change!
*   It is strongly recommended that locales be selected with valid BCP-47
*   tags instead of the English names.
*
*   This API is also very brute-force and resource intensive, reading in all
*   of the locales, forcing them to be cached, and looking up their names.
*
*WARNING:
*   In the event of a 2 or 3 letter friendly name (Asu, Edo, Ewe, Yi, ...)
*   then this function will fail
*
*Entry:
*   pchLanguage     - language string
*   bAbbrevLanguage - language string is a three-letter abbreviation
*   pchCountry      - country string
*   bAbbrevCountry  - country string ia a three-letter abbreviation
*   iPrimaryLen     - length of language string with primary name
*
*Exit:
*   localeName - locale name of given language and country
*
*Exceptions:
*
*******************************************************************************/
static void GetLocaleNameFromLangCountry (__crt_qualified_locale_data* _psetloc_data)
{
    //  initialize static variables for callback use
    _psetloc_data->bAbbrevLanguage = wcslen(_psetloc_data->pchLanguage) == 3;
    _psetloc_data->bAbbrevCountry = wcslen(_psetloc_data->pchCountry) == 3;

    _psetloc_data->iPrimaryLen = _psetloc_data->bAbbrevLanguage ?
                             2 : GetPrimaryLen(_psetloc_data->pchLanguage);

    // Enumerate all locales that come with the operating system,
    // including replacement locales, but excluding alternate sorts.
    __acrt_EnumSystemLocalesEx(LangCountryEnumProcEx, LOCALE_WINDOWS | LOCALE_SUPPLEMENTAL, 0, nullptr);

    //  locale value is invalid if the language was not installed or the language
    //  was not available for the country specified
    if (!(_psetloc_data->iLocState & __LOC_LANGUAGE) ||
        !(_psetloc_data->iLocState & __LOC_EXISTS) ||
        !(_psetloc_data->iLocState & (__LOC_FULL |
                                    __LOC_PRIMARY |
                                    __LOC_DEFAULT)))
        _psetloc_data->iLocState = 0;
}

/***
*BOOL CALLBACK LangCountryEnumProcEx - callback routine for GetLocaleNameFromLangCountry
*
*Purpose:
*   Determine if locale name given matches the language in pchLanguage
*   and country in pchCountry.
*
*Entry:
*   lpLocaleString   - pointer to locale name string string
*   pchCountry     - pointer to country name
*   bAbbrevCountry - set if country is three-letter abbreviation
*
*Exit:
*   iLocState   - status of match
*       __LOC_FULL - both language and country match (best match)
*       __LOC_PRIMARY - primary language and country match (better)
*       __LOC_DEFAULT - default language and country match (good)
*       __LOC_LANGUAGE - default primary language exists
*       __LOC_EXISTS - full match of language string exists
*       (Overall match occurs for the best of FULL/PRIMARY/DEFAULT
*        and LANGUAGE/EXISTS both set.)
*   localeName - lpLocaleString matched
*   FALSE if match occurred to terminate enumeration, else TRUE.
*
*Exceptions:
*
*******************************************************************************/
static BOOL CALLBACK LangCountryEnumProcEx(LPWSTR lpLocaleString, DWORD dwFlags, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(dwFlags);
    UNREFERENCED_PARAMETER(lParam);

    __crt_qualified_locale_data* _psetloc_data = &__acrt_getptd()->_setloc_data;
    wchar_t  rgcInfo[MAX_LANG_LEN]; // MAX_LANG_LEN == MAX_CTRY_LEN == 64

    //  test locale country against input value
    if (__acrt_GetLocaleInfoEx(lpLocaleString,
                               _psetloc_data->bAbbrevCountry ? LOCALE_SABBREVCTRYNAME : LOCALE_SENGLISHCOUNTRYNAME,
                               rgcInfo, _countof(rgcInfo)) == 0)
    {
        //  set error condition and exit
        _psetloc_data->iLocState = 0;
        return TRUE;
    }

    //  if country names matched
    if (_wcsicmp(_psetloc_data->pchCountry, rgcInfo) == 0)
    {
        //  test for language match
        if (__acrt_GetLocaleInfoEx(lpLocaleString,
                                   _psetloc_data->bAbbrevLanguage ?
                                   LOCALE_SABBREVLANGNAME : LOCALE_SENGLISHLANGUAGENAME,
                                   rgcInfo, _countof(rgcInfo)) == 0)
        {
            //  set error condition and exit
            _psetloc_data->iLocState = 0;
            return TRUE;
        }

        if (_wcsicmp(_psetloc_data->pchLanguage, rgcInfo) == 0)
        {
            //  language matched also - set state and value
            //  this is the best match
            _psetloc_data->iLocState |= (__LOC_FULL |
                                       __LOC_LANGUAGE |
                                       __LOC_EXISTS);

            _ERRCHECK(wcsncpy_s(_psetloc_data->_cacheLocaleName, _countof(_psetloc_data->_cacheLocaleName), lpLocaleString, wcslen(lpLocaleString) + 1));
        }
        //  test if match already for primary langauage
        else if (!(_psetloc_data->iLocState & __LOC_PRIMARY))
        {
            //  if not, use _psetloc_data->iPrimaryLen to partial match language string
            if (_psetloc_data->iPrimaryLen && !_wcsnicmp(_psetloc_data->pchLanguage, rgcInfo, _psetloc_data->iPrimaryLen))
            {
                //  primary language matched - set locale name
                _psetloc_data->iLocState |= __LOC_PRIMARY;
                _ERRCHECK(wcsncpy_s(_psetloc_data->_cacheLocaleName, _countof(_psetloc_data->_cacheLocaleName), lpLocaleString, wcslen(lpLocaleString) + 1));
            }

            //  test if default language already defined
            else if (!(_psetloc_data->iLocState & __LOC_DEFAULT))
            {
                //  if not, test if locale language is default for country
                if (TestDefaultCountry(lpLocaleString))
                {
                    //  default language for country - set state, value
                    _psetloc_data->iLocState |= __LOC_DEFAULT;
                    _ERRCHECK(wcsncpy_s(_psetloc_data->_cacheLocaleName, _countof(_psetloc_data->_cacheLocaleName), lpLocaleString, wcslen(lpLocaleString) + 1));
                }
            }
        }
    }

    //  test if input language both exists and default primary language defined
    if ((_psetloc_data->iLocState & (__LOC_LANGUAGE | __LOC_EXISTS)) !=
                      (__LOC_LANGUAGE | __LOC_EXISTS))
    {
        //  test language match to determine whether it is installed
        if (__acrt_GetLocaleInfoEx(lpLocaleString, _psetloc_data->bAbbrevLanguage ? LOCALE_SABBREVLANGNAME
                                                                           : LOCALE_SENGLISHLANGUAGENAME,
                           rgcInfo, _countof(rgcInfo)) == 0)
        {
            //  set error condition and exit
            _psetloc_data->iLocState = 0;
            return TRUE;
        }

        // the input language matches
        if (_wcsicmp(_psetloc_data->pchLanguage, rgcInfo) == 0)
        {
            //  language matched - set bit for existance
            _psetloc_data->iLocState |= __LOC_EXISTS;

            if (_psetloc_data->bAbbrevLanguage)
            {
                //  abbreviation - set state
                //  also set language locale name if not set already
                _psetloc_data->iLocState |= __LOC_LANGUAGE;
                if (!_psetloc_data->_cacheLocaleName[0])
                    _ERRCHECK(wcsncpy_s(_psetloc_data->_cacheLocaleName, _countof(_psetloc_data->_cacheLocaleName), lpLocaleString, wcslen(lpLocaleString) + 1));
            }

            //  test if language is primary only (no sublanguage)
            else if (_psetloc_data->iPrimaryLen && ((int)wcslen(_psetloc_data->pchLanguage) == _psetloc_data->iPrimaryLen))
            {
                //  primary language only - test if default locale name
                if (TestDefaultLanguage(lpLocaleString, TRUE, _psetloc_data))
                {
                    //  default primary language - set state
                    //  also set locale name if not set already
                    _psetloc_data->iLocState |= __LOC_LANGUAGE;
                    if (!_psetloc_data->_cacheLocaleName[0])
                        _ERRCHECK(wcsncpy_s(_psetloc_data->_cacheLocaleName, _countof(_psetloc_data->_cacheLocaleName), lpLocaleString, wcslen(lpLocaleString) + 1));
                }
            }
            else
            {
                //  language with sublanguage - set state
                //  also set locale name if not set already
                _psetloc_data->iLocState |= __LOC_LANGUAGE;
                if (!_psetloc_data->_cacheLocaleName[0])
                    _ERRCHECK(wcsncpy_s(_psetloc_data->_cacheLocaleName, _countof(_psetloc_data->_cacheLocaleName), lpLocaleString, wcslen(lpLocaleString) + 1));
            }
        }
    }

    //  if LOCALE_FULL set, return FALSE to stop enumeration,
    //  else return TRUE to continue
    return (_psetloc_data->iLocState & __LOC_FULL) == 0;
}

/***
*void GetLocaleNameFromLanguage - get locale name from language string
*
*Purpose:
*   Match the best locale name to the language string given.  After global
*   variables are initialized, the LanguageEnumProcEx routine is
*   registered as an EnumSystemLocalesEx callback to actually perform
*   the matching as the locale names are enumerated.
*
*WARNING:
*   This depends on an exact match with a localized string that can change!
*   It is strongly recommended that locales be selected with valid BCP-47
*   tags instead of the English names.
*
*   This API is also very brute-force and resource intensive, reading in all
*   of the locales, forcing them to be cached, and looking up their names.
*
*WARNING:
*   In the event of a 3 letter BCP-47 tag that happens to match a Windows
*   propriatary language code, this function will return the wrong answer!
*
*WARNING:
*   In the event of a 2 or 3 letter friendly name (Asu, Edo, Ewe, Yi, ...)
*   then this function will fail
*
*Entry:
*   pchLanguage     - language string
*   bAbbrevLanguage - language string is a three-letter abbreviation
*   iPrimaryLen     - length of language string with primary name
*
*Exit:
*   localeName - locale name of language with default country
*
*Exceptions:
*
*******************************************************************************/
static void GetLocaleNameFromLanguage (__crt_qualified_locale_data* _psetloc_data)
{
    //  initialize static variables for callback use
    _psetloc_data->bAbbrevLanguage = wcslen(_psetloc_data->pchLanguage) == 3;
    _psetloc_data->iPrimaryLen = _psetloc_data->bAbbrevLanguage ? 2 : GetPrimaryLen(_psetloc_data->pchLanguage);

    // Enumerate all locales that come with the operating system, including replacement locales,
    // but excluding alternate sorts.
    __acrt_EnumSystemLocalesEx(LanguageEnumProcEx, LOCALE_WINDOWS | LOCALE_SUPPLEMENTAL, 0, nullptr);

    //  locale value is invalid if the language was not installed
    //  or the language was not available for the country specified
    if ((_psetloc_data->iLocState & __LOC_FULL) == 0)
        _psetloc_data->iLocState = 0;
}

/***
*BOOL CALLBACK LanguageEnumProcEx - callback routine for GetLocaleNameFromLanguage
*
*Purpose:
*   Determine if locale name given matches the default country for the
*   language in pchLanguage.
*
*Entry:
*   lpLocaleString    - pointer to string with locale name
*   dwFlags     - not used
*   lParam      - not used
*
*Exit:
*   localeName - locale name matched
*   FALSE if match occurred to terminate enumeration, else TRUE.
*
*Exceptions:
*
*******************************************************************************/
static BOOL CALLBACK LanguageEnumProcEx (LPWSTR lpLocaleString, DWORD dwFlags, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(dwFlags);
    UNREFERENCED_PARAMETER(lParam);

    __crt_qualified_locale_data* _psetloc_data = &__acrt_getptd()->_setloc_data;
    wchar_t    rgcInfo[120];

    //  test locale for language specified
    if (__acrt_GetLocaleInfoEx(lpLocaleString, _psetloc_data->bAbbrevLanguage ? LOCALE_SABBREVLANGNAME
                                                                              : LOCALE_SENGLISHLANGUAGENAME,
                               rgcInfo, _countof(rgcInfo)) == 0)
    {
        //  set error condition and exit
        _psetloc_data->iLocState = 0;
        return TRUE;
    }

    if (_wcsicmp(_psetloc_data->pchLanguage, rgcInfo) == 0)
    {
        //  language matches
        _ERRCHECK(wcsncpy_s(_psetloc_data->_cacheLocaleName, _countof(_psetloc_data->_cacheLocaleName), lpLocaleString, wcslen(lpLocaleString) + 1));

        _psetloc_data->iLocState |= __LOC_FULL;
    }

    return (_psetloc_data->iLocState & __LOC_FULL) == 0;
}


/***
*void GetLocaleNameFromDefault - get default locale names
*
*Purpose:
*   Set both language and country locale names to the user default.
*
*Entry:
*   None.
*
*Exceptions:
*
*******************************************************************************/
static void GetLocaleNameFromDefault (__crt_qualified_locale_data* _psetloc_data)
{
    wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
    _psetloc_data->iLocState |= (__LOC_FULL | __LOC_LANGUAGE);

    // Store the default user locale name. The returned buffer size includes the
    // terminating null character, so only store if the size returned is > 1
    if (__acrt_GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH) > 1)
    {
        _ERRCHECK(wcsncpy_s(_psetloc_data->_cacheLocaleName, _countof(_psetloc_data->_cacheLocaleName), localeName, wcslen(localeName) + 1));
    }
}

/***
*int ProcessCodePage - convert codepage string to numeric value
*
*Purpose:
*   Process codepage string consisting of a decimal string, or the
*   special case strings "ACP" and "OCP", for ANSI and OEM codepages,
*   respectively.  Null pointer or string returns the ANSI codepage.
*
*Entry:
*   lpCodePageStr - pointer to codepage string
*
*Exit:
*   Returns numeric value of codepage, zero if GetLocaleInfoEx failed.
*   (which then would mean caller aborts and locale is not set)
*
*Exceptions:
*
*******************************************************************************/
static int ProcessCodePage (LPCWSTR lpCodePageStr, __crt_qualified_locale_data* _psetloc_data)
{
    int iCodePage;

    if (!lpCodePageStr || !*lpCodePageStr || wcscmp(lpCodePageStr, L"ACP") == 0)
    {
        //  get ANSI codepage for the country locale name
        //  CONSIDER: If system is running UTF-8 ACP, then always return UTF-8?
        if (__acrt_GetLocaleInfoEx(_psetloc_data->_cacheLocaleName, LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER,
                                 (LPWSTR) &iCodePage, sizeof(iCodePage) / sizeof(wchar_t)) == 0)
            return 0;

        // Locales with no code page ("Unicode only locales") should return UTF-8
        // (0, 1 & 2 are Unicode-Only ACP, OEMCP & MacCP flags)
        if (iCodePage < 3)
        {
            return CP_UTF8;
        }

    }
    else if (_wcsicmp(lpCodePageStr, L"utf8") == 0 ||
             _wcsicmp(lpCodePageStr, L"utf-8") == 0)
    {
        // Use UTF-8
        return CP_UTF8;
    }
    else if (wcscmp(lpCodePageStr, L"OCP") == 0)
    {
        //  get OEM codepage for the country locale name
        //  CONSIDER: If system is running UTF-8 ACP, then always return UTF-8?
        if (__acrt_GetLocaleInfoEx(_psetloc_data->_cacheLocaleName, LOCALE_IDEFAULTCODEPAGE | LOCALE_RETURN_NUMBER,
                                 (LPWSTR) &iCodePage, sizeof(iCodePage) / sizeof(wchar_t)) == 0)
            return 0;

        // Locales with no code page ("unicode only locales") should return UTF-8
        // (0, 1 & 2 are Unicode-Only ACP, OEMCP & MacCP flags)
        if (iCodePage < 3)
        {
            return CP_UTF8;
        }
    }
    else
    {
         // convert decimal string to numeric value
         iCodePage = (int)_wtol(lpCodePageStr);
    }

    return iCodePage;
}

/***
*BOOL TestDefaultCountry - determine if default locale for country
*
*Purpose:
*   Determine if the locale of the given locale name has the default sublanguage.
*   This is determined by checking if the given language is neutral.
*
*Entry:
*   localeName - name of locale to test
*
*Exit:
*   Returns TRUE if default sublanguage, else FALSE.
*
*Exceptions:
*
*******************************************************************************/
static BOOL TestDefaultCountry (LPCWSTR localeName)
{
    wchar_t sIso639LangName[9]; // The maximum length for LOCALE_SISO3166CTRYNAME
                                // is 9 including nullptr

    //  Get 2-letter ISO Standard 639 or 3-letter ISO 639-2 value
    if (__acrt_GetLocaleInfoEx(localeName, LOCALE_SISO639LANGNAME,
                                sIso639LangName, _countof(sIso639LangName)) == 0)
        return FALSE;

    // Determine if this is a neutral language
    if (wcsncmp(sIso639LangName, localeName, _countof(sIso639LangName)) == 0)
        return TRUE;

    return FALSE;
}

/***
*BOOL TestDefaultLanguage - determine if default locale for language
*
*Purpose:
*   Determines if the given locale name has the default sublanguage.
*   If bTestPrimary is set, also allow TRUE when string contains an
*   implicit sublanguage.
*
*Entry:
*   localeName         - locale name of locale to test
*   bTestPrimary - set if testing if language is primary
*
*Exit:
*   Returns TRUE if sublanguage is default for locale tested.
*   If bTestPrimary set, TRUE is language has implied sublanguge.
*
*Exceptions:
*
*******************************************************************************/
static BOOL TestDefaultLanguage(LPCWSTR localeName, BOOL bTestPrimary, __crt_qualified_locale_data* _psetloc_data)
{
    if (!TestDefaultCountry (localeName))
    {
        //  test if string contains an implicit sublanguage by
        //  having a character other than upper/lowercase letters.
        if (bTestPrimary && GetPrimaryLen(_psetloc_data->pchLanguage) == (int)wcslen(_psetloc_data->pchLanguage))
            return FALSE;
    }

    return TRUE;
}

/***
*int GetPrimaryLen - get length of primary language name
*
*Purpose:
*   Determine primary language string length by scanning until
*   first non-alphabetic character.
*
*Entry:
*   pchLanguage - string to scan
*
*Exit:
*   Returns length of primary language string.
*
*Exceptions:
*
*******************************************************************************/
static int GetPrimaryLen(LPCWSTR pchLanguage)
{
    int     len = 0;
    wchar_t    ch;

    if (!pchLanguage)
        return 0;

    ch = *pchLanguage++;
    while ((ch >= L'A' && ch <= L'Z') || (ch >= L'a' && ch <= L'z'))
    {
        len++;
        ch = *pchLanguage++;
    }

    return len;
}



} // extern "C"
