#ifndef __CRTFREE_H_
#define __CRTFREE_H_


//
// Code to help free modules from the bondage and tyranny of CRT libraries
//
// Include this header in a single component and #define CPP_FUNCTIONS
//

void *  __cdecl operator new(size_t nSize)
    {
    // Zero init just to save some headaches
    return((LPVOID)LocalAlloc(LPTR, nSize));
    }


void  __cdecl operator delete(void *pv)
    {
    LocalFree((HLOCAL)pv);
    }

extern "C" int __cdecl _purecall(void) {return 0;}

#endif  // __CRTFREE_H_

