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
// Module:		ttelhndl.cpp
//
// Contents:	Telnet Handler
//
// Product:		telnet
//
// Revisions: August 30, 1998 Paul Brannan <pbranna@clemson.edu>
//            June 15, 1998   pbranna@clemson.edu (Paul Brannan)
//
//            This is code originally from tnnet.cpp and ansiprsr.cpp
//
///////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include "ttelhndl.h"
#include "telnet.h"
#include "tnconfig.h"
#include "tparams.h"

int naws_string(char *buf, int width, int height);

// This helps make the code more readable (Paul Brannan 1/1/99)
#ifdef DEBUG_TELOPT
#define TELOPT_PRINTD(x) printit(x);
#define TELOPT_PRINTD2(x,n) {		\
	static char buf[20];			\
	printit(s);						\
	printit(" ");					\
	itoa(d, buf, 10);				\
	printit(buf);					\
	printit("\n");					\
}
#else
#define TELOPT_PRINTD(x) ;
#define TELOPT_PRINTD2(x,n) ;
#endif

// A new print function for debugging (Paul Brannan 5/15/98)
#ifdef DEBUG_TELOPT
void TTelnetHandler::print_telopt(const char *s, int d) {
	static char buf[20];
	printit(s);
	printit(" ");
	itoa(d, buf, 10);
	printit(buf);
	printit("\n");
}
#endif

TTelnetHandler::TTelnetHandler(TNetwork &RefNetwork, TConsole &RefConsole,
							   TParser &RefParser):
Network(RefNetwork), Console(RefConsole), Parser(RefParser) {
	init();

	// Paul Brannan 9/13/98
	dwBuffer = ini.get_buffer_size();
	szBuffer = new char [dwBuffer];
	Network.SetNawsFunc(NULL);
}

void TTelnetHandler::init() {
	iTermSet = 0;
	bInBinaryRx = 0;
	bInBinaryTx = 0;
	bInEchoTx = 0;
	bInEchoRx = 0;
	Network.set_local_echo(1);
}

TTelnetHandler::~TTelnetHandler() {
	delete[] szBuffer;
}

int TTelnetHandler::escapeIAC(char *buf, int length){
	// The size of buffer must be greater than 2 * length to ensure no memory
	// out of bounds errors.  The 0xff is escaped into 0xff 0xff.
	char * temp;
	temp = new char [length * 2];
	int current=0;
	for (int x=0; x < length; x++){
		if (buf[x] == (signed char)IAC)
			temp[current++]=(char)IAC;
		temp[current++]=buf[x];
	}
	memcpy( buf, temp, current);
	delete [] temp;
	return current;
}

// This lets us get rid of all the printf's (Paul Brannan 5/15/98)
void TTelnetHandler::SendIAC(char c) {
	static char buf[2] = {IAC};
	buf[1] = c;
	Network.WriteString(buf, 2);
}
void TTelnetHandler::SendIAC(char c1, char c2) {
	static char buf[3] = {IAC};
	buf[1] = c1; buf[2] = c2;
	Network.WriteString(buf, 3);
}
void TTelnetHandler::SendIACParams(char c) {
	static char buf[2];
	buf[0] = c;
	static int length = escapeIAC(buf, 1);
	Network.WriteString(buf, length);
}
void TTelnetHandler::SendIACParams(char c1, char c2) {
	static char buf[4];
	buf[0] = c1; buf[1] = c2;
	static int length = escapeIAC(buf, 2);
	Network.WriteString(buf, length);
}

int naws_string(char *b, int width, int height) {
	int l = 0;
	unsigned char *buf = (unsigned char *)b;

	union {
		char szResponse[2];
		int n;
	};

	buf[l++] = IAC;
	buf[l++] = SB;
	buf[l++] = TELOPT_NAWS;

	n = width;
	buf[l] = szResponse[1];
	if(buf[l-1] == IAC) buf[l++] = IAC;
	buf[l++] = szResponse[0];
	if(buf[l-1] == IAC) buf[l++] = IAC;

	n = height;
	buf[l++] = szResponse[1];
	if(buf[l-1] == IAC) buf[l++] = IAC;
	buf[l++] = szResponse[0];
	if(buf[l-1] == IAC) buf[l++] = IAC;

	buf[l++] = IAC;
	buf[l++] = SE;

	return l;
}

//  Ioannou 29 May 1998 : Something strange happens with
//  Borland compiler at this point when it passes the arguments
//  to SendIACParams. It always sends 80 lines to the server !!!
//  There seems to be a bug with optimization (the disassemble shows
//  that it uses an address plus 0xa than the right one).
//  This turns them off for this point.
#ifdef __BORLANDC__
#pragma -O-
#endif

