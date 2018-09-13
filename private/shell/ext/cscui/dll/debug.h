//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       debug.h
//
//--------------------------------------------------------------------------

#ifndef __debug_h
#define __debug_h

/*-----------------------------------------------------------------------------
/ Debugging APIs (use the Macros, they make it easier and cope with correctly
/ removing debugging when it is disabled at build time).
/----------------------------------------------------------------------------*/
void DoTraceSetMask(DWORD dwMask);
void DoTraceSetMaskFromRegKey(HKEY hkRoot, LPCTSTR pszSubKey);
void DoTraceSetMaskFromCLSID(REFCLSID rCLSID);
void DoTraceEnter(DWORD dwMask, LPCTSTR pName);
void DoTraceLeave(void);
void DoTrace(LPCTSTR pFormat, ...);
void DoTraceGUID(LPCTSTR pPrefix, REFGUID rGUID);
void DoTraceAssert(int iLine, LPTSTR pFilename, LPTSTR pCondition);


/*-----------------------------------------------------------------------------
/ Macros to ease the use of the debugging APIS.
/----------------------------------------------------------------------------*/

#ifdef DEBUG

void DebugThreadDetach(void);           // optional
void DebugProcessAttach(void);          // required
void DebugProcessDetach(void);          // required

#define TraceSetMask(dwMask)            DoTraceSetMask(dwMask)
#define TraceSetMaskFromRegKey(hk, psz) DoTraceSetMaskFromRegKey(hk, psz)
#define TraceSetMaskFromCLSID(rCLSID)   DoTraceSetMaskFromCLSID(rCLSID)
#define TraceEnter(dwMask, fn)          DoTraceEnter(dwMask, TEXT(fn))
#define TraceLeave                      DoTraceLeave

#define Trace(x)                        DoTrace x
#define TraceMsg(s)                     DoTrace(TEXT(s))
#define TraceGUID(s, rGUID)             DoTraceGUID(TEXT(s), rGUID)

#define TraceAssert(x) \
                { if ( !(x) ) DoTraceAssert(__LINE__, TEXT(__FILE__), TEXT(#x)); }

#define TraceLeaveResult(hr) \
                { HRESULT __hr = (hr); if (FAILED(__hr)) Trace((TEXT("Failed (%08x)"), __hr)); TraceLeave(); return __hr; }

#define TraceLeaveVoid() \
                { TraceLeave(); return; }

#define TraceLeaveValue(value) \
                { TraceLeave(); return (value); }

#else   // !DEBUG

#define DebugThreadDetach()
#define DebugProcessAttach()
#define DebugProcessDetach()

#define TraceSetMask(dwMask)
#define TraceSetMaskFromRegKey(hk, psz)
#define TraceSetMaskFromCLSID(rCLSID)
#define TraceEnter(dwMask, fn)
#define TraceLeave()

#define Trace(x)
#define TraceMsg(s)
#define TraceGUID(s, rGUID)

#define TraceAssert(x)
#define TraceLeaveResult(hr)    { return (hr); }
#define TraceLeaveVoid()        { return; }
#define TraceLeaveValue(value)  { return (value); }

#endif  // DEBUG


#endif  // __debug_h
