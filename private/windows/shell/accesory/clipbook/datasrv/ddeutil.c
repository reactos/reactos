

#include    <windows.h>
#include    <string.h>
#include    <stdlib.h>
#include    <stdio.h>
#include    <memory.h>
#include    <ddeml.h>

#include    "common.h"

#include    "clipfile.h"
#include    "clipsrv.h"
#include    "ddeutil.h"
#include    "callback.h"
#include    "debugout.h"







/*
 *      GetTopicListA
 *
 *  Purpose: Creata a DDE handle to a block containing a tab-delimited
 *     list of the available topics with a NULL at the end.
 *
 *  Parameters:
 *     fAllTopicReq - TRUE to get all topics, FALSE to just get shared.
 *
 *  Returns: The data handle.
 *
 *  Notes: AW variants
 */

HDDEDATA GetTopicListA(
    HCONV   hConv,
    BOOL    fAllTopicReq)
{
LPSTR       lpszTopics;
LPSTR       pTmp;
HDDEDATA    hData;
DWORD       cbData = 0L;
pShrInfo    pshrinfo;



    for ( pshrinfo=SIHead; pshrinfo; pshrinfo = pshrinfo->Next )
        {
       /*
        * Keep enought buffer for MBCS string,
        * Should I use nls function to compute MBCS length ?.
        * but go easy way.
        */
        cbData += ( ( lstrlenW( pshrinfo->szName ) + 1 ) * sizeof(WORD) );
        }

        cbData += sizeof(szUpdateName) +1;


    // if there are no entries, must send a "" string, not 0!

    if ( !cbData )
        return (DdeCreateDataHandle (idInst, TEXT(""), 1, 0L, hszTopicList, CF_TEXT, 0));





    hData = DdeCreateDataHandle (idInst, NULL, cbData, 0L, hszTopicList, CF_TEXT, 0);


    if ( !hData )
        {
        SetXactErr (hConv, XERRT_DDE, DdeGetLastError (idInst));
        PERROR(TEXT("error creating hddedata for topic list\n\r"));
        }
    else
        {
        if ( ( pTmp = lpszTopics = DdeAccessData(hData, NULL) ) == (LPSTR)NULL )
            {
            SetXactErr (hConv, XERRT_DDE, DdeGetLastError (idInst));
            PERROR(TEXT("error accessing data handle for topic list\n\r"));

            DdeFreeDataHandle(hData);
            hData = 0L;
            }
        else
            {
            // put the updated name first
            #ifdef UNICODE
              wsprintfA (pTmp, "%lc%ls\t", BOGUS_CHAR, szUpdateName);
            #else
              wsprintfA (pTmp, "%hc%hs\t", BOGUS_CHAR, szUpdateName);
            #endif

            pTmp += lstrlenA (pTmp);


            // Create a tab-delimited list of topics in CF_TEXT format.
            for ( pshrinfo=SIHead; pshrinfo; pshrinfo = pshrinfo->Next )
                {
                // Only list SHARED topics if this isn't a "[alltopiclist]" req
                if (SHR_CHAR == pshrinfo->szName[0] ||
                    fAllTopicReq)
                    {

                    WideCharToMultiByte (CP_ACP,
                                         0,
                                         pshrinfo->szName,
                                         -1,
                                         pTmp,
                                         (int)(cbData - (size_t)(pTmp - lpszTopics)),
                                         NULL,
                                         NULL);


                    pTmp += lstrlenA(pTmp);
                    *pTmp++ = '\t';
                    }
                }

            // Turn the last tab into a NULL.
            if ( pTmp != lpszTopics )
                *--pTmp = '\0';

            DdeUnaccessData ( hData );
            }
        }


    return hData;
}








/*
 *      GetTopicListW
 */

