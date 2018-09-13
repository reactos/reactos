//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1997 - 1998.
//
//  File:       scrpctrl.hxx
//
//  History:    19-Jan-1998     sramani     Created
//
//  Contents:   CScriptControl definition
//
//----------------------------------------------------------------------------

#ifndef I_SCRPCTRL_HXX_
#define I_SCRPCTRL_HXX_
#pragma INCMSG("--- Beg 'scrpctrl.hxx'")

class CScriptlet;

#define _hxx_
#include "scrptlet.hdl"

MtExtern(CScriptControl)

/////////////////////////////////////////////////////////////////////////////
// CScriptControl
class CScriptControl : public CBase,
                       public IWBScriptControl
{
    DECLARE_CLASS_TYPES(CScriptControl, CBase)

public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CScriptControl))
    CScriptControl(CScriptlet *pScriptlet) { _pScriptlet = pScriptlet; }

    DECLARE_PLAIN_IUNKNOWN(CScriptControl)
    DECLARE_PRIVATE_QI_FUNCS(CScriptControl)
    DECLARE_DERIVED_DISPATCH(CBase);

    #define _CScriptControl_
    #include "scrptlet.hdl"

    DECLARE_CLASSDESC_MEMBERS
    
private:
    CScriptlet *_pScriptlet;
};

#pragma INCMSG("--- End 'scrpctrl.hxx'")
#else
#pragma INCMSG("*** Dup 'scrpctrl.hxx'")
#endif
