//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// debug.cpp 
//
//   Debug file.  This file makes use to the shell debugging macros and
//   functions defined in shell\inc\debug.h
//
//   History:
//
//       3/16/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

#include "stdinc.h"

//
// Define strings used by debug.h.  Declaring DECLARE_DEBUG causes debug.h to
// define its c objects here.
//

#define SZ_DEBUGINI         "shellext.ini"
#define SZ_DEBUGSECTION     "cdfview"
#define SZ_MODULE           "CDFVIEW"

#define DECLARE_DEBUG

#include <ccstock.h>   // TEXTW macro used in debug.h  
#include <debug.h>

#ifdef UNIX
#ifdef DEBUG
extern "C" void _DebugAssertMsgUnix(char *msg, char *fileName, int line)
{
    printf("CDFVIEW: asrt(%s) %s, l %d\n", msg, fileName, line);
}

extern "C" void _DebugMsgUnix(int i, const char *s, ...)
{
    char buffer[1024];
    va_list ap;
    va_start(ap,s);
    vsprintf(buffer,s,ap);
    printf(buffer); printf("\n");
    va_end(ap);
}
#endif /* DEBUG */
#endif /* UNIX */

