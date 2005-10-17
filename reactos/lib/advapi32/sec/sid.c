/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * WINE COPYRIGHT:
 * Copyright 1999, 2000 Juergen Schmied <juergen.schmied@debitel.net>
 * Copyright 2003 CodeWeavers Inc. (Ulrich Czekalla)
 *
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/sec/sid.c
 * PURPOSE:         Security ID functions
 */

#include <advapi32.h>
#include <wine/debug.h>
#include <wine/unicode.h>

WINE_DEFAULT_DEBUG_CHANNEL(advapi);


static BOOL ParseStringSidToSid(LPCWSTR StringSid, PSID pSid, LPDWORD cBytes);
static BOOL ParseStringAclToAcl(LPCWSTR StringAcl, LPDWORD lpdwFlags, 
    PACL pAcl, LPDWORD cBytes);
static BYTE ParseAceStringFlags(LPCWSTR* StringAcl);
static BYTE ParseAceStringType(LPCWSTR* StringAcl);
static DWORD ParseAceStringRights(LPCWSTR* StringAcl);
static DWORD ParseAclStringFlags(LPCWSTR* StringAcl);

typedef struct _ACEFLAG
{
   LPCWSTR wstr;
   DWORD value;
} ACEFLAG, *LPACEFLAG;

static SID const sidWorld = { SID_REVISION, 1, { SECURITY_WORLD_SID_AUTHORITY} , { SECURITY_WORLD_RID } };

/*
 * ACE access rights
 */
static const WCHAR SDDL_READ_CONTROL[]     = {'R','C',0};
static const WCHAR SDDL_WRITE_DAC[]        = {'W','D',0};
static const WCHAR SDDL_WRITE_OWNER[]      = {'W','O',0};
static const WCHAR SDDL_STANDARD_DELETE[]  = {'S','D',0};
static const WCHAR SDDL_GENERIC_ALL[]      = {'G','A',0};
static const WCHAR SDDL_GENERIC_READ[]     = {'G','R',0};
static const WCHAR SDDL_GENERIC_WRITE[]    = {'G','W',0};
static const WCHAR SDDL_GENERIC_EXECUTE[]  = {'G','X',0};

/*
 * ACE types
 */
static const WCHAR SDDL_ACCESS_ALLOWED[]        = {'A',0};
static const WCHAR SDDL_ACCESS_DENIED[]         = {'D',0};
static const WCHAR SDDL_OBJECT_ACCESS_ALLOWED[] = {'O','A',0};
static const WCHAR SDDL_OBJECT_ACCESS_DENIED[]  = {'O','D',0};
static const WCHAR SDDL_AUDIT[]                 = {'A','U',0};
static const WCHAR SDDL_ALARM[]                 = {'A','L',0};
static const WCHAR SDDL_OBJECT_AUDIT[]          = {'O','U',0};
static const WCHAR SDDL_OBJECT_ALARMp[]         = {'O','L',0};

/*
 * ACE flags
 */
static const WCHAR SDDL_CONTAINER_INHERIT[]  = {'C','I',0};
static const WCHAR SDDL_OBJECT_INHERIT[]     = {'O','I',0};
static const WCHAR SDDL_NO_PROPAGATE[]       = {'N','P',0};
static const WCHAR SDDL_INHERIT_ONLY[]       = {'I','O',0};
static const WCHAR SDDL_INHERITED[]          = {'I','D',0};
static const WCHAR SDDL_AUDIT_SUCCESS[]      = {'S','A',0};
static const WCHAR SDDL_AUDIT_FAILURE[]      = {'F','A',0};

/* set last error code from NT status and get the proper boolean return value */
/* used for functions that are a simple wrapper around the corresponding ntdll API */
static inline BOOL set_ntstatus( NTSTATUS status )
{
    if (status) SetLastError( RtlNtStatusToDosError( status ));
    return !status;
}

#define	WINE_SIZE_OF_WORLD_ACCESS_ACL	(sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + sizeof(sidWorld) - sizeof(DWORD))


/* some helper functions - taken from winehq cvs 20050916 */
/******************************************************************************
 * ComputeStringSidSize
 */
