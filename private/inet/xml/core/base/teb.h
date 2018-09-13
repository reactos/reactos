/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef TEB_H_
#include "teb.hxx"
#endif // TEB_H_
#ifdef FOSSIL_CODE

// we define the TEB here since we need it in Win95 as well
struct TEB
{
    PVOID ExceptionList;    // this is a real structure pointer but we don't care...
    PVOID StackBase;
    PVOID StackLimit;
};

EXTERN_C DLLEXPORT TEB * getTEB();
#endif // FOSSIL_CODE
