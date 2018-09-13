#include "generic.h"

#pragma code_seg(_ICM2SEG)
#pragma optimize("",off)

#define TempBfSize 128
#define LINELENG   128
#ifdef ICMDLL
#define ICM2SEG
#endif

static  char    ICM2SEG NewLine[]       = "\n" ;
static  char    ICM2SEG ASCII85DecodeBegine[] = "<~";
static  char    ICM2SEG ASCII85DecodeEnd[] = "~> cvx exec ";

//******************************************************************
//   Local functions to deal with output to the memory buffer

static  SINT    CPLastError;

BOOL    EXTERN     SetCPLastError(SINT LastError)
{
    CPLastError = LastError;
    return(TRUE);
}

SINT    EXTERN     GetCPLastError()
{
    return(CPLastError);
}

BOOL    EXTERN MemAlloc(SINT Size, HGLOBAL FAR *hMemory, LPMEMPTR lpMH)
{
    HGLOBAL hMem;
    LPVOID  lpMem;

    *hMemory = 0;
    if(lpMH == NULL )
    {
        SetCPLastError(CP_NULL_POINTER_ERR);
        return(FALSE);
    }

    hMem = GlobalAlloc(GHND, Size) ;
    if(hMem == 0 )
    {
        SetCPLastError(CP_MEMORY_ALLOC_ERR);
        return(FALSE);
    }

    lpMem = GlobalLock(hMem);
    if(lpMem == NULL )
    {
        GlobalFree(hMem);
        SetCPLastError(CP_MEMORY_ALLOC_ERR);
        return(FALSE);
    }
    *lpMH = (MEMPTR)lpMem ;
    *hMemory = hMem;
    return (TRUE);
}

BOOL    EXTERN MemFree(HGLOBAL hMem)
{
    if(hMem == NULL )
    {
        SetCPLastError(CP_NULL_POINTER_ERR);
        return(FALSE);
    }

    GlobalUnlock(hMem);
    GlobalFree(hMem) ;
    return(TRUE);
}

//******************************************************************
//      Low-level Profile functions - used for individual tag access
//******************************************************************
#ifndef ICMDLL
BOOL    EXTERN LoadCP(LPCSTR filename, HGLOBAL FAR *phMem, LPCHANDLE lpCP)
{
    icHeader    CPHeader;
    HFILE       hFile;
    SINT        Res, CPSize;
    MEMPTR      mpCP;
    DWORD       dwFileSize;

    *phMem = 0;
    if (lpCP == NULL)
    {
        SetCPLastError(CP_NULL_POINTER_ERR);
        return(FALSE);
    }

    hFile = _lopen(filename, READ );
    if( hFile == HFILE_ERROR )
    {
        SetCPLastError(CP_FILE_OPEN_ERR);
        return(FALSE);
    }

    dwFileSize = _lseek(hFile, 0, SEEK_END);
    _lseek(hFile, 0, SEEK_SET);

    Res = _lread(hFile, (LPVOID) &CPHeader, sizeof(CPHeader));
    if( (Res == HFILE_ERROR) ||
        (Res != sizeof(CPHeader)) )
    {
        _lclose(hFile);
        SetCPLastError(CP_FILE_READ_ERR);
        return(FALSE);
    }
    
    // Make the initial check for validity of the profile
    if( SigtoCSIG(CPHeader.magic) != icMagicNumber )
    {
        _lclose(hFile);
        SetCPLastError(CP_FORMAT_ERR);
        return(FALSE);
    }

    if (dwFileSize == 0xFFFFFFFF)
        CPSize = ui32toSINT(CPHeader.size);
    else
        CPSize = (SINT)dwFileSize;
    
    if( MemAlloc(CPSize, phMem, (LPMEMPTR) &mpCP) )
    {

        *lpCP = (CHANDLE) mpCP;  // Put the memory pointer as  handle
        // Read profile into  memory
         _lseek(hFile, 0L, SEEK_SET);

        while(CPSize)
        {
            Res = _lread(hFile, (LPVOID) mpCP, 4096);
            if (Res == HFILE_ERROR) 
            {
                _lclose(hFile);
                SetCPLastError(CP_FILE_READ_ERR);
                return(FALSE);
            }
            mpCP    += Res;
            CPSize  -= Res;
        }
    }else
    {
        *phMem = 0;
        _lclose(hFile);
        return(FALSE);
    }
    _lclose(hFile);
    return (TRUE);
}

#else
//******************************************************************
//      Low-level Profile functions - used for individual tag access
//******************************************************************
BOOL    EXTERN LoadCP32(LPCSTR filename, HGLOBAL *phMem, LPCHANDLE lpCP)
{
    icHeader    CPHeader;
    HANDLE      hFile;
    SINT        Res, CPSize;
    MEMPTR      mpCP;
    BOOL        Success;
    DWORD       dwFileSize;

    if (lpCP == NULL)
    {
        SetCPLastError(CP_NULL_POINTER_ERR);
        return(FALSE);
    }
    hFile = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if( hFile == INVALID_HANDLE_VALUE )
    {
        SetCPLastError(CP_FILE_OPEN_ERR);
        return(FALSE);
    }
    
    dwFileSize = GetFileSize(hFile, NULL);

    Success = ReadFile(hFile, (LPVOID) &CPHeader, sizeof(CPHeader), &Res, NULL);
    if( ( !Success ) || (Res != sizeof(CPHeader)) )
    {
        CloseHandle(hFile);
        SetCPLastError(CP_FILE_READ_ERR);
        return(FALSE);
    }

    // Make the initial check for validity of the profile
    if( SigtoCSIG(CPHeader.magic) != icMagicNumber )
    {
        CloseHandle(hFile);
        SetCPLastError(CP_FORMAT_ERR);
        return(FALSE);
    }

    if (dwFileSize == 0xFFFFFFFF)
        CPSize = ui32toSINT(CPHeader.size);
    else
        CPSize = (SINT)dwFileSize;

    if( MemAlloc(CPSize, phMem, (LPMEMPTR) &mpCP) )
    {

        *lpCP = (CHANDLE) mpCP;  // Put the memory pointer as  handle
        // Read profile into  memory
        SetFilePointer(hFile, 0L, NULL, FILE_BEGIN);
        Success = ReadFile(hFile, (LPVOID) (LPVOID)mpCP, CPSize, &Res, NULL);
        if (!Success) 
        {
            CloseHandle(hFile);
            SetCPLastError(CP_FILE_READ_ERR);
            return(FALSE);
        }
    }else
    {
        CloseHandle(hFile);
        return(FALSE);
    }
    CloseHandle(hFile);
    return (TRUE);
}
#endif

