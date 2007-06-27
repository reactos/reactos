#ifndef _WIN32K_ACCELERATOR_H
#define _WIN32K_ACCELERATOR_H

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

VOID
RegisterThreadAcceleratorTable(struct _ETHREAD *Thread);

#endif /* _WIN32K_ACCELERATOR_H */
