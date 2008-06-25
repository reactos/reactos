// A TParser is a class for parsing input and formatting it (presumabyl for
// display on the screen).  All parsers are derived from the TParser class,
// in order to facilitate extending telnet to include other kinds of
// output.  Currently, only one parser is implemented, the ANSI parser.
// A TParser includes:
//   - A ParseBuffer function, which takes as parameters start and end
//     pointers.  It returns a pointer to the last character parsed plus 1.
//     The start pointer is the beginning of the buffer, and the end
//     pointer is one character after the end of the buffer.
//   - An Init() function, which will re-initialize the parser when
//     necessary.

#ifndef __TPARSER_H
#define __TPARSER_H

#include "tconsole.h"
#include "keytrans.h"
#include "tscroll.h"
#include "tnetwork.h"
#include "tcharmap.h"

class TParser {
public:
	TParser(TConsole &RefConsole, KeyTranslator &RefKeyTrans,
		TScroller &RefScroller, TNetwork &RefNetwork, TCharmap &RefCharmap) :
	Console(RefConsole), KeyTrans(RefKeyTrans), Scroller (RefScroller),
	Network(RefNetwork), Charmap(RefCharmap) {}
	virtual ~TParser() {}

/*	TParser& operator= (const TParser &p) {
		Console = p.Console;
		KeyTrans = p.KeyTrans;
		Scroller = p.Scroller;
		Network = p.Network;
		return *this;
	}*/

	virtual char *ParseBuffer(char *pszBuffer, char *pszBufferEnd) = 0;
	virtual void Init() = 0;

protected:
	TConsole &Console;
	KeyTranslator &KeyTrans;
	TScroller &Scroller;
	TNetwork &Network;
	TCharmap &Charmap;
};

#endif