BOOL    EXTERN FreeCP(HGLOBAL hMem)
{
    return( MemFree(hMem) );
}


BOOL    EXTERN GetCPElementCount(CHANDLE CP, LPSINT lpCount)
{
    lpcpTagList lpTL;
    if (lpCount == NULL)
    {
        SetCPLastError(CP_NULL_POINTER_ERR);
        return(FALSE);
    }
    lpTL = (lpcpTagList) &(((lpcpProfile)CP)->count);
    *lpCount = ui32toSINT(lpTL->count);
    return(TRUE);
}


BOOL    EXTERN GetCPElementInfo(CHANDLE CP, SINT Index,
                                LPMEMPTR lpTagData, LPMEMPTR lpElemData)
{
    SINT    Count;
    lpcpTagList lpTL;

    if ( (lpTagData == NULL) || (lpElemData == NULL) )
    {
        SetCPLastError(CP_NULL_POINTER_ERR);
        return(FALSE);
    }

    lpTL = (lpcpTagList) &(((lpcpProfile)CP)->count);
    Count   = ui32toSINT(lpTL->count);
    if ( Count <= Index )
    {
        SetCPLastError(CP_OUT_OF_RANGE_ERR);
        return(FALSE);
    }
    *lpTagData      = ((MEMPTR) &(lpTL->tags[0])) + (Index * sizeof(icTag)) ;
    *lpElemData     = ((MEMPTR) CP) +
                                ui32toSINT( ((lpcpTag)*lpTagData)->offset);
    return(TRUE);
}


/* Checks if the profile has all required fields  for
    this specific type of the color profile             */
BOOL    EXTERN ValidateCP(CHANDLE CP)
{
    BOOL    Result;
    CSIG    ProfileClass;

    if(GetCPClass(CP, (LPCSIG) &ProfileClass) )
    {
        // All profiles must have a ProfileDescription and
        //  a Copyright  tags.

        if( !DoesCPTagExist(CP, icSigProfileDescriptionTag) || 
            !DoesCPTagExist(CP, icSigCopyrightTag ) )
        {
            SetCPLastError(CP_NOT_FOUND_ERR);
            return(FALSE);
        }

        // All profiles, except Device-link, must have a mediaWhitePoint Tag
        switch( ProfileClass )
        {
            case     icSigLinkClass :        /* 'link' */
                if( DoesCPTagExist(CP, icSigAToB0Tag) &&
                    DoesCPTagExist(CP, icSigProfileSequenceDescTag)
                  )
                {
                    Result = TRUE;
                }else
                {
                    Result = FALSE;
                }
                break;

            case     icSigInputClass:       /* 'scnr' */
                if( DoesCPTagExist(CP, icSigGrayTRCTag) ||
                    DoesCPTagExist(CP, icSigAToB0Tag)      )
                {
                    Result = TRUE;
                }else if( DoesCPTagExist(CP, icSigGreenColorantTag) )
                {
                    if( DoesCPTagExist(CP, icSigRedColorantTag) &&
                        DoesCPTagExist(CP, icSigBlueColorantTag) &&
                        DoesCPTagExist(CP, icSigRedTRCTag) &&
                        DoesCPTagExist(CP, icSigGreenTRCTag) &&
                        DoesCPTagExist(CP, icSigBlueTRCTag)
                      )
                    {
                        Result = TRUE;
                    }else
                    {
                        Result = FALSE;
                    }
                }else
                {
                    Result = FALSE;
                }
                Result  &= DoesCPTagExist(CP, icSigMediaWhitePointTag);
                break;

            case     icSigDisplayClass:     /* 'mntr' */
                if( DoesCPTagExist(CP, icSigGrayTRCTag) )
                {
                    Result = TRUE;
                }else if( DoesCPTagExist(CP, icSigGreenColorantTag) )
                {
                    if( DoesCPTagExist(CP, icSigRedColorantTag) &&
                        DoesCPTagExist(CP, icSigBlueColorantTag) &&
                        DoesCPTagExist(CP, icSigRedTRCTag) &&
                        DoesCPTagExist(CP, icSigGreenTRCTag) &&
                        DoesCPTagExist(CP, icSigBlueTRCTag)
                      )
                    {
                        Result = TRUE;
                    }else
                    {
                        Result = FALSE;
                    }
                }else
                {
                    Result = FALSE;
                }
                Result  &= DoesCPTagExist(CP, icSigMediaWhitePointTag);
                break;

            case     icSigOutputClass:      /* 'prtr' */
                if( DoesCPTagExist(CP, icSigGrayTRCTag) )
                {
                    Result = TRUE;
                }else if( DoesCPTagExist(CP, icSigAToB0Tag) &&
                          DoesCPTagExist(CP, icSigAToB1Tag) &&
                          DoesCPTagExist(CP, icSigAToB2Tag) &&
                          DoesCPTagExist(CP, icSigBToA0Tag) &&
                          DoesCPTagExist(CP, icSigBToA1Tag) &&
                          DoesCPTagExist(CP, icSigBToA2Tag) &&
                          DoesCPTagExist(CP, icSigGamutTag) 
                        )
                {
                    Result = TRUE;
                }else
                {
                    Result = FALSE;
                }
                Result  &= DoesCPTagExist(CP, icSigMediaWhitePointTag);
                break;

            case     icSigAbstractClass:    /* 'abst' */
                if( DoesCPTagExist(CP, icSigAToB0Tag) )
                {
                    Result = TRUE;
                }else
                {
                    Result = FALSE;
                }
                Result  &= DoesCPTagExist(CP, icSigMediaWhitePointTag);
                break;

            case     icSigColorSpaceClass:  /* 'spac' */
                if( DoesCPTagExist(CP, icSigAToB0Tag) &&
                    DoesCPTagExist(CP, icSigBToA0Tag)
                  )
                {
                    Result = TRUE;
                }else
                {
                    Result = FALSE;
                }
                Result  &= DoesCPTagExist(CP, icSigMediaWhitePointTag);
                break;

            default:
                Result = FALSE;
                break;
        }
    }else
    {
        return(FALSE);
    }
    if( Result == FALSE )
    {
        SetCPLastError(CP_NOT_FOUND_ERR);
    }
    return(Result);
}

