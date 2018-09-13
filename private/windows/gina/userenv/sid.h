//*************************************************************
//
//  Header file for Sid.c
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

LPTSTR GetSidString(HANDLE UserToken);
VOID DeleteSidString(LPTSTR SidString);
PSID GetUserSid (HANDLE UserToken);
VOID DeleteUserSid(PSID Sid);
NTSTATUS AllocateAndInitSidFromString (const WCHAR* lpszSidStr, PSID* ppSid);
NTSTATUS LoadSidAuthFromString (const WCHAR* pString, PSID_IDENTIFIER_AUTHORITY pSidAuth);
NTSTATUS GetIntFromUnicodeString (const WCHAR* szNum, ULONG Base, PULONG pValue);
NTSTATUS GetDomainSidFromDomainRid(PSID pSid, DWORD dwRid, PSID *ppNewSid);

