/* Header file guard bands */
#ifndef ICC_H
#define ICC_H

/*****************************************************************
 Copyright (c) 1994 SunSoft, Inc.

                    All Rights Reserved

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without restrict-
ion, including without limitation the rights to use, copy, modify,
merge, publish distribute, sublicense, and/or sell copies of the
Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-
INFRINGEMENT.  IN NO EVENT SHALL SUNSOFT, INC. OR ITS PARENT
COMPANY BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of SunSoft, Inc.
shall not be used in advertising or otherwise to promote the
sale, use or other dealings in this Software without written
authorization from SunSoft Inc.
******************************************************************/

/*
 * This version of the header file corresponds to the profile
 * specification version 3.0.
 *
 * All header file entries are pre-fixed with "ic" to help
 * avoid name space collisions. Signatures are pre-fixed with
 * icSig.
 *
 * The structures defined in this header file were created to
 * represent a description of an ICC profile on disk. Rather
 * than use pointers a technique is used where a single byte array
 * was placed at the end of each structure. This allows us in "C"
 * to extend the structure by allocating more data than is needed
 * to account for variable length structures.
 *
 * This also ensures that data following is allocated
 * contiguously and makes it easier to write and read data from
 * the file.
 *
 * For example to allocate space for a 256 count length UCR
 * and BG array, and fill the allocated data.
 *
        icUcrBgCurve    *ucrCurve, *bgCurve;
        int             ucr_nbytes, bg_nbytes;
        icUcrBg         *ucrBgWrite;

        ucr_nbytes = sizeof(icUInt32Number) +
                 (UCR_CURVE_SIZE * sizeof(icUInt16Number));
        bg_nbytes = sizeof(icUInt32Number) +
                 (BG_CURVE_SIZE * sizeof(icUInt16Number));

        ucrBgWrite = (icUcrBg *)malloc((ucr_nbytes + bg_nbytes));

        ucrCurve = (icUcrBgCurve *)ucrBgWrite->data;
        ucrCurve->count = UCR_CURVE_SIZE;
        for (i=0; i<ucrCurve->count; i++)
                ucrCurve->curve[i] = (icUInt16Number)i;

        bgCurve = (icUcrBgCurve *)((char *)ucrCurve + ucr_nbytes);
        bgCurve->count = BG_CURVE_SIZE;
        for (i=0; i<bgCurve->count; i++)
                bgCurve->curve[i] = 255 - (icUInt16Number)i;
 *
 */

/*
 * Many of the structures contain variable length arrays. This
 * is represented by the use of the convention.
 *
 *      type    data[icAny];
 */

/*------------------------------------------------------------------------*/
/*
 * Defines used in the specification
 */
#define icMagicNumber                   0x61637370      /* 'acsp' */
#define icVersionNumber                 0x02000000      /* 2.0, BCD */

/* Screening Encodings */
#define icPrtrDefaultScreensFalse       0x00000000      /* Bit position 0 */
#define icPrtrDefaultScreensTrue        0x00000001      /* Bit position 0 */
#define icLinesPerInch                  0x00000002      /* Bit position 1 */
#define icLinesPerCm                    0x00000000      /* Bit position 1 */

/*
 * Device attributes, currently defined values correspond
 * to the low 4 bytes of the 8 byte attribute quantity, see
 * the header for their location.
 */
#define icReflective                    0x00000000      /* Bit position 0 */
#define icTransparency                  0x00000001      /* Bit position 0 */
#define icGlossy                        0x00000000      /* Bit position 1 */
#define icMatte                         0x00000002      /* Bit position 1 */

/*
 * Profile header flags, the low 16 bits are reserved for consortium
 * use.
 */
#define icEmbeddedProfileFalse          0x00000000      /* Bit position 0 */
#define icEmbeddedProfileTrue           0x00000001      /* Bit position 0 */
#define icUseAnywhere                   0x00000000      /* Bit position 1 */
#define icUseWithEmdeddedDataOnly       0x00000002      /* Bit position 1 */

/* Ascii or Binary data */
#define icAsciiData                     0x00000000      /* Used in dataType */
#define icBinaryData                    0x00000001

/*
 * Define used to indicate that this is a variable length array
 */
#define icAny                           1

