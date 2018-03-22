
#ifndef __CRYPT32_PRECOMP_H__
#define __CRYPT32_PRECOMP_H__

#include <wine/config.h>
#include <wine/port.h>

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define NONAMELESSUNION

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wine/winternl.h>
#include <winuser.h>
#include <winreg.h>
#include <snmp.h>

#define CERT_CHAIN_PARA_HAS_EXTRA_FIELDS
#define CERT_REVOCATION_PARA_HAS_EXTRA_FIELDS
#define CRYPT_OID_INFO_HAS_EXTRA_FIELDS
#include <wincrypt.h>

#include <mssip.h>

#include <wine/unicode.h>
#include <wine/exception.h>
#include <wine/debug.h>

#include "crypt32_private.h"
#include "cryptres.h"

#endif /* !__CRYPT32_PRECOMP_H__ */
