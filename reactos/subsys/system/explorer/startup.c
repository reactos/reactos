/*
 * Copyright (C) 2002 Andreas Mohr
 * Copyright (C) 2002 Shachar Shemesh
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
/* Based on the Wine "bootup" handler application
 *
 * This app handles the various "hooks" windows allows for applications to perform
 * as part of the bootstrap process. Theses are roughly devided into three types.
 * Knowledge base articles that explain this are 137367, 179365, 232487 and 232509.
 * Also, 119941 has some info on grpconv.exe
 * The operations performed are (by order of execution):
 *
 * Preboot (prior to fully loading the Windows kernel):
 * - wininit.exe (rename operations left in wininit.ini - Win 9x only)
 * - PendingRenameOperations (rename operations left in the registry - Win NT+ only)
 *
 * Startup (before the user logs in)
 * - Services (NT, ?semi-synchronous?, not implemented yet)
 * - HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\RunServicesOnce (9x, asynch)
 * - HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\RunServices (9x, asynch)
 * 
 * After log in
 * - HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\RunOnce (all, synch)
 * - HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\Run (all, asynch)
 * - HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run (all, asynch)
 * - Startup folders (all, ?asynch?, no imp)
 * - HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\RunOnce (all, asynch)
 *   
 * Somewhere in there is processing the RunOnceEx entries (also no imp)
 * 
 * Bugs:
 * - If a pending rename registry does not start with \??\ the entry is
 *   processed anyways. I'm not sure that is the Windows behaviour.
 * - Need to check what is the windows behaviour when trying to delete files
 *   and directories that are read-only
 * - In the pending rename registry processing - there are no traces of the files
 *   processed (requires translations from Unicode to Ansi).
 */

#include <stdio.h>
#include <windows.h>

#define MAX_LINE_LENGTH (2*MAX_PATH+2)

static BOOL GetLine( HANDLE hFile, char *buf, size_t buflen )
{
    size_t i=0;
    buf[0]='\0';

    do
    {
        DWORD read;
        if( !ReadFile( hFile, buf, 1, &read, NULL ) || read!=1 )
        {
            return FALSE;
        }

    } while( isspace( *buf ) );

    while( buf[i]!='\n' && i<=buflen &&
            ReadFile( hFile, buf+i+1, 1, NULL, NULL ) )
    {
        ++i;
    }


    if( buf[i]!='\n' )
    {
	return FALSE;
    }

    if( i>0 && buf[i-1]=='\r' )
        --i;

    buf[i]='\0';

    return TRUE;
}

/* Performs the rename operations dictated in %SystemRoot%\Wininit.ini.
 * Returns FALSE if there was an error, or otherwise if all is ok.
 */
static BOOL wininit()
{
    return TRUE;
}

