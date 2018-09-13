//---------------------------------------------------------------------------
// MSC2R.h : Main header file for the MSC2R DLL
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------

#ifndef __MSC2R_H__
#define __MSC2R_H__

// {E55A7600-E666-11cf-84A5-0000C08C00C4}
const GUID CLSID_CRowsetFromCursor =	
	{ 0xe55a7600, 0xe666, 0x11cf, { 0x84, 0xa5, 0x0, 0x0, 0xc0, 0x8c, 0x0, 0xc4 } };

// {E55A7601-E666-11cf-84A5-0000C08C00C4}
const IID IID_IRowsetFromCursor = 
	{ 0xe55a7601, 0xe666, 0x11cf, { 0x84, 0xa5, 0x0, 0x0, 0xc0, 0x8c, 0x0, 0xc4 } };

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRowsetFromCursor : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetRowset( 
            /* [in] */ ICursor __RPC_FAR *pCursor,
            /* [out] */ IRowset __RPC_FAR **ppRowset,
            /* [in] */ LCID lcid) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRowsetFromCursor
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRowsetFromCursor __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRowsetFromCursor __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRowsetFromCursor __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRowset )( 
            IRowsetFromCursor __RPC_FAR * This,
            /* [in] */ ICursor __RPC_FAR *pCursor,
            /* [out] */ IRowset __RPC_FAR **ppRowset,
            /* [in] */ LCID lcid);
        
        END_INTERFACE
    } IRowsetFromCursorVtbl;

    interface IRowsetFromCursor
    {
        CONST_VTBL struct IRowsetFromCursorVtbl __RPC_FAR *lpVtbl;
    };

#endif 	/* C style interface */

#ifdef __cplusplus
extern "C" {
#endif
// MSC2R entry point
HRESULT WINAPI VDGetIRowsetFromICursor(ICursor * pCursor,
									   IRowset ** ppRowset,
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
// MessageId: VD_E_CANNOTCONNECTINOTIFYDBEVENTS
//
// MessageText:
//
//  Unable to connect INotifyDBEvents
//
#define VD_E_CANNOTCONNECTINOTIFYDBEVENTS       ((HRESULT)0x80050E01L)

//
// MessageId: VD_E_CANNOTSETBINDINGSONCOLUMNSCURSOR
//
// MessageText:
//
//  Unable to set bindings on columns cursor
//
#define VD_E_CANNOTSETBINDINGSONCOLUMNSCURSOR   ((HRESULT)0x80050E02L)

//
// MessageId: VD_E_CANNOTSETBINDINGSONCLONECURSOR
//
// MessageText:
//
//  Unable to set bindings on clone cursor
//
#define VD_E_CANNOTSETBINDINGSONCLONECURSOR     ((HRESULT)0x80050E03L)

#endif //__MSC2R_H__
/////////////////////////////////////////////////////////////////////////////
