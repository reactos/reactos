#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "ctx.h" 
 
#define TYPE_FORMAT_STRING_SIZE   23                                
#define PROC_FORMAT_STRING_SIZE   21                                
#define TRANSMIT_AS_TABLE_SIZE    0            
#define WIRE_MARSHAL_TABLE_SIZE   0   

typedef struct _MIDL_TYPE_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];
    } MIDL_TYPE_FORMAT_STRING;

typedef struct _MIDL_PROC_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ PROC_FORMAT_STRING_SIZE ];
} MIDL_PROC_FORMAT_STRING;

extern const MIDL_STUB_DESC hello_StubDesc;
extern const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString;
//extern const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString;*/

/*****************************************************************
 * Modified from midl-generated stubs                            *
 *****************************************************************/


void m_CtxOpen( 
    /* [out] */ PCTXTYPE __RPC_FAR *pphContext,
    /* [in] */ long Value)
{
    RPC_BINDING_HANDLE _Handle	=	0;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;

	char *ctx, *buf;
	int i;
	
	printf("\n*******************************************************************\n");
	printf("**** CtxOpen()                                                  ***\n");
	printf("*******************************************************************\n\n");
    
    if(!pphContext)
        {
        RpcRaiseException(RPC_X_NULL_REF_POINTER);
        }
    RpcTryFinally
        {
        NdrClientInitializeNew(
                          ( PRPC_MESSAGE  )&_RpcMessage,
                          ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                          ( PMIDL_STUB_DESC  )&hello_StubDesc,
                          0);
        
        
        _Handle = hBinding;
        
        
        _StubMsg.BufferLength = 4U;
        NdrGetBuffer( (PMIDL_STUB_MESSAGE) &_StubMsg, _StubMsg.BufferLength, _Handle );
        
        *(( long __RPC_FAR * )_StubMsg.Buffer)++ = Value;
        
        NdrSendReceive( (PMIDL_STUB_MESSAGE) &_StubMsg, (unsigned char __RPC_FAR *) _StubMsg.Buffer );
        
        if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[0] );


        printf("Before NdrClientContextUnmarshall: Buflen=%d\nBuffer: ",  _StubMsg.RpcMsg->BufferLength);
        for(buf = _StubMsg.Buffer, i = 0; i < _StubMsg.RpcMsg->BufferLength; i++)
        	printf("0x%x, ", buf[i] & 0x0FF);
        printf("\n\n");
        
        *pphContext = (void *)0;
        NdrClientContextUnmarshall(
                              ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                              ( NDR_CCONTEXT __RPC_FAR * )pphContext,
                              _Handle);
        
        ctx = (char*)*pphContext;
        printf("\nNdrClientContextUnmarshall returned: handle=0x%p\n", ctx);
        printf("00: 0x%x <- obviously pointer to binding handle copyed from _Handle\n", *((int*)ctx));
        ctx+=4;
        printf("04: 0x%x <- unknown field\n", *((int*)ctx));
        printf("08: ");
        
        for(ctx+=4, i = 0; i < 20; i++)
        	printf("0x%x,", *(ctx+i) & 0x0FF); printf(" <- ndr 20 bytes\n\n");
        	
        printf("Buflen=%d, Buffer: ", _StubMsg.BufferLength);
        for(buf = _StubMsg.BufferStart; buf < _StubMsg.BufferEnd; buf++)
        	printf("0x%x,", *buf & 0x0FF);
        printf("\n");
        
        
        }
    RpcFinally
        {
        NdrFreeBuffer( (PMIDL_STUB_MESSAGE) &_StubMsg );
        
        }
    RpcEndFinally
    
}


