//#----------------------------------------------------------------------------
//
//  File:           sspspm.h
//
//      Synopsis:   Definitions specific to SSPI SPM DLL.
//
//      Copyright (C) 1995  Microsoft Corporation.  All Rights Reserved.
//
//  Authors:        LucyC       Created                         25 Sept 1995
//
//-----------------------------------------------------------------------------
#ifndef _SSPSPM_H_
#define _SSPSPM_H_

#include <platform.h>

//
//  Names of secruity DLL
//
#define SSP_SPM_NT_DLL      "security.dll"
#define SSP_SPM_WIN95_DLL   "secur32.dll"
#define SSP_SPM_UNIX_DLL    "secur32.dll"
#define SSP_SPM_SSPC_DLL    "msnsspc.dll"

#define SSP_SPM_DLL_NAME_SIZE   16          // max. length of security DLL names

#define MAX_SSPI_PKG        32              // Max. no. of SSPI supported

#define SSPPKG_ERROR        ((UCHAR) 0xff)
#define SSPPKG_NO_PKG       SSPPKG_ERROR
#define MAX_AUTH_MSG_SIZE   10000
#define TCP_PRINT   fprintf
#define DBG_CONTEXT stderr

#define MAX_BLOB_SIZE       13000

//
//  Server host list definition.
//  This list contains server hosts which do not use MSN authentication.
//  The following defines an entry in the server host list.
//
typedef struct _ssp_host_list
{
    struct _ssp_host_list   *pNext;

    unsigned char           *pHostname; // name of server host
    unsigned char           pkgID;      // the package being used for this host

} SspHosts, *PSspHosts;

//
//  List of SSPI packages installed on this machine.
//  The following defines an entry of the SSPI package list.
//
typedef struct _ssp_auth_pkg
{
    LPTSTR       pName;         // package name
    DWORD        Capabilities ; // Interesting capabilities bit
} SSPAuthPkg, *PSSPAuthPkg;

#define SSPAUTHPKG_SUPPORT_NTLM_CREDS   0x00000001

//
//  The following defines the global data structure which the SPM DLL keeps
//  in the HTSPM structure.
//
typedef struct _ssp_htspm
{
    PSecurityFunctionTable pFuncTbl;

    SSPAuthPkg      **PkgList;          // array of pointers to auth packages
    UCHAR           PkgCnt;

    UCHAR           MsnPkg;             // Index to MSN pkg in the pkg list

    BOOLEAN         bKeepList;          // whether to keep a list of servers
                                        // which use non-MSN SSPI packages
    PSspHosts       pHostlist;

} SspData, *PSspData;

#define SPM_STATUS_OK                   0
#define SPM_ERROR                       1
#define SPM_STATUS_WOULD_BLOCK          2
#define SPM_STATUS_INSUFFICIENT_BUFFER  3

/////////////////////////////////////////////////////////////////////////////
//
//  Function headers from sspcalls.c
//
/////////////////////////////////////////////////////////////////////////////

DWORD
GetSecAuthMsg (
    PSspData        pData,
    PCredHandle     pCredential,
    DWORD           pkgID,              // the package index into package list
    PCtxtHandle     pInContext,
    PCtxtHandle     pOutContext,
    ULONG           fContextReq,        // Request Flags
    VOID            *pBuffIn,
    DWORD           cbBuffIn,
    char            *pFinalBuff,
    DWORD           *pcbBuffOut,
    SEC_CHAR        *pszTarget,         // Server Host Name
    UINT            bNonBlock,
    LPSTR           pszScheme,
	SECURITY_STATUS *pssResult
    );

INT
GetPkgId(LPTSTR  lpszPkgName);

DWORD
GetPkgCapabilities(
    INT Package
    );

/////////////////////////////////////////////////////////////////////////////
//
//  Function headers from buffspm.c
//
/////////////////////////////////////////////////////////////////////////////

PSspHosts
SspSpmNewHost (
    PSspData pData,
    UCHAR    *pHost,       // name of server host to be added
    UCHAR    Package
    );

VOID
SspSpmDeleteHost(
    SspData     *pData,
    PSspHosts   pDelHost
    );

VOID
SspSpmTrashHostList(
    SspData     *pData
    );

PSspHosts
SspSpmGetHost(
    PSspData pData,
    UCHAR *pHost
    );


#endif  /* _SSPSPM_H_ */
