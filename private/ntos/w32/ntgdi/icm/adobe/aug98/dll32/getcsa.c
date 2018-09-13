#include "generic.h"
#include "icmstr.h"

#pragma code_seg(_ICMSEG)

static char  *DecodeABCLab[] = {"50 mul 16 add 116 div ", 
                               "128 mul 128 sub 500 div", 
                               "128 mul 128 sub 200 div"};

#pragma optimize("",off)

BOOL IsSRGB( CHANDLE cp );

/**************************************************************************/

SINT CreateColSpArray (CHANDLE cp, MEMPTR lpMem, CSIG CPTag, BOOL AllowBinary);
SINT CreateColSpProc (CHANDLE cp, MEMPTR lpMem, CSIG CPTag, BOOL AllowBinary);
SINT CreateFloatString (CHANDLE cp, MEMPTR lpMem, CSIG CPTag);
static SINT SendCSABWPoint(MEMPTR lpMem, CSIG Intent, 
    LPSFLOAT IlluminantWP, LPSFLOAT MediaWP);
static void GetMediaWP(CHANDLE cp, CSIG InputIntent, 
    LPSFLOAT IlluminantWP, LPSFLOAT MediaWP);

/***************************************************************************
*                           GetDevideRGB
*  function:
*    this is the function which creates the DeviceRGB ColorSpace (CS)
*  prototype:
*       BOOL EXTERN GetDeviceRGB(
*                          MEMPTR       lpMem,
*                          BOOL         AllowBinary)
*  parameters:
*       lpMem       --  Pointer to the memory block
*       AllowBinary --  1: binary CS allowed,  0: only ascii CS allowed.
*  returns:
*  returns:
*       BOOL        --  TRUE if the function was successful,
*                       FALSE otherwise.
***************************************************************************/
static BOOL
GetDeviceRGB (
              MEMPTR lpMem,
              LPDWORD lpcbSize,
              DWORD InpDrvClrSp,
              BOOL  BackupCSA)
{
    MEMPTR lpOldMem = lpMem;
    if ((InpDrvClrSp != icSigRgbData) &&
        (InpDrvClrSp != icSigDefData) &&
        (InpDrvClrSp != 0))
        return FALSE;

    if (lpMem == NULL)
    {
        *lpcbSize = lstrlen (DeviceRGBTag) + 8;
        return (TRUE);
    }

    if (BackupCSA)
    {
        lpMem += WriteNewLineObject (lpMem, NotSupportDEFG_S);
    }

    lpMem += WriteNewLineObject (lpMem, DeviceRGBTag);

    if (BackupCSA)
    {
        lpMem += WriteNewLineObject (lpMem, SupportDEFG_E);
    }

    *lpcbSize = (DWORD) (lpMem - lpOldMem);
    return (TRUE);
}
/***************************************************************************
*                           GetDevideCMYK
*  function:
*    this is the function which creates the DeviceCMYK ColorSpace (CS)
*  prototype:
*       BOOL EXTERN GetDeviceCMYK(
*                          MEMPTR       lpMem,
*                          BOOL         AllowBinary)
*  parameters:
*       lpMem       --  Pointer to the memory block
*       AllowBinary --  1: binary CS allowed,  0: only ascii CS allowed.
*  returns:
*  returns:
*       BOOL        --  TRUE if the function was successful,
*                       FALSE otherwise.
***************************************************************************/

static BOOL
GetDeviceCMYK (
               MEMPTR lpMem,
               LPDWORD lpcbSize,
               DWORD InpDrvClrSp)
{
    MEMPTR lpOldMem = lpMem;
    if ((InpDrvClrSp != icSigCmykData) &&
        (InpDrvClrSp != 0))
        return FALSE;
    if (lpMem == NULL)
    {
        *lpcbSize = lstrlen (DeviceCMYKTag) + 8;
        return (TRUE);
    }
    lpMem += WriteNewLineObject (lpMem, DeviceCMYKTag);

    *lpcbSize = (DWORD) (lpMem - lpOldMem);
    return (TRUE);

}
/***************************************************************************
*                           GetDeviceGray
***************************************************************************/

static BOOL
GetDeviceGray (
               MEMPTR lpMem,
               LPDWORD lpcbSize,
               DWORD InpDrvClrSp)
{
    MEMPTR lpOldMem = lpMem;
    if ((InpDrvClrSp == icSigRgbData) ||
        (InpDrvClrSp == icSigDefData) ||
        (InpDrvClrSp == 0))
    {
        if (lpMem == NULL)
        {
            *lpcbSize = lstrlen (DeviceRGBTag) + 8;
            return (TRUE);
        }
        lpMem += WriteNewLineObject (lpMem, DeviceRGBTag);

        *lpcbSize = (DWORD) (lpMem - lpOldMem);
        return (TRUE);
    } else if (InpDrvClrSp == icSigGrayData)
    {
        if (lpMem == NULL)
        {
            *lpcbSize = lstrlen (DeviceGrayTag) + 8;
            return (TRUE);
        }
        lpMem += WriteNewLineObject (lpMem, DeviceGrayTag);

        *lpcbSize = (DWORD) (lpMem - lpOldMem);
        return (TRUE);
    }
}
/***************************************************************************
*                           GetPublicArrayName
***************************************************************************/

SINT
GetPublicArrayName (CHANDLE cp, CSIG IntentSig, MEMPTR PublicArrayName)
{
    MEMPTR OldPtr;
    OldPtr = PublicArrayName;
    PublicArrayName[0] = 0;
    MemCopy (PublicArrayName, (MEMPTR) & IntentSig, sizeof (CSIG));
    PublicArrayName += sizeof (CSIG);
    PublicArrayName[0] = 0;
    return (PublicArrayName - OldPtr);
}

static SINT SendCSABWPoint(MEMPTR lpMem, CSIG Intent, 
                    LPSFLOAT IlluminantWP, LPSFLOAT MediaWP)
{
    SINT   i;
    MEMPTR lpOldMem = lpMem;

 //********** /BlackPoint
    lpMem += WriteNewLineObject (lpMem, BlackPointTag);
    lpMem += WriteObject (lpMem, BlackPoint);

 //********** /WhitePoint
    lpMem += WriteNewLineObject (lpMem, WhitePointTag);
    lpMem += WriteObject (lpMem, BeginArray);
    for (i = 0; i < 3; i++)
    {
        if (Intent == icAbsoluteColorimetric)
        {
            lpMem += WriteFloat (lpMem, (double) MediaWP[i]);
        }
        else
        {
            lpMem += WriteFloat (lpMem, (double) IlluminantWP[i]);
        }
    }
    lpMem += WriteObject (lpMem, EndArray);
    return (SINT)(lpMem - lpOldMem);
}

static void GetMediaWP(CHANDLE cp, CSIG InputIntent, 
                       LPSFLOAT IlluminantWP, LPSFLOAT MediaWP)
{
    if (InputIntent == icAbsoluteColorimetric)
    {
        if (!GetCPMediaWhitePoint (cp, (LPSFLOAT) & MediaWP)) // .. Media WhitePoint
        {
            MediaWP[0] = IlluminantWP[0];
            MediaWP[1] = IlluminantWP[1];
            MediaWP[2] = IlluminantWP[2];
        }
    }
}

SINT  BeginGlobalDict(MEMPTR lpMem)
{
    MEMPTR lpOldMem = lpMem;

    lpMem += WriteNewLineObject (lpMem, GlobalDictOp);
    lpMem += WriteObject (lpMem, BeginOp);

    return (lpMem - lpOldMem);
}

SINT  EndGlobalDict(MEMPTR lpMem)
{
    MEMPTR lpOldMem = lpMem;

    lpMem += WriteNewLineObject (lpMem, EndOp);
    lpMem += WriteObject (lpMem, SetGlobalOp);

    return (lpMem - lpOldMem);
}

SINT  EnableGlobalDict(MEMPTR lpMem)
{
    MEMPTR lpOldMem = lpMem;

    lpMem += WriteNewLineObject (lpMem, CurrentGlobalOp);
    lpMem += WriteObject (lpMem, TrueOp);
    lpMem += WriteObject (lpMem, SetGlobalOp);
    return (lpMem - lpOldMem);
}

