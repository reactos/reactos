//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       sigprov.cpp
//
//  Contents:   Microsoft Internet Security Authenticode Policy Provider
//
//  Functions:  SoftpubLoadSignature
//
//              *** local functions ***
//              _ExtractSigner
//              _ExtractCounterSigners
//              _HandleCertChoice
//              _HandleSignerChoice
//              _FindCertificate
//              _FindCounterSignersCert
//              _IsValidTimeStampCert
//
//  History:    05-Jun-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"

BOOL _ExtractSigner(HCRYPTMSG hMsg, CRYPT_PROVIDER_DATA *pProvData,
                    int idxSigner);
BOOL _ExtractCounterSigners(CRYPT_PROVIDER_DATA *pProvData, DWORD idxSigner);
HRESULT _HandleCertChoice(CRYPT_PROVIDER_DATA *pProvData);
HRESULT _HandleSignerChoice(CRYPT_PROVIDER_DATA *pProvData);
PCCERT_CONTEXT _FindCertificate(CRYPT_PROVIDER_DATA *pProvData, CERT_INFO *pCert);
PCCERT_CONTEXT _FindCounterSignersCert(CRYPT_PROVIDER_DATA *pProvData, 
                                            CERT_NAME_BLOB *psIssuer,
                                            CRYPT_INTEGER_BLOB *psSerial);
BOOL WINAPI _IsValidTimeStampCert(PCCERT_CONTEXT pCertContext);

#ifdef CMS_PKCS7
BOOL _VerifyMessageSignatureWithChainPubKeyParaInheritance(
    IN CRYPT_PROVIDER_DATA *pProvData,
    IN DWORD dwSignerIndex,
    IN PCCERT_CONTEXT pSigner
    );

BOOL _VerifyCountersignatureWithChainPubKeyParaInheritance(
    IN CRYPT_PROVIDER_DATA *pProvData,
    IN PBYTE pbSignerInfo,
    IN DWORD cbSignerInfo,
    IN PBYTE pbSignerInfoCountersignature,
    IN DWORD cbSignerInfoCountersignature,
    IN PCCERT_CONTEXT pSigner
    );
#endif  // CMS_PKCS7