/*------------------------------------------------------------------------*/
/*
 * Signatures, these are basically 4 byte identifiers
 * used to differentiate between tags and other items
 * in the profile format.
 */
typedef unsigned char    icSignature[4];

typedef unsigned char   icTagSignature[4];
/* public tags and sizes */
#define    icSigAToB0Tag                       0x41324230   /* 'A2B0' */
#define    icSigAToB1Tag                       0x41324231   /* 'A2B1' */
#define    icSigAToB2Tag                       0x41324232   /* 'A2B2' */
#define    icSigAToB3Tag                       0x41324233   /* 'A2B2' */
#define    icSigBlueColorantTag                0x6258595A   /* 'bXYZ' */
#define    icSigBlueTRCTag                     0x62545243   /* 'bTRC' */
#define    icSigBToA0Tag                       0x42324130   /* 'B2A0' */
#define    icSigBToA1Tag                       0x42324131   /* 'B2A1' */
#define    icSigBToA2Tag                       0x42324132   /* 'B2A2' */
#define    icSigBToA3Tag                       0x42324133   /* 'B2A3' */
#define    icSigCalibrationDateTimeTag         0x63616C74   /* 'calt' */
#define    icSigCharTargetTag                  0x74617267   /* 'targ' */
#define    icSigCopyrightTag                   0x63707274   /* 'cprt' */
#define    icSigDeviceMfgDescTag               0x646D6E64   /* 'dmnd' */
#define    icSigDeviceModelDescTag             0x646D6464   /* 'dmdd' */
#define    icSigGamutTag                       0x676d7420   /* 'gmt ' */
#define    icSigGrayTRCTag                     0x6b545243   /* 'kTRC' */
#define    icSigGreenColorantTag               0x6758595A   /* 'gXYZ' */
#define    icSigGreenTRCTag                    0x67545243   /* 'gTRC' */
#define    icSigLuminanceTag                   0x6C756d69   /* 'lumi' */
#define    icSigMeasurementTag                 0x6D656173   /* 'meas' */
#define    icSigMediaBlackPointTag             0x626B7074   /* 'bkpt' */
#define    icSigMediaWhitePointTag             0x77747074   /* 'wtpt' */
#define    icSigNamedColorTag                  0x6E636f6C   /* 'ncol' */
#define    icSigPreview0Tag                    0x70726530   /* 'pre0' */
#define    icSigPreview1Tag                    0x70726531   /* 'pre1' */
#define    icSigPreview2Tag                    0x70726532   /* 'pre2' */
#define    icSigProfileDescriptionTag          0x64657363   /* 'desc' */
#define    icSigProfileSequenceDescTag         0x70736571   /* 'pseq' */
#define    icSigPs2CRD0Tag                     0x70736430   /* 'psd0' */
#define    icSigPs2CRD1Tag                     0x70736431   /* 'psd1' */
#define    icSigPs2CRD2Tag                     0x70736432   /* 'psd2' */
#define    icSigPs2CRD3Tag                     0x70736433   /* 'psd3' */
#define    icSigPs2CSATag                      0x70733273   /* 'ps2s' */
#define    icSigPs2Intent0Tag                  0x70736930   /* 'psi0' */
#define    icSigPs2Intent1Tag                  0x70736931   /* 'psi1' */
#define    icSigPs2Intent2Tag                  0x70736932   /* 'psi2' */
#define    icSigPs2Intent3Tag                  0x70736933   /* 'psi3' */
#define    icSigRedColorantTag                 0x7258595A   /* 'rXYZ' */
#define    icSigRedTRCTag                      0x72545243   /* 'rTRC' */
#define    icSigScreeningDescTag               0x73637264   /* 'scrd' */
#define    icSigScreeningTag                   0x7363726E   /* 'scrn' */
#define    icSigTechnologyTag                  0x74656368   /* 'tech' */
#define    icSigUcrBgTag                       0x62666420   /* 'bfd ' */
#define    icSigViewingCondDescTag             0x76756564   /* 'vued' */
#define    icSigViewingConditionsTag           0x76696577   /* 'view' */
#define    icSigK007Tag                        0x4B303037   /* 'K007' */
#define    icMaxEnumTag                        0xFFFFFFFF   /* enum = 4
bytes max */

