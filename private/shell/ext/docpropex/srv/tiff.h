/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tiff.h

Abstract:

    This file the data structures and constant
    definitions for the TIFF file format.
    See the TIFF specification Revision 6.0,
    dated 6-3-92, from Adobe for specific details.

Environment:

        WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/


#ifndef _TIFF_
#define _TIFF_

#define TIFF_VERSION    42

#define TIFF_BIGENDIAN          0x4d4d
#define TIFF_LITTLEENDIAN       0x4949

#pragma pack(1)

typedef struct _TIFF_HEADER {
    WORD        Identifier;
    WORD        Version;
    DWORD       IFDOffset;
} TIFF_HEADER, *PTIFF_HEADER;

//
// TIFF Image File Directories are comprised of
// a table of field descriptors of the form shown
// below.  The table is sorted in ascending order
// by tag.  The values associated with each entry
// are disjoint and may appear anywhere in the file
// (so long as they are placed on a word boundary).
//
// If the value is 4 bytes or less, then it is placed
// in the offset field to save space.  If the value
// is less than 4 bytes, it is left-justified in the
// offset field.
//
typedef struct _TIFF_TAG {
    WORD        TagId;
    WORD        DataType;
    DWORD       DataCount;
    DWORD       DataOffset;
} TIFF_TAG;

typedef TIFF_TAG UNALIGNED *PTIFF_TAG;

typedef struct TIFF_IFD {
    WORD        wEntries;
    TIFF_TAG    ifdEntries[1];
} TIFF_IFD;

typedef TIFF_IFD UNALIGNED *PTIFF_IFD;


#pragma pack()


//
// NB: In the comments below,
//  - items marked with a + are obsoleted by revision 5.0,
//  - items marked with a ! are introduced in revision 6.0.
//  - items marked with a % are introduced post revision 6.0.
//  - items marked with a $ are obsoleted by revision 6.0.
//

//
// Tag data type information.
//
#define TIFF_NOTYPE                     0       // placeholder
#define TIFF_BYTE                       1       // 8-bit unsigned integer
#define TIFF_ASCII                      2       // 8-bit bytes w/ last byte null
#define TIFF_SHORT                      3       // 16-bit unsigned integer
#define TIFF_LONG                       4       // 32-bit unsigned integer
#define TIFF_RATIONAL                   5       // 64-bit unsigned fraction
#define TIFF_SBYTE                      6       // !8-bit signed integer
#define TIFF_UNDEFINED                  7       // !8-bit untyped data
#define TIFF_SSHORT                     8       // !16-bit signed integer
#define TIFF_SLONG                      9       // !32-bit signed integer
#define TIFF_SRATIONAL                  10      // !64-bit signed fraction
#define TIFF_FLOAT                      11      // !32-bit IEEE floating point
#define TIFF_DOUBLE                     12      // !64-bit IEEE floating point