/***************************************************************************
*                           GetPS2CSA_DEFG
*  function:
*    this is the function which creates the CIEBasedDEF(G) ColorSpace (CS)
*    from the data supplied in the RGB or CMYK Input Profile.
*  prototype:
*       GetPS2CSA_DEFG(
*                       CHANDLE     cp,
*                       MEMPTR      lpMem,
*                       LPDWORD     lpcbSize,
*                       int         Type
*                       BOOL        AllowBinary)
*  parameters:
*       cp          --  Color Profile handle
*       lpMem       --  Pointer to the memory block. If this point is NULL,
*                       require buffer size.
*       lpcbSize    --  Size of the memory block
*       Type        --  CieBasedDEF or CieBasedDEFG.
*       AllowBinary --  1: binary CSA allowed,  0: only ascii CSA allowed.
*  returns:
*       BOOL        --  TRUE if the function was successful,
*                       FALSE otherwise.
***************************************************************************/
static BOOL
GetPS2CSA_DEFG (
                CHANDLE cp,
                MEMPTR lpMem,
                LPDWORD lpcbSize,
                CSIG InputIntent,
                SINT Index,
                int Type,
                BOOL AllowBinary)
{
    CSIG PCS, LutTag;
    CSIG IntentSig;
    SFLOAT IlluminantWP[3];
    SFLOAT MediaWP[3];
    SINT nInputCh, nOutputCh, nGrids, SecondGrids;
    SINT nInputTable, nOutputTable, nNumbers;
    SINT i, j, k;
    MEMPTR lpTable;
    MEMPTR lpOldMem = lpMem;
    MEMPTR lpLut = NULL;
    MEMPTR lpLineStart;
    HGLOBAL hLut = 0;
    SINT LutSize;
    char PublicArrayName[TempBfSize];
 // Check if we can generate the CS.
 // Required  tags are: red, green and blue Colorants.
 // As for TRC tags - we are quite flexible here - if we cannot find the
 // required tag - we assume the linear responce
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

 // Level 2 printers support only tri-component CIEBasedABC input,
 // but can have either 3 or 4 output channels.
    if (!(nOutputCh == 3) ||
        !((nInputCh == 3) && (Type == TYPE_CIEBASEDDEF)) &&
        !((nInputCh == 4) && (Type == TYPE_CIEBASEDDEFG)))
    {
        SetCPLastError (CP_POSTSCRIPT_ERR);
        MemFree (hLut);
        return (FALSE);
    }
    if (lpMem == NULL)                  // This is a size request
    {
        if (Type == TYPE_CIEBASEDDEFG)
            *lpcbSize = nOutputCh * nGrids * nGrids * nGrids * nGrids * 2;  // LUT HEX bytes
        else
            *lpcbSize = nOutputCh * nGrids * nGrids * nGrids * 2;   // LUT HEX bytes

        *lpcbSize = *lpcbSize +
            nInputCh * nInputTable * 6 +
            nOutputCh * nOutputTable * 6 +  // Number of INT bytes
            nInputCh * (lstrlen (IndexArray) +
                        lstrlen (StartClip) +
                        lstrlen (EndClip)) +
            nOutputCh * (lstrlen (IndexArray) +
                         lstrlen (StartClip) +
                         lstrlen (EndClip)) +
            4096;                       // + other PS stuff


        return (TRUE);
    }
 // Get info about Illuminant White Point from the header
    GetCPWhitePoint (cp, (LPSFLOAT) & IlluminantWP);          // .. Illuminant

 // Support absolute whitePoint
    GetMediaWP(cp, InputIntent, IlluminantWP, MediaWP);

 //*********** Testing CieBasedDEFG support
    lpMem += WriteNewLineObject (lpMem, TestingDEFG);

 //*********** Creating global data
    GetPublicArrayName (cp, IntentSig, PublicArrayName);
    if (Type == TYPE_CIEBASEDDEFG)
        lpMem += WriteNewLineObject (lpMem, CieBasedDEFGBegin);
    else
        lpMem += WriteNewLineObject (lpMem, CieBasedDEFBegin);

    lpMem += EnableGlobalDict(lpMem);
    lpMem += WriteNewLineObject (lpMem, SupportDEFG_S);
    lpMem += BeginGlobalDict(lpMem);

    lpMem += CreateInputArray (lpMem, nInputCh, nInputTable,
             (MEMPTR) PublicArrayName, LutTag, lpLut, AllowBinary, NULL);

    if (Type == TYPE_CIEBASEDDEFG)
    {
        i = nInputTable * nInputCh +
            nGrids * nGrids * nGrids * nGrids * nOutputCh;
    } else
    {
        i = nInputTable * nInputCh +
            nGrids * nGrids * nGrids * nOutputCh;
    }
    lpMem += CreateOutputArray (lpMem, nOutputCh, nOutputTable, i,
             (MEMPTR) PublicArrayName, LutTag, lpLut, AllowBinary, NULL);

    lpMem += WriteNewLineObject (lpMem, EndOp);
    lpMem += WriteNewLineObject (lpMem, SupportDEFG_E);
    lpMem += WriteNewLineObject (lpMem, SetGlobalOp);
    lpMem += WriteNewLineObject (lpMem, SupportDEFG_S);

 //*********** Start creating the ColorSpace
    lpMem += WriteNewLineObject (lpMem, BeginArray);   // Begin array

 //********** /CIEBasedDEF(G)
    if (Type == TYPE_CIEBASEDDEFG)
        lpMem += WriteObject (lpMem, CIEBasedDEFGTag);
    else
        lpMem += WriteObject (lpMem, CIEBasedDEFTag);
    lpMem += WriteObject (lpMem, BeginDict);    // Begin dictionary

 //********** Black/White Point
    lpMem += SendCSABWPoint(lpMem, InputIntent, IlluminantWP, MediaWP);

 //********** /DecodeDEF(G)
    lpLineStart = lpMem;
    if (Type == TYPE_CIEBASEDDEFG)
        lpMem += WriteNewLineObject (lpMem, DecodeDEFGTag);
    else
        lpMem += WriteNewLineObject (lpMem, DecodeDEFTag);

    lpMem += WriteObject (lpMem, BeginArray);
    for (i = 0; i < nInputCh; i++)
    {
        lpLineStart = lpMem;

        lpMem += WriteNewLineObject (lpMem, BeginFunction);
#if 0
        if (PCS == icSigLabData)
        {
            lpMem += WriteObject (lpMem,
                                  (0 == i) ? EncodeABCLab1 : EncodeABCLab2);
        }
#endif
        lpMem += WriteObject (lpMem, StartClip);
        lpMem += WriteObject (lpMem, InputArray);
        lpMem += WriteObjectN (lpMem, (MEMPTR) PublicArrayName, lstrlen (PublicArrayName));
        lpMem += WriteInt (lpMem, i);

        if (!AllowBinary)               // Output ASCII
        {
            lpMem += WriteObject (lpMem, IndexArray);
        } else
        {                               // Output BINARY
            if (LutTag == icSigLut8Type)
            {
                lpMem += WriteObject (lpMem, IndexArray);
            } else
            {
                lpMem += WriteObject (lpMem, IndexArray16b);
            }
        }
        lpMem += WriteObject (lpMem, (LutTag == icSigLut8Type) ? Scale8 : Scale16);
        lpMem += WriteObject (lpMem, EndClip);
        lpMem += WriteObject (lpMem, EndFunction);
    }
    lpMem += WriteObject (lpMem, EndArray);

 //********** /Table
    lpMem += WriteNewLineObject (lpMem, TableTag);
    lpMem += WriteObject (lpMem, BeginArray);

    lpMem += WriteInt (lpMem, nGrids);  // Send down Nh
    lpMem += WriteInt (lpMem, nGrids);  // Send down Ni
    lpMem += WriteInt (lpMem, nGrids);  // Send down Nj
    nNumbers = nGrids * nGrids * nOutputCh;
    SecondGrids = 1;
    if (Type == TYPE_CIEBASEDDEFG)
    {
        lpMem += WriteInt (lpMem, nGrids);  // Send down Nk
//       nNumbers = nGrids * nGrids * nGrids * nOutputCh ;
        SecondGrids = nGrids;
    }
    lpMem += WriteNewLineObject (lpMem, BeginArray);
    for (i = 0; i < nGrids; i++)        // Nh strings should be sent
    {
        if (Type == TYPE_CIEBASEDDEFG)
        {
            lpMem += WriteNewLineObject (lpMem, BeginArray);
        }
        for (k = 0; k < SecondGrids; k++)
        {
            lpLineStart = lpMem;
            lpMem += WriteObject (lpMem, NewLine);
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

            if (!AllowBinary)           // Output ASCII
            {
                lpMem += WriteObject (lpMem, BeginString);
                if (LutTag == icSigLut8Type)
                    lpMem += WriteHexBuffer (lpMem, lpTable, lpLineStart, nNumbers);
                else
                {
                    for (j = 0; j < nNumbers; j++)
                    {
                        lpMem += WriteHex (lpMem, ui16toSINT (lpTable) / 256);
                        lpTable += sizeof (icUInt16Number);
                        if (((SINT) (lpMem - lpLineStart)) > MAX_LINELENG)
                        {
                            lpLineStart = lpMem;
                            lpMem += WriteObject (lpMem, NewLine);
                        }
                    }
                }
                lpMem += WriteObject (lpMem, EndString);
            } else
            {                           // Output BINARY
                lpMem += WriteStringToken (lpMem, 143, nNumbers);
                if (LutTag == icSigLut8Type)
                    lpMem += WriteByteString (lpMem, lpTable, nNumbers);
                else
                    lpMem += WriteInt2ByteString (lpMem, lpTable, nNumbers);
            }
            lpMem += WriteObject (lpMem, NewLine);
        }
        if (Type == TYPE_CIEBASEDDEFG)
        {
            lpMem += WriteObject (lpMem, EndArray);
        }
    }
    lpMem += WriteObject (lpMem, EndArray);
    lpMem += WriteObject (lpMem, EndArray); // End array

 //********** /DecodeABC
    lpLineStart = lpMem;
    lpMem += WriteNewLineObject (lpMem, DecodeABCTag);
    lpMem += WriteObject (lpMem, BeginArray);
    for (i = 0; i < nOutputCh; i++)
    {
        lpLineStart = lpMem;
        lpMem += WriteNewLineObject (lpMem, BeginFunction);
        lpMem += WriteObject (lpMem, Clip01);
        lpMem += WriteObject (lpMem, OutputArray);
        lpMem += WriteObjectN (lpMem, (MEMPTR) PublicArrayName, lstrlen (PublicArrayName));
        lpMem += WriteInt (lpMem, i);

        if (!AllowBinary)               // Output ASCII CRD
        {
            lpMem += WriteObject (lpMem, NewLine);

            if (LutTag == icSigLut8Type)
            {
                lpMem += WriteObject (lpMem, TFunction8XYZ);
            } else
            {
                lpMem += WriteObject (lpMem, IndexArray);
                lpMem += WriteObject (lpMem, Scale16XYZ);
            }
        } else
        {                               // Output BINARY CRD
            if (LutTag == icSigLut8Type)
            {
                lpMem += WriteObject (lpMem, TFunction8XYZ);
            } else
            {
                lpMem += WriteObject (lpMem, IndexArray16b);
                lpMem += WriteObject (lpMem, Scale16XYZ);
            }
        }

 // Now, We get CieBasedXYZ output. Output range 0 --> 1.99997
 // If the connection space is absolute XYZ, We need to convert 
 // from relative XYZ to absolute XYZ.
        if ((PCS == icSigXYZData) &&
            (InputIntent == icAbsoluteColorimetric))
        {
            lpMem += WriteFloat (lpMem, (double)MediaWP[i]/IlluminantWP[i]);
            lpMem += WriteObject (lpMem, MulOp); 
        }
 // If the connection space is Lab, We need to convert XYZ to Lab.
        else if (PCS == icSigLabData)
            lpMem += WriteObject (lpMem, DecodeABCLab[i]);
        lpMem += WriteObject (lpMem, EndFunction);
    }
    lpMem += WriteObject (lpMem, EndArray);

    if (PCS == icSigLabData)
    {
 //********** /MatrixABC
        lpMem += WriteNewLineObject (lpMem, MatrixABCTag);
        lpMem += WriteObject (lpMem, MatrixABCLab);

 //********** /DecodeLMN
        lpLineStart = lpMem;
        lpMem += WriteNewLineObject (lpMem, DecodeLMNTag);
        lpMem += WriteObject (lpMem, BeginArray);
        for (i = 0; i < 3; i++)
        {
            lpLineStart = lpMem;
            lpMem += WriteNewLineObject (lpMem, BeginFunction);
            lpMem += WriteObject (lpMem, DecodeLMNLab);
            if (InputIntent == icAbsoluteColorimetric)
                lpMem += WriteFloat (lpMem, (double) MediaWP[i]);
            else 
                lpMem += WriteFloat (lpMem, (double) IlluminantWP[i]);
            lpMem += WriteObject (lpMem, MulOp);
            lpMem += WriteObject (lpMem, EndFunction);
        }
        lpMem += WriteObject (lpMem, EndArray);
    } else
    {
 //********** /RangeLMN
        lpMem += WriteNewLineObject (lpMem, RangeLMNTag);
        lpMem += WriteObject (lpMem, RangeLMN);
    }

 //********** End dictionary definition
    lpMem += WriteNewLineObject (lpMem, EndDict);
    lpMem += WriteObject (lpMem, EndArray);

    if (Type == TYPE_CIEBASEDDEFG)
        lpMem += WriteNewLineObject (lpMem, CieBasedDEFGEnd);
    else
        lpMem += WriteNewLineObject (lpMem, CieBasedDEFEnd);

    lpMem += WriteNewLineObject (lpMem, SupportDEFG_E);

    *lpcbSize = (DWORD) (lpMem - lpOldMem);

    MemFree (hLut);
    return (TRUE);
}

