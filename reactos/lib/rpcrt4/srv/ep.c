/*
 * 
 */

/* INCLUDES ******************************************************************/

#include <windows.h>

/* FUNCTIONS *****************************************************************/

RPC_STATUS RpcServerUseProtseqA(unsigned char* Protseq,
				unsigned int MaxCalls,
				void* SecurityDescriptor)
{
   return(RpcServerUseProtseqEpA(Protseq,
				 MaxCalls,
				 SecuritDescriptor));
}

RPC_STATUS RpcServerUseProtseqW(unsigned wchar_t* Protseq,
				unsigned int MaxCalls,
				void* SecurityDescriptor)
{
   return(RpcServerUseProtseqEpW(Protseq,
				 MaxCalls,
				 SecuritDescriptor));
}

RPC_STATUS RpcServerUseProtseqEpA(unsigned char* Protseq,
				  unsigned int MaxCalls,
				  unsigned char* Endpoint,
				  void* SecurityDescriptor)
{
   unsigned wchar_t* ProtseqW;
   UNICODE_STRING ProtseqU;
   ANSI_STRING ProtseqA;
   unsigned wchar_t* EndpointW;
   UNICODE_STRING EndpointU;
   ANSI_STRING EndpointA;
   RPC_STATUS Status;
   
   if (Protseq != NULL)
     {
	RtlInitAnsiString(&ProtseqA, Protseq);
	RtlAnsiStringToUnicodeString(&ProtseqU, &ProtseqA, TRUE);
	ProtseqW = ProtseqU.Buffer;
     }
   else
     {
	ProtseqW = NULL;
     }
   if (Endpoint != NULL)
     {
	RtlInitAnsiString(&EndpointA, Endpoint);
	RtlAnsiStringToUnicodeString(&EndpointU, &EndpointA, TRUE);
	EndpointW = EndpointU.Buffer;
     }
   else
     {
	EndpointW = NULL;
     }
   
   Status = RpcServerUseProtseqEpW(ProtseqW,
				   MaxCalls,
				   EndpointW,
				   SecurityDescriptor);
   
   RtlFreeUnicodeString(&EndpointU);
   RtlFreeUnicodeString(&ProtseqU);
   return(Status);
}

RPC_STATUS RpcServerUseProtseqEpW(unsigned wchar_t* Protseq,
				  unsigned int MaxCalls,
				  unsigned wchar_t* Endpoint,
				  void* SecurityDescriptor)
{
}

RPC_STATUS RpcServerRegisterIf(RPC_IF_HANDLE IfSpec,
			       UUID* MgrTypeUuid,
			       RPC_MGR_EPV* MgrEpv)
{
   
}


