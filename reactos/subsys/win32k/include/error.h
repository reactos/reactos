#ifndef _WIN32K_ERROR_H
#define _WIN32K_ERROR_H

VOID INTERNAL_CALL
SetLastNtError(
  NTSTATUS Status);

VOID INTERNAL_CALL
SetLastWin32Error(
  DWORD Status);

NTSTATUS INTERNAL_CALL
GetLastNtError();

#endif /* _WIN32K_ERROR_H */

/* EOF */