static BOOL pendingRename()
{
    static const WCHAR ValueName[] = {'P','e','n','d','i','n','g',
                                      'F','i','l','e','R','e','n','a','m','e',
                                      'O','p','e','r','a','t','i','o','n','s',0};
    static const WCHAR SessionW[] = { 'S','y','s','t','e','m','\\',
                                     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                     'C','o','n','t','r','o','l','\\',
                                     'S','e','s','s','i','o','n',' ','M','a','n','a','g','e','r',0};
    WCHAR *buffer=NULL;
    const WCHAR *src=NULL, *dst=NULL;
    DWORD dataLength=0;
    HKEY hSession=NULL;
    DWORD res;

    printf("Entered\n");

    if( (res=RegOpenKeyExW( HKEY_LOCAL_MACHINE, SessionW, 0, KEY_ALL_ACCESS, &hSession ))
            !=ERROR_SUCCESS )
    {
        if( res==ERROR_FILE_NOT_FOUND )
        {
            printf("The key was not found - skipping\n");
            res=TRUE;
        }
        else
        {
            printf("Couldn't open key, error %ld\n", res );
            res=FALSE;
        }

        goto end;
    }

    res=RegQueryValueExW( hSession, ValueName, NULL, NULL /* The value type does not really interest us, as it is not
                                                             truely a REG_MULTI_SZ anyways */,
            NULL, &dataLength );
    if( res==ERROR_FILE_NOT_FOUND )
    {
        /* No value - nothing to do. Great! */
        printf("Value not present - nothing to rename\n");
        res=TRUE;
        goto end;
    }

    if( res!=ERROR_SUCCESS )
    {
        printf("Couldn't query value's length (%ld)\n", res );
        res=FALSE;
        goto end;
    }

    buffer=malloc( dataLength );
    if( buffer==NULL )
    {
        printf("Couldn't allocate %lu bytes for the value\n", dataLength );
        res=FALSE;
        goto end;
    }

    res=RegQueryValueExW( hSession, ValueName, NULL, NULL, (LPBYTE)buffer, &dataLength );
    if( res!=ERROR_SUCCESS )
    {
        printf("Couldn't query value after successfully querying before (%lu),\n"
                "please report to wine-devel@winehq.org\n", res);
        res=FALSE;
        goto end;
    }

    /* Make sure that the data is long enough and ends with two NULLs. This
     * simplifies the code later on.
     */
    if( dataLength<2*sizeof(buffer[0]) ||
            buffer[dataLength/sizeof(buffer[0])-1]!='\0' ||
            buffer[dataLength/sizeof(buffer[0])-2]!='\0' )
    {
        printf("Improper value format - doesn't end with NULL\n");
        res=FALSE;
        goto end;
    }

    for( src=buffer; (src-buffer)*sizeof(src[0])<dataLength && *src!='\0';
            src=dst+lstrlenW(dst)+1 )
    {
        DWORD dwFlags=0;

        printf("processing next command\n");

        dst=src+lstrlenW(src)+1;

        /* We need to skip the \??\ header */
        if( src[0]=='\\' && src[1]=='?' && src[2]=='?' && src[3]=='\\' )
            src+=4;

        if( dst[0]=='!' )
        {
            dwFlags|=MOVEFILE_REPLACE_EXISTING;
            dst++;
        }

        if( dst[0]=='\\' && dst[1]=='?' && dst[2]=='?' && dst[3]=='\\' )
            dst+=4;

        if( *dst!='\0' )
        {
            /* Rename the file */
            MoveFileExW( src, dst, dwFlags );
        } else
        {
            /* Delete the file or directory */
            if( (res=GetFileAttributesW(src))!=INVALID_FILE_ATTRIBUTES )
            {
                if( (res&FILE_ATTRIBUTE_DIRECTORY)==0 )
                {
                    /* It's a file */
                    DeleteFileW(src);
                } else
                {
                    /* It's a directory */
                    RemoveDirectoryW(src);
                }
            } else
            {
                printf("couldn't get file attributes (%ld)\n", GetLastError() );
            }
        }
    }

    if((res=RegDeleteValueW(hSession, ValueName))!=ERROR_SUCCESS )
    {
        printf("Error deleting the value (%lu)\n", GetLastError() );
        res=FALSE;
    } else
        res=TRUE;
    
end:
    if( buffer!=NULL )
        free(buffer);

    if( hSession!=NULL )
        RegCloseKey( hSession );

    return res;
}

enum runkeys {
    RUNKEY_RUN, RUNKEY_RUNONCE, RUNKEY_RUNSERVICES, RUNKEY_RUNSERVICESONCE
};

const WCHAR runkeys_names[][30]=
{
    {'R','u','n',0},
    {'R','u','n','O','n','c','e',0},
    {'R','u','n','S','e','r','v','i','c','e','s',0},
    {'R','u','n','S','e','r','v','i','c','e','s','O','n','c','e',0}
};

#define INVALID_RUNCMD_RETURN -1
/*
 * This function runs the specified command in the specified dir.
 * [in,out] cmdline - the command line to run. The function may change the passed buffer.
 * [in] dir - the dir to run the command in. If it is NULL, then the current dir is used.
 * [in] wait - whether to wait for the run program to finish before returning.
 * [in] minimized - Whether to ask the program to run minimized.
 *
 * Returns:
 * If running the process failed, returns INVALID_RUNCMD_RETURN. Use GetLastError to get the error code.
 * If wait is FALSE - returns 0 if successful.
 * If wait is TRUE - returns the program's return value.
 */
