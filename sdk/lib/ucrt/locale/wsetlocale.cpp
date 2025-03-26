/***
*wsetlocale.cpp - Contains the wsetlocale function
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Contains the wsetlocale() function.
*
****/

#include <corecrt_internal.h>
#include <locale.h>

extern "C" {

static const char _first_127char[] = {
        1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17,
        18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34,
        35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68,
        69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85,
        86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100,101,102,
        103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,
        120,121,122,123,124,125,126,127
};

extern __crt_locale_data __acrt_initial_locale_data;
extern const unsigned short _wctype[];
static const unsigned short *_ctype_loc_style = _wctype + 2;

long __acrt_locale_changed_data = FALSE;

/* helper function prototypes */
_Success_(return != 0)
static wchar_t * _expandlocale(
    _In_z_                                wchar_t const * expr,
    _Out_writes_z_(sizeInChars)           wchar_t *       output,
    _In_                                  size_t          sizeInChars,
    _Out_writes_z_(localeNameSizeInChars) wchar_t *       localeNameOutput,
    _In_                                  size_t          localeNameSizeInChars,
    _Out_                                 UINT&           cp
    );

void _wcscats(_Inout_updates_z_(_Param_(2)) wchar_t *, size_t, int, ...);
void __lc_lctowcs(_Inout_updates_z_(_Param_(2)) wchar_t *, size_t, const __crt_locale_strings *);
int __lc_wcstolc(__crt_locale_strings *, const wchar_t *);
static wchar_t * __cdecl _wsetlocale_set_cat(__crt_locale_data*, int, const wchar_t *);
static wchar_t * __cdecl _wsetlocale_get_all(__crt_locale_data*);
static __crt_locale_data* __cdecl _updatetlocinfo_nolock(void);
static wchar_t * __cdecl _wsetlocale_nolock(__crt_locale_data*, int, const wchar_t *);
int __cdecl _setmbcp_nolock(int, __crt_multibyte_data*);
__crt_locale_data* __cdecl _updatetlocinfoEx_nolock(__crt_locale_data**, __crt_locale_data*);

void __cdecl __acrt_set_locale_changed()
{
    _InterlockedExchange(&__acrt_locale_changed_data, TRUE);
}

/***
* _configthreadlocale(int i)
*
* Purpose:
*       To set _own_locale flag on threadinfo sturct. If this flag is set, this thread
*       is going own it's threadlocinfo struct. Setlocale call on other thread will have
*       no effect on this thread's locale. If 0 is passed then nothing is changed, but
*       current status is returned.
* Exit   :
*       Returns the current status - i.e. per thread locale is enabled or not.
*
*******************************************************************************/
int __cdecl _configthreadlocale(int i)
{
    /*
     * ownlocale flag struct:
     * bits: 000000000000000000000000 000W 00P1
     * P is set when _ENABLE_PER_THREAD_LOCALE is called for this thread
     * or _ENABLE_PER_THREAD_LOCALE_NEW was set when this thread was created.
     * W is set when _WSETLOCALE_AVOID_SYNC_LOCALE_BIT is set by _wsetlocale.
     * It is used to disable global-ptd resynchronization during a call to _wsetlocale.
     *
     * __globallocalestatus structure:
     * bits: 111111111111111111111111 1111 1N1G
     * G is set if _ENABLE_PER_THREAD_LOCALE_GLOBAL is set.
     * G is 0 if _ENABLE_PER_THREAD_LOCALE_GLOBAL is not set.
     * N is set if _ENABLE_PER_THREAD_LOCALE_NEW is set.
     * N is 0 if _ENABLE_PER_THREAD_LOCALE_NEW is not set.
     */
    __acrt_ptd* const ptd = __acrt_getptd();
    int retval = (ptd->_own_locale & _PER_THREAD_LOCALE_BIT)==0 ? _DISABLE_PER_THREAD_LOCALE:_ENABLE_PER_THREAD_LOCALE;

    switch(i)
    {
        case _ENABLE_PER_THREAD_LOCALE:
            // same behavior as __acrt_disable_global_locale_sync()
            ptd->_own_locale = ptd->_own_locale | _PER_THREAD_LOCALE_BIT;
            break;

        case _DISABLE_PER_THREAD_LOCALE:
            // same behavior as __acrt_enable_global_locale_sync()
            ptd->_own_locale = ptd->_own_locale & ~_PER_THREAD_LOCALE_BIT;
            break;

        case 0:
            break;

        /* used only during dll startup for linkopt */
        case -1:
            __globallocalestatus = -1;
            break;

        default:
            _VALIDATE_RETURN(("Invalid parameter for _configthreadlocale",0),EINVAL,-1);
            break;
    }

    return retval;
}

extern "C" void __cdecl __acrt_uninitialize_locale()
{
    __acrt_lock_and_call(__acrt_locale_lock, [&]
    {
        __acrt_current_locale_data.uninitialize([](__crt_locale_data*& locale)
        {
            if (locale == &__acrt_initial_locale_data)
            {
                return;
            }

            locale = _updatetlocinfoEx_nolock(&locale, &__acrt_initial_locale_data);
        });
    });
}

/***
*_free_locale() - free threadlocinfo
*
*Purpose:
*       Free up the per-thread locale info structure specified by the passed
*       pointer.
*
*Entry:
*       __crt_locale_data* ptloci
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _free_locale(
        _locale_t plocinfo
        )
{
    if (plocinfo != nullptr)
    {
        __acrt_lock(__acrt_multibyte_cp_lock);
        __try
        {
            if (plocinfo->mbcinfo != nullptr &&
                    InterlockedDecrement(&(plocinfo->mbcinfo->refcount)) == 0 &&
                    plocinfo->mbcinfo != &__acrt_initial_multibyte_data )
            {
                _free_crt(plocinfo->mbcinfo);
            }
        }
        __finally
        {
            __acrt_unlock(__acrt_multibyte_cp_lock);
        }
        __endtry

        if (plocinfo->locinfo != nullptr)
        {
            /*
             * this portion has to be in locale lock as there may be case when
             * not this thread but some other thread is still holding to this
             * locale and is also trying to free this locale. In this case
             * we may end up leaking memory.
             */

            __acrt_lock(__acrt_locale_lock);
            __try
            {
                __acrt_release_locale_ref(plocinfo->locinfo);
                if ( (plocinfo->locinfo != nullptr) &&
                     (plocinfo->locinfo->refcount == 0) &&
                     (plocinfo->locinfo != &__acrt_initial_locale_data) )
                    __acrt_free_locale(plocinfo->locinfo);
            }
            __finally
            {
                __acrt_unlock(__acrt_locale_lock);
            }
            __endtry
        }

        _free_crt(plocinfo);
    }
}

