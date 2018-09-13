//--------------------------------------------------------------------------
// ORPC_DBG.C (tabs 4)
//
//  !!!!!!!!! !!!!!!!!! NOTE NOTE NOTE NOTE !!!!!!!!! !!!!!!!!!!
//
//          SEND MAIL TO SANJAYS  IF YOU MODIFY THIS FILE!
//            WE MUST KEEP OLE AND LANGUAGES IN SYNC!
//
//  !!!!!!!!! !!!!!!!!! NOTE NOTE NOTE NOTE !!!!!!!!! !!!!!!!!!!
//
// Created 08-Oct-1993 by Mike Morearty.  The master copy of this file
// is in the LANGAPI project owned by the Languages group.
//
// Helper functions for OLE RPC debugging.
//--------------------------------------------------------------------------

#include <windows.h>
#ifndef _CHICAGO_
#include <tchar.h>
#endif

#include "orpc_dbg.h"

static TCHAR tszAeDebugName[] = TEXT("AeDebug");
static TCHAR tszAutoName[] = TEXT("Auto");
static TCHAR tszOldAutoName[] = TEXT("OldAuto");
static TCHAR tszDebugObjectRpcEnabledName[] =
#ifdef _CHICAGO_
	"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DebugObjectRPCEnabled";
#else
	TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\DebugObjectRPCEnabled");
#endif

// Emit the ORPC signature into the bytestream of the function
#define ORPC_EMIT_SIGNATURE()	'M', 'A', 'R', 'B',

// Emit a LONG into the bytestream
#define ORPC_EMIT_LONG(l)	\
	((l >>  0) & 0xFF),		\
	((l >>  8) & 0xFF),		\
	((l >> 16) & 0xFF),		\
	((l >> 24) & 0xFF),

// Emit a WORD into the bytestream
#define ORPC_EMIT_WORD(w)	\
	((w >> 0) & 0xFF),		\
	((w >> 8) & 0xFF),

// Emit a BYTE into the bytestream
#define ORPC_EMIT_BYTE(b)	\
	b,

// Emit a GUID into the bytestream
#define ORPC_EMIT_GUID(l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)	\
	ORPC_EMIT_LONG(l)												\
	ORPC_EMIT_WORD(w1) ORPC_EMIT_WORD(w2)							\
	ORPC_EMIT_BYTE(b1) ORPC_EMIT_BYTE(b2)							\
	ORPC_EMIT_BYTE(b3) ORPC_EMIT_BYTE(b4)							\
	ORPC_EMIT_BYTE(b5) ORPC_EMIT_BYTE(b6)							\
	ORPC_EMIT_BYTE(b7) ORPC_EMIT_BYTE(b8)

BYTE rgbClientGetBufferSizeSignature[] =
{
	ORPC_EMIT_SIGNATURE()
	ORPC_EMIT_GUID(0x9ED14F80, 0x9673, 0x101A, 0xB0, 0x7B,
		0x00, 0xDD, 0x01, 0x11, 0x3F, 0x11)
	ORPC_EMIT_LONG(0)
};

BYTE rgbClientFillBufferSignature[] =
{
	ORPC_EMIT_SIGNATURE()
	ORPC_EMIT_GUID(0xDA45F3E0, 0x9673, 0x101A, 0xB0, 0x7B,
		0x00, 0xDD, 0x01, 0x11, 0x3F, 0x11)
	ORPC_EMIT_LONG(0)
};

BYTE rgbClientNotifySignature[] =
{
	ORPC_EMIT_SIGNATURE()
	ORPC_EMIT_GUID(0x4F60E540, 0x9674, 0x101A, 0xB0, 0x7B,
		0x00, 0xDD, 0x01, 0x11, 0x3F, 0x11)
	ORPC_EMIT_LONG(0)
};

BYTE rgbServerNotifySignature[] =
{
	ORPC_EMIT_SIGNATURE()
	ORPC_EMIT_GUID(0x1084FA00, 0x9674, 0x101A, 0xB0, 0x7B,
		0x00, 0xDD, 0x01, 0x11, 0x3F, 0x11)
	ORPC_EMIT_LONG(0)
};

