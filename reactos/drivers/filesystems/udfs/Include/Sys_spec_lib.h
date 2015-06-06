////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
////////////////////////////////////////////////////////////////////
/*************************************************************************
*
* File: sys_spec_lib.h
*
* Module: UDF File System Driver (Kernel mode execution only)
*
* Description:
*   The main include file for the UDF file system driver.
*
* Author: Alter
*
*************************************************************************/

#ifndef _UDF_SYS_SPEC_LIB__H_
#define _UDF_SYS_SPEC_LIB__H_

typedef struct _UDF_PH_CALL_CONTEXT {
    KEVENT          event;
    IO_STATUS_BLOCK IosbToUse;
} UDF_PH_CALL_CONTEXT, *PUDF_PH_CALL_CONTEXT;

#ifdef _BROWSE_UDF_

// convert UDF timestamp to NT time
LONGLONG UDFTimeToNT(IN PUDF_TIME_STAMP UdfTime);
// translate UDF file attributes to NT ones
ULONG    UDFAttributesToNT(IN PDIR_INDEX_ITEM FileDirNdx,
                           IN tag* FileEntry);
// translate NT file attributes to UDF ones
VOID     UDFAttributesToUDF(IN PDIR_INDEX_ITEM FileDirNdx,
                            IN tag* FileEntry,
                            IN ULONG NTAttr);
// translate all file information to NT
NTSTATUS UDFFileDirInfoToNT(IN PVCB Vcb,
                            IN PDIR_INDEX_ITEM FileDirNdx,
                            OUT PFILE_BOTH_DIR_INFORMATION NTFileInfo);
// convert NT time to UDF timestamp
VOID     UDFTimeToUDF(IN LONGLONG NtTime,
                      OUT PUDF_TIME_STAMP UdfTime);
// change xxxTime field(s) in (Ext)FileEntry
VOID     UDFSetFileXTime(IN PUDF_FILE_INFO FileInfo,
                         IN LONGLONG* CrtTime,
                         IN LONGLONG* AccTime,
                         IN LONGLONG* AttrTime,
                         IN LONGLONG* ChgTime);
// get xxxTime field(s) in (Ext)FileEntry
VOID     UDFGetFileXTime(IN PUDF_FILE_INFO FileInfo,
                         OUT LONGLONG* CrtTime,
                         OUT LONGLONG* AccTime,
                         OUT LONGLONG* AttrTime,
                         OUT LONGLONG* ChgTime);
//
#define UDFUpdateAccessTime(Vcb, FileInfo)             \
if(Vcb->CompatFlags & UDF_VCB_IC_UPDATE_ACCESS_TIME) {       \
    LONGLONG NtTime;                                   \
    KeQuerySystemTime((PLARGE_INTEGER)&NtTime);            \
    UDFSetFileXTime(FileInfo, NULL, &NtTime, NULL, NULL);  \
}
//
#define UDFUpdateModifyTime(Vcb, FileInfo)             \
if(Vcb->CompatFlags & UDF_VCB_IC_UPDATE_MODIFY_TIME) {       \
    LONGLONG NtTime;                                   \
    ULONG Attr;                                        \
    PDIR_INDEX_ITEM DirNdx;                            \
    KeQuerySystemTime((PLARGE_INTEGER)&NtTime);               \
    UDFSetFileXTime(FileInfo, NULL, &NtTime, NULL, &NtTime);  \
    DirNdx = UDFDirIndex(UDFGetDirIndexByFileInfo(FileInfo), (FileInfo)->Index); \
    Attr = UDFAttributesToNT(DirNdx, (FileInfo)->Dloc->FileEntry); \
    if(!(Attr & FILE_ATTRIBUTE_ARCHIVE))                            \
        UDFAttributesToUDF(DirNdx, (FileInfo)->Dloc->FileEntry, Attr); \
}
//
#define UDFUpdateAttrTime(Vcb, FileInfo)               \
if(Vcb->CompatFlags & UDF_VCB_IC_UPDATE_ATTR_TIME) {         \
    LONGLONG NtTime;                                   \
    KeQuerySystemTime((PLARGE_INTEGER)&NtTime);               \
    UDFSetFileXTime(FileInfo, NULL, &NtTime, &NtTime, NULL);  \
}
//
#define UDFUpdateCreateTime(Vcb, FileInfo)             \
{                                                      \
    LONGLONG NtTime;                                   \
    KeQuerySystemTime((PLARGE_INTEGER)&NtTime);               \
    UDFSetFileXTime(FileInfo, &NtTime, &NtTime, &NtTime, &NtTime);  \
}

void 
__fastcall
UDFDOSNameOsNative(
    IN OUT PUNICODE_STRING DosName,
    IN PUNICODE_STRING UdfName,
    IN BOOLEAN KeepIntact
    );

VOID     UDFNormalizeFileName(IN PUNICODE_STRING FName,
                              IN USHORT valueCRC);

NTSTATUS MyAppendUnicodeStringToString_(IN PUNICODE_STRING Str1,
                                        IN PUNICODE_STRING Str2
#ifdef UDF_TRACK_UNICODE_STR
                                       ,IN PCHAR Tag
#endif 
                                       );

