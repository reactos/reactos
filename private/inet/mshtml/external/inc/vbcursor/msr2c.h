//---------------------------------------------------------------------------
// MSR2C.h : Main header file for Viaduct phase II
//
// Copyright (c) 1996, 1997 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------

  /////////////////////////////////////////////////////////////////////////
  // NOTE - you must define VD_INCLUDE_ROWPOSITION before including this //
  //        header for ICursorFromRowPosition to be defined in your code //
  /////////////////////////////////////////////////////////////////////////

#ifndef __MSR2C_H__
#define __MSR2C_H__

// {5B5E7E70-E653-11cf-84A5-0000C08C00C4}
const GUID CLSID_CCursorFromRowset =	
	{ 0x5b5e7e70, 0xe653, 0x11cf, { 0x84, 0xa5, 0x0, 0x0, 0xc0, 0x8c, 0x0, 0xc4 } };

// {5B5E7E72-E653-11cf-84A5-0000C08C00C4}
const IID IID_ICursorFromRowset = 
	{ 0x5b5e7e72, 0xe653, 0x11cf, { 0x84, 0xa5, 0x0, 0x0, 0xc0, 0x8c, 0x0, 0xc4 } };

#ifdef VD_INCLUDE_ROWPOSITION

// {5B5E7E73-E653-11cf-84A5-0000C08C00C4}
const IID IID_ICursorFromRowPosition = 
	{ 0x5b5e7e73, 0xe653, 0x11cf, { 0x84, 0xa5, 0x0, 0x0, 0xc0, 0x8c, 0x0, 0xc4 } };

#endif //VD_INCLUDE_ROWPOSITION

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ICursorFromRowset : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCursor( 
            /* [in]  */ IRowset __RPC_FAR *pRowset,
            /* [out] */ ICursor __RPC_FAR **ppCursor,
            /* [in]  */ LCID lcid) = 0;
    };
    
#ifdef VD_INCLUDE_ROWPOSITION

    interface ICursorFromRowPosition : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCursor( 
            /* [in]  */ IRowPosition __RPC_FAR *pRowPosition,
            /* [out] */ ICursor __RPC_FAR **ppCursor,
            /* [in]  */ LCID lcid) = 0;
    };

#endif //VD_INCLUDE_ROWPOSITION

#else 	/* C style interface */

    typedef struct ICursorFromRowset
    {
        BEGIN_INTERFACE
        
        HRESULT (STDMETHODCALLTYPE __RPC_FAR *QueryInterface)( 
            ICursorFromRowset __RPC_FAR *This,
            /* [in]  */ REFIID riid,
            /* [out] */ void __RPC_FAR **ppvObject);
        
        ULONG (STDMETHODCALLTYPE __RPC_FAR *AddRef)( 
            ICursorFromRowset __RPC_FAR *This);
        
        ULONG (STDMETHODCALLTYPE __RPC_FAR *Release)( 
            ICursorFromRowset __RPC_FAR *This);
        
        HRESULT (STDMETHODCALLTYPE __RPC_FAR *GetCursor)( 
            ICursorFromRowset __RPC_FAR *This,
            /* [in]  */ IRowset __RPC_FAR *pRowset,
            /* [out] */ ICursor __RPC_FAR **ppCursor,
            /* [in]  */ LCID lcid);
        
        END_INTERFACE
    } ICursorFromRowsetVtbl;

    interface ICursorFromRowset
    {
        CONST_VTBL struct ICursorFromRowsetVtbl __RPC_FAR *lpVtbl;
    };

#ifdef VD_INCLUDE_ROWPOSITION

    typedef struct ICursorFromRowPosition
    {
        BEGIN_INTERFACE
        
        HRESULT (STDMETHODCALLTYPE __RPC_FAR *QueryInterface)( 
            ICursorFromRowPosition __RPC_FAR *This,
            /* [in]  */ REFIID riid,
            /* [out] */ void __RPC_FAR **ppvObject);
        
        ULONG (STDMETHODCALLTYPE __RPC_FAR *AddRef)( 
            ICursorFromRowPosition __RPC_FAR *This);
        
        ULONG (STDMETHODCALLTYPE __RPC_FAR *Release)( 
            ICursorFromRowPosition __RPC_FAR *This);
        
        HRESULT (STDMETHODCALLTYPE __RPC_FAR *GetCursor)( 
            ICursorFromRowPosition __RPC_FAR *This,
            /* [in]  */ IRowPosition __RPC_FAR *pRowPosition,
            /* [out] */ ICursor __RPC_FAR **ppCursor,
            /* [in]  */ LCID lcid);
        
        END_INTERFACE
    } ICursorFromRowPositionVtbl;

    interface ICursorFromRowPosition
    {
        CONST_VTBL struct ICursorFromRowPositionVtbl __RPC_FAR *lpVtbl;
    };

#endif //VD_INCLUDE_ROWPOSITION

#endif 	/* C style interface */

#ifdef __cplusplus
extern "C" {
#endif
// old entry point
HRESULT WINAPI VDGetICursorFromIRowset(IRowset * pRowset, 
                                       ICursor ** ppCursor,
                                       LCID lcid);
#ifdef __cplusplus
}
#endif

//
// MessageId: VD_E_CANNOTGETMANDATORYINTERFACE
//
// MessageText:
//
//  Unable to get required interface
//
#define VD_E_CANNOTGETMANDATORYINTERFACE        ((HRESULT)0x80050E00L)

//
// MessageId: VD_E_CANNOTCONNECTIROWSETNOTIFY
//
// MessageText:
//
//  Unable to connect IRowsetNotify
//
#define VD_E_CANNOTCONNECTIROWSETNOTIFY         ((HRESULT)0x80050E31L)

//
// MessageId: VD_E_CANNOTGETCOLUMNINFO
//
// MessageText:
//
//  Unable to get column information
//
#define VD_E_CANNOTGETCOLUMNINFO                ((HRESULT)0x80050E32L)

//
// MessageId: VD_E_CANNOTCREATEBOOKMARKACCESSOR
//
// MessageText:
//
//  Unable to create bookmark accessor
//
#define VD_E_CANNOTCREATEBOOKMARKACCESSOR       ((HRESULT)0x80050E33L)

//
// MessageId: VD_E_REQUIREDPROPERTYNOTSUPPORTED
//
// MessageText:
//
//  Require rowset property is not supported
//
#define VD_E_REQUIREDPROPERTYNOTSUPPORTED       ((HRESULT)0x80050E34L)

//
// MessageId: VD_E_CANNOTGETROWSETINTERFACE
//
// MessageText:
//
//  Unable to get rowset interface
//
#define VD_E_CANNOTGETROWSETINTERFACE			((HRESULT)0x80050E35L)

//
// MessageId: VD_E_CANNOTCONNECTIROWPOSITIONCHANGE
//
// MessageText:
//
//  Unable to connect IRowPositionChange
//
#define VD_E_CANNOTCONNECTIROWPOSITIONCHANGE	((HRESULT)0x80050E36L)

#endif //__MSR2C_H__

/////////////////////////////////////////////////////////////////////////////
