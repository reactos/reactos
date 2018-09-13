//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       testevnt.hxx
//
//  Contents:   CEventCallBack class.
//
//-------------------------------------------------------------------------

#ifndef _TESTEVNT_HXX_
#define _TESTEVNT_HXX_ 1

class CEventCallBack
{
public:

    virtual int Event(LPCTSTR pchEvent, BOOL fReset = FALSE) = 0;
};

#endif _TESTEVNT_HXX_