/***
*localref.c - Contains the __[add|remove]localeref() functions.
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Contains the __[add|remove]localeref() functions.
*
*******************************************************************************/
#include <corecrt_internal.h>
#include <locale.h>



/***
* __acrt_add_locale_ref(__crt_locale_data* ptloci)
*
* Purpose:
*       Increment the refrence count for each element in the localeinfo struct.
*
*******************************************************************************/
extern "C" void __cdecl __acrt_add_locale_ref(__crt_locale_data* ptloci)
{
    _InterlockedIncrement(&ptloci->refcount);

    if (ptloci->lconv_intl_refcount != nullptr)
    {
        _InterlockedIncrement(ptloci->lconv_intl_refcount);
    }

    if (ptloci->lconv_mon_refcount != nullptr)
    {
        _InterlockedIncrement(ptloci->lconv_mon_refcount);
    }

    if (ptloci->lconv_num_refcount != nullptr)
    {
        _InterlockedIncrement(ptloci->lconv_num_refcount);
    }

    if (ptloci->ctype1_refcount != nullptr)
    {
        _InterlockedIncrement(ptloci->ctype1_refcount);
    }

    for (int category = LC_MIN; category <= LC_MAX; ++category)
    {
        if (ptloci->lc_category[category].wlocale != __acrt_wide_c_locale_string &&
            ptloci->lc_category[category].wrefcount != nullptr)
        {
            _InterlockedIncrement(ptloci->lc_category[category].wrefcount);
        }

        if (ptloci->lc_category[category].locale != nullptr &&
            ptloci->lc_category[category].refcount != nullptr)
        {
            _InterlockedIncrement(ptloci->lc_category[category].refcount);
        }
    }

    __acrt_locale_add_lc_time_reference(ptloci->lc_time_curr);
}

/***
* __acrt_release_locale_ref(__crt_locale_data* ptloci)
*
* Purpose:
*       Decrement the refrence count for each elemnt in the localeinfo struct.
*
******************************************************************************/
extern "C" void __cdecl __acrt_release_locale_ref(__crt_locale_data* ptloci)
{
    if (ptloci == nullptr)
        return;

    _InterlockedDecrement(&ptloci->refcount);

    if (ptloci->lconv_intl_refcount != nullptr)
        _InterlockedDecrement(ptloci->lconv_intl_refcount);

    if (ptloci->lconv_mon_refcount != nullptr)
        _InterlockedDecrement(ptloci->lconv_mon_refcount);

    if (ptloci->lconv_num_refcount != nullptr)
        _InterlockedDecrement(ptloci->lconv_num_refcount);

    if (ptloci->ctype1_refcount != nullptr)
        _InterlockedDecrement(ptloci->ctype1_refcount);

    for (int category = LC_MIN; category <= LC_MAX; ++category)
    {
        if (ptloci->lc_category[category].wlocale != __acrt_wide_c_locale_string &&
            ptloci->lc_category[category].wrefcount != nullptr)
        {
            _InterlockedDecrement(ptloci->lc_category[category].wrefcount);
        }

        if (ptloci->lc_category[category].locale != nullptr &&
            ptloci->lc_category[category].refcount != nullptr)
        {
            _InterlockedDecrement(ptloci->lc_category[category].refcount);
        }
    }

    __acrt_locale_release_lc_time_reference(ptloci->lc_time_curr);
}


/***
*__acrt_free_locale() - free __crt_locale_data
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

extern "C" void __cdecl __acrt_free_locale(__crt_locale_data* ptloci)
{
    /*
     * Free up lconv struct
     */
    if (ptloci->lconv != nullptr &&
        ptloci->lconv != &__acrt_lconv_c &&
        ptloci->lconv_intl_refcount != nullptr &&
        *ptloci->lconv_intl_refcount == 0)
    {
        if (ptloci->lconv_mon_refcount != nullptr && *ptloci->lconv_mon_refcount == 0)
        {
            _free_crt(ptloci->lconv_mon_refcount);
            __acrt_locale_free_monetary(ptloci->lconv);
        }

        if (ptloci->lconv_num_refcount != nullptr && *ptloci->lconv_num_refcount == 0)
        {
            _free_crt(ptloci->lconv_num_refcount);
            __acrt_locale_free_numeric(ptloci->lconv);
        }

        _free_crt(ptloci->lconv_intl_refcount);
        _free_crt(ptloci->lconv);
    }

    /*
     * Free up ctype tables
     */
    if (ptloci->ctype1_refcount != nullptr && *ptloci->ctype1_refcount == 0)
    {
        _free_crt(ptloci->ctype1 - _COFFSET);
        _free_crt(const_cast<unsigned char*>(ptloci->pclmap - _COFFSET - 1));
        _free_crt(const_cast<unsigned char*>(ptloci->pcumap - _COFFSET - 1));
        _free_crt(ptloci->ctype1_refcount);
    }

    __acrt_locale_free_lc_time_if_unreferenced(ptloci->lc_time_curr);

    for (int category = LC_MIN; category <= LC_MAX; ++category)
    {
        if (ptloci->lc_category[category].wlocale != __acrt_wide_c_locale_string &&
            ptloci->lc_category[category].wrefcount != nullptr &&
            *ptloci->lc_category[category].wrefcount == 0)
        {
            _free_crt(ptloci->lc_category[category].wrefcount);
            _free_crt(ptloci->locale_name[category]);
        }

        _ASSERTE((ptloci->lc_category[category].locale != nullptr && ptloci->lc_category[category].refcount != nullptr) ||
                 (ptloci->lc_category[category].locale == nullptr && ptloci->lc_category[category].refcount == nullptr));

        if (ptloci->lc_category[category].locale != nullptr &&
            ptloci->lc_category[category].refcount != nullptr &&
            *ptloci->lc_category[category].refcount == 0)
        {
            _free_crt(ptloci->lc_category[category].refcount);
        }
    }

    /*
     * Free up the __crt_locale_data struct
     */
    _free_crt(ptloci);
}


