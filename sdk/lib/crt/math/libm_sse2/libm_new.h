
/***********************************************************************************/
/** MIT License **/
/** ----------- **/
/** **/  
/** Copyright (c) 2002-2019 Advanced Micro Devices, Inc. **/
/** **/
/** Permission is hereby granted, free of charge, to any person obtaining a copy **/
/** of this Software and associated documentaon files (the "Software"), to deal **/
/** in the Software without restriction, including without limitation the rights **/
/** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell **/
/** copies of the Software, and to permit persons to whom the Software is **/
/** furnished to do so, subject to the following conditions: **/
/** **/ 
/** The above copyright notice and this permission notice shall be included in **/
/** all copies or substantial portions of the Software. **/
/** **/
/** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR **/
/** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, **/
/** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE **/
/** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER **/
/** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, **/
/** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN **/
/** THE SOFTWARE. **/
/***********************************************************************************/

#ifndef __LIBM_NEW_H__
#define __LIBM_NEW_H__

// Defines, protos, etc for *new* math funcs updated by AMD 11/2008
// Old files will continue to include libm_util.h, libm.h, libm_inlines.h
// until such time as these have all been refreshed w/ new versions.

typedef float F32;
typedef unsigned int U32;

typedef double F64;
typedef unsigned long long U64;

union UT32_ 
{
    F32 f32;
    U32 u32;
};

union UT64_ 
{
    F64 f64;
    U64 u64;
    
    F32 f32[2];
    U32 u32[2];
};

typedef union UT32_ UT32;
typedef union UT64_ UT64;

#define SIGN_MASK_32        0x80000000
#define MANTISSA_MASK_32    0x007fffff
#define EXPONENT_MASK_32    0x7f800000
#define QNAN_MASK_32        0x00400000

#define INF_POS_32          0x7f800000
#define INF_NEG_32          0xff800000
#define QNAN_POS_32         0x7fc00000
#define QNAN_NEG_32         0xffc00000
#define IND_32              0xffc00000

#define EXPONENT_FULL_32    0x7f800000
#define SIGN_SET_32         0x80000000
#define QNAN_SET_32         0x00400000

#define INF_POS_64          0x7ff0000000000000
#define INF_NEG_64          0xfff0000000000000

#define MANTISSA_MASK_64    0x000fffffffffffff
#define SIGN_MASK_64        0x8000000000000000
#define IND_64              0xfff8000000000000
#define QNAN_MASK_64        0x0008000000000000

// constants for 'flags' argument of _handle_error and _handle_errorf
#define AMD_F_INEXACT     0x00000010
#define AMD_F_OVERFLOW    0x00000001
#define AMD_F_UNDERFLOW   0x00000002
#define AMD_F_DIVBYZERO   0x00000004
#define AMD_F_INVALID     0x00000008

// define the Microsoft specific error handling routine

// Note to mainainers: 
// These prototypes may appear, at first glance, to differ from the versions
// declared in libm_inlines.h and defined in libm_error.c.  The third 
// parameter appears to have changed type from unsigned long to unsigned long
// long.  In fact they are the same because in both of the aforementioned 
// files, long has been #defined to __int64 in a most cowardly fashion.  This
// disgusts me.  The buck stops here. - MAS

double _handle_error(
        char *fname,
        int opcode,
        unsigned long long value,
        int type,
        int flags,
        int error,
        double arg1,
        double arg2,
        int nargs
        );
float _handle_errorf(
        char *fname,
        int opcode,
        unsigned long long value,
        int type,
        int flags,
        int error,
        float arg1,
        float arg2,
        int nargs
        );

#endif // __LIBM_NEW_H

