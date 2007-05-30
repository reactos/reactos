#ifndef _RAS_H
#define _RAS_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef _LMCONS_H
#include <lmcons.h>
#endif

/* TODO
include <basetsd.h> from winnt.h so that this typedef is not necessary
*/
#ifndef _BASETSD_H
typedef unsigned long ULONG_PTR, *PULONG_PTR;
#endif

#include <pshpack4.h>

#define RAS_MaxDeviceType     16
#define RAS_MaxPhoneNumber    128
#define RAS_MaxIpAddress      15
#define RAS_MaxIpxAddress     21
#define RAS_MaxEntryName      256
#define RAS_MaxDeviceName     128
#define RAS_MaxCallbackNumber RAS_MaxPhoneNumber
#define RAS_MaxAreaCode       10
#define RAS_MaxPadType        32
#define RAS_MaxX25Address     200
#define RAS_MaxFacilities     200
#define RAS_MaxUserData       200
#define RAS_MaxReplyMessage   1024
#define RDEOPT_UsePrefixSuffix           0x00000001
#define RDEOPT_PausedStates              0x00000002
#define RDEOPT_IgnoreModemSpeaker        0x00000004
#define RDEOPT_SetModemSpeaker           0x00000008
#define RDEOPT_IgnoreSoftwareCompression 0x00000010
#define RDEOPT_SetSoftwareCompression    0x00000020
#define RDEOPT_DisableConnectedUI        0x00000040
#define RDEOPT_DisableReconnectUI        0x00000080
#define RDEOPT_DisableReconnect          0x00000100
#define RDEOPT_NoUser                    0x00000200
#define RDEOPT_PauseOnScript             0x00000400
#define RDEOPT_Router                    0x00000800
#define REN_User                         0x00000000
#define REN_AllUsers                     0x00000001
#define VS_Default		                0
#define VS_PptpOnly	                    1
#define VS_PptpFirst	                2
#define VS_L2tpOnly 	                3
#define VS_L2tpFirst	                4
#define RASDIALEVENT                    "RasDialEvent"
#define WM_RASDIALEVENT                 0xCCCD
#define RASEO_UseCountryAndAreaCodes    0x00000001
#define RASEO_SpecificIpAddr            0x00000002
#define RASEO_SpecificNameServers       0x00000004
#define RASEO_IpHeaderCompression       0x00000008
#define RASEO_RemoteDefaultGateway      0x00000010
#define RASEO_DisableLcpExtensions      0x00000020
#define RASEO_TerminalBeforeDial        0x00000040
#define RASEO_TerminalAfterDial         0x00000080
#define RASEO_ModemLights               0x00000100
#define RASEO_SwCompression             0x00000200
#define RASEO_RequireEncryptedPw        0x00000400
#define RASEO_RequireMsEncryptedPw      0x00000800
#define RASEO_RequireDataEncryption     0x00001000
#define RASEO_NetworkLogon              0x00002000
#define RASEO_UseLogonCredentials       0x00004000
#define RASEO_PromoteAlternates         0x00008000
#define RASNP_NetBEUI                   0x00000001
#define RASNP_Ipx                       0x00000002
#define RASNP_Ip                        0x00000004
#define RASFP_Ppp                       0x00000001
#define RASFP_Slip                      0x00000002
#define RASFP_Ras                       0x00000004
#define RASDT_Modem                     TEXT("modem")
#define RASDT_Isdn                      TEXT("isdn")
#define RASDT_X25                       TEXT("x25")
#define RASDT_Vpn                       TEXT("vpn")
#define RASDT_Pad                       TEXT("pad")
#define RASDT_Generic                   TEXT("GENERIC")
#define RASDT_Serial        		TEXT("SERIAL")
#define RASDT_FrameRelay                TEXT("FRAMERELAY")
#define RASDT_Atm                       TEXT("ATM")
#define RASDT_Sonet                     TEXT("SONET")
#define RASDT_SW56                      TEXT("SW56")
#define RASDT_Irda                      TEXT("IRDA")
#define RASDT_Parallel                  TEXT("PARALLEL")
#define RASET_Phone     1
#define RASET_Vpn       2
#define RASET_Direct    3
#define RASET_Internet  4
#if (WINVER >= 0x401)
#define RASEO_SecureLocalFiles  0x00010000
#define RASCN_Connection        0x00000001
#define RASCN_Disconnection     0x00000002
#define RASCN_BandwidthAdded    0x00000004
#define RASCN_BandwidthRemoved  0x00000008
#define RASEDM_DialAll          1
#define RASEDM_DialAsNeeded     2
#define RASIDS_Disabled         0xffffffff
#define RASIDS_UseGlobalValue   0
#define RASADFLG_PositionDlg    0x00000001
#define RASCM_UserName       0x00000001
#define RASCM_Password       0x00000002
#define RASCM_Domain         0x00000004
#define RASADP_DisableConnectionQuery   0
#define RASADP_LoginSessionDisable      1
#define RASADP_SavedAddressesLimit      2
#define RASADP_FailedConnectionTimeout  3
#define RASADP_ConnectionQueryTimeout   4
#endif  /* (WINVER >= 0x401) */
#if (WINVER >= 0x500)
#define RDEOPT_CustomDial   0x00001000
#define RASLCPAP_PAP        0xC023
#define RASLCPAP_SPAP       0xC027
#define RASLCPAP_CHAP       0xC223
#define RASLCPAP_EAP        0xC227
#define RASLCPAD_CHAP_MD5   0x05
#define RASLCPAD_CHAP_MS    0x80
#define RASLCPAD_CHAP_MSV2  0x81
#define RASLCPO_PFC         0x00000001
#define RASLCPO_ACFC        0x00000002
#define RASLCPO_SSHF        0x00000004
#define RASLCPO_DES_56      0x00000008
#define RASLCPO_3_DES       0x00000010
#define RASCCPCA_MPPC       0x00000006
#define RASCCPCA_STAC       0x00000005
#define RASCCPO_Compression         0x00000001
#define RASCCPO_HistoryLess         0x00000002
#define RASCCPO_Encryption56bit     0x00000010
#define RASCCPO_Encryption40bit     0x00000020
#define RASCCPO_Encryption128bit    0x00000040
#define RASEO_RequireEAP            0x00020000
#define RASEO_RequirePAP            0x00040000
#define RASEO_RequireSPAP           0x00080000
#define RASEO_Custom                0x00100000
#define RASEO_PreviewPhoneNumber    0x00200000
#define RASEO_SharedPhoneNumbers    0x00800000
#define RASEO_PreviewUserPw         0x01000000
#define RASEO_PreviewDomain         0x02000000
#define RASEO_ShowDialingProgress   0x04000000
#define RASEO_RequireCHAP           0x08000000
#define RASEO_RequireMsCHAP         0x10000000
#define RASEO_RequireMsCHAP2        0x20000000
#define RASEO_RequireW95MSCHAP      0x40000000
#define RASEO_CustomScript          0x80000000
#define RASIPO_VJ                   0x00000001
#define RCD_SingleUser              0
#define RCD_AllUsers                0x00000001
#define RCD_Eap                     0x00000002
#define RASEAPF_NonInteractive      0x00000002
#define RASEAPF_Logon               0x00000004
#define RASEAPF_Preview             0x00000008
#define ET_40Bit        1
#define ET_128Bit       2
#define ET_None         0
#define ET_Require      1
#define ET_RequireMax   2
#define ET_Optional     3
#endif /* (WINVER >= 0x500) */

