//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       cabinet.h
//
//--------------------------------------------------------------------------

/***    cabinet.h - Definitions for Cabinet File structure
 *
 *  Author:
 *      Benjamin W. Slivka
 *
 *  History:
 *      15-Aug-1993 bens        Initial version
 *      05-Sep-1993 bens        Added Overview section
 *      29-Nov-1993 chuckst     Added disk names to folder first & next
 *                              Used "CF" consistently
 *                              Eliminated redundant cch fields
 *      09-Feb-1994 chuckst     merged in some related global constants
 *      09-Mar-1994 bens        Add RESERVE defintions (for encryption)
 *      17-Mar-1994 bens        Improve comments about split CFDATA structures
 *      25-Mar-1994 bens        Add cabinet set ID
 *      13-May-1994 bens        Define bad value for iCabinet
 *      15-Jun-1997 pberkman    added CABSignatureStruct_
 *
 *  Overview:
 *      This file contains definitions for the Diamond Cabinet File format.
 *      A Cabinet File exists to store one or more files.  Usually these
 *      files have been compressed, but that is not required.  It is also
 *      possible for a cabinet file to contain only a portion of a larger
 *      file.
 *
 *      In designing this format, the following goals where achieved:
 *      1) Minimize overhead in the CF format
 *         ==> Where ever possible BYTEs or USHORTs were used, rather
 *             than using LONGs, even though the latter would be easier
 *             to manipulate on certain RISC platforms.
 *      2) Support little-endian and big-endian byte ordering.
 *         ==> For simplicity on x86 systems, multi-byte numbers are
 *             stored in a little-endian form, but the code to read and
 *             write these numbers operates correctly on either type of
 *             computer.
 *
 *      A cabinet file contains the following structures in the following
 *      order:
 *          Name            Description
 *          -----------     -------------------
 *          CFHEADER        Cabinet description
 *            [CFRESERVE]   Optional RESERVED control information in CFHEADER
 *          CFFOLDER(s)     Folder descriptions
 *            [reserved]    Optional RESERVED data per folder
 *          CFFILE(s)       File descriptions
 *          CFDATA(s)       Data blocks
 *            [reserved]    Optional RESERVED data per data block
 *
 *  Data Integrity Strategy:
 *      The Cabinet File has built-in data integrity checks, since it is
 *      possible for customers to have damaged diskettes, or for accidental
 *      or malicious damage to occur.  Rather than doing an individual
 *      checksum for the entire cabinet file (which would have a dramatic
 *      impact on the speed of installation from floppy disk, since the
 *      entire file would need to be read), we have per-component
 *      checksums, and compute and check them as we read the various
 *      components of the file.
 *
 *      1)  Checksum CFHEADER
 *      2)  Store cabinet file length in CFHEADER (to detect file truncation)
 *      3)  Checksum entire set of CFFOLDER structures
 *      4)  Checksum entire set of CFFILE structures
 *      5)  Checksum each (compressed) data block independantly
 *
 *      This approach allows us to avoid reading unnecessary parts of the
 *      file cabinet (though reading all of CFFOLDER and CFFILE structures
 *      would otherwise not be required in all cases), while still providing
 *      adequate integrity checking.
 */

#ifndef INCLUDED_CABINET
#define INCLUDED_CABINET 1

typedef unsigned long CHECKSUM;
typedef unsigned long COFF;
typedef unsigned long UOFF;

//** Pack structures tightly in cabinet files!
#pragma pack(1)


/***    verCF - Cabinet File format version
 *
 *      The low-order byte is interpreted as a decimal number for the minor
 *      (1/100ths) portion of the version number.
 *      The high-order byte is interpreted as a decimal number for the major
 *      portion of the version number.
 *
 *      Examples:
 *          0x0000  0.00
 *          0x010A  1.10
 *          0x0410  4.16
 *
 *      History:
 *          1.01    Original
 *          1.02    Added flags field, changed signature
 *          1.03    Added setId,iCabinet so FDI can ensure correct cabinet
 *                      on continuation.
 */
#define verCF           0x0103      // CF version 1.03


