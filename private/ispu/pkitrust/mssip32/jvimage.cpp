//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       jvimage.cpp
//
//  Contents:   Microsoft SIP Provider (JAVA utilities)
//
//  History:    15-Feb-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include "global.hxx"

BOOL SeekAndReadFile(HANDLE hFile, DWORD lFileOffset,BYTE *pb,
                     DWORD cb);

BOOL SeekAndWriteFile(HANDLE hFile, DWORD lFileOffset, BYTE *pb,
                      DWORD cb);

typedef void *HSHPDIGESTDATA;
typedef BOOL (WINAPI *PFN_SHP_DIGEST_DATA)(HSHPDIGESTDATA hDigestData,
                                           BYTE *pbData,
                                           DWORD cbData);

typedef void *HSHPSIGNFILE;


typedef struct _JAVA_OPEN_ARG 
{
    HANDLE  hFile;
} JAVA_OPEN_ARG, *PJAVA_OPEN_ARG;


typedef struct _JAVA_FUNC_PARA 
{
    HANDLE              hFile;
    BYTE                *pbSignedData;
} JAVA_FUNC_PARA, *PJAVA_FUNC_PARA;

typedef struct _JAVA_DIGEST_PARA 
{
    BOOL                fDisableDigest;
    PFN_SHP_DIGEST_DATA pfnDigestData;
    HSHPDIGESTDATA      hDigestData;
} JAVA_DIGEST_PARA, *PJAVA_DIGEST_PARA;

typedef struct _JAVA_SIGN_PARA 
{
    WORD                wConstPoolCount;
    WORD                wSignConstPoolIndex;
    LONG                lSignConstPoolOffset;
    WORD                wAttrCount;
    LONG                lAttrCountOffset;
    WORD                wSignAttrIndex;
    DWORD               dwSignAttrLength;
    LONG                lSignAttrOffset;
    LONG                lEndOfFileOffset;
} JAVA_SIGN_PARA, *PJAVA_SIGN_PARA;

typedef struct _JAVA_READ_PARA 
{
    BOOL                fResult;
    DWORD               dwLastError;
    LONG                lFileOffset;
    DWORD               cbCacheRead;
    DWORD               cbCacheRemain;
} JAVA_READ_PARA, *PJAVA_READ_PARA;

#define JAVA_READ_CACHE_LEN 512

typedef struct _JAVA_PARA 
{
    JAVA_FUNC_PARA      Func;
    JAVA_DIGEST_PARA    Digest;
    JAVA_SIGN_PARA      Sign;
    JAVA_READ_PARA      Read;
    BYTE                rgbCache[JAVA_READ_CACHE_LEN];
} JAVA_PARA, *PJAVA_PARA;



#define JAVA_MAGIC          0xCAFEBABE
#define JAVA_MINOR_VERSION  3
#define JAVA_MAJOR_VERSION  45

// Constant Pool tags
//
// Note: CONSTANT_Long and CONSTANT_Double use two constant pool indexes.
enum 
{
    CONSTANT_Utf8                   = 1,
    CONSTANT_Unicode                = 2,
    CONSTANT_Integer                = 3,
    CONSTANT_Float                  = 4,
    CONSTANT_Long                   = 5,
    CONSTANT_Double                 = 6,
    CONSTANT_Class                  = 7,
    CONSTANT_String                 = 8,
    CONSTANT_Fieldref               = 9,
    CONSTANT_Methodref              = 10,
    CONSTANT_InterfaceMethodref     = 11,
    CONSTANT_NameAndType            = 12
};

// Constant Pool Info lengths (excludes the tag)
DWORD rgConstPoolLength[] = 
{
    0, // tag of zero not used
    0, // CONSTANT_Utf8 (special case)
    0, // CONSTANT_Unicode (special case)
    4, // CONSTANT_Integer_info
    4, // CONSTANT_Float_info
    8, // CONSTANT_Long_info
    8, // CONSTANT_Double_info
    2, // CONSTANT_Class_info
    2, // CONSTANT_String_info
    4, // CONSTANT_Fieldref_info
    4, // CONSTANT_Methodref_info
    4, // CONSTANT_InterfaceMethodref_info
    4  // CONSTANT_NameAndType_info
};

