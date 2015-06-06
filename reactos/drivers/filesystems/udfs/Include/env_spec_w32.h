////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
////////////////////////////////////////////////////////////////////
/*************************************************************************
*
* File: env_spec_w32.h
*
* Module: User-mode applications (User mode execution only)
*
* Description:
*   
*
* Author: Alter
*
*************************************************************************/

#ifndef __ENV_SPEC_W32__H_
#define __ENV_SPEC_W32__H_

#ifdef NT_NATIVE_MODE
//#include "ntddk.h" // include this for its native functions and defn's
#include "nt_native.h"
#else //NT_NATIVE_MODE
#include <windows.h>
#endif //NT_NATIVE_MODE
#include "platform.h"
//#ifndef WITHOUT_FORMATTER
#include "udferr_usr.h"
//#endif WITHOUT_FORMATTER

#ifndef NT_NATIVE_MODE
#ifdef ASSERT
  #undef ASSERT
  #define ASSERT(a)    if(!(a)) {__asm int 3;}
#endif
#endif //NT_NATIVE_MODE

#ifndef MAXIMUM_FILENAME_LENGTH
#define MAXIMUM_FILENAME_LENGTH MAX_PATH
#endif //MAXIMUM_FILENAME_LENGTH

#ifndef PAGE_SHIFT
#define PAGE_SHIFT 12
#endif //PAGE_SHIFT

#ifndef PAGE_SIZE
#define PAGE_SIZE (ULONG)0x1000
#endif //PAGE_SIZE

#ifndef PHYSICAL_ADDRESS
#define PHYSICAL_ADDRESS  LARGE_INTEGER
#endif //PHYSICAL_ADDRESS

#define OS_SUCCESS(a)     NT_SUCCESS(a)
#define OSSTATUS          NTSTATUS

#if defined UDF_DBG || defined DEBUG
#define DBG
#ifndef CDRW_W32
#define UDF_DBG
#endif //CDRW_W32
#endif 

#define ERESEOURCE        ULONG
#define PERESEOURCE       PULONG

#define KEVENT            ULONG
#define PKEVENT           PULONG

typedef ULONG KSPIN_LOCK;  // winnt ntndis
typedef KSPIN_LOCK *PKSPIN_LOCK;

#ifndef NT_NATIVE_MODE
// Status ot
#define NTSTATUS LONG

#define NT_SUCCESS(x) ( (NTSTATUS)(x)>=0 )

#define PsGetCurrentThread()  GetCurrentThreadId()

#define PsGetVersion(a,b,c,d) { \
    OSVERSIONINFO OsVer;        \
    OsVer.dwOSVersionInfoSize = sizeof(OsVer); \
    GetVersionEx(&OsVer); \
    if(a) (*(a)) = OsVer.dwMajorVersion; \
    if(b) (*(b)) = OsVer.dwMinorVersion; \
    if(c) (*(c)) = OsVer.dwBuildNumber;  \
    if(d) (d)->Buffer = L"";  \
    if(d) (d)->Length = 0;  \
    if(d) (d)->MaximumLength = 0;  \
}

extern "C"
VOID
PrintDbgConsole(
    PCHAR DebugMessage,
    ...
    );

#else //NT_NATIVE_MODE
#define HINSTANCE HANDLE
#endif //NT_NATIVE_MODE

typedef
int (*PSKIN_INIT) (
    HINSTANCE hInstance,      // handle to current instance
    HINSTANCE hPrevInstance,  // handle to previous instance
    int nCmdShow              // show state
    );

typedef
int (*PSKIN_PRINTF) (
    const char* Message,
    ...
    );

typedef
PWCHAR (__stdcall *PSKIN_GETCMDLINE) (
    VOID
    );

typedef
ULONG (__stdcall *PSKIN_MSG) (
    ULONG MsgId,
    ULONG MsgSubId,
    PVOID DataIn,
    ULONG DataInLen,
    PVOID DataOut,
    ULONG DataInOut
    );

