//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       checks.cpp
//
//  Contents:   Microsoft Internet Security Authenticode Policy Provider
//
//  Functions:  checkCertificateChain
//              checkGetErrorBasedOnStepErrors
//              checkSetCommercial
//              checkBasicConstraints
//              checkBasicConstraints2
//              checkCertPurpose
//              checkCertAnyUnknownCriticalExtensions
//              checkMeetsMinimalFinancialCriteria
//              checkRevocation
//
//  History:    06-Jun-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"


#undef  _PBERKMAN_NONCOMMERCIAL_HISEC_CHECK // per Anthony Short - "don't check this yet!" - 01-May-1997
#undef  _PBERKMAN_FINANCIALCRIT_CHECK       // per KeithV - "Don't check it!" 26-Jun-1997        


DWORD checkGetErrorBasedOnStepErrors(CRYPT_PROVIDER_DATA *pProvData)
{
    //
    //  initial allocation of the step errors?
    //
    if (!(pProvData->padwTrustStepErrors))
    {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    // problem with file
    if ((pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FILEIO] != 0) ||
        (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_CATALOGFILE] != 0))
    {
        return(CRYPT_E_FILE_ERROR);
    }

    //
    //  did we fail in one of the last steps?
    //
    if (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV] != 0)
    {
        return(pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV]);
    }

    if (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV] != 0)
    {
        return(pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV]);
    }

    if (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV] != 0)
    {
        return(pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV]);
    }

    if (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV] != 0)
    {
        return(pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV]);
    }

    if (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTCHKPROV] != 0)
    {
        return(pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTCHKPROV]);
    }

    if (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_POLICYPROV] != 0)
    {
        return(pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_POLICYPROV]);
    }

    return(ERROR_SUCCESS);
}



BOOL checkIsTrustedRoot(CRYPT_PROVIDER_CERT *pRoot)
{
    //
    //  this is actually set in the Cert Check provider (softpub:chkcert.cpp)
    //
    if (pRoot->fTrustedRoot)
    {
        return(TRUE);
    }

    return(FALSE);
}

