//-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       tcrack.cpp
//
//  Contents:   API testing of CryptEncodeObject/CryptDecodeObject.
//
//  History:    29-January-97   xiaohs   created
//
//--------------------------------------------------------------------------
#include "global.hxx"

//--------------------------------------------------------------------------
//  See if the sequence "81 7f" is in the BLOB.  If it is, we need to fix it
//--------------------------------------------------------------------------
BOOL    BadCert(DWORD   cbEncoded, BYTE *pbEncoded)
{
    DWORD   iIndex=0;
    DWORD   iLimit=cbEncoded-2;
    BYTE    rgByte[2];


    assert(pbEncoded);


    memset(rgByte, 0, 2);

    //set the rgByte to be the patter of 0x81 0x7F, which is 10000001 and 01111111,
    //whic his 129 and 127 in decimal
    rgByte[0]=rgByte[0]|129;
    rgByte[1]=rgByte[1]|127;


    for(iIndex=0;iIndex<=iLimit;iIndex++)
    {
        if(memcmp(rgByte,&(pbEncoded[iIndex]),2)==0)
            return TRUE;
    }

    return FALSE;

}

//--------------------------------------------------------------------------
//  Copy the BLOBs
//--------------------------------------------------------------------------
void    SetData(DWORD   cbNewData, BYTE *pbNewData,
                DWORD   *pcbOldData, BYTE **ppbOldData)
{
    assert(pcbOldData);
    assert(ppbOldData);

    *pcbOldData=cbNewData;
    *ppbOldData=pbNewData;
}


///////////////////////////////////////////////////////////////////////////
//Certificate Manipulation Functions
//--------------------------------------------------------------------------
//  This is the functions
//--------------------------------------------------------------------------
BOOL    Fix7FCert(DWORD cbEncoded, BYTE *pbEncoded, DWORD *pcbEncoded,
                        BYTE    **ppbEncoded)
{

    //init
    *pcbEncoded=0;
    *ppbEncoded=NULL;



    if(!BadCert(cbEncoded, pbEncoded))
        return TRUE;


    if(DecodeX509_CERT(cbEncoded, pbEncoded,pcbEncoded,
                        ppbEncoded))
        return TRUE;
    else
    {
        //release the memory
        SAFE_FREE(ppbEncoded)
        *ppbEncoded=NULL;
        *pcbEncoded=0;
        return FALSE;
    }


}

//--------------------------------------------------------------------------
//  A general routine to encode a struct based on lpszStructType
//--------------------------------------------------------------------------
BOOL    EncodeStruct(LPCSTR lpszStructType, void *pStructInfo,DWORD *pcbEncoded,
                     BYTE **ppbEncoded)
{
    BOOL    fSucceeded=FALSE;
    DWORD   cbEncoded=NULL;

    //init
    *pcbEncoded=0;
    *ppbEncoded=NULL;

    assert(lpszStructType);
    assert(pStructInfo);


    //length only calculation
    TESTC(CryptEncodeObject(CRYPT_ENCODE_TYPE,lpszStructType, pStructInfo,NULL,
            &cbEncoded),TRUE)

    //the struct has to be more than 0 byte
    assert(cbEncoded);

    //allocate the correct amount of memory
    *ppbEncoded=(BYTE *)SAFE_ALLOC(cbEncoded);
    CHECK_POINTER(*ppbEncoded);

    //Encode the strcut with *pcbEncoded == the correct length
    *pcbEncoded=cbEncoded;

    //Encode the struct
    TESTC(CryptEncodeObject(CRYPT_ENCODE_TYPE,lpszStructType,pStructInfo,*ppbEncoded,
        pcbEncoded),TRUE)

    fSucceeded=TRUE;

TCLEANUP:

    return fSucceeded;

}

