/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    infload.h

Abstract:

    Private header file for internal inf routines.

Author:

    Ted Miller (tedm) 19-Jan-1995

Revision History:

    Gabe Schaffer (t-gabes) 19-Jul-1998
        Added LogContext to LOADED_INF
--*/


//
// Define maximum string sizes allowed in INFs.
//
#define MAX_STRING_LENGTH 511 // this is the maximum size of an unsubstituted string
#define MAX_SECT_NAME_LEN 255
#if MAX_SECT_NAME_LEN > MAX_STRING_LENGTH
#error MAX_SECT_NAME_LEN is too large!
#endif

#define MAX_LOGCONFKEYSTR_LEN       15

#include "pshpack1.h"

//
// Make absolutely sure that these structures are DWORD aligned
// because we turn alignment off, to make sure sdtructures are
// packed as tightly as possible into memory blocks.
//

//
// Internal representation of a section in an inf file
//
typedef struct _INF_LINE {

    //
    // Number of values on the line
    // This includes the key if Flags has INF_LINE_HASKEY
    // (In that case the first two entries in the Values array
    // contain the key--the first one in case-insensitive form used
    // for lookup, and the second in case-sensitive form for display.
    // INF lines with a single value (no key) are treated the same way.)
    // Otherwise the first entry in the Values array is the first
    // value on the line
    //
    WORD ValueCount;
    WORD Flags;

    //
    // String IDs for the values on the line.
    // The values are stored in the value block,
    // one after another.
    //
    // The value is the offset within the value block as opposed to
    // an actual pointer. We do this because the value block gets
    // reallocated as the inf file is loaded.
    //
    UINT Values;

} INF_LINE, *PINF_LINE;

//
// INF_LINE.Flags
//
#define INF_LINE_HASKEY     0x0000001
#define INF_LINE_SEARCHABLE 0x0000002

#define HASKEY(Line)       ((Line)->Flags & INF_LINE_HASKEY)
#define ISSEARCHABLE(Line) ((Line)->Flags & INF_LINE_SEARCHABLE)

//
// INF section
// This guy is kept separate and has a pointer to the actual data
// to make sorting the sections a little easier
//
typedef struct _INF_SECTION {
    //
    // String Table ID of the name of the section
    //
    LONG  SectionName;

    //
    // Number of lines in this section
    //
    DWORD LineCount;

    //
    // The section's lines. The line structures are stored packed
    // in the line block, one after another.
    //
    // The value is the offset within the line block as opposed to
    // an actual pointer. We do it this way because the line block
    // gets reallocated as the inf file is loaded.
    //
    UINT Lines;

} INF_SECTION, *PINF_SECTION;

//
// Params for section enumeration
//

typedef struct {
    PTSTR       Buffer;
    UINT        Size;
    UINT        SizeNeeded;
    PTSTR       End;
} SECTION_ENUM_PARAMS, *PSECTION_ENUM_PARAMS;


#include "poppack.h"

//
// Define structures for user-defined DIRID storage.
//
typedef struct _USERDIRID {
    UINT Id;
    TCHAR Directory[MAX_PATH];
} USERDIRID, *PUSERDIRID;

typedef struct _USERDIRID_LIST {
    PUSERDIRID UserDirIds;  // may be NULL
    UINT UserDirIdCount;
} USERDIRID_LIST, *PUSERDIRID_LIST;

typedef struct _STRINGSUBST_NODE {
    UINT ValueOffset;
    LONG TemplateStringId;
    BOOL CaseSensitive;
} STRINGSUBST_NODE, *PSTRINGSUBST_NODE;

//
// Any system DIRID (i.e., >0x8000) that has bit 0x4000 set is a 'volatile'
// DIRID (these DIRIDs are volatile in the sense that, while they're not in the 
// user-definable range, they're treated as if they were, and string replacement 
// is done on-the-fly each time the PNF is loaded.  The shell special folders
// (CSIDL_* defines in sdk\inc\shlobj.h), for example, are in this range.  In 
// the case of shell special folders, the actual CSIDL value (i.e., as is
// passed into SHGetSpecialFolderPath) can be obtained by simply masking out
// the volatile DIRID bit.
//
// Define the bitmask used to determine whether a system DIRID is volatile.
//
#define VOLATILE_DIRID_FLAG 0x4000

