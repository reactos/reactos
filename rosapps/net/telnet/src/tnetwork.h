// This is a simple class to handle socket connections
// (Paul Brannan 6/15/98)

#ifndef __TNETWORK_H
#define __TNETWORK_H

#include <windows.h>

// Mingw32 doesn't use winsock.h (Paul Brannan 9/4/98)
//#ifdef __MINGW32__
//#ifdef __CYGWIN__
#include <winsock.h>
//#else
//#include <Windows32/sockets.h>
//#endif
//#else
//#include <winsock.h>
//#endif

enum NetworkType {TN_NETSOCKET, TN_NETPIPE};

typedef int(*Naws_func_t)(char *, int, int);

class TNetwork {
private:
	SOCKET socket;
	BOOL local_echo;					// Paul Brannan 8/25/98
	BOOL line_mode;						// Paul Brannan 12/31/98
	NetworkType net_type;				// Paul Brannan 3/18/99
	HANDLE pipeIn, pipeOut;				// Paul Brannan 3/18/99
	Naws_func_t naws_func;
	char *local_address;

public:
	TNetwork(SOCKET s = 0): socket(s), local_echo(1), line_mode(1),
		net_type(TN_NETSOCKET), naws_func((Naws_func_t)NULL),
		local_address((char *)NULL) {}
	~TNetwork() {if(local_address) delete local_address;}

	void SetSocket(SOCKET s);
	SOCKET GetSocket() {return socket;}
	void SetPipe(HANDLE pIn, HANDLE pOut);
	void SetNawsFunc(Naws_func_t func) {naws_func = func;}
	void SetLocalAddress(char *buf);
	const char* GetLocalAddress() {return local_address;}

	NetworkType get_net_type() {return net_type;}

	int WriteString(const char *str, const int length);
	int ReadString (char *str, const int length);

	BOOL get_local_echo() {return local_echo;}
	void set_local_echo(BOOL b) {local_echo = b;}

	BOOL get_line_mode() {return line_mode;}
	void set_line_mode(BOOL b) {line_mode = b;}

	void do_naws(int width, int height);
};

#endif
