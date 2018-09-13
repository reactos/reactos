#include "generic.h"
#include "icmstr.h"

#pragma code_seg(_ICM3SEG)

#define MAXCOLOR8  255

#pragma optimize("", off)

static SINT
CreateHostInputOutputArray (MEMPTR lpMem, PMEMPTR ppArray,
       SINT numChan, SINT tableSize, SINT Offset, CSIG Tag, MEMPTR Buff);
static BOOL
CheckInputOutputTable(LPHOSTCLUT lpHostClut, float far *fTemp, BOOL, BOOL);
BOOL
GetHostCSA_Intent (CHANDLE cp, MEMPTR lpBuffer, LPDWORD lpcbSize,
       CSIG Intent, int Type);
static BOOL
CheckColorLookupTable(LPHOSTCLUT lpHostClut, float far *fTemp);
static BOOL
DoHostConversionCRD (LPHOSTCLUT lpHostCRD, LPHOSTCLUT lpHostCSA,
       float far *Input, float far *Output, 
       CSIG ColorSpace, BOOL bCheckOutputTable);
static BOOL
DoHostConversionCSA (LPHOSTCLUT lpHostClut, float far *Input, float far *Output);
static BOOL
GetCRDInputOutputArraySize(CHANDLE cp, DWORD Intent, 
       LPSINT lpInTbSize, LPSINT lpOutTbSize, 
       LPCSIG lpIntentTag, LPSINT lpGrids);
static void
LabToXYZ(float far *Input, float far *Output, float far *whitePoint);

/***************************************************************************
*                           CreateHostInputOutputArray
*  function:
*    this is the function which creates the output array from the data
*    supplied in the ColorProfile's LUT8 or LUT16 tag.
*  parameters:
*    MEMPTR     lpMem        : The buffer to save output array.
*    LPHOSTCLUT lpHostClut   : 
*    SINT       nOutputCh    : Number of input channel.
*    SINT       nOutputTable : The size of each input table. 
*    SINT       Offset       : The position of source output data(in icc profile).
*    CSIG       Tag          : To determin the Output table is 8 or 16 bits.
*    MEMPTR     Buff         : The buffer that contains source data(copyed from icc profile)
*
*  returns:
*       SINT    Returns number of bytes of Output Array
*
***************************************************************************/

static SINT
CreateHostInputOutputArray (MEMPTR lpMem, PMEMPTR ppArray,
                            SINT numChan, SINT tableSize, 
                            SINT Offset, CSIG Tag, MEMPTR Buff)
{
    SINT    i, j;
    PUSHORT lpMemPtr16;
    MEMPTR  lpMemPtr8;
    MEMPTR  lpTable;

    if (Tag == icSigLut8Type)
        lpMemPtr8 = lpMem;
    else
        lpMemPtr16 = (PUSHORT)lpMem;

    for (i = 0; i < numChan; i++)
    {
        if (Tag == icSigLut8Type)
        {
            ppArray[i] = lpMemPtr8;
            lpTable = (MEMPTR) (((lpcpLut8Type) Buff)->lut.data) +
                Offset +
                tableSize * i;
            MemCopy(lpMemPtr8, lpTable, tableSize);
                lpMemPtr8 += tableSize;
        }
        else
        {
            ppArray[i] = (MEMPTR)lpMemPtr16;
            lpTable = (MEMPTR) (((lpcpLut16Type) Buff)->lut.data) +
                2 * Offset +
                2 * tableSize * i;
            for (j = 0; j < tableSize; j++)
            {
                *lpMemPtr16++ = (USHORT) ui16toSINT (lpTable);
                lpTable += sizeof (icUInt16Number);
            }
        }
    }
    if (Tag == icSigLut8Type)
        return ((SINT) ((MEMPTR)lpMemPtr8 - lpMem));
    else
        return ((SINT) ((MEMPTR)lpMemPtr16 - lpMem));

}

VOID
GetCLUTinfo(CSIG LutTag, MEMPTR lpLut, LPSINT nInputCh, LPSINT nOutputCh, 
            LPSINT nGrids, LPSINT nInputTable, LPSINT nOutputTable, LPSINT size)
{
    if (LutTag == icSigLut8Type)
    {
        *nInputCh = ui8toSINT (((lpcpLut8Type) lpLut)->lut.inputChan);
        *nOutputCh = ui8toSINT (((lpcpLut8Type) lpLut)->lut.outputChan);
        *nGrids = ui8toSINT (((lpcpLut8Type) lpLut)->lut.clutPoints);
        *nInputTable = 256L;
        *nOutputTable = 256L;
        *size = 1;  // one byte for each input\output table entry
    } else
    {
        *nInputCh = ui8toSINT (((lpcpLut16Type) lpLut)->lut.inputChan);
        *nOutputCh = ui8toSINT (((lpcpLut16Type) lpLut)->lut.outputChan);
        *nGrids = ui8toSINT (((lpcpLut16Type) lpLut)->lut.clutPoints);
        *nInputTable = ui16toSINT (((lpcpLut16Type) lpLut)->lut.inputEnt);
        *nOutputTable = ui16toSINT (((lpcpLut16Type) lpLut)->lut.outputEnt);
        *size = 2;  // two bytes for each input\output table entry
    }
}

/***************************************************************************
*                           GetHostCSA
*  function:
*    this is the function which creates a Host CSA
*  parameters:
*       CHANDLE cp       --  Color Profile handle 
*       MEMPTR lpMem     --  Pointer to the memory block. If this point is NULL,
*                            require buffer size.
*       LPDWORD lpcbSize --  Size of the memory block
*       CSIG InputIntent --
*       SINT Index       --  to the icc profile tag that contains the data of Intent
*       int  Type        --  DEF or DEFG
*  returns:
*       BOOL        --  TRUE if the function was successful,
*                       FALSE otherwise.
***************************************************************************/
static BOOL
GetHostCSA (CHANDLE cp, MEMPTR lpMem, LPDWORD lpcbSize,
            CSIG InputIntent, SINT Index, int Type)
{
    CSIG    PCS, LutTag;
    CSIG    IntentSig;
    SINT    nInputCh, nOutputCh, nGrids, SecondGrids;
    SINT    nInputTable, nOutputTable, nNumbers;
    SINT    i, j, k;
    MEMPTR  lpTable;
    MEMPTR  lpOldMem = lpMem;
    MEMPTR  lpLut = NULL;
    HGLOBAL hLut = 0;
    SINT    LutSize;
    LPHOSTCLUT lpHostClut;

    // Check if we can generate the CS.
    // If we cannot find the required tag - we will return false
    if (!GetCPConnSpace (cp, (LPCSIG) & PCS) ||
        (PCS != icSigLabData) && (PCS != icSigXYZData) ||
        !GetCPTagSig (cp, Index, (LPCSIG) & IntentSig))
    {
        return (FALSE);
    }
    if (!GetCPElementType (cp, Index, (LPCSIG) & LutTag) ||
        ((LutTag != icSigLut8Type) && (LutTag != icSigLut16Type)) ||
        !GetCPElementSize (cp, Index, (LPSINT) & LutSize) ||
        !MemAlloc (LutSize, (HGLOBAL FAR *) &hLut, (LPMEMPTR) & lpLut) ||
        !GetCPElement (cp, Index, lpLut, LutSize))
    {
        if (0 != hLut)
        {
            MemFree (hLut);
        }
        return (FALSE);
    }

    // Estimate the memory size required to hold CS
    GetCLUTinfo(LutTag, lpLut, &nInputCh, &nOutputCh, 
                &nGrids, &nInputTable, &nOutputTable, &i);

    if (!(nOutputCh == 3) ||
        !((nInputCh == 3) && (Type == TYPE_CIEBASEDDEF)) &&
        !((nInputCh == 4) && (Type == TYPE_CIEBASEDDEFG)))
    {
        SetCPLastError (CP_POSTSCRIPT_ERR);
        MemFree (hLut);
        return (FALSE);
    }
    
    // First Pass. This is a size request
    if (lpMem == NULL)                  
    {
        if (Type == TYPE_CIEBASEDDEFG)
            *lpcbSize = nOutputCh * nGrids * nGrids * nGrids * nGrids;
        else
            *lpcbSize = nOutputCh * nGrids * nGrids * nGrids;
        *lpcbSize = *lpcbSize             +   // size of RenderTable 8-bits only
            nInputCh * nInputTable * i    +   // size of input table 8/16-bits
            nOutputCh * nOutputTable * i  +   // size of output table 8/16-bits
            sizeof(HOSTCLUT) + 1024;          // data structure + extra safe space
        MemFree (hLut);
        return (TRUE);
    }

    // Second pass. constructure real HostCSA
    lpHostClut = (LPHOSTCLUT)lpMem;
    lpMem += sizeof(HOSTCLUT);
    lpHostClut->size = sizeof(HOSTCLUT);
    lpHostClut->pcs = PCS;
    lpHostClut->intent = InputIntent;
    lpHostClut->lutBits = (LutTag == icSigLut8Type)? 8:16;

    // Get info about Illuminant White Point from the header
    GetCPWhitePoint (cp, (LPSFLOAT)lpHostClut->whitePoint);          // .. Illuminant

    lpHostClut->inputChan = (unsigned char)nInputCh;
    lpHostClut->outputChan = (unsigned char)nOutputCh;
    lpHostClut->clutPoints = (unsigned char)nGrids;
    lpHostClut->inputEnt = (USHORT)nInputTable;
    lpHostClut->outputEnt = (USHORT)nOutputTable;
    // Input Array
    lpMem += CreateHostInputOutputArray (lpMem, lpHostClut->inputArray, 
             nInputCh, nInputTable, 0, LutTag, lpLut);

    if (Type == TYPE_CIEBASEDDEFG)
    {
        i = nInputTable * nInputCh +
            nGrids * nGrids * nGrids * nGrids * nOutputCh;
    } else
    {
        i = nInputTable * nInputCh +
            nGrids * nGrids * nGrids * nOutputCh;
    }
    // ourput array
    lpMem += CreateHostInputOutputArray (lpMem, lpHostClut->outputArray, 
             nOutputCh, nOutputTable, i, LutTag, lpLut);
 //********** /Table

    lpHostClut->clut = lpMem;
    nNumbers = nGrids * nGrids * nOutputCh;
    SecondGrids = 1;
    if (Type == TYPE_CIEBASEDDEFG)
    {
        SecondGrids = nGrids;
    }
    for (i = 0; i < nGrids; i++)        // Nh strings should be sent
    {
        for (k = 0; k < SecondGrids; k++)
        {
            if (LutTag == icSigLut8Type)
            {
                lpTable = (MEMPTR) (((lpcpLut8Type) lpLut)->lut.data) +
                    nInputTable * nInputCh +
                    nNumbers * (i * SecondGrids + k);
            } else
            {
                lpTable = (MEMPTR) (((lpcpLut16Type) lpLut)->lut.data) +
                    2 * nInputTable * nInputCh +
                    2 * nNumbers * (i * SecondGrids + k);
            }

            if (LutTag == icSigLut8Type)
            {
                // Copy 8-bit data.
                MemCopy(lpMem, lpTable, nNumbers);
                lpMem += nNumbers;
            }
            else
            {
                // convert 16 bit integer to right format. then copy only 8 bits.
                for (j = 0; j < nNumbers; j++)
                {
                    *lpMem++ = (BYTE)(ui16toSINT (lpTable) / 256);
                    lpTable += sizeof (icUInt16Number);
                }
            }
        }
    }

    *lpcbSize = (DWORD) (lpMem - lpOldMem);

    MemFree (hLut);
    return (TRUE);
}

