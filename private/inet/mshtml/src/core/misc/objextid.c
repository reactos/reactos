/*
   Please do not use any single line comments before the inclusion of w4warn.h!
*/

#define INITGUID
#include <w4warn.h>
#ifndef _MAC
#include <windef.h>
#include <basetyps.h>
#endif

// Include the object extensions GUID's
//
// These weird stuff with windows.h is necessary because it reenables warning
// 4001 and generally causes havoc at warning level 4.
//
#pragma warning(disable:4115) // named type in parenthses

#include <windows.h>
#include <w4warn.h>
#include <servprov.h>
#include <ole2.h>
#include <objext.h>
