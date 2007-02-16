/* 
 * PROJECT:          ReactOS RpcSs service
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             services/rpcss/rpcss.c
 * PURPOSE:          Service initialization
 * COPYRIGHT:        Copyright 2002 Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "rpcss.h"

#define NDEBUG
#include <debug.h>



/* GLOBALS ******************************************************************/



/* FUNCTIONS *****************************************************************/


VOID CALLBACK
ServiceMain(DWORD argc, LPTSTR *argv)
{

  DPRINT("ServiceMain() called\n");

  StartEndpointMapper();

  DPRINT("ServiceMain() done\n");
}


int main(int argc, char *argv[])
{
#if 0
  SERVICE_TABLE_ENTRY ServiceTable[] = {{"RpcSs", ServiceMain},{NULL, NULL}};
#endif

  DPRINT("RpcSs service\n");

#if 0
  StartServiceCtrlDispatcher(ServiceTable);
#endif

  ServiceMain(0, NULL);

  DPRINT("RpcSS: done\n");

  return 0;
}

/* EOF */
