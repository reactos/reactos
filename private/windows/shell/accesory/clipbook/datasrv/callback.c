

#include    <windows.h>
#include    <nddeapi.h>
#include    <nddesec.h>
#include    <string.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <memory.h>
#include    <assert.h>
#include    <ddeml.h>

#include    "common.h"

#include    "clipsrv.h"
#include    "clipfile.h"
#include    "ddeutil.h"
#include    "callback.h"

#include    "debugout.h"






#define AUTOUPDATE
#define ASKED_FOR_LINK 1
#define ASKED_FOR_OBJECTLINK 2


#define MAX_XERR    128         // maximum number of XERRs we'll store
#define XERR_INUSE  1


typedef struct _XERR            // struct for Xaction ERRor
    {
    HCONV       hConv;          // the conversation handle
    DWORD       dwErr;          // the error code
    DWORD       dwUse;          // usage, when & with XERR_MASK gives type,
    }                           //    bit 1 is set when if struct is in use.
    XERR, *PXERR;


XERR    gXactErr[MAX_XERR];
TCHAR   gszXactErrStr[30];




TCHAR szInitShareCmd[]      = SZCMD_INITSHARE;
TCHAR szExitCmd[]           = SZCMD_EXIT;
TCHAR szPasteShareCmd[]     = SZCMD_PASTESHARE;
TCHAR szDelShareCmd[]       = SZCMD_DELETE;
TCHAR szMarkSharedCmd[]     = SZCMD_SHARE;
TCHAR szMarkUnSharedCmd[]   = SZCMD_UNSHARE;
TCHAR szKeepCmd[]           = SZCMD_PASTE;
TCHAR szSaveAsCmd[]         = SZCMD_SAVEAS;
TCHAR szSaveAsOldCmd[]      = SZCMD_SAVEASOLD;
TCHAR szOpenCmd[]           = SZCMD_OPEN;
TCHAR szDebugCmd[]          = SZCMD_DEBUG;
TCHAR szVersionCmd[]        = SZCMD_VERSION;
TCHAR szSecurityCmd[]       = SZCMD_SECURITY;
TCHAR szDebug[]             = TEXT("Debug");
TCHAR szVer[]               = TEXT("1.1"); // BUGBUG need to be able to
                                           // handle Uni or Ansi req's for this

TCHAR szSection[]           = TEXT("Software\\Microsoft\\Clipbook Server");
TCHAR szClipviewRoot[]      = TEXT("Software\\Microsoft\\Clipbook");
TCHAR szRegClass[]          = TEXT("Config");

HSZ hszSysTopic;
HSZ hszTopicList;
HSZ hszFormatList;
HSZ hszErrorRequest;











/*
 *      MakeTheDamnKey
 *
 *  Purpose: Open the usual key (the one named by szSection) with the
 *     specified access.
 *
 *  Parameters:
 *     phkey - Pointer to the HKEY to fill
 *     regsam - The access types, same as RegCreateKeyEx
 *
 *  Returns:
 *     ERROR_SUCCESS on success, whatever RegOpenKeyEx returns on fail.
 */

LONG MakeTheDamnKey(
    PHKEY   phkey,
    REGSAM  regsam)
{
DWORD   dwR;
DWORD   dwIck;


    dwR = RegCreateKeyEx (HKEY_LOCAL_MACHINE,
                          szSection,
                          0,
                          szRegClass,
                          REG_OPTION_NON_VOLATILE,
                          regsam,
                          NULL,
                          phkey,
                          &dwIck);

    if (dwR == ERROR_SUCCESS)
       {
       return ERROR_SUCCESS;
       }



    dwR = RegOpenKeyEx (HKEY_LOCAL_MACHINE, szSection, 0, regsam, phkey);

    if (dwR == ERROR_SUCCESS)
       {
       return ERROR_SUCCESS;
       }


    PERROR(TEXT("Couldn't regopen %s with %lx access - #%lx\r\n"), szSection, (long)regsam, dwR);

    return dwR;
    
}








/*
 *      lstrncmp
 */

int lstrncmp(
    LPTSTR  s1,
    LPTSTR  s2,
    WORD    count )
{
unsigned i;
register TCHAR tch1;
register TCHAR tch2;

    for (i = 0; i < (unsigned)count; i++)
        {
        if ( (tch1 = *s1++) != (tch2 = *s2++) )
            {
            if (tch1 < tch2)
                return -1;
            else
                return 1;
            }
        }

    return 0;

}









/*
 *      AddXactErr
 *
 *  To add a XERR record for hConv.  Called
 *  when XTYP_CONNECT_CONFIRM.
 */

static VOID    AddXactErr(
    HCONV   hConv)
{
INT i;


    for (i=0; i<MAX_XERR; i++)
        if (!gXactErr[i].dwUse & XERR_INUSE)
            {
            gXactErr[i].hConv = hConv;
            gXactErr[i].dwErr = 0;
            gXactErr[i].dwUse = XERR_INUSE;

            return;
            }

}




/*
 *      DelXactErr
 *
 *  To delete a XERR record for hConv.
 *  Called at XTYP_DISCONNECT.
 */

static VOID    DelXactErr(
    HCONV   hConv)
{
INT i;

    for (i=0; i<MAX_XERR; i++)
        if ((gXactErr[i].dwUse & XERR_INUSE) && gXactErr[i].hConv == hConv)
            {
            gXactErr[i].dwUse = 0;
            return;
            }

}




/*
 *      GetXactErr
 *
 *  Returns the XERR error code associated with hConv.
 */

DWORD   GetXactErr(
    HCONV   hConv)
{
INT i;

    for (i=0; i<MAX_XERR; i++)
        if ((gXactErr[i].dwUse & XERR_INUSE) && gXactErr[i].hConv == hConv)
            return gXactErr[i].dwErr;

    return 0L;
}






/*
 *      GetXactErrType
 *
 *  Returns the XERR error type associated with hConv.
 */

DWORD   GetXactErrType(
    HCONV   hConv)
{
INT i;

    for (i=0; i<MAX_XERR; i++)
        if ((gXactErr[i].dwUse & XERR_INUSE) && gXactErr[i].hConv == hConv)
            return gXactErr[i].dwUse & XERRT_MASK;

    return 0L;
}






