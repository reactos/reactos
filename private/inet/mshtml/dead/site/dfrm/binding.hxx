//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994
//
//  File:       binding.hxx
//
//  Contents:   Definition of binding class
//
//  Classes:    CBindSource
//
//  History:    08-Nov-94   IstvanC     Created
//              25-Feb-95   JerryD      Use DIRT; no VARIANT in BindSource
//              13-Apr-95   TerryLu     Changed Set/Get Data routines to take
//                                      a DataLayerAccessor instead of BINDID.
//
//----------------------------------------------------------------------------


#ifndef _BINDING_HXX_
#define _BINDING_HXX_   1


// BINDID is just a cookie to identify the host object for the data source

typedef HACCESSOR BINDID;

// For every bound property objects have a bind source pointer property and
// a BINDID property associated with. In our controls the default behavior is
// that the BINDID is stored in the control template and the bind source pointer
// is NULL. In this case the control will call the immediate layout parent to
// get and set data.

//+---------------------------------------------------------------------------
//
//  Class:      CBindSource
//
//  Purpose:    Base class for simple property binding
//
//----------------------------------------------------------------------------
/*
class CBindSource
{
public:
    virtual HRESULT GetData (CDataLayerAccessor * dla, LPVOID lpv) = 0;
    virtual HRESULT SetData (CDataLayerAccessor * dla, LPVOID lpv) = 0;
};
*/

class CBaseFrame;

class CCreateInfo
{
public:
    CCreateInfo () :
      _pBindSource(NULL), _bindid(NULL), _fSetBindid(FALSE)
       {   }

    CBaseFrame *    _pBindSource; //CBindSource *   _pBindSource;
    BINDID          _bindid;
    BOOL            _fSetBindid;
};

#endif _BINDING_HXX_
