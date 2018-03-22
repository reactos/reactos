
#ifndef _CRYPT32_WINETEST_PRECOMP_H_
#define _CRYPT32_WINETEST_PRECOMP_H_

#include <stdio.h>

#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <ntstatus.h>
#define WIN32_NO_STATUS

#include <wine/test.h>
#include <winreg.h>

#define CMSG_SIGNER_ENCODE_INFO_HAS_CMS_FIELDS
#define CMSG_SIGNED_ENCODE_INFO_HAS_CMS_FIELDS
#define CRYPT_OID_INFO_HAS_EXTRA_FIELDS
#include <wincrypt.h>

#endif /* !_CRYPT32_WINETEST_PRECOMP_H_ */