/*
* _locale_t _get_current_locale() -
*    Gets the current locale setting.
*
* Purpose:
*       Gets the current locale setting for this thread. Returns locale
*       in form of _locale_t, which then can be used with other locale
*       aware string functions.
*
*/

_locale_t __cdecl _get_current_locale(void)
{
    __acrt_ptd* const ptd = __acrt_getptd();

    auto locale_copy(_calloc_crt_t(__crt_locale_pointers, 1));
    if (!locale_copy)
        return nullptr; // calloc will set errno

    __acrt_update_thread_locale_data();
    __acrt_update_thread_multibyte_data();

    // No one can free the data pointed to by ptlocinfo while we're copying
    // it, since we're copying this thread's ptlocinfo, which won't be updated
    // during the copy.  So there are no worries about it being freed from
    // under us.  We still need a lock while adding a reference for the new
    // copy, though, because of the race condition found in _wsetlocale.
    locale_copy.get()->locinfo = ptd->_locale_info;
    locale_copy.get()->mbcinfo = ptd->_multibyte_info;
    __acrt_lock_and_call(__acrt_locale_lock, [&]
    {
        __acrt_add_locale_ref(locale_copy.get()->locinfo);
    });

    __acrt_lock_and_call(__acrt_multibyte_cp_lock, [&]
    {
        InterlockedIncrement(&locale_copy.get()->mbcinfo->refcount);
    });

    return locale_copy.detach();
}

// Enclaves do not support dynamic locales, but they support reading the default one.
#ifndef _UCRT_ENCLAVE_BUILD

// This is used as the initializer if a category does not need initialization.
static int __cdecl no_op_initialize(__crt_locale_data*)
{
    return 0;
}

void __cdecl __acrt_free_locale(__crt_locale_data*);

_CRT_LINKER_FORCE_INCLUDE(__acrt_locale_terminator);

extern __declspec(selectany) struct {
        const wchar_t * catname;
        wchar_t * locale;
        int (* init)(__crt_locale_data*);
} const __lc_category[LC_MAX-LC_MIN+1] = {
        /* code assumes locale initialization is "__wclocalestr" */
        { L"LC_ALL",     nullptr,                        no_op_initialize                  },
        { L"LC_COLLATE", __acrt_wide_c_locale_string,    no_op_initialize                  },
        { L"LC_CTYPE",   __acrt_wide_c_locale_string,    __acrt_locale_initialize_ctype    },
        { L"LC_MONETARY",__acrt_wide_c_locale_string,    __acrt_locale_initialize_monetary },
        { L"LC_NUMERIC", __acrt_wide_c_locale_string,    __acrt_locale_initialize_numeric  },
        { L"LC_TIME",    __acrt_wide_c_locale_string,    __acrt_locale_initialize_time     }
};

/***
*
* _copytlocinfo_nolock(__crt_locale_data* ptlocid, __crt_locale_data* ptlocis)
*
* Purpose:
*       Copy the contents of ptlocis to ptlocid and increase the refcount of all the
*       elements in ptlocid after copy.
*
******************************************************************************/
static void __cdecl _copytlocinfo_nolock(
    __crt_locale_data* ptlocid,
    __crt_locale_data* ptlocis)
{
    if (ptlocis != nullptr && ptlocid != nullptr && ptlocid != ptlocis) {
        *ptlocid = *ptlocis;
        ptlocid->refcount = 0;
        __acrt_add_locale_ref(ptlocid);
    }
}

/***
* void sync_legacy_variables_lk()
*
* Purpose:
*   Syncs all the legacy locale specific variables to the global locale.
*
*******************************************************************************/
static __inline void sync_legacy_variables_lk()
{
    __acrt_lconv = __acrt_current_locale_data.value()->lconv;
    _pctype = __acrt_current_locale_data.value()->_public._locale_pctype;
    __mb_cur_max = __acrt_current_locale_data.value()->_public._locale_mb_cur_max;
}

/***
* _locale_t _wcreate_locale(int category, char *wlocale) -
*    Set one or all locale categories of global locale structure
*
* Purpose:
*       The _wcreate_locale() routine allows the user to create a _locale_t
*       object that can be used with other locale functions.
*
* Entry:
*       int category = One of the locale categories defined in locale.h
*       char *locale = String identifying a specific locale.
*
* Exit:
*       If supplied locale pointer != nullptr:
*
*           If locale string is '\0', set locale to default.
*
*           If desired setting can be honored, return a pointer to the
*           locale string for the appropriate category.
*
*           If desired setting can NOT be honored, return nullptr.
*
* Exceptions:
*       Compound locale strings of the form "LC_COLLATE=xxx;LC_CTYPE=xxx;..."
*       are allowed for the LC_ALL category.  This is to support the ability
*       to restore locales with the returned string, as specified by ANSI.
*       Setting the locale with a compound locale string will succeed unless
*       *all* categories failed.  The returned string will reflect the current
*       locale.  For example, if LC_CTYPE fails in the above string, setlocale
*       will return "LC_COLLATE=xxx;LC_CTYPE=yyy;..." where yyy is the
*       previous locale (or the C locale if restoring the previous locale
*       also failed).  Unrecognized LC_* categories are ignored.
*
*******************************************************************************/

_locale_t __cdecl _wcreate_locale(
        int _category,
        const wchar_t *locale
        )
{
    if (_category < LC_MIN || _category > LC_MAX || locale == nullptr)
        return nullptr;

    auto result = _calloc_crt_t(__crt_locale_pointers, 1);
    _VALIDATE_RETURN_NOEXC(result, ENOMEM, nullptr);

    auto locale_data = _calloc_crt_t(__crt_locale_data, 1);
    _VALIDATE_RETURN_NOEXC(locale_data, ENOMEM, nullptr);

    auto multibyte_data = _calloc_crt_t(__crt_multibyte_data, 1);
    _VALIDATE_RETURN_NOEXC(multibyte_data, ENOMEM, nullptr);

    _copytlocinfo_nolock(locale_data.get(), &__acrt_initial_locale_data);

    if (_wsetlocale_nolock(locale_data.get(), _category, locale) == nullptr ||
        _setmbcp_nolock(locale_data.get()->_public._locale_lc_codepage, multibyte_data.get()) != 0)
    {
        __acrt_release_locale_ref(locale_data.get());
        __acrt_free_locale(locale_data.detach());
        return nullptr;
    }

    multibyte_data.get()->refcount = 1;

    result.get()->locinfo = locale_data.detach();
    result.get()->mbcinfo = multibyte_data.detach();
    return result.detach();
}