static DWORD ComputeStringSidSize(LPCWSTR StringSid)
{
    int ctok = 0;
    DWORD size = sizeof(SID);

    while (*StringSid)
    {
        if (*StringSid == '-')
            ctok++;
        StringSid++;
    }

    if (ctok > 3)
        size += (ctok - 3) * sizeof(DWORD);

    return size;
}

/******************************************************************************
 * ParseAceStringType
 */
ACEFLAG AceType[] =
{
    { SDDL_ACCESS_ALLOWED, ACCESS_ALLOWED_ACE_TYPE },
    { SDDL_ALARM,          SYSTEM_ALARM_ACE_TYPE },
    { SDDL_AUDIT,          SYSTEM_AUDIT_ACE_TYPE },
    { SDDL_ACCESS_DENIED,  ACCESS_DENIED_ACE_TYPE },
    /*
    { SDDL_OBJECT_ACCESS_ALLOWED, ACCESS_ALLOWED_OBJECT_ACE_TYPE },
    { SDDL_OBJECT_ACCESS_DENIED,  ACCESS_DENIED_OBJECT_ACE_TYPE },
    { SDDL_OBJECT_ALARM,          SYSTEM_ALARM_OBJECT_ACE_TYPE },
    { SDDL_OBJECT_AUDIT,          SYSTEM_AUDIT_OBJECT_ACE_TYPE },
    */
    { NULL, 0 },
};

static BYTE ParseAceStringType(LPCWSTR* StringAcl)
{
    UINT len = 0;
    LPCWSTR szAcl = *StringAcl;
    LPACEFLAG lpaf = AceType;

    while (lpaf->wstr &&
        (len = strlenW(lpaf->wstr)) &&
        strncmpW(lpaf->wstr, szAcl, len))
        lpaf++;

    if (!lpaf->wstr)
        return 0;

    *StringAcl += len;
    return lpaf->value;
}


/******************************************************************************
 * ParseAceStringFlags
 */
ACEFLAG AceFlags[] =
{
    { SDDL_CONTAINER_INHERIT, CONTAINER_INHERIT_ACE },
    { SDDL_AUDIT_FAILURE,     FAILED_ACCESS_ACE_FLAG },
    { SDDL_INHERITED,         INHERITED_ACE },
    { SDDL_INHERIT_ONLY,      INHERIT_ONLY_ACE },
    { SDDL_NO_PROPAGATE,      NO_PROPAGATE_INHERIT_ACE },
    { SDDL_OBJECT_INHERIT,    OBJECT_INHERIT_ACE },
    { SDDL_AUDIT_SUCCESS,     SUCCESSFUL_ACCESS_ACE_FLAG },
    { NULL, 0 },
};

static BYTE ParseAceStringFlags(LPCWSTR* StringAcl)
{
    UINT len = 0;
    BYTE flags = 0;
    LPCWSTR szAcl = *StringAcl;

    while (*szAcl != ';')
    {
        LPACEFLAG lpaf = AceFlags;

        while (lpaf->wstr &&
               (len = strlenW(lpaf->wstr)) &&
               strncmpW(lpaf->wstr, szAcl, len))
            lpaf++;

        if (!lpaf->wstr)
            return 0;

	flags |= lpaf->value;
        szAcl += len;
    }

    *StringAcl = szAcl;
    return flags;
}


/******************************************************************************
 * ParseAceStringRights
 */
ACEFLAG AceRights[] =
{
    { SDDL_GENERIC_ALL,     GENERIC_ALL },
    { SDDL_GENERIC_READ,    GENERIC_READ },
    { SDDL_GENERIC_WRITE,   GENERIC_WRITE },
    { SDDL_GENERIC_EXECUTE, GENERIC_EXECUTE },
    { SDDL_READ_CONTROL,    READ_CONTROL },
    { SDDL_STANDARD_DELETE, DELETE },
    { SDDL_WRITE_DAC,       WRITE_DAC },
    { SDDL_WRITE_OWNER,     WRITE_OWNER },
    { NULL, 0 },
};

