/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/setypes.h
 * PURPOSE:         Defintions for Security Subsystem Types not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _SETYPES_H
#define _SETYPES_H

/* DEPENDENCIES **************************************************************/

/* EXPORTED DATA *************************************************************/

/* CONSTANTS *****************************************************************/

/* ENUMERATIONS **************************************************************/

/* TYPES *********************************************************************/

typedef struct _SEP_AUDIT_POLICY_CATEGORIES 
{
    UCHAR System:4;
    UCHAR Logon:4;
    UCHAR ObjectAccess:4;
    UCHAR PrivilegeUse:4;
    UCHAR DetailedTracking:4;
    UCHAR PolicyChange:4;
    UCHAR AccountManagement:4;
    UCHAR DirectoryServiceAccess:4;
    UCHAR AccountLogon:4;
} SEP_AUDIT_POLICY_CATEGORIES, *PSEP_AUDIT_POLICY_CATEGORIES;

typedef struct _SEP_AUDIT_POLICY_OVERLAY 
{
    ULONGLONG PolicyBits:36;
    UCHAR SetBit:1;
} SEP_AUDIT_POLICY_OVERLAY, *PSEP_AUDIT_POLICY_OVERLAY;

typedef struct _SEP_AUDIT_POLICY 
{
    union 
    {
        SEP_AUDIT_POLICY_CATEGORIES PolicyElements;
        SEP_AUDIT_POLICY_OVERLAY PolicyOverlay;
        ULONGLONG Overlay;
    };
} SEP_AUDIT_POLICY, *PSEP_AUDIT_POLICY;
 
typedef struct _TOKEN 
{
  TOKEN_SOURCE        TokenSource;               /* 0x00 */
  LUID                TokenId;                   /* 0x10 */
  LUID                AuthenticationId;          /* 0x18 */
  LUID              ParentTokenId;             /* 0x20 */
  LARGE_INTEGER        ExpirationTime;            /* 0x28 */
  struct _ERESOURCE *TokenLock;                /* 0x30 */
  ULONG             Padding;                   /* 0x34 */
  SEP_AUDIT_POLICY  AuditPolicy;               /* 0x38 */
  LUID                ModifiedId;                /* 0x40 */
  ULONG             SessionId;                 /* 0x48 */
  ULONG                UserAndGroupCount;         /* 0x4C */
  ULONG             RestrictedSidCount;        /* 0x50 */
  ULONG                PrivilegeCount;            /* 0x54 */
  ULONG                VariableLength;            /* 0x58 */
  ULONG                DynamicCharged;            /* 0x5C */
  ULONG                DynamicAvailable;          /* 0x60 */
  ULONG                DefaultOwnerIndex;         /* 0x64 */
  PSID_AND_ATTRIBUTES UserAndGroups;           /* 0x68 */
  PSID_AND_ATTRIBUTES RestrictedSids;          /* 0x6C */
  PSID                PrimaryGroup;              /* 0x70 */
  PLUID_AND_ATTRIBUTES Privileges;             /* 0x74 */
  PULONG            DynamicPart;               /* 0x78 */
  PACL                DefaultDacl;               /* 0x7C */
  TOKEN_TYPE        TokenType;                 /* 0x80 */
  SECURITY_IMPERSONATION_LEVEL    ImpersonationLevel;  /* 0x84 */
  ULONG                TokenFlags;                /* 0x88 */
  ULONG                TokenInUse;                /* 0x8C */
  PVOID                ProxyData;                 /* 0x90 */
  PVOID                AuditData;                 /* 0x94 */
  LUID              OriginatingLogonSession;   /* 0x98 */
  UCHAR                VariablePart[1];           /* 0xA0 */
} TOKEN, *PTOKEN;

#endif