// Removed old printf code that was commented out to clean this function
// up a bit (Paul brannan 6/15/98)
char* TTelnetHandler::ParseIAC(char* pszBuffer, char* pszBufferEnd)
{
	//	int n,l;
	//	char szResponse[40];
	//  Ioannou 29 May 1998 : I prefer the union redefinitions
	//  than the typecasting (used with them from Pascal and Cobol :-) )
	//  FIX ME !!!! Shall we use the winsock routines instead ?
	
	union {
		char szResponse[2];
		int n;
	};
	
	// Added support for user-defined term name (Paul Brannan 5/13/98)
#define LASTTERM 4
	const char *pszTerms[] =  {ini.get_term(), "ANSI","DEC-VT100","DEC-VT52","UNKNOWN"};
	if(!iTermSet && (pszTerms[0] == 0 || *pszTerms[0] == 0)) iTermSet++;
	
	if (pszBuffer + 2 < pszBufferEnd) {
		switch ((unsigned char)pszBuffer[1]) {
			
			///////////////// DO ////////////////////
		case DO:
			{
				switch (pszBuffer[2]){
				case TELOPT_BINARY:
					TELOPT_PRINTD("RCVD DO TELOPT_BINARY\n");
					if (!bInBinaryRx){
						SendIAC(WILL, TELOPT_BINARY);
						bInBinaryRx = 1;
						TELOPT_PRINTD("SENT WILL TELOPT_BINARY\n");
					}
					break;
				case TELOPT_ECHO:
					// we shouldn't echo for the server! (Paul Brannan 5/30/98)
					TELOPT_PRINTD2("RCVD DO TELOPT_ECHO", pszBuffer[2]);
					SendIAC(WONT, TELOPT_ECHO);
					TELOPT_PRINTD("SENT WONT TELOPT_ECHO\n");
					break;
				case TELOPT_TTYPE:
					TELOPT_PRINTD("RCVD DO TELOPT_TTYPE\n");
					SendIAC(WILL, TELOPT_TTYPE);
					TELOPT_PRINTD("SENT WILL TELOPT_TTYPE\n");
					break;
				case TELOPT_NAWS:
					TELOPT_PRINTD("RCVD DO TELOPT_NAWS\n");
					SendIAC(WILL, TELOPT_NAWS);
					SendIAC(SB, TELOPT_NAWS);
					
					Network.SetNawsFunc(naws_string);
					
					n = Console.GetWidth();
					SendIACParams(szResponse[1],szResponse [0]);
					
					n = Console.GetHeight();
					SendIACParams(szResponse[1],szResponse[0]);
					
					SendIAC(SE);
					TELOPT_PRINTD("SENT WILL TELOPT_NAWS\n");
					break;
				case TELOPT_XDISPLOC:
					TELOPT_PRINTD("RCVD DO TELOPT_XDISPLOC\n");
					SendIAC(WILL, TELOPT_XDISPLOC);
					TELOPT_PRINTD("SENT WILL TELOPT_XDISPLOC\n");
					printit("Retrieving IP...");
						break;
				default:
					TELOPT_PRINTD2("RCVD DO", pszBuffer[2]);
					SendIAC(WONT, pszBuffer[2]);
					TELOPT_PRINTD2("SENT WONT", pszBuffer[2]);
					break;
				}
				if (pszBuffer + 2 < pszBufferEnd)
					pszBuffer += 3;
				break;
			}
			
			///////////////// WILL ////////////////////
		case WILL:
			{
				switch ((unsigned char)pszBuffer[2]){
				case TELOPT_BINARY:
					TELOPT_PRINTD("RCVD WILL TELOPT_BINARY\n");
					if (!bInBinaryTx){
						SendIAC(DO, TELOPT_BINARY);
						bInBinaryTx = 1;
						TELOPT_PRINTD("SENT DO TELOPT_BINARY\n");
					}
					break;
				case TELOPT_ECHO:
					TELOPT_PRINTD2("RCVD WILL TELOPT_ECHO", pszBuffer[2]);
					if(!bInEchoRx) {
						SendIAC(DO, TELOPT_ECHO);
						bInEchoRx = 1;
						Network.set_local_echo(0); // Paul Brannan 8/25/98
						if(iWillSGA) Network.set_line_mode(0);
						TELOPT_PRINTD2("SENT DO TELOPT_ECHO", pszBuffer[2]);
						if(Network.get_local_echo()) Network.set_line_mode(0);
					}
					break;
					
					// Suppress Go Ahead (Paul Brannan 12/31/98)
				case TELOPT_SGA:
					TELOPT_PRINTD("RCVD WILL TELOPT_SGA\n");
					if(!iWillSGA) {
						SendIAC(DO, TELOPT_SGA);
						if(bInEchoRx) Network.set_line_mode(0);
						iWillSGA = 1;
						TELOPT_PRINTD("SENT DO TELOPT_SGA\n");
					}
					break;
					
					////added 1/28/97
				default:
					TELOPT_PRINTD2("RCVD WILL", pszBuffer[2]);
					SendIAC(DONT, pszBuffer[2]);
					TELOPT_PRINTD2("SENT DONT", pszBuffer[2]);
					break;
					////
				}
				if (pszBuffer + 2 < pszBufferEnd)
					pszBuffer += 3;
				break;
			}
			
			///////////////// WONT ////////////////////
		case WONT:
			{
				switch ((unsigned char)pszBuffer[2]){
				case TELOPT_ECHO:
					TELOPT_PRINTD("RCVD WONT TELOPT_ECHO\n");
					if (bInEchoRx){
						SendIAC(DONT, TELOPT_ECHO);
						// bInBinaryRx = 0;
						bInEchoRx = 0; // Paul Brannan 8/25/98
						Network.set_local_echo(1);
						Network.set_line_mode(0);
						TELOPT_PRINTD("SENT DONT TELOPT_ECHO\n");
					}
					break;
					
					// Suppress Go Ahead (Paul Brannan 12/31/98)
				case TELOPT_SGA:
					TELOPT_PRINTD("RCVD WONT TELOPT_SGA\n");
					if(iWillSGA) {
						SendIAC(DONT, TELOPT_SGA);
						Network.set_line_mode(0);
						iWillSGA = 0;
						TELOPT_PRINTD("SENT DONT TELOPT_SGA\n");
					}
					break;
					
				default:
					TELOPT_PRINTD2("RCVD WONT", pszBuffer[2]);
					break;
				}
				if (pszBuffer + 2 < pszBufferEnd)
					pszBuffer += 3;
				break;
			}
			
			///////////////// DONT ////////////////////
		case DONT:
			{
				switch ((unsigned char)pszBuffer[2]){
				case TELOPT_ECHO:
					TELOPT_PRINTD("RCVD DONT TELOPT_ECHO\n");
					if (bInEchoTx){
						SendIAC(WONT, TELOPT_ECHO);
						bInEchoTx = 0;
						TELOPT_PRINTD("SENT WONT TELOPT_ECHO\n");
					}
					break;
				case TELOPT_NAWS:
					TELOPT_PRINTD("RCVD DONT TELOPT_NAWS\n");
					SendIAC(WONT, TELOPT_NAWS);
					Network.SetNawsFunc(naws_string);
					TELOPT_PRINTD("SENT WONT TELOPT_NAWS\n");
					break;
				default:
					TELOPT_PRINTD2("RCVD DONT", pszBuffer[2]);
					break;
				}
				if (pszBuffer + 2 < pszBufferEnd)
					pszBuffer += 3;
				break;
			}
			
			///////////////// SB ////////////////////
		case SB:
			{
				switch ((unsigned char)pszBuffer[2]){
				case TELOPT_TTYPE:
					if (pszBuffer + 5 < pszBufferEnd) {
						TELOPT_PRINTD("RCVD SB TELOPT_TTYPE\n");
						if (pszBuffer[3] == 1){
							TELOPT_PRINTD("SENT SB TT");
							TELOPT_PRINTD(pszTerms[iTermSet]);
							TELOPT_PRINTD("\n");
							SendIAC(SB, TELOPT_TTYPE);
							SendIACParams(0);
							Network.WriteString(pszTerms[iTermSet], strlen(pszTerms[iTermSet]));
							SendIAC(SE);
							
							if (iTermSet < LASTTERM )
								iTermSet+=1;
						}
						if (pszBuffer + 5 < pszBufferEnd)
							pszBuffer += 6;
					}
						break;
				case TELOPT_XDISPLOC:
					if(pszBuffer + 5 < pszBufferEnd) {
						TELOPT_PRINTD("RCVD SB XDISPLOC\n");
						SendIAC(SB, TELOPT_XDISPLOC);
						TELOPT_PRINTD("SENT SB XDISPLOC");
						SendIACParams(0);
						if(Network.GetLocalAddress()) Network.WriteString(Network.GetLocalAddress(),
							strlen(Network.GetLocalAddress()));
						TELOPT_PRINTD(Network.GetLocalAddress());
						TELOPT_PRINTD("\n");
						SendIAC(SE);
						if (pszBuffer + 5 < pszBufferEnd)
							pszBuffer += 6;
					}
					break;
				default: break;
				}
				break;
			}
		default:
			pszBuffer += 2;
			break;
		}
	}
	return pszBuffer;
}