//
// Version block structure that is stored (packed) in the opaque
// VersionData buffer of a caller-supplied SP_INF_INFORMATION structure.
//
typedef struct _INF_VERSION_BLOCK {
    UINT NextOffset;
    FILETIME LastWriteTime;
    WORD DatumCount;
    WORD OffsetToData; // offset (in bytes) from beginning of Filename buffer.
    UINT DataSize;     // DataSize and TotalSize are both byte counts.
    UINT TotalSize;
    TCHAR Filename[ANYSIZE_ARRAY];
    //
    // Data follows Filename in the buffer
    //
} INF_VERSION_BLOCK, *PINF_VERSION_BLOCK;

//
// Internal version block node.
//
typedef struct _INF_VERSION_NODE {
    FILETIME LastWriteTime;
    UINT FilenameSize;
    CONST TCHAR *DataBlock;
    UINT DataSize;
    WORD DatumCount;
    TCHAR Filename[MAX_PATH];
} INF_VERSION_NODE, *PINF_VERSION_NODE;

//
// Internal representation of an inf file.
//
typedef struct _LOADED_INF {
    DWORD Signature;

    //
    // The following 3 fields are used for precompiled INFs (PNF).
    // If FileHandle is not INVALID_HANDLE_VALUE, then this is a PNF,
    // and the MappingHandle and ViewAddress fields are also valid.
    // Otherwise, this is a plain old in-memory INF.
    //
    HANDLE FileHandle;
    HANDLE MappingHandle;
    PVOID  ViewAddress;

    PVOID StringTable;
    DWORD SectionCount;
    PINF_SECTION SectionBlock;
    PINF_LINE LineBlock;
    PLONG ValueBlock;
    INF_VERSION_NODE VersionBlock;
    BOOL HasStrings;

    //
    // If this INF contains any DIRID references to the system partition, then
    // store the OsLoader path that was used when compiling this INF here.  (This
    // value will always be correct when the INF is loaded.  However, if drive letters
    // are subsequently reassigned, then it will be incorrect until the INF is unloaded
    // and re-loaded.)
    //
    PCTSTR OsLoaderPath;    // may be NULL

    //
    // Remember the location where this INF originally came from (may be a directory
    // path or a URL).
    //
    DWORD  InfSourceMediaType;  // SPOST_PATH or SPOST_URL
    PCTSTR InfSourcePath;       // may be NULL

    //
    // Remember the INF's original filename, before it was installed into
    // %windir%\Inf (i.e., automatically via device installation or explicitly 
    // via SetupCopyOEMInf).
    //
    PCTSTR OriginalInfName;     // may be NULL

    //
    // Maintain a list of value offsets that require string substitution at
    // run-time.
    //
    PSTRINGSUBST_NODE SubstValueList;   // may be NULL
    WORD SubstValueCount;

    //
    // Place the style WORD here (immediately following another WORD field),
    // to fill a single DWORD.
    //
    WORD Style;                         // INF_STYLE_OLDNT, INF_STYLE_WIN4

    //
    // Sizes in bytes of various buffers
    //
    UINT SectionBlockSizeBytes;
    UINT LineBlockSizeBytes;
    UINT ValueBlockSizeBytes;

    //
    // Track what language was used when loading this INF.
    //
    DWORD LanguageId;

    //
    // Embedded structure containing information about the current user-defined
    // DIRID values.
    //
    USERDIRID_LIST UserDirIdList;

    //
    // Synchronization.
    //
    MYLOCK Lock;

    //
    // Log context for error logging
    //
    PSETUP_LOG_CONTEXT LogContext;

    //
    // Other flags
    //
    DWORD Flags;

    //
    // INFs are append-loaded via a doubly-linked list of LOADED_INFs.
    // (list is not circular--Prev of head is NULL, Next of tail is NULL)
    //
    struct _LOADED_INF *Prev;
    struct _LOADED_INF *Next;

} LOADED_INF, *PLOADED_INF;

#define LOADED_INF_SIG   0x24666e49      // Inf$

#define LockInf(Inf)    BeginSynchronizedAccess(&(Inf)->Lock)
#define UnlockInf(Inf)  EndSynchronizedAccess(&(Inf)->Lock)

