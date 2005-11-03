/*
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS Multimedia
 * FILE:                 lib/mmdrv/auxil.c
 * PURPOSE:              Multimedia User Mode Driver
 * PROGRAMMER:           Andrew Greenwood
 *                       Aleksey Bragin
 * UPDATE HISTORY:
 *                       Mar 16, 2004: Created skeleton implementation
 */

#include "mmdrv.h"

#define NDEBUG
#include <debug.h>

APIENTRY DWORD auxMessage(UINT uDevice,
                  UINT uMessage,
                  DWORD dwUser,
                  DWORD dwParam1,
                  DWORD dwParam2)

{
    DPRINT("auxMessage\n");

	// the following cases are documented by DDK
	switch (uMessage)
	{
	case AUXDM_GETDEVCAPS:
		return 0;
	case AUXDM_GETNUMDEVS:
		return 0;
	case AUXDM_GETVOLUME:
		return 0;
	case AUXDM_SETVOLUME:
		return 0;
	}

    return MMSYSERR_NOERROR;
}
