/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */
#ifndef _DBGHELP_
#define _DBGHELP_

#ifdef _WIN64
#ifndef _IMAGEHLP64
#define _IMAGEHLP64
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define IMAGEAPI DECLSPEC_IMPORT WINAPI
#define DBHLP_DEPRECIATED __declspec(deprecated)

#define DBHLPAPI IMAGEAPI

#define IMAGE_SEPARATION (64*1024)

  typedef struct _LOADED_IMAGE {
    PSTR ModuleName;
    HANDLE hFile;
    PUCHAR MappedAddress;
#ifdef _IMAGEHLP64
    PIMAGE_NT_HEADERS64 FileHeader;
#else
    PIMAGE_NT_HEADERS32 FileHeader;
#endif
    PIMAGE_SECTION_HEADER LastRvaSection;
    ULONG NumberOfSections;
    PIMAGE_SECTION_HEADER Sections;
    ULONG Characteristics;
    BOOLEAN fSystemImage;
    BOOLEAN fDOSImage;
    LIST_ENTRY Links;
    ULONG SizeOfImage;
  } LOADED_IMAGE,*PLOADED_IMAGE;

#define MAX_SYM_NAME 2000

  typedef BOOL (CALLBACK *PFIND_DEBUG_FILE_CALLBACK)(HANDLE FileHandle,PSTR FileName,PVOID CallerData);
  typedef BOOL (CALLBACK *PFINDFILEINPATHCALLBACK)(PSTR filename,PVOID context);
  typedef BOOL (CALLBACK *PFIND_EXE_FILE_CALLBACK)(HANDLE FileHandle,PSTR FileName,PVOID CallerData);

  typedef BOOL (WINAPI *PSYMBOLSERVERPROC)(LPCSTR,LPCSTR,PVOID,DWORD,DWORD,LPSTR);
  typedef BOOL (WINAPI *PSYMBOLSERVEROPENPROC)(VOID);
  typedef BOOL (WINAPI *PSYMBOLSERVERCLOSEPROC)(VOID);
  typedef BOOL (WINAPI *PSYMBOLSERVERSETOPTIONSPROC)(UINT_PTR,ULONG64);
  typedef BOOL (CALLBACK WINAPI *PSYMBOLSERVERCALLBACKPROC)(UINT_PTR action,ULONG64 data,ULONG64 context);
  typedef UINT_PTR (WINAPI *PSYMBOLSERVERGETOPTIONSPROC)();
  typedef BOOL (WINAPI *PSYMBOLSERVERPINGPROC)(LPCSTR);

  HANDLE IMAGEAPI FindDebugInfoFile(PSTR FileName,PSTR SymbolPath,PSTR DebugFilePath);
  HANDLE IMAGEAPI FindDebugInfoFileEx(PSTR FileName,PSTR SymbolPath,PSTR DebugFilePath,PFIND_DEBUG_FILE_CALLBACK Callback,PVOID CallerData);
  BOOL IMAGEAPI SymFindFileInPath(HANDLE hprocess,LPSTR SearchPath,LPSTR FileName,PVOID id,DWORD two,DWORD three,DWORD flags,LPSTR FoundFile,PFINDFILEINPATHCALLBACK callback,PVOID context);
  HANDLE IMAGEAPI FindExecutableImage(PSTR FileName,PSTR SymbolPath,PSTR ImageFilePath);
  HANDLE IMAGEAPI FindExecutableImageEx(PSTR FileName,PSTR SymbolPath,PSTR ImageFilePath,PFIND_EXE_FILE_CALLBACK Callback,PVOID CallerData);
  PIMAGE_NT_HEADERS IMAGEAPI ImageNtHeader(PVOID Base);
  PVOID IMAGEAPI ImageDirectoryEntryToDataEx(PVOID Base,BOOLEAN MappedAsImage,USHORT DirectoryEntry,PULONG Size,PIMAGE_SECTION_HEADER *FoundHeader);
  PVOID IMAGEAPI ImageDirectoryEntryToData(PVOID Base,BOOLEAN MappedAsImage,USHORT DirectoryEntry,PULONG Size);
  PIMAGE_SECTION_HEADER IMAGEAPI ImageRvaToSection(PIMAGE_NT_HEADERS NtHeaders,PVOID Base,ULONG Rva);
  PVOID IMAGEAPI ImageRvaToVa(PIMAGE_NT_HEADERS NtHeaders,PVOID Base,ULONG Rva,PIMAGE_SECTION_HEADER *LastRvaSection);

#define SSRVOPT_CALLBACK 0x0001
#define SSRVOPT_DWORD 0x0002
#define SSRVOPT_DWORDPTR 0x0004
#define SSRVOPT_GUIDPTR 0x0008
#define SSRVOPT_OLDGUIDPTR 0x0010
#define SSRVOPT_UNATTENDED 0x0020
#define SSRVOPT_NOCOPY 0x0040
#define SSRVOPT_PARENTWIN 0x0080
#define SSRVOPT_PARAMTYPE 0x0100
#define SSRVOPT_SECURE 0x0200
#define SSRVOPT_TRACE 0x0400
#define SSRVOPT_SETCONTEXT 0x0800
#define SSRVOPT_PROXY 0x1000
#define SSRVOPT_DOWNSTREAM_STORE 0x2000
#define SSRVOPT_RESET ((ULONG_PTR)-1)

#define SSRVACTION_TRACE 1
#define SSRVACTION_QUERYCANCEL 2
#define SSRVACTION_EVENT 3

#ifndef _WIN64

  typedef struct _IMAGE_DEBUG_INFORMATION {
    LIST_ENTRY List;
    DWORD ReservedSize;
    PVOID ReservedMappedBase;
    USHORT ReservedMachine;
    USHORT ReservedCharacteristics;
    DWORD ReservedCheckSum;
    DWORD ImageBase;
    DWORD SizeOfImage;
    DWORD ReservedNumberOfSections;
    PIMAGE_SECTION_HEADER ReservedSections;
    DWORD ReservedExportedNamesSize;
    PSTR ReservedExportedNames;
    DWORD ReservedNumberOfFunctionTableEntries;
    PIMAGE_FUNCTION_ENTRY ReservedFunctionTableEntries;
    DWORD ReservedLowestFunctionStartingAddress;
    DWORD ReservedHighestFunctionEndingAddress;
    DWORD ReservedNumberOfFpoTableEntries;
    PFPO_DATA ReservedFpoTableEntries;
    DWORD SizeOfCoffSymbols;
    PIMAGE_COFF_SYMBOLS_HEADER CoffSymbols;
    DWORD ReservedSizeOfCodeViewSymbols;
    PVOID ReservedCodeViewSymbols;
    PSTR ImageFilePath;
    PSTR ImageFileName;
    PSTR ReservedDebugFilePath;
    DWORD ReservedTimeDateStamp;
    BOOL ReservedRomImage;
    PIMAGE_DEBUG_DIRECTORY ReservedDebugDirectory;
    DWORD ReservedNumberOfDebugDirectories;
    DWORD ReservedOriginalFunctionTableBaseAddress;
    DWORD Reserved[2 ];
  } IMAGE_DEBUG_INFORMATION,*PIMAGE_DEBUG_INFORMATION;

  PIMAGE_DEBUG_INFORMATION IMAGEAPI MapDebugInformation(HANDLE FileHandle,PSTR FileName,PSTR SymbolPath,DWORD ImageBase);
  BOOL IMAGEAPI UnmapDebugInformation(PIMAGE_DEBUG_INFORMATION DebugInfo);
#endif

  typedef BOOL (CALLBACK *PENUMDIRTREE_CALLBACK)(LPCSTR FilePath,PVOID CallerData);

  BOOL IMAGEAPI SearchTreeForFile(PSTR RootPath,PSTR InputPathName,PSTR OutputPathBuffer);
  BOOL IMAGEAPI EnumDirTree(HANDLE hProcess,PSTR RootPath,PSTR InputPathName,PSTR OutputPathBuffer,PENUMDIRTREE_CALLBACK Callback,PVOID CallbackData);
  BOOL IMAGEAPI MakeSureDirectoryPathExists(PCSTR DirPath);

#define UNDNAME_COMPLETE (0x0000)
#define UNDNAME_NO_LEADING_UNDERSCORES (0x0001)
#define UNDNAME_NO_MS_KEYWORDS (0x0002)
#define UNDNAME_NO_FUNCTION_RETURNS (0x0004)
#define UNDNAME_NO_ALLOCATION_MODEL (0x0008)
#define UNDNAME_NO_ALLOCATION_LANGUAGE (0x0010)
#define UNDNAME_NO_MS_THISTYPE (0x0020)
#define UNDNAME_NO_CV_THISTYPE (0x0040)
#define UNDNAME_NO_THISTYPE (0x0060)
#define UNDNAME_NO_ACCESS_SPECIFIERS (0x0080)
#define UNDNAME_NO_THROW_SIGNATURES (0x0100)
#define UNDNAME_NO_MEMBER_TYPE (0x0200)
#define UNDNAME_NO_RETURN_UDT_MODEL (0x0400)
#define UNDNAME_32_BIT_DECODE (0x0800)
#define UNDNAME_NAME_ONLY (0x1000)
#define UNDNAME_NO_ARGUMENTS (0x2000)
#define UNDNAME_NO_SPECIAL_SYMS (0x4000)

  DWORD IMAGEAPI WINAPI UnDecorateSymbolName(PCSTR DecoratedName,PSTR UnDecoratedName,DWORD UndecoratedLength,DWORD Flags);

