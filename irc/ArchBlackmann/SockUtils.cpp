// SockUtils.cpp - Some basic socket utility functions.
// (C) 2002-2004 Royce Mitchell III
// This file is under the BSD & LGPL licenses

#include <stdio.h>
#include "SockUtils.h"
#ifdef WIN32
#  ifndef SD_SEND // defined in winsock2.h, but not winsock.h
#    define SD_SEND 1
#  endif
#  define snprintf _snprintf
#  ifdef _MSC_VER
#    pragma comment ( lib, "ws2_32.lib" )
#  endif//_MSC_VER
#elif defined(UNIX)
#  include <errno.h>
#  include "string.h" // memset
#  include <netdb.h> // hostent
#  include <arpa/inet.h> //inet_addr
#  include <sys/time.h>
#  define SD_SEND SHUT_WR //bah thou shalt name thy defines the same
#else
#  error unrecognized target
#endif
//// Constants /////////////////////////////////////////////////////////
const int kBufferSize = 1024;
// creates broadcast address
SockAddrIn::SockAddrIn()
{
	memset ( this, 0, sizeof(sockaddr_in) );
	sin_family = AF_INET;
}
SockAddrIn::SockAddrIn ( const char* szAddr, u_short iPort )
{
	memset ( this, 0, sizeof(sockaddr_in) );
	sin_family = AF_INET;
	sin_addr.s_addr = suLookupAddress(szAddr);
	sin_port = htons(iPort);
}
SockAddrIn::SockAddrIn ( in_addr_t iAddr, u_short iPort )
{
	memset ( this, 0, sizeof(sockaddr_in) );
	sin_family = AF_INET;
	sin_addr.s_addr = iAddr;
	sin_port = htons(iPort);
}
bool suStartup()
{
#ifdef WIN32
	WSADATA wsaData;
	if ( WSAStartup ( MAKEWORD(2,0), &wsaData ) )
		return false;
	if ( wsaData.wVersion != MAKEWORD(2,0) )
	{
		WSACleanup();
		return false;
	}
	return true;
#elif defined(UNIX)
	// nothing special required here
	return true;
#else
#  error unrecognized target
#endif
}
//// suTcpSocket ////////////////////////////////////////////////
// Creates a TCP socket.
SOCKET suTcpSocket()
{
	SOCKET so = socket ( AF_INET, SOCK_STREAM, 0 );
#if defined(_DEBUG) && defined(WIN32)
	if ( so == INVALID_SOCKET && WSANOTINITIALISED == WSAGetLastError() )
		MessageBox ( NULL, "You forgot to call suStartup()!", "SockUtils", MB_OK|MB_ICONEXCLAMATION );
#endif
	return so;
}
//// suUdpSocket ////////////////////////////////////////////////
// Creates a UDP socket. Compensates for new "functionality" introduced
// in Win2K with regards to select() calls
// MS Transport Provider IOCTL to control
// reporting PORT_UNREACHABLE messages
// on UDP sockets via recv/WSARecv/etc.
// Path TRUE in input buffer to enable (default if supported),
// FALSE to disable.
#ifndef SIO_UDP_CONNRESET
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR,12)
#endif//SIO_UDP_CONNRESET
SOCKET suUdpSocket()
{
	SOCKET so = socket ( AF_INET, SOCK_DGRAM, 0 );
#if defined(_DEBUG) && defined(WIN32)
	if ( so == INVALID_SOCKET && WSANOTINITIALISED == WSAGetLastError() )
		MessageBox ( NULL, "You forgot to call suStartup()!", "SockUtils", MB_OK|MB_ICONEXCLAMATION );
#endif
#ifdef WIN32
	// for Windows 2000, disable new behavior...
	// see: http://www-pc.uni-regensburg.de/systemsw/W2KPRO/UPDATE/POSTSP1/Q263823.htm
	// this code is innocuous on other win32 platforms
	DWORD dwBytesReturned = 0;
	BOOL bNewBehavior = FALSE;
	// disable  new Win2K behavior using
	// IOCTL: SIO_UDP_CONNRESET
	// we don't care about return value :)
	WSAIoctl(so, SIO_UDP_CONNRESET,
		&bNewBehavior, sizeof(bNewBehavior),
		NULL, 0, &dwBytesReturned,
		NULL, NULL);
#endif
	return so;
}
//// suShutdownConnection ////////////////////////////////////////////////
// Gracefully shuts the connection sd down.  Returns true if it was able
// to shut it down nicely, false if we had to "slam" it shut.
// (either way, the socket does get closed)
bool suShutdownConnection(SOCKET sd)
{
	if ( sd == INVALID_SOCKET )
		return true;
	// Disallow any further data sends.  This will tell the other side
	// that we want to go away now.  If we skip this step, we don't
	// shut the connection down nicely.
	if (shutdown(sd, SD_SEND) == SOCKET_ERROR)
	{
		closesocket(sd);
		return false;
	}
	// Receive any extra data still sitting on the socket.  After all
	// data is received, this call will block until the remote host
	// acknowledges the TCP control packet sent by the shutdown above.
	// Then we'll get a 0 back from recv, signalling that the remote
	// host has closed its side of the connection.
	char acReadBuffer[kBufferSize];
	for ( ;; )
	{
		int nNewBytes = recv(sd, acReadBuffer, kBufferSize, 0);
		if (nNewBytes == SOCKET_ERROR)
		{
			closesocket(sd);
			return false;
		}
		else if (nNewBytes != 0)
		{
			// FYI, received (nNewBytes) unexpected bytes during shutdown.
		}
		else
		{
			// Okay, we're done!
			break;
		}
	}
	// Close the socket.
	if (closesocket(sd) == SOCKET_ERROR)
	{
		return false;
	}
	return true;
}
//// suLookupAddress ////////////////////////////////////////////////
// Basically converts a name address to an ip address
in_addr_t suLookupAddress ( const char* pcHost )
{
	in_addr_t nRemoteAddr = inet_addr(pcHost);
	if ( nRemoteAddr == INADDR_NONE )
	{
		// pcHost isn't a dotted IP, so resolve it through DNS
		hostent* pHE = gethostbyname(pcHost);
		if ( pHE == 0 )
		{
#if defined(_DEBUG) && defined(WIN32)
			if ( WSANOTINITIALISED == WSAGetLastError() )
				MessageBox ( NULL, "You forgot to call suStartup()!", "SockUtils", MB_OK|MB_ICONEXCLAMATION );
#endif
			return INADDR_NONE;
		}
		nRemoteAddr = *((in_addr_t*)pHE->h_addr_list[0]);
	}
	return nRemoteAddr;
}
bool suConnect ( SOCKET so, in_addr_t iAddress, u_short iPort )
{
	SockAddrIn sinRemote ( iAddress, iPort );
	if ( SOCKET_ERROR == connect(so,sinRemote,sizeof(sinRemote)) )
	{
#if defined(_DEBUG) && defined(WIN32)
		if ( WSANOTINITIALISED == WSAGetLastError() )
			MessageBox ( NULL, "You forgot to call suStartup()!", "SockUtils", MB_OK|MB_ICONEXCLAMATION );
#endif
		return false;
	}
	return true;
}
bool suConnect ( SOCKET so, const char* szAddress, u_short iPort )
{
	return suConnect ( so, suLookupAddress(szAddress), iPort );
}
//// suEstablishConnection ////////////////////////////////////////////////
// creates a socket of the specified type, connects to the ip address/port
// requested, and returns the SOCKET created
SOCKET suEstablishConnection ( in_addr_t iAddress, u_short iPort, int type )
{
	// Create a socket
	if ( type != SOCK_STREAM && type != SOCK_DGRAM )
		return INVALID_SOCKET;
	SOCKET so = socket(AF_INET, type, 0);
	if ( so == INVALID_SOCKET )
		return so;
	if ( !suConnect(so, iAddress, iPort) )
	{
		closesocket(so);
		return INVALID_SOCKET;
	}
	return so;
}
//// suEstablishConnection ////////////////////////////////////////////////
// creates a socket of the specified type, connects to the address/port
// requested, and returns the SOCKET created
SOCKET suEstablishConnection ( const char* szAddress, u_short iPort, int type )
{
	return suEstablishConnection ( suLookupAddress ( szAddress ), iPort, type );
}
//// suBroadcast ////////////////////////////////////////////////
// takes a previously created broadcast-enabled UDP socket, and broadcasts
// a message on the local network
bool suBroadcast ( SOCKET so, u_short port, const char* buf, int len /* = -1 */ )
{
	if ( len == -1 )
		len = (int)strlen(buf);
#if 1
	SockAddrIn to ( INADDR_BROADCAST, port );
#else // some strange MS OS's don't broadcast to localhost...
	SockAddrIn to ( "127.0.0.1", port );
	if ( SOCKET_ERROR == sendto ( so, buf, len, 0, to, sizeof(to) ) )
		return false;
	to.sin_addr.s_addr = INADDR_BROADCAST;
#endif
	if ( SOCKET_ERROR == sendto ( so, buf, len, 0, to, sizeof(to) ) )
		return false;
	return true;
}
//// suRecv ////////////////////////////////////////////////
// retrieves data sent to our TCP socket. If no data, waits for
// a period of timeout ms.
// returns bytes received
// -1 == SOCKET_ERROR
// -2 == timed out waiting for data
int suRecv ( SOCKET so, char* buf, int buflen, int timeout )
{
	struct timeval to;
	fd_set rread;
	int res;
	FD_ZERO(&rread);   // clear the fd_set
	FD_SET(so,&rread); // indicate which socket(s) we want to check
	memset((char *)&to,0,sizeof(to)); // clear the timeval struct
	to.tv_sec = timeout;  // timeout select after (timeout) seconds
	// select returns > 0 if there is an event on the socket
	res = select((int)so+1, &rread, (fd_set *)0, (fd_set *)0, &to );
	if (res < 0)
		return -1; // socket error
	// there was an event on the socket
	if ( (res>0) && (FD_ISSET(so,&rread)) )
		return recv ( so, buf, buflen, 0 );
	return -2;
}
//// suRecvFrom ////////////////////////////////////////////////
// retrieves data sent to our UDP socket. If no data, waits for
// a period of timeout ms.
// returns bytes received
// returns bytes received
// -1 == SOCKET_ERROR
// -2 == timed out waiting for data
int suRecvFrom ( SOCKET so, char* buf, int buflen, int timeout, sockaddr_in* from, socklen_t* fromlen )
{
	struct timeval to;
	fd_set rread;
	int res;
	FD_ZERO(&rread);   // clear the fd_set
	FD_SET(so,&rread); // indicate which socket(s) we want to check
	memset((char *)&to,0,sizeof(to)); // clear the timeval struct
	to.tv_sec = timeout;  // timeout select after (timeout) seconds
	// select returns > 0 if there is an event on the socket
	res = select((int)so+1, &rread, (fd_set *)0, (fd_set *)0, &to );
	if (res < 0)
		return -1; // socket error
	// there was an event on the socket
	if ( (res>0) && (FD_ISSET(so,&rread)) )
		return recvfrom ( so, buf, buflen, 0, (sockaddr*)from, fromlen );
	return -2; // timeout
}
//// suBind ////////////////////////////////////////////////
// binds a UDP socket to an interface & port to receive
// data on that port
bool suBind ( SOCKET so, in_addr_t iInterfaceAddress, u_short iListenPort, bool bReuseAddr /* = false */ )
{
	SockAddrIn sinInterface ( iInterfaceAddress, iListenPort );
	if ( bReuseAddr )
	{
		int optval = -1; // true
		if ( SOCKET_ERROR == setsockopt ( so, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(optval) ) )
		{
#if defined(_DEBUG) && defined(WIN32)
			if ( WSANOTINITIALISED == WSAGetLastError() )
				MessageBox ( NULL, "You forgot to call suStartup()!", "SockUtils", MB_OK|MB_ICONEXCLAMATION );
#endif
			return false;
		}
	}
	if ( SOCKET_ERROR == bind(so, sinInterface, sizeof(sinInterface)) )
	{
		int err = SUERRNO;
		if ( err != EADDRINUSE )
			return false;
#if defined(_DEBUG) && defined(WIN32)
		if ( WSANOTINITIALISED == WSAGetLastError() )
			MessageBox ( NULL, "You forgot to call suStartup()!", "SockUtils", MB_OK|MB_ICONEXCLAMATION );
#endif
	}
	return true;
}
//// suBind ////////////////////////////////////////////////
// binds a UDP socket to an interface & port to receive
// data on that port
bool suBind ( SOCKET so, const char* szInterfaceAddress, u_short iListenPort, bool bReuseAddr /* = false */ )
{
	in_addr_t iInterfaceAddr = inet_addr(szInterfaceAddress);
	if ( iInterfaceAddr == INADDR_NONE )
		return false;
	return suBind ( so, iInterfaceAddr, iListenPort, bReuseAddr );
}
//// suEnableBroadcast ////////////////////////////////////////////////
// in order to send broadcast messages on a UDP socket, this function
// must be called first
bool suEnableBroadcast ( SOCKET so, bool bEnable /* = true */ )
{
	int optval = bEnable ? -1 : 0;
	if ( SOCKET_ERROR == setsockopt ( so, SOL_SOCKET, SO_BROADCAST, (const char*)&optval, sizeof(optval) ) )
		return false;
	return true;
}
//// suErrDesc ////////////////////////////////////////////////
// returns text description of error code
const char* suErrDesc ( int err )
{
	static char errbuf[256];
#ifdef WIN32
	switch ( err )
	{
#define X(E) case E: return #E;
		X(WSAEINTR)	X(WSAEBADF)
		X(WSAEACCES)	X(WSAEFAULT)
		X(WSAEINVAL)	X(WSAEMFILE)
		X(WSAEWOULDBLOCK)	X(WSAEINPROGRESS)
		X(WSAEALREADY)	X(WSAENOTSOCK)
		X(WSAEDESTADDRREQ)	X(WSAEMSGSIZE)
		X(WSAEPROTOTYPE)	X(WSAENOPROTOOPT)
		X(WSAEPROTONOSUPPORT)	X(WSAESOCKTNOSUPPORT)
		X(WSAEOPNOTSUPP)	X(WSAEPFNOSUPPORT)
		X(WSAEAFNOSUPPORT)	X(WSAEADDRINUSE)
		X(WSAEADDRNOTAVAIL)	X(WSAENETDOWN)
		X(WSAENETUNREACH)	X(WSAENETRESET)
		X(WSAECONNABORTED)	X(WSAECONNRESET)
		X(WSAENOBUFS)	X(WSAEISCONN)
		X(WSAENOTCONN)	X(WSAESHUTDOWN)
		X(WSAETOOMANYREFS)	X(WSAETIMEDOUT)
		X(WSAECONNREFUSED)	X(WSAELOOP)
		X(WSAENAMETOOLONG)	X(WSAEHOSTDOWN)
		X(WSAEHOSTUNREACH)	X(WSAENOTEMPTY)
		X(WSAEPROCLIM)	X(WSAEUSERS)
		X(WSAEDQUOT)	X(WSAESTALE)
		X(WSAEREMOTE)	X(WSASYSNOTREADY)
		X(WSAVERNOTSUPPORTED)	X(WSANOTINITIALISED)
		X(WSAEDISCON)	X(WSAENOMORE)
		X(WSAECANCELLED)	X(WSAEINVALIDPROCTABLE)
		X(WSAEINVALIDPROVIDER)	X(WSAEPROVIDERFAILEDINIT)
		X(WSASYSCALLFAILURE)	X(WSASERVICE_NOT_FOUND)
		X(WSATYPE_NOT_FOUND)	X(WSA_E_NO_MORE)
		X(WSA_E_CANCELLED)	X(WSAEREFUSED)
#undef X
	}
	snprintf ( errbuf, sizeof(errbuf), "Unknown socket error (%lu)", err );
	errbuf[sizeof(errbuf)-1] = '\0';
	return errbuf;
#elif defined(UNIX)
	perror(errbuf);
	return errbuf;
#else
#  error unrecognized target
#endif
}
#if defined(UNICODE) || defined(_UNICODE)
in_addr_t suLookupAddress ( const wchar_t* pcHost )
{
	int len = wcslen(pcHost);
	char* p = new char[len+1];
	wcstombs ( p, pcHost, len );
	p[len] = 0;
	in_addr_t rc = suLookupAddress ( p );
	delete[] p;
	return rc;
}
bool suBroadcast ( SOCKET so, u_short port, const wchar_t* buf, int len /* = -1 */ )
{
	char* p = new char[len+1];
	wcstombs ( p, buf, len );
	p[len] = 0;
	bool rc = suBroadcast ( so, port, p, len );
	delete[] p;
	return rc;
}
int suRecv ( SOCKET so, wchar_t* buf, int buflen, int timeout )
{
	char* p = new char[buflen+1];
	int rc = suRecv ( so, p, buflen, timeout );
	p[buflen] = 0;
	mbstowcs ( buf, p, buflen );
	delete[] p;
	return rc;
}
int suRecvFrom ( SOCKET so, wchar_t* buf, int buflen, int timeout, sockaddr_in* from, int* fromlen )
{
	char* p = new char[buflen+1];
	int rc = suRecvFrom ( so, p, buflen, timeout, from, fromlen );
	p[buflen] = 0;
	mbs2wcs ( buf, p, buflen );
	delete[] p;
	return rc;
}
bool suBind ( SOCKET so, const wchar_t* szInterfaceAddress, u_short iListenPort, bool bReuseAddr /* = false */ )
{
	int len = wcslen(szInterfaceAddress);
	char* p = new char[len+1];
	wcstombs ( p, szInterfaceAddress, len );
	p[len] = 0;
	bool rc = suBind ( so, p, iListenPort, bReuseAddr );
	delete[] p;
	return rc;
}
#endif//UNICODE

suBufferedRecvSocket::suBufferedRecvSocket ( SOCKET so )
	: suSocket ( so ), _off(0), _len(0)
{
}

int suBufferedRecvSocket::recvUntil ( std::string& buf, char until, int timeout )
{
	if ( !_len )
		_off = 0;
	else if ( _off > (sizeof(_buf)>>1) )
	{
		memmove ( _buf, &_buf[_off], _len );
		_off = 0;
	}
	char* poff = &_buf[_off];
	for ( ;; )
	{
		char* p = (char*)memchr ( poff, until, _len );
		if ( p /*&& p < &poff[_len]*/ )
		{
			int ret_len = p-poff+1;
			buf.resize ( ret_len );
			memmove ( &buf[0], poff, ret_len );
			_off += ret_len;
			_len -= ret_len;
			return ret_len;
		}
		int rc = suRecv ( *this, &poff[_len], sizeof(_buf)-_len-_off, timeout );
		if ( rc < 0 )
		{
			if ( _len )
			{
				rc = _len;
				buf.resize ( rc );
				memmove ( &buf[0], &_buf[_off], rc );
				_len = 0;
			}
			return rc;
		}
		_len += rc;
	}
}

void suBufferedRecvSocket::recvPending()
{
	if ( !_len )
		_off = 0;
	else if ( _off > (sizeof(_buf)>>1) )
	{
		memmove ( _buf, &_buf[_off], _len );
		_off = 0;
	}
	char* poff = &_buf[_off];
	while ( sizeof(_buf)-_len-_off )
	{
		int rc = suRecv ( *this, &poff[_len], sizeof(_buf)-_len-_off, 1 );
		if ( rc <= 0 )
			break;
		_len += rc;
	}
}

bool suBufferedRecvSocket::recvInStr ( char c )
{
	return NULL != memchr ( &_buf[_off], c, _len );
}
