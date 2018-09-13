/*----------------------------------------------------------------------------
/ Title;
/   idlist.cpp
/
/ Authors;
/   Rick Turner (ricktu)
/
/ Notes;
/----------------------------------------------------------------------------*/
#include "precomp.hxx"
#include "stddef.h"
#pragma hdrstop

/*-----------------------------------------------------------------------------
/ IDLISTs are opaque structures, they can be stored at any aligment, ours
/ contain the following data.  However this should not be treated as the
/ public version.  The pack and unpack functions below are responsible for
/ generating this structure, use those.
/----------------------------------------------------------------------------*/

#pragma pack(1)

struct _myiconinfo {
    UINT    uIndex;
    TCHAR   szPath[1];
};

struct _myidlist {
    WORD    cbSize;         // size of the idlist
    DWORD   dwMagic;        // gaurd word
    DWORD   dwFlags;        // idlist flags
    DWORD   dwIconOffset;   // offset to icon info (present if MDIDL_HASICONINFO)
    DWORD   dwNameOffset;   // offset to name info (present if MDIDL_SPECIALITEM)
    DWORD   dwPathOffset;   // offset to path info (present if MDIDL_HASPATH)
    DWORD   dwReserved;
    ITEMIDLIST idl;         // shell's idlist (if present)
};
#pragma pack()

typedef struct _myiconinfo MYICONINFO;
typedef UNALIGNED MYICONINFO* LPMYICONINFO;

typedef struct _myidlist    MYIDLIST;
typedef UNALIGNED MYIDLIST* LPMYIDLIST;


/*-----------------------------------------------------------------------------
/ IsIDLRootOfNameSpace
/ ---------------------
/   Given an idlist, checks to see if it is one that points to the root
/   our namespace (a regitem pidl w/our class id in it)
/
/ In:
/   pidl   = one of our pidls
/
/ Out:
/   TRUE or FALSE
/----------------------------------------------------------------------------*/
BOOL
IsIDLRootOfNameSpace( LPITEMIDLIST pidlIN )
{
    LPIDREGITEM pidl = (LPIDREGITEM)pidlIN;

    if ( (pidl->cb == (sizeof(IDREGITEM) - sizeof(WORD)))     &&
         (pidl->bFlags == SHID_ROOT_REGITEM) &&
         IsEqualGUID( pidl->clsid, CLSID_MyDocumentsExt)
        )
    {
        return TRUE;
    }

    return FALSE;

}


/*-----------------------------------------------------------------------------
/ IsIDLFSJunction
/ ---------------
/   Given an idlist & attributes of item, checks to see if the item
/   is a junction point (i.e., a folder w/a desktop.ini in it).
/
/ In:
/   pidl
/   dwAttrb
/
/ Out:
/   TRUE or FALSE
/----------------------------------------------------------------------------*/
BOOL
IsIDLFSJunction( LPCITEMIDLIST pidlIN, DWORD dwAttrb )
{
    LPIDFSITEM pidl = (LPIDFSITEM)pidlIN;

    if ( (!(dwAttrb & SFGAO_FILESYSTEM)) ||
         (!(dwAttrb & SFGAO_FOLDER))     ||
         (!pidlIN)                       ||
         ILIsEmpty(pidlIN)
        )
    {
        return FALSE;
    }

    if ( (pidl->bFlags & (SHID_JUNCTION)) &&
         ((pidl->bFlags & SHID_FS_DIRECTORY) || (pidl->bFlags & SHID_FS_DIRUNICODE))
        )
    {
        return TRUE;
    }

    return FALSE;

}


