/* $Id: main.c,v 1.3 2002/03/11 20:48:25 hyperion Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/misc/main.c
 * PURPOSE:     psxdll.dll entry point
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              27/12/2001: Created
 */

#include <psx/debug.h>
#include <psx/pdata.h>

#define DLL_PROCESS_ATTACH 1    
#define DLL_THREAD_ATTACH  2    
#define DLL_THREAD_DETACH  3    
#define DLL_PROCESS_DETACH 0    

int __stdcall DllMain(void *pDllInstance, unsigned long int nReason, void *pUnknown)
{
#if 0
 if(nReason == DLL_PROCESS_ATTACH)
 {
  __PPDX_PDATA ppdProcessData = __PdxGetProcessData();

  if(ppdProcessData == 0)
   WARN("this process doesn't have a process data block\n");
  else if(ppdProcessData->Spawned)
  {
   INFO("the process has been spawned\n");
  }
 }
#endif
 return (1);
}

/* EOF */

