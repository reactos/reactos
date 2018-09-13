/*******************************************************
    MultiUsr.cpp

    Code for handling multiple user functionality in IE
    and friends

    Initially by Christopher Evans (cevans) 4/28/98
********************************************************/

#define DONT_WANT_SHELLDEBUG
#include "private.h"
#include "resource.h"
#include "multiusr.h"
#include <assert.h>
#include "multiutl.h"
#include "strconst.h"
#include "Shlwapi.h"
#include "multiui.h"
#include <shlobj.h>
#include "mluisup.h"
#include <lmwksta.h>

TCHAR g_szRegRoot[MAX_PATH] = "";
extern HINSTANCE g_hInst;
static void _CreateIdentitiesFolder();


// add a backslash to a qualified path
//
// in:
//  lpszPath    path (A:, C:\foo, etc)
//
// out:
//  lpszPath    A:\, C:\foo\    ;
//
// returns:
//  pointer to the NULL that terminates the path

// this is here to avoid a dependancy on shlwapi.dll
#define CH_WHACK TEXT('\\')

STDAPI_(LPTSTR)
_PathAddBackslash(
    LPTSTR lpszPath)
{
    LPTSTR lpszEnd;

    // perf: avoid lstrlen call for guys who pass in ptr to end
    // of buffer (or rather, EOB - 1).
    // note that such callers need to check for overflow themselves.
    int ichPath = (*lpszPath && !*(lpszPath + 1)) ? 1 : lstrlen(lpszPath);

    // try to keep us from tromping over MAX_PATH in size.
    // if we find these cases, return NULL.  Note: We need to
    // check those places that call us to handle their GP fault
    // if they try to use the NULL!
    if (ichPath >= (MAX_PATH - 1))
    {
        Assert(FALSE);      // Let the caller know!
        return(NULL);
    }

    lpszEnd = lpszPath + ichPath;

    // this is really an error, caller shouldn't pass
    // an empty string
    if (!*lpszPath)
        return lpszEnd;

    /* Get the end of the source directory
    */
    switch(*CharPrev(lpszPath, lpszEnd)) {
    case CH_WHACK:
        break;

    default:
        *lpszEnd++ = CH_WHACK;
        *lpszEnd = TEXT('\0');
    }
    return lpszEnd;
}


STDAPI_(DWORD)
_SHGetValueA(
    IN  HKEY    hkey,
    IN  LPCSTR  pszSubKey,          OPTIONAL
    IN  LPCSTR  pszValue,           OPTIONAL
    OUT LPDWORD pdwType,            OPTIONAL
    OUT LPVOID  pvData,             OPTIONAL
    OUT LPDWORD pcbData)            OPTIONAL
{
    DWORD dwRet;
    HKEY hkeyNew;

    dwRet = RegOpenKeyExA(hkey, pszSubKey, 0, KEY_QUERY_VALUE, &hkeyNew);
    if (NO_ERROR == dwRet)
    {
        dwRet = RegQueryValueEx(hkeyNew, pszValue, NULL, pdwType, (LPBYTE)pvData, pcbData);
        RegCloseKey(hkeyNew);
    }
    else if (pcbData)
        *pcbData = 0;

    return dwRet;
}

/*----------------------------------------------------------
Purpose: Recursively delete the key, including all child values
         and keys.  Mimics what RegDeleteKey does in Win95.

Returns: 
Cond:    --
*/
DWORD
_DeleteKeyRecursively(
    IN HKEY   hkey, 
    IN LPCSTR pszSubKey)
{
    DWORD dwRet;
    HKEY hkSubKey;

    // Open the subkey so we can enumerate any children
    dwRet = RegOpenKeyExA(hkey, pszSubKey, 0, MAXIMUM_ALLOWED, &hkSubKey);
    if (ERROR_SUCCESS == dwRet)
    {
        DWORD   dwIndex;
        CHAR    szSubKeyName[MAX_PATH + 1];
        DWORD   cchSubKeyName = ARRAYSIZE(szSubKeyName);
        CHAR    szClass[MAX_PATH];
        DWORD   cbClass = ARRAYSIZE(szClass);

        // I can't just call RegEnumKey with an ever-increasing index, because
        // I'm deleting the subkeys as I go, which alters the indices of the
        // remaining subkeys in an implementation-dependent way.  In order to
        // be safe, I have to count backwards while deleting the subkeys.

        // Find out how many subkeys there are
        dwRet = RegQueryInfoKeyA(hkSubKey,
                                 szClass,
                                 &cbClass,
                                 NULL,
                                 &dwIndex, // The # of subkeys -- all we need
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL);

        if (NO_ERROR == dwRet)
        {
            // dwIndex is now the count of subkeys, but it needs to be
            // zero-based for RegEnumKey, so I'll pre-decrement, rather
            // than post-decrement.
            while (ERROR_SUCCESS == RegEnumKeyA(hkSubKey, --dwIndex, szSubKeyName, cchSubKeyName))
            {
                _DeleteKeyRecursively(hkSubKey, szSubKeyName);
            }
        }

        RegCloseKey(hkSubKey);

        dwRet = RegDeleteKeyA(hkey, pszSubKey);
    }

    return dwRet;
}

// ****************************************************************************************************
//  C   S   T   R   I   N   G   L   I   S   T       C   L   A   S   S
//
//  A really basic string list class.  Actually, its a string array class, but you don't need to know
//  that.  It could do so much more, but for now, it only maintains an array of C strings.
//


CStringList::CStringList()
{
    m_count = 0;
    m_ptrCount = 0;
    m_strings = NULL;
}

/*
    CStringList::~CStringList

    Clean up any memory that was allocated in the CStringList object
*/
CStringList::~CStringList()
{
    if (m_strings)
    {
        for (int i = 0; i < m_count; i++)
        {
            if (m_strings[i])
            {
                MemFree(m_strings[i]);
                m_strings[i] = NULL;
            }
        }
        MemFree(m_strings);
        m_strings = NULL;
        m_count = 0;
    }
}


/*
    CStringList::AddString

    Add a string to the end of the string list.
*/
void    CStringList::AddString(TCHAR* lpszInString)
{
    // make more room for pointers, if necessary
    if (m_ptrCount == m_count)
    {
        m_ptrCount += 5;
        if (!MemRealloc((void **)&m_strings, sizeof(TCHAR *) * m_ptrCount))
        {
            m_ptrCount -= 5;
            Assert(false);
            return;
        }

        // initialize the new strings to nil
        for (int i = m_count; i < m_ptrCount; i++)
            m_strings[i] = NULL;

    }
    
    //now put the string in the next location
    int iNewIndex = m_count++;

    if(MemAlloc((void **)&m_strings[iNewIndex], sizeof(TCHAR) * lstrlen(lpszInString)+1))
    {
        lstrcpy(m_strings[iNewIndex], lpszInString);
    }
    else
    {
        // couldn't allocate space for the string.  Don't count that spot as filled
        m_count--;
    }
}

/*
    CStringList::RemoveString
    
    Remove a string at zero based index iIndex 
*/

void    CStringList::RemoveString(int   iIndex)
{
    int     iCopySize;

    iCopySize = ((m_count - iIndex) - 1) * 4;

    // free the memory for the string
    if (m_strings[iIndex])
    {
        MemFree(m_strings[iIndex]);
        m_strings[iIndex] = NULL;
    }

    // move the other strings down
    if (iCopySize)
    {
        memmove(&(m_strings[iIndex]), &(m_strings[iIndex+1]), iCopySize);
    }

    // null out the last item in the list and decrement the counter.
    m_strings[--m_count] = NULL;
}

/*
    CStringList::GetString
    
    Return the pointer to the string at zero based index iIndex.

    Return the string at the given index.  Note that the TCHAR pointer
    is still owned by the string list and should not be deleted.
*/

TCHAR    *CStringList::GetString(int iIndex)
{
    if (iIndex < m_count && iIndex >= 0)
        return m_strings[iIndex];
    else
        return NULL;
}


