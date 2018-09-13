#if !defined(AFX_DEBUG_H__8DEDB8FC_5C6D_11D1_8CCD_00C04FD918D0__INCLUDED_)
#define AFX_DEBUG_H__8DEDB8FC_5C6D_11D1_8CCD_00C04FD918D0__INCLUDED_

/////////////////////////////////////////////////////////////////////////////
// Diagnostic support

#ifdef _PSEUDO_DEBUG

#undef TRACE
#undef VERIFY
#undef ASSERT
#undef THIS_FILE
#undef TRACE0
#undef TRACE1
#undef TRACE2
#undef TRACE3


// Note: file names are still ANSI strings (filenames rarely need UNICODE)
BOOL AssertFailedLine(LPCSTR lpszFileName, int nLine);

void Trace(LPCTSTR lpszFormat, ...);

// by default, debug break is asm int 3, or a call to DebugBreak, or nothing
#if defined(_M_IX86)
#define CustomDebugBreak() _asm { int 3 }
#else
#define CustomDebugBreak() DebugBreak()
#endif

#define TRACE              ::Trace
#define THIS_FILE          __FILE__
#define ASSERT(f) \
	do \
	{ \
	if (!(f) && AssertFailedLine(THIS_FILE, __LINE__)) \
		CustomDebugBreak(); \
	} while (0) \

#define VERIFY(f)          ASSERT(f)

// The following trace macros are provided for backward compatiblity
//  (they also take a fixed number of parameters which provides
//   some amount of extra error checking)
#define TRACE0(sz)              ::Trace(_T(sz))
#define TRACE1(sz, p1)          ::Trace(_T(sz), p1)
#define TRACE2(sz, p1, p2)      ::Trace(_T(sz), p1, p2)
#define TRACE3(sz, p1, p2, p3)  ::Trace(_T(sz), p1, p2, p3)

#endif // !_PSEUDO_DEBUG


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DEBUG_H__8DEDB8FC_5C6D_11D1_8CCD_00C04FD918D0__INCLUDED_)
