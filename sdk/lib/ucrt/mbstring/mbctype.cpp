/***
*mbctype.c - MBCS table used by the functions that test for types of char
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       table used to determine the type of char
*
*******************************************************************************/
#include <corecrt_internal.h>
#include <locale.h>
#include <corecrt_internal_mbstring.h>
#include <mbctype.h>
#include <winnls.h>

#ifndef CRTDLL

_CRT_LINKER_FORCE_INCLUDE(__acrt_multibyte_initializer);

#endif  /* CRTDLL */

#define _CHINESE_SIMP_CP    936
#define _KOREAN_WANGSUNG_CP 949
#define _CHINESE_TRAD_CP    950
#define _KOREAN_JOHAB_CP    1361

#define NUM_CHARS 257 /* -1 through 255 */

#define NUM_CTYPES 4 /* table contains 4 types of info */
#define MAX_RANGES 8 /* max number of ranges needed given languages so far */

/* character type info in ranges (pair of low/high), zeros indicate end */
typedef struct
{
    int             code_page;
    unsigned short  mbulinfo[NUM_ULINFO];
    unsigned char   rgrange[NUM_CTYPES][MAX_RANGES];
} code_page_info;

extern "C"
{
__crt_multibyte_data __acrt_initial_multibyte_data =
{
    0,                       /* refcount */
    CP_ACP,                  /* mbcodepage: _MB_CP_ANSI */
    0,                       /* ismbcodepage */
    { 0, 0, 0, 0, 0, 0 },    /* mbulinfo[6] */
    {                        /* mbctype[257] */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00 /* rest is zero */
    },
    {     /* mbcasemap[256] */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
    0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73,
    0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b,
    0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
    0x58, 0x59, 0x5a, 0x00, 0x00, 0x00, 0x00, 0x00 /* rest is zero */
    },
    nullptr /* mblocalename */
};

#define _MBCTYPE_DEFAULT                                                        \
    {                                                                           \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, \
        0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, \
        0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, \
        0x00, 0x00, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, \
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, \
        0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00 /* rest is zero */       \
    }

/* MBCS ctype array */
static unsigned char _mbctypes[__crt_state_management::state_index_count][NUM_CHARS] =
{
    _MBCTYPE_DEFAULT
    #ifdef _CRT_GLOBAL_STATE_ISOLATION
    ,_MBCTYPE_DEFAULT
    #endif
};

#define _MBCASEMAP_DEFAULT                                                      \
    {                                                                           \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, \
        0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, \
        0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x00, 0x00, 0x00, 0x00, 0x00, \
        0x00, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, \
        0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, \
        0x58, 0x59, 0x5a, 0x00, 0x00, 0x00, 0x00, 0x00 /* rest is zero */       \
    }

static unsigned char _mbcasemaps[__crt_state_management::state_index_count][256] =
{
    _MBCASEMAP_DEFAULT
    #ifdef _CRT_GLOBAL_STATE_ISOLATION
    ,_MBCASEMAP_DEFAULT
    #endif
};

/* global pointer to the multi-byte case type (i.e. upper or lower or n/a) */
__crt_state_management::dual_state_global<unsigned char*> _mbctype;

/* global pointer to the multi-byte casemap */
__crt_state_management::dual_state_global<unsigned char*> _mbcasemap;

/* global pointer to the current per-thread mbc information structure. */
__crt_state_management::dual_state_global<__crt_multibyte_data*> __acrt_current_multibyte_data;
}

static int fSystemSet;

static char __rgctypeflag[NUM_CTYPES] = { _MS, _MP, _M1, _M2 };

