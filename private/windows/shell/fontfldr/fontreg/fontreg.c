/***************************************************************************
 * MODULE:  FontReg.c
 *
 * PURPOSE: This module does a very simple thing: Moves font information 
 *          from the [fonts] section of the WIN.INI into the registry.
 *
 *          1)  If a font is in the \windows\system directory, it is moved
 *              to \windows\fonts. (If it is already in the fonts directory
 *              it is just left there.)
 *
 *          2)  The information is written into the Registry at:
 *                   HKLM\Software\Microsoft\Windows\CurrentVersion\Fonts
 *
 *          3)  The entry i removed from WIN.INI
 *
 *
 *          NOTE: This program has NOT been checked in a DBCS environment.
 *          Somethings will need to be changed, most notibly, checking for 
 *          a backslash in a path name.
 *
 *
 * REVISION LOG:
 *    10/17/94    Initial revision. Eric Robinson, ElseWare Corporation.
 *                eric@elseware.com.
 *
 *
 ***************************************************************************/

////#include <windows.h>

#include <excpt.h>	// From windows.h.
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <winnls.h>
#include <winreg.h>


#include "ttf.h"


HKEY  hKeyFonts;

char szWindowsDir[MAX_PATH];
char szSystemDir[MAX_PATH];
char szSharedDir[MAX_PATH];
char szFontsDir[MAX_PATH];

BOOL bNetworkInstall;

BYTE Buffer[2048];


typedef struct
 {
  DWORD dwVersion;
  DWORD dwFlags;
  UINT  uDriveType;
  char  *pszFilePart;
  char  szIniName[MAX_PATH];
  char  szFullPath[MAX_PATH];
 } FILEDETAILS, *PFILEDETAILS;


//*****************************************************************************
//**************************   D P R I N T F   ********************************
//*****************************************************************************

#ifdef DEBUG   //************  DEBUG  **************

void msgprintf( LPCSTR pszMsg, ... )
 {
  char ach[256];

  wvsprintf(ach, pszMsg, ((char *)&pszMsg + sizeof(char *)));
  OutputDebugString(ach);
 }

#define dprintf( args ) msgprintf args

#else          //***********  RETAIL  *************

#define dprintf( args )

#endif


//*****************************************************************************
//******************   A P P E N D   F I L E   N A M E   **********************
//*****************************************************************************

void AppendFileName( PSZ pszPath, PSZ pszFile )
 {
  int cbPath;

  cbPath = lstrlen(pszPath);
  if( pszPath[cbPath-1] != '\\' ) pszPath[cbPath++] = '\\';

  lstrcpy( pszPath+cbPath, pszFile );
 }


//*****************************************************************************
//*************************   O P E N   K E Y   *******************************
//*****************************************************************************

#define szCurrentVersion "Software\\Microsoft\\Windows\\CurrentVersion"

HKEY OpenKey( char *pszKey )
 {
  HKEY  hKey;
  LONG  lrc;
  DWORD dwBogus;
  char  szSubKey[256];


  lstrcpy( szSubKey, szCurrentVersion );
  lstrcat( szSubKey, pszKey );

  dprintf(( "   RegCreateKeyEx('%s')\n", szSubKey ));

  lrc = RegCreateKeyEx( HKEY_LOCAL_MACHINE,	    // hKey
  						      szSubKey,				    // lpszSubKey
  						      0L,							 // dwReserved
  						      NULL,						    // lpszClass
  						      REG_OPTION_NON_VOLATILE, // fdwOptions
  						      KEY_ALL_ACCESS,			 // samDesired
  						      NULL,							 // lpSecurityAttributes
  						      &hKey,						 // phkResult
  						      &dwBogus );					 // pdwDisposition

  if( lrc != ERROR_SUCCESS )
	{
    hKey = NULL;
	 dprintf(( "   Couldn't open registry key '%s'\n", szSubKey ));
	}

  return hKey;
 }