static DWORD ParseAceStringRights(LPCWSTR* StringAcl)
{
    UINT len = 0;
    DWORD rights = 0;
    LPCWSTR szAcl = *StringAcl;

    if ((*szAcl == '0') && (*(szAcl + 1) == 'x'))
    {
        LPCWSTR p = szAcl;

	while (*p && *p != ';')
            p++;

	if (p - szAcl <= 8)
	{
	    rights = strtoulW(szAcl, NULL, 16);
	    *StringAcl = p;
	}
	else
            WARN("Invalid rights string format: %s\n", debugstr_wn(szAcl, p - szAcl));
    }
    else
    {
        while (*szAcl != ';')
        {
            LPACEFLAG lpaf = AceRights;

            while (lpaf->wstr &&
               (len = strlenW(lpaf->wstr)) &&
               strncmpW(lpaf->wstr, szAcl, len))
	    {
               lpaf++;
	    }

            if (!lpaf->wstr)
                return 0;

	    rights |= lpaf->value;
            szAcl += len;
        }
    }

    *StringAcl = szAcl;
    return rights;
}

/******************************************************************************
 * ParseStringAclToAcl
 * 
 * dacl_flags(string_ace1)(string_ace2)... (string_acen) 
 */
static BOOL ParseStringAclToAcl(LPCWSTR StringAcl, LPDWORD lpdwFlags, 
    PACL pAcl, LPDWORD cBytes)
{
    DWORD val;
    DWORD sidlen;
    DWORD length = sizeof(ACL);
    PACCESS_ALLOWED_ACE pAce = NULL; /* pointer to current ACE */

    TRACE("%s\n", debugstr_w(StringAcl));

    if (!StringAcl)
	return FALSE;

    if (pAcl) /* pAce is only useful if we're setting values */
        pAce = (PACCESS_ALLOWED_ACE) ((LPBYTE)pAcl + sizeof(PACL));

    /* Parse ACL flags */
    *lpdwFlags = ParseAclStringFlags(&StringAcl);

    /* Parse ACE */
    while (*StringAcl == '(')
    {
        StringAcl++;

        /* Parse ACE type */
        val = ParseAceStringType(&StringAcl);
	if (pAce)
            pAce->Header.AceType = (BYTE) val;
        if (*StringAcl != ';')
            goto lerr;
        StringAcl++;

        /* Parse ACE flags */
	val = ParseAceStringFlags(&StringAcl);
	if (pAce)
            pAce->Header.AceFlags = (BYTE) val;
        if (*StringAcl != ';')
            goto lerr;
        StringAcl++;

        /* Parse ACE rights */
	val = ParseAceStringRights(&StringAcl);
	if (pAce)
            pAce->Mask = val;
        if (*StringAcl != ';')
            goto lerr;
        StringAcl++;

        /* Parse ACE object guid */
        if (*StringAcl != ';')
        {
            FIXME("Support for *_OBJECT_ACE_TYPE not implemented\n");
            goto lerr;
        }
        StringAcl++;

        /* Parse ACE inherit object guid */
        if (*StringAcl != ';')
        {
            FIXME("Support for *_OBJECT_ACE_TYPE not implemented\n");
            goto lerr;
        }
        StringAcl++;

        /* Parse ACE account sid */
        if (ParseStringSidToSid(StringAcl, pAce ? (PSID)&pAce->SidStart : NULL, &sidlen))
	{
            while (*StringAcl && *StringAcl != ')')
                StringAcl++;
	}

        if (*StringAcl != ')')
            goto lerr;
        StringAcl++;

	length += sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + sidlen;
    }

    *cBytes = length;
    return TRUE;

lerr:
    WARN("Invalid ACE string format\n");
    return FALSE;
}

/******************************************************************************
 * ParseStringSecurityDescriptorToSecurityDescriptor
 */