/*
 *      SetXactErr
 *
 *  Sets XERR for hConv.
 *  dwType specifies type, this should be one of the XERRT_ defines.
 *  dwErr specifies error code.
 */

VOID    SetXactErr(
    HCONV   hConv,
    DWORD   dwType,
    DWORD   dwErr)
{
INT     i;

    for (i=0; i<MAX_XERR; i++)
        if ((gXactErr[i].dwUse & XERR_INUSE) && gXactErr[i].hConv == hConv)
            {
            gXactErr[i].dwErr = dwErr;
            gXactErr[i].dwUse = (gXactErr[i].dwUse & ~XERRT_MASK) | dwType;
            return;
            }

}






/*
 *      DdeCallback
 */

HDDEDATA EXPENTRY DdeCallback(
    WORD        wType,
    WORD        wFmt,
    HCONV       hConv,
    HSZ         hszTopic,
    HSZ         hszItem,
    HDDEDATA    hData,
    DWORD       lData1,
    DWORD       lData2)
{
HDDEDATA    hDDEtmp = 0L;
UINT        uiErr;



    PINFO(TEXT("\n>>>> DdeCallback\n"));

    if (!(wType & XCLASS_NOTIFICATION))
        {
        PINFO(TEXT("Impersonating\n"));
        DdeImpersonateClient(hConv);
        }



    switch ( wType )
        {
        case XTYP_CONNECT_CONFIRM:

            PINFO (TEXT("XTYP_CONNECT_CONFIRM\n"));

            AddXactErr (hConv);
            PINFO(TEXT("Confirming connect\r\n"));
            hDDEtmp = (HDDEDATA)TRUE;
            break;


        case XTYP_EXECUTE:

            PINFO (TEXT("XTYP_EXECUTE\n"));

            // no error yet

            SetXactErr (hConv, 0, 0);


            // We only take executes on the System topic.
            // And only in Unicode or CF_TEXT format.

            if ((wFmt == CF_TEXT || wFmt == CF_UNICODETEXT) &&
                DdeCmpStringHandles ( hszTopic, hszSysTopic ) )
                {
                PERROR(TEXT("XTYP_EXECUTE received on non-system topic\n\r"));
                hDDEtmp = (HDDEDATA)DDE_FNOTPROCESSED;

                break;
                }


            DdeGetData(hData, (LPBYTE)szExec, MAX_EXEC, 0);
            szExec[MAX_EXEC - 1] = '\0';


            hDDEtmp = DDE_FNOTPROCESSED;



            #if 0
            //a-mgates, 12/92. We no longer shut down when asked this way,
            //'cause we're a system service, not just part of the Clipbook app.
            if (0 == lstrncmp ( szExec, szExitCmd, (WORD)lstrlen(szExitCmd)))
                {
                DdeDisconnect ( hConv );
                PostMessage ( hwndApp, WM_CLOSE, 0, 0L );
                hDDEtmp = (HDDEDATA)DDE_FACK;
                }
            //a-mgates This is never used by NT clipbook, 'cause we have to set
            //the page up before we can set its security
            else if ( 0 == lstrncmp( szExec, szPasteShareCmd,
                (WORD)lstrlen(szPasteShareCmd )) )
                {
                PINFO(TEXT("Add/sharing %s\n\r"),
                   (LPSTR)szExec + lstrlen(szPasteShareCmd ) );

                AddShare ( szExec + lstrlen(szPasteShareCmd ), SIF_SHARED );

                hDDEtmp = (HDDEDATA)DDE_FACK;
                }
            else
            #endif


            if (!lstrcmp (szExec, szInitShareCmd))
                {
                InitShares();
                hDDEtmp = (HDDEDATA)DDE_FACK;
                }
            else if (!lstrncmp (szExec, szKeepCmd, (WORD)lstrlen(szKeepCmd)))
                {
                DWORD dwErr;

                dwErr = AddShare ( szExec + lstrlen(szKeepCmd ), 0);
                if (dwErr != NO_ERROR)
                    SetXactErr (hConv, XERRT_SYS, dwErr);
                else
                    hDDEtmp = (HDDEDATA)DDE_FACK;
                }
            else if (!lstrncmp (szExec, szVersionCmd, (WORD)lstrlen(szVersionCmd)))
                {
                hDDEtmp = DdeCreateDataHandle(idInst, szVer, sizeof(szVer) + 1, 0, 0L, wFmt, 0L);
                }
            else if (!lstrncmp( szExec, szSaveAsCmd, (WORD)lstrlen(szSaveAsCmd)))
                {
                DWORD dwErr;

                fNTSaveFileFormat = TRUE;

                dwErr = SaveClipboardToFile (hwndApp, NULL, szExec + lstrlen(szSaveAsCmd ), FALSE);
                if (dwErr != NO_ERROR)
                    SetXactErr (hConv, XERRT_SYS, dwErr);
                else
                    hDDEtmp = (HDDEDATA)DDE_FACK;
                }
            else if (!lstrncmp (szExec, szSaveAsOldCmd, (WORD)lstrlen(szSaveAsOldCmd)))
                {
                DWORD dwErr;

                fNTSaveFileFormat = FALSE;

                dwErr = SaveClipboardToFile(hwndApp, NULL, szExec + lstrlen(szSaveAsOldCmd), FALSE);
                if (dwErr != NO_ERROR)
                    SetXactErr (hConv, XERRT_SYS, dwErr);
                else
                    hDDEtmp = (HDDEDATA)DDE_FACK;
                }
            else if (!lstrncmp (szExec, szOpenCmd, (WORD)lstrlen(szOpenCmd )) )
                {
                DWORD dwErr;

                dwErr = OpenClipboardFile(hwndApp, szExec + lstrlen(szOpenCmd));
                if (dwErr != NO_ERROR)
                    SetXactErr (hConv, XERRT_SYS, dwErr);
                else
                    hDDEtmp = (HDDEDATA)DDE_FACK;
                }
            else if (!lstrncmp (szExec, szDelShareCmd, (WORD)lstrlen(szDelShareCmd)))
                {
                PINFO(TEXT("Deleting %s\n\r"), (LPSTR)szExec + lstrlen(szDelShareCmd));

                if (DelShare ( hConv, szExec + lstrlen(szDelShareCmd) ))
                    {
                    hDDEtmp = (HDDEDATA)DDE_FACK;
                    }
                }
            else if (!lstrncmp (szExec,szMarkSharedCmd, (WORD)lstrlen(szMarkSharedCmd)))
                {
                PINFO(TEXT("Marking %s as shared\n\r"), (LPSTR)szExec + lstrlen(szMarkSharedCmd));

                if ( MarkShare (szExec + lstrlen(szMarkSharedCmd), SIF_SHARED ))
                    {
                    hDDEtmp = (HDDEDATA)DDE_FACK;
                    }
                }
            else if (!lstrncmp(szExec,szMarkUnSharedCmd, (WORD)lstrlen(szMarkUnSharedCmd)))
                {
                if ( MarkShare ( szExec + lstrlen(szMarkUnSharedCmd ), 0 ))
                    {
                    hDDEtmp = (HDDEDATA)DDE_FACK;
                    }
                }

            #if DEBUG
            else if (!lstrncmp(szExec,szDebugCmd, (WORD)lstrlen(szDebugCmd)))
                {
                DumpShares();
                hDDEtmp = (HDDEDATA)DDE_FACK;
                }
            #endif
            else
                {
                PERROR(TEXT("Invalid execute\r\n"));
                hDDEtmp = (HDDEDATA)DDE_FNOTPROCESSED;
                }

            break;

        case XTYP_DISCONNECT:

            PINFO (TEXT("XTYP_DISCONNECT\n"));

            DelXactErr (hConv);
            break;

        case XTYP_CONNECT:

            PINFO (TEXT("XTYP_CONNECT\n"));

            hDDEtmp = (HDDEDATA)FALSE;

            if ( IsSupportedTopic( hszTopic ) )
                {
                if (!DdeKeepStringHandle (idInst, hszAppName))
                    {
                    PERROR(TEXT("DdeKSHandle fail in DdeCB\r\n"));
                    }
                hDDEtmp = (HDDEDATA)TRUE;
                }
            #if DEBUG
            else
                {
                TCHAR buf[128];
                DdeQueryString ( idInst, hszTopic, buf, 128, CP_WINANSI );
                PERROR(TEXT("ClipSRV: Unsupported topic %s requested\n\r"), (LPSTR)buf );
                }
            #endif

            break;


        case XTYP_ADVREQ:
        case XTYP_REQUEST:

            // must be a valid topic
            {
            TCHAR atch[128];

            PINFO (wType==XTYP_ADVREQ?
                     TEXT("XTYP_ADVREQ\n"): TEXT("XTYP_REQUEST\n"));

            DdeQueryString(idInst, hszTopic, atch, 128, CP_WINANSI);
            PINFO(TEXT("Topic = %s, "), atch);
            DdeQueryString(idInst, hszItem, atch, 128, CP_WINANSI);
            PINFO(TEXT("item = %s\r\n"), atch);
            }

            if (!IsSupportedTopic ( hszTopic ))
                {
                #if DEBUG
                TCHAR buf[128];
                DdeQueryString ( idInst, hszTopic, buf, 128, CP_WINANSI );
                PERROR(TEXT("Topic %s unsupported!\n\r"), (LPTSTR)buf );
                #endif

                hDDEtmp = (HDDEDATA)0;
                }
            else
                {
                PINFO("System topic request\r\n");

                if (!DdeCmpStringHandles (hszTopic, hszSysTopic))
                    {
                    if (!DdeCmpStringHandles (hszItem,  hszErrorRequest))
                        {
                        sprintf (gszXactErrStr,
                                 XERR_FORMAT,
                                 GetXactErrType (hConv),
                                 GetXactErr (hConv));

                        hDDEtmp = DdeCreateDataHandle (idInst,
                                                       gszXactErrStr,
                                                       lstrlen(gszXactErrStr)+sizeof(CHAR),
                                                       0,
                                                       hszErrorRequest,
                                                       wFmt,
                                                       0);

                        uiErr = DdeGetLastError (idInst);
                        }
                    else if (!DdeCmpStringHandles (hszItem, hszTopicList))
                        {
                        PINFO(TEXT("Topic list requested\r\n"));

                        SetXactErr (hConv, 0, 0);

                        if (CF_TEXT == wFmt)
                            {
                            hDDEtmp = (HDDEDATA)GetTopicListA(hConv, TRUE);
                            }
                        else if (CF_UNICODETEXT == wFmt)
                            {
                            hDDEtmp = (HDDEDATA)GetTopicListW(hConv, TRUE);
                            }
                        else // Can't get the topiclist in anything but CF_TEXT or UNICODE
                            {
                            PERROR(TEXT("ClSrv\\DdeCB: Client asked for topics in bad fmt\r\n"));
                            hDDEtmp = (HDDEDATA)0;
                            }
                        }
                    else
                        {
                        #if DEBUG
                        TCHAR rgtch[128];

                        DdeQueryString(idInst, hszItem,rgtch, 128, CP_WINANSI);
                        PERROR(TEXT("item %s requested under system\n\r"), rgtch);
                        #endif
                        hDDEtmp = (HDDEDATA)0;
                        }
                    }
                else
                    {
                    // all other topics are assumed clipboard shares!!!

                    // Is format list the requested item?
                    if (!DdeCmpStringHandles (hszItem,  hszErrorRequest))
                        {
                        sprintf (gszXactErrStr,
                                 XERR_FORMAT,
                                 GetXactErrType (hConv),
                                 GetXactErr (hConv));

                        hDDEtmp = DdeCreateDataHandle (idInst,
                                                       gszXactErrStr,
                                                       lstrlen(gszXactErrStr)+sizeof(CHAR),
                                                       0,
                                                       hszErrorRequest,
                                                       wFmt,
                                                       0);

                        uiErr = DdeGetLastError (idInst);
                        }
                    else if (!DdeCmpStringHandles (hszItem, hszFormatList))
                        {
                        PINFO(TEXT("Getting format list\r\n"));

                        SetXactErr (hConv, 0, 0);

                        if (CF_TEXT == wFmt)
                            {
                            hDDEtmp = (HDDEDATA)GetFormatListA(hConv, hszTopic);
                            }
                        else
                            {
                            hDDEtmp = (HDDEDATA)GetFormatListW(hConv, hszTopic);
                            }
                        }
                    else
                        {   // request for specific format, or invalid
                        SetXactErr (hConv, 0, 0);

                        hDDEtmp = GetFormat ( hConv, hszTopic, hszItem );
                        }
                    }
                }
            break;


        case XTYP_ADVSTART:

            PINFO (TEXT("XTYP_ADVSTART\n"));

            // a-mgates 12/92- Added check for system topic.
            if (0 == DdeCmpStringHandles(hszItem, hszTopicList) &&
                0 == DdeCmpStringHandles(hszTopic, hszSysTopic))
                {
                PINFO(TEXT("Advise on topiclist OK\r\n"));
                hDDEtmp = (HDDEDATA)TRUE;
                }
            else
                {
                PERROR(TEXT("Advise loop requested on item other than topiclist\n\r"));
                hDDEtmp = (HDDEDATA)FALSE;
                }
            break;

        default:

            PINFO (TEXT("unknown wType %#x\n"), wType);

            break;
        }




    if (!(wType & XCLASS_NOTIFICATION))
        {
        RevertToSelf();
        }


    if (0L == hDDEtmp)
        {
        TCHAR atch[128];

        DdeQueryString(idInst, hszTopic, atch, 128, CP_WINANSI);
        PINFO(TEXT("Topic was %s, "), atch);
        DdeQueryString(idInst, hszItem, atch, 128, CP_WINANSI);
        PINFO(TEXT("item was %s\r\n"), atch);
        }



    PINFO(TEXT("Return %#x\n"), hDDEtmp);
    PINFO(TEXT("<<<< DdeCallback\n\n"));


    return hDDEtmp;

}












