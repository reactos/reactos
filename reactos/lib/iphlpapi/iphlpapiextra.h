#ifndef _IPHLPAPIEXTRA_H
#define _IPHLPAPIEXTRA_H

#include <w32api.h>
/* This is here until we switch to version 2.5 of the mingw headers */
#if (__W32API_MAJOR_VERSION < 2 || __W32API_MINOR_VERSION < 5)
BOOL WINAPI
GetComputerNameExA(COMPUTER_NAME_FORMAT,LPSTR,LPDWORD);
#endif

#endif/*_IPHLPAPIEXTRA_H*/