BOOL    EXTERN DoesCPTagExist(CHANDLE CP, CSIG CPTag)
{
    SINT    Count;
    MEMPTR   Data;
    lpcpTagList lpTL;

    lpTL = (lpcpTagList) &(((lpcpProfile)CP)->count);
    Count   = ui32toSINT(lpTL->count);
    Data    = (MEMPTR)  &(lpTL->tags[0]) ; 
    while ( Count-- )
    {
        if(  SigtoCSIG( ((lpcpTag)Data)->sig) == CPTag )
        {
            return(TRUE);
        }else
        {
            Data    += sizeof(icTag);   // Bump pointer to the next tag
        }
    }
    return(FALSE);
}


BOOL    EXTERN GetCPTagIndex(CHANDLE CP, CSIG CPTag, LPSINT lpIndex)
{
    SINT    Count;
    MEMPTR   Data;
    SINT    i;
    lpcpTagList lpTL;

    if (lpIndex == NULL)
    {
        SetCPLastError(CP_NULL_POINTER_ERR);
        return(FALSE);
    }

    lpTL = (lpcpTagList) &(((lpcpProfile)CP)->count);
    Count   = ui32toSINT(lpTL->count);
    Data    = (MEMPTR)  &(lpTL->tags[0]) ;

    for (i = 0; i < Count; i++ )
    {
        if(  SigtoCSIG( ((lpcpTag)Data)->sig) == CPTag )
        {
            *lpIndex = i;
            return(TRUE);
        }else
        {
            Data    += sizeof(icTag);   // Bump pointer to the next tag
        }
    }

    SetCPLastError(CP_NOT_FOUND_ERR);
    return(FALSE);
}



BOOL    EXTERN GetCPTagSig(CHANDLE CP, SINT Index, LPCSIG lpCPTag)
{
    MEMPTR   TagData, ElemData;
    if (lpCPTag == NULL)
    {
        SetCPLastError(CP_NULL_POINTER_ERR);
        return(FALSE);
    }
    if ( GetCPElementInfo(CP, Index, (LPMEMPTR) &TagData,
            (LPMEMPTR) &ElemData) )
    {
        *lpCPTag    = SigtoCSIG( ((lpcpTag)TagData)->sig ) ;
    }else
    {
        return(FALSE);
    }
    return(TRUE);
}

//***************************************************************
//      Function applicable to the elements
//
//***************************************************************

BOOL    EXTERN GetCPElementType(CHANDLE CP, SINT Index, LPCSIG lpCSig)
{
    MEMPTR   TagData, ElemData;
    if ( GetCPElementInfo(CP, Index, (LPMEMPTR) &TagData,
            (LPMEMPTR) &ElemData) )
    {
        *lpCSig    = SigtoCSIG( ((lpcpTagBase)ElemData)->sig ) ;
    }else
    {
        return(FALSE);
    }
    return(TRUE);
}



BOOL    EXTERN GetCPElementSize(CHANDLE CP, SINT Index, LPSINT lpSize)
{
    MEMPTR   TagData, ElemData;
    if (lpSize == NULL)
    {
        SetCPLastError(CP_NULL_POINTER_ERR);
        return(FALSE);
    }

    if ( GetCPElementInfo(CP, Index, (LPMEMPTR) &TagData,
            (LPMEMPTR) &ElemData) )
    {
        *lpSize     = ui32toSINT( ((lpcpTag)TagData)->size );
    }else
    {
        return(FALSE);
    }

    return(TRUE);
}

BOOL    EXTERN GetCPElementDataSize(CHANDLE CP, SINT Index, LPSINT lpSize)
{
    MEMPTR  TagData, ElemData;
    if (lpSize == NULL)
    {
        SetCPLastError(CP_NULL_POINTER_ERR);
        return(FALSE);
    }

    if ( GetCPElementInfo(CP, Index, (LPMEMPTR) &TagData,
            (LPMEMPTR) &ElemData) )
    {
//  Changed by jjia 8/24/95
//        *lpSize     = ui32toSINT( ((lpcpTag)TagData)->size ) -
//                                             sizeof(lpcpTagBase);
    *lpSize    = ui32toSINT( ((lpcpTag)TagData)->size) - 
                    sizeof(icTagBase) - sizeof(icUInt32Number);
    }else
    {
        return(FALSE);
    }

    return(TRUE);
}

//***************************************************************
//  The difference between GetCPElement and GetCPElementData
//  is that GetCPElement reads all fields of the element,
//  including the data tag, reserved fields and element data,
//  while GetCPElementData only reads the actual data.
//  Number of bytes that are required to hold the whole data element can be
//  obtained by calling the function GetCPElementSize().
//  The actulal number of data bytes is determined by
//  the call to GetCPElementDataSize().
//***************************************************************
BOOL    EXTERN GetCPElement(CHANDLE CP, SINT Index,
                               MEMPTR lpData, SINT Size)
{
    SINT        ElemSize;
    MEMPTR      TagData, ElemData;
    if (lpData == NULL)
    {
        SetCPLastError(CP_NULL_POINTER_ERR);
        return(FALSE);
    }

    if ( !GetCPElementInfo(CP, Index, (LPMEMPTR) &TagData,
            (LPMEMPTR) &ElemData) )
    {
        return(FALSE);
    }
    ElemSize    = ui32toSINT( ((lpcpTag)TagData)->size);
    if(ElemSize > Size )
    {
        SetCPLastError(CP_NO_MEMORY_ERR);
        return(FALSE);
    }
    MemCopy(lpData, ElemData, ElemSize);
    return(TRUE);

}


BOOL    EXTERN GetCPElementData(CHANDLE CP, SINT Index,
                                MEMPTR lpData, SINT Size)
{
    SINT     ElemSize;
    MEMPTR   TagData, ElemData;
    if (lpData == NULL)
    {
        SetCPLastError(CP_NULL_POINTER_ERR);
        return(FALSE);
    }

    if ( !GetCPElementInfo(CP, Index, (LPMEMPTR) &TagData,
            (LPMEMPTR) &ElemData) )
    {
        return(FALSE);
    }
//  Changed by jjia 8/24/95
//    ElemData    +=  sizeof(lpcpTagBase);
//    ElemSize    = ui32toSINT( ((lpcpTag)TagData)->size) - 
//                     sizeof(lpcpTagBase);
    ElemData    +=  sizeof(icTagBase) + sizeof(icUInt32Number);
    ElemSize    = ui32toSINT( ((lpcpTag)TagData)->size) - 
                    sizeof(icTagBase) - sizeof(icUInt32Number);

    if(ElemSize > Size )
    {
        SetCPLastError(CP_NO_MEMORY_ERR);
        return(FALSE);
    }

    MemCopy(lpData, ElemData, ElemSize);
    return(TRUE);
}

