/****************************Module*Header******************************\
* Module Name: PS2.C
*
* Module Descripton: Functions for retrieving or creating PostScript
*    Level 2 operators from a profile. It is shared by mscms & pscript5
*
* Warnings:
*
* Issues:
*
* Public Routines:
*
* Created:  13 May 1996
* Author:   Srinivasan Chandrasekar    [srinivac]
*
* Copyright (c) 1996, 1997  Microsoft Corporation
\***********************************************************************/

#include <math.h>

#define MAX_LINELEN             240
#define REVCURVE_RATIO          1
#define CIEXYZRange             0x1FFEC       // 1.9997 in 16.16 notation
#define ALIGN_DWORD(nBytes)     (((nBytes) + 3) & ~3)

#define FIX_16_16_SHIFT         16
#define FIX_16_16_SCALE         (1 << (FIX_16_16_SHIFT))

#define TO_FIX(x)               ((x) << FIX_16_16_SHIFT)
#define TO_INT(x)               ((x) >> FIX_16_16_SHIFT)
#define FIX_MUL(x, y)           MulDiv((x), (y), FIX_16_16_SCALE)
#define FIX_DIV(x, y)           MulDiv((x), FIX_16_16_SCALE, (y))

#define FLOOR(x)                ((x) >> FIX_16_16_SHIFT << FIX_16_16_SHIFT)

#define TYPE_CIEBASEDDEF        1
#define TYPE_CIEBASEDDEFG       2

#define TAG_PS2CSA              'ps2s'
#define TAG_REDCOLORANT         'rXYZ'
#define TAG_GREENCOLORANT       'gXYZ'
#define TAG_BLUECOLORANT        'bXYZ'
#define TAG_REDTRC              'rTRC'
#define TAG_GREENTRC            'gTRC'
#define TAG_BLUETRC             'bTRC'
#define TAG_GRAYTRC             'kTRC'
#define TAG_MEDIAWHITEPOINT     'wtpt'
#define TAG_AToB0               'A2B0'
#define TAG_AToB1               'A2B1'
#define TAG_AToB2               'A2B2'
#define TAG_PS2INTENT0          'psi0'
#define TAG_PS2INTENT1          'psi1'
#define TAG_PS2INTENT2          'psi2'
#define TAG_PS2INTENT3          'psi3'
#define TAG_CRDINTENT0          'psd0'
#define TAG_CRDINTENT1          'psd1'
#define TAG_CRDINTENT2          'psd2'
#define TAG_CRDINTENT3          'psd3'
#define TAG_BToA0               'B2A0'
#define TAG_BToA1               'B2A1'
#define TAG_BToA2               'B2A2'
#define TAG_BToA3               'B2A3'

#define LUT8_TYPE               'mft1'
#define LUT16_TYPE              'mft2'

#define SIG_CURVE_TYPE          'curv'

#define GetCPConnSpace(pProfile)     (FIX_ENDIAN(((PPROFILEHEADER)pProfile)->phConnectionSpace))
#define GetCPDevSpace(pProfile)      (FIX_ENDIAN(((PPROFILEHEADER)pProfile)->phDataColorSpace))
#define GetCPRenderIntent(pProfile)  (FIX_ENDIAN(((PPROFILEHEADER)pProfile)->phRenderingIntent))
#define WriteObject(pBuf, pStr)      (STRCPY(pBuf, pStr), STRLEN(pStr))

#ifdef KERNEL_MODE
#define WriteInt(pBuf, i)            OPSprintf(pBuf, "%l ", (i))
#define WriteHex(pBuf, x)            OPSprintf(pBuf, "%x", ((x) & 0x00FF))
#define STRLEN                       strlen
#define STRCPY                       strcpy
#else
#define WriteInt(pBuf, i)            wsprintfA(pBuf, "%lu ", (i))
#define WriteHex(pBuf, x)            wsprintfA(pBuf, "%2.2x", ((x) & 0x00FF))
#define STRLEN                       lstrlenA
#define STRCPY                       lstrcpyA
#endif

#define MAXCHANNELS             4
#define PREVIEWCRDGRID          16
#define MAXCOLOR8               255

#define DATATYPE_LUT            0
#define DATATYPE_MATRIX         1

#define sRGB_CRC                0xa3d777b4L
#define sRGB_TAGSIZE            6168

//
// Local typedefs
//

typedef DWORD FIX_16_16, *PFIX_16_16;

typedef struct tagCURVETYPE {
    DWORD dwSignature;
    DWORD dwReserved;
    DWORD nCount;
    WORD  data[0];
} CURVETYPE, *PCURVETYPE;

typedef struct tagXYZTYPE {
    DWORD     dwSignature;
    DWORD     dwReserved;
    FIX_16_16 afxData[0];
} XYZTYPE, *PXYZTYPE;

typedef struct tagLUT8TYPE {
    DWORD     dwSignature;
    DWORD     dwReserved;
    BYTE      nInputChannels;
    BYTE      nOutputChannels;
    BYTE      nClutPoints;
    BYTE      padding;
    FIX_16_16 e00;
    FIX_16_16 e01;
    FIX_16_16 e02;
    FIX_16_16 e10;
    FIX_16_16 e11;
    FIX_16_16 e12;
    FIX_16_16 e20;
    FIX_16_16 e21;
    FIX_16_16 e22;
    BYTE      data[0];
} LUT8TYPE, *PLUT8TYPE;

typedef struct tagLUT16TYPE {
    DWORD     dwSignature;
    DWORD     dwReserved;
    BYTE      nInputChannels;
    BYTE      nOutputChannels;
    BYTE      nClutPoints;
    BYTE      padding;
    FIX_16_16 e00;
    FIX_16_16 e01;
    FIX_16_16 e02;
    FIX_16_16 e10;
    FIX_16_16 e11;
    FIX_16_16 e12;
    FIX_16_16 e20;
    FIX_16_16 e21;
    FIX_16_16 e22;
    WORD      wInputEntries;
    WORD      wOutputEntries;
    WORD      data[0];
} LUT16TYPE, *PLUT16TYPE;

typedef struct tagHOSTCLUT {
    WORD        wSize;
    WORD        wDataType;
    DWORD       dwDev;
    DWORD       dwPCS;
    DWORD       dwIntent;
    FIX_16_16   afxIlluminantWP[3];
    FIX_16_16   afxMediaWP[3];
    BYTE        nInputCh;
    BYTE        nOutputCh;
    BYTE        nClutPoints;
    BYTE        nLutBits;
    FIX_16_16   e[9];
    WORD        nInputEntries;
    WORD        nOutputEntries;
    PBYTE       inputArray[MAXCHANNELS];
    PBYTE       outputArray[MAXCHANNELS];
    PBYTE       clut;
} HOSTCLUT, *PHOSTCLUT;

//
// Internal functions
//

BOOL  IsSRGBColorProfile(PBYTE);

BOOL  GetCSAFromProfile (PBYTE, DWORD, DWORD, PBYTE, PDWORD, PBOOL);
BOOL  GetPS2CSA_MONO_A(PBYTE, PBYTE, PDWORD, DWORD, PBOOL);
BOOL  GetPS2CSA_ABC(PBYTE, PBYTE, PDWORD, DWORD, PBOOL, BOOL);
BOOL  GetPS2CSA_ABC_Lab(PBYTE, PBYTE, PDWORD, DWORD, PBOOL);
BOOL  GetPS2CSA_DEFG(PBYTE, PBYTE, PDWORD, DWORD, DWORD, PBOOL);

BOOL  CreateMonoCRD(PBYTE, DWORD, PBYTE, PDWORD, DWORD);
BOOL  CreateLutCRD(PBYTE, DWORD, PBYTE, PDWORD, DWORD, BOOL);

BOOL  DoesCPTagExist(PBYTE, DWORD, PDWORD);
BOOL  DoesTRCAndColorantTagExist(PBYTE);
BOOL  GetCPWhitePoint(PBYTE, PFIX_16_16);
BOOL  GetCPMediaWhitePoint(PBYTE, PFIX_16_16);
BOOL  GetCPElementDataSize(PBYTE, DWORD, PDWORD);
BOOL  GetCPElementSize(PBYTE, DWORD, PDWORD);
BOOL  GetCPElementDataType(PBYTE, DWORD, PDWORD);
BOOL  GetCPElementData(PBYTE, DWORD, PBYTE, PDWORD);
BOOL  GetTRCElementSize(PBYTE, DWORD, PDWORD, PDWORD);

DWORD Ascii85Encode(PBYTE, DWORD, DWORD);

BOOL  GetCRDInputOutputArraySize(PBYTE, DWORD, PDWORD, PDWORD, PDWORD, PDWORD);
BOOL  GetHostCSA(PBYTE, PBYTE, PDWORD, DWORD, DWORD);
BOOL  GetHostColorRenderingDictionary(PBYTE, DWORD, PBYTE, PDWORD);
BOOL  GetHostColorSpaceArray(PBYTE, DWORD, PBYTE, PDWORD);

DWORD SendCRDBWPoint(PBYTE, PFIX_16_16);
DWORD SendCRDPQR(PBYTE, DWORD, PFIX_16_16);
DWORD SendCRDLMN(PBYTE, DWORD, PFIX_16_16, PFIX_16_16, DWORD);
DWORD SendCRDABC(PBYTE, PBYTE, DWORD, DWORD, PBYTE, PFIX_16_16, DWORD, BOOL);
DWORD SendCRDOutputTable(PBYTE, PBYTE, DWORD, DWORD, BOOL, BOOL);

DWORD SendCSABWPoint(PBYTE, DWORD, PFIX_16_16, PFIX_16_16);
VOID  GetMediaWP(PBYTE, DWORD, PFIX_16_16, PFIX_16_16);

DWORD CreateCRDRevArray(PBYTE, PBYTE, PCURVETYPE, PWORD, DWORD, BOOL);
DWORD SendCRDRevArray(PBYTE, PBYTE, PCURVETYPE, DWORD, BOOL);

DWORD CreateColSpArray(PBYTE, PBYTE, DWORD, BOOL);
DWORD CreateColSpProc(PBYTE, PBYTE, DWORD, BOOL);
DWORD CreateFloatString(PBYTE, PBYTE, DWORD);
DWORD CreateInputArray(PBYTE, DWORD, DWORD, PBYTE, DWORD, PBYTE, BOOL, PBYTE);
DWORD CreateOutputArray(PBYTE, DWORD, DWORD, DWORD, PBYTE, DWORD, PBYTE, BOOL, PBYTE);

DWORD GetPublicArrayName(DWORD, PBYTE);
BOOL  GetRevCurve(PCURVETYPE, PWORD, PWORD);
VOID  GetCLUTInfo(DWORD, PBYTE, PDWORD, PDWORD, PDWORD, PDWORD, PDWORD, PDWORD);
DWORD EnableGlobalDict(PBYTE);
DWORD BeginGlobalDict(PBYTE);
DWORD EndGlobalDict(PBYTE);

DWORD WriteNewLineObject(PBYTE, const char *);
DWORD WriteHNAToken(PBYTE, BYTE, DWORD);
DWORD WriteIntStringU2S(PBYTE, PBYTE, DWORD);
DWORD WriteIntStringU2S_L(PBYTE, PBYTE, DWORD);
DWORD WriteHexBuffer(PBYTE, PBYTE, PBYTE, DWORD);
DWORD WriteStringToken(PBYTE, BYTE, DWORD);
DWORD WriteByteString(PBYTE, PBYTE, DWORD);
DWORD WriteInt2ByteString(PBYTE, PBYTE, DWORD);
DWORD WriteFixed(PBYTE, FIX_16_16);
DWORD WriteFixed2dot30(PBYTE, DWORD);
#if !defined(KERNEL_MODE) || defined(USERMODE_DRIVER)
DWORD WriteDouble(PBYTE, double);

BOOL  CreateMatrixCRD(PBYTE, PBYTE, PDWORD, DWORD, BOOL);
DWORD CreateHostLutCRD(PBYTE, DWORD, PBYTE, DWORD);
DWORD CreateHostMatrixCSAorCRD(PBYTE, PBYTE, PDWORD, DWORD, BOOL);
DWORD CreateHostInputOutputArray(PBYTE, PBYTE*, DWORD, DWORD, DWORD, DWORD, PBYTE);
DWORD CreateHostTRCInputTable(PBYTE, PHOSTCLUT, PCURVETYPE, PCURVETYPE, PCURVETYPE);
DWORD CreateHostRevTRCInputTable(PBYTE, PHOSTCLUT, PCURVETYPE, PCURVETYPE, PCURVETYPE);

BOOL  CheckInputOutputTable(PHOSTCLUT, float*, BOOL, BOOL);
BOOL  CheckColorLookupTable(PHOSTCLUT, float*);

BOOL  DoHostConversionCSA(PHOSTCLUT, float*, float*);
BOOL  DoHostConversionCRD(PHOSTCLUT, PHOSTCLUT, float*, float*, BOOL);

float g(float);
float inverse_g(float);
BOOL  TableInterp3(PHOSTCLUT, float*);
BOOL  TableInterp4(PHOSTCLUT, float*);
void  LabToXYZ(float*, float*, PFIX_16_16);
void  XYZToLab(float*, float*, PFIX_16_16);
VOID  ApplyMatrix(FIX_16_16 *e, float *Input, float *Output);

BOOL  CreateColorantArray(PBYTE, double *, DWORD);
BOOL  InvertColorantArray(double *, double *);
#endif

DWORD crc32(PBYTE buff, DWORD length);

//
// Global variables
//

const char  ASCII85DecodeBegin[] = "<~";
const char  ASCII85DecodeEnd[]   = "~> cvx exec ";
const char  TestingDEFG[]       = "/SupportDEFG? {/CIEBasedDEFG \
 /ColorSpaceFamily resourcestatus { pop pop languagelevel 3 ge}{false} ifelse} def";
const char  SupportDEFG_S[]     = "SupportDEFG? { ";
const char  NotSupportDEFG_S[]  = "SupportDEFG? not { ";
const char  SupportDEFG_E[]     = "}if ";
const char  IndexArray16b[]     = " dup length 1 sub 3 -1 roll mul dup dup floor cvi\
 exch ceiling cvi 3 index exch get 32768 add 4 -1 roll 3 -1 roll get 32768 add\
 dup 3 1 roll sub 3 -1 roll dup floor cvi sub mul add ";

const char  IndexArray[]        = " dup length 1 sub 3 -1 roll mul dup dup floor cvi\
 exch ceiling cvi 3 index exch get 4 -1 roll 3 -1 roll get\
 dup 3 1 roll sub 3 -1 roll dup floor cvi sub mul add ";
const char  StartClip[]         = "dup 1.0 le{dup 0.0 ge{" ;
const char  EndClip[]           = "}if}if " ;
const char  BeginString[]       = "<";
const char  EndString[]         = ">";
const char  BeginArray[]        = "[";
const char  EndArray[]          = "]";
const char  BeginFunction[]     = "{";
const char  EndFunction[]       = "}bind ";
const char  BeginDict[]         = "<<" ;
const char  EndDict[]           = ">>" ;
const char  BlackPoint[]        = "[0 0 0]" ;
const char  DictType[]          = "/ColorRenderingType 1 ";
const char  IntentType[]        = "/RenderingIntent ";
const char  IntentPer[]         = "/Perceptual";
const char  IntentSat[]         = "/Saturation";
const char  IntentACol[]        = "/AbsoluteColorimetric";
const char  IntentRCol[]        = "/RelativeColorimetric";

const char  WhitePointTag[]     = "/WhitePoint " ;
const char  BlackPointTag[]     = "/BlackPoint " ;
const char  RangePQRTag[]       = "/RangePQR " ;
const char  TransformPQRTag[]   = "/TransformPQR " ;
const char  MatrixPQRTag[]      = "/MatrixPQR " ;
const char  RangePQR[]          = "[ -0.07 2.2 -0.02 1.4 -0.2 4.8 ]";
const char  MatrixPQR[]         = "[0.8951 -0.7502 0.0389 0.2664 1.7135 -0.0685 -0.1614 0.0367 1.0296]";
#ifdef BARDFORD_TRANSFORM
const char  *TransformPQR[3]    = {"exch pop exch 3 get mul exch pop exch 3 get div ",
                                   "exch pop exch 4 get mul exch pop exch 4 get div ",
                                   "exch pop exch 5 get mul exch pop exch 5 get div " };
#else
const char  *TransformPQR[3]    = {"4 index 0 get div 2 index 0 get mul 4 {exch pop} repeat ",
                                   "4 index 1 get div 2 index 1 get mul 4 {exch pop} repeat ",
                                   "4 index 2 get div 2 index 2 get mul 4 {exch pop} repeat " };
#endif
const char  RangeABCTag[]       = "/RangeABC " ;
const char  MatrixATag[]        = "/MatrixA ";
const char  MatrixABCTag[]      = "/MatrixABC ";
const char  EncodeABCTag[]      = "/EncodeABC " ;
const char  RangeLMNTag[]       = "/RangeLMN " ;
const char  MatrixLMNTag[]      = "/MatrixLMN " ;
const char  EncodeLMNTag[]      = "/EncodeLMN " ;
const char  RenderTableTag[]    = "/RenderTable " ;
const char  CIEBasedATag[]      = "/CIEBasedA " ;
const char  CIEBasedABCTag[]    = "/CIEBasedABC " ;
const char  CIEBasedDEFGTag[]   = "/CIEBasedDEFG " ;
const char  CIEBasedDEFTag[]    = "/CIEBasedDEF " ;
const char  DecodeATag[]        = "/DecodeA " ;
const char  DecodeABCTag[]      = "/DecodeABC " ;
const char  DecodeLMNTag[]      = "/DecodeLMN " ;
const char  DeviceRGBTag[]      = "/DeviceRGB " ;
const char  DeviceCMYKTag[]     = "/DeviceCMYK " ;
const char  DeviceGrayTag[]     = "/DeviceGray " ;
const char  TableTag[]          = "/Table " ;
const char  DecodeDEFGTag[]     = "/DecodeDEFG " ;
const char  DecodeDEFTag[]      = "/DecodeDEF " ;

const char  NullOp[]            = "";
const char  DupOp[]             = "dup ";
const char  UserDictOp[]        = "userdict ";
const char  GlobalDictOp[]      = "globaldict ";
const char  CurrentGlobalOp[]   = "currentglobal ";
const char  SetGlobalOp[]       = "setglobal ";
const char  DefOp[]             = "def ";
const char  BeginOp[]           = "begin ";
const char  EndOp[]             = "end ";
const char  TrueOp[]            = "true ";
const char  FalseOp[]           = "false ";
const char  MulOp[]             = "mul ";
const char  DivOp[]             = "div ";

const char  NewLine[]           = "\r\n" ;
const char  Slash[]             = "/" ;
const char  Space[]             = " " ;
const char  CRDBegin[]          = "%** CRD Begin ";
const char  CRDEnd[]            = "%** CRD End ";
const char  CieBasedDEFGBegin[] = "%** CieBasedDEFG CSA Begin ";
const char  CieBasedDEFBegin[]  = "%** CieBasedDEF CSA Begin ";
const char  CieBasedABCBegin[]  = "%** CieBasedABC CSA Begin ";
const char  CieBasedABegin[]    = "%** CieBasedA CSA Begin ";
const char  CieBasedDEFGEnd[]   = "%** CieBasedDEFG CSA End ";
const char  CieBasedDEFEnd[]    = "%** CieBasedDEF CSA End ";
const char  CieBasedABCEnd[]    = "%** CieBasedABC CSA End ";
const char  CieBasedAEnd[]      = "%** CieBasedA CSA End ";
const char  RangeABC[]          = "[ 0 1 0 1 0 1 ] ";
const char  RangeLMN[]          = "[ 0 2 0 2 0 2 ] ";
const char  Identity[]          = "[1 0 0 0 1 0 0 0 1]";
const char  RangeABC_Lab[]      = "[0 100 -128 127 -128 127]";
const char  Clip01[]            = "dup 1.0 ge{pop 1.0}{dup 0.0 lt{pop 0.0}if}ifelse " ;
const char  DecodeA3[]          = "256 div exp ";
const char  DecodeA3Rev[]       = "256 div 1.0 exch div exp ";
const char  DecodeABCArray[]    = "DecodeABC_";
const char  InputArray[]        = "Inp_";
const char  OutputArray[]       = "Out_";
const char  Scale8[]            = "255 div " ;
const char  Scale16[]           = "65535 div " ;
const char  Scale16XYZ[]        = "32768 div " ;
const char  TFunction8[]        = "exch 255 mul round cvi get 255 div " ;
const char  TFunction8XYZ[]     = "exch 255 mul round cvi get 128 div " ;
const char  MatrixABCLab[]      = "[1 1 1 1 0 0 0 0 -1]" ;
const char  DecodeABCLab1[]     = "[{16 add 116 div} bind {500 div} bind {200 div} bind]";
const char  DecodeALab[]        = " 50 mul 16 add 116 div ";
const char  DecodeLMNLab[]      = "dup 0.206897 ge{dup dup mul mul}{0.137931 sub 0.128419 mul} ifelse ";

const char  RangeLMNLab[]       = "[0 1 0 1 0 1]" ;
const char  EncodeLMNLab[]      = "dup 0.008856 le{7.787 mul 0.13793 add}{0.3333 exp}ifelse " ;

const char  MatrixABCLabCRD[]   = "[0 500 0 116 -500 200 0 0 -200]" ;
const char  MatrixABCXYZCRD[]   = "[0 1 0 1 0 0 0 0 1]" ;
const char  EncodeABCLab1[]     = "16 sub 100 div " ;
const char  EncodeABCLab2[]     = "128 add 255 div " ;
const char  *DecodeABCLab[]     = {"50 mul 16 add 116 div ",
                                   "128 mul 128 sub 500 div",
                                   "128 mul 128 sub 200 div"};

const char ColorSpace1[]        = "/CIEBasedABC << /DecodeLMN ";
const char ColorSpace3[]        = " exp} bind ";
const char ColorSpace5[]        = "/WhitePoint [0.9642 1 0.8249] ";

const char PreViewInArray[]     = "IPV_";
const char PreViewOutArray[]    = "OPV_";

const char sRGBColorSpaceArray[] = "[/CIEBasedABC << \r\n\
/DecodeLMN [{dup 0.03928 le {12.92321 div}{0.055 add 1.055 div 2.4 exp}ifelse} bind dup dup ] \r\n\
/MatrixLMN [0.412457 0.212673 0.019334 0.357576 0.715152 0.119192 0.180437 0.072175 0.950301] \r\n\
/WhitePoint [ 0.9505 1 1.0890 ] >> ]";

#ifdef BRADFORD_TRANSFORM
const char sRGBColorRenderingDictionary[] = "\
/RangePQR [ -0.5 2 -0.5 2 -0.5 2 ] \r\n\
/MatrixPQR [0.8951 -0.7502  0.0389 0.2664  1.7135 -0.0685 -0.1614  0.0367  1.0296] \r\n\
/TransformPQR [\
{exch pop exch 3 get mul exch pop exch 3 get div} bind \
{exch pop exch 4 get mul exch pop exch 4 get div} bind \
{exch pop exch 5 get mul exch pop exch 5 get div} bind] \r\n\
/MatrixLMN [3.240449 -0.969265  0.055643 -1.537136  1.876011 -0.204026 -0.498531  0.041556  1.057229] \r\n\
/EncodeABC [{dup 0.00304 le {12.92321 mul}{1 2.4 div exp 1.055 mul 0.055 sub}ifelse} bind dup dup] \r\n\
/WhitePoint[0.9505 1 1.0890] >>";
#else
const char sRGBColorRenderingDictionary[] = "\
/RangePQR [ -0.5 2 -0.5 2 -0.5 2 ] \r\n\
/MatrixPQR [0.8951 -0.7502  0.0389 0.2664  1.7135 -0.0685 -0.1614  0.0367  1.0296] \r\n\
/TransformPQR [\
{4 index 0 get div 2 index 0 get mul 4 {exch pop} repeat} \
{4 index 1 get div 2 index 1 get mul 4 {exch pop} repeat} \
{4 index 2 get div 2 index 2 get mul 4 {exch pop} repeat}] \r\n\
/MatrixLMN [3.240449 -0.969265  0.055643 -1.537136  1.876011 -0.204026 -0.498531  0.041556  1.057229] \r\n\
/EncodeABC [{dup 0.00304 le {12.92321 mul}{1 2.4 div exp 1.055 mul 0.055 sub}ifelse} bind dup dup] \r\n\
/WhitePoint[0.9505 1 1.0890] >>";
#endif