typedef unsigned char   icTechnologySignature[4];
/* technology signature descriptions */
#define    icSigFilmScanner                    0x6673636E   /* 'fscn' */
#define    icSigReflectiveScanner              0x7273636E   /* 'rscn' */
#define    icSigInkJetPrinter                  0x696A6574   /* 'ijet' */
#define    icSigThermalWaxPrinter              0x74776178   /* 'twax' */
#define    icSigElectrophotographicPrinter     0x6570686F   /* 'epho' */
#define    icSigElectrostaticPrinter           0x65737461   /* 'esta' */
#define    icSigDyeSublimationPrinter          0x64737562   /* 'dsub' */
#define    icSigPhotographicPaperPrinter       0x7270686F   /* 'rpho' */
#define    icSigFilmWriter                     0x6670726E   /* 'fprn' */
#define    icSigVideoMonitor                   0x7669646D   /* 'vidm' */
#define    icSigVideoCamera                    0x76696463   /* 'vidc' */
#define    icSigProjectionTelevision           0x706A7476   /* 'pjtv' */
#define    icSigCRTDisplay                     0x43525420   /* 'CRT ' */
#define    icSigPMDisplay                      0x504D4420   /* 'PMD ' */
#define    icSigAMDisplay                      0x414D4420   /* 'AMD ' */
#define    icSigPhotoCD                        0x4B504344   /* 'KPCD' */
#define    icSigPhotoImageSetter               0x696D6773   /* 'imgs' */
#define    icSigGravure                        0x67726176   /* 'grav' */
#define    icSigOffsetLithography              0x6F666673   /* 'offs' */
#define    icSigSilkscreen                     0x73696C6B   /* 'silk' */
#define    icSigFlexography                    0x666C6578   /* 'flex' */
#define    icMaxEnumTechnology                 0xFFFFFFFF   /* enum = 4
bytes max */

typedef unsigned char   icTagTypeSignature[4];
/* type signatures */
#define    icSigCurveType                      0x63757276   /* 'curv' */
#define    icSigDataType                       0x64617461   /* 'data' */
#define    icSigDateTimeType                   0x6474696D   /* 'dtim' */
#define    icSigLut16Type                      0x6d667432   /* 'mft2' */
#define    icSigLut8Type                       0x6d667431   /* 'mft1' */
#define    icSigMeasurementType                0x6D656173   /* 'meas' */
#define    icSigNamedColorType                 0x6E636f6C   /* 'ncol' */
#define    icSigProfileSequenceDescType        0x70736571   /* 'pseq' */
#define    icSigS15Fixed16ArrayType            0x73663332   /* 'sf32' */
#define    icSigScreeningType                  0x7363726E   /* 'scrn' */
#define    icSigSignatureType                  0x73696720   /* 'sig ' */
#define    icSigTextType                       0x74657874   /* 'text' */
#define    icSigTextDescriptionType            0x64657363   /* 'desc' */
#define    icSigU16Fixed16ArrayType            0x75663332   /* 'uf32' */
#define    icSigUcrBgType                      0x62666420   /* 'bfd ' */
#define    icSigUInt16ArrayType                0x75693136   /* 'ui16' */
#define    icSigUInt32ArrayType                0x75693332   /* 'ui32' */
#define    icSigUInt64ArrayType                0x75693634   /* 'ui64' */
#define    icSigUInt8ArrayType                 0x75693038   /* 'ui08' */
#define    icSigViewingConditionsType          0x76696577   /* 'view' */
#define    icSigXYZType                        0x58595A20   /* 'XYZ ' */
#define    icMaxEnumType                       0xFFFFFFFF   /* enum = 4
bytes max */

/*
 * Color Space Signatures
 * Note that only icSigXYZData and icSigLabData are valid
 * Profile Connection Spaces (PCSs)
 */
