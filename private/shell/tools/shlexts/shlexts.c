/****************************** Module Header ******************************\
* Module Name: shlexts.c
*
* Copyright (c) 1997, Microsoft Corporation
*
* This module contains user related debugging extensions.
*
* History:
* 10/28/97 created by cdturner (butchered from the userexts.dll)
\***************************************************************************/

#include <precomp.h>
#pragma hdrstop

#include <winver.h>
#include <shlwapi.h>


char * pszExtName = "SHLEXTS";

#include <stdexts.h>
#include <stdexts.c>

BOOL bShowFlagNames = TRUE;
#define NO_FLAG (LPSTR)0xFFFFFFFF  // use this for non-meaningful entries.

LPSTR apszSFGAOFlags[] =
{
    "SFGAO_CANCOPY",        // 0x00000001L
    "SFGAO_CANMOVE",        // 0x00000002L
    "SFGAO_CANLINK",        // 0x00000004L
    NO_FLAG,
    "SFGAO_CANRENAME",      // 0x00000010L     // Objects can be renamed
    "SFGAO_CANDELETE",      // 0x00000020L     // Objects can be deleted
    "SFGAO_HASPROPSHEET",   // 0x00000040L     // Objects have property sheets
    NO_FLAG,
    "SFGAO_DROPTARGET",     // 0x00000100L     // Objects are drop target
    NO_FLAG,
    NO_FLAG,
    NO_FLAG,
    "SFGAO_LINK",           // 0x00010000L     // Shortcut (link)
    "SFGAO_SHARE",          // 0x00020000L     // shared
    "SFGAO_READONLY",       // 0x00040000L     // read-only
    "SFGAO_GHOSTED",        // 0x00080000L     // ghosted icon
    "SFGAO_NONENUMERATED",  // 0x00100000L     // is a non-enumerated object
    "SFGAO_NEWCONTENT",     // 0x00200000L     // should show bold in explorer tree
    NO_FLAG,
    NO_FLAG,
    "SFGAO_VALIDATE",       // 0x01000000L     // invalidate cached information
    "SFGAO_REMOVABLE",      // 0x02000000L     // is this removeable media?
    "SFGAO_COMPRESSED",     // 0x04000000L     // Object is compressed (use alt color)
    "SFGAO_BROWSABLE",      // 0x08000000L     // is in-place browsable
    "SFGAO_FILESYSANCESTOR",// 0x10000000L     // It contains file system folder
    "SFGAO_FOLDER",         // 0x20000000L     // It's a folder.
    "SFGAO_FILESYSTEM",     // 0x40000000L     // is a file system thing (file/folder/root)
    "SFGAO_HASSUBFOLDER",   // 0x80000000L     // Expandable in the map pane
    NULL
};

LPSTR apszSLDFFlags[] = 
{
   "SLDF_HAS_ID_LIST",      // = 0x0001,   // Shell link saved with ID list
   "SLDF_HAS_LINK_INFO",    // = 0x0002,   // Shell link saved with LinkInfo
   "SLDF_HAS_NAME",         // = 0x0004,
   "SLDF_HAS_RELPATH",      // = 0x0008,
   "SLDF_HAS_WORKINGDIR",   // = 0x0010,
   "SLDF_HAS_ARGS",         // = 0x0020,
   "SLDF_HAS_ICONLOCATION", // = 0x0040,
   "SLDF_UNICODE",          // = 0x0080,   // the strings are unicode (NT is comming!)
   "SLDF_FORCE_NO_LINKINFO",// = 0x0100,   // don't create a LINKINFO (make a dumb link)
   "SLDF_HAS_EXP_SZ"        // = 0x0200,   // the link contains expandable env strings
   "SLDF_RUN_IN_SEPARATE",  // = 0x0400,   // Run the 16-bit target exe in a separate VDM/WOW
   "SLDF_HAS_LOGO3ID",      // = 0x0800,   // this link is a special Logo3/MSICD link
   "SLDF_HAS_DARWINID",     // = 0x1000    // this link is a special Darwin link
   NULL
};

LPSTR apszFWFFlags[] =
{
    "FWF_AUTOARRANGE",          // =  0x0001,
    "FWF_ABBREVIATEDNAMES",     // =  0x0002,
    "FWF_SNAPTOGRID",           // =  0x0004,
    "FWF_OWNERDATA",            // =  0x0008,
    "FWF_BESTFITWINDOW",        // =  0x0010,
    "FWF_DESKTOP",              // =  0x0020,
    "FWF_SINGLESEL",            // =  0x0040,
    "FWF_NOSUBFOLDERS",         // =  0x0080,
    "FWF_TRANSPARENT",          // =  0x0100,
    "FWF_NOCLIENTEDGE",         // =  0x0200,
    "FWF_NOSCROLL",             // =  0x0400,
    "FWF_ALIGNLEFT",            // =  0x0800,
    "FWF_NOICONS",              // =  0x1000,
    "FWF_SINGLECLICKACTIVATE",  // = 0x8000  // TEMPORARY -- NO UI FOR THIS
    NULL
};

