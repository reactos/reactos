//
// sys/timeb.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The _ftime() family of functions
//
#pragma once

#include <corecrt.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Types
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
struct __timeb32
{
    __time32_t     time;
    unsigned short millitm;
    short          timezone;
    short          dstflag;
};

struct __timeb64
{
    __time64_t     time;
    unsigned short millitm;
    short          timezone;
    short          dstflag;
};

#if defined(_CRT_INTERNAL_NONSTDC_NAMES) && _CRT_INTERNAL_NONSTDC_NAMES  && !defined _CRT_NO_TIME_T
    struct timeb
    {
        time_t         time;
        unsigned short millitm;
        short          timezone;
        short          dstflag;
    };
#endif



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Functions
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#ifdef _USE_32BIT_TIME_T
    #define _timeb      __timeb32
    #define _ftime      _ftime32
    #define _ftime_s    _ftime32_s
#else
    #define _timeb      __timeb64
    #define _ftime      _ftime64
    #define _ftime_s    _ftime64_s
#endif



_CRT_INSECURE_DEPRECATE(_ftime32_s)
_ACRTIMP void __cdecl _ftime32(
    _Out_ struct __timeb32* _Time
    );

_ACRTIMP errno_t __cdecl _ftime32_s(
    _Out_ struct __timeb32* _Time
    );

_CRT_INSECURE_DEPRECATE(_ftime64_s)
_ACRTIMP void __cdecl _ftime64(
    _Out_ struct __timeb64* _Time
    );

_ACRTIMP errno_t __cdecl _ftime64_s(
    _Out_ struct __timeb64* _Time
    );



#if !defined RC_INVOKED && !defined __midl && defined(_CRT_INTERNAL_NONSTDC_NAMES) && _CRT_INTERNAL_NONSTDC_NAMES && !defined _CRT_NO_TIME_T

    #ifdef _USE_32BIT_TIME_T

        static __inline void __CRTDECL ftime(struct timeb* const _Tmb)
        {
            _STATIC_ASSERT(sizeof(struct timeb) == sizeof(struct __timeb32));
            _ftime32((struct __timeb32*)_Tmb);
        }

    #else

        static __inline void __CRTDECL ftime(struct timeb* const _Tmb)
        {
            _STATIC_ASSERT(sizeof(struct timeb) == sizeof(struct __timeb64));
            _ftime64((struct __timeb64*)_Tmb);
        }

    #endif

#endif

_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
