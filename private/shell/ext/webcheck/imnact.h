/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.02.88 */
/* at Tue Jun 03 10:53:09 1997
 */
/* Compiler settings for imnact.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __imnact_h__
#define __imnact_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IImnAdviseAccount_FWD_DEFINED__
#define __IImnAdviseAccount_FWD_DEFINED__
typedef interface IImnAdviseAccount IImnAdviseAccount;
#endif 	/* __IImnAdviseAccount_FWD_DEFINED__ */


#ifndef __IImnAdviseMigrateServer_FWD_DEFINED__
#define __IImnAdviseMigrateServer_FWD_DEFINED__
typedef interface IImnAdviseMigrateServer IImnAdviseMigrateServer;
#endif 	/* __IImnAdviseMigrateServer_FWD_DEFINED__ */


#ifndef __IImnEnumAccounts_FWD_DEFINED__
#define __IImnEnumAccounts_FWD_DEFINED__
typedef interface IImnEnumAccounts IImnEnumAccounts;
#endif 	/* __IImnEnumAccounts_FWD_DEFINED__ */


#ifndef __IImnAccountManager_FWD_DEFINED__
#define __IImnAccountManager_FWD_DEFINED__
typedef interface IImnAccountManager IImnAccountManager;
#endif 	/* __IImnAccountManager_FWD_DEFINED__ */


#ifndef __IPropertyContainer_FWD_DEFINED__
#define __IPropertyContainer_FWD_DEFINED__
typedef interface IPropertyContainer IPropertyContainer;
#endif 	/* __IPropertyContainer_FWD_DEFINED__ */


#ifndef __IImnAccount_FWD_DEFINED__
#define __IImnAccount_FWD_DEFINED__
typedef interface IImnAccount IImnAccount;
#endif 	/* __IImnAccount_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL_itf_imnact_0000
 * at Tue Jun 03 10:53:09 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 



//=--------------------------------------------------------------------------=
// Imnact.h
//=--------------------------------------------------------------------------=
// (C) Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=

#pragma comment(lib,"uuid.lib")

//---------------------------------------------------------------------------=
// Internet Mail and News Account Manager Interfaces.

