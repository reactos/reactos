/*
 * Copyright 2002 Andriy Palamarchuk
 *
 * netapi32 access functions
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

#include "netapi32.h"

WINE_DEFAULT_DEBUG_CHANNEL(netapi32);

/************************************************************
 *                ACCESS_QueryAdminDisplayInformation
 *
 *  Creates a buffer with information for the Admin User
 */
static void ACCESS_QueryAdminDisplayInformation(PNET_DISPLAY_USER *buf, PDWORD pdwSize)
{
    static const WCHAR sAdminUserName[] = {
        'A','d','m','i','n','i','s','t','r','a','t','o','r',0};

    /* sizes of the field buffers in WCHARS */
    int name_sz, comment_sz, full_name_sz;
    PNET_DISPLAY_USER usr;

    /* set up buffer */
    name_sz = lstrlenW(sAdminUserName) + 1;
    comment_sz = 1;
    full_name_sz = 1;
    
    *pdwSize = sizeof(NET_DISPLAY_USER);
    *pdwSize += (name_sz + comment_sz + full_name_sz) * sizeof(WCHAR);
    NetApiBufferAllocate(*pdwSize, (LPVOID *) buf);

    usr = *buf;
    usr->usri1_name = (LPWSTR) ((PBYTE) usr + sizeof(NET_DISPLAY_USER));
    usr->usri1_comment = (LPWSTR) (
        ((PBYTE) usr->usri1_name) + name_sz * sizeof(WCHAR));
    usr->usri1_full_name = (LPWSTR) (
        ((PBYTE) usr->usri1_comment) + comment_sz * sizeof(WCHAR));

    /* set data */
    lstrcpyW(usr->usri1_name, sAdminUserName);
    usr->usri1_comment[0] = 0;
    usr->usri1_flags = UF_SCRIPT | UF_NORMAL_ACCOUNT | UF_DONT_EXPIRE_PASSWD;
    usr->usri1_full_name[0] = 0;
    usr->usri1_user_id = DOMAIN_USER_RID_ADMIN;
    usr->usri1_next_index = 0;
}

/************************************************************
 *                ACCESS_QueryGuestDisplayInformation
 *
 *  Creates a buffer with information for the Guest User
 */
static void ACCESS_QueryGuestDisplayInformation(PNET_DISPLAY_USER *buf, PDWORD pdwSize)
{
    static const WCHAR sGuestUserName[] = {
        'G','u','e','s','t',0 };

    /* sizes of the field buffers in WCHARS */
    int name_sz, comment_sz, full_name_sz;
    PNET_DISPLAY_USER usr;

    /* set up buffer */
    name_sz = lstrlenW(sGuestUserName) + 1;
    comment_sz = 1;
    full_name_sz = 1;
    
    *pdwSize = sizeof(NET_DISPLAY_USER);
    *pdwSize += (name_sz + comment_sz + full_name_sz) * sizeof(WCHAR);
    NetApiBufferAllocate(*pdwSize, (LPVOID *) buf);

    usr = *buf;
    usr->usri1_name = (LPWSTR) ((PBYTE) usr + sizeof(NET_DISPLAY_USER));
    usr->usri1_comment = (LPWSTR) (
        ((PBYTE) usr->usri1_name) + name_sz * sizeof(WCHAR));
    usr->usri1_full_name = (LPWSTR) (
        ((PBYTE) usr->usri1_comment) + comment_sz * sizeof(WCHAR));

    /* set data */
    lstrcpyW(usr->usri1_name, sGuestUserName);
    usr->usri1_comment[0] = 0;
    usr->usri1_flags = UF_ACCOUNTDISABLE | UF_SCRIPT | UF_NORMAL_ACCOUNT |
        UF_DONT_EXPIRE_PASSWD;
    usr->usri1_full_name[0] = 0;
    usr->usri1_user_id = DOMAIN_USER_RID_GUEST;
    usr->usri1_next_index = 0;
}

/************************************************************
 * Copies NET_DISPLAY_USER record.
 */
static void ACCESS_CopyDisplayUser(const NET_DISPLAY_USER *dest, LPWSTR *dest_buf,
                            PNET_DISPLAY_USER src)
{
    LPWSTR str = *dest_buf;

    src->usri1_name = str;
    lstrcpyW(src->usri1_name, dest->usri1_name);
    str = (LPWSTR) (
        ((PBYTE) str) + (lstrlenW(str) + 1) * sizeof(WCHAR));

    src->usri1_comment = str;
    lstrcpyW(src->usri1_comment, dest->usri1_comment);
    str = (LPWSTR) (
        ((PBYTE) str) + (lstrlenW(str) + 1) * sizeof(WCHAR));

    src->usri1_flags = dest->usri1_flags;

    src->usri1_full_name = str;
    lstrcpyW(src->usri1_full_name, dest->usri1_full_name);
    str = (LPWSTR) (
        ((PBYTE) str) + (lstrlenW(str) + 1) * sizeof(WCHAR));

    src->usri1_user_id = dest->usri1_user_id;
    src->usri1_next_index = dest->usri1_next_index;
    *dest_buf = str;
}