int __cdecl _CSL_Compare(const void *p1, const void *p2)
{
    TCHAR *psz1, *psz2;

    psz1 = *((TCHAR **)p1);
    psz2 = *((TCHAR **)p2);

    return lstrcmpi(psz1, psz2);
}


/*
    CStringList::Sort
    
    Sort the strings in the list
*/

void    CStringList::Sort()
{
    qsort(m_strings, m_count, sizeof(TCHAR *), _CSL_Compare);
}


/*
    MU_Init

    Initialize the memory allocator and make sure that there is
    at least one user in the registry.
*/
static BOOL g_inited = FALSE;
EXTERN_C void    MU_Init()
{
    CStringList* pList;

    if (!g_inited)
    {
        MemInit();

        pList = MU_GetUsernameList();

        if (!pList || pList->GetLength() == 0)
        {
            _MakeDefaultFirstUser();
        }
        if (pList)
            delete pList;
        g_inited = TRUE;
    }
}


/*
    MU_GetUsernameList
    
    Build a CStringList with all of the names of the users 
    stored in HKLM
*/
#define MAXKEYNAME          256

CStringList*    MU_GetUsernameList(void)
{
    CStringList*    vList = NULL;
    HKEY    hSourceSubKey;
    DWORD   dwEnumIndex = 0, dwStatus, dwSize, dwType;
    int     cb;
    TCHAR    szKeyNameBuffer[MAXKEYNAME];

    vList = new CStringList;
    Assert(vList);
    
    if (vList)
    {
        if (RegCreateKey(HKEY_CURRENT_USER, c_szRegRoot, &hSourceSubKey) == ERROR_SUCCESS)
        {
            while (TRUE) 
            {
                HKEY    hkUserKey;

                if (RegEnumKey(hSourceSubKey, dwEnumIndex++, szKeyNameBuffer,MAXKEYNAME)
                    !=  ERROR_SUCCESS)
                    break;

                cb = lstrlen(szKeyNameBuffer);
                
                if (RegOpenKey(hSourceSubKey, szKeyNameBuffer, &hkUserKey) == ERROR_SUCCESS)
                {
                    dwSize = sizeof(szKeyNameBuffer);
                    dwStatus = RegQueryValueEx(hkUserKey, c_szUsername, NULL, &dwType, (LPBYTE)&szKeyNameBuffer, &dwSize);
                    
                    Assert(ERROR_SUCCESS == dwStatus);
                    Assert(*szKeyNameBuffer != 0);
                    //filter names that begin with _ to hide things like "_Outlook News"
                    if (ERROR_SUCCESS == dwStatus && *szKeyNameBuffer != '_')
                        vList->AddString(szKeyNameBuffer);
        
                    RegCloseKey(hkUserKey); 
                }
                else
                    AssertSz(FALSE, "Couldn't open user's Key");
            }
            RegCloseKey(hSourceSubKey);
        }
        else
            AssertSz(FALSE, "Couldn't open user profiles root Key");
    }

    return vList;
}


/*
    MU_UsernameToUserId

    Given a username, find its user id and return it.  Returns E_FAIL if it can't 
    find the given username.
*/

HRESULT   MU_UsernameToUserId(TCHAR *lpszUsername, GUID *puidID)
{
    HKEY    hSourceSubKey;
    ULONG   ulEnumIndex = 0;
    DWORD   dwStatus, dwSize, dwType;
    TCHAR    szKeyNameBuffer[MAXKEYNAME];
    BOOL    fFound = FALSE;
    TCHAR    szUid[255];

    ZeroMemory(puidID, sizeof(GUID));

    if (RegCreateKey(HKEY_CURRENT_USER, c_szRegRoot, &hSourceSubKey) == ERROR_SUCCESS)
    {
        while (!fFound) 
        {
            HKEY    hkUserKey;

            if (RegEnumKey(hSourceSubKey, ulEnumIndex++, szKeyNameBuffer,MAXKEYNAME)
                !=  ERROR_SUCCESS)
                break;
            
            if (RegOpenKey(hSourceSubKey, szKeyNameBuffer, &hkUserKey) == ERROR_SUCCESS)
            {
                dwSize = sizeof(szKeyNameBuffer);
                dwStatus = RegQueryValueEx(hkUserKey, c_szUsername, NULL, &dwType, (LPBYTE)&szKeyNameBuffer, &dwSize);
                
                if (ERROR_SUCCESS == dwStatus && lstrcmpi(lpszUsername, szKeyNameBuffer) == 0)
                {
                    dwSize = sizeof(szUid);
                    dwStatus = RegQueryValueEx(hkUserKey, c_szUserID, NULL, &dwType, (LPBYTE)&szUid, &dwSize);
                    fFound = (dwStatus == ERROR_SUCCESS);

                    if (fFound)
                        fFound = SUCCEEDED(GUIDFromAString(szUid, puidID));
                }
                RegCloseKey(hkUserKey); 
            }
        }
        RegCloseKey(hSourceSubKey);
    }
    

    return (fFound ? S_OK : E_FAIL);
}

/*
    MU_GetPasswordForUsername

    Get the password for the provided user and return it in szOutPassword.  Return in 
    pfUsePassword if password is enabled and false if it is disabled.

    Function returns true if the password data could be found, false otherwise
*/

BOOL  MU_GetPasswordForUsername(TCHAR *lpszInUsername, TCHAR *szOutPassword, BOOL *pfUsePassword)
{
#ifdef IDENTITY_PASSWORDS
    TCHAR           szPath[MAX_PATH];
    TCHAR           szPassword[255] = "";
    HKEY            hDestinationSubKey;
    DWORD           dwSize, dwStatus, dwType;
    DWORD           dwPWEnabled = 0;
    GUID            uidUserID;
    HRESULT         hr;
    PASSWORD_STORE  pwStore;

    hr = MU_UsernameToUserId(lpszInUsername, &uidUserID);
    Assert(SUCCEEDED(hr));
    
    if (uidUserID == GUID_NULL)
    {
        *pfUsePassword = FALSE;
        return TRUE;
    }
    
    if (SUCCEEDED(hr = ReadIdentityPassword(&uidUserID, &pwStore)))
    {
        lstrcpy(szOutPassword, pwStore.szPassword);
        *pfUsePassword = pwStore.fUsePassword;
        return TRUE;
    }
    else
    {
        BOOL fFoundPassword = FALSE;
        
        //build the user level key. 
        MU_GetRegRootForUserID(&uidUserID, szPath);
    
        if (RegCreateKey(HKEY_CURRENT_USER, szPath, &hDestinationSubKey) == ERROR_SUCCESS)
        {
            dwSize = sizeof(dwPWEnabled);
            dwStatus = RegQueryValueEx(hDestinationSubKey, c_szUsePassword, NULL, &dwType, (LPBYTE)&dwPWEnabled, &dwSize);
        
            if (ERROR_SUCCESS == dwStatus && 0 != dwPWEnabled)
            {
                dwSize = sizeof(szPassword);
                dwStatus = RegQueryValueEx(hDestinationSubKey, c_szPassword, NULL, &dwType, (LPBYTE)&szPassword, &dwSize);
        
                if (ERROR_SUCCESS == dwStatus)
                {
                    ULONG   cbSize;

                    fFoundPassword = TRUE;
                    cbSize = dwSize;
                    if (cbSize > 1)
                    {
                        DecodeUserPassword(szPassword, &cbSize);
                        strcpy(szOutPassword, szPassword);  
                    }
                    else
                    {
                        *szOutPassword = 0;
                    }
                }
            }
        
            RegCloseKey(hDestinationSubKey);
        }

        // Herein lies the suck.  We can't count on being able to access any
        // given pstore from any given profile on Win9x.  If you log on with
        // a blank password, or hit escape (not much difference to a user)
        // you will have a different pstore.  If we store our passwords in the
        // registry, they can be whacked pretty simply.  If we can't find the 
        // password, we will disable it for now and say there is none.  It 
        // seems that most people don't put passwords on identities now 
        // anyway, though this will change. 
        if (!fFoundPassword)
        {
            fFoundPassword = TRUE;
            dwPWEnabled = 0;
        }
        // Here ends the suck
        
        *pfUsePassword = (dwPWEnabled != 0);
        return fFoundPassword;
    }
#else
    *pfUsePassword = FALSE;
    return TRUE;
#endif //IDENTITY_PASSWORDS

}

