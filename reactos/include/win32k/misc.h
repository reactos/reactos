#ifndef __WIN32K_MISC_H
#define __WIN32K_MISC_H

BOOLEAN
STDCALL
Win32kInitialize (VOID);

VOID
FASTCALL
DestroyThreadWindows(struct _ETHREAD *Thread);

#endif /* __WIN32K_MISC_H */
