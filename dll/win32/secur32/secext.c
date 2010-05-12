/*
 * Copyright (C) 2004 Juan Lang
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

#include <precomp.h>

#define NDEBUG
#include <debug.h>

#define UNLEN 256


/***********************************************************************
 *		GetComputerObjectNameA (SECUR32.@) Wine 1.1.14
 *
 * Get the local computer's name using the format specified.
 *
 * PARAMS
 *  NameFormat   [I] The format for the name.
 *  lpNameBuffer [O] Pointer to buffer to receive the name.
 *  nSize        [I/O] Size in characters of buffer.
 *
 * RETURNS
 *  TRUE  If the name was written to lpNameBuffer.
 *  FALSE If the name couldn't be written.
 *
 * NOTES
 *  If lpNameBuffer is NULL, then the size of the buffer needed to hold the
 *  name will be returned in *nSize.
 *
 *  nSize returns the number of characters written when lpNameBuffer is not
 *  NULL or the size of the buffer needed to hold the name when the buffer
 *  is too short or lpNameBuffer is NULL.
 * 
 *  It the buffer is too short, ERROR_INSUFFICIENT_BUFFER error will be set.
 */
BOOLEAN WINAPI GetComputerObjectNameA(
  EXTENDED_NAME_FORMAT NameFormat, LPSTR lpNameBuffer, PULONG nSize)
{
    BOOLEAN rc;
    LPWSTR bufferW = NULL;
    ULONG sizeW = *nSize;
    DPRINT("(%d %p %p)\n", NameFormat, lpNameBuffer, nSize);
    if (lpNameBuffer) {
        bufferW = HeapAlloc(GetProcessHeap(), 0, sizeW * sizeof(WCHAR));
        if (bufferW == NULL) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
    }
    rc = GetComputerObjectNameW(NameFormat, bufferW, &sizeW);
    if (rc && bufferW) {
        ULONG len = WideCharToMultiByte(CP_ACP, 0, bufferW, -1, NULL, 0, NULL, NULL);
        WideCharToMultiByte(CP_ACP, 0, bufferW, -1, lpNameBuffer, *nSize, NULL, NULL);
        *nSize = len;
    }
    else
        *nSize = sizeW;
    HeapFree(GetProcessHeap(), 0, bufferW);
    return rc;
}

/***********************************************************************
 *		GetComputerObjectNameW (SECUR32.@) Wine 1.1.14
 */
BOOLEAN WINAPI GetComputerObjectNameW(
  EXTENDED_NAME_FORMAT NameFormat, LPWSTR lpNameBuffer, PULONG nSize)
{
    LSA_HANDLE policyHandle;
    LSA_OBJECT_ATTRIBUTES objectAttributes;
    PPOLICY_DNS_DOMAIN_INFO domainInfo;
    NTSTATUS ntStatus;
    BOOLEAN status;
    DPRINT("(%d %p %p)\n", NameFormat, lpNameBuffer, nSize);

    if (NameFormat == NameUnknown)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ZeroMemory(&objectAttributes, sizeof(objectAttributes));
    objectAttributes.Length = sizeof(objectAttributes);

    ntStatus = LsaOpenPolicy(NULL, &objectAttributes,
                             POLICY_VIEW_LOCAL_INFORMATION,
                             &policyHandle);
    if (ntStatus != STATUS_SUCCESS)
    {
        SetLastError(LsaNtStatusToWinError(ntStatus));
        DPRINT1("LsaOpenPolicy failed with NT status %u\n", GetLastError());
        return FALSE;
    }

    ntStatus = LsaQueryInformationPolicy(policyHandle,
                                         PolicyDnsDomainInformation,
                                         (PVOID *)&domainInfo);
    if (ntStatus != STATUS_SUCCESS)
    {
        SetLastError(LsaNtStatusToWinError(ntStatus));
        DPRINT1("LsaQueryInformationPolicy failed with NT status %u\n",
             GetLastError());
        LsaClose(policyHandle);
        return FALSE;
    }

    if (domainInfo->Sid)
    {
        switch (NameFormat)
        {
        case NameSamCompatible:
            {
                WCHAR name[MAX_COMPUTERNAME_LENGTH + 1];
                DWORD size = sizeof(name)/sizeof(name[0]);
                if (GetComputerNameW(name, &size))
                {
                    DWORD len = domainInfo->Name.Length + size + 3;
                    if (lpNameBuffer)
                    {
                        if (*nSize < len)
                        {
                            *nSize = len;
                            SetLastError(ERROR_INSUFFICIENT_BUFFER);
                            status = FALSE;
                        }
                        else
                        {
                            WCHAR bs[] = { '\\', 0 };
                            WCHAR ds[] = { '$', 0 };
                            lstrcpyW(lpNameBuffer, domainInfo->Name.Buffer);
                            lstrcatW(lpNameBuffer, bs);
                            lstrcatW(lpNameBuffer, name);
                            lstrcatW(lpNameBuffer, ds);
                            status = TRUE;
                        }
                    }
                    else	/* just requesting length required */
                    {
                        *nSize = len;
                        status = TRUE;
                    }
                }
                else
                {
                    SetLastError(ERROR_INTERNAL_ERROR);
                    status = FALSE;
                }
            }
            break;
        case NameFullyQualifiedDN:
        case NameDisplay:
        case NameUniqueId:
        case NameCanonical:
        case NameUserPrincipal:
        case NameCanonicalEx:
        case NameServicePrincipal:
        case NameDnsDomain:
            DPRINT1("NameFormat %d not implemented\n", NameFormat);
            SetLastError(ERROR_CANT_ACCESS_DOMAIN_INFO);
            status = FALSE;
            break;
        default:
            SetLastError(ERROR_INVALID_PARAMETER);
            status = FALSE;
        }
    }
    else
    {
        SetLastError(ERROR_CANT_ACCESS_DOMAIN_INFO);
        status = FALSE;
    }

    LsaFreeMemory(domainInfo);
    LsaClose(policyHandle);

    return status;
}