/*
    _FillListBoxWithUsernames

    Fill a listbox with the names of the users,  Adds (Default) 
    to the default user.
*/
BOOL _FillListBoxWithUsernames(HWND hListbox)
{
    CStringList *lpCStringList;
    GUID        uidDefault;
    GUID        uidUser;

    lpCStringList = MU_GetUsernameList();
    
    MU_GetDefaultUserID(&uidDefault);

    SendMessage(hListbox, LB_RESETCONTENT, 0, 0);
    lpCStringList->Sort();

    if (lpCStringList)
    {
        for(int i = 0; i < lpCStringList->GetLength(); i++)
        {
            if (lpCStringList->GetString(i))
            {
                SendMessage(hListbox, LB_ADDSTRING, 0, (LPARAM)lpCStringList->GetString(i));
            }
        }
        delete lpCStringList;
        return true;
    }
    return false;
}

BOOL _FillComboBoxWithUsernames(HWND hCombobox, HWND hListbox)
{
    TCHAR szRes[128];
    DWORD_PTR cIndex, dwCount = SendMessage(hListbox, LB_GETCOUNT, 0, 0);

    SendMessage(hCombobox, CB_RESETCONTENT, 0, 0);

    for (cIndex = 0; cIndex < dwCount; cIndex++)
    {
        SendMessage(hListbox, LB_GETTEXT, cIndex, (LPARAM)szRes);
        SendMessage(hCombobox, CB_ADDSTRING, 0, (LPARAM)szRes);
    }
    return true;
}

/*
    MU_UsernameExists
    
    Does the given name already exist as a username?
*/

BOOL        MU_UsernameExists(TCHAR*    lpszUsername)
{
    GUID uidID;
    
    return SUCCEEDED(MU_UsernameToUserId(lpszUsername, &uidID));

}

/*
    MU_GetUserInfo
    
    Fill in the user info structure with current values
*/

BOOL    MU_GetUserInfo(GUID *puidUserID, LPUSERINFO lpUserInfo)
{
    TCHAR           szPWBuffer[255];
    TCHAR           szRegPath[MAX_PATH];
    HKEY            hKey;
    BOOL            bResult = false;
    LONG            lValue;
    DWORD           dwStatus, dwType, dwSize;
    GUID            uidUser;
    TCHAR           szUid[255];
    HRESULT         hr;
    PASSWORD_STORE  pwStore;

    lpUserInfo->fPasswordValid = FALSE;
    
    if( puidUserID == NULL)
    {
        MU_GetCurrentUserID(&uidUser);
        if (uidUser == GUID_NULL)
            return FALSE;
    }
    else
        uidUser = *puidUserID;

    MU_GetRegRootForUserID(&uidUser, szRegPath);
    
    if (RegOpenKey(HKEY_CURRENT_USER, szRegPath, &hKey) == ERROR_SUCCESS)
    {
        *lpUserInfo->szPassword = 0;
        lpUserInfo->fUsePassword = false;
        ZeroMemory(&lpUserInfo->uidUserID, sizeof(GUID));

        dwSize = sizeof(lpUserInfo->szUsername);
        if ((dwStatus = RegQueryValueEx(hKey, c_szUsername, NULL, &dwType, (LPBYTE)lpUserInfo->szUsername, &dwSize)) == ERROR_SUCCESS &&
                (0 != *lpUserInfo->szUsername))
        {
            //we have the username, that is the only required part.  The others are optional.
            bResult = true;
            
#ifdef IDENTITY_PASSWORDS
            lpUserInfo->fPasswordValid = FALSE;
            if (SUCCEEDED(hr = ReadIdentityPassword(&uidUser, &pwStore)))
            {
                lstrcpy(lpUserInfo->szPassword, pwStore.szPassword);
                lpUserInfo->fUsePassword = pwStore.fUsePassword;
                lpUserInfo->fPasswordValid = TRUE;
            }
            else
            {
                dwSize = sizeof(lValue);
                if ((dwStatus = RegQueryValueEx(hKey, c_szUsePassword, NULL, &dwType, (LPBYTE)&lValue, &dwSize)) == ERROR_SUCCESS)
                {
                    lpUserInfo->fUsePassword = (lValue != 0);
                }

                dwSize = sizeof(szPWBuffer);
                dwStatus = RegQueryValueEx(hKey, c_szPassword, NULL, &dwType, (LPBYTE)szPWBuffer, &dwSize);

                ULONG   cbSize;

                lpUserInfo->fPasswordValid = (ERROR_SUCCESS == dwStatus);

                // Herein lies the suck (Volume 2).  We can't count on being able to access any
                // given pstore from any given profile on Win9x.  If you log on with
                // a blank password, or hit escape (not much difference to a user)
                // you will have a different pstore.  If we store our passwords in the
                // registry, they can be whacked pretty simply.  If we can't find the 
                // password, we will disable it for now and say there is none.  It 
                // seems that most people don't put passwords on identities now 
                // anyway, though this will change.  
                if (!lpUserInfo->fPasswordValid)
                {
                    lpUserInfo->fPasswordValid = TRUE;
                    lpUserInfo->fUsePassword = FALSE;
                }
                // Here ends the suck

                cbSize = dwSize;
                if (ERROR_SUCCESS == dwStatus && cbSize > 1)
                {
                    DecodeUserPassword(szPWBuffer, &cbSize);
                    strcpy(lpUserInfo->szPassword, szPWBuffer);
                }
                else
                    *lpUserInfo->szPassword = 0;
            }
#endif 
            dwSize = sizeof(szUid);
            if ((dwStatus = RegQueryValueEx(hKey, c_szUserID, NULL, &dwType, (LPBYTE)&szUid, &dwSize)) == ERROR_SUCCESS)
            {
                hr = GUIDFromAString(szUid, &lpUserInfo->uidUserID);
                Assert(hr);
            }

        }
        RegCloseKey(hKey);
    }
        
    return bResult;
}


