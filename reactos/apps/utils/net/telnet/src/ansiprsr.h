#ifndef __ANSIPRSR_H
#define __ANSIPRSR_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "tnconfig.h"
#include "tparser.h"

// added this color table to make things go faster (Paul Branann 5/8/98)
enum Colors {BLACK=0, BLUE, GREEN, CYAN, RED, MAGENTA, YELLOW, WHITE};
static const int ANSIColors[] = {BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE};

// This should be greater than the largest conceivable window size
// 200 should suffice
#define MAX_TAB_POSITIONS 200

// Added by Frediano Ziglio 6/2/2000
// Include Meridian Emulator support
// undefine it to remove support
#define MTE_SUPPORT 1

// TANSIParser is now properly no longer a base class for TTelnetParser.
// Screen output is handled in TConsole.cpp.
// (Paul Brannan 6/15/98)
class TANSIParser : public TParser {
private:
	char* ParseEscapeANSI(char* pszBuffer, char* pszBufferEnd);
	char* ParseANSIBuffer(char* pszBuffer, char* pszBufferEnd);
	char* ParseEscape(char* pszBuffer, char* pszBufferEnd);
	// Added by I.Ioannou 06/04/97
	char* PrintBuffer(char* pszBuffer, char* pszBufferEnd);
	char* PrintGoodChars(char * pszHead, char * pszTail);

#ifdef MTE_SUPPORT
    // Added by Frediano Ziglio, 5/31/2000
    char* ParseEscapeMTE(char* pszBuffer, char* pszBufferEnd);
	short int mteRegionXF,mteRegionYF;
#endif

	void ConSetAttribute(unsigned char wAttr);
	char *GetTerminalID();
	void ConSetCursorPos(int x, int y);
	void ResetTerminal();
	void Init();

	void SaveCurX(int iX);
	void SaveCurY(int iY);

	void resetTabStops();
	
	int iSavedCurX;
	int iSavedCurY;
	unsigned char iSavedAttributes;
	FILE * dumpfile;

	// Added by I.Ioannou 06 April 1997
	FILE * printfile;
	char InPrintMode;
	int inGraphMode;

	char last_char;                 // TITUS++: 2. November 98

	char map_G0, map_G1;
	int current_map;
	bool vt52_mode;
	bool print_ctrl;
	bool ignore_margins;
	bool fast_write;
	bool newline_mode;

	int tab_stops[MAX_TAB_POSITIONS];

public:
	// Changed by Paul Brannan 5/13/98
	TANSIParser(TConsole &Console, KeyTranslator &RefKeyTrans,
		TScroller &RefScroller, TNetwork &NetHandler, TCharmap &RefCharmap);
	~TANSIParser();
	
	char* ParseBuffer(char* pszBuffer, char* pszBufferEnd);
	static int StripBuffer(char* pszBuffer, char* pszBufferEnd, int width);
};

#endif