#ifdef __BORLANDC__
// bring bug optimazations
#pragma -O.
#endif

// This is the code from TANSIParser::ParseBuffer.  It parses out IACs, and
// then calls TParser::ParseBuffer to do the terminal emulation.
// (Paul Brannan 6/15/98)
// Hopefully eliminating the unnecessary copying should speed things up a
// little.  (Paul Brannan 6/28/98)
char* TTelnetHandler::ParseBuffer(char* pszBuffer, char* pszBufferEnd){
	char *pszResult;
	char *pszHead = pszBuffer;

	if(Network.get_net_type() == TN_NETSOCKET) {
		while (pszBuffer < pszBufferEnd) {
			// if IAC then parse IAC
			if((unsigned char) *pszBuffer == IAC) {
			
				// check for escaped IAC
				if((pszBufferEnd >= pszBuffer + 1) &&
					(unsigned char)*(pszBuffer + 1) == IAC) {
					// we move data at the front of the buffer to the end so
					// that if we only have IACs we won't return pszBuffer
					// even though we did parse something.  Returning
					// pszBuffer is an error condition.
					memmove(pszHead + 1, pszHead, pszBuffer - pszHead);
					pszBuffer+=2;
					pszHead++;
				}
				// parse the IAC
				else {
					pszResult = ParseIAC(pszBuffer, pszBufferEnd);
					if(pszBuffer == pszResult) return pszBuffer;
					// see above regarding moving from front to end.
					memmove(pszHead + (pszResult - pszBuffer), pszHead,
						pszBuffer - pszHead);
					pszHead += (pszResult - pszBuffer);
					pszBuffer = pszResult;
				}
			}
			// else copy char over to ANSI buffer
			else {
				pszBuffer++;
			}
		}

	// Not a socket connection, so don't parse out IACs.
	// (Paul Brannan 3/19/99)
	} else {
		pszBuffer = pszBufferEnd;
	}

	return(Parser.ParseBuffer(pszHead, pszBuffer));
}

