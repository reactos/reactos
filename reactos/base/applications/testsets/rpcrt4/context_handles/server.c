#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "ctx.h"

void CtxOpen( PCTXTYPE *pphContext,
	 long Value)
{
	printf("CtxOpen(): Value=%d\n",Value);
	*pphContext = (PCTXTYPE)midl_user_allocate( sizeof(CTXTYPE) );
	**pphContext = Value;
}

void CtxHello( PCTXTYPE phContext )
{
	printf("CtxHello(): Hello, World! Context value: %d\n", *phContext);
}

void CtxClose(PCTXTYPE *pphContext )
{
	printf("CtxClose(): %d\n", **pphContext);
	midl_user_free(*pphContext);
	*pphContext = NULL;
}


void main()
{
	RPC_STATUS status;
	unsigned int    cMinCalls      = 1;
	unsigned int    cMaxCalls      = 20;
	int i;

	status = RpcServerUseProtseqEp("ncacn_np", 20, "\\pipe\\hello", NULL);

	if (status) 
	{
		printf("RpcServerUseProtseqEp %x\n", status);
		exit(status);
	}

	status = RpcServerRegisterIf(hello_v1_0_s_ifspec, NULL, NULL);

	if (status) 
	{
		printf("RpcServerRegisterIf %x\n", status);
	exit(status);
	}

	status = RpcServerListen(1, 20, FALSE);

	if (status) 
	{
		printf("RpcServerListen %x", status);
		exit(status);
	}

	scanf("%d", &i);
}


void __RPC_USER PCTXTYPE_rundown(
    PCTXTYPE hContext)
{
	PCTXTYPE pCtx = (PCTXTYPE)hContext;
    printf("Context rundown: Value=%d \n", *pCtx);
    midl_user_free(hContext);
}

void __RPC_FAR * __RPC_USER midl_user_allocate(size_t len)
{
	return(malloc(len));
}
 
void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
	free(ptr);
}