static BOOL ParseStringSecurityDescriptorToSecurityDescriptor(
    LPCWSTR StringSecurityDescriptor,
    SECURITY_DESCRIPTOR* SecurityDescriptor,
    LPDWORD cBytes)
{
    BOOL bret = FALSE;
    WCHAR toktype;
    WCHAR tok[MAX_PATH];
    LPCWSTR lptoken;
    LPBYTE lpNext = NULL;
    DWORD len;

    *cBytes = 0;

    if (SecurityDescriptor)
        lpNext = ((LPBYTE) SecurityDescriptor) + sizeof(SECURITY_DESCRIPTOR);

    while (*StringSecurityDescriptor)
    {
        toktype = *StringSecurityDescriptor;

	/* Expect char identifier followed by ':' */
	StringSecurityDescriptor++;
        if (*StringSecurityDescriptor != ':')
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto lend;
        }
	StringSecurityDescriptor++;

	/* Extract token */
	lptoken = StringSecurityDescriptor;
	while (*lptoken && *lptoken != ':')
            lptoken++;

	if (*lptoken)
            lptoken--;

        len = lptoken - StringSecurityDescriptor;
        memcpy( tok, StringSecurityDescriptor, len * sizeof(WCHAR) );
        tok[len] = 0;

        switch (toktype)
	{
            case 'O':
            {
                DWORD bytes;

                if (!ParseStringSidToSid(tok, (PSID)lpNext, &bytes))
                    goto lend;

                if (SecurityDescriptor)
                {
                    SecurityDescriptor->Owner = (PSID) ((DWORD) lpNext -
                        (DWORD) SecurityDescriptor);
                    lpNext += bytes; /* Advance to next token */
                }

		*cBytes += bytes;

                break;
            }

            case 'G':
            {
                DWORD bytes;

                if (!ParseStringSidToSid(tok, (PSID)lpNext, &bytes))
                    goto lend;

                if (SecurityDescriptor)
                {
                    SecurityDescriptor->Group = (PSID) ((DWORD) lpNext - 
                        (DWORD) SecurityDescriptor);
                    lpNext += bytes; /* Advance to next token */
                }

		*cBytes += bytes;

                break;
            }

            case 'D':
	    {
                DWORD flags;
                DWORD bytes;

                if (!ParseStringAclToAcl(tok, &flags, (PACL)lpNext, &bytes))
                    goto lend;

                if (SecurityDescriptor)
                {
                    SecurityDescriptor->Control |= SE_DACL_PRESENT | flags;
                    SecurityDescriptor->Dacl = (PACL) ((DWORD) lpNext -
                        (DWORD) SecurityDescriptor);
                    lpNext += bytes; /* Advance to next token */
		}

		*cBytes += bytes;

		break;
            }

            case 'S':
            {
                DWORD flags;
                DWORD bytes;

                if (!ParseStringAclToAcl(tok, &flags, (PACL)lpNext, &bytes))
                    goto lend;

                if (SecurityDescriptor)
                {
                    SecurityDescriptor->Control |= SE_SACL_PRESENT | flags;
                    SecurityDescriptor->Sacl = (PACL) ((DWORD) lpNext -
                        (DWORD) SecurityDescriptor);
                    lpNext += bytes; /* Advance to next token */
		}

		*cBytes += bytes;

		break;
            }

            default:
                FIXME("Unknown token\n");
                SetLastError(ERROR_INVALID_PARAMETER);
		goto lend;
	}

        StringSecurityDescriptor = lptoken;
    }

    bret = TRUE;

lend:
    return bret;
}

/******************************************************************************
 * ParseAclStringFlags
 */
static DWORD ParseAclStringFlags(LPCWSTR* StringAcl)
{
    DWORD flags = 0;
    LPCWSTR szAcl = *StringAcl;

    while (*szAcl != '(')
    {
        if (*szAcl == 'P')
	{
            flags |= SE_DACL_PROTECTED;
	}
        else if (*szAcl == 'A')
        {
            szAcl++;
            if (*szAcl == 'R')
                flags |= SE_DACL_AUTO_INHERIT_REQ;
	    else if (*szAcl == 'I')
                flags |= SE_DACL_AUTO_INHERITED;
        }
        szAcl++;
    }

    *StringAcl = szAcl;
    return flags;
}

/******************************************************************************
 * ParseStringSidToSid
 */