BYTE rgbServerGetBufferSizeSignature[] =
{
	ORPC_EMIT_SIGNATURE()
	ORPC_EMIT_GUID(0x22080240, 0x9674, 0x101A, 0xB0, 0x7B,
		0x00, 0xDD, 0x01, 0x11, 0x3F, 0x11)
	ORPC_EMIT_LONG(0)
};

BYTE rgbServerFillBufferSignature[] =
{
	ORPC_EMIT_SIGNATURE()
	ORPC_EMIT_GUID(0x2FC09500, 0x9674, 0x101A, 0xB0, 0x7B,
		0x00, 0xDD, 0x01, 0x11, 0x3F, 0x11)
	ORPC_EMIT_LONG(0)
};

// Macro to deal with assigning refiid for both C and C++.
#if defined(__cplusplus)
#define ASSIGN_REFIID(orpc_all, iid)	((orpc_all).refiid = &iid)
#else
#define ASSIGN_REFIID(orpc_all, iid)	((orpc_all).refiid = iid)
#endif

#pragma code_seg(".orpc")

//--------------------------------------------------------------------------
// SzSubStr()
//
// Find str2 in str2
//--------------------------------------------------------------------------

static LPTSTR SzSubStr(LPTSTR str1, LPTSTR str2)
{
	CharLower(str1);
	CharLower(str2);

#ifdef _CHICAGO_
	return strstr(str1, str2);
#else
	return _tcsstr(str1, str2);
#endif
}

//--------------------------------------------------------------------------
// DebugORPCSetAuto()
//
// Sets the "Auto" value in the "AeDebug" key to "1", and saves info
// necessary to restore the previous value later.
//--------------------------------------------------------------------------

BOOL WINAPI DebugORPCSetAuto(VOID)
{
	HKEY	hkey;
	TCHAR	rgtchDebugger[256];	// 256 is the length NT itself uses for this
	TCHAR	rgtchAuto[256];
	TCHAR	rgtchOldAuto[2];	// don't need to get the whole thing

	// If the "DebugObjectRPCEnabled" key does not exist, then do not
	// cause any notifications
	if (RegOpenKey(HKEY_LOCAL_MACHINE, tszDebugObjectRpcEnabledName, &hkey))
		return FALSE;
	RegCloseKey(hkey);

	// If the AeDebug debugger string does not exist, or if it contains
	// "drwtsn32" anywhere in it, then don't cause any notifications,
	// because Dr. Watson is not capable of fielding OLE notifications.
	if (!GetProfileString(tszAeDebugName, TEXT("Debugger"), TEXT(""),
			rgtchDebugger, sizeof(rgtchDebugger)) ||
		SzSubStr(rgtchDebugger, TEXT("drwtsn32")) != NULL)
	{
		return FALSE;
	}

	// Must ensure that the "Auto" value in the AeDebug registry key
	// is set to "1", so that the embedded INT 3 below will cause the
	// debugger to be automatically spawned if it doesn't already
	// exist.

	// Get old "Auto" value
	GetProfileString(tszAeDebugName, tszAutoName, TEXT(""),
		rgtchAuto, sizeof(rgtchAuto));

	// If "OldAuto" already existed, then it's probably left over from
	// a previous invocation of the debugger, so don't overwrite it.
	// Otherwise, copy "Auto" value to "OldAuto"
	if (!GetProfileString(tszAeDebugName, tszOldAutoName, TEXT(""),
		rgtchOldAuto, sizeof(rgtchOldAuto)))
	{
		if (!WriteProfileString(tszAeDebugName, tszOldAutoName, rgtchAuto))
			return FALSE;
	}

	// Change "Auto" value to "1"
	if (!WriteProfileString(tszAeDebugName, tszAutoName, TEXT("1")))
		return FALSE;

	return TRUE;
}

