//////////////////////////////////////////////////////////////////////////
//
// This is the public header file for apps that call MSPRINT.DLL
//
//////////////////////////////////////////////////////////////////////////

#ifndef SETUPX_INC
typedef WORD RETERR;
#endif

typedef RETERR (FAR PASCAL* PRINTERSETUPPROC)(HWND,WORD,LPSTR,WORD);

RETERR WINAPI PrinterSetup(HWND hWnd,WORD wAction,LPSTR lpBuffer,WORD wBufSize);

#ifdef WIN32

typedef BOOL (WINAPI* PRINTERSETUPPROC32)(HWND,WORD,WORD,LPBYTE,LPWORD);

BOOL WINAPI PrinterSetup32(HWND,WORD,WORD,LPBYTE,LPWORD);

#endif

#define MSPRINT_PRINTERSETUP (MAKEINTRESOURCE(50))

#define MSP_NEWPRINTER            1
#define MSP_NETPRINTER            2
#define MSP_NEWDRIVER             3
#define MSP_NETADMIN              4
#define MSP_TESTPAGEFULLPROMPT    5
#define MSP_TESTPAGEPARTIALPROMPT 7
#define MSP_TESTPAGENOPROMPT      8

// These are obsolete and will be going away soon...
#define MSPRINT_NEWPRINTER (MAKEINTRESOURCE(100))
#define MSPRINT_SETUPENTRY (MAKEINTRESOURCE(101))