#define RASCS_PAUSED 0x1000
#define RASCS_DONE   0x2000
typedef enum tagRASCONNSTATE {
      RASCS_OpenPort = 0,
      RASCS_PortOpened,
      RASCS_ConnectDevice,
      RASCS_DeviceConnected,
      RASCS_AllDevicesConnected,
      RASCS_Authenticate,
      RASCS_AuthNotify,
      RASCS_AuthRetry,
      RASCS_AuthCallback,
      RASCS_AuthChangePassword,
      RASCS_AuthProject,
      RASCS_AuthLinkSpeed,
      RASCS_AuthAck,
      RASCS_ReAuthenticate,
      RASCS_Authenticated,
      RASCS_PrepareForCallback,
      RASCS_WaitForModemReset,
      RASCS_WaitForCallback,
      RASCS_Projected,
      RASCS_StartAuthentication,
      RASCS_CallbackComplete,
      RASCS_LogonNetwork,
      RASCS_SubEntryConnected,
      RASCS_SubEntryDisconnected,
      RASCS_Interactive = RASCS_PAUSED,
      RASCS_RetryAuthentication,
      RASCS_CallbackSetByCaller,
      RASCS_PasswordExpired,
#if (WINVER >= 0x500)
      RASCS_InvokeEapUI,
#endif
      RASCS_Connected = RASCS_DONE,
      RASCS_Disconnected
}  RASCONNSTATE, *LPRASCONNSTATE;

typedef enum tagRASPROJECTION {
    RASP_Amb =      0x10000,
    RASP_PppNbf =   0x803F,
    RASP_PppIpx =   0x802B,
    RASP_PppIp =    0x8021,
#if (WINVER >= 0x500)
    RASP_PppCcp =   0x80FD,
#endif
    RASP_PppLcp =   0xC021,
    RASP_Slip =     0x20000
} RASPROJECTION, *LPRASPROJECTION;

DECLARE_HANDLE (HRASCONN);
typedef  HRASCONN* LPHRASCONN;

typedef struct tagRASCONNW {
    DWORD dwSize;
    HRASCONN hrasconn;
    WCHAR szEntryName[RAS_MaxEntryName + 1];
#if (WINVER >= 0x400)
    WCHAR szDeviceType[RAS_MaxDeviceType + 1];
    WCHAR szDeviceName[RAS_MaxDeviceName + 1];
#endif
#if (WINVER >= 0x401)
    WCHAR szPhonebook[MAX_PATH];
    DWORD dwSubEntry;
#endif
#if (WINVER >= 0x500)
    GUID guidEntry;
#endif
#if (WINVER >= 0x501)
    DWORD dwSessionId;
    DWORD dwFlags;
    LUID luid;
#endif
} RASCONNW, *LPRASCONNW;

typedef struct tagRASCONNA {
    DWORD dwSize;
    HRASCONN hrasconn;
    CHAR szEntryName[RAS_MaxEntryName + 1];
#if (WINVER >= 0x400)
    CHAR szDeviceType[RAS_MaxDeviceType + 1];
    CHAR szDeviceName[RAS_MaxDeviceName + 1];
#endif
#if (WINVER >= 0x401)
    CHAR szPhonebook[MAX_PATH];
    DWORD dwSubEntry;
#endif
#if (WINVER >= 0x500)
    GUID guidEntry;
#endif
#if (WINVER >= 0x501)
    DWORD dwSessionId;
    DWORD dwFlags;
    LUID luid;
#endif
} RASCONNA, *LPRASCONNA;

typedef struct tagRASCONNSTATUSW {
    DWORD dwSize;
    RASCONNSTATE rasconnstate;
    DWORD dwError;
    WCHAR szDeviceType[RAS_MaxDeviceType + 1];
    WCHAR szDeviceName[RAS_MaxDeviceName + 1];
#if (WINVER >= 0x401)
    WCHAR szPhoneNumber[RAS_MaxPhoneNumber + 1];
#endif
} RASCONNSTATUSW, *LPRASCONNSTATUSW;

typedef struct tagRASCONNSTATUSA {
    DWORD dwSize;
    RASCONNSTATE rasconnstate;
    DWORD dwError;
    CHAR szDeviceType[RAS_MaxDeviceType + 1];
    CHAR szDeviceName[RAS_MaxDeviceName + 1];
#if (WINVER >= 0x401)
    CHAR szPhoneNumber[RAS_MaxPhoneNumber + 1];
#endif
} RASCONNSTATUSA, *LPRASCONNSTATUSA;