/*
    MU_SetUserInfo
    
    Save the user info structure with the user values
*/
BOOL        MU_SetUserInfo(LPUSERINFO lpUserInfo)
{
    DWORD           dwType, dwSize, dwValue, dwStatus;
    HKEY            hkCurrUser;
    TCHAR           szPath[MAX_PATH];
    WCHAR           szwPath[MAX_PATH];
    TCHAR           szUid[255];
    BOOL            fNewIdentity = FALSE;
    PASSWORD_STORE  pwStore;
    HRESULT         hr;

    MU_GetRegRootForUserID(&lpUserInfo->uidUserID, szPath);
    
    Assert(pszRegPath && *pszRegPath);
    Assert(lpUserInfo->uidUserID != GUID_NULL);
    
    if ((dwStatus = RegCreateKey(HKEY_CURRENT_USER, szPath, &hkCurrUser)) == ERROR_SUCCESS)
    {
        ULONG   cbSize;
        TCHAR   szBuffer[255];

        // write out the correct values
        dwType = REG_SZ;
        dwSize = lstrlen(lpUserInfo->szUsername) + 1;
        RegSetValueEx(hkCurrUser, c_szUsername, 0, dwType, (LPBYTE)lpUserInfo->szUsername, dwSize);

        dwSize = sizeof(DWORD);
        if ((dwStatus = RegQueryValueEx(hkCurrUser, c_szDirName, NULL, &dwType, (LPBYTE)&dwValue, &dwSize)) != ERROR_SUCCESS)
        {
            dwValue = MU_GenerateDirectoryNameForIdentity(&lpUserInfo->uidUserID);
        
            dwType = REG_DWORD;
            dwSize = sizeof(dwValue);
            RegSetValueEx(hkCurrUser, c_szDirName, 0, dwType, (LPBYTE)&dwValue, dwSize);
            fNewIdentity = TRUE;
        }

#ifdef IDENTITY_PASSWORDS
        lstrcpy(pwStore.szPassword, lpUserInfo->szPassword);
        pwStore.fUsePassword = lpUserInfo->fUsePassword;

        if (FAILED(hr = WriteIdentityPassword(&lpUserInfo->uidUserID, &pwStore)))
        {
            dwType = REG_BINARY ;
            cbSize = strlen(lpUserInfo->szPassword) + 1;
            lstrcpy(szBuffer, lpUserInfo->szPassword);
            EncodeUserPassword(szBuffer, &cbSize);
            dwSize = cbSize;
            RegSetValueEx(hkCurrUser, c_szPassword, 0, dwType, (LPBYTE)szBuffer, dwSize);
        
            dwType = REG_DWORD;
            dwValue = (lpUserInfo->fUsePassword ? 1 : 0);
            dwSize = sizeof(dwValue);
            RegSetValueEx(hkCurrUser, c_szUsePassword, 0, dwType, (LPBYTE)&dwValue, dwSize);
        }
        else
        {
            //don't keep the registry values if we could save it to the pstore.
            RegDeleteValue(hkCurrUser, c_szPassword);
            RegDeleteValue(hkCurrUser, c_szUsePassword);
        }
#endif //IDENTITY_PASSWORDS

        Assert(lpUserInfo->uidUserID != GUID_NULL);
        AStringFromGUID(&lpUserInfo->uidUserID,  szUid, 255);

        dwType = REG_SZ;
        dwSize = lstrlen(szUid) + 1;
        RegSetValueEx(hkCurrUser, c_szUserID, 0, dwType, (LPBYTE)&szUid, dwSize);

        RegCloseKey(hkCurrUser);

        if (fNewIdentity)
        {
            if (SUCCEEDED(MU_GetUserDirectoryRoot(&lpUserInfo->uidUserID, GIF_ROAMING_FOLDER, szwPath, MAX_PATH)))
            {
                if (!CreateDirectoryWrapW(szwPath,NULL))
                {
                    _CreateIdentitiesFolder();
                    CreateDirectoryWrapW(szwPath,NULL);
                }
            }
            
            if (SUCCEEDED(MU_GetUserDirectoryRoot(&lpUserInfo->uidUserID, GIF_NON_ROAMING_FOLDER, szwPath, MAX_PATH)))
            {
                if (!CreateDirectoryWrapW(szwPath,NULL))
                {
                    _CreateIdentitiesFolder();
                    CreateDirectoryWrapW(szwPath,NULL);
                }
            }
        }
        return TRUE;
    }
    return FALSE;
}

/*
    MU_SwitchToUser

    Currently, this just saves the last user's info.
*/
HRESULT  MU_SwitchToUser(TCHAR *lpszUsername)
{
    GUID    uidUserID;
    TCHAR    szUid[255];
    HRESULT hr;

    Assert(lpszUsername);
    
    if (*lpszUsername == 0) //  null string means null guid
    {
        uidUserID = GUID_NULL;
    }
    else
    {
        hr = MU_UsernameToUserId(lpszUsername, &uidUserID);
        if (FAILED(hr))
            return hr;
    }


    AStringFromGUID(&uidUserID,  szUid, 255);
    Assert(uidUserID != GUID_NULL || (*lpszUsername == 0));

    wsprintf(g_szRegRoot, "%.100s\\%.40s", c_szRegRoot, szUid);
    
    // remember who we last switched to
    HKEY    hkey;
    if (RegCreateKey(HKEY_CURRENT_USER, c_szRegRoot, &hkey) == ERROR_SUCCESS)
    {
        DWORD   dwType, dwSize;

        dwType = REG_SZ;
        dwSize = lstrlen(lpszUsername) + 1;
        RegSetValueEx(hkey, c_szLastUserName, 0, dwType, (LPBYTE)lpszUsername, dwSize);

        dwType = REG_SZ;
        dwSize = lstrlen(szUid) + 1;
        RegSetValueEx(hkey, c_szLastUserID, 0, dwType, (LPBYTE)szUid, dwSize);

        RegCloseKey(hkey);
    }

    return S_OK;
}

/*
    MU_SwitchToLastUser

    Makes the last user current, if there is no
    last user, it switches to the first user it can
    find. If there are no users, it creates a 
    user called "Main User"
*/
void MU_SwitchToLastUser()
{
    HKEY    hkey;
    TCHAR   szUserUid[255];
    TCHAR   szUsername[CCH_USERNAME_MAX_LENGTH + 1];
    BOOL    fSwitched = FALSE;
    GUID    uidUserId;

    if (RegCreateKey(HKEY_CURRENT_USER, c_szRegRoot, &hkey) == ERROR_SUCCESS)
    {
        DWORD   dwType, dwStatus, dwSize;
        dwSize = sizeof(szUserUid);
        dwStatus = RegQueryValueEx(hkey, c_szLastUserID, NULL, &dwType, (LPBYTE)szUserUid, &dwSize);
        
        RegCloseKey(hkey);

        if (ERROR_SUCCESS == dwStatus && SUCCEEDED(GUIDFromAString(szUserUid, &uidUserId)) && 
                    SUCCEEDED(MU_UserIdToUsername(&uidUserId, szUsername, CCH_USERNAME_MAX_LENGTH)))
        {
            MU_SwitchToUser(szUsername);
            fSwitched = true;
        }
    }

    if (!fSwitched)
    {
        LPSTR   pszName;

        CStringList*    pList = MU_GetUsernameList();
        
        if (pList)
        {
            DWORD   dwIndex, dwLen = pList->GetLength();

            // find the first non hidden user and switch to them
            for (dwIndex = 0; dwIndex < dwLen; dwIndex++)
            {
                pszName = pList->GetString(dwIndex);
                
                if (pszName && *pszName  && *pszName != '_')
                {
                    MU_SwitchToUser(pszName);
                    fSwitched = TRUE;
                    break;
                }
            }
            delete pList;
        }
    }

    if (!fSwitched)
    {
        _MakeDefaultFirstUser();
        CStringList*    pList = MU_GetUsernameList();
        
        if (pList && pList->GetLength() > 0)
            MU_SwitchToUser(pList->GetString(0));
        
        if (pList)
            delete pList; 
    }
}

/*
    _CreateIdentitiesFolder

    Create the parent folder of all of the identities folders.
*/

static void _CreateIdentitiesFolder()
{
    HRESULT     hr;
    TCHAR       szAppDir[MAX_PATH], szSubDir[MAX_PATH], *psz;
    DWORD       dw, type;

    hr = E_FAIL;

    dw = MAX_PATH;


    if (ERROR_SUCCESS == _SHGetValueA(HKEY_CURRENT_USER, c_szRegFolders, c_szValueAppData, &type, (LPBYTE)szAppDir, &dw))
    {
        lstrcpy(szSubDir, c_szIdentitiesFolderName);
        psz = _PathAddBackslash(szSubDir);

        psz = _PathAddBackslash(szAppDir);
        lstrcpy(psz, szSubDir);

        psz = _PathAddBackslash(szAppDir);
        
        CreateDirectory(szAppDir, NULL);

    }
}


