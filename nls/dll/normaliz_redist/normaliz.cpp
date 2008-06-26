// FIXME: clean up this mess

#ifndef _DEBUG
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

#endif

// EOF
