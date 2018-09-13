//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       cwargv.cpp
//
//  Contents:   argv parsing api
//
//  History:    02-Oct-1997 pberkman    create
//
//--------------------------------------------------------------------------

#include    "global.hxx"

#include    "cwargv.hxx"
#include    "fparse.hxx"

#define SID_FILES               0

typedef struct ARGVSTORAGE_
{
    DWORD       dwsidOption;
    WCHAR       *pwszOption;

    DWORD       dwsidOptionHelp;
    WCHAR       *pwszOptionHelp;

    BOOL        fHiddenCmd;

    BOOL        fSet;

    DWORD       dwValueType;
    union
    {
        WCHAR   *pwszValue;
        WCHAR   *pwszCmdFile;
        DWORD   dwValue;
        BOOL    fValue;
    };

} ARGVSTORAGE;

cWArgv_::cWArgv_(HINSTANCE hInst0, BOOL fChkCmdFile)
{
    pwszThisFilename        = NULL; // don't delete!
    pwszUsageWord           = NULL;
    pwszUsageOptionsText    = NULL;
    pwszUsageCmdFileText    = NULL;
    pwszUsageAddText        = NULL;
    pwszOptionParamText     = NULL;
    pwszUsageString         = NULL;
    pwszNonParamArgBlanks   = NULL;
    fShowHiddenArgs         = FALSE;
    fNonHiddenParamArgs     = FALSE;
    dwLongestArg            = 0;
    this->hInst             = hInst0;
    this->fChkCmdF          = fChkCmdFile;
    pArgs                   = new Stack_(NULL); // no sorting!
}

cWArgv_::~cWArgv_(void)
{
    ARGVSTORAGE *pArg;
    DWORD       dwIdx;

    DELETE_OBJECT(pwszUsageWord);
    DELETE_OBJECT(pwszUsageOptionsText);
    DELETE_OBJECT(pwszUsageCmdFileText);
    DELETE_OBJECT(pwszUsageAddText);
    DELETE_OBJECT(pwszOptionParamText);
    DELETE_OBJECT(pwszUsageString);
    DELETE_OBJECT(pwszNonParamArgBlanks);

    dwIdx = 0;

    while (pArg = (ARGVSTORAGE *)pArgs->Get(dwIdx))
    {
        DELETE_OBJECT(pArg->pwszOption);
        DELETE_OBJECT(pArg->pwszOptionHelp);

        dwIdx++;
    }

    DELETE_OBJECT(pArgs);
}

void cWArgv_::AddUsageText(DWORD dwsidUsageWord, DWORD dwsidUsageOptions, DWORD dwsidUsageCmdFileText,
                           DWORD dwsidUsageAddText, DWORD dwsidOptionParamText)
{
    WCHAR       wszString[MAX_PATH];

    wszString[0] = NULL;
    LoadStringU(this->hInst, dwsidUsageWord, &wszString[0], MAX_PATH);

    if (wszString[0])
    {
        if (!(pwszUsageWord = new WCHAR[wcslen(&wszString[0]) + 1]))
        {
            return;
        }

        wcscpy(pwszUsageWord, &wszString[0]);
    }

    wszString[0] = NULL;
    LoadStringU(this->hInst, dwsidUsageOptions, &wszString[0], MAX_PATH);

    if (wszString[0])
    {
        if (!(pwszUsageOptionsText = new WCHAR[wcslen(&wszString[0]) + 1]))
        {
            return;
        }

        wcscpy(pwszUsageOptionsText, &wszString[0]);
    }

    wszString[0] = NULL;
    LoadStringU(this->hInst, dwsidUsageCmdFileText, &wszString[0], MAX_PATH);

    if (wszString[0])
    {
        if (!(pwszUsageCmdFileText = new WCHAR[wcslen(&wszString[0]) + 1]))
        {
            return;
        }

        wcscpy(pwszUsageCmdFileText, &wszString[0]);
    }

    wszString[0] = NULL;
    LoadStringU(this->hInst, dwsidOptionParamText, &wszString[0], MAX_PATH);

    if (wszString[0])
    {
        if (!(pwszOptionParamText = new WCHAR[wcslen(&wszString[0]) + 1]))
        {
            return;
        }

        wcscpy(pwszOptionParamText, &wszString[0]);
    }

    wszString[0] = NULL;
    LoadStringU(this->hInst, dwsidUsageAddText, &wszString[0], MAX_PATH);

    if (wszString[0])
    {
        if (!(pwszUsageAddText = new WCHAR[wcslen(&wszString[0]) + 1]))
        {
            return;
        }

        wcscpy(pwszUsageAddText, &wszString[0]);
    }
}