// BUGBUG   -- MOVE to CSPROF.C
HGLOBAL GetTRCData(CHANDLE cp,
        LPMEMPTR lpRed,  LPMEMPTR lpGreen,  LPMEMPTR lpBlue,
        LPSINT   lpnRed, LPSINT   lpnGreen, LPSINT   lpnBlue)
{
    SINT     RedTRCIndex, GreenTRCIndex, BlueTRCIndex;
    SINT     RedTRCSize = 0, GreenTRCSize = 0, BlueTRCSize = 0;
    SINT     MemSize;
    HGLOBAL  hMem;

 // Check if we can generate the CRD
    if (!GetTRCElementSize(cp, icSigRedTRCTag, &RedTRCIndex, &RedTRCSize) ||
        !GetTRCElementSize(cp, icSigGreenTRCTag, &GreenTRCIndex, &GreenTRCSize) ||
        !GetTRCElementSize(cp, icSigBlueTRCTag, &BlueTRCIndex, &BlueTRCSize))
    {
         return 0;
    }
    MemSize = RedTRCSize + GreenTRCSize + BlueTRCSize;
    if (!MemAlloc (MemSize, (HGLOBAL FAR *)&hMem, (LPMEMPTR) lpRed))
        return 0;

    *lpGreen = *lpRed + RedTRCSize;
    *lpBlue = *lpGreen + GreenTRCSize;
    if (!GetCPElement (cp, RedTRCIndex, *lpRed, RedTRCSize) ||
        !GetCPElement (cp, GreenTRCIndex, *lpGreen, GreenTRCSize ) ||
        !GetCPElement (cp, BlueTRCIndex, *lpBlue, BlueTRCSize ))
    {
        MemFree (hMem);
        return (NULL);
    }
    *lpnRed = ui32toSINT (((lpcpCurveType) *lpRed)->curve.count);
    *lpnGreen = ui32toSINT (((lpcpCurveType) *lpGreen)->curve.count);
    *lpnBlue = ui32toSINT (((lpcpCurveType) *lpBlue)->curve.count);

    return (hMem);
}


static SINT
CreateHostTRCInputTable(MEMPTR lpMem, LPHOSTCLUT lpHostClut,
                        MEMPTR lpRed, MEMPTR lpGreen, MEMPTR lpBlue)
{
    SINT    i;
    PUSHORT lpPtr16;
    MEMPTR  lpTable;

    lpPtr16 = (PUSHORT)lpMem;
   
    lpHostClut->inputArray[0] = (MEMPTR)lpPtr16;
    lpTable = (MEMPTR)(((lpcpCurveType) lpRed)->curve.data);
    for (i = 0; i < (SINT)(lpHostClut->inputEnt); i++)
    {
        *lpPtr16++ = (USHORT) ui16toSINT(lpTable);
        lpTable += sizeof(icUInt16Number);
    }

    lpHostClut->inputArray[1] = (MEMPTR)lpPtr16;
    lpTable = (MEMPTR)(((lpcpCurveType) lpGreen)->curve.data);
    for (i = 0; i < (SINT)(lpHostClut->inputEnt); i++)
    {
        *lpPtr16++ = (USHORT) ui16toSINT(lpTable);
        lpTable += sizeof(icUInt16Number);
    }

    lpHostClut->inputArray[2] = (MEMPTR)lpPtr16;
    lpTable = (MEMPTR)(((lpcpCurveType) lpBlue)->curve.data);
    for (i = 0; i < (SINT)(lpHostClut->inputEnt); i++)
    {
        *lpPtr16++ = (USHORT) ui16toSINT(lpTable);
        lpTable += sizeof(icUInt16Number);
    }
    return ((MEMPTR)lpPtr16 - lpMem);
}

static SINT
CreateHostRevTRCInputTable(MEMPTR lpMem, LPHOSTCLUT lpHostClut,
                           MEMPTR lpRed, MEMPTR lpGreen, MEMPTR lpBlue)
{
    HGLOBAL   hTemp;
    MEMPTR    lpTemp;

    if (!MemAlloc (lpHostClut->outputEnt * (REVCURVE_RATIO + 1) * 2 ,
                  (HGLOBAL FAR *) &hTemp, (LPMEMPTR) &lpTemp))
    {
        return (0);
    }

    lpHostClut->outputArray[0] = lpMem;
    GetRevCurve (lpRed, lpTemp, lpHostClut->outputArray[0]);
    lpHostClut->outputArray[1] = lpHostClut->outputArray[0] +
                                2 * REVCURVE_RATIO * lpHostClut->outputEnt;
    GetRevCurve (lpGreen, lpTemp, lpHostClut->outputArray[1]);
    lpHostClut->outputArray[2] = lpHostClut->outputArray[1] +
                                2 * REVCURVE_RATIO * lpHostClut->outputEnt;
    GetRevCurve (lpBlue, lpTemp, lpHostClut->outputArray[2]);

    MemFree (hTemp);
    return ( 2 * REVCURVE_RATIO * lpHostClut->outputEnt * 3);
}

static BOOL
GetHostMatrixCSAorCRD(CHANDLE cp, MEMPTR lpMem, LPDWORD lpcbSize, BOOL bCSA)
{
    SINT     nRedCount, nGreenCount, nBlueCount;
    MEMPTR   lpRed = NULL,lpGreen, lpBlue;
    HGLOBAL  hMem;
    LPHOSTCLUT lpHostClut;
    MEMPTR   lpOldMem = lpMem;
    double   pArray[9], pRevArray[9], pTemp[9];
    SINT     i;

    hMem = GetTRCData(cp,
        (LPMEMPTR)&lpRed, (LPMEMPTR)&lpGreen, (LPMEMPTR)&lpBlue,
        (LPSINT)&nRedCount,(LPSINT)&nGreenCount, (LPSINT)&nBlueCount);

    // Estimate the memory size required to hold CRD
    *lpcbSize = (nRedCount + nGreenCount + nBlueCount) * 2 +
           sizeof(HOSTCLUT) + 1024;      // data structure + extra safe space

    if (lpMem == NULL)                   // This is a size request
    {
        MemFree (hMem);
        return TRUE;
    }

    lpHostClut = (LPHOSTCLUT)lpMem;
    lpMem += sizeof(HOSTCLUT);
    lpHostClut->size = sizeof(HOSTCLUT);
    lpHostClut->dataType = DATA_matrix;
    lpHostClut->clutPoints = 2;
    lpHostClut->pcs = icSigXYZData;
    GetCPWhitePoint(cp, (LPSFLOAT)lpHostClut->whitePoint);

    if (bCSA)
    {
        lpHostClut->inputEnt = (USHORT)nRedCount;
        lpHostClut->inputChan = 3;
        lpMem += CreateHostTRCInputTable(lpMem, lpHostClut,
                                         lpRed, lpGreen, lpBlue);
    }
    else
    {
        lpHostClut->outputEnt = (USHORT)nRedCount;
        lpHostClut->outputChan = 3;
        lpMem += CreateHostRevTRCInputTable(lpMem, lpHostClut,
                                            lpRed, lpGreen, lpBlue);
    }

    MemFree (hMem);
    *lpcbSize = (DWORD) (lpMem - lpOldMem);

    if (!CreateColorantArray(cp, &pTemp[0], icSigRedColorantTag) ||
        !CreateColorantArray(cp, &pTemp[3], icSigGreenColorantTag) ||
        !CreateColorantArray(cp, &pTemp[6], icSigBlueColorantTag))
    {
       return (FALSE);
    }

    for (i = 0; i < 9; i++)
    {
        pArray[i] = pTemp[i/8*8 + i*3%8];
    }

    if (bCSA)
    {
        for (i = 0; i < 9; i++)
            lpHostClut->e[i] = (float)pArray[i];
    }
    else
    {
        InvertMatrix(pArray, pRevArray);
        for (i = 0; i < 9; i++)
            lpHostClut->e[i] = (float)pRevArray[i];
    }

    return TRUE;
}

/***************************************************************************
*                        GetHostCSA_Intent
*  function:
*       This is the function which creates the Host DEF or DEFGColorSpace array
*       based on Intent.
*  parameters:
*       cp          --  Color Profile handle
*       lpBuffer    --  Pointer to the memory block. If this point is NULL,
*                       require buffer size.
*       lpcbSize    --  Size of the memory block
*       Intent      --  Intent.
*       Type        --  CieBasedDEF or CieBasedDEF.
*  returns:
*       BOOL        --  TRUE if the function was successful,
*                       FALSE otherwise.
***************************************************************************/

