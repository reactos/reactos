/***
*initctype.cpp - contains __acrt_locale_initialize_ctype
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Contains the locale-category initialization function: __acrt_locale_initialize_ctype().
*
*       Each initialization function sets up locale-specific information
*       for their category, for use by functions which are affected by
*       their locale category.
*
*       *** For internal use by setlocale() only ***
*
*******************************************************************************/
#include <corecrt_internal.h>
#include <locale.h>
#include <stdlib.h>

#define _CTABSIZE   257     /* size of ctype tables */

// Enclaves have no ability to create new locales.
#ifndef _UCRT_ENCLAVE_BUILD

/***
*int __acrt_locale_initialize_ctype() - initialization for LC_CTYPE locale category.
*
*Purpose:
*       In non-C locales, preread ctype tables for chars and wide-chars.
*       Old tables are freed when new tables are fully established, else
*       the old tables remain intact (as if original state unaltered).
*       The leadbyte table is implemented as the high bit in ctype1.
*
*       In the C locale, ctype tables are freed, and pointers point to
*       the static ctype table.
*
*       Tables contain 257 entries: -1 to 256.
*       Table pointers point to entry 0 (to allow index -1).
*
*Entry:
*       None.
*
*Exit:
*       0 success
*       1 fail
*
*Exceptions:
*
*******************************************************************************/

