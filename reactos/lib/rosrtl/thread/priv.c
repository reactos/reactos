#include <windows.h>
#include <rosrtl/priv.h>

/*
 * Utility to copy and enable thread privileges
 *
 * OUT HANDLE *hPreviousToken -> Pointer to a variable that receives the previous token handle
 * IN  LUID *Privileges       -> Points to an array of privileges to be enabled
 * IN  DWORD PrivilegeCount   -> Number of the privileges in the array
 *
 * Returns TRUE on success and copies the thread token (if any) handle that was active before impersonation.
 */
BOOL
RosEnableThreadPrivileges(HANDLE *hPreviousToken,
                          LUID *Privileges,
                          DWORD PrivilegeCount)
{
  HANDLE hToken;
  
  if(hPreviousToken == NULL ||
     Privileges == NULL || PrivilegeCount == 0)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  
  /* Just open the thread token, we'll duplicate the handle later */
  if(OpenThreadToken(GetCurrentThread(), TOKEN_DUPLICATE, FALSE, &hToken))
  {
    PTOKEN_PRIVILEGES privs;
    PLUID_AND_ATTRIBUTES la;
    HANDLE hNewToken;
    BOOL Ret;
    
    /* duplicate the token handle */
    if(!DuplicateTokenEx(hToken, TOKEN_IMPERSONATE, NULL, SecurityImpersonation,
                         TokenImpersonation, &hNewToken))
    {
      CloseHandle(hToken);
      return FALSE;
    }
    
    /* Allocate the required space for the privilege list */
    privs = (PTOKEN_PRIVILEGES)LocalAlloc(LPTR, sizeof(TOKEN_PRIVILEGES) +
                                          ((PrivilegeCount - 1) * sizeof(LUID_AND_ATTRIBUTES)));
    if(privs == NULL)
    {
      CloseHandle(hNewToken);
      CloseHandle(hToken);
      return FALSE;
    }
    
    /* Build the privilege list */
    privs->PrivilegeCount = PrivilegeCount;
    for(la = privs->Privileges; PrivilegeCount-- > 0; la++)
    {
      la->Luid = *(Privileges++);
      la->Attributes = SE_PRIVILEGE_ENABLED;
    }
    
    /* Enable the token privileges */
    if(!AdjustTokenPrivileges(hNewToken, FALSE, privs, 0, NULL, NULL))
    {
      LocalFree((HLOCAL)privs);
      CloseHandle(hNewToken);
      CloseHandle(hToken);
      return FALSE;
    }
    
    /* we don't need the privileges list anymore */
    LocalFree((HLOCAL)privs);
    
    /* Perform the impersonation */
    Ret = SetThreadToken(NULL, hNewToken);
    
    if(Ret)
    {
      /* only copy the previous token handle on success */
      *hPreviousToken = hToken;
    }
    else
    {
      /* We don't return the previous token handle on failure, so close it here */
      CloseHandle(hToken);
    }
    
    /* We don't need the handle to the new token anymore */
    CloseHandle(hNewToken);
    
    return Ret;
  }
  
  return FALSE;
}


/*
 * Utility to reset the thread privileges previously changed with RosEnableThreadPrivileges()
 *
 * IN HANDLE hToken -> Handle of the thread token to be restored
 *
 * Returns TRUE on success.
 */
BOOL
RosResetThreadPrivileges(HANDLE hToken)
{
  if(hToken != INVALID_HANDLE_VALUE)
  {
    BOOL Ret;

    /* Set the previous token handle */
    Ret = SetThreadToken(NULL, hToken);

    /* We don't need the handle anymore, close it */
    CloseHandle(hToken);
    return Ret;
  }
  return FALSE;
}