static BOOL ParseStringSidToSid(LPCWSTR StringSid, PSID pSid, LPDWORD cBytes)
{
    BOOL bret = FALSE;
    SID* pisid=pSid;

    TRACE("%s, %p, %p\n", debugstr_w(StringSid), pSid, cBytes);
    if (!StringSid)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        TRACE("StringSid is NULL, returning FALSE\n");
	return FALSE;
    }

    *cBytes = ComputeStringSidSize(StringSid);
    if (!pisid) /* Simply compute the size */
    {
        TRACE("only size requested, returning TRUE\n");
        return TRUE;
    }

    if (*StringSid != 'S' || *StringSid != '-') /* S-R-I-S-S */
    {
        DWORD i = 0, identAuth;
	DWORD csubauth = ((*cBytes - sizeof(SID)) / sizeof(DWORD)) + 1;

        StringSid += 2; /* Advance to Revision */
        pisid->Revision = atoiW(StringSid);

        if (pisid->Revision != SDDL_REVISION)
        {
            TRACE("Revision %d is unknown\n", pisid->Revision);
            goto lend; /* ERROR_INVALID_SID */
        }
        if (csubauth == 0)
        {
            TRACE("SubAuthorityCount is 0\n");
            goto lend; /* ERROR_INVALID_SID */
        }

	pisid->SubAuthorityCount = csubauth;

        /* Advance to identifier authority */
	while (*StringSid && *StringSid != '-')
            StringSid++;
        if (*StringSid == '-')
            StringSid++;

        /* MS' implementation can't handle values greater than 2^32 - 1, so
         * we don't either; assume most significant bytes are always 0
         */
        pisid->IdentifierAuthority.Value[0] = 0;
        pisid->IdentifierAuthority.Value[1] = 0;
        identAuth = atoiW(StringSid);
        pisid->IdentifierAuthority.Value[5] = identAuth & 0xff;
        pisid->IdentifierAuthority.Value[4] = (identAuth & 0xff00) >> 8;
        pisid->IdentifierAuthority.Value[3] = (identAuth & 0xff0000) >> 16;
        pisid->IdentifierAuthority.Value[2] = (identAuth & 0xff000000) >> 24;

        /* Advance to first sub authority */
        while (*StringSid && *StringSid != '-')
            StringSid++;
        if (*StringSid == '-')
            StringSid++;

        while (*StringSid)
	{	
	    while (*StringSid && *StringSid != '-')
                StringSid++;

            pisid->SubAuthority[i++] = atoiW(StringSid);
        }

	if (i != pisid->SubAuthorityCount)
            goto lend; /* ERROR_INVALID_SID */

        bret = TRUE;
    }
    else /* String constant format  - Only available in winxp and above */
    {
        pisid->Revision = SDDL_REVISION;
	pisid->SubAuthorityCount = 1;

	FIXME("String constant not supported: %s\n", debugstr_wn(StringSid, 2));

	/* TODO: Lookup string of well-known SIDs in table */
	pisid->IdentifierAuthority.Value[5] = 0;
	pisid->SubAuthority[0] = 0;

        bret = TRUE;
    }

lend:
    if (!bret)
        SetLastError(ERROR_INVALID_SID);

    TRACE("returning %s\n", bret ? "TRUE" : "FALSE");
    return bret;
}

/* Exported functions */

/*
 * @implemented
 */
