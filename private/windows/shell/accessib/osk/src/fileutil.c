//FILEUTIL.C    file utilities

#define STRICT

#include <windows.h>
#include <malloc.h>

#include "kbmain.h"
#include "resource.h"

//
// This header contains the default settings in a global variable.
// This file was generated from the osksetti.reg file using a perl
// script.  If the:
//   Software\\Microsoft\\Osk  key is missing or the 
//   Settings value is empty then we will create this value from this varable.
//
#include "osksetti.h"


#define ACL_BUFFER_SIZE     1024
#define REG_INSTALLED       TEXT("Installed")      // Last value written during
                                                   // application installation.
/****************************************************************************/
extern BOOL settingChanged;
extern DWORD platform;

/****************************************************************************/
/*    FUNCTIONS IN THIS FILE											    */
/****************************************************************************/

PSID GetCurrentUserInfo(void);
BOOL RunningAsAdministrator(void);
BOOL OpenUserSetting(void);
BOOL SaveUserSetting(void);

/**************************************************************/


PSID GetCurrentUserInfo(void)
{
   // This function returns security information about the person who owns
   // this thread.

   HANDLE htkThread;

   TOKEN_USER *ptu;
   DWORD      cbtu;

   TOKEN_GROUPS *ptg = NULL;
   SID_IDENTIFIER_AUTHORITY SystemSidAuthority= SECURITY_NT_AUTHORITY;

   // First we must open a handle to the access token for this thread.

   if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &htkThread))
      if (GetLastError() == ERROR_NO_TOKEN)
      {
         // If the thread does not have an access token, we'll examine the
         // access token associated with the process.

         if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &htkThread))
         return NULL;
      }
      else return NULL;


   if (GetTokenInformation(htkThread, TokenUser, NULL, 0, &cbtu))
      return NULL;

   // Here we verify that GetTokenInformation failed for lack of a large
   // enough buffer.

   if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
      return NULL;

   // Now we allocate a buffer for the group information.
   // Since _alloca allocates on the stack, we don't have
   // to explicitly deallocate it. That happens automatically
   // when we exit this function.

   if (!(ptu= LocalAlloc(LPTR, cbtu))) return NULL;

   // Now we ask for the user information again.
   // This may fail if an administrator has changed SID information
   // for this user.

   if (!GetTokenInformation(htkThread, TokenUser, ptu, cbtu, &cbtu))
      return FALSE;

// if (GetTokenInformation(htkThread, TokenUser, &tu, sizeof(tu), &cbtu))
//    return NULL;

   return ptu;
}
/***************************************************************************/

typedef HRESULT (*CHECKTOKENMEMBERSHIP)(HANDLE TokenHandle, PSID SidToCheck, PBOOL IsMember);

// CheckToken Returns TRUE the New NT5 CheckTokenMembership works
// Returns FALSE otherwise.
BOOL CheckToken(HANDLE hAccessToken, BOOL *pfIsAdmin)
{
    BOOL bNewNT5check = FALSE;
    HINSTANCE hAdvapi32 = NULL;
    CHECKTOKENMEMBERSHIP pf;
    PSID AdministratorsGroup;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    hAdvapi32 = LoadLibrary(TEXT("advapi32.dll"));
    if (hAdvapi32)
    {
        pf = (CHECKTOKENMEMBERSHIP)GetProcAddress(hAdvapi32, "CheckTokenMembership");
        if (pf)
        {
            if(AllocateAndInitializeSid( &NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
              DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup) )
            {
                bNewNT5check = pf(hAccessToken, AdministratorsGroup, pfIsAdmin);
                FreeSid(AdministratorsGroup);
            }
        }
        FreeLibrary(hAdvapi32);
    }
    return bNewNT5check;
}

