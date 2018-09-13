#include "generic.h"

//#pragma code_seg(_ICM2SEG)
#pragma optimize("",off)

#define TempBfSize 128
#define LINELENG   128

static  char    NewLine[]       = "\n" ;
static  char    ASCII85DecodeBegine[] = "<~";
static  char    ASCII85DecodeEnd[] = "~> cvx exec ";

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

BOOL    EXTERN LoadCP(LPCSTR filename, HGLOBAL FAR *phMem, LPCHANDLE lpCP)
{
    icHeader    CPHeader;
    HFILE       hFile;
    SINT        Res, CPSize;
    MEMPTR      mpCP;
    OFSTRUCT    OFStruct;

    *phMem = 0;
    if (lpCP == NULL)
    {
        SetCPLastError(CP_NULL_POINTER_ERR);
        return(FALSE);
    }

    hFile = OpenFile(filename, &OFStruct, OF_READ );
    if( hFile == HFILE_ERROR )
    {
        SetCPLastError(CP_FILE_OPEN_ERR);
        return(FALSE);
    }
 
    Res = _lread(hFile, (LPVOID) &CPHeader, sizeof(CPHeader));
    if( (Res == HFILE_ERROR) ||
        (Res != sizeof(CPHeader)) )
    {
        _close(hFile);
        SetCPLastError(CP_FILE_READ_ERR);
        return(FALSE);
    }

    // Make the initial check for validity of the profile
    if( SigtoCSIG(CPHeader.magic) != icMagicNumber )
    {
        _close(hFile);
        SetCPLastError(CP_FORMAT_ERR);
        return(FALSE);
    }

    CPSize = ui32toSINT(CPHeader.size);
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
                _close(hFile);
                SetCPLastError(CP_FILE_READ_ERR);
                return(FALSE);
            }
            mpCP    += Res;
            CPSize  -= Res;
        }
    }else
    {
        *phMem = 0;
        _close(hFile);
        return(FALSE);
    }
    _close(hFile);
    return (TRUE);
}

#ifdef ICMDLL
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

    CPSize = ui32toSINT(CPHeader.size);
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
BOOL EXTERN ValidColorSpace(LPPDEVICE lppd, LPICMINFO lpICMI)
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
    if( SigtoCSIG(CPHeader.deviceClass) != icSigOutputClass )
    {
        return(FALSE);
    }
    CPColorSpaceTag = SigtoCSIG(CPHeader.colorSpace);

    switch ( lppd->lpPSExtDevmode->dm.iColorMatchingMethod )
    {
        case COLOR_MATCHING_ON_HOST:
            if ((CPColorSpaceTag == icSigCmyData) ||
                (CPColorSpaceTag == icSigRgbData))
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
#endif

