//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       updcat.cpp
//
//  Contents:   Update Catalog Entry
//
//  History:    02-Sep-98    kirtd    Created
//
//----------------------------------------------------------------------------
#include <windows.h>
#include <assert.h>
#include "wincrypt.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>
#include <mscat.h>
#include <mssip.h>
#include <sipguids.h>
#include <wintrust.h>

// Prototypes

BOOL AddFileToCatalog (IN HANDLE hCatalog, IN LPWSTR pwszFileName);
BOOL RemoveHashFromCatalog(IN LPWSTR pwszCatalogFile, IN LPSTR pszHash);
extern "C" BOOL MsCatConstructHashTag (IN DWORD cbDigest, IN LPBYTE pbDigest, OUT LPWSTR* ppwszHashTag);
extern "C" VOID MsCatFreeHashTag (IN LPWSTR pwszHashTag);
//+---------------------------------------------------------------------------
//
//  Function:   Usage
//
//  Synopsis:   prints the usage statement
//
//----------------------------------------------------------------------------
static void Usage(void)
{
    printf("Usage: updcat <Catalog File> [-a <FileName>]\n");
    printf("Usage: updcat <Catalog File> [-d <Hash>]\n");
    printf("Usage: updcat <Catalog File> [-r <Hash> <FileName>]\n");
    printf("       -a, add the file by hash to the catalog\n");
    printf("       -d, delete the hash from the catalog\n");
    printf("       -r, replace the hash in the catalog with the hash of the file\n");
}