/*
 *      IsSupportedTopic
 */

BOOL IsSupportedTopic ( HSZ hszTopic )
{
pShrInfo p;

    if ( !DdeCmpStringHandles (hszTopic, hszSysTopic))
        {
        DdeKeepStringHandle ( idInst, hszTopic );
        return TRUE;
        }

    for ( p = SIHead; p; p = p->Next )
        {
        if (!DdeCmpStringHandles (hszTopic, p->hszName))
            return TRUE;
        }

    return FALSE;

}







/*
 *      CleanUpShares
 */

BOOL CleanUpShares ( VOID )
{
pShrInfo p, tmp;

    for (p=SIHead; p; p=tmp)
        {
        DdeFreeStringHandle ( idInst, p->hszName );

        #ifdef CACHEFORMATLIST
        PINFO(TEXT("freeing cached format list\n\r"));
        if ( p->hFormatList )
            DdeFreeDataHandle ( p->hFormatList );
        #endif

        #ifdef CACHEPREVIEWS
        PINFO(TEXT("freeing cached preview bitmap\n\r"));
        if ( p->hPreviewBmp )
            DdeFreeDataHandle ( p->hPreviewBmp );
        #endif

        tmp = p->Next;
        LocalFree ((HLOCAL) p );
        }


    SIHead = NULL;

    return TRUE;

}








