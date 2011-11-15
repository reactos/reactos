#ifndef _NTDLL_H
#define _NTDLL_H

typedef enum _SHUTDOWN_ACTION {
     ShutdownNoReboot,
     ShutdownReboot,
     ShutdownPowerOff
 } SHUTDOWN_ACTION;

DWORD WINAPI NtShutdownSystem (SHUTDOWN_ACTION Action);

#endif /* _NTDLL_H */