/******************************************************************************
 *
 *                       InternalGetPS2ColorSpaceArray
 *
 *  Function:
 *       This functions retrieves the PostScript Level 2 CSA from the profile,
 *       or creates it if the profile tag is not present
 *
 *  Arguments:
 *       hProfile  - handle identifing the profile object
 *       dwIntent  - rendering intent of CSA
 *       dwCSAType - type of CSA
 *       pbuffer   - pointer to receive the CSA
 *       pcbSize   - pointer to size of buffer. If function fails because
 *                   buffer is not big enough, it is filled with required size.
 *       pcbBinary - TRUE if binary data is requested. On return it is set to
 *                   reflect the data returned
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL
InternalGetPS2ColorSpaceArray (
    PBYTE     pProfile,
    DWORD     dwIntent,
    DWORD     dwCSAType,
    PBYTE     pBuffer,
    PDWORD    pcbSize,
    LPBOOL    pbBinary
    )
{
    DWORD dwInpBufSize;
    BOOL  bRc;

    //
    // If profile has a CSA tag, get it directly
    //

    bRc = GetCSAFromProfile(pProfile, dwIntent, dwCSAType, pBuffer,
              pcbSize, pbBinary);

    if (! bRc && (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
    {
        //
        // Create a CSA from the profile data
        //

        switch (dwCSAType)
        {
        case CSA_ABC:
            bRc = GetPS2CSA_ABC(pProfile, pBuffer, pcbSize,
                      dwIntent, pbBinary, FALSE);
            break;

        case CSA_DEF:
            bRc = GetPS2CSA_DEFG(pProfile, pBuffer, pcbSize, dwIntent,
                      TYPE_CIEBASEDDEF, pbBinary);
            break;

        case CSA_RGB:
        case CSA_Lab:

            dwInpBufSize = *pcbSize;

            //
            // We get a DEF CSA followed by an ABC CSA and set it up so that
            // on PS interpreters that do not support the DEF CSA, the ABC one
            // is active
            //

            bRc = GetPS2CSA_DEFG(pProfile, pBuffer, pcbSize, dwIntent,
                      TYPE_CIEBASEDDEF, pbBinary);

            if (bRc)
            {
                //
                // Create CieBasedABC for printers that do not support CieBasedDEF
                //

                DWORD cbNewSize = 0;
                PBYTE pNewBuffer;
                PBYTE pOldBuffer;

                if (pBuffer)
                {
                    pNewBuffer = pBuffer + *pcbSize;
                    pOldBuffer = pNewBuffer;
                    pNewBuffer += WriteObject(pNewBuffer, NewLine);
                    if (dwCSAType == CSA_Lab)
                    {
                        pNewBuffer += WriteNewLineObject(pNewBuffer, NotSupportDEFG_S);
                    }
                    cbNewSize = dwInpBufSize - (DWORD)(pNewBuffer - pBuffer);
                }
                else
                {
                    pNewBuffer = NULL;
                }

                bRc = GetPS2CSA_ABC(pProfile, pNewBuffer, &cbNewSize,
                          dwIntent, pbBinary, TRUE);

                if (pBuffer)
                {
                    pNewBuffer += cbNewSize;
                    if (dwCSAType == CSA_Lab)
                    {
                        pNewBuffer += WriteNewLineObject(pNewBuffer, SupportDEFG_E);
                    }
                    *pcbSize += (DWORD) (pNewBuffer - pOldBuffer);
                }
                else
                {
                    *pcbSize += cbNewSize;
                }

            }
            else
            {
                *pcbSize = dwInpBufSize;

                bRc = GetPS2CSA_ABC(pProfile, pBuffer, pcbSize, dwIntent, pbBinary, FALSE);
            }

            break;

        case CSA_CMYK:
        case CSA_DEFG:
            bRc = GetPS2CSA_DEFG(pProfile, pBuffer, pcbSize, dwIntent,
                      TYPE_CIEBASEDDEFG, pbBinary);
            break;

        case CSA_GRAY:
        case CSA_A:
            bRc = GetPS2CSA_MONO_A(pProfile, pBuffer, pcbSize, dwIntent, pbBinary);
            break;

        default:
            WARNING((__TEXT("Invalid CSA type passed to GetPS2ColorSpaceArray: %d\n"), dwCSAType));
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }

    return bRc;
}


/******************************************************************************
 *
 *                       InternalGetPS2ColorRenderingIntent
 *
 *  Function:
 *       This functions retrieves the PostScript Level 2 color rendering intent
 *       from the profile, or creates it if the profile tag is not present
 *
 *  Arguments:
 *       hProfile  - handle identifing the profile object
 *       pbuffer   - pointer to receive the color rendering intent
 *       pcbSize   - pointer to size of buffer. If function fails because
 *                   buffer is not big enough, it is filled with required size.
 *       pcbBinary - TRUE if binary data is requested. On return it is set to
 *                   reflect the data returned
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL
InternalGetPS2ColorRenderingIntent(
    PBYTE     pProfile,
    DWORD     dwIntent,
    PBYTE     pBuffer,
    PDWORD    pcbSize
    )
{
    DWORD dwIndex, dwTag, dwSize;
    BOOL  bRc = FALSE;

    switch (dwIntent)
    {
    case INTENT_PERCEPTUAL:
        dwTag = TAG_PS2INTENT0;
        break;

    case INTENT_RELATIVE_COLORIMETRIC:
        dwTag = TAG_PS2INTENT1;
        break;

    case INTENT_SATURATION:
        dwTag = TAG_PS2INTENT2;
        break;

    case INTENT_ABSOLUTE_COLORIMETRIC:
        dwTag = TAG_PS2INTENT3;
        break;

    default:
        WARNING((__TEXT("Invalid intent passed to GetPS2ColorRenderingIntent: %d\n"), dwIntent));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (DoesCPTagExist(pProfile, dwTag, &dwIndex))
    {
        (void)GetCPElementDataSize(pProfile, dwIndex, &dwSize);

        if (pBuffer)
        {
            if (*pcbSize >= dwSize + 1) // for NULL terminating
            {
                bRc = GetCPElementData(pProfile, dwIndex, pBuffer, &dwSize);
            }
            else
            {
                WARNING((__TEXT("Buffer too small to get CRI\n")));
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
            }
        }
        else
            bRc = TRUE;

        *pcbSize = dwSize;
    }
    else
    {
        WARNING((__TEXT("psi tag not present for intent %d in profile\n"), dwIntent));
        SetLastError(ERROR_TAG_NOT_PRESENT);
    }

    //
    // NULL terminate
    //

    if (bRc)
    {
        if (pBuffer)
        {
            pBuffer[*pcbSize] = '\0';
        }
        (*pcbSize)++;
    }

    return bRc;
}


/******************************************************************************
 *
 *                       InternalGetPS2ColorRenderingDictionary
 *
 *  Function:
 *       This functions retrieves the PostScript Level 2 CRD from the profile,
 *       or creates it if the profile tag is not preesnt
 *
 *  Arguments:
 *       hProfile  - handle identifing the profile object
 *       dwIntent  - intent whose CRD is required
 *       pbuffer   - pointer to receive the CSA
 *       pcbSize   - pointer to size of buffer. If function fails because
 *                   buffer is not big enough, it is filled with required size.
 *       pcbBinary - TRUE if binary data is requested. On return it is set to
 *                   reflect the data returned
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL
InternalGetPS2ColorRenderingDictionary(
    PBYTE     pProfile,
    DWORD     dwIntent,
    PBYTE     pBuffer,
    PDWORD    pcbSize,
    PBOOL     pbBinary
    )
{
    DWORD dwIndex, dwSize, dwDataType;
    DWORD dwCRDTag, dwBToATag;
    BOOL  bRc = FALSE;

    switch (dwIntent)
    {
    case INTENT_PERCEPTUAL:
        dwCRDTag = TAG_CRDINTENT0;
        dwBToATag = TAG_BToA0;
        break;

    case INTENT_RELATIVE_COLORIMETRIC:
        dwCRDTag = TAG_CRDINTENT1;
        dwBToATag = TAG_BToA1;
        break;

    case INTENT_SATURATION:
        dwCRDTag = TAG_CRDINTENT2;
        dwBToATag = TAG_BToA2;
        break;

    case INTENT_ABSOLUTE_COLORIMETRIC:
        dwCRDTag = TAG_CRDINTENT3;
        dwBToATag = TAG_BToA1;
        break;

    default:
        WARNING((__TEXT("Invalid intent passed to GetPS2ColorRenderingDictionary: %d\n"), dwIntent));
        SetLastError(ERROR_INVALID_PARAMETER);
        return bRc;
    }

    if (DoesCPTagExist(pProfile, dwCRDTag, &dwIndex))
    {
        (void)GetCPElementDataSize(pProfile, dwIndex, &dwSize);

        (void)GetCPElementDataType(pProfile, dwIndex, &dwDataType);

        if (! *pbBinary && dwDataType == 1)
        {
            //
            // Profile has binary data, user asked for ASCII, so we have to
            // ASCII 85 encode it
            //

            dwSize = dwSize * 5 / 4 + sizeof(ASCII85DecodeBegin) + sizeof(ASCII85DecodeEnd) + 2048;
        }

        if (pBuffer)
        {
            if (*pcbSize >= dwSize)
            {
                (void)GetCPElementData(pProfile, dwIndex, pBuffer, &dwSize);

                if (! *pbBinary && dwDataType == 1)
                {
                    dwSize = Ascii85Encode(pBuffer, dwSize, *pcbSize);
                }
                bRc = TRUE;
            }
            else
            {
                WARNING((__TEXT("Buffer too small to get CRD\n")));
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
            }
        }
        else
            bRc = TRUE;

        *pcbSize = dwSize;
    }
    else if (DoesCPTagExist(pProfile, dwBToATag, &dwIndex))
    {
        bRc = CreateLutCRD(pProfile, dwIndex, pBuffer, pcbSize, dwIntent, *pbBinary);
    }
    else if (DoesCPTagExist(pProfile, TAG_GRAYTRC, &dwIndex))
    {
        bRc = CreateMonoCRD(pProfile, dwIndex, pBuffer, pcbSize, dwIntent);
    }
#if !defined(KERNEL_MODE) || defined(USERMODE_DRIVER)
    else if (DoesTRCAndColorantTagExist(pProfile))
    {
        bRc = CreateMatrixCRD(pProfile, pBuffer, pcbSize, dwIntent, *pbBinary);
    }
#endif // !defined(KERNEL_MODE) || defined(USERMODE_DRIVER)
    else
    {
        WARNING((__TEXT("Profile doesn't have tags to create CRD\n")));
        SetLastError(ERROR_INVALID_PROFILE);
    }

    return bRc;
}

#if !defined(KERNEL_MODE) || defined(USERMODE_DRIVER)

/******************************************************************************
 *
 *                    InternalGetPS2PreviewCRD
 *
 *  Function:
 *       This functions creates a preview PostScript Level 2 CRD from the
 *       specified destination and target profiles
 *       To do this, it does the following:
 *           1) Creates host deviceCRD deviceCSA targetCRD.
 *           2) Creates proofing CRD by sampling deviceCRD deviceCSA and targetCRD.
 *           3) Uses deviceCRD's input table as proofingCRD's input table.
 *           4) Uses targetCRD's output table as proofingCRD's output table.
 *           5) Sample data is XYZ or Lab, depends on PCS of targetCRD.
 *
 *  Arguments:
 *       pDestProf    - memory mapped pointer to destination profile
 *       pTargetProf  - memory mapped pointer to target profile
 *       dwIntent     - intent whose CRD is required
 *       pbuffer      - pointer to receive the CSA
 *       pcbSize      - pointer to size of buffer. If function fails because
 *                      buffer is not big enough, it is filled with required size.
 *       pcbBinary    - TRUE if binary data is requested. On return it is set to
 *                      reflect the data returned
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL
InternalGetPS2PreviewCRD(
    PBYTE   pDestProf,
    PBYTE   pTargetProf,
    DWORD   dwIntent,
    PBYTE   pBuffer,
    PDWORD  pcbSize,
    PBOOL   pbBinary
    )
{
    DWORD      i, j, k, l, dwDev, dwTag, dwPCS;
    DWORD      dwInArraySize, dwOutArraySize;
    DWORD      nDevGrids, nTargetGrids, nPreviewCRDGrids;
    DWORD      cbDevCRD, cbTargetCSA, cbTargetCRD;
    float      fInput[MAXCHANNELS];
    float      fOutput[MAXCHANNELS];
    float      fTemp[MAXCHANNELS];
    PBYTE      pLineStart, pStart = pBuffer;
    PBYTE      lpDevCRD = NULL, lpTargetCSA = NULL, lpTargetCRD = NULL;
    char       pPublicArrayName[5];
    BOOL       bRc = FALSE;

    dwDev = GetCPDevSpace(pTargetProf);
    i = (dwDev == SPACE_CMYK) ? 4 : 3;

    //
    // Get the input array size IntentTag and Grid of the destination profile
    //

    if (!GetCRDInputOutputArraySize(
            pTargetProf,
            dwIntent,
            &dwInArraySize,
            NULL,
            &dwTag,
            &nTargetGrids))
        return FALSE;

    //
    // Get the output array size IntentTag and Grid of the target profile
    //

    if (!GetCRDInputOutputArraySize(
            pDestProf,
            dwIntent,
            NULL,
            &dwOutArraySize,
            &dwTag,
            &nDevGrids))
        return FALSE;

    nPreviewCRDGrids = (nTargetGrids > nDevGrids) ? nTargetGrids : nDevGrids;

    //
    // Min proofing CRD grid will be PREVIEWCRDGRID
    //

    if (nPreviewCRDGrids < PREVIEWCRDGRID)
        nPreviewCRDGrids = PREVIEWCRDGRID;

    if (pBuffer == NULL)
    {
        //
        // Return size of buffer needed
        //

        *pcbSize = nPreviewCRDGrids * nPreviewCRDGrids * nPreviewCRDGrids *
                   i * 2        +    // CLUT size (Hex output)
                   dwOutArraySize +  // Output Array size
                   dwInArraySize  +  // Input Array size
                   4096;             // Extra PostScript stuff

        //
        // Add space for new line.
        //

        *pcbSize += (((*pcbSize) / MAX_LINELEN) + 1) * STRLEN(NewLine);

        return TRUE;
    }

    //
    // Query the sizes of host target CRD, target CSA and device CRD
    //

    if (!GetHostColorRenderingDictionary(pTargetProf, dwIntent, NULL, &cbTargetCRD) ||
        !GetHostColorSpaceArray(pTargetProf, dwIntent, NULL, &cbTargetCSA) ||
        !GetHostColorRenderingDictionary(pDestProf, dwIntent, NULL, &cbDevCRD))
    {
        return FALSE;
    }

    //
    // Allocate buffers for host target CRD, target CSA and device CRD
    //

    if (((lpTargetCRD = MemAlloc(cbTargetCRD)) == NULL) ||
        ((lpTargetCSA = MemAlloc(cbTargetCSA)) == NULL) ||
        ((lpDevCRD = MemAlloc(cbDevCRD)) == NULL))
    {
        goto Done;
    }

    //
    // Build host target CRD, target CSA and device CRD
    //

    if (!GetHostColorRenderingDictionary(pTargetProf, dwIntent, lpTargetCRD, &cbTargetCRD) ||
        !GetHostColorSpaceArray(pTargetProf, dwIntent, lpTargetCSA, &cbTargetCSA) ||
        !GetHostColorRenderingDictionary(pDestProf, dwIntent, lpDevCRD, &cbDevCRD))
    {
        goto Done;
    }

    //
    // Create global data
    //

    GetPublicArrayName(dwTag, pPublicArrayName);

    //
    // Build Proofing CRD based on Host target CRD, target CSA and dest CRD.
    // We use target CRD input tables and matrix as the input tables and
    // matrix of the ProofCRD. We use dest CRD output tables as the
    // output tables of the ProofCRD.
    //

    pBuffer += WriteNewLineObject(pBuffer, CRDBegin);

    pBuffer += EnableGlobalDict(pBuffer);
    pBuffer += BeginGlobalDict(pBuffer);

    pBuffer += CreateInputArray(pBuffer, 0, 0, pPublicArrayName,
                    0, NULL, *pbBinary, lpTargetCRD);

    pBuffer += CreateOutputArray(pBuffer, 0, 0, 0,
                    pPublicArrayName, 0, NULL, *pbBinary, lpDevCRD);

    pBuffer += EndGlobalDict(pBuffer);

    //
    // Start writing the CRD
    //

    pBuffer += WriteNewLineObject(pBuffer, BeginDict);     // Begin dictionary
    pBuffer += WriteObject(pBuffer, DictType);      // Dictionary type

    //
    // Send /RenderingIntent
    //

    switch (dwIntent)
    {
        case INTENT_PERCEPTUAL:
            pBuffer += WriteNewLineObject(pBuffer, IntentType);
            pBuffer += WriteObject(pBuffer, IntentPer);
            break;

        case INTENT_SATURATION:
            pBuffer += WriteNewLineObject(pBuffer, IntentType);
            pBuffer += WriteObject(pBuffer, IntentSat);
            break;

        case INTENT_RELATIVE_COLORIMETRIC:
            pBuffer += WriteNewLineObject(pBuffer, IntentType);
            pBuffer += WriteObject(pBuffer, IntentRCol);
            break;

        case INTENT_ABSOLUTE_COLORIMETRIC:
            pBuffer += WriteNewLineObject(pBuffer, IntentType);
            pBuffer += WriteObject(pBuffer, IntentACol);
            break;
    }

    //
    // /BlackPoint & /WhitePoint
    //

    pBuffer += SendCRDBWPoint(pBuffer, ((PHOSTCLUT)lpTargetCRD)->afxIlluminantWP);

    //
    // Send PQR - used for Absolute Colorimetric
    //

    pBuffer += SendCRDPQR(pBuffer, dwIntent, ((PHOSTCLUT)lpTargetCRD)->afxIlluminantWP);

    //
    // Send LMN - For Absolute Colorimetric use WhitePoint's XYZs
    //

    pBuffer += SendCRDLMN(pBuffer, dwIntent,
                   ((PHOSTCLUT)lpTargetCRD)->afxIlluminantWP,
                   ((PHOSTCLUT)lpTargetCRD)->afxMediaWP,
                   ((PHOSTCLUT)lpTargetCRD)->dwPCS);

    //
    // Send ABC
    //

    pBuffer += SendCRDABC(pBuffer, pPublicArrayName,
                   ((PHOSTCLUT)lpTargetCRD)->dwPCS,
                   ((PHOSTCLUT)lpTargetCRD)->nInputCh,
                   NULL,
                   ((PHOSTCLUT)lpTargetCRD)->e,
                   (((PHOSTCLUT)lpTargetCRD)->nLutBits == 8)? LUT8_TYPE : LUT16_TYPE,
                   *pbBinary);

    //
    // /RenderTable
    //

    pBuffer += WriteNewLineObject(pBuffer, RenderTableTag);
    pBuffer += WriteObject(pBuffer, BeginArray);

    pBuffer += WriteInt(pBuffer, nPreviewCRDGrids);  // Send down Na
    pBuffer += WriteInt(pBuffer, nPreviewCRDGrids);  // Send down Nb
    pBuffer += WriteInt(pBuffer, nPreviewCRDGrids);  // Send down Nc

    pLineStart = pBuffer;
    pBuffer += WriteNewLineObject(pBuffer, BeginArray);
    dwPCS = ((PHOSTCLUT)lpDevCRD)->dwPCS;

    for (i=0; i<nPreviewCRDGrids; i++)        // Na strings should be sent
    {
        pBuffer += WriteObject(pBuffer, NewLine);
        pLineStart = pBuffer;
        if (*pbBinary)
        {
            pBuffer += WriteStringToken(pBuffer, 143,
                nPreviewCRDGrids * nPreviewCRDGrids * ((PHOSTCLUT)lpDevCRD)->nOutputCh);
        }
        else
        {
            pBuffer += WriteObject(pBuffer, BeginString);
        }
        fInput[0] = ((float)i) / (nPreviewCRDGrids - 1);
        for (j=0; j<nPreviewCRDGrids; j++)
        {
            fInput[1] = ((float)j) / (nPreviewCRDGrids - 1);
            for (k=0; k<nPreviewCRDGrids; k++)
            {
                fInput[2] = ((float)k) / (nPreviewCRDGrids - 1);

                DoHostConversionCRD((PHOSTCLUT)lpTargetCRD, NULL, fInput, fOutput, 1);
                DoHostConversionCSA((PHOSTCLUT)lpTargetCSA, fOutput, fTemp);
                DoHostConversionCRD((PHOSTCLUT)lpDevCRD, (PHOSTCLUT)lpTargetCSA,
                                     fTemp, fOutput, 0);
                for (l=0; l<((PHOSTCLUT)lpDevCRD)->nOutputCh; l++)
                {
                    if (*pbBinary)
                    {
                        *pBuffer++ = (BYTE)(fOutput[l] * 255);
                    }
                    else
                    {
                        pBuffer += WriteHex(pBuffer, (USHORT)(fOutput[l] * 255));
                        if ((pBuffer - pLineStart) > MAX_LINELEN)
                        {
                            pLineStart = pBuffer;
                            pBuffer += WriteObject(pBuffer, NewLine);
                        }
                    }
                }
            }
        }
        if (!*pbBinary)
            pBuffer += WriteObject(pBuffer, EndString);
    }
    pBuffer += WriteNewLineObject(pBuffer, EndArray);
    pBuffer += WriteInt(pBuffer, ((PHOSTCLUT)lpDevCRD)->nOutputCh);

    //
    // Send output table
    //

    pBuffer += SendCRDOutputTable(pBuffer, pPublicArrayName,
                    ((PHOSTCLUT)lpDevCRD)->nOutputCh,
                    (((PHOSTCLUT)lpDevCRD)->nLutBits == 8)? LUT8_TYPE : LUT16_TYPE,
                    TRUE,
                    *pbBinary);

    pBuffer += WriteNewLineObject(pBuffer, EndArray);
    pBuffer += WriteObject(pBuffer, EndDict); // End dictionary definition
    pBuffer += WriteNewLineObject(pBuffer, CRDEnd);
    bRc = TRUE;

Done:
    *pcbSize = (DWORD)(pBuffer - pStart);

    if (lpTargetCRD)
         MemFree(lpTargetCRD);
    if (lpTargetCSA)
         MemFree(lpTargetCSA);
    if (lpDevCRD)
         MemFree(lpDevCRD);

    return bRc;
}

#endif //  !defined(KERNEL_MODE) || defined(USERMODE_DRIVER)


/******************************************************************************
 *
 *                               GetCSAFromProfile
 *
 *  Function:
 *       This function gets the Color Space Array from the profile if the
 *       tag is present
 *
 *  Arguments:
 *       pProfile  - pointer to the memory mapped  profile
 *       dwIntent  - rendering intent of CSA requested
 *       dwCSAType - type of CSA requested
 *       pBuffer   - pointer to receive the CSA
 *       pcbSize   - pointer to size of buffer. If function fails because
 *                   buffer is not big enough, it is filled with required size.
 *       pcbBinary - TRUE if binary data is requested. On return it is set to
 *                   reflect the data returned
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL
GetCSAFromProfile (
    PBYTE  pProfile,
    DWORD  dwIntent,
    DWORD  dwCSAType,
    PBYTE  pBuffer,
    PDWORD pcbSize,
    PBOOL  pbBinary
    )
{
    DWORD dwDev, dwProfileIntent;
    DWORD dwIndex, dwSize, dwDataType;
    BOOL  bRc = FALSE;

    //
    // This function can fail without setting an error, so reset error
    // here to prevent confusion later
    //

    SetLastError(0);

    //
    // Get the profile's color space and rendering intent
    //

    dwDev = GetCPDevSpace(pProfile);
    dwProfileIntent = GetCPRenderIntent(pProfile);

    //
    // If the rendering intents don't match, or the profile's color space
    // is incompatible with requested CSA type, fail
    //

    if  ((dwIntent != dwProfileIntent) ||
         ((dwDev == SPACE_GRAY) &&
         ((dwCSAType != CSA_GRAY) && (dwCSAType != CSA_A))))
    {
        WARNING((__TEXT("Can't use profile's CSA tag due to different rendering intents\n")));
        return FALSE;
    }

    if (DoesCPTagExist(pProfile, TAG_PS2CSA, &dwIndex))
    {
        (void)GetCPElementDataSize(pProfile, dwIndex, &dwSize);

        (void)GetCPElementDataType(pProfile, dwIndex, &dwDataType);

        if (! *pbBinary && dwDataType == 1)
        {
            //
            // Profile has binary data, user asked for ASCII, so we have to
            // ASCII 85 encode it
            //

            dwSize = dwSize * 5 / 4 + sizeof(ASCII85DecodeBegin) + sizeof(ASCII85DecodeEnd) + 2048;
        }

        if (pBuffer)
        {
            if (*pcbSize >= dwSize)
            {
                (void)GetCPElementData(pProfile, dwIndex, pBuffer, &dwSize);

                if (! *pbBinary && dwDataType == 1)
                {
                    dwSize = Ascii85Encode(pBuffer, dwSize, *pcbSize);
                }
                bRc = TRUE;
            }
            else
            {
                WARNING((__TEXT("Buffer too small to get CSA\n")));
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
            }
        }
        else
            bRc = TRUE;

        *pcbSize = dwSize;
    }

    return bRc;
}


/******************************************************************************
 *
 *                              GetPS2CSA_MONO_A
 *
 *  Function:
 *       This function creates a CIEBasedA colorspace array from an input
 *       GRAY profile
 *
 *  Arguments:
 *       pProfile  - pointer to the memory mapped profile
 *       pBuffer   - pointer to receive the CSA
 *       pcbSize   - pointer to size of buffer. If function fails because
 *                   buffer is not big enough, it is filled with required size.
 *       dwIntent  - rendering intent of CSA requested
 *       pcbBinary - TRUE if binary data is requested. On return it is set to
 *                   reflect the data returned
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL
GetPS2CSA_MONO_A(
    PBYTE  pProfile,
    PBYTE  pBuffer,
    PDWORD pcbSize,
    DWORD  dwIntent,
    PBOOL  pbBinary
    )
{
    PCURVETYPE pData;
    PTAGDATA   pTagData;
    PBYTE      pLineStart, pStart = pBuffer;
    PBYTE      pTable;
    DWORD      i, dwPCS, nCount;
    DWORD      dwIndex, dwSize;
    DWORD      afxIlluminantWP[3];
    DWORD      afxMediaWP[3];

    //
    // Check if we can generate the CSA
    // Required tag is gray TRC
    //

    if (! DoesCPTagExist(pProfile, TAG_GRAYTRC, &dwIndex))
    {
        WARNING((__TEXT("Gray TRC tag not present to create MONO_A CSA\n")));
        SetLastError(ERROR_TAG_NOT_PRESENT);
        return FALSE;
    }

    dwPCS = GetCPConnSpace(pProfile);

    (void)GetCPElementSize(pProfile, dwIndex, &dwSize);

    pTagData = (PTAGDATA)(pProfile + sizeof(PROFILEHEADER) + sizeof(DWORD) +
               dwIndex * sizeof(TAGDATA));

    pData = (PCURVETYPE)(pProfile + FIX_ENDIAN(pTagData->dwOffset));

    nCount = FIX_ENDIAN(pData->nCount);

    //
    // Estimate size required to hold the CSA
    //

    dwSize = nCount * 6 +               // Number of INT elements
        3 * (STRLEN(IndexArray) +
             STRLEN(StartClip) +
             STRLEN(EndClip)) +
        2048;                           // + other PS stuff

    //
    // Add space for new line.
    //

    dwSize += (((dwSize) / MAX_LINELEN) + 1) * STRLEN(NewLine);

    if (! pBuffer)
    {
        *pcbSize = dwSize;
        return TRUE;
    }
    else if (*pcbSize < dwSize)
    {
        WARNING((__TEXT("Buffer too small to get MONO_A CSA\n")));
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    //
    // Get info about Illuminant White Point from the header
    //

    (void)GetCPWhitePoint(pProfile, afxIlluminantWP);

    //
    // Support absolute whitePoint
    //

    (void)GetMediaWP(pProfile, dwIntent, afxIlluminantWP, afxMediaWP);

    //
    // Start creating the ColorSpace
    //

    pBuffer += WriteNewLineObject(pBuffer, CieBasedABegin);

    pBuffer += WriteNewLineObject(pBuffer, BeginArray);   // Begin array

    //
    // /CIEBasedA
    //

    pBuffer += WriteObject(pBuffer, CIEBasedATag); // Create entry
    pBuffer += WriteObject(pBuffer, BeginDict);    // Begin dictionary

    //
    // Send /BlackPoint & /WhitePoint
    //

    pBuffer += SendCSABWPoint(pBuffer, dwIntent, afxIlluminantWP, afxMediaWP);

    //
    // /DecodeA
    //

    pBuffer += WriteObject(pBuffer, NewLine);
    pLineStart = pBuffer;

    if (nCount != 0)
    {
        pBuffer += WriteObject(pBuffer, DecodeATag);
        pBuffer += WriteObject(pBuffer, BeginArray);

        pBuffer += WriteObject(pBuffer, BeginFunction);

        pTable = (PBYTE)(pData->data);

        if (nCount == 1)                // Gamma supplied in ui16 format
        {
            pBuffer += WriteInt(pBuffer, FIX_ENDIAN16(*((PWORD)pTable)));
            pBuffer += WriteObject(pBuffer, DecodeA3);

            //
            // If the PCS is Lab, we need to convert Lab to XYZ
            // Now, the range is from 0 --> 0.99997.
            // Actually, the conversion from Lab to XYZ is not needed
            //

            if (dwPCS == SPACE_Lab)
            {
                pBuffer += WriteObject(pBuffer, DecodeALab);
                pBuffer += WriteObject(pBuffer, DecodeLMNLab);
            }
        }
        else
        {
            pBuffer += WriteObject(pBuffer, StartClip);
            pBuffer += WriteObject(pBuffer, BeginArray);
            for (i=0; i<nCount; i++)
            {
                pBuffer += WriteInt(pBuffer, FIX_ENDIAN16(*((PWORD)pTable)));
                pTable += sizeof(WORD);
                if (((DWORD) (pBuffer - pLineStart)) > MAX_LINELEN)
                {
                    pLineStart = pBuffer;
                    pBuffer += WriteObject (pBuffer, NewLine);
                }
            }
            pBuffer += WriteObject(pBuffer, EndArray);
            pLineStart = pBuffer;
            pBuffer += WriteObject(pBuffer, NewLine);

            pBuffer += WriteObject(pBuffer, IndexArray);
            pBuffer += WriteObject(pBuffer, Scale16);

            //
            // If the PCS is Lab, we need to convert Lab to XYZ
            // Now, the range is from 0 --> .99997.
            // Actually, the conversion from Lab to XYZ is not needed.
            //

            if (dwPCS == SPACE_Lab)
            {
                pBuffer += WriteObject(pBuffer, DecodeALab);
                pBuffer += WriteObject(pBuffer, DecodeLMNLab);
            }
            pBuffer += WriteObject(pBuffer, EndClip);
        }

        pBuffer += WriteObject(pBuffer, EndFunction);
        pBuffer += WriteObject(pBuffer, EndArray);
    }

    //
    // /MatrixA
    //

    pBuffer += WriteNewLineObject(pBuffer, MatrixATag);
    pBuffer += WriteObject(pBuffer, BeginArray);
    for (i=0; i<3; i++)
    {
        if (dwIntent == INTENT_ABSOLUTE_COLORIMETRIC)
        {
            pBuffer += WriteFixed(pBuffer, afxMediaWP[i]);
        }
        else
        {
            pBuffer += WriteFixed(pBuffer, afxIlluminantWP[i]);
        }
    }
    pBuffer += WriteObject(pBuffer, EndArray);

    //
    // /RangeLMN
    //

    pBuffer += WriteNewLineObject(pBuffer, RangeLMNTag);
    pBuffer += WriteObject(pBuffer, RangeLMN);

    //
    // /End dictionary
    //

    pBuffer += WriteObject(pBuffer, EndDict);  // End dictionary definition
    pBuffer += WriteObject(pBuffer, EndArray);

    pBuffer += WriteNewLineObject(pBuffer, CieBasedAEnd);

    *pcbSize = (DWORD) (pBuffer - pStart);

    return TRUE;
}


/******************************************************************************
 *
 *                              GetPS2CSA_ABC
 *
 *  Function:
 *       This function creates a CIEBasedABC colorspace array from an input
 *       RGB profile
 *
 *  Arguments:
 *       pProfile  - pointer to the memory mapped profile
 *       pBuffer   - pointer to receive the CSA
 *       pcbSize   - pointer to size of buffer. If function fails because
 *                   buffer is not big enough, it is filled with required size.
 *       dwIntent  - rendering intent of CSA requested
 *       pcbBinary - TRUE if binary data is requested. On return it is set to
 *                   reflect the data returned
 *       bBackup   - TRUE: A CIEBasedDEF has been created, this CSA is a backup
 *                          in case some old printer can not support CIEBasedDEF.
 *                   FALSE: No CIEBasedDEF. This is the only CSA.
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL
GetPS2CSA_ABC(
    PBYTE  pProfile,
    PBYTE  pBuffer,
    PDWORD pcbSize,
    DWORD  dwIntent,
    PBOOL  pbBinary,
    BOOL   bBackup
    )
{
    PBYTE     pStart = pBuffer;
    DWORD     i, dwPCS, dwDev, dwSize;
    FIX_16_16 afxIlluminantWP[3];
    FIX_16_16 afxMediaWP[3];

    //
    // Check if we can generate the CSA:
    // Required  tags are red, green and blue Colorants & TRCs
    //

    dwPCS = GetCPConnSpace(pProfile);
    dwDev = GetCPDevSpace(pProfile);

    //
    // Call another function to handle Lab profiles
    //

    if (dwDev == SPACE_Lab)
    {
        return GetPS2CSA_ABC_Lab(pProfile, pBuffer, pcbSize, dwIntent, pbBinary);
    }

    //
    // We only handle RGB profiles in this function
    //

    if ((dwDev != SPACE_RGB)                   ||
        !DoesTRCAndColorantTagExist(pProfile))
    {
        WARNING((__TEXT("Colorant or TRC tag not present to create ABC CSA\n")));
        SetLastError(ERROR_TAG_NOT_PRESENT);
        return FALSE;
    }

    //
    // Estimate size required to hold the CSA
    //

    dwSize = 65530;

    if (! pBuffer)
    {
        *pcbSize = dwSize;
        return TRUE;
    }
    else if (*pcbSize < dwSize)
    {
        WARNING((__TEXT("Buffer too small to get ABC CSA\n")));
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    //
    // Get info about Illuminant White Point from the header
    //

    (void)GetCPWhitePoint(pProfile, afxIlluminantWP);

    //
    // Support absolute whitePoint
    //

    (void)GetMediaWP(pProfile, dwIntent, afxIlluminantWP, afxMediaWP);

    //
    // Create global data
    //

    pBuffer += WriteNewLineObject(pBuffer, CieBasedABCBegin);

    if (IsSRGBColorProfile(pProfile))
    {
        pBuffer += WriteNewLineObject(pBuffer, sRGBColorSpaceArray);
    }
    else
    {
        pBuffer += EnableGlobalDict(pBuffer);

        if (bBackup)
        {
            pBuffer += WriteNewLineObject(pBuffer, NotSupportDEFG_S);
        }

        pBuffer += BeginGlobalDict(pBuffer);

        pBuffer += CreateColSpArray(pProfile, pBuffer, TAG_REDTRC, *pbBinary);
        pBuffer += CreateColSpArray(pProfile, pBuffer, TAG_GREENTRC, *pbBinary);
        pBuffer += CreateColSpArray(pProfile, pBuffer, TAG_BLUETRC, *pbBinary);

        pBuffer += WriteNewLineObject(pBuffer, EndOp);

        if (bBackup)
        {
            pBuffer += WriteNewLineObject(pBuffer, SupportDEFG_E);
        }

        pBuffer += WriteNewLineObject(pBuffer, SetGlobalOp);

        if (bBackup)
        {
            pBuffer += WriteNewLineObject(pBuffer, NotSupportDEFG_S);
        }

        //
        // Start creating the ColorSpace
        //

        pBuffer += WriteNewLineObject(pBuffer, BeginArray);       // Begin array

        //
        // /CIEBasedABC
        //

        pBuffer += WriteObject(pBuffer, CIEBasedABCTag);   // Create entry
        pBuffer += WriteObject(pBuffer, BeginDict);        // Begin dictionary

        //
        // /BlackPoint & /WhitePoint
        //

        pBuffer += SendCSABWPoint(pBuffer, dwIntent, afxIlluminantWP, afxMediaWP);

        //
        // /DecodeABC
        //

        pBuffer += WriteNewLineObject(pBuffer, DecodeABCTag);
        pBuffer += WriteObject(pBuffer, BeginArray);

        pBuffer += WriteObject(pBuffer, NewLine);
        pBuffer += CreateColSpProc(pProfile, pBuffer, TAG_REDTRC, *pbBinary);
        pBuffer += WriteObject(pBuffer, NewLine);
        pBuffer += CreateColSpProc(pProfile, pBuffer, TAG_GREENTRC, *pbBinary);
        pBuffer += WriteObject(pBuffer, NewLine);
        pBuffer += CreateColSpProc(pProfile, pBuffer, TAG_BLUETRC, *pbBinary);
        pBuffer += WriteObject(pBuffer, EndArray);

        //
        // /MatrixABC
        //

        pBuffer += WriteNewLineObject(pBuffer, MatrixABCTag);
        pBuffer += WriteObject(pBuffer, BeginArray);

        pBuffer += CreateFloatString(pProfile, pBuffer, TAG_REDCOLORANT);
        pBuffer += CreateFloatString(pProfile, pBuffer, TAG_GREENCOLORANT);
        pBuffer += CreateFloatString(pProfile, pBuffer, TAG_BLUECOLORANT);

        pBuffer += WriteObject(pBuffer, EndArray);

        //
        // /RangeLMN
        //

        pBuffer += WriteObject(pBuffer, NewLine);
        pBuffer += WriteObject(pBuffer, RangeLMNTag);
        pBuffer += WriteObject(pBuffer, RangeLMN);

        //
        // /DecodeLMN
        //

        if (dwIntent == INTENT_ABSOLUTE_COLORIMETRIC)
        {
            //
            // Support absolute whitePoint
            //

            pBuffer += WriteNewLineObject(pBuffer, DecodeLMNTag);
            pBuffer += WriteObject(pBuffer, BeginArray);
            for (i=0; i<3; i++)
            {
                pBuffer += WriteObject(pBuffer, BeginFunction);
                pBuffer += WriteFixed(pBuffer, FIX_DIV(afxMediaWP[i], afxIlluminantWP[i]));
                pBuffer += WriteObject(pBuffer, MulOp);
                pBuffer += WriteObject(pBuffer, EndFunction);
            }
            pBuffer += WriteObject (pBuffer, EndArray);
        }

        //
        // End dictionary definition
        //

        pBuffer += WriteNewLineObject(pBuffer, EndDict);
        pBuffer += WriteObject(pBuffer, EndArray);

        pBuffer += WriteNewLineObject(pBuffer, CieBasedABCEnd);
    }

    *pcbSize = (DWORD) (pBuffer - pStart);

    return TRUE;
}


/******************************************************************************
 *
 *                              GetPS2CSA_ABC_Lab
 *
 *  Function:
 *       This function creates a CIEBasedABC colorspace array from an input
 *       Lab profile
 *
 *  Arguments:
 *       pProfile  - pointer to the memory mapped profile
 *       pBuffer   - pointer to receive the CSA
 *       pcbSize   - pointer to size of buffer. If function fails because
 *                   buffer is not big enough, it is filled with required size.
 *       dwIntent  - rendering intent of CSA requested
 *       pcbBinary - TRUE if binary data is requested. On return it is set to
 *                   reflect the data returned
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL
GetPS2CSA_ABC_Lab(
    PBYTE  pProfile,
    PBYTE  pBuffer,
    PDWORD pcbSize,
    DWORD  dwIntent,
    PBOOL  pbBinary
    )
{
    PBYTE     pStart = pBuffer;
    DWORD     i, dwSize;
    FIX_16_16 afxIlluminantWP[3];
    FIX_16_16 afxMediaWP[3];

    //
    // Estimate size required to hold the CSA
    //

    dwSize = 65530;

    if (! pBuffer)
    {
        *pcbSize = dwSize;
        return TRUE;
    }
    else if (*pcbSize < dwSize)
    {
        WARNING((__TEXT("Buffer too small to get ABC_Lab CSA\n")));
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    //
    // Get info about Illuminant White Point from the header
    //

    (void)GetCPWhitePoint(pProfile, afxIlluminantWP);

    //
    // Support absolute whitePoint
    //

    GetMediaWP(pProfile, dwIntent, afxIlluminantWP, afxMediaWP);

    //
    // Start creating the ColorSpace
    //

    pBuffer += WriteNewLineObject(pBuffer, BeginArray);       // Begin array

    //
    // /CIEBasedABC
    //

    pBuffer += WriteObject(pBuffer, CIEBasedABCTag);   // Create entry
    pBuffer += WriteObject(pBuffer, BeginDict);        // Begin dictionary

    //
    // /BlackPoint & /WhitePoint
    //

    pBuffer += SendCSABWPoint(pBuffer, dwIntent, afxIlluminantWP, afxMediaWP);

    //
    // /RangeABC
    //

    pBuffer += WriteNewLineObject(pBuffer, RangeABCTag);
    pBuffer += WriteObject(pBuffer, RangeABC_Lab);

    //
    // /DecodeABC
    //

    pBuffer += WriteNewLineObject(pBuffer, DecodeABCTag);
    pBuffer += WriteObject(pBuffer, DecodeABCLab1);

    //
    // /MatrixABC
    //

    pBuffer += WriteNewLineObject(pBuffer, MatrixABCTag);
    pBuffer += WriteObject(pBuffer, MatrixABCLab);

    //
    // /DecodeLMN
    //

    pBuffer += WriteNewLineObject(pBuffer, DecodeLMNTag);
    pBuffer += WriteObject(pBuffer, BeginArray);
    for (i=0; i<3; i++)
    {
        pBuffer += WriteObject(pBuffer, BeginFunction);
        pBuffer += WriteObject(pBuffer, DecodeLMNLab);

        if (dwIntent == INTENT_ABSOLUTE_COLORIMETRIC)
        {
            pBuffer += WriteFixed(pBuffer, afxMediaWP[i]);
        }
        else
        {
            pBuffer += WriteFixed(pBuffer, afxIlluminantWP[i]);
        }
        pBuffer += WriteObject(pBuffer, MulOp);
        pBuffer += WriteObject(pBuffer, EndFunction);
        pBuffer += WriteObject(pBuffer, NewLine);
    }
    pBuffer += WriteObject(pBuffer, EndArray);


    //
    // End dictionary definition
    //

    pBuffer += WriteNewLineObject(pBuffer, EndDict);
    pBuffer += WriteObject(pBuffer, EndArray);

    pBuffer += WriteNewLineObject(pBuffer, CieBasedABCEnd);

    *pcbSize = (DWORD) (pBuffer - pStart);

    return TRUE;
}


/******************************************************************************
 *
 *                           GetPS2CSA_DEFG
 *
 *  Function:
 *       This function creates DEF and DEFG based CSAs from the data supplied
 *       in the RGB or CMYK profiles respectively
 *
 *  Arguments:
 *       pProfile  - pointer to the memory mapped profile
 *       pBuffer   - pointer to receive the CSA
 *       pcbSize   - pointer to size of buffer. If function fails because
 *                   buffer is not big enough, it is filled with required size.
 *       dwIntent  - rendering intent of CSA requested
 *       dwType    - whether DEF CSA or DEFG CSA is required
 *       pcbBinary - TRUE if binary data is requested. On return it is set to
 *                   reflect the data returned
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL
GetPS2CSA_DEFG(
    PBYTE  pProfile,
    PBYTE  pBuffer,
    PDWORD pcbSize,
    DWORD  dwIntent,
    DWORD  dwType,
    PBOOL  pbBinary
    )
{
    PLUT16TYPE pLut;
    PTAGDATA   pTagData;
    PBYTE      pLineStart, pStart = pBuffer;
    PBYTE      pTable;
    DWORD      i, j, k, dwPCS, dwDev, dwIndex, dwTag, dwLutSig, SecondGrids, dwSize;
    DWORD      nInputCh, nOutputCh, nGrids, nInputTable, nOutputTable, nNumbers;
    FIX_16_16  afxIlluminantWP[3];
    FIX_16_16  afxMediaWP[3];
    char       pPublicArrayName[5];

    //
    // Make sure required tags exist
    //

    switch (dwIntent)
    {
    case INTENT_PERCEPTUAL:
        dwTag = TAG_AToB0;
        break;

    case INTENT_RELATIVE_COLORIMETRIC:
        dwTag = TAG_AToB1;
        break;

    case INTENT_SATURATION:
        dwTag = TAG_AToB2;
        break;

    case INTENT_ABSOLUTE_COLORIMETRIC:
        dwTag = TAG_AToB1;
        break;

    default:
        WARNING((__TEXT("Invalid intent passed to GetPS2CSA_DEFG: %d\n"), dwIntent));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (! DoesCPTagExist(pProfile, dwTag, &dwIndex))
    {
        WARNING((__TEXT("AToB%d tag not present to create DEF(G) CSA\n"), dwIntent));
        SetLastError(ERROR_TAG_NOT_PRESENT);
        return FALSE;
    }

    //
    // Check if we can generate the CSA
    // Required  tags is AToBi, where i is the rendering intent
    //

    dwPCS = GetCPConnSpace(pProfile);
    dwDev = GetCPDevSpace(pProfile);

    if ((dwType == TYPE_CIEBASEDDEF  && dwDev != SPACE_RGB) ||
        (dwType == TYPE_CIEBASEDDEFG && dwDev != SPACE_CMYK))
    {
        WARNING((__TEXT("RGB profile & requesting CMYK CSA or vice versa\n")));
        SetLastError(ERROR_TAG_NOT_PRESENT);
        return FALSE;
    }

    pTagData = (PTAGDATA)(pProfile + sizeof(PROFILEHEADER) + sizeof(DWORD) +
               dwIndex * sizeof(TAGDATA));

    pLut = (PLUT16TYPE)(pProfile + FIX_ENDIAN(pTagData->dwOffset));

    dwLutSig = FIX_ENDIAN(pLut->dwSignature);

    if (((dwPCS != SPACE_Lab) && (dwPCS != SPACE_XYZ)) ||
        ((dwLutSig != LUT8_TYPE) && (dwLutSig != LUT16_TYPE)))
    {
        WARNING((__TEXT("Invalid color space - unable to create DEF(G) CSA\n")));
        SetLastError(ERROR_INVALID_PROFILE);
        return FALSE;
    }

    //
    // Estimate size required to hold the CSA
    //

    (void)GetCLUTInfo(dwLutSig, (PBYTE)pLut, &nInputCh, &nOutputCh, &nGrids,
                &nInputTable, &nOutputTable, NULL);

    //
    // Calculate size of buffer needed
    //

    if (dwType == TYPE_CIEBASEDDEFG)
    {
        dwSize = nOutputCh * nGrids * nGrids * nGrids * nGrids * 2;
    }
    else
    {
        dwSize = nOutputCh * nGrids * nGrids * nGrids * 2;
    }

    dwSize = dwSize +
        nInputCh * nInputTable * 6 +
        nOutputCh * nOutputTable * 6 +      // Number of INT bytes
        nInputCh * (STRLEN(IndexArray) +
                    STRLEN(StartClip) +
                    STRLEN(EndClip)) +
        nOutputCh * (STRLEN(IndexArray) +
                     STRLEN(StartClip) +
                     STRLEN(EndClip)) +
        4096;                               // + other PS stuff

    //
    // Add space for new line.
    //

    dwSize += (((dwSize) / MAX_LINELEN) + 1) * STRLEN(NewLine);

    if (! pBuffer)
    {
        *pcbSize = dwSize;
        return TRUE;
    }
    else if (*pcbSize < dwSize)
    {
        WARNING((__TEXT("Buffer too small to get DEFG CSA\n")));
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    //
    // Get info about Illuminant White Point from the header
    //

    (void)GetCPWhitePoint(pProfile, afxIlluminantWP);

    //
    // Support absolute whitePoint
    //

    (void)GetMediaWP(pProfile, dwIntent, afxIlluminantWP, afxMediaWP);

    //
    // Testing CieBasedDEFG support
    //

    pBuffer += WriteNewLineObject(pBuffer, TestingDEFG);

    //
    // Create global data
    //

    GetPublicArrayName(dwTag, pPublicArrayName);

    if (dwType == TYPE_CIEBASEDDEFG)
    {
        pBuffer += WriteNewLineObject(pBuffer, CieBasedDEFGBegin);
    }
    else
    {
        pBuffer += WriteNewLineObject(pBuffer, CieBasedDEFBegin);
    }

    pBuffer += EnableGlobalDict(pBuffer);
    pBuffer += WriteNewLineObject(pBuffer, SupportDEFG_S);
    pBuffer += BeginGlobalDict(pBuffer);

    pBuffer += CreateInputArray(pBuffer, nInputCh, nInputTable,
                 pPublicArrayName, dwLutSig, (PBYTE)pLut, *pbBinary, NULL);

    if (dwType == TYPE_CIEBASEDDEFG)
    {
        i = nInputTable * nInputCh +
            nGrids * nGrids * nGrids * nGrids * nOutputCh;
    }
    else
    {
        i = nInputTable * nInputCh +
            nGrids * nGrids * nGrids * nOutputCh;
    }
    pBuffer += CreateOutputArray(pBuffer, nOutputCh, nOutputTable, i,
                 pPublicArrayName, dwLutSig, (PBYTE)pLut, *pbBinary, NULL);

    pBuffer += WriteNewLineObject(pBuffer, EndOp);
    pBuffer += WriteNewLineObject(pBuffer, SupportDEFG_E);
    pBuffer += WriteNewLineObject(pBuffer, SetGlobalOp);
    pBuffer += WriteNewLineObject(pBuffer, SupportDEFG_S);

    //
    // Start creating the ColorSpace
    //

    pBuffer += WriteNewLineObject(pBuffer, BeginArray);   // Begin array

    //
    // /CIEBasedDEF(G)
    //

    if (dwType == TYPE_CIEBASEDDEFG)
    {
        pBuffer += WriteObject(pBuffer, CIEBasedDEFGTag);
    }
    else
    {
        pBuffer += WriteObject(pBuffer, CIEBasedDEFTag);
    }

    pBuffer += WriteObject(pBuffer, BeginDict);    // Begin dictionary

    //
    // /BlackPoint & /WhitePoint
    //

    pBuffer += SendCSABWPoint(pBuffer, dwIntent, afxIlluminantWP, afxMediaWP);

    //
    // /DecodeDEF(G)
    //

    pLineStart = pBuffer;

    if (dwType == TYPE_CIEBASEDDEFG)
    {
        pBuffer += WriteNewLineObject(pBuffer, DecodeDEFGTag);
    }
    else
    {
        pBuffer += WriteNewLineObject(pBuffer, DecodeDEFTag);
    }

    pBuffer += WriteObject(pBuffer, BeginArray);
    for (i=0; i<nInputCh; i++)
    {
        pLineStart = pBuffer;

        pBuffer += WriteNewLineObject(pBuffer, BeginFunction);
        pBuffer += WriteObject(pBuffer, StartClip);
        pBuffer += WriteObject(pBuffer, InputArray);
        pBuffer += WriteObject(pBuffer, pPublicArrayName);
        pBuffer += WriteInt(pBuffer, i);

        if (! *pbBinary)               // Output ASCII
        {
            pBuffer += WriteObject(pBuffer, IndexArray);
        }
        else
        {                               // Output BINARY
            if (dwLutSig == LUT8_TYPE)
            {
                pBuffer += WriteObject(pBuffer, IndexArray);
            }
            else
            {
                pBuffer += WriteObject(pBuffer, IndexArray16b);
            }
        }
        pBuffer += WriteObject(pBuffer, (dwLutSig == LUT8_TYPE) ? Scale8 : Scale16);
        pBuffer += WriteObject(pBuffer, EndClip);
        pBuffer += WriteObject(pBuffer, EndFunction);
    }
    pBuffer += WriteObject(pBuffer, EndArray);

    //
    // /Table
    //

    pBuffer += WriteNewLineObject(pBuffer, TableTag);
    pBuffer += WriteObject(pBuffer, BeginArray);

    pBuffer += WriteInt(pBuffer, nGrids);  // Send down Nh
    pBuffer += WriteInt(pBuffer, nGrids);  // Send down Ni
    pBuffer += WriteInt(pBuffer, nGrids);  // Send down Nj
    nNumbers = nGrids * nGrids * nOutputCh;
    SecondGrids = 1;

    if (dwType == TYPE_CIEBASEDDEFG)
    {
        pBuffer += WriteInt (pBuffer, nGrids);  // Send down Nk
        SecondGrids = nGrids;
    }
    pBuffer += WriteNewLineObject(pBuffer, BeginArray);

    for (i=0; i<nGrids; i++)        // Nh strings should be sent
    {
        if (dwType == TYPE_CIEBASEDDEFG)
        {
            pBuffer += WriteNewLineObject(pBuffer, BeginArray);
        }
        for (k=0; k<SecondGrids; k++)
        {
            pLineStart = pBuffer;
            pBuffer += WriteObject(pBuffer, NewLine);
            if (dwLutSig == LUT8_TYPE)
            {
                pTable = (PBYTE)(((PLUT8TYPE)pLut)->data) +
                    nInputTable * nInputCh +
                    nNumbers * (i * SecondGrids + k);
            }
            else
            {
                pTable = (PBYTE)(((PLUT16TYPE)pLut)->data) +
                    2 * nInputTable * nInputCh +
                    2 * nNumbers * (i * SecondGrids + k);
            }

            if (! *pbBinary)           // Output ASCII
            {
                pBuffer += WriteObject(pBuffer, BeginString);
                if (dwLutSig == LUT8_TYPE)
                {
                    pBuffer += WriteHexBuffer(pBuffer, pTable, pLineStart, nNumbers);
                }
                else
                {
                    for (j=0; j<nNumbers; j++)
                    {
                        pBuffer += WriteHex(pBuffer, FIX_ENDIAN16(*((PWORD)pTable)) / 256);
                        pTable += sizeof(WORD);

                        if (((DWORD) (pBuffer - pLineStart)) > MAX_LINELEN)
                        {
                            pLineStart = pBuffer;
                            pBuffer += WriteObject(pBuffer, NewLine);
                        }
                    }
                }
                pBuffer += WriteObject(pBuffer, EndString);
            }
            else
            {                           // Output BINARY
                pBuffer += WriteStringToken(pBuffer, 143, nNumbers);
                if (dwLutSig == LUT8_TYPE)
                    pBuffer += WriteByteString(pBuffer, pTable, nNumbers);
                else
                    pBuffer += WriteInt2ByteString(pBuffer, pTable, nNumbers);
            }
            pBuffer += WriteObject (pBuffer, NewLine);
        }
        if (dwType == TYPE_CIEBASEDDEFG)
        {
            pBuffer += WriteObject(pBuffer, EndArray);
        }
    }
    pBuffer += WriteObject(pBuffer, EndArray);
    pBuffer += WriteObject(pBuffer, EndArray); // End array

    //
    // /DecodeABC
    //

    pLineStart = pBuffer;
    pBuffer += WriteNewLineObject(pBuffer, DecodeABCTag);
    pBuffer += WriteObject(pBuffer, BeginArray);
    for (i=0; i<nOutputCh; i++)
    {
        pLineStart = pBuffer;

        pBuffer += WriteNewLineObject(pBuffer, BeginFunction);
        pBuffer += WriteObject(pBuffer, Clip01);
        pBuffer += WriteObject(pBuffer, OutputArray);
        pBuffer += WriteObject(pBuffer, pPublicArrayName);
        pBuffer += WriteInt(pBuffer, i);

        if (! *pbBinary)
        {
            pBuffer += WriteObject(pBuffer, NewLine);

            if (dwLutSig == LUT8_TYPE)
            {
                pBuffer += WriteObject(pBuffer, TFunction8XYZ);
            }
            else
            {
                pBuffer += WriteObject(pBuffer, IndexArray);
                pBuffer += WriteObject(pBuffer, Scale16XYZ);
            }
        }
        else
        {
            if (dwLutSig == LUT8_TYPE)
            {
                pBuffer += WriteObject(pBuffer, TFunction8XYZ);
            }
            else
            {
                pBuffer += WriteObject(pBuffer, IndexArray16b);
                pBuffer += WriteObject(pBuffer, Scale16XYZ);
            }
        }

        //
        // Now, We get CieBasedXYZ output. Output range 0 --> 1.99997
        // If the connection space is absolute XYZ, We need to convert
        // from relative XYZ to absolute XYZ.
        //

        if ((dwPCS == SPACE_XYZ) && (dwIntent == INTENT_ABSOLUTE_COLORIMETRIC))
        {
            pBuffer += WriteFixed(pBuffer, FIX_DIV(afxMediaWP[i], afxIlluminantWP[i]));
            pBuffer += WriteObject(pBuffer, MulOp);
        }
        else if (dwPCS == SPACE_Lab)
        {
            //
            // If the connection space is Lab, We need to convert XYZ to Lab
            //

            pBuffer += WriteObject(pBuffer, DecodeABCLab[i]);
        }
        pBuffer += WriteObject(pBuffer, EndFunction);
    }
    pBuffer += WriteObject(pBuffer, EndArray);

    if (dwPCS == SPACE_Lab)
    {
        //
        // /MatrixABC
        //

        pBuffer += WriteNewLineObject(pBuffer, MatrixABCTag);
        pBuffer += WriteObject(pBuffer, MatrixABCLab);

        //
        // /DecodeLMN
        //

        pLineStart = pBuffer;
        pBuffer += WriteNewLineObject(pBuffer, DecodeLMNTag);
        pBuffer += WriteObject(pBuffer, BeginArray);
        for (i=0; i<3; i++)
        {
            pLineStart = pBuffer;

            pBuffer += WriteNewLineObject(pBuffer, BeginFunction);
            pBuffer += WriteObject(pBuffer, DecodeLMNLab);
            if (dwIntent == INTENT_ABSOLUTE_COLORIMETRIC)
            {
                pBuffer += WriteFixed(pBuffer, afxMediaWP[i]);
            }
            else
            {
                pBuffer += WriteFixed(pBuffer, afxIlluminantWP[i]);
            }
            pBuffer += WriteObject(pBuffer, MulOp);
            pBuffer += WriteObject(pBuffer, EndFunction);
        }
        pBuffer += WriteObject(pBuffer, EndArray);
    }
    else
    {
        //
        // /RangeLMN
        //

        pBuffer += WriteNewLineObject(pBuffer, RangeLMNTag);
        pBuffer += WriteObject(pBuffer, RangeLMN);
    }

    //
    // End dictionary definition
    //

    pBuffer += WriteNewLineObject(pBuffer, EndDict);
    pBuffer += WriteObject(pBuffer, EndArray);

    if (dwType == TYPE_CIEBASEDDEFG)
    {
        pBuffer += WriteNewLineObject(pBuffer, CieBasedDEFGEnd);
    }
    else
    {
        pBuffer += WriteNewLineObject(pBuffer, CieBasedDEFEnd);
    }

    pBuffer += WriteNewLineObject(pBuffer, SupportDEFG_E);

    *pcbSize = (DWORD) (pBuffer - pStart);

    return TRUE;
}


BOOL
InternalGetPS2CSAFromLCS(
    LPLOGCOLORSPACE pLogColorSpace,
    PBYTE           pBuffer,
    PDWORD          pcbSize,
    PBOOL           pbBinary
    )
{
    PBYTE  pStart = pBuffer;
    DWORD  dwSize = 1024*2; // same value as in pscript/icm.c

    if (! pBuffer)
    {
        *pcbSize = dwSize;

        return TRUE;
    }

    if (*pcbSize < dwSize)
    {
        WARNING((__TEXT("Buffer too small to get CSA from LCS\n")));
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    pBuffer += WriteObject(pBuffer, NewLine);
    pBuffer += WriteObject(pBuffer, BeginArray);    // Begin array

    pBuffer += WriteObject(pBuffer, ColorSpace1);
    pBuffer += WriteObject(pBuffer, BeginArray);    // [

    //
    // Red gamma
    //

    pBuffer += WriteObject(pBuffer, BeginFunction);
    pBuffer += WriteFixed(pBuffer, pLogColorSpace->lcsGammaRed);
    pBuffer += WriteObject(pBuffer, ColorSpace3);

    //
    // Green gamma
    //

    pBuffer += WriteObject(pBuffer, BeginFunction);
    pBuffer += WriteFixed(pBuffer, pLogColorSpace->lcsGammaGreen);
    pBuffer += WriteObject(pBuffer, ColorSpace3);

    //
    // Blue Gamma
    //

    pBuffer += WriteObject(pBuffer, BeginFunction);
    pBuffer += WriteFixed(pBuffer, pLogColorSpace->lcsGammaBlue);
    pBuffer += WriteObject(pBuffer, ColorSpace3);

    pBuffer += WriteObject(pBuffer, EndArray);      // ]

    pBuffer += WriteObject(pBuffer, ColorSpace5);   // /WhitePoint

    //
    // Matrix LMN
    //

    pBuffer += WriteObject(pBuffer, MatrixLMNTag);
    pBuffer += WriteObject(pBuffer, BeginArray);

    //
    // Red Value
    //

    pBuffer += WriteFixed2dot30(pBuffer, pLogColorSpace->lcsEndpoints.ciexyzRed.ciexyzX);
    pBuffer += WriteFixed2dot30(pBuffer, pLogColorSpace->lcsEndpoints.ciexyzRed.ciexyzY);
    pBuffer += WriteFixed2dot30(pBuffer, pLogColorSpace->lcsEndpoints.ciexyzRed.ciexyzZ);

    //
    // Green Value
    //

    pBuffer += WriteFixed2dot30(pBuffer, pLogColorSpace->lcsEndpoints.ciexyzGreen.ciexyzX);
    pBuffer += WriteFixed2dot30(pBuffer, pLogColorSpace->lcsEndpoints.ciexyzGreen.ciexyzY);
    pBuffer += WriteFixed2dot30(pBuffer, pLogColorSpace->lcsEndpoints.ciexyzGreen.ciexyzZ);

    //
    // Blue Value
    //

    pBuffer += WriteFixed2dot30(pBuffer, pLogColorSpace->lcsEndpoints.ciexyzBlue.ciexyzX);
    pBuffer += WriteFixed2dot30(pBuffer, pLogColorSpace->lcsEndpoints.ciexyzBlue.ciexyzY);
    pBuffer += WriteFixed2dot30(pBuffer, pLogColorSpace->lcsEndpoints.ciexyzBlue.ciexyzZ);

    pBuffer += WriteObject(pBuffer, EndArray);      // ]
    pBuffer += WriteObject(pBuffer, EndDict);       // End dictionary

    pBuffer += WriteObject(pBuffer, EndArray);      // ]

    *pcbSize = (DWORD) (pBuffer - pStart);

    return TRUE;
}


/******************************************************************************
 *
 *                           CreateColorSpArray
 *
 *  Function:
 *       This function creates an array that is used in /DecodeABC
 *
 *  Arguments:
 *       pProfile  - pointer to the memory mapped profile
 *       pBuffer   - pointer to receive the array
 *       dwCPTag   - Channel TRC tag
 *       bBinary   - TRUE if binary data is requested
 *
 *  Returns:
 *       Length of the data created in bytes
 *
 ******************************************************************************/

