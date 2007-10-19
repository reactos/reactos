
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the RPC server stubs */


 /* File created by MIDL compiler version 5.03.0280 */
/* at Fri Mar 24 18:32:16 2006
 */
/* Compiler settings for ctx.idl:
    Os (OptLev=s), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data
    VC __declspec() decoration level:
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#if !defined(_M_IA64) && !defined(_M_AXP64)
#include <string.h>
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

extern const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString;
extern const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString;

/* Standard interface: hello, ver. 1.0,
   GUID={0x7a98c250,0x6808,0x11cf,{0xb7,0x3b,0x00,0xaa,0x00,0xb6,0x77,0xa7}} */


extern RPC_DISPATCH_TABLE hello_v1_0_DispatchTable;

static const RPC_SERVER_INTERFACE hello___RpcServerInterface =
    {
    sizeof(RPC_SERVER_INTERFACE),
    {{0x7a98c250,0x6808,0x11cf,{0xb7,0x3b,0x00,0xaa,0x00,0xb6,0x77,0xa7}},{1,0}},
    {{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}},
    &hello_v1_0_DispatchTable,
    0,
    0,
    0,
    0,
    0
    };
RPC_IF_HANDLE hello_v1_0_s_ifspec = (RPC_IF_HANDLE)& hello___RpcServerInterface;

extern const MIDL_STUB_DESC hello_StubDesc;

void __RPC_STUB
hello_CtxOpen(
    PRPC_MESSAGE _pRpcMessage )
{
    long Value;
    MIDL_STUB_MESSAGE _StubMsg;
    NDR_SCONTEXT pphContext;
    RPC_STATUS _Status;

    ((void)(_Status));
    NdrServerInitializeNew(
                          _pRpcMessage,
                          &_StubMsg,
                          &hello_StubDesc);

    ( PCTXTYPE __RPC_FAR * )pphContext = 0;
    RpcTryFinally
        {
        RpcTryExcept
            {
            if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[0] );

            Value = *(( long __RPC_FAR * )_StubMsg.Buffer)++;

            if(_StubMsg.Buffer > _StubMsg.BufferEnd)
                {
                RpcRaiseException(RPC_X_BAD_STUB_DATA);
                }
            }
        RpcExcept( RPC_BAD_STUB_DATA_EXCEPTION_FILTER )
            {
            RpcRaiseException(RPC_X_BAD_STUB_DATA);
            }
        RpcEndExcept
        pphContext = NDRSContextUnmarshall( (char *)0, _pRpcMessage->DataRepresentation );


        CtxOpen(( PCTXTYPE __RPC_FAR * )NDRSContextValue(pphContext),Value);

        _StubMsg.BufferLength = 20U;
        _pRpcMessage->BufferLength = _StubMsg.BufferLength;

        _Status = I_RpcGetBuffer( _pRpcMessage );
        if ( _Status )
            RpcRaiseException( _Status );

        _StubMsg.Buffer = (unsigned char __RPC_FAR *) _pRpcMessage->Buffer;

        NdrServerContextMarshall(
                            ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                            ( NDR_SCONTEXT  )pphContext,
                            ( NDR_RUNDOWN  )PCTXTYPE_rundown);

        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength =
        (unsigned int)(_StubMsg.Buffer - (unsigned char __RPC_FAR *)_pRpcMessage->Buffer);

}

void __RPC_STUB
hello_CtxHello(
    PRPC_MESSAGE _pRpcMessage )
{
    MIDL_STUB_MESSAGE _StubMsg;
    NDR_SCONTEXT phContext;
    RPC_STATUS _Status;

    ((void)(_Status));
    NdrServerInitializeNew(
                          _pRpcMessage,
                          &_StubMsg,
                          &hello_StubDesc);

    RpcTryFinally
        {
        RpcTryExcept
            {
            if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[8] );

            phContext = NdrServerContextUnmarshall(( PMIDL_STUB_MESSAGE  )&_StubMsg);

            if(_StubMsg.Buffer > _StubMsg.BufferEnd)
                {
                RpcRaiseException(RPC_X_BAD_STUB_DATA);
                }
            }
        RpcExcept( RPC_BAD_STUB_DATA_EXCEPTION_FILTER )
            {
            RpcRaiseException(RPC_X_BAD_STUB_DATA);
            }
        RpcEndExcept

        CtxHello(( PCTXTYPE  )*NDRSContextValue(phContext));

        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength =
        (unsigned int)(_StubMsg.Buffer - (unsigned char __RPC_FAR *)_pRpcMessage->Buffer);

}

