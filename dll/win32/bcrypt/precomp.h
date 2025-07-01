
#ifndef _BCRYPT_PRECOMP_H
#define _BCRYPT_PRECOMP_H

#include <wine/config.h>
#include <wine/port.h>

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <ntsecapi.h>
#include <wincrypt.h>
#include <bcrypt.h>

#include <wine/debug.h>
#include <wine/unicode.h>
#include <wine/library.h>

#if defined(HAVE_GNUTLS_HASH)
#include <gnutls/gnutls.h>
#elif defined(SONAME_LIBMBEDTLS)
#include <mbedtls/md.h>
#include <mbedtls/cipher.h>
#include <mbedtls/pk.h>
#include <mbedtls/dhm.h>
#endif

#include "bcrypt_internal.h"

#endif /* !_BCRYPT_PRECOMP_H */
