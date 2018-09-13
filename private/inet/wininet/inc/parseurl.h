/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    parseurl.h

Abstract:

    Header for parseurl.c and protocol-specific parsers

Author:

    Richard L Firth (rfirth) 26-Apr-1995

Revision History:

    26-Apr-1995
        Created

--*/

#if defined(__cplusplus)
extern "C" {
#endif

//
// manifests
//

#define SAFE                0x00    // character is safe in all schemes
#define SCHEME_FTP          0x01    // character is unsafe in FTP scheme
#define SCHEME_GOPHER       0x02    // character is unsafe in gopher scheme
#define SCHEME_HTTP         0x04    // character is unsafe in HTTP scheme
#define UNSAFE              0x80    // character is unsafe in all schemes

#define SCHEME_ANY          (SCHEME_FTP | SCHEME_GOPHER | SCHEME_HTTP)

//
// types
//

typedef
DWORD
(*LPFN_URL_PARSER)(
    LPHINTERNET,
    LPSTR,
    DWORD,
    LPSTR,
    DWORD,
    DWORD,
    DWORD_PTR
    );

//
// prototypes
//

BOOL
IsValidUrl(
    IN LPCSTR lpszUrl
    );

BOOL
DoesSchemeRequireSlashes(
    IN LPSTR lpszScheme,
    IN DWORD dwSchemeLength,
    IN BOOL bHasHostName
    );

DWORD
ParseUrl(
    IN OUT LPHINTERNET hInternet,
    IN LPVOID hMapped,
    IN LPCSTR Url,
    IN LPCSTR Headers,
    IN DWORD HeadersLength,
    IN DWORD OpenFlags,
    IN DWORD_PTR Context
    );

DWORD
CrackUrl(
    IN OUT LPSTR lpszUrl,
    IN DWORD dwUrlLength,
    IN BOOL bEscape,
    OUT LPINTERNET_SCHEME lpSchemeType OPTIONAL,
    OUT LPSTR* lpszSchemeName OPTIONAL,
    OUT LPDWORD lpdwSchemeNameLength OPTIONAL,
    OUT LPSTR* lpszHostName OPTIONAL,
    OUT LPDWORD lpdwHostNameLength OPTIONAL,
    OUT LPINTERNET_PORT lpServerPort OPTIONAL,
    OUT LPSTR* lpszUserName OPTIONAL,
    OUT LPDWORD lpdwUserNameLength OPTIONAL,
    OUT LPSTR* lpszPassword OPTIONAL,
    OUT LPDWORD lpdwPasswordLength OPTIONAL,
    OUT LPSTR* UrlPath OPTIONAL,
    OUT LPDWORD lpdwUrlPathLength OPTIONAL,
    OUT LPSTR* lpszExtraInfo OPTIONAL,
    OUT LPDWORD lpdwExtraInfoLength OPTIONAL,
    OUT LPBOOL pHavePort
    );

DWORD
EncodeUrlPath(
    IN DWORD Flags,
    IN DWORD SchemeFlags,
    IN LPSTR UrlPath,
    IN DWORD UrlPathLength,
    OUT LPSTR EncodedUrlPath,
    IN OUT LPDWORD EncodedUrlPathLength
    );

//
// flags for EncodeUrlPath
//

#define NO_ENCODE_PATH_SEP  0x00000001

DWORD
DecodeUrl(
    IN LPSTR Url,
    IN DWORD UrlLength,
    OUT LPSTR UnescapedString,
    IN OUT LPDWORD UnescapedLength
    );

DWORD
DecodeUrlInSitu(
    IN LPSTR BufferAddress,
    IN OUT LPDWORD BufferLength
    );

DWORD
DecodeUrlStringInSitu(
    IN LPSTR BufferAddress,
    IN OUT LPDWORD BufferLength
    );

DWORD
GetUrlAddressInfo(
    IN OUT LPSTR* Url,
    IN OUT LPDWORD UrlLength,
    OUT LPSTR* PartOne,
    OUT LPDWORD PartOneLength,
    OUT LPBOOL PartOneEscape,
    OUT LPSTR* PartTwo,
    OUT LPDWORD PartTwoLength,
    OUT LPBOOL PartTwoEscape
    );

DWORD
GetUrlAddress(
    IN OUT LPSTR* lpszUrl,
    OUT LPDWORD lpdwUrlLength,
    OUT LPSTR* lpszUserName OPTIONAL,
    OUT LPDWORD lpdwUserNameLength OPTIONAL,
    OUT LPSTR* lpszPassword OPTIONAL,
    OUT LPDWORD lpdwPasswordLength OPTIONAL,
    OUT LPSTR* lpszHostName OPTIONAL,
    OUT LPDWORD lpdwHostNameLength OPTIONAL,
    OUT INTERNET_PORT* lpPort OPTIONAL,
    OUT LPBOOL pHavePort
    );

INTERNET_SCHEME
MapUrlSchemeName(
    IN LPSTR lpszSchemeName,
    IN DWORD dwSchemeNameLength
    );

LPSTR
MapUrlScheme(
    IN INTERNET_SCHEME Scheme,
    OUT LPDWORD lpdwSchemeNameLength
    );

LPSTR
MapUrlSchemeToName(
    IN INTERNET_SCHEME Scheme
    );

//
// protocol-specific URL parsers
//

DWORD
ParseFtpUrl(
    IN OUT LPHINTERNET hInternet,
    IN LPSTR Url,
    IN DWORD SchemeLength,
    IN LPSTR Headers,
    IN DWORD HeadersLength,
    IN DWORD OpenFlags,
    IN DWORD_PTR Context
    );

DWORD
ParseGopherUrl(
    IN OUT LPHINTERNET hInternet,
    IN LPSTR Url,
    IN DWORD SchemeLength,
    IN LPSTR Headers,
    IN DWORD HeadersLength,
    IN DWORD OpenFlags,
    IN DWORD_PTR Context
    );

DWORD
GopherLocatorToUrl(
    IN LPSTR Locator,
    OUT LPSTR Buffer,
    IN DWORD BufferLength,
    OUT LPDWORD UrlLength
    );

DWORD
ParseHttpUrl(
    IN OUT LPHINTERNET hInternet,
    IN LPSTR Url,
    IN DWORD SchemeLength,
    IN LPSTR Headers,
    IN DWORD HeadersLength,
    IN DWORD OpenFlags,
    IN DWORD_PTR Context
    );

#if defined(__cplusplus)
}
#endif
