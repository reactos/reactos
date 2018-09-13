/*===========================================================================*/
/*          Copyright (c) 1987 - 1988, Future Soft Engineering, Inc.         */
/*                              Houston, Texas                               */
/*===========================================================================*/

#define NOLSTRING    TRUE  /* jtf win3 mod */
#include <windows.h>
#include "port1632.h"
#include "dcrc.h"
#include "dynacomm.h"
#include "task.h"


/*****************************************************************************/
/* taskInit() -                                                              */
/*****************************************************************************/

VOID taskInit()
{
   kbdLock = KBD_UNLOCK;                     /* should be part of taskState */
   memcpy(taskState.string, NULL_STR, 2);
}

