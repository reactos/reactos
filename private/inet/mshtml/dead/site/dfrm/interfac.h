//-------------------------------------------------------------------------------------------
//
// File: interface.h
//
// Copyright: (c) 1994-1995, Microsoft Corporation
// All rights Reserved.
// Information contained herein is Proprietary and Confidential.
//
// Contents: Usefull Macros. Typedefs, Etc.
//
// This file is always included in the header files. So we can rely on the fact that this is the 1st #include file.
//
// Functions: None.
//
// History: 6/24/94 alexa Created.
//
//-------------------------------------------------------------------------------------------

#ifndef	_INTERFACE_H_
#	define _INTERFACE_H_ 1

#	define interface	struct

//	Structure forwarding declaration macro.
#	define DEFINE_STRUCT(T) struct T; typedef T* P##T;

//	Class forwarding declaration macro.
#	define DEFINE_CLASS(T) class C##T; typedef C##T* PC##T;

//	Interface class forwarding declaration macro.
#	define DEFINE_INTERFACE(T)	\
	struct I##T; 			\
	typedef I##T* PI##T; 		\
	EXTERN_C const IID IID_I##T;

//	Class abreviation declaration.    
#	define ABBREVIATION(T,N) typedef T N; typedef N* P##N;

#	ifdef _DEBUG
#		define	DEBUGMSG(S)		AfxMessageBox(S, MB_OK, 0);
#		define	ERRORMSG(S)		AfxMessageBox(S, MB_OK, 0);
#	else _DEBUG
#		define	DEBUGMSG(S)		;
#		define	ERRORMSG(S)		;
#	endif _DEBUG



#endif	// _INTERFACE_H_
