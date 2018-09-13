//+---------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//   File:      ext.cxx
//
//  Contents:   Http Extension object implementation
//
//  Classes:    CHttpExtension
//
//  History:   03-Sep-98   tomfakes  Created
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_MSHTMSRV_HXX_
#define X_MSHTMSRV_HXX_
#include "mshtmsrv.hxx"
#endif

//+----------------------------------------------------------------------------
//
// Method: GetVersion
//
//+----------------------------------------------------------------------------
BOOL
CIISExtension::GetVersion(HSE_VERSION_INFO *pVer)
{
    BOOL        fRet = TRUE;

 	pVer->dwExtensionVersion = MAKELONG(HSE_VERSION_MAJOR, HSE_VERSION_MINOR);
	strcpy(pVer->lpszExtensionDesc, "MSHTMSRV 1.0");

    return fRet;
}


//+----------------------------------------------------------------------------
//
// Method: Terminate
//
//+----------------------------------------------------------------------------
BOOL
CIISExtension::Terminate(DWORD)
{
    Assert(g_pApp);
    g_pApp->ReleaseApp();

    return TRUE;
}


//+----------------------------------------------------------------------------
//
// Method: ExtensionProc
//
//+----------------------------------------------------------------------------
DWORD           
CIISExtension::ExtensionProc(LPEXTENSION_CONTROL_BLOCK pECB)
{
    DWORD       dwRet = HSE_STATUS_SUCCESS;

    // special request to dump the debug info
    if (lstrcmpA(pECB->lpszQueryString, "debug") == 0) 
    {
        DumpDebugInfo(pECB);
        goto Cleanup;
    }


Cleanup:
    return dwRet;
}


//+----------------------------------------------------------------------------
//
// Method: ReportError
//
//+----------------------------------------------------------------------------
void 
CIISExtension::ReportError(LPEXTENSION_CONTROL_BLOCK pECB, CHAR *errorText, CHAR *status) 
{
    if (status == NULL) {
        status = "500 Server Error";
    }

	(*pECB->ServerSupportFunction)(
		pECB->ConnID,
		HSE_REQ_SEND_RESPONSE_HEADER,
		status,
		NULL,
		(LPDWORD)"Content-Type: text/html\r\n\r\n"
		);

	static const char ErrorFormat[] = 
		"<HTML><HEAD><TITLE>MSHTML ISAPI Error</TITLE></HEAD><BODY>\r\n"
		"<H1>%s</H1><P><B>MSHTML ISAPI Error</B></P>\r\n"
		"<P>%s.</P>\r\n</BODY></HTML>";

	DWORD errorBufferSize = sizeof(ErrorFormat) + 256;
	CHAR errorBuffer[2048];
	wsprintfA(errorBuffer, ErrorFormat, status, errorText);
	DWORD errorBytes = strlen(errorBuffer);

	(*pECB->WriteClient)(
		pECB->ConnID,
		(LPVOID)errorBuffer,
		(LPDWORD)&errorBytes,
		0
		);
}


//+----------------------------------------------------------------------------
//
// Method: DumpDebugInfo
//
//+----------------------------------------------------------------------------
void 
CIISExtension::DumpDebugInfo(LPEXTENSION_CONTROL_BLOCK pECB) 
{
	(*pECB->ServerSupportFunction)(
		pECB->ConnID,
		HSE_REQ_SEND_RESPONSE_HEADER,
		"200 OK",
		NULL,
		(LPDWORD)"Content-Type: text/html\r\n\r\n"
		);

    char buf[256];

#define WRITEBUF()  { DWORD n = strlen(buf); \
                      (*pECB->WriteClient)(pECB->ConnID, buf, &n, 0); }

    // header

    wsprintfA(buf, "<HTML><HEAD><TITLE>MSHISAPI Statistics</TITLE></HEAD>\r\n");
    WRITEBUF();

    wsprintfA(buf, "<BODY><H2>MSHISAPI Statistics</H2>\r\n");
    WRITEBUF();

    // filter requests

    wsprintfA(buf, "<H3>Filter Requests:</H3><UL>\r\n");
    WRITEBUF();

    wsprintfA(buf, "<LI>Total number of requests: <B>%d</B>\r\n", 
                  g_pApp->GetIISFilter()->s_ulTotalRequests);
    WRITEBUF();

    wsprintfA(buf, "<LI>Number of requests resolved from cache: <B>%d</B>\r\n", 
                  g_pApp->GetIISFilter()->s_ulRequestsFoundInCache);
    WRITEBUF();

    wsprintfA(buf, "<LI>Number of requests passed to Trident: <B>%d</B>\r\n", 
                  g_pApp->GetIISFilter()->s_ulRequestsToTrident);
    WRITEBUF();

    wsprintfA(buf, "<LI>Number of requests passed through (Client is IE5): <B>%d</B>\r\n", 
                  g_pApp->GetIISFilter()->s_ulTotalRequests - 
                    (g_pApp->GetIISFilter()->s_ulRequestsFoundInCache + 
                     g_pApp->GetIISFilter()->s_ulRequestsToTrident));
    WRITEBUF();

    wsprintfA(buf, "</UL>\r\n\r\n");
    WRITEBUF();

    // tail

    wsprintfA(buf, "\r\n\r\n</BODY></HTML>");
    WRITEBUF();

#undef WRITEBUF
}
