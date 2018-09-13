/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    proxysup.cxx

Abstract:

    Contains class implementation for proxy server and proxy bypass list

    Contents:
        IsLocalMacro

        PROXY_SERVER_LIST_ENTRY::WriteEntry

        PROXY_SERVER_LIST::AddList
        PROXY_SERVER_LIST::Find
        PROXY_SERVER_LIST::Add
        PROXY_SERVER_LIST::ProxyScheme
        PROXY_SERVER_LIST::GetProxyHostName
        PROXY_SERVER_LIST::AddToBypassList
        PROXY_SERVER_LIST::GetList

        PROXY_BYPASS_LIST_ENTRY::WriteEntry

        PROXY_BYPASS_LIST::AddList
        PROXY_BYPASS_LIST::Find
        PROXY_BYPASS_LIST::Add
        PROXY_BYPASS_LIST::IsBypassed
        PROXY_BYPASS_LIST::IsHostInBypassList
        PROXY_BYPASS_LIST::GetList

        PROXY_INFO::GetProxyStringInfo
        PROXY_INFO::HostBypassesProxy
        PROXY_INFO::RedoSendRequest
        PROXY_INFO::Terminate
        PROXY_INFO::CleanOutLists

        PROXY_STATE::GetNextProxy

        (GetRegistryProxyParameter)

Author:

    Richard L Firth (rfirth) 03-Feb-1996

Revision History:

    03-Feb-1996 rfirth
        Created

--*/

#include <wininetp.h>

//
// private manifests
//

#define DEFAULT_PROXY_BUFFER_LENGTH     (4 K)
#define MAX_IP_ADDRESS_STRING_LENGTH    (4 * 4 - 1) // ###.###.###.###
#define PROXY_REGISTRY_STRING_LENGTH    (4 K)

//
// private types
//

typedef enum {
    STATE_START,
    STATE_PROTOCOL,
    STATE_SCHEME,
    STATE_SERVER,
    STATE_PORT,
    STATE_END,
    STATE_ERROR
} PARSER_STATE;


//
// private prototypes
//

PRIVATE
LPSTR
GetRegistryProxyParameter(
    IN LPSTR lpszParameterName
    );





//
// functions
//


BOOL
IsLocalMacro(
    IN LPSTR lpszMetaName,
    IN DWORD dwMetaNameLength
    )

/*++

Routine Description:

    Checks for local macro name

Arguments:

    lpszMetaName        - name to check

    dwMetaNameLength    - length

Return Value:

    BOOL
        TRUE    - it is <local>

        FALSE   - not

--*/

{
    INET_ASSERT(lpszMetaName != NULL);

    static const char s_local[] = "<local>";

    return (strnicmp(s_local, lpszMetaName, dwMetaNameLength) == 0);
}


//PRIVATE
//BOOL
//IsNetscapeProxyListString(
//    IN LPSTR lpszProxyStr,
//    IN DWORD dwcbProxyStr
//    )
//
///*++
//
//Routine Description:
//
//    Determines if a string is part of a Netscape style multiple proxy list.
//
//Arguments:
//
//    lpszProxyStr - The string to examine
//
//    dwcbProxyStr - Size of the string to check.
//
//Return Value:
//
//    BOOL
//        TRUE    - if its a Netscape list of proxies.
//
//        FALSE   - its just a proxy name.
//
//--*/
//
//{
//    INET_ASSERT(lpszProxyStr);
//    INET_ASSERT(dwcbProxyStr > 0);
//
//    SKIPWS(lpszProxyStr);
//
//    //
//    // If it SOCKS, PROXY, or DIRECT (and) there is some delimiter
//    //  we accept it as a Netscape Proxy String.
//    //
//
//    if ( strncmp( lpszProxyStr, "DIRECT", min(dwcbProxyStr, sizeof("DIRECT")-1) ) == 0  )
//    {
//        return TRUE;
//    }
//
//    if ( strncmp( lpszProxyStr, "PROXY",  min(dwcbProxyStr, sizeof("PROXY")-1) )  == 0  ||
//         strncmp( lpszProxyStr, "SOCKS",  min(dwcbProxyStr, sizeof("SOCKS")-1) )  == 0   )
//    {
//        for ( DWORD i = 0; i < dwcbProxyStr; i++ )
//        {
//            if ( lpszProxyStr[i] == ':' || lpszProxyStr[i] == ';' )
//            {
//                return TRUE;
//            }
//        }
//    }
//
//    return FALSE;
//}


//
// member functions
//


BOOL
PROXY_SERVER_LIST_ENTRY::WriteEntry(
    OUT LPSTR lpszBuffer,
    IN OUT LPDWORD lpdwBufferLength
    )

/*++

Routine Description:

    Writes this proxy server list entry as a string in the supplied buffer

Arguments:

    lpszBuffer          - pointer to buffer where string is written

    lpdwBufferLength    - IN: amount of space in buffer
                          OUT: number of bytes copied, or required size

Return Value:

    BOOL
        TRUE    - entry written to buffer

        FALSE   - entry not written to buffer - *lpdwBufferLength contains
                  required size

--*/

{
    DWORD requiredLength;
    LPSTR protocolName;
    DWORD protocolNameLength;
    LPSTR schemeName;
    DWORD schemeNameLength;
    INTERNET_PORT magnitude;

    protocolName = MapUrlScheme(_Protocol, &protocolNameLength);
    if (protocolName != NULL) {
        requiredLength = protocolNameLength + 1;    // for '='
    } else {
        requiredLength = 0;
    }
    schemeName = MapUrlScheme(_Scheme, &schemeNameLength);
    if (schemeName != NULL) {
        requiredLength += schemeNameLength + sizeof("://") - 1;
    }
    requiredLength += _ProxyName.StringLength();
    if (_ProxyPort != INTERNET_INVALID_PORT_NUMBER) {
        for (INTERNET_PORT n = 10000, i = 5; n > 0; n /= 10, --i) {
            if (_ProxyPort / n) {
                requiredLength += i + 1;    // for ':'
                magnitude = n;
                break;
            }
        }
    }

    BOOL success;

    if (*lpdwBufferLength > requiredLength) {
        if (protocolName != NULL) {
            memcpy(lpszBuffer, protocolName, protocolNameLength);
            lpszBuffer += protocolNameLength;
            *lpszBuffer++ = '=';
        }
        if (schemeName != NULL) {
            memcpy(lpszBuffer, schemeName, schemeNameLength);
            lpszBuffer += schemeNameLength;
            memcpy(lpszBuffer, "://", sizeof("://") - 1);
            lpszBuffer += sizeof("://") - 1;
        }
        _ProxyName.CopyTo(lpszBuffer);
        lpszBuffer += _ProxyName.StringLength();
        if (_ProxyPort != INTERNET_INVALID_PORT_NUMBER) {
            *lpszBuffer++ = ':';
            for (INTERNET_PORT n = _ProxyPort, i = magnitude; i; i /= 10) {
                *lpszBuffer++ = (char)(n / i) + '0';
                n %= i;
            }
        }
        success = TRUE;
    } else {
        success = FALSE;
    }
    *lpdwBufferLength = requiredLength;
    return success;
}


DWORD
PROXY_SERVER_LIST::AddList(
    IN LPSTR lpszList
    )

/*++

Routine Description:

    Parses a list of proxy servers and creates a PROXY_SERVER_LIST_ENTRY for
    each one

Arguments:

    lpszList    - pointer to list of proxies of the form:

                    [<scheme>=][<scheme>"://"]<server>[":"<port>][";"*]

                  The list can be NULL, in which case we read it from the
                  registry

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_PARAMETER
                    At least one entry in lpszList is bogus

--*/

{
    DEBUG_ENTER((DBG_PROXY,
                Dword,
                "PROXY_SERVER_LIST::AddList",
                "%.80q",
                lpszList
                ));

    DWORD entryLength;
    LPSTR protocolName;
    DWORD protocolLength;
    LPSTR schemeName;
    DWORD schemeLength;
    LPSTR serverName;
    DWORD serverLength;
    PARSER_STATE state;
    DWORD nSlashes;
    INTERNET_PORT port;
    BOOL done;

    entryLength = 0;
    protocolName = lpszList;
    protocolLength = 0;
    schemeName = NULL;
    schemeLength = 0;
    serverName = NULL;
    serverLength = 0;
    state = STATE_PROTOCOL;
    nSlashes = 0;
    port = 0;
    done = FALSE;

    //
    // walk the list, pulling out the various scheme parts
    //

    do {

        char ch = *lpszList++;

        if ((nSlashes == 1) && (ch != '/')) {
            state = STATE_ERROR;
            break;
        }

        switch (ch) {
        case '=':
            if ((state == STATE_PROTOCOL) && (entryLength != 0)) {
                protocolLength = entryLength;
                entryLength = 0;
                state = STATE_SCHEME;
                schemeName = lpszList;
            } else {

                //
                // '=' can't legally appear anywhere else
                //

                state = STATE_ERROR;
            }
            break;

        case ':':
            switch (state) {
            case STATE_PROTOCOL:
                if (*lpszList == '/') {
                    schemeName = protocolName;
                    protocolName = NULL;
                    schemeLength = entryLength;
                    protocolLength = 0;
                    state = STATE_SCHEME;
                } else if (*lpszList != '\0') {
                    serverName = protocolName;
                    serverLength = entryLength;
                    state = STATE_PORT;
                } else {
                    state = STATE_ERROR;
                }
                entryLength = 0;
                break;

            case STATE_SCHEME:
                if (*lpszList == '/') {
                    schemeLength = entryLength;
                } else if (*lpszList != '\0') {
                    serverName = schemeName;
                    serverLength = entryLength;
                    state = STATE_PORT;
                } else {
                    state = STATE_ERROR;
                }
                entryLength = 0;
                break;

            case STATE_SERVER:
                serverLength = entryLength;
                state = STATE_PORT;
                entryLength = 0;
                break;

            default:
                state = STATE_ERROR;
                break;
            }
            break;

        case '/':
            if ((state == STATE_SCHEME) && (nSlashes < 2) && (entryLength == 0)) {
                if (++nSlashes == 2) {
                    state = STATE_SERVER;
                    serverName = lpszList;
                }
            } else {
                state = STATE_ERROR;
            }
            break;

        default:
            if (state != STATE_PORT) {
                ++entryLength;
            } else if (isdigit(ch)) {

                //
                // BUGBUG - we will overflow if >65535
                //

                port = port * 10 + (ch - '0');
            } else {

                //
                // STATE_PORT && non-digit character - error
                //

                state = STATE_ERROR;
            }
            break;

        case '\0':
            done = TRUE;

            //
            // fall through
            //

        case '\t':
        case '\n':
        case '\v':  // vertical tab, 0x0b
        case '\f':  // form feed, 0x0c
        case '\r':
        case ' ':
        case ';':
        case ',':
            if (serverLength == 0) {
                serverLength = entryLength;
            }
            if (serverLength != 0) {
                if (serverName == NULL) {
                    serverName = (schemeName != NULL)
                        ? schemeName
                        : protocolName;
                }

                INET_ASSERT(serverName != NULL);

                INTERNET_SCHEME protocol;

                if (protocolLength != 0) {
                    protocol = MapUrlSchemeName(protocolName, protocolLength);
                } else {
                    protocol = INTERNET_SCHEME_DEFAULT;
                }

                INTERNET_SCHEME scheme;

                if (schemeLength != 0) {
                    scheme = MapUrlSchemeName(schemeName, schemeLength);
                } else {
                    scheme = INTERNET_SCHEME_DEFAULT;
                }

                //
                // add an entry if this is a protocol we handle and we don't
                // already have an entry for it
                //

                if ((protocol != INTERNET_SCHEME_UNKNOWN)
                && (scheme != INTERNET_SCHEME_UNKNOWN)

                //
                // we can only currently handle CERN (secure or unsecure) and
                // FTP proxies, so kick out anything that wants to go via any
                // other proxy scheme
                //

                && ((scheme == INTERNET_SCHEME_DEFAULT)
                || (scheme == INTERNET_SCHEME_FTP)
                || (scheme == INTERNET_SCHEME_HTTP)
                || (scheme == INTERNET_SCHEME_HTTPS))) {
                    if (!Find(protocol)) {

                        //
                        // don't worry if Add() fails - we just continue
                        //

                        Add(protocol, scheme, serverName, serverLength, port);
                    }
                }
            }
            entryLength = 0;
            protocolName = lpszList;
            protocolLength = 0;
            schemeName = NULL;
            schemeLength = 0;
            serverName = NULL;
            serverLength = 0;
            nSlashes = 0;
            port = 0;
            state = STATE_PROTOCOL;
            break;
        }
        if (state == STATE_ERROR) {
            break;
        }
    } while (!done);

    DWORD error;

    if (state == STATE_ERROR) {
        error = ERROR_INVALID_PARAMETER;
    } else {
        error = ERROR_SUCCESS;
    }

    DEBUG_LEAVE(error);

    return error;
}