static inline void *ShpAlloc(DWORD cbytes)
{
    void    *pvRet;

    pvRet = (void *)new BYTE[cbytes];

    if (!(pvRet))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }
    return(pvRet);
}

static inline void ShpFree(void *pv)
{
    if (pv)
    {
        delete pv;
    }
}

// The following functions convert to/from Java bytes which are big endian
static inline void ToJavaU2(WORD w, BYTE rgb[])
{
    rgb[0] = HIBYTE(w);
    rgb[1] = LOBYTE(w);
}
static inline WORD FromJavaU2(BYTE rgb[])
{
    return  ((WORD)rgb[0]<<8) | ((WORD)rgb[1]<<0);
}

static inline void ToJavaU4(DWORD dw, BYTE rgb[])
{
    rgb[0] = HIBYTE(HIWORD(dw));
    rgb[1] = LOBYTE(HIWORD(dw));
    rgb[2] = HIBYTE(LOWORD(dw));
    rgb[3] = LOBYTE(LOWORD(dw));
}
static inline DWORD FromJavaU4(BYTE rgb[])
{
    return  ((DWORD)rgb[0]<<24) |
            ((DWORD)rgb[1]<<16) |
            ((DWORD)rgb[2]<<8)  |
            ((DWORD)rgb[3]<<0);
}

#define CONST_POOL_COUNT_OFFSET     8
#define UTF8_HDR_LENGTH             (1+2)
#define ATTR_HDR_LENGTH             (2+4)
#define SIGN_ATTR_NAME_LENGTH       19
#define SIGN_CONST_POOL_LENGTH      (UTF8_HDR_LENGTH + SIGN_ATTR_NAME_LENGTH)

static const char rgchSignAttrName[SIGN_ATTR_NAME_LENGTH + 1] =
                                "_digital_signature_";


//+-------------------------------------------------------------------------
//  Shift the bytes in the file.
//
//  If lbShift is positive, the bytes are shifted toward the end of the file.
//  If lbShift is negative, the bytes are shifted toward the start of the file.
//--------------------------------------------------------------------------
static
BOOL
JavaShiftFileBytes(
    IN HANDLE hFile,
    IN PBYTE pbCache,
    IN LONG cbCache,
    IN LONG lStartOffset,
    IN LONG lEndOffset,
    IN LONG lbShift
    )
{
    LONG cbTotalMove, cbMove;

    cbTotalMove = lEndOffset - lStartOffset;
    while (cbTotalMove) {
        cbMove = min(cbTotalMove, cbCache);

        if (lbShift > 0) {
            if (!SeekAndReadFile(hFile, lEndOffset - cbMove,
                    pbCache, cbMove))
                return FALSE;
            if (!SeekAndWriteFile(hFile, (lEndOffset - cbMove) + lbShift,
                    pbCache, cbMove))
                return FALSE;
            lEndOffset -= cbMove;
        } else if (lbShift < 0) {
            if (!SeekAndReadFile(hFile, lStartOffset, pbCache, cbMove))
                return FALSE;
            if (!SeekAndWriteFile(hFile, lStartOffset + lbShift,
                    pbCache, cbMove))
                return FALSE;
            lStartOffset += cbMove;
        }
        cbTotalMove -= cbMove;
    }
    return TRUE;
}