HRESULT SoftpubLoadSignature(CRYPT_PROVIDER_DATA *pProvData)
{
    if (!(pProvData->padwTrustStepErrors) ||
        (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_WVTINIT] != ERROR_SUCCESS) ||
        (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV] != ERROR_SUCCESS))
    {
        return(S_FALSE);
    }

    switch (pProvData->pWintrustData->dwUnionChoice)
    {
        case WTD_CHOICE_CERT:
                    return(_HandleCertChoice(pProvData));

        case WTD_CHOICE_SIGNER:
                    return(_HandleSignerChoice(pProvData));

        case WTD_CHOICE_FILE:
        case WTD_CHOICE_CATALOG:
        case WTD_CHOICE_BLOB:
                    break;

        default:
                    pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV] = TRUST_E_NOSIGNATURE;
                    return(S_FALSE);
    }

    if (!(pProvData->hMsg))
    {
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV]   = TRUST_E_NOSIGNATURE;
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_MSG_SIGNERCOUNT] = GetLastError();

        return(S_FALSE);
    }

    if ((_ISINSTRUCT(CRYPT_PROVIDER_DATA, pProvData->cbStruct, fRecallWithState)) &&
        (pProvData->fRecallWithState))
    {
        return(S_OK);
    }

    int                 i;
    DWORD               cbSize;
    DWORD               csSigners;
    CRYPT_PROVIDER_SGNR *pSgnr;
    CRYPT_PROVIDER_SGNR sSgnr;
    CRYPT_PROVIDER_CERT *pCert;


    cbSize = sizeof(DWORD);

    // signer count
    if (!(CryptMsgGetParam(pProvData->hMsg,
                           CMSG_SIGNER_COUNT_PARAM,
                           0,
                           &csSigners,
                           &cbSize)))
    {
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV] = TRUST_E_NOSIGNATURE;
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_MSG_SIGNERCOUNT] = GetLastError();

        return(S_FALSE);
    }

    if (csSigners == 0)
    {
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV] = TRUST_E_NOSIGNATURE;

        return(S_FALSE);
    }

    for (i = 0; i < (int)csSigners; i++)
    {
        memset(&sSgnr, 0x00, sizeof(CRYPT_PROVIDER_SGNR));

        sSgnr.cbStruct = sizeof(CRYPT_PROVIDER_SGNR);

        if (!(pProvData->psPfns->pfnAddSgnr2Chain(pProvData, FALSE, i, &sSgnr)))
        {
            pProvData->dwError = GetLastError();
            pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV] = TRUST_E_SYSTEM_ERROR;

            return(S_FALSE);
        }

        pSgnr = WTHelperGetProvSignerFromChain(pProvData, i, FALSE, 0);

        if (_ExtractSigner(pProvData->hMsg, pProvData, i))
        {
            memcpy(&pSgnr->sftVerifyAsOf, &pProvData->sftSystemTime, sizeof(FILETIME));

            _ExtractCounterSigners(pProvData, i);
        }
    }

    //
    //  verify the integrity of the signature(s)
    //
    for (i = 0; i < (int)pProvData->csSigners; i++)
    {
        pSgnr = WTHelperGetProvSignerFromChain(pProvData, i, FALSE, 0);
        pCert = WTHelperGetProvCertFromChain(pSgnr, 0);

        if (pSgnr->csCertChain > 0)
        {
#ifdef CMS_PKCS7
            if(!_VerifyMessageSignatureWithChainPubKeyParaInheritance(
                                pProvData,
                                i,
                                pCert->pCert))
#else
            if (!(CryptMsgControl(pProvData->hMsg, 
                                  0,
                                  CMSG_CTRL_VERIFY_SIGNATURE,
                                  pCert->pCert->pCertInfo)))
#endif  // CMS_PKCS7
            {
                if (pSgnr->dwError == 0)
                {
                    pSgnr->dwError = GetLastError();
                }
                
                pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV]   = TRUST_E_NOSIGNATURE;

                return(S_FALSE);
            }
        }
    }

    return(S_OK);
}


HRESULT _HandleCertChoice(CRYPT_PROVIDER_DATA *pProvData)
{
    if (!(pProvData->pWintrustData->pCert) ||
        !(WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(WINTRUST_CERT_INFO, 
                                          pProvData->pWintrustData->pCert->cbStruct,
                                          pahStores)) ||
        !(pProvData->pWintrustData->pCert->psCertContext))
    {
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV]   = ERROR_INVALID_PARAMETER;
        
        return(S_FALSE);
    }

    //
    //  add the stores passed in by the client
    //
    for (int i = 0; i < (int)pProvData->pWintrustData->pCert->chStores; i++)
    {
        if (!(pProvData->psPfns->pfnAddStore2Chain(pProvData, 
                                                pProvData->pWintrustData->pCert->pahStores[i])))
        {
            pProvData->dwError = GetLastError();
            pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV] = TRUST_E_SYSTEM_ERROR;

            return(S_FALSE);
        }
    }

    //
    //  add a dummy signer
    //
    CRYPT_PROVIDER_SGNR sSgnr;

    memset(&sSgnr, 0x00, sizeof(CRYPT_PROVIDER_SGNR));

    sSgnr.cbStruct = sizeof(CRYPT_PROVIDER_SGNR);

    memcpy(&sSgnr.sftVerifyAsOf, &pProvData->sftSystemTime, sizeof(FILETIME));

    if ((_ISINSTRUCT(WINTRUST_CERT_INFO, pProvData->pWintrustData->pCert->cbStruct, psftVerifyAsOf)) &&
        (pProvData->pWintrustData->pCert->psftVerifyAsOf))
    {
        memcpy(&sSgnr.sftVerifyAsOf, pProvData->pWintrustData->pCert->psftVerifyAsOf, sizeof(FILETIME));
    }

    if (!(pProvData->psPfns->pfnAddSgnr2Chain(pProvData, FALSE, 0, &sSgnr)))
    {
        pProvData->dwError = GetLastError();
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV] = TRUST_E_SYSTEM_ERROR;

        return(S_FALSE);
    }


    //
    //  add the "signer's" cert...
    //
    pProvData->psPfns->pfnAddCert2Chain(pProvData, 0, FALSE, 0, 
                                        pProvData->pWintrustData->pCert->psCertContext);

    return(ERROR_SUCCESS);

}

