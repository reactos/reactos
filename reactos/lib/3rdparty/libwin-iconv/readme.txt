win_iconv is a iconv implementation using Win32 API to convert.

win_iconv is placed in the public domain.

ENVIRONMENT VARIABLE:
    WINICONV_LIBICONV_DLL
        If $WINICONV_LIBICONV_DLL is set, win_iconv uses the DLL.  If
        loading the DLL or iconv_open() failed, falls back to internal
        conversion.  If a few DLL are specified as comma separated list,
        the first loadable DLL is used.  The DLL should have
        iconv_open(), iconv_close() and iconv().  Or libiconv_open(),
        libiconv_close() and libiconv().
        (only available when USE_LIBICONV_DLL is defined at compile time)

Win32 API does not support strict encoding conversion for some codepage.
And MLang function drops or replaces invalid bytes and does not return
useful error status as iconv does.  This implementation cannot be used for
encoding validation purpose.

Yukihiro Nakadaira <yukihiro.nakadaira@gmail.com>