BOOL checkCertificateChain(CRYPT_PROVIDER_DATA *pProvData, CRYPT_PROVIDER_SGNR *pProvSgnr, DWORD *pdwError)
{
    CRYPT_PROVIDER_CERT *pCert;
    BOOL                fCommercial;
    BOOL                fInCTLChain;

    fInCTLChain = FALSE;

    *pdwError = ERROR_SUCCESS;

    if (!(checkSetCommercial(pProvData, pProvSgnr, &fCommercial)))
    {
        *pdwError       = CERT_E_PURPOSE;
        return(FALSE);
    }

    for (int i = 0; i < (int)pProvSgnr->csCertChain; i++)
    {
        pCert = WTHelperGetProvCertFromChain(pProvSgnr, i);

        if ((_ISINSTRUCT(CRYPT_PROVIDER_CERT, pCert->cbStruct, fTrustListSignerCert)) &&
            (pCert->fTrustListSignerCert))
        {
            fInCTLChain = TRUE;
        }

        if (!(pProvData->dwRegPolicySettings  & WTPF_IGNOREEXPIRATION))
        {
            if (!(pCert->dwConfidence & CERT_CONFIDENCE_TIME))
            {
                *pdwError       = CERT_E_EXPIRED;
                pCert->dwError  = *pdwError;
                return(FALSE);
            }
        }

        //
        //  check purpose
        //
        if (!(checkCertPurpose(pProvData, pCert, fCommercial)))
        {
            *pdwError       = CERT_E_PURPOSE;
            pCert->dwError  = *pdwError;
            return(FALSE);
        }

        //
        //  check eku for code signing
        //
        char        *pszUsageOID    = szOID_PKIX_KP_CODE_SIGNING;
        char        *pszCTLSigning  = szOID_KP_CTL_USAGE_SIGNING;

        if ((_ISINSTRUCT(CRYPT_PROVIDER_DATA, pProvData->cbStruct, pszUsageOID)) &&
            (pProvData->pszUsageOID))
        {
                pszUsageOID = pProvData->pszUsageOID;
        }

        if (fInCTLChain)
        {
            if ((_ISINSTRUCT(CRYPT_PROVIDER_DATA, pProvData->cbStruct, pszCTLSignerUsageOID)) &&
                (pProvData->pszCTLSignerUsageOID))
            {
                pszCTLSigning = pProvData->pszCTLSignerUsageOID;
            }


            pszUsageOID = pszCTLSigning;
        }

        if (_stricmp(pszUsageOID, "ALL") != 0)
        {
            if (!(WTHelperCheckCertUsage(pCert->pCert, pszUsageOID)))
            {
                *pdwError       = CERT_E_WRONG_USAGE;
                pCert->dwError  = *pdwError;
                return(FALSE);
            }
        }

        //
        //  check any unknown critical extensions
        //
        if (!(checkCertAnyUnknownCriticalExtensions(pProvData, pCert->pCert->pCertInfo)))
        {
            *pdwError       = CERT_E_MALFORMED;
            pCert->dwError  = *pdwError;
            return(FALSE);
        }

        //
        //  check basic constraints.
        //
        if (!(checkBasicConstraints(pProvData, pCert, i, pCert->pCert->pCertInfo)))
        {
            *pdwError       = TRUST_E_BASIC_CONSTRAINTS;
            pCert->dwError  = *pdwError;
            return(FALSE);
        }

        if ((i + 1) < (int)pProvSgnr->csCertChain)
        {
            //
            //  check time nesting...
            //
            if (!(pCert->dwConfidence & CERT_CONFIDENCE_TIMENEST))
            {
                *pdwError       = CERT_E_VALIDITYPERIODNESTING;
                pCert->dwError  = *pdwError;
                return(FALSE);
            }
            
        }
    }

    if (!(pProvData->dwRegPolicySettings & WTPF_IGNOREREVOKATION))
    {
        // root cert is test?
        pCert = WTHelperGetProvCertFromChain(pProvSgnr, pProvSgnr->csCertChain - 1);

        if (pCert)
        {
            if (!(pCert->fTestCert))
            {
                //
                //  not a test root, check signer cert for revocation
                //
                if (!(checkRevocation(pProvData, pProvSgnr, 
                                      fCommercial, pdwError)))
                {
                    return(FALSE);
                }
            }
        }
    }

    return(TRUE);
}

BOOL checkTimeStampCertificateChain(CRYPT_PROVIDER_DATA *pProvData, CRYPT_PROVIDER_SGNR *pProvSgnr, DWORD *pdwError)
{
    CRYPT_PROVIDER_CERT *pCert;

    for (int i = 0; i < (int)pProvSgnr->csCertChain; i++)
    {
        pCert = WTHelperGetProvCertFromChain(pProvSgnr, i);

        if (!(pProvData->dwRegPolicySettings  & WTPF_IGNOREEXPIRATION))
        {
            //
            //  this check was done in the Certificate Provider, however, it may not have passed
            //  because all its looking for is a confidence level and didn't check the end..
            //
            if (CertVerifyTimeValidity(&pProvSgnr->sftVerifyAsOf, pCert->pCert->pCertInfo) != 0)
            {
                *pdwError       = CERT_E_EXPIRED;
                pCert->dwError  = *pdwError;
                return(FALSE);
            }
        }

        if ((i + 1) < (int)pProvSgnr->csCertChain)
        {
            //
            //  check time nesting...
            //
            if (!(pCert->dwConfidence & CERT_CONFIDENCE_TIMENEST))
            {
                *pdwError       = CERT_E_VALIDITYPERIODNESTING;
                pCert->dwError  = *pdwError;
                return(FALSE);
            }
            
        }

        if (i == 0)
        {
            //
            //  check revocation on signer cert ONLY!
            //
            if (!(pProvData->dwRegPolicySettings & WTPF_IGNOREREVOCATIONONTS))
            {
                if (!(checkRevocation(pProvData, pProvSgnr, TRUE, pdwError)))
                {
                    return(FALSE);
                }
            }
        }
    }

    return(TRUE);
}

