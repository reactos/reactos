#ifndef __MULTIMON_H
#define __MULTIMON_H

#ifdef __cplusplus
extern "C" {
#endif

HMONITOR WINAPI MonitorFromRect(LPCRECT,DWORD);
HMONITOR WINAPI MonitorFromWindow(HWND,DWORD);
HMONITOR WINAPI MonitorFromPoint(POINT,DWORD);

#ifdef __cplusplus
}
#endif

#endif /* __MULTIMON_H &/