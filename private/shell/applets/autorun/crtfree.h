// -----------------------------------------------------------------------
//  REMOVES THE BURDEN OF THE CRTs
// -----------------------------------------------------------------------
#pragma once

#pragma intrinsic(memcpy)
#pragma intrinsic(memcmp)
#pragma intrinsic(memset)

void * __cdecl operator new(UINT nSize)
{
    return (LPVOID)GlobalAlloc(GPTR, nSize);
}

void __cdecl operator delete(LPVOID object)
{
    GlobalFree((HGLOBAL)object);
}

extern "C" int __cdecl _purecall(void) {return 0;}
// -----------------------------------------------------------------------