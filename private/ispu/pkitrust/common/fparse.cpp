//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       fparse.cpp
//
//  Contents:   File parsing api -- INI file types
//
//  History:    01-Oct-1997 pberkman    create
//
//--------------------------------------------------------------------------

#include    "global.hxx"

#include    "fparse.hxx"


fParse_::fParse_(WCHAR *pwszFilename, DWORD dwMaxLine0, DWORD dwFileAccess, DWORD dwFileSharing)
{
    hFile               = CreateFileU(pwszFilename, dwFileAccess, dwFileSharing, NULL, OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL, NULL);
    dwMaxLine           = dwMaxLine0;
    pwszCurrentLine     = new WCHAR[dwMaxLine];
    dwCurLineFilePos    = 0;
    dwLastGroupFilePos  = 0;
    dwLastTagFilePos    = 0;
    fEOF                = FALSE;
    pwszTempFName       = NULL;
    pwszLastGroupTag    = NULL;

    if (pwszFName = new WCHAR[wcslen(pwszFilename) + 1])
    {
        wcscpy(pwszFName, pwszFilename);
    }
}

fParse_::~fParse_(void)
{
    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
    }

    DELETE_OBJECT(pwszCurrentLine);

    DELETE_OBJECT(pwszLastGroupTag);

    if (pwszTempFName)
    {
        CopyFileU(pwszTempFName, pwszFName, FALSE);

        DeleteFileU(pwszTempFName);

        delete pwszTempFName;
    }

    DELETE_OBJECT(pwszFName);
}

BOOL fParse_::AddTagToFile(WCHAR *pwszGroup, WCHAR *pwszTag, WCHAR *pwszValue)
{
    if (!(this->pwszCurrentLine) || (this->hFile == INVALID_HANDLE_VALUE))
    {
        return(FALSE);
    }

    char    szTFile[MAX_PATH * 2];
    WCHAR   wszGroup[MAX_PATH];
    HANDLE  hTFile;
    DWORD   ccTFile;
    DWORD   cbWrite;
    BOOL    fWritten;
    
    if (!(pwszLastGroupTag))
    {
        return(FALSE);
    }

    if (pwszTag[0] != L'[')
    {
        wcscpy(&wszGroup[0], L"[");
        wcscat(&wszGroup[0], pwszGroup);
        wcscat(&wszGroup[0], L"]");
    }
    else
    {
        wcscpy(&wszGroup[0], pwszTag);
    }

    szTFile[0] = NULL;
    GetTempFileName(".", "FPS", 0, &szTFile[0]);

    if (!(szTFile[0]))
    {
        return(FALSE);
    }

    ccTFile = MultiByteToWideChar(0, 0, &szTFile[0], -1, NULL, 0);

    if (ccTFile < 1)
    {
        return(FALSE);
    }

    if (!(pwszTempFName = new WCHAR[ccTFile + 1]))
    {
        return(FALSE);
    }

    MultiByteToWideChar(0, 0, &szTFile[0], -1, pwszTempFName, ccTFile + 1);

    hTFile = CreateFileU(pwszTempFName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL, NULL);

    if (hTFile == INVALID_HANDLE_VALUE)
    {
        return(FALSE);
    }

    SetFilePointer(this->hFile, 0, NULL, FILE_BEGIN);

    fWritten = FALSE;

    while (this->GetNextLine())
    {
        szTFile[0] = NULL;
        WideCharToMultiByte(0, 0, this->pwszCurrentLine, wcslen(this->pwszCurrentLine) + 1,
                            &szTFile[0], MAX_PATH * 2, NULL, NULL);
        if (szTFile[0])
        {
            WriteFile(hTFile, &szTFile[0], strlen(&szTFile[0]), &cbWrite, NULL);

            if (!(fWritten))
            {
                this->EOLRemove();
    
                if (_memicmp(this->pwszCurrentLine, &wszGroup[0], 
                           wcslen(&wszGroup[0]) * sizeof(WCHAR)) == 0)
                {
                    //
                    //  add our line
                    //
                    szTFile[0] = NULL;
                    WideCharToMultiByte(0, 0, pwszTag, wcslen(pwszTag) + 1, &szTFile[0], MAX_PATH, NULL, NULL);
                    WriteFile(hTFile, &szTFile[0], strlen(&szTFile[0]), &cbWrite, NULL);
                    WriteFile(hTFile, "=", 1, &cbWrite, NULL);
                    szTFile[0] = NULL;
                    WideCharToMultiByte(0, 0, pwszValue, wcslen(pwszValue) + 1, &szTFile[0], MAX_PATH * 2, NULL, NULL);
                    WriteFile(hTFile, &szTFile[0], strlen(&szTFile[0]), &cbWrite, NULL);
                    WriteFile(hTFile, "\r\n", 2, &cbWrite, NULL);
                    fWritten = TRUE;
                }
            }
        }
    }

    CloseHandle(hTFile);

    this->Reset();

    return(TRUE);
}