//*****************************************************************************
//****************   G E T   F O N T S   D I R E C T O R Y   ******************
//*****************************************************************************

void GetFontsDirectory( void )
 {
  HKEY   hKey;
  LONG   lrc = ERROR_SUCCESS+1;
  DWORD  cbShared;


  dprintf(( "Getting shared directory string from registry\r\n" ));

  hKey = OpenKey( "\\Setup" );
  if( hKey )
   {
    cbShared = MAX_PATH;

    lrc = RegQueryValueEx( hKey,					// hKey
    							  "SharedDir",			// lpszValueName
    							  NULL,					// lpdwReserved
    							  NULL,					// lpdwType
    							  szSharedDir,		   // lpbData
    							  &cbShared );    	// lpcbData

    RegCloseKey( hKey );
   }

  if( lrc != ERROR_SUCCESS )
   {
    dprintf(( "RegQueryValue( SharedDir ) failed! Error = %d\r\n", lrc ));
    dprintf(( "  Using path from GetWindowsDirectory\r\n" ));

    lstrcpy( szSharedDir, szWindowsDir );
   }

  dprintf(( "  szSharedDir = '%s'\r\n", szSharedDir ));

  lstrcpy( szFontsDir, szSharedDir );
  AppendFileName( szFontsDir, "fonts" );

  dprintf(( "  szFontsDir = '%s'\r\n", szFontsDir ));
 }


//*****************************************************************************
//*************************   G E T   D I R S   *******************************
//*****************************************************************************

void GetDirs( void )
 {
  GetWindowsDirectory( szWindowsDir, MAX_PATH );
  GetSystemDirectory( szSystemDir, MAX_PATH );

  GetFontsDirectory();

  bNetworkInstall = (lstrcmpi( szSharedDir, szWindowsDir ) != 0);

  dprintf(( "GetDirs\r\n" ));
  dprintf(( "  szWindowsDir = '%s'\r\n", szWindowsDir ));
  dprintf(( "  szSystemDir  = '%s'\r\n", szSystemDir  ));
  dprintf(( "  szSharedDir  = '%s'\r\n", szSharedDir  ));
  dprintf(( "  szFontsDir   = '%s'\r\n", szFontsDir   ));

  dprintf(( "  bNetworkInstall = %d\r\n", bNetworkInstall ));

 }


//*****************************************************************************
//********************   F I N D   F O N T   F I L E   ************************
//*****************************************************************************

#define MODE_WIN31  1
#define MODE_WIN95  2


PSZ FindFontFile( PSZ pszFile, PSZ pszFullPath, DWORD dwMode )
 {
  DWORD dwrc;
  PSTR  pszFilePart;


  dprintf(( "FindFontFile\r\n" ));
  pszFilePart = NULL;


// Search fonts directory first if in win95 mode

  if( dwMode == MODE_WIN95 )
   {
    dprintf(( "Calling SearchPath( %s, %s )\r\n", szFontsDir, pszFile ));
    dwrc = SearchPath( szFontsDir, 				// Path to search for file
     					     pszFile, 					// Filename to search for
  					        NULL, 						// No extension
  					        MAX_PATH,   				// Size of output buffer
    					     pszFullPath,				// Output buffer
    					     &pszFilePart );			// & of filename pointer

    dprintf(( "  dwrc = %u\n", dwrc ));
    if( dwrc ) goto FFFReturn;
   }


// Search win31 style

  dprintf(( "Calling SearchPath( NULL, %s )\r\n", pszFile ));
  dwrc = SearchPath( NULL,							// Path to search for file
                     pszFile,						// Filename to search for
                     NULL,							// No extension
                     MAX_PATH,					// Size of output buffer
                     pszFullPath,				// Output buffer
                     &pszFilePart );			// & of filename pointer

  dprintf(( "  dwrc = %u\r\n", dwrc ));


// If not found and in win31 mode, search in fonts folder

  if( !dwrc && dwMode == MODE_WIN31 )
   {
    dprintf(( "Calling SearchPath( %s, %s )\r\n", szFontsDir, pszFile ));
    dwrc = SearchPath( szFontsDir, 				// Path to search for file
     					     pszFile, 					// Filename to search for
  					        NULL, 						// No extension
  					        MAX_PATH,   				// Size of output buffer
    					     pszFullPath,				// Output buffer
    					     &pszFilePart );			// & of filename pointer

    dprintf(( "  dwrc = %u\n", dwrc ));
   }


FFFReturn:
  return pszFilePart;
 }


