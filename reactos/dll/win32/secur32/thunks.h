/* Copyright (C) 2004 Juan Lang
 *
 * This file defines thunks between wide char and multibyte functions for
 * SSPs that only provide one or the other.
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

#ifndef __SECUR32_THUNKS_H__
#define __SECUR32_THUNKS_H__

/* Prototypes for functions that thunk between wide char and multibyte versions,
 * for SSPs that only provide one or the other.
 */
SECURITY_STATUS SEC_ENTRY thunk_AcquireCredentialsHandleA(
 SEC_CHAR *pszPrincipal, SEC_CHAR *pszPackage, ULONG fCredentialsUse,
 PLUID pvLogonID, PVOID pAuthData, SEC_GET_KEY_FN pGetKeyFn,
 PVOID pvGetKeyArgument, PCredHandle phCredential, PTimeStamp ptsExpiry);
SECURITY_STATUS SEC_ENTRY thunk_AcquireCredentialsHandleW(
 SEC_WCHAR *pszPrincipal, SEC_WCHAR *pszPackage, ULONG fCredentialsUse,
 PLUID pvLogonID, PVOID pAuthData, SEC_GET_KEY_FN pGetKeyFn,
 PVOID pvGetKeyArgument, PCredHandle phCredential, PTimeStamp ptsExpiry);
SECURITY_STATUS SEC_ENTRY thunk_InitializeSecurityContextA(
 PCredHandle phCredential, PCtxtHandle phContext,
 SEC_CHAR *pszTargetName, ULONG fContextReq,
 ULONG Reserved1, ULONG TargetDataRep, PSecBufferDesc pInput,
 ULONG Reserved2, PCtxtHandle phNewContext, PSecBufferDesc pOutput,
 ULONG *pfContextAttr, PTimeStamp ptsExpiry);
SECURITY_STATUS SEC_ENTRY thunk_InitializeSecurityContextW(
 PCredHandle phCredential, PCtxtHandle phContext,
 SEC_WCHAR *pszTargetName, ULONG fContextReq,
 ULONG Reserved1, ULONG TargetDataRep, PSecBufferDesc pInput,
 ULONG Reserved2, PCtxtHandle phNewContext, PSecBufferDesc pOutput,
 ULONG *pfContextAttr, PTimeStamp ptsExpiry);
SECURITY_STATUS SEC_ENTRY thunk_ImportSecurityContextA(
 SEC_CHAR *pszPackage, PSecBuffer pPackedContext, void *Token,
 PCtxtHandle phContext);
SECURITY_STATUS SEC_ENTRY thunk_ImportSecurityContextW(
 SEC_WCHAR *pszPackage, PSecBuffer pPackedContext, void *Token,
 PCtxtHandle phContext);
SECURITY_STATUS SEC_ENTRY thunk_AddCredentialsA(PCredHandle hCredentials,
 SEC_CHAR *pszPrincipal, SEC_CHAR *pszPackage, ULONG fCredentialUse,
 void *pAuthData, SEC_GET_KEY_FN pGetKeyFn, void *pvGetKeyArgument,
 PTimeStamp ptsExpiry);
SECURITY_STATUS SEC_ENTRY thunk_AddCredentialsW(PCredHandle hCredentials,
 SEC_WCHAR *pszPrincipal, SEC_WCHAR *pszPackage, ULONG fCredentialUse,
 void *pAuthData, SEC_GET_KEY_FN pGetKeyFn, void *pvGetKeyArgument,
 PTimeStamp ptsExpiry);
SECURITY_STATUS SEC_ENTRY thunk_QueryCredentialsAttributesA(
 PCredHandle phCredential, ULONG ulAttribute, void *pBuffer);
SECURITY_STATUS SEC_ENTRY thunk_QueryCredentialsAttributesW(
 PCredHandle phCredential, ULONG ulAttribute, void *pBuffer);
SECURITY_STATUS SEC_ENTRY thunk_QueryContextAttributesA(
 PCtxtHandle phContext, ULONG ulAttribute, void *pBuffer);
SECURITY_STATUS SEC_ENTRY thunk_QueryContextAttributesW(
 PCtxtHandle phContext, ULONG ulAttribute, void *pBuffer);
SECURITY_STATUS SEC_ENTRY thunk_SetContextAttributesA(PCtxtHandle phContext,
 ULONG ulAttribute, void *pBuffer, ULONG cbBuffer);
SECURITY_STATUS SEC_ENTRY thunk_SetContextAttributesW(PCtxtHandle phContext,
 ULONG ulAttribute, void *pBuffer, ULONG cbBuffer);

#endif /* ndef __SECUR32_THUNKS_H__ */