static DWORD runCmd(LPWSTR cmdline, LPCWSTR dir, BOOL wait, BOOL minimized)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION info;
    DWORD exit_code=0;

    memset(&si, 0, sizeof(si));
    si.cb=sizeof(si);
    if( minimized )
    {
        si.dwFlags=STARTF_USESHOWWINDOW;
        si.wShowWindow=SW_MINIMIZE;
    }
    memset(&info, 0, sizeof(info));

    if( !CreateProcessW(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, dir, &si, &info) )
    {
        printf("Failed to run command (%ld)\n", GetLastError() );

        return INVALID_RUNCMD_RETURN;
    }

    printf("Successfully ran command\n"); //%s - Created process handle %p\n",
               //wine_dbgstr_w(cmdline), info.hProcess );

    if(wait)
    {   /* wait for the process to exit */
        WaitForSingleObject(info.hProcess, INFINITE);
        GetExitCodeProcess(info.hProcess, &exit_code);
    }

    CloseHandle( info.hProcess );

    return exit_code;
}

/*
 * Process a "Run" type registry key.
 * hkRoot is the HKEY from which "Software\Microsoft\Windows\CurrentVersion" is
 *      opened.
 * szKeyName is the key holding the actual entries.
 * bDelete tells whether we should delete each value right before executing it.
 * bSynchronous tells whether we should wait for the prog to complete before
 *      going on to the next prog.
 */
static BOOL ProcessRunKeys( HKEY hkRoot, LPCWSTR szKeyName, BOOL bDelete,
        BOOL bSynchronous )
{
    static const WCHAR WINKEY_NAME[]={'S','o','f','t','w','a','r','e','\\',
        'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\',
        'C','u','r','r','e','n','t','V','e','r','s','i','o','n',0};
    HKEY hkWin=NULL, hkRun=NULL;
    DWORD res=ERROR_SUCCESS;
    DWORD i, nMaxCmdLine=0, nMaxValue=0;
    WCHAR *szCmdLine=NULL;
    WCHAR *szValue=NULL;

    if (hkRoot==HKEY_LOCAL_MACHINE)
        wprintf(L"processing %s entries under HKLM\n", szKeyName);
    else
        wprintf(L"processing %s entries under HKCU\n", szKeyName);

    if( (res=RegOpenKeyExW( hkRoot, WINKEY_NAME, 0, KEY_READ, &hkWin ))!=ERROR_SUCCESS )
    {
        printf("RegOpenKey failed on Software\\Microsoft\\Windows\\CurrentVersion (%ld)\n",
                res);

        goto end;
    }

    if( (res=RegOpenKeyExW( hkWin, szKeyName, 0, bDelete?KEY_ALL_ACCESS:KEY_READ, &hkRun ))!=
            ERROR_SUCCESS)
    {
        if( res==ERROR_FILE_NOT_FOUND )
        {
            printf("Key doesn't exist - nothing to be done\n");

            res=ERROR_SUCCESS;
        }
        else
            printf("RegOpenKey failed on run key (%ld)\n", res);

        goto end;
    }
    
    if( (res=RegQueryInfoKeyW( hkRun, NULL, NULL, NULL, NULL, NULL, NULL, &i, &nMaxValue,
                    &nMaxCmdLine, NULL, NULL ))!=ERROR_SUCCESS )
    {
        printf("Couldn't query key info (%ld)\n", res );

        goto end;
    }

    if( i==0 )
    {
        printf("No commands to execute.\n");

        res=ERROR_SUCCESS;
        goto end;
    }
    
    if( (szCmdLine=malloc(nMaxCmdLine))==NULL )
    {
        printf("Couldn't allocate memory for the commands to be executed\n");

        res=ERROR_NOT_ENOUGH_MEMORY;
        goto end;
    }

    if( (szValue=malloc((++nMaxValue)*sizeof(*szValue)))==NULL )
    {
        printf("Couldn't allocate memory for the value names\n");

        res=ERROR_NOT_ENOUGH_MEMORY;
        goto end;
    }
    
    while( i>0 )
    {
        DWORD nValLength=nMaxValue, nDataLength=nMaxCmdLine;
        DWORD type;

        --i;

        if( (res=RegEnumValueW( hkRun, i, szValue, &nValLength, 0, &type,
                        (LPBYTE)szCmdLine, &nDataLength ))!=ERROR_SUCCESS )
        {
            printf("Couldn't read in value %ld - %ld\n", i, res );

            continue;
        }

        if( bDelete && (res=RegDeleteValueW( hkRun, szValue ))!=ERROR_SUCCESS )
        {
            printf("Couldn't delete value - %ld, %ld. Running command anyways.\n", i, res );
        }
        
        if( type!=REG_SZ )
        {
            printf("Incorrect type of value #%ld (%ld)\n", i, type );

            continue;
        }

        if( (res=runCmd(szCmdLine, NULL, bSynchronous, FALSE ))==INVALID_RUNCMD_RETURN )
        {
            printf("Error running cmd #%ld (%ld)\n", i, GetLastError() );
        }

        printf("Done processing cmd #%ld\n", i);
    }

    res=ERROR_SUCCESS;

end:
    if( hkRun!=NULL )
        RegCloseKey( hkRun );
    if( hkWin!=NULL )
        RegCloseKey( hkWin );

    printf("done\n");

    return res==ERROR_SUCCESS?TRUE:FALSE;
}