//--------------------------------------------------------------------------
//  A general routine to decode a BLOB based on lpszStructType
//
//--------------------------------------------------------------------------
BOOL  DecodeBLOB(LPCSTR lpszStructType,DWORD cbEncoded, BYTE *pbEncoded,
                  DWORD *pcbStructInfo, void **ppvStructInfo)
{
    BOOL    fSucceeded=FALSE;
    DWORD   cbStructInfo=0;

    //init
    *pcbStructInfo=0;
    *ppvStructInfo=NULL;

    assert(lpszStructType);
    assert(pbEncoded);
    assert(cbEncoded);

    //Decode.  Length Only Calculation
    TESTC(CryptDecodeObject(CRYPT_ENCODE_TYPE,lpszStructType,pbEncoded,cbEncoded,
    CRYPT_DECODE_FLAG,NULL,&cbStructInfo),TRUE)

    //the struct has to be more than 0 byte
    assert(cbStructInfo);

    *ppvStructInfo=(BYTE *)SAFE_ALLOC(cbStructInfo);
    CHECK_POINTER(*ppvStructInfo);

    //Decode the BLOB with *pcbStructInfo==correct length
    *pcbStructInfo=cbStructInfo;

    TESTC(CryptDecodeObject(CRYPT_ENCODE_TYPE,lpszStructType,pbEncoded,cbEncoded,
    CRYPT_DECODE_FLAG,*ppvStructInfo,pcbStructInfo),TRUE)


    fSucceeded=TRUE;

TCLEANUP:

    return fSucceeded;

}

//--------------------------------------------------------------------------
//  Decode X509_CERT BLOBs
//
//--------------------------------------------------------------------------
BOOL    DecodeX509_CERT(DWORD cbEncoded, BYTE *pbEncoded,DWORD *pcbEncoded,
                        BYTE    **ppbEncoded)
{
    BOOL    fSucceeded=FALSE;
    DWORD   cbStructInfo=0;
    void    *pStructInfo=NULL;
    LPCSTR  lpszStructType=NULL;
    DWORD   cbToBeSigned=0;
    BYTE    *pbToBeSigned=NULL;
    DWORD   cbOldSigned=0;
    BYTE    *pbOldSigned=NULL;

    //init
    lpszStructType=X509_CERT;


    //Decode the encoded BLOB
    TESTC(DecodeBLOB(lpszStructType,cbEncoded, pbEncoded,&cbStructInfo,
        &pStructInfo),TRUE)

    //Further Decode the X509_CERT_TO_BE_SIGNED
    //Notice we should use the original cbData and pbData passed in for Decode
    //but use ToBeSigned in CERT_SIGNED_CONTENT_INFO for encode purpose

    TESTC(DecodeX509_CERT_TO_BE_SIGNED(cbEncoded,
        pbEncoded,&cbToBeSigned,&pbToBeSigned),TRUE);

    //copy the new encoded BLOB
    SetData((((PCERT_SIGNED_CONTENT_INFO)pStructInfo)->ToBeSigned).cbData,
               (((PCERT_SIGNED_CONTENT_INFO)pStructInfo)->ToBeSigned).pbData,
               &cbOldSigned, &pbOldSigned);

    SetData(cbToBeSigned, pbToBeSigned,
                &((((PCERT_SIGNED_CONTENT_INFO)pStructInfo)->ToBeSigned).cbData),
                &((((PCERT_SIGNED_CONTENT_INFO)pStructInfo)->ToBeSigned).pbData));


    //if requested, encode the BLOB back to what it was.  Make sure no data is lost
    //by checking the size of the encoded blob and do a memcmp.
    TESTC(EncodeStruct(lpszStructType, pStructInfo,pcbEncoded, ppbEncoded),TRUE);

    fSucceeded=TRUE;

TCLEANUP:

    SetData(cbOldSigned, pbOldSigned,
            &((((PCERT_SIGNED_CONTENT_INFO)pStructInfo)->ToBeSigned).cbData),
            &((((PCERT_SIGNED_CONTENT_INFO)pStructInfo)->ToBeSigned).pbData));


    SAFE_FREE(pStructInfo)

    SAFE_FREE(pbToBeSigned)

    return fSucceeded;

}



