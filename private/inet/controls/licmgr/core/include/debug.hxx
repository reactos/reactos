//+----------------------------------------------------------------------------
//  File:       debug.hxx
//
//  Synopsis:   Debug macros, classes, and helpers
//
//-----------------------------------------------------------------------------


#ifndef _DEBUG_HXX
#define _DEBUG_HXX

// Macros ---------------------------------------------------------------------
// Debug macros/routines
#ifdef  _DEBUG
#define	Assert(x)		{   \
                        if (!(x) && (1 == _CrtDbgReport(_CRT_ASSERT, __FILE__, __LINE__, NULL, #x))) \
                            _CrtDbgBreak(); \
                        }
#define AssertF(x)      {   \
                        if (1 == _CrtDbgReport(_CRT_ASSERT, __FILE__, __LINE__, NULL, #x)) \
                            _CrtDbgBreak(); \
                        }
#define	Implies(x,y)	Assert((!x)||(y))
#define Verify(x)       Assert(x)

#define Debug(x)        x

// Non-Debug macros/routines
#else
#define	Assert(x)
#define AssertF(x)
#define	Implies(x,y)
#define Verify(x)       x

#define Debug(x)

#endif


#endif  // _DEBUG_HXX
