//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	asyncerr.hxx
//
//  Contents:	New error codes for async storage
//
//  Classes:	
//
//  Functions:	
//
//  History:	03-Jan-96	PhilipLa	Created
//
//  Notes:  At some point, all the error codes in the file need to end
//          up in winerror.h.
//
//----------------------------------------------------------------------------

#ifndef __ASYNCERR_HXX__
#define __ASYNCERR_HXX__

#ifndef _HRESULT_TYPEDEF_
#define _HRESULT_TYPEDEF_(sc) sc
#endif


#ifndef STG_E_PENDING
#define STG_E_PENDING _HRESULT_TYPEDEF_(0x80030200L)
#endif

#ifndef STG_E_INCOMPLETE
#define STG_E_INCOMPLETE _HRESULT_TYPEDEF_(0x80030201L)
#endif

#ifndef STG_E_TERMINATED
#define STG_E_TERMINATED _HRESULT_TYPEDEF_(0x80030202L)
#endif


#endif // #ifndef __ASYNCERR_HXX__