/***
*
* _updatelocinfoEx_nolock(__crt_locale_data* *pptlocid, __crt_locale_data* ptlocis)
*
* Purpose:
*       Update *pptlocid to ptlocis. This might include freeing contents of *pptlocid.
*
******************************************************************************/
extern "C" __crt_locale_data* __cdecl _updatetlocinfoEx_nolock(
    __crt_locale_data** pptlocid,
    __crt_locale_data*  ptlocis
    )
{
    if (ptlocis == nullptr || pptlocid == nullptr)
        return nullptr;

    __crt_locale_data* const ptloci = *pptlocid;
    if (ptloci == ptlocis)
        return ptlocis;

    /*
    * Update to the current locale info structure and increment the
    * reference counts.
    */
    *pptlocid = ptlocis;
    __acrt_add_locale_ref(ptlocis);

    /*
    * Decrement the reference counts in the old locale info
    * structure.
    */
    if (ptloci != nullptr)
    {
        __acrt_release_locale_ref(ptloci);
    }

    /*
    * Free the old locale info structure, if necessary.  Must be done
    * after incrementing reference counts in current locale in case
    * any refcounts are shared with the old locale.
    */
    if (ptloci != nullptr &&
        ptloci->refcount == 0 &&
        ptloci != &__acrt_initial_locale_data)
    {
        __acrt_free_locale(ptloci);
    }

    return ptlocis;
}


/***
*__acrt_update_thread_locale_data() - refresh the thread's locale info
*
*Purpose:
*       If this thread does not have it's ownlocale which means that either
*       ownlocale flag in ptd is not set or ptd->ptloci == nullptr, then Update
*       the current thread's reference to the locale information to match the
*       current global locale info. Decrement the reference on the old locale
*       information struct and if this count is now zero (so that no threads
*       are using it), free it.
*
*Entry:
*
*Exit:
*
*       if (!_getptd()->ownlocale || _getptd()->ptlocinfo == nullptr)
*           _getptd()->ptlocinfo == __acrt_current_locale_data
*       else
*           _getptd()->ptlocinfo
*
*Exceptions:
*
*******************************************************************************/
extern "C" __crt_locale_data* __cdecl __acrt_update_thread_locale_data()
{
    __crt_locale_data* ptloci = nullptr;
    __acrt_ptd* const ptd = __acrt_getptd();

    if (__acrt_should_sync_with_global_locale(ptd) || ptd->_locale_info == nullptr)
    {
        __acrt_lock(__acrt_locale_lock);
        __try
        {
            ptloci = _updatetlocinfoEx_nolock(&ptd->_locale_info, __acrt_current_locale_data.value());
        }
        __finally
        {
            __acrt_unlock(__acrt_locale_lock);
        }
        __endtry
    }
    else
    {
        ptloci = ptd->_locale_info;
    }

    if (!ptloci)
    {
        abort();
    }

    return ptloci;
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// __crt_lc_time_data reference counting
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// The C locale lc_time data is immutable and persistent.  The lc_time data for
// any other locale is dynamically allocated (and thus not actually immutable).
// The lc_time_curr data member of __crt_locale_data is declared const to help
// ensure that code does not accidentally attempt to modify the C locale lc_time
// data.
//
// These functions encapsulate all mutation of the lc_time_curr.  They accept a
// __crt_lc_time_data const*, handle the C locale as a special case, then cast
// away constness to mutate the lc_time data.
extern "C" long __cdecl __acrt_locale_add_lc_time_reference(
    __crt_lc_time_data const* const lc_time
    )
{
    if (lc_time == nullptr || lc_time == &__lc_time_c)
    {
        return LONG_MAX;
    }

    return _InterlockedIncrement(&const_cast<__crt_lc_time_data*>(lc_time)->refcount);
}

extern "C" long __cdecl __acrt_locale_release_lc_time_reference(
    __crt_lc_time_data const* const lc_time
    )
{
    if (lc_time == nullptr || lc_time == &__lc_time_c)
    {
        return LONG_MAX;
    }

    return _InterlockedDecrement(&const_cast<__crt_lc_time_data*>(lc_time)->refcount);
}

extern "C" void __cdecl __acrt_locale_free_lc_time_if_unreferenced(
    __crt_lc_time_data const* const lc_time
    )
{
    if (lc_time == nullptr || lc_time == &__lc_time_c)
    {
        return;
    }

    if (__crt_interlocked_read(&lc_time->refcount) != 0)
    {
        return;
    }

    __acrt_locale_free_time(const_cast<__crt_lc_time_data*>(lc_time));
    _free_crt(const_cast<__crt_lc_time_data*>(lc_time));
}
