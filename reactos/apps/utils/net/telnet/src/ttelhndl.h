#ifndef __TTELHNDL_H
#define __TTELHNDL_H

#include "tparser.h"
#include "tnetwork.h"

DWORD telProcessNetwork(LPVOID pvParams);

class TTelnetHandler {
private:
	int iTermSet;
	int bInBinaryRx, bInBinaryTx;
	int bInEchoTx, bInEchoRx;
	int iWillSGA;

	void init();

	int escapeIAC(char *buf, int length);

	// Paul Brannan 5/15/98
	void SendIAC(char c);
	void SendIAC(char c1, char c2);
	void SendIACParams(char c);
	void SendIACParams(char c1, char c2);
	void print_telopt(const char *s, int d);

	TNetwork &Network;
	TConsole &Console;
	TParser &Parser;

	char* ParseBuffer(char* pszBuffer, char* pszBufferEnd);
	char* ParseIAC(char* pszBuffer, char* pszBufferEnd);

	// Paul Brannan 9/13/98
	char *szBuffer, *tmpBuffer;
	char *ansiBufferStart, *ansiBufferEnd;
	int dwBuffer;

	void do_naws(int width, int height);

public:
	TTelnetHandler(TNetwork &RefNetwork, TConsole &RefConsole,
		TParser &RefParser);
	~TTelnetHandler();

	DWORD Go(LPVOID pvParams);

	int get_term() {return iTermSet;}
};

#endif
