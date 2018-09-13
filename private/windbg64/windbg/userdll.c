#include "precomp.h"
#pragma hdrstop

//
//  Externals
//
#include "include\cntxthlp.h"

///////////////////////////////////////////////////////////////////////
//
//              Module List stuff
//
//  This list contains the default symbol load actions for all
//  modules. It is kept in the shell so it can be saved and loaded
//  from a workspace and modified by the user before loading any
//  debugger DLL.
//
///////////////////////////////////////////////////////////////////////


//
// Helper functions
//
BOOL
TruncatePathsToMaxPath(
    PCSTR pszOriginal,
    PSTR & pszTruncated
    )
/*++
Description:    
    Used to truncate a series of paths to MAX_PATH.

    Each path is delimited by a semicolon: e:\;f:\hello;c:\dos

    NOTE: Allocation done with malloc.

Args:
    pszOriginal - Pointer to string to be validated. Cannot be NULL.
    pszTruncated - Cannot be NULL. Destination of the newly allocated
        string with all of the truncated paths. See Return values...

Returns:
    TRUE - 1 or more of the individual paths were longer that MAX_PATH.
        pszTruncated contains munged paths. The calling routine must free
        the allocated memory.
    FALSE - All of the individual path were shorter than MAX_PATH.
        pszTruncated is NULL. NO changes were made
--*/
{
    Assert(pszOriginal);
    Assert(&pszTruncated);

    BOOL bTruncation = FALSE;

    // Place holder for the truncated paths
    pszTruncated = (PSTR) calloc(strlen(pszOriginal) +1, 1);
    Dbg(pszTruncated);

    // Individual Paths
    PSTR pszBuffer = _strdup(pszOriginal);
    Dbg(pszBuffer);

    PSTR pszToken = strtok(pszBuffer, ";");
    while(pszToken) {
        if (strlen(pszToken) >= MAX_PATH) {
            pszToken[MAX_PATH-1] = NULL;
            bTruncation = TRUE;
        }
        strcat(pszTruncated, pszToken);

        pszToken = strtok(NULL, ";");
    }

    //
    // cleanup
    //
    free(pszBuffer);

    if (!bTruncation) {
        // No need to truncate
        free(pszTruncated);
        pszTruncated = NULL;
    }

    return bTruncation;
}

BOOL
IsLenOfIndivPathValid(
    PCSTR pszOriginal,
    BOOL bWarn
    )
/*++
Description:    
    Used to validate a series of paths to make sure that each individual
    path is length than MAX_PATH.

    Each path is delimited by a semicolon: e:\;f:\hello;c:\dos

Args:
    pszOriginal - Pointer to string to be validated. Cannot be NULL.
    bWarn - Indicates whether to issue the standard message box warning

Returns:
    TRUE - All of the individual path were shorter than MAX_PATH
    FALSE - 1 or more of the individual paths were longer that MAX_PATH
--*/
{
    Assert(pszOriginal);

    BOOL bAllShorterThanMaxPath = TRUE;

    // Individual Paths
    PSTR pszBuffer = _strdup(pszOriginal);

    PSTR pszToken = strtok(pszBuffer, ";");
    while(pszToken) {
        if (strlen(pszToken) >= MAX_PATH) {            
            bAllShorterThanMaxPath = FALSE;
            if (bWarn) {
                pszToken[MAX_PATH-4] = NULL;
                strcat(pszToken, "...");
                InformationBox(ERR_Path_Too_Long, MAX_PATH-1, pszToken);        
            }
            break;
        }
        pszToken = strtok( NULL, ";");
    }

    free(pszBuffer);
    return bAllShorterThanMaxPath;
}



//
//  How to search for a module
//
typedef enum _MATCHKIND {
    MATCH_FULLPATH,
    MATCH_FILENAME,
    MATCH_BASENAME
} MATCHKIND;