//
// Define values for LOADED_INF.Flags field
//
//
// WARNING: The LIF_INF_DIGITALLY_SIGNED flag does not guarantee that the INF
// is currently digitally signed. When creating the PNF we verify that the INF
// is correctly digitally signed and then set this bit in the PNF. Currently we
// only use this flag to determine whether we should use the DriverVer date
// or not.
//
#define LIF_HAS_VOLATILE_DIRIDS    (0x00000001)
#define LIF_INF_DIGITALLY_SIGNED   (0x00000002)


//
// Helper define
//
#define INF_STYLE_ALL   (INF_STYLE_WIN4 | INF_STYLE_OLDNT)


//
// Define file header structure for precompiled INF (.PNF).
//
typedef struct _PNF_HEADER {

    WORD  Version;  // HiByte - Major Ver#, LoByte - Minor Ver#
    WORD  InfStyle;
    DWORD Flags;

    DWORD    InfSubstValueListOffset;
    WORD     InfSubstValueCount;

    WORD     InfVersionDatumCount;
    DWORD    InfVersionDataSize;
    DWORD    InfVersionDataOffset;
    FILETIME InfVersionLastWriteTime;

    DWORD StringTableBlockOffset;
    DWORD StringTableBlockSize;

    DWORD InfSectionCount;
    DWORD InfSectionBlockOffset;
    DWORD InfSectionBlockSize;
    DWORD InfLineBlockOffset;
    DWORD InfLineBlockSize;
    DWORD InfValueBlockOffset;
    DWORD InfValueBlockSize;

    DWORD WinDirPathOffset;
    DWORD OsLoaderPathOffset;

    WORD StringTableHashBucketCount;

    WORD LanguageId;

    DWORD InfSourcePathOffset;      // may be 0

    DWORD OriginalInfNameOffset;    // may be 0

} PNF_HEADER, *PPNF_HEADER;

//
// Define Major and Minor versions of the PNF format (currently 1.1)
//
#define PNF_MAJOR_VERSION (0x01)
#define PNF_MINOR_VERSION (0x01)

//
// Define flag values for the PNF header's Flags field.
//
// WARNING: The PNF_FLAG_INF_DIGITALLY_SIGNED flag does not guarantee that the INF
// is currently digitally signed. When creating the PNF we verify that the INF
// is correctly digitally signed and then set this bit in the PNF. Currently we
// only use this flag to determine whether we should use the DriverVer date
// or not.

#define PNF_FLAG_IS_UNICODE             (0x00000001)
#define PNF_FLAG_HAS_STRINGS            (0x00000002)
#define PNF_FLAG_SRCPATH_IS_URL         (0x00000004)
#define PNF_FLAG_HAS_VOLATILE_DIRIDS    (0x00000008)
#define PNF_FLAG_INF_VERIFIED           (0x00000010)
#define PNF_FLAG_INF_DIGITALLY_SIGNED   (0x00000020)

//
// Define our string table hash bucket count here, since this number is stored
// in the PNF header and validated against at load time.
//
#define HASH_BUCKET_COUNT 509


//
// Public inf functions in infload.c. All other routines are private to
// the inf handler package.
//
DWORD
DetermineInfStyle(
    IN PCTSTR            Filename,
    IN LPWIN32_FIND_DATA FindData
    );

//
// Flags for LoadInfFile.
//
#define LDINF_FLAG_MATCH_CLASS_GUID        (0x00000001)
#define LDINF_FLAG_ALWAYS_TRY_PNF          (0x00000002)
#define LDINF_FLAG_IGNORE_VOLATILE_DIRIDS  (0x00000004) // includes system partition
#define LDINF_FLAG_IGNORE_LANGUAGE         (0x00000008)
#define LDINF_FLAG_REGENERATE_PNF          (0x00000010)
#define LDINF_FLAG_SRCPATH_IS_URL          (0x00000020)
#define LDINF_FLAG_ALWAYS_GET_SRCPATH      (0x00000040) // used to work around TZ change in FAT

DWORD
LoadInfFile(
    IN  PCTSTR            Filename,
    IN  LPWIN32_FIND_DATA FileData,
    IN  DWORD             Style,
    IN  DWORD             Flags,
    IN  PCTSTR            ClassGuidString, OPTIONAL
    IN  PCTSTR            InfSourcePath,   OPTIONAL
    IN  PCTSTR            OriginalInfName, OPTIONAL
    IN  PLOADED_INF       AppendInf,       OPTIONAL
    IN  PSETUP_LOG_CONTEXT LogContext,     OPTIONAL
    OUT PLOADED_INF      *LoadedInf,
    OUT UINT             *ErrorLineNumber,
    OUT BOOL             *PnfWasUsed       OPTIONAL
    );

