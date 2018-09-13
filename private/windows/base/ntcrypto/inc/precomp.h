#undef UNICODE					// ## Not Yet
#include <windows.h>
#include <windef.h>
#include <malloc.h>
#include <string.h>
#include <time.h>
#include <wtypes.h>

#ifndef WIN95
#include "assert.h"
#endif
#ifdef SECDBG					// ITV Security
#define	NTAGDEBUG				// Turn on internal debugging
#else	// SECDBG
#ifndef ASSERT
#define ASSERT(x)				// default to base
#endif
#endif	// SECDBG

#include "scp.h"
#include "rsa.h"
#include "contman.h"
#include "ntagimp1.h"
#include "manage.h"

#pragma	hdrstop
