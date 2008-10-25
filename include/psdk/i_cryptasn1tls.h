/*
 * Copyright (C) 2007 Francois Gouget
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_I_CRYPTASN1TLS_H
#define __WINE_I_CRYPTASN1TLS_H

typedef void *ASN1decoding_t;
typedef void *ASN1encoding_t;
typedef void *ASN1module_t;
typedef DWORD HCRYPTASN1MODULE;

#ifdef __cplusplus
extern "C" {
#endif

ASN1decoding_t WINAPI I_CryptGetAsn1Decoder(HCRYPTASN1MODULE);
ASN1encoding_t WINAPI I_CryptGetAsn1Encoder(HCRYPTASN1MODULE);
BOOL        WINAPI I_CryptInstallAsn1Module(ASN1module_t, DWORD, void*);
BOOL        WINAPI I_CryptUninstallAsn1Module(HCRYPTASN1MODULE);

#ifdef __cplusplus
}
#endif

#endif  /* __WINE_I_CRYPTASN1TLS_H */