typedef unsigned char   icColorSpaceSignature[4];
#define    icSigXYZData                        0x58595A20   /* 'XYZ ' */
#define    icSigLabData                        0x4C616220   /* 'Lab ' */
#define    icSigLuvData                        0x4C757620   /* 'Luv ' */
#define    icSigYCbCrData                      0x59436272   /* 'YCbr' */
#define    icSigYxyData                        0x59787920   /* 'Yxy ' */
#define    icSigRgbData                        0x52474220   /* 'RGB ' */
#define    icSigGrayData                       0x47524159   /* 'GRAY' */
#define    icSigHsvData                        0x48535620   /* 'HSV ' */
#define    icSigHlsData                        0x484C5320   /* 'HLS ' */
#define    icSigCmykData                       0x434D594B   /* 'CMYK' */
#define    icSigCmyData                        0x434D5920   /* 'CMY ' */
#define    icSigDefData                        0x44454620   /* 'DEF ' New Definition */
#define    icMaxEnumData                       0xFFFFFFFF   /* enum = 4
bytes max */

/* profileClass enumerations */
typedef unsigned char   icProfileClassSignature[4];
#define    icSigInputClass                     0x73636E72   /* 'scnr' */
#define    icSigDisplayClass                   0x6D6E7472   /* 'mntr' */
#define    icSigOutputClass                    0x70727472   /* 'prtr' */
#define    icSigLinkClass                      0x6C696E6B   /* 'link' */
#define    icSigAbstractClass                  0x61627374   /* 'abst' */
#define    icSigColorSpaceClass                0x73706163   /* 'spac' */
#define    icMaxEnumClass                      0xFFFFFFFF   /* enum = 4
bytes max */

/* Platform Signatures */
typedef unsigned char   icPlatformSignature[4];
#define    icSigMacintosh                      0x4150504C   /* 'APPL' */
#define    icSigMicrosoft                      0x4D534654   /* 'MSFT' */
#define    icSigSolaris                        0x53554E57   /* 'SUNW' */
#define    icSigSGI                            0x53474920   /* 'SGI ' */
#define    icSigTaligent                       0x54474E54   /* 'TGNT' */
#define    icMaxEnumPlatform                   0xFFFFFFFF   /* enum = 4
bytes max */

/*------------------------------------------------------------------------*/
/*
 * Other enums
 */

/* Measurement Flare, used in the measurmentType tag */
typedef unsigned char   icMeasurementFlare[4];
#define     icFlare0                    0x00000000      /* 0% flare */
#define     icFlare100                  0x00000001      /* 100% flare */
#define     icMaxFlare                  0xFFFFFFFF      /* enum = 4 bytes max */

/* Measurement Geometry, used in the measurmentType tag */
typedef unsigned char   icMeasurementGeometry[4];
#define     icGeometryUnknown           0x00000000      /* Unknown geometry */
#define     icGeometry045or450          0x00000001      /* 0/45 or 45/0 */
#define     icGeometry0dord0            0x00000002      /* 0/d or d/0 */
#define     icMaxGeometry               0xFFFFFFFF      /* enum = 4 bytes max */

/* Rendering Intents, used in the profile header */
typedef unsigned char   icRenderingIntent[4];
#define     icPerceptual                0
#define     icRelativeColorimetric      1
#define     icSaturation                2
#define     icAbsoluteColorimetric      3
#define     icUseRenderingIntent        0xFFFFFFFF      /* New Definition  */
#define     icMaxEnumIntent             0xFFFFFFFF      /* enum = 4 bytes max */

/* Different Spot Shapes currently defined, used for screeningType */
typedef unsigned char   icSpotShape[4];
#define     icSpotShapeUnknown          0
#define     icSpotShapePrinterDefault   1
#define     icSpotShapeRound            2
#define     icSpotShapeDiamond          3
#define     icSpotShapeEllipse          4
#define     icSpotShapeLine             5
#define     icSpotShapeSquare           6
#define     icSpotShapeCross            7
#define     icMaxEnumSpot               0xFFFFFFFF      /* enum = 4 bytes max */

/* Standard Observer, used in the measurmentType tag */
typedef unsigned char   icStandardObserver[4];
#define     icStdObsUnknown             0x00000000      /* Unknown observer */
#define     icStdObs1931TwoDegrees      0x00000001      /* 1931 two degrees */
#define     icStdObs1964TenDegrees      0x00000002      /* 1961 ten degrees */
#define     icMaxStdObs                 0xFFFFFFFF      /* enum = 4 bytes max */

