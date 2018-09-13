#ifndef __debug_h
#define __debug_h




/*-----------------------------------------------------------------------------
/ Macros to ease the use of the debugging APIS.
/----------------------------------------------------------------------------*/

#if defined(DBG) || defined(DEBUG)

#ifndef DEBUG
#define DEBUG
#endif

#else

#undef  DEBUG

#endif


#ifdef DEBUG

#define MDTraceSetMask(dwMask)    MDDoTraceSetMask(dwMask)
#define MDTraceEnter(dwMask, fn)  MDDoTraceEnter(dwMask, TEXT(fn))
#define MDTraceLeave              MDDoTraceLeave

#define MDTrace                   MDDoTrace
#define MDTraceMsg(s)             MDDoTrace(TEXT(s))
#define MDTraceGUID(s, rGUID)     MDDoTraceGUID(TEXT(s), rGUID)
#define MDTraceViewMsg(m, w, l)   MDDoTraceViewMsg(m, w, l)
#define MDTraceMenuMsg(m, w, l)   MDDoTraceMenuMsg(m, w, l)


/*-----------------------------------------------------------------------------
/ Debugging APIs (use the Macros, they make it easier and cope with correctly
/ removing debugging when it is disabled at built time).
/----------------------------------------------------------------------------*/
void MDDoTraceSetMask(DWORD dwMask);
void MDDoTraceEnter(DWORD dwMask, LPCTSTR pName);
void MDDoTraceLeave(void);
void MDDoTrace(LPCTSTR pFormat, ...);
void MDDoTraceGUID(LPCTSTR pPrefix, REFGUID rGUID);
void MDDoTraceViewMsg( UINT uMsg, WPARAM wParam, LPARAM lParam );
void MDDoTraceMenuMsg( UINT uMsg, WPARAM wParam, LPARAM lParam );
void MDDoTraceAssert(int iLine, LPTSTR pFilename);


#define MDTraceAssert(x) \
                { if ( !(x) ) MDDoTraceAssert(__LINE__, TEXT(__FILE__)); }

#define MDTraceLeaveResult(hr) \
                { HRESULT __hr = hr; if (FAILED(__hr)) MDTrace(TEXT("Failed (%08x)"), hr); MDTraceLeave(); return __hr; }

#define MDTraceLeaveResultNoRet(hr) \
                { HRESULT __hr = hr; if (FAILED(__hr)) MDTrace(TEXT("Failed (%08x)"), hr); MDTraceLeave(); }

#define MDTraceLeaveVoid() \
                { MDTraceLeave(); return; }

#define MDTraceLeaveValue(value) \
                { MDTraceLeave(); return(value); }

#define MDTraceCheckResult(hr,x) \
                { HRESULT __hr = hr; if (FAILED(__hr)) MDDoTrace( TEXT("%s (hr=%08X)"), TEXT(x), hr ); }

#else

#define MDTraceSetMask(dwMask)
#define MDTraceEnter(dwMask, fn)
#define MDTraceLeave()

#define MDTrace if (FALSE)
#define MDTraceMsg(s)
#define MDTraceGUID(s, rGUID)
#define MDTraceViewMsg(m, w, l)
#define MDTraceMenuMsg(m, w, l)
#define MDTraceCheckResult(hr,x) { ; }


#define MDTraceAssert(x)
#define MDTraceLeaveResult(hr)    { return hr; }
#define MDTraceLeaveVoid()        { return; }
#define MDTraceLeaveValue(value)  { return(value); }
#define MDTraceLeaveResultNoRet(hr) { return; }
#endif


#endif