BOOLEAN WINAPI GetUserNameExA(
  EXTENDED_NAME_FORMAT NameFormat, LPSTR lpNameBuffer, PULONG nSize)
{
    BOOLEAN rc;
    LPWSTR bufferW = NULL;
    ULONG sizeW = *nSize;
    DPRINT("(%d %p %p)\n", NameFormat, lpNameBuffer, nSize);
    if (lpNameBuffer) {
        bufferW = HeapAlloc(GetProcessHeap(), 0, sizeW * sizeof(WCHAR));
        if (bufferW == NULL) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
    }
    rc = GetUserNameExW(NameFormat, bufferW, &sizeW);
    if (rc) {
        ULONG len = WideCharToMultiByte(CP_ACP, 0, bufferW, -1, NULL, 0, NULL, NULL);
        if (len <= *nSize)
        {
            WideCharToMultiByte(CP_ACP, 0, bufferW, -1, lpNameBuffer, *nSize, NULL, NULL);
            *nSize = len - 1;
        }
        else
        {
            *nSize = len;
            rc = FALSE;
            SetLastError(ERROR_MORE_DATA);
        }
    }
    else
        *nSize = sizeW;
    HeapFree(GetProcessHeap(), 0, bufferW);
    return rc;
}


BOOLEAN WINAPI GetUserNameExW(
  EXTENDED_NAME_FORMAT NameFormat, LPWSTR lpNameBuffer, PULONG nSize)
{
    BOOLEAN status;
    WCHAR samname[UNLEN + 1 + MAX_COMPUTERNAME_LENGTH + 1];
    LPWSTR out;
    DWORD len;
    DPRINT("(%d %p %p)\n", NameFormat, lpNameBuffer, nSize);

    switch (NameFormat)
    {
    case NameSamCompatible:
        {
            /* This assumes the current user is always a local account */
            len = MAX_COMPUTERNAME_LENGTH + 1;
            if (GetComputerNameW(samname, &len))
            {
                out = samname + lstrlenW(samname);
                *out++ = '\\';
                len = UNLEN + 1;
                if (GetUserNameW(out, &len))
                {
                    status = (lstrlenW(samname) < *nSize);
                    if (status)
                    {
                        lstrcpyW(lpNameBuffer, samname);
                        *nSize = lstrlenW(samname);
                    }
                    else
                    {
                        SetLastError(ERROR_MORE_DATA);
                        *nSize = lstrlenW(samname) + 1;
                    }
                }
                else
                    status = FALSE;
            }
            else
                status = FALSE;
        }
        break;
    case NameUnknown:
    case NameFullyQualifiedDN:
    case NameDisplay:
    case NameUniqueId:
    case NameCanonical:
    case NameUserPrincipal:
    case NameCanonicalEx:
    case NameServicePrincipal:
    case NameDnsDomain:
        SetLastError(ERROR_NONE_MAPPED);
        status = FALSE;
        break;
    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        status = FALSE;
    }

    return status;
}