BOOL STDCALL
AllocateLocallyUniqueId(PLUID Luid)
{
  NTSTATUS Status;

  Status = NtAllocateLocallyUniqueId (Luid);
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
AllocateAndInitializeSid (PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
			  BYTE nSubAuthorityCount,
			  DWORD dwSubAuthority0,
			  DWORD dwSubAuthority1,
			  DWORD dwSubAuthority2,
			  DWORD dwSubAuthority3,
			  DWORD dwSubAuthority4,
			  DWORD dwSubAuthority5,
			  DWORD dwSubAuthority6,
			  DWORD dwSubAuthority7,
			  PSID *pSid)
{
  NTSTATUS Status;

  Status = RtlAllocateAndInitializeSid (pIdentifierAuthority,
					nSubAuthorityCount,
					dwSubAuthority0,
					dwSubAuthority1,
	                                      dwSubAuthority2,
	                                      dwSubAuthority3,
	                                      dwSubAuthority4,
	                                      dwSubAuthority5,
	                                      dwSubAuthority6,
	                                      dwSubAuthority7,
	                                      pSid);
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
CopySid (DWORD nDestinationSidLength,
	 PSID pDestinationSid,
	 PSID pSourceSid)
{
  NTSTATUS Status;

  Status = RtlCopySid (nDestinationSidLength,
		       pDestinationSid,
		       pSourceSid);
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  return TRUE;
}
/* Winehq cvs 20050916 */
/******************************************************************************
 * ConvertStringSecurityDescriptorToSecurityDescriptorW [ADVAPI32.@]
 * @implemented
 */
BOOL WINAPI ConvertStringSecurityDescriptorToSecurityDescriptorW(
        LPCWSTR StringSecurityDescriptor,
        DWORD StringSDRevision,
        PSECURITY_DESCRIPTOR* SecurityDescriptor,
        PULONG SecurityDescriptorSize)
{
    DWORD cBytes;
    SECURITY_DESCRIPTOR* psd;
    BOOL bret = FALSE;

    TRACE("%s\n", debugstr_w(StringSecurityDescriptor));

    if (GetVersion() & 0x80000000)
    {
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        goto lend;
    }
    else if (StringSDRevision != SID_REVISION)
    {
        SetLastError(ERROR_UNKNOWN_REVISION);
	goto lend;
    }

    /* Compute security descriptor length */
    if (!ParseStringSecurityDescriptorToSecurityDescriptor(StringSecurityDescriptor,
        NULL, &cBytes))
	goto lend;

    psd = *SecurityDescriptor = (SECURITY_DESCRIPTOR*) LocalAlloc(
        GMEM_ZEROINIT, cBytes);

    psd->Revision = SID_REVISION;
    psd->Control |= SE_SELF_RELATIVE;

    if (!ParseStringSecurityDescriptorToSecurityDescriptor(StringSecurityDescriptor,
        psd, &cBytes))
    {
        LocalFree(psd);
	goto lend;
    }

    if (SecurityDescriptorSize)
        *SecurityDescriptorSize = cBytes;

    bret = TRUE;
 
lend:
    TRACE(" ret=%d\n", bret);
    return bret;
}

/* Winehq cvs 20050916 */
/******************************************************************************
 * ConvertStringSecurityDescriptorToSecurityDescriptorA [ADVAPI32.@]
 * @implemented
 */
BOOL WINAPI ConvertStringSecurityDescriptorToSecurityDescriptorA(
        LPCSTR StringSecurityDescriptor,
        DWORD StringSDRevision,
        PSECURITY_DESCRIPTOR* SecurityDescriptor,
        PULONG SecurityDescriptorSize)
{
    UINT len;
    BOOL ret = FALSE;
    LPWSTR StringSecurityDescriptorW;

    len = MultiByteToWideChar(CP_ACP, 0, StringSecurityDescriptor, -1, NULL, 0);
    StringSecurityDescriptorW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));

    if (StringSecurityDescriptorW)
    {
        MultiByteToWideChar(CP_ACP, 0, StringSecurityDescriptor, -1, StringSecurityDescriptorW, len);

        ret = ConvertStringSecurityDescriptorToSecurityDescriptorW(StringSecurityDescriptorW,
                                                                   StringSDRevision, SecurityDescriptor,
                                                                   SecurityDescriptorSize);
        HeapFree(GetProcessHeap(), 0, StringSecurityDescriptorW);
    }

    return ret;
}

/*
 * @implemented
 */
BOOL STDCALL
EqualPrefixSid (PSID pSid1,
		PSID pSid2)
{
  return RtlEqualPrefixSid (pSid1, pSid2);
}


/*
 * @implemented
 */
BOOL STDCALL
EqualSid (PSID pSid1,
	  PSID pSid2)
{
  return RtlEqualSid (pSid1, pSid2);
}


/*
 * @implemented
 *
 * RETURNS
 *  Docs says this function does NOT return a value
 *  even thou it's defined to return a PVOID...
 */
PVOID STDCALL
FreeSid (PSID pSid)
{
   return RtlFreeSid (pSid);
}


/*
 * @implemented
 */
DWORD STDCALL
GetLengthSid (PSID pSid)
{
  return (DWORD)RtlLengthSid (pSid);
}


/*
 * @implemented
 */
PSID_IDENTIFIER_AUTHORITY STDCALL
GetSidIdentifierAuthority (PSID pSid)
{
  return RtlIdentifierAuthoritySid (pSid);
}


/*
 * @implemented
 */
DWORD STDCALL
GetSidLengthRequired (UCHAR nSubAuthorityCount)
{
  return (DWORD)RtlLengthRequiredSid (nSubAuthorityCount);
}


