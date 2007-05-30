

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0493 */
/* at Sun Aug 13 16:46:05 2006
 */
/* Compiler settings for .\mstsclib_redist.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__


#ifndef __mstsclib_redist_h_h__
#define __mstsclib_redist_h_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IMsTscAxEvents_FWD_DEFINED__
#define __IMsTscAxEvents_FWD_DEFINED__
typedef interface IMsTscAxEvents IMsTscAxEvents;
#endif 	/* __IMsTscAxEvents_FWD_DEFINED__ */


#ifndef __IMsTscAx_FWD_DEFINED__
#define __IMsTscAx_FWD_DEFINED__
typedef interface IMsTscAx IMsTscAx;
#endif 	/* __IMsTscAx_FWD_DEFINED__ */


#ifndef __IMsRdpClient_FWD_DEFINED__
#define __IMsRdpClient_FWD_DEFINED__
typedef interface IMsRdpClient IMsRdpClient;
#endif 	/* __IMsRdpClient_FWD_DEFINED__ */


#ifndef __IMsRdpClient2_FWD_DEFINED__
#define __IMsRdpClient2_FWD_DEFINED__
typedef interface IMsRdpClient2 IMsRdpClient2;
#endif 	/* __IMsRdpClient2_FWD_DEFINED__ */


#ifndef __IMsRdpClient3_FWD_DEFINED__
#define __IMsRdpClient3_FWD_DEFINED__
typedef interface IMsRdpClient3 IMsRdpClient3;
#endif 	/* __IMsRdpClient3_FWD_DEFINED__ */


#ifndef __IMsRdpClient4_FWD_DEFINED__
#define __IMsRdpClient4_FWD_DEFINED__
typedef interface IMsRdpClient4 IMsRdpClient4;
#endif 	/* __IMsRdpClient4_FWD_DEFINED__ */


#ifndef __IMsTscNonScriptable_FWD_DEFINED__
#define __IMsTscNonScriptable_FWD_DEFINED__
typedef interface IMsTscNonScriptable IMsTscNonScriptable;
#endif 	/* __IMsTscNonScriptable_FWD_DEFINED__ */


#ifndef __IMsRdpClientNonScriptable_FWD_DEFINED__
#define __IMsRdpClientNonScriptable_FWD_DEFINED__
typedef interface IMsRdpClientNonScriptable IMsRdpClientNonScriptable;
#endif 	/* __IMsRdpClientNonScriptable_FWD_DEFINED__ */


#ifndef __IMsRdpClientNonScriptable2_FWD_DEFINED__
#define __IMsRdpClientNonScriptable2_FWD_DEFINED__
typedef interface IMsRdpClientNonScriptable2 IMsRdpClientNonScriptable2;
#endif 	/* __IMsRdpClientNonScriptable2_FWD_DEFINED__ */


#ifndef __IMsTscAdvancedSettings_FWD_DEFINED__
#define __IMsTscAdvancedSettings_FWD_DEFINED__
typedef interface IMsTscAdvancedSettings IMsTscAdvancedSettings;
#endif 	/* __IMsTscAdvancedSettings_FWD_DEFINED__ */


#ifndef __IMsRdpClientAdvancedSettings_FWD_DEFINED__
#define __IMsRdpClientAdvancedSettings_FWD_DEFINED__
typedef interface IMsRdpClientAdvancedSettings IMsRdpClientAdvancedSettings;
#endif 	/* __IMsRdpClientAdvancedSettings_FWD_DEFINED__ */


#ifndef __IMsRdpClientAdvancedSettings2_FWD_DEFINED__
#define __IMsRdpClientAdvancedSettings2_FWD_DEFINED__
typedef interface IMsRdpClientAdvancedSettings2 IMsRdpClientAdvancedSettings2;
#endif 	/* __IMsRdpClientAdvancedSettings2_FWD_DEFINED__ */


#ifndef __IMsRdpClientAdvancedSettings3_FWD_DEFINED__
#define __IMsRdpClientAdvancedSettings3_FWD_DEFINED__
typedef interface IMsRdpClientAdvancedSettings3 IMsRdpClientAdvancedSettings3;
#endif 	/* __IMsRdpClientAdvancedSettings3_FWD_DEFINED__ */


#ifndef __IMsRdpClientAdvancedSettings4_FWD_DEFINED__
#define __IMsRdpClientAdvancedSettings4_FWD_DEFINED__
typedef interface IMsRdpClientAdvancedSettings4 IMsRdpClientAdvancedSettings4;
#endif 	/* __IMsRdpClientAdvancedSettings4_FWD_DEFINED__ */


#ifndef __IMsTscSecuredSettings_FWD_DEFINED__
#define __IMsTscSecuredSettings_FWD_DEFINED__
typedef interface IMsTscSecuredSettings IMsTscSecuredSettings;
#endif 	/* __IMsTscSecuredSettings_FWD_DEFINED__ */


#ifndef __IMsRdpClientSecuredSettings_FWD_DEFINED__
#define __IMsRdpClientSecuredSettings_FWD_DEFINED__
typedef interface IMsRdpClientSecuredSettings IMsRdpClientSecuredSettings;
#endif 	/* __IMsRdpClientSecuredSettings_FWD_DEFINED__ */


#ifndef __IMsTscDebug_FWD_DEFINED__
#define __IMsTscDebug_FWD_DEFINED__
typedef interface IMsTscDebug IMsTscDebug;
#endif 	/* __IMsTscDebug_FWD_DEFINED__ */


#ifndef __MsTscAx_FWD_DEFINED__
#define __MsTscAx_FWD_DEFINED__

#ifdef __cplusplus
typedef class MsTscAx MsTscAx;
#else
typedef struct MsTscAx MsTscAx;
#endif /* __cplusplus */

#endif 	/* __MsTscAx_FWD_DEFINED__ */


#ifndef __MsRdpClient_FWD_DEFINED__
#define __MsRdpClient_FWD_DEFINED__

#ifdef __cplusplus
typedef class MsRdpClient MsRdpClient;
#else
typedef struct MsRdpClient MsRdpClient;
#endif /* __cplusplus */

#endif 	/* __MsRdpClient_FWD_DEFINED__ */


#ifndef __MsRdpClient2_FWD_DEFINED__
#define __MsRdpClient2_FWD_DEFINED__

#ifdef __cplusplus
typedef class MsRdpClient2 MsRdpClient2;
#else
typedef struct MsRdpClient2 MsRdpClient2;
#endif /* __cplusplus */

#endif 	/* __MsRdpClient2_FWD_DEFINED__ */


#ifndef __MsRdpClient3_FWD_DEFINED__
#define __MsRdpClient3_FWD_DEFINED__

#ifdef __cplusplus
typedef class MsRdpClient3 MsRdpClient3;
#else
typedef struct MsRdpClient3 MsRdpClient3;
#endif /* __cplusplus */

#endif 	/* __MsRdpClient3_FWD_DEFINED__ */


#ifndef __MsRdpClient4_FWD_DEFINED__
#define __MsRdpClient4_FWD_DEFINED__

#ifdef __cplusplus
typedef class MsRdpClient4 MsRdpClient4;
#else
typedef struct MsRdpClient4 MsRdpClient4;
#endif /* __cplusplus */

#endif 	/* __MsRdpClient4_FWD_DEFINED__ */


