//
// matherr.cpp
//
//      Copyright (c) 2024 Timo Kreuzer
//
// User math error support.
//
// SPDX-License-Identifier: MIT
//

#include <math.h>
#include <corecrt_startup.h>
#include <corecrt_internal.h>

static __crt_state_management::dual_state_global<_UserMathErrorFunctionPointer> __acrt_global_user_matherr;

//
// Declared in corecrt_internal.h
// Called from initialize_pointers()
//
extern "C"
void __cdecl __acrt_initialize_user_matherr(void* encoded_null)
{
    __acrt_global_user_matherr.initialize(
        reinterpret_cast<_UserMathErrorFunctionPointer>(encoded_null));
}

//
// Declared in corecrt_internal.h
//
extern "C"
bool __cdecl __acrt_has_user_matherr(void)
{
    _UserMathErrorFunctionPointer user_matherr =
        __crt_fast_decode_pointer(__acrt_global_user_matherr.value());
    return user_matherr != nullptr;
}

//
// Declared in corecrt_internal.h
//
extern "C"
int  __cdecl __acrt_invoke_user_matherr(struct _exception* _Exp)
{
    _UserMathErrorFunctionPointer user_matherr =
        __crt_fast_decode_pointer(__acrt_global_user_matherr.value());
    if (user_matherr != nullptr)
    {
        return user_matherr(_Exp);
    }

    return 0;
}

//
// Declared in corecrt_startup.h
//
extern "C"
void
__cdecl
__setusermatherr(
    _UserMathErrorFunctionPointer _UserMathErrorFunction)
{
    _UserMathErrorFunctionPointer encodedPtr =
        __crt_fast_encode_pointer(_UserMathErrorFunction);

    __acrt_global_user_matherr.value() = encodedPtr;
}

//
// Used by libm
//
extern "C"
int
__cdecl
_invoke_matherr(
    int type,
    char* name,
    double arg1,
    double arg2,
    double retval)
{
    struct _exception excpt;
    excpt.type = type;
    excpt.name = name;
    excpt.arg1 = arg1;
    excpt.arg2 = arg2;
    excpt.retval = retval;
    return __acrt_invoke_user_matherr(&excpt);
}
