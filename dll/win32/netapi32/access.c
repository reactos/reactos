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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "lmcons.h"
#include "lmaccess.h"
#include "lmapibuf.h"
#include "lmerr.h"
#include "netapi32_misc.h"
#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(netapi32);

static const WCHAR sAdminUserName[] = {'A','d','m','i','n','i','s','t','r','a','t',
                                'o','r',0};
static const WCHAR sGuestUserName[] = {'G','u','e','s','t',0};

/************************************************************
 *                NETAPI_ValidateServername
 *
 * Validates server name
 */
static NET_API_STATUS NETAPI_ValidateServername(LPCWSTR ServerName)
{
    if (ServerName)
    {
        if (ServerName[0] == 0)
            return ERROR_BAD_NETPATH;
        else if (
            ((ServerName[0] == '\\') &&
             (ServerName[1] != '\\'))
            ||
            ((ServerName[0] == '\\') &&
             (ServerName[1] == '\\') &&
             (ServerName[2] == 0))
            )
            return ERROR_INVALID_NAME;
    }
    return NERR_Success;
}

/************************************************************
 *                NETAPI_IsKnownUser
 *
 * Checks whether the user name indicates current user.
 */
static BOOL NETAPI_IsKnownUser(LPCWSTR UserName)
{
    DWORD dwSize = UNLEN + 1;
    BOOL Result;
    LPWSTR buf;

    if (!lstrcmpW(UserName, sAdminUserName) ||
        !lstrcmpW(UserName, sGuestUserName))
        return TRUE;
    NetApiBufferAllocate(dwSize * sizeof(WCHAR), (LPVOID *) &buf);
    Result = GetUserNameW(buf, &dwSize);

    Result = Result && !lstrcmpW(UserName, buf);
    NetApiBufferFree(buf);

    return Result;
}

#define NETAPI_ForceKnownUser(UserName, FailureCode) \
    if (!NETAPI_IsKnownUser(UserName)) \
    { \
        FIXME("Can't find information for user %s\n", \
              debugstr_w(UserName)); \
        return FailureCode; \
    }

/************************************************************
 *                NetUserAdd (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetUserAdd(LPCWSTR servername,
                  DWORD level, LPBYTE bufptr, LPDWORD parm_err)
{
    NET_API_STATUS status;
    FIXME("(%s, %ld, %p, %p) stub!\n", debugstr_w(servername), level, bufptr, parm_err);

    status = NETAPI_ValidateServername(servername);
    if (status != NERR_Success)
        return status;
    
    if ((bufptr != NULL) && (level > 0) && (level <= 4))
    {
        PUSER_INFO_1 ui = (PUSER_INFO_1) bufptr;
        TRACE("usri%ld_name: %s\n", level, debugstr_w(ui->usri1_name));
        TRACE("usri%ld_password: %s\n", level, debugstr_w(ui->usri1_password));
        TRACE("usri%ld_comment: %s\n", level, debugstr_w(ui->usri1_comment));
    }
    return status;
}

/************************************************************
 *                NetUserDel  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetUserDel(LPCWSTR servername, LPCWSTR username)
{
    NET_API_STATUS status;
    FIXME("(%s, %s) stub!\n", debugstr_w(servername), debugstr_w(username));

    status = NETAPI_ValidateServername(servername);
    if (status != NERR_Success)
        return status;

    if (!NETAPI_IsKnownUser(username))
        return NERR_UserNotFound;

    /* Delete the user here */
    return status;
}

/************************************************************
 *                NetUserGetInfo  (NETAPI32.@)
 */