#define DBHHEADER_DEBUGDIRS 0x1

  typedef struct _MODLOAD_DATA {
    DWORD ssize;
    DWORD ssig;
    PVOID data;
    DWORD size;
    DWORD flags;
  } MODLOAD_DATA,*PMODLOAD_DATA;

  typedef enum {
    AddrMode1616,AddrMode1632,AddrModeReal,AddrModeFlat
  } ADDRESS_MODE;

  typedef struct _tagADDRESS64 {
    DWORD64 Offset;
    WORD Segment;
    ADDRESS_MODE Mode;
  } ADDRESS64,*LPADDRESS64;

#ifdef _IMAGEHLP64
#define ADDRESS ADDRESS64
#define LPADDRESS LPADDRESS64
#else
  typedef struct _tagADDRESS {
    DWORD Offset;
    WORD Segment;
    ADDRESS_MODE Mode;
  } ADDRESS,*LPADDRESS;

  static __inline void Address32To64(LPADDRESS a32,LPADDRESS64 a64) {
    a64->Offset = (ULONG64)(LONG64)(LONG)a32->Offset;
    a64->Segment = a32->Segment;
    a64->Mode = a32->Mode;
  }

  static __inline void Address64To32(LPADDRESS64 a64,LPADDRESS a32) {
    a32->Offset = (ULONG)a64->Offset;
    a32->Segment = a64->Segment;
    a32->Mode = a64->Mode;
  }
#endif

  typedef struct _KDHELP64 {
    DWORD64 Thread;
    DWORD ThCallbackStack;
    DWORD ThCallbackBStore;
    DWORD NextCallback;
    DWORD FramePointer;
    DWORD64 KiCallUserMode;
    DWORD64 KeUserCallbackDispatcher;
    DWORD64 SystemRangeStart;
    DWORD64 Reserved[8];
  } KDHELP64,*PKDHELP64;

#ifdef _IMAGEHLP64
#define KDHELP KDHELP64
#define PKDHELP PKDHELP64
#else
  typedef struct _KDHELP {
    DWORD Thread;
    DWORD ThCallbackStack;
    DWORD NextCallback;
    DWORD FramePointer;
    DWORD KiCallUserMode;
    DWORD KeUserCallbackDispatcher;
    DWORD SystemRangeStart;
    DWORD ThCallbackBStore;
    DWORD Reserved[8];
  } KDHELP,*PKDHELP;

  static __inline void KdHelp32To64(PKDHELP p32,PKDHELP64 p64) {
    p64->Thread = p32->Thread;
    p64->ThCallbackStack = p32->ThCallbackStack;
    p64->NextCallback = p32->NextCallback;
    p64->FramePointer = p32->FramePointer;
    p64->KiCallUserMode = p32->KiCallUserMode;
    p64->KeUserCallbackDispatcher = p32->KeUserCallbackDispatcher;
    p64->SystemRangeStart = p32->SystemRangeStart;
  }
#endif

  typedef struct _tagSTACKFRAME64 {
    ADDRESS64 AddrPC;
    ADDRESS64 AddrReturn;
    ADDRESS64 AddrFrame;
    ADDRESS64 AddrStack;
    ADDRESS64 AddrBStore;
    PVOID FuncTableEntry;
    DWORD64 Params[4];
    BOOL Far;
    BOOL Virtual;
    DWORD64 Reserved[3];
    KDHELP64 KdHelp;
  } STACKFRAME64,*LPSTACKFRAME64;

#ifdef _IMAGEHLP64
#define STACKFRAME STACKFRAME64
#define LPSTACKFRAME LPSTACKFRAME64
#else
  typedef struct _tagSTACKFRAME {
    ADDRESS AddrPC;
    ADDRESS AddrReturn;
    ADDRESS AddrFrame;
    ADDRESS AddrStack;
    PVOID FuncTableEntry;
    DWORD Params[4];
    BOOL Far;
    BOOL Virtual;
    DWORD Reserved[3];
    KDHELP KdHelp;
    ADDRESS AddrBStore;
  } STACKFRAME,*LPSTACKFRAME;
#endif

  typedef BOOL (WINAPI *PREAD_PROCESS_MEMORY_ROUTINE64)(HANDLE hProcess,DWORD64 qwBaseAddress,PVOID lpBuffer,DWORD nSize,LPDWORD lpNumberOfBytesRead);
  typedef PVOID (WINAPI *PFUNCTION_TABLE_ACCESS_ROUTINE64)(HANDLE hProcess,DWORD64 AddrBase);
  typedef DWORD64 (WINAPI *PGET_MODULE_BASE_ROUTINE64)(HANDLE hProcess,DWORD64 Address);
  typedef DWORD64 (WINAPI *PTRANSLATE_ADDRESS_ROUTINE64)(HANDLE hProcess,HANDLE hThread,LPADDRESS64 lpaddr);

  BOOL IMAGEAPI StackWalk64(DWORD MachineType,HANDLE hProcess,HANDLE hThread,LPSTACKFRAME64 StackFrame,PVOID ContextRecord,PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress);

#ifdef _IMAGEHLP64
#define PREAD_PROCESS_MEMORY_ROUTINE PREAD_PROCESS_MEMORY_ROUTINE64
#define PFUNCTION_TABLE_ACCESS_ROUTINE PFUNCTION_TABLE_ACCESS_ROUTINE64
#define PGET_MODULE_BASE_ROUTINE PGET_MODULE_BASE_ROUTINE64
#define PTRANSLATE_ADDRESS_ROUTINE PTRANSLATE_ADDRESS_ROUTINE64
#define StackWalk StackWalk64
#else
  typedef BOOL (WINAPI *PREAD_PROCESS_MEMORY_ROUTINE)(HANDLE hProcess,DWORD lpBaseAddress,PVOID lpBuffer,DWORD nSize,PDWORD lpNumberOfBytesRead);
  typedef PVOID (WINAPI *PFUNCTION_TABLE_ACCESS_ROUTINE)(HANDLE hProcess,DWORD AddrBase);
  typedef DWORD (WINAPI *PGET_MODULE_BASE_ROUTINE)(HANDLE hProcess,DWORD Address);
  typedef DWORD (WINAPI *PTRANSLATE_ADDRESS_ROUTINE)(HANDLE hProcess,HANDLE hThread,LPADDRESS lpaddr);

  BOOL IMAGEAPI StackWalk(DWORD MachineType,HANDLE hProcess,HANDLE hThread,LPSTACKFRAME StackFrame,PVOID ContextRecord,PREAD_PROCESS_MEMORY_ROUTINE ReadMemoryRoutine,PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine,PGET_MODULE_BASE_ROUTINE GetModuleBaseRoutine,PTRANSLATE_ADDRESS_ROUTINE TranslateAddress);
#endif

#define API_VERSION_NUMBER 9

  typedef struct API_VERSION {
    USHORT MajorVersion;
    USHORT MinorVersion;
    USHORT Revision;
    USHORT Reserved;
  } API_VERSION,*LPAPI_VERSION;

  LPAPI_VERSION IMAGEAPI ImagehlpApiVersion(VOID);
  LPAPI_VERSION IMAGEAPI ImagehlpApiVersionEx(LPAPI_VERSION AppVersion);
  DWORD IMAGEAPI GetTimestampForLoadedLibrary(HMODULE Module);

  typedef BOOL (CALLBACK *PSYM_ENUMMODULES_CALLBACK64)(PSTR ModuleName,DWORD64 BaseOfDll,PVOID UserContext);
  typedef BOOL (CALLBACK *PSYM_ENUMSYMBOLS_CALLBACK64)(PSTR SymbolName,DWORD64 SymbolAddress,ULONG SymbolSize,PVOID UserContext);
  typedef BOOL (CALLBACK *PSYM_ENUMSYMBOLS_CALLBACK64W)(PWSTR SymbolName,DWORD64 SymbolAddress,ULONG SymbolSize,PVOID UserContext);
  typedef BOOL (CALLBACK *PENUMLOADED_MODULES_CALLBACK64)(PSTR ModuleName,DWORD64 ModuleBase,ULONG ModuleSize,PVOID UserContext);
  typedef BOOL (CALLBACK *PSYMBOL_REGISTERED_CALLBACK64)(HANDLE hProcess,ULONG ActionCode,ULONG64 CallbackData,ULONG64 UserContext);
  typedef PVOID (CALLBACK *PSYMBOL_FUNCENTRY_CALLBACK)(HANDLE hProcess,DWORD AddrBase,PVOID UserContext);
  typedef PVOID (CALLBACK *PSYMBOL_FUNCENTRY_CALLBACK64)(HANDLE hProcess,ULONG64 AddrBase,ULONG64 UserContext);

