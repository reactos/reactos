////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
////////////////////////////////////////////////////////////////////

#ifndef _PLATFORM_SPECIFIC_H_
#define _PLATFORM_SPECIFIC_H_

#if defined  _X86_
typedef char                int8;
typedef short               int16;
typedef long                int32;
typedef long long           int64;

typedef unsigned char       uint8;
typedef unsigned short      uint16;
typedef unsigned long       uint32;
typedef unsigned long long  uint64;

typedef uint32              lba_t;

#elif defined _AMD64_

typedef __int8              int8;
typedef __int16             int16;
typedef __int32             int32;
typedef __int64             int64;

typedef unsigned __int8     uint8;
typedef unsigned __int16    uint16;
typedef unsigned __int32    uint32;
typedef unsigned __int64    uint64;

typedef uint32              lba_t;


#else       // Please define appropriate types here

#error !!!! You must define your types here for compilation to proceed !!!!

#endif  // if _X86_

#endif  // _PLATFORM_SPECIFIC_H_
