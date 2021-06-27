/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    DumpSup.c

Abstract:

    This module implements a collection of data structure dump routines
    for debugging the Fat file system


--*/

#include "fatprocs.h"

#ifdef FASTFATDBG

VOID FatDump(IN PVOID Ptr);

VOID FatDumpDataHeader();
VOID FatDumpVcb(IN PVCB Ptr);
VOID FatDumpFcb(IN PFCB Ptr);
VOID FatDumpCcb(IN PCCB Ptr);

ULONG FatDumpCurrentColumn;

#define DumpNewLine() {       \
    DbgPrint("\n");            \
    FatDumpCurrentColumn = 1; \
}

#define DumpLabel(Label,Width) {                                          \
    size_t i, LastPeriod=0;                                                \
    CHAR _Str[20];                                                        \
    for(i=0;i<2;i++) { _Str[i] = UCHAR_SP;}                               \
    for(i=0;i<strlen(#Label);i++) {if (#Label[i] == '.') LastPeriod = i;} \
    strncpy(&_Str[2],&#Label[LastPeriod],Width);                          \
    for(i=strlen(_Str);i<Width;i++) {_Str[i] = UCHAR_SP;}                 \
    _Str[Width] = '\0';                                                   \
    DbgPrint("%s", _Str);                                                  \
}

#define DumpField(Field) {                                         \
    if ((FatDumpCurrentColumn + 18 + 9 + 9) > 80) {DumpNewLine();} \
    FatDumpCurrentColumn += 18 + 9 + 9;                            \
    DumpLabel(Field,18);                                           \
    DbgPrint(":%p", Ptr->Field);                                  \
    DbgPrint("         ");                                          \
}

#define DumpListEntry(Links) {                                     \
    if ((FatDumpCurrentColumn + 18 + 9 + 9) > 80) {DumpNewLine();} \
    FatDumpCurrentColumn += 18 + 9 + 9;                            \
    DumpLabel(Links,18);                                           \
    DbgPrint(":%p", Ptr->Links.Flink);                            \
    DbgPrint(":%p", Ptr->Links.Blink);                            \
}

#define DumpName(Field,Width) {                                    \
    ULONG i;                                                       \
    CHAR _String[256];                                             \
    if ((FatDumpCurrentColumn + 18 + Width) > 80) {DumpNewLine();} \
    FatDumpCurrentColumn += 18 + Width;                            \
    DumpLabel(Field,18);                                           \
    for(i=0;i<Width;i++) {_String[i] = (CHAR)Ptr->Field[i];}             \
    _String[Width] = '\0';                                         \
    DbgPrint("%s", _String);                                        \
}

#define TestForNull(Name) {                                 \
    if (Ptr == NULL) {                                      \
        DbgPrint("%s - Cannot dump a NULL pointer\n", Name); \
        return;                                             \
    }                                                       \
}


VOID
FatDump (
    IN PVOID Ptr
    )

/*++

Routine Description:

    This routine determines the type of internal record reference by ptr and
    calls the appropriate dump routine.

Arguments:

    Ptr - Supplies the pointer to the record to be dumped

Return Value:

    None

--*/

{
    TestForNull("FatDump");

    switch (NodeType(Ptr)) {

    case FAT_NTC_DATA_HEADER:

        FatDumpDataHeader();
        break;

    case FAT_NTC_VCB:

        FatDumpVcb(Ptr);
        break;

    case FAT_NTC_FCB:
    case FAT_NTC_DCB:
    case FAT_NTC_ROOT_DCB:

        FatDumpFcb(Ptr);
        break;

    case FAT_NTC_CCB:

        FatDumpCcb(Ptr);
        break;

    default :

        DbgPrint("FatDump - Unknown Node type code %p\n", *((PNODE_TYPE_CODE)(Ptr)));
        break;
    }

    return;
}


VOID
FatDumpDataHeader (
    )

/*++

Routine Description:

    Dump the top data structures and all Device structures

Arguments:

    None

Return Value:

    None

--*/

{
    PFAT_DATA Ptr;
    PLIST_ENTRY Links;

    Ptr = &FatData;

    TestForNull("FatDumpDataHeader");

    DumpNewLine();
    DbgPrint("FatData@ %lx", (Ptr));
    DumpNewLine();

    DumpField           (NodeTypeCode);
    DumpField           (NodeByteSize);
    DumpListEntry       (VcbQueue);
    DumpField           (DriverObject);
    DumpField           (OurProcess);
    DumpNewLine();

    for (Links = Ptr->VcbQueue.Flink;
         Links != &Ptr->VcbQueue;
         Links = Links->Flink) {

        FatDumpVcb(CONTAINING_RECORD(Links, VCB, VcbLinks));
    }

    return;
}


VOID
FatDumpVcb (
    IN PVCB Ptr
    )

/*++

Routine Description:

    Dump an Device structure, its Fcb queue amd direct access queue.

Arguments:

    Ptr - Supplies the Device record to be dumped

Return Value:

    None

--*/

