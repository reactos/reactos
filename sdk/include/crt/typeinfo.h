/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#include <corecrt.h>

#ifndef _INC_TYPEINFO
#define _INC_TYPEINFO

#pragma pack(push,_CRT_PACKING)

#ifndef RC_INVOKED

#ifndef __cplusplus
#error This header requires a C++ compiler ...
#endif

#include <typeinfo>

#ifdef __RTTI_OLDNAMES
using std::bad_cast;
using std::bad_typeid;

typedef type_info Type_info;
typedef bad_cast Bad_cast;
typedef bad_typeid Bad_typeid;
#endif
#endif

#pragma pack(pop)
#endif
