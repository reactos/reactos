/*
 * Copyright (C) 2003 Ulrich Czekalla for CodeWeavers
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __SDDL_H__
#define __SDDL_H__


#ifdef __cplusplus
extern "C" {
#endif

/*
 * SDDL Version information
 */
#define SDDL_REVISION_1     1
#define SDDL_REVISION       SDDL_REVISION_1

/*
 * SDDL Component tags
 */
#define SDDL_OWNER                          TEXT("O")
#define SDDL_GROUP                          TEXT("G")
#define SDDL_DACL                           TEXT("D")
#define SDDL_SACL                           TEXT("S")

/*
 * SDDL Security descriptor controls
 */
#define SDDL_PROTECTED                      TEXT("P")
#define SDDL_AUTO_INHERIT_REQ               TEXT("AR")
#define SDDL_AUTO_INHERITED                 TEXT("AI")

/*
 * SDDL Rights
 */
#define SDDL_READ_PROPERTY                  TEXT("RP")
#define SDDL_WRITE_PROPERTY                 TEXT("WP")
#define SDDL_CREATE_CHILD                   TEXT("CC")
#define SDDL_DELETE_CHILD                   TEXT("DC")
#define SDDL_LIST_CHILDREN                  TEXT("LC")
#define SDDL_SELF_WRITE                     TEXT("SW")
#define SDDL_LIST_OBJECT                    TEXT("LO")
#define SDDL_DELETE_TREE                    TEXT("DT")
#define SDDL_CONTROL_ACCESS                 TEXT("CR")
#define SDDL_READ_CONTROL                   TEXT("RC")
#define SDDL_WRITE_DAC                      TEXT("WD")
#define SDDL_WRITE_OWNER                    TEXT("WO")
#define SDDL_STANDARD_DELETE                TEXT("SD")
#define SDDL_GENERIC_ALL                    TEXT("GA")
#define SDDL_GENERIC_READ                   TEXT("GR")
#define SDDL_GENERIC_WRITE                  TEXT("GW")
#define SDDL_GENERIC_EXECUTE                TEXT("GX")
#define SDDL_FILE_ALL                       TEXT("FA")
#define SDDL_FILE_READ                      TEXT("FR")
#define SDDL_FILE_WRITE                     TEXT("FW")
#define SDDL_FILE_EXECUTE                   TEXT("FX")
#define SDDL_KEY_ALL                        TEXT("KA")
#define SDDL_KEY_READ                       TEXT("KR")
#define SDDL_KEY_WRITE                      TEXT("KW")
#define SDDL_KEY_EXECUTE                    TEXT("KX")

#define SDDL_ALIAS_SIZE                     2

/*
 * SDDL User aliases
 */
#define SDDL_DOMAIN_ADMINISTRATORS          TEXT("DA")
#define SDDL_DOMAIN_GUESTS                  TEXT("DG")
#define SDDL_DOMAIN_USERS                   TEXT("DU")
#define SDDL_ENTERPRISE_DOMAIN_CONTROLLERS  TEXT("ED")
#define SDDL_DOMAIN_DOMAIN_CONTROLLERS      TEXT("DD")
#define SDDL_DOMAIN_COMPUTERS               TEXT("DC")
#define SDDL_BUILTIN_ADMINISTRATORS         TEXT("BA")
#define SDDL_BUILTIN_GUESTS                 TEXT("BG")
#define SDDL_BUILTIN_USERS                  TEXT("BU")
#define SDDL_LOCAL_ADMIN                    TEXT("LA")
#define SDDL_LOCAL_GUEST                    TEXT("LG")
#define SDDL_ACCOUNT_OPERATORS              TEXT("AO")
#define SDDL_BACKUP_OPERATORS               TEXT("BO")
#define SDDL_PRINTER_OPERATORS              TEXT("PO")
#define SDDL_SERVER_OPERATORS               TEXT("SO")
#define SDDL_AUTHENTICATED_USERS            TEXT("AU")
#define SDDL_PERSONAL_SELF                  TEXT("PS")
#define SDDL_CREATOR_OWNER                  TEXT("CO")
#define SDDL_CREATOR_GROUP                  TEXT("CG")
#define SDDL_LOCAL_SYSTEM                   TEXT("SY")
#define SDDL_POWER_USERS                    TEXT("PU")
#define SDDL_EVERYONE                       TEXT("WD")
#define SDDL_REPLICATOR                     TEXT("RE")
#define SDDL_INTERACTIVE                    TEXT("IU")
#define SDDL_NETWORK                        TEXT("NU")
#define SDDL_SERVICE                        TEXT("SU")
#define SDDL_RESTRICTED_CODE                TEXT("RC")
#define SDDL_ANONYMOUS                      TEXT("AN")
#define SDDL_SCHEMA_ADMINISTRATORS          TEXT("SA")
#define SDDL_CERT_SERV_ADMINISTRATORS       TEXT("CA")
#define SDDL_RAS_SERVERS                    TEXT("RS")
#define SDDL_ENTERPRISE_ADMINS              TEXT("EA")
#define SDDL_GROUP_POLICY_ADMINS            TEXT("PA")
#define SDDL_ALIAS_PREW2KCOMPACC            TEXT("RU")
#define SDDL_LOCAL_SERVICE                  TEXT("LS")
#define SDDL_NETWORK_SERVICE                TEXT("NS")
#define SDDL_REMOTE_DESKTOP                 TEXT("RD")
#define SDDL_NETWORK_CONFIGURATION_OPS      TEXT("NO")
#define SDDL_PERFMON_USERS                  TEXT("MU")
#define SDDL_PERFLOG_USERS                  TEXT("LU")