typedef struct tagRASDIALPARAMSW {
    DWORD dwSize;
    WCHAR szEntryName[RAS_MaxEntryName + 1];
    WCHAR szPhoneNumber[RAS_MaxPhoneNumber + 1];
    WCHAR szCallbackNumber[RAS_MaxCallbackNumber + 1];
    WCHAR szUserName[UNLEN + 1];
    WCHAR szPassword[PWLEN + 1];
    WCHAR szDomain[DNLEN + 1];
#if (WINVER >= 0x401)
    DWORD dwSubEntry;
    ULONG_PTR dwCallbackId;
#endif
} RASDIALPARAMSW, *LPRASDIALPARAMSW;

typedef struct tagRASDIALPARAMSA {
    DWORD dwSize;
    CHAR szEntryName[RAS_MaxEntryName + 1];
    CHAR szPhoneNumber[RAS_MaxPhoneNumber + 1];
    CHAR szCallbackNumber[RAS_MaxCallbackNumber + 1];
    CHAR szUserName[UNLEN + 1];
    CHAR szPassword[PWLEN + 1];
    CHAR szDomain[DNLEN + 1];
#if (WINVER >= 0x401)
    DWORD dwSubEntry;
    ULONG_PTR dwCallbackId;
#endif
} RASDIALPARAMSA, *LPRASDIALPARAMSA;

#if (WINVER >= 0x500)
typedef struct tagRASEAPINFO {
    DWORD dwSizeofEapInfo;
    BYTE *pbEapInfo;
} RASEAPINFO;
#endif

typedef struct tagRASDIALEXTENSIONS {
    DWORD dwSize;
    DWORD dwfOptions;
    HWND hwndParent;
    ULONG_PTR reserved;
#if (WINVER >= 0x500)
    ULONG_PTR reserved1;
    RASEAPINFO RasEapInfo;
#endif
} RASDIALEXTENSIONS, *LPRASDIALEXTENSIONS;

typedef struct tagRASENTRYNAMEW {
    DWORD dwSize;
    WCHAR szEntryName[RAS_MaxEntryName + 1];
#if (WINVER >= 0x500)
    DWORD dwFlags;
    WCHAR szPhonebookPath[MAX_PATH + 1];
#endif
} RASENTRYNAMEW, *LPRASENTRYNAMEW;

typedef struct tagRASENTRYNAMEA {
    DWORD dwSize;
    CHAR szEntryName[RAS_MaxEntryName + 1];
#if (WINVER >= 0x500)
    DWORD dwFlags;
    CHAR szPhonebookPath[MAX_PATH + 1];
#endif
} RASENTRYNAMEA, *LPRASENTRYNAMEA;

typedef struct tagRASAMBW {
    DWORD dwSize;
    DWORD dwError;
    WCHAR szNetBiosError[NETBIOS_NAME_LEN + 1];
    BYTE bLana;
} RASAMBW, *LPRASAMBW;

typedef struct tagRASAMBA {
    DWORD dwSize;
    DWORD dwError;
    CHAR szNetBiosError[NETBIOS_NAME_LEN + 1];
    BYTE bLana;
} RASAMBA, *LPRASAMBA;

typedef struct tagRASPPPNBFW {
    DWORD dwSize;
    DWORD dwError;
    DWORD dwNetBiosError;
    WCHAR szNetBiosError[NETBIOS_NAME_LEN + 1];
    WCHAR szWorkstationName[NETBIOS_NAME_LEN + 1];
    BYTE bLana;
} RASPPPNBFW, *LPRASPPPNBFW;

typedef struct tagRASPPPNBFA {
    DWORD dwSize;
    DWORD dwError;
    DWORD dwNetBiosError;
    CHAR szNetBiosError[NETBIOS_NAME_LEN + 1];
    CHAR szWorkstationName[NETBIOS_NAME_LEN + 1];
    BYTE bLana;
} RASPPPNBFA, *LPRASPPPNBFA;

typedef struct tagRASIPXW {
    DWORD dwSize;
    DWORD dwError;
    WCHAR szIpxAddress[RAS_MaxIpxAddress + 1];
} RASPPPIPXW, *LPRASPPPIPXW;

typedef struct tagRASIPXA {
    DWORD dwSize;
    DWORD dwError;
    CHAR szIpxAddress[RAS_MaxIpxAddress + 1];
} RASPPPIPXA, *LPRASPPPIPXA;

typedef struct tagRASPPPIPW {
    DWORD dwSize;
    DWORD dwError;
    WCHAR szIpAddress[RAS_MaxIpAddress + 1];
#ifndef WINNT35COMPATIBLE
    WCHAR szServerIpAddress[RAS_MaxIpAddress + 1];
#endif
#if (WINVER >= 0x500)
    DWORD dwOptions;
    DWORD dwServerOptions;
#endif
} RASPPPIPW, *LPRASPPPIPW;

typedef struct tagRASPPPIPA {
    DWORD dwSize;
    DWORD dwError;
    CHAR szIpAddress[RAS_MaxIpAddress + 1];
#ifndef WINNT35COMPATIBLE
    CHAR szServerIpAddress[RAS_MaxIpAddress + 1];
#endif
#if (WINVER >= 0x500)
    DWORD dwOptions;
    DWORD dwServerOptions;
#endif
} RASPPPIPA, *LPRASPPPIPA;

typedef struct tagRASPPPLCPW {
    DWORD dwSize;
    BOOL fBundled;
#if (WINVER >= 0x500)
    DWORD dwError;
    DWORD dwAuthenticationProtocol;
    DWORD dwAuthenticationData;
    DWORD dwEapTypeId;
    DWORD dwServerAuthenticationProtocol;
    DWORD dwServerAuthenticationData;
    DWORD dwServerEapTypeId;
    BOOL fMultilink;
    DWORD dwTerminateReason;
    DWORD dwServerTerminateReason;
    WCHAR szReplyMessage[RAS_MaxReplyMessage];
    DWORD dwOptions;
    DWORD dwServerOptions;
#endif
} RASPPPLCPW, *LPRASPPPLCPW;

