/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS HTTP Daemon
 * FILE:        http.cpp
 * PURPOSE:     HTTP 1.1 parser engine
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH  01/09/2000 Created
 * TODO:        - Implement message-body
 *              - Implement more generel-header entries
 *              - Implement more request-header entries
 *              - Implement more entity-header entries
 */
#include <debug.h>
#include <iostream.h>
#include <string.h>
#include <http.h>

CHAR MethodTable[NUMMETHODS][8] = {"OPTIONS", "GET", "HEAD", "POST", "PUT", 
    "DELETE", "TRACE"};

CHAR GenerelTable[NUMGENERELS][18] = {"Cache-Control", "Connection", "Date", "Pragma", 
    "Transfer-Encoding", "Upgrade", "Via"};

CHAR RequestTable[NUMREQUESTS][20] = {"Accept", "Accept-Charset", "Accept-Encoding",
    "Accept-Language", "Authorization", "From", "Host", "If-Modified-Since", "If-Match",
    "If-None-Match", "If-Range", "If-Unmodified-Since", "Max-Forwards",
    "Proxy-Authorization", "Range", "Referer", "User-Agent"};

CHAR EntityTable[NUMENTITIES][17] = {"Allow", "Content-Base", "Content-Encoding",
    "Content-Language", "Content-Length", "Content-Location", "Content-MD5",
    "Content-Range", "Content-Type", "ETag", "Expires", "Last-Modified"};

// *************************** CHttpParser ***************************

// Default constructor
CHttpParser::CHttpParser()
{
    nHead = 0;
    nTail = 0;
}

// Default destructor
CHttpParser::~CHttpParser()
{
}

// Returns TRUE if a complete HTTP message is in buffer
BOOL CHttpParser::Complete()
{
    UINT nTmp;

    /*DPRINT("--1:-%d---\n", sBuffer[nHead-2]);
    DPRINT("--2:-%d---\n", sBuffer[nHead-1]);

    sBuffer[nHead] = '!';
    sBuffer[nHead+1] = 0;
    DPRINT("Examining buffer: (Head: %d, Tail: %d)\n", nHead, nTail);
    DPRINT("%s\n", (LPSTR)&sBuffer[nTail]);*/

    nTmp = nTail;
    if (!Parse()) {
        if (!bUnknownMethod)
            nTail = nTmp;
        return FALSE;
    } else
        return TRUE;
}


// Read a character from buffer
BOOL CHttpParser::ReadChar(LPSTR lpsStr)
{
    if (nTail <= nHead) {
        if (nTail != nHead) {
            lpsStr[0] = sBuffer[nTail];
            nTail++;
            return TRUE;
        } else {
            lpsStr[0] = 0;
            return FALSE;
        }
    } else {
        if (nTail == sizeof(sBuffer))
            nTail = 0;
        if (nTail != nHead) {
            lpsStr[0] = sBuffer[nTail];
            nTail++;
            return TRUE;
        } else {
            lpsStr[0] = 0;
            return FALSE;
        }
    }
}

// Peek at a character in the buffer
BOOL CHttpParser::PeekChar(LPSTR lpsStr)
{
    UINT nFakeTail;
    
    if (nTail == sizeof(sBuffer))
        nFakeTail = 0;
    else
        nFakeTail = nTail;
    if (nFakeTail != nHead) {
        lpsStr[0] = sBuffer[nFakeTail];
        return TRUE;
    } else {
        lpsStr[0] = 0;
        return FALSE;
    }
}

// Read a string from buffer. Only A-Z, a-z, 0-9 and '-' are valid characters
BOOL CHttpParser::ReadString(LPSTR lpsStr, UINT nLength)
{
    UINT i = 0;
    CHAR sTmp;
    
    while (PeekChar(&sTmp)) {
        if (((sTmp >= 'A') && (sTmp <= 'Z')) || ((sTmp >= 'a') && (sTmp <= 'z')) ||
            ((sTmp >= '0') && (sTmp <= '9')) || (sTmp == '-')) { 
            if (i >= (nLength - 1)) {
                lpsStr[0] = 0;
                return FALSE;
            }
            ReadChar(&sTmp);
            lpsStr[i] = sTmp;
            i++;
        } else {
            lpsStr[i] = 0;
            return TRUE;
        }
    }
    lpsStr[0] = 0;
    return FALSE;
}

// Read a string from buffer. Stop if SP or CR is found or when there are no more 
// characters
BOOL CHttpParser::ReadSpecial(LPSTR lpsStr, UINT nLength)
{
    UINT i = 0;
    CHAR sTmp;
 
    while (PeekChar(&sTmp) && (sTmp != ' ') && (sTmp != 13)) {
        if (i >= (nLength - 1)) {
            lpsStr[nLength - 1] = 0;
            return FALSE;
        }
        ReadChar(&sTmp);
        lpsStr[i] = sTmp;
        i++;
    }
    lpsStr[i] = 0;
    return TRUE;
}

// Skip until "sCh" is found
VOID CHttpParser::Skip(CHAR sCh)
{
    CHAR sTmp;

    while (PeekChar(&sTmp) && (sTmp != sCh))
        ReadChar(&sTmp);
}