/***************************************************************************
*                           GetPS2CSA_ABC
*  function:
*    this is the function which creates the CIEBasedABC ColorSpace (CS)
*    from the data supplied in the RGB Input Profile.
*  prototype:
*       GetPS2CSA_ABC(
*                       CHANDLE     cp,
*                       MEMPTR      lpMem,
*                       LPDWORD     lpcbSize,
*                       BOOL        AllowBinary)
*  parameters:
*       cp          --  Color Profile handle
*       lpMem       --  Pointer to the memory block. If this point is NULL,
*                       require buffer size.
*       lpcbSize    --  Size of the memory block
*       InputIntent --
*       InpDrvClrSp --
*       AllowBinary --  1: binary CSA allowed,  0: only ascii CSA allowed.
*       BackupCSA   --  1: A CIEBasedDEF has been created, this CSA is a backup 
*                          in case some old printer can not support CIEBasedDEF.
*                       0: No CIEBasedDEF. This is the only CSA.
*  returns:
*       BOOL        --  TRUE if the function was successful,
*                       FALSE otherwise.
***************************************************************************/

BOOL
GetPS2CSA_ABC (CHANDLE cp, MEMPTR lpMem, LPDWORD lpcbSize,
               CSIG InputIntent, DWORD InpDrvClrSp, 
               BOOL AllowBinary, BOOL BackupCSA)
{
    CSIG PCS, Dev;
    SFLOAT IlluminantWP[3];
    SFLOAT MediaWP[3];

    SINT i;
    MEMPTR lpOldMem = lpMem;
    SINT Ret = 0; 
    
 // Check if we can generate the CS.
 // Required  tags are: red, green and blue Colorants.
 // As for TRC tags - we are quite flexible here - if we cannot find the
 // required tag - we assume the linear responce
    if (!GetCPConnSpace (cp, (LPCSIG) & PCS) ||
        !GetCPDevSpace (cp, (LPCSIG) & Dev) ||
        (Dev != icSigRgbData) ||
        !DoesTRCAndColorantTagExist(cp))
    {
        return (FALSE);
    }
    if ((InpDrvClrSp != icSigRgbData) &&
        (InpDrvClrSp != icSigDefData) &&
        (InpDrvClrSp != 0))
    {
        return (FALSE);
    }
 // Estimate the memory size required to hold CS

    if (lpMem == NULL)                  // This is a size request
    {
        *lpcbSize = 65530;
        return (TRUE);
    }

 // Get info about Illuminant White Point from the header
    GetCPWhitePoint (cp, (LPSFLOAT) & IlluminantWP);     // .. Illuminant
                                                            
 // Support absolute whitePoint
    GetMediaWP(cp, InputIntent, IlluminantWP, MediaWP);

 //*********** Creating global data

    lpMem += WriteNewLineObject (lpMem, CieBasedABCBegin);

    if (IsSRGB(cp))
    {
       lpMem += WriteNewLineObject (lpMem, AdobeCSA);
    }
    else
    {
       lpMem += EnableGlobalDict(lpMem);
       
       if (BackupCSA)
       {
           lpMem += WriteNewLineObject (lpMem, NotSupportDEFG_S);
       }
   
       lpMem += BeginGlobalDict(lpMem);
   
       lpMem += CreateColSpArray (cp, lpMem, icSigRedTRCTag, AllowBinary);
       lpMem += CreateColSpArray (cp, lpMem, icSigGreenTRCTag, AllowBinary);
       lpMem += CreateColSpArray (cp, lpMem, icSigBlueTRCTag, AllowBinary);
   
       lpMem += WriteNewLineObject (lpMem, EndOp);
   
       if (BackupCSA)
       {
           lpMem += WriteNewLineObject (lpMem, SupportDEFG_E);
       }
       lpMem += WriteNewLineObject (lpMem, SetGlobalOp);
   
       if (BackupCSA)
       {
           lpMem += WriteNewLineObject (lpMem, NotSupportDEFG_S);
       }
   
    //*********** Start creating the ColorSpace
       lpMem += WriteNewLineObject (lpMem, BeginArray);   // Begin array
    //********** /CIEBasedABC
       lpMem += WriteObject (lpMem, CIEBasedABCTag);   // Create entry
       lpMem += WriteObject (lpMem, BeginDict);    // Begin dictionary
   
    //********** Black/White Point
       lpMem += SendCSABWPoint(lpMem, InputIntent, IlluminantWP, MediaWP);
   
    //********** /DecodeABC
       lpMem += WriteNewLineObject (lpMem, DecodeABCTag);
       lpMem += WriteObject (lpMem, BeginArray);
   
       lpMem += WriteObject (lpMem, NewLine);
       lpMem += CreateColSpProc (cp, lpMem, icSigRedTRCTag, AllowBinary);
       lpMem += WriteObject (lpMem, NewLine);
       lpMem += CreateColSpProc (cp, lpMem, icSigGreenTRCTag, AllowBinary);
       lpMem += WriteObject (lpMem, NewLine);
       lpMem += CreateColSpProc (cp, lpMem, icSigBlueTRCTag, AllowBinary);
       lpMem += WriteObject (lpMem, EndArray);
   
    //********** /MatrixABC
       lpMem += WriteNewLineObject (lpMem, MatrixABCTag);
       lpMem += WriteObject (lpMem, BeginArray);
   
       lpMem += CreateFloatString (cp, lpMem, icSigRedColorantTag);
       lpMem += CreateFloatString (cp, lpMem, icSigGreenColorantTag);
       lpMem += CreateFloatString (cp, lpMem, icSigBlueColorantTag);
   
       lpMem += WriteObject (lpMem, EndArray);
   
    //********** /RangeLMN
       lpMem += WriteNewLineObject (lpMem, RangeLMNTag);
       lpMem += WriteObject (lpMem, RangeLMN);
   
    //********** /DecodeLMN
       if (InputIntent == icAbsoluteColorimetric)
       {
           // Support absolute whitePoint
           lpMem += WriteNewLineObject (lpMem, DecodeLMNTag);
           lpMem += WriteObject (lpMem, BeginArray);
           for (i = 0; i < 3; i ++)
           {
               lpMem += WriteObject (lpMem, BeginFunction);
               lpMem += WriteFloat (lpMem, (double)MediaWP[i]/IlluminantWP[i]);
               lpMem += WriteObject (lpMem, MulOp); 
               lpMem += WriteObject (lpMem, EndFunction);
           }
           lpMem += WriteObject (lpMem, EndArray);
       }
   
    //********** End dictionary definition
       lpMem += WriteNewLineObject (lpMem, EndDict);
       lpMem += WriteObject (lpMem, EndArray);
   
       if (BackupCSA)
       {
           lpMem += WriteNewLineObject (lpMem, SupportDEFG_E);
       }
   
    }
    lpMem += WriteNewLineObject (lpMem, CieBasedABCEnd);
    *lpcbSize = (DWORD) ((lpMem - lpOldMem));
    return (TRUE);
}