VOID
FreeInfFile(
    IN PLOADED_INF LoadedInf
    );


//
// Global strings used throughout the inf loaders/runtime stuff.  Sizes are
// included so that we can do sizeof() instead of lstrlen() to determine string
// length.
//
// The content of the following strings is defined in infstr.h:
//
extern CONST TCHAR pszSignature[SIZECHARS(INFSTR_KEY_SIGNATURE)],
                   pszVersion[SIZECHARS(INFSTR_SECT_VERSION)],
                   pszClass[SIZECHARS(INFSTR_KEY_HARDWARE_CLASS)],
                   pszClassGuid[SIZECHARS(INFSTR_KEY_HARDWARE_CLASSGUID)],
                   pszProvider[SIZECHARS(INFSTR_KEY_PROVIDER)],
                   pszStrings[SIZECHARS(SZ_KEY_STRINGS)],
                   pszLayoutFile[SIZECHARS(SZ_KEY_LAYOUT_FILE)],
                   pszManufacturer[SIZECHARS(INFSTR_SECT_MFG)],
                   pszControlFlags[SIZECHARS(INFSTR_CONTROLFLAGS_SECTION)],
                   pszSourceDisksNames[SIZECHARS(SZ_KEY_SRCDISKNAMES)],
                   pszSourceDisksFiles[SIZECHARS(SZ_KEY_SRCDISKFILES)],
                   pszDestinationDirs[SIZECHARS(SZ_KEY_DESTDIRS)],
                   pszDefaultDestDir[SIZECHARS(SZ_KEY_DEFDESTDIR)],
                   pszReboot[SIZECHARS(INFSTR_REBOOT)],
                   pszRestart[SIZECHARS(INFSTR_RESTART)],
                   pszClassInstall32[SIZECHARS(INFSTR_SECT_CLASS_INSTALL_32)],
                   pszAddInterface[SIZECHARS(SZ_KEY_ADDINTERFACE)],
                   pszInterfaceInstall32[SIZECHARS(INFSTR_SECT_INTERFACE_INSTALL_32)],
                   pszAddService[SIZECHARS(SZ_KEY_ADDSERVICE)],
                   pszDelService[SIZECHARS(SZ_KEY_DELSERVICE)],
                   pszCatalogFile[SIZECHARS(INFSTR_KEY_CATALOGFILE)],
                   pszMemConfig[SIZECHARS(INFSTR_KEY_MEMCONFIG)],
                   pszIOConfig[SIZECHARS(INFSTR_KEY_IOCONFIG)],
                   pszIRQConfig[SIZECHARS(INFSTR_KEY_IRQCONFIG)],
                   pszDMAConfig[SIZECHARS(INFSTR_KEY_DMACONFIG)],
                   pszPcCardConfig[SIZECHARS(INFSTR_KEY_PCCARDCONFIG)],
                   pszMfCardConfig[SIZECHARS(INFSTR_KEY_MFCARDCONFIG)],
                   pszConfigPriority[SIZECHARS(INFSTR_KEY_CONFIGPRIORITY)],
                   pszDriverVer[SIZECHARS(INFSTR_DRIVERVERSION_SECTION)];