#ifdef _IMAGEHLP64
#define PSYM_ENUMMODULES_CALLBACK PSYM_ENUMMODULES_CALLBACK64
#define PSYM_ENUMSYMBOLS_CALLBACK PSYM_ENUMSYMBOLS_CALLBACK64
#define PSYM_ENUMSYMBOLS_CALLBACKW PSYM_ENUMSYMBOLS_CALLBACK64W
#define PENUMLOADED_MODULES_CALLBACK PENUMLOADED_MODULES_CALLBACK64
#define PSYMBOL_REGISTERED_CALLBACK PSYMBOL_REGISTERED_CALLBACK64
#define PSYMBOL_FUNCENTRY_CALLBACK PSYMBOL_FUNCENTRY_CALLBACK64
#else
  typedef BOOL (CALLBACK *PSYM_ENUMMODULES_CALLBACK)(PSTR ModuleName,ULONG BaseOfDll,PVOID UserContext);
  typedef BOOL (CALLBACK *PSYM_ENUMSYMBOLS_CALLBACK)(PSTR SymbolName,ULONG SymbolAddress,ULONG SymbolSize,PVOID UserContext);
  typedef BOOL (CALLBACK *PSYM_ENUMSYMBOLS_CALLBACKW)(PWSTR SymbolName,ULONG SymbolAddress,ULONG SymbolSize,PVOID UserContext);
  typedef BOOL (CALLBACK *PENUMLOADED_MODULES_CALLBACK)(PSTR ModuleName,ULONG ModuleBase,ULONG ModuleSize,PVOID UserContext);
  typedef BOOL (CALLBACK *PSYMBOL_REGISTERED_CALLBACK)(HANDLE hProcess,ULONG ActionCode,PVOID CallbackData,PVOID UserContext);
#endif

#define SYMFLAG_VALUEPRESENT 0x00000001
#define SYMFLAG_REGISTER 0x00000008
#define SYMFLAG_REGREL 0x00000010
#define SYMFLAG_FRAMEREL 0x00000020
#define SYMFLAG_PARAMETER 0x00000040
#define SYMFLAG_LOCAL 0x00000080
#define SYMFLAG_CONSTANT 0x00000100
#define SYMFLAG_EXPORT 0x00000200
#define SYMFLAG_FORWARDER 0x00000400
#define SYMFLAG_FUNCTION 0x00000800
#define SYMFLAG_VIRTUAL 0x00001000
#define SYMFLAG_THUNK 0x00002000
#define SYMFLAG_TLSREL 0x00004000

  typedef enum {
    SymNone = 0,SymCoff,SymCv,SymPdb,SymExport,SymDeferred,SymSym,SymDia,SymVirtual,NumSymTypes
  } SYM_TYPE;

  typedef struct _IMAGEHLP_SYMBOL64 {
    DWORD SizeOfStruct;
    DWORD64 Address;
    DWORD Size;
    DWORD Flags;
    DWORD MaxNameLength;
    CHAR Name[1];
  } IMAGEHLP_SYMBOL64,*PIMAGEHLP_SYMBOL64;

  typedef struct _IMAGEHLP_SYMBOL64_PACKAGE {
    IMAGEHLP_SYMBOL64 sym;
    CHAR name[MAX_SYM_NAME + 1];
  } IMAGEHLP_SYMBOL64_PACKAGE,*PIMAGEHLP_SYMBOL64_PACKAGE;

#ifdef _IMAGEHLP64

#define IMAGEHLP_SYMBOL IMAGEHLP_SYMBOL64
#define PIMAGEHLP_SYMBOL PIMAGEHLP_SYMBOL64
#define IMAGEHLP_SYMBOL_PACKAGE IMAGEHLP_SYMBOL64_PACKAGE
#define PIMAGEHLP_SYMBOL_PACKAGE PIMAGEHLP_SYMBOL64_PACKAGE
#else

  typedef struct _IMAGEHLP_SYMBOL {
    DWORD SizeOfStruct;
    DWORD Address;
    DWORD Size;
    DWORD Flags;
    DWORD MaxNameLength;
    CHAR Name[1];
  } IMAGEHLP_SYMBOL,*PIMAGEHLP_SYMBOL;

  typedef struct _IMAGEHLP_SYMBOL_PACKAGE {
    IMAGEHLP_SYMBOL sym;
    CHAR name[MAX_SYM_NAME + 1];
  } IMAGEHLP_SYMBOL_PACKAGE,*PIMAGEHLP_SYMBOL_PACKAGE;
#endif

  typedef struct _IMAGEHLP_MODULE64 {
    DWORD SizeOfStruct;
    DWORD64 BaseOfImage;
    DWORD ImageSize;
    DWORD TimeDateStamp;
    DWORD CheckSum;
    DWORD NumSyms;
    SYM_TYPE SymType;
    CHAR ModuleName[32];
    CHAR ImageName[256];
    CHAR LoadedImageName[256];
    CHAR LoadedPdbName[256];
    DWORD CVSig;
    CHAR CVData[MAX_PATH*3];
    DWORD PdbSig;
    GUID PdbSig70;
    DWORD PdbAge;
    BOOL PdbUnmatched;
    BOOL DbgUnmatched;
    BOOL LineNumbers;
    BOOL GlobalSymbols;
    BOOL TypeInfo;
  } IMAGEHLP_MODULE64,*PIMAGEHLP_MODULE64;

  typedef struct _IMAGEHLP_MODULE64W {
    DWORD SizeOfStruct;
    DWORD64 BaseOfImage;
    DWORD ImageSize;
    DWORD TimeDateStamp;
    DWORD CheckSum;
    DWORD NumSyms;
    SYM_TYPE SymType;
    WCHAR ModuleName[32];
    WCHAR ImageName[256];
    WCHAR LoadedImageName[256];
    WCHAR LoadedPdbName[256];
    DWORD CVSig;
    WCHAR CVData[MAX_PATH*3];
    DWORD PdbSig;
    GUID PdbSig70;
    DWORD PdbAge;
    BOOL PdbUnmatched;
    BOOL DbgUnmatched;
    BOOL LineNumbers;
    BOOL GlobalSymbols;
    BOOL TypeInfo;
  } IMAGEHLP_MODULEW64,*PIMAGEHLP_MODULEW64;

#ifdef _IMAGEHLP64
#define IMAGEHLP_MODULE IMAGEHLP_MODULE64
#define PIMAGEHLP_MODULE PIMAGEHLP_MODULE64
#define IMAGEHLP_MODULEW IMAGEHLP_MODULEW64
#define PIMAGEHLP_MODULEW PIMAGEHLP_MODULEW64
#else
  typedef struct _IMAGEHLP_MODULE {
    DWORD SizeOfStruct;
    DWORD BaseOfImage;
    DWORD ImageSize;
    DWORD TimeDateStamp;
    DWORD CheckSum;
    DWORD NumSyms;
    SYM_TYPE SymType;
    CHAR ModuleName[32];
    CHAR ImageName[256];
    CHAR LoadedImageName[256];
  } IMAGEHLP_MODULE,*PIMAGEHLP_MODULE;

  typedef struct _IMAGEHLP_MODULEW {
    DWORD SizeOfStruct;
    DWORD BaseOfImage;
    DWORD ImageSize;
    DWORD TimeDateStamp;
    DWORD CheckSum;
    DWORD NumSyms;
    SYM_TYPE SymType;
    WCHAR ModuleName[32];
    WCHAR ImageName[256];
    WCHAR LoadedImageName[256];
  } IMAGEHLP_MODULEW,*PIMAGEHLP_MODULEW;
#endif

  typedef struct _IMAGEHLP_LINE64 {
    DWORD SizeOfStruct;
    PVOID Key;
    DWORD LineNumber;
    PCHAR FileName;
    DWORD64 Address;
  } IMAGEHLP_LINE64,*PIMAGEHLP_LINE64;

#ifdef _IMAGEHLP64
#define IMAGEHLP_LINE IMAGEHLP_LINE64
#define PIMAGEHLP_LINE PIMAGEHLP_LINE64
#else
  typedef struct _IMAGEHLP_LINE {
    DWORD SizeOfStruct;
    PVOID Key;
    DWORD LineNumber;
    PCHAR FileName;
    DWORD Address;
  } IMAGEHLP_LINE,*PIMAGEHLP_LINE;
#endif

  typedef struct _SOURCEFILE {
    DWORD64 ModBase;
    PCHAR FileName;
  } SOURCEFILE,*PSOURCEFILE;

