#ifndef __TSCRIPT_H
#define __TSCRIPT_H

#include <windows.h>
#include <stdio.h>
#include "tnetwork.h"

class TScript {
public:
	TScript(TNetwork &RefNetwork):Network(RefNetwork) {fp = NULL;}
	~TScript() {}
	BOOL processScript(char *data);
	void initScript(char *filename);
private:
	FILE *fp;
	char *script;
	TNetwork &Network;
};

#endif