//
// Other misc. global strings:
//
#define DISTR_INF_DRVDESCFMT               (TEXT("%s.") INFSTR_STRKEY_DRVDESC)
#define DISTR_INF_HWSECTIONFMT             (TEXT("%s.") INFSTR_SUBKEY_HW)
#define DISTR_INF_CHICAGOSIG               (TEXT("$Chicago$"))
#define DISTR_INF_WINNTSIG                 (TEXT("$Windows NT$"))
#define DISTR_INF_WIN95SIG                 (TEXT("$Windows 95$"))
#define DISTR_INF_WIN_SUFFIX               (TEXT(".") INFSTR_PLATFORM_WIN)
#define DISTR_INF_NT_SUFFIX                (TEXT(".") INFSTR_PLATFORM_NT)
#define DISTR_INF_PNF_SUFFIX               (TEXT(".PNF"))
#define DISTR_INF_INF_SUFFIX               (TEXT(".INF"))
#define DISTR_INF_CAT_SUFFIX               (TEXT(".CAT"))
#define DISTR_INF_SERVICES_SUFFIX          (TEXT(".") INFSTR_SUBKEY_SERVICES)
#define DISTR_INF_INTERFACES_SUFFIX        (TEXT(".") INFSTR_SUBKEY_INTERFACES)
#define DISTR_INF_COINSTALLERS_SUFFIX      (TEXT(".") INFSTR_SUBKEY_COINSTALLERS)
#define DISTR_INF_LOGCONFIGOVERRIDE_SUFFIX (TEXT(".") INFSTR_SUBKEY_LOGCONFIGOVERRIDE)
//
// Define all platform-specific suffix strings for which we support non-native
// digital signature verification...
//
#define DISTR_INF_NTALPHA_SUFFIX           (TEXT(".") INFSTR_PLATFORM_NTALPHA)
#define DISTR_INF_NTX86_SUFFIX             (TEXT(".") INFSTR_PLATFORM_NTX86)
#define DISTR_INF_NTIA64_SUFFIX            (TEXT(".") INFSTR_PLATFORM_NTIA64)
#define DISTR_INF_NTAXP64_SUFFIX           (TEXT(".") INFSTR_PLATFORM_NTAXP64)

//
// Define platform suffix string based upon architecture being compiled for.
//
#if defined(_ALPHA_)

#define DISTR_INF_NTPLATFORM_SUFFIX (TEXT(".") INFSTR_PLATFORM_NTALPHA)

#elif defined(_MIPS_)

#define DISTR_INF_NTPLATFORM_SUFFIX (TEXT(".") INFSTR_PLATFORM_NTMIPS)

#elif defined(_PPC_)

#define DISTR_INF_NTPLATFORM_SUFFIX (TEXT(".") INFSTR_PLATFORM_NTPPC)

#elif defined(_X86_)

#define DISTR_INF_NTPLATFORM_SUFFIX (TEXT(".") INFSTR_PLATFORM_NTX86)

#elif defined(_IA64_)

#define DISTR_INF_NTPLATFORM_SUFFIX (TEXT(".") INFSTR_PLATFORM_NTIA64)

#elif defined(_AXP64_)

#define DISTR_INF_NTPLATFORM_SUFFIX (TEXT(".") INFSTR_PLATFORM_NTAXP64)

#else

#error Unknown processor type

#endif


//
// (Sizes are included for all strings that we define privately.  This
// is done so that we can do sizeof() instead of lstrlen() to determine
// string length.  Keep in sync with definitions in infload.c!)
//
extern CONST TCHAR pszDrvDescFormat[SIZECHARS(DISTR_INF_DRVDESCFMT)],
                   pszHwSectionFormat[SIZECHARS(DISTR_INF_HWSECTIONFMT)],
                   pszChicagoSig[SIZECHARS(DISTR_INF_CHICAGOSIG)],
                   pszWindowsNTSig[SIZECHARS(DISTR_INF_WINNTSIG)],
                   pszWindows95Sig[SIZECHARS(DISTR_INF_WIN95SIG)],
                   pszWinSuffix[SIZECHARS(DISTR_INF_WIN_SUFFIX)],
                   pszNtSuffix[SIZECHARS(DISTR_INF_NT_SUFFIX)],
                   pszNtAlphaSuffix[SIZECHARS(DISTR_INF_NTALPHA_SUFFIX)],
                   pszNtX86Suffix[SIZECHARS(DISTR_INF_NTX86_SUFFIX)],
                   pszNtIA64Suffix[SIZECHARS(DISTR_INF_NTIA64_SUFFIX)],
                   pszNtAXP64Suffix[SIZECHARS(DISTR_INF_NTAXP64_SUFFIX)],
                   pszNtPlatformSuffix[SIZECHARS(DISTR_INF_NTPLATFORM_SUFFIX)],
                   pszPnfSuffix[SIZECHARS(DISTR_INF_PNF_SUFFIX)],
                   pszInfSuffix[SIZECHARS(DISTR_INF_INF_SUFFIX)],
                   pszCatSuffix[SIZECHARS(DISTR_INF_CAT_SUFFIX)],
                   pszServicesSectionSuffix[SIZECHARS(DISTR_INF_SERVICES_SUFFIX)],
                   pszInterfacesSectionSuffix[SIZECHARS(DISTR_INF_INTERFACES_SUFFIX)],
                   pszCoInstallersSectionSuffix[SIZECHARS(DISTR_INF_COINSTALLERS_SUFFIX)],
                   pszLogConfigOverrideSectionSuffix[SIZECHARS(DISTR_INF_LOGCONFIGOVERRIDE_SUFFIX)];


