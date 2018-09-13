//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       dbinding.hxx
//
//  Contents:
//
//  History:
//
//  30-Jul-96   TerryLu     Creation
//
//----------------------------------------------------------------------------

#ifndef I_DISPBIND_HXX_
#define I_DISPBIND_HXX_
#pragma INCMSG("--- Beg 'dispbind.hxx'")

#ifndef X_DBINDING_HXX_
#define X_DBINDING_HXX_
#include "dbinding.hxx"
#endif

class CDispatchInstance;        // Forward ref.

// IDispatch data source.
class CDispatchInstance : public CInstance
{
public:
    CDispatchInstance() :
      _pDispSrc(NULL)
        {   }

    ~CDispatchInstance()
        {   }

#if DBG == 1
    virtual INST_TYPE Kind()
        { return IT_DISPATCH; }
#endif

    void SetDispatch(IDispatch *pDispSrc);

    IDispatch * GetDispatch()
        { return _pDispSrc; }

    DEBUG_METHODS;

private:
    IDispatch      *_pDispSrc;
};


#if 0
class CDispatchDBAgent : public CDataBindAgent
{
public:
#if DBG == 1
    virtual AGENT_KIND Kind()
        { return AK_DISPATCH; }
#endif

private:
    CDispatchXferThunk  _dispXT;
    CDispatchInstance   _srcInstance;
    CXfer               _xfer;
};
#endif

#pragma INCMSG("--- End 'dispbind.hxx'")
#else
#pragma INCMSG("*** Dup 'dispbind.hxx'")
#endif
