#ifndef __THREADPARAMS
#define __THREADPARAMS

#include "ttelhndl.h"

typedef struct {
	HANDLE hExit;
	HANDLE hPause, hUnPause;
	volatile int *bNetPaused;
	volatile int *bNetFinished;
	volatile int *bNetFinish;
} NetParams;

// We could make TelHandler a pointer rather than a reference, but making it
// a reference forces us to initialize it when it is created, thus avoiding
// a possible segfault (Paul Brannan 6/15/98)
class TelThreadParams {
public:
	TelThreadParams(TTelnetHandler &RefTelHandler): TelHandler(RefTelHandler) {}
	NetParams p;
	TTelnetHandler &TelHandler;
};

#endif
