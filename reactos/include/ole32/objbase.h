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

#include "guiddef.h"

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

#endif
