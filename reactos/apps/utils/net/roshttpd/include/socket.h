/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS HTTP Daemon
 * FILE:        include/socket.h
 */
#ifndef __SOCKET_H
#define __SOCKET_H
#include <crtdll/stdio.h>
#include <windows.h>
#include <winsock2.h>
#include <thread.h>
#include <list.h>
#include <exception>
#include <assert.h>

#define MAX_PENDING_CONNECTS 4      // The backlog allowed for listen()

VOID InitWinsock();
VOID DeinitWinsock();

class CSocket;
class CClientSocket;
class CServerClientSocket;
class CServerClientThread;
class CServerSocket;

typedef CSocket* LPCSocket;
typedef CClientSocket* LPCClientSocket;
typedef CServerClientSocket* LPCServerClientSocket;
typedef CServerClientThread* LPCServerClientThread;
typedef CServerSocket* LPCServerSocket;

class ESocket {
public:
	ESocket() { Description = NULL; }
	ESocket(LPTSTR description)  { Description = description; }
	LPTSTR what()  { return Description; }
protected:
	LPTSTR Description;
};

class ESocketWinsock : public ESocket {
public:
	ESocketWinsock(LPTSTR description) { Description = description; }
};

class ESocketDll : public ESocket {
public:
	ESocketDll(LPTSTR description) { Description = description; }
};

class ESocketOpen : public ESocket {
public:
	ESocketOpen(LPTSTR description) { Description = description; }
};

class ESocketClose : public ESocket {
public:
	ESocketClose(LPTSTR description) { Description = description; }
};

class ESocketSend : public ESocket {
public:
	ESocketSend(LPTSTR description) { Description = description; }
};

class ESocketReceive : public ESocket {
public:
	ESocketReceive(LPTSTR description) { Description = description; }
};


class CSocket {
public:
	CSocket();
	virtual ~CSocket();
	virtual SOCKET GetSocket();
	virtual VOID SetSocket(SOCKET socket);
	virtual SOCKADDR_IN GetSockAddrIn();
	virtual VOID SetSockAddrIn(SOCKADDR_IN sockaddrin);
	virtual VOID SetEvents(LONG lEvents);
	virtual LONG GetEvents();
    virtual VOID SetPort( UINT nPort) {};
	virtual VOID Open();
	virtual VOID Close();
	virtual INT Transmit( LPSTR lpsBuffer,  UINT nLength) { return 0; };
	virtual INT Receive(LPSTR lpsBuffer,  UINT nLength) { return 0; };
	virtual INT SendText( LPSTR lpsStr) { return 0; };
protected:
	SOCKET Socket;
	SOCKADDR_IN SockAddrIn;
	WSAEVENT Event;
	UINT Port;
	BOOL Active;
private:
	LONG Events;
};

class CServerClientSocket : public CSocket {
public:
	CServerClientSocket() {};
	CServerClientSocket(LPCServerSocket lpServerSocket);
	CServerSocket *GetServerSocket();
	virtual INT Transmit( LPSTR lpsBuffer,  UINT nLength);
	virtual INT Receive(LPSTR lpsBuffer,  UINT nLength);
	virtual INT SendText( LPSTR lpsText);
	virtual VOID MessageLoop();
	virtual VOID OnRead() {};
	//virtual VOID OnWrite() {};
	virtual VOID OnClose() {};
protected:
	LPCServerSocket ServerSocket;
};

class CServerClientThread : public CThread {
public:
	CServerClientThread() {};
	CServerClientThread(CServerClientSocket *socket);
	virtual ~CServerClientThread();
protected:
	CServerClientSocket *ClientSocket;
};

class CServerSocket : public CSocket {
public:
	CServerSocket();
	virtual ~CServerSocket();
	virtual VOID SetPort( UINT nPort);
	virtual VOID Open();
	virtual VOID Close();
	virtual LPCServerClientSocket OnGetSocket(LPCServerSocket lpServerSocket);
	virtual LPCServerClientThread OnGetThread(LPCServerClientSocket lpSocket);
	virtual VOID OnAccept( LPCServerClientThread lpThread) {};
	virtual VOID MessageLoop();
	VOID InsertClient(LPCServerClientThread lpClient);
	VOID RemoveClient(LPCServerClientThread lpClient);
protected:
	CList<LPCServerClientThread> Connections;
};

#endif /* __SOCKET_H */
