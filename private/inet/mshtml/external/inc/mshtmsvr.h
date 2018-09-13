/*++

     Copyright (c) 1998 Microsoft Corporation

     Module Name:
         mshtmsvr.h

     Abstract:
         This header file declares the API for connecting IIS to MSHTML.DLL

     Authors:
         Anand Ramakrishna (anandra)
         Dmitry Robsman (dmitryr)

     Revision History:

--*/

#ifndef _MSHTMSVR_H_
#define _MSHTMSVR_H_

//
//  Callback definitions
//
//

typedef BOOL
(WINAPI *PFN_SVR_WRITER_CALLBACK)(
    VOID *pvSvrContext,             // [in] passed to GetDLText
    void *pvBuffer,                 // [in] data
    DWORD cbBuffer                  // [in] data length (bytes)
    );

typedef BOOL
(WINAPI *PFN_SVR_MAPPER_CALLBACK)(
    VOID *pvSvrContext,             // [in] passed to GetDLText
    CHAR *pchVirtualFileName,       // [in] virtual file name to map
    CHAR *pchPhysicalFilename,      // [in, out] physical file name
    DWORD cchMax                    // [in] buffer size
    );

typedef BOOL
(WINAPI *PFN_SVR_GETINFO_CALLBACK)(
    VOID *pvSvrContext,             // [in] passed to GetDLText
    DWORD dwInfo,                   // [in] One of the SVRINFO_XXXX contants
    CHAR *pchBuffer,                // [in, out] buffer
    DWORD cchMax                    // [in] buffer size
    );

//
//  Constants for GETINFO callback
//
                                        // Example          http://host/page.htm?a=v
#define SVRINFO_PROTOCOL        1       // SERVER_PROTOCOL  HTTP/1.1
#define SVRINFO_HOST            2       // SERVER_NAME      host
#define SVRINFO_PATH            3       // PATH_INFO        /page.htm
#define SVRINFO_PATH_TRANSLATED 4       // PATH_TRANSLATED  c:\wwwroot\page.htm
#define SVRINFO_QUERY_STRING    5       // QUERY_STRING     a=v
#define SVRINFO_PORT            6       // SERVER_PORT      80
#define SVRINFO_METHOD          7       // REQUEST_METHOD   GET
#define SVRINFO_USERAGENT       8       // HTTP_USER_AGENT  Mozilla/4.0 ...

//
//  Constants for Normalized User Agent
//

#define USERAGENT_RESERVED  0xffffffff
#define USERAGENT_DEFAULT   0
#define USERAGENT_IE3       1
#define USERAGENT_IE4       2
#define USERAGENT_NAV5      3
#define USERAGENT_NAV4      4
#define USERAGENT_NAV3      5
#define USERAGENT_IE5       10000

//
//  The API
//
//

#ifdef __cplusplus
extern "C" {
#endif

BOOL WINAPI 
SvrTri_NormalizeUA(
    CHAR  *pchUA,                      // [in] User agent string
    DWORD *pdwUA                       // [out] User agend id
    );

BOOL WINAPI
SvrTri_GetDLText(
    VOID *pvSrvContext,                // [in] Server Context
    DWORD dwUA,                        // [in] User Agent (Normalized)
    CHAR *pchFileName,                 // [in] Physical file name of htm file
    IDispatch *pdisp,                  // [in] OA 'Server' object for scripting
    PFN_SVR_GETINFO_CALLBACK pfnInfo,  // [in] GetInfo callback
    PFN_SVR_MAPPER_CALLBACK pfnMapper, // [in] Mapper callback
    PFN_SVR_WRITER_CALLBACK pfnWriter, // [in] Writer callback
    DWORD *rgdwUAEquiv,                // [in, out] Array of ua equivalences
    DWORD cUAEquivMax,                 // [in] Size of array of ua equiv
    DWORD *pcUAEquiv                   // [out] # of UA Equivalencies filled in
    );

#ifdef __cplusplus
};
#endif

#endif // _MSHTMSVR_H_

/************************ End of File ***********************/