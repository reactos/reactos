//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       test.cpp
//
//  Contents:   Microsoft Internet Security Authenticode Policy Provider
//
//  Functions:  SoftpubDumpStructure
//
//  History:    05-Jun-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"

#define     TEST_POLICY_DUMP_FILE       L"C:\\TRUSTPOL.TXT"

void _cdecl FPrintfU(HANDLE hFile, WCHAR *pwszFmt, ...);
void PrintfPFNs(HANDLE hFile, CRYPT_PROVIDER_DATA *pProvData);
void PrintfSignerStruct(HANDLE hFile, CRYPT_PROVIDER_SGNR *pS, int idxSigner, BOOL fCounter, int idxCounter);
void PrintfCertStruct(HANDLE hFile, int cCert, CRYPT_PROVIDER_CERT *pC, int idxCert);
void GetStringDateTime(FILETIME *pFTime, WCHAR *pwszRetTime, WCHAR *pwszRetDate);
WCHAR *GetNameFromBlob(CERT_NAME_BLOB *psNameBlob);

HRESULT WINAPI SoftpubDumpStructure(CRYPT_PROVIDER_DATA *pProvData)
{
    HANDLE  hFile;

    if ((hFile = CreateFileU(TEST_POLICY_DUMP_FILE,
                             GENERIC_WRITE | GENERIC_READ,
                             0,                 // no sharing!!
                             NULL,
                             CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL)) == INVALID_HANDLE_VALUE)
    {
        return(S_FALSE);
    }

    WCHAR           wszTime[64];
    WCHAR           wszDate[128];
    WCHAR           wszGuid[64];
    int             i, i2, i3;

    //
    //  CRYPT_PROVIDER_DATA
    //
    FPrintfU(hFile, L"CRYPT_PROVIDER_DATA:\r\n");


    //
    //  WINTRUST_DATA
    //
    WINTRUST_DATA   *pWT;

    pWT = pProvData->pWintrustData;

    FPrintfU(hFile, L"+======================================================\r\n");
    FPrintfU(hFile, L"+-- pWintrustData:\r\n");
    FPrintfU(hFile, L"|   |.. cbStruct:                     %ld\r\n", pWT->cbStruct);
    FPrintfU(hFile, L"|   |.. pPolicyCallbackData:          %p\r\n", pWT->pPolicyCallbackData);
    FPrintfU(hFile, L"|   |.. dwUIChoice:                   %ld\r\n", pWT->dwUIChoice);
    FPrintfU(hFile, L"|   |.. fdRevocationChecks:           %ld\r\n", pWT->fdwRevocationChecks);
    FPrintfU(hFile, L"|   |.. dwUnionChoice:                %ld\r\n", pWT->dwUnionChoice);

    switch (pWT->dwUnionChoice)
    {
        case WTD_CHOICE_FILE:
            if (!(pWT->pFile) ||
                !(WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(WINTRUST_FILE_INFO, pWT->pFile->cbStruct, hFile)))
            {
                FPrintfU(hFile, L"|   +-- pFile: <<< bad parameter! >>>\r\n");
                break;
            }
            FPrintfU(hFile, L"|   +-- pFile:\r\n");
            FPrintfU(hFile, L"|       |.. cbStruct:                 %ld\r\n", pWT->pFile->cbStruct);
            FPrintfU(hFile, L"|       |.. pcwszFilePath:            %s\r\n", pWT->pFile->pcwszFilePath);
            FPrintfU(hFile, L"|       |.. hFile:                    0x%p\r\n", pWT->pFile->hFile);

            wszGuid[0] = NULL;
            if (WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(WINTRUST_FILE_INFO,
                    pWT->pFile->cbStruct, pgKnownSubject) &&
                        pWT->pFile->pgKnownSubject)
            {
                guid2wstr(pWT->pFile->pgKnownSubject, &wszGuid[0]);
            }
            FPrintfU(hFile, L"|       +-- pgKnownSubject:           %s\r\n", &wszGuid[0]);
            break;

        case WTD_CHOICE_CATALOG:
            if (!(pWT->pCatalog) ||
                !(WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(WINTRUST_CATALOG_INFO, pWT->pCatalog->cbStruct, hMemberFile)))
            {
                FPrintfU(hFile, L"|   +-- pCatalog: <<< bad parameter! >>>\r\n");
                break;
            }
            FPrintfU(hFile, L"|   +-- pCatalog:\r\n");
            FPrintfU(hFile, L"|       |.. cbStruct:                 %ld\r\n", pWT->pCatalog->cbStruct);
            FPrintfU(hFile, L"|       |.. dwCatalogVersion:         0x%lx\r\n", pWT->pCatalog->dwCatalogVersion);
            FPrintfU(hFile, L"|       |.. pcwszCatalogFilePath:     %s\r\n", pWT->pCatalog->pcwszCatalogFilePath);
            FPrintfU(hFile, L"|       |.. pcwszMemberTag:           %s\r\n", pWT->pCatalog->pcwszMemberTag);
            FPrintfU(hFile, L"|       |.. pcwszMemberFilePath:      %s\r\n", pWT->pCatalog->pcwszMemberFilePath);
            FPrintfU(hFile, L"|       |.. hMemberFile:              0x%p\r\n", pWT->pCatalog->hMemberFile);
            FPrintfU(hFile, L"|       |.. pbCaclulatedFileHash:     ");

            for (i = 0; i < (int)pWT->pCatalog->cbCalculatedFileHash; i++)
            {
                FPrintfU(hFile, L"%02.2X", pWT->pCatalog->pbCalculatedFileHash[i]);
            }
            FPrintfU(hFile, L"\r\n");
            FPrintfU(hFile, L"|       +-- cbCaclulatedFileHash:     %ld\r\n", pWT->pCatalog->cbCalculatedFileHash);
            break;

        case WTD_CHOICE_BLOB:
            if (!(pWT->pBlob) ||
                !(WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(WINTRUST_BLOB_INFO, pWT->pBlob->cbStruct, pbMemSignedMsg)))
            {
                FPrintfU(hFile, L"|   +-- pBlob: <<< bad parameter! >>>\r\n");
                break;
            }
            FPrintfU(hFile, L"|   +-- pBlob:\r\n");
            FPrintfU(hFile, L"|       |.. cbStruct:                 %ld\r\n", pWT->pBlob->cbStruct);
            wszGuid[0] = NULL;
            guid2wstr(&pWT->pBlob->gSubject, &wszGuid[0]);
            FPrintfU(hFile, L"        |.. gSubject:                 %s\r\n", &wszGuid[0]);
            FPrintfU(hFile, L"|       |.. pcwszDisplayName:         %s\r\n", pWT->pBlob->pcwszDisplayName);
            FPrintfU(hFile, L"|       |.. cbMemObject:              %ld\r\n", pWT->pBlob->cbMemObject);
            FPrintfU(hFile, L"|       |.. pbMemObject:              0x%p\r\n", pWT->pBlob->pbMemObject);
            FPrintfU(hFile, L"|       |.. cbMemSignedMsg:           %ld\r\n", pWT->pBlob->cbMemSignedMsg);
            FPrintfU(hFile, L"|       +.. pbMemSignedMsg:           0x%p\r\n", pWT->pBlob->pbMemSignedMsg);
            break;

        case WTD_CHOICE_SIGNER:
            if (!(pWT->pSgnr) ||
                !(WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(WINTRUST_SGNR_INFO, pWT->pSgnr->cbStruct, pahStores)))
            {
                FPrintfU(hFile, L"|   +-- pSgnr: <<< bad parameter! >>>\r\n");
                break;
            }
            FPrintfU(hFile, L"|   +-- pSgnr:\r\n");
            FPrintfU(hFile, L"|       |.. cbStruct:                 %ld\r\n", pWT->pSgnr->cbStruct);
            FPrintfU(hFile, L"|       |.. pcwszDisplayName:         %s\r\n", pWT->pSgnr->pcwszDisplayName);
            FPrintfU(hFile, L"|       |.. psSignerInfo:             0x%p\r\n", pWT->pSgnr->psSignerInfo);
            FPrintfU(hFile, L"|       |.. chStores:                 %ld\r\n", pWT->pSgnr->chStores);
            for (i = 0; i < (int)pWT->pSgnr->chStores; i++)
            {
                if (i == (int)(pWT->pSgnr->chStores - 1))
                {
                    FPrintfU(hFile, L"|       +.. pahStores[%02.2d]:        0x%p\r\n", i, pWT->pSgnr->pahStores[i]);
                }
                else
                {
                    FPrintfU(hFile, L"|       |.. pahStores[%02.2d]:        0x%p\r\n", i, pWT->pSgnr->pahStores[i]);
                }
            }
            break;

        case WTD_CHOICE_CERT:
            if (!(pWT->pCert) ||
                !(WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(WINTRUST_CERT_INFO, pWT->pCert->cbStruct, psftVerifyAsOf)))
            {
                FPrintfU(hFile, L"|   +-- pCert: <<< bad parameter! >>>\r\n");
                break;
            }
            FPrintfU(hFile, L"|   +-- pCert:\r\n");
            FPrintfU(hFile, L"|       |.. cbStruct:                 %ld\r\n", pWT->pCert->cbStruct);
            FPrintfU(hFile, L"|       |.. pcwszDisplayName:         %s\r\n", pWT->pCert->pcwszDisplayName);
            FPrintfU(hFile, L"|       |.. psCertContext:            0x%p\r\n", pWT->pCert->psCertContext);
            FPrintfU(hFile, L"|       |.. chStores:                 %ld\r\n", pWT->pCert->chStores);
            for (i = 0; i < (int)pWT->pCert->chStores; i++)
            {
                FPrintfU(hFile, L"|       |.. pahStores[%02.2d]:        0x%p\r\n", i, pWT->pCert->pahStores[i]);
            }

            FPrintfU(hFile, L"|       |.. dwFlags:                  0x%08.8lX\r\n", pWT->pCert->dwFlags);

            wszTime[0] = NULL;
            wszDate[0] = NULL;

            if (pWT->pCert->psftVerifyAsOf)
            {
                GetStringDateTime(pWT->pCert->psftVerifyAsOf, &wszTime[0], &wszDate[0]);
            }

            FPrintfU(hFile, L"|       |-- psftVerifyAsOf:               %s - %s\r\n", &wszDate[0], &wszTime[0]);
            break;

        default:
            FPrintfU(hFile, L"|       +.. ***Unknown structure type***\r\n");
            break;
    }

    FPrintfU(hFile, L"|.. WndParent:                        0x%p\r\n", pProvData->hWndParent);

    wszGuid[0] = NULL;
    guid2wstr(pProvData->pgActionID, &wszGuid[0]);
    FPrintfU(hFile, L"|.. pgActionID:                       %s\r\n", &wszGuid[0]);
    FPrintfU(hFile, L"|.. hProv:                            0x%p\r\n", pProvData->hProv);
    FPrintfU(hFile, L"|.. dwError:                          0x%08.8lx\r\n", pProvData->dwError);
    FPrintfU(hFile, L"|.. dwRegSecuritySettings:            0x%08.8lx\r\n", pProvData->dwRegSecuritySettings);
    FPrintfU(hFile, L"|.. dwRegPolicySettings:              0x%08.8lx\r\n", pProvData->dwRegPolicySettings);
    FPrintfU(hFile, L"|.. dwEncoding:                       0x%08.8lx\r\n", pProvData->dwEncoding);

    PrintfPFNs(hFile, pProvData);

    FPrintfU(hFile, L"|.. padwTrustStepErrors:\r\n");

    for (i = 0; i < (int)pProvData->cdwTrustStepErrors; i++)
    {
        if (i == (int)(pProvData->cdwTrustStepErrors - 1))
        {
            FPrintfU(hFile, L"|   +.. Step[%02.2d]:                     0x%08.8lx\r\n", i, pProvData->padwTrustStepErrors[i]);
        }
        else
        {
            FPrintfU(hFile, L"|   |.. Step[%02.2d]:                     0x%08.8lx\r\n", i, pProvData->padwTrustStepErrors[i]);
        }
    }

    FPrintfU(hFile, L"|.. pahStores:\r\n");

    for (i = 0; i < (int)pProvData->chStores; i++)
    {
        if (i == (int)(pProvData->chStores - 1))
        {
            FPrintfU(hFile, L"|   +.. Store[%02.2d]:                    0x%lx\r\n", i, pProvData->pahStores[i]);
        }
        else
        {
            FPrintfU(hFile, L"|   |.. Store[%02.2d]:                    0x%lx\r\n", i, pProvData->pahStores[i]);
        }
    }

    FPrintfU(hFile, L"|.. hMsg:                             0x%p\r\n", pProvData->hMsg);

    if (pProvData->dwSubjectChoice == CPD_CHOICE_SIP)
    {
        wszGuid[0] = NULL;
        guid2wstr(&pProvData->pPDSip->gSubject, &wszGuid[0]);
        FPrintfU(hFile, L"|.. pPDSip:\r\n");
        FPrintfU(hFile, L"|   |.. gSubject:                     %s\r\n", &wszGuid[0]);

        FPrintfU(hFile, L"|   |.. pSip:                         0x%p\r\n", pProvData->pPDSip->pSip);
        FPrintfU(hFile, L"|   |.. pCATSip:                      0x%p\r\n", pProvData->pPDSip->pCATSip);
        // TBDTBD: break it out!
        FPrintfU(hFile, L"|   |.. psSipSubjectInfo:             0x%p\r\n", pProvData->pPDSip->psSipSubjectInfo);
        // TBDTBD: break it out!
        FPrintfU(hFile, L"|   |.. psSipCATSubjectInfo:          0x%p\r\n", pProvData->pPDSip->psSipCATSubjectInfo);
        // TBDTBD: break it out!
        FPrintfU(hFile, L"|   +.. psIndirectData:               0x%p\r\n", pProvData->pPDSip->psIndirectData);
    }

    FPrintfU(hFile, L"|.. csSigners:                        %lu\r\n", pProvData->csSigners);

    CRYPT_PROVIDER_SGNR *pSgnr;
    CRYPT_PROVIDER_SGNR *pCounterSgnr;

    for (i = 0; i < (int)pProvData->csSigners; i++)
    {
        pSgnr = WTHelperGetProvSignerFromChain(pProvData, i, FALSE, 0);

        PrintfSignerStruct(hFile, pSgnr, i, FALSE, 0);

        if (pSgnr->csCounterSigners > 0)
        {
            for (int i2 = 0; i2 < (int)pSgnr->csCounterSigners; i2++)
            {
                pCounterSgnr = WTHelperGetProvSignerFromChain(pProvData, i, TRUE, i2);
                PrintfSignerStruct(hFile, pCounterSgnr, i, TRUE, i2);
            }
        }
    }

    FPrintfU(hFile, L"|.. pszUsageOID:                      %p\r\n", pProvData->pszUsageOID);
    FPrintfU(hFile, L"|.. fRecallWithState:                 %s\r\n", (pProvData->fRecallWithState) ? "TRUE" : "FALSE");

    GetStringDateTime(&pProvData->sftSystemTime, &wszTime[0], &wszDate[0]);
    FPrintfU(hFile, L"|.. sftSystemTime:                    %s - %s\r\n", &wszDate[0], &wszTime[0]);


    FPrintfU(hFile, L"+======================================================\r\n");

    CloseHandle(hFile);

    return(S_OK);
}