//*****************************************************************************
//*******************   G E T   F I L E   D E T A I L S   *********************
//*****************************************************************************

#define IN_SYSTEM   0x0001
#define IN_WINDOWS  0x0002
#define IN_SHARED   0x0004
#define IN_FONTS    0x0008
#define IN_OTHER    0x0010

#define FON_FILE    0x0100
#define FOT_FILE    0x0200
#define TTF_FILE    0x0400

#define SZT2BANNER  "This is a TrueType font, not a program.\r\r\n$"


BOOL GetFileDetails( PSTR pszIniFile, PFILEDETAILS pfd, DWORD dwMode )
 {
  HANDLE hFile;
  char   szDir[MAX_PATH];


//-----------------------  Crunch source filename  ----------------------------

  pfd->dwFlags    = 0;
  pfd->uDriveType = 0;
  pfd->dwVersion  = 0;
  lstrcpy( pfd->szIniName, pszIniFile );


  dprintf(( "GetFileDetails\r\n" ));
  dprintf(( "  pszIniFile = '%s'\r\n",    pszIniFile ));
  dprintf(( "  dwMode     = 0x%.8lX\r\n", dwMode     ));


  pfd->pszFilePart = FindFontFile( pszIniFile, pfd->szFullPath, dwMode );
  if( !pfd->pszFilePart )
   {
    dprintf(( "  Couldn't find file\r\n" ));
    return FALSE;
   }

  dprintf(( "    szFullPath  = '%s'\r\n", pfd->szFullPath ));
  dprintf(( "    pszFilePart = '%s'\r\n", pfd->pszFilePart ));


//------------------  Figure out which directory its in  ----------------------

  lstrcpy( szDir, pfd->szFullPath );
  szDir[pfd->pszFilePart-pfd->szFullPath-1] = '\0';

  dprintf(( "  szDir = '%s'\r\n", szDir ));

  if(       !lstrcmpi(szSystemDir,szDir) )
    pfd->dwFlags |= IN_SYSTEM;
   else if( !lstrcmpi(szWindowsDir,szDir) )
    pfd->dwFlags |= IN_WINDOWS;
   else if( !lstrcmpi(szSharedDir,szDir) )
    pfd->dwFlags |= IN_SHARED;
   else if( !lstrcmpi(szFontsDir,szDir) )
    pfd->dwFlags |= IN_FONTS;


  if( szDir[0] != '\\' )
    {
     szDir[3] = 0;
     dprintf(( "GetDriveType( %s )\r\n", szDir ));
     pfd->uDriveType = GetDriveType( szDir );
    }
   else
    pfd->uDriveType = DRIVE_REMOTE;

  dprintf(( "  uDriveType = %d\r\n", pfd->uDriveType ));


//-----------------  Read in first chunk for examination  ---------------------

  hFile = CreateFile( pfd->szFullPath,
                      GENERIC_READ,
                      FILE_SHARE_READ,
                      NULL,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL,
                      NULL );

  if( hFile != INVALID_HANDLE_VALUE )
    {
     DWORD dwRead;

     if( ReadFile( hFile, Buffer, sizeof(Buffer), &dwRead, NULL ) )
       {
        if( lstrcmp( &Buffer[78], SZT2BANNER ) == 0 )   // FOT check
          {
           dprintf(( "  FOT file\r\n" ));
           pfd->dwFlags |= FOT_FILE;
          }
         else if( *(WORD *)&Buffer[0] == 0x5A4D )       // FON check
          {
           dprintf(( "  FON file\r\n" ));
           pfd->dwFlags |= FON_FILE;
          }
         else                                           // TTF check
          {
           int i, nOffsets;
           sfnt_OffsetTablePtr    pOffsetTable;
           sfnt_DirectoryEntryPtr pDirEntry;
           sfnt_FontHeader        FontHeader;


           dprintf(( " Checking for TTF file\r\n" ));

           pOffsetTable = (sfnt_OffsetTablePtr)Buffer;
           nOffsets     = (int)SWAPW(pOffsetTable->numOffsets);

           dprintf(( "   Version    = 0x%.8X\r\n", SWAPL(pOffsetTable->version) ));
           dprintf(( "   numOffsets = %d\r\n",     nOffsets ));

           if( nOffsets < sizeof(Buffer)/sizeof(sfnt_DirectoryEntry) )
             {
              pDirEntry = (sfnt_DirectoryEntryPtr)pOffsetTable->table;

              for( i = 0; i < nOffsets; i++, pDirEntry++ )
               {
#ifdef DEBUG
                DWORD dwTag[2];
             
                dwTag[0] = (DWORD)pDirEntry->tag;
                dwTag[1] = 0;
                dprintf(( "  tag[%d] = '%s'\r\n", i, (PSZ)&dwTag[0] ));
#endif

                if( pDirEntry->tag == tag_head )
                 {
                  dprintf(( "Found 'head' table\r\n" ));
                  dprintf(( "  offset = %d\r\n", SWAPL(pDirEntry->offset) ));
                  dprintf(( "  length = %d\r\n", SWAPL(pDirEntry->length) ));

                  SetFilePointer( hFile, SWAPL(pDirEntry->offset), NULL, FILE_BEGIN );
                  
                  FontHeader.fontRevision = 0;
                  ReadFile( hFile, &FontHeader, sizeof(FontHeader), &dwRead, NULL );
                  
                  dprintf(( "  fontRevision = 0x%.8X\r\n", SWAPL(FontHeader.fontRevision) ));
                  
                  pfd->dwVersion = SWAPL(FontHeader.fontRevision);
                  pfd->dwFlags  |= TTF_FILE;
                  
                  break;
                 }
               }
           
             }
            else
             {
              dprintf(( "OffsetTable.numOffsets is out of range!" ));
             }
          }
       }
      else
       {
        dprintf(( "  ReadFile failed\r\n" ));
       }

  	  CloseHandle( hFile );
    }
   else
    {
     dprintf(( "  CreateFile failed\r\n" ));
    }


  dprintf(( "  dwFlags = 0x%.8lX\r\n", pfd->dwFlags ));

  return TRUE;
 }