BOOL
GetHostCSA_Intent (CHANDLE cp, MEMPTR lpBuffer, LPDWORD lpcbSize,
                   CSIG Intent, int Type)
{
    SINT Index;
    BOOL Success = FALSE;
    CSIG AToBxTag;

    switch (Intent)
    {
        case icPerceptual:
            AToBxTag = icSigAToB0Tag;
            break;
        case icRelativeColorimetric:
        case icAbsoluteColorimetric:
            // use RelativeColorimetric data to build it.
            AToBxTag = icSigAToB1Tag;
            break;
        case icSaturation:
            AToBxTag = icSigAToB2Tag;
            break;
        default:
            return FALSE;
            break;
    }
    if (DoesCPTagExist (cp, AToBxTag) &&
        GetCPTagIndex (cp, AToBxTag, (LPSINT) & Index))
    {
        Success = GetHostCSA(cp, lpBuffer, lpcbSize, Intent, Index, Type);
    }
    else if ((DoesTRCAndColorantTagExist(cp)) &&
            (Type == TYPE_CIEBASEDDEF))
    {
        Success = GetHostMatrixCSAorCRD(cp, lpBuffer, lpcbSize, TRUE);
    }

    return Success;
}
/***************************************************************************
*                            GetHostColorSpaceArray
*  function:
*    This is the main function which creates the Host CSA
*    from the data supplied in the Profile.
*  parameters:
*       cp          --  Color Profile handle
*       InputIntent --  Intent.
*       lpBuffer    --  Pointer to the memory block. If this point is NULL,
*                       require buffer size.
*       lpcbSize    --  Size of the memory block
*  returns:
*       BOOL        --  TRUE if the function was successful,
*                       FALSE otherwise.
***************************************************************************/
static BOOL
GetHostColorSpaceArray (CHANDLE cp, DWORD InputIntent,
                       MEMPTR  lpBuffer, LPDWORD lpcbSize)
{
    CSIG ColorSpace, Intent;
    BOOL Success = FALSE;

    if (!cp)
        return Success;

    if (!GetCPDevSpace (cp, (LPCSIG) & ColorSpace) ||
        !GetCPRenderIntent (cp, (LPCSIG) & Intent))
    {
        return Success;
    }
    if (InputIntent == icUseRenderingIntent)
        InputIntent = (DWORD)Intent;

    if (!Success)
    {
        switch (ColorSpace)
        {
            case icSigRgbData:
                Success = GetHostCSA_Intent (cp, lpBuffer, lpcbSize,
                          (CSIG) InputIntent, TYPE_CIEBASEDDEF);
                break;
            case icSigCmykData:
                Success = GetHostCSA_Intent (cp, lpBuffer, lpcbSize,
                          (CSIG) InputIntent, TYPE_CIEBASEDDEFG);
                break;
            default:
                break;
        }
    }
    return Success;
}

//===========================================================================

/***************************************************************************
*                             CreateHostLutCRD
*  function:
*    this is the function which creates the Host CRD
*    from the data supplied in the ColorProfile's LUT8 or LUT16 tag.
*  parameters:
*       cp          --  Color Profile handle
*       Index       --  Index of the tag
*       lpMem       --  Pointer to the memory block.If this point is NULL,
*                       require buffer size.
*       InputIntent --  Intent.
*
*  returns:
*       SINT        --  Size of Host CRD.
***************************************************************************/

static SINT 
CreateHostLutCRD (CHANDLE cp, SINT Index, MEMPTR lpMem, DWORD InputIntent)
{
    SINT     nInputCh, nOutputCh, nGrids;
    SINT     nInputTable, nOutputTable, nNumbers;
    CSIG     Tag, PCS;
    CSIG     IntentSig;

    SINT     Ret;
    SINT     i, j;
    MEMPTR   lpTable;

    MEMPTR   Buff = NULL;
    SINT     MemSize = 0;
    MEMPTR   lpOldMem = lpMem;
    HGLOBAL  hMem;
    LPHOSTCLUT  lpHostClut;

    // Check if we can generate the CRD
    if (!GetCPTagSig (cp, Index, (LPCSIG) & IntentSig) ||
        !GetCPElementType (cp, Index, (LPCSIG) & Tag) ||
        ((Tag != icSigLut8Type) && (Tag != icSigLut16Type)) ||
        !GetCPConnSpace (cp, (LPCSIG) & PCS) ||
        !GetCPElementSize (cp, Index, (LPSINT) & MemSize) ||
        !MemAlloc (MemSize, (HGLOBAL FAR *)&hMem, (LPMEMPTR) & Buff) ||
        !GetCPElement (cp, Index, Buff, MemSize))
    {
        if (NULL != Buff)
        {
            MemFree (hMem);
        }
        return (0);
    }

    GetCLUTinfo(Tag, Buff, &nInputCh, &nOutputCh, 
                &nGrids, &nInputTable, &nOutputTable, &i);

    if (((nOutputCh != 3) && (nOutputCh != 4)) ||
        (nInputCh != 3))
    {
        SetCPLastError (CP_POSTSCRIPT_ERR);
        MemFree (hMem);
        return (0);
    }

    // First Pass. This is a size request
    if (lpMem == NULL)       
    {
        Ret = nInputCh * nInputTable * i         +  // Input table 8/16-bits
            nOutputCh * nOutputTable * i         +  // Output table 8/16-bits
            nOutputCh * nGrids * nGrids * nGrids +  // CLUT 8-bits only
            sizeof(HOSTCLUT)                     +  // Data structure 
            1024;                                   // safe

        MemFree (hMem);
        return (Ret);
    }
     
    // Second Pass. Get a HostCRD
    lpHostClut = (LPHOSTCLUT)lpMem;
    lpMem += sizeof(HOSTCLUT);
    lpHostClut->size = sizeof(HOSTCLUT);
    lpHostClut->pcs = PCS;
    lpHostClut->intent = InputIntent;
    lpHostClut->lutBits = (Tag == icSigLut8Type)? 8:16;

    GetCPWhitePoint (cp, (LPSFLOAT)lpHostClut->whitePoint);          // .. Illuminant

    // Support absolute whitePoint
    if (!GetCPMediaWhitePoint (cp, (LPSFLOAT)lpHostClut->mediaWP)) // .. Media WhitePoint
    {
        lpHostClut->mediaWP[0] = lpHostClut->whitePoint[0];
        lpHostClut->mediaWP[1] = lpHostClut->whitePoint[1];
        lpHostClut->mediaWP[2] = lpHostClut->whitePoint[2];
    }
    lpHostClut->inputChan = (unsigned char)nInputCh;
    lpHostClut->outputChan = (unsigned char)nOutputCh;
    lpHostClut->clutPoints = (unsigned char)nGrids;
    lpHostClut->inputEnt = (USHORT)nInputTable;
    lpHostClut->outputEnt = (USHORT)nOutputTable;

//******** Input array
    lpMem += CreateHostInputOutputArray (lpMem, lpHostClut->inputArray, 
             nInputCh, nInputTable, 0, Tag, Buff);
//******** the offset to the position of output array.
    i = nInputTable * nInputCh +
        nGrids * nGrids * nGrids * nOutputCh;
//******** Output array
    lpMem += CreateHostInputOutputArray (lpMem, lpHostClut->outputArray, 
             nOutputCh, nOutputTable, i, Tag, Buff);
//******** Matrix.
    if (PCS == icSigXYZData)
    {
        if (Tag == icSigLut8Type)
        {
            lpTable = (MEMPTR) & ((lpcpLut8Type) Buff)->lut.e00;
        } else
        {
            lpTable = (MEMPTR) & ((lpcpLut16Type) Buff)->lut.e00;
        }
        for (i = 0; i < 9; i++)
        {
            lpHostClut->e[i] = (float)((si16f16toSFLOAT (lpTable)) / CIEXYZRange);
            lpTable += sizeof (icS15Fixed16Number);
        }
    }
//********** RenderTable
    nNumbers = nGrids * nGrids * nOutputCh;
    lpHostClut->clut = lpMem;
    for (i = 0; i < nGrids; i++)        // Na strings should be sent
    {
        if (Tag == icSigLut8Type)
        {
            lpTable = (MEMPTR) (((lpcpLut8Type) Buff)->lut.data) +
                nInputTable * nInputCh +
                nNumbers * i;
        } else
        {
            lpTable = (MEMPTR) (((lpcpLut16Type) Buff)->lut.data) +
                2 * nInputTable * nInputCh +
                2 * nNumbers * i;
        }
        if (Tag == icSigLut8Type)
        {
            MemCopy(lpMem, lpTable, nNumbers);
            lpMem += nNumbers;
        }
        else
        {
            for (j = 0; j < nNumbers; j++)
            {
                *lpMem++ = (BYTE)(ui16toSINT (lpTable) / 256);
                lpTable += sizeof (icUInt16Number);
            }
        }
    }

    MemFree (hMem);
    return ((SINT) ((unsigned long) (lpMem - lpOldMem)));
}


/***************************************************************************
*                      GetHostColorRenderingDictionary
*  function:
*    this is the main function which creates the Host CRD
*  parameters:
*       cp          --  Color Profile handle
*       Intent      --  Intent.
*       lpMem       --  Pointer to the memory block.If this point is NULL,
*                       require buffer size.
*       lpcbSize    --  size of memory block.
*
*  returns:
*       SINT        --  !=0 if the function was successful,
*                         0 otherwise.
*                       Returns number of bytes required/transferred
***************************************************************************/
static BOOL
GetHostColorRenderingDictionary (CHANDLE cp, DWORD Intent,
                                MEMPTR lpMem, LPDWORD lpcbSize)
{
    SINT Index;
    CSIG BToAxTag;

    if (!cp)
        return FALSE;

    if ((lpMem == NULL) || (*lpcbSize == 0))
    {
        lpMem = NULL;
        *lpcbSize = 0;
    }

    switch (Intent)
    {
        case icPerceptual:
            BToAxTag = icSigBToA0Tag;
            break;

        case icRelativeColorimetric:
        case icAbsoluteColorimetric:
            // Use RelativeColorimetric to calculate this CRD.
            BToAxTag = icSigBToA1Tag;
            break;

        case icSaturation:
            BToAxTag = icSigBToA2Tag;
            break;

        default:
           *lpcbSize = (DWORD) 0;
            return FALSE;
    }

    if (DoesCPTagExist (cp, BToAxTag) &&
        GetCPTagIndex (cp, BToAxTag, (LPSINT) & Index))
    {
        *lpcbSize = CreateHostLutCRD (cp, Index, lpMem, Intent);
    }
    else if(DoesTRCAndColorantTagExist(cp))
    {
        GetHostMatrixCSAorCRD(cp, lpMem, lpcbSize, FALSE);
    }
    return (*lpcbSize > 0);
}

//========================================================================
/***************************************************************************
*                               g
*  function:
*    Calculate function y = g(x). used in Lab->XYZ conversion
*    y = g(x):      g(x) = x*x*x             if x >= 6/29
*                   g(x) = 108/841*(x-4/29)  otherwise
*  parameters:
*       f           --  x
*  returns:
*       SINT        --  y
***************************************************************************/

