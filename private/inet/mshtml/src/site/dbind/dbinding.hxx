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

#ifndef I_DBINDING_HXX_
#define I_DBINDING_HXX_
#pragma INCMSG("--- Beg 'dbinding.hxx'")

#ifndef X_DLCURSOR_HXX_
#define X_DLCURSOR_HXX_
#include "dlcursor.hxx"
#endif

#ifndef X_ELEMDB_HXX_
#define X_ELEMDB_HXX_
#include "elemdb.hxx"   // for dbind_htmlok
#endif

#ifdef WIN16
#define DOUBLE double
#endif

interface ISimpleDataConverter;

MtExtern(CXfer);
MtExtern(CXferThunk);
MtExtern(CHRowAccessor);

//+----------------------------------------------------------------------------
//
//  Class CTypeCoerce
//
//  Purpose:
//      Basic type conversion table and routines to convert VARIANT and Nile
//      style DBTYPE_nnn.
//
//  Created by TerryLu
//
class CTypeCoerce
{
public:
    CTypeCoerce ()
       {   }

    // Enumeration of canonical types.  Note that TYPE_END is a value used to
    // signal the number of canonical types so add all new CAN_TYPEs before
    // TYPE_END.
    enum CAN_ENUM {TYPE_NULL    = 0,  TYPE_I2      = 1,  TYPE_I4      = 2,
                   TYPE_R4      = 3,  TYPE_R8      = 4,  TYPE_CY      = 5,
                   TYPE_DATE    = 6,  TYPE_BSTR    = 7,  TYPE_BOOL    = 8,
                   TYPE_VARIANT = 9,  TYPE_CHAPTER = 10, TYPE_END };

    typedef BYTE CAN_TYPE;
    
    enum { MAX_CAN_TYPES = TYPE_END };                  // Number of CAN_TYPES.

    static HRESULT CanonicalizeType (DBTYPE dbType, CAN_TYPE & canType);
    static HRESULT ConvertData (CAN_TYPE typeIn, void *pValueIn,
                                CAN_TYPE typeOut, void *pValueOut);
    static BOOL FVariantType(DBTYPE);
    static HRESULT IsEqual(VARTYPE, void *, void *);
    static size_t MemSize(CAN_TYPE cant)
        {
            static const BYTE acbType[] =
            {
                0,                      // TYPE_NULL
                sizeof(SHORT),          // TYPE_I2
                sizeof(LONG),           // TYPE_I4
                sizeof(FLOAT),          // TYPE_R4
                sizeof(DOUBLE),         // TYPE_R8
                sizeof(CY),             // TYPE_CY
                sizeof(DATE),           // TYPE_DATE
                sizeof(BSTR),           // TYPE_BSTR
                sizeof(VARIANT_BOOL),   // TYPE_BOOL
                sizeof(VARIANT),        // TYPE_VARIANT
                sizeof(HCHAPTER),       // TYPE_CHAPTER
            };
    
            Assert(cant > TYPE_NULL);
            Assert(cant < TYPE_END);
            Assert(ARRAY_SIZE(acbType) == TYPE_END);
            return acbType[cant];
        }

private:
    static VARIANT_BOOL BoolFromStr(OLECHAR *pValueIn);
    static HRESULT CallStr (void *pFunc,
                            CAN_TYPE inType, void *pIn,
                            void *pOut);

    static HRESULT CallThru (void *pFunc, CAN_TYPE inType,
                             void *pIn, void *pOut);



    NO_COPY(CTypeCoerce);
};



class CElement;                 // Forward ref.
class CXferThunk;               // Forward ref.
class CInstance;                // Forward ref.
class CXfer;                    // Forward ref.
class CDataSourceProvider;
class CRecordInstance;