// Check the data format is binary or ascii    8/22/95  jjia

BOOL    EXTERN GetCPElementDataType(CHANDLE CP, SINT Index, long far *lpDataType)
{
    MEMPTR   TagData, ElemData;

    if (lpDataType == NULL)
    {
        SetCPLastError(CP_NULL_POINTER_ERR);
        return(FALSE);
    }

    if ( !GetCPElementInfo(CP, Index, (LPMEMPTR) &TagData,
            (LPMEMPTR) &ElemData) )
    {
        return(FALSE);
    }
    ElemData    +=  sizeof(icTagBase);
    *lpDataType = ui32toSINT( ((icData __huge *)ElemData)->dataFlag);
    return (TRUE);
}

BOOL    EXTERN ValidateCPElement(CHANDLE CP, SINT Index)
{
    CSIG    TagSig, DataSig;
    BOOL    Result;
    if( GetCPTagSig(CP, Index, (LPCSIG) &TagSig) &&
        GetCPElementType(CP, Index, (LPCSIG) &DataSig) )
    {
        switch(TagSig)
        {
            case     icSigAToB0Tag:
            case     icSigAToB1Tag:
            case     icSigAToB2Tag:
            case     icSigBToA0Tag:
            case     icSigBToA1Tag:
            case     icSigBToA2Tag:
            case     icSigGamutTag:
            case     icSigPreview0Tag:
            case     icSigPreview1Tag:
            case     icSigPreview2Tag:
                Result = (DataSig == icSigLut16Type) ||
                         (DataSig == icSigLut8Type) ;
                break;

            case     icSigRedColorantTag: 
            case     icSigGreenColorantTag:
            case     icSigBlueColorantTag:
            case     icSigLuminanceTag:
            case     icSigMediaBlackPointTag:
            case     icSigMediaWhitePointTag:
                Result = (DataSig == icSigXYZType);
                break;

            case     icSigRedTRCTag:
            case     icSigGreenTRCTag:
            case     icSigBlueTRCTag:
            case     icSigGrayTRCTag:
                Result = (DataSig == icSigCurveType);
                break;

            case     icSigPs2CRD0Tag:
            case     icSigPs2CRD1Tag:
            case     icSigPs2CRD2Tag:
            case     icSigPs2CRD3Tag:
            case     icSigPs2CSATag:
            case     icSigPs2Intent0Tag:
            case     icSigPs2Intent1Tag:
            case     icSigPs2Intent2Tag:
            case     icSigPs2Intent3Tag:
                Result = (DataSig == icSigDataType);
                break;

            case     icSigCharTargetTag:
            case     icSigCopyrightTag:
                Result = (DataSig == icSigTextType);
                break;

            case     icSigCalibrationDateTimeTag:
                Result = (DataSig == icSigDateTimeType);
                break;

            case     icSigDeviceMfgDescTag:
            case     icSigDeviceModelDescTag:
            case     icSigProfileDescriptionTag:
            case     icSigScreeningDescTag:
            case     icSigViewingCondDescTag:
                Result = (DataSig == icSigTextDescriptionType);
                break;

            case     icSigMeasurementTag:
                Result = (DataSig == icSigMeasurementTag);
                break;

            case     icSigNamedColorTag:
                Result = (DataSig == icSigNamedColorTag);
                break;
        
            case      icSigProfileSequenceDescTag:
                Result = (DataSig == icSigProfileSequenceDescTag);
                break;

            case     icSigScreeningTag:
                Result = (DataSig == icSigScreeningTag);
                break;

            case     icSigTechnologyTag:
                Result = (DataSig == icSigSignatureType);
                break;

            case     icSigUcrBgTag:
                Result = (DataSig == icSigUcrBgTag);
                break;

            case     icSigViewingConditionsTag:
                Result = (DataSig == icSigViewingConditionsTag);
                break;

            default:
                Result = TRUE;
                break;
        }
    }else
    {
        Result = FALSE;
    }
    return(Result);
}

//******************************************************************
// Functions that get all information from the Color Profile Header
//******************************************************************
BOOL    EXTERN GetCPSize(CHANDLE CP, LPSINT lpSize)
{
    if (lpSize == NULL)
    {
        SetCPLastError(CP_NULL_POINTER_ERR);
        return(FALSE);
    }
    *lpSize    = ui32toSINT( ((lpcpHeader)CP)->size);
    return(TRUE);
}

BOOL    EXTERN GetCPCMMType(CHANDLE CP, LPCSIG lpType)
{
    if (lpType == NULL)
    {
        SetCPLastError(CP_NULL_POINTER_ERR);
        return(FALSE);
    }
    *lpType    = SigtoCSIG( ((lpcpHeader)CP)->cmmId);
    return(TRUE);
}

BOOL    EXTERN GetCPVersion(CHANDLE CP, LPSINT lpVers)
{
    if (lpVers == NULL)
    {
        SetCPLastError(CP_NULL_POINTER_ERR);
        return(FALSE);
    }
    *lpVers    = ui32toSINT( ((lpcpHeader)CP)->version);
    return(TRUE);
}

BOOL    EXTERN GetCPClass(CHANDLE CP, LPCSIG lpClass)
{
    if (lpClass == NULL)
    {
        SetCPLastError(CP_NULL_POINTER_ERR);
        return(FALSE);
    }
    *lpClass    = SigtoCSIG( ((lpcpHeader)CP)->deviceClass);
    return(TRUE);
}

BOOL    EXTERN GetCPDevSpace(CHANDLE CP, LPCSIG lpInSpace)
{
    if (lpInSpace == NULL)
    {
        SetCPLastError(CP_NULL_POINTER_ERR);
        return(FALSE);
    }
    *lpInSpace    = SigtoCSIG( ((lpcpHeader)CP)->colorSpace);
    return(TRUE);
}

BOOL    EXTERN GetCPConnSpace(CHANDLE CP, LPCSIG lpOutSpace)
{
    if (lpOutSpace == NULL)
    {
        SetCPLastError(CP_NULL_POINTER_ERR);
        return(FALSE);
    }
    *lpOutSpace    = SigtoCSIG( ((lpcpHeader)CP)->pcs);
    return(TRUE);
}

