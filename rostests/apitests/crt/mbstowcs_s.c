/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for mbstowcs_s
 */

#define WIN32_NO_STATUS
#include <stdio.h>
#include <stdlib.h>
#include <wine/test.h>
#include <specstrings.h>

#define ok_errno(x) ok_hex(errno, (x))

errno_t
mbstowcs_s(
    size_t *returnval,
    wchar_t *widechar,
    size_t charoutct,
    const char *multibyte,
    size_t count);

void
_invalid_parameter(
   const wchar_t * expression,
   const wchar_t * function,
   const wchar_t * file,
   unsigned int line,
   uintptr_t pReserved);

void
_invalid_parameterA(
   const char * expression,
   const char * function,
   const char * file,
   unsigned int line,
   uintptr_t pReserved)
{
    // call _invalid_parameter
}

#define MSVCRT_INVALID_PMT(x) _invalid_parameterA(#x, __FUNCTION__, __FILE__, __LINE__, 0)
#define MSVCRT_CHECK_PMT(x)   ((x) || (MSVCRT_INVALID_PMT(x),0))

#define _mbsnlen strnlen

errno_t
mbstowcs_s(
   _Out_opt_ size_t *pReturnValue,
   _Out_ wchar_t *pwcstr,
   _In_ size_t sizeInWords,
   _In_ const char *pmbstr,
   _In_ size_t count)
{
    size_t cchMax, cwcWritten;
    errno_t retval = 0;

    if (!MSVCRT_CHECK_PMT( ((sizeInWords != 0) && (pwcstr != 0)) ||
                           ((sizeInWords == 0) && (pwcstr == 0)) ))
    {
        _set_errno(EINVAL);
        return EINVAL;
    }

    /* Check if we have a return value pointer */
    if (pReturnValue)
    {
        /* Default to 0 bytes written */
        *pReturnValue = 0;
    }

    if (!MSVCRT_CHECK_PMT((count == 0) || (pmbstr != 0)))
    {
        _set_errno(EINVAL);
        return EINVAL;
    }

    /* Check if there is anything to do */
    if ((pwcstr == 0) && (pmbstr == 0))
    {
        _set_errno(EINVAL);
        return EINVAL;
    }

    /* Check if we have a source string */
    if (pmbstr)
    {
        /* Check if we also have a wchar buffer */
        if (pwcstr)
        {
            /* Calculate the maximum the we can write */
            cchMax = (count < sizeInWords) ? count + 1 : sizeInWords;

            /* Now do the conversion */
            cwcWritten = mbstowcs(pwcstr, pmbstr, cchMax);

            /* Check if the buffer was not zero terminated */
            if (cwcWritten == cchMax)
            {
                /* Check if we reached the max size of the dest buffer */
                if (cwcWritten == sizeInWords)
                {
                    /* Does the caller allow this? */
                    if (count != _TRUNCATE)
                    {
                        /* Not allowed, truncate to 0 length */
                        pwcstr[0] = L'\0';

                        /* Return error */
                        _set_errno(ERANGE);
                        return ERANGE;
                    }

                    /* Inform the caller about truncation */
                    retval = STRUNCATE;
                }

                /* zero teminate the buffer */
                pwcstr[cwcWritten - 1] = L'\0';
            }
            else
            {
                /* The buffer is zero terminated, count the terminating char */
                cwcWritten++;
            }
        }
        else
        {
            /* Get the length of the string, plus 0 terminator */
            cwcWritten = _mbsnlen(pmbstr, count) + 1;
        }
    }
    else
    {
        cwcWritten = count + 1;
    }

    /* Check if we have a return value pointer */
    if (pReturnValue)
    {
        /* Default to 0 bytes written */
        *pReturnValue = cwcWritten;
    }

    return retval;
}



wchar_t g_expression[64];
wchar_t g_function[64];
wchar_t g_file[128];
unsigned int g_line;
uintptr_t g_pReserved;

void
_test_invalid_parameter(
   const wchar_t * expression,
   const wchar_t * function,
   const wchar_t * file,
   unsigned int line,
   uintptr_t pReserved)
{
    wcsncpy(g_expression, expression, sizeof(g_expression) / sizeof(wchar_t));
    wcsncpy(g_function, function, sizeof(g_function) / sizeof(wchar_t));
    wcsncpy(g_file, file, sizeof(g_file) / sizeof(wchar_t));
    g_line = line;
    g_pReserved = pReserved;
}