/*-----------------------------------------------------------------------------
/ MDGetPathFromIDL
/ ---------------------
/   Given one of our IDLIST, return the relative path to the item...
/
/ In:
/   pidl   = one of our pidls
/   pPath  = buffer of where to store path
/
/ Out:
/   pointer to the path (should NOT be freed)
/----------------------------------------------------------------------------*/
BOOL
MDGetPathFromIDL( LPITEMIDLIST pidl, LPTSTR pPath, LPTSTR pRootPath )
{
    LPMYIDLIST pIDL = (LPMYIDLIST)pidl;
    LPITEMIDLIST pidlTmp;
    BOOL fRes = FALSE;
    TCHAR szPath[ MAX_PATH ];

    if (pIDL->dwMagic == MDIDL_MAGIC)
    {
        if ( (pIDL->dwFlags & MDIDL_HASNOSHELLPIDL) &&
             (pIDL->dwFlags & MDIDL_HASPATHINFO)
            )
        {
            UINT cch = MAX_PATH;

            if (SUCCEEDED(MDGetPathInfoFromIDL( pidl, pPath, &cch )))
                return TRUE;
            else
                return FALSE;
        }

        pidlTmp = &pIDL->idl;
    }
    else
    {
        pidlTmp = pidl;
    }

    if (SHGetPathFromIDList( pidlTmp, szPath ))
    {

        if (MDIsSpecialIDL(pidl) || (!pRootPath))
        {
            lstrcpy( pPath, szPath );
        }
        else
        {
            LPTSTR pFileName;

            pFileName = PathFindFileName( szPath );
            if (pRootPath && *pRootPath)
            {
                lstrcpy( pPath, pRootPath );

                if (pPath[lstrlen(pPath)-1] != TEXT('\\'))
                {
                    lstrcat( pPath, TEXT("\\") );
                }
                lstrcat( pPath, pFileName );
            }
            else
            {
                lstrcpy( pPath, pFileName );
            }
        }
        fRes = TRUE;
    }

    return fRes;
}

/*-----------------------------------------------------------------------------
/ MDGetFullPathFromIDL
/ ---------------------
/   Given one of our IDLIST, return the path to the item walking down the
/   idlist...
/
/ In:
/   pidl   = one of our pidls
/   pPath  = buffer of where to store path
/
/ Out:
/   pointer to the path (should NOT be freed)
/----------------------------------------------------------------------------*/
BOOL
MDGetFullPathFromIDL( LPITEMIDLIST pidl, LPTSTR pPath, LPTSTR pRootPath )
{
    LPITEMIDLIST pidlClone, pidlTmp;
    LPTSTR pTmpPath = pPath, pRoot;
    TCHAR szNull = 0, chLast;

    pidlClone = ILClone( pidl );

    if (pRootPath)
        pRoot = pRootPath;
    else
        pRoot = &szNull;

    if (!pidlClone)
        return FALSE;

    for ( pidlTmp = pidlClone;
          pidlTmp && !ILIsEmpty(pidlTmp);
          pidlTmp = ILGetNext( pidlTmp )
         )
    {
        LPITEMIDLIST pidlNext = ILGetNext(pidlTmp);
        WORD cbNext = pidlNext->mkid.cb;
        pidlNext->mkid.cb = 0;

        // get the path for this item
        if (!MDGetPathFromIDL( pidlTmp, pTmpPath, pRoot ))
            return FALSE;

        // if there are more items, then tack on a '\' to the end of the path
        if (cbNext)
        {
            for( ; (chLast = *pTmpPath) && chLast; pTmpPath++ )
                ;

            //
            // only add '\' to end if it isn't already there (as is the case
            // with a drive --> c:\
            //

            if (chLast != TEXT('\\'))
            {
                *pTmpPath++ = TEXT('\\');
                *pTmpPath = 0;
            }

        }

        pidlNext->mkid.cb = cbNext;
        pRoot = &szNull;

    }

    ILFree( pidlClone );

    return TRUE;

}