/*
    MU_GetCurrentUserDirectoryRoot

    Return the path to the top of the current user's root directory.
    This is the directory where the mail store should be located.
    It is in a subfolder the App Data folder.

    lpszUserRoot is a pointer to a character buffer that is cch chars
    in size.
*/
HRESULT MU_GetUserDirectoryRoot(GUID *uidUserID, DWORD dwFlags, WCHAR   *lpszwUserRoot, int cch)
{
    HRESULT         hr;
    WCHAR           szwSubDir[MAX_PATH], *pszw, szwUid[255]; 
    int             cb;
    DWORD           type, dwDirId;
    LPITEMIDLIST    pidl = NULL;
    IShellFolder   *psf = NULL;
    STRRET          str;
    IMalloc         *pMalloc = NULL;
    BOOL            fNeedHelp = FALSE;

    Assert(lpszUserRoot != NULL);
    Assert(uidUserID);
    Assert(cch >= MAX_PATH);
    Assert((dwFlags & (GIF_NON_ROAMING_FOLDER | GIF_ROAMING_FOLDER)));

    hr = MU_GetDirectoryIdForIdentity(uidUserID, &dwDirId);
    StringFromGUID2(*uidUserID, szwUid, 255);

    if (FAILED(hr))
        return hr;

    hr = SHGetMalloc(&pMalloc);
    Assert(pMalloc);
    if (!pMalloc)
        return E_OUTOFMEMORY;

    hr = E_FAIL;

    if (!!(dwFlags & GIF_NON_ROAMING_FOLDER))
    {
        hr = SHGetSpecialFolderLocation(GetDesktopWindow(), CSIDL_LOCAL_APPDATA, &pidl);
        
        if (FAILED(hr) || pidl == 0)
            hr = SHGetSpecialFolderLocation(GetDesktopWindow(), CSIDL_APPDATA, &pidl);

        if (FAILED(hr))
            fNeedHelp = TRUE;

    }
    else if (!!(dwFlags & GIF_ROAMING_FOLDER))
    {
        hr = SHGetSpecialFolderLocation(GetDesktopWindow(), CSIDL_APPDATA, &pidl);

        if (FAILED(hr))
            fNeedHelp = TRUE;
    }
    else
        hr = E_INVALIDARG;

    *lpszwUserRoot = 0;
    if (SUCCEEDED(hr) && pidl)
    {
        if (FAILED(hr = SHGetDesktopFolder(&psf)))
            goto exit;

        if (FAILED(hr = psf->GetDisplayNameOf(pidl, SHGDN_FORPARSING, &str)))
            goto exit;

        switch(str.uType)
        {
            case STRRET_WSTR:
                lstrcpyW(lpszwUserRoot, str.pOleStr);
                pMalloc->Free(str.pOleStr);
                break;

            case STRRET_OFFSET:
                MultiByteToWideChar(CP_ACP, 0, (LPSTR)pidl+str.uOffset, -1, lpszwUserRoot, cch-11);
                break;

            case STRRET_CSTR:
                MultiByteToWideChar(CP_ACP, 0, (LPSTR)str.cStr, -1, lpszwUserRoot, cch-11);
                break;

            default:
                Assert(FALSE);
                goto exit;
        }

        pszw = PathAddBackslashW(lpszwUserRoot);

        if (lstrlenW(lpszwUserRoot) < cch - 10)
        {
            StrCatW(pszw, L"Identities\\");
            StrCatW(pszw, szwUid);
            StrCatW(pszw, L"\\");
        }
        else
        {
            hr = E_OUTOFMEMORY;
            *lpszwUserRoot = 0;
        }
    }
    else if (fNeedHelp)
    {
        // $$$Review: NEIL QFE
        // SHGetSpecialFolderLocation(GetDesktopWindow(), CSIDL_APPDATA, &pidl) fails on non-SI OSR2.
        HKEY hkeySrc;
        DWORD cb;

        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
                                          0, KEY_QUERY_VALUE, &hkeySrc))
        {
            // -1 for the backslash we may add
            cb = cch - 1;
            if (ERROR_SUCCESS == RegQueryValueExWrapW(hkeySrc, L"AppData", 0, NULL, (LPBYTE)lpszwUserRoot, &cb))
            {
                pszw = PathAddBackslashW(lpszwUserRoot);

                if (lstrlenW(lpszwUserRoot) < cch - 10)
                {
                    StrCatW(pszw, L"Identities\\");
                    StrCatW(pszw, szwUid);
                    StrCatW(pszw, L"\\");
                    hr = S_OK;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    *lpszwUserRoot = 0;
                }
            }

            RegCloseKey(hkeySrc);
        }
    } 
exit:
    Assert(lstrlenW(lpszwUserRoot) > 0);
    SafeRelease(psf);
    pMalloc->Free(pidl);
    SafeRelease(pMalloc);

    return hr;
}


/*
    _ClaimNextUserId

    Get the next available user id.  Currently this means starting 
    with the CURRENT_USER GUID and changing the first DWORD of it
    until it is unique.  
*/
HRESULT   _ClaimNextUserId(GUID *puidId)
{
    ULONG   ulValue = 1;
    DWORD   dwType, dwSize, dwStatus;
    HKEY    hkeyProfiles;
    TCHAR   szUsername[CCH_USERNAME_MAX_LENGTH+1];
    GUID    uid;
    FILETIME    ft;

    if (FAILED(CoCreateGuid(&uid)))
    {
        uid = UID_GIBC_CURRENT_USER;
        GetSystemTimeAsFileTime(&ft);
        uid.Data1 = ft.dwLowDateTime;

        //make sure it hasn't been used
        while (MU_UserIdToUsername(&uid, szUsername, CCH_USERNAME_MAX_LENGTH))
            uid.Data1 ++;
    }
    
    *puidId = uid;

    return S_OK;
}



BOOL MU_GetCurrentUserID(GUID *puidUserID)
{
    BOOL    fFound = FALSE;
    HKEY    hkey;
    GUID    uidUserId;
    TCHAR   szUid[255];

    if (RegCreateKey(HKEY_CURRENT_USER, c_szRegRoot, &hkey) == ERROR_SUCCESS)
    {
        DWORD   dwSize;

        dwSize = 255;
        fFound = (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szLastUserID, 0, NULL, (LPBYTE)szUid, &dwSize));

        if (fFound)
            fFound = SUCCEEDED(GUIDFromAString(szUid, puidUserID));

        if (fFound && *puidUserID == GUID_NULL)
            fFound = false;

        RegCloseKey(hkey);
    }

#ifdef DEBUG
    TCHAR   szUsername[CCH_USERNAME_MAX_LENGTH+1];

    Assert(MU_UserIdToUsername(puidUserID, szUsername, CCH_USERNAME_MAX_LENGTH));
#endif

    return fFound;
}

/*
    MU_UserIdToUsername

    Return the user name for the user whose user id is passed in.  Returns
    whether or not the user was found.
*/
BOOL MU_UserIdToUsername(GUID *puidUserID, TCHAR *lpszUsername, ULONG cch)
{
    HKEY    hkey;
    TCHAR   szPath[MAX_PATH];
    BOOL    fFound = FALSE;

    Assert(lpszUsername);
    lpszUsername[0] = 0;

    MU_GetRegRootForUserID(puidUserID, szPath);    
    Assert(*szPath);

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, szPath, 0, KEY_QUERY_VALUE, &hkey))
    {
        fFound = (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szUsername, 0, NULL, (LPBYTE)lpszUsername, &cch));
        RegCloseKey(hkey);
    }

    return fFound;
}

/*
    MU_CountUsers

    Returns the number of users currently configured.
*/
ULONG  MU_CountUsers(void)
{
    CStringList *psList;
    ULONG       ulCount = 0;

    psList = MU_GetUsernameList();

    if (psList)
    {
        ulCount = psList->GetLength();
        delete psList;
    }

    return ulCount;
}

/*
    MU_GetRegRootForUserid

    Get the reg root path for a given user id.
*/
HRESULT     MU_GetRegRootForUserID(GUID *puidUserID, LPSTR pszPath)
{
    TCHAR szUid[255];

    Assert(pszPath);
    Assert(puidUserID);

    AStringFromGUID(puidUserID,  szUid, 255);
    wsprintf(pszPath, "%.100s\\%.40s", c_szRegRoot, szUid);

    return S_OK;
}

