/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NTFS FS library
 * FILE:        lib/fslib/ntfslib/files.c
 * PURPOSE:     NTFS metadata builder
 *
 * Builds MFT records 0..35 into one buffer, then writes the MFT and the
 * auxiliary streams ($LogFile, $AttrDef, $UpCase, $Secure:$SDS/$SDH,
 * $MFT:$BITMAP, the cluster $Bitmap, the root $I30 index, the TxF payloads).
 * Records 0..15 are the reserved system files; 24..35 are the $Extend subtree.
 * Directory index entries are copied from each child's own $FILE_NAME in the
 * buffer, so the two copies always agree.
 *
 * The $MFT is reserved large and contiguous (LAYOUT.MftClusters): the NTFS VBR
 * assumes a contiguous MFT, and a full install must not fragment it.
 */

/* INCLUDES ******************************************************************/

#include "ntfslib.h"
#include "data.h"      /* ATTRIBUTES_TABLE, UPCASE_TABLE */
#include "refdata.h"   /* DefaultSds, RootSdValue, TxfDataValue, TopsData, ... */

#define NDEBUG
#include <debug.h>


/* CONSTANTS *****************************************************************/

/* MFT record numbers. 0..11 system files, 12..15 reserved stubs; the $Extend
 * children sit at 24..35, above the reserved 0..15 range. */
#define MREC_MFT        0
#define MREC_MFTMIRR    1
#define MREC_LOGFILE    2
#define MREC_VOLUME     3
#define MREC_ATTRDEF    4
#define MREC_ROOT       5
#define MREC_BITMAP     6
#define MREC_BOOT       7
#define MREC_BADCLUS    8
#define MREC_SECURE     9
#define MREC_UPCASE     10
#define MREC_EXTEND     11
#define MREC_QUOTA      24
#define MREC_OBJID      25
#define MREC_REPARSE    26
#define MREC_RMMETA     27   /* $Extend/$RmMetadata */
#define MREC_REPAIR     28   /* $RmMetadata/$Repair */
#define MREC_DELETED    29   /* $Extend/$Deleted */
#define MREC_TXFLOG     30   /* $RmMetadata/$TxfLog */
#define MREC_TXF        31   /* $RmMetadata/$Txf */
#define MREC_TOPS       32   /* $TxfLog/$Tops */
#define MREC_TXFBLF     33   /* $TxfLog/$TxfLog.blf */
#define MREC_CONT1      34   /* $TxfLog/$TxfLogContainer...0001 */
#define MREC_CONT2      35   /* $TxfLog/$TxfLogContainer...0002 */
#define MREC_COUNT      36   /* records materialized in the build buffer */

/* File attribute (STD_INFO / FILE_NAME) flags. */
#define FA_HIDDEN   0x0002u
#define FA_SYSTEM   0x0004u
#define FA_ARCHIVE  0x0020u
#define FA_DIR      0x10000000u   /* directory (has $I30) */
#define FA_VIEW     0x20000000u   /* carries a view index ($Extend children / $Secure) */

/* Collation rules not already in ntfslib.h. */
#define COLL_SID     0x11
#define COLL_ULONGS  0x13

/* Update-sequence check value (any value except 0x0000 / 0xFFFF). */
#define USN_VALUE  1

#define HOLE_LCN  (~0ULL)


/* UNALIGNED ACCESS HELPERS **************************************************/

#define WR16(p, o, v) (*(PUSHORT)((PBYTE)(p) + (o)) = (USHORT)(v))
#define WR32(p, o, v) (*(PULONG)((PBYTE)(p) + (o)) = (ULONG)(v))
#define WR64(p, o, v) (*(PULONGLONG)((PBYTE)(p) + (o)) = (ULONGLONG)(v))
#define RD16(p, o)    (*(PUSHORT)((PBYTE)(p) + (o)))
#define RD32(p, o)    (*(PULONG)((PBYTE)(p) + (o)))

#define ALIGN8(x)   (((x) + 7u) & ~7u)


/* STRUCTURES ****************************************************************/

typedef struct
{
    ULONGLONG Lcn;    /* HOLE_LCN for a sparse hole */
    ULONGLONG Len;    /* in clusters */
} MK_RUN;

typedef struct
{
    PBYTE  Buf;
    ULONG  Offset;      /* current append offset */
    USHORT NextAttrId;
} MK_REC;

typedef struct
{
    const BYTE *Key;
    USHORT      KeyLen;
    const BYTE *Data;
    USHORT      DataLen;
} MK_VIEW_ENTRY;


/* RUN-LIST (MAPPING PAIRS) ENCODER ******************************************/

/* Minimal *signed* byte count for Value (both run length and LCN delta are
 * stored as signed values in NTFS mapping pairs). */
static
ULONG
MkSignBytes(IN LONGLONG Value)
{
    ULONG    Bytes;
    LONGLONG V;

    if (Value == 0)
        return 0;

    Bytes = 0;
    V = Value;
    do
    {
        Bytes++;
        V >>= 8;
    }
    while (!((V ==  0) && (((Value >> (Bytes * 8 - 1)) & 1) == 0)) &&
           !((V == -1) && (((Value >> (Bytes * 8 - 1)) & 1) == 1)));

    return Bytes;
}

/* Encodes a runlist into NTFS mapping pairs (plus terminator). Returns length. */
static
ULONG
MkEncodeRuns(IN  const MK_RUN *Runs,
             IN  ULONG         RunCount,
             OUT PBYTE         Out)
{
    ULONG     Pos = 0;
    ULONG     Index;
    ULONGLONG PrevLcn = 0;

    for (Index = 0; Index < RunCount; Index++)
    {
        ULONG     LenBytes;
        ULONG     OffBytes;
        LONGLONG  LcnDelta;
        ULONG     B;
        ULONGLONG Len = Runs[Index].Len;

        /* Length is unsigned but stored minimally-signed with a clear top bit. */
        LenBytes = MkSignBytes((LONGLONG)Len);
        if (LenBytes == 0)
            LenBytes = 1;
        if ((Len >> (LenBytes * 8 - 1)) & 1)
            LenBytes++;

        if (Runs[Index].Lcn == HOLE_LCN)
        {
            OffBytes = 0;
            LcnDelta = 0;
        }
        else
        {
            LcnDelta = (LONGLONG)Runs[Index].Lcn - (LONGLONG)PrevLcn;
            OffBytes = MkSignBytes(LcnDelta);
            if (OffBytes == 0)
                OffBytes = 1;
            PrevLcn = Runs[Index].Lcn;
        }

        Out[Pos++] = (BYTE)((OffBytes << 4) | (LenBytes & 0x0F));
        for (B = 0; B < LenBytes; B++)
            Out[Pos++] = (BYTE)(Len >> (B * 8));
        for (B = 0; B < OffBytes; B++)
            Out[Pos++] = (BYTE)((ULONGLONG)LcnDelta >> (B * 8));
    }

    Out[Pos++] = 0;   /* terminator */
    return Pos;
}


/* MFT RECORD BUILDER ********************************************************/

static
VOID
MkInit(OUT MK_REC *Rec,
       IN  PBYTE   Buf,
       IN  ULONG   RecordNumber,
       IN  USHORT  Flags)
{
    USHORT UsaCount = (USHORT)(MFT_RECORD_SIZE / BYTES_PER_SECTOR + 1);

    RtlZeroMemory(Buf, MFT_RECORD_SIZE);
    Rec->Buf        = Buf;
    Rec->NextAttrId = 0;

    RtlCopyMemory(Buf + 0x00, "FILE", 4);
    WR16(Buf, 0x04, 0x30);                            /* USA offset */
    WR16(Buf, 0x06, UsaCount);                        /* USA count */
    WR64(Buf, 0x08, 0);                               /* $LogFile LSN */
    WR16(Buf, 0x10, RECORD_SEQUENCE(RecordNumber));   /* sequence number */
    WR16(Buf, 0x12, 1);                               /* hard link count */
    WR16(Buf, 0x14, 0x38);                            /* first attribute offset */
    WR16(Buf, 0x16, Flags);
    WR32(Buf, 0x1C, MFT_RECORD_SIZE);                 /* allocated size */
    WR64(Buf, 0x20, 0);                               /* base record */
    WR16(Buf, 0x28, 0);                               /* next attr id (set at finalize) */
    WR32(Buf, 0x2C, RecordNumber);                    /* this record number */

    Rec->Offset = 0x38;
}

static
VOID
MkSetHardLinks(IN MK_REC *Rec,
               IN USHORT  Count)
{
    WR16(Rec->Buf, 0x12, Count);
}

/* Appends a resident attribute. Name is UTF-16 (NameChars units, may be 0). */
static
VOID
MkAddResident(IN OUT MK_REC     *Rec,
              IN     ULONG        Type,
              IN     LPCWSTR      Name,
              IN     BYTE         NameChars,
              IN     BYTE         ResidentFlags,
              IN     const VOID  *Value,
              IN     ULONG        ValueLen)
{
    PBYTE  A = Rec->Buf + Rec->Offset;
    USHORT ValueOff = (USHORT)ALIGN8(0x18u + (ULONG)NameChars * 2);
    ULONG  AttrLen  = ALIGN8(ValueOff + ValueLen);
    ULONG  I;

    WR32(A, 0x00, Type);
    WR32(A, 0x04, AttrLen);
    A[0x08] = 0;                        /* resident */
    A[0x09] = NameChars;
    WR16(A, 0x0A, 0x18);               /* name offset (Windows sets it even if unnamed) */
    WR16(A, 0x0C, 0);                  /* flags */
    WR16(A, 0x0E, Rec->NextAttrId++);
    WR32(A, 0x10, ValueLen);
    WR16(A, 0x14, ValueOff);
    A[0x16] = ResidentFlags;
    A[0x17] = 0;

    for (I = 0; I < NameChars; I++)
        WR16(A, 0x18 + I * 2, Name[I]);
    if (ValueLen > 0)
        RtlCopyMemory(A + ValueOff, Value, ValueLen);

    Rec->Offset += AttrLen;
}