static code_page_info __rgcode_page_info[] =
{
    {
      _KANJI_CP, /* Kanji (Japanese) Code Page */
      { 0x8260, 0x8279,   /* Full-Width Latin Upper Range 1 */
        0x8281 - 0x8260,  /* Full-Width Latin Case Difference 1 */

        0x0000, 0x0000,   /* Full-Width Latin Upper Range 2 */
        0x0000            /* Full-Width Latin Case Difference 2 */
      },
      {
        { 0xA6, 0xDF, 0,    0,    0,    0,    0, 0, }, /* Single Byte Ranges */
        { 0xA1, 0xA5, 0,    0,    0,    0,    0, 0, }, /* Punctuation Ranges */
        { 0x81, 0x9F, 0xE0, 0xFC, 0,    0,    0, 0, }, /* Lead Byte Ranges */
        { 0x40, 0x7E, 0x80, 0xFC, 0,    0,    0, 0, }, /* Trail Byte Ranges */
      }
    },
    {
      _CHINESE_SIMP_CP, /* Chinese Simplified (PRC) Code Page */
      { 0xA3C1, 0xA3DA,   /* Full-Width Latin Upper Range 1 */
        0xA3E1 - 0xA3C1,  /* Full-Width Latin Case Difference 1 */

        0x0000, 0x0000,   /* Full-Width Latin Upper Range 2 */
        0x0000            /* Full-Width Latin Case Difference 2 */
      },
      {
        { 0,    0,    0,    0,    0,    0,    0, 0, }, /* Single Byte Ranges */
        { 0,    0,    0,    0,    0,    0,    0, 0, }, /* Punctuation Ranges */
        { 0x81, 0xFE, 0,    0,    0,    0,    0, 0, }, /* Lead Byte Ranges */
        { 0x40, 0xFE, 0,    0,    0,    0,    0, 0, }, /* Trail Byte Ranges */
      }
    },
    {
      _KOREAN_WANGSUNG_CP, /* Wangsung (Korean) Code Page */
      { 0xA3C1, 0xA3DA,   /* Full-Width Latin Upper Range 1 */
        0xA3E1 - 0xA3C1,  /* Full-Width Latin Case Difference 1 */

        0x0000, 0x0000,   /* Full-Width Latin Upper Range 2 */
        0x0000            /* Full-Width Latin Case Difference 2 */
      },
      {
        { 0,    0,    0,    0,    0,    0,    0, 0, }, /* Single Byte Ranges */
        { 0,    0,    0,    0,    0,    0,    0, 0, }, /* Punctuation Ranges */
        { 0x81, 0xFE, 0,    0,    0,    0,    0, 0, }, /* Lead Byte Ranges */
        { 0x41, 0xFE, 0,    0,    0,    0,    0, 0, }, /* Trail Byte Ranges */
      }
    },
    {
      _CHINESE_TRAD_CP, /* Chinese Traditional (Taiwan) Code Page */
      { 0xA2CF, 0xA2E4,   /* Full-Width Latin Upper Range 1 */
        0xA2E9 - 0xA2CF,  /* Full-Width Latin Case Difference 1 */

        0xA2E5, 0xA2E8,   /* Full-Width Latin Upper Range 2 */
        0xA340 - 0XA2E5   /* Full-Width Latin Case Difference 2 */
      },
      {
        { 0,    0,    0,    0,    0,    0,    0, 0, }, /* Single Byte Ranges */
        { 0,    0,    0,    0,    0,    0,    0, 0, }, /* Punctuation Ranges */
        { 0x81, 0xFE, 0,    0,    0,    0,    0, 0, }, /* Lead Byte Ranges */
        { 0x40, 0x7E, 0xA1, 0xFE, 0,    0,    0, 0, }, /* Trail Byte Ranges */
      }
    },
    {
      _KOREAN_JOHAB_CP, /* Johab (Korean) Code Page */
      { 0xDA51, 0xDA5E,   /* Full-Width Latin Upper Range 1 */
        0xDA71 - 0xDA51,  /* Full-Width Latin Case Difference 1 */

        0xDA5F, 0xDA6A,   /* Full-Width Latin Upper Range 2 */
        0xDA91 - 0xDA5F   /* Full-Width Latin Case Difference 2 */
      },
      {
        { 0,    0,    0,    0,    0,    0,    0, 0, }, /* Single Byte Ranges */
        { 0,    0,    0,    0,    0,    0,    0, 0, }, /* Punctuation Ranges */
        { 0x81, 0xD3, 0xD8, 0xDE, 0xE0, 0xF9, 0, 0, }, /* Lead Byte Ranges */
        { 0x31, 0x7E, 0x81, 0xFE, 0,    0,    0, 0, }, /* Trail Byte Ranges */
      }
    }
};

#define JAPANSE_DEFAULT_LOCALE_NAME_INDEX         0
#define CHINESE_SIMPLIFIED_LOCALE_NAME_INDEX      1
#define KOREAN_DEFAULT_LOCALE_NAME_INDEX          2
#define CHINESE_TRADITIONAL_LOCALE_NAME_INDEX     3

const wchar_t* const _mb_locale_names[] =
{
    L"ja-JP",   /* JAPANSE_DEFAULT_LOCALE_NAME_INDEX     */
    L"zh-CN",   /* CHINESE_SIMPLIFIED_LOCALE_NAME_INDEX  */
    L"ko-KR",   /* KOREAN_DEFAULT_LOCALE_NAME_INDEX      */
    L"zh-TW",   /* CHINESE_TRADITIONAL_LOCALE_NAME_INDEX */
};



extern "C" unsigned char* __cdecl __p__mbctype()
{
    return _mbctype.value();
}

extern "C" unsigned char* __cdecl __p__mbcasemap()
{
    return _mbcasemap.value();
}



extern "C" int __cdecl _setmbcp_nolock(int, __crt_multibyte_data*);

static int getSystemCP (int);

/***
*setSBCS() - Set MB code page to SBCS.
*
*Purpose:
*           Set MB code page to SBCS.
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

static void setSBCS (__crt_multibyte_data* ptmbci)
{
    int i;

    /* set for single-byte code page */
    for (i = 0; i < NUM_CHARS; i++)
        ptmbci->mbctype[i] = 0;

    /* code page has changed, set global flag */
    ptmbci->mbcodepage = 0;

    /* clear flag to indicate single-byte code */
    ptmbci->ismbcodepage = 0;

    ptmbci->mblocalename = nullptr;

    for (i = 0; i < NUM_ULINFO; i++)
        ptmbci->mbulinfo[i] = 0;

    for ( i = 0 ; i < 257 ; i++ )
        ptmbci->mbctype[i] = __acrt_initial_multibyte_data.mbctype[i];

    for ( i = 0 ; i < 256 ; i++ )
        ptmbci->mbcasemap[i] = __acrt_initial_multibyte_data.mbcasemap[i];
}

/***
*__acrt_update_thread_multibyte_data() - refresh the thread's mbc info
*
*Purpose:
*       Update the current thread's reference to the multibyte character
*       information to match the current global mbc info. Decrement the
*       reference on the old mbc information struct and if this count is now
*       zero (so that no threads are using it), free it.
*
*Entry:
*
*Exit:
*       _getptd()->ptmbcinfo == current_multibyte_data (which should always be __acrt_current_multibyte_data)
*
*Exceptions:
*
*******************************************************************************/

