#ifndef I_TYPENAV_HXX_
#define I_TYPENAV_HXX_
#pragma INCMSG("--- Beg 'typenav.hxx'")

//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995.
//
//  File:       typenav.hxx
//
//  Contents:   Navigation of ITypeInfo.
//
//  Classes:    CTypeInfoNav        -- Navigates an IDispatch's ITypeInfo
//                                     looking for bound properties.
//
//
//              Usage:
//              ======
//              LPVARDESC   pvDesc = 0;
//
//              CTypeInfoNav cTINav;
//              hr = cTINav.InitIDispatch(oleControl, 0, VARFLAG_FBINDABLE |
//                                                       VARFLAG_FDISPLAYBIND);
//              if (!hr) {
//                  while (SUCCEEDED(cTINav.Next())) {
//                      pvDesc = cTINav.GetVarD();
//
//                      ...Look at the VARDESC data...
//
//                  }   // end while
//              }
//
//
//  Functions:  (none)
//
//  History:    03-28-95   terrylu   Created
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Class:      CTypeInfoNav
//
//  Purpose:    ITypeInfo navigation
//
//  History:    03-28-95   terrylu   Created
//
//----------------------------------------------------------------------------
class CTypeInfoNav
{
public:
    CTypeInfoNav ();
    ~CTypeInfoNav ();
    
    //
    // Object which supports IDispatch, to traverse ITypeInfo.  Return the
    // ITypeInfo, this interface is AddRef'd so Release is necessary on the
    // pITypeInfo.  If pITypeInfo is NULL then it is not returned.
    //
    HRESULT         InitIDispatch (IUnknown * pUnk,
                                   ITypeInfo ** pITypeInfo = 0,
                                   WORD wVFFilter = 0,
                                   DISPID dispid = DISPID_UNKNOWN);

    HRESULT         InitIDispatch (IDispatch * pDisp,
                                   ITypeInfo ** pITypeInfo = 0,
                                   WORD wVFFilter = 0,
                                   DISPID dispid = DISPID_UNKNOWN);

    //
    // Traverse the ITypeInfo passed in.
    //
    HRESULT         InitITypeInfo (ITypeInfo * pTypeInfo,
                                   WORD wVFFilter = 0,
                                   DISPID dispid = DISPID_UNKNOWN);

    HRESULT         Next ();

    BOOL            IsDualInterface ()
        { return _fIsDual; }

    //
    // Returns the Current VARDESC if index is valid and if we're not dual
    // interface.  This VARDESC pointer is only valid until the Next() is called
    // then the VARDESC being pointed to is Released.
    //
    VARDESC *       getVarD ()
        { return _fFuncDesc ? 0 : _pVD; }

    //
    // Returns the Current FUNCDESC if index is valid and if we're dual
    // interface.  This FUNCDESC pointer is only valid until the Next() is
    // called then the FUNCDESC being pointed to is Released.
    //
    FUNCDESC *      getFuncD ()
        { return _fFuncDesc ? _pFD : 0; }

    //
    // Returns ITypeInfo pointer this pointer is not addref'd, therefore, a
    // Release is not required.
    //
    ITypeInfo *     getITypeInfo ()
        { return _pTI; }

private:
#if DBG == 1
    void DbgUserDefined ();
#endif

    ITypeInfo*      _pTI;                       // ITypeInfo pointer.
    WORD            _wVarCount;                 // Number of data properties.
    WORD            _wFuncCount;                // Number of functions.
    UINT            _uIndex;                    // Current index (iterator).
    WORD            _wVarFlagsFilter;           // Filter to determine if valid.
    DISPID          _dispid;                    // dispid to match.
    unsigned int    _fIsDual:1;                 // true for a dual interface
    unsigned int    _fFuncDesc :1;              // Tag describing below union.
    union {
        VARDESC *   _pVD;                       // VARDESC of current index.
        FUNCDESC *  _pFD;                       // FUNCDESC of current index.
    };
};

#pragma INCMSG("--- End 'typenav.hxx'")
#else
#pragma INCMSG("*** Dup 'typenav.hxx'")
#endif

