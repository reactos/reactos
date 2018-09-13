#ifndef _ODASSERT_
#define _ODASSERT_

#ifdef __cplusplus
extern "C" {
#endif

#if DBG

#define assert(exp) { \
    if (!(exp)) { \
        LBAssert( #exp, __FILE__, __LINE__); \
    } \
}

#else

#define assert(exp)

#endif /* DBG */

#ifdef __cplusplus
} // extern "C" {
#endif

#endif /* _ODASSERT_ */