// Returns true if our process has admin priviliges.
// Returns false otherwise.
// We need to know this in order to start UtilMan as an app when we
// do it from menu under non-admin account
// We also return FALSE when any allocation or other error is encountered
BOOL RunningAsAdministrator()
{
   BOOL  fAdmin;
   HANDLE htkThread;
   TOKEN_GROUPS *ptg = NULL;
   DWORD cbTokenGroups;
   DWORD iGroup;
   SID_IDENTIFIER_AUTHORITY SystemSidAuthority= SECURITY_NT_AUTHORITY;
   PSID psidAdmin;

   // This function returns TRUE if the user identifier associated with this
   // process is a member of the the Administrators group.

   // First we must open a handle to the access token for this thread.

   if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &htkThread))
      if (GetLastError() == ERROR_NO_TOKEN)
      {
         // If the thread does not have an access token, we'll examine the
         // access token associated with the process.

         if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &htkThread))
         return FALSE;
      }
      else return FALSE;

      if (CheckToken(htkThread, &fAdmin))
      {
		 return fAdmin;
	  }
   // Then we must query the size of the group information associated with
   // the token. Note that we expect a FALSE result from GetTokenInformation
   // because we've given it a NULL buffer. On exit cbTokenGroups will tell
   // the size of the group information.

   if (GetTokenInformation(htkThread, TokenGroups, NULL, 0, &cbTokenGroups))
      return FALSE;

   // Here we verify that GetTokenInformation failed for lack of a large
   // enough buffer.

   if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
      return FALSE;

   // Now we allocate a buffer for the group information.
   // Since _alloca allocates on the stack, we don't have
   // to explicitly deallocate it. That happens automatically
   // when we exit this function.

   if (!(ptg= _alloca(cbTokenGroups))) return FALSE;

   // Now we ask for the group information again.
   // This may fail if an administrator has added this account
   // to an additional group between our first call to
   // GetTokenInformation and this one.

    if (!GetTokenInformation(htkThread, TokenGroups, ptg, cbTokenGroups, 
                            &cbTokenGroups))
    {
        return FALSE;
    }

   // Now we must create a System Identifier for the Admin group.

   if (!AllocateAndInitializeSid
          (&SystemSidAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                   DOMAIN_ALIAS_RID_ADMINS,
                                   0, 0, 0, 0, 0, 0,
                                   &psidAdmin
          )
      )
      return FALSE;

   // Finally we'll iterate through the list of groups for this access
   // token looking for a match against the SID we created above.

   fAdmin= FALSE;

   for (iGroup= 0; iGroup < ptg->GroupCount; iGroup++)
   {
      if (EqualSid(ptg->Groups[iGroup].Sid, psidAdmin))
      {
         fAdmin= TRUE;

         break;
      }
   }

   // Before we exit we must explicity deallocate the SID
   // we created.

   FreeSid(psidAdmin);

   return(fAdmin);
}

