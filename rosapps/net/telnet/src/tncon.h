#ifndef __TNCON_H
#define __TNCON_H

#include "tparams.h"
#include "tnclip.h"
#include "ttelhndl.h"

enum {
	SC_UP,
	SC_DOWN,
	SC_ESC,
	SC_MOUSE
};

enum {
	TNNOCON,
	TNPROMPT,
	TNSCROLLBACK,
	TNSPAWN,
	TNDONE
};

int telProcessConsole(NetParams *pParams, KeyTranslator &KeyTrans,
					  TConsole &Console, TNetwork &Network, TMouse &Mouse,
					  Tnclip &Clipboard, HANDLE hThread);
WORD scrollkeys ();

// Thomas Briggs 8/17/98
BOOL WINAPI ControlEventHandler(DWORD);

// Bryan Montgomery 10/14/98
void setTNetwork(TNetwork);

#endif