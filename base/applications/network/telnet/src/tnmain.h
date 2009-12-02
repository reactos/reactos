#ifndef __TNMAIN_H
#define __TNMAIN_H

#include <stdlib.h>
#include <process.h>
#include "tncon.h"
#include "tnclass.h"
#include "ttelhndl.h"
#include "tnerror.h"
// Paul Brannan 5/25/98
#include "tnconfig.h"

struct cmdHistory {
	char cmd[80];
	struct cmdHistory *next;
	struct cmdHistory *prev;
};

#endif
