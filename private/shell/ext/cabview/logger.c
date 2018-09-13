#include <stdio.h>
#include <stdarg.h>
#include <windows.h>

#include "logger.h"

void Log(char *pszfmt, ...)
{
    char message[1000];
    va_list args;
    FILE *logFile;

    logFile = fopen("c:\\cabview.txt","a");
    if (logFile != NULL)
    {
        va_start(args,pszfmt);
        vsprintf(message,pszfmt,args);
        va_end(args);

        fprintf(logFile,message);

        fclose(logFile);
    }
	else
	{
		Beep(0,0);
	}
}