/*
    MU_GetDefaultUserID

    Get the user id for the user who is currently marked as the default user.
    Returns true if the proper user was found, false if not.
*/
BOOL MU_GetDefaultUserID(GUID *puidUserID)
{
    BOOL    fFound = FALSE;
    HKEY    hkey;
    TCHAR    szUid[255];

    if (RegCreateKey(HKEY_CURRENT_USER, c_szRegRoot, &hkey) == ERROR_SUCCESS)
    {
        DWORD   dwSize;

        dwSize = sizeof(szUid);
        fFound = (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szDefaultUserID, 0, NULL, (LPBYTE)szUid, &dwSize));

        if (fFound)
            fFound = SUCCEEDED(GUIDFromAString(szUid, puidUserID));

        RegCloseKey(hkey);
    }

#ifdef DEBUG
    TCHAR   szUsername[CCH_USERNAME_MAX_LENGTH+1];

    Assert(MU_UserIdToUsername(ulUserID, szUsername, CCH_USERNAME_MAX_LENGTH));
#endif

    return fFound;
}

/*
    MU_MakeDefaultUser

    Set the user referenced by id ulUserID to be the default user.
    The default user is referenced by certain applications which
    can only deal with one user. MS Phone is a good example.

*/
HRESULT MU_MakeDefaultUser(GUID *puidUserID)
{
    HRESULT hr = E_FAIL;
    TCHAR   szUsername[CCH_USERNAME_MAX_LENGTH+1];
    TCHAR   szUid[255];
    HKEY    hkey;
    
    // make sure the user exists and get their name to put in the 
    // Default Username reg key
    
    AStringFromGUID(puidUserID,  szUid, 255);
    Assert(*puidUserID != GUID_NULL);

    if (MU_UserIdToUsername(puidUserID, szUsername, CCH_USERNAME_MAX_LENGTH))
    {        
        if (RegCreateKey(HKEY_CURRENT_USER, c_szRegRoot, &hkey) == ERROR_SUCCESS)
        {
            DWORD   dwType, dwSize;
            LONG    lError;

            dwType = REG_SZ;
            dwSize = lstrlen(szUid) + 1;
            lError = RegSetValueEx(hkey, c_szDefaultUserID, 0, dwType, (LPBYTE)szUid, dwSize);

            if (lError)
            {
                hr = E_FAIL;
                goto error;
            }

            hr = S_OK;

error:
            RegCloseKey(hkey);
            
        }
    }

    return hr;
}

/*
    MU_DeleteUser

    Remove a user from the registry.  This does not delete
    anything in the user's folder, but it does blow away
    their reg settings.
*/
HRESULT MU_DeleteUser(GUID *puidUserID)
{
    GUID   uidDefault, uidCurrent;
    TCHAR   szPath[MAX_PATH];

    MU_GetCurrentUserID(&uidCurrent);
    MU_GetDefaultUserID(&uidDefault);

    // Can't delete the current user
    if (*puidUserID == uidCurrent)
        return E_FAIL;
    
    // Can't delete the default user if current user is null
    if (GUID_NULL == uidCurrent && *puidUserID == uidDefault)
        return E_FAIL;
    
    // If they are deleting the default user,
    // make the current user the default
    if (*puidUserID == uidDefault)
        MU_MakeDefaultUser(&uidCurrent);

    MU_GetRegRootForUserID(puidUserID, szPath);
    
    _DeleteKeyRecursively(HKEY_CURRENT_USER, szPath);

    // don't delete the directory since the user may need 
    // data out of it.
    PostMessage(HWND_BROADCAST, WM_IDENTITY_INFO_CHANGED, 0, IIC_IDENTITY_DELETED);

    return S_OK;
}


/*
    MU_CreateUser

    Create a user with the user info passed in.  This includes 
    creating their spot in the registry and their directory in the
    identities folder.
*/

HRESULT MU_CreateUser(LPUSERINFO   lpUserInfo)
{
    TCHAR           szPath[MAX_PATH], szBuffer[MAX_PATH], szUid[255];
    WCHAR           szwPath[MAX_PATH];
    HKEY            hkey;
    HRESULT         hr = S_OK;
    DWORD           dwType, dwSize, cbSize, dwValue;
    PASSWORD_STORE  pwStore;

    MU_GetRegRootForUserID(&lpUserInfo->uidUserID, szPath);
    
    Assert(*szPath && *szAcctPath);
 
    AStringFromGUID(&lpUserInfo->uidUserID,  szUid, 255);
    Assert(lpUserInfo->uidUserID != GUID_NULL);

    if (RegCreateKey(HKEY_CURRENT_USER, szPath, &hkey) == ERROR_SUCCESS)
    {
        // write out the correct values
        dwType = REG_SZ;
        dwSize = lstrlen(lpUserInfo->szUsername) + 1;
        RegSetValueEx(hkey, c_szUsername, 0, dwType, (LPBYTE)lpUserInfo->szUsername, dwSize);

#ifdef IDENTITY_PASSWORDS
        lstrcpy(pwStore.szPassword, lpUserInfo->szPassword);
        pwStore.fUsePassword = lpUserInfo->fUsePassword;
        if (FAILED(hr = WriteIdentityPassword(&lpUserInfo->uidUserID, &pwStore)))
        {
            dwType = REG_BINARY ;
            cbSize = strlen(lpUserInfo->szPassword) + 1;
            lstrcpy(szBuffer, lpUserInfo->szPassword);
            EncodeUserPassword(szBuffer, &cbSize);
            dwSize = cbSize;
            RegSetValueEx(hkey, c_szPassword, 0, dwType, (LPBYTE)szBuffer, dwSize);
        
            dwType = REG_DWORD;
            dwValue = (lpUserInfo->fUsePassword ? 1 : 0);
            dwSize = sizeof(dwValue);
            RegSetValueEx(hkey, c_szUsePassword, 0, dwType, (LPBYTE)&dwValue, dwSize);
        }
#endif //IDENTITY_PASSWORDS
        dwType = REG_SZ;
        dwSize = lstrlen(szUid) + 1;
        RegSetValueEx(hkey, c_szUserID, 0, dwType, (LPBYTE)&szUid, dwSize);
    
        RegCloseKey(hkey);

        if (SUCCEEDED(MU_GetUserDirectoryRoot(&lpUserInfo->uidUserID, GIF_ROAMING_FOLDER, szwPath, MAX_PATH)))
            if (!CreateDirectoryWrapW(szwPath,NULL))
            {
                _CreateIdentitiesFolder();
                CreateDirectoryWrapW(szwPath,NULL);
            }

        if (SUCCEEDED(MU_GetUserDirectoryRoot(&lpUserInfo->uidUserID, GIF_NON_ROAMING_FOLDER, szwPath, MAX_PATH)))
            if (!CreateDirectoryWrapW(szwPath,NULL))
            {
                _CreateIdentitiesFolder();
                CreateDirectoryWrapW(szwPath,NULL);
            }
    }
    else
        hr = E_FAIL;

    return hr;
}

/*
    MU_GetRegRoot

    Returns a pointer to a string containing the location 
    in HKEY_CURRENT_USER for the current user.
*/
LPCTSTR     MU_GetRegRoot()
{
    if (*g_szRegRoot)
        return g_szRegRoot;
    else
    {
        TCHAR   szUsername[CCH_USERNAME_MAX_LENGTH + 1];

        if (MU_Login(NULL, 0, szUsername))
        {
            GUID uidUserId;
            TCHAR szUid[255];
            
            MU_UsernameToUserId(szUsername, &uidUserId);

            AStringFromGUID(&uidUserId,  szUid, 255);
            wsprintf(g_szRegRoot, "%.100s\\%.40s", c_szRegRoot, szUid);

            return g_szRegRoot;
        }
        else
        {
            Assert(FALSE);
        }
    }
    return NULL;
}


void _MakeDefaultFirstUser()
{
    USERINFO    nuInfo;
    TCHAR        szUid[255];

    MLLoadStringA(idsMainUser, nuInfo.szUsername, CCH_USERNAME_MAX_LENGTH);
	if (nuInfo.szUsername[0] == 0)
	{
		lstrcpy(nuInfo.szUsername, TEXT("Main Identity"));
	}
    *nuInfo.szPassword = 0;
    nuInfo.fUsePassword = false;
    nuInfo.fPasswordValid = true;
    _ClaimNextUserId(&nuInfo.uidUserID);

    MU_CreateUser(&nuInfo);
    MU_MakeDefaultUser(&nuInfo.uidUserID);
    MU_SwitchToUser(nuInfo.szUsername);

    AStringFromGUID(&nuInfo.uidUserID,  szUid, 255);
    wsprintf(g_szRegRoot, "%.100s\\%.40s", c_szRegRoot, szUid);
}

