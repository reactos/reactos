
#define NTOS_MODE_USER
#include <ntos.h>
#include <windows.h>

#include <lsass/lsasrv.h>

#define NDEBUG
#include <debug.h>

VOID StartLsaPortThread(VOID);


NTSTATUS STDCALL
LsapInitLsa(VOID)
{
  DPRINT1("LsapInitLsa() called\n");

  StartLsaPortThread();

  return STATUS_SUCCESS;
}

/* EOF */
