/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            include\ole32\objbase.h
 * PURPOSE:         Header file for the ole32.dll implementation
 * PROGRAMMER:      jurgen van gael [jurgen.vangael@student.kuleuven.ac.be]
 * UPDATE HISTORY:
 *                  Created 01/05/2001
 */
/********************************************************************


This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.


********************************************************************/
#ifndef _OBJBASE
#define _OBJBASE

#include <ole32\guiddef.h>


#ifndef EXTERN_C
	#ifdef __cplusplus
		#define EXTERN_C    extern "C"
	#else
		#define EXTERN_C    extern
	#endif
#endif

#define	STDMETHODCALLTYPE		__stdcall
#define	STDMETHODVCALLTYPE		__cdecl

#define	STDAPICALLTYPE			__stdcall
#define	STDAPIVCALLTYPE			__cdecl


#define	STDAPI					EXTERN_C HRESULT STDMETHODCALLTYPE
#define	STDAPI_(type)			EXTERN_C type STDAPICALLTYPE

#define	STDMETHODIMP			HRESULT STDMETHODCALLTYPE
#define	STDMETHODIMP_(type)		type STDMETHODCALLTYPE


//	variable argument lists versions
#define STDAPIV                 EXTERN_C HRESULT STDAPIVCALLTYPE
#define STDAPIV_(type)          EXTERN_C type STDAPIVCALLTYPE

#define STDMETHODIMPV           HRESULT STDMETHODVCALLTYPE
#define STDMETHODIMPV_(type)    type STDMETHODVCALLTYPE

#ifndef WINOLEAPI
	#define WINOLEAPI        STDAPI
	#define WINOLEAPI_(type) STDAPI_(type)
#endif

//	context in which to create COM objects
typedef enum tagCLSCTX
{
	CLSCTX_INPROC_SERVER	= 0x1,
	CLSCTX_INPROC_HANDLER	= 0x2,
	CLSCTX_LOCAL_SERVER		= 0x4,
	CLSCTX_INPROC_SERVER16	= 0x8,
	CLSCTX_REMOTE_SERVER	= 0x10,
	CLSCTX_INPROC_HANDLER16	= 0x20,
	CLSCTX_INPROC_SERVERX86	= 0x40,
	CLSCTX_INPROC_HANDLERX86= 0x80,
	CLSCTX_ESERVER_HANDLER	= 0x100,
	CLSCTX_RESERVED			= 0x200,
	CLSCTX_NO_CODE_DOWNLOAD	= 0x400,
	CLSCTX_NO_WX86_TRANSLATION	= 0x800,
	CLSCTX_NO_CUSTOM_MARSHAL	= 0x1000,
	CLSCTX_ENABLE_CODE_DOWNLOAD	= 0x2000,
	CLSCTX_NO_FAILURE_LOG		= 0x4000
}CLSCTX;
#define CLSCTX_INPROC           (CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER)

//	COM initialization flags, passed to CoInitialize.
typedef enum tagCOINIT
{
	COINIT_APARTMENTTHREADED	=	0x2,		//	apartement threaded model
	COINIT_MULTITHREADED      = 0x0,			//	OLE calls objects on any thread
	COINIT_DISABLE_OLE1DDE    = 0x4,			//	don't use DDE for Ole1 support
	COINIT_SPEED_OVER_MEMORY  = 0x8,			//	trade memory for speed*/
}COINIT;

//
//	COM interface declaration macros [from the wine implementation
//
//	These macros declare interfaces for both C and C++ depending for what
//	language you are compiling
//
//	DECLARE_INTERFACE(iface): declare not derived interface
//	DECLARE_INTERFACE(iface, baseiface): declare derived interface
//
//	Use CINTERFACE if you want to force a c style interface declaration,
//	else, the compiler will look at the source file extension
//
#if	defined(__cplusplus) && !defined(CINTERFACE)
	//	define the c++ macros
	#define interface					struct
	#define STDMETHOD(method)			virtual	HRESULT	STDMETHODCALLTYPE	method
	#define STDMETHOD_(type,method)		virtual type	STDMETHODCALLTYPE	method
	#define STDMETHODV(method)			virtual HRESULT	STDMETHODVCALLTYPE	method
	#define STDMETHODV_(type,method)	virtual type	STDMETHODVCALLTYPE	method
	#define PURE						= 0
	#define THIS_
	#define THIS
	#define DECLARE_INTERFACE(iface)				interface iface
	#define DECLARE_INTERFACE_(iface, baseiface)	interface iface : public baseiface

