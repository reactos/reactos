///////////////////////////////////////////////////////////////////////////////
//Telnet Win32 : an ANSI telnet client.
//Copyright (C) 1998  Paul Brannan
//Copyright (C) 1998  I.Ioannou
//Copyright (C) 1997  Brad Johnson
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
// Module:		tnclass.cpp
//
// Contents:	telnet object definition
//
// Product:		telnet
//
// Revisions: August 30, 1998 Paul Brannan <pbranna@clemson.edu>
//            July 12, 1998 Paul Brannan
//            June 15, 1998 Paul Brannan
//            May 14, 1998	Paul Brannan
//            5.April.1997 jbj@nounname.com
//            14.Sept.1996 jbj@nounname.com
//            Version 2.0
//
///////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <string.h>
#include "tnclass.h"
#include "tnmisc.h"

// Mingw32 needs these (Paul Brannan 9/4/98)
#ifndef ICON_SMALL
#define ICON_SMALL 0
#endif
#ifndef ICON_BIG
#define ICON_BIG 1
#endif

// Ioannou Dec. 8, 1998
#ifdef __BORLANDC__
#ifndef WM_SETICON
#define WM_SETICON STM_SETICON
#endif
#endif

// DoInit() - performs initialization that is common to both the
// constructors (Paul Brannan 6/15/98)
void Telnet::DoInit() {
	Socket = INVALID_SOCKET;
	bConnected = 0;
	bNetPaused = 1;
	bNetFinished = 1;
	bNetFinish = 0;
	hThread = 0;								// Sam Robertson 12/7/98
	hProcess = 0;

	WSADATA WsaData;

	// Set the title
	telSetConsoleTitle("No Connection");

	// Change the icon
	hConsoleWindow = GetConsoleWindow();
	iconChange = SetIcon(hConsoleWindow, 0, &oldBIcon, &oldSIcon, ini.get_startdir());

	if (WSAStartup(MAKEWORD(1, 1), &WsaData)) {
		DWORD dwLastError = GetLastError();
		printm(0, FALSE, MSG_ERROR, "WSAStartup()");
		printm(0, TRUE, dwLastError);
		bWinsockUp = 0;
		return;
	}
	bWinsockUp = 1;

	// Get keyfile (Paul Brannan 5/12/98)
	const char *keyfile = ini.get_keyfile();

	// This should be changed later to use the Tnerror routines
	// This has been done (Paul Brannan 6/5/98)
	if(LoadKeyMap( keyfile, ini.get_default_config()) != 1)
		// printf("Error loading keymap.\n");
		printm(0, FALSE, MSG_ERRKEYMAP);
}

Telnet::Telnet():
MapLoader(KeyTrans, Charmap),
Console(GetStdHandle(STD_OUTPUT_HANDLE)),
TelHandler(Network, Console, Parser),
ThreadParams(TelHandler),
Clipboard(GetConsoleWindow(), Network),
Mouse(Clipboard),
Scroller(Mouse, ini.get_scroll_size()),
Parser(Console, KeyTrans, Scroller, Network, Charmap) {
	DoInit();
}

Telnet::Telnet(const char * szHost1, const char *strPort1):
MapLoader(KeyTrans, Charmap),
Console(GetStdHandle(STD_OUTPUT_HANDLE)),
TelHandler(Network, Console, Parser),
ThreadParams(TelHandler),
Clipboard(GetConsoleWindow(), Network),
Mouse(Clipboard),
Scroller(Mouse, ini.get_scroll_size()),
Parser(Console, KeyTrans, Scroller, Network, Charmap) {
	DoInit();
	Open( szHost1, strPort1);
}

Telnet::~Telnet(){
	if (bWinsockUp){
		if(bConnected) Close();
		WSACleanup();
	}

	// Paul Brannan 8/10/98
	if(iconChange) {
		ResetIcon(hConsoleWindow, oldBIcon, oldSIcon);
	}

}

// changed from char * to const char * (Paul Brannan 5/12/98)
int Telnet::LoadKeyMap(const char * file, const char * name){
	// printf("Loading %s from %s.\n", name ,file);
	printm(0, FALSE, MSG_KEYMAP, name, file);
	return MapLoader.Load(file,name);
}

void Telnet::DisplayKeyMap(){ // display available keymaps
	MapLoader.Display();
};

int  Telnet::SwitchKeyMap(int to) { // switch to selected keymap
	int ret = KeyTrans.SwitchTo(to);
	switch(ret) {
	case -1: printm(0, FALSE, MSG_KEYNOKEYMAPS); break;
	case 0: printm(0, FALSE, MSG_KEYBADMAP); break;
	case 1: printm(0, FALSE, MSG_KEYMAPSWITCHED); break;
	}
	return ret;
};