BOOL    EXTERN GetCPTarget(CHANDLE CP, LPCSIG lpTarget)
{
    if (lpTarget == NULL)
    {
        SetCPLastError(CP_NULL_POINTER_ERR);
        return(FALSE);
    }
    *lpTarget    = SigtoCSIG( ((lpcpHeader)CP)->platform);
    return(TRUE);
}

BOOL    EXTERN GetCPManufacturer(CHANDLE CP, LPCSIG lpManuf)
{
    if (lpManuf == NULL)
    {
        SetCPLastError(CP_NULL_POINTER_ERR);
        return(FALSE);
    }
    *lpManuf    = SigtoCSIG( ((lpcpHeader)CP)->manufacturer);
    return(TRUE);
}

BOOL    EXTERN GetCPModel(CHANDLE CP, LPCSIG lpModel)
{
    if (lpModel == NULL)
    {
        SetCPLastError(CP_NULL_POINTER_ERR);
        return(FALSE);
    }
    *lpModel    = SigtoCSIG( ((lpcpHeader)CP)->model);
    return(TRUE);
}

BOOL    EXTERN GetCPFlags(CHANDLE CP, LPSINT lpFlags)
{
    if (lpFlags == NULL)
    {
        SetCPLastError(CP_NULL_POINTER_ERR);
        return(FALSE);
    }
    *lpFlags    = ui32toSINT( ((lpcpHeader)CP)->flags);
    return(TRUE);
}

BOOL    EXTERN GetCPRenderIntent(CHANDLE CP, LPSINT lpIntent)
{
    if (lpIntent == NULL)
    {
        SetCPLastError(CP_NULL_POINTER_ERR);
        return(FALSE);
    }
    *lpIntent    = ui32toSINT( ((lpcpHeader)CP)->renderingIntent);
    return(TRUE);
}

BOOL    EXTERN GetCPWhitePoint(CHANDLE CP,  LPSFLOAT lpWP)
{
    if (lpWP == NULL)
    {
        SetCPLastError(CP_NULL_POINTER_ERR);
        return(FALSE);
    }
    lpWP[0]    = (SFLOAT) si16f16toSFLOAT( ((lpcpHeader)CP)->illuminant.X);
    lpWP[1]    = (SFLOAT) si16f16toSFLOAT( ((lpcpHeader)CP)->illuminant.Y);
    lpWP[2]    = (SFLOAT) si16f16toSFLOAT( ((lpcpHeader)CP)->illuminant.Z);
    return(TRUE);
}

BOOL    EXTERN GetCPAttributes(CHANDLE CP, LPATTRIB lpAttributes)
{
    return(TRUE);
}

BOOL    EXTERN GetCPMediaWhitePoint(CHANDLE cp,  LPSFLOAT lpMediaWP)
{
    HGLOBAL   hTempMem;
    SINT      TempSize;
    MEMPTR    TempBuff;
    MEMPTR    lpTable;
    SINT      i, Index;

    if (DoesCPTagExist (cp, icSigMediaWhitePointTag) &&
        GetCPTagIndex (cp, icSigMediaWhitePointTag, (LPSINT) & Index) &&
        GetCPElementSize (cp, Index, (LPSINT) & TempSize) &&
        MemAlloc (TempSize, (HGLOBAL *) & hTempMem, (LPMEMPTR) & TempBuff) &&
        GetCPElement (cp, Index, TempBuff, TempSize))
    {
        lpTable = (MEMPTR) & (((lpcpXYZType) TempBuff)->data);
        for (i = 0; i < 3; i++)
        {
            lpMediaWP[i] = (SFLOAT) si16f16toSFLOAT (lpTable);
            lpTable += sizeof (icS15Fixed16Number);
        }
        MemFree (hTempMem);
        return (TRUE);
    }
    return (FALSE);
}

/***************************************************************************
*                               GetPS2ColorRenderingIntent
*  function:
*    this is the function which creates the Intent string 
*    from the data supplied in the Profile that can be used
*    in --findcolorrendering-- operator.
*  prototype:
*       BOOL EXTERN GetPS2ColorRenderingIntent(
*                          char         *FileName,
*                          DWORD        Intent, 
*                          MEMPTR       lpMem,
*                          LPDWORD      lpcbSize )
*  parameters:
*       FileName    --  Color Profile Filename
*       Intent      --  Intent 
*       lpMem       --  Pointer to the memory block
*       lpcbSize        --  Size of the memory block
*                       Returns number of bytes required/transferred
*  returns:
*       BOOL        --  TRUE   if the function was successful,
*                       FALSE  otherwise.
***************************************************************************/
BOOL EXTERN GetPS2ColorRenderingIntent(CHANDLE cp, DWORD Intent,
                                       MEMPTR lpMem, LPDWORD lpcbSize)
{
    SINT       Index;
    SINT       Size;
        
    if (!cp)
        return FALSE;

    Size = (SINT) *lpcbSize;
    if( ( lpMem == NULL ) || ( Size == 0 ) )
    {
        lpMem = NULL;
        Size = 0;
        *lpcbSize = 0;
    }
        
    switch(Intent)
    {
        case icPerceptual:
        if( DoesCPTagExist(cp, icSigPs2Intent0Tag) &&
            GetCPTagIndex(cp, icSigPs2Intent0Tag, (LPSINT) &Index) &&
            GetCPElementDataSize(cp, Index, (LPSINT) &Size) &&
            ( ( lpMem == NULL ) ||
                GetCPElementData(cp, Index, lpMem, Size) ) )
        {
        }
        break;
        
        case icRelativeColorimetric:
        if( DoesCPTagExist(cp, icSigPs2Intent1Tag) &&
            GetCPTagIndex(cp, icSigPs2Intent1Tag, (LPSINT) &Index) &&
            GetCPElementDataSize(cp, Index, (LPSINT) &Size) &&
            ( ( lpMem == NULL ) ||
                GetCPElementData(cp, Index, lpMem, Size) ) )
        {
        }
        break;
    
        case icSaturation:
        if( DoesCPTagExist(cp, icSigPs2Intent2Tag) &&
            GetCPTagIndex(cp, icSigPs2Intent2Tag, (LPSINT) &Index) &&
            GetCPElementDataSize(cp, Index, (LPSINT) &Size) &&
            ( ( lpMem == NULL ) ||
                GetCPElementData(cp, Index, lpMem, Size ) )
          )
        {
        }
        break;
        case icAbsoluteColorimetric:
        if( DoesCPTagExist(cp, icSigPs2Intent3Tag) &&
            GetCPTagIndex(cp, icSigPs2Intent3Tag, (LPSINT) &Index) &&
            GetCPElementDataSize(cp, Index, (LPSINT) &Size) &&
            ( ( lpMem == NULL ) ||
                GetCPElementData(cp, Index, lpMem, Size) ) )
        {
        }
        break;
        default:
            Size = 0 ;
        break;
    }
        
    if (Size != 0)
    {
        if (lpMem)
        {
            lpMem[Size] = '\0';
        }
        Size ++;
        *lpcbSize = (DWORD) Size;
        return (TRUE);
    }
    else
    {
        return(FALSE);
    }
}