HRESULT _HandleSignerChoice(CRYPT_PROVIDER_DATA *pProvData)
{

    if (!(pProvData->pWintrustData->pSgnr) ||
        !(WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(WINTRUST_SGNR_INFO, 
                                          pProvData->pWintrustData->pSgnr->cbStruct,
                                          pahStores)) ||
        !(pProvData->pWintrustData->pSgnr->psSignerInfo))
    {
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV]   = ERROR_INVALID_PARAMETER;
        
        return(S_FALSE);
    }

    int     i;

    if (1 < pProvData->pWintrustData->pCert->chStores &&
            0 == pProvData->chStores) 
        WTHelperOpenKnownStores(pProvData);

    //
    //  add the stores passed in by the client
    //
    for (i = 0; i < (int)pProvData->pWintrustData->pCert->chStores; i++)
    {
        if (!(pProvData->psPfns->pfnAddStore2Chain(pProvData, 
                                                pProvData->pWintrustData->pCert->pahStores[i])))
        {
            pProvData->dwError = GetLastError();
            pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV] = TRUST_E_SYSTEM_ERROR;

            return(S_FALSE);
        }
    }

    CRYPT_PROVIDER_SGNR sSgnr;
    CRYPT_PROVIDER_SGNR *pSgnr;

    memset(&sSgnr, 0x00, sizeof(CRYPT_PROVIDER_SGNR));

    sSgnr.cbStruct = sizeof(CRYPT_PROVIDER_SGNR);

    if (!(sSgnr.psSigner = (CMSG_SIGNER_INFO *)pProvData->psPfns->pfnAlloc(sizeof(CMSG_SIGNER_INFO))))
    {
        pProvData->dwError = GetLastError();
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV] = TRUST_E_SYSTEM_ERROR;

        return(S_FALSE);
    }

    memcpy(sSgnr.psSigner, pProvData->pWintrustData->pSgnr->psSignerInfo, 
                sizeof(CMSG_SIGNER_INFO));

    memcpy(&sSgnr.sftVerifyAsOf, &pProvData->sftSystemTime, sizeof(FILETIME));

    if (!(pProvData->psPfns->pfnAddSgnr2Chain(pProvData, FALSE, 0, &sSgnr)))
    {
        pProvData->psPfns->pfnFree(sSgnr.psSigner);

        pProvData->dwError = GetLastError();
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV] = TRUST_E_SYSTEM_ERROR;

        return(S_FALSE);
    }

    if (!(pSgnr = WTHelperGetProvSignerFromChain(pProvData, 0, FALSE, 0)))
    {
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV] = GetLastError();
        return(S_FALSE);
    }

    CERT_INFO       sCert;
    PCCERT_CONTEXT  pCertContext;

    memset(&sCert, 0x00, sizeof(CERT_INFO));

    sCert.Issuer.cbData         = pSgnr->psSigner->Issuer.cbData;
    sCert.Issuer.pbData         = pSgnr->psSigner->Issuer.pbData;

    sCert.SerialNumber.cbData   = pSgnr->psSigner->SerialNumber.cbData;
    sCert.SerialNumber.pbData   = pSgnr->psSigner->SerialNumber.pbData;

    if (!(pCertContext = _FindCertificate(pProvData, &sCert)))
    {
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV] = TRUST_E_NO_SIGNER_CERT;
        return(FALSE);
    }

    pProvData->psPfns->pfnAddCert2Chain(pProvData, 0, FALSE, 0, pCertContext);

    _ExtractCounterSigners(pProvData, 0);

    return(ERROR_SUCCESS);
}