//
// TIFF Tag Definitions.
//
#define TIFFTAG_SUBFILETYPE             254     // subfile data descriptor
#define     FILETYPE_REDUCEDIMAGE       0x1     // reduced resolution version
#define     FILETYPE_PAGE               0x2     // one page of many
#define     FILETYPE_MASK               0x4     // transparency mask
#define TIFFTAG_OSUBFILETYPE            255     // +kind of data in subfile
#define     OFILETYPE_IMAGE             1       // full resolution image data
#define     OFILETYPE_REDUCEDIMAGE      2       // reduced size image data
#define     OFILETYPE_PAGE              3       // one page of many
#define TIFFTAG_IMAGEWIDTH              256     // image width in pixels
#define TIFFTAG_IMAGELENGTH             257     // image height in pixels
#define TIFFTAG_BITSPERSAMPLE           258     // bits per channel (sample)
#define TIFFTAG_COMPRESSION             259     // data compression technique
#define     COMPRESSION_NONE            1       // dump mode
#define     COMPRESSION_CCITTRLE        2       // CCITT modified Huffman RLE
#define     COMPRESSION_CCITTFAX3       3       // CCITT Group 3 fax encoding
#define     COMPRESSION_CCITTFAX4       4       // CCITT Group 4 fax encoding
#define     COMPRESSION_LZW             5       // Lempel-Ziv  & Welch
#define     COMPRESSION_OJPEG           6       // !6.0 JPEG
#define     COMPRESSION_JPEG            7       // %JPEG DCT compression
#define     COMPRESSION_NEXT            32766   // NeXT 2-bit RLE
#define     COMPRESSION_CCITTRLEW       32771   // #1 w/ word alignment
#define     COMPRESSION_PACKBITS        32773   // Macintosh RLE
#define     COMPRESSION_THUNDERSCAN     32809   // ThunderScan RLE
//
// compression codes 32908-32911 are reserved for Pixar
//
#define     COMPRESSION_PIXARFILM       32908   // Pixar companded 10bit LZW
#define     COMPRESSION_DEFLATE         32946   // Deflate compression
#define     COMPRESSION_JBIG            34661   // ISO JBIG
#define TIFFTAG_PHOTOMETRIC             262     // photometric interpretation
#define     PHOTOMETRIC_MINISWHITE      0       // min value is white
#define     PHOTOMETRIC_MINISBLACK      1       // min value is black
#define     PHOTOMETRIC_RGB             2       // RGB color model
#define     PHOTOMETRIC_PALETTE         3       // color map indexed
#define     PHOTOMETRIC_MASK            4       // $holdout mask
#define     PHOTOMETRIC_SEPARATED       5       // !color separations
#define     PHOTOMETRIC_YCBCR           6       // !CCIR 601
#define     PHOTOMETRIC_CIELAB          8       // !1976 CIE L*a*b*
#define TIFFTAG_THRESHHOLDING           263     // +thresholding used on data
#define     THRESHHOLD_BILEVEL          1       // b&w art scan
#define     THRESHHOLD_HALFTONE         2       // or dithered scan
#define     THRESHHOLD_ERRORDIFFUSE     3       // usually floyd-steinberg
#define TIFFTAG_CELLWIDTH               264     // +dithering matrix width
#define TIFFTAG_CELLLENGTH              265     // +dithering matrix height
#define TIFFTAG_FILLORDER               266     // data order within a byte
#define     FILLORDER_MSB2LSB           1       // most significant -> least
#define     FILLORDER_LSB2MSB           2       // least significant -> most
#define TIFFTAG_DOCUMENTNAME            269     // name of doc. image is from
#define TIFFTAG_IMAGEDESCRIPTION        270     // info about image
#define TIFFTAG_MAKE                    271     // scanner manufacturer name
#define TIFFTAG_MODEL                   272     // scanner model name/number
#define TIFFTAG_STRIPOFFSETS            273     // offsets to data strips
#define TIFFTAG_ORIENTATION             274     // +image orientation
#define     ORIENTATION_TOPLEFT         1       // row 0 top, col 0 lhs
#define     ORIENTATION_TOPRIGHT        2       // row 0 top, col 0 rhs
#define     ORIENTATION_BOTRIGHT        3       // row 0 bottom, col 0 rhs
#define     ORIENTATION_BOTLEFT         4       // row 0 bottom, col 0 lhs
#define     ORIENTATION_LEFTTOP         5       // row 0 lhs, col 0 top
#define     ORIENTATION_RIGHTTOP        6       // row 0 rhs, col 0 top
#define     ORIENTATION_RIGHTBOT        7       // row 0 rhs, col 0 bottom
#define     ORIENTATION_LEFTBOT         8       // row 0 lhs, col 0 bottom
#define TIFFTAG_SAMPLESPERPIXEL         277     // samples per pixel
#define TIFFTAG_ROWSPERSTRIP            278     // rows per strip of data
#define TIFFTAG_STRIPBYTECOUNTS         279     // bytes counts for strips
#define TIFFTAG_MINSAMPLEVALUE          280     // +minimum sample value
#define TIFFTAG_MAXSAMPLEVALUE          281     // +maximum sample value
#define TIFFTAG_XRESOLUTION             282     // pixels/resolution in x
#define TIFFTAG_YRESOLUTION             283     // pixels/resolution in y
#define TIFFTAG_PLANARCONFIG            284     // storage organization
#define     PLANARCONFIG_CONTIG         1       // single image plane
#define     PLANARCONFIG_SEPARATE       2       // separate planes of data
#define TIFFTAG_PAGENAME                285     // page name image is from
#define TIFFTAG_XPOSITION               286     // x page offset of image lhs
#define TIFFTAG_YPOSITION               287     // y page offset of image lhs
#define TIFFTAG_FREEOFFSETS             288     // +byte offset to free block
#define TIFFTAG_FREEBYTECOUNTS          289     // +sizes of free blocks
#define TIFFTAG_GRAYRESPONSEUNIT        290     // $gray scale curve accuracy
#define     GRAYRESPONSEUNIT_10S        1       // tenths of a unit
#define     GRAYRESPONSEUNIT_100S       2       // hundredths of a unit
#define     GRAYRESPONSEUNIT_1000S      3       // thousandths of a unit
#define     GRAYRESPONSEUNIT_10000S     4       // ten-thousandths of a unit
#define     GRAYRESPONSEUNIT_100000S    5       // hundred-thousandths
#define TIFFTAG_GRAYRESPONSECURVE       291     // $gray scale response curve
#define TIFFTAG_GROUP3OPTIONS           292     // 32 flag bits
#define     GROUP3OPT_2DENCODING        0x1     // 2-dimensional coding
#define     GROUP3OPT_UNCOMPRESSED      0x2     // data not compressed
#define     GROUP3OPT_FILLBITS          0x4     // fill to byte boundary
#define TIFFTAG_GROUP4OPTIONS           293     // 32 flag bits
#define     GROUP4OPT_UNCOMPRESSED      0x2     // data not compressed
#define TIFFTAG_RESOLUTIONUNIT          296     // units of resolutions
#define     RESUNIT_NONE                1       // no meaningful units
#define     RESUNIT_INCH                2       // english
#define     RESUNIT_CENTIMETER          3       // metric
#define TIFFTAG_PAGENUMBER              297     // page numbers of multi-page
#define TIFFTAG_COLORRESPONSEUNIT       300     // $color curve accuracy
#define     COLORRESPONSEUNIT_10S       1       // tenths of a unit
#define     COLORRESPONSEUNIT_100S      2       // hundredths of a unit
#define     COLORRESPONSEUNIT_1000S     3       // thousandths of a unit
#define     COLORRESPONSEUNIT_10000S    4       // ten-thousandths of a unit
#define     COLORRESPONSEUNIT_100000S   5       // hundred-thousandths
#define TIFFTAG_TRANSFERFUNCTION        301     // !colorimetry info
#define TIFFTAG_SOFTWARE                305     // name & release
#define TIFFTAG_DATETIME                306     // creation date and time
#define TIFFTAG_ARTIST                  315     // creator of image
#define TIFFTAG_HOSTCOMPUTER            316     // machine where created
#define TIFFTAG_PREDICTOR               317     // prediction scheme w/ LZW
#define TIFFTAG_WHITEPOINT              318     // image white point
#define TIFFTAG_PRIMARYCHROMATICITIES   319     // !primary chromaticities
#define TIFFTAG_COLORMAP                320     // RGB map for pallette image
#define TIFFTAG_HALFTONEHINTS           321     // !highlight+shadow info
#define TIFFTAG_TILEWIDTH               322     // !rows/data tile
#define TIFFTAG_TILELENGTH              323     // !cols/data tile
#define TIFFTAG_TILEOFFSETS             324     // !offsets to data tiles
#define TIFFTAG_TILEBYTECOUNTS          325     // !byte counts for tiles
#define TIFFTAG_BADFAXLINES             326     // lines w/ wrong pixel count
#define TIFFTAG_CLEANFAXDATA            327     // regenerated line info
#define     CLEANFAXDATA_CLEAN          0       // no errors detected
#define     CLEANFAXDATA_REGENERATED    1       // receiver regenerated lines
#define     CLEANFAXDATA_UNCLEAN        2       // uncorrected errors exist
#define TIFFTAG_CONSECUTIVEBADFAXLINES  328     // max consecutive bad lines
#define TIFFTAG_SUBIFD                  330     // subimage descriptors
#define TIFFTAG_INKSET                  332     // !inks in separated image
#define     INKSET_CMYK                 1       // !cyan-magenta-yellow-black
#define TIFFTAG_INKNAMES                333     // !ascii names of inks
#define TIFFTAG_DOTRANGE                336     // !0% and 100% dot codes
#define TIFFTAG_TARGETPRINTER           337     // !separation target
#define TIFFTAG_EXTRASAMPLES            338     // !info about extra samples
#define     EXTRASAMPLE_UNSPECIFIED     0       // !unspecified data
#define     EXTRASAMPLE_ASSOCALPHA      1       // !associated alpha data
#define     EXTRASAMPLE_UNASSALPHA      2       // !unassociated alpha data
#define TIFFTAG_SAMPLEFORMAT            339     // !data sample format
#define     SAMPLEFORMAT_UINT           1       // !unsigned integer data
#define     SAMPLEFORMAT_INT            2       // !signed integer data
#define     SAMPLEFORMAT_IEEEFP         3       // !IEEE floating point data
#define     SAMPLEFORMAT_VOID           4       // !untyped data
#define TIFFTAG_SMINSAMPLEVALUE         340     // !variable MinSampleValue
#define TIFFTAG_SMAXSAMPLEVALUE         341     // !variable MaxSampleValue
#define TIFFTAG_JPEGTABLES              347     // %JPEG table stream
//
// Tags 512-521 are obsoleted by Technical Note #2
// which specifies a revised JPEG-in-TIFF scheme.
//
#define TIFFTAG_JPEGPROC                512     // !JPEG processing algorithm
#define     JPEGPROC_BASELINE           1       // !baseline sequential
#define     JPEGPROC_LOSSLESS           14      // !Huffman coded lossless
#define TIFFTAG_JPEGIFOFFSET            513     // !pointer to SOI marker
#define TIFFTAG_JPEGIFBYTECOUNT         514     // !JFIF stream length
#define TIFFTAG_JPEGRESTARTINTERVAL     515     // !restart interval length
#define TIFFTAG_JPEGLOSSLESSPREDICTORS  517     // !lossless proc predictor
#define TIFFTAG_JPEGPOINTTRANSFORM      518     // !lossless point transform
#define TIFFTAG_JPEGQTABLES             519     // !Q matrice offsets
#define TIFFTAG_JPEGDCTABLES            520     // !DCT table offsets
#define TIFFTAG_JPEGACTABLES            521     // !AC coefficient offsets
#define TIFFTAG_YCBCRCOEFFICIENTS       529     // !RGB -> YCbCr transform
#define TIFFTAG_YCBCRSUBSAMPLING        530     // !YCbCr subsampling factors
#define TIFFTAG_YCBCRPOSITIONING        531     // !subsample positioning
#define     YCBCRPOSITION_CENTERED      1       // !as in PostScript Level 2
#define     YCBCRPOSITION_COSITED       2       // !as in CCIR 601-1
#define TIFFTAG_REFERENCEBLACKWHITE     532     // !colorimetry info
//
// tags 32952-32956 are private tags registered to Island Graphics
//
#define TIFFTAG_REFPTS                  32953   // image reference points
#define TIFFTAG_REGIONTACKPOINT         32954   // region-xform tack point
#define TIFFTAG_REGIONWARPCORNERS       32955   // warp quadrilateral
#define TIFFTAG_REGIONAFFINE            32956   // affine transformation mat
//
// tags 32995-32999 are private tags registered to SGI
//
#define TIFFTAG_MATTEING                32995   // $use ExtraSamples
#define TIFFTAG_DATATYPE                32996   // $use SampleFormat
#define TIFFTAG_IMAGEDEPTH              32997   // z depth of image
#define TIFFTAG_TILEDEPTH               32998   // z depth/data tile
//
// tags 33300-33309 are private tags registered to Pixar
//
// TIFFTAG_PIXAR_IMAGEFULLWIDTH and TIFFTAG_PIXAR_IMAGEFULLLENGTH
// are set when an image has been cropped out of a larger image.
// They reflect the size of the original uncropped image.
// The TIFFTAG_XPOSITION and TIFFTAG_YPOSITION can be used
// to determine the position of the smaller image in the larger one.
//
#define TIFFTAG_PIXAR_IMAGEFULLWIDTH    33300   // full image size in x
#define TIFFTAG_PIXAR_IMAGEFULLLENGTH   33301   // full image size in y
//
// tag 33432 is listed in the 6.0 spec w/ unknown ownership
//
#define TIFFTAG_COPYRIGHT               33432   // copyright string
//
// tags 34232-34236 are private tags registered to Texas Instruments
//
#define TIFFTAG_FRAMECOUNT              34232   // Sequence Frame Count
//
// tag 34750 is a private tag registered to Pixel Magic
//
#define TIFFTAG_JBIGOPTIONS             34750   // JBIG options
//
// tags 34908-34914 are private tags registered to SGI
//
#define TIFFTAG_FAXRECVPARAMS           34908   // encoded Class 2 ses. parms
#define TIFFTAG_FAXSUBADDRESS           34909   // received SubAddr string
#define TIFFTAG_FAXRECVTIME             34910   // receive time (secs)
//
// tags 40001-40100 are private tags registered to ms
//
#define TIFFTAG_RECIP_NAME              40001
#define TIFFTAG_RECIP_NUMBER            40002
#define TIFFTAG_SENDER_NAME             40003
#define TIFFTAG_ROUTING                 40004
#define TIFFTAG_CALLERID                40005
#define TIFFTAG_TSID                    40006
#define TIFFTAG_CSID                    40007
#define TIFFTAG_FAX_TIME                40008
//
// The following are ``pseudo tags'' that can be
// used to control codec-specific functionality.
// These tags are not written to file.  Note that
// these values start at 0xffff+1 so that they'll
// never collide with Aldus-assigned tags.
//
// If you want your private pseudo tags ``registered''
// (i.e. added to this file), send mail to sam@sgi.com
// with the appropriate C definitions to add.
//
#define TIFFTAG_FAXMODE                 65536   // Group 3/4 format control
#define     FAXMODE_CLASSIC     0x0000          // default, include RTC
#define     FAXMODE_NORTC       0x0001          // no RTC at end of data
#define     FAXMODE_NOEOL       0x0002          // no EOL code at end of row
#define     FAXMODE_BYTEALIGN   0x0004          // byte align row
#define     FAXMODE_WORDALIGN   0x0008          // word align row
#define     FAXMODE_CLASSF      FAXMODE_NORTC   // TIFF Class F
#define TIFFTAG_JPEGQUALITY             65537   // Compression quality level
//
// Note: quality level is on the IJG 0-100 scale.  Default value is 75
//
#define TIFFTAG_JPEGCOLORMODE           65538   // Auto RGB<=>YCbCr convert?
#define     JPEGCOLORMODE_RAW   0x0000          // no conversion (default)
#define     JPEGCOLORMODE_RGB   0x0001          // do auto conversion
#define TIFFTAG_JPEGTABLESMODE          65539   // What to put in JPEGTables
#define     JPEGTABLESMODE_QUANT 0x0001         // include quantization tbls
#define     JPEGTABLESMODE_HUFF 0x0002          // include Huffman tbls
//
// Note: default is JPEGTABLESMODE_QUANT | JPEGTABLESMODE_HUFF
//
#define TIFFTAG_FAXFILLFUNC             65540   // G3/G4 fill function
#define TIFFTAG_PIXARLOGDATAFMT         65549   // PixarLogCodec I/O data sz
#define     PIXARLOGDATAFMT_8BIT        0       // regular u_char samples
#define     PIXARLOGDATAFMT_8BITABGR    1       // ABGR-order u_chars
#define     PIXARLOGDATAFMT_10BITLOG    2       // 10-bit log-encoded (raw)
#define     PIXARLOGDATAFMT_12BITPICIO  3       // as per PICIO (1.0==2048)
#define     PIXARLOGDATAFMT_16BIT       4       // signed short samples
#define     PIXARLOGDATAFMT_FLOAT       5       // IEEE float samples

#endif