void __RPC_STUB
hello_CtxClose(
    PRPC_MESSAGE _pRpcMessage )
{
    MIDL_STUB_MESSAGE _StubMsg;
    NDR_SCONTEXT pphContext;
    RPC_STATUS _Status;

    ((void)(_Status));
    NdrServerInitializeNew(
                          _pRpcMessage,
                          &_StubMsg,
                          &hello_StubDesc);

    ( PCTXTYPE __RPC_FAR * )pphContext = 0;
    RpcTryFinally
        {
        RpcTryExcept
            {
            if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[14] );

            pphContext = NdrServerContextUnmarshall(( PMIDL_STUB_MESSAGE  )&_StubMsg);

            if(_StubMsg.Buffer > _StubMsg.BufferEnd)
                {
                RpcRaiseException(RPC_X_BAD_STUB_DATA);
                }
            }
        RpcExcept( RPC_BAD_STUB_DATA_EXCEPTION_FILTER )
            {
            RpcRaiseException(RPC_X_BAD_STUB_DATA);
            }
        RpcEndExcept

        CtxClose(( PCTXTYPE __RPC_FAR * )NDRSContextValue(pphContext));

        _StubMsg.BufferLength = 20U;
        _pRpcMessage->BufferLength = _StubMsg.BufferLength;

        _Status = I_RpcGetBuffer( _pRpcMessage );
        if ( _Status )
            RpcRaiseException( _Status );

        _StubMsg.Buffer = (unsigned char __RPC_FAR *) _pRpcMessage->Buffer;

        NdrServerContextMarshall(
                            ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                            ( NDR_SCONTEXT  )pphContext,
                            ( NDR_RUNDOWN  )PCTXTYPE_rundown);

        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength =
        (unsigned int)(_StubMsg.Buffer - (unsigned char __RPC_FAR *)_pRpcMessage->Buffer);

}


static const MIDL_STUB_DESC hello_StubDesc =
    {
    (void __RPC_FAR *)& hello___RpcServerInterface,
    MIDL_user_allocate,
    MIDL_user_free,
    0,
    0,
    0,
    0,
    0,
    __MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x10001, /* Ndr library version */
    0,
    0x5030118, /* MIDL Version 5.3.280 */
    0,
    0,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0,  /* Reserved3 */
    0,  /* Reserved4 */
    0   /* Reserved5 */
    };

static RPC_DISPATCH_FUNCTION hello_table[] =
    {
    hello_CtxOpen,
    hello_CtxHello,
    hello_CtxClose,
    0
    };
RPC_DISPATCH_TABLE hello_v1_0_DispatchTable =
    {
    3,
    hello_table
    };

#if !defined(__RPC_WIN32__)
#error  Invalid build platform for this stub.
#endif

static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =
    {
        0,
        {

			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC stack size = 1 */
#else
			0x2,		/* Alpha stack size = 2 */
#endif
/*  2 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */
/*  4 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/*  6 */	0x5b,		/* FC_END */
			0x5c,		/* FC_PAD */
/*  8 */
			0x4d,		/* FC_IN_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC stack size = 1 */
#else
			0x2,		/* Alpha stack size = 2 */
#endif
/* 10 */	NdrFcShort( 0xa ),	/* Type Offset=10 */
/* 12 */	0x5b,		/* FC_END */
			0x5c,		/* FC_PAD */
/* 14 */
			0x50,		/* FC_IN_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC stack size = 1 */
#else
			0x2,		/* Alpha stack size = 2 */
#endif
/* 16 */	NdrFcShort( 0xe ),	/* Type Offset=14 */
/* 18 */	0x5b,		/* FC_END */
			0x5c,		/* FC_PAD */

			0x0
        }
    };

static const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */
/*  2 */
			0x11, 0x0,	/* FC_RP */
/*  4 */	NdrFcShort( 0x2 ),	/* Offset= 2 (6) */
/*  6 */	0x30,		/* FC_BIND_CONTEXT */
			0xa0,		/* Ctxt flags:  via ptr, out, */
/*  8 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 10 */	0x30,		/* FC_BIND_CONTEXT */
			0x41,		/* Ctxt flags:  in, can't be null */
/* 12 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 14 */
			0x11, 0x0,	/* FC_RP */
/* 16 */	NdrFcShort( 0x2 ),	/* Offset= 2 (18) */
/* 18 */	0x30,		/* FC_BIND_CONTEXT */
			0xe1,		/* Ctxt flags:  via ptr, in, out, can't be null */
/* 20 */	0x0,		/* 0 */
			0x0,		/* 0 */

			0x0
        }
    };


#endif /* !defined(_M_IA64) && !defined(_M_AXP64)*/