/*
 * SDDL Separators - character version
 */
#define SDDL_SEPERATORC                     TEXT(';')
#define SDDL_DELIMINATORC                   TEXT(':')
#define SDDL_ACE_BEGINC                     TEXT('(')
#define SDDL_ACE_ENDC                       TEXT(')')

/*
 * SDDL Separators - string version
 */
#define SDDL_SEPERATOR                     TEXT(";")
#define SDDL_DELIMINATOR                   TEXT(":")
#define SDDL_ACE_BEGIN                     TEXT("(")
#define SDDL_ACE_END                       TEXT(")")

BOOL WINAPI ConvertSidToStringSidA( PSID, LPSTR* );
BOOL WINAPI ConvertSidToStringSidW( PSID, LPWSTR* );
BOOL WINAPI ConvertStringSidToSidA( LPCSTR, PSID* );
BOOL WINAPI ConvertStringSidToSidW( LPCWSTR, PSID* );
BOOL WINAPI ConvertStringSecurityDescriptorToSecurityDescriptorA(
    LPCSTR, DWORD, PSECURITY_DESCRIPTOR*, PULONG );
BOOL WINAPI ConvertStringSecurityDescriptorToSecurityDescriptorW(
    LPCWSTR, DWORD, PSECURITY_DESCRIPTOR*, PULONG );
BOOL WINAPI ConvertSecurityDescriptorToStringSecurityDescriptorA(
    PSECURITY_DESCRIPTOR, DWORD, SECURITY_INFORMATION, LPSTR*, PULONG );
BOOL WINAPI ConvertSecurityDescriptorToStringSecurityDescriptorW(
    PSECURITY_DESCRIPTOR, DWORD, SECURITY_INFORMATION, LPWSTR*, PULONG );

#ifdef UNICODE
#define ConvertSidToStringSid ConvertSidToStringSidW
#define ConvertStringSidToSid ConvertStringSidToSidW
#define ConvertStringSecurityDescriptorToSecurityDescriptor \
    ConvertStringSecurityDescriptorToSecurityDescriptorW
#define ConvertSecurityDescriptorToStringSecurityDescriptor \
    ConvertSecurityDescriptorToStringSecurityDescriptorW
#else /* UNICODE */
#define ConvertSidToStringSid ConvertSidToStringSidA
#define ConvertStringSidToSid ConvertStringSidToSidA
#define ConvertStringSecurityDescriptorToSecurityDescriptor \
    ConvertStringSecurityDescriptorToSecurityDescriptorA
#define ConvertSecurityDescriptorToStringSecurityDescriptor \
    ConvertSecurityDescriptorToStringSecurityDescriptorA
#endif /* UNICODE */

#ifdef __cplusplus
}
#endif

#endif  /* __SDDL_H__ */