/* Appends a non-resident attribute with an explicit runlist. */
static
VOID
MkAddNonResident(IN OUT MK_REC       *Rec,
                 IN     ULONG          Type,
                 IN     LPCWSTR        Name,
                 IN     BYTE           NameChars,
                 IN     USHORT         Flags,
                 IN     const MK_RUN  *Runs,
                 IN     ULONG          RunCount,
                 IN     ULONGLONG      HighVcn,
                 IN     ULONGLONG      AllocSize,
                 IN     ULONGLONG      RealSize,
                 IN     ULONGLONG      InitSize)
{
    PBYTE   A = Rec->Buf + Rec->Offset;
    BYTE    MapBuf[64];
    ULONG   MapLen;
    USHORT  DataBase;
    USHORT  MapOff;
    ULONG   AttrLen;
    ULONG   I;
    BOOLEAN HasCompSize;

    MapLen = MkEncodeRuns(Runs, RunCount, MapBuf);

    /* A compressed (0x0001) or sparse (0x8000) attribute carries an extra
     * 8-byte "compressed size" field at 0x40, pushing name/runs to 0x48. */
    HasCompSize = (Flags & (ATTR_IS_COMPRESSED | ATTR_IS_SPARSE)) != 0;
    DataBase = (USHORT)(HasCompSize ? 0x48 : 0x40);

    MapOff  = (USHORT)ALIGN8((ULONG)DataBase + (ULONG)NameChars * 2);
    AttrLen = ALIGN8(MapOff + MapLen);

    WR32(A, 0x00, Type);
    WR32(A, 0x04, AttrLen);
    A[0x08] = 1;                       /* non-resident */
    A[0x09] = NameChars;
    WR16(A, 0x0A, DataBase);           /* name offset */
    WR16(A, 0x0C, Flags);
    WR16(A, 0x0E, Rec->NextAttrId++);
    WR64(A, 0x10, 0);                  /* starting VCN */
    WR64(A, 0x18, HighVcn);            /* last VCN */
    WR16(A, 0x20, MapOff);
    WR16(A, 0x22, 0);                  /* compression unit */
    WR32(A, 0x24, 0);                  /* padding */
    WR64(A, 0x28, AllocSize);
    WR64(A, 0x30, RealSize);
    WR64(A, 0x38, InitSize);
    if (HasCompSize)
        WR64(A, 0x40, 0);              /* compressed/allocated size (our only sparse stream is all holes) */

    for (I = 0; I < NameChars; I++)
        WR16(A, DataBase + I * 2, Name[I]);
    RtlCopyMemory(A + MapOff, MapBuf, MapLen);

    Rec->Offset += AttrLen;
}

/* Terminates the attribute list, records bytes-in-use / next attr id, and
 * applies the update-sequence (fixup) so the record is ready to write. */
static
VOID
MkFinalize(IN MK_REC *Rec)
{
    PBYTE  Buf = Rec->Buf;
    ULONG  Used;
    USHORT UsaOff;
    USHORT UsaCount;
    ULONG  Sector;

    WR32(Buf, Rec->Offset, AttributeEnd);   /* end marker */
    WR32(Buf, Rec->Offset + 4, 0);
    Used = Rec->Offset + 8;

    WR32(Buf, 0x18, Used);                   /* bytes in use */
    WR16(Buf, 0x28, Rec->NextAttrId);        /* next attr id */

    UsaOff   = RD16(Buf, 0x04);
    UsaCount = RD16(Buf, 0x06);
    WR16(Buf, UsaOff, USN_VALUE);
    for (Sector = 0; Sector + 1 < UsaCount; Sector++)
    {
        PBYTE Tail = Buf + (Sector + 1) * BYTES_PER_SECTOR - 2;

        WR16(Buf, UsaOff + (Sector + 1) * 2, RD16(Tail, 0));
        WR16(Tail, 0, USN_VALUE);
    }
}


/* ATTRIBUTE VALUE BUILDERS **************************************************/

static
VOID
MkStdInfo(OUT PBYTE     Out,
          IN  ULONG     FileAttributes,
          IN  ULONGLONG Now,
          IN  ULONG     SecurityId)
{
    RtlZeroMemory(Out, 72);
    WR64(Out, 0x00, Now);   /* creation */
    WR64(Out, 0x08, Now);   /* modified */
    WR64(Out, 0x10, Now);   /* mft changed */
    WR64(Out, 0x18, Now);   /* accessed */
    WR32(Out, 0x20, FileAttributes);
    WR32(Out, 0x34, SecurityId);
}

/* Short (v1.2, 48-byte) STANDARD_INFORMATION with no SecurityId. Used by the
 * files that carry an embedded $SECURITY_DESCRIPTOR: root, $Boot, recs 12..15. */
static
VOID
MkStdInfoShort(OUT PBYTE     Out,
               IN  ULONG     FileAttributes,
               IN  ULONGLONG Now)
{
    RtlZeroMemory(Out, 48);
    WR64(Out, 0x00, Now);
    WR64(Out, 0x08, Now);
    WR64(Out, 0x10, Now);
    WR64(Out, 0x18, Now);
    WR32(Out, 0x20, FileAttributes);
}

static
ULONG
MkFileName(OUT PBYTE     Out,
           IN  ULONGLONG ParentRef,
           IN  ULONG     FileAttributes,
           IN  ULONGLONG AllocSize,
           IN  ULONGLONG RealSize,
           IN  ULONGLONG Now,
           IN  LPCWSTR   Name,
           IN  BYTE      Namespace)
{
    BYTE NameLen = 0;
    ULONG I;

    while (Name[NameLen] != L'\0')
        NameLen++;

    RtlZeroMemory(Out, 0x42 + (ULONG)NameLen * 2);
    WR64(Out, 0x00, ParentRef);
    WR64(Out, 0x08, Now);
    WR64(Out, 0x10, Now);
    WR64(Out, 0x18, Now);
    WR64(Out, 0x20, Now);
    WR64(Out, 0x28, AllocSize);
    WR64(Out, 0x30, RealSize);
    WR32(Out, 0x38, FileAttributes);
    Out[0x40] = NameLen;
    Out[0x41] = Namespace;
    for (I = 0; I < NameLen; I++)
        WR16(Out, 0x42 + I * 2, Name[I]);

    return 0x42 + (ULONG)NameLen * 2;
}

/* Copies the resident $FILE_NAME (0x30) value out of a built MFT record.
 * Safe on a finalized record: the FILE_NAME sits far from the fixup bytes. */
static
USHORT
MkExtractFileName(IN  const BYTE *RecBuf,
                  OUT PBYTE       Out)
{
    ULONG P = RD16(RecBuf, 0x14);

    while ((P + 4) <= MFT_RECORD_SIZE)
    {
        ULONG Type = RD32(RecBuf, P);
        ULONG Len;

        if (Type == (ULONG)AttributeEnd)
            break;
        Len = RD32(RecBuf, P + 4);
        if (Len == 0)
            break;
        if ((Type == (ULONG)AttributeFileName) && (RecBuf[P + 8] == 0))
        {
            ULONG  ValLen = RD32(RecBuf, P + 0x10);
            USHORT ValOff = RD16(RecBuf, P + 0x14);

            RtlCopyMemory(Out, RecBuf + P + ValOff, ValLen);
            return (USHORT)ValLen;
        }
        P += Len;
    }
    return 0;
}

/* Empty ($I30) directory INDEX_ROOT (header + end marker). Returns 0x30. */
static
ULONG
MkEmptyI30Root(OUT PBYTE Out)
{
    RtlZeroMemory(Out, 0x30);
    WR32(Out, 0x00, AttributeFileName);
    WR32(Out, 0x04, COLLATION_FILE_NAME);
    WR32(Out, 0x08, INDEX_RECORD_SIZE);
    Out[0x0C] = (BYTE)LAYOUT.ClustersPerIndexRecord;
    WR32(Out, 0x10, 0x10);              /* entries offset */
    WR32(Out, 0x14, 0x10 + 0x10);       /* index length */
    WR32(Out, 0x18, 0x10 + 0x10);       /* allocated */
    Out[0x1C] = 0;                      /* small index */
    WR16(Out, 0x28, 0x10);             /* end entry length */
    WR16(Out, 0x2C, INDEX_ENTRY_END);
    return 0x30;
}

/* Large ($I30) directory INDEX_ROOT: no inline entries, end marker flagged
 * LAST|HAS_SUBNODE pointing at INDX VCN 0. Returns 0x38. */
static
ULONG
MkLargeI30Root(OUT PBYTE Out)
{
    RtlZeroMemory(Out, 0x38);
    WR32(Out, 0x00, AttributeFileName);
    WR32(Out, 0x04, COLLATION_FILE_NAME);
    WR32(Out, 0x08, INDEX_RECORD_SIZE);
    Out[0x0C] = (BYTE)LAYOUT.ClustersPerIndexRecord;
    WR32(Out, 0x10, 0x10);
    WR32(Out, 0x14, 0x10 + 0x18);
    WR32(Out, 0x18, 0x10 + 0x18);
    Out[0x1C] = INDEX_NODE_LARGE;
    WR16(Out, 0x28, 0x18);             /* entry length */
    WR16(Out, 0x2C, INDEX_ENTRY_END | INDEX_ENTRY_NODE);
    WR64(Out, 0x30, 0);                /* subnode VCN */
    return 0x38;
}

/* Small resident $I30 INDEX_ROOT holding filename entries for the given records
 * (collation order), plus the end marker. Returns the value length. */
static
ULONG
MkI30RootWithEntries(OUT PBYTE        Out,
                     IN  const BYTE  *Mft,
                     IN  const ULONG *Order,
                     IN  ULONG        Count)
{
    ULONG P;
    ULONG Index;
    ULONG IndexLen;

    RtlZeroMemory(Out, 0x20);
    WR32(Out, 0x00, AttributeFileName);
    WR32(Out, 0x04, COLLATION_FILE_NAME);
    WR32(Out, 0x08, INDEX_RECORD_SIZE);
    Out[0x0C] = (BYTE)LAYOUT.ClustersPerIndexRecord;
    WR32(Out, 0x10, 0x10);              /* entries offset */
    Out[0x1C] = 0;                      /* small index */

    P = 0x20;
    for (Index = 0; Index < Count; Index++)
    {
        BYTE   Fn[256];
        USHORT FnLen    = MkExtractFileName(Mft + (ULONG_PTR)Order[Index] * MFT_RECORD_SIZE, Fn);
        ULONG  EntryLen = ALIGN8(0x10u + FnLen);

        RtlZeroMemory(Out + P, EntryLen);
        WR64(Out, P + 0x00, MFT_REFERENCE(Order[Index]));
        WR16(Out, P + 0x08, EntryLen);
        WR16(Out, P + 0x0A, FnLen);
        WR16(Out, P + 0x0C, 0);
        WR16(Out, P + 0x0E, 0);
        RtlCopyMemory(Out + P + 0x10, Fn, FnLen);
        P += EntryLen;
    }

    /* End marker */
    RtlZeroMemory(Out + P, 0x10);
    WR16(Out, P + 0x08, 0x10);
    WR16(Out, P + 0x0C, INDEX_ENTRY_END);
    P += 0x10;

    IndexLen = P - 0x10;
    WR32(Out, 0x14, IndexLen);
    WR32(Out, 0x18, IndexLen);
    return P;
}