#define CBA_DEFERRED_SYMBOL_LOAD_START 0x00000001
#define CBA_DEFERRED_SYMBOL_LOAD_COMPLETE 0x00000002
#define CBA_DEFERRED_SYMBOL_LOAD_FAILURE 0x00000003
#define CBA_SYMBOLS_UNLOADED 0x00000004
#define CBA_DUPLICATE_SYMBOL 0x00000005
#define CBA_READ_MEMORY 0x00000006
#define CBA_DEFERRED_SYMBOL_LOAD_CANCEL 0x00000007
#define CBA_SET_OPTIONS 0x00000008
#define CBA_EVENT 0x00000010
#define CBA_DEFERRED_SYMBOL_LOAD_PARTIAL 0x00000020
#define CBA_DEBUG_INFO 0x10000000

  typedef struct _IMAGEHLP_CBA_READ_MEMORY {
    DWORD64 addr;
    PVOID buf;
    DWORD bytes;
    DWORD *bytesread;
  } IMAGEHLP_CBA_READ_MEMORY,*PIMAGEHLP_CBA_READ_MEMORY;

  enum {
    sevInfo = 0,sevProblem,sevAttn,sevFatal,sevMax
  };

  typedef struct _IMAGEHLP_CBA_EVENT {
    DWORD severity;
    DWORD code;
    PCHAR desc;
    PVOID object;
  } IMAGEHLP_CBA_EVENT,*PIMAGEHLP_CBA_EVENT;

  typedef struct _IMAGEHLP_DEFERRED_SYMBOL_LOAD64 {
    DWORD SizeOfStruct;
    DWORD64 BaseOfImage;
    DWORD CheckSum;
    DWORD TimeDateStamp;
    CHAR FileName[MAX_PATH];
    BOOLEAN Reparse;
    HANDLE hFile;
    DWORD Flags;
  } IMAGEHLP_DEFERRED_SYMBOL_LOAD64,*PIMAGEHLP_DEFERRED_SYMBOL_LOAD64;

#define DSLFLAG_MISMATCHED_PDB 0x1
#define DSLFLAG_MISMATCHED_DBG 0x2

#ifdef _IMAGEHLP64
#define IMAGEHLP_DEFERRED_SYMBOL_LOAD IMAGEHLP_DEFERRED_SYMBOL_LOAD64
#define PIMAGEHLP_DEFERRED_SYMBOL_LOAD PIMAGEHLP_DEFERRED_SYMBOL_LOAD64
#else
  typedef struct _IMAGEHLP_DEFERRED_SYMBOL_LOAD {
    DWORD SizeOfStruct;
    DWORD BaseOfImage;
    DWORD CheckSum;
    DWORD TimeDateStamp;
    CHAR FileName[MAX_PATH];
    BOOLEAN Reparse;
    HANDLE hFile;
  } IMAGEHLP_DEFERRED_SYMBOL_LOAD,*PIMAGEHLP_DEFERRED_SYMBOL_LOAD;
#endif

  typedef struct _IMAGEHLP_DUPLICATE_SYMBOL64 {
    DWORD SizeOfStruct;
    DWORD NumberOfDups;
    PIMAGEHLP_SYMBOL64 Symbol;
    DWORD SelectedSymbol;
  } IMAGEHLP_DUPLICATE_SYMBOL64,*PIMAGEHLP_DUPLICATE_SYMBOL64;

#ifdef _IMAGEHLP64
#define IMAGEHLP_DUPLICATE_SYMBOL IMAGEHLP_DUPLICATE_SYMBOL64
#define PIMAGEHLP_DUPLICATE_SYMBOL PIMAGEHLP_DUPLICATE_SYMBOL64
#else
  typedef struct _IMAGEHLP_DUPLICATE_SYMBOL {
    DWORD SizeOfStruct;
    DWORD NumberOfDups;
    PIMAGEHLP_SYMBOL Symbol;
    DWORD SelectedSymbol;
  } IMAGEHLP_DUPLICATE_SYMBOL,*PIMAGEHLP_DUPLICATE_SYMBOL;
#endif

  BOOL IMAGEAPI SymSetParentWindow(HWND hwnd);
  PCHAR IMAGEAPI SymSetHomeDirectory(PCSTR dir);
  PCHAR IMAGEAPI SymGetHomeDirectory(DWORD type,PSTR dir,size_t size);

  enum {
    hdBase = 0,hdSym,hdSrc,hdMax
  };

#define SYMOPT_CASE_INSENSITIVE 0x00000001
#define SYMOPT_UNDNAME 0x00000002
#define SYMOPT_DEFERRED_LOADS 0x00000004
#define SYMOPT_NO_CPP 0x00000008
#define SYMOPT_LOAD_LINES 0x00000010
#define SYMOPT_OMAP_FIND_NEAREST 0x00000020
#define SYMOPT_LOAD_ANYTHING 0x00000040
#define SYMOPT_IGNORE_CVREC 0x00000080
#define SYMOPT_NO_UNQUALIFIED_LOADS 0x00000100
#define SYMOPT_FAIL_CRITICAL_ERRORS 0x00000200
#define SYMOPT_EXACT_SYMBOLS 0x00000400
#define SYMOPT_ALLOW_ABSOLUTE_SYMBOLS 0x00000800
#define SYMOPT_IGNORE_NT_SYMPATH 0x00001000
#define SYMOPT_INCLUDE_32BIT_MODULES 0x00002000
#define SYMOPT_PUBLICS_ONLY 0x00004000
#define SYMOPT_NO_PUBLICS 0x00008000
#define SYMOPT_AUTO_PUBLICS 0x00010000
#define SYMOPT_NO_IMAGE_SEARCH 0x00020000
#define SYMOPT_SECURE 0x00040000
#define SYMOPT_NO_PROMPTS 0x00080000

#define SYMOPT_DEBUG 0x80000000

  DWORD IMAGEAPI SymSetOptions(DWORD SymOptions);
  DWORD IMAGEAPI SymGetOptions(VOID);
  BOOL IMAGEAPI SymCleanup(HANDLE hProcess);
  BOOL IMAGEAPI SymMatchString(LPSTR string,LPSTR expression,BOOL fCase);

  typedef BOOL (CALLBACK *PSYM_ENUMSOURCFILES_CALLBACK)(PSOURCEFILE pSourceFile,PVOID UserContext);

  BOOL IMAGEAPI SymEnumSourceFiles(HANDLE hProcess,ULONG64 ModBase,LPSTR Mask,PSYM_ENUMSOURCFILES_CALLBACK cbSrcFiles,PVOID UserContext);
  BOOL IMAGEAPI SymEnumerateModules64(HANDLE hProcess,PSYM_ENUMMODULES_CALLBACK64 EnumModulesCallback,PVOID UserContext);

#ifdef _IMAGEHLP64
#define SymEnumerateModules SymEnumerateModules64
#else
  BOOL IMAGEAPI SymEnumerateModules(HANDLE hProcess,PSYM_ENUMMODULES_CALLBACK EnumModulesCallback,PVOID UserContext);
#endif

  BOOL IMAGEAPI SymEnumerateSymbols64(HANDLE hProcess,DWORD64 BaseOfDll,PSYM_ENUMSYMBOLS_CALLBACK64 EnumSymbolsCallback,PVOID UserContext);
  BOOL IMAGEAPI SymEnumerateSymbolsW64(HANDLE hProcess,DWORD64 BaseOfDll,PSYM_ENUMSYMBOLS_CALLBACK64W EnumSymbolsCallback,PVOID UserContext);

#ifdef _IMAGEHLP64
#define SymEnumerateSymbols SymEnumerateSymbols64
#define SymEnumerateSymbolsW SymEnumerateSymbolsW64
#else
  BOOL IMAGEAPI SymEnumerateSymbols(HANDLE hProcess,DWORD BaseOfDll,PSYM_ENUMSYMBOLS_CALLBACK EnumSymbolsCallback,PVOID UserContext);
  BOOL IMAGEAPI SymEnumerateSymbolsW(HANDLE hProcess,DWORD BaseOfDll,PSYM_ENUMSYMBOLS_CALLBACKW EnumSymbolsCallback,PVOID UserContext);
#endif

  BOOL IMAGEAPI EnumerateLoadedModules64(HANDLE hProcess,PENUMLOADED_MODULES_CALLBACK64 EnumLoadedModulesCallback,PVOID UserContext);
#ifdef _IMAGEHLP64
#define EnumerateLoadedModules EnumerateLoadedModules64
#else
  BOOL IMAGEAPI EnumerateLoadedModules(HANDLE hProcess,PENUMLOADED_MODULES_CALLBACK EnumLoadedModulesCallback,PVOID UserContext);
#endif

  PVOID IMAGEAPI SymFunctionTableAccess64(HANDLE hProcess,DWORD64 AddrBase);

#ifdef _IMAGEHLP64
#define SymFunctionTableAccess SymFunctionTableAccess64
#else
  PVOID IMAGEAPI SymFunctionTableAccess(HANDLE hProcess,DWORD AddrBase);