WCHAR *cWArgv_::GetUsageString(void)
{
    int i;

    if (pwszUsageString)
    {
        return(pwszUsageString);
    }

    if (!(pwszThisFilename))
    {
        return(NULL);
    }

    DWORD   ccLen;

    ccLen = wcslen(pwszThisFilename);

    if (pwszUsageWord)
    {
        ccLen += wcslen(pwszUsageWord);
    }

    if (pwszUsageOptionsText)
    {
        ccLen += wcslen(pwszUsageOptionsText);
    }

    if (fChkCmdF)
    {
        if (pwszUsageCmdFileText)
        {
            ccLen += wcslen(pwszUsageCmdFileText);
        }
    }

    if (pwszUsageAddText)
    {
        ccLen += wcslen(pwszUsageAddText);
    }

    ARGVSTORAGE *pArg;
    DWORD       dwIdx;

    dwIdx = 0;

    while (pArg = (ARGVSTORAGE *)pArgs->Get(dwIdx))
    {
        if (pArg->dwsidOption != SID_FILES)
        {
            if (pArg->pwszOption)
            {
                if ((pArg->fHiddenCmd) && (!(fShowHiddenArgs)))
                {
                    dwIdx++;
                    continue;
                }

                ccLen += 6;             // 5 spaces + 1 for '-'
                ccLen += dwLongestArg;  // wcslen(pArg->pwszOption);

                if (((fNonHiddenParamArgs) && !(fShowHiddenArgs)) ||
                    (fShowHiddenArgs))
                {
                    if ((pwszNonParamArgBlanks) && (pwszOptionParamText))
                    {
                        ccLen++;    // space
                        ccLen += wcslen(pwszOptionParamText);
                    }
                }

                if (pArg->pwszOptionHelp)
                {
                    ccLen += 2; // : + space
                    ccLen += wcslen(pArg->pwszOptionHelp);
                }

                ccLen += 2; // cr/lf
            }
        }

        dwIdx++;
    }


    ccLen += 10;

    if (!(pwszUsageString = new WCHAR[ccLen + 1]))
    {
        return(NULL);
    }

    swprintf(pwszUsageString, L"%s: %s %s %s %s\r\n",
                            pwszUsageWord,
                            pwszThisFilename,
                            (pwszUsageOptionsText) ? pwszUsageOptionsText : L"",
                            (pwszUsageCmdFileText && fChkCmdF) ? pwszUsageCmdFileText : L"",
                            (pwszUsageAddText) ? pwszUsageAddText : L"");

    dwIdx = pArgs->Count() - 1;

    while (pArg = (ARGVSTORAGE *)pArgs->Get(dwIdx))
    {
        if (pArg->dwsidOption != SID_FILES)
        {
            if (pArg->pwszOption)
            {
                if ((pArg->fHiddenCmd) && (!(fShowHiddenArgs)))
                {
                    if (dwIdx == 0)
                    {
                        break;
                    }

                    dwIdx--;
                    continue;
                }

                wcscat(pwszUsageString, L"     -");
                wcscat(pwszUsageString, pArg->pwszOption);

                if ((pArg->dwValueType != WARGV_VALUETYPE_BOOL) &&
                    (pwszOptionParamText))
                {
                    wcscat(pwszUsageString, L" ");
                    wcscat(pwszUsageString, pwszOptionParamText);
                }

                if (pArg->pwszOptionHelp)
                {
                    wcscat(pwszUsageString, L": ");

                    for (i = 0; i < (int)(dwLongestArg - wcslen(pArg->pwszOption)); i++)
                    {
                        wcscat(pwszUsageString, L" ");
                    }

                    if ((pArg->dwValueType == WARGV_VALUETYPE_BOOL) &&
                        (((fNonHiddenParamArgs) && !(fShowHiddenArgs)) || (fShowHiddenArgs)) &&
                        (pwszNonParamArgBlanks))
                    {
                        wcscat(pwszUsageString, pwszNonParamArgBlanks);
                    }

                    wcscat(pwszUsageString, pArg->pwszOptionHelp);
                }

                wcscat(pwszUsageString, L"\r\n");
            }
        }

        if (dwIdx == 0)
        {
            break;
        }

        dwIdx--;
    }

    return(pwszUsageString);
}