/*-----------------------------------------------------------------------------
/ _WrapSHIDL
/ ---------------------
/   Given one of the shell's IDLISTs, return it wrapped in one of our own...
/
/ In:
/   pidlShell   = one of the shell's pidls
/
/ Out:
/   pidl (one of ours)
/----------------------------------------------------------------------------*/
LPMYIDLIST _WrapSHIDL( LPITEMIDLIST pidlRoot, LPITEMIDLIST pidlShell, LPTSTR pPath )
{
    LPMYIDLIST pidl;
    LPITEMIDLIST pidlCombine;
    UINT cb;
    DWORD dw;
    TCHAR szPath[ MAX_PATH ];


    MDTraceEnter( TRACE_IDLIST, "_WrapSHIDL" );
//    MDTrace(TEXT("Incoming: pidlRoot = %08X, pidl = %08X"), pidlRoot, pidlShell );

    if (pidlRoot)
    {
        pidlCombine = ILCombine( pidlRoot, pidlShell );
    }
    else
    {
        pidlCombine = pidlShell;
    }

    cb  = ILGetSize( pidlCombine );

    pidl = (LPMYIDLIST)SHAlloc( cb + sizeof(MYIDLIST) );

    if (pidl)
    {
        memset( pidl, 0, cb + sizeof(MYIDLIST) );
        pidl->cbSize = cb + sizeof(MYIDLIST) - sizeof(WORD);
        pidl->dwMagic = MDIDL_MAGIC;

        memcpy( (LPVOID)&(pidl->idl), pidlCombine, cb );
    }

    if (!pPath)
    {
        SHGetPathFromIDList( pidlCombine, szPath );
        pPath = szPath;
    }


    //
    // If this item is directory, mark it is a junction...
    //

    dw = GetFileAttributes( pPath );
    if (dw!=0xFFFFFFFF)
    {
        if (dw & FILE_ATTRIBUTE_DIRECTORY)
        {
            pidl->dwFlags |= MDIDL_ISJUNCTION;
        }
    }


//    MDTrace(TEXT("Outgoing pidl is %08X"), pidl );
    MDTraceLeave();

    return pidl;

}


/*-----------------------------------------------------------------------------
/ _UnWrapMDIDL
/ ---------------------
/   Given one of our IDLISTs, return a copy of the embedded shell idlist...
/
/ In:
/   pidlShell   = one of the shell's pidls
/
/ Out:
/   pidl (one of ours)
/----------------------------------------------------------------------------*/
LPITEMIDLIST _UnWrapMDIDL( LPMYIDLIST pidl )
{
    LPITEMIDLIST pidlOut = (LPITEMIDLIST)pidl;

    MDTraceEnter( TRACE_IDLIST, "_UnWrapMDIDL" );
//    MDTrace(TEXT("Incoming pidl is %08X"), pidl );

    if (pidl->dwMagic == MDIDL_MAGIC)
    {
        if (pidl->dwFlags & MDIDL_HASNOSHELLPIDL)
        {
            pidlOut = NULL;
        }
        else
        {
            pidlOut = &pidl->idl;
        }
    }

//    MDTrace(TEXT("Outgoing pidl is %08X"), pidlOut );
    MDTraceLeave();

    return pidlOut;

}