int __cdecl __acrt_locale_initialize_ctype (
        __crt_locale_data* ploci
        )
{
    /* non-C locale table for char's */
    unsigned short *newctype1 = nullptr;          /* temp new table */
    unsigned char *newclmap = nullptr;            /* temp new map table */
    unsigned char *newcumap = nullptr;            /* temp new map table */

    /* non-C locale table for wchar_t's */

    unsigned char *cbuffer = nullptr;      /* char working buffer */

    int i;                              /* general purpose counter */
    unsigned char *cp;                  /* char pointer */
    CPINFO cpInfo;                      /* struct for use with GetCPInfo */
    int mb_cur_max;
    __crt_locale_pointers locinfo;

    locinfo.locinfo = ploci;
    locinfo.mbcinfo = 0;

    /* allocate and set up buffers before destroying old ones */
    /* codepage will be restored by setlocale if error */

    if (ploci->locale_name[LC_CTYPE] != nullptr)
    {
        long* refcount = nullptr;

        if (ploci->_public._locale_lc_codepage == 0)
        { /* code page was not specified */
            if ( __acrt_GetLocaleInfoA( &locinfo, LC_INT_TYPE,
                                  ploci->locale_name[LC_CTYPE],
                                  LOCALE_IDEFAULTANSICODEPAGE,
                                  (char **)&ploci->_public._locale_lc_codepage ) )
                goto error_cleanup;
        }

        /* allocate a new (thread) reference counter */
        refcount = _calloc_crt_t(long, 1).detach();

        /* allocate new buffers for tables */
        newctype1 = _calloc_crt_t(unsigned short, _COFFSET + _CTABSIZE).detach();
        newclmap  = _calloc_crt_t(unsigned char,  _COFFSET + _CTABSIZE).detach();
        newcumap  = _calloc_crt_t(unsigned char,  _COFFSET + _CTABSIZE).detach();
        cbuffer   = _calloc_crt_t(unsigned char,  _CTABSIZE).detach();

        if (!refcount || !newctype1 || !cbuffer || !newclmap || !newcumap)
            goto error_cleanup;

        /* construct string composed of first 256 chars in sequence */
        for (cp=cbuffer, i=0; i<_CTABSIZE-1; i++)
            *cp++ = (unsigned char)i;

        if (GetCPInfo( ploci->_public._locale_lc_codepage, &cpInfo) == FALSE)
            goto error_cleanup;

        if (cpInfo.MaxCharSize > MB_LEN_MAX)
            goto error_cleanup;

        mb_cur_max = (unsigned short) cpInfo.MaxCharSize;

        /* zero (space actually) out leadbytes so GetStringType and
           LCMapStringA don't interpret them as multi-byte chars */
        if (mb_cur_max > 1)
        {
            if (ploci->_public._locale_lc_codepage == CP_UTF8)
            {
                // For UTF-8 anything above 0x7f is part of a multibyte sequence and
                // would confuse the codepage/character code below.
                for (i = 0x80; i <= 0xff; i++)
                {
                    // spaces are safe.
                    cbuffer[i] = ' ';
                }
            }
            else
            {
                // use the lead byte table to mark off the appropriate bytes
                for (cp = (unsigned char *)cpInfo.LeadByte; cp[0] && cp[1]; cp += 2)
                {
                    for (i = cp[0]; i <= cp[1]; i++)
                        cbuffer[i] = ' ';
                }
            }
        }

        /*
         * LCMapString will map past nullptr. Must find nullptr if in string
         * before cchSrc characters.
         */
        if ( __acrt_LCMapStringA(nullptr,
                    ploci->locale_name[LC_CTYPE],
                    LCMAP_LOWERCASE,
                    reinterpret_cast<char*>(cbuffer + 1),
                    _CTABSIZE-2,
                    reinterpret_cast<char*>(newclmap + 2 + _COFFSET),
                    _CTABSIZE-2,
                    ploci->_public._locale_lc_codepage,
                    FALSE ) == FALSE)
            goto error_cleanup;

        if ( __acrt_LCMapStringA(nullptr,
                    ploci->locale_name[LC_CTYPE],
                    LCMAP_UPPERCASE,
                    reinterpret_cast<char*>(cbuffer + 1),
                    _CTABSIZE-2,
                    reinterpret_cast<char*>(newcumap + 2 + _COFFSET),
                    _CTABSIZE-2,
                    ploci->_public._locale_lc_codepage,
                    FALSE ) == FALSE)
            goto error_cleanup;

        /* convert to newctype1 table - ignore invalid char errors */
        if ( __acrt_GetStringTypeA(nullptr,  CT_CTYPE1,
                                  reinterpret_cast<char*>(cbuffer),
                                  _CTABSIZE-1,
                                  newctype1+1+_COFFSET,
                                  ploci->_public._locale_lc_codepage,
                                  FALSE ) == FALSE )
            goto error_cleanup;

        newctype1[_COFFSET] = 0; /* entry for EOF */
        newclmap[_COFFSET] = 0;
        newcumap[_COFFSET] = 0;
        newclmap[_COFFSET+1] = 0; /* entry for null */
        newcumap[_COFFSET+1] = 0; /* entry for null */

        /* ignore DefaultChar */

        /* mark lead-byte entries in newctype1 table and
           restore original values for lead-byte entries for clmap/cumap */
        if (mb_cur_max > 1)
        {
            if (ploci->_public._locale_lc_codepage == CP_UTF8)
            {
                // For UTF-8 anything above 0x7f is part of a multibyte sequence
                // "real" leadbytes start with C0 and end at F7
                // However, C0 & C1 are overlong encoded ASCII, F5 & F6 would be > U+10FFFF.
                // Note that some starting with E0 and F0 are overlong and not legal though.
                for (i = 0x80; i <= 0xff; i++)
                {
                    newctype1[_COFFSET + i + 1] = (i >= 0xc2 && i < 0xf5) ? _LEADBYTE : 0;
                    newclmap[_COFFSET + i + 1] = static_cast<unsigned char>(i);
                    newcumap[_COFFSET + i + 1] = static_cast<unsigned char>(i);
                }
            }
            else
            {
                for (cp = (unsigned char *)cpInfo.LeadByte; cp[0] && cp[1]; cp += 2)
                {
                    for (i = cp[0]; i <= cp[1]; i++)
                    {
                        newctype1[_COFFSET + i + 1] = _LEADBYTE;
                        newclmap[_COFFSET + i + 1] = static_cast<unsigned char>(i);
                        newcumap[_COFFSET + i + 1] = static_cast<unsigned char>(i);
                    }
                }
            }
        }
        /* copy last-1 _COFFSET unsigned short to front
         * note -1, we don't really want to copy 0xff
         */
        memcpy(newctype1,newctype1+_CTABSIZE-1,_COFFSET*sizeof(unsigned short));
        memcpy(newclmap,newclmap+_CTABSIZE-1,_COFFSET*sizeof(char));
        memcpy(newcumap,newcumap+_CTABSIZE-1,_COFFSET*sizeof(char));

        /* free old tables */
        if ((ploci->ctype1_refcount) &&
            (InterlockedDecrement(ploci->ctype1_refcount) == 0))
        {
            _ASSERT(0);
            _free_crt(ploci->ctype1 - _COFFSET);
            _free_crt((char *)(ploci->pclmap - _COFFSET - 1));
            _free_crt((char *)(ploci->pcumap - _COFFSET - 1));
            _free_crt(ploci->ctype1_refcount);
        }
        (*refcount) = 1;
        ploci->ctype1_refcount = refcount;
        /* set pointers to point to entry 0 of tables */
        ploci->_public._locale_pctype     = newctype1 + 1 + _COFFSET;
        ploci->ctype1                     = newctype1 +     _COFFSET;
        ploci->pclmap                     = newclmap  + 1 + _COFFSET;
        ploci->pcumap                     = newcumap  + 1 + _COFFSET;
        ploci->_public._locale_mb_cur_max = mb_cur_max;

        /* cleanup and return success */
        _free_crt (cbuffer);
        return 0;

error_cleanup:
        _free_crt (refcount);
        _free_crt (newctype1);
        _free_crt (newclmap);
        _free_crt (newcumap);
        _free_crt (cbuffer);
        return 1;

    } else {

        if ( (ploci->ctype1_refcount != nullptr)&&
             (InterlockedDecrement(ploci->ctype1_refcount) == 0))
        {
            _ASSERTE(*ploci->ctype1_refcount > 0);
        }
        ploci->ctype1_refcount        = nullptr;
        ploci->ctype1                 = nullptr;
        ploci->_public._locale_pctype = __newctype + 1 + _COFFSET;
        ploci->pclmap                 = __newclmap + 1 + _COFFSET;
        ploci->pcumap                 = __newcumap + 1 + _COFFSET;
        ploci->_public._locale_mb_cur_max = 1;

        return 0;
    }
}

