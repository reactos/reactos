#define _AUTHZ_
#include <windows.h>
#include <authz.h>

ULONG DbgPrint(PCH Format,...);
#ifndef DPRINT1
#define DPRINT1 DbgPrint
#endif

#ifndef UNIMPLEMENTED
#define UNIMPLEMENTED DbgPrint("AUTHZ.DLL: %s is UNIMPLEMENTED!\n", __FUNCTION__)
#endif

#if DBG

#define RESMAN_TAG  0x89ABCDEF
#define VALID_RESMAN_HANDLE(handle) ASSERT(((PAUTHZ_RESMAN)handle)->Tag == RESMAN_TAG)
#ifndef ASSERT
#define ASSERT(cond) if (!(cond)) { DbgPrint("%s:%i: ASSERTION %s failed!\n", __FILE__, __LINE__, #cond ); }
#endif

#else

#define VALID_RESMAN_HANDLE(handle)
#ifndef ASSERT
#define ASSERT(cond)
#endif

#endif


/* EOF */