struct op_mask {
    BOOL w9xonly; /* Perform only operations done on Windows 9x */
    BOOL ntonly; /* Perform only operations done on Windows NT */
    BOOL startup; /* Perform the operations that are performed every boot */
    BOOL preboot; /* Perform file renames typically done before the system starts */
    BOOL prelogin; /* Perform the operations typically done before the user logs in */
    BOOL postlogin; /* Operations done after login */
};

static const struct op_mask SESSION_START={FALSE, FALSE, TRUE, TRUE, TRUE, TRUE},
    SETUP={FALSE, FALSE, FALSE, TRUE, TRUE, TRUE};
#define DEFAULT SESSION_START

int startup( int argc, char *argv[] )
{
    struct op_mask ops; /* Which of the ops do we want to perform? */
    /* First, set the current directory to SystemRoot */
    TCHAR gen_path[MAX_PATH];
    DWORD res;

    res=GetWindowsDirectory( gen_path, sizeof(gen_path) );
    
    if( res==0 )
    {
	printf("Couldn't get the windows directory - error %ld\n",
		GetLastError() );

	return 100;
    }

    if( res>=sizeof(gen_path) )
    {
	printf("Windows path too long (%ld)\n", res );

	return 100;
    }

    if( !SetCurrentDirectory( gen_path ) )
    {
        printf("Cannot set the dir to %s (%ld)\n", gen_path, GetLastError() );

        return 100;
    }

    if( argc>1 )
    {
        switch( argv[1][0] )
        {
        case 'r': /* Restart */
            ops=SETUP;
            break;
        case 's': /* Full start */
            ops=SESSION_START;
            break;
        default:
            ops=DEFAULT;
            break;
        }
    } else
        ops=DEFAULT;

    /* Perform the ops by order, stopping if one fails, skipping if necessary */
    /* Shachar: Sorry for the perl syntax */
    res=(ops.ntonly || !ops.preboot || wininit())&&
        (ops.w9xonly || !ops.preboot || pendingRename()) &&
        (ops.ntonly || !ops.prelogin ||
         ProcessRunKeys( HKEY_LOCAL_MACHINE, runkeys_names[RUNKEY_RUNSERVICESONCE],
                TRUE, FALSE )) &&
        (ops.ntonly || !ops.prelogin || !ops.startup ||
         ProcessRunKeys( HKEY_LOCAL_MACHINE, runkeys_names[RUNKEY_RUNSERVICES],
                FALSE, FALSE )) &&
        (!ops.postlogin ||
         ProcessRunKeys( HKEY_LOCAL_MACHINE, runkeys_names[RUNKEY_RUNONCE],
                TRUE, TRUE )) &&
        (!ops.postlogin || !ops.startup ||
         ProcessRunKeys( HKEY_LOCAL_MACHINE, runkeys_names[RUNKEY_RUN],
                FALSE, FALSE )) &&
        (!ops.postlogin || !ops.startup ||
         ProcessRunKeys( HKEY_CURRENT_USER, runkeys_names[RUNKEY_RUN],
                FALSE, FALSE ));

    printf("Operation done\n");

    return res?0:101;
}
