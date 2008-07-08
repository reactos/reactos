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

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "lmcons.h"
#include "lmaccess.h"
#include "lmapibuf.h"
#include "lmerr.h"
#include "lmuse.h"
#include "ntsecapi.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(netapi32);

/* NOTE: So far, this is implemented to support tests that require user logins,
 *       but not designed to handle real user databases. Those should probably
 *       be synced with either the host's user database or with Samba.
 *
 * FIXME: The user database should hold all the information the USER_INFO_4 struct
 * needs, but for the first try, I will just implement the USER_INFO_1 fields.
 */

struct sam_user
{
    struct list entry;
    WCHAR user_name[LM20_UNLEN+1];
    WCHAR user_password[PWLEN + 1];
    DWORD sec_since_passwd_change;
    DWORD user_priv;
    LPWSTR home_dir;
    LPWSTR user_comment;
    DWORD user_flags;
    LPWSTR user_logon_script_path;
};

static struct list user_list = LIST_INIT( user_list );

BOOL NETAPI_IsLocalComputer(LPCWSTR ServerName);

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
 *                NETAPI_FindUser
 *
 * Looks for a user in the user database.
 * Returns a pointer to the entry in the user list when the user
 * is found, NULL otherwise.
 */
static struct sam_user* NETAPI_FindUser(LPCWSTR UserName)
{
    struct sam_user *user;

    LIST_FOR_EACH_ENTRY(user, &user_list, struct sam_user, entry)
    {
        if(lstrcmpW(user->user_name, UserName) == 0)
            return user;
    }
    return NULL;
}

static BOOL NETAPI_IsCurrentUser(LPCWSTR username)
{
    LPWSTR curr_user = NULL;
    DWORD dwSize;
    BOOL ret = FALSE;

    dwSize = LM20_UNLEN+1;
    curr_user = HeapAlloc(GetProcessHeap(), 0, dwSize);
    if(!curr_user)
    {
        ERR("Failed to allocate memory for user name.\n");
        goto end;
    }
    if(!GetUserNameW(curr_user, &dwSize))
    {
        ERR("Failed to get current user's user name.\n");
        goto end;
    }
    if (!lstrcmpW(curr_user, username))
    {
        ret = TRUE;
    }

end:
    HeapFree(GetProcessHeap(), 0, curr_user);
    return ret;
}

/************************************************************
 *                NetUserAdd (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetUserAdd(LPCWSTR servername,
                  DWORD level, LPBYTE bufptr, LPDWORD parm_err)
{
    NET_API_STATUS status;
    struct sam_user * su = NULL;

    FIXME("(%s, %d, %p, %p) stub!\n", debugstr_w(servername), level, bufptr, parm_err);

    if((status = NETAPI_ValidateServername(servername)) != NERR_Success)
        return status;

    switch(level)
    {
    /* Level 3 and 4 are identical for the purposes of NetUserAdd */
    case 4:
    case 3:
        FIXME("Level 3 and 4 not implemented.\n");
        /* Fall through */
    case 2:
        FIXME("Level 2 not implemented.\n");
        /* Fall through */
    case 1:
    {
        PUSER_INFO_1 ui = (PUSER_INFO_1) bufptr;
        su = HeapAlloc(GetProcessHeap(), 0, sizeof(struct sam_user));
        if(!su)
        {
            status = NERR_InternalError;
            break;
        }

        if(lstrlenW(ui->usri1_name) > LM20_UNLEN)
        {
            status = NERR_BadUsername;
            break;
        }

        /*FIXME: do other checks for a valid username */
        lstrcpyW(su->user_name, ui->usri1_name);

        if(lstrlenW(ui->usri1_password) > PWLEN)
        {
            /* Always return PasswordTooShort on invalid passwords. */
            status = NERR_PasswordTooShort;
            break;
        }
        lstrcpyW(su->user_password, ui->usri1_password);

        su->sec_since_passwd_change = ui->usri1_password_age;
        su->user_priv = ui->usri1_priv;
        su->user_flags = ui->usri1_flags;

        /*FIXME: set the other LPWSTRs to NULL for now */
        su->home_dir = NULL;
        su->user_comment = NULL;
        su->user_logon_script_path = NULL;

        list_add_head(&user_list, &su->entry);
        return NERR_Success;
    }
    default:
        TRACE("Invalid level %d specified.\n", level);
        status = ERROR_INVALID_LEVEL;
        break;
    }

    HeapFree(GetProcessHeap(), 0, su);

    return status;
}