//+-------------------------------------------------------------------------
//  Low level functions for reading the Java Class File.
//
//  If not disabled, the read bytes are also hashed.
//
//  For an error, remaining unread values are zero'ed and
//  pPara->Read.fResult = FALSE.
//--------------------------------------------------------------------------
static void ReadJavaBytes(
    IN PJAVA_PARA pPara,
    OUT OPTIONAL BYTE *pb,  // if NULL, bytes are hashed and then skipped
    IN DWORD cb
    )
{
    DWORD cbCacheRemain = pPara->Read.cbCacheRemain;
    DWORD lFileOffset = pPara->Read.lFileOffset;
    BOOL fDisableDigest = pPara->Digest.pfnDigestData == NULL ||
                                pPara->Digest.fDisableDigest;

    if (!pPara->Read.fResult)
        goto ErrorReturn;

    while (cb > 0) {
        DWORD cbCopy;
        BYTE *pbCache;

        if (cbCacheRemain == 0) {
            if (!ReadFile(pPara->Func.hFile, pPara->rgbCache,
                    sizeof(pPara->rgbCache), &cbCacheRemain, NULL))
                goto ErrorReturn;
            if (cbCacheRemain == 0) goto ErrorReturn;
            pPara->Read.cbCacheRead = cbCacheRemain;
        }

        cbCopy = min(cb, cbCacheRemain);
        pbCache = &pPara->rgbCache[pPara->Read.cbCacheRead - cbCacheRemain];
        if (!fDisableDigest) {
            if (!pPara->Digest.pfnDigestData(
                    pPara->Digest.hDigestData,
                    pbCache,
                    cbCopy)) goto ErrorReturn;
        }
        if (pb) {
            memcpy(pb, pbCache, cbCopy);
            pb += cbCopy;
        }
        cb -= cbCopy;
        cbCacheRemain -= cbCopy;
        lFileOffset += cbCopy;
    }
    goto CommonReturn;

ErrorReturn:
    if (pPara->Read.fResult) {
        // First error
        pPara->Read.fResult = FALSE;
        pPara->Read.dwLastError = GetLastError();
    }
    if (pb && cb)
        memset(pb, 0, cb);
CommonReturn:
    pPara->Read.cbCacheRemain = cbCacheRemain;
    pPara->Read.lFileOffset = lFileOffset;

}

static void SkipJavaBytes(IN PJAVA_PARA pPara, IN DWORD cb)
{
    ReadJavaBytes(pPara, NULL, cb);
}

static BYTE ReadJavaU1(IN PJAVA_PARA pPara)
{
    BYTE b;
    ReadJavaBytes(pPara, &b, 1);
    return b;
}
static WORD ReadJavaU2(IN PJAVA_PARA pPara) 
{
    BYTE rgb[2];
    ReadJavaBytes(pPara, rgb, 2);
    return FromJavaU2(rgb);
}
static DWORD ReadJavaU4(IN PJAVA_PARA pPara) 
{
    BYTE rgb[4];
    ReadJavaBytes(pPara, rgb, 4);
    return FromJavaU4(rgb);
}


//+-------------------------------------------------------------------------
//  .
//--------------------------------------------------------------------------
static
BOOL
GetSignedDataFromJavaClassFile(
    IN HSHPSIGNFILE hSignFile,
    OUT BYTE **ppbSignedData,
    OUT DWORD *pcbSignedData
    )
{
    BOOL fResult;
    PJAVA_PARA pPara = (PJAVA_PARA) hSignFile;
    BYTE *pbSignedData = NULL;
    DWORD cbSignedData;

    cbSignedData = pPara->Sign.dwSignAttrLength;
    if (cbSignedData == 0) {
        SetLastError((DWORD)TRUST_E_NOSIGNATURE);
        goto ErrorReturn;
    }

    pbSignedData = pPara->Func.pbSignedData;
    if (pbSignedData == NULL) {
        if (NULL == (pbSignedData = (BYTE *) ShpAlloc(cbSignedData)))
            goto ErrorReturn;
        if (!SeekAndReadFile(
                pPara->Func.hFile,
                pPara->Sign.lSignAttrOffset + ATTR_HDR_LENGTH,
                pbSignedData,
                cbSignedData))
            goto ErrorReturn;
        pPara->Func.pbSignedData = pbSignedData;
    }

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbSignedData) {
        ShpFree(pbSignedData);
        pbSignedData = NULL;
    }
    cbSignedData = 0;
    fResult = FALSE;
CommonReturn:
    *ppbSignedData = pbSignedData;
    *pcbSignedData = cbSignedData;
    return fResult;
}