static __crt_multibyte_data* __cdecl update_thread_multibyte_data_internal(
    __acrt_ptd*           const ptd,
    __crt_multibyte_data** const current_multibyte_data
    ) throw()
{
        __crt_multibyte_data* ptmbci = nullptr;

        if (__acrt_should_sync_with_global_locale(ptd) || ptd->_locale_info == nullptr)
        {
            __acrt_lock(__acrt_multibyte_cp_lock);
            __try
            {
                ptmbci = ptd->_multibyte_info;
                if (ptmbci != *current_multibyte_data)
                {
                    /*
                     * Decrement the reference count in the old mbc info structure
                     * and free it, if necessary
                     */
                    if (ptmbci != nullptr &&
                        InterlockedDecrement(&ptmbci->refcount) == 0 &&
                        ptmbci != &__acrt_initial_multibyte_data)
                    {
                        /*
                         * Free it
                         */
                        _free_crt(ptmbci);
                    }

                    /*
                     * Point to the current mbc info structure and increment its
                     * reference count.
                     */
                    ptmbci = ptd->_multibyte_info = *current_multibyte_data;
                    InterlockedIncrement(&ptmbci->refcount);
                }
            }
            __finally
            {
                __acrt_unlock(__acrt_multibyte_cp_lock);
            }
            __endtry
        }
        else
        {
            ptmbci = ptd->_multibyte_info;
        }

        if (!ptmbci)
        {
            abort();
        }

        return ptmbci;
}

extern "C" __crt_multibyte_data* __cdecl __acrt_update_thread_multibyte_data()
{
    return update_thread_multibyte_data_internal(__acrt_getptd(), &__acrt_current_multibyte_data.value());
}

/***
*_setmbcp() - Set MBC data based on code page
*
*Purpose:
*       Init MBC character type tables based on code page number. If
*       given code page is supported, load that code page info into
*       mbctype table. If not, query OS to find the information,
*       otherwise set up table with single byte info.
*
*       Multithread Notes: First, allocate an mbc information struct. Set the
*       mbc info in the static vars and arrays as does the single-thread
*       version. Then, copy this info into the new allocated struct and set
*       the current mbc info pointer (__acrt_current_multibyte_data) to point to it.
*
*Entry:
*       codepage - code page to initialize MBC table
*           _MB_CP_OEM = use system OEM code page
*           _MB_CP_ANSI = use system ANSI code page
*           _MB_CP_SBCS = set to single byte 'code page'
*
*Exit:
*        0 = Success
*       -1 = Error, code page not changed.
*
*Exceptions:
*
*******************************************************************************/

static int __cdecl setmbcp_internal(
    int                    const requested_codepage,
    bool                   const is_for_crt_initialization,
    __acrt_ptd*            const ptd,
    __crt_multibyte_data** const current_multibyte_data
    ) throw()
{
    update_thread_multibyte_data_internal(ptd, current_multibyte_data);
    int const system_codepage = getSystemCP(requested_codepage);

    // If it's not a new codepage, just return success:
    if (system_codepage == ptd->_multibyte_info->mbcodepage)
    {
        return 0;
    }

    // Always allocate space so that we don't have to take a lock for any update:
    __crt_unique_heap_ptr<__crt_multibyte_data> mb_data(_malloc_crt_t(__crt_multibyte_data, 1));
    if (!mb_data)
    {
        return -1;
    }

    // Initialize the new multibyte data structure from the current multibyte
    // data structure for this thread, resetting the reference count (since it
    // is not actually referenced by anything yet).
    *mb_data.get() = *ptd->_multibyte_info;
    mb_data.get()->refcount = 0;

    // Actually initialize the new multibyte data using the new codepage:
    // CRT_REFACTOR TODO _setmbcp_nolock is a terrible name.
    int const setmbcp_status = _setmbcp_nolock(system_codepage, mb_data.get());
    if (setmbcp_status == -1)
    {
        errno = EINVAL;
        return -1;
    }

    // At this point, we have a valid, new set of multibyte data to swap in.  If
    // this is not the initial codepage initialization during process startup,
    // we need to toggle the locale-changed state:
    if (!is_for_crt_initialization)
    {
        __acrt_set_locale_changed();
    }

    if (InterlockedDecrement(&ptd->_multibyte_info->refcount) == 0 &&
        ptd->_multibyte_info != &__acrt_initial_multibyte_data)
    {
        _free_crt(ptd->_multibyte_info);
    }

    // Update the multibyte codepage for this thread:
    mb_data.get()->refcount = 1;
    ptd->_multibyte_info = mb_data.detach();

    // If this thread has its own locale, do not update the global codepage:
    if (!__acrt_should_sync_with_global_locale(ptd))
    {
        return setmbcp_status;
    }

    // Otherwise, update the global codepage:
    __acrt_lock_and_call(__acrt_multibyte_cp_lock, [&]
    {
        memcpy_s(_mbctype.value(),   sizeof(_mbctypes[0]),   ptd->_multibyte_info->mbctype,   sizeof(ptd->_multibyte_info->mbctype));
        memcpy_s(_mbcasemap.value(), sizeof(_mbcasemaps[0]), ptd->_multibyte_info->mbcasemap, sizeof(ptd->_multibyte_info->mbcasemap));

        if (InterlockedDecrement(&(*current_multibyte_data)->refcount) == 0 &&
            (*current_multibyte_data) != &__acrt_initial_multibyte_data)
        {
            _free_crt(*current_multibyte_data);
        }

        *current_multibyte_data = ptd->_multibyte_info;
        InterlockedIncrement(&ptd->_multibyte_info->refcount);
    });

    if (is_for_crt_initialization)
    {
        __acrt_initial_locale_pointers.mbcinfo = *current_multibyte_data;
    }

    return setmbcp_status;
}