/************************************************************
 *                NetUserDel  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetUserDel(LPCWSTR servername, LPCWSTR username)
{
    NET_API_STATUS status;
    struct sam_user *user;

    TRACE("(%s, %s)\n", debugstr_w(servername), debugstr_w(username));

    if((status = NETAPI_ValidateServername(servername))!= NERR_Success)
        return status;

    if ((user = NETAPI_FindUser(username)) == NULL)
        return NERR_UserNotFound;

    list_remove(&user->entry);

    HeapFree(GetProcessHeap(), 0, user->home_dir);
    HeapFree(GetProcessHeap(), 0, user->user_comment);
    HeapFree(GetProcessHeap(), 0, user->user_logon_script_path);
    HeapFree(GetProcessHeap(), 0, user);

    return NERR_Success;
}

/************************************************************
 *                NetUserGetInfo  (NETAPI32.@)
 */
NET_API_STATUS WINAPI
NetUserGetInfo(LPCWSTR servername, LPCWSTR username, DWORD level,
               LPBYTE* bufptr)
{
    NET_API_STATUS status;
    TRACE("(%s, %s, %d, %p)\n", debugstr_w(servername), debugstr_w(username),
          level, bufptr);
    status = NETAPI_ValidateServername(servername);
    if (status != NERR_Success)
        return status;

    if(!NETAPI_IsLocalComputer(servername))
    {
        FIXME("Only implemented for local computer, but remote server"
              "%s was requested.\n", debugstr_w(servername));
        return NERR_InvalidComputer;
    }

    if(!NETAPI_FindUser(username) && !NETAPI_IsCurrentUser(username))
    {
        TRACE("User %s is unknown.\n", debugstr_w(username));
        return NERR_UserNotFound;
    }

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
        FIXME("Level %d is not implemented\n", level);
        return NERR_InternalError;
    }
    default:
        TRACE("Invalid level %d is specified\n", level);
        return ERROR_INVALID_LEVEL;
    }
    return NERR_Success;
}

/************************************************************
 *                NetUserGetLocalGroups  (NETAPI32.@)
 */