BOOL cWArgv_::Add2List(DWORD dwsidOption, DWORD dwsidOptionHelp, DWORD dwValueType, void *pvDefaultValue,
                       BOOL fInternalCmd)
{
    if (!(pArgs))
    {
        return(FALSE);
    }

    ARGVSTORAGE *pArg;
    WCHAR       wszString[MAX_PATH];
    DWORD       i;

    if (!(pArg = (ARGVSTORAGE *)pArgs->Add(sizeof(ARGVSTORAGE))))
    {
        return(FALSE);
    }

    memset(pArg, 0x00, sizeof(ARGVSTORAGE));

    pArg->dwValueType   = dwValueType;

    if (pArg->dwValueType != WARGV_VALUETYPE_BOOL)
    {
        if (!(pwszNonParamArgBlanks))
        {
            if (pwszOptionParamText)
            {
                if (pwszNonParamArgBlanks = new WCHAR[wcslen(pwszOptionParamText) + 2])
                {
                    for (i = 0; i <= (DWORD)wcslen(pwszOptionParamText); i++)
                    {
                        pwszNonParamArgBlanks[i] = L' ';
                    }
                    pwszNonParamArgBlanks[i] = NULL;
                }
            }
        }

        fNonHiddenParamArgs = TRUE;
    }

    pArg->fHiddenCmd    = fInternalCmd;

    pArg->dwsidOption   = dwsidOption;
    wszString[0] = NULL;
    LoadStringU(this->hInst, dwsidOption, &wszString[0], MAX_PATH);

    if (wszString[0])
    {
        if (!(pArg->pwszOption = new WCHAR[wcslen(&wszString[0]) + 1]))
        {
            return(FALSE);
        }

        wcscpy(pArg->pwszOption, &wszString[0]);

        if ((DWORD)wcslen(&wszString[0]) > dwLongestArg)
        {
            dwLongestArg = wcslen(&wszString[0]);
        }
    }


    pArg->dwsidOptionHelp   = dwsidOptionHelp;
    wszString[0]            = NULL;
    LoadStringU(this->hInst, dwsidOptionHelp, &wszString[0], MAX_PATH);

    if (wszString[0])
    {
        if (!(pArg->pwszOptionHelp = new WCHAR[wcslen(&wszString[0]) + 1]))
        {
            return(FALSE);
        }

        wcscpy(pArg->pwszOptionHelp, &wszString[0]);
    }

    if (pvDefaultValue)
    {
        switch (dwValueType)
        {
            case WARGV_VALUETYPE_BOOL:      pArg->fValue    = (BOOL)((DWORD_PTR)pvDefaultValue);     break;
            case WARGV_VALUETYPE_DWORDD:
            case WARGV_VALUETYPE_DWORDH:    pArg->dwValue   = (DWORD)((DWORD_PTR)pvDefaultValue);    break;
            case WARGV_VALUETYPE_WCHAR:     pArg->pwszValue = (WCHAR *)pvDefaultValue;  break;
            default:
                return(FALSE);
        }
    }

    return(TRUE);
}