/*
 *      CheckPageName
 *
 *  Check to see if the page name saved is
 *  a real DDE share.
 */

BOOL    CheckPageName (
    LPTSTR  ComputerName,
    LPTSTR  Name)
{
TCHAR   cTmp;
BOOL    bRet = FALSE;
UINT    uiRet;
DWORD   dwSize;
WORD    wItem;


    cTmp = Name[0];
    Name[0] = SHR_CHAR;

    wItem = 0;
    uiRet = NDdeShareGetInfo (ComputerName,
                              Name,
                              2,
                              NULL,
                              0,
                              &dwSize,
                              &wItem);

    Name[0] = cTmp;



    // No error if:
    //  1. no error
    //  2. buffer is too small; the share exists
    //  3. no access; share exists but we still need to
    //     let the user to see it on the list.


    if (uiRet == NDDE_NO_ERROR ||
        uiRet == NDDE_BUF_TOO_SMALL ||
        uiRet == NDDE_ACCESS_DENIED)
        return TRUE;
    else
        return FALSE;

}







/*
 *      InitShares
 */

BOOL InitShares (VOID)
{
TCHAR   szComputerName[MAX_COMPUTERNAME_LENGTH+3];
TCHAR   rgtchPageName[MAX_CLPSHRNAME+1];
TCHAR   rgtchPageFile[MAX_FILEPATH+1];
DWORD   dwPageSize;
DWORD   dwNameSize;
DWORD   dwType;
HKEY    hkeyClp;
DWORD   dwR;

unsigned iValue = 0;
unsigned iKeys = 0;



    CleanUpShares ();



    if (ERROR_SUCCESS != MakeTheDamnKey(&hkeyClp, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE))
        {
        PERROR(TEXT("Couldn't get to Clipbook root key\r\n"));
        return FALSE;
        }


    szComputerName[0] = szComputerName[1] = TEXT('\\');
    dwNameSize = MAX_COMPUTERNAME_LENGTH+1;
    GetComputerName (szComputerName+2, &dwNameSize);


    do  {

        dwNameSize = sizeof (rgtchPageName);
        dwPageSize = sizeof (rgtchPageFile);

        dwR = RegEnumValue (hkeyClp,
                            iValue,
                            rgtchPageName,
                            &dwNameSize,
                            NULL,
                            &dwType,
                            (LPBYTE)rgtchPageFile,
                            &dwPageSize);


        if (dwR == ERROR_SUCCESS)
            {
            rgtchPageName[dwNameSize] = 0;
            rgtchPageFile[dwPageSize] = 0;

            //if (CheckPageName (szComputerName, rgtchPageName))
            // this does not work in 548

                AddRecord (rgtchPageName,
                           rgtchPageFile,
                           (SHR_CHAR == rgtchPageName[0]) ? SIF_SHARED : 0);
            }


        iValue++;

        } while (dwR != ERROR_NO_MORE_ITEMS);




    RegCloseKey(hkeyClp);

    PINFO(TEXT("Read %d pages\r\n"),iKeys);


    return TRUE;

}








/*
 *      GetFormat
 */

