#define PASTE2(x,y)       x##y
#define PASTE(x,y)         PASTE2(x,y)

#ifdef EXPLICIT_32BIT
  #define STRUCT(x) PASTE(x,32)
  #define PTR(x) ULONG
#elif defined(EXPLICIT_64BIT)
  #define STRUCT(x) PASTE(x,64)
  #define PTR(x) ULONG64
#else
  #define STRUCT(x) x
  #define PTR(x) x
#endif

#if (defined(_WIN64) && !defined(EXPLICIT_32BIT)) || defined(EXPLICIT_64BIT)
  #define GDI_HANDLE_BUFFER_SIZE 60
#else
  #define GDI_HANDLE_BUFFER_SIZE 34
#endif

#if defined(_NTDDK_INCLUDED_) || defined(_NTIFS_)
#define PPEB PPEB_RENAMED
#endif

typedef struct STRUCT(_PEB)
{
    BOOLEAN InheritedAddressSpace;
    BOOLEAN ReadImageFileExecOptions;
    BOOLEAN BeingDebugged;
#if (NTDDI_VERSION >= NTDDI_WS03)
    union
    {
        BOOLEAN BitField;
        struct
        {
            BOOLEAN ImageUsesLargePages:1;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
            BOOLEAN IsProtectedProcess:1;
            BOOLEAN IsLegacyProcess:1;
            BOOLEAN IsImageDynamicallyRelocated:1;
            BOOLEAN SkipPatchingUser32Forwarders:1;
            BOOLEAN SpareBits:3;
#else
            BOOLEAN SpareBits:7;
#endif
        };
    };
#else
    BOOLEAN SpareBool;
#endif
    PTR(HANDLE) Mutant;
    PTR(PVOID) ImageBaseAddress;
    PTR(PPEB_LDR_DATA) Ldr;
    PTR(struct _RTL_USER_PROCESS_PARAMETERS*) ProcessParameters;
    PTR(PVOID) SubSystemData;
    PTR(PVOID) ProcessHeap;
    PTR(struct _RTL_CRITICAL_SECTION*) FastPebLock;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    PTR(PVOID) AltThunkSListPtr;
    PTR(PVOID) IFEOKey;
    union
    {
        ULONG CrossProcessFlags;
        struct
        {
            ULONG ProcessInJob:1;
            ULONG ProcessInitializing:1;
            ULONG ProcessUsingVEH:1;
            ULONG ProcessUsingVCH:1;
            ULONG ReservedBits0:28;
        };
    };
    union
    {
        PTR(PVOID) KernelCallbackTable;
        PTR(PVOID) UserSharedInfoPtr;
    };
#elif (NTDDI_VERSION >= NTDDI_WS03)
    PTR(PVOID) AltThunkSListPtr;
    PTR(PVOID) SparePtr2;
    ULONG EnvironmentUpdateCount;
    PTR(PVOID) KernelCallbackTable;
#else
    PTR(PPEBLOCKROUTINE) FastPebLockRoutine;
    PTR(PPEBLOCKROUTINE) FastPebUnlockRoutine;
    ULONG EnvironmentUpdateCount;
    PTR(PVOID) KernelCallbackTable;
#endif
    ULONG SystemReserved[1];
    ULONG SpareUlong; // AtlThunkSListPtr32
    PTR(PPEB_FREE_BLOCK) FreeList;
    ULONG TlsExpansionCounter;
    PTR(PVOID) TlsBitmap;
    ULONG TlsBitmapBits[2];
    PTR(PVOID) ReadOnlySharedMemoryBase;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    PTR(PVOID) HotpatchInformation;
#else
    PTR(PVOID) ReadOnlySharedMemoryHeap;
#endif
    PTR(PVOID*) ReadOnlyStaticServerData;
    PTR(PVOID) AnsiCodePageData;
    PTR(PVOID) OemCodePageData;
    PTR(PVOID) UnicodeCaseTableData;
    ULONG NumberOfProcessors;
    ULONG NtGlobalFlag;
    LARGE_INTEGER CriticalSectionTimeout;
    PTR(ULONG_PTR) HeapSegmentReserve;
    PTR(ULONG_PTR) HeapSegmentCommit;
    PTR(ULONG_PTR) HeapDeCommitTotalFreeThreshold;
    PTR(ULONG_PTR) HeapDeCommitFreeBlockThreshold;
    ULONG NumberOfHeaps;
    ULONG MaximumNumberOfHeaps;
    PTR(PVOID*) ProcessHeaps;
    PTR(PVOID) GdiSharedHandleTable;
    PTR(PVOID) ProcessStarterHelper;
    ULONG GdiDCAttributeList;
    PTR(struct _RTL_CRITICAL_SECTION*) LoaderLock;
    ULONG OSMajorVersion;
    ULONG OSMinorVersion;
    USHORT OSBuildNumber;
    USHORT OSCSDVersion;
    ULONG OSPlatformId;
    ULONG ImageSubsystem;
    ULONG ImageSubsystemMajorVersion;
    ULONG ImageSubsystemMinorVersion;
    PTR(ULONG_PTR) ImageProcessAffinityMask;
    ULONG GdiHandleBuffer[GDI_HANDLE_BUFFER_SIZE];
    PTR(PPOST_PROCESS_INIT_ROUTINE) PostProcessInitRoutine;
    PTR(PVOID) TlsExpansionBitmap;
    ULONG TlsExpansionBitmapBits[32];
    ULONG SessionId;
#if (NTDDI_VERSION >= NTDDI_WINXP)
    ULARGE_INTEGER AppCompatFlags;
    ULARGE_INTEGER AppCompatFlagsUser;
    PTR(PVOID) pShimData;
    PTR(PVOID) AppCompatInfo;
    STRUCT(UNICODE_STRING) CSDVersion;
    PTR(struct _ACTIVATION_CONTEXT_DATA*) ActivationContextData;
    PTR(struct _ASSEMBLY_STORAGE_MAP*) ProcessAssemblyStorageMap;
    PTR(struct _ACTIVATION_CONTEXT_DATA*) SystemDefaultActivationContextData;
    PTR(struct _ASSEMBLY_STORAGE_MAP*) SystemAssemblyStorageMap;
    PTR(ULONG_PTR) MinimumStackCommit;
#endif
#if (NTDDI_VERSION >= NTDDI_WS03)
    PTR(PVOID*) FlsCallback;
    STRUCT(LIST_ENTRY) FlsListHead;
    PTR(PVOID) FlsBitmap;
    ULONG FlsBitmapBits[4]; // [FLS_MAXIMUM_AVAILABLE/(sizeof(ULONG)*8)];
    ULONG FlsHighIndex;
#endif
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    PTR(PVOID) WerRegistrationData;
    PTR(PVOID) WerShipAssertPtr;
#endif
} STRUCT(PEB), *STRUCT(PPEB);

