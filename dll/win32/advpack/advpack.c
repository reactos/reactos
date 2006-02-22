/*
 * ReactOS Advpack User Library
 * Copyright (C) 2004 ReactOS Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 *  See reference ie6_lib/include/advpub.h from msdn. 
 *  Query "internet explorer headers libraries".
 *
 *  Used wine/debug.h, if someone wanted to, they could port advpack over
 *  to the Wine project.
 */


#include <windows.h>
#include "advpack.h"
#include "wine/debug.h"

/*
 * @unimplemented
 */
BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  return TRUE;
}

/*
 * @unimplemented
 */
HRESULT WINAPI
AddDelBackupEntry( LPCSTR FileList, 
                   LPCSTR BackupDir,
                   LPCSTR BaseName, 
                   DWORD  Flags )
{
  FIXME("AddDelBackupEntry not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return E_FAIL;
}

/*
 * @unimplemented
 */
HRESULT WINAPI
AdvInstallFile( HWND hwnd,
                LPCSTR SourceDir,
                LPCSTR SourceFile,
                LPCSTR DestDir,
                LPCSTR DestFile,
                DWORD Flags,
                DWORD Reserved)
{
  FIXME("AdvInstallFile not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return E_FAIL;
}

/*
 * @unimplemented
 */
HRESULT WINAPI
CloseINFEngine( HINF hinf )
{
  FIXME("CloseINFEngine not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return E_FAIL;
}

/*
 * @unimplemented
 */
HRESULT WINAPI
DelNode( LPCSTR FileOrDirName,
         DWORD Flags )
{
  FIXME("DelNode not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return E_FAIL;
}

/*
 * @unimplemented
 */
HRESULT WINAPI
DelNodeRunDLL32( HWND hwnd,
                 HINSTANCE Inst,
                 PSTR Params,
                 INT Index )
{
  FIXME("DelNodeRunDLL32 not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return E_FAIL;
}

/*
 * @unimplemented
 */
HRESULT WINAPI
ExecuteCab( HWND hwnd,
            PCABINFO Cab,
            LPVOID Reserved )
{
  FIXME("ExecuteCab not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return E_FAIL;
}

/*
 * @unimplemented
 */
HRESULT WINAPI
ExtractFiles( LPCSTR CabName,
              LPCSTR ExpandDir,
              DWORD Flags,
              LPCSTR FileList,
              LPVOID LReserved,
              DWORD Reserved )
{
  FIXME("ExtractFiles not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return E_FAIL;
}

/*
 * @unimplemented
 */
HRESULT WINAPI
FileSaveMarkNotExist( LPSTR FileList,
                      LPSTR PathDir,
                      LPSTR BackupBaseName )
{
  FIXME("FileSaveMarkNotExist not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return E_FAIL;
}


/*
 * @unimplemented
 */
HRESULT WINAPI
FileSaveRestore( HWND hwnd,
                 LPSTR FileList,
                 LPSTR PathDir,
                 LPSTR BackupBaseName,
                 DWORD Flags )
{
  FIXME("FileSaveRestore not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return E_FAIL;
}
                 

/*
 * @unimplemented
 */
HRESULT WINAPI
FileSaveRestoreOnINF( HWND hwnd,
                      PCSTR Title,
                      PCSTR InfFilename,
                      PCSTR Section,
                      HKEY BackupKey,
                      HKEY NBackupKey,
                      DWORD Flags )
{
  FIXME("FileSaveRestoreOnINF not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return E_FAIL;
}
                      

/*
 * @implemented
 */
HRESULT WINAPI
GetVersionFromFile( LPSTR Filename,
                    LPDWORD MajorVer,
                    LPDWORD MinorVer,
                    BOOL  Version )
{
  return (GetVersionFromFileEx(Filename, MajorVer, MinorVer, Version));
}

/*
 * @implemented
 */
HRESULT WINAPI
GetVersionFromFileEx( LPSTR Filename,
                      LPDWORD MajorVer,
                      LPDWORD MinorVer,
                      BOOL  Version )
{
  DWORD Size, Reserved, Param;
  UINT Length;
  LPVOID Buffer;
  
  if (Version)
    {
       Size = GetFileVersionInfoSizeA(Filename, &Reserved);
  
       Buffer = LocalAlloc ( GMEM_ZEROINIT, Size );
  
       GetFileVersionInfoA( Filename, 0, Size, Buffer); 

       VerQueryValueA( Buffer,
                       "\\StringFileInfo\\040904b0\\FileVersion",
                       (LPVOID) &Param,
                       &Length );

       MajorVer = (LPDWORD) (DWORD) HIWORD(Param);
       MinorVer = (LPDWORD) (DWORD) LOWORD(Param);

       LocalFree( Buffer );
    } 
  else
    {
       /* Language ID */
       MajorVer = (LPDWORD) LANG_ENGLISH;
       /*  I guess EnumResourceLanguages is used with a call back.
        *  Return PRIMARYLANGID(wIDLanguage) from the call back.
        */
       
       /* Codepage ID */
       MinorVer = (LPDWORD) GetACP();
       /* Here again, another guess. */
    }

  return S_OK;
}


/*
 * @implemented
 */
BOOL WINAPI
IsNTAdmin( DWORD Reserved,
           PDWORD PReserved )
{
  HANDLE Process, Token;
  UINT i;
  BOOL Good = FALSE;
  DWORD Buffer[4096];
  DWORD Size;
  PTOKEN_GROUPS TokenGrp = (PTOKEN_GROUPS) Buffer;
  SID_IDENTIFIER_AUTHORITY AuthSid = {SECURITY_NT_AUTHORITY};
  PSID psid = NULL;
  
  Process = GetCurrentProcess();

  if (OpenProcessToken(Process, TOKEN_QUERY, &Token) == FALSE)
    {
       CloseHandle(Process);
       return FALSE;
    }
  
  if ( GetTokenInformation( Token, TokenGroups, Buffer, 4096, &Size) == FALSE)
    {
       CloseHandle(Process);
       CloseHandle(Token);
       return FALSE;
    }

  CloseHandle(Process);
  CloseHandle(Token);

  if ( AllocateAndInitializeSid( &AuthSid,
                                  2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0,
                                 &psid) == FALSE)
    {
       return FALSE;
    }

  for ( i = 0; i < TokenGrp->GroupCount; i++ )
     {
        if ( EqualSid( psid , TokenGrp->Groups[i].Sid) != FALSE )
          {
             Good = TRUE;
             break;
          }
     }
  
  FreeSid( psid);
  
  return(Good);
}


/*
 * @unimplemented
 */
INT WINAPI
LaunchINFSection( HWND hwnd,
                  HINSTANCE Inst,
                  PSTR Params,
                  INT Index )
{
  FIXME("LaunchINFSection not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return E_FAIL;
}


/*
 * @unimplemented
 */
HRESULT WINAPI
LaunchINFSectionEx( HWND hwnd,
                    HINSTANCE Inst,
                    PSTR Params,
                    INT Index )
{
  FIXME("LaunchINFSectionEx not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return E_FAIL;
}


/*
 * @unimplemented
 */
BOOL WINAPI
NeedReboot( DWORD RebootCheck )
{
  FIXME("NeedReboot not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}



/*
 * @unimplemented
 */
DWORD WINAPI
NeedRebootInit( VOID )
{
  FIXME("NeedRebootInit not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return 0;
}


/*
 * @unimplemented
 */
HRESULT WINAPI
OpenINFEngine( PCSTR InfFilename,
               PCSTR InstallSection,
               DWORD Flags,
               HINF hinf,
               PVOID Reserved )
{
  FIXME("OpenINFEngine not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return E_FAIL;
}


/*
 * @unimplemented
 */
HRESULT WINAPI
RebootCheckOnInstall( HWND hwnd,
                      PCSTR InfFilename,
                      PCSTR InfSection,
                      DWORD Reserved )
{
  FIXME("RebootCheckOnInstall not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return E_FAIL;
}


/*
 * @unimplemented
 */
HRESULT WINAPI
RegInstall( HMODULE hm,
            LPCSTR InfSectionExec,
            LPCSTRTABLE TabStringSub )
{
  FIXME("RegInstall not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return E_FAIL;
}


/*
 * I found this on the net,
 * O4 - HKLM\..\RunOnce: [mcagntps.dll] rundll32.exe advpack.dll,
 *                RegisterOCX c:\PROGRA~1\mcafee.com\agent\mcagntps.dll
 *
 * Not sure if this implementation is correct. I guess it's ActiveX stuff.
 */
/*
 * @implemented
 */
HRESULT WINAPI
RegisterOCX(LPCTSTR Filename)
{
  FARPROC DllEntryPoint;
  LPCTSTR DllName = Filename;
  HINSTANCE hlib = LoadLibrary(DllName);

  FIXME("RegisterOCX not implemented\n");

  if (hlib < (HINSTANCE)HINSTANCE_ERROR)
       return E_FAIL; 
  DllEntryPoint = GetProcAddress( hlib, "DllRegisterServer");
  if (DllEntryPoint != NULL)
    {
       if (FAILED((*DllEntryPoint)()))
         {
            FreeLibrary(hlib);
            return E_FAIL;
         }
       return S_OK;
    }
  else
       return E_FAIL;
}


/*
 * @unimplemented
 */
HRESULT WINAPI
RegRestoreAll( HWND hwnd,
               PCSTR TitleString,
               HKEY BackupKey )
{
  FIXME("RegRestoreAll not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return E_FAIL;
}


/*
 * @unimplemented
 */
HRESULT WINAPI
RegSaveRestore( HWND hwnd,
                PCSTR TitleString,
                HKEY BackupKey,
                PCSTR RootKey,
                PCSTR SubKey,
                PCSTR ValueName,
                DWORD Flags )
{
  FIXME("RegSaveRestore not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return E_FAIL;
}


/*
 * @unimplemented
 */
HRESULT WINAPI
RegSaveRestoreOnINF( HWND hwnd,
                     PCSTR Title,
                     PCSTR InfFilename,
                     PCSTR Section,
                     HKEY BackupKey,
                     HKEY NBackupKey,
                     DWORD Flags )
{
  FIXME("RegSaveRestoreOnINF not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return E_FAIL;
}


/*
 * @unimplemented
 */
HRESULT WINAPI
RunSetupCommand( HWND hwnd, 
                 LPCSTR ExeFilename,
                 LPCSTR InfSection,
                 LPCSTR PathExtractedFile,
                 LPCSTR DialogTitle,
                 PHANDLE HExeWait,
                 DWORD Flags,
                 LPVOID Reserved)
{
  FIXME("RunSetupCommand not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return E_FAIL;
}


/*
 * @unimplemented
 */
HRESULT WINAPI
SetPerUserSecValues( PPERUSERSECTION PerUser )
{
  FIXME("SetPerUserSecValues not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return E_FAIL;
}

/*
 * @unimplemented
 */
HRESULT WINAPI
TranslateInfString( PCSTR InfFilename,
                    PCSTR InstallSection,
                    PCSTR TranslateSection,
                    PCSTR TranslateKey,
                    PSTR  BufferToKey,
                    DWORD BufferSize,
                    PVOID Reserved )
{
  FIXME("TranslateInfString not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return E_FAIL;
}


/*
 * @unimplemented
 */
HRESULT WINAPI
TranslateInfStringEx( HINF hinf,
                      PCSTR InfFilename,
                      PCSTR InstallSection,
                      PCSTR TranslateSection,
                      PCSTR TranslateKey,
                      PSTR  BufferToKey,
                      DWORD BufferSize,
                      PVOID Reserved )
{
  FIXME("TranslateInfStringEx not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return E_FAIL;
}

/*
 * @unimplemented
 */
HRESULT WINAPI
UserInstStubWrapper( HWND hwnd,
                     HINSTANCE Inst,
                     PSTR Params,
                     INT Index )
{
  FIXME("UserInstStubWrapper not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return E_FAIL;
}

/*
 * @unimplemented
 */
HRESULT WINAPI
UserUnInstStubWrapper( HWND hwnd,
                       HINSTANCE Inst,
                       PSTR Params,
                       INT Index )
{
  FIXME("UserUnInstStubWrapper not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return E_FAIL;
}


/* EOF */