void PrintfPFNs(HANDLE hFile, CRYPT_PROVIDER_DATA *pPD)
{
    FPrintfU(hFile, L"|.. psPfns:\r\n");

    if (!(pPD->psPfns) ||
        !(WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(CRYPT_PROVIDER_FUNCTIONS, pPD->psPfns->cbStruct, pfnTestFinalPolicy)))
    {
        FPrintfU(hFile, L"|   +.. *** invalid parameter ***\r\n");
        return;
    }
    FPrintfU(hFile, L"|   |.. cbStruct:                     %lu\r\n", pPD->psPfns->cbStruct);
    FPrintfU(hFile, L"|   |.. pfnAlloc:                     0x%p\r\n", pPD->psPfns->pfnAlloc);
    FPrintfU(hFile, L"|   |.. pfnFree:                      0x%p\r\n", pPD->psPfns->pfnFree);
    FPrintfU(hFile, L"|   |.. pfnAddStore2Chain:            0x%p\r\n", pPD->psPfns->pfnAddStore2Chain);
    FPrintfU(hFile, L"|   |.. pfnAddSgnr2Chain:             0x%p\r\n", pPD->psPfns->pfnAddSgnr2Chain);
    FPrintfU(hFile, L"|   |.. pfnAddCert2Chain:             0x%p\r\n", pPD->psPfns->pfnAddCert2Chain);
    FPrintfU(hFile, L"|   |.. pfnAddPrivData2Chain:         0x%p\r\n", pPD->psPfns->pfnAddPrivData2Chain);
    FPrintfU(hFile, L"|   |.. pfnInitialize:                0x%p\r\n", pPD->psPfns->pfnInitialize);
    FPrintfU(hFile, L"|   |.. pfnObjectTrust:               0x%p\r\n", pPD->psPfns->pfnObjectTrust);
    FPrintfU(hFile, L"|   |.. pfnSignatureTrust:            0x%p\r\n", pPD->psPfns->pfnSignatureTrust);
    FPrintfU(hFile, L"|   |.. pfnCertificateTrust:          0x%p\r\n", pPD->psPfns->pfnCertificateTrust);
    FPrintfU(hFile, L"|   |.. pfnFinalPolicy:               0x%p\r\n", pPD->psPfns->pfnFinalPolicy);
    FPrintfU(hFile, L"|   |.. pfnCertCheckPolicy:           0x%p\r\n", pPD->psPfns->pfnCertCheckPolicy);
    FPrintfU(hFile, L"|   |.. pfnTestFinalPolicy:           0x%p\r\n", pPD->psPfns->pfnTestFinalPolicy);

    if (WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(CRYPT_PROVIDER_FUNCTIONS, pPD->psPfns->cbStruct, pfnCleanupPolicy))
    {
        FPrintfU(hFile, L"|   |.. pfnCleanupPolicy:             0x%p\r\n", pPD->psPfns->pfnCleanupPolicy);
    }

    FPrintfU(hFile, L"|   +.. psUIpfns:\r\n");
    if (!(pPD->psPfns->psUIpfns) ||
        !(WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(CRYPT_PROVUI_FUNCS, pPD->psPfns->psUIpfns->cbStruct, pfnOnAdvancedClickDefault)))
    {
        FPrintfU(hFile, L"|       +.. *** invalid parameter ***\r\n");
        return;
    }

    FPrintfU(hFile, L"|       |.. cbStruct:                 %lu\r\n", pPD->psPfns->psUIpfns->cbStruct);
    FPrintfU(hFile, L"|       |.. psUIData:\r\n");

    if (!(pPD->psPfns->psUIpfns->psUIData) ||
        !(WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(CRYPT_PROVUI_DATA, pPD->psPfns->psUIpfns->psUIData->cbStruct, pCopyActionTextNotSigned)))
    {
        FPrintfU(hFile, L"|       |   +.. *** invalid parameter ***\r\n");
    }
    else
    {
        FPrintfU(hFile, L"|       |   |.. cbStruct:             %lu\r\n", pPD->psPfns->psUIpfns->psUIData->cbStruct);
        FPrintfU(hFile, L"|       |   |.. dwFinalError:         0x%08.8lx\r\n", pPD->psPfns->psUIpfns->psUIData->dwFinalError);
        FPrintfU(hFile, L"|       |   |.. pYesButtonText:       %s\r\n", pPD->psPfns->psUIpfns->psUIData->pYesButtonText);
        FPrintfU(hFile, L"|       |   |.. pNoButtonText:        %s\r\n", pPD->psPfns->psUIpfns->psUIData->pNoButtonText);
        FPrintfU(hFile, L"|       |   |.. pMoreInfoButtonText:  %s\r\n", pPD->psPfns->psUIpfns->psUIData->pMoreInfoButtonText);
        FPrintfU(hFile, L"|       |   |.. pAdvancedLinkText:    %s\r\n", pPD->psPfns->psUIpfns->psUIData->pAdvancedLinkText);
        FPrintfU(hFile, L"|       |   |.. pCopyActionText:      %s\r\n", pPD->psPfns->psUIpfns->psUIData->pCopyActionText);
        FPrintfU(hFile, L"|       |   |.. pCopyActionTextNoTS:  %s\r\n", pPD->psPfns->psUIpfns->psUIData->pCopyActionTextNoTS);
        FPrintfU(hFile, L"|       |   |.. pCopyActionTextNotSigned:  %s\r\n", pPD->psPfns->psUIpfns->psUIData->pCopyActionTextNotSigned);
    }

    FPrintfU(hFile, L"|       |.. pfnOnMoreInfoClick:       0x%p\r\n", pPD->psPfns->psUIpfns->pfnOnMoreInfoClick);
    FPrintfU(hFile, L"|       |.. pfnOnMoreInfoClickDefault:0x%p\r\n", pPD->psPfns->psUIpfns->pfnOnMoreInfoClickDefault);
    FPrintfU(hFile, L"|       |.. pfnOnAdvancedClick:       0x%p\r\n", pPD->psPfns->psUIpfns->pfnOnAdvancedClick);
    FPrintfU(hFile, L"|       +.. pfnOnAdvancedClickDefault:0x%p\r\n", pPD->psPfns->psUIpfns->pfnOnAdvancedClickDefault);
}