/* Pre-defined illuminants, used in measurement and viewing conditions type */
typedef unsigned char   icIlluminant[4];
#define     icIlluminantUnknown                 = 0x00000000
#define     icIlluminantD50                     = 0x00000001
#define     icIlluminantD65                     = 0x00000002
#define     icIlluminantD93                     = 0x00000003
#define     icIlluminantF2                      = 0x00000004
#define     icIlluminantD55                     = 0x00000005
#define     icIlluminantA                       = 0x00000006
#define     icIlluminantEquiPowerE              = 0x00000007    /* Equi-Power (E) */
#define     icIlluminantF8                      = 0x00000008
#define     icMaxEnumIluminant                  = 0xFFFFFFFF    /* enum = 4

/*------------------------------------------------------------------------*/
/*
 * Number definitions
 */

/* Unsigned integer numbers */
typedef unsigned char   icUInt8Number;
typedef unsigned char   icUInt16Number[2];
typedef unsigned char   icUInt32Number[4];
typedef unsigned char   icUInt64Number[8];

/* Signed numbers */
typedef signed char     icInt8Number;
typedef signed char     icInt16Number[2];
typedef signed char     icInt32Number[4];
typedef signed char     icInt64Number[8];

/* Fixed numbers */
typedef signed char     icS15Fixed16Number[4];
typedef unsigned char   icU16Fixed16Number[4];

/*------------------------------------------------------------------------*/
/*
 * Arrays of numbers
 */

/* Int8 Array */
typedef struct {
    icInt8Number        data[icAny];    /* Variable array of values */
} icInt8Array;

/* UInt8 Array */
typedef struct {
    icUInt8Number       data[icAny];    /* Variable array of values */
} icUInt8Array;

/* uInt16 Array */
typedef struct {
    icUInt16Number      data[icAny];    /* Variable array of values */
} icUInt16Array;

/* Int16 Array */
typedef struct {
    icInt16Number       data[icAny];    /* Variable array of values */
} icInt16Array;

/* uInt32 Array */
typedef struct {
    icUInt32Number      data[icAny];    /* Variable array of values */
} icUInt32Array;

/* Int32 Array */
typedef struct {
    icInt32Number       data[icAny];    /* Variable array of values */
} icInt32Array;

/* UInt64 Array */
typedef struct {
    icUInt64Number      data[icAny];    /* Variable array of values */
} icUInt64Array;

/* Int64 Array */
typedef struct {
    icInt64Number       data[icAny];    /* Variable array of values */
} icInt64Array;

/* u16Fixed16 Array */
typedef struct {
    icU16Fixed16Number  data[icAny];    /* Variable array of values */
} icU16Fixed16Array;

/* s15Fixed16 Array */
typedef struct {
    icS15Fixed16Number  data[icAny];    /* Variable array of values */
} icS15Fixed16Array;

/* The base date time number */
typedef struct {
    icUInt16Number      year;
    icUInt16Number      month;
    icUInt16Number      day;
    icUInt16Number      hours;
    icUInt16Number      minutes;
    icUInt16Number      seconds;
} icDateTimeNumber;

/* XYZ Number  */
typedef struct {
    icS15Fixed16Number  X;
    icS15Fixed16Number  Y;
    icS15Fixed16Number  Z;
} icXYZNumber;

/* XYZ Array */
typedef struct {
    icXYZNumber         data[icAny];    /* Variable array of XYZ numbers */
} icXYZArray;

/* Curve */
typedef struct {
    icUInt32Number      count;          /* Number of entries */
    icUInt16Number      data[icAny];    /* The actual table data, real
                                         * number is determined by count
                                         */
} icCurve;

/* Data */
typedef struct {
    icUInt32Number      dataFlag;       /* 0 = ascii, 1 = binary */
    icInt8Number        data[icAny];    /* Data, size determined from tag */
} icData;

