/*****************************************************************************
 *
 *	util.cpp - Shared stuff that operates on all classes
 *
 *****************************************************************************/

#include "priv.h"
#include "util.h"



HINSTANCE g_hinst;              /* My instance handle */
#ifndef UNICODE
BSTR AllocBStrFromString(LPTSTR psz)
{
    OLECHAR wsz[INFOTIPSIZE];  // assumes INFOTIPSIZE number of chars max

    SHAnsiToUnicode(psz, wsz, ARRAYSIZE(wsz));
    return SysAllocString(wsz);

}
#endif