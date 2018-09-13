#ifndef _MSNSPMH_H_
#define _MSNSPMH_H_

#define SECURITY_WIN32  1

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <windows.h>
#include <winerror.h>
#include <rpc.h>
#include <stdarg.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>

#include <sspi.h>
#include <issperr.h>
#include <ntlmsp.h>
//#include <crypt.h>
//#include <ntlmsspi.h>
#include <msnssp.h>
#include <wininet.h>
#include <spluginx.hxx>
#include "htuu.h"
#include "sspspm.h"
#include "winctxt.h"

extern SspData  *g_pSspData;
LPVOID SSPI_InitGlobals(void);

#endif  // _MSNSPMH_H_