#ifdef __cplusplus
extern "C"{
#endif 



#ifndef __MSTSCLib_LIBRARY_DEFINED__
#define __MSTSCLib_LIBRARY_DEFINED__

/* library MSTSCLib */
/* [version][uuid] */ 


















typedef /* [public][public][public] */ 
enum __MIDL___MIDL_itf_mstsax_0275_0001
    {	autoReconnectContinueAutomatic	= 0,
	autoReconnectContinueStop	= 1,
	autoReconnectContinueManual	= 2
    } 	AutoReconnectContinueState;

typedef /* [public][public][public] */ 
enum __MIDL_IMsRdpClient_0001
    {	exDiscReasonNoInfo	= 0,
	exDiscReasonAPIInitiatedDisconnect	= 1,
	exDiscReasonAPIInitiatedLogoff	= 2,
	exDiscReasonServerIdleTimeout	= 3,
	exDiscReasonServerLogonTimeout	= 4,
	exDiscReasonReplacedByOtherConnection	= 5,
	exDiscReasonOutOfMemory	= 6,
	exDiscReasonServerDeniedConnection	= 7,
	exDiscReasonServerDeniedConnectionFips	= 8,
	exDiscReasonLicenseInternal	= 256,
	exDiscReasonLicenseNoLicenseServer	= 257,
	exDiscReasonLicenseNoLicense	= 258,
	exDiscReasonLicenseErrClientMsg	= 259,
	exDiscReasonLicenseHwidDoesntMatchLicense	= 260,
	exDiscReasonLicenseErrClientLicense	= 261,
	exDiscReasonLicenseCantFinishProtocol	= 262,
	exDiscReasonLicenseClientEndedProtocol	= 263,
	exDiscReasonLicenseErrClientEncryption	= 264,
	exDiscReasonLicenseCantUpgradeLicense	= 265,
	exDiscReasonLicenseNoRemoteConnections	= 266,
	exDiscReasonProtocolRangeStart	= 4096,
	exDiscReasonProtocolRangeEnd	= 32767
    } 	ExtendedDisconnectReasonCode;

typedef /* [public][public][public] */ 
enum __MIDL_IMsRdpClient_0002
    {	controlCloseCanProceed	= 0,
	controlCloseWaitForEvents	= 1
    } 	ControlCloseStatus;

typedef /* [custom][public] */ unsigned __int3264 UINT_PTR;

typedef /* [custom][public] */ __int3264 LONG_PTR;


EXTERN_C const IID LIBID_MSTSCLib;

#ifndef __IMsTscAxEvents_DISPINTERFACE_DEFINED__
#define __IMsTscAxEvents_DISPINTERFACE_DEFINED__

/* dispinterface IMsTscAxEvents */
/* [uuid] */ 


EXTERN_C const IID DIID_IMsTscAxEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("336D5562-EFA8-482E-8CB3-C5C0FC7A7DB6")
    IMsTscAxEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct IMsTscAxEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMsTscAxEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMsTscAxEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMsTscAxEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IMsTscAxEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IMsTscAxEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IMsTscAxEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IMsTscAxEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } IMsTscAxEventsVtbl;

    interface IMsTscAxEvents
    {
        CONST_VTBL struct IMsTscAxEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMsTscAxEvents_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMsTscAxEvents_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMsTscAxEvents_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMsTscAxEvents_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IMsTscAxEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IMsTscAxEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IMsTscAxEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __IMsTscAxEvents_DISPINTERFACE_DEFINED__ */


#ifndef __IMsTscAx_INTERFACE_DEFINED__
#define __IMsTscAx_INTERFACE_DEFINED__

/* interface IMsTscAx */
/* [object][oleautomation][dual][uuid] */ 


EXTERN_C const IID IID_IMsTscAx;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("327BB5CD-834E-4400-AEF2-B30E15E5D682")
    IMsTscAx : public IDispatch
    {
    public:
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Server( 
            /* [in] */ BSTR pServer) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Server( 
            /* [retval][out] */ BSTR *pServer) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Domain( 
            /* [in] */ BSTR pDomain) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Domain( 
            /* [retval][out] */ BSTR *pDomain) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_UserName( 
            /* [in] */ BSTR pUserName) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_UserName( 
            /* [retval][out] */ BSTR *pUserName) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_DisconnectedText( 
            /* [in] */ BSTR pDisconnectedText) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_DisconnectedText( 
            /* [retval][out] */ BSTR *pDisconnectedText) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_ConnectingText( 
            /* [in] */ BSTR pConnectingText) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_ConnectingText( 
            /* [retval][out] */ BSTR *pConnectingText) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Connected( 
            /* [retval][out] */ short *pIsConnected) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_DesktopWidth( 
            /* [in] */ long pVal) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_DesktopWidth( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_DesktopHeight( 
            /* [in] */ long pVal) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_DesktopHeight( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_StartConnected( 
            /* [in] */ long pfStartConnected) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_StartConnected( 
            /* [retval][out] */ long *pfStartConnected) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_HorizontalScrollBarVisible( 
            /* [retval][out] */ long *pfHScrollVisible) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_VerticalScrollBarVisible( 
            /* [retval][out] */ long *pfVScrollVisible) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_FullScreenTitle( 
            /* [in] */ BSTR rhs) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_CipherStrength( 
            /* [retval][out] */ long *pCipherStrength) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Version( 
            /* [retval][out] */ BSTR *pVersion) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_SecuredSettingsEnabled( 
            /* [retval][out] */ long *pSecuredSettingsEnabled) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_SecuredSettings( 
            /* [retval][out] */ IMsTscSecuredSettings **ppSecuredSettings) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_AdvancedSettings( 
            /* [retval][out] */ IMsTscAdvancedSettings **ppAdvSettings) = 0;
        
        virtual /* [hidden][propget][id] */ HRESULT STDMETHODCALLTYPE get_Debugger( 
            /* [retval][out] */ IMsTscDebug **ppDebugger) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Connect( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Disconnect( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateVirtualChannels( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SendOnVirtualChannel( 
            /* [in] */ BSTR chanName,
            /* [in] */ BSTR ChanData) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMsTscAxVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMsTscAx * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMsTscAx * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMsTscAx * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IMsTscAx * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IMsTscAx * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IMsTscAx * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IMsTscAx * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Server )( 
            IMsTscAx * This,
            /* [in] */ BSTR pServer);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Server )( 
            IMsTscAx * This,
            /* [retval][out] */ BSTR *pServer);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Domain )( 
            IMsTscAx * This,
            /* [in] */ BSTR pDomain);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Domain )( 
            IMsTscAx * This,
            /* [retval][out] */ BSTR *pDomain);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_UserName )( 
            IMsTscAx * This,
            /* [in] */ BSTR pUserName);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_UserName )( 
            IMsTscAx * This,
            /* [retval][out] */ BSTR *pUserName);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DisconnectedText )( 
            IMsTscAx * This,
            /* [in] */ BSTR pDisconnectedText);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DisconnectedText )( 
            IMsTscAx * This,
            /* [retval][out] */ BSTR *pDisconnectedText);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ConnectingText )( 
            IMsTscAx * This,
            /* [in] */ BSTR pConnectingText);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ConnectingText )( 
            IMsTscAx * This,
            /* [retval][out] */ BSTR *pConnectingText);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Connected )( 
            IMsTscAx * This,
            /* [retval][out] */ short *pIsConnected);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DesktopWidth )( 
            IMsTscAx * This,
            /* [in] */ long pVal);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DesktopWidth )( 
            IMsTscAx * This,
            /* [retval][out] */ long *pVal);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DesktopHeight )( 
            IMsTscAx * This,
            /* [in] */ long pVal);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DesktopHeight )( 
            IMsTscAx * This,
            /* [retval][out] */ long *pVal);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_StartConnected )( 
            IMsTscAx * This,
            /* [in] */ long pfStartConnected);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_StartConnected )( 
            IMsTscAx * This,
            /* [retval][out] */ long *pfStartConnected);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HorizontalScrollBarVisible )( 
            IMsTscAx * This,
            /* [retval][out] */ long *pfHScrollVisible);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_VerticalScrollBarVisible )( 
            IMsTscAx * This,
            /* [retval][out] */ long *pfVScrollVisible);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_FullScreenTitle )( 
            IMsTscAx * This,
            /* [in] */ BSTR rhs);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CipherStrength )( 
            IMsTscAx * This,
            /* [retval][out] */ long *pCipherStrength);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Version )( 
            IMsTscAx * This,
            /* [retval][out] */ BSTR *pVersion);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SecuredSettingsEnabled )( 
            IMsTscAx * This,
            /* [retval][out] */ long *pSecuredSettingsEnabled);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SecuredSettings )( 
            IMsTscAx * This,
            /* [retval][out] */ IMsTscSecuredSettings **ppSecuredSettings);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AdvancedSettings )( 
            IMsTscAx * This,
            /* [retval][out] */ IMsTscAdvancedSettings **ppAdvSettings);
        
        /* [hidden][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Debugger )( 
            IMsTscAx * This,
            /* [retval][out] */ IMsTscDebug **ppDebugger);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Connect )( 
            IMsTscAx * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Disconnect )( 
            IMsTscAx * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateVirtualChannels )( 
            IMsTscAx * This,
            /* [in] */ BSTR newVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SendOnVirtualChannel )( 
            IMsTscAx * This,
            /* [in] */ BSTR chanName,
            /* [in] */ BSTR ChanData);
        
        END_INTERFACE
    } IMsTscAxVtbl;

    interface IMsTscAx
    {
        CONST_VTBL struct IMsTscAxVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMsTscAx_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMsTscAx_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMsTscAx_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMsTscAx_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IMsTscAx_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IMsTscAx_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IMsTscAx_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IMsTscAx_put_Server(This,pServer)	\
    ( (This)->lpVtbl -> put_Server(This,pServer) ) 

#define IMsTscAx_get_Server(This,pServer)	\
    ( (This)->lpVtbl -> get_Server(This,pServer) ) 

#define IMsTscAx_put_Domain(This,pDomain)	\
    ( (This)->lpVtbl -> put_Domain(This,pDomain) ) 

#define IMsTscAx_get_Domain(This,pDomain)	\
    ( (This)->lpVtbl -> get_Domain(This,pDomain) ) 

#define IMsTscAx_put_UserName(This,pUserName)	\
    ( (This)->lpVtbl -> put_UserName(This,pUserName) ) 

#define IMsTscAx_get_UserName(This,pUserName)	\
    ( (This)->lpVtbl -> get_UserName(This,pUserName) ) 

#define IMsTscAx_put_DisconnectedText(This,pDisconnectedText)	\
    ( (This)->lpVtbl -> put_DisconnectedText(This,pDisconnectedText) ) 

#define IMsTscAx_get_DisconnectedText(This,pDisconnectedText)	\
    ( (This)->lpVtbl -> get_DisconnectedText(This,pDisconnectedText) ) 

#define IMsTscAx_put_ConnectingText(This,pConnectingText)	\
    ( (This)->lpVtbl -> put_ConnectingText(This,pConnectingText) ) 

#define IMsTscAx_get_ConnectingText(This,pConnectingText)	\
    ( (This)->lpVtbl -> get_ConnectingText(This,pConnectingText) ) 

#define IMsTscAx_get_Connected(This,pIsConnected)	\
    ( (This)->lpVtbl -> get_Connected(This,pIsConnected) ) 

#define IMsTscAx_put_DesktopWidth(This,pVal)	\
    ( (This)->lpVtbl -> put_DesktopWidth(This,pVal) ) 

#define IMsTscAx_get_DesktopWidth(This,pVal)	\
    ( (This)->lpVtbl -> get_DesktopWidth(This,pVal) ) 

#define IMsTscAx_put_DesktopHeight(This,pVal)	\
    ( (This)->lpVtbl -> put_DesktopHeight(This,pVal) ) 

#define IMsTscAx_get_DesktopHeight(This,pVal)	\
    ( (This)->lpVtbl -> get_DesktopHeight(This,pVal) ) 

#define IMsTscAx_put_StartConnected(This,pfStartConnected)	\
    ( (This)->lpVtbl -> put_StartConnected(This,pfStartConnected) ) 

#define IMsTscAx_get_StartConnected(This,pfStartConnected)	\
    ( (This)->lpVtbl -> get_StartConnected(This,pfStartConnected) ) 

#define IMsTscAx_get_HorizontalScrollBarVisible(This,pfHScrollVisible)	\
    ( (This)->lpVtbl -> get_HorizontalScrollBarVisible(This,pfHScrollVisible) ) 

#define IMsTscAx_get_VerticalScrollBarVisible(This,pfVScrollVisible)	\
    ( (This)->lpVtbl -> get_VerticalScrollBarVisible(This,pfVScrollVisible) ) 

#define IMsTscAx_put_FullScreenTitle(This,rhs)	\
    ( (This)->lpVtbl -> put_FullScreenTitle(This,rhs) ) 

#define IMsTscAx_get_CipherStrength(This,pCipherStrength)	\
    ( (This)->lpVtbl -> get_CipherStrength(This,pCipherStrength) ) 

#define IMsTscAx_get_Version(This,pVersion)	\
    ( (This)->lpVtbl -> get_Version(This,pVersion) ) 

#define IMsTscAx_get_SecuredSettingsEnabled(This,pSecuredSettingsEnabled)	\
    ( (This)->lpVtbl -> get_SecuredSettingsEnabled(This,pSecuredSettingsEnabled) ) 

#define IMsTscAx_get_SecuredSettings(This,ppSecuredSettings)	\
    ( (This)->lpVtbl -> get_SecuredSettings(This,ppSecuredSettings) ) 

#define IMsTscAx_get_AdvancedSettings(This,ppAdvSettings)	\
    ( (This)->lpVtbl -> get_AdvancedSettings(This,ppAdvSettings) ) 

#define IMsTscAx_get_Debugger(This,ppDebugger)	\
    ( (This)->lpVtbl -> get_Debugger(This,ppDebugger) ) 

#define IMsTscAx_Connect(This)	\
    ( (This)->lpVtbl -> Connect(This) ) 

#define IMsTscAx_Disconnect(This)	\
    ( (This)->lpVtbl -> Disconnect(This) ) 

#define IMsTscAx_CreateVirtualChannels(This,newVal)	\
    ( (This)->lpVtbl -> CreateVirtualChannels(This,newVal) ) 

#define IMsTscAx_SendOnVirtualChannel(This,chanName,ChanData)	\
    ( (This)->lpVtbl -> SendOnVirtualChannel(This,chanName,ChanData) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMsTscAx_INTERFACE_DEFINED__ */


#ifndef __IMsRdpClient_INTERFACE_DEFINED__
#define __IMsRdpClient_INTERFACE_DEFINED__

/* interface IMsRdpClient */
/* [object][oleautomation][dual][uuid] */ 


EXTERN_C const IID IID_IMsRdpClient;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("92B4A539-7115-4B7C-A5A9-E5D9EFC2780A")
    IMsRdpClient : public IMsTscAx
    {
    public:
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_ColorDepth( 
            /* [in] */ long pcolorDepth) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_ColorDepth( 
            /* [retval][out] */ long *pcolorDepth) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_AdvancedSettings2( 
            /* [retval][out] */ IMsRdpClientAdvancedSettings **ppAdvSettings) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_SecuredSettings2( 
            /* [retval][out] */ IMsRdpClientSecuredSettings **ppSecuredSettings) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_ExtendedDisconnectReason( 
            /* [retval][out] */ ExtendedDisconnectReasonCode *pExtendedDisconnectReason) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_FullScreen( 
            /* [in] */ VARIANT_BOOL pfFullScreen) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_FullScreen( 
            /* [retval][out] */ VARIANT_BOOL *pfFullScreen) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetVirtualChannelOptions( 
            /* [in] */ BSTR chanName,
            /* [in] */ long chanOptions) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetVirtualChannelOptions( 
            /* [in] */ BSTR chanName,
            /* [retval][out] */ long *pChanOptions) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE RequestClose( 
            /* [retval][out] */ ControlCloseStatus *pCloseStatus) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMsRdpClientVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMsRdpClient * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMsRdpClient * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMsRdpClient * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IMsRdpClient * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IMsRdpClient * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IMsRdpClient * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IMsRdpClient * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Server )( 
            IMsRdpClient * This,
            /* [in] */ BSTR pServer);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Server )( 
            IMsRdpClient * This,
            /* [retval][out] */ BSTR *pServer);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Domain )( 
            IMsRdpClient * This,
            /* [in] */ BSTR pDomain);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Domain )( 
            IMsRdpClient * This,
            /* [retval][out] */ BSTR *pDomain);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_UserName )( 
            IMsRdpClient * This,
            /* [in] */ BSTR pUserName);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_UserName )( 
            IMsRdpClient * This,
            /* [retval][out] */ BSTR *pUserName);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DisconnectedText )( 
            IMsRdpClient * This,
            /* [in] */ BSTR pDisconnectedText);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DisconnectedText )( 
            IMsRdpClient * This,
            /* [retval][out] */ BSTR *pDisconnectedText);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ConnectingText )( 
            IMsRdpClient * This,
            /* [in] */ BSTR pConnectingText);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ConnectingText )( 
            IMsRdpClient * This,
            /* [retval][out] */ BSTR *pConnectingText);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Connected )( 
            IMsRdpClient * This,
            /* [retval][out] */ short *pIsConnected);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DesktopWidth )( 
            IMsRdpClient * This,
            /* [in] */ long pVal);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DesktopWidth )( 
            IMsRdpClient * This,
            /* [retval][out] */ long *pVal);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DesktopHeight )( 
            IMsRdpClient * This,
            /* [in] */ long pVal);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DesktopHeight )( 
            IMsRdpClient * This,
            /* [retval][out] */ long *pVal);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_StartConnected )( 
            IMsRdpClient * This,
            /* [in] */ long pfStartConnected);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_StartConnected )( 
            IMsRdpClient * This,
            /* [retval][out] */ long *pfStartConnected);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HorizontalScrollBarVisible )( 
            IMsRdpClient * This,
            /* [retval][out] */ long *pfHScrollVisible);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_VerticalScrollBarVisible )( 
            IMsRdpClient * This,
            /* [retval][out] */ long *pfVScrollVisible);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_FullScreenTitle )( 
            IMsRdpClient * This,
            /* [in] */ BSTR rhs);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CipherStrength )( 
            IMsRdpClient * This,
            /* [retval][out] */ long *pCipherStrength);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Version )( 
            IMsRdpClient * This,
            /* [retval][out] */ BSTR *pVersion);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SecuredSettingsEnabled )( 
            IMsRdpClient * This,
            /* [retval][out] */ long *pSecuredSettingsEnabled);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SecuredSettings )( 
            IMsRdpClient * This,
            /* [retval][out] */ IMsTscSecuredSettings **ppSecuredSettings);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AdvancedSettings )( 
            IMsRdpClient * This,
            /* [retval][out] */ IMsTscAdvancedSettings **ppAdvSettings);
        
        /* [hidden][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Debugger )( 
            IMsRdpClient * This,
            /* [retval][out] */ IMsTscDebug **ppDebugger);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Connect )( 
            IMsRdpClient * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Disconnect )( 
            IMsRdpClient * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateVirtualChannels )( 
            IMsRdpClient * This,
            /* [in] */ BSTR newVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SendOnVirtualChannel )( 
            IMsRdpClient * This,
            /* [in] */ BSTR chanName,
            /* [in] */ BSTR ChanData);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ColorDepth )( 
            IMsRdpClient * This,
            /* [in] */ long pcolorDepth);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ColorDepth )( 
            IMsRdpClient * This,
            /* [retval][out] */ long *pcolorDepth);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AdvancedSettings2 )( 
            IMsRdpClient * This,
            /* [retval][out] */ IMsRdpClientAdvancedSettings **ppAdvSettings);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SecuredSettings2 )( 
            IMsRdpClient * This,
            /* [retval][out] */ IMsRdpClientSecuredSettings **ppSecuredSettings);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ExtendedDisconnectReason )( 
            IMsRdpClient * This,
            /* [retval][out] */ ExtendedDisconnectReasonCode *pExtendedDisconnectReason);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_FullScreen )( 
            IMsRdpClient * This,
            /* [in] */ VARIANT_BOOL pfFullScreen);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_FullScreen )( 
            IMsRdpClient * This,
            /* [retval][out] */ VARIANT_BOOL *pfFullScreen);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetVirtualChannelOptions )( 
            IMsRdpClient * This,
            /* [in] */ BSTR chanName,
            /* [in] */ long chanOptions);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetVirtualChannelOptions )( 
            IMsRdpClient * This,
            /* [in] */ BSTR chanName,
            /* [retval][out] */ long *pChanOptions);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RequestClose )( 
            IMsRdpClient * This,
            /* [retval][out] */ ControlCloseStatus *pCloseStatus);
        
        END_INTERFACE
    } IMsRdpClientVtbl;

    interface IMsRdpClient
    {
        CONST_VTBL struct IMsRdpClientVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMsRdpClient_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMsRdpClient_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMsRdpClient_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMsRdpClient_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IMsRdpClient_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IMsRdpClient_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IMsRdpClient_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IMsRdpClient_put_Server(This,pServer)	\
    ( (This)->lpVtbl -> put_Server(This,pServer) ) 

#define IMsRdpClient_get_Server(This,pServer)	\
    ( (This)->lpVtbl -> get_Server(This,pServer) ) 

#define IMsRdpClient_put_Domain(This,pDomain)	\
    ( (This)->lpVtbl -> put_Domain(This,pDomain) ) 

#define IMsRdpClient_get_Domain(This,pDomain)	\
    ( (This)->lpVtbl -> get_Domain(This,pDomain) ) 

#define IMsRdpClient_put_UserName(This,pUserName)	\
    ( (This)->lpVtbl -> put_UserName(This,pUserName) ) 

#define IMsRdpClient_get_UserName(This,pUserName)	\
    ( (This)->lpVtbl -> get_UserName(This,pUserName) ) 

#define IMsRdpClient_put_DisconnectedText(This,pDisconnectedText)	\
    ( (This)->lpVtbl -> put_DisconnectedText(This,pDisconnectedText) ) 

#define IMsRdpClient_get_DisconnectedText(This,pDisconnectedText)	\
    ( (This)->lpVtbl -> get_DisconnectedText(This,pDisconnectedText) ) 

#define IMsRdpClient_put_ConnectingText(This,pConnectingText)	\
    ( (This)->lpVtbl -> put_ConnectingText(This,pConnectingText) ) 

#define IMsRdpClient_get_ConnectingText(This,pConnectingText)	\
    ( (This)->lpVtbl -> get_ConnectingText(This,pConnectingText) ) 

#define IMsRdpClient_get_Connected(This,pIsConnected)	\
    ( (This)->lpVtbl -> get_Connected(This,pIsConnected) ) 

#define IMsRdpClient_put_DesktopWidth(This,pVal)	\
    ( (This)->lpVtbl -> put_DesktopWidth(This,pVal) ) 

#define IMsRdpClient_get_DesktopWidth(This,pVal)	\
    ( (This)->lpVtbl -> get_DesktopWidth(This,pVal) ) 

#define IMsRdpClient_put_DesktopHeight(This,pVal)	\
    ( (This)->lpVtbl -> put_DesktopHeight(This,pVal) ) 

#define IMsRdpClient_get_DesktopHeight(This,pVal)	\
    ( (This)->lpVtbl -> get_DesktopHeight(This,pVal) ) 

#define IMsRdpClient_put_StartConnected(This,pfStartConnected)	\
    ( (This)->lpVtbl -> put_StartConnected(This,pfStartConnected) ) 

#define IMsRdpClient_get_StartConnected(This,pfStartConnected)	\
    ( (This)->lpVtbl -> get_StartConnected(This,pfStartConnected) ) 

#define IMsRdpClient_get_HorizontalScrollBarVisible(This,pfHScrollVisible)	\
    ( (This)->lpVtbl -> get_HorizontalScrollBarVisible(This,pfHScrollVisible) ) 

#define IMsRdpClient_get_VerticalScrollBarVisible(This,pfVScrollVisible)	\
    ( (This)->lpVtbl -> get_VerticalScrollBarVisible(This,pfVScrollVisible) ) 

#define IMsRdpClient_put_FullScreenTitle(This,rhs)	\
    ( (This)->lpVtbl -> put_FullScreenTitle(This,rhs) ) 

#define IMsRdpClient_get_CipherStrength(This,pCipherStrength)	\
    ( (This)->lpVtbl -> get_CipherStrength(This,pCipherStrength) ) 

#define IMsRdpClient_get_Version(This,pVersion)	\
    ( (This)->lpVtbl -> get_Version(This,pVersion) ) 

#define IMsRdpClient_get_SecuredSettingsEnabled(This,pSecuredSettingsEnabled)	\
    ( (This)->lpVtbl -> get_SecuredSettingsEnabled(This,pSecuredSettingsEnabled) ) 

#define IMsRdpClient_get_SecuredSettings(This,ppSecuredSettings)	\
    ( (This)->lpVtbl -> get_SecuredSettings(This,ppSecuredSettings) ) 

#define IMsRdpClient_get_AdvancedSettings(This,ppAdvSettings)	\
    ( (This)->lpVtbl -> get_AdvancedSettings(This,ppAdvSettings) ) 

#define IMsRdpClient_get_Debugger(This,ppDebugger)	\
    ( (This)->lpVtbl -> get_Debugger(This,ppDebugger) ) 

#define IMsRdpClient_Connect(This)	\
    ( (This)->lpVtbl -> Connect(This) ) 

#define IMsRdpClient_Disconnect(This)	\
    ( (This)->lpVtbl -> Disconnect(This) ) 

#define IMsRdpClient_CreateVirtualChannels(This,newVal)	\
    ( (This)->lpVtbl -> CreateVirtualChannels(This,newVal) ) 

#define IMsRdpClient_SendOnVirtualChannel(This,chanName,ChanData)	\
    ( (This)->lpVtbl -> SendOnVirtualChannel(This,chanName,ChanData) ) 


#define IMsRdpClient_put_ColorDepth(This,pcolorDepth)	\
    ( (This)->lpVtbl -> put_ColorDepth(This,pcolorDepth) ) 

#define IMsRdpClient_get_ColorDepth(This,pcolorDepth)	\
    ( (This)->lpVtbl -> get_ColorDepth(This,pcolorDepth) ) 

#define IMsRdpClient_get_AdvancedSettings2(This,ppAdvSettings)	\
    ( (This)->lpVtbl -> get_AdvancedSettings2(This,ppAdvSettings) ) 

#define IMsRdpClient_get_SecuredSettings2(This,ppSecuredSettings)	\
    ( (This)->lpVtbl -> get_SecuredSettings2(This,ppSecuredSettings) ) 

#define IMsRdpClient_get_ExtendedDisconnectReason(This,pExtendedDisconnectReason)	\
    ( (This)->lpVtbl -> get_ExtendedDisconnectReason(This,pExtendedDisconnectReason) ) 

#define IMsRdpClient_put_FullScreen(This,pfFullScreen)	\
    ( (This)->lpVtbl -> put_FullScreen(This,pfFullScreen) ) 

#define IMsRdpClient_get_FullScreen(This,pfFullScreen)	\
    ( (This)->lpVtbl -> get_FullScreen(This,pfFullScreen) ) 

#define IMsRdpClient_SetVirtualChannelOptions(This,chanName,chanOptions)	\
    ( (This)->lpVtbl -> SetVirtualChannelOptions(This,chanName,chanOptions) ) 

#define IMsRdpClient_GetVirtualChannelOptions(This,chanName,pChanOptions)	\
    ( (This)->lpVtbl -> GetVirtualChannelOptions(This,chanName,pChanOptions) ) 

#define IMsRdpClient_RequestClose(This,pCloseStatus)	\
    ( (This)->lpVtbl -> RequestClose(This,pCloseStatus) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMsRdpClient_INTERFACE_DEFINED__ */


#ifndef __IMsRdpClient2_INTERFACE_DEFINED__
#define __IMsRdpClient2_INTERFACE_DEFINED__

/* interface IMsRdpClient2 */
/* [object][oleautomation][dual][uuid] */ 


EXTERN_C const IID IID_IMsRdpClient2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E7E17DC4-3B71-4BA7-A8E6-281FFADCA28F")
    IMsRdpClient2 : public IMsRdpClient
    {
    public:
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_AdvancedSettings3( 
            /* [retval][out] */ IMsRdpClientAdvancedSettings2 **ppAdvSettings) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_ConnectedStatusText( 
            /* [in] */ BSTR pConnectedStatusText) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_ConnectedStatusText( 
            /* [retval][out] */ BSTR *pConnectedStatusText) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMsRdpClient2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMsRdpClient2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMsRdpClient2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMsRdpClient2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IMsRdpClient2 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IMsRdpClient2 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IMsRdpClient2 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IMsRdpClient2 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Server )( 
            IMsRdpClient2 * This,
            /* [in] */ BSTR pServer);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Server )( 
            IMsRdpClient2 * This,
            /* [retval][out] */ BSTR *pServer);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Domain )( 
            IMsRdpClient2 * This,
            /* [in] */ BSTR pDomain);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Domain )( 
            IMsRdpClient2 * This,
            /* [retval][out] */ BSTR *pDomain);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_UserName )( 
            IMsRdpClient2 * This,
            /* [in] */ BSTR pUserName);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_UserName )( 
            IMsRdpClient2 * This,
            /* [retval][out] */ BSTR *pUserName);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DisconnectedText )( 
            IMsRdpClient2 * This,
            /* [in] */ BSTR pDisconnectedText);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DisconnectedText )( 
            IMsRdpClient2 * This,
            /* [retval][out] */ BSTR *pDisconnectedText);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ConnectingText )( 
            IMsRdpClient2 * This,
            /* [in] */ BSTR pConnectingText);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ConnectingText )( 
            IMsRdpClient2 * This,
            /* [retval][out] */ BSTR *pConnectingText);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Connected )( 
            IMsRdpClient2 * This,
            /* [retval][out] */ short *pIsConnected);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DesktopWidth )( 
            IMsRdpClient2 * This,
            /* [in] */ long pVal);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DesktopWidth )( 
            IMsRdpClient2 * This,
            /* [retval][out] */ long *pVal);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DesktopHeight )( 
            IMsRdpClient2 * This,
            /* [in] */ long pVal);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DesktopHeight )( 
            IMsRdpClient2 * This,
            /* [retval][out] */ long *pVal);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_StartConnected )( 
            IMsRdpClient2 * This,
            /* [in] */ long pfStartConnected);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_StartConnected )( 
            IMsRdpClient2 * This,
            /* [retval][out] */ long *pfStartConnected);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HorizontalScrollBarVisible )( 
            IMsRdpClient2 * This,
            /* [retval][out] */ long *pfHScrollVisible);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_VerticalScrollBarVisible )( 
            IMsRdpClient2 * This,
            /* [retval][out] */ long *pfVScrollVisible);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_FullScreenTitle )( 
            IMsRdpClient2 * This,
            /* [in] */ BSTR rhs);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CipherStrength )( 
            IMsRdpClient2 * This,
            /* [retval][out] */ long *pCipherStrength);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Version )( 
            IMsRdpClient2 * This,
            /* [retval][out] */ BSTR *pVersion);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SecuredSettingsEnabled )( 
            IMsRdpClient2 * This,
            /* [retval][out] */ long *pSecuredSettingsEnabled);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SecuredSettings )( 
            IMsRdpClient2 * This,
            /* [retval][out] */ IMsTscSecuredSettings **ppSecuredSettings);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AdvancedSettings )( 
            IMsRdpClient2 * This,
            /* [retval][out] */ IMsTscAdvancedSettings **ppAdvSettings);
        
        /* [hidden][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Debugger )( 
            IMsRdpClient2 * This,
            /* [retval][out] */ IMsTscDebug **ppDebugger);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Connect )( 
            IMsRdpClient2 * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Disconnect )( 
            IMsRdpClient2 * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateVirtualChannels )( 
            IMsRdpClient2 * This,
            /* [in] */ BSTR newVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SendOnVirtualChannel )( 
            IMsRdpClient2 * This,
            /* [in] */ BSTR chanName,
            /* [in] */ BSTR ChanData);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ColorDepth )( 
            IMsRdpClient2 * This,
            /* [in] */ long pcolorDepth);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ColorDepth )( 
            IMsRdpClient2 * This,
            /* [retval][out] */ long *pcolorDepth);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AdvancedSettings2 )( 
            IMsRdpClient2 * This,
            /* [retval][out] */ IMsRdpClientAdvancedSettings **ppAdvSettings);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SecuredSettings2 )( 
            IMsRdpClient2 * This,
            /* [retval][out] */ IMsRdpClientSecuredSettings **ppSecuredSettings);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ExtendedDisconnectReason )( 
            IMsRdpClient2 * This,
            /* [retval][out] */ ExtendedDisconnectReasonCode *pExtendedDisconnectReason);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_FullScreen )( 
            IMsRdpClient2 * This,
            /* [in] */ VARIANT_BOOL pfFullScreen);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_FullScreen )( 
            IMsRdpClient2 * This,
            /* [retval][out] */ VARIANT_BOOL *pfFullScreen);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetVirtualChannelOptions )( 
            IMsRdpClient2 * This,
            /* [in] */ BSTR chanName,
            /* [in] */ long chanOptions);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetVirtualChannelOptions )( 
            IMsRdpClient2 * This,
            /* [in] */ BSTR chanName,
            /* [retval][out] */ long *pChanOptions);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RequestClose )( 
            IMsRdpClient2 * This,
            /* [retval][out] */ ControlCloseStatus *pCloseStatus);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AdvancedSettings3 )( 
            IMsRdpClient2 * This,
            /* [retval][out] */ IMsRdpClientAdvancedSettings2 **ppAdvSettings);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ConnectedStatusText )( 
            IMsRdpClient2 * This,
            /* [in] */ BSTR pConnectedStatusText);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ConnectedStatusText )( 
            IMsRdpClient2 * This,
            /* [retval][out] */ BSTR *pConnectedStatusText);
        
        END_INTERFACE
    } IMsRdpClient2Vtbl;

    interface IMsRdpClient2
    {
        CONST_VTBL struct IMsRdpClient2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMsRdpClient2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMsRdpClient2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMsRdpClient2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMsRdpClient2_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IMsRdpClient2_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IMsRdpClient2_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IMsRdpClient2_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IMsRdpClient2_put_Server(This,pServer)	\
    ( (This)->lpVtbl -> put_Server(This,pServer) ) 

#define IMsRdpClient2_get_Server(This,pServer)	\
    ( (This)->lpVtbl -> get_Server(This,pServer) ) 

#define IMsRdpClient2_put_Domain(This,pDomain)	\
    ( (This)->lpVtbl -> put_Domain(This,pDomain) ) 

#define IMsRdpClient2_get_Domain(This,pDomain)	\
    ( (This)->lpVtbl -> get_Domain(This,pDomain) ) 

#define IMsRdpClient2_put_UserName(This,pUserName)	\
    ( (This)->lpVtbl -> put_UserName(This,pUserName) ) 

#define IMsRdpClient2_get_UserName(This,pUserName)	\
    ( (This)->lpVtbl -> get_UserName(This,pUserName) ) 

#define IMsRdpClient2_put_DisconnectedText(This,pDisconnectedText)	\
    ( (This)->lpVtbl -> put_DisconnectedText(This,pDisconnectedText) ) 

#define IMsRdpClient2_get_DisconnectedText(This,pDisconnectedText)	\
    ( (This)->lpVtbl -> get_DisconnectedText(This,pDisconnectedText) ) 

#define IMsRdpClient2_put_ConnectingText(This,pConnectingText)	\
    ( (This)->lpVtbl -> put_ConnectingText(This,pConnectingText) ) 

#define IMsRdpClient2_get_ConnectingText(This,pConnectingText)	\
    ( (This)->lpVtbl -> get_ConnectingText(This,pConnectingText) ) 

#define IMsRdpClient2_get_Connected(This,pIsConnected)	\
    ( (This)->lpVtbl -> get_Connected(This,pIsConnected) ) 

#define IMsRdpClient2_put_DesktopWidth(This,pVal)	\
    ( (This)->lpVtbl -> put_DesktopWidth(This,pVal) ) 

#define IMsRdpClient2_get_DesktopWidth(This,pVal)	\
    ( (This)->lpVtbl -> get_DesktopWidth(This,pVal) ) 

#define IMsRdpClient2_put_DesktopHeight(This,pVal)	\
    ( (This)->lpVtbl -> put_DesktopHeight(This,pVal) ) 

#define IMsRdpClient2_get_DesktopHeight(This,pVal)	\
    ( (This)->lpVtbl -> get_DesktopHeight(This,pVal) ) 

#define IMsRdpClient2_put_StartConnected(This,pfStartConnected)	\
    ( (This)->lpVtbl -> put_StartConnected(This,pfStartConnected) ) 

#define IMsRdpClient2_get_StartConnected(This,pfStartConnected)	\
    ( (This)->lpVtbl -> get_StartConnected(This,pfStartConnected) ) 

#define IMsRdpClient2_get_HorizontalScrollBarVisible(This,pfHScrollVisible)	\
    ( (This)->lpVtbl -> get_HorizontalScrollBarVisible(This,pfHScrollVisible) ) 

#define IMsRdpClient2_get_VerticalScrollBarVisible(This,pfVScrollVisible)	\
    ( (This)->lpVtbl -> get_VerticalScrollBarVisible(This,pfVScrollVisible) ) 

#define IMsRdpClient2_put_FullScreenTitle(This,rhs)	\
    ( (This)->lpVtbl -> put_FullScreenTitle(This,rhs) ) 

#define IMsRdpClient2_get_CipherStrength(This,pCipherStrength)	\
    ( (This)->lpVtbl -> get_CipherStrength(This,pCipherStrength) ) 

#define IMsRdpClient2_get_Version(This,pVersion)	\
    ( (This)->lpVtbl -> get_Version(This,pVersion) ) 

#define IMsRdpClient2_get_SecuredSettingsEnabled(This,pSecuredSettingsEnabled)	\
    ( (This)->lpVtbl -> get_SecuredSettingsEnabled(This,pSecuredSettingsEnabled) ) 

#define IMsRdpClient2_get_SecuredSettings(This,ppSecuredSettings)	\
    ( (This)->lpVtbl -> get_SecuredSettings(This,ppSecuredSettings) ) 

#define IMsRdpClient2_get_AdvancedSettings(This,ppAdvSettings)	\
    ( (This)->lpVtbl -> get_AdvancedSettings(This,ppAdvSettings) ) 

#define IMsRdpClient2_get_Debugger(This,ppDebugger)	\
    ( (This)->lpVtbl -> get_Debugger(This,ppDebugger) ) 

#define IMsRdpClient2_Connect(This)	\
    ( (This)->lpVtbl -> Connect(This) ) 

#define IMsRdpClient2_Disconnect(This)	\
    ( (This)->lpVtbl -> Disconnect(This) ) 

#define IMsRdpClient2_CreateVirtualChannels(This,newVal)	\
    ( (This)->lpVtbl -> CreateVirtualChannels(This,newVal) ) 

#define IMsRdpClient2_SendOnVirtualChannel(This,chanName,ChanData)	\
    ( (This)->lpVtbl -> SendOnVirtualChannel(This,chanName,ChanData) ) 


#define IMsRdpClient2_put_ColorDepth(This,pcolorDepth)	\
    ( (This)->lpVtbl -> put_ColorDepth(This,pcolorDepth) ) 

#define IMsRdpClient2_get_ColorDepth(This,pcolorDepth)	\
    ( (This)->lpVtbl -> get_ColorDepth(This,pcolorDepth) ) 

#define IMsRdpClient2_get_AdvancedSettings2(This,ppAdvSettings)	\
    ( (This)->lpVtbl -> get_AdvancedSettings2(This,ppAdvSettings) ) 

#define IMsRdpClient2_get_SecuredSettings2(This,ppSecuredSettings)	\
    ( (This)->lpVtbl -> get_SecuredSettings2(This,ppSecuredSettings) ) 

#define IMsRdpClient2_get_ExtendedDisconnectReason(This,pExtendedDisconnectReason)	\
    ( (This)->lpVtbl -> get_ExtendedDisconnectReason(This,pExtendedDisconnectReason) ) 

#define IMsRdpClient2_put_FullScreen(This,pfFullScreen)	\
    ( (This)->lpVtbl -> put_FullScreen(This,pfFullScreen) ) 

#define IMsRdpClient2_get_FullScreen(This,pfFullScreen)	\
    ( (This)->lpVtbl -> get_FullScreen(This,pfFullScreen) ) 

#define IMsRdpClient2_SetVirtualChannelOptions(This,chanName,chanOptions)	\
    ( (This)->lpVtbl -> SetVirtualChannelOptions(This,chanName,chanOptions) ) 

#define IMsRdpClient2_GetVirtualChannelOptions(This,chanName,pChanOptions)	\
    ( (This)->lpVtbl -> GetVirtualChannelOptions(This,chanName,pChanOptions) ) 

#define IMsRdpClient2_RequestClose(This,pCloseStatus)	\
    ( (This)->lpVtbl -> RequestClose(This,pCloseStatus) ) 


#define IMsRdpClient2_get_AdvancedSettings3(This,ppAdvSettings)	\
    ( (This)->lpVtbl -> get_AdvancedSettings3(This,ppAdvSettings) ) 

#define IMsRdpClient2_put_ConnectedStatusText(This,pConnectedStatusText)	\
    ( (This)->lpVtbl -> put_ConnectedStatusText(This,pConnectedStatusText) ) 

#define IMsRdpClient2_get_ConnectedStatusText(This,pConnectedStatusText)	\
    ( (This)->lpVtbl -> get_ConnectedStatusText(This,pConnectedStatusText) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMsRdpClient2_INTERFACE_DEFINED__ */


#ifndef __IMsRdpClient3_INTERFACE_DEFINED__
#define __IMsRdpClient3_INTERFACE_DEFINED__

/* interface IMsRdpClient3 */
/* [object][oleautomation][dual][uuid] */ 


EXTERN_C const IID IID_IMsRdpClient3;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("91B7CBC5-A72E-4FA0-9300-D647D7E897FF")
    IMsRdpClient3 : public IMsRdpClient2
    {
    public:
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_AdvancedSettings4( 
            /* [retval][out] */ IMsRdpClientAdvancedSettings3 **ppAdvSettings) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMsRdpClient3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMsRdpClient3 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMsRdpClient3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMsRdpClient3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IMsRdpClient3 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IMsRdpClient3 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IMsRdpClient3 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IMsRdpClient3 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Server )( 
            IMsRdpClient3 * This,
            /* [in] */ BSTR pServer);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Server )( 
            IMsRdpClient3 * This,
            /* [retval][out] */ BSTR *pServer);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Domain )( 
            IMsRdpClient3 * This,
            /* [in] */ BSTR pDomain);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Domain )( 
            IMsRdpClient3 * This,
            /* [retval][out] */ BSTR *pDomain);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_UserName )( 
            IMsRdpClient3 * This,
            /* [in] */ BSTR pUserName);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_UserName )( 
            IMsRdpClient3 * This,
            /* [retval][out] */ BSTR *pUserName);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DisconnectedText )( 
            IMsRdpClient3 * This,
            /* [in] */ BSTR pDisconnectedText);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DisconnectedText )( 
            IMsRdpClient3 * This,
            /* [retval][out] */ BSTR *pDisconnectedText);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ConnectingText )( 
            IMsRdpClient3 * This,
            /* [in] */ BSTR pConnectingText);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ConnectingText )( 
            IMsRdpClient3 * This,
            /* [retval][out] */ BSTR *pConnectingText);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Connected )( 
            IMsRdpClient3 * This,
            /* [retval][out] */ short *pIsConnected);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DesktopWidth )( 
            IMsRdpClient3 * This,
            /* [in] */ long pVal);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DesktopWidth )( 
            IMsRdpClient3 * This,
            /* [retval][out] */ long *pVal);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DesktopHeight )( 
            IMsRdpClient3 * This,
            /* [in] */ long pVal);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DesktopHeight )( 
            IMsRdpClient3 * This,
            /* [retval][out] */ long *pVal);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_StartConnected )( 
            IMsRdpClient3 * This,
            /* [in] */ long pfStartConnected);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_StartConnected )( 
            IMsRdpClient3 * This,
            /* [retval][out] */ long *pfStartConnected);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HorizontalScrollBarVisible )( 
            IMsRdpClient3 * This,
            /* [retval][out] */ long *pfHScrollVisible);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_VerticalScrollBarVisible )( 
            IMsRdpClient3 * This,
            /* [retval][out] */ long *pfVScrollVisible);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_FullScreenTitle )( 
            IMsRdpClient3 * This,
            /* [in] */ BSTR rhs);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CipherStrength )( 
            IMsRdpClient3 * This,
            /* [retval][out] */ long *pCipherStrength);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Version )( 
            IMsRdpClient3 * This,
            /* [retval][out] */ BSTR *pVersion);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SecuredSettingsEnabled )( 
            IMsRdpClient3 * This,
            /* [retval][out] */ long *pSecuredSettingsEnabled);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SecuredSettings )( 
            IMsRdpClient3 * This,
            /* [retval][out] */ IMsTscSecuredSettings **ppSecuredSettings);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AdvancedSettings )( 
            IMsRdpClient3 * This,
            /* [retval][out] */ IMsTscAdvancedSettings **ppAdvSettings);
        
        /* [hidden][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Debugger )( 
            IMsRdpClient3 * This,
            /* [retval][out] */ IMsTscDebug **ppDebugger);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Connect )( 
            IMsRdpClient3 * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Disconnect )( 
            IMsRdpClient3 * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateVirtualChannels )( 
            IMsRdpClient3 * This,
            /* [in] */ BSTR newVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SendOnVirtualChannel )( 
            IMsRdpClient3 * This,
            /* [in] */ BSTR chanName,
            /* [in] */ BSTR ChanData);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ColorDepth )( 
            IMsRdpClient3 * This,
            /* [in] */ long pcolorDepth);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ColorDepth )( 
            IMsRdpClient3 * This,
            /* [retval][out] */ long *pcolorDepth);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AdvancedSettings2 )( 
            IMsRdpClient3 * This,
            /* [retval][out] */ IMsRdpClientAdvancedSettings **ppAdvSettings);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SecuredSettings2 )( 
            IMsRdpClient3 * This,
            /* [retval][out] */ IMsRdpClientSecuredSettings **ppSecuredSettings);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ExtendedDisconnectReason )( 
            IMsRdpClient3 * This,
            /* [retval][out] */ ExtendedDisconnectReasonCode *pExtendedDisconnectReason);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_FullScreen )( 
            IMsRdpClient3 * This,
            /* [in] */ VARIANT_BOOL pfFullScreen);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_FullScreen )( 
            IMsRdpClient3 * This,
            /* [retval][out] */ VARIANT_BOOL *pfFullScreen);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetVirtualChannelOptions )( 
            IMsRdpClient3 * This,
            /* [in] */ BSTR chanName,
            /* [in] */ long chanOptions);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetVirtualChannelOptions )( 
            IMsRdpClient3 * This,
            /* [in] */ BSTR chanName,
            /* [retval][out] */ long *pChanOptions);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RequestClose )( 
            IMsRdpClient3 * This,
            /* [retval][out] */ ControlCloseStatus *pCloseStatus);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AdvancedSettings3 )( 
            IMsRdpClient3 * This,
            /* [retval][out] */ IMsRdpClientAdvancedSettings2 **ppAdvSettings);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ConnectedStatusText )( 
            IMsRdpClient3 * This,
            /* [in] */ BSTR pConnectedStatusText);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ConnectedStatusText )( 
            IMsRdpClient3 * This,
            /* [retval][out] */ BSTR *pConnectedStatusText);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AdvancedSettings4 )( 
            IMsRdpClient3 * This,
            /* [retval][out] */ IMsRdpClientAdvancedSettings3 **ppAdvSettings);
        
        END_INTERFACE
    } IMsRdpClient3Vtbl;

    interface IMsRdpClient3
    {
        CONST_VTBL struct IMsRdpClient3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMsRdpClient3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMsRdpClient3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMsRdpClient3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMsRdpClient3_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IMsRdpClient3_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IMsRdpClient3_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IMsRdpClient3_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IMsRdpClient3_put_Server(This,pServer)	\
    ( (This)->lpVtbl -> put_Server(This,pServer) ) 

#define IMsRdpClient3_get_Server(This,pServer)	\
    ( (This)->lpVtbl -> get_Server(This,pServer) ) 

#define IMsRdpClient3_put_Domain(This,pDomain)	\
    ( (This)->lpVtbl -> put_Domain(This,pDomain) ) 

#define IMsRdpClient3_get_Domain(This,pDomain)	\
    ( (This)->lpVtbl -> get_Domain(This,pDomain) ) 

#define IMsRdpClient3_put_UserName(This,pUserName)	\
    ( (This)->lpVtbl -> put_UserName(This,pUserName) ) 

#define IMsRdpClient3_get_UserName(This,pUserName)	\
    ( (This)->lpVtbl -> get_UserName(This,pUserName) ) 

#define IMsRdpClient3_put_DisconnectedText(This,pDisconnectedText)	\
    ( (This)->lpVtbl -> put_DisconnectedText(This,pDisconnectedText) ) 

#define IMsRdpClient3_get_DisconnectedText(This,pDisconnectedText)	\
    ( (This)->lpVtbl -> get_DisconnectedText(This,pDisconnectedText) ) 

#define IMsRdpClient3_put_ConnectingText(This,pConnectingText)	\
    ( (This)->lpVtbl -> put_ConnectingText(This,pConnectingText) ) 

#define IMsRdpClient3_get_ConnectingText(This,pConnectingText)	\
    ( (This)->lpVtbl -> get_ConnectingText(This,pConnectingText) ) 

#define IMsRdpClient3_get_Connected(This,pIsConnected)	\
    ( (This)->lpVtbl -> get_Connected(This,pIsConnected) ) 

#define IMsRdpClient3_put_DesktopWidth(This,pVal)	\
    ( (This)->lpVtbl -> put_DesktopWidth(This,pVal) ) 

#define IMsRdpClient3_get_DesktopWidth(This,pVal)	\
    ( (This)->lpVtbl -> get_DesktopWidth(This,pVal) ) 

#define IMsRdpClient3_put_DesktopHeight(This,pVal)	\
    ( (This)->lpVtbl -> put_DesktopHeight(This,pVal) ) 

#define IMsRdpClient3_get_DesktopHeight(This,pVal)	\
    ( (This)->lpVtbl -> get_DesktopHeight(This,pVal) ) 

#define IMsRdpClient3_put_StartConnected(This,pfStartConnected)	\
    ( (This)->lpVtbl -> put_StartConnected(This,pfStartConnected) ) 

#define IMsRdpClient3_get_StartConnected(This,pfStartConnected)	\
    ( (This)->lpVtbl -> get_StartConnected(This,pfStartConnected) ) 

#define IMsRdpClient3_get_HorizontalScrollBarVisible(This,pfHScrollVisible)	\
    ( (This)->lpVtbl -> get_HorizontalScrollBarVisible(This,pfHScrollVisible) ) 

#define IMsRdpClient3_get_VerticalScrollBarVisible(This,pfVScrollVisible)	\
    ( (This)->lpVtbl -> get_VerticalScrollBarVisible(This,pfVScrollVisible) ) 

#define IMsRdpClient3_put_FullScreenTitle(This,rhs)	\
    ( (This)->lpVtbl -> put_FullScreenTitle(This,rhs) ) 

#define IMsRdpClient3_get_CipherStrength(This,pCipherStrength)	\
    ( (This)->lpVtbl -> get_CipherStrength(This,pCipherStrength) ) 

#define IMsRdpClient3_get_Version(This,pVersion)	\
    ( (This)->lpVtbl -> get_Version(This,pVersion) ) 

#define IMsRdpClient3_get_SecuredSettingsEnabled(This,pSecuredSettingsEnabled)	\
    ( (This)->lpVtbl -> get_SecuredSettingsEnabled(This,pSecuredSettingsEnabled) ) 

#define IMsRdpClient3_get_SecuredSettings(This,ppSecuredSettings)	\
    ( (This)->lpVtbl -> get_SecuredSettings(This,ppSecuredSettings) ) 

#define IMsRdpClient3_get_AdvancedSettings(This,ppAdvSettings)	\
    ( (This)->lpVtbl -> get_AdvancedSettings(This,ppAdvSettings) ) 

#define IMsRdpClient3_get_Debugger(This,ppDebugger)	\
    ( (This)->lpVtbl -> get_Debugger(This,ppDebugger) ) 

#define IMsRdpClient3_Connect(This)	\
    ( (This)->lpVtbl -> Connect(This) ) 

#define IMsRdpClient3_Disconnect(This)	\
    ( (This)->lpVtbl -> Disconnect(This) ) 

#define IMsRdpClient3_CreateVirtualChannels(This,newVal)	\
    ( (This)->lpVtbl -> CreateVirtualChannels(This,newVal) ) 

#define IMsRdpClient3_SendOnVirtualChannel(This,chanName,ChanData)	\
    ( (This)->lpVtbl -> SendOnVirtualChannel(This,chanName,ChanData) ) 


#define IMsRdpClient3_put_ColorDepth(This,pcolorDepth)	\
    ( (This)->lpVtbl -> put_ColorDepth(This,pcolorDepth) ) 

#define IMsRdpClient3_get_ColorDepth(This,pcolorDepth)	\
    ( (This)->lpVtbl -> get_ColorDepth(This,pcolorDepth) ) 

#define IMsRdpClient3_get_AdvancedSettings2(This,ppAdvSettings)	\
    ( (This)->lpVtbl -> get_AdvancedSettings2(This,ppAdvSettings) ) 

#define IMsRdpClient3_get_SecuredSettings2(This,ppSecuredSettings)	\
    ( (This)->lpVtbl -> get_SecuredSettings2(This,ppSecuredSettings) ) 

#define IMsRdpClient3_get_ExtendedDisconnectReason(This,pExtendedDisconnectReason)	\
    ( (This)->lpVtbl -> get_ExtendedDisconnectReason(This,pExtendedDisconnectReason) ) 

#define IMsRdpClient3_put_FullScreen(This,pfFullScreen)	\
    ( (This)->lpVtbl -> put_FullScreen(This,pfFullScreen) ) 

#define IMsRdpClient3_get_FullScreen(This,pfFullScreen)	\
    ( (This)->lpVtbl -> get_FullScreen(This,pfFullScreen) ) 

#define IMsRdpClient3_SetVirtualChannelOptions(This,chanName,chanOptions)	\
    ( (This)->lpVtbl -> SetVirtualChannelOptions(This,chanName,chanOptions) ) 

#define IMsRdpClient3_GetVirtualChannelOptions(This,chanName,pChanOptions)	\
    ( (This)->lpVtbl -> GetVirtualChannelOptions(This,chanName,pChanOptions) ) 

#define IMsRdpClient3_RequestClose(This,pCloseStatus)	\
    ( (This)->lpVtbl -> RequestClose(This,pCloseStatus) ) 


#define IMsRdpClient3_get_AdvancedSettings3(This,ppAdvSettings)	\
    ( (This)->lpVtbl -> get_AdvancedSettings3(This,ppAdvSettings) ) 

#define IMsRdpClient3_put_ConnectedStatusText(This,pConnectedStatusText)	\
    ( (This)->lpVtbl -> put_ConnectedStatusText(This,pConnectedStatusText) ) 

#define IMsRdpClient3_get_ConnectedStatusText(This,pConnectedStatusText)	\
    ( (This)->lpVtbl -> get_ConnectedStatusText(This,pConnectedStatusText) ) 


#define IMsRdpClient3_get_AdvancedSettings4(This,ppAdvSettings)	\
    ( (This)->lpVtbl -> get_AdvancedSettings4(This,ppAdvSettings) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMsRdpClient3_INTERFACE_DEFINED__ */


#ifndef __IMsRdpClient4_INTERFACE_DEFINED__
#define __IMsRdpClient4_INTERFACE_DEFINED__

/* interface IMsRdpClient4 */
/* [object][oleautomation][dual][uuid] */ 


EXTERN_C const IID IID_IMsRdpClient4;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("095E0738-D97D-488B-B9F6-DD0E8D66C0DE")
    IMsRdpClient4 : public IMsRdpClient3
    {
    public:
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_AdvancedSettings5( 
            /* [retval][out] */ IMsRdpClientAdvancedSettings4 **ppAdvSettings5) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMsRdpClient4Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMsRdpClient4 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMsRdpClient4 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMsRdpClient4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IMsRdpClient4 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IMsRdpClient4 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IMsRdpClient4 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IMsRdpClient4 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Server )( 
            IMsRdpClient4 * This,
            /* [in] */ BSTR pServer);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Server )( 
            IMsRdpClient4 * This,
            /* [retval][out] */ BSTR *pServer);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Domain )( 
            IMsRdpClient4 * This,
            /* [in] */ BSTR pDomain);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Domain )( 
            IMsRdpClient4 * This,
            /* [retval][out] */ BSTR *pDomain);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_UserName )( 
            IMsRdpClient4 * This,
            /* [in] */ BSTR pUserName);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_UserName )( 
            IMsRdpClient4 * This,
            /* [retval][out] */ BSTR *pUserName);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DisconnectedText )( 
            IMsRdpClient4 * This,
            /* [in] */ BSTR pDisconnectedText);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DisconnectedText )( 
            IMsRdpClient4 * This,
            /* [retval][out] */ BSTR *pDisconnectedText);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ConnectingText )( 
            IMsRdpClient4 * This,
            /* [in] */ BSTR pConnectingText);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ConnectingText )( 
            IMsRdpClient4 * This,
            /* [retval][out] */ BSTR *pConnectingText);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Connected )( 
            IMsRdpClient4 * This,
            /* [retval][out] */ short *pIsConnected);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DesktopWidth )( 
            IMsRdpClient4 * This,
            /* [in] */ long pVal);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DesktopWidth )( 
            IMsRdpClient4 * This,
            /* [retval][out] */ long *pVal);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DesktopHeight )( 
            IMsRdpClient4 * This,
            /* [in] */ long pVal);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DesktopHeight )( 
            IMsRdpClient4 * This,
            /* [retval][out] */ long *pVal);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_StartConnected )( 
            IMsRdpClient4 * This,
            /* [in] */ long pfStartConnected);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_StartConnected )( 
            IMsRdpClient4 * This,
            /* [retval][out] */ long *pfStartConnected);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HorizontalScrollBarVisible )( 
            IMsRdpClient4 * This,
            /* [retval][out] */ long *pfHScrollVisible);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_VerticalScrollBarVisible )( 
            IMsRdpClient4 * This,
            /* [retval][out] */ long *pfVScrollVisible);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_FullScreenTitle )( 
            IMsRdpClient4 * This,
            /* [in] */ BSTR rhs);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CipherStrength )( 
            IMsRdpClient4 * This,
            /* [retval][out] */ long *pCipherStrength);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Version )( 
            IMsRdpClient4 * This,
            /* [retval][out] */ BSTR *pVersion);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SecuredSettingsEnabled )( 
            IMsRdpClient4 * This,
            /* [retval][out] */ long *pSecuredSettingsEnabled);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SecuredSettings )( 
            IMsRdpClient4 * This,
            /* [retval][out] */ IMsTscSecuredSettings **ppSecuredSettings);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AdvancedSettings )( 
            IMsRdpClient4 * This,
            /* [retval][out] */ IMsTscAdvancedSettings **ppAdvSettings);
        
        /* [hidden][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Debugger )( 
            IMsRdpClient4 * This,
            /* [retval][out] */ IMsTscDebug **ppDebugger);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Connect )( 
            IMsRdpClient4 * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Disconnect )( 
            IMsRdpClient4 * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateVirtualChannels )( 
            IMsRdpClient4 * This,
            /* [in] */ BSTR newVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SendOnVirtualChannel )( 
            IMsRdpClient4 * This,
            /* [in] */ BSTR chanName,
            /* [in] */ BSTR ChanData);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ColorDepth )( 
            IMsRdpClient4 * This,
            /* [in] */ long pcolorDepth);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ColorDepth )( 
            IMsRdpClient4 * This,
            /* [retval][out] */ long *pcolorDepth);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AdvancedSettings2 )( 
            IMsRdpClient4 * This,
            /* [retval][out] */ IMsRdpClientAdvancedSettings **ppAdvSettings);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SecuredSettings2 )( 
            IMsRdpClient4 * This,
            /* [retval][out] */ IMsRdpClientSecuredSettings **ppSecuredSettings);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ExtendedDisconnectReason )( 
            IMsRdpClient4 * This,
            /* [retval][out] */ ExtendedDisconnectReasonCode *pExtendedDisconnectReason);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_FullScreen )( 
            IMsRdpClient4 * This,
            /* [in] */ VARIANT_BOOL pfFullScreen);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_FullScreen )( 
            IMsRdpClient4 * This,
            /* [retval][out] */ VARIANT_BOOL *pfFullScreen);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetVirtualChannelOptions )( 
            IMsRdpClient4 * This,
            /* [in] */ BSTR chanName,
            /* [in] */ long chanOptions);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetVirtualChannelOptions )( 
            IMsRdpClient4 * This,
            /* [in] */ BSTR chanName,
            /* [retval][out] */ long *pChanOptions);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RequestClose )( 
            IMsRdpClient4 * This,
            /* [retval][out] */ ControlCloseStatus *pCloseStatus);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AdvancedSettings3 )( 
            IMsRdpClient4 * This,
            /* [retval][out] */ IMsRdpClientAdvancedSettings2 **ppAdvSettings);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ConnectedStatusText )( 
            IMsRdpClient4 * This,
            /* [in] */ BSTR pConnectedStatusText);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ConnectedStatusText )( 
            IMsRdpClient4 * This,
            /* [retval][out] */ BSTR *pConnectedStatusText);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AdvancedSettings4 )( 
            IMsRdpClient4 * This,
            /* [retval][out] */ IMsRdpClientAdvancedSettings3 **ppAdvSettings);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AdvancedSettings5 )( 
            IMsRdpClient4 * This,
            /* [retval][out] */ IMsRdpClientAdvancedSettings4 **ppAdvSettings5);
        
        END_INTERFACE
    } IMsRdpClient4Vtbl;

    interface IMsRdpClient4
    {
        CONST_VTBL struct IMsRdpClient4Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMsRdpClient4_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMsRdpClient4_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMsRdpClient4_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMsRdpClient4_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IMsRdpClient4_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IMsRdpClient4_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IMsRdpClient4_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IMsRdpClient4_put_Server(This,pServer)	\
    ( (This)->lpVtbl -> put_Server(This,pServer) ) 

#define IMsRdpClient4_get_Server(This,pServer)	\
    ( (This)->lpVtbl -> get_Server(This,pServer) ) 

#define IMsRdpClient4_put_Domain(This,pDomain)	\
    ( (This)->lpVtbl -> put_Domain(This,pDomain) ) 

#define IMsRdpClient4_get_Domain(This,pDomain)	\
    ( (This)->lpVtbl -> get_Domain(This,pDomain) ) 

#define IMsRdpClient4_put_UserName(This,pUserName)	\
    ( (This)->lpVtbl -> put_UserName(This,pUserName) ) 

#define IMsRdpClient4_get_UserName(This,pUserName)	\
    ( (This)->lpVtbl -> get_UserName(This,pUserName) ) 

#define IMsRdpClient4_put_DisconnectedText(This,pDisconnectedText)	\
    ( (This)->lpVtbl -> put_DisconnectedText(This,pDisconnectedText) ) 

#define IMsRdpClient4_get_DisconnectedText(This,pDisconnectedText)	\
    ( (This)->lpVtbl -> get_DisconnectedText(This,pDisconnectedText) ) 

#define IMsRdpClient4_put_ConnectingText(This,pConnectingText)	\
    ( (This)->lpVtbl -> put_ConnectingText(This,pConnectingText) ) 

#define IMsRdpClient4_get_ConnectingText(This,pConnectingText)	\
    ( (This)->lpVtbl -> get_ConnectingText(This,pConnectingText) ) 

#define IMsRdpClient4_get_Connected(This,pIsConnected)	\
    ( (This)->lpVtbl -> get_Connected(This,pIsConnected) ) 

#define IMsRdpClient4_put_DesktopWidth(This,pVal)	\
    ( (This)->lpVtbl -> put_DesktopWidth(This,pVal) ) 

#define IMsRdpClient4_get_DesktopWidth(This,pVal)	\
    ( (This)->lpVtbl -> get_DesktopWidth(This,pVal) ) 

#define IMsRdpClient4_put_DesktopHeight(This,pVal)	\
    ( (This)->lpVtbl -> put_DesktopHeight(This,pVal) ) 

#define IMsRdpClient4_get_DesktopHeight(This,pVal)	\
    ( (This)->lpVtbl -> get_DesktopHeight(This,pVal) ) 

#define IMsRdpClient4_put_StartConnected(This,pfStartConnected)	\
    ( (This)->lpVtbl -> put_StartConnected(This,pfStartConnected) ) 

#define IMsRdpClient4_get_StartConnected(This,pfStartConnected)	\
    ( (This)->lpVtbl -> get_StartConnected(This,pfStartConnected) ) 

#define IMsRdpClient4_get_HorizontalScrollBarVisible(This,pfHScrollVisible)	\
    ( (This)->lpVtbl -> get_HorizontalScrollBarVisible(This,pfHScrollVisible) ) 

#define IMsRdpClient4_get_VerticalScrollBarVisible(This,pfVScrollVisible)	\
    ( (This)->lpVtbl -> get_VerticalScrollBarVisible(This,pfVScrollVisible) ) 

#define IMsRdpClient4_put_FullScreenTitle(This,rhs)	\
    ( (This)->lpVtbl -> put_FullScreenTitle(This,rhs) ) 

#define IMsRdpClient4_get_CipherStrength(This,pCipherStrength)	\
    ( (This)->lpVtbl -> get_CipherStrength(This,pCipherStrength) ) 

#define IMsRdpClient4_get_Version(This,pVersion)	\
    ( (This)->lpVtbl -> get_Version(This,pVersion) ) 

#define IMsRdpClient4_get_SecuredSettingsEnabled(This,pSecuredSettingsEnabled)	\
    ( (This)->lpVtbl -> get_SecuredSettingsEnabled(This,pSecuredSettingsEnabled) ) 

#define IMsRdpClient4_get_SecuredSettings(This,ppSecuredSettings)	\
    ( (This)->lpVtbl -> get_SecuredSettings(This,ppSecuredSettings) ) 

#define IMsRdpClient4_get_AdvancedSettings(This,ppAdvSettings)	\
    ( (This)->lpVtbl -> get_AdvancedSettings(This,ppAdvSettings) ) 

#define IMsRdpClient4_get_Debugger(This,ppDebugger)	\
    ( (This)->lpVtbl -> get_Debugger(This,ppDebugger) ) 

#define IMsRdpClient4_Connect(This)	\
    ( (This)->lpVtbl -> Connect(This) ) 

#define IMsRdpClient4_Disconnect(This)	\
    ( (This)->lpVtbl -> Disconnect(This) ) 

#define IMsRdpClient4_CreateVirtualChannels(This,newVal)	\
    ( (This)->lpVtbl -> CreateVirtualChannels(This,newVal) ) 

#define IMsRdpClient4_SendOnVirtualChannel(This,chanName,ChanData)	\
    ( (This)->lpVtbl -> SendOnVirtualChannel(This,chanName,ChanData) ) 


#define IMsRdpClient4_put_ColorDepth(This,pcolorDepth)	\
    ( (This)->lpVtbl -> put_ColorDepth(This,pcolorDepth) ) 

#define IMsRdpClient4_get_ColorDepth(This,pcolorDepth)	\
    ( (This)->lpVtbl -> get_ColorDepth(This,pcolorDepth) ) 

#define IMsRdpClient4_get_AdvancedSettings2(This,ppAdvSettings)	\
    ( (This)->lpVtbl -> get_AdvancedSettings2(This,ppAdvSettings) ) 

#define IMsRdpClient4_get_SecuredSettings2(This,ppSecuredSettings)	\
    ( (This)->lpVtbl -> get_SecuredSettings2(This,ppSecuredSettings) ) 

#define IMsRdpClient4_get_ExtendedDisconnectReason(This,pExtendedDisconnectReason)	\
    ( (This)->lpVtbl -> get_ExtendedDisconnectReason(This,pExtendedDisconnectReason) ) 

#define IMsRdpClient4_put_FullScreen(This,pfFullScreen)	\
    ( (This)->lpVtbl -> put_FullScreen(This,pfFullScreen) ) 

#define IMsRdpClient4_get_FullScreen(This,pfFullScreen)	\
    ( (This)->lpVtbl -> get_FullScreen(This,pfFullScreen) ) 

#define IMsRdpClient4_SetVirtualChannelOptions(This,chanName,chanOptions)	\
    ( (This)->lpVtbl -> SetVirtualChannelOptions(This,chanName,chanOptions) ) 

#define IMsRdpClient4_GetVirtualChannelOptions(This,chanName,pChanOptions)	\
    ( (This)->lpVtbl -> GetVirtualChannelOptions(This,chanName,pChanOptions) ) 

#define IMsRdpClient4_RequestClose(This,pCloseStatus)	\
    ( (This)->lpVtbl -> RequestClose(This,pCloseStatus) ) 


#define IMsRdpClient4_get_AdvancedSettings3(This,ppAdvSettings)	\
    ( (This)->lpVtbl -> get_AdvancedSettings3(This,ppAdvSettings) ) 

#define IMsRdpClient4_put_ConnectedStatusText(This,pConnectedStatusText)	\
    ( (This)->lpVtbl -> put_ConnectedStatusText(This,pConnectedStatusText) ) 

#define IMsRdpClient4_get_ConnectedStatusText(This,pConnectedStatusText)	\
    ( (This)->lpVtbl -> get_ConnectedStatusText(This,pConnectedStatusText) ) 


#define IMsRdpClient4_get_AdvancedSettings4(This,ppAdvSettings)	\
    ( (This)->lpVtbl -> get_AdvancedSettings4(This,ppAdvSettings) ) 


#define IMsRdpClient4_get_AdvancedSettings5(This,ppAdvSettings5)	\
    ( (This)->lpVtbl -> get_AdvancedSettings5(This,ppAdvSettings5) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMsRdpClient4_INTERFACE_DEFINED__ */


#ifndef __IMsTscNonScriptable_INTERFACE_DEFINED__
#define __IMsTscNonScriptable_INTERFACE_DEFINED__

/* interface IMsTscNonScriptable */
/* [object][uuid] */ 


EXTERN_C const IID IID_IMsTscNonScriptable;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C1E6743A-41C1-4A74-832A-0DD06C1C7A0E")
    IMsTscNonScriptable : public IUnknown
    {
    public:
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ClearTextPassword( 
            /* [in] */ BSTR rhs) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_PortablePassword( 
            /* [in] */ BSTR pPortablePass) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PortablePassword( 
            /* [retval][out] */ BSTR *pPortablePass) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_PortableSalt( 
            /* [in] */ BSTR pPortableSalt) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PortableSalt( 
            /* [retval][out] */ BSTR *pPortableSalt) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_BinaryPassword( 
            /* [in] */ BSTR pBinaryPassword) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_BinaryPassword( 
            /* [retval][out] */ BSTR *pBinaryPassword) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_BinarySalt( 
            /* [in] */ BSTR pSalt) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_BinarySalt( 
            /* [retval][out] */ BSTR *pSalt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResetPassword( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMsTscNonScriptableVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMsTscNonScriptable * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMsTscNonScriptable * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMsTscNonScriptable * This);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ClearTextPassword )( 
            IMsTscNonScriptable * This,
            /* [in] */ BSTR rhs);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_PortablePassword )( 
            IMsTscNonScriptable * This,
            /* [in] */ BSTR pPortablePass);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PortablePassword )( 
            IMsTscNonScriptable * This,
            /* [retval][out] */ BSTR *pPortablePass);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_PortableSalt )( 
            IMsTscNonScriptable * This,
            /* [in] */ BSTR pPortableSalt);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PortableSalt )( 
            IMsTscNonScriptable * This,
            /* [retval][out] */ BSTR *pPortableSalt);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_BinaryPassword )( 
            IMsTscNonScriptable * This,
            /* [in] */ BSTR pBinaryPassword);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_BinaryPassword )( 
            IMsTscNonScriptable * This,
            /* [retval][out] */ BSTR *pBinaryPassword);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_BinarySalt )( 
            IMsTscNonScriptable * This,
            /* [in] */ BSTR pSalt);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_BinarySalt )( 
            IMsTscNonScriptable * This,
            /* [retval][out] */ BSTR *pSalt);
        
        HRESULT ( STDMETHODCALLTYPE *ResetPassword )( 
            IMsTscNonScriptable * This);
        
        END_INTERFACE
    } IMsTscNonScriptableVtbl;

    interface IMsTscNonScriptable
    {
        CONST_VTBL struct IMsTscNonScriptableVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMsTscNonScriptable_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMsTscNonScriptable_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMsTscNonScriptable_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMsTscNonScriptable_put_ClearTextPassword(This,rhs)	\
    ( (This)->lpVtbl -> put_ClearTextPassword(This,rhs) ) 

#define IMsTscNonScriptable_put_PortablePassword(This,pPortablePass)	\
    ( (This)->lpVtbl -> put_PortablePassword(This,pPortablePass) ) 

#define IMsTscNonScriptable_get_PortablePassword(This,pPortablePass)	\
    ( (This)->lpVtbl -> get_PortablePassword(This,pPortablePass) ) 

#define IMsTscNonScriptable_put_PortableSalt(This,pPortableSalt)	\
    ( (This)->lpVtbl -> put_PortableSalt(This,pPortableSalt) ) 

#define IMsTscNonScriptable_get_PortableSalt(This,pPortableSalt)	\
    ( (This)->lpVtbl -> get_PortableSalt(This,pPortableSalt) ) 

#define IMsTscNonScriptable_put_BinaryPassword(This,pBinaryPassword)	\
    ( (This)->lpVtbl -> put_BinaryPassword(This,pBinaryPassword) ) 

#define IMsTscNonScriptable_get_BinaryPassword(This,pBinaryPassword)	\
    ( (This)->lpVtbl -> get_BinaryPassword(This,pBinaryPassword) ) 

#define IMsTscNonScriptable_put_BinarySalt(This,pSalt)	\
    ( (This)->lpVtbl -> put_BinarySalt(This,pSalt) ) 

#define IMsTscNonScriptable_get_BinarySalt(This,pSalt)	\
    ( (This)->lpVtbl -> get_BinarySalt(This,pSalt) ) 

#define IMsTscNonScriptable_ResetPassword(This)	\
    ( (This)->lpVtbl -> ResetPassword(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMsTscNonScriptable_INTERFACE_DEFINED__ */


#ifndef __IMsRdpClientNonScriptable_INTERFACE_DEFINED__
#define __IMsRdpClientNonScriptable_INTERFACE_DEFINED__

/* interface IMsRdpClientNonScriptable */
/* [object][uuid] */ 


EXTERN_C const IID IID_IMsRdpClientNonScriptable;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2F079C4C-87B2-4AFD-97AB-20CDB43038AE")
    IMsRdpClientNonScriptable : public IMsTscNonScriptable
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE NotifyRedirectDeviceChange( 
            /* [in] */ UINT_PTR wParam,
            /* [in] */ LONG_PTR lParam) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SendKeys( 
            /* [in] */ long numKeys,
            /* [in] */ VARIANT_BOOL *pbArrayKeyUp,
            /* [in] */ long *plKeyData) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMsRdpClientNonScriptableVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMsRdpClientNonScriptable * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMsRdpClientNonScriptable * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMsRdpClientNonScriptable * This);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ClearTextPassword )( 
            IMsRdpClientNonScriptable * This,
            /* [in] */ BSTR rhs);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_PortablePassword )( 
            IMsRdpClientNonScriptable * This,
            /* [in] */ BSTR pPortablePass);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PortablePassword )( 
            IMsRdpClientNonScriptable * This,
            /* [retval][out] */ BSTR *pPortablePass);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_PortableSalt )( 
            IMsRdpClientNonScriptable * This,
            /* [in] */ BSTR pPortableSalt);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PortableSalt )( 
            IMsRdpClientNonScriptable * This,
            /* [retval][out] */ BSTR *pPortableSalt);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_BinaryPassword )( 
            IMsRdpClientNonScriptable * This,
            /* [in] */ BSTR pBinaryPassword);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_BinaryPassword )( 
            IMsRdpClientNonScriptable * This,
            /* [retval][out] */ BSTR *pBinaryPassword);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_BinarySalt )( 
            IMsRdpClientNonScriptable * This,
            /* [in] */ BSTR pSalt);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_BinarySalt )( 
            IMsRdpClientNonScriptable * This,
            /* [retval][out] */ BSTR *pSalt);
        
        HRESULT ( STDMETHODCALLTYPE *ResetPassword )( 
            IMsRdpClientNonScriptable * This);
        
        HRESULT ( STDMETHODCALLTYPE *NotifyRedirectDeviceChange )( 
            IMsRdpClientNonScriptable * This,
            /* [in] */ UINT_PTR wParam,
            /* [in] */ LONG_PTR lParam);
        
        HRESULT ( STDMETHODCALLTYPE *SendKeys )( 
            IMsRdpClientNonScriptable * This,
            /* [in] */ long numKeys,
            /* [in] */ VARIANT_BOOL *pbArrayKeyUp,
            /* [in] */ long *plKeyData);
        
        END_INTERFACE
    } IMsRdpClientNonScriptableVtbl;

    interface IMsRdpClientNonScriptable
    {
        CONST_VTBL struct IMsRdpClientNonScriptableVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMsRdpClientNonScriptable_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMsRdpClientNonScriptable_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMsRdpClientNonScriptable_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMsRdpClientNonScriptable_put_ClearTextPassword(This,rhs)	\
    ( (This)->lpVtbl -> put_ClearTextPassword(This,rhs) ) 

#define IMsRdpClientNonScriptable_put_PortablePassword(This,pPortablePass)	\
    ( (This)->lpVtbl -> put_PortablePassword(This,pPortablePass) ) 

#define IMsRdpClientNonScriptable_get_PortablePassword(This,pPortablePass)	\
    ( (This)->lpVtbl -> get_PortablePassword(This,pPortablePass) ) 

#define IMsRdpClientNonScriptable_put_PortableSalt(This,pPortableSalt)	\
    ( (This)->lpVtbl -> put_PortableSalt(This,pPortableSalt) ) 

#define IMsRdpClientNonScriptable_get_PortableSalt(This,pPortableSalt)	\
    ( (This)->lpVtbl -> get_PortableSalt(This,pPortableSalt) ) 

#define IMsRdpClientNonScriptable_put_BinaryPassword(This,pBinaryPassword)	\
    ( (This)->lpVtbl -> put_BinaryPassword(This,pBinaryPassword) ) 

#define IMsRdpClientNonScriptable_get_BinaryPassword(This,pBinaryPassword)	\
    ( (This)->lpVtbl -> get_BinaryPassword(This,pBinaryPassword) ) 

#define IMsRdpClientNonScriptable_put_BinarySalt(This,pSalt)	\
    ( (This)->lpVtbl -> put_BinarySalt(This,pSalt) ) 

#define IMsRdpClientNonScriptable_get_BinarySalt(This,pSalt)	\
    ( (This)->lpVtbl -> get_BinarySalt(This,pSalt) ) 

#define IMsRdpClientNonScriptable_ResetPassword(This)	\
    ( (This)->lpVtbl -> ResetPassword(This) ) 


#define IMsRdpClientNonScriptable_NotifyRedirectDeviceChange(This,wParam,lParam)	\
    ( (This)->lpVtbl -> NotifyRedirectDeviceChange(This,wParam,lParam) ) 

#define IMsRdpClientNonScriptable_SendKeys(This,numKeys,pbArrayKeyUp,plKeyData)	\
    ( (This)->lpVtbl -> SendKeys(This,numKeys,pbArrayKeyUp,plKeyData) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMsRdpClientNonScriptable_INTERFACE_DEFINED__ */


#ifndef __IMsRdpClientNonScriptable2_INTERFACE_DEFINED__
#define __IMsRdpClientNonScriptable2_INTERFACE_DEFINED__

/* interface IMsRdpClientNonScriptable2 */
/* [object][uuid] */ 


EXTERN_C const IID IID_IMsRdpClientNonScriptable2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("17A5E535-4072-4FA4-AF32-C8D0D47345E9")
    IMsRdpClientNonScriptable2 : public IMsRdpClientNonScriptable
    {
    public:
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_UIParentWindowHandle( 
            /* [in] */ HWND phwndUIParentWindowHandle) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_UIParentWindowHandle( 
            /* [retval][out] */ HWND *phwndUIParentWindowHandle) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMsRdpClientNonScriptable2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMsRdpClientNonScriptable2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMsRdpClientNonScriptable2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMsRdpClientNonScriptable2 * This);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ClearTextPassword )( 
            IMsRdpClientNonScriptable2 * This,
            /* [in] */ BSTR rhs);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_PortablePassword )( 
            IMsRdpClientNonScriptable2 * This,
            /* [in] */ BSTR pPortablePass);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PortablePassword )( 
            IMsRdpClientNonScriptable2 * This,
            /* [retval][out] */ BSTR *pPortablePass);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_PortableSalt )( 
            IMsRdpClientNonScriptable2 * This,
            /* [in] */ BSTR pPortableSalt);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PortableSalt )( 
            IMsRdpClientNonScriptable2 * This,
            /* [retval][out] */ BSTR *pPortableSalt);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_BinaryPassword )( 
            IMsRdpClientNonScriptable2 * This,
            /* [in] */ BSTR pBinaryPassword);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_BinaryPassword )( 
            IMsRdpClientNonScriptable2 * This,
            /* [retval][out] */ BSTR *pBinaryPassword);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_BinarySalt )( 
            IMsRdpClientNonScriptable2 * This,
            /* [in] */ BSTR pSalt);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_BinarySalt )( 
            IMsRdpClientNonScriptable2 * This,
            /* [retval][out] */ BSTR *pSalt);
        
        HRESULT ( STDMETHODCALLTYPE *ResetPassword )( 
            IMsRdpClientNonScriptable2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *NotifyRedirectDeviceChange )( 
            IMsRdpClientNonScriptable2 * This,
            /* [in] */ UINT_PTR wParam,
            /* [in] */ LONG_PTR lParam);
        
        HRESULT ( STDMETHODCALLTYPE *SendKeys )( 
            IMsRdpClientNonScriptable2 * This,
            /* [in] */ long numKeys,
            /* [in] */ VARIANT_BOOL *pbArrayKeyUp,
            /* [in] */ long *plKeyData);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_UIParentWindowHandle )( 
            IMsRdpClientNonScriptable2 * This,
            /* [in] */ HWND phwndUIParentWindowHandle);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_UIParentWindowHandle )( 
            IMsRdpClientNonScriptable2 * This,
            /* [retval][out] */ HWND *phwndUIParentWindowHandle);
        
        END_INTERFACE
    } IMsRdpClientNonScriptable2Vtbl;

    interface IMsRdpClientNonScriptable2
    {
        CONST_VTBL struct IMsRdpClientNonScriptable2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMsRdpClientNonScriptable2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMsRdpClientNonScriptable2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMsRdpClientNonScriptable2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMsRdpClientNonScriptable2_put_ClearTextPassword(This,rhs)	\
    ( (This)->lpVtbl -> put_ClearTextPassword(This,rhs) ) 

#define IMsRdpClientNonScriptable2_put_PortablePassword(This,pPortablePass)	\
    ( (This)->lpVtbl -> put_PortablePassword(This,pPortablePass) ) 

#define IMsRdpClientNonScriptable2_get_PortablePassword(This,pPortablePass)	\
    ( (This)->lpVtbl -> get_PortablePassword(This,pPortablePass) ) 

#define IMsRdpClientNonScriptable2_put_PortableSalt(This,pPortableSalt)	\
    ( (This)->lpVtbl -> put_PortableSalt(This,pPortableSalt) ) 

#define IMsRdpClientNonScriptable2_get_PortableSalt(This,pPortableSalt)	\
    ( (This)->lpVtbl -> get_PortableSalt(This,pPortableSalt) ) 

#define IMsRdpClientNonScriptable2_put_BinaryPassword(This,pBinaryPassword)	\
    ( (This)->lpVtbl -> put_BinaryPassword(This,pBinaryPassword) ) 

#define IMsRdpClientNonScriptable2_get_BinaryPassword(This,pBinaryPassword)	\
    ( (This)->lpVtbl -> get_BinaryPassword(This,pBinaryPassword) ) 

#define IMsRdpClientNonScriptable2_put_BinarySalt(This,pSalt)	\
    ( (This)->lpVtbl -> put_BinarySalt(This,pSalt) ) 

#define IMsRdpClientNonScriptable2_get_BinarySalt(This,pSalt)	\
    ( (This)->lpVtbl -> get_BinarySalt(This,pSalt) ) 

#define IMsRdpClientNonScriptable2_ResetPassword(This)	\
    ( (This)->lpVtbl -> ResetPassword(This) ) 


#define IMsRdpClientNonScriptable2_NotifyRedirectDeviceChange(This,wParam,lParam)	\
    ( (This)->lpVtbl -> NotifyRedirectDeviceChange(This,wParam,lParam) ) 

#define IMsRdpClientNonScriptable2_SendKeys(This,numKeys,pbArrayKeyUp,plKeyData)	\
    ( (This)->lpVtbl -> SendKeys(This,numKeys,pbArrayKeyUp,plKeyData) ) 


#define IMsRdpClientNonScriptable2_put_UIParentWindowHandle(This,phwndUIParentWindowHandle)	\
    ( (This)->lpVtbl -> put_UIParentWindowHandle(This,phwndUIParentWindowHandle) ) 

#define IMsRdpClientNonScriptable2_get_UIParentWindowHandle(This,phwndUIParentWindowHandle)	\
    ( (This)->lpVtbl -> get_UIParentWindowHandle(This,phwndUIParentWindowHandle) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMsRdpClientNonScriptable2_INTERFACE_DEFINED__ */


#ifndef __IMsTscAdvancedSettings_INTERFACE_DEFINED__
#define __IMsTscAdvancedSettings_INTERFACE_DEFINED__

/* interface IMsTscAdvancedSettings */
/* [object][oleautomation][dual][uuid] */ 


EXTERN_C const IID IID_IMsTscAdvancedSettings;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("809945CC-4B3B-4A92-A6B0-DBF9B5F2EF2D")
    IMsTscAdvancedSettings : public IDispatch
    {
    public:
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Compress( 
            /* [in] */ long pcompress) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Compress( 
            /* [retval][out] */ long *pcompress) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_BitmapPeristence( 
            /* [in] */ long pbitmapPeristence) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_BitmapPeristence( 
            /* [retval][out] */ long *pbitmapPeristence) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_allowBackgroundInput( 
            /* [in] */ long pallowBackgroundInput) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_allowBackgroundInput( 
            /* [retval][out] */ long *pallowBackgroundInput) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_KeyBoardLayoutStr( 
            /* [in] */ BSTR rhs) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_PluginDlls( 
            /* [in] */ BSTR rhs) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_IconFile( 
            /* [in] */ BSTR rhs) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_IconIndex( 
            /* [in] */ long rhs) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_ContainerHandledFullScreen( 
            /* [in] */ long pContainerHandledFullScreen) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_ContainerHandledFullScreen( 
            /* [retval][out] */ long *pContainerHandledFullScreen) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_DisableRdpdr( 
            /* [in] */ long pDisableRdpdr) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_DisableRdpdr( 
            /* [retval][out] */ long *pDisableRdpdr) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMsTscAdvancedSettingsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMsTscAdvancedSettings * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMsTscAdvancedSettings * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMsTscAdvancedSettings * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IMsTscAdvancedSettings * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IMsTscAdvancedSettings * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IMsTscAdvancedSettings * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IMsTscAdvancedSettings * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Compress )( 
            IMsTscAdvancedSettings * This,
            /* [in] */ long pcompress);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Compress )( 
            IMsTscAdvancedSettings * This,
            /* [retval][out] */ long *pcompress);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_BitmapPeristence )( 
            IMsTscAdvancedSettings * This,
            /* [in] */ long pbitmapPeristence);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BitmapPeristence )( 
            IMsTscAdvancedSettings * This,
            /* [retval][out] */ long *pbitmapPeristence);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_allowBackgroundInput )( 
            IMsTscAdvancedSettings * This,
            /* [in] */ long pallowBackgroundInput);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_allowBackgroundInput )( 
            IMsTscAdvancedSettings * This,
            /* [retval][out] */ long *pallowBackgroundInput);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_KeyBoardLayoutStr )( 
            IMsTscAdvancedSettings * This,
            /* [in] */ BSTR rhs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_PluginDlls )( 
            IMsTscAdvancedSettings * This,
            /* [in] */ BSTR rhs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_IconFile )( 
            IMsTscAdvancedSettings * This,
            /* [in] */ BSTR rhs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_IconIndex )( 
            IMsTscAdvancedSettings * This,
            /* [in] */ long rhs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ContainerHandledFullScreen )( 
            IMsTscAdvancedSettings * This,
            /* [in] */ long pContainerHandledFullScreen);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ContainerHandledFullScreen )( 
            IMsTscAdvancedSettings * This,
            /* [retval][out] */ long *pContainerHandledFullScreen);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DisableRdpdr )( 
            IMsTscAdvancedSettings * This,
            /* [in] */ long pDisableRdpdr);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DisableRdpdr )( 
            IMsTscAdvancedSettings * This,
            /* [retval][out] */ long *pDisableRdpdr);
        
        END_INTERFACE
    } IMsTscAdvancedSettingsVtbl;

    interface IMsTscAdvancedSettings
    {
        CONST_VTBL struct IMsTscAdvancedSettingsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMsTscAdvancedSettings_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMsTscAdvancedSettings_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMsTscAdvancedSettings_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMsTscAdvancedSettings_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IMsTscAdvancedSettings_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IMsTscAdvancedSettings_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IMsTscAdvancedSettings_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IMsTscAdvancedSettings_put_Compress(This,pcompress)	\
    ( (This)->lpVtbl -> put_Compress(This,pcompress) ) 

#define IMsTscAdvancedSettings_get_Compress(This,pcompress)	\
    ( (This)->lpVtbl -> get_Compress(This,pcompress) ) 

#define IMsTscAdvancedSettings_put_BitmapPeristence(This,pbitmapPeristence)	\
    ( (This)->lpVtbl -> put_BitmapPeristence(This,pbitmapPeristence) ) 

#define IMsTscAdvancedSettings_get_BitmapPeristence(This,pbitmapPeristence)	\
    ( (This)->lpVtbl -> get_BitmapPeristence(This,pbitmapPeristence) ) 

#define IMsTscAdvancedSettings_put_allowBackgroundInput(This,pallowBackgroundInput)	\
    ( (This)->lpVtbl -> put_allowBackgroundInput(This,pallowBackgroundInput) ) 

#define IMsTscAdvancedSettings_get_allowBackgroundInput(This,pallowBackgroundInput)	\
    ( (This)->lpVtbl -> get_allowBackgroundInput(This,pallowBackgroundInput) ) 

#define IMsTscAdvancedSettings_put_KeyBoardLayoutStr(This,rhs)	\
    ( (This)->lpVtbl -> put_KeyBoardLayoutStr(This,rhs) ) 

#define IMsTscAdvancedSettings_put_PluginDlls(This,rhs)	\
    ( (This)->lpVtbl -> put_PluginDlls(This,rhs) ) 

#define IMsTscAdvancedSettings_put_IconFile(This,rhs)	\
    ( (This)->lpVtbl -> put_IconFile(This,rhs) ) 

#define IMsTscAdvancedSettings_put_IconIndex(This,rhs)	\
    ( (This)->lpVtbl -> put_IconIndex(This,rhs) ) 

#define IMsTscAdvancedSettings_put_ContainerHandledFullScreen(This,pContainerHandledFullScreen)	\
    ( (This)->lpVtbl -> put_ContainerHandledFullScreen(This,pContainerHandledFullScreen) ) 

#define IMsTscAdvancedSettings_get_ContainerHandledFullScreen(This,pContainerHandledFullScreen)	\
    ( (This)->lpVtbl -> get_ContainerHandledFullScreen(This,pContainerHandledFullScreen) ) 

#define IMsTscAdvancedSettings_put_DisableRdpdr(This,pDisableRdpdr)	\
    ( (This)->lpVtbl -> put_DisableRdpdr(This,pDisableRdpdr) ) 

#define IMsTscAdvancedSettings_get_DisableRdpdr(This,pDisableRdpdr)	\
    ( (This)->lpVtbl -> get_DisableRdpdr(This,pDisableRdpdr) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMsTscAdvancedSettings_INTERFACE_DEFINED__ */


#ifndef __IMsRdpClientAdvancedSettings_INTERFACE_DEFINED__
#define __IMsRdpClientAdvancedSettings_INTERFACE_DEFINED__

/* interface IMsRdpClientAdvancedSettings */
/* [object][oleautomation][dual][uuid] */ 


EXTERN_C const IID IID_IMsRdpClientAdvancedSettings;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3C65B4AB-12B3-465B-ACD4-B8DAD3BFF9E2")
    IMsRdpClientAdvancedSettings : public IMsTscAdvancedSettings
    {
    public:
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_SmoothScroll( 
            /* [in] */ long psmoothScroll) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_SmoothScroll( 
            /* [retval][out] */ long *psmoothScroll) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_AcceleratorPassthrough( 
            /* [in] */ long pacceleratorPassthrough) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_AcceleratorPassthrough( 
            /* [retval][out] */ long *pacceleratorPassthrough) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_ShadowBitmap( 
            /* [in] */ long pshadowBitmap) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_ShadowBitmap( 
            /* [retval][out] */ long *pshadowBitmap) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_TransportType( 
            /* [in] */ long ptransportType) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_TransportType( 
            /* [retval][out] */ long *ptransportType) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_SasSequence( 
            /* [in] */ long psasSequence) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_SasSequence( 
            /* [retval][out] */ long *psasSequence) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_EncryptionEnabled( 
            /* [in] */ long pencryptionEnabled) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_EncryptionEnabled( 
            /* [retval][out] */ long *pencryptionEnabled) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_DedicatedTerminal( 
            /* [in] */ long pdedicatedTerminal) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_DedicatedTerminal( 
            /* [retval][out] */ long *pdedicatedTerminal) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_RDPPort( 
            /* [in] */ long prdpPort) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_RDPPort( 
            /* [retval][out] */ long *prdpPort) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_EnableMouse( 
            /* [in] */ long penableMouse) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_EnableMouse( 
            /* [retval][out] */ long *penableMouse) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_DisableCtrlAltDel( 
            /* [in] */ long pdisableCtrlAltDel) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_DisableCtrlAltDel( 
            /* [retval][out] */ long *pdisableCtrlAltDel) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_EnableWindowsKey( 
            /* [in] */ long penableWindowsKey) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_EnableWindowsKey( 
            /* [retval][out] */ long *penableWindowsKey) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_DoubleClickDetect( 
            /* [in] */ long pdoubleClickDetect) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_DoubleClickDetect( 
            /* [retval][out] */ long *pdoubleClickDetect) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_MaximizeShell( 
            /* [in] */ long pmaximizeShell) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_MaximizeShell( 
            /* [retval][out] */ long *pmaximizeShell) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_HotKeyFullScreen( 
            /* [in] */ long photKeyFullScreen) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_HotKeyFullScreen( 
            /* [retval][out] */ long *photKeyFullScreen) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_HotKeyCtrlEsc( 
            /* [in] */ long photKeyCtrlEsc) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_HotKeyCtrlEsc( 
            /* [retval][out] */ long *photKeyCtrlEsc) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_HotKeyAltEsc( 
            /* [in] */ long photKeyAltEsc) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_HotKeyAltEsc( 
            /* [retval][out] */ long *photKeyAltEsc) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_HotKeyAltTab( 
            /* [in] */ long photKeyAltTab) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_HotKeyAltTab( 
            /* [retval][out] */ long *photKeyAltTab) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_HotKeyAltShiftTab( 
            /* [in] */ long photKeyAltShiftTab) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_HotKeyAltShiftTab( 
            /* [retval][out] */ long *photKeyAltShiftTab) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_HotKeyAltSpace( 
            /* [in] */ long photKeyAltSpace) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_HotKeyAltSpace( 
            /* [retval][out] */ long *photKeyAltSpace) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_HotKeyCtrlAltDel( 
            /* [in] */ long photKeyCtrlAltDel) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_HotKeyCtrlAltDel( 
            /* [retval][out] */ long *photKeyCtrlAltDel) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_orderDrawThreshold( 
            /* [in] */ long porderDrawThreshold) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_orderDrawThreshold( 
            /* [retval][out] */ long *porderDrawThreshold) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_BitmapCacheSize( 
            /* [in] */ long pbitmapCacheSize) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_BitmapCacheSize( 
            /* [retval][out] */ long *pbitmapCacheSize) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_BitmapVirtualCacheSize( 
            /* [in] */ long pbitmapVirtualCacheSize) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_BitmapVirtualCacheSize( 
            /* [retval][out] */ long *pbitmapVirtualCacheSize) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_ScaleBitmapCachesByBPP( 
            /* [in] */ long pbScale) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_ScaleBitmapCachesByBPP( 
            /* [retval][out] */ long *pbScale) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_NumBitmapCaches( 
            /* [in] */ long pnumBitmapCaches) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_NumBitmapCaches( 
            /* [retval][out] */ long *pnumBitmapCaches) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_CachePersistenceActive( 
            /* [in] */ long pcachePersistenceActive) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_CachePersistenceActive( 
            /* [retval][out] */ long *pcachePersistenceActive) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_PersistCacheDirectory( 
            /* [in] */ BSTR rhs) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_brushSupportLevel( 
            /* [in] */ long pbrushSupportLevel) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_brushSupportLevel( 
            /* [retval][out] */ long *pbrushSupportLevel) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_minInputSendInterval( 
            /* [in] */ long pminInputSendInterval) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_minInputSendInterval( 
            /* [retval][out] */ long *pminInputSendInterval) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_InputEventsAtOnce( 
            /* [in] */ long pinputEventsAtOnce) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_InputEventsAtOnce( 
            /* [retval][out] */ long *pinputEventsAtOnce) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_maxEventCount( 
            /* [in] */ long pmaxEventCount) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_maxEventCount( 
            /* [retval][out] */ long *pmaxEventCount) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_keepAliveInterval( 
            /* [in] */ long pkeepAliveInterval) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_keepAliveInterval( 
            /* [retval][out] */ long *pkeepAliveInterval) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_shutdownTimeout( 
            /* [in] */ long pshutdownTimeout) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_shutdownTimeout( 
            /* [retval][out] */ long *pshutdownTimeout) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_overallConnectionTimeout( 
            /* [in] */ long poverallConnectionTimeout) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_overallConnectionTimeout( 
            /* [retval][out] */ long *poverallConnectionTimeout) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_singleConnectionTimeout( 
            /* [in] */ long psingleConnectionTimeout) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_singleConnectionTimeout( 
            /* [retval][out] */ long *psingleConnectionTimeout) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_KeyboardType( 
            /* [in] */ long pkeyboardType) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_KeyboardType( 
            /* [retval][out] */ long *pkeyboardType) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_KeyboardSubType( 
            /* [in] */ long pkeyboardSubType) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_KeyboardSubType( 
            /* [retval][out] */ long *pkeyboardSubType) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_KeyboardFunctionKey( 
            /* [in] */ long pkeyboardFunctionKey) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_KeyboardFunctionKey( 
            /* [retval][out] */ long *pkeyboardFunctionKey) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_WinceFixedPalette( 
            /* [in] */ long pwinceFixedPalette) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_WinceFixedPalette( 
            /* [retval][out] */ long *pwinceFixedPalette) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_ConnectToServerConsole( 
            /* [in] */ VARIANT_BOOL pConnectToConsole) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_ConnectToServerConsole( 
            /* [retval][out] */ VARIANT_BOOL *pConnectToConsole) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_BitmapPersistence( 
            /* [in] */ long pbitmapPersistence) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_BitmapPersistence( 
            /* [retval][out] */ long *pbitmapPersistence) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_MinutesToIdleTimeout( 
            /* [in] */ long pminutesToIdleTimeout) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_MinutesToIdleTimeout( 
            /* [retval][out] */ long *pminutesToIdleTimeout) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_SmartSizing( 
            /* [in] */ VARIANT_BOOL pfSmartSizing) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_SmartSizing( 
            /* [retval][out] */ VARIANT_BOOL *pfSmartSizing) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_RdpdrLocalPrintingDocName( 
            /* [in] */ BSTR pLocalPrintingDocName) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_RdpdrLocalPrintingDocName( 
            /* [retval][out] */ BSTR *pLocalPrintingDocName) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_RdpdrClipCleanTempDirString( 
            /* [in] */ BSTR clipCleanTempDirString) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_RdpdrClipCleanTempDirString( 
            /* [retval][out] */ BSTR *clipCleanTempDirString) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_RdpdrClipPasteInfoString( 
            /* [in] */ BSTR clipPasteInfoString) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_RdpdrClipPasteInfoString( 
            /* [retval][out] */ BSTR *clipPasteInfoString) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_ClearTextPassword( 
            /* [in] */ BSTR rhs) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_DisplayConnectionBar( 
            /* [in] */ VARIANT_BOOL pDisplayConnectionBar) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_DisplayConnectionBar( 
            /* [retval][out] */ VARIANT_BOOL *pDisplayConnectionBar) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_PinConnectionBar( 
            /* [in] */ VARIANT_BOOL pPinConnectionBar) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_PinConnectionBar( 
            /* [retval][out] */ VARIANT_BOOL *pPinConnectionBar) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_GrabFocusOnConnect( 
            /* [in] */ VARIANT_BOOL pfGrabFocusOnConnect) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_GrabFocusOnConnect( 
            /* [retval][out] */ VARIANT_BOOL *pfGrabFocusOnConnect) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_LoadBalanceInfo( 
            /* [in] */ BSTR pLBInfo) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_LoadBalanceInfo( 
            /* [retval][out] */ BSTR *pLBInfo) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_RedirectDrives( 
            /* [in] */ VARIANT_BOOL pRedirectDrives) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_RedirectDrives( 
            /* [retval][out] */ VARIANT_BOOL *pRedirectDrives) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_RedirectPrinters( 
            /* [in] */ VARIANT_BOOL pRedirectPrinters) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_RedirectPrinters( 
            /* [retval][out] */ VARIANT_BOOL *pRedirectPrinters) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_RedirectPorts( 
            /* [in] */ VARIANT_BOOL pRedirectPorts) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_RedirectPorts( 
            /* [retval][out] */ VARIANT_BOOL *pRedirectPorts) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_RedirectSmartCards( 
            /* [in] */ VARIANT_BOOL pRedirectSmartCards) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_RedirectSmartCards( 
            /* [retval][out] */ VARIANT_BOOL *pRedirectSmartCards) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_BitmapVirtualCache16BppSize( 
            /* [in] */ long pBitmapVirtualCache16BppSize) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_BitmapVirtualCache16BppSize( 
            /* [retval][out] */ long *pBitmapVirtualCache16BppSize) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_BitmapVirtualCache24BppSize( 
            /* [in] */ long pBitmapVirtualCache24BppSize) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_BitmapVirtualCache24BppSize( 
            /* [retval][out] */ long *pBitmapVirtualCache24BppSize) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_PerformanceFlags( 
            /* [in] */ long pDisableList) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_PerformanceFlags( 
            /* [retval][out] */ long *pDisableList) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_ConnectWithEndpoint( 
            /* [in] */ VARIANT *rhs) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_NotifyTSPublicKey( 
            /* [in] */ VARIANT_BOOL pfNotify) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_NotifyTSPublicKey( 
            /* [retval][out] */ VARIANT_BOOL *pfNotify) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMsRdpClientAdvancedSettingsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMsRdpClientAdvancedSettings * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMsRdpClientAdvancedSettings * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IMsRdpClientAdvancedSettings * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Compress )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long pcompress);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Compress )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *pcompress);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_BitmapPeristence )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long pbitmapPeristence);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BitmapPeristence )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *pbitmapPeristence);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_allowBackgroundInput )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long pallowBackgroundInput);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_allowBackgroundInput )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *pallowBackgroundInput);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_KeyBoardLayoutStr )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ BSTR rhs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_PluginDlls )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ BSTR rhs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_IconFile )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ BSTR rhs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_IconIndex )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long rhs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ContainerHandledFullScreen )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long pContainerHandledFullScreen);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ContainerHandledFullScreen )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *pContainerHandledFullScreen);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DisableRdpdr )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long pDisableRdpdr);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DisableRdpdr )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *pDisableRdpdr);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_SmoothScroll )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long psmoothScroll);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SmoothScroll )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *psmoothScroll);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_AcceleratorPassthrough )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long pacceleratorPassthrough);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AcceleratorPassthrough )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *pacceleratorPassthrough);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ShadowBitmap )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long pshadowBitmap);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ShadowBitmap )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *pshadowBitmap);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_TransportType )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long ptransportType);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_TransportType )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *ptransportType);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_SasSequence )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long psasSequence);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SasSequence )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *psasSequence);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_EncryptionEnabled )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long pencryptionEnabled);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_EncryptionEnabled )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *pencryptionEnabled);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DedicatedTerminal )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long pdedicatedTerminal);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DedicatedTerminal )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *pdedicatedTerminal);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RDPPort )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long prdpPort);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RDPPort )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *prdpPort);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_EnableMouse )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long penableMouse);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_EnableMouse )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *penableMouse);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DisableCtrlAltDel )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long pdisableCtrlAltDel);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DisableCtrlAltDel )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *pdisableCtrlAltDel);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_EnableWindowsKey )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long penableWindowsKey);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_EnableWindowsKey )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *penableWindowsKey);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DoubleClickDetect )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long pdoubleClickDetect);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DoubleClickDetect )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *pdoubleClickDetect);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_MaximizeShell )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long pmaximizeShell);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_MaximizeShell )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *pmaximizeShell);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_HotKeyFullScreen )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long photKeyFullScreen);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HotKeyFullScreen )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *photKeyFullScreen);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_HotKeyCtrlEsc )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long photKeyCtrlEsc);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HotKeyCtrlEsc )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *photKeyCtrlEsc);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_HotKeyAltEsc )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long photKeyAltEsc);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HotKeyAltEsc )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *photKeyAltEsc);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_HotKeyAltTab )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long photKeyAltTab);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HotKeyAltTab )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *photKeyAltTab);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_HotKeyAltShiftTab )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long photKeyAltShiftTab);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HotKeyAltShiftTab )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *photKeyAltShiftTab);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_HotKeyAltSpace )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long photKeyAltSpace);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HotKeyAltSpace )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *photKeyAltSpace);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_HotKeyCtrlAltDel )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long photKeyCtrlAltDel);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HotKeyCtrlAltDel )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *photKeyCtrlAltDel);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_orderDrawThreshold )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long porderDrawThreshold);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_orderDrawThreshold )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *porderDrawThreshold);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_BitmapCacheSize )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long pbitmapCacheSize);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BitmapCacheSize )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *pbitmapCacheSize);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_BitmapVirtualCacheSize )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long pbitmapVirtualCacheSize);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BitmapVirtualCacheSize )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *pbitmapVirtualCacheSize);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ScaleBitmapCachesByBPP )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long pbScale);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ScaleBitmapCachesByBPP )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *pbScale);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_NumBitmapCaches )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long pnumBitmapCaches);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_NumBitmapCaches )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *pnumBitmapCaches);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_CachePersistenceActive )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long pcachePersistenceActive);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CachePersistenceActive )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *pcachePersistenceActive);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_PersistCacheDirectory )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ BSTR rhs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_brushSupportLevel )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long pbrushSupportLevel);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_brushSupportLevel )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *pbrushSupportLevel);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_minInputSendInterval )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long pminInputSendInterval);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_minInputSendInterval )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *pminInputSendInterval);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_InputEventsAtOnce )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long pinputEventsAtOnce);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_InputEventsAtOnce )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *pinputEventsAtOnce);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_maxEventCount )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long pmaxEventCount);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_maxEventCount )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *pmaxEventCount);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_keepAliveInterval )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long pkeepAliveInterval);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_keepAliveInterval )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *pkeepAliveInterval);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_shutdownTimeout )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long pshutdownTimeout);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_shutdownTimeout )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *pshutdownTimeout);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_overallConnectionTimeout )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long poverallConnectionTimeout);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_overallConnectionTimeout )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *poverallConnectionTimeout);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_singleConnectionTimeout )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long psingleConnectionTimeout);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_singleConnectionTimeout )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *psingleConnectionTimeout);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_KeyboardType )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long pkeyboardType);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_KeyboardType )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *pkeyboardType);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_KeyboardSubType )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long pkeyboardSubType);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_KeyboardSubType )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *pkeyboardSubType);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_KeyboardFunctionKey )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long pkeyboardFunctionKey);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_KeyboardFunctionKey )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *pkeyboardFunctionKey);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_WinceFixedPalette )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long pwinceFixedPalette);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_WinceFixedPalette )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *pwinceFixedPalette);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ConnectToServerConsole )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ VARIANT_BOOL pConnectToConsole);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ConnectToServerConsole )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ VARIANT_BOOL *pConnectToConsole);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_BitmapPersistence )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long pbitmapPersistence);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BitmapPersistence )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *pbitmapPersistence);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_MinutesToIdleTimeout )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long pminutesToIdleTimeout);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_MinutesToIdleTimeout )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *pminutesToIdleTimeout);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_SmartSizing )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ VARIANT_BOOL pfSmartSizing);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SmartSizing )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ VARIANT_BOOL *pfSmartSizing);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RdpdrLocalPrintingDocName )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ BSTR pLocalPrintingDocName);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RdpdrLocalPrintingDocName )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ BSTR *pLocalPrintingDocName);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RdpdrClipCleanTempDirString )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ BSTR clipCleanTempDirString);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RdpdrClipCleanTempDirString )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ BSTR *clipCleanTempDirString);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RdpdrClipPasteInfoString )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ BSTR clipPasteInfoString);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RdpdrClipPasteInfoString )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ BSTR *clipPasteInfoString);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ClearTextPassword )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ BSTR rhs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DisplayConnectionBar )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ VARIANT_BOOL pDisplayConnectionBar);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DisplayConnectionBar )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ VARIANT_BOOL *pDisplayConnectionBar);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_PinConnectionBar )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ VARIANT_BOOL pPinConnectionBar);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_PinConnectionBar )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ VARIANT_BOOL *pPinConnectionBar);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_GrabFocusOnConnect )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ VARIANT_BOOL pfGrabFocusOnConnect);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_GrabFocusOnConnect )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ VARIANT_BOOL *pfGrabFocusOnConnect);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_LoadBalanceInfo )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ BSTR pLBInfo);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_LoadBalanceInfo )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ BSTR *pLBInfo);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RedirectDrives )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ VARIANT_BOOL pRedirectDrives);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RedirectDrives )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ VARIANT_BOOL *pRedirectDrives);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RedirectPrinters )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ VARIANT_BOOL pRedirectPrinters);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RedirectPrinters )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ VARIANT_BOOL *pRedirectPrinters);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RedirectPorts )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ VARIANT_BOOL pRedirectPorts);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RedirectPorts )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ VARIANT_BOOL *pRedirectPorts);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RedirectSmartCards )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ VARIANT_BOOL pRedirectSmartCards);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RedirectSmartCards )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ VARIANT_BOOL *pRedirectSmartCards);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_BitmapVirtualCache16BppSize )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long pBitmapVirtualCache16BppSize);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BitmapVirtualCache16BppSize )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *pBitmapVirtualCache16BppSize);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_BitmapVirtualCache24BppSize )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long pBitmapVirtualCache24BppSize);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BitmapVirtualCache24BppSize )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *pBitmapVirtualCache24BppSize);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_PerformanceFlags )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ long pDisableList);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_PerformanceFlags )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ long *pDisableList);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ConnectWithEndpoint )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ VARIANT *rhs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_NotifyTSPublicKey )( 
            IMsRdpClientAdvancedSettings * This,
            /* [in] */ VARIANT_BOOL pfNotify);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_NotifyTSPublicKey )( 
            IMsRdpClientAdvancedSettings * This,
            /* [retval][out] */ VARIANT_BOOL *pfNotify);
        
        END_INTERFACE
    } IMsRdpClientAdvancedSettingsVtbl;

    interface IMsRdpClientAdvancedSettings
    {
        CONST_VTBL struct IMsRdpClientAdvancedSettingsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMsRdpClientAdvancedSettings_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMsRdpClientAdvancedSettings_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMsRdpClientAdvancedSettings_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMsRdpClientAdvancedSettings_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IMsRdpClientAdvancedSettings_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IMsRdpClientAdvancedSettings_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IMsRdpClientAdvancedSettings_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IMsRdpClientAdvancedSettings_put_Compress(This,pcompress)	\
    ( (This)->lpVtbl -> put_Compress(This,pcompress) ) 

#define IMsRdpClientAdvancedSettings_get_Compress(This,pcompress)	\
    ( (This)->lpVtbl -> get_Compress(This,pcompress) ) 

#define IMsRdpClientAdvancedSettings_put_BitmapPeristence(This,pbitmapPeristence)	\
    ( (This)->lpVtbl -> put_BitmapPeristence(This,pbitmapPeristence) ) 

#define IMsRdpClientAdvancedSettings_get_BitmapPeristence(This,pbitmapPeristence)	\
    ( (This)->lpVtbl -> get_BitmapPeristence(This,pbitmapPeristence) ) 

#define IMsRdpClientAdvancedSettings_put_allowBackgroundInput(This,pallowBackgroundInput)	\
    ( (This)->lpVtbl -> put_allowBackgroundInput(This,pallowBackgroundInput) ) 

#define IMsRdpClientAdvancedSettings_get_allowBackgroundInput(This,pallowBackgroundInput)	\
    ( (This)->lpVtbl -> get_allowBackgroundInput(This,pallowBackgroundInput) ) 

#define IMsRdpClientAdvancedSettings_put_KeyBoardLayoutStr(This,rhs)	\
    ( (This)->lpVtbl -> put_KeyBoardLayoutStr(This,rhs) ) 

#define IMsRdpClientAdvancedSettings_put_PluginDlls(This,rhs)	\
    ( (This)->lpVtbl -> put_PluginDlls(This,rhs) ) 

#define IMsRdpClientAdvancedSettings_put_IconFile(This,rhs)	\
    ( (This)->lpVtbl -> put_IconFile(This,rhs) ) 

#define IMsRdpClientAdvancedSettings_put_IconIndex(This,rhs)	\
    ( (This)->lpVtbl -> put_IconIndex(This,rhs) ) 

#define IMsRdpClientAdvancedSettings_put_ContainerHandledFullScreen(This,pContainerHandledFullScreen)	\
    ( (This)->lpVtbl -> put_ContainerHandledFullScreen(This,pContainerHandledFullScreen) ) 

#define IMsRdpClientAdvancedSettings_get_ContainerHandledFullScreen(This,pContainerHandledFullScreen)	\
    ( (This)->lpVtbl -> get_ContainerHandledFullScreen(This,pContainerHandledFullScreen) ) 

#define IMsRdpClientAdvancedSettings_put_DisableRdpdr(This,pDisableRdpdr)	\
    ( (This)->lpVtbl -> put_DisableRdpdr(This,pDisableRdpdr) ) 

#define IMsRdpClientAdvancedSettings_get_DisableRdpdr(This,pDisableRdpdr)	\
    ( (This)->lpVtbl -> get_DisableRdpdr(This,pDisableRdpdr) ) 


#define IMsRdpClientAdvancedSettings_put_SmoothScroll(This,psmoothScroll)	\
    ( (This)->lpVtbl -> put_SmoothScroll(This,psmoothScroll) ) 

#define IMsRdpClientAdvancedSettings_get_SmoothScroll(This,psmoothScroll)	\
    ( (This)->lpVtbl -> get_SmoothScroll(This,psmoothScroll) ) 

#define IMsRdpClientAdvancedSettings_put_AcceleratorPassthrough(This,pacceleratorPassthrough)	\
    ( (This)->lpVtbl -> put_AcceleratorPassthrough(This,pacceleratorPassthrough) ) 

#define IMsRdpClientAdvancedSettings_get_AcceleratorPassthrough(This,pacceleratorPassthrough)	\
    ( (This)->lpVtbl -> get_AcceleratorPassthrough(This,pacceleratorPassthrough) ) 

#define IMsRdpClientAdvancedSettings_put_ShadowBitmap(This,pshadowBitmap)	\
    ( (This)->lpVtbl -> put_ShadowBitmap(This,pshadowBitmap) ) 

#define IMsRdpClientAdvancedSettings_get_ShadowBitmap(This,pshadowBitmap)	\
    ( (This)->lpVtbl -> get_ShadowBitmap(This,pshadowBitmap) ) 

#define IMsRdpClientAdvancedSettings_put_TransportType(This,ptransportType)	\
    ( (This)->lpVtbl -> put_TransportType(This,ptransportType) ) 

#define IMsRdpClientAdvancedSettings_get_TransportType(This,ptransportType)	\
    ( (This)->lpVtbl -> get_TransportType(This,ptransportType) ) 

#define IMsRdpClientAdvancedSettings_put_SasSequence(This,psasSequence)	\
    ( (This)->lpVtbl -> put_SasSequence(This,psasSequence) ) 

#define IMsRdpClientAdvancedSettings_get_SasSequence(This,psasSequence)	\
    ( (This)->lpVtbl -> get_SasSequence(This,psasSequence) ) 

#define IMsRdpClientAdvancedSettings_put_EncryptionEnabled(This,pencryptionEnabled)	\
    ( (This)->lpVtbl -> put_EncryptionEnabled(This,pencryptionEnabled) ) 

#define IMsRdpClientAdvancedSettings_get_EncryptionEnabled(This,pencryptionEnabled)	\
    ( (This)->lpVtbl -> get_EncryptionEnabled(This,pencryptionEnabled) ) 

#define IMsRdpClientAdvancedSettings_put_DedicatedTerminal(This,pdedicatedTerminal)	\
    ( (This)->lpVtbl -> put_DedicatedTerminal(This,pdedicatedTerminal) ) 

#define IMsRdpClientAdvancedSettings_get_DedicatedTerminal(This,pdedicatedTerminal)	\
    ( (This)->lpVtbl -> get_DedicatedTerminal(This,pdedicatedTerminal) ) 

#define IMsRdpClientAdvancedSettings_put_RDPPort(This,prdpPort)	\
    ( (This)->lpVtbl -> put_RDPPort(This,prdpPort) ) 

#define IMsRdpClientAdvancedSettings_get_RDPPort(This,prdpPort)	\
    ( (This)->lpVtbl -> get_RDPPort(This,prdpPort) ) 

#define IMsRdpClientAdvancedSettings_put_EnableMouse(This,penableMouse)	\
    ( (This)->lpVtbl -> put_EnableMouse(This,penableMouse) ) 

#define IMsRdpClientAdvancedSettings_get_EnableMouse(This,penableMouse)	\
    ( (This)->lpVtbl -> get_EnableMouse(This,penableMouse) ) 

#define IMsRdpClientAdvancedSettings_put_DisableCtrlAltDel(This,pdisableCtrlAltDel)	\
    ( (This)->lpVtbl -> put_DisableCtrlAltDel(This,pdisableCtrlAltDel) ) 

#define IMsRdpClientAdvancedSettings_get_DisableCtrlAltDel(This,pdisableCtrlAltDel)	\
    ( (This)->lpVtbl -> get_DisableCtrlAltDel(This,pdisableCtrlAltDel) ) 

#define IMsRdpClientAdvancedSettings_put_EnableWindowsKey(This,penableWindowsKey)	\
    ( (This)->lpVtbl -> put_EnableWindowsKey(This,penableWindowsKey) ) 

#define IMsRdpClientAdvancedSettings_get_EnableWindowsKey(This,penableWindowsKey)	\
    ( (This)->lpVtbl -> get_EnableWindowsKey(This,penableWindowsKey) ) 

#define IMsRdpClientAdvancedSettings_put_DoubleClickDetect(This,pdoubleClickDetect)	\
    ( (This)->lpVtbl -> put_DoubleClickDetect(This,pdoubleClickDetect) ) 

#define IMsRdpClientAdvancedSettings_get_DoubleClickDetect(This,pdoubleClickDetect)	\
    ( (This)->lpVtbl -> get_DoubleClickDetect(This,pdoubleClickDetect) ) 

#define IMsRdpClientAdvancedSettings_put_MaximizeShell(This,pmaximizeShell)	\
    ( (This)->lpVtbl -> put_MaximizeShell(This,pmaximizeShell) ) 

#define IMsRdpClientAdvancedSettings_get_MaximizeShell(This,pmaximizeShell)	\
    ( (This)->lpVtbl -> get_MaximizeShell(This,pmaximizeShell) ) 

#define IMsRdpClientAdvancedSettings_put_HotKeyFullScreen(This,photKeyFullScreen)	\
    ( (This)->lpVtbl -> put_HotKeyFullScreen(This,photKeyFullScreen) ) 

#define IMsRdpClientAdvancedSettings_get_HotKeyFullScreen(This,photKeyFullScreen)	\
    ( (This)->lpVtbl -> get_HotKeyFullScreen(This,photKeyFullScreen) ) 

#define IMsRdpClientAdvancedSettings_put_HotKeyCtrlEsc(This,photKeyCtrlEsc)	\
    ( (This)->lpVtbl -> put_HotKeyCtrlEsc(This,photKeyCtrlEsc) ) 

#define IMsRdpClientAdvancedSettings_get_HotKeyCtrlEsc(This,photKeyCtrlEsc)	\
    ( (This)->lpVtbl -> get_HotKeyCtrlEsc(This,photKeyCtrlEsc) ) 

#define IMsRdpClientAdvancedSettings_put_HotKeyAltEsc(This,photKeyAltEsc)	\
    ( (This)->lpVtbl -> put_HotKeyAltEsc(This,photKeyAltEsc) ) 

#define IMsRdpClientAdvancedSettings_get_HotKeyAltEsc(This,photKeyAltEsc)	\
    ( (This)->lpVtbl -> get_HotKeyAltEsc(This,photKeyAltEsc) ) 

#define IMsRdpClientAdvancedSettings_put_HotKeyAltTab(This,photKeyAltTab)	\
    ( (This)->lpVtbl -> put_HotKeyAltTab(This,photKeyAltTab) ) 

#define IMsRdpClientAdvancedSettings_get_HotKeyAltTab(This,photKeyAltTab)	\
    ( (This)->lpVtbl -> get_HotKeyAltTab(This,photKeyAltTab) ) 

#define IMsRdpClientAdvancedSettings_put_HotKeyAltShiftTab(This,photKeyAltShiftTab)	\
    ( (This)->lpVtbl -> put_HotKeyAltShiftTab(This,photKeyAltShiftTab) ) 

#define IMsRdpClientAdvancedSettings_get_HotKeyAltShiftTab(This,photKeyAltShiftTab)	\
    ( (This)->lpVtbl -> get_HotKeyAltShiftTab(This,photKeyAltShiftTab) ) 

#define IMsRdpClientAdvancedSettings_put_HotKeyAltSpace(This,photKeyAltSpace)	\
    ( (This)->lpVtbl -> put_HotKeyAltSpace(This,photKeyAltSpace) ) 

#define IMsRdpClientAdvancedSettings_get_HotKeyAltSpace(This,photKeyAltSpace)	\
    ( (This)->lpVtbl -> get_HotKeyAltSpace(This,photKeyAltSpace) ) 

#define IMsRdpClientAdvancedSettings_put_HotKeyCtrlAltDel(This,photKeyCtrlAltDel)	\
    ( (This)->lpVtbl -> put_HotKeyCtrlAltDel(This,photKeyCtrlAltDel) ) 

#define IMsRdpClientAdvancedSettings_get_HotKeyCtrlAltDel(This,photKeyCtrlAltDel)	\
    ( (This)->lpVtbl -> get_HotKeyCtrlAltDel(This,photKeyCtrlAltDel) ) 

#define IMsRdpClientAdvancedSettings_put_orderDrawThreshold(This,porderDrawThreshold)	\
    ( (This)->lpVtbl -> put_orderDrawThreshold(This,porderDrawThreshold) ) 

#define IMsRdpClientAdvancedSettings_get_orderDrawThreshold(This,porderDrawThreshold)	\
    ( (This)->lpVtbl -> get_orderDrawThreshold(This,porderDrawThreshold) ) 

#define IMsRdpClientAdvancedSettings_put_BitmapCacheSize(This,pbitmapCacheSize)	\
    ( (This)->lpVtbl -> put_BitmapCacheSize(This,pbitmapCacheSize) ) 

#define IMsRdpClientAdvancedSettings_get_BitmapCacheSize(This,pbitmapCacheSize)	\
    ( (This)->lpVtbl -> get_BitmapCacheSize(This,pbitmapCacheSize) ) 

#define IMsRdpClientAdvancedSettings_put_BitmapVirtualCacheSize(This,pbitmapVirtualCacheSize)	\
    ( (This)->lpVtbl -> put_BitmapVirtualCacheSize(This,pbitmapVirtualCacheSize) ) 

#define IMsRdpClientAdvancedSettings_get_BitmapVirtualCacheSize(This,pbitmapVirtualCacheSize)	\
    ( (This)->lpVtbl -> get_BitmapVirtualCacheSize(This,pbitmapVirtualCacheSize) ) 

#define IMsRdpClientAdvancedSettings_put_ScaleBitmapCachesByBPP(This,pbScale)	\
    ( (This)->lpVtbl -> put_ScaleBitmapCachesByBPP(This,pbScale) ) 

#define IMsRdpClientAdvancedSettings_get_ScaleBitmapCachesByBPP(This,pbScale)	\
    ( (This)->lpVtbl -> get_ScaleBitmapCachesByBPP(This,pbScale) ) 

#define IMsRdpClientAdvancedSettings_put_NumBitmapCaches(This,pnumBitmapCaches)	\
    ( (This)->lpVtbl -> put_NumBitmapCaches(This,pnumBitmapCaches) ) 

#define IMsRdpClientAdvancedSettings_get_NumBitmapCaches(This,pnumBitmapCaches)	\
    ( (This)->lpVtbl -> get_NumBitmapCaches(This,pnumBitmapCaches) ) 

#define IMsRdpClientAdvancedSettings_put_CachePersistenceActive(This,pcachePersistenceActive)	\
    ( (This)->lpVtbl -> put_CachePersistenceActive(This,pcachePersistenceActive) ) 

#define IMsRdpClientAdvancedSettings_get_CachePersistenceActive(This,pcachePersistenceActive)	\
    ( (This)->lpVtbl -> get_CachePersistenceActive(This,pcachePersistenceActive) ) 

#define IMsRdpClientAdvancedSettings_put_PersistCacheDirectory(This,rhs)	\
    ( (This)->lpVtbl -> put_PersistCacheDirectory(This,rhs) ) 

#define IMsRdpClientAdvancedSettings_put_brushSupportLevel(This,pbrushSupportLevel)	\
    ( (This)->lpVtbl -> put_brushSupportLevel(This,pbrushSupportLevel) ) 

#define IMsRdpClientAdvancedSettings_get_brushSupportLevel(This,pbrushSupportLevel)	\
    ( (This)->lpVtbl -> get_brushSupportLevel(This,pbrushSupportLevel) ) 

#define IMsRdpClientAdvancedSettings_put_minInputSendInterval(This,pminInputSendInterval)	\
    ( (This)->lpVtbl -> put_minInputSendInterval(This,pminInputSendInterval) ) 

#define IMsRdpClientAdvancedSettings_get_minInputSendInterval(This,pminInputSendInterval)	\
    ( (This)->lpVtbl -> get_minInputSendInterval(This,pminInputSendInterval) ) 

#define IMsRdpClientAdvancedSettings_put_InputEventsAtOnce(This,pinputEventsAtOnce)	\
    ( (This)->lpVtbl -> put_InputEventsAtOnce(This,pinputEventsAtOnce) ) 

#define IMsRdpClientAdvancedSettings_get_InputEventsAtOnce(This,pinputEventsAtOnce)	\
    ( (This)->lpVtbl -> get_InputEventsAtOnce(This,pinputEventsAtOnce) ) 

#define IMsRdpClientAdvancedSettings_put_maxEventCount(This,pmaxEventCount)	\
    ( (This)->lpVtbl -> put_maxEventCount(This,pmaxEventCount) ) 

#define IMsRdpClientAdvancedSettings_get_maxEventCount(This,pmaxEventCount)	\
    ( (This)->lpVtbl -> get_maxEventCount(This,pmaxEventCount) ) 

#define IMsRdpClientAdvancedSettings_put_keepAliveInterval(This,pkeepAliveInterval)	\
    ( (This)->lpVtbl -> put_keepAliveInterval(This,pkeepAliveInterval) ) 

#define IMsRdpClientAdvancedSettings_get_keepAliveInterval(This,pkeepAliveInterval)	\
    ( (This)->lpVtbl -> get_keepAliveInterval(This,pkeepAliveInterval) ) 

#define IMsRdpClientAdvancedSettings_put_shutdownTimeout(This,pshutdownTimeout)	\
    ( (This)->lpVtbl -> put_shutdownTimeout(This,pshutdownTimeout) ) 

#define IMsRdpClientAdvancedSettings_get_shutdownTimeout(This,pshutdownTimeout)	\
    ( (This)->lpVtbl -> get_shutdownTimeout(This,pshutdownTimeout) ) 

#define IMsRdpClientAdvancedSettings_put_overallConnectionTimeout(This,poverallConnectionTimeout)	\
    ( (This)->lpVtbl -> put_overallConnectionTimeout(This,poverallConnectionTimeout) ) 

#define IMsRdpClientAdvancedSettings_get_overallConnectionTimeout(This,poverallConnectionTimeout)	\
    ( (This)->lpVtbl -> get_overallConnectionTimeout(This,poverallConnectionTimeout) ) 

#define IMsRdpClientAdvancedSettings_put_singleConnectionTimeout(This,psingleConnectionTimeout)	\
    ( (This)->lpVtbl -> put_singleConnectionTimeout(This,psingleConnectionTimeout) ) 

#define IMsRdpClientAdvancedSettings_get_singleConnectionTimeout(This,psingleConnectionTimeout)	\
    ( (This)->lpVtbl -> get_singleConnectionTimeout(This,psingleConnectionTimeout) ) 

#define IMsRdpClientAdvancedSettings_put_KeyboardType(This,pkeyboardType)	\
    ( (This)->lpVtbl -> put_KeyboardType(This,pkeyboardType) ) 

#define IMsRdpClientAdvancedSettings_get_KeyboardType(This,pkeyboardType)	\
    ( (This)->lpVtbl -> get_KeyboardType(This,pkeyboardType) ) 

#define IMsRdpClientAdvancedSettings_put_KeyboardSubType(This,pkeyboardSubType)	\
    ( (This)->lpVtbl -> put_KeyboardSubType(This,pkeyboardSubType) ) 

#define IMsRdpClientAdvancedSettings_get_KeyboardSubType(This,pkeyboardSubType)	\
    ( (This)->lpVtbl -> get_KeyboardSubType(This,pkeyboardSubType) ) 

#define IMsRdpClientAdvancedSettings_put_KeyboardFunctionKey(This,pkeyboardFunctionKey)	\
    ( (This)->lpVtbl -> put_KeyboardFunctionKey(This,pkeyboardFunctionKey) ) 

#define IMsRdpClientAdvancedSettings_get_KeyboardFunctionKey(This,pkeyboardFunctionKey)	\
    ( (This)->lpVtbl -> get_KeyboardFunctionKey(This,pkeyboardFunctionKey) ) 

#define IMsRdpClientAdvancedSettings_put_WinceFixedPalette(This,pwinceFixedPalette)	\
    ( (This)->lpVtbl -> put_WinceFixedPalette(This,pwinceFixedPalette) ) 

#define IMsRdpClientAdvancedSettings_get_WinceFixedPalette(This,pwinceFixedPalette)	\
    ( (This)->lpVtbl -> get_WinceFixedPalette(This,pwinceFixedPalette) ) 

#define IMsRdpClientAdvancedSettings_put_ConnectToServerConsole(This,pConnectToConsole)	\
    ( (This)->lpVtbl -> put_ConnectToServerConsole(This,pConnectToConsole) ) 

#define IMsRdpClientAdvancedSettings_get_ConnectToServerConsole(This,pConnectToConsole)	\
    ( (This)->lpVtbl -> get_ConnectToServerConsole(This,pConnectToConsole) ) 

#define IMsRdpClientAdvancedSettings_put_BitmapPersistence(This,pbitmapPersistence)	\
    ( (This)->lpVtbl -> put_BitmapPersistence(This,pbitmapPersistence) ) 

#define IMsRdpClientAdvancedSettings_get_BitmapPersistence(This,pbitmapPersistence)	\
    ( (This)->lpVtbl -> get_BitmapPersistence(This,pbitmapPersistence) ) 

#define IMsRdpClientAdvancedSettings_put_MinutesToIdleTimeout(This,pminutesToIdleTimeout)	\
    ( (This)->lpVtbl -> put_MinutesToIdleTimeout(This,pminutesToIdleTimeout) ) 

#define IMsRdpClientAdvancedSettings_get_MinutesToIdleTimeout(This,pminutesToIdleTimeout)	\
    ( (This)->lpVtbl -> get_MinutesToIdleTimeout(This,pminutesToIdleTimeout) ) 

#define IMsRdpClientAdvancedSettings_put_SmartSizing(This,pfSmartSizing)	\
    ( (This)->lpVtbl -> put_SmartSizing(This,pfSmartSizing) ) 

#define IMsRdpClientAdvancedSettings_get_SmartSizing(This,pfSmartSizing)	\
    ( (This)->lpVtbl -> get_SmartSizing(This,pfSmartSizing) ) 

#define IMsRdpClientAdvancedSettings_put_RdpdrLocalPrintingDocName(This,pLocalPrintingDocName)	\
    ( (This)->lpVtbl -> put_RdpdrLocalPrintingDocName(This,pLocalPrintingDocName) ) 

#define IMsRdpClientAdvancedSettings_get_RdpdrLocalPrintingDocName(This,pLocalPrintingDocName)	\
    ( (This)->lpVtbl -> get_RdpdrLocalPrintingDocName(This,pLocalPrintingDocName) ) 

#define IMsRdpClientAdvancedSettings_put_RdpdrClipCleanTempDirString(This,clipCleanTempDirString)	\
    ( (This)->lpVtbl -> put_RdpdrClipCleanTempDirString(This,clipCleanTempDirString) ) 

#define IMsRdpClientAdvancedSettings_get_RdpdrClipCleanTempDirString(This,clipCleanTempDirString)	\
    ( (This)->lpVtbl -> get_RdpdrClipCleanTempDirString(This,clipCleanTempDirString) ) 

#define IMsRdpClientAdvancedSettings_put_RdpdrClipPasteInfoString(This,clipPasteInfoString)	\
    ( (This)->lpVtbl -> put_RdpdrClipPasteInfoString(This,clipPasteInfoString) ) 

#define IMsRdpClientAdvancedSettings_get_RdpdrClipPasteInfoString(This,clipPasteInfoString)	\
    ( (This)->lpVtbl -> get_RdpdrClipPasteInfoString(This,clipPasteInfoString) ) 

#define IMsRdpClientAdvancedSettings_put_ClearTextPassword(This,rhs)	\
    ( (This)->lpVtbl -> put_ClearTextPassword(This,rhs) ) 

#define IMsRdpClientAdvancedSettings_put_DisplayConnectionBar(This,pDisplayConnectionBar)	\
    ( (This)->lpVtbl -> put_DisplayConnectionBar(This,pDisplayConnectionBar) ) 

#define IMsRdpClientAdvancedSettings_get_DisplayConnectionBar(This,pDisplayConnectionBar)	\
    ( (This)->lpVtbl -> get_DisplayConnectionBar(This,pDisplayConnectionBar) ) 

#define IMsRdpClientAdvancedSettings_put_PinConnectionBar(This,pPinConnectionBar)	\
    ( (This)->lpVtbl -> put_PinConnectionBar(This,pPinConnectionBar) ) 

#define IMsRdpClientAdvancedSettings_get_PinConnectionBar(This,pPinConnectionBar)	\
    ( (This)->lpVtbl -> get_PinConnectionBar(This,pPinConnectionBar) ) 

#define IMsRdpClientAdvancedSettings_put_GrabFocusOnConnect(This,pfGrabFocusOnConnect)	\
    ( (This)->lpVtbl -> put_GrabFocusOnConnect(This,pfGrabFocusOnConnect) ) 

#define IMsRdpClientAdvancedSettings_get_GrabFocusOnConnect(This,pfGrabFocusOnConnect)	\
    ( (This)->lpVtbl -> get_GrabFocusOnConnect(This,pfGrabFocusOnConnect) ) 

#define IMsRdpClientAdvancedSettings_put_LoadBalanceInfo(This,pLBInfo)	\
    ( (This)->lpVtbl -> put_LoadBalanceInfo(This,pLBInfo) ) 

#define IMsRdpClientAdvancedSettings_get_LoadBalanceInfo(This,pLBInfo)	\
    ( (This)->lpVtbl -> get_LoadBalanceInfo(This,pLBInfo) ) 

#define IMsRdpClientAdvancedSettings_put_RedirectDrives(This,pRedirectDrives)	\
    ( (This)->lpVtbl -> put_RedirectDrives(This,pRedirectDrives) ) 

#define IMsRdpClientAdvancedSettings_get_RedirectDrives(This,pRedirectDrives)	\
    ( (This)->lpVtbl -> get_RedirectDrives(This,pRedirectDrives) ) 

#define IMsRdpClientAdvancedSettings_put_RedirectPrinters(This,pRedirectPrinters)	\
    ( (This)->lpVtbl -> put_RedirectPrinters(This,pRedirectPrinters) ) 

#define IMsRdpClientAdvancedSettings_get_RedirectPrinters(This,pRedirectPrinters)	\
    ( (This)->lpVtbl -> get_RedirectPrinters(This,pRedirectPrinters) ) 

#define IMsRdpClientAdvancedSettings_put_RedirectPorts(This,pRedirectPorts)	\
    ( (This)->lpVtbl -> put_RedirectPorts(This,pRedirectPorts) ) 

#define IMsRdpClientAdvancedSettings_get_RedirectPorts(This,pRedirectPorts)	\
    ( (This)->lpVtbl -> get_RedirectPorts(This,pRedirectPorts) ) 

#define IMsRdpClientAdvancedSettings_put_RedirectSmartCards(This,pRedirectSmartCards)	\
    ( (This)->lpVtbl -> put_RedirectSmartCards(This,pRedirectSmartCards) ) 

#define IMsRdpClientAdvancedSettings_get_RedirectSmartCards(This,pRedirectSmartCards)	\
    ( (This)->lpVtbl -> get_RedirectSmartCards(This,pRedirectSmartCards) ) 

#define IMsRdpClientAdvancedSettings_put_BitmapVirtualCache16BppSize(This,pBitmapVirtualCache16BppSize)	\
    ( (This)->lpVtbl -> put_BitmapVirtualCache16BppSize(This,pBitmapVirtualCache16BppSize) ) 

#define IMsRdpClientAdvancedSettings_get_BitmapVirtualCache16BppSize(This,pBitmapVirtualCache16BppSize)	\
    ( (This)->lpVtbl -> get_BitmapVirtualCache16BppSize(This,pBitmapVirtualCache16BppSize) ) 

#define IMsRdpClientAdvancedSettings_put_BitmapVirtualCache24BppSize(This,pBitmapVirtualCache24BppSize)	\
    ( (This)->lpVtbl -> put_BitmapVirtualCache24BppSize(This,pBitmapVirtualCache24BppSize) ) 

#define IMsRdpClientAdvancedSettings_get_BitmapVirtualCache24BppSize(This,pBitmapVirtualCache24BppSize)	\
    ( (This)->lpVtbl -> get_BitmapVirtualCache24BppSize(This,pBitmapVirtualCache24BppSize) ) 

#define IMsRdpClientAdvancedSettings_put_PerformanceFlags(This,pDisableList)	\
    ( (This)->lpVtbl -> put_PerformanceFlags(This,pDisableList) ) 

#define IMsRdpClientAdvancedSettings_get_PerformanceFlags(This,pDisableList)	\
    ( (This)->lpVtbl -> get_PerformanceFlags(This,pDisableList) ) 

#define IMsRdpClientAdvancedSettings_put_ConnectWithEndpoint(This,rhs)	\
    ( (This)->lpVtbl -> put_ConnectWithEndpoint(This,rhs) ) 

#define IMsRdpClientAdvancedSettings_put_NotifyTSPublicKey(This,pfNotify)	\
    ( (This)->lpVtbl -> put_NotifyTSPublicKey(This,pfNotify) ) 

#define IMsRdpClientAdvancedSettings_get_NotifyTSPublicKey(This,pfNotify)	\
    ( (This)->lpVtbl -> get_NotifyTSPublicKey(This,pfNotify) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget][id] */ HRESULT STDMETHODCALLTYPE IMsRdpClientAdvancedSettings_get_RedirectSmartCards_Proxy( 
    IMsRdpClientAdvancedSettings * This,
    /* [retval][out] */ VARIANT_BOOL *pRedirectSmartCards);


void __RPC_STUB IMsRdpClientAdvancedSettings_get_RedirectSmartCards_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMsRdpClientAdvancedSettings_put_BitmapVirtualCache16BppSize_Proxy( 
    IMsRdpClientAdvancedSettings * This,
    /* [in] */ long pBitmapVirtualCache16BppSize);


void __RPC_STUB IMsRdpClientAdvancedSettings_put_BitmapVirtualCache16BppSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IMsRdpClientAdvancedSettings_get_BitmapVirtualCache16BppSize_Proxy( 
    IMsRdpClientAdvancedSettings * This,
    /* [retval][out] */ long *pBitmapVirtualCache16BppSize);


void __RPC_STUB IMsRdpClientAdvancedSettings_get_BitmapVirtualCache16BppSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMsRdpClientAdvancedSettings_put_BitmapVirtualCache24BppSize_Proxy( 
    IMsRdpClientAdvancedSettings * This,
    /* [in] */ long pBitmapVirtualCache24BppSize);


void __RPC_STUB IMsRdpClientAdvancedSettings_put_BitmapVirtualCache24BppSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IMsRdpClientAdvancedSettings_get_BitmapVirtualCache24BppSize_Proxy( 
    IMsRdpClientAdvancedSettings * This,
    /* [retval][out] */ long *pBitmapVirtualCache24BppSize);


void __RPC_STUB IMsRdpClientAdvancedSettings_get_BitmapVirtualCache24BppSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMsRdpClientAdvancedSettings_put_PerformanceFlags_Proxy( 
    IMsRdpClientAdvancedSettings * This,
    /* [in] */ long pDisableList);


void __RPC_STUB IMsRdpClientAdvancedSettings_put_PerformanceFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IMsRdpClientAdvancedSettings_get_PerformanceFlags_Proxy( 
    IMsRdpClientAdvancedSettings * This,
    /* [retval][out] */ long *pDisableList);


void __RPC_STUB IMsRdpClientAdvancedSettings_get_PerformanceFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMsRdpClientAdvancedSettings_put_ConnectWithEndpoint_Proxy( 
    IMsRdpClientAdvancedSettings * This,
    /* [in] */ VARIANT *rhs);


void __RPC_STUB IMsRdpClientAdvancedSettings_put_ConnectWithEndpoint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMsRdpClientAdvancedSettings_put_NotifyTSPublicKey_Proxy( 
    IMsRdpClientAdvancedSettings * This,
    /* [in] */ VARIANT_BOOL pfNotify);


void __RPC_STUB IMsRdpClientAdvancedSettings_put_NotifyTSPublicKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IMsRdpClientAdvancedSettings_get_NotifyTSPublicKey_Proxy( 
    IMsRdpClientAdvancedSettings * This,
    /* [retval][out] */ VARIANT_BOOL *pfNotify);


void __RPC_STUB IMsRdpClientAdvancedSettings_get_NotifyTSPublicKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMsRdpClientAdvancedSettings_INTERFACE_DEFINED__ */


#ifndef __IMsRdpClientAdvancedSettings2_INTERFACE_DEFINED__
#define __IMsRdpClientAdvancedSettings2_INTERFACE_DEFINED__

/* interface IMsRdpClientAdvancedSettings2 */
/* [object][oleautomation][dual][uuid] */ 


EXTERN_C const IID IID_IMsRdpClientAdvancedSettings2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9AC42117-2B76-4320-AA44-0E616AB8437B")
    IMsRdpClientAdvancedSettings2 : public IMsRdpClientAdvancedSettings
    {
    public:
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_CanAutoReconnect( 
            /* [retval][out] */ VARIANT_BOOL *pfCanAutoReconnect) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_EnableAutoReconnect( 
            /* [in] */ VARIANT_BOOL pfEnableAutoReconnect) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_EnableAutoReconnect( 
            /* [retval][out] */ VARIANT_BOOL *pfEnableAutoReconnect) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_MaxReconnectAttempts( 
            /* [in] */ long pMaxReconnectAttempts) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_MaxReconnectAttempts( 
            /* [retval][out] */ long *pMaxReconnectAttempts) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMsRdpClientAdvancedSettings2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMsRdpClientAdvancedSettings2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMsRdpClientAdvancedSettings2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Compress )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pcompress);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Compress )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pcompress);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_BitmapPeristence )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pbitmapPeristence);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BitmapPeristence )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pbitmapPeristence);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_allowBackgroundInput )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pallowBackgroundInput);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_allowBackgroundInput )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pallowBackgroundInput);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_KeyBoardLayoutStr )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ BSTR rhs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_PluginDlls )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ BSTR rhs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_IconFile )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ BSTR rhs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_IconIndex )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long rhs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ContainerHandledFullScreen )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pContainerHandledFullScreen);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ContainerHandledFullScreen )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pContainerHandledFullScreen);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DisableRdpdr )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pDisableRdpdr);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DisableRdpdr )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pDisableRdpdr);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_SmoothScroll )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long psmoothScroll);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SmoothScroll )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *psmoothScroll);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_AcceleratorPassthrough )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pacceleratorPassthrough);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AcceleratorPassthrough )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pacceleratorPassthrough);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ShadowBitmap )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pshadowBitmap);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ShadowBitmap )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pshadowBitmap);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_TransportType )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long ptransportType);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_TransportType )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *ptransportType);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_SasSequence )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long psasSequence);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SasSequence )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *psasSequence);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_EncryptionEnabled )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pencryptionEnabled);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_EncryptionEnabled )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pencryptionEnabled);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DedicatedTerminal )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pdedicatedTerminal);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DedicatedTerminal )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pdedicatedTerminal);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RDPPort )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long prdpPort);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RDPPort )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *prdpPort);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_EnableMouse )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long penableMouse);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_EnableMouse )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *penableMouse);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DisableCtrlAltDel )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pdisableCtrlAltDel);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DisableCtrlAltDel )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pdisableCtrlAltDel);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_EnableWindowsKey )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long penableWindowsKey);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_EnableWindowsKey )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *penableWindowsKey);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DoubleClickDetect )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pdoubleClickDetect);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DoubleClickDetect )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pdoubleClickDetect);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_MaximizeShell )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pmaximizeShell);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_MaximizeShell )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pmaximizeShell);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_HotKeyFullScreen )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long photKeyFullScreen);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HotKeyFullScreen )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *photKeyFullScreen);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_HotKeyCtrlEsc )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long photKeyCtrlEsc);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HotKeyCtrlEsc )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *photKeyCtrlEsc);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_HotKeyAltEsc )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long photKeyAltEsc);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HotKeyAltEsc )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *photKeyAltEsc);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_HotKeyAltTab )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long photKeyAltTab);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HotKeyAltTab )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *photKeyAltTab);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_HotKeyAltShiftTab )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long photKeyAltShiftTab);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HotKeyAltShiftTab )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *photKeyAltShiftTab);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_HotKeyAltSpace )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long photKeyAltSpace);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HotKeyAltSpace )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *photKeyAltSpace);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_HotKeyCtrlAltDel )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long photKeyCtrlAltDel);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HotKeyCtrlAltDel )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *photKeyCtrlAltDel);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_orderDrawThreshold )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long porderDrawThreshold);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_orderDrawThreshold )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *porderDrawThreshold);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_BitmapCacheSize )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pbitmapCacheSize);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BitmapCacheSize )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pbitmapCacheSize);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_BitmapVirtualCacheSize )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pbitmapVirtualCacheSize);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BitmapVirtualCacheSize )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pbitmapVirtualCacheSize);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ScaleBitmapCachesByBPP )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pbScale);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ScaleBitmapCachesByBPP )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pbScale);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_NumBitmapCaches )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pnumBitmapCaches);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_NumBitmapCaches )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pnumBitmapCaches);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_CachePersistenceActive )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pcachePersistenceActive);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CachePersistenceActive )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pcachePersistenceActive);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_PersistCacheDirectory )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ BSTR rhs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_brushSupportLevel )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pbrushSupportLevel);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_brushSupportLevel )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pbrushSupportLevel);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_minInputSendInterval )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pminInputSendInterval);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_minInputSendInterval )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pminInputSendInterval);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_InputEventsAtOnce )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pinputEventsAtOnce);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_InputEventsAtOnce )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pinputEventsAtOnce);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_maxEventCount )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pmaxEventCount);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_maxEventCount )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pmaxEventCount);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_keepAliveInterval )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pkeepAliveInterval);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_keepAliveInterval )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pkeepAliveInterval);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_shutdownTimeout )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pshutdownTimeout);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_shutdownTimeout )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pshutdownTimeout);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_overallConnectionTimeout )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long poverallConnectionTimeout);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_overallConnectionTimeout )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *poverallConnectionTimeout);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_singleConnectionTimeout )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long psingleConnectionTimeout);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_singleConnectionTimeout )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *psingleConnectionTimeout);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_KeyboardType )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pkeyboardType);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_KeyboardType )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pkeyboardType);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_KeyboardSubType )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pkeyboardSubType);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_KeyboardSubType )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pkeyboardSubType);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_KeyboardFunctionKey )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pkeyboardFunctionKey);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_KeyboardFunctionKey )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pkeyboardFunctionKey);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_WinceFixedPalette )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pwinceFixedPalette);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_WinceFixedPalette )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pwinceFixedPalette);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ConnectToServerConsole )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ VARIANT_BOOL pConnectToConsole);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ConnectToServerConsole )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ VARIANT_BOOL *pConnectToConsole);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_BitmapPersistence )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pbitmapPersistence);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BitmapPersistence )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pbitmapPersistence);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_MinutesToIdleTimeout )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pminutesToIdleTimeout);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_MinutesToIdleTimeout )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pminutesToIdleTimeout);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_SmartSizing )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ VARIANT_BOOL pfSmartSizing);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SmartSizing )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ VARIANT_BOOL *pfSmartSizing);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RdpdrLocalPrintingDocName )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ BSTR pLocalPrintingDocName);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RdpdrLocalPrintingDocName )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ BSTR *pLocalPrintingDocName);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RdpdrClipCleanTempDirString )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ BSTR clipCleanTempDirString);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RdpdrClipCleanTempDirString )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ BSTR *clipCleanTempDirString);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RdpdrClipPasteInfoString )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ BSTR clipPasteInfoString);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RdpdrClipPasteInfoString )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ BSTR *clipPasteInfoString);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ClearTextPassword )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ BSTR rhs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DisplayConnectionBar )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ VARIANT_BOOL pDisplayConnectionBar);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DisplayConnectionBar )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ VARIANT_BOOL *pDisplayConnectionBar);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_PinConnectionBar )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ VARIANT_BOOL pPinConnectionBar);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_PinConnectionBar )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ VARIANT_BOOL *pPinConnectionBar);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_GrabFocusOnConnect )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ VARIANT_BOOL pfGrabFocusOnConnect);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_GrabFocusOnConnect )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ VARIANT_BOOL *pfGrabFocusOnConnect);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_LoadBalanceInfo )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ BSTR pLBInfo);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_LoadBalanceInfo )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ BSTR *pLBInfo);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RedirectDrives )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ VARIANT_BOOL pRedirectDrives);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RedirectDrives )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ VARIANT_BOOL *pRedirectDrives);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RedirectPrinters )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ VARIANT_BOOL pRedirectPrinters);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RedirectPrinters )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ VARIANT_BOOL *pRedirectPrinters);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RedirectPorts )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ VARIANT_BOOL pRedirectPorts);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RedirectPorts )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ VARIANT_BOOL *pRedirectPorts);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RedirectSmartCards )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ VARIANT_BOOL pRedirectSmartCards);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RedirectSmartCards )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ VARIANT_BOOL *pRedirectSmartCards);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_BitmapVirtualCache16BppSize )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pBitmapVirtualCache16BppSize);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BitmapVirtualCache16BppSize )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pBitmapVirtualCache16BppSize);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_BitmapVirtualCache24BppSize )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pBitmapVirtualCache24BppSize);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BitmapVirtualCache24BppSize )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pBitmapVirtualCache24BppSize);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_PerformanceFlags )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pDisableList);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_PerformanceFlags )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pDisableList);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ConnectWithEndpoint )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ VARIANT *rhs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_NotifyTSPublicKey )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ VARIANT_BOOL pfNotify);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_NotifyTSPublicKey )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ VARIANT_BOOL *pfNotify);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CanAutoReconnect )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ VARIANT_BOOL *pfCanAutoReconnect);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_EnableAutoReconnect )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ VARIANT_BOOL pfEnableAutoReconnect);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_EnableAutoReconnect )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ VARIANT_BOOL *pfEnableAutoReconnect);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_MaxReconnectAttempts )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [in] */ long pMaxReconnectAttempts);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_MaxReconnectAttempts )( 
            IMsRdpClientAdvancedSettings2 * This,
            /* [retval][out] */ long *pMaxReconnectAttempts);
        
        END_INTERFACE
    } IMsRdpClientAdvancedSettings2Vtbl;

    interface IMsRdpClientAdvancedSettings2
    {
        CONST_VTBL struct IMsRdpClientAdvancedSettings2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMsRdpClientAdvancedSettings2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMsRdpClientAdvancedSettings2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMsRdpClientAdvancedSettings2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMsRdpClientAdvancedSettings2_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IMsRdpClientAdvancedSettings2_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IMsRdpClientAdvancedSettings2_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IMsRdpClientAdvancedSettings2_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IMsRdpClientAdvancedSettings2_put_Compress(This,pcompress)	\
    ( (This)->lpVtbl -> put_Compress(This,pcompress) ) 

#define IMsRdpClientAdvancedSettings2_get_Compress(This,pcompress)	\
    ( (This)->lpVtbl -> get_Compress(This,pcompress) ) 

#define IMsRdpClientAdvancedSettings2_put_BitmapPeristence(This,pbitmapPeristence)	\
    ( (This)->lpVtbl -> put_BitmapPeristence(This,pbitmapPeristence) ) 

#define IMsRdpClientAdvancedSettings2_get_BitmapPeristence(This,pbitmapPeristence)	\
    ( (This)->lpVtbl -> get_BitmapPeristence(This,pbitmapPeristence) ) 

#define IMsRdpClientAdvancedSettings2_put_allowBackgroundInput(This,pallowBackgroundInput)	\
    ( (This)->lpVtbl -> put_allowBackgroundInput(This,pallowBackgroundInput) ) 

#define IMsRdpClientAdvancedSettings2_get_allowBackgroundInput(This,pallowBackgroundInput)	\
    ( (This)->lpVtbl -> get_allowBackgroundInput(This,pallowBackgroundInput) ) 

#define IMsRdpClientAdvancedSettings2_put_KeyBoardLayoutStr(This,rhs)	\
    ( (This)->lpVtbl -> put_KeyBoardLayoutStr(This,rhs) ) 

#define IMsRdpClientAdvancedSettings2_put_PluginDlls(This,rhs)	\
    ( (This)->lpVtbl -> put_PluginDlls(This,rhs) ) 

#define IMsRdpClientAdvancedSettings2_put_IconFile(This,rhs)	\
    ( (This)->lpVtbl -> put_IconFile(This,rhs) ) 

#define IMsRdpClientAdvancedSettings2_put_IconIndex(This,rhs)	\
    ( (This)->lpVtbl -> put_IconIndex(This,rhs) ) 

#define IMsRdpClientAdvancedSettings2_put_ContainerHandledFullScreen(This,pContainerHandledFullScreen)	\
    ( (This)->lpVtbl -> put_ContainerHandledFullScreen(This,pContainerHandledFullScreen) ) 

#define IMsRdpClientAdvancedSettings2_get_ContainerHandledFullScreen(This,pContainerHandledFullScreen)	\
    ( (This)->lpVtbl -> get_ContainerHandledFullScreen(This,pContainerHandledFullScreen) ) 

#define IMsRdpClientAdvancedSettings2_put_DisableRdpdr(This,pDisableRdpdr)	\
    ( (This)->lpVtbl -> put_DisableRdpdr(This,pDisableRdpdr) ) 

#define IMsRdpClientAdvancedSettings2_get_DisableRdpdr(This,pDisableRdpdr)	\
    ( (This)->lpVtbl -> get_DisableRdpdr(This,pDisableRdpdr) ) 


#define IMsRdpClientAdvancedSettings2_put_SmoothScroll(This,psmoothScroll)	\
    ( (This)->lpVtbl -> put_SmoothScroll(This,psmoothScroll) ) 

#define IMsRdpClientAdvancedSettings2_get_SmoothScroll(This,psmoothScroll)	\
    ( (This)->lpVtbl -> get_SmoothScroll(This,psmoothScroll) ) 

#define IMsRdpClientAdvancedSettings2_put_AcceleratorPassthrough(This,pacceleratorPassthrough)	\
    ( (This)->lpVtbl -> put_AcceleratorPassthrough(This,pacceleratorPassthrough) ) 

#define IMsRdpClientAdvancedSettings2_get_AcceleratorPassthrough(This,pacceleratorPassthrough)	\
    ( (This)->lpVtbl -> get_AcceleratorPassthrough(This,pacceleratorPassthrough) ) 

#define IMsRdpClientAdvancedSettings2_put_ShadowBitmap(This,pshadowBitmap)	\
    ( (This)->lpVtbl -> put_ShadowBitmap(This,pshadowBitmap) ) 

#define IMsRdpClientAdvancedSettings2_get_ShadowBitmap(This,pshadowBitmap)	\
    ( (This)->lpVtbl -> get_ShadowBitmap(This,pshadowBitmap) ) 

#define IMsRdpClientAdvancedSettings2_put_TransportType(This,ptransportType)	\
    ( (This)->lpVtbl -> put_TransportType(This,ptransportType) ) 

#define IMsRdpClientAdvancedSettings2_get_TransportType(This,ptransportType)	\
    ( (This)->lpVtbl -> get_TransportType(This,ptransportType) ) 

#define IMsRdpClientAdvancedSettings2_put_SasSequence(This,psasSequence)	\
    ( (This)->lpVtbl -> put_SasSequence(This,psasSequence) ) 

#define IMsRdpClientAdvancedSettings2_get_SasSequence(This,psasSequence)	\
    ( (This)->lpVtbl -> get_SasSequence(This,psasSequence) ) 

#define IMsRdpClientAdvancedSettings2_put_EncryptionEnabled(This,pencryptionEnabled)	\
    ( (This)->lpVtbl -> put_EncryptionEnabled(This,pencryptionEnabled) ) 

#define IMsRdpClientAdvancedSettings2_get_EncryptionEnabled(This,pencryptionEnabled)	\
    ( (This)->lpVtbl -> get_EncryptionEnabled(This,pencryptionEnabled) ) 

#define IMsRdpClientAdvancedSettings2_put_DedicatedTerminal(This,pdedicatedTerminal)	\
    ( (This)->lpVtbl -> put_DedicatedTerminal(This,pdedicatedTerminal) ) 

#define IMsRdpClientAdvancedSettings2_get_DedicatedTerminal(This,pdedicatedTerminal)	\
    ( (This)->lpVtbl -> get_DedicatedTerminal(This,pdedicatedTerminal) ) 

#define IMsRdpClientAdvancedSettings2_put_RDPPort(This,prdpPort)	\
    ( (This)->lpVtbl -> put_RDPPort(This,prdpPort) ) 

#define IMsRdpClientAdvancedSettings2_get_RDPPort(This,prdpPort)	\
    ( (This)->lpVtbl -> get_RDPPort(This,prdpPort) ) 

#define IMsRdpClientAdvancedSettings2_put_EnableMouse(This,penableMouse)	\
    ( (This)->lpVtbl -> put_EnableMouse(This,penableMouse) ) 

#define IMsRdpClientAdvancedSettings2_get_EnableMouse(This,penableMouse)	\
    ( (This)->lpVtbl -> get_EnableMouse(This,penableMouse) ) 

#define IMsRdpClientAdvancedSettings2_put_DisableCtrlAltDel(This,pdisableCtrlAltDel)	\
    ( (This)->lpVtbl -> put_DisableCtrlAltDel(This,pdisableCtrlAltDel) ) 

#define IMsRdpClientAdvancedSettings2_get_DisableCtrlAltDel(This,pdisableCtrlAltDel)	\
    ( (This)->lpVtbl -> get_DisableCtrlAltDel(This,pdisableCtrlAltDel) ) 

#define IMsRdpClientAdvancedSettings2_put_EnableWindowsKey(This,penableWindowsKey)	\
    ( (This)->lpVtbl -> put_EnableWindowsKey(This,penableWindowsKey) ) 

#define IMsRdpClientAdvancedSettings2_get_EnableWindowsKey(This,penableWindowsKey)	\
    ( (This)->lpVtbl -> get_EnableWindowsKey(This,penableWindowsKey) ) 

#define IMsRdpClientAdvancedSettings2_put_DoubleClickDetect(This,pdoubleClickDetect)	\
    ( (This)->lpVtbl -> put_DoubleClickDetect(This,pdoubleClickDetect) ) 

#define IMsRdpClientAdvancedSettings2_get_DoubleClickDetect(This,pdoubleClickDetect)	\
    ( (This)->lpVtbl -> get_DoubleClickDetect(This,pdoubleClickDetect) ) 

#define IMsRdpClientAdvancedSettings2_put_MaximizeShell(This,pmaximizeShell)	\
    ( (This)->lpVtbl -> put_MaximizeShell(This,pmaximizeShell) ) 

#define IMsRdpClientAdvancedSettings2_get_MaximizeShell(This,pmaximizeShell)	\
    ( (This)->lpVtbl -> get_MaximizeShell(This,pmaximizeShell) ) 

#define IMsRdpClientAdvancedSettings2_put_HotKeyFullScreen(This,photKeyFullScreen)	\
    ( (This)->lpVtbl -> put_HotKeyFullScreen(This,photKeyFullScreen) ) 

#define IMsRdpClientAdvancedSettings2_get_HotKeyFullScreen(This,photKeyFullScreen)	\
    ( (This)->lpVtbl -> get_HotKeyFullScreen(This,photKeyFullScreen) ) 

#define IMsRdpClientAdvancedSettings2_put_HotKeyCtrlEsc(This,photKeyCtrlEsc)	\
    ( (This)->lpVtbl -> put_HotKeyCtrlEsc(This,photKeyCtrlEsc) ) 

#define IMsRdpClientAdvancedSettings2_get_HotKeyCtrlEsc(This,photKeyCtrlEsc)	\
    ( (This)->lpVtbl -> get_HotKeyCtrlEsc(This,photKeyCtrlEsc) ) 

#define IMsRdpClientAdvancedSettings2_put_HotKeyAltEsc(This,photKeyAltEsc)	\
    ( (This)->lpVtbl -> put_HotKeyAltEsc(This,photKeyAltEsc) ) 

#define IMsRdpClientAdvancedSettings2_get_HotKeyAltEsc(This,photKeyAltEsc)	\
    ( (This)->lpVtbl -> get_HotKeyAltEsc(This,photKeyAltEsc) ) 

#define IMsRdpClientAdvancedSettings2_put_HotKeyAltTab(This,photKeyAltTab)	\
    ( (This)->lpVtbl -> put_HotKeyAltTab(This,photKeyAltTab) ) 

#define IMsRdpClientAdvancedSettings2_get_HotKeyAltTab(This,photKeyAltTab)	\
    ( (This)->lpVtbl -> get_HotKeyAltTab(This,photKeyAltTab) ) 

#define IMsRdpClientAdvancedSettings2_put_HotKeyAltShiftTab(This,photKeyAltShiftTab)	\
    ( (This)->lpVtbl -> put_HotKeyAltShiftTab(This,photKeyAltShiftTab) ) 

#define IMsRdpClientAdvancedSettings2_get_HotKeyAltShiftTab(This,photKeyAltShiftTab)	\
    ( (This)->lpVtbl -> get_HotKeyAltShiftTab(This,photKeyAltShiftTab) ) 

#define IMsRdpClientAdvancedSettings2_put_HotKeyAltSpace(This,photKeyAltSpace)	\
    ( (This)->lpVtbl -> put_HotKeyAltSpace(This,photKeyAltSpace) ) 

#define IMsRdpClientAdvancedSettings2_get_HotKeyAltSpace(This,photKeyAltSpace)	\
    ( (This)->lpVtbl -> get_HotKeyAltSpace(This,photKeyAltSpace) ) 

#define IMsRdpClientAdvancedSettings2_put_HotKeyCtrlAltDel(This,photKeyCtrlAltDel)	\
    ( (This)->lpVtbl -> put_HotKeyCtrlAltDel(This,photKeyCtrlAltDel) ) 

#define IMsRdpClientAdvancedSettings2_get_HotKeyCtrlAltDel(This,photKeyCtrlAltDel)	\
    ( (This)->lpVtbl -> get_HotKeyCtrlAltDel(This,photKeyCtrlAltDel) ) 

#define IMsRdpClientAdvancedSettings2_put_orderDrawThreshold(This,porderDrawThreshold)	\
    ( (This)->lpVtbl -> put_orderDrawThreshold(This,porderDrawThreshold) ) 

#define IMsRdpClientAdvancedSettings2_get_orderDrawThreshold(This,porderDrawThreshold)	\
    ( (This)->lpVtbl -> get_orderDrawThreshold(This,porderDrawThreshold) ) 

#define IMsRdpClientAdvancedSettings2_put_BitmapCacheSize(This,pbitmapCacheSize)	\
    ( (This)->lpVtbl -> put_BitmapCacheSize(This,pbitmapCacheSize) ) 

#define IMsRdpClientAdvancedSettings2_get_BitmapCacheSize(This,pbitmapCacheSize)	\
    ( (This)->lpVtbl -> get_BitmapCacheSize(This,pbitmapCacheSize) ) 

#define IMsRdpClientAdvancedSettings2_put_BitmapVirtualCacheSize(This,pbitmapVirtualCacheSize)	\
    ( (This)->lpVtbl -> put_BitmapVirtualCacheSize(This,pbitmapVirtualCacheSize) ) 

#define IMsRdpClientAdvancedSettings2_get_BitmapVirtualCacheSize(This,pbitmapVirtualCacheSize)	\
    ( (This)->lpVtbl -> get_BitmapVirtualCacheSize(This,pbitmapVirtualCacheSize) ) 

#define IMsRdpClientAdvancedSettings2_put_ScaleBitmapCachesByBPP(This,pbScale)	\
    ( (This)->lpVtbl -> put_ScaleBitmapCachesByBPP(This,pbScale) ) 

#define IMsRdpClientAdvancedSettings2_get_ScaleBitmapCachesByBPP(This,pbScale)	\
    ( (This)->lpVtbl -> get_ScaleBitmapCachesByBPP(This,pbScale) ) 

#define IMsRdpClientAdvancedSettings2_put_NumBitmapCaches(This,pnumBitmapCaches)	\
    ( (This)->lpVtbl -> put_NumBitmapCaches(This,pnumBitmapCaches) ) 

#define IMsRdpClientAdvancedSettings2_get_NumBitmapCaches(This,pnumBitmapCaches)	\
    ( (This)->lpVtbl -> get_NumBitmapCaches(This,pnumBitmapCaches) ) 

#define IMsRdpClientAdvancedSettings2_put_CachePersistenceActive(This,pcachePersistenceActive)	\
    ( (This)->lpVtbl -> put_CachePersistenceActive(This,pcachePersistenceActive) ) 

#define IMsRdpClientAdvancedSettings2_get_CachePersistenceActive(This,pcachePersistenceActive)	\
    ( (This)->lpVtbl -> get_CachePersistenceActive(This,pcachePersistenceActive) ) 

#define IMsRdpClientAdvancedSettings2_put_PersistCacheDirectory(This,rhs)	\
    ( (This)->lpVtbl -> put_PersistCacheDirectory(This,rhs) ) 

#define IMsRdpClientAdvancedSettings2_put_brushSupportLevel(This,pbrushSupportLevel)	\
    ( (This)->lpVtbl -> put_brushSupportLevel(This,pbrushSupportLevel) ) 

#define IMsRdpClientAdvancedSettings2_get_brushSupportLevel(This,pbrushSupportLevel)	\
    ( (This)->lpVtbl -> get_brushSupportLevel(This,pbrushSupportLevel) ) 

#define IMsRdpClientAdvancedSettings2_put_minInputSendInterval(This,pminInputSendInterval)	\
    ( (This)->lpVtbl -> put_minInputSendInterval(This,pminInputSendInterval) ) 

#define IMsRdpClientAdvancedSettings2_get_minInputSendInterval(This,pminInputSendInterval)	\
    ( (This)->lpVtbl -> get_minInputSendInterval(This,pminInputSendInterval) ) 

#define IMsRdpClientAdvancedSettings2_put_InputEventsAtOnce(This,pinputEventsAtOnce)	\
    ( (This)->lpVtbl -> put_InputEventsAtOnce(This,pinputEventsAtOnce) ) 

#define IMsRdpClientAdvancedSettings2_get_InputEventsAtOnce(This,pinputEventsAtOnce)	\
    ( (This)->lpVtbl -> get_InputEventsAtOnce(This,pinputEventsAtOnce) ) 

#define IMsRdpClientAdvancedSettings2_put_maxEventCount(This,pmaxEventCount)	\
    ( (This)->lpVtbl -> put_maxEventCount(This,pmaxEventCount) ) 

#define IMsRdpClientAdvancedSettings2_get_maxEventCount(This,pmaxEventCount)	\
    ( (This)->lpVtbl -> get_maxEventCount(This,pmaxEventCount) ) 

#define IMsRdpClientAdvancedSettings2_put_keepAliveInterval(This,pkeepAliveInterval)	\
    ( (This)->lpVtbl -> put_keepAliveInterval(This,pkeepAliveInterval) ) 

#define IMsRdpClientAdvancedSettings2_get_keepAliveInterval(This,pkeepAliveInterval)	\
    ( (This)->lpVtbl -> get_keepAliveInterval(This,pkeepAliveInterval) ) 

#define IMsRdpClientAdvancedSettings2_put_shutdownTimeout(This,pshutdownTimeout)	\
    ( (This)->lpVtbl -> put_shutdownTimeout(This,pshutdownTimeout) ) 

#define IMsRdpClientAdvancedSettings2_get_shutdownTimeout(This,pshutdownTimeout)	\
    ( (This)->lpVtbl -> get_shutdownTimeout(This,pshutdownTimeout) ) 

#define IMsRdpClientAdvancedSettings2_put_overallConnectionTimeout(This,poverallConnectionTimeout)	\
    ( (This)->lpVtbl -> put_overallConnectionTimeout(This,poverallConnectionTimeout) ) 

#define IMsRdpClientAdvancedSettings2_get_overallConnectionTimeout(This,poverallConnectionTimeout)	\
    ( (This)->lpVtbl -> get_overallConnectionTimeout(This,poverallConnectionTimeout) ) 

#define IMsRdpClientAdvancedSettings2_put_singleConnectionTimeout(This,psingleConnectionTimeout)	\
    ( (This)->lpVtbl -> put_singleConnectionTimeout(This,psingleConnectionTimeout) ) 

#define IMsRdpClientAdvancedSettings2_get_singleConnectionTimeout(This,psingleConnectionTimeout)	\
    ( (This)->lpVtbl -> get_singleConnectionTimeout(This,psingleConnectionTimeout) ) 

#define IMsRdpClientAdvancedSettings2_put_KeyboardType(This,pkeyboardType)	\
    ( (This)->lpVtbl -> put_KeyboardType(This,pkeyboardType) ) 

#define IMsRdpClientAdvancedSettings2_get_KeyboardType(This,pkeyboardType)	\
    ( (This)->lpVtbl -> get_KeyboardType(This,pkeyboardType) ) 

#define IMsRdpClientAdvancedSettings2_put_KeyboardSubType(This,pkeyboardSubType)	\
    ( (This)->lpVtbl -> put_KeyboardSubType(This,pkeyboardSubType) ) 

#define IMsRdpClientAdvancedSettings2_get_KeyboardSubType(This,pkeyboardSubType)	\
    ( (This)->lpVtbl -> get_KeyboardSubType(This,pkeyboardSubType) ) 

#define IMsRdpClientAdvancedSettings2_put_KeyboardFunctionKey(This,pkeyboardFunctionKey)	\
    ( (This)->lpVtbl -> put_KeyboardFunctionKey(This,pkeyboardFunctionKey) ) 

#define IMsRdpClientAdvancedSettings2_get_KeyboardFunctionKey(This,pkeyboardFunctionKey)	\
    ( (This)->lpVtbl -> get_KeyboardFunctionKey(This,pkeyboardFunctionKey) ) 

#define IMsRdpClientAdvancedSettings2_put_WinceFixedPalette(This,pwinceFixedPalette)	\
    ( (This)->lpVtbl -> put_WinceFixedPalette(This,pwinceFixedPalette) ) 

#define IMsRdpClientAdvancedSettings2_get_WinceFixedPalette(This,pwinceFixedPalette)	\
    ( (This)->lpVtbl -> get_WinceFixedPalette(This,pwinceFixedPalette) ) 

#define IMsRdpClientAdvancedSettings2_put_ConnectToServerConsole(This,pConnectToConsole)	\
    ( (This)->lpVtbl -> put_ConnectToServerConsole(This,pConnectToConsole) ) 

#define IMsRdpClientAdvancedSettings2_get_ConnectToServerConsole(This,pConnectToConsole)	\
    ( (This)->lpVtbl -> get_ConnectToServerConsole(This,pConnectToConsole) ) 

#define IMsRdpClientAdvancedSettings2_put_BitmapPersistence(This,pbitmapPersistence)	\
    ( (This)->lpVtbl -> put_BitmapPersistence(This,pbitmapPersistence) ) 

#define IMsRdpClientAdvancedSettings2_get_BitmapPersistence(This,pbitmapPersistence)	\
    ( (This)->lpVtbl -> get_BitmapPersistence(This,pbitmapPersistence) ) 

#define IMsRdpClientAdvancedSettings2_put_MinutesToIdleTimeout(This,pminutesToIdleTimeout)	\
    ( (This)->lpVtbl -> put_MinutesToIdleTimeout(This,pminutesToIdleTimeout) ) 

#define IMsRdpClientAdvancedSettings2_get_MinutesToIdleTimeout(This,pminutesToIdleTimeout)	\
    ( (This)->lpVtbl -> get_MinutesToIdleTimeout(This,pminutesToIdleTimeout) ) 

#define IMsRdpClientAdvancedSettings2_put_SmartSizing(This,pfSmartSizing)	\
    ( (This)->lpVtbl -> put_SmartSizing(This,pfSmartSizing) ) 

#define IMsRdpClientAdvancedSettings2_get_SmartSizing(This,pfSmartSizing)	\
    ( (This)->lpVtbl -> get_SmartSizing(This,pfSmartSizing) ) 

#define IMsRdpClientAdvancedSettings2_put_RdpdrLocalPrintingDocName(This,pLocalPrintingDocName)	\
    ( (This)->lpVtbl -> put_RdpdrLocalPrintingDocName(This,pLocalPrintingDocName) ) 

#define IMsRdpClientAdvancedSettings2_get_RdpdrLocalPrintingDocName(This,pLocalPrintingDocName)	\
    ( (This)->lpVtbl -> get_RdpdrLocalPrintingDocName(This,pLocalPrintingDocName) ) 

#define IMsRdpClientAdvancedSettings2_put_RdpdrClipCleanTempDirString(This,clipCleanTempDirString)	\
    ( (This)->lpVtbl -> put_RdpdrClipCleanTempDirString(This,clipCleanTempDirString) ) 

#define IMsRdpClientAdvancedSettings2_get_RdpdrClipCleanTempDirString(This,clipCleanTempDirString)	\
    ( (This)->lpVtbl -> get_RdpdrClipCleanTempDirString(This,clipCleanTempDirString) ) 

#define IMsRdpClientAdvancedSettings2_put_RdpdrClipPasteInfoString(This,clipPasteInfoString)	\
    ( (This)->lpVtbl -> put_RdpdrClipPasteInfoString(This,clipPasteInfoString) ) 

#define IMsRdpClientAdvancedSettings2_get_RdpdrClipPasteInfoString(This,clipPasteInfoString)	\
    ( (This)->lpVtbl -> get_RdpdrClipPasteInfoString(This,clipPasteInfoString) ) 

#define IMsRdpClientAdvancedSettings2_put_ClearTextPassword(This,rhs)	\
    ( (This)->lpVtbl -> put_ClearTextPassword(This,rhs) ) 

#define IMsRdpClientAdvancedSettings2_put_DisplayConnectionBar(This,pDisplayConnectionBar)	\
    ( (This)->lpVtbl -> put_DisplayConnectionBar(This,pDisplayConnectionBar) ) 

#define IMsRdpClientAdvancedSettings2_get_DisplayConnectionBar(This,pDisplayConnectionBar)	\
    ( (This)->lpVtbl -> get_DisplayConnectionBar(This,pDisplayConnectionBar) ) 

#define IMsRdpClientAdvancedSettings2_put_PinConnectionBar(This,pPinConnectionBar)	\
    ( (This)->lpVtbl -> put_PinConnectionBar(This,pPinConnectionBar) ) 

#define IMsRdpClientAdvancedSettings2_get_PinConnectionBar(This,pPinConnectionBar)	\
    ( (This)->lpVtbl -> get_PinConnectionBar(This,pPinConnectionBar) ) 

#define IMsRdpClientAdvancedSettings2_put_GrabFocusOnConnect(This,pfGrabFocusOnConnect)	\
    ( (This)->lpVtbl -> put_GrabFocusOnConnect(This,pfGrabFocusOnConnect) ) 

#define IMsRdpClientAdvancedSettings2_get_GrabFocusOnConnect(This,pfGrabFocusOnConnect)	\
    ( (This)->lpVtbl -> get_GrabFocusOnConnect(This,pfGrabFocusOnConnect) ) 

#define IMsRdpClientAdvancedSettings2_put_LoadBalanceInfo(This,pLBInfo)	\
    ( (This)->lpVtbl -> put_LoadBalanceInfo(This,pLBInfo) ) 

#define IMsRdpClientAdvancedSettings2_get_LoadBalanceInfo(This,pLBInfo)	\
    ( (This)->lpVtbl -> get_LoadBalanceInfo(This,pLBInfo) ) 

#define IMsRdpClientAdvancedSettings2_put_RedirectDrives(This,pRedirectDrives)	\
    ( (This)->lpVtbl -> put_RedirectDrives(This,pRedirectDrives) ) 

#define IMsRdpClientAdvancedSettings2_get_RedirectDrives(This,pRedirectDrives)	\
    ( (This)->lpVtbl -> get_RedirectDrives(This,pRedirectDrives) ) 

#define IMsRdpClientAdvancedSettings2_put_RedirectPrinters(This,pRedirectPrinters)	\
    ( (This)->lpVtbl -> put_RedirectPrinters(This,pRedirectPrinters) ) 

#define IMsRdpClientAdvancedSettings2_get_RedirectPrinters(This,pRedirectPrinters)	\
    ( (This)->lpVtbl -> get_RedirectPrinters(This,pRedirectPrinters) ) 

#define IMsRdpClientAdvancedSettings2_put_RedirectPorts(This,pRedirectPorts)	\
    ( (This)->lpVtbl -> put_RedirectPorts(This,pRedirectPorts) ) 

#define IMsRdpClientAdvancedSettings2_get_RedirectPorts(This,pRedirectPorts)	\
    ( (This)->lpVtbl -> get_RedirectPorts(This,pRedirectPorts) ) 

#define IMsRdpClientAdvancedSettings2_put_RedirectSmartCards(This,pRedirectSmartCards)	\
    ( (This)->lpVtbl -> put_RedirectSmartCards(This,pRedirectSmartCards) ) 

#define IMsRdpClientAdvancedSettings2_get_RedirectSmartCards(This,pRedirectSmartCards)	\
    ( (This)->lpVtbl -> get_RedirectSmartCards(This,pRedirectSmartCards) ) 

#define IMsRdpClientAdvancedSettings2_put_BitmapVirtualCache16BppSize(This,pBitmapVirtualCache16BppSize)	\
    ( (This)->lpVtbl -> put_BitmapVirtualCache16BppSize(This,pBitmapVirtualCache16BppSize) ) 

#define IMsRdpClientAdvancedSettings2_get_BitmapVirtualCache16BppSize(This,pBitmapVirtualCache16BppSize)	\
    ( (This)->lpVtbl -> get_BitmapVirtualCache16BppSize(This,pBitmapVirtualCache16BppSize) ) 

#define IMsRdpClientAdvancedSettings2_put_BitmapVirtualCache24BppSize(This,pBitmapVirtualCache24BppSize)	\
    ( (This)->lpVtbl -> put_BitmapVirtualCache24BppSize(This,pBitmapVirtualCache24BppSize) ) 

#define IMsRdpClientAdvancedSettings2_get_BitmapVirtualCache24BppSize(This,pBitmapVirtualCache24BppSize)	\
    ( (This)->lpVtbl -> get_BitmapVirtualCache24BppSize(This,pBitmapVirtualCache24BppSize) ) 

#define IMsRdpClientAdvancedSettings2_put_PerformanceFlags(This,pDisableList)	\
    ( (This)->lpVtbl -> put_PerformanceFlags(This,pDisableList) ) 

#define IMsRdpClientAdvancedSettings2_get_PerformanceFlags(This,pDisableList)	\
    ( (This)->lpVtbl -> get_PerformanceFlags(This,pDisableList) ) 

#define IMsRdpClientAdvancedSettings2_put_ConnectWithEndpoint(This,rhs)	\
    ( (This)->lpVtbl -> put_ConnectWithEndpoint(This,rhs) ) 

#define IMsRdpClientAdvancedSettings2_put_NotifyTSPublicKey(This,pfNotify)	\
    ( (This)->lpVtbl -> put_NotifyTSPublicKey(This,pfNotify) ) 

#define IMsRdpClientAdvancedSettings2_get_NotifyTSPublicKey(This,pfNotify)	\
    ( (This)->lpVtbl -> get_NotifyTSPublicKey(This,pfNotify) ) 


#define IMsRdpClientAdvancedSettings2_get_CanAutoReconnect(This,pfCanAutoReconnect)	\
    ( (This)->lpVtbl -> get_CanAutoReconnect(This,pfCanAutoReconnect) ) 

#define IMsRdpClientAdvancedSettings2_put_EnableAutoReconnect(This,pfEnableAutoReconnect)	\
    ( (This)->lpVtbl -> put_EnableAutoReconnect(This,pfEnableAutoReconnect) ) 

#define IMsRdpClientAdvancedSettings2_get_EnableAutoReconnect(This,pfEnableAutoReconnect)	\
    ( (This)->lpVtbl -> get_EnableAutoReconnect(This,pfEnableAutoReconnect) ) 

#define IMsRdpClientAdvancedSettings2_put_MaxReconnectAttempts(This,pMaxReconnectAttempts)	\
    ( (This)->lpVtbl -> put_MaxReconnectAttempts(This,pMaxReconnectAttempts) ) 

#define IMsRdpClientAdvancedSettings2_get_MaxReconnectAttempts(This,pMaxReconnectAttempts)	\
    ( (This)->lpVtbl -> get_MaxReconnectAttempts(This,pMaxReconnectAttempts) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget][id] */ HRESULT STDMETHODCALLTYPE IMsRdpClientAdvancedSettings2_get_CanAutoReconnect_Proxy( 
    IMsRdpClientAdvancedSettings2 * This,
    /* [retval][out] */ VARIANT_BOOL *pfCanAutoReconnect);


void __RPC_STUB IMsRdpClientAdvancedSettings2_get_CanAutoReconnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMsRdpClientAdvancedSettings2_put_EnableAutoReconnect_Proxy( 
    IMsRdpClientAdvancedSettings2 * This,
    /* [in] */ VARIANT_BOOL pfEnableAutoReconnect);


void __RPC_STUB IMsRdpClientAdvancedSettings2_put_EnableAutoReconnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IMsRdpClientAdvancedSettings2_get_EnableAutoReconnect_Proxy( 
    IMsRdpClientAdvancedSettings2 * This,
    /* [retval][out] */ VARIANT_BOOL *pfEnableAutoReconnect);


void __RPC_STUB IMsRdpClientAdvancedSettings2_get_EnableAutoReconnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMsRdpClientAdvancedSettings2_put_MaxReconnectAttempts_Proxy( 
    IMsRdpClientAdvancedSettings2 * This,
    /* [in] */ long pMaxReconnectAttempts);


void __RPC_STUB IMsRdpClientAdvancedSettings2_put_MaxReconnectAttempts_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IMsRdpClientAdvancedSettings2_get_MaxReconnectAttempts_Proxy( 
    IMsRdpClientAdvancedSettings2 * This,
    /* [retval][out] */ long *pMaxReconnectAttempts);


void __RPC_STUB IMsRdpClientAdvancedSettings2_get_MaxReconnectAttempts_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMsRdpClientAdvancedSettings2_INTERFACE_DEFINED__ */


#ifndef __IMsRdpClientAdvancedSettings3_INTERFACE_DEFINED__
#define __IMsRdpClientAdvancedSettings3_INTERFACE_DEFINED__

/* interface IMsRdpClientAdvancedSettings3 */
/* [object][oleautomation][dual][uuid] */ 


EXTERN_C const IID IID_IMsRdpClientAdvancedSettings3;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("19CD856B-C542-4C53-ACEE-F127E3BE1A59")
    IMsRdpClientAdvancedSettings3 : public IMsRdpClientAdvancedSettings2
    {
    public:
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_ConnectionBarShowMinimizeButton( 
            /* [in] */ VARIANT_BOOL pfShowMinimize) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_ConnectionBarShowMinimizeButton( 
            /* [retval][out] */ VARIANT_BOOL *pfShowMinimize) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_ConnectionBarShowRestoreButton( 
            /* [in] */ VARIANT_BOOL pfShowRestore) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_ConnectionBarShowRestoreButton( 
            /* [retval][out] */ VARIANT_BOOL *pfShowRestore) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMsRdpClientAdvancedSettings3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMsRdpClientAdvancedSettings3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMsRdpClientAdvancedSettings3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Compress )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pcompress);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Compress )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pcompress);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_BitmapPeristence )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pbitmapPeristence);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BitmapPeristence )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pbitmapPeristence);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_allowBackgroundInput )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pallowBackgroundInput);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_allowBackgroundInput )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pallowBackgroundInput);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_KeyBoardLayoutStr )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ BSTR rhs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_PluginDlls )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ BSTR rhs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_IconFile )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ BSTR rhs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_IconIndex )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long rhs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ContainerHandledFullScreen )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pContainerHandledFullScreen);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ContainerHandledFullScreen )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pContainerHandledFullScreen);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DisableRdpdr )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pDisableRdpdr);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DisableRdpdr )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pDisableRdpdr);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_SmoothScroll )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long psmoothScroll);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SmoothScroll )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *psmoothScroll);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_AcceleratorPassthrough )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pacceleratorPassthrough);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AcceleratorPassthrough )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pacceleratorPassthrough);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ShadowBitmap )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pshadowBitmap);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ShadowBitmap )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pshadowBitmap);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_TransportType )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long ptransportType);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_TransportType )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *ptransportType);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_SasSequence )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long psasSequence);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SasSequence )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *psasSequence);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_EncryptionEnabled )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pencryptionEnabled);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_EncryptionEnabled )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pencryptionEnabled);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DedicatedTerminal )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pdedicatedTerminal);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DedicatedTerminal )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pdedicatedTerminal);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RDPPort )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long prdpPort);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RDPPort )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *prdpPort);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_EnableMouse )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long penableMouse);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_EnableMouse )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *penableMouse);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DisableCtrlAltDel )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pdisableCtrlAltDel);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DisableCtrlAltDel )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pdisableCtrlAltDel);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_EnableWindowsKey )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long penableWindowsKey);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_EnableWindowsKey )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *penableWindowsKey);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DoubleClickDetect )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pdoubleClickDetect);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DoubleClickDetect )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pdoubleClickDetect);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_MaximizeShell )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pmaximizeShell);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_MaximizeShell )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pmaximizeShell);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_HotKeyFullScreen )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long photKeyFullScreen);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HotKeyFullScreen )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *photKeyFullScreen);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_HotKeyCtrlEsc )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long photKeyCtrlEsc);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HotKeyCtrlEsc )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *photKeyCtrlEsc);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_HotKeyAltEsc )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long photKeyAltEsc);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HotKeyAltEsc )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *photKeyAltEsc);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_HotKeyAltTab )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long photKeyAltTab);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HotKeyAltTab )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *photKeyAltTab);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_HotKeyAltShiftTab )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long photKeyAltShiftTab);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HotKeyAltShiftTab )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *photKeyAltShiftTab);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_HotKeyAltSpace )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long photKeyAltSpace);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HotKeyAltSpace )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *photKeyAltSpace);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_HotKeyCtrlAltDel )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long photKeyCtrlAltDel);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HotKeyCtrlAltDel )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *photKeyCtrlAltDel);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_orderDrawThreshold )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long porderDrawThreshold);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_orderDrawThreshold )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *porderDrawThreshold);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_BitmapCacheSize )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pbitmapCacheSize);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BitmapCacheSize )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pbitmapCacheSize);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_BitmapVirtualCacheSize )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pbitmapVirtualCacheSize);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BitmapVirtualCacheSize )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pbitmapVirtualCacheSize);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ScaleBitmapCachesByBPP )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pbScale);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ScaleBitmapCachesByBPP )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pbScale);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_NumBitmapCaches )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pnumBitmapCaches);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_NumBitmapCaches )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pnumBitmapCaches);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_CachePersistenceActive )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pcachePersistenceActive);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CachePersistenceActive )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pcachePersistenceActive);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_PersistCacheDirectory )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ BSTR rhs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_brushSupportLevel )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pbrushSupportLevel);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_brushSupportLevel )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pbrushSupportLevel);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_minInputSendInterval )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pminInputSendInterval);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_minInputSendInterval )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pminInputSendInterval);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_InputEventsAtOnce )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pinputEventsAtOnce);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_InputEventsAtOnce )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pinputEventsAtOnce);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_maxEventCount )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pmaxEventCount);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_maxEventCount )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pmaxEventCount);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_keepAliveInterval )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pkeepAliveInterval);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_keepAliveInterval )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pkeepAliveInterval);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_shutdownTimeout )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pshutdownTimeout);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_shutdownTimeout )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pshutdownTimeout);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_overallConnectionTimeout )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long poverallConnectionTimeout);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_overallConnectionTimeout )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *poverallConnectionTimeout);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_singleConnectionTimeout )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long psingleConnectionTimeout);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_singleConnectionTimeout )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *psingleConnectionTimeout);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_KeyboardType )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pkeyboardType);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_KeyboardType )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pkeyboardType);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_KeyboardSubType )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pkeyboardSubType);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_KeyboardSubType )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pkeyboardSubType);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_KeyboardFunctionKey )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pkeyboardFunctionKey);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_KeyboardFunctionKey )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pkeyboardFunctionKey);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_WinceFixedPalette )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pwinceFixedPalette);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_WinceFixedPalette )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pwinceFixedPalette);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ConnectToServerConsole )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ VARIANT_BOOL pConnectToConsole);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ConnectToServerConsole )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ VARIANT_BOOL *pConnectToConsole);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_BitmapPersistence )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pbitmapPersistence);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BitmapPersistence )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pbitmapPersistence);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_MinutesToIdleTimeout )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pminutesToIdleTimeout);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_MinutesToIdleTimeout )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pminutesToIdleTimeout);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_SmartSizing )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ VARIANT_BOOL pfSmartSizing);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SmartSizing )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ VARIANT_BOOL *pfSmartSizing);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RdpdrLocalPrintingDocName )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ BSTR pLocalPrintingDocName);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RdpdrLocalPrintingDocName )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ BSTR *pLocalPrintingDocName);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RdpdrClipCleanTempDirString )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ BSTR clipCleanTempDirString);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RdpdrClipCleanTempDirString )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ BSTR *clipCleanTempDirString);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RdpdrClipPasteInfoString )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ BSTR clipPasteInfoString);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RdpdrClipPasteInfoString )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ BSTR *clipPasteInfoString);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ClearTextPassword )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ BSTR rhs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DisplayConnectionBar )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ VARIANT_BOOL pDisplayConnectionBar);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DisplayConnectionBar )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ VARIANT_BOOL *pDisplayConnectionBar);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_PinConnectionBar )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ VARIANT_BOOL pPinConnectionBar);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_PinConnectionBar )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ VARIANT_BOOL *pPinConnectionBar);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_GrabFocusOnConnect )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ VARIANT_BOOL pfGrabFocusOnConnect);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_GrabFocusOnConnect )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ VARIANT_BOOL *pfGrabFocusOnConnect);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_LoadBalanceInfo )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ BSTR pLBInfo);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_LoadBalanceInfo )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ BSTR *pLBInfo);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RedirectDrives )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ VARIANT_BOOL pRedirectDrives);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RedirectDrives )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ VARIANT_BOOL *pRedirectDrives);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RedirectPrinters )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ VARIANT_BOOL pRedirectPrinters);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RedirectPrinters )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ VARIANT_BOOL *pRedirectPrinters);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RedirectPorts )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ VARIANT_BOOL pRedirectPorts);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RedirectPorts )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ VARIANT_BOOL *pRedirectPorts);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RedirectSmartCards )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ VARIANT_BOOL pRedirectSmartCards);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RedirectSmartCards )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ VARIANT_BOOL *pRedirectSmartCards);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_BitmapVirtualCache16BppSize )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pBitmapVirtualCache16BppSize);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BitmapVirtualCache16BppSize )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pBitmapVirtualCache16BppSize);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_BitmapVirtualCache24BppSize )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pBitmapVirtualCache24BppSize);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BitmapVirtualCache24BppSize )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pBitmapVirtualCache24BppSize);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_PerformanceFlags )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pDisableList);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_PerformanceFlags )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pDisableList);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ConnectWithEndpoint )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ VARIANT *rhs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_NotifyTSPublicKey )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ VARIANT_BOOL pfNotify);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_NotifyTSPublicKey )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ VARIANT_BOOL *pfNotify);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CanAutoReconnect )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ VARIANT_BOOL *pfCanAutoReconnect);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_EnableAutoReconnect )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ VARIANT_BOOL pfEnableAutoReconnect);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_EnableAutoReconnect )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ VARIANT_BOOL *pfEnableAutoReconnect);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_MaxReconnectAttempts )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ long pMaxReconnectAttempts);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_MaxReconnectAttempts )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ long *pMaxReconnectAttempts);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ConnectionBarShowMinimizeButton )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ VARIANT_BOOL pfShowMinimize);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ConnectionBarShowMinimizeButton )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ VARIANT_BOOL *pfShowMinimize);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ConnectionBarShowRestoreButton )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [in] */ VARIANT_BOOL pfShowRestore);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ConnectionBarShowRestoreButton )( 
            IMsRdpClientAdvancedSettings3 * This,
            /* [retval][out] */ VARIANT_BOOL *pfShowRestore);
        
        END_INTERFACE
    } IMsRdpClientAdvancedSettings3Vtbl;

    interface IMsRdpClientAdvancedSettings3
    {
        CONST_VTBL struct IMsRdpClientAdvancedSettings3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMsRdpClientAdvancedSettings3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMsRdpClientAdvancedSettings3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMsRdpClientAdvancedSettings3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMsRdpClientAdvancedSettings3_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IMsRdpClientAdvancedSettings3_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IMsRdpClientAdvancedSettings3_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IMsRdpClientAdvancedSettings3_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IMsRdpClientAdvancedSettings3_put_Compress(This,pcompress)	\
    ( (This)->lpVtbl -> put_Compress(This,pcompress) ) 

#define IMsRdpClientAdvancedSettings3_get_Compress(This,pcompress)	\
    ( (This)->lpVtbl -> get_Compress(This,pcompress) ) 

#define IMsRdpClientAdvancedSettings3_put_BitmapPeristence(This,pbitmapPeristence)	\
    ( (This)->lpVtbl -> put_BitmapPeristence(This,pbitmapPeristence) ) 

#define IMsRdpClientAdvancedSettings3_get_BitmapPeristence(This,pbitmapPeristence)	\
    ( (This)->lpVtbl -> get_BitmapPeristence(This,pbitmapPeristence) ) 

#define IMsRdpClientAdvancedSettings3_put_allowBackgroundInput(This,pallowBackgroundInput)	\
    ( (This)->lpVtbl -> put_allowBackgroundInput(This,pallowBackgroundInput) ) 

#define IMsRdpClientAdvancedSettings3_get_allowBackgroundInput(This,pallowBackgroundInput)	\
    ( (This)->lpVtbl -> get_allowBackgroundInput(This,pallowBackgroundInput) ) 

#define IMsRdpClientAdvancedSettings3_put_KeyBoardLayoutStr(This,rhs)	\
    ( (This)->lpVtbl -> put_KeyBoardLayoutStr(This,rhs) ) 

#define IMsRdpClientAdvancedSettings3_put_PluginDlls(This,rhs)	\
    ( (This)->lpVtbl -> put_PluginDlls(This,rhs) ) 

#define IMsRdpClientAdvancedSettings3_put_IconFile(This,rhs)	\
    ( (This)->lpVtbl -> put_IconFile(This,rhs) ) 

#define IMsRdpClientAdvancedSettings3_put_IconIndex(This,rhs)	\
    ( (This)->lpVtbl -> put_IconIndex(This,rhs) ) 

#define IMsRdpClientAdvancedSettings3_put_ContainerHandledFullScreen(This,pContainerHandledFullScreen)	\
    ( (This)->lpVtbl -> put_ContainerHandledFullScreen(This,pContainerHandledFullScreen) ) 

#define IMsRdpClientAdvancedSettings3_get_ContainerHandledFullScreen(This,pContainerHandledFullScreen)	\
    ( (This)->lpVtbl -> get_ContainerHandledFullScreen(This,pContainerHandledFullScreen) ) 

#define IMsRdpClientAdvancedSettings3_put_DisableRdpdr(This,pDisableRdpdr)	\
    ( (This)->lpVtbl -> put_DisableRdpdr(This,pDisableRdpdr) ) 

#define IMsRdpClientAdvancedSettings3_get_DisableRdpdr(This,pDisableRdpdr)	\
    ( (This)->lpVtbl -> get_DisableRdpdr(This,pDisableRdpdr) ) 


#define IMsRdpClientAdvancedSettings3_put_SmoothScroll(This,psmoothScroll)	\
    ( (This)->lpVtbl -> put_SmoothScroll(This,psmoothScroll) ) 

#define IMsRdpClientAdvancedSettings3_get_SmoothScroll(This,psmoothScroll)	\
    ( (This)->lpVtbl -> get_SmoothScroll(This,psmoothScroll) ) 

#define IMsRdpClientAdvancedSettings3_put_AcceleratorPassthrough(This,pacceleratorPassthrough)	\
    ( (This)->lpVtbl -> put_AcceleratorPassthrough(This,pacceleratorPassthrough) ) 

#define IMsRdpClientAdvancedSettings3_get_AcceleratorPassthrough(This,pacceleratorPassthrough)	\
    ( (This)->lpVtbl -> get_AcceleratorPassthrough(This,pacceleratorPassthrough) ) 

#define IMsRdpClientAdvancedSettings3_put_ShadowBitmap(This,pshadowBitmap)	\
    ( (This)->lpVtbl -> put_ShadowBitmap(This,pshadowBitmap) ) 

#define IMsRdpClientAdvancedSettings3_get_ShadowBitmap(This,pshadowBitmap)	\
    ( (This)->lpVtbl -> get_ShadowBitmap(This,pshadowBitmap) ) 

#define IMsRdpClientAdvancedSettings3_put_TransportType(This,ptransportType)	\
    ( (This)->lpVtbl -> put_TransportType(This,ptransportType) ) 

#define IMsRdpClientAdvancedSettings3_get_TransportType(This,ptransportType)	\
    ( (This)->lpVtbl -> get_TransportType(This,ptransportType) ) 

#define IMsRdpClientAdvancedSettings3_put_SasSequence(This,psasSequence)	\
    ( (This)->lpVtbl -> put_SasSequence(This,psasSequence) ) 

#define IMsRdpClientAdvancedSettings3_get_SasSequence(This,psasSequence)	\
    ( (This)->lpVtbl -> get_SasSequence(This,psasSequence) ) 

#define IMsRdpClientAdvancedSettings3_put_EncryptionEnabled(This,pencryptionEnabled)	\
    ( (This)->lpVtbl -> put_EncryptionEnabled(This,pencryptionEnabled) ) 

#define IMsRdpClientAdvancedSettings3_get_EncryptionEnabled(This,pencryptionEnabled)	\
    ( (This)->lpVtbl -> get_EncryptionEnabled(This,pencryptionEnabled) ) 

#define IMsRdpClientAdvancedSettings3_put_DedicatedTerminal(This,pdedicatedTerminal)	\
    ( (This)->lpVtbl -> put_DedicatedTerminal(This,pdedicatedTerminal) ) 

#define IMsRdpClientAdvancedSettings3_get_DedicatedTerminal(This,pdedicatedTerminal)	\
    ( (This)->lpVtbl -> get_DedicatedTerminal(This,pdedicatedTerminal) ) 

#define IMsRdpClientAdvancedSettings3_put_RDPPort(This,prdpPort)	\
    ( (This)->lpVtbl -> put_RDPPort(This,prdpPort) ) 

#define IMsRdpClientAdvancedSettings3_get_RDPPort(This,prdpPort)	\
    ( (This)->lpVtbl -> get_RDPPort(This,prdpPort) ) 

#define IMsRdpClientAdvancedSettings3_put_EnableMouse(This,penableMouse)	\
    ( (This)->lpVtbl -> put_EnableMouse(This,penableMouse) ) 

#define IMsRdpClientAdvancedSettings3_get_EnableMouse(This,penableMouse)	\
    ( (This)->lpVtbl -> get_EnableMouse(This,penableMouse) ) 

#define IMsRdpClientAdvancedSettings3_put_DisableCtrlAltDel(This,pdisableCtrlAltDel)	\
    ( (This)->lpVtbl -> put_DisableCtrlAltDel(This,pdisableCtrlAltDel) ) 

#define IMsRdpClientAdvancedSettings3_get_DisableCtrlAltDel(This,pdisableCtrlAltDel)	\
    ( (This)->lpVtbl -> get_DisableCtrlAltDel(This,pdisableCtrlAltDel) ) 

#define IMsRdpClientAdvancedSettings3_put_EnableWindowsKey(This,penableWindowsKey)	\
    ( (This)->lpVtbl -> put_EnableWindowsKey(This,penableWindowsKey) ) 

#define IMsRdpClientAdvancedSettings3_get_EnableWindowsKey(This,penableWindowsKey)	\
    ( (This)->lpVtbl -> get_EnableWindowsKey(This,penableWindowsKey) ) 

#define IMsRdpClientAdvancedSettings3_put_DoubleClickDetect(This,pdoubleClickDetect)	\
    ( (This)->lpVtbl -> put_DoubleClickDetect(This,pdoubleClickDetect) ) 

#define IMsRdpClientAdvancedSettings3_get_DoubleClickDetect(This,pdoubleClickDetect)	\
    ( (This)->lpVtbl -> get_DoubleClickDetect(This,pdoubleClickDetect) ) 

#define IMsRdpClientAdvancedSettings3_put_MaximizeShell(This,pmaximizeShell)	\
    ( (This)->lpVtbl -> put_MaximizeShell(This,pmaximizeShell) ) 

#define IMsRdpClientAdvancedSettings3_get_MaximizeShell(This,pmaximizeShell)	\
    ( (This)->lpVtbl -> get_MaximizeShell(This,pmaximizeShell) ) 

#define IMsRdpClientAdvancedSettings3_put_HotKeyFullScreen(This,photKeyFullScreen)	\
    ( (This)->lpVtbl -> put_HotKeyFullScreen(This,photKeyFullScreen) ) 

#define IMsRdpClientAdvancedSettings3_get_HotKeyFullScreen(This,photKeyFullScreen)	\
    ( (This)->lpVtbl -> get_HotKeyFullScreen(This,photKeyFullScreen) ) 

#define IMsRdpClientAdvancedSettings3_put_HotKeyCtrlEsc(This,photKeyCtrlEsc)	\
    ( (This)->lpVtbl -> put_HotKeyCtrlEsc(This,photKeyCtrlEsc) ) 

#define IMsRdpClientAdvancedSettings3_get_HotKeyCtrlEsc(This,photKeyCtrlEsc)	\
    ( (This)->lpVtbl -> get_HotKeyCtrlEsc(This,photKeyCtrlEsc) ) 

#define IMsRdpClientAdvancedSettings3_put_HotKeyAltEsc(This,photKeyAltEsc)	\
    ( (This)->lpVtbl -> put_HotKeyAltEsc(This,photKeyAltEsc) ) 

#define IMsRdpClientAdvancedSettings3_get_HotKeyAltEsc(This,photKeyAltEsc)	\
    ( (This)->lpVtbl -> get_HotKeyAltEsc(This,photKeyAltEsc) ) 

#define IMsRdpClientAdvancedSettings3_put_HotKeyAltTab(This,photKeyAltTab)	\
    ( (This)->lpVtbl -> put_HotKeyAltTab(This,photKeyAltTab) ) 

#define IMsRdpClientAdvancedSettings3_get_HotKeyAltTab(This,photKeyAltTab)	\
    ( (This)->lpVtbl -> get_HotKeyAltTab(This,photKeyAltTab) ) 

#define IMsRdpClientAdvancedSettings3_put_HotKeyAltShiftTab(This,photKeyAltShiftTab)	\
    ( (This)->lpVtbl -> put_HotKeyAltShiftTab(This,photKeyAltShiftTab) ) 

#define IMsRdpClientAdvancedSettings3_get_HotKeyAltShiftTab(This,photKeyAltShiftTab)	\
    ( (This)->lpVtbl -> get_HotKeyAltShiftTab(This,photKeyAltShiftTab) ) 

#define IMsRdpClientAdvancedSettings3_put_HotKeyAltSpace(This,photKeyAltSpace)	\
    ( (This)->lpVtbl -> put_HotKeyAltSpace(This,photKeyAltSpace) ) 

#define IMsRdpClientAdvancedSettings3_get_HotKeyAltSpace(This,photKeyAltSpace)	\
    ( (This)->lpVtbl -> get_HotKeyAltSpace(This,photKeyAltSpace) ) 

#define IMsRdpClientAdvancedSettings3_put_HotKeyCtrlAltDel(This,photKeyCtrlAltDel)	\
    ( (This)->lpVtbl -> put_HotKeyCtrlAltDel(This,photKeyCtrlAltDel) ) 

#define IMsRdpClientAdvancedSettings3_get_HotKeyCtrlAltDel(This,photKeyCtrlAltDel)	\
    ( (This)->lpVtbl -> get_HotKeyCtrlAltDel(This,photKeyCtrlAltDel) ) 

#define IMsRdpClientAdvancedSettings3_put_orderDrawThreshold(This,porderDrawThreshold)	\
    ( (This)->lpVtbl -> put_orderDrawThreshold(This,porderDrawThreshold) ) 

#define IMsRdpClientAdvancedSettings3_get_orderDrawThreshold(This,porderDrawThreshold)	\
    ( (This)->lpVtbl -> get_orderDrawThreshold(This,porderDrawThreshold) ) 

#define IMsRdpClientAdvancedSettings3_put_BitmapCacheSize(This,pbitmapCacheSize)	\
    ( (This)->lpVtbl -> put_BitmapCacheSize(This,pbitmapCacheSize) ) 

#define IMsRdpClientAdvancedSettings3_get_BitmapCacheSize(This,pbitmapCacheSize)	\
    ( (This)->lpVtbl -> get_BitmapCacheSize(This,pbitmapCacheSize) ) 

#define IMsRdpClientAdvancedSettings3_put_BitmapVirtualCacheSize(This,pbitmapVirtualCacheSize)	\
    ( (This)->lpVtbl -> put_BitmapVirtualCacheSize(This,pbitmapVirtualCacheSize) ) 

#define IMsRdpClientAdvancedSettings3_get_BitmapVirtualCacheSize(This,pbitmapVirtualCacheSize)	\
    ( (This)->lpVtbl -> get_BitmapVirtualCacheSize(This,pbitmapVirtualCacheSize) ) 

#define IMsRdpClientAdvancedSettings3_put_ScaleBitmapCachesByBPP(This,pbScale)	\
    ( (This)->lpVtbl -> put_ScaleBitmapCachesByBPP(This,pbScale) ) 

#define IMsRdpClientAdvancedSettings3_get_ScaleBitmapCachesByBPP(This,pbScale)	\
    ( (This)->lpVtbl -> get_ScaleBitmapCachesByBPP(This,pbScale) ) 

#define IMsRdpClientAdvancedSettings3_put_NumBitmapCaches(This,pnumBitmapCaches)	\
    ( (This)->lpVtbl -> put_NumBitmapCaches(This,pnumBitmapCaches) ) 

#define IMsRdpClientAdvancedSettings3_get_NumBitmapCaches(This,pnumBitmapCaches)	\
    ( (This)->lpVtbl -> get_NumBitmapCaches(This,pnumBitmapCaches) ) 

#define IMsRdpClientAdvancedSettings3_put_CachePersistenceActive(This,pcachePersistenceActive)	\
    ( (This)->lpVtbl -> put_CachePersistenceActive(This,pcachePersistenceActive) ) 

#define IMsRdpClientAdvancedSettings3_get_CachePersistenceActive(This,pcachePersistenceActive)	\
    ( (This)->lpVtbl -> get_CachePersistenceActive(This,pcachePersistenceActive) ) 

#define IMsRdpClientAdvancedSettings3_put_PersistCacheDirectory(This,rhs)	\
    ( (This)->lpVtbl -> put_PersistCacheDirectory(This,rhs) ) 

#define IMsRdpClientAdvancedSettings3_put_brushSupportLevel(This,pbrushSupportLevel)	\
    ( (This)->lpVtbl -> put_brushSupportLevel(This,pbrushSupportLevel) ) 

#define IMsRdpClientAdvancedSettings3_get_brushSupportLevel(This,pbrushSupportLevel)	\
    ( (This)->lpVtbl -> get_brushSupportLevel(This,pbrushSupportLevel) ) 

#define IMsRdpClientAdvancedSettings3_put_minInputSendInterval(This,pminInputSendInterval)	\
    ( (This)->lpVtbl -> put_minInputSendInterval(This,pminInputSendInterval) ) 

#define IMsRdpClientAdvancedSettings3_get_minInputSendInterval(This,pminInputSendInterval)	\
    ( (This)->lpVtbl -> get_minInputSendInterval(This,pminInputSendInterval) ) 

#define IMsRdpClientAdvancedSettings3_put_InputEventsAtOnce(This,pinputEventsAtOnce)	\
    ( (This)->lpVtbl -> put_InputEventsAtOnce(This,pinputEventsAtOnce) ) 

#define IMsRdpClientAdvancedSettings3_get_InputEventsAtOnce(This,pinputEventsAtOnce)	\
    ( (This)->lpVtbl -> get_InputEventsAtOnce(This,pinputEventsAtOnce) ) 

#define IMsRdpClientAdvancedSettings3_put_maxEventCount(This,pmaxEventCount)	\
    ( (This)->lpVtbl -> put_maxEventCount(This,pmaxEventCount) ) 

#define IMsRdpClientAdvancedSettings3_get_maxEventCount(This,pmaxEventCount)	\
    ( (This)->lpVtbl -> get_maxEventCount(This,pmaxEventCount) ) 

#define IMsRdpClientAdvancedSettings3_put_keepAliveInterval(This,pkeepAliveInterval)	\
    ( (This)->lpVtbl -> put_keepAliveInterval(This,pkeepAliveInterval) ) 

#define IMsRdpClientAdvancedSettings3_get_keepAliveInterval(This,pkeepAliveInterval)	\
    ( (This)->lpVtbl -> get_keepAliveInterval(This,pkeepAliveInterval) ) 

#define IMsRdpClientAdvancedSettings3_put_shutdownTimeout(This,pshutdownTimeout)	\
    ( (This)->lpVtbl -> put_shutdownTimeout(This,pshutdownTimeout) ) 

#define IMsRdpClientAdvancedSettings3_get_shutdownTimeout(This,pshutdownTimeout)	\
    ( (This)->lpVtbl -> get_shutdownTimeout(This,pshutdownTimeout) ) 

#define IMsRdpClientAdvancedSettings3_put_overallConnectionTimeout(This,poverallConnectionTimeout)	\
    ( (This)->lpVtbl -> put_overallConnectionTimeout(This,poverallConnectionTimeout) ) 

#define IMsRdpClientAdvancedSettings3_get_overallConnectionTimeout(This,poverallConnectionTimeout)	\
    ( (This)->lpVtbl -> get_overallConnectionTimeout(This,poverallConnectionTimeout) ) 

#define IMsRdpClientAdvancedSettings3_put_singleConnectionTimeout(This,psingleConnectionTimeout)	\
    ( (This)->lpVtbl -> put_singleConnectionTimeout(This,psingleConnectionTimeout) ) 

#define IMsRdpClientAdvancedSettings3_get_singleConnectionTimeout(This,psingleConnectionTimeout)	\
    ( (This)->lpVtbl -> get_singleConnectionTimeout(This,psingleConnectionTimeout) ) 

#define IMsRdpClientAdvancedSettings3_put_KeyboardType(This,pkeyboardType)	\
    ( (This)->lpVtbl -> put_KeyboardType(This,pkeyboardType) ) 

#define IMsRdpClientAdvancedSettings3_get_KeyboardType(This,pkeyboardType)	\
    ( (This)->lpVtbl -> get_KeyboardType(This,pkeyboardType) ) 

#define IMsRdpClientAdvancedSettings3_put_KeyboardSubType(This,pkeyboardSubType)	\
    ( (This)->lpVtbl -> put_KeyboardSubType(This,pkeyboardSubType) ) 

#define IMsRdpClientAdvancedSettings3_get_KeyboardSubType(This,pkeyboardSubType)	\
    ( (This)->lpVtbl -> get_KeyboardSubType(This,pkeyboardSubType) ) 

#define IMsRdpClientAdvancedSettings3_put_KeyboardFunctionKey(This,pkeyboardFunctionKey)	\
    ( (This)->lpVtbl -> put_KeyboardFunctionKey(This,pkeyboardFunctionKey) ) 

#define IMsRdpClientAdvancedSettings3_get_KeyboardFunctionKey(This,pkeyboardFunctionKey)	\
    ( (This)->lpVtbl -> get_KeyboardFunctionKey(This,pkeyboardFunctionKey) ) 

#define IMsRdpClientAdvancedSettings3_put_WinceFixedPalette(This,pwinceFixedPalette)	\
    ( (This)->lpVtbl -> put_WinceFixedPalette(This,pwinceFixedPalette) ) 

#define IMsRdpClientAdvancedSettings3_get_WinceFixedPalette(This,pwinceFixedPalette)	\
    ( (This)->lpVtbl -> get_WinceFixedPalette(This,pwinceFixedPalette) ) 

#define IMsRdpClientAdvancedSettings3_put_ConnectToServerConsole(This,pConnectToConsole)	\
    ( (This)->lpVtbl -> put_ConnectToServerConsole(This,pConnectToConsole) ) 

#define IMsRdpClientAdvancedSettings3_get_ConnectToServerConsole(This,pConnectToConsole)	\
    ( (This)->lpVtbl -> get_ConnectToServerConsole(This,pConnectToConsole) ) 

#define IMsRdpClientAdvancedSettings3_put_BitmapPersistence(This,pbitmapPersistence)	\
    ( (This)->lpVtbl -> put_BitmapPersistence(This,pbitmapPersistence) ) 

#define IMsRdpClientAdvancedSettings3_get_BitmapPersistence(This,pbitmapPersistence)	\
    ( (This)->lpVtbl -> get_BitmapPersistence(This,pbitmapPersistence) ) 

#define IMsRdpClientAdvancedSettings3_put_MinutesToIdleTimeout(This,pminutesToIdleTimeout)	\
    ( (This)->lpVtbl -> put_MinutesToIdleTimeout(This,pminutesToIdleTimeout) ) 

#define IMsRdpClientAdvancedSettings3_get_MinutesToIdleTimeout(This,pminutesToIdleTimeout)	\
    ( (This)->lpVtbl -> get_MinutesToIdleTimeout(This,pminutesToIdleTimeout) ) 

#define IMsRdpClientAdvancedSettings3_put_SmartSizing(This,pfSmartSizing)	\
    ( (This)->lpVtbl -> put_SmartSizing(This,pfSmartSizing) ) 

#define IMsRdpClientAdvancedSettings3_get_SmartSizing(This,pfSmartSizing)	\
    ( (This)->lpVtbl -> get_SmartSizing(This,pfSmartSizing) ) 

#define IMsRdpClientAdvancedSettings3_put_RdpdrLocalPrintingDocName(This,pLocalPrintingDocName)	\
    ( (This)->lpVtbl -> put_RdpdrLocalPrintingDocName(This,pLocalPrintingDocName) ) 

#define IMsRdpClientAdvancedSettings3_get_RdpdrLocalPrintingDocName(This,pLocalPrintingDocName)	\
    ( (This)->lpVtbl -> get_RdpdrLocalPrintingDocName(This,pLocalPrintingDocName) ) 

#define IMsRdpClientAdvancedSettings3_put_RdpdrClipCleanTempDirString(This,clipCleanTempDirString)	\
    ( (This)->lpVtbl -> put_RdpdrClipCleanTempDirString(This,clipCleanTempDirString) ) 

#define IMsRdpClientAdvancedSettings3_get_RdpdrClipCleanTempDirString(This,clipCleanTempDirString)	\
    ( (This)->lpVtbl -> get_RdpdrClipCleanTempDirString(This,clipCleanTempDirString) ) 

#define IMsRdpClientAdvancedSettings3_put_RdpdrClipPasteInfoString(This,clipPasteInfoString)	\
    ( (This)->lpVtbl -> put_RdpdrClipPasteInfoString(This,clipPasteInfoString) ) 

#define IMsRdpClientAdvancedSettings3_get_RdpdrClipPasteInfoString(This,clipPasteInfoString)	\
    ( (This)->lpVtbl -> get_RdpdrClipPasteInfoString(This,clipPasteInfoString) ) 

#define IMsRdpClientAdvancedSettings3_put_ClearTextPassword(This,rhs)	\
    ( (This)->lpVtbl -> put_ClearTextPassword(This,rhs) ) 

#define IMsRdpClientAdvancedSettings3_put_DisplayConnectionBar(This,pDisplayConnectionBar)	\
    ( (This)->lpVtbl -> put_DisplayConnectionBar(This,pDisplayConnectionBar) ) 

#define IMsRdpClientAdvancedSettings3_get_DisplayConnectionBar(This,pDisplayConnectionBar)	\
    ( (This)->lpVtbl -> get_DisplayConnectionBar(This,pDisplayConnectionBar) ) 

#define IMsRdpClientAdvancedSettings3_put_PinConnectionBar(This,pPinConnectionBar)	\
    ( (This)->lpVtbl -> put_PinConnectionBar(This,pPinConnectionBar) ) 

#define IMsRdpClientAdvancedSettings3_get_PinConnectionBar(This,pPinConnectionBar)	\
    ( (This)->lpVtbl -> get_PinConnectionBar(This,pPinConnectionBar) ) 

#define IMsRdpClientAdvancedSettings3_put_GrabFocusOnConnect(This,pfGrabFocusOnConnect)	\
    ( (This)->lpVtbl -> put_GrabFocusOnConnect(This,pfGrabFocusOnConnect) ) 

#define IMsRdpClientAdvancedSettings3_get_GrabFocusOnConnect(This,pfGrabFocusOnConnect)	\
    ( (This)->lpVtbl -> get_GrabFocusOnConnect(This,pfGrabFocusOnConnect) ) 

#define IMsRdpClientAdvancedSettings3_put_LoadBalanceInfo(This,pLBInfo)	\
    ( (This)->lpVtbl -> put_LoadBalanceInfo(This,pLBInfo) ) 

#define IMsRdpClientAdvancedSettings3_get_LoadBalanceInfo(This,pLBInfo)	\
    ( (This)->lpVtbl -> get_LoadBalanceInfo(This,pLBInfo) ) 

#define IMsRdpClientAdvancedSettings3_put_RedirectDrives(This,pRedirectDrives)	\
    ( (This)->lpVtbl -> put_RedirectDrives(This,pRedirectDrives) ) 

#define IMsRdpClientAdvancedSettings3_get_RedirectDrives(This,pRedirectDrives)	\
    ( (This)->lpVtbl -> get_RedirectDrives(This,pRedirectDrives) ) 

#define IMsRdpClientAdvancedSettings3_put_RedirectPrinters(This,pRedirectPrinters)	\
    ( (This)->lpVtbl -> put_RedirectPrinters(This,pRedirectPrinters) ) 

#define IMsRdpClientAdvancedSettings3_get_RedirectPrinters(This,pRedirectPrinters)	\
    ( (This)->lpVtbl -> get_RedirectPrinters(This,pRedirectPrinters) ) 

#define IMsRdpClientAdvancedSettings3_put_RedirectPorts(This,pRedirectPorts)	\
    ( (This)->lpVtbl -> put_RedirectPorts(This,pRedirectPorts) ) 

#define IMsRdpClientAdvancedSettings3_get_RedirectPorts(This,pRedirectPorts)	\
    ( (This)->lpVtbl -> get_RedirectPorts(This,pRedirectPorts) ) 

#define IMsRdpClientAdvancedSettings3_put_RedirectSmartCards(This,pRedirectSmartCards)	\
    ( (This)->lpVtbl -> put_RedirectSmartCards(This,pRedirectSmartCards) ) 

#define IMsRdpClientAdvancedSettings3_get_RedirectSmartCards(This,pRedirectSmartCards)	\
    ( (This)->lpVtbl -> get_RedirectSmartCards(This,pRedirectSmartCards) ) 

#define IMsRdpClientAdvancedSettings3_put_BitmapVirtualCache16BppSize(This,pBitmapVirtualCache16BppSize)	\
    ( (This)->lpVtbl -> put_BitmapVirtualCache16BppSize(This,pBitmapVirtualCache16BppSize) ) 

#define IMsRdpClientAdvancedSettings3_get_BitmapVirtualCache16BppSize(This,pBitmapVirtualCache16BppSize)	\
    ( (This)->lpVtbl -> get_BitmapVirtualCache16BppSize(This,pBitmapVirtualCache16BppSize) ) 

#define IMsRdpClientAdvancedSettings3_put_BitmapVirtualCache24BppSize(This,pBitmapVirtualCache24BppSize)	\
    ( (This)->lpVtbl -> put_BitmapVirtualCache24BppSize(This,pBitmapVirtualCache24BppSize) ) 

#define IMsRdpClientAdvancedSettings3_get_BitmapVirtualCache24BppSize(This,pBitmapVirtualCache24BppSize)	\
    ( (This)->lpVtbl -> get_BitmapVirtualCache24BppSize(This,pBitmapVirtualCache24BppSize) ) 

#define IMsRdpClientAdvancedSettings3_put_PerformanceFlags(This,pDisableList)	\
    ( (This)->lpVtbl -> put_PerformanceFlags(This,pDisableList) ) 

#define IMsRdpClientAdvancedSettings3_get_PerformanceFlags(This,pDisableList)	\
    ( (This)->lpVtbl -> get_PerformanceFlags(This,pDisableList) ) 

#define IMsRdpClientAdvancedSettings3_put_ConnectWithEndpoint(This,rhs)	\
    ( (This)->lpVtbl -> put_ConnectWithEndpoint(This,rhs) ) 

#define IMsRdpClientAdvancedSettings3_put_NotifyTSPublicKey(This,pfNotify)	\
    ( (This)->lpVtbl -> put_NotifyTSPublicKey(This,pfNotify) ) 

#define IMsRdpClientAdvancedSettings3_get_NotifyTSPublicKey(This,pfNotify)	\
    ( (This)->lpVtbl -> get_NotifyTSPublicKey(This,pfNotify) ) 


#define IMsRdpClientAdvancedSettings3_get_CanAutoReconnect(This,pfCanAutoReconnect)	\
    ( (This)->lpVtbl -> get_CanAutoReconnect(This,pfCanAutoReconnect) ) 

#define IMsRdpClientAdvancedSettings3_put_EnableAutoReconnect(This,pfEnableAutoReconnect)	\
    ( (This)->lpVtbl -> put_EnableAutoReconnect(This,pfEnableAutoReconnect) ) 

#define IMsRdpClientAdvancedSettings3_get_EnableAutoReconnect(This,pfEnableAutoReconnect)	\
    ( (This)->lpVtbl -> get_EnableAutoReconnect(This,pfEnableAutoReconnect) ) 

#define IMsRdpClientAdvancedSettings3_put_MaxReconnectAttempts(This,pMaxReconnectAttempts)	\
    ( (This)->lpVtbl -> put_MaxReconnectAttempts(This,pMaxReconnectAttempts) ) 

#define IMsRdpClientAdvancedSettings3_get_MaxReconnectAttempts(This,pMaxReconnectAttempts)	\
    ( (This)->lpVtbl -> get_MaxReconnectAttempts(This,pMaxReconnectAttempts) ) 


#define IMsRdpClientAdvancedSettings3_put_ConnectionBarShowMinimizeButton(This,pfShowMinimize)	\
    ( (This)->lpVtbl -> put_ConnectionBarShowMinimizeButton(This,pfShowMinimize) ) 

#define IMsRdpClientAdvancedSettings3_get_ConnectionBarShowMinimizeButton(This,pfShowMinimize)	\
    ( (This)->lpVtbl -> get_ConnectionBarShowMinimizeButton(This,pfShowMinimize) ) 

#define IMsRdpClientAdvancedSettings3_put_ConnectionBarShowRestoreButton(This,pfShowRestore)	\
    ( (This)->lpVtbl -> put_ConnectionBarShowRestoreButton(This,pfShowRestore) ) 

#define IMsRdpClientAdvancedSettings3_get_ConnectionBarShowRestoreButton(This,pfShowRestore)	\
    ( (This)->lpVtbl -> get_ConnectionBarShowRestoreButton(This,pfShowRestore) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMsRdpClientAdvancedSettings3_put_ConnectionBarShowMinimizeButton_Proxy( 
    IMsRdpClientAdvancedSettings3 * This,
    /* [in] */ VARIANT_BOOL pfShowMinimize);


void __RPC_STUB IMsRdpClientAdvancedSettings3_put_ConnectionBarShowMinimizeButton_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IMsRdpClientAdvancedSettings3_get_ConnectionBarShowMinimizeButton_Proxy( 
    IMsRdpClientAdvancedSettings3 * This,
    /* [retval][out] */ VARIANT_BOOL *pfShowMinimize);


void __RPC_STUB IMsRdpClientAdvancedSettings3_get_ConnectionBarShowMinimizeButton_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMsRdpClientAdvancedSettings3_put_ConnectionBarShowRestoreButton_Proxy( 
    IMsRdpClientAdvancedSettings3 * This,
    /* [in] */ VARIANT_BOOL pfShowRestore);


void __RPC_STUB IMsRdpClientAdvancedSettings3_put_ConnectionBarShowRestoreButton_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IMsRdpClientAdvancedSettings3_get_ConnectionBarShowRestoreButton_Proxy( 
    IMsRdpClientAdvancedSettings3 * This,
    /* [retval][out] */ VARIANT_BOOL *pfShowRestore);


void __RPC_STUB IMsRdpClientAdvancedSettings3_get_ConnectionBarShowRestoreButton_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMsRdpClientAdvancedSettings3_INTERFACE_DEFINED__ */


#ifndef __IMsRdpClientAdvancedSettings4_INTERFACE_DEFINED__
#define __IMsRdpClientAdvancedSettings4_INTERFACE_DEFINED__

/* interface IMsRdpClientAdvancedSettings4 */
/* [object][oleautomation][dual][uuid] */ 


EXTERN_C const IID IID_IMsRdpClientAdvancedSettings4;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FBA7F64E-7345-4405-AE50-FA4A763DC0DE")
    IMsRdpClientAdvancedSettings4 : public IMsRdpClientAdvancedSettings3
    {
    public:
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_AuthenticationLevel( 
            /* [in] */ unsigned int puiAuthLevel) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_AuthenticationLevel( 
            /* [retval][out] */ unsigned int *puiAuthLevel) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMsRdpClientAdvancedSettings4Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMsRdpClientAdvancedSettings4 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMsRdpClientAdvancedSettings4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Compress )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pcompress);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Compress )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pcompress);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_BitmapPeristence )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pbitmapPeristence);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BitmapPeristence )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pbitmapPeristence);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_allowBackgroundInput )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pallowBackgroundInput);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_allowBackgroundInput )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pallowBackgroundInput);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_KeyBoardLayoutStr )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ BSTR rhs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_PluginDlls )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ BSTR rhs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_IconFile )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ BSTR rhs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_IconIndex )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long rhs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ContainerHandledFullScreen )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pContainerHandledFullScreen);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ContainerHandledFullScreen )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pContainerHandledFullScreen);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DisableRdpdr )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pDisableRdpdr);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DisableRdpdr )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pDisableRdpdr);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_SmoothScroll )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long psmoothScroll);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SmoothScroll )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *psmoothScroll);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_AcceleratorPassthrough )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pacceleratorPassthrough);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AcceleratorPassthrough )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pacceleratorPassthrough);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ShadowBitmap )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pshadowBitmap);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ShadowBitmap )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pshadowBitmap);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_TransportType )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long ptransportType);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_TransportType )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *ptransportType);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_SasSequence )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long psasSequence);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SasSequence )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *psasSequence);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_EncryptionEnabled )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pencryptionEnabled);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_EncryptionEnabled )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pencryptionEnabled);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DedicatedTerminal )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pdedicatedTerminal);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DedicatedTerminal )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pdedicatedTerminal);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RDPPort )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long prdpPort);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RDPPort )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *prdpPort);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_EnableMouse )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long penableMouse);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_EnableMouse )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *penableMouse);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DisableCtrlAltDel )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pdisableCtrlAltDel);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DisableCtrlAltDel )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pdisableCtrlAltDel);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_EnableWindowsKey )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long penableWindowsKey);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_EnableWindowsKey )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *penableWindowsKey);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DoubleClickDetect )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pdoubleClickDetect);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DoubleClickDetect )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pdoubleClickDetect);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_MaximizeShell )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pmaximizeShell);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_MaximizeShell )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pmaximizeShell);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_HotKeyFullScreen )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long photKeyFullScreen);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HotKeyFullScreen )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *photKeyFullScreen);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_HotKeyCtrlEsc )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long photKeyCtrlEsc);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HotKeyCtrlEsc )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *photKeyCtrlEsc);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_HotKeyAltEsc )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long photKeyAltEsc);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HotKeyAltEsc )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *photKeyAltEsc);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_HotKeyAltTab )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long photKeyAltTab);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HotKeyAltTab )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *photKeyAltTab);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_HotKeyAltShiftTab )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long photKeyAltShiftTab);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HotKeyAltShiftTab )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *photKeyAltShiftTab);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_HotKeyAltSpace )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long photKeyAltSpace);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HotKeyAltSpace )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *photKeyAltSpace);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_HotKeyCtrlAltDel )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long photKeyCtrlAltDel);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HotKeyCtrlAltDel )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *photKeyCtrlAltDel);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_orderDrawThreshold )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long porderDrawThreshold);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_orderDrawThreshold )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *porderDrawThreshold);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_BitmapCacheSize )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pbitmapCacheSize);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BitmapCacheSize )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pbitmapCacheSize);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_BitmapVirtualCacheSize )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pbitmapVirtualCacheSize);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BitmapVirtualCacheSize )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pbitmapVirtualCacheSize);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ScaleBitmapCachesByBPP )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pbScale);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ScaleBitmapCachesByBPP )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pbScale);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_NumBitmapCaches )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pnumBitmapCaches);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_NumBitmapCaches )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pnumBitmapCaches);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_CachePersistenceActive )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pcachePersistenceActive);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CachePersistenceActive )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pcachePersistenceActive);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_PersistCacheDirectory )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ BSTR rhs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_brushSupportLevel )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pbrushSupportLevel);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_brushSupportLevel )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pbrushSupportLevel);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_minInputSendInterval )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pminInputSendInterval);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_minInputSendInterval )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pminInputSendInterval);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_InputEventsAtOnce )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pinputEventsAtOnce);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_InputEventsAtOnce )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pinputEventsAtOnce);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_maxEventCount )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pmaxEventCount);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_maxEventCount )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pmaxEventCount);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_keepAliveInterval )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pkeepAliveInterval);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_keepAliveInterval )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pkeepAliveInterval);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_shutdownTimeout )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pshutdownTimeout);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_shutdownTimeout )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pshutdownTimeout);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_overallConnectionTimeout )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long poverallConnectionTimeout);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_overallConnectionTimeout )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *poverallConnectionTimeout);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_singleConnectionTimeout )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long psingleConnectionTimeout);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_singleConnectionTimeout )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *psingleConnectionTimeout);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_KeyboardType )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pkeyboardType);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_KeyboardType )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pkeyboardType);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_KeyboardSubType )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pkeyboardSubType);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_KeyboardSubType )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pkeyboardSubType);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_KeyboardFunctionKey )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pkeyboardFunctionKey);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_KeyboardFunctionKey )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pkeyboardFunctionKey);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_WinceFixedPalette )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pwinceFixedPalette);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_WinceFixedPalette )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pwinceFixedPalette);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ConnectToServerConsole )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ VARIANT_BOOL pConnectToConsole);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ConnectToServerConsole )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ VARIANT_BOOL *pConnectToConsole);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_BitmapPersistence )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pbitmapPersistence);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BitmapPersistence )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pbitmapPersistence);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_MinutesToIdleTimeout )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pminutesToIdleTimeout);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_MinutesToIdleTimeout )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pminutesToIdleTimeout);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_SmartSizing )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ VARIANT_BOOL pfSmartSizing);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SmartSizing )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ VARIANT_BOOL *pfSmartSizing);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RdpdrLocalPrintingDocName )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ BSTR pLocalPrintingDocName);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RdpdrLocalPrintingDocName )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ BSTR *pLocalPrintingDocName);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RdpdrClipCleanTempDirString )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ BSTR clipCleanTempDirString);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RdpdrClipCleanTempDirString )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ BSTR *clipCleanTempDirString);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RdpdrClipPasteInfoString )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ BSTR clipPasteInfoString);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RdpdrClipPasteInfoString )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ BSTR *clipPasteInfoString);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ClearTextPassword )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ BSTR rhs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DisplayConnectionBar )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ VARIANT_BOOL pDisplayConnectionBar);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DisplayConnectionBar )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ VARIANT_BOOL *pDisplayConnectionBar);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_PinConnectionBar )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ VARIANT_BOOL pPinConnectionBar);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_PinConnectionBar )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ VARIANT_BOOL *pPinConnectionBar);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_GrabFocusOnConnect )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ VARIANT_BOOL pfGrabFocusOnConnect);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_GrabFocusOnConnect )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ VARIANT_BOOL *pfGrabFocusOnConnect);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_LoadBalanceInfo )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ BSTR pLBInfo);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_LoadBalanceInfo )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ BSTR *pLBInfo);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RedirectDrives )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ VARIANT_BOOL pRedirectDrives);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RedirectDrives )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ VARIANT_BOOL *pRedirectDrives);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RedirectPrinters )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ VARIANT_BOOL pRedirectPrinters);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RedirectPrinters )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ VARIANT_BOOL *pRedirectPrinters);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RedirectPorts )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ VARIANT_BOOL pRedirectPorts);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RedirectPorts )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ VARIANT_BOOL *pRedirectPorts);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RedirectSmartCards )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ VARIANT_BOOL pRedirectSmartCards);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RedirectSmartCards )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ VARIANT_BOOL *pRedirectSmartCards);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_BitmapVirtualCache16BppSize )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pBitmapVirtualCache16BppSize);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BitmapVirtualCache16BppSize )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pBitmapVirtualCache16BppSize);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_BitmapVirtualCache24BppSize )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pBitmapVirtualCache24BppSize);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BitmapVirtualCache24BppSize )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pBitmapVirtualCache24BppSize);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_PerformanceFlags )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pDisableList);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_PerformanceFlags )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pDisableList);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ConnectWithEndpoint )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ VARIANT *rhs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_NotifyTSPublicKey )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ VARIANT_BOOL pfNotify);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_NotifyTSPublicKey )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ VARIANT_BOOL *pfNotify);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CanAutoReconnect )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ VARIANT_BOOL *pfCanAutoReconnect);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_EnableAutoReconnect )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ VARIANT_BOOL pfEnableAutoReconnect);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_EnableAutoReconnect )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ VARIANT_BOOL *pfEnableAutoReconnect);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_MaxReconnectAttempts )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ long pMaxReconnectAttempts);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_MaxReconnectAttempts )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ long *pMaxReconnectAttempts);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ConnectionBarShowMinimizeButton )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ VARIANT_BOOL pfShowMinimize);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ConnectionBarShowMinimizeButton )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ VARIANT_BOOL *pfShowMinimize);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ConnectionBarShowRestoreButton )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ VARIANT_BOOL pfShowRestore);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ConnectionBarShowRestoreButton )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ VARIANT_BOOL *pfShowRestore);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_AuthenticationLevel )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [in] */ unsigned int puiAuthLevel);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AuthenticationLevel )( 
            IMsRdpClientAdvancedSettings4 * This,
            /* [retval][out] */ unsigned int *puiAuthLevel);
        
        END_INTERFACE
    } IMsRdpClientAdvancedSettings4Vtbl;

    interface IMsRdpClientAdvancedSettings4
    {
        CONST_VTBL struct IMsRdpClientAdvancedSettings4Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMsRdpClientAdvancedSettings4_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMsRdpClientAdvancedSettings4_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMsRdpClientAdvancedSettings4_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMsRdpClientAdvancedSettings4_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IMsRdpClientAdvancedSettings4_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IMsRdpClientAdvancedSettings4_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IMsRdpClientAdvancedSettings4_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IMsRdpClientAdvancedSettings4_put_Compress(This,pcompress)	\
    ( (This)->lpVtbl -> put_Compress(This,pcompress) ) 

#define IMsRdpClientAdvancedSettings4_get_Compress(This,pcompress)	\
    ( (This)->lpVtbl -> get_Compress(This,pcompress) ) 

#define IMsRdpClientAdvancedSettings4_put_BitmapPeristence(This,pbitmapPeristence)	\
    ( (This)->lpVtbl -> put_BitmapPeristence(This,pbitmapPeristence) ) 

#define IMsRdpClientAdvancedSettings4_get_BitmapPeristence(This,pbitmapPeristence)	\
    ( (This)->lpVtbl -> get_BitmapPeristence(This,pbitmapPeristence) ) 

#define IMsRdpClientAdvancedSettings4_put_allowBackgroundInput(This,pallowBackgroundInput)	\
    ( (This)->lpVtbl -> put_allowBackgroundInput(This,pallowBackgroundInput) ) 

#define IMsRdpClientAdvancedSettings4_get_allowBackgroundInput(This,pallowBackgroundInput)	\
    ( (This)->lpVtbl -> get_allowBackgroundInput(This,pallowBackgroundInput) ) 

#define IMsRdpClientAdvancedSettings4_put_KeyBoardLayoutStr(This,rhs)	\
    ( (This)->lpVtbl -> put_KeyBoardLayoutStr(This,rhs) ) 

#define IMsRdpClientAdvancedSettings4_put_PluginDlls(This,rhs)	\
    ( (This)->lpVtbl -> put_PluginDlls(This,rhs) ) 

#define IMsRdpClientAdvancedSettings4_put_IconFile(This,rhs)	\
    ( (This)->lpVtbl -> put_IconFile(This,rhs) ) 

#define IMsRdpClientAdvancedSettings4_put_IconIndex(This,rhs)	\
    ( (This)->lpVtbl -> put_IconIndex(This,rhs) ) 

#define IMsRdpClientAdvancedSettings4_put_ContainerHandledFullScreen(This,pContainerHandledFullScreen)	\
    ( (This)->lpVtbl -> put_ContainerHandledFullScreen(This,pContainerHandledFullScreen) ) 

#define IMsRdpClientAdvancedSettings4_get_ContainerHandledFullScreen(This,pContainerHandledFullScreen)	\
    ( (This)->lpVtbl -> get_ContainerHandledFullScreen(This,pContainerHandledFullScreen) ) 

#define IMsRdpClientAdvancedSettings4_put_DisableRdpdr(This,pDisableRdpdr)	\
    ( (This)->lpVtbl -> put_DisableRdpdr(This,pDisableRdpdr) ) 

#define IMsRdpClientAdvancedSettings4_get_DisableRdpdr(This,pDisableRdpdr)	\
    ( (This)->lpVtbl -> get_DisableRdpdr(This,pDisableRdpdr) ) 


#define IMsRdpClientAdvancedSettings4_put_SmoothScroll(This,psmoothScroll)	\
    ( (This)->lpVtbl -> put_SmoothScroll(This,psmoothScroll) ) 

#define IMsRdpClientAdvancedSettings4_get_SmoothScroll(This,psmoothScroll)	\
    ( (This)->lpVtbl -> get_SmoothScroll(This,psmoothScroll) ) 

#define IMsRdpClientAdvancedSettings4_put_AcceleratorPassthrough(This,pacceleratorPassthrough)	\
    ( (This)->lpVtbl -> put_AcceleratorPassthrough(This,pacceleratorPassthrough) ) 

#define IMsRdpClientAdvancedSettings4_get_AcceleratorPassthrough(This,pacceleratorPassthrough)	\
    ( (This)->lpVtbl -> get_AcceleratorPassthrough(This,pacceleratorPassthrough) ) 

#define IMsRdpClientAdvancedSettings4_put_ShadowBitmap(This,pshadowBitmap)	\
    ( (This)->lpVtbl -> put_ShadowBitmap(This,pshadowBitmap) ) 

#define IMsRdpClientAdvancedSettings4_get_ShadowBitmap(This,pshadowBitmap)	\
    ( (This)->lpVtbl -> get_ShadowBitmap(This,pshadowBitmap) ) 

#define IMsRdpClientAdvancedSettings4_put_TransportType(This,ptransportType)	\
    ( (This)->lpVtbl -> put_TransportType(This,ptransportType) ) 

#define IMsRdpClientAdvancedSettings4_get_TransportType(This,ptransportType)	\
    ( (This)->lpVtbl -> get_TransportType(This,ptransportType) ) 

#define IMsRdpClientAdvancedSettings4_put_SasSequence(This,psasSequence)	\
    ( (This)->lpVtbl -> put_SasSequence(This,psasSequence) ) 

#define IMsRdpClientAdvancedSettings4_get_SasSequence(This,psasSequence)	\
    ( (This)->lpVtbl -> get_SasSequence(This,psasSequence) ) 

#define IMsRdpClientAdvancedSettings4_put_EncryptionEnabled(This,pencryptionEnabled)	\
    ( (This)->lpVtbl -> put_EncryptionEnabled(This,pencryptionEnabled) ) 

#define IMsRdpClientAdvancedSettings4_get_EncryptionEnabled(This,pencryptionEnabled)	\
    ( (This)->lpVtbl -> get_EncryptionEnabled(This,pencryptionEnabled) ) 

#define IMsRdpClientAdvancedSettings4_put_DedicatedTerminal(This,pdedicatedTerminal)	\
    ( (This)->lpVtbl -> put_DedicatedTerminal(This,pdedicatedTerminal) ) 

#define IMsRdpClientAdvancedSettings4_get_DedicatedTerminal(This,pdedicatedTerminal)	\
    ( (This)->lpVtbl -> get_DedicatedTerminal(This,pdedicatedTerminal) ) 

#define IMsRdpClientAdvancedSettings4_put_RDPPort(This,prdpPort)	\
    ( (This)->lpVtbl -> put_RDPPort(This,prdpPort) ) 

#define IMsRdpClientAdvancedSettings4_get_RDPPort(This,prdpPort)	\
    ( (This)->lpVtbl -> get_RDPPort(This,prdpPort) ) 

#define IMsRdpClientAdvancedSettings4_put_EnableMouse(This,penableMouse)	\
    ( (This)->lpVtbl -> put_EnableMouse(This,penableMouse) ) 

#define IMsRdpClientAdvancedSettings4_get_EnableMouse(This,penableMouse)	\
    ( (This)->lpVtbl -> get_EnableMouse(This,penableMouse) ) 

#define IMsRdpClientAdvancedSettings4_put_DisableCtrlAltDel(This,pdisableCtrlAltDel)	\
    ( (This)->lpVtbl -> put_DisableCtrlAltDel(This,pdisableCtrlAltDel) ) 

#define IMsRdpClientAdvancedSettings4_get_DisableCtrlAltDel(This,pdisableCtrlAltDel)	\
    ( (This)->lpVtbl -> get_DisableCtrlAltDel(This,pdisableCtrlAltDel) ) 

#define IMsRdpClientAdvancedSettings4_put_EnableWindowsKey(This,penableWindowsKey)	\
    ( (This)->lpVtbl -> put_EnableWindowsKey(This,penableWindowsKey) ) 

#define IMsRdpClientAdvancedSettings4_get_EnableWindowsKey(This,penableWindowsKey)	\
    ( (This)->lpVtbl -> get_EnableWindowsKey(This,penableWindowsKey) ) 

#define IMsRdpClientAdvancedSettings4_put_DoubleClickDetect(This,pdoubleClickDetect)	\
    ( (This)->lpVtbl -> put_DoubleClickDetect(This,pdoubleClickDetect) ) 

#define IMsRdpClientAdvancedSettings4_get_DoubleClickDetect(This,pdoubleClickDetect)	\
    ( (This)->lpVtbl -> get_DoubleClickDetect(This,pdoubleClickDetect) ) 

#define IMsRdpClientAdvancedSettings4_put_MaximizeShell(This,pmaximizeShell)	\
    ( (This)->lpVtbl -> put_MaximizeShell(This,pmaximizeShell) ) 

#define IMsRdpClientAdvancedSettings4_get_MaximizeShell(This,pmaximizeShell)	\
    ( (This)->lpVtbl -> get_MaximizeShell(This,pmaximizeShell) ) 

#define IMsRdpClientAdvancedSettings4_put_HotKeyFullScreen(This,photKeyFullScreen)	\
    ( (This)->lpVtbl -> put_HotKeyFullScreen(This,photKeyFullScreen) ) 

#define IMsRdpClientAdvancedSettings4_get_HotKeyFullScreen(This,photKeyFullScreen)	\
    ( (This)->lpVtbl -> get_HotKeyFullScreen(This,photKeyFullScreen) ) 

#define IMsRdpClientAdvancedSettings4_put_HotKeyCtrlEsc(This,photKeyCtrlEsc)	\
    ( (This)->lpVtbl -> put_HotKeyCtrlEsc(This,photKeyCtrlEsc) ) 

#define IMsRdpClientAdvancedSettings4_get_HotKeyCtrlEsc(This,photKeyCtrlEsc)	\
    ( (This)->lpVtbl -> get_HotKeyCtrlEsc(This,photKeyCtrlEsc) ) 

#define IMsRdpClientAdvancedSettings4_put_HotKeyAltEsc(This,photKeyAltEsc)	\
    ( (This)->lpVtbl -> put_HotKeyAltEsc(This,photKeyAltEsc) ) 

#define IMsRdpClientAdvancedSettings4_get_HotKeyAltEsc(This,photKeyAltEsc)	\
    ( (This)->lpVtbl -> get_HotKeyAltEsc(This,photKeyAltEsc) ) 

#define IMsRdpClientAdvancedSettings4_put_HotKeyAltTab(This,photKeyAltTab)	\
    ( (This)->lpVtbl -> put_HotKeyAltTab(This,photKeyAltTab) ) 

#define IMsRdpClientAdvancedSettings4_get_HotKeyAltTab(This,photKeyAltTab)	\
    ( (This)->lpVtbl -> get_HotKeyAltTab(This,photKeyAltTab) ) 

#define IMsRdpClientAdvancedSettings4_put_HotKeyAltShiftTab(This,photKeyAltShiftTab)	\
    ( (This)->lpVtbl -> put_HotKeyAltShiftTab(This,photKeyAltShiftTab) ) 

#define IMsRdpClientAdvancedSettings4_get_HotKeyAltShiftTab(This,photKeyAltShiftTab)	\
    ( (This)->lpVtbl -> get_HotKeyAltShiftTab(This,photKeyAltShiftTab) ) 

#define IMsRdpClientAdvancedSettings4_put_HotKeyAltSpace(This,photKeyAltSpace)	\
    ( (This)->lpVtbl -> put_HotKeyAltSpace(This,photKeyAltSpace) ) 

#define IMsRdpClientAdvancedSettings4_get_HotKeyAltSpace(This,photKeyAltSpace)	\
    ( (This)->lpVtbl -> get_HotKeyAltSpace(This,photKeyAltSpace) ) 

#define IMsRdpClientAdvancedSettings4_put_HotKeyCtrlAltDel(This,photKeyCtrlAltDel)	\
    ( (This)->lpVtbl -> put_HotKeyCtrlAltDel(This,photKeyCtrlAltDel) ) 

#define IMsRdpClientAdvancedSettings4_get_HotKeyCtrlAltDel(This,photKeyCtrlAltDel)	\
    ( (This)->lpVtbl -> get_HotKeyCtrlAltDel(This,photKeyCtrlAltDel) ) 

#define IMsRdpClientAdvancedSettings4_put_orderDrawThreshold(This,porderDrawThreshold)	\
    ( (This)->lpVtbl -> put_orderDrawThreshold(This,porderDrawThreshold) ) 

#define IMsRdpClientAdvancedSettings4_get_orderDrawThreshold(This,porderDrawThreshold)	\
    ( (This)->lpVtbl -> get_orderDrawThreshold(This,porderDrawThreshold) ) 

#define IMsRdpClientAdvancedSettings4_put_BitmapCacheSize(This,pbitmapCacheSize)	\
    ( (This)->lpVtbl -> put_BitmapCacheSize(This,pbitmapCacheSize) ) 

#define IMsRdpClientAdvancedSettings4_get_BitmapCacheSize(This,pbitmapCacheSize)	\
    ( (This)->lpVtbl -> get_BitmapCacheSize(This,pbitmapCacheSize) ) 

#define IMsRdpClientAdvancedSettings4_put_BitmapVirtualCacheSize(This,pbitmapVirtualCacheSize)	\
    ( (This)->lpVtbl -> put_BitmapVirtualCacheSize(This,pbitmapVirtualCacheSize) ) 

#define IMsRdpClientAdvancedSettings4_get_BitmapVirtualCacheSize(This,pbitmapVirtualCacheSize)	\
    ( (This)->lpVtbl -> get_BitmapVirtualCacheSize(This,pbitmapVirtualCacheSize) ) 

#define IMsRdpClientAdvancedSettings4_put_ScaleBitmapCachesByBPP(This,pbScale)	\
    ( (This)->lpVtbl -> put_ScaleBitmapCachesByBPP(This,pbScale) ) 

#define IMsRdpClientAdvancedSettings4_get_ScaleBitmapCachesByBPP(This,pbScale)	\
    ( (This)->lpVtbl -> get_ScaleBitmapCachesByBPP(This,pbScale) ) 

#define IMsRdpClientAdvancedSettings4_put_NumBitmapCaches(This,pnumBitmapCaches)	\
    ( (This)->lpVtbl -> put_NumBitmapCaches(This,pnumBitmapCaches) ) 

#define IMsRdpClientAdvancedSettings4_get_NumBitmapCaches(This,pnumBitmapCaches)	\
    ( (This)->lpVtbl -> get_NumBitmapCaches(This,pnumBitmapCaches) ) 

#define IMsRdpClientAdvancedSettings4_put_CachePersistenceActive(This,pcachePersistenceActive)	\
    ( (This)->lpVtbl -> put_CachePersistenceActive(This,pcachePersistenceActive) ) 

#define IMsRdpClientAdvancedSettings4_get_CachePersistenceActive(This,pcachePersistenceActive)	\
    ( (This)->lpVtbl -> get_CachePersistenceActive(This,pcachePersistenceActive) ) 

#define IMsRdpClientAdvancedSettings4_put_PersistCacheDirectory(This,rhs)	\
    ( (This)->lpVtbl -> put_PersistCacheDirectory(This,rhs) ) 

#define IMsRdpClientAdvancedSettings4_put_brushSupportLevel(This,pbrushSupportLevel)	\
    ( (This)->lpVtbl -> put_brushSupportLevel(This,pbrushSupportLevel) ) 

#define IMsRdpClientAdvancedSettings4_get_brushSupportLevel(This,pbrushSupportLevel)	\
    ( (This)->lpVtbl -> get_brushSupportLevel(This,pbrushSupportLevel) ) 

#define IMsRdpClientAdvancedSettings4_put_minInputSendInterval(This,pminInputSendInterval)	\
    ( (This)->lpVtbl -> put_minInputSendInterval(This,pminInputSendInterval) ) 

#define IMsRdpClientAdvancedSettings4_get_minInputSendInterval(This,pminInputSendInterval)	\
    ( (This)->lpVtbl -> get_minInputSendInterval(This,pminInputSendInterval) ) 

#define IMsRdpClientAdvancedSettings4_put_InputEventsAtOnce(This,pinputEventsAtOnce)	\
    ( (This)->lpVtbl -> put_InputEventsAtOnce(This,pinputEventsAtOnce) ) 

#define IMsRdpClientAdvancedSettings4_get_InputEventsAtOnce(This,pinputEventsAtOnce)	\
    ( (This)->lpVtbl -> get_InputEventsAtOnce(This,pinputEventsAtOnce) ) 

#define IMsRdpClientAdvancedSettings4_put_maxEventCount(This,pmaxEventCount)	\
    ( (This)->lpVtbl -> put_maxEventCount(This,pmaxEventCount) ) 

#define IMsRdpClientAdvancedSettings4_get_maxEventCount(This,pmaxEventCount)	\
    ( (This)->lpVtbl -> get_maxEventCount(This,pmaxEventCount) ) 

#define IMsRdpClientAdvancedSettings4_put_keepAliveInterval(This,pkeepAliveInterval)	\
    ( (This)->lpVtbl -> put_keepAliveInterval(This,pkeepAliveInterval) ) 

#define IMsRdpClientAdvancedSettings4_get_keepAliveInterval(This,pkeepAliveInterval)	\
    ( (This)->lpVtbl -> get_keepAliveInterval(This,pkeepAliveInterval) ) 

#define IMsRdpClientAdvancedSettings4_put_shutdownTimeout(This,pshutdownTimeout)	\
    ( (This)->lpVtbl -> put_shutdownTimeout(This,pshutdownTimeout) ) 

#define IMsRdpClientAdvancedSettings4_get_shutdownTimeout(This,pshutdownTimeout)	\
    ( (This)->lpVtbl -> get_shutdownTimeout(This,pshutdownTimeout) ) 

#define IMsRdpClientAdvancedSettings4_put_overallConnectionTimeout(This,poverallConnectionTimeout)	\
    ( (This)->lpVtbl -> put_overallConnectionTimeout(This,poverallConnectionTimeout) ) 

#define IMsRdpClientAdvancedSettings4_get_overallConnectionTimeout(This,poverallConnectionTimeout)	\
    ( (This)->lpVtbl -> get_overallConnectionTimeout(This,poverallConnectionTimeout) ) 

#define IMsRdpClientAdvancedSettings4_put_singleConnectionTimeout(This,psingleConnectionTimeout)	\
    ( (This)->lpVtbl -> put_singleConnectionTimeout(This,psingleConnectionTimeout) ) 

#define IMsRdpClientAdvancedSettings4_get_singleConnectionTimeout(This,psingleConnectionTimeout)	\
    ( (This)->lpVtbl -> get_singleConnectionTimeout(This,psingleConnectionTimeout) ) 

#define IMsRdpClientAdvancedSettings4_put_KeyboardType(This,pkeyboardType)	\
    ( (This)->lpVtbl -> put_KeyboardType(This,pkeyboardType) ) 

#define IMsRdpClientAdvancedSettings4_get_KeyboardType(This,pkeyboardType)	\
    ( (This)->lpVtbl -> get_KeyboardType(This,pkeyboardType) ) 

#define IMsRdpClientAdvancedSettings4_put_KeyboardSubType(This,pkeyboardSubType)	\
    ( (This)->lpVtbl -> put_KeyboardSubType(This,pkeyboardSubType) ) 

#define IMsRdpClientAdvancedSettings4_get_KeyboardSubType(This,pkeyboardSubType)	\
    ( (This)->lpVtbl -> get_KeyboardSubType(This,pkeyboardSubType) ) 

#define IMsRdpClientAdvancedSettings4_put_KeyboardFunctionKey(This,pkeyboardFunctionKey)	\
    ( (This)->lpVtbl -> put_KeyboardFunctionKey(This,pkeyboardFunctionKey) ) 

#define IMsRdpClientAdvancedSettings4_get_KeyboardFunctionKey(This,pkeyboardFunctionKey)	\
    ( (This)->lpVtbl -> get_KeyboardFunctionKey(This,pkeyboardFunctionKey) ) 

#define IMsRdpClientAdvancedSettings4_put_WinceFixedPalette(This,pwinceFixedPalette)	\
    ( (This)->lpVtbl -> put_WinceFixedPalette(This,pwinceFixedPalette) ) 

#define IMsRdpClientAdvancedSettings4_get_WinceFixedPalette(This,pwinceFixedPalette)	\
    ( (This)->lpVtbl -> get_WinceFixedPalette(This,pwinceFixedPalette) ) 

#define IMsRdpClientAdvancedSettings4_put_ConnectToServerConsole(This,pConnectToConsole)	\
    ( (This)->lpVtbl -> put_ConnectToServerConsole(This,pConnectToConsole) ) 

#define IMsRdpClientAdvancedSettings4_get_ConnectToServerConsole(This,pConnectToConsole)	\
    ( (This)->lpVtbl -> get_ConnectToServerConsole(This,pConnectToConsole) ) 

#define IMsRdpClientAdvancedSettings4_put_BitmapPersistence(This,pbitmapPersistence)	\
    ( (This)->lpVtbl -> put_BitmapPersistence(This,pbitmapPersistence) ) 

#define IMsRdpClientAdvancedSettings4_get_BitmapPersistence(This,pbitmapPersistence)	\
    ( (This)->lpVtbl -> get_BitmapPersistence(This,pbitmapPersistence) ) 

#define IMsRdpClientAdvancedSettings4_put_MinutesToIdleTimeout(This,pminutesToIdleTimeout)	\
    ( (This)->lpVtbl -> put_MinutesToIdleTimeout(This,pminutesToIdleTimeout) ) 

#define IMsRdpClientAdvancedSettings4_get_MinutesToIdleTimeout(This,pminutesToIdleTimeout)	\
    ( (This)->lpVtbl -> get_MinutesToIdleTimeout(This,pminutesToIdleTimeout) ) 

#define IMsRdpClientAdvancedSettings4_put_SmartSizing(This,pfSmartSizing)	\
    ( (This)->lpVtbl -> put_SmartSizing(This,pfSmartSizing) ) 

#define IMsRdpClientAdvancedSettings4_get_SmartSizing(This,pfSmartSizing)	\
    ( (This)->lpVtbl -> get_SmartSizing(This,pfSmartSizing) ) 

#define IMsRdpClientAdvancedSettings4_put_RdpdrLocalPrintingDocName(This,pLocalPrintingDocName)	\
    ( (This)->lpVtbl -> put_RdpdrLocalPrintingDocName(This,pLocalPrintingDocName) ) 

#define IMsRdpClientAdvancedSettings4_get_RdpdrLocalPrintingDocName(This,pLocalPrintingDocName)	\
    ( (This)->lpVtbl -> get_RdpdrLocalPrintingDocName(This,pLocalPrintingDocName) ) 

#define IMsRdpClientAdvancedSettings4_put_RdpdrClipCleanTempDirString(This,clipCleanTempDirString)	\
    ( (This)->lpVtbl -> put_RdpdrClipCleanTempDirString(This,clipCleanTempDirString) ) 

#define IMsRdpClientAdvancedSettings4_get_RdpdrClipCleanTempDirString(This,clipCleanTempDirString)	\
    ( (This)->lpVtbl -> get_RdpdrClipCleanTempDirString(This,clipCleanTempDirString) ) 

#define IMsRdpClientAdvancedSettings4_put_RdpdrClipPasteInfoString(This,clipPasteInfoString)	\
    ( (This)->lpVtbl -> put_RdpdrClipPasteInfoString(This,clipPasteInfoString) ) 

#define IMsRdpClientAdvancedSettings4_get_RdpdrClipPasteInfoString(This,clipPasteInfoString)	\
    ( (This)->lpVtbl -> get_RdpdrClipPasteInfoString(This,clipPasteInfoString) ) 

#define IMsRdpClientAdvancedSettings4_put_ClearTextPassword(This,rhs)	\
    ( (This)->lpVtbl -> put_ClearTextPassword(This,rhs) ) 

#define IMsRdpClientAdvancedSettings4_put_DisplayConnectionBar(This,pDisplayConnectionBar)	\
    ( (This)->lpVtbl -> put_DisplayConnectionBar(This,pDisplayConnectionBar) ) 

#define IMsRdpClientAdvancedSettings4_get_DisplayConnectionBar(This,pDisplayConnectionBar)	\
    ( (This)->lpVtbl -> get_DisplayConnectionBar(This,pDisplayConnectionBar) ) 

#define IMsRdpClientAdvancedSettings4_put_PinConnectionBar(This,pPinConnectionBar)	\
    ( (This)->lpVtbl -> put_PinConnectionBar(This,pPinConnectionBar) ) 

#define IMsRdpClientAdvancedSettings4_get_PinConnectionBar(This,pPinConnectionBar)	\
    ( (This)->lpVtbl -> get_PinConnectionBar(This,pPinConnectionBar) ) 

#define IMsRdpClientAdvancedSettings4_put_GrabFocusOnConnect(This,pfGrabFocusOnConnect)	\
    ( (This)->lpVtbl -> put_GrabFocusOnConnect(This,pfGrabFocusOnConnect) ) 

#define IMsRdpClientAdvancedSettings4_get_GrabFocusOnConnect(This,pfGrabFocusOnConnect)	\
    ( (This)->lpVtbl -> get_GrabFocusOnConnect(This,pfGrabFocusOnConnect) ) 

#define IMsRdpClientAdvancedSettings4_put_LoadBalanceInfo(This,pLBInfo)	\
    ( (This)->lpVtbl -> put_LoadBalanceInfo(This,pLBInfo) ) 

#define IMsRdpClientAdvancedSettings4_get_LoadBalanceInfo(This,pLBInfo)	\
    ( (This)->lpVtbl -> get_LoadBalanceInfo(This,pLBInfo) ) 

#define IMsRdpClientAdvancedSettings4_put_RedirectDrives(This,pRedirectDrives)	\
    ( (This)->lpVtbl -> put_RedirectDrives(This,pRedirectDrives) ) 

#define IMsRdpClientAdvancedSettings4_get_RedirectDrives(This,pRedirectDrives)	\
    ( (This)->lpVtbl -> get_RedirectDrives(This,pRedirectDrives) ) 

#define IMsRdpClientAdvancedSettings4_put_RedirectPrinters(This,pRedirectPrinters)	\
    ( (This)->lpVtbl -> put_RedirectPrinters(This,pRedirectPrinters) ) 

#define IMsRdpClientAdvancedSettings4_get_RedirectPrinters(This,pRedirectPrinters)	\
    ( (This)->lpVtbl -> get_RedirectPrinters(This,pRedirectPrinters) ) 

#define IMsRdpClientAdvancedSettings4_put_RedirectPorts(This,pRedirectPorts)	\
    ( (This)->lpVtbl -> put_RedirectPorts(This,pRedirectPorts) ) 

#define IMsRdpClientAdvancedSettings4_get_RedirectPorts(This,pRedirectPorts)	\
    ( (This)->lpVtbl -> get_RedirectPorts(This,pRedirectPorts) ) 

#define IMsRdpClientAdvancedSettings4_put_RedirectSmartCards(This,pRedirectSmartCards)	\
    ( (This)->lpVtbl -> put_RedirectSmartCards(This,pRedirectSmartCards) ) 

#define IMsRdpClientAdvancedSettings4_get_RedirectSmartCards(This,pRedirectSmartCards)	\
    ( (This)->lpVtbl -> get_RedirectSmartCards(This,pRedirectSmartCards) ) 

#define IMsRdpClientAdvancedSettings4_put_BitmapVirtualCache16BppSize(This,pBitmapVirtualCache16BppSize)	\
    ( (This)->lpVtbl -> put_BitmapVirtualCache16BppSize(This,pBitmapVirtualCache16BppSize) ) 

#define IMsRdpClientAdvancedSettings4_get_BitmapVirtualCache16BppSize(This,pBitmapVirtualCache16BppSize)	\
    ( (This)->lpVtbl -> get_BitmapVirtualCache16BppSize(This,pBitmapVirtualCache16BppSize) ) 

#define IMsRdpClientAdvancedSettings4_put_BitmapVirtualCache24BppSize(This,pBitmapVirtualCache24BppSize)	\
    ( (This)->lpVtbl -> put_BitmapVirtualCache24BppSize(This,pBitmapVirtualCache24BppSize) ) 

#define IMsRdpClientAdvancedSettings4_get_BitmapVirtualCache24BppSize(This,pBitmapVirtualCache24BppSize)	\
    ( (This)->lpVtbl -> get_BitmapVirtualCache24BppSize(This,pBitmapVirtualCache24BppSize) ) 

#define IMsRdpClientAdvancedSettings4_put_PerformanceFlags(This,pDisableList)	\
    ( (This)->lpVtbl -> put_PerformanceFlags(This,pDisableList) ) 

#define IMsRdpClientAdvancedSettings4_get_PerformanceFlags(This,pDisableList)	\
    ( (This)->lpVtbl -> get_PerformanceFlags(This,pDisableList) ) 

#define IMsRdpClientAdvancedSettings4_put_ConnectWithEndpoint(This,rhs)	\
    ( (This)->lpVtbl -> put_ConnectWithEndpoint(This,rhs) ) 

#define IMsRdpClientAdvancedSettings4_put_NotifyTSPublicKey(This,pfNotify)	\
    ( (This)->lpVtbl -> put_NotifyTSPublicKey(This,pfNotify) ) 

#define IMsRdpClientAdvancedSettings4_get_NotifyTSPublicKey(This,pfNotify)	\
    ( (This)->lpVtbl -> get_NotifyTSPublicKey(This,pfNotify) ) 


#define IMsRdpClientAdvancedSettings4_get_CanAutoReconnect(This,pfCanAutoReconnect)	\
    ( (This)->lpVtbl -> get_CanAutoReconnect(This,pfCanAutoReconnect) ) 

#define IMsRdpClientAdvancedSettings4_put_EnableAutoReconnect(This,pfEnableAutoReconnect)	\
    ( (This)->lpVtbl -> put_EnableAutoReconnect(This,pfEnableAutoReconnect) ) 

#define IMsRdpClientAdvancedSettings4_get_EnableAutoReconnect(This,pfEnableAutoReconnect)	\
    ( (This)->lpVtbl -> get_EnableAutoReconnect(This,pfEnableAutoReconnect) ) 

#define IMsRdpClientAdvancedSettings4_put_MaxReconnectAttempts(This,pMaxReconnectAttempts)	\
    ( (This)->lpVtbl -> put_MaxReconnectAttempts(This,pMaxReconnectAttempts) ) 

#define IMsRdpClientAdvancedSettings4_get_MaxReconnectAttempts(This,pMaxReconnectAttempts)	\
    ( (This)->lpVtbl -> get_MaxReconnectAttempts(This,pMaxReconnectAttempts) ) 


#define IMsRdpClientAdvancedSettings4_put_ConnectionBarShowMinimizeButton(This,pfShowMinimize)	\
    ( (This)->lpVtbl -> put_ConnectionBarShowMinimizeButton(This,pfShowMinimize) ) 

#define IMsRdpClientAdvancedSettings4_get_ConnectionBarShowMinimizeButton(This,pfShowMinimize)	\
    ( (This)->lpVtbl -> get_ConnectionBarShowMinimizeButton(This,pfShowMinimize) ) 

#define IMsRdpClientAdvancedSettings4_put_ConnectionBarShowRestoreButton(This,pfShowRestore)	\
    ( (This)->lpVtbl -> put_ConnectionBarShowRestoreButton(This,pfShowRestore) ) 

#define IMsRdpClientAdvancedSettings4_get_ConnectionBarShowRestoreButton(This,pfShowRestore)	\
    ( (This)->lpVtbl -> get_ConnectionBarShowRestoreButton(This,pfShowRestore) ) 


#define IMsRdpClientAdvancedSettings4_put_AuthenticationLevel(This,puiAuthLevel)	\
    ( (This)->lpVtbl -> put_AuthenticationLevel(This,puiAuthLevel) ) 

#define IMsRdpClientAdvancedSettings4_get_AuthenticationLevel(This,puiAuthLevel)	\
    ( (This)->lpVtbl -> get_AuthenticationLevel(This,puiAuthLevel) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMsRdpClientAdvancedSettings4_put_AuthenticationLevel_Proxy( 
    IMsRdpClientAdvancedSettings4 * This,
    /* [in] */ unsigned int puiAuthLevel);


void __RPC_STUB IMsRdpClientAdvancedSettings4_put_AuthenticationLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IMsRdpClientAdvancedSettings4_get_AuthenticationLevel_Proxy( 
    IMsRdpClientAdvancedSettings4 * This,
    /* [retval][out] */ unsigned int *puiAuthLevel);


void __RPC_STUB IMsRdpClientAdvancedSettings4_get_AuthenticationLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMsRdpClientAdvancedSettings4_INTERFACE_DEFINED__ */


#ifndef __IMsTscSecuredSettings_INTERFACE_DEFINED__
#define __IMsTscSecuredSettings_INTERFACE_DEFINED__

/* interface IMsTscSecuredSettings */
/* [object][oleautomation][dual][uuid] */ 


EXTERN_C const IID IID_IMsTscSecuredSettings;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C9D65442-A0F9-45B2-8F73-D61D2DB8CBB6")
    IMsTscSecuredSettings : public IDispatch
    {
    public:
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_StartProgram( 
            /* [in] */ BSTR pStartProgram) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_StartProgram( 
            /* [retval][out] */ BSTR *pStartProgram) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_WorkDir( 
            /* [in] */ BSTR pWorkDir) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_WorkDir( 
            /* [retval][out] */ BSTR *pWorkDir) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_FullScreen( 
            /* [in] */ long pfFullScreen) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_FullScreen( 
            /* [retval][out] */ long *pfFullScreen) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMsTscSecuredSettingsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMsTscSecuredSettings * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMsTscSecuredSettings * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMsTscSecuredSettings * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IMsTscSecuredSettings * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IMsTscSecuredSettings * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IMsTscSecuredSettings * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IMsTscSecuredSettings * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_StartProgram )( 
            IMsTscSecuredSettings * This,
            /* [in] */ BSTR pStartProgram);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_StartProgram )( 
            IMsTscSecuredSettings * This,
            /* [retval][out] */ BSTR *pStartProgram);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_WorkDir )( 
            IMsTscSecuredSettings * This,
            /* [in] */ BSTR pWorkDir);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_WorkDir )( 
            IMsTscSecuredSettings * This,
            /* [retval][out] */ BSTR *pWorkDir);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_FullScreen )( 
            IMsTscSecuredSettings * This,
            /* [in] */ long pfFullScreen);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_FullScreen )( 
            IMsTscSecuredSettings * This,
            /* [retval][out] */ long *pfFullScreen);
        
        END_INTERFACE
    } IMsTscSecuredSettingsVtbl;

    interface IMsTscSecuredSettings
    {
        CONST_VTBL struct IMsTscSecuredSettingsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMsTscSecuredSettings_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMsTscSecuredSettings_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMsTscSecuredSettings_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMsTscSecuredSettings_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IMsTscSecuredSettings_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IMsTscSecuredSettings_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IMsTscSecuredSettings_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IMsTscSecuredSettings_put_StartProgram(This,pStartProgram)	\
    ( (This)->lpVtbl -> put_StartProgram(This,pStartProgram) ) 

#define IMsTscSecuredSettings_get_StartProgram(This,pStartProgram)	\
    ( (This)->lpVtbl -> get_StartProgram(This,pStartProgram) ) 

#define IMsTscSecuredSettings_put_WorkDir(This,pWorkDir)	\
    ( (This)->lpVtbl -> put_WorkDir(This,pWorkDir) ) 

#define IMsTscSecuredSettings_get_WorkDir(This,pWorkDir)	\
    ( (This)->lpVtbl -> get_WorkDir(This,pWorkDir) ) 

#define IMsTscSecuredSettings_put_FullScreen(This,pfFullScreen)	\
    ( (This)->lpVtbl -> put_FullScreen(This,pfFullScreen) ) 

#define IMsTscSecuredSettings_get_FullScreen(This,pfFullScreen)	\
    ( (This)->lpVtbl -> get_FullScreen(This,pfFullScreen) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMsTscSecuredSettings_INTERFACE_DEFINED__ */


#ifndef __IMsRdpClientSecuredSettings_INTERFACE_DEFINED__
#define __IMsRdpClientSecuredSettings_INTERFACE_DEFINED__

/* interface IMsRdpClientSecuredSettings */
/* [object][oleautomation][dual][uuid] */ 


EXTERN_C const IID IID_IMsRdpClientSecuredSettings;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("605BEFCF-39C1-45CC-A811-068FB7BE346D")
    IMsRdpClientSecuredSettings : public IMsTscSecuredSettings
    {
    public:
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_KeyboardHookMode( 
            /* [in] */ long pkeyboardHookMode) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_KeyboardHookMode( 
            /* [retval][out] */ long *pkeyboardHookMode) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_AudioRedirectionMode( 
            /* [in] */ long pAudioRedirectionMode) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_AudioRedirectionMode( 
            /* [retval][out] */ long *pAudioRedirectionMode) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMsRdpClientSecuredSettingsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMsRdpClientSecuredSettings * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMsRdpClientSecuredSettings * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMsRdpClientSecuredSettings * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IMsRdpClientSecuredSettings * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IMsRdpClientSecuredSettings * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IMsRdpClientSecuredSettings * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IMsRdpClientSecuredSettings * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_StartProgram )( 
            IMsRdpClientSecuredSettings * This,
            /* [in] */ BSTR pStartProgram);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_StartProgram )( 
            IMsRdpClientSecuredSettings * This,
            /* [retval][out] */ BSTR *pStartProgram);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_WorkDir )( 
            IMsRdpClientSecuredSettings * This,
            /* [in] */ BSTR pWorkDir);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_WorkDir )( 
            IMsRdpClientSecuredSettings * This,
            /* [retval][out] */ BSTR *pWorkDir);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_FullScreen )( 
            IMsRdpClientSecuredSettings * This,
            /* [in] */ long pfFullScreen);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_FullScreen )( 
            IMsRdpClientSecuredSettings * This,
            /* [retval][out] */ long *pfFullScreen);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_KeyboardHookMode )( 
            IMsRdpClientSecuredSettings * This,
            /* [in] */ long pkeyboardHookMode);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_KeyboardHookMode )( 
            IMsRdpClientSecuredSettings * This,
            /* [retval][out] */ long *pkeyboardHookMode);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_AudioRedirectionMode )( 
            IMsRdpClientSecuredSettings * This,
            /* [in] */ long pAudioRedirectionMode);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AudioRedirectionMode )( 
            IMsRdpClientSecuredSettings * This,
            /* [retval][out] */ long *pAudioRedirectionMode);
        
        END_INTERFACE
    } IMsRdpClientSecuredSettingsVtbl;

    interface IMsRdpClientSecuredSettings
    {
        CONST_VTBL struct IMsRdpClientSecuredSettingsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMsRdpClientSecuredSettings_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMsRdpClientSecuredSettings_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMsRdpClientSecuredSettings_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMsRdpClientSecuredSettings_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IMsRdpClientSecuredSettings_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IMsRdpClientSecuredSettings_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IMsRdpClientSecuredSettings_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IMsRdpClientSecuredSettings_put_StartProgram(This,pStartProgram)	\
    ( (This)->lpVtbl -> put_StartProgram(This,pStartProgram) ) 

#define IMsRdpClientSecuredSettings_get_StartProgram(This,pStartProgram)	\
    ( (This)->lpVtbl -> get_StartProgram(This,pStartProgram) ) 

#define IMsRdpClientSecuredSettings_put_WorkDir(This,pWorkDir)	\
    ( (This)->lpVtbl -> put_WorkDir(This,pWorkDir) ) 

#define IMsRdpClientSecuredSettings_get_WorkDir(This,pWorkDir)	\
    ( (This)->lpVtbl -> get_WorkDir(This,pWorkDir) ) 

#define IMsRdpClientSecuredSettings_put_FullScreen(This,pfFullScreen)	\
    ( (This)->lpVtbl -> put_FullScreen(This,pfFullScreen) ) 

#define IMsRdpClientSecuredSettings_get_FullScreen(This,pfFullScreen)	\
    ( (This)->lpVtbl -> get_FullScreen(This,pfFullScreen) ) 


#define IMsRdpClientSecuredSettings_put_KeyboardHookMode(This,pkeyboardHookMode)	\
    ( (This)->lpVtbl -> put_KeyboardHookMode(This,pkeyboardHookMode) ) 

#define IMsRdpClientSecuredSettings_get_KeyboardHookMode(This,pkeyboardHookMode)	\
    ( (This)->lpVtbl -> get_KeyboardHookMode(This,pkeyboardHookMode) ) 

#define IMsRdpClientSecuredSettings_put_AudioRedirectionMode(This,pAudioRedirectionMode)	\
    ( (This)->lpVtbl -> put_AudioRedirectionMode(This,pAudioRedirectionMode) ) 

#define IMsRdpClientSecuredSettings_get_AudioRedirectionMode(This,pAudioRedirectionMode)	\
    ( (This)->lpVtbl -> get_AudioRedirectionMode(This,pAudioRedirectionMode) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMsRdpClientSecuredSettings_INTERFACE_DEFINED__ */


#ifndef __IMsTscDebug_INTERFACE_DEFINED__
#define __IMsTscDebug_INTERFACE_DEFINED__

/* interface IMsTscDebug */
/* [object][oleautomation][dual][hidden][uuid] */ 


EXTERN_C const IID IID_IMsTscDebug;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("209D0EB9-6254-47B1-9033-A98DAE55BB27")
    IMsTscDebug : public IDispatch
    {
    public:
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_HatchBitmapPDU( 
            /* [in] */ long phatchBitmapPDU) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_HatchBitmapPDU( 
            /* [retval][out] */ long *phatchBitmapPDU) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_HatchSSBOrder( 
            /* [in] */ long phatchSSBOrder) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_HatchSSBOrder( 
            /* [retval][out] */ long *phatchSSBOrder) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_HatchMembltOrder( 
            /* [in] */ long phatchMembltOrder) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_HatchMembltOrder( 
            /* [retval][out] */ long *phatchMembltOrder) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_HatchIndexPDU( 
            /* [in] */ long phatchIndexPDU) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_HatchIndexPDU( 
            /* [retval][out] */ long *phatchIndexPDU) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_LabelMemblt( 
            /* [in] */ long plabelMemblt) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_LabelMemblt( 
            /* [retval][out] */ long *plabelMemblt) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_BitmapCacheMonitor( 
            /* [in] */ long pbitmapCacheMonitor) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_BitmapCacheMonitor( 
            /* [retval][out] */ long *pbitmapCacheMonitor) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_MallocFailuresPercent( 
            /* [in] */ long pmallocFailuresPercent) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_MallocFailuresPercent( 
            /* [retval][out] */ long *pmallocFailuresPercent) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_MallocHugeFailuresPercent( 
            /* [in] */ long pmallocHugeFailuresPercent) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_MallocHugeFailuresPercent( 
            /* [retval][out] */ long *pmallocHugeFailuresPercent) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_NetThroughput( 
            /* [in] */ long NetThroughput) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_NetThroughput( 
            /* [retval][out] */ long *NetThroughput) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_CLXCmdLine( 
            /* [in] */ BSTR pCLXCmdLine) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_CLXCmdLine( 
            /* [retval][out] */ BSTR *pCLXCmdLine) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_CLXDll( 
            /* [in] */ BSTR pCLXDll) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_CLXDll( 
            /* [retval][out] */ BSTR *pCLXDll) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMsTscDebugVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMsTscDebug * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMsTscDebug * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMsTscDebug * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IMsTscDebug * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IMsTscDebug * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IMsTscDebug * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IMsTscDebug * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_HatchBitmapPDU )( 
            IMsTscDebug * This,
            /* [in] */ long phatchBitmapPDU);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HatchBitmapPDU )( 
            IMsTscDebug * This,
            /* [retval][out] */ long *phatchBitmapPDU);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_HatchSSBOrder )( 
            IMsTscDebug * This,
            /* [in] */ long phatchSSBOrder);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HatchSSBOrder )( 
            IMsTscDebug * This,
            /* [retval][out] */ long *phatchSSBOrder);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_HatchMembltOrder )( 
            IMsTscDebug * This,
            /* [in] */ long phatchMembltOrder);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HatchMembltOrder )( 
            IMsTscDebug * This,
            /* [retval][out] */ long *phatchMembltOrder);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_HatchIndexPDU )( 
            IMsTscDebug * This,
            /* [in] */ long phatchIndexPDU);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HatchIndexPDU )( 
            IMsTscDebug * This,
            /* [retval][out] */ long *phatchIndexPDU);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_LabelMemblt )( 
            IMsTscDebug * This,
            /* [in] */ long plabelMemblt);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_LabelMemblt )( 
            IMsTscDebug * This,
            /* [retval][out] */ long *plabelMemblt);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_BitmapCacheMonitor )( 
            IMsTscDebug * This,
            /* [in] */ long pbitmapCacheMonitor);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BitmapCacheMonitor )( 
            IMsTscDebug * This,
            /* [retval][out] */ long *pbitmapCacheMonitor);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_MallocFailuresPercent )( 
            IMsTscDebug * This,
            /* [in] */ long pmallocFailuresPercent);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_MallocFailuresPercent )( 
            IMsTscDebug * This,
            /* [retval][out] */ long *pmallocFailuresPercent);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_MallocHugeFailuresPercent )( 
            IMsTscDebug * This,
            /* [in] */ long pmallocHugeFailuresPercent);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_MallocHugeFailuresPercent )( 
            IMsTscDebug * This,
            /* [retval][out] */ long *pmallocHugeFailuresPercent);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_NetThroughput )( 
            IMsTscDebug * This,
            /* [in] */ long NetThroughput);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_NetThroughput )( 
            IMsTscDebug * This,
            /* [retval][out] */ long *NetThroughput);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_CLXCmdLine )( 
            IMsTscDebug * This,
            /* [in] */ BSTR pCLXCmdLine);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CLXCmdLine )( 
            IMsTscDebug * This,
            /* [retval][out] */ BSTR *pCLXCmdLine);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_CLXDll )( 
            IMsTscDebug * This,
            /* [in] */ BSTR pCLXDll);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CLXDll )( 
            IMsTscDebug * This,
            /* [retval][out] */ BSTR *pCLXDll);
        
        END_INTERFACE
    } IMsTscDebugVtbl;

    interface IMsTscDebug
    {
        CONST_VTBL struct IMsTscDebugVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMsTscDebug_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMsTscDebug_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMsTscDebug_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMsTscDebug_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IMsTscDebug_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IMsTscDebug_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IMsTscDebug_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IMsTscDebug_put_HatchBitmapPDU(This,phatchBitmapPDU)	\
    ( (This)->lpVtbl -> put_HatchBitmapPDU(This,phatchBitmapPDU) ) 

#define IMsTscDebug_get_HatchBitmapPDU(This,phatchBitmapPDU)	\
    ( (This)->lpVtbl -> get_HatchBitmapPDU(This,phatchBitmapPDU) ) 

#define IMsTscDebug_put_HatchSSBOrder(This,phatchSSBOrder)	\
    ( (This)->lpVtbl -> put_HatchSSBOrder(This,phatchSSBOrder) ) 

#define IMsTscDebug_get_HatchSSBOrder(This,phatchSSBOrder)	\
    ( (This)->lpVtbl -> get_HatchSSBOrder(This,phatchSSBOrder) ) 

#define IMsTscDebug_put_HatchMembltOrder(This,phatchMembltOrder)	\
    ( (This)->lpVtbl -> put_HatchMembltOrder(This,phatchMembltOrder) ) 

#define IMsTscDebug_get_HatchMembltOrder(This,phatchMembltOrder)	\
    ( (This)->lpVtbl -> get_HatchMembltOrder(This,phatchMembltOrder) ) 

#define IMsTscDebug_put_HatchIndexPDU(This,phatchIndexPDU)	\
    ( (This)->lpVtbl -> put_HatchIndexPDU(This,phatchIndexPDU) ) 

#define IMsTscDebug_get_HatchIndexPDU(This,phatchIndexPDU)	\
    ( (This)->lpVtbl -> get_HatchIndexPDU(This,phatchIndexPDU) ) 

#define IMsTscDebug_put_LabelMemblt(This,plabelMemblt)	\
    ( (This)->lpVtbl -> put_LabelMemblt(This,plabelMemblt) ) 

#define IMsTscDebug_get_LabelMemblt(This,plabelMemblt)	\
    ( (This)->lpVtbl -> get_LabelMemblt(This,plabelMemblt) ) 

#define IMsTscDebug_put_BitmapCacheMonitor(This,pbitmapCacheMonitor)	\
    ( (This)->lpVtbl -> put_BitmapCacheMonitor(This,pbitmapCacheMonitor) ) 

#define IMsTscDebug_get_BitmapCacheMonitor(This,pbitmapCacheMonitor)	\
    ( (This)->lpVtbl -> get_BitmapCacheMonitor(This,pbitmapCacheMonitor) ) 

#define IMsTscDebug_put_MallocFailuresPercent(This,pmallocFailuresPercent)	\
    ( (This)->lpVtbl -> put_MallocFailuresPercent(This,pmallocFailuresPercent) ) 

#define IMsTscDebug_get_MallocFailuresPercent(This,pmallocFailuresPercent)	\
    ( (This)->lpVtbl -> get_MallocFailuresPercent(This,pmallocFailuresPercent) ) 

#define IMsTscDebug_put_MallocHugeFailuresPercent(This,pmallocHugeFailuresPercent)	\
    ( (This)->lpVtbl -> put_MallocHugeFailuresPercent(This,pmallocHugeFailuresPercent) ) 

#define IMsTscDebug_get_MallocHugeFailuresPercent(This,pmallocHugeFailuresPercent)	\
    ( (This)->lpVtbl -> get_MallocHugeFailuresPercent(This,pmallocHugeFailuresPercent) ) 

#define IMsTscDebug_put_NetThroughput(This,NetThroughput)	\
    ( (This)->lpVtbl -> put_NetThroughput(This,NetThroughput) ) 

#define IMsTscDebug_get_NetThroughput(This,NetThroughput)	\
    ( (This)->lpVtbl -> get_NetThroughput(This,NetThroughput) ) 

#define IMsTscDebug_put_CLXCmdLine(This,pCLXCmdLine)	\
    ( (This)->lpVtbl -> put_CLXCmdLine(This,pCLXCmdLine) ) 

#define IMsTscDebug_get_CLXCmdLine(This,pCLXCmdLine)	\
    ( (This)->lpVtbl -> get_CLXCmdLine(This,pCLXCmdLine) ) 

#define IMsTscDebug_put_CLXDll(This,pCLXDll)	\
    ( (This)->lpVtbl -> put_CLXDll(This,pCLXDll) ) 

#define IMsTscDebug_get_CLXDll(This,pCLXDll)	\
    ( (This)->lpVtbl -> get_CLXDll(This,pCLXDll) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMsTscDebug_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_MsTscAx;

#ifdef __cplusplus

class DECLSPEC_UUID("1FB464C8-09BB-4017-A2F5-EB742F04392F")
MsTscAx;
#endif

EXTERN_C const CLSID CLSID_MsRdpClient;

#ifdef __cplusplus

class DECLSPEC_UUID("791FA017-2DE3-492E-ACC5-53C67A2B94D0")
MsRdpClient;
#endif

EXTERN_C const CLSID CLSID_MsRdpClient2;

#ifdef __cplusplus

class DECLSPEC_UUID("9059F30F-4EB1-4BD2-9FDC-36F43A218F4A")
MsRdpClient2;
#endif

EXTERN_C const CLSID CLSID_MsRdpClient3;

#ifdef __cplusplus

class DECLSPEC_UUID("7584C670-2274-4EFB-B00B-D6AABA6D3850")
MsRdpClient3;
#endif

EXTERN_C const CLSID CLSID_MsRdpClient4;

#ifdef __cplusplus

class DECLSPEC_UUID("6AE29350-321B-42BE-BBE5-12FB5270C0DE")
MsRdpClient4;
#endif
#endif /* __MSTSCLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