/***    Various cabinet file limits
 *
 */
#define cMAX_FOLDERS_PER_CABINET    (ifoldMASK-1)
#define cMAX_FILES_PER_CABINET      65535


/***    cbRESERVE_XXX_MAX - Maximum size of RESERVE sections
 *
 *  NOTE: cbRESERVE_HEADER_MAX is a fair bit less than 64K because in
 *        the 16-bit version of this code, we want to have a USHORT
 *        variable that holds the size of the CFHEADER structure +
 *        the size of the CFRESERVE structure + the size of the per-header
 *        reserved data.
 */
//BUGBUG 16-Mar-1994 bens Define better bound for cbRESERVE_HEADER_MAX
#define cbRESERVE_HEADER_MAX        60000   // Fits in a USHORT
#define cbRESERVE_FOLDER_MAX          255   // Fits in a BYTE
#define cbRESERVE_DATA_MAX            255   // Fits in a BYTE


/***    ifoldXXXX - Special values for CFFILE.iFolder
 *
 */
#define ifoldMASK                    0xFFFC  // Low two bits zero
#define ifoldCONTINUED_FROM_PREV     0xFFFD
#define ifoldCONTINUED_TO_NEXT       0xFFFE
#define ifoldCONTINUED_PREV_AND_NEXT 0xFFFF

#define IS_CONTD_FORWARD(ifold) ((ifold & 0xfffe) == ifoldCONTINUED_TO_NEXT)
#define IS_CONTD_BACK(ifold) ((ifold & 0xfffd) == ifoldCONTINUED_FROM_PREV)


#ifndef MAKESIG
/***    MAKESIG - Construct a 4 byte signature
 *
 *  Entry:
 *      ch1,ch2,ch3,ch4 - four characters
 *
 *  Exit:
 *      returns unsigned long
 */
#define MAKESIG(ch1,ch2,ch3,ch4)          \
          (  ((unsigned long)ch1)      +  \
            (((unsigned long)ch2)<< 8) +  \
            (((unsigned long)ch3)<<16) +  \
            (((unsigned long)ch4)<<24) )
#endif // !MAKESIG


#define sigCFHEADER MAKESIG('M','S','C','F')  // CFHEADER signature


/***    cfhdrXXX - bit flags for cfheader.flags field
 *
 */
#define cfhdrPREV_CABINET       0x0001  // Set if previous cab/disk present
#define cfhdrNEXT_CABINET       0x0002  // Set if next cab/disk present
#define cfhdrRESERVE_PRESENT    0x0004  // Set if RESERVE_CONTROL is present


/***    CFHEADER - Cabinet File Header
 *
 */