void PrintfSignerStruct(HANDLE hFile, CRYPT_PROVIDER_SGNR *pS, int idxSigner, BOOL fCounter, int idxCounter)
{
    if (!(fCounter))
    {
        FPrintfU(hFile, L"|.. pasSigners[%d]:\r\n", idxSigner);
    }
    else
    {
        FPrintfU(hFile, L"|.. pasSigners[%d] - CounterSigner[%d]:\r\n", idxSigner, idxCounter);
    }

    FPrintfU(hFile, L"|   |.. cbStruct:                     %lu\r\n", pS->cbStruct);

    WCHAR           wszTime[64];
    WCHAR           wszDate[128];



    GetStringDateTime(&pS->sftVerifyAsOf, &wszTime[0], &wszDate[0]);

    FPrintfU(hFile, L"|   |.. sftVerifyAsOf:                %s - %s\r\n", &wszDate[0], &wszTime[0]);
    FPrintfU(hFile, L"|   |.. dwSignerType:                 0x%08.8lX\r\n", pS->dwSignerType);
    FPrintfU(hFile, L"|   |.. csCertChain:                  %lu\r\n", pS->csCertChain);

    CRYPT_PROVIDER_CERT *pCert;

    for (int i = 0; i < (int)pS->csCertChain; i++)
    {
        pCert = WTHelperGetProvCertFromChain(pS, i);

        PrintfCertStruct(hFile, pS->csCertChain, pCert, i);
    }

    FPrintfU(hFile, L"|   |.. psSigner:                     0x%p\r\n", pS->psSigner);
    FPrintfU(hFile, L"|   |.. dwError:                      0x%08.8lx\r\n", pS->dwError);
    FPrintfU(hFile, L"|   +.. csCounterSigners:             %ld\r\n", pS->csCounterSigners);
}

