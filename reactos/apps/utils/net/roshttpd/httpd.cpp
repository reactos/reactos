/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS HTTP Daemon
 * FILE:        httpd.cpp
 * PURPOSE:     HTTP daemon
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH  01/09/2000 Created
 */
#include <debug.h>
#include <new>
#include <malloc.h>
#include <string.h>
#include <config.h>
#include <httpd.h>
#include <error.h>

using namespace std;

CHAR HttpMsg400[] = "<HEAD><TITLE>400 Bad Request</TITLE></HEAD>\n\r<BODY><H1>400 Bad Request</H1>\n\rThe request had bad syntax.<BR>\n\r</BODY>\n\r\n\r";
CHAR HttpMsg404[] = "<HEAD><TITLE>404 Not Found</TITLE></HEAD>\n\r<BODY><H1>404 Not Found</H1>\n\rThe requested URL was not found on this server.<BR>\n\r</BODY>\n\r\n\r";
CHAR HttpMsg405[] = "<HEAD><TITLE>405 Method Not Allowed</TITLE></HEAD>\n\r<BODY><H1>405 Method Not Allowed</H1>\n\rThe requested method is not supported on this server.<BR>\n\r</BODY>\n\r\n\r";
CHAR HttpMsg500[] = "<HEAD><TITLE>500 Internal Server Error</TITLE></HEAD>\n\r<BODY><H1>500 Internal Server Error</H1>\n\rAn internal error occurred.<BR>\n\r</BODY>\n\r\n\r";
CHAR HttpMsg501[] = "<HEAD><TITLE>501 Not Implemented</TITLE></HEAD>\n\r<BODY><H1>501 Not Implemented</H1>\n\rThe feature is not implemented.<BR>\n\r</BODY>\n\r\n\r";


// *************************** CHttpClient ***************************

// Default constructor
CHttpClient::CHttpClient()
{
}

// Constructor with server socket as starter value
CHttpClient::CHttpClient(CServerSocket *serversocket)
{
	ServerSocket = serversocket;
}

// Split URIs into its parts (ie. |http://|www.host.com|/resource|?parameters|)
VOID CHttpClient::SplitUri(LPSTR lpsUri, LPSTR lpsHost, LPSTR lpsResource, LPSTR lpsParams)
{
    LPSTR lpsPos;
	LPSTR lpsStr;
    UINT i;

	strcpy(lpsHost, "");
	strcpy(lpsResource, "");
	strcpy(lpsParams, "");

    lpsPos = strstr(lpsUri, "://");
    if (lpsPos != NULL)
        lpsStr = &lpsPos[3];
	else
		lpsStr = lpsUri;
        
    lpsPos = strstr(lpsStr, "/");
    if (lpsPos != NULL) {
        strncat(lpsHost, lpsPos, lpsPos - lpsStr);
        lpsStr = &lpsPos[1];

        lpsPos = strstr(lpsStr, "?");
        if (lpsPos != NULL) {
            strncat(lpsResource, lpsStr, lpsPos - lpsStr);
            strcpy(lpsParams, &lpsPos[1]);
        } else {
            strcpy(lpsResource, lpsStr);
            strcpy(lpsParams, "");
        }

        // Replace "/" with "\"
		for (i = 0; i < strlen(lpsResource); i++) {
            if (lpsResource[i] == '/')
                lpsResource[i] = '\\';
        }
    }
}

// Split resource into its parts (ie. |/path/|filename|.extension|)
VOID CHttpClient::SplitResource(LPSTR lpsResource, LPSTR lpsPath, LPSTR lpsFilename, LPSTR lpsExtension)
{
    INT i,len,fileptr,extptr;
	
	strcpy(lpsPath, "");
	strcpy(lpsFilename, "");
	strcpy(lpsExtension, "");
	
	len = strlen(lpsResource);
	if (len != 0) {
		if (lpsResource[len - 1] == '/') {
			// There is only a path
			strcpy(lpsPath, lpsResource);
		} else {
			// Find extension
			i = len - 1;
			while ((i >= 0) && (lpsResource[i] != '.')) i--;
			extptr = i;
			while ((i >= 0) && (lpsResource[i] != '/')) i--;
			if (i > 0) {
				// There is at least one directory in the path (besides root directory)
				fileptr = i + 1;
				strncat(lpsPath, lpsResource, fileptr);
			} else
				fileptr = 1;
			
			// Get filename and possibly extension
			if (extptr != 0) {
				strncat(lpsFilename, &lpsResource[fileptr], extptr - fileptr);
				// Get extension
				strncat(lpsExtension, &lpsResource[extptr + 1], len - extptr - 1);
			} else
				strncat(lpsFilename, &lpsResource[fileptr], len - fileptr);
		}
	}
}