/***************************************************************************
*                           GetPS2CSA_ABC_LAB
*  function:
*    this is the function which creates the CIEBasedABC ColorSpace (CS)
*    from the data supplied in the LAB Input Profile.
*  prototype:
*       GetPS2CSA_ABC(
*                       CHANDLE     cp,
*                       MEMPTR      lpMem,
*                       LPDWORD     lpcbSize,
*                       BOOL        AllowBinary)
*  parameters:
*       cp          --  Color Profile handle
*       lpMem       --  Pointer to the memory block. If this point is NULL,
*                       require buffer size.
*       lpcbSize    --  Size of the memory block
*       InputIntent --
*       InpDrvClrSp --
*       AllowBinary --  1: binary CSA allowed,  0: only ascii CSA allowed.
*  returns:
*       BOOL        --  TRUE if the function was successful,
*                       FALSE otherwise.
***************************************************************************/

BOOL
GetPS2CSA_ABC_LAB (CHANDLE cp, MEMPTR lpMem, LPDWORD lpcbSize,
               CSIG InputIntent, DWORD InpDrvClrSp, BOOL AllowBinary)
{
    CSIG PCS, Dev;
    SFLOAT IlluminantWP[3];
    SFLOAT MediaWP[3];

    SINT i;
    MEMPTR lpOldMem = lpMem;
    SINT Ret = 0;
 // Check if we can generate the CS.
 // Required  tags are: red, green and blue Colorants.
 // As for TRC tags - we are quite flexible here - if we cannot find the
 // required tag - we assume the linear responce
    if (!GetCPConnSpace (cp, (LPCSIG) & PCS) ||
        !GetCPDevSpace (cp, (LPCSIG) & Dev) ||
        (Dev != icSigLabData))
    {
        return (FALSE);
    }
    if ((InpDrvClrSp != icSigLabData) &&
        (InpDrvClrSp != icSigDefData) &&
        (InpDrvClrSp != 0))
    {
        return (FALSE);
    }
 // Estimate the memory size required to hold CS

    if (lpMem == NULL)                  // This is a size request
    {
        *lpcbSize = 65530;
        return (TRUE);
    }
 // Get info about Illuminant White Point from the header
    GetCPWhitePoint (cp, (LPSFLOAT) & IlluminantWP);     // .. Illuminant
                                                         
 // Support absolute whitePoint
    GetMediaWP(cp, InputIntent, IlluminantWP, MediaWP);

 //*********** Start creating the ColorSpace
    lpMem += WriteNewLineObject (lpMem, BeginArray);   // Begin array

 //********** /CIEBasedABC
    lpMem += WriteObject (lpMem, CIEBasedABCTag);   // Create entry
    lpMem += WriteObject (lpMem, BeginDict);    // Begin dictionary

 //********** Black/White Point
    lpMem += SendCSABWPoint(lpMem, InputIntent, IlluminantWP, MediaWP);

 //********** /RangeABC
    lpMem += WriteNewLineObject (lpMem, RangeABCTag);
    lpMem += WriteObject (lpMem, RangeABC_Lab);

 //********** /DecodeABC
    lpMem += WriteNewLineObject (lpMem, DecodeABCTag);
    lpMem += WriteObject (lpMem, DecodeABCLab1);

 //********** /MatrixABC
    lpMem += WriteNewLineObject (lpMem, MatrixABCTag);
    lpMem += WriteObject (lpMem, MatrixABCLab);

 //********** /DecodeLMN
    lpMem += WriteNewLineObject (lpMem, DecodeLMNTag);
    lpMem += WriteObject (lpMem, BeginArray);
    for (i = 0; i < 3; i ++)
    {
        lpMem += WriteObject (lpMem, BeginFunction);
        lpMem += WriteObject (lpMem, DecodeLMNLab);
        if (InputIntent == icAbsoluteColorimetric)
        {
            lpMem += WriteFloat (lpMem, (double) MediaWP[i]);
        }
        else
        {
            lpMem += WriteFloat (lpMem, (double) IlluminantWP[i]);
        }
        lpMem += WriteObject (lpMem, MulOp); 
        lpMem += WriteObject (lpMem, EndFunction);
        lpMem += WriteObject (lpMem, NewLine);
    }
    lpMem += WriteObject (lpMem, EndArray);


 //********** End dictionary definition
    lpMem += WriteNewLineObject (lpMem, EndDict);
    lpMem += WriteObject (lpMem, EndArray);

    lpMem += WriteNewLineObject (lpMem, CieBasedABCEnd);
    *lpcbSize = (DWORD) ((lpMem - lpOldMem));
    return (TRUE);
}

