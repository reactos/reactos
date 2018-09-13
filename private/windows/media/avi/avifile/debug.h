#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */

#ifdef UNICODE
//
//  Map GetProfileInt calls to the registry
//
#include "profile.h"
#endif

#ifdef DEBUG
    void CDECL dprintf0(LPSTR, ...);
    void CDECL dprintf(LPSTR, ...);
    void CDECL dprintf2(LPSTR, ...);
    void CDECL dprintf3(LPSTR, ...);
    #define DPF0 dprintf0
    #define DPF dprintf
    #define DPF2 dprintf2
    #define DPF3 dprintf3
#else
    #define DPF0 ; / ## /
    #define DPF ; / ## /
    #define DPF2 ; / ## /
    #define DPF3 ; / ## /
#endif

#undef Assert
#undef AssertSz

#ifdef DEBUG
	/* Assert() macros */
        #define AssertSz(x,sz)           ((x) ? (void)0 : (void)_Assert(sz, __FILE__, __LINE__))
        #define Assert(expr)             AssertSz(expr, #expr)

        extern void FAR PASCAL _Assert(char *szExp, char *szFile, int iLine);
#else
	/* Assert() macros */
        #define AssertSz(x, expr)           ((void)0)
        #define Assert(expr)             ((void)0)
#endif

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif	/* __cplusplus */
