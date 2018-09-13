/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
// debug.hxx

#ifndef __DEBUG_HXX__
#define __DEBUG_HXX__

#ifdef _DEBUG

extern bool g_fLogDebugOutput;

#define ASSERT(X) {if (!(X)) DebugAssert(__FILE__, __LINE__);}
void DebugAssert(LPSTR szFilename, DWORD dwLine);

#define TRACE(X) {if (g_fLogDebugOutput) OutputDebugString(X);}
#define TRACEW(X) {if (g_fLogDebugOutput) OutputDebugStringW(X);}

#else // !_DEBUG

#define ASSERT(X)
#define TRACE(X)

#endif // _DEBUG

#endif // __DEBUG_HXX__