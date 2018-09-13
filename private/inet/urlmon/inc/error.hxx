//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992
//
//  File:	error.hxx
//
//  Contents:	Error code handler routines
//
//  History:	19-Mar-92	DrewB	Created
//
//---------------------------------------------------------------

#ifndef __ERROR_HXX__
#define __ERROR_HXX__

#if DBG == 1
#define ErrJmp(comp, label, errval, var) \
{\
    var = errval;\
    comp##DebugOut((DEB_IERROR, "Error %lX at %s:%d\n",\
		    (unsigned long)var, __FILE__, __LINE__));\
    goto label;\
}
#else
#define ErrJmp(comp, label, errval, var) \
{\
    var = errval;\
    goto label;\
}
#endif

#endif // #ifndef __ERROR_HXX__
