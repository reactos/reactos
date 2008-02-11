/*
 * PROJECT:          Mke2fs
 * FILE:             Timer.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 */

/* INCLUDES **************************************************************/

#include "windows.h"
#include "types.h"

/* DEFINITIONS ***********************************************************/


/* FUNCTIONS *************************************************************/

void uuid_generate(__u8 * uuid)
{
#if 0
    UuidCreate((UUID *) uuid);
#else
    RtlZeroMemory(uuid, 16);
#endif
}
