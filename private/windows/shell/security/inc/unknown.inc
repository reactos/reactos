//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       unknown.inc
//
//  Magic include file to help with implementing IUnknown
//
//--------------------------------------------------------------------------


/*----------------------------------------------------------------------------
/ Macros to handle AddRef & Release (by calling CUnknown methods)
/----------------------------------------------------------------------------*/

STDMETHODIMP_(ULONG) CLASS_NAME::AddRef()
{
    return CUnknown::HandleAddRef();
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP_(ULONG) CLASS_NAME::Release()
{
    return CUnknown::HandleRelease();
}