/***************************************************************************
*
*  Function to check if color matching mathod and icc profile type is 
*       supported by driver.
*  parameters:
*
*  returns:
*       BOOL:   TRUE or FALSE.
*
***************************************************************************/

#ifndef ICMDLL
BOOL EXTERN ValidColorSpace(LPPDEVICE lppd, LPICMINFO lpICMI, LPCSIG lpDevCS )
{
    icHeader    CPHeader;
    HFILE       hFile;
    SINT        Res;
    CSIG        CPColorSpaceTag;

    if (NULL == lpICMI)
    {
        return(FALSE);
    }
    hFile = _lopen(lpICMI->lcsDestFilename, READ);
    if( hFile == HFILE_ERROR )
    {
        return(FALSE);
    }

    Res = _lread(hFile, (LPVOID) &CPHeader, sizeof(CPHeader));
    _lclose(hFile);
    if( (Res == HFILE_ERROR) || (Res != sizeof(CPHeader)) )
    {
        return(FALSE);
    }

    // Make the initial check for validity of the profile
    if( SigtoCSIG(CPHeader.magic) != icMagicNumber )
    {
        return(FALSE);
    }
    // Make sure the profile is 'prtr'
    // SRGB98
    // if( SigtoCSIG(CPHeader.deviceClass) != icSigOutputClass )
    // {
    //     return(FALSE);
    // }
    CPColorSpaceTag = SigtoCSIG(CPHeader.colorSpace);
    *lpDevCS = CPColorSpaceTag;             // 247974

    switch ( lppd->lpPSExtDevmode->dm.iColorMatchingMethod )
    {
        case COLOR_MATCHING_ON_HOST:
            if ((CPColorSpaceTag == icSigCmyData))
//                (CPColorSpaceTag == icSigRgbData))
//                (CPColorSpaceTag == icSigGrayData))
            {
                return(FALSE);
            }
            break;
            case COLOR_MATCHING_ON_PRINTER:
            if ((CPColorSpaceTag == icSigCmyData))
//                (CPColorSpaceTag == icSigGrayData))
            {
                return(FALSE);
            }
            break;
        case COLOR_MATCHING_PRINTER_CALIBRATION:
        default:
            break;
    }
    return (TRUE);
}
#endif

//***************************************************************************
//
//      Set of functions to output data into memory buffer
//
//***************************************************************************


/***************************************************************************
*
*   Function to put the chunk of memory as string of Hex
*
***************************************************************************/
SINT    WriteHexBuffer(MEMPTR lpMem, MEMPTR lpBuff, MEMPTR lpLineStart, DWORD dwBytes)
{
    SINT    Res;
    char    TempArray[TempBfSize];
    MEMPTR  lpOldPtr = lpMem;

    for ( ; dwBytes ; dwBytes-- )
    {
        Res = wsprintf( (MEMPTR)TempArray, (LPSTR) "%2.2x", *lpBuff );
        *lpMem++ = TempArray[0];
        *lpMem++ = TempArray[1];
        lpBuff++;
        if (((SINT)(lpMem - lpLineStart)) > MAX_LINELENG)
        {
            lpLineStart = lpMem;
            lpMem += WriteObject(lpMem,  NewLine);
        }
    }
    return( (SINT)(lpMem - lpOldPtr)); 
}

/***************************************************************************
*
*   Function to put the string into the buffer
*
***************************************************************************/
SINT    WriteNewLineObject(MEMPTR lpMem, MEMPTR Obj)
{
    SINT    Res1, Res2;

    Res1 = lstrlen(NewLine);
    MemCopy(lpMem, NewLine, Res1);

    lpMem += Res1;
    Res2 = lstrlen(Obj);
    MemCopy(lpMem, Obj, Res2);
    return( Res1 + Res2 );
}

SINT    WriteObject(MEMPTR lpMem, MEMPTR Obj)
{
    SINT    Res;

    Res = lstrlen(Obj);
    MemCopy(lpMem, Obj, Res);
    return( Res );
}

SINT    WriteObjectN(MEMPTR lpMem, MEMPTR Obj, SINT n)
{
    MemCopy(lpMem, Obj, n);
    return( n );
}
/***************************************************************************
*
*   Function to write the integer into the buffer
*
***************************************************************************/
SINT WriteInt(MEMPTR lpMem, SINT Number)
{
    SINT    Res;
    char    TempArray[TempBfSize];
    
    Res = wsprintf( (MEMPTR)TempArray, "%lu ", Number );
    MemCopy(lpMem, TempArray, lstrlen(TempArray));
    return( Res );
}

/***************************************************************************
*
*   Function to write the integer into the buffer as hex
*
***************************************************************************/
SINT WriteHex(MEMPTR lpMem, SINT Number)
{
    SINT    Res;
    char    TempArray[TempBfSize];

    Res = wsprintf( TempArray, "%2.2x", (int)(Number & 0x00FF) );
    MemCopy(lpMem, TempArray, lstrlen(TempArray));
    return( Res );
}

/***************************************************************************
*
*   Function to write the float into the buffer
*
***************************************************************************/

SINT WriteFloat(MEMPTR lpMem, double dFloat)
{
    char    cSign;
    double  dInt ;
    double  dFract ;
    LONG    lFloat ;
    SINT    Res;
    char    TempArray[TempBfSize];

    lFloat = (LONG) floor( dFloat * 10000.0 + 0.5);

    dFloat = lFloat  / 10000.0 ;

    dInt = floor(fabs(dFloat));
    dFract =  fabs(dFloat) - dInt ;

    cSign   = ' ' ;
    if ( dFloat < 0 )
    {
        cSign   = '-' ;
    }

    Res = wsprintf( (LPSTR) TempArray, (LPSTR) "%c%d.%0.4lu ",
       cSign, (WORD) dInt , (DWORD) (dFract *10000.0)  );
    MemCopy(lpMem, TempArray, lstrlen(TempArray));
    return ( Res );
}

