#ifndef _WZCSAPI_H
#define _WZCSAPI_H

# ifdef __cplusplus
extern "C" {
# endif

#include <windot11.h>

#ifndef MIDL_PASS
#include <ntddndis.h>
#endif

/* Defines */
#define NWB_AUTHMODE_MASK       0x03
#define NWB_SELCATEG_MASK       0x1C

#define NWB_SET_AUTHMODE(pNWB, nAM)     (pNWB)->Reserved[1] = (UCHAR)(((pNWB)->Reserved[1] & ~NWB_AUTHMODE_MASK) | ((nAM) & NWB_AUTHMODE_MASK))
#define NWB_GET_AUTHMODE(pNWB)          ((pNWB)->Reserved[1] & NWB_AUTHMODE_MASK)
#define NWB_SET_SELCATEG(pNWB, nSC)     (pNWB)->Reserved[1] = (UCHAR)(((pNWB)->Reserved[1] & ~NWB_SELCATEG_MASK) | (((nSC)<<2) & NWB_SELCATEG_MASK))
#define NWB_GET_SELCATEG(pNWB)          (((pNWB)->Reserved[1] & NWB_SELCATEG_MASK)>>2)

#define WZCCTL_MAX_WEPK_MATERIAL   32
#define WZCCTL_WEPK_PRESENT        0x0001
#define WZCCTL_WEPK_XFORMAT        0x0002
#define WZCCTL_VOLATILE            0x0004
#define WZCCTL_POLICY              0x0008
#define WZCCTL_ONEX_ENABLED        0x0010

#ifndef MIDL_PASS

#define WZCDLG_IS_WZC(x)         (((x) & 0x00010000) == 0x00010000)
#define WZCDLG_FAILED            0x00010001

#endif

#define INTF_ALL            0xffffffff

#define INTF_ALL_FLAGS      0x0000ffff
#define INTF_CM_MASK        0x00000007
#define INTF_ENABLED        0x00008000
#define INTF_FALLBACK       0x00004000
#define INTF_OIDSSUPP       0x00002000
#define INTF_VOLATILE       0x00001000
#define INTF_POLICY         0x00000800

#define INTF_DESCR          0x00010000
#define INTF_NDISMEDIA      0x00020000
#define INTF_PREFLIST       0x00040000
#define INTF_CAPABILITIES   0x00080000

#define INTF_ALL_OIDS       0xfff00000
#define INTF_HANDLE         0x00100000
#define INTF_INFRAMODE      0x00200000
#define INTF_AUTHMODE       0x00400000
#define INTF_WEPSTATUS      0x00800000
#define INTF_SSID           0x01000000
#define INTF_BSSID          0x02000000
#define INTF_BSSIDLIST      0x04000000
#define INTF_LIST_SCAN      0x08000000
#define INTF_ADDWEPKEY      0x10000000
#define INTF_REMWEPKEY      0x20000000
#define INTF_LDDEFWKEY      0x40000000

#define INTFCTL_CM_MASK     0x0007
#define INTFCTL_ENABLED     0x8000
#define INTFCTL_FALLBACK    0x4000
#define INTFCTL_OIDSSUPP    0x2000
#define INTFCTL_VOLATILE    0x1000
#define INTFCTL_POLICY      0x0800

#define INTFCAP_MAX_CIPHER_MASK     0x000000ff
#define INTFCAP_SSN                 0x00000100

#define TMMS_DEFAULT_TR	0x00000bb8
#define TMMS_DEFAULT_TC 0x0000ea60 
#define TMMS_DEFAULT_TP 0x000007d0
#define TMMS_DEFAULT_TF 0x0000ea60
#define TMMS_DEFAULT_TD 0x00001388

#define WZC_CTXT_LOGGING_ON      0x00000001

#define WZC_CONTEXT_CTL_LOG         0x00000001
#define WZC_CONTEXT_CTL_TIMER_TR	0x00000002
#define WZC_CONTEXT_CTL_TIMER_TC	0x00000004
#define WZC_CONTEXT_CTL_TIMER_TP	0x00000008
#define WZC_CONTEXT_CTL_TIMER_TF	0x00000010
#define WZC_CONTEXT_CTL_TIMER_TD	0x00000020

#define RpcCAlloc(nBytes)   MIDL_user_allocate(nBytes)
#define RpcFree(pMem)       MIDL_user_free(pMem)

#define EAPOL_DISABLED                  0
#define EAPOL_ENABLED                   0x80000000

#define EAPOL_MACHINE_AUTH_DISABLED     0
#define EAPOL_MACHINE_AUTH_ENABLED      0x40000000

#define EAPOL_GUEST_AUTH_DISABLED       0
#define EAPOL_GUEST_AUTH_ENABLED        0x20000000

#define EAP_TYPE_MD5                    4
#define EAP_TYPE_TLS                    13
#define EAP_TYPE_PEAP                   25
#define EAP_TYPE_MSCHAPv2               26

#define DEFAULT_EAP_TYPE                EAP_TYPE_TLS
#define DEFAULT_EAPOL_STATE             EAPOL_ENABLED
#define DEFAULT_MACHINE_AUTH_STATE      EAPOL_MACHINE_AUTH_ENABLED
#define DEFAULT_GUEST_AUTH_STATE        EAPOL_GUEST_AUTH_DISABLED

#define DEFAULT_EAP_STATE               (DEFAULT_EAPOL_STATE | DEFAULT_MACHINE_AUTH_STATE | DEFAULT_GUEST_AUTH_STATE)

#define IS_EAPOL_ENABLED(x) \
    ((x & EAPOL_ENABLED)?1:0)
#define IS_MACHINE_AUTH_ENABLED(x) \
    ((x & EAPOL_MACHINE_AUTH_ENABLED)?1:0)
#define IS_GUEST_AUTH_ENABLED(x) \
    ((x & EAPOL_GUEST_AUTH_ENABLED)?1:0)

#define     SUPPLICANT_MODE_0       0
#define     SUPPLICANT_MODE_1       1
#define     SUPPLICANT_MODE_2       2
#define     SUPPLICANT_MODE_3       3
#define     MAX_SUPPLICANT_MODE     SUPPLICANT_MODE_3
#define     EAPOL_DEFAULT_SUPPLICANT_MODE   SUPPLICANT_MODE_2 

#define     EAPOL_AUTH_MODE_0       0
#define     EAPOL_AUTH_MODE_1       1
#define     EAPOL_AUTH_MODE_2       2
#define     MAX_EAPOL_AUTH_MODE     EAPOL_AUTH_MODE_2
#define     EAPOL_DEFAULT_AUTH_MODE   EAPOL_AUTH_MODE_1

#define     GUID_STRING_LEN_WITH_TERM   39

#define DtlGetFirstNode(pdtllist)   ((pdtllist)->pdtlnodeFirst)
#define DtlGetNextNode(pdtlnode)    ((pdtlnode)->pdtlnodeNext)
#define DtlGetData(pdtlnode)        ((pdtlnode)->pData)

#define RAS_EAP_VALUENAME_HIDEPEAPMSCHAPv2 TEXT("HidePEAPMSCHAPv2")

#define EAPCFG_FLAG_RequireUsername   0x1
#define EAPCFG_FLAG_RequirePassword   0x2

#define EAPOL_MUTUAL_AUTH_EAP_ONLY    0x00000001

#define MAX_SSID_LEN    32

#define     EAPOL_VERSION_1           1
#define     EAPOL_VERSION_2           2
#define     EAPOL_VERSION_3           3

#define     EAPOL_CURRENT_VERSION     EAPOL_VERSION_3

#define NUM_RESP_BLOBS 3

#define     EAPOL_CERT_TYPE_SMARTCARD   1
#define     EAPOL_CERT_TYPE_MC_CERT     2

#define     EAPOLUI_GET_USERIDENTITY            0x00000001
#define     EAPOLUI_GET_USERNAMEPASSWORD        0x00000002
#define     EAPOLUI_INVOKEINTERACTIVEUI         0x00000004
#define     EAPOLUI_EAP_NOTIFICATION            0x00000008
#define     EAPOLUI_REAUTHENTICATE              0x00000010
#define     EAPOLUI_CREATEBALLOON               0x00000020
#define     EAPOLUI_CLEANUP                     0x00000040
#define     EAPOLUI_DUMMY                       0x00000080

#define     NUM_EAPOL_DLG_MSGS      8

#define WZC_PORTFLAGS_UP                 0x00000001
#define WZC_PORTFLAGS_DOWN               0x00000002
#define WZC_PORTFLAGS_SUCCESSFUL_ROAM    0x00000004
#define WZC_PORTFLAGS_PEERMAC_AVAILABLE  0x00000008

/* Enumerations */

typedef struct
{
    DWORD   dwDataLen;
#if defined(MIDL_PASS)
    [unique, size_is(dwDataLen)] LPBYTE pData;
#else
    LPBYTE  pData;
#endif
} RAW_DATA, *PRAW_DATA;

#ifndef MIDL_PASS

typedef struct
{
    ULONG                               Length;
    DWORD                               dwCtlFlags;
    NDIS_802_11_MAC_ADDRESS             MacAddress;
    UCHAR                               Reserved[2];
    NDIS_802_11_SSID                    Ssid;
    ULONG                               Privacy;
    NDIS_802_11_RSSI                    Rssi;
    NDIS_802_11_NETWORK_TYPE            NetworkTypeInUse;
    NDIS_802_11_CONFIGURATION           Configuration;
    NDIS_802_11_NETWORK_INFRASTRUCTURE  InfrastructureMode;
    NDIS_802_11_RATES                   SupportedRates;
    ULONG                               KeyIndex;
    ULONG                               KeyLength;
    UCHAR                               KeyMaterial[WZCCTL_MAX_WEPK_MATERIAL];
    NDIS_802_11_AUTHENTICATION_MODE     AuthenticationMode;
    RAW_DATA                            rdUserData;
    RAW_DATA                            rdNetworkData;
    ULONG                               WPAMCastCipher;
    ULONG                               ulMediaType;

} WZC_WLAN_CONFIG, *PWZC_WLAN_CONFIG;

typedef struct
{
    ULONG           NumberOfItems;
    ULONG           Index;
    WZC_WLAN_CONFIG Config[1];
} WZC_802_11_CONFIG_LIST, *PWZC_802_11_CONFIG_LIST;

typedef struct _WZCDLG_DATA
{
    DWORD       dwCode;
    DWORD       lParam;
} WZCDLG_DATA, *PWZCDLG_DATA;

#endif

typedef struct
{
#ifdef MIDL_PASS
    [unique, string] LPWSTR wszGuid;
#else
    LPWSTR wszGuid;
#endif
} INTF_KEY_ENTRY, *PINTF_KEY_ENTRY;

typedef struct
{
    DWORD dwNumIntfs;
#ifdef MIDL_PASS
    [size_is(dwNumIntfs)] PINTF_KEY_ENTRY pIntfs;
#else
    PINTF_KEY_ENTRY pIntfs;
#endif
} INTFS_KEY_TABLE, *PINTFS_KEY_TABLE;

typedef struct {
#ifdef MIDL_PASS
    [string] LPWSTR wszGuid;
#else
    LPWSTR          wszGuid;
#endif
#ifdef MIDL_PASS
    [string] LPWSTR wszDescr;
#else
    LPWSTR          wszDescr;
#endif
    ULONG           ulMediaState;
    ULONG           ulMediaType;
    ULONG           ulPhysicalMediaType;
    INT             nInfraMode;
    INT             nAuthMode;
    INT             nWepStatus;
    DWORD           dwCtlFlags;
    DWORD           dwCapabilities;
    RAW_DATA        rdSSID;
    RAW_DATA        rdBSSID;
    RAW_DATA        rdBSSIDList;
    RAW_DATA        rdStSSIDList;
    RAW_DATA        rdCtrlData;
    DWORD           nWPAMCastCipher;

} INTF_ENTRY, *PINTF_ENTRY;

typedef struct _wzc_context_t 
{
  DWORD dwFlags;
  DWORD tmTr;
  DWORD tmTc;
  DWORD tmTp;
  DWORD tmTf;
  DWORD tmTd;
} WZC_CONTEXT, *PWZC_CONTEXT;

typedef struct _DTLNODE
{
    struct _DTLNODE* pdtlnodePrev;
    struct _DTLNODE* pdtlnodeNext;
    VOID*    pData;
    LONG_PTR lNodeId;
} DTLNODE;

typedef struct _DTLLIST 
{
    struct _DTLNODE* pdtlnodeFirst;
    struct _DTLNODE* pdtlnodeLast;
    LONG     lNodes;
    LONG_PTR lListId;
} DTLLIST;

typedef VOID (*PDESTROYNODE)( IN DTLNODE* );

typedef enum _EAPTLS_CONNPROP_ATTRIBUTE_TYPE_
{ 
 
    ecatMinimum = 0,
    ecatFlagRegistryCert,
    ecatFlagScard,
    ecatFlagValidateServer,
    ecatFlagValidateName,
    ecatFlagDiffUser,
    ecatServerNames,
    ecatRootHashes

} EAPTLS_CONNPROP_ATTRIBUTE_TYPE;

typedef struct _EAPTLS_CONNPROP_ATTRIBUTE
{
    EAPTLS_CONNPROP_ATTRIBUTE_TYPE  ecaType;
    DWORD                           dwLength;
    PVOID                           Value;

} EAPTLS_CONNPROP_ATTRIBUTE, *PEAPTLS_CONNPROP_ATTRIBUTE;

typedef DWORD (APIENTRY * RASEAPFREE)(PBYTE);
typedef DWORD (APIENTRY * RASEAPINVOKECONFIGUI)(DWORD, HWND, DWORD, PBYTE, DWORD, PBYTE*, DWORD*);
typedef DWORD (APIENTRY * RASEAPGETIDENTITY)(DWORD, HWND, DWORD, const WCHAR*, const WCHAR*, PBYTE, DWORD, PBYTE, DWORD, PBYTE*, DWORD*, WCHAR**);
typedef DWORD (APIENTRY * RASEAPINVOKEINTERACTIVEUI)(DWORD, HWND, PBYTE, DWORD, PBYTE*, DWORD*);
typedef DWORD (APIENTRY * RASEAPCREATECONNPROP)(PEAPTLS_CONNPROP_ATTRIBUTE, PVOID*, DWORD*, PVOID*, DWORD*);

typedef struct _EAPCFG
{
    DWORD dwKey;
    TCHAR* pszFriendlyName;
    TCHAR* pszConfigDll;
    TCHAR* pszIdentityDll;
    DWORD dwStdCredentialFlags;
    BOOL fForceConfig;
    BOOL fProvidesMppeKeys;
    BYTE* pData;
    DWORD cbData;
    BYTE* pUserData;
    DWORD cbUserData;
    BOOL fConfigDllCalled;
    GUID guidConfigCLSID;
} EAPCFG;

typedef struct _EAPOL_INTF_PARAMS
{
    DWORD   dwVersion;
    DWORD   dwReserved2;
    DWORD   dwEapFlags;
    DWORD   dwEapType;
    DWORD   dwSizeOfSSID;
    BYTE    bSSID[MAX_SSID_LEN];
} EAPOL_INTF_PARAMS, *PEAPOL_INTF_PARAMS;

typedef enum _EAPOL_STATE
{
    EAPOLSTATE_LOGOFF = 0,
    EAPOLSTATE_DISCONNECTED,
    EAPOLSTATE_CONNECTING,
    EAPOLSTATE_ACQUIRED,
    EAPOLSTATE_AUTHENTICATING,
    EAPOLSTATE_HELD,
    EAPOLSTATE_AUTHENTICATED,
    EAPOLSTATE_UNDEFINED
} EAPOL_STATE;

typedef enum _EAPUISTATE
{
    EAPUISTATE_WAITING_FOR_IDENTITY = 1,
    EAPUISTATE_WAITING_FOR_UI_RESPONSE
} EAPUISTATE;

typedef struct _EAPOL_INTF_STATE
{
#ifdef MIDL_PASS
    [unique, string]    LPWSTR    pwszLocalMACAddr;
#else
    LPWSTR      pwszLocalMACAddr;
#endif
#ifdef MIDL_PASS
    [unique, string]    LPWSTR    pwszRemoteMACAddr;
#else
    LPWSTR      pwszRemoteMACAddr;
#endif
    DWORD   dwSizeOfSSID;
    BYTE    bSSID[MAX_SSID_LEN+1];
#ifdef MIDL_PASS
    [unique, string]    LPSTR    pszEapIdentity;
#else
    LPSTR       pszEapIdentity;
#endif
    EAPOL_STATE     dwState;
    EAPUISTATE      dwEapUIState;
    DWORD   dwEAPOLAuthMode;
    DWORD   dwEAPOLAuthenticationType;
    DWORD   dwEapType;
    DWORD   dwFailCount;
    DWORD   dwPhysicalMediumType;
} EAPOL_INTF_STATE, *PEAPOL_INTF_STATE;

typedef struct _EAPOL_AUTH_DATA
{
    DWORD   dwEapType;
    DWORD   dwSize;
    BYTE    bData[1];
} EAPOL_AUTH_DATA, *PEAPOL_AUTH_DATA;

typedef struct _EAPOLUI_RESP
{
    RAW_DATA    rdData0;
    RAW_DATA    rdData1;
    RAW_DATA    rdData2;
} EAPOLUI_RESP, *PEAPOLUI_RESP;

typedef struct _EAPOL_POLICY_DATA {
    BYTE  pbWirelessSSID[32];
    DWORD dwWirelessSSIDLen;
    DWORD dwEnable8021x;
    DWORD dw8021xMode;
    DWORD dwEAPType;
    DWORD dwMachineAuthentication;
    DWORD dwMachineAuthenticationType;
    DWORD dwGuestAuthentication;
    DWORD dwIEEE8021xMaxStart;
    DWORD dwIEEE8021xStartPeriod;
    DWORD dwIEEE8021xAuthPeriod;
    DWORD dwIEEE8021xHeldPeriod;
    DWORD dwEAPDataLen;
    LPBYTE pbEAPData;
} EAPOL_POLICY_DATA, *PEAPOL_POLICY_DATA;

typedef struct _EAPOL_POLICY_LIST {
	DWORD			    dwNumberOfItems;
	EAPOL_POLICY_DATA	EAPOLPolicy[1];
} EAPOL_POLICY_LIST, *PEAPOL_POLICY_LIST;

typedef struct _EAPOL_EAP_UI_CONTEXT
{
    DWORD       dwEAPOLUIMsgType;
    WCHAR       wszGUID[39];
    DWORD       dwSessionId;
    DWORD       dwContextId;
    DWORD       dwEapId;
    DWORD       dwEapTypeId;
    DWORD       dwEapFlags;
    WCHAR       wszSSID[MAX_SSID_LEN+1];
    DWORD       dwSizeOfSSID;
    BYTE        bSSID[MAX_SSID_LEN];
    DWORD       dwEAPOLState;
    DWORD       dwRetCode;
    DWORD       dwSizeOfEapUIData;
    BYTE        bEapUIData[1];
} EAPOL_EAP_UI_CONTEXT, *PEAPOL_EAP_UI_CONTEXT;

typedef struct _WZC_PORT_INFO {
    GUID gAdatperId;
    RAW_DATA PeerMacAddress;
    DWORD dwFlags;
} WZC_PORT_INFO, *PWZC_PORT_INFO;

/* Functions */

VOID
WINAPI
WZCDeleteIntfObj(
    IN PINTF_ENTRY pIntf);

VOID
WINAPI
WZCPassword2Key(
    PWZC_WLAN_CONFIG pwzcConfig,
    LPCSTR cszPassword);

DWORD
WINAPI
WZCEnumInterfaces(
    IN LPWSTR            pSrvAddr,
    OUT PINTFS_KEY_TABLE pIntfs);
 
DWORD
WINAPI
WZCQueryInterface(
    IN LPWSTR              pSrvAddr,
    IN DWORD               dwInFlags,
    IN OUT PINTF_ENTRY     pIntf,
    OUT LPDWORD            pdwOutFlags);

DWORD
WINAPI
WZCSetInterface(
    IN LPWSTR              pSrvAddr,
    IN DWORD               dwInFlags,
    IN PINTF_ENTRY         pIntf,
    OUT LPDWORD            pdwOutFlags);

DWORD
WINAPI
WZCRefreshInterface(
    IN LPWSTR              pSrvAddr,
    IN DWORD               dwInFlags,
    IN PINTF_ENTRY         pIntf,
    OUT LPDWORD            pdwOutFlags);

DWORD
WINAPI
WZCQueryContext(
    IN LPWSTR              pSrvAddr,
    IN DWORD               dwInFlags,
    IN PWZC_CONTEXT        pContext,
    OUT LPDWORD            pdwOutFlags);

DWORD
WINAPI
WZCSetContext(
    IN LPWSTR              pSrvAddr,
    IN DWORD               dwInFlags,
    IN PWZC_CONTEXT        pContext,
    OUT LPDWORD            pdwOutFlags);

VOID WINAPI DtlDestroyList(DTLLIST*, PDESTROYNODE);

DTLNODE*
WINAPI
CreateEapcfgNode(
    void);

VOID
WINAPI
DestroyEapcfgNode(
    IN OUT DTLNODE* pNode);

DTLNODE*
WINAPI
EapcfgNodeFromKey(
    IN DTLLIST* pList,
    IN DWORD dwKey);

DTLLIST*
WINAPI
ReadEapcfgList(IN DWORD dwFlags);

DWORD
WINAPI
WZCGetEapUserInfo(
    IN WCHAR           *pwszGUID,
    IN DWORD           dwEapTypeId,
    IN DWORD           dwSizOfSSID,
    IN BYTE            *pbSSID,
    IN OUT PBYTE       pbUserInfo,
    IN OUT DWORD       *pdwInfoSize);

#ifndef MIDL_PASS

DWORD
WINAPI
WZCEapolGetCustomAuthData( 
    IN LPWSTR        pSrvAddr,
    IN PWCHAR        pwszGuid,
    IN DWORD         dwEapTypeId,
    IN DWORD         dwSizeOfSSID,
    IN BYTE          *pbSSID,
    IN OUT PBYTE      pbConnInfo,
    IN OUT PDWORD     pdwInfoSize);

DWORD
WINAPI
WZCEapolSetCustomAuthData(
    IN LPWSTR        pSrvAddr,
    IN PWCHAR        pwszGuid,
    IN DWORD         dwEapTypeId,
    IN DWORD         dwSizeOfSSID,
    IN BYTE          *pbSSID,
    IN PBYTE         pbConnInfo,
    IN DWORD         dwInfoSize);

DWORD
WINAPI
WZCEapolGetInterfaceParams(
    IN LPWSTR                pSrvAddr,
    IN PWCHAR                pwszGuid,
    IN DWORD                 dwSizeOfSSID,
    IN BYTE                  *pbSSID,
    IN OUT EAPOL_INTF_PARAMS *pIntfParams);

DWORD
WINAPI
WZCEapolSetInterfaceParams(
    IN LPWSTR            pSrvAddr,
    IN PWCHAR            pwszGuid,
    IN DWORD             dwSizeOfSSID,
    IN BYTE              *pbSSID,
    IN EAPOL_INTF_PARAMS *pIntfParams);

DWORD
WINAPI
WZCEapolReAuthenticate(
    IN LPWSTR        pSrvAddr,
    IN PWCHAR        pwszGuid);

DWORD
WINAPI
WZCEapolQueryState( 
    IN LPWSTR               pSrvAddr,
    IN PWCHAR               pwszGuid,
    IN OUT EAPOL_INTF_STATE *pIntfState);

#endif // MIDL_PASS

DWORD
WINAPI
WZCEapolFreeState(
    IN EAPOL_INTF_STATE    *pIntfState);

DWORD
WINAPI
WZCEapolUIResponse(
    IN LPWSTR                  pSrvAddr,
    IN EAPOL_EAP_UI_CONTEXT    EapolUIContext,
    IN EAPOLUI_RESP            EapolUIResp);

# ifdef __cplusplus
}
# endif

#endif // _WZCSAPI_H
