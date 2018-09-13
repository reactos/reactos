/*
   dlayidpr.c

   Please do not use any single line comments before the inclusion of w4warn.h!
*/

#define INITGUID

#define INC_OLE2


#include <w4warn.h>
#ifndef WIN16
#include <windef.h>
#include <basetyps.h>
#else
#include <windows.h>
#endif

#define PUBLIC_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)

#ifdef _MAC
#define _tagVARIANT_DEFINED
#define FACILITY_WINDOWS
#endif