HDDEDATA GetTopicListW(
    HCONV   hConv,
    BOOL    fAllTopicsReq)
{
LPWSTR      lpszTopics;
LPWSTR      pTmp;
HDDEDATA    hData;
DWORD       cbData = 0L;
pShrInfo    pshrinfo;

    for (pshrinfo=SIHead; pshrinfo; pshrinfo = pshrinfo->Next)
        {
       /*
        * Keep enought buffer for MBCS string,
        * Should I use nls function to compute MBCS length ?.
        * but go easy way.
        */
        cbData += ( ( lstrlenW( pshrinfo->szName ) + 1 ) * sizeof(WORD) );

        }

        cbData += sizeof(szUpdateName)/sizeof(TCHAR) +1;

    // if there are no entries, must send a "" string, not 0!

    if ( !cbData )
        return DdeCreateDataHandle (idInst, TEXT(""), 1, 0L, hszTopicList, CF_UNICODETEXT, 0);


    hData = DdeCreateDataHandle (idInst, NULL, cbData, 0L, hszTopicList, CF_UNICODETEXT, 0);


    if ( !hData )
        {
        SetXactErr (hConv, XERRT_DDE, DdeGetLastError(idInst));
        PERROR(TEXT("error creating hddedata for topic list\n\r"));
        }
    else
        {
        if ((pTmp = lpszTopics = (LPWSTR)DdeAccessData( hData, NULL ))
                 == (LPWSTR)NULL)
            {
            SetXactErr (hConv, XERRT_DDE, DdeGetLastError(idInst));
            PERROR(TEXT("error accessing data handle for topic list\n\r"));

            DdeFreeDataHandle(hData);
            hData = 0L;
            }
        else
            {

            // put the updated name first
            #ifdef UNICODE
              wsprintfW (pTmp, L"%lc%ls\t", BOGUS_CHAR, szUpdateName);
            #else
              wsprintfW (pTmp, L"%hc%hs\t", BOGUS_CHAR, szUpdateName);
            #endif

            pTmp += lstrlenW (pTmp);
            // Create a tab-delimited list of topics in CF_TEXT format.
            for (pshrinfo=SIHead; pshrinfo; pshrinfo = pshrinfo->Next )
                {
                if (SHR_CHAR == pshrinfo->szName[0] ||
                    fAllTopicsReq)
                    {
                    lstrcpyW( pTmp, pshrinfo->szName );
                    pTmp += lstrlenW(pTmp);
                    *pTmp++ = '\t';
                    }
                }

            // Turn the last tab into a NULL.
            if ( pTmp != lpszTopics )
                *--pTmp = '\0';

            DdeUnaccessData ( hData );
            }
        }


    return hData;

}








/*
 *      GetFormatListA
 *
 *  Purpose: Create a DDE handle containing a tab-delimited list of the
 *     formats available for the given topic -- it's assumed to be a share.
 *
 *  Parameters:
 *     hszTopic - String handle to the topic name.
 *
 *  Returns:
 *     DDE handle to the list.
 *
 *  Notes:
 *     AW variants
 */

HDDEDATA GetFormatListA(
    HCONV   hConv,
    HSZ     hszTopic )
{
LPSTR           lpszFormats;
LPSTR           lpcsTmp;
HDDEDATA        hData = 0L;
DWORD           cbData = 0L;
pShrInfo        pshrinfo;
unsigned        cFormats;
FORMATHEADER    FormatHeader;
unsigned        i;
HANDLE          fh;



    PINFO(TEXT("GetFormatList:"));

    for ( pshrinfo=SIHead; pshrinfo; pshrinfo = pshrinfo->Next )
        {
        if ( DdeCmpStringHandles ( hszTopic, pshrinfo->hszName ) == 0 )
            {

            #ifdef CACHEFORMATLIST
            if (pshrinfo->hFormatList)
                return pshrinfo->hFormatList;
            #endif


            fh = CreateFileW (pshrinfo->szFileName,
                              GENERIC_READ,
                              0,
                              NULL,
                              OPEN_EXISTING,
                              0,
                              NULL);


            if ( INVALID_HANDLE_VALUE == fh)
                {
                SetXactErr (hConv, XERRT_SYS, GetLastError());
                PERROR(TEXT("ERROR opening %s\n\r"), pshrinfo->szFileName);
                }
            else
                {
                cFormats = ReadFileHeader(fh);

                // allocate max data - some will be wasted...
                // It's this or go through the loop twice...

                /*
                 * Keep enought buffer for MBCS string,
                 * Should I use nls function to compute MBCS length ?.
                 * but go easy way.
                 */
                 cbData = cFormats * CCHFMTNAMEMAX * sizeof(WORD);
               hData = DdeCreateDataHandle (idInst,
                                             NULL,
                                             cbData,
                                             0L,
                                             hszFormatList,
                                             CF_TEXT,
                                           #ifdef CACHEFORMATLIST
                                             HDATA_APPOWNED );
                                           #else
                                             0 );
                                           #endif

                if (!hData)
                    {
                    SetXactErr (hConv, XERRT_DDE, DdeGetLastError (idInst));
                    PERROR(TEXT("DdeCreateDataHandle failed!!!\n\r"));
                    }
                else
                    {
                    lpszFormats = DdeAccessData(hData, NULL);
                    lpcsTmp = lpszFormats;
                    if (NULL == lpcsTmp)
                        {
                        SetXactErr (hConv, XERRT_DDE, DdeGetLastError (idInst));
                        DdeFreeDataHandle ( hData );
                        hData = 0L;
                        PERROR(TEXT("DdeAccessData failed!!!\n\r"));
                        }
                    else
                        {
                        PINFO(TEXT("%d formats found\n\r"), cFormats);

                        // form tab-separated list

                        for (i=0; i < cFormats; i++)
                            {
                            ReadFormatHeader(fh, &FormatHeader, i);
                            PINFO(TEXT("getformat: read >%ws<\n\r"), FormatHeader.Name);
                            WideCharToMultiByte (CP_ACP,
                                                 0,
                                                 FormatHeader.Name,
                                                 -1,
                                                 lpcsTmp,
                                                 cbData,
                                                 NULL,
                                                 NULL);


                            //  lstrcpyW( lpcsTmp, FormatHeader.Name );
                            lpcsTmp += lstrlenA(lpcsTmp);
                            *lpcsTmp++ = '\t';
                            }

                       *--lpcsTmp = '\0';

                       PINFO(TEXT("clipsrv: returning format list >%cs<\n\r"), lpszFormats );
                       DdeUnaccessData ( hData );

                       #ifdef CACHEFORMATLIST
                         pshrinfo->hFormatList = hData;
                       #endif
                       }
                   }
                CloseHandle(fh);
                }
            }
        }


    if (!hData)
        {
        PERROR (TEXT("GetFormatList: Topic not found\n\r"));
        }


    return hData;

}









