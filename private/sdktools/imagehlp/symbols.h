
//
// defines for symbol file searching
//
#define SYMBOL_PATH             "_NT_SYMBOL_PATH"
#define ALTERNATE_SYMBOL_PATH   "_NT_ALT_SYMBOL_PATH"
#define WINDIR                  "windir"
#define HASH_MODULO             253
#define OMAP_SYM_EXTRA          1024
#define CPP_EXTRA               2
#define OMAP_SYM_STRINGS        (OMAP_SYM_EXTRA * 256)
#define TMP_SYM_LEN             4096
#define MAX_SYM_NAME            2000
//
// structures
//
typedef struct _LOADED_MODULE {
    PENUMLOADED_MODULES_CALLBACK      EnumLoadedModulesCallback32;
    PENUMLOADED_MODULES_CALLBACK64      EnumLoadedModulesCallback64;
    PVOID                               Context;
} LOADED_MODULE, *PLOADED_MODULE;

typedef struct _PROCESS_ENTRY {
    LIST_ENTRY                      ListEntry;
    LIST_ENTRY                      ModuleList;
    ULONG                           Count;
    HANDLE                          hProcess;
    DWORD                           pid;
    LPSTR                           SymbolSearchPath;
    PSYMBOL_REGISTERED_CALLBACK     pCallbackFunction32;
    PSYMBOL_REGISTERED_CALLBACK64   pCallbackFunction64;
    ULONG64                         CallbackUserContext;
    PSYMBOL_FUNCENTRY_CALLBACK      pFunctionEntryCallback32;
    PSYMBOL_FUNCENTRY_CALLBACK64    pFunctionEntryCallback64;
    ULONG64                         FunctionEntryUserContext;
} PROCESS_ENTRY, *PPROCESS_ENTRY;

typedef struct _OMAP {
    ULONG  rva;
    ULONG  rvaTo;
} OMAP, *POMAP;

typedef struct _OMAPLIST {
   struct _OMAPLIST *next;
   OMAP             omap;
   ULONG            cb;
} OMAPLIST, *POMAPLIST;

#define SYMF_DUPLICATE    0x80000000
#define SYMF_OMAP_GENERATED   0x00000001
#define SYMF_OMAP_MODIFIED    0x00000002

typedef struct _SYMBOL_ENTRY {
    struct _SYMBOL_ENTRY        *Next;
    DWORD                       Size;
    DWORD                       Flags;
    DWORD64                     Address;
    LPSTR                       Name;
    ULONG                       NameLength;
} SYMBOL_ENTRY, *PSYMBOL_ENTRY;

typedef struct _SECTION_START {
    ULONG64                     Offset;
    DWORD                       Size;
    DWORD                       Flags;
} SECTION_START, *PSECTION_START;

//
// source file and line number information
//
typedef struct _SOURCE_LINE {
    DWORD64             Addr;
    DWORD               Line;
} SOURCE_LINE, *PSOURCE_LINE;

typedef struct _SOURCE_ENTRY {
    struct _SOURCE_ENTRY       *Next;
    DWORD64                     MinAddr;
    DWORD64                     MaxAddr;
    LPSTR                       File;
    DWORD                       Lines;
    PSOURCE_LINE                LineInfo;
    PVOID                       PdbModule;
} SOURCE_ENTRY, *PSOURCE_ENTRY;

//
// module flags
//
#define MIF_DEFERRED_LOAD   0x00000001
#define MIF_NO_SYMBOLS      0x00000002
#define MIF_ROM_IMAGE       0x00000004

typedef struct _MODULE_ENTRY {
    LIST_ENTRY                      ListEntry;
    ULONG64                         BaseOfDll;
    ULONG                           DllSize;
    ULONG                           TimeDateStamp;
    ULONG                           CheckSum;
    USHORT                          MachineType;
    CHAR                            ModuleName[32];
    CHAR                            AliasName[32];
    PSTR                            ImageName;
    PSTR                            LoadedImageName;
    PSYMBOL_ENTRY                   symbolTable;
    LPSTR                           SymStrings;
    PSYMBOL_ENTRY                   NameHashTable[HASH_MODULO];
    ULONG                           numsyms;
    ULONG                           MaxSyms;
    ULONG                           StringSize;
    SYM_TYPE                        SymType;
    PVOID                           pdb;
    PVOID                           dbi;
    PVOID                           gsi;
    PVOID                           globals;
    PVOID                           ptpi;
    PIMAGE_SECTION_HEADER           SectionHdrs;
    ULONG                           NumSections;
    PFPO_DATA                       pFpoData;       // pointer to fpo data (x86)
    PVOID                           pExceptionData; // pointer to pdata (risc)
    ULONG                           dwEntries;      // # of fpo or pdata recs
    POMAP                           pOmapFrom;      // pointer to omap data
    ULONG                           cOmapFrom;      // count of omap entries
    POMAP                           pOmapTo;        // pointer to omap data
    ULONG                           cOmapTo;        // count of omap entries
    SYMBOL_ENTRY                    TmpSym;         // used only for pdb symbols
    ULONG                           Flags;
    HANDLE                          hFile;
    PIMAGE_SECTION_HEADER           OriginalSectionHdrs;
    ULONG                           OriginalNumSections;
    PSOURCE_ENTRY                   SourceFiles;
    
    HANDLE                          hProcess;
    ULONG64                         InProcImageBase;
    BOOL                            fInProcHeader;
    DWORD                           dsExceptions;
} MODULE_ENTRY, *PMODULE_ENTRY;

