/* 
   basetyps.h

   definitions for basic types used to develop COM
   macro definitions for interface declaration

   Copyright (C) 1996 Free Software Foundation, Inc.

   Author:  Jurgen Van Gael <jurgen.vangael@student.kuleuven.ac.be>
   Date: january 2001

   This file is part of the Windows32 API Library.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   If you are interested in a warranty or support for this source code,
   contact Scott Christley <scottc@net-community.com> for more information.
   
   You should have received a copy of the GNU Library General Public
   License along with this library; see the file COPYING.LIB.
   If not, write to the Free Software Foundation, 
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
#ifndef _BASETYPS
#define _BASETYPS

#ifndef EXTERN_C
#ifdef __cplusplus
	#define EXTERN_C    extern "C"
#else
	#define EXTERN_C    extern
#endif
#endif


#ifndef _ERROR_STATUS_T_DEFINED
typedef unsigned long error_status_t;
#define _ERROR_STATUS_T_DEFINED
#endif


#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
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


/*************************	interface declaration	**********************************
 *
 *      Here are some macros you can use to define interfaces. These macro's are
 *		usefull because they declare an interface for a c++ implementation or a
 *		a c implementation. The c interface will be made when the user compiles a
 *		.c file. The c++ interface will be declared when a .cpp file is compiled.
 *
 *      use DECLARE_INTERFACE(iface) when the interface does not derive from another
 *      use DECLARE_INTERFACE_(iface, baseiface) when the interface derives from another
 *		interface
 */
#if	defined(__cplusplus) && !defined(CINTERFACE)
	//	define the c++ macros
	#define interface					struct
	#define STDMETHOD(method)			virtual	HRESULT	STDMETHODCALLTYPE	method
	#define STDMETHOD_(type,method)		virtual type	STDMETHODCALLTYPE	method
	#define STDMETHODV(method)			virtual HRESULT	STDMETHODVCALLTYPE	method
	#define STDMETHODV_(type,method)	virtual type	STDMETHODVCALLTYPE	method
	#define PURE						= 0
	#define THIS_
	#define THIS						void
	#define DECLARE_INTERFACE(iface)	interface DECLSPEC_NOVTABLE iface
	#define DECLARE_INTERFACE_(iface, baseiface)	interface DECLSPEC_NOVTABLE iface : public baseiface

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