// --------------------------------------------------------------------------------
// GUIDS
// --------------------------------------------------------------------------------
// {8D4B04E1-1331-11d0-81B8-00C04FD85AB4}
DEFINE_GUID(CLSID_ImnAccountManager, 0x8d4b04e1, 0x1331, 0x11d0, 0x81, 0xb8, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {FD465484-1384-11d0-ABBD-0020AFDFD10A}
DEFINE_GUID(IID_IPropertyContainer, 0xfd465484, 0x1384, 0x11d0, 0xab, 0xbd, 0x0, 0x20, 0xaf, 0xdf, 0xd1, 0xa);

// {FD465481-1384-11d0-ABBD-0020AFDFD10A}
DEFINE_GUID(IID_IImnAccountManager, 0xfd465481, 0x1384, 0x11d0, 0xab, 0xbd, 0x0, 0x20, 0xaf, 0xdf, 0xd1, 0xa);

// {FD465482-1384-11d0-ABBD-0020AFDFD10A}
DEFINE_GUID(IID_IImnAccount, 0xfd465482, 0x1384, 0x11d0, 0xab, 0xbd, 0x0, 0x20, 0xaf, 0xdf, 0xd1, 0xa);

// {FD465483-1384-11d0-ABBD-0020AFDFD10A}
DEFINE_GUID(IID_IImnEnumAccounts, 0xfd465483, 0x1384, 0x11d0, 0xab, 0xbd, 0x0, 0x20, 0xaf, 0xdf, 0xd1, 0xa);

// {8D0AED11-1638-11d0-81B9-00C04FD85AB4}
DEFINE_GUID(IID_IImnAdviseMigrateServer, 0x8d0aed11, 0x1638, 0x11d0, 0x81, 0xb9, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {0A06BD31-166F-11d0-81B9-00C04FD85AB4}
DEFINE_GUID(IID_IImnAdviseAccount, 0xa06bd31, 0x166f, 0x11d0, 0x81, 0xb9, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// --------------------------------------------------------------------------------
// Exported C Functions
// --------------------------------------------------------------------------------
#if !defined(_IMNACCT_)
#define IMNACCTAPI DECLSPEC_IMPORT HRESULT WINAPI
#define IMNACCTAPI_(_type_) DECLSPEC_IMPORT _type_ WINAPI
#else
#define IMNACCTAPI HRESULT WINAPI
#define IMNACCTAPI_(_type_) _type_ WINAPI
#endif
#ifdef __cplusplus
extern "C" {
#endif

IMNACCTAPI HrCreateAccountManager(IImnAccountManager **ppAccountManager);
IMNACCTAPI ValidEmailAddress(LPSTR lpAddress);
IMNACCTAPI CreateAccountsFromFile(LPSTR lpFile, DWORD dwFlags);

#ifdef __cplusplus
}
#endif

// --------------------------------------------------------------------------------
// Errors
// --------------------------------------------------------------------------------
#ifndef FACILITY_INTERNET
#define FACILITY_INTERNET 12
#endif
#ifndef HR_E
#define HR_E(n) MAKE_SCODE(SEVERITY_ERROR, FACILITY_INTERNET, n)
#endif
#ifndef HR_S
#define HR_S(n) MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_INTERNET, n)
#endif
#ifndef HR_CODE
#define HR_CODE(hr) (INT)(hr & 0xffff)
#endif

// --------------------------------------------------------------------------------
// General Errors
// --------------------------------------------------------------------------------
#define E_RegCreateKeyFailed         HR_E(0xCD00)
#define E_RegQueryInfoKeyFailed      HR_E(0xCD01)
#define E_UserCancel                 HR_E(0xCD02)
#define E_RegOpenKeyFailed           HR_E(0xCD03)
#define E_RegSetValueFailed          HR_E(0xCD04)
#define E_RegDeleteKeyFailed         HR_E(0xCD05)
#define E_DuplicateAccountName       HR_E(0xCD06)
#define S_NonStandardValue           HR_S(0xCD07)
#define E_InvalidValue               HR_E(0xCD08)

// --------------------------------------------------------------------------------
// IPropertyContainer Errors
// --------------------------------------------------------------------------------
#define E_NoPropData                 HR_E(0xCDA0)
#define E_BufferTooSmall             HR_E(0xCDA1)
#define E_BadPropType                HR_E(0xCDA2)
#define E_BufferSizeMismatch         HR_E(0xCDA3)
#define E_InvalidBooleanValue        HR_E(0xCDA4)
#define E_InvalidMinMaxValue         HR_E(0xCDA5)
#define E_PropNotFound               HR_E(0xCDA6)
#define E_InvalidPropTag             HR_E(0xCDA7)
#define E_InvalidPropertySet         HR_E(0xCDA8)
#define E_EnumFinished               HR_E(0xCDA9)
#define S_NoSaveNeeded               HR_S(0xCDAA)
#define S_PasswordDeleted            HR_S(0xCDAB)
#define S_PasswordNotFound           HR_S(0xCDAC)

// --------------------------------------------------------------------------------
// ImnAccountManager errors
// --------------------------------------------------------------------------------
#define E_NoAccounts                 HR_E(0xCDD0)
#define E_BadFriendlyName            HR_E(0xCDD1)

// --------------------------------------------------------------------------------
// RAS Connection Types
// --------------------------------------------------------------------------------
#define CF_AUTO_CONNECT          FLAG02 // Automatically connect

// --------------------------------------------------------------------------------
// Property Macros
// --------------------------------------------------------------------------------
#define PROPTAG_MASK                 ((ULONG)0x0000FFFF)
#define PROPTAG_TYPE(ulPropTag)	    (PROPTYPE)(((ULONG)(ulPropTag)) & PROPTAG_MASK)
#define PROPTAG_ID(ulPropTag)		(((ULONG)(ulPropTag))>>16)
#define PROPTAG(ulPropType,ulPropID)	((((ULONG)(ulPropID))<<16)|((ULONG)(ulPropType)))
#define MAX_PROPID                   ((ULONG) 0xFFFF)

// --------------------------------------------------------------------------------
// IImnAccount Properties
// --------------------------------------------------------------------------------
#define AP_FIRST                    0X1234
// per account properties
#define AP_ACCOUNT_NAME             PROPTAG(TYPE_STRING,    AP_FIRST+1)
#define AP_ACCOUNT_FIRST            AP_ACCOUNT_NAME
#define AP_TEMP_ACCOUNT             PROPTAG(TYPE_DWORD,     AP_FIRST+3)
#define AP_LAST_UPDATED             PROPTAG(TYPE_FILETIME,  AP_FIRST+4)
#define AP_RAS_CONNECTION_TYPE      PROPTAG(TYPE_DWORD,     AP_FIRST+5)
#define AP_RAS_CONNECTOID           PROPTAG(TYPE_STRING,    AP_FIRST+6)
#define AP_RAS_CONNECTION_FLAGS     PROPTAG(TYPE_DWORD,     AP_FIRST+7)
#define AP_ACCOUNT_ID               PROPTAG(TYPE_STRING,    AP_FIRST+9)
#define AP_ACCOUNT_LAST             AP_ACCOUNT_ID

// IMAP properties
#define AP_IMAP_SERVER              PROPTAG(TYPE_STRING,    AP_FIRST+100)
#define AP_IMAP_FIRST               AP_IMAP_SERVER
#define AP_IMAP_USERNAME            PROPTAG(TYPE_STRING,    AP_FIRST+101)
#define AP_IMAP_PASSWORD            PROPTAG(TYPE_PASS,      AP_FIRST+102)
#define AP_IMAP_USE_SICILY          PROPTAG(TYPE_BOOL,      AP_FIRST+104)
#define AP_IMAP_PORT                PROPTAG(TYPE_DWORD,     AP_FIRST+105)
#define AP_IMAP_SSL                 PROPTAG(TYPE_BOOL,      AP_FIRST+106)
#define AP_IMAP_TIMEOUT             PROPTAG(TYPE_DWORD,     AP_FIRST+107)
#define AP_IMAP_ROOT_FOLDER         PROPTAG(TYPE_STRING,    AP_FIRST+108)
#define AP_IMAP_DATA_DIR            PROPTAG(TYPE_STRING,    AP_FIRST+109)
#define AP_IMAP_SKIP                PROPTAG(TYPE_BOOL,      AP_FIRST+110)
#define AP_IMAP_LAST                AP_IMAP_SKIP

// LDAP properties
#define AP_LDAP_SERVER              PROPTAG(TYPE_STRING,    AP_FIRST+200)
#define AP_LDAP_FIRST               AP_LDAP_SERVER
#define AP_LDAP_USERNAME            PROPTAG(TYPE_STRING,    AP_FIRST+201)
#define AP_LDAP_PASSWORD            PROPTAG(TYPE_PASS,      AP_FIRST+202)
#define AP_LDAP_AUTHENTICATION      PROPTAG(TYPE_DWORD,     AP_FIRST+203)
#define AP_LDAP_TIMEOUT             PROPTAG(TYPE_DWORD,     AP_FIRST+204)
#define AP_LDAP_SEARCH_RETURN       PROPTAG(TYPE_DWORD,     AP_FIRST+205)
#define AP_LDAP_SEARCH_BASE         PROPTAG(TYPE_STRING,    AP_FIRST+206)
#define AP_LDAP_SERVER_ID           PROPTAG(TYPE_DWORD,     AP_FIRST+207)
#define AP_LDAP_RESOLVE_FLAG        PROPTAG(TYPE_DWORD,     AP_FIRST+208)
#define AP_LDAP_URL                 PROPTAG(TYPE_STRING,    AP_FIRST+209)
#define AP_LDAP_PORT                PROPTAG(TYPE_DWORD,     AP_FIRST+210)
#define AP_LDAP_SSL                 PROPTAG(TYPE_BOOL,      AP_FIRST+211)
#define AP_LDAP_LOGO                PROPTAG(TYPE_STRING,    AP_FIRST+212)
#define AP_LDAP_USE_BIND_DN         PROPTAG(TYPE_DWORD,     AP_FIRST+213)
#define AP_LDAP_LAST                AP_LDAP_USE_BIND_DN

// NNTP properties
#define AP_NNTP_SERVER              PROPTAG(TYPE_STRING,    AP_FIRST+300)
#define AP_NNTP_FIRST               AP_NNTP_SERVER
#define AP_NNTP_USERNAME            PROPTAG(TYPE_STRING,    AP_FIRST+301)
#define AP_NNTP_PASSWORD            PROPTAG(TYPE_PASS,      AP_FIRST+302)
#define AP_NNTP_USE_SICILY          PROPTAG(TYPE_BOOL,      AP_FIRST+304)
#define AP_NNTP_PORT                PROPTAG(TYPE_DWORD,     AP_FIRST+305)
#define AP_NNTP_SSL                 PROPTAG(TYPE_BOOL,      AP_FIRST+306)
#define AP_NNTP_TIMEOUT             PROPTAG(TYPE_DWORD,     AP_FIRST+307)
#define AP_NNTP_DISPLAY_NAME        PROPTAG(TYPE_STRING,    AP_FIRST+308)
#define AP_NNTP_ORG_NAME            PROPTAG(TYPE_STRING,    AP_FIRST+309)
#define AP_NNTP_EMAIL_ADDRESS       PROPTAG(TYPE_STRING,    AP_FIRST+310)
#define AP_NNTP_REPLY_EMAIL_ADDRESS PROPTAG(TYPE_STRING,    AP_FIRST+311)
#define AP_NNTP_SPLIT_MESSAGES      PROPTAG(TYPE_BOOL,      AP_FIRST+312)
#define AP_NNTP_SPLIT_SIZE          PROPTAG(TYPE_DWORD,     AP_FIRST+313)
#define AP_NNTP_USE_DESCRIPTIONS    PROPTAG(TYPE_BOOL,      AP_FIRST+314)
#define AP_NNTP_DATA_DIR            PROPTAG(TYPE_STRING,    AP_FIRST+315)
#define AP_NNTP_LAST                AP_NNTP_DATA_DIR

// POP3 properties
#define AP_POP3_SERVER              PROPTAG(TYPE_STRING,    AP_FIRST+400)
#define AP_POP3_FIRST               AP_POP3_SERVER
#define AP_POP3_USERNAME            PROPTAG(TYPE_STRING,    AP_FIRST+401)
#define AP_POP3_PASSWORD            PROPTAG(TYPE_PASS,      AP_FIRST+402)
#define AP_POP3_USE_SICILY          PROPTAG(TYPE_BOOL,      AP_FIRST+404)
#define AP_POP3_PORT                PROPTAG(TYPE_DWORD,     AP_FIRST+405)
#define AP_POP3_SSL                 PROPTAG(TYPE_BOOL,      AP_FIRST+406)
#define AP_POP3_TIMEOUT             PROPTAG(TYPE_DWORD,     AP_FIRST+407)
#define AP_POP3_LEAVE_ON_SERVER     PROPTAG(TYPE_BOOL,      AP_FIRST+408)
#define AP_POP3_REMOVE_DELETED      PROPTAG(TYPE_BOOL,      AP_FIRST+409)
#define AP_POP3_REMOVE_EXPIRED      PROPTAG(TYPE_BOOL,      AP_FIRST+410)
#define AP_POP3_EXPIRE_DAYS         PROPTAG(TYPE_DWORD,     AP_FIRST+411)
#define AP_POP3_SKIP                PROPTAG(TYPE_BOOL,      AP_FIRST+412)
#define AP_POP3_LAST                AP_POP3_SKIP

// SMTP properties
#define AP_SMTP_SERVER              PROPTAG(TYPE_STRING,    AP_FIRST+500)
#define AP_SMTP_FIRST               AP_SMTP_SERVER
#define AP_SMTP_USERNAME            PROPTAG(TYPE_STRING,    AP_FIRST+501)
#define AP_SMTP_PASSWORD            PROPTAG(TYPE_PASS,      AP_FIRST+502)
#define AP_SMTP_USE_SICILY          PROPTAG(TYPE_DWORD,     AP_FIRST+504) // SMTPAUTHTYEP
#define AP_SMTP_PORT                PROPTAG(TYPE_DWORD,     AP_FIRST+505)
#define AP_SMTP_SSL                 PROPTAG(TYPE_BOOL,      AP_FIRST+506)
#define AP_SMTP_TIMEOUT             PROPTAG(TYPE_DWORD,     AP_FIRST+507)
#define AP_SMTP_DISPLAY_NAME        PROPTAG(TYPE_STRING,    AP_FIRST+508)
#define AP_SMTP_ORG_NAME            PROPTAG(TYPE_STRING,    AP_FIRST+509)
#define AP_SMTP_EMAIL_ADDRESS       PROPTAG(TYPE_STRING,    AP_FIRST+510)
#define AP_SMTP_REPLY_EMAIL_ADDRESS PROPTAG(TYPE_STRING,    AP_FIRST+511)
#define AP_SMTP_SPLIT_MESSAGES      PROPTAG(TYPE_BOOL,      AP_FIRST+512)
#define AP_SMTP_SPLIT_SIZE          PROPTAG(TYPE_DWORD,     AP_FIRST+513)
#define AP_SMTP_CERTIFICATE         PROPTAG(TYPE_BINARY,    AP_FIRST+514)
#define AP_SMTP_LAST                AP_SMTP_CERTIFICATE

// --------------------------------------------------------------------------------
// Account Flags
// --------------------------------------------------------------------------------
#define ACCT_FLAG_NEWS           ((DWORD)1)
#define ACCT_FLAG_MAIL           ((DWORD)2)
#define ACCT_FLAG_DIR_SERV       ((DWORD)4)
#define ACCT_FLAG_ALL            (ACCT_FLAG_NEWS | ACCT_FLAG_MAIL | ACCT_FLAG_DIR_SERV)

// --------------------------------------------------------------------------------
// Server Types
// --------------------------------------------------------------------------------
#define SRV_NNTP                 ((DWORD)1)
#define SRV_IMAP                 ((DWORD)2)
#define SRV_POP3                 ((DWORD)4)
#define SRV_SMTP                 ((DWORD)8)
#define SRV_LDAP                 ((DWORD)16)
#define SRV_MAIL                 ((DWORD)(SRV_IMAP | SRV_POP3 | SRV_SMTP))
#define SRV_ALL                  ((DWORD)(SRV_NNTP | SRV_IMAP | SRV_POP3 | SRV_SMTP | SRV_LDAP))

// --------------------------------------------------------------------------------
// LDAP Authentication Types
// --------------------------------------------------------------------------------
#define LDAP_AUTH_ANONYMOUS      ((DWORD)0)
#define LDAP_AUTH_PASSWORD       ((DWORD)1)
#define LDAP_AUTH_MEMBER_SYSTEM  ((DWORD)2)
#define LDAP_AUTH_MAX            ((DWORD)2)

// -----------------------------------------------------------------------------
// Account Manager Notification Types
// -----------------------------------------------------------------------------
#define AN_ACCOUNT_DELETED           WM_USER + 1
#define AN_ACCOUNT_ADDED             WM_USER + 2
#define AN_ACCOUNT_CHANGED           WM_USER + 3
#define AN_DEFAULT_CHANGED           WM_USER + 4
#define AN_SERVERTYPES_CHANGED       WM_USER + 5

// IImnAccountManager::AccountListDialog flags
// IImnAccount::ShowProperties flags
#define ACCTDLG_NO_IMAP          0x0001
#define ACCTDLG_NO_REMOVEDELETE  0x0002
#define ACCTDLG_NO_BREAKMESSAGES 0x0004
#define ACCTDLG_NO_REMOVEAFTER   0x0008
#define ACCTDLG_NO_SENDRECEIVE   0x0010
#define ACCTDLG_ALL              0x001f

// AP_RAS_CONNECTION_TYPE values
#define CONNECTION_TYPE_LAN      0
#define CONNECTION_TYPE_MANUAL   1
#define CONNECTION_TYPE_RAS      2

// IImnAccount::DoWizard flags
#define ACCT_WIZ_MIGRATE         0x0001

typedef 
enum tagSMTPAUTHTYPE
    {	SMTP_AUTH_NONE	= 0,
	SMTP_AUTH_SICILY	= SMTP_AUTH_NONE + 1,
	SMTP_AUTH_USE_POP3ORIMAP_SETTINGS	= SMTP_AUTH_SICILY + 1,
	SMTP_AUTH_USE_SMTP_SETTINGS	= SMTP_AUTH_USE_POP3ORIMAP_SETTINGS + 1
    }	SMTPAUTHTYPE;

#define	CCHMAX_ORG_NAME	( 256 )

#define	CCHMAX_DISPLAY_NAME	( 256 )

#define	CCHMAX_ACCOUNT_NAME	( 256 )

#define	CCHMAX_SERVER_NAME	( 256 )

#define	CCHMAX_PASSWORD	( 256 )

#define	CCHMAX_USERNAME	( 256 )

#define	CCHMAX_EMAIL_ADDRESS	( 256 )

#define	CCHMAX_CONNECTOID	( 256 )

#define	CCHMAX_SEARCH_BASE	( 256 )

#define	CCHMAX_ROOT_FOLDER	( 256 )



extern RPC_IF_HANDLE __MIDL_itf_imnact_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_imnact_0000_v0_0_s_ifspec;

#ifndef __IImnAdviseAccount_INTERFACE_DEFINED__
#define __IImnAdviseAccount_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IImnAdviseAccount
 * at Tue Jun 03 10:53:09 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [public][public][public][public][public][public][public][public][public][public] */ 
enum __MIDL_IImnAdviseAccount_0001
    {	ACCT_NEWS	= 0,
	ACCT_MAIL	= ACCT_NEWS + 1,
	ACCT_DIR_SERV	= ACCT_MAIL + 1,
	ACCT_LAST	= ACCT_DIR_SERV + 1
    }	ACCTTYPE;

typedef struct  tagAccountContext
    {
    ACCTTYPE AcctType;
    LPSTR pszAccountID;
    LPSTR pszOldName;
    DWORD dwServerType;
    }	ACTX;


EXTERN_C const IID IID_IImnAdviseAccount;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("0A06BD31-166F-11d0-81B9-00C04FD85AB4")
    IImnAdviseAccount : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AdviseAccount( 
            /* [in] */ DWORD dwAdviseType,
            /* [in] */ ACTX __RPC_FAR *pAcctCtx) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IImnAdviseAccountVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IImnAdviseAccount __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IImnAdviseAccount __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IImnAdviseAccount __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AdviseAccount )( 
            IImnAdviseAccount __RPC_FAR * This,
            /* [in] */ DWORD dwAdviseType,
            /* [in] */ ACTX __RPC_FAR *pAcctCtx);
        
        END_INTERFACE
    } IImnAdviseAccountVtbl;

    interface IImnAdviseAccount
    {
        CONST_VTBL struct IImnAdviseAccountVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IImnAdviseAccount_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IImnAdviseAccount_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IImnAdviseAccount_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IImnAdviseAccount_AdviseAccount(This,dwAdviseType,pAcctCtx)	\
    (This)->lpVtbl -> AdviseAccount(This,dwAdviseType,pAcctCtx)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IImnAdviseAccount_AdviseAccount_Proxy( 
    IImnAdviseAccount __RPC_FAR * This,
    /* [in] */ DWORD dwAdviseType,
    /* [in] */ ACTX __RPC_FAR *pAcctCtx);


void __RPC_STUB IImnAdviseAccount_AdviseAccount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IImnAdviseAccount_INTERFACE_DEFINED__ */


#ifndef __IImnAdviseMigrateServer_INTERFACE_DEFINED__
#define __IImnAdviseMigrateServer_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IImnAdviseMigrateServer
 * at Tue Jun 03 10:53:09 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IImnAdviseMigrateServer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("8D0AED11-1638-11d0-81B9-00C04FD85AB4")
    IImnAdviseMigrateServer : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE MigrateServer( 
            /* [in] */ DWORD dwSrvType,
            /* [in] */ IImnAccount __RPC_FAR *pAccount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IImnAdviseMigrateServerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IImnAdviseMigrateServer __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IImnAdviseMigrateServer __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IImnAdviseMigrateServer __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MigrateServer )( 
            IImnAdviseMigrateServer __RPC_FAR * This,
            /* [in] */ DWORD dwSrvType,
            /* [in] */ IImnAccount __RPC_FAR *pAccount);
        
        END_INTERFACE
    } IImnAdviseMigrateServerVtbl;

    interface IImnAdviseMigrateServer
    {
        CONST_VTBL struct IImnAdviseMigrateServerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IImnAdviseMigrateServer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IImnAdviseMigrateServer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IImnAdviseMigrateServer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IImnAdviseMigrateServer_MigrateServer(This,dwSrvType,pAccount)	\
    (This)->lpVtbl -> MigrateServer(This,dwSrvType,pAccount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IImnAdviseMigrateServer_MigrateServer_Proxy( 
    IImnAdviseMigrateServer __RPC_FAR * This,
    /* [in] */ DWORD dwSrvType,
    /* [in] */ IImnAccount __RPC_FAR *pAccount);


void __RPC_STUB IImnAdviseMigrateServer_MigrateServer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IImnAdviseMigrateServer_INTERFACE_DEFINED__ */


#ifndef __IImnEnumAccounts_INTERFACE_DEFINED__
#define __IImnEnumAccounts_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IImnEnumAccounts
 * at Tue Jun 03 10:53:09 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IImnEnumAccounts;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("FD465483-1384-11d0-ABBD-0020AFDFD10A")
    IImnEnumAccounts : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCount( 
            /* [out] */ ULONG __RPC_FAR *pcItems) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SortByAccountName( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNext( 
            /* [out] */ IImnAccount __RPC_FAR *__RPC_FAR *ppAccount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IImnEnumAccountsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IImnEnumAccounts __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IImnEnumAccounts __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IImnEnumAccounts __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCount )( 
            IImnEnumAccounts __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcItems);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SortByAccountName )( 
            IImnEnumAccounts __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNext )( 
            IImnEnumAccounts __RPC_FAR * This,
            /* [out] */ IImnAccount __RPC_FAR *__RPC_FAR *ppAccount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IImnEnumAccounts __RPC_FAR * This);
        
        END_INTERFACE
    } IImnEnumAccountsVtbl;

    interface IImnEnumAccounts
    {
        CONST_VTBL struct IImnEnumAccountsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IImnEnumAccounts_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IImnEnumAccounts_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IImnEnumAccounts_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IImnEnumAccounts_GetCount(This,pcItems)	\
    (This)->lpVtbl -> GetCount(This,pcItems)

#define IImnEnumAccounts_SortByAccountName(This)	\
    (This)->lpVtbl -> SortByAccountName(This)

#define IImnEnumAccounts_GetNext(This,ppAccount)	\
    (This)->lpVtbl -> GetNext(This,ppAccount)

#define IImnEnumAccounts_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IImnEnumAccounts_GetCount_Proxy( 
    IImnEnumAccounts __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pcItems);


void __RPC_STUB IImnEnumAccounts_GetCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImnEnumAccounts_SortByAccountName_Proxy( 
    IImnEnumAccounts __RPC_FAR * This);


void __RPC_STUB IImnEnumAccounts_SortByAccountName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImnEnumAccounts_GetNext_Proxy( 
    IImnEnumAccounts __RPC_FAR * This,
    /* [out] */ IImnAccount __RPC_FAR *__RPC_FAR *ppAccount);


void __RPC_STUB IImnEnumAccounts_GetNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImnEnumAccounts_Reset_Proxy( 
    IImnEnumAccounts __RPC_FAR * This);


void __RPC_STUB IImnEnumAccounts_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IImnEnumAccounts_INTERFACE_DEFINED__ */


#ifndef __IImnAccountManager_INTERFACE_DEFINED__
#define __IImnAccountManager_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IImnAccountManager
 * at Tue Jun 03 10:53:09 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object] */ 


typedef struct  tagACCTLISTINFO
    {
    DWORD cbSize;
    ACCTTYPE AcctTypeInit;
    DWORD dwAcctFlags;
    DWORD dwFlags;
    }	ACCTLISTINFO;


EXTERN_C const IID IID_IImnAccountManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("FD465481-1384-11d0-ABBD-0020AFDFD10A")
    IImnAccountManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Init( 
            /* [in] */ IImnAdviseMigrateServer __RPC_FAR *pAdviseMigrateServer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateAccountObject( 
            /* [in] */ ACCTTYPE AcctType,
            /* [out] */ IImnAccount __RPC_FAR *__RPC_FAR *ppAccount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Enumerate( 
            /* [in] */ DWORD dwSrvTypes,
            /* [out] */ IImnEnumAccounts __RPC_FAR *__RPC_FAR *ppEnumAccounts) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAccountCount( 
            /* [in] */ ACCTTYPE AcctType,
            /* [out] */ ULONG __RPC_FAR *pcServers) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindAccount( 
            /* [in] */ DWORD dwPropTag,
            /* [in] */ LPCTSTR pszSearchData,
            /* [out] */ IImnAccount __RPC_FAR *__RPC_FAR *ppAccount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDefaultAccount( 
            /* [in] */ ACCTTYPE AcctType,
            /* [out] */ IImnAccount __RPC_FAR *__RPC_FAR *ppAccount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDefaultAccountName( 
            /* [in] */ ACCTTYPE AcctType,
            /* [ref][in] */ LPTSTR pszAccount,
            /* [in] */ ULONG cchMax) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ProcessNotification( 
            /* [in] */ UINT uMsg,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ValidateDefaultSendAccount( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AccountListDialog( 
            /* [in] */ HWND hwnd,
            /* [in] */ ACCTLISTINFO __RPC_FAR *pinfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Advise( 
            /* [in] */ IImnAdviseAccount __RPC_FAR *pAdviseAccount,
            /* [out] */ DWORD __RPC_FAR *pdwConnection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Unadvise( 
            /* [in] */ DWORD dwConnection) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IImnAccountManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IImnAccountManager __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IImnAccountManager __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IImnAccountManager __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Init )( 
            IImnAccountManager __RPC_FAR * This,
            /* [in] */ IImnAdviseMigrateServer __RPC_FAR *pAdviseMigrateServer);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateAccountObject )( 
            IImnAccountManager __RPC_FAR * This,
            /* [in] */ ACCTTYPE AcctType,
            /* [out] */ IImnAccount __RPC_FAR *__RPC_FAR *ppAccount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Enumerate )( 
            IImnAccountManager __RPC_FAR * This,
            /* [in] */ DWORD dwSrvTypes,
            /* [out] */ IImnEnumAccounts __RPC_FAR *__RPC_FAR *ppEnumAccounts);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAccountCount )( 
            IImnAccountManager __RPC_FAR * This,
            /* [in] */ ACCTTYPE AcctType,
            /* [out] */ ULONG __RPC_FAR *pcServers);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindAccount )( 
            IImnAccountManager __RPC_FAR * This,
            /* [in] */ DWORD dwPropTag,
            /* [in] */ LPCTSTR pszSearchData,
            /* [out] */ IImnAccount __RPC_FAR *__RPC_FAR *ppAccount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDefaultAccount )( 
            IImnAccountManager __RPC_FAR * This,
            /* [in] */ ACCTTYPE AcctType,
            /* [out] */ IImnAccount __RPC_FAR *__RPC_FAR *ppAccount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDefaultAccountName )( 
            IImnAccountManager __RPC_FAR * This,
            /* [in] */ ACCTTYPE AcctType,
            /* [ref][in] */ LPTSTR pszAccount,
            /* [in] */ ULONG cchMax);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ProcessNotification )( 
            IImnAccountManager __RPC_FAR * This,
            /* [in] */ UINT uMsg,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ValidateDefaultSendAccount )( 
            IImnAccountManager __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AccountListDialog )( 
            IImnAccountManager __RPC_FAR * This,
            /* [in] */ HWND hwnd,
            /* [in] */ ACCTLISTINFO __RPC_FAR *pinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Advise )( 
            IImnAccountManager __RPC_FAR * This,
            /* [in] */ IImnAdviseAccount __RPC_FAR *pAdviseAccount,
            /* [out] */ DWORD __RPC_FAR *pdwConnection);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Unadvise )( 
            IImnAccountManager __RPC_FAR * This,
            /* [in] */ DWORD dwConnection);
        
        END_INTERFACE
    } IImnAccountManagerVtbl;

    interface IImnAccountManager
    {
        CONST_VTBL struct IImnAccountManagerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IImnAccountManager_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IImnAccountManager_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IImnAccountManager_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IImnAccountManager_Init(This,pAdviseMigrateServer)	\
    (This)->lpVtbl -> Init(This,pAdviseMigrateServer)

#define IImnAccountManager_CreateAccountObject(This,AcctType,ppAccount)	\
    (This)->lpVtbl -> CreateAccountObject(This,AcctType,ppAccount)

#define IImnAccountManager_Enumerate(This,dwSrvTypes,ppEnumAccounts)	\
    (This)->lpVtbl -> Enumerate(This,dwSrvTypes,ppEnumAccounts)

#define IImnAccountManager_GetAccountCount(This,AcctType,pcServers)	\
    (This)->lpVtbl -> GetAccountCount(This,AcctType,pcServers)

#define IImnAccountManager_FindAccount(This,dwPropTag,pszSearchData,ppAccount)	\
    (This)->lpVtbl -> FindAccount(This,dwPropTag,pszSearchData,ppAccount)

#define IImnAccountManager_GetDefaultAccount(This,AcctType,ppAccount)	\
    (This)->lpVtbl -> GetDefaultAccount(This,AcctType,ppAccount)

#define IImnAccountManager_GetDefaultAccountName(This,AcctType,pszAccount,cchMax)	\
    (This)->lpVtbl -> GetDefaultAccountName(This,AcctType,pszAccount,cchMax)

#define IImnAccountManager_ProcessNotification(This,uMsg,wParam,lParam)	\
    (This)->lpVtbl -> ProcessNotification(This,uMsg,wParam,lParam)

#define IImnAccountManager_ValidateDefaultSendAccount(This)	\
    (This)->lpVtbl -> ValidateDefaultSendAccount(This)

#define IImnAccountManager_AccountListDialog(This,hwnd,pinfo)	\
    (This)->lpVtbl -> AccountListDialog(This,hwnd,pinfo)

#define IImnAccountManager_Advise(This,pAdviseAccount,pdwConnection)	\
    (This)->lpVtbl -> Advise(This,pAdviseAccount,pdwConnection)

#define IImnAccountManager_Unadvise(This,dwConnection)	\
    (This)->lpVtbl -> Unadvise(This,dwConnection)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IImnAccountManager_Init_Proxy( 
    IImnAccountManager __RPC_FAR * This,
    /* [in] */ IImnAdviseMigrateServer __RPC_FAR *pAdviseMigrateServer);


void __RPC_STUB IImnAccountManager_Init_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImnAccountManager_CreateAccountObject_Proxy( 
    IImnAccountManager __RPC_FAR * This,
    /* [in] */ ACCTTYPE AcctType,
    /* [out] */ IImnAccount __RPC_FAR *__RPC_FAR *ppAccount);


void __RPC_STUB IImnAccountManager_CreateAccountObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImnAccountManager_Enumerate_Proxy( 
    IImnAccountManager __RPC_FAR * This,
    /* [in] */ DWORD dwSrvTypes,
    /* [out] */ IImnEnumAccounts __RPC_FAR *__RPC_FAR *ppEnumAccounts);


void __RPC_STUB IImnAccountManager_Enumerate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImnAccountManager_GetAccountCount_Proxy( 
    IImnAccountManager __RPC_FAR * This,
    /* [in] */ ACCTTYPE AcctType,
    /* [out] */ ULONG __RPC_FAR *pcServers);


void __RPC_STUB IImnAccountManager_GetAccountCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImnAccountManager_FindAccount_Proxy( 
    IImnAccountManager __RPC_FAR * This,
    /* [in] */ DWORD dwPropTag,
    /* [in] */ LPCTSTR pszSearchData,
    /* [out] */ IImnAccount __RPC_FAR *__RPC_FAR *ppAccount);


void __RPC_STUB IImnAccountManager_FindAccount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImnAccountManager_GetDefaultAccount_Proxy( 
    IImnAccountManager __RPC_FAR * This,
    /* [in] */ ACCTTYPE AcctType,
    /* [out] */ IImnAccount __RPC_FAR *__RPC_FAR *ppAccount);


void __RPC_STUB IImnAccountManager_GetDefaultAccount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImnAccountManager_GetDefaultAccountName_Proxy( 
    IImnAccountManager __RPC_FAR * This,
    /* [in] */ ACCTTYPE AcctType,
    /* [ref][in] */ LPTSTR pszAccount,
    /* [in] */ ULONG cchMax);


void __RPC_STUB IImnAccountManager_GetDefaultAccountName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImnAccountManager_ProcessNotification_Proxy( 
    IImnAccountManager __RPC_FAR * This,
    /* [in] */ UINT uMsg,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam);


void __RPC_STUB IImnAccountManager_ProcessNotification_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImnAccountManager_ValidateDefaultSendAccount_Proxy( 
    IImnAccountManager __RPC_FAR * This);


void __RPC_STUB IImnAccountManager_ValidateDefaultSendAccount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImnAccountManager_AccountListDialog_Proxy( 
    IImnAccountManager __RPC_FAR * This,
    /* [in] */ HWND hwnd,
    /* [in] */ ACCTLISTINFO __RPC_FAR *pinfo);


void __RPC_STUB IImnAccountManager_AccountListDialog_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImnAccountManager_Advise_Proxy( 
    IImnAccountManager __RPC_FAR * This,
    /* [in] */ IImnAdviseAccount __RPC_FAR *pAdviseAccount,
    /* [out] */ DWORD __RPC_FAR *pdwConnection);


void __RPC_STUB IImnAccountManager_Advise_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImnAccountManager_Unadvise_Proxy( 
    IImnAccountManager __RPC_FAR * This,
    /* [in] */ DWORD dwConnection);


void __RPC_STUB IImnAccountManager_Unadvise_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IImnAccountManager_INTERFACE_DEFINED__ */


#ifndef __IPropertyContainer_INTERFACE_DEFINED__
#define __IPropertyContainer_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IPropertyContainer
 * at Tue Jun 03 10:53:09 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [public] */ 
enum __MIDL_IPropertyContainer_0001
    {	TYPE_ERROR	= 1000,
	TYPE_DWORD	= TYPE_ERROR + 1,
	TYPE_LONG	= TYPE_DWORD + 1,
	TYPE_WORD	= TYPE_LONG + 1,
	TYPE_SHORT	= TYPE_WORD + 1,
	TYPE_BYTE	= TYPE_SHORT + 1,
	TYPE_CHAR	= TYPE_BYTE + 1,
	TYPE_FILETIME	= TYPE_CHAR + 1,
	TYPE_STRING	= TYPE_FILETIME + 1,
	TYPE_BINARY	= TYPE_STRING + 1,
	TYPE_FLAGS	= TYPE_BINARY + 1,
	TYPE_STREAM	= TYPE_FLAGS + 1,
	TYPE_WSTRING	= TYPE_STREAM + 1,
	TYPE_BOOL	= TYPE_WSTRING + 1,
	TYPE_PASS	= TYPE_BOOL + 1,
	TYPE_LAST	= TYPE_PASS + 1
    }	PROPTYPE;


EXTERN_C const IID IID_IPropertyContainer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("FD465484-1384-11d0-ABBD-0020AFDFD10A")
    IPropertyContainer : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetProp( 
            /* [in] */ DWORD dwPropTag,
            /* [ref][size_is][in] */ BYTE __RPC_FAR *pb,
            /* [in] */ ULONG __RPC_FAR *pcb) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPropDw( 
            /* [in] */ DWORD dwPropTag,
            /* [out] */ DWORD __RPC_FAR *pdw) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPropSz( 
            /* [in] */ DWORD dwPropTag,
            /* [ref][in] */ LPSTR psz,
            /* [in] */ ULONG cchMax) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProp( 
            /* [in] */ DWORD dwPropTag,
            /* [size_is][in] */ BYTE __RPC_FAR *pb,
            /* [in] */ ULONG cb) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPropDw( 
            /* [in] */ DWORD dwPropTag,
            /* [in] */ DWORD dw) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPropSz( 
            /* [in] */ DWORD dwPropTag,
            /* [in] */ LPSTR psz) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPropertyContainerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IPropertyContainer __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IPropertyContainer __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IPropertyContainer __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetProp )( 
            IPropertyContainer __RPC_FAR * This,
            /* [in] */ DWORD dwPropTag,
            /* [ref][size_is][in] */ BYTE __RPC_FAR *pb,
            /* [in] */ ULONG __RPC_FAR *pcb);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPropDw )( 
            IPropertyContainer __RPC_FAR * This,
            /* [in] */ DWORD dwPropTag,
            /* [out] */ DWORD __RPC_FAR *pdw);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPropSz )( 
            IPropertyContainer __RPC_FAR * This,
            /* [in] */ DWORD dwPropTag,
            /* [ref][in] */ LPSTR psz,
            /* [in] */ ULONG cchMax);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetProp )( 
            IPropertyContainer __RPC_FAR * This,
            /* [in] */ DWORD dwPropTag,
            /* [size_is][in] */ BYTE __RPC_FAR *pb,
            /* [in] */ ULONG cb);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPropDw )( 
            IPropertyContainer __RPC_FAR * This,
            /* [in] */ DWORD dwPropTag,
            /* [in] */ DWORD dw);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPropSz )( 
            IPropertyContainer __RPC_FAR * This,
            /* [in] */ DWORD dwPropTag,
            /* [in] */ LPSTR psz);
        
        END_INTERFACE
    } IPropertyContainerVtbl;

    interface IPropertyContainer
    {
        CONST_VTBL struct IPropertyContainerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPropertyContainer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPropertyContainer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPropertyContainer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPropertyContainer_GetProp(This,dwPropTag,pb,pcb)	\
    (This)->lpVtbl -> GetProp(This,dwPropTag,pb,pcb)

#define IPropertyContainer_GetPropDw(This,dwPropTag,pdw)	\
    (This)->lpVtbl -> GetPropDw(This,dwPropTag,pdw)

#define IPropertyContainer_GetPropSz(This,dwPropTag,psz,cchMax)	\
    (This)->lpVtbl -> GetPropSz(This,dwPropTag,psz,cchMax)

#define IPropertyContainer_SetProp(This,dwPropTag,pb,cb)	\
    (This)->lpVtbl -> SetProp(This,dwPropTag,pb,cb)

#define IPropertyContainer_SetPropDw(This,dwPropTag,dw)	\
    (This)->lpVtbl -> SetPropDw(This,dwPropTag,dw)

#define IPropertyContainer_SetPropSz(This,dwPropTag,psz)	\
    (This)->lpVtbl -> SetPropSz(This,dwPropTag,psz)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPropertyContainer_GetProp_Proxy( 
    IPropertyContainer __RPC_FAR * This,
    /* [in] */ DWORD dwPropTag,
    /* [ref][size_is][in] */ BYTE __RPC_FAR *pb,
    /* [in] */ ULONG __RPC_FAR *pcb);


void __RPC_STUB IPropertyContainer_GetProp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPropertyContainer_GetPropDw_Proxy( 
    IPropertyContainer __RPC_FAR * This,
    /* [in] */ DWORD dwPropTag,
    /* [out] */ DWORD __RPC_FAR *pdw);


void __RPC_STUB IPropertyContainer_GetPropDw_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPropertyContainer_GetPropSz_Proxy( 
    IPropertyContainer __RPC_FAR * This,
    /* [in] */ DWORD dwPropTag,
    /* [ref][in] */ LPSTR psz,
    /* [in] */ ULONG cchMax);


void __RPC_STUB IPropertyContainer_GetPropSz_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPropertyContainer_SetProp_Proxy( 
    IPropertyContainer __RPC_FAR * This,
    /* [in] */ DWORD dwPropTag,
    /* [size_is][in] */ BYTE __RPC_FAR *pb,
    /* [in] */ ULONG cb);


void __RPC_STUB IPropertyContainer_SetProp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPropertyContainer_SetPropDw_Proxy( 
    IPropertyContainer __RPC_FAR * This,
    /* [in] */ DWORD dwPropTag,
    /* [in] */ DWORD dw);


void __RPC_STUB IPropertyContainer_SetPropDw_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPropertyContainer_SetPropSz_Proxy( 
    IPropertyContainer __RPC_FAR * This,
    /* [in] */ DWORD dwPropTag,
    /* [in] */ LPSTR psz);


void __RPC_STUB IPropertyContainer_SetPropSz_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPropertyContainer_INTERFACE_DEFINED__ */


#ifndef __IImnAccount_INTERFACE_DEFINED__
#define __IImnAccount_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IImnAccount
 * at Tue Jun 03 10:53:09 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IImnAccount;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("FD465482-1384-11d0-ABBD-0020AFDFD10A")
    IImnAccount : public IPropertyContainer
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Exist( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAsDefault( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Delete( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SaveChanges( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAccountType( 
            /* [out] */ ACCTTYPE __RPC_FAR *pAcctType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetServerTypes( 
            /* [out] */ DWORD __RPC_FAR *pdwSrvTypes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShowProperties( 
            /* [in] */ HWND hwnd,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ValidateProperty( 
            /* [in] */ DWORD dwPropTag,
            /* [size_is][in] */ BYTE __RPC_FAR *pb,
            /* [in] */ ULONG cb) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DoWizard( 
            /* [in] */ HWND hwnd,
            /* [in] */ DWORD dwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IImnAccountVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IImnAccount __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IImnAccount __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IImnAccount __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetProp )( 
            IImnAccount __RPC_FAR * This,
            /* [in] */ DWORD dwPropTag,
            /* [ref][size_is][in] */ BYTE __RPC_FAR *pb,
            /* [in] */ ULONG __RPC_FAR *pcb);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPropDw )( 
            IImnAccount __RPC_FAR * This,
            /* [in] */ DWORD dwPropTag,
            /* [out] */ DWORD __RPC_FAR *pdw);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPropSz )( 
            IImnAccount __RPC_FAR * This,
            /* [in] */ DWORD dwPropTag,
            /* [ref][in] */ LPSTR psz,
            /* [in] */ ULONG cchMax);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetProp )( 
            IImnAccount __RPC_FAR * This,
            /* [in] */ DWORD dwPropTag,
            /* [size_is][in] */ BYTE __RPC_FAR *pb,
            /* [in] */ ULONG cb);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPropDw )( 
            IImnAccount __RPC_FAR * This,
            /* [in] */ DWORD dwPropTag,
            /* [in] */ DWORD dw);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPropSz )( 
            IImnAccount __RPC_FAR * This,
            /* [in] */ DWORD dwPropTag,
            /* [in] */ LPSTR psz);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Exist )( 
            IImnAccount __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAsDefault )( 
            IImnAccount __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )( 
            IImnAccount __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SaveChanges )( 
            IImnAccount __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAccountType )( 
            IImnAccount __RPC_FAR * This,
            /* [out] */ ACCTTYPE __RPC_FAR *pAcctType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetServerTypes )( 
            IImnAccount __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwSrvTypes);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ShowProperties )( 
            IImnAccount __RPC_FAR * This,
            /* [in] */ HWND hwnd,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ValidateProperty )( 
            IImnAccount __RPC_FAR * This,
            /* [in] */ DWORD dwPropTag,
            /* [size_is][in] */ BYTE __RPC_FAR *pb,
            /* [in] */ ULONG cb);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DoWizard )( 
            IImnAccount __RPC_FAR * This,
            /* [in] */ HWND hwnd,
            /* [in] */ DWORD dwFlags);
        
        END_INTERFACE
    } IImnAccountVtbl;

    interface IImnAccount
    {
        CONST_VTBL struct IImnAccountVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IImnAccount_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IImnAccount_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IImnAccount_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IImnAccount_GetProp(This,dwPropTag,pb,pcb)	\
    (This)->lpVtbl -> GetProp(This,dwPropTag,pb,pcb)

#define IImnAccount_GetPropDw(This,dwPropTag,pdw)	\
    (This)->lpVtbl -> GetPropDw(This,dwPropTag,pdw)

#define IImnAccount_GetPropSz(This,dwPropTag,psz,cchMax)	\
    (This)->lpVtbl -> GetPropSz(This,dwPropTag,psz,cchMax)

#define IImnAccount_SetProp(This,dwPropTag,pb,cb)	\
    (This)->lpVtbl -> SetProp(This,dwPropTag,pb,cb)

#define IImnAccount_SetPropDw(This,dwPropTag,dw)	\
    (This)->lpVtbl -> SetPropDw(This,dwPropTag,dw)

#define IImnAccount_SetPropSz(This,dwPropTag,psz)	\
    (This)->lpVtbl -> SetPropSz(This,dwPropTag,psz)


#define IImnAccount_Exist(This)	\
    (This)->lpVtbl -> Exist(This)

#define IImnAccount_SetAsDefault(This)	\
    (This)->lpVtbl -> SetAsDefault(This)

#define IImnAccount_Delete(This)	\
    (This)->lpVtbl -> Delete(This)

#define IImnAccount_SaveChanges(This)	\
    (This)->lpVtbl -> SaveChanges(This)

#define IImnAccount_GetAccountType(This,pAcctType)	\
    (This)->lpVtbl -> GetAccountType(This,pAcctType)

#define IImnAccount_GetServerTypes(This,pdwSrvTypes)	\
    (This)->lpVtbl -> GetServerTypes(This,pdwSrvTypes)

#define IImnAccount_ShowProperties(This,hwnd,dwFlags)	\
    (This)->lpVtbl -> ShowProperties(This,hwnd,dwFlags)

#define IImnAccount_ValidateProperty(This,dwPropTag,pb,cb)	\
    (This)->lpVtbl -> ValidateProperty(This,dwPropTag,pb,cb)

#define IImnAccount_DoWizard(This,hwnd,dwFlags)	\
    (This)->lpVtbl -> DoWizard(This,hwnd,dwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IImnAccount_Exist_Proxy( 
    IImnAccount __RPC_FAR * This);


void __RPC_STUB IImnAccount_Exist_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImnAccount_SetAsDefault_Proxy( 
    IImnAccount __RPC_FAR * This);


void __RPC_STUB IImnAccount_SetAsDefault_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImnAccount_Delete_Proxy( 
    IImnAccount __RPC_FAR * This);


void __RPC_STUB IImnAccount_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImnAccount_SaveChanges_Proxy( 
    IImnAccount __RPC_FAR * This);


void __RPC_STUB IImnAccount_SaveChanges_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImnAccount_GetAccountType_Proxy( 
    IImnAccount __RPC_FAR * This,
    /* [out] */ ACCTTYPE __RPC_FAR *pAcctType);


void __RPC_STUB IImnAccount_GetAccountType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImnAccount_GetServerTypes_Proxy( 
    IImnAccount __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwSrvTypes);


void __RPC_STUB IImnAccount_GetServerTypes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImnAccount_ShowProperties_Proxy( 
    IImnAccount __RPC_FAR * This,
    /* [in] */ HWND hwnd,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IImnAccount_ShowProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImnAccount_ValidateProperty_Proxy( 
    IImnAccount __RPC_FAR * This,
    /* [in] */ DWORD dwPropTag,
    /* [size_is][in] */ BYTE __RPC_FAR *pb,
    /* [in] */ ULONG cb);


void __RPC_STUB IImnAccount_ValidateProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImnAccount_DoWizard_Proxy( 
    IImnAccount __RPC_FAR * This,
    /* [in] */ HWND hwnd,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IImnAccount_DoWizard_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IImnAccount_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  HWND_UserSize(     unsigned long __RPC_FAR *, unsigned long            , HWND __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HWND_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HWND __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HWND_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HWND __RPC_FAR * ); 
void                      __RPC_USER  HWND_UserFree(     unsigned long __RPC_FAR *, HWND __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
