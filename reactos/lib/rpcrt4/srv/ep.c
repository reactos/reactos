/*
 * 
 */

/* INCLUDES ******************************************************************/

#include <windows.h>

/* FUNCTIONS *****************************************************************/

RPC_STATUS RpcServerUseProtseqEpA(unsigned char* Protseq,
				  unsigned int MaxCalls,
				  unsigned char* Endpoint,
				  void* SecurityDescriptor)
{
}

RPC_STATUS RpcServerUseProtseqEpW(unsigned short* Protseq,
				  unsigned int MaxCalls,
				  unsigned short* Endpoint,
				  void* SecurityDescriptor)
{
}

RPC_STATUS RpcServerRegisterIf(RPC_IF_HANDLE IfSpec,
			       UUID* MgrTypeUuid,
			       RPC_MGR_EPV* MgrEpv)
{
   
}

RPC_STATUS RpcServerListen(unsigned int MinimumCallThreads,
			   unsigned int MaxCalls,
			   unsigned int DontWait)
{
}