/*-----------------------------------------------------------------------------
/ MDCreateIDLFromPath
/ ---------------------
/   Given a pointer to filename (or UNC path), create an IDLIST from that.
/   that.
/
/ In:
/   ppidl = receives a pointer to new IDLIST
/   pPath = filesystem (or UNC) path to object
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT
MDCreateIDLFromPath( LPITEMIDLIST* ppidl,
                     LPTSTR pPath
                     )
{
    HRESULT hr = S_OK;
    LPMYIDLIST pidl = NULL;
    LPITEMIDLIST pidlShell = NULL;

    MDTraceEnter(TRACE_IDLIST, "MDCreateIDLFromPath");

    MDTraceAssert(ppidl);
    MDTraceAssert(pPath);

    *ppidl = NULL;                      // incase we fail


    //
    // Get pidl from shell
    //

    hr = SHILCreateFromPath( pPath, &pidlShell, NULL );
    FailGracefully( hr, "SHILCreateFromPath failed" );

    //
    // Now wrap it...
    //

    pidl = _WrapSHIDL( NULL, pidlShell, pPath );
    if (!pidl)
        ExitGracefully( hr, E_FAIL, "_WrapSHIDL failed" );


    *ppidl = (LPITEMIDLIST)pidl;

//    MDTrace( TEXT("returning pidl of %08X"), pidl );

exit_gracefully:

    DoILFree( pidlShell );
    MDTraceLeaveResult(hr);
}

/*-----------------------------------------------------------------------------
/ MDWrapShellIDList
/ ---------------------
/   Given an array of shell idlists, return an array of MyDocuments idlists
/   that wrap the shell's idlists.
/
/ In:
/   celt  = number of IDLISTs in array
/   rgelt = array of IDLISTs
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT
MDWrapShellIDLists( LPITEMIDLIST pidlRoot,
                    UINT celtIN,
                    LPITEMIDLIST * rgeltIN,
                    LPITEMIDLIST * rgeltOUT
                   )
{
    HRESULT hr = S_OK;
    UINT i;

    MDTraceEnter( TRACE_IDLIST, "MDWrapShellIDLists" );

    if (!celtIN || !rgeltIN || !rgeltOUT)
        ExitGracefully( hr, E_INVALIDARG, "either bad incoming or outgoing parameters" );

    //
    // Set up prgeltOUT to hold the results...
    //

    i = 0;
    while( i < celtIN )
    {
        rgeltOUT[i] = (LPITEMIDLIST)_WrapSHIDL( pidlRoot, rgeltIN[i], NULL );
        if (!rgeltOUT[i])
            ExitGracefully( hr, E_FAIL, "Failure to wrap an IDLIST" );

        i++;
    }


exit_gracefully:

    MDTraceLeaveResult(hr);

}


/*-----------------------------------------------------------------------------
/ MDUnWrapMDIDList
/ ---------------------
/   Given an array of our idlists, return an array of shell idlists.
/
/ In:
/   celt  = number of IDLISTs in array
/   rgelt = array of IDLISTs
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT
MDUnWrapMDIDLists( UINT celtIN,
                   LPITEMIDLIST * rgeltIN,
                   LPITEMIDLIST * rgeltOUT
                  )
{
    HRESULT hr = S_OK;
    UINT i;

    MDTraceEnter( TRACE_IDLIST, "MDUnWrapMDIDLists" );

    if (!celtIN || !rgeltIN || !rgeltOUT)
        ExitGracefully( hr, E_INVALIDARG, "either bad incoming or outgoing parameters" );

    //
    // Set up prgeltOUT to hold the results...
    //

    i = 0;
    while( i < celtIN )
    {
        LPITEMIDLIST pidl = _UnWrapMDIDL( (LPMYIDLIST)rgeltIN[i] );

        if (pidl)
        {
            rgeltOUT[i] = ILFindLastID(pidl);
        }
        else
        {
            rgeltOUT[i] = NULL;
        }

        if (!rgeltOUT[i])
            ExitGracefully( hr, E_FAIL, "Failure to unwrap an IDLIST" );

        i++;
    }


exit_gracefully:

    MDTraceLeaveResult(hr);

}

/*-----------------------------------------------------------------------------
/ SHIDLFromMDIDL
/ ---------------------
/   If it's one of our pidls, return the embedded shell IDLIST
/
/ In:
/   pidl = one of our pidls
/
/ Out:
/   pidl that is shell's embedded pidl
/----------------------------------------------------------------------------*/
LPITEMIDLIST
SHIDLFromMDIDL( LPCITEMIDLIST pidl )
{
    LPITEMIDLIST pidlOut = (LPITEMIDLIST)pidl;
    LPMYIDLIST pIDL = (LPMYIDLIST)pidl;

    MDTraceEnter( TRACE_IDLIST, "SHIDLFroMDIDL" );

    if ( pIDL
         && (pIDL->dwMagic == MDIDL_MAGIC)
        )
    {
        if ( ( pIDL->dwFlags & MDIDL_HASNOSHELLPIDL ) )
        {
            pidlOut = NULL;
        }
        else
        {
            pidlOut = &((LPMYIDLIST)pidl)->idl;
        }
    }

    MDTraceLeave();

    return pidlOut;
}