static float g(float f)
{
    float frc;
    if (f >= (6/29))
    {
        frc = f * f * f;
    }
    else
    {
        frc = f - (4.0f / 29.0f) * (108.0f / 841.0f);
    }
    return frc;
}

/***************************************************************************
*                          inverse_g
*  function:
*    Calculate inverse function y = g(x). used in XYZ->Lab conversion
*  parameters:
*       f           --  y
*  returns:
*       SINT        --  x
***************************************************************************/
static float inverse_g(float f)
{
    double frc;
    if (f >= (6.0*6.0*6.0)/(29.0*29.0*29.0))
    {
        frc = pow(f, 1.0 / 3.0);
    }
    else
    {
        frc = f * (841.0 / 108.0) + (4.0 / 29.0);
    }
    return (float)frc;
}

//========================================================================

static BOOL
TableInterp3(LPHOSTCLUT lpHostClut, float far *fTemp)
{
    int    tmpA, tmpBC;
    int    cellA, cellB, cellC;
    float  a, b, c;
    short  Grids;
    short  outputChan;
    MEMPTR v000, v001, v010, v011;
    MEMPTR v100, v101, v110, v111;
    float  vx0x, vx1x;
    float  v0xx, v1xx;
    int    idx;

    cellA = (int)(fTemp[0]);
    a = fTemp[0] - cellA;

    cellB = (int)(fTemp[1]);
    b = fTemp[1] - cellB;

    cellC = (int)(fTemp[2]);
    c = fTemp[2] - cellC;

    Grids = lpHostClut->clutPoints;
    outputChan = lpHostClut->outputChan;
    tmpA  = outputChan * Grids * Grids; 
    tmpBC = outputChan * (Grids * cellB + cellC);

    // Calculate 8 surrounding cells.
    v000 = lpHostClut->clut + tmpA * cellA + tmpBC;
    v001 = (cellC < (Grids - 1))? v000 + outputChan : v000;
    v010 = (cellB < (Grids - 1))? v000 + outputChan * Grids : v000;
    v011 = (cellC < (Grids - 1))? v010 + outputChan : v010 ;

    v100 = (cellA < (Grids - 1))? v000 + tmpA : v000;
    v101 = (cellC < (Grids - 1))? v100 + outputChan : v100;
    v110 = (cellB < (Grids - 1))? v100 + outputChan * Grids : v100;
    v111 = (cellC < (Grids - 1))? v110 + outputChan : v110;

    for (idx = 0; idx < outputChan; idx++)
    {
        // Calculate the average of 4 bottom cells.
        vx0x = *v000 + c * (int)((int)*v001 - (int)*v000);
        vx1x = *v010 + c * (int)((int)*v011 - (int)*v010);
        v0xx = vx0x + b * (vx1x - vx0x);

        // Calculate the average of 4 upper cells.
        vx0x = *v100 + c * (int)((int)*v101 - (int)*v100);
        vx1x = *v110 + c * (int)((int)*v111 - (int)*v110);
        v1xx = vx0x + b * (vx1x - vx0x);

        // Calculate the bottom and upper average.
        fTemp[idx] = (v0xx + a * (v1xx - v0xx)) / MAXCOLOR8;

        if ( idx < (outputChan - 1))
        {
            v000++;
            v001++;
            v010++;
            v011++;
            v100++;
            v101++;
            v110++;
            v111++;
        }
    }

    return TRUE;
}

static BOOL
TableInterp4(LPHOSTCLUT lpHostClut, float far *fTemp)
{
    int    tmpH, tmpI, tmpJK;
    int    cellH, cellI, cellJ, cellK;
    float  h, i, j, k;
    short  Grids;
    short  outputChan;
    MEMPTR v0000, v0001, v0010, v0011;
    MEMPTR v0100, v0101, v0110, v0111;
    MEMPTR v1000, v1001, v1010, v1011;
    MEMPTR v1100, v1101, v1110, v1111;
    float  vxx0x, vxx1x;
    float  vx0xx, vx1xx;
    float  v0xxx, v1xxx;
    int    idx;

    cellH = (int)(fTemp[0]);
    h = fTemp[0] - cellH;

    cellI = (int)(fTemp[1]);
    i = fTemp[1] - cellI;

    cellJ = (int)(fTemp[2]);
    j = fTemp[2] - cellJ;

    cellK = (int)(fTemp[3]);
    k = fTemp[3] - cellK;

    Grids = lpHostClut->clutPoints;
    outputChan = lpHostClut->outputChan;
    tmpI  = outputChan * Grids * Grids;
    tmpH  = tmpI * Grids; 
    tmpJK = outputChan * (Grids * cellJ + cellK);

    // Calculate 16 surrounding cells.
    v0000 = lpHostClut->clut + tmpH * cellH + tmpI * cellI + tmpJK;
    v0001 = (cellK < (Grids - 1))? v0000 + outputChan : v0000;
    v0010 = (cellJ < (Grids - 1))? v0000 + outputChan * Grids : v0000;
    v0011 = (cellK < (Grids - 1))? v0010 + outputChan : v0010;

    v0100 = (cellI < (Grids - 1))? v0000 + tmpI : v0000;
    v0101 = (cellK < (Grids - 1))? v0100 + outputChan : v0100;
    v0110 = (cellJ < (Grids - 1))? v0100 + outputChan * Grids : v0100;
    v0111 = (cellK < (Grids - 1))? v0110 + outputChan : v0110;

    v1000 = (cellH < (Grids - 1))? v0000 + tmpH : v0000;
    v1001 = (cellK < (Grids - 1))? v1000 + outputChan : v1000;
    v1010 = (cellJ < (Grids - 1))? v1000 + outputChan * Grids : v1000;
    v1011 = (cellK < (Grids - 1))? v1010 + outputChan : v1010;

    v1100 = (cellI < (Grids - 1))? v1000 + tmpI : v1000;
    v1101 = (cellK < (Grids - 1))? v1100 + outputChan : v1100;
    v1110 = (cellJ < (Grids - 1))? v1100 + outputChan * Grids : v1100;
    v1111 = (cellK < (Grids - 1))? v1110 + outputChan : v1110;

    for (idx = 0; idx < outputChan; idx++)
    {
        // Calculate the average of 8 bottom cells.
        vxx0x = *v0000 + k * (int)((int)*v0001 - (int)*v0000);
        vxx1x = *v0010 + k * (int)((int)*v0011 - (int)*v0010);
        vx0xx = vxx0x + j * (vxx1x - vxx0x);
        vxx0x = *v0100 + k * (int)((int)*v0101 - (int)*v0100);
        vxx1x = *v0110 + k * (int)((int)*v0111 - (int)*v0110);
        vx1xx = vxx0x + j * (vxx1x - vxx0x);
        v0xxx = vx0xx + i * (vx1xx - vx0xx);

        // Calculate the average of 8 upper cells.
        vxx0x = *v1000 + k * (int)((int)*v1001 - (int)*v1000);
        vxx1x = *v1010 + k * (int)((int)*v1011 - (int)*v1010);
        vx0xx = vxx0x + j * (vxx1x - vxx0x);
        vxx0x = *v1100 + k * (int)((int)*v1101 - (int)*v1100);
        vxx1x = *v1110 + k * (int)((int)*v1111 - (int)*v1110);
        vx1xx = vxx0x + j * (vxx1x - vxx0x);
        v1xxx = vx0xx + i * (vx1xx - vx0xx);

        // Calculate the bottom and upper average.
        fTemp[idx] = (v0xxx + h * (v1xxx - v0xxx)) / MAXCOLOR8;

        if ( idx < (outputChan - 1))
        {
            v0000++;
            v0001++;
            v0010++;
            v0011++;
            v0100++;
            v0101++;
            v0110++;
            v0111++;
            v1000++;
            v1001++;
            v1010++;
            v1011++;
            v1100++;
            v1101++;
            v1110++;
            v1111++;
        }
    }

    return TRUE;
}


/***************************************************************************
*                         CheckColorLookupTable
*  function:
*    This function check RenderTable.
*  parameters:
*       LPHOSTCLUT lpHostClut -- 
*       float far  *fTemp     --  Input (in range [0 gred-1]) /
*                                 output(in range [0 1)
*  returns:
*       BOOL                  -- TRUE
***************************************************************************/

static BOOL
CheckColorLookupTable(LPHOSTCLUT lpHostClut, float far *fTemp) 
{
    if (lpHostClut->inputChan == 3)
    {
        TableInterp3(lpHostClut, fTemp);
    }
    else if(lpHostClut->inputChan == 4)
    {
        TableInterp4(lpHostClut, fTemp);
    }
    return TRUE;
}

/***************************************************************************
*                         CheckInputOutputTable
*  function:
*    This function check inputTable.
*  parameters:
*       LPHOSTCLUT lpHostClut -- 
*       float far  *fTemp     --  Input / output data
*  returns:
*       BOOL                  -- TRUE
***************************************************************************/
static BOOL
CheckInputOutputTable(LPHOSTCLUT lpHostClut, float far *fTemp, 
                      BOOL bCSA, BOOL bInputTable) 
{
    int     i;
    short   Grids;
    USHORT  floor1, ceiling1;
    float   fIndex;
    int     numChan;
    int     numEnt;
    PMEMPTR ppArray;

    if (bInputTable)
    {
        numChan = lpHostClut->inputChan;
        numEnt = lpHostClut->inputEnt - 1;
        ppArray = lpHostClut->inputArray;
    }
    else
    {
        numChan = lpHostClut->outputChan;
        numEnt = lpHostClut->outputEnt - 1;
        ppArray = lpHostClut->outputArray;
    }

    Grids = lpHostClut->clutPoints;
    for (i = 0; (i <= MAXCHANNELS) && (i < numChan); i++)
    {
        fTemp[i] = (fTemp[i] < 0)? 0: ((fTemp[i] > 1)? 1: fTemp[i]);
        fIndex = fTemp[i] * numEnt;
        if (lpHostClut->lutBits == 8)
        {
            floor1 = ppArray[i][(int)fIndex];
            ceiling1 = ppArray[i][((int)fIndex) + 1];
            fTemp[i] = (float)(floor1 + (ceiling1 - floor1) * (fIndex - floor(fIndex)));
            if (bCSA && !bInputTable)
                fTemp[i] = (float)(fTemp[i] / 127.0);
            else
                fTemp[i] = (float)(fTemp[i] / 255.0);
        }
        else
        {
            floor1 = ((PUSHORT)(ppArray[i]))[(int)fIndex];
            ceiling1 = ((PUSHORT)(ppArray[i]))[((int)fIndex) + 1];
            fTemp[i] = (float)(floor1 + (ceiling1 - floor1) * (fIndex - floor(fIndex)));
            if (bCSA && !bInputTable)
                fTemp[i] = (float)(fTemp[i] / 32767.0);
            else
                fTemp[i] = (float)(fTemp[i] / 65535.0);

        }
        if (bInputTable)
        {
            fTemp[i] *= (Grids - 1);
            if (fTemp[i] > (Grids - 1))
                fTemp[i] = (float)(Grids - 1);
        }
    }
    return TRUE;
}