/* Enclaves only support built-in CP_ACP */
#ifdef _UCRT_ENCLAVE_BUILD

static int getSystemCP(int)
{
    return CP_ACP;
}


extern "C" int __cdecl _setmbcp_nolock(int, __crt_multibyte_data* ptmbci)
{
    setSBCS(ptmbci);
    return 0;
}

#else /* ^^^ _UCRT_ENCLAVE_BUILD ^^^ // vvv !_UCRT_ENCLAVE_BUILD vvv */

    /***
*CPtoLocaleName() - Code page to locale name.
*
*Purpose:
*       Some API calls want a locale name, so convert MB CP to appropriate locale name,
*       and then API converts back to ANSI CP for that locale name.
*
*Entry:
*   codepage - code page to convert
*Exit:
*       returns appropriate locale name
*       Returned locale names are stored in static structs, so they must not be deleted.
*
*Exceptions:
*
*******************************************************************************/

static const wchar_t* CPtoLocaleName (int codepage)
{
    switch (codepage) {
    case 932:
        return _mb_locale_names[JAPANSE_DEFAULT_LOCALE_NAME_INDEX];
    case 936:
        return _mb_locale_names[CHINESE_SIMPLIFIED_LOCALE_NAME_INDEX];
    case 949:
        return _mb_locale_names[KOREAN_DEFAULT_LOCALE_NAME_INDEX];
    case 950:
        return _mb_locale_names[CHINESE_TRADITIONAL_LOCALE_NAME_INDEX];
    }

    return 0;
}

/***
*getSystemCP - Get system default CP if requested.
*
*Purpose:
*       Get system default CP if requested.
*
*Entry:
*       codepage - user requested code page/world script
*
*       Docs specify:
*          _MB_CP_SBCS    0 - Use a single byte codepage
*          _MB_CP_OEM    -2 - use the OEMCP
*          _MB_CP_ANSI   -3 - use the ACP
*          _MB_CP_LOCALE -4 - use the codepage for a previous setlocale call
*          Codepage #       - use the specified codepage (UTF-7 is disallowed)
*                           - 54936 and other interesting stateful codepages aren't
*                           - explicitly disallowed but I can't imagine them working right.
*
*Exit:
*       requested code page
*
*Exceptions:
*
*******************************************************************************/
static int getSystemCP(int codepage)
{
    _locale_t plocinfo = nullptr;
    _LocaleUpdate _loc_update(plocinfo);
    fSystemSet = 0;

    /* get system code page values if requested */

    if (codepage == _MB_CP_OEM)
    {
        fSystemSet = 1;
        return GetOEMCP();
    }
    else if (codepage == _MB_CP_ANSI)
    {
        fSystemSet = 1;
        return GetACP();
    }
    else if (codepage == _MB_CP_LOCALE)
    {
        fSystemSet = 1;
        return _loc_update.GetLocaleT()->locinfo->_public._locale_lc_codepage;
    }

    return codepage;
}