/*-----------------------------------------------------------------------------
/ MDCreateSpecialIDL
/ ---------------------
/   Return an IDL for the Published Documents folder
/
/ In:
/   pidl = one of our pidls
/
/ Out:
/   pidl that is shell's embedded pidl
/----------------------------------------------------------------------------*/
LPITEMIDLIST
MDCreateSpecialIDL( LPTSTR pPath,
                    LPTSTR pName,
                    LPTSTR pIconPath,
                    UINT   uIconIndex
                   )
{
    HRESULT hr = S_OK;
    LPMYIDLIST pidl = NULL;
    UINT cbSize = 0, cbName = 0, cbIconInfo = 0, cbPath = 0;
    DWORD dwFlags = 0;

    MDTraceEnter(TRACE_IDLIST, "MDCreateSpecialIDL");

    MDTraceAssert(pPath);

//#ifdef WINNT
//    DebugBreak();
//#else
//    _asm {
//        int 3;
//    }
//#endif

    if ((pPath && *pPath))
    {

#ifdef TRY_TO_STORE_PIDL
        if (PathFileExists(pPath))
        {
            //
            // Create a pidl
            //

            hr = MDCreateIDLFromPath( (LPITEMIDLIST *)&pidl, pPath );
            FailGracefully( hr, "MDCreateIDLFromPath failed" );
        }
        else
#endif
        {
            cbPath = ((lstrlen(pPath) + 1) * sizeof(TCHAR));
            dwFlags |= MDIDL_HASPATHINFO;
        }

    }

    dwFlags |= MDIDL_ISSPECIALITEM;
    if (pidl)
    {
        cbSize = ILGetSize( (LPITEMIDLIST)pidl ) + sizeof(WORD);
    }
    else
    {
        cbSize = sizeof(MYIDLIST);
    }

    //
    // If there's a display name, make room for it...
    //
    if (pName && *pName)
    {
        cbName = ((lstrlen(pName) + 1) * sizeof(TCHAR));
        dwFlags |= MDIDL_HASNAME;
    }

    //
    // If there's icon information, make room for it...
    //
    if (pIconPath && *pIconPath)
    {
        // NULL terminator is already in MYICONINFO
        cbIconInfo = ( sizeof(MYICONINFO) + (lstrlen(pIconPath) * sizeof(TCHAR)) );
        dwFlags |= MDIDL_HASICONINFO;
    }

    //
    // Now, combine all the info into one place...
    //

    if (cbName || cbIconInfo || cbPath)
    {
        UINT cbTotal = cbSize + cbName + cbIconInfo + cbPath;

        LPITEMIDLIST pidlTemp =
                (LPITEMIDLIST)SHAlloc( cbTotal );

        if (pidlTemp)
        {

            memset( pidlTemp, 0, cbTotal );

            //
            // Copy the old stuff and adjust the size field...
            //

            if (pidl)
            {
                pidl->dwFlags = dwFlags;
                cbSize -= sizeof(WORD);
                memcpy( (LPVOID)pidlTemp, (LPVOID)pidl, cbSize );
                DoILFree( pidl );
                pidlTemp->mkid.cb += sizeof(WORD);
            }
            else
            {
                ((LPMYIDLIST)pidlTemp)->dwMagic = MDIDL_MAGIC;
                ((LPMYIDLIST)pidlTemp)->dwFlags = (dwFlags | MDIDL_HASNOSHELLPIDL);
                cbSize -= sizeof(WORD);
                ((LPMYIDLIST)pidlTemp)->cbSize  = cbSize;
            }

            pidl = (LPMYIDLIST)pidlTemp;
            pidl->cbSize += (cbName + cbIconInfo + cbPath);

            //
            // Now insert the new stuff...
            //

            if (cbName)
            {
                LPTSTR pTmp;

                pidl->dwNameOffset = cbSize;
                pTmp = (LPTSTR)((LPBYTE)pidl + pidl->dwNameOffset);
                lstrcpy( pTmp, pName );
            }

            if (cbIconInfo)
            {
                LPMYICONINFO pii;

                pidl->dwIconOffset = cbSize + cbName;

                pii = (LPMYICONINFO)((LPBYTE)pidl + pidl->dwIconOffset);

                pii->uIndex = uIconIndex;
                lstrcpy( pii->szPath, pIconPath );
            }

            if (cbPath)
            {
                LPTSTR pTmp;

                pidl->dwPathOffset = cbSize + cbName + cbIconInfo;

                pTmp = (LPTSTR)((LPBYTE)pidl + pidl->dwPathOffset);
                lstrcpy( pTmp, pPath );

            }

        }
        else
        {
            DoILFree( pidl )
            pidl = NULL;
        }



    }


#if 0
    MDTrace(TEXT("pidl->cbSize = %d"), pidl->cbSize );
    MDTrace(TEXT("pidl->dwFlags = %08X"), pidl->dwFlags );
    if (cbName)
        MDTrace(TEXT("Name is: -%s-"), (LPTSTR)((LPBYTE)pidl + pidl->dwNameOffset) );
    if (cbIconInfo)
    {
        LPMYICONINFO pii = (LPMYICONINFO)((LPBYTE)pidl + pidl->dwIconOffset);
        MDTrace(TEXT("Icon info: %s,%d"), pii->szPath, pii->uIndex );
    }
    MDTrace(TEXT("ILNext(pidl) is %04X"), *((WORD *)((LPBYTE)pidl + pidl->cbSize)));
#endif

#ifdef TRY_TO_STORE_PIDL
exit_gracefully:
#endif

//    MDTrace(TEXT("returning pidl of %08X"), pidl );

    MDTraceLeave();


    return (LPITEMIDLIST)pidl;

}