typedef struct _PDB_INFO {
    CHAR    Signature[4];   // "NBxx"
    ULONG   Offset;         // always zero
    ULONG   sig;
    ULONG   age;
    CHAR    PdbName[_MAX_PATH];
} PDB_INFO, *PPDB_INFO;

#define n_name          N.ShortName
#define n_zeroes        N.Name.Short
#define n_nptr          N.LongName[1]
#define n_offset        N.Name.Long

// Forces symbols to be present for the given module, if possible.
// Returns BOOL.
#define ENSURE_SYMBOLS(hProcess, mi) \
    (((mi)->Flags & MIF_DEFERRED_LOAD) && !((mi)->Flags & MIF_NO_SYMBOLS) ? \
     CompleteDeferredSymbolLoad(hProcess, mi) : \
     TRUE)


//
// global externs
//
extern LIST_ENTRY      ProcessList;
extern BOOL            SymInitialized;
extern DWORD           SymOptions;


//
// internal prototypes
//
DWORD_PTR
GetPID(
    HANDLE hProcess
    );

DWORD
GetProcessModules(
    HANDLE                  hProcess,
    PINTERNAL_GET_MODULE    InternalGetModule,
    PVOID                   Context
    );

BOOL
InternalGetModule(
    HANDLE  hProcess,
    LPSTR   ModuleName,
    DWORD64 ImageBase,
    DWORD   ImageSize,
    PVOID   Context
    );

VOID
FreeModuleEntry(
    PMODULE_ENTRY ModuleEntry
    );

PPROCESS_ENTRY
FindProcessEntry(
    HANDLE  hProcess
    );

PPROCESS_ENTRY
FindFirstProcessEntry(
    );

VOID
GetSymName(
    PIMAGE_SYMBOL Symbol,
    PUCHAR        StringTable,
    LPSTR         s,
    DWORD         size
    );

BOOL
ProcessOmapSymbol(
    PMODULE_ENTRY   mi,
    PSYMBOL_ENTRY   sym
    );

DWORD64
ConvertOmapFromSrc(
    PMODULE_ENTRY  mi,
    DWORD64        addr,
    LPDWORD        bias
    );

DWORD64
ConvertOmapToSrc(
    PMODULE_ENTRY  mi,
    DWORD64        addr,
    LPDWORD        bias,
    BOOL           fBackup
    );

POMAP
GetOmapFromSrcEntry(
    PMODULE_ENTRY  mi,
    DWORD64        addr
    );

VOID
ProcessOmapForModule(
    PMODULE_ENTRY      mi,
    PIMGHLP_DEBUG_DATA pIDD
    );

BOOL
LoadCoffSymbols(
    HANDLE             hProcess,
    PMODULE_ENTRY      mi,
    PIMGHLP_DEBUG_DATA pIDD
    );

BOOL
LoadCodeViewSymbols(
    HANDLE             hProcess,
    PMODULE_ENTRY      mi,
    PIMGHLP_DEBUG_DATA pIDD
    );

ULONG
LoadExportSymbols(
    PMODULE_ENTRY      mi,
    PIMGHLP_DEBUG_DATA pIDD
    );

PMODULE_ENTRY
GetModuleForPC(
    PPROCESS_ENTRY  ProcessEntry,
    DWORD64         dwPcAddr,
    BOOL            ExactMatch
    );

PSYMBOL_ENTRY
GetSymFromAddr(
    DWORD64         dwAddr,
    PDWORD64        pqwDisplacement,
    PMODULE_ENTRY   mi
    );

LPSTR
StringDup(
    LPSTR str
    );

DWORD64
InternalLoadModule(
    IN  HANDLE          hProcess,
    IN  PSTR            ImageName,
    IN  PSTR            ModuleName,
    IN  DWORD64         BaseOfDll,
    IN  DWORD           SizeOfDll,
    IN  HANDLE          hFile
    );

DWORD
ComputeHash(
    LPSTR   lpname,
    ULONG   cb
    );

PSYMBOL_ENTRY
FindSymbolByName(
    PPROCESS_ENTRY  ProcessEntry,
    PMODULE_ENTRY   mi,
    LPSTR           SymName
    );

PFPO_DATA
SwSearchFpoData(
    DWORD     key,
    PFPO_DATA base,
    DWORD     num
    );

PIMAGE_FUNCTION_ENTRY
LookupFunctionEntryAxp32 (
    PMODULE_ENTRY mi,
    DWORD         ControlPc
    );

PIMAGE_FUNCTION_ENTRY64
LookupFunctionEntryAxp64 (
    PMODULE_ENTRY mi,
    DWORD64       ControlPc
    );

PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY
GetAlphaFunctionEntryFromDebugInfo (
    PPROCESS_ENTRY  ProcessEntry,
    DWORD64         ControlPc
    );

PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY
LookupFunctionEntryIA64 (
    PMODULE_ENTRY mi,
    DWORD         ControlPc
    );

BOOL
LoadedModuleEnumerator(
    HANDLE         hProcess,
    LPSTR          ModuleName,
    DWORD64        ImageBase,
    DWORD          ImageSize,
    PLOADED_MODULE lm
    );

BOOL
CompleteDeferredSymbolLoad(
    IN  HANDLE          hProcess,
    IN  PMODULE_ENTRY   mi
    );

LPSTR
symfmt(
    LPSTR DstName,
    LPSTR SrcName,
    ULONG Length
    );

PIMAGEHLP_SYMBOL
symcpy32(
    PIMAGEHLP_SYMBOL  External,
    PSYMBOL_ENTRY       Internal
    );

PIMAGEHLP_SYMBOL64
symcpy64(
    PIMAGEHLP_SYMBOL64  External,
    PSYMBOL_ENTRY       Internal
    );

BOOL
SympConvertSymbol32To64(
    PIMAGEHLP_SYMBOL Symbol32,
    PIMAGEHLP_SYMBOL64 Symbol64
    );

BOOL
SympConvertSymbol64To32(
    PIMAGEHLP_SYMBOL64 Symbol64,
    PIMAGEHLP_SYMBOL Symbol32
    );

BOOL
SympConvertLine32To64(
    PIMAGEHLP_LINE Line32,
    PIMAGEHLP_LINE64 Line64
    );

BOOL
SympConvertLine64To32(
    PIMAGEHLP_LINE64 Line64,
    PIMAGEHLP_LINE Line32
    );

BOOL
SympConvertAnsiModule32ToUnicodeModule32(
    PIMAGEHLP_MODULE  A_Symbol32,
    PIMAGEHLP_MODULEW W_Symbol32
    );

BOOL
SympConvertUnicodeModule32ToAnsiModule32(
    PIMAGEHLP_MODULEW W_Symbol32,
    PIMAGEHLP_MODULE  A_Symbol32
    );

BOOL
SympConvertAnsiModule64ToUnicodeModule64(
    PIMAGEHLP_MODULE64  A_Symbol64,
    PIMAGEHLP_MODULEW64 W_Symbol64
    );

BOOL
SympConvertUnicodeModule64ToAnsiModule64(
    PIMAGEHLP_MODULEW64 W_Symbol64,
    PIMAGEHLP_MODULE64  A_Symbol64
    );

PMODULE_ENTRY
FindModule(
    HANDLE hModule,
    PPROCESS_ENTRY ProcessEntry,
    LPSTR ModuleName,
    BOOL LoadSymbols
    );

LPSTR
SymUnDNameInternal(
    LPSTR UnDecName,
    DWORD UnDecNameLength,
    LPSTR DecName,
    DWORD MaxDecNameLength
    );

BOOL
GetLineFromAddr(
    PMODULE_ENTRY mi,
    DWORD64 Addr,
    PDWORD Displacement,
    PIMAGEHLP_LINE64 Line
    );

BOOL
FindLineByName(
    PMODULE_ENTRY mi,
    LPSTR FileName,
    DWORD LineNumber,
    PLONG Displacement,
    PIMAGEHLP_LINE64 Line
    );

BOOL
AddLinesForCoff(
    PMODULE_ENTRY mi,
    PIMAGE_SYMBOL allSymbols,
    DWORD numberOfSymbols,
    PIMAGE_LINENUMBER LineNumbers
    );

BOOL
AddLinesForOmfSourceModule(
    PMODULE_ENTRY mi,
    BYTE *Base,
    OMFSourceModule *OmfSrcMod,
    PVOID PdbModule
    );

DWORD
ReadInProcMemory(
    HANDLE    hProcess,
    DWORD64   addr,
    PVOID     buf,
    DWORD     bytes,
    DWORD    *bytesread
    );

BOOL
GetPData(
    HANDLE        hp,
    PMODULE_ENTRY mi
    );

VOID
SympSendDebugString(
    PPROCESS_ENTRY ProcessEntry,
    LPSTR          String
    );

int
WINAPIV
SympDprintf(
    PPROCESS_ENTRY ProcessEntry,
    LPSTR          Format,
    ...
    );

int
WINAPIV
SympEprintf(
    PPROCESS_ENTRY ProcessEntry,
    LPSTR Format,
    ...
    );

BOOL 
GetPdbTypeInfo(
   DBI *pDbi, 
   GSI *pGsi, 
   LPSTR symName, 
   BOOL PrefixMatch,
   IMAGEHLP_TYPES ReturnType,
   PBYTE pRetVal
   );

#define DPRINTF ((SymOptions & SYMOPT_DEBUG) == SYMOPT_DEBUG)&&SympDprintf
#define EPRINTF ((SymOptions & SYMOPT_DEBUG) == SYMOPT_DEBUG)&&SympEprintf
#define CPRINTF SympDprintf
