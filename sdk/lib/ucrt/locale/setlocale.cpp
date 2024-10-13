/***
*setlocal.c - Contains the setlocale function
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Contains the _wsetlocale() function.
*
*******************************************************************************/

#include <corecrt_internal.h>
#include <locale.h>
#include <stdlib.h>

static wchar_t* __cdecl call_wsetlocale(int const category, char const* const narrow_locale)
{
    if (narrow_locale == nullptr)
        return _wsetlocale(category, nullptr);

    size_t size;
    _ERRCHECK_EINVAL_ERANGE(mbstowcs_s(&size, nullptr, 0, narrow_locale, INT_MAX));

    __crt_unique_heap_ptr<wchar_t> wide_locale(_calloc_crt_t(wchar_t, size));
    if (wide_locale.get() == nullptr)
        return nullptr;

    if (_ERRCHECK_EINVAL_ERANGE(mbstowcs_s(nullptr, wide_locale.get(), size, narrow_locale, _TRUNCATE)) != 0)
        return nullptr;

    return _wsetlocale(category, wide_locale.get());
}

extern "C" char* __cdecl setlocale(int _category, char const* _locale)
{
    // Deadlock Avoidance:  See the comment in _wsetlocale on usage of this function.
    __acrt_eagerly_load_locale_apis();

    return __acrt_lock_and_call(__acrt_locale_lock, [&]() -> char*
    {
        // Convert ASCII string into a wide character string:
        wchar_t* const outwlocale = call_wsetlocale(_category, _locale);
        if (outwlocale == nullptr)
        {
            return nullptr;
        }

        __acrt_ptd* const ptd = __acrt_getptd();

        __crt_locale_pointers locale;
        locale.locinfo = ptd->_locale_info;
        locale.mbcinfo = ptd->_multibyte_info;

        // We now have a locale string, but the global locale can be changed by
        // another thread.  If we allow this thread's locale to be updated before
        // we're done with this string, it might be freed from under us.  We call
        // the versions of the wide-to-multibyte conversions that do not update the
        // current thread's locale.
        size_t size = 0;
        if (_ERRCHECK_EINVAL_ERANGE(_wcstombs_s_l(&size, nullptr, 0, outwlocale, 0, &locale)) != 0)
        {
            return nullptr;
        }

        long* const refcount = static_cast<long*>(_malloc_crt(size + sizeof(long)));
        if (refcount == nullptr)
        {
            return nullptr;
        }

        char* outlocale = reinterpret_cast<char*>(&refcount[1]);

        /* convert return value to ASCII */
        if (_ERRCHECK_EINVAL_ERANGE(_wcstombs_s_l(nullptr, outlocale, size, outwlocale, _TRUNCATE, &locale)) != 0)
        {
            _free_crt(refcount);
            return nullptr;
        }

        __crt_locale_data* ptloci = locale.locinfo;

        _ASSERTE((ptloci->lc_category[_category].locale != nullptr && ptloci->lc_category[_category].refcount != nullptr) ||
                 (ptloci->lc_category[_category].locale == nullptr && ptloci->lc_category[_category].refcount == nullptr));

        if (ptloci->lc_category[_category].refcount != nullptr &&
            _InterlockedDecrement(ptloci->lc_category[_category].refcount) == 0)
        {
            _free_crt(ptloci->lc_category[_category].refcount);
            ptloci->lc_category[_category].refcount = nullptr;
        }

        if (__acrt_should_sync_with_global_locale(ptd))
        {
            if (ptloci->lc_category[_category].refcount != nullptr &&
                _InterlockedDecrement(ptloci->lc_category[_category].refcount) == 0)
            {
                _free_crt(ptloci->lc_category[_category].refcount);
                ptloci->lc_category[_category].refcount = nullptr;
            }
        }

        /*
        * Note that we are using a risky trick here.  We are adding this
        * locale to an existing __crt_locale_data struct, and thus starting
        * the locale's refcount with the same value as the whole struct.
        * That means all code which modifies both __crt_locale_data::refcount
        * and __crt_locale_data::lc_category[]::refcount in structs that are
        * potentially shared across threads must make those modifications
        * under the locale lock.  Otherwise, there's a race condition
        * for some other thread modifying __crt_locale_data::refcount after
        * we load it but before we store it to refcount.
        */
        *refcount = ptloci->refcount;
        ptloci->lc_category[_category].refcount = refcount;
        ptloci->lc_category[_category].locale = outlocale;

        return outlocale;
    });
}