#endif

  BOOL IMAGEAPI SymGetModuleInfo64(HANDLE hProcess,DWORD64 qwAddr,PIMAGEHLP_MODULE64 ModuleInfo);
  BOOL IMAGEAPI SymGetModuleInfoW64(HANDLE hProcess,DWORD64 qwAddr,PIMAGEHLP_MODULEW64 ModuleInfo);

#ifdef _IMAGEHLP64
#define SymGetModuleInfo SymGetModuleInfo64
#define SymGetModuleInfoW SymGetModuleInfoW64
#else
  BOOL IMAGEAPI SymGetModuleInfo(HANDLE hProcess,DWORD dwAddr,PIMAGEHLP_MODULE ModuleInfo);
  BOOL IMAGEAPI SymGetModuleInfoW(HANDLE hProcess,DWORD dwAddr,PIMAGEHLP_MODULEW ModuleInfo);
#endif

  DWORD64 IMAGEAPI SymGetModuleBase64(HANDLE hProcess,DWORD64 qwAddr);

#ifdef _IMAGEHLP64
#define SymGetModuleBase SymGetModuleBase64
#else
  DWORD IMAGEAPI SymGetModuleBase(HANDLE hProcess,DWORD dwAddr);
#endif

  BOOL IMAGEAPI SymGetSymNext64(HANDLE hProcess,PIMAGEHLP_SYMBOL64 Symbol);

#ifdef _IMAGEHLP64
#define SymGetSymNext SymGetSymNext64
#else
  BOOL IMAGEAPI SymGetSymNext(HANDLE hProcess,PIMAGEHLP_SYMBOL Symbol);
#endif

  BOOL IMAGEAPI SymGetSymPrev64(HANDLE hProcess,PIMAGEHLP_SYMBOL64 Symbol);

#ifdef _IMAGEHLP64
#define SymGetSymPrev SymGetSymPrev64
#else
  BOOL IMAGEAPI SymGetSymPrev(HANDLE hProcess,PIMAGEHLP_SYMBOL Symbol);
#endif

  typedef struct _SRCCODEINFO {
    DWORD SizeOfStruct;
    PVOID Key;
    DWORD64 ModBase;
    CHAR Obj[MAX_PATH + 1];
    CHAR FileName[MAX_PATH + 1];
    DWORD LineNumber;
    DWORD64 Address;
  } SRCCODEINFO,*PSRCCODEINFO;

  typedef BOOL (CALLBACK *PSYM_ENUMLINES_CALLBACK)(PSRCCODEINFO LineInfo,PVOID UserContext);

  BOOL IMAGEAPI SymEnumLines(HANDLE hProcess,ULONG64 Base,PCSTR Obj,PCSTR File,PSYM_ENUMLINES_CALLBACK EnumLinesCallback,PVOID UserContext);
  BOOL IMAGEAPI SymGetLineFromAddr64(HANDLE hProcess,DWORD64 qwAddr,PDWORD pdwDisplacement,PIMAGEHLP_LINE64 Line64);

#ifdef _IMAGEHLP64
#define SymGetLineFromAddr SymGetLineFromAddr64
#else
  BOOL IMAGEAPI SymGetLineFromAddr(HANDLE hProcess,DWORD dwAddr,PDWORD pdwDisplacement,PIMAGEHLP_LINE Line);
#endif

  BOOL IMAGEAPI SymGetLineFromName64(HANDLE hProcess,PSTR ModuleName,PSTR FileName,DWORD dwLineNumber,PLONG plDisplacement,PIMAGEHLP_LINE64 Line);

#ifdef _IMAGEHLP64
#define SymGetLineFromName SymGetLineFromName64
#else
  BOOL IMAGEAPI SymGetLineFromName(HANDLE hProcess,PSTR ModuleName,PSTR FileName,DWORD dwLineNumber,PLONG plDisplacement,PIMAGEHLP_LINE Line);
#endif

  BOOL IMAGEAPI SymGetLineNext64(HANDLE hProcess,PIMAGEHLP_LINE64 Line);

#ifdef _IMAGEHLP64
#define SymGetLineNext SymGetLineNext64
#else
  BOOL IMAGEAPI SymGetLineNext(HANDLE hProcess,PIMAGEHLP_LINE Line);
#endif

  BOOL IMAGEAPI SymGetLinePrev64(HANDLE hProcess,PIMAGEHLP_LINE64 Line);

#ifdef _IMAGEHLP64
#define SymGetLinePrev SymGetLinePrev64
#else
  BOOL IMAGEAPI SymGetLinePrev(HANDLE hProcess,PIMAGEHLP_LINE Line);
#endif

  BOOL IMAGEAPI SymMatchFileName(PSTR FileName,PSTR Match,PSTR *FileNameStop,PSTR *MatchStop);
  BOOL IMAGEAPI SymInitialize(HANDLE hProcess,PSTR UserSearchPath,BOOL fInvadeProcess);
  BOOL IMAGEAPI SymGetSearchPath(HANDLE hProcess,PSTR SearchPath,DWORD SearchPathLength);
  BOOL IMAGEAPI SymSetSearchPath(HANDLE hProcess,PSTR SearchPath);
  DWORD64 IMAGEAPI SymLoadModule64(HANDLE hProcess,HANDLE hFile,PSTR ImageName,PSTR ModuleName,DWORD64 BaseOfDll,DWORD SizeOfDll);

#define SLMFLAG_VIRTUAL 0x1

  DWORD64 IMAGEAPI SymLoadModuleEx(HANDLE hProcess,HANDLE hFile,PSTR ImageName,PSTR ModuleName,DWORD64 BaseOfDll,DWORD DllSize,PMODLOAD_DATA Data,DWORD Flags);

#ifdef _IMAGEHLP64
#define SymLoadModule SymLoadModule64
#else
  DWORD IMAGEAPI SymLoadModule(HANDLE hProcess,HANDLE hFile,PSTR ImageName,PSTR ModuleName,DWORD BaseOfDll,DWORD SizeOfDll);
#endif

  BOOL IMAGEAPI SymUnloadModule64(HANDLE hProcess,DWORD64 BaseOfDll);

#ifdef _IMAGEHLP64
#define SymUnloadModule SymUnloadModule64
#else
  BOOL IMAGEAPI SymUnloadModule(HANDLE hProcess,DWORD BaseOfDll);
#endif

  BOOL IMAGEAPI SymUnDName64(PIMAGEHLP_SYMBOL64 sym,PSTR UnDecName,DWORD UnDecNameLength);

#ifdef _IMAGEHLP64
#define SymUnDName SymUnDName64
#else
  BOOL IMAGEAPI SymUnDName(PIMAGEHLP_SYMBOL sym,PSTR UnDecName,DWORD UnDecNameLength);
#endif

  BOOL IMAGEAPI SymRegisterCallback64(HANDLE hProcess,PSYMBOL_REGISTERED_CALLBACK64 CallbackFunction,ULONG64 UserContext);

  BOOL IMAGEAPI SymRegisterFunctionEntryCallback64(HANDLE hProcess,PSYMBOL_FUNCENTRY_CALLBACK64 CallbackFunction,ULONG64 UserContext);

#ifdef _IMAGEHLP64
#define SymRegisterCallback SymRegisterCallback64
#define SymRegisterFunctionEntryCallback SymRegisterFunctionEntryCallback64
#else
  BOOL IMAGEAPI SymRegisterCallback(HANDLE hProcess,PSYMBOL_REGISTERED_CALLBACK CallbackFunction,PVOID UserContext);
  BOOL IMAGEAPI SymRegisterFunctionEntryCallback(HANDLE hProcess,PSYMBOL_FUNCENTRY_CALLBACK CallbackFunction,PVOID UserContext);
