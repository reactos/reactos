//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       winspool.cxx
//
//  Contents:   Wrappers for non-unicode winspool functions
//
//----------------------------------------------------------------------------

#include "precomp.hxx"

#ifndef X_WINSPOOL_H_
#define X_WINSPOOL_H_
#include "winspool.h"
#endif

#ifndef X_UNICWRAP_HXX_
#define X_UNICWRAP_HXX_
#include "unicwrap.hxx"
#endif

#ifdef UNIX
DYNLIB g_dynlibWINSPOOL = { NULL, NULL, "MW32.DLL" };
#else
DYNLIB g_dynlibWINSPOOL = { NULL, NULL, "WINSPOOL.DRV" };
#endif 

#define WRAPIT_(type, fn, a1, a2)\
type WINAPI fn a1\
{\
    static DYNPROC s_dynproc##fn = { NULL, &g_dynlibWINSPOOL, #fn };\
    if(THR(LoadProcedure(&s_dynproc##fn)))\
        return FALSE;\
    return ((*(type (APIENTRY *) a1)s_dynproc##fn.pfn) a2);\
}
#define WRAPIT(fn, a1, a2) WRAPIT_(BOOL, fn, a1, a2)

#define DOCINFO(x) (Level==1)?((DOC_INFO_1W *)pDocInfo)->x:((DOC_INFO_2W *)pDocInfo)->x
#define DOCINFO2(x) (Level==1)?0:((DOC_INFO_2W *)pDocInfo)->x
#define JOBINFO1W(x) ((JOB_INFO_1W *)pJob)->x
#define JOBINFO1A(x) ((JOB_INFO_1A *)pJobA)->x
#define COPY_JOBINFO1A_MEMBER(x)\
    if (JOBINFO1A(x))\
    {\
        strcpy((LPSTR) strOutString, JOBINFO1A(x));\
        JOBINFO1W(x) = pNextString;\
    }\
    else\
    {\
        JOBINFO1W(x) = NULL;\
    }
#define COPY_JOBINFO1W_MEMBER(x)\
    memcpy(((LPBYTE)pJobA)+cbTaken, x##A, x##A.strlen()+1);\
    JOBINFO1A(x) = ((LPBYTE)pJobA)+cbTaken;\
    cbTaken += x##A.strlen()+1;



WRAPIT(OpenPrinterA,
    (LPSTR pPrinterName, LPHANDLE phPrinter, LPPRINTER_DEFAULTSA pDefault),
    (pPrinterName, phPrinter, pDefault));

BOOL WINAPI OpenPrinterW(
   LPWSTR    pPrinterName,
   LPHANDLE phPrinter,
   LPPRINTER_DEFAULTSW pDefault
)
{
    Assert(!pDefault && "Default parameter not supported in OpenPrinter wrapper.  See src\\core\\wrappers\\winspool.cxx");
    CStrIn strInPrinterName(pPrinterName);

    return OpenPrinterA(strInPrinterName, phPrinter, NULL);
}

WRAPIT(ClosePrinter, (HANDLE hPrinter), (hPrinter));

/* if we ever need a wrapper for SetPrinter (Level 0), uncomment this
WRAPIT(SetPrinterA,
    (HANDLE hPrinter, DWORD Level, LPBYTE pPrinter, DWORD Command),
    (hPrinter, Level, pPrinter, Command));

BOOL WINAPI SetPrinterW(
   HANDLE hPrinter,
   DWORD Level,
   LPBYTE pPrinter,
   DWORD Command
)
{
    Assert(!Level && !pPrinter && "Only level 0 supported in SetPrinter wrapper.  See src\\core\\wrappers\\winspool.cxx");

    return SetPrinterA(hPrinter, 0, NULL, Command);
}
*/

WRAPIT_(DWORD, StartDocPrinterA,
    (HANDLE hPrinter, DWORD Level, LPBYTE pDocInfo),
    (hPrinter, Level, pDocInfo));

DWORD
WINAPI
StartDocPrinterW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pDocInfo
)
{
    if (!pDocInfo || Level < 1 || Level > 2)
    {
        return 0;
    }

    // Need to convert pDocInfo strings
    CStrIn strInDocName( DOCINFO(pDocName) );
    CStrIn strInOutputFile( DOCINFO(pOutputFile) );
    CStrIn strInDatatype( DOCINFO(pDatatype) );
    DOC_INFO_1A DocInfo1A = { strInDocName, strInOutputFile, strInDatatype };
    DOC_INFO_2A DocInfo2A = { strInDocName, strInOutputFile, strInDatatype, DOCINFO2(dwMode), DOCINFO2(JobId) };

    return StartDocPrinterA(hPrinter, Level, (Level==1) ? (LPBYTE)&DocInfo1A : (LPBYTE)&DocInfo2A );
}

WRAPIT(EndDocPrinter, (HANDLE hPrinter), (hPrinter));

WRAPIT(GetJobA,
    (HANDLE hPrinter, DWORD JobId, DWORD Level, LPBYTE pJob, DWORD cbBuf, LPDWORD pcbNeeded),
    (hPrinter, JobId, Level, pJob, cbBuf, pcbNeeded));

BOOL
WINAPI
GetJobW(
   HANDLE   hPrinter,
   DWORD    JobId,
   DWORD    Level,
   LPBYTE   pJob,
   DWORD    cbBuf,
   LPDWORD  pcbNeeded
)
{
    // Allocate another buffer whose length matches the buffer passed in in non-unicode (byte) units.
    // That way we hope to simply propagate any buffer size errors in and out.
    DWORD cbBufA = cbBuf/sizeof(TCHAR) - sizeof(JOB_INFO_1W)/sizeof(TCHAR) + sizeof(JOB_INFO_1A);
    LPBYTE pJobA = (BYTE *)MemAlloc(Mt(Mem), cbBufA);
    DWORD cbNeededA;
    BOOL fResult = FALSE;

    Assert(Level == 1 && "GetJob wrapper only supports Level 1 in wrapper.  See src\\core\\wrappers\\winspool.cxx");
    Assert(pJob && pcbNeeded);

    if (!pJobA)
    {
        goto Cleanup;
    }

    fResult = GetJobA(hPrinter, JobId, Level, pJobA, cbBufA, &cbNeededA);

    if (!fResult)
    {
        goto Cleanup;
    }
    else
    {
        DWORD cbTaken = sizeof(JOB_INFO_1W);
        LONG index;

        // Copy the JOBINFO structure.
        memcpy(pJob, pJobA, sizeof(JOB_INFO_1W));

        // Copy and convert all output strings.
        for ( index = 1 ; index <= 6 ; index++ )
        {
            LPWSTR pNextString = (LPWSTR)pJob + cbTaken;
            {
                CStrOut strOutString(pNextString, cbBuf - cbTaken);

                switch (index)
                {
                case 1:
                    COPY_JOBINFO1A_MEMBER(pPrinterName)
                    break;
                case 2:
                    COPY_JOBINFO1A_MEMBER(pMachineName)
                    break;
                case 3:
                    COPY_JOBINFO1A_MEMBER(pUserName)
                    break;
                case 4:
                    COPY_JOBINFO1A_MEMBER(pDocument)
                    break;
                case 5:
                    COPY_JOBINFO1A_MEMBER(pDatatype)
                    break;
                case 6:
                    COPY_JOBINFO1A_MEMBER(pStatus)
                    break;
                }

                cbTaken += strOutString.ConvertIncludingNul();
            }
        }
    }

    // Convert pcbNeeded back to unicode units.
    *pcbNeeded = cbNeededA*sizeof(TCHAR) - sizeof(JOB_INFO_1A)*sizeof(TCHAR) + sizeof(JOB_INFO_1W);

Cleanup:

    if (pJobA)
    {
        MemFree(pJobA);
    }

    return fResult;
}


WRAPIT(SetJobA, (HANDLE hPrinter, DWORD JobId, DWORD Level, LPBYTE Job, DWORD Command), (hPrinter, JobId, Level, Job, Command));

BOOL
WINAPI
SetJobW(
    HANDLE hPrinter,
    DWORD JobId,
    DWORD Level,
    LPBYTE Job,
    DWORD Command)
{
    Assert(Level == 0 && Job == NULL && "SetJob wrapper only supports Level 0 in wrapper.  See src\\core\\wrappers\\winspool.cxx");

    return SetJobA(hPrinter, JobId, 0, NULL, Command);

/* If we ever need SetJob at level 1, we can use this code:
    else if (Level == 1)
    {
        LPBYTE pJob = Job;
        CStrIn pPrinterNameA(JOBINFO1W(pPrinterName));
        CStrIn pMachineNameA(JOBINFO1W(pMachineName));
        CStrIn pUserNameA(JOBINFO1W(pUserName));
        CStrIn pDocumentA(JOBINFO1W(pDocument));
        CStrIn pDatatypeA(JOBINFO1W(pDatatype));
        CStrIn pStatusA(JOBINFO1W(pStatus));
        BYTE   pJobA[sizeof(JOB_INFO_1A) + 6*MAX_PATH];
        DWORD  cbTaken = sizeof(JOB_INFO_1A);
        LONG   index;

        Assert(Job);

        memcpy(pJobA, pJob, sizeof(JOB_INFO_1A));

        // Copy and convert all output strings.
        for ( index = 1 ; index <= 6 ; index++ )
        {
            switch (index)
            {
            case 1:
                COPY_JOBINFO1W_MEMBER(pPrinterName)
                break;
            case 2:
                COPY_JOBINFO1W_MEMBER(pMachineName)
                break;
            case 3:
                COPY_JOBINFO1W_MEMBER(pUserName)
                break;
            case 4:
                COPY_JOBINFO1W_MEMBER(pDocument)
                break;
            case 5:
                COPY_JOBINFO1W_MEMBER(pDatatype)
                break;
            case 6:
                COPY_JOBINFO1W_MEMBER(pStatus)
                break;
            }
        }

        return SetJobA(hPrinter, JobId, 1, pJobA, Command);
    }
    */
}


WRAPIT_(LONG, DocumentPropertiesA,
    (HWND hWnd, HANDLE hPrinter, LPSTR pDeviceName, PDEVMODEA pDevModeOutput, PDEVMODEA pDevModeInput, DWORD fMode),
    (hWnd, hPrinter, pDeviceName, pDevModeOutput, pDevModeInput, fMode));

LONG
WINAPI
DocumentPropertiesW(
    HWND hWnd,
    HANDLE hPrinter,
    LPWSTR pDeviceName,
    PDEVMODEW pDevModeOutput,
    PDEVMODEW pDevModeInput,
    DWORD fMode)
{
    CStrIn strInDeviceName(pDeviceName);

    // IMPORTANT: We are not converting the DEVMODE structure back and forth
    // from ASCII to Unicode on Win95 anymore because we are not touching the
    // two strings or any other member.  Converting the DEVMODE structure can
    // be tricky because of potential and common discrepancies between the
    // value of the dmSize member and sizeof(DEVMODE).  (25155)

    // Since we are not converting the DEVMODE structure back and forth, we can
    // simply forward the call to DocumentPropertiesA.  Note that both pDevModeOutput
    // and pDevModeInput (if not NULL) have to point to DEVMODEA buffers.

    return DocumentPropertiesA(hWnd, hPrinter, strInDeviceName, (PDEVMODEA) pDevModeOutput, (PDEVMODEA) pDevModeInput, fMode);

    // We do this instead of converting as follows (left here in case we ever have to go back):
    /*
    LONG lReturn;

    // Different semantics depending on value of fMode.
    switch (fMode)
    {
    case DM_OUT_BUFFER:
    {
        CStrOut strOutDeviceName(&(pDevModeOutput->dmDeviceName[0]), CCHDEVICENAME);
        CStrOut strOutFormName(&(pDevModeOutput->dmFormName[0]), CCHFORMNAME);
        LONG lSize = DocumentPropertiesA(0, hPrinter, strInDeviceName, NULL, NULL, 0);
        HGLOBAL hDevModeAOutput = (lSize>0)?GlobalAlloc(GHND, lSize):0;
        DEVMODEA *pDevModeAOutput;

        Assert(!pDevModeInput && "pDevModeInput parameter not supported in DocumentProperties wrapper.  See src\\core\\wrappers\\winspool.cxx.");

        if (hDevModeAOutput)
        {
            pDevModeAOutput = (LPDEVMODEA) GlobalLock(hDevModeAOutput);

            if (pDevModeAOutput)
            {
                lReturn = DocumentPropertiesA(hWnd, hPrinter, strInDeviceName, pDevModeAOutput, NULL, fMode);

                // Copy portion between the two strings.
                memcpy(&(pDevModeOutput->dmSpecVersion), &(pDevModeAOutput->dmSpecVersion), ((BYTE *)&(pDevModeAOutput->dmFormName)) - ((BYTE *)&(pDevModeAOutput->dmSpecVersion)));

                // Adjust dmSize member for wide-character strings.
                pDevModeOutput->dmSize = pDevModeAOutput->dmSize + CCHDEVICENAME + CCHFORMNAME;

                // Copy portion following the second string, including the driver specific stuff following the
                // allocation of the DEVMODEA structure.
                memcpy(&(pDevModeOutput->dmLogPixels), &(pDevModeAOutput->dmLogPixels), ((BYTE *)pDevModeAOutput) + sizeof(DEVMODEA) - ((BYTE *)&(pDevModeAOutput->dmLogPixels)) + pDevModeAOutput->dmDriverExtra);

                // Copy the two strings (the cause of this headache).

                strcpy((LPSTR) strOutDeviceName, (const char *) &(pDevModeAOutput->dmDeviceName[0]));
                strOutDeviceName.ConvertIncludingNul();

                strcpy((LPSTR) strOutFormName, (const char *) &(pDevModeAOutput->dmFormName[0]));
                strOutFormName.ConvertIncludingNul();

                GlobalUnlock(hDevModeAOutput);
            }

            GlobalFree(hDevModeAOutput);
        }
    }
        break;

    case 0:
    {
        // Obtain size of DEVMODE structure.
        lReturn = DocumentPropertiesA(hWnd, hPrinter, strInDeviceName, NULL, NULL, 0);

        // Since we don't convert the DEVMODE structure back and forth (see IMPORTANT statement
        // above), the size of the structure doesn't increase, and we don't have to
        // add additional space needed by DEVMODEW:
        // lReturn += (lReturn>0)?(CCHDEVICENAME + CCHFORMNAME):0;
    }
        break;

    default:

        lReturn = -1;

        Assert(!"Given fMode not supported in DocumentProperties wrapper.  See src\\core\\wrappers\\winspool.cxx.");
    }

    return lReturn;
    */
}

WRAPIT(GetPrinterDriverA,
    (HANDLE hPrinter, LPSTR pEnvironment, DWORD Level, LPBYTE pDriverInfo, DWORD cbBuf, LPDWORD pcbNeeded),
    (hPrinter, pEnvironment, Level, pDriverInfo, cbBuf, pcbNeeded));

