/*
* COPYRIGHT:   See COPYING in the top level directory
* PROJECT:     ReactOS system libraries
* FILE:        lib/crt/??????
* PURPOSE:     Unknown
* PROGRAMER:   Unknown
* UPDATE HISTORY:
*              25/11/05: Created
*/

#include <precomp.h>
#include <tchar.h>

#ifdef _UNICODE
#define sT "S"
#else
#define sT "s"
#endif

#define MK_STR(s) #s

int alloc_fd(HANDLE hand, int flag); //FIXME: Remove
unsigned split_oflags(unsigned oflags); //FIXME: Remove

/*
 * @implemented
 */
FILE *_tpopen (const _TCHAR *cm, const _TCHAR *md) /* program name, pipe mode */
{
    _TCHAR *szCmdLine=NULL;
    _TCHAR *szComSpec=NULL;
    _TCHAR *s;
    FILE *pf;
    HANDLE hReadPipe, hWritePipe;
    BOOL result;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

    TRACE(MK_STR(_tpopen)"('%"sT"', '%"sT"')\n", cm, md);

    if (cm == NULL)
        return( NULL );

    szComSpec = _tgetenv(_T("COMSPEC"));
    if (szComSpec == NULL)
    {
        szComSpec = _T("cmd.exe");
    }

    s = max(_tcsrchr(szComSpec, '\\'), _tcsrchr(szComSpec, '/'));
    if (s == NULL)
        s = szComSpec;
    else
        s++;

    szCmdLine = malloc((_tcslen(s) + 4 + _tcslen(cm) + 1) * sizeof(_TCHAR));
    if (szCmdLine == NULL)
    {
        return NULL;
    }

    _tcscpy(szCmdLine, s);
    s = _tcsrchr(szCmdLine, '.');
    if (s)
        *s = 0;
    _tcscat(szCmdLine, _T(" /C "));
    _tcscat(szCmdLine, cm);

    if ( !CreatePipe(&hReadPipe,&hWritePipe,&sa,1024))
    {
        free (szCmdLine);
        return NULL;
    }

    memset(&StartupInfo, 0, sizeof(STARTUPINFO));
    StartupInfo.cb = sizeof(STARTUPINFO);

    if (*md == 'r' ) {
        StartupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        StartupInfo.hStdOutput = hWritePipe;
        StartupInfo.dwFlags |= STARTF_USESTDHANDLES;
    }
    else if ( *md == 'w' ) {
        StartupInfo.hStdInput = hReadPipe;
        StartupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        StartupInfo.dwFlags |= STARTF_USESTDHANDLES;
    }

    if (StartupInfo.dwFlags & STARTF_USESTDHANDLES)
        StartupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    result = CreateProcess(szComSpec,
        szCmdLine,
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        NULL,
        &StartupInfo,
        &ProcessInformation);
    free (szCmdLine);

    if (result == FALSE)
    {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return NULL;
    }

    CloseHandle(ProcessInformation.hThread);

    if ( *md == 'r' )
    {
        pf = _tfdopen(alloc_fd(hReadPipe,  split_oflags(_fmode)) , _T("r"));
        CloseHandle(hWritePipe);
    }
    else
    {
        pf = _tfdopen( alloc_fd(hWritePipe, split_oflags(_fmode)) , _T("w"));
        CloseHandle(hReadPipe);
    }

    return( pf );
}

#ifndef _UNICODE

/*
 * @implemented
 */
int _pclose (FILE *pp)
{
    TRACE("_pclose(%x)",pp);

    fclose(pp);
    //if (!TerminateProcess(pp->_tmpfname ,0))
    //  return( -1 );
    return( 0 );
}

#endif