static void
LabToXYZ(float far *Input, float far *Output, float far *whitePoint)
{
    float   fL, fa, fb;

    fL = (Input[0] * 50 + 16) / 116;
    fa = (Input[1] * 128 - 128) / 500;
    fb = (Input[2] * 128 - 128) / 200;
    Output[0] = whitePoint[0] * g(fL + fa);
    Output[1] = whitePoint[1] * g(fL);
    Output[2] = whitePoint[2] * g(fL - fb);
}

static void
XYZToLab(float far *Input, float far *Output, float far *whitePoint)
{
    float   fL, fa, fb;

    fL = inverse_g(Input[0] / whitePoint[0]);
    fa = inverse_g(Input[1] / whitePoint[1]);
    fb = inverse_g(Input[2] / whitePoint[2]);
    Output[0] = (fa * 116 - 16) / 100;
    Output[1] = (fL * 500 - fa * 500 + 128) / 255;
    Output[2] = (fa * 200 - fb * 200 + 128) / 255;
}

static void
ApplyMatrix(PFLOAT e, float far *Input, float far *Output)
{
    SINT  i, j;

    for (i = 0; i < 3; i++)
    {
        j = i*3;
        Output[i] = e[j ]    * Input[0] +
                    e[j + 1] * Input[1] +
                    e[j + 2] * Input[2];
    }
}

/***************************************************************************
*                         DoHostConversionCRD
*  function:
*    This function converts XYZ/Lab to RGB/CMYK by using HostCRD
*  parameters:
*       LPHOSTCLUT lpHostCRD  -- pointer to a HostCRD
*       LPHOSTCLUT lpHostCSA  -- pointer to a HostCSA
*       float far *Input      -- Input XYZ/Lab
*       float far *Output     -- Output RGB/CMYK
*  returns:
*       BOOL                  -- TRUE
***************************************************************************/
static BOOL
DoHostConversionCRD (LPHOSTCLUT lpHostCRD, LPHOSTCLUT lpHostCSA,
                     float far *Input, float far *Output,
                     CSIG ColorSpace, BOOL bCheckOutputTable)
{
    float   fTemp[MAXCHANNELS];
    float   fTemp1[MAXCHANNELS];
    int     i;

/**
** Input XYZ or Lab in range [0 2]
***/
    // When sampling the deviceCRD, skip the input table.
    // If lpHostCSA is not NULL, the current CRD is targetCRD, we
    // need to do input table conversion
    if (lpHostCSA)
    {
        // Convert Lab to XYZ  in range [ 0 whitePoint ]
        if ((lpHostCRD->pcs == icSigXYZData) && 
            (lpHostCSA->pcs == icSigLabData))
        {
            LabToXYZ(Input, fTemp1, lpHostCRD->whitePoint);
        }
        // Convert XYZ to Lab in range [ 0 1]
        else if ((lpHostCRD->pcs == icSigLabData) && 
                 (lpHostCSA->pcs == icSigXYZData))
        {
            XYZToLab(Input, fTemp, lpHostCSA->whitePoint);
        }
        // Convert Lab to range [ 0 1]
        else if ((lpHostCRD->pcs == icSigLabData) && 
                 (lpHostCSA->pcs == icSigLabData))
        {
            for (i = 0; i < 3; i++)
                fTemp[i] = Input[i] / 2;
        }
        // Convert XYZ to XYZ (based on white point) to range [0 1]
        else
        {   // TODO: different intents using different conversion.
            // icRelativeColorimetric: using Bradford transform.
            // icAbsoluteColorimetric: using scaling.
            for (i = 0; i < 3; i++)
                fTemp1[i] = Input[i] * lpHostCRD->whitePoint[i] / lpHostCSA->whitePoint[i];
        }
 
        // Matrix, used for XYZ data only or Matrix icc profile only
        if (lpHostCRD->pcs == icSigXYZData)
        {
            ApplyMatrix(lpHostCRD->e, fTemp1, fTemp);
        }
     
        if (lpHostCRD->dataType != DATA_matrix)
        {
            //Search input Table
            CheckInputOutputTable(lpHostCRD, fTemp, 0, 1);
        }
    }
    // If the current CRD is device CRD, we do not need to do input
    // table conversion.
    else
    {
        short   Grids;
        Grids = lpHostCRD->clutPoints;
        // Sample data may be XYZ or Lab. It depends on Target icc profile.
        // If the PCS of the target icc profile is XYZ, input data will be XYZ.
        // If the PCS of the target icc profile is Lab, input data will be Lab.

        if (lpHostCRD->dataType == DATA_matrix)
        {
            for (i = 0; i < 3; i++)
            {
                fTemp[i] = Input[i];
            }
        }
        else
        {
            for (i = 0; i < 3; i++)
            {
                fTemp[i] = Input[i]* (Grids - 1);
                if (fTemp[i] > (Grids - 1))
                    fTemp[i] = (float)(Grids - 1);
            }
        }
    }   // bCheckInputTable

    if (lpHostCRD->dataType != DATA_matrix)
    {
        // Rendering table
        CheckColorLookupTable(lpHostCRD, fTemp);

        /**
         ** Output RGB or CMYK in range [0 1]
        ***/
    }
    if (bCheckOutputTable)
    {
        //Output Table
        CheckInputOutputTable(lpHostCRD, fTemp, 0, 0);
    }
    for (i = 0; (i <= MAXCHANNELS) && (i < lpHostCRD->outputChan); i++)
    {
        Output[i] = fTemp[i];
    }

   return TRUE;
}

/***************************************************************************
*                         DoHostConversionCSA
*  function:
*    This function converts RGB/CMYK to XYZ/Lab by using HostCSA
*  parameters:
*       LPHOSTCLUT lpHostCLUT -- pointer to a HostCSA
*       float far *Input      -- Input XYZ/Lab
*       float far *Output     -- Output RGB/CMYK
*  returns:
*       BOOL                  -- TRUE
***************************************************************************/

static BOOL
DoHostConversionCSA (LPHOSTCLUT lpHostClut, float far *Input, float far *Output)
{
    float   fTemp[MAXCHANNELS];
    int     i;

/**
** Input RGB or CMYK in range [0 1]
***/
    for (i = 0; (i <= MAXCHANNELS) && (i < lpHostClut->inputChan); i++)
    {
        fTemp[i] = Input[i];
    }

    if (lpHostClut->dataType == DATA_matrix)
    {
        //Search input Table
        CheckInputOutputTable(lpHostClut, fTemp, 1, 1);
        ApplyMatrix(lpHostClut->e, fTemp, Output);
    }
    else
    {
        //Search input Table
        CheckInputOutputTable(lpHostClut, fTemp, 1, 1);

        // Rendering table
        CheckColorLookupTable(lpHostClut, fTemp);

        //Output Table
        CheckInputOutputTable(lpHostClut, fTemp, 1, 0 );

        /**
         ** Output XYZ or Lab in range [0 2]
         ***/
        for (i = 0; (i <= MAXCHANNELS) && (i < lpHostClut->outputChan); i++)
        {
            Output[i] = fTemp[i];
        }
    }

    return TRUE;
}               

static BOOL
GetCRDInputOutputArraySize(CHANDLE cp, DWORD Intent, 
    LPSINT lpInTbSize, LPSINT lpOutTbSize, 
    LPCSIG lpIntentTag, LPSINT lpGrids)
{
    CSIG    Tag;
    SINT    Index;
    SINT    Ret = 0;
    MEMPTR  Buff = NULL;
    SINT    MemSize = 0;
    HGLOBAL hMem;
    SINT    outputChan, outputEnt;
    SINT    inputChan, inputEnt;
    SINT    Grids;
    SINT    i;

    switch (Intent)
    {
        case icPerceptual:
            *lpIntentTag = icSigBToA0Tag;
            break;

        case icRelativeColorimetric:
        case icAbsoluteColorimetric:
            *lpIntentTag = icSigBToA1Tag;
            break;

        case icSaturation:
            *lpIntentTag = icSigBToA2Tag;
            break;

        default:
            return FALSE;
    }
    if (!DoesCPTagExist (cp, *lpIntentTag) ||
        !GetCPTagIndex (cp, *lpIntentTag, (LPSINT) & Index) ||
        !GetCPElementType (cp, Index, (LPCSIG) & Tag) ||
        ((Tag != icSigLut8Type) && (Tag != icSigLut16Type)) ||
        !GetCPElementSize (cp, Index, (LPSINT) & MemSize) ||
        !MemAlloc (MemSize, (HGLOBAL FAR *)&hMem, (LPMEMPTR) & Buff) ||
        !GetCPElement (cp, Index, Buff, MemSize))
    {
        BOOL retVal = FALSE;

        if (NULL != Buff)
        {
            MemFree (hMem);
        }

        // Matrix icc profile.

        *lpGrids = 2;
        if (lpInTbSize)
        {
            retVal = GetHostCSA_Intent (cp, NULL, lpInTbSize,
                          (CSIG) Intent, TYPE_CIEBASEDDEF);
            *lpInTbSize = *lpInTbSize * 3;
        }
        if (lpOutTbSize)
        {
            retVal = GetHostCSA_Intent (cp, NULL, lpOutTbSize,
                          (CSIG) Intent, TYPE_CIEBASEDDEF);
            *lpOutTbSize = *lpOutTbSize * 3;
        }
        return retVal;
    }

    if (lpInTbSize)
    {
        GetCLUTinfo(Tag, Buff, &inputChan, &outputChan, 
                &Grids, &inputEnt, &outputEnt, &i);

        if (inputChan != 3)
        {
            MemFree (hMem);
            return FALSE;
        }

        *lpInTbSize = inputChan * inputEnt * 6;  // Number of INT bytes
        *lpGrids = Grids;
    }

    if (lpOutTbSize)
    {
        GetCLUTinfo(Tag, Buff, &inputChan, &outputChan, 
                &Grids, &inputEnt, &outputEnt, &i);

        if ((outputChan != 3) && (outputChan != 4))
        {
            MemFree (hMem);
            return FALSE;
        }
        *lpOutTbSize = outputChan * outputEnt * 6; // Number of INT bytes
        *lpGrids = Grids;
    }

    MemFree (hMem);
    return TRUE;
}