/***************************************************************************
*                           GetPS2CSA_MONO_ABC
*  function:
*    this is the function which creates the CIEBasedABC ColorSpace (CS)
*    from the data supplied in the GRAY Input Profile.
*  prototype:
*       GetPS2CSA_MONO_ABC(
*                       CHANDLE     cp,
*                       MEMPTR      lpMem,
*                       LPDWORD     lpcbSize,
*                       BOOL        AllowBinary)
*  parameters:
*       cp          --  Color Profile handle
*       lpMem       --  Pointer to the memory block. If this point is NULL,
*                       require buffer size.
*       lpcbSize    --  Size of the memory block
*       AllowBinary --  1: binary CSA allowed,  0: only ascii CSA allowed.
*  returns:
*       BOOL        --  TRUE if the function was successful,
*                       FALSE otherwise.
***************************************************************************/

static BOOL
GetPS2CSA_MONO_ABC (CHANDLE cp, MEMPTR lpMem, LPDWORD lpcbSize, 
                    CSIG InputIntent, BOOL AllowBinary)
{
    SINT nCount;
    CSIG Tag, PCS;
    SINT i, j, Index;
    MEMPTR lpTable;
    SFLOAT IlluminantWP[3];
    SFLOAT MediaWP[3];
    MEMPTR lpBuff = NULL;
    SINT MemSize = 0;
    MEMPTR lpOldMem = lpMem;
    HGLOBAL hBuff;
    MEMPTR lpLineStart;
 // Check if we can generate the CS
    if (!DoesCPTagExist (cp, icSigGrayTRCTag) ||
        !GetCPTagIndex (cp, icSigGrayTRCTag, &Index) ||
        !GetCPElementType (cp, Index, (LPCSIG) & Tag) ||
        (Tag != icSigCurveType) ||
        !GetCPConnSpace (cp, (LPCSIG) & PCS) ||
        !GetCPElementSize (cp, Index, (LPSINT) & MemSize) ||
        !MemAlloc (MemSize, (HGLOBAL FAR *)&hBuff, (LPMEMPTR) & lpBuff) ||
        !GetCPElement (cp, Index, lpBuff, MemSize))
    {
        if (NULL != lpBuff)
        {
            MemFree (hBuff);
        }
        return (FALSE);
    }
    nCount = ui32toSINT (((lpcpCurveType) lpBuff)->curve.count);

 // Estimate the memory size required to hold CS
    *lpcbSize = nCount * 6 +            // Number of INT elements
        3 * (lstrlen (IndexArray) +
             lstrlen (StartClip) +
             lstrlen (EndClip)) +
        2048;                           // + other PS stuff
    if (lpMem == NULL)                  // This is a size request
    {
        MemFree (hBuff);
        return (TRUE);
    }
 // Get info about Illuminant White Point from the header
    GetCPWhitePoint (cp, (LPSFLOAT) & IlluminantWP);          // .. Illuminant

 // Support absolute whitePoint
    GetMediaWP(cp, InputIntent, IlluminantWP, MediaWP);

 //*********** Start creating the ColorSpace
    lpMem += WriteNewLineObject (lpMem, CieBasedABCBegin);

    lpMem += WriteNewLineObject (lpMem, BeginArray);   // Begin array
 //********** /CIEBasedABC
    lpMem += WriteObject (lpMem, CIEBasedABCTag);   // Create entry
    lpMem += WriteObject (lpMem, BeginDict);    // Begin dictionary

 //********** Black/White Point
    lpMem += SendCSABWPoint(lpMem, InputIntent, IlluminantWP, MediaWP);

 //********** /DecodeABC
    lpMem += WriteObject (lpMem, NewLine);
    lpLineStart = lpMem;

    if (nCount != 0)
    {
        lpMem += WriteObject (lpMem, DecodeABCTag);
        lpMem += WriteObject (lpMem, BeginArray);

        lpMem += WriteObject (lpMem, BeginFunction);
        if (nCount == 1)                // Gamma supplied in ui16 format
        {
            lpTable = (MEMPTR) (((lpcpCurveType) lpBuff)->curve.data);
            lpMem += WriteInt (lpMem, ui16toSINT (lpTable));
            lpMem += WriteObject (lpMem, DecodeA3);
        } else
        {
            lpMem += WriteObject (lpMem, StartClip);
            lpTable = (MEMPTR) (((lpcpCurveType) lpBuff)->curve.data);
            lpMem += WriteObject (lpMem, BeginArray);
            for (i = 0; i < nCount; i++)
            {
                lpMem += WriteInt (lpMem, ui16toSINT (lpTable));
                lpTable += sizeof (icUInt16Number);
                if (((SINT) (lpMem - lpLineStart)) > MAX_LINELENG)
                {
                    lpLineStart = lpMem;
                    lpMem += WriteObject (lpMem, NewLine);
                }
            }
            lpMem += WriteObject (lpMem, EndArray);
            lpLineStart = lpMem;
            lpMem += WriteObject (lpMem, NewLine);

            lpMem += WriteObject (lpMem, IndexArray);
            lpMem += WriteObject (lpMem, Scale16);
            lpMem += WriteObject (lpMem, EndClip);
        }
        lpMem += WriteObject (lpMem, EndFunction);
        lpMem += WriteObject (lpMem, DupOp);
        lpMem += WriteObject (lpMem, DupOp);
        lpMem += WriteObject (lpMem, EndArray);
    }
 //********** /MatrixABC
    lpMem += WriteNewLineObject (lpMem, MatrixABCTag);
    lpMem += WriteObject (lpMem, BeginArray);
    for (i = 0; i < 3; i ++)
    {
        for (j = 0; j < 3; j++)
        {
            if (i != j)
                lpMem += WriteFloat (lpMem, (double)0);
            else
            {
                if (InputIntent == icAbsoluteColorimetric)
                    lpMem += WriteFloat (lpMem, (double)MediaWP[i]);
                else
                    lpMem += WriteFloat (lpMem, (double)IlluminantWP[i]);
            }
        }
    }
    lpMem += WriteObject (lpMem, EndArray);

 //********** /RangeLMN
    lpMem += WriteNewLineObject (lpMem, RangeLMNTag);
    lpMem += WriteObject (lpMem, RangeLMN);


    lpMem += WriteObject (lpMem, EndDict);  // End dictionary definition
    lpMem += WriteObject (lpMem, EndArray);

    MemFree (hBuff);

    lpMem += WriteNewLineObject (lpMem, CieBasedABCEnd);
    *lpcbSize = (DWORD) (lpMem - lpOldMem);
    return (TRUE);
}
/***************************************************************************
*                           GetPS2CSA_MONO_A
*  function:
*    this is the function which creates the CIEBasedA ColorSpace (CS)
*    from the data supplied in the GRAY Input Profile.
*  prototype:
*       GetPS2CSA_MONO_A(
*                       CHANDLE     cp,
*                       MEMPTR      lpMem,
*                       LPDWORD     lpcbSize,
*                       BOOL        AllowBinary)
*  parameters:
*       cp          --  Color Profile handle
*       lpMem       --  Pointer to the memory block. If this point is NULL,
*                       require buffer size.
*       lpcbSize    --  Size of the memory block
*       AllowBinary --  1: binary CSA allowed,  0: only ascii CSA allowed.
*  returns:
*       BOOL        --  TRUE if the function was successful,
*                       FALSE otherwise.
***************************************************************************/