#undef PPEB

#if defined(_WIN64) && !defined(EXPLICIT_32BIT)
C_ASSERT(FIELD_OFFSET(STRUCT(PEB), Mutant) == 0x08);
C_ASSERT(FIELD_OFFSET(STRUCT(PEB), Ldr) == 0x18);
C_ASSERT(FIELD_OFFSET(STRUCT(PEB), FastPebLock) == 0x038);
C_ASSERT(FIELD_OFFSET(STRUCT(PEB), TlsExpansionCounter) == 0x070);
C_ASSERT(FIELD_OFFSET(STRUCT(PEB), NtGlobalFlag) == 0x0BC);
C_ASSERT(FIELD_OFFSET(STRUCT(PEB), GdiSharedHandleTable) == 0x0F8);
C_ASSERT(FIELD_OFFSET(STRUCT(PEB), LoaderLock) == 0x110);
C_ASSERT(FIELD_OFFSET(STRUCT(PEB), ImageSubsystem) == 0x128);
C_ASSERT(FIELD_OFFSET(STRUCT(PEB), ImageProcessAffinityMask) == 0x138);
C_ASSERT(FIELD_OFFSET(STRUCT(PEB), PostProcessInitRoutine) == 0x230);
C_ASSERT(FIELD_OFFSET(STRUCT(PEB), SessionId) == 0x2C0);
#if (NTDDI_VERSION >= NTDDI_WS03)
C_ASSERT(FIELD_OFFSET(STRUCT(PEB), FlsHighIndex) == 0x350);
#endif
#else
C_ASSERT(FIELD_OFFSET(STRUCT(PEB), Mutant) == 0x04);
C_ASSERT(FIELD_OFFSET(STRUCT(PEB), Ldr) == 0x0C);
C_ASSERT(FIELD_OFFSET(STRUCT(PEB), FastPebLock) == 0x01C);
C_ASSERT(FIELD_OFFSET(STRUCT(PEB), TlsExpansionCounter) == 0x03C);
C_ASSERT(FIELD_OFFSET(STRUCT(PEB), NtGlobalFlag) == 0x068);
C_ASSERT(FIELD_OFFSET(STRUCT(PEB), GdiSharedHandleTable) == 0x094);
C_ASSERT(FIELD_OFFSET(STRUCT(PEB), LoaderLock) == 0x0A0);
C_ASSERT(FIELD_OFFSET(STRUCT(PEB), ImageSubsystem) == 0x0B4);
C_ASSERT(FIELD_OFFSET(STRUCT(PEB), ImageProcessAffinityMask) == 0x0C0);
C_ASSERT(FIELD_OFFSET(STRUCT(PEB), PostProcessInitRoutine) == 0x14C);
C_ASSERT(FIELD_OFFSET(STRUCT(PEB), SessionId) == 0x1D4);
#if (NTDDI_VERSION >= NTDDI_WS03)
C_ASSERT(FIELD_OFFSET(STRUCT(PEB), FlsHighIndex) == 0x22C);
#endif
#endif

