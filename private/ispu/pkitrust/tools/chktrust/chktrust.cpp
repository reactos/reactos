//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       chktrust.cpp
//
//  Contents:   Microsoft Internet Security Trust Checker
//
//  History:    05-May-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    <stdio.h>
#include    <windows.h>
#include    <io.h>	
#include    <wchar.h>					

#include    "cryptreg.h"
#include    "wincrypt.h"
#include    "wintrust.h"
#include    "softpub.h"
#include    "mscat.h"
#include    "unicode.h"
#include    "dbgdef.h"

#include    "gendefs.h"
#include    "cwargv.hxx"
#include    "printfu.hxx"

#include    "mssip.h"
#include    "resource.h"

HRESULT _CallWVT(WCHAR *pwszFilename);
HRESULT _ExplodeCatalog(WCHAR *pwszCatalogFile);
HRESULT _CallCatalogWVT(WCHAR *pwszCatalogFile, WCHAR *pwszMemberTag, WCHAR *pwszMemberFile);
int     _ShowError(DWORD dwError, WCHAR *pwszFile);
void    _ToLower(WCHAR *pwszInOut);
HRESULT _AddCatalogToDatabase(WCHAR *pwszFileIn);


GUID        guidPublishedSoftware   = WINTRUST_ACTION_GENERIC_VERIFY_V2;
GUID        guidProviderTest        = WINTRUST_ACTION_TRUSTPROVIDER_TEST;
GUID        guidProviderDriver      = DRIVER_ACTION_VERIFY;
GUID        guidPassedIn;
GUID        guidCatRoot;

GUID        *pguidActionID          = &guidPublishedSoftware;
GUID        *pguidCatRoot           = NULL;

DWORD       dwExpectedError         = ERROR_SUCCESS;

WCHAR       *pwszCatalogFile        = NULL;
WCHAR       *pwszCatalogMember      = NULL;

PrintfU_    *pPrint                 = NULL;

HCATADMIN   hCatAdmin               = NULL;

BOOL        fVerbose;
BOOL        fQuiet;
BOOL        fIECall;
BOOL        fTestDump;
BOOL        fCheckExpectedError     = FALSE;
BOOL        fProcessAllCatMembers;
BOOL        fCatalogMemberVerify    = FALSE;
BOOL        fUseCatalogDatabase;
BOOL        fAdd2CatalogDatabase;
BOOL        fReplaceCatfile;
BOOL        fNT5;
BOOL        fNoTimeStampWarning;