#else
	//	define the c macros
	#define	interface				struct
	#define	STDMETHOD(method)		HRESULT	(STDMETHODCALLTYPE*		method)
	#define	STDMETHOD_(type,method)	type	(STDMETHODCALLTYPE*		method)
	#define	STDMETHODV(method)		HRESULT	(STDMETHODVCALLTYPE*	method)
	#define	STDMETHODV_(type,method)type	(STDMETHODVCALLTYPE*	method)
	#define PURE
	#define THIS_					INTERFACE	FAR*	This,
	#define THIS                    INTERFACE	FAR*	This

	#ifdef CONST_VTABLE
		#define DECLARE_INTERFACE(iface)	typedef	interface	iface	{	\
					const	struct	iface##Vtbl	FAR*	lpVtbl;				\
					}	iface;												\
					typedef	const	struct	iface##Vtbl	iface##Vtbl;		\
					const	struct	iface##Vtbl
	#else
		#define DECLARE_INTERFACE(iface)	typedef	interface	iface	{	\
					struct	iface##Vtbl	FAR*	lpVtbl;						\
					}	iface;												\
					typedef	struct	iface##Vtbl	iface##Vtbl;				\
					struct	iface##Vtbl
	#endif
	
	#define DECLARE_INTERFACE_(iface, baseiface)    DECLARE_INTERFACE(iface)
#endif



DEFINE_OLEGUID(IID_IUnknown,		0x00000000L, 0, 0);
DEFINE_OLEGUID(IID_IClassFactory,	0x00000001L, 0, 0);

//	IUnknown
//
DECLARE_INTERFACE(IUnknown)
{
	// *** IUnknown methods ***
	STDMETHOD(QueryInterface)	(THIS_ REFIID riid, VOID* FAR* ppUnk) PURE;
	STDMETHOD_(ULONG,AddRef)	(THIS) PURE;
	STDMETHOD_(ULONG,Release)	(THIS) PURE;
};

//	IClassFactory
//
DECLARE_INTERFACE_(IClassFactory, IUnknown)
{
	// *** IUnknown methods ***
	STDMETHOD(QueryInterface)	(THIS_ REFIID iid, VOID* FAR* ppvObject) PURE;
	STDMETHOD_(ULONG,AddRef)	(THIS) PURE;
	STDMETHOD_(ULONG,Release)	(THIS) PURE;

	// *** IClassFactory methods ***
	STDMETHOD(CreateInstance)	(THIS_ IUnknown* pUnkOuter, REFIID riid, VOID* FAR* ppvObject) PURE;
	STDMETHOD(LockServer)		(THIS_ BOOL fLock) PURE;
};


//
//	COM API definition
//
WINOLEAPI_(VOID)	CoUninitialize();
WINOLEAPI_(VOID)	CoFreeAllLibraries();
WINOLEAPI_(DWORD)	CoBuildVersion();
WINOLEAPI	CoCreateInstance(REFCLSID rclsid, IUnknown* pUnkOuter, DWORD dwClsContext, REFIID riid, VOID** ppv);
WINOLEAPI	CoGetClassObject(REFCLSID rclsid, DWORD dwClsContext, VOID* pvReserved, REFIID riid, VOID** ppv);
WINOLEAPI	CoInitializeEx(VOID* lpReserved, DWORD dwCoInit);
WINOLEAPI	CoInitialize(VOID* lpReserved);



#endif
