/*----------------------------------------------------------------------------
/ Title;
/   copyhook.cpp
/
/ Authors;
/   Rick Turner (ricktu)
/
/ Notes;
/   Implements ICopyHook for My Documents.
/----------------------------------------------------------------------------*/
#include "precomp.hxx"
#pragma hdrstop

/*-----------------------------------------------------------------------------
/ CMyDocsCopyHook
/----------------------------------------------------------------------------*/
CMyDocsCopyHook::CMyDocsCopyHook( )
{
    MDTraceEnter(TRACE_COPYHOOK, "CMyDocsCopyHook::CMyDocsCopyHook");

    MDTraceLeave();
}

// IUnknown bits

#undef CLASS_NAME
#define CLASS_NAME CMyDocsCopyHook
#include "unknown.inc"

STDMETHODIMP CMyDocsCopyHook::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    INTERFACES iface[] =
    {
        &IID_IShellCopyHook, (ICopyHook *)this
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
}


/*-----------------------------------------------------------------------------
/ ICopyHook methods
/----------------------------------------------------------------------------*/
UINT
CMyDocsCopyHook::CopyCallback( HWND hwnd,
                               UINT wFunc,
                               UINT wFlags,
                               LPCTSTR pszSrcFile,
                               DWORD dwSrcAttribs,
                               LPCTSTR pszDestFile,
                               DWORD dwDestAttribs
                              )
{
    UINT uRes = IDYES;

    MDTraceEnter(TRACE_COPYHOOK, "CMyDocsCopyHook::CopyCallback");
    MDTrace(TEXT("wFunc = 0x%X"), wFunc );

    MDTraceAssert(pszSrcFile && (*pszSrcFile));
    if ((!pszSrcFile) || (!(*pszSrcFile)))
    {
        MDTrace(TEXT("pszSrcFile is NULL!"));
        goto exit_gracefully;
    }

    if ( ((wFunc == FO_COPY) || (wFunc == FO_MOVE)) &&
         ((!pszDestFile) || (!(*pszDestFile)))
        )
    {
        MDTrace(TEXT("pszDestFile is NULL on copy or move!"));
        goto exit_gracefully;
    }

#ifdef DEBUG
    MDTrace(TEXT("Source = %s"), pszSrcFile );
    if (pszDestFile)
    {
        MDTrace(TEXT("Dest   = %s"), pszDestFile );
    }
#endif

    if ( (wFunc == FO_COPY) || (wFunc == FO_MOVE) || (wFunc == FO_DELETE) )
    {
        TCHAR szPersonal[ MAX_PATH ];
        DWORD dwRes;

        //
        //  Get the personal directory path
        //
        if (!SHGetSpecialFolderPath( hwnd, szPersonal, CSIDL_PERSONAL, FALSE ))
        {
            goto exit_gracefully;
        }

        //
        // See if the source is the personal directory
        //
        if (lstrcmpi( pszSrcFile, szPersonal )!=0)
        {
            goto exit_gracefully;
        }

        GetMyDocumentsDisplayName( szPersonal, ARRAYSIZE(szPersonal) );

        //
        // check to see if we're trying to delete the MyDocs folder
        // on the disk...this is a no-no!
        //
        if (wFunc == FO_DELETE)
        {
            LPTSTR pName = PathFindFileName( pszSrcFile );

            //
            // Alert the user that they cannot delete this guy...
            //
            uRes = IDNO;

            ShellMessageBox( g_hInstance, hwnd,
                             (LPTSTR)IDS_NODRAG_RECYCLEBIN, szPersonal,
                             MB_OK | MB_ICONSTOP | MB_APPLMODAL | MB_TOPMOST,
                             pName
                            );

            goto exit_gracefully;
        }

        //
        // the source is the personal directory, now check if the
        // destination is on the desktop...
        //
        dwRes = IsPathGoodMyDocsPath( hwnd, (LPTSTR)pszDestFile );

        if (dwRes == PATH_IS_NONEXISTENT)
        {
            LPTSTR pLastSlash = NULL, pTmp;

            //
            // Find the last slash in the file name to isolate the
            // new sub-directory that will be created...
            //

            for( pTmp = (LPTSTR)pszDestFile; *pTmp; pTmp++ )
            {
                if (*pTmp == TEXT('\\'))
                {
                    pLastSlash = pTmp;
                }
            }


            //
            // Check to see if the new folder will be a direct sub-folder of the
            // desktop...
            //

            if (pLastSlash)
            {
                TCHAR ch = *pLastSlash;

                *pLastSlash = 0;
                dwRes = IsPathGoodMyDocsPath( hwnd, (LPTSTR)pszDestFile );
                *pLastSlash = ch;
            }

        }

        switch( dwRes )
        {

        case PATH_IS_DESKTOP:
            //
            // The user is trying to drag the personal folder to the
            // desktop.  This is a no-no!
            //
            {
                TCHAR szVerb[ 32 ];
                LPTSTR pName = PathFindFileName( pszSrcFile );
                UINT verbID = (wFunc == FO_COPY) ? IDS_COPY : IDS_MOVE;

                szVerb[32] = 0;
                LoadString( g_hInstance, verbID, szVerb, ARRAYSIZE(szVerb) );


                uRes = IDNO;

                if (IsMyDocsHidden())
                {
                    if ( IDYES == ShellMessageBox( g_hInstance, hwnd,
                                                   (LPTSTR)IDS_NODRAG_DESKTOP_HIDDEN,
                                                   szPersonal,
                                                   MB_YESNO | MB_ICONQUESTION |
                                                   MB_APPLMODAL | MB_TOPMOST,
                                                   pName, szVerb, pName
                                                  )
                       )
                    {
                        RestoreMyDocsFolder( hwnd, g_hInstance, NULL, 0 );
                    }
                }
                else
                {
                    ShellMessageBox( g_hInstance, hwnd,
                                     (LPTSTR)IDS_NODRAG_DESKTOP_NOT_HIDDEN,
                                     szPersonal,
                                     MB_OK | MB_ICONSTOP |
                                     MB_APPLMODAL | MB_TOPMOST,
                                     pName, szVerb
                                    );
                }
            }
            break;

        default:
            break;

        }
    }

exit_gracefully:

#ifdef DEBUG
    if (uRes == IDYES)
    {
        MDTrace(TEXT("returning IDYES"));
    }
    else if (uRes == IDNO)
    {
        MDTrace(TEXT("returning IDNO"));
    }
    else if (uRes == IDCANCEL)
    {
        MDTrace(TEXT("returning IDCANCEL"));
    }
#endif
    MDTraceLeave();

    return uRes;
}