static BOOL
GetPS2CSA_MONO_A (CHANDLE cp, MEMPTR lpMem, LPDWORD lpcbSize, 
                  CSIG InputIntent, BOOL AllowBinary)
{
    SINT nCount;
    CSIG Tag, PCS;
    SINT i, Index;
    MEMPTR lpTable;
    SFLOAT IlluminantWP[3];
    SFLOAT MediaWP[3];
    MEMPTR lpBuff = NULL;
    SINT MemSize = 0;
    MEMPTR lpOldMem = lpMem;
    HGLOBAL hBuff;
    MEMPTR lpLineStart;
 // Check if we can generate the CS
    if (!DoesCPTagExist (cp, icSigGrayTRCTag) ||
        !GetCPTagIndex (cp, icSigGrayTRCTag, &Index) ||
        !GetCPElementType (cp, Index, (LPCSIG) & Tag) ||
        (Tag != icSigCurveType) ||
        !GetCPConnSpace (cp, (LPCSIG) & PCS) ||
        !GetCPElementSize (cp, Index, (LPSINT) & MemSize) ||
        !MemAlloc (MemSize, (HGLOBAL FAR *)&hBuff, (LPMEMPTR) & lpBuff) ||
        !GetCPElement (cp, Index, lpBuff, MemSize))
    {
        if (NULL != lpBuff)
        {
            MemFree (hBuff);
        }
        return (FALSE);
    }
    nCount = ui32toSINT (((lpcpCurveType) lpBuff)->curve.count);

 // Estimate the memory size required to hold CS
    *lpcbSize = nCount * 6 +            // Number of INT elements
        3 * (lstrlen (IndexArray) +
             lstrlen (StartClip) +
             lstrlen (EndClip)) +
        2048;                           // + other PS stuff
    if (lpMem == NULL)                  // This is a size request
    {
        MemFree (hBuff);
        return (TRUE);
    }
 // Get info about Illuminant White Point from the header
    GetCPWhitePoint (cp, (LPSFLOAT) & IlluminantWP);          // .. Illuminant

 // Support absolute whitePoint
    GetMediaWP(cp, InputIntent, IlluminantWP, MediaWP);

 //*********** Start creating the ColorSpace
    lpMem += WriteNewLineObject (lpMem, CieBasedABegin);

    lpMem += WriteNewLineObject (lpMem, BeginArray);   // Begin array
 //********** /CIEBasedA
    lpMem += WriteObject (lpMem, CIEBasedATag); // Create entry
    lpMem += WriteObject (lpMem, BeginDict);    // Begin dictionary

 //********** Black/White Point
    lpMem += SendCSABWPoint(lpMem, InputIntent, IlluminantWP, MediaWP);

 //********** /DecodeA
    lpMem += WriteObject (lpMem, NewLine);
    lpLineStart = lpMem;

    if (nCount != 0)
    {
        lpMem += WriteObject (lpMem, DecodeATag);

        lpMem += WriteObject (lpMem, BeginFunction);
        if (nCount == 1)                // Gamma supplied in ui16 format
        {
            lpTable = (MEMPTR) (((lpcpCurveType) lpBuff)->curve.data);
            lpMem += WriteInt (lpMem, ui16toSINT (lpTable));
            lpMem += WriteObject (lpMem, DecodeA3);
    // If the PCS is Lab, we need to convert Lab to XYZ
    // Now, the range is from 0 --> 0.99997.
    // Actually, the conversion from Lab to XYZ is not needed.
            if (PCS == icSigLabData)
            {
                lpMem += WriteObject (lpMem, DecodeALab);
                lpMem += WriteObject (lpMem, DecodeLMNLab);
            }
        } else
        {
            lpMem += WriteObject (lpMem, StartClip);
            lpTable = (MEMPTR) (((lpcpCurveType) lpBuff)->curve.data);
            lpMem += WriteObject (lpMem, BeginArray);
            for (i = 0; i < nCount; i++)
            {
                lpMem += WriteInt (lpMem, ui16toSINT (lpTable));
                lpTable += sizeof (icUInt16Number);
                if (((SINT) (lpMem - lpLineStart)) > MAX_LINELENG)
                {
                    lpLineStart = lpMem;
                    lpMem += WriteObject (lpMem, NewLine);
                }
            }
            lpMem += WriteObject (lpMem, EndArray);
            lpLineStart = lpMem;
            lpMem += WriteObject (lpMem, NewLine);

            lpMem += WriteObject (lpMem, IndexArray);
            lpMem += WriteObject (lpMem, Scale16);
    // If the PCS is Lab, we need to convert Lab to XYZ
    // Now, the range is from 0 --> .99997.
    // Actually, the conversion from Lab to XYZ is not needed.
            if (PCS == icSigLabData)
            {
                lpMem += WriteObject (lpMem, DecodeALab);
                lpMem += WriteObject (lpMem, DecodeLMNLab);
            }
            lpMem += WriteObject (lpMem, EndClip);
        }
        lpMem += WriteObject (lpMem, EndFunction);
    }

 //********** /MatrixA
    lpMem += WriteNewLineObject (lpMem, MatrixATag);
    lpMem += WriteObject (lpMem, BeginArray);
    for (i = 0; i < 3; i++)
    {
        if (InputIntent == icAbsoluteColorimetric)
            lpMem += WriteFloat (lpMem, (double) MediaWP[i]);
        else
            lpMem += WriteFloat (lpMem, (double) IlluminantWP[i]);
    }
    lpMem += WriteObject (lpMem, EndArray);

 //********** /RangeLMN
    lpMem += WriteNewLineObject (lpMem, RangeLMNTag);
    lpMem += WriteObject (lpMem, RangeLMN);

 //********** /End dictionary
    lpMem += WriteObject (lpMem, EndDict);  // End dictionary definition
    lpMem += WriteObject (lpMem, EndArray);

    MemFree (hBuff);

    lpMem += WriteNewLineObject (lpMem, CieBasedAEnd);
    *lpcbSize = (DWORD) (lpMem - lpOldMem);
    return (TRUE);
}
/***************************************************************************
*                           GetPS2CSA_MONO
*  function:
*    this is the function which creates the MONO ColorSpace (CS)
*    from the data supplied in the GRAY Input Profile.
*  prototype:
*       GetPS2CSA_MONO(
*                       CHANDLE     cp,
*                       MEMPTR      lpMem,
*                       LPDWORD     lpcbSize,
*                       WORD        InpDrvClrSp
*                       BOOL        AllowBinary)
*  parameters:
*       cp          --  Color Profile handle
*       lpMem       --  Pointer to the memory block. If this point is NULL,
*                       require buffer size.
*       lpcbSize    --  Size of the memory block
*       InpDrvClrSp --  Device color type (RGB or GRAY).
*       AllowBinary --  1: binary CSA allowed,  0: only ascii CSA allowed.
*  returns:
*       BOOL        --  TRUE if the function was successful,
*                       FALSE otherwise.
***************************************************************************/
static BOOL
GetPS2CSA_MONO (CHANDLE cp, MEMPTR lpMem, LPDWORD lpcbSize,
                DWORD InpDrvClrSp, CSIG InputIntent, BOOL AllowBinary)
{
    BOOL Success = FALSE;
#if 0
    if ((InpDrvClrSp == icSigRgbData) ||
        (InpDrvClrSp == icSigDefData) ||
        (InpDrvClrSp == 0))
    {
        Success = GetPS2CSA_MONO_ABC (cp, lpMem, lpcbSize, InputIntent, AllowBinary);
    } else if (InpDrvClrSp == icSigGrayData)
    {
        Success = GetPS2CSA_MONO_A (cp, lpMem, lpcbSize, InputIntent, AllowBinary);
    }
#else
    if ((InpDrvClrSp == icSigGrayData) ||
        (InpDrvClrSp == 0))
    {
        Success = GetPS2CSA_MONO_A (cp, lpMem, lpcbSize, InputIntent, AllowBinary);
    }
    else
    {
        Success = FALSE;
    }
#endif
    return Success;
}
/***************************************************************************
*
*   Function to create a procedure for Color space.
*
***************************************************************************/

SINT 
CreateColSpProc (CHANDLE cp, MEMPTR lpMem, CSIG CPTag, BOOL AllowBinary)
{
    SINT nCount, Index;
    MEMPTR lpTable;
    MEMPTR Buff = NULL;
    SINT MemSize = 0;
    MEMPTR lpOldMem;
    HGLOBAL hBuff;
    lpOldMem = lpMem;
    lpMem += WriteObject (lpMem, BeginFunction);
    if (DoesCPTagExist (cp, CPTag) &&
        GetCPTagIndex (cp, CPTag, (LPSINT) & Index) &&
        GetCPElementSize (cp, Index, (LPSINT) & MemSize) &&
        MemAlloc (MemSize, (HGLOBAL FAR *)&hBuff, (LPMEMPTR) & Buff) &&
        GetCPElement (cp, Index, Buff, MemSize))
    {
        nCount = ui32toSINT (((lpcpCurveType) Buff)->curve.count);
        if (nCount != 0)
        {
            if (nCount == 1)            // Gamma supplied in ui16 format
            {
                lpTable = (MEMPTR) (((lpcpCurveType) Buff)->curve.data);
                lpMem += WriteInt (lpMem, ui16toSINT (lpTable));
                lpMem += WriteObject (lpMem, DecodeA3);
            } else
            {
                lpMem += WriteObject (lpMem, StartClip);
                lpTable = (MEMPTR) (((lpcpCurveType) Buff)->curve.data);
                lpMem += WriteObject (lpMem, DecodeABCArray);
                lpMem += WriteInt (lpMem, (SINT) CPTag);

                if (!AllowBinary)       // Output ASCII CS
                {
                    lpMem += WriteObject (lpMem, IndexArray);
                } else
                {                       // Output BINARY CS
                    lpMem += WriteObject (lpMem, IndexArray16b);
                }
                lpMem += WriteObject (lpMem, Scale16);
                lpMem += WriteObject (lpMem, EndClip);
            }
        }
        MemFree (hBuff);
    }
    lpMem += WriteObject (lpMem, EndFunction);
    return ((SINT) (lpMem - lpOldMem));
}
/***************************************************************************
*
*   Function to create a procedure for Color space.
*
***************************************************************************/