BOOL
PROXY_SERVER_LIST::Find(
    IN INTERNET_SCHEME tScheme
    )

/*++

Routine Description:

    Find a PROXY_SERVER_LIST_ENTRY based on the scheme

Arguments:

    tScheme - protocol scheme to find

Return Value:

    BOOL

--*/

{
    DEBUG_ENTER((DBG_PROXY,
                Bool,
                "PROXY_SERVER_LIST::Find",
                "%s",
                InternetMapScheme(tScheme)
                ));

    BOOL found = FALSE;

    LockSerializedList(&_List);

    for (PLIST_ENTRY entry = HeadOfSerializedList(&_List);
        entry != (PLIST_ENTRY)SlSelf(&_List);
        entry = entry->Flink) {

        PROXY_SERVER_LIST_ENTRY * info;

        info = CONTAINING_RECORD(entry, PROXY_SERVER_LIST_ENTRY, _List);

        if (info->_Protocol == tScheme) {
            found = TRUE;
            break;
        }
    }

    UnlockSerializedList(&_List);

    DEBUG_LEAVE(found);

    return found;
}


DWORD
PROXY_SERVER_LIST::Add(
    IN INTERNET_SCHEME tProtocol,
    IN INTERNET_SCHEME tScheme,
    IN LPSTR lpszHostName,
    IN DWORD dwHostNameLength,
    IN INTERNET_PORT nPort
    )

/*++

Routine Description:

    Create an add a PROXY_SERVER_LIST_ENTRY to the PROXY_SERVER_LIST

Arguments:

    tProtocol           - protocol which uses the proxy

    tScheme             - scheme used to talk to the proxy

    lpszHostName        - proxy host name

    dwHostNameLength    - length of proxy host name

    nPort               - port at proxy host

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DEBUG_ENTER((DBG_PROXY,
                Dword,
                "PROXY_SERVER_LIST::Add",
                "%s, %s, %.*q, %d, %d",
                InternetMapScheme(tProtocol),
                InternetMapScheme(tScheme),
                dwHostNameLength,
                lpszHostName,
                dwHostNameLength,
                nPort
                ));

    PROXY_SERVER_LIST_ENTRY * entry;

    entry = new PROXY_SERVER_LIST_ENTRY(tProtocol,
                                        tScheme,
                                        lpszHostName,
                                        dwHostNameLength,
                                        nPort
                                        );

    DWORD error;

    if (entry != NULL) {
        //error = entry->ResolveAddress();
        //if (error == ERROR_SUCCESS) {
        //    InsertAtTailOfSerializedList(&_List, &entry->_List);
        //}
        if (entry->_Protocol == INTERNET_SCHEME_DEFAULT) {
            InsertAtTailOfSerializedList(&_List, &entry->_List);
        } else {
            InsertAtHeadOfSerializedList(&_List, &entry->_List);
        }
        error = ERROR_SUCCESS;
    } else {
        error = ERROR_NOT_ENOUGH_MEMORY;
    }

    DEBUG_LEAVE(error);

    return error;
}


INTERNET_SCHEME
PROXY_SERVER_LIST::ProxyScheme(
    IN INTERNET_SCHEME tProtocol
    )

/*++

Routine Description:

    Determines protocol over which tScheme goes through proxy

Arguments:

    tProtocol   - protocol scheme used to retrieve data (e.g. FTP)

Return Value:

    INTERNET_SCHEME
        Success - scheme by which protocol goes via proxy

        Failure - INTERNET_SCHEME_UNKNOWN
--*/

{
    DEBUG_ENTER((DBG_PROXY,
                Int,
                "PROXY_SERVER_LIST::ProxyScheme",
                "%s",
                InternetMapScheme(tProtocol)
                ));

    INTERNET_SCHEME tScheme = INTERNET_SCHEME_UNKNOWN;

    LockSerializedList(&_List);

    //
    // the list really shouldn't be empty if we're here
    //

    INET_ASSERT(!IsSerializedListEmpty(&_List));

    for (PLIST_ENTRY entry = HeadOfSerializedList(&_List);
        entry != (PLIST_ENTRY)SlSelf(&_List);
        entry = entry->Flink) {

        PROXY_SERVER_LIST_ENTRY * info;

        info = CONTAINING_RECORD(entry, PROXY_SERVER_LIST_ENTRY, _List);

        //
        // if we find a match for the protocol, or this protocol is handled by
        // the default proxy entry then we are done
        //

        if ((info->_Protocol == tProtocol)
        || (info->_Protocol == INTERNET_SCHEME_DEFAULT)) {
            tScheme = info->_Scheme;

            //
            // the default scheme is HTTP (CERN proxy)
            //

            if (tScheme == INTERNET_SCHEME_DEFAULT) {
                tScheme = INTERNET_SCHEME_HTTP;
            }
            break;
        }
    }

    UnlockSerializedList(&_List);

    DEBUG_LEAVE(tScheme);

    return tScheme;
}


BOOL
PROXY_SERVER_LIST::GetProxyHostName(
    IN INTERNET_SCHEME tProtocol,
    IN OUT LPINTERNET_SCHEME lptScheme,
    OUT LPSTR * lplpszHostName,
    OUT LPBOOL lpbFreeHostName,
    OUT LPDWORD lpdwHostNameLength,
    OUT LPINTERNET_PORT lpHostPort
    )

/*++

Routine Description:

    Given a protocol, map it to the proxy we use to retrieve the data

Arguments:

    tProtocol           - protocol to map (e.g. find the proxy for FTP)

    lptScheme           - IN: preferred scheme if INTERNET_SCHEME_DEFAULT
                          OUT: returned scheme

    lplpszHostName      - pointer to returned pointer to host name

    lpbFreeHostName     - returned TRUE if *lplpszHostName allocated

    lpdwHostNameLength  - pointer to returned host name length

    lpHostPort          - pointer to returned host port

Return Value:

    BOOL
        TRUE    - requested info has been returned

        FALSE   - requested info was not found

--*/

{
    DEBUG_ENTER((DBG_PROXY,
                 Bool,
                 "PROXY_SERVER_LIST::GetProxyHostName",
                 "%s, %#x, %#x, %#x, %#x, %#x",
                 InternetMapScheme(tProtocol),
                 lptScheme,
                 lplpszHostName,
                 lpbFreeHostName,
                 lpdwHostNameLength,
                 lpHostPort
                 ));

    INET_ASSERT(tProtocol != INTERNET_SCHEME_UNKNOWN);

    //
    // *lptScheme must now be one of the recognized schemes, or the default
    //

    INET_ASSERT((*lptScheme == INTERNET_SCHEME_DEFAULT)
                || (*lptScheme == INTERNET_SCHEME_FTP)
                || (*lptScheme == INTERNET_SCHEME_GOPHER)
                || (*lptScheme == INTERNET_SCHEME_HTTP)
                || (*lptScheme == INTERNET_SCHEME_HTTPS)
                || (*lptScheme == INTERNET_SCHEME_SOCKS)
                );

    BOOL found = FALSE;

    LockSerializedList(&_List);

    //
    // the list really shouldn't be empty if we're here
    //

    INET_ASSERT(!IsSerializedListEmpty(&_List));

    for (PLIST_ENTRY entry = HeadOfSerializedList(&_List);
        entry != (PLIST_ENTRY)SlSelf(&_List);
        entry = entry->Flink) {

        PROXY_SERVER_LIST_ENTRY * info;

        info = CONTAINING_RECORD(entry, PROXY_SERVER_LIST_ENTRY, _List);

        //
        // if we find a match for the protocol, or this protocol is handled by
        // the default proxy entry then we are done
        //
        // Hack: But make sure its NOT socks since, socks must be
        //  an exact match !!! No defaults.
        //

        if ((info->_Protocol == tProtocol)
        || ((info->_Protocol == INTERNET_SCHEME_DEFAULT)
                && (tProtocol != INTERNET_SCHEME_SOCKS)  )) {

            INTERNET_SCHEME scheme = info->_Scheme;

            //
            // the returned scheme is the input preferred scheme unless it was
            // the default scheme in which case we return HTTP (CERN proxy)
            //

            if (scheme == INTERNET_SCHEME_DEFAULT) {
                scheme = (*lptScheme == INTERNET_SCHEME_DEFAULT)
                            ? INTERNET_SCHEME_HTTP
                            : *lptScheme;
            }
            *lptScheme = scheme;
            *lpbFreeHostName = FALSE;
            *lpdwHostNameLength = 0;
            *lplpszHostName = NewString(info->_ProxyName.StringAddress(),
                                        info->_ProxyName.StringLength()
                                        );
            if (*lplpszHostName != NULL) {
                *lpbFreeHostName = TRUE;
                *lpdwHostNameLength = info->_ProxyName.StringLength();
            }

            INTERNET_PORT port = info->_ProxyPort;

            //
            // map the default port value
            //

            if (port == INTERNET_INVALID_PORT_NUMBER) {
                switch (scheme) {
                case INTERNET_SCHEME_FTP:
                    port = INTERNET_DEFAULT_FTP_PORT;
                    break;

                case INTERNET_SCHEME_GOPHER:
                    port = INTERNET_DEFAULT_GOPHER_PORT;
                    break;

                case INTERNET_SCHEME_HTTP:
                    port = INTERNET_DEFAULT_HTTP_PORT;
                    break;

                case INTERNET_SCHEME_HTTPS:
                    port = INTERNET_DEFAULT_HTTPS_PORT;
                    break;

                case INTERNET_SCHEME_SOCKS:
                    port = INTERNET_DEFAULT_SOCKS_PORT;
                    break;
                }
            }
            *lpHostPort = port;
            found = TRUE;

            DEBUG_PRINT(PROXY,
                        INFO,
                        ("proxy = %s://%s:%d\n",
                        MapUrlSchemeToName(scheme),
                        info->_ProxyName.StringAddress(),
                        port
                        ));

            break;
        }
    }

    UnlockSerializedList(&_List);

    DEBUG_LEAVE(found);

    return found;
}


