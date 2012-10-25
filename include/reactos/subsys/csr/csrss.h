/*****************************  CSRSS Data  ***********************************/

#ifndef __INCLUDE_CSRSS_CSRSS_H
#define __INCLUDE_CSRSS_CSRSS_H

// Used in ntdll/csr/connect.c
#define CSR_CSRSS_SECTION_SIZE    (65536)


/*** win32csr thingies to remove. ***/
#if 1

typedef struct
{
    HDESK DesktopHandle;
} CSRSS_CREATE_DESKTOP, *PCSRSS_CREATE_DESKTOP;

typedef struct
{
    HWND DesktopWindow;
    ULONG Width;
    ULONG Height;
} CSRSS_SHOW_DESKTOP, *PCSRSS_SHOW_DESKTOP;

typedef struct
{
    HWND DesktopWindow;
} CSRSS_HIDE_DESKTOP, *PCSRSS_HIDE_DESKTOP;

#endif
/************************************/

#endif /* __INCLUDE_CSRSS_CSRSS_H */
