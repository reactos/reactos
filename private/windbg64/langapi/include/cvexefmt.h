/***    cvexefmt.h - format of CodeView information in exe
 *
 *      Structures, constants, etc. for reading CodeView information
 *      from the executable.
 *
 */


/***    The master copy of this file resides in the LANGAPI project.
 *      All Microsoft projects are required to use the master copy without
 *      modification.  Modification of the master version or a copy
 *      without consultation with all parties concerned is extremely
 *      risky.
 *
 */


#ifndef _VC_VER_INC
#include "..\include\vcver.h"
#endif



//  The following structures and constants describe the format of the
//  CodeView Debug OMF for that will be accepted by CodeView 4.0 and
//  later.  These are executables with signatures of NB05, NB06 and NB08.
//  There is some confusion about the signatures NB03 and NB04 so none
//  of the utilites will accept executables with these signatures.  NB07 is
//  the signature for QCWIN 1.0 packed executables.

//  All of the structures described below must start on a long word boundary
//  to maintain natural alignment.  Pad space can be inserted during the
//  write operation and the addresses adjusted without affecting the contents
//  of the structures.

#ifndef _CV_INFO_INCLUDED
#include "cvinfo.h"
#endif

#ifndef FAR
#if _M_IX86 >= 300
#define FAR
#else
#define FAR far
#endif
#endif


//  Type of subsection entry.

#define sstModule           0x120
#define sstTypes            0x121
#define sstPublic           0x122
#define sstPublicSym        0x123   // publics as symbol (waiting for link)
#define sstSymbols          0x124
#define sstAlignSym         0x125
#define sstSrcLnSeg         0x126   // because link doesn't emit SrcModule
#define sstSrcModule        0x127
#define sstLibraries        0x128
#define sstGlobalSym        0x129
#define sstGlobalPub        0x12a
#define sstGlobalTypes      0x12b
#define sstMPC              0x12c
#define sstSegMap           0x12d
#define sstSegName          0x12e
#define sstPreComp          0x12f   // precompiled types
#define sstPreCompMap       0x130   // map precompiled types in global types
#define sstOffsetMap16      0x131
#define sstOffsetMap32      0x132
#define sstFileIndex        0x133   // Index of file names
#define sstStaticSym        0x134

typedef enum OMFHash {
    OMFHASH_NONE,           // no hashing
    OMFHASH_SUMUC16,        // upper case sum of chars in 16 bit table
    OMFHASH_SUMUC32,        // upper case sum of chars in 32 bit table
    OMFHASH_ADDR16,         // sorted by increasing address in 16 bit table
    OMFHASH_ADDR32          // sorted by increasing address in 32 bit table
} OMFHASH;

//  CodeView Debug OMF signature.  The signature at the end of the file is
//  a negative offset from the end of the file to another signature.  At
//  the negative offset (base address) is another signature whose filepos
//  field points to the first OMFDirHeader in a chain of directories.
//  The NB05 signature is used by the link utility to indicated a completely
//  unpacked file.  The NB06 signature is used by ilink to indicate that the
//  executable has had CodeView information from an incremental link appended
//  to the executable.  The NB08 signature is used by cvpack to indicate that
//  the CodeView Debug OMF has been packed.  CodeView will only process
//  executables with the NB08 signature.


typedef struct OMFSignature {
    char            Signature[4];   // "NBxx"
    long            filepos;        // offset in file
} OMFSignature;



//  directory information structure
//  This structure contains the information describing the directory.
//  It is pointed to by the signature at the base address or the directory
//  link field of a preceeding directory.  The directory entries immediately
//  follow this structure.


typedef struct OMFDirHeader {
    unsigned short  cbDirHeader;    // length of this structure
    unsigned short  cbDirEntry;     // number of bytes in each directory entry
    unsigned long   cDir;           // number of directorie entries
    long            lfoNextDir;     // offset from base of next directory
    unsigned long   flags;          // status flags
} OMFDirHeader;




//  directory structure
//  The data in this structure is used to reference the data for each
//  subsection of the CodeView Debug OMF information.  Tables that are
//  not associated with a specific module will have a module index of
//  oxffff.  These tables are the global types table, the global symbol
//  table, the global public table and the library table.


typedef struct OMFDirEntry {
    unsigned short  SubSection;     // subsection type (sst...)
    unsigned short  iMod;           // module index
    long            lfo;            // large file offset of subsection
    unsigned long   cb;             // number of bytes in subsection
} OMFDirEntry;



//  information decribing each segment in a module

typedef struct OMFSegDesc {
    unsigned short  Seg;            // segment index
    unsigned short  pad;            // pad to maintain alignment
    unsigned long   Off;            // offset of code in segment
    unsigned long   cbSeg;          // number of bytes in segment
} OMFSegDesc;




