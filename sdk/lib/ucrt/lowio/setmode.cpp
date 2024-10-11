//
// setmode.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines _setmode(), which sets the translation mode for a file, and
// _set_fmode() and _get_fmode(), which control the global default translation
// mode.
//
#include <corecrt_internal_lowio.h>
#include <stdlib.h>



// Sets the file translation mode.  This changes the file mode to text or binary,
// depending on the mode argument.  This affects whether reads and writes on the
// file translate between CRLF and LF.  Returns the old file translation mode on
// success, or -1 on failure.
extern "C" int __cdecl _setmode(int const fh, int const mode)
{
    _VALIDATE_RETURN(mode == _O_TEXT   ||
                     mode == _O_BINARY ||
                     mode == _O_WTEXT  ||
                     mode == _O_U8TEXT ||
                     mode == _O_U16TEXT,
                     EINVAL, -1);

    _CHECK_FH_RETURN(fh, EBADF, -1);
    _VALIDATE_RETURN((fh >= 0 && (unsigned)fh < (unsigned)_nhandle), EBADF, -1);
    _VALIDATE_RETURN((_osfile(fh) & FOPEN), EBADF, -1);

    __acrt_lowio_lock_fh(fh);
    int result = -1;
    __try
    {
        if ((_osfile(fh) & FOPEN) == 0)
        {
            errno = EBADF;
            _ASSERTE(("Invalid file descriptor. File possibly closed by a different thread",0));
            __leave;
        }

        result = _setmode_nolock(fh, mode);
    }
    __finally
    {
        __acrt_lowio_unlock_fh(fh);
    }
    __endtry
    return result;
}



extern "C" int __cdecl _setmode_nolock(int const fh, int const mode)
{
    int const old_mode = _osfile(fh) & FTEXT;
    __crt_lowio_text_mode const old_textmode = _textmode(fh);

    switch (mode)
    {
    case _O_BINARY:
        _osfile(fh) &= ~FTEXT;
        break;

    case _O_TEXT:
        _osfile(fh) |= FTEXT;
        _textmode(fh) = __crt_lowio_text_mode::ansi;
        break;

    case _O_U8TEXT:
        _osfile(fh) |= FTEXT;
        _textmode(fh) = __crt_lowio_text_mode::utf8;
        break;

    case _O_U16TEXT:
    case _O_WTEXT:
        _osfile(fh) |= FTEXT;
        _textmode(fh) = __crt_lowio_text_mode::utf16le;
        break;
    }

    if (old_mode == 0)
        return _O_BINARY;

	if (old_textmode == __crt_lowio_text_mode::ansi)
	{
		return _O_TEXT;
	}
	else if (old_textmode == __crt_lowio_text_mode::utf8)
	{
		return _O_U8TEXT;
	}

    return _O_WTEXT;
}



extern "C" errno_t __cdecl _set_fmode(int const mode)
{
    _VALIDATE_RETURN_ERRCODE(mode == _O_TEXT || mode == _O_BINARY || mode == _O_WTEXT, EINVAL);

    _BEGIN_SECURE_CRT_DEPRECATION_DISABLE
    _InterlockedExchange(reinterpret_cast<long*>(&_fmode.value()), mode);
    _END_SECURE_CRT_DEPRECATION_DISABLE

    return 0;
}



extern "C" errno_t __cdecl _get_fmode(int* const pMode)
{
    _VALIDATE_RETURN_ERRCODE(pMode != nullptr, EINVAL);

    _BEGIN_SECURE_CRT_DEPRECATION_DISABLE
    *pMode = __crt_interlocked_read(&_fmode.value());
    _END_SECURE_CRT_DEPRECATION_DISABLE

    return 0;
}