/*
 * @implemented
 */
PDWORD STDCALL
GetSidSubAuthority (PSID pSid,
		    DWORD nSubAuthority)
{
  return (PDWORD)RtlSubAuthoritySid (pSid, nSubAuthority);
}


/*
 * @implemented
 */
PUCHAR STDCALL
GetSidSubAuthorityCount (PSID pSid)
{
  return RtlSubAuthorityCountSid (pSid);
}


/*
 * @implemented
 */
BOOL STDCALL
InitializeSid (PSID Sid,
	       PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
	       BYTE nSubAuthorityCount)
{
  NTSTATUS Status;

  Status = RtlInitializeSid (Sid,
			     pIdentifierAuthority,
			     nSubAuthorityCount);
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
IsValidSid (PSID pSid)
{
  return (BOOL)RtlValidSid (pSid);
}

/*
 * @implemented
 */
BOOL STDCALL
ConvertSidToStringSidW(PSID Sid, LPWSTR *StringSid)
{
  NTSTATUS Status;
  UNICODE_STRING UnicodeString;
  WCHAR FixedBuffer[64];

  if (! RtlValidSid(Sid))
    {
      SetLastError(ERROR_INVALID_SID);
      return FALSE;
    }

  UnicodeString.Length = 0;
  UnicodeString.MaximumLength = sizeof(FixedBuffer);
  UnicodeString.Buffer = FixedBuffer;
  Status = RtlConvertSidToUnicodeString(&UnicodeString, Sid, FALSE);
  if (STATUS_BUFFER_TOO_SMALL == Status)
    {
      Status = RtlConvertSidToUnicodeString(&UnicodeString, Sid, TRUE);
    }
  if (! NT_SUCCESS(Status))
    {
      SetLastError(RtlNtStatusToDosError(Status));
      return FALSE;
    }

  *StringSid = LocalAlloc(LMEM_FIXED, UnicodeString.Length + sizeof(WCHAR));
  if (NULL == *StringSid)
    {
      if (UnicodeString.Buffer != FixedBuffer)
        {
          RtlFreeUnicodeString(&UnicodeString);
        }
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
    }

  MoveMemory(*StringSid, UnicodeString.Buffer, UnicodeString.Length);
  ZeroMemory((PCHAR) *StringSid + UnicodeString.Length, sizeof(WCHAR));
  if (UnicodeString.Buffer != FixedBuffer)
    {
      RtlFreeUnicodeString(&UnicodeString);
    }

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
ConvertSidToStringSidA(PSID Sid, LPSTR *StringSid)
{
  LPWSTR StringSidW;
  int Len;

  if (! ConvertSidToStringSidW(Sid, &StringSidW))
    {
      return FALSE;
    }

  Len = WideCharToMultiByte(CP_ACP, 0, StringSidW, -1, NULL, 0, NULL, NULL);
  if (Len <= 0)
    {
      LocalFree(StringSidW);
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
    }
  *StringSid = LocalAlloc(LMEM_FIXED, Len);
  if (NULL == *StringSid)
    {
      LocalFree(StringSidW);
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
    }

  if (! WideCharToMultiByte(CP_ACP, 0, StringSidW, -1, *StringSid, Len, NULL, NULL))
    {
      LocalFree(StringSid);
      LocalFree(StringSidW);
      return FALSE;
    }

  LocalFree(StringSidW);

  return TRUE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
EqualDomainSid(IN PSID pSid1,
               IN PSID pSid2,
               OUT BOOL* pfEqual)
{
    FIXME("%s() not implemented!\n", __FUNCTION__);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
GetWindowsAccountDomainSid(IN PSID pSid,
                           OUT PSID ppDomainSid,
                           IN OUT DWORD* cbSid)
{
    FIXME("%s() not implemented!\n", __FUNCTION__);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
CreateWellKnownSid(IN WELL_KNOWN_SID_TYPE WellKnownSidType,
                   IN PSID DomainSid  OPTIONAL,
                   OUT PSID pSid,
                   IN OUT DWORD* cbSid)
{
    FIXME("unimplemented!\n", __FUNCTION__);
    return FALSE;
}

/* EOF */