// Return TRUE if sCh is the next character
BOOL CHttpParser::Expect(CHAR sCh)
{
    CHAR sTmp;

    if (PeekChar(&sTmp)) {
        if (sTmp == sCh) {
            ReadChar(&sTmp);
            return TRUE;
        }
    }
    return FALSE;
}

// Return TRUE if CRLF are the next characters
BOOL CHttpParser::ExpectCRLF()
{
    return (Expect(13) && Expect(10));
}

// Request = RequestLine | *( GenerelHeader | RequestHeader | EntityHeader ) 
//           CRLF [ MessageBody ]
BOOL CHttpParser::Parse()
{
    BOOL bStatus;


    CHAR ch;

    if (RequestLine()) {
        do {
            if (!ReadString(sHeader, sizeof(sHeader)))
                break;
            bStatus = (GenerelHeader());
            bStatus = (RequestHeader() || bStatus);
            bStatus = (EntityHeader() || bStatus);
        } while (bStatus);
        // CRLF
        if (!ExpectCRLF())
            return FALSE;
        MessageBody();
        return TRUE;
    }
    return FALSE;
}

// RequestLine = Method SP RequestURI SP HTTP-Version CRLF
BOOL CHttpParser::RequestLine()
{
    CHAR sCh;
    UINT i;

    bUnknownMethod = FALSE;

    // RFC 2068 states that servers SHOULD ignore any empty nine(s) received where a 
    // Request-Line is expected
    while (PeekChar(&sCh) && ((sCh == 13) || (sCh == 10)));
    
    if (!ReadString(sMethod, sizeof(sMethod)))
        return FALSE;

    for (i = 0; i < NUMMETHODS; i++) {
        if (strcmp(MethodTable[i], sMethod) == 0) {
            nMethodNo = i;
            if (!Expect(' '))
                return FALSE;
            // URI (ie. host/directory/resource)
            if (!ReadSpecial(sUri, sizeof(sUri)))
                return FALSE;
            if (!Expect(' '))
                return FALSE;
            // HTTP version (eg. HTTP/1.1)
            if (!ReadSpecial(sVersion, sizeof(sVersion)))
                return FALSE;
            // CRLF
            if (!ExpectCRLF())
                return FALSE;

            return TRUE;
        }
    }
    bUnknownMethod = TRUE;
    return FALSE;
}

// GenerelHeader = Cache-Control | Connection | Date | Pragma | Transfer-Encoding | 
//                 Upgrade | Via
BOOL CHttpParser::GenerelHeader()
{
    INT i;

    for (i = 0; i < NUMGENERELS; i++) {
        if (strcmp(GenerelTable[i], sHeader) == 0) {
            switch (i) {
                case 1: {
                    //Connection
                    Expect(':');
                    Expect(' ');
                    Skip(13);
                    ExpectCRLF();
                    break;
                }
                default: {
                    Expect(':');
                    Expect(' ');
                    Skip(13);
                    ExpectCRLF();
                }
            }
            return TRUE;
        }
    }
    return FALSE;
}

// RequestHeader = Accept | Accept-Charset | Accept-Encoding | Accept-Language |
//                 Authorization | From | Host | If-Modified-Since | If-Match |
//                 If-None-Match | If-Range | If-Unmodified-Since | Max-Forwards |
//                 Proxy-Authorization | Range | Referer | User-Agent
BOOL CHttpParser::RequestHeader()
{
    INT i;

    for (i = 0; i < NUMREQUESTS; i++) {
        if (strcmp(RequestTable[i], sHeader) == 0) {
            switch (i) {
                case 0: {
                    //Accept
                    Expect(':');
                    Expect(' ');
                    Skip(13);
                    ExpectCRLF();
                    break;
                }
                case 2: {
                    //Accept-Encoding
                    Expect(':');
                    Expect(' ');
                    Skip(13);
                    ExpectCRLF();
                    break;
                }
                case 3: {
                    //Accept-Language
                    Expect(':');
                    Expect(' ');
                    Skip(13);
                    ExpectCRLF();
                    break;
                }
                case 6: {
                    //Host
                    Expect(':');
                    Expect(' ');
                    Skip(13);
                    ExpectCRLF();
                    break;
                }
                case 16: {
                    //User-Agent
                    Expect(':');
                    Expect(' ');
                    Skip(13);
                    ExpectCRLF();
                    break;
                }
                default: {
                    Expect(':');
                    Expect(' ');
                    Skip(13);
                    ExpectCRLF();
                    return TRUE;
                }
            }
            return TRUE;
        }
    }
    return FALSE;
}

// EntityHeader = Allow | Content-Base | Content-Encoding | Content-Language |
//                Content-Length | Content-Location | Content-MD5 |
//                Content-Range | Content-Type | ETag | Expires |
//                Last-Modified | extension-header
BOOL CHttpParser::EntityHeader()
{
    INT i;

    for (i = 0; i < NUMENTITIES; i++) {
        if (strcmp(EntityTable[i], sHeader) == 0) {
            switch (i) {
                case 0: 
                default: {
                    //cout << "<Entity-Header>: #" << i << endl; 
                    Expect(':');
                    Expect(' ');
                    Skip(13);
                    ExpectCRLF();
                    return TRUE;
                }
            }
            return FALSE;
        }
    }
    return FALSE;
}

// MessageBody = *OCTET
BOOL CHttpParser::MessageBody()
{
    return FALSE;
}
