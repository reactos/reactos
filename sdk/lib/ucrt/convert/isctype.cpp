//
// isctype.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Multibyte and debug support functionality for the character classification
// functions and macros.
//
#include <corecrt_internal.h>
#include <ctype.h>
#include <locale.h>



// Ensure that the masks used by GetCharType() match the CRT masks, so that we
// can simply reinterpret the masks.
#if _UPPER   != C1_UPPER || \
    _LOWER   != C1_LOWER || \
    _DIGIT   != C1_DIGIT || \
    _SPACE   != C1_SPACE || \
    _PUNCT   != C1_PUNCT || \
    _CONTROL != C1_CNTRL
    #error Character type masks do not agree in ctype and winnls
#endif



// The _chvalidator function is called by the character classification functions
// in the debug CRT.  This function tests the character argument to ensure that
// it is not out of range.  For performance reasons, this function is not used
// in the retail CRT.
#if defined _DEBUG

extern "C" int __cdecl _chvalidator(int const c, int const mask)
{
    _ASSERTE(c >= -1 && c <= 255);
    return _chvalidator_l(nullptr, c, mask);
}

extern "C" int __cdecl _chvalidator_l(_locale_t const locale, int const c, int const mask)
{
    _ASSERTE(c >= -1 && c <= 255);

    _LocaleUpdate locale_update(locale);

    return __acrt_locale_get_ctype_array_value(locale_update.GetLocaleT()->locinfo->_public._locale_pctype, c, mask);
}

#endif // _DEBUG



// The _isctype and _isctype_l functions provide support to the character
// classification functions and macros for two-byte multibyte characters.
// It returns nonzero or zero depending on whether the character argument
// does or does not satisfy the character class property encoded by the mask.
//
// The leadbyte and trailbyte should be packed into the integer c like so:
//     H.......|.......|.......|.......L
//         0       0   leadbyte trailbyte
//
extern "C" int __cdecl _isctype_l(int const c, int const mask, _locale_t const locale)
{
    _LocaleUpdate locale_update(locale);

    if (c >= -1 && c <= 255)
    {
        // Direct access to _locale_pctype is allowed due to bounds check.
        return locale_update.GetLocaleT()->locinfo->_public._locale_pctype[c] & mask;
    }

    size_t const buffer_count{3};

    int  buffer_length;
    char buffer[buffer_count];
    if (_isleadbyte_fast_internal(c >> 8 & 0xff, locale_update.GetLocaleT()))
    {
        buffer[0] = (c >> 8 & 0xff); // Put lead-byte at start of the string
        buffer[1] = static_cast<char>(c);
        buffer[2] = '\0';
        buffer_length = 2;
    }
    else
    {
        buffer[0] = static_cast<char>(c);
        buffer[1] = '\0';
        buffer_length = 1;
    }


    unsigned short character_type[buffer_count]{};
    if (__acrt_GetStringTypeA(
            locale_update.GetLocaleT(),
            CT_CTYPE1,
            buffer,
            buffer_length,
            character_type,
            locale_update.GetLocaleT()->locinfo->_public._locale_lc_codepage,
            TRUE
        ) == 0)
    {
        return 0;
    }

    return static_cast<int>(character_type[0] & mask);
}

extern "C" int __cdecl _isctype(int const c, int const mask)
{
    if (__acrt_locale_changed())
    {
        return _isctype_l(c, mask, nullptr);
    }

    return __acrt_locale_get_ctype_array_value(__acrt_initial_locale_data._public._locale_pctype, c, mask);
}