#endif

  typedef struct _IMAGEHLP_SYMBOL_SRC {
    DWORD sizeofstruct;
    DWORD type;
    char file[MAX_PATH];
  } IMAGEHLP_SYMBOL_SRC,*PIMAGEHLP_SYMBOL_SRC;

  typedef struct _MODULE_TYPE_INFO {
    USHORT dataLength;
    USHORT leaf;
    BYTE data[1];
  } MODULE_TYPE_INFO,*PMODULE_TYPE_INFO;

  typedef struct _SYMBOL_INFO {
    ULONG SizeOfStruct;
    ULONG TypeIndex;
    ULONG64 Reserved[2];
    ULONG info;
    ULONG Size;
    ULONG64 ModBase;
    ULONG Flags;
    ULONG64 Value;
    ULONG64 Address;
    ULONG Register;
    ULONG Scope;
    ULONG Tag;
    ULONG NameLen;
    ULONG MaxNameLen;
    CHAR Name[1];
  } SYMBOL_INFO,*PSYMBOL_INFO;

  typedef struct _SYMBOL_INFO_PACKAGE {
    SYMBOL_INFO si;
    CHAR name[MAX_SYM_NAME + 1];
  } SYMBOL_INFO_PACKAGE,*PSYMBOL_INFO_PACKAGE;

  typedef struct _IMAGEHLP_STACK_FRAME
  {
    ULONG64 InstructionOffset;
    ULONG64 ReturnOffset;
    ULONG64 FrameOffset;
    ULONG64 StackOffset;
    ULONG64 BackingStoreOffset;
    ULONG64 FuncTableEntry;
    ULONG64 Params[4];
    ULONG64 Reserved[5];
    BOOL Virtual;
    ULONG Reserved2;
  } IMAGEHLP_STACK_FRAME,*PIMAGEHLP_STACK_FRAME;

  typedef VOID IMAGEHLP_CONTEXT,*PIMAGEHLP_CONTEXT;

  BOOL IMAGEAPI SymSetContext(HANDLE hProcess,PIMAGEHLP_STACK_FRAME StackFrame,PIMAGEHLP_CONTEXT Context);
  BOOL IMAGEAPI SymFromAddr(HANDLE hProcess,DWORD64 Address,PDWORD64 Displacement,PSYMBOL_INFO Symbol);
  BOOL IMAGEAPI SymFromToken(HANDLE hProcess,DWORD64 Base,DWORD Token,PSYMBOL_INFO Symbol);
  BOOL IMAGEAPI SymFromName(HANDLE hProcess,LPSTR Name,PSYMBOL_INFO Symbol);

  typedef BOOL (CALLBACK *PSYM_ENUMERATESYMBOLS_CALLBACK)(PSYMBOL_INFO pSymInfo,ULONG SymbolSize,PVOID UserContext);

  BOOL IMAGEAPI SymEnumSymbols(HANDLE hProcess,ULONG64 BaseOfDll,PCSTR Mask,PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,PVOID UserContext);
  BOOL IMAGEAPI SymEnumSymbolsForAddr(HANDLE hProcess,DWORD64 Address,PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,PVOID UserContext);

#define SYMENUMFLAG_FULLSRCH 1
#define SYMENUMFLAG_SPEEDSRCH 2

  typedef enum _IMAGEHLP_SYMBOL_TYPE_INFO {
    TI_GET_SYMTAG,TI_GET_SYMNAME,TI_GET_LENGTH,TI_GET_TYPE,TI_GET_TYPEID,TI_GET_BASETYPE,TI_GET_ARRAYINDEXTYPEID,TI_FINDCHILDREN,
    TI_GET_DATAKIND,TI_GET_ADDRESSOFFSET,TI_GET_OFFSET,TI_GET_VALUE,TI_GET_COUNT,TI_GET_CHILDRENCOUNT,TI_GET_BITPOSITION,TI_GET_VIRTUALBASECLASS,
    TI_GET_VIRTUALTABLESHAPEID,TI_GET_VIRTUALBASEPOINTEROFFSET,TI_GET_CLASSPARENTID,TI_GET_NESTED,TI_GET_SYMINDEX,TI_GET_LEXICALPARENT,
    TI_GET_ADDRESS,TI_GET_THISADJUST,TI_GET_UDTKIND,TI_IS_EQUIV_TO,TI_GET_CALLING_CONVENTION
  } IMAGEHLP_SYMBOL_TYPE_INFO;

  typedef struct _TI_FINDCHILDREN_PARAMS {
    ULONG Count;
    ULONG Start;
    ULONG ChildId[1];
  } TI_FINDCHILDREN_PARAMS;

  BOOL IMAGEAPI SymGetTypeInfo(HANDLE hProcess,DWORD64 ModBase,ULONG TypeId,IMAGEHLP_SYMBOL_TYPE_INFO GetType,PVOID pInfo);
  BOOL IMAGEAPI SymEnumTypes(HANDLE hProcess,ULONG64 BaseOfDll,PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,PVOID UserContext);
  BOOL IMAGEAPI SymGetTypeFromName(HANDLE hProcess,ULONG64 BaseOfDll,LPSTR Name,PSYMBOL_INFO Symbol);
  BOOL IMAGEAPI SymAddSymbol(HANDLE hProcess,ULONG64 BaseOfDll,PCSTR Name,DWORD64 Address,DWORD Size,DWORD Flags);
  BOOL IMAGEAPI SymDeleteSymbol(HANDLE hProcess,ULONG64 BaseOfDll,PCSTR Name,DWORD64 Address,DWORD Flags);

  typedef BOOL (WINAPI *PDBGHELP_CREATE_USER_DUMP_CALLBACK)(DWORD DataType,PVOID *Data,LPDWORD DataLength,PVOID UserData);

  BOOL WINAPI DbgHelpCreateUserDump(LPSTR FileName,PDBGHELP_CREATE_USER_DUMP_CALLBACK Callback,PVOID UserData);
  BOOL WINAPI DbgHelpCreateUserDumpW(LPWSTR FileName,PDBGHELP_CREATE_USER_DUMP_CALLBACK Callback,PVOID UserData);
  BOOL IMAGEAPI SymGetSymFromAddr64(HANDLE hProcess,DWORD64 qwAddr,PDWORD64 pdwDisplacement,PIMAGEHLP_SYMBOL64 Symbol);

#ifdef _IMAGEHLP64
#define SymGetSymFromAddr SymGetSymFromAddr64
#else
  BOOL IMAGEAPI SymGetSymFromAddr(HANDLE hProcess,DWORD dwAddr,PDWORD pdwDisplacement,PIMAGEHLP_SYMBOL Symbol);
#endif

  BOOL IMAGEAPI SymGetSymFromName64(HANDLE hProcess,PSTR Name,PIMAGEHLP_SYMBOL64 Symbol);

#ifdef _IMAGEHLP64
#define SymGetSymFromName SymGetSymFromName64
#else
  BOOL IMAGEAPI SymGetSymFromName(HANDLE hProcess,PSTR Name,PIMAGEHLP_SYMBOL Symbol);
#endif

  DBHLP_DEPRECIATED BOOL IMAGEAPI FindFileInPath(HANDLE hprocess,LPSTR SearchPath,LPSTR FileName,PVOID id,DWORD two,DWORD three,DWORD flags,LPSTR FilePath);
  DBHLP_DEPRECIATED BOOL IMAGEAPI FindFileInSearchPath(HANDLE hprocess,LPSTR SearchPath,LPSTR FileName,DWORD one,DWORD two,DWORD three,LPSTR FilePath);
  DBHLP_DEPRECIATED BOOL IMAGEAPI SymEnumSym(HANDLE hProcess,ULONG64 BaseOfDll,PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,PVOID UserContext);

#define SYMF_OMAP_GENERATED 0x00000001
#define SYMF_OMAP_MODIFIED 0x00000002
#define SYMF_REGISTER 0x00000008
#define SYMF_REGREL 0x00000010
#define SYMF_FRAMEREL 0x00000020
#define SYMF_PARAMETER 0x00000040
#define SYMF_LOCAL 0x00000080
#define SYMF_CONSTANT 0x00000100
#define SYMF_EXPORT 0x00000200
#define SYMF_FORWARDER 0x00000400
#define SYMF_FUNCTION 0x00000800
#define SYMF_VIRTUAL 0x00001000
#define SYMF_THUNK 0x00002000
#define SYMF_TLSREL 0x00004000

#define IMAGEHLP_SYMBOL_INFO_VALUEPRESENT 1
#define IMAGEHLP_SYMBOL_INFO_REGISTER SYMF_REGISTER
#define IMAGEHLP_SYMBOL_INFO_REGRELATIVE SYMF_REGREL
#define IMAGEHLP_SYMBOL_INFO_FRAMERELATIVE SYMF_FRAMEREL
#define IMAGEHLP_SYMBOL_INFO_PARAMETER SYMF_PARAMETER
#define IMAGEHLP_SYMBOL_INFO_LOCAL SYMF_LOCAL
#define IMAGEHLP_SYMBOL_INFO_CONSTANT SYMF_CONSTANT
#define IMAGEHLP_SYMBOL_FUNCTION SYMF_FUNCTION
#define IMAGEHLP_SYMBOL_VIRTUAL SYMF_VIRTUAL
#define IMAGEHLP_SYMBOL_THUNK SYMF_THUNK
#define IMAGEHLP_SYMBOL_INFO_TLSRELATIVE SYMF_TLSREL

#include <pshpack4.h>