/***
* _locale_t _create_locale(int category, char *locale) -
*    Set one or all locale categories of global locale structure
****/
_locale_t __cdecl _create_locale(
        int _category,
        const char *_locale
        )
{
    wchar_t _wlocale[MAX_LC_LEN];

    /* Early input validation */
    if ( (_category < LC_MIN) || (_category > LC_MAX) || _locale == nullptr)
        return nullptr;

    if ( __acrt_MultiByteToWideChar(CP_ACP, 0, _locale, -1, _wlocale, _countof(_wlocale)) == 0 )
    { // conversion to wide char failed
        return nullptr;
    }

    return _wcreate_locale(_category, _wlocale);
}

/*
*char * setlocale(int category, char *locale) - Set one or all locale categories
*
*Purpose:
*       The setlocale() routine allows the user to set one or more of
*       the locale categories to the specific locale selected by the
*       user.  [ANSI]
*
*       NOTE: Under !_INTL, the C libraries only support the "C" locale.
*       Attempts to change the locale will fail.
*
*Entry:
*       int category = One of the locale categories defined in locale.h
*       char *locale = String identifying a specific locale or nullptr to
*                  query the current locale.
*
*Exit:
*       If supplied locale pointer == nullptr:
*
*           Return pointer to current locale string and do NOT change
*           the current locale.
*
*       If supplied locale pointer != nullptr:
*
*           If locale string is '\0', set locale to default.
*
*           If desired setting can be honored, return a pointer to the
*           locale string for the appropriate category.
*
*           If desired setting can NOT be honored, return nullptr.
*
*Exceptions:
*       Compound locale strings of the form "LC_COLLATE=xxx;LC_CTYPE=xxx;..."
*       are allowed for the LC_ALL category.  This is to support the ability
*       to restore locales with the returned string, as specified by ANSI.
*       Setting the locale with a compound locale string will succeed unless
*       *all* categories failed.  The returned string will reflect the current
*       locale.  For example, if LC_CTYPE fails in the above string, setlocale
*       will return "LC_COLLATE=xxx;LC_CTYPE=yyy;..." where yyy is the
*       previous locale (or the C locale if restoring the previous locale
*       also failed).  Unrecognized LC_* categories are ignored.
*
*/

wchar_t * __cdecl _wsetlocale (
        int _category,
        const wchar_t *_wlocale
        )
{
    wchar_t * retval=nullptr;
    __crt_locale_data* ptloci = nullptr;

    /* Validate category */
    _VALIDATE_RETURN(LC_MIN <= _category && _category <= LC_MAX, EINVAL, nullptr);

    __acrt_ptd* const ptd = __acrt_getptd();

    // Deadlock Avoidance:  When a new thread is created in the process, we
    // create a new PTD for the thread.  The PTD initialization function is
    // called under the loader lock.  This initialization function will also
    // acquire the locale lock in order to acquire a reference to the current
    // global locale for the new thread.
    //
    // Some of the locale APIs are not available on all supported target OSes.
    // We dynamically obtain these libraries via LoadLibrary/GetProcAddress.
    // We must ensure that no call to LoadLibrary is made while we hold the
    // locale lock, lest we deadlock due to lock order inversion between the
    // loader lock and the locale lock.
    //
    // This function call here will ensure that any required modules are loaded
    // before we acquire the locale lock.
    __acrt_eagerly_load_locale_apis();

    __acrt_update_thread_locale_data();

    // Prevent global locale resynchronization - we call things like stricmp() further down
    // without passing our _locale_t which would trigger a resynchronization.
    // Use _WSETLOCALE_AVOID_SYNC_LOCALE_BIT to avoid interfering with other locale settings
    // (see _configthreadlocale() for details).

    // This may not be necessary anymore and may be a hold-over from when _wsetlocale called
    // setlocale instead of the other way around.
    // (MSFT:20394962 - Investigate whether _WSETLOCALE_AVOID_SYNC_LOCALE_BIT is needed)
    __acrt_disable_global_locale_sync(ptd, _WSETLOCALE_AVOID_SYNC_LOCALE_BIT);

    __crt_call_and_cleanup([&]
    {
        if ((ptloci = _calloc_crt_t(__crt_locale_data, 1).detach()) != nullptr)
        {
            __acrt_lock_and_call(__acrt_locale_lock, [&]
            {
                _copytlocinfo_nolock(ptloci, ptd->_locale_info);

                if ((retval = _wsetlocale_nolock(ptloci, _category, _wlocale)) != 0)
                {
                    // If no call has been made to setlocale to change locale from "C" locale
                    // to some other locale, we keep locale_changed = 0. Other functions that
                    // depend on locale use this variable to optimize performance for C locale
                    // which is normally the case in applications.
                    if (_wlocale != nullptr)
                    {
                        if (wcscmp(_wlocale, __acrt_wide_c_locale_string) != 0)
                        {
                            __acrt_set_locale_changed();
                        }
                        // If it is again set to the C locale, could we not retain that state?
                        // else
                        // {
                        //    __acrt_set_locale_unchanged();
                        // }
                    }

                    (void)_updatetlocinfoEx_nolock(&ptd->_locale_info, ptloci);
                    __acrt_release_locale_ref(ptloci);
                    // Note that after incrementing _own_locale, if this thread doesn't
                    // have its own locale, _own_locale variable should be 1.

                    // Do not use __acrt_should_sync_with_global_locale() because
                    // _WSETLOCALE_AVOID_SYNC_LOCALE_BIT will interfere.
                    if (!(ptd->_own_locale & _PER_THREAD_LOCALE_BIT) &&
                        !(__globallocalestatus & _GLOBAL_LOCALE_BIT))
                    {
                        (void)_updatetlocinfoEx_nolock(&__acrt_current_locale_data.value(), ptd->_locale_info);
                        sync_legacy_variables_lk();
                    }
                }
                else
                {
                    __acrt_release_locale_ref(ptloci);
                    __acrt_free_locale(ptloci);
                }
            });
        }
    },
    [&]{ __acrt_enable_global_locale_sync(ptd, _WSETLOCALE_AVOID_SYNC_LOCALE_BIT); });

    return retval;
}

