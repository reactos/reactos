#ifndef _WINVER_H
#define _WINVER_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif
#define VS_FILE_INFO RT_VERSION
#define VS_VERSION_INFO 1
#define VS_USER_DEFINED 100
#define VS_FFI_SIGNATURE 0xFEEF04BD
#define VS_FFI_STRUCVERSION 0x10000
#define VS_FFI_FILEFLAGSMASK 0x3F
#define VS_FF_DEBUG 1
#define VS_FF_PRERELEASE 2
#define VS_FF_PATCHED 4
#define VS_FF_PRIVATEBUILD 8
#define VS_FF_INFOINFERRED 16
#define VS_FF_SPECIALBUILD 32
#define VOS_UNKNOWN 0
#define VOS_DOS 0x10000
#define VOS_OS216 0x20000
#define VOS_OS232 0x30000
#define VOS_NT 0x40000
#define VOS__BASE 0
#define VOS__WINDOWS16 1
#define VOS__PM16 2
#define VOS__PM32 3
#define VOS__WINDOWS32 4
#define VOS_DOS_WINDOWS16 0x10001
#define VOS_DOS_WINDOWS32 0x10004
#define VOS_OS216_PM16 0x20002
#define VOS_OS232_PM32 0x30003
#define VOS_NT_WINDOWS32 0x40004
#define VFT_UNKNOWN 0
#define VFT_APP 1
#define VFT_DLL 2
#define VFT_DRV 3
#define VFT_FONT 4
#define VFT_VXD 5
#define VFT_STATIC_LIB 7
#define VFT2_UNKNOWN 0
#define VFT2_DRV_PRINTER 1
#define VFT2_DRV_KEYBOARD 2
#define VFT2_DRV_LANGUAGE 3
#define VFT2_DRV_DISPLAY 4
#define VFT2_DRV_MOUSE 5
#define VFT2_DRV_NETWORK 6
#define VFT2_DRV_SYSTEM 7
#define VFT2_DRV_INSTALLABLE 8
#define VFT2_DRV_SOUND 9
#define VFT2_DRV_COMM 10
#define VFT2_DRV_INPUTMETHOD 11
#define VFT2_FONT_RASTER 1
#define VFT2_FONT_VECTOR 2
#define VFT2_FONT_TRUETYPE 3
#define VFFF_ISSHAREDFILE 1
#define VFF_CURNEDEST 1
#define VFF_FILEINUSE 2
#define VFF_BUFFTOOSMALL 4
#define VIFF_FORCEINSTALL 1
#define VIFF_DONTDELETEOLD 2
#define VIF_TEMPFILE 1
#define VIF_MISMATCH 2
#define VIF_SRCOLD 4
#define VIF_DIFFLANG 8
#define VIF_DIFFCODEPG 16
#define VIF_DIFFTYPE 32
#define VIF_WRITEPROT 64
#define VIF_FILEINUSE 128
#define VIF_OUTOFSPACE 256
#define VIF_ACCESSVIOLATION 512
#define VIF_SHARINGVIOLATION 1024
#define VIF_CANNOTCREATE 2048
#define VIF_CANNOTDELETE 4096
#define VIF_CANNOTRENAME 8192
#define VIF_CANNOTDELETECUR 16384
#define VIF_OUTOFMEMORY 32768
#define VIF_CANNOTREADSRC  65536
#define VIF_CANNOTREADDST 0x20000
#define VIF_BUFFTOOSMALL 0x40000
#ifndef RC_INVOKED
typedef struct tagVS_FIXEDFILEINFO {
	DWORD dwSignature;
	DWORD dwStrucVersion;
	DWORD dwFileVersionMS;
	DWORD dwFileVersionLS;
	DWORD dwProductVersionMS;
	DWORD dwProductVersionLS;
	DWORD dwFileFlagsMask;
	DWORD dwFileFlags;
	DWORD dwFileOS;
	DWORD dwFileType;
	DWORD dwFileSubtype;
	DWORD dwFileDateMS;
	DWORD dwFileDateLS;
} VS_FIXEDFILEINFO;
DWORD WINAPI VerFindFileA(DWORD,LPSTR,LPSTR,LPSTR,LPSTR,PUINT,LPSTR,PUINT);
DWORD WINAPI VerFindFileW(DWORD,LPWSTR,LPWSTR,LPWSTR,LPWSTR,PUINT,LPWSTR,PUINT);
DWORD WINAPI VerInstallFileA(DWORD,LPSTR,LPSTR,LPSTR,LPSTR,LPSTR,LPSTR,PUINT);
DWORD WINAPI VerInstallFileW(DWORD,LPWSTR,LPWSTR,LPWSTR,LPWSTR,LPWSTR,LPWSTR,PUINT);
DWORD WINAPI GetFileVersionInfoSizeA(LPSTR,PDWORD);
DWORD WINAPI GetFileVersionInfoSizeW(LPWSTR,PDWORD);
BOOL WINAPI GetFileVersionInfoA(LPSTR,DWORD,DWORD,PVOID);
BOOL WINAPI GetFileVersionInfoW(LPWSTR,DWORD,DWORD,PVOID);
DWORD WINAPI VerLanguageNameA(DWORD,LPSTR,DWORD);
DWORD WINAPI VerLanguageNameW(DWORD,LPWSTR,DWORD);
BOOL WINAPI VerQueryValueA(const LPVOID,LPSTR,LPVOID*,PUINT);
BOOL WINAPI VerQueryValueW(const LPVOID,LPWSTR,LPVOID*,PUINT);
#ifdef UNICODE
#define VerFindFile VerFindFileW
#define VerQueryValue VerQueryValueW
#define VerInstallFile VerInstallFileW
#define GetFileVersionInfoSize GetFileVersionInfoSizeW
#define GetFileVersionInfo GetFileVersionInfoW
#define VerLanguageName VerLanguageNameW
#define VerQueryValue VerQueryValueW
#else
#define VerQueryValue VerQueryValueA
#define VerFindFile VerFindFileA
#define VerInstallFile VerInstallFileA
#define GetFileVersionInfoSize GetFileVersionInfoSizeA
#define GetFileVersionInfo GetFileVersionInfoA
#define VerLanguageName VerLanguageNameA
#define VerQueryValue VerQueryValueA
#endif
#endif
#ifdef __cplusplus
}
#endif
#endif