/* lut16 */
typedef struct {
    icUInt8Number       inputChan;      /* Number of input channels */
    icUInt8Number       outputChan;     /* Number of output channels */
    icUInt8Number       clutPoints;     /* Number of clutTable grid points */
    icInt8Number        pad;            /* Padding for byte alignment */
    icS15Fixed16Number  e00;            /* e00 in the 3 * 3 */
    icS15Fixed16Number  e01;            /* e01 in the 3 * 3 */
    icS15Fixed16Number  e02;            /* e02 in the 3 * 3 */
    icS15Fixed16Number  e10;            /* e10 in the 3 * 3 */
    icS15Fixed16Number  e11;            /* e11 in the 3 * 3 */
    icS15Fixed16Number  e12;            /* e12 in the 3 * 3 */
    icS15Fixed16Number  e20;            /* e20 in the 3 * 3 */
    icS15Fixed16Number  e21;            /* e21 in the 3 * 3 */
    icS15Fixed16Number  e22;            /* e22 in the 3 * 3 */
    icUInt16Number      inputEnt;       /* Number of input table entries */
    icUInt16Number      outputEnt;      /* Number of output table entries */
    icUInt16Number      data[icAny];    /* Data follows see spec for size */
/*
 *  Data that follows is of this form
 *
 *  icUInt16Number      inputTable[inputChan][icAny];   * The input table
 *  icUInt16Number      clutTable[icAny];               * The clut table
 *  icUInt16Number      outputTable[outputChan][icAny]; * The output table
 */
} icLut16;

/* lut8, input & output tables are always 256 bytes in length */
typedef struct {
    icUInt8Number       inputChan;      /* Number of input channels */
    icUInt8Number       outputChan;     /* Number of output channels */
    icUInt8Number       clutPoints;     /* Number of clutTable grid points */
    icInt8Number        pad;
    icS15Fixed16Number  e00;            /* e00 in the 3 * 3 */
    icS15Fixed16Number  e01;            /* e01 in the 3 * 3 */
    icS15Fixed16Number  e02;            /* e02 in the 3 * 3 */
    icS15Fixed16Number  e10;            /* e10 in the 3 * 3 */
    icS15Fixed16Number  e11;            /* e11 in the 3 * 3 */
    icS15Fixed16Number  e12;            /* e12 in the 3 * 3 */
    icS15Fixed16Number  e20;            /* e20 in the 3 * 3 */
    icS15Fixed16Number  e21;            /* e21 in the 3 * 3 */
    icS15Fixed16Number  e22;            /* e22 in the 3 * 3 */
    icUInt8Number       data[icAny];    /* Data follows see spec for size */
/*
 *  Data that follows is of this form
 *
 *  icUInt8Number       inputTable[inputChan][256];     * The input table
 *  icUInt8Number       clutTable[icAny];               * The clut table
 *  icUInt8Number       outputTable[outputChan][256];   * The output table
 */
} icLut8;

/* Measurement Data */
typedef struct {
    icStandardObserver          stdObserver;    /* Standard observer */
    icXYZNumber                 backing;        /* XYZ for backing material */
    icMeasurementGeometry       geometry;       /* Measurement geometry */
    icMeasurementFlare          flare;          /* Measurement flare */
    icIlluminant                illuminant;     /* Illuminant */
} icMeasurement;

/* Named color */
typedef struct {
    icUInt32Number      vendorFlag;     /* Bottom 16 bits for IC use */
    icUInt32Number      count;          /* Count of named colors */
    icInt8Number        data[icAny];    /* Named color data follows */
/*
 *  Data that follows is of this form
 *
 * icInt8Number         prefix[icAny];  * Prefix for the color name, max = 32
 * icInt8Number         suffix[icAny];  * Suffix for the color name, max = 32
 * icInt8Number         root1[icAny];   * Root name for first color, max = 32
 * icInt8Number         coords1[icAny]; * Color co-ordinates of first color
 * icInt8Number         root2[icAny];   * Root name for first color, max = 32
 * icInt8Number         coords2[icAny]; * Color co-ordinates of first color
 *                      :
 *                      :
 * Repeat for root name and color co-ordinates up to (count-1)
 */
} icNamedColor;

/* Profile sequence structure */
typedef struct {
    icSignature                 deviceMfg;      /* Device Manufacturer */
    icSignature                 deviceModel;    /* Decvice Model */
    icUInt64Number              attributes;     /* Device attributes */
    icTechnologySignature       technology;     /* Technology signature */
    icInt8Number                data[icAny];    /* Descriptions text
follows */
/*
 *  Data that follows is of this form
 *
 * icTextDescription            deviceMfgDesc[icAny];   * Manufacturer text
 * icTextDescription            modelDesc[icAny];       * Model text
 */
} icDescStruct;

/* Profile sequence description */
typedef struct {
    icUInt32Number      count;          /* Number of descriptions */
    icDescStruct        data[icAny];    /* Array of description struct */
} icProfileSequenceDesc;