//+---------------------------------------------------------------------------
//  Class:      CXferThunk
//
//  Purpose:    Manages non-instance binding specifications.  This class can
//              be late bound (in other words the rowset may not be realized
//              until data must be transferred).  Therefore, the CXferThunk
//              will not be built until a TransferFromSrc or TransferToSrc is
//              called.  There are two variations to XferThunks (XT) one is
//              IRowset bound (OLE/DB) and the other is IDispatch bound.  For
//              the IRowset bound case the meta-data may not be realized until
//              the TransferXXXXX is actually needed.
//----------------------------------------------------------------------------
class CXferThunk : public IUnknown
{
    // CXferThunk::CAccessor -- helper abstract class; derived classes do work
    //  specific to individual source types.

    // we originally designed CAccessor as a base class, with CHRowAccessor
    //  being a derived concreate class, and other accessor types allowed for
    //  binding to single-valued sources (IDispatches).  Because we will ship
    //  with only one Accessor class, CHRowAccessor is no longer derived,
    //  and we use a typedef so that references to the former base class
    //  still work.
    class CHRowAccessor
    {
    public:
        DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHRowAccessor))

        /* virtual */ HRESULT ValueFromSrc(const CXferThunk *pXT,
                                           CInstance *pSrc,
                                           LPVOID lpvData,
                                           CElement *pElem) const;

        /* virtual */ HRESULT ValueToSrc(const CXferThunk *pXT,
                                         CInstance *pSrc,
                                         LPVOID lpvData,
                                         DWORD *pdwStatus,
                                         CElement *pElem) const;
                                         
        /* virtual */ BOOL IsSrcWritable(CInstance *pSrc) const;
        /* virtual */ void Detach();

        HRESULT Init(CXferThunk *pXT, CDataLayerCursor *pDLC);
        
        DEBUG_METHODS
        
    private:
        // members for optimized transfer between element and HROW
        HACCESSOR   _hAccessor;      // OLE DB accessor for data in HROW
        CDataLayerCursor *_pDLC;     // cursor for releasing _hAccessor
    };
    friend CHRowAccessor;
    typedef CHRowAccessor CAccessor; // former base class

public:
    // IUnknown members.  We derive from IUnknown so that other code can
    //  put us in a FormsPtrAry.
    DECLARE_FORMS_IUNKNOWN_METHODS;

    // this static routine takes care of new and init; Release
    //  manages destruction
    static HRESULT Create(CDataLayerCursor *pDLC, LPCTSTR strField, CVarType vt,
                            ISimpleDataConverter *pSDC, CXferThunk **ppXT);

    // Do the actual work to transfer data
    HRESULT ValueFromSrc(CInstance *pSrc, LPVOID lpv, CElement *pElem);
    HRESULT ValueToSrc(CInstance *pSrc, LPVOID lpv, DWORD *pdwStatus, CElement *pElem);
    BOOL IsWritable(CInstance *pSrc);

    // access to private member
    DBORDINAL IdSrc () const        { return _idSrc; }
    void SetIdSrc(DBORDINAL idSrc)  { _idSrc = idSrc; }

    // initialize variant based on the type desired by an element, and
    //  return a pointer either to beginning of the variant or to the
    //  data-portion, depending on what the element wants.
    VOID *PvInitVar(VARIANT *pvar) const;
    static VOID *PvInitVar(VARIANT *pvar, CVarType vt);
    
    DEBUG_METHODS

private:
    // All allocation, construction, and destruction go through
    //  either Create or Release
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CXferThunk))

    CXferThunk()    {}  // all work done in Create
    ~CXferThunk();
    HRESULT EnsureAccessor(CInstance *); // helper for late binding CAccessor

    ULONG               _ulRefCount;

    // members initalized by CXferThunk::CAccessor
    CAccessor          *_pAccessor;
    DBORDINAL           _idSrc;     // dispid or column number (-1 invalid)
    DBTYPE              _dbtSrc;    // provider's native datatype
    ISimpleDataConverter *_pSDC;    // DSO's data converter, if any
    
    CVarType            _vtDest;        // datatype desired by destination

    // coercion support
    CTypeCoerce::CAN_TYPE _cantSrc;
    CTypeCoerce::CAN_TYPE _cantDest;

    // flags packed together; some are here for use by CXferThunk::CAccessor,
    // some for use by CXferThunk itself,  Packed into a USHORT, next to the
    // two CAN_TYPEs, for memory efficiency.
    USHORT              _fAccessorError:1;  // error on accessor creation?
    USHORT              _fWriteSrc:1;       // is data source writable?
    USHORT              _fUseSDC:1;         // use SDC for data conversion?
};




