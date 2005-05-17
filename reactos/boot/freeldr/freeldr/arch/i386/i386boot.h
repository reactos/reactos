#ifndef I386BOOT_H_INCLUDED
#define I386BOOT_H_INCLUDED

VOID
STDCALL
i386BootStartup(ULONG Magic);

VOID
FASTCALL
i386BootSetupPae(BOOLEAN PaeModeEnabled, ULONG Magic);

BOOLEAN
FASTCALL
i386BootGetPaeMode(VOID);

VOID
FASTCALL
i386BootSetupPageDirectory(BOOLEAN PaeModeEnabled);

#endif /* ! defined I386BOOT_H_INCLUDED */