// Process HTTP request
VOID CHttpClient::ProcessRequest()
{
    CHAR sStr[255];
	CHAR sHost[255];
    CHAR sResource[255];
    CHAR sParams[255];

    // Which method?
    switch (Parser.nMethodNo) {
		case hmGET: {
			SplitUri(Parser.sUri, sHost, sResource, sParams);
			
			// Default resource?
			if (strlen(sResource) == 0) {
				CIterator<LPSTR> *i = pConfiguration->GetDefaultResources()->CreateIterator();

				// FIXME: All default resources should be tried
				// Iterate through all strings
				//for (i->First(); !i->IsDone(); i->Next())
				i->First();
				if (!i->IsDone()) {
					strcat(sResource, i->CurrentItem());
					delete i;
				} else {
					// File not found
					Report("404 Not Found", HttpMsg404);
					break;
				}
			}
			strcpy(sStr, pConfiguration->GetHttpBase());
	        strcat(sStr, sResource);
			SendFile(sStr);
			break;
		}
		default: {
			// Method is not implemented
			Report("501 Not Implemented", HttpMsg501);
		}
	}
}

// Send a file to socket
VOID CHttpClient::SendFile(LPSTR lpsFilename)
{
    CHAR str[255];
    CHAR str2[32];
    union BigNum {
        unsigned __int64 Big;
        struct {
            DWORD Low;
            DWORD High; 
        };
    } nTotalBytes;
	DWORD nBytesToRead;
	DWORD nBytesRead;
	BOOL bStatus;

	// Try to open file
    hFile = CreateFileA(lpsFilename,
        GENERIC_READ,               // Open for reading 
        FILE_SHARE_READ,            // Share for reading 
        NULL,                       // No security 
        OPEN_EXISTING,              // Existing file only 
        FILE_ATTRIBUTE_NORMAL,      // Normal file 
        NULL);                      // No attr. template 
    if (hFile == INVALID_HANDLE_VALUE) { 
        // File not found
        Report("404 Not Found", HttpMsg404);
        return; 
    }
    // Get file size
    nTotalBytes.Low = GetFileSize(hFile, &nTotalBytes.High);
    if ((nTotalBytes.Low == 0xFFFFFFFF) && ((GetLastError()) != NO_ERROR)) {
        // Internal server error
		Report("500 Internal Server Error", HttpMsg500);
		// Close file
		CloseHandle(hFile);
        return;
    }

	// Determine buffer size
	if (nTotalBytes.Big < 65536)
		nBufferSize = 1024;
	else
		nBufferSize = 32768;
	// Allocate memory on heap
	lpsBuffer = (PCHAR) malloc(nBufferSize);

	if (lpsBuffer == NULL) {
		// Internal server error
		Report("500 Internal Server Error", HttpMsg500);
		// Close file
		CloseHandle(hFile);
		return;
	}

	SendText("HTTP/1.1 200 OK");
    SendText("Server: ROSHTTPD");
    SendText("MIME-version: 1.0");
    SendText("Content-Type: text/plain");
    SendText("Accept-Ranges: bytes");
    strcpy(str, "Content-Length: ");
    _itoa(nTotalBytes.Low, str2, 10);
    strcat(str, str2);
    SendText(str);
    SendText("");
	// Read and transmit file
	nTotalRead = 0;
	nFileSize = nTotalBytes.Big;
	bStop = FALSE;

	fd_set wfds;
	FD_ZERO(&wfds);
	FD_SET(Socket, &wfds);
	do {
		MessageLoop();

		if (nTotalRead + nBufferSize < nFileSize)
			nBytesToRead = nBufferSize;
		else nBytesToRead = nFileSize - nTotalRead;

		bStatus = ReadFile(hFile, lpsBuffer, nBytesToRead, &nBytesRead, NULL);
		if (bStatus) {
			select(0, NULL, &wfds, NULL, NULL);
			bStatus = (Transmit(lpsBuffer, nBytesRead) == (INT)nBytesRead);
			nTotalRead += nBytesRead;
		}
    } while ((!bStop) && (bStatus) && (nTotalRead < nFileSize));

	if (bStatus)
		SendText("");
	else
		// We can't send an error message here as we are in the process of sending a file.
		// We have to terminate the connection instead
		Close();
	
	// Free allocated memory
	free(lpsBuffer);

	// Close file
    CloseHandle(hFile);
}

// Report something to client
VOID CHttpClient::Report(LPSTR lpsCode, LPSTR lpsStr)
{
    CHAR sTmp[128];
    CHAR sTmp2[16];
	
    strcpy(sTmp, "HTTP/1.1 ");
    strcat(sTmp, lpsCode);
    SendText(sTmp);
    SendText("Server: ROSHTTPD");
    SendText("MIME-version: 1.0");
    SendText("Content-Type: text/html");
    SendText("Accept-Ranges: bytes");
    strcpy(sTmp, "Content-Length: ");
    if (lpsStr != NULL) {
        _itoa(strlen(lpsStr), sTmp2, 10);
        strcat(sTmp,  sTmp2);
    } else
        strcat(sTmp, "0");
    SendText(sTmp);
    SendText("");
    if (lpsStr != NULL)
        SendText(lpsStr);
    SendText("");
}

