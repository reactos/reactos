// SockUtils.h - Declarations for the Winsock utility functions module.
// (C) 2002-2004 Royce Mitchell III
// This file is under the BSD & LGPL licenses

#ifndef __SOCKUTILS_H
#define __SOCKUTILS_H

#include <string>
#ifdef WIN32
#  include <winsock2.h>
#  define in_addr_t u_long
#  define SUERRNO WSAGetLastError()
#  define EADDRINUSE WSAEADDRINUSE
#  define ENOTSOCK WSAENOTSOCK
#  define socklen_t int
#elif defined(UNIX)
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <unistd.h>
#  include <errno.h>
#  define closesocket(so) close(so)
#  define SOCKET int
#  define INVALID_SOCKET -1
#  define SOCKET_ERROR -1
#  define SUERRNO errno
#  ifdef MACOSX
#    define socklen_t int  //Stupid mac
#  endif
#else
#  error unrecognized target
#endif

#include <assert.h>

extern bool suStartup();
extern SOCKET suTcpSocket();
extern SOCKET suUdpSocket();
extern bool suShutdownConnection(SOCKET sd);
extern in_addr_t suLookupAddress ( const char* pcHost );
extern bool suConnect ( SOCKET so, in_addr_t iAddress, u_short iPort );
extern bool suConnect ( SOCKET so, const char* szAddress, u_short iPort );
extern SOCKET suEstablishConnection ( in_addr_t iAddress, u_short iPort, int type );
extern SOCKET suEstablishConnection ( const char* szAddress, u_short iPort, int type );
extern bool suBroadcast ( SOCKET so, u_short port, const char* buf, int len = -1 );
extern int suRecv ( SOCKET so, char* buf, int buflen, int timeout );
extern int suRecvFrom ( SOCKET so, char* buf, int buflen, int timeout, sockaddr_in* from, socklen_t* fromlen );
extern bool suBind ( SOCKET so, in_addr_t iInterfaceAddress, u_short iListenPort, bool bReuseAddr = false );
extern bool suBind ( SOCKET so, const char* szInterfaceAddress, u_short iListenPort, bool bReuseAddr = false );
extern bool suEnableBroadcast ( SOCKET so, bool bEnable = true );
extern const char* suErrDesc ( int err );

#if defined(UNICODE) || defined(_UNICODE)
extern in_addr_t suLookupAddress ( const wchar_t* pcHost );
extern bool suBroadcast ( SOCKET so, u_short port, const wchar_t* buf, int len = -1 );
extern int suRecv ( SOCKET so, wchar_t* buf, int buflen, int timeout );
extern int suRecvFrom ( SOCKET so, wchar_t* buf, int buflen, int timeout, sockaddr_in* from, int* fromlen );
extern bool suBind ( SOCKET so, const wchar_t* szInterfaceAddress, u_short iListenPort, bool bReuseAddr = false );
#endif//UNICODE

class suSocket
{
	SOCKET _so;
public:
	suSocket ( SOCKET so = INVALID_SOCKET ) : _so(so)
	{
	}
	const suSocket& operator = ( SOCKET so )
	{
		assert ( _so == INVALID_SOCKET ); // must Detach() or Close() existing socket first
		_so = so;
		return *this;
	}
	virtual ~suSocket()
	{
		Close();
	}
	void Close()
	{
		if ( _so != INVALID_SOCKET )
		{
			//suShutdownConnection ( _so ); // TODO - only valid on TCP sockets
			closesocket ( _so );
			_so = INVALID_SOCKET;
		}
	}
	operator SOCKET() const
	{
		return _so;
	}
	SOCKET Attach ( SOCKET so )
	{
		SOCKET old = Detach();
		_so = so;
		return old;
	}
	SOCKET Detach()
	{
		SOCKET so = _so;
		_so = INVALID_SOCKET;
		return so;
	}

private:
	// disable copy semantics
	suSocket ( const suSocket& );
	const suSocket& operator = ( const suSocket& );
};

class suBufferedRecvSocket : public suSocket
{
	char _buf[2048];
	int _off;
	int _len;
public:
	suBufferedRecvSocket ( SOCKET so = INVALID_SOCKET );
	int recvUntil ( std::string& buf, char until, int timeout );
	void recvPending();
	bool recvInStr ( char c );
};

class SockAddrIn : public sockaddr_in
{
public:
	SockAddrIn(); // creates broadcast address
	SockAddrIn ( const char* szAddr, u_short iPort );
	SockAddrIn ( in_addr_t iAddr, u_short iPort );
	operator sockaddr* () { return (sockaddr*)this; }
	operator sockaddr_in* () { return (sockaddr_in*)this; }
};

#endif//__SOCKUTILS_H