static wchar_t * __cdecl _wsetlocale_nolock(
        __crt_locale_data* ploci,
        int _category,
        const wchar_t *_wlocale
        )
{
    wchar_t * retval;
    /* Interpret locale */

    if (_category != LC_ALL)
    {
        retval = (_wlocale) ? _wsetlocale_set_cat(ploci, _category,_wlocale) :
            ploci->lc_category[_category].wlocale;

    } else { /* LC_ALL */
        wchar_t lctemp[MAX_LC_LEN];
        int i;
        int same = 1;
        int fLocaleSet = 0; /* flag to indicate if anything successfully set */

        if (_wlocale != nullptr)
        {
            if ( (_wlocale[0]==L'L') && (_wlocale[1]==L'C') && (_wlocale[2]==L'_') )
            {
                /* parse compound locale string */
                size_t len;
                const wchar_t * p = _wlocale;  /* start of string to parse */
                const wchar_t * s;

                do {
                    s = wcspbrk(p, L"=;");

                    if (s == nullptr || (len=(size_t)(s - p)) == 0 || (*s == L';'))
                        return nullptr;  /* syntax error */

                    /* match with known LC_ strings, if possible, else ignore */
                    for (i=LC_ALL+1; i<=LC_MAX; i++)
                    {
                        if ((!wcsncmp(__lc_category[i].catname,p,len))
                            && (len==wcslen(__lc_category[i].catname)))
                        {
                            break;  /* matched i */
                        }
                    } /* no match if (i>LC_MAX) -- just ignore */

                    if ((len = wcscspn(++s, L";")) == 0 && *s != L';')
                        return nullptr;  /* syntax error */

                    if (i<=LC_MAX)
                    {
                        _ERRCHECK(wcsncpy_s(lctemp, _countof(lctemp), s, len));
                        lctemp[len]=L'\0';   /* null terminate string */

                        /* don't fail unless all categories fail */
                        if (_wsetlocale_set_cat(ploci, i,lctemp))
                            fLocaleSet++;       /* record a success */
                    }
                    if (*(p = s+len)!=L'\0')
                        p++;  /* skip ';', if present */

                } while (*p);

                retval = (fLocaleSet) ? _wsetlocale_get_all(ploci) : nullptr;

            } else { /* simple LC_ALL locale string */

                UINT code_page;
                wchar_t localeNameTemp[LOCALE_NAME_MAX_LENGTH];
                /* confirm locale is supported, get expanded locale */
                retval = _expandlocale(_wlocale, lctemp, _countof(lctemp), localeNameTemp, _countof(localeNameTemp), code_page);
                if (retval != 0)
                {
                    for (i=LC_MIN; i<=LC_MAX; i++)
                    {
                        if (i!=LC_ALL)
                        {
                            if (wcscmp(lctemp, ploci->lc_category[i].wlocale) != 0)
                            { // does not match the LC_ALL locale
                                if (_wsetlocale_set_cat(ploci, i, lctemp))
                                {
                                    fLocaleSet++;   /* record a success */
                                }
                                else
                                {
                                    same = 0;       /* record a failure */
                                }
                            }
                            else
                                fLocaleSet++;   /* trivial succcess */
                        }
                    }
                    if (same) /* needn't call setlocale_get_all() if all the same */
                    {
                        retval = _wsetlocale_get_all(ploci);
                        /* retval set above */
                    }
                    else
                        retval = (fLocaleSet) ? _wsetlocale_get_all(ploci) : nullptr;
                }
            }
        } else { /* LC_ALL & nullptr */
            retval = _wsetlocale_get_all (ploci);
        }
    }

    /* common exit point */
    return retval;
} /* setlocale */


static wchar_t * __cdecl _wsetlocale_set_cat (
        __crt_locale_data* ploci,
        int category,
        const wchar_t * wlocale
        )
{
    wchar_t * oldlocale;
    wchar_t * oldlocalename;
    UINT oldcodepage;

    UINT cptemp;
    wchar_t lctemp[MAX_LC_LEN];
    wchar_t localeNameString[LOCALE_NAME_MAX_LENGTH];
    wchar_t * pch = nullptr;
    wchar_t * pch_cat_locale = nullptr;
    size_t cch = 0;
    unsigned short out[sizeof(_first_127char)];
    int i;
    __acrt_ptd* const ptd = __acrt_getptd();
    __crt_ctype_compatibility_data* _Loc_c = ptd->_setloc_data._Loc_c; // __setloc_data._Loc_c is array
    int _LOC_CCACHE = _countof(ptd->_setloc_data._Loc_c);
    __crt_ctype_compatibility_data buf1, buf2;

    if (!_expandlocale(wlocale, lctemp, _countof(lctemp), localeNameString, _countof(localeNameString), cptemp))
        return nullptr;

    // if this category's locale hadn't changed
    if (wcscmp(lctemp, ploci->lc_category[category].wlocale) == 0)
    {
        return ploci->lc_category[category].wlocale;
    }

    cch = wcslen(lctemp) + 1;
    if ((pch = static_cast<wchar_t*>(_malloc_crt(sizeof(int) + (cch * sizeof(wchar_t))))) == nullptr)
        return nullptr;  /* error if malloc fails */

    pch_cat_locale = pch + (sizeof(int) / sizeof(wchar_t));

     /* save for possible restore */
    oldlocale = ploci->lc_category[category].wlocale;
    oldlocalename = ploci->locale_name[category];
    oldcodepage = ploci->_public._locale_lc_codepage;

    /* update locale string */
    _ERRCHECK(wcscpy_s(pch_cat_locale, cch, lctemp));
    ploci->lc_category[category].wlocale = pch_cat_locale;

    /* Copy locale name */
    if (lctemp[0] == L'C' && lctemp[1] == L'\x0') // if "C" locale
        ploci->locale_name[category] = nullptr;
    else
        ploci->locale_name[category] = __acrt_copy_locale_name(localeNameString);

    /* To speedup locale based comparisions, we identify if the current
     * local has first 127 character set same as CLOCALE. If yes then
     * ploci->lc_clike = TRUE.
     */

    if (category==LC_CTYPE)
    {
        ploci->_public._locale_lc_codepage = cptemp;
        buf1 = _Loc_c[_LOC_CCACHE -1];
        /* brings the recently used codepage to the top. or else shifts
         * every thing down by one so that new _Loc_c can be placed at
         * the top.
         */
        for ( i = 0; i < _LOC_CCACHE; i++)
        {
            if (ploci->_public._locale_lc_codepage == _Loc_c[i].id)
            {
                /* We don't really want to swap cache around in case what we are looking
                 *  for is the first element of the cache
                 */
                if (i!=0)
                {
                    _Loc_c[0] = _Loc_c[i];
                    _Loc_c[i] = buf1;
                }
                break;
            }
            else
            {
                buf2 = _Loc_c[i];
                _Loc_c[i] = buf1;
                buf1 = buf2;
            }
        }
        if ( i == _LOC_CCACHE)
        {
            if ( __acrt_GetStringTypeA(nullptr, CT_CTYPE1,
                                       _first_127char,
                                       sizeof(_first_127char),
                                       out,
                                       ploci->_public._locale_lc_codepage,
                                       TRUE ))
            {
                int j;
                for ( j = 0; j < sizeof(_first_127char); j++)
                    out[j] = out[j]&
                            (_UPPER|_LOWER|_DIGIT|_SPACE|_PUNCT|_CONTROL|_BLANK|_HEX|_ALPHA);
                if ( !memcmp(out, _ctype_loc_style, (sizeof(_first_127char)/sizeof(char))*sizeof(short)))
                {
                    _Loc_c[0].is_clike = TRUE;
                }
                else
                {
                    _Loc_c[0].is_clike = FALSE;
                }
            }
            else
                _Loc_c[0].is_clike = FALSE;
            _Loc_c[0].id = ploci->_public._locale_lc_codepage;
        }
        ploci->lc_clike = _Loc_c[0].is_clike;
    } /* category==LC_CTYPE */
    else if ( category == LC_COLLATE )
        ploci->lc_collate_cp = cptemp;
    else if ( category == LC_TIME )
        ploci->lc_time_cp = cptemp;

    if (__lc_category[category].init(ploci) != 0)
    {
        /* restore previous state */
        ploci->lc_category[category].wlocale = oldlocale;
        _free_crt(ploci->locale_name[category]);
        ploci->locale_name[category] = oldlocalename;
        _free_crt(pch);
        ploci->_public._locale_lc_codepage = oldcodepage;

        return nullptr; /* error if non-zero return */
    }

    /* locale set up successfully */
    /* Cleanup */
    if ((oldlocale != __acrt_wide_c_locale_string) &&
        (InterlockedDecrement(ploci->lc_category[category].wrefcount) == 0)
        )
    {
        _ASSERT(0);
        _free_crt(ploci->lc_category[category].wrefcount);
        _free_crt(ploci->lc_category[category].refcount);
        _free_crt(ploci->locale_name[category]);
        ploci->lc_category[category].wlocale = nullptr;
        ploci->locale_name[category] = nullptr;
    }
    if (pch)
    {
        reinterpret_cast<long&>(*pch) = 1;
    }
    ploci->lc_category[category].wrefcount = reinterpret_cast<long*>(pch);

    return ploci->lc_category[category].wlocale;
} /* _wsetlocale_set_cat */