extern "C" int __cdecl wmain(int argc, WCHAR **wargv)
{
    cWArgv_                 *pArgs;

    BOOL                    fFind;
    HANDLE                  hFind;
    WIN32_FIND_DATA         sFindData;

    int                     iRet;
    HRESULT                 hr;

    WCHAR                   *pwszFileIn;
    WCHAR                   *pwszLastSlash;
    WCHAR                   wszFile[MAX_PATH];
    WCHAR                   wszDir[MAX_PATH];
    char                    szFile[MAX_PATH * 2];
    DWORD                   dwFiles;
    DWORD                   dwDirLen;

    iRet    = 0;
    pPrint  = NULL;
    hFind   = INVALID_HANDLE_VALUE;
    dwFiles = 0;

    if (!(pPrint = new PrintfU_()))
    {
        goto MemoryError;
    }

    if (!(pArgs = new cWArgv_((HINSTANCE)GetModuleHandle(NULL), FALSE)))
    {
        goto MemoryError;
    }

    pArgs->AddUsageText(IDS_USAGETEXT_USAGE, IDS_USAGETEXT_OPTIONS,
                        IDS_USAGETEXT_CMDFILE, IDS_USAGETEXT_ADD,
                        IDS_USAGETEXT_OPTPARAM);

    pArgs->Add2List(IDS_PARAM_HELP,         IDS_PARAMTEXT_HELP,         WARGV_VALUETYPE_BOOL, (void *)FALSE, FALSE);
    pArgs->Add2List(IDS_PARAM_VERBOSE,      IDS_PARAMTEXT_VERBOSE,      WARGV_VALUETYPE_BOOL, (void *)FALSE, FALSE);
    pArgs->Add2List(IDS_PARAM_QUIET,        IDS_PARAMTEXT_QUIET,        WARGV_VALUETYPE_BOOL, (void *)FALSE, FALSE);
    pArgs->Add2List(IDS_PARAM_TPROV,        IDS_PARAMTEXT_TPROV,        WARGV_VALUETYPE_WCHAR, NULL,         TRUE);
    pArgs->Add2List(IDS_PARAM_IECALL,       IDS_PARAMTEXT_IECALL,       WARGV_VALUETYPE_BOOL, (void *)FALSE, TRUE);
    pArgs->Add2List(IDS_PARAM_TESTDUMP,     IDS_PARAMTEXT_TESTDUMP,     WARGV_VALUETYPE_BOOL, (void *)FALSE, TRUE);
    pArgs->Add2List(IDS_PARAM_EXPERROR,     IDS_PARAMTEXT_EXPERROR,     WARGV_VALUETYPE_DWORDH, NULL,        TRUE);
    pArgs->Add2List(IDS_PARAM_TESTDRV,      IDS_PARAMTEXT_TESTDRV,      WARGV_VALUETYPE_BOOL, (void *)FALSE, TRUE);
    pArgs->Add2List(IDS_PARAM_CATFILE,      IDS_PARAMTEXT_CATFILE,      WARGV_VALUETYPE_WCHAR, NULL,         TRUE);
    pArgs->Add2List(IDS_PARAM_CATMEMBER,    IDS_PARAMTEXT_CATMEMBER,    WARGV_VALUETYPE_WCHAR, NULL,         TRUE);
    pArgs->Add2List(IDS_PARAM_ALLCATMEM,    IDS_PARAMTEXT_ALLCATMEM,    WARGV_VALUETYPE_BOOL, (void *)FALSE, TRUE);
    pArgs->Add2List(IDS_PARAM_CATUSELIST,   IDS_PARAMTEXT_CATUSELIST,   WARGV_VALUETYPE_BOOL, (void *)FALSE, TRUE);
    pArgs->Add2List(IDS_PARAM_CATADDLIST,   IDS_PARAMTEXT_CATADDLIST,   WARGV_VALUETYPE_BOOL, (void *)FALSE, TRUE);
    pArgs->Add2List(IDS_PARAM_REPLACECATFILE,IDS_PARAMTEXT_REPLACECATFILE,   WARGV_VALUETYPE_BOOL, (void *)FALSE, TRUE);
    pArgs->Add2List(IDS_PARAM_CATROOT,      IDS_PARAMTEXT_CATROOT,      WARGV_VALUETYPE_WCHAR, NULL,         TRUE);
    pArgs->Add2List(IDS_PARAM_NT5,          IDS_PARAMTEXT_NT5,          WARGV_VALUETYPE_BOOL, (void *)FALSE, TRUE);
    pArgs->Add2List(IDS_PARAM_TSWARN,       IDS_PARAMTEXT_TSWARN,       WARGV_VALUETYPE_BOOL, (void *)FALSE, TRUE);
    
    
    if (!(pArgs->Fill(argc, wargv)) ||
        (pArgs->GetValue(IDS_PARAM_HELP)))
    {
        wprintf(L"%s", pArgs->GetUsageString());
        goto NeededHelp;
    }

    pwszCatalogFile         = (WCHAR *)pArgs->GetValue(IDS_PARAM_CATFILE);
    pwszCatalogMember       = (WCHAR *)pArgs->GetValue(IDS_PARAM_CATMEMBER);
    fVerbose                = (BOOL)((DWORD_PTR)pArgs->GetValue(IDS_PARAM_VERBOSE));
    fQuiet                  = (BOOL)((DWORD_PTR)pArgs->GetValue(IDS_PARAM_QUIET));
    fIECall                 = (BOOL)((DWORD_PTR)pArgs->GetValue(IDS_PARAM_IECALL));
    fTestDump               = (BOOL)((DWORD_PTR)pArgs->GetValue(IDS_PARAM_TESTDUMP));
    fProcessAllCatMembers   = (BOOL)((DWORD_PTR)pArgs->GetValue(IDS_PARAM_ALLCATMEM));
    fUseCatalogDatabase     = (BOOL)((DWORD_PTR)pArgs->GetValue(IDS_PARAM_CATUSELIST));
    fAdd2CatalogDatabase    = (BOOL)((DWORD_PTR)pArgs->GetValue(IDS_PARAM_CATADDLIST));
    fReplaceCatfile         = (BOOL)((DWORD_PTR)pArgs->GetValue(IDS_PARAM_REPLACECATFILE));
    fNT5                    = (BOOL)((DWORD_PTR)pArgs->GetValue(IDS_PARAM_NT5));
    fNoTimeStampWarning     = (BOOL)((DWORD_PTR)pArgs->GetValue(IDS_PARAM_TSWARN));

    //
    // the win2k flag implies -q and -ucl (unless -acl is used, then it just implies -q)
    //
    if (fNT5)
    {
        fQuiet = TRUE;
        if (!fAdd2CatalogDatabase)
        {
            fUseCatalogDatabase = TRUE;
        }
    }

    if (fUseCatalogDatabase || fNT5)
    {
        fCatalogMemberVerify = TRUE;
    }

    if (pArgs->IsSet(IDS_PARAM_EXPERROR))
    {
        dwExpectedError     = (DWORD)((DWORD_PTR)pArgs->GetValue(IDS_PARAM_EXPERROR));
        fCheckExpectedError = TRUE;
    }

    if (!(pwszFileIn = pArgs->GetFileName()))
    {
        wprintf(L"%s", pArgs->GetUsageString());
        goto ParamError;
    }

    if (((pwszCatalogFile) && !(pwszCatalogMember)) ||
        (!(pwszCatalogFile) && (pwszCatalogMember)))
    {
        wprintf(L"%s", pArgs->GetUsageString());
        goto ParamError;
    }

    if ((pwszCatalogFile) && (pwszCatalogMember))
    {
        fCatalogMemberVerify = TRUE;
    }

    //
    //  set the appropriete provider
    //
    if (pArgs->IsSet(IDS_PARAM_TPROV) || fNT5)
    {
        if (fNT5)
        {
            wstr2guid(L"{F750E6C3-38EE-11D1-85E5-00C04FC295EE}", &guidPassedIn);
        }
        else if (!(wstr2guid((WCHAR *)pArgs->GetValue(IDS_PARAM_TPROV), &guidPassedIn)))
        {
            goto GuidError;
        }

        pguidActionID   = &guidPassedIn;
    }
    else if (fTestDump)
    {
        pguidActionID   = &guidProviderTest;
    }
    else if (pArgs->GetValue(IDS_PARAM_TESTDRV))
    {
        pguidActionID   = &guidProviderDriver;
    }
    else
    {
        pguidActionID   = &guidPublishedSoftware;
    }

    //
    // Get the catalog subsystem GUID to use
    //
    if (pArgs->IsSet(IDS_PARAM_CATROOT) || fNT5)
    {
        if (fNT5)
        {
            wstr2guid(L"{F750E6C3-38EE-11D1-85E5-00C04FC295EE}", &guidCatRoot);
        }
        else if (!(wstr2guid((WCHAR *)pArgs->GetValue(IDS_PARAM_CATROOT), &guidCatRoot)))
        {
            goto GuidError;
        }

        pguidCatRoot   = &guidCatRoot;
    }

    //
    //  if we are calling just like IE, we only have one file and don't want to
    //  check if it exists or not... just call WVT.
    //
    if (fIECall)
    {
        dwFiles++;

        hr = _CallWVT(pwszFileIn);

        iRet = _ShowError(hr, pwszFileIn);

        goto CommonReturn;
    }

    //
    //  OK....   go into a findfirst/next loop we could have been called with *.*
    //
    if (pwszLastSlash = wcsrchr(pwszFileIn, L'\\'))
    {
        *pwszLastSlash  = NULL;
        wcscpy(&wszDir[0], pwszFileIn);
        wcscat(&wszDir[0], L"\\");
        *pwszLastSlash  = L'\\';
        dwDirLen        = wcslen(&wszDir[0]);
    }
    else
    {
        wszDir[0]   = NULL;
        dwDirLen    = 0;
    }

    szFile[0] = NULL;
    WideCharToMultiByte(0, 0, pwszFileIn, -1, &szFile[0], MAX_PATH * 2, NULL, NULL);

    if ((hFind = FindFirstFile(&szFile[0], &sFindData)) == INVALID_HANDLE_VALUE)
    {
        pPrint->Display(IDS_CAN_NOT_OPEN_FILE, pwszFileIn);
        goto FileNotFound;
    }

    fFind   = TRUE;
    dwFiles = 0;

    while (fFind)
    {
        if (!(sFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            if (dwDirLen > 0)
            {
                wcscpy(&wszFile[0], &wszDir[0]);
            }

            wszFile[dwDirLen] = NULL;
            MultiByteToWideChar(0, 0, &sFindData.cFileName[0], -1, &wszFile[dwDirLen], MAX_PATH * sizeof(WCHAR));

            if (wszFile[0])
            {
                if (fAdd2CatalogDatabase)
                {
                    hr = _AddCatalogToDatabase(&wszFile[0]);
                }
                else
                {
                    hr = _CallWVT(&wszFile[0]);
                }

                iRet = _ShowError(hr, &wszFile[0]);

                if (iRet == 0)
                {
                    hr = ERROR_SUCCESS;
                }

                dwFiles++;
            }
        }

        fFind = FindNextFile(hFind, &sFindData);
	}

    if (dwFiles < 1)
    {
        pPrint->Display(IDS_CAN_NOT_OPEN_FILE, pwszFileIn);
        goto FileNotFound;
    }


    CommonReturn:
        DELETE_OBJECT(pArgs);
        DELETE_OBJECT(pPrint);

        if (hFind != INVALID_HANDLE_VALUE)
        {
            FindClose(hFind);
        }

        if (hCatAdmin)
        {
            CryptCATAdminReleaseContext(hCatAdmin, 0);
        }


        return(iRet);

    ErrorReturn:
        iRet = 1;
        goto CommonReturn;

    TRACE_ERROR_EX(DBG_SS_APP, MemoryError);
    TRACE_ERROR_EX(DBG_SS_APP, FileNotFound);
    TRACE_ERROR_EX(DBG_SS_APP, ParamError);
    TRACE_ERROR_EX(DBG_SS_APP, GuidError);
    TRACE_ERROR_EX(DBG_SS_APP, NeededHelp);
}

HRESULT _CallWVT(WCHAR *pwszFilename)
{
    if (fCatalogMemberVerify)
    {
        return(_CallCatalogWVT(pwszCatalogFile, pwszCatalogMember, pwszFilename));
    }

    HRESULT                 hr;
    WINTRUST_DATA           sWTD;
    WINTRUST_FILE_INFO      sWTFI;

    memset(&sWTD, 0x00, sizeof(WINTRUST_DATA));

    sWTD.cbStruct           = sizeof(WINTRUST_DATA);
    sWTD.dwUIChoice         = (fQuiet) ? WTD_UI_NONE : WTD_UI_ALL;
    sWTD.dwUnionChoice      = WTD_CHOICE_FILE;
    sWTD.pFile              = &sWTFI;

    memset(&sWTFI, 0x00, sizeof(WINTRUST_FILE_INFO));

    sWTFI.cbStruct          = sizeof(WINTRUST_FILE_INFO);
    sWTFI.pcwszFilePath     = pwszFilename;
    sWTFI.hFile             = CreateFileU(pwszFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                          FILE_ATTRIBUTE_NORMAL, NULL);

    hr = WinVerifyTrust(NULL, pguidActionID, &sWTD);

    if (sWTFI.hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(sWTFI.hFile);
    }

    if ((fCheckExpectedError) && ((DWORD)hr == dwExpectedError) && (fProcessAllCatMembers))
    {
        if (IsCatalogFile(INVALID_HANDLE_VALUE, pwszFilename))
        {
            return(_ExplodeCatalog(pwszFilename));
        }
    }

    return(hr);
}

HRESULT _ExplodeCatalog(WCHAR *pwszCatalogFile)
{
    HRESULT         hrReturn;
    HANDLE          hCat;
    CRYPTCATMEMBER  *psMember;

    hrReturn = ERROR_SUCCESS;

    //
    // open the catalog
    //
    if (!(hCat = CryptCATOpen(pwszCatalogFile, 0, NULL, 0, 0)))
    {
        goto ErrorCatOpen;
    }

    psMember = NULL;

    while (psMember = CryptCATEnumerateMember(hCat, psMember))
    {
        hrReturn |= _CallCatalogWVT(pwszCatalogFile, psMember->pwszReferenceTag,
                                    psMember->pwszReferenceTag);
    }

    CommonReturn:
        if (hCat)
        {
            CryptCATClose(hCat);
        }

        return(hrReturn);

    ErrorReturn:
        hrReturn = GetLastError();
        goto CommonReturn;

    TRACE_ERROR_EX(DBG_SS_APP, ErrorCatOpen);
}

HRESULT _CallCatalogWVT(WCHAR *pwszCatalogFile, WCHAR *pwszMemberTag, WCHAR *pwszMemberFile)
{
    HRESULT                 hr;
    DWORD                   cbHash;
    BYTE                    bHash[40];
    WCHAR                   *pwsz;
    WINTRUST_DATA           sWTD;
    WINTRUST_CATALOG_INFO   sWTCI;

    memset(&sWTD, 0x00, sizeof(WINTRUST_DATA));

    sWTD.cbStruct           = sizeof(WINTRUST_DATA);
    sWTD.dwUIChoice         = (fQuiet) ? WTD_UI_NONE : WTD_UI_ALL;
    sWTD.dwUnionChoice      = WTD_CHOICE_CATALOG;
    sWTD.pCatalog           = &sWTCI;

    memset(&sWTCI, 0x00, sizeof(WINTRUST_CATALOG_INFO));

    sWTCI.cbStruct              = sizeof(WINTRUST_CATALOG_INFO);
    sWTCI.pcwszCatalogFilePath  = pwszCatalogFile;
    sWTCI.pcwszMemberTag        = pwszMemberTag;
    sWTCI.pcwszMemberFilePath   = pwszMemberFile;
    sWTCI.hMemberFile           = CreateFileU(pwszMemberFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                              FILE_ATTRIBUTE_NORMAL, NULL);

    cbHash  = 40;

    if (!(CryptCATAdminCalcHashFromFileHandle(sWTCI.hMemberFile, &cbHash, &bHash[0], 0)))
    {
        goto CatAdminCalcHashError;
    }

    sWTCI.pbCalculatedFileHash  = &bHash[0];
    sWTCI.cbCalculatedFileHash  = cbHash;

    if (fUseCatalogDatabase)
    {
        HCATINFO                hCatInfo;
        CATALOG_INFO            sCatInfo;

        if (!(hCatAdmin))
        {
            if (!(CryptCATAdminAcquireContext(&hCatAdmin, pguidCatRoot, 0)))
            {
                goto CatAdminAcquireError;
            }
        }

        pwsz    = NULL;

        if (pwsz = wcsrchr(pwszMemberFile, L'\\'))
        {
            pwsz++;
        }
        else
        {
            pwsz = pwszMemberFile;
        }

        _ToLower(pwsz);

        sWTCI.pcwszMemberTag = pwsz;

        memset(&sCatInfo, 0x00, sizeof(CATALOG_INFO));
        sCatInfo.cbStruct = sizeof(CATALOG_INFO);

        hCatInfo = NULL;

        while (hCatInfo = CryptCATAdminEnumCatalogFromHash(hCatAdmin, &bHash[0], cbHash, 0, &hCatInfo))
        {
            if (!(CryptCATCatalogInfoFromContext(hCatInfo, &sCatInfo, 0)))
            {
                // should do something (??)
                continue;
            }

            sWTCI.pcwszCatalogFilePath = &sCatInfo.wszCatalogFile[0];

            hr = WinVerifyTrust(NULL, pguidActionID, &sWTD);

            if (hr == (HRESULT)dwExpectedError)
            {
                CryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfo, 0);

                goto CommonReturn;
            }
        }

        SetLastError(ERROR_NOT_FOUND);

        goto CatMemberNotFound;
    }

    hr = WinVerifyTrust(NULL, pguidActionID, &sWTD);

    CommonReturn:
        if (sWTCI.hMemberFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(sWTCI.hMemberFile);
        }

        return(hr);

    ErrorReturn:
        hr = GetLastError();
        goto CommonReturn;

    TRACE_ERROR_EX(DBG_SS_APP, CatAdminCalcHashError);
    TRACE_ERROR_EX(DBG_SS_APP, CatAdminAcquireError);
    TRACE_ERROR_EX(DBG_SS_APP, CatMemberNotFound);
}



