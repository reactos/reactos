//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       clstab.hxx
//
//  Contents:   Class table for CDoc.
//
//
//----------------------------------------------------------------------------

#ifndef I_CLSTAB_HXX_
#define I_CLSTAB_HXX_
#pragma INCMSG("--- Beg 'clstab.hxx'")

MtExtern(CClassTable)
MtExtern(CClassTable_aryci_pv)

class CTypeInfoNav;                 // Forward reference.
class CDoc;

ExternTag(tagShowHideVerb);

//+------------------------------------------------------------------------
//
//  Enum:       COMPAT
//
//  Synopsis:   OLE Control compatiblity flags.
//
//-------------------------------------------------------------------------

enum 
{
    COMPAT_AGGREGATE =                  0x00000001, // Aggregate this ocx
    COMPAT_NO_OBJECTSAFETY =            0x00000002, // Ignore safety info presented by ocx
    COMPAT_NO_PROPNOTIFYSINK =          0x00000004, // Don't attach prop notify sink
    COMPAT_SEND_SHOW =                  0x00000008, // DoVerb(SHOW) before
                                                //   DoVerb(INPLACE)
    COMPAT_SEND_HIDE =                  0x00000010, // DoVerb(HIDE) before 
                                                //   InPlaceDeactivate()
    COMPAT_ALWAYS_INPLACEACTIVATE =     0x00000020, // Baseline state is INPLACE
                                                    //   in browse mode even if
                                                    //   not visible.
    COMPAT_NO_SETEXTENT =               0x00000040, // Don't bother calling SetExtent
    COMPAT_NO_UIACTIVATE =              0x00000080, // Never let this ctrl uiactivate
    COMPAT_NO_QUICKACTIVATE =           0x00000100, // Don't bother with IQuickActivate
    COMPAT_NO_BINDF_OFFLINEOPERATION =  0x00000200, // filter out BINDF_OFFLINEOPERATION flag.
    COMPAT_EVIL_DONT_LOAD =             0x00000400, // don't load this control at all.
    COMPAT_PROGSINK_UNTIL_ACTIVATED =   0x00000800, // delay OnLoad() event firing until this
                                                    //   ctrl is inplace activated.  
                                                    //   ALWAYS USE WITH COMPAT_ALWAYS_INPLACEACTIVATE
    COMPAT_USE_PROPBAG_AND_STREAM =     0x00001000, // Call both IPersistPropBag::Load
                                                    //   and IPersistStreamInit::Load
    COMPAT_DISABLEWINDOWLESS      =     0x00002000, // do not allow control to get inplace activated windowless
    COMPAT_SETWINDOWRGN           =     0x00004000, // SetWindowRgn to clip rect before call to SetObjectRects
                                                    // to avoid flickering
    COMPAT_PRINTPLUGINSITE        =     0x00008000, // When printing, ask the plugin site to print directly instead
    COMPAT_INPLACEACTIVATEEVENWHENINVISIBLE =   0x00010000, // Inplace Activate even when not visible
    COMPAT_NEVERFOCUSSABLE        =     0x00020000, // Hack for #68793
    COMPAT_ALWAYSDEFERSETWINDOWRGN  = 0x00040000,   // Hack for #71466
    COMPAT_INPLACEACTIVATESYNCHRONOUSLY = 0x00080000, // Hack for #71073
    COMPAT_SECURITYCHECKONREDIRECT = 0x00100000, // #76867, Behaviors should perform an AccessAllowed check on server-side redirects
};

HRESULT CompatFlagsFromClsid(REFCLSID clsid, DWORD *pdwCompat, DWORD *pdwOleMisc);

//+------------------------------------------------------------------------
//
//  Struct:     CLASSINFO
//
//  Synopsis:   Information maintained by the form about a control class.
//
//-------------------------------------------------------------------------
struct INSTANTCLASSINFO   // no TypeInfo required
{
protected:
    // Because we no longer binary persist all flags, we now use bitfields,
    //  rather than a dwFlags DWORD.  Note that this flags can packed smaller
    //  than 32-bits if it makes sense given the rest of the structure.

