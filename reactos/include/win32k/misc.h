#ifndef __WIN32K_MISC_H
#define __WIN32K_MISC_H

/* Process context in which miniport driver is opened/used */
extern PEPROCESS Win32kDeviceProcess;

BOOLEAN
STDCALL
Win32kInitialize (VOID);

VOID
FASTCALL
DestroyThreadWindows(struct _ETHREAD *Thread);

#endif /* __WIN32K_MISC_H */
