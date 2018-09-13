/************************************************************************/
/*                                                                      */
/* RCPP - Resource Compiler Pre-Processor for NT system                 */
/*                                                                      */
/* ERROR.C - Error Handler Routines                                     */
/*                                                                      */
/* 04-Dec-90 w-BrianM  Update for NT from PM SDK RCPP                   */
/*                                                                      */
/************************************************************************/

#include "rc.h"
#include "rcmsgs.h"


/* defines for message types */
#define W_MSG   4000
#define E_MSG   2000
#define F_MSG   1000

static CHAR  Errbuff[128] = {0};


/************************************************************************/
/* Local Function Prototypes                                            */
/************************************************************************/
void message (int, int, PCHAR);


/************************************************************************/
/* ERROR - Print an error message to STDOUT.                            */
/************************************************************************/
#define MAX_ERRORS 100

void
error (
    int msgnum
    )
{
    message(E_MSG, msgnum, Msg_Text);
    if (++Nerrors > MAX_ERRORS) {
        Msg_Temp = GET_MSG (1003);
        SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, MAX_ERRORS);
        fatal(1003);            /* die - too many errors */
    }
    return;
}


/************************************************************************/
/* FATAL - Print an error message to STDOUT and exit.                   */
/************************************************************************/
void
fatal (
    int msgnum
    )
{
    message(F_MSG, msgnum, Msg_Text);
    quit(NULL);
}


/************************************************************************/
/* WARNING - Print an error message to STDOUT.                          */
/************************************************************************/
void
warning (
    int msgnum
    )
{
    message(W_MSG, msgnum, Msg_Text);
}


/************************************************************************/
/* MESSAGE - format and print the message to STDERR.                    */
/* The msg goes out in the form :                                       */
/*     <file>(<line>) : <msgtype> <errnum> <expanded msg>               */
/************************************************************************/
void
message(
    int msgtype,
    int msgnum,
    PCHAR msg
    )
{
    static CHAR mbuff[512];
    static CHAR mbuffT[512];
    PCHAR   p = mbuff;
    PCHAR   pT;
    PCHAR   msgname;
    CHAR    msgnumstr[32];

    if (Linenumber > 0 && Filename) {
        wsprintfA(p, "%ws(%d) : ", Filename, Linenumber);
        p += strlen(p);
    }
    if (msgtype) {
        switch (msgtype) {
            case W_MSG:
                msgname = GET_MSG(MSG_WARN);
                break;
            case E_MSG:
                msgname = GET_MSG(MSG_ERROR);
                break;
            case F_MSG:
                msgname = GET_MSG(MSG_FATAL);
                break;
        }
        /* remove CR and LF from message */
        for (pT = msgname ; *pT && *pT != '\n' && *pT != '\r' ; pT++)
            ;
        *pT = '\0';
        strcpy(p, msgname);
        p += strlen(msgname);
        wsprintfA(msgnumstr, " %s%d: ", "RC", msgnum);
        strcpy(p, msgnumstr);
        p += strlen(msgnumstr);
        strcpy(p, msg);
        p += strlen(p);
    }

    p = mbuff;
    pT = mbuffT;
    while (*p) {
        if (*p == '\\' && p[1] == '\\')
            p++;
        *pT++ = *p++;
    }

    *pT = '\0';
    p = mbuffT; // error message to print

    if (lpfnMessageCallback)
        (*lpfnMessageCallback)(0, 0, mbuff);
    if (hWndCaller) {
        if (SendMessageA(hWndCaller, WM_RC_ERROR, TRUE, (LPARAM) mbuff) != 0)
            quit("\n");
    }
    return;
}