/* Empty view-index INDEX_ROOT (header + end marker). Returns 0x30. */
static
ULONG
MkEmptyViewRoot(OUT PBYTE Out,
                IN  ULONG CollationRule)
{
    RtlZeroMemory(Out, 0x30);
    WR32(Out, 0x00, 0);                 /* view index (indexed attr type 0) */
    WR32(Out, 0x04, CollationRule);
    WR32(Out, 0x08, INDEX_RECORD_SIZE);
    Out[0x0C] = (BYTE)LAYOUT.ClustersPerIndexRecord;
    WR32(Out, 0x10, 0x10);
    WR32(Out, 0x14, 0x10 + 0x10);
    WR32(Out, 0x18, 0x10 + 0x10);
    Out[0x1C] = 0;
    WR16(Out, 0x28, 0x10);
    WR16(Out, 0x2C, INDEX_ENTRY_END);
    return 0x30;
}

/* Large view-index INDEX_ROOT: entries live in the $INDEX_ALLOCATION. Returns 0x38. */
static
ULONG
MkLargeViewRoot(OUT PBYTE Out,
                IN  ULONG CollationRule)
{
    RtlZeroMemory(Out, 0x38);
    WR32(Out, 0x00, 0);
    WR32(Out, 0x04, CollationRule);
    WR32(Out, 0x08, INDEX_RECORD_SIZE);
    Out[0x0C] = (BYTE)LAYOUT.ClustersPerIndexRecord;
    WR32(Out, 0x10, 0x10);
    WR32(Out, 0x14, 0x10 + 0x18);
    WR32(Out, 0x18, 0x10 + 0x18);
    Out[0x1C] = INDEX_NODE_LARGE;
    WR16(Out, 0x28, 0x18);
    WR16(Out, 0x2C, INDEX_ENTRY_END | INDEX_ENTRY_NODE);
    WR64(Out, 0x30, 0);
    return 0x38;
}

/* Resident view-index INDEX_ROOT with N entries (collation order) plus the end
 * marker. Data follows each key immediately at 0x10 + KeyLen (the key itself is
 * not padded to 8 bytes; only the whole entry is). Returns the length. */
static
ULONG
MkViewRootN(OUT PBYTE                Out,
            IN  ULONG                CollationRule,
            IN  const MK_VIEW_ENTRY *Entries,
            IN  ULONG                Count)
{
    ULONG P;
    ULONG Index;
    ULONG IndexLen;

    RtlZeroMemory(Out, 0x20);
    WR32(Out, 0x00, 0);
    WR32(Out, 0x04, CollationRule);
    WR32(Out, 0x08, INDEX_RECORD_SIZE);
    Out[0x0C] = (BYTE)LAYOUT.ClustersPerIndexRecord;
    WR32(Out, 0x10, 0x10);
    Out[0x1C] = 0;

    P = 0x20;
    for (Index = 0; Index < Count; Index++)
    {
        USHORT DataOff  = (USHORT)(0x10 + Entries[Index].KeyLen);
        ULONG  EntryLen = ALIGN8((ULONG)DataOff + Entries[Index].DataLen);

        RtlZeroMemory(Out + P, EntryLen);
        WR16(Out, P + 0x00, DataOff);
        WR16(Out, P + 0x02, Entries[Index].DataLen);
        WR16(Out, P + 0x08, EntryLen);
        WR16(Out, P + 0x0A, Entries[Index].KeyLen);
        WR16(Out, P + 0x0C, 0);
        RtlCopyMemory(Out + P + 0x10, Entries[Index].Key, Entries[Index].KeyLen);
        RtlCopyMemory(Out + P + DataOff, Entries[Index].Data, Entries[Index].DataLen);
        P += EntryLen;
    }

    RtlZeroMemory(Out + P, 0x10);
    WR16(Out, P + 0x08, 0x10);
    WR16(Out, P + 0x0C, INDEX_ENTRY_END);
    P += 0x10;

    IndexLen = P - 0x10;
    WR32(Out, 0x14, IndexLen);
    WR32(Out, 0x18, IndexLen);
    return P;
}

/* NTFS security-descriptor hash: rol32(hash,3) + dword over the descriptor. */
static
ULONG
MkSecHash(IN const BYTE *Sd,
          IN ULONG       Len)
{
    ULONG Hash = 0;
    ULONG I;

    for (I = 0; I + 4 <= Len; I += 4)
    {
        ULONG Dword = RD32(Sd, I);
        Hash = Dword + ((Hash << 3) | (Hash >> 29));
    }
    return Hash;
}

/* Applies the update sequence (fixup) to an INDX block spanning the whole buffer. */
static
VOID
MkIndxFixup(IN OUT PBYTE Blk,
            IN     ULONG BlkSize)
{
    USHORT UsaCount = (USHORT)(BlkSize / BYTES_PER_SECTOR + 1);
    ULONG  Sector;

    WR16(Blk, 0x28, USN_VALUE);
    for (Sector = 0; Sector + 1 < UsaCount; Sector++)
    {
        PBYTE Tail = Blk + (Sector + 1) * BYTES_PER_SECTOR - 2;

        WR16(Blk, 0x28 + (Sector + 1) * 2, RD16(Tail, 0));
        WR16(Tail, 0, USN_VALUE);
    }
}

/* One INDX block holding view-index entries (key immediately followed by data). */
static
VOID
MkViewIndxBlock(OUT PBYTE                Blk,
                IN  const MK_VIEW_ENTRY *Entries,
                IN  ULONG                Count)
{
    ULONG  P;
    ULONG  Index;
    USHORT UsaCount = (USHORT)(INDEX_RECORD_SIZE / BYTES_PER_SECTOR + 1);

    RtlZeroMemory(Blk, INDEX_RECORD_SIZE);
    RtlCopyMemory(Blk + 0x00, "INDX", 4);
    WR16(Blk, 0x04, 0x28);                 /* USA offset */
    WR16(Blk, 0x06, UsaCount);
    WR64(Blk, 0x10, 0);                    /* VCN */
    WR32(Blk, 0x18, 0x28);                 /* entries offset -> 0x40 */
    WR32(Blk, 0x20, INDEX_RECORD_SIZE - 0x18);
    Blk[0x24] = 0;                         /* leaf */

    P = 0x40;
    for (Index = 0; Index < Count; Index++)
    {
        USHORT DataOff  = (USHORT)(0x10 + Entries[Index].KeyLen);
        ULONG  EntryLen = ALIGN8((ULONG)DataOff + Entries[Index].DataLen);
        ULONG  Pad;

        WR16(Blk, P + 0x00, DataOff);
        WR16(Blk, P + 0x02, Entries[Index].DataLen);
        WR16(Blk, P + 0x08, EntryLen);
        WR16(Blk, P + 0x0A, Entries[Index].KeyLen);
        WR16(Blk, P + 0x0C, 0);
        RtlCopyMemory(Blk + P + 0x10, Entries[Index].Key, Entries[Index].KeyLen);
        RtlCopyMemory(Blk + P + DataOff, Entries[Index].Data, Entries[Index].DataLen);
        /* Slack after each $SDH entry's data holds the UTF-16 unit 0x0049 ('I'),
         * not zero. */
        for (Pad = (ULONG)DataOff + Entries[Index].DataLen; Pad + 1 < EntryLen; Pad += 2)
            WR16(Blk, P + Pad, 0x0049);
        P += EntryLen;
    }

    WR16(Blk, P + 0x08, 0x10);
    WR16(Blk, P + 0x0C, INDEX_ENTRY_END);
    P += 0x10;
    WR32(Blk, 0x1C, P - 0x18);             /* index length */

    MkIndxFixup(Blk, INDEX_RECORD_SIZE);
}

/* One INDX block for the root $I30 index: a filename entry per record in Order
 * (collation order), fixup applied. */
static
VOID
MkRootIndxBlock(OUT PBYTE        Blk,
                IN  const BYTE  *Mft,
                IN  const ULONG *Order,
                IN  ULONG        Count)
{
    ULONG  P;
    ULONG  Index;
    USHORT UsaCount = (USHORT)(INDEX_RECORD_SIZE / BYTES_PER_SECTOR + 1);

    RtlZeroMemory(Blk, INDEX_RECORD_SIZE);
    RtlCopyMemory(Blk + 0x00, "INDX", 4);
    WR16(Blk, 0x04, 0x28);
    WR16(Blk, 0x06, UsaCount);
    WR64(Blk, 0x08, 0);
    WR64(Blk, 0x10, 0);                    /* VCN of this INDX */
    WR32(Blk, 0x18, 0x28);                 /* entries offset -> 0x40 */
    WR32(Blk, 0x20, INDEX_RECORD_SIZE - 0x18);
    Blk[0x24] = 0;                         /* leaf */

    P = 0x40;
    for (Index = 0; Index < Count; Index++)
    {
        const BYTE *RecBuf = Mft + (ULONG_PTR)Order[Index] * MFT_RECORD_SIZE;
        BYTE        Fn[256];
        USHORT      FnLen    = MkExtractFileName(RecBuf, Fn);
        ULONG       EntryLen = ALIGN8(0x10u + FnLen);

        WR64(Blk, P + 0x00, MFT_REFERENCE(Order[Index]));
        WR16(Blk, P + 0x08, EntryLen);
        WR16(Blk, P + 0x0A, FnLen);
        WR16(Blk, P + 0x0C, 0);
        WR16(Blk, P + 0x0E, 0);
        RtlCopyMemory(Blk + P + 0x10, Fn, FnLen);
        P += EntryLen;
    }

    WR16(Blk, P + 0x08, 0x10);
    WR16(Blk, P + 0x0C, INDEX_ENTRY_END);
    P += 0x10;
    WR32(Blk, 0x1C, P - 0x18);             /* index length */

    MkIndxFixup(Blk, INDEX_RECORD_SIZE);
}


/* DISK WRITE HELPERS ********************************************************/

static
NTSTATUS
WriteAt(IN ULONGLONG Lcn,
        IN PVOID     Buffer,
        IN ULONG     Length)
{
    LARGE_INTEGER   Offset;
    IO_STATUS_BLOCK IoStatusBlock;

    Offset.QuadPart = (LONGLONG)Lcn * BYTES_PER_CLUSTER;
    return NtWriteFile(DISK_HANDLE, NULL, NULL, NULL, &IoStatusBlock,
                       Buffer, Length, &Offset, NULL);
}

