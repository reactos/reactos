/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS HTTP Daemon
 * FILE:        socket.cpp
 * PURPOSE:     Socket classes
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH  01/09/2000 Created
 */
#include <error.h>
#include <socket.h>
#include <iterator.h>

// ***************************** CSocket *****************************

// Default constructor
CSocket::CSocket()
{
    Active = FALSE;
	Event  = WSA_INVALID_EVENT;
	Events = 0;
	Socket = INVALID_SOCKET;

	// INET address family
	SockAddrIn.sin_family = AF_INET;

    // Any address will do
    SockAddrIn.sin_addr.s_addr = INADDR_ANY;

    // Convert to network ordering 
    SockAddrIn.sin_port = htons(0);
}

// Default destructor
CSocket::~CSocket()
{
}

// Return winsock socket handle
SOCKET CSocket::GetSocket()
{
	return Socket;
}

// Set winsock socket handle
VOID CSocket::SetSocket(SOCKET socket)
{
	Socket = socket;
}
	

// Return socket address
SOCKADDR_IN CSocket::GetSockAddrIn()
{
	return SockAddrIn;
}

// Set socket address
VOID CSocket::SetSockAddrIn(SOCKADDR_IN sockaddrin)
{
	SockAddrIn = sockaddrin;
}

// Associate winsock events with socket
VOID CSocket::SetEvents(LONG lEvents)
{
	if (Event == WSA_INVALID_EVENT) {
		// Create socket event
		Event = WSACreateEvent();
		if (Event == WSA_INVALID_EVENT)
			throw ESocketOpen(TS("Unable to create event."));
	}
	
	if (lEvents != Events) {
		// Associate network events with socket
		if (WSAEventSelect(Socket, Event, lEvents) == SOCKET_ERROR)
			throw ESocketOpen(TS("Unable to select socket events."));
		Events = lEvents;
	}
}

// Return associated winsock events
LONG CSocket::GetEvents()
{
	return Events;
}

// Open socket
VOID CSocket::Open()
{
}

// Close socket
VOID CSocket::Close()
{
}


// *********************** CServerClientSocket ***********************

// Constructor with serversocket as parameter
CServerClientSocket::CServerClientSocket(LPCServerSocket lpServerSocket)
{
	ServerSocket = lpServerSocket;
}

// Transmit data to socket
INT CServerClientSocket::Transmit( LPSTR lpsBuffer, UINT nLength)
{
    return send(Socket, lpsBuffer, nLength, 0);
}

// Send a string to socket
INT CServerClientSocket::SendText( LPSTR lpsText)
{
    static CHAR crlf[3] = {0x0D, 0x0A, 0x00};
	INT nCount;

    nCount = Transmit(lpsText, strlen(lpsText));
	nCount += Transmit(crlf, strlen(crlf));
	return nCount;
}

// Receive data from socket
INT CServerClientSocket::Receive(LPSTR lpsBuffer, UINT nLength)
{
	return recv(Socket, lpsBuffer, nLength, 0);
}

// Process winsock messages if any
VOID CServerClientSocket::MessageLoop()
{
	UINT nStatus;
    WSANETWORKEVENTS NetworkEvents;

    nStatus = WSAWaitForMultipleEvents(1, &Event, FALSE, 0, FALSE);
    if ((nStatus == 0) && (WSAEnumNetworkEvents(Socket, Event, &NetworkEvents) != SOCKET_ERROR)) {
        if ((NetworkEvents.lNetworkEvents & FD_READ) != 0) {
			OnRead();
		} 
        if ((NetworkEvents.lNetworkEvents & FD_CLOSE) != 0) {
			OnClose();
        }
    }
}

// Return server socket that own this socket
LPCServerSocket CServerClientSocket::GetServerSocket()
{
	return ServerSocket;
}


// *********************** CServerClientThread ***********************

CServerClientThread::CServerClientThread(LPCServerClientSocket lpSocket)
{
	ClientSocket = lpSocket;
}

CServerClientThread::~CServerClientThread()
{
	ClientSocket->GetServerSocket()->RemoveClient((LPCServerClientThread) this);
}


// ************************** CServerSocket **************************	

// Default constructor
CServerSocket::CServerSocket()
{
}

// Default destructor
CServerSocket::~CServerSocket()
{
	if (Active)
		Close();
}