//
//  These flags tell us what to do with the module
//
#define MOD_FLAG_NONE       0x00000000
#define MOD_FLAG_TO_ADD     0x00000001
#define MOD_FLAG_TO_DELETE  0x00000002
#define MOD_FLAG_TO_MODIFY  0x00000004

//
//  Node in the Module load list
//
typedef struct _MOD_NODE  *PMOD_NODE;
typedef struct _MOD_NODE {
    PMOD_NODE   Prev;
    PMOD_NODE   Next;
    CHAR        Name[ MAX_PATH ];
    SHE         SheDefault;
    SHE         SheCurrent;
    SHE         SheNew;
    DWORD       Flags;
    BOOL        fLoaded;
} MOD_NODE;

static PMOD_NODE    ModListHead = NULL;
static SHE          SheDefault  = sheNone;


INT         ModMatch( CHAR *, CHAR *, MATCHKIND );
PMOD_NODE   ModFind( CHAR *, MATCHKIND, BOOL );
PMOD_NODE   ModFindIndex( DWORD, BOOL );
INT         ModIndex( PMOD_NODE );
VOID        ModAdd( PMOD_NODE );
PMOD_NODE   ModNameAdd( CHAR *, SHE );
VOID        ModDel( PMOD_NODE );




INT
ModMatch (
    CHAR        *Name1,
    CHAR        *Name2,
    MATCHKIND   MatchKind
    )
{
    INT     Match;
    char    Base1[ _MAX_FNAME ];
    char    Base2[ _MAX_FNAME ];
    char    Ext1[ _MAX_EXT ];
    char    Ext2[ _MAX_EXT ];

    Assert( Name1 );
    Assert( Name2 );

    if ( MatchKind == MATCH_FULLPATH ) {

        Match = _stricmp( Name1, Name2 );

    } else {

        _splitpath( Name1, NULL, NULL, Base1, Ext1 );
        _splitpath( Name2, NULL, NULL, Base2, Ext2 );

        Match = _stricmp( Base1, Base2 );

        if ( !Match && MatchKind == MATCH_FILENAME ) {
            Match = _stricmp( Ext1, Ext2 );
        }
    }

    return Match;
}


PMOD_NODE
ModFind(
    CHAR       *Name,
    MATCHKIND   MatchKind,
    BOOL        IgnoreFlags
    )
{
    PMOD_NODE   ModNode = ModListHead;

    Assert(Name != NULL);
    if (Name == NULL) {
        return NULL;
    }

    while( ModNode ) {
        if ( IgnoreFlags || !(ModNode->Flags & MOD_FLAG_TO_ADD) ) {
            if ( !ModMatch( ModNode->Name, Name, MatchKind ) ) {
                break;
            }
        }
        ModNode = ModNode->Next;
    }

    return ModNode;
}


PMOD_NODE
ModFindIndex(
    DWORD   Index,
    BOOL    UseFlag
    )
{
    PMOD_NODE   ModNode;
    DWORD       i;

    ModNode = ModListHead;
    i       = 0;

    while ( ModNode ) {
        if ( !UseFlag || !(ModNode->Flags & MOD_FLAG_TO_DELETE) ) {
            i++;
        }

        if ( i > Index ) {
            break;
        }

        ModNode = ModNode->Next;
    }

    return ModNode;
}


INT
ModIndex(
    PMOD_NODE   ModNode
    )
{
    PMOD_NODE   ModNodeTmp;
    INT         Index;

    Index       = 0;
    ModNodeTmp  = ModListHead;

    while ( ModNodeTmp ) {

        if ( ModNodeTmp == ModNode ) {
            break;
        }

        Index++;
        ModNodeTmp = ModNodeTmp->Next;
    }

    if ( !ModNodeTmp ) {
        Index = -1;
    }

    return Index;
}


