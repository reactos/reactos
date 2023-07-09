#include <stdafx.h>
#include "Errors.h"
#include "EventCmd.h"

DWORD _E(DWORD dwErrCode,
         DWORD dwMsgId,
         ...)
{
    va_list arglist;

    gStrMessage.LoadString(dwMsgId);
    gStrMessage.AnsiToOem();
    va_start(arglist, dwMsgId);
    printf("[Err%05u] ", dwErrCode);
    vprintf(gStrMessage, arglist);
    fflush(stdout);
    return dwErrCode;
}

DWORD _W(DWORD dwWarnLevel,
         DWORD dwMsgId,
         ...)
{
    if (dwWarnLevel <= gCommandLine.GetVerboseLevel())
    {
        va_list arglist;

        gStrMessage.LoadString(dwMsgId);
        gStrMessage.AnsiToOem();
        va_start(arglist, dwMsgId);
        printf("[Wrn%02u] ", dwWarnLevel);
        vprintf(gStrMessage, arglist);
        fflush(stdout);
    }

    return dwWarnLevel;

}