// Open server socket so clients can connect
VOID CServerSocket::Open()
{
	assert(!Active);
	
	// Convert to network ordering 
	SockAddrIn.sin_port = htons(Port);

	if (Socket == INVALID_SOCKET) {
		// Create socket
		Socket = socket(AF_INET, SOCK_STREAM, 0);
		if (Socket == INVALID_SOCKET)
	        throw ESocketOpen(TS("Unable to allocate a socket."));
	}

	// Associate an address with server socket
	if (bind(Socket, (struct sockaddr FAR *) &SockAddrIn, sizeof(SockAddrIn)) == SOCKET_ERROR)
		throw ESocketOpen(TS("Unable to associate address with socket."));

	// Listen for incoming connections
	if (listen(Socket, MAX_PENDING_CONNECTS) != 0)
		throw ESocketOpen(TS("Unable to listen on socket."));

	// Associate network events with socket
	SetEvents(FD_ACCEPT | FD_CONNECT | FD_CLOSE);

	Active = TRUE;
}

// Close server socket and all current connections
VOID CServerSocket::Close()
{
	assert(Active);

	if (Event != WSA_INVALID_EVENT) {
		// Tell winsock not to notify us about any events
		if (WSAEventSelect(Socket, Event, 0) == SOCKET_ERROR)
			throw ESocketClose(TS("Unable to select socket events."));

		if (!WSACloseEvent(Event))
			throw ESocketClose(TS("Unable to close socket event."));
		Event = WSA_INVALID_EVENT;
	}
	
	CIterator<LPCServerClientThread> *i = Connections.CreateIterator();

	// Terminate and free all client threads
	for (i->First(); !i->IsDone(); i->Next()) {
		//i->CurrentItem()->Terminate();
		delete i->CurrentItem();
	}
	delete i;
	Connections.RemoveAll();

	closesocket(Socket);
	Socket = INVALID_SOCKET;

	Active = FALSE;
}

// Set port number to listen on
VOID CServerSocket::SetPort(UINT nPort)
{
	assert(!Active);

	Port = nPort;
}

// Process messages from winsock if any
VOID CServerSocket::MessageLoop()
{
    UINT nStatus;
	INT nAddrLen;
	SOCKET ClientSocket;
	SOCKADDR_IN SockAddrIn;
    WSANETWORKEVENTS NetworkEvents;
	LPCServerClientSocket lpClient;
	LPCServerClientThread lpThread;
	
    nStatus = WSAWaitForMultipleEvents(1, &Event, FALSE, 0, FALSE);
    if ((nStatus == 0) && (WSAEnumNetworkEvents(Socket, Event, &NetworkEvents) != SOCKET_ERROR)) {
        if ((NetworkEvents.lNetworkEvents & FD_ACCEPT) != 0) {
			lpClient = OnGetSocket(this);
			nAddrLen = sizeof(SockAddrIn);
			ClientSocket = accept(Socket, (SOCKADDR *) &SockAddrIn, &nAddrLen);
			if (ClientSocket != INVALID_SOCKET) {
				// Set socket handle
				lpClient->SetSocket(ClientSocket);
				// Set socket address
				lpClient->SetSockAddrIn(SockAddrIn);
				// Set winsock events
				lpClient->SetEvents(FD_READ | FD_CLOSE);
				// Create client connection thread
				lpThread = OnGetThread(lpClient);
				// Add client thread to connection list
				InsertClient(lpThread);
				// Call OnAccept event handler
				OnAccept(lpThread);
			} else {
				delete lpClient;
				lpClient = NULL;
				throw ESocketOpen(TS("No more sockets available."));
			}
		}
        /*if ((NetworkEvents.lNetworkEvents & FD_CONNECT) != 0) {
        }
        if ((NetworkEvents.lNetworkEvents & FD_CLOSE) != 0) {
        }*/
    }
}

// Insert client into connection list
VOID CServerSocket::InsertClient(LPCServerClientThread lpClient)
{
	Connections.Insert(lpClient);
}

// Remove client from connection list
VOID CServerSocket::RemoveClient(LPCServerClientThread lpClient)
{
	Connections.Remove(lpClient);
}

// OnGetSocket event handler
LPCServerClientSocket CServerSocket::OnGetSocket(LPCServerSocket lpServerSocket)
{
	return NULL;
}

// OnGetThread event handler
LPCServerClientThread CServerSocket::OnGetThread(LPCServerClientSocket lpSocket)
{
	return NULL;
}


// Initialize WinSock DLL
VOID InitWinsock()
{
    WORD wVersionRequested;
    WSADATA wsaData;

    wVersionRequested = MAKEWORD(2, 0);
 
    if (WSAStartup(wVersionRequested, &wsaData) != 0)
        // Return FALSE as we couldn't find a usable WinSock DLL
		throw ESocketWinsock(TS("Unable to initialize winsock dll."));
   
    /* Confirm that the WinSock DLL supports 2.0 */
 
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 0) {
        // We couldn't find a usable winsock dll
        WSACleanup();
		throw ESocketDll(TS("Winsock dll version is not 2.0 or higher."));
    }
}

// Deinitialize WinSock DLL
VOID DeinitWinsock()
{
	if (WSACleanup() != 0)
		throw ESocketWinsock(TS("Unable to deinitialize winsock dll."));
}