VOID
ModAdd(
    PMOD_NODE   ModNode
    )
{
    PMOD_NODE   Mod     = ModListHead;
    PMOD_NODE   ModPrev = NULL;

    while ( Mod && ModMatch( ModNode->Name, Mod->Name, MATCH_FILENAME ) > 0 ) {
        ModPrev = Mod;
        Mod     = Mod->Next;
    }

    ModNode->Prev = ModPrev;
    ModNode->Next = Mod;

    if ( ModPrev ) {
        ModPrev->Next = ModNode;
    } else {
        ModListHead   = ModNode;
    }

    if ( Mod ) {
        Mod->Prev = ModNode;
    }
}

PMOD_NODE
ModNameAdd(
    CHAR    *Name,
    SHE      SheDefault
    )
{
    PMOD_NODE   ModNode;

    Assert( Name );

    ModNode = (PMOD_NODE)calloc(sizeof(MOD_NODE), 1);
    Dbg(ModNode);

    if ( ModNode ) {
        
        Dbg(strlen(Name) < sizeof(ModNode->Name));

        // No need to null terminate since we used calloc
        strncpy(ModNode->Name, Name, sizeof(ModNode->Name)-1);

        ModNode->Prev       = NULL;
        ModNode->Next       = NULL;
        ModNode->SheDefault = SheDefault;
        ModNode->SheCurrent = SheDefault;
        ModNode->Flags      = MOD_FLAG_NONE;
        ModNode->fLoaded    = FALSE;

        ModAdd( ModNode );
    }

    return ModNode;
}



VOID
ModDel(
    PMOD_NODE   ModNode
    )
{
    if ( ModNode->Prev ) {
        ModNode->Prev->Next = ModNode->Next;
    } else {
        ModListHead = ModNode->Next;
    }

    if ( ModNode->Next ) {
        ModNode->Next->Prev = ModNode->Prev;
    }

    free( ModNode );
}









///////////////////////////////////////////////////////////////////////
//
//              External interface
//
///////////////////////////////////////////////////////////////////////





VOID
ModListSetDefaultShe (
    SHE     She
    )
{
    SheDefault = She;
}


BOOL
ModListGetDefaultShe(
    TCHAR * pszName,
    SHE *  pShe
    )
/*++
Description:
    Get the default SHE, or the SHE for a particular file.

Args:
    pszName - if NULL, the default SHE is returned.
            The name of a file, that will be matched in a
            list to find it's particular SHE. 

    pShe - Can't be NULL. Pointer to a SHE, that will hold the 
            return value.

Return:
    TRUE - Success. Valid SHE.
    FALSE - Failure. SHE value is undefined.
--*/
{
    BOOL        bRet = FALSE;
    PMOD_NODE   pModNode;

    Assert( pShe );

    if ( pszName ) {

        pModNode = ModFind( pszName, MATCH_FULLPATH, FALSE );
        if ( !pModNode ) {
            pModNode = ModFind( pszName, MATCH_FILENAME, FALSE );
            if ( !pModNode ) {
                pModNode = ModFind( pszName, MATCH_BASENAME, FALSE );
            }
        }

        if ( pModNode ) {
            *pShe = pModNode->SheDefault;
            bRet  = TRUE;
        }

    } else {

        *pShe = SheDefault;
        bRet  = TRUE;
    }

    return bRet;
}


VOID
ModListInit(
    VOID
    )
{
    PMOD_NODE   ModNode;
    PMOD_NODE   ModNodeTmp;

    ModNode = ModListHead;
    while( ModNode ) {
        ModNodeTmp  = ModNode;
        ModNode     = ModNode->Next;
        ModDel( ModNodeTmp );
    }

    ModListHead = NULL;

    FREE_STR( g_contPaths_WkSp.m_pszSymPath );
}


VOID
ModListAdd(
    CHAR    *Name,
    SHE      SheDefault
    )
{
    ModNameAdd( Name, SheDefault );
}