BOOL 
OpenSIP(const WCHAR* pwsFileName,
		SIP_SUBJECTINFO** ppSubjectInfo,
		SIP_DISPATCH_INFO** ppDispatchInfo,
		GUID* pgSubject)
{
	*ppSubjectInfo = (SIP_SUBJECTINFO*) new(BYTE[sizeof(SIP_SUBJECTINFO)]);
	*ppDispatchInfo = (SIP_DISPATCH_INFO*) new(BYTE[sizeof(SIP_DISPATCH_INFO)]);

	if ((pgSubject == NULL)		||
		(*ppSubjectInfo == NULL)	||
		(*ppDispatchInfo == NULL))
	{
		return FALSE;
	}

	memset((void*)*ppSubjectInfo, 0, sizeof(SIP_SUBJECTINFO));
	memset((void*)*ppDispatchInfo, 0, sizeof(SIP_DISPATCH_INFO));
	memset((void*)pgSubject, 0, sizeof(GUID));

	// Get the type of SIP
	if (!CryptSIPRetrieveSubjectGuid(
					pwsFileName,
					NULL,
					pgSubject))
	{
		goto ErrorReturn;
	}

	(*ppDispatchInfo)->cbSize = sizeof(SIP_DISPATCH_INFO);

	// Load the SIP
	if (!CryptSIPLoad(
				pgSubject,
				0,
				*ppDispatchInfo))
	{
		goto ErrorReturn;
	}

	// Fill in the SIP_SUBJECTINFO struct
	(*ppSubjectInfo)->cbSize = sizeof(SIP_SUBJECTINFO);
	(*ppSubjectInfo)->pgSubjectType = pgSubject;
	(*ppSubjectInfo)->pwsFileName = pwsFileName;

	goto CommonReturn;

ErrorReturn:
	delete[](*ppSubjectInfo);
	delete[](*ppDispatchInfo);
	return FALSE;

CommonReturn:
	return TRUE;
}