DWORD
CreateColSpArray(
    PBYTE pProfile,
    PBYTE pBuffer,
    DWORD dwCPTag,
    BOOL  bBinary
    )
{
    PCURVETYPE pData;
    PTAGDATA   pTagData;
    PBYTE      pLineStart, pStart = pBuffer;
    PBYTE      pTable;
    DWORD      i, nCount, dwIndex;

    pLineStart = pBuffer;

    if (DoesCPTagExist(pProfile, dwCPTag, &dwIndex))
    {
        pTagData = (PTAGDATA)(pProfile + sizeof(PROFILEHEADER) + sizeof(DWORD) +
                   dwIndex * sizeof(TAGDATA));

        pData = (PCURVETYPE)(pProfile + FIX_ENDIAN(pTagData->dwOffset));

        nCount = FIX_ENDIAN(pData->nCount);

        if (nCount > 1)
        {
            pBuffer += WriteNewLineObject(pBuffer, Slash);
            pBuffer += WriteObject(pBuffer, DecodeABCArray);
            pBuffer += WriteInt(pBuffer, dwCPTag);

            pTable = (PBYTE)(pData->data);

            if (! bBinary)           // Output ASCII CS
            {
                pBuffer += WriteObject(pBuffer, BeginArray);
                for (i=0; i<nCount; i++)
                {
                    pBuffer += WriteInt(pBuffer, FIX_ENDIAN16(*((PWORD)pTable)));
                    pTable += sizeof(WORD);

                    if (((DWORD) (pBuffer - pLineStart)) > MAX_LINELEN)
                    {
                        pLineStart = pBuffer;
                        pBuffer += WriteObject(pBuffer, NewLine);
                    }
                }
                pBuffer += WriteObject(pBuffer, EndArray);
            }
            else
            {                           // Output BINARY CS
                pBuffer += WriteHNAToken(pBuffer, 149, nCount);
                pBuffer += WriteIntStringU2S(pBuffer, pTable, nCount);
            }

            pBuffer += WriteObject(pBuffer, DefOp);
        }
    }
    return (DWORD) (pBuffer - pStart);
}