NET_API_STATUS WINAPI
NetUserGetInfo(LPCWSTR servername, LPCWSTR username, DWORD level,
               LPBYTE* bufptr)
{
    NET_API_STATUS status;
    TRACE("(%s, %s, %ld, %p)\n", debugstr_w(servername), debugstr_w(username),
          level, bufptr);
    status = NETAPI_ValidateServername(servername);
    if (status != NERR_Success)
        return status;
    NETAPI_ForceLocalComputer(servername, NERR_InvalidComputer);
    NETAPI_ForceKnownUser(username, NERR_UserNotFound);

    switch (level)
    {
    case 0:
    {
        PUSER_INFO_0 ui;
        int name_sz;

        name_sz = lstrlenW(username) + 1;

        /* set up buffer */
        NetApiBufferAllocate(sizeof(USER_INFO_0) + name_sz * sizeof(WCHAR),
                             (LPVOID *) bufptr);

        ui = (PUSER_INFO_0) *bufptr;
        ui->usri0_name = (LPWSTR) (*bufptr + sizeof(USER_INFO_0));

        /* get data */
        lstrcpyW(ui->usri0_name, username);
        break;
    }

    case 10:
    {
        PUSER_INFO_10 ui;
        PUSER_INFO_0 ui0;
        NET_API_STATUS status;
        /* sizes of the field buffers in WCHARS */
        int name_sz, comment_sz, usr_comment_sz, full_name_sz;

        comment_sz = 1;
        usr_comment_sz = 1;
        full_name_sz = 1;

        /* get data */
        status = NetUserGetInfo(servername, username, 0, (LPBYTE *) &ui0);
        if (status != NERR_Success)
        {
            NetApiBufferFree(ui0);
            return status;
        }
        name_sz = lstrlenW(ui0->usri0_name) + 1;

        /* set up buffer */
        NetApiBufferAllocate(sizeof(USER_INFO_10) +
                             (name_sz + comment_sz + usr_comment_sz +
                              full_name_sz) * sizeof(WCHAR),
                             (LPVOID *) bufptr);
        ui = (PUSER_INFO_10) *bufptr;
        ui->usri10_name = (LPWSTR) (*bufptr + sizeof(USER_INFO_10));
        ui->usri10_comment = (LPWSTR) (
            ((PBYTE) ui->usri10_name) + name_sz * sizeof(WCHAR));
        ui->usri10_usr_comment = (LPWSTR) (
            ((PBYTE) ui->usri10_comment) + comment_sz * sizeof(WCHAR));
        ui->usri10_full_name = (LPWSTR) (
            ((PBYTE) ui->usri10_usr_comment) + usr_comment_sz * sizeof(WCHAR));

        /* set data */
        lstrcpyW(ui->usri10_name, ui0->usri0_name);
        NetApiBufferFree(ui0);
        ui->usri10_comment[0] = 0;
        ui->usri10_usr_comment[0] = 0;
        ui->usri10_full_name[0] = 0;
        break;
    }

    case 1:
      {
        static const WCHAR homedirW[] = {'H','O','M','E',0};
        PUSER_INFO_1 ui;
        PUSER_INFO_0 ui0;
        NET_API_STATUS status;
        /* sizes of the field buffers in WCHARS */
        int name_sz, password_sz, home_dir_sz, comment_sz, script_path_sz;

        password_sz = 1; /* not filled out for security reasons for NetUserGetInfo*/
        comment_sz = 1;
        script_path_sz = 1;

       /* get data */
        status = NetUserGetInfo(servername, username, 0, (LPBYTE *) &ui0);
        if (status != NERR_Success)
        {
            NetApiBufferFree(ui0);
            return status;
        }
        name_sz = lstrlenW(ui0->usri0_name) + 1;
        home_dir_sz = GetEnvironmentVariableW(homedirW, NULL,0);
        /* set up buffer */
        NetApiBufferAllocate(sizeof(USER_INFO_1) +
                             (name_sz + password_sz + home_dir_sz +
                              comment_sz + script_path_sz) * sizeof(WCHAR),
                             (LPVOID *) bufptr);

        ui = (PUSER_INFO_1) *bufptr;
        ui->usri1_name = (LPWSTR) (ui + 1);
        ui->usri1_password = ui->usri1_name + name_sz;
        ui->usri1_home_dir = ui->usri1_password + password_sz;
        ui->usri1_comment = ui->usri1_home_dir + home_dir_sz;
        ui->usri1_script_path = ui->usri1_comment + comment_sz;
        /* set data */
        lstrcpyW(ui->usri1_name, ui0->usri0_name);
        NetApiBufferFree(ui0);
        ui->usri1_password[0] = 0;
        ui->usri1_password_age = 0;
        ui->usri1_priv = 0;
        GetEnvironmentVariableW(homedirW, ui->usri1_home_dir,home_dir_sz);
        ui->usri1_comment[0] = 0;
        ui->usri1_flags = 0;
        ui->usri1_script_path[0] = 0;
        break;
      }
    case 2:
    case 3:
    case 4:
    case 11:
    case 20:
    case 23:
    case 1003:
    case 1005:
    case 1006:
    case 1007:
    case 1008:
    case 1009:
    case 1010:
    case 1011:
    case 1012:
    case 1013:
    case 1014:
    case 1017:
    case 1018:
    case 1020:
    case 1023:
    case 1024:
    case 1025:
    case 1051:
    case 1052:
    case 1053:
    {
        FIXME("Level %ld is not implemented\n", level);
        break;
    }
    default:
        ERR("Invalid level %ld is specified\n", level);
        return ERROR_INVALID_LEVEL;
    }
    return NERR_Success;
}



