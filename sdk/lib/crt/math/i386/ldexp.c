/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implements the ldexp CRT function for IA-32 with Windows-compatible error codes.
 * COPYRIGHT:   Copyright 2010 Timo Kreuzer (timo.kreuzer@reactos.org)
 *              Copyright 2011 Pierre Schweitzer (pierre@reactos.org)
 *              Copyright 2019 Colin Finck (colin@reactos.org)
 */

#include <precomp.h>

double ldexp (double value, int exp)
{
#ifdef __GNUC__
    register double result;
#endif

    /* Check for value correctness
     * and set errno if required
     */
    if (_isnan(value))
    {
        errno = EDOM;
    }

#ifdef __GNUC__
    asm ("fscale"
         : "=t" (result)
         : "0" (value), "u" ((double)exp)
         : "1");
    return result;
#else /* !__GNUC__ */
    __asm
    {
        fild exp
        fld value
        fscale
        fstp st(1)
    }

    /* "fstp st(1)" has copied st(0) to st(1), then popped the FPU stack,
     * so that the value is again in st(0) now. Effectively, we have reduced
     * the FPU stack by one element while preserving st(0).
     * st(0) is also the register used for returning a double value. */
#endif /* !__GNUC__ */
}
