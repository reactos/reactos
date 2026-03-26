/*
 * PROJECT:     GCC C++ support library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     _fseeki64/_ftelli64 shims for GCC 15 libstdc++
 * COPYRIGHT:   Copyright 2026 ReactOS Team
 *
 * GCC 15's libstdc++ ext11-inst.o references _fseeki64 and _ftelli64 as
 * DLL imports. _fseeki64 is version-gated to >= 0x600; _ftelli64 is not
 * exported from msvcrt at all. Delegate to fseek/ftell (32-bit offset).
 */

/*
 * Avoid including <stdio.h> — the CRT headers declare _fseeki64/_ftelli64
 * with __declspec(dllimport), which conflicts with our local definitions.
 */
typedef struct _iobuf FILE;
int __cdecl fseek(FILE *, long, int);
long __cdecl ftell(FILE *);

int __cdecl _fseeki64(FILE *stream, long long offset, int origin)
{
    return fseek(stream, (long)offset, origin);
}

long long __cdecl _ftelli64(FILE *stream)
{
    return (long long)ftell(stream);
}

#ifdef _M_IX86
void *_imp___fseeki64 = _fseeki64;
void *_imp___ftelli64 = _ftelli64;
#else
void *__imp__fseeki64 = _fseeki64;
void *__imp__ftelli64 = _ftelli64;
#endif
