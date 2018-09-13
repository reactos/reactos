#ifndef __common_h
#define __common_h

//
// Avoid bringing in C runtime code for NO reason
//
#if defined(__cplusplus)
inline void * __cdecl operator new(unsigned int size) { return (void *)LocalAlloc(LPTR, size); }
inline void __cdecl operator delete(void *ptr) { LocalFree(ptr); }
extern "C" inline __cdecl _purecall(void) { return 0; }
#endif  // __cplusplus

#include "mddebug.h"
#include "dbmem.h"
#include "unknown.h"
#include "strings.h"

/*-----------------------------------------------------------------------------
/ Flags to control the trace output from parts of the common library
/----------------------------------------------------------------------------*/
#define TRACE_COMMON_STR       0x80000000
#define TRACE_COMMON_ASSERT    0x40000000
#define TRACE_COMMON_MEMORY    0x20000000


/*-----------------------------------------------------------------------------
/ Exit macros for macro
/   - these assume that a label "exit_gracefully:" prefixes the prolog
/     to your function
/----------------------------------------------------------------------------*/
#define ExitGracefully(hr, result, text)            \
            { MDTraceMsg(text); hr = result; goto exit_gracefully; }

#define FailGracefully(hr, text)                    \
            { if ( FAILED(hr) ) { MDTraceMsg(text); goto exit_gracefully; } }


/*-----------------------------------------------------------------------------
/ Object / memory release macros
/----------------------------------------------------------------------------*/

#define DoRelease(pInterface)                       \
        { if ( pInterface ) { pInterface->Release(); pInterface = NULL; } }

#define DoILFree(pidl)                              \
        { if (pidl) {ILFree((LPITEMIDLIST)pidl); pidl = NULL;} }


/*-----------------------------------------------------------------------------
/ String helper macros
/----------------------------------------------------------------------------*/
#define StringByteCopy(pDest, iOffset, sz)          \
        { memcpy(&(((LPBYTE)pDest)[iOffset]), sz, StringByteSize(sz)); }

#define StringByteSize(sz)                          \
        ((lstrlen(sz)+1)*SIZEOF(TCHAR))


/*-----------------------------------------------------------------------------
/ Other helpful macros
/----------------------------------------------------------------------------*/
#define ByteOffset(base, offset)                    \
        (((LPBYTE)base)+offset)

#endif