BOOL _ExtractSigner(HCRYPTMSG hMsg, CRYPT_PROVIDER_DATA *pProvData, int idxSigner)
{
    DWORD               cb;
    BYTE                *pb;
    CRYPT_PROVIDER_SGNR *pSgnr;
    PCCERT_CONTEXT      pCertContext;

    pSgnr = WTHelperGetProvSignerFromChain(pProvData, idxSigner, FALSE, 0);

    //
    //  signer info
    //
    cb = 0;

    CryptMsgGetParam(hMsg, CMSG_SIGNER_INFO_PARAM, idxSigner, NULL, &cb);

    if (cb == 0)
    {
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV] = TRUST_E_NOSIGNATURE;
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_MSG_SIGNERINFO] = GetLastError();
        return(FALSE);
    }

    if (!(pSgnr->psSigner = (CMSG_SIGNER_INFO *)pProvData->psPfns->pfnAlloc(cb)))
    {
        pProvData->dwError = GetLastError();
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV] = TRUST_E_SYSTEM_ERROR;
        return(FALSE);
    }

    memset(pSgnr->psSigner, 0x00, cb);

    if (!(CryptMsgGetParam(hMsg, CMSG_SIGNER_INFO_PARAM, idxSigner, pSgnr->psSigner, &cb)))
    {
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV] = TRUST_E_NOSIGNATURE;
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_MSG_SIGNERINFO] = GetLastError();
        return(FALSE);
    }

    //
    //  cert info
    //
    cb = 0;

    CryptMsgGetParam(hMsg, CMSG_SIGNER_CERT_INFO_PARAM, idxSigner, NULL, &cb);

    if (cb == 0)
    {
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV] = TRUST_E_NO_SIGNER_CERT;
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_MSG_SIGNERINFO] = GetLastError();
        return(FALSE);
    }

    if (!(pb = (BYTE *)pProvData->psPfns->pfnAlloc(cb)))
    {
        pProvData->dwError = GetLastError();
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV] = TRUST_E_SYSTEM_ERROR;
        return(FALSE);
    }

    memset(pb, 0x00, cb);

    if (!(CryptMsgGetParam(hMsg, CMSG_SIGNER_CERT_INFO_PARAM, idxSigner, pb, &cb)))
    {
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV] = TRUST_E_NO_SIGNER_CERT;
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_MSG_SIGNERINFO] = GetLastError();

        pProvData->psPfns->pfnFree(pb);

        return(FALSE);
    }

    if (!(pCertContext = _FindCertificate(pProvData, (CERT_INFO *)pb)))
    {
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV] = TRUST_E_NO_SIGNER_CERT;
        pProvData->psPfns->pfnFree(pb);
        return(FALSE);
    }

    pProvData->psPfns->pfnFree(pb);

    pProvData->psPfns->pfnAddCert2Chain(pProvData, idxSigner, FALSE, 0, pCertContext);

    CertFreeCertificateContext(pCertContext);

    return(TRUE);
}

