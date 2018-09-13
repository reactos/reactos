#ifndef __dll_h
#define __dll_h

#define T_HKEY   0x0001
#define T_VALUE  0x0002
#define T_DWORD  0x0003
#define T_END    0xFFFF

extern HINSTANCE g_hInstance;

STDAPI_(void) DllAddRef(void);
STDAPI_(void) DllRelease(void);

void InstallMyPictures();

#ifdef DEBUG
void DllSetTraceMask(void);
#endif

#endif