/* Fills a contiguous run of clusters with a byte pattern in bounded chunks. */
static
NTSTATUS
WritePatternToClusters(IN ULONGLONG Address,
                       IN ULONG     ClustersCount,
                       IN BYTE      Pattern)
{
    PBYTE     Buffer;
    ULONG     ChunkClusters;
    ULONG     ChunkBytes;
    ULONG     Remaining;
    ULONGLONG Lcn;
    NTSTATUS  Status = STATUS_SUCCESS;

    if (ClustersCount == 0)
        return STATUS_SUCCESS;

    ChunkClusters = (1024 * 1024) / BYTES_PER_CLUSTER;
    if (ChunkClusters == 0)
        ChunkClusters = 1;
    if (ChunkClusters > ClustersCount)
        ChunkClusters = ClustersCount;
    ChunkBytes = ChunkClusters * BYTES_PER_CLUSTER;

    Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, ChunkBytes);
    if (!Buffer)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlFillMemory(Buffer, ChunkBytes, Pattern);

    Lcn = Address;
    Remaining = ClustersCount;
    while (Remaining > 0)
    {
        ULONG ThisClusters = (Remaining < ChunkClusters) ? Remaining : ChunkClusters;

        Status = WriteAt(Lcn, Buffer, ThisClusters * BYTES_PER_CLUSTER);
        if (!NT_SUCCESS(Status))
            break;

        Lcn       += ThisClusters;
        Remaining -= ThisClusters;
    }

    FREE(Buffer);
    return Status;
}

static
NTSTATUS
WriteZerosToClusters(IN ULONGLONG Address,
                     IN ULONG     ClustersCount)
{
    return WritePatternToClusters(Address, ClustersCount, 0x00);
}

/* Writes Content (zero-padded) into a cluster-aligned region. */
static
NTSTATUS
WriteRegion(IN ULONGLONG    Lcn,
            IN ULONG        Clusters,
            IN const VOID  *Content,
            IN ULONG        ContentLen)
{
    ULONG    Size = Clusters * BYTES_PER_CLUSTER;
    PBYTE    Buffer;
    NTSTATUS Status;

    Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, Size);
    if (!Buffer)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(Buffer, Size);
    if (Content && ContentLen)
        RtlCopyMemory(Buffer, Content, ContentLen);

    Status = WriteAt(Lcn, Buffer, Size);

    FREE(Buffer);
    return Status;
}


/* THE METADATA BUILDER *****************************************************/

