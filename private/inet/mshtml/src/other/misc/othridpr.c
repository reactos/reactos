/*
   formidpr.c

   Please do not use any single line comments before the inclusion of w4warn.h!
*/

#define INITGUID
#include <w4warn.h>
#ifdef WIN16
#include <windows.h>
#else
#include <windef.h>
#endif
#include <basetyps.h>

#define PUBLIC_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)

#include <othrguid.h>