BOOL checkSetCommercial(CRYPT_PROVIDER_DATA *pProvData, 
                        CRYPT_PROVIDER_SGNR *pSgnr,
                        BOOL *pfCommercial)
{
    BOOL                fIndividual;
    PCRYPT_ATTRIBUTE    pAttr;
    PCRYPT_ATTR_BLOB    pValue;
    PSPC_STATEMENT_TYPE pInfo;
    DWORD               cbInfo;
    DWORD               cKeyPurposeId;
    LPSTR               *ppszKeyPurposeId;
    BOOL                fRet;


    *pfCommercial       = FALSE;

    fIndividual         = FALSE;
    pInfo               = NULL;

    fRet                = TRUE;
    
    if (!(pSgnr->psSigner))
    {
        if (pProvData->pWintrustData->dwUnionChoice == WTD_CHOICE_CERT)
        {
            goto CommonReturn;
        }

        goto NoSigner;
    }

    if (pSgnr->psSigner->AuthAttrs.cAttr == 0)
    {
        goto NoAttributes;
    }

    if (!(pAttr = CertFindAttribute(SPC_STATEMENT_TYPE_OBJID,
                                    pSgnr->psSigner->AuthAttrs.cAttr,
                                    pSgnr->psSigner->AuthAttrs.rgAttr)))
    {
        goto NoStatementType;
    }

    if (pAttr->cValue == 0)
    {
        goto NoStatementType;
    }

    if (!(TrustDecode(WVT_MODID_SOFTPUB, (BYTE **)&pInfo, &cbInfo, 100,
                      pProvData->dwEncoding, SPC_STATEMENT_TYPE_STRUCT,
                      pAttr->rgValue[0].pbData, pAttr->rgValue[0].cbData,
                      CRYPT_DECODE_NOCOPY_FLAG)))
    {
        goto DecodeError;
    }

    cKeyPurposeId       = pInfo->cKeyPurposeId;
    ppszKeyPurposeId    = pInfo->rgpszKeyPurposeId;

    for (; cKeyPurposeId > 0; cKeyPurposeId--, ppszKeyPurposeId++) 
    {
        if (strcmp(*ppszKeyPurposeId, SPC_COMMERCIAL_SP_KEY_PURPOSE_OBJID) == 0)
        {
            *pfCommercial  = TRUE;
        }
        else if (strcmp(*ppszKeyPurposeId, SPC_INDIVIDUAL_SP_KEY_PURPOSE_OBJID) == 0)
        {
            fIndividual = TRUE;
        }
    }
    
    if (!(fIndividual) && !(*pfCommercial))
    {
        goto NoPurpose;
    }

    fRet = TRUE;

CommonReturn:

    if (pInfo)
    {
        TrustFreeDecode(WVT_MODID_SOFTPUB, (BYTE **)&pInfo);
    }

    return(fRet);

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
 
    TRACE_ERROR_EX(DBG_SS, NoSigner);
    TRACE_ERROR_EX(DBG_SS, NoAttributes);    
    TRACE_ERROR_EX(DBG_SS, NoStatementType);
    TRACE_ERROR_EX(DBG_SS, NoPurpose);
    TRACE_ERROR_EX(DBG_SS, DecodeError);
}

