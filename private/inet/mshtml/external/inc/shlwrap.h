#ifndef I_SHLWRAP_H_
#define I_SHLWRAP_H_

#if defined(_M_IX86) && !defined(WINCE)

#define UNICODE_SHDOCVW
#define POST_IE5_BETA

#ifndef X_W95WRAPS_H_
#define X_W95WRAPS_H_
#include <w95wraps.h>
#endif

// Trident doesn't want certain system functions wrapped

#undef TextOutW
#undef ExtTextOutW

#else

// Manually enable wrapping for certain APIs

// (JBEDA, via DINARTEM)  We may want to take this ifdef out entirely so
// that we always use shlwapi on Alpha/ia64

#define ShellExecuteExW             ShellExecuteExWrapW

#endif

#endif