/******************************************************************************
 *
 *                           CreateColorSpProc
 *
 *  Function:
 *       This function creates a PostScript procedure for the color space
 *
 *  Arguments:
 *       pProfile  - pointer to the memory mapped profile
 *       pBuffer   - pointer to receive the procedure
 *       dwCPTag   - Channel TRC tag
 *       bBinary   - TRUE if binary data is requested
 *
 *  Returns:
 *       Length of the data created in bytes
 *
 ******************************************************************************/

DWORD
CreateColSpProc(
    PBYTE pProfile,
    PBYTE pBuffer,
    DWORD dwCPTag,
    BOOL  bBinary
    )
{
    PCURVETYPE pData;
    PTAGDATA   pTagData;
    PBYTE      pStart = pBuffer;
    PBYTE      pTable;
    DWORD      nCount, dwIndex;

    pBuffer += WriteObject(pBuffer, BeginFunction);

    if (DoesCPTagExist(pProfile, dwCPTag, &dwIndex))
    {
        pTagData = (PTAGDATA)(pProfile + sizeof(PROFILEHEADER) + sizeof(DWORD) +
                   dwIndex * sizeof(TAGDATA));

        pData = (PCURVETYPE)(pProfile + FIX_ENDIAN(pTagData->dwOffset));

        nCount = FIX_ENDIAN(pData->nCount);

        if (nCount != 0)
        {
            if (nCount == 1)            // Gamma supplied in ui16 format
            {
                pTable = (PBYTE)(pData->data);
                pBuffer += WriteInt(pBuffer, FIX_ENDIAN16(*((PWORD)pTable)));
                pBuffer += WriteObject(pBuffer, DecodeA3);
            }
            else
            {
                pBuffer += WriteObject(pBuffer, StartClip);
                pBuffer += WriteObject(pBuffer, DecodeABCArray);
                pBuffer += WriteInt(pBuffer, dwCPTag);

                if (! bBinary)       // Output ASCII CS
                {
                    pBuffer += WriteObject(pBuffer, IndexArray);
                }
                else
                {                    // Output BINARY CS
                    pBuffer += WriteObject(pBuffer, IndexArray16b);
                }
                pBuffer += WriteObject(pBuffer, Scale16);
                pBuffer += WriteObject(pBuffer, EndClip);
            }
        }
    }
    pBuffer += WriteObject(pBuffer, EndFunction);

    return (DWORD) (pBuffer - pStart);
}


/******************************************************************************
 *
 *                           CreateFloatString
 *
 *  Function:
 *       This function creates a string of floating point numbers for
 *       the X, Y and Z values of the specified colorant.
 *
 *  Arguments:
 *       pProfile  - pointer to the memory mapped profile
 *       pBuffer   - pointer to receive the string
 *       dwCPTag   - Colorant tag
 *
 *  Returns:
 *       Length of the data created in bytes
 *
 ******************************************************************************/

DWORD
CreateFloatString(
    PBYTE pProfile,
    PBYTE pBuffer,
    DWORD dwCPTag
    )
{
    PTAGDATA   pTagData;
    PBYTE      pStart = pBuffer;
    PDWORD     pTable;
    DWORD      i, dwIndex;

    if (DoesCPTagExist(pProfile, dwCPTag, &dwIndex))
    {
        pTagData = (PTAGDATA)(pProfile + sizeof(PROFILEHEADER) + sizeof(DWORD) +
                   dwIndex * sizeof(TAGDATA));

        pTable = (PDWORD)(pProfile + FIX_ENDIAN(pTagData->dwOffset)) + 2;

        for (i=0; i<3; i++)
        {
            pBuffer += WriteFixed(pBuffer, FIX_ENDIAN(*pTable));
            pTable ++;
        }
    }

    return (DWORD) (pBuffer - pStart);
}


/******************************************************************************
 *
 *                           CreateInputArray
 *
 *  Function:
 *       This function creates the Color Rendering Dictionary (CRD)
 *       from the data supplied in the ColorProfile's LUT8 or LUT16 tag.
 *
 *  Arguments:
 *       pBuffer        - pointer to receive the data
 *       nInputChannels - number of input channels
 *       nInputTable    - size of input table
 *       pIntent        - rendering intent signature (eg. A2B0)
 *       dwTag          - signature of the look up table (8 or 16 bits)
 *       pLut           - pointer to the look up table
 *       bBinary        - TRUE if binary data is requested
 *
 *  Returns:
 *       Length of the data created in bytes
 *
 ******************************************************************************/

DWORD
CreateInputArray(
    PBYTE pBuffer,
    DWORD nInputChannels,
    DWORD nInputTable,
    PBYTE pIntent,
    DWORD dwTag,
    PBYTE pLut,
    BOOL  bBinary,
    PBYTE pHostClut
    )
{
    DWORD i, j;
    PBYTE pLineStart, pStart = pBuffer;
    PBYTE pTable;

    if (pHostClut)
    {
        nInputChannels = ((PHOSTCLUT)pHostClut)->nInputCh;
        nInputTable = ((PHOSTCLUT)pHostClut)->nInputEntries;
        dwTag = ((PHOSTCLUT)pHostClut)->nLutBits == 8 ? LUT8_TYPE : LUT16_TYPE;
    }

    for (i=0; i<nInputChannels; i++)
    {
        pLineStart = pBuffer;
        pBuffer += WriteNewLineObject(pBuffer, Slash);
        if (pHostClut)
            pBuffer += WriteObject(pBuffer, PreViewInArray);
        else
            pBuffer += WriteObject(pBuffer, InputArray);

        pBuffer += WriteObject(pBuffer, pIntent);
        pBuffer += WriteInt(pBuffer, i);

        if (pHostClut)
        {
            pTable = ((PHOSTCLUT)pHostClut)->inputArray[i];
        }
        else
        {
            if (dwTag == LUT8_TYPE)
            {
                pTable = (PBYTE)(((PLUT8TYPE)pLut)->data) + nInputTable * i;
            }
            else
            {
                pTable = (PBYTE)(((PLUT16TYPE)pLut)->data) + 2 * nInputTable * i;
            }
        }

        if (! bBinary)
        {
            if (dwTag == LUT8_TYPE)
            {
                pBuffer += WriteObject(pBuffer, BeginString);
                pBuffer += WriteHexBuffer(pBuffer, pTable, pLineStart, nInputTable);
                pBuffer += WriteObject(pBuffer, EndString);
            }
            else
            {
                pBuffer += WriteObject(pBuffer, BeginArray);
                for (j=0; j<nInputTable; j++)
                {
                    if (pHostClut)
                        pBuffer += WriteInt(pBuffer, *((PWORD)pTable));
                    else
                        pBuffer += WriteInt(pBuffer, FIX_ENDIAN16(*((PWORD)pTable)));
                    pTable += sizeof(WORD);
                    if (((DWORD) (pBuffer - pLineStart)) > MAX_LINELEN)
                    {
                        pLineStart = pBuffer;
                        pBuffer += WriteObject(pBuffer, NewLine);
                    }
                }
                pBuffer += WriteObject(pBuffer, EndArray);
            }
        }
        else
        {
            if (dwTag == LUT8_TYPE)
            {
                pBuffer += WriteStringToken(pBuffer, 143, nInputTable);
                pBuffer += WriteByteString(pBuffer, pTable, nInputTable);
            }
            else
            {
                pBuffer += WriteHNAToken(pBuffer, 149, nInputTable);
                if (pHostClut)
                    pBuffer += WriteIntStringU2S_L(pBuffer, pTable, nInputTable);
                else
                    pBuffer += WriteIntStringU2S(pBuffer, pTable, nInputTable);
            }
        }
        pBuffer += WriteObject(pBuffer, DefOp);
    }

    return (DWORD) (pBuffer - pStart);
}



/******************************************************************************
 *
 *                           CreateOutputArray
 *
 *  Function:
 *       This function creates the Color Rendering Dictionary (CRD)
 *       from the data supplied in the ColorProfile's LUT8 or LUT16 tag.
 *
 *  Arguments:
 *       pBuffer        - pointer to receive the data
 *       nOutputChannels- number of output channels
 *       nOutputTable   - size of output table
 *       dwOffset       - offset into the output table
 *       pIntent        - rendering intent signature (eg. A2B0)
 *       dwTag          - signature of the look up table (8 or 16 bits)
 *       pLut           - pointer to the look up table
 *       bBinary        - TRUE if binary data is requested
 *
 *  Returns:
 *       Length of the data created in bytes
 *
 ******************************************************************************/

DWORD
CreateOutputArray(
    PBYTE pBuffer,
    DWORD nOutputChannels,
    DWORD nOutputTable,
    DWORD dwOffset,
    PBYTE pIntent,
    DWORD dwTag,
    PBYTE pLut,
    BOOL  bBinary,
    PBYTE pHostClut
    )
{
    DWORD i, j;
    PBYTE pLineStart, pStart = pBuffer;
    PBYTE pTable;

    if (pHostClut)
    {
        nOutputChannels = ((PHOSTCLUT)pHostClut)->nOutputCh;
        nOutputTable = ((PHOSTCLUT)pHostClut)->nOutputEntries;
        dwTag = ((PHOSTCLUT)pHostClut)->nLutBits == 8 ? LUT8_TYPE : LUT16_TYPE;
    }

    for (i=0; i<nOutputChannels; i++)
    {
        pLineStart = pBuffer;
        pBuffer += WriteNewLineObject(pBuffer, Slash);
        if (pHostClut)
            pBuffer += WriteObject(pBuffer, PreViewOutArray);
        else
            pBuffer += WriteObject(pBuffer, OutputArray);
        pBuffer += WriteObject(pBuffer, pIntent);
        pBuffer += WriteInt(pBuffer, i);

        if (pHostClut)
        {
            pTable = ((PHOSTCLUT)pHostClut)->outputArray[i];
        }
        else
        {
            if (dwTag == LUT8_TYPE)
            {
                pTable = (PBYTE)(((PLUT8TYPE)pLut)->data) +
                    dwOffset + nOutputTable * i;
            }
            else
            {
                pTable = (PBYTE)(((PLUT16TYPE)pLut)->data) +
                    2 * dwOffset + 2 * nOutputTable * i;
            }
        }

        if (! bBinary)
        {
            if (dwTag == LUT8_TYPE)
            {
                pBuffer += WriteObject(pBuffer, BeginString);
                pBuffer += WriteHexBuffer(pBuffer, pTable, pLineStart, nOutputTable);
                pBuffer += WriteObject(pBuffer, EndString);
            }
            else
            {
                pBuffer += WriteObject(pBuffer, BeginArray);
                for (j=0; j<nOutputTable; j++)
                {
                    if (pHostClut)
                        pBuffer += WriteInt(pBuffer, *((PWORD)pTable));
                    else
                        pBuffer += WriteInt(pBuffer, FIX_ENDIAN16(*((PWORD)pTable)));
                    pTable += sizeof(WORD);
                    if (((DWORD) (pBuffer - pLineStart)) > MAX_LINELEN)
                    {
                        pLineStart = pBuffer;
                        pBuffer += WriteObject(pBuffer, NewLine);
                    }
                }
                pBuffer += WriteObject(pBuffer, EndArray);
            }
        }
        else
        {
            if (dwTag == LUT8_TYPE)
            {
                pBuffer += WriteStringToken(pBuffer, 143, 256);
                pBuffer += WriteByteString(pBuffer, pTable, 256L);
            }
            else
            {
                pBuffer += WriteHNAToken(pBuffer, 149, nOutputTable);
                if (pHostClut)
                    pBuffer += WriteIntStringU2S_L(pBuffer, pTable, nOutputTable);
                else
                    pBuffer += WriteIntStringU2S(pBuffer, pTable, nOutputTable);
            }
        }
        pBuffer += WriteObject(pBuffer, DefOp);
    }

    return (DWORD)(pBuffer - pStart);
}


/******************************************************************************
 *
 *                           GetPublicArrayName
 *
 *  Function:
 *       This function creates a string with the lookup table's signature
 *
 *  Arguments:
 *       dwIntentSig      - the look up table signature
 *       pPublicArrayName - pointer to buffer
 *
 *  Returns:
 *       Length of the data created in bytes
 *
 ******************************************************************************/

DWORD
GetPublicArrayName(
    DWORD   dwIntentSig,
    PBYTE   pPublicArrayName
    )
{
    *((DWORD *)pPublicArrayName) = dwIntentSig;
    pPublicArrayName[sizeof(DWORD)] = '\0';

    return sizeof(DWORD) + 1;
}


/***************************************************************************
*                               CreateMonoCRD
*  function:
*    this is the function which creates the Color Rendering Dictionary (CRD)
*    from the data supplied in the GrayTRC tag.
*
*  returns:
*       BOOL        --  !=0 if the function was successful,
*                         0 otherwise.
*                       Returns number of bytes required/transferred
***************************************************************************/