/***************************************************************************
*
*   Function to write the string token into the buffer
*
***************************************************************************/

SINT    WriteStringToken(MEMPTR lpMem, BYTE Token, SINT sNum)
{
    *lpMem++ = Token;
    *lpMem++ = (BYTE)((sNum & 0xFF00) >> 8);
    *lpMem++ = (BYTE)(sNum & 0x00FF);
    return (3);
}

/***************************************************************************
*
*   Function to write the Homogeneous Number Array token into the buffer
*
***************************************************************************/

SINT    WriteHNAToken(MEMPTR lpMem, BYTE Token, SINT sNum)
{
    *lpMem++ = Token;
    *lpMem++ = 32;       // 16-bit fixed integer, high-order byte first
    *lpMem++ = (BYTE)((sNum & 0xFF00) >> 8);
    *lpMem++ = (BYTE)(sNum & 0x00FF);
    return (4);
}

/***************************************************************************
*
*   Function to convert 2-bytes unsigned integer to 2-bytes signed 
*   integer(-32768) and write them to the buffer. High byte first.
*
***************************************************************************/

SINT    WriteIntStringU2S(MEMPTR lpMem, MEMPTR lpBuff, SINT sNum)
{
    SINT    i;
    SINT    Temp;

    for (i = 0; i < sNum; i ++)
    {
        Temp = ui16toSINT( lpBuff) - 32768;
        *lpMem++ = (BYTE)((Temp & 0xFF00) >> 8);
        *lpMem++ = (BYTE)(Temp & 0x00FF);
        lpBuff += sizeof(icUInt16Number);
    }
    return(sNum * 2); 
}

/***************************************************************************
*
*   Function to convert 2-bytes unsigned integer to 2-bytes signed 
*   integer(-32768) and write them to the buffer. Low-order byte first.
*
***************************************************************************/

SINT    WriteIntStringU2S_L(MEMPTR lpMem, MEMPTR lpBuff, SINT sNum)
{
    SINT    i;
    SINT    Temp;

    for (i = 0; i < sNum; i ++)
    {
        Temp = (SINT)*((PUSHORT)lpBuff) - 32768;
        *lpMem++ = (BYTE)((Temp & 0xFF00) >> 8);
        *lpMem++ = (BYTE)(Temp & 0x00FF);
        lpBuff += sizeof(icUInt16Number);
    }
    return(sNum * 2); 
}

/***************************************************************************
*
*   Function to put the chunk of memory into buffer
*
***************************************************************************/
SINT    WriteByteString(MEMPTR lpMem, MEMPTR lpBuff, SINT sBytes)
{
    SINT    i;

    for (i = 0; i < sBytes; i ++)
        *lpMem++ = *lpBuff++;
    
    return(sBytes); 
}

/***************************************************************************
*
*   Function to put the chunk of memory into buffer
*
***************************************************************************/
SINT    WriteInt2ByteString(MEMPTR lpMem, MEMPTR lpBuff, SINT sBytes)
{
    SINT    i;

       for( i = 0; i < sBytes ; i++)
    {
        *lpMem++ = (BYTE)(ui16toSINT( lpBuff)/256) ;
        lpBuff += sizeof(icUInt16Number);
    }
    return(sBytes); 
}

/***************************************************************************
*
*  Function to control ascii85 encoding.
*  parameters:
*       lpDest      --  Pointer to the encording result buffer.
*       BufSize     --  Size of encording result buffer. 
*       lpSource    --  Pointer to the input buffer
*       DataSize    --  Size of the input buffer
*  returns:
*       SINT        --  Number of bytes actually outputed.
*
****************************************************************************/

SINT    WriteASCII85Cont(MEMPTR lpDest, SINT BufSize, MEMPTR lpSource, SINT DataSize)
{
   SINT     incount;
   MEMPTR   lpPtr, lpSave;
   SINT     rem;
   SINT     bcount;
   SINT     dex;
   unsigned long word;
   
   /* encode the initial 4-tuples */
   lpSave = lpDest;
   lpPtr  = lpSource;
   word   = 0UL;
   bcount = 0;

   for (incount = 0; incount < DataSize; incount ++)
   {
      if ( incount  && ((incount % LINELENG) == 0) )
      lpDest += WriteObject(lpDest,  NewLine);
      word = (word<<8);
      word |= (BYTE)*lpPtr++;
      if (bcount == 3)
      {
         lpDest += WriteAscii85(lpDest, word, 5);
         word = 0UL;
         bcount = 0;
      }
      else
      {
         bcount ++;
      }
   }
   
   /* now do the last partial 4-tuple -- if there is one */
   /* see the Red Book spec for the rules on how this is done */
   if (bcount > 0)
   {
      rem = 4 - bcount;  /* count the remaining bytes */
      for (dex = 0; dex < rem; dex ++) /* shift left for each of them */
      {
         word = (word<<8);      /* (equivalent to adding in ZERO's)*/
         word |= (BYTE)32;
      }
//      lpDest += WriteAscii85(lpDest, word, (bcount + 1));  /* output only meaningful
      lpDest += WriteAscii85(lpDest, word, 5);               /* output only meaningful bytes + 1 */
   }
   return (lpDest - lpSave);
}

/************************************************************************
*
*  Function to convert 4 bytes binary data to 5 bytes ascii85 encorded data.
*  parameters:
*       lpDest      --  Pointer to the encording result buffer.
*       inWord      --  Input word (4-bytes) 
*       nBytes      --  Number of bytes should be outputed.
*  returns:
*       SINT        --  Number of bytes actually outputed.
* 
*************************************************************************/

SINT    WriteAscii85(MEMPTR lpDest, unsigned long inWord, SINT nBytes)
{
    unsigned long divisor;
    int      bcount;
    BYTE     outchar;
    MEMPTR   lpSave = lpDest;

    if ((inWord == 0UL) && (nBytes == 5))
        *lpDest++ = 'z';
    else
    {
        divisor = 52200625UL;
        for (bcount = 0; bcount < nBytes; bcount ++)
        { 
            outchar = (BYTE)((int)(inWord/divisor) + (int)'!');
            *lpDest++ = outchar;
            if (bcount < 4)
            {
                inWord = (inWord % divisor);
                divisor =(divisor / 85);
            }
        }
    }
    return (SINT)(lpDest - lpSave);
}

