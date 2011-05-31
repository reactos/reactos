
#include "audsrvapi.h"


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{   
	RPC_STATUS status;
    unsigned short * pszStringBinding    = NULL;
	
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		status = RpcStringBindingComposeW(NULL,L"ncacn_np",NULL,L"\\pipe\\audsrv", NULL,&pszStringBinding);

		status = RpcBindingFromStringBindingW(pszStringBinding, &audsrv_v0_0_c_ifspec);

		if (status) printf("Connection Problem p %d \n",status);

		status = RpcStringFree(&pszStringBinding);

		if (status) printf("Problem Freeing  String : %d \n",status);
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		status = RpcBindingFree(audsrv_v0_0_c_ifspec);
 		if (status == RPC_S_INVALID_BINDING) printf("Error : %d Invalid RPC S HANDLE\n",status);
	break;
	}
	return TRUE;
}
