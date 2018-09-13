// Dolphin Performance Team Instrumentation API

#ifndef _VC_VER_INC
#include "..\include\vcver.h"
#endif

typedef char* SZ;
typedef void* LOG;
typedef enum {
	letypeMin,
	letypeBegin = letypeMin,
	letypeEnd,
	letypeEvent,
	letypeMax
} LETYPE; // type of log event

#ifdef __cplusplus
#define C_LINKAGE extern "C"
#else
#define C_LINKAGE
#endif

#ifndef DLLSPEC
#if defined(_X86_) || defined(_MIPS_)
#ifdef __INSTRAPI_DLL__
#define DLLSPEC __declspec(dllexport)
#else
#define DLLSPEC __declspec(dllimport)
#endif
#else
#define DLLSPEC
#endif
#endif

C_LINKAGE LOG DLLSPEC _cdecl LogOpen(void);
// Open a log.  Returns 0 upon failure.  If failure, do not issue diagnostics;
// logging is simply disabled.

C_LINKAGE void DLLSPEC _cdecl LogNoteEvent(LOG log, SZ szComponent, SZ szSubComponent,
							LETYPE letype, SZ szMiscFmt, ...);
// Note some event to the log.  log may be 0, in which case nothing happens.
// szComponent, szSubComponent, letype, and szMisc describe the component,
// subcomponent, log event type, and miscellaneous description of the event
// as described above (all lowercase, please).  Note that if any of the sz*
// parameters are 0, reasonable defaults will be supplied.
//
// Note that szMiscFmt is a sprintf format string and can be followed by
// additional arguments as necessary.  It is not necessary to add a newline,
// however.

C_LINKAGE void DLLSPEC _cdecl LogClose(LOG log);
// Close the log.  log may be 0, in which case nothing happens.

// (Failure strategy: we never assert, but upon errors may silently disable
// logging.)