void fParse_::Reset(void)
{
    this->dwCurLineFilePos      = 0;
    this->dwLastGroupFilePos    = 0;
    this->dwLastTagFilePos      = 0;

    SetFilePointer(this->hFile, 0, NULL, FILE_BEGIN);
}

BOOL fParse_::FindGroup(WCHAR *pwszGroup)
{
    if (!(this->pwszCurrentLine) || (this->hFile == INVALID_HANDLE_VALUE))
    {
        return(FALSE);
    }

    this->pwszCurrentLine[0] = NULL;

    if (SetFilePointer(this->hFile, 0, NULL, FILE_BEGIN) == 0xFFFFFFFF)
    {
        return(FALSE);
    }

    WCHAR       wszGroup[MAX_PATH];

    if (pwszGroup[0] != L'[')
    {
        wcscpy(&wszGroup[0], L"[");
        wcscat(&wszGroup[0], pwszGroup);
        wcscat(&wszGroup[0], L"]");
    }
    else
    {
        wcscpy(&wszGroup[0], pwszGroup);
    }

    while (this->GetNextLine() > 0)
    {
        if ((this->pwszCurrentLine[0] == L'#') ||
            (this->pwszCurrentLine[0] == L';') ||
            (this->pwszCurrentLine[0] == 0x000d))
        {
            continue;
        }

        if (this->pwszCurrentLine[0] == L'[')
        {
            if (_memicmp(this->pwszCurrentLine, &wszGroup[0], 
                       wcslen(&wszGroup[0]) * sizeof(WCHAR)) == 0)
            {
                this->dwLastGroupFilePos = this->dwCurLineFilePos;

                this->EOLRemove();

                DELETE_OBJECT(this->pwszLastGroupTag);
                this->pwszLastGroupTag = new WCHAR[wcslen(this->pwszCurrentLine) + 1];
                wcscpy(this->pwszLastGroupTag, this->pwszCurrentLine);

                return(TRUE);
            }
        }
    }

    this->pwszCurrentLine[0] = NULL;

    return(FALSE);
}

BOOL fParse_::PositionAtLastGroup(void)
{
    if (SetFilePointer(this->hFile, this->dwLastGroupFilePos, NULL,
                       FILE_BEGIN) == 0xFFFFFFFF)
    {
        return(FALSE);
    }

    return(TRUE);
}

BOOL fParse_::PositionAtLastTag(void)
{
    if (this->dwLastTagFilePos == 0)
    {
        return(FALSE);
    }

    if (SetFilePointer(this->hFile, this->dwLastTagFilePos, NULL,
                       FILE_BEGIN) == 0xFFFFFFFF)
    {
        return(FALSE);
    }

    return(TRUE);
}

BOOL fParse_::GetLineInCurrentGroup(void)
{
    if (this->dwLastGroupFilePos == 0)
    {
        return(FALSE);
    }

    if (this->dwLastTagFilePos == 0)
    {
        this->PositionAtLastGroup();
    }

    while (this->GetNextLine() > 0)
    {
        if ((this->pwszCurrentLine[0] == L'#') ||
            (this->pwszCurrentLine[0] == L';') ||
            (this->pwszCurrentLine[0] == 0x000d))
        {
            continue;
        }

        if (this->pwszCurrentLine[0] == L'[')
        {
            this->pwszCurrentLine[0] = NULL;

            return(FALSE);
        }

        this->EOLRemove();

        if (wcslen(this->pwszCurrentLine) > 0)
        {
            this->dwLastTagFilePos = this->dwCurLineFilePos;
            
            return(TRUE);
        }
    }

    this->pwszCurrentLine[0] = NULL;
    
    return(FALSE);
}

BOOL fParse_::FindTagInCurrentGroup(WCHAR *pwszTag)
{
    if (this->dwLastGroupFilePos == 0)
    {
        return(FALSE);
    }

    WCHAR   wszCheck[MAX_PATH];
    WCHAR   wszCheck2[MAX_PATH];
    LPWSTR  pwszEqual;
    DWORD   ccRet;
    DWORD   ccLastMember;
    BOOL    fFoundLast;

    wcscpy(&wszCheck[0], pwszTag);
    wcscat(&wszCheck[0], L"=");

    wcscpy(&wszCheck2[0], pwszTag);
    wcscpy(&wszCheck2[0], L" =");

    this->PositionAtLastGroup();

    while (this->GetNextLine() > 0)
    {
        if ((this->pwszCurrentLine[0] == L'#') ||
            (this->pwszCurrentLine[0] == L';') ||
            (this->pwszCurrentLine[0] == 0x000d))
        {
            continue;
        }

        if (this->pwszCurrentLine[0] == L'[')
        {
            this->pwszCurrentLine[0] = NULL;

            return(FALSE);
        }

        if ((_memicmp(this->pwszCurrentLine, &wszCheck[0], wcslen(&wszCheck[0]) * sizeof(WCHAR)) == 0) ||
            (_memicmp(this->pwszCurrentLine, &wszCheck2[0], wcslen(&wszCheck2[0]) * sizeof(WCHAR)) == 0))
        {
            this->dwLastTagFilePos = this->dwCurLineFilePos;

            this->EOLRemove();

            return(TRUE);
        }
    }

    this->pwszCurrentLine[0] = NULL;
    
    return(FALSE);
}

