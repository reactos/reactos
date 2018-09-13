// Debug utility functions

#include <windows.h>

_CRTIMP int __cdecl _CrtDbgReport(
        int nRptType, 
        const char * szFile, 
        int nLine,
        const char * szModule,
        const char * szFormat, 
        ...
        )
{
    char szBuf[1024];
    LPSTR lpsz = NULL;
    DWORD dwRet;
    va_list arglist;
    va_start(arglist, szFormat);
    
    wsprintf(szBuf, "CrtDbgReport: %s %s Line %d\r\n", szModule, szFile, nLine);
    OutputDebugString(szBuf);
    dwRet = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
        szFormat, 0, 0,(LPSTR)&lpsz, 0, &arglist);

    if(!dwRet || !lpsz)
    {
        OutputDebugString("FormatMessage failed\r\n");
        OutputDebugString(szFormat);
        return(-1);
    }

    OutputDebugString(lpsz);
    OutputDebugString("\r\n");
    
    if(lpsz)
        LocalFree((HLOCAL)lpsz);
        
    return(0);
}