    WORD        fiidDEInitialized:1;    // iidDispEvent has been initialized,
                                        //  stuff requiring us to inspect
                                        //  TypeInfo may not have been.
    WORD        fAllInitialized:1;      // We've initialized the whole struct,
                                        //  based on TypeInfo, if any
    WORD        fDualInterface:1;       // interface, is dual, not just
                                        //  IDispatch based; clients can use
                                        //  vtable offsets.
    WORD        fImmediateBind:1;       // dispIDBind has the IMMEDIATEBIND
                                        //  attribute set; save values on
                                        //  any change to data source.
    WORD        _fIDEx2Queried:1;       // has IDispatchEx been queryied for?
    WORD        _fIsDispatchEx2:1;      // if _newEnum is present we assume
                                        //  that this is exposed as an ole-
                                        //  collection. which changes how invoke
                                        //  handles paramters in CObjectElement
    WORD        fOC96:1;                // TRUE if this control is what we
                                        //  would consider to be oc96 or greater.
public:
    DWORD       dwCompatFlags;          // compatibility flags
    DWORD       dwMiscStatusFlags;      // OLEMISC_* compatibility flags.
    CLSID       clsid;                  // class identifier

    // expose the bitfield member ininitalized from the TypeInfo
    
    inline BOOL IsDispatchEx2()     { return _fIsDispatchEx2; }
    inline void SetIsDispatchEx2( BOOL fCol ) {
                    _fIDEx2Queried=TRUE;  _fIsDispatchEx2=fCol; }
    inline BOOL HasIDex2BeenCalled() { return _fIDEx2Queried;    }
    inline BOOL FImmediateBind()   { return fImmediateBind; }
    inline BOOL FDualInterface()   { return fDualInterface; }
    inline BOOL IsOC96()
        {   return fOC96; }
    inline void SetOC96(BOOL f)
        {   fOC96 = f; }
};

struct QUICKCLASSINFO: public INSTANTCLASSINFO   // TypeInfo might not be required
{    
    IID             iidDispEvent;       // dispinterface event set (Private)
};

struct CLASSINFO : public QUICKCLASSINFO    // requires TypeInfo
{
    friend class CClassTable;

public:
    // WARNING:  since this struct is used in a 
    // CDataAry<CLASSINFO> _aryci; template declared array, the
    // constructor and destructor will never get called.  Don't use
    // them.
    //CLASSINFO ();
    //~CLASSINFO ();


    void Init(REFCLSID clsid, BOOL fInitCompatFlags);

    // Change the entry from Dual to non; presumably, because class insn't
    //  thread-safe, and attempt to QI for iidDefault failed.
    void ClearFDualInterface();

    // information about the class as a whole

    IID             iidDefault;         // default programmability interface.
    int             cMethodsDefault;    // number of methods on default dual interface

    // information about DISPID_VALUE
    unsigned int    uGetValueIndex;     // vtable index of DISPID_VALUE get prop
    unsigned int    uPutValueIndex;     // vtable index of DISPID_VALUE put prop
    VARTYPE         vtValueType;        // automation type of DISPID_VALUE prop
    DWORD           dwFlagsValue;       // invoke flags for value prop
    
    // information about the DEFAULTBIND property, if any
    VARTYPE         vtBindType;         // automation type of def bindable prop
    DISPID          dispIDBind;         // dispid of def bindable prop
    unsigned int    uGetBindIndex;      // vtable index of def bindable get prop
    unsigned int    uPutBindIndex;      // vtable index of def bindable put prop
    DWORD           dwFlagsBind;        // invoke flags for def bindable prop

    // information about about data providing and consuming properties
    DISPID          dispidIDataSource;  // dispid of IDataSource property
    unsigned int    uSetIDataSource;    // vtable index of the IDataSource set prop
    unsigned int    uGetIDataSource;    // vtable index to get IDataSource property
    DWORD           dwFlagsDataSource;  // invoke flags for value prop

    DISPID          dispidRowset;       // dispid of IRowset property
    unsigned int    uSetRowset;         // vtable index of the IRowset set prop
    unsigned int    uGetRowset;         // vtable index to get IRowset property
    DWORD           dwFlagsRowset;      // invoke flags for value prop

    DISPID          dispidCursor;       // dispid for ICursor consumption
    unsigned int    uSetCursor;         // vtable index to set ICursor

    DISPID          dispidSTD;          // dispid for STD source
    unsigned int    uGetSTD;            // vtable index to get ISimpleTabularData

    ITypeInfo *     _pTypeInfoEvents;
};

extern CLASSINFO g_ciNull;

//+------------------------------------------------------------------------
//
//  Class:      CClassTable
//
//  Synopsis:   Maintains an array of CLASSINFOs.
//
//-------------------------------------------------------------------------

class CClassTable
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CClassTable))
    CClassTable() : _aryci(Mt(CClassTable_aryci_pv)) {}
    ~CClassTable() { Reset(); }

    void            Reset();

    HRESULT         AssignWclsid(
                            CDoc *pDoc,
                            REFCLSID clsid,
                            WORD *pwclsid);
                            

    // no typeinfo load required for this. if the wclsid is invalid
    // then return the class info for GUID_NULL
    INSTANTCLASSINFO *GetInstantClassInfo(WORD wclsid)
        {
            Assert(0 <= wclsid && wclsid <= _aryci.Size());
            INSTANTCLASSINFO * pci = (wclsid == 0) ? &g_ciNull : &_aryci[wclsid - 1];

            // Fix for IE5 Bug 77593. pci is NULL in some cases. This causes
            // a crash during some stress tests and some AOL usage scenarios.
            // We check pci to make sure it's not NULL before using it. 
            // Also, all clients of this method have been changed to check 
            // the return value of this method. This will be revisited for 
            // IE6 to find out the real reason why pci is NULL. 
            //
            Assert(pci);
            
#if DBG == 1
            if (pci && wclsid && IsTagEnabled(tagShowHideVerb))
                pci->dwCompatFlags |= (COMPAT_SEND_SHOW | COMPAT_SEND_HIDE);
#endif

            return pci;
        }
        
    // Get the object's CLSID without trigger load of TypeLib
    CLSID *         GetpCLSID(int wclsid)
        {
            INSTANTCLASSINFO * pici = GetInstantClassInfo((WORD)wclsid);
            if (pici)
                return &pici->clsid;
                
            return NULL;
        }
        
    // Get the class information that *might* not requirea load of TypeLib
    QUICKCLASSINFO *GetQuickClassInfo(int wclsid, IUnknown *pUnk);

    // Get the default event-set IID, avoid TypeLib load if possible
    IID * GetpIIDDispEvent(int wclsid, IUnknown *pUnk)
    {
        QUICKCLASSINFO* pQCI = GetQuickClassInfo(wclsid, pUnk);
        if (pQCI)
        {
            return &pQCI->iidDispEvent;
        }
        else
        {
            return NULL;
        }
    }

    // Get full class information; loads TypeLib
    CLASSINFO *     GetClassInfo(int wclsid, IUnknown *pUnk);
    
    ULONG           Size() { return(_aryci.Size() + 1); }

private:
    static BOOL     IsInterfaceProperty (ITypeInfo *pTI, TYPEDESC *pTypeDesc,
                                                         IID *piid);

    static void     GetDefaultBindInfoForGet(CLASSINFO *pci, FUNCDESC *pfDesc,
                                        CTypeInfoNav& cTINav);
    static void     GetDefaultBindInfoForPut(CLASSINFO *pci, FUNCDESC *pfDesc);
    static void     GetDualInfo(CTypeInfoNav & cTINav, FUNCDESC *pfDesc,
                                CLASSINFO *pci);

    static void     FindTypelibInfo(ITypeInfo *pTI, CLASSINFO *pci);
    static HRESULT  InitializeIIDsFromTIDefault(CLASSINFO *pci,
                                                ITypeInfo *pTIDefault,
                                                TYPEATTR *ptaDefault);
    static HRESULT  InitializeIIDs(CLASSINFO *pci, IUnknown *pUnk);

    CDataAry<CLASSINFO> _aryci;

};

#pragma INCMSG("--- End 'clstab.hxx'")
#else
#pragma INCMSG("*** Dup 'clstab.hxx'")
#endif