LPSTR apszICIFlags[] = 
{
    "ICIFLAG_LARGE",       // 0x0001
    "ICIFLAG_SMALL",       // 0x0002
    "ICIFLAG_BITMAP",      // 0x0004
    "ICIFLAG_ICON",        // 0x0008
    "ICIFLAG_INDEX",       // 0x0010
    "ICIFLAG_NAME",        // 0x0020
    "ICIFLAG_FLAGS",       // 0x0040
    "ICIFLAG_NOUSAGE",     // 0x0080
    NULL
};

LPSTR apszFDFlags[] =
{
    "FD_CLSID",            // = 0x0001,
    "FD_SIZEPOINT",        // = 0x0002,
    "FD_ATTRIBUTES",       // = 0x0004,
    "FD_CREATETIME",       // = 0x0008,
    "FD_ACCESSTIME",       // = 0x0010,
    "FD_WRITESTIME",       // = 0x0020,
    "FD_FILESIZE",         // = 0x0040,
    "FD_LINKUI",           // = 0x8000,       // 'link' UI is prefered
    NULL
};

LPSTR apszSHCNEFlags[] =
{
    "SHCNE_RENAMEITEM",         // 0x00000001L
    "SHCNE_CREATE",             // 0x00000002L
    "SHCNE_DELETE",             // 0x00000004L
    "SHCNE_MKDIR",              // 0x00000008L
    "SHCNE_RMDIR",              // 0x00000010L
    "SHCNE_MEDIAINSERTED",      // 0x00000020L
    "SHCNE_MEDIAREMOVED",       // 0x00000040L
    "SHCNE_DRIVEREMOVED",       // 0x00000080L
    "SHCNE_DRIVEADD",           // 0x00000100L
    "SHCNE_NETSHARE",           // 0x00000200L
    "SHCNE_NETUNSHARE",         // 0x00000400L
    "SHCNE_ATTRIBUTES",         // 0x00000800L
    "SHCNE_UPDATEDIR",          // 0x00001000L
    "SHCNE_UPDATEITEM",         // 0x00002000L
    "SHCNE_SERVERDISCONNECT",   // 0x00004000L
    "SHCNE_UPDATEIMAGE",        // 0x00008000L
    "SHCNE_DRIVEADDGUI",        // 0x00010000L
    "SHCNE_RENAMEFOLDER",       // 0x00020000L
    "SHCNE_FREESPACE",          // 0x00040000L
    NO_FLAG,
    NO_FLAG,
    NO_FLAG,
    "SHCNE_EXTENDED_EVENT",     // 0x04000000L
    "SHCNE_ASSOCCHANGED",       // 0x08000000L
    NULL
};

LPSTR apszSSFFlags[] =
{
    "SSF_SHOWALLOBJECTS",       // 0x0001
    "SSF_SHOWEXTENSIONS",       // 0x0002
    "SSF_HIDDENFILEEXTS",       // 0x0004  // ;Internal - corresponding SHELLSTATE fields don't exist in SHELLFLAGSTATE
    "SSF_SHOWCOMPCOLOR",        // 0x0008
    "SSF_SORTCOLUMNS",          // 0x0010  // ;Internal - corresponding SHELLSTATE fields don't exist in SHELLFLAGSTATE
    "SSF_SHOWSYSFILES",         // 0x0020
    "SSF_DOUBLECLICKINWEBVIEW", // 0x0080
    "SSF_SHOWATTRIBCOL",        // 0x0100
    "SSF_DESKTOPHTML",          // 0x0200
    "SSF_WIN95CLASSIC",         // 0x0400
    "SSF_DONTPRETTYPATH",       // 0x0800
    "SSF_MAPNETDRVBUTTON",      // 0x1000
    "SSF_SHOWINFOTIP",          // 0x2000
    "SSF_HIDEICONS",            // 0x4000
    "SSF_NOCONFIRMRECYCLE",     // 0x8000
    NULL
};

enum GF_FLAGS {
    GL_SFGAO = 0,
    GL_SLDF,
    GL_FWF,
    GL_ICI,
    GL_FD,
    GL_SHCNE,
    GL_SSF,
    GF_MAX,
};