#endif /* _UCRT_ENCLAVE_BUILD */

/* Define a number of functions which exist so, under _STATIC_CPPLIB, the
 * static multithread C++ Library libcpmt.lib can access data found in the
 * main CRT DLL without using __declspec(dllimport).
 */

int __cdecl ___mb_cur_max_func()
{
    /*
     * Note that we don't need _LocaleUpdate in this function.
     * The main reason being, that this is a leaf function in
     * locale usage terms.
     */
    __acrt_ptd* const ptd = __acrt_getptd();
    __crt_locale_data* ptloci = ptd->_locale_info;

    __acrt_update_locale_info(ptd, &ptloci);

    return ptloci->_public._locale_mb_cur_max;
}

int __cdecl ___mb_cur_max_l_func(_locale_t locale)
{
    return locale == nullptr
        ? ___mb_cur_max_func()
        : locale->locinfo->_public._locale_mb_cur_max;
}

unsigned int __cdecl ___lc_codepage_func()
{
    /*
     * Note that we don't need _LocaleUpdate in this function.
     * The main reason being, that this is a leaf function in
     * locale usage terms.
     */
    __acrt_ptd* const ptd = __acrt_getptd();
    __crt_locale_data* ptloci = ptd->_locale_info;

    __acrt_update_locale_info(ptd, &ptloci);

    return ptloci->_public._locale_lc_codepage;
}

unsigned int __cdecl ___lc_collate_cp_func()
{
    /*
     * Note that we don't need _LocaleUpdate in this function.
     * The main reason being, that this is a leaf function in
     * locale usage terms.
     */
    __acrt_ptd* const ptd = __acrt_getptd();
    __crt_locale_data* ptloci = ptd->_locale_info;

    __acrt_update_locale_info(ptd, &ptloci);

    return ptloci->lc_collate_cp;
}

wchar_t** __cdecl ___lc_locale_name_func()
{
    /*
     * Note that we don't need _LocaleUpdate in this function.
     * The main reason being, that this is a leaf function in
     * locale usage terms.
     */
    __acrt_ptd* const ptd = __acrt_getptd();
    __crt_locale_data* ptloci = ptd->_locale_info;

    __acrt_update_locale_info(ptd, &ptloci);

    return ptloci->locale_name;
}
