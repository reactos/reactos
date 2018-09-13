#include <windows.h>
#include <stdio.h>
#include <io.h>


/*
    Usage:

    sublocal -winini <section> <key> <value>

    IE,
        sublocal -winini fonts "MS Sans Serif (8,10,12,18,24 (VGA res)" sserifeg.fon

    sublocal -userdef


    Return codes:

        0 - success
        1 - invalid command line args
        2 - failure (-winini case)
        3 - failure (-userdef case)
*/



typedef enum {
    ResultSuccess,
    ResultInvalidArgs,
    ResultWinIniFailure,
    ResultUserDefFailure,
    ResultTurkishKeyboardFailure
} RESULTCODE;



BOOL
EnablePrivilege(
    IN PTSTR PrivilegeName
    )
{
    HANDLE Token;
    BOOL b;
    TOKEN_PRIVILEGES NewPrivileges;
    LUID Luid;

    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES,&Token)) {
        return(FALSE);
    }

    if(!LookupPrivilegeValue(NULL,PrivilegeName,&Luid)) {
        CloseHandle(Token);
        return(FALSE);
    }

    NewPrivileges.PrivilegeCount = 1;
    NewPrivileges.Privileges[0].Luid = Luid;
    NewPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    b = AdjustTokenPrivileges(
            Token,
            FALSE,
            &NewPrivileges,
            0,
            NULL,
            NULL
            );

    CloseHandle(Token);

    return(b);
}


RESULTCODE
DoNewUserDef(
    VOID
    )
{
    TCHAR UserDefName[MAX_PATH];
    TCHAR TempName[MAX_PATH];
    BOOL Renamed;
    ULONG u;

    //
    // Form the full pathname of the userdef hive
    // and a temporary filename.
    //
    GetSystemDirectory(UserDefName,MAX_PATH);
    lstrcat(UserDefName,TEXT("\\CONFIG"));
    GetTempFileName(UserDefName,TEXT("HIVE"),0,TempName);
    lstrcat(UserDefName,TEXT("\\USERDEF"));

    //
    // Rename the existing userdef to the temp file name.
    // If this fails, assume there is no existing userdef hive (!!!)
    // and continue.
    //
    // Note that GetTempFileName creates the temp file and leaves it
    // there so we have to use MoveFileEx to replace it.
    //
    Renamed = MoveFileEx(UserDefName,TempName,MOVEFILE_REPLACE_EXISTING);

    //
    // Save the current user to userdef.
    //
    EnablePrivilege(SE_BACKUP_NAME);
    u = RegSaveKey(HKEY_CURRENT_USER,UserDefName,NULL);
    if(u != NO_ERROR) {
        //
        // Save failed. Clean up and return.
        //
        DeleteFile(UserDefName);
        if(Renamed) {
            MoveFile(TempName,UserDefName);
        }
        return(ResultUserDefFailure);
    }

    //
    // Save worked.  Get rid of the original userdef.
    //
    SetFileAttributes(TempName,FILE_ATTRIBUTE_NORMAL);
    DeleteFile(TempName);

    return(ResultSuccess);
}


static TCHAR    szKey[] = TEXT("System\\CurrentControlSet\\Control\\Keyboard Layout\\DosKeybIDs");

RESULTCODE
PatchTurkishDosKeybIDs(
    VOID
    )
{
    DWORD   dwSize, dwType;
    int     rc;
    HANDLE  hkey;
    TCHAR   szX0000041F[40];
    TCHAR   szX0001041F[40];

    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0L, KEY_WRITE, (PHKEY)&hkey)) {

        // dwSize = sizeof(szX0000041F);
        // rc = RegQueryValueEx (hkey, TEXT("0000041F"), NULL, &dwType,
        //                     (LPBYTE)szX0000041F, &dwSize);
	    rc = RegSetValueEx (hkey, TEXT("0000041F"), 0L, REG_SZ,
		       (LPBYTE)TEXT("179"), (lstrlen(TEXT("179")) + 1)*sizeof(TCHAR));
        if (rc != 0) {
	        RegCloseKey (hkey);
	        printf("Error = %d\n", GetLastError());
            return ResultTurkishKeyboardFailure;
        }

        // dwSize = sizeof(szX0001041F);
        // rc = RegQueryValueEx (hkey, TEXT("0001041F"), NULL, &dwType,
        //                      (LPBYTE)szX0001041F, &dwSize);
	    rc = RegSetValueEx (hkey, TEXT("0001041F"), 0L, REG_SZ,
		       (LPBYTE)TEXT("440"), (lstrlen(TEXT("440")) + 1)*sizeof(TCHAR));

        RegCloseKey (hkey);

        if (rc != 0) {
	        printf("Error = %d\n", GetLastError());
            return ResultTurkishKeyboardFailure;
        }

	    // printf("0000041F=%s, 0001041F=%s\n", szX0000041F, szX0001041F);

    }

    else {
	    printf("Error = %d\n", GetLastError());
	    return ResultTurkishKeyboardFailure;
    }

    return rc;
}

int
tmain(
    IN int   argc,
    IN PTSTR argv[]
    )
{
    RESULTCODE result;

    //
    // Check arguments.
    //
    if((argc < 2)
    || ((argv[1][0] != TEXT('-')) && (argv[1][0] != TEXT('/')))) {

        return(ResultInvalidArgs);
    }

    //
    // See whether we are supposed to set a win.ini value.
    //
    if(!lstrcmpi(argv[1]+1,TEXT("winini"))) {

        result = (argc == 5)
               ? (WriteProfileString(argv[2],argv[3],argv[4]) ? ResultSuccess
                                                              : ResultWinIniFailure)
               : ResultInvalidArgs;

    //
    // See whether we are supposed to create a new userdef hive.
    //
    }
    else if(!lstrcmpi(argv[1]+1,TEXT("userdef"))) {

	result = DoNewUserDef();

    }
    else if(!lstrcmpi(argv[1]+1,TEXT("PatchTurkishKeyb"))) {

	result = PatchTurkishDosKeybIDs();

    }
    else {

	//
	// Unknown operation.
	//
	result = ResultInvalidArgs;
    }

    return(result);
}



int
__cdecl
main(
    IN int   argc,
    IN char *argv[]
    )
{
    return(tmain(argc,argv));
}
