/* Written by Krzysztof Kowalczyk (http://blog.kowalczyk.info)
   The author disclaims copyright to this source code. */

#include "base_util.h"

#include "WinUtil.hpp"

#include "str_util.h"

// TODO: exe name might be unicode so to support everything cmd or args
// should be unicode or we can assume that cmd and args are utf8 and
// convert them to utf16 and call CreateProcessW
#define DONT_INHERIT_HANDLES FALSE

// Given name of the command to exececute 'cmd', and its arguments 'args'
// return WinProcess object that makes it easier to handle the process
// Returns NULL if failed to create the process. Caller can use GetLastError()
// for detailed error information.
WinProcess * WinProcess::Create(const char* cmd, char* args)
{
    UINT                res;
    HANDLE              stdOut = INVALID_HANDLE_VALUE;
    HANDLE              stdErr = INVALID_HANDLE_VALUE;
    STARTUPINFOA        siStartupInfo;
    PROCESS_INFORMATION piProcessInfo;
    SECURITY_ATTRIBUTES sa;

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = 0;
    sa.bInheritHandle = 1;

    memzero(&siStartupInfo, sizeof(siStartupInfo));
    memzero(&piProcessInfo, sizeof(piProcessInfo));
    siStartupInfo.cb = sizeof(siStartupInfo);

    char stdoutTempName[MAX_PATH] = {0};
    char stderrTempName[MAX_PATH] = {0};
    char *stdoutTempNameCopy = NULL;
    char *stderrTempNameCopy = NULL;

    char buf[MAX_PATH] = {0};
    int len = GetTempPathA(sizeof(buf), buf);
    assert(len < sizeof(buf));
    // create temporary files for capturing stdout and stderr or the command
    res = GetTempFileNameA(buf, "stdout", 0, stdoutTempName);
    if (0 == res)
        goto Error;

    res = GetTempFileNameA(buf, "stderr", 0, stderrTempName);
    if (0 == res)
        goto Error;

    stdoutTempNameCopy = str_dup(stdoutTempName);
    stderrTempNameCopy = str_dup(stderrTempName);

    stdOut = CreateFileA(stdoutTempNameCopy,
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_WRITE|FILE_SHARE_READ,
        &sa, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, 0);
    if (INVALID_HANDLE_VALUE == stdOut)
        goto Error;

    stdErr = CreateFileA(stderrTempNameCopy,
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_WRITE|FILE_SHARE_READ,
        &sa, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, 0);
    if (INVALID_HANDLE_VALUE == stdErr)
        goto Error;

    siStartupInfo.hStdOutput = stdOut;
    siStartupInfo.hStdError = stdErr;

    BOOL ok = CreateProcessA(cmd, args, NULL, NULL, DONT_INHERIT_HANDLES,
        CREATE_DEFAULT_ERROR_MODE, NULL /*env*/, NULL /*curr dir*/,
        &siStartupInfo, &piProcessInfo);

    if (!ok)
        goto Error;

    // TODO: pass stdoutTempNameCopy and stderrTempNameCopy so upon
    // WinProcess destruction the files can be deleted and their memory freed
    WinProcess *wp = new WinProcess(&piProcessInfo);
    return wp;

Error:
    if (INVALID_HANDLE_VALUE != stdOut) {
        CloseHandle(stdOut);
    }

    if (INVALID_HANDLE_VALUE != stdErr) {
        CloseHandle(stdErr);
    }

    if (stdoutTempName[0]) {
        // TODO: delete stdoutTempName
    }
    if (stderrTempName[0]) {
        // TODO: delete stderrTempName
    }
    free(stdoutTempNameCopy);
    free(stderrTempNameCopy);
    return NULL;
}

WinProcess::WinProcess(PROCESS_INFORMATION *pi)
{
    memcpy(&m_processInfo, pi, sizeof(PROCESS_INFORMATION));
}


