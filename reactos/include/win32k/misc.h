#ifndef __WIN32K_MISC_H
#define __WIN32K_MISC_H

#include <internal/ps.h>

/* Process context in which miniport driver is opened/used */
extern PEPROCESS W32kDeviceProcess;

BOOLEAN
STDCALL
W32kInitialize (VOID);

VOID
FASTCALL
DestroyThreadWindows(PETHREAD Thread);

#endif /* __WIN32K_MISC_H */
