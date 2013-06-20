#pragma once

RTL_ATOM FASTCALL IntAddAtom(LPWSTR AtomName);
ULONG FASTCALL IntGetAtomName(RTL_ATOM nAtom, LPWSTR lpBuffer, ULONG nSize);
RTL_ATOM FASTCALL IntAddGlobalAtom(LPWSTR,BOOL);