/***************************************************************************
*                         CreateOutputArray
*  function:
*       Create CSA/CRD output arrays from the data supplied in icc profile's
*       LUT8 or LUT16 tags.
*  parameters:
*       MEMPTR  lpMem        -- a pointer to a buffer which will contain the arrays. 
*       SINT    nOutputCh    -- Number of output channel. if lpHostClut, no meaning.
*       SINT    nOutputTable -- saze of each array.  if lpHostClut, no meaning.
*       SINT    Offset       -- offset of Buff, point to the 1st byte of output array in CLUT.
*                               if lpHostClut, no meaning.
*       MEMPTR  Intent       -- 
*       CSIG    Tag          -- LUT8 or LUT16
*       MEMPTR  Buff         -- point to a buffer which contain LUT8 or LUT16.
*                               if NULL, use lpHostClut.
*       BOOL    AllowBinary  --
*       MEMPTR  lpHostClut   -- point to host CSA/CRD. if NULL, use Buff.
*  returns:
*       SINT                 -- The size of output array created.
***************************************************************************/

SINT
CreateOutputArray (MEMPTR lpMem, SINT nOutputCh, 
    SINT nOutputTable, SINT Offset, MEMPTR Intent, 
    CSIG Tag, MEMPTR Buff, BOOL AllowBinary, MEMPTR lpHostClut)
{
    SINT i, j;
    MEMPTR lpOldMem;
    MEMPTR lpTable;
    MEMPTR lpLineStart;
    lpOldMem = lpMem;

    if (lpHostClut)
    {
        nOutputCh = (SINT)(((LPHOSTCLUT)lpHostClut)->outputChan);
        nOutputTable = (SINT)(((LPHOSTCLUT)lpHostClut)->outputEnt);
        Tag = (((LPHOSTCLUT)lpHostClut)->lutBits == 8)? 
              icSigLut8Type : icSigLut16Type;
    }

    for (i = 0; i < nOutputCh; i++)
    {
        lpLineStart = lpMem;
        lpMem += WriteNewLineObject (lpMem, Slash);
        if (lpHostClut)
            lpMem += WriteObject (lpMem, PreViewOutArray);
        else
            lpMem += WriteObject (lpMem, OutputArray);
        lpMem += WriteObjectN (lpMem, Intent, lstrlen (Intent));
        lpMem += WriteInt (lpMem, i);
        
        if (lpHostClut)
            lpTable = ((LPHOSTCLUT)lpHostClut)->outputArray[i];
        else
        {
            if (Tag == icSigLut8Type)
                lpTable = (MEMPTR) (((lpcpLut8Type) Buff)->lut.data) +
                    Offset +
                    nOutputTable * i;
            else
                lpTable = (MEMPTR) (((lpcpLut16Type) Buff)->lut.data) +
                    2 * Offset +
                    2 * nOutputTable * i;
        }

        if (!AllowBinary)               // Output ASCII CRD
        {
            if (Tag == icSigLut8Type)
            {
                lpMem += WriteObject (lpMem, BeginString);
                lpMem += WriteHexBuffer (lpMem, lpTable, lpLineStart, nOutputTable);
                lpMem += WriteObject (lpMem, EndString);
            } else
            {
                lpMem += WriteObject (lpMem, BeginArray);
                for (j = 0; j < nOutputTable; j++)
                {
                    if (lpHostClut)
                        lpMem += WriteInt (lpMem, *((PUSHORT)lpTable));
                    else
                        lpMem += WriteInt (lpMem, ui16toSINT (lpTable));
                    lpTable += sizeof (icUInt16Number);
                    if (((SINT) (lpMem - lpLineStart)) > MAX_LINELENG)
                    {
                        lpLineStart = lpMem;
                        lpMem += WriteObject (lpMem, NewLine);
                    }
                }
                lpMem += WriteObject (lpMem, EndArray);
            }
        } else
        {                               // Output BINARY CRD
            if (Tag == icSigLut8Type)
            {
                lpMem += WriteStringToken (lpMem, 143, 256);
                lpMem += WriteByteString (lpMem, lpTable, 256L);
            } else
            {
                lpMem += WriteHNAToken (lpMem, 149, nOutputTable);
                if (lpHostClut)
                    lpMem += WriteIntStringU2S_L (lpMem, lpTable, nOutputTable);
                else
                    lpMem += WriteIntStringU2S (lpMem, lpTable, nOutputTable);
            }
        }
        lpMem += WriteObject (lpMem, DefOp);
    }

    return ((SINT) (lpMem - lpOldMem));
}

/***************************************************************************
*                         CreateInputArray
*  function:
*       Create CSA/CRD Input arrays from the data supplied in icc profile's
*       LUT8 or LUT16 tags.
*  parameters:
*       MEMPTR  lpMem        -- a pointer to the buffer which will contain the arrays.
*       SINT    nInputCh     -- Number of input channel. if lpHostClut, no meaning.
*       SINT    nInputTable  -- saze of each array.  if lpHostClut, no meaning.
*       SINT    Offset       -- offset of Buff, point to the 1st byte of output array in CLUT.
*                               if lpHostClut, no meaning.
*       MEMPTR  Intent       -- 
*       CSIG    Tag          -- LUT8 or LUT16
*       MEMPTR  Buff         -- point to a buffer which contains LUT8 or LUT16.
*                               if NULL, use lpHostClut.
*       BOOL    AllowBinary  --
*       MEMPTR  lpHostClut   -- point to host CSA/CRD. if NULL, use Buff.
*  returns:
*       SINT                 -- The size of inpput array created.
***************************************************************************/

SINT
CreateInputArray (MEMPTR lpMem, SINT nInputCh, 
    SINT nInputTable, MEMPTR Intent, CSIG Tag, 
    MEMPTR Buff, BOOL bAllowBinary, MEMPTR lpHostClut)
{
    SINT i, j;
    MEMPTR lpOldMem;
    MEMPTR lpTable;
    MEMPTR lpLineStart;
    lpOldMem = lpMem;

    if (lpHostClut)
    {
        nInputCh = (SINT)(((LPHOSTCLUT)lpHostClut)->inputChan);
        nInputTable = (SINT)(((LPHOSTCLUT)lpHostClut)->inputEnt);
        Tag = (((LPHOSTCLUT)lpHostClut)->lutBits == 8)? 
               icSigLut8Type : icSigLut16Type;
    }

    for (i = 0; i < nInputCh; i++)
    {
        lpLineStart = lpMem;
        lpMem += WriteNewLineObject (lpMem, Slash);
        if (lpHostClut)
            lpMem += WriteObject (lpMem, PreViewInArray);
        else
            lpMem += WriteObject (lpMem, InputArray);
        lpMem += WriteObjectN (lpMem, Intent, lstrlen (Intent));
        lpMem += WriteInt (lpMem, i);

        if (lpHostClut)
        {
            lpTable = ((LPHOSTCLUT)lpHostClut)->inputArray[i];
        }
        else
        {
            if (Tag == icSigLut8Type)
                lpTable = (MEMPTR) (((lpcpLut8Type) Buff)->lut.data) + nInputTable * i;
            else
                lpTable = (MEMPTR) (((lpcpLut16Type) Buff)->lut.data) + 2 * nInputTable * i;
        }
        if (!bAllowBinary)               // Output ASCII CRD
        {
            if (Tag == icSigLut8Type)
            {
                lpMem += WriteObject (lpMem, BeginString);
                lpMem += WriteHexBuffer (lpMem, lpTable,lpLineStart, nInputTable);
                lpMem += WriteObject (lpMem, EndString);
            } else
            {
                lpMem += WriteObject (lpMem, BeginArray);
                for (j = 0; j < nInputTable; j++)
                {
                    if(lpHostClut)
                        lpMem += WriteInt (lpMem, *((PUSHORT)lpTable));
                    else
                        lpMem += WriteInt (lpMem, ui16toSINT (lpTable));
                    lpTable += sizeof (icUInt16Number);
                    if (((SINT) (lpMem - lpLineStart)) > MAX_LINELENG)
                    {
                        lpLineStart = lpMem;
                        lpMem += WriteObject (lpMem, NewLine);
                    }
                }
                lpMem += WriteObject (lpMem, EndArray);
            }
        } else
        {                               // Output BINARY CRD
            if (Tag == icSigLut8Type)
            {
                lpMem += WriteStringToken (lpMem, 143, 256);
                lpMem += WriteByteString (lpMem, lpTable, 256L);
            } else
            {
                lpMem += WriteHNAToken (lpMem, 149, nInputTable);
                if (lpHostClut)
                    lpMem += WriteIntStringU2S_L (lpMem, lpTable, nInputTable);
                else
                    lpMem += WriteIntStringU2S (lpMem, lpTable, nInputTable);
            }
        }
        lpMem += WriteObject (lpMem, DefOp);
    }

    return ((SINT) (lpMem - lpOldMem));
}

SINT
SendCRDLMN(MEMPTR lpMem, CSIG Intent, LPSFLOAT whitePoint, LPSFLOAT mediaWP, CSIG pcs)
{
    MEMPTR  lpOldMem;
    SINT    i, j;

    lpOldMem = lpMem;

//********** /MatrixLMN
    if (icAbsoluteColorimetric == Intent)
    {
        lpMem += WriteNewLineObject (lpMem, MatrixLMNTag);

        lpMem += WriteObject (lpMem, BeginArray);
        for (i = 0; i < 3; i++)
        {
            for (j = 0; j < 3; j++)
                lpMem += WriteFloat (lpMem,
                    (double) (i == j) ? whitePoint[i] / mediaWP[i] : 0.0);
        }
        lpMem += WriteObject (lpMem, EndArray);
    }
 //********** /RangeLMN
    lpMem += WriteNewLineObject (lpMem, RangeLMNTag);
    if (pcs == icSigXYZData)
    {
        lpMem += WriteObject (lpMem, BeginArray);
        for (i = 0; i < 3; i++)
        {
            lpMem += WriteFloat (lpMem, (double) 0);
            lpMem += WriteFloat (lpMem, (double) whitePoint[i]);
        }
        lpMem += WriteObject (lpMem, EndArray);
    } else
    {
        lpMem += WriteObject (lpMem, RangeLMNLab);
    }

 //********** /EncodeLMN
    lpMem += WriteNewLineObject (lpMem, EncodeLMNTag);
    lpMem += WriteObject (lpMem, BeginArray);
    for (i = 0; i < 3; i++)
    {
        lpMem += WriteObject (lpMem, BeginFunction);
        if (pcs != icSigXYZData)
        {
            lpMem += WriteFloat (lpMem, (double)whitePoint[i]);
            lpMem += WriteObject (lpMem, DivOp);
            lpMem += WriteObject (lpMem, EncodeLMNLab);
        }
        lpMem += WriteObject (lpMem, EndFunction);
    }
    lpMem += WriteObject (lpMem, EndArray);

    return (SINT)(lpMem - lpOldMem);
}