NTSTATUS
WriteMetafiles(VOID)
{
    NTSTATUS  Status;
    PBYTE     Mft;
    ULONG     C = BYTES_PER_CLUSTER;
    ULONGLONG Now = FORMAT_TIME;
    ULONGLONG ClusterCount = LAYOUT.ClusterCount;

    /* --- $Secure descriptor precompute (blob/$SDS offsets + hashes) ------ */
    ULONG SdBlobOff[NTFS_DEFAULT_SD_COUNT];
    ULONG SdSdsOff[NTFS_DEFAULT_SD_COUNT];
    ULONG SdHash[NTFS_DEFAULT_SD_COUNT];
    ULONG SdsContentLen;

    /* $SDH hash-sorted order of the 8 default descriptors (index i = id 0x100+i). */
    static const BYTE SdhOrder[NTFS_DEFAULT_SD_COUNT] = { 4, 6, 2, 3, 7, 5, 0, 1 };

    ULONG Blob = 0;
    ULONG Sds  = 0;
    ULONG i;

    for (i = 0; i < NTFS_DEFAULT_SD_COUNT; i++)
    {
        SdBlobOff[i] = Blob;
        SdSdsOff[i]  = Sds;
        SdHash[i]    = MkSecHash(DefaultSds + Blob, DefaultSdLen[i]);
        Blob += DefaultSdLen[i];
        Sds   = ALIGN_UP_BY(Sds + 0x14 + DefaultSdLen[i], 16);
    }
    SdsContentLen = SdSdsOff[NTFS_DEFAULT_SD_COUNT - 1] + 0x14 +
                    DefaultSdLen[NTFS_DEFAULT_SD_COUNT - 1];

    /* --- Build the MFT in memory (records 0..35) ------------------------ */
    Mft = RtlAllocateHeap(RtlGetProcessHeap(), 0, MREC_COUNT * MFT_RECORD_SIZE);
    if (!Mft)
    {
        DPRINT1("ERROR: Unable to allocate MFT build buffer!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(Mft, MREC_COUNT * MFT_RECORD_SIZE);

    {
        MK_REC Rec;
        BYTE   Val[2048];
        ULONG  Len;
        MK_RUN Runs[1];

        #define REC_BUF(n)  (Mft + (ULONG_PTR)(n) * MFT_RECORD_SIZE)

        /* Record 0: $MFT (large, contiguous $DATA run). */
        MkInit(&Rec, REC_BUF(MREC_MFT), MREC_MFT, MFT_RECORD_IN_USE);
        MkStdInfo(Val, FA_HIDDEN | FA_SYSTEM, Now, NTFS_SECURITY_ID);
        MkAddResident(&Rec, AttributeStandardInformation, NULL, 0, 0, Val, 72);
        Len = MkFileName(Val, MFT_REFERENCE(MREC_ROOT), FA_HIDDEN | FA_SYSTEM,
                         (ULONGLONG)LAYOUT.MftClusters * C,
                         (ULONGLONG)LAYOUT.MftAllocRecords * MFT_RECORD_SIZE,
                         Now, L"$MFT", FILE_NAME_WIN32_AND_DOS);
        MkAddResident(&Rec, AttributeFileName, NULL, 0, RA_INDEXED, Val, Len);
        Runs[0].Lcn = LAYOUT.MftLcn; Runs[0].Len = LAYOUT.MftClusters;
        MkAddNonResident(&Rec, AttributeData, NULL, 0, 0, Runs, 1, LAYOUT.MftClusters - 1,
                         (ULONGLONG)LAYOUT.MftClusters * C,
                         (ULONGLONG)LAYOUT.MftAllocRecords * MFT_RECORD_SIZE,
                         (ULONGLONG)LAYOUT.MftAllocRecords * MFT_RECORD_SIZE);
        /* $BITMAP (non-resident): one bit per reserved MFT record. */
        {
            ULONGLONG MftBmpBytes = ((ULONGLONG)LAYOUT.MftAllocRecords + 7) / 8;
            Runs[0].Lcn = LAYOUT.MftBitmapLcn; Runs[0].Len = LAYOUT.MftBitmapClusters;
            MkAddNonResident(&Rec, AttributeBitmap, NULL, 0, 0, Runs, 1, LAYOUT.MftBitmapClusters - 1,
                             (ULONGLONG)LAYOUT.MftBitmapClusters * C, MftBmpBytes, MftBmpBytes);
        }
        MkFinalize(&Rec);

        /* Record 1: $MFTMirr. */
        MkInit(&Rec, REC_BUF(MREC_MFTMIRR), MREC_MFTMIRR, MFT_RECORD_IN_USE);
        MkStdInfo(Val, FA_HIDDEN | FA_SYSTEM, Now, NTFS_SECURITY_ID);
        MkAddResident(&Rec, AttributeStandardInformation, NULL, 0, 0, Val, 72);
        Len = MkFileName(Val, MFT_REFERENCE(MREC_ROOT), FA_HIDDEN | FA_SYSTEM,
                         (ULONGLONG)LAYOUT.MftMirrClusters * C, (ULONGLONG)MFT_MIRR_COUNT * MFT_RECORD_SIZE,
                         Now, L"$MFTMirr", FILE_NAME_WIN32_AND_DOS);
        MkAddResident(&Rec, AttributeFileName, NULL, 0, RA_INDEXED, Val, Len);
        Runs[0].Lcn = LAYOUT.MftMirrLcn; Runs[0].Len = LAYOUT.MftMirrClusters;
        MkAddNonResident(&Rec, AttributeData, NULL, 0, 0, Runs, 1, LAYOUT.MftMirrClusters - 1,
                         (ULONGLONG)LAYOUT.MftMirrClusters * C,
                         (ULONGLONG)MFT_MIRR_COUNT * MFT_RECORD_SIZE,
                         (ULONGLONG)MFT_MIRR_COUNT * MFT_RECORD_SIZE);
        MkFinalize(&Rec);

        /* Record 2: $LogFile. */
        MkInit(&Rec, REC_BUF(MREC_LOGFILE), MREC_LOGFILE, MFT_RECORD_IN_USE);
        MkStdInfo(Val, FA_HIDDEN | FA_SYSTEM, Now, NTFS_SECURITY_ID);
        MkAddResident(&Rec, AttributeStandardInformation, NULL, 0, 0, Val, 72);
        Len = MkFileName(Val, MFT_REFERENCE(MREC_ROOT), FA_HIDDEN | FA_SYSTEM,
                         (ULONGLONG)LAYOUT.LogFileClusters * C, (ULONGLONG)LAYOUT.LogFileClusters * C,
                         Now, L"$LogFile", FILE_NAME_WIN32_AND_DOS);
        MkAddResident(&Rec, AttributeFileName, NULL, 0, RA_INDEXED, Val, Len);
        Runs[0].Lcn = LAYOUT.LogFileLcn; Runs[0].Len = LAYOUT.LogFileClusters;
        MkAddNonResident(&Rec, AttributeData, NULL, 0, 0, Runs, 1, LAYOUT.LogFileClusters - 1,
                         (ULONGLONG)LAYOUT.LogFileClusters * C, (ULONGLONG)LAYOUT.LogFileClusters * C,
                         (ULONGLONG)LAYOUT.LogFileClusters * C);
        MkFinalize(&Rec);

        /* Record 3: $Volume. */
        MkInit(&Rec, REC_BUF(MREC_VOLUME), MREC_VOLUME, MFT_RECORD_IN_USE);
        MkStdInfo(Val, FA_HIDDEN | FA_SYSTEM, Now, 0x101);
        MkAddResident(&Rec, AttributeStandardInformation, NULL, 0, 0, Val, 72);
        Len = MkFileName(Val, MFT_REFERENCE(MREC_ROOT), FA_HIDDEN | FA_SYSTEM,
                         0, 0, Now, L"$Volume", FILE_NAME_WIN32_AND_DOS);
        MkAddResident(&Rec, AttributeFileName, NULL, 0, RA_INDEXED, Val, Len);
        {
            BYTE  VName[128];
            ULONG VLen = 0;
            if (LABEL && LABEL->Buffer && LABEL->Length > 0)
            {
                VLen = LABEL->Length;
                if (VLen > 128)
                    VLen = 128;
                RtlCopyMemory(VName, LABEL->Buffer, VLen);
            }
            MkAddResident(&Rec, AttributeVolumeName, NULL, 0, 0, VName, VLen);
        }
        /* $VOLUME_INFORMATION: version 3.1, flags 0 (the 0x80 dirty bit left clear). */
        RtlZeroMemory(Val, 12);
        Val[0x08] = NTFS_MAJOR_VERSION;
        Val[0x09] = NTFS_MINOR_VERSION;
        MkAddResident(&Rec, AttributeVolumeInformation, NULL, 0, 0, Val, 12);
        MkAddResident(&Rec, AttributeData, NULL, 0, 0, NULL, 0);   /* empty $DATA */
        MkFinalize(&Rec);

        /* Record 4: $AttrDef. */
        MkInit(&Rec, REC_BUF(MREC_ATTRDEF), MREC_ATTRDEF, MFT_RECORD_IN_USE);
        MkStdInfo(Val, FA_HIDDEN | FA_SYSTEM, Now, NTFS_SECURITY_ID);
        MkAddResident(&Rec, AttributeStandardInformation, NULL, 0, 0, Val, 72);
        Len = MkFileName(Val, MFT_REFERENCE(MREC_ROOT), FA_HIDDEN | FA_SYSTEM,
                         (ULONGLONG)LAYOUT.AttrDefClusters * C,
                         sizeof(ATTRIBUTES_TABLE) - sizeof(ATTRIBUTES_TABLE[0]),
                         Now, L"$AttrDef", FILE_NAME_WIN32_AND_DOS);
        MkAddResident(&Rec, AttributeFileName, NULL, 0, RA_INDEXED, Val, Len);
        Runs[0].Lcn = LAYOUT.AttrDefLcn; Runs[0].Len = LAYOUT.AttrDefClusters;
        MkAddNonResident(&Rec, AttributeData, NULL, 0, 0, Runs, 1, LAYOUT.AttrDefClusters - 1,
                         (ULONGLONG)LAYOUT.AttrDefClusters * C,
                         sizeof(ATTRIBUTES_TABLE), sizeof(ATTRIBUTES_TABLE));
        MkFinalize(&Rec);

        /* Record 6: $Bitmap. The stream length must be a multiple of 8 bytes
         * (NTFS tracks the cluster bitmap in 64-bit groups); an unaligned length
         * gets rounded up and zero-filled by the OS, freeing phantom clusters. */
        {
            ULONGLONG BitmapBytes = (((ClusterCount + 7) / 8) + 7) & ~7ULL;
            MkInit(&Rec, REC_BUF(MREC_BITMAP), MREC_BITMAP, MFT_RECORD_IN_USE);
            MkStdInfo(Val, FA_HIDDEN | FA_SYSTEM, Now, NTFS_SECURITY_ID);
            MkAddResident(&Rec, AttributeStandardInformation, NULL, 0, 0, Val, 72);
            Len = MkFileName(Val, MFT_REFERENCE(MREC_ROOT), FA_HIDDEN | FA_SYSTEM,
                             (ULONGLONG)LAYOUT.BitmapClusters * C, BitmapBytes,
                             Now, L"$Bitmap", FILE_NAME_WIN32_AND_DOS);
            MkAddResident(&Rec, AttributeFileName, NULL, 0, RA_INDEXED, Val, Len);
            Runs[0].Lcn = LAYOUT.BitmapLcn; Runs[0].Len = LAYOUT.BitmapClusters;
            MkAddNonResident(&Rec, AttributeData, NULL, 0, 0, Runs, 1, LAYOUT.BitmapClusters - 1,
                             (ULONGLONG)LAYOUT.BitmapClusters * C, BitmapBytes, BitmapBytes);
            MkFinalize(&Rec);
        }

        /* Record 7: $Boot (short STD_INFO + embedded default descriptor 0x100). */
        MkInit(&Rec, REC_BUF(MREC_BOOT), MREC_BOOT, MFT_RECORD_IN_USE);
        MkStdInfoShort(Val, FA_HIDDEN | FA_SYSTEM, Now);
        MkAddResident(&Rec, AttributeStandardInformation, NULL, 0, 0, Val, 48);
        Len = MkFileName(Val, MFT_REFERENCE(MREC_ROOT), FA_HIDDEN | FA_SYSTEM,
                         (ULONGLONG)LAYOUT.BootClusters * C, 0x2000,
                         Now, L"$Boot", FILE_NAME_WIN32_AND_DOS);
        MkAddResident(&Rec, AttributeFileName, NULL, 0, RA_INDEXED, Val, Len);
        MkAddResident(&Rec, AttributeSecurityDescriptor, NULL, 0, 0,
                      DefaultSds + SdBlobOff[0], DefaultSdLen[0]);
        Runs[0].Lcn = LAYOUT.BootLcn; Runs[0].Len = LAYOUT.BootClusters;
        MkAddNonResident(&Rec, AttributeData, NULL, 0, 0, Runs, 1, LAYOUT.BootClusters - 1,
                         (ULONGLONG)LAYOUT.BootClusters * C, 0x2000, 0x2000);
        MkFinalize(&Rec);

        /* Record 8: $BadClus (empty $DATA + sparse whole-volume $Bad hole). */
        MkInit(&Rec, REC_BUF(MREC_BADCLUS), MREC_BADCLUS, MFT_RECORD_IN_USE);
        MkStdInfo(Val, FA_HIDDEN | FA_SYSTEM, Now, NTFS_SECURITY_ID);
        MkAddResident(&Rec, AttributeStandardInformation, NULL, 0, 0, Val, 72);
        Len = MkFileName(Val, MFT_REFERENCE(MREC_ROOT), FA_HIDDEN | FA_SYSTEM,
                         0, 0, Now, L"$BadClus", FILE_NAME_WIN32_AND_DOS);
        MkAddResident(&Rec, AttributeFileName, NULL, 0, RA_INDEXED, Val, Len);
        MkAddResident(&Rec, AttributeData, NULL, 0, 0, NULL, 0);
        /* $Bad is a plain (non-sparse) non-resident attribute whose single
         * mapping-pair is a hole; alloc/real = whole volume, init = 0. */
        Runs[0].Lcn = HOLE_LCN; Runs[0].Len = ClusterCount;
        MkAddNonResident(&Rec, AttributeData, L"$Bad", 4, 0, Runs, 1, ClusterCount - 1,
                         ClusterCount * C, ClusterCount * C, 0);
        MkFinalize(&Rec);

        /* Record 9: $Secure ($SDS + large $SDH + resident $SII). */
        {
            BYTE          SdsHdr[NTFS_DEFAULT_SD_COUNT][0x14];
            BYTE          SiiKeys[NTFS_DEFAULT_SD_COUNT][4];
            BYTE          SdhKeys[NTFS_DEFAULT_SD_COUNT][8];
            MK_VIEW_ENTRY SiiEnt[NTFS_DEFAULT_SD_COUNT];
            BYTE          ViewRoot[512];

            for (i = 0; i < NTFS_DEFAULT_SD_COUNT; i++)
            {
                ULONG Id = 0x100 + i;
                WR32(SdsHdr[i], 0x00, SdHash[i]);
                WR32(SdsHdr[i], 0x04, Id);
                WR64(SdsHdr[i], 0x08, SdSdsOff[i]);
                WR32(SdsHdr[i], 0x10, 0x14 + DefaultSdLen[i]);
                WR32(SiiKeys[i], 0, Id);
                WR32(SdhKeys[i], 0, SdHash[i]);
                WR32(SdhKeys[i], 4, Id);
                SiiEnt[i].Key = SiiKeys[i]; SiiEnt[i].KeyLen = 4;
                SiiEnt[i].Data = SdsHdr[i]; SiiEnt[i].DataLen = 0x14;
            }

            MkInit(&Rec, REC_BUF(MREC_SECURE), MREC_SECURE, MFT_RECORD_IN_USE | MFT_RECORD_VIEW_INDEX);
            MkStdInfo(Val, FA_HIDDEN | FA_SYSTEM | FA_VIEW, Now, 0x101);
            MkAddResident(&Rec, AttributeStandardInformation, NULL, 0, 0, Val, 72);
            Len = MkFileName(Val, MFT_REFERENCE(MREC_ROOT), FA_HIDDEN | FA_SYSTEM | FA_VIEW,
                             0, 0, Now, L"$Secure", FILE_NAME_WIN32_AND_DOS);
            MkAddResident(&Rec, AttributeFileName, NULL, 0, RA_INDEXED, Val, Len);
            /* $DATA:$SDS (non-resident: 8 descriptors + a 256 KiB-offset mirror). */
            Runs[0].Lcn = LAYOUT.SdsLcn; Runs[0].Len = LAYOUT.SdsClusters;
            MkAddNonResident(&Rec, AttributeData, L"$SDS", 4, 0, Runs, 1, LAYOUT.SdsClusters - 1,
                             (ULONGLONG)LAYOUT.SdsClusters * C,
                             (ULONGLONG)NTFS_SDS_MIRROR + SdsContentLen,
                             (ULONGLONG)NTFS_SDS_MIRROR + SdsContentLen);
            /* Attribute order (type, name): $SDH root, $SII root, then $SDH alloc, $SDH bitmap. */
            Len = MkLargeViewRoot(ViewRoot, COLLATION_NTOFS_SECURITY_HASH);
            MkAddResident(&Rec, AttributeIndexRoot, L"$SDH", 4, 0, ViewRoot, Len);
            Len = MkViewRootN(ViewRoot, COLLATION_NTOFS_ULONG, SiiEnt, NTFS_DEFAULT_SD_COUNT);
            MkAddResident(&Rec, AttributeIndexRoot, L"$SII", 4, 0, ViewRoot, Len);
            Runs[0].Lcn = LAYOUT.SdhIdxLcn; Runs[0].Len = LAYOUT.SdhIdxClusters;
            MkAddNonResident(&Rec, AttributeIndexAllocation, L"$SDH", 4, 0, Runs, 1,
                             LAYOUT.SdhIdxClusters - 1, (ULONGLONG)LAYOUT.SdhIdxClusters * C,
                             INDEX_RECORD_SIZE, INDEX_RECORD_SIZE);
            RtlZeroMemory(Val, 8);
            Val[0] = 0x01;
            MkAddResident(&Rec, AttributeBitmap, L"$SDH", 4, 0, Val, 8);
            MkFinalize(&Rec);
        }

        /* Record 10: $UpCase (+ $Info CRC stream). */
        MkInit(&Rec, REC_BUF(MREC_UPCASE), MREC_UPCASE, MFT_RECORD_IN_USE);
        MkStdInfo(Val, FA_HIDDEN | FA_SYSTEM, Now, NTFS_SECURITY_ID);
        MkAddResident(&Rec, AttributeStandardInformation, NULL, 0, 0, Val, 72);
        Len = MkFileName(Val, MFT_REFERENCE(MREC_ROOT), FA_HIDDEN | FA_SYSTEM,
                         sizeof(UPCASE_TABLE), sizeof(UPCASE_TABLE),
                         Now, L"$UpCase", FILE_NAME_WIN32_AND_DOS);
        MkAddResident(&Rec, AttributeFileName, NULL, 0, RA_INDEXED, Val, Len);
        Runs[0].Lcn = LAYOUT.UpCaseLcn; Runs[0].Len = LAYOUT.UpCaseClusters;
        MkAddNonResident(&Rec, AttributeData, NULL, 0, 0, Runs, 1, LAYOUT.UpCaseClusters - 1,
                         (ULONGLONG)LAYOUT.UpCaseClusters * C, sizeof(UPCASE_TABLE), sizeof(UPCASE_TABLE));
        MkAddResident(&Rec, AttributeData, L"$Info", 5, 0, UpCaseInfo, sizeof(UpCaseInfo));
        MkFinalize(&Rec);

        /* ---- $Extend children (build children before their index parents) --- */

        /* Record 24: $Quota (default quota entries). */
        {
            BYTE          Qce[64];
            BYTE          QKey1[4];
            BYTE          QKey256[4];
            BYTE          OData[4];
            BYTE          QIdx[256];
            BYTE          OIdx[128];
            MK_VIEW_ENTRY QEnt[2];
            MK_VIEW_ENTRY OEnt[1];

            RtlZeroMemory(Qce, sizeof(Qce));
            WR32(Qce, 0x00, 2);                        /* version */
            WR32(Qce, 0x04, 1);                        /* flags */
            WR64(Qce, 0x08, 0);                        /* bytes used */
            WR64(Qce, 0x10, Now);                      /* change time */
            WR64(Qce, 0x18, 0xFFFFFFFFFFFFFFFFULL);    /* threshold */
            WR64(Qce, 0x20, 0xFFFFFFFFFFFFFFFFULL);    /* limit */
            WR64(Qce, 0x28, 0);                        /* exceeded time */
            RtlCopyMemory(Qce + 48, SidAdmins, 16);    /* id 0x100 carries the SID */

            WR32(QKey1, 0, 1);
            WR32(QKey256, 0, 0x100);
            WR32(OData, 0, 0x100);

            QEnt[0].Key = QKey1;   QEnt[0].KeyLen = 4;  QEnt[0].Data = Qce; QEnt[0].DataLen = 48;
            QEnt[1].Key = QKey256; QEnt[1].KeyLen = 4;  QEnt[1].Data = Qce; QEnt[1].DataLen = 64;
            OEnt[0].Key = SidAdmins; OEnt[0].KeyLen = 16; OEnt[0].Data = OData; OEnt[0].DataLen = 4;

            MkInit(&Rec, REC_BUF(MREC_QUOTA), MREC_QUOTA,
                   MFT_RECORD_IN_USE | MFT_RECORD_UNKNOWN1 | MFT_RECORD_VIEW_INDEX);
            MkStdInfo(Val, FA_HIDDEN | FA_SYSTEM | FA_VIEW, Now, 0x101);
            MkAddResident(&Rec, AttributeStandardInformation, NULL, 0, 0, Val, 72);
            Len = MkFileName(Val, MFT_REFERENCE(MREC_EXTEND), FA_HIDDEN | FA_SYSTEM | FA_VIEW,
                             0, 0, Now, L"$Quota", FILE_NAME_POSIX);
            MkAddResident(&Rec, AttributeFileName, NULL, 0, RA_INDEXED, Val, Len);
            Len = MkViewRootN(OIdx, COLL_SID, OEnt, 1);
            MkAddResident(&Rec, AttributeIndexRoot, L"$O", 2, 0, OIdx, Len);
            Len = MkViewRootN(QIdx, COLLATION_NTOFS_ULONG, QEnt, 2);
            MkAddResident(&Rec, AttributeIndexRoot, L"$Q", 2, 0, QIdx, Len);
            MkFinalize(&Rec);
        }

        /* Record 25: $ObjId (empty $O). */
        MkInit(&Rec, REC_BUF(MREC_OBJID), MREC_OBJID,
               MFT_RECORD_IN_USE | MFT_RECORD_UNKNOWN1 | MFT_RECORD_VIEW_INDEX);
        MkStdInfo(Val, FA_HIDDEN | FA_SYSTEM | FA_VIEW, Now, 0x101);
        MkAddResident(&Rec, AttributeStandardInformation, NULL, 0, 0, Val, 72);
        Len = MkFileName(Val, MFT_REFERENCE(MREC_EXTEND), FA_HIDDEN | FA_SYSTEM | FA_VIEW,
                         0, 0, Now, L"$ObjId", FILE_NAME_POSIX);
        MkAddResident(&Rec, AttributeFileName, NULL, 0, RA_INDEXED, Val, Len);
        Len = MkEmptyViewRoot(Val, COLL_ULONGS);
        MkAddResident(&Rec, AttributeIndexRoot, L"$O", 2, 0, Val, Len);
        MkFinalize(&Rec);

        /* Record 26: $Reparse (empty $R). */
        MkInit(&Rec, REC_BUF(MREC_REPARSE), MREC_REPARSE,
               MFT_RECORD_IN_USE | MFT_RECORD_UNKNOWN1 | MFT_RECORD_VIEW_INDEX);
        MkStdInfo(Val, FA_HIDDEN | FA_SYSTEM | FA_VIEW, Now, 0x101);
        MkAddResident(&Rec, AttributeStandardInformation, NULL, 0, 0, Val, 72);
        Len = MkFileName(Val, MFT_REFERENCE(MREC_EXTEND), FA_HIDDEN | FA_SYSTEM | FA_VIEW,
                         0, 0, Now, L"$Reparse", FILE_NAME_POSIX);
        MkAddResident(&Rec, AttributeFileName, NULL, 0, RA_INDEXED, Val, Len);
        Len = MkEmptyViewRoot(Val, COLL_ULONGS);
        MkAddResident(&Rec, AttributeIndexRoot, L"$R", 2, 0, Val, Len);
        MkFinalize(&Rec);

        /* Records 12..15: reserved stubs - in use, 0 hard links, short STD_INFO,
         * embedded default descriptor 0x101, empty resident unnamed $DATA. */
        for (i = 12; i <= 15; i++)
        {
            MkInit(&Rec, REC_BUF(i), i, MFT_RECORD_IN_USE);
            MkSetHardLinks(&Rec, 0);
            MkStdInfoShort(Val, FA_HIDDEN | FA_SYSTEM, Now);
            MkAddResident(&Rec, AttributeStandardInformation, NULL, 0, 0, Val, 48);
            MkAddResident(&Rec, AttributeSecurityDescriptor, NULL, 0, 0,
                          DefaultSds + SdBlobOff[1], DefaultSdLen[1]);
            MkAddResident(&Rec, AttributeData, NULL, 0, 0, NULL, 0);
            MkFinalize(&Rec);
        }

        /* ---- $Extend/$RmMetadata TxF subtree ------------------------------ */

        /* Record 28: $Repair - empty non-resident $DATA + $DATA:$Config. */
        MkInit(&Rec, REC_BUF(MREC_REPAIR), MREC_REPAIR, MFT_RECORD_IN_USE | MFT_RECORD_UNKNOWN1);
        MkStdInfo(Val, FA_HIDDEN | FA_SYSTEM, Now, 0x101);
        MkAddResident(&Rec, AttributeStandardInformation, NULL, 0, 0, Val, 72);
        Len = MkFileName(Val, MFT_REFERENCE(MREC_RMMETA), FA_HIDDEN | FA_SYSTEM,
                         0, 0, Now, L"$Repair", FILE_NAME_POSIX);
        MkAddResident(&Rec, AttributeFileName, NULL, 0, RA_INDEXED, Val, Len);
        MkAddNonResident(&Rec, AttributeData, NULL, 0, 0, NULL, 0, (ULONGLONG)-1, 0, 0, 0);
        MkAddResident(&Rec, AttributeData, L"$Config", 7, 0, RepairCfg, sizeof(RepairCfg));
        MkFinalize(&Rec);

        /* Record 29: $Deleted - empty dir with a pre-allocated (unused) 64 KiB index. */
        MkInit(&Rec, REC_BUF(MREC_DELETED), MREC_DELETED,
               MFT_RECORD_IN_USE | MFT_RECORD_IS_DIRECTORY | MFT_RECORD_UNKNOWN1);
        MkStdInfo(Val, FA_HIDDEN | FA_SYSTEM, Now, 0x101);
        MkAddResident(&Rec, AttributeStandardInformation, NULL, 0, 0, Val, 72);
        Len = MkFileName(Val, MFT_REFERENCE(MREC_EXTEND), FA_HIDDEN | FA_SYSTEM | FA_DIR,
                         0, 0, Now, L"$Deleted", FILE_NAME_POSIX);
        MkAddResident(&Rec, AttributeFileName, NULL, 0, RA_INDEXED, Val, Len);
        Len = MkEmptyI30Root(Val);
        MkAddResident(&Rec, AttributeIndexRoot, L"$I30", 4, 0, Val, Len);
        Runs[0].Lcn = LAYOUT.DeletedIdxLcn; Runs[0].Len = LAYOUT.DeletedIdxClusters;
        MkAddNonResident(&Rec, AttributeIndexAllocation, L"$I30", 4, 0, Runs, 1,
                         LAYOUT.DeletedIdxClusters - 1, (ULONGLONG)LAYOUT.DeletedIdxClusters * C, 0, 0);
        RtlZeroMemory(Val, 8);
        MkAddResident(&Rec, AttributeBitmap, L"$I30", 4, 0, Val, 8);
        MkFinalize(&Rec);

        /* Record 31: $Txf - empty dir + $TXF_DATA. */
        MkInit(&Rec, REC_BUF(MREC_TXF), MREC_TXF, MFT_RECORD_IN_USE | MFT_RECORD_IS_DIRECTORY);
        MkStdInfo(Val, 0x80000006, Now, 0x102);
        MkAddResident(&Rec, AttributeStandardInformation, NULL, 0, 0, Val, 72);
        Len = MkFileName(Val, MFT_REFERENCE(MREC_RMMETA), FA_HIDDEN | FA_SYSTEM | FA_DIR,
                         0, 0, Now, L"$Txf", FILE_NAME_POSIX);
        MkAddResident(&Rec, AttributeFileName, NULL, 0, RA_INDEXED, Val, Len);
        Len = MkEmptyI30Root(Val);
        MkAddResident(&Rec, AttributeIndexRoot, L"$I30", 4, 0, Val, Len);
        MkAddResident(&Rec, AttributeLoggedUtilityStream, L"$TXF_DATA", 9, 0, TxfInfo, sizeof(TxfInfo));
        MkFinalize(&Rec);

        /* Record 32: $Tops - resident $DATA + $DATA:$T (1 MiB). */
        MkInit(&Rec, REC_BUF(MREC_TOPS), MREC_TOPS, MFT_RECORD_IN_USE);
        MkStdInfo(Val, FA_HIDDEN | FA_SYSTEM, Now, 0x102);
        MkAddResident(&Rec, AttributeStandardInformation, NULL, 0, 0, Val, 72);
        Len = MkFileName(Val, MFT_REFERENCE(MREC_TXFLOG), FA_HIDDEN | FA_SYSTEM,
                         0, 0, Now, L"$Tops", FILE_NAME_POSIX);
        MkAddResident(&Rec, AttributeFileName, NULL, 0, RA_INDEXED, Val, Len);
        MkAddResident(&Rec, AttributeData, NULL, 0, 0, TopsData, sizeof(TopsData));
        Runs[0].Lcn = LAYOUT.TopsTLcn; Runs[0].Len = LAYOUT.TopsTClusters;
        MkAddNonResident(&Rec, AttributeData, L"$T", 2, 0, Runs, 1, LAYOUT.TopsTClusters - 1,
                         (ULONGLONG)LAYOUT.TopsTClusters * C,
                         (ULONGLONG)LAYOUT.TopsTClusters * C, (ULONGLONG)LAYOUT.TopsTClusters * C);
        MkFinalize(&Rec);

        /* Record 33: $TxfLog.blf (64 KiB). */
        MkInit(&Rec, REC_BUF(MREC_TXFBLF), MREC_TXFBLF, MFT_RECORD_IN_USE);
        MkStdInfo(Val, FA_ARCHIVE, Now, 0x103);
        MkAddResident(&Rec, AttributeStandardInformation, NULL, 0, 0, Val, 72);
        Len = MkFileName(Val, MFT_REFERENCE(MREC_TXFLOG), FA_ARCHIVE,
                         (ULONGLONG)LAYOUT.BlfClusters * C, 0, Now, L"$TxfLog.blf", FILE_NAME_POSIX);
        MkAddResident(&Rec, AttributeFileName, NULL, 0, RA_INDEXED, Val, Len);
        Runs[0].Lcn = LAYOUT.BlfLcn; Runs[0].Len = LAYOUT.BlfClusters;
        MkAddNonResident(&Rec, AttributeData, NULL, 0, 0, Runs, 1, LAYOUT.BlfClusters - 1,
                         (ULONGLONG)LAYOUT.BlfClusters * C,
                         (ULONGLONG)LAYOUT.BlfClusters * C, (ULONGLONG)LAYOUT.BlfClusters * C);
        MkFinalize(&Rec);

        /* Records 34/35: $TxfLogContainer...0001 / ...0002 (2 MiB each). */
        MkInit(&Rec, REC_BUF(MREC_CONT1), MREC_CONT1, MFT_RECORD_IN_USE);
        MkStdInfo(Val, FA_ARCHIVE, Now, 0x104);
        MkAddResident(&Rec, AttributeStandardInformation, NULL, 0, 0, Val, 72);
        Len = MkFileName(Val, MFT_REFERENCE(MREC_TXFLOG), FA_ARCHIVE,
                         (ULONGLONG)LAYOUT.Cont1Clusters * C, 0, Now,
                         L"$TxfLogContainer00000000000000000001", FILE_NAME_POSIX);
        MkAddResident(&Rec, AttributeFileName, NULL, 0, RA_INDEXED, Val, Len);
        Runs[0].Lcn = LAYOUT.Cont1Lcn; Runs[0].Len = LAYOUT.Cont1Clusters;
        MkAddNonResident(&Rec, AttributeData, NULL, 0, 0, Runs, 1, LAYOUT.Cont1Clusters - 1,
                         (ULONGLONG)LAYOUT.Cont1Clusters * C,
                         (ULONGLONG)LAYOUT.Cont1Clusters * C, (ULONGLONG)LAYOUT.Cont1Clusters * C);
        MkFinalize(&Rec);

        MkInit(&Rec, REC_BUF(MREC_CONT2), MREC_CONT2, MFT_RECORD_IN_USE);
        MkStdInfo(Val, FA_ARCHIVE, Now, 0x104);
        MkAddResident(&Rec, AttributeStandardInformation, NULL, 0, 0, Val, 72);
        Len = MkFileName(Val, MFT_REFERENCE(MREC_TXFLOG), FA_ARCHIVE,
                         (ULONGLONG)LAYOUT.Cont2Clusters * C, 0, Now,
                         L"$TxfLogContainer00000000000000000002", FILE_NAME_POSIX);
        MkAddResident(&Rec, AttributeFileName, NULL, 0, RA_INDEXED, Val, Len);
        Runs[0].Lcn = LAYOUT.Cont2Lcn; Runs[0].Len = LAYOUT.Cont2Clusters;
        MkAddNonResident(&Rec, AttributeData, NULL, 0, 0, Runs, 1, LAYOUT.Cont2Clusters - 1,
                         (ULONGLONG)LAYOUT.Cont2Clusters * C,
                         (ULONGLONG)LAYOUT.Cont2Clusters * C, (ULONGLONG)LAYOUT.Cont2Clusters * C);
        MkFinalize(&Rec);

        /* Record 30: $TxfLog directory, indexing $Tops/$TxfLog.blf/containers. */
        {
            static const ULONG TxfLogOrder[] = { MREC_TOPS, MREC_TXFBLF, MREC_CONT1, MREC_CONT2 };

            MkInit(&Rec, REC_BUF(MREC_TXFLOG), MREC_TXFLOG, MFT_RECORD_IN_USE | MFT_RECORD_IS_DIRECTORY);
            MkStdInfo(Val, FA_HIDDEN | FA_SYSTEM, Now, 0x102);
            MkAddResident(&Rec, AttributeStandardInformation, NULL, 0, 0, Val, 72);
            Len = MkFileName(Val, MFT_REFERENCE(MREC_RMMETA), FA_HIDDEN | FA_SYSTEM | FA_DIR,
                             0, 0, Now, L"$TxfLog", FILE_NAME_POSIX);
            MkAddResident(&Rec, AttributeFileName, NULL, 0, RA_INDEXED, Val, Len);
            Len = MkI30RootWithEntries(Val, Mft, TxfLogOrder, ARR_SIZE(TxfLogOrder));
            MkAddResident(&Rec, AttributeIndexRoot, L"$I30", 4, 0, Val, Len);
            MkFinalize(&Rec);
        }

        /* Record 27: $RmMetadata directory, indexing $Repair/$Txf/$TxfLog. */
        {
            static const ULONG RmOrder[] = { MREC_REPAIR, MREC_TXF, MREC_TXFLOG };

            MkInit(&Rec, REC_BUF(MREC_RMMETA), MREC_RMMETA, MFT_RECORD_IN_USE | MFT_RECORD_IS_DIRECTORY);
            MkStdInfo(Val, FA_HIDDEN | FA_SYSTEM, Now, 0x102);
            MkAddResident(&Rec, AttributeStandardInformation, NULL, 0, 0, Val, 72);
            Len = MkFileName(Val, MFT_REFERENCE(MREC_EXTEND), FA_HIDDEN | FA_SYSTEM | FA_DIR,
                             0, 0, Now, L"$RmMetadata", FILE_NAME_POSIX);
            MkAddResident(&Rec, AttributeFileName, NULL, 0, RA_INDEXED, Val, Len);
            Len = MkI30RootWithEntries(Val, Mft, RmOrder, ARR_SIZE(RmOrder));
            MkAddResident(&Rec, AttributeIndexRoot, L"$I30", 4, 0, Val, Len);
            MkFinalize(&Rec);
        }

        /* Record 11: $Extend directory, indexing all children (collation order
         * $Deleted < $ObjId < $Quota < $Reparse < $RmMetadata). The directory
         * bit is set in FILE_NAME but not in STD_INFO. */
        {
            static const ULONG ExtendOrder[] =
                { MREC_DELETED, MREC_OBJID, MREC_QUOTA, MREC_REPARSE, MREC_RMMETA };

            MkInit(&Rec, REC_BUF(MREC_EXTEND), MREC_EXTEND, MFT_RECORD_IN_USE | MFT_RECORD_IS_DIRECTORY);
            MkStdInfo(Val, FA_HIDDEN | FA_SYSTEM, Now, 0x101);
            MkAddResident(&Rec, AttributeStandardInformation, NULL, 0, 0, Val, 72);
            Len = MkFileName(Val, MFT_REFERENCE(MREC_ROOT), FA_HIDDEN | FA_SYSTEM | FA_DIR,
                             0, 0, Now, L"$Extend", FILE_NAME_WIN32_AND_DOS);
            MkAddResident(&Rec, AttributeFileName, NULL, 0, RA_INDEXED, Val, Len);
            Len = MkI30RootWithEntries(Val, Mft, ExtendOrder, ARR_SIZE(ExtendOrder));
            MkAddResident(&Rec, AttributeIndexRoot, L"$I30", 4, 0, Val, Len);
            MkFinalize(&Rec);
        }

        /* Record 5: root "." - large $I30 index + embedded (unique) SD + $TXF_DATA. */
        MkInit(&Rec, REC_BUF(MREC_ROOT), MREC_ROOT, MFT_RECORD_IN_USE | MFT_RECORD_IS_DIRECTORY);
        MkStdInfoShort(Val, FA_HIDDEN | FA_SYSTEM, Now);
        MkAddResident(&Rec, AttributeStandardInformation, NULL, 0, 0, Val, 48);
        Len = MkFileName(Val, MFT_REFERENCE(MREC_ROOT), FA_HIDDEN | FA_SYSTEM | FA_DIR,
                         0, 0, Now, L".", FILE_NAME_WIN32_AND_DOS);
        MkAddResident(&Rec, AttributeFileName, NULL, 0, RA_INDEXED, Val, Len);
        MkAddResident(&Rec, AttributeSecurityDescriptor, NULL, 0, 0, RootSdValue, sizeof(RootSdValue));
        Len = MkLargeI30Root(Val);
        MkAddResident(&Rec, AttributeIndexRoot, L"$I30", 4, 0, Val, Len);
        Runs[0].Lcn = LAYOUT.RootIdxLcn; Runs[0].Len = LAYOUT.RootIdxClusters;
        MkAddNonResident(&Rec, AttributeIndexAllocation, L"$I30", 4, 0, Runs, 1,
                         LAYOUT.RootIdxClusters - 1, (ULONGLONG)LAYOUT.RootIdxClusters * C,
                         INDEX_RECORD_SIZE, INDEX_RECORD_SIZE);
        RtlZeroMemory(Val, 8);
        Val[0] = 0x01;                          /* INDX VCN 0 allocated */
        MkAddResident(&Rec, AttributeBitmap, L"$I30", 4, 0, Val, 8);
        MkAddResident(&Rec, AttributeLoggedUtilityStream, L"$TXF_DATA", 9, 0,
                      TxfDataValue, sizeof(TxfDataValue));
        MkFinalize(&Rec);

        #undef REC_BUF
    }

    /* --- Emit everything to disk ---------------------------------------- */

    /* Zero the whole reserved $MFT region, then lay the 36 built records over
     * the start of it. Records 16..23 and 36+ are left zero (no FILE record). */
    Status = WriteZerosToClusters(LAYOUT.MftLcn, LAYOUT.MftClusters);
    if (!NT_SUCCESS(Status)) { DPRINT1("ERROR: clear $MFT (Status %lx)\n", Status); goto done; }

    Status = WriteAt(LAYOUT.MftLcn, Mft, MREC_COUNT * MFT_RECORD_SIZE);
    if (!NT_SUCCESS(Status)) { DPRINT1("ERROR: write $MFT (Status %lx)\n", Status); goto done; }

    /* $MFTMirr = the first four records. */
    Status = WriteRegion(LAYOUT.MftMirrLcn, LAYOUT.MftMirrClusters, Mft, MFT_MIRR_COUNT * MFT_RECORD_SIZE);
    if (!NT_SUCCESS(Status)) { DPRINT1("ERROR: write $MFTMirr (Status %lx)\n", Status); goto done; }

    /* $LogFile: fill 0xFF (a driver treats an all-0xFF log as uninitialized). */
    Status = WritePatternToClusters(LAYOUT.LogFileLcn, LAYOUT.LogFileClusters, 0xFF);
    if (!NT_SUCCESS(Status)) { DPRINT1("ERROR: write $LogFile (Status %lx)\n", Status); goto done; }

    /* $AttrDef. */
    Status = WriteRegion(LAYOUT.AttrDefLcn, LAYOUT.AttrDefClusters, &ATTRIBUTES_TABLE, sizeof(ATTRIBUTES_TABLE));
    if (!NT_SUCCESS(Status)) { DPRINT1("ERROR: write $AttrDef (Status %lx)\n", Status); goto done; }

    /* $UpCase. */
    Status = WriteRegion(LAYOUT.UpCaseLcn, LAYOUT.UpCaseClusters, &UPCASE_TABLE, sizeof(UPCASE_TABLE));
    if (!NT_SUCCESS(Status)) { DPRINT1("ERROR: write $UpCase (Status %lx)\n", Status); goto done; }

    /* $Secure:$SDS - the 8 descriptors (16-byte aligned) with a duplicate copy
     * of the whole set placed 256 KiB further on. */
    {
        ULONG Size = LAYOUT.SdsClusters * BYTES_PER_CLUSTER;
        PBYTE Buf = RtlAllocateHeap(RtlGetProcessHeap(), 0, Size);
        if (!Buf) { Status = STATUS_INSUFFICIENT_RESOURCES; goto done; }
        RtlZeroMemory(Buf, Size);
        for (i = 0; i < NTFS_DEFAULT_SD_COUNT; i++)
        {
            ULONG EntryLen = 0x14 + DefaultSdLen[i];
            PBYTE E = Buf + SdSdsOff[i];
            WR32(E, 0x00, SdHash[i]);
            WR32(E, 0x04, 0x100 + i);
            WR64(E, 0x08, SdSdsOff[i]);
            WR32(E, 0x10, EntryLen);
            RtlCopyMemory(E + 0x14, DefaultSds + SdBlobOff[i], DefaultSdLen[i]);
            /* The copy is identical to the primary, offset field included (it
             * keeps pointing at the primary, not at the copy). */
            RtlCopyMemory(Buf + NTFS_SDS_MIRROR + SdSdsOff[i], E, EntryLen);
        }
        Status = WriteAt(LAYOUT.SdsLcn, Buf, Size);
        FREE(Buf);
        if (!NT_SUCCESS(Status)) { DPRINT1("ERROR: write $SDS (Status %lx)\n", Status); goto done; }
    }

    /* $Secure:$SDH INDX block (8 entries keyed by {hash,id}, hash order). */
    {
        BYTE          SdsHdr[NTFS_DEFAULT_SD_COUNT][0x14];
        BYTE          SdhKeys[NTFS_DEFAULT_SD_COUNT][8];
        MK_VIEW_ENTRY SdhEnt[NTFS_DEFAULT_SD_COUNT];
        PBYTE         Indx;

        for (i = 0; i < NTFS_DEFAULT_SD_COUNT; i++)
        {
            ULONG Id = 0x100 + i;
            WR32(SdsHdr[i], 0x00, SdHash[i]);
            WR32(SdsHdr[i], 0x04, Id);
            WR64(SdsHdr[i], 0x08, SdSdsOff[i]);
            WR32(SdsHdr[i], 0x10, 0x14 + DefaultSdLen[i]);
            WR32(SdhKeys[i], 0, SdHash[i]);
            WR32(SdhKeys[i], 4, Id);
        }
        for (i = 0; i < NTFS_DEFAULT_SD_COUNT; i++)
        {
            BYTE J = SdhOrder[i];
            SdhEnt[i].Key = SdhKeys[J]; SdhEnt[i].KeyLen = 8;
            SdhEnt[i].Data = SdsHdr[J]; SdhEnt[i].DataLen = 0x14;
        }

        Indx = RtlAllocateHeap(RtlGetProcessHeap(), 0, INDEX_RECORD_SIZE);
        if (!Indx) { Status = STATUS_INSUFFICIENT_RESOURCES; goto done; }
        MkViewIndxBlock(Indx, SdhEnt, NTFS_DEFAULT_SD_COUNT);
        Status = WriteRegion(LAYOUT.SdhIdxLcn, LAYOUT.SdhIdxClusters, Indx, INDEX_RECORD_SIZE);
        FREE(Indx);
        if (!NT_SUCCESS(Status)) { DPRINT1("ERROR: write $SDH (Status %lx)\n", Status); goto done; }
    }

    /* $MFT:$BITMAP - records 0..15 and 24..35 in use. */
    {
        ULONG Size = LAYOUT.MftBitmapClusters * BYTES_PER_CLUSTER;
        PBYTE Buf = RtlAllocateHeap(RtlGetProcessHeap(), 0, Size);
        if (!Buf) { Status = STATUS_INSUFFICIENT_RESOURCES; goto done; }
        RtlZeroMemory(Buf, Size);
        Buf[0] = 0xFF; Buf[1] = 0xFF;   /* records 0..15  */
        Buf[3] = 0xFF; Buf[4] = 0x0F;   /* records 24..35 */
        Status = WriteAt(LAYOUT.MftBitmapLcn, Buf, Size);
        FREE(Buf);
        if (!NT_SUCCESS(Status)) { DPRINT1("ERROR: write $MFT:$BITMAP (Status %lx)\n", Status); goto done; }
    }

    /* Cluster $Bitmap: metadata clusters [0, FirstFreeLcn) used, the rest free,
     * and the bits past ClusterCount (up to the 8-byte-aligned end) set. */
    {
        ULONG     Size = LAYOUT.BitmapClusters * BYTES_PER_CLUSTER;
        ULONGLONG BitmapBytes = (((ClusterCount + 7) / 8) + 7) & ~7ULL;
        ULONGLONG BitmapBits = BitmapBytes * 8;
        ULONGLONG c;
        PBYTE     Buf = RtlAllocateHeap(RtlGetProcessHeap(), 0, Size);
        if (!Buf) { Status = STATUS_INSUFFICIENT_RESOURCES; goto done; }
        RtlZeroMemory(Buf, Size);
        for (c = 0; c < LAYOUT.FirstFreeLcn; c++)
            Buf[c / 8] |= (1 << (c % 8));
        for (c = ClusterCount; c < BitmapBits; c++)
            Buf[c / 8] |= (1 << (c % 8));
        Status = WriteAt(LAYOUT.BitmapLcn, Buf, Size);
        FREE(Buf);
        if (!NT_SUCCESS(Status)) { DPRINT1("ERROR: write $Bitmap (Status %lx)\n", Status); goto done; }
    }

    /* Root $I30 INDX block: the system files in collation (uppercased-name)
     * order. "$" names (0x24) sort before root's own "." (0x2E), which is last. */
    {
        static const ULONG RootOrder[] =
        {
            MREC_ATTRDEF, MREC_BADCLUS, MREC_BITMAP, MREC_BOOT, MREC_EXTEND,
            MREC_LOGFILE, MREC_MFT, MREC_MFTMIRR, MREC_SECURE, MREC_UPCASE,
            MREC_VOLUME, MREC_ROOT
        };
        PBYTE Indx = RtlAllocateHeap(RtlGetProcessHeap(), 0, INDEX_RECORD_SIZE);
        if (!Indx) { Status = STATUS_INSUFFICIENT_RESOURCES; goto done; }
        MkRootIndxBlock(Indx, Mft, RootOrder, ARR_SIZE(RootOrder));
        Status = WriteRegion(LAYOUT.RootIdxLcn, LAYOUT.RootIdxClusters, Indx, INDEX_RECORD_SIZE);
        FREE(Indx);
        if (!NT_SUCCESS(Status)) { DPRINT1("ERROR: write root index (Status %lx)\n", Status); goto done; }
    }

    /* Zero the TxF payload streams (CLFS logs are empty on a fresh volume). */
    Status = WriteZerosToClusters(LAYOUT.TopsTLcn, LAYOUT.TopsTClusters);
    if (NT_SUCCESS(Status)) Status = WriteZerosToClusters(LAYOUT.BlfLcn, LAYOUT.BlfClusters);
    if (NT_SUCCESS(Status)) Status = WriteZerosToClusters(LAYOUT.Cont1Lcn, LAYOUT.Cont1Clusters);
    if (NT_SUCCESS(Status)) Status = WriteZerosToClusters(LAYOUT.Cont2Lcn, LAYOUT.Cont2Clusters);
    if (NT_SUCCESS(Status)) Status = WriteZerosToClusters(LAYOUT.DeletedIdxLcn, LAYOUT.DeletedIdxClusters);
    if (!NT_SUCCESS(Status)) { DPRINT1("ERROR: zero TxF payloads (Status %lx)\n", Status); goto done; }

done:
    FREE(Mft);
    return Status;
}