typedef struct tagRASPPPLCPA {
    DWORD dwSize;
    BOOL fBundled;
#if (WINVER >= 0x500)
    DWORD dwError;
    DWORD dwAuthenticationProtocol;
    DWORD dwAuthenticationData;
    DWORD dwEapTypeId;
    DWORD dwServerAuthenticationProtocol;
    DWORD dwServerAuthenticationData;
    DWORD dwServerEapTypeId;
    BOOL fMultilink;
    DWORD dwTerminateReason;
    DWORD dwServerTerminateReason;
    CHAR szReplyMessage[RAS_MaxReplyMessage];
    DWORD dwOptions;
    DWORD dwServerOptions;
#endif
} RASPPPLCPA, *LPRASPPPLCPA;

typedef struct tagRASSLIPW {
    DWORD dwSize;
    DWORD dwError;
    WCHAR szIpAddress[RAS_MaxIpAddress + 1];
} RASSLIPW, *LPRASSLIPW;


typedef struct tagRASSLIPA {
    DWORD dwSize;
    DWORD dwError;
    CHAR szIpAddress[RAS_MaxIpAddress + 1];
} RASSLIPA, *LPRASSLIPA;

typedef struct tagRASDEVINFOW {
    DWORD dwSize;
    WCHAR szDeviceType[RAS_MaxDeviceType + 1];
    WCHAR szDeviceName[RAS_MaxDeviceName + 1];
} RASDEVINFOW, *LPRASDEVINFOW;

typedef struct tagRASDEVINFOA {
    DWORD dwSize;
    CHAR szDeviceType[RAS_MaxDeviceType + 1];
    CHAR szDeviceName[RAS_MaxDeviceName + 1];
} RASDEVINFOA, *LPRASDEVINFOA;

typedef struct tagRASCTRYINFO {
    DWORD dwSize;
    DWORD dwCountryID;
    DWORD dwNextCountryID;
    DWORD dwCountryCode;
    DWORD dwCountryNameOffset;
} RASCTRYINFO, *LPRASCTRYINFO;

typedef RASCTRYINFO  RASCTRYINFOW, *LPRASCTRYINFOW;
typedef RASCTRYINFO  RASCTRYINFOA, *LPRASCTRYINFOA;

typedef struct tagRASIPADDR {
    BYTE a;
    BYTE b;
    BYTE c;
    BYTE d;
} RASIPADDR;

typedef struct tagRASENTRYW {
    DWORD dwSize;
    DWORD dwfOptions;
    DWORD dwCountryID;
    DWORD dwCountryCode;
    WCHAR szAreaCode[RAS_MaxAreaCode + 1];
    WCHAR szLocalPhoneNumber[RAS_MaxPhoneNumber + 1];
    DWORD dwAlternateOffset;
    RASIPADDR ipaddr;
    RASIPADDR ipaddrDns;
    RASIPADDR ipaddrDnsAlt;
    RASIPADDR ipaddrWins;
    RASIPADDR ipaddrWinsAlt;
    DWORD dwFrameSize;
    DWORD dwfNetProtocols;
    DWORD dwFramingProtocol;
    WCHAR szScript[MAX_PATH];
    WCHAR szAutodialDll[MAX_PATH];
    WCHAR szAutodialFunc[MAX_PATH];
    WCHAR szDeviceType[RAS_MaxDeviceType + 1];
    WCHAR szDeviceName[RAS_MaxDeviceName + 1];
    WCHAR szX25PadType[RAS_MaxPadType + 1];
    WCHAR szX25Address[RAS_MaxX25Address + 1];
    WCHAR szX25Facilities[RAS_MaxFacilities + 1];
    WCHAR szX25UserData[RAS_MaxUserData + 1];
    DWORD dwChannels;
    DWORD dwReserved1;
    DWORD dwReserved2;
#if (WINVER >= 0x401)
    DWORD dwSubEntries;
    DWORD dwDialMode;
    DWORD dwDialExtraPercent;
    DWORD dwDialExtraSampleSeconds;
    DWORD dwHangUpExtraPercent;
    DWORD dwHangUpExtraSampleSeconds;
    DWORD dwIdleDisconnectSeconds;
#endif
#if (WINVER >= 0x500)
    DWORD dwType;
    DWORD dwEncryptionType;
    DWORD dwCustomAuthKey;
    GUID guidId;
    WCHAR szCustomDialDll[MAX_PATH];
    DWORD dwVpnStrategy;
#endif
} RASENTRYW, *LPRASENTRYW;

typedef struct tagRASENTRYA {
    DWORD dwSize;
    DWORD dwfOptions;
    DWORD dwCountryID;
    DWORD dwCountryCode;
    CHAR szAreaCode[RAS_MaxAreaCode + 1];
    CHAR szLocalPhoneNumber[RAS_MaxPhoneNumber + 1];
    DWORD dwAlternateOffset;
    RASIPADDR ipaddr;
    RASIPADDR ipaddrDns;
    RASIPADDR ipaddrDnsAlt;
    RASIPADDR ipaddrWins;
    RASIPADDR ipaddrWinsAlt;
    DWORD dwFrameSize;
    DWORD dwfNetProtocols;
    DWORD dwFramingProtocol;
    CHAR szScript[MAX_PATH];
    CHAR szAutodialDll[MAX_PATH];
    CHAR szAutodialFunc[MAX_PATH];
    CHAR szDeviceType[RAS_MaxDeviceType + 1];
    CHAR szDeviceName[RAS_MaxDeviceName + 1];
    CHAR szX25PadType[RAS_MaxPadType + 1];
    CHAR szX25Address[RAS_MaxX25Address + 1];
    CHAR szX25Facilities[RAS_MaxFacilities + 1];
    CHAR szX25UserData[RAS_MaxUserData + 1];
    DWORD dwChannels;
    DWORD dwReserved1;
    DWORD dwReserved2;
#if (WINVER >= 0x401)
    DWORD dwSubEntries;
    DWORD dwDialMode;
    DWORD dwDialExtraPercent;
    DWORD dwDialExtraSampleSeconds;
    DWORD dwHangUpExtraPercent;
    DWORD dwHangUpExtraSampleSeconds;
    DWORD dwIdleDisconnectSeconds;
#endif
#if (WINVER >= 0x500)
    DWORD dwType;
    DWORD dwEncryptionType;
    DWORD dwCustomAuthKey;
    GUID guidId;
    CHAR szCustomDialDll[MAX_PATH];
    DWORD dwVpnStrategy;
#endif
} RASENTRYA, *LPRASENTRYA;


