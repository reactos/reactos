#ifndef _RASEAPIF_
#define _RASEAPIF_

#ifdef __cplusplus
extern "C" {
#endif

#if(WINVER >= 0x0500)

#define RAS_EAP_REGISTRY_LOCATION TEXT("SYSTEM\\CurrentControlSet\\Services\\Rasman\\PPP\\EAP")

#define RAS_EAP_VALUENAME_PATH                  TEXT("Path")
#define RAS_EAP_VALUENAME_CONFIGUI              TEXT("ConfigUIPath")
#define RAS_EAP_VALUENAME_INTERACTIVEUI         TEXT("InteractiveUIPath")
#define RAS_EAP_VALUENAME_IDENTITY              TEXT("IdentityPath")
#define RAS_EAP_VALUENAME_FRIENDLY_NAME         TEXT("FriendlyName")
#define RAS_EAP_VALUENAME_DEFAULT_DATA          TEXT("ConfigData")
#define RAS_EAP_VALUENAME_REQUIRE_CONFIGUI      TEXT("RequireConfigUI")
#define RAS_EAP_VALUENAME_ENCRYPTION            TEXT("MPPEEncryptionSupported")
#define RAS_EAP_VALUENAME_INVOKE_NAMEDLG        TEXT("InvokeUsernameDialog")
#define RAS_EAP_VALUENAME_INVOKE_PWDDLG         TEXT("InvokePasswordDialog")
#define RAS_EAP_VALUENAME_CONFIG_CLSID          TEXT("ConfigCLSID")
#define RAS_EAP_VALUENAME_STANDALONE_SUPPORTED  TEXT("StandaloneSupported")
#define RAS_EAP_VALUENAME_ROLES_SUPPORTED       TEXT("RolesSupported")
#define RAS_EAP_VALUENAME_PER_POLICY_CONFIG     TEXT("PerPolicyConfig")

#define RAS_EAP_ROLE_AUTHENTICATOR              0x00000001
#define RAS_EAP_ROLE_AUTHENTICATEE              0x00000002

#define RAS_EAP_ROLE_EXCLUDE_IN_EAP             0x00000004
#define RAS_EAP_ROLE_EXCLUDE_IN_PEAP            0x00000008
#define RAS_EAP_ROLE_EXCLUDE_IN_VPN             0x00000010

#define  raatARAPChallenge                      33
#define  raatARAPOldPassword                    19
#define  raatARAPNewPassword                    20
#define  raatARAPPasswordChangeReason           21

#define EAPCODE_Request                         1
#define EAPCODE_Response                        2
#define EAPCODE_Success                         3
#define EAPCODE_Failure                         4

#define MAXEAPCODE                              4

#define RAS_EAP_FLAG_ROUTER                     0x00000001
#define RAS_EAP_FLAG_NON_INTERACTIVE            0x00000002
#define RAS_EAP_FLAG_LOGON                      0x00000004
#define RAS_EAP_FLAG_PREVIEW                    0x00000008
#define RAS_EAP_FLAG_FIRST_LINK                 0x00000010
#define RAS_EAP_FLAG_MACHINE_AUTH               0x00000020
#define RAS_EAP_FLAG_GUEST_ACCESS               0x00000040
#define RAS_EAP_FLAG_8021X_AUTH                 0x00000080
#define RAS_EAP_FLAG_HOSTED_IN_PEAP             0x00000100
#define RAS_EAP_FLAG_RESUME_FROM_HIBERNATE      0x00000200
#define RAS_EAP_FLAG_PEAP_UPFRONT               0x00000400
#define RAS_EAP_FLAG_ALTERNATIVE_USER_DB        0x00000800

typedef enum _RAS_AUTH_ATTRIBUTE_TYPE_
{
    raatMinimum = 0,
    raatUserName,
    raatUserPassword,
    raatMD5CHAPPassword,
    raatNASIPAddress,
    raatNASPort,
    raatServiceType,
    raatFramedProtocol,
    raatFramedIPAddress,
    raatFramedIPNetmask,
    raatFramedRouting = 10,
    raatFilterId,
    raatFramedMTU,
    raatFramedCompression,
    raatLoginIPHost,
    raatLoginService,
    raatLoginTCPPort,
    raatUnassigned17,
    raatReplyMessage,
    raatCallbackNumber,
    raatCallbackId =20,
    raatUnassigned21,
    raatFramedRoute,
    raatFramedIPXNetwork,
    raatState,
    raatClass,
    raatVendorSpecific,
    raatSessionTimeout,
    raatIdleTimeout,
    raatTerminationAction,
    raatCalledStationId = 30,
    raatCallingStationId,
    raatNASIdentifier,
    raatProxyState,
    raatLoginLATService,
    raatLoginLATNode,
    raatLoginLATGroup,
    raatFramedAppleTalkLink,
    raatFramedAppleTalkNetwork,
    raatFramedAppleTalkZone,
    raatAcctStatusType = 40,
    raatAcctDelayTime,
    raatAcctInputOctets,
    raatAcctOutputOctets,
    raatAcctSessionId,
    raatAcctAuthentic,
    raatAcctSessionTime,
    raatAcctInputPackets,
    raatAcctOutputPackets,
    raatAcctTerminateCause,
    raatAcctMultiSessionId = 50,
    raatAcctLinkCount,
    raatAcctEventTimeStamp = 55,
    raatMD5CHAPChallenge = 60,
    raatNASPortType,
    raatPortLimit,
    raatLoginLATPort,
    raatTunnelType,
    raatTunnelMediumType,
    raatTunnelClientEndpoint,
    raatTunnelServerEndpoint,
    raatARAPPassword = 70,
    raatARAPFeatures,
    raatARAPZoneAccess,
    raatARAPSecurity,
    raatARAPSecurityData,
    raatPasswordRetry,
    raatPrompt,
    raatConnectInfo,
    raatConfigurationToken,
    raatEAPMessage,
    raatSignature = 80,
    raatARAPChallengeResponse = 84,
    raatAcctInterimInterval = 85,
    raatARAPGuestLogon = 8096,
    raatCertificateOID,
    raatEAPConfiguration,
    raatPEAPEmbeddedEAPTypeId,
    raatPEAPFastRoamedSession,
    raatEAPTLV = 8102,
    raatReserved = 0xFFFFFFFF

}RAS_AUTH_ATTRIBUTE_TYPE;

typedef struct _RAS_AUTH_ATTRIBUTE
{
    RAS_AUTH_ATTRIBUTE_TYPE raaType;
    DWORD dwLength;
    PVOID Value;

}RAS_AUTH_ATTRIBUTE, *PRAS_AUTH_ATTRIBUTE;

typedef struct _PPP_EAP_PACKET
{
    BYTE Code;
    BYTE Id;
    BYTE Length[2];
    BYTE Data[1];
}PPP_EAP_PACKET, *PPPP_EAP_PACKET;

#define PPP_EAP_PACKET_HDR_LEN (sizeof(PPP_EAP_PACKET) - 1)

typedef struct _PPP_EAP_INPUT
{
    DWORD dwSizeInBytes;
    DWORD fFlags;
    BOOL fAuthenticator;
    WCHAR* pwszIdentity;
    WCHAR* pwszPassword;
    BYTE bInitialId;
    RAS_AUTH_ATTRIBUTE* pUserAttributes;
    BOOL fAuthenticationComplete;
    DWORD dwAuthResultCode;
    OPTIONAL HANDLE hTokenImpersonateUser;
    BOOL fSuccessPacketReceived;
    BOOL fDataReceivedFromInteractiveUI;
    OPTIONAL PBYTE pDataFromInteractiveUI;
    DWORD dwSizeOfDataFromInteractiveUI;
    OPTIONAL PBYTE pConnectionData;
    DWORD dwSizeOfConnectionData;
    OPTIONAL PBYTE pUserData;
    DWORD dwSizeOfUserData;
    HANDLE hReserved;
}PPP_EAP_INPUT, *PPPP_EAP_INPUT;

typedef enum _PPP_EAP_ACTION
{
    EAPACTION_NoAction,
    EAPACTION_Authenticate,
    EAPACTION_Done,
    EAPACTION_SendAndDone,
    EAPACTION_Send,
    EAPACTION_SendWithTimeout,
    EAPACTION_SendWithTimeoutInteractive,
	EAPACTION_IndicateTLV,
	EAPACTION_IndicateIdentity
}PPP_EAP_ACTION;

typedef struct _PPP_EAP_OUTPUT
{
    DWORD dwSizeInBytes;
    PPP_EAP_ACTION Action;
    DWORD dwAuthResultCode;
    OPTIONAL RAS_AUTH_ATTRIBUTE* pUserAttributes;
    BOOL fInvokeInteractiveUI;
    OPTIONAL PBYTE pUIContextData;
    DWORD dwSizeOfUIContextData;
    BOOL fSaveConnectionData;
    OPTIONAL PBYTE pConnectionData;
    DWORD dwSizeOfConnectionData;
    BOOL fSaveUserData;
    OPTIONAL PBYTE pUserData;
    DWORD dwSizeOfUserData;
}PPP_EAP_OUTPUT, *PPPP_EAP_OUTPUT;

typedef struct _PPP_EAP_INFO
{
    DWORD dwSizeInBytes;
    DWORD dwEapTypeId;
    DWORD (APIENTRY *RasEapInitialize)(IN BOOL fInitialize );
    DWORD (APIENTRY *RasEapBegin)(OUT VOID** ppWorkBuffer, IN PPP_EAP_INPUT* pPppEapInput);
    DWORD (APIENTRY *RasEapEnd)(IN VOID* pWorkBuffer);
    DWORD (APIENTRY *RasEapMakeMessage)(IN VOID* pWorkBuf, IN PPP_EAP_PACKET* pReceivePacket, OUT PPP_EAP_PACKET* pSendPacket, IN DWORD cbSendPacket, OUT PPP_EAP_OUTPUT* pEapOutput, IN PPP_EAP_INPUT* pEapInput);
}PPP_EAP_INFO, *PPPP_EAP_INFO;

DWORD APIENTRY
RasEapGetInfo(IN DWORD dwEapTypeId,
              OUT PPP_EAP_INFO* pEapInfo);

DWORD APIENTRY
RasEapFreeMemory(IN BYTE* pMemory);

DWORD APIENTRY
RasEapInvokeInteractiveUI(IN DWORD dwEapTypeId,
                          IN HWND hwndParent,
                          IN BYTE* pUIContextData,
                          IN DWORD dwSizeOfUIContextData,
                          OUT BYTE** ppDataFromInteractiveUI,
                          OUT DWORD* pdwSizeOfDataFromInteractiveUI);

DWORD APIENTRY
RasEapInvokeConfigUI(IN DWORD dwEapTypeId,
                     IN HWND hwndParent,
                     IN DWORD dwFlags,
                     IN BYTE* pConnectionDataIn,
                     IN DWORD dwSizeOfConnectionDataIn,
                     OUT BYTE** ppConnectionDataOut,
                     OUT DWORD* pdwSizeOfConnectionDataOut);

DWORD APIENTRY
RasEapGetIdentity(IN DWORD dwEapTypeId,
                  IN HWND hwndParent,
                  IN DWORD dwFlags,
                  IN const WCHAR* pwszPhonebook,
                  IN const WCHAR* pwszEntry,
                  IN BYTE* pConnectionDataIn,
                  IN DWORD dwSizeOfConnectionDataIn,
                  IN BYTE* pUserDataIn,
                  IN DWORD dwSizeOfUserDataIn,
                  OUT BYTE** ppUserDataOut,
                  OUT DWORD* pdwSizeOfUserDataOut,
                  OUT WCHAR** ppwszIdentity
);

#endif /* WINVER >= 0x0500 */

#ifdef __cplusplus
}
#endif

#endif /* _RASEAPIF_ */