DWORD
PROXY_SERVER_LIST::AddToBypassList(
    IN PROXY_BYPASS_LIST * lpBypassList
    )

/*++

Routine Description:

    For all proxy servers in the server list, we add the details to the bypass
    list. By default, an app mustn't send a request to the proxy via the proxy!
    Additionally, the app should not have to specifically nominate the proxy
    server(s) as bypassing the proxy

Arguments:

    lpBypassList    - pointer to bypass proxy list where proxy servers will be
                      added

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DWORD error = ERROR_SUCCESS;
    PLIST_ENTRY entry = HeadOfSerializedList(&_List);

    while ((entry != (PLIST_ENTRY)SlSelf(&_List)) && (error == ERROR_SUCCESS)) {

        PROXY_SERVER_LIST_ENTRY * info = (PROXY_SERVER_LIST_ENTRY *)entry;

        if (!lpBypassList->Find(info->_Scheme,
                                info->_ProxyName.StringAddress(),
                                info->_ProxyName.StringLength(),
                                info->_ProxyPort)) {
            error = lpBypassList->Add(info->_Scheme,
                                      info->_ProxyName.StringAddress(),
                                      info->_ProxyName.StringLength(),
                                      info->_ProxyPort
                                      );
        }
        entry = entry->Flink;
    }
    return error;
}


VOID
PROXY_SERVER_LIST::GetList(
    OUT LPSTR * lplpszList,
    IN DWORD dwBufferLength,
    IN OUT LPDWORD lpdwRequiredLength
    )

/*++

Routine Description:

    Writes the list of proxy servers to a buffer, and/or returns the required
    buffer length

Arguments:

    lplpszList          - pointer to pointer to buffer where list is written, if
                          sufficient space

    dwBufferLength      - amount of space in *lplpszList

    lpdwRequiredLength  - OUT: cumulative size of data

Return Value:

    None.

--*/

{
    LPSTR lpszList = *lplpszList;
    BOOL firstTime = TRUE;
    BOOL outOfBuffer = FALSE;

    LockSerializedList(&_List);

    for (PLIST_ENTRY entry = HeadOfSerializedList(&_List);
        entry != (PLIST_ENTRY)SlSelf(&_List);
        entry = entry->Flink) {

        PROXY_SERVER_LIST_ENTRY * info;

        info = CONTAINING_RECORD(entry, PROXY_SERVER_LIST_ENTRY, _List);
        if (!firstTime) {

            //
            // write delimiter if enough space
            //

            if (dwBufferLength >= 1) {
                *lpszList++ = ' ';
                --dwBufferLength;
            }
            ++*lpdwRequiredLength;
        } else {
            firstTime = FALSE;
        }

        //
        // find the length of the current entry & write it to the buffer if
        // enough space
        //

        DWORD length = dwBufferLength;

        info->WriteEntry(lpszList, &length);
        if (dwBufferLength >= length) {

            //
            // we wrote it
            //

            dwBufferLength -= length;
        } else {

            //
            // no buffer left
            //

            dwBufferLength = 0;
            outOfBuffer = TRUE;
        }
        *lpdwRequiredLength += length;
        lpszList += length;
    }

    if (!outOfBuffer) {
        if (dwBufferLength > 0) {
            *lpszList++ = '\0';
            *lplpszList = lpszList;
        }
    }

    //
    // add 1 for the terminating NUL
    //

    ++*lpdwRequiredLength;

    UnlockSerializedList(&_List);
}


BOOL
PROXY_BYPASS_LIST_ENTRY::WriteEntry(
    OUT LPSTR lpszBuffer,
    IN OUT LPDWORD lpdwBufferLength
    )

/*++

Routine Description:

    Writes this proxy bypass list entry as a string in the supplied buffer

Arguments:

    lpszBuffer          - pointer to buffer where string is written

    lpdwBufferLength    - IN: amount of space in buffer
                          OUT: number of bytes copied, or required size

Return Value:

    BOOL
        TRUE    - entry written to buffer

        FALSE   - entry not written to buffer - *lpdwBufferLength contains
                  required size

--*/

{
    DWORD requiredLength;
    LPSTR schemeName;
    DWORD schemeNameLength;
    INTERNET_PORT magnitude;

    if (_Scheme != INTERNET_SCHEME_DEFAULT) {
        schemeName = MapUrlScheme(_Scheme, &schemeNameLength);
        requiredLength = schemeNameLength + sizeof("://") - 1;
    } else {
        schemeName = NULL;
        requiredLength = 0;
    }
    if (IsLocal()) {
        requiredLength += sizeof("<local>") - 1;
    } else {
        requiredLength += _Name.StringLength();
    }
    if (_Port != INTERNET_INVALID_PORT_NUMBER) {
        for (INTERNET_PORT n = 10000, i = 5; n > 0; n /= 10, --i) {
            if (_Port / n) {
                requiredLength += i + 1;
                magnitude = n;
                break;
            }
        }
    }

    BOOL success;

    if (*lpdwBufferLength > requiredLength) {
        if (schemeName != NULL) {
            memcpy(lpszBuffer, schemeName, schemeNameLength);
            lpszBuffer += schemeNameLength;
            memcpy(lpszBuffer, "://", sizeof("://") - 1);
            lpszBuffer += sizeof("://") - 1;
        }
        if (IsLocal()) {
            memcpy(lpszBuffer, "<local>", sizeof("<local>") - 1);
            lpszBuffer += sizeof("<local>") - 1;
        } else {
            _Name.CopyTo(lpszBuffer);
            lpszBuffer += _Name.StringLength();
        }
        if (_Port != INTERNET_INVALID_PORT_NUMBER) {
            *lpszBuffer++ = ':';
            for (INTERNET_PORT n = _Port, i = magnitude; i; i /= 10) {
                *lpszBuffer++ = (char)(n / i) + '0';
                n %= i;
            }
        }
        success = TRUE;
    } else {
        success = FALSE;
    }
    *lpdwBufferLength = requiredLength;
    return success;
}


DWORD
PROXY_BYPASS_LIST::AddList(
    IN LPSTR lpszList
    )

/*++

Routine Description:

    Parses a list of proxy bypass specifiers and adds them to the list

Arguments:

    lpszList    - pointer to string containing list of proxy bypass specifiers.
                  The format is:

                    [<scheme>"://"][<server>][":"<port>"]

                  The list can be NULL, in which case we read it from the
                  registry

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_PARAMETER

--*/

{
    DEBUG_ENTER((DBG_PROXY,
                Dword,
                "PROXY_BYPASS_LIST::AddList",
                "%.80q",
                lpszList
                ));

    DWORD entryLength;
    LPSTR schemeName;
    DWORD schemeLength;
    LPSTR serverName;
    DWORD serverLength;
    PARSER_STATE state;
    DWORD nSlashes;
    INTERNET_PORT port;
    BOOL done;

    entryLength = 0;
    schemeName = lpszList;
    schemeLength = 0;
    serverName = NULL;
    serverLength = 0;
    state = STATE_SCHEME;
    nSlashes = 0;
    port = 0;
    done = FALSE;

    //
    // walk the list, pulling out the various scheme parts
    //

    do {

        char ch = *lpszList++;

        if ((nSlashes == 1) && (ch != '/')) {
            state = STATE_ERROR;
            break;
        }

        switch (ch) {
        case ':':
            switch (state) {
            case STATE_SCHEME:
                if (*lpszList == '/') {
                    schemeLength = entryLength;
                } else if (*lpszList != '\0') {
                    serverName = schemeName;
                    serverLength = entryLength;
                    if (serverLength == 0) {
                        serverLength = 1;
                        serverName = "*";
                    }
                    state = STATE_PORT;
                } else {
                    state = STATE_ERROR;
                }
                entryLength = 0;
                break;

            case STATE_SERVER:
                serverLength = entryLength;
                state = STATE_PORT;
                entryLength = 0;
                break;

            default:
                state = STATE_ERROR;
                break;
            }
            break;

        case '/':
            if ((state == STATE_SCHEME) && (nSlashes < 2) && (entryLength == 0)) {
                if (++nSlashes == 2) {
                    state = STATE_SERVER;
                    serverName = lpszList;
                }
            } else {
                state = STATE_ERROR;
            }
            break;

        default:
            if (state != STATE_PORT) {
                ++entryLength;
            } else if (isdigit(ch)) {

                //
                // BUGBUG - we will overflow if >65535
                //

                port = port * 10 + (ch - '0');
            } else {

                //
                // STATE_PORT && non-digit character - error
                //

                state = STATE_ERROR;
            }
            break;

        case '\0':
            done = TRUE;

            //
            // fall through
            //

        case '\t':
        case '\n':
        case '\v':  // vertical tab, 0x0b
        case '\f':  // form feed, 0x0c
        case '\r':
        case ' ':
        case ';':
        case ',':
            if (serverLength == 0) {
                serverLength = entryLength;
                if ((serverLength == 0)
                && ((state == STATE_SERVER) || (state == STATE_PORT))) {

                    //
                    // we found e.g. "http://" or "http://:80". We allow this as
                    // "http://*" or "http://*:80"
                    //

                    serverLength = 1;
                    serverName = "*";
                }
            }
            if (serverLength != 0) {
                if (serverName == NULL) {
                    serverName = schemeName;
                }

                INTERNET_SCHEME scheme;

                if (schemeLength != 0) {
                    scheme = MapUrlSchemeName(schemeName, schemeLength);
                } else {
                    scheme = INTERNET_SCHEME_DEFAULT;
                }

                //
                // add an entry if this is a protocol we handle and we don't
                // already have an entry for it
                //

                if ((scheme != INTERNET_SCHEME_UNKNOWN)
                && !Find(scheme, serverName, serverLength, port)) {

                    //
                    // don't worry if Add() fails - we just continue
                    //

                    Add(scheme, serverName, serverLength, port);
                }
            }
            entryLength = 0;
            schemeName = lpszList;
            schemeLength = 0;
            serverName = NULL;
            serverLength = 0;
            nSlashes = 0;
            port = 0;
            state = STATE_SCHEME;
            break;
        }
        if (state == STATE_ERROR) {
            break;
        }
    } while (!done);

    DWORD error;

    if (state == STATE_ERROR) {
        error = ERROR_INVALID_PARAMETER;
    } else {
        error = ERROR_SUCCESS;
    }

    DEBUG_LEAVE(error);

    return error;
}


BOOL
PROXY_BYPASS_LIST::Find(
    IN INTERNET_SCHEME tScheme,
    IN LPSTR lpszHostName OPTIONAL,
    IN DWORD dwHostNameLength,
    IN INTERNET_PORT nPort
    )

/*++

Routine Description:

    Determines if a proxy bypass entry matches the criteria.

    Currently, name matching is simplistic: e.g. "*.com" and "**.com" are
    treated as 2 separate strings, where we should collapse multiple wildcard
    specifiers, etc. Also: "w*" should replace "ww*", etc.

Arguments:

    tScheme             - scheme for this entry

    lpszHostName        - host name or address. May contain wildcards (*)

    dwHostNameLength    - length of host name or address

    nPort               - port

Return Value:

    BOOL
        TRUE    - an entry corresponding to the arguments was found

        FALSE   - didn't find entry

--*/