/*-----------------------------------------------------------------------------
/ MDIsSepcialIDL
/ ---------------------
/   Check if this pidl is a special item (ie: PUBLISHED DOCUMENTS) pidl
/
/ In:
/   pidl
/
/ Out:
/   TRUE or FALSE
/----------------------------------------------------------------------------*/
BOOL
MDIsSpecialIDL( LPITEMIDLIST pidl )
{
    
    if (pidl && ((sizeof(MYIDLIST) - sizeof(WORD)) <= (pidl->mkid).cb)) {
        if ( ( ((LPMYIDLIST)pidl)->dwMagic == MDIDL_MAGIC ) &&
             ( ((LPMYIDLIST)pidl)->dwFlags & MDIDL_ISSPECIALITEM )
            )
            return TRUE;
    }

    return FALSE;

}

/*-----------------------------------------------------------------------------
/ MDIsJunctionIDL
/ ---------------------
/   Check if this pidl represents a junction to a folder
/
/ In:
/   pidl
/
/ Out:
/   TRUE or FALSE
/----------------------------------------------------------------------------*/
BOOL
MDIsJunctionIDL( LPITEMIDLIST pidl )
{

    if (pidl && ((sizeof(MYIDLIST) - sizeof(WORD)) <= (pidl->mkid).cb)) {

        if ( ( ((LPMYIDLIST)pidl)->dwMagic == MDIDL_MAGIC ) &&
             ( ((LPMYIDLIST)pidl)->dwFlags & MDIDL_ISJUNCTION )
            )
            return TRUE;
    
        //
        // HACK: 0x31 is the SHITEMID flags for a folder and
        //       0x35 is the SHITEMID flags for a unicode folder...
        //
    
        if ( (((_idregitem *)pidl)->bFlags == 0x31) ||
             (((_idregitem *)pidl)->bFlags == 0x35) )
        {
            return TRUE;
        }

    }

    return FALSE;

}