BOOL cWArgv_::Fill(int argc, WCHAR **wargv)
{
    if (!(pArgs))
    {
        return(FALSE);
    }

    if (!(pwszThisFilename))
    {
        if (pwszThisFilename = wcsrchr(&wargv[0][0], L'\\'))
        {
            pwszThisFilename++;
        }
        else
        {
            pwszThisFilename    = &wargv[0][0];
        }
    }

    int     i;

    for (i = 1; i < argc; ++i)
    {
        switch (wargv[i][0])
        {
            case L'-':
            case L'/':
                if (wargv[i][1] == L'~')
                {
                    fShowHiddenArgs     = TRUE;
                    return(FALSE);
                }

                i += this->ProcessArg(argc - i, &wargv[i]);
                break;

            case L'@':
                this->ProcessCommandFile(&wargv[i][1]);
                break;

            default:
                this->AddFile(&wargv[i][0]);
                break;
        }
    }

    return(TRUE);
}

int cWArgv_::ProcessArg(int argc, WCHAR **wargv)
{
    ARGVSTORAGE *pArg;
    DWORD       dwIdx;
    DWORD       ccOption;
    WCHAR       *pwszArg;
    int         iRet;

    iRet    = 0;

    pwszArg             = &wargv[0][1]; // skip over - or /

    dwIdx = 0;

    while (pArg = (ARGVSTORAGE *)pArgs->Get(dwIdx))
    {
        if (pArg->pwszOption)
        {
            ccOption = (DWORD)wcslen(pArg->pwszOption);

            if (_memicmp(pArg->pwszOption, pwszArg, ccOption * sizeof(WCHAR)) == 0)
            {
                pArg->fSet = TRUE;

                switch (pArg->dwValueType)
                {
                    case WARGV_VALUETYPE_BOOL:
                            pArg->fValue = TRUE;

                            return(iRet);

                    case WARGV_VALUETYPE_DWORDH:
                    case WARGV_VALUETYPE_DWORDD:
                            if (!(pwszArg[ccOption]))
                            {
                                pwszArg = &wargv[1][0];
                                iRet++;
                            }
                            else
                            {
                                pwszArg = &wargv[0][ccOption + 1];
                            }

                            if (pArg->dwValueType == WARGV_VALUETYPE_DWORDH)
                            {
                                pArg->dwValue = (DWORD)wcstoul(pwszArg, NULL, 16);
                            }
                            else
                            {
                                pArg->dwValue = (DWORD)wcstoul(pwszArg, NULL, 10);
                            }

                            return(iRet);

                    case WARGV_VALUETYPE_WCHAR:
                            if (!(pwszArg[ccOption]))
                            {
                                pArg->pwszValue = &wargv[1][0];
                                iRet++;
                            }
                            else
                            {
                                pArg->pwszValue = &wargv[0][ccOption];
                            }

                            return(iRet);

                    default:
                            return(iRet);
                }
            }
        }

        dwIdx++;
    }

    return(iRet);
}

BOOL cWArgv_::IsSet(DWORD dwsidOption)
{
    ARGVSTORAGE *pArg;
    DWORD   dwIdx;

    dwIdx = 0;

    while (pArg = (ARGVSTORAGE *)pArgs->Get(dwIdx))
    {
        if (pArg->dwsidOption == dwsidOption)
        {
            return((pArg->fSet) ? TRUE : FALSE);
        }

        dwIdx++;
    }

    return(FALSE);
}