void PrintfCertStruct(HANDLE hFile, int cCert, CRYPT_PROVIDER_CERT *pC, int idxCert)
{
    WCHAR           wszTime[64];
    WCHAR           wszDate[128];

    if (idxCert < (cCert - 1))
    {
        FPrintfU(hFile, L"|   |   |.. casCertChain[%d]:\r\n", idxCert);
    }
    else
    {
        FPrintfU(hFile, L"|   |   +.. casCertChain[%d]:\r\n", idxCert);
    }
    FPrintfU(hFile, L"|   |   |   |.. cbStruct:             %ld\r\n", pC->cbStruct);
    FPrintfU(hFile, L"|   |   |   |.. pCert:                0x%p\r\n", pC->pCert);
    FPrintfU(hFile, L"|   |   |   |   |.. dwCertEncoding:   0x%08.8lx\r\n", pC->pCert->dwCertEncodingType);
    FPrintfU(hFile, L"|   |   |   |   |.. pCertInfo:\r\n");
    FPrintfU(hFile, L"|   |   |   |   |   |.. Issuer:       %s\r\n", GetNameFromBlob(&pC->pCert->pCertInfo->Issuer));

    GetStringDateTime(&pC->pCert->pCertInfo->NotBefore, &wszTime[0], &wszDate[0]);
    FPrintfU(hFile, L"|   |   |   |   |   |.. NotBefore:    %s - %s\r\n", &wszDate[0], &wszTime[0]);

    GetStringDateTime(&pC->pCert->pCertInfo->NotAfter, &wszTime[0], &wszDate[0]);
    FPrintfU(hFile, L"|   |   |   |   |   |.. NotAfter:     %s - %s\r\n", &wszDate[0], &wszTime[0]);
    FPrintfU(hFile, L"|   |   |   |   |   +.. Subject:      %s\r\n", GetNameFromBlob(&pC->pCert->pCertInfo->Subject));
    FPrintfU(hFile, L"|   |   |   |   +.. hCertStore:       0x%p\r\n", pC->pCert->hCertStore);

    FPrintfU(hFile, L"|   |   |   |.. fCommercial:          %s\r\n", (pC->fCommercial) ? L"True" : L"False");
    FPrintfU(hFile, L"|   |   |   |.. fTrustedRoot:         %s\r\n", (pC->fTrustedRoot) ? L"True" : L"False");
    FPrintfU(hFile, L"|   |   |   |.. fSelfSigned:          %s\r\n", (pC->fSelfSigned) ? L"True" : L"False");
    FPrintfU(hFile, L"|   |   |   |.. fTestCert:            %s\r\n", (pC->fTestCert) ? L"True" : L"False");
    FPrintfU(hFile, L"|   |   |   |.. dwRevokedReason:      0x%08.8lx\r\n", pC->dwRevokedReason);
    FPrintfU(hFile, L"|   |   |   |.. dwConfidence:         0x%08.8lx\r\n", pC->dwConfidence);
    FPrintfU(hFile, L"|   |   |   |.. pTrustListContext:    0x%p\r\n", pC->pTrustListContext);

    if (idxCert == (cCert - 1))
    {
        FPrintfU(hFile, L"|   |   +-- +.. dwError:              0x%08.8lx\r\n", pC->dwError);
    }
    else
    {
        FPrintfU(hFile, L"|   |   |   +.. dwError:              0x%08.8lx\r\n", pC->dwError);
    }
}