/*
 *      GetFormatListW
 */

HDDEDATA GetFormatListW(
    HCONV   hConv,
    HSZ     hszTopic )
{
LPWSTR          lpszFormats;
LPWSTR          lpwsTmp;
HDDEDATA        hData = 0L;
DWORD           cbData = 0L;
pShrInfo        pshrinfo;
unsigned        cFormats;
FORMATHEADER    FormatHeader;
unsigned        i;
HANDLE          fh;


    PINFO(TEXT("GetFormatList:"));



    for ( pshrinfo=SIHead; pshrinfo; pshrinfo = pshrinfo->Next )
        {
        if ( DdeCmpStringHandles ( hszTopic, pshrinfo->hszName ) == 0 )
            {

            #ifdef CACHEFORMATLIST
            if ( pshrinfo->hFormatList )
                return pshrinfo->hFormatList;
            #endif

            fh = CreateFileW (pshrinfo->szFileName,
                              GENERIC_READ,
                              0,
                              NULL,
                              OPEN_EXISTING,
                              0,
                              NULL);


            if ( INVALID_HANDLE_VALUE == fh)
                {
                SetXactErr (hConv, XERRT_SYS, GetLastError());
                PERROR(TEXT("ERROR opening %s\n\r"), pshrinfo->szFileName);
                }
            else
                {
                cFormats = ReadFileHeader(fh);

                // allocate max data - some will be wasted...
                // It's this or go through the loop twice...
               /*
                * Keep enought buffer for MBCS string,
                * Should I use nls function to compute MBCS length ?.
                * but go easy way.
              */
                cbData = cFormats * CCHFMTNAMEMAX * sizeof(WORD);

                hData = DdeCreateDataHandle (idInst,
                                             NULL,
                                             cbData,
                                             0L,
                                             hszFormatList,
                                             CF_TEXT,
                                           #ifdef CACHEFORMATLIST
                                             HDATA_APPOWNED );
                                           #else
                                             0);
                                           #endif

                if ( !hData )
                    {
                    SetXactErr (hConv, XERRT_DDE, DdeGetLastError(idInst));
                    PERROR(TEXT("DdeCreateDataHandle failed!!!\n\r"));
                    }
                else
                    {
                    lpszFormats = (LPWSTR)DdeAccessData(hData, NULL);
                    lpwsTmp = lpszFormats;
                    if (NULL == lpwsTmp)
                        {
                        SetXactErr (hConv, XERRT_DDE, DdeGetLastError(idInst));

                        DdeFreeDataHandle ( hData );
                        hData = 0L;
                        PERROR(TEXT("DdeAccessData failed!!!\n\r"));
                        }
                    else
                        {
                        PINFO(TEXT("%d formats found\n\r"), cFormats);

                        // form tab-separated list

                        for (i=0; i < cFormats; i++)
                            {
                            ReadFormatHeader (fh, &FormatHeader, i);
                            PINFO(TEXT("getformat: read >%ws<\n\r"), FormatHeader.Name);

                            lstrcpyW (lpwsTmp, FormatHeader.Name);
                            lpwsTmp += lstrlenW (FormatHeader.Name);
                            *lpwsTmp++ = '\t';
                            }

                        *--lpwsTmp = '\0';

                        PINFO(TEXT("clipsrv: returning format list >%ws<\n\r"), lpszFormats );
                        DdeUnaccessData ( hData );


                        #ifdef CACHEFORMATLIST
                        pshrinfo->hFormatList = hData;
                        #endif
                        }
                    }

                CloseHandle(fh);
                }
            }
        }



    if (!hData)
        {
        PERROR (TEXT("GetFormatList: Topic not found\n\r"));
        }



    return hData;

}
