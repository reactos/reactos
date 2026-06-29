//
// byteswap.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines functions that swap the bytes of an unsigned integer.
//
#include <stdlib.h>



#pragma function(_byteswap_ulong, _byteswap_uint64, _byteswap_ushort)

/***
*unsigned long _byteswap_ulong(i) - long byteswap
*
*Purpose:
*       Performs a byte swap on an unsigned integer.
*
*Entry:
*       unsigned long i:        value to swap
*
*Exit:
*       returns swaped
*
*Exceptions:
*       None.
*
*******************************************************************************/

extern "C" unsigned long __cdecl _byteswap_ulong(unsigned long const i)
{
    unsigned int j;
    j =  (i << 24);
    j += (i <<  8) & 0x00FF0000;
    j += (i >>  8) & 0x0000FF00;
    j += (i >> 24);
    return j;
}

extern "C" unsigned short __cdecl _byteswap_ushort(unsigned short const i)
{
    unsigned short j;
    j =  (i << 8);
    j += (i >> 8);
    return j;
}

extern "C" unsigned __int64 __cdecl _byteswap_uint64(unsigned __int64 const i)
{
    unsigned __int64 j;
    j =  (i << 56);
    j += (i << 40) & 0x00FF000000000000;
    j += (i << 24) & 0x0000FF0000000000;
    j += (i <<  8) & 0x000000FF00000000;
    j += (i >>  8) & 0x00000000FF000000;
    j += (i >> 24) & 0x0000000000FF0000;
    j += (i >> 40) & 0x000000000000FF00;
    j += (i >> 56);
    return j;

}