struct _tagFlags
{
    LPSTR * apszFlags;
    LPSTR pszFlagsname;
} argFlag[GF_MAX] = 
{
    {apszSFGAOFlags,    "SFGAO"},
    {apszSLDFFlags,     "SLD"},
    {apszFWFFlags,      "FWF"},
    {apszICIFlags,      "ICIFLAG"},
    {apszFDFlags,       "FD"},
    {apszSHCNEFlags,    "SHCNE"},
    {apszSSFFlags,      "SSF"}
};

/************************************************************************\
* Procedure: GetFlags
*
* Description:
*
* Converts a 32bit set of flags into an appropriate string.
* pszBuf should be large enough to hold this string, no checks are done.
* pszBuf can be NULL, allowing use of a local static buffer but note that
* this is not reentrant.
* Output string has the form: "FLAG1 | FLAG2 ..." or "0"
*
* Returns: pointer to given or static buffer with string in it.
*
* 6/9/1995  Created SanfordS
* 11/5/1997 cdturner changed the aapszFlag type 
*
\************************************************************************/
LPSTR GetFlags(
    WORD    wType,
    DWORD   dwFlags,
    LPSTR   pszBuf,
    BOOL    fPrintZero)
{
    static char szT[512];
    WORD i;
    BOOL fFirst = TRUE;
    BOOL fNoMoreNames = FALSE;
    LPSTR *apszFlags;

    if (pszBuf == NULL) {
        pszBuf = szT;
    }
    if (!bShowFlagNames) {
        sprintf(pszBuf, "%x", dwFlags);
        return pszBuf;
    }

    *pszBuf = '\0';

    if (wType >= GF_MAX) {
        strcpy(pszBuf, "Invalid flag type.");
        return pszBuf;
    }

    apszFlags = argFlag[wType].apszFlags;

    for (i = 0; dwFlags; dwFlags >>= 1, i++) {
        if (!fNoMoreNames && apszFlags[i] == NULL) {
            fNoMoreNames = TRUE;
        }

        if (dwFlags & 1) {
            if (!fFirst) {
                strcat(pszBuf, " | ");
            } else {
                fFirst = FALSE;
            }

            if (fNoMoreNames || apszFlags[i] == NO_FLAG) {
                char ach[16];
                sprintf(ach, "0x%lx", 1 << i);
                strcat(pszBuf, ach);
            } else {
                strcat(pszBuf, apszFlags[i]);
            }
        }
    }

    if (fFirst && fPrintZero) {
        sprintf(pszBuf, "0");
    }

    return pszBuf;
}

/************************************************************************\
* Procedure: Iflags
*
* Description:
*
*     outputs the list of flags for the given flags type
*
* 11/5/1997 Created cdturner
*
\************************************************************************/
BOOL Iflags( DWORD dwOpts,
             LPSTR pszArgs )
{
    CHAR szBuffer[100];
    int iOffset = 0;
    int iFlags;
    LPDWORD pAddr;
    BOOL bAddr = FALSE;
    DWORD dwValue;
    LPSTR pszOut;
    
    if ( dwOpts & OFLAG(l))
    {
        // list all the struct names
        Print("Flags types known:\n");

        for ( iFlags = 0; iFlags < GF_MAX; iFlags ++ )
        {
            sprintf( szBuffer, "    %s\n", argFlag[iFlags].pszFlagsname);
            Print( szBuffer );
        }
        return TRUE;
    }

    // skip whitespace
    while ( *pszArgs == ' ' )
        pszArgs ++;

    // now grab the flagsname
    while ( pszArgs[iOffset] != ' ' && pszArgs[iOffset] != '\0' )
    {
        szBuffer[iOffset] = pszArgs[iOffset];
        iOffset ++;
    };

    // terminate the string
    szBuffer[iOffset] = 0;
    
    // find the flags value
    for ( iFlags = 0; iFlags < GF_MAX; iFlags ++ )
    {
        if ( lstrcmpA( szBuffer, argFlag[iFlags].pszFlagsname ) == 0 )
            break;
    }

    if ( iFlags >= GF_MAX )
    {
        Print( "unknown flagsname - ");
        Print( szBuffer );
        Print( "\n" );
        return TRUE;
    }

    // skip white space
    while ( pszArgs[iOffset] == ' ' )
        iOffset ++;

    if ( pszArgs[iOffset] == '*' )
    {
        bAddr = TRUE;
        iOffset ++;
    }
    
    pAddr = (LPDWORD) EvalExp( pszArgs + iOffset );

    if ( bAddr )
    {
        if ( !tryDword( &dwValue, pAddr ) )
        {
            Print( "unable to access memory at that location\n");
            return TRUE;
        }
    }
    else 
    {
        dwValue = PtrToUlong(pAddr);
    }
    
    pszOut = GetFlags( (WORD) iFlags, dwValue, NULL, TRUE ); 
    if ( pszOut )
    {
        sprintf( szBuffer, "Value = %8X, pAddr = %8X\n", dwValue, (DWORD_PTR)pAddr );
        Print( szBuffer );
        Print( pszOut );
        Print( "\n" );
    }
    
    return TRUE;
}

