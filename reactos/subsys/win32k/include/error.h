#ifndef __WIN32K_ERROR_H
#define __WIN32K_ERROR_H

VOID
SetLastNtError(
  NTSTATUS Status);

VOID
SetLastWin32Error(
  DWORD Status);

#endif /* __WIN32K_ERROR_H */

/* EOF */
