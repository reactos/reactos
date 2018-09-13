//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       prsparse.cpp
//
//  Contents:   Microsoft Internet Security Internal Utility
//
//  Functions:  main
//
//              *** local functions ***
//              GetLine
//              EOLOut
//              ReformatLine
//              ParseAndReformatLine2
//              AddDefaultsForLine2
//              AddPRSNumber
//
//  History:    20-Aug-1997 pberkman   created
//
//--------------------------------------------------------------------------


#include    <stdio.h>
#include    <windows.h>
#include    <io.h> 

#define MAX_PRS_LINE        1024
#define PRS_LINE1_NUMPARAMS 4
#define PRS_FILE            "PRS.TXT"

DWORD   GetLine(HANDLE hFile, char *pszBuf, DWORD cbMaxRead);
void    EOLOut(char *psz, DWORD ccLen);
void    ReformatLine(char *pszIn, char *pszOut, DWORD cbMax);
void    ParseAndReformatLine2(char *pszIn, char *pszOut, DWORD cbMax);
void    AddDefaultsForLine2(char *pszOut, DWORD cbMax);
void    AddPRSNumber(char *pszOut, DWORD cbMax);

HANDLE  hPRSFile;

extern "C" int __cdecl main(int argc, char **argv)
{
    if (argc < 3)
    {
        printf("\nUsage: %s infile outfile\n", argv[0]);
        return(0);
    }

    HANDLE  hFileIn;
    HANDLE  hFileOut;
    char    szBufIn[MAX_PRS_LINE];
    char    szBufOut[MAX_PRS_LINE];
    char    *psz;
    DWORD   cbWritten;

    if ((hFileIn = CreateFile(argv[1], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
    {
        return(0);
    }

    if ((hFileOut = CreateFile(argv[2], GENERIC_WRITE | GENERIC_READ, 0, NULL, CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFileIn);
        return(0);
    }

    strcpy(&szBufIn[0], argv[1]);

    if (psz = strrchr(&szBufIn[0], '\\'))
    {
        psz++;
    }
    else if (psz = strrchr(&szBufIn[0], ':'))
    {
        psz++;
    }
    else
    {
        psz = &szBufIn[0];
    }

    strcpy(psz, PRS_FILE);

    hPRSFile = CreateFile(&szBufIn[0], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL, NULL);

    szBufOut[0] = NULL;

    while (GetLine(hFileIn, &szBufIn[0], MAX_PRS_LINE) > 0)
    {
        EOLOut(&szBufIn[0], strlen(&szBufIn[0]) + 4);

        if ((szBufOut[0]) && (szBufIn[0] == '-'))
        {
            //
            //  line continues...  the second line needs to be parsed...
            //
            ParseAndReformatLine2(&szBufIn[0], &szBufOut[0], MAX_PRS_LINE);
        }
        else if (szBufOut[0])
        {
            AddDefaultsForLine2(&szBufOut[0], MAX_PRS_LINE);
        }

        if (szBufOut[0])
        {
            cbWritten = 0;
            WriteFile(hFileOut, &szBufOut[0], strlen(&szBufOut[0]), &cbWritten, NULL);

            szBufOut[0] = NULL;

            continue;
        }

        if ((szBufIn[0] == ';') || (szBufIn[0] == '#') || !(szBufIn[0]) || (szBufIn[0] == ' '))
        {
            continue;
        }

        ReformatLine(&szBufIn[0], &szBufOut[0], MAX_PRS_LINE);
    }

    CloseHandle(hFileIn);
    CloseHandle(hFileOut);
    
    if (hPRSFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hPRSFile);
    }
    
    return(1);
}

void ReformatLine(char *pszIn, char *pszOut, DWORD cbMax)
{
    int idxIn;
    int idxOut;
    int len;
    int params;

    params      = 1;
    idxOut      = 0;
    pszOut[0]   = NULL;

    len = strlen(pszIn);

    if (len > 0)
    {
        pszOut[idxOut++] = '\"';
        pszOut[idxOut]   = NULL;
    }

    for (idxIn = 0; idxIn < len; idxIn++)
    {
        if (pszIn[idxIn] == ',') 
        {
            if (pszIn[idxIn - 1] != '\"')
            {
                pszOut[idxOut++] = '\"';
                pszOut[idxOut]   = NULL;
            }
         
            pszOut[idxOut++] = ',';
            pszOut[idxOut]   = NULL;

            if (pszIn[idxIn + 1] != '\"')
            {
               pszOut[idxOut++] = '\"';
                pszOut[idxOut]   = NULL;
            }

            params++;
        }
        else
        {
            pszOut[idxOut++] = pszIn[idxIn];
        }
    }

    if (params < PRS_LINE1_NUMPARAMS)
    {
        for (idxIn = params; idxIn < PRS_LINE1_NUMPARAMS; idxIn++)
        {
            pszOut[idxOut++] = ',';
            pszOut[idxOut++] = '\"';
            pszOut[idxOut++] = '\"';
        }
    }

    if (len > 0)
    {
        if (pszOut[idxOut - 1] != '\"')
        {
            pszOut[idxOut++] = '\"';
        }
        pszOut[idxOut]   = NULL;
    }
}

void ParseAndReformatLine2(char *pszIn, char *pszOut, DWORD cbMax)
{
    int idxIn;
    int idxOut;
    int len;
    int params;

    params          = PRS_LINE1_NUMPARAMS;
    idxOut          = strlen(pszOut);
    pszOut[idxOut]  = NULL;

    len = strlen(pszIn);

    if (len > 0)
    {
        pszOut[idxOut++] = ',';

        if (pszIn[1] != '\"')
        {
            pszOut[idxOut++] = '\"';
        }

        pszOut[idxOut]   = NULL;
    }

    for (idxIn = 1; idxIn < len; idxIn++)   // idxIn = 1: pass over '-'
    {
        if (pszIn[idxIn] == ',') 
        {
            if (pszIn[idxIn - 1] != '\"')
            {
                pszOut[idxOut++] = '\"';
                pszOut[idxOut]   = NULL;
            }
         
            pszOut[idxOut++] = ',';
            pszOut[idxOut]   = NULL;

            if (pszIn[idxIn + 1] != '\"')
            {
               pszOut[idxOut++] = '\"';
                pszOut[idxOut]   = NULL;
            }

            params++;
        }
        else
        {
            pszOut[idxOut++] = pszIn[idxIn];
        }
    }

    if (len > 0)
    {
        if (pszIn[idxIn - 1] != '\"')
        {
            pszOut[idxOut++] = '\"';
        }
        pszOut[idxOut]   = NULL;
    }
    
    AddPRSNumber(pszOut, cbMax);

    strcat(pszOut, "\r\n");
}

void AddDefaultsForLine2(char *pszOut, DWORD cbMax)
{
    strcat(&pszOut[0], ",\"PN:\",\"SET:\",\"VN:\",\"MV:\"");

    AddPRSNumber(pszOut, cbMax);

    strcat(pszOut, "\r\n");
}

void AddPRSNumber(char *pszOut, DWORD cbMax)
{
    if (hPRSFile != INVALID_HANDLE_VALUE)
    {
        char    szRead[MAX_PATH];

        while (GetLine(hPRSFile, &szRead[0], MAX_PATH) > 0)
        {
            EOLOut(&szRead[0], strlen(&szRead[0]) + 4);

            if ((szRead[0] == ';') || (szRead[0] == '#') || !(szRead[0]) || (szRead[0] == ' '))
            {
                continue;
            }

            strcat(pszOut, ",\"JOBNO:");
            strcat(pszOut, &szRead[0]);
            strcat(pszOut, "\"");

            break;
        }
    }
}

DWORD GetLine(HANDLE hFile, char *pszBuf, DWORD cbMaxRead)
{
	DWORD   dwHold;
	DWORD   cbRead;
	DWORD   dw;
    int     iAmt;

    pszBuf[0] = NULL;

    if ((dwHold = SetFilePointer(hFile, 0, NULL, FILE_CURRENT)) == 0xFFFFFFFF)
    {
        return(0);
    }

    cbRead = 0;

    if (ReadFile(hFile, pszBuf, cbMaxRead, &cbRead, NULL))
    {
        if (cbRead == 0)
        {
            return(0);
        }

        pszBuf[cbRead] = 0x00;

        if (cbRead > 0)
        {
            iAmt = 0;
		    for (dw = 0; dw < (cbRead - 1); dw++)
		    {
		    	if ((pszBuf[dw] == 0x0d) || 
                    (pszBuf[dw] == 0x0a))
		    	{
                    iAmt++;
		    		if (pszBuf[dw + 1] == 0x0a)
		    		{
                        dw++;
                        iAmt++;
		    		}
            
                    SetFilePointer(hFile, dwHold + (dw + 1), NULL, FILE_BEGIN);
            
		    		pszBuf[dw + 1] = 0x00;
            
		    		return(cbRead + 1);
		    	}
		    }
        }
	}
	else
	{
		return(0);
	}

	if (pszBuf[cbRead - 1] == 0x1a)  /* EOF */
	{
		cbRead--;
	}

	return(cbRead);
}

void EOLOut(char *psz, DWORD ccLen)
{
	DWORD   i;

	for (i = 0; i < ccLen; i++)
	{
		if ((psz[i] == 0x0a) || (psz[i] == 0x0d))
		{
			psz[i] = NULL;
			return;
		}
	}
	psz[ccLen] = NULL;
}

