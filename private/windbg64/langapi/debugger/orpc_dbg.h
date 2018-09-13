//--------------------------------------------------------------------------
// ORPC_DBG.H (tabs 4)
//
//  !!!!!!!!! !!!!!!!!! NOTE NOTE NOTE NOTE !!!!!!!!! !!!!!!!!!!
//
//          SEND MAIL TO SANJAYS  IF YOU MODIFY THIS FILE!
//            WE MUST KEEP OLE AND LANGUAGES IN SYNC!
//
//  !!!!!!!!! !!!!!!!!! NOTE NOTE NOTE NOTE !!!!!!!!! !!!!!!!!!!
//
// Created 07-Oct-1993 by Mike Morearty.  The master copy of this file
// is in the LANGAPI project owned by the Languages group.
//
// Macros and functions for OLE RPC debugging.  For a detailed explanation,
// see OLE2DBG.DOC.
//
//--------------------------------------------------------------------------


#ifndef __ORPC_DBG__
#define __ORPC_DBG__

//--------------------------------------------------------------------------
// Public:
//--------------------------------------------------------------------------

// This structure is the information packet which OLE sends the debugger
// when it is notifying it about an OLE debug event. The first field in this
// structure points to the signature which identifies the type of the debug 
// notification. The consumer of the notification can then get the relevant 
// information from the struct members. Note that for each OLE debug notification
// only a subset of the struct members are meaningful. 


typedef struct ORPC_DBG_ALL 
{
	BYTE *				pSignature;
	RPCOLEMESSAGE *		pMessage;
	const IID *	 		refiid;
	IRpcChannelBuffer *	pChannel;
	IUnknown *			pUnkProxyMgr;
	void *				pInterface;
	IUnknown *			pUnkObject;
	HRESULT				hresult;
	void *				pvBuffer;
	ULONG				cbBuffer;	
	ULONG *				lpcbBuffer; 
	void * 				reserved;
} ORPC_DBG_ALL;

typedef ORPC_DBG_ALL __RPC_FAR *LPORPC_DBG_ALL;

// Interface definition for IOrpcDebugNotify 

typedef interface IOrpcDebugNotify IOrpcDebugNotify;

typedef IOrpcDebugNotify __RPC_FAR * LPORPCDEBUGNOTIFY;

#if defined(__cplusplus) && !defined(CINTERFACE)

	interface IOrpcDebugNotify : public IUnknown
	{
	public:
		virtual VOID __stdcall ClientGetBufferSize (LPORPC_DBG_ALL) = 0;
		virtual VOID __stdcall ClientFillBuffer (LPORPC_DBG_ALL) = 0;
		virtual VOID __stdcall ClientNotify (LPORPC_DBG_ALL) = 0;
		virtual VOID __stdcall ServerNotify (LPORPC_DBG_ALL) = 0;
		virtual VOID __stdcall ServerGetBufferSize (LPORPC_DBG_ALL) = 0;
		virtual VOID __stdcall ServerFillBuffer (LPORPC_DBG_ALL) = 0;
	};

#else /* C style interface */

	typedef struct IOrpcDebugNotifyVtbl
	{
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IOrpcDebugNotify __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IOrpcDebugNotify __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IOrpcDebugNotify __RPC_FAR * This);

		VOID ( __stdcall __RPC_FAR *ClientGetBufferSize)(
			IOrpcDebugNotify __RPC_FAR * This,
			LPORPC_DBG_ALL lpOrpcDebugAll);
		
		VOID ( __stdcall __RPC_FAR *ClientFillBuffer)(
			IOrpcDebugNotify __RPC_FAR * This,
			LPORPC_DBG_ALL lpOrpcDebugAll);
		
		VOID ( __stdcall __RPC_FAR *ClientNotify)(
			IOrpcDebugNotify __RPC_FAR * This,
			LPORPC_DBG_ALL lpOrpcDebugAll);
		
		VOID ( __stdcall __RPC_FAR *ServerNotify)(
			IOrpcDebugNotify __RPC_FAR * This,
			LPORPC_DBG_ALL lpOrpcDebugAll);
		
		VOID ( __stdcall __RPC_FAR *ServerGetBufferSize)(
			IOrpcDebugNotify __RPC_FAR * This,
			LPORPC_DBG_ALL lpOrpcDebugAll);
		
		VOID ( __stdcall __RPC_FAR *ServerFillBuffer)(
			IOrpcDebugNotify __RPC_FAR * This,
			LPORPC_DBG_ALL lpOrpcDebugAll);

		} IOrpcDebugNotifyVtbl;

		interface IOrpcDebugNotify 
		{
			CONST_VTBL struct IOrpcDebugNotifyVtbl __RPC_FAR *lpVtbl;
		};