#if (WINVER >= 0x401)
typedef struct tagRASADPARAMS {
    DWORD dwSize;
    HWND hwndOwner;
    DWORD dwFlags;
    LONG xDlg;
    LONG yDlg;
} RASADPARAMS, *LPRASADPARAMS;

typedef struct tagRASSUBENTRYW {
    DWORD dwSize;
    DWORD dwfFlags;
    WCHAR szDeviceType[RAS_MaxDeviceType + 1];
    WCHAR szDeviceName[RAS_MaxDeviceName + 1];
    WCHAR szLocalPhoneNumber[RAS_MaxPhoneNumber + 1];
    DWORD dwAlternateOffset;
} RASSUBENTRYW, *LPRASSUBENTRYW;

typedef struct tagRASSUBENTRYA {
    DWORD dwSize;
    DWORD dwfFlags;
    CHAR szDeviceType[RAS_MaxDeviceType + 1];
    CHAR szDeviceName[RAS_MaxDeviceName + 1];
    CHAR szLocalPhoneNumber[RAS_MaxPhoneNumber + 1];
    DWORD dwAlternateOffset;
} RASSUBENTRYA, *LPRASSUBENTRYA;

typedef struct tagRASCREDENTIALSW {
    DWORD dwSize;
    DWORD dwMask;
    WCHAR szUserName[UNLEN + 1];
    WCHAR szPassword[PWLEN + 1];
    WCHAR szDomain[DNLEN + 1];
} RASCREDENTIALSW, *LPRASCREDENTIALSW;

typedef struct tagRASCREDENTIALSA {
    DWORD dwSize;
    DWORD dwMask;
    CHAR szUserName[UNLEN + 1];
    CHAR szPassword[PWLEN + 1];
    CHAR szDomain[DNLEN + 1];
} RASCREDENTIALSA, *LPRASCREDENTIALSA;

typedef struct tagRASAUTODIALENTRYW {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwDialingLocation;
    WCHAR szEntry[RAS_MaxEntryName + 1];
} RASAUTODIALENTRYW, *LPRASAUTODIALENTRYW;

typedef struct tagRASAUTODIALENTRYA {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwDialingLocation;
    CHAR szEntry[RAS_MaxEntryName + 1];
} RASAUTODIALENTRYA, *LPRASAUTODIALENTRYA;
#endif /* (WINVER >= 0x401) */

#if (WINVER >= 0x500)
typedef struct tagRASPPPCCP {
    DWORD dwSize;
    DWORD dwError;
    DWORD dwCompressionAlgorithm;
    DWORD dwOptions;
    DWORD dwServerCompressionAlgorithm;
    DWORD dwServerOptions;
} RASPPPCCP, *LPRASPPPCCP;

typedef struct tagRASEAPUSERIDENTITYW {
    WCHAR szUserName[UNLEN + 1];
    DWORD dwSizeofEapInfo;
    BYTE pbEapInfo[1];
} RASEAPUSERIDENTITYW, *LPRASEAPUSERIDENTITYW;

typedef struct tagRASEAPUSERIDENTITYA {
    CHAR szUserName[UNLEN + 1];
    DWORD dwSizeofEapInfo;
    BYTE pbEapInfo[1];
} RASEAPUSERIDENTITYA, *LPRASEAPUSERIDENTITYA;

typedef struct tagRAS_STATS {
    DWORD dwSize;
    DWORD dwBytesXmited;
    DWORD dwBytesRcved;
    DWORD dwFramesXmited;
    DWORD dwFramesRcved;
    DWORD dwCrcErr;
    DWORD dwTimeoutErr;
    DWORD dwAlignmentErr;
    DWORD dwHardwareOverrunErr;
    DWORD dwFramingErr;
    DWORD dwBufferOverrunErr;
    DWORD dwCompressionRatioIn;
    DWORD dwCompressionRatioOut;
    DWORD dwBps;
    DWORD dwConnectDuration;
} RAS_STATS, *PRAS_STATS;
#endif /* (WINVER >= 0x500) */


/* UNICODE typedefs for structures*/
#ifdef UNICODE
typedef RASCONNW RASCONN, *LPRASCONN;
typedef RASENTRYW  RASENTRY, *LPRASENTRY;
typedef RASCONNSTATUSW RASCONNSTATUS, *LPRASCONNSTATUS;
typedef RASDIALPARAMSW RASDIALPARAMS, *LPRASDIALPARAMS;
typedef RASAMBW RASAMB, *LPRASAM;
typedef RASPPPNBFW RASPPPNBF, *LPRASPPPNBF;
typedef RASPPPIPXW RASPPPIPX, *LPRASPPPIPX;
typedef RASPPPIPW RASPPPIP, *LPRASPPPIP;
typedef RASPPPLCPW RASPPPLCP, *LPRASPPPLCP;
typedef RASSLIPW RASSLIP, *LPRASSLIP;
typedef RASDEVINFOW  RASDEVINFO, *LPRASDEVINFO;
typedef RASENTRYNAMEW RASENTRYNAME, *LPRASENTRYNAME;

#if (WINVER >= 0x401)
typedef RASSUBENTRYW RASSUBENTRY, *LPRASSUBENTRY;
typedef RASCREDENTIALSW RASCREDENTIALS, *LPRASCREDENTIALS;
typedef RASAUTODIALENTRYW RASAUTODIALENTRY, *LPRASAUTODIALENTRY;
#endif /* (WINVER >= 0x401) */

#if (WINVER >= 0x500)
typedef RASEAPUSERIDENTITYW RASEAPUSERIDENTITY, *LPRASEAPUSERIDENTITY;
#endif /* (WINVER >= 0x500) */