/***
*setSBUpLow() - Set single byte range upper/lower mappings
*
*Purpose:
*           Set single byte mapping for tolower/toupper.
*           Basically this is ASCII-mapping plus a few if you're a lucky
*           SBCS codepage.
*           DBCS + UTF ranges > 0x7f are basically ignored.
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

static void setSBUpLow (__crt_multibyte_data* ptmbci)
{
    BYTE *  pbPair;
    UINT    ich;
    CPINFO  cpInfo;
    UCHAR   sbVector[256];
    UCHAR   upVector[256];
    UCHAR   lowVector[256];
    USHORT  wVector[512];

    //    test if codepage exists
    if (ptmbci->mbcodepage != CP_UTF8 && GetCPInfo(ptmbci->mbcodepage, &cpInfo) != 0)
    {
        // This code attempts to generate casing tables for characters 0-255
        // For DBCS codepages that will be basically ASCII casing but won't help DBCS mapping.
        // For SBCS codepages that will include the codepage-specific characters.
        // Mappings do not appear to include Turkish-i variations.

        //  if so, create vector 0-255
        for (ich = 0; ich < 256; ich++)
            sbVector[ich] = (UCHAR) ich;

        //  set byte 0 and any leading byte value to non-alpha char ' '
        sbVector[0] = (UCHAR)' ';
        for (pbPair = &cpInfo.LeadByte[0]; *pbPair; pbPair += 2)
            // make sure ich within a valid range
            for (ich = *pbPair; ich <= *(pbPair + 1) && ich < 256; ich++)
                sbVector[ich] = (UCHAR)' ';

        //  get char type for character vector

        __acrt_GetStringTypeA(nullptr, CT_CTYPE1, (LPCSTR)sbVector, 256, wVector,
                            ptmbci->mbcodepage, FALSE);

        //  get lower case mappings for character vector

        __acrt_LCMapStringA(nullptr, ptmbci->mblocalename, LCMAP_LOWERCASE, (LPCSTR)sbVector, 256,
                                    (LPSTR)lowVector, 256, ptmbci->mbcodepage, FALSE);

        //  get upper case mappings for character vector

        __acrt_LCMapStringA(nullptr, ptmbci->mblocalename, LCMAP_UPPERCASE, (LPCSTR)sbVector, 256,
                                    (LPSTR)upVector, 256, ptmbci->mbcodepage, FALSE);

        //  set _SBUP, _SBLOW in ptmbci->mbctype if type is upper. lower
        //  set mapping array with lower or upper mapping value

        for (ich = 0; ich < 256; ich++)
        {
            if (wVector[ich] & _UPPER)
            {
                // WARNING: +1 because the mbctype array starts with a -1 EOF character
                ptmbci->mbctype[ich + 1] |= _SBUP;
                ptmbci->mbcasemap[ich] = lowVector[ich];
            }
            else if (wVector[ich] & _LOWER)
            {
                // WARNING: +1 because the mbctype array starts with a -1 EOF character
                ptmbci->mbctype[ich + 1] |= _SBLOW;
                ptmbci->mbcasemap[ich] = upVector[ich];
            }
            else
                ptmbci->mbcasemap[ich] = 0;
        }
    }
    else
    {
        //  Either no codepage or UTF-8 (which looks a lot like ASCII in the lower bits)
        //  Set 'A'-'Z' as upper, 'a'-'z' as lower (eg: ASCII casing)
        for (ich = 0; ich < 256; ich++)
        {
            if (ich >= (UINT)'A' && ich <= (UINT)'Z')
            {
                // WARNING: +1 because the mbctype array starts with a -1 EOF character
                ptmbci->mbctype[ich + 1] |= _SBUP;
                ptmbci->mbcasemap[ich] = static_cast<unsigned char>(ich + ('a' - 'A'));
            }
            else if (ich >= (UINT)'a' && ich <= (UINT)'z')
            {
                // WARNING: +1 because the mbctype array starts with a -1 EOF character
                ptmbci->mbctype[ich + 1] |= _SBLOW;
                ptmbci->mbcasemap[ich] = static_cast<unsigned char>(ich - ('a' - 'A'));
            }
            else
                ptmbci->mbcasemap[ich] = 0;
        }
    }
}

extern "C" int __cdecl _setmbcp(int const codepage)
{
    return setmbcp_internal(codepage, false, __acrt_getptd(), &__acrt_current_multibyte_data.value());
}

extern "C" int __cdecl _setmbcp_nolock(int codepage, __crt_multibyte_data* ptmbci)
{
        unsigned int icp;
        unsigned int irg;
        unsigned int ich;
        unsigned char *rgptr;
        CPINFO cpInfo;

        codepage = getSystemCP(codepage);

        /* user wants 'single-byte' MB code page */
        if (codepage == _MB_CP_SBCS)
        {
            setSBCS(ptmbci);
            return 0;
        }

        /* check for CRT code page info */
        for (icp = 0;
            icp < (sizeof(__rgcode_page_info) / sizeof(code_page_info));
            icp++)
        {
            /* see if we have info for this code page */
            if (__rgcode_page_info[icp].code_page == codepage)
            {
                /* clear the table */
                for (ich = 0; ich < NUM_CHARS; ich++)
                    ptmbci->mbctype[ich] = 0;

                /* for each type of info, load table */
                for (irg = 0; irg < NUM_CTYPES; irg++)
                {
                    /* go through all the ranges for each type of info */
                    for (rgptr = (unsigned char *)__rgcode_page_info[icp].rgrange[irg];
                        rgptr[0] && rgptr[1];
                        rgptr += 2)
                    {
                        /* set the type for every character in range */
                        for (ich = rgptr[0]; ich <= rgptr[1] && ich < 256; ich++)
                            ptmbci->mbctype[ich + 1] |= __rgctypeflag[irg];
                    }
                }
                /* code page has changed */
                ptmbci->mbcodepage = codepage;
                /* all the code pages we keep info for are truly multibyte */
                ptmbci->ismbcodepage = 1;
                ptmbci->mblocalename = CPtoLocaleName(ptmbci->mbcodepage);
                for (irg = 0; irg < NUM_ULINFO; irg++)
                {
                    ptmbci->mbulinfo[irg] = __rgcode_page_info[icp].mbulinfo[irg];
                }

                /* return success */
                setSBUpLow(ptmbci);
                return 0;
            }
        }

        /*  verify codepage validity */
        // Unclear why UTF7 is excluded yet stateful and other complex encodings are not
        if (codepage == 0 || codepage == CP_UTF7 || !IsValidCodePage((WORD)codepage))
        {
            /* return failure, code page not changed */
            return -1;
        }

        // Special case for UTF-8
        if (codepage == CP_UTF8)
        {
            ptmbci->mbcodepage = CP_UTF8;
            ptmbci->mblocalename = nullptr;

            // UTF-8 does not have lead or trail bytes in the terms
            // the CRT thinks of it for DBCS codepages, so we'll
            // clear the flags for all bytes.
            // Note that this array is 257 bytes because there's a
            // "-1" that is used someplaces for EOF.  So this array
            // is actually -1 based.
            for (ich = 0; ich < NUM_ULINFO; ich++)
            {
                ptmbci->mbctype[ich] = 0;
            }

            // not really a multibyte code page, we'll have to test
            // ptmbci->mbcodepage == CP_UTF8 when we use this structure.
            ptmbci->ismbcodepage = 0;

            // CJK encodings have some full-width mappings, but not here.
            for (irg = 0; irg < NUM_ULINFO; irg++)
            {
                ptmbci->mbulinfo[irg] = 0;
            }

            setSBUpLow(ptmbci);

            // return success
            return 0;
        }
        /* code page not supported by CRT, try the OS */\
        else if (GetCPInfo(codepage, &cpInfo) != 0)
        {
            BYTE *lbptr;

            /* clear the table */
            for (ich = 0; ich < NUM_CHARS; ich++)
            {
                ptmbci->mbctype[ich] = 0;
            }

            ptmbci->mbcodepage = codepage;
            ptmbci->mblocalename = nullptr;

            // Special case for DBCS where we know there may be a leadbyte/trailbyte pattern
            if (cpInfo.MaxCharSize == 2)
            {
                /* LeadByte range always terminated by two 0's */
                for (lbptr = cpInfo.LeadByte; *lbptr && *(lbptr + 1); lbptr += 2)
                {
                    for (ich = *lbptr; ich <= *(lbptr + 1); ich++)
                        ptmbci->mbctype[ich + 1] |= _M1;
                }

                /* All chars > 1 must be considered valid trail bytes */
                for (ich = 0x01; ich < 0xFF; ich++)
                {
                    ptmbci->mbctype[ich + 1] |= _M2;
                }

                /* code page has changed */
                ptmbci->mblocalename = CPtoLocaleName(ptmbci->mbcodepage);

                /* really a multibyte code page */
                ptmbci->ismbcodepage = 1;
            }
            else
            {
                /* single-byte code page */
                ptmbci->ismbcodepage = 0;
            }

            for (irg = 0; irg < NUM_ULINFO; irg++)
            {
                ptmbci->mbulinfo[irg] = 0;
            }

            setSBUpLow(ptmbci);
            /* return success */
            return 0;
        }


        /* If system default call, don't fail - set to SBCS */
        if (fSystemSet)
        {
            setSBCS(ptmbci);
            return 0;
        }

        /* return failure, code page not changed */
        return -1;
}

