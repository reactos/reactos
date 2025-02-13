/*
 * PROJECT:          Mke2fs
 * FILE:             Timer.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 */

/* INCLUDES **************************************************************/

#include "Mke2fs.h"

/* DEFINITIONS ***********************************************************/


/* FUNCTIONS *************************************************************/

void uuid_generate(__u8 *uuid)
{
    int i;
    ULONG seed = NtGetTickCount();

    for (i = 0; i < 16; i++) {
        ULONG r = RtlRandom(&seed);
        uuid[i] = r & 0xFF;
    }
    uuid[6] = (uuid[6] & 0x0F) | 0x40;
    uuid[8] = (uuid[8] & 0x3F) | 0x80;
}