void FixMissingIdentityNames()
{
    HKEY    hSourceSubKey;
    ULONG   ulEnumIndex = 0;
    DWORD   dwStatus, dwSize, dwType, dwValue;
    BOOL    fFound = FALSE;
    TCHAR   szKeyNameBuffer[MAX_PATH];
	TCHAR	szUsername[CCH_USERNAME_MAX_LENGTH];

    if (RegCreateKey(HKEY_CURRENT_USER, c_szRegRoot, &hSourceSubKey) == ERROR_SUCCESS)
    {
        while (!fFound) 
        {
            HKEY    hkUserKey;

            if (RegEnumKey(hSourceSubKey, ulEnumIndex++, szKeyNameBuffer,MAXKEYNAME)
                !=  ERROR_SUCCESS)
                break;
            
            if (RegOpenKey(hSourceSubKey, szKeyNameBuffer, &hkUserKey) == ERROR_SUCCESS)
            {
                dwSize = sizeof(szUsername);
                dwStatus = RegQueryValueEx(hkUserKey, c_szUsername, NULL, &dwType, (LPBYTE)szUsername, &dwSize);
                
                if (ERROR_SUCCESS != dwStatus || 0 == szUsername[0])
                {
					lstrcpy(szUsername, "Main Identity");
					dwStatus = RegSetValueEx(hkUserKey, c_szUsername, 0, REG_SZ, (LPBYTE)szUsername, lstrlen(szUsername)+1);
                }
                RegCloseKey(hkUserKey); 
            }
        }
        RegCloseKey(hSourceSubKey);
    }
}

typedef DWORD (STDAPICALLTYPE *PNetWkstaUserGetInfo)
    (LPWSTR reserved, DWORD level, LPBYTE *bufptr);


#if 0
/*
    _DomainControllerPresent

    Identities are disabled when the machine they are running on is part of a domain, unless 
    there is a policy to explicitly allow them.  This function checks to see if the machine
    is joined to a domain.
*/
BOOL _DomainControllerPresent()
{
    static BOOL fInDomain = FALSE;
    static BOOL fValid = FALSE;
    HINSTANCE  hInst;
    PNetWkstaUserGetInfo pNetWkstaUserGetInfo;
    _WKSTA_USER_INFO_1  *pwui1;

    if (!fValid)
    {
        fValid = TRUE;
        hInst = LoadLibrary(TEXT("NETAPI32.DLL"));

        if (hInst)
        {
            pNetWkstaUserGetInfo = (PNetWkstaUserGetInfo)GetProcAddress(hInst, TEXT("NetWkstaUserGetInfo"));

            if (pNetWkstaUserGetInfo && (pNetWkstaUserGetInfo(NULL, 1, (LPBYTE*)&pwui1) == NOERROR))
            {
                if (pwui1->wkui1_logon_domain && pwui1->wkui1_logon_server && lstrcmpW(pwui1->wkui1_logon_server, pwui1->wkui1_logon_domain) != 0)
                {
                    fInDomain = TRUE;
                }
            }
            FreeLibrary(hInst);
        }
    }
    return fInDomain;
}
#endif

/*
    MU_IdentitiesDisabled

    Returns if identities is disabled due to a policy 
    or whatever.
*/
BOOL MU_IdentitiesDisabled()
{
    TCHAR   szPolicyPath[] = "Software\\Policies\\Microsoft\\Windows\\CurrentVersion\\Identities";
    HKEY    hkey;
    DWORD   dwValue, dwSize;
    BOOL    fLockedDown = FALSE;

    if (RegOpenKey(HKEY_LOCAL_MACHINE, szPolicyPath, &hkey) == ERROR_SUCCESS)
    { 
        dwSize = sizeof(DWORD);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szPolicyKey, 0, NULL, (LPBYTE)&dwValue, &dwSize) && 1 == dwValue)
            fLockedDown = TRUE;

        RegCloseKey(hkey);
    }

    if (!fLockedDown && RegOpenKey(HKEY_CURRENT_USER, szPolicyPath, &hkey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(DWORD);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szPolicyKey, 0, NULL, (LPBYTE)&dwValue, &dwSize) && 1 == dwValue)
            fLockedDown = TRUE;

        RegCloseKey(hkey);
    }

#if 0
    // turned off for now, pending determination of whether we even want to
    // have this policy
    if (!fLockedDown && _DomainControllerPresent())
    {
        fLockedDown = TRUE;

        if (RegOpenKey(HKEY_LOCAL_MACHINE, szPolicyPath, &hkey) == ERROR_SUCCESS)
        {
            dwSize = sizeof(DWORD);
            if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szEnableDCPolicyKey, 0, NULL, (LPBYTE)&dwValue, &dwSize) && 1 == dwValue)
                fLockedDown = FALSE;

            RegCloseKey(hkey);
        }

        if (fLockedDown && RegOpenKey(HKEY_CURRENT_USER, szPolicyPath, &hkey) == ERROR_SUCCESS)
        {
            dwSize = sizeof(DWORD);
            if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szEnableDCPolicyKey, 0, NULL, (LPBYTE)&dwValue, &dwSize) && 1 == dwValue)
                fLockedDown = FALSE;

            RegCloseKey(hkey);
        }
    }
#endif

    return fLockedDown;
}

static GUID    g_uidLoginOption;
static BOOLEAN g_uidLoginOptionSet;

void  _ResetRememberedLoginOption(void)
{
    g_uidLoginOption = GUID_NULL;
    g_uidLoginOptionSet = FALSE;
}

void  _RememberLoginOption(HWND hwndCombo)
{
    LRESULT dFoundItem;
    TCHAR   szUsername[CCH_IDENTITY_NAME_MAX_LENGTH * 2];
    GUID    uidUser;

    *szUsername = 0;

    g_uidLoginOptionSet = TRUE;

    dFoundItem = SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);

    SendMessage(hwndCombo, CB_GETLBTEXT, dFoundItem, (LPARAM)szUsername);
    
    if (FAILED(MU_UsernameToUserId(szUsername, &uidUser)))
        g_uidLoginOption = GUID_NULL;
    else
        g_uidLoginOption = uidUser;
}

DWORD MU_GetDefaultOptionIndex(HWND hwndCombo)
{
    GUID        uidStart, uidDefault;
    USERINFO    uiDefault;
    DWORD       dwResult = 0;

    if (MU_GetDefaultUserID(&uidDefault))
    {
        MU_GetUserInfo(&uidDefault, &uiDefault);

        if (uiDefault.szUsername[0])
        {
            dwResult = (DWORD)SendMessage(hwndCombo, CB_FINDSTRING, 0, (LPARAM)uiDefault.szUsername);
        }
    }
    return dwResult;
}

DWORD MU_GetLoginOptionIndex(HWND hwndCombo)
{
    GUID        uidStart, uidDefault;
    USERINFO    uiLogin;
    DWORD       dwResult = ASK_BEFORE_LOGIN;

    if (GUID_NULL == g_uidLoginOption)
    {
        if (g_uidLoginOptionSet)
            goto exit;

        MU_GetLoginOption(&uidStart);
    }
    else
        uidStart = g_uidLoginOption;
    
    if (uidStart == GUID_NULL)
        goto exit;

    if(!MU_GetUserInfo(&uidStart, &uiLogin))
        goto exit;

    dwResult = (DWORD)SendMessage(hwndCombo, CB_FINDSTRING, 0, (LPARAM)uiLogin.szUsername);
exit:
    return dwResult;
}
/*
    MU_GetLoginOption

    return the user's choice for what should happen when there is no current 
    user
*/