typedef struct {
//**    LONGs are first, to ensure alignment
    long        sig;            // Cabinet File identification string
    CHECKSUM    csumHeader;     // Structure checksum (excluding csumHeader!)
    long        cbCabinet;      // Total length of file (consistency check)
    CHECKSUM    csumFolders;    // Checksum of CFFOLDER list
    COFF        coffFiles;      // Location in cabinet file of CFFILE list
    CHECKSUM    csumFiles;      // Checksum of CFFILE list

//**    SHORTs are next, to ensure alignment
    USHORT      version;        // Cabinet File version (verCF)
    USHORT      cFolders;       // Count of folders (CFIFOLDERs) in cabinet
    USHORT      cFiles;         // Count of files (CFIFILEs) in cabinet
    USHORT      flags;          // Flags to indicate optional data presence
    USHORT      setID;          // Cabinet set ID (identifies set of cabinets)
    USHORT      iCabinet;       // Cabinet number in set (0 based)
#define iCABINET_BAD    0xFFFF  // Illegal number for iCabinet

//**    If flags has the cfhdrRESERVE_PRESENT bit set, then a CFRESERVE
//      structure appears here, followed possibly by some CFHEADER reserved
//      space. The CFRESERVE structure has fields to define how much reserved
//      space is present in the CFHEADER, CFFOLDER, and CFDATA structures.
//      If CFRESERVE.cbCFHeader is non-zero, then abReserve[] immediately
//      follows the CFRESERVE structure.  Note that all of these sizes are
//      multiples of 4 bytes, to ensure structure alignment!
//
//  CFRESERVE   cfres;          // Reserve information
//  BYTE        abReserve[];    // Reserved data space
//

//**    The following fields presence depends upon the settings in the flags
//      field above.  If cfhdrPREV_CABINET is set, then there are two ASCIIZ
//      strings to describe the previous disk and cabinet.
//
//      NOTE: This "previous" cabinet is not necessarily the immediately
//            preceding cabinet!  While it usually will be, if a file is
//            continued into the current cabinet, then the "previous"
//            cabinet identifies the cabinet where the folder that contains
//            this file *starts*!  For example, if EXCEL.EXE starts in
//            cabinet excel.1 and is continued through excel.2 to excel.3,
//            then cabinet excel.3 will point back to *cabinet.1*, since
//            that is where you have to start in order to extract EXCEL.EXE.
//
//  char    szCabinetPrev[];    // Prev Cabinet filespec
//  char    szDiskPrev[];       // Prev descriptive disk name
//
//      Similarly, If cfhdrNEXT_CABINET is set, then there are two ASCIIZ
//      strings to describe the next disk and cabinet:
//
//  char    szCabinetNext[];    // Next Cabinet filespec
//  char    szDiskNext[];       // Next descriptive disk name
//
} CFHEADER; /* cfheader */


/***    CFRESERVE - Cabinet File Reserved data information
 *
 *  This structure is present in the middle of the CFHEADER structure if
 *  CFHEADER.flags has the cfhdrRESERVE_PRESENT bit set.  This structure
 *  defines the sizes of all the reserved data sections in the CFHEADER,
 *  CFFOLDER, and CFDATA structures.
 *
 *  These reserved sizes can be zero (although it would be silly to have
 *  all of them be zero), but otherwise must be a multiple of 4, to ensure
 *  structure alignment for RISC machines.
 */
typedef struct {
    USHORT  cbCFHeader;         // Size of abReserve in CFHEADER structure
    BYTE    cbCFFolder;         // Size of abReserve in CFFOLDER structure
    BYTE    cbCFData;           // Size of abReserve in CFDATA   structure
} CFRESERVE; /* cfreserve */

#define cbCF_HEADER_BAD     0xFFFF      // Bad value for CFRESERVE.cbCFHeader

//
//  the following struct identifies the content of the signature area
//  of abReserved for Athenticode version 2.
//
typedef struct CABSignatureStruct_
{
    DWORD       cbFileOffset;
    DWORD       cbSig;
    BYTE        Filler[8];
} CABSignatureStruct_;



/***    CFFOLDER - Cabinet Folder
 *
 *  This structure describes a partial or complete "compression unit".
 *  A folder is by definition a stream of compressed data.  To retrieve
 *  an uncompressed data from a folder, you *must* start decompressing
 *  the data at the start of the folder, regardless of how far into the
 *  folder the data you want actually starts.
 *
 *  Folders may start in one cabinet, and continue on to one or more
 *  succeeding cabinets.  In general, if a folder has been continued over
 *  a cabinet boundary, Diamond/FCI will terminate that folder as soon as
 *  the current file has been completely compressed.  Generally this means
 *  that a folder would span at most two cabinets, but if the file is really
 *  large, it could span more than two cabinets.
 *
 *  Note: CFFOLDERs actually refer to folder *fragments*, not necessarily
 *        complete folders.  You know that a CFFOLDER is the beginning of a
 *        folder (as opposed to a continuation in a subsequent cabinet file)
 *        if a file starts in it (i.e., the CFFILE.uoffFolderStart field is
 *        0).
 */
typedef struct {
    COFF    coffCabStart;       // Offset in cabinet file of first CFDATA
                                // block for this folder.

    USHORT  cCFData;            // Count of CFDATAs for this folder that
                                //  are actually in this cabinet.  Note that
                                //  a folder can continue into another cabinet
                                //  and have many more CFDATA blocks in that
                                //  cabinet, *and* a folder may have started
                                //  in a previous cabinet.  This count is
                                //  only of CFDATAs for this folder that are
                                //  (at least partially) in this cabinet.

    short   typeCompress;       // Indicates compression type for all CFDATA
                                //   blocks for this folder.  The valid values
                                //   are defined in the types.h built into
                                //   fci.h/fdi.h.

//**    If CFHEADER.flags has the cfhdrRESERVE_PRESENT bit set, and
//      CFRESERVE.cbCFFolder is non-zero, then abReserve[] appears here.
//
//  BYTE    abReserve[];        // Reserved data space
//
} CFFOLDER; /* cffolder */



/***    CFFILE - Cabinet File structure describing a single file in the cabinet
 *
 *  NOTE: iFolder is used to indicatate continuation cases, so we have to
 *        calculate the real iFolder by examining the cabinet files:
 *
 *        ifoldCONTINUED_FROM_PREV
 *            This file ends in this cabinet, but is continued from a
 *            previous cabinet.  Therefore, the portion of the file contained
 *            in *this* cabinet *must* start in the first folder.
 *
 *            NOTE: szCabinetPrev is the name of the cabinet where this file
 *                  *starts*, which is not necessarily the immediately
 *                  preceeding cabinet!  Since it only makes sense to
 *                  decompress a file from its start, the starting cabinet
 *                  is what is important!
 *
 *        ifoldCONTINUED_TO_NEXT
 *            This file starts in this cabinet, but is continued to the next
 *            cabinet.  Therfore, this file must start in the *last* folder
 *            in this cabinet.
 *
 *        ifoldCONTINUED_PREV_AND_NEXT
 *            This file is the *middle* portion of a file that started in a
 *            previous cabinet and is continued in the next cabinet.  Since
 *            this cabinet only contain this piece of a single file, there
 *            is only a single folder fragment in this cabinet.
 */
typedef struct {
    long    cbFile;             // Uncompressed size of file

    UOFF    uoffFolderStart;    // Offset in folder IN UNCOMPRESSED BYTES
                                //  of the start of this file

    USHORT  iFolder;            // Index of folder containing this file;
                                //  0 is first folder in this cabinet.
                                //  See ifoldCONTINUED_XXXX values above
                                //  for treatment of continuation files.

    USHORT  date;               // Date stamp in FAT file system format

    USHORT  time;               // Time stamp in FAT file system format

    USHORT  attribs;            // Attribute in FAT file system format

//  char    szName[];           // File name (may include path characters)
} CFFILE; /* cffile */


/***    CFDATA - Cabinet File structure describing a data block
 *
 */
typedef struct {
    CHECKSUM    csum;           // Checksum (excluding this field itself!)
                                //  of this structure and the data that
                                //  follows.  If this CFDATA structure is
                                //  continued to the next cabinet, then
                                //  the value of this field is ignored
                                //  (and set to zero).

    USHORT      cbData;         // Size of ab[] data that resides in the
                                //  current cabinet.  A CFDATA may be split
                                //  across a cabinet boundary, so this
                                //  value indicates only the amount of data
                                //  store in this cabinet.

    USHORT      cbUncomp;       // Uncompressed size of ab[] data; if this
                                //  CFDATA block is continued to the next
                                //  cabinet, then this value is zero!
                                //  If this CFDATA block the remainder of
                                //  of a CFDATA block that started in the
                                //  previous cabinet, then this value is
                                //  the total size of the uncompressed data
                                //  represented by the two CFDATA blocks!

//**    If CFHEADER.flags has the cfhdrRESERVE_PRESENT bit set, and
//      CFRESERVE.cbCFData is non-zero, then abReserve[] appears here.
//
//  BYTE        abReserve[];    // Reserved data space
//

//**    The actual data follows here, cbData bytes in length.
//
//  BYTE        ab[];           // Data
//
} CFDATA; /* cfdata */



//** Attribute Bit to use for Run after extract
#define  RUNATTRIB  0x40


//** Revert to default structure packing!
#pragma pack()

#endif // !INCLUDED_CABINET
