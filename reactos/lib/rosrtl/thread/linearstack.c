/* $Id: linearstack.c,v 1.1.32.2 2004/10/25 02:25:01 ion Exp $
*/
/*
*/

#include <windows.h>
#include <ndk/umtypes.h>
#include <ndk/zwtypes.h>
#include "thread.h"


NTSTATUS NTAPI RtlpRosValidateLinearUserStack
(
 IN PVOID StackBase,
 IN PVOID StackLimit,
 IN BOOLEAN Direction
)
{
 /* the stack has a null or negative (relatively to its direction) length */
 /*
  Direction: TRUE for down-top, FALSE for top-down
 
  Direction == TRUE and positive stack size: OK
  Direction == TRUE and negative stack size: error
  Direction == FALSE and positive stack size: error
  Direction == FALSE and negative stack size: OK
 */
 if
 (
  StackBase == StackLimit ||
  (Direction ^ ((PCHAR)StackBase < (PCHAR)StackLimit))
 )
  return STATUS_BAD_INITIAL_STACK;

 /* valid stack */
 return STATUS_SUCCESS;
}

/* EOF */
