//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       commit.hxx
//
//  Contents:   Commit engine header file
//
//  History:    07-05-96  AnandRa   Created
//
//-------------------------------------------------------------------------

#ifndef I_COMMIT_HXX_
#define I_COMMIT_HXX_
#pragma INCMSG("--- Beg 'commit.hxx'")

#ifndef X_OTHRDISP_H_
#define X_OTHRDISP_H_
#include "othrdisp.h"
#endif

class CCommitEngine;
class CCommitHolder;

// Function to create a commit Holder
HRESULT EnsureCommitHolder(DWORD_PTR dwID, CCommitHolder **ppHolder);

MtExtern(CCommitHolder)
MtExtern(CCommitEngine)

//+---------------------------------------------------------------------------
//
//  Class:      CCommitHolder
//
//+---------------------------------------------------------------------------

class CCommitHolder
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CCommitHolder))
    // ctor/dtor
    CCommitHolder(DWORD_PTR dwID);
    ~CCommitHolder();
    
    ULONG       AddRef();
    void        Release();

    // Public methods
    HRESULT     GetEngine(
        long cObjects, 
        IDispatch **ppDisp, 
        CCommitEngine **ppEngine);

    // Data members
    CPtrAry<CCommitEngine *>    _aryEngine; // Current array of engines
    DWORD_PTR                   _dwID;      // ID of this commit holder
    unsigned long               _ulRefs;    // Ref count
};



//+------------------------------------------------------------------------
//
//  EVAL structs are used to list the allowed values for enumerations.
//  EVALINIT structs are used to initialize arrays of EVAL's.
//
//-------------------------------------------------------------------------

struct EVAL
{
    BSTR        bstr;
    int         value;
};

struct EVALINIT
{
    LPTSTR      pstr;
    long        value;
};

//+------------------------------------------------------------------------
//
//  Tracks information about a particular property supported by the
//  current selection.
//
//-------------------------------------------------------------------------

struct DPD
{
    BSTR            bstrName;               //  Property name
    BSTR            bstrType;               //  Property type, or NULL if
                                            //    the name is implied by vt
    DISPID          dispid;                 //  Member ID
    VARENUM         vt;                     //  Property type
    VARIANT         var;                    //  Cached common property value
                                            //    from the selection
    CDataAry<EVAL> *pAryEVAL;               //  For enumerated props, the
                                            //    array of allowed values

    unsigned        fVisit:1;               //  Marks structs while merging
    unsigned        fMemberNotFound:1;      //  TRUE if the member was not
                                            //    found on all selected objects
    unsigned        fNoMatch:1;             //  TRUE if the member was found,
                                            //    but not all objects had the
                                            //    the same value
    unsigned        fSpecialCaseFont:1;     //  TRUE if this prop is a Font
    unsigned        fSpecialCaseColor:1;    //  TRUE if this prop is a Color
    unsigned        fSpecialCasePicture:1;  //  TRUE if this prop is a Picture
    unsigned        fSpecialCaseMouseIcon:1;//  TRUE if this prop is a MouseIcon
    unsigned        fSpecialCaseUnitMeasurement:1; // TRUE if unit measurement sub-obj
    unsigned        fOwnEVAL:1;             //  TRUE if this struct owns the
                                            //    storage of pAryEVAL
    unsigned        fDirty:1;               //  TRUE if the property has been
                                            //    edited
    unsigned        fIndirect:1;            //  TRUE if there was a single
                                            //    extra indirection in the
                                            //    type of the property (eg,
                                            //    IFont *)
    unsigned        fReadOnly:1;            // TRUE if this prop is read only.

    void        Free();
    HRESULT     AppendEnumValue(TCHAR * pstr, int value);
};


//+---------------------------------------------------------------------------
//
//  Class:      CCommitEngine
//
//+---------------------------------------------------------------------------

class CCommitEngine
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CCommitEngine))

    // ctor/dtor
    CCommitEngine();
    ~CCommitEngine();

    // Public methods
    HRESULT     SetObjects(long cObjects, IDispatch **ppDisp);
    HRESULT     ReleaseObjects();
    HRESULT     Commit();
    HRESULT     GetProperty(DISPID dispid, VARIANT *pvar);
    HRESULT     SetProperty(DISPID dispid, VARIANT *pvar);
    CDataAry<DPD> * GetDPDs()
        { return &_aryDPD; }
    
    // Helpers
    HRESULT     CreatePropertyDescriptor();
    HRESULT     ReleasePropertyDescriptor();
    HRESULT     ParseUserDefined(ITypeInfo * pTI, void * pVD, DPD * pDPD, BOOL fVar);
    HRESULT     ParseUnknown(ITypeInfo *pTI, DPD * pDPD);
    HRESULT     FindReadPropFuncDesc(
                    ITypeInfo *  pTI,
                    DISPID      dispid,
                    int         iStart,
                    int         iCount,
                    FUNCDESC **  ppFD);
    void        UpdateValues();
    
    
    // Data members
    CPtrAry<IDispatch *>_aryObjs;   //  Current set of objects
    CDataAry<DPD>       _aryDPD;    //  Property descriptors for current
                                    //    set of objects

    DPD                 _dpdBool;   //  Fixed list of enum values for
                                    //    Boolean properties
    DPD                 _dpdColor;  //  Fixed list of enum values for
                                    //    Color properties
};

#pragma INCMSG("--- End 'commit.hxx'")
#else
#pragma INCMSG("*** Dup 'commit.hxx'")
#endif
