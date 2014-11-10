/*
 * newdevp.h
 *
 * Private header for newdev.dll
 *
 */

#ifndef __NEWDEVP__H
#define __NEWDEVP__H

BOOL
WINAPI
DevInstallW(
    IN HWND hWndParent,
    IN HINSTANCE hInstance,
    IN LPCWSTR InstanceId,
    IN INT Show);

BOOL
WINAPI
InstallDevInst(
    IN HWND hWndParent,
    IN LPCWSTR InstanceId,
    IN BOOL bUpdate,
    OUT LPDWORD lpReboot);

#endif /* __NEWDEVP__H */