VOID
ModListModLoad(
    TCHAR    *Name,
    SHE     SheCurrent
    )
{
    Assert(Name);
    PMOD_NODE   ModNode;

    ModNode = ModFind( Name, MATCH_FULLPATH, FALSE );
    if ( !ModNode ) {
        ModNode = ModFind( Name, MATCH_FILENAME, FALSE );
        if ( !ModNode ) {
            ModNode = ModFind( Name, MATCH_BASENAME, FALSE );
        }
    }

    if ( !ModNode ) {
        ModNode = ModNameAdd( Name, SheDefault );
    }

    if ( ModNode ) {
        ModNode->SheCurrent = SheCurrent;
        ModNode->fLoaded    = TRUE;
    }
}

VOID
ModListModUnload(
    TCHAR    *Name
    )
{
    Assert(Name);

    PMOD_NODE   ModNode;

    ModNode = ModFind( Name, MATCH_FULLPATH, FALSE );
    if ( ModNode ) {
        ModNode->SheCurrent = sheNone;
        ModNode->fLoaded    = TRUE;
    }
}

PVOID
ModListGetNext(
    PVOID   Previous,
    TCHAR   *Name,
    SHE    *SheDefault
    )
{
    PMOD_NODE   ModNode;

    if ( !Previous ) {
        ModNode = ModListHead;
    } else {
        ModNode = ((PMOD_NODE)Previous)->Next;
    }

    if ( ModNode ) {
        strcpy( Name, ModNode->Name );
        *SheDefault = ModNode->SheDefault;
    }

    return (PVOID)ModNode;
}


VOID
ModListSetSearchPath(
    CHAR    *Path
    )
{
    FREE_STR( g_contPaths_WkSp.m_pszSymPath );

    ModListAddSearchPath( Path );
}

VOID
ModListAddSearchPath(
    TCHAR    *pszPath
    )
{
    size_t  uNewPathLen;

    Assert( pszPath );

    uNewPathLen = strlen( pszPath );

    if ( !g_contPaths_WkSp.m_pszSymPath ) {
        g_contPaths_WkSp.m_pszSymPath = _strdup(pszPath);
    } else {
        size_t uOldPathLen = _tcslen(g_contPaths_WkSp.m_pszSymPath);
        
        // +1 for ";" and +1 for NULL terminator
        PSTR psz = (TCHAR *) realloc( 
                                    g_contPaths_WkSp.m_pszSymPath, 
                                    (uOldPathLen + uNewPathLen +2) * sizeof(TCHAR) 
                                    );
        if (psz) {
            g_contPaths_WkSp.m_pszSymPath = psz;

            if (';' != g_contPaths_WkSp.m_pszSymPath[uOldPathLen -1] 
                && ';' != *pszPath) {

                _tcscat( g_contPaths_WkSp.m_pszSymPath, ";" );

            }
            
            strcat( g_contPaths_WkSp.m_pszSymPath, pszPath );
        }
    }


    if (g_contPaths_WkSp.m_pszSymPath) {
        PTSTR pszTruncated = NULL;
        if (TruncatePathsToMaxPath(g_contPaths_WkSp.m_pszSymPath, pszTruncated)) {
            // Free the previous string and then copy the new truncated one in.
            FREE_STR(g_contPaths_WkSp.m_pszSymPath);
            g_contPaths_WkSp.m_pszSymPath = pszTruncated;
        }
    }
}

DWORD
ModListGetSearchPath(
    CHAR    *Buffer,
    DWORD   Size
    )
/*++
    Buffer - If NULL:
                No search path - return 0
                else, simply returns the string length +1 (null terminator)
             else,
                Will copy the search path into the buffer, truncating and appending
                    a 0, if necessary.

    Size - Size of the buffer. Can only be 0 if Buffer is NULL.

Returns
    Number of chars copied, or the length of the search path if buffer is NULL. Value
    include 0 terminator at end.
--*/
{
    DWORD   Len = 0;

    if ( g_contPaths_WkSp.m_pszSymPath ) {

        Len = (DWORD)strlen(g_contPaths_WkSp.m_pszSymPath);

        if ( Buffer ) {

            if ( Size > Len ) {
                strcpy( Buffer, g_contPaths_WkSp.m_pszSymPath );
                Len++;
            } else {
                strncpy( Buffer, g_contPaths_WkSp.m_pszSymPath, Size-1 );
                Buffer[Size-1] = '\0';
                Len = Size;
            }

        } else {

            Len++;
        }
    }

    return Len;
}



