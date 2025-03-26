/***
*getqloc_downlevel.c - get qualified locale for downlevel OS
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines __acrt_get_qualified_locale_downlevel - get complete locale information
*
*******************************************************************************/
#include <corecrt_internal.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>

extern "C" {

//  local defines
#define __LCID_DEFAULT  0x1     //  default language locale for country
#define __LCID_PRIMARY  0x2     //  primary language locale for country
#define __LCID_FULL     0x4     //  fully matched language locale for country
#define __LCID_LANGUAGE 0x100   //  language default seen
#define __LCID_EXISTS   0x200   //  language is installed

typedef struct tagRGLOCINFO
{
    LCID        lcid;
    wchar_t        chILanguage[8];
    wchar_t *      pchSEngLanguage;
    wchar_t        chSAbbrevLangName[4];
    wchar_t *      pchSEngCountry;
    wchar_t        chSAbbrevCtryName[4];
    wchar_t        chIDefaultCodepage[8];
    wchar_t        chIDefaultAnsiCodepage[8];
} RGLOCINFO;

static bool TranslateName(const __crt_locale_string_table *, int, const wchar_t **);

static void GetLcidFromLangCountry (__crt_qualified_locale_data_downlevel* _psetloc_downlevel_data);
static BOOL CALLBACK LangCountryEnumProc(_In_z_ PWSTR);

static void GetLcidFromLanguage (__crt_qualified_locale_data_downlevel* _psetloc_downlevel_data);
static BOOL CALLBACK LanguageEnumProc(_In_z_ PWSTR);

static void GetLcidFromCountry (__crt_qualified_locale_data_downlevel* _psetloc_downlevel_data);
static BOOL CALLBACK CountryEnumProc(_In_z_ PWSTR);

static void GetLcidFromDefault (__crt_qualified_locale_data_downlevel* _psetloc_downlevel_data);

static int ProcessCodePage (LPCWSTR lpCodePageStr, __crt_qualified_locale_data_downlevel* _psetloc_downlevel_data);
static BOOL TestDefaultCountry(LCID);
static BOOL TestDefaultLanguage (LCID lcid, BOOL bTestPrimary, __crt_qualified_locale_data_downlevel* _psetloc_downlevel_data);

static LCID LcidFromHexString(_In_z_ PCWSTR);
static int GetPrimaryLen(wchar_t const*);

//  LANGID's of locales of nondefault languages
extern __declspec(selectany) LANGID const __rglangidNotDefault[] =
{
    MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH_CANADIAN),
    MAKELANGID(LANG_SERBIAN, SUBLANG_SERBIAN_CYRILLIC),
    MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN_LUXEMBOURG),
    MAKELANGID(LANG_AFRIKAANS, SUBLANG_DEFAULT),
    MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH_BELGIAN),
    MAKELANGID(LANG_BASQUE, SUBLANG_DEFAULT),
    MAKELANGID(LANG_CATALAN, SUBLANG_DEFAULT),
    MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH_SWISS),
    MAKELANGID(LANG_ITALIAN, SUBLANG_ITALIAN_SWISS),
    MAKELANGID(LANG_SWEDISH, SUBLANG_SWEDISH_FINLAND)
};