BOOL checkCertPurpose(CRYPT_PROVIDER_DATA *pProvData, CRYPT_PROVIDER_CERT *pCert, BOOL fCommercialMsg)
{
    PCERT_EXTENSION                     pExt;
    PCERT_KEY_USAGE_RESTRICTION_INFO    pInfo;
    CERT_INFO                           *pCertInfo;
    DWORD                               cbInfo;
    BOOL                                fRet;

    pCertInfo   = pCert->pCert->pCertInfo;
    pInfo       = NULL;
    fRet        = TRUE;

    if (pCertInfo->cExtension == 0)
    {
        goto CommonReturn;
    }
    
    if (!(pExt = CertFindExtension(szOID_KEY_USAGE_RESTRICTION,
                                   pCertInfo->cExtension,
                                   pCertInfo->rgExtension)))
    {
        goto CommonReturn;
    }
    
    if (!(TrustDecode(WVT_MODID_SOFTPUB, (BYTE **)&pInfo, &cbInfo, 200,
                          pProvData->dwEncoding, X509_KEY_USAGE_RESTRICTION,
                          pExt->Value.pbData, pExt->Value.cbData,
                          CRYPT_DECODE_NOCOPY_FLAG)))
    {
        goto DecodeError;
    }

    if (pInfo->cCertPolicyId) 
    {
        char            **ppszElementId;
        BYTE            bKeyUsage;
        BOOL            fCommercial;
        BOOL            fIndividual;
        DWORD           cElementId;
        DWORD           cPolicyId;
        PCERT_POLICY_ID pPolicyId;

        fCommercial = FALSE;
        fIndividual = FALSE;
        
        cPolicyId = pInfo->cCertPolicyId;
        pPolicyId = pInfo->rgCertPolicyId;

        for ( ; cPolicyId > 0; cPolicyId--, pPolicyId++) 
        {
            cElementId      = pPolicyId->cCertPolicyElementId;
            ppszElementId   = pPolicyId->rgpszCertPolicyElementId;

            for ( ; cElementId > 0; cElementId--, ppszElementId++) 
            {
                if (strcmp(*ppszElementId, SPC_COMMERCIAL_SP_KEY_PURPOSE_OBJID) == 0)
                {
                    fCommercial = TRUE;
                }
                else if (strcmp(*ppszElementId, SPC_INDIVIDUAL_SP_KEY_PURPOSE_OBJID) == 0)
                {
                    fIndividual = TRUE;
                }
            }
        }

        if (!(fCommercial) && !(fIndividual))
        {
            goto KeyPurposeError;
        }

        if ((fCommercial) && (fIndividual))
        {
           ; // place holder -- it supports both (??? -- test cert only)
        }
        else if ((((fCommercialMsg) && !(fCommercial)) ||
                 (!(fCommercialMsg) && (fCommercial))) &&
                 (pProvData->pWintrustData->dwUnionChoice != WTD_CHOICE_CERT))
        {
            goto KeyPurposeError;
        }

        pCert->fCommercial = fCommercial;

        if (pInfo->RestrictedKeyUsage.cbData) 
        {
            bKeyUsage = pInfo->RestrictedKeyUsage.pbData[0];
            
            if (!(bKeyUsage & (CERT_DIGITAL_SIGNATURE_KEY_USAGE | CERT_KEY_CERT_SIGN_KEY_USAGE))) 
            {
                goto KeyUsageError;
            }
        }
    }
    
CommonReturn:

    if (pInfo)
    {
        TrustFreeDecode(WVT_MODID_SOFTPUB, (BYTE **)&pInfo);
    }

    return(fRet);

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
 
    TRACE_ERROR_EX(DBG_SS, KeyPurposeError);
    TRACE_ERROR_EX(DBG_SS, KeyUsageError);
    TRACE_ERROR_EX(DBG_SS, DecodeError);
}