// OnRead event handler
VOID CHttpClient::OnRead()
{
	LONG nCount;

	nCount = Receive((LPSTR) &Parser.sBuffer[Parser.nHead],
        sizeof(Parser.sBuffer) - Parser.nHead);

    Parser.nHead += nCount;
	if (Parser.nHead >= sizeof(Parser.sBuffer))
		Parser.nHead = 0;

    if (Parser.Complete()) {
		ProcessRequest();
    }

	if (Parser.bUnknownMethod) {
		// Method Not Allowed
		Report("405 Method Not Allowed", HttpMsg405);
		// Terminate connection
		Close();
	}
}
/*
// OnWrite event handler
VOID CHttpClient::OnWrite()
{
	DWORD nBytesToRead;
	DWORD nBytesRead;

	OutputDebugString(_T("Can write\n"));

	if (bSendingFile) {
		if (nTotalRead + nBufferSize < nFileSize)
			nBytesToRead = nBufferSize;
		else nBytesToRead = nFileSize - nTotalRead;

		bError = ReadFile(hFile, Buffer, nBytesToRead, &nBytesRead, NULL);
		if (!bError) {
			Transmit(Buffer, nBytesRead);
			nTotalRead += nBytesRead;
		}
	}
}
*/
// OnClose event handler
VOID CHttpClient::OnClose()
{
	// Stop sending file if we are doing that now
	bStop = TRUE;
}


// ************************ CHttpClientThread ************************

// Constructor with client socket as starter value
CHttpClientThread::CHttpClientThread(LPCServerClientSocket lpSocket)
{
	ClientSocket = lpSocket;
}

// Execute client thread code
VOID CHttpClientThread::Execute()
{
    MSG Msg;

	while (!Terminated()) {
		((  CHttpClient *) ClientSocket)->MessageLoop();
        if (PeekMessage(&Msg, 0, 0, 0, PM_REMOVE) != 0) {
            switch (Msg.message) {
                case HTTPD_START: {
					// TODO: Start thread
                    break;
                }
                case HTTPD_STOP: {
					// TODO: Stop thread
                    break;
                }
                default:
                    DispatchMessage(&Msg);
            }

        }
    }

	if (ClientSocket != NULL) {
		delete ClientSocket;
		ClientSocket = NULL;
	}
}


// *************************** CHttpDaemon ***************************

// Default constructor
CHttpDaemon::CHttpDaemon()
{
    State = hsStopped;
	Start();
}

// Default destructor
CHttpDaemon::~CHttpDaemon()
{
	if (State==hsRunning)
		Stop();
}

// Return daemon state
HTTPdState CHttpDaemon::GetState() const
{
	return State;
}

// Start HTTP daemon
BOOL CHttpDaemon::Start()
{
	assert(State==hsStopped);

	SetPort(pConfiguration->GetPort());

	Open();
	
	State = hsRunning;
	
    return TRUE;
}

// Stop HTTP daemon
BOOL CHttpDaemon::Stop()
{
	assert(State==hsRunning);

	Close();

	State = hsStopped;

    return TRUE;
}

// OnGetSocket event handler
LPCServerClientSocket CHttpDaemon::OnGetSocket(LPCServerSocket lpServerSocket)
{
	return new CHttpClient(lpServerSocket);
}

// OnGetThread event handler
LPCServerClientThread CHttpDaemon::OnGetThread(LPCServerClientSocket lpSocket)
{
	return new CHttpClientThread(lpSocket);
}

// OnAccept event handler
VOID CHttpDaemon::OnAccept(LPCServerClientThread lpThread)
{
}


// ************************ CHttpDaemonThread ************************

// Execute daemon thread code
VOID CHttpDaemonThread::Execute()
{
	MSG Msg;
	
	try {
		Daemon = NULL;
		Daemon = new CHttpDaemon;

		while (!Terminated()) {
			Daemon->MessageLoop();
			if (PeekMessage(&Msg, 0, 0, 0, PM_REMOVE) != 0) {
	            switch (Msg.message) {
		            case HTTPD_START: {
			            if (Daemon->GetState() == hsStopped)
				            Daemon->Start();
					    break;
					}
					case HTTPD_STOP: {
	                    if (Daemon->GetState() == hsRunning)
							Daemon->Stop();
						break;
					}
					case HTTPD_SUSPEND: {
						if (Daemon->GetState() == hsRunning){}
							// FIXME: Suspend service
						break;
					}
					case HTTPD_RESUME: {
						if (Daemon->GetState() != hsSuspended){}
							// FIXME: Resume service
						break;
					}
					default:
                        DispatchMessage(&Msg);
				}
			}
	    }
		delete Daemon;
	} catch (ESocket e) {
		ReportErrorStr(e.what());
	} catch	(bad_alloc e) {
		ReportErrorStr(TS("Insufficient resources."));
	}
}