NTSTATUS MyAppendUnicodeToString_(IN PUNICODE_STRING Str1,
                                  IN PWSTR Str2
#ifdef UDF_TRACK_UNICODE_STR
                                 ,IN PCHAR Tag
#endif 
                                 );

#ifdef UDF_TRACK_UNICODE_STR
  #define MyAppendUnicodeStringToString(s1,s2)         MyAppendUnicodeStringToString_(s1,s2,"AppUStr")
  #define MyAppendUnicodeStringToStringTag(s1,s2,tag)  MyAppendUnicodeStringToString_(s1,s2,tag)
  #define MyAppendUnicodeToString(s1,s2)               MyAppendUnicodeToString_(s1,s2,"AppStr")
  #define MyAppendUnicodeToStringTag(s1,s2,tag)        MyAppendUnicodeToString_(s1,s2,tag)
#else
  #define MyAppendUnicodeStringToString(s1,s2)  MyAppendUnicodeStringToString_(s1,s2)
  #define MyAppendUnicodeStringToStringTag(s1,s2,tag)  MyAppendUnicodeStringToString_(s1,s2)
  #define MyAppendUnicodeToString(s1,s2)               MyAppendUnicodeToString_(s1,s2)
  #define MyAppendUnicodeToStringTag(s1,s2,tag)        MyAppendUnicodeToString_(s1,s2)
#endif

NTSTATUS MyInitUnicodeString(IN PUNICODE_STRING Str1,
                             IN PCWSTR Str2);

NTSTATUS MyCloneUnicodeString(IN PUNICODE_STRING Str1,
                              IN PUNICODE_STRING Str2);

/*ULONG    MyCompareUnicodeString(PUNICODE_STRING s1,
                                PUNICODE_STRING s2,
                                BOOLEAN UpCase);*/

/*
#define UDFAllocFileInfo() \
    ExAllocateFromZone(&(UDFGlobalData.FileInfoZoneHeader))
*/

#define UDFIsDataCached(Vcb,Lba,BCount) \
    ( WCacheIsInitialized__(&((Vcb)->FastCache)) &&        \
     (KeGetCurrentIrql() < DISPATCH_LEVEL) && \
      WCacheIsCached__(&((Vcb)->FastCache),Lba, BCount) )

BOOLEAN  UDFIsDirInfoCached(IN PVCB Vcb,
                            IN PUDF_FILE_INFO DirInfo);

#define UDFGetNTFileId(Vcb, fi, fn) (((fi)->Dloc->FELoc.Mapping[0].extLocation - UDFPartStart(Vcb, -2)) + \
                                     ((ULONG)(UDFUnicodeCksum((fn)->Buffer, (fn)->Length/sizeof(WCHAR))) << 16) + \
                                     ((LONGLONG)Vcb<<32) )

#define UnicodeIsPrint(a) RtlIsValidOemCharacter(&(a))

#define UDFSysGetAllocSize(Vcb, Size) ((Size + Vcb->LBlockSize - 1) & ~((LONGLONG)(Vcb->LBlockSize - 1)))

NTSTATUS UDFDoesOSAllowFileToBeTargetForRename__(IN PUDF_FILE_INFO FileInfo);
#define UDFDoesOSAllowFileToBeTargetForHLink__  UDFDoesOSAllowFileToBeTargetForRename__
NTSTATUS UDFDoesOSAllowFileToBeUnlinked__(IN PUDF_FILE_INFO FileInfo);
#define UDFDoesOSAllowFileToBeMoved__  UDFDoesOSAllowFileToBeUnlinked__
NTSTATUS UDFDoesOSAllowFilePretendDeleted__(IN PUDF_FILE_INFO FileInfo);
BOOLEAN UDFRemoveOSReferences__(IN PUDF_FILE_INFO FileInfo);

#define UDFIsFSDevObj(DeviceObject) \
    (DeviceObject->DeviceExtension && \
      ( (((PVCB)(DeviceObject->DeviceExtension))->NodeIdentifier.NodeType == \
              UDF_NODE_TYPE_UDFFS_DEVOBJ) || \
        (((PVCB)(DeviceObject->DeviceExtension))->NodeIdentifier.NodeType == \
              UDF_NODE_TYPE_UDFFS_DRVOBJ) \
      ) \
    )
/*
extern ULONG  MajorVersion;
extern ULONG  MinorVersion;
extern ULONG  BuildNumber;

#define WinVer_Is351  (MajorVersion==0x03 && MinorVersion==51)
#define WinVer_IsNT   (MajorVersion==0x04)
#define WinVer_Is2k   (MajorVersion==0x05 && MinorVersion==0x00)
#define WinVer_IsXP   (MajorVersion==0x05 && MinorVersion==0x01)
#define WinVer_IsdNET (MajorVersion==0x05 && MinorVersion==0x02)
*/
#endif //_BROWSE_UDF_

#endif  // _UDF_SYS_SPEC_LIB__H_