static wchar_t * __cdecl _wsetlocale_get_all ( __crt_locale_data* ploci)
{
    int i;
    int same = 1;
    wchar_t *pch = nullptr;
    size_t cch = 0;
    long *refcount = nullptr;
    size_t refcountSize = 0;

    /* allocate memory if necessary */
    refcountSize = sizeof(int) + (sizeof(wchar_t) * (MAX_LC_LEN+1) * (LC_MAX-LC_MIN+1)) + (sizeof(wchar_t) * CATNAMES_LEN);
    if ( (refcount = static_cast<long*>(_malloc_crt(refcountSize))) == nullptr)
        return nullptr;

    pch = ((wchar_t*)refcount) + (sizeof(int) / sizeof(wchar_t));
    cch = (refcountSize - sizeof(int)) / sizeof(wchar_t);
    *pch = L'\0';
    *refcount = 1;
    for (i=LC_MIN+1; ; i++)
    {
        _wcscats(pch, cch, 3, __lc_category[i].catname, L"=", ploci->lc_category[i].wlocale);
        if (i<LC_MAX)
        {
            _ERRCHECK(wcscat_s(pch, cch, L";"));
            if (wcscmp(ploci->lc_category[i].wlocale, ploci->lc_category[i+1].wlocale))
                same=0;
        }
        else
        {
            if (!same) {
                if (ploci->lc_category[LC_ALL].wrefcount != nullptr &&
                    InterlockedDecrement(ploci->lc_category[LC_ALL].wrefcount) == 0) {
                    _ASSERT(0);
                    _free_crt(ploci->lc_category[LC_ALL].wrefcount);
                }
                if (ploci->lc_category[LC_ALL].refcount != nullptr &&
                    InterlockedDecrement(ploci->lc_category[LC_ALL].refcount) == 0) {
                    _ASSERT(0);
                    _free_crt(ploci->lc_category[LC_ALL].refcount);
                }
                ploci->lc_category[LC_ALL].refcount = nullptr;
                ploci->lc_category[LC_ALL].locale = nullptr;
                ploci->lc_category[LC_ALL].wrefcount = refcount;
                return ploci->lc_category[LC_ALL].wlocale = pch;
            } else {
                _free_crt(refcount);
                if (ploci->lc_category[LC_ALL].wrefcount != nullptr &&
                    InterlockedDecrement(ploci->lc_category[LC_ALL].wrefcount) == 0) {
                    _ASSERT(0);
                    _free_crt(ploci->lc_category[LC_ALL].wrefcount);
                }
                if (ploci->lc_category[LC_ALL].refcount != nullptr &&
                    InterlockedDecrement(ploci->lc_category[LC_ALL].refcount) == 0) {
                    _ASSERT(0);
                    _free_crt(ploci->lc_category[LC_ALL].refcount);
                }
                ploci->lc_category[LC_ALL].refcount = nullptr;
                ploci->lc_category[LC_ALL].locale = nullptr;
                ploci->lc_category[LC_ALL].wrefcount = nullptr;
                ploci->lc_category[LC_ALL].wlocale = nullptr;
                return ploci->lc_category[LC_CTYPE].wlocale;
            }
        }
    }
} /* _wsetlocale_get_all */

// --- BCP-47 tag parsing for _expandlocale() ---
//
// Provides parse_bcp47() which parses a given locale expression and updates a __crt_locale_strings structure accordingly.
// Unsupported:
// * Extension and private tags
// * 4-8 character language subtags
// * Grandfathered tags
// * Lone code page specifiers (POSIX extension for BCP-47)

enum class _bcp47_section_delimiter
{
    normal,
    end_of_string,
    code_page
};

struct _bcp47_section
{
    wchar_t const *          ptr;
    size_t                   length;
    _bcp47_section_delimiter delimiter;
};

static _bcp47_section_delimiter categorize_delimiter(wchar_t const wc)
{
    switch (wc)
    {
        case '-':
        case '_':
            return _bcp47_section_delimiter::normal;
        case '.':
            return _bcp47_section_delimiter::code_page;
        case '\0':
        default:
            return _bcp47_section_delimiter::end_of_string;
    }
}

static bool string_is_alpha(wchar_t const * const str, size_t const len)
{
    for (size_t i = 0; i < len; ++i)
    {
        if (!__ascii_isalpha(str[i]))
        {
            return false;
        }
    }
    return true;
}