/* textDescription */
typedef struct {
    icUInt32Number      count;          /* Description length */
    icInt8Number        data[icAny];    /* Descriptions follow */
/*
 *  Data that follows is of this form
 *
 * icInt8Number         desc[icAny]     * NULL terminated ascii string
 * icUInt32Number       ucLangCode;     * UniCode language code
 * icUInt32Number       ucCount;        * UniCode description length
 * icInt8Number         ucDesc[icAny;   * The UniCode description
 * icUInt16Number       scCode;         * ScriptCode code
 * icUInt8Number        scCount;        * ScriptCode count
 * icInt8Number         scDesc[64];     * ScriptCode Description
 */
} icTextDescription;

/* Screening Data */
typedef struct {
    icS15Fixed16Number  frequency;      /* Frequency */
    icS15Fixed16Number  angle;          /* Screen angle */
    icSpotShape         spotShape;      /* Spot Shape encodings below */
} icScreeningData;

typedef struct {
    icUInt32Number      screeningFlag;  /* Screening flag */
    icUInt32Number      channels;       /* Number of channels */
    icScreeningData     data[icAny];    /* Array of screening data */
} icScreening;

/* Text Data */
typedef struct {
    icInt8Number        data[icAny];    /* Variable array of characters */
} icText;

/* Structure describing either a UCR or BG curve */
typedef struct {
    icUInt32Number      count;          /* Curve length */
    icUInt16Number      curve[icAny];   /* The array of curve values */
} icUcrBgCurve;

/* Under color removal, black generation */
typedef struct {
    icUInt8Number       data[icAny];            /* The Ucr BG data */
/*
 *  Data that follows is of this form
 *
 * icUcrBgCurve         ucr;            * Ucr curve
 * icUcrBgCurve         bg;             * Bg curve
 */
} icUcrBg;

/* viewingConditionsType */
typedef struct {
    icXYZNumber         illuminant;     /* In candelas per metre sq'd */
    icXYZNumber         surround;       /* In candelas per metre sq'd */
    icIlluminant        stdIluminant;   /* See icIlluminant defines */
} icViewingCondition;


/*------------------------------------------------------------------------*/
/*
 * Tag Type definitions
 */

/*
 * Many of the structures contain variable length arrays. This
 * is represented by the use of the convention.
 *
 *      type    data[icAny];
 */

/* The base part of each tag */
typedef struct {
    icTagTypeSignature  sig;            /* Signature */
    icInt8Number        reserved[4];    /* Reserved, set to 0 */
} icTagBase;

/* curveType */
typedef struct {
    icTagBase           base;           /* Signature, "curv" */
    icCurve             curve;          /* The curve data */
} icCurveType;

/* dataType */
typedef struct {
    icTagBase           base;           /* Signature, "data" */
    icData              data;           /* The data structure */
} icDataType;

/* dateTimeType */
typedef struct {
    icTagBase           base;           /* Signature, "dtim" */
    icDateTimeNumber    date;           /* The date */
} icDateTimeType;

/* lut16Type */
typedef struct {
    icTagBase           base;           /* Signature, "mft2" */
    icLut16             lut;            /* Lut16 data */
} icLut16Type;

/* lut8Type, input & output tables are always 256 bytes in length */
typedef struct {
    icTagBase           base;           /* Signature, "mft1" */
    icLut8              lut;            /* Lut8 data */
} icLut8Type;

/* Measurement Type */
typedef struct {
    icTagBase           base;           /* Signature, "meas" */
    icMeasurement       measurement;    /* Measurement data */
} icMeasurementType;

/* Named color type */
typedef struct {
    icTagBase           base;           /* Signature, "ncol" */
    icNamedColor        ncolor;         /* Named color data */
} icNamedColorType;

/* Profile sequence description type */
typedef struct {
    icTagBase                   base;   /* Signature, "pseq" */
    icProfileSequenceDesc       desc;   /* The seq description */
} icProfileSequenceDescType;

/* textDescriptionType */
typedef struct {
    icTagBase                   base;   /* Signature, "desc" */
    icTextDescription           desc;           /* The description */
} icTextDescriptionType;