//  per module information
//  There is one of these subsection entries for each module
//  in the executable.  The entry is generated by link/ilink.
//  This table will probably require padding because of the
//  variable length module name.

typedef struct OMFModule {
    unsigned short  ovlNumber;      // overlay number
    unsigned short  iLib;           // library that the module was linked from
    unsigned short  cSeg;           // count of number of segments in module
    char            Style[2];       // debugging style "CV"
    OMFSegDesc      SegInfo[1];     // describes segments in module
    char            Name[];         // length prefixed module name padded to
                                    // long word boundary
} OMFModule;



//  Symbol hash table format
//  This structure immediately preceeds the global publics table
//  and global symbol tables.

typedef struct  OMFSymHash {
    unsigned short  symhash;        // symbol hash function index
    unsigned short  addrhash;       // address hash function index
    unsigned long   cbSymbol;       // length of symbol information
    unsigned long   cbHSym;         // length of symbol hash data
    unsigned long   cbHAddr;        // length of address hashdata
} OMFSymHash;



//  Global types subsection format
//  This structure immediately preceeds the global types table.
//  The offsets in the typeOffset array are relative to the address
//  of ctypes.  Each type entry following the typeOffset array must
//  begin on a long word boundary.

typedef struct OMFTypeFlags {
    unsigned long   sig     :8;
    unsigned long   unused  :24;
} OMFTypeFlags;


typedef struct OMFGlobalTypes {
    OMFTypeFlags    flags;
    unsigned long   cTypes;         // number of types
    unsigned long   typeOffset[];   // array of offsets to types
} OMFGlobalTypes;




//  Precompiled types mapping table
//  This table should be ignored by all consumers except the incremental
//  packer.


typedef struct OMFPreCompMap {
    CV_typ_t        FirstType;      // first precompiled type index
    CV_typ_t        cTypes;         // number of precompiled types
    unsigned long   signature;      // precompiled types signature
    CV_typ_t        map[];          // mapping of precompiled types
} OMFPreCompMap;



//  Source line to address mapping table.
//  This table is generated by the link/ilink utility from line number
//  information contained in the object file OMF data.  This table contains
//  only the code contribution for one segment from one source file.


typedef struct OMFSourceLine {
    unsigned short  Seg;            // linker segment index
    unsigned short  cLnOff;         // count of line/offset pairs
    unsigned long   offset[1];      // array of offsets in segment
    unsigned short  lineNbr[1];     // array of line lumber in source
} OMFSourceLine;

typedef OMFSourceLine * LPSL;


//  Source file description
//  This table is generated by the linker


typedef struct OMFSourceFile {
    unsigned short  cSeg;           // number of segments from source file
    unsigned short  reserved;       // reserved
    unsigned long   baseSrcLn[1];   // base of OMFSourceLine tables
                                    // this array is followed by array
                                    // of segment start/end pairs followed by
                                    // an array of linker indices
                                    // for each segment in the file
    unsigned short  cFName;         // length of source file name
    char            Name;           // name of file padded to long boundary
} OMFSourceFile;

typedef OMFSourceFile * LPSF;


//  Source line to address mapping header structure
//  This structure describes the number and location of the
//  OMFAddrLine tables for a module.  The offSrcLine entries are
//  relative to the beginning of this structure.


typedef struct OMFSourceModule {
    unsigned short  cFile;          // number of OMFSourceTables
    unsigned short  cSeg;           // number of segments in module
    unsigned long   baseSrcFile[1]; // base of OMFSourceFile table
                                    // this array is followed by array
                                    // of segment start/end pairs followed
                                    // by an array of linker indices
                                    // for each segment in the module
} OMFSourceModule;

typedef OMFSourceModule * LPSM;

//  sstLibraries

typedef struct OMFLibrary {
    unsigned char   cbLibs;     // count of library names
    char            Libs[1];    // array of length prefixed lib names (first entry zero length)
} OMFLibrary;


// sstFileIndex - An index of all of the files contributing to an
//  executable.

typedef struct OMFFileIndex {
    unsigned short  cmodules;       // Number of modules
    unsigned short  cfilerefs;      // Number of file references
    unsigned short  modulelist[1];  // Index to beginning of list of files
                                    // for module i. (0 for module w/o files)
    unsigned short  cfiles[1];      // Number of file names associated
                                    // with module i.
    unsigned long   ulNames[1];     // Offsets from the beginning of this
                                    // table to the file names
    char            Names[];        // The length prefixed names of files
} OMFFileIndex;


//  Offset mapping table
//  This table provides a mapping from logical to physical offsets.
//  This mapping is applied between the logical to physical mapping
//  described by the seg map table.