BOOL checkBasicConstraints(CRYPT_PROVIDER_DATA *pProvData, CRYPT_PROVIDER_CERT *pCert,
                           DWORD idxCert, PCERT_INFO pCertInfo)
{
    PCERT_EXTENSION                 pExt;
    PCERT_BASIC_CONSTRAINTS_INFO    pInfo;
    DWORD                           cbInfo;
    BYTE                            bRole;
    BOOL                            fRet;

    fRet    = TRUE;
    pInfo   = NULL;

    if (pCertInfo->cExtension == 0) 
    {
        goto CommonReturn;
    }
    
    if (!(pExt = CertFindExtension(szOID_BASIC_CONSTRAINTS,
                                   pCertInfo->cExtension,
                                   pCertInfo->rgExtension)))
    {
        fRet = checkBasicConstraints2(pProvData, pCert, idxCert, pCertInfo);
        goto CommonReturn;
    }

    if (!(TrustDecode(WVT_MODID_SOFTPUB, (BYTE **)&pInfo, &cbInfo, 201,
                          pProvData->dwEncoding, X509_BASIC_CONSTRAINTS, 
                          pExt->Value.pbData, pExt->Value.cbData,
                          CRYPT_DECODE_NOCOPY_FLAG)))
    {
        if (pExt->fCritical) 
        {
            goto DecodeError;
        }

        goto CommonReturn;
    }

    if (pInfo->SubjectType.cbData > 0) 
    {
        bRole = pInfo->SubjectType.pbData[0];

        if ((idxCert == 0) && !(pCert->fSelfSigned))
        {
            if (!(bRole & CERT_END_ENTITY_SUBJECT_FLAG)) 
            {
                goto NotACA;
            }
        } 
        else 
        {
            if (!(bRole & CERT_CA_SUBJECT_FLAG)) 
            {
                goto NotACA;
            }

            if (pInfo->fPathLenConstraint)
            {
                // Check count of CAs below
                if ((idxCert > 0) && ((idxCert - 1) > pInfo->dwPathLenConstraint))
                {
                    goto PathLengthError;
                }
            }
        }
    }
    
    if ((pExt->fCritical) && (pInfo->cSubtreesConstraint))
    {
        goto SubtreesError;
    }


CommonReturn:

    if (pInfo)
    {
        TrustFreeDecode(WVT_MODID_SOFTPUB, (BYTE **)&pInfo);
    }

    return(fRet);

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
 
    TRACE_ERROR_EX(DBG_SS, NotACA);
    TRACE_ERROR_EX(DBG_SS, SubtreesError);
    TRACE_ERROR_EX(DBG_SS, PathLengthError);
    TRACE_ERROR_EX(DBG_SS, DecodeError);
}

BOOL checkBasicConstraints2(CRYPT_PROVIDER_DATA *pProvData, CRYPT_PROVIDER_CERT *pCert,
                           DWORD idxCert, PCERT_INFO pCertInfo)
{
    PCERT_EXTENSION                 pExt;
    PCERT_BASIC_CONSTRAINTS2_INFO   pInfo;
    DWORD                           cbInfo;
    BOOL                            fRet;

    pInfo   = NULL;
    fRet    = TRUE;

    if (pCertInfo->cExtension == 0) 
    {
        goto CommonReturn;
    }
    
    if (!(pExt = CertFindExtension(szOID_BASIC_CONSTRAINTS2,
                                   pCertInfo->cExtension,
                                   pCertInfo->rgExtension)))
    {
        goto CommonReturn;
    }

    if (!(TrustDecode(WVT_MODID_SOFTPUB, (BYTE **)&pInfo, &cbInfo, 201,
                          pProvData->dwEncoding, X509_BASIC_CONSTRAINTS2, 
                          pExt->Value.pbData, pExt->Value.cbData,
                          CRYPT_DECODE_NOCOPY_FLAG)))
    {
        if (pExt->fCritical) 
        {
            goto DecodeError;
        }

        goto CommonReturn;
    }

    if (((idxCert != 0) || (pCert->fSelfSigned)) &&
        !(pCert->fTrustListSignerCert))
    {
        if (!(pInfo->fCA))
        {
            goto NotACA;
        }

        if (pInfo->fPathLenConstraint)
        {
            if ((idxCert > 0) && ((idxCert - 1) > pInfo->dwPathLenConstraint))
            {
                goto PathLengthError;
            }
        }
    }
    
CommonReturn:

    if (pInfo)
    {
        TrustFreeDecode(WVT_MODID_SOFTPUB, (BYTE **)&pInfo);
    }

    return(fRet);

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
 
    TRACE_ERROR_EX(DBG_SS, NotACA);
    TRACE_ERROR_EX(DBG_SS, PathLengthError);
    TRACE_ERROR_EX(DBG_SS, DecodeError);
}

