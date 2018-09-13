//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       mscat32.cpp
//
//  Contents:   Microsoft Internet Security Catalog Utilities
//
//  Functions:  DllMain
//              DllRegisterServer
//              DllUnregisterServer
//
//              *** local functions ***
//              CatalogNew
//              CatalogFreeMember
//              CatalogFreeAttribute
//              CatalogCheckForDuplicateMember
//
//  History:    25-Apr-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"
#include    "mscat32.h"

CRITICAL_SECTION    MSCAT_CriticalSection;

HINSTANCE           hInst;

//////////////////////////////////////////////////////////////////////////////////////
//
// standard DLL exports ...
//
//

BOOL WINAPI mscat32DllMain(HANDLE hInstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
                hInst = (HINSTANCE)hInstDLL;

                InitializeCriticalSection(&MSCAT_CriticalSection);
                break;

        case DLL_PROCESS_DETACH:
                DeleteCriticalSection(&MSCAT_CriticalSection);
                break;
    }

    return(CatAdminDllMain(hInstDLL, fdwReason, lpvReserved));
}

STDAPI mscat32DllRegisterServer(void)
{
    return(S_OK);
}


STDAPI mscat32DllUnregisterServer(void)
{
    return(S_OK);
}

//////////////////////////////////////////////////////////////////////////////////////
//
//  local utility functions
//
//

void *CatalogNew(DWORD cbSize)
{
    void    *pvRet;

    pvRet = (void *)new BYTE[cbSize];

    if (!(pvRet))
    {
        assert(pvRet);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }
    return(pvRet);
}

BOOL CatalogFreeAttribute(CRYPTCATATTRIBUTE *pCatAttr)
{
    if (!(pCatAttr) ||
        (pCatAttr->cbStruct != sizeof(CRYPTCATATTRIBUTE)))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    DELETE_OBJECT(pCatAttr->pwszReferenceTag);
    DELETE_OBJECT(pCatAttr->pbValue);
    pCatAttr->cbValue               = 0;
    pCatAttr->dwAttrTypeAndAction   = 0;
    pCatAttr->dwReserved            = 0;

    return(TRUE);
}

BOOL CatalogFreeMember(CRYPTCATMEMBER *pCatMember)
{
    if (!(pCatMember) ||
        (pCatMember->cbStruct != sizeof(CRYPTCATMEMBER)))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    //
    //  reference tag
    //
    DELETE_OBJECT(pCatMember->pwszReferenceTag);

    //
    //  file name
    //
    DELETE_OBJECT(pCatMember->pwszFileName);

    //
    //  free indirect data
    //
    if (pCatMember->pIndirectData)
    {
            // Data.pszObjId
        DELETE_OBJECT(pCatMember->pIndirectData->Data.pszObjId);

            // Data.Value.pbData
        DELETE_OBJECT(pCatMember->pIndirectData->Data.Value.pbData);

            // DigestAlgorithm.pszObjId
        DELETE_OBJECT(pCatMember->pIndirectData->DigestAlgorithm.pszObjId);

            // Digest.pbData
        DELETE_OBJECT(pCatMember->pIndirectData->Digest.pbData);

            // the structure itself!
        DELETE_OBJECT(pCatMember->pIndirectData);
    }

    //
    //  free encoded indirect data
    //
    DELETE_OBJECT(pCatMember->sEncodedIndirectData.pbData);
    pCatMember->sEncodedIndirectData.cbData = 0;

    //
    //  free encoded member info
    //
    DELETE_OBJECT(pCatMember->sEncodedMemberInfo.pbData);
    pCatMember->sEncodedMemberInfo.cbData = 0;


    //
    //  free attribute data
    //
    if (pCatMember->hReserved)
    {
        Stack_  *ps;
        DWORD               cStack;
        CRYPTCATATTRIBUTE   *pAttr;

        ps = (Stack_ *)pCatMember->hReserved;

        cStack = 0;

        while (pAttr = (CRYPTCATATTRIBUTE *)ps->Get(cStack))
        {
            CatalogFreeAttribute(pAttr);

            cStack++;
        }

        DELETE_OBJECT(ps);

        pCatMember->hReserved = NULL;
    }

    return(TRUE);
}

BOOL CatalogCheckForDuplicateMember(Stack_ *pMembers, WCHAR *pwszReferenceTag)
{
    CRYPTCATMEMBER  *pMem;
    DWORD           iCur;
    DWORD           ccRT;

    ccRT    = wcslen(pwszReferenceTag);
    iCur    = 0;

    while (iCur < pMembers->Count())
    {
        pMem = (CRYPTCATMEMBER *)pMembers->Get(iCur);

        if (pMem)
        {
            if (ccRT == (DWORD)wcslen(pMem->pwszReferenceTag))
            {
                if (wcscmp(pwszReferenceTag, pMem->pwszReferenceTag) == 0)
                {
                    return(TRUE);
                }
            }
        }

        //
        //  increment our index!
        //
        iCur++;
    }

    return(FALSE);
}

WCHAR aHexDigit[] = {
           L'0',
           L'1',
           L'2',
           L'3',
           L'4',
           L'5',
           L'6',
           L'7',
           L'8',
           L'9',
           L'A',
           L'B',
           L'C',
           L'D',
           L'E',
           L'F'
           };

VOID ByteToWHex (IN LPBYTE pbDigest, IN DWORD iByte, OUT LPWSTR pwszHashTag)
{
    DWORD iTag;
    DWORD iHexDigit1;
    DWORD iHexDigit2;

    iTag = iByte * 2;
    iHexDigit1 = ( pbDigest[ iByte ] & 0xF0 ) >> 4;
    iHexDigit2 = ( pbDigest[ iByte ] & 0x0F );

    pwszHashTag[ iTag ] = aHexDigit[ iHexDigit1 ];
    pwszHashTag[ iTag + 1 ] = aHexDigit[ iHexDigit2 ];
}

BOOL MsCatConstructHashTag (IN DWORD cbDigest, IN LPBYTE pbDigest, OUT LPWSTR* ppwszHashTag)
{
    DWORD  cwTag;
    LPWSTR pwszHashTag;
    DWORD  cCount;

    cwTag = ( ( cbDigest * 2 ) + 1 );
    pwszHashTag = (LPWSTR)CatalogNew( cwTag * sizeof( WCHAR ) );
    if ( pwszHashTag == NULL )
    {
        SetLastError( E_OUTOFMEMORY );
        return( FALSE );
    }

    for ( cCount = 0; cCount < cbDigest; cCount++ )
    {
        ByteToWHex( pbDigest, cCount, pwszHashTag );
    }

    pwszHashTag[ cwTag - 1 ] = L'\0';

    *ppwszHashTag = pwszHashTag;

    return( TRUE );
}

VOID MsCatFreeHashTag (IN LPWSTR pwszHashTag)
{
    DELETE_OBJECT(pwszHashTag);
}