#else  /* ! defined UNICODE */
typedef RASCONNA RASCONN, *LPRASCONN;
typedef RASENTRYA  RASENTRY, *LPRASENTRY;
typedef RASCONNSTATUSA RASCONNSTATUS, *LPRASCONNSTATUS;
typedef RASDIALPARAMSA RASDIALPARAMS, *LPRASDIALPARAMS;
typedef RASAMBA RASAMB, *LPRASAM;
typedef RASPPPNBFA RASPPPNBF, *LPRASPPPNBF;
typedef RASPPPIPXA RASPPPIPX, *LPRASPPPIPX;
typedef RASPPPIPA RASPPPIP, *LPRASPPPIP;
typedef RASPPPLCPA RASPPPLCP, *LPRASPPPLCP;
typedef RASSLIPA RASSLIP, *LPRASSLIP;
typedef RASDEVINFOA  RASDEVINFO, *LPRASDEVINFO;
typedef RASENTRYNAMEA RASENTRYNAME, *LPRASENTRYNAME;

#if (WINVER >= 0x401)
typedef RASSUBENTRYA RASSUBENTRY, *LPRASSUBENTRY;
typedef RASCREDENTIALSA RASCREDENTIALS, *LPRASCREDENTIALS;
typedef RASAUTODIALENTRYA RASAUTODIALENTRY, *LPRASAUTODIALENTRY;
#endif /*(WINVER >= 0x401)*/
#if (WINVER >= 0x500)
typedef RASEAPUSERIDENTITYA RASEAPUSERIDENTITY, *LPRASEAPUSERIDENTITY;
#endif /* (WINVER >= 0x500) */
#endif /* ! UNICODE */

/* Callback prototypes */
typedef BOOL (WINAPI * ORASADFUNC) (HWND, LPSTR, DWORD, LPDWORD); /* deprecated */
typedef VOID (WINAPI * RASDIALFUNC) (UINT, RASCONNSTATE, DWORD);
typedef VOID (WINAPI * RASDIALFUNC1) (HRASCONN, UINT, RASCONNSTATE, DWORD,
					DWORD);
typedef DWORD (WINAPI * RASDIALFUNC2) (ULONG_PTR, DWORD, HRASCONN, UINT,
					RASCONNSTATE, DWORD, DWORD);

/* External functions */
DWORD APIENTRY RasDialA (LPRASDIALEXTENSIONS, LPCSTR, LPRASDIALPARAMSA,
	    		DWORD, LPVOID, LPHRASCONN);
DWORD APIENTRY RasDialW (LPRASDIALEXTENSIONS, LPCWSTR, LPRASDIALPARAMSW,
    	        DWORD, LPVOID, LPHRASCONN);
DWORD APIENTRY RasEnumConnectionsA (LPRASCONNA, LPDWORD, LPDWORD);
DWORD APIENTRY RasEnumConnectionsW (LPRASCONNW, LPDWORD, LPDWORD);
DWORD APIENTRY RasEnumEntriesA (LPCSTR, LPCSTR, LPRASENTRYNAMEA, LPDWORD,
				LPDWORD);
DWORD APIENTRY RasEnumEntriesW (LPCWSTR, LPCWSTR, LPRASENTRYNAMEW, LPDWORD,
				LPDWORD);
DWORD APIENTRY RasGetConnectStatusA (HRASCONN, LPRASCONNSTATUSA);
DWORD APIENTRY RasGetConnectStatusW (HRASCONN, LPRASCONNSTATUSW);
DWORD APIENTRY RasGetErrorStringA (UINT, LPSTR, DWORD);
DWORD APIENTRY RasGetErrorStringW (UINT, LPWSTR, DWORD);
DWORD APIENTRY RasHangUpA (HRASCONN);
DWORD APIENTRY RasHangUpW (HRASCONN);
DWORD APIENTRY RasGetProjectionInfoA (HRASCONN, RASPROJECTION, LPVOID,
 				LPDWORD);
DWORD APIENTRY RasGetProjectionInfoW (HRASCONN, RASPROJECTION, LPVOID,
				LPDWORD);
DWORD APIENTRY RasCreatePhonebookEntryA (HWND, LPCSTR);
DWORD APIENTRY RasCreatePhonebookEntryW (HWND, LPCWSTR);
DWORD APIENTRY RasEditPhonebookEntryA (HWND, LPCSTR, LPCSTR);
DWORD APIENTRY RasEditPhonebookEntryW (HWND, LPCWSTR, LPCWSTR);
DWORD APIENTRY RasSetEntryDialParamsA (LPCSTR, LPRASDIALPARAMSA, BOOL);
DWORD APIENTRY RasSetEntryDialParamsW (LPCWSTR, LPRASDIALPARAMSW, BOOL);
DWORD APIENTRY RasGetEntryDialParamsA (LPCSTR, LPRASDIALPARAMSA, LPBOOL);
DWORD APIENTRY RasGetEntryDialParamsW (LPCWSTR, LPRASDIALPARAMSW, LPBOOL);
DWORD APIENTRY RasEnumDevicesA (LPRASDEVINFOA, LPDWORD, LPDWORD);
DWORD APIENTRY RasEnumDevicesW (LPRASDEVINFOW, LPDWORD, LPDWORD);
DWORD APIENTRY RasGetCountryInfoA (LPRASCTRYINFOA, LPDWORD);
DWORD APIENTRY RasGetCountryInfoW (LPRASCTRYINFOW, LPDWORD);
DWORD APIENTRY RasGetEntryPropertiesA (LPCSTR, LPCSTR, LPRASENTRYA, LPDWORD,
				 LPBYTE, LPDWORD);
DWORD APIENTRY RasGetEntryPropertiesW (LPCWSTR, LPCWSTR, LPRASENTRYW,
				 LPDWORD, LPBYTE, LPDWORD);
DWORD APIENTRY RasSetEntryPropertiesA (LPCSTR, LPCSTR, LPRASENTRYA, DWORD,
				 LPBYTE, DWORD);
DWORD APIENTRY RasSetEntryPropertiesW (LPCWSTR, LPCWSTR, LPRASENTRYW, DWORD,
				 LPBYTE, DWORD);