typedef struct _SKIN_API {
    PSKIN_INIT        Init;
    PSKIN_PRINTF      Printf;
    PSKIN_GETCMDLINE  GetCommandLine;
    PSKIN_MSG         Msg;
} SKIN_API, *PSKIN_API;

#ifdef USE_SKIN_MODEL

extern "C" PSKIN_API SkinAPI;
extern PSKIN_API SkinLoad(
    PWCHAR path,
    HINSTANCE hInstance,      // handle to current instance
    HINSTANCE hPrevInstance,  // handle to previous instance
    int nCmdShow              // show state
    );

#define SkinPrintf                       SkinAPI->Printf
#define SkinGetCmdLine                   SkinAPI->GetCommandLine
#define SkinNotify(op, state, ctx, sz)   SkinAPI->Msg(op, state, ctx, sz, NULL, 0)
#define SkinAsk(op, state, ctx, def)     SkinAPI->Msg(op, state, ctx, sizeof(ctx), NULL, 0)

#else

#define SkinLoad(path)    {;}

#if defined(CDRW_W32) || defined(LIBUDFFMT) || defined(LIBUDF)
  #define SkinPrintf(x)                    {;}
/*VOID
inline
SkinPrintf(
    PCHAR Message,
    ...
    )
{
    //do nothing
    return;
}*/
#else // defined(CDRW_W32) || defined(LIBUDFFMT) || defined(LIBUDF)
  #define SkinPrintf                       printf
#endif

#define SkinGetCmdLine                   GetCommandLineW
#define SkinNotify(op, state, ctx)       {;}
#define SkinAsk(op, state, ctx, def)     (def)

#endif // defined(CDRW_W32) || defined(LIBUDFFMT) || defined(LIBUDF)

#if defined(CDRW_W32) || defined(LIBUDFFMT) || defined(LIBUDF)

  #if defined(PRINT_TO_DBG_LOG) || defined(PRINT_ALWAYS)
    #define UserPrint(x) PrintDbgConsole x
  #else
    #define UserPrint(x) {;}
  #endif // PRINT_TO_DBG_LOG

#else // defined(CDRW_W32) || defined(LIBUDFFMT) || defined(LIBUDF)

  #if defined(PRINT_TO_DBG_LOG) || defined(PRINT_ALWAYS)

    #define UserPrint(x) \
    {                    \
     SkinPrintf x ;          \
     PrintDbgConsole x ; \
    }

  #else // PRINT_TO_DBG_LOG

    #define UserPrint(x) \
    {                    \
     SkinPrintf x ;          \
    }

  #endif // PRINT_TO_DBG_LOG

#endif // defined(CDRW_W32) || defined(LIBUDFFMT) || defined(LIBUDF)

#if defined(DBG) || defined(PRINT_ALWAYS)

  #define DbgPrint SkinPrintf

  #ifdef KdPrint
    #undef KdPrint
  #endif

  #if defined(CDRW_W32) || defined(LIBUDFFMT) || defined(LIBUDF)
    #ifdef PRINT_TO_DBG_LOG
      #define KdPrint(x) PrintDbgConsole x;
    #else
      #define KdPrint(x) {;}
    #endif
  #else

    #if defined(PRINT_TO_DBG_LOG) || defined(PRINT_ALWAYS)
      #define KdPrint(x) \
      {                    \
       SkinPrintf x ;          \
       PrintDbgConsole x ; \
      }
    #else
      #define KdPrint(x) \
      {                    \
       SkinPrintf x ;          \
      }
    #endif
  #endif

  #ifdef USE_MM_PRINT
    #define MmPrint(_x_) DbgPrint _x_
  #else
    #define MmPrint(_x_) {NOTHING;}
  #endif //USE_MM_PRINT

  #ifdef USE_TIME_PRINT
    extern ULONG UdfTimeStamp;
    #define TmPrint(_x_) {UdfTimeStamp++;KdPrint(("TM:%d: ",UdfTimeStamp));KdPrint(_x_);}
  #else
    #define TmPrint KdPrint
  #endif //USE_MM_PRINT

  #ifdef USE_PERF_PRINT
    #define PerfPrint(_x_) DbgPrint _x_
  #else
    #define PerfPrint(_x_) {NOTHING;}
  #endif //USE_MM_PRINT

