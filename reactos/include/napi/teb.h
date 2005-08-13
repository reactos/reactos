/* TEB/PEB parameters */
#ifndef __INCLUDE_INTERNAL_TEB
#define __INCLUDE_INTERNAL_TEB

#include <napi/types.h>

#ifdef __USE_W32API
#include <w32api.h>
#include <ddk/ntapi.h>
#endif /* !__USE_W32API */

typedef struct _CURDIR
{
   UNICODE_STRING DosPath;
   PVOID Handle;
} CURDIR, *PCURDIR;

typedef struct RTL_DRIVE_LETTER_CURDIR
{
   USHORT Flags;
   USHORT Length;
   ULONG TimeStamp;
   UNICODE_STRING DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

typedef struct _PEB_FREE_BLOCK
{
   struct _PEB_FREE_BLOCK* Next;
   ULONG Size;
} PEB_FREE_BLOCK, *PPEB_FREE_BLOCK;

/* RTL_USER_PROCESS_PARAMETERS.Flags */
#define PPF_NORMALIZED	(1)

#define TEB_BASE        (0x7FFDE000)
#define PEB_BASE        (0x7FFDF000)

typedef struct _PEB_LDR_DATA
{
   ULONG Length;
   BOOLEAN Initialized;
   PVOID SsHandle;
   LIST_ENTRY InLoadOrderModuleList;
   LIST_ENTRY InMemoryOrderModuleList;
   LIST_ENTRY InInitializationOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef VOID (STDCALL *PPEBLOCKROUTINE)(PVOID);

typedef struct _PEB
{
   UCHAR InheritedAddressSpace;                     /* 00h */
   UCHAR ReadImageFileExecOptions;                  /* 01h */
   UCHAR BeingDebugged;                             /* 02h */
   BOOLEAN SpareBool;                               /* 03h */
   HANDLE Mutant;                                   /* 04h */
   PVOID ImageBaseAddress;                          /* 08h */
   PPEB_LDR_DATA Ldr;                               /* 0Ch */
   PRTL_USER_PROCESS_PARAMETERS ProcessParameters;  /* 10h */
   PVOID SubSystemData;                             /* 14h */
   PVOID ProcessHeap;                               /* 18h */
   PVOID FastPebLock;                               /* 1Ch */
   PPEBLOCKROUTINE FastPebLockRoutine;              /* 20h */
   PPEBLOCKROUTINE FastPebUnlockRoutine;            /* 24h */
   ULONG EnvironmentUpdateCount;                    /* 28h */
   PVOID* KernelCallbackTable;                      /* 2Ch */
   PVOID EventLogSection;                           /* 30h */
   PVOID EventLog;                                  /* 34h */
   PPEB_FREE_BLOCK FreeList;                        /* 38h */
   ULONG TlsExpansionCounter;                       /* 3Ch */
   PVOID TlsBitmap;                                 /* 40h */
   ULONG TlsBitmapBits[0x2];                        /* 44h */
   PVOID ReadOnlySharedMemoryBase;                  /* 4Ch */
   PVOID ReadOnlySharedMemoryHeap;                  /* 50h */
   PVOID* ReadOnlyStaticServerData;                 /* 54h */
   PVOID AnsiCodePageData;                          /* 58h */
   PVOID OemCodePageData;                           /* 5Ch */
   PVOID UnicodeCaseTableData;                      /* 60h */
   ULONG NumberOfProcessors;                        /* 64h */
   ULONG NtGlobalFlag;                              /* 68h */
   LARGE_INTEGER CriticalSectionTimeout;            /* 70h */
   ULONG HeapSegmentReserve;                        /* 78h */
   ULONG HeapSegmentCommit;                         /* 7Ch */
   ULONG HeapDeCommitTotalFreeThreshold;            /* 80h */
   ULONG HeapDeCommitFreeBlockThreshold;            /* 84h */
   ULONG NumberOfHeaps;                             /* 88h */
   ULONG MaximumNumberOfHeaps;                      /* 8Ch */
   PVOID* ProcessHeaps;                             /* 90h */
   PVOID GdiSharedHandleTable;                      /* 94h */
   PVOID ProcessStarterHelper;                      /* 98h */
   PVOID GdiDCAttributeList;                        /* 9Ch */
   PVOID LoaderLock;                                /* A0h */
   ULONG OSMajorVersion;                            /* A4h */
   ULONG OSMinorVersion;                            /* A8h */
   USHORT OSBuildNumber;                            /* ACh */
   USHORT OSCSDVersion;                             /* AEh */
   ULONG OSPlatformId;                              /* B0h */
   ULONG ImageSubSystem;                            /* B4h */
   ULONG ImageSubSystemMajorVersion;                /* B8h */
   ULONG ImageSubSystemMinorVersion;                /* BCh */
   ULONG ImageProcessAffinityMask;                  /* C0h */
   ULONG GdiHandleBuffer[0x22];                     /* C4h */
   PVOID PostProcessInitRoutine;                    /* 14Ch */
   PVOID *TlsExpansionBitmap;                       /* 150h */
   ULONG TlsExpansionBitmapBits[0x20];              /* 154h */
   ULONG SessionId;                                 /* 1D4h */
   PVOID AppCompatInfo;                             /* 1D8h */
   UNICODE_STRING CSDVersion;                       /* 1DCh */
} PEB;

typedef struct _GDI_TEB_BATCH
{
   ULONG Offset;
   ULONG HDC;
   ULONG Buffer[0x136];
} GDI_TEB_BATCH, *PGDI_TEB_BATCH;

typedef struct _TEB
{
   NT_TIB Tib;                         /* 00h */
   PVOID EnvironmentPointer;           /* 1Ch */
   CLIENT_ID Cid;                      /* 20h */
   PVOID ActiveRpcInfo;                /* 28h */
   PVOID ThreadLocalStoragePointer;    /* 2Ch */
   PPEB Peb;                           /* 30h */
   ULONG LastErrorValue;               /* 34h */
   ULONG CountOfOwnedCriticalSections; /* 38h */
   PVOID CsrClientThread;              /* 3Ch */
   struct _W32THREAD* Win32ThreadInfo; /* 40h */
   ULONG Win32ClientInfo[0x1F];        /* 44h */
   PVOID WOW32Reserved;                /* C0h */
   LCID CurrentLocale;                 /* C4h */
   ULONG FpSoftwareStatusRegister;     /* C8h */
   PVOID SystemReserved1[0x36];        /* CCh */
   PVOID Spare1;                       /* 1A4h */
   LONG ExceptionCode;                 /* 1A8h */
   UCHAR SpareBytes1[0x28];            /* 1ACh */
   PVOID SystemReserved2[0xA];         /* 1D4h */
   GDI_TEB_BATCH GdiTebBatch;          /* 1FCh */
   ULONG gdiRgn;                       /* 6DCh */
   ULONG gdiPen;                       /* 6E0h */
   ULONG gdiBrush;                     /* 6E4h */
   CLIENT_ID RealClientId;             /* 6E8h */
   PVOID GdiCachedProcessHandle;       /* 6F0h */
   ULONG GdiClientPID;                 /* 6F4h */
   ULONG GdiClientTID;                 /* 6F8h */
   PVOID GdiThreadLocaleInfo;          /* 6FCh */
   PVOID UserReserved[5];              /* 700h */
   PVOID glDispatchTable[0x118];       /* 714h */
   ULONG glReserved1[0x1A];            /* B74h */
   PVOID glReserved2;                  /* BDCh */
   PVOID glSectionInfo;                /* BE0h */
   PVOID glSection;                    /* BE4h */
   PVOID glTable;                      /* BE8h */
   PVOID glCurrentRC;                  /* BECh */
   PVOID glContext;                    /* BF0h */
   NTSTATUS LastStatusValue;           /* BF4h */
   UNICODE_STRING StaticUnicodeString; /* BF8h */
   WCHAR StaticUnicodeBuffer[0x105];   /* C00h */
   PVOID DeallocationStack;            /* E0Ch */
   PVOID TlsSlots[0x40];               /* E10h */
   LIST_ENTRY TlsLinks;                /* F10h */
   PVOID Vdm;                          /* F18h */
   PVOID ReservedForNtRpc;             /* F1Ch */
   PVOID DbgSsReserved[0x2];           /* F20h */
   ULONG HardErrorDisabled;            /* F28h */
   PVOID Instrumentation[0x10];        /* F2Ch */
   PVOID WinSockData;                  /* F6Ch */
   ULONG GdiBatchCount;                /* F70h */
   USHORT Spare2;                      /* F74h */
   BOOLEAN IsFiber;                    /* F76h */
   UCHAR Spare3;                       /* F77h */
   ULONG Spare4;                       /* F78h */
   ULONG Spare5;                       /* F7Ch */
   PVOID ReservedForOle;               /* F80h */
   ULONG WaitingOnLoaderLock;          /* F84h */
   ULONG Unknown[11];                  /* F88h */
   PVOID FlsSlots;                     /* FB4h */
   PVOID WineDebugInfo;                /* Needed for WINE DLL's  */
} TEB, *PTEB;

#ifdef _M_IX86

static inline struct _PEB * NtCurrentPeb(void)
{
 struct _PEB * pPeb;

#if defined(__GNUC__)

 __asm__ __volatile__
 (
  "movl %%fs:0x30, %0\n" /* fs:30h == Teb->Peb */
  : "=r" (pPeb) /* can't have two memory operands */
  : /* no inputs */
 );

#elif defined(_MSC_VER)

	__asm mov eax, fs:0x30;
	__asm mov pPeb, eax

#else
#error Unknown compiler for inline assembler
#endif

 return pPeb;
}

#else
/* generic NtCurrentPeb() */
#define NtCurrentPeb() (NtCurrentTeb()->Peb)
#endif

#endif /* __INCLUDE_INTERNAL_TEB */