static bool string_is_digit(wchar_t const * const str, size_t const len)
{
    for (size_t i = 0; i < len; ++i)
    {
        if (!__ascii_isdigit(str[i]))
        {
            return false;
        }
    }
    return true;
}

static bool parse_bcp47_language(__crt_locale_strings * const names, _bcp47_section const& section)
{
    if (section.delimiter != _bcp47_section_delimiter::normal)
    {
        return false;
    }

    if (section.length < 2 || section.length > 3)
    {
        return false; // Failure, only 2-3 letter language codes permitted.
    }

    if (!string_is_alpha(section.ptr, section.length))
    {
        return false;
    }

    _ERRCHECK(wcsncpy_s(names->szLanguage, _countof(names->szLanguage), section.ptr, section.length));
    _ERRCHECK(wcsncpy_s(names->szLocaleName, _countof(names->szLocaleName), section.ptr, section.length));
    return true;
}

static bool parse_bcp47_script(__crt_locale_strings * const names, _bcp47_section const& section)
{
    if (section.delimiter != _bcp47_section_delimiter::normal)
    {
        return false;
    }

    if (section.length != 4) {
        return false; // Failure, only 4 letter script codes permitted.
    }

    if (!string_is_alpha(section.ptr, section.length))
    {
        return false;
    }

    _ERRCHECK(wcsncat_s(names->szLocaleName, _countof(names->szLocaleName), L"-", 1));
    _ERRCHECK(wcsncat_s(names->szLocaleName, _countof(names->szLocaleName), section.ptr, section.length));
    return true;
}

static bool parse_bcp47_region(__crt_locale_strings * const names, _bcp47_section const& section)
{
    if (section.delimiter != _bcp47_section_delimiter::normal)
    {
        return false;
    }

    if (   !(section.length == 2 && string_is_alpha(section.ptr, section.length))  // if not 2 letters
        && !(section.length == 3 && string_is_digit(section.ptr, section.length))) // and not 3 digits
    {
        return false;
    }

    _ERRCHECK(wcsncpy_s(names->szCountry, _countof(names->szCountry), section.ptr, section.length));
    _ERRCHECK(wcsncat_s(names->szLocaleName, _countof(names->szLocaleName), L"-", 1));
    _ERRCHECK(wcsncat_s(names->szLocaleName, _countof(names->szLocaleName), section.ptr, section.length));
    return true;
}

static bool parse_bcp47_code_page(__crt_locale_strings * const names, _bcp47_section const& section)
{
    if (section.delimiter != _bcp47_section_delimiter::code_page)
    {
        return false;
    }

    _ERRCHECK(wcsncpy_s(names->szCodePage, _countof(names->szCodePage), section.ptr, section.length));
    return true; // Success
}

static bool parse_bcp47(__crt_locale_strings * const names, const wchar_t * const expr)
{
    wchar_t const * p = expr;
    wchar_t const * const delimiters = L"-_.";

    memset(names, 0, sizeof(__crt_locale_strings));

    size_t const max_sections = 4;
    _bcp47_section sections[max_sections];
    size_t num_sections = 0;

    for (auto last_delimiter = _bcp47_section_delimiter::normal;
         last_delimiter != _bcp47_section_delimiter::end_of_string;
         last_delimiter = categorize_delimiter(*p++)
        )
    {
        if (num_sections >= max_sections)
        {
            // Didn't reach end of string before running out of sections to parse.
            return false;
        }

        size_t const section_length = (last_delimiter != _bcp47_section_delimiter::code_page)
            ? wcscspn(p, delimiters) : wcslen(p);

        sections[num_sections].ptr = p;
        sections[num_sections].length = section_length;
        sections[num_sections].delimiter = last_delimiter;

        p += section_length;
        ++num_sections;
    }

    switch (num_sections)
    {
        case 1:
            // en
            return parse_bcp47_language(names, sections[0]);
        case 2:
            // en-US
            // zh-Hans
            // en.utf-8
            // .utf-8
            return parse_bcp47_language(names, sections[0])
                   && (   parse_bcp47_script(names, sections[1])
                       || parse_bcp47_region(names, sections[1])
                       || parse_bcp47_code_page(names, sections[1])
                      );
        case 3:
            // en-US.utf-8
            // zh-Hans-HK
            return parse_bcp47_language(names, sections[0])
                   && (    parse_bcp47_script(names, sections[1]) && (parse_bcp47_region(names, sections[2]) || parse_bcp47_code_page(names, sections[2]))
                       || (parse_bcp47_region(names, sections[1]) &&  parse_bcp47_code_page(names, sections[2]))
                      );
        case 4:
            // zh-Hans-HK.utf-8
            return parse_bcp47_language(names, sections[0])
                && parse_bcp47_script(names, sections[1])
                && parse_bcp47_region(names, sections[2])
                && parse_bcp47_code_page(names, sections[3]);
        default:
            return false;
    }
}

// Utility functions/structs for _expandlocale().

static int get_default_code_page(wchar_t const * const valid_windows_locale_name)
{
    int code_page{};

    int const size_written = __acrt_GetLocaleInfoEx(
        valid_windows_locale_name,
        LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER,
        reinterpret_cast<LPWSTR>(&code_page),
        sizeof(int) / sizeof(wchar_t)
        );

    if (size_written == 0 || code_page == 0)
    {   // Default to UTF-8 if could not find or no default code page.
        return CP_UTF8;
    }

    return code_page;
}

class _expandlocale_locale_name_cache
{
    // Scope guard to keep invariants in _expandlocale for the localeNameOutput and _cacheLocaleName variables.
    // The cacheLocaleName/localeNameOutput is the only cache where work is done progressively in the cache value,
    // (inside __acrt_get_qualified_locale and __acrt_get_qualified_locale_downlevel)
    // so we must save/restore at function start and invalidate the cache once we have stuff to commit to both.
public:
    _expandlocale_locale_name_cache(
        wchar_t *                     const localeNameOutput,
        size_t                        const localeNameSizeInChars,
        __crt_qualified_locale_data * const psetloc_data
    ) : _localeNameOutput(localeNameOutput),
        _localeNameSizeInChars(localeNameSizeInChars),
        _psetloc_data(psetloc_data),
        _committed(false)
    {
        _ERRCHECK(wcsncpy_s(localeNameOutput, localeNameSizeInChars, psetloc_data->_cacheLocaleName, _countof(psetloc_data->_cacheLocaleName)));
    }

    ~_expandlocale_locale_name_cache()
    {
        if (!_committed)
        {
            _ERRCHECK(wcsncpy_s(_psetloc_data->_cacheLocaleName, _countof(_psetloc_data->_cacheLocaleName), _localeNameOutput, _localeNameSizeInChars));
        }
    }

