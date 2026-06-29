/***
*mbstok_s.c - Break string into tokens (MBCS)
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Break string into tokens (MBCS)
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal_mbstring.h>
#include <corecrt_internal_securecrt.h>

unsigned char * __cdecl _mbstok_s_l(unsigned char *_String, const unsigned char *_Control, unsigned char **_Context, _LOCALE_ARG_DECL)
{
    unsigned char *token;
    const unsigned char *ctl;
    int dbc;

    /* validation section */
    _VALIDATE_POINTER_ERROR_RETURN(_Context, EINVAL, nullptr);
    _VALIDATE_POINTER_ERROR_RETURN(_Control, EINVAL, nullptr);
    _VALIDATE_CONDITION_ERROR_RETURN(_String != nullptr || *_Context != nullptr, EINVAL, nullptr);

    _LOCALE_UPDATE;
    if (_LOCALE_SHORTCUT_TEST)
    {
        return (unsigned char*)strtok_s((char *)_String, (const char *)_Control, (char **)_Context);
    }

    /* If string==nullptr, continue with previous string */
    if (!_String)
    {
        _String = *_Context;
    }

    /* Find beginning of token (skip over leading delimiters). Note that
    * there is no token iff this loop sets string to point to the terminal null. */
    for ( ; *_String != 0; _String++)
    {
        for (ctl = _Control; *ctl != 0; ctl++)
        {
            if (_ISMBBLEAD(*ctl))
            {
                if (ctl[1] == 0)
                {
                    ctl++;
                    _SET_MBCS_ERROR;
                    break;
                }
                if (*ctl == *_String && ctl[1] == _String[1])
                {
                    break;
                }
                ctl++;
            }
            else
            {
                if (*ctl == *_String)
                {
                    break;
                }
            }
        }
        if (*ctl == 0)
        {
            break;
        }
        if (_ISMBBLEAD(*_String))
        {
            _String++;
            if (*_String == 0)
            {
                _SET_MBCS_ERROR;
                break;
            }
        }
    }

    token = _String;

    /* Find the end of the token. If it is not the end of the string,
    * put a null there. */
    for ( ; *_String != 0; _String++)
    {
        for (ctl = _Control, dbc = 0; *ctl != 0; ctl++)
        {
            if (_ISMBBLEAD(*ctl))
            {
                if (ctl[1] == 0)
                {
                    ctl++;
                    break;
                }
                if (ctl[0] == _String[0] && ctl[1] == _String[1])
                {
                    dbc = 1;
                    break;
                }
                ctl++;
            }
            else
            {
                if (*ctl == *_String)
                {
                    break;
                }
            }
        }
        if (*ctl != 0)
        {
            *_String++ = 0;
            if (dbc)
            {
                *_String++ = 0;
            }
            break;
        }
        if (_ISMBBLEAD(_String[0]))
        {
            if (_String[1] == 0)
            {
                *_String = 0;
                break;
            }
            _String++;
        }
    }

    /* Update the context */
    *_Context = _String;

    /* Determine if a token has been found. */
    if (token == _String)
    {
        return nullptr;
    }
    else
    {
        return token;
    }
}

_REDIRECT_TO_L_VERSION_3(unsigned char *, _mbstok_s, unsigned char *, const unsigned char *, unsigned char **)