BOOL _ExtractCounterSigners(CRYPT_PROVIDER_DATA *pProvData, DWORD idxSigner)
{
    if ((_ISINSTRUCT(CRYPT_PROVIDER_DATA, pProvData->cbStruct, fRecallWithState)) &&
        (pProvData->fRecallWithState))
    {
        return(TRUE);
    }

    CRYPT_ATTRIBUTE     *pAttr;
    PCCERT_CONTEXT      pCertContext;
    CRYPT_PROVIDER_SGNR *pSgnr;
    CRYPT_PROVIDER_SGNR sCS;
    CRYPT_PROVIDER_SGNR *pCS;
    CRYPT_PROVIDER_CERT *pCert;
    DWORD               cbSize;

    pSgnr = WTHelperGetProvSignerFromChain(pProvData, idxSigner, FALSE, 0);

    //
    //  counter signers are stored in the UN-authenticated attributes of the
    //  signer.
    //
    if ((pAttr = CertFindAttribute(szOID_RSA_counterSign,
                                   pSgnr->psSigner->UnauthAttrs.cAttr,
                                   pSgnr->psSigner->UnauthAttrs.rgAttr)) == NULL)
    {
        //
        //  no counter signature
        //
        return(FALSE);
    }


    memset(&sCS, 0x00, sizeof(CRYPT_PROVIDER_SGNR));
    sCS.cbStruct = sizeof(CRYPT_PROVIDER_SGNR);

    memcpy(&sCS.sftVerifyAsOf, &pProvData->sftSystemTime, sizeof(FILETIME));

    if (!(pProvData->psPfns->pfnAddSgnr2Chain(pProvData, TRUE, idxSigner, &sCS)))
    {
        pProvData->dwError = GetLastError();
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV] = TRUST_E_SYSTEM_ERROR;
        return(FALSE);
    }

    pCS = WTHelperGetProvSignerFromChain(pProvData, idxSigner, TRUE, pSgnr->csCounterSigners - 1);

    // Crack the encoded signer

    if (!(TrustDecode(WVT_MODID_SOFTPUB, (BYTE **)&pCS->psSigner, &cbSize, 1024,
                      pProvData->dwEncoding, PKCS7_SIGNER_INFO, pAttr->rgValue[0].pbData, pAttr->rgValue[0].cbData,
                      CRYPT_DECODE_NOCOPY_FLAG)))
    {
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV] = TRUST_E_COUNTER_SIGNER;
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_MSG_COUNTERSIGINFO] = GetLastError();
        pCS->dwError = GetLastError();
        return(FALSE);
    }

    //
    //  counter signers cert
    //

    if (!(pCertContext = _FindCounterSignersCert(pProvData, 
                                                 &pCS->psSigner->Issuer,
                                                 &pCS->psSigner->SerialNumber)))
    {
        pCS->dwError = TRUST_E_NO_SIGNER_CERT;
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV] = TRUST_E_COUNTER_SIGNER;
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_MSG_COUNTERSIGCERT] = GetLastError();
        return(FALSE);
    }


    pProvData->psPfns->pfnAddCert2Chain(pProvData, idxSigner, TRUE, 
                                      pProvData->pasSigners[idxSigner].csCounterSigners - 1, 
                                      pCertContext);

    CertFreeCertificateContext(pCertContext);

    pCert           = WTHelperGetProvCertFromChain(pCS, pCS->csCertChain - 1);
    pCertContext    = pCert->pCert;

    {
        //
        // Verify the counter's signature
        //

        BYTE *pbEncodedSigner = NULL;
        DWORD cbEncodedSigner;
        BOOL fResult;

        // First need to re-encode the Signer.
        fResult = CryptEncodeObjectEx(
            PKCS_7_ASN_ENCODING | CRYPT_ASN_ENCODING,
            PKCS7_SIGNER_INFO,
            pSgnr->psSigner,
            CRYPT_ENCODE_ALLOC_FLAG,
            NULL,                       // pEncodePara
            (void *) &pbEncodedSigner,
            &cbEncodedSigner
            );

        if (fResult)
#ifdef CMS_PKCS7
            fResult = _VerifyCountersignatureWithChainPubKeyParaInheritance(
                                pProvData,
                                pbEncodedSigner,
                                cbEncodedSigner,
                                pAttr->rgValue[0].pbData,
                                pAttr->rgValue[0].cbData,
                                pCertContext
                                );
#else
            fResult = CryptMsgVerifyCountersignatureEncoded(
                                NULL,   //HCRYPTPROV
                                PKCS_7_ASN_ENCODING | CRYPT_ASN_ENCODING,
                                pbEncodedSigner,
                                cbEncodedSigner,
                                pAttr->rgValue[0].pbData,
                                pAttr->rgValue[0].cbData,
                                pCertContext->pCertInfo
                                );
#endif  // CMS_PKCS7
        if (pbEncodedSigner)
            LocalFree((HLOCAL) pbEncodedSigner);

        if (!fResult)
        {
            pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV] = TRUST_E_COUNTER_SIGNER;
            pProvData->padwTrustStepErrors[TRUSTERROR_STEP_MSG_COUNTERSIGINFO] = GetLastError();
            pCS->dwError = GetLastError();
            return(FALSE);
        }
    }

    //
    // see if the counter signer is a TimeStamp.
    //
    if (!(_IsValidTimeStampCert(pCertContext)))
    {
        return(TRUE);
    }

    // get the time
    if (!(pAttr = CertFindAttribute(szOID_RSA_signingTime, 
                                   pCS->psSigner->AuthAttrs.cAttr,
                                   pCS->psSigner->AuthAttrs.rgAttr)))
    {
        //
        //  not a time stamp...
        //
        return(TRUE);
    }

    //
    // the time stamp counter signature must have 1 value!
    //
    if (pAttr->cValue <= 0) 
    {
        //
        //  not a time stamp...
        //
        return(TRUE);
    }

    //
    // Crack the time stamp and get the file time.
    //
    FILETIME        ftHold;

    cbSize = sizeof(FILETIME);

    CryptDecodeObject(pProvData->dwEncoding, 
                      PKCS_UTC_TIME,
                      pAttr->rgValue[0].pbData, 
                      pAttr->rgValue[0].cbData,
                      0, 
                      &ftHold, 
                      &cbSize);

    if (cbSize == 0)
    {
        pCS->dwError = GetLastError();
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV] = TRUST_E_TIME_STAMP;
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_MSG_COUNTERSIGINFO] = GetLastError();
        return(FALSE);
    }
        

    //
    //  set the signer's verify date to the date in the time stamp!
    //
    memcpy(&pSgnr->sftVerifyAsOf, &ftHold, sizeof(FILETIME));

    // On 12-January-99 Keithv gave me the orders to change the
    // countersigning to use the current time
    //
    // On 25-January-99 backed out the above change
    memcpy(&pCS->sftVerifyAsOf, &ftHold, sizeof(FILETIME));

    pCS->dwSignerType |= SGNR_TYPE_TIMESTAMP;

    return(TRUE);
}

