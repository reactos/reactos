#ifndef __WIN32K_ERROR_H
#define __WIN32K_ERROR_H

VOID FASTCALL
SetLastNtError(
  NTSTATUS Status);

VOID FASTCALL
SetLastWin32Error(
  DWORD Status);

#endif /* __WIN32K_ERROR_H */

/* EOF */