BOOL 
ReadMsgBlob(const WCHAR* pwsFileName,
			CRYPT_DATA_BLOB* pData)
{
	if ((pwsFileName == NULL)	||
		(pData == NULL))
	{
		return FALSE;
	}

	SIP_SUBJECTINFO* pSubjectInfo = NULL;
	SIP_DISPATCH_INFO* pDispatchInfo = NULL;
	GUID gSubject;

	memset((void*)&gSubject, 0, sizeof(gSubject));

	// Get the SIP
	if (!OpenSIP(
				pwsFileName,
				&pSubjectInfo,
				&pDispatchInfo,
				&gSubject))
	{
		return FALSE;
	}

	// Load the message blob
	DWORD dwEncodingType = 0;
	pData->cbData = 0;

	if (!pDispatchInfo->pfGet(
						pSubjectInfo,
						&dwEncodingType,
						0,
						&(pData->cbData),
						NULL))
	{
		delete[](pSubjectInfo);
		delete[](pDispatchInfo);
		return FALSE;
	}

	pData->pbData = (BYTE*)new(BYTE[pData->cbData]);
	if (pData->pbData == NULL)
		return FALSE;

	memset((void*)pData->pbData, 0, pData->cbData);

	if (!pDispatchInfo->pfGet(
						pSubjectInfo,
						&dwEncodingType,
						0,
						&(pData->cbData),
						pData->pbData))
	{
		delete[](pData->pbData);
		delete[](pSubjectInfo);
		delete[](pDispatchInfo);
		return FALSE;
	}

	delete[](pSubjectInfo);
	delete[](pDispatchInfo);
	return TRUE;
}