{
    TestForNull("FatDumpVcb");

    DumpNewLine();
    DbgPrint("Vcb@ %lx", (Ptr));
    DumpNewLine();

    DumpField           (VolumeFileHeader.NodeTypeCode);
    DumpField           (VolumeFileHeader.NodeByteSize);
    DumpListEntry       (VcbLinks);
    DumpField           (TargetDeviceObject);
    DumpField           (Vpb);
    DumpField           (VcbState);
    DumpField           (VcbCondition);
    DumpField           (RootDcb);
    DumpField           (DirectAccessOpenCount);
    DumpField           (OpenFileCount);
    DumpField           (ReadOnlyCount);
    DumpField           (AllocationSupport);
    DumpField           (AllocationSupport.RootDirectoryLbo);
    DumpField           (AllocationSupport.RootDirectorySize);
    DumpField           (AllocationSupport.FileAreaLbo);
    DumpField           (AllocationSupport.NumberOfClusters);
    DumpField           (AllocationSupport.NumberOfFreeClusters);
    DumpField           (AllocationSupport.FatIndexBitSize);
    DumpField           (AllocationSupport.LogOfBytesPerSector);
    DumpField           (AllocationSupport.LogOfBytesPerCluster);
    DumpField           (DirtyFatMcb);
    DumpField           (FreeClusterBitMap);
    DumpField           (VirtualVolumeFile);
    DumpField           (SectionObjectPointers.DataSectionObject);
    DumpField           (SectionObjectPointers.SharedCacheMap);
    DumpField           (SectionObjectPointers.ImageSectionObject);
    DumpField           (ClusterHint);
    DumpNewLine();

    FatDumpFcb(Ptr->RootDcb);

    return;
}


VOID
FatDumpFcb (
    IN PFCB Ptr
    )

/*++

Routine Description:

    Dump an Fcb structure, its various queues

Arguments:

    Ptr - Supplies the Fcb record to be dumped

Return Value:

    None

--*/

{
    PLIST_ENTRY Links;

    TestForNull("FatDumpFcb");

    DumpNewLine();
    if      (NodeType(&Ptr->Header) == FAT_NTC_FCB)      {DbgPrint("Fcb@ %lx", (Ptr));}
    else if (NodeType(&Ptr->Header) == FAT_NTC_DCB)      {DbgPrint("Dcb@ %lx", (Ptr));}
    else if (NodeType(&Ptr->Header) == FAT_NTC_ROOT_DCB) {DbgPrint("RootDcb@ %lx", (Ptr));}
    else {DbgPrint("NonFcb NodeType @ %lx", (Ptr));}
    DumpNewLine();

    DumpField           (Header.NodeTypeCode);
    DumpField           (Header.NodeByteSize);
    DumpListEntry       (ParentDcbLinks);
    DumpField           (ParentDcb);
    DumpField           (Vcb);
    DumpField           (FcbState);
    DumpField           (FcbCondition);
    DumpField           (UncleanCount);
    DumpField           (OpenCount);
    DumpField           (DirentOffsetWithinDirectory);
    DumpField           (DirentFatFlags);
    DumpField           (FullFileName.Length);
    DumpField           (FullFileName.Buffer);
    DumpName            (FullFileName.Buffer, 32);
    DumpField           (ShortName.Name.Oem.Length);
    DumpField           (ShortName.Name.Oem.Buffer);
    DumpField           (NonPaged);
    DumpField           (Header.AllocationSize.LowPart);
    DumpField           (NonPaged->SectionObjectPointers.DataSectionObject);
    DumpField           (NonPaged->SectionObjectPointers.SharedCacheMap);
    DumpField           (NonPaged->SectionObjectPointers.ImageSectionObject);

    if ((Ptr->Header.NodeTypeCode == FAT_NTC_DCB) ||
        (Ptr->Header.NodeTypeCode == FAT_NTC_ROOT_DCB)) {

        DumpListEntry   (Specific.Dcb.ParentDcbQueue);
        DumpField       (Specific.Dcb.DirectoryFileOpenCount);
        DumpField       (Specific.Dcb.DirectoryFile);

    } else if (Ptr->Header.NodeTypeCode == FAT_NTC_FCB) {

        DumpField       (Header.FileSize.LowPart);

    } else {

        DumpNewLine();
        DbgPrint("Illegal Node type code");

    }
    DumpNewLine();

    if ((Ptr->Header.NodeTypeCode == FAT_NTC_DCB) ||
        (Ptr->Header.NodeTypeCode == FAT_NTC_ROOT_DCB)) {

        for (Links = Ptr->Specific.Dcb.ParentDcbQueue.Flink;
             Links != &Ptr->Specific.Dcb.ParentDcbQueue;
             Links = Links->Flink) {

            FatDumpFcb(CONTAINING_RECORD(Links, FCB, ParentDcbLinks));
        }
    }

    return;
}


VOID
FatDumpCcb (
    IN PCCB Ptr
    )

/*++

Routine Description:

    Dump a Ccb structure

Arguments:

    Ptr - Supplies the Ccb record to be dumped

Return Value:

    None

--*/

{
    TestForNull("FatDumpCcb");

    DumpNewLine();
    DbgPrint("Ccb@ %lx", (Ptr));
    DumpNewLine();

    DumpField           (NodeTypeCode);
    DumpField           (NodeByteSize);
    DumpField           (UnicodeQueryTemplate.Length);
    DumpName            (UnicodeQueryTemplate.Buffer, 32);
    DumpField           (OffsetToStartSearchFrom);
    DumpNewLine();

    return;
}

#endif // FASTFATDBG

