/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    Udf.h

Abstract:

    This module contains all definitions specified by the OSTA UDF standard which
    are not defined in ISO 13346 and associated errta.  UDF is a subset of ISO 13346
    which restricts many facets of the ISO standard and is currently standardized
    by the Optical Storage Technology Association (http://www.osta.org).  Some
    aspects of the structures we read may seem illogical unless viewed in this light.

    Unless otherwise specified, section references will be to ISO 13346.

    Also unless otherwise specified, all descriptors mentioned will be sector aligned 

    A UDF volume is recognized by searching the Volume Recognition Area (2/8.3) for a
    Volume Structure Descriptor (2/9.1) which advertises itself as NSR02, the filesystem
    format specified by ISO 13346 section 4.  This is aligned to match ISO 9660, and the
    first descriptor may in fact be a 9660 PVD.  ISO 13346 descriptors are bounded by
    a Begin Extended Area descriptor (2/9.2) and a Terminate Extended Area descriptor
    (2/9.3).

    +-------+-------+     +-------+     +-------+
    |       |       |     |       |     |       |
    | CD001 | BEA01 | ... | NSR02 | ... | TEA01 |
    |       |       |     |       |     |       |
    +-------+-------+     +-------+     +-------+

    A UDF volume is then discovered by looking for an Anchor Volume Descriptor (3/10.2),
    which reveals the location of a pair of extents of the physical volume that contain
    copies of the Volume Descriptor Sequence.  Both of these copies are defined to be
    equivalent (duplication is intended for diasaster recovery).

    +-------+              +------------------------------------+
    |       | -----------> |                                    |
    |  AVD  |              |   Main Volume Descriptor Sequence  |
    |       | ----+        |                                    |
    +-------+     |        +------------------------------------+
                  |         
                  |        +------------------------------------+
                  |        |                                    | 
                  +------> | Reserve Volume Descriptor Sequence |
                           |                                    |
                           +------------------------------------+

    An ISO 13346 logical (mountable) volume is composed of a number Np of physical partitions
    spread across a number Nd of physical volumes (media), all of which may be multiply
    referenced to create a numbed Nv of logical volumes.  While ISO 13346 allows this level of
    complexity, UDF restricts as follows: Nv = 1 and Np = Nd except if Nd = 1 then perhaps
    Np = 2 and one partition is read/write while the other is readonly.  There are three levels
    of conforming implementations which are defined by ISO 1336 in 3/11 which progress from 1,
    a restricted Nd = 1, to 3 where Nd > 1.  This is a readonly level 2 implementation, which
    is an unrestricted single physical media implementation - other than those imposed by UDF.

    A Volume Descriptor Sequence is composed of a number of descriptors which collectively nail
    down a volume:

        Primary Volume Descriptor           (PVD)  Identification of the physical media and its
        (3/10.1)                                     relation to a volume set.

        Volume Descriptor Pointer           (VSD)  Identification of a continuing extent of the Volume
        (3/10.3)                                    Descriptor Sequence (the VDS need not be a single
                                                    extent).
                                                
        Implementation Use Volume Desciptor (IUVD) Exactly that.
        (3/10.4)

        Partition Descriptor                (PD)   Identification of a linear extent of sectors
        (3/10.5)                                    on a physical media (type 1) or an implementation
                                                    defined object (type 2).

        Logical Volume Descriptor           (LVD)  Identification of a mountable volume by
        (3/10.6)                                    referring to partition(s) and a location for
                                                    a File Set Descriptor.
                                                
        Unallocated Space Descriptor        (USD)  Identification of an unallocated extents of the
        (3/10.8)                                    media which could be added to existing partitions
                                                    or allocated through new partitions.

        Terminating Descriptor              (TD)   A method of terminating the Volume Descriptor
        (3/10.9)                                    Sequence.  A VDS may also be terminated by an
                                                    unrecorded sector or running to the end of an
                                                    extent.

    An ISO 13346 volume set is a grouping of physical media, identified collectively by examining
    the PVD of each unit. A Volume Descriptor Sequence is recorded on each constituent of the volume
    set, but only the volume with the highest Volume Sequence Number may contain LVD.  An LVD may
    refer to any PD on any member of the volume set.

    Each descriptor contains a Volume Sequence Number which allows an otherwise identification
    equivalent descriptor (i.e., specifies the same Partition Number (for PD), same Logical
    Volume Identifier (for LVD), etc. (3/8.4.3)) to override one of lower VSN.
    
    So, a picture of what a Volume Descriptor Sequence could look like is
    
    +------+------+------+------+------+
    |      |      |      |      |      |
    | PVD  | LVD  |  PD  |  PD  |  VDP |
    |      |      |      |      |      |
    +------+------+------+------+------+
                                    |
                                    |  +------+------+------+------+
                                    |  |      |      |      |      |
                                    +->| USD  | IUVD | IUVD |  TD  |
                                       |      |      |      |      |
                                       +------+------+------+------+
                                       
    The LVD points to a File Set Descriptor (4/14.1), which finally points to a root directory.
    
Author:

    Dan Lovinger    [DanLo]   10-Jul-1996

Revision History:

--*/

#ifndef _UDF_
#define _UDF_

#include <iso13346.h>

//
//  This is the version of UDF that we recognize, per the Domain Identifier
//  specification in UDF 2.1.5.3.
//
//  The values below indicate we understand UDF 1.50.  We will also define
//  specific revisions so that we can assert correctness for some structures
//  that we know appeared for the first time in certain specifications.
//
//

#define UDF_VERSION_100         0x0100
#define UDF_VERSION_101         0x0101
#define UDF_VERSION_102         0x0102
#define UDF_VERSION_150         0x0150

#define UDF_VERSION_RECOGNIZED  UDF_VERSION_150

#define UDF_VERSION_MINIMUM     UDF_VERSION_100


//
//  Method 2 Fixup.
//
//  This really isn't UDF, but for lack of a better place ... and since we are doing
//  the work for UDF only.  In the filesystem.  Sigh.
//
//  Various bad CD-ROM units, when reading fixed-packet CD-RW media, fail to map out
//  the runin/out  blocks that follow each packet of 32 sectors on the media.  As a
//  result, we have to fixup all of the byte offsets to read the image.
//
//  Note: fixed packet. Variable packet discs do have the runin/out exposed, but
//  imaging software will have realized this and numbered sectors right.
//
//  Normally we would refuse to deal with this garbage, but Adaptec made the decision
//  for us by having their reader handle these drives.  So that we don't have to deal
//  with endless "but it works with Adaptec", we've got to do it here.
//
//  This is really depressing.
//

#define CDRW_PACKET_LENGTH              32
#define CDRW_RUNOUT_LENGTH              7

//
//  LONGLONG UdfMethod2TransformByteOffset (
//      PVCB Vcb,
//      LONGLONG ByteOffset
//      )
//
//  Takes a normal byteoffset and adds in the differential implied by the number
//  of runout areas it spans.
//

#define UdfMethod2TransformByteOffset(V, BO)                                \
    ((BO) + LlBytesFromSectors((V), ((LlSectorsFromBytes((V), BO) / CDRW_PACKET_LENGTH) * CDRW_RUNOUT_LENGTH)))

#define UdfMethod2TransformSector(V, S)                                     \
    ((S) + ((S) / CDRW_PACKET_LENGTH) * CDRW_RUNOUT_LENGTH)

//
//  ULONG UdfMethod2NextRunoutInSectors (
//      PVCB Vcb,
//      LONGLONG ByteOffset
//      )
//
//  Takes a normal byteoffset and figures out how many sectors remain until the next
//  (forward) runout area.
//

#define UdfMethod2NextRunoutInSectors(V, BO)                                \
    (CDRW_PACKET_LENGTH - (LlSectorsFromBytes((V), (BO)) % CDRW_PACKET_LENGTH))


//
//  Generic constants
//

#define BYTE_COUNT_8_DOT_3                          (24)

//
//  Constants for the name transform algorithm.  Names greater than MAXLEN will be
//  rendered.  MAX_PATH comes from user-side includes that we don't get here.
//
//  UDF specifies rules for converting names from illegal->legal forms for a given OS.
//  The rest of the constants/macros are used to convert the clipped code for these
//  algorithims into a form we can directly use.
//
//  The NativeCharLength question is really not answerable for the non-8.3 case, since
//  NT internally is completely ignorant of the eventual destination of the name.
//

#define MAX_PATH            260

#define MAX_LEN             (MAX_PATH - 5)
#define EXT_LEN             5
#define CRC_LEN             5

#define DOS_NAME_LEN        8
#define DOS_EXT_LEN         3
#define DOS_CRC_LEN         4

#define IsFileNameCharLegal(c)      UdfIsCharacterLegal(c)
#define IsDeviceName(s, n)          FALSE
#define NativeCharLength(c)         1
#define UnicodeToUpper(c)           (c)

#define INT16 LONG
#define UINT16 ULONG
#define UNICODE_CHAR WCHAR

#define PERIOD              (L'.')
#define SPACE               (L' ')
#define CRC_MARK            (L'#')
#define ILLEGAL_CHAR_MARK   (L'_')

//
//  Place a non-tail recursable depth limit on ICB hierarchies.  We cannot read
//  ICB hierarchies that are deeper than this.
//

#define UDF_ICB_RECURSION_LIMIT 10


//
//  Entity ID (REGID) Suffixes are used in UDF to encode extra information away from
//  the string data in the Identifier.  See UDF 2.1.4.2.
//

//
//  A Domain Suffix is encoded for the Logical Volume Descriptor and File Set Descriptor
//

typedef struct _UDF_SUFFIX_DOMAIN {

    USHORT UdfRevision;
    UCHAR Flags;
    UCHAR Reserved[5];

} UDF_SUFFIX_DOMAIN, *PUDF_SUFFIX_DOMAIN;

#define UDF_SUFFIX_DOMAIN_FLAG_HARD_WRITEPROTECT 0x01
#define UDF_SUFFIX_DOMAIN_FLAG_SOFT_WRITEPROTECT 0x02

//
//  A UDF Suffix is encoded for extended attributes, Implementation Use Volume
//  Descriptors and VATs (among others).
//

typedef struct _UDF_SUFFIX_UDF {

    USHORT UdfRevision;
    UCHAR OSClass;
    UCHAR OSIdentifier;
    UCHAR Reserved[4];

} UDF_SUFFIX_UDF, *PUDF_SUFFIX_UDF;

//
//  An Implementation Suffix is encoded for almost every other structure containing
//  an Entity ID.
//

typedef struct _UDF_SUFFIX_IMPLEMENTATION {

    UCHAR OSClass;
    UCHAR OSIdentifier;
    UCHAR ImplementationUse[6];

} UDF_SUFFIX_IMPLEMENTATION, *PUDF_SUFFIX_IMPLEMENTATION;

//
//  OS Classes and Identifiers are defined by OSTA as of UDF 1.50
//
//  We also take the minor liberty of defining an invalid set for
//  the purposes of hinting internally that we don't care about them.
//  It is unlikely that UDF will ever hit 255, even though these are
//  technically avaliable for allocation.
//

#define OSCLASS_INVALID             255
#define OSIDENTIFIER_INVALID        255


#define OSCLASS_UNDEFINED           0
#define OSCLASS_DOS                 1
#define OSCLASS_OS2                 2
#define OSCLASS_MACOS               3
#define OSCLASS_UNIX                4
#define OSCLASS_WIN9X               5
#define OSCLASS_WINNT               6

#define OSIDENTIFIER_DOS_DOS        0

#define OSIDENTIFIER_OS2_OS2        0

#define OSIDENTIFIER_MACOS_MACOS7   0

#define OSIDENTIFIER_UNIX_GENERIC   0
#define OSIDENTIFIER_UNIX_AIX       1
#define OSIDENTIFIER_UNIX_SOLARIS   2
#define OSIDENTIFIER_UNIX_HPUX      3
#define OSIDENTIFIER_UNIX_IRIX      4
#define OSIDENTIFIER_UNIX_LINUX     5
#define OSIDENTIFIER_UNIX_MKLINUX   6
#define OSIDENTIFIER_UNIX_FREEBSD   7

#define OSIDENTIFIER_WIN9X_WIN95    0

#define OSIDENTIFIED_WINNT_WINNT    0


//
//  Character Set Lists are actually just a 32bit word where each bit N on/off specifies
//  that Character Set N is used on the volume.  Per UDF, the only character set we
//  recognize is CS0, so construct a bitmask Character Set List for that. (1/7.2.11)
//

#define UDF_CHARSETLIST 0x00000001


//
//  Generic partition map for UDF.  This allows partition maps to be typed and the
//  UDF entity identifier for the various type 2 maps to be inspected.
//

typedef struct _PARTMAP_UDF_GENERIC {

    UCHAR       Type;                   //  Partition Map Type = 2
    UCHAR       Length;                 //  Partition Map Length = 64
    UCHAR       Reserved2[2];           //  Reserved Padding
    REGID       PartID;                 //  Paritition Entity Identifier
    UCHAR       Reserved24[28];         //  Reserved Padding

} PARTMAP_UDF_GENERIC, *PPARTMAP_UDF_GENERIC;

//
//  UDF 1.50 CD UDF Partition Types
//

//////////
//  UDF Virtual Partitions are identified via a type 2 partition map of the following form.
//////////

typedef struct _PARTMAP_VIRTUAL {

    UCHAR       Type;                   //  Partition Map Type = 2
    UCHAR       Length;                 //  Partition Map Length = 64
    UCHAR       Reserved2[2];           //  Reserved Padding
    REGID       PartID;                 //  Paritition Entity Identifier
                                        //   == UdfVirtualPartitionDomainIdentifier
    USHORT      VolSetSeq;              //  Volume Set Sequence
    USHORT      Partition;              //  Related Partition
    UCHAR       Reserved40[24];         //  Reserved Padding

} PARTMAP_VIRTUAL, *PPARTMAP_VIRTUAL;

//
//  A VAT minimally contains a mapping for a single block, the REGID identifying
//  the VAT, and the identification of a previous VAT ICB location.  We also identify
//  an arbitrary sanity limit that the VAT isn't bigger than 8mb since it is extremely
//  difficult to imagine such a VAT existing in practice since each sector describes
//  (on most of our media) 2048/4 = 512 entries ... meaning at 8mb the VAT would
//  describe ~2^21 blocks.
//

#define UDF_CDUDF_TRAILING_DATA_SIZE    (sizeof(REGID) + sizeof(ULONG))

#define UDF_CDUDF_MINIMUM_VAT_SIZE      (sizeof(ULONG) + UDF_CDUDF_TRAILING_DATA_SIZE)
#define UDF_CDUDF_MAXIMUM_VAT_SIZE      (UDF_CDUDF_MINIMUM_VAT_SIZE + (8 * 1024 * 1024))

//////////
//  UDF Sparable Partitions are identified via a type 2 partition map of the following form.
//////////

typedef struct _PARTMAP_SPARABLE {

    UCHAR       Type;                   //  Partition Map Type = 2
    UCHAR       Length;                 //  Partition Map Length = 64
    UCHAR       Reserved2[2];           //  Reserved Padding
    REGID       PartID;                 //  Paritition Entity Identifier
                                        //   == UdfSparablePartitionDomainIdentifier
    USHORT      VolSetSeq;              //  Volume Set Sequence
    USHORT      Partition;              //  Related Partition
    USHORT      PacketLength;           //  Packet Length == 32 (number of data blocks
                                        //   per packet)
    UCHAR       NumSparingTables;       //  Number of pparing tables on the media
    UCHAR       Reserved43;             //  Reserved Padding
    ULONG       TableSize;              //  Size of sparing tables
    ULONG       TableLocation[4];       //  Location of each sparing table (each
                                        //   sparing table should be in a distinct packet)

} PARTMAP_SPARABLE, *PPARTMAP_SPARABLE;

//
//  Sparing tables lead off with this header structure.
//

typedef struct _SPARING_TABLE_HEADER {

    DESTAG      Destag;                 //  Ident = 0
    REGID       RegID;                  //  == UdfSparingTableIdentifier
    USHORT      TableEntries;           //  Number of entries in the table
    USHORT      Reserved50;             //  Reserved Padding
    ULONG       Sequence;               //  Sequence Number (incremented on rewrite of table)

} *PSPARING_TABLE_HEADER, SPARING_TABLE_HEADER;

//
//  Sparing table map entries.
//

typedef struct _SPARING_TABLE_ENTRY {

    ULONG Original;                     //  Original LBN
    ULONG Mapped;                       //  Mapped PSN

} *PSPARING_TABLE_ENTRY, SPARING_TABLE_ENTRY;

//
//  Fixed values for original sectors, indicating that either the
//  mapped packet is avaliable for sparing use or is defective.
//

#define UDF_SPARING_AVALIABLE           0xffffffff
#define UDF_SPARING_DEFECTIVE           0xfffffff0
 
//
//  The unit of media in each sparing packet is fixed at 32 physical sectors.
//

#define UDF_SPARING_PACKET_LENGTH       CDRW_PACKET_LENGTH

#endif // _UDF_