/************************************************************************\
* Procedure: Itest
*
* Description: Tests the basic stdexts macros and functions - a good check
*   on the debugger extensions in general before you waste time debuging
*   entensions.
*
* Returns: fSuccess
*
* 11/4/1997 Created cdturner
*
\************************************************************************/
BOOL Itest()
{
    Print("Print test!\n");
    SAFEWHILE(TRUE) 
    {
        Print("SAFEWHILE test...  Hit Ctrl-C NOW!\n");
    }
    return TRUE;
}



/************************************************************************\
* Procedure: Iver
*
* Description: Dumps versions of extensions and winsrv/win32k
*
* Returns: fSuccess
*
* 11/4/1997 Created cdturner
*
\************************************************************************/
BOOL Iver()
{
#if DEBUG
    Print("SHLEXTS version: Debug.\n");
#else
    Print("SHLEXTS version: Retail.\n");
#endif

    return TRUE;
}


/************************************************************************\
* Procedure: Ifilever
*
* Description: Dumps versions of extensions and winsrv/win32k
*
* Returns: fSuccess
*
* 11/4/1997 Created cdturner
*
\************************************************************************/
BOOL Ifilever( DWORD dwOpts,
             LPSTR pszArgs )
{
    HINSTANCE hDll = NULL;
    DLLGETVERSIONPROC pGetVer = NULL;
    DWORD dwHandle;
    DWORD dwBlockLen;
    LPVOID pBlock = NULL;
    char szMessage[200];
    BOOL fSkipLoad = FALSE;
    
    
    // do they want the speech on shell versions ....
    if ( pszArgs == NULL || lstrlenA( pszArgs ) == 0 )
    {
        pszArgs = "Shell32.dll";
    }

    if ( !(dwOpts & OFLAG(n)) )
    {
        hDll = LoadLibraryA(pszArgs);
        if ( hDll == NULL )
        {
            Print("ERROR: Can't Load ");
            Print(pszArgs);
            Print("\n");
            
            return TRUE;
        }
        pGetVer = (DLLGETVERSIONPROC) GetProcAddress( hDll, "DllGetVersion");
        if ( pGetVer )    
        {
            DLLVERSIONINFO rgVerInfo;

            rgVerInfo.cbSize = sizeof( rgVerInfo );
            
            pGetVer( &rgVerInfo );
            
            wsprintfA( szMessage, "DllGetVersion for %s Reports:\n    Major = %d\n    Minor = %d\n    Build = %d\n",
                pszArgs, rgVerInfo.dwMajorVersion, rgVerInfo.dwMinorVersion, rgVerInfo.dwBuildNumber );
                
            Print(szMessage );
        }
        FreeLibrary( hDll );
    }
    
    // now test the normal version details...
    dwBlockLen = GetFileVersionInfoSizeA( pszArgs, &dwHandle );
    if ( dwBlockLen == 0 )
    {
        Print("GetFileVersionSize(");
        Print(pszArgs);
        Print(" failed\n");

        return TRUE;
    }

    pBlock = LocalAlloc( LPTR, dwBlockLen );
    if ( pBlock )
    {
        if (GetFileVersionInfoA( pszArgs, dwHandle, dwBlockLen, pBlock ))
        {
            VS_FIXEDFILEINFO * pFileInfo;
            UINT uLen;
        
            VerQueryValueA( pBlock, "\\", (LPVOID *) &pFileInfo, &uLen );
            wsprintfA( szMessage, "GetFileVersionInfo for %s Reports:\n    Major = %X\n    Minor = %d.%d\n",
                pszArgs,
                (pFileInfo->dwFileVersionMS & 0xf0000) >> 16,
                (pFileInfo->dwFileVersionLS & 0xffff0000) >> 16,
                (pFileInfo->dwFileVersionLS & 0xffff));
            Print( szMessage );
        }
        LocalFree( pBlock );
    }
    return TRUE;
}