void *cWArgv_::GetValue(DWORD dwsidOption)
{
    ARGVSTORAGE *pArg;
    DWORD   dwIdx;

    dwIdx = 0;

    while (pArg = (ARGVSTORAGE *)pArgs->Get(dwIdx))
    {
        if (pArg->dwsidOption == dwsidOption)
        {
            switch (pArg->dwValueType)
            {
                case WARGV_VALUETYPE_BOOL:      return((void *)(UINT_PTR)pArg->fValue);
                case WARGV_VALUETYPE_DWORDD:
                case WARGV_VALUETYPE_DWORDH:    return((void *)(UINT_PTR)pArg->dwValue);
                case WARGV_VALUETYPE_WCHAR:     return((void *)pArg->pwszValue);
                default:
                    return(NULL);
            }
        }

        dwIdx++;
    }

    return(NULL);
}

WCHAR *cWArgv_::GetOptionHelp(DWORD dwsidOption)
{
    if (!(pArgs))
    {
        return(NULL);
    }

    ARGVSTORAGE *pArg;
    DWORD       dwIdx;

    dwIdx   = 0;

    while (pArg = (ARGVSTORAGE *)pArgs->Get(dwIdx))
    {
        if (pArg->dwsidOption == dwsidOption)
        {
            return(pArg->pwszOptionHelp);
        }

        dwIdx++;
    }

    return(NULL);
}

WCHAR *cWArgv_::GetOption(DWORD dwsidOption)
{
    if (!(pArgs))
    {
        return(NULL);
    }

    ARGVSTORAGE *pArg;
    DWORD       dwIdx;

    dwIdx   = 0;

    while (pArg = (ARGVSTORAGE *)pArgs->Get(dwIdx))
    {
        if (pArg->dwsidOption == dwsidOption)
        {
            return(pArg->pwszOption);
        }

        dwIdx++;
    }

    return(NULL);
}

WCHAR *cWArgv_::GetFileName(DWORD *pdwidxLast)
{
    if (!(pArgs))
    {
        return(NULL);
    }

    ARGVSTORAGE *pArg;
    DWORD       dwIdx;
    DWORD       dwFIdx;

    dwIdx   = 0;
    dwFIdx  = 1;

    while (pArg = (ARGVSTORAGE *)pArgs->Get(dwIdx))
    {
        if (pArg->dwsidOption == SID_FILES)
        {
            if (!(pdwidxLast) || (dwFIdx > *pdwidxLast))
            {
                return(pArg->pwszValue);
            }

            dwFIdx++;
        }

        dwIdx++;
    }

    return(NULL);
}

BOOL cWArgv_::AddFile(WCHAR *pwszFile)
{
    ARGVSTORAGE *pArg;

    if (!(pArg = (ARGVSTORAGE *)pArgs->Add(sizeof(ARGVSTORAGE))))
    {
        return(FALSE);
    }

    memset(pArg, 0x00, sizeof(ARGVSTORAGE));

    pArg->dwsidOption   = SID_FILES;
    pArg->dwValueType   = WARGV_VALUETYPE_WCHAR;
    pArg->pwszValue     = pwszFile;

    this->StripQuotes(pArg->pwszValue);

    return(TRUE);
}

BOOL cWArgv_::ProcessCommandFile(WCHAR *pwszFile)
{
    if (!(this->fChkCmdF))
    {
        return(FALSE);
    }

    HANDLE      hFile;
    fParse_     fp(pwszFile);
    WCHAR       *pwsz;

    fp.Reset();

    while (fp.GetNextLine())
    {
        fp.EOLRemove();

        if ((fp.GetCurrentLine()) && (fp.GetCurrentLine()[0]))
        {
            pwsz = fp.GetCurrentLine();
            this->Fill(1, &pwsz);
        }
    }

    return(TRUE);
}

void cWArgv_::StripQuotes(WCHAR *pwszIn)
{
    DWORD   dwSrc;
    DWORD   dwDst;
    DWORD   dwLen;

    dwSrc = 0;
    dwDst = 0;
    dwLen = wcslen(pwszIn);

    while (dwSrc < dwLen)
    {
        if (pwszIn[dwSrc] != L'\"')
        {
            pwszIn[dwDst] = pwszIn[dwSrc];
            dwDst++;
        }
        dwSrc++;
    }

    pwszIn[dwDst] = NULL;
}
