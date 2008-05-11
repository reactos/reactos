#ifndef _WIN32K_ERROR_H
#define _WIN32K_ERROR_H

VOID FASTCALL
SetLastNtError(
  NTSTATUS Status);

VOID FASTCALL
SetLastWin32Error(
  DWORD Status);

NTSTATUS FASTCALL
GetLastNtError();

#endif /* _WIN32K_ERROR_H */

/* EOF */