// telProcessNetwork calls the member function TTelnetHandler::Go, since
// TTelnetHandler::Go is not a static function, and cannot be called with
// CreateThread().  (Paul Brannan 6/15/98)
DWORD telProcessNetwork(LPVOID pvParams) {
	TelThreadParams *pParams = (TelThreadParams *)pvParams;
	return pParams->TelHandler.Go(&pParams->p);
}

// This function is what used to be telProcessNetwork (Paul Brannan 6/15/98)
DWORD TTelnetHandler::Go(LPVOID pvParams)
{
	NetParams *pParams = (NetParams *)pvParams;

	// No longer a need to copy pParams-> socket and create an instance
	// of TANSIParser (Paul Brannan 6/15/98)

	Console.sync(); // Sync with the parser so the cursor is positioned
	
	Parser.Init(); // Reset the parser (Paul Brannan 9/19/98)
	init(); // Turn on local echo (Paul Brannan 9/19/98)

	*pParams->bNetFinished = 0;
	char* pszHead = szBuffer;
	char* pszTail = szBuffer;
	while (!*pParams->bNetFinish) {
		// Get data from Socket
		*pParams->bNetPaused = 1;  //Pause
		int Result = Network.ReadString(pszTail, (szBuffer + dwBuffer) - pszTail);

		// Speed up mouse by not going into loop (Paul Brannan 8/10/98)
		// while(*pParams->bNetPause && !*pParams->bNetFinish) *pParams->bNetPaused = 1;  //Pause
		if(WaitForSingleObject(pParams->hPause, 0) == WAIT_OBJECT_0)
			WaitForSingleObject(pParams->hUnPause, INFINITE);

		*pParams->bNetPaused = 0;  //UnPause

		if (Result <= 0 || Result > dwBuffer ){
			break;
		}
		pszTail += Result;
		
		// Process the buffer
		char* pszNewHead = pszHead;
		do {
			// Speed up mouse by not going into loop (Paul Brannan 8/10/98)
			if(WaitForSingleObject(pParams->hPause, 0) == WAIT_OBJECT_0) {
				*pParams->bNetPaused = 1;
				WaitForSingleObject(pParams->hUnPause, INFINITE);
				*pParams->bNetPaused = 0;
			}
			
			pszHead = pszNewHead;
			pszNewHead = ParseBuffer(pszHead, pszTail); // Parse buffer
		} while ((pszNewHead != pszHead) && (pszNewHead < pszTail) && !*pParams->bNetFinish);
		pszHead = pszNewHead;
		
		// When we reach the end of the buffer, move contents to the
		// beginning of the buffer to get free space at the end.
		if (pszTail == (szBuffer + dwBuffer)) {
			memmove(szBuffer, pszHead, pszTail - pszHead);
			pszTail = szBuffer + (pszTail - pszHead);
			pszHead = szBuffer;
		}
	}
	SetEvent(pParams->hExit);

	printm(0, FALSE, MSG_TERMBYREM);
	*pParams->bNetPaused = 1;  //Pause
	*pParams->bNetFinished = 1;
	return 0;
}
