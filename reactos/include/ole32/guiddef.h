/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            include\ole32\guiddef.h
 * PURPOSE:         Guid definition macros
 * PROGRAMMER:      jurgen van gael [jurgen.vangael@student.kuleuven.ac.be]
 * UPDATE HISTORY:
 *                  Created 05/01/2001
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
#ifndef _GUIDDEF_H
#define _GUIDDEF_H

#include <string.h>


#ifndef EXTERN_C
	#ifdef __cplusplus
		#define EXTERN_C    extern "C"
	#else
		#define EXTERN_C    extern
	#endif
#endif


//	guid definition
#ifndef GUID_DEFINED
	#define GUID_DEFINED
	typedef struct _GUID {
		unsigned long  Data1;
		unsigned short Data2;
		unsigned short Data3;
		unsigned char  Data4[ 8 ];
	} GUID;
	typedef GUID*		LPGUID;
	typedef const GUID*	LPCGUID;
#endif


//	guid definition macro
#ifdef DEFINE_GUID
	#undef DEFINE_GUID
#endif

#ifdef INITGUID
	#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
		const GUID name = {l, w1, w2, {b1, b2,  b3,  b4,  b5,  b6,  b7,  b8}}
#else
	#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
		const GUID name
#endif
#define DEFINE_OLEGUID(name, l, w1, w2) DEFINE_GUID(name, l, w1, w2, 0xC0,0,0,0,0,0,0,0x46)


//	IID section
typedef	GUID		IID;
typedef	IID*		LPIID;
#define IsEqualIID(riid1, riid2) IsEqualGUID(riid1, riid2)


//	CLSID section
typedef GUID		CLSID;
typedef CLSID*		LPCLSID;
#define IsEqualCLSID(rclsid1, rclsid2) IsEqualGUID(rclsid1, rclsid2)

//	FMTID
typedef	GUID		FMTID;
typedef	FMTID*		LPFMTID;
#define	IsEqualFMTID(rfmtid1, rfmtid2) IsEqualGUID(rfmtid1, rfmtid2)


//	REFGUID section
#ifndef _REFGUID_DEFINED
	#define _REFGUID_DEFINED
	#ifdef __cplusplus
		#define REFGUID const GUID &
	#else
		#define REFGUID const GUID *
	#endif
#endif

//	REFIID section
#ifndef _REFIID_DEFINED
	#define _REFIID_DEFINED
	#ifdef __cplusplus
		#define REFIID const IID &
	#else
		#define REFIID const IID *
	#endif
#endif

//	REFCLSID section
#ifndef _REFCLSID_DEFINED
	#define _REFCLSID_DEFINED
	#ifdef __cplusplus
		#define REFCLSID const IID &
	#else
		#define REFCLSID const IID *
	#endif
#endif

//	REFFMTID section
#ifndef _REFFMTID_DEFINED
	#define _REFFMTID_DEFINED
	#ifdef __cplusplus
		#define REFFMTID const IID &
	#else
		#define REFFMTID const IID *
	#endif
#endif


//	compare functions for GUID
#ifdef __cplusplus
	//	cpp versions
	__inline int InlineIsEqualGUID(REFGUID rguid1, REFGUID rguid2)
	{
		return(((unsigned long *) &rguid1)[0] == ((unsigned long *) &rguid2)[0] &&
			((unsigned long *) &rguid1)[1] == ((unsigned long *) &rguid2)[1] &&
			((unsigned long *) &rguid1)[2] == ((unsigned long *) &rguid2)[2] &&
			((unsigned long *) &rguid1)[3] == ((unsigned long *) &rguid2)[3]);
	}
	__inline int IsEqualGUID(REFGUID rguid1, REFGUID rguid2)
	{
		return !memcmp(&rguid1, &rguid2, sizeof(GUID));
	}
#else
	//	c versions
	#define InlineIsEqualGUID(rguid1, rguid2)									\
		(((unsigned long *) rguid1)[0] == ((unsigned long *) rguid2)[0] &&		\
		((unsigned long *) rguid1)[1] == ((unsigned long *) rguid2)[1] &&		\
		((unsigned long *) rguid1)[2] == ((unsigned long *) rguid2)[2] &&		\
		((unsigned long *) rguid1)[3] == ((unsigned long *) rguid2)[3])

	#define IsEqualGUID(rguid1, rguid2) (!memcmp(rguid1, rguid2, sizeof(GUID)))
#endif

//	use the inline version???
#ifdef __INLINE_ISEQUAL_GUID
	#define IsEqualGUID(rguid1, rguid2)	InlineIsEqualGUID(rguid1, rguid2)
#endif


//	compare functions for IID CLSID
#define IsEqualIID(riid1, riid2) IsEqualGUID(riid1, riid2)
#define IsEqualCLSID(rclsid1, rclsid2) IsEqualGUID(rclsid1, rclsid2)

//	c++ helper functions
#if !defined _SYS_GUID_OPERATOR_EQ_ && !defined _NO_SYS_GUID_OPERATOR_EQ_
	#define _SYS_GUID_OPERATOR_EQ_
	#ifdef __cplusplus
		__inline int operator==(REFGUID guidOne, REFGUID guidTwo)
		{
			return IsEqualGUID(guidOne ,guidTwo);
		}

		__inline int operator!=(REFGUID guidOne, REFGUID guidTwo)
		{
			return !(guidOne == guidTwo);
		}
	#endif
#endif

#endif