int Telnet::Open(const char *szHost1, const char *strPort1){
	if (bWinsockUp && !bConnected){
		telSetConsoleTitle(szHost1);

		strncpy (szHost,szHost1, 127);
		strncpy(strPort, strPort1, sizeof(strPort));

		// Determine whether to pipe to an executable or use our own sockets
		// (Paul Brannan March 18, 1999)
		const char *netpipe;
		if(*(netpipe=ini.get_netpipe())) {
			PROCESS_INFORMATION pi;
			HANDLE hInWrite, hOutRead, hErrRead;
			if(!CreateHiddenConsoleProcess(netpipe, &pi, &hInWrite,
				&hOutRead, &hErrRead)) {
				printm(0, FALSE, MSG_ERRPIPE);
				return TNNOCON;
			}
			Network.SetPipe(hOutRead, hInWrite);
			hProcess = pi.hProcess;
		} else {
			Socket = Connect();
			if (Socket == INVALID_SOCKET) {
				printm(0, FALSE, GetLastError());
				return TNNOCON;
			}
			Network.SetSocket(Socket);
			SetLocalAddress(Socket);
		}

		bNetFinish = 0;
		bConnected = 1;
		ThreadParams.p.bNetPaused = &bNetPaused;
		ThreadParams.p.bNetFinish = &bNetFinish;
		ThreadParams.p.bNetFinished = &bNetFinished;
		ThreadParams.p.hExit = CreateEvent(0, TRUE, FALSE, "");
		ThreadParams.p.hPause = CreateEvent(0, FALSE, FALSE, "");
		ThreadParams.p.hUnPause = CreateEvent(0, FALSE, FALSE, "");
		DWORD idThread;

		// Disable Ctrl-break (PB 5/14/98);
		// Fixed (Thomas Briggs 8/17/98)
        if(ini.get_disable_break() || ini.get_control_break_as_c())
			SetConsoleCtrlHandler(ControlEventHandler, TRUE);

		hThread = CreateThread(0, 0,
			(LPTHREAD_START_ROUTINE) telProcessNetwork,
			(LPVOID)&ThreadParams, 0, &idThread);
		// This helps the display thread a little (Paul Brannan 8/3/98)
		SetThreadPriority(hThread, THREAD_PRIORITY_ABOVE_NORMAL);
		return Resume();
	} else if(bWinsockUp && bConnected) {
			printm (0, FALSE, MSG_ALREADYCONNECTED, szHost);
	}
 	
	return TNNOCON; // cannot do winsock stuff or already connected
}

// There seems to be a bug with MSVC's optimization.  This turns them off
// for these two functions.
// (Paul Brannan 5/14/98)
#ifdef _MSC_VER
#pragma optimize("", off)
#endif


int Telnet::Close() {
	Console.sync();
	switch(Network.get_net_type()) {
	case TN_NETSOCKET:
		if(Socket != INVALID_SOCKET) closesocket(Socket);
		Socket = INVALID_SOCKET;
		break;
	case TN_NETPIPE:
		if(hProcess != 0) {
			TerminateProcess(hProcess, 0);
			CloseHandle(hProcess);
			hProcess = 0;
		}
		break;
	}

	// Enable Ctrl-break (PB 5/14/98);
	// Ioannou : this must be FALSE
    if(ini.get_disable_break()) SetConsoleCtrlHandler(NULL, FALSE);

	if (hThread) CloseHandle(hThread);		// Paul Brannan 8/11/98
	hThread = NULL;							// Daniel Straub 11/12/98

	SetEvent(ThreadParams.p.hUnPause);
	bNetFinish = 1;
	while (!bNetFinished)
		Sleep (0);	// give up our time slice- this lets our connection thread
					// finish itself, so we don't hang -crn@ozemail.com.au
	telSetConsoleTitle("No Connection");
	bConnected = 0;
	return 1;
}

int Telnet::Resume(){
	int i;
	if (bConnected) {
		Console.sync();
		for(;;){
			SetEvent(ThreadParams.p.hUnPause);
			i = telProcessConsole(&ThreadParams.p, KeyTrans, Console,
				Network, Mouse, Clipboard, hThread);
			if (i) bConnected = 1;
			else bConnected = 0;
			ResetEvent(ThreadParams.p.hUnPause);
			SetEvent(ThreadParams.p.hPause);
			while (!bNetPaused)
				Sleep (0);	// give up our time slice- this lets our connection thread
							// unpause itself, so we don't hang -crn@ozemail.com.au
			switch (i){
			case TNNOCON:
				Close();
				return TNDONE;
			case TNPROMPT:
				return TNPROMPT;
			case TNSCROLLBACK:
				Scroller.ScrollBack();
				break;
			case TNSPAWN:
				NewProcess();
			}
		}
	}
	return TNNOCON;
}

