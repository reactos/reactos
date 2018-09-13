//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// crt.cpp 
//
//   Functions defined to avoid a dependency on the crt lib.  Defining these
//   functions here greatly reduces code size.
//
//   History:
//
//       3/16/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//

#include "stdinc.h"

#ifndef UNIX

//
// C runtime functions.  Copied from shell\inc\crtfree.h
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** new ***
//
//   Use LocalAlloc to allocate memory via the new operator.  Note that LPTR
//   passed into LocalAlloc zero inits memory.          
//
////////////////////////////////////////////////////////////////////////////////
void*  __cdecl operator new(size_t nSize)
{
    return((LPVOID)LocalAlloc(LPTR, nSize));
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** delete ***
//
//   Corresponding delete operator for the above new.          
//
////////////////////////////////////////////////////////////////////////////////
void  __cdecl operator delete(void *pv)
{
    LocalFree((HLOCAL)pv);
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** _purecall ***
//
//   Used to remove a link dependency on _main in the CRT lib.
//
////////////////////////////////////////////////////////////////////////////////
extern "C" int __cdecl _purecall(void) {return 0;}

#endif /* !UNIX */

