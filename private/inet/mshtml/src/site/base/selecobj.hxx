//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       selecobj.hxx
//
//  Contents:   Declaration of the CSelectionObj class.
//
//  Classes:    CSelectionObj
//
//----------------------------------------------------------------------------

#ifndef I_SELECOBJ_HXX_
#define I_SELECOBJ_HXX_
#pragma INCMSG("--- Beg 'selecobj.hxx'")

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#define _hxx_
#include "selecobj.hdl"

#define OP_SELECT_QUERY 0x1
#define OP_SELECT_EMPTY 0x2

MtExtern(CSelectionObject)

class CSelectionObject :  public CBase,
                          public IHTMLSelectionObject,
                          public IDispatchEx
{
    typedef CBase super;

public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CSelectionObject))

    CSelectionObject( CDoc * pDoc);
    ~CSelectionObject();

            //IDispatch implementations:
    DECLARE_DERIVED_DISPATCHEx2( CBase );
    DECLARE_PLAIN_IUNKNOWN( CSelectionObject );
    DECLARE_PRIVATE_QI_FUNCS(CBase)

    // Need to support getting the atom table for expando support on IDEX2.
    virtual CAtomTable * GetAtomTable (BOOL *pfExpando = NULL)
        { if (pfExpando)
            {
                *pfExpando = _pDoc->_fExpando;
            }
          return &_pDoc->_AtomTable; }

            // interface definitions
    #define _CSelectionObject_
    #include "selecobj.hdl"

            // administrative functions:
    HRESULT CloseErrorInfo(HRESULT hr)
                { return _pDoc->CloseErrorInfo(hr); };

        // helper fns
    HRESULT GetType( htmlSelection *pSelType );


protected:
    DECLARE_CLASSDESC_MEMBERS;

    CDoc              * _pDoc;    // to get to the document's selection
}; // end class CSelection Obj

#pragma INCMSG("--- End 'selecobj.hxx'")
#else
#pragma INCMSG("*** Dup 'selecobj.hxx'")
#endif