    _expandlocale_locale_name_cache(_expandlocale_locale_name_cache const&) = delete;
    _expandlocale_locale_name_cache& operator=(_expandlocale_locale_name_cache const&) = delete;

    void commit_locale_name(wchar_t const * const new_locale_name, size_t const new_locale_name_length)
    {
        _ERRCHECK(wcsncpy_s(_psetloc_data->_cacheLocaleName, _countof(_psetloc_data->_cacheLocaleName), new_locale_name, new_locale_name_length));
        commit_locale_name_cache_already_updated(new_locale_name, new_locale_name_length);
    }

    void commit_locale_name_cache_already_updated(wchar_t const * const new_locale_name, size_t const new_locale_name_length)
    {
        _ERRCHECK(wcsncpy_s(_localeNameOutput, _localeNameSizeInChars, new_locale_name, new_locale_name_length));
        _committed = true;
    }

private:
    wchar_t *                     _localeNameOutput;
    size_t                        _localeNameSizeInChars;
    __crt_qualified_locale_data * _psetloc_data;
    bool                          _committed;
};

wchar_t * _expandlocale(
    wchar_t const * const expr,
    wchar_t *       const output,
    size_t          const sizeInChars,
    wchar_t *       const localeNameOutput,
    size_t          const localeNameSizeInChars,
    UINT&                 output_code_page
    )
{
    // Returns: locale name to return to user (lifetime bound to PTD variable _cacheout)
    // Out Parameters:
    //  * output: locale name to return to user (lifetime bound to supplied buffer)
    //  * localeNameOutput: normalized locale name to be used internally (lifetime bound to supplied buffer)
    //  * output_code_page: code page used
    //
    // Effects:
    //  Parses an input locale string and returns the string that will replicate the effects just taken (output), the string to be used
    //  for Win32 calls (localeNameOutput), and the code page.
    //  Note that there are three modes here:
    //  * Legacy and Windows Locale Names:
    //      The output locale string and internally used locale string become normalized for Win32 APIs.
    //      Neither necessarily match the user input locale.
    //      Uses ACP as default if Windows does not know the code page.
    //  * Permissive Windows locale names:
    //      Ask Windows whether full given user string is a known locale name.
    //      In that case, the output locale string, internally used locale string, and user input locale all match.
    //      Uses UTF-8 as default if Windows does not know the code page.
    //  * BCP-47 Locale Names:
    //      Windows doesn't recognize BCP-47 + code page specifiers, but we must support them for proper UTF-8 support.
    //      The output locale name must include the code page, but internally we modify the format so Windows understands it.
    //      Uses UTF-8 as default if Windows does not know the code page.
    //
    //  Also populates _psetloc_data structure from the PTD:
    //  Caching Internals:
    //    * _cachein: Caches last successful 'expr'.
    //    * _cacheout: Caches last successful 'output' (which is also the return value).
    //    * _cachecp: Caches last successful 'output_code_page'.
    //    * _cacheLocaleName: Caches last successful 'localeNameOutput'.
    //  Legacy Internals (used by __acrt_get_qualified_locale and __acrt_get_qualified_locale_downlevel only):
    //    * pchLanguage: Pointer to start of language name, ex: "English"
    //    * pchCountry: Pointer to start of country/region name, ex: "United States"
    //    * iLocState: Used to record the match degree for locales checked (i.e. language and region match vs just language match)
    //    * bAbbrevLanguage: Whether language name is a three-letter short-form, ex: "ENU"
    //    * bAbbrevCountry: Whether country/region name is a three letter short-form, ex: "USA"
    //    * iPrimaryLen: Length of pchLanguage portion of locale string
    //  Also in _psetloc_data, but not used in _expandlocale: _Loc_c

    if (!expr)
    {
        return nullptr; // error if no input
    }

    // C locale
    // Callers know that LocaleNameOutput has not been updated and check for "C" locale before using it.
    if (expr[0] == L'C' && expr[1] == L'\0')
    {
        _ERRCHECK(wcscpy_s(output, sizeInChars, L"C"));
        output_code_page = CP_ACP;
        return output;
    }

    __crt_qualified_locale_data * const _psetloc_data    = &__acrt_getptd()->_setloc_data;
    UINT *                        const pcachecp         = &_psetloc_data->_cachecp;
    wchar_t *                     const cachein          = _psetloc_data->_cachein;
    size_t                        const cacheinLen       = _countof(_psetloc_data->_cachein);
    wchar_t *                     const cacheout         = _psetloc_data->_cacheout;
    size_t                        const cacheoutLen      = _countof(_psetloc_data->_cacheout);
    size_t                              charactersInExpression = 0;
    int                                 iCodePage = 0;

    // This guard owns access to localeNameOutput. It expresses the invariants that localeNameOutput and _cacheLocaleName have.
    _expandlocale_locale_name_cache locale_name_guard(localeNameOutput, localeNameSizeInChars, _psetloc_data);

    // First, make sure we didn't just do this one
    charactersInExpression = wcslen(expr);
    if (charactersInExpression >= MAX_LC_LEN ||       // we would never have cached this
        (wcscmp(cacheout, expr) && wcscmp(cachein, expr)))
    {
        __crt_locale_strings names;
        BOOL getqloc_results = FALSE;
        BOOL const isDownlevel = !__acrt_can_use_vista_locale_apis();

        // Check if can parse as Legacy/Windows locale name
        if (__lc_wcstolc(&names, expr) == 0)
        {
            if (isDownlevel)
            {
                getqloc_results = __acrt_get_qualified_locale_downlevel(&names, pcachecp, &names);
            }
            else
            {
                getqloc_results = __acrt_get_qualified_locale(&names, pcachecp, &names);
            }
        }

        // If successfully parsed as Legacy/Windows name
        if (getqloc_results)
        {
            // Legacy/Windows name resolution returns applies normalization to both returned and internally used locale names.
            __lc_lctowcs(cacheout, cacheoutLen, &names);

            // __acrt_get_qualified_locale and __acrt_get_qualified_locale_downlevel update _cacheLocaleName - if successful commit locale changes
            locale_name_guard.commit_locale_name_cache_already_updated(names.szLocaleName, wcslen(names.szLocaleName) + 1);
        }
        else if (__acrt_IsValidLocaleName(expr))
        {
            // Windows recognizes the locale expression - only work is to grab the code page to be used.
            iCodePage = get_default_code_page(expr);

            // Commit code page
            *pcachecp = static_cast<WORD>(iCodePage);

            // Commit locale name that we will return to the user (same as input locale string).
            _ERRCHECK(wcsncpy_s(cacheout, cacheoutLen, expr, charactersInExpression + 1));

            // Commit locale name for internal use (same as input locale string).
            locale_name_guard.commit_locale_name(expr, charactersInExpression + 1);
        }
        else if (parse_bcp47(&names, expr) && __acrt_IsValidLocaleName(names.szLocaleName))
        {
            // Parsed as Windows-recognized BCP-47 tag
            wchar_t const * const normalized_locale_name = names.szLocaleName;

            if (names.szCodePage[0])
            {
                wchar_t const * const cp = names.szCodePage;

                // Allow .utf8/.utf-8 for BCP-47 tags.
                if (   __ascii_towlower(cp[0]) == L'u'
                    && __ascii_towlower(cp[1]) == L't'
                    && __ascii_towlower(cp[2]) == L'f'
                    &&     (cp[3] == L'8' && cp[4] == L'\0')
                        || (cp[3] == L'-' && cp[4] == L'8' && cp[5] == L'\0'))
                {
                    iCodePage = CP_UTF8;
                }
                else
                {
                    // Other codepage tokens (fr-FR.1252, etc.) aren't supported for BCP-47 tags (eg: Use Unicode!)
                    return nullptr;
                }
            }
            else
            {
                iCodePage = get_default_code_page(normalized_locale_name);
            }

            // Commit code page
            *pcachecp = static_cast<WORD>(iCodePage);

            // Commit locale name that we will return to the user (same as input locale string).
            _ERRCHECK(wcsncpy_s(cacheout, cacheoutLen, expr, charactersInExpression + 1));

            // Commit normalized name for internal use.
            locale_name_guard.commit_locale_name(normalized_locale_name, wcslen(normalized_locale_name) + 1);
        }
        else
        {
            return nullptr;  // input unrecognized as locale name
        }

        // Operation succeeded - commit input cache.
        if (*expr && charactersInExpression < MAX_LC_LEN)
        {
            _ERRCHECK(wcsncpy_s(cachein, cacheinLen, expr, charactersInExpression + 1));
        }
        else
        {
            *cachein = L'\x0';
        }
    }

    output_code_page = *pcachecp; // Update code page

    _ERRCHECK(wcscpy_s(output, sizeInChars, cacheout));
    return cacheout; // Return locale name to be given back to user, with lifetime bound in PTD
}