/* s15Fixed16Type */
typedef struct {
    icTagBase           base;           /* Signature, "sf32" */
    icS15Fixed16Array   data;           /* Array of values */
} icS15Fixed16ArrayType;

typedef struct {
    icTagBase           base;           /* Signature, "scrn" */
    icScreening         screen;         /* Screening structure */
} icScreeningType;

/* sigType */
typedef struct {
    icTagBase           base;           /* Signature, "sig" */
    icSignature         signature;      /* The signature data */
} icSignatureType;

/* textType */
typedef struct {
    icTagBase           base;           /* Signature, "text" */
    icText              data;           /* Variable array of characters */
} icTextType;

/* u16Fixed16Type */
typedef struct {
    icTagBase           base;           /* Signature, "uf32" */
    icU16Fixed16Array   data;           /* Variable array of values */
} icU16Fixed16ArrayType;

/* Under color removal, black generation type */
typedef struct {
    icTagBase           base;           /* Signature, "bfd " */
    icUcrBg             data;           /* ucrBg structure */
} icUcrBgType;

/* uInt16Type */
typedef struct {
    icTagBase           base;           /* Signature, "ui16" */
    icUInt16Array       data;           /* Variable array of values */
} icUInt16ArrayType;

/* uInt32Type */
typedef struct {
    icTagBase           base;           /* Signature, "ui32" */
    icUInt32Array       data;           /* Variable array of values */
} icUInt32ArrayType;

/* uInt64Type */
typedef struct {
    icTagBase           base;           /* Signature, "ui64" */
    icUInt64Array       data;           /* Variable array of values */
} icUInt64ArrayType;

/* uInt8Type */
typedef struct {
    icTagBase           base;           /* Signature, "ui08" */
    icUInt8Array        data;           /* Variable array of values */
} icUInt8ArrayType;

/* viewingConditionsType */
typedef struct {
    icTagBase           base;           /* Signature, "view" */
    icViewingCondition  view;           /* Viewing conditions */
} icViewingConditionType;

/* XYZ Type */
typedef struct {
    icTagBase           base;           /* Signature, "XYZ" */
    icXYZArray          data;           /* Variable array of XYZ numbers */
} icXYZType;

/*------------------------------------------------------------------------*/

/*
 * Lists of tags, tags, profile header and profile strcuture
 */

/* A tag */
typedef struct {
    icTagSignature      sig;            /* The tag signature */
    icUInt32Number      offset;         /* Start of tag relative to
                                         * start of header, Spec Section 8 */
    icUInt32Number      size;           /* Size in bytes */
} icTag;

/* A Structure that may be used independently for a list of tags */
typedef struct {
    icUInt32Number      count;          /* Number of tags in the profile */
    icTag               tags[icAny];    /* Variable array of tags */
} icTagList;

/* The Profile header */
typedef struct {
    icUInt32Number              size;           /* Profile size in bytes */
    icSignature                 cmmId;          /* CMM for this profile */
    icUInt32Number              version;        /* Format version number */
    icProfileClassSignature     deviceClass;    /* Type of profile */
    icColorSpaceSignature       colorSpace;     /* Color space of data */
    icColorSpaceSignature       pcs;            /* PCS, XYZ or Lab only */
    icDateTimeNumber            date;           /* Date profile was created */
    icSignature                 magic;          /* icMagicNumber */
    icPlatformSignature         platform;       /* Primary Platform */
    icUInt32Number              flags;          /* Various bit settings */
    icSignature                 manufacturer;   /* Device manufacturer */
    icUInt32Number              model;          /* Device model number */
    icUInt64Number              attributes;     /* Device attributes */
    icUInt32Number              renderingIntent;/* Rendering intent */
    icXYZNumber                 illuminant;     /* Profile illuminant */
    icInt8Number                reserved[48];   /* Reserved for future use */
} icHeader;

/*
 * A profile,
 * we can't use icTagList here because its not at the end of the structure
 */
typedef struct {
    icHeader            header;         /* The header */
    icUInt32Number      count;          /* Number of tags in the profile */
    icInt8Number        data[icAny];    /* The tagTable and tagData */
/*
 * Data that follows is of the form
 *
 * icTag        tagTable[icAny];        * The tag table
 * icInt8Number tagData[icAny];         * The tag data
 */
} icProfile;    

/*------------------------------------------------------------------------*/
#endif