PCCERT_CONTEXT _FindCertificate(CRYPT_PROVIDER_DATA *pProvData, CERT_INFO *pCert)
{
    PCCERT_CONTEXT pCertContext;
    DWORD i;

    if (!(pCert))
    {
        return(NULL);
    }

    for (i = 0; i < pProvData->chStores; i++)
    {
        if (pCertContext = CertGetSubjectCertificateFromStore(pProvData->pahStores[i],
                                                                            pProvData->dwEncoding,
                                                                            pCert))
        {
            return(pCertContext);
        }
    }

    if (1 >= pProvData->chStores) {
        DWORD cOrig = pProvData->chStores;

        WTHelperOpenKnownStores(pProvData);
        for (i = cOrig; i < pProvData->chStores; i++) {
            if (pCertContext = CertGetSubjectCertificateFromStore(
                    pProvData->pahStores[i],
                    pProvData->dwEncoding,
                    pCert))
                return (pCertContext);
        }
    }

    return(NULL);
}

PCCERT_CONTEXT _FindCounterSignersCert(CRYPT_PROVIDER_DATA *pProvData, 
                                            CERT_NAME_BLOB *psIssuer,
                                            CRYPT_INTEGER_BLOB *psSerial)
{
    CERT_INFO       sCert;
    PCCERT_CONTEXT  pCertContext;
    DWORD           i;

    memset(&sCert, 0x00, sizeof(CERT_INFO));

    sCert.Issuer        = *psIssuer;
    sCert.SerialNumber  = *psSerial;

    for (i = 0; i < pProvData->chStores; i++)
    {
        if (pCertContext = CertGetSubjectCertificateFromStore(pProvData->pahStores[i],
                                                                            pProvData->dwEncoding,
                                                                            &sCert))
        {
            return(pCertContext);
        }
    }

    if (1 >= pProvData->chStores) {
        DWORD cOrig = pProvData->chStores;

        WTHelperOpenKnownStores(pProvData);
        for (i = cOrig; i < pProvData->chStores; i++) {
            if (pCertContext = CertGetSubjectCertificateFromStore(
                    pProvData->pahStores[i],
                    pProvData->dwEncoding,
                    &sCert))
                return (pCertContext);
        }
    }

    return(NULL);
}

#define SH1_HASH_LENGTH     20

