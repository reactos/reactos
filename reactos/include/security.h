#ifndef __INCLUDE_SECURITY_H
#define __INCLUDE_SECURITY_H

typedef ULONG SECURITY_INFORMATION, *PSECURITY_INFORMATION; 

typedef enum _TOKEN_INFORMATION_CLASS {
    TokenUser = 1, 
    TokenGroups, 
    TokenPrivileges, 
    TokenOwner, 
    TokenPrimaryGroup, 
    TokenDefaultDacl, 
    TokenSource, 
    TokenType, 
    TokenImpersonationLevel, 
    TokenStatistics 
} TOKEN_INFORMATION_CLASS; 

typedef ULONG SECURITY_IMPERSONATION_LEVEL, *PSECURITY_IMPERSONATION_LEVEL;

#define SecurityAnonymous ((SECURITY_IMPERSONATION_LEVEL)1)
#define SecurityIdentification ((SECURITY_IMPERSONATION_LEVEL)2)
#define SecurityImpersonation ((SECURITY_IMPERSONATION_LEVEL)3)
#define SecurityDelegation ((SECURITY_IMPERSONATION_LEVEL)4)

typedef ULONG TOKEN_TYPE, *PTOKEN_TYPE;

#define TokenPrimary           ((TOKEN_TYPE)1)
#define TokenImpersonation     ((TOKEN_TYPE)2)

typedef ULONG ACCESS_MASK;
typedef ULONG ACCESS_MODE, *PACCESS_MODE;

typedef struct _SECURITY_QUALITY_OF_SERVICE { 
  DWORD Length; 
  SECURITY_IMPERSONATION_LEVEL ImpersonationLevel; 
  /* SECURITY_CONTEXT_TRACKING_MODE ContextTrackingMode; */
  WINBOOL ContextTrackingMode; 
  BOOLEAN EffectiveOnly; 
} SECURITY_QUALITY_OF_SERVICE; 

typedef SECURITY_QUALITY_OF_SERVICE* PSECURITY_QUALITY_OF_SERVICE;

typedef struct _ACE_HEADER
{
   CHAR AceType;
   CHAR AceFlags;
   USHORT AceSize;
   ACCESS_MASK AccessMask;
} ACE_HEADER, *PACE_HEADER;

typedef struct
{
   ACE_HEADER Header;
} ACE, *PACE;

typedef struct _SID_IDENTIFIER_AUTHORITY
{
   BYTE Value[6];
} SID_IDENTIFIER_AUTHORITY, *PSID_IDENTIFIER_AUTHORITY;

#define SECURITY_WORLD_SID_AUTHORITY      {0,0,0,0,0,1}

typedef struct _SID 
{
   UCHAR  Revision;
   UCHAR  SubAuthorityCount;
   SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
   ULONG SubAuthority[1];
} SID, *PSID;

typedef struct _ACL {
  UCHAR AclRevision; 
  UCHAR Sbz1; 
  USHORT AclSize; 
  USHORT AceCount; 
  USHORT Sbz2; 
} ACL, *PACL; 

typedef USHORT SECURITY_DESCRIPTOR_CONTROL, *PSECURITY_DESCRIPTOR_CONTROL;

typedef struct _SECURITY_DESCRIPTOR_CONTEXT
{
} SECURITY_DESCRIPTOR_CONTEXT, *PSECURITY_DESCRIPTOR_CONTEXT;

typedef LARGE_INTEGER LUID, *PLUID;

typedef struct _SECURITY_DESCRIPTOR {
  UCHAR  Revision;
  UCHAR  Sbz1;
  SECURITY_DESCRIPTOR_CONTROL Control;
  PSID Owner;
  PSID Group;
  PACL Sacl;
  PACL Dacl;
} SECURITY_DESCRIPTOR, *PSECURITY_DESCRIPTOR;

typedef struct _LUID_AND_ATTRIBUTES 
{   
   LUID  Luid; 
   DWORD Attributes; 
} LUID_AND_ATTRIBUTES, *PLUID_AND_ATTRIBUTES;

typedef struct _TOKEN_SOURCE {
  CHAR SourceName[8]; 
  LUID SourceIdentifier; 
} TOKEN_SOURCE, *PTOKEN_SOURCE; 

typedef struct _SID_AND_ATTRIBUTES { 
  PSID  Sid; 
  DWORD Attributes; 
} SID_AND_ATTRIBUTES, *PSID_AND_ATTRIBUTES;
 
typedef SID_AND_ATTRIBUTES SID_AND_ATTRIBUTES_ARRAY[ANYSIZE_ARRAY];
typedef SID_AND_ATTRIBUTES_ARRAY *PSID_AND_ATTRIBUTES_ARRAY;

typedef struct _TOKEN_USER { 
  SID_AND_ATTRIBUTES User; 
} TOKEN_USER, *PTOKEN_USER; 

typedef struct _TOKEN_PRIMARY_GROUP { 
  PSID PrimaryGroup; 
} TOKEN_PRIMARY_GROUP, *PTOKEN_PRIMARY_GROUP; 

typedef struct _TOKEN_GROUPS { 
  DWORD GroupCount; 
  SID_AND_ATTRIBUTES Groups[ANYSIZE_ARRAY]; 
} TOKEN_GROUPS, *PTOKEN_GROUPS, *LPTOKEN_GROUPS; 

typedef struct _TOKEN_PRIVILEGES { 
  DWORD PrivilegeCount; 
  LUID_AND_ATTRIBUTES Privileges[ANYSIZE_ARRAY]; 
} TOKEN_PRIVILEGES, *PTOKEN_PRIVILEGES, *LPTOKEN_PRIVILEGES; 

typedef struct _TOKEN_OWNER { 
  PSID Owner; 
} TOKEN_OWNER, *PTOKEN_OWNER; 

typedef struct _TOKEN_DEFAULT_DACL {  
  PACL DefaultDacl; 
} TOKEN_DEFAULT_DACL, *PTOKEN_DEFAULT_DACL;

typedef struct _TOKEN_STATISTICS { 
  LUID  TokenId; 
  LUID  AuthenticationId; 
  LARGE_INTEGER ExpirationTime; 
  TOKEN_TYPE    TokenType; 
  SECURITY_IMPERSONATION_LEVEL ImpersonationLevel; 
  DWORD DynamicCharged; 
  DWORD DynamicAvailable; 
  DWORD GroupCount; 
  DWORD PrivilegeCount; 
  LUID  ModifiedId; 
} TOKEN_STATISTICS, *PTOKEN_STATISTICS; 

#endif /* __INCLUDE_SECURITY_H */