HDDEDATA GetFormat (
    HCONV   hConv,
    HSZ     hszTopic,
    HSZ     hszItem )
{
HDDEDATA        hData    = 0l;
HANDLE          hClpData = NULL;
DWORD           cbData   = 0L;
pShrInfo        pshrinfo;
HANDLE          fh;
FORMATHEADER    FormatHeader;
unsigned        i;
TCHAR           szItemKey[CCHFMTNAMEMAX];
unsigned        cFormats;
BOOL            fPreviewRequested;
unsigned        fLocalAskedForLocal = 0;

#ifndef UNICODE
TCHAR           szFormatName[CCHFMTNAMEMAX * 2];
#endif




    PINFO(TEXT("Clsrv\\GetFormat:"));

    if (!DdeQueryString (idInst,hszItem,szItemKey,CCHFMTNAMEMAX,CP_WINANSI))
        {
        PERROR(TEXT("invalid item\n\r"));
        return 0;
        }




    // is the asked-for format cf_preview?
    fPreviewRequested = !lstrcmpi ( szItemKey, SZPREVNAME );



    // If the user asks for cf_link or cf_objectlink, we have to determine
    // whether they are local or not. If local, we need to give them cf_linkcopy
    // or cf_objectlinkcopy (which don't have NetDDE references).
    // if (!lstrcmpi(szItemKey, SZOBJECTLINK) && IsUserLocal(hConv))
    //    {
    //    lstrcpy(szItemKey, SZOBJECTLINKCOPY);
    //    fLocalAskedForLocal = ASKED_FOR_OBJECTLINK;
    //    }
    // else if (!lstrcmpi(szItemKey, SZLINK) && IsUserLocal(hConv))
    //    {
    //    lstrcpy(szItemKey, SZLINKCOPY);
    //    fLocalAskedForLocal = ASKED_FOR_LINK;
    //    };




    for ( pshrinfo=SIHead; pshrinfo; pshrinfo = pshrinfo->Next )
        {
        if ( DdeCmpStringHandles ( hszTopic, pshrinfo->hszName ) == 0 )
            {
            DdeKeepStringHandle ( idInst, hszTopic );

            if ( fPreviewRequested && pshrinfo->hPreviewBmp )
                {
                return pshrinfo->hPreviewBmp;
                }
            fh = CreateFileW(pshrinfo->szFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

            if ( INVALID_HANDLE_VALUE == fh)
                {
                PERROR(TEXT("ERROR opening %ls\n\r"), pshrinfo->szFileName );
                }
            else
                {
                cFormats = ReadFileHeader(fh);

                if (0 == cFormats)
                    {
                    PERROR(TEXT("Bad .CLP file\r\n"));
                    }

                for (i=0; i < cFormats; i++)
                    {
                    ReadFormatHeader(fh, &FormatHeader, i);


                    #ifndef UNICODE
                    WideCharToMultiByte (CP_ACP,
                                         0,
                                         FormatHeader.Name,
                                         CCHFMTNAMEMAX,
                                         szFormatName,
                                         CCHFMTNAMEMAX * 2,
                                         NULL,
                                         NULL);


                    if (!lstrcmpi (szItemKey, szFormatName))
                    #else
                    if (!lstrcmpi (szItemKey, FormatHeader.Name))
                    #endif
                        {
                        // Put back the format names, if a local client asked
                        // us for objectlink or link.
                        if (ASKED_FOR_OBJECTLINK == fLocalAskedForLocal)
                            {
                            lstrcpyW(FormatHeader.Name, LSZOBJECTLINK);
                            }
                        else if (ASKED_FOR_LINK == fLocalAskedForLocal)
                            {
                            lstrcpyW(FormatHeader.Name, LSZLINK);
                            }

                        hData = RenderRawFormatToDDE(&FormatHeader, fh);

                        #ifdef CACHEPREVIEWS
                        if ( fPreviewRequested )
                            {
                            PINFO(TEXT("GetFormat: caching preview\n\r"));
                            pshrinfo->hPreviewBmp = hData;
                            }
                        #endif

                        #if DEBUG
                        if (!hData)
                            {
                            PERROR(TEXT("RenderRawFormatToDDE resulted in 0 handle\n\r"));
                            }
                        #endif
                        }
                    }
                if (!hData)
                    {
                    PERROR(TEXT("GetFormat: requested format %s not found\n\r"),
                                   (LPSTR)szItemKey );
                    }
                CloseHandle(fh);
                }
            }
        }


    PINFO("Returning %lx",hData);

    return hData;
}









/*
 *      DelShare
 *
 *  Purpose: Delete a ClipBook page.
 *
 *  Parameters:
 *     pszName - The name of the page.
 *
 *  Returns:
 *     TRUE on success, FALSE on failure.
 */

BOOL DelShare(
    HCONV   hConv,
    TCHAR   *pszName)
{
pShrInfo    pshrinfo;
pShrInfo    q;
HKEY        hkeyClp;
TCHAR       atch[MAX_COMPUTERNAME_LENGTH + 3];
DWORD       dwLen = MAX_COMPUTERNAME_LENGTH + 1;
BOOL        fOK = FALSE;
DWORD       ret;
WCHAR       rgwchT[MAX_CLPSHRNAME + 1];
TCHAR       tch;




    assert(pszName);
    assert(*pszName);

    #ifndef UNICODE
    MultiByteToWideChar(CP_ACP, 0, pszName, -1, rgwchT, MAX_CLPSHRNAME + 1);
    #else
    lstrcpy(rgwchT, pszName);
    #endif

    PINFO(TEXT("Looking for %ls\r\n"), rgwchT);

    q = NULL;


    for (pshrinfo = SIHead; pshrinfo; pshrinfo = (q = pshrinfo)->Next)
        {
        assert(pshrinfo->szName);
        PINFO(TEXT("Comparing to %ls\r\n"), pshrinfo->szName);

        if (!lstrcmpW(pshrinfo->szName, rgwchT))
            {

            // Delete the Network DDE share for this item -- this also makes

            atch[0] = atch[1] = TEXT('\\');
            dwLen = MAX_COMPUTERNAME_LENGTH +1;

            GetComputerName(atch+2, &dwLen);
            tch = pszName[0];
            pszName[0] = SHR_CHAR;


            PINFO(TEXT("Deleting share %s on %s\r\n"), pszName, atch);

            ret = NDdeShareDel(atch, pszName, 0);
            pszName[0] = tch;

            if (NDDE_NO_ERROR == ret)
                {
                // Delete the key in the registry
                RevertToSelf();
                if (ERROR_SUCCESS == MakeTheDamnKey(&hkeyClp, KEY_SET_VALUE))
                    {
                    RegDeleteValue(hkeyClp, pszName);
                    RegCloseKey(hkeyClp);
                    }
                else
                    {
                    PERROR(TEXT("Couldn't delete key! #%ld\r\n"), GetLastError());
                    }

                DdeImpersonateClient(hConv);


                // force render all if applicable!
                SendMessage (hwndApp, WM_RENDERALLFORMATS, 0, 0L);

                // unlink file!
                DeleteFileW(pshrinfo->szFileName);

                // Take this page out of the linked list of pages.
                if (q == NULL)
                    {
                    SIHead = pshrinfo->Next;
                    }
                else
                    {
                    q->Next = pshrinfo->Next;
                    }

                DdeFreeStringHandle ( idInst, pshrinfo->hszName );

                if ( pshrinfo->hFormatList )
                    DdeFreeDataHandle ( pshrinfo->hFormatList );

                if ( pshrinfo->hPreviewBmp )
                    DdeFreeDataHandle ( pshrinfo->hPreviewBmp );

                LocalFree ( (HLOCAL)pshrinfo );

                #ifdef AUTOUPDATE
                  DdePostAdvise ( idInst, hszSysTopic, hszTopicList );
                #endif

                fOK = TRUE;
                }
            else
                {
                PERROR(TEXT("Csrv: NDde err %ld on delshare\r\n"), ret);
                SetXactErr (hConv, XERRT_NDDE, ret);
                }

            break; // Don't loop thru additional pages if you found the right one
            }
        }

    if (!fOK)
       {
       PERROR(TEXT("Clipsrv: item to delete '%s' not found\n\r"), pszName );
       }

    return fOK;
}








/*
 *      AddRecord
 *
 *  Purpose:
 *     Add a record to the linked list of Clipbook pages in memory.
 *
 *  Parameters:
 *     lpszName - Name of the page.
 *     lpszFileName - Name of the .CLP file containing the page's data.
 *     siflags - Flags for the page.
 *
 *  Returns:
 *     TRUE on success, FALSE on failure
 */

BOOL AddRecord (
    LPTSTR  lpszName,
    LPTSTR  lpszFileName,
    ULONG   siflags)
{
    pShrInfo pshrinfo;


    PINFO(TEXT("Making page %s with file %s\r\n"), lpszName, lpszFileName);

    pshrinfo = (pShrInfo) LocalAlloc ( LPTR, sizeof ( ShrInfo ) );


    if ( !pshrinfo )
        {
        PERROR(TEXT("AddRecord: LocalAlloc failed\n\r"));
        return FALSE;
        }


    if ( !( pshrinfo->hszName = DdeCreateStringHandle ( idInst, lpszName, 0 )))
        {
        PERROR(TEXT("AddRecord: DdeCHSZ fail\r\n"));

        LocalFree (pshrinfo);
        return(FALSE);
        }


    #ifdef UNICODE
      lstrcpy( pshrinfo->szFileName, lpszFileName );
      lstrcpy( pshrinfo->szName, lpszName );
    #else
      MultiByteToWideChar(CP_ACP, 0L, lpszFileName, -1, pshrinfo->szFileName, MAX_FILEPATH - 1);
      MultiByteToWideChar(CP_ACP, 0L, lpszName, -1, pshrinfo->szName, MAX_CLPSHRNAME);
    #endif


    PINFO(TEXT("Made page %ls with file %ls\r\n"), pshrinfo->szName, pshrinfo->szFileName);



    #ifdef CACHEFORMATLIST
     pshrinfo->hFormatList = 0L;
    #endif

    #ifdef CACHEPREVIEWS
     pshrinfo->hPreviewBmp = 0L;
    #endif

    pshrinfo->Next = SIHead;
    SIHead = pshrinfo;
    pshrinfo->flags = (WORD) siflags;


    return TRUE;

}






/*
 *      ResetRecord
 *
 *  When paste into an existing page, we need to clear the cached
 *  stuff for the old data.
 */

void    ResetRecord (pShrInfo   pInfo)
{

    // clear the format list

    if (pInfo->hFormatList)
        {
        DdeFreeDataHandle (pInfo->hFormatList);
        pInfo->hFormatList = NULL;
        }

    // clear the preview bitmap

    if (pInfo->hPreviewBmp)
        {
        DdeFreeDataHandle (pInfo->hPreviewBmp);
        pInfo->hPreviewBmp = NULL;
        }

}









/*
 *      GetShareFileName
 *
 */

pShrInfo    GetShareFileName (LPTSTR szSName, LPTSTR szFName)
{
BOOL        bRet = FALSE;
HSZ         hS;
CHAR        cSave;
pShrInfo    pInfo;



    cSave = *szSName;



    hS = DdeCreateStringHandle (idInst, szSName, 0);
    if (!hS)
        goto done;



    for (pInfo = SIHead; pInfo; pInfo = pInfo->Next)
        if (!DdeCmpStringHandles (pInfo->hszName, hS))
            {
            #ifndef UNICODE
                WideCharToMultiByte (CP_ACP,
                                     0,
                                     pInfo->szFileName,
                                     -1,
                                     szFName,
                                     MAX_PATH+1,
                                     NULL,
                                     NULL);
            #else
                lstrcpy (szFName, pInfo->szFileName);
            #endif

            bRet = TRUE;
            goto done;
            }



    DdeFreeStringHandle (idInst, hS);





    // not found, change share name to shared or unshared

    if (UNSHR_CHAR == cSave)
        *szSName = SHR_CHAR;
    else
        *szSName = UNSHR_CHAR;




    // try again with new share name

    hS = DdeCreateStringHandle (idInst, szSName, 0);
    if (!hS)
        goto done;



    for (pInfo = SIHead; pInfo; pInfo = pInfo->Next)
        if (!DdeCmpStringHandles (pInfo->hszName, hS))
            {
            #ifndef UNICODE
                WideCharToMultiByte (CP_ACP,
                                     0,
                                     pInfo->szFileName,
                                     -1,
                                     szFName,
                                     MAX_PATH+1,
                                     NULL,
                                     NULL);
            #else
                lstrcpy (szFName, pInfo->szFileName);
            #endif

            bRet = TRUE;
            goto done;
            }





done:

    if (hS)
        DdeFreeStringHandle (idInst, hS);


    *szSName = cSave;


    if (bRet)
        return pInfo;
    else
        return NULL;

}







/*
 *      AddShare
 *
 *  Purpose:
 *     Creates a new Clipbook page by doing this:
 *        - Save the current clipboard with some random file name.
 *        - Add the Clipbook page to the list in memory
 *        - Record the existence of the page in the Clipbook Server
 *           section of the registry. The value name is the page name,
 *           and the value is the filename.
 *
 *  Parameters:
 *     pszName - Name of the page.
 *     flags   - Flags to store with the page.
 *
 *  Returns:
 *     TRUE on success, FALSE on failure.
 */

DWORD AddShare(
    LPTSTR  pszName,
    WORD    flags)
{
TCHAR       szFName[MAX_PATH+1];
HKEY        hkeyClp;
DWORD       dwR = NO_ERROR;
pShrInfo    pInfo;



    fNTSaveFileFormat = TRUE;


    if (pInfo = GetShareFileName (pszName, szFName))
        {
        dwR = SaveClipboardToFile (hwndApp, pszName, szFName, TRUE);

        ResetRecord (pInfo);

        lstrcpy (szUpdateName, pszName);
        }
    else
        {
        dwR = GetRandShareFileName (szFName);
        if (dwR != NO_ERROR)
            return dwR;

        dwR = SaveClipboardToFile (hwndApp, pszName, szFName, TRUE);
        if (dwR != NO_ERROR)
            return dwR;


        if (!AddRecord ( pszName, szFName, flags ))
            return ERROR_NOT_ENOUGH_MEMORY;


        if (ERROR_SUCCESS == MakeTheDamnKey(&hkeyClp, KEY_SET_VALUE))
            {
            RegSetValueEx(hkeyClp, pszName, 0, REG_SZ, szFName,
                  lstrlen(szFName));
            RegCloseKey(hkeyClp);

            PINFO(TEXT("%s is being written...\n\r"), pszName);
            }
        }



    #ifdef AUTOUPDATE
      DdePostAdvise ( idInst, hszSysTopic, hszTopicList );
    #endif



    return dwR;

}








#if DEBUG

/*
 *      DumpShares
 */

VOID DumpShares (void)
{
char     buf[65];
pShrInfo pshrinfo;
int      i;
DWORD    cbRet;

    for ( i=0, pshrinfo = SIHead; pshrinfo; pshrinfo = pshrinfo->Next, i++ )
        {
        PINFO(TEXT("---------- Share %d  flags:%x-------------\n\r"), i, pshrinfo->flags );
        cbRet = DdeQueryString ( idInst, pshrinfo->hszName, buf, 128L, CP_WINANSI );
        PINFO(TEXT("name: >%s<\n\r"), (LPSTR)pshrinfo->szName );
        PINFO(TEXT("hsz:  >%s<\n\r"), cbRet? (LPSTR)buf : (LPSTR)TEXT("ERROR") );
        }

}
#else
VOID DumpShares (void)
{
}
#endif








/*
 *      MarkShare
 *
 *  Purpose: Mark a Clipbook page as shared or unshared.
 *
 *  Parameters:
 *     pszName - Name of the page.
 *     flags   - 0 for "unshared", SIF_SHARED for "shared."
 *
 *  Returns:
 *     TRUE on success, FALSE on failure.
 *
 */

BOOL MarkShare(
    TCHAR   *pszName,
    WORD    flags)
{
PSECURITY_DESCRIPTOR pSDShare;
pShrInfo    pshrinfo;
HKEY        hkeyClp;
INT         iTmp;

ACCESS_ALLOWED_ACE *pace;
WCHAR       rgwchT[MAX_CLPSHRNAME+1];
UINT        ret;
DWORD       dwBytes = sizeof(pSDShare);
WORD        wItems = 0;
PACL        Acl;
BOOL        fDacl;
BOOL        fDefault;
DWORD       i;


#ifndef UNICODE
    MultiByteToWideChar(CP_ACP, 0, pszName, -1, rgwchT, MAX_CLPSHRNAME);
#endif





    PINFO(TEXT("Entering MarkShare\r\n"));

    for (pshrinfo = SIHead; pshrinfo; pshrinfo = pshrinfo->Next)
        {

        #ifdef UNICODE
        iTmp = lstrcmpW (pshrinfo->szName+1, pszName+1);
        #else
        iTmp = lstrcmpW (pshrinfo->szName+1, rgwchT+1);
        #endif

        if (!iTmp)
            {
            PINFO(TEXT("MarkShare: marking %s %d\n\r"), (LPSTR)pszName, flags );

            // If the name's changing, need to delete old reg key.
            // (We make the new one after hitting the file security.)
            if ((pshrinfo->flags & SIF_SHARED) != (flags & SIF_SHARED))
                {
                PINFO(TEXT("Changing shared status\r\n"));

                // Delete the registry item with the old name
                if (ERROR_SUCCESS == MakeTheDamnKey(&hkeyClp, KEY_SET_VALUE))
                    {
                    PINFO(TEXT("Deleting old name %ws\r\n"),pshrinfo->szName);
                    RegDeleteValueW(hkeyClp, pshrinfo->szName);
                    RegCloseKey(hkeyClp);
                    }
                else
                    {
                    PERROR(TEXT("MarkShare: Couldn't open registry!\r\n"));
                    }
                }


            // Set name to reflect shared/unshared status
            pshrinfo->szName[0] = (flags & SIF_SHARED) ? SHR_CHAR : UNSHR_CHAR;
            pshrinfo->flags = flags;



            // Sync the security on the Clipbook page file to be
            // analogous to the security set on the NetDDE share.
            pszName[0] = SHR_CHAR;
            NDdeGetShareSecurity(NULL, pszName, DACL_SECURITY_INFORMATION, NULL, 0, &i);

            PINFO(TEXT("Getting security %ld bytes\r\n"), i);


            if (!(pSDShare = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, i)))
                {
                PERROR(TEXT("LocalAlloc fail\r\n"));
                }
            else
                {
                ret = NDdeGetShareSecurity (NULL,
                                            pszName,
                                            DACL_SECURITY_INFORMATION,
                                            pSDShare,
                                            i,
                                            &i);

                if (NDDE_NO_ERROR != ret)
                    {
                    PERROR(TEXT("Couldn't get sec #%ld\r\n"), ret);
                    }
                else if (!GetSecurityDescriptorDacl(pSDShare, &fDacl, &Acl, &fDefault))
                    {
                    PERROR(TEXT("GetDACL fail %ld\r\n"), GetLastError());
                    }
                else
                    {
                    DWORD dwGeneric;

                    for (i = 0; GetAce(Acl, i, &pace); i++)
                        {
                        dwGeneric = 0L;


                        // Convert NDDE access mask types to generic access
                        // mask types
                        if (ACCESS_ALLOWED_ACE_TYPE == pace->Header.AceType ||
                            ACCESS_DENIED_ACE_TYPE == pace->Header.AceType)
                            {
                            if ((pace->Mask & NDDE_SHARE_READ) == NDDE_SHARE_READ)
                                {
                                PINFO(TEXT("R"));
                                dwGeneric |= GENERIC_READ;
                                }

                            if ((pace->Mask & NDDE_SHARE_WRITE) == NDDE_SHARE_WRITE)
                                {
                                PINFO(TEXT("R"));
                                dwGeneric |= GENERIC_WRITE;
                                }

                            if ((pace->Mask & NDDE_GUI_CHANGE) == NDDE_GUI_CHANGE)
                                {
                                PINFO(TEXT("D"));
                                dwGeneric |= GENERIC_WRITE|GENERIC_READ|DELETE;
                                }

                            if ((pace->Mask & NDDE_GUI_FULL_CONTROL) ==
                                  NDDE_GUI_FULL_CONTROL)
                                {
                                PINFO(TEXT("A"));
                                dwGeneric |= GENERIC_ALL;
                                }

                            PINFO(TEXT(" = %ld\r\n"), dwGeneric);
                            pace->Mask = dwGeneric;
                            }
                        else
                            {
                            PERROR(TEXT("Invalid ACE type!!!\r\n"));
                            }
                        }

                    ret = SetFileSecurityW (pshrinfo->szFileName,
                                            DACL_SECURITY_INFORMATION,
                                            pSDShare);

                    if (FALSE == ret)
                        {
                        PERROR(TEXT("SetFSec err %ld\r\n"), GetLastError());
                        }

                    }

                LocalFree(pSDShare);
                }


            DdeFreeStringHandle ( idInst, pshrinfo->hszName );
            pshrinfo->hszName = DdeCreateStringHandleW( idInst, pshrinfo->szName,
                  CP_WINUNICODE);

            if ( !pshrinfo->hszName )
                {
                PERROR(TEXT("DdeCreateStringHandle failed\n\r"));
                }
            else
                {
                // update the registry to show shared/unshared status
                if (ERROR_SUCCESS == MakeTheDamnKey(&hkeyClp, KEY_SET_VALUE))
                    {
                    PINFO(TEXT("Making registry key %ls from %ls, %d\r\n"),
                          pshrinfo->szName, pshrinfo->szFileName,
                          lstrlenW(pshrinfo->szFileName));

                    RegSetValueExW (hkeyClp,
                                    pshrinfo->szName,
                                    0,
                                    REG_SZ,
                                    (LPBYTE)pshrinfo->szFileName,
                                    lstrlenW (pshrinfo->szFileName) *sizeof(WCHAR) +sizeof(WCHAR));

                    RegCloseKey(hkeyClp);

                    DdePostAdvise ( idInst, hszSysTopic, hszTopicList );
                    return TRUE;
                    }
                else
                    {
                    PERROR(TEXT("Could not make registry key to record %s"),
                           pshrinfo->szName);
                    }
                }
            }
        }




    PERROR(TEXT("Item to mark '%s' not found\n\r"), pszName );

    return FALSE;

}









