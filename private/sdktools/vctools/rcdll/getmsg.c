/************************************************************************/
/*                                                                      */
/* RCPP - Resource Compiler Pre-Processor for NT system                 */
/*                                                                      */
/* GETMSG.C - Replaces NMSGHDR.ASM and MSGS.ASM                         */
/*                                                                      */
/* 28-Nov-90 w-BrianM  Created to remove need for MKMSG.EXE             */
/*                                                                      */
/************************************************************************/

#include "rc.h"


CHAR msgbuf[4096];

/************************************************************************/
/* GET_MSG - Given a message number, get the correct format string      */
/************************************************************************/
PCHAR
GET_MSG (
    int msgnumber
    )
{
    int cb;

    cb = FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE|FORMAT_MESSAGE_IGNORE_INSERTS,
                   (HMODULE)hInstance, msgnumber, 0, msgbuf, 4096, NULL);
    if (cb)
        return msgbuf;
    else {
#if DBG
        printf("Internal error : message not found: %d\n", msgnumber);
#endif
        return ("");
    }
}


/************************************************************************/
/* SET_MSG - Given a format string, format it and store it in first parm*/
/************************************************************************/
void
SET_MSG (
    PCHAR exp,
    UINT n,
    PCHAR fmt,
    ...
    )
{
    va_list     arg_list;

    va_start (arg_list, fmt);

    FormatMessageA (FORMAT_MESSAGE_FROM_STRING, fmt, 0, 0,
                    exp, n, &arg_list);

    va_end (arg_list);
}