SINT 
CreateFloatString (CHANDLE cp, MEMPTR lpMem, CSIG CPTag)
{
    SINT i, Index;
    MEMPTR lpTable;
    MEMPTR Buff = NULL;
    SINT MemSize = 0;
    MEMPTR lpOldMem;
    HGLOBAL hBuff;
    lpOldMem = lpMem;
    if (GetCPTagIndex (cp, CPTag, (LPSINT) & Index) &&
        GetCPElementSize (cp, Index, (LPSINT) & MemSize) &&
        MemAlloc (MemSize, (HGLOBAL FAR *)&hBuff, (LPMEMPTR) & Buff) &&
        GetCPElement (cp, Index, Buff, MemSize))
    {
        lpTable = (MEMPTR) & (((lpcpXYZType) Buff)->data);
        for (i = 0; i < 3; i++)
        {
            lpMem += WriteFloat (lpMem, si16f16toSFLOAT (lpTable));
            lpTable += sizeof (icS15Fixed16Number);
        }
        MemFree (hBuff);
    }
    return ((SINT) (lpMem - lpOldMem));
}
/***************************************************************************
*
*   Function to create a array later to be used in ColorSpace's DecodeABC.
*
***************************************************************************/

SINT 
CreateColSpArray (CHANDLE cp, MEMPTR lpMem, CSIG CPTag, BOOL AllowBinary)
{
    SINT i, nCount, Index;
    MEMPTR lpTable;
    MEMPTR Buff = NULL;
    SINT MemSize = 0;
    MEMPTR lpOldMem, lpLineStart;
    HGLOBAL hBuff;
    lpOldMem = lpMem;

    lpLineStart = lpMem;

    if (DoesCPTagExist (cp, CPTag) &&
        GetCPTagIndex (cp, CPTag, (LPSINT) & Index) &&
        GetCPElementSize (cp, Index, (LPSINT) & MemSize) &&
        MemAlloc (MemSize, (HGLOBAL FAR *)&hBuff, (LPMEMPTR) & Buff) &&
        GetCPElement (cp, Index, Buff, MemSize))
    {
        nCount = ui32toSINT (((lpcpCurveType) Buff)->curve.count);
        if (nCount > 1)
        {
            lpMem += WriteNewLineObject (lpMem, Slash);
            lpMem += WriteObject (lpMem, DecodeABCArray);
            lpMem += WriteInt (lpMem, (SINT) CPTag);

            lpTable = (MEMPTR) (((lpcpCurveType) Buff)->curve.data);
            if (!AllowBinary)           // Output ASCII CS
            {
                lpMem += WriteObject (lpMem, BeginArray);
                for (i = 0; i < nCount; i++)
                {
                    lpMem += WriteInt (lpMem, ui16toSINT (lpTable));
                    lpTable += sizeof (icUInt16Number);
                    if (((SINT) (lpMem - lpLineStart)) > MAX_LINELENG)
                    {
                        lpLineStart = lpMem;
                        lpMem += WriteObject (lpMem, NewLine);
                    }
                }
                lpMem += WriteObject (lpMem, EndArray);
            } else
            {                           // Output BINARY CS
                lpMem += WriteHNAToken (lpMem, 149, nCount);
                lpMem += WriteIntStringU2S (lpMem, lpTable, nCount);
            }
            lpMem += WriteObject (lpMem, DefOp);
        }
        MemFree (hBuff);
    }
    return ((SINT) (lpMem - lpOldMem));
}
/***************************************************************************
*                               GetCSAFromProfile
*  function:
*    this is the function which gets the ColorSpace dictionary array
*    from the the Profile.
*  prototype:
*       static BOOL GetCSAFromProfile(
*                       CHANDLE     cp,
*                       MEMPTR      lpMem,
*                       LPDWORD     lpcbSize,
*                       WORD        InpDrvClrSp,
*                       CSIG        DrvColorSpace,
*                       BOOL        AllowBinary)
*  parameters:
*       cp          --  Color Profile handle
*       lpBuffer    --  Pointer to the memory block. If this point is NULL,
*                       require buffer size.
*       lpcbSize    --  Size of the memory block
*       InpDrvClrSp --  Input device color space.
*       DrvColorSpace --  Profile device color space.
*       AllowBinary --  1: binary CS allowed,  0: only ascii CS allowed.
*  returns:
*       BOOL        --  TRUE if the function was successful,
*                       FALSE otherwise.
***************************************************************************/

static BOOL
GetCSAFromProfile (
                   CHANDLE cp,
                   MEMPTR lpMem,
                   LPDWORD lpcbSize,
                   DWORD InpDrvClrSp,
                   CSIG DrvColorSpace,
                   BOOL AllowBinary)
{
    SINT Index;
    SINT Size;
    if ((DrvColorSpace == icSigGrayData) && (InpDrvClrSp != icSigGrayData))
        return FALSE;

    if (DoesCPTagExist (cp, icSigPs2CSATag) &&
        GetCPTagIndex (cp, icSigPs2CSATag, (LPSINT) & Index) &&
        GetCPElementDataSize (cp, Index, (LPSINT) & Size) &&
        ((lpMem == NULL) || GetCPElementData (cp, Index, lpMem, Size)) &&
        (*lpcbSize = Convert2Ascii (cp, Index, lpMem, *lpcbSize, Size, AllowBinary)))
    {
        return TRUE;
    } else
    {
        return FALSE;
    }
}
/***************************************************************************
*                           GetPS2CSA_DEFG_Intent
*  function:
*    This is the function which creates the CieBasedDEF(G)ColorSpace array
*    based on Intent.
*  prototype:
*       static BOOL GetPS2CSA_DEFG_Intent(
*                   CHANDLE     cp,
*                   MEMPTR      lpBuffer,
*                   LPDWORD     lpcbSize,
*                   CSIG        Intent,
*                   int         Type,
*                   BOOL        AllowBinary)
*  parameters:
*       cp          --  Color Profile handle
*       lpBuffer    --  Pointer to the memory block. If this point is NULL,
*                       require buffer size.
*       lpcbSize    --  Size of the memory block
*       Intent      --  Intent.
*       Type        --  CieBasedDEF or CieBasedDEF.
*       AllowBinary --  1: binary CS allowed,  0: only ascii CS allowed.
*  returns:
*       BOOL        --  TRUE if the function was successful,
*                       FALSE otherwise.
***************************************************************************/

