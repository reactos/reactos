/****************************************************************************
*
* Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved
*
*
*
*
*
*
*
****************************************************************************/

#include "stdafx.h"

#include "trace.h"

#ifdef _DEBUG

void __cdecl Trace(LPSTR lprgchFormat, ...)
{
    char rgch[128], rgchOutput[256];
    wsprintfA(rgch, "%s\n", lprgchFormat);
#if defined(_M_IX86)
    wvsprintfA(rgchOutput, rgch, (LPSTR)(((LPSTR)&lprgchFormat) + sizeof(LPSTR)));
#else
    {
    va_list lpArgs;
    va_start(lpArgs, lprgchFormat);
    wvsprintfA(rgchOutput, rgch, lpArgs);
    va_end(lpArgs);
    }
#endif
    OutputDebugStringA(rgchOutput);
}

#endif //_DEBUG
