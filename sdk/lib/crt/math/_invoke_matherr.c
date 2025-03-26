/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _invoke_matherr and __setusermatherr
 * COPYRIGHT:   Copyright 2021 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

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
_invoke_matherr(
    int type,
    char* name,
    double arg1,
    double arg2,
    double retval)
{
    if (user_matherr != NULL)
    {
        struct _exception excpt;
        excpt.type = type;
        excpt.name = name;
        excpt.arg1 = arg1;
        excpt.arg2 = arg2;
        excpt.retval = retval;
        return user_matherr(&excpt);
    }
    else
    {
        return 0;
    }
}