/***
*BOOL __acrt_get_qualified_locale_downlevel - return fully qualified locale
*
*Purpose:
*       get default locale, qualify partially complete locales
*
*Entry:
*       lpInStr - input strings to be qualified
*       lpOutId - pointer to numeric LCIDs and codepage output
*       lpOutStr - pointer to string LCIDs and codepage output
*
*Exit:
*       TRUE if success, qualified locale is valid
*       FALSE if failure
*
*Exceptions:
*
*******************************************************************************/
BOOL __cdecl __acrt_get_qualified_locale_downlevel(const __crt_locale_strings* lpInStr, UINT* lpOutCodePage, __crt_locale_strings* lpOutStr)
{
    int     iCodePage;

    // Get setloc data from per thread data struct
    __crt_qualified_locale_data* _psetloc_data = &__acrt_getptd()->_setloc_data;

    // Set downlevel setloc data in per thread data struct for use by LCID downlevel APIs
    __crt_qualified_locale_data_downlevel downlevel_setloc;
    __crt_qualified_locale_data_downlevel* _psetloc_downlevel_data;

    memset(&downlevel_setloc, '\0', sizeof(__crt_qualified_locale_data_downlevel));
    _psetloc_downlevel_data = __acrt_getptd()->_setloc_downlevel_data = &downlevel_setloc;

    //  initialize pointer to call locale info routine based on operating system

    _psetloc_data->pchLanguage = lpInStr->szLanguage;

    //  convert non-NLS country strings to three-letter abbreviations
    _psetloc_data->pchCountry = lpInStr->szCountry;
    if (_psetloc_data->pchCountry && *_psetloc_data->pchCountry)
        TranslateName(__acrt_rg_country,
                      static_cast<int>(__acrt_rg_country_count - 1),
                      &_psetloc_data->pchCountry);

    _psetloc_downlevel_data->iLcidState = 0;

    if (_psetloc_data->pchLanguage && *_psetloc_data->pchLanguage)
    {
        if (_psetloc_data->pchCountry && *_psetloc_data->pchCountry)
        {
            //  both language and country strings defined
            GetLcidFromLangCountry(_psetloc_downlevel_data);
        }
        else
        {
            //  language string defined, but country string undefined
            GetLcidFromLanguage(_psetloc_downlevel_data);
        }

        if (!_psetloc_downlevel_data->iLcidState) {
            //  first attempt failed, try substituting the language name
            //  convert non-NLS language strings to three-letter abbrevs
            if (TranslateName(__acrt_rg_language,
                              static_cast<int>(__acrt_rg_language_count - 1),
                              &_psetloc_data->pchLanguage))
            {
                if (_psetloc_data->pchCountry && *_psetloc_data->pchCountry)
                {
                    GetLcidFromLangCountry(_psetloc_downlevel_data);
                }
                else
                {
                    GetLcidFromLanguage(_psetloc_downlevel_data);
                }
            }
        }
    }
    else
    {
        if (_psetloc_data->pchCountry && *_psetloc_data->pchCountry)
        {
            //  country string defined, but language string undefined
            GetLcidFromCountry(_psetloc_downlevel_data);
        }
        else
        {
            //  both language and country strings undefined
            GetLcidFromDefault(_psetloc_downlevel_data);
        }
    }

    //  test for error in LCID processing
    if (!_psetloc_downlevel_data->iLcidState)
        return FALSE;

    //  process codepage value
    iCodePage = ProcessCodePage(lpInStr ? lpInStr->szCodePage: nullptr, _psetloc_downlevel_data);

    //  verify codepage validity
    if (!iCodePage || !IsValidCodePage((WORD)iCodePage))
        return FALSE;

    //  verify locale is installed
    if (!IsValidLocale(_psetloc_downlevel_data->lcidLanguage, LCID_INSTALLED))
        return FALSE;

    //  set codepage
    if (lpOutCodePage)
    {
        *lpOutCodePage = (UINT)iCodePage;
    }

    // store locale name in cache
    __acrt_LCIDToLocaleName(
        _psetloc_downlevel_data->lcidLanguage,
        _psetloc_data->_cacheLocaleName,
        (int)_countof(_psetloc_data->_cacheLocaleName),
        0);

    //  set locale name and codepage results
    if (lpOutStr)
    {
        __acrt_LCIDToLocaleName(
            _psetloc_downlevel_data->lcidLanguage,
            lpOutStr->szLocaleName,
            (int)_countof(lpOutStr->szLocaleName),
            0);

        if (GetLocaleInfoW(_psetloc_downlevel_data->lcidLanguage, LOCALE_SENGLANGUAGE,
                                 lpOutStr->szLanguage, MAX_LANG_LEN) == 0)
            return FALSE;

        if (GetLocaleInfoW(_psetloc_downlevel_data->lcidCountry, LOCALE_SENGCOUNTRY,
                                 lpOutStr->szCountry, MAX_CTRY_LEN) == 0)
            return FALSE;

        _itow_s((int)iCodePage, (wchar_t *)lpOutStr->szCodePage, MAX_CP_LEN, 10);
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
static bool TranslateName (
    const __crt_locale_string_table* lpTable,
    int                              high,
    const wchar_t **                 ppchName)
{
    int low = 0;

    //  typical binary search - do until no more to search or match
    while (low <= high)
    {
        int const i = (low + high) / 2;
        int const cmp = _wcsicmp(*ppchName, lpTable[i].szName);

        if (cmp == 0)
        {
            *ppchName = lpTable[i].chAbbrev;
            return true;
        }
        else if (cmp < 0)
            high = i - 1;
        else
            low = i + 1;
    }

    return false;
}

/***
*void GetLcidFromLangCountry - get LCIDs from language and country strings
*
*Purpose:
*   Match the best LCIDs to the language and country string given.
*   After global variables are initialized, the LangCountryEnumProc
*   routine is registered as an EnumSystemLocalesA callback to actually
*   perform the matching as the LCIDs are enumerated.
*
*Entry:
*   pchLanguage     - language string
*   bAbbrevLanguage - language string is a three-letter abbreviation
*   pchCountry      - country string
*   bAbbrevCountry  - country string ia a three-letter abbreviation
*   iPrimaryLen     - length of language string with primary name
*
*Exit:
*   lcidLanguage - LCID of language string
*   lcidCountry  - LCID of country string
*
*Exceptions:
*
*******************************************************************************/
static void GetLcidFromLangCountry (__crt_qualified_locale_data_downlevel* _psetloc_downlevel_data)
{
    __crt_qualified_locale_data*    _psetloc_data = &__acrt_getptd()->_setloc_data;

    //  initialize static variables for callback use
    _psetloc_data->bAbbrevLanguage = wcslen(_psetloc_data->pchLanguage) == 3;
    _psetloc_data->bAbbrevCountry = wcslen(_psetloc_data->pchCountry) == 3;
    _psetloc_downlevel_data->lcidLanguage = 0;
    _psetloc_data->iPrimaryLen = _psetloc_data->bAbbrevLanguage ?
                             2 : GetPrimaryLen(_psetloc_data->pchLanguage);

    EnumSystemLocalesW(LangCountryEnumProc, LCID_INSTALLED);

    //  locale value is invalid if the language was not installed or the language
    //  was not available for the country specified
    if (!(_psetloc_downlevel_data->iLcidState & __LCID_LANGUAGE) ||
        !(_psetloc_downlevel_data->iLcidState & __LCID_EXISTS) ||
        !(_psetloc_downlevel_data->iLcidState & (__LCID_FULL |
                                    __LCID_PRIMARY |
                                    __LCID_DEFAULT)))
        _psetloc_downlevel_data->iLcidState = 0;
}

/***
*BOOL CALLBACK LangCountryEnumProc - callback routine for GetLcidFromLangCountry
*
*Purpose:
*   Determine if LCID given matches the language in pchLanguage
*   and country in pchCountry.
*
*Entry:
*   lpLcidString   - pointer to string with decimal LCID
*   pchCountry     - pointer to country name
*   bAbbrevCountry - set if country is three-letter abbreviation
*
*Exit:
*   iLcidState   - status of match
*       __LCID_FULL - both language and country match (best match)
*       __LCID_PRIMARY - primary language and country match (better)
*       __LCID_DEFAULT - default language and country match (good)
*       __LCID_LANGUAGE - default primary language exists
*       __LCID_EXISTS - full match of language string exists
*       (Overall match occurs for the best of FULL/PRIMARY/DEFAULT
*        and LANGUAGE/EXISTS both set.)
*   lcidLanguage - LCID matched
*   lcidCountry  - LCID matched
*   FALSE if match occurred to terminate enumeration, else TRUE.
*
*Exceptions:
*
*******************************************************************************/
static BOOL CALLBACK LangCountryEnumProc (_In_z_ PWSTR lpLcidString)
{
    __crt_qualified_locale_data*    _psetloc_data = &__acrt_getptd()->_setloc_data;
    __crt_qualified_locale_data_downlevel*    _psetloc_downlevel_data = __acrt_getptd()->_setloc_downlevel_data;
    LCID    lcid = LcidFromHexString(lpLcidString);
    wchar_t    rgcInfo[120];

    //  test locale country against input value
    if (GetLocaleInfoW(lcid,
                             _psetloc_data->bAbbrevCountry ?
                             LOCALE_SABBREVCTRYNAME : LOCALE_SENGCOUNTRY,
                             rgcInfo, _countof(rgcInfo)) == 0)
    {
        //  set error condition and exit
        _psetloc_downlevel_data->iLcidState = 0;
        return TRUE;
    }
    if (!_wcsicmp(_psetloc_data->pchCountry, rgcInfo))
    {
        //  country matched - test for language match
        if (GetLocaleInfoW(lcid,
                                 _psetloc_data->bAbbrevLanguage ?
                                 LOCALE_SABBREVLANGNAME : LOCALE_SENGLANGUAGE,
                                 rgcInfo, _countof(rgcInfo)) == 0)
        {
            //  set error condition and exit
            _psetloc_downlevel_data->iLcidState = 0;
            return TRUE;
        }
        if (!_wcsicmp(_psetloc_data->pchLanguage, rgcInfo))
        {
            //  language matched also - set state and value
            _psetloc_downlevel_data->iLcidState |= (__LCID_FULL |
                                       __LCID_LANGUAGE |
                                       __LCID_EXISTS);
            _psetloc_downlevel_data->lcidLanguage = _psetloc_downlevel_data->lcidCountry = lcid;
        }

        //  test if match already for primary langauage
        else if (!(_psetloc_downlevel_data->iLcidState & __LCID_PRIMARY))
        {
            //  if not, use _psetloc_data->iPrimaryLen to partial match language string
            if (_psetloc_data->iPrimaryLen && !_wcsnicmp(_psetloc_data->pchLanguage, rgcInfo, _psetloc_data->iPrimaryLen))
            {
                //  primary language matched - set state and country LCID
                _psetloc_downlevel_data->iLcidState |= __LCID_PRIMARY;
                _psetloc_downlevel_data->lcidCountry = lcid;

                //  if language is primary only (no subtype), set language LCID
                if ((int)wcslen(_psetloc_data->pchLanguage) == _psetloc_data->iPrimaryLen)
                    _psetloc_downlevel_data->lcidLanguage = lcid;
            }

            //  test if default language already defined
            else if (!(_psetloc_downlevel_data->iLcidState & __LCID_DEFAULT))
            {
                //  if not, test if locale language is default for country
                if (TestDefaultCountry(lcid))
                {
                    //  default language for country - set state, value
                    _psetloc_downlevel_data->iLcidState |= __LCID_DEFAULT;
                    _psetloc_downlevel_data->lcidCountry = lcid;
                }
            }
        }
    }
    //  test if input language both exists and default primary language defined
    if ((_psetloc_downlevel_data->iLcidState & (__LCID_LANGUAGE | __LCID_EXISTS)) !=
                      (__LCID_LANGUAGE | __LCID_EXISTS))
    {
        //  test language match to determine whether it is installed
        if (GetLocaleInfoW(lcid, _psetloc_data->bAbbrevLanguage ? LOCALE_SABBREVLANGNAME
                                                       : LOCALE_SENGLANGUAGE,
                           rgcInfo, _countof(rgcInfo)) == 0)
        {
            //  set error condition and exit
            _psetloc_downlevel_data->iLcidState = 0;
            return TRUE;
        }

        if (!_wcsicmp(_psetloc_data->pchLanguage, rgcInfo))
        {
            //  language matched - set bit for existance
            _psetloc_downlevel_data->iLcidState |= __LCID_EXISTS;

            if (_psetloc_data->bAbbrevLanguage)
            {
                //  abbreviation - set state
                //  also set language LCID if not set already
                _psetloc_downlevel_data->iLcidState |= __LCID_LANGUAGE;
                if (!_psetloc_downlevel_data->lcidLanguage)
                    _psetloc_downlevel_data->lcidLanguage = lcid;
            }

            //  test if language is primary only (no sublanguage)
            else if (_psetloc_data->iPrimaryLen && ((int)wcslen(_psetloc_data->pchLanguage) == _psetloc_data->iPrimaryLen))
            {
                //  primary language only - test if default LCID
                if (TestDefaultLanguage(lcid, TRUE, _psetloc_downlevel_data))
                {
                    //  default primary language - set state
                    //  also set LCID if not set already
                    _psetloc_downlevel_data->iLcidState |= __LCID_LANGUAGE;
                    if (!_psetloc_downlevel_data->lcidLanguage)
                        _psetloc_downlevel_data->lcidLanguage = lcid;
                }
            }
            else
            {
                //  language with sublanguage - set state
                //  also set LCID if not set already
                _psetloc_downlevel_data->iLcidState |= __LCID_LANGUAGE;
                if (!_psetloc_downlevel_data->lcidLanguage)
                    _psetloc_downlevel_data->lcidLanguage = lcid;
            }
        }
        else if (!_psetloc_data->bAbbrevLanguage && _psetloc_data->iPrimaryLen
                               && !_wcsicmp(_psetloc_data->pchLanguage, rgcInfo))
        {
            //  primary language match - test for default language only
            if (TestDefaultLanguage(lcid, FALSE, _psetloc_downlevel_data))
            {
                //  default primary language - set state
                //  also set LCID if not set already
                _psetloc_downlevel_data->iLcidState |= __LCID_LANGUAGE;
                if (!_psetloc_downlevel_data->lcidLanguage)
                    _psetloc_downlevel_data->lcidLanguage = lcid;
            }
        }
    }

    //  if LOCALE_FULL set, return FALSE to stop enumeration,
    //  else return TRUE to continue
    return (_psetloc_downlevel_data->iLcidState & __LCID_FULL) == 0;
}

/***
*void GetLcidFromLanguage - get LCIDs from language string
*
*Purpose:
*   Match the best LCIDs to the language string given.  After global
*   variables are initialized, the LanguageEnumProc routine is
*   registered as an EnumSystemLocalesA callback to actually perform
*   the matching as the LCIDs are enumerated.
*
*Entry:
*   pchLanguage     - language string
*   bAbbrevLanguage - language string is a three-letter abbreviation
*   iPrimaryLen     - length of language string with primary name
*
*Exit:
*   lcidLanguage - lcidCountry  - LCID of language with default
*                                 country
*
*Exceptions:
*
*******************************************************************************/
static void GetLcidFromLanguage (__crt_qualified_locale_data_downlevel* _psetloc_downlevel_data)
{
    __crt_qualified_locale_data* _psetloc_data = &__acrt_getptd()->_setloc_data;

    //  initialize static variables for callback use
    _psetloc_data->bAbbrevLanguage = wcslen(_psetloc_data->pchLanguage) == 3;
    _psetloc_data->iPrimaryLen = _psetloc_data->bAbbrevLanguage ? 2 : GetPrimaryLen(_psetloc_data->pchLanguage);

    EnumSystemLocalesW(LanguageEnumProc, LCID_INSTALLED);

    //  locale value is invalid if the language was not installed
    //  or the language was not available for the country specified
    if (!(_psetloc_downlevel_data->iLcidState & __LCID_FULL))
        _psetloc_downlevel_data->iLcidState = 0;
}

/***
*BOOL CALLBACK LanguageEnumProc - callback routine for GetLcidFromLanguage
*
*Purpose:
*   Determine if LCID given matches the default country for the
*   language in pchLanguage.
*
*Entry:
*   lpLcidString    - pointer to string with decimal LCID
*   pchLanguage     - pointer to language name
*   bAbbrevLanguage - set if language is three-letter abbreviation
*
*Exit:
*   lcidLanguage - lcidCountry - LCID matched
*   FALSE if match occurred to terminate enumeration, else TRUE.
*
*Exceptions:
*
*******************************************************************************/
static BOOL CALLBACK LanguageEnumProc (_In_z_ PWSTR lpLcidString)
{
    __crt_qualified_locale_data*    _psetloc_data = &__acrt_getptd()->_setloc_data;
    __crt_qualified_locale_data_downlevel*    _psetloc_downlevel_data = __acrt_getptd()->_setloc_downlevel_data;

    LCID    lcid = LcidFromHexString(lpLcidString);
    wchar_t    rgcInfo[120];

    //  test locale for language specified
    if (GetLocaleInfoW(lcid, _psetloc_data->bAbbrevLanguage ? LOCALE_SABBREVLANGNAME
                                                   : LOCALE_SENGLANGUAGE,
                       rgcInfo, _countof(rgcInfo)) == 0)
    {
        //  set error condition and exit
        _psetloc_downlevel_data->iLcidState = 0;
        return TRUE;
    }

    if (!_wcsicmp(_psetloc_data->pchLanguage, rgcInfo))
    {
        //  language matched - test if locale country is default
        //  or if locale is implied in the language string
        if (_psetloc_data->bAbbrevLanguage || TestDefaultLanguage(lcid, TRUE, _psetloc_downlevel_data))
        {
            //  this locale has the default country
            _psetloc_downlevel_data->lcidLanguage = _psetloc_downlevel_data->lcidCountry = lcid;
            _psetloc_downlevel_data->iLcidState |= __LCID_FULL;
        }
    }
    else if (!_psetloc_data->bAbbrevLanguage && _psetloc_data->iPrimaryLen
                              && !_wcsicmp(_psetloc_data->pchLanguage, rgcInfo))
    {
        //  primary language matched - test if locale country is default
        if (TestDefaultLanguage(lcid, FALSE, _psetloc_downlevel_data))
        {
            //  this is the default country
            _psetloc_downlevel_data->lcidLanguage = _psetloc_downlevel_data->lcidCountry = lcid;
            _psetloc_downlevel_data->iLcidState |= __LCID_FULL;
        }
    }

    return (_psetloc_downlevel_data->iLcidState & __LCID_FULL) == 0;
}

/***
*void GetLcidFromCountry - get LCIDs from country string
*
*Purpose:
*   Match the best LCIDs to the country string given.  After global
*   variables are initialized, the CountryEnumProc routine is
*   registered as an EnumSystemLocalesA callback to actually perform
*   the matching as the LCIDs are enumerated.
*
*Entry:
*   pchCountry     - country string
*   bAbbrevCountry - country string is a three-letter abbreviation
*
*Exit:
*   lcidLanguage - lcidCountry  - LCID of country with default
*                                 language
*
*Exceptions:
*
*******************************************************************************/
static void GetLcidFromCountry (__crt_qualified_locale_data_downlevel* _psetloc_downlevel_data)
{
    __crt_qualified_locale_data*    _psetloc_data = &__acrt_getptd()->_setloc_data;
    _psetloc_data->bAbbrevCountry = wcslen(_psetloc_data->pchCountry) == 3;

    EnumSystemLocalesW(CountryEnumProc, LCID_INSTALLED);

    //  locale value is invalid if the country was not defined or
    //  no default language was found
    if (!(_psetloc_downlevel_data->iLcidState & __LCID_FULL))
        _psetloc_downlevel_data->iLcidState = 0;
}

/***
*BOOL CALLBACK CountryEnumProc - callback routine for GetLcidFromCountry
*
*Purpose:
*   Determine if LCID given matches the default language for the
*   country in pchCountry.
*
*Entry:
*   lpLcidString   - pointer to string with decimal LCID
*   pchCountry     - pointer to country name
*   bAbbrevCountry - set if country is three-letter abbreviation
*
*Exit:
*   lcidLanguage - lcidCountry - LCID matched
*   FALSE if match occurred to terminate enumeration, else TRUE.
*
*Exceptions:
*
*******************************************************************************/
static BOOL CALLBACK CountryEnumProc (_In_z_ PWSTR lpLcidString)
{
    __crt_qualified_locale_data*    _psetloc_data = &__acrt_getptd()->_setloc_data;
    __crt_qualified_locale_data_downlevel*    _psetloc_downlevel_data = __acrt_getptd()->_setloc_downlevel_data;
    LCID    lcid = LcidFromHexString(lpLcidString);
    wchar_t    rgcInfo[120];

    //  test locale for country specified
    if (GetLocaleInfoW(lcid, _psetloc_data->bAbbrevCountry ? LOCALE_SABBREVCTRYNAME
                                                  : LOCALE_SENGCOUNTRY,
                       rgcInfo, _countof(rgcInfo)) == 0)
    {
        //  set error condition and exit
        _psetloc_downlevel_data->iLcidState = 0;
        return TRUE;
    }
    if (!_wcsicmp(_psetloc_data->pchCountry, rgcInfo))
    {
        //  language matched - test if locale country is default
        if (TestDefaultCountry(lcid))
        {
            //  this locale has the default language
            _psetloc_downlevel_data->lcidLanguage = _psetloc_downlevel_data->lcidCountry = lcid;
            _psetloc_downlevel_data->iLcidState |= __LCID_FULL;
        }
    }
    return (_psetloc_downlevel_data->iLcidState & __LCID_FULL) == 0;
}

/***
*void GetLcidFromDefault - get default LCIDs
*
*Purpose:
*   Set both language and country LCIDs to the system default.
*
*Entry:
*   None.
*
*Exit:
*   lcidLanguage - set to system LCID
*   lcidCountry  - set to system LCID
*
*Exceptions:
*
*******************************************************************************/
static void GetLcidFromDefault (__crt_qualified_locale_data_downlevel* _psetloc_downlevel_data)
{
    _psetloc_downlevel_data->iLcidState |= (__LCID_FULL | __LCID_LANGUAGE);
    _psetloc_downlevel_data->lcidLanguage = _psetloc_downlevel_data->lcidCountry = GetUserDefaultLCID();
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
*   Returns numeric value of codepage.
*
*Exceptions:
*
*******************************************************************************/
static int ProcessCodePage (LPCWSTR lpCodePageStr, __crt_qualified_locale_data_downlevel* _psetloc_downlevel_data)
{
    int iCodePage;

    if (!lpCodePageStr || !*lpCodePageStr || !wcscmp(lpCodePageStr, L"ACP"))
    {
        //  get ANSI codepage for the country LCID
        if (GetLocaleInfoW(_psetloc_downlevel_data->lcidCountry, LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER,
                                 (LPWSTR) &iCodePage, sizeof(iCodePage) / sizeof(wchar_t)) == 0)
            return 0;

        if (iCodePage == 0) // for locales have no assoicated ANSI codepage, e.g. Hindi locale
            return GetACP();
    }
    else if (!wcscmp(lpCodePageStr, L"OCP"))
    {
        //  get OEM codepage for the country LCID
        if (GetLocaleInfoW(_psetloc_downlevel_data->lcidCountry, LOCALE_IDEFAULTCODEPAGE | LOCALE_RETURN_NUMBER,
                                 (LPWSTR) &iCodePage, sizeof(iCodePage) / sizeof(wchar_t)) == 0)
            return 0;
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
*   Using a hardcoded list, determine if the locale of the given LCID
*   has the default sublanguage for the locale primary language.  The
*   list contains the locales NOT having the default sublanguage.
*
*Entry:
*   lcid - LCID of locale to test
*
*Exit:
*   Returns TRUE if default sublanguage, else FALSE.
*
*Exceptions:
*
*******************************************************************************/
static BOOL TestDefaultCountry (LCID lcid)
{
    LANGID  langid = LANGIDFROMLCID(lcid);
    int     i;

    for (i = 0; i < _countof(__rglangidNotDefault); i++)
    {
        if (langid == __rglangidNotDefault[i])
            return FALSE;
    }
    return TRUE;
}

/***
*BOOL TestDefaultLanguage - determine if default locale for language
*
*Purpose:
*   Determines if the given LCID has the default sublanguage.
*   If bTestPrimary is set, also allow TRUE when string contains an
*   implicit sublanguage.
*
*Entry:
*   LCID         - lcid of locale to test
*   bTestPrimary - set if testing if language is primary
*
*Exit:
*   Returns TRUE if sublanguage is default for locale tested.
*   If bTestPrimary set, TRUE is language has implied sublanguge.
*
*Exceptions:
*
*******************************************************************************/
static BOOL TestDefaultLanguage (LCID lcid, BOOL bTestPrimary, __crt_qualified_locale_data_downlevel* _psetloc_downlevel_data)
{
    UNREFERENCED_PARAMETER(_psetloc_downlevel_data); // CRT_REFACTOR TODO

    DWORD dwLanguage;
    LCID lcidDefault = MAKELCID(MAKELANGID(PRIMARYLANGID(LANGIDFROMLCID(lcid)), SUBLANG_DEFAULT), SORT_DEFAULT);
    __crt_qualified_locale_data* _psetloc_data = &__acrt_getptd()->_setloc_data;

    if (GetLocaleInfoW(lcidDefault, LOCALE_ILANGUAGE | LOCALE_RETURN_NUMBER,
                                          (LPWSTR) &dwLanguage, sizeof(dwLanguage) / sizeof(wchar_t)) == 0)
        return FALSE;

    if (lcid != dwLanguage)
    {
        //  test if string contains an implicit sublanguage by
        //  having a character other than upper/lowercase letters.
        if (bTestPrimary && GetPrimaryLen(_psetloc_data->pchLanguage) == (int)wcslen(_psetloc_data->pchLanguage))
            return FALSE;
    }
    return TRUE;
}

/***
*LCID LcidFromHexString - convert hex string to value for LCID
*
*Purpose:
*   LCID values returned in hex ANSI strings - straight conversion
*
*Entry:
*   lpHexString - pointer to hex string to convert
*
*Exit:
*   Returns LCID computed.
*
*Exceptions:
*
*******************************************************************************/
static LCID LcidFromHexString (_In_z_ PCWSTR lpHexString)
{
    wchar_t    ch;
    DWORD   lcid = 0;

#pragma warning(disable:__WARNING_POTENTIAL_BUFFER_OVERFLOW_NULLTERMINATED) // 26018 This is an idiomatic nul termination check that Prefast doesn't understand.
    while ((ch = *lpHexString++) != '\0')
    {
        if (ch >= 'a' && ch <= 'f')
            ch += static_cast<wchar_t>('9' + 1 - 'a');
        else if (ch >= 'A' && ch <= 'F')
            ch += static_cast<wchar_t>('9' + 1 - 'A');
        lcid = lcid * 0x10 + ch - '0';
    }

    return (LCID)lcid;
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
static int GetPrimaryLen (wchar_t const* pchLanguage)
{
    int     len = 0;
    wchar_t    ch;

    ch = *pchLanguage++;
    while ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))
    {
        len++;
        ch = *pchLanguage++;
    }

    return len;
}

} // extern "C"