//--------------------------------------------------------------------------
// DebugORPCRestoreAuto()
//
// Restores the previous value of the "Auto" value in the AeDebug key.
//--------------------------------------------------------------------------

VOID WINAPI DebugORPCRestoreAuto(VOID)
{
	TCHAR	rgtchAuto[256];

	// Restore old Auto value (or delete it if it didn't exist before).
	// Very minor bug here: if "Auto" was previously "", then we will
	// now delete it.  That's not a big deal though, as an empty "Auto"
	// and a nonexistent one have the same effect.
	GetProfileString(tszAeDebugName, tszOldAutoName, TEXT(""),
		rgtchAuto, sizeof(rgtchAuto));

	WriteProfileString(tszAeDebugName, tszAutoName,
		rgtchAuto[0] ? rgtchAuto : NULL);

	// Delete OldAuto value
	WriteProfileString(tszAeDebugName, tszOldAutoName, NULL);
}

 // This pragma is necessary in case the compiler chooses not to inline these
// functions (e.g. in a debug build, when optimizations are off).

#pragma code_seg(".orpc")

__inline DWORD WINAPI OrpcBreakpointFilter(
	LPEXCEPTION_POINTERS lpExcptPtr,
	BOOL *lpAeDebugAttached )									\
{
	BOOL fAeDebugAttached = FALSE;
	DWORD dwRet;
																
	if ( lpExcptPtr->ExceptionRecord->ExceptionCode == EXCEPTION_ORPC_DEBUG )	
	{
		if  ( UnhandledExceptionFilter(lpExcptPtr) == EXCEPTION_CONTINUE_SEARCH )
		{
			// It is important that we don't return EXCEPTION_CONTINUE_SEARCH.
			// This is because there might an handler up the stack which could
			// handle this exception. Just set the flag indicating that a 
			// debugger is now attached.
			 
			fAeDebugAttached = TRUE;
		}
		dwRet = EXCEPTION_EXECUTE_HANDLER;
	}
	else 
	{
		// Not one of our exceptions.
		dwRet = EXCEPTION_CONTINUE_SEARCH;
	}

	if ( lpAeDebugAttached != NULL )
		(*lpAeDebugAttached) = fAeDebugAttached;
	
	return dwRet;	
}
							
ULONG WINAPI DebugORPCClientGetBufferSize(
	RPCOLEMESSAGE *		pMessage,
	REFIID				iid,
	void *				reserved,
	IUnknown *			pUnkProxyMgr,
	LPORPC_INIT_ARGS	lpInitArgs,
	BOOL				fHookEnabled)
{
	ULONG	cbBuffer = 0;
	ORPC_DBG_ALL	orpc_all = {0};
	ORPC_DBG_ALL *  lpOrpcAll = &orpc_all;

	if (!fHookEnabled)
		return 0; // We should be able to assert that this never happens.

	orpc_all.pSignature = rgbClientGetBufferSizeSignature;
	orpc_all.pMessage = pMessage;
	orpc_all.reserved = reserved;
	orpc_all.pUnkProxyMgr = pUnkProxyMgr;
	orpc_all.lpcbBuffer = &cbBuffer;
	ASSIGN_REFIID(orpc_all, iid);

	if ( lpInitArgs == NULL || lpInitArgs->lpIntfOrpcDebug == NULL )
	{
		// Do Orpc debug notification using an exception.
		__try 
		{
			RaiseException(EXCEPTION_ORPC_DEBUG, 0, 1, (LPDWORD)&lpOrpcAll);
		}
		__except(OrpcBreakpointFilter(GetExceptionInformation(), NULL))
		{
			// this just goes down to the  to the return.
		}
	}
	else 
	{
		IOrpcDebugNotify __RPC_FAR *lpIntf = lpInitArgs->lpIntfOrpcDebug;

		// call the appropriate method in the registered interface 
		// ( this is typically used by in-proc debuggers)
#if defined(__cplusplus) && !defined(CINTERFACE)
		lpIntf->ClientGetBufferSize(lpOrpcAll);
#else
		lpIntf->lpVtbl->ClientGetBufferSize(lpIntf, lpOrpcAll);
#endif
		
	}

	return cbBuffer;
}