//+-------------------------------------------------------------------------
//  .
//--------------------------------------------------------------------------
static
BOOL
SetSignedDataIntoJavaClassFile(
    IN HSHPSIGNFILE hSignFile,
    IN const BYTE *pbSignedData,
    IN DWORD cbSignedData
    )
{
    PJAVA_PARA pPara = (PJAVA_PARA) hSignFile;
    HANDLE hFile = pPara->Func.hFile;

    if (pbSignedData == NULL || cbSignedData == 0)
        // Length only
        return TRUE;

    if (pPara->Sign.wSignConstPoolIndex == pPara->Sign.wConstPoolCount) {
        BYTE rgb[SIGN_CONST_POOL_LENGTH];
        // Add a new constant pool entry for the name of the
        // signed data attribute.

        // First, make room in the file by shifting all the bytes that follow.
        if (!JavaShiftFileBytes(
                pPara->Func.hFile,
                pPara->rgbCache,
                sizeof(pPara->rgbCache),
                pPara->Sign.lSignConstPoolOffset,
                pPara->Sign.lEndOfFileOffset,
                SIGN_CONST_POOL_LENGTH))
            return FALSE;

        // Update offsets that have been shifted
        pPara->Sign.lAttrCountOffset += SIGN_CONST_POOL_LENGTH;
        pPara->Sign.lSignAttrOffset += SIGN_CONST_POOL_LENGTH;
        pPara->Sign.lEndOfFileOffset += SIGN_CONST_POOL_LENGTH;

        // Increment u2 constant_pool_count and update in file
        pPara->Sign.wConstPoolCount++;
        ToJavaU2(pPara->Sign.wConstPoolCount, rgb);
        if (!SeekAndWriteFile(hFile, CONST_POOL_COUNT_OFFSET, rgb, 2))
            return FALSE;

        // Add constant pool entry for the sign attr name and update in file
        rgb[0] = CONSTANT_Utf8;
        ToJavaU2(SIGN_ATTR_NAME_LENGTH, &rgb[1]);
        memcpy(&rgb[1+2], rgchSignAttrName, SIGN_ATTR_NAME_LENGTH);
        if (!SeekAndWriteFile(hFile, pPara->Sign.lSignConstPoolOffset, rgb,
                SIGN_CONST_POOL_LENGTH))
            return FALSE;
    }

    if (pPara->Sign.dwSignAttrLength == 0) {
        // Add a new attribute for the signed data. The attribute will
        // be added at the end of the file.
        assert(pPara->Sign.lSignAttrOffset == pPara->Sign.lEndOfFileOffset);
        pPara->Sign.lEndOfFileOffset += ATTR_HDR_LENGTH + cbSignedData;

        // Increment u2 attribute_count and update in file
        BYTE rgb[2];
        pPara->Sign.wAttrCount++;
        ToJavaU2(pPara->Sign.wAttrCount, rgb);
        if (!SeekAndWriteFile(hFile, pPara->Sign.lAttrCountOffset, rgb, 2))
            return FALSE;

    } else {
        // The file already has a signed data attribute.

        // If its length is different from the new signed data
        // then, the bytes that follow the attribute will
        // need to be shifted by the difference in length of the old and new
        // signed data
        LONG lbShift = cbSignedData - pPara->Sign.dwSignAttrLength;
        if (lbShift != 0) {
            if (!JavaShiftFileBytes(
                    pPara->Func.hFile,
                    pPara->rgbCache,
                    sizeof(pPara->rgbCache),
                    pPara->Sign.lSignAttrOffset +
                            (ATTR_HDR_LENGTH + pPara->Sign.dwSignAttrLength),
                    pPara->Sign.lEndOfFileOffset,
                    lbShift))
            return FALSE;
            
            pPara->Sign.lEndOfFileOffset += lbShift;
        }
    }
    pPara->Sign.dwSignAttrLength = cbSignedData;

    {
        // Update the file with the signed data attribute
        BYTE rgb[ATTR_HDR_LENGTH];
        DWORD cbWritten;
        ToJavaU2(pPara->Sign.wSignConstPoolIndex, rgb); // u2 attribute_name
        ToJavaU4(cbSignedData, &rgb[2]);                // u4 attribute_length
        if (!SeekAndWriteFile(hFile, pPara->Sign.lSignAttrOffset, rgb,
                ATTR_HDR_LENGTH))
            return FALSE;
        if (!WriteFile(hFile, pbSignedData, cbSignedData, &cbWritten, NULL) ||
                cbWritten != cbSignedData)
            return FALSE;
    }

    // Set end of file
    if (0xFFFFFFFF == SetFilePointer(
            hFile,
            pPara->Sign.lEndOfFileOffset,
            NULL,           // lpDistanceToMoveHigh
            FILE_BEGIN))
        return FALSE;
    return SetEndOfFile(hFile);
}

