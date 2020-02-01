#ifndef _MSV1_0_PRECOMP_H_
#define _MSV1_0_PRECOMP_H_

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <wchar.h>
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <winreg.h>
#include <wincrypt.h>
#define NTOS_MODE_USER
#include <ndk/cmfuncs.h>
#include <ndk/kefuncs.h>
#include <ndk/lpctypes.h>
#include <ndk/lpcfuncs.h>
#include <ndk/mmfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ndk/setypes.h>
#include <ndk/sefuncs.h>

#include <lmcons.h>
#include <lmjoin.h>
#include <lmwksta.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <sspi.h>
#include <ntsecapi.h>
#include <ntsecpkg.h>
#include <ntsam.h>
#include <ntlsa.h>

#include <samsrv/samsrv.h>
//#include <lsass/lsasrv.h>

#include "lsa.h"
#include "msv1_0.h"
#include "ntlm/ciphers.h"
#include "ntlm/strutil.h"
#include "ntlm/ntlmssp.h"
#include "ntlm/context.h"
#include "ntlm/protocol.h"
#include "ntlm/credentials.h"
#include "ntlm/messages.h"
#include "splsa.h"
#include "spuser.h"
#include "usercontext.h"

#include "pseh/pseh2.h"

#include <wine/debug.h>

#endif
