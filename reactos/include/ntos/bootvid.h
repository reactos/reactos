#ifndef __INCLUDE_NTOS_BOOTVID_H
#define __INCLUDE_NTOS_BOOTVID_H

VOID
STDCALL
InbvAcquireDisplayOwnership(VOID);

BOOLEAN
STDCALL
InbvCheckDisplayOwnership(VOID);

BOOLEAN
STDCALL
InbvDisplayString(IN PCHAR String);

VOID
STDCALL
InbvEnableBootDriver(IN BOOLEAN Enable);

BOOLEAN
STDCALL
InbvEnableDisplayString(IN BOOLEAN Enable);

VOID
STDCALL
InbvInstallDisplayStringFilter(IN PVOID Unknown);

BOOLEAN
STDCALL
InbvIsBootDriverInstalled(VOID);

VOID
STDCALL
InbvNotifyDisplayOwnershipLost(IN PVOID Callback);

BOOLEAN
STDCALL
InbvResetDisplay(VOID);

VOID
STDCALL
InbvSetScrollRegion(IN ULONG Left,
  IN ULONG Top,
  IN ULONG Width,
  IN ULONG Height);

VOID
STDCALL
InbvSetTextColor(IN ULONG Color);

VOID
STDCALL
InbvSolidColorFill(IN ULONG Left,
  IN ULONG Top,
  IN ULONG Width,
  IN ULONG Height,
  IN ULONG Color);

VOID
STDCALL
VidCleanUp(VOID);

BOOLEAN
STDCALL
VidInitialize(VOID);

BOOLEAN
STDCALL
VidResetDisplay(VOID);

BOOLEAN
STDCALL
VidIsBootDriverInstalled(VOID);

#endif /* __INCLUDE_NTOS_BOOTVID_H */
