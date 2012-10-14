/*
 * PROJECT:			ReactOS SvcHost
 * LICENSE:			GPL - See COPYING in the top level directory
 * FILE:			/base/services/svchost/svchost.h
 * PURPOSE:			Provide dll service loader
 * PROGRAMMERS:		Gregor Brunmar (gregor.brunmar@home.se)
 */

#pragma once

/* INCLUDES ******************************************************************/

#if 0
#define _CRT_SECURE_NO_DEPRECATE 1
#endif

#include <stdio.h>
#include <winsock2.h>
#include <tchar.h>

/* DEFINES *******************************************************************/

#define CS_TIMEOUT	1000

typedef struct _SERVICE {
    PTSTR		Name;
	HINSTANCE	hServiceDll;	
    LPSERVICE_MAIN_FUNCTION	ServiceMainFunc;
	struct _SERVICE	*Next;
} SERVICE, *PSERVICE;


/* FUNCTIONS *****************************************************************/

/* EOF */