#endif /* _UCRT_ENCLAVE_BUILD */

/***
*_getmbcp() - Get the current MBC code page
*
*Purpose:
*           Get code page value.
*Entry:
*       none.
*Exit:
*           return current MB codepage value.
*
*Exceptions:
*
*******************************************************************************/

extern "C" int __cdecl _getmbcp()
{
    _locale_t plocinfo = nullptr;
    _LocaleUpdate _loc_update(plocinfo);
    if ( _loc_update.GetLocaleT()->mbcinfo->ismbcodepage )
        return _loc_update.GetLocaleT()->mbcinfo->mbcodepage;
    else
        return 0;
}


/***
*_initmbctable() - Set MB ctype table to initial default value.
*
*Purpose:
*       Initialization.
*Entry:
*       none.
*Exit:
*       Returns 0 to indicate no error.
*Exceptions:
*
*******************************************************************************/

extern "C" bool __cdecl __acrt_initialize_multibyte()
{
    static bool initialized = false;

    // Synchronization note:  it is not possible for a data race to occur here.
    // In the CRT DLLs, this function is called during CRT startup, befor any
    // user code using the CRT may run.  In the static CRT, this function is
    // called by a CRT initializer (at the top of this file), so 'initialized'
    // will be true before any user code can enter the CRT.
    //
    // CRT_REFACTOR TODO We should split this function into two parts:  one that
    // does the initialization (without any check), and one that does nothing,
    // but can be used to cause this object to be linked in.
    if (!initialized)
    {
        // initialize global pointer to the current per-thread mbc information structure
        __acrt_current_multibyte_data.initialize(&__acrt_initial_multibyte_data);

        // initialize mbc pointers
        _mbcasemap.initialize_from_array(_mbcasemaps);
        _mbctype  .initialize_from_array(_mbctypes);

        // initialize the multibyte globals
        __acrt_ptd* const ptd_head = __acrt_getptd_head();
        for (size_t i = 0; i != __crt_state_management::state_index_count; ++i)
        {
            setmbcp_internal(_MB_CP_ANSI, true, ptd_head + i, &__acrt_current_multibyte_data.dangerous_get_state_array()[i]);
        }

        initialized = 1;
    }

    return true;
}


