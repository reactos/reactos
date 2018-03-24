
#ifndef _WINLDAP_PRECOMP_H_
#define _WINLDAP_PRECOMP_H_

#include <wine/config.h>

#include <stdarg.h>

#ifdef HAVE_LDAP_H
#include <ldap.h>
#endif

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>

#include <wine/debug.h>

#include "winldap_private.h"
#include "wldap32.h"

#endif /* !_WINLDAP_PRECOMP_H_ */