SINT
SendCRDPQR(MEMPTR lpMem, CSIG Intent, LPSFLOAT whitePoint)
{
    MEMPTR  lpOldMem;
    SINT    i;

    lpOldMem = lpMem;

    if (icAbsoluteColorimetric != Intent)
    {
     //********** /RangePQR
        lpMem += WriteNewLineObject (lpMem, RangePQRTag);
        lpMem += WriteObject (lpMem, RangePQR);

     //********** /MatrixPQR
        lpMem += WriteNewLineObject (lpMem, MatrixPQRTag);
        lpMem += WriteObject (lpMem, MatrixPQR);
    }
    else
    {
    //********** /RangePQR
        lpMem += WriteNewLineObject (lpMem, RangePQRTag);
        lpMem += WriteObject (lpMem, BeginArray);
        for (i = 0; i < 3; i++)
        {
            lpMem += WriteFloat (lpMem, (double) 0);
            lpMem += WriteFloat (lpMem, (double)(whitePoint[i]));
        }
        lpMem += WriteObject (lpMem, EndArray);
    //********** /MatrixPQR
        lpMem += WriteNewLineObject (lpMem, MatrixPQRTag);
        lpMem += WriteObject (lpMem, Identity);
    }
//********** /TransformPQR
    lpMem += WriteNewLineObject (lpMem, TransformPQRTag);
    lpMem += WriteObject (lpMem, BeginArray);
    for (i = 0; i < 3; i++)
    {
        lpMem += WriteObject (lpMem, BeginFunction);
        lpMem += WriteObject (lpMem,
            (icAbsoluteColorimetric != Intent) ? TransformPQR[i] : NullOp);
        lpMem += WriteObject (lpMem, EndFunction);
    }
    lpMem += WriteObject (lpMem, EndArray);
   
    return (SINT)(lpMem - lpOldMem);
} 

SINT
SendCRDABC(MEMPTR lpMem, MEMPTR PublicArrayName, CSIG pcs, SINT nInputCh,
           MEMPTR Buff, LPSFLOAT e, CSIG LutTag, BOOL bAllowBinary)
{
    MEMPTR  lpOldMem;
    SINT    i, j;
    double TempMatrixABC[9];
    MEMPTR lpTable;
    MEMPTR lpLineStart;
    lpOldMem = lpMem;

 //********** /RangeABC
    lpMem += WriteNewLineObject (lpMem, RangeABCTag);
    lpMem += WriteObject (lpMem, RangeABC);
 //********** /MatrixABC
    lpMem += WriteNewLineObject (lpMem, MatrixABCTag);
    if (pcs == icSigXYZData)
    {
        lpMem += WriteObject (lpMem, BeginArray);
        if (e)
        {
            for (i = 0; i < 3; i++)
            {
                for (j = 0; j < 3; j++)
                {
                    lpMem += WriteFloat (lpMem, e[i + j * 3]);
                }
            }
        }
        else
        {
            if (LutTag == icSigLut8Type)
            {
                lpTable = (MEMPTR) & ((lpcpLut8Type) Buff)->lut.e00;
            } else
            {
                lpTable = (MEMPTR) & ((lpcpLut16Type) Buff)->lut.e00;
            }
            for (i = 0; i < 9; i++)
            {
                TempMatrixABC[i] = ((double) si16f16toSFLOAT (lpTable)) / CIEXYZRange;
                lpTable += sizeof (icS15Fixed16Number);
            }
            for (i = 0; i < 3; i++)
            {
                for (j = 0; j < 3; j++)
                {
                    lpMem += WriteFloat (lpMem, TempMatrixABC[i + j * 3]);
                }
            }
        }
        lpMem += WriteObject (lpMem, EndArray);
    } else
    {
        lpMem += WriteObject (lpMem, MatrixABCLabCRD);
    }
 //********** /EncodeABC
    if (nInputCh == 0)
        return (SINT)(lpMem - lpOldMem);

    lpLineStart = lpMem;
    lpMem += WriteNewLineObject (lpMem, EncodeABCTag);
    lpMem += WriteObject (lpMem, BeginArray);
    for (i = 0; i < nInputCh; i++)
    {
        lpLineStart = lpMem;
        lpMem += WriteNewLineObject (lpMem, BeginFunction);
        if (pcs == icSigLabData)
        {
            lpMem += WriteObject (lpMem,
                                  (0 == i) ? EncodeABCLab1 : EncodeABCLab2);
        }
        lpMem += WriteObject (lpMem, StartClip);
        if (e)
            lpMem += WriteObject (lpMem, PreViewInArray);
        else
            lpMem += WriteObject (lpMem, InputArray);
        lpMem += WriteObjectN (lpMem, (MEMPTR) PublicArrayName, lstrlen (PublicArrayName));
        lpMem += WriteInt (lpMem, i);

        if (!bAllowBinary)              // Output ASCII CRD
        {
            lpMem += WriteNewLineObject (lpMem, IndexArray);
        } else
        {                               // Output BINARY CRD
            if (LutTag == icSigLut8Type)
            {
                lpMem += WriteObject (lpMem, IndexArray);
            } else
            {
                lpMem += WriteObject (lpMem, IndexArray16b);
            }
        }
        lpMem += WriteObject (lpMem, (LutTag == icSigLut8Type) ? 
                              Scale8 : Scale16);
        lpMem += WriteObject (lpMem, EndClip);
        lpMem += WriteObject (lpMem, EndFunction);
    }
    lpMem += WriteObject (lpMem, EndArray);
    return (SINT)(lpMem - lpOldMem);
}

SINT
SendCRDBWPoint(MEMPTR lpMem, LPSFLOAT whitePoint)
{
    MEMPTR  lpOldMem;
    SINT    i;

    lpOldMem = lpMem;

//********** /BlackPoint
    lpMem += WriteNewLineObject (lpMem, BlackPointTag);
    lpMem += WriteObject (lpMem, BlackPoint);

//********** /WhitePoint
    lpMem += WriteNewLineObject (lpMem, WhitePointTag);
    lpMem += WriteObject (lpMem, BeginArray);
    for (i = 0; i < 3; i++)
    {
        lpMem += WriteFloat (lpMem, (double)(whitePoint[i]));
    }
    lpMem += WriteObject (lpMem, EndArray);
    return (SINT)(lpMem - lpOldMem);
}

SINT SendCRDOutputTable(MEMPTR lpMem, MEMPTR PublicArrayName, 
        SINT nOutputCh, CSIG LutTag, BOOL bHost, BOOL bAllowBinary)
{
    MEMPTR  lpOldMem;
    SINT    i;

    lpOldMem = lpMem;

    for (i = 0; i < nOutputCh; i++)
    {
        lpMem += WriteNewLineObject (lpMem, BeginFunction);
        lpMem += WriteObject (lpMem, Clip01);
        if (bHost)
            lpMem += WriteObject (lpMem, PreViewOutArray);
        else
            lpMem += WriteObject (lpMem, OutputArray);
        lpMem += WriteObjectN (lpMem, (MEMPTR) PublicArrayName, lstrlen (PublicArrayName));
        lpMem += WriteInt (lpMem, i);

        if (!bAllowBinary)               // Output ASCII CRD
        {
            lpMem += WriteObject (lpMem, NewLine);
            if (LutTag == icSigLut8Type)
            {
                lpMem += WriteObject (lpMem, TFunction8);
            } else
            {
                lpMem += WriteObject (lpMem, IndexArray);
                lpMem += WriteObject (lpMem, Scale16);
            }
        } else
        {                               // Output BINARY CRD
            if (LutTag == icSigLut8Type)
            {
                lpMem += WriteObject (lpMem, TFunction8);
            } else
            {
                lpMem += WriteObject (lpMem, IndexArray16b);
                lpMem += WriteObject (lpMem, Scale16);
            }
        }

        lpMem += WriteObject (lpMem, EndFunction);
    }
    return (SINT)(lpMem - lpOldMem);
}

//========================================================================
/***************************************************************************
*                  GetPS2PreviewColorRenderingDictionary
*  function:
*    This is the main function that creates proofing CRD.
*    It does the following:
*       1) Creates host TargetCRD, TargetCSA and DevCRD.
*       2) Create proofing CRD by sampling TargetCRD TargetCSA and DevCRD.
*       3) Uses TargetCRD's input table as proofingCRD's input table.
*       4) Uses DevCRD's output table as proofingCRD's output table.
*       5) Sample data is XYZ or Lab, depends on PCS of TargetCRD.
*
*  parameters:
*       CHANDLE  cpDev        -- handle to Target icc profile.
*       CHANDLE  cpTarget     -- handle to Dev icc profile.
*       DWORD    Intent       -- intent 
*       MEMPTR   lpMem        -- pointer to buffer for proofCRD,
*                                NULL means query buffer size.
*       LPDWORD  lpcbSize     -- as input: current buffer size
*                             -- as output: real proofCRD size.
*       BOOL     bAllowBinary -- create a ascii or binary proofCRD.
*
*  returns:
*       BOOL                  -- TRUE/FALSE
***************************************************************************/