typedef struct OMFOffsetMap16 {
    unsigned long   csegment;       // Count of physical segments

    // The next six items are repeated for each segment

    unsigned long   crangeLog;      // Count of logical offset ranges
    unsigned short  rgoffLog[1];    // Array of logical offsets
    short           rgbiasLog[1];   // Array of logical->physical bias
    unsigned long   crangePhys;     // Count of physical offset ranges
    unsigned short  rgoffPhys[1];   // Array of physical offsets
    short           rgbiasPhys[1];  // Array of physical->logical bias
} OMFOffsetMap16;

typedef struct OMFOffsetMap32 {
    unsigned long   csection;       // Count of physical sections

    // The next six items are repeated for each section

    unsigned long   crangeLog;      // Count of logical offset ranges
    unsigned long   rgoffLog[1];    // Array of logical offsets
    long            rgbiasLog[1];   // Array of logical->physical bias
    unsigned long   crangePhys;     // Count of physical offset ranges
    unsigned long   rgoffPhys[1];   // Array of physical offsets
    long            rgbiasPhys[1];  // Array of physical->logical bias
} OMFOffsetMap32;

//  Pcode support.  This subsection contains debug information generated
//  by the MPC utility used to process Pcode executables.  Currently
//  it contains a mapping table from segment index (zero based) to
//  frame paragraph.  MPC converts segmented exe's to non-segmented
//  exe's for DOS support.  To avoid backpatching all CV info, this
//  table is provided for the mapping.  Additional info may be provided
//  in the future for profiler support.

typedef struct OMFMpcDebugInfo {
    unsigned short  cSeg;       // number of segments in module
    unsigned short  mpSegFrame[1];  // map seg (zero based) to frame
} OMFMpcDebugInfo;

//  The following structures and constants describe the format of the
//  CodeView Debug OMF for linkers that emit executables with the NB02
//  signature.  Current utilities with the exception of cvpack and cvdump
//  will not accept or emit executables with the NB02 signature.  Cvdump
//  will dump an unpacked executable with the NB02 signature.  Cvpack will
//  read an executable with the NB02 signature but the packed executable
//  will be written with the table format, contents and signature of NB08.





//  subsection type constants

#define SSTMODULE       0x101   // Basic info. about object module
#define SSTPUBLIC       0x102   // Public symbols
#define SSTTYPES        0x103   // Type information
#define SSTSYMBOLS      0x104   // Symbol Data
#define SSTSRCLINES     0x105   // Source line information
#define SSTLIBRARIES    0x106   // Names of all library files used
#define SSTIMPORTS      0x107   // Symbols for DLL fixups
#define SSTCOMPACTED    0x108   // Compacted types section
#define SSTSRCLNSEG     0x109   // Same as source lines, contains segment


typedef struct DirEntry{
    unsigned short  SubSectionType;
    unsigned short  ModuleIndex;
    long            lfoStart;
    unsigned short  Size;
} DirEntry;


//  information decribing each segment in a module

typedef struct oldnsg {
    unsigned short  Seg;            // segment index
    unsigned short  Off;            // offset of code in segment
    unsigned short  cbSeg;          // number of bytes in segment
} oldnsg;


//   old subsection module information

typedef struct oldsmd {
    oldnsg          SegInfo;        // describes first segment in module
    unsigned short  ovlNbr;         // overlay number
    unsigned short  iLib;
    unsigned char   cSeg;           // Number of segments in module
    char            reserved;
    unsigned char   cbName[1];      // length prefixed name of module
    oldnsg          arnsg[];        // cSeg-1 structures exist for alloc text or comdat code
} oldsmd;

typedef struct{
    unsigned short  Seg;
    unsigned long   Off;
    unsigned long   cbSeg;
} oldnsg32;

typedef struct {
    oldnsg32        SegInfo;        // describes first segment in module
    unsigned short  ovlNbr;         // overlay number
    unsigned short  iLib;
    unsigned char   cSeg;           // Number of segments in module
    char            reserved;
    unsigned char   cbName[1];      // length prefixed name of module
    oldnsg32        arnsg[];        // cSeg-1 structures exist for alloc text or comdat code
} oldsmd32;


// OMFSegMap - This table contains the mapping between the logical segment indices
// used in the symbol table and the physical segments where the program is loaded

typedef struct OMFSegMapDesc {
    unsigned short  flags;          // descriptor flags bit field.
    unsigned short  ovl;            // the logical overlay number
    unsigned short  group;          // group index into the descriptor array
    unsigned short  frame;          // logical segment index - interpreted via flags
    unsigned short  iSegName;       // segment or group name - index into sstSegName
    unsigned short  iClassName;     // class name - index into sstSegName
    unsigned long   offset;         // byte offset of the logical within the physical segment
    unsigned long   cbSeg;          // byte count of the logical segment or group
} OMFSegMapDesc;

typedef struct OMFSegMap {
    unsigned short  cSeg;           // total number of segment descriptors
    unsigned short  cSegLog;        // number of logical segment descriptors
    OMFSegMapDesc   rgDesc[0];      // array of segment descriptors
} OMFSegMap;

