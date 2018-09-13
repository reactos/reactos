/* Copyright (c) 1992-1995, Microsoft Corporation, all rights reserved
**
** ras.h
** Remote Access Session Management Service Provider Interface
** Public header for Session Management Provider Interface
*/

#ifndef _RNASPI_H_
#define _RNASPI_H_


//****************************************************************************
// RNA Session Management Module Service Provider Interface
//****************************************************************************

#define     RNA_MaxSMMType         32

// The type of RNA session
//
typedef enum {
    SESSTYPE_INITIATOR,
    SESSTYPE_RESPONDER
} SESSTYPE;

// Session configuration options
//
#define SMMCFG_SW_COMPRESSION       0x00000001  // Software compression is on
#define SMMCFG_PW_ENCRYPTED         0x00000002  // Encrypted password only
#define SMMCFG_NW_LOGON             0x00000004  // Logon to the network
#define SMMCFG_SW_ENCRYPTION        0x00000010  // SW encryption is okay
#define SMMCFG_MULTILINK            0x80000000  // Use multilink

#define SMMCFG_ALL                  0x00000017  // All the user-specified options

// Negotiated protocols
//
#define SMMPROT_NB                  0x00000001  // NetBEUI
#define SMMPROT_IPX                 0x00000002  // IPX
#define SMMPROT_IP                  0x00000004  // TCP/IP

#define SMMPROT_ALL                 0x00000007  // all protocols negotiated

// Error codes that a MAC can return when posting disconnect message
//
#define MACERR_REMOTE_DISCONNECTING 0x00000001
#define MACERR_REMOTE_NO_RESPONSE   0x00000002


// The session confuration information
//
typedef struct tagSESS_CONFIGURATION_INFO
{
    DWORD           dwSize;
    char            szEntryName[RAS_MaxEntryName + 1];
    SESSTYPE        stSessType;
    DWORD           fdwSessOption;
    DWORD           fdwProtocols;
    BOOL            fUserSecurity;
    char            szUserName[UNLEN + 1];
    char            szPassword[PWLEN + 1];
    char            szDomainName[DNLEN + 1];
} SESS_CONFIGURATION_INFO, *PSESS_CONFIGURATION_INFO,
  FAR *LPSESS_CONFIGURATION_INFO;

// Session configuration start/stop functions
//
typedef DWORD (WINAPI * SESSSTARTPROC)(HANDLE, LPSESS_CONFIGURATION_INFO);
typedef DWORD (WINAPI * SESSSTOPPROC)(HANDLE);

// Session configuration entry point table
//
typedef struct tagRNA_FUNCS
{
    DWORD           dwSize;                    // The structure size
    SESSSTARTPROC   lpfnStart;                 // RnaSessStart Entry
    SESSSTOPPROC    lpfnStop;                  // RnaSessStop Entry
} RNA_FUNCS, *PRNA_FUNCS, FAR *LPRNA_FUNCS;

// Session Management Module initialization function
//
typedef DWORD (WINAPI * SESSINITIALIZEPROC)(LPSTR, LPRNA_FUNCS);

//****************************************************************************
// RNA Session Manager Service Interface
//****************************************************************************

typedef struct  tagRnaComplete_Info
{
    DWORD           dwSize;                     // The structure size
    DWORD           dwResult;                   // The returning error code
    UINT            idMsg;                      // SMM-specific error message ID
    BOOL            fUnload;                    // Unload the module on success?
    HANDLE          hThread;                    // Event to wait for unloading
} COMPLETE_INFO, *PCOMPLETE_INFO, FAR *LPCOMPLETE_INFO;

typedef struct  tagProjection_Info
{
    DWORD           dwSize;                     // The structure size
    RASPROJECTION   RasProjection;              // The projection type
    union {
        RASAMB      RasAMB;
        RASPPPNBF   RasPPPNBF;
        RASPPPIPX   RasPPPIPX;
        RASPPPIP    RasPPPIP;
    }               ProjInfo;
} PROJECTION_INFO, *PPROJECTION_INFO, FAR *LPPROJECTION_INFO;

//
// Responses to Session Management Request
//
DWORD WINAPI RnaComplete( HANDLE hConn, LPCOMPLETE_INFO lpci,
                          LPPROJECTION_INFO lppi, DWORD cEntries);
DWORD WINAPI RnaTerminate( HANDLE hConn, HANDLE hThread );

//
// MAC management services
//

#define IEEE_ADDRESS_LENGTH	6   // Token-ring and Ethernet address lengths