// Turn optimization back on (Paul Brannan 5/12/98)
#ifdef _MSC_VER
#pragma optimize("", on)
#endif

// The scrollback functions have been moved to TScroll.cpp
// (Paul Brannan 6/15/98)
SOCKET Telnet::Connect()
{
	SOCKET Socket1 = socket(AF_INET, SOCK_STREAM, 0);
	SOCKADDR_IN SockAddr;
	SockAddr.sin_family = AF_INET;
	SockAddr.sin_addr.s_addr = inet_addr(szHost);

	// determine the port correctly -crn@ozemail.com.au 15/12/98
	SERVENT *sp;
	sp = getservbyname (strPort, "tcp");
	if (sp == NULL) {
		if (isdigit (*(strPort)))
			SockAddr.sin_port = htons(atoi(strPort));
		else {
			printm(0, FALSE, MSG_NOSERVICE, strPort);
			return INVALID_SOCKET;
		}
	} else
		SockAddr.sin_port = sp->s_port;
	///

	// Were we given host name?
	if (SockAddr.sin_addr.s_addr == INADDR_NONE) {

		// Resolve host name to IP address.
		printm(0, FALSE, MSG_RESOLVING, szHost);
		hostent* pHostEnt = gethostbyname(szHost);
		if (!pHostEnt)
			return INVALID_SOCKET;
		printit("\n");

		SockAddr.sin_addr.s_addr = *(DWORD*)pHostEnt->h_addr;
	}

	// Print a message telling the user the IP we are connecting to
	// (Paul Brannan 5/14/98)
	char ss_b1[4], ss_b2[4], ss_b3[4], ss_b4[4], ss_b5[12];
	itoa(SockAddr.sin_addr.S_un.S_un_b.s_b1, ss_b1, 10);
	itoa(SockAddr.sin_addr.S_un.S_un_b.s_b2, ss_b2, 10);
	itoa(SockAddr.sin_addr.S_un.S_un_b.s_b3, ss_b3, 10);
	itoa(SockAddr.sin_addr.S_un.S_un_b.s_b4, ss_b4, 10);
	itoa(ntohs(SockAddr.sin_port), ss_b5, 10);
	printm(0, FALSE, MSG_TRYING, ss_b1, ss_b2, ss_b3, ss_b4, ss_b5);
	
	if (connect(Socket1, (sockaddr*)&SockAddr, sizeof(SockAddr)))
		return INVALID_SOCKET;

	char esc[2];
	esc [0] = ini.get_escape_key();
	esc [1] = 0;
	printm(0, FALSE, MSG_CONNECTED, szHost, esc);

	return Socket1;
}

void Telnet::telSetConsoleTitle(const char * szHost1)
{
	char szTitle[128] = "Telnet - ";
	strcat(szTitle, szHost1);
	if(ini.get_set_title()) SetConsoleTitle(szTitle);
}

void Telnet::NewProcess() {
	char cmd_line[MAX_PATH*2];
	PROCESS_INFORMATION pi;

	strcpy(cmd_line, ini.get_startdir());
	strcat(cmd_line, ini.get_exename());	// Thomas Briggs 12/7/98
	
	if(!SpawnProcess(cmd_line, &pi)) printm(0, FALSE, MSG_NOSPAWN);
}

void Telnet::SetLocalAddress(SOCKET s) {
	SOCKADDR_IN SockAddr;
	int size = sizeof(SOCKADDR_IN);
	memset(&SockAddr, 0, sizeof(SockAddr));
	SockAddr.sin_family = AF_INET;

	getsockname(Network.GetSocket(), (sockaddr*)&SockAddr, &size);
	char ss_b1[4], ss_b2[4], ss_b3[4], ss_b4[4];
	itoa(SockAddr.sin_addr.S_un.S_un_b.s_b1, ss_b1, 10);
	itoa(SockAddr.sin_addr.S_un.S_un_b.s_b2, ss_b2, 10);
	itoa(SockAddr.sin_addr.S_un.S_un_b.s_b3, ss_b3, 10);
	itoa(SockAddr.sin_addr.S_un.S_un_b.s_b4, ss_b4, 10);

	char addr[40];
	strcpy(addr, ss_b1);
	strcat(addr, ".");
	strcat(addr, ss_b2);
	strcat(addr, ".");
	strcat(addr, ss_b3);
	strcat(addr, ".");
	strcat(addr, ss_b4);
	strcat(addr, ":0.0");

	Network.SetLocalAddress(addr);
}

