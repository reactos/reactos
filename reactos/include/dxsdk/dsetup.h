#ifndef __DSETUP_H__
#define __DSETUP_H__

#include <windows.h>

#ifdef _WIN32
#define COM_NO_WINDOWS_H
#include <objbase.h>
#else
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum _DSETUP_CB_PROGRESS_PHASE
{
   DSETUP_INITIALIZING,
   DSETUP_EXTRACTING,
   DSETUP_COPYING,
   DSETUP_FINALIZING
};

typedef struct _DSETUP_CB_PROGRESS
{
   DWORD dwPhase;
   DWORD dwInPhaseMaximum;
   DWORD dwInPhaseProgress;
   DWORD dwOverallMaximum;
   DWORD dwOverallProgress;
} DSETUP_CB_PROGRESS;

#ifdef _WIN32

INT WINAPI DirectXUnRegisterApplication( HWND     hWnd, LPGUID   lpGUID);

#ifndef ANSI_ONLY
    typedef struct _DIRECTXREGISTERAPPW
    {
        DWORD dwSize;
        DWORD dwFlags;
        LPWSTR lpszApplicationName;
        LPGUID lpGUID;
        LPWSTR lpszFilename;
        LPWSTR lpszCommandLine;
        LPWSTR lpszPath;
        LPWSTR lpszCurrentDirectory;
    } DIRECTXREGISTERAPPW, *PDIRECTXREGISTERAPPW, *LPDIRECTXREGISTERAPPW;

    typedef struct _DIRECTXREGISTERAPP2W
    {
        DWORD dwSize;
        DWORD dwFlags;
        LPWSTR lpszApplicationName;
        LPGUID lpGUID;
        LPWSTR lpszFilename;
        LPWSTR lpszCommandLine;
        LPWSTR lpszPath;
        LPWSTR lpszCurrentDirectory;
        LPWSTR lpszLauncherName;
    } DIRECTXREGISTERAPP2W, *PDIRECTXREGISTERAPP2W, *LPDIRECTXREGISTERAPP2W;

    INT WINAPI DirectXSetupW( HWND hWnd, LPWSTR lpszRootPath, DWORD  dwFlags);
    INT WINAPI DirectXRegisterApplicationW( HWND hWnd, LPVOID lpDXRegApp);
    UINT WINAPI DirectXSetupGetEULAW( LPWSTR lpszEULA, UINT cchEULA, WORD LangID);
#endif

#ifndef UNICODE_ONLY
    typedef struct _DIRECTXREGISTERAPPA
    {
        DWORD dwSize;
        DWORD dwFlags;
        LPSTR lpszApplicationName;
        LPGUID lpGUID;
        LPSTR lpszFilename;
        LPSTR lpszCommandLine;
        LPSTR lpszPath;
        LPSTR lpszCurrentDirectory;
    } DIRECTXREGISTERAPPA, *PDIRECTXREGISTERAPPA, *LPDIRECTXREGISTERAPPA;

    typedef struct _DIRECTXREGISTERAPP2A
    {
        DWORD dwSize;
        DWORD dwFlags;
        LPSTR lpszApplicationName;
        LPGUID lpGUID;
        LPSTR lpszFilename;
        LPSTR lpszCommandLine;
        LPSTR lpszPath;
        LPSTR lpszCurrentDirectory;
        LPSTR lpszLauncherName;
    } DIRECTXREGISTERAPP2A, *PDIRECTXREGISTERAPP2A, *LPDIRECTXREGISTERAPP2A;

    INT WINAPI DirectXSetupA( HWND hWnd, LPSTR lpszRootPath, DWORD dwFlags);
    INT WINAPI DirectXRegisterApplicationA( HWND hWnd, LPVOID lpDXRegApp);
    UINT WINAPI DirectXSetupGetEULAA( LPSTR lpszEULA, UINT cchEULA, WORD LangID);
#endif

#ifdef UNICODE
    typedef DIRECTXREGISTERAPPW DIRECTXREGISTERAPP;
    typedef PDIRECTXREGISTERAPPW PDIRECTXREGISTERAPP;
    typedef LPDIRECTXREGISTERAPPW LPDIRECTXREGISTERAPP;
    typedef DIRECTXREGISTERAPP2W DIRECTXREGISTERAPP2;
    typedef PDIRECTXREGISTERAPP2W PDIRECTXREGISTERAPP2;
    typedef LPDIRECTXREGISTERAPP2W LPDIRECTXREGISTERAPP2;

    typedef INT (WINAPI * LPDIRECTXSETUP)(HWND, LPWSTR, DWORD);
    typedef INT (WINAPI * LPDIRECTXREGISTERAPPLICATION)(HWND, LPVOID);
    typedef UINT (WINAPI * LPDIRECTXSETUPGETEULA)(LPWSTR, UINT, WORD);

    #define DirectXSetup  DirectXSetupW
    #define DirectXRegisterApplication  DirectXRegisterApplicationW
    #define DirectXSetupGetEULA  DirectXSetupGetEULAW