START_TEST(mbstowcs_s)
{
//    _set_invalid_parameter_handler(_test_invalid_parameter);

    errno_t ret;
    size_t returnval;
    wchar_t widechar[10];
#if 1
    _set_errno(0);
    returnval = 0xf00bac;
    widechar[5] = 0xFF;
    ret = mbstowcs_s(&returnval, widechar, 6, "hallo", 5);
    ok_long(ret, 0);
    ok_size_t(returnval, 6);
    ok_char(widechar[5], 0);
    ok_wstr(widechar, L"hallo");
    ok_errno(0);

    _set_errno(0);
    returnval = 0xf00bac;
    widechar[0] = 0xFF;
    ret = mbstowcs_s(&returnval, widechar, 1, "", 0);
    ok_long(ret, 0);
    ok_size_t(returnval, 1);
    ok_char(widechar[0], 0);
    ok_errno(0);

    _set_errno(0);
    returnval = 0xf00bac;
    widechar[0] = 0xFF;
    widechar[1] = 0xFF;
    widechar[2] = 0xFF;
    widechar[3] = 0xFF;
    widechar[4] = 0xFF;
    widechar[5] = 0xFF;
    ret = mbstowcs_s(&returnval, widechar, 5, "hallo", 5);
    ok_long(ret, ERANGE);
    ok_size_t(returnval, 0);
    ok_char(widechar[5], 0xFF);
    ok_char(widechar[4], L'o');
    ok_char(widechar[3], L'l');
    ok_char(widechar[2], L'l');
    ok_char(widechar[1], L'a');
    ok_char(widechar[0], 0);
    ok_errno(ERANGE);

    _set_errno(0);
    returnval = 0xf00bac;
    widechar[0] = 0xFF;
    widechar[1] = 0xFF;
    widechar[2] = 0xFF;
    widechar[3] = 0xFF;
    widechar[4] = 0xFF;
    widechar[5] = 0xFF;
    ret = mbstowcs_s(&returnval, widechar, 3, "hallo", 5);
    ok_long(ret, ERANGE);
    ok_size_t(returnval, 0);
    ok_char(widechar[5], 0xFF);
    ok_char(widechar[4], 0xFF);
    ok_char(widechar[3], 0xFF);
    ok_char(widechar[2], L'l');
    ok_char(widechar[1], L'a');
    ok_char(widechar[0], 0);
    ok_errno(ERANGE);

    _set_errno(0);
    ret = mbstowcs_s(0, 0, 0, 0, 0);
    ok_long(ret, EINVAL);
    ok_errno(EINVAL);

    _set_errno(0);
    returnval = 0xf00bac;
    ret = mbstowcs_s(&returnval, 0, 0, 0, 0);
    ok_long(ret, EINVAL);
    ok_size_t(returnval, 0);
    ok_errno(EINVAL);

    _set_errno(0);
    widechar[0] = L'x';
    ret = mbstowcs_s(0, widechar, 0, 0, 0);
    ok_long(ret, EINVAL);
    ok_char(widechar[0], L'x');
    ok_errno(EINVAL);

    _set_errno(0);
    ret = mbstowcs_s(0, widechar, 10, "hallo", 5);
    ok_long(ret, 0);
    ok_errno(0);

    _set_errno(0);
    ret = mbstowcs_s(0, widechar, 0, "hallo", 5);
    ok_long(ret, EINVAL);
    ok_errno(EINVAL);

    _set_errno(0);
    returnval = 0xf00bac;
    ret = mbstowcs_s(&returnval, 0, 10, "hallo", 5);
    ok_long(ret, EINVAL);
    ok_size_t(returnval, 0xf00bac);
    ok_errno(EINVAL);

    _set_errno(0);
    returnval = 0xf00bac;
    ret = mbstowcs_s(&returnval, 0, 0, "hallo", 5);
    ok_long(ret, 0);
    ok_size_t(returnval, 6);
    ok_errno(0);

    _set_errno(0);
    returnval = 0xf00bac;
    ret = mbstowcs_s(&returnval, widechar, 10, 0, 5);
    ok_long(ret, EINVAL);
    ok_size_t(returnval, 0);
    ok_errno(EINVAL);

    _set_errno(0);
    returnval = 0xf00bac;
    ret = mbstowcs_s(&returnval, widechar, 10, "hallo", 0);
    ok_long(ret, 0);
    ok_size_t(returnval, 1);
    ok_errno(0);
#endif
    _set_errno(0);
    returnval = 0xf00bac;
    widechar[0] = 0xABCD;
    widechar[1] = 0xABCD;
    widechar[2] = 0xABCD;
    widechar[3] = 0xABCD;
    widechar[4] = 0xABCD;
    widechar[5] = 0xABCD;
    ret = mbstowcs_s(&returnval, widechar, 10, "hallo", 2);
    ok_long(ret, 0);
    ok_size_t(returnval, 3);
    ok_char(widechar[5], 0xABCD);
    ok_char(widechar[4], 0xABCD);
    ok_char(widechar[3], 0xABCD);
    ok_char(widechar[2], 0);
    ok_char(widechar[1], L'a');
    ok_char(widechar[0], L'h');
    ok_errno(0);
#if 1
    _set_errno(0);
    returnval = 0xf00bac;
    ret = mbstowcs_s(&returnval, widechar, 10, 0, 0);
    ok_long(ret, 0);
    ok_size_t(returnval, 1);
    ok_errno(0);

    _set_errno(0);
    returnval = 0xf00bac;
    ret = mbstowcs_s(&returnval, widechar, 10, "hallo", 7);
    ok_long(ret, 0);
    ok_size_t(returnval, 6);
    ok_errno(0);

    _set_errno(0);
    returnval = 0xf00bac;
    ret = mbstowcs_s(&returnval, 0, 0, "hallo", 7);
    ok_long(ret, 0);
    ok_size_t(returnval, 6);
    ok_errno(0);

    _set_errno(0);
    returnval = 0xf00bac;
    widechar[0] = 0xABCD;
    widechar[1] = 0xABCD;
    widechar[2] = 0xABCD;
    widechar[3] = 0xABCD;
    widechar[4] = 0xABCD;
    widechar[5] = 0xABCD;
    ret = mbstowcs_s(&returnval, widechar, 5, "hallo", _TRUNCATE);
    ok_long(ret, STRUNCATE);
    ok_size_t(returnval, 5);
    ok_char(widechar[5], 0xABCD);
    ok_char(widechar[4], 0);
    ok_char(widechar[3], L'l');
    ok_char(widechar[2], L'l');
    ok_char(widechar[1], L'a');
    ok_char(widechar[0], L'h');
    ok_errno(0);

    _set_errno(0);
    returnval = 0xf00bac;
    ret = mbstowcs_s(&returnval, widechar, 10, "hallo", -1);
    ok_long(ret, 0);
    ok_size_t(returnval, 6);
    ok_errno(0);
#endif

}