///////////////////////////////////////////////////////////////////////
//
//              User DLL Dialog stuff
//
///////////////////////////////////////////////////////////////////////


SHE
UserDllGetShe(
    HWND    hwndDlg
    )
{
    if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, ID_USERDLL_LOAD)) {
        return sheNone;
    } else if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, ID_USERDLL_DEFER)) {
        return sheDeferSyms;
    } else {
        Assert(!"Should never happen");
        // Just in case he presses ignore
        return sheNone;
    }
}

VOID
Symbols_UserDllListInit(
    HWND    hwndDlg
    )
{
    SHE she;

    // Get the default SHE. 
    Assert(ModListGetDefaultShe(NULL, &she));

    switch (she) {
    case sheNone:
        CheckDlgButton(hwndDlg, ID_USERDLL_LOAD, BST_CHECKED);
        break;

    case sheDeferSyms:
    case sheSuppressSyms:
    default:
        CheckDlgButton(hwndDlg, ID_USERDLL_DEFER, BST_CHECKED);
        break;
    }

    {
        int nLen = ModListGetSearchPath( NULL, 0 );
        PSTR pszPath = (PSTR) calloc(1, nLen +2);
        Assert(pszPath);

        ModListGetSearchPath( pszPath, nLen );
        SetDlgItemText(hwndDlg, IDC_EDIT_SEARCH_PATH, pszPath);

        free(pszPath);
    }

    CheckRadioButton(
        hwndDlg,
        ID_DBUGOPT_SUFFIXA,
        ID_DBUGOPT_SUFFIXNONE,
        (SuffixToAppend == 'A') ? ID_DBUGOPT_SUFFIXA :
        (SuffixToAppend == 'W') ? ID_DBUGOPT_SUFFIXW :
        ID_DBUGOPT_SUFFIXNONE );

    //
    // Error handling on sym loads
    //
    {
        int nId;

        if (g_contWorkspace_WkSp.m_bIgnoreAllSymbolErrors) {
            nId = IDC_RADIO_IGNORE_ERRORS;
        } else {
            if (g_contWorkspace_WkSp.m_bBrowseForSymsOnSymLoadErrors) {
                nId = IDC_RADIO_PROMPT_ON_ERROR;
            } else {
                nId = IDC_RADIO_NEVER_LOAD_BAD_SYMBOLS;
            }
        }

        CheckRadioButton(hwndDlg, IDC_RADIO_IGNORE_ERRORS, IDC_RADIO_NEVER_LOAD_BAD_SYMBOLS, nId);
    }
}