//
// Define a (non-CONST) array of strings that specifies what lines to look for
// in an INF's [ControlFlags] section when determining whether a particular device
// ID should be excluded.  This is filled in during process attach for speed
// reasons.
//
// The max string length (including NULL) is 32, and there can be a maximum of 3
// such strings.  E.g.: ExcludeFromSelect, ExcludeFromSelect.NT, ExcludeFromSelect.NTAlpha
//
extern TCHAR pszExcludeFromSelectList[3][32];
extern DWORD ExcludeFromSelectListUb;  // contains the number of strings in the above list (2 or 3).


//
// Routine to determine whether a character is whitespace.
//
BOOL
IsWhitespace(
    IN PCTSTR pc
    );

//
// Routine to skip whitespace (but not newlines)
//
VOID
SkipWhitespace(
    IN OUT PCTSTR *Location,
    IN     PCTSTR  BufferEnd
    );

PINF_SECTION
InfLocateSection(
    IN  PLOADED_INF Inf,
    IN  PCTSTR      SectionName,
    OUT PUINT       SectionNumber   OPTIONAL
    );

BOOL
InfLocateLine(
    IN     PLOADED_INF   Inf,
    IN     PINF_SECTION  Section,
    IN     PCTSTR        Key,        OPTIONAL
    IN OUT PUINT         LineNumber,
    OUT    PINF_LINE    *Line
    );

PTSTR
InfGetKeyOrValue(
    IN  PLOADED_INF Inf,
    IN  PCTSTR      SectionName,
    IN  PCTSTR      LineKey,     OPTIONAL
    IN  UINT        LineNumber,  OPTIONAL
    IN  UINT        ValueNumber,
    OUT PLONG       StringId     OPTIONAL
    );

PTSTR
InfGetField(
    IN  PLOADED_INF Inf,
    IN  PINF_LINE   InfLine,
    IN  UINT        ValueNumber,
    OUT PLONG       StringId     OPTIONAL
    );

PINF_LINE
InfLineFromContext(
    IN PINFCONTEXT Context
    );


//
// Define a macro to retrieve the case-insensitive (i.e., searchable) string ID
// for an INF line's key, or -1 if there is no key.
// NOTE: INF lock must have been acquired before calling this macro!
//
// LONG
// pInfGetLineKeyId(
//     IN  PLOADED_INF Inf,
//     IN  PINF_LINE   InfLine
//     )
//
#define pInfGetLineKeyId(Inf,InfLine)  (ISSEARCHABLE(InfLine) ? (Inf)->ValueBlock[(InfLine)->Values] : -1)

//
// Routine to allocate and initialize a loaded inf descriptor.
//
PLOADED_INF
AllocateLoadedInfDescriptor(
    IN DWORD SectionBlockSize,
    IN DWORD LineBlockSize,
    IN DWORD ValueBlockSize,
    IN  PSETUP_LOG_CONTEXT LogContext OPTIONAL
    );

VOID
FreeInfOrPnfStructures(
    IN PLOADED_INF Inf
    );

//
// Define a macro to free all memory blocks associated with a loaded INF or PNF,
// and then free the memory for the loaded INF structure itself
//
// VOID
// FreeLoadedInfDescriptor(
//     IN PLOADED_INF Inf
//     );
//
#define FreeLoadedInfDescriptor(Inf) {  \
    FreeInfOrPnfStructures(Inf);        \
    MyFree(Inf);                        \
}

BOOL
AddDatumToVersionBlock(
    IN OUT PINF_VERSION_NODE VersionNode,
    IN     PCTSTR            DatumName,
    IN     PCTSTR            DatumValue
    );

