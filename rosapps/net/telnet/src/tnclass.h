#ifndef __TNCLASS_H_
#define __TNCLASS_H_

#include <windows.h>
#include "tnconfig.h"
#include "ttelhndl.h"
#include "tncon.h"
#include "tnerror.h"
#include "tparams.h"
#include "keytrans.h"
#include "ansiprsr.h"
#include "tcharmap.h"
#include "tnclip.h"
#include "tmouse.h"
#include "tmapldr.h"

class Telnet {
public:
	// create a telnet instance
	Telnet();
	// open a connection return on break/quit
	Telnet(const char * szHost1, const char *strPort1);
	~Telnet();

	// open a connection return on break/quit
	int Open(const char *szHost, const char *strPort = "23");
	int Close();				// close current connection
	int Resume();				// resume current session
	
	// changes to the keymap profile in the file
	int LoadKeyMap( const char * file, const char * name);
	void DisplayKeyMap();		// display available keymaps
	int  SwitchKeyMap(int);		// switch to selected keymap
private:
	SOCKET Connect();
	void telSetConsoleTitle(const char * szHost);
	void DoInit();
	
	SOCKET Socket;
	char strPort[32]; // int iPort;
	char szHost[127];
	volatile int bConnected;
	volatile int bWinsockUp;
	volatile int bNetPaused;
	volatile int bNetFinished;
	volatile int bNetFinish;

	// The order of member classes in the class definition MUST come in
	// this order! (Paul Brannan 12/4/98)
	TNetwork Network;
	TCharmap Charmap;
	KeyTranslator KeyTrans;
	TMapLoader MapLoader;
	TConsole Console;
	TTelnetHandler TelHandler;
	TelThreadParams ThreadParams;
	Tnclip Clipboard;
	TMouse Mouse;
	TScroller Scroller;
	TANSIParser Parser;

	HWND hConsoleWindow;				// Paul Brannan 8/10/98
	LPARAM oldBIcon, oldSIcon;			// Paul Brannan 8/10/98
	bool iconChange;

	HANDLE hThread;						// Paul Brannan 8/11/98
	HANDLE hProcess;					// Paul Brannan 7/15/99

	void NewProcess();					// Paul Brannan 9/13/98
	void SetLocalAddress(SOCKET s);
};

#endif