BOOL
GetPS2CSA_DEFG_Intent (
                       CHANDLE cp,
                       MEMPTR lpBuffer,
                       LPDWORD lpcbSize,
                       DWORD InpDrvClrSp,
                       CSIG Intent,
                       int Type,
                       BOOL AllowBinary)
{
    SINT Index;
    BOOL Success = FALSE;
    CSIG icSigAToBx;

 // Try to create CieBasedDEFG CSA first.
    if (((Type == TYPE_CIEBASEDDEFG) && (InpDrvClrSp != icSigCmykData) ||
         (Type == TYPE_CIEBASEDDEF) && (InpDrvClrSp != icSigDefData)) &&
        (InpDrvClrSp != 0))
    {
        return (FALSE);
    }
    switch (Intent)
    {
        case icPerceptual:
            icSigAToBx = icSigAToB0Tag;
            break;
        case icRelativeColorimetric:
            icSigAToBx = icSigAToB1Tag;
            break;
        case icSaturation:
            icSigAToBx = icSigAToB2Tag;
            break;
        case icAbsoluteColorimetric:
            icSigAToBx = icSigAToB1Tag;
            break;
        default:
            return Success;
    }

    if (DoesCPTagExist (cp, icSigAToBx) &&
        GetCPTagIndex (cp, icSigAToBx, (LPSINT) & Index))
    {
        Success = GetPS2CSA_DEFG (cp, lpBuffer, lpcbSize, Intent, Index, Type, AllowBinary);
    }

    return Success;
}
/***************************************************************************
*                               GetPS2ColorSpaceArray
*  function:
*    This is the main function which creates the ColorSpace array
*    from the data supplied in the Profile.
*  prototype:
*       BOOL GetPS2ColorSpaceArray(
*               CHANDLE     cp,
*               CSIG        InputIntent,
*               WORD        InpDrvClrSp,
*               MEMPTR      lpBuffer,
*               LPDWORD     lpcbSize,
*               BOOL        AllowBinary)
*  parameters:
*       cp          --  Color Profile handle
*       lpBuffer    --  Pointer to the memory block. If this point is NULL,
*                       require buffer size.
*       lpcbSize    --  Size of the memory block
*       InpDrvClrSp --  Input device color space.
*                       icSigCmykData: input data is cmyk, create CiebasedDEFG CSA.
*                       icSigRgbData : input data is rgb, create CieBasedABC CSA.
*                       icSigDefData : input data is rgb or lab, create CiebasedDEF CSA.
*                       isSigGrayData: input data is gray, create CieBasedA CSA.
*                       0            : Auto. Create CSA depends on profile color space.
*       InputIntent --  Intent.
*       AllowBinary --  1: binary CS allowed,  0: only ascii CS allowed.
*  returns:
*       BOOL        --  TRUE if the function was successful,
*                       FALSE otherwise.
***************************************************************************/
BOOL EXTERN
GetPS2ColorSpaceArray (
                       CHANDLE cp,
                       DWORD InputIntent,
                       DWORD InpDrvClrSp,
                       MEMPTR lpBuffer,
                       LPDWORD lpcbSize,
                       BOOL AllowBinary)
{
    CSIG ColorSpace, Intent;
    BOOL Success = FALSE;
    DWORD dwSaveSize;
    if (!cp)
        return Success;

    if (!GetCPDevSpace (cp, (LPCSIG) & ColorSpace) ||
        !GetCPRenderIntent (cp, (LPCSIG) & Intent))
    {
        return Success;
    }
    dwSaveSize = *lpcbSize;
    if (InputIntent == icUseRenderingIntent)
        InputIntent = (DWORD)Intent;

 // Get ColorSpace from Profile.
    if ((CSIG) InputIntent == Intent)
    {
        Success = GetCSAFromProfile (cp, lpBuffer, lpcbSize,
                                     InpDrvClrSp, ColorSpace, AllowBinary);
    }
    if (!Success)
    {
        switch (ColorSpace)
        {
            case icSigRgbData:
                Success = GetPS2CSA_DEFG_Intent (cp, lpBuffer, lpcbSize,
                                                 InpDrvClrSp, (CSIG) InputIntent, 
                                                 TYPE_CIEBASEDDEF, AllowBinary);
                if (Success)
                {                       // Create CieBasedABC or DeviceRGB
                                        // for the printer
                                        // which does not support CieBasedDEF(G).
                    DWORD cbNewSize = 0;
                    MEMPTR lpNewBuffer;
                    MEMPTR lpOldBuffer;
                    if (lpBuffer)
                    {
                        lpNewBuffer = lpBuffer + *lpcbSize;
                        lpOldBuffer = lpNewBuffer;
                        lpNewBuffer += WriteObject (lpNewBuffer, NewLine);
                        cbNewSize = dwSaveSize - (DWORD) (lpNewBuffer - lpBuffer);
                    } else
                        lpNewBuffer = NULL;

                    if (!GetPS2CSA_ABC (cp, lpNewBuffer, &cbNewSize,
                                        (CSIG)InputIntent, InpDrvClrSp, 
                                        AllowBinary, 1))   // create a backup CSA
                        GetDeviceRGB (lpNewBuffer, &cbNewSize, InpDrvClrSp, 1);

                    if (lpBuffer)
                    {
                        lpNewBuffer += cbNewSize;
                        *lpcbSize += (DWORD) (lpNewBuffer - lpOldBuffer);
                    } else
                        *lpcbSize += cbNewSize;

                }
                if (!Success)
                {                           // Create CieBasedABC
                    Success = GetPS2CSA_ABC (cp, lpBuffer, lpcbSize,
                                         (CSIG)InputIntent, InpDrvClrSp, 
                                         AllowBinary, 0);
                }
                if (!Success)
                {                           // Create DeviceRGB
                    Success = GetDeviceRGB (lpBuffer, lpcbSize, InpDrvClrSp, 0);
                    Success = FALSE;
                }
                break;
            case icSigCmykData:
                Success = GetPS2CSA_DEFG_Intent (cp, lpBuffer, lpcbSize,
                                                 InpDrvClrSp, (CSIG) InputIntent, 
                                                 TYPE_CIEBASEDDEFG, AllowBinary);
                if (Success)
                {                       // Create DeviceCMYK for the printer
                                        // which does not support CieBasedDEF(G).
                    DWORD cbNewSize = 0;
                    MEMPTR lpNewBuffer;
                    MEMPTR lpOldBuffer;
                    if (lpBuffer)
                    {
                        lpNewBuffer = lpBuffer + *lpcbSize;
                        lpOldBuffer = lpNewBuffer;
                        lpNewBuffer += WriteObject (lpNewBuffer, NewLine);
                        lpNewBuffer += WriteNewLineObject (lpNewBuffer, NotSupportDEFG_S);
                        cbNewSize = dwSaveSize - (DWORD) (lpNewBuffer - lpBuffer);
                    } else
                        lpNewBuffer = NULL;

                    GetDeviceCMYK (lpNewBuffer, &cbNewSize, InpDrvClrSp);

                    if (lpBuffer)
                    {
                        lpNewBuffer += cbNewSize;
                        lpNewBuffer += WriteNewLineObject (lpNewBuffer, SupportDEFG_E);
                        *lpcbSize += (DWORD) (lpNewBuffer - lpOldBuffer);
                    } else
                        *lpcbSize += cbNewSize;
                }
                if (!Success)
                {                           // Create DeviceCMYK
                    Success = GetDeviceCMYK (lpBuffer, lpcbSize, InpDrvClrSp);
                    Success = FALSE;
                }
                break;
            case icSigGrayData:
                Success = GetPS2CSA_MONO (cp, lpBuffer, lpcbSize, InpDrvClrSp, 
                                          (CSIG)InputIntent, AllowBinary);
                if (!Success)
                {                           // Create DeviceGray
                    Success = GetDeviceGray (lpBuffer, lpcbSize, InpDrvClrSp);
                    Success = FALSE;
                }
                break;
            case icSigLabData:
                Success = GetPS2CSA_DEFG_Intent (cp, lpBuffer, lpcbSize,
                                                 InpDrvClrSp, (CSIG) InputIntent, 
                                                 TYPE_CIEBASEDDEF, AllowBinary);
                if (Success)
                {                       // Create CieBasedABC or DeviceRGB
                                        // for the printer
                                        // which does not support CieBasedDEF(G).
                    DWORD cbNewSize = 0;
                    MEMPTR lpNewBuffer;
                    MEMPTR lpOldBuffer;
                    if (lpBuffer)
                    {
                        lpNewBuffer = lpBuffer + *lpcbSize;
                        lpOldBuffer = lpNewBuffer;
                        lpNewBuffer += WriteObject (lpNewBuffer, NewLine);
                        lpNewBuffer += WriteNewLineObject (lpNewBuffer, NotSupportDEFG_S);
                        cbNewSize = dwSaveSize - (DWORD) (lpNewBuffer - lpBuffer);
                    } else
                        lpNewBuffer = NULL;

                    GetPS2CSA_ABC_LAB (cp, lpNewBuffer, &cbNewSize,
                                        (CSIG)InputIntent, InpDrvClrSp, AllowBinary);

                    if (lpBuffer)
                    {
                        lpNewBuffer += cbNewSize;
                        lpNewBuffer += WriteNewLineObject (lpNewBuffer, SupportDEFG_E);
                        *lpcbSize += (DWORD) (lpNewBuffer - lpOldBuffer);
                    } else
                        *lpcbSize += cbNewSize;
                }
                if (!Success)
                {                           // Create CieBasedABC
                    Success = GetPS2CSA_ABC_LAB (cp, lpBuffer, lpcbSize,
                                         (CSIG)InputIntent, InpDrvClrSp, AllowBinary);
                }
                break;

            default:
                break;
        }
    }
    return Success;
}