DWORD APIENTRY RasRenameEntryA (LPCSTR, LPCSTR, LPCSTR);
DWORD APIENTRY RasRenameEntryW (LPCWSTR, LPCWSTR, LPCWSTR);
DWORD APIENTRY RasDeleteEntryA (LPCSTR, LPCSTR);
DWORD APIENTRY RasDeleteEntryW (LPCWSTR, LPCWSTR);
DWORD APIENTRY RasValidateEntryNameA (LPCSTR, LPCSTR);
DWORD APIENTRY RasValidateEntryNameW (LPCWSTR, LPCWSTR);

#if (WINVER >= 0x401)
typedef BOOL (WINAPI * RASADFUNCA) (LPSTR, LPSTR, LPRASADPARAMS, LPDWORD);
typedef BOOL (WINAPI * RASADFUNCW) (LPWSTR, LPWSTR, LPRASADPARAMS, LPDWORD);

DWORD APIENTRY RasGetSubEntryHandleA (HRASCONN, DWORD, LPHRASCONN);
DWORD APIENTRY RasGetSubEntryHandleW (HRASCONN, DWORD, LPHRASCONN);
DWORD APIENTRY RasGetCredentialsA (LPCSTR, LPCSTR, LPRASCREDENTIALSA);
DWORD APIENTRY RasGetCredentialsW (LPCWSTR, LPCWSTR, LPRASCREDENTIALSW);
DWORD APIENTRY RasSetCredentialsA (LPCSTR, LPCSTR, LPRASCREDENTIALSA, BOOL);
DWORD APIENTRY RasSetCredentialsW (LPCWSTR, LPCWSTR, LPRASCREDENTIALSW, BOOL);
DWORD APIENTRY RasConnectionNotificationA (HRASCONN, HANDLE, DWORD);
DWORD APIENTRY RasConnectionNotificationW (HRASCONN, HANDLE, DWORD);
DWORD APIENTRY RasGetSubEntryPropertiesA (LPCSTR, LPCSTR, DWORD,
					LPRASSUBENTRYA, LPDWORD, LPBYTE, LPDWORD);
DWORD APIENTRY RasGetSubEntryPropertiesW (LPCWSTR, LPCWSTR, DWORD,
					LPRASSUBENTRYW, LPDWORD, LPBYTE, LPDWORD);
DWORD APIENTRY RasSetSubEntryPropertiesA (LPCSTR, LPCSTR, DWORD,
					LPRASSUBENTRYA, DWORD, LPBYTE, DWORD);
DWORD APIENTRY RasSetSubEntryPropertiesW (LPCWSTR, LPCWSTR, DWORD,
					LPRASSUBENTRYW, DWORD, LPBYTE, DWORD);
DWORD APIENTRY RasGetAutodialAddressA (LPCSTR, LPDWORD, LPRASAUTODIALENTRYA,
			        LPDWORD, LPDWORD);
DWORD APIENTRY RasGetAutodialAddressW (LPCWSTR, LPDWORD,
					LPRASAUTODIALENTRYW, LPDWORD, LPDWORD);
DWORD APIENTRY RasSetAutodialAddressA (LPCSTR, DWORD, LPRASAUTODIALENTRYA,
					DWORD, DWORD);
DWORD APIENTRY RasSetAutodialAddressW (LPCWSTR, DWORD, LPRASAUTODIALENTRYW,
					DWORD, DWORD);
DWORD APIENTRY RasEnumAutodialAddressesA (LPSTR *, LPDWORD, LPDWORD);
DWORD APIENTRY RasEnumAutodialAddressesW (LPWSTR *, LPDWORD, LPDWORD);
DWORD APIENTRY RasGetAutodialEnableA (DWORD, LPBOOL);
DWORD APIENTRY RasGetAutodialEnableW (DWORD, LPBOOL);
DWORD APIENTRY RasSetAutodialEnableA (DWORD, BOOL);
DWORD APIENTRY RasSetAutodialEnableW (DWORD, BOOL);
DWORD APIENTRY RasGetAutodialParamA (DWORD, LPVOID, LPDWORD);
DWORD APIENTRY RasGetAutodialParamW (DWORD, LPVOID, LPDWORD);
DWORD APIENTRY RasSetAutodialParamA (DWORD, LPVOID, DWORD);
DWORD APIENTRY RasSetAutodialParamW (DWORD, LPVOID, DWORD);
#endif

#if (WINVER >= 0x500)
typedef DWORD (WINAPI * RasCustomHangUpFn) (HRASCONN);
typedef DWORD (WINAPI * RasCustomDeleteEntryNotifyFn) (LPCTSTR,	LPCTSTR, DWORD);
typedef DWORD (WINAPI * RasCustomDialFn) (HINSTANCE, LPRASDIALEXTENSIONS,
				LPCTSTR, LPRASDIALPARAMS, DWORD, LPVOID, LPHRASCONN, DWORD);

DWORD APIENTRY RasInvokeEapUI (HRASCONN, DWORD, LPRASDIALEXTENSIONS, HWND);
DWORD APIENTRY RasGetLinkStatistics (HRASCONN, DWORD, RAS_STATS*);
DWORD APIENTRY RasGetConnectionStatistics (HRASCONN, RAS_STATS*);
DWORD APIENTRY RasClearLinkStatistics (HRASCONN, DWORD);
DWORD APIENTRY RasClearConnectionStatistics (HRASCONN);
DWORD APIENTRY RasGetEapUserDataA (HANDLE, LPCSTR, LPCSTR, BYTE*, DWORD*);
DWORD APIENTRY RasGetEapUserDataW (HANDLE, LPCWSTR, LPCWSTR, BYTE*, DWORD*);
DWORD APIENTRY RasSetEapUserDataA (HANDLE, LPCSTR, LPCSTR, BYTE*, DWORD);
DWORD APIENTRY RasSetEapUserDataW (HANDLE, LPCWSTR, LPCWSTR, BYTE*, DWORD);
DWORD APIENTRY RasGetCustomAuthDataA (LPCSTR,	LPCSTR,	BYTE*,	DWORD*);
DWORD APIENTRY RasGetCustomAuthDataW (LPCWSTR, LPCWSTR, BYTE*, DWORD*);
DWORD APIENTRY RasSetCustomAuthDataA (LPCSTR,	LPCSTR,	BYTE*,	DWORD);
DWORD APIENTRY RasSetCustomAuthDataW (LPCWSTR, LPCWSTR, BYTE*, DWORD);
DWORD APIENTRY RasGetEapUserIdentityW (LPCWSTR, LPCWSTR, DWORD, HWND, LPRASEAPUSERIDENTITYW*);
DWORD APIENTRY RasGetEapUserIdentityA (LPCSTR, LPCSTR, DWORD, HWND, LPRASEAPUSERIDENTITYA*);
VOID APIENTRY RasFreeEapUserIdentityW (LPRASEAPUSERIDENTITYW);
VOID APIENTRY RasFreeEapUserIdentityA (LPRASEAPUSERIDENTITYA);
#endif  /* (WINVER >= 0x500) */


