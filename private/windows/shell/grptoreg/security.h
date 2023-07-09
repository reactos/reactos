/****************************** Module Header ******************************\
* Module Name: security.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Define various winlogon security-related routines
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/


BOOL
SetWorldSecurity(
    PSID    UserSid,
    PSECURITY_DESCRIPTOR *pSecDesc,
    BOOL bCommonGroupAccess
    );

BOOL InitializeSecurityAttributes(
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    BOOL bCommonGroupAccess
    );