BOOL EXTERN
GetPS2PreviewColorRenderingDictionary (CHANDLE cpDev,
                                CHANDLE cpTarget,
                                DWORD Intent,
                                MEMPTR lpMem,
                                LPDWORD lpcbSize,
                                BOOL bAllowBinary)
{
    MEMPTR    lpTargetCRD, lpTargetCSA, lpDevCRD;
    DWORD     cbTargetCRD, cbTargetCSA, cbDevCRD;
    HGLOBAL   hTargetCRD, hTargetCSA, hDevCRD;
    BOOL      Success = FALSE;
    float     Input[MAXCHANNELS];
    float     Output[MAXCHANNELS];
    float     Temp[MAXCHANNELS];
    int       i, j, k, l;
    MEMPTR    lpLineStart;
    MEMPTR    lpOldMem;
    CSIG      ColorSpace;
    CSIG      DevColorSpace;
    static CSIG      IntentTag;
    static SINT      PreviewCRDGrid;
    SINT      OutArraySize, InArraySize;
    char      PublicArrayName[TempBfSize];
    SINT      TargetGrids, DevGrids;

    // First pass, return the size of Previewind CRD.
    if (lpMem == NULL)
    {
        SINT   dwOutArraySizr = 0;

        i = 3;      // Default output channal;
        if ((GetCPDevSpace (cpDev, (LPCSIG) & DevColorSpace)) &&
                          (DevColorSpace == icSigCmykData))
        {
            i = 4;
        }

        // Get the input array size IntentTag and Grid of the Target icc profile.
        if (!GetCRDInputOutputArraySize(cpTarget, Intent,
            &InArraySize, NULL, &IntentTag, &TargetGrids ))
            return FALSE;

        // Get the output array size IntentTag and Grid of the Dev icc profile.
        if (!GetCRDInputOutputArraySize(cpDev, Intent,
            NULL, &OutArraySize, &IntentTag, &DevGrids ))
            return FALSE;

        PreviewCRDGrid = (TargetGrids > DevGrids)? TargetGrids: DevGrids;

        // Min proofing CRD grid will be PREVIEWCRDGRID
        if (PreviewCRDGrid < PREVIEWCRDGRID)
            PreviewCRDGrid = PREVIEWCRDGRID;
        *lpcbSize = PreviewCRDGrid * PreviewCRDGrid * PreviewCRDGrid * 
                    i * 2 +           // CLUT size (Hex output)
                    OutArraySize +    // Output Array size
                    InArraySize  +    // Input Array size
                    4096;             // Extra PostScript staff.
         return (TRUE);
    }

    // Second pass, return the Previewind CRD.
    lpOldMem = lpMem;

    //Query the sizes of Host TargetCRD, TargetCSA and DevCRD.
    if (!(GetHostColorRenderingDictionary (cpTarget, Intent, NULL, &cbTargetCRD)) ||
        !(GetHostColorSpaceArray (cpTarget, Intent, NULL, &cbTargetCSA)) ||
        !(GetHostColorRenderingDictionary (cpDev, Intent, NULL, &cbDevCRD)))
    {
        return (Success);
    }

    //Alloc the buffers for Host TargetCRD, TargetCSA and DevCRD.
    hTargetCRD = hTargetCSA = hDevCRD = 0;
    if (!MemAlloc (cbTargetCRD, (HGLOBAL FAR *)&hTargetCRD, (LPMEMPTR)&lpTargetCRD) ||
        !MemAlloc (cbTargetCSA, (HGLOBAL FAR *)&hTargetCSA, (LPMEMPTR)&lpTargetCSA) ||
        !MemAlloc (cbDevCRD, (HGLOBAL FAR *)&hDevCRD, (LPMEMPTR)&lpDevCRD))
    {
        goto Done;
    }

    //Build Host TargetCRD, TargetCSA and DevCRD.
    if (!(GetHostColorRenderingDictionary (cpTarget, Intent, lpTargetCRD, &cbTargetCRD)) ||
        !(GetHostColorSpaceArray (cpTarget, Intent, lpTargetCSA, &cbTargetCSA)) ||
        !(GetHostColorRenderingDictionary (cpDev, Intent, lpDevCRD, &cbDevCRD)))
    {
        goto Done;
    }

//  Build Proofing CRD based on Host TargetCRD TargetCSA and DevCRD.
//  We use TargetCRD input tables and matrix as the
//  input tables and matrix of the ProofCRD.
//  We use DevCRD output tables as the output tables of the ProofCRD.

//******** Define golbal array used in EncodeABC and RenderTaber
    GetPublicArrayName (cpDev, IntentTag, PublicArrayName);
    lpMem += WriteNewLineObject (lpMem, CRDBegin);

    lpMem += EnableGlobalDict(lpMem);
    lpMem += BeginGlobalDict(lpMem);

    lpMem += CreateInputArray (lpMem, (SINT)0, (SINT)0, (MEMPTR)PublicArrayName, 
             (CSIG)0, NULL, bAllowBinary, lpTargetCRD);

    lpMem += CreateOutputArray (lpMem, (SINT)0, (SINT)0, (SINT)0, 
             (MEMPTR)PublicArrayName, (CSIG)0, NULL, bAllowBinary, lpDevCRD);

    lpMem += EndGlobalDict(lpMem);

//************* Start writing  CRD  ****************************
    lpMem += WriteNewLineObject (lpMem, BeginDict);    // Begin dictionary
    lpMem += WriteObject (lpMem, DictType); // Dictionary type

    lpMem += WriteNewLineObject (lpMem, IntentType); // RenderingIntent
    switch (Intent)
    {
        case icPerceptual:
            lpMem += WriteObject (lpMem, IntentPer);
            break;

        case icSaturation:
            lpMem += WriteObject (lpMem, IntentSat);
            break;

        case icRelativeColorimetric:
            lpMem += WriteObject (lpMem, IntentRCol);
            break;

        case icAbsoluteColorimetric:
            lpMem += WriteObject (lpMem, IntentACol);
            break;
    }

//********** Send Black/White Point.
    lpMem += SendCRDBWPoint(lpMem, 
        ((LPHOSTCLUT)lpTargetCRD)->whitePoint);

//********** Send PQR - For White Point correction
    lpMem += SendCRDPQR(lpMem, Intent, 
        ((LPHOSTCLUT)lpTargetCRD)->whitePoint);

//********** Send LMN - For Absolute Colorimetric use WhitePoint's XYZs
    lpMem += SendCRDLMN(lpMem, Intent, 
        ((LPHOSTCLUT)lpTargetCRD)->whitePoint,
        ((LPHOSTCLUT)lpTargetCRD)->mediaWP,
        ((LPHOSTCLUT)lpTargetCRD)->pcs);

//********** Create MatrixABC and  EncodeABC  stuff
    lpMem += SendCRDABC(lpMem, PublicArrayName, 
        ((LPHOSTCLUT)lpTargetCRD)->pcs,
        ((LPHOSTCLUT)lpTargetCRD)->inputChan,
        NULL,
        ((LPHOSTCLUT)lpTargetCRD)->e,
        (((LPHOSTCLUT)lpTargetCRD)->lutBits == 8)? icSigLut8Type:icSigLut16Type,
        bAllowBinary);

//********** /RenderTable
    lpMem += WriteNewLineObject (lpMem, RenderTableTag);
    lpMem += WriteObject (lpMem, BeginArray);

    lpMem += WriteInt (lpMem, PreviewCRDGrid);  // Send down Na
    lpMem += WriteInt (lpMem, PreviewCRDGrid);  // Send down Nb
    lpMem += WriteInt (lpMem, PreviewCRDGrid);  // Send down Nc

    lpLineStart = lpMem;
    lpMem += WriteNewLineObject (lpMem, BeginArray);
    ColorSpace = ((LPHOSTCLUT)lpDevCRD)->pcs;
    for (i = 0; i < PreviewCRDGrid; i++)        // Na strings should be sent
    {
        lpMem += WriteObject (lpMem, NewLine);
        lpLineStart = lpMem;
        if (bAllowBinary)
        {
            lpMem += WriteStringToken (lpMem, 143, 
                PreviewCRDGrid * PreviewCRDGrid * ((LPHOSTCLUT)lpDevCRD)->outputChan);
        }
        else
        {
            lpMem += WriteObject (lpMem, BeginString);
        }
        Input[0] = ((float)i) / (PreviewCRDGrid - 1);
        for (j = 0; j < PreviewCRDGrid; j++)
        {
            Input[1] = ((float)j) / (PreviewCRDGrid - 1);
            for (k = 0; k < PreviewCRDGrid; k++)
            {
                Input[2] = ((float)k) / (PreviewCRDGrid - 1);

                DoHostConversionCRD ((LPHOSTCLUT)lpTargetCRD, NULL, Input, Output, ColorSpace, 1);
                DoHostConversionCSA ((LPHOSTCLUT)lpTargetCSA, Output, Temp);
                DoHostConversionCRD ((LPHOSTCLUT)lpDevCRD, (LPHOSTCLUT)lpTargetCSA, 
                                     Temp, Output, 0, 0);
                for (l = 0; l < ((LPHOSTCLUT)lpDevCRD)->outputChan; l++)
                {
                    if (bAllowBinary)
                    {
                        *lpMem++ = (BYTES)(Output[l]*255);
                    }
                    else
                    {
                        lpMem += WriteHex (lpMem, (USHORT)(Output[l]*255));
                        if (((SINT) (lpMem - lpLineStart)) > MAX_LINELENG)
                        {
                            lpLineStart = lpMem;
                            lpMem += WriteObject (lpMem, NewLine);
                        }
                    }
                }
            }
        }
        if (!bAllowBinary)
            lpMem += WriteObject (lpMem, EndString);
    }
    lpMem += WriteNewLineObject (lpMem, EndArray);
    lpMem += WriteInt (lpMem, ((LPHOSTCLUT)lpDevCRD)->outputChan);

//********** Send Output Table.
    lpMem += SendCRDOutputTable(lpMem, PublicArrayName, 
        ((LPHOSTCLUT)lpDevCRD)->outputChan,
        (((LPHOSTCLUT)lpDevCRD)->lutBits == 8)? icSigLut8Type:icSigLut16Type,
        TRUE,
        bAllowBinary);


    lpMem += WriteNewLineObject (lpMem, EndArray);
    lpMem += WriteObject (lpMem, EndDict); // End dictionary definition
    lpMem += WriteNewLineObject (lpMem, CRDEnd);
    Success = TRUE;

Done:
    *lpcbSize = (DWORD)(lpMem - lpOldMem);

    if (hTargetCRD)
         MemFree(hTargetCRD);
    if (hTargetCSA)
         MemFree(hTargetCSA);
    if (hDevCRD)
         MemFree(hDevCRD);
    return (Success);
}
