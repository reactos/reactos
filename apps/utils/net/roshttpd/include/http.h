/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS HTTP Daemon
 * FILE:        include/http.h
 */
#ifndef __HTTP_H
#define __HTTP_H

#include <windows.h>

// Generel HTTP related constants
#define NUMMETHODS 7
#define NUMGENERELS 7
#define NUMREQUESTS 17
#define NUMENTITIES 12

// HTTP method constants
#define hmOPTIONS	0
#define hmGET		1
#define hmHEAD		2
#define hmPOST		3
#define hmPUT		4
#define hmDELETE	5
#define hmTRACE		6

class CHttpParser {
public:
    CHAR sBuffer[2048];
    UINT nHead;
    UINT nTail;
    CHAR sUri[255];
    CHAR sVersion[15];
    CHAR sHeader[63];
    CHAR sMethod[63];
    UINT nMethodNo;
    BOOL bUnknownMethod;
    BOOL bBadRequest;
    CHttpParser();
    ~CHttpParser();
    BOOL Complete();
    BOOL Parse();
private:
    BOOL ReadChar(LPSTR lpsStr);
    BOOL PeekChar(LPSTR lpsStr);
    BOOL ReadString(LPSTR lpsStr, UINT nLength);
    BOOL ReadSpecial(LPSTR lpStr, UINT nLength);
    VOID Skip(CHAR sStr);
    BOOL Expect(CHAR sStr);
    BOOL ExpectCRLF();
    BOOL RequestLine();
    BOOL GenerelHeader();
    BOOL RequestHeader();
    BOOL EntityHeader();
    BOOL MessageBody();
};

#endif /* __HTTP_H */