NET_API_STATUS WINAPI
NetUserGetLocalGroups(LPCWSTR servername, LPCWSTR username, DWORD level,
                      DWORD flags, LPBYTE* bufptr, DWORD prefmaxlen,
                      LPDWORD entriesread, LPDWORD totalentries)
{
    NET_API_STATUS status;

    FIXME("(%s, %s, %d, %08x, %p %d, %p, %p) stub!\n",
          debugstr_w(servername), debugstr_w(username), level, flags, bufptr,
          prefmaxlen, entriesread, totalentries);

    status = NETAPI_ValidateServername(servername);
    if (status != NERR_Success)
        return status;

    if (!NETAPI_FindUser(username))
        return NERR_UserNotFound;

    if (bufptr) *bufptr = NULL;
    if (entriesread) *entriesread = 0;
    if (totalentries) *totalentries = 0;

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
  FIXME("(%s,%d, 0x%d,%p,%d,%p,%p,%p) stub!\n", debugstr_w(servername), level,
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
                             SortedBuffer);
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


/******************************************************************************
 * NetUserModalsGet  (NETAPI32.@)
 *
 * Retrieves global information for all users and global groups in the security
 * database.
 *
 * PARAMS
 *  szServer   [I] Specifies the DNS or the NetBIOS name of the remote server
 *                 on which the function is to execute.
 *  level      [I] Information level of the data.
 *     0   Return global passwords parameters. bufptr points to a
 *         USER_MODALS_INFO_0 struct.
 *     1   Return logon server and domain controller information. bufptr
 *         points to a USER_MODALS_INFO_1 struct.
 *     2   Return domain name and identifier. bufptr points to a 
 *         USER_MODALS_INFO_2 struct.
 *     3   Return lockout information. bufptr points to a USER_MODALS_INFO_3
 *         struct.
 *  pbuffer    [I] Buffer that receives the data.
 *
 * RETURNS
 *  Success: NERR_Success.
 *  Failure: 
 *     ERROR_ACCESS_DENIED - the user does not have access to the info.
 *     NERR_InvalidComputer - computer name is invalid.
 */
NET_API_STATUS WINAPI NetUserModalsGet(
    LPCWSTR szServer, DWORD level, LPBYTE *pbuffer)
{
    TRACE("(%s %d %p)\n", debugstr_w(szServer), level, pbuffer);
    
    switch (level)
    {
        case 0:
            /* return global passwords parameters */
            FIXME("level 0 not implemented!\n");
            *pbuffer = NULL;
            return NERR_InternalError;
        case 1:
            /* return logon server and domain controller info */
            FIXME("level 1 not implemented!\n");
            *pbuffer = NULL;
            return NERR_InternalError;
        case 2:
        {
            /* return domain name and identifier */
            PUSER_MODALS_INFO_2 umi;
            LSA_HANDLE policyHandle;
            LSA_OBJECT_ATTRIBUTES objectAttributes;
            PPOLICY_ACCOUNT_DOMAIN_INFO domainInfo;
            NTSTATUS ntStatus;
            PSID domainIdentifier = NULL;
            int domainNameLen;

            ZeroMemory(&objectAttributes, sizeof(objectAttributes));
            objectAttributes.Length = sizeof(objectAttributes);

            ntStatus = LsaOpenPolicy(NULL, &objectAttributes,
                                     POLICY_VIEW_LOCAL_INFORMATION,
                                     &policyHandle);
            if (ntStatus != STATUS_SUCCESS)
            {
                WARN("LsaOpenPolicy failed with NT status %x\n",
                     LsaNtStatusToWinError(ntStatus));
                return ntStatus;
            }

            ntStatus = LsaQueryInformationPolicy(policyHandle,
                                                 PolicyAccountDomainInformation,
                                                 (PVOID *)&domainInfo);
            if (ntStatus != STATUS_SUCCESS)
            {
                WARN("LsaQueryInformationPolicy failed with NT status %x\n",
                     LsaNtStatusToWinError(ntStatus));
                LsaClose(policyHandle);
                return ntStatus;
            }

            domainIdentifier = domainInfo->DomainSid;
            domainNameLen = lstrlenW(domainInfo->DomainName.Buffer) + 1;
            LsaClose(policyHandle);

            ntStatus = NetApiBufferAllocate(sizeof(USER_MODALS_INFO_2) +
                                            GetLengthSid(domainIdentifier) +
                                            domainNameLen * sizeof(WCHAR),
                                            (LPVOID *)pbuffer);

            if (ntStatus != NERR_Success)
            {
                WARN("NetApiBufferAllocate() failed\n");
                LsaFreeMemory(domainInfo);
                return ntStatus;
            }

            umi = (USER_MODALS_INFO_2 *) *pbuffer;
            umi->usrmod2_domain_id = (PSID)(*pbuffer +
                sizeof(USER_MODALS_INFO_2));
            umi->usrmod2_domain_name = (LPWSTR)(*pbuffer +
                sizeof(USER_MODALS_INFO_2) + GetLengthSid(domainIdentifier));

            lstrcpynW(umi->usrmod2_domain_name,
                      domainInfo->DomainName.Buffer,
                      domainNameLen);
            CopySid(GetLengthSid(domainIdentifier), umi->usrmod2_domain_id,
                    domainIdentifier);

            LsaFreeMemory(domainInfo);

            break;
        } 
        case 3:
            /* return lockout information */
            FIXME("level 3 not implemented!\n");
            *pbuffer = NULL;
            return NERR_InternalError;
        default:
            TRACE("Invalid level %d is specified\n", level);
            *pbuffer = NULL;
            return ERROR_INVALID_LEVEL;
    }

    return NERR_Success;
}

/******************************************************************************
 *                NetUserChangePassword  (NETAPI32.@)
 * PARAMS
 *  domainname  [I] Optional. Domain on which the user resides or the logon
 *                  domain of the current user if NULL.
 *  username    [I] Optional. Username to change the password for or the name
 *                  of the current user if NULL.
 *  oldpassword [I] The user's current password.
 *  newpassword [I] The password that the user will be changed to using.
 *
 * RETURNS
 *  Success: NERR_Success.
 *  Failure: NERR_* failure code or win error code.
 *
 */
NET_API_STATUS WINAPI NetUserChangePassword(LPCWSTR domainname, LPCWSTR username,
    LPCWSTR oldpassword, LPCWSTR newpassword)
{
    struct sam_user *user;

    TRACE("(%s, %s, ..., ...)\n", debugstr_w(domainname), debugstr_w(username));

    if(domainname)
        FIXME("Ignoring domainname %s.\n", debugstr_w(domainname));

    if((user = NETAPI_FindUser(username)) == NULL)
        return NERR_UserNotFound;

    if(lstrcmpW(user->user_password, oldpassword) != 0)
        return ERROR_INVALID_PASSWORD;

    if(lstrlenW(newpassword) > PWLEN)
        return ERROR_PASSWORD_RESTRICTION;

    lstrcpyW(user->user_password, newpassword);

    return NERR_Success;
}

NET_API_STATUS WINAPI NetUseAdd(LMSTR servername, DWORD level, LPBYTE bufptr, LPDWORD parm_err)
{
    FIXME("%s %d %p %p stub\n", debugstr_w(servername), level, bufptr, parm_err);
    return NERR_Success;
}