/************************************************************
 *                NetUserEnum  (NETAPI32.@)
 */
NET_API_STATUS WINAPI
NetUserEnum(LPCWSTR servername, DWORD level, DWORD filter, LPBYTE* bufptr,
	    DWORD prefmaxlen, LPDWORD entriesread, LPDWORD totalentries,
	    LPDWORD resume_handle)
{
  FIXME("(%s,%ld, 0x%ld,%p,%ld,%p,%p,%p) stub!\n", debugstr_w(servername), level,
        filter, bufptr, prefmaxlen, entriesread, totalentries, resume_handle);

  return ERROR_ACCESS_DENIED;
}

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
    name_sz = lstrlenW(sAdminUserName);
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
    usr->usri1_user_id = 500;
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
    name_sz = lstrlenW(sGuestUserName);
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
    usr->usri1_user_id = 500;
    usr->usri1_next_index = 0;
}

/************************************************************
 *                NetQueryDisplayInformation  (NETAPI32.@)
 * Copies NET_DISPLAY_USER record.
 */
static void ACCESS_CopyDisplayUser(PNET_DISPLAY_USER dest, LPWSTR *dest_buf,
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
    TRACE("(%s, %ld, %ld, %ld, %ld, %p, %p)\n", debugstr_w(ServerName),
          Level, Index, EntriesRequested, PreferredMaximumLength,
          ReturnedEntryCount, SortedBuffer);
    NETAPI_ForceLocalComputer(ServerName, ERROR_ACCESS_DENIED);
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

        FIXME("Level %ld partially implemented\n", Level);
        *ReturnedEntryCount = records;
        comment_sz = 1;
        full_name_sz = 1;

        /* get data */
        dwSize = UNLEN + 1;
        NetApiBufferAllocate(dwSize, (LPVOID *) &name);
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
                             (LPVOID *) SortedBuffer);
        inf = (PNET_DISPLAY_USER) *SortedBuffer;
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
        FIXME("Level %ld is not implemented\n", Level);
        break;
    }

    default:
        ERR("Invalid level %ld is specified\n", Level);
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


/************************************************************
 *                NetUserModalsGet  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetUserModalsGet(LPCWSTR szServer, DWORD level, LPBYTE *pbuffer)
{
    FIXME("(%s %ld %p) stub!\n", debugstr_w(szServer), level, pbuffer);
    return NERR_InternalError;
}

/************************************************************
 *                NetLocalGroupAdd  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupAdd(LPCWSTR servername, DWORD level,
                 LPBYTE buf, LPDWORD parm_err)
{
    FIXME("(%s %ld %p %p) stub!\n", debugstr_w(servername), level, buf, parm_err);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupSetMember (NETAPI32.@)
 */

NET_API_STATUS WINAPI NetLocalGroupSetMembers(LPCWSTR servername,
             LPCWSTR groupname, DWORD level, LPBYTE buf, DWORD totalentries)
{
    FIXME("(%s %s %ld %p %ld) stub!\n", debugstr_w(servername), 
            debugstr_w(groupname),level, buf, totalentries);
    return NERR_Success;
}