//--------------------------------------------------------------------------

void WINAPI DebugORPCClientFillBuffer(
	RPCOLEMESSAGE *		pMessage,
	REFIID				iid,
	void *				reserved,
	IUnknown *			pUnkProxyMgr,
	void *				pvBuffer,
	ULONG				cbBuffer,
	LPORPC_INIT_ARGS	lpInitArgs,
	BOOL				fHookEnabled)
{
	ORPC_DBG_ALL	orpc_all = {0};
	ORPC_DBG_ALL *  lpOrpcAll = &orpc_all;

	if (!fHookEnabled)
		return; // We should be able to assert that this never happens

	orpc_all.pSignature = rgbClientFillBufferSignature;

	orpc_all.pMessage = pMessage;
	orpc_all.reserved = reserved;
	orpc_all.pUnkProxyMgr = pUnkProxyMgr;
	ASSIGN_REFIID(orpc_all, iid);

	orpc_all.pvBuffer = pvBuffer;
	orpc_all.cbBuffer = cbBuffer;

	
	if ( lpInitArgs == NULL || lpInitArgs->lpIntfOrpcDebug == NULL )
	{
		// Do Orpc debug notification using an exception.
		__try 
		{
			RaiseException(EXCEPTION_ORPC_DEBUG, 0, 1, (LPDWORD)&lpOrpcAll);
		}
		__except(OrpcBreakpointFilter(GetExceptionInformation(), NULL))
		{
			// this just returns.
		}
	}
	else 
	{
		IOrpcDebugNotify __RPC_FAR *lpIntf = lpInitArgs->lpIntfOrpcDebug;

		// call the appropriate method in the registered interface 
		// ( this is typically used by in-proc debuggers)
#if defined(__cplusplus) && !defined(CINTERFACE)
		lpIntf->ClientFillBuffer(lpOrpcAll);
#else
		lpIntf->lpVtbl->ClientFillBuffer(lpIntf, lpOrpcAll);
#endif	
	}
}

//--------------------------------------------------------------------------

// This special value is to ensure backward compatibility with VC 2.0. 
// It is not exposed in the header files. The behavior if this is the value
// in the first four bytes of the debug packet, should be identical to 
// ORPC_DEBUG_ALWAYS.

#define ORPC_COMPATIBILITY_CODE		(0x4252414DL)

void WINAPI DebugORPCClientNotify(
	RPCOLEMESSAGE *		pMessage,
	REFIID				iid,
	void *				reserved,
	IUnknown *			pUnkProxyMgr,
	HRESULT				hresult,
	void *				pvBuffer,
	ULONG				cbBuffer,
	LPORPC_INIT_ARGS	lpInitArgs,
	BOOL				fHookEnabled)
{
	ORPC_DBG_ALL orpc_all = {0};
	ORPC_DBG_ALL * lpOrpcAll = &orpc_all;
	BOOL fRethrow = FALSE;

	// First check to see if the debugger on the other side 
	// wants us to notify this side if the hook is not enabled.
	if (!fHookEnabled)
	{
		if (cbBuffer >= 4)
		{
			LONG orpcCode = *(LONG *)pvBuffer;
			if ( orpcCode == ORPC_DEBUG_IF_HOOK_ENABLED)
				return;		// No notification in this case.
		}
	}

	orpc_all.pSignature = rgbClientNotifySignature;

	orpc_all.pMessage = pMessage;
	orpc_all.reserved = reserved;
	orpc_all.pUnkProxyMgr = pUnkProxyMgr;
	orpc_all.hresult = hresult;
	ASSIGN_REFIID(orpc_all, iid);

	orpc_all.pvBuffer = pvBuffer;
	orpc_all.cbBuffer = cbBuffer;

	if ( lpInitArgs == NULL || lpInitArgs->lpIntfOrpcDebug == NULL )
	{
		if (DebugORPCSetAuto())
		{
			// Do Orpc debug notification using an exception.
			__try 
			{
				RaiseException(EXCEPTION_ORPC_DEBUG, 0, 1, (LPDWORD)&lpOrpcAll);
			}
			__except(OrpcBreakpointFilter(GetExceptionInformation(), &fRethrow))
			{
				// Fall through.
			}

			if (fRethrow)
			{
				// At this point we are sure that a debugger is attached
				// so we raise this exception outside of a __try block.
				RaiseException(EXCEPTION_ORPC_DEBUG, 0, 1, (LPDWORD)&lpOrpcAll);
			}

			DebugORPCRestoreAuto();
		}
	
	}
	else 
	{
		IOrpcDebugNotify __RPC_FAR *lpIntf = lpInitArgs->lpIntfOrpcDebug;

		// call the appropriate method in the registered interface 
		// ( this is typically used by in-proc debuggers)
#if defined(__cplusplus) && !defined(CINTERFACE)
		lpIntf->ClientNotify(lpOrpcAll);
#else
		lpIntf->lpVtbl->ClientNotify(lpIntf, lpOrpcAll);
#endif
	}

}

