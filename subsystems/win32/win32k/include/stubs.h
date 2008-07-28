/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Win32K
 * FILE:            subsystems/win32/win32k/include/stubs.h
 * PURPOSE:         Stubs declared here, temp header
 * PROGRAMMER:      Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ******************************************************************/

W32KAPI VOID APIENTRY UMPDDrvQuerySpoolType(DWORD Param1, DWORD Param2);
W32KAPI VOID APIENTRY DefaultHTCallBack(DWORD Param1);
VOID APIENTRY DxEngGetRedirectionBitmap(DWORD Param1); // -> dxeng.h