/***************************************************************************
*
*  Function to convert binary data to ascii by performing ASCII85 encording
*  parameters:
*       lpMem       --  A pointer to the buffer. 
*                       as input: contains binary data; 
*                       as output: contains ascii data. 
*       DataSize    --  The size of input binary data. 
*       BufSize     --  The size of buffer pointed by lpMem.
*  returns:
*       SINT        --  Number of bytes actually outputed.
*
***************************************************************************/

SINT    ConvertBinaryData2Ascii(MEMPTR lpMem, SINT DataSize, SINT BufSize)
{
    MEMPTR      intrbuf, Temp;
    HANDLE      intrhandle;
    SINT        AsciiDataSize = 0;

    if (BufSize >= (SINT)(DataSize/4*5 + sizeof(ASCII85DecodeBegine)+sizeof(ASCII85DecodeEnd) + 2048))
    {
        if ((intrhandle = GlobalAlloc(GHND, BufSize)) != NULL)
        {
            if ((intrbuf = (MEMPTR) GlobalLock(intrhandle)) != NULL)
            {
                Temp = intrbuf;
                Temp += WriteObject(Temp,  NewLine);
                Temp += WriteObject(Temp,  ASCII85DecodeBegine);
                Temp += WriteObject(Temp,  NewLine);
                Temp += WriteASCII85Cont(Temp, BufSize, lpMem, DataSize);
                Temp += WriteObject(Temp,  ASCII85DecodeEnd);
                AsciiDataSize = (SINT)(Temp - intrbuf);
                lstrcpyn(lpMem, intrbuf, (WORD)AsciiDataSize); 
                GlobalUnlock(intrhandle);
            }
        }
        GlobalFree(intrhandle);
   }
   return (AsciiDataSize);
}

/***************************************************************************
*
*  Function to check if it is need to convert a CRD from binary to ascii
*  parameters:
*       CP          --  Handle of memory block which contains icm profile.
*       Index       --  Index of the element data of the profile.
*       lpData      --  A pointer to the buffer. 
*                       as input: contains binary data; 
*                       as output: contains ascii data. 
*       BufSize     --  The size of the buffer pointed by lpData.
*       DataSize    --  The size of input binary data.
*       AllowBinary --  Allow binary or not(1/0).
*  returns:
*       SINT        --  Number of bytes required/actually outputed.
*
***************************************************************************/

SINT    Convert2Ascii(CHANDLE CP, SINT Index,
                      MEMPTR lpData, SINT BufSize, 
                      SINT DataSize, BOOL AllowBinary)
{
    long    DataType;

    GetCPElementDataType(CP, Index, &DataType);
    if (BufSize == 0)
    {
        if (AllowBinary)
            return (DataSize);
        else if (DataType == 0)    // Ascii data in Profile 
            return (DataSize);
        else                       // Keep space for ascii85 encoding.
            return (DataSize / 4 * 5 + sizeof(ASCII85DecodeBegine)+sizeof(ASCII85DecodeEnd) + 2048);
    }
    else
    {
        if (AllowBinary)
            return (DataSize);
        else if(DataType == 0) 
            return (DataSize);
        else 
            return (ConvertBinaryData2Ascii(lpData, DataSize, BufSize) );
    }
}

#ifdef ICMDLL
SINT    MemCopy(MEMPTR Dest, MEMPTR Source, SINT Length)
{
    SINT    i;

    for (i = 0; i < Length; i++)
    {
        Dest[i] = Source[i];
    }
    return( Length );
}

// SRGB98
BOOL EXTERN InvertMatrix (double FAR * lpInMatrix,
                   double FAR * lpOutMatrix)
{
    double det;

    double FAR *a;
    double FAR *b;
    double FAR *c;
    if ((NULL == lpInMatrix) ||
        (NULL == lpOutMatrix))
    {
        return (FALSE);
    }
    a = (double FAR *) &(lpInMatrix[0]);
    b = (double FAR *) &(lpInMatrix[3]);
    c = (double FAR *) &(lpInMatrix[6]);

    det = a[0] * b[1] * c[2] + a[1] * b[2] * c[0] + a[2] * b[0] * c[1] -
        (a[2] * b[1] * c[0] + a[1] * b[0] * c[2] + a[0] * b[2] * c[1]);

    if (det == 0.0)                     // What to do?
    {
        lpOutMatrix[0] = 1.0;
        lpOutMatrix[1] = 0.0;
        lpOutMatrix[2] = 0.0;

        lpOutMatrix[3] = 0.0;
        lpOutMatrix[4] = 1.0;
        lpOutMatrix[5] = 0.0;

        lpOutMatrix[6] = 0.0;
        lpOutMatrix[7] = 0.0;
        lpOutMatrix[8] = 1.0;
        return (FALSE);
    } else
    {
        lpOutMatrix[0] = (b[1] * c[2] - b[2] * c[1]) / det;
        lpOutMatrix[3] = -(b[0] * c[2] - b[2] * c[0]) / det;
        lpOutMatrix[6] = (b[0] * c[1] - b[1] * c[0]) / det;

        lpOutMatrix[1] = -(a[1] * c[2] - a[2] * c[1]) / det;
        lpOutMatrix[4] = (a[0] * c[2] - a[2] * c[0]) / det;
        lpOutMatrix[7] = -(a[0] * c[1] - a[1] * c[0]) / det;

        lpOutMatrix[2] = (a[1] * b[2] - a[2] * b[1]) / det;
        lpOutMatrix[5] = -(a[0] * b[2] - a[2] * b[0]) / det;
        lpOutMatrix[8] = (a[0] * b[1] - a[1] * b[0]) / det;
        return (TRUE);
    }
}

/*
 *  Crc - 32 BIT ANSI X3.66 CRC checksum files
 *
 *
 * Copyright (C) 1986 Gary S. Brown.  You may use this program, or
 * code or tables extracted from it, as desired without restriction.
 */

static DWORD  crc_32_tab[] = { /* CRC polynomial 0xedb88320 */
0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

DWORD  FAR  PASCAL  crc32(MEMPTR buff, DWORD  length)
{
  DWORD  crc, charcnt;
  BYTE    c;


  crc = 0xFFFFFFFF;
  charcnt = 0;

  for (charcnt = 0 ; charcnt < length ; charcnt++)
  {
    c = buff[charcnt] ;
    crc = crc_32_tab[(crc ^ c) & 0xff] ^ (crc >> 8);
  }

  return crc;
}
#endif