void m_CtxHello( 
    /* [in] */ PCTXTYPE phContext)
{

    RPC_BINDING_HANDLE _Handle	=	0;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    char *buf;
    int i;
    
	printf("\n*******************************************************************\n");
	printf("**** CtxHello()                                                 ***\n");
	printf("*******************************************************************\n\n");
    
    RpcTryFinally
        {
        NdrClientInitializeNew(
                          ( PRPC_MESSAGE  )&_RpcMessage,
                          ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                          ( PMIDL_STUB_DESC  )&hello_StubDesc,
                          1);
        
        
        if(phContext != 0)
            {
            _Handle = NDRCContextBinding(( NDR_CCONTEXT  )phContext);;
            
            }
        else
            {
            RpcRaiseException(RPC_X_SS_IN_NULL_CONTEXT);
            }
        
        _StubMsg.BufferLength = 20U;
        NdrGetBuffer( (PMIDL_STUB_MESSAGE) &_StubMsg, _StubMsg.BufferLength, _Handle );
        
        NdrClientContextMarshall(
                            ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                            ( NDR_CCONTEXT  )phContext,
                            1);
        printf("After NdrClientContextMarshall: Buflen=%d\nBuffer: ", _StubMsg.BufferLength );
        for(buf = _StubMsg.Buffer, i = 0; i < _StubMsg.BufferLength; i++)
        	printf("0x%x, ", buf[i] & 0x0FF);
        printf("\n\n");
        
        NdrSendReceive( (PMIDL_STUB_MESSAGE) &_StubMsg, (unsigned char __RPC_FAR *) _StubMsg.Buffer );
        
        }
    RpcFinally
        {
        NdrFreeBuffer( (PMIDL_STUB_MESSAGE) &_StubMsg );
        
        }
    RpcEndFinally
    
}


void m_CtxClose( 
    /* [out][in] */ PCTXTYPE __RPC_FAR *pphContext)
{

    RPC_BINDING_HANDLE _Handle	=	0;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    char *buf;
    int i;
	
	printf("\n*******************************************************************\n");
	printf("**** CtxClose()                                                 ***\n");
	printf("*******************************************************************\n\n");
	
    if(!pphContext)
        {
        RpcRaiseException(RPC_X_NULL_REF_POINTER);
        }
    RpcTryFinally
        {
        NdrClientInitializeNew(
                          ( PRPC_MESSAGE  )&_RpcMessage,
                          ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                          ( PMIDL_STUB_DESC  )&hello_StubDesc,
                          2);
        
        
        if(*pphContext != 0)
            {
            _Handle = NDRCContextBinding(( NDR_CCONTEXT  )*pphContext);;
            
            }
        
        _StubMsg.BufferLength = 20U;
        NdrGetBuffer( (PMIDL_STUB_MESSAGE) &_StubMsg, _StubMsg.BufferLength, _Handle );
        
        NdrClientContextMarshall(
                            ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                            ( NDR_CCONTEXT  )*pphContext,
                            0);
        
        NdrSendReceive( (PMIDL_STUB_MESSAGE) &_StubMsg, (unsigned char __RPC_FAR *) _StubMsg.Buffer );
        
        if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[14] );
        
        
      printf("Before NdrClientContextUnmarshall: Buflen=%d\nBuffer: ",  _StubMsg.BufferLength );
        for(buf = _StubMsg.Buffer, i = 0; i < _StubMsg.BufferLength; i++)
        	printf("0x%x, ", buf[i] & 0x0FF);
        printf("\n\n");
        
        NdrClientContextUnmarshall(
                              ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                              ( NDR_CCONTEXT __RPC_FAR * )pphContext,
                              _Handle);
                            
 
		printf("\nNdrClientContextUnmarshall returned: handle=0x%p\n", *pphContext);
               
        
        }
    RpcFinally
        {
        NdrFreeBuffer( (PMIDL_STUB_MESSAGE) &_StubMsg );
        
        }
    RpcEndFinally
    
}

void main()
{
	RPC_STATUS status;
	unsigned long ulCode;
	PCTXTYPE hContext;
	char *pszStringBinding = NULL;

	status = RpcStringBindingCompose(NULL, 
		"ncacn_np", 
		NULL, 
		"\\pipe\\hello", 
		NULL, 
		&pszStringBinding);

	if (status) 
	{
		printf("RpcStringBindingCompose %x\n", status);
		exit(status);
	}
	
	status = RpcBindingFromStringBinding(pszStringBinding, &hBinding);

	if (status)
	{
		printf("RpcBindingFromStringBinding %x\n", status);
		exit(status);
	}

	RpcStringFree(&pszStringBinding); 
	
	m_CtxOpen(&hContext, 31337);
	RpcBindingFree(&hBinding);
	
	m_CtxHello(hContext);
	
	m_CtxClose(&hContext);
	

}


void __RPC_FAR * __RPC_USER midl_user_allocate(size_t len)
{
	return(malloc(len));
}
 
void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
	free(ptr);
}