typedef struct tagMAC_FEATURES {
    DWORD           SendFeatureBits;	// A bit field of compression/features sendable
    DWORD           RecvFeatureBits;	// A bit field of compression/features receivable
    DWORD           MaxSendFrameSize;	// Maximum frame size that can be sent
                                        // must be less than or equal default
    DWORD           MaxRecvFrameSize;	// Maximum frame size that can be rcvd
                                        // must be less than or equal default
    DWORD           LinkSpeed;		// New RAW link speed in bits/sec
                                        // Ignored if 0
} MAC_FEATURES, *PMAC_FEATURES, FAR* LPMAC_FEATURES;

#pragma pack(4)
typedef struct tagMAC_OPEN {
    WORD            hRasEndpoint;        // unique for each endpoint assigned
    LPVOID          MacAdapter;          // Which binding to AsyMac to use
                                         // if NULL, will default to last binding
    DWORD           LinkSpeed;           // RAW link speed in bits per sec
    WORD            QualOfConnect;       // NdisAsyncRaw, NdisAsyncErrorControl, ...

    BYTE            IEEEAddress[IEEE_ADDRESS_LENGTH];	// The 802.5 or 802.3
    MAC_FEATURES    macFeatures;         // Readable configuration parameters
    enum {                               // All different types of device drivers
                    SERIAL_DEVICE,       // are listed here.  For instance
                    SNA_DEVICE,          // the serial driver requires diff.
                                         // irps than the SNA driver.
                    MINIPORT_DEVICE      // NDIS WAN Miniport Devices.

    }               DeviceType;

    union {                              // handles required for above device
                                         // driver types.
        LONG        FileHandle;          // the Win32 or Nt File Handle
        struct SNAHandle {
            LPVOID  ReadHandle;
            LPVOID  WriteHandle;
        };
    }               Handles;

    DWORD           hWndConn;            // Window handle for connection
    DWORD           wMsg;                // The msg to post when disconnecting
    DWORD           dwStatus;            // The status of the open call
} MAC_OPEN, *PMAC_OPEN, FAR* LPMAC_OPEN;
#pragma pack()

typedef struct tagDEVICE_PORT_INFO {
    DWORD   dwSize;
    HANDLE  hDevicePort;
    HANDLE  hLine;
    HANDLE  hCall;
    DWORD   dwAddressID;
    DWORD   dwLinkSpeed;
    char    szDeviceClass[RAS_MaxDeviceType+1];
} DEVICE_PORT_INFO, *PDEVICE_PORT_INFO, FAR* LPDEVICE_PORT_INFO;

DWORD WINAPI RnaGetDevicePort( HANDLE hConn, LPDEVICE_PORT_INFO lpdpi );
DWORD WINAPI RnaOpenMac( HANDLE hConn, HANDLE *lphMAC,
                         LPMAC_OPEN lpmo, DWORD dwSize, HANDLE hEvent );
DWORD WINAPI RnaCloseMac( HANDLE hConn );

//
// User Profile Services
//

typedef enum tagRNAACCESSTYPE { PCONLY, NETANDPC } RNAACCESSTYPE;

typedef struct tagUSER_PROFILE
{
    DWORD           dwSize;
    char            szUserName[UNLEN + 1];
    char            szPassword[PWLEN + 1];
    char            szDomainName[DNLEN + 1];
    BOOL            fUseCallbacks;
    RNAACCESSTYPE   accesstype;
    UINT            uTimeOut;
    
} USER_PROFILE, *PUSER_PROFILE, FAR *LPUSER_PROFILE;

DWORD WINAPI RnaGetUserProfile( HANDLE hConn, LPUSER_PROFILE lpUserProfile );

//
// Callback security services
//

// Callback security type
//
enum {
    CALLBACK_SECURE,
    CALLBACK_CONVENIENCE
};

DWORD WINAPI RnaGetCallbackList( DWORD * lpdwType,
                                 LPSTR lpszLocList, LPINT lpcbLoc,
                                 LPSTR lpszPhoneList, LPINT lpcbPhone,
                                 LPINT lpcEntries);
DWORD WINAPI RnaUICallbackDialog( HANDLE hConn, LPSTR lpszLocList,
                                  DWORD dwType, BOOL  fOptional,
                                  LPINT lpIndex,
                                  LPSTR lpszSelectLocation, UINT cbBuff);
DWORD WINAPI RnaStartCallback( HANDLE hConn, HANDLE hEvent);

// Miscellaneous services
//
DWORD WINAPI RnaUIUsernamePassword( HANDLE hConn,    LPSTR lpszUsername,
                                    UINT cbUsername, LPSTR lpszPassword,
                                    UINT cbPassword, LPSTR lpszDomain,
                                    UINT cbDomain);
DWORD WINAPI RnaUIChangePassword( HANDLE hConn,    LPSTR lpszUsername,
                                  UINT cbPassword);
DWORD WINAPI RnaGetOverlaidSMM ( LPSTR lpszSMMType, LPSTR lpszModuleName,
                                 LPBYTE lpBuf, DWORD dwSize);

#endif // _RNASPI_H_