static const char *rgpszKnownExtObjId[] = 
{
    szOID_AUTHORITY_KEY_IDENTIFIER,
    szOID_AUTHORITY_KEY_IDENTIFIER2,

    szOID_CRL_REASON_CODE,
    szOID_CRL_DIST_POINTS,

    szOID_ENHANCED_KEY_USAGE,
    szOID_KEY_USAGE,
    szOID_KEY_USAGE_RESTRICTION,
    szOID_KEY_ATTRIBUTES,

    szOID_SUBJECT_ALT_NAME,
    szOID_SUBJECT_ALT_NAME2,
    szOID_SUBJECT_DIR_ATTRS,
    szOID_SUBJECT_KEY_IDENTIFIER,

    szOID_ISSUER_ALT_NAME,
    szOID_ISSUER_ALT_NAME2,

    szOID_BASIC_CONSTRAINTS,
    szOID_BASIC_CONSTRAINTS2,

    szOID_CERT_POLICIES,
    
    szOID_PRIVATEKEY_USAGE_PERIOD,

    szOID_POLICY_MAPPINGS,

    SPC_COMMON_NAME_OBJID,
    SPC_SP_AGENCY_INFO_OBJID,
    SPC_MINIMAL_CRITERIA_OBJID,
    SPC_FINANCIAL_CRITERIA_OBJID,

    NULL
};


BOOL checkCertAnyUnknownCriticalExtensions(CRYPT_PROVIDER_DATA *pProvData, PCERT_INFO pCertInfo)
{
    PCERT_EXTENSION     pExt;
    DWORD               cExt;
    const char          **ppszObjId;
    const char          *pszObjId;
    
    cExt = pCertInfo->cExtension;
    pExt = pCertInfo->rgExtension;

    for ( ; cExt > 0; cExt--, pExt++) 
    {
        if (pExt->fCritical) 
        {
            ppszObjId = (const char **)rgpszKnownExtObjId;

            while (pszObjId = *ppszObjId++) 
            {
                if (strcmp(pszObjId, pExt->pszObjId) == 0)
                {
                    break;
                }
            }

            if (!(pszObjId))
            {
                //
                //  we don't "know" it...  see if we can decode it.  if so,
                //  we are OK...
                //
                DWORD   cbDecode;

                cbDecode = 0;
                if (!(CryptDecodeObject(pProvData->dwEncoding, pExt->pszObjId,
                                pExt->Value.pbData, pExt->Value.cbData,
                                0, NULL, &cbDecode)))
                {
                    return(FALSE);
                }

                return(TRUE);
            }
        }
    }

    return(TRUE);
}

