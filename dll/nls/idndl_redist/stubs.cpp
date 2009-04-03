/*
 * Copyright (c) 2008, KJK::Hyperion
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  - Neither the name of the ReactOS Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

// FIXME: move stubs elsewhere

#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void free(void * memory)
{
	HeapFree(GetProcessHeap(), 0, memory);
}

void * malloc(size_t size)
{
	return HeapAlloc(GetProcessHeap(), 0, size);
}

void * realloc(void * memory, size_t size)
{
	return HeapReAlloc(GetProcessHeap(), 0, memory, size);
}

void operator delete(void * memory)
{
	free(memory);
}

extern "C" int __cdecl _purecall()
{
	FatalAppExitW(0, L"pure virtual call");
	FatalExit(0);
	return 0;
}

extern "C" void __cxa_pure_virtual() { _purecall(); }

extern "C" void _assert()
{
	FatalAppExitW(0, L"assertion failed");
	FatalExit(0);
}

// EOF