{
    DEBUG_ENTER((DBG_PROXY,
                Bool,
                "PROXY_BYPASS_LIST::Find",
                "%s, %.*q, %d, %d",
                InternetMapScheme(tScheme),
                dwHostNameLength,
                lpszHostName,
                dwHostNameLength,
                nPort
                ));

    BOOL isLocal = IsLocalMacro(lpszHostName, dwHostNameLength);
    BOOL found = FALSE;

    LockSerializedList(&_List);

    for (PLIST_ENTRY entry = HeadOfSerializedList(&_List);
        entry != (PLIST_ENTRY)SlSelf(&_List);
        entry = entry->Flink) {

        PROXY_BYPASS_LIST_ENTRY * info;

        info = CONTAINING_RECORD(entry, PROXY_BYPASS_LIST_ENTRY, _List);

        //
        // do the easy bits first
        //

        if (!((info->_Scheme == tScheme)
        || (info->_Scheme == INTERNET_SCHEME_DEFAULT))) {
            continue;
        }
        if (!((info->_Port == nPort)
        || (info->_Port == INTERNET_INVALID_PORT_NUMBER))) {
            continue;
        }

        //
        // check for name match
        //

        if (info->_LocalSemantics) {
            if (isLocal) {
                found = TRUE;
                break;
            } else {
                continue;
            }
        }

        //
        // not local semantics, have to match target
        //

        //
        // BUGBUG - we only do simplistic matching. If the strings don't match
        //          exactly, except for case, they are deemed to be different
        //

        if (info->_Name.Strnicmp(lpszHostName, (int)dwHostNameLength) != 0) {
            continue;
        }

        //
        // any path that didn't continue, or has not already broken out has
        // succeeded in finding a match
        //

        DEBUG_PRINT(PROXY,
                    INFO,
                    ("Matched: %q, %q\n",
                    lpszHostName,
                    info->_Name.StringAddress()
                    ));

        found = TRUE;
        break;
    }

    UnlockSerializedList(&_List);

    DEBUG_LEAVE(found);

    return found;
}


DWORD
PROXY_BYPASS_LIST::Add(
    IN INTERNET_SCHEME tScheme,
    IN LPSTR lpszHostName,
    IN DWORD dwHostNameLength,
    IN INTERNET_PORT nPort
    )

/*++

Routine Description:

    Create and add a PROXY_BYPASS_LIST_ENTRY to the PROXY_BYPASS_LIST

Arguments:

    tScheme             - scheme to bypass. May be 0 meaning any protocol

    lpszHostName        - name of host to bypass. May be name or IP address and
                          may contain wildcard characters

    dwHostNameLength    - length of bypass name string

    nPort               - port to bypass. May be 0, meaning any port

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DEBUG_ENTER((DBG_PROXY,
                Dword,
                "PROXY_BYPASS_LIST::Add",
                "%s, %.*q, %d, %d",
                InternetMapScheme(tScheme),
                dwHostNameLength,
                lpszHostName,
                dwHostNameLength,
                nPort
                ));

    PROXY_BYPASS_LIST_ENTRY * entry;

    entry = new PROXY_BYPASS_LIST_ENTRY(tScheme,
                                        lpszHostName,
                                        dwHostNameLength,
                                        nPort
                                        );

    DWORD error;

    if (entry != NULL) {

        //
        // if the bypass entry uses local name matching semantics, then we add
        // it to the end of the list, else the head. The reason we do this is
        // to allow <local> to be a default after all other (possibly also
        // local) entries are checked
        //

        if (entry->IsLocal()) {
            InsertAtTailOfSerializedList(&_List, &entry->_List);
        } else {
            InsertAtHeadOfSerializedList(&_List, &entry->_List);
        }
        error = ERROR_SUCCESS;
    } else {
        error = ERROR_NOT_ENOUGH_MEMORY;
    }

    DEBUG_LEAVE(error);

    return error;
}


BOOL
PROXY_BYPASS_LIST::IsBypassed(
    IN INTERNET_SCHEME tScheme,
    IN LPSTR lpszHostName,
    IN DWORD dwHostNameLength,
    IN INTERNET_PORT nPort
    )

/*++

Routine Description:

    Determines if a scheme/name/port is bypassed

Arguments:

    tScheme             - can be 0, meaning match any scheme

    lpszHostName        - can contain wildcards. May be name or IP address

    dwHostNameLength    - length of name/address part. May be 0, meaning match
                          any name/address

    nPort               - can be 0, meaning match any port

Return Value:

    BOOL
        TRUE    - an entry on the bypass list matched the criteria

        FALSE   - the host identified by the parameters is not on this bypass
                  list

--*/

{
    DEBUG_ENTER((DBG_PROXY,
                Bool,
                "PROXY_BYPASS_LIST::IsBypassed",
                "%s, %.*q, %d, %d",
                InternetMapScheme(tScheme),
                dwHostNameLength,
                lpszHostName,
                dwHostNameLength,
                nPort
                ));

    INET_ASSERT(lpszHostName != NULL);
    INET_ASSERT(dwHostNameLength != 0);

    //
    // determine if what we were given is an address, in which case we don't
    // perform <local> semantics matching
    //

    BOOL isAddress = FALSE;
    LPSTR mappedName = NULL;
    LPSTR allocedName = NULL;

    if (dwHostNameLength <= MAX_IP_ADDRESS_STRING_LENGTH) {

        char addressBuffer[MAX_IP_ADDRESS_STRING_LENGTH + 1];

        //
        // make the host name/address an ASCIIZ string
        //

        memcpy((LPVOID)addressBuffer, (LPVOID)lpszHostName, dwHostNameLength);
        addressBuffer[dwHostNameLength] = '\0';
        if (_I_inet_addr(addressBuffer) != INADDR_NONE) {

            //
            // looks like we were given an IP address
            //

            //
            // maybe this is the IP address of a known server (in cache)
            //

            mappedName = MapNetAddressToName(addressBuffer, &allocedName);
            if (mappedName == addressBuffer) {

                //
                // BUGBUG - transport independence?
                //

                isAddress = TRUE;
            } else {
                lpszHostName = mappedName;
                dwHostNameLength = lstrlen(lpszHostName);
            }
        }
    }

    BOOL found;
    found = IsHostInBypassList (
                tScheme,
                lpszHostName,
                dwHostNameLength,
                nPort,
                isAddress);

    if (allocedName != NULL) {

        allocedName = (LPSTR)FREE_MEMORY(allocedName);
        INET_ASSERT(allocedName == NULL);
    }

    DEBUG_LEAVE(found);
    return found;
}