/*
 *      Hszize
 *  This creates often used global hszs from standard global strings.
 *  It also fills the hsz fields of the topic and item tables.
 *
 */

void Hszize(void)
{

    hszAppName      = DdeCreateStringHandle (idInst, szServer,             0L);
    hszSysTopic     = DdeCreateStringHandle (idInst, SZDDESYS_TOPIC,       0L);
    hszTopicList    = DdeCreateStringHandle (idInst, SZDDESYS_ITEM_TOPICS, 0L);
    hszFormatList   = DdeCreateStringHandle (idInst, SZ_FORMAT_LIST,       0L);
    hszErrorRequest = DdeCreateStringHandle (idInst, SZ_ERR_REQUEST, 0L);

    #ifdef DEBUG
    if ( !hszAppName || !hszSysTopic || !hszTopicList || !hszFormatList )
        {
        PERROR(TEXT("error creating HSZ constants\n\r"));
        }
    #endif
}




/*
 *      UnHszize
 */

void UnHszize(void)
{
    DdeFreeStringHandle (idInst, hszAppName);
    DdeFreeStringHandle (idInst, hszSysTopic);
    DdeFreeStringHandle (idInst, hszTopicList);
    DdeFreeStringHandle (idInst, hszFormatList);
    DdeFreeStringHandle (idInst, hszErrorRequest);
}







/*
 *      GetRandShareFileName
 *
 *  Purpose:
 *     Generate a random share file name in the Windows directory.
 *
 *  Parameters:
 *     buf - Buffer to place the file name in.
 *
 *  Returns:
 *     TRUE if a valid filename was found, or FALSE if all of the random
 *     filenames are taken up.
 */

DWORD GetRandShareFileName (
    LPTSTR  buf)
{
TCHAR   szWinDir[144];
BOOL    IsUnique = FALSE;
WORD    rand;
WORD    cTry = 0;
HANDLE  hFile;


    if (!GetWindowsDirectory( szWinDir, 144))
        {
        return GetLastError();
        }

    rand = (WORD)GetTickCount() % 10000;

    do  {
        wsprintf ( buf, TEXT("%s\\CBK%04d.CLP"), szWinDir, rand++ );
        hFile = CreateFile (buf,
                            GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_NEW,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

        }while (INVALID_HANDLE_VALUE == hFile && cTry++ < 10000);


    if (INVALID_HANDLE_VALUE == hFile)
        {
        PERROR(TEXT("GetRandShareFileName: More than 1000 clipbook file exist!\r\n"));
        return GetLastError();
        }
    else
        {
        CloseHandle(hFile);
        return NO_ERROR;
        }

}