BOOL
CheckForTimeStamp(WCHAR *pwszFile)
{
    BOOL        fRet = TRUE;
    HANDLE      hFile = INVALID_HANDLE_VALUE;
    HCRYPTMSG   hMsg = NULL;
    BYTE        *pb = NULL;
    DWORD       cb = 0;
    DWORD       cbRead = 0;
    PCMSG_ATTR  pMsgAttr = NULL;
    DWORD       cbMsgAttr = 0;
    CRYPT_ATTRIBUTE     *pAttr = NULL;
    CRYPT_DATA_BLOB blob;
    DWORD       dwEncodingType;

    if (!ReadMsgBlob(pwszFile, &blob))
    {
        return FALSE;
    }

    //
    // If the encoded message was passed in the use CryptMsg to crack the encoded PKCS7 Signed Message
    //
    if (!(hMsg = CryptMsgOpenToDecode(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING ,
                                      0,
                                      0,
                                      0,
                                      NULL,
                                      NULL)))
    {
        goto ErrorReturn;
    }

    if (!CryptMsgUpdate(hMsg,
                        blob.pbData,
                        blob.cbData,
                        TRUE))                    // fFinal
    {
        CryptMsgClose(hMsg);
        goto ErrorReturn;
    }
    
    //
    // get the unauthenticated attributes because that is where the counter signer is
    //
    CryptMsgGetParam(hMsg,
                     CMSG_SIGNER_UNAUTH_ATTR_PARAM,
                     0,
                     NULL,
                     &cbMsgAttr);

    if (cbMsgAttr == 0)
    {
        goto ErrorReturn;
    }

    if (NULL == (pMsgAttr = (CMSG_ATTR *) new(BYTE[cbMsgAttr])))
    {
        goto ErrorReturn;
    }

    if (!CryptMsgGetParam(hMsg,
                          CMSG_SIGNER_UNAUTH_ATTR_PARAM,
                          0,
                          (void *) pMsgAttr,
                          &cbMsgAttr))
    {
        goto ErrorReturn;
    }

    //
    // search for the counter signer in the unauthenticated attributes
    //
    if ((pAttr = CertFindAttribute(szOID_RSA_counterSign,
                                   pMsgAttr->cAttr,
                                   pMsgAttr->rgAttr)) == NULL)
    {
        //
        //  no counter signature
        //
        goto ErrorReturn;
    }

    
Cleanup:

    delete[](blob.pbData);

    if (pMsgAttr)
        delete[](pMsgAttr);

    if (hMsg != NULL)
        CryptMsgClose(hMsg);

    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto Cleanup;


}

