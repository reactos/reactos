/*
 * Compatibility header
 */

#include <w32api.h>
#include_next <winuser.h>
#if (__W32API_MAJOR_VERSION >= 2) && (__W32API_MINOR_VERSION < 4)
#ifndef _WINUSER_COMPAT_H
#define _WINUSER_COMPAT_H
typedef LPDLGTEMPLATE LPDLGTEMPLATEA, LPDLGTEMPLATEW;
#if (WINVER >= _W2K)
#define DFCS_TRANSPARENT 0x800
#define DFCS_HOT 0x1000
#endif /* WINVER >= _W2K */
#endif
#endif