//*****************************************************************************
//*******************   D O R K   O N   R E G I S T R Y   *********************
//*****************************************************************************

void DorkOnRegistry( PSZ pszDescription, PSZ pszRegFile )
 {
  LONG lrc;

  dprintf(( "  Writing registry entry: '%s' '%s'\r\n", pszDescription, pszRegFile ));

  lrc = RegSetValueEx( hKeyFonts,
                       pszDescription,
                       0,
                       REG_SZ, 
                       pszRegFile,
                       lstrlen(pszRegFile) );

  if( lrc != ERROR_SUCCESS )
   {
    dprintf(( "*** error writing to registry! rc = %d\r\n", GetLastError() ));
   }


  dprintf(( "<<< Calling AddFontResource(%s)\r\n", pszRegFile ));
  lrc = AddFontResource( pszRegFile );
  if( !lrc )
   {
    dprintf(( "*** AddFontResource(%s) failed!\r\n", pszRegFile ));
   }
 }


//*****************************************************************************
//********************   H A N D L E   E X I S T I N G   **********************
//*****************************************************************************

BOOL HandleExisting( char *pszCheck, PFILEDETAILS pfdSource )
 {
  FILEDETAILS fdExisting;


  dprintf(( "Looking for existing file'\r\n" ));

  dprintf(( "  pszCheck   = '%s'\r\n",   pszCheck ));

  dprintf(( "  Source file details\r\n" ));
  dprintf(( "    szIniName  = '%s'\r\n",   pfdSource->szIniName   ));
  dprintf(( "    szFullPath = '%s'\r\n",   pfdSource->szFullPath  ));
  dprintf(( "    szFilePart = '%s'\r\n",   pfdSource->pszFilePart ));
  dprintf(( "    dwFlags    = 0x%.8X\r\n", pfdSource->dwFlags     ));
  dprintf(( "    dwVersion  = 0x%.8X\r\n", pfdSource->dwVersion   ));

  if( !GetFileDetails( pszCheck, &fdExisting, MODE_WIN95 ) ) return FALSE;

  dprintf(( "  Existing file details\r\n" ));
  dprintf(( "    szIniPath  = '%s'\r\n",   fdExisting.szIniName   ));
  dprintf(( "    szFullPath = '%s'\r\n",   fdExisting.szFullPath  ));
  dprintf(( "    szFilePart = '%s'\r\n",   fdExisting.pszFilePart ));
  dprintf(( "    dwFlags    = 0x%.8X\r\n", fdExisting.dwFlags     ));
  dprintf(( "    dwVersion  = 0x%.8X\r\n", fdExisting.dwVersion   ));

  if( fdExisting.dwVersion >= pfdSource->dwVersion )
   {
    dprintf(( "Existing file is newer or the same, killing ini entry\r\n" ));

    if( (pfdSource->dwFlags & (TTF_FILE+IN_SYSTEM)) == (TTF_FILE+IN_SYSTEM) )
      {
       dprintf(( "Deleting older source file '%s'\r\n", pfdSource->szFullPath ));
       if( !DeleteFile( pfdSource->szFullPath ) )
        {
         dprintf(( "*** couldn't delete '%s', le = #%d\r\n", pfdSource->szFullPath, GetLastError() ));
        }
      }
     else
      {
       dprintf(( "Ini file not SYSTEM dir, not deleting\r\n" ));
      }

    return TRUE;   // return "source was older"
   }


  dprintf(( "Removing existing font resource\r\n" ));

  while( RemoveFontResource(fdExisting.szIniName) );   // Clear all instances
  while( RemoveFontResource(fdExisting.szFullPath) );
  while( RemoveFontResource(fdExisting.pszFilePart) );


  // If existing font is in fonts or system directory, delete it. But make
  //  make sure that the file we're deleting isn't the source file.

  if( fdExisting.dwFlags & (IN_SYSTEM+IN_FONTS)                   &&
      lstrcmpi(fdExisting.szFullPath,pfdSource->szFullPath)  != 0    )
   {
    dprintf(( "Deleting existing file '%s'\r\n", fdExisting.szFullPath ));
    if( !DeleteFile( fdExisting.szFullPath ) )
     {
      dprintf(( "*** error deleting '%s', le = #%d\r\n", fdExisting.szFullPath, GetLastError() ));
     }
   }

  return FALSE;   // return "source was newer or not existing"
 }


