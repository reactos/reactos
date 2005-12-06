/* 
    ADVPACK.H
 */

#ifndef _ADVPACK_H
#define _ADVPACK_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _StrEntry {
    LPSTR   Name;
    LPSTR   Value;
} STRENTRY, *LPSTRENTRY;

typedef const STRENTRY CSTRENTRY;
typedef CSTRENTRY *LPCSTRENTRY;

typedef struct _StrTable {
    DWORD       Entries;
    LPSTRENTRY  Array;
} STRTABLE, *LPSTRTABLE;

typedef const STRTABLE CSTRTABLE;
typedef CSTRTABLE *LPCSTRTABLE;

typedef struct _CabInfo {
    PSTR  Cab;
    PSTR  Inf;
    PSTR  Section;
    char  Path[MAX_PATH];
    DWORD Flags;
} CABINFO, *PCABINFO;

typedef PVOID HINF;

typedef struct _PERUSERSECTION {
    char GUID[59];
    char DisplayName[128];
    char Locale[10];
    char Stub[MAX_PATH*4];
    char Version[32];
    char CompId[128];
    DWORD IsInstalled;
    BOOL RollBack;
} PERUSERSECTION, *PPERUSERSECTION;


HRESULT WINAPI
AddDelBackupEntry( LPCSTR FileList, 
                   LPCSTR BackupDir,
                   LPCSTR BaseName, 
                   DWORD  Flags );

HRESULT WINAPI
AdvInstallFile( HWND hwnd,
                LPCSTR SourceDir,
                LPCSTR SourceFile,
                LPCSTR DestDir,
                LPCSTR DestFile,
                DWORD Flags,
                DWORD Reserved);

HRESULT WINAPI
CloseINFEngine( HINF hinf );

HRESULT WINAPI
DelNode( LPCSTR FileOrDirName,
         DWORD Flags );

HRESULT WINAPI
DelNodeRunDLL32( HWND hwnd,
                 HINSTANCE Inst,
                 PSTR Params,
                 INT Index );


HRESULT WINAPI
ExecuteCab( HWND hwnd,
            PCABINFO Cab,
            LPVOID Reserved );

HRESULT WINAPI
ExtractFiles( LPCSTR CabName,
              LPCSTR ExpandDir,
              DWORD Flags,
              LPCSTR FileList,
              LPVOID LReserved,
              DWORD Reserved );

HRESULT WINAPI
FileSaveMarkNotExist( LPSTR FileList,
                      LPSTR PathDir,
                      LPSTR BackupBaseName );

HRESULT WINAPI
FileSaveRestore( HWND hwnd,
                 LPSTR FileList,
                 LPSTR PathDir,
                 LPSTR BackupBaseName,
                 DWORD Flags );
                 

HRESULT WINAPI
FileSaveRestoreOnINF( HWND hwnd,
                      PCSTR Title,
                      PCSTR InfFilename,
                      PCSTR Section,
                      HKEY BackupKey,
                      HKEY NBackupKey,
                      DWORD Flags );
                      

HRESULT WINAPI
GetVersionFromFile( LPSTR Filename,
                    LPDWORD MajorVer,
                    LPDWORD MinorVer,
                    BOOL  Version );

HRESULT WINAPI
GetVersionFromFileEx( LPSTR Filename,
                      LPDWORD MajorVer,
                      LPDWORD MinorVer,
                      BOOL  Version );

BOOL WINAPI
IsNTAdmin( DWORD Reserved,
           PDWORD PReserved );

INT WINAPI
LaunchINFSection( HWND hwnd,
                  HINSTANCE Inst,
                  PSTR Params,
                  INT Index );

HRESULT WINAPI
LaunchINFSectionEx( HWND hwnd,
                    HINSTANCE Inst,
                    PSTR Params,
                    INT Index );

BOOL WINAPI
NeedReboot( DWORD RebootCheck );

DWORD WINAPI
NeedRebootInit( VOID );

HRESULT WINAPI
OpenINFEngine( PCSTR InfFilename,
               PCSTR InstallSection,
               DWORD Flags,
               HINF hinf,
               PVOID Reserved );

HRESULT WINAPI
RebootCheckOnInstall( HWND hwnd,
                      PCSTR InfFilename,
                      PCSTR InfSection,
                      DWORD Reserved );

HRESULT WINAPI
RegInstall( HMODULE hm,
            LPCSTR InfSectionExec,
            LPCSTRTABLE TabStringSub );

HRESULT WINAPI
RegRestoreAll( HWND hwnd,
               PCSTR TitleString,
               HKEY BackupKey );

HRESULT WINAPI
RegSaveRestore( HWND hwnd,
                PCSTR TitleString,
                HKEY BackupKey,
                PCSTR RootKey,
                PCSTR SubKey,
                PCSTR ValueName,
                DWORD Flags );

HRESULT WINAPI
RegSaveRestoreOnINF( HWND hwnd,
                     PCSTR Title,
                     PCSTR InfFilename,
                     PCSTR Section,
                     HKEY BackupKey,
                     HKEY NBackupKey,
                     DWORD Flags );


HRESULT WINAPI
RunSetupCommand( HWND hwnd, 
                 LPCSTR ExeFilename,
                 LPCSTR InfSection,
                 LPCSTR PathExtractedFile,
                 LPCSTR DialogTitle,
                 PHANDLE HExeWait,
                 DWORD Flags,
                 LPVOID Reserved);


HRESULT WINAPI
TranslateInfString( PCSTR InfFilename,
                    PCSTR InstallSection,
                    PCSTR TranslateSection,
                    PCSTR TranslateKey,
                    PSTR  BufferToKey,
                    DWORD BufferSize,
                    PVOID Reserved );

HRESULT WINAPI
TranslateInfStringEx( HINF hinf,
                      PCSTR InfFilename,
                      PCSTR InstallSection,
                      PCSTR TranslateSection,
                      PCSTR TranslateKey,
                      PSTR  BufferToKey,
                      DWORD BufferSize,
                      PVOID Reserved );

HRESULT WINAPI
SetPerUserSecValues( PPERUSERSECTION PerUser );

HRESULT WINAPI
UserInstStubWrapper( HWND hwnd,
                     HINSTANCE Inst,
                     PSTR Params,
                     INT Index );

HRESULT WINAPI
UserUnInstStubWrapper( HWND hwnd,
                       HINSTANCE Inst,
                       PSTR Params,
                       INT Index );

/*
 *
 *
 */
HRESULT WINAPI                      
DoInfInstall( DWORD Unknown);

HRESULT WINAPI
RegisterOCX( LPCTSTR Filename );



#ifdef __cplusplus
}
#endif
#endif
