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
 * SDDL Separators - character version
 */
#define SDDL_SEPERATORC                     TEXT(";")
#define SDDL_DELIMINATORC                   TEXT(":")
#define SDDL_ACE_BEGINC                     TEXT("(")
#define SDDL_ACE_ENDC                       TEXT(")")

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
#else /* UNICODE */
#define ConvertSidToStringSid ConvertSidToStringSidA
#endif /* UNICODE */

#ifdef __cplusplus
}
#endif

#endif  /* __SDDL_H__ */