//*****************************************************************************
//*******************   P R O C E S S   I N I   L I N E   *********************
//*****************************************************************************

void ProcessIniLine( PSZ pszDescription, PSZ pszIniFile )
 {
  FILEDETAILS fdSource;
  FILEDETAILS fdFOT;

  char  szRegFile[MAX_PATH];


  dprintf(( "ProcessIniLine\r\n" ));
  dprintf(( "  pszDescription = '%s'\r\n", pszDescription ));
  dprintf(( "  pszIniFile     = '%s'\r\n", pszIniFile     ));

  fdSource.dwFlags = 0;
  fdFOT.dwFlags    = 0;

//--------------------  Get details on file in WIN.INI  -----------------------

  GetFileDetails( pszIniFile, &fdSource, MODE_WIN31 );
  if( fdSource.dwFlags == 0 )
   {
    // This is bad, it means we couldn't find the font file referenced
    //  in WIN.INI. That means that gdi probably won't find it either but
    //  to be extra safe I'll just copy the ini file entry to the registry
    //  so that we get the same bootup behavior as before.

////    dprintf(( "  Error getting file details, copying to registry\r\n" ));
////    DorkOnRegistry( pszDescription, pszIniFile );

    dprintf(( "  Couldn't find source file, nuking WIN.INI entry\r\n" ));
    WriteProfileString( "Fonts", pszDescription, NULL );

    return;
   }

  if( fdSource.dwFlags & FOT_FILE )
   {
    dprintf(( "  its an FOT file, getting TTF information\r\n" ));

    fdFOT = fdSource;
    GetFileDetails( &Buffer[0x400], &fdSource, MODE_WIN31 );

    //  If the ttf file for this fot file isn't found then simply
    //    use the fot file. This will emulate win31 behavior.
    //
    //  03/25/95 - mikegi
    //
    //  When the ttf file installed 'in place' on some sort of
    //    removable disk, then we'll use the fot file. Do this
    //    because we can't guarantee that the ttf file will be
    //    available on subsequent boots.
    //
    //  04/20/95 - mikegi

    if( fdSource.dwFlags    == 0           ||     // ttf file not found or
        fdSource.uDriveType != DRIVE_FIXED    )   //   is on non-fixed media
     {
      dprintf(( "  couldn't find ttf file or its on removable media" ));
      dprintf(( "    installing fot file" ));

      fdSource = fdFOT;
      fdFOT.dwFlags = 0;
     }
   }

//--------------  See if entry already exists in the registry  ----------------

  {
   LONG  lrc;
   DWORD dwExisting;
   char  szExisting[MAX_PATH];


   dprintf(( "Calling RegQueryValueEx('%s')\r\n", pszDescription ));

   dwExisting = sizeof(szExisting);
   lrc = RegQueryValueEx( hKeyFonts, pszDescription, NULL, NULL, szExisting, &dwExisting );
   dprintf(( "  lrc = %d\r\n", lrc ));

   if( lrc == ERROR_SUCCESS )
    {
     dprintf(( "Entry already exists in registry\r\n" ));
     dprintf(( "  szExisting = '%s'\r\n", szExisting ));

     if( HandleExisting(szExisting,&fdSource) ) goto CleanupSource;
    }
  }

//---------------------  Copy file to fonts directory  ------------------------

  if( fdSource.dwFlags & IN_SYSTEM )
    {
     dprintf(( "  Copying source from SYSTEM directory to FONTS folder\r\n" ));

     if( !(fdSource.dwFlags & FON_FILE) )
       {
        lstrcpy( szRegFile, szFontsDir );
        AppendFileName( szRegFile, fdSource.pszFilePart );
      
        dprintf(( "    Checking for existing file in FONTS folder\r\n" ));
      
        if( HandleExisting(szRegFile,&fdSource) )
          {
           // file exists in fonts dir, but no registry entry for it
           dprintf(( "  already in FONTS directory, just adding registry entry" ));
          }
         else
          {
           dprintf(( "    Moving file: '%s' to '%s'\r\n", fdSource.szFullPath, szRegFile ));
           if( !MoveFile( fdSource.szFullPath, szRegFile ) )
            {
             dprintf(( "*** MoveFile failed! rc = %d\r\n", GetLastError() ));
            }
          }
       }
      else
       {
        dprintf(( "  leaving .FON in SYSTEM directory\r\n" ));
       }

     lstrcpy( szRegFile, fdSource.pszFilePart );
    }
   else if( fdSource.dwFlags & IN_FONTS )
    {
     dprintf(( "  Leaving in FONTS dir: '%s'\r\n", fdSource.szFullPath ));
     lstrcpy( szRegFile, fdSource.pszFilePart );
    }
   else
    {
     dprintf(( "  Installing file in place: '%s'\r\n", fdSource.szFullPath ));
     lstrcpy( szRegFile, fdSource.szFullPath );
    }

  DorkOnRegistry( pszDescription, szRegFile );


//-----------------------  Delete source FOT file  ----------------------------

CleanupSource:
  dprintf(( "  Nuking WIN.INI entry: '%s'\r\n", pszDescription ));
  WriteProfileString( "Fonts", pszDescription, NULL );

  if( fdFOT.dwFlags & (FOT_FILE+IN_SYSTEM) == FOT_FILE+IN_SYSTEM )
   {
    dprintf(( "  Deleting '%s'\r\n", fdFOT.szFullPath ));
    if( !DeleteFile( fdFOT.szFullPath ) )
     {
      dprintf(( "*** couldn't delete '%s', le = #%d\r\n", fdFOT.szFullPath, GetLastError() ));
     }
   }
 }


