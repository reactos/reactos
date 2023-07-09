#ifndef __fixmfc_h
#define __fixmfc_h

#if DSUI_DEBUG || defined(_DEBUG)
void __TRACE(LPTSTR pFormat, ...);
#define TRACE if (TRUE) __TRACE
#else
#define TRACE if (FALSE) NULL
#endif

#define VERIFY(x)
#define ASSERT(x)

#endif