BOOL checkMeetsMinimalFinancialCriteria(CRYPT_PROVIDER_DATA *pProvData,
                                        PCCERT_CONTEXT pCert,
                                        BOOL *pfAvail, BOOL *pfMeets)
{
    SPC_FINANCIAL_CRITERIA  FinancialCriteria;
    PCERT_EXTENSION         pExt;
    DWORD                   cbInfo;
    BOOL                    fMinimalCriteria;

    pExt        = NULL;

    *pfAvail    = FALSE;
    *pfMeets    = FALSE;

    if (pCert->pCertInfo->cExtension == 0)
    {
        return(TRUE);
    }

    pExt = CertFindExtension(   SPC_FINANCIAL_CRITERIA_OBJID,
                                pCert->pCertInfo->cExtension,
                                pCert->pCertInfo->rgExtension);
    
    if (pExt) 
    {
        cbInfo = sizeof(SPC_FINANCIAL_CRITERIA);

        if (!(CryptDecodeObject(pProvData->dwEncoding,
                                SPC_FINANCIAL_CRITERIA_STRUCT,
                                pExt->Value.pbData,
                                pExt->Value.cbData,
                                0,                  // dwFlags
                                &FinancialCriteria,
                                &cbInfo)))
        {
            return(FALSE);
        }
        
        *pfAvail = FinancialCriteria.fFinancialInfoAvailable;
        *pfMeets = FinancialCriteria.fMeetsCriteria;

        return(TRUE);
    }
    
    pExt = CertFindExtension(SPC_MINIMAL_CRITERIA_OBJID,
                             pCert->pCertInfo->cExtension,
                             pCert->pCertInfo->rgExtension);
    if (!(pExt))
    {
        return(TRUE);
    }

    cbInfo = sizeof(BOOL);

    if (!(CryptDecodeObject(pProvData->dwEncoding,
                            SPC_MINIMAL_CRITERIA_STRUCT,
                            pExt->Value.pbData,
                            pExt->Value.cbData,
                            0,                  // dwFlags
                            &fMinimalCriteria,
                            &cbInfo)))
    {
        return(FALSE);
    }
    
    *pfAvail = TRUE;
    *pfMeets = fMinimalCriteria;
    

    return(TRUE);
}


BOOL checkRevocation(CRYPT_PROVIDER_DATA *pProvData, CRYPT_PROVIDER_SGNR *pSgnr, BOOL fCommercial,
                     DWORD *pdwError)
{
    if (pProvData->pWintrustData->fdwRevocationChecks != WTD_REVOKE_NONE)
    {
        return(TRUE);
    }

    CERT_REVOCATION_PARA    sRevPara;
    CERT_REVOCATION_STATUS  sRevStatus;
    PCERT_CONTEXT           pasCertContext[1];
    CRYPT_PROVIDER_CERT     *pCert;


    memset(&sRevPara, 0x00, sizeof(CERT_REVOCATION_PARA));

    sRevPara.cbSize         = sizeof(CERT_REVOCATION_PARA);

    // issuer cert = 1
    if (pCert = WTHelperGetProvCertFromChain(pSgnr, 1))
    {
        sRevPara.pIssuerCert    = pCert->pCert;
    }

    memset(&sRevStatus, 0x00, sizeof(CERT_REVOCATION_STATUS));

    sRevStatus.cbSize       = sizeof(CERT_REVOCATION_STATUS);

    // publisher cert = 0
    pCert = WTHelperGetProvCertFromChain(pSgnr, 0);
    pasCertContext[0]       = (PCERT_CONTEXT)pCert->pCert;

    if (pCert->fSelfSigned)
    {
        return(TRUE); // verisign doesn't keep thier roots in the database (???)
    }

    if (!(CertVerifyRevocation(pProvData->dwEncoding,
                               CERT_CONTEXT_REVOCATION_TYPE,
                               1,
                               (void **)pasCertContext,
                               0, // CERT_VERIFY_REV_CHAIN_FLAG,
                               &sRevPara,
                               &sRevStatus)))
    {
        pCert->dwRevokedReason  = sRevStatus.dwReason;

        switch(sRevStatus.dwError)
        {
            case CRYPT_E_REVOKED:
                *pdwError = CERT_E_REVOKED;
                pCert->dwError = *pdwError;
                return(FALSE);

            case CRYPT_E_NOT_IN_REVOCATION_DATABASE:
                return(TRUE);

            case CRYPT_E_REVOCATION_OFFLINE:
            default:
                if (fCommercial)
                {
                    if (pProvData->dwRegPolicySettings & WTPF_OFFLINEOK_COM)
                    {
                        return(TRUE);
                    }
                }
                else
                {
                    if (pProvData->dwRegPolicySettings & WTPF_OFFLINEOK_IND)
                    {
                        return(TRUE);
                    }
                }   

                *pdwError = CERT_E_REVOCATION_FAILURE;
                pCert->dwError = *pdwError;
                return(FALSE);

        }
    }

    return(TRUE);
}