//*****************************************************************************
//***********************   G E T   S E C T I O N   ***************************
//*****************************************************************************

int GetSection( LPSTR lpFile, LPSTR lpSection, LPHANDLE hSection)
{
    int    nCount;
    int    nSize;
    HANDLE hLocal, hTemp;
    char *pszSect;

    if( !( hLocal = LocalAlloc( LMEM_MOVEABLE, nSize=4096 ) ) )
        return( 0 );

    //
    //  Now that a buffer exists, Enumerate all LHS of the section.  If the
    //  buffer overflows, reallocate it and try again. 
    //

    do
    {
        pszSect = (PSTR) LocalLock( hLocal );

		if( lpFile )
			nCount = GetPrivateProfileString(lpSection, NULL, "", pszSect,
																		nSize, lpFile);
		else
			nCount = GetProfileString(lpSection, NULL, "", pszSect, nSize);

		LocalUnlock(hLocal);

      if (nCount <= nSize-10)
			break;

		nSize += 2048;
		if (!(hLocal = LocalReAlloc(hTemp=hLocal, nSize, LMEM_MOVEABLE)))
			{
			LocalFree(hTemp);
			return(0);
			}
    } while (1) ;

	*hSection = hLocal;
	return (nCount);
}


//*****************************************************************************
//******************   P R O C E S S   I N I   F I L E   **********************
//*****************************************************************************