/************************ Code Page info from NT/Win95 ********************


*** Code Page 932 ***

0x824f  ;Fullwidth Digit Zero
0x8250  ;Fullwidth Digit One
0x8251  ;Fullwidth Digit Two
0x8252  ;Fullwidth Digit Three
0x8253  ;Fullwidth Digit Four
0x8254  ;Fullwidth Digit Five
0x8255  ;Fullwidth Digit Six
0x8256  ;Fullwidth Digit Seven
0x8257  ;Fullwidth Digit Eight
0x8258  ;Fullwidth Digit Nine

0x8281  0x8260  ;Fullwidth Small A -> Fullwidth Capital A
0x8282  0x8261  ;Fullwidth Small B -> Fullwidth Capital B
0x8283  0x8262  ;Fullwidth Small C -> Fullwidth Capital C
0x8284  0x8263  ;Fullwidth Small D -> Fullwidth Capital D
0x8285  0x8264  ;Fullwidth Small E -> Fullwidth Capital E
0x8286  0x8265  ;Fullwidth Small F -> Fullwidth Capital F
0x8287  0x8266  ;Fullwidth Small G -> Fullwidth Capital G
0x8288  0x8267  ;Fullwidth Small H -> Fullwidth Capital H
0x8289  0x8268  ;Fullwidth Small I -> Fullwidth Capital I
0x828a  0x8269  ;Fullwidth Small J -> Fullwidth Capital J
0x828b  0x826a  ;Fullwidth Small K -> Fullwidth Capital K
0x828c  0x826b  ;Fullwidth Small L -> Fullwidth Capital L
0x828d  0x826c  ;Fullwidth Small M -> Fullwidth Capital M
0x828e  0x826d  ;Fullwidth Small N -> Fullwidth Capital N
0x828f  0x826e  ;Fullwidth Small O -> Fullwidth Capital O
0x8290  0x826f  ;Fullwidth Small P -> Fullwidth Capital P
0x8291  0x8270  ;Fullwidth Small Q -> Fullwidth Capital Q
0x8292  0x8271  ;Fullwidth Small R -> Fullwidth Capital R
0x8293  0x8272  ;Fullwidth Small S -> Fullwidth Capital S
0x8294  0x8273  ;Fullwidth Small T -> Fullwidth Capital T
0x8295  0x8274  ;Fullwidth Small U -> Fullwidth Capital U
0x8296  0x8275  ;Fullwidth Small V -> Fullwidth Capital V
0x8297  0x8276  ;Fullwidth Small W -> Fullwidth Capital W
0x8298  0x8277  ;Fullwidth Small X -> Fullwidth Capital X
0x8299  0x8278  ;Fullwidth Small Y -> Fullwidth Capital Y
0x829a  0x8279  ;Fullwidth Small Z -> Fullwidth Capital Z


*** Code Page 936 ***

0xa3b0  ;Fullwidth Digit Zero
0xa3b1  ;Fullwidth Digit One
0xa3b2  ;Fullwidth Digit Two
0xa3b3  ;Fullwidth Digit Three
0xa3b4  ;Fullwidth Digit Four
0xa3b5  ;Fullwidth Digit Five
0xa3b6  ;Fullwidth Digit Six
0xa3b7  ;Fullwidth Digit Seven
0xa3b8  ;Fullwidth Digit Eight
0xa3b9  ;Fullwidth Digit Nine

0xa3e1  0xa3c1  ;Fullwidth Small A -> Fullwidth Capital A
0xa3e2  0xa3c2  ;Fullwidth Small B -> Fullwidth Capital B
0xa3e3  0xa3c3  ;Fullwidth Small C -> Fullwidth Capital C
0xa3e4  0xa3c4  ;Fullwidth Small D -> Fullwidth Capital D
0xa3e5  0xa3c5  ;Fullwidth Small E -> Fullwidth Capital E
0xa3e6  0xa3c6  ;Fullwidth Small F -> Fullwidth Capital F
0xa3e7  0xa3c7  ;Fullwidth Small G -> Fullwidth Capital G
0xa3e8  0xa3c8  ;Fullwidth Small H -> Fullwidth Capital H
0xa3e9  0xa3c9  ;Fullwidth Small I -> Fullwidth Capital I
0xa3ea  0xa3ca  ;Fullwidth Small J -> Fullwidth Capital J
0xa3eb  0xa3cb  ;Fullwidth Small K -> Fullwidth Capital K
0xa3ec  0xa3cc  ;Fullwidth Small L -> Fullwidth Capital L
0xa3ed  0xa3cd  ;Fullwidth Small M -> Fullwidth Capital M
0xa3ee  0xa3ce  ;Fullwidth Small N -> Fullwidth Capital N
0xa3ef  0xa3cf  ;Fullwidth Small O -> Fullwidth Capital O
0xa3f0  0xa3d0  ;Fullwidth Small P -> Fullwidth Capital P
0xa3f1  0xa3d1  ;Fullwidth Small Q -> Fullwidth Capital Q
0xa3f2  0xa3d2  ;Fullwidth Small R -> Fullwidth Capital R
0xa3f3  0xa3d3  ;Fullwidth Small S -> Fullwidth Capital S
0xa3f4  0xa3d4  ;Fullwidth Small T -> Fullwidth Capital T
0xa3f5  0xa3d5  ;Fullwidth Small U -> Fullwidth Capital U
0xa3f6  0xa3d6  ;Fullwidth Small V -> Fullwidth Capital V
0xa3f7  0xa3d7  ;Fullwidth Small W -> Fullwidth Capital W
0xa3f8  0xa3d8  ;Fullwidth Small X -> Fullwidth Capital X
0xa3f9  0xa3d9  ;Fullwidth Small Y -> Fullwidth Capital Y
0xa3fa  0xa3da  ;Fullwidth Small Z -> Fullwidth Capital Z


*** Code Page 949 ***

0xa3b0  ;Fullwidth Digit Zero
0xa3b1  ;Fullwidth Digit One
0xa3b2  ;Fullwidth Digit Two
0xa3b3  ;Fullwidth Digit Three
0xa3b4  ;Fullwidth Digit Four
0xa3b5  ;Fullwidth Digit Five
0xa3b6  ;Fullwidth Digit Six
0xa3b7  ;Fullwidth Digit Seven
0xa3b8  ;Fullwidth Digit Eight
0xa3b9  ;Fullwidth Digit Nine

0xa3e1  0xa3c1  ;Fullwidth Small A -> Fullwidth Capital A
0xa3e2  0xa3c2  ;Fullwidth Small B -> Fullwidth Capital B
0xa3e3  0xa3c3  ;Fullwidth Small C -> Fullwidth Capital C
0xa3e4  0xa3c4  ;Fullwidth Small D -> Fullwidth Capital D
0xa3e5  0xa3c5  ;Fullwidth Small E -> Fullwidth Capital E
0xa3e6  0xa3c6  ;Fullwidth Small F -> Fullwidth Capital F
0xa3e7  0xa3c7  ;Fullwidth Small G -> Fullwidth Capital G
0xa3e8  0xa3c8  ;Fullwidth Small H -> Fullwidth Capital H
0xa3e9  0xa3c9  ;Fullwidth Small I -> Fullwidth Capital I
0xa3ea  0xa3ca  ;Fullwidth Small J -> Fullwidth Capital J
0xa3eb  0xa3cb  ;Fullwidth Small K -> Fullwidth Capital K
0xa3ec  0xa3cc  ;Fullwidth Small L -> Fullwidth Capital L
0xa3ed  0xa3cd  ;Fullwidth Small M -> Fullwidth Capital M
0xa3ee  0xa3ce  ;Fullwidth Small N -> Fullwidth Capital N
0xa3ef  0xa3cf  ;Fullwidth Small O -> Fullwidth Capital O
0xa3f0  0xa3d0  ;Fullwidth Small P -> Fullwidth Capital P
0xa3f1  0xa3d1  ;Fullwidth Small Q -> Fullwidth Capital Q
0xa3f2  0xa3d2  ;Fullwidth Small R -> Fullwidth Capital R
0xa3f3  0xa3d3  ;Fullwidth Small S -> Fullwidth Capital S
0xa3f4  0xa3d4  ;Fullwidth Small T -> Fullwidth Capital T
0xa3f5  0xa3d5  ;Fullwidth Small U -> Fullwidth Capital U
0xa3f6  0xa3d6  ;Fullwidth Small V -> Fullwidth Capital V
0xa3f7  0xa3d7  ;Fullwidth Small W -> Fullwidth Capital W
0xa3f8  0xa3d8  ;Fullwidth Small X -> Fullwidth Capital X
0xa3f9  0xa3d9  ;Fullwidth Small Y -> Fullwidth Capital Y
0xa3fa  0xa3da  ;Fullwidth Small Z -> Fullwidth Capital Z


*** Code Page 950 ***

0xa2af  ;Fullwidth Digit Zero
0xa2b0  ;Fullwidth Digit One
0xa2b1  ;Fullwidth Digit Two
0xa2b2  ;Fullwidth Digit Three
0xa2b3  ;Fullwidth Digit Four
0xa2b4  ;Fullwidth Digit Five
0xa2b5  ;Fullwidth Digit Six
0xa2b6  ;Fullwidth Digit Seven
0xa2b7  ;Fullwidth Digit Eight
0xa2b8  ;Fullwidth Digit Nine

0xa2e9  0xa2cf  ;Fullwidth Small A -> Fullwidth Capital A
0xa2ea  0xa2d0  ;Fullwidth Small B -> Fullwidth Capital B
0xa2eb  0xa2d1  ;Fullwidth Small C -> Fullwidth Capital C
0xa2ec  0xa2d2  ;Fullwidth Small D -> Fullwidth Capital D
0xa2ed  0xa2d3  ;Fullwidth Small E -> Fullwidth Capital E
0xa2ee  0xa2d4  ;Fullwidth Small F -> Fullwidth Capital F
0xa2ef  0xa2d5  ;Fullwidth Small G -> Fullwidth Capital G
0xa2f0  0xa2d6  ;Fullwidth Small H -> Fullwidth Capital H
0xa2f1  0xa2d7  ;Fullwidth Small I -> Fullwidth Capital I
0xa2f2  0xa2d8  ;Fullwidth Small J -> Fullwidth Capital J
0xa2f3  0xa2d9  ;Fullwidth Small K -> Fullwidth Capital K
0xa2f4  0xa2da  ;Fullwidth Small L -> Fullwidth Capital L
0xa2f5  0xa2db  ;Fullwidth Small M -> Fullwidth Capital M
0xa2f6  0xa2dc  ;Fullwidth Small N -> Fullwidth Capital N
0xa2f7  0xa2dd  ;Fullwidth Small O -> Fullwidth Capital O
0xa2f8  0xa2de  ;Fullwidth Small P -> Fullwidth Capital P
0xa2f9  0xa2df  ;Fullwidth Small Q -> Fullwidth Capital Q
0xa2fa  0xa2e0  ;Fullwidth Small R -> Fullwidth Capital R
0xa2fb  0xa2e1  ;Fullwidth Small S -> Fullwidth Capital S
0xa2fc  0xa2e2  ;Fullwidth Small T -> Fullwidth Capital T
0xa2fd  0xa2e3  ;Fullwidth Small U -> Fullwidth Capital U
0xa2fe  0xa2e4  ;Fullwidth Small V -> Fullwidth Capital V

...Note break in sequence...

0xa340  0xa2e5  ;Fullwidth Small W -> Fullwidth Capital W
0xa341  0xa2e6  ;Fullwidth Small X -> Fullwidth Capital X
0xa342  0xa2e7  ;Fullwidth Small Y -> Fullwidth Capital Y
0xa343  0xa2e8  ;Fullwidth Small Z -> Fullwidth Capital Z


*** Code Page 1361 ***

Not yet available (05/17/94)



****************************************************************************/
