#pragma once

VOID FASTCALL
SetLastNtError(
  NTSTATUS Status);

VOID FASTCALL
SetLastWin32Error(
  DWORD Status);

NTSTATUS FASTCALL
GetLastNtError(VOID);

/* EOF */