//+-------------------------------------------------------------------------
//  Reads and optionally digests the Java Class file. Locates the signed data.
//--------------------------------------------------------------------------
static
BOOL
ProcessJavaClassFile(
    PJAVA_PARA pPara,
    BOOL fInit
    )
{
    char rgchTmpSignAttrName[SIGN_ATTR_NAME_LENGTH];
    WORD wLength;
    DWORD dwLength;
    WORD wCount;
    WORD wConstPoolCount;
    WORD wConstPoolIndex;
    WORD wSignConstPoolIndex;
    WORD wAttrCount;
    WORD wAttrIndex;
    WORD wAttrName;
    WORD wSignAttrIndex;
    LONG lAddConstPoolOffset;
    int i;

    memset(&pPara->Read, 0, sizeof(pPara->Read));
    pPara->Read.fResult = TRUE;
    if (0xFFFFFFFF == SetFilePointer(
            pPara->Func.hFile,
            0,              // lDistanceToMove
            NULL,           // lpDistanceToMoveHigh
            FILE_BEGIN))
        return FALSE;
    if (fInit) {
        memset(&pPara->Digest, 0, sizeof(pPara->Digest));
        memset(&pPara->Sign, 0, sizeof(pPara->Sign));
    }

    // Default is to be digested. We'll disable where appropriate. Note,
    // skipped bytes are still digested.
    pPara->Digest.fDisableDigest = FALSE;

    // Read / skip the fields at the beginning of the class file
    if (ReadJavaU4(pPara) != JAVA_MAGIC) 
    {  // u4 magic
        SetLastError(ERROR_BAD_FORMAT);
        return FALSE;
    }
    SkipJavaBytes(pPara, 2 + 2);            // u2 minor_version
                                            // u2 major_version

    pPara->Digest.fDisableDigest = TRUE;
    wConstPoolCount = ReadJavaU2(pPara);    // u2 constant_pool_count
    pPara->Digest.fDisableDigest = FALSE;

    // For fInit, wSignConstPoolIndex has already been zeroed
    wSignConstPoolIndex = pPara->Sign.wSignConstPoolIndex;

    // Iterate through the constant pools. Don't digest the constant pool
    // containing the _digital_signature_ name (wSignConstPoolIndex).
    // For fInit, find the last "_digital_signature_".
    //
    // Note: constant pool index 0 isn't stored in the file.
    wConstPoolIndex = 1;
    while (wConstPoolIndex < wConstPoolCount) {
        BYTE bTag;

        if (wConstPoolIndex == wSignConstPoolIndex)
            pPara->Digest.fDisableDigest = TRUE;

        bTag = ReadJavaU1(pPara);
        switch (bTag) {
        case CONSTANT_Utf8:
            wLength = ReadJavaU2(pPara);
            if (fInit && wLength == SIGN_ATTR_NAME_LENGTH) {
                ReadJavaBytes(pPara, (BYTE *) rgchTmpSignAttrName,
                    SIGN_ATTR_NAME_LENGTH);
                if (memcmp(rgchSignAttrName, rgchTmpSignAttrName,
                        SIGN_ATTR_NAME_LENGTH) == 0) {
                    wSignConstPoolIndex = wConstPoolIndex;
                    pPara->Sign.lSignConstPoolOffset =
                        pPara->Read.lFileOffset - SIGN_CONST_POOL_LENGTH;

                }
            } else
                SkipJavaBytes(pPara, wLength);
            break;
        case CONSTANT_Unicode:
            wLength = ReadJavaU2(pPara);
            SkipJavaBytes(pPara, ((DWORD) wLength) * 2);
            break;
        case CONSTANT_Integer:
        case CONSTANT_Float:
        case CONSTANT_Long:
        case CONSTANT_Double:
        case CONSTANT_Class:
        case CONSTANT_String:
        case CONSTANT_Fieldref:
        case CONSTANT_Methodref:
        case CONSTANT_InterfaceMethodref:
        case CONSTANT_NameAndType:
            SkipJavaBytes(pPara, rgConstPoolLength[bTag]);
            break;
        default:
            SetLastError(ERROR_BAD_FORMAT);
            return FALSE;
        }
            
        pPara->Digest.fDisableDigest = FALSE;

        if (bTag == CONSTANT_Long || bTag == CONSTANT_Double)
            wConstPoolIndex += 2;
        else
            wConstPoolIndex++;
    }

    if (fInit) {
        lAddConstPoolOffset = pPara->Read.lFileOffset;
        if (wSignConstPoolIndex == 0) {
            // Didn't find a constant pool for the digital_signature. Update
            // with where it will need to be added
            wSignConstPoolIndex = wConstPoolCount;
            pPara->Sign.lSignConstPoolOffset = lAddConstPoolOffset;
        }
    }

    // Globble up and hash the bytes until we reach the attributes which are
    // at the end of the file.

    SkipJavaBytes(pPara, 2 + 2 + 2);        // u2 access_flags
                                            // u2 this_class
                                            // u2 super_class
    wCount = ReadJavaU2(pPara);             // u2 interfaces_count
    // u2 interfaces[interfaces_count]
    SkipJavaBytes(pPara, ((DWORD) wCount) * 2);

    // Since fields and methods have identical class file storage, do this
    // twice.
    i = 2;
    while (i--) {
        wCount = ReadJavaU2(pPara);         // u2 fields_count | methods_count
        while (wCount--) {
            SkipJavaBytes(pPara, 2 + 2 + 2);    // u2 access_flags
                                                // u2 name_index
                                                // u2 signature_index
            wAttrCount = ReadJavaU2(pPara);     // u2 attributes_count
            while (wAttrCount--) {
                SkipJavaBytes(pPara, 2);            // u2 attribute_name
                dwLength = ReadJavaU4(pPara);       // u4 attribute_length
                SkipJavaBytes(pPara, dwLength);     // u1 info[attribute_length]
            }
        }
    }

    // Finally, the attributes. This is where the signed data is

    pPara->Sign.lAttrCountOffset = pPara->Read.lFileOffset;
    pPara->Digest.fDisableDigest = TRUE;
    wAttrCount = ReadJavaU2(pPara);         // u2 attributes_count
    pPara->Digest.fDisableDigest = FALSE;

    if (fInit) {
        pPara->Sign.wAttrCount = wAttrCount;
        wSignAttrIndex = 0xFFFF;
    } else
        wSignAttrIndex = pPara->Sign.wSignAttrIndex;

    for (wAttrIndex = 0; wAttrIndex < wAttrCount; wAttrIndex++) {
        if (wAttrIndex == wSignAttrIndex)
            pPara->Digest.fDisableDigest = TRUE;

        wAttrName = ReadJavaU2(pPara);
        dwLength = ReadJavaU4(pPara);       // u4 attribute_length
        SkipJavaBytes(pPara, dwLength);     // u1 info[attribute_length]
        if (fInit && wAttrName == wSignConstPoolIndex && dwLength > 0 &&
                wSignConstPoolIndex < wConstPoolCount) {
            wSignAttrIndex = wAttrIndex;
            pPara->Sign.lSignAttrOffset =
                pPara->Read.lFileOffset - (ATTR_HDR_LENGTH + dwLength);
            pPara->Sign.dwSignAttrLength = dwLength;
        }

        pPara->Digest.fDisableDigest = FALSE;
    }

    if (fInit) {
        if (wSignAttrIndex == 0xFFFF) {
            // Didn't find an attribute for the digital_signature. Update
            // with where it will need to be added
            wSignAttrIndex = wAttrCount;
            pPara->Sign.lSignAttrOffset = pPara->Read.lFileOffset;

            // Also, force us to use a new const pool for the name of the
            // attribute
            wSignConstPoolIndex = wConstPoolCount;
            pPara->Sign.lSignConstPoolOffset = lAddConstPoolOffset;
        }

        pPara->Sign.wSignConstPoolIndex = wSignConstPoolIndex;
        pPara->Sign.wConstPoolCount = wConstPoolCount;
        pPara->Sign.wSignAttrIndex = wSignAttrIndex;
        pPara->Sign.lEndOfFileOffset = pPara->Read.lFileOffset;
    }

    // Now check if we got any hash or file errors while processing the file
    return pPara->Read.fResult;
}

