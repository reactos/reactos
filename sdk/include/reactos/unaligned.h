/*
 * PROJECT:     ReactOS SDK
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Unaligened access helper functions
 * COPYRIGHT:   Copyright 2024 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_M_IX86) || defined(_M_AMD64)
#define _UNALIGNED_ACCESS_ALLOWED 1
#else // defined(_M_IX86) || defined(_M_AMD64)
#define _UNALIGNED_ACCESS_ALLOWED 0
#endif // defined(_M_IX86) || defined(_M_AMD64)

#if _UNALIGNED_ACCESS_ALLOWED

__forceinline
unsigned short
ReadUnalignedU16(const unsigned short* p)
{
    return *p;
}

__forceinline
unsigned long
ReadUnalignedU32(const unsigned long* p)
{
    return *p;
}

__forceinline
unsigned long long
ReadUnalignedU64(const unsigned long long* p)
{
    return *p;
}

__forceinline
void
WriteUnalignedU16(unsigned short* p, unsigned short val)
{
    *p = val;
}

__forceinline
void
WriteUnalignedU32(unsigned long* p, unsigned long val)
{
    *p = val;
}

__forceinline
void
WriteUnalignedU64(unsigned long long* p, unsigned long long val)
{
    *p = val;
}

#else // !UNALIGNED_ACCESS_ALLOWED

__forceinline
unsigned short
ReadUnalignedU16(const unsigned short* p)
{
    unsigned char* p1 = (unsigned char*)p;
    return (unsigned short)(p1[0] | (p1[1] << 8));
}

__forceinline
unsigned long
ReadUnalignedU32(const unsigned long* p)
{
    unsigned char* p1 = (unsigned char*)p;
    return (((unsigned long)p1[0] << 0) |
            ((unsigned long)p1[1] << 8) |
            ((unsigned long)p1[2] << 16) |
            ((unsigned long)p1[3] << 24));
}

__forceinline
unsigned long long
ReadUnalignedU64(const unsigned long long* p)
{
    unsigned char* p1 = (unsigned char*)p;
    return (((unsigned long long)p1[0] << 0)  |
            ((unsigned long long)p1[1] << 8)  |
            ((unsigned long long)p1[2] << 16) |
            ((unsigned long long)p1[3] << 24) |
            ((unsigned long long)p1[4] << 32) |
            ((unsigned long long)p1[5] << 40) |
            ((unsigned long long)p1[6] << 48) |
         ((unsigned long long)p1[7] << 56));
}

__forceinline
void
WriteUnalignedU16(unsigned short* p, unsigned short val)
{
    unsigned char* p1 = (unsigned char*)p;
    p1[0] = (unsigned char)(val >> 0);
    p1[1] = (unsigned char)(val >> 8);
}

__forceinline
void
WriteUnalignedU32(unsigned long* p, unsigned long val)
{
    unsigned char* p1 = (unsigned char*)p;
    p1[0] = (unsigned char)(val >> 0);
    p1[1] = (unsigned char)(val >> 8);
    p1[2] = (unsigned char)(val >> 16);
    p1[3] = (unsigned char)(val >> 24);
}

__forceinline
void
WriteUnalignedU64(unsigned long long* p, unsigned long long val)
{
    unsigned char* p1 = (unsigned char*)p;
    p1[0] = (unsigned char)(val >> 0);
    p1[1] = (unsigned char)(val >> 8);
    p1[2] = (unsigned char)(val >> 16);
    p1[3] = (unsigned char)(val >> 24);
    p1[4] = (unsigned char)(val >> 32);
    p1[5] = (unsigned char)(val >> 40);
    p1[6] = (unsigned char)(val >> 48);
    p1[7] = (unsigned char)(val >> 56);
}

#endif // !UNALIGNED_ACCESS_ALLOWED

#ifdef _WIN64
#define ReadUnalignedUlongPtr ReadUnalignedU64
#define WriteUnalignedUlongPtr WriteUnalignedU64
#else // _WIN64
#define ReadUnalignedUlongPtr ReadUnalignedU32
#define WriteUnalignedUlongPtr WriteUnalignedU32
#endif // _WIN64

#ifdef __cplusplus
} // extern "C"
#endif
