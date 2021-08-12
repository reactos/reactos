/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of __setusermatherr and _invoke_user_matherr
 * COPYRIGHT:   Copyright 2021 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

// DO NOT SYNC WITH WINE OR MINGW32

#include <math.h>

/* MS headers have this in corecrt_startup.h */
typedef int (*_UserMathErrorFunctionPointer)(struct _exception *);

static _UserMathErrorFunctionPointer user_matherr = NULL;;

void
__cdecl
__setusermatherr(_UserMathErrorFunctionPointer func)
{
    user_matherr = func;
}

int
__cdecl
_invoke_user_matherr(struct _exception *e)
{
    if (user_matherr != NULL)
    {
        return user_matherr(e);
    }
    else
    {
        return 0;
    }
}