//+-------------------------------------------------------------------------
//  Digest the appropriate bytes from a java file, for a digital signature.
//--------------------------------------------------------------------------
BOOL
JavaGetDigestStream(
    IN      HANDLE          FileHandle,
    IN      DWORD           DigestLevel,
    IN      DIGEST_FUNCTION DigestFunction,
    IN      DIGEST_HANDLE   DigestHandle
    )
{
    BOOL        fRet;
    JAVA_PARA   Para;
    memset( &Para.Func, 0, sizeof(Para.Func));

    assert( DigestLevel == 0);
    Para.Func.hFile = FileHandle;
    if (!ProcessJavaClassFile( &Para, TRUE))
        goto ProcessJavaClassFileTrueError;

    Para.Digest.pfnDigestData = DigestFunction;
    Para.Digest.hDigestData = DigestHandle;

    if (!ProcessJavaClassFile( &Para, FALSE))
        goto ProcessJavaClassFileFalseError;

    fRet = TRUE;
CommonReturn:
    if (Para.Func.pbSignedData)
        ShpFree( Para.Func.pbSignedData);

    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR_EX(DBG_SS,ProcessJavaClassFileTrueError)
TRACE_ERROR_EX(DBG_SS,ProcessJavaClassFileFalseError)
}


//+-------------------------------------------------------------------------
//  Add a digital signature to a java file.
//--------------------------------------------------------------------------
BOOL
JavaAddCertificate(
    IN      HANDLE              FileHandle,
    IN      LPWIN_CERTIFICATE   Certificate,
    OUT     PDWORD              Index
    )
{
    BOOL        fRet;
    JAVA_PARA   Para;
    memset( &Para.Func, 0, sizeof(Para.Func));

    Para.Func.hFile = FileHandle;
    if (!ProcessJavaClassFile( &Para, TRUE))
        goto ProcessJavaClassFileTrueError;

    if (!SetSignedDataIntoJavaClassFile(
                (HSHPSIGNFILE)&Para,
                (PBYTE)&(Certificate->bCertificate),
                Certificate->dwLength - OFFSETOF(WIN_CERTIFICATE,bCertificate)))
        goto SetSignedDataIntoJavaClassFileError;

    fRet = TRUE;
CommonReturn:
    if (Para.Func.pbSignedData)
        ShpFree( Para.Func.pbSignedData);

    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR_EX(DBG_SS,ProcessJavaClassFileTrueError)
TRACE_ERROR_EX(DBG_SS,SetSignedDataIntoJavaClassFileError)
}