#else
    typedef DIRECTXREGISTERAPPA DIRECTXREGISTERAPP;
    typedef PDIRECTXREGISTERAPPA PDIRECTXREGISTERAPP;
    typedef LPDIRECTXREGISTERAPPA LPDIRECTXREGISTERAPP;
    typedef DIRECTXREGISTERAPP2A DIRECTXREGISTERAPP2;
    typedef PDIRECTXREGISTERAPP2A PDIRECTXREGISTERAPP2;
    typedef LPDIRECTXREGISTERAPP2A LPDIRECTXREGISTERAPP2;

    typedef INT (WINAPI * LPDIRECTXSETUP)(HWND, LPSTR, DWORD);
    typedef INT (WINAPI * LPDIRECTXREGISTERAPPLICATION)(HWND, LPVOID);
    typedef UINT (WINAPI * LPDIRECTXSETUPGETEULA)(LPSTR, UINT, WORD);

    #define DirectXSetup  DirectXSetupA
    #define DirectXRegisterApplication  DirectXRegisterApplicationA
    #define DirectXSetupGetEULA  DirectXSetupGetEULAA
    
#endif

    typedef DWORD (*DSETUP_CALLBACK)( DWORD Reason, DWORD MsgType, LPSTR szMessage,
                                      LPSTR szName, void *pInfo);

    INT WINAPI DirectXSetupSetCallback(DSETUP_CALLBACK Callback);
    INT WINAPI DirectXSetupGetVersion(DWORD *lpdwVersion, DWORD *lpdwMinorVersion);
    INT WINAPI DirectXSetupShowEULA(HWND hWndParent);

#endif

#define FOURCC_VERS                                 mmioFOURCC('v','e','r','s')
#define DSETUPERR_SUCCESS_RESTART                    1
#define DSETUPERR_SUCCESS                            0
#define DSETUPERR_BADWINDOWSVERSION                 -1
#define DSETUPERR_SOURCEFILENOTFOUND                -2
#define DSETUPERR_NOCOPY                            -5
#define DSETUPERR_OUTOFDISKSPACE                    -6
#define DSETUPERR_CANTFINDINF                       -7
#define DSETUPERR_CANTFINDDIR                       -8
#define DSETUPERR_INTERNAL                          -9
#define DSETUPERR_UNKNOWNOS                         -11
#define DSETUPERR_NEWERVERSION                      -14
#define DSETUPERR_NOTADMIN                          -15
#define DSETUPERR_UNSUPPORTEDPROCESSOR              -16
#define DSETUPERR_MISSINGCAB_MANAGEDDX              -17
#define DSETUPERR_NODOTNETFRAMEWORKINSTALLED        -18
#define DSETUPERR_CABDOWNLOADFAIL                   -19
#define DSETUP_DDRAWDRV                             0x00000008
#define DSETUP_DSOUNDDRV                            0x00000010
#define DSETUP_DXCORE                               0x00010000
#define DSETUP_DIRECTX                              (DSETUP_DXCORE|DSETUP_DDRAWDRV|DSETUP_DSOUNDDRV)
#define DSETUP_MANAGEDDX                            0x00004000
#define DSETUP_TESTINSTALL                          0x00020000
#define DSETUP_DDRAW                                0x00000001
#define DSETUP_DSOUND                               0x00000002
#define DSETUP_DPLAY                                0x00000004
#define DSETUP_DPLAYSP                              0x00000020
#define DSETUP_DVIDEO                               0x00000040
#define DSETUP_D3D                                  0x00000200
#define DSETUP_DINPUT                               0x00000800
#define DSETUP_DIRECTXSETUP                         0x00001000
#define DSETUP_NOUI                                 0x00002000
#define DSETUP_PROMPTFORDRIVERS                     0x10000000
#define DSETUP_RESTOREDRIVERS                       0x20000000
#define DSETUP_CB_MSG_NOMESSAGE                      0
#define DSETUP_CB_MSG_INTERNAL_ERROR                10
#define DSETUP_CB_MSG_BEGIN_INSTALL                 13
#define DSETUP_CB_MSG_BEGIN_INSTALL_RUNTIME         14
#define DSETUP_CB_MSG_PROGRESS                      18
#define DSETUP_CB_MSG_WARNING_DISABLED_COMPONENT    19


#ifdef __cplusplus
};
#endif

#endif