BOOL
PROXY_BYPASS_LIST::IsHostInBypassList(
    IN INTERNET_SCHEME tScheme,
    IN LPSTR lpszHostName,
    IN DWORD dwHostNameLength,
    IN INTERNET_PORT nPort,
    IN BOOL isAddress
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    tScheme             -
    lpszHostName        -
    dwHostNameLength    -
    nPort               -
    isAddress           -

Return Value:

    BOOL

--*/

{
    DEBUG_ENTER((DBG_PROXY,
                 Bool,
                 "PROXY_BYPASS_LIST::IsHostInBypassList",
                 "%d (%s), %.*q, %d, %d, %B",
                 tScheme,
                 InternetMapScheme(tScheme),
                 dwHostNameLength,
                 lpszHostName,
                 dwHostNameLength,
                 nPort,
                 isAddress
                 ));

    BOOL found = FALSE;

    //
    // if not an address, determine if the name contains at least one dot
    //

    BOOL isDot;

    if (!isAddress) {
        isDot = FALSE;
        for (DWORD i = 0; i < dwHostNameLength; ++i) {
            if (lpszHostName[i] == '.') {
                isDot = TRUE;
                break;
            }
        }
    } else {

        //
        // addresses have dots
        //

        isDot = TRUE;
    }

    LockSerializedList(&_List);

    for (PLIST_ENTRY entry = HeadOfSerializedList(&_List);
        entry != (PLIST_ENTRY)SlSelf(&_List);
        entry = entry->Flink) {

        PROXY_BYPASS_LIST_ENTRY * info;

        info = CONTAINING_RECORD(entry, PROXY_BYPASS_LIST_ENTRY, _List);

        //
        // do the easy bits first
        //

        if (!((info->_Scheme == tScheme)
        || (info->_Scheme == INTERNET_SCHEME_DEFAULT))) {
            continue;
        }
        if (!((info->_Port == nPort)
        || (info->_Port == INTERNET_INVALID_PORT_NUMBER))) {
            continue;
        }

        //
        // check local semantics
        //

        if (info->_LocalSemantics) {
            if (!isDot) {

                DEBUG_PRINT(PROXY,
                            INFO,
                            ("%q matched by <local>\n",
                            lpszHostName
                            ));

                found = TRUE;

                //
                // <local> is in the bypass list and the name does not contain a
                // dot. It bypasses the proxy
                //

                break;
            } else {

                //
                // the name contains a dot, but it may be matched by another
                // proxy bypass entry
                //

                continue;
            }
        }

        //
        // check for name match. Note that we take no special action if the host
        // name contains wildcard characters
        //

        LPSTR target = info->_Name.StringAddress();

        //
        // NULL target name matches any server name/address
        //

        if (target != NULL) {

            DEBUG_PRINT(PROXY,
                        INFO,
                        ("trying to match %q with %q\n",
                        lpszHostName,
                        target
                        ));

            DWORD i = 0;
            DWORD j = 0;
            DWORD i_back = (DWORD)-1;

            while ((target[i] != '\0') && (j < dwHostNameLength)) {
                if (target[i] == tolower(lpszHostName[j])) {
                    ++i;
                    ++j;
                } else if (target[i] == '*') {
                    while (target[i + 1] == '*') {
                        ++i;
                    }
                    i_back = i;
                    ++i;
                    while ((tolower(lpszHostName[j]) != target[i])
                    && (j < dwHostNameLength)) {
                        ++j;
                    }
                } else if (i_back != (DWORD)-1) {

                    //
                    // '*' is greedy closure. We already saw a '*' but later we
                    // discovered a mismatch. We will go back and try to eat as
                    // many characters as we can till the next match, or we hit
                    // the end of the string
                    //

                    i = i_back;
                } else {

                    //
                    // no match; quit
                    //

                    j = 0;
                    break;
                }

                //
                // if we reached the end of the target, but not the host name
                // AND we already met a '*' then back up
                //

                if ((target[i] == '\0')
                && (j != dwHostNameLength)
                && (i_back != (DWORD)-1)) {
                    i = i_back;
                }
            }

            //
            // if we hit the end of the host name while matching any character,
            // bump the target to the next non-star character
            //

            while (target[i] == '*') {
                ++i;
            }

            //
            // the host name matched if we reached the end of the target and end
            // of the host name
            //

            if (!((target[i] == '\0') && (j == dwHostNameLength))) {
                continue;
            }
        }

        //
        // any path that didn't continue, or has not already broken out has
        // succeeded in finding a match
        //

        DEBUG_PRINT(PROXY,
                    INFO,
                    ("Matched: %q, %q\n",
                    lpszHostName,
                    target
                    ));

        found = TRUE;
        break;
    }

    UnlockSerializedList(&_List);

    DEBUG_LEAVE(found);

    return found;
}


VOID
PROXY_BYPASS_LIST::GetList(
    OUT LPSTR * lplpszList,
    IN DWORD dwBufferLength,
    IN OUT LPDWORD lpdwRequiredLength
    )

/*++

Routine Description:

    Writes the list of proxy bypass servers to a buffer, and/or returns the
    required buffer length

Arguments:

    lplpszList          - pointer to pointer to buffer where list is written, if
                          sufficient space

    dwBufferLength      - amount of space in *lplpszList

    lpdwRequiredLength  - OUT: cumulative size of data

Return Value:

    None.
--*/

{
    LPSTR lpszList = *lplpszList;
    BOOL firstTime = TRUE;
    BOOL outOfBuffer = FALSE;

    LockSerializedList(&_List);

    for (PLIST_ENTRY entry = HeadOfSerializedList(&_List);
        entry != (PLIST_ENTRY)SlSelf(&_List);
        entry = entry->Flink) {

        PROXY_BYPASS_LIST_ENTRY * info;

        info = CONTAINING_RECORD(entry, PROXY_BYPASS_LIST_ENTRY, _List);
        if (!firstTime) {

            //
            // write delimiter if enough space
            //

            if (dwBufferLength >= 1) {
                *lpszList++ = ' ';
                --dwBufferLength;
            }
            ++*lpdwRequiredLength;
        } else {
            firstTime = FALSE;
        }

        //
        // find the length of the current entry & write it to the buffer if
        // enough space
        //

        DWORD length = dwBufferLength;

        info->WriteEntry(lpszList, &length);
        if (dwBufferLength >= length) {

            //
            // we wrote it
            //

            dwBufferLength -= length;
        } else {

            //
            // no buffer left
            //

            dwBufferLength = 0;
            outOfBuffer = TRUE;
        }
        *lpdwRequiredLength += length;
        lpszList += length;
    }

    if (!outOfBuffer) {
        if (dwBufferLength > 0) {
            *lpszList++ = '\0';
            *lplpszList = lpszList;
        }
    }

    //
    // add 1 for the terminating NUL
    //

    ++*lpdwRequiredLength;
    UnlockSerializedList(&_List);
}

//
// PROXY_INFO - methods are defined below
//

VOID 
PROXY_INFO::InitializeProxySettings(
    VOID
    )    

/*++

Routine Description:

    Initalizes Proxy_Info objects

Arguments:

    None.

Return Value:

    None. 

--*/

{
    _ProxyServerList    = NULL;
    _ProxyBypassList    = NULL;
    _fDisableDirect     = FALSE;
    _fModifiedInProcess = FALSE;

    _Lock.Initialize();
    _Error = _Lock.IsInitialized()
                ? ERROR_SUCCESS
                : ERROR_INTERNET_INTERNAL_ERROR;
}


VOID 
PROXY_INFO::TerminateProxySettings(
    VOID
    )

/*++

Routine Description:

    Cleans up and destroys Proxy_Info objects

Arguments:

    None.

Return Value:

    None. 

--*/

{
    //DEBUG_ENTER((DBG_OBJECTS,
    //             None,
    //             "PROXY_INFO::TerminateProxySettings",
    //             NULL
    //             ));

    Lock(TRUE);
    CleanOutLists();
    Unlock();

    //DEBUG_LEAVE(0);
}




DWORD
PROXY_INFO::SetProxySettings(
    IN LPINTERNET_PROXY_INFO_EX  lpProxySettings,
    IN BOOL fModifiedInProcess
    )

/*++

Routine Description:

    Sets the proxy info. Either creates new proxy server and bypass lists, or
    removes them (proxy to direct)

    Assumes: 1. The parameters have already been validated in the API that calls
                this method (i.e. InternetOpen(), InternetSetOption())

Arguments:

    
    lpProxySettings     - a set of relevent fields describing proxy settings

    fModifiedInProcess  - TRUE, if this object keeps a seperate set of values from those
                            stored in the registry store



Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_PARAMETER
                    The lpszProxy or lpszProxyBypass list was bad

                  ERROR_NOT_ENOUGH_MEMORY
                    Failed to create an object or allocate space for a list,
                    etc.

--*/

{
    DEBUG_ENTER((DBG_PROXY,
                 Dword,
                 "PROXY_INFO::SetProxySettings",
                 "%x, %B",
                 lpProxySettings,
                 fModifiedInProcess
                 ));

    //
    // parameters should already be validated by caller
    //

    BOOL newList;
    BOOL possibleNewAutoProxy;
    LPCTSTR serverList;
    LPCTSTR bypassList;

    DWORD error = ERROR_SUCCESS;

    serverList = NULL;
    bypassList = NULL;
    newList = FALSE;
    _fModifiedInProcess = fModifiedInProcess;
    _dwSettingsVersion  = lpProxySettings->dwCurrentSettingsVersion;

    UPDATE_GLOBAL_PROXY_VERSION();

    if ( lpProxySettings->dwFlags & PROXY_TYPE_PROXY ) 
    {        
        serverList = lpProxySettings->lpszProxy;
        bypassList = lpProxySettings->lpszProxyBypass;

        if (serverList != NULL) {            
            newList = TRUE;
        }
    }

    //
    // about to start changing contents - acquire lock
    //

    Lock(TRUE);

    // remember disable direct flag...
    SetDisableDirect( (lpProxySettings->dwFlags & PROXY_TYPE_DIRECT) ? FALSE : TRUE  );

    //
    // clear out current contents,
    //

    CleanOutLists();

    //
    // Set the Static Proxy Lists
    //
        
    if (newList) 
    {

        INET_ASSERT((serverList != NULL) && (*serverList != 0));

        _ProxyServerList = new PROXY_SERVER_LIST(serverList);
        _ProxyBypassList = new PROXY_BYPASS_LIST(bypassList);

        if ((_ProxyServerList != NULL) && (_ProxyBypassList != NULL)) {
            _Error = _ProxyServerList->GetError();
            if (_Error == ERROR_SUCCESS) {
                _Error = _ProxyBypassList->GetError();
                if (_Error == ERROR_SUCCESS) {

                    //
                    // add all proxy servers to bypass list
                    //

                    _ProxyServerList->AddToBypassList(_ProxyBypassList);
                }
            }
        } else {
            _Error = ERROR_NOT_ENOUGH_MEMORY;
            CleanOutLists();
        }
        error = _Error;
    }

    //
    // other threads free to access this PROXY_INFO again
    //

    Unlock();

    DEBUG_LEAVE(error);

    return error;
}


DWORD
PROXY_INFO::GetProxySettings(
    OUT LPINTERNET_PROXY_INFO_EX  lpProxySettings,
    IN BOOL fCheckVersion = FALSE
    )

/*++

Routine Description:

    Gets the proxy info. 

    Assumes: 1. The parameters have already been validated in the API that calls
                this method (i.e. InternetOpen(), InternetSetOption())

Arguments:

    
    lpProxySettings     - a set of relevent fields describing proxy settings

    fCheckVersion       - ignored 


Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - 

--*/

{
    DEBUG_ENTER((DBG_PROXY,
                 Dword,
                 "PROXY_INFO::GetProxySettings",
                 "%x, %B",
                 lpProxySettings,
                 fCheckVersion
                 ));

    DWORD error = ERROR_SUCCESS;

    if ( fCheckVersion == TRUE )
    {
        INET_ASSERT(FALSE);
        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }

    //
    // about to start reading contents - acquire lock
    //

    Lock(FALSE);

    if ( ! IsDisableDirect() ) {
        lpProxySettings->dwFlags |= PROXY_TYPE_DIRECT;
    }

    if ( IsProxySettingsConfigured() ) 
    {   
        lpProxySettings->dwFlags |= PROXY_TYPE_PROXY;

        lpProxySettings->lpszProxy       = _ProxyServerList->CopyString();
        lpProxySettings->lpszProxyBypass = _ProxyBypassList->CopyString();

        if ( lpProxySettings->lpszProxy == NULL || 
             lpProxySettings->lpszProxyBypass == NULL )
        {
            error = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    //
    // other threads free to access this PROXY_INFO again
    //

    Unlock();

quit:

    DEBUG_LEAVE(error);

    return error;
}


DWORD
PROXY_INFO::RefreshProxySettings(
    IN BOOL fForceRefresh
    )
/*++

Routine Description:

    Refreshes the Proxy Information

    This doesn't make sense on PROXY_INFO, nothing done

Arguments:

    fForceRefresh - forces a resync of all settings, turning this on slows things down

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - 

--*/

{
    DEBUG_ENTER((DBG_PROXY,
                 Dword,
                 "PROXY_INFO::RefreshProxySettings",
                 "%B",
                 fForceRefresh
                 ));


    DEBUG_LEAVE(ERROR_SUCCESS);
 
    return ERROR_SUCCESS;
}


DWORD
PROXY_INFO::QueryProxySettings(
    IN AUTO_PROXY_ASYNC_MSG **ppQueryForProxyInfo
    )

/*++

Routine Description:

    Determines what proxy type, proxy name, and port the caller should use
    given an Url, its length, a target host, a target port, and output space
    to store the result.

    The result may be a complete string containing a Netscape style string with
    a delimited set of proxies, and access methods.  An example of this may
    look like:
    "PROXY itgproxy:80; PROXY proxy:80; PROXY 192.168.100.2:1080; SOCKS 192.168.100.2; DIRECT"
    This means we must try itgproxy, if this proxy fails we go on to proxy, and on to 192.168.100.2, etc.
    Note that if itgproxy, proxy, and 192.168.100.2 all fail we can try a SOCKS proxy, and if this fails we
    can try a direct connection.

    OR

    The result can be a simply IE 3.0 proxyname.  Ex: "proxy".  You can figure out
    which by calling IsNetscapeProxyListString() on the returned proxy hostname.

    If there is an external proxy DLL registered and valid, we defer to it to decide
    what proxy to use, and thus ignore internal proxy information.

    Note this function can also be used to retrive mapping of protocol to proxy.  For example,
    if tUrlProtocol == INTERNET_SCHEME_FTP, the result *lptProxyScheme == INTERNET_SCHEME_SOCKS
    which means we should use a socks proxy/firewall for FTP accesss.

Arguments:

    tScheme             - can be 0, meaning match any scheme

    lpszHostName        - can contain wildcards. May be name or IP address

    nPort               - can be 0, meaning match any port

    pfAutoProxy         - TRUE if an auto-proxy is being used.

Return Value:

    BOOL
        TRUE    - an entry on the bypass list matched the criteria

        FALSE   - the host identified by the parameters is not on this bypass
                  list

--*/

{
    DEBUG_ENTER((DBG_PROXY,
                 Dword,
                 "PROXY_INFO::QueryProxySettings",
                 "%#X",
                 ppQueryForProxyInfo
                 ));

    AUTO_PROXY_ASYNC_MSG *pQueryForProxyInfo = *ppQueryForProxyInfo;
    INTERNET_SCHEME tProxyScheme = pQueryForProxyInfo->_tUrlProtocol;
    BOOL fIsByPassed = FALSE;
    BOOL fProxyConnect = FALSE;
    
    DWORD error = ERROR_SUCCESS;

    Lock(FALSE);

    if (!IsProxySettingsConfigured())  // virtual func, perhaps replace with faster internal?
    {
        fProxyConnect = FALSE;
        goto quit;
    }

    //
    // Ok, if we're here we are NOT using the Auto-Proxy DLL.
    //  1. Determine if we are Bypassed ( and thus prevented from using a proxy )
    //  2. Map the Protocol to a Proxy type.
    //  3. Grab the hostname of the proxy we wish to use.
    //

    if ( pQueryForProxyInfo->_lpszUrlHostName && pQueryForProxyInfo->_dwUrlHostNameLength > 0 )
    {
        fIsByPassed = IsBypassed(
                        pQueryForProxyInfo->_tUrlProtocol,
                        pQueryForProxyInfo->_lpszUrlHostName,
                        pQueryForProxyInfo->_dwUrlHostNameLength,
                        pQueryForProxyInfo->_nUrlPort
                        );

        if ( fIsByPassed )
        {
            goto quit;
        }
    }

    pQueryForProxyInfo->_tProxyScheme = ProxyScheme(pQueryForProxyInfo->_tUrlProtocol);

    if ( pQueryForProxyInfo->_tProxyScheme == INTERNET_SCHEME_UNKNOWN )
    {
       pQueryForProxyInfo->_tProxyScheme = INTERNET_SCHEME_SOCKS;
       pQueryForProxyInfo->_tUrlProtocol = INTERNET_SCHEME_SOCKS;
    }
    if (pQueryForProxyInfo->_bFreeProxyHostName
        && (pQueryForProxyInfo->_lpszProxyHostName != NULL)) {
        FREE_MEMORY(pQueryForProxyInfo->_lpszProxyHostName);
    }

    fProxyConnect = GetProxyHostName(
                        pQueryForProxyInfo->_tUrlProtocol,
                        &(pQueryForProxyInfo->_tProxyScheme),
                        &(pQueryForProxyInfo->_lpszProxyHostName),
                        &(pQueryForProxyInfo->_bFreeProxyHostName),
                        &(pQueryForProxyInfo->_dwProxyHostNameLength),
                        &(pQueryForProxyInfo->_nProxyHostPort)
                        );

quit:

    pQueryForProxyInfo->_dwQueryResult = (DWORD) fProxyConnect;

    //
    // If we've disabled direct connections, then fail
    //  when there is no proxy
    //

    if ( !fProxyConnect && IsDisableDirect() ) {         
        error = ERROR_INTERNET_CANNOT_CONNECT;
    }

    Unlock();

    DEBUG_LEAVE(error);

    return error;
}



DWORD
PROXY_INFO::GetProxyStringInfo(
    OUT LPVOID lpBuffer,
    IN OUT LPDWORD lpdwBufferLength
    )

/*++

Routine Description:

    IMPORTANT PLEASE READ: LEGACY FUNCTION, this does not support all the new
      proxy behaviors, left here for Wininet compat with older programs

    Returns the proxy server and bypass lists in an INTERNET_PROXY_INFO. Called
    by InternetQueryOption(INTERNET_OPTION_PROXY)

    Assumes: Access to this is serialized while we are getting this info

Arguments:

    lpBuffer            - pointer to buffer where information will be returned

    lpdwBufferLength    - IN: size of lpBuffer in BYTEs
                          OUT: number of BYTEs returned in lpBuffer

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INSUFFICIENT_BUFFER
                    *lpdwBufferLength contains the required buffer length
--*/

{
    DEBUG_ENTER((DBG_PROXY,
                Dword,
                "PROXY_INFO::GetProxyStringInfo",
                "%#x, %#x [%d]",
                lpBuffer,
                lpdwBufferLength,
                *lpdwBufferLength
                ));

    DEBUG_PRINT(PROXY,
                INFO,
                ("Calling Legacy GetProxyStringInfo, NEW CODE SHOULD AVOID THIS CODE PATH\n"
                ));

    DWORD requiredSize = sizeof(INTERNET_PROXY_INFO);
    LPSTR lpVariable = (LPSTR)(((LPINTERNET_PROXY_INFO)lpBuffer) + 1);
    LPSTR lpszProxy;

    Lock(FALSE);

    if (_ProxyServerList != NULL) {
        lpszProxy = lpVariable;
        _ProxyServerList->GetList(&lpVariable,
                                  (*lpdwBufferLength > requiredSize)
                                    ? (*lpdwBufferLength - requiredSize)
                                    : 0,
                                  &requiredSize
                                  );
    } else {
        lpszProxy = NULL;
    }

    LPSTR lpszProxyBypass;

    if (_ProxyBypassList != NULL) {

        DWORD size = requiredSize;

        lpszProxyBypass = lpVariable;
        _ProxyBypassList->GetList(&lpVariable,
                                  (*lpdwBufferLength > requiredSize)
                                    ? (*lpdwBufferLength - requiredSize)
                                    : 0,
                                  &requiredSize
                                  );
        if (requiredSize == size) {
            lpszProxyBypass = NULL;
        }
    } else {
        lpszProxyBypass = NULL;
    }

    DWORD error;

    if (*lpdwBufferLength >= requiredSize) {

        LPINTERNET_PROXY_INFO lpInfo = (LPINTERNET_PROXY_INFO)lpBuffer;

        lpInfo->dwAccessType = (lpszProxy == NULL)
                                    ? INTERNET_OPEN_TYPE_DIRECT
                                    : INTERNET_OPEN_TYPE_PROXY;
        lpInfo->lpszProxy = lpszProxy;
        lpInfo->lpszProxyBypass = lpszProxyBypass;
        error = ERROR_SUCCESS;
    } else {
        error = ERROR_INSUFFICIENT_BUFFER;
    }
    *lpdwBufferLength = requiredSize;

    Unlock();

    DEBUG_LEAVE(error);

    return error;
}


BOOL
PROXY_INFO::RedoSendRequest(
    IN OUT LPDWORD lpdwError,
    IN AUTO_PROXY_ASYNC_MSG *pQueryForProxyInfo,
    IN CServerInfo *pOriginServer,
    IN CServerInfo *pProxyServer
    )
/*++

Routine Description:

    Determines whether a connection needs to be retried do to a failed proxy.

Arguments:


    lpdwError   - Error code of connection.

    pProxyState - Pointer to proxy_state returned when acquiring the proxy information.


Return Value:

    LPSTR
        Success - pointer to allocated buffer

        Failure - NULL

--*/

{
    DEBUG_ENTER((DBG_PROXY,
                 Bool,
                 "PROXY_INFO::RedoSendRequest",
                 "%#x [%d], %#x",
                 lpdwError,
                 lpdwError ? *lpdwError : 0,
                 pQueryForProxyInfo
                 ));

    BOOL fReturn = FALSE;
    PROXY_STATE *pProxyState = NULL;
    DWORD dwVersion;
    LPSTR lpszConnection;
    BOOL fCanCache = FALSE;

    if ( pQueryForProxyInfo )
    {
        pProxyState = pQueryForProxyInfo->_pProxyState;

        //
        // On success,
        //

        if ( *lpdwError == ERROR_SUCCESS )
        {
            if ( pQueryForProxyInfo->IsCanCacheResult() && 
                 pProxyState &&                  
                 pOriginServer &&
                 pProxyServer )
            {
                
                pOriginServer->SetCachedProxyServerInfo(
                    pProxyServer,                    
                    pQueryForProxyInfo->GetVersion(),
                    pQueryForProxyInfo->IsUseProxy(),
                    pQueryForProxyInfo->_tUrlProtocol,
                    pQueryForProxyInfo->_nUrlPort,
                    pQueryForProxyInfo->_tProxyScheme,
                    pQueryForProxyInfo->_nProxyHostPort
                    );
            }
        }
        else if ( *lpdwError != ERROR_SUCCESS &&
                  *lpdwError != ERROR_INTERNET_OPERATION_CANCELLED &&
                  *lpdwError != ERROR_INTERNET_SEC_CERT_ERRORS &&
                  *lpdwError != ERROR_INTERNET_SECURITY_CHANNEL_ERROR &&
                  *lpdwError != ERROR_INTERNET_SEC_CERT_REVOKED &&
                  *lpdwError != ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED )
        {
            //
            // For backround detection, we need to retry
            //  waiting for the backround results to complete
            //
            // Otherwise, If we have additional proxies, 
            //  we need to retry them as well.
            //

            if ( pQueryForProxyInfo->IsBackroundDetectionPending() )
            {
                *lpdwError = ERROR_SUCCESS;
                fReturn = TRUE;
            }
            else if ( pProxyState &&
                     !pProxyState->IsEmpty() &&
                      pProxyState->IsAnotherProxyAvail() )               
            {
                INTERNET_PORT LastProxyUsedPort; 
                LPSTR lpszLastProxyUsed = pProxyState->GetLastProxyUsed(&LastProxyUsedPort);

                Lock(FALSE);

                if ( ( lpszLastProxyUsed == NULL ) ||
                     _BadProxyList.AddEntry(lpszLastProxyUsed, LastProxyUsedPort) != ERROR_SUCCESS )
                {
                    fReturn = FALSE;                    
                }
                else
                {
                    *lpdwError = ERROR_SUCCESS;
                    fReturn = TRUE;
                }

                Unlock();
            }
        }
    }

    DEBUG_LEAVE(fReturn);

    return fReturn;
}


VOID
PROXY_INFO::CleanOutLists(
    VOID
    )

/*++

Routine Description:

    Delete proxy server and bypass lists if not empty

    N.B. Exclusive lock MUST be acquired before calling this method

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_PROXY,
                 None,
                 "PROXY_INFO::CleanOutLists",
                 NULL
                 ));

    if (_ProxyServerList != NULL) {
        delete _ProxyServerList;
        _ProxyServerList = NULL;
    }
    if (_ProxyBypassList != NULL) {
        delete _ProxyBypassList;
        _ProxyBypassList = NULL;
    }

    DEBUG_LEAVE(0);
}

//
// PROXY_INFO_GLOBAL - Global Object thats inherits and expands the basic functionality 
//   of basic PROXY_INFO behavior.  The new functionality includes wrapping Auto-Proxy
///  and Auto-detection routines
//


VOID
PROXY_INFO_GLOBAL::TerminateProxySettings(
    VOID
    )

/*++

Routine Description:

    Destroy PROXY_INFO_GLOBAL object

Arguments:

    None.

Return Value:

    None.

--*/

{
    //DEBUG_ENTER((DBG_OBJECTS,
    //             None,
    //             "PROXY_INFO_GLOBAL::TerminateProxySettings",
    //             NULL
    //             ));

    Lock(TRUE);    
    if (_AutoProxyList != NULL) {
        _AutoProxyList->FreeAutoProxyInfo(); // free async component of object
        delete _AutoProxyList;
        _AutoProxyList = NULL;
    }
    Unlock();
    PROXY_INFO::TerminateProxySettings();

    //DEBUG_LEAVE(0);
}

DWORD
PROXY_INFO_GLOBAL::SetProxySettings(
    IN LPINTERNET_PROXY_INFO_EX  lpProxySettings,
    IN BOOL fModifiedInProcess
    )

/*++

Routine Description:

    Sets the proxy info.  Mainly handles Auto-Config, its decendent will handle static stuff

    Assumes: 1. The parameters have already been validated in the API that calls
                this method (i.e. InternetOpen(), InternetSetOption())

Arguments:
    
    lpProxySettings     - a set of relevent fields describing proxy settings

    fModifiedInProcess  - TRUE if this object is not from the registry, but 
                           a modifed setting for this process (that overrides reg values)

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_PARAMETER
                    The lpszProxy or lpszProxyBypass list was bad

                  ERROR_NOT_ENOUGH_MEMORY
                    Failed to create an object or allocate space for a list,
                    etc.

--*/

{
    DEBUG_ENTER((DBG_PROXY,
                 Dword,
                 "PROXY_INFO_GLOBAL::SetProxySettings",
                 "%x, %B",
                 lpProxySettings,
                 fModifiedInProcess
                 ));

    DWORD error = ERROR_SUCCESS;

    //
    // about to start changing contents - acquire lock
    //

    Lock(TRUE);

    //
    // Once we're set to Modified, we're modified for the lifetime of the
    //   the process, and thus we no longer accept Registry settings
    //

    if ( IsModifiedInProcess() && 
         !fModifiedInProcess )
    {
        error = ERROR_SUCCESS;
        goto quit;
    }

    if ( _lpszConnectionName != NULL ) {
        FREE_MEMORY(_lpszConnectionName);
    }

    _lpszConnectionName = lpProxySettings->lpszConnectionName ? 
                            NewString(lpProxySettings->lpszConnectionName) : 
                            NULL;

    _dwProxyFlags = lpProxySettings->dwFlags;

    //
    // Allocate Auto-Proxy/Auto-Detect Object, and reset its settings
    //

    if ( ((lpProxySettings->dwFlags & PROXY_TYPE_AUTO_PROXY_URL) ||
          (lpProxySettings->dwFlags & PROXY_TYPE_AUTO_DETECT)) &&
         IsGlobal() &&
         !( _AutoProxyList &&
            _AutoProxyList->IsOnAsyncAutoProxyThread()) )
    {        
        if (_AutoProxyList == NULL)
        {
            //
            // Create a new Auto-Proxy List, this will suck down registry
            //   info (for the DLLs) at initialization
            //

            _AutoProxyList = new AUTO_PROXY_DLLS();

            if ( _AutoProxyList == NULL )
            {
                error = ERROR_NOT_ENOUGH_MEMORY;
                goto quit;
            }

            error =  _AutoProxyList->GetError();

            if ( error != ERROR_SUCCESS ) {
                goto quit;
            }
        }

        error = _AutoProxyList->SetProxySettings(
                    lpProxySettings,
                    fModifiedInProcess,
                    TRUE     // fAllowOverwrite
                    );
        if ( error != ERROR_SUCCESS ) { 
            goto quit;
        }
    }

    //
    // Set the Static Proxy Lists
    //
        
    error = PROXY_INFO::SetProxySettings(lpProxySettings, fModifiedInProcess);

quit:

    //
    // other threads free to access this PROXY_INFO again
    //

    Unlock();

    DEBUG_LEAVE(error);

    return error;
}


DWORD
PROXY_INFO_GLOBAL::GetProxySettings(
    IN LPINTERNET_PROXY_INFO_EX  lpProxySettings,
    IN BOOL fCheckVersion = FALSE
    )

/*++

Routine Description:

    Gather the proxy info.  Mainly handles Auto-Config, its decendent will handle static stuff

Arguments:
    
    lpProxySettings     - a set of relevent fields describing proxy settings

    fCheckVersion       - 

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_PARAMETER
                    The lpszProxy or lpszProxyBypass list was bad

                  ERROR_NOT_ENOUGH_MEMORY
                    Failed to create an object or allocate space for a list,
                    etc.

--*/

{
    DEBUG_ENTER((DBG_PROXY,
                 Dword,
                 "PROXY_INFO_GLOBAL::GetProxySettings",
                 "%x, %B",
                 lpProxySettings,
                 fCheckVersion
                 ));

    //
    // about to start reading contents - acquire lock
    //

    Lock(FALSE);

    DWORD error = ERROR_SUCCESS;

    lpProxySettings->lpszConnectionName =
                        _lpszConnectionName ? 
                            NewString(_lpszConnectionName) : 
                            NULL;

    lpProxySettings->dwFlags = _dwProxyFlags;

    //
    // Check Auto-Proxy/Auto-Detect Object, and read its settings
    //

    if ( IsGlobal() &&
         _AutoProxyList)
    {        
        error = _AutoProxyList->GetProxySettings(
                    lpProxySettings,
                    fCheckVersion   
                    );

        if ( error != ERROR_SUCCESS ) { 
            goto quit;
        }
    }

    //
    // Get the Static Proxy Lists
    //
        
    error = PROXY_INFO::GetProxySettings(lpProxySettings, fCheckVersion);

quit:

    //
    // other threads free to access this PROXY_INFO again
    //

    Unlock();

    DEBUG_LEAVE(error);

    return error;
}


DWORD
PROXY_INFO_GLOBAL::RefreshProxySettings(
    IN BOOL fForceRefresh
    )

/*++

Routine Description:

    Force a refresh of automatic settings, such as auto-proxy, auto-detect

Arguments:
    
    fForceRefresh - Forces a hard refresh

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_PARAMETER
                    The lpszProxy or lpszProxyBypass list was bad

                  ERROR_NOT_ENOUGH_MEMORY
                    Failed to create an object or allocate space for a list,
                    etc.

--*/

{
    DEBUG_ENTER((DBG_PROXY,
                 Dword,
                 "PROXY_INFO_GLOBAL::RefreshProxySettings",
                 "%B",
                 fForceRefresh
                 ));

    DWORD error = ERROR_SUCCESS;

    Lock(FALSE);

    //
    // Force reload of registry settings and download of auto-proxy file from server
    //

    if ( IsRefreshDisabled())
    {
        QueueRefresh();
        goto quit;
    }

    //
    // Check Auto-Proxy/Auto-Detect Object, and read its settings
    //

    if ( IsGlobal() &&
         _AutoProxyList &&
         !(_AutoProxyList->IsOnAsyncAutoProxyThread()) )
    {                
        error = _AutoProxyList->RefreshProxySettings(
                    fForceRefresh
                    );

        if ( error != ERROR_SUCCESS ) { 
            goto quit;
        }
    }

    //
    // Get the Static Proxy Lists
    //
        
    //error = PROXY_INFO::RefreshProxySettings(fForceRefresh);

quit:

    //
    // other threads free to access this PROXY_INFO again
    //

    Unlock();

    DEBUG_LEAVE(error);

    return error;
}

VOID
PROXY_INFO_GLOBAL::ReleaseQueuedRefresh(
    VOID
    )
/*++

Routine Description:

    Force a refresh of automatic settings, such as auto-proxy, auto-detect,
     When InternetOpen is called, allowing async threads.

Arguments:
    
    None.

Return Value:

    None.

--*/

{
    DWORD error = ERROR_SUCCESS;

    Lock(FALSE);

    SetRefreshDisabled(FALSE);

    if ( _fQueuedRefresh ) 
    {
        error = RefreshProxySettings(
                    FALSE
                    );

    }

    _fQueuedRefresh = FALSE;

    //
    // other threads free to access this PROXY_INFO again
    //

    Unlock();
}




DWORD
PROXY_INFO_GLOBAL::QueryProxySettings(
    IN AUTO_PROXY_ASYNC_MSG **ppQueryForProxyInfo
    )

/*++

Routine Description:

    Aquries Proxy Information.

    Note: if ppProxyState returns a non-NULL pointer than the Proxy
    strings can be assumed to be allocated pointers to strings.  Freeing
    the ppProxyState object will result in the pointers being freed as well.
    Otherwise the pointers will be to global string data that will not be
    freed unexpectedly.



Arguments:

    tUrlProtocol -  Scheme type, protocol that is being used.

    lpszUrl      -  Url being accessed.

    dwUrlLength  -  size of Url.

    lpszUrlHostName - Host name of site to connect to.

    dwUrlHostNameLength - Host name length.

    nUrlPort      - Port of server to connect to.

    lptProxyScheme - On output contains the correct proxy server type to use.
                        ex: if a SOCKS proxy is to be used to handle the FTP protocol,
                            this will be a INTERNET_SCHEME_SOCKS.

    lplpszProxyHostName - Pointer to allocated memory containing the address of
                           the proxy host name.



Return Value:

    LPSTR
        Success - pointer to allocated buffer

        Failure - NULL

--*/

{
    DEBUG_ENTER((DBG_PROXY,
                 Dword,
                 "PROXY_INFO_GLOBAL::QueryProxySettings",
                 "%#x",
                 ppQueryForProxyInfo
                 ));

    INET_ASSERT(ppQueryForProxyInfo);

    BOOL fReturn = FALSE;
    DWORD error = ERROR_SUCCESS;
    BOOL fNeedsGetNextProxy = FALSE;
    BOOL fLocked = FALSE;
    AUTO_PROXY_ASYNC_MSG *pQueryForProxyInfo = *ppQueryForProxyInfo;

    //
    // If we're dealing with a list of Proxies, we may have already tried one
    //  proxy and failed.  So go to the next proxy in the list
    //  and see if another one is availble to try.
    //

    if ( pQueryForProxyInfo->IsProxyEnumeration() )
    {
        fNeedsGetNextProxy = TRUE;
        goto quit;
    }

    if ( pQueryForProxyInfo->IsQueryOnCallback() &&
         ! pQueryForProxyInfo->IsAvoidAsyncCall())
    {
        goto quit;
    }

    Lock(FALSE);
    fLocked = TRUE;

    //
    // Determine if we're dealing with an Auto-Proxy DLL,
    //  that needs a call into its own implimentation of GetProxyInfo
    //

    if ( _AutoProxyList &&
         ((_dwProxyFlags & PROXY_TYPE_AUTO_PROXY_URL) ||
          (_dwProxyFlags & PROXY_TYPE_AUTO_DETECT))
          )
    {
        AUTO_PROXY_DLLS  *pAutoProxyList = _AutoProxyList;

        Unlock();
        fLocked = FALSE;

        error = pAutoProxyList->QueryProxySettings(ppQueryForProxyInfo);
        if ( error == ERROR_IO_PENDING ) {
            goto fastquit;
        }

        pQueryForProxyInfo = *ppQueryForProxyInfo;
        if ( pQueryForProxyInfo->IsUseProxy() || 
             error != ERROR_SUCCESS ) 
        {
           goto quit;
        }

        Lock(FALSE);
        fLocked = TRUE;
    }

    //
    // BUGBUG [arthurbi] I am NOT happy with this outragously bad logic
    //  and feel this is getting way out of hand.  This code below is to
    //  simply catch a case where we have auto-proxy and InternetOpenUrl
    //  needs to detect whether to use HTTP over proxy for {gopher, ftp}
    //  OR direct gopher, ftp.
    //

    if ( pQueryForProxyInfo->IsAvoidAsyncCall() &&
         pQueryForProxyInfo->IsDontWantProxyStrings() &&
         _AutoProxyList &&
         _AutoProxyList->IsAutoProxy() &&
         _AutoProxyList->IsAutoProxyEnabled() )
    {
       pQueryForProxyInfo->SetUseProxy(TRUE);

       //if ( pQueryForProxyInfo->_tUrlProtocol == INTERNET_SCHEME_HTTPS )
       //{
       //    pQueryForProxyInfo->_tProxyScheme = INTERNET_SCHEME_HTTPS;
       //}
       //else
       //{
            pQueryForProxyInfo->_tProxyScheme = INTERNET_SCHEME_HTTP;

       //}

       goto quit;
    }


    //
    // Use normal Proxy infomation stored in the registry
    //

    error = PROXY_INFO::QueryProxySettings(&pQueryForProxyInfo);

    if ( error != ERROR_SUCCESS)
    {
        goto quit;
    }

quit:

    if ( error == ERROR_SUCCESS &&
          ( fNeedsGetNextProxy ||
            pQueryForProxyInfo->IsProxyEnumeration())  )
    {
        error = pQueryForProxyInfo->GetNextProxy(_BadProxyList);
    }

    if ( fLocked )
    {
        Unlock();
    }

fastquit:

    DEBUG_LEAVE(error);

    return error;
}


BOOL
PROXY_INFO_GLOBAL::HostBypassesProxy(
    IN INTERNET_SCHEME tScheme,
    IN LPSTR           lpszHostName,
    IN DWORD           cchHostName
    )

/*++

Routine Description:

    Determine if request should bypass proxy for host

Arguments:

    tScheme         -
    lpszHostName    -
    cchHostName     -

Return Value:

    BOOL

--*/

{
    BOOL bReturn = FALSE;
    DWORD dwServiceType;
    BOOL bTryByPassList = TRUE;

    // Only do this if it is for a scheme wininet supports.
    if (tScheme == INTERNET_SCHEME_HTTP ||
        tScheme == INTERNET_SCHEME_HTTPS ||
        tScheme == INTERNET_SCHEME_DEFAULT)
    {
        dwServiceType = INTERNET_SERVICE_HTTP;
    }
    else if (tScheme == INTERNET_SCHEME_FTP)
    {
        dwServiceType = INTERNET_SERVICE_FTP;
    }
    else if (tScheme == INTERNET_SCHEME_GOPHER)
    {
        dwServiceType = INTERNET_SERVICE_GOPHER;
    }
    else
    {
        return bReturn;
    }

    // LOCK
    Lock(FALSE);

    if (IsAutoProxy() && 
        _AutoProxyList->IsAutoProxyEnabled() &&
        _AutoProxyList->IsAutoProxyThreadReadyForRequests() &&
        ((_dwProxyFlags & PROXY_TYPE_AUTO_PROXY_URL) ||
         (_dwProxyFlags & PROXY_TYPE_AUTO_DETECT))
         )
    {
        CServerInfo * pServerInfo = NULL;

        DWORD error = ::GetServerInfo(lpszHostName,
                                      dwServiceType,
                                      FALSE,
                                      &pServerInfo
                                      );

        if (pServerInfo != NULL)
        {
            bTryByPassList = FALSE;

            if (pServerInfo->IsProxyByPassSet())
            {
                bReturn = pServerInfo->WasProxyByPassed();
            }
            else
            {
                // We have to know call the autoproxy code to determine if
                // the proxy is being bypassed.

                // First create an URL which corresponds to
                // scheme://hostname

                ICSTRING urlName;
                char hostName[INTERNET_MAX_HOST_NAME_LENGTH + 1];

                DWORD dwSchemeLength;
                LPSTR schemeName = MapUrlScheme(tScheme, &dwSchemeLength);

                int bufLength = dwSchemeLength
                             + 3              // For the ://
                             + cchHostName
                             + 1 ;            // Trailing NULL.

                urlName.CreateStringBuffer((LPVOID)schemeName, dwSchemeLength, bufLength);

                if (dwSchemeLength != 0)
                {
                    urlName += "://" ;
                }
                urlName.Strncat(lpszHostName, cchHostName);


                AUTO_PROXY_ASYNC_MSG proxyInfoQuery(tScheme,
                                               urlName.StringAddress(),
                                               hostName,
                                               sizeof(hostName));

                AUTO_PROXY_ASYNC_MSG *pProxyInfoQuery = &proxyInfoQuery;

                if ( _AutoProxyList->IsAutoProxyThreadReadyForRequests() ) {
                    proxyInfoQuery.SetBlockUntilCompletetion(TRUE);
                } else {
                    proxyInfoQuery.SetAvoidAsyncCall(TRUE);
                }

                Unlock();
                error = QueryProxySettings(&pProxyInfoQuery);
                // LOCK AGAIN
                Lock(FALSE);
                if (error == ERROR_SUCCESS)
                {
                    bReturn = !pProxyInfoQuery->IsUseProxy();
                    pServerInfo->SetProxyByPassed(bReturn);
                    if (pProxyInfoQuery && pProxyInfoQuery->IsAlloced())
                    {
                        delete pProxyInfoQuery;
                    }
                }
            }
            ::ReleaseServerInfo(pServerInfo);
        }
    }

    if (bTryByPassList) {        
        bReturn = IsHostInBypassList(lpszHostName, cchHostName);
    }

    Unlock();
    return bReturn;
}

//
// PROXY_STATE - an abstraction object used to provice simple string enumeration
//   given a list of proxies that need to be tested 
// 



BOOL
PROXY_STATE::GetNextProxy(
    IN  INTERNET_SCHEME   tUrlScheme,
    IN  BAD_PROXY_LIST &  BadProxyList,
    OUT LPINTERNET_SCHEME lptProxyScheme,
    OUT LPSTR * lplpszProxyHostName,
    OUT LPBOOL lpbFreeProxyHostName,
    OUT LPDWORD lpdwProxyHostNameLength,
    OUT LPINTERNET_PORT lpProxyHostPort
    )

/*++

Routine Description:

    Parses the current Proxy State information to acquire the
        proxy name, port, type to use.


Arguments:

    tUrlScheme   -  Scheme type, protocol that is being used.

    BadProxyList -  Reference to array of bad proxies that we can add/remove/check
                        from.

    lptProxyScheme - On output contains the correct proxy server type to use.
                        ex: if a SOCKS proxy is to be used to handle the FTP protocol,
                            this will be a INTERNET_SCHEME_SOCKS.

    lplpszProxyHostName - Pointer to allocated memory containing the address of
                           the proxy host name.

    lpbFreeProxyHostName - TRUE if *lplpszProxyHostName was allocated

    lpdwProxyHostNameLength - length of lplpszProxyHostName.

    lpProxyHostPort    - Host Port of Proxy.



Return Value:

    LPSTR
        Success - pointer to allocated buffer

        Failure - NULL

--*/

{
    LPSTR lpszDelimiter = NULL;
    BOOL  fReturn       = FALSE;
    LPSTR lpszPortDelim = NULL;
    LPSTR lpszPort      = NULL;


    if ( !_fIsMultiProxyList )
    {
        *lptProxyScheme = _tProxyScheme;
        *lplpszProxyHostName = _lpszAutoProxyList;
        *lpbFreeProxyHostName = FALSE;
        *lpdwProxyHostNameLength = _dwcbAutoProxyList;
        *lpProxyHostPort  = _proxyHostPort;
    }

    _fIsAnotherProxyAvail = FALSE;

    while ( *_lpszOffset != '\0' )
    {
        LPSTR lpszNewOffset ;

        //
        // Remove the delimiter so we can see the next token.
        //  ex: PROXY foo:80; DIRECT
        //  we would find DIRECT first with strstr, if we didn't
        //  delimit first.
        //

        lpszDelimiter = strchr(_lpszOffset, ';' );

        if ( lpszDelimiter == NULL )
        {
            lpszDelimiter = strchr(_lpszOffset, ',' );
        }

        if ( lpszDelimiter )
        {
            *lpszDelimiter = '\0';
        }

        lpszNewOffset=
            strstr(_lpszOffset, "DIRECT");

        if ( lpszNewOffset )
        {
            lpszNewOffset += sizeof("DIRECT");
            _lpszOffset    = lpszNewOffset;

            //
            // FALSE means direct.
            //

            fReturn = FALSE;
            goto quit;
        }

        //
        // Its not direct, try PROXY or SOCKS.
        //

        lpszNewOffset =
            strstr(_lpszOffset, "PROXY");


        if ( lpszNewOffset)
        {
            lpszNewOffset += sizeof("PROXY");
            *lpProxyHostPort = INTERNET_DEFAULT_HTTP_PORT;

            if ( tUrlScheme == INTERNET_SCHEME_HTTPS )
            {
                *lptProxyScheme = INTERNET_SCHEME_HTTPS;
            }
            else
            {
                *lptProxyScheme = INTERNET_SCHEME_HTTP;

            }
        }
        else
        {
            lpszNewOffset =
                strstr(_lpszOffset, "SOCKS");

            if ( lpszNewOffset )
            {
                lpszNewOffset   += sizeof("SOCKS");
                *lptProxyScheme  = INTERNET_SCHEME_SOCKS;
                *lpProxyHostPort = INTERNET_DEFAULT_SOCKS_PORT;
            }
        }

        //
        // Now do the generic common things for SOCKS, or PROXY
        // entries, ie: get port, hostname, and hostname size.
        //

        if ( lpszNewOffset )
        {
            _lpszOffset    = lpszNewOffset;

            SKIPWS(_lpszOffset);

            *lplpszProxyHostName = _lpszOffset;
            *lpbFreeProxyHostName = FALSE;

            lpszPortDelim = strchr(_lpszOffset, ':');

            if ( lpszPortDelim )
            {
                *lpszPortDelim = '\0';
                lpszPort  = lpszPortDelim+1;

                *lpProxyHostPort = (INTERNET_PORT)
                    atoi(lpszPort);
            }

            *lpdwProxyHostNameLength = lstrlen(_lpszOffset);

            if (BadProxyList.IsBadProxyName(*lplpszProxyHostName, *lpProxyHostPort))
            {
                if ( lpszDelimiter )
                {
                    _lpszOffset = (lpszDelimiter+1);
                }
                else
                {
                    _lpszOffset = _lpszAutoProxyList + _dwcbAutoProxyList;
                }

                continue;
            }

            fReturn = TRUE;
        }

        break;
    }

quit:

    //if ( lpszPortDelim )
    //{
    //    *lpszPortDelim = ':';
    //}

    if ( lpszDelimiter )
    {
        *lpszDelimiter = ';';

        _lpszOffset = (lpszDelimiter+1);
    }
    else
    {
        _lpszOffset = _lpszAutoProxyList + _dwcbAutoProxyList;
    }

    if ( fReturn )
    {
        _lpszLastProxyUsed = *lplpszProxyHostName;
        _LastProxyUsedPort = *lpProxyHostPort;

        //
        // If theres another possible proxy in this list,
        //   then we'll need to remember that
        //

        if ( _lpszOffset &&
             *_lpszOffset &&
                (strstr(_lpszOffset, "PROXY") ||
                 strstr(_lpszOffset, "DIRECT") ||  
                 strstr(_lpszOffset, "SOCKS"))
            )
        {
            _fIsAnotherProxyAvail = TRUE;
        }
    }

    return fReturn;
}




PRIVATE
LPSTR
GetRegistryProxyParameter(
    IN LPSTR lpszParameterName
    )

/*++

Routine Description:

    Reads a string from the registry into a buffer, then shrinks the buffer

Arguments:

    lpszParameterName   - name of string to retrieve

Return Value:

    LPSTR
        Success - pointer to allocated buffer

        Failure - NULL

--*/

{
    LPSTR buffer = NULL;
    DWORD length = PROXY_REGISTRY_STRING_LENGTH;
    BOOL done = FALSE;

    do {
        buffer = (LPSTR)ResizeBuffer(buffer, length, FALSE);
        if (done || (buffer == NULL)) {
            break;
        }

        DWORD error;

        error = InternetReadRegistryString(lpszParameterName, buffer, &length);
        length = (error == ERROR_SUCCESS) ? ((length == 0) ? 0 : (length + 1)) : 0;
        done = TRUE;
    } while (TRUE);

    return buffer;
}



//
// lame wrapper function for urlzones.
//

BOOLAPI IsHostInProxyBypassList (INTERNET_SCHEME tScheme, LPCSTR pszHost, DWORD cchHost)
{
    BOOL    fRet = FALSE;

    if (!GlobalDataInitialized) {
        GlobalDataInitialize();
    }

    if(ERROR_SUCCESS == LoadWinsock())
    {
        fRet = GlobalProxyInfo.HostBypassesProxy(tScheme, (LPSTR)pszHost, cchHost);
    }

    return fRet;
}
