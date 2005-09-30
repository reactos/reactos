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


/* EOF */