BOOL
CreateMonoCRD(
    PBYTE  pProfile,
    DWORD  dwIndex,
    PBYTE  pBuffer,
    PDWORD pcbSize,
    DWORD  dwIntent
    )
{
    PTAGDATA   pTagData;
    PCURVETYPE pData;
    PBYTE      pLineStart, pStart = pBuffer;
    PWORD      pCurve, pRevCurve, pRevCurveStart;
    DWORD      dwPCS, dwSize, nCount, i;
    FIX_16_16  afxIlluminantWP[3];
    FIX_16_16  afxMediaWP[3];

    dwPCS = GetCPConnSpace(pProfile);

    pTagData = (PTAGDATA)(pProfile + sizeof(PROFILEHEADER) + sizeof(DWORD) +
               dwIndex * sizeof(TAGDATA));

    pData = (PCURVETYPE)(pProfile + FIX_ENDIAN(pTagData->dwOffset));

    nCount = FIX_ENDIAN(pData->nCount);

    //
    // Estimate size required to hold the CRD
    //

    dwSize = nCount * 6 * REVCURVE_RATIO + // Number of INT elements
        2048;                              // + other PS stuff

    //
    // Add space for new line.
    //

    dwSize += (((dwSize) / MAX_LINELEN) + 1) * STRLEN(NewLine);

    if (! pBuffer)
    {
        *pcbSize = dwSize;
        return TRUE;
    }
    else if (*pcbSize < dwSize)
    {
        WARNING((__TEXT("Buffer too small to get Mono CRD\n")));
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    //
    // Allocate memory, each entry occupy 2 bytes (1 word),
    //
    // input buffer  = (nCount * sizeof(WORD)
    // output buffer = (nCount * sizeof(WORD) * REVCURVE_RATIO)
    //

    if ((pRevCurveStart = MemAlloc(nCount * sizeof(WORD) * (REVCURVE_RATIO + 1))) == NULL)
    {
        WARNING((__TEXT("Unable to allocate memory for reverse curve\n")));
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    //
    // pCurve will points input buffer (which used in GetRevCurve)
    //

    pCurve    = pRevCurveStart + nCount * REVCURVE_RATIO;
    pRevCurve = pRevCurveStart;

    (void)GetRevCurve(pData, pCurve, pRevCurve);

    //
    // Get info about Illuminant White Point from the header
    //

    (void)GetCPWhitePoint(pProfile, afxIlluminantWP);

    //
    // Support absolute whitePoint
    //

    if (dwIntent == INTENT_ABSOLUTE_COLORIMETRIC)
    {
        if (! GetCPMediaWhitePoint(pProfile, afxMediaWP))
        {
            afxMediaWP[0] = afxIlluminantWP[0];
            afxMediaWP[1] = afxIlluminantWP[1];
            afxMediaWP[2] = afxIlluminantWP[2];
        }
    }

    //
    // Start writing the CRD
    //

    pBuffer += WriteNewLineObject(pBuffer, BeginDict); // Begin dictionary
    pBuffer += WriteObject(pBuffer, DictType); // Dictionary type

    //
    // Send /RenderingIntent
    //

    switch (dwIntent)
    {
        case INTENT_PERCEPTUAL:
            pBuffer += WriteNewLineObject(pBuffer, IntentType);
            pBuffer += WriteObject(pBuffer, IntentPer);
            break;

        case INTENT_SATURATION:
            pBuffer += WriteNewLineObject(pBuffer, IntentType);
            pBuffer += WriteObject(pBuffer, IntentSat);
            break;

        case INTENT_RELATIVE_COLORIMETRIC:
            pBuffer += WriteNewLineObject(pBuffer, IntentType);
            pBuffer += WriteObject(pBuffer, IntentRCol);
            break;

        case INTENT_ABSOLUTE_COLORIMETRIC:
            pBuffer += WriteNewLineObject(pBuffer, IntentType);
            pBuffer += WriteObject(pBuffer, IntentACol);
            break;
    }

    //
    // Send /BlackPoint & /WhitePoint
    //

    pBuffer += SendCRDBWPoint(pBuffer, afxIlluminantWP);

    //
    // Send PQR
    //

    pBuffer += SendCRDPQR(pBuffer, dwIntent, afxIlluminantWP);

    //
    // Send LMN
    //

    pBuffer += SendCRDLMN(pBuffer, dwIntent, afxIlluminantWP, afxMediaWP, dwPCS);

    //
    // /MatrixABC
    //

    if (dwPCS == SPACE_XYZ)
    {
        //
        // Switch ABC to BAC, since we want to output B
        // which is converted from Y
        //

        pBuffer += WriteNewLineObject(pBuffer, MatrixABCTag);
        pBuffer += WriteObject(pBuffer, MatrixABCXYZCRD);
    }
    else if (dwPCS == SPACE_Lab)
    {
        pBuffer += WriteNewLineObject(pBuffer, MatrixABCTag);
        pBuffer += WriteObject(pBuffer, MatrixABCLabCRD);
    }

    //
    // /EncodeABC
    //

    if (nCount != 0)
    {
        pBuffer += WriteObject(pBuffer, NewLine);
        pLineStart = pBuffer;
        pBuffer += WriteObject(pBuffer, EncodeABCTag);
        pBuffer += WriteObject(pBuffer, BeginArray);
        pBuffer += WriteObject(pBuffer, BeginFunction);
        if (nCount == 1)                // Gamma supplied in ui16 format
        {
            PBYTE pTable;

            pTable = (PBYTE) (pData->data);
            pBuffer += WriteInt(pBuffer, FIX_ENDIAN16(*((PWORD)pTable)));
            pBuffer += WriteObject(pBuffer, DecodeA3Rev);
        }
        else
        {
            if (dwPCS == SPACE_Lab)
            {
                pBuffer += WriteObject(pBuffer, EncodeABCLab1);
            }
            pBuffer += WriteObject(pBuffer, StartClip);
            pBuffer += WriteObject(pBuffer, BeginArray);
            for (i=0; i<nCount * REVCURVE_RATIO; i++)
            {
                pBuffer += WriteInt(pBuffer, *((WORD *)pRevCurve));
                pRevCurve++;
                if (((DWORD) (pBuffer - pLineStart)) > MAX_LINELEN)
                {
                    pLineStart = pBuffer;
                    pBuffer += WriteObject(pBuffer, NewLine);
                }
            }
            pBuffer += WriteObject(pBuffer, EndArray);
            pLineStart = pBuffer;

            pBuffer += WriteNewLineObject(pBuffer, IndexArray);
            pBuffer += WriteObject(pBuffer, Scale16);
            pBuffer += WriteObject(pBuffer, EndClip);
        }
        pBuffer += WriteObject (pBuffer, EndFunction);
        pBuffer += WriteObject (pBuffer, DupOp);
        pBuffer += WriteObject (pBuffer, DupOp);
        pBuffer += WriteObject (pBuffer, EndArray);
    }
    pBuffer += WriteObject(pBuffer, EndDict);  // End dictionary definition

    MemFree(pRevCurveStart);

    *pcbSize = (DWORD) (pBuffer - pStart);

    return TRUE;
}


/***************************************************************************
*                               CreateLutCRD
*  function:
*    this is the function which creates the Color Rendering Dictionary (CRD)
*    from the data supplied in the ColorProfile's LUT8 or LUT16 tag.
*
*  returns:
*       BOOL        --  !=0 if the function was successful,
*                         0 otherwise.
*                       Returns number of bytes required/transferred
***************************************************************************/

BOOL
CreateLutCRD(
    PBYTE  pProfile,
    DWORD  dwIndex,
    PBYTE  pBuffer,
    PDWORD pcbSize,
    DWORD  dwIntent,
    BOOL   bBinary
    )
{
    PTAGDATA   pTagData;
    PLUT16TYPE pLut;
    PBYTE      pTable;
    PBYTE      pLineStart, pStart = pBuffer;
    DWORD      dwPCS, dwSize, dwLutSig, dwTag, i, j;
    DWORD      nInputCh, nOutputCh, nGrids, nInputTable, nOutputTable, nNumbers;
    FIX_16_16  afxIlluminantWP[3];
    FIX_16_16  afxMediaWP[3];
    char       pPublicArrayName[5];

    //
    // Check if we can generate the CSA
    // Required  tags is AToBi, where i is the rendering intent
    //

    dwPCS = GetCPConnSpace(pProfile);

    pTagData = (PTAGDATA)(pProfile + sizeof(PROFILEHEADER) + sizeof(DWORD) +
               dwIndex * sizeof(TAGDATA));

    dwTag = FIX_ENDIAN(pTagData->tagType);

    pLut = (PLUT16TYPE)(pProfile + FIX_ENDIAN(pTagData->dwOffset));

    dwLutSig = FIX_ENDIAN(pLut->dwSignature);

    if ((dwLutSig != LUT8_TYPE) && (dwLutSig != LUT16_TYPE))
    {
        WARNING((__TEXT("Invalid profile - unable to create Lut CRD\n")));
        SetLastError(ERROR_INVALID_PROFILE);
        return FALSE;
    }

    //
    // Estimate size required to hold the CSA
    //

    (void)GetCLUTInfo(dwLutSig, (PBYTE)pLut, &nInputCh, &nOutputCh, &nGrids,
                &nInputTable, &nOutputTable, NULL);

    //
    // Calculate size of buffer needed
    //

    dwSize = nInputCh * nInputTable * 6 +
        nOutputCh * nOutputTable * 6 +               // Number of INT bytes
        nOutputCh * nGrids * nGrids * nGrids * 2 +   // LUT HEX bytes
        nInputCh * (STRLEN(IndexArray) +
                    STRLEN(StartClip) +
                    STRLEN(EndClip)) +
        nOutputCh * (STRLEN(IndexArray) +
                     STRLEN(StartClip) +
                     STRLEN(EndClip)) +
        2048;                                        // + other PS stuff

    //
    // Add space for new line.
    //

    dwSize += (((dwSize) / MAX_LINELEN) + 1) * STRLEN(NewLine);

    if (! pBuffer)
    {
        *pcbSize = dwSize;
        return TRUE;
    }
    else if (*pcbSize < dwSize)
    {
        WARNING((__TEXT("Buffer too small to get DEFG CSA\n")));
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    //
    // Get info about Illuminant White Point from the header
    //

    (void)GetCPWhitePoint(pProfile, afxIlluminantWP);

    //
    // Support absolute whitePoint
    //

    if (dwIntent == INTENT_ABSOLUTE_COLORIMETRIC)
    {
        if (! GetCPMediaWhitePoint(pProfile, afxMediaWP))
        {
            afxMediaWP[0] = afxIlluminantWP[0];
            afxMediaWP[1] = afxIlluminantWP[1];
            afxMediaWP[2] = afxIlluminantWP[2];
        }
    }

    //
    // Define global array used in EncodeABC and RenderTable
    //

    GetPublicArrayName(dwTag, pPublicArrayName);
    pBuffer += WriteNewLineObject(pBuffer, CRDBegin);

    pBuffer += EnableGlobalDict(pBuffer);
    pBuffer += BeginGlobalDict(pBuffer);

    pBuffer += CreateInputArray(pBuffer, nInputCh, nInputTable,
                 pPublicArrayName, dwLutSig, (PBYTE)pLut, bBinary, NULL);

    i = nInputTable * nInputCh +
        nGrids * nGrids * nGrids * nOutputCh;
    pBuffer += CreateOutputArray(pBuffer, nOutputCh, nOutputTable, i,
                 pPublicArrayName, dwLutSig, (PBYTE)pLut, bBinary, NULL);

    pBuffer += EndGlobalDict(pBuffer);

    //
    // Start writing the CRD
    //

    pBuffer += WriteNewLineObject(pBuffer, BeginDict); // Begin dictionary
    pBuffer += WriteObject(pBuffer, DictType); // Dictionary type

    //
    // Send /RenderingIntent
    //

    switch (dwIntent)
    {
        case INTENT_PERCEPTUAL:
            pBuffer += WriteNewLineObject(pBuffer, IntentType);
            pBuffer += WriteObject(pBuffer, IntentPer);
            break;

        case INTENT_SATURATION:
            pBuffer += WriteNewLineObject(pBuffer, IntentType);
            pBuffer += WriteObject(pBuffer, IntentSat);
            break;

        case INTENT_RELATIVE_COLORIMETRIC:
            pBuffer += WriteNewLineObject(pBuffer, IntentType);
            pBuffer += WriteObject(pBuffer, IntentRCol);
            break;

        case INTENT_ABSOLUTE_COLORIMETRIC:
            pBuffer += WriteNewLineObject(pBuffer, IntentType);
            pBuffer += WriteObject(pBuffer, IntentACol);
            break;
    }

    //
    // Send /BlackPoint & /WhitePoint
    //

    pBuffer += SendCRDBWPoint(pBuffer, afxIlluminantWP);

    //
    // Send PQR
    //

    pBuffer += SendCRDPQR(pBuffer, dwIntent, afxIlluminantWP);

    //
    // Send LMN
    //

    pBuffer += SendCRDLMN(pBuffer, dwIntent, afxIlluminantWP, afxMediaWP, dwPCS);

    //
    // Send ABC
    //

    pBuffer += SendCRDABC(pBuffer, pPublicArrayName, dwPCS, nInputCh,
                    (PBYTE)pLut, NULL, dwLutSig, bBinary);

    //
    // /RenderTable
    //

    pBuffer += WriteNewLineObject(pBuffer, RenderTableTag);
    pBuffer += WriteObject(pBuffer, BeginArray);

    pBuffer += WriteInt(pBuffer, nGrids);  // Send down Na
    pBuffer += WriteInt(pBuffer, nGrids);  // Send down Nb
    pBuffer += WriteInt(pBuffer, nGrids);  // Send down Nc

    pLineStart = pBuffer;
    pBuffer += WriteNewLineObject(pBuffer, BeginArray);
    nNumbers = nGrids * nGrids * nOutputCh;

    for (i=0; i<nGrids; i++)        // Na strings should be sent
    {
        pBuffer += WriteObject(pBuffer, NewLine);
        pLineStart = pBuffer;
        if (dwLutSig == LUT8_TYPE)
        {
            pTable = (PBYTE)(((PLUT8TYPE)pLut)->data) + nInputTable * nInputCh + nNumbers * i;
        }
        else
        {
            pTable = (PBYTE)(((PLUT16TYPE)pLut)->data) + 2 * nInputTable * nInputCh + 2 * nNumbers * i;
        }

        if (! bBinary)
        {
            pBuffer += WriteObject(pBuffer, BeginString);
            if (dwLutSig == LUT8_TYPE)
            {
                pBuffer += WriteHexBuffer(pBuffer, pTable, pLineStart, nNumbers);
            }
            else
            {
                for (j=0; j<nNumbers; j++)
                {
                    pBuffer += WriteHex(pBuffer, FIX_ENDIAN16(*((PWORD)pTable)) / 256);
                    pTable += sizeof(WORD);
                    if (((DWORD) (pBuffer - pLineStart)) > MAX_LINELEN)
                    {
                        pLineStart = pBuffer;
                        pBuffer += WriteObject(pBuffer, NewLine);
                    }
                }
            }
            pBuffer += WriteObject(pBuffer, EndString);
        }
        else
        {
            pBuffer += WriteStringToken(pBuffer, 143, nNumbers);
            if (dwLutSig == LUT8_TYPE)
            {
                pBuffer += WriteByteString(pBuffer, pTable, nNumbers);
            }
            else
            {
                pBuffer += WriteInt2ByteString(pBuffer, pTable, nNumbers);
            }
        }
    }

    pBuffer += WriteObject(pBuffer, EndArray); // End array
    pBuffer += WriteInt(pBuffer, nOutputCh);   // Send down m

    pBuffer += SendCRDOutputTable(pBuffer, pPublicArrayName,
                    nOutputCh, dwLutSig, FALSE, bBinary);

    pBuffer += WriteObject(pBuffer, EndArray); // End array
    pBuffer += WriteObject(pBuffer, EndDict);  // End dictionary definition

    pBuffer += WriteNewLineObject(pBuffer, CRDEnd);

    *pcbSize = (DWORD) (pBuffer - pStart);

    return TRUE;
}

#if !defined(KERNEL_MODE) || defined(USERMODE_DRIVER)

/***************************************************************************
*                           CreateMatrixCRD
*  function:
*    this is the function which creates the Color Rendering Dictionary (CRD)
*    from the data supplied in the redTRC, greenTRC, blueTRA, redColorant,
*    greenColorant and BlueColorant tags
*
*  returns:
*       BOOL        --  !=0 if the function was successful,
*                         0 otherwise.
*                       Returns number of bytes required/transferred
***************************************************************************/

// With matrix/TRC model, only the CIEXYZ encoding of the PCS can be used.
// So, we don't need to worry about CIELAB.

BOOL
CreateMatrixCRD(
    PBYTE  pProfile,
    PBYTE  pBuffer,
    PDWORD pcbSize,
    DWORD  dwIntent,
    BOOL   bBinary
    )
{
    PTAGDATA   pTagData;
    DWORD      dwRedTRCIndex, dwGreenTRCIndex, dwBlueTRCIndex;
    DWORD      dwRedCount, dwGreenCount, dwBlueCount;
    PBYTE      pMem = NULL;
    PCURVETYPE pRed, pGreen, pBlue;
    DWORD      i, dwSize;
    PBYTE      pStart = pBuffer;
    PWORD      pRevCurve;
    FIX_16_16  afxIlluminantWP[3];
    double     adColorant[9];
    double     adRevColorant[9];

    //
    // Check this is sRGB color profile or not.
    //

    if (IsSRGBColorProfile(pProfile))
    {
        dwSize = 4096; // hack - approx.

        //
        // Return buffer size, if this is a size request
        //

        if (! pBuffer)
        {
            *pcbSize = dwSize;
            return TRUE;
        }

        //
        // Check buffer size.
        //

        if (*pcbSize < dwSize)
        {
            WARNING((__TEXT("Buffer too small to get sRGB CRD\n")));
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        //
        // Start writing the CRD
        //

        pBuffer += WriteNewLineObject(pBuffer, CRDBegin);
        pBuffer += WriteNewLineObject(pBuffer, BeginDict);  // Begin dictionary
        pBuffer += WriteObject(pBuffer, DictType);          // Dictionary type

        //
        // Send /RenderingIntent
        //

        switch (dwIntent)
        {
            case INTENT_PERCEPTUAL:
                pBuffer += WriteNewLineObject(pBuffer, IntentType);
                pBuffer += WriteObject(pBuffer, IntentPer);
                break;

            case INTENT_SATURATION:
                pBuffer += WriteNewLineObject(pBuffer, IntentType);
                pBuffer += WriteObject(pBuffer, IntentSat);
                break;

            case INTENT_RELATIVE_COLORIMETRIC:
                pBuffer += WriteNewLineObject(pBuffer, IntentType);
                pBuffer += WriteObject(pBuffer, IntentRCol);
                break;

            case INTENT_ABSOLUTE_COLORIMETRIC:
                pBuffer += WriteNewLineObject(pBuffer, IntentType);
                pBuffer += WriteObject(pBuffer, IntentACol);
                break;
        }

        //
        // Write prepaired sRGB CRD.
        //

        pBuffer += WriteNewLineObject(pBuffer, sRGBColorRenderingDictionary);

        //
        // End CRD.
        //

        pBuffer += WriteNewLineObject(pBuffer, CRDEnd);
    }
    else
    {
        //
        // Get each TRC index for Red, Green and Blue.
        //

        if (!DoesCPTagExist(pProfile, TAG_REDTRC, &dwRedTRCIndex) ||
            !DoesCPTagExist(pProfile, TAG_GREENTRC, &dwGreenTRCIndex) ||
            !DoesCPTagExist(pProfile, TAG_BLUETRC, &dwBlueTRCIndex))
        {
            return FALSE;
        }

        //
        // Get CURVETYPE data for each Red, Green and Blue
        //

        pTagData = (PTAGDATA)(pProfile + sizeof(PROFILEHEADER) + sizeof(DWORD) +
                   dwRedTRCIndex * sizeof(TAGDATA));

        pRed = (PCURVETYPE)(pProfile + FIX_ENDIAN(pTagData->dwOffset));

        pTagData = (PTAGDATA)(pProfile + sizeof(PROFILEHEADER) + sizeof(DWORD) +
                   dwGreenTRCIndex * sizeof(TAGDATA));

        pGreen = (PCURVETYPE)(pProfile + FIX_ENDIAN(pTagData->dwOffset));

        pTagData = (PTAGDATA)(pProfile + sizeof(PROFILEHEADER) + sizeof(DWORD) +
                   dwBlueTRCIndex * sizeof(TAGDATA));

        pBlue = (PCURVETYPE)(pProfile + FIX_ENDIAN(pTagData->dwOffset));

        //
        // Get curve count for each Red, Green and Blue.
        //

        dwRedCount   = FIX_ENDIAN(pRed->nCount);
        dwGreenCount = FIX_ENDIAN(pGreen->nCount);
        dwBlueCount  = FIX_ENDIAN(pBlue->nCount);

        //
        // Estimate the memory size required to hold CRD
        //

        dwSize = (dwRedCount + dwGreenCount + dwBlueCount) * 6 * REVCURVE_RATIO +
                 4096;  // Number of INT elements + other PS stuff

        //
        // Add space for new line.
        //

        dwSize += (((dwSize) / MAX_LINELEN) + 1) * STRLEN(NewLine);

        if (pBuffer == NULL)     // This is a size request
        {
            *pcbSize = dwSize;
            return TRUE;
        }

        //
        // Check buffer size.
        //

        if (*pcbSize < dwSize)
        {
            WARNING((__TEXT("Buffer too small to get sRGB CRD\n")));
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        //
        // Allocate buffer for curves
        //

        if ((pRevCurve = MemAlloc(dwRedCount * sizeof(WORD) * (REVCURVE_RATIO + 1))) == NULL)
        {
            WARNING((__TEXT("Unable to allocate memory for reserved curve\n")));
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            MemFree(pMem);
            return FALSE;
        }

        //
        // Get info about Illuminant White Point from the header
        //

        (void)GetCPWhitePoint(pProfile, afxIlluminantWP);

        //
        // Start writing the CRD
        //

        pBuffer += EnableGlobalDict(pBuffer);
        pBuffer += BeginGlobalDict(pBuffer);

        pBuffer += CreateCRDRevArray(pProfile, pBuffer, pRed, pRevCurve, TAG_REDTRC, bBinary);
        pBuffer += CreateCRDRevArray(pProfile, pBuffer, pGreen, pRevCurve, TAG_GREENTRC, bBinary);
        pBuffer += CreateCRDRevArray(pProfile, pBuffer, pBlue, pRevCurve, TAG_BLUETRC, bBinary);

        pBuffer += EndGlobalDict(pBuffer);

        //
        // Start writing the CRD
        //

        pBuffer += WriteNewLineObject(pBuffer, CRDBegin);
        pBuffer += WriteNewLineObject(pBuffer, BeginDict);  // Begin dictionary
        pBuffer += WriteObject(pBuffer, DictType);          // Dictionary type

        //
        // Send /RenderingIntent
        //

        switch (dwIntent)
        {
            case INTENT_PERCEPTUAL:
                pBuffer += WriteNewLineObject(pBuffer, IntentType);
                pBuffer += WriteObject(pBuffer, IntentPer);
                break;

            case INTENT_SATURATION:
                pBuffer += WriteNewLineObject(pBuffer, IntentType);
                pBuffer += WriteObject(pBuffer, IntentSat);
                break;

            case INTENT_RELATIVE_COLORIMETRIC:
                pBuffer += WriteNewLineObject(pBuffer, IntentType);
                pBuffer += WriteObject(pBuffer, IntentRCol);
                break;

            case INTENT_ABSOLUTE_COLORIMETRIC:
                pBuffer += WriteNewLineObject(pBuffer, IntentType);
                pBuffer += WriteObject(pBuffer, IntentACol);
                break;
        }

        //
        // Send /BlackPoint & /WhitePoint
        //

        pBuffer += SendCRDBWPoint(pBuffer, afxIlluminantWP);

        //
        // Send PQR
        //

        pBuffer += SendCRDPQR(pBuffer, dwIntent, afxIlluminantWP);

        //
        // Send LMN
        //

        CreateColorantArray(pProfile, &adColorant[0], TAG_REDCOLORANT);
        CreateColorantArray(pProfile, &adColorant[3], TAG_GREENCOLORANT);
        CreateColorantArray(pProfile, &adColorant[6], TAG_BLUECOLORANT);
        InvertColorantArray(adColorant, adRevColorant);

        pBuffer += WriteNewLineObject(pBuffer, MatrixLMNTag);

        pBuffer += WriteObject(pBuffer, BeginArray);
        for (i = 0; i < 9; i++)
        {
            pBuffer += WriteDouble(pBuffer, adRevColorant[i]);
        }
        pBuffer += WriteObject(pBuffer, EndArray);

        //
        // /EncodeABC
        //

        pBuffer += WriteNewLineObject(pBuffer, EncodeABCTag);
        pBuffer += WriteObject(pBuffer, BeginArray);

        pBuffer += WriteObject(pBuffer, NewLine);
        pBuffer += SendCRDRevArray(pProfile, pBuffer, pRed, TAG_REDTRC, bBinary);

        pBuffer += WriteObject(pBuffer, NewLine);
        pBuffer += SendCRDRevArray(pProfile, pBuffer, pGreen, TAG_GREENTRC, bBinary);

        pBuffer += WriteObject(pBuffer, NewLine);
        pBuffer += SendCRDRevArray(pProfile, pBuffer, pBlue, TAG_BLUETRC, bBinary);

        pBuffer += WriteNewLineObject(pBuffer, EndArray);
        pBuffer += WriteObject(pBuffer, EndDict);  // End dictionary definition

        pBuffer += WriteNewLineObject(pBuffer, CRDEnd);

        MemFree (pRevCurve);
    }

    *pcbSize = (DWORD)(pBuffer - pStart);

    return TRUE;
}


DWORD
CreateCRDRevArray(
    PBYTE      pProfile,
    PBYTE      pBuffer,
    PCURVETYPE pData,
    PWORD      pRevCurve,
    DWORD      dwTag,
    BOOL       bBinary
    )
{
    DWORD i, nCount;
    PBYTE pStart, pLineStart;
    PWORD pCurve;

    pStart = pBuffer;
    pLineStart = pBuffer;

    nCount = FIX_ENDIAN(pData->nCount);
    if (nCount > 1)
    {
        pBuffer += WriteNewLineObject(pBuffer, Slash);
        pBuffer += WriteObject(pBuffer, InputArray);
        pBuffer += WriteInt(pBuffer, (INT) dwTag);

        pCurve = pRevCurve + (REVCURVE_RATIO * nCount);

        GetRevCurve (pData, pCurve, pRevCurve);

        if (!bBinary)  // Output ASCII DATA
        {
            pBuffer += WriteObject(pBuffer, BeginArray);
            for (i = 0; i < nCount * REVCURVE_RATIO; i++)
            {
                pBuffer += WriteInt(pBuffer, *pRevCurve);
                pRevCurve++;
                if (((DWORD) (pBuffer - pLineStart)) > MAX_LINELEN)
                {
                    pLineStart = pBuffer;
                    pBuffer += WriteObject(pBuffer, NewLine);
                }
            }
            pBuffer += WriteObject(pBuffer, EndArray);
        }
        else // Output BINARY DATA
        {
            pBuffer += WriteHNAToken(pBuffer, 149, nCount);
            pBuffer += WriteIntStringU2S_L(pBuffer, (PBYTE) pRevCurve, nCount);
        }
        pBuffer += WriteObject(pBuffer, DefOp);
    }

    return (DWORD)(pBuffer - pStart);
}


DWORD
SendCRDRevArray(
    PBYTE pProfile,
    PBYTE pBuffer,
    PCURVETYPE pData,
    DWORD dwTag,
    BOOL  bBinary
    )
{
    DWORD nCount;
    PBYTE pStart;
    PWORD pTable;

    pStart = pBuffer;

    pBuffer += WriteObject(pBuffer, BeginFunction);
    nCount = FIX_ENDIAN(pData->nCount);
    if (nCount != 0)
    {
        if (nCount == 1)            // Gamma supplied in ui16 format
        {
            pTable = pData->data;
            pBuffer += WriteInt(pBuffer, FIX_ENDIAN16(*pTable));
            pBuffer += WriteObject(pBuffer, DecodeA3Rev);
        }
        else
        {
            pBuffer += WriteObject(pBuffer, StartClip);
            pBuffer += WriteObject(pBuffer, InputArray);
            pBuffer += WriteInt(pBuffer, dwTag);

            if (!bBinary)       // Output ASCII CS
            {
                pBuffer += WriteObject(pBuffer, IndexArray);
            }
            else                // Output BINARY CS
            {
                pBuffer += WriteObject(pBuffer, IndexArray16b);
            }

            pBuffer += WriteObject(pBuffer, Scale16);
            pBuffer += WriteObject(pBuffer, EndClip);
        }
    }
    pBuffer += WriteObject(pBuffer, EndFunction);

    return (DWORD)(pBuffer - pStart);
}


BOOL
CreateColorantArray(
    PBYTE pProfile,
    double *lpArray,
    DWORD dwTag
    )
{
    PTAGDATA pTagData;
    PXYZTYPE pData;
    PFIX_16_16 pTable;
    DWORD i, dwIndex;
    BYTE buffer[1000];

    if (DoesCPTagExist(pProfile, dwTag, &dwIndex))
    {
        pTagData = (PTAGDATA)(pProfile + sizeof(PROFILEHEADER) + sizeof(DWORD) +
                   dwIndex * sizeof(TAGDATA));

        pData = (PXYZTYPE)(pProfile + FIX_ENDIAN(pTagData->dwOffset));

        pTable = pData->afxData;

        for (i = 0; i < 3; i++)
        {
            FIX_16_16 afxData = FIX_ENDIAN(*pTable);

            //
            // Convert Fix 16.16 to double.
            //

            *lpArray = ((double) afxData) / ((double) FIX_16_16_SCALE);

            pTable++; lpArray++;
        }

        return (TRUE);
    }

    return (FALSE);
}

#endif // !defined(KERNEL_MODE) || defined(USERMODE_DRIVER)

/***************************************************************************
*                               GetRevCurve
*  function:
*  returns:
*       BOOL        --  TRUE:  successful,
*                       FALSE: otherwise.
***************************************************************************/

BOOL
GetRevCurve(
    PCURVETYPE pData,
    PWORD      pInput,
    PWORD      pOutput
    )
{
    PBYTE       pTable;
    DWORD       nCount, dwStore, i, j;
    DWORD       dwBegin, dwEnd, dwTemp;

    nCount = FIX_ENDIAN(pData->nCount);
    pTable = (PBYTE)pData->data;

    for (i=0; i<nCount; i++)
    {
        pInput[i] = FIX_ENDIAN16(*((PWORD)pTable));
        pTable += sizeof(WORD);
    }

    j = nCount * REVCURVE_RATIO;
    for (i=0; i<j; i++)
    {
        dwStore = i * 65535 / (j - 1);
        pOutput[i] = (dwStore < 65535) ? (WORD) dwStore : 65535;
    }

    for (i=0; i<j; i++)
    {
        dwBegin = 0;
        dwEnd = nCount - 1;
        for (;;)
        {
            if ((dwEnd - dwBegin) <= 1)
                break;
            dwTemp = (dwEnd + dwBegin) / 2;
            if (pOutput[i] < pInput[dwTemp])
                dwEnd = dwTemp;
            else
                dwBegin = dwTemp;
        }
        if (pOutput[i] <= pInput[dwBegin])
        {
            dwStore = dwBegin;
        }
        else if (pOutput[i] >= pInput[dwEnd])
        {
            dwStore = dwEnd;
        }
        else
        {
            dwStore = (pInput[dwEnd] - pOutput[i]) / (pOutput[i] - pInput[dwBegin]);
            dwStore = (dwBegin * dwStore + dwEnd) / (dwStore + 1);
        }

        dwStore = dwStore * 65535 / (nCount - 1);
        pOutput[i] = (dwStore < 65535) ? (WORD) dwStore : 65535;
    }

    return TRUE;
}


BOOL
DoesCPTagExist(
    PBYTE     pProfile,
    DWORD     dwTag,
    PDWORD    pdwIndex
    )
{
    DWORD    i, dwCount;
    PTAGDATA pTagData;
    BOOL     bRc;

    //
    // Get count of tag items - it is right after the profile header
    //

    dwCount = FIX_ENDIAN(*((DWORD *)(pProfile + sizeof(PROFILEHEADER))));

    //
    // Tag data records follow the count.
    //

    pTagData = (PTAGDATA)(pProfile + sizeof(PROFILEHEADER) + sizeof(DWORD));

    //
    // Check if any of these records match the tag passed in.
    //

    bRc = FALSE;
    dwTag = FIX_ENDIAN(dwTag);      // to match tags in profile
    for (i=0; i<dwCount; i++)
    {
        if (pTagData->tagType == dwTag)
        {
            if (pdwIndex)
            {
                *pdwIndex = i;
            }

            bRc = TRUE;
            break;
        }
        pTagData++;                     // Next record
    }

    return bRc;
}


BOOL
DoesTRCAndColorantTagExist(
    PBYTE pProfile
    )
{
    if (DoesCPTagExist(pProfile,TAG_REDCOLORANT,NULL) &&
        DoesCPTagExist(pProfile,TAG_REDTRC,NULL) &&
        DoesCPTagExist(pProfile,TAG_GREENCOLORANT,NULL) &&
        DoesCPTagExist(pProfile,TAG_GREENTRC,NULL) &&
        DoesCPTagExist(pProfile,TAG_BLUECOLORANT,NULL) &&
        DoesCPTagExist(pProfile,TAG_BLUETRC,NULL))
    {
        return TRUE;
    }

    return FALSE;
}


BOOL
GetCPWhitePoint(
    PBYTE      pProfile,
    PFIX_16_16 pafxWP
    )
{
    pafxWP[0]  = FIX_ENDIAN(((PPROFILEHEADER)pProfile)->phIlluminant.ciexyzX);
    pafxWP[1]  = FIX_ENDIAN(((PPROFILEHEADER)pProfile)->phIlluminant.ciexyzY);
    pafxWP[2]  = FIX_ENDIAN(((PPROFILEHEADER)pProfile)->phIlluminant.ciexyzZ);

    return TRUE;
}

BOOL
GetCPMediaWhitePoint(
    PBYTE      pProfile,
    PFIX_16_16 pafxMediaWP
    )
{
    PTAGDATA pTagData;
    PDWORD   pTable;
    DWORD    dwIndex, i;

    if (DoesCPTagExist (pProfile, TAG_MEDIAWHITEPOINT, &dwIndex))
    {
        pTagData = (PTAGDATA)(pProfile + sizeof(PROFILEHEADER) + sizeof(DWORD) +
               dwIndex * sizeof(TAGDATA));

        //
        // Skip the first 2 DWORDs to get to the real data
        //

        pTable = (PDWORD)(pProfile + FIX_ENDIAN(pTagData->dwOffset)) + 2;

        for (i=0; i<3; i++)
        {
            pafxMediaWP[i] = FIX_ENDIAN(*pTable);
            pTable++;
        }

        return TRUE;
    }

    return FALSE;
}


BOOL
GetCPElementDataSize(
    PBYTE  pProfile,
    DWORD  dwIndex,
    PDWORD pcbSize)
{
    PTAGDATA pTagData;

    pTagData = (PTAGDATA)(pProfile + sizeof(PROFILEHEADER) + sizeof(DWORD) +
               dwIndex * sizeof(TAGDATA));

    //
    // Actual data Size of elements of type 'dataType' is 3 DWORDs less than the
    // total tag data size
    //

    *pcbSize = FIX_ENDIAN(pTagData->cbSize) - 3 * sizeof(DWORD);

    return TRUE;
}


BOOL
GetCPElementSize(
    PBYTE  pProfile,
    DWORD  dwIndex,
    PDWORD pcbSize)
{
    PTAGDATA pTagData;

    pTagData = (PTAGDATA)(pProfile + sizeof(PROFILEHEADER) + sizeof(DWORD) +
               dwIndex * sizeof(TAGDATA));

    *pcbSize = FIX_ENDIAN(pTagData->cbSize);

    return TRUE;
}


BOOL
GetCPElementDataType(
    PBYTE  pProfile,
    DWORD  dwIndex,
    PDWORD pdwDataType)
{
    PTAGDATA pTagData;
    PBYTE    pData;

    pTagData = (PTAGDATA)(pProfile + sizeof(PROFILEHEADER) + sizeof(DWORD) +
               dwIndex * sizeof(TAGDATA));

    pData = pProfile + FIX_ENDIAN(pTagData->dwOffset);

    *pdwDataType = FIX_ENDIAN(*((DWORD *)(pData + 2 * sizeof(DWORD))));

    return TRUE;
}


BOOL
GetCPElementData(
    PBYTE  pProfile,
    DWORD  dwIndex,
    PBYTE  pBuffer,
    PDWORD pdwSize
    )
{
    PTAGDATA pTagData;
    PBYTE    pData;

    pTagData = (PTAGDATA)(pProfile + sizeof(PROFILEHEADER) + sizeof(DWORD) +
               dwIndex * sizeof(TAGDATA));

    pData = pProfile + FIX_ENDIAN(pTagData->dwOffset);

    //
    // Actual data Size of elements of type 'dataType' is 3 DWORDs less than the
    // total tag data size
    //

    *pdwSize = FIX_ENDIAN(pTagData->cbSize) - 3 * sizeof(DWORD);

    if (pBuffer)
    {
        CopyMemory(pBuffer, (pData + 3*sizeof(DWORD)), *pdwSize);
    }

    return TRUE;
}


BOOL
GetTRCElementSize(
    PBYTE  pProfile,
    DWORD  dwTag,
    PDWORD pdwIndex,
    PDWORD pdwSize
    )
{
    DWORD dwDataType;

    if (!DoesCPTagExist(pProfile, dwTag, pdwIndex) ||
        !GetCPElementDataType(pProfile, *pdwIndex, &dwDataType) ||
        !(dwDataType != SIG_CURVE_TYPE) ||
        !GetCPElementSize(pProfile, *pdwIndex, pdwSize))
    {
        return FALSE;
    }

    return TRUE;
}


DWORD
Ascii85Encode(
    PBYTE pBuffer,
    DWORD dwDataSize,
    DWORD dwBufSize
    )
{
    // BUGBUG - To be done

#if 0
    PBYTE     pTempBuf, pPtr;
    DWORD     dwASCII85Size = 0;
    DWORD     dwBufSize = DataSize * 5 / 4 + sizeof(ASCII85DecodeBegin)+sizeof(ASCII85DecodeEnd) + 2048;

    if ((pTempBuf = (PBYTE)MemAlloc(dwBufSize)))
    {
        pPtr = pTempBuf;
        pPtr += WriteObject(pPtr,  NewLine);
        pPtr += WriteObject(pPtr,  ASCII85DecodeBegin);
        pPtr += WriteObject(pPtr,  NewLine);
        pPtr += WriteASCII85Cont(pPtr, dwBufSize, pBuffer, dwDataSize);
        pPtr += WriteObject(pPtr,  ASCII85DecodeEnd);
        dwAscii85Size = (DWORD)(pPtr - pTempBuf);
        lstrcpyn(pBuffer, pTempBuf, dwAscii85Size);

        MemFree(pTempBuf);
    }

    return dwAscii85Size;
#else
    return 0;
#endif
}

/***************************************************************************
*
*   Function to write the Homogeneous Number Array token into the buffer
*
***************************************************************************/

DWORD
WriteHNAToken(
    PBYTE pBuffer,
    BYTE  token,
    DWORD dwNum
    )
{
    *pBuffer++ = token;
    *pBuffer++ = 32;       // 16-bit fixed integer, high-order byte first
    *pBuffer++ = (BYTE)((dwNum & 0xFF00) >> 8);
    *pBuffer++ = (BYTE)(dwNum & 0x00FF);

    return 4;
}

/***************************************************************************
*
*   Function to convert 2-bytes unsigned integer to 2-bytes signed
*   integer(-32768) and write them to the buffer. High byte first.
*
***************************************************************************/

DWORD
WriteIntStringU2S(
    PBYTE pBuffer,
    PBYTE pData,
    DWORD dwNum
    )
{
    DWORD i, dwTemp;

    for (i=0; i<dwNum; i++)
    {
        dwTemp = FIX_ENDIAN16(*((PWORD)pData)) - 32768;
        *pBuffer++ = (BYTE)((dwTemp & 0xFF00) >> 8);
        *pBuffer++ = (BYTE)(dwTemp & 0x00FF);
        pData += sizeof(WORD);
    }

    return dwNum * 2;
}


/***************************************************************************
*
*   Function to convert 2-bytes unsigned integer to 2-bytes signed
*   integer(-32768) and write them to the buffer. Low-order byte first.
*
***************************************************************************/

DWORD
WriteIntStringU2S_L(
    PBYTE pBuffer,
    PBYTE pData,
    DWORD dwNum
    )
{
    DWORD i, dwTemp;

    for (i=0; i<dwNum; i++)
    {
        dwTemp = *((PWORD)pData) - 32768;
        *pBuffer++ = (BYTE)((dwTemp & 0xFF00) >> 8);
        *pBuffer++ = (BYTE)(dwTemp & 0x00FF);
        pData += sizeof(WORD);
    }

    return dwNum * 2;
}


/***************************************************************************
*
*   Function to put the chunk of memory as string of Hex
*
***************************************************************************/

DWORD
WriteHexBuffer(
    PBYTE pBuffer,
    PBYTE pData,
    PBYTE pLineStart,
    DWORD dwBytes
    )
{
    PBYTE  pStart = pBuffer;

    for ( ; dwBytes ; dwBytes-- )
    {
        WriteHex(pBuffer, *pData);
        pBuffer += 2;
        pData++;
        if (((DWORD)(pBuffer - pLineStart)) > MAX_LINELEN)
        {
            pLineStart = pBuffer;
            pBuffer += WriteObject(pBuffer,  NewLine);
        }
    }
    return( (DWORD)(pBuffer - pStart));
}

/***************************************************************************
*
*   Function to write the string token into the buffer
*
***************************************************************************/

DWORD
WriteStringToken(
    PBYTE pBuffer,
    BYTE  token,
    DWORD dwNum
    )
{
    *pBuffer++ = token;
    *pBuffer++ = (BYTE)((dwNum & 0xFF00) >> 8);
    *pBuffer++ = (BYTE)(dwNum & 0x00FF);

    return 3;
}

/***************************************************************************
*
*   Function to put the chunk of memory into buffer
*
***************************************************************************/

DWORD
WriteByteString(
    PBYTE pBuffer,
    PBYTE pData,
    DWORD dwBytes
    )
{
    DWORD  i;

    for (i=0; i<dwBytes; i++)
        *pBuffer++ = *pData++;

    return dwBytes;
}

/***************************************************************************
*
*   Function to put the chunk of memory into buffer
*
***************************************************************************/

DWORD
WriteInt2ByteString(
    PBYTE pBuffer,
    PBYTE pData,
    DWORD dwBytes
    )
{
    DWORD  i;

    for (i=0; i<dwBytes ; i++)
    {
        *pBuffer++ = (BYTE)(FIX_ENDIAN16(*((PWORD)pData))/256);
        pData += sizeof(WORD);
    }

    return dwBytes;
}

#ifndef KERNEL_MODE
DWORD
WriteFixed(
    PBYTE     pBuffer,
    FIX_16_16 fxNum
    )
{
    double dFloat = (double) ((long) fxNum) / (double) FIX_16_16_SCALE;

    return (WriteDouble(pBuffer,dFloat));
}
#else
DWORD
WriteFixed(
    PBYTE     pBuffer,
    FIX_16_16 fxNum
    )
{
    PBYTE pStart = pBuffer;
    DWORD i;

    //
    // Integer portion
    //

    #ifndef KERNEL_MODE
    pBuffer += wsprintfA(pBuffer, "%lu", fxNum >> FIX_16_16_SHIFT);
    #else
    pBuffer += OPSprintf(pBuffer, "%l", fxNum >> FIX_16_16_SHIFT);
    #endif

    //
    // Fractional part
    //

    fxNum &= 0xffff;
    if (fxNum != 0)
    {
        //
        // We output a maximum of 6 digits after the
        // decimal point
        //

        *pBuffer++ = '.';

        i = 0;
        while (fxNum && i++ < 6)
        {
            fxNum *= 10;
            *pBuffer++ = (BYTE)(fxNum >> FIX_16_16_SHIFT) + '0';  // quotient + '0'
            fxNum -= FLOOR(fxNum);          // remainder
        }
    }

    *pBuffer++ = ' ';

    return (DWORD) (pBuffer - pStart);
}
#endif

DWORD
WriteFixed2dot30(
    PBYTE pBuffer,
    DWORD fxNum
    )
{
    PBYTE pStart = pBuffer;
    DWORD i;

    //
    // Integer portion
    //

    #ifndef KERNEL_MODE
    pBuffer += wsprintfA(pBuffer, "%lu", fxNum >> 30);
    #else
    pBuffer += OPSprintf(pBuffer, "%l", fxNum >> 30);
    #endif

    //
    // Fractional part
    //

    fxNum &= 0x3fffffffL;
    if (fxNum != 0)
    {
        //
        // We output a maximum of 10 digits after the
        // decimal point
        //

        *pBuffer++ = '.';

        i = 0;
        while (fxNum && i++ < 10)
        {
            fxNum *= 10;
            *pBuffer++ = (BYTE)(fxNum >> 30) + '0';  // quotient + '0'
            fxNum -= ((fxNum >> 30) << 30);          // remainder
        }
    }

    *pBuffer++ = ' ';

    return (DWORD) (pBuffer - pStart);
}

#if !defined(KERNEL_MODE) || defined(USERMODE_DRIVER)

/***************************************************************************
*
*   Function to write the float into the buffer
*
***************************************************************************/

DWORD WriteDouble(PBYTE pBuffer, double dFloat)
{
    LONG   lFloat  = (LONG) floor(dFloat * 10000.0 + 0.5);
    double dFloat1 = lFloat  / 10000.0 ;
    double dInt    = floor(fabs(dFloat1));
    double dFract =  fabs(dFloat1) - dInt ;
    char   cSign  = ' ' ;

    if (dFloat1 < 0)
    {
        cSign = '-' ;
    }

    return (wsprintfA(pBuffer, (LPSTR) "%c%d.%0.4lu ",
                        cSign, (WORD) dInt , (DWORD) (dFract * 10000.0)));
}

#endif // !defined(KERNEL_MODE) || defined(USERMODE_DRIVER)

DWORD WriteNewLineObject(
    PBYTE       pBuffer,
    const char *pData)
{
    PBYTE pStart = pBuffer;

    pBuffer += WriteObject(pBuffer, NewLine);
    pBuffer += WriteObject(pBuffer, pData);

    return (DWORD)(pBuffer - pStart);
}


DWORD
SendCRDBWPoint(
    PBYTE      pBuffer,
    PFIX_16_16 pafxWP
    )
{
    PBYTE pStart = pBuffer;
    int   i;

    //
    // /BlackPoint
    //

    pBuffer += WriteObject(pBuffer, NewLine);
    pBuffer += WriteObject(pBuffer, BlackPointTag);
    pBuffer += WriteObject(pBuffer, BlackPoint);

    //
    // /WhitePoint
    //

    pBuffer += WriteObject(pBuffer, NewLine);
    pBuffer += WriteObject(pBuffer, WhitePointTag);
    pBuffer += WriteObject(pBuffer, BeginArray);
    for (i=0; i<3; i++)
    {
        pBuffer += WriteFixed(pBuffer, pafxWP[i]);
    }
    pBuffer += WriteObject(pBuffer, EndArray);

    return (DWORD)(pBuffer - pStart);
}


DWORD
SendCRDPQR(
    PBYTE      pBuffer,
    DWORD      dwIntent,
    PFIX_16_16 pafxWP
    )
{
    PBYTE pStart = pBuffer;
    int   i;

    if (dwIntent != INTENT_ABSOLUTE_COLORIMETRIC)
    {
        //
        // /RangePQR
        //

        pBuffer += WriteNewLineObject(pBuffer, RangePQRTag);
        pBuffer += WriteObject(pBuffer, RangePQR);

        //
        // /MatrixPQR
        //

        pBuffer += WriteNewLineObject(pBuffer, MatrixPQRTag);
        pBuffer += WriteObject(pBuffer, MatrixPQR);
    }
    else
    {
        //
        // /RangePQR
        //

        pBuffer += WriteNewLineObject(pBuffer, RangePQRTag);
        pBuffer += WriteObject(pBuffer, BeginArray);
        for (i=0; i<3; i++)
        {
            pBuffer += WriteFixed(pBuffer, 0);
            pBuffer += WriteFixed(pBuffer, pafxWP[i]);
        }
        pBuffer += WriteObject(pBuffer, EndArray);

        //
        // /MatrixPQR
        //

        pBuffer += WriteNewLineObject(pBuffer, MatrixPQRTag);
        pBuffer += WriteObject(pBuffer, Identity);
    }

    //
    // /TransformPQR
    //

    pBuffer += WriteNewLineObject(pBuffer, TransformPQRTag);
    pBuffer += WriteObject(pBuffer, BeginArray);
    for (i=0; i<3; i++)
    {
        pBuffer += WriteObject(pBuffer, BeginFunction);
        pBuffer += WriteObject(pBuffer,
            (dwIntent != INTENT_ABSOLUTE_COLORIMETRIC) ? TransformPQR[i] : NullOp);
        pBuffer += WriteObject(pBuffer, EndFunction);
    }
    pBuffer += WriteObject(pBuffer, EndArray);

    return (DWORD)(pBuffer - pStart);
}


DWORD
SendCRDLMN(
    PBYTE      pBuffer,
    DWORD      dwIntent,
    PFIX_16_16 pafxIlluminantWP,
    PFIX_16_16 pafxMediaWP,
    DWORD      dwPCS
    )
{
    PBYTE pStart = pBuffer;
    DWORD i, j;

    //
    // /MatrixLMN
    //

    if (dwIntent == INTENT_ABSOLUTE_COLORIMETRIC)
    {
        pBuffer += WriteNewLineObject(pBuffer, MatrixLMNTag);

        pBuffer += WriteObject(pBuffer, BeginArray);
        for (i=0; i<3; i++)
        {
            for (j=0; j<3; j++)
                pBuffer += WriteFixed(pBuffer,
                    (i == j) ? FIX_DIV(pafxIlluminantWP[i], pafxMediaWP[i]) : 0);
        }
        pBuffer += WriteObject(pBuffer, EndArray);
    }

    //
    // /RangeLMN
    //

    pBuffer += WriteNewLineObject(pBuffer, RangeLMNTag);
    if (dwPCS == SPACE_XYZ)
    {
        pBuffer += WriteObject(pBuffer, BeginArray);
        for (i=0; i<3; i++)
        {
            pBuffer += WriteFixed(pBuffer, 0);
            pBuffer += WriteFixed(pBuffer, pafxIlluminantWP[i]);
        }
        pBuffer += WriteObject(pBuffer, EndArray);
    }
    else
    {
        pBuffer += WriteObject(pBuffer, RangeLMNLab);
    }

    //
    // /EncodeLMN
    //

    pBuffer += WriteNewLineObject(pBuffer, EncodeLMNTag);
    pBuffer += WriteObject(pBuffer, BeginArray);
    for (i=0; i<3; i++)
    {
        pBuffer += WriteObject(pBuffer, BeginFunction);
        if (dwPCS != SPACE_XYZ)
        {
            pBuffer += WriteFixed(pBuffer, pafxIlluminantWP[i]);
            pBuffer += WriteObject(pBuffer, DivOp);
            pBuffer += WriteObject(pBuffer, EncodeLMNLab);
        }
        pBuffer += WriteObject(pBuffer, EndFunction);
    }
    pBuffer += WriteObject(pBuffer, EndArray);

    return (DWORD)(pBuffer - pStart);
}


DWORD
SendCRDABC(
    PBYTE       pBuffer,
    PBYTE       pPublicArrayName,
    DWORD       dwPCS,
    DWORD       nInputCh,
    PBYTE       pLut,
    PFIX_16_16  e,
    DWORD       dwLutSig,
    BOOL        bBinary
    )
{
    PBYTE      pLineStart, pStart = pBuffer;
    PBYTE      pTable;
    DWORD      i, j;
    FIX_16_16  fxTempMatrixABC[9];

    //
    // /RangeABC
    //

    pBuffer += WriteNewLineObject(pBuffer, RangeABCTag);
    pBuffer += WriteObject(pBuffer, RangeABC);

    //
    // /MatrixABC
    //

    pBuffer += WriteNewLineObject(pBuffer, MatrixABCTag);
    if (dwPCS == SPACE_XYZ)
    {
        pBuffer += WriteObject(pBuffer, BeginArray);
        if (e)
        {
            for (i=0; i<3; i++)
            {
                for (j=0; j<3; j++)
                {
                    pBuffer += WriteFixed(pBuffer, e[i + j * 3]);
                }
            }
        }
        else
        {
            if (dwLutSig == LUT8_TYPE)
            {
                pTable = (PBYTE) &((PLUT8TYPE)pLut)->e00;
            }
            else
            {
                pTable = (PBYTE) &((PLUT16TYPE)pLut)->e00;
            }

            for (i=0; i<9; i++)
            {
                fxTempMatrixABC[i] = FIX_DIV(FIX_ENDIAN(*((PDWORD)pTable)), CIEXYZRange);
                pTable += sizeof(DWORD);
            }
            for (i=0; i<3; i++)
            {
                for (j=0; j<3; j++)
                {
                    pBuffer += WriteFixed(pBuffer, fxTempMatrixABC[i + j * 3]);
                }
            }
        }
        pBuffer += WriteObject(pBuffer, EndArray);
    }
    else
    {
        pBuffer += WriteObject(pBuffer, MatrixABCLabCRD);
    }

    //
    // /EncodeABC
    //

    if (nInputCh == 0)
    {
        return (DWORD)(pBuffer - pStart);
    }

    pLineStart = pBuffer;
    pBuffer += WriteNewLineObject(pBuffer, EncodeABCTag);
    pBuffer += WriteObject(pBuffer, BeginArray);
    for (i=0; i<nInputCh; i++)
    {
        pLineStart = pBuffer;

        pBuffer += WriteNewLineObject(pBuffer, BeginFunction);
        if (dwPCS == SPACE_Lab)
        {
            pBuffer += WriteObject(pBuffer, (i == 0) ? EncodeABCLab1 : EncodeABCLab2);
        }

        pBuffer += WriteObject(pBuffer, StartClip);
        if (e)
            pBuffer += WriteObject(pBuffer, PreViewInArray);
        else
            pBuffer += WriteObject(pBuffer, InputArray);

        pBuffer += WriteObject(pBuffer, pPublicArrayName);
        pBuffer += WriteInt(pBuffer, i);

        if (!bBinary)              // Output ASCII CRD
        {
            pBuffer += WriteNewLineObject(pBuffer, IndexArray);
        }
        else
        {                               // Output BINARY CRD
            if (dwLutSig == LUT8_TYPE)
            {
                pBuffer += WriteObject(pBuffer, IndexArray);
            }
            else
            {
                pBuffer += WriteObject(pBuffer, IndexArray16b);
            }
        }

        pBuffer += WriteObject(pBuffer, (dwLutSig == LUT8_TYPE) ?
                         Scale8 : Scale16);
        pBuffer += WriteObject(pBuffer, EndClip);
        pBuffer += WriteObject(pBuffer, EndFunction);
    }
    pBuffer += WriteObject(pBuffer, EndArray);

    return (DWORD)(pBuffer - pStart);
}


DWORD
SendCRDOutputTable(
    PBYTE pBuffer,
    PBYTE pPublicArrayName,
    DWORD nOutputCh,
    DWORD dwLutSig,
    BOOL  bHost,
    BOOL  bBinary
    )
{
    PBYTE pStart = pBuffer;
    DWORD i;

    for (i=0; i<nOutputCh; i++)
    {
        pBuffer += WriteNewLineObject(pBuffer, BeginFunction);
        pBuffer += WriteObject(pBuffer, Clip01);
        if (bHost)
            pBuffer += WriteObject(pBuffer, PreViewOutArray);
        else
            pBuffer += WriteObject(pBuffer, OutputArray);

        pBuffer += WriteObject(pBuffer, pPublicArrayName);
        pBuffer += WriteInt(pBuffer, i);

        if (! bBinary)
        {
            pBuffer += WriteObject(pBuffer, NewLine);
            if (dwLutSig == LUT8_TYPE)
            {
                pBuffer += WriteObject(pBuffer, TFunction8);
            }
            else
            {
                pBuffer += WriteObject(pBuffer, IndexArray);
                pBuffer += WriteObject(pBuffer, Scale16);
            }
        }
        else
        {
            if (dwLutSig == LUT8_TYPE)
            {
                pBuffer += WriteObject(pBuffer, TFunction8);
            }
            else
            {
                pBuffer += WriteObject(pBuffer, IndexArray16b);
                pBuffer += WriteObject(pBuffer, Scale16);
            }
        }

        pBuffer += WriteObject(pBuffer, EndFunction);
    }

    return (DWORD)(pBuffer - pStart);
}


VOID
GetCLUTInfo(
    DWORD  dwLutSig,
    PBYTE  pLut,
    PDWORD pnInputCh,
    PDWORD pnOutputCh,
    PDWORD pnGrids,
    PDWORD pnInputTable,
    PDWORD pnOutputTable,
    PDWORD pdwSize
    )
{
    if (dwLutSig == LUT8_TYPE)
    {
        *pnInputCh = ((PLUT8TYPE)pLut)->nInputChannels;
        *pnOutputCh = ((PLUT8TYPE)pLut)->nOutputChannels;
        *pnGrids = ((PLUT8TYPE)pLut)->nClutPoints;
        *pnInputTable = 256L;
        *pnOutputTable = 256L;
        if (pdwSize)
            *pdwSize = 1;
    }
    else
    {
        *pnInputCh = ((PLUT16TYPE)pLut)->nInputChannels;
        *pnOutputCh = ((PLUT16TYPE)pLut)->nOutputChannels;
        *pnGrids =  ((PLUT16TYPE)pLut)->nClutPoints;
        *pnInputTable = FIX_ENDIAN16(((PLUT16TYPE)pLut)->wInputEntries);
        *pnOutputTable = FIX_ENDIAN16(((PLUT16TYPE)pLut)->wOutputEntries);
        if (pdwSize)
            *pdwSize = 2;
    }

    return;
}

DWORD
EnableGlobalDict(
    PBYTE pBuffer
    )
{
    PBYTE pStart = pBuffer;

    pBuffer += WriteNewLineObject(pBuffer, CurrentGlobalOp);
    pBuffer += WriteObject(pBuffer, TrueOp);
    pBuffer += WriteObject(pBuffer, SetGlobalOp);

    return (DWORD)(pBuffer - pStart);
}

DWORD
BeginGlobalDict(
    PBYTE pBuffer
    )
{
    PBYTE pStart = pBuffer;

    pBuffer += WriteNewLineObject(pBuffer, GlobalDictOp);
    pBuffer += WriteObject(pBuffer, BeginOp);

    return (DWORD)(pBuffer - pStart);
}

DWORD
EndGlobalDict(
    PBYTE pBuffer
    )
{
    PBYTE pStart = pBuffer;

    pBuffer += WriteNewLineObject(pBuffer, EndOp);
    pBuffer += WriteObject(pBuffer, SetGlobalOp);

    return (DWORD)(pBuffer - pStart);
}

DWORD
SendCSABWPoint(
    PBYTE      pBuffer,
    DWORD      dwIntent,
    PFIX_16_16 pafxIlluminantWP,
    PFIX_16_16 pafxMediaWP
    )
{
    PBYTE pStart = pBuffer;
    int   i;

    //
    // /BlackPoint
    //

    pBuffer += WriteNewLineObject(pBuffer, BlackPointTag);
    pBuffer += WriteObject(pBuffer, BlackPoint);

    //
    // /WhitePoint
    //

    pBuffer += WriteNewLineObject(pBuffer, WhitePointTag);
    pBuffer += WriteObject(pBuffer, BeginArray);
    for (i=0; i<3; i++)
    {
        if (dwIntent == INTENT_ABSOLUTE_COLORIMETRIC)
        {
            pBuffer += WriteFixed(pBuffer, pafxMediaWP[i]);
        }
        else
        {
            pBuffer += WriteFixed(pBuffer, pafxIlluminantWP[i]);
        }
    }
    pBuffer += WriteObject(pBuffer, EndArray);

    return (DWORD)(pBuffer - pStart);
}


VOID
GetMediaWP(
    PBYTE      pProfile,
    DWORD      dwIntent,
    PFIX_16_16 pafxIlluminantWP,
    PFIX_16_16 pafxMediaWP
    )
{
    if (dwIntent == INTENT_ABSOLUTE_COLORIMETRIC)
    {
        if (! GetCPMediaWhitePoint(pProfile, pafxMediaWP))
        {
            pafxMediaWP[0] = pafxIlluminantWP[0];
            pafxMediaWP[1] = pafxIlluminantWP[1];
            pafxMediaWP[2] = pafxIlluminantWP[2];
        }
    }

    return;
}


BOOL
GetCRDInputOutputArraySize(
    PBYTE  pProfile,
    DWORD  dwIntent,
    PDWORD pdwInTblSize,
    PDWORD pdwOutTblSize,
    PDWORD pdwTag,
    PDWORD pnGrids
    )
{
    PTAGDATA   pTagData;
    PLUT16TYPE pLut;
    DWORD      dwIndex, dwLutSig;
    DWORD      nInputEntries, nOutputEntries;
    DWORD      nInputCh, nOutputCh, nGrids;
    BOOL       bRet;

    //
    // Make sure required tags exist
    //

    switch (dwIntent)
    {
    case INTENT_PERCEPTUAL:
        *pdwTag = TAG_BToA0;
        break;

    case INTENT_RELATIVE_COLORIMETRIC:
    case INTENT_ABSOLUTE_COLORIMETRIC:
        *pdwTag = TAG_BToA1;
        break;

    case INTENT_SATURATION:
        *pdwTag = TAG_BToA2;
        break;

    default:
        WARNING((__TEXT("Invalid intent passed to GetCRDInputOutputArraySize: %d\n"), dwIntent));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (DoesCPTagExist(pProfile, *pdwTag, &dwIndex))
    {
        pTagData = (PTAGDATA)(pProfile + sizeof(PROFILEHEADER) + sizeof(DWORD) +
                   dwIndex * sizeof(TAGDATA));

        pLut = (PLUT16TYPE)(pProfile + FIX_ENDIAN(pTagData->dwOffset));

        dwLutSig = FIX_ENDIAN(pLut->dwSignature);

        if ((dwLutSig != LUT8_TYPE) && (dwLutSig != LUT16_TYPE))
        {
            WARNING((__TEXT("Invalid Lut type - unable to create proofing CRD\n")));
            SetLastError(ERROR_INVALID_PROFILE);
            return FALSE;
        }

        (void)GetCLUTInfo(dwLutSig, (PBYTE)pLut, &nInputCh, &nOutputCh,
                &nGrids, &nInputEntries, &nOutputEntries, NULL);

        if (pdwInTblSize)
        {
            if (nInputCh != 3)
            {
                return FALSE;
            }

            *pdwInTblSize = nInputCh * nInputEntries * 6;  // Number of INT bytes
            *pnGrids = nGrids;
        }

        if (pdwOutTblSize)
        {
            if ((nOutputCh != 3) && (nOutputCh != 4))
            {
                return FALSE;
            }

            *pdwOutTblSize = nOutputCh * nOutputEntries * 6; // Number of INT bytes
            *pnGrids = nGrids;
        }

        return TRUE;
    }
    else
    {
        //
        // Matrix icc profile.
        //

        *pnGrids = 2;

        if (pdwInTblSize)
        {
            bRet = GetHostCSA(pProfile, NULL, pdwInTblSize,
                              dwIntent, TYPE_CIEBASEDDEF);
            *pdwInTblSize = *pdwInTblSize * 3;
        }

        if (bRet && pdwOutTblSize)
        {
            bRet = GetHostCSA(pProfile, NULL, pdwInTblSize,
                              dwIntent, TYPE_CIEBASEDDEF);
            *pdwOutTblSize = *pdwOutTblSize * 3;
        }

        return bRet;
    }
}

#if !defined(KERNEL_MODE) || defined(USERMODE_DRIVER)

DWORD
CreateHostLutCRD(
    PBYTE  pProfile,
    DWORD  dwIndex,
    PBYTE  pBuffer,
    DWORD  dwIntent
    )
{
    PLUT16TYPE pLut;
    PHOSTCLUT  pHostClut;
    PTAGDATA   pTagData;
    PBYTE      pTable;
    DWORD      nInputCh, nOutputCh, nGrids;
    DWORD      nInputEntries, nOutputEntries, nNumbers;
    DWORD      dwPCS, dwLutSig;
    DWORD      dwSize, i, j;
    PBYTE      pStart = pBuffer;

    //
    // Check if we can generate the CSA
    // Required  tags is AToBi, where i is the rendering intent
    //

    dwPCS = GetCPConnSpace(pProfile);

    pTagData = (PTAGDATA)(pProfile + sizeof(PROFILEHEADER) + sizeof(DWORD) +
               dwIndex * sizeof(TAGDATA));

    pLut = (PLUT16TYPE)(pProfile + FIX_ENDIAN(pTagData->dwOffset));

    dwLutSig = FIX_ENDIAN(pLut->dwSignature);

    if ((dwLutSig != LUT8_TYPE) && (dwLutSig != LUT16_TYPE))
    {
        WARNING((__TEXT("Invalid profile - unable to create Lut CRD\n")));
        SetLastError(ERROR_INVALID_PROFILE);
        return 0;
    }

    (void)GetCLUTInfo(dwLutSig, (PBYTE)pLut, &nInputCh, &nOutputCh,
            &nGrids, &nInputEntries, &nOutputEntries, &i);

    if (((nOutputCh != 3) && (nOutputCh != 4)) ||  (nInputCh != 3))
    {
        return 0;
    }

    if (pBuffer == NULL)
    {
        //
        // Return size
        //

        dwSize = nInputCh * nInputEntries * i    +  // Input table 8/16-bits
            nOutputCh * nOutputEntries * i       +  // Output table 8/16-bits
            nOutputCh * nGrids * nGrids * nGrids +  // CLUT 8-bits only
            sizeof(HOSTCLUT)                     +  // Data structure
            2048;                                   // Other PS stuff

        //
        // Add space for new line.
        //

        dwSize += (((dwSize) / MAX_LINELEN) + 1) * STRLEN(NewLine);

        return dwSize;
    }
	
    pHostClut = (PHOSTCLUT)pBuffer;
    pBuffer += sizeof(HOSTCLUT);
    pHostClut->wSize = sizeof(HOSTCLUT);
    pHostClut->wDataType = DATATYPE_LUT;
    pHostClut->dwPCS = dwPCS;
    pHostClut->dwIntent = dwIntent;
    pHostClut->nLutBits = (dwLutSig == LUT8_TYPE) ? 8 : 16;

    (void)GetCPWhitePoint(pProfile, pHostClut->afxIlluminantWP);

    //
    // Support absolute whitePoint
    //

    if (!GetCPMediaWhitePoint(pProfile, pHostClut->afxMediaWP))
    {
        pHostClut->afxMediaWP[0] = pHostClut->afxIlluminantWP[0];
        pHostClut->afxMediaWP[1] = pHostClut->afxIlluminantWP[1];
        pHostClut->afxMediaWP[2] = pHostClut->afxIlluminantWP[2];
    }

    pHostClut->nInputCh = (BYTE)nInputCh;
    pHostClut->nOutputCh = (BYTE)nOutputCh;
    pHostClut->nClutPoints = (BYTE)nGrids;
    pHostClut->nInputEntries = (WORD)nInputEntries;
    pHostClut->nOutputEntries = (WORD)nOutputEntries;

    //
    // Input array
    //

    pBuffer += CreateHostInputOutputArray(
                        pBuffer,
                        pHostClut->inputArray,
                        nInputCh,
                        nInputEntries,
                        0,
                        dwLutSig,
                        (PBYTE)pLut);

    //
    // The offset to the position of output array.
    //

    i = nInputEntries * nInputCh +
        nGrids * nGrids * nGrids * nOutputCh;

    //
    // Output array
    //

    pBuffer += CreateHostInputOutputArray(
                        pBuffer,
                        pHostClut->outputArray,
                        nOutputCh,
                        nOutputEntries,
                        i,
                        dwLutSig,
                        (PBYTE)pLut);

    //
    // Matrix
    //

    if (dwPCS == SPACE_XYZ)
    {
        if (dwLutSig == LUT8_TYPE)
        {
            pTable = (PBYTE) &((PLUT8TYPE)pLut)->e00;
        } else
        {
            pTable = (PBYTE) &((PLUT16TYPE)pLut)->e00;
        }

        for (i=0; i<9; i++)
        {
            pHostClut->e[i] = FIX_DIV(FIX_ENDIAN(*((PDWORD)pTable)), CIEXYZRange);
            pTable += sizeof(DWORD);
        }
    }

    //
    // RenderTable
    //

    nNumbers = nGrids * nGrids * nOutputCh;
    pHostClut->clut = pBuffer;

    for (i=0; i<nGrids; i++)        // Na strings should be sent
    {
        if (dwLutSig == LUT8_TYPE)
        {
            pTable = (PBYTE)(((PLUT8TYPE)pLut)->data) +
                nInputEntries * nInputCh +
                nNumbers * i;
        }
        else
        {
            pTable = (PBYTE)(((PLUT16TYPE)pLut)->data) +
                2 * nInputEntries * nInputCh +
                2 * nNumbers * i;
        }

        if (dwLutSig == LUT8_TYPE)
        {
            CopyMemory(pBuffer, pTable, nNumbers);
            pBuffer += nNumbers;
        }
        else
        {
            for (j=0; j<nNumbers; j++)
            {
                *pBuffer++ = (BYTE)(FIX_ENDIAN16(*((PWORD)pTable)) / 256);
                pTable += sizeof(WORD);
            }
        }
    }

    return (DWORD)(pBuffer - pStart);
}


DWORD
CreateHostMatrixCSAorCRD(
    PBYTE pProfile,
    PBYTE pBuffer,
    PDWORD pcbSize,
    DWORD dwIntent,
    BOOL bCSA
    )
{
    PTAGDATA   pTagData;
    DWORD      dwRedTRCIndex, dwGreenTRCIndex, dwBlueTRCIndex;
    DWORD      dwRedCount, dwGreenCount, dwBlueCount;
    PCURVETYPE pRed, pGreen, pBlue;
    PHOSTCLUT  pHostClut;
    PBYTE      pStart = pBuffer;
    DWORD      i, dwSize;
    double     adArray[9], adRevArray[9], adTemp[9];

    //
    // Get each TRC index for Red, Green and Blue.
    //

    if (!DoesCPTagExist(pProfile, TAG_REDTRC, &dwRedTRCIndex) ||
        !DoesCPTagExist(pProfile, TAG_GREENTRC, &dwGreenTRCIndex) ||
        !DoesCPTagExist(pProfile, TAG_BLUETRC, &dwBlueTRCIndex))
    {
        return FALSE;
    }

    //
    // Get CURVETYPE data for each Red, Green and Blue
    //

    pTagData = (PTAGDATA)(pProfile + sizeof(PROFILEHEADER) + sizeof(DWORD) +
               dwRedTRCIndex * sizeof(TAGDATA));

    pRed = (PCURVETYPE)(pProfile + FIX_ENDIAN(pTagData->dwOffset));

    pTagData = (PTAGDATA)(pProfile + sizeof(PROFILEHEADER) + sizeof(DWORD) +
               dwGreenTRCIndex * sizeof(TAGDATA));

    pGreen = (PCURVETYPE)(pProfile + FIX_ENDIAN(pTagData->dwOffset));

    pTagData = (PTAGDATA)(pProfile + sizeof(PROFILEHEADER) + sizeof(DWORD) +
               dwBlueTRCIndex * sizeof(TAGDATA));

    pBlue = (PCURVETYPE)(pProfile + FIX_ENDIAN(pTagData->dwOffset));

    //
    // Get curve count for each Red, Green and Blue.
    //

    dwRedCount   = FIX_ENDIAN(pRed->nCount);
    dwGreenCount = FIX_ENDIAN(pGreen->nCount);
    dwBlueCount  = FIX_ENDIAN(pBlue->nCount);

    //
    // Estimate the memory size required to hold CRD
    //

    dwSize = (dwRedCount + dwGreenCount + dwBlueCount) * 2 +
             sizeof(HOSTCLUT) + 2048;   // data structure + extra safe space

    //
    // Add space for new line.
    //

    dwSize += (((dwSize) / MAX_LINELEN) + 1) * STRLEN(NewLine);

    if (pBuffer == NULL)     // This is a size request
    {
        *pcbSize = dwSize;
        return TRUE;
    }

    //
    // Check buffer size.
    //

    if (*pcbSize < dwSize)
    {
        WARNING((__TEXT("Buffer too small to get Host Matrix CSA/CRD\n")));
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    pHostClut = (PHOSTCLUT)pBuffer;
    pBuffer += sizeof(HOSTCLUT);
    pHostClut->wSize = sizeof(HOSTCLUT);
    pHostClut->wDataType = DATATYPE_MATRIX;
    pHostClut->dwPCS = SPACE_XYZ;
    pHostClut->dwIntent = dwIntent;
    pHostClut->nClutPoints = 2;

    (void)GetCPWhitePoint(pProfile, pHostClut->afxIlluminantWP);

    if (bCSA)
    {
        pHostClut->nInputEntries = (USHORT) dwRedCount;
        pHostClut->nInputCh      = 3;

        pBuffer += CreateHostTRCInputTable(pBuffer, pHostClut,
                                           pRed, pGreen, pBlue);
    }
    else
    {
        pHostClut->nOutputEntries = (USHORT) dwRedCount;
        pHostClut->nOutputCh = 3;

        pBuffer += CreateHostRevTRCInputTable(pBuffer, pHostClut,
                                              pRed, pGreen, pBlue);
    }

    if (!CreateColorantArray(pProfile, &adTemp[0], TAG_REDCOLORANT) ||
        !CreateColorantArray(pProfile, &adTemp[3], TAG_GREENCOLORANT) ||
        !CreateColorantArray(pProfile, &adTemp[6], TAG_BLUECOLORANT))
    {
        WARNING((__TEXT("Fail to create colorant array for Host Matrix CSA/CRD\n")));
        return FALSE;
    }

    for (i = 0; i < 9; i++)
    {
        adArray[i] = adTemp[i/8*8 + i*3%8];
    }

    if (bCSA)
    {
        for (i = 0; i < 9; i++)
        {
            //
            // Convert double to Fix 16.16
            //

            pHostClut->e[i] = (FIX_16_16)(adArray[i] * (double)FIX_16_16_SCALE);
        }
    }
    else
    {
        InvertColorantArray(adArray, adRevArray);
        for (i = 0; i < 9; i++)
        {
            //
            // Convert double to Fix 16.16
            //

            pHostClut->e[i] = (FIX_16_16)(adRevArray[i] * (double)FIX_16_16_SCALE);
        }
    }

    *pcbSize = (DWORD)(pBuffer - pStart);

    return TRUE;
}


DWORD
CreateHostTRCInputTable(
    PBYTE pBuffer,
    PHOSTCLUT pHostClut,
    PCURVETYPE pRed,
    PCURVETYPE pGreen,
    PCURVETYPE pBlue
    )
{
    DWORD i;
    PWORD pBuffer16 = (PWORD) pBuffer;
    PWORD pTable;

    //
    // Red
    //

    pHostClut->inputArray[0] = (PBYTE) pBuffer16;
    pTable = pRed->data;
    for (i = 0; i < pHostClut->nInputEntries; i++)
    {
        *pBuffer16++ = FIX_ENDIAN16(*pTable);
        pTable++;
    }

    //
    // Green
    //

    pHostClut->inputArray[1] = (PBYTE) pBuffer16;
    pTable = pGreen->data;
    for (i = 0; i < pHostClut->nInputEntries; i++)
    {
        *pBuffer16++ = FIX_ENDIAN16(*pTable);
        pTable++;
    }

    //
    // Blue
    //

    pHostClut->inputArray[2] = (PBYTE) pBuffer16;
    pTable = pBlue->data;
    for (i = 0; i < pHostClut->nInputEntries; i++)
    {
        *pBuffer16++ = FIX_ENDIAN16(*pTable);
        pTable++;
    }

    return (DWORD)((PBYTE)pBuffer16 - pBuffer);
}

DWORD
CreateHostRevTRCInputTable(
    PBYTE pBuffer,
    PHOSTCLUT pHostClut,
    PCURVETYPE pRed,
    PCURVETYPE pGreen,
    PCURVETYPE pBlue
    )
{
    PWORD pTemp = MemAlloc(pHostClut->nOutputEntries * (REVCURVE_RATIO + 1) * 2);

    if (! pTemp)
    {
        return 0;
    }

    //
    // Red
    //

    pHostClut->outputArray[0] = pBuffer;
    GetRevCurve(pRed, pTemp, (PWORD) pHostClut->outputArray[0]);

    //
    // Green
    //

    pHostClut->outputArray[1] = pHostClut->outputArray[0] +
                                2 * REVCURVE_RATIO * pHostClut->nOutputEntries;
    GetRevCurve(pGreen, pTemp, (PWORD) pHostClut->outputArray[1]);

    //
    // Blue
    //

    pHostClut->outputArray[2] = pHostClut->outputArray[1] +
                                2 * REVCURVE_RATIO * pHostClut->nOutputEntries;
    GetRevCurve(pBlue, pTemp, (PWORD) pHostClut->outputArray[2]);

    MemFree(pTemp);

    return (DWORD)(2 * REVCURVE_RATIO * pHostClut->nOutputEntries * 3);
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
*       lpcbSize    -- 	size of memory block.
*
*  returns:
*       SINT        --  !=0 if the function was successful,
*                         0 otherwise.
*                       Returns number of bytes required/transferred
***************************************************************************/

BOOL
GetHostColorRenderingDictionary(
    PBYTE  pProfile,
    DWORD  dwIntent,
    PBYTE  pBuffer,
    PDWORD pdwSize
    )
{
    DWORD dwBToATag, dwIndex;

    switch (dwIntent)
    {
    case INTENT_PERCEPTUAL:
        dwBToATag = TAG_BToA0;
        break;

    case INTENT_RELATIVE_COLORIMETRIC:
    case INTENT_ABSOLUTE_COLORIMETRIC:
        dwBToATag = TAG_BToA1;
        break;

    case INTENT_SATURATION:
        dwBToATag = TAG_BToA2;
        break;

    default:
        *pdwSize = 0;
        return FALSE;
    }

    if (DoesCPTagExist(pProfile, dwBToATag, &dwIndex))
    {
        *pdwSize = CreateHostLutCRD(pProfile, dwIndex, pBuffer, dwIntent);

        return *pdwSize > 0;
    }
    else if (DoesTRCAndColorantTagExist(pProfile))
    {
        return CreateHostMatrixCSAorCRD(pProfile, pBuffer, pdwSize, dwIntent, FALSE);
    }
    else
    {
        return FALSE;
    }
}


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

DWORD
CreateHostInputOutputArray(
    PBYTE  pBuffer,
    PBYTE  *ppArray,
    DWORD  nNumChan,
    DWORD  nTableSize,
    DWORD  dwOffset,
    DWORD  dwLutSig,
    PBYTE  pLut
    )
{
    PBYTE   pStart = pBuffer;
    PBYTE   pTable;
    DWORD   i, j;

    for (i=0; i<nNumChan; i++)
    {
        ppArray[i] = pBuffer;

        if (dwLutSig == LUT8_TYPE)
        {
            pTable = (PBYTE) (((PLUT8TYPE)pLut)->data) +
                dwOffset + nTableSize * i;

            CopyMemory(pBuffer, pTable, nTableSize);

            pBuffer += nTableSize;
        }
        else
        {
            pTable = (PBYTE) (((PLUT16TYPE)pLut)->data) +
                2 * dwOffset +
                2 * nTableSize * i;

            for (j=0; j<nTableSize; j++)
            {
                *((PWORD)pBuffer) = FIX_ENDIAN16(*((PWORD)pTable));
                pBuffer += sizeof(WORD);
                pTable += sizeof(WORD);
            }
        }
    }

    return (DWORD) (pBuffer - pStart);
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

BOOL
GetHostCSA(
    PBYTE  pProfile,
    PBYTE  pBuffer,
    PDWORD pdwSize,
    DWORD  dwIntent,
    DWORD  dwType
    )
{
    PHOSTCLUT  pHostClut;
    PTAGDATA   pTagData;
    PLUT16TYPE pLut;
    DWORD      dwAToBTag;
    DWORD      dwPCS, dwLutSig;
    DWORD      nInputCh, nOutputCh, nGrids, SecondGrids;
    DWORD      nInputTable, nOutputTable, nNumbers;
    DWORD      dwIndex, i, j, k;
    PBYTE      pTable;
    PBYTE      pStart = pBuffer;

    switch (dwIntent)
    {
    case INTENT_PERCEPTUAL:
        dwAToBTag = TAG_AToB0;
        break;

    case INTENT_RELATIVE_COLORIMETRIC:
    case INTENT_ABSOLUTE_COLORIMETRIC:
        dwAToBTag = TAG_AToB1;
        break;

    case INTENT_SATURATION:
        dwAToBTag = TAG_AToB2;
        break;

    default:
        WARNING((__TEXT("Invalid intent passed to GetHostCSA: %d\n"), dwIntent));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
        break;
    }

    if (!DoesCPTagExist(pProfile, dwAToBTag, &dwIndex))
    {
        if (DoesTRCAndColorantTagExist(pProfile) && (dwType == TYPE_CIEBASEDDEF))
        {
            //
            // Create Host CSA from Matrix.
            //

            return CreateHostMatrixCSAorCRD(pProfile,pBuffer,pdwSize,dwIntent,TRUE);
        }
        else
        {
            WARNING((__TEXT("AToB tag not present for intent %d\n"), dwIntent));
            SetLastError(ERROR_TAG_NOT_PRESENT);
            return FALSE;
        }
    }

    //
    // Check if we can generate the CSA
    //

    dwPCS = GetCPConnSpace(pProfile);

    pTagData = (PTAGDATA)(pProfile + sizeof(PROFILEHEADER) + sizeof(DWORD) +
               dwIndex * sizeof(TAGDATA));

    pLut = (PLUT16TYPE)(pProfile + FIX_ENDIAN(pTagData->dwOffset));

    dwLutSig = FIX_ENDIAN(pLut->dwSignature);

    if (((dwPCS != SPACE_Lab) && (dwPCS != SPACE_XYZ)) ||
        ((dwLutSig != LUT8_TYPE) && (dwLutSig != LUT16_TYPE)))
    {
        WARNING((__TEXT("Invalid color space - unable to create DEF(G) host CSA\n")));
        SetLastError(ERROR_INVALID_PROFILE);
        return FALSE;
    }

    //
    // Estimate the memory size required to hold CSA
    //

    (void)GetCLUTInfo(dwLutSig, (PBYTE)pLut, &nInputCh, &nOutputCh, &nGrids,
                &nInputTable, &nOutputTable, &i);

    if (!(nOutputCh == 3) ||
        !((nInputCh == 3) && (dwType == TYPE_CIEBASEDDEF)) &&
        !((nInputCh == 4) && (dwType == TYPE_CIEBASEDDEFG)))
    {
        return FALSE;
    }
	
    if (pBuffer == NULL)
    {
        //
        // Return size
        //

        if (dwType == TYPE_CIEBASEDDEFG)
            *pdwSize = nOutputCh * nGrids * nGrids * nGrids * nGrids;
        else
            *pdwSize = nOutputCh * nGrids * nGrids * nGrids;

        *pdwSize +=                           // size of RenderTable 8-bits only
            nInputCh * nInputTable * i    +   // size of input table 8/16-bits
            nOutputCh * nOutputTable * i  +   // size of output table 8/16-bits
            sizeof(HOSTCLUT) + 2048;          // data structure + other PS stuff

        //
        // Add space for new line.
        //

        *pdwSize += (((*pdwSize) / MAX_LINELEN) + 1) * STRLEN(NewLine);

        return TRUE;
    }

    pHostClut = (PHOSTCLUT)pBuffer;
    pBuffer += sizeof(HOSTCLUT);
    pHostClut->wSize = sizeof(HOSTCLUT);
    pHostClut->wDataType = DATATYPE_LUT;
    pHostClut->dwPCS = dwPCS;
    pHostClut->dwIntent = dwIntent;
    pHostClut->nLutBits = (dwLutSig == LUT8_TYPE) ? 8 : 16;

    //
    // Get info about Illuminant White Point from the header
    //

    (void)GetCPWhitePoint(pProfile, pHostClut->afxIlluminantWP);

    pHostClut->nInputCh = (BYTE)nInputCh;
    pHostClut->nOutputCh = (BYTE)nOutputCh;
    pHostClut->nClutPoints = (BYTE)nGrids;
    pHostClut->nInputEntries = (WORD)nInputTable;
    pHostClut->nOutputEntries = (WORD)nOutputTable;

    //
    // Input Array
    //

    pBuffer += CreateHostInputOutputArray(pBuffer, pHostClut->inputArray,
             nInputCh, nInputTable, 0, dwLutSig, (PBYTE)pLut);

    if (dwType == TYPE_CIEBASEDDEFG)
    {
        i = nInputTable * nInputCh +
            nGrids * nGrids * nGrids * nGrids * nOutputCh;
    }
    else
    {
        i = nInputTable * nInputCh +
            nGrids * nGrids * nGrids * nOutputCh;
    }

    //
    // Output Array
    //

    pBuffer += CreateHostInputOutputArray(pBuffer, pHostClut->outputArray,
             nOutputCh, nOutputTable, i, dwLutSig, (PBYTE)pLut);

    //
    // /Table
    //

    pHostClut->clut = pBuffer;
    nNumbers = nGrids * nGrids * nOutputCh;
    SecondGrids = 1;
    if (dwType == TYPE_CIEBASEDDEFG)
    {
        SecondGrids = nGrids;
    }

    for (i=0; i<nGrids; i++)        // Nh strings should be sent
    {
        for (k=0; k<SecondGrids; k++)
        {
            if (dwLutSig == LUT8_TYPE)
            {
                pTable = (PBYTE) (((PLUT8TYPE)pLut)->data) +
                    nInputTable * nInputCh +
                    nNumbers * (i * SecondGrids + k);
            }
            else
            {
                pTable = (PBYTE) (((PLUT16TYPE)pLut)->data) +
                    2 * nInputTable * nInputCh +
                    2 * nNumbers * (i * SecondGrids + k);
            }

            if (dwLutSig == LUT8_TYPE)
            {
                CopyMemory(pBuffer, pTable, nNumbers);
                pBuffer += nNumbers;
            }
            else
            {
                for (j=0; j<nNumbers; j++)
                {
                    *pBuffer++ = (BYTE)(FIX_ENDIAN16(*((PWORD)pTable)) / 256);
                    pTable += sizeof(WORD);
                }
            }
        }
    }

    *pdwSize = (DWORD) (pBuffer - pStart);

    return TRUE;
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

BOOL
GetHostColorSpaceArray(
    PBYTE  pProfile,
    DWORD  dwIntent,
    PBYTE  pBuffer,
    PDWORD pdwSize
    )
{
    DWORD dwDev;
    BOOL bRc = FALSE;

    dwDev = GetCPDevSpace(pProfile);

    switch (dwDev)
    {
    case SPACE_RGB:
        bRc = GetHostCSA(pProfile, pBuffer, pdwSize,
                  dwIntent, TYPE_CIEBASEDDEF);
        break;
    case SPACE_CMYK:
        bRc = GetHostCSA(pProfile, pBuffer, pdwSize,
                  dwIntent, TYPE_CIEBASEDDEFG);
        break;
    default:
        break;
    }

    return bRc;
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

BOOL
DoHostConversionCRD(
    PHOSTCLUT   pHostCRD,
    PHOSTCLUT   pHostCSA,
    float       *pfInput,
    float       *pfOutput,
    BOOL        bCheckOutputTable
    )
{
    float      fTemp[MAXCHANNELS];
    float      fTemp1[MAXCHANNELS];
    DWORD      i, j;

    //
    // Input XYZ or Lab in range [0 2]
    //
    // When sampling the deviceCRD, skip the input table.
    // If pHostCSA is not NULL, the current CRD is targetCRD, we
    // need to do input table conversion
    //

    if (pHostCSA)
    {
        //
        // Convert Lab to XYZ  in range [ 0 whitePoint ]
        //

        if ((pHostCRD->dwPCS == SPACE_XYZ) &&
            (pHostCSA->dwPCS == SPACE_Lab))
        {
            LabToXYZ(pfInput, fTemp1, pHostCRD->afxIlluminantWP);
        }
        else if ((pHostCRD->dwPCS == SPACE_Lab) &&
                 (pHostCSA->dwPCS == SPACE_XYZ))
        {
            //
            // Convert XYZ to Lab in range [ 0 1]
            //

            XYZToLab(pfInput, fTemp, pHostCSA->afxIlluminantWP);
        }
        else if ((pHostCRD->dwPCS == SPACE_Lab) &&
                 (pHostCSA->dwPCS == SPACE_Lab))
        {
            //
            // Convert Lab to range [ 0 1]
            //

            for (i=0; i<3; i++)
                fTemp[i] = pfInput[i] / 2;
        }
        else
        {
            //
            // Convert XYZ to XYZ (based on white point) to range [0 1]
            //
            // TODO:
            //   different intents using different conversion.
            //   icRelativeColorimetric: using Bradford transform.
            //   icAbsoluteColorimetric: using scaling.
            //

            for (i=0; i<3; i++)
                fTemp1[i] = (pfInput[i] * pHostCRD->afxIlluminantWP[i]) / pHostCSA->afxIlluminantWP[i];
        }

        //
        // Matrix, used for XYZ data only.
        //

        if (pHostCRD->dwPCS == SPACE_XYZ)
        {
            ApplyMatrix(pHostCRD->e, fTemp1, fTemp);
        }

        if (pHostCRD->wDataType != DATATYPE_MATRIX)
        {
            //
            // Search input Table
            //

            (void)CheckInputOutputTable(pHostCRD, fTemp, FALSE, TRUE);
        }
    }

    //
    // If the current CRD is device CRD, we do not need to do input
    // table conversion
    //

    else
    {
        WORD nGrids;

    	nGrids = pHostCRD->nClutPoints;

        //
        // Sample data may be XYZ or Lab. It depends on Target icc profile.
        // If the PCS of the target icc profile is XYZ, input data will be XYZ.
        // If the PCS of the target icc profile is Lab, input data will be Lab.
        //

        if (pHostCRD->wDataType == DATATYPE_MATRIX)
        {
            for (i = 0; i < 3; i++)
            {
                fTemp[i] = pfInput[i];
            }
        }
        else
        {
            for (i=0; i<3; i++)
            {
                fTemp[i] = pfInput[i] * (nGrids - 1);
                if (fTemp[i] > (nGrids - 1))
                    fTemp[i] = (float)(nGrids - 1);
            }
        }
    }

    if (pHostCRD->wDataType != DATATYPE_MATRIX)
    {
        //
        // Rendering table
        //

        (void)CheckColorLookupTable(pHostCRD, fTemp);
    }

    //
    // Output RGB or CMYK in range [0 1]
    //

    if (bCheckOutputTable)
    {
        (void)CheckInputOutputTable(pHostCRD, fTemp, FALSE, FALSE);
    }

    for (i=0; (i<=MAXCHANNELS) && (i<pHostCRD->nOutputCh); i++)
    {
        pfOutput[i] = fTemp[i];
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

BOOL
DoHostConversionCSA(
    PHOSTCLUT     pHostClut,
    float         *pfInput,
    float         *pfOutput
    )
{
    float      fTemp[MAXCHANNELS];
    DWORD      i;

    //
    // Input RGB or CMYK in range [0 1]
    //

    for (i=0; (i<=MAXCHANNELS) && (i<pHostClut->nInputCh); i++)
    {
        fTemp[i] = pfInput[i];
    }

    //
    // Search input Table
    //

    (void)CheckInputOutputTable(pHostClut, fTemp, TRUE, TRUE);

    if (pHostClut->wDataType == DATATYPE_MATRIX)
    {
        ApplyMatrix(pHostClut->e, fTemp, pfOutput);
    }
    else
    {
        //
        // Rendering table
        //

        (void)CheckColorLookupTable(pHostClut, fTemp);

        //
        // Output Table
        //

        (void)CheckInputOutputTable(pHostClut, fTemp, TRUE, FALSE);

        //
        // Output XYZ or Lab in range [0 2]
        //

        for (i=0; (i<=MAXCHANNELS) && (i<pHostClut->nOutputCh); i++)
        {
            pfOutput[i] = fTemp[i];
        }
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

BOOL
CheckInputOutputTable(
    PHOSTCLUT    pHostClut,
    float        *pfTemp,
    BOOL         bCSA,
    BOOL         bInputTable
    )
{
    PBYTE      *ppArray;
    float      fIndex;
    DWORD      nNumCh;
    DWORD      nNumEntries, i;
    WORD       nGrids;
    WORD       floor1, ceiling1;

    if (bInputTable)
    {
        nNumCh = pHostClut->nInputCh;
        nNumEntries = pHostClut->nInputEntries - 1;
        ppArray = pHostClut->inputArray;
    }
    else
    {
        nNumCh = pHostClut->nOutputCh;
        nNumEntries = pHostClut->nOutputEntries - 1;
        ppArray = pHostClut->outputArray;
    }

    nGrids = pHostClut->nClutPoints;
    for (i=0; (i<=MAXCHANNELS) && (i<nNumCh); i++)
    {
        pfTemp[i] = (pfTemp[i] < 0) ? 0 : ((pfTemp[i] > 1) ? 1 : pfTemp[i]);

        fIndex = pfTemp[i] * nNumEntries;

        if (pHostClut->nLutBits == 8)
        {
            floor1 = ppArray[i][(DWORD)fIndex];
            ceiling1 = ppArray[i][((DWORD)fIndex) + 1];

            pfTemp[i] = (float)(floor1 + (ceiling1 - floor1) * (fIndex - floor(fIndex)));

            if (bCSA && !bInputTable)
                pfTemp[i] = pfTemp[i] / 127.0f;
            else
                pfTemp[i] = pfTemp[i] / 255.0f;
        }
        else
        {
            floor1 = ((PWORD)(ppArray[i]))[(DWORD)fIndex];
            ceiling1 = ((PWORD)(ppArray[i]))[((DWORD)fIndex) + 1];

            pfTemp[i] = (float)(floor1 + (ceiling1 - floor1) * (fIndex - floor(fIndex)));

            if (bCSA && !bInputTable)
                pfTemp[i] = pfTemp[i] / 32767.0f;
            else
                pfTemp[i] = pfTemp[i] / 65535.0f;

        }

        if (bInputTable)
        {
            pfTemp[i] *= (nGrids - 1);
            if (pfTemp[i] > (nGrids - 1))
                pfTemp[i] =  (float)(nGrids - 1);
        }
    }

    return TRUE;
}


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

float g(
    float f
    )
{
    float fRc;

    if (f >= (6.0f/29.0f))
    {
        fRc = f * f * f;
    }
    else
    {
        fRc = f - (4.0f / 29.0f) * (108.0f / 841.0f);
    }

    return fRc;
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

float
inverse_g(
    float f
    )
{
    double fRc;

    if (f >= (6.0f*6.0f*6.0f)/(29.0f*29.0f*29.0f))
    {
        fRc = pow(f, 1.0 / 3.0);
    }
    else
    {
        fRc = f * (841.0f / 108.0f) + (4.0f / 29.0f);
    }

    return (float)fRc;
}


void
LabToXYZ(
    float      *pfInput,
    float      *pfOutput,
    PFIX_16_16 pafxWP
    )
{
    float   fL, fa, fb;

    fL = (pfInput[0] * 50 + 16) / 116;
    fa = (pfInput[1] * 128 - 128) / 500;
    fb = (pfInput[2] * 128 - 128) / 200;

    pfOutput[0] = pafxWP[0] * g(fL + fa) / FIX_16_16_SCALE;
    pfOutput[1] = pafxWP[1] * g(fL) / FIX_16_16_SCALE;
    pfOutput[2] = pafxWP[2] * g(fL - fb) / FIX_16_16_SCALE;

    return;
}


void
XYZToLab(
    float      *pfInput,
    float      *pfOutput,
    PFIX_16_16 pafxWP
    )
{
    float  fL, fa, fb;

    fL = inverse_g(pfInput[0] * FIX_16_16_SCALE / pafxWP[0]);
    fa = inverse_g(pfInput[1] * FIX_16_16_SCALE / pafxWP[1]);
    fb = inverse_g(pfInput[2] * FIX_16_16_SCALE / pafxWP[2]);

    pfOutput[0] = (fa * 116 - 16) / 100;
    pfOutput[1] = (fL * 500 - fa * 500 + 128) / 255;
    pfOutput[2] = (fa * 200 - fb * 200 + 128) / 255;

    return;
}


BOOL
TableInterp3(
    PHOSTCLUT  pHostClut,
    float      *pfTemp
    )
{

    PBYTE        v000, v001, v010, v011;
    PBYTE        v100, v101, v110, v111;
    float        fA, fB, fC;
    float        fVx0x, fVx1x;
    float        fV0xx, fV1xx;
    DWORD        tmpA, tmpBC;
    DWORD        cellA, cellB, cellC;
    DWORD        idx;
    WORD         nGrids;
    WORD         nOutputCh;

    cellA = (DWORD)pfTemp[0];
    fA = pfTemp[0] - cellA;

    cellB = (DWORD)pfTemp[1];
    fB = pfTemp[1] - cellB;

    cellC = (DWORD)pfTemp[2];
    fC = pfTemp[2] - cellC;

    nGrids = pHostClut->nClutPoints;
    nOutputCh = pHostClut->nOutputCh;
    tmpA  = nOutputCh * nGrids * nGrids;
    tmpBC = nOutputCh * (nGrids * cellB + cellC);

    //
    // Calculate 8 surrounding cells.
    //

    v000 = pHostClut->clut + tmpA * cellA + tmpBC;
    v001 = (cellC < (DWORD)(nGrids - 1)) ? v000 + nOutputCh : v000;
    v010 = (cellB < (DWORD)(nGrids - 1)) ? v000 + nOutputCh * nGrids : v000;
    v011 = (cellC < (DWORD)(nGrids - 1)) ? v010 + nOutputCh : v010 ;

    v100 = (cellA < (DWORD)(nGrids - 1)) ? v000 + tmpA : v000;
    v101 = (cellC < (DWORD)(nGrids - 1)) ? v100 + nOutputCh : v100;
    v110 = (cellB < (DWORD)(nGrids - 1)) ? v100 + nOutputCh * nGrids : v100;
    v111 = (cellC < (DWORD)(nGrids - 1)) ? v110 + nOutputCh : v110;

    for (idx=0; idx<nOutputCh; idx++)
    {
        //
        // Calculate the average of 4 bottom cells.
        //

        fVx0x = *v000 + fC * (int)((int)*v001 - (int)*v000);
        fVx1x = *v010 + fC * (int)((int)*v011 - (int)*v010);
        fV0xx = fVx0x + fB * (fVx1x - fVx0x);

        //
        // Calculate the average of 4 upper cells.
        //

        fVx0x = *v100 + fC * (int)((int)*v101 - (int)*v100);
        fVx1x = *v110 + fC * (int)((int)*v111 - (int)*v110);
        fV1xx = fVx0x + fB * (fVx1x - fVx0x);

        //
        // Calculate the bottom and upper average.
        //

        pfTemp[idx] = (fV0xx + fA * (fV1xx - fV0xx)) / MAXCOLOR8;

        if ( idx < (DWORD)(nOutputCh - 1))
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


BOOL
TableInterp4(
    PHOSTCLUT  pHostClut,
    float      *pfTemp
    )
{
    PBYTE     v0000, v0001, v0010, v0011;
    PBYTE     v0100, v0101, v0110, v0111;
    PBYTE     v1000, v1001, v1010, v1011;
    PBYTE     v1100, v1101, v1110, v1111;
    float     fH, fI, fJ, fK;
    float     fVxx0x, fVxx1x;
    float     fVx0xx, fVx1xx;
    float     fV0xxx, fV1xxx;
    DWORD     tmpH, tmpI, tmpJK;
    DWORD     cellH, cellI, cellJ, cellK;
    DWORD     idx;
    WORD      nGrids;
    WORD      nOutputCh;

    cellH = (DWORD)pfTemp[0];
    fH = pfTemp[0] - cellH;

    cellI = (DWORD)pfTemp[1];
    fI = pfTemp[1] - cellI;

    cellJ = (DWORD)pfTemp[2];
    fJ = pfTemp[2] - cellJ;

    cellK = (DWORD)pfTemp[3];
    fK = pfTemp[3] - cellK;

    nGrids = pHostClut->nClutPoints;
    nOutputCh = pHostClut->nOutputCh;
    tmpI  = nOutputCh * nGrids * nGrids;
    tmpH  = tmpI * nGrids;
    tmpJK = nOutputCh * (nGrids * cellJ + cellK);

    //
    // Calculate 16 surrounding cells.
    //

    v0000 = pHostClut->clut + tmpH * cellH + tmpI * cellI + tmpJK;
    v0001 = (cellK < (DWORD)(nGrids - 1))? v0000 + nOutputCh : v0000;
    v0010 = (cellJ < (DWORD)(nGrids - 1))? v0000 + nOutputCh * nGrids : v0000;
    v0011 = (cellK < (DWORD)(nGrids - 1))? v0010 + nOutputCh : v0010;

    v0100 = (cellI < (DWORD)(nGrids - 1))? v0000 + tmpI : v0000;
    v0101 = (cellK < (DWORD)(nGrids - 1))? v0100 + nOutputCh : v0100;
    v0110 = (cellJ < (DWORD)(nGrids - 1))? v0100 + nOutputCh * nGrids : v0100;
    v0111 = (cellK < (DWORD)(nGrids - 1))? v0110 + nOutputCh : v0110;

    v1000 = (cellH < (DWORD)(nGrids - 1))? v0000 + tmpH : v0000;
    v1001 = (cellK < (DWORD)(nGrids - 1))? v1000 + nOutputCh : v1000;
    v1010 = (cellJ < (DWORD)(nGrids - 1))? v1000 + nOutputCh * nGrids : v1000;
    v1011 = (cellK < (DWORD)(nGrids - 1))? v1010 + nOutputCh : v1010;

    v1100 = (cellI < (DWORD)(nGrids - 1))? v1000 + tmpI : v1000;
    v1101 = (cellK < (DWORD)(nGrids - 1))? v1100 + nOutputCh : v1100;
    v1110 = (cellJ < (DWORD)(nGrids - 1))? v1100 + nOutputCh * nGrids : v1100;
    v1111 = (cellK < (DWORD)(nGrids - 1))? v1110 + nOutputCh : v1110;


    for (idx=0; idx<nOutputCh; idx++)
    {
        //
        // Calculate the average of 8 bottom cells.
        //

        fVxx0x = *v0000 + fK * (int)((int)*v0001 - (int)*v0000);
        fVxx1x = *v0010 + fK * (int)((int)*v0011 - (int)*v0010);
        fVx0xx = fVxx0x + fJ * (fVxx1x - fVxx0x);
        fVxx0x = *v0100 + fK * (int)((int)*v0101 - (int)*v0100);
        fVxx1x = *v0110 + fK * (int)((int)*v0111 - (int)*v0110);
        fVx1xx = fVxx0x + fJ * (fVxx1x - fVxx0x);
        fV0xxx = fVx0xx + fI * (fVx1xx - fVx0xx);

        //
        // Calculate the average of 8 upper cells.
        //

        fVxx0x = *v1000 + fK * (int)((int)*v1001 - (int)*v1000);
        fVxx1x = *v1010 + fK * (int)((int)*v1011 - (int)*v1010);
        fVx0xx = fVxx0x + fJ * (fVxx1x - fVxx0x);
        fVxx0x = *v1100 + fK * (int)((int)*v1101 - (int)*v1100);
        fVxx1x = *v1110 + fK * (int)((int)*v1111 - (int)*v1110);
        fVx1xx = fVxx0x + fJ * (fVxx1x - fVxx0x);
        fV1xxx = fVx0xx + fI * (fVx1xx - fVx0xx);

        //
        // Calculate the bottom and upper average.
        //

        pfTemp[idx] = (fV0xxx + fH * (fV1xxx - fV0xxx)) / MAXCOLOR8;

        if (idx < (DWORD)(nOutputCh - 1))
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

BOOL
InvertColorantArray(
    double *lpInMatrix,
    double *lpOutMatrix)
{
    double det;

    double *a;
    double *b;
    double *c;

    a = &(lpInMatrix[0]);
    b = &(lpInMatrix[3]);
    c = &(lpInMatrix[6]);

    det = a[0] * b[1] * c[2] + a[1] * b[2] * c[0] + a[2] * b[0] * c[1] -
         (a[2] * b[1] * c[0] + a[1] * b[0] * c[2] + a[0] * b[2] * c[1]);

    if (det == 0.0)    // What to do?
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
    }
    else
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
    }

    return (TRUE);
}

VOID
ApplyMatrix(
    FIX_16_16 *e,
    float *Input,
    float *Output)
{
    DWORD i, j;

    for (i=0; i<3; i++)
    {
        j = i * 3;

        Output[i] = ((e[j]     * Input[0]) / FIX_16_16_SCALE) +
                    ((e[j + 1] * Input[1]) / FIX_16_16_SCALE) +
                    ((e[j + 2] * Input[2]) / FIX_16_16_SCALE);
    }
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

BOOL
CheckColorLookupTable(
    PHOSTCLUT   pHostClut,
    float       *pfTemp
    )
{
    if (pHostClut->nInputCh == 3)
    {
        return TableInterp3(pHostClut, pfTemp);
    }
    else if (pHostClut->nInputCh == 4)
    {
        return TableInterp4(pHostClut, pfTemp);
    }
    else
        return FALSE;
}

//
// For testing purposes
//

BOOL WINAPI
GetPS2PreviewCRD (
    HPROFILE  hDestProfile,
    HPROFILE  hTargetProfile,
    DWORD     dwIntent,
    PBYTE     pBuffer,
    PDWORD    pcbSize,
    LPBOOL    pbBinary
    )
{
    PPROFOBJ pDestProfObj;
    PPROFOBJ pTargetProfObj;

    pDestProfObj = (PPROFOBJ)HDLTOPTR(hDestProfile);
    pTargetProfObj = (PPROFOBJ)HDLTOPTR(hTargetProfile);


    return InternalGetPS2PreviewCRD(pDestProfObj->pView, pTargetProfObj->pView, dwIntent, pBuffer, pcbSize, pbBinary);
}

#endif // !defined(KERNEL_MODE) || defined(USERMODE_DRIVER)

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

DWORD crc32(PBYTE buff, DWORD length)
{
    DWORD crc, charcnt;
    BYTE  c;

    crc = 0xFFFFFFFF;
    charcnt = 0;

    for (charcnt = 0 ; charcnt < length ; charcnt++)
    {
        c = buff[charcnt];
        crc = crc_32_tab[(crc ^ c) & 0xff] ^ (crc >> 8);
    }

    return crc;
}

/***************************************************************************
*                           IsSRGBColorProfile
*
*  function: check if the profile is sRGB
*
*  parameters:
*       cp          --  Color Profile handle
*
*  returns:
*       BOOL        --  TRUE if the profile is sRGB
*                       FALSE otherwise.
***************************************************************************/

BOOL IsSRGBColorProfile(
    PBYTE pProfile
    )
{
    BOOL  bMatch = FALSE;
    DWORD dwRedTRCIndex, dwGreenTRCIndex, dwBlueTRCIndex;
    DWORD dwRedCIndex, dwGreenCIndex, dwBlueCIndex;
    DWORD dwSize;
    DWORD dwRedTRCSize=0, dwGreenTRCSize=0, dwBlueTRCSize=0;
    DWORD dwRedCSize=0, dwGreenCSize=0, dwBlueCSize=0;
    PBYTE pRed, pGreen, pBlue, pRedC, pGreenC, pBlueC;
    BYTE  DataBuffer[ALIGN_DWORD(sRGB_TAGSIZE)];

    if (DoesCPTagExist(pProfile, TAG_REDTRC, &dwRedTRCIndex)             &&
        GetCPElementDataSize(pProfile, dwRedTRCIndex, &dwRedTRCSize)     &&

        DoesCPTagExist(pProfile, TAG_GREENTRC, &dwGreenTRCIndex)         &&
        GetCPElementDataSize(pProfile, dwGreenTRCIndex, &dwGreenTRCSize) &&

        DoesCPTagExist(pProfile, TAG_BLUETRC, &dwBlueTRCIndex)           &&
        GetCPElementDataSize(pProfile, dwBlueTRCIndex, &dwBlueTRCSize)   &&

        DoesCPTagExist(pProfile, TAG_REDCOLORANT, &dwRedCIndex)          &&
        GetCPElementDataSize(pProfile, dwRedCIndex, &dwRedCSize)         &&

        DoesCPTagExist(pProfile, TAG_GREENCOLORANT, &dwGreenCIndex)      &&
        GetCPElementDataSize(pProfile, dwGreenCIndex, &dwGreenCSize)     &&

        DoesCPTagExist(pProfile, TAG_BLUECOLORANT, &dwBlueCIndex)        &&
        GetCPElementDataSize(pProfile, dwBlueCIndex, &dwBlueCSize))
    {
        dwSize = dwRedTRCSize + dwGreenTRCSize + dwBlueTRCSize +
                 dwRedCSize   + dwGreenCSize   + dwBlueCSize;

        if (dwSize == sRGB_TAGSIZE)
        {
            ZeroMemory(DataBuffer,sizeof(DataBuffer));

            pRed    = DataBuffer;
            pGreen  = pRed    + dwRedTRCSize;
            pBlue   = pGreen  + dwGreenTRCSize;
            pRedC   = pBlue   + dwBlueTRCSize;
            pGreenC = pRedC   + dwRedCSize;
            pBlueC  = pGreenC + dwGreenCSize;

            if (GetCPElementData(pProfile, dwRedTRCIndex, pRed, &dwRedTRCSize)       &&
                GetCPElementData(pProfile, dwGreenTRCIndex, pGreen, &dwGreenTRCSize) &&
                GetCPElementData(pProfile, dwBlueTRCIndex, pBlue, &dwBlueTRCSize)    &&
                GetCPElementData(pProfile, dwRedCIndex, pRedC, &dwRedCSize)          &&
                GetCPElementData(pProfile, dwGreenCIndex, pGreenC, &dwGreenCSize)    &&
                GetCPElementData(pProfile, dwBlueCIndex, pBlueC, &dwBlueCSize))
            {
                bMatch = (crc32(DataBuffer, sRGB_TAGSIZE) == sRGB_CRC);
            }
        }
    }

    return (bMatch);
}