//+-------------------------------------------------------------------------
//  Remove a digital signature from a java file.
//--------------------------------------------------------------------------
BOOL
JavaRemoveCertificate(
    IN      HANDLE   FileHandle,
    IN      DWORD    Index
    )
{
    return FALSE;       // BUGBUG need to fill this in
}


//+-------------------------------------------------------------------------
//  Enum the digital signatures in a java file.
//--------------------------------------------------------------------------
BOOL
JavaEnumerateCertificates(
    IN      HANDLE  FileHandle,
    IN      WORD    TypeFilter,
    OUT     PDWORD  CertificateCount,
    IN OUT  PDWORD  Indices OPTIONAL,
    IN OUT  DWORD   IndexCount  OPTIONAL
    )
{
    return FALSE;       // BUGBUG need to fill this in
}


//+-------------------------------------------------------------------------
//  Get a digital signature from a java file.
//--------------------------------------------------------------------------
static
BOOL
I_JavaGetCertificate(
    IN      HANDLE              FileHandle,
    IN      DWORD               CertificateIndex,
    OUT     LPWIN_CERTIFICATE   Certificate,
    IN OUT OPTIONAL PDWORD      RequiredLength
    )
{
    BOOL        fRet;
    JAVA_PARA   Para;
    memset( &Para.Func, 0, sizeof(Para.Func));
    BYTE       *pbSignedData = NULL;
    DWORD       cbSignedData;
    DWORD       cbCert;
    DWORD       dwError;

    if (CertificateIndex != 0)
        goto IndexNonZeroError;

    Para.Func.hFile = FileHandle;
    if (!ProcessJavaClassFile( &Para, TRUE))
        goto ProcessJavaClassFileTrueError;

    if (!GetSignedDataFromJavaClassFile(
                (HSHPSIGNFILE)&Para,
                &pbSignedData,
                &cbSignedData))
        goto GetSignedDataFromJavaClassFileError;

    cbCert = OFFSETOF(WIN_CERTIFICATE,bCertificate) + cbSignedData;
    dwError = 0;
    __try {
        if (RequiredLength) {
            // RequiredLength non-NULL only if getting cert data
            if (*RequiredLength < cbCert) {
                *RequiredLength = cbCert;
                dwError = ERROR_INSUFFICIENT_BUFFER;
            } else {
                memcpy( Certificate->bCertificate, pbSignedData, cbSignedData);
            }
        }
        if (dwError == 0) {
            Certificate->dwLength         = cbCert;
            Certificate->wRevision        = WIN_CERT_REVISION_1_0;
            Certificate->wCertificateType = WIN_CERT_TYPE_PKCS_SIGNED_DATA;
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwError = ERROR_INVALID_PARAMETER;
    }

    if (dwError) {
        SetLastError( dwError);
        fRet = FALSE;
    } else {
        fRet = TRUE;
    }
CommonReturn:
    ShpFree( Para.Func.pbSignedData);
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR_EX(DBG_SS,IndexNonZeroError)
TRACE_ERROR_EX(DBG_SS,ProcessJavaClassFileTrueError)
TRACE_ERROR_EX(DBG_SS,GetSignedDataFromJavaClassFileError)
}


//+-------------------------------------------------------------------------
//  Get a digital signature from a java file.
//--------------------------------------------------------------------------
BOOL
JavaGetCertificateData(
    IN      HANDLE              FileHandle,
    IN      DWORD               CertificateIndex,
    OUT     LPWIN_CERTIFICATE   Certificate,
    IN OUT  PDWORD              RequiredLength
    )
{
    BOOL        fRet;

    if (RequiredLength == NULL)
        goto RequiredLengthNullError;

    fRet = I_JavaGetCertificate(
                    FileHandle,
                    CertificateIndex,
                    Certificate,
                    RequiredLength
                    );

CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(RequiredLengthNullError, ERROR_INVALID_PARAMETER)
}


//+-------------------------------------------------------------------------
//  Get the header of a digital signature from a java file.
//--------------------------------------------------------------------------
BOOL
JavaGetCertificateHeader(
    IN      HANDLE              FileHandle,
    IN      DWORD               CertificateIndex,
    IN OUT  LPWIN_CERTIFICATE   Certificateheader
    )
{
    return I_JavaGetCertificate(
                    FileHandle,
                    CertificateIndex,
                    Certificateheader,
                    NULL
                    );
}

//+-------------------------------------------------------------------------
//  Seeks and writes bytes to file
//--------------------------------------------------------------------------
BOOL
SeekAndWriteFile(
    IN HANDLE hFile,
    IN DWORD lFileOffset,
    IN BYTE *pb,
    IN DWORD cb
    )
{
    DWORD cbWritten;

    if (0xFFFFFFFF == SetFilePointer(
            hFile,
            lFileOffset,
            NULL,           // lpDistanceToMoveHigh
            FILE_BEGIN))
        return FALSE;
    if (!WriteFile(hFile, pb, cb, &cbWritten, NULL) || cbWritten != cb)
        return FALSE;

    return TRUE;
}

//+-------------------------------------------------------------------------
//  Seeks and reads bytes from file
//--------------------------------------------------------------------------
BOOL
SeekAndReadFile(
    IN HANDLE hFile,
    IN DWORD lFileOffset,
    OUT BYTE *pb,
    IN DWORD cb
    )
{
    DWORD cbRead;

    if (0xFFFFFFFF == SetFilePointer(
            hFile,
            lFileOffset,
            NULL,           // lpDistanceToMoveHigh
            FILE_BEGIN))
        return FALSE;
    if (!ReadFile(hFile, pb, cb, &cbRead, NULL) || cbRead != cb)
        return FALSE;

    return TRUE;
}




