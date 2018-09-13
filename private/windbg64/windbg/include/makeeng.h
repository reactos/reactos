/*++ BUILD Version: 0001    // Increment this if a change has global effects
---*/
/*********************************************************************

    File:                   makeeng.h

    Date created:       27/8/90

    Author:             Tim Bell

    Description:

    Windows Make Engine API

    Modified:

*********************************************************************/



typedef enum
{
    EXEC_RESTART,
    EXEC_GO,
    EXEC_STEPANDGO,
    EXEC_TOCURSOR,
    EXEC_TRACEINTO,
    EXEC_STEPOVER
} EXECTYPE;

BOOL PASCAL ExecDebuggee(EXECTYPE ExecType);