/* UNICODE defines for functions */
#ifdef UNICODE
#define RasDial RasDialW
#define RasEnumConnections RasEnumConnectionsW
#define RasEnumEntries RasEnumEntriesW
#define RasGetConnectStatus RasGetConnectStatusW
#define RasGetErrorString RasGetErrorStringW
#define RasHangUp RasHangUpW
#define RasGetProjectionInfo RasGetProjectionInfoW
#define RasCreatePhonebookEntry RasCreatePhonebookEntryW
#define RasEditPhonebookEntry RasEditPhonebookEntryW
#define RasSetEntryDialParams RasSetEntryDialParamsW
#define RasGetEntryDialParams RasGetEntryDialParamsW
#define RasEnumDevices RasEnumDevicesW
#define RasGetCountryInfo RasGetCountryInfoW
#define RasGetEntryProperties RasGetEntryPropertiesW
#define RasSetEntryProperties RasSetEntryPropertiesW
#define RasRenameEntry RasRenameEntryW
#define RasDeleteEntry RasDeleteEntryW
#define RasValidateEntryName RasValidateEntryNameW
#if (WINVER >= 0x401)
#define RASADFUNC RASADFUNCW
#define RasGetSubEntryHandle RasGetSubEntryHandleW
#define RasConnectionNotification RasConnectionNotificationW
#define RasGetSubEntryProperties RasGetSubEntryPropertiesW
#define RasSetSubEntryProperties RasSetSubEntryPropertiesW
#define RasGetCredentials RasGetCredentialsW
#define RasSetCredentials RasSetCredentialsW
#define RasGetAutodialAddress RasGetAutodialAddressW
#define RasSetAutodialAddress RasSetAutodialAddressW
#define RasEnumAutodialAddresses RasEnumAutodialAddressesW
#define RasGetAutodialEnable RasGetAutodialEnableW
#define RasSetAutodialEnable RasSetAutodialEnableW
#define RasGetAutodialParam RasGetAutodialParamW
#define RasSetAutodialParam RasSetAutodialParamW
#endif /* (WINVER >= 0x401) */
#if (WINVER >= 0x500)
#define RasGetEapUserData RasGetEapUserDataW
#define RasSetEapUserData RasSetEapUserDataW
#define RasGetCustomAuthData RasGetCustomAuthDataW
#define RasSetCustomAuthData RasSetCustomAuthDataW
#define RasGetEapUserIdentity RasGetEapUserIdentityW
#define RasFreeEapUserIdentity RasFreeEapUserIdentityW
#endif /* (WINVER >= 0x500) */

#else  /* ! defined UNICODE */
#define RasDial RasDialA
#define RasEnumConnections RasEnumConnectionsA
#define RasEnumEntries RasEnumEntriesA
#define RasGetConnectStatus RasGetConnectStatusA
#define RasGetErrorString RasGetErrorStringA
#define RasHangUp RasHangUpA
#define RasGetProjectionInfo RasGetProjectionInfoA
#define RasCreatePhonebookEntry RasCreatePhonebookEntryA
#define RasEditPhonebookEntry RasEditPhonebookEntryA
#define RasSetEntryDialParams RasSetEntryDialParamsA
#define RasGetEntryDialParams RasGetEntryDialParamsA
#define RasEnumDevices RasEnumDevicesA
#define RasGetCountryInfo RasGetCountryInfoA
#define RasGetEntryProperties RasGetEntryPropertiesA
#define RasSetEntryProperties RasSetEntryPropertiesA
#define RasRenameEntry RasRenameEntryA
#define RasDeleteEntry RasDeleteEntryA
#define RasValidateEntryName RasValidateEntryNameA

#if (WINVER >= 0x401)
#define RASADFUNC RASADFUNCA
#define RasGetSubEntryHandle RasGetSubEntryHandleA
#define RasConnectionNotification RasConnectionNotificationA
#define RasGetSubEntryProperties RasGetSubEntryPropertiesA
#define RasSetSubEntryProperties RasSetSubEntryPropertiesA
#define RasGetCredentials RasGetCredentialsA
#define RasSetCredentials RasSetCredentialsA
#define RasGetAutodialAddress RasGetAutodialAddressA
#define RasSetAutodialAddress RasSetAutodialAddressA
#define RasEnumAutodialAddressesRasEnumAutodialAddressesA
#define RasGetAutodialEnable RasGetAutodialEnableA
#define RasSetAutodialEnable RasSetAutodialEnableA
#define RasGetAutodialParam RasGetAutodialParamA
#define RasSetAutodialParam RasSetAutodialParamA
#endif /*(WINVER >= 0x401)*/

#if (WINVER >= 0x500)
#define RasGetEapUserData RasGetEapUserDataA
#define RasSetEapUserData RasSetEapUserDataA
#define RasGetCustomAuthData RasGetCustomAuthDataA
#define RasSetCustomAuthData RasSetCustomAuthDataA
#define RasGetEapUserIdentity RasGetEapUserIdentityA
#define RasFreeEapUserIdentity RasFreeEapUserIdentityA
#endif /* (WINVER >= 0x500) */
#endif /* ! UNICODE */

#ifdef __cplusplus
}
#endif
#include <poppack.h>
#endif /* _RAS_H */