#endif

// This is the structure that is passed by the debugger to OLE when it enables ORPC 
// debugging. 
typedef struct ORPC_INIT_ARGS
{
	IOrpcDebugNotify __RPC_FAR * lpIntfOrpcDebug;
	void *	pvPSN;	// contains ptr to Process Serial No. for Mac ORPC debugging.
	DWORD	dwReserved1; // For future use, must be 0.
	DWORD	dwReserved2;
} ORPC_INIT_ARGS;

typedef ORPC_INIT_ARGS  __RPC_FAR * LPORPC_INIT_ARGS;
				
// Function pointer prototype for the "DllDebugObjectRPCHook" function.
typedef BOOL (WINAPI* ORPCHOOKPROC)(BOOL, LPORPC_INIT_ARGS); 

// The first four bytes in the debug specific packet are interpreted by the
// ORPC debug layer. The valid values are the ones defined below.

#define ORPC_DEBUG_ALWAYS					(0x00000000L)	// Notify always.
#define ORPC_DEBUG_IF_HOOK_ENABLED			(0x00000001L)	// Notify only if hook enabled.
 

// This exception code indicates that the exception is really an 
// ORPC debug notification.

#define EXCEPTION_ORPC_DEBUG (0x804f4c45)


//--------------------------------------------------------------------------------------
// Private: Declarations below this point are related to the implementation and should
// be removed from the distributable version of the header file.
//--------------------------------------------------------------------------------------


// Helper routines to set & restore the "Auto" value in the registry

BOOL WINAPI DebugORPCSetAuto(VOID);
VOID WINAPI DebugORPCRestoreAuto(VOID);

 ULONG WINAPI DebugORPCClientGetBufferSize(
	RPCOLEMESSAGE *	pMessage,
	REFIID			iid,
	void *			reserved,
	IUnknown *		pUnkProxyMgr,
	LPORPC_INIT_ARGS	lpInitArgs,
	BOOL				fHookEnabled);

void WINAPI DebugORPCClientFillBuffer(
	RPCOLEMESSAGE *		pMessage,
	REFIID				iid,
	void *				reserved,
	IUnknown *			pUnkProxyMgr,
	void *				pvBuffer,
	ULONG				cbBuffer,
	LPORPC_INIT_ARGS	lpInitArgs,
	BOOL				fHookEnabled);

void WINAPI DebugORPCClientNotify(
	RPCOLEMESSAGE *	pMessage,
	REFIID			iid,
	void *			reserved,
	IUnknown *		pUnkProxyMgr,
	HRESULT			hresult,
	void *			pvBuffer,
	ULONG			cbBuffer,
	LPORPC_INIT_ARGS	lpInitArgs,
	BOOL				fHookEnabled);

void WINAPI DebugORPCServerNotify(
	RPCOLEMESSAGE *		pMessage,
	REFIID				iid,
	IRpcChannelBuffer *	pChannel,
	void *				pInterface,
	IUnknown *			pUnkObject,
	void *				pvBuffer,
	ULONG				cbBuffer,
	LPORPC_INIT_ARGS	lpInitArgs,
	BOOL				fHookEnabled);

ULONG WINAPI DebugORPCServerGetBufferSize(
	RPCOLEMESSAGE *		pMessage,
	REFIID				iid,
	IRpcChannelBuffer *	pChannel,
	void *				pInterface,
	IUnknown *			pUnkObject,
	LPORPC_INIT_ARGS	lpInitArgs,
	BOOL				fHookEnabled);

void WINAPI DebugORPCServerFillBuffer(
	RPCOLEMESSAGE *		pMessage,
	REFIID				iid,
	IRpcChannelBuffer *	pChannel,
	void *				pInterface,
	IUnknown *			pUnkObject,
	void *				pvBuffer,
	ULONG				cbBuffer,
	LPORPC_INIT_ARGS	lpInitArgs,
	BOOL				fHookEnabled);

#endif // __ORPC_DBG__