/****************************************************************************/
BOOL OpenUserSetting(void)
{  
//   HKEY hkGlobal;
   TCHAR pathbuff[50]=TEXT("Software\\Microsoft\\Osk");
   
   HKEY hkPerUser  = NULL;
   LONG lResult;
   DWORD dwType, cbData, dwStepping;
   DWORD dwDisposition;
   TOKEN_USER *ptu = NULL;
   PSID psidUser   = NULL,
        psidAdmins = NULL;
   PACL  paclKey = NULL;
   SID_IDENTIFIER_AUTHORITY		SystemSidAuthority= SECURITY_NT_AUTHORITY;
   SECURITY_ATTRIBUTES sa;
   SECURITY_DESCRIPTOR sdPermissions;

   TCHAR errstr[256]=TEXT("");
   TCHAR title[256]=TEXT("");
    //a-anilk
   int actualKeybdType;
   // First we'll setup the security attributes we're going to
   // use with the application's global key.

   sa.nLength              = sizeof(SECURITY_ATTRIBUTES);
   sa.bInheritHandle       = FALSE;
   sa.lpSecurityDescriptor = &sdPermissions;

	// ***  Do extra checking if we are in NT  ***
	if(platform == VER_PLATFORM_WIN32_NT)   
	{	
		// Here we're creating a System Identifier (SID) to represent
		// the Admin group.
		if (!AllocateAndInitializeSid
			  (&SystemSidAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                   DOMAIN_ALIAS_RID_ADMINS,
                                   0, 0, 0, 0, 0, 0,
                                   &psidAdmins))
        {
			goto security_failure;
        }

		// We also need a SID for the current user.

		if (!(ptu= GetCurrentUserInfo()) || !(psidUser= ptu->User.Sid)) 
			goto security_failure;

		if (!InitializeSecurityDescriptor(&sdPermissions, 
                                          SECURITY_DESCRIPTOR_REVISION1))
        {
			goto security_failure;
        }

		// We want the current user to own this key.

		if (!SetSecurityDescriptorOwner(&sdPermissions, psidUser, 0))
			goto security_failure;

		// Finally we must allocate and construct the discretionary
		// access control list (DACL) for the key.

		// Note that _alloca allocates memory on the stack frame
		// which will automatically be deallocated when this routine
		// exits.

		if (!(paclKey= (PACL) _alloca(ACL_BUFFER_SIZE)))
			goto memory_limited;

		if (!InitializeAcl(paclKey, ACL_BUFFER_SIZE, ACL_REVISION2))
			goto security_failure;

		// Our DACL will two access control entries (ACEs). The first ACE
		// provides full access to the current user. The second ACE gives
		// the Admin group full access. By default all other users will have
		// no access to the key.

		// The reason for admin access is to allow an administrator to
		// run special utilties to cleanup inconsistencies and disasters
		// in the per-user data area.

		if (!AddAccessAllowedAce(paclKey, ACL_REVISION2, KEY_ALL_ACCESS, psidUser))
			goto security_failure;

		if (!AddAccessAllowedAce(paclKey, ACL_REVISION2, KEY_ALL_ACCESS, psidAdmins))
			goto security_failure;

		// We must bind this DACL to the security descriptor...

		if (!SetSecurityDescriptorDacl(&sdPermissions, TRUE, paclKey, FALSE))
			goto security_failure;

	}   //end of extra checking for NT
    
	// Now we'll attempt to create the key with the security attributes...

	lResult= RegCreateKeyEx(HKEY_CURRENT_USER, 
                            &pathbuff[0], 
                            0,
                            TEXT("Application Per-User Data"), 
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            &sa, 
                            &hkPerUser, 
                            &dwDisposition);

	if (lResult != ERROR_SUCCESS) 
    {
        goto registry_access_error;
    }

   // Usually the disposition value will indicate that we've created a
   // new key. Sometimes it may instead state that we've opened an existing
   // key. This can happen when installation is incomplete and interrupted,
   // say by loss of electrical power.

	if ((dwDisposition != REG_CREATED_NEW_KEY) && 
        (dwDisposition != REG_OPENED_EXISTING_KEY)) 
    {
		goto registry_access_error;
    }

	
	kbPref = (KBPREFINFO *)malloc(sizeof(KBPREFINFO));   

    if (kbPref == NULL)
        goto memory_limited;

    dwType=REG_DWORD;
    cbData=sizeof(DWORD);
    lstrcpy(pathbuff,TEXT("Stepping"));
	lResult=RegQueryValueEx(hkPerUser, &pathbuff[0], NULL, &dwType, 
                            (LPBYTE)&dwStepping, &cbData); 

    if (lResult != ERROR_SUCCESS)
        dwStepping=0;

	dwType= REG_BINARY;
	cbData= sizeof(KBPREFINFO);
	lstrcpy(pathbuff, TEXT("Setting"));
	lResult=RegQueryValueEx(hkPerUser, &pathbuff[0], NULL, &dwType, 
                            (LPBYTE)kbPref, &cbData); 

	if((lResult != ERROR_SUCCESS) || (dwStepping < CURRENT_STEPPING)) 
    {
        //
        // if it is not there then create the default Settings value
        //
        RegSetValueEx(hkPerUser, pathbuff, 0, REG_BINARY,  
                      g_DefaultSettings, sizeof(g_DefaultSettings));
        
    	cbData= sizeof(KBPREFINFO);
	    lResult=RegQueryValueEx(hkPerUser, &pathbuff[0], NULL, &dwType, 
                                (LPBYTE)kbPref, &cbData); 

        // HACK by a-anilk
        // This is the place where the Default values are taken by OSK. 
        // Just change the keyboard layout based on the keyboard type used
        actualKeybdType = GetKeyboardType(0);
        switch(actualKeybdType)
        {
            case 1:
            case 3:
            case 4:
            case 5:
            case 6:
                // 101 keyboard
                kbPref->KBLayout = 101;
                break;

            case 2:
                // 102 keyboard
                kbPref->KBLayout = 102;
                break;

            case 7:
                // Japanese Keyboard
                kbPref->KBLayout = 106;
                break;

            default:
                // 101 keyboard
                kbPref->KBLayout = 101;
                break;
        }

        if (lResult != ERROR_SUCCESS)
    		goto registry_access_error;

        //
        // update the stepping
        //
        dwType=REG_DWORD;
	    cbData=sizeof(DWORD);
        lstrcpy(pathbuff,TEXT("Stepping"));
        dwStepping=CURRENT_STEPPING;

        RegSetValueEx(hkPerUser, pathbuff, 0, REG_DWORD,  
                      (LPBYTE)&dwStepping, sizeof(DWORD));
    }

	RegCloseKey(hkPerUser);
	FreeSid(psidAdmins);
	LocalFree(ptu);
	return(TRUE);

//**************
//Error handler
//**************
/*
registry_damage_error:

   // We'll display a warning that the registry info has
   // been damaged.
	LoadString(hInst, IDS_REGISTRY_DAMAGE, &errstr[0], 256);
	LoadString(hInst, IDS_TITLE1, &title[0], 256);
	MessageBox(0, errstr, title, MB_OK|MB_ICONHAND);

   // Then we discard the REG_INSTALLED flag to insure that a reinstallation
   // can proceed.

   RegDeleteValue(hkGlobal, REG_INSTALLED);

   goto clean_up_registry_keys;
*/

registry_access_error:

	LoadString(hInst, IDS_REGISTRY_ACCESS_ERROR, &errstr[0], 256);
	LoadString(hInst, IDS_TITLE1, &title[0], 256);
	MessageBox(0, errstr, title, MB_OK|MB_ICONHAND);

/*
clean_up_registry_keys:

   // We've constructed some, but not all of the global key state.
   // So we must remove any keys we created. The Defaults key must
   // be deleted first before the Global key can be deleted.

   if (hkPerUser) 
	   RegDeleteKey(HKEY_CURRENT_USER, &pathbuff[0]);

   goto clean_up_after_failure;
*/

memory_limited:

	LoadString(hInst, IDS_MEMORY_LIMITED, &errstr[0], 256);
	LoadString(hInst, IDS_TITLE1, &title[0], 256);
	MessageBox(0, errstr, title, MB_OK|MB_ICONHAND);
    goto clean_up_after_failure;


security_failure:

	LoadString(hInst, IDS_SECURITY_FAILURE, &errstr[0], 256);
	LoadString(hInst, IDS_TITLE1, &title[0], 256);
	MessageBox(0, errstr, title, MB_OK|MB_ICONHAND);


clean_up_after_failure:

   if (psidAdmins) 
	   FreeSid(psidAdmins);
   if (ptu) 
	   LocalFree(psidUser);

   return FALSE;
}
/****************************************************************************/
BOOL SaveUserSetting(void)
{  
//    HKEY hkGlobal;
    TCHAR pathbuff[50]=TEXT("Software\\Microsoft\\Osk");
    TCHAR errstr[256];
    TCHAR title[256];
   
    HKEY hkPerUser  = NULL;
    LONG lResult;
    DWORD dwDisposition;

    TOKEN_USER *ptu = NULL;

    PSID psidUser   = NULL;
    PSID psidAdmins = NULL;

    PACL  paclKey = NULL;

    SID_IDENTIFIER_AUTHORITY SystemSidAuthority= SECURITY_NT_AUTHORITY;

    SECURITY_ATTRIBUTES sa;
    SECURITY_DESCRIPTOR sdPermissions;

	

   // First we'll see whether this user has admin privileges...
//   if (RunningAsAdministrator())
//		MessageBox(0, TEXT("Running as administrator"), TEXT("Admin"), MB_OK);




   // First we'll setup the security attributes we're going to
   // use with the application's global key.

   sa.nLength              = sizeof(SECURITY_ATTRIBUTES);
   sa.bInheritHandle       = FALSE;
   sa.lpSecurityDescriptor = &sdPermissions;

	// ***  Do extra checking if we are in NT  ***
	if(platform == VER_PLATFORM_WIN32_NT)   
	{
   
		// Here we're creating a System Identifier (SID) to represent
		// the Admin group.

		if (!AllocateAndInitializeSid
			  (&SystemSidAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
				                       DOMAIN_ALIAS_RID_ADMINS,
					                   0, 0, 0, 0, 0, 0,
						               &psidAdmins))
        {
			goto security_failure;
        }

		// We also need a SID for the current user.

		if (!(ptu= GetCurrentUserInfo()) || !(psidUser= ptu->User.Sid)) 
			goto security_failure;

        if (!InitializeSecurityDescriptor(&sdPermissions, 
                                          SECURITY_DESCRIPTOR_REVISION1))
        {
			goto security_failure;
        }

		// We want the current user to own this key.

		if (!SetSecurityDescriptorOwner(&sdPermissions, psidUser, 0))
			goto security_failure;

		// Finally we must allocate and construct the discretionary
		// access control list (DACL) for the key.

		// Note that _alloca allocates memory on the stack frame
		// which will automatically be deallocated when this routine
		// exits.

		if (!(paclKey= (PACL) _alloca(ACL_BUFFER_SIZE)))
			goto memory_limited;

		if (!InitializeAcl(paclKey, ACL_BUFFER_SIZE, ACL_REVISION2))
			goto security_failure;

		// Our DACL will two access control entries (ACEs). The first ACE
		// provides full access to the current user. The second ACE gives
		// the Admin group full access. By default all other users will have
		// no access to the key.

		// The reason for admin access is to allow an administrator to
		// run special utilties to cleanup inconsistencies and disasters
		// in the per-user data area.

		if (!AddAccessAllowedAce(paclKey, ACL_REVISION2, KEY_ALL_ACCESS, 
                                 psidUser))
        {
            goto security_failure;
        }

        if (!AddAccessAllowedAce(paclKey, ACL_REVISION2, KEY_ALL_ACCESS, 
                                 psidAdmins))
        {
            goto security_failure;
        }

		// We must bind this DACL to the security descriptor...

		if (!SetSecurityDescriptorDacl(&sdPermissions, TRUE, paclKey, FALSE))
			goto security_failure;

	}	//end of extra checking for NT

   
   // Now we'll attempt to create the key with the security attributes...

   lResult= RegCreateKeyEx(HKEY_CURRENT_USER, &pathbuff[0], 0,
                           TEXT("Application Per-User Data"), 
                           REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS,
                           &sa, &hkPerUser, &dwDisposition
                          );

   if (lResult != ERROR_SUCCESS) goto registry_access_error;

   // Usually the disposition value will indicate that we've created a
   // new key. Sometimes it may instead state that we've opened an existing
   // key. This can happen when installation is incomplete and interrupted,
   // say by loss of electrical power.

    if (dwDisposition != REG_CREATED_NEW_KEY  &&
        dwDisposition != REG_OPENED_EXISTING_KEY) 
    {
        goto registry_access_error;
    }


	//Save the whole setting
	lstrcpy(pathbuff, TEXT("Setting"));
	lResult= RegSetValueEx(hkPerUser, &pathbuff[0], 0, REG_BINARY,
                           (LPBYTE) kbPref, sizeof(KBPREFINFO));

	if (lResult != ERROR_SUCCESS) 
		goto registry_access_error;

   RegCloseKey(hkPerUser);
   FreeSid(psidAdmins);
   LocalFree(ptu);

   free(kbPref);		//v-mjgran: Memory Leak fixed


   return(TRUE);

//Error handler
/*
registry_damage_error:

   // We'll display a warning that the registry info has
   // been damaged.

//   MPError(hwndFrame, MB_OK | MB_ICONHAND, IDS_MUTEX_LOGIC_ERR, NULL);
    LoadString(hInst, IDS_REGISTRY_DAMAGE, &errstr[0], 256);
	LoadString(hInst, IDS_TITLE1, &title[0], 256);
MessageBox(0, errstr, title, MB_OK|MB_ICONHAND);

   // Then we discard the REG_INSTALLED flag to insure that a reinstallation
   // can proceed.

   RegDeleteValue(hkGlobal, REG_INSTALLED);

   goto clean_up_registry_keys;
*/

registry_access_error:

//   MPError(hwndFrame, MB_OK | MB_ICONHAND, IDS_REG_ACCESS_ERROR, NULL);
    LoadString(hInst, IDS_REGISTRY_ACCESS_ERROR, &errstr[0], 256);
	LoadString(hInst, IDS_TITLE1, &title[0], 256);
MessageBox(0, errstr, title, MB_OK|MB_ICONHAND);

/*
clean_up_registry_keys:

   // We've constructed some, but not all of the global key state.
   // So we must remove any keys we created. The Defaults key must
   // be deleted first before the Global key can be deleted.

   if (hkPerUser) 
        RegDeleteKey(HKEY_CURRENT_USER, &pathbuff[0]);

//   if (hkDefaults) RegCloseKey(hkDefaults);

   goto clean_up_after_failure;
*/

memory_limited:

//   MPError(hwndFrame, MB_OK | MB_ICONHAND, IDS_MEMORY_LIMITED, NULL);
    LoadString(hInst, IDS_MEMORY_LIMITED, &errstr[0], 256);
	LoadString(hInst, IDS_TITLE1, &title[0], 256);
    MessageBox(0, errstr,title, MB_OK|MB_ICONHAND);
    goto clean_up_after_failure;

security_failure:

//   MPError(hwndFrame, MB_OK | MB_ICONHAND, IDS_SECURITY_FAIL_I, NULL);
    LoadString(hInst, IDS_SECURITY_FAILURE, &errstr[0], 256);
	LoadString(hInst, IDS_TITLE1, &title[0], 256);
    MessageBox(0, errstr, title, MB_OK|MB_ICONHAND);


clean_up_after_failure:

   if (psidAdmins) 
	   FreeSid(psidAdmins);
   if (ptu) 
	   LocalFree(psidUser);

   return FALSE;
}



/*****************************************************************************/