/* helpers */

void _wcscats ( wchar_t *outstr, size_t numberOfElements, int n, ...)
{
    int i;
    va_list substr;

    va_start (substr, n);

    for (i =0; i<n; i++)
    {
        _ERRCHECK(wcscat_s(outstr, numberOfElements, va_arg(substr, wchar_t *)));
    }
    va_end(substr);
}

// Parse the wlocale string to find the array of language/region/codepage strings (if available)
// in the form .CodePage, Language, Language_Region, Language_Region.Codepage, or Language.CodePage
// If the input is a BCP-47 tag, then that tag is returned in the names language.
int __lc_wcstolc ( __crt_locale_strings *names, const wchar_t *wlocale)
{
    int i;
    size_t len;
    wchar_t wch;

    memset((void *)names, '\0', sizeof(__crt_locale_strings));  /* clear out result */

    if (*wlocale==L'\0')
        return 0; /* trivial case */

    /* only code page is given */
    if (wlocale[0] == L'.' && wlocale[1] != L'\0')
    {
        _ERRCHECK(wcsncpy_s(names->szCodePage, _countof(names->szCodePage), &wlocale[1], MAX_CP_LEN-1));
        /* Make sure to null terminate the string in case wlocale is > MAX_CP_LEN */
        names->szCodePage[ MAX_CP_LEN -1] = 0;
        return 0;
    }

    // Looks like Language_Country/Region.Codepage
    for (i=0; ; i++)
    {
        // _ language country/region separator, . is before codepage, either , or \0 are end of string.
        len = wcscspn(wlocale, L"_.,");
        if (len == 0)
            return -1;  /* syntax error, can't start with a separator */

        wch = wlocale[len];

        if ((i == 0) && (len < MAX_LANG_LEN))
        {
            _ERRCHECK(wcsncpy_s(names->szLanguage, _countof(names->szLanguage), wlocale, len));
            if (wch == L'.')
            {
                // '.' is a delimiter before codepage, so codepage is expected next (skip country/region)
                i++;
            }
        }

        else if ((i==1) && (len<MAX_CTRY_LEN) && (wch!=L'_'))
            _ERRCHECK(wcsncpy_s(names->szCountry, _countof(names->szCountry), wlocale, len));

        else if ((i==2) && (len<MAX_CP_LEN) && (wch==L'\0' || wch==L','))
            _ERRCHECK(wcsncpy_s(names->szCodePage, _countof(names->szCodePage), wlocale, len));

        else
            return -1;  /* error parsing wlocale string */

        if (wch==L',')
        {
            /* modifier not used in current implementation, but it
               must be parsed to for POSIX/XOpen conformance */
            /*  wcsncpy(names->szModifier, wlocale, MAX_MODIFIER_LEN-1); */
            break;
        }

        if (!wch)
            break;
        wlocale+=(len+1);
    }
    return 0;
}
// Append locale name pieces together in the form of "Language_country/region.CodePage"
// Note that the preferred form is the BCP-47 tag, but we don't want to change legacy names
void __lc_lctowcs ( wchar_t *locale, size_t numberOfElements, const __crt_locale_strings *names)
{
    _ERRCHECK(wcscpy_s(locale, numberOfElements, names->szLanguage));
    if (*(names->szCountry))
        _wcscats(locale, numberOfElements, 2, L"_", names->szCountry);
    if (*(names->szCodePage))
        _wcscats(locale, numberOfElements, 2, L".", names->szCodePage);
}

// Create a copy of a locale name string
LPWSTR __cdecl __acrt_copy_locale_name(LPCWSTR localeName)
{
    size_t cch;
    wchar_t* localeNameCopy;

    if (!localeName) // Input cannot be nullptr
        return nullptr;

    cch = wcsnlen(localeName, LOCALE_NAME_MAX_LENGTH);
    if (cch >= LOCALE_NAME_MAX_LENGTH) // locale name length must be <= LOCALE_NAME_MAX_LENGTH
        return nullptr;

    if ((localeNameCopy = (wchar_t *) _malloc_crt((cch + 1) * sizeof(wchar_t))) == nullptr)
        return nullptr;

    _ERRCHECK(wcsncpy_s(localeNameCopy, cch+1, localeName, cch+1));
    return localeNameCopy;
}

#endif /* _UCRT_ENCLAVE_BUILD */

} // extern "C"
