#pragma once

#include <include/win32.h>
#include <include/winsta.h>
#include <include/window.h>

typedef struct _ACCELERATOR_TABLE
{
  HEAD head;
  int Count;
  LPACCEL Table;
} ACCELERATOR_TABLE, *PACCELERATOR_TABLE;

NTSTATUS FASTCALL
InitAcceleratorImpl(VOID);

NTSTATUS FASTCALL
CleanupAcceleratorImpl(VOID);

PACCELERATOR_TABLE FASTCALL UserGetAccelObject(HACCEL);