//--------------------------------------------------------------------------
//  Decode X509_CERT_TO_BE_SIGNED BLOBs
//
//--------------------------------------------------------------------------
BOOL    DecodeX509_CERT_TO_BE_SIGNED(DWORD  cbEncoded, BYTE *pbEncoded, DWORD *pcbEncoded,
                        BYTE    **ppbEncoded)
{

    BOOL    fSucceeded=FALSE;
    DWORD   cbStructInfo=0;
    void    *pStructInfo=NULL;
    LPCSTR  lpszStructType=NULL;


    DWORD   cbOldIssuer=0;
    BYTE    *pbOldIssuer=NULL;
    DWORD   cbIssuer=0;
    BYTE    *pbIssuer=NULL;


    DWORD   cbOldSubject=0;
    BYTE    *pbOldSubject=NULL;
    DWORD   cbSubject=0;
    BYTE    *pbSubject=NULL;


    //init
    lpszStructType=X509_CERT_TO_BE_SIGNED;


    //Decode the encoded BLOB
    TESTC(DecodeBLOB(lpszStructType,cbEncoded, pbEncoded,&cbStructInfo,
        &pStructInfo),TRUE)


      //Decode Issuer in CERT_INFO struct
    TESTC(DecodeX509_NAME((((PCERT_INFO)pStructInfo)->Issuer).cbData,
    (((PCERT_INFO)pStructInfo)->Issuer).pbData,&cbIssuer,&pbIssuer),TRUE)

    SetData((((PCERT_INFO)pStructInfo)->Issuer).cbData,
            (((PCERT_INFO)pStructInfo)->Issuer).pbData,&cbOldIssuer,&pbOldIssuer);

    SetData(cbIssuer, pbIssuer,
            &((((PCERT_INFO)pStructInfo)->Issuer).cbData),
            &((((PCERT_INFO)pStructInfo)->Issuer).pbData));



    //Decode Subject in CERT_INFO struct
    TESTC(DecodeX509_NAME((((PCERT_INFO)pStructInfo)->Subject).cbData,
    (((PCERT_INFO)pStructInfo)->Subject).pbData,&cbSubject,&pbSubject),TRUE)

    SetData((((PCERT_INFO)pStructInfo)->Subject).cbData,
    (((PCERT_INFO)pStructInfo)->Subject).pbData,
    &cbOldSubject, &pbOldSubject);

    SetData(cbSubject, pbSubject,
        &((((PCERT_INFO)pStructInfo)->Subject).cbData),
        &((((PCERT_INFO)pStructInfo)->Subject).pbData));




    //if requested, encode the BLOB back to what it was.  Make sure no data is lost
    //by checking the size of the encoded blob and do a memcmp.
    TESTC(EncodeStruct(lpszStructType, pStructInfo,pcbEncoded,
        ppbEncoded),TRUE);



    fSucceeded=TRUE;

TCLEANUP:

    //copy back the old values
    SetData(cbOldSubject, pbOldSubject,
        &((((PCERT_INFO)pStructInfo)->Subject).cbData),
        &((((PCERT_INFO)pStructInfo)->Subject).pbData));


    SetData(cbOldIssuer, pbOldIssuer,
            &((((PCERT_INFO)pStructInfo)->Issuer).cbData),
            &((((PCERT_INFO)pStructInfo)->Issuer).pbData));



    SAFE_FREE(pStructInfo)

    SAFE_FREE(pbSubject)

    SAFE_FREE(pbIssuer)

    return fSucceeded;

}

//--------------------------------------------------------------------------
//  Decode X509_NAME BLOBs
//
//--------------------------------------------------------------------------
BOOL    DecodeX509_NAME(DWORD   cbEncoded, BYTE *pbEncoded, DWORD *pcbEncoded,
                        BYTE    **ppbEncoded)
{

    BOOL    fSucceeded=FALSE;
    DWORD   cbStructInfo=0;
    void    *pStructInfo=NULL;
    LPCSTR  lpszStructType=NULL;


    //init
    lpszStructType=X509_NAME;


    //Decode the encoded BLOB
    TESTC(DecodeBLOB(lpszStructType,cbEncoded, pbEncoded,&cbStructInfo,
        &pStructInfo),TRUE)


    //if requested, encode the BLOB back to what it was.  Make sure no data is lost
    //by checking the size of the encoded blob and do a memcmp.
    TESTC(EncodeStruct(lpszStructType, pStructInfo,pcbEncoded, ppbEncoded),TRUE);


    fSucceeded=TRUE;

TCLEANUP:

    SAFE_FREE(pStructInfo)

    return fSucceeded;

}