/************************************************************
 *                NetQueryDisplayInformation  (NETAPI32.@)
 *
 * The buffer structure:
 * - array of fixed size record of the level type
 * - strings, referenced by the record of the level type
 */
NET_API_STATUS WINAPI
NetQueryDisplayInformation(
    LPCWSTR ServerName, DWORD Level, DWORD Index, DWORD EntriesRequested,
    DWORD PreferredMaximumLength, LPDWORD ReturnedEntryCount,
    PVOID *SortedBuffer)
{
    TRACE("(%s, %d, %d, %d, %d, %p, %p)\n", debugstr_w(ServerName),
          Level, Index, EntriesRequested, PreferredMaximumLength,
          ReturnedEntryCount, SortedBuffer);

    if(!NETAPI_IsLocalComputer(ServerName))
    {
        FIXME("Only implemented on local computer, but requested for "
              "remote server %s\n", debugstr_w(ServerName));
        return ERROR_ACCESS_DENIED;
    }

    switch (Level)
    {
    case 1:
    {
        /* current record */
        PNET_DISPLAY_USER inf;
        /* current available strings buffer */
        LPWSTR str;
        PNET_DISPLAY_USER admin, guest;
        DWORD admin_size, guest_size;
        LPWSTR name = NULL;
        DWORD dwSize;

        /* sizes of the field buffers in WCHARS */
        int name_sz, comment_sz, full_name_sz;

        /* number of the records, returned in SortedBuffer
           3 - for current user, Administrator and Guest users
         */
        int records = 3;

        FIXME("Level %d partially implemented\n", Level);
        *ReturnedEntryCount = records;
        comment_sz = 1;
        full_name_sz = 1;

        /* get data */
        dwSize = UNLEN + 1;
        NetApiBufferAllocate(dwSize * sizeof(WCHAR), (LPVOID *) &name);
        if (!GetUserNameW(name, &dwSize))
        {
            NetApiBufferFree(name);
            return ERROR_ACCESS_DENIED;
        }
        name_sz = dwSize;
        ACCESS_QueryAdminDisplayInformation(&admin, &admin_size);
        ACCESS_QueryGuestDisplayInformation(&guest, &guest_size);

        /* set up buffer */
        dwSize = sizeof(NET_DISPLAY_USER) * records;
        dwSize += (name_sz + comment_sz + full_name_sz) * sizeof(WCHAR);

        NetApiBufferAllocate(dwSize +
                             admin_size - sizeof(NET_DISPLAY_USER) +
                             guest_size - sizeof(NET_DISPLAY_USER),
                             SortedBuffer);
        inf = *SortedBuffer;
        str = (LPWSTR) ((PBYTE) inf + sizeof(NET_DISPLAY_USER) * records);
        inf->usri1_name = str;
        str = (LPWSTR) (
            ((PBYTE) str) + name_sz * sizeof(WCHAR));
        inf->usri1_comment = str;
        str = (LPWSTR) (
            ((PBYTE) str) + comment_sz * sizeof(WCHAR));
        inf->usri1_full_name = str;
        str = (LPWSTR) (
            ((PBYTE) str) + full_name_sz * sizeof(WCHAR));

        /* set data */
        lstrcpyW(inf->usri1_name, name);
        NetApiBufferFree(name);
        inf->usri1_comment[0] = 0;
        inf->usri1_flags =
            UF_SCRIPT | UF_NORMAL_ACCOUNT | UF_DONT_EXPIRE_PASSWD;
        inf->usri1_full_name[0] = 0;
        inf->usri1_user_id = 0;
        inf->usri1_next_index = 0;

        inf++;
        ACCESS_CopyDisplayUser(admin, &str, inf);
        NetApiBufferFree(admin);

        inf++;
        ACCESS_CopyDisplayUser(guest, &str, inf);
        NetApiBufferFree(guest);
        break;
    }

    case 2:
    case 3:
    {
        FIXME("Level %d is not implemented\n", Level);
        break;
    }

    default:
        TRACE("Invalid level %d is specified\n", Level);
        return ERROR_INVALID_LEVEL;
    }
    return NERR_Success;
}

/************************************************************
 *                NetGetDCName  (NETAPI32.@)
 *
 *  Return the name of the primary domain controller (PDC)
 */

NET_API_STATUS WINAPI
NetGetDCName(LPCWSTR servername, LPCWSTR domainname, LPBYTE *bufptr)
{
  FIXME("(%s, %s, %p) stub!\n", debugstr_w(servername),
                 debugstr_w(domainname), bufptr);
  return NERR_DCNotFound; /* say we can't find a domain controller */  
}