//--------------------------------------------------------------------------

void WINAPI DebugORPCServerNotify(
	RPCOLEMESSAGE *		pMessage,
	REFIID				iid,
	IRpcChannelBuffer *	pChannel,
	void *				pInterface,
	IUnknown *			pUnkObject,
	void *				pvBuffer,
	ULONG				cbBuffer,
	LPORPC_INIT_ARGS	lpInitArgs,
	BOOL				fHookEnabled)

{
	ORPC_DBG_ALL orpc_all = {0};
	ORPC_DBG_ALL * lpOrpcAll = &orpc_all;
	BOOL fRethrow = FALSE;

	// First check to see if the debugger on the other side 
	// wants us to notify this side if the hook is not enabled.
	if (!fHookEnabled)
	{
		if (cbBuffer >= 4)
		{
			LONG orpcCode = *(LONG *)pvBuffer;
			if ( orpcCode == ORPC_DEBUG_IF_HOOK_ENABLED)
				return;		// No notification in this case.
		}
	}

	orpc_all.pSignature = rgbServerNotifySignature;

	orpc_all.pMessage = pMessage;
	orpc_all.pChannel = pChannel;
	orpc_all.pInterface = pInterface;
	orpc_all.pUnkObject = pUnkObject;
	ASSIGN_REFIID(orpc_all, iid);
	
	orpc_all.pvBuffer = pvBuffer;
	orpc_all.cbBuffer = cbBuffer;

	if ( lpInitArgs == NULL || lpInitArgs->lpIntfOrpcDebug == NULL )
	{
		if (DebugORPCSetAuto())
		{
			// Do Orpc debug notification using an exception.
			__try 
			{
				RaiseException(EXCEPTION_ORPC_DEBUG, 0, 1, (LPDWORD)&lpOrpcAll);
			}
			__except(OrpcBreakpointFilter(GetExceptionInformation(), &fRethrow))
			{
				// Fall through
			}

			if (fRethrow)
			{
				// At this point we are sure that a debugger is attached
				// so we raise this exception outside of a __try block.
				RaiseException(EXCEPTION_ORPC_DEBUG, 0, 1, (LPDWORD)&lpOrpcAll);
			}

			DebugORPCRestoreAuto();
		}
	
	}
	else 
	{
		IOrpcDebugNotify __RPC_FAR *lpIntf = lpInitArgs->lpIntfOrpcDebug;

		// call the appropriate method in the registered interface 
		// ( this is typically used by in-proc debuggers)
#if defined(__cplusplus) && !defined(CINTERFACE)
		lpIntf->ServerNotify(lpOrpcAll);
#else
		lpIntf->lpVtbl->ServerNotify(lpIntf, lpOrpcAll);
#endif
	}

}

//--------------------------------------------------------------------------

