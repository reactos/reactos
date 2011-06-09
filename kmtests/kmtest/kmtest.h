/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Loader Application
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#ifndef _KMTESTS_H_
#define _KMTESTS_H_

#include <windows.h>

/* service control functions */
typedef DWORD SERVICE_FUNC(SC_HANDLE hManager);

SERVICE_FUNC Service_Create;
SERVICE_FUNC Service_Delete;
SERVICE_FUNC Service_Start;
SERVICE_FUNC Service_Stop;

DWORD Service_Control(SERVICE_FUNC *Service_Func);

#endif /* !defined _KMTESTS_H_ */