//+---------------------------------------------------------------------------
//
//  Function:   main
//
//  Synopsis:   main program entry point
//
//----------------------------------------------------------------------------
int _cdecl main(int argc, char * argv[])
{
    BOOL   fResult = TRUE;
    LPSTR  pszCatalogFile = NULL;
    LPWSTR pwszCatalogFile = NULL;
    LPSTR  pszFileName = NULL;
    LPSTR  pszHash = NULL;
    LPWSTR pwszFileName = NULL;
    BOOL   fAddEntry = FALSE;
    DWORD  cch = 0;
    HANDLE hCatalog = NULL;
    BOOL   fOptionChosen = FALSE; 

    if ( argc < 2 )
    {
        Usage();
        return( 1 );
    }

    argv++;
    argc--;

    printf( "command line: %s\n", GetCommandLineA() );

    pszCatalogFile = argv[0];
    cch = strlen( pszCatalogFile );

    while ( --argc > 0 )
    {
        if ( **++argv == '-' )
        {
            switch( argv[0][1] )
            {
            case 'a':
            case 'A':

                if ( argc < 2 )
                {
                    Usage();
                    return( 1 );
                }

                pszFileName = argv[1];
                fAddEntry = TRUE;
                break;

            case 'd':
            case 'D':
                
                if ( argc < 2 )
                {
                    Usage();
                    return( 1 );
                }

                pszHash = argv[1];
                break;

            case 'r':
            case 'R':
                
                if ( argc < 2 )
                {
                    Usage();
                    return( 1 );
                }

                pszHash = argv[1];
                fAddEntry = TRUE;
                pszFileName = argv[2];
                break;

            default:
                Usage();
                return -1;
            }
            
            fOptionChosen = TRUE;
            argc -= 1;
            argv++;
        }
    }

    pwszCatalogFile = new WCHAR [ cch + 1 ];
    if ( pwszCatalogFile != NULL )
    {
        if ( MultiByteToWideChar(
                  CP_ACP,
                  0,
                  pszCatalogFile,
                  -1,
                  pwszCatalogFile,
                  cch + 1
                  ) == 0 )
        {
            delete pwszCatalogFile;
            return( 1 );
        }
    }

    if (!fOptionChosen)
    {
        Usage();
        delete pwszCatalogFile;
        return -1;
    }

    if (pszFileName != NULL)
    {
        cch = strlen( pszFileName );

        pwszFileName = new WCHAR [ cch + 1 ];
        if ( pwszFileName != NULL )
        {
            if ( MultiByteToWideChar(
                      CP_ACP,
                      0,
                      pszFileName,
                      -1,
                      pwszFileName,
                      cch + 1
                      ) == 0 )
            {
                delete pwszCatalogFile;
                delete pwszFileName;
                return( 1 );
            }
        }
    }

    if ( pszHash != NULL )
    {
        fResult = RemoveHashFromCatalog(pwszCatalogFile, pszHash);

        if ( fResult == FALSE )
        {
            printf("Error removing <%s> from catalog <%s>\n", pszHash, pszCatalogFile);
        }
    }

    //
    // If there hasn't been any errors, and we are adding a hash
    //
    if (( fResult == TRUE ) && ( fAddEntry == TRUE ))
    {
        hCatalog = CryptCATOpen(
                        pwszCatalogFile,
                        CRYPTCAT_OPEN_ALWAYS,
                        NULL,
                        0x00000001,
                        0x00010001
                        );

        if ( hCatalog == NULL )
        {
            fResult = FALSE;
        }

        if ( fResult == TRUE )
        {
            fResult = AddFileToCatalog( hCatalog, pwszFileName );
            CryptCATClose( hCatalog );
        }

        if ( fResult == FALSE )
        {
            printf("Error adding <%s> to catalog <%s>\n", pszFileName, pszCatalogFile);
        }
    }

    return( !fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   AddFileToCatalog
//
//  Synopsis:   add a file as an entry to the catalog.  The tag will be the
//              hash
//
//----------------------------------------------------------------------------
BOOL AddFileToCatalog (IN HANDLE hCatalog, IN LPWSTR pwszFileName)
{
    BOOL               fResult;
    GUID               FlatSubject = CRYPT_SUBJTYPE_FLAT_IMAGE;
    GUID               SubjectType;
    SIP_SUBJECTINFO    SubjectInfo;
    SIP_DISPATCH_INFO  DispatchInfo;
    DWORD              cbIndirectData;
    SIP_INDIRECT_DATA* pIndirectData;
    CRYPTCATSTORE*     pCatStore = CryptCATStoreFromHandle( hCatalog );
    LPWSTR             pwszHashTag = NULL;

    memset( &SubjectInfo, 0, sizeof( SubjectInfo ) );
    memset( &DispatchInfo, 0, sizeof( DispatchInfo ) );

    if ( CryptSIPRetrieveSubjectGuid(
              pwszFileName,
              NULL,
              &SubjectType
              ) == FALSE )
    {
        memcpy( &SubjectType, &FlatSubject, sizeof( GUID ) );
    }

    if ( CryptSIPLoad( &SubjectType, 0, &DispatchInfo ) == FALSE )
    {
        return( FALSE );
    }

    // BUGBUG: Some of this subject info stuff should be configurable but
    //         since the CDF API does not allow it we won't worry about it
    //         yet.  Ah, what a wonderfully $#@%^&*! API.
    SubjectInfo.cbSize = sizeof( SubjectInfo );
    SubjectInfo.hProv = pCatStore->hProv;
    SubjectInfo.DigestAlgorithm.pszObjId = (char *)CertAlgIdToOID( CALG_SHA1 );

    SubjectInfo.dwFlags = SPC_INC_PE_RESOURCES_FLAG |
                          SPC_INC_PE_IMPORT_ADDR_TABLE_FLAG |
                          MSSIP_FLAGS_PROHIBIT_RESIZE_ON_CREATE;

    SubjectInfo.dwEncodingType = pCatStore->dwEncodingType;
    SubjectInfo.pgSubjectType = &SubjectType;
    SubjectInfo.pwsFileName = pwszFileName;

    fResult = DispatchInfo.pfCreate( &SubjectInfo, &cbIndirectData, NULL );

    if ( fResult == TRUE )
    {
        pIndirectData = (SIP_INDIRECT_DATA *)new BYTE [ cbIndirectData ];
        if ( pIndirectData != NULL )
        {
            fResult = DispatchInfo.pfCreate(
                                     &SubjectInfo,
                                     &cbIndirectData,
                                     pIndirectData
                                     );
        }
        else
        {
            SetLastError( E_OUTOFMEMORY );
            fResult = FALSE;
        }
    }

    if ( fResult == TRUE )
    {
        fResult = MsCatConstructHashTag(
                       pIndirectData->Digest.cbData,
                       pIndirectData->Digest.pbData,
                       &pwszHashTag
                       );
    }

    if ( fResult == TRUE )
    {
        CRYPTCATMEMBER* pMember;

        pMember = CryptCATPutMemberInfo(
                       hCatalog,
                       pwszFileName,
                       pwszHashTag,
                       &SubjectType,
                       SubjectInfo.dwIntVersion,
                       cbIndirectData,
                       (LPBYTE)pIndirectData
                       );

        if ( pMember != NULL )
        {
            fResult = CryptCATPersistStore( hCatalog );
        }
        else
        {
            fResult = FALSE;
        }
    }

    if ( pwszHashTag != NULL )
    {
        MsCatFreeHashTag( pwszHashTag );
    }

    delete (LPBYTE)pIndirectData;

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   RemoveHashFromCatalog
//
//  Synopsis:   removes a hash entry from the catalog.  
//
//----------------------------------------------------------------------------
BOOL
RemoveHashFromCatalog(IN LPWSTR pwszCatalogFile, IN LPSTR pszHash)
{
    BOOL            fRet = TRUE;
    LPSTR           pChar = NULL;
    int             i, j;
    DWORD           dwContentType;
    PCTL_CONTEXT    pCTLContext = NULL;
    CTL_CONTEXT     CTLContext;
    CTL_INFO        CTLInfo;
    DWORD           cbEncodedCTL = 0;
    BYTE            *pbEncodedCTL = NULL;
    DWORD           cbWritten = 0;
    HANDLE          hFile = INVALID_HANDLE_VALUE;
    DWORD           cch = 0;
    LPWSTR          pwszHash = NULL;
    BOOL            fHashFound = FALSE;

    CMSG_SIGNED_ENCODE_INFO signedInfo;
    memset(&signedInfo, 0, sizeof(signedInfo));
    signedInfo.cbSize = sizeof(signedInfo);


    CTLInfo.rgCTLEntry = NULL;

    cch = strlen( pszHash );

    pwszHash = new WCHAR [ cch + 1 ];
    if ( pwszHash == NULL )
    {
       goto ErrorReturn;   
    }
    if ( MultiByteToWideChar(
                  CP_ACP,
                  0,
                  pszHash,
                  -1,
                  pwszHash,
                  cch + 1
                  ) == 0 )
    {
        goto ErrorReturn;
    }

    //
    // Get rid of all the ' ' chars
    //
    i = 0;
    j = 0;
    for (i=0; i<(int)wcslen(pwszHash); i++)
    {
        if (pwszHash[i] != ' ')
        {
            pwszHash[j++] = pwszHash[i];
        }
    }
    pwszHash[j] = '\0';
        
    //
    // Open the cat file as a CTL
    //
    if (!CryptQueryObject(
            CERT_QUERY_OBJECT_FILE,
            pwszCatalogFile,
            CERT_QUERY_CONTENT_FLAG_CTL,
            CERT_QUERY_FORMAT_FLAG_BINARY,
            0, //flags
            NULL,
            &dwContentType,
            NULL,
            NULL,
            NULL,
            (const void **) &pCTLContext))
    {
        goto ErrorReturn;
    }

    if (dwContentType != CERT_QUERY_CONTENT_CTL)
    {
        goto ErrorReturn;
    }

    //
    // Create another CTL context just like pCTLContext
    //
    CTLInfo = *(pCTLContext->pCtlInfo);
    CTLInfo.rgCTLEntry = (PCTL_ENTRY) new CTL_ENTRY[pCTLContext->pCtlInfo->cCTLEntry];

    if (CTLInfo.rgCTLEntry == NULL)
    {
        goto ErrorReturn;
    }

    //
    // Loop through all the ctl entries and remove the entry
    // that corresponds to the hash given
    //
    CTLInfo.cCTLEntry = 0;
    for (i=0; i<(int)pCTLContext->pCtlInfo->cCTLEntry; i++)
    {
        if (wcscmp(
                (LPWSTR) pCTLContext->pCtlInfo->rgCTLEntry[i].SubjectIdentifier.pbData, 
                pwszHash) != 0)
        {
            CTLInfo.rgCTLEntry[CTLInfo.cCTLEntry++] = pCTLContext->pCtlInfo->rgCTLEntry[i];
        }
        else
        {
            fHashFound = TRUE;
        }
    }

    if (!fHashFound)
    {
        printf("<%S> not found in <%S>\n", pwszHash, pwszCatalogFile);
        goto ErrorReturn;
    }

    //
    // now save the CTL which is exactly the same as the previous one,
    // except it doesn't doesn't have the hash being removed, back to
    // the original filename
    //
    if (!CryptMsgEncodeAndSignCTL(
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                &CTLInfo,
                &signedInfo,
                0,
                NULL,
                &cbEncodedCTL))
    {
        goto ErrorReturn;
    }

    if (NULL == (pbEncodedCTL = new BYTE[cbEncodedCTL]))
    {
        goto ErrorReturn;
    }

    if (!CryptMsgEncodeAndSignCTL(
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                &CTLInfo,
                &signedInfo,
                0,
                pbEncodedCTL,
                &cbEncodedCTL))
    {
        goto ErrorReturn;
    }



    if (INVALID_HANDLE_VALUE == (hFile = CreateFileW(
                                            pwszCatalogFile,
                                            GENERIC_READ | GENERIC_WRITE,
                                            0,
                                            NULL,
                                            CREATE_ALWAYS,
                                            FILE_ATTRIBUTE_NORMAL,
                                            NULL)))
    {
        goto ErrorReturn;
    }

    if (!WriteFile(
            hFile,
            pbEncodedCTL,
            cbEncodedCTL,
            &cbWritten,
            NULL))
    {
        printf("WriteFile of <%S> failed with %x\n", pwszCatalogFile, GetLastError());
        goto ErrorReturn;
    }

    if (cbWritten != cbEncodedCTL)
    {
        goto ErrorReturn;
    }

    CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;

CommonReturn:
    if (pwszHash != NULL)
    {
        delete (pwszHash);
    }

    if (pCTLContext != NULL)
    {
        CertFreeCTLContext(pCTLContext);
    }

    if (CTLInfo.rgCTLEntry != NULL)
    {
        delete (CTLInfo.rgCTLEntry);
    }

    if (pbEncodedCTL != NULL)
    {
        delete (pbEncodedCTL);
    }

    if (hFile != INVALID_HANDLE_VALUE)
    {
        if (!CloseHandle(hFile))
        {
            fRet = FALSE;
        }
    }

    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
}

