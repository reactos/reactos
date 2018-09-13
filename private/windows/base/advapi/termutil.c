#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "tsappcmp.h"
#include <hydra\regapi.h>

PTERMSRVCREATEREGENTRY gpfnTermsrvCreateRegEntry;

PTERMSRVOPENREGENTRY gpfnTermsrvOpenRegEntry;

PTERMSRVSETVALUEKEY gpfnTermsrvSetValueKey;

PTERMSRVDELETEKEY gpfnTermsrvDeleteKey;

PTERMSRVDELETEVALUE gpfnTermsrvDeleteValue;

PTERMSRVRESTOREKEY gpfnTermsrvRestoreKey;

PTERMSRVSETKEYSECURITY gpfnTermsrvSetKeySecurity;

PTERMSRVOPENUSERCLASSES gpfnTermsrvOpenUserClasses;

PTERMSRVGETPRESETVALUE gpfnTermsrvGetPreSetValue;

BOOL IsTerminalServerCompatible(VOID)
{

PIMAGE_NT_HEADERS NtHeader = RtlImageNtHeader( NtCurrentPeb()->ImageBaseAddress );

    if (NtHeader->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE) {
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL IsSystemLUID(VOID)
{
    HANDLE      TokenHandle;
    UCHAR       TokenInformation[ sizeof( TOKEN_STATISTICS ) ];
    ULONG       ReturnLength;
    LUID        CurrentLUID = { 0, 0 };
    LUID        SystemLUID = SYSTEM_LUID;
    NTSTATUS Status;

    if ( CurrentLUID.LowPart == 0 && CurrentLUID.HighPart == 0 ) {

        Status = NtOpenProcessToken( NtCurrentProcess(),
                                     TOKEN_QUERY,
                                     &TokenHandle );
        if ( !NT_SUCCESS( Status ) )
            return(TRUE);

        NtQueryInformationToken( TokenHandle, TokenStatistics, &TokenInformation,
                                 sizeof(TokenInformation), &ReturnLength );
        NtClose( TokenHandle );

        RtlCopyLuid(&CurrentLUID,
                    &(((PTOKEN_STATISTICS)TokenInformation)->AuthenticationId));
    }

    if (RtlEqualLuid(&CurrentLUID, &SystemLUID)) {
        return(TRUE);
    } else {
        return(FALSE );
    }
}

BOOL IsTSAppCompatEnabled(VOID)
{

   NTSTATUS NtStatus;
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING UniString;
   HKEY   hKey = 0;
   ULONG  ul, ulcbuf;
   PKEY_VALUE_PARTIAL_INFORMATION pKeyValInfo = NULL;

   BOOL retval = TRUE;


   RtlInitUnicodeString(&UniString,REG_NTAPI_CONTROL_TSERVER);



   // Determine the value info buffer size
   ulcbuf = sizeof(KEY_VALUE_FULL_INFORMATION) + MAX_PATH*sizeof(WCHAR) + 
            sizeof(ULONG);

   pKeyValInfo = RtlAllocateHeap(RtlProcessHeap(), 
                                 0, 
                                 ulcbuf);

   // Did everything initialize OK?
   if (UniString.Buffer && pKeyValInfo) {

       InitializeObjectAttributes(&ObjectAttributes,
                                  &UniString,
                                  OBJ_CASE_INSENSITIVE,
                                  NULL,
                                  NULL
                                 );
   
       NtStatus = NtOpenKey(&hKey, KEY_READ, &ObjectAttributes);
   
       if (NT_SUCCESS(NtStatus)) {
   
           RtlInitUnicodeString(&UniString, 
                               L"TSAppCompat");
           NtStatus = NtQueryValueKey(hKey,
                                      &UniString,
                                      KeyValuePartialInformation,
                                      pKeyValInfo,
                                      ulcbuf,
                                      &ul);

           if (NT_SUCCESS(NtStatus) && (REG_DWORD == pKeyValInfo->Type)) {

               if ((*(PULONG)pKeyValInfo->Data) == 0) {
                  retval = FALSE;
               }

           }

           NtClose(hKey);
       }
   }

   // Free up the buffers we allocated
   // Need to zero out the buffers, because some apps (MS Internet Assistant)
   // won't install if the heap is not zero filled.
   if (pKeyValInfo) {
       memset(pKeyValInfo, 0, ulcbuf);
       RtlFreeHeap( RtlProcessHeap(), 0, pKeyValInfo );
   }

   return(retval);

}


ULONG GetCompatFlags()
{
    ULONG    ulAppFlags = 0;
    PRTL_USER_PROCESS_PARAMETERS pUserParam;
    PWCHAR  pwch, pwchext;
    WCHAR   pwcAppName[MAX_PATH+1];
    NTSTATUS NtStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UniString;
    HKEY   hKey = 0;
    ULONG  ul, ulcbuf;
    PKEY_VALUE_PARTIAL_INFORMATION pKeyValInfo = NULL;
    LPWSTR UniBuff = NULL;

    ULONG dwCompatFlags = 0;
    UniString.Buffer = NULL;



    // Get the path of the executable name
    pUserParam = NtCurrentPeb()->ProcessParameters;

    // Get the executable name, if there's no \ just use the name as it is
    pwch = wcsrchr(pUserParam->ImagePathName.Buffer, L'\\');
    if (pwch) {
        pwch++;
    } else {
        pwch = pUserParam->ImagePathName.Buffer;
    }
    wcscpy(pwcAppName, pwch);
    pwch = pwcAppName;

    // Remove the extension
    if (pwchext = wcsrchr(pwch, L'.')) {
        *pwchext = '\0';
    }


    UniString.Buffer = NULL;


    ul = sizeof(TERMSRV_COMPAT_APP) + (wcslen(pwch) + 1)*sizeof(WCHAR);

    UniBuff = RtlAllocateHeap(RtlProcessHeap(), 
                              0, 
                              ul);

    if (UniBuff) {
        wcscpy(UniBuff, TERMSRV_COMPAT_APP);
        wcscat(UniBuff, pwch);
    
        RtlInitUnicodeString(&UniString, UniBuff);
    }

    // Determine the value info buffer size
    ulcbuf = sizeof(KEY_VALUE_FULL_INFORMATION) + MAX_PATH*sizeof(WCHAR) + 
             sizeof(ULONG);

    pKeyValInfo = RtlAllocateHeap(RtlProcessHeap(), 
                                  0, 
                                  ulcbuf);

    // Did everything initialize OK?
    if (UniString.Buffer && pKeyValInfo) {

        InitializeObjectAttributes(&ObjectAttributes,
                                   &UniString,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL
                                  );
    
        NtStatus = NtOpenKey(&hKey, KEY_READ, &ObjectAttributes);
    
        if (NT_SUCCESS(NtStatus)) {
    
            RtlInitUnicodeString(&UniString, 
                                COMPAT_FLAGS);
            NtStatus = NtQueryValueKey(hKey,
                                       &UniString,
                                       KeyValuePartialInformation,
                                       pKeyValInfo,
                                       ulcbuf,
                                       &ul);

            if (NT_SUCCESS(NtStatus) && (REG_DWORD == pKeyValInfo->Type)) {


                dwCompatFlags = *(PULONG)pKeyValInfo->Data; 

            }

            NtClose(hKey);
        }
    }

    // Free up the buffers we allocated
    // Need to zero out the buffers, because some apps (MS Internet Assistant)
    // won't install if the heap is not zero filled.
    if (UniBuff) {
        memset(UniBuff, 0, UniString.MaximumLength);
        RtlFreeHeap( RtlProcessHeap(), 0, UniBuff );
    }
    if (pKeyValInfo) {
        memset(pKeyValInfo, 0, ulcbuf);
        RtlFreeHeap( RtlProcessHeap(), 0, pKeyValInfo );
    }

    return(dwCompatFlags);

}

void InitializeTermsrvFpns(void)
{

    HANDLE          dllHandle;
    ULONG           dwCompatFlags;


    if (IsTerminalServerCompatible() || (!IsTSAppCompatEnabled())) {
        return;
    }

    dwCompatFlags = GetCompatFlags();


    //Don't load app compatibility dll for system components

    if (IsSystemLUID()) {

        if ( (dwCompatFlags & (TERMSRV_COMPAT_SYSREGMAP | TERMSRV_COMPAT_WIN32))
                     != (TERMSRV_COMPAT_SYSREGMAP | TERMSRV_COMPAT_WIN32) ) {

            //
            // Process is running as SYSTEM and we don't have an app
            // compatibility flag telling us to do the regmap stuff.
            //
            
            return;

        }
    
    } else if ( (dwCompatFlags & (TERMSRV_COMPAT_NOREGMAP | TERMSRV_COMPAT_WIN32))
                     == (TERMSRV_COMPAT_NOREGMAP | TERMSRV_COMPAT_WIN32) ) {
        //    
        // We don't want to do registry mapping for this user process
        //
        return;

    }

    //
    // Load Terminal Server application compatibility dll
    //
    dllHandle = LoadLibrary("tsappcmp.dll");
    
    if (dllHandle) {

        gpfnTermsrvCreateRegEntry = 
            (PTERMSRVCREATEREGENTRY)GetProcAddress(dllHandle,"TermsrvCreateRegEntry"); 

        gpfnTermsrvOpenRegEntry = 
            (PTERMSRVOPENREGENTRY)GetProcAddress(dllHandle,"TermsrvOpenRegEntry"); 

        gpfnTermsrvSetValueKey = 
            (PTERMSRVSETVALUEKEY)GetProcAddress(dllHandle,"TermsrvSetValueKey"); 

        gpfnTermsrvDeleteKey = 
            (PTERMSRVDELETEKEY)GetProcAddress(dllHandle,"TermsrvDeleteKey"); 

        gpfnTermsrvDeleteValue = 
            (PTERMSRVDELETEVALUE)GetProcAddress(dllHandle,"TermsrvDeleteValue"); 

         gpfnTermsrvRestoreKey = 
            (PTERMSRVRESTOREKEY)GetProcAddress(dllHandle,"TermsrvRestoreKey"); 

        gpfnTermsrvSetKeySecurity = 
            (PTERMSRVSETKEYSECURITY)GetProcAddress(dllHandle,"TermsrvSetKeySecurity"); 

        gpfnTermsrvOpenUserClasses = 
            (PTERMSRVOPENUSERCLASSES)GetProcAddress(dllHandle,"TermsrvOpenUserClasses"); 

        gpfnTermsrvGetPreSetValue = 
            (PTERMSRVGETPRESETVALUE)GetProcAddress(dllHandle,"TermsrvGetPreSetValue"); 
        
        
    }    
}