BOOL WINAPI _IsValidTimeStampCert(PCCERT_CONTEXT pCertContext)
{
    DWORD               cbSize;
    PCERT_ENHKEY_USAGE  pCertEKU;
    BYTE                baSignersThumbPrint[SH1_HASH_LENGTH];
    static BYTE         baVerisignTimeStampThumbPrint[SH1_HASH_LENGTH] =
                            { 0x38, 0x73, 0xB6, 0x99, 0xF3, 0x5B, 0x9C, 0xCC, 0x36, 0x62,
                              0xB6, 0x48, 0x3A, 0x96, 0xBD, 0x6E, 0xEC, 0x97, 0xCF, 0xB7 };

    cbSize = SH1_HASH_LENGTH;

    if (!(CertGetCertificateContextProperty(pCertContext, CERT_SHA1_HASH_PROP_ID, 
                                          &baSignersThumbPrint[0], &cbSize)))
    {
        return(FALSE);
    }

    //
    //  1st, check to see if it's Verisign's first timestamp certificate.  This one did NOT
    //  have the enhanced key usage in it.
    //
    if (memcmp(&baSignersThumbPrint[0], &baVerisignTimeStampThumbPrint[0], SH1_HASH_LENGTH) == 0)
    {
        return(TRUE);
    }

    //
    //  see if the certificate has the proper enhanced key usage OID
    //
    cbSize = 0;

    CertGetEnhancedKeyUsage(pCertContext, 
                            CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG,
                            NULL,
                            &cbSize);

    if (cbSize == 0)
    {
        return(FALSE);
    }
                      
    if (!(pCertEKU = (PCERT_ENHKEY_USAGE)new BYTE[cbSize]))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    if (!(CertGetEnhancedKeyUsage(pCertContext,
                                  CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG,
                                  pCertEKU,
                                  &cbSize)))
    {
        delete pCertEKU;
        return(FALSE);
    }

    for (int i = 0; i < (int)pCertEKU->cUsageIdentifier; i++)
    {
        if (strcmp(pCertEKU->rgpszUsageIdentifier[i], szOID_KP_TIME_STAMP_SIGNING) == 0)
        {
            delete pCertEKU;
            return(TRUE);
        }

        if (strcmp(pCertEKU->rgpszUsageIdentifier[i], szOID_PKIX_KP_TIMESTAMP_SIGNING) == 0)
        {
            delete pCertEKU;
            return(TRUE);
        }
    }

    delete pCertEKU;

    return(FALSE);
}

#ifdef CMS_PKCS7

void _BuildChainForPubKeyParaInheritance(
    IN CRYPT_PROVIDER_DATA *pProvData,
    IN PCCERT_CONTEXT pSigner
    )
{
    PCCERT_CHAIN_CONTEXT pChainContext;
    CERT_CHAIN_PARA ChainPara;
    HCERTSTORE hAdditionalStore;

    if (0 == pProvData->chStores)
        hAdditionalStore = NULL;
    else if (1 < pProvData->chStores) {
        if (hAdditionalStore = CertOpenStore(
                CERT_STORE_PROV_COLLECTION,
                0,                      // dwEncodingType
                0,                      // hCryptProv
                0,                      // dwFlags
                NULL                    // pvPara
                )) {
            DWORD i;
            for (i = 0; i < pProvData->chStores; i++)
                CertAddStoreToCollection(
                    hAdditionalStore,
                    pProvData->pahStores[i],
                    CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG,
                    0                       // dwPriority
                    );
        }
    } else
        hAdditionalStore = CertDuplicateStore(pProvData->pahStores[0]);

    // Build a chain. Hopefully, the signer inherit's its public key
    // parameters from up the chain

    memset(&ChainPara, 0, sizeof(ChainPara));
    ChainPara.cbSize = sizeof(ChainPara);
    if (CertGetCertificateChain(
            NULL,                   // hChainEngine
            pSigner,
            NULL,                   // pTime
            hAdditionalStore,
            &ChainPara,
            CERT_CHAIN_CACHE_ONLY_URL_RETRIEVAL,
            NULL,                   // pvReserved
            &pChainContext
            ))
        CertFreeCertificateChain(pChainContext);
    if (hAdditionalStore)
        CertCloseStore(hAdditionalStore, 0);
}