void MU_GetLoginOption(GUID *puidStartAs)
{
    HKEY    hkey;
    DWORD   dwSize;
    TCHAR   szUid[255];
    GUID    uidUser;

    ZeroMemory(puidStartAs, sizeof(GUID));
    if (RegCreateKey(HKEY_CURRENT_USER, c_szRegRoot, &hkey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(szUid);
        if (ERROR_SUCCESS != RegQueryValueEx(hkey, c_szLoginAs, 0, NULL, (LPBYTE)szUid, &dwSize))
            MU_GetDefaultUserID(puidStartAs);
        else
            GUIDFromAString(szUid, puidStartAs);

        RegCloseKey(hkey);
    }
}

/*
    MU_SetLoginOption

    return the user's choice for what should happen when there is no current 
    user
*/

BOOL MU_SetLoginOption(HWND hwndCombo,  LRESULT dOption)
{
    HKEY    hkey;
    BOOL    fResult = FALSE;
    TCHAR   szUsername[CCH_IDENTITY_NAME_MAX_LENGTH * 2];
    TCHAR   szUid[255];
    GUID    uidUser;


    SendMessage(hwndCombo, CB_GETLBTEXT, dOption, (LPARAM)szUsername);
    
    if (dOption == (LRESULT)ASK_BEFORE_LOGIN || FAILED(MU_UsernameToUserId(szUsername, &uidUser)))
    {
        ZeroMemory(&uidUser, sizeof(uidUser));
    }
    AStringFromGUID(&uidUser,  szUid, sizeof(szUid));
    
    if (RegCreateKey(HKEY_CURRENT_USER, c_szRegRoot, &hkey) == ERROR_SUCCESS)
    {
        fResult = (ERROR_SUCCESS == RegSetValueEx(hkey, c_szLoginAs, 0, REG_SZ, (LPBYTE)szUid, lstrlen(szUid)+1));

        RegCloseKey(hkey);
    }

    return TRUE;
}


/*
    MU_CanEditIdentity

    Is the current identity allowed to edit the indicated identity's settings?
*/
BOOL MU_CanEditIdentity(HWND hwndParent, GUID *puidIdentityId)
{
#ifndef IDENTITY_PASSWORDS
    return TRUE;
#else
    USERINFO        uiCurrent, uiQuery;
    TCHAR           szBuffer[255];    // really ought to be big enough
    LPTSTR          lpString = NULL;
    TCHAR*          rgsz[1] = {uiQuery.szUsername};
    BOOL            fResult = FALSE;
    PASSWORD_STORE  pwStore;

    ZeroMemory(&uiQuery, sizeof(USERINFO));

    if (MU_GetUserInfo(puidIdentityId, &uiQuery))
    {
        if (!uiQuery.fPasswordValid)
        {
            MU_ShowErrorMessage(hwndParent, idsPwdNotFound, idsPwdError);
            return FALSE;
        }

        if (uiQuery.szPassword[0] == 0)
        {
            return TRUE;    
        }
        
        if (MU_GetUserInfo(NULL, &uiCurrent))
        {
            if (uiCurrent.uidUserID == uiQuery.uidUserID)
                return TRUE;
        }
    }
    else
        return FALSE;

    MLLoadStringA(idsConfirmEdit, szBuffer, sizeof(szBuffer));

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_STRING |
                  FORMAT_MESSAGE_ARGUMENT_ARRAY,
                  szBuffer,
                  0, 0,
                  (LPTSTR)&lpString, 0, (va_list *)rgsz);
    
    if (lpString)
    {

        fResult = MU_ConfirmUserPassword(hwndParent, lpString, uiQuery.szPassword);

        LocalFree(lpString);
    }

    return fResult;
#endif //IDENTITY_PASSWORDS
}

static BOOL _DirectoryIdInUse(DWORD dwId)
{
    HKEY    hSourceSubKey;
    ULONG   ulEnumIndex = 0;
    DWORD   dwStatus, dwSize, dwType, dwValue;
    BOOL    fFound = FALSE;
    TCHAR   szKeyNameBuffer[MAX_PATH];

    if (RegCreateKey(HKEY_CURRENT_USER, c_szRegRoot, &hSourceSubKey) == ERROR_SUCCESS)
    {
        while (!fFound) 
        {
            HKEY    hkUserKey;

            if (RegEnumKey(hSourceSubKey, ulEnumIndex++, szKeyNameBuffer,MAXKEYNAME)
                !=  ERROR_SUCCESS)
                break;
            
            if (RegOpenKey(hSourceSubKey, szKeyNameBuffer, &hkUserKey) == ERROR_SUCCESS)
            {
                dwSize = sizeof(dwValue);
                dwStatus = RegQueryValueEx(hkUserKey, c_szDirName, NULL, &dwType, (LPBYTE)&dwValue, &dwSize);
                
                if (ERROR_SUCCESS == dwStatus && dwValue == dwId)
                {
                    fFound = TRUE;
                    RegCloseKey(hkUserKey); 
                    break;
                }
                RegCloseKey(hkUserKey); 
            }
        }
        RegCloseKey(hSourceSubKey);
    }
    

    return fFound;
}


DWORD   MU_GenerateDirectoryNameForIdentity(GUID *puidIdentityId)
{   
    DWORD dwId, dwRegValue;

    dwId = puidIdentityId->Data1;

    while (_DirectoryIdInUse(dwId))
        dwId++;

    return dwId;
}

HRESULT MU_GetDirectoryIdForIdentity(GUID *puidIdentityId, DWORD *pdwDirId)
{
    TCHAR   szRegPath[MAX_PATH];
    HKEY    hkey;
    HRESULT hr = E_FAIL;
    DWORD   dwSize, dwStatus, dwValue, dwType;

    MU_GetRegRootForUserID(puidIdentityId, szRegPath);

    if (RegOpenKey(HKEY_CURRENT_USER, szRegPath, &hkey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(dwValue);
        dwStatus = RegQueryValueEx(hkey, c_szDirName, NULL, &dwType, (LPBYTE)&dwValue, &dwSize);
    
        if (ERROR_SUCCESS == dwStatus)
        {
            *pdwDirId = dwValue;
            hr = S_OK;
        }
        else
        {
            // try to generate one
            dwValue = MU_GenerateDirectoryNameForIdentity(puidIdentityId);
        
            dwType = REG_DWORD;
            dwSize = sizeof(dwValue);
            dwStatus = RegSetValueEx(hkey, c_szDirName, 0, dwType, (LPBYTE)&dwValue, dwSize);

            if (ERROR_SUCCESS == dwStatus)
            {
                *pdwDirId = dwValue;
                hr = S_OK;
            }
        }
        RegCloseKey(hkey);
    }

    return hr;
}


void _MigratePasswords()
{
    CStringList *psList;
    int   i, iCount = 0;
	USERINFO uiUser;
	DWORD dwStatus, dwValue, dwType, dwSize;

	dwType = REG_DWORD;
	dwSize = sizeof(DWORD);
	dwStatus = SHGetValue(HKEY_CURRENT_USER, c_szRegRoot, c_szMigrated5, &dwType, &dwValue, &dwSize);	

	if (dwStatus == ERROR_SUCCESS && dwValue == 1)
		return;
		
    psList = MU_GetUsernameList();

    if (psList)
    {
        iCount = psList->GetLength();

		for (i = 0; i < iCount; i++)
		{
			GUID	uidUser;
			if (SUCCEEDED(MU_UsernameToUserId(psList->GetString(i), &uidUser)) 
				&& MU_GetUserInfo(&uidUser, &uiUser))
			{
				if (!uiUser.fPasswordValid)
				{
					uiUser.fUsePassword = false;
					*uiUser.szPassword = 0;
					MU_SetUserInfo(&uiUser);
				}
			}
		}
        delete psList;
    }

	dwValue = 1;
	SHSetValue(HKEY_CURRENT_USER, c_szRegRoot, c_szMigrated5, REG_DWORD, &dwValue, sizeof(DWORD));	
}


