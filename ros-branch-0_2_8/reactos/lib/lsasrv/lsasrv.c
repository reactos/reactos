
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

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
