#ifndef _WIN32K_USERATOM_H
#define _WIN32K_USERATOM_H

#include <windows.h>
#include <ddk/ntapi.h>

RTL_ATOM INTERNAL_CALL
IntAddAtom(LPWSTR AtomName);
ULONG INTERNAL_CALL
IntGetAtomName(RTL_ATOM nAtom, LPWSTR lpBuffer, ULONG nSize);

#endif /* _WIN32K_USERATOM_H */
