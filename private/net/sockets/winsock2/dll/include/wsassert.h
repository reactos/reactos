#ifndef _WSASSERT_
#define _WSASSERT_


//
// Define an assert that actually works.
//

#if DBG

#ifdef __cplusplus
extern "C" {
#endif

VOID
WsAssert(
    LPVOID FailedAssertion,
    LPVOID FileName,
    ULONG LineNumber
    );

#ifdef __cplusplus
}
#endif

#define WS_ASSERT(exp)      if( !(exp) )                                \
                                WsAssert( #exp, __FILE__, __LINE__ );   \
                            else

#define WS_REQUIRE(exp)     WS_ASSERT(exp)

#else

#define WS_ASSERT(exp)
#define WS_REQUIRE(exp)     ((VOID)(exp))

#endif


//
// Map lame CRT assert to our manly assert.
//

#undef assert
#define assert WS_ASSERT


#endif  // _WSASSERT_

