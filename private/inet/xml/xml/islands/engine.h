/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
// engine.h
// Precompiled headers
#ifndef __ENGINE_H__
#define __ENGINE_H__

#include <windows.h>
#include <shlwapi.h>
#include <activscp.h>
#include <mshtml.h>
#include <objsafe.h>
#include "debug.hxx"
#include "islandshared.hxx"

#ifndef _SYS_GUID_OPERATORS_
// Some constants

#ifndef _OLE32_
inline BOOL InlineIsEqualGUID(REFGUID rguid1, REFGUID rguid2)
{
   return (
      ((PLONG) &rguid1)[0] == ((PLONG) &rguid2)[0] &&
      ((PLONG) &rguid1)[1] == ((PLONG) &rguid2)[1] &&
      ((PLONG) &rguid1)[2] == ((PLONG) &rguid2)[2] &&
      ((PLONG) &rguid1)[3] == ((PLONG) &rguid2)[3]);
}
#endif // _OLE32_
#endif


#endif //__ENGINE_H__

// end of file engine.h