ULONG WINAPI DebugORPCServerGetBufferSize(
	RPCOLEMESSAGE *		pMessage,
	REFIID				iid,
	IRpcChannelBuffer *	pChannel,
	void *				pInterface,
	IUnknown *			pUnkObject,
	LPORPC_INIT_ARGS	lpInitArgs,
	BOOL				fHookEnabled)

{
	ULONG	cbBuffer = 0;
	ORPC_DBG_ALL orpc_all = {0};
	ORPC_DBG_ALL * lpOrpcAll = &orpc_all;

	if (!fHookEnabled)
		return 0; // We should be able to assert that this never happens.

	orpc_all.pSignature = rgbServerGetBufferSizeSignature;

	orpc_all.pMessage = pMessage;
	orpc_all.pChannel = pChannel;
	orpc_all.pInterface = pInterface;
	orpc_all.pUnkObject = pUnkObject;
	orpc_all.lpcbBuffer = &cbBuffer;
	ASSIGN_REFIID(orpc_all, iid);

	if ( lpInitArgs == NULL || lpInitArgs->lpIntfOrpcDebug == NULL )
	{
		// Do Orpc debug notification using an exception.
		__try 
		{
			RaiseException(EXCEPTION_ORPC_DEBUG, 0, 1, (LPDWORD)&lpOrpcAll);
		}
		__except(OrpcBreakpointFilter(GetExceptionInformation(), NULL))
		{
			// this just goes down to the return.
		}
	}
	else 
	{
		IOrpcDebugNotify __RPC_FAR *lpIntf = lpInitArgs->lpIntfOrpcDebug;

		// call the appropriate method in the registered interface 
		// ( this is typically used by in-proc debuggers)
#if defined(__cplusplus) && !defined(CINTERFACE)
		lpIntf->ServerGetBufferSize(lpOrpcAll);
#else
		lpIntf->lpVtbl->ServerGetBufferSize(lpIntf, lpOrpcAll);
#endif
	}

	return cbBuffer;
}

//--------------------------------------------------------------------------

void WINAPI DebugORPCServerFillBuffer(
	RPCOLEMESSAGE *		pMessage,
	REFIID				iid,
	IRpcChannelBuffer *	pChannel,
	void *				pInterface,
	IUnknown *			pUnkObject,
	void *				pvBuffer,
	ULONG				cbBuffer,
	LPORPC_INIT_ARGS	lpInitArgs,
	BOOL				fHookEnabled)
{
	ORPC_DBG_ALL orpc_all = {0};
	ORPC_DBG_ALL * lpOrpcAll = &orpc_all;

	if (!fHookEnabled)
		return;	// We should be able to assert that this never happens.

	orpc_all.pSignature = rgbServerFillBufferSignature;

	orpc_all.pMessage = pMessage;
	orpc_all.pChannel = pChannel;
	orpc_all.pInterface = pInterface;
	orpc_all.pUnkObject = pUnkObject;
	ASSIGN_REFIID(orpc_all, iid);

	orpc_all.pvBuffer = pvBuffer;
	orpc_all.cbBuffer = cbBuffer;

	if ( lpInitArgs == NULL || lpInitArgs->lpIntfOrpcDebug == NULL )
	{
		// Do Orpc debug notification using an exception.
		__try 
		{
			RaiseException(EXCEPTION_ORPC_DEBUG, 0, 1, (LPDWORD)&lpOrpcAll);
		}
		__except(OrpcBreakpointFilter(GetExceptionInformation(), NULL))
		{
			// this just returns.
		}
	}
	else 
	{
		IOrpcDebugNotify __RPC_FAR *lpIntf = lpInitArgs->lpIntfOrpcDebug;

		// call the appropriate method in the registered interface 
		// ( this is typically used by in-proc debuggers)
#if defined(__cplusplus) && !defined(CINTERFACE)
		lpIntf->ServerFillBuffer(lpOrpcAll);
#else
		lpIntf->lpVtbl->ServerFillBuffer(lpIntf, lpOrpcAll);
#endif
	}
}

// WARNING: there is no way to "pop" to the previously active code_seg:
// this will revert to what the code seg was when compilation began.
#pragma code_seg()