int _ShowError(DWORD dwError, WCHAR *pwszFile)
{
    pPrint->Display(IDS_FILEREF, pwszFile);

    if (fCheckExpectedError)
    {
        if (dwError == dwExpectedError)
        {
            pPrint->Display(IDS_SUCCEEDED);
            return(0);
        }
        else
        {
            pPrint->Display(IDS_EXPECTED_HRESULT, dwExpectedError, dwError);
            return(1);
        }
    }

	switch(dwError)
	{
		case S_OK:
            pPrint->Display(IDS_SUCCEEDED);
            if (fNoTimeStampWarning)
            {
                if (!CheckForTimeStamp(pwszFile))
                {
                    pPrint->Display(IDS_NO_TIMESTAMP_WARNING);
                }
            }
            return(0);

		case TRUST_E_SUBJECT_FORM_UNKNOWN:
            pPrint->Display(IDS_UNKNOWN_FILE_TYPE);
			break;

		case TRUST_E_PROVIDER_UNKNOWN:
            pPrint->Display(IDS_UNKNOWN_PROVIDER);
			break;

		case TRUST_E_ACTION_UNKNOWN:
            pPrint->Display(IDS_UNKNOWN_ACTION);
			break;

        case TRUST_E_SUBJECT_NOT_TRUSTED:
            pPrint->Display(IDS_SUBJECT_NOT_TRUSTED);
			break;

		default:
            pPrint->Display(IDS_FAIL);
			break;
    }
    return(1);
}

HRESULT _AddCatalogToDatabase(WCHAR *pwszFileIn)
{
    HCATINFO                hCatInfo;
    WCHAR                   *pwszBaseName;
    
    if (!(hCatAdmin))
    {
        if (!(CryptCATAdminAcquireContext(&hCatAdmin, pguidCatRoot, 0)))
        {
            return(GetLastError());
        }
    }

    //
    //  set the base file name
    //
    if (!(pwszBaseName = wcsrchr(pwszFileIn, L'\\')))
    {
        pwszBaseName = wcsrchr(pwszFileIn, L':');
    }

    if (pwszBaseName)
    {
        *pwszBaseName++;
    }
    else
    {
        pwszBaseName = pwszFileIn;
    }

    if (hCatInfo = CryptCATAdminAddCatalog(hCatAdmin, pwszFileIn, fReplaceCatfile ? pwszBaseName : NULL, 0))
    {
        CryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfo, 0);

        return(ERROR_SUCCESS);
    }

    return(GetLastError());
}

void _ToLower(WCHAR *pwszInOut)
{
    while (*pwszInOut)
    {
        *pwszInOut = towlower(*pwszInOut);
        pwszInOut++;
    }
}