void ProcessIt( void )
{
    char	  szFonts[] = "FONTS";
    PSTR	  pszItem;
    HANDLE  hLocalBuf;
    PSTR	  pLocalBuf, pEnd;
    int     nCount;
    char    szPath[ MAX_PATH ];


    GetDirs();

    nCount = GetSection(NULL, szFonts, &hLocalBuf);

    if( !hLocalBuf ) return;

    pLocalBuf = (PSTR) LocalLock(hLocalBuf);
    pEnd = pLocalBuf+nCount;

    // Add all the fonts in the list, if they haven't been added already

    for( pszItem=pLocalBuf; pszItem<pEnd; pszItem+=lstrlen(pszItem)+1)
    {
  	    if( !*pszItem ) continue;

  	    GetProfileString(szFonts, pszItem, "", szPath, sizeof(szPath));

  	    if( *szPath )
        {
            ProcessIniLine( pszItem, szPath );
  	    }
    }

    LocalUnlock( hLocalBuf );
    LocalFree( hLocalBuf );
}


//*****************************************************************************
//*************************   W I N   M A I N   *******************************
//*****************************************************************************

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
 {
  LONG  lrc;
  DWORD dwDisposition;

  lrc = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Fonts", 
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_SET_VALUE, 
                        NULL,
                        &hKeyFonts,
                        &dwDisposition );

  if( lrc == ERROR_SUCCESS )
    {
     ProcessIt();

     dprintf(( "  Flushing fonts key\r\n" ));
     RegFlushKey( hKeyFonts );

     RegCloseKey( hKeyFonts );
    }
   else
    {
     dprintf(( "*** RegCreateKeyEx failed! rc = %d\r\n", GetLastError() ));
    }                                                
                                                     
  return 0;                                          
 }                                                   
                                                     