INT_PTR
CALLBACK 
DlgProc_Symbols(
    HWND hwndDlg, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    static DWORD HelpArray[]=
    {
       ID_USERDLL_LOAD, IDH_LOAD,
       ID_USERDLL_DEFER, IDH_DEFER,
       ID_DBUGOPT_SUFFIXA, IDH_ANSI,
       ID_DBUGOPT_SUFFIXW, IDH_UNICODE,
       ID_DBUGOPT_SUFFIXNONE, IDH_NOSUFFIX,
       IDC_EDIT_SEARCH_PATH, IDH_SYMPATH,
       IDC_RADIO_NEVER_LOAD_BAD_SYMBOLS, IDH_BADSYM_NOLOAD,
       IDC_RADIO_PROMPT_ON_ERROR, IDH_BADSYM_PROMPT,
       IDC_RADIO_IGNORE_ERRORS, IDH_BADSYM_IGNORE,
       0, 0
    };

    switch (uMsg) {
    case WM_INITDIALOG:
        Symbols_UserDllListInit(hwndDlg);
        return FALSE;
        
    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, "windbg.hlp", HELP_WM_HELP,
            (DWORD_PTR)(LPVOID) HelpArray );
        return TRUE;
        
    case WM_CONTEXTMENU:
        WinHelp ((HWND) wParam, "windbg.hlp", HELP_CONTEXTMENU,
            (DWORD_PTR)(LPVOID) HelpArray );
        return TRUE;
        
    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code) {
        case PSN_KILLACTIVE:
            {
                BOOL bSuccess = TRUE;
                
                HWND hwndSrchPath = GetDlgItem(hwndDlg, IDC_EDIT_SEARCH_PATH);
                Assert(hwndSrchPath);
                
                int nLen = GetWindowTextLength(hwndSrchPath) +1;
                
                PSTR pszSrcPaths = (PSTR) calloc(1, nLen);
                GetWindowText(hwndSrchPath, pszSrcPaths, nLen);
                
                if (IsLenOfIndivPathValid(pszSrcPaths, TRUE)) {
                    // Ok
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, FALSE);
                } else {
                    // Error
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                }
            }
            return TRUE;
            
        case PSN_APPLY:
            {
                PMOD_NODE   pModNode;
                SHE         She;
                char        ch;
                int         nUpdate = UPDATE_NONE;
                
                She = UserDllGetShe(hwndDlg);
                ModListSetDefaultShe(She);
                
                // Change the defaults for all of the DLLs
                for ( pModNode = ModListHead; pModNode; pModNode = pModNode->Next ) {
                    if ( pModNode->Flags & MOD_FLAG_TO_DELETE ) {
                        ModDel( pModNode );
                    } else {
                        pModNode->SheDefault = pModNode->SheNew = She;
                        pModNode->Flags = MOD_FLAG_NONE;
                    }
                }
                
                {
                    HWND hwndSrchPath = GetDlgItem(hwndDlg, IDC_EDIT_SEARCH_PATH);
                    Assert(hwndSrchPath);
                    
                    int nLen = GetWindowTextLength(hwndSrchPath) +1;
                    
                    PSTR psz = (PSTR) calloc(1, nLen);
                    Assert(psz);
                    
                    GetWindowText(hwndSrchPath, psz, nLen);
                    
                    ModListSetSearchPath(psz);
                    
                    free(psz);
                }
                
                
                if (IsDlgButtonChecked(hwndDlg, ID_DBUGOPT_SUFFIXA)) {
                    ch = 'A';
                } else if (IsDlgButtonChecked(hwndDlg, ID_DBUGOPT_SUFFIXW)) {
                    ch = 'W';
                } else {
                    ch = '\0';
                }
                
                if (ch != SuffixToAppend) {
                    if ( HModEE ) {
#if 0
                        // BUGBUG kentf
                        EESetSuffix( ch );
#endif
                    }
                    SuffixToAppend = ch;
                    nUpdate = UPDATE_WATCH|UPDATE_LOCALS|UPDATE_MEMORY;
                }
                
                //
                // Set behavior for error handling on symbol load
                if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_RADIO_IGNORE_ERRORS)) {
                    g_contWorkspace_WkSp.m_bIgnoreAllSymbolErrors = TRUE;
                } else if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_RADIO_PROMPT_ON_ERROR)) {
                    g_contWorkspace_WkSp.m_bIgnoreAllSymbolErrors = FALSE;
                    g_contWorkspace_WkSp.m_bBrowseForSymsOnSymLoadErrors = TRUE;
                } else if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_RADIO_NEVER_LOAD_BAD_SYMBOLS)) {
                    g_contWorkspace_WkSp.m_bIgnoreAllSymbolErrors = FALSE;
                    g_contWorkspace_WkSp.m_bBrowseForSymsOnSymLoadErrors = FALSE;
                } else {
                    Assert(!"Not supposed to happen");
                }
                
                if (nUpdate != UPDATE_NONE) {
                    UpdateDebuggerState(nUpdate);
                }
            }
            return TRUE;
        }
    }
    return FALSE;
}


