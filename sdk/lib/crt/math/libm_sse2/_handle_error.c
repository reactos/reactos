/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _handle_error / _handle_errorf for libm
 * COPYRIGHT:   Copyright 2022 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <math.h>

int
__cdecl
_invoke_matherr(
    int type,
    char* name,
    double arg1,
    double arg2,
    double retval);

/*!
 * @brief Handles an error condition. 
 * @param fname - The name of the function that caused the error.
 * @param opcode - Opcode of the function that cause the error (see OP_* consants in fpieee.h).
 * @param value - The value to be returned, encoded as uint64_t.
 * @param type - The type of error (see _DOMAIN, ... in math.h)
 * @param flags - Exception flags (see AMD_F_* constants).
 * @param error - Specifies the CRT error code (EDOM, ...).
 * @param arg1 - First parameter to the function that cause the error.
 * @param arg2 - Second parameter to the function that cause the error.
 * @param nargs - Number of parameters to the function that cause the error.
 * @return The value to be returned.
 */
double
__cdecl
_handle_error(
    char *fname,
    int opcode,
    unsigned long long value,
    int type,
    int flags,
    int error,
    double arg1,
    double arg2,
    int nargs)
{
    float retval = *(double*)&value;

    _invoke_matherr(type, fname, arg1, arg2, retval);

    return retval;
}



float
__cdecl
_handle_errorf(
    char *fname,
    int opcode,
    unsigned long long value,
    int type,
    int flags,
    int error,
    float arg1,
    float arg2,
    int nargs)
{
    float retval = *(float*)&value;

    _invoke_matherr(type, fname, arg1, arg2, retval);

    return retval;
}