#if defined(CDRW_W32) || defined(LIBUDFFMT) || defined(LIBUDF)
  #ifdef USE_AD_PRINT
    #undef USE_AD_PRINT
  #endif
  #ifdef USE_TH_PRINT
    #undef USE_TH_PRINT
  #endif
#endif

  #ifdef USE_AD_PRINT
    #define AdPrint(_x_) {DbgPrint("Thrd:%x:",PsGetCurrentThread());DbgPrint _x_;}
  #else 
    #define AdPrint(_x_) {NOTHING;}
  #endif

  #ifdef USE_TH_PRINT
    #define ThPrint(_x_) {DbgPrint("Thrd:%x:",PsGetCurrentThread());DbgPrint _x_;}
  #else
    #define ThPrint(_x_) {NOTHING;}
  #endif

  #ifdef USE_DUMP_EXT
    #define ExtPrint(_x_) DbgPrint _x_
  #else
    #define ExtPrint(_x_) {NOTHING;}
  #endif //USE_MM_PRINT

#else
  #define KdPrint(x)     {NOTHING;}
  #define MmPrint(_x_)   {NOTHING;}
  #define TmPrint(_x_)   {NOTHING;}
  #define PerfPrint(_x_) {NOTHING;}
  #define AdPrint(_x_)   {NOTHING;}
  #define ThPrint(_x_)   {NOTHING;}
  #define ExtPrint(_x_)  {NOTHING;}
#endif

#define DbgTouch(a)

#ifndef NT_NATIVE_MODE
#include "assert.h"

#define ASSERT(_x_)   assert(_x_)
#define UDFTouch(a)

#endif //NT_NATIVE_MODE

#define NonPagedPool            0
#define PagedPool               1
//#define NonPagedPoolMustSucceed 2
#define NonPagedPoolCacheAligned 4

#define KdDump(a,b)                         \
if((a)!=NULL) {                             \
    ULONG i;                                \
    for(i=0; i<(b); i++) {                  \
        ULONG c;                            \
        c = (ULONG)(*(((PUCHAR)(a))+i));    \
        KdPrint(("%2.2x ",c));              \
        if ((i & 0x0f) == 0x0f) KdPrint(("\n"));   \
    }                                       \
    KdPrint(("\n"));                        \
}

//mem ot
//#define ExAllocatePool(hernya,size) MyAllocatePool(size)
//#define ExFreePool(size) MyFreePool((PCHAR)(size))
//#define SystemAllocatePool(hernya,size) GlobalAlloc(GMEM_DISCARDABLE, size);
//#define SystemFreePool(addr) GlobalFree((PVOID)(addr))
#define DbgMoveMemory     RtlMoveMemory
#define DbgCompareMemory  RtlCompareMemory
#define DbgCopyMemory     RtlCopyMemory
#define DbgAllocatePool   ExAllocatePool
#define DbgAllocatePoolWithTag(a,b,c)   ExAllocatePool(a,b)
#define DbgFreePool       ExFreePool

#ifdef NT_NATIVE_MODE
/*
#define GlobalAlloc(foo, size)  MyGlobalAlloc( size );
#define GlobalFree(ptr)         MyGlobalFree( ptr );

extern "C"
PVOID MyGlobalAlloc(ULONG Size);

extern "C"
VOID  MyGlobalFree(PVOID Addr);
*/
#endif

#if !defined(KERNEL_MODE_MM_BEHAVIOR) && !defined(USE_THREAD_HEAPS)
#define ExAllocatePoolWithTag(hernya,size,tag) GlobalAlloc(GMEM_DISCARDABLE, (size))
#define ExAllocatePool(hernya,size) GlobalAlloc(GMEM_DISCARDABLE, (size))
#define ExFreePool(addr) GlobalFree((PVOID)(addr))
#endif