void _cdecl FPrintfU(HANDLE hFile, WCHAR *pwszFmt, ...)
{
    va_list     vaArgs;
    WCHAR       wsz[2048];
    char        sz[2048];
    DWORD       cbWritten;
    DWORD       cbConv;

    va_start(vaArgs, pwszFmt);

    vswprintf(&wsz[0], pwszFmt, vaArgs);

    va_end(vaArgs);

    cbConv = 2048;

    cbConv = WideCharToMultiByte(0, 0,
                                &wsz[0], wcslen(&wsz[0]) + 1,
                                &sz[0], cbConv, NULL, NULL);

    sz[cbConv] = NULL;

    cbWritten = 0;

    WriteFile(hFile, &sz[0], cbConv, &cbWritten, NULL);
}


void GetStringDateTime(FILETIME *pFTime, WCHAR *pwszRetTime, WCHAR *pwszRetDate)
{
    SYSTEMTIME      sSysTime;
    char            szTime[128];
    char            szDate[128];

    memset(&sSysTime, 0x00, sizeof(SYSTEMTIME));
    FileTimeToSystemTime(pFTime, &sSysTime);

    szTime[0] = 0;
    GetTimeFormat(LOCALE_USER_DEFAULT, TIME_FORCE24HOURFORMAT, &sSysTime, NULL, &szTime[0], 64);
    MultiByteToWideChar(CP_ACP, 0, (const char *)&szTime[0], -1, pwszRetTime, 64);

    szDate[0] = 0;
    GetDateFormat(LOCALE_USER_DEFAULT, 0, &sSysTime, TEXT("dd'-'MMM'-'yyyy"), &szDate[0], 128);
    MultiByteToWideChar(CP_ACP, 0, (const char *)&szDate[0], -1, pwszRetDate, 128);

}

WCHAR *GetNameFromBlob(CERT_NAME_BLOB *psNameBlob)
{
    static WCHAR    wsz[256];
    PCERT_NAME_INFO pNameInfo;
    PCERT_RDN_ATTR  pRDNAttr;
    DWORD           cbInfo;

    cbInfo      = 0;
    wsz[0]      = NULL;


    CryptDecodeObject(X509_ASN_ENCODING, X509_NAME, psNameBlob->pbData, psNameBlob->cbData,
                        0, NULL, &cbInfo);
    if (cbInfo > 0)
    {
        if (pNameInfo = (PCERT_NAME_INFO)new BYTE[cbInfo])
        {
            if (CryptDecodeObject(X509_ASN_ENCODING, X509_NAME, psNameBlob->pbData, psNameBlob->cbData,
                                  0, pNameInfo, &cbInfo))
            {
                if (pRDNAttr = CertFindRDNAttr(szOID_COMMON_NAME, pNameInfo))
                {
                    CertRDNValueToStrW(pRDNAttr->dwValueType, &pRDNAttr->Value, wsz, 256);;
                }
            }

            delete pNameInfo;
        }
    }

    return(&wsz[0]);
}
