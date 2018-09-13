#include "rc.h"

int CALLBACK RC(HWND hWnd, int fStatus,
	RC_MESSAGE_CALLBACK lpfnMsg, RC_PARSE_CALLBACK lpfnParse,
	int argc, PCHAR *argv);

extern BOOL WINAPI     Handler(DWORD fdwCtrlType);

int CALLBACK UpdateRCStatus(ULONG u, ULONG dw, PCHAR p)
{
    int i = strlen(p);
    PCHAR ppercent = strchr(p, '%');
    
    if (i >= 2 && ((p[i - 1] == '\n' && p[i - 2] == '\r') ||
		   (p[i - 1] == '\r' && p[i - 2] == '\n')))
	p[i-2] = '\0';

    if (*p) {
	if (ppercent) {
	    for (ppercent=p,i=0 ; *ppercent ; ppercent++)
		if (*ppercent == '%')
		    i++;
	    ppercent = malloc(strlen(p) + 1 + i);
	    if (ppercent) {
		PCHAR pT = ppercent;

		while (*p) {
		    if (*p == '%')
			*pT++ = '%';
		    *pT++ = *p++;
		}
		*pT++ = '\0';
		printf(ppercent);
	    }
	    else {
		while (strchr(p, '%'))
		    *strchr(p, '%') = ' ';
		printf(p);
	    }
	}
	else {
	    printf(p);
	}
    }
    printf("\n");
    
    return(0);
}

int __cdecl main(int nArgC, char** pArgV)
{
    int rc;

    SetConsoleCtrlHandler(Handler, TRUE);
    rc = RC(NULL, 0, UpdateRCStatus, NULL, nArgC, (PCHAR*)pArgV);
    SetConsoleCtrlHandler(Handler, FALSE);
    exit(rc);
    return 0;
}