//+-------------------------------------------------------------------------
//  If the verify signature fails with CRYPT_E_MISSING_PUBKEY_PARA,
//  build a certificate chain. Retry. Hopefully, the signer's
//  CERT_PUBKEY_ALG_PARA_PROP_ID property gets set while building the chain.
//--------------------------------------------------------------------------
BOOL _VerifyMessageSignatureWithChainPubKeyParaInheritance(
    IN CRYPT_PROVIDER_DATA *pProvData,
    IN DWORD dwSignerIndex,
    IN PCCERT_CONTEXT pSigner
    )
{
    CMSG_CTRL_VERIFY_SIGNATURE_EX_PARA CtrlPara;

    memset(&CtrlPara, 0, sizeof(CtrlPara));
    CtrlPara.cbSize = sizeof(CtrlPara);
    // CtrlPara.hCryptProv =
    CtrlPara.dwSignerIndex = dwSignerIndex;
    CtrlPara.dwSignerType = CMSG_VERIFY_SIGNER_CERT;
    CtrlPara.pvSigner = (void *) pSigner;

    if (CryptMsgControl(
            pProvData->hMsg, 
            0,                              // dwFlags
            CMSG_CTRL_VERIFY_SIGNATURE_EX,
            &CtrlPara
            ))
        return TRUE;
    else if (CRYPT_E_MISSING_PUBKEY_PARA != GetLastError())
        return FALSE;
    else {
        _BuildChainForPubKeyParaInheritance(pProvData, pSigner);

        // Try again. Hopefully the above chain building updated the signer's
        // context property with the missing public key parameters
        return CryptMsgControl(
            pProvData->hMsg, 
            0,                              // dwFlags
            CMSG_CTRL_VERIFY_SIGNATURE_EX,
            &CtrlPara
            );
    }
}

//+-------------------------------------------------------------------------
//  If the verify counter signature fails with CRYPT_E_MISSING_PUBKEY_PARA,
//  build a certificate chain. Retry. Hopefully, the signer's
//  CERT_PUBKEY_ALG_PARA_PROP_ID property gets set while building the chain.
//--------------------------------------------------------------------------
BOOL _VerifyCountersignatureWithChainPubKeyParaInheritance(
    IN CRYPT_PROVIDER_DATA *pProvData,
    IN PBYTE pbSignerInfo,
    IN DWORD cbSignerInfo,
    IN PBYTE pbSignerInfoCountersignature,
    IN DWORD cbSignerInfoCountersignature,
    IN PCCERT_CONTEXT pSigner
    )
{
    if (CryptMsgVerifyCountersignatureEncodedEx(
            0,                                      // hCryptProv
            PKCS_7_ASN_ENCODING | CRYPT_ASN_ENCODING,
            pbSignerInfo,
            cbSignerInfo,
            pbSignerInfoCountersignature,
            cbSignerInfoCountersignature,
            CMSG_VERIFY_SIGNER_CERT,
            (void *) pSigner,
            0,                                      // dwFlags
            NULL                                    // pvReserved
            ))
        return TRUE;
    else if (CRYPT_E_MISSING_PUBKEY_PARA != GetLastError())
        return FALSE;
    else {
        _BuildChainForPubKeyParaInheritance(pProvData, pSigner);

        // Try again. Hopefully the above chain building updated the signer's
        // context property with the missing public key parameters
        return CryptMsgVerifyCountersignatureEncodedEx(
                0,                                      // hCryptProv
                PKCS_7_ASN_ENCODING | CRYPT_ASN_ENCODING,
                pbSignerInfo,
                cbSignerInfo,
                pbSignerInfoCountersignature,
                cbSignerInfoCountersignature,
                CMSG_VERIFY_SIGNER_CERT,
                (void *) pSigner,
                0,                                      // dwFlags
                NULL                                    // pvReserved
                );
    }
}

#endif  // CMS_PKCS7
