#ifndef _WIN32K_ACCELERATOR_H
#define _WIN32K_ACCELERATOR_H

#include <include/win32.h>
#include <include/winsta.h>
#include <include/window.h>

typedef struct _ACCELERATOR_TABLE
{
  int Count;
  LPACCEL Table;
} ACCELERATOR_TABLE, *PACCELERATOR_TABLE;

NTSTATUS FASTCALL
InitAcceleratorImpl();

NTSTATUS FASTCALL
CleanupAcceleratorImpl();

PACCELERATOR_TABLE FASTCALL UserGetAccelObject(HACCEL);

#endif /* _WIN32K_ACCELERATOR_H */