BOOL fParse_::FindTagFromCurrentPos(WCHAR *pwszTag)
{
    WCHAR   wszCheck[MAX_PATH];
    WCHAR   wszCheck2[MAX_PATH];
    LPWSTR  pwszEqual;
    DWORD   ccRet;
    DWORD   ccLastMember;
    BOOL    fFoundLast;

    wcscpy(&wszCheck[0], pwszTag);
    wcscat(&wszCheck[0], L"=");

    wcscpy(&wszCheck2[0], pwszTag);
    wcscat(&wszCheck2[0], L" =");

    this->dwLastTagFilePos++;

    while (this->GetNextLine() > 0)
    {
        if ((this->pwszCurrentLine[0] == L'#') ||
            (this->pwszCurrentLine[0] == L';') ||
            (this->pwszCurrentLine[0] == 0x000d))
        {
            continue;
        }

        if ((_memicmp(this->pwszCurrentLine, &wszCheck[0], wcslen(&wszCheck[0]) * sizeof(WCHAR)) == 0) ||
            (_memicmp(this->pwszCurrentLine, &wszCheck2[0], wcslen(&wszCheck2[0]) * sizeof(WCHAR)) == 0))
        {
            this->dwLastTagFilePos = this->dwCurLineFilePos;

            this->EOLRemove();

            return(TRUE);
        }
    }

    this->pwszCurrentLine[0] = NULL;
    
    return(FALSE);
}

DWORD fParse_::GetNextLine(void)
{
    if (!(this->pwszCurrentLine) ||
        (this->hFile == INVALID_HANDLE_VALUE))
    {
        return(0);
    }

	DWORD   dwHold;
	DWORD   cbRead;
    DWORD   cwbRead;
	DWORD   dw;
    int     iAmt;
    BYTE    *pb;

    if ((dwHold = SetFilePointer(this->hFile, 0, NULL, FILE_CURRENT)) == 0xFFFFFFFF)
    {
        return(0);
    }

    if (!(pb = new BYTE[dwMaxLine + 2]))
    {
        return(0);
    }

    cbRead = 0;

    if (ReadFile(this->hFile, pb, dwMaxLine, &cbRead, NULL))
    {
        if (cbRead == 0)
        {
            this->fEOF = TRUE;

            delete pb;

            return(0);
        }

        pb[cbRead] = 0x00;

        this->fEOF = FALSE;

        if (cbRead > 0)
        {
            iAmt = 0;
		    for (dw = 0; dw < (cbRead - 1); dw++)
		    {
		    	if ((pb[dw] == 0x0d) || 
                    (pb[dw] == 0x0a))
		    	{
                    iAmt++;
		    		if (pb[dw + 1] == 0x0a)
		    		{
                        dw++;
                        iAmt++;
		    		}
            
                    if (SetFilePointer(this->hFile, dwHold + (dw + 1),
                                        NULL, FILE_BEGIN) == 0xFFFFFFFF)
                    {
                        this->dwCurLineFilePos = 0;
                    }
                    else
                    {
                        this->dwCurLineFilePos = SetFilePointer(this->hFile, 0, NULL, FILE_CURRENT) - iAmt;
                    }
            
		    		pb[dw + 1] = 0x00;
            
                    cwbRead = MultiByteToWideChar(0, 0, (const char *)pb, -1, 
                                                    pwszCurrentLine, dwMaxLine);

                    delete pb;

		    		return(cwbRead + 1);
		    	}
		    }
        }
	}
	else
	{
        delete pb;

		return(0);
	}

	if (pb[cbRead - 1] == 0x1a)  /* EOF */
	{
		cbRead--;
        this->dwCurLineFilePos  = 0;
        this->fEOF              = TRUE;
	}
    else
    {
        this->dwCurLineFilePos  = dwHold;
    }

	pb[cbRead] = 0x00;

    cwbRead = MultiByteToWideChar(0, 0, (const char *)pb, -1, 
                                  pwszCurrentLine, dwMaxLine);


    delete pb;

	return(cwbRead);
}

void fParse_::EOLRemove(void)
{
	DWORD   i;
    DWORD   ccLen;

    ccLen = wcslen(this->pwszCurrentLine);

	for (i = 0; i < ccLen; i++)
	{
		if ((this->pwszCurrentLine[i] == (WCHAR)0x0a) || 
            (this->pwszCurrentLine[i] == (WCHAR)0x0d))
		{
			this->pwszCurrentLine[i] = NULL;
			return;
		}
	}
	this->pwszCurrentLine[ccLen] = NULL;
}