#define MINIDUMP_SIGNATURE ('PMDM')
#define MINIDUMP_VERSION (42899)
  typedef DWORD RVA;
  typedef ULONG64 RVA64;

  typedef struct _MINIDUMP_LOCATION_DESCRIPTOR {
    ULONG32 DataSize;
    RVA Rva;
  } MINIDUMP_LOCATION_DESCRIPTOR;

  typedef struct _MINIDUMP_LOCATION_DESCRIPTOR64 {
    ULONG64 DataSize;
    RVA64 Rva;
  } MINIDUMP_LOCATION_DESCRIPTOR64;

  typedef struct _MINIDUMP_MEMORY_DESCRIPTOR {
    ULONG64 StartOfMemoryRange;
    MINIDUMP_LOCATION_DESCRIPTOR Memory;
  } MINIDUMP_MEMORY_DESCRIPTOR,*PMINIDUMP_MEMORY_DESCRIPTOR;

  typedef struct _MINIDUMP_MEMORY_DESCRIPTOR64 {
    ULONG64 StartOfMemoryRange;
    ULONG64 DataSize;
  } MINIDUMP_MEMORY_DESCRIPTOR64,*PMINIDUMP_MEMORY_DESCRIPTOR64;

  typedef struct _MINIDUMP_HEADER {
    ULONG32 Signature;
    ULONG32 Version;
    ULONG32 NumberOfStreams;
    RVA StreamDirectoryRva;
    ULONG32 CheckSum;
    union {
      ULONG32 Reserved;
      ULONG32 TimeDateStamp;
    } DUMMYUNIONNAME;
    ULONG64 Flags;
  } MINIDUMP_HEADER,*PMINIDUMP_HEADER;

  typedef struct _MINIDUMP_DIRECTORY {
    ULONG32 StreamType;
    MINIDUMP_LOCATION_DESCRIPTOR Location;
  } MINIDUMP_DIRECTORY,*PMINIDUMP_DIRECTORY;

  typedef struct _MINIDUMP_STRING {
    ULONG32 Length;
    WCHAR Buffer [0];
  } MINIDUMP_STRING,*PMINIDUMP_STRING;

  typedef enum _MINIDUMP_STREAM_TYPE {
    UnusedStream = 0,ReservedStream0 = 1,ReservedStream1 = 2,ThreadListStream = 3,ModuleListStream = 4,MemoryListStream = 5,
    ExceptionStream = 6,SystemInfoStream = 7,ThreadExListStream = 8,Memory64ListStream = 9,CommentStreamA = 10,CommentStreamW = 11,
    HandleDataStream = 12,FunctionTableStream = 13,UnloadedModuleListStream = 14,MiscInfoStream = 15,LastReservedStream = 0xffff
  } MINIDUMP_STREAM_TYPE;

  typedef union _CPU_INFORMATION {
    struct {
      ULONG32 VendorId [3 ];
      ULONG32 VersionInformation;
      ULONG32 FeatureInformation;
      ULONG32 AMDExtendedCpuFeatures;
    } X86CpuInfo;
    struct {
      ULONG64 ProcessorFeatures [2 ];
    } OtherCpuInfo;
  } CPU_INFORMATION,*PCPU_INFORMATION;

  typedef struct _MINIDUMP_SYSTEM_INFO {
    USHORT ProcessorArchitecture;
    USHORT ProcessorLevel;
    USHORT ProcessorRevision;
    union {
      USHORT Reserved0;
      struct {
	UCHAR NumberOfProcessors;
	UCHAR ProductType;
      } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    ULONG32 MajorVersion;
    ULONG32 MinorVersion;
    ULONG32 BuildNumber;
    ULONG32 PlatformId;
    RVA CSDVersionRva;
    union {
      ULONG32 Reserved1;
      struct {
	USHORT SuiteMask;
	USHORT Reserved2;
      } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME1;
    CPU_INFORMATION Cpu;
  } MINIDUMP_SYSTEM_INFO,*PMINIDUMP_SYSTEM_INFO;

  C_ASSERT (sizeof (((PPROCESS_INFORMATION)0)->dwThreadId)==4);

  typedef struct _MINIDUMP_THREAD {
    ULONG32 ThreadId;
    ULONG32 SuspendCount;
    ULONG32 PriorityClass;
    ULONG32 Priority;
    ULONG64 Teb;
    MINIDUMP_MEMORY_DESCRIPTOR Stack;
    MINIDUMP_LOCATION_DESCRIPTOR ThreadContext;
  } MINIDUMP_THREAD,*PMINIDUMP_THREAD;

  typedef struct _MINIDUMP_THREAD_LIST {
    ULONG32 NumberOfThreads;
    MINIDUMP_THREAD Threads [0];
  } MINIDUMP_THREAD_LIST,*PMINIDUMP_THREAD_LIST;

  typedef struct _MINIDUMP_THREAD_EX {
    ULONG32 ThreadId;
    ULONG32 SuspendCount;
    ULONG32 PriorityClass;
    ULONG32 Priority;
    ULONG64 Teb;
    MINIDUMP_MEMORY_DESCRIPTOR Stack;
    MINIDUMP_LOCATION_DESCRIPTOR ThreadContext;
    MINIDUMP_MEMORY_DESCRIPTOR BackingStore;
  } MINIDUMP_THREAD_EX,*PMINIDUMP_THREAD_EX;

  typedef struct _MINIDUMP_THREAD_EX_LIST {
    ULONG32 NumberOfThreads;
    MINIDUMP_THREAD_EX Threads [0];
  } MINIDUMP_THREAD_EX_LIST,*PMINIDUMP_THREAD_EX_LIST;

  typedef struct _MINIDUMP_EXCEPTION {
    ULONG32 ExceptionCode;
    ULONG32 ExceptionFlags;
    ULONG64 ExceptionRecord;
    ULONG64 ExceptionAddress;
    ULONG32 NumberParameters;
    ULONG32 __unusedAlignment;
    ULONG64 ExceptionInformation [EXCEPTION_MAXIMUM_PARAMETERS ];
  } MINIDUMP_EXCEPTION,*PMINIDUMP_EXCEPTION;

  typedef struct MINIDUMP_EXCEPTION_STREAM {
    ULONG32 ThreadId;
    ULONG32 __alignment;
    MINIDUMP_EXCEPTION ExceptionRecord;
    MINIDUMP_LOCATION_DESCRIPTOR ThreadContext;
  } MINIDUMP_EXCEPTION_STREAM,*PMINIDUMP_EXCEPTION_STREAM;

  typedef struct _MINIDUMP_MODULE {
    ULONG64 BaseOfImage;
    ULONG32 SizeOfImage;
    ULONG32 CheckSum;
    ULONG32 TimeDateStamp;
    RVA ModuleNameRva;
    VS_FIXEDFILEINFO VersionInfo;
    MINIDUMP_LOCATION_DESCRIPTOR CvRecord;
    MINIDUMP_LOCATION_DESCRIPTOR MiscRecord;
    ULONG64 Reserved0;
    ULONG64 Reserved1;
  } MINIDUMP_MODULE,*PMINIDUMP_MODULE;

  typedef struct _MINIDUMP_MODULE_LIST {
    ULONG32 NumberOfModules;
    MINIDUMP_MODULE Modules [0 ];
  } MINIDUMP_MODULE_LIST,*PMINIDUMP_MODULE_LIST;

  typedef struct _MINIDUMP_MEMORY_LIST {
    ULONG32 NumberOfMemoryRanges;
    MINIDUMP_MEMORY_DESCRIPTOR MemoryRanges [0];
  } MINIDUMP_MEMORY_LIST,*PMINIDUMP_MEMORY_LIST;

  typedef struct _MINIDUMP_MEMORY64_LIST {
    ULONG64 NumberOfMemoryRanges;
    RVA64 BaseRva;
    MINIDUMP_MEMORY_DESCRIPTOR64 MemoryRanges [0];
  } MINIDUMP_MEMORY64_LIST,*PMINIDUMP_MEMORY64_LIST;

  typedef struct _MINIDUMP_EXCEPTION_INFORMATION {
    DWORD ThreadId;
    PEXCEPTION_POINTERS ExceptionPointers;
    BOOL ClientPointers;
  } MINIDUMP_EXCEPTION_INFORMATION,*PMINIDUMP_EXCEPTION_INFORMATION;

  typedef struct _MINIDUMP_EXCEPTION_INFORMATION64 {
    DWORD ThreadId;
    ULONG64 ExceptionRecord;
    ULONG64 ContextRecord;
    BOOL ClientPointers;
  } MINIDUMP_EXCEPTION_INFORMATION64,*PMINIDUMP_EXCEPTION_INFORMATION64;

  typedef struct _MINIDUMP_HANDLE_DESCRIPTOR {
    ULONG64 Handle;
    RVA TypeNameRva;
    RVA ObjectNameRva;
    ULONG32 Attributes;
    ULONG32 GrantedAccess;
    ULONG32 HandleCount;
    ULONG32 PointerCount;
  } MINIDUMP_HANDLE_DESCRIPTOR,*PMINIDUMP_HANDLE_DESCRIPTOR;

  typedef struct _MINIDUMP_HANDLE_DATA_STREAM {
    ULONG32 SizeOfHeader;
    ULONG32 SizeOfDescriptor;
    ULONG32 NumberOfDescriptors;
    ULONG32 Reserved;
  } MINIDUMP_HANDLE_DATA_STREAM,*PMINIDUMP_HANDLE_DATA_STREAM;

  typedef struct _MINIDUMP_FUNCTION_TABLE_DESCRIPTOR {
    ULONG64 MinimumAddress;
    ULONG64 MaximumAddress;
    ULONG64 BaseAddress;
    ULONG32 EntryCount;
    ULONG32 SizeOfAlignPad;
  } MINIDUMP_FUNCTION_TABLE_DESCRIPTOR,*PMINIDUMP_FUNCTION_TABLE_DESCRIPTOR;

  typedef struct _MINIDUMP_FUNCTION_TABLE_STREAM {
    ULONG32 SizeOfHeader;
    ULONG32 SizeOfDescriptor;
    ULONG32 SizeOfNativeDescriptor;
    ULONG32 SizeOfFunctionEntry;
    ULONG32 NumberOfDescriptors;
    ULONG32 SizeOfAlignPad;
  } MINIDUMP_FUNCTION_TABLE_STREAM,*PMINIDUMP_FUNCTION_TABLE_STREAM;

  typedef struct _MINIDUMP_UNLOADED_MODULE {
    ULONG64 BaseOfImage;
    ULONG32 SizeOfImage;
    ULONG32 CheckSum;
    ULONG32 TimeDateStamp;
    RVA ModuleNameRva;
  } MINIDUMP_UNLOADED_MODULE,*PMINIDUMP_UNLOADED_MODULE;

  typedef struct _MINIDUMP_UNLOADED_MODULE_LIST {
    ULONG32 SizeOfHeader;
    ULONG32 SizeOfEntry;
    ULONG32 NumberOfEntries;
  } MINIDUMP_UNLOADED_MODULE_LIST,*PMINIDUMP_UNLOADED_MODULE_LIST;

#define MINIDUMP_MISC1_PROCESS_ID 0x00000001
#define MINIDUMP_MISC1_PROCESS_TIMES 0x00000002

  typedef struct _MINIDUMP_MISC_INFO {
    ULONG32 SizeOfInfo;
    ULONG32 Flags1;
    ULONG32 ProcessId;
    ULONG32 ProcessCreateTime;
    ULONG32 ProcessUserTime;
    ULONG32 ProcessKernelTime;
  } MINIDUMP_MISC_INFO,*PMINIDUMP_MISC_INFO;

  typedef struct _MINIDUMP_USER_RECORD {
    ULONG32 Type;
    MINIDUMP_LOCATION_DESCRIPTOR Memory;
  } MINIDUMP_USER_RECORD,*PMINIDUMP_USER_RECORD;

  typedef struct _MINIDUMP_USER_STREAM {
    ULONG32 Type;
    ULONG BufferSize;
    PVOID Buffer;
  } MINIDUMP_USER_STREAM,*PMINIDUMP_USER_STREAM;

  typedef struct _MINIDUMP_USER_STREAM_INFORMATION {
    ULONG UserStreamCount;
    PMINIDUMP_USER_STREAM UserStreamArray;
  } MINIDUMP_USER_STREAM_INFORMATION,*PMINIDUMP_USER_STREAM_INFORMATION;

  typedef enum _MINIDUMP_CALLBACK_TYPE {
    ModuleCallback,ThreadCallback,ThreadExCallback,IncludeThreadCallback,IncludeModuleCallback,MemoryCallback
  } MINIDUMP_CALLBACK_TYPE;

  typedef struct _MINIDUMP_THREAD_CALLBACK {
    ULONG ThreadId;
    HANDLE ThreadHandle;
    CONTEXT Context;
    ULONG SizeOfContext;
    ULONG64 StackBase;
    ULONG64 StackEnd;
  } MINIDUMP_THREAD_CALLBACK,*PMINIDUMP_THREAD_CALLBACK;

  typedef struct _MINIDUMP_THREAD_EX_CALLBACK {
    ULONG ThreadId;
    HANDLE ThreadHandle;
    CONTEXT Context;
    ULONG SizeOfContext;
    ULONG64 StackBase;
    ULONG64 StackEnd;
    ULONG64 BackingStoreBase;
    ULONG64 BackingStoreEnd;
  } MINIDUMP_THREAD_EX_CALLBACK,*PMINIDUMP_THREAD_EX_CALLBACK;

  typedef struct _MINIDUMP_INCLUDE_THREAD_CALLBACK {
    ULONG ThreadId;
  } MINIDUMP_INCLUDE_THREAD_CALLBACK,*PMINIDUMP_INCLUDE_THREAD_CALLBACK;

  typedef enum _THREAD_WRITE_FLAGS {
    ThreadWriteThread = 0x0001,ThreadWriteStack = 0x0002,ThreadWriteContext = 0x0004,ThreadWriteBackingStore = 0x0008,
    ThreadWriteInstructionWindow = 0x0010,ThreadWriteThreadData = 0x0020
  } THREAD_WRITE_FLAGS;

  typedef struct _MINIDUMP_MODULE_CALLBACK {
    PWCHAR FullPath;
    ULONG64 BaseOfImage;
    ULONG SizeOfImage;
    ULONG CheckSum;
    ULONG TimeDateStamp;
    VS_FIXEDFILEINFO VersionInfo;
    PVOID CvRecord;
    ULONG SizeOfCvRecord;
    PVOID MiscRecord;
    ULONG SizeOfMiscRecord;
  } MINIDUMP_MODULE_CALLBACK,*PMINIDUMP_MODULE_CALLBACK;

  typedef struct _MINIDUMP_INCLUDE_MODULE_CALLBACK {
    ULONG64 BaseOfImage;
  } MINIDUMP_INCLUDE_MODULE_CALLBACK,*PMINIDUMP_INCLUDE_MODULE_CALLBACK;

  typedef enum _MODULE_WRITE_FLAGS {
    ModuleWriteModule = 0x0001,ModuleWriteDataSeg = 0x0002,ModuleWriteMiscRecord = 0x0004,ModuleWriteCvRecord = 0x0008,
    ModuleReferencedByMemory = 0x0010
  } MODULE_WRITE_FLAGS;

  typedef struct _MINIDUMP_CALLBACK_INPUT {
    ULONG ProcessId;
    HANDLE ProcessHandle;
    ULONG CallbackType;
    union {
      MINIDUMP_THREAD_CALLBACK Thread;
      MINIDUMP_THREAD_EX_CALLBACK ThreadEx;
      MINIDUMP_MODULE_CALLBACK Module;
      MINIDUMP_INCLUDE_THREAD_CALLBACK IncludeThread;
      MINIDUMP_INCLUDE_MODULE_CALLBACK IncludeModule;
    } DUMMYUNIONNAME;
  } MINIDUMP_CALLBACK_INPUT,*PMINIDUMP_CALLBACK_INPUT;

  typedef struct _MINIDUMP_CALLBACK_OUTPUT {
    union {
      ULONG ModuleWriteFlags;
      ULONG ThreadWriteFlags;
      struct {
	ULONG64 MemoryBase;
	ULONG MemorySize;
      } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
  } MINIDUMP_CALLBACK_OUTPUT,*PMINIDUMP_CALLBACK_OUTPUT;

  typedef enum _MINIDUMP_TYPE {
    MiniDumpNormal = 0x0000,MiniDumpWithDataSegs = 0x0001,MiniDumpWithFullMemory = 0x0002,MiniDumpWithHandleData = 0x0004,
    MiniDumpFilterMemory = 0x0008,MiniDumpScanMemory = 0x0010,MiniDumpWithUnloadedModules = 0x0020,MiniDumpWithIndirectlyReferencedMemory = 0x0040,
    MiniDumpFilterModulePaths = 0x0080,MiniDumpWithProcessThreadData = 0x0100,MiniDumpWithPrivateReadWriteMemory = 0x0200,
    MiniDumpWithoutOptionalData = 0x0400
  } MINIDUMP_TYPE;

  typedef BOOL (WINAPI *MINIDUMP_CALLBACK_ROUTINE)(PVOID CallbackParam,CONST PMINIDUMP_CALLBACK_INPUT CallbackInput,PMINIDUMP_CALLBACK_OUTPUT CallbackOutput);

  typedef struct _MINIDUMP_CALLBACK_INFORMATION {
    MINIDUMP_CALLBACK_ROUTINE CallbackRoutine;
    PVOID CallbackParam;
  } MINIDUMP_CALLBACK_INFORMATION,*PMINIDUMP_CALLBACK_INFORMATION;

#define RVA_TO_ADDR(Mapping,Rva) ((PVOID)(((ULONG_PTR) (Mapping)) + (Rva)))

  BOOL WINAPI MiniDumpWriteDump(HANDLE hProcess,DWORD ProcessId,HANDLE hFile,MINIDUMP_TYPE DumpType,CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);
  BOOL WINAPI MiniDumpReadDumpStream(PVOID BaseOfDump,ULONG StreamNumber,PMINIDUMP_DIRECTORY *Dir,PVOID *StreamPointer,ULONG *StreamSize);

#include <poppack.h>

#ifdef __cplusplus
}
#endif
#endif