#if defined(KERNEL_MODE_MM_BEHAVIOR) || defined(USE_THREAD_HEAPS)
#define ExAllocatePoolWithTag(MemoryType,size,tag) ExAllocatePool((MemoryType), (size))

extern "C"
PVOID ExAllocatePool(ULONG MemoryType, ULONG Size);

extern "C"
VOID  ExFreePool(PVOID Addr);
#endif //KERNEL_MODE_MM_BEHAVIOR || USE_THREAD_HEAPS

#ifndef NT_NATIVE_MODE

//string ot
typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;

typedef struct _ANSI_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PSTR   Buffer;
} ANSI_STRING;
typedef ANSI_STRING *PANSI_STRING;

#endif //NT_NATIVE_MODE

#define PtrOffset(BASE,OFFSET) ((ULONG)((ULONG)(OFFSET) - (ULONG)(BASE)))

// Device object ot

#ifndef DO_UNLOAD_PENDING
// Define Device Object (DO) flags
//

#define DO_UNLOAD_PENDING               0x00000001
#define DO_VERIFY_VOLUME                0x00000002
#define DO_BUFFERED_IO                  0x00000004
#define DO_EXCLUSIVE                    0x00000008
#define DO_DIRECT_IO                    0x00000010
#define DO_MAP_IO_BUFFER                0x00000020
#define DO_DEVICE_HAS_NAME              0x00000040
#define DO_DEVICE_INITIALIZING          0x00000080
#define DO_SYSTEM_BOOT_PARTITION        0x00000100
#define DO_LONG_TERM_REQUESTS           0x00000200
#define DO_NEVER_LAST_DEVICE            0x00000400
#define DO_SHUTDOWN_REGISTERED          0x00000800

#endif //DO_UNLOAD_PENDING

#ifdef NT_NATIVE_MODE
#define _DEVICE_OBJECT _MY_DEVICE_OBJECT
#define DEVICE_OBJECT  MY_DEVICE_OBJECT
#define PDEVICE_OBJECT PMY_DEVICE_OBJECT
#endif //NT_NATIVE_MODE

typedef struct _DEVICE_OBJECT {
    
#ifndef LIBUDF
    
    HANDLE h;
    PVOID  DeviceExtension;
    ULONG  Flags;
    ULONG  AlignmentRequirement;
    UCHAR  StackSize;

#endif // LIBUDF


#ifdef LIBUDFFMT

    struct _UDF_FMT_PARAMETERS* cbio;
    PVOID  lpContext;

#else
#ifdef LIBUDF
    PVOID  lpContext;
#endif // LIBUDF
#endif // LIBUDFFMT

} DEVICE_OBJECT, *PDEVICE_OBJECT;

#ifndef CDRW_W32
/*
typedef ULONG DEVICE_OBJECT;
typedef ULONG PDEVICE_OBJECT;
*/
#define INVALID_PACKET 0x01

typedef struct _PACKET {
    // Node Identifier
//  UDFIdentifier               NodeIdentifier; 
    // Pointer to the buffer (in non-paged pool)
    PCHAR           buffer;
    // Offset, from which this data was read
    LARGE_INTEGER   offset;
    // Flags
    UCHAR           flags;
} PACKET, *PPACKET;

#define UDFInitPacket(x) STATUS_SUCCESS
#endif //CDRW_W32

#define try_return(S)   { S; goto try_exit; }
#define NOTHING

#define FlagOn(x,f) ((x) & (f))

#define RtlCompareMemory(s1,s2,l)  MyRtlCompareMemory(s1,s2,l)
// Structure ot
extern "C"
ULONG
MyRtlCompareMemory(
    PVOID s1,
    PVOID s2,
    ULONG len
    );
//#define RtlCompareMemory(d1,d2,l)   (ULONG)(memcmp (d1,d2,l))

#define KeSetEvent(pEvt, foo, foo2)          {NOTHING;}
#define KeInitializeEvent(pEvt, foo, foo2)   {NOTHING;}
#define KeWaitForSingleObject(pEvt, foo, a, b, c)     {NOTHING;}
#define DbgWaitForSingleObject(o, to)   KeWaitForSingleObject(o, Executive, KernelMode, FALSE, to);
//#define DbgWaitForSingleObject   KeWaitForSingleObject
#ifdef NT_NATIVE_MODE
#define KeDelayExecutionThread(mode, foo, t)  { NtDelayExecution(false, t); }
#else //NT_NATIVE_MODE
#define KeDelayExecutionThread(mode, foo, t) { Sleep( abs((LONG)(((t)->QuadPart)/10000)) ); }
#endif //NT_NATIVE_MODE

/*#define RtlCompareUnicodeString(s1,s2,cs) \
 (((s1)->Length == (s2)->Length) && \
  (RtlCompareMemory(s1,s2,(s1)->Length))) 
*/
#ifndef CDRW_W32
#ifdef _X86_

// This is an illegal use of INT3
#define UDFBreakPoint() __asm int 3
#else // _X86_

#define UDFBreakPoint() DbgBreakPoint()
#endif // _X86_

#ifdef BRUTE
#define BrutePoint() UDFBreakPoint()
#else
#define BrutePoint() {}
#endif // BRUTE

#ifdef VALIDATE_STRUCTURES
#define ValidateFileInfo(fi)            \
{    /* validate FileInfo */            \
    if((fi)->IntegrityTag) {            \
        KdPrint(("UDF: ERROR! Using deallocated structure !!!\n"));\
        /*BrutePoint();*/                   \
    }                                   \
}
#else
#define ValidateFileInfo(fi)  {}
#endif

#else //CDRW_W32

#ifdef BRUTE
#ifdef _X86_

// This is an illegal use of INT3
#define BrutePoint() __asm int 3
#else // _X86_

#define BrutePoint() DbgBreakPoint()
#endif // _X86_
#else
#define BrutePoint() {}
#endif // BRUTE

#endif //CDRW_W32

#ifndef NT_NATIVE_MODE

extern "C"
ULONG
RtlCompareUnicodeString(
    PUNICODE_STRING s1,
    PUNICODE_STRING s2,
    BOOLEAN UpCase);

extern "C"
NTSTATUS
RtlUpcaseUnicodeString(
    PUNICODE_STRING dst,
    PUNICODE_STRING src,
    BOOLEAN Alloc
    );

extern "C"
NTSTATUS
RtlAppendUnicodeToString(
    IN PUNICODE_STRING Str1,
    IN PWSTR Str2
    );

#endif //NT_NATIVE_MODE

extern "C"
NTSTATUS
MyInitUnicodeString(
    IN PUNICODE_STRING Str1,
    IN PCWSTR Str2
    );

#ifndef NT_NATIVE_MODE
#define KeQuerySystemTime(t)     GetSystemTimeAsFileTime((LPFILETIME)(t));

typedef struct _FILE_BOTH_DIR_INFORMATION {
    ULONG           NextEntryOffset;
    ULONG           FileIndex;
    LARGE_INTEGER   CreationTime;
    LARGE_INTEGER   LastAccessTime;
    LARGE_INTEGER   LastWriteTime;
    LARGE_INTEGER   ChangeTime;
    LARGE_INTEGER   EndOfFile;
    LARGE_INTEGER   AllocationSize;
    ULONG           FileAttributes;
    ULONG           FileNameLength;
    ULONG           EaSize;
    CCHAR           ShortNameLength;
    WCHAR           ShortName[12];
    WCHAR           FileName[1];
} FILE_BOTH_DIR_INFORMATION, *PFILE_BOTH_DIR_INFORMATION;

#endif //NT_NATIVE_MODE

typedef UCHAR KIRQL;
typedef KIRQL *PKIRQL;

typedef ULONG ERESOURCE;
typedef ERESOURCE *PERESOURCE;

#define KeRaiseIrql(irql, oldIrql)                    \
{                                                     \
    *oldIrql = 0;                                     \
}

#define KeLowerIrql(oldIrql)  {;}

#define KeInitializeSpinLock(sl)                    \
{                                                     \
  *(sl) = 0;                                          \
}

#define KeAcquireSpinLock(sl,irql)                    \
{                                                     \
    ULONG isLocked = TRUE;                            \
    while(isLocked) AcquireXLock(*(sl), isLocked, TRUE);\
}

#define ExAcquireResourceExclusiveLite(res, wait)     \
{                                                     \
    ULONG isLocked = TRUE;                            \
    while(isLocked) AcquireXLock(*(res), isLocked, TRUE);\
}

#define ExAcquireResourceSharedLite(res, wait)        \
{                                                     \
    ULONG isLocked = TRUE;                            \
    while(isLocked) AcquireXLock(*(res), isLocked, TRUE);\
}

#define KeReleaseSpinLock(sl,irql)                    \
{                                                     \
    ULONG isLocked;                                   \
    AcquireXLock(*(sl), isLocked, FALSE);             \
}

#define ExGetCurrentResourceThread()  0

#define ExReleaseResourceForThreadLite(res, thrdID)   \
{                                                     \
    ULONG isLocked;                                   \
    AcquireXLock(*(res), isLocked, FALSE);            \
}

NTSTATUS inline ExInitializeResourceLite(PULONG res)
{
   *(res) = 0;
   return STATUS_SUCCESS;
}

#define ExDeleteResourceLite(res)                     \
{                                                     \
   *(res) = 0;                                        \
}

#define ExConvertExclusiveToSharedLite(res) {/* do nothing */}

#ifndef CDRW_W32

#define UDFAcquireResourceExclusive(Resource,CanWait)  \
    ExAcquireResourceExclusiveLite((Resource),(CanWait)) 
#define UDFAcquireResourceShared(Resource,CanWait) \
    ExAcquireResourceSharedLite((Resource),(CanWait)) 
// a convenient macro (must be invoked in the context of the thread that acquired the resource)
#define UDFReleaseResource(Resource)    \
    ExReleaseResourceForThreadLite((Resource), ExGetCurrentResourceThread())
#define UDFDeleteResource(Resource)    \
    ExDeleteResourceLite((Resource))
#define UDFConvertExclusiveToSharedLite(Resource) \
    ExConvertExclusiveToSharedLite((Resource))
#define UDFInitializeResourceLite(Resource) \
    ExInitializeResourceLite((Resource))
#define UDFAcquireSharedStarveExclusive(Resource,CanWait) \
    ExAcquireSharedStarveExclusive((Resource),(CanWait))
#define UDFAcquireSharedWaitForExclusive(Resource,CanWait) \
    ExAcquireSharedWaitForExclusive((Resource),(CanWait))
//#define UDFDebugAcquireResourceExclusiveLite(a,b,c,d) ExAcquireResourceExclusiveLite(a,b)

#define UDFInterlockedIncrement(addr) \
    ((*addr)++)
#define UDFInterlockedDecrement(addr) \
    ((*addr)--)
int
__inline
UDFInterlockedExchangeAdd(PLONG addr, LONG i) {
    LONG Old = (*addr);
    (*addr) += i;
    return Old;
}

#endif //CDRW_W32

//
// Interrupt Request Level definitions
//

#define PASSIVE_LEVEL 0             // Passive release level
#define LOW_LEVEL 0                 // Lowest interrupt level
#define APC_LEVEL 1                 // APC interrupt level
#define DISPATCH_LEVEL 2            // Dispatcher level

#define PROFILE_LEVEL 27            // timer used for profiling.
#define CLOCK1_LEVEL 28             // Interval clock 1 level - Not used on x86
#define CLOCK2_LEVEL 28             // Interval clock 2 level
#define IPI_LEVEL 29                // Interprocessor interrupt level
#define POWER_LEVEL 30              // Power failure level
#define HIGH_LEVEL 31               // Highest interrupt level
#define SYNCH_LEVEL (IPI_LEVEL-1)   // synchronization level

#define KeGetCurrentIrql()      PASSIVE_LEVEL

#ifndef NT_NATIVE_MODE

typedef struct _TIME_FIELDS {
    USHORT Year;        // range [1601...]
    USHORT Month;       // range [1..12]
    USHORT Day;         // range [1..31]
    USHORT Hour;        // range [0..23]
    USHORT Minute;      // range [0..59]
    USHORT Second;      // range [0..59]
    USHORT Milliseconds;// range [0..999]
    USHORT Weekday;     // range [0..6] == [Sunday..Saturday]
} TIME_FIELDS;
typedef TIME_FIELDS *PTIME_FIELDS;

//#define RtlTimeFieldsToTime(a,b)     TRUE
BOOLEAN
RtlTimeFieldsToTime(
    IN PTIME_FIELDS TimeFields,
    IN PLARGE_INTEGER Time
    );

#define ExSystemTimeToLocalTime(SysTime, LocTime)     FileTimeToLocalFileTime((PFILETIME)(SysTime), (PFILETIME)(LocTime))

//#define RtlTimeToTimeFields(a,b) {}
BOOLEAN
RtlTimeToTimeFields(
    IN PLARGE_INTEGER Time,
    IN PTIME_FIELDS TimeFields
    );

#define ExLocalTimeToSystemTime(LocTime, SysTime)     LocalFileTimeToFileTime((PFILETIME)(LocTime), (PFILETIME)(SysTime))

#endif //NT_NATIVE_MODE

#ifndef CDRW_W32

typedef struct _FSRTL_COMMON_FCB_HEADER {
    SHORT           NodeTypeCode;
    SHORT           NodeByteSize;
    UCHAR           Flags;
    UCHAR           IsFastIoPossible;
#if (_WIN32_WINNT >= 0x0400)
    UCHAR           Flags2;
    UCHAR           Reserved;
#endif // (_WIN32_WINNT >= 0x0400)
    PERESOURCE      Resource;
    PERESOURCE      PagingIoResource;
    LARGE_INTEGER   AllocationSize;
    LARGE_INTEGER   FileSize;
    LARGE_INTEGER   ValidDataLength;
} FSRTL_COMMON_FCB_HEADER, *PFSRTL_COMMON_FCB_HEADER;

typedef struct _SECTION_OBJECT_POINTERS {
    PVOID DataSectionObject;
    PVOID SharedCacheMap;
    PVOID ImageSectionObject;
} SECTION_OBJECT_POINTERS;
typedef SECTION_OBJECT_POINTERS *PSECTION_OBJECT_POINTERS;


extern NTSTATUS UDFPhReadSynchronous(
                   PDEVICE_OBJECT      DeviceObject,
                   PVOID           Buffer,
                   ULONG           Length,
                   LONGLONG        Offset,
                   PULONG          ReadBytes,
                   ULONG           Flags);

extern NTSTATUS UDFPhWriteSynchronous(
                   PDEVICE_OBJECT  DeviceObject,   // the physical device object
                   PVOID           Buffer,
                   ULONG           Length,
                   LONGLONG        Offset,
                   PULONG          WrittenBytes,
                   ULONG           Flags);

#if 0
extern NTSTATUS
UDFPhWriteVerifySynchronous(
    PDEVICE_OBJECT  DeviceObject,   // the physical device object
    PVOID           Buffer,
    ULONG           Length,
    LONGLONG        Offset,
    PULONG          WrittenBytes,
    ULONG           Flags
    );
#endif

#define UDFPhWriteVerifySynchronous   UDFPhWriteSynchronous

extern NTSTATUS UDFPhSendIOCTL(
    IN ULONG IoControlCode,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID InputBuffer ,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer ,
    IN ULONG OutputBufferLength,
    IN BOOLEAN OverrideVerify,
    OUT PVOID Iosb OPTIONAL);

#endif //CDRW_W32

VOID set_image_size(HANDLE h,
//                    ULONG LBA);
                    int64  len);

#ifdef UDF_FORMAT_MEDIA
struct _UDFVolumeControlBlock;
#endif

#ifndef UDF_FORMAT_MEDIA
ULONG write(
            HANDLE h,
            PCHAR buff,
            ULONG len);
#endif

extern "C"
HANDLE
my_open(
#ifndef CDRW_W32
    struct _UDFVolumeControlBlock* Vcb,
#endif //CDRW_W32
    PWCHAR fn);

#ifdef UDF_FORMAT_MEDIA
struct _UDFFmtState;
#endif //UDF_FORMAT_MEDIA

extern
void
my_exit(
#ifdef UDF_FORMAT_MEDIA
    struct _UDFFmtState* fms,
#endif //UDF_FORMAT_MEDIA
    int rc
    );

#ifndef CDRW_W32
uint64 udf_lseek64(HANDLE fd, uint64 offset, int whence);
#endif //CDRW_W32

#ifdef LIBUDFFMT
BOOLEAN
udf_get_sizes(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG* blocks
    );
#endif //LIBUDFFMT

int64
get_file_size(
    HANDLE h
    );

int64
set_file_pointer(
    HANDLE h,
    int64 sz
    );

#ifndef NT_NATIVE_MODE
typedef struct _IO_STATUS_BLOCK {
    ULONG Status;
    ULONG Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;
#endif //NT_NATIVE_MODE


#ifndef UDF_FORMAT_MEDIA
extern ULONG   LockMode;
extern BOOLEAN open_as_device;
extern BOOLEAN opt_invalidate_volume;
#endif //UDF_FORMAT_MEDIA

extern "C"
ULONG
MyLockVolume(
    HANDLE h,
    ULONG* pLockMode // OUT
    );

extern "C"
ULONG
MyUnlockVolume(
    HANDLE h,
    ULONG* pLockMode // IN
    );

#ifndef CDRW_W32
ULONG
UDFGetDevType(PDEVICE_OBJECT DeviceObject);
#endif //CDRW_W32

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE    ((HANDLE)(-1))
#endif

#ifndef ANSI_DOS_STAR

#define ANSI_DOS_STAR   ('<')
#define ANSI_DOS_QM     ('>')
#define ANSI_DOS_DOT    ('"')

#define DOS_STAR        (L'<')
#define DOS_QM          (L'>')
#define DOS_DOT         (L'"')

#endif //ANSI_DOS_STAR

extern "C"
BOOLEAN
ProbeMemory(
    PVOID   MemPtr,
    ULONG   Length,
    BOOLEAN ForWrite
    );

#ifdef NT_NATIVE_MODE
#include "env_spec_nt.h"
#endif //NT_NATIVE_MODE

#ifndef InitializeListHead

//
//  Doubly-linked list manipulation routines.  Implemented as macros
//  but logically these are procedures.
//

//
//  VOID
//  InitializeListHead(
//      PLIST_ENTRY ListHead
//      );
//

#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))

//
//  BOOLEAN
//  IsListEmpty(
//      PLIST_ENTRY ListHead
//      );
//

#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))

//
//  PLIST_ENTRY
//  RemoveHeadList(
//      PLIST_ENTRY ListHead
//      );
//

#define RemoveHeadList(ListHead) \
    (ListHead)->Flink;\
    {RemoveEntryList((ListHead)->Flink)}

//
//  PLIST_ENTRY
//  RemoveTailList(
//      PLIST_ENTRY ListHead
//      );
//

#define RemoveTailList(ListHead) \
    (ListHead)->Blink;\
    {RemoveEntryList((ListHead)->Blink)}

//
//  VOID
//  RemoveEntryList(
//      PLIST_ENTRY Entry
//      );
//

#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }

//
//  VOID
//  InsertTailList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    }

//
//  VOID
//  InsertHeadList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertHeadList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Flink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Flink = _EX_ListHead->Flink;\
    (Entry)->Flink = _EX_Flink;\
    (Entry)->Blink = _EX_ListHead;\
    _EX_Flink->Blink = (Entry);\
    _EX_ListHead->Flink = (Entry);\
    }

#endif //InitializeListHead

#endif  // __ENV_SPEC_W32__H_
