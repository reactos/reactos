#ifndef _WIN32K_USERATOM_H
#define _WIN32K_USERATOM_H

#include <windows.h>
#include <ddk/ntapi.h>

RTL_ATOM FASTCALL
IntAddAtom(LPWSTR AtomName);
ULONG FASTCALL
IntGetAtomName(RTL_ATOM nAtom, LPWSTR lpBuffer, ULONG nSize);

#endif /* _WIN32K_USERATOM_H */
