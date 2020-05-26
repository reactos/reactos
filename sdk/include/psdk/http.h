/*
 * HTTP Server API definitions
 *
 * Copyright (C) 2009 Andrey Turkin
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_HTTP_H
#define __WINE_HTTP_H

#include <winsock2.h>
#include <ws2tcpip.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _HTTPAPI_VERSION
{
    USHORT HttpApiMajorVersion;
    USHORT HttpApiMinorVersion;
} HTTPAPI_VERSION, *PHTTPAPI_VERSION;

#define HTTPAPI_VERSION_1 {1,0}
#define HTTPAPI_VERSION_2 {2,0}

#define HTTP_CREATE_REQUEST_QUEUE_FLAG_OPEN_EXISTING    0x00000001
#define HTTP_CREATE_REQUEST_QUEUE_FLAG_CONTROLLER       0x00000002

#define HTTP_INITIALIZE_SERVER 0x00000001
#define HTTP_INITIALIZE_CONFIG 0x00000002

#define HTTP_RECEIVE_REQUEST_FLAG_COPY_BODY     0x00000001
#define HTTP_RECEIVE_REQUEST_FLAG_FLUSH_BODY    0x00000002

#define HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS   0x00000001
#define HTTP_REQUEST_FLAG_IP_ROUTED                 0x00000002
#define HTTP_REQUEST_FLAG_HTTP2                     0x00000004

#define HTTP_SEND_RESPONSE_FLAG_DISCONNECT      0x00000001
#define HTTP_SEND_RESPONSE_FLAG_MORE_DATA       0x00000002
#define HTTP_SEND_RESPONSE_FLAG_BUFFER_DATA     0x00000004
#define HTTP_SEND_RESPONSE_FLAG_ENABLE_NAGLING  0x00000008
#define HTTP_SEND_RESPONSE_FLAG_PROCESS_RANGES  0x00000020
#define HTTP_SEND_RESPONSE_FLAG_OPAQUE          0x00000040

#define HTTP_URL_FLAG_REMOVE_ALL    0x0000001

typedef enum _HTTP_SERVICE_CONFIG_ID
{
    HttpServiceConfigIPListenList,
    HttpServiceConfigSSLCertInfo,
    HttpServiceConfigUrlAclInfo,
    HttpServiceConfigTimeout,
    HttpServiceConfigMax
} HTTP_SERVICE_CONFIG_ID, *PHTTP_SERVICE_CONFIG_ID;

#define HTTP_NULL_ID ((ULONGLONG)0)

typedef ULONGLONG HTTP_OPAQUE_ID, *PHTTP_OPAQUE_ID;
typedef HTTP_OPAQUE_ID HTTP_CONNECTION_ID, *PHTTP_CONNECTION_ID;
typedef HTTP_OPAQUE_ID HTTP_RAW_CONNECTION_ID, *PHTTP_RAW_CONNECTION_ID;
typedef HTTP_OPAQUE_ID HTTP_REQUEST_ID, *PHTTP_REQUEST_ID;
typedef HTTP_OPAQUE_ID HTTP_SERVER_SESSION_ID, *PHTTP_SERVER_SESSION_ID;
typedef HTTP_OPAQUE_ID HTTP_URL_GROUP_ID, *PHTTP_URL_GROUP_ID;
typedef ULONGLONG HTTP_URL_CONTEXT;

typedef struct _HTTP_VERSION
{
    USHORT MajorVersion;
    USHORT MinorVersion;
} HTTP_VERSION, *PHTTP_VERSION;

typedef enum _HTTP_VERB
{
    HttpVerbUnparsed = 0,
    HttpVerbUnknown,
    HttpVerbInvalid,
    HttpVerbOPTIONS,
    HttpVerbGET,
    HttpVerbHEAD,
    HttpVerbPOST,
    HttpVerbPUT,
    HttpVerbDELETE,
    HttpVerbTRACE,
    HttpVerbCONNECT,
    HttpVerbTRACK,
    HttpVerbMOVE,
    HttpVerbCOPY,
    HttpVerbPROPFIND,
    HttpVerbPROPPATCH,
    HttpVerbMKCOL,
    HttpVerbLOCK,
    HttpVerbUNLOCK,
    HttpVerbSEARCH,
    HttpVerbMaximum,
} HTTP_VERB, *PHTTP_VERB;

typedef struct _HTTP_COOKED_URL
{
    USHORT FullUrlLength;
    USHORT HostLength;
    USHORT AbsPathLength;
    USHORT QueryStringLength;
    const WCHAR *pFullUrl;
    const WCHAR *pHost;
    const WCHAR *pAbsPath;
    const WCHAR *pQueryString;
} HTTP_COOKED_URL, *PHTTP_COOKED_URL;

typedef struct _HTTP_TRANSPORT_ADDRESS
{
    SOCKADDR *pRemoteAddress;
    SOCKADDR *pLocalAddress;
} HTTP_TRANSPORT_ADDRESS, *PHTTP_TRANSPORT_ADDRESS;

typedef struct _HTTP_UNKNOWN_HEADER
{
    USHORT NameLength;
    USHORT RawValueLength;
    const char *pName;
    const char *pRawValue;
} HTTP_UNKNOWN_HEADER, *PHTTP_UNKNOWN_HEADER;

typedef struct _HTTP_KNOWN_HEADER
{
    USHORT RawValueLength;
    const char *pRawValue;
} HTTP_KNOWN_HEADER, *PHTTP_KNOWN_HEADER;

typedef enum _HTTP_HEADER_ID
{
    HttpHeaderCacheControl = 0,
    HttpHeaderConnection = 1,
    HttpHeaderDate = 2,
    HttpHeaderKeepAlive = 3,
    HttpHeaderPragma = 4,
    HttpHeaderTrailer = 5,
    HttpHeaderTransferEncoding = 6,
    HttpHeaderUpgrade = 7,
    HttpHeaderVia = 8,
    HttpHeaderWarning = 9,
    HttpHeaderAllow = 10,
    HttpHeaderContentLength = 11,
    HttpHeaderContentType = 12,
    HttpHeaderContentEncoding = 13,
    HttpHeaderContentLanguage = 14,
    HttpHeaderContentLocation = 15,
    HttpHeaderContentMd5 = 16,
    HttpHeaderContentRange = 17,
    HttpHeaderExpires = 18,
    HttpHeaderLastModified = 19,

    HttpHeaderAccept = 20,
    HttpHeaderAcceptCharset = 21,
    HttpHeaderAcceptEncoding = 22,
    HttpHeaderAcceptLanguage = 23,
    HttpHeaderAuthorization = 24,
    HttpHeaderCookie = 25,
    HttpHeaderExpect = 26,
    HttpHeaderFrom = 27,
    HttpHeaderHost = 28,
    HttpHeaderIfMatch = 29,
    HttpHeaderIfModifiedSince = 30,
    HttpHeaderIfNoneMatch = 31,
    HttpHeaderIfRange = 32,
    HttpHeaderIfUnmodifiedSince = 33,
    HttpHeaderMaxForwards = 34,
    HttpHeaderProxyAuthorization = 35,
    HttpHeaderReferer = 36,
    HttpHeaderRange = 37,
    HttpHeaderTe = 38,
    HttpHeaderTranslate = 39,
    HttpHeaderUserAgent = 40,
    HttpHeaderRequestMaximum = 41,

    HttpHeaderAcceptRanges = 20,
    HttpHeaderAge = 21,
    HttpHeaderEtag = 22,
    HttpHeaderLocation = 23,
    HttpHeaderProxyAuthenticate = 24,
    HttpHeaderRetryAfter = 25,
    HttpHeaderServer = 26,
    HttpHeaderSetCookie = 27,
    HttpHeaderVary = 28,
    HttpHeaderWwwAuthenticate = 29,
    HttpHeaderResponseMaximum = 30,

    HttpHeaderMaximum = 41,
} HTTP_HEADER_ID, *PHTTP_HEADER_ID;

typedef struct _HTTP_REQUEST_HEADERS
{
    USHORT UnknownHeaderCount;
    HTTP_UNKNOWN_HEADER *pUnknownHeaders;
    USHORT TrailerCount;
    HTTP_UNKNOWN_HEADER *pTrailers;
    HTTP_KNOWN_HEADER KnownHeaders[HttpHeaderRequestMaximum];
} HTTP_REQUEST_HEADERS, *PHTTP_REQUEST_HEADERS;

typedef enum _HTTP_DATA_CHUNK_TYPE
{
    HttpDataChunkFromMemory = 0,
    HttpDataChunkFromFileHandle,
    HttpDataChunkFromFragmentCache,
    HttpDataChunkFromFragmentCacheEx,
    HttpDataChunkMaximum,
} HTTP_DATA_CHUNK_TYPE, *PHTTP_DATA_CHUNK_TYPE;

#define HTTP_BYTE_RANGE_TO_EOF ((ULONGLONG)-1)

typedef struct _HTTP_BYTE_RANGE
{
    ULARGE_INTEGER StartingOffset;
    ULARGE_INTEGER Length;
} HTTP_BYTE_RANGE, *PHTTP_BYTE_RANGE;

typedef struct _HTTP_DATA_CHUNK
{
    HTTP_DATA_CHUNK_TYPE DataChunkType;
    __C89_NAMELESS union
    {
        struct
        {
            void *pBuffer;
            ULONG BufferLength;
        } FromMemory;
        struct
        {
            HTTP_BYTE_RANGE ByteRange;
            HANDLE FileHandle;
        } FromFileHandle;
        struct
        {
            USHORT FragmentNameLength;
            const WCHAR *pFragmentName;
        } FromFragmentCache;
    } DUMMYUNIONNAME;
} HTTP_DATA_CHUNK, *PHTTP_DATA_CHUNK;

typedef struct _HTTP_SSL_CLIENT_CERT_INFO
{
    ULONG CertFlags;
    ULONG CertEncodedSize;
    UCHAR *pCertEncoded;
    HANDLE Token;
    BOOLEAN CertDeniedByMapper;
} HTTP_SSL_CLIENT_CERT_INFO, *PHTTP_SSL_CLIENT_CERT_INFO;

typedef struct _HTTP_SSL_INFO
{
    USHORT ServerCertKeySize;
    USHORT ConnectionKeySize;
    ULONG ServerCertIssuerSize;
    ULONG ServerCertSubjectSize;
    const char *pServerCertIssuer;
    const char *pServerCertSubject;
    HTTP_SSL_CLIENT_CERT_INFO *pClientCertInfo;
    ULONG SslClientCertNegotiated;
} HTTP_SSL_INFO, *PHTTP_SSL_INFO;

typedef struct _HTTP_REQUEST_V1
{
    ULONG Flags;
    HTTP_CONNECTION_ID ConnectionId;
    HTTP_REQUEST_ID RequestId;
    HTTP_URL_CONTEXT UrlContext;
    HTTP_VERSION Version;
    HTTP_VERB Verb;
    USHORT UnknownVerbLength;
    USHORT RawUrlLength;
    const char *pUnknownVerb;
    const char *pRawUrl;
    HTTP_COOKED_URL CookedUrl;
    HTTP_TRANSPORT_ADDRESS Address;
    HTTP_REQUEST_HEADERS Headers;
    ULONGLONG BytesReceived;
    USHORT EntityChunkCount;
    HTTP_DATA_CHUNK *pEntityChunks;
    HTTP_RAW_CONNECTION_ID RawConnectionId;
    HTTP_SSL_INFO *pSslInfo;
} HTTP_REQUEST_V1;

typedef enum _HTTP_REQUEST_INFO_TYPE
{
    HttpRequestInfoTypeAuth = 0,
} HTTP_REQUEST_INFO_TYPE, *PHTTP_REQUEST_INFO_TYPE;

typedef struct _HTTP_REQUEST_INFO
{
    HTTP_REQUEST_INFO_TYPE InfoType;
    ULONG InfoLength;
    void *pInfo;
} HTTP_REQUEST_INFO, *PHTTP_REQUEST_INFO;

#ifdef __cplusplus
typedef struct _HTTP_REQUEST_V2 : HTTP_REQUEST_V1
{
    USHORT RequestInfoCount;
    HTTP_REQUEST_INFO *pRequestInfo;
} HTTP_REQUEST_V2, *PHTTP_REQUEST_V2;
#else
typedef struct _HTTP_REQUEST_V2
{
    HTTP_REQUEST_V1 s;
    USHORT RequestInfoCount;
    HTTP_REQUEST_INFO *pRequestInfo;
} HTTP_REQUEST_V2, *PHTTP_REQUEST_V2;
#endif

typedef HTTP_REQUEST_V2 HTTP_REQUEST, *PHTTP_REQUEST;

typedef struct _HTTP_RESPONSE_HEADERS
{
    USHORT UnknownHeaderCount;
    HTTP_UNKNOWN_HEADER *pUnknownHeaders;
    USHORT TrailerCount;
    HTTP_UNKNOWN_HEADER *pTrailers;
    HTTP_KNOWN_HEADER KnownHeaders[HttpHeaderResponseMaximum];
} HTTP_RESPONSE_HEADERS,*PHTTP_RESPONSE_HEADERS;

typedef struct _HTTP_RESPONSE_V1
{
    ULONG Flags;
    HTTP_VERSION Version;
    USHORT StatusCode;
    USHORT ReasonLength;
    const char *pReason;
    HTTP_RESPONSE_HEADERS Headers;
    USHORT EntityChunkCount;
    HTTP_DATA_CHUNK *pEntityChunks;
} HTTP_RESPONSE_V1, *PHTTP_RESPONSE_V1;

typedef enum _HTTP_RESPONSE_INFO_TYPE
{
    HttpResponseInfoTypeMultipleKnownHeaders = 0,
    HttpResponseInfoTypeAuthenticationProperty,
    HttpResponseInfoTypeQosProperty,
    HttpResponseInfoTypeChannelBind,
} HTTP_RESPONSE_INFO_TYPE, *PHTTP_RESPONSE_INFO_TYPE;

typedef struct _HTTP_RESPONSE_INFO
{
    HTTP_RESPONSE_INFO_TYPE Type;
    ULONG Length;
    void *pInfo;
} HTTP_RESPONSE_INFO, *PHTTP_RESPONSE_INFO;

#ifdef __cplusplus
typedef struct _HTTP_RESPONSE_V2 : HTTP_RESPONSE_V1
{
    USHORT ResponseInfoCount;
    HTTP_RESPONSE_INFO *pResponseInfo;
} HTTP_RESPONSE_V2, *PHTTP_RESPONSE_V2;
#else
typedef struct _HTTP_RESPONSE_V2
{
    HTTP_RESPONSE_V1 s;
    USHORT ResponseInfoCount;
    HTTP_RESPONSE_INFO *pResponseInfo;
} HTTP_RESPONSE_V2, *PHTTP_RESPONSE_V2;
#endif

typedef HTTP_RESPONSE_V2 HTTP_RESPONSE, *PHTTP_RESPONSE;

typedef enum _HTTP_CACHE_POLICY_TYPE
{
    HttpCachePolicyNocache,
    HttpCachePolicyUserInvalidates,
    HttpCachePolicyTimeToLive,
    HttpCachePolicyMaximum,
} HTTP_CACHE_POLICY_TYPE, *PHTTP_CACHE_POLICY_TYPE;

typedef struct _HTTP_CACHE_POLICY
{
    HTTP_CACHE_POLICY_TYPE Policy;
    ULONG SecondsToLive;
} HTTP_CACHE_POLICY, *PHTTP_CACHE_POLICY;

typedef enum _HTTP_LOG_DATA_TYPE
{
    HttpLogDataTypeFields = 0,
} HTTP_LOG_DATA_TYPE, *PHTTP_LOG_DATA_TYPE;

typedef struct _HTTP_LOG_DATA
{
    HTTP_LOG_DATA_TYPE Type;
} HTTP_LOG_DATA, *PHTTP_LOG_DATA;

typedef enum _HTTP_SERVER_PROPERTY
{
    HttpServerAuthenticationProperty,
    HttpServerLoggingProperty,
    HttpServerQosProperty,
    HttpServerTimeoutsProperty,
    HttpServerQueueLengthProperty,
    HttpServerStateProperty,
    HttpServer503VerbosityProperty,
    HttpServerBindingProperty,
    HttpServerExtendedAuthenticationProperty,
    HttpServerListenEndpointProperty,
    HttpServerChannelBindProperty,
    HttpServerProtectionLevelProperty,
} HTTP_SERVER_PROPERTY, *PHTTP_SERVER_PROPERTY;

typedef struct _HTTP_PROPERTY_FLAGS
{
    ULONG Present : 1;
} HTTP_PROPERTY_FLAGS, *PHTTP_PROPERTY_FLAGS;

typedef struct _HTTP_BINDING_INFO
{
    HTTP_PROPERTY_FLAGS Flags;
    HANDLE RequestQueueHandle;
} HTTP_BINDING_INFO, *PHTTP_BINDING_INFO;

ULONG WINAPI HttpAddUrl(HANDLE,PCWSTR,PVOID);
ULONG WINAPI HttpAddUrlToUrlGroup(HTTP_URL_GROUP_ID id, const WCHAR *url, HTTP_URL_CONTEXT context, ULONG reserved);
ULONG WINAPI HttpCloseRequestQueue(HANDLE handle);
ULONG WINAPI HttpCloseServerSession(HTTP_SERVER_SESSION_ID id);
ULONG WINAPI HttpCloseUrlGroup(HTTP_URL_GROUP_ID id);
ULONG WINAPI HttpCreateHttpHandle(PHANDLE,ULONG);
ULONG WINAPI HttpCreateRequestQueue(HTTPAPI_VERSION version, const WCHAR *name, SECURITY_ATTRIBUTES *sa, ULONG flags, HANDLE *handle);
ULONG WINAPI HttpCreateServerSession(HTTPAPI_VERSION,PHTTP_SERVER_SESSION_ID,ULONG);
ULONG WINAPI HttpCreateUrlGroup(HTTP_SERVER_SESSION_ID session_id, HTTP_URL_GROUP_ID *group_id, ULONG reserved);
ULONG WINAPI HttpDeleteServiceConfiguration(HANDLE,HTTP_SERVICE_CONFIG_ID,PVOID,ULONG,LPOVERLAPPED);
ULONG WINAPI HttpInitialize(HTTPAPI_VERSION version, ULONG flags, void *reserved);
ULONG WINAPI HttpTerminate(ULONG flags, void *reserved);
ULONG WINAPI HttpQueryServiceConfiguration(HANDLE,HTTP_SERVICE_CONFIG_ID,PVOID,ULONG,PVOID,ULONG,PULONG,LPOVERLAPPED);
ULONG WINAPI HttpReceiveHttpRequest(HANDLE queue, HTTP_REQUEST_ID id, ULONG flags, HTTP_REQUEST *request, ULONG size, ULONG *ret_size, OVERLAPPED *ovl);
ULONG WINAPI HttpRemoveUrl(HANDLE queue, const WCHAR *url);
ULONG WINAPI HttpRemoveUrlFromUrlGroup(HTTP_URL_GROUP_ID id, const WCHAR *url, ULONG flags);
ULONG WINAPI HttpSendHttpResponse(HANDLE queue, HTTP_REQUEST_ID id, ULONG flags, HTTP_RESPONSE *response, HTTP_CACHE_POLICY *cache_policy, ULONG *ret_size, void *reserved1, ULONG reserved2, OVERLAPPED *ovl, HTTP_LOG_DATA *log_data);
ULONG WINAPI HttpSetServiceConfiguration(HANDLE,HTTP_SERVICE_CONFIG_ID,PVOID,ULONG,LPOVERLAPPED);
ULONG WINAPI HttpSetUrlGroupProperty(HTTP_URL_GROUP_ID id, HTTP_SERVER_PROPERTY property, void *value, ULONG length);

#ifdef __cplusplus
}
#endif

#endif  /* __WINE_HTTP_H */
