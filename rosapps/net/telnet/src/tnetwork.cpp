///////////////////////////////////////////////////////////////////////////////
//Telnet Win32 : an ANSI telnet client.
//Copyright (C) 1998-2000 Paul Brannan
//Copyright (C) 1998 I.Ioannou
//Copyright (C) 1997 Brad Johnson
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either version 2
//of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
//I.Ioannou
//roryt@hol.gr
//
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
// Module:		tnetwork.cpp
//
// Contents:	telnet network module
//
// Product:		telnet
//
// Revisions:	March 18, 1999		Paul Brannan (pbranna@clemson.edu)
//
///////////////////////////////////////////////////////////////////////////////

#include "tnetwork.h"

void TNetwork::SetSocket(SOCKET s) {
	socket = s;
	net_type = TN_NETSOCKET;
	local_echo = line_mode = 1;
}

void TNetwork::SetPipe(HANDLE pIn, HANDLE pOut) {
	pipeIn = pIn;
	pipeOut = pOut;
	net_type = TN_NETPIPE;
	local_echo = line_mode = 0;
}

int TNetwork::WriteString(const char *str, const int length) {
	switch(net_type) {
	case TN_NETSOCKET:
		return send(socket, str, length, 0);
	case TN_NETPIPE:
		{
			DWORD dwWritten;
			if(!WriteFile(pipeOut, str, length, &dwWritten, (LPOVERLAPPED)NULL)) return -1;
			return dwWritten;
		}
	}
	return 0;
}

int TNetwork::ReadString (char *str, const int length) {
	switch(net_type) {
	case TN_NETSOCKET:
		return recv(socket, str, length, 0);
	case TN_NETPIPE:
		{
			DWORD dwRead;
			if(!ReadFile(pipeIn, str, length, &dwRead, (LPOVERLAPPED)NULL)) return -1;
			return dwRead;
		}
	}
	return 0;
}

void TNetwork::do_naws(int width, int height) {
	if(!naws_func) return;
	char buf[100];
	int len = (*naws_func)(buf, width, height);
	WriteString(buf, len);
}

void TNetwork::SetLocalAddress(char *buf) {
	local_address = new char[strlen(buf) + 1];
	strcpy(local_address, buf);
}