#define GDI_BATCH_BUFFER_SIZE 0x136
//
// GDI Batch Descriptor
//
typedef struct STRUCT(_GDI_TEB_BATCH)
{
    ULONG Offset;
    PTR(HANDLE) HDC;
    ULONG Buffer[GDI_BATCH_BUFFER_SIZE];
} STRUCT(GDI_TEB_BATCH), *STRUCT(PGDI_TEB_BATCH);

//
// Thread Environment Block (TEB)
//
typedef struct STRUCT(_TEB)
{
    STRUCT(NT_TIB)         NtTib;
    PTR(PVOID)             EnvironmentPointer;
    STRUCT(CLIENT_ID)      ClientId;
    PTR(PVOID)             ActiveRpcHandle;
    PTR(PVOID)             ThreadLocalStoragePointer;
    PTR(STRUCT(PPEB))      ProcessEnvironmentBlock;
    ULONG                  LastErrorValue;
    ULONG                  CountOfOwnedCriticalSections;
    PTR(PVOID)             CsrClientThread;
    PTR(PVOID)             Win32ThreadInfo;
    ULONG                  User32Reserved[26];
    ULONG                  UserReserved[5];
    PTR(PVOID)             WOW32Reserved;
    LCID                   CurrentLocale;
    ULONG                  FpSoftwareStatusRegister;
    PTR(PVOID)             SystemReserved1[54];
    LONG                   ExceptionCode;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    PTR(struct _ACTIVATION_CONTEXT_STACK*) ActivationContextStackPointer;
    UCHAR                  SpareBytes1[0x30 - 3 * sizeof(PTR(PVOID))];
    ULONG                  TxFsContext;
#elif (NTDDI_VERSION >= NTDDI_WS03)
    PTR(struct _ACTIVATION_CONTEXT_STACK*) ActivationContextStackPointer;
    UCHAR                  SpareBytes1[0x34 - 3 * sizeof(PTR(PVOID))];
#else
    ACTIVATION_CONTEXT_STACK ActivationContextStack;
    UCHAR                  SpareBytes1[24];
#endif
    STRUCT(GDI_TEB_BATCH)  GdiTebBatch;
    STRUCT(CLIENT_ID)      RealClientId;
    PTR(PVOID)             GdiCachedProcessHandle;
    ULONG                  GdiClientPID;
    ULONG                  GdiClientTID;
    PTR(PVOID)             GdiThreadLocalInfo;
    PTR(SIZE_T)            Win32ClientInfo[62];
    PTR(PVOID)             glDispatchTable[233];
    PTR(SIZE_T)            glReserved1[29];
    PTR(PVOID)             glReserved2;
    PTR(PVOID)             glSectionInfo;
    PTR(PVOID)             glSection;
    PTR(PVOID)             glTable;
    PTR(PVOID)             glCurrentRC;
    PTR(PVOID)             glContext;
    NTSTATUS               LastStatusValue;
    STRUCT(UNICODE_STRING) StaticUnicodeString;
    WCHAR                  StaticUnicodeBuffer[261];
    PTR(PVOID)             DeallocationStack;
    PTR(PVOID)             TlsSlots[64];
    STRUCT(LIST_ENTRY)     TlsLinks;
    PTR(PVOID)             Vdm;
    PTR(PVOID)             ReservedForNtRpc;
    PTR(PVOID)             DbgSsReserved[2];
#if (NTDDI_VERSION >= NTDDI_WS03)
    ULONG                  HardErrorMode;
#else
    ULONG                  HardErrorsAreDisabled;
#endif
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    PTR(PVOID)             Instrumentation[13 - sizeof(GUID)/sizeof(PTR(PVOID))];
    GUID                   ActivityId;
    PTR(PVOID)             SubProcessTag;
    PTR(PVOID)             EtwLocalData;
    PTR(PVOID)             EtwTraceData;
#elif (NTDDI_VERSION >= NTDDI_WS03)
    PTR(PVOID)             Instrumentation[14];
    PTR(PVOID)             SubProcessTag;
    PTR(PVOID)             EtwLocalData;
#else
    PTR(PVOID)             Instrumentation[16];
#endif
    PTR(PVOID)             WinSockData;
    ULONG                  GdiBatchCount;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    BOOLEAN                SpareBool0;
    BOOLEAN                SpareBool1;
    BOOLEAN                SpareBool2;
#else
    BOOLEAN                InDbgPrint;
    BOOLEAN                FreeStackOnTermination;
    BOOLEAN                HasFiberData;
#endif
    UCHAR                  IdealProcessor;
#if (NTDDI_VERSION >= NTDDI_WS03)
    ULONG                  GuaranteedStackBytes;
#else
    ULONG                  Spare3;
#endif
    PTR(PVOID)             ReservedForPerf;
    PTR(PVOID)             ReservedForOle;
    ULONG                  WaitingOnLoaderLock;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    PTR(PVOID)             SavedPriorityState;
    PTR(ULONG_PTR)         SoftPatchPtr1;
    PTR(ULONG_PTR)         ThreadPoolData;
#elif (NTDDI_VERSION >= NTDDI_WS03)
    PTR(ULONG_PTR)         SparePointer1;
    PTR(ULONG_PTR)         SoftPatchPtr1;
    PTR(ULONG_PTR)         SoftPatchPtr2;
#else
    Wx86ThreadState        Wx86Thread;
#endif
    PTR(PVOID*)            TlsExpansionSlots;
#if defined(_WIN64) && !defined(EXPLICIT_32BIT)
    PTR(PVOID)             DeallocationBStore;                                                  
    PTR(PVOID)             BStoreLimit;                                                         
#endif
    ULONG                  ImpersonationLocale;
    ULONG                  IsImpersonating;
    PTR(PVOID)             NlsCache;
    PTR(PVOID)             pShimData;
    ULONG                  HeapVirtualAffinity;
    PTR(HANDLE)            CurrentTransactionHandle;
    PTR(PTEB_ACTIVE_FRAME) ActiveFrame;
#if (NTDDI_VERSION >= NTDDI_WS03)
    PVOID FlsData;
#endif
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    PVOID PreferredLangauges;
    PVOID UserPrefLanguages;
    PVOID MergedPrefLanguages;
    ULONG MuiImpersonation;
    union
    {
        struct
        {
            USHORT SpareCrossTebFlags:16;
        };
        USHORT CrossTebFlags;
    };
    union
    {
        struct
        {
            USHORT DbgSafeThunkCall:1;
            USHORT DbgInDebugPrint:1;
            USHORT DbgHasFiberData:1;
            USHORT DbgSkipThreadAttach:1;
            USHORT DbgWerInShipAssertCode:1;
            USHORT DbgIssuedInitialBp:1;
            USHORT DbgClonedThread:1;
            USHORT SpareSameTebBits:9;
        };
        USHORT SameTebFlags;
    };
    PTR(PVOID) TxnScopeEntercallback;
    PTR(PVOID) TxnScopeExitCAllback;
    PTR(PVOID) TxnScopeContext;
    ULONG LockCount;
    ULONG ProcessRundown;
    ULONG64 LastSwitchTime;
    ULONG64 TotalSwitchOutTime;
    LARGE_INTEGER WaitReasonBitMap;
#else
    BOOLEAN SafeThunkCall;
    BOOLEAN BooleanSpare[3];
#endif
} STRUCT(TEB), *STRUCT(PTEB);

#if defined(_WIN64) && !defined(EXPLICIT_32BIT)
C_ASSERT(FIELD_OFFSET(STRUCT(TEB), EnvironmentPointer) == 0x038);
C_ASSERT(FIELD_OFFSET(STRUCT(TEB), ExceptionCode) == 0x2C0);
C_ASSERT(FIELD_OFFSET(STRUCT(TEB), GdiTebBatch) == 0x2F0);
C_ASSERT(FIELD_OFFSET(STRUCT(TEB), LastStatusValue) == 0x1250);
C_ASSERT(FIELD_OFFSET(STRUCT(TEB), Vdm) == 0x1690);
C_ASSERT(FIELD_OFFSET(STRUCT(TEB), HardErrorMode) == 0x16B0);
C_ASSERT(FIELD_OFFSET(STRUCT(TEB), GdiBatchCount) == 0x1740);
C_ASSERT(FIELD_OFFSET(STRUCT(TEB), IdealProcessor) == 0x1747);
C_ASSERT(FIELD_OFFSET(STRUCT(TEB), WaitingOnLoaderLock) == 0x1760);
C_ASSERT(FIELD_OFFSET(STRUCT(TEB), TlsExpansionSlots) == 0x1780);
C_ASSERT(FIELD_OFFSET(STRUCT(TEB), WaitingOnLoaderLock) == 0x1760);
C_ASSERT(FIELD_OFFSET(STRUCT(TEB), ActiveFrame) == 0x17C0);
#else
C_ASSERT(FIELD_OFFSET(STRUCT(TEB), EnvironmentPointer) == 0x01C);
C_ASSERT(FIELD_OFFSET(STRUCT(TEB), ExceptionCode) == 0x1A4);
C_ASSERT(FIELD_OFFSET(STRUCT(TEB), GdiTebBatch) == 0x1D4);
C_ASSERT(FIELD_OFFSET(STRUCT(TEB), LastStatusValue) == 0xBF4);
C_ASSERT(FIELD_OFFSET(STRUCT(TEB), Vdm) == 0xF18);
C_ASSERT(FIELD_OFFSET(STRUCT(TEB), GdiBatchCount) == 0xF70);
C_ASSERT(FIELD_OFFSET(STRUCT(TEB), TlsExpansionSlots) == 0xF94);
C_ASSERT(FIELD_OFFSET(STRUCT(TEB), ActiveFrame) == 0xFB0);
#endif

#undef PTR
#undef STRUCT
#undef PASTE
#undef PASTE2
#undef GDI_HANDLE_BUFFER_SIZE