//
// Old inf manipulation routines, called by new inf loader
//
DWORD
ParseOldInf(
    IN  PCTSTR       FileImage,
    IN  DWORD        FileImageSize,
    IN  PSETUP_LOG_CONTEXT LogContext, OPTIONAL
    OUT PLOADED_INF *Inf,
    OUT UINT        *ErrorLineNumber
    );

DWORD
ProcessOldInfVersionBlock(
    IN PLOADED_INF Inf
    );

//
// Run-time helper routines.
//
PCTSTR
pSetupFilenameFromLine(
    IN PINFCONTEXT Context,
    IN BOOL        GetSourceName
    );


//
// Logical configuration stuff, inflogcf.c
//
DWORD
pSetupInstallLogConfig(
    IN HINF    Inf,
    IN PCTSTR  SectionName,
    IN DEVINST DevInst,
    IN DWORD   Flags,
    IN HMACHINE hMachine
    );

//
// INF Version information retrieval
//
PCTSTR
pSetupGetVersionDatum(
    IN PINF_VERSION_NODE VersionNode,
    IN PCTSTR            DatumName
    );

BOOL
pSetupGetCatalogFileValue(
    IN  PINF_VERSION_NODE    InfVersionNode,
    OUT LPTSTR               Buffer,
    IN  DWORD                BufferSize,
    IN  PSP_ALTPLATFORM_INFO AltPlatformInfo OPTIONAL
    );

VOID
pSetupGetPhysicalInfFilepath(
    IN  PINFCONTEXT LineContext,
    OUT LPTSTR      Buffer,
    IN  DWORD       BufferSize
    );

//
// Private installation routines.
//

//
// Private Flags & context for _SetupInstallFromInfSection.
// passed onto pSetupInstallRegistry
//

typedef struct _REGMOD_CONTEXT {
    DWORD               Flags;          // indicates what fields are filled in
    HKEY                UserRootKey;    // HKR
    LPGUID              ClassGuid;      // INF_PFLAG_CLASSPROP
    HMACHINE            hMachine;       // INF_PFLAG_CLASSPROP
    DWORD               DevInst;        // INF_PFLAG_DEVPROP
} REGMOD_CONTEXT, *PREGMOD_CONTEXT;

#define INF_PFLAG_CLASSPROP        (0x00000001)  // set if called for a ClassInstall32 section
#define INF_PFLAG_DEVPROP          (0x00000002)  // set if called for registry properties
#define INF_PFLAG_HKR              (0x00000004)  // indicates override _SetupInstallFromInfSection RelativeKeyRoot

BOOL
_SetupInstallFromInfSection(
    IN HWND             Owner,              OPTIONAL
    IN HINF             InfHandle,
    IN PCTSTR           SectionName,
    IN UINT             Flags,
    IN HKEY             RelativeKeyRoot,    OPTIONAL
    IN PCTSTR           SourceRootPath,     OPTIONAL
    IN UINT             CopyFlags,
    IN PVOID            MsgHandler,
    IN PVOID            Context,            OPTIONAL
    IN HDEVINFO         DeviceInfoSet,      OPTIONAL
    IN PSP_DEVINFO_DATA DeviceInfoData,     OPTIONAL
    IN BOOL             IsMsgHandlerNativeCharWidth,
    IN PREGMOD_CONTEXT  RegContext          OPTIONAL
    );

DWORD
pSetupInstallFiles(
    IN HINF              Inf,
    IN HINF              LayoutInf,         OPTIONAL
    IN PCTSTR            SectionName,
    IN PCTSTR            SourceRootPath,    OPTIONAL
    IN PSP_FILE_CALLBACK MsgHandler,        OPTIONAL
    IN PVOID             Context,           OPTIONAL
    IN UINT              CopyStyle,
    IN HWND              Owner,             OPTIONAL
    IN HSPFILEQ          UserFileQ,         OPTIONAL
    IN BOOL              IsMsgHandlerNativeCharWidth
    );

DWORD
pSetupInstallRegistry(
    IN HINF             Inf,
    IN PCTSTR           SectionName,
    IN PREGMOD_CONTEXT  RegContext          OPTIONAL
    );

DWORD
_AppendStringToMultiSz(
    IN PCTSTR           SubKeyName,         OPTIONAL
    IN PCTSTR           ValueName,          OPTIONAL
    IN PCTSTR           String,
    IN BOOL             AllowDuplicates,
    IN PREGMOD_CONTEXT  RegContext          OPTIONAL
    );


