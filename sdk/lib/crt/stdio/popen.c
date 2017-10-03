/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS C runtime library
 * FILE:            lib/sdk/crt/stdio/popen.c
 * PURPOSE:         Pipe Functions
 * PROGRAMMERS:     Eric Kohl
 *                  Hartmut Birr
 *                  Also adapted from Wine team code by Andreas Maier.
 */

#include <precomp.h>
#include <tchar.h>

#ifdef _UNICODE
#define sT "S"
#else
#define sT "s"
#endif

#define MK_STR(s) #s

int msvcrt_alloc_fd(HANDLE hand, int flag); //FIXME: Remove
unsigned split_oflags(unsigned oflags); //FIXME: Remove

#ifndef _UNICODE
struct popen_handle *popen_handles = NULL;
DWORD popen_handles_size = 0;

void msvcrt_free_popen_data(void)
{
    free(popen_handles);
}
#endif

/*
 * @implemented
 */
FILE *_tpopen (const _TCHAR *cm, const _TCHAR *md) /* program name, pipe mode */
{
    _TCHAR *szCmdLine=NULL;
    _TCHAR *szComSpec=NULL;
    _TCHAR *s;
    FILE *ret;
    HANDLE hReadPipe, hWritePipe;
    BOOL result;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
    struct popen_handle *container;
    DWORD i;

    TRACE(MK_STR(_tpopen)"('%"sT"', '%"sT"')\n", cm, md);

    if (cm == NULL)
        return NULL;

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

    memset(&ProcessInformation, 0, sizeof(ProcessInformation));
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

    _mlock(_POPEN_LOCK);
    for(i=0; i<popen_handles_size; i++)
    {
        if (!popen_handles[i].f)
            break;
    }
    if (i==popen_handles_size)
    {
        i = (popen_handles_size ? popen_handles_size*2 : 8);
        container = realloc(popen_handles, i*sizeof(*container));
        if (!container) goto error;

        popen_handles = container;
        container = popen_handles+popen_handles_size;
        memset(container, 0, (i-popen_handles_size)*sizeof(*container));
        popen_handles_size = i;
    }
    else container = popen_handles+i;

    if ( *md == 'r' )
    {
        ret = _tfdopen(msvcrt_alloc_fd(hReadPipe,  split_oflags(_fmode)) , _T("r"));
        CloseHandle(hWritePipe);
    }
    else
    {
        ret = _tfdopen( msvcrt_alloc_fd(hWritePipe, split_oflags(_fmode)) , _T("w"));
        CloseHandle(hReadPipe);
    }

    container->f = ret;
    container->proc = ProcessInformation.hProcess;
    _munlock(_POPEN_LOCK);

    return ret;

error:
    _munlock(_POPEN_LOCK);
    if (ProcessInformation.hProcess != 0)
        CloseHandle(ProcessInformation.hProcess);
    return NULL;
}

#ifndef _UNICODE

/*
 * @implemented
 */
int CDECL _pclose(FILE* file)
{
    HANDLE h;
    DWORD i;

    if (!MSVCRT_CHECK_PMT(file != NULL)) return -1;

    _mlock(_POPEN_LOCK);
    for(i=0; i<popen_handles_size; i++)
    {
        if (popen_handles[i].f == file)
            break;
    }
    if(i == popen_handles_size)
    {
        _munlock(_POPEN_LOCK);
        *_errno() = EBADF;
        return -1;
    }

    h = popen_handles[i].proc;
    popen_handles[i].f = NULL;
    _munlock(_POPEN_LOCK);

    fclose(file);
    if(WaitForSingleObject(h, INFINITE)==WAIT_FAILED || !GetExitCodeProcess(h, &i))
    {
        _dosmaperr(GetLastError());
        CloseHandle(h);
        return -1;
    }

    CloseHandle(h);
    return i;
}

#endif