/*-----------------------------------------------------------------------------
/ MDGetIconInfoFromIDL
/ ---------------------
/   Check if this pidl is a special pidl, and then returns the icon
/   information if present...
/
/ In:
/   pidl -> pidl in question
/   pIconPath -> pointer to buffer to contain path (assumed to be MAX_PATH long)
/   pwIndex -> index of icon within pIconPath
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT
MDGetIconInfoFromIDL( LPITEMIDLIST pidl,
                      LPTSTR pIconPath,
                      UINT cch,
                      UINT * pIndex
                     )
{
    HRESULT hr = S_OK;
    LPMYIDLIST pIDL = (LPMYIDLIST)pidl;
    LPMYICONINFO pii;

    MDTraceEnter(TRACE_IDLIST, "MDGetIconInfoFromIDL");

    if (!pIconPath)
        ExitGracefully( hr, E_INVALIDARG, "pIconPath is invalid" );

    if (!pIndex)
        ExitGracefully( hr, E_INVALIDARG, "pIndex is invalid" );

    *pIconPath = 0;
    *pIndex = 0;

    if (!MDIsSpecialIDL(pidl))
        ExitGracefully( hr, E_FAIL, "Not a special idlist" );

    if (!(pIDL->dwFlags & MDIDL_HASICONINFO))
        ExitGracefully( hr, E_FAIL, "No icon info to return..." );

    pii = (LPMYICONINFO)( (LPBYTE)pIDL + pIDL->dwIconOffset );

    lstrcpyn( pIconPath, pii->szPath, cch );
    *pIndex = pii->uIndex;


exit_gracefully:

    MDTraceLeaveResult(hr);

}

/*-----------------------------------------------------------------------------
/ MDGetNameFromIDL
/ ---------------------
/   Check if this pidl is a special pidl, and then returns the name
/   information if present...
/
/ In:
/   pidl -> pidl in question
/   pName -> pointer to buffer to contain name
/   cchName -> size of name buffer, in characters
/   fFullPath -> if TRUE, then walk down the IDLIST building up a path.  If
/                FALSE, just return the display name for the first item in the
/                ID List...
/
/
/ Out:
/   HRESULT
/       E_INVALIDARG --> buffer is to small, *pcch hold required size
/----------------------------------------------------------------------------*/
HRESULT
MDGetNameFromIDL( LPITEMIDLIST pidl,
                  LPTSTR pName,
                  UINT *  pcch,
                  BOOL fFullPath
                 )
{
    HRESULT hr = S_OK;
    LPITEMIDLIST pidlTemp = pidl;
    LPMYIDLIST pIDL;
    LPTSTR pNAME;
    UINT cch;
    TCHAR szPath[ MAX_PATH ];

    MDTraceEnter(TRACE_IDLIST, "MDGetNameFromIDL");

    if (!pName)
        ExitGracefully( hr, E_FAIL, "pName is invalid" );

    if (!pcch)
        ExitGracefully( hr, E_FAIL, "cch is invalid" );

    *pName = 0;
    cch = *pcch;
    *pcch = 0;

    while( pidlTemp && !ILIsEmpty(pidlTemp) )
    {
        pIDL = (LPMYIDLIST)pidlTemp;

        if (MDIsSpecialIDL(pidlTemp) && (pIDL->dwFlags & MDIDL_HASNAME))
        {
            pNAME = (LPTSTR)( (LPBYTE)pIDL + pIDL->dwNameOffset );
        }
        else
        {
            if (fFullPath)
            {
                pName = szPath;
                MDGetPathFromIDL( pidlTemp, szPath, NULL );
            }
            else
            {
                ExitGracefully( hr, E_INVALIDARG, "fFullPath is FALSE and encountered a non-special IDL" );
            }
        }

        if ((*pcch += (lstrlen(pNAME)+2)) <= cch)
        {
            lstrcat( pName, pNAME );
        }
        else
        {
            hr = E_INVALIDARG;
            break;
        }

        pidlTemp = ILGetNext( pidlTemp );
        if (pidlTemp && !ILIsEmpty( pidlTemp ))
        {
            lstrcat( pName, TEXT("\\") );

        }
    }

exit_gracefully:

    MDTraceLeaveResult(hr);

}


/*-----------------------------------------------------------------------------
/ MDGetPathInfoIDL
/ ---------------------
/   Check if this pidl is a special pidl, and then if it has a path stored
/   int he pidl, and if so, it returns that path...
/
/ In:
/   pidl -> pidl in question
/   pName -> pointer to buffer to contain name
/   cchName -> size of name buffer, in characters
/
/ Out:
/   HRESULT
/       E_INVALIDARG --> buffer is to small, *pcch hold required size
/----------------------------------------------------------------------------*/
HRESULT
MDGetPathInfoFromIDL( LPITEMIDLIST pidl,
                      LPTSTR pPath,
                      UINT *  pcch
                     )
{
    HRESULT hr = S_OK;
    LPMYIDLIST pIDL = (LPMYIDLIST)pidl;
    LPTSTR pPATH = NULL;
    UINT cch;

    MDTraceEnter(TRACE_IDLIST, "MDGetPathInfoIDL");

    if (!pPath)
        ExitGracefully( hr, E_FAIL, "pName is invalid" );

    if (!pcch)
        ExitGracefully( hr, E_FAIL, "cch is invalid" );

    if (!pIDL || (!(pIDL->dwFlags & MDIDL_HASPATHINFO)))
    {
        ExitGracefully( hr, E_FAIL, "pidl is NULL or doesn't have a path!" );
    }

    *pPath = 0;
    cch = *pcch;
    *pcch = 0;

    pPATH = (LPTSTR)((LPBYTE)pIDL + pIDL->dwPathOffset);

    if (cch < (UINT)((lstrlen(pPATH)+1)))
    {
        *pcch = (UINT)(lstrlen(pPATH)+1);
        ExitGracefully( hr, E_INVALIDARG, "*pcch wasn't big enough" );
    }

    if (0==ExpandEnvironmentStrings( pPATH, pPath, cch ))
    {
        lstrcpy( pPath, pPATH );
    }

exit_gracefully:

    MDTraceLeaveResult(hr);

}