// Data source instance.
class CInstance
{
#if DBG == 1
public:
    enum INST_TYPE { IT_UNKNOWN = 0, IT_ROWSET = 1, IT_DISPATCH = 2};
    virtual INST_TYPE Kind() const = 0;
#endif
};


class CXfer
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CXfer))
    
    CXfer()     {}  // zero-filed by operator new

    static HRESULT CreateBinding(CElement *pElement,  LONG id, LPCTSTR strDataFld,
                                    CDataSourceProvider *pProvider,
                                    CRecordInstance *pSrcInstance,
                                    CXfer **ppXfer = NULL,
                                    BOOL fDontTransfer = FALSE);

    HRESULT Init(CElement *pElem, CInstance *pSrcInstance, CXferThunk *pXT,
                LONG id, BOOL fHTML, DWORD dwTransfer);

    ~CXfer();

    void Detach();

    CElement * GetDestElement() const
        { return _pDestElem; }

    HRESULT SetDestElement(CElement *pDestElem);

    CInstance * GetSrcInstance() const
        { return _pSrcInstance; }

    void SetSrcInstance(CInstance *pSrcInst)
        { _pSrcInstance = pSrcInst; }

    BOOL FTransferringToDest() const
        { return _fTransferringToDest; }

    BOOL FTransferringToSrc() const
        { return _fTransferringToSrc; }

    CXferThunk * GetXT() const
        { return _pXT; }

    void SetXT(CXferThunk *pXT)
        { Assert(pXT); _pXT = pXT; }

    HRESULT TransferFromSrc();
    HRESULT ClearValue();
    HRESULT TransferToSrc(DWORD *pdwStatus);
    HRESULT ValueFromSrc(LPVOID lpv)
        { RRETURN(_pXT->ValueFromSrc(_pSrcInstance, lpv, _pDestElem)); }
        
    HRESULT CompareWithSrc();
    void    EnableTransfers(BOOL fTransferOK) { _fDontTransfer = !fTransferOK; }

    HRESULT ColumnsChanged(DBORDINAL cColumns, DBORDINAL aColumns[]);

    HRESULT ShowDiscardMessage (HRESULT hrError, DWORD dwStatus, int *pnResult);
    
    DEBUG_METHODS

    LONG IdElem () const            { return _idElem; }
    CElement *PElemOwner() const    { return _pDestElem; }
    CVarType VtElem () const        { return _vtElem; }

    // should we tell destination element to use HTML for transfer?
    BOOL FHTML() const
        { return _fHTMLSpecified && (_dwTransfer & DBIND_HTMLOK) != 0; }
    BOOL FOneWay() const
        { return (_dwTransfer & DBIND_ONEWAY) != 0; }
private:
    CElement           *_pDestElem;
    CInstance          *_pSrcInstance;
    CXferThunk         *_pXT;

    // specification of "destination" or "client"
    LONG                _idElem;       // bound ID of destination Element
                                        // (see CElement::BINDINFO)
    CVarType            _vtElem;        // datatype desired by _idElem
    DWORD               _dwTransfer;    // transfer restrctions
    USHORT              _fHTMLSpecified:1;  // HTML formatting specified?
    unsigned int       _fTransferringToSrc : 1;
    unsigned int       _fTransferringToDest : 1;
    unsigned int       _fError : 1;     // error occured on last xfer to elem
    unsigned int       _fDontTransfer : 1;      // data transfer not allowed

    HRESULT ConnectXferToElement();

    void DisonnectXferToElement();
    
    NO_COPY(CXfer);
};

#pragma INCMSG("--- End 'dbinding.hxx'")
#else
#pragma INCMSG("*** Dup 'dbinding.hxx'")
#endif
