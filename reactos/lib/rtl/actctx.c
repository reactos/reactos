/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Runtime Library
 * PURPOSE:         Activation Context Support
 * FILE:            lib/rtl/actctx.c
 * PROGRAMERS:
 *                  Jon Griffiths
 *                  Eric Pouech
 *                  Jacek Caban for CodeWeavers
 *                  Alexandre Julliard
 *                  Stefan Ginsberg (stefan__100__@hotmail.com)
 *                  Samuel Serapión
 */

/* Based on Wine Staging 1.7.37 */

#include <rtl.h>

#define NDEBUG
#include <debug.h>

#include <wine/unicode.h>

BOOLEAN RtlpNotAllowingMultipleActivation;

#define ACTCTX_FLAGS_ALL (\
 ACTCTX_FLAG_PROCESSOR_ARCHITECTURE_VALID |\
 ACTCTX_FLAG_LANGID_VALID |\
 ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID |\
 ACTCTX_FLAG_RESOURCE_NAME_VALID |\
 ACTCTX_FLAG_SET_PROCESS_DEFAULT |\
 ACTCTX_FLAG_APPLICATION_NAME_VALID |\
 ACTCTX_FLAG_SOURCE_IS_ASSEMBLYREF |\
 ACTCTX_FLAG_HMODULE_VALID )

#define STRSECTION_MAGIC   0x64487353 /* dHsS */
#define GUIDSECTION_MAGIC  0x64487347 /* dHsG */

#define ACTCTX_MAGIC_MARKER (PVOID)'gMcA'

#define ACTCTX_FAKE_HANDLE ((HANDLE) 0xf00baa)
#define ACTCTX_FAKE_COOKIE ((ULONG_PTR) 0xf00bad)



typedef struct
{
    const WCHAR        *ptr;
    unsigned int        len;
} xmlstr_t;

typedef struct
{
    const WCHAR        *ptr;
    const WCHAR        *end;
} xmlbuf_t;

struct file_info
{
    ULONG               type;
    WCHAR              *info;
};

struct assembly_version
{
    USHORT              major;
    USHORT              minor;
    USHORT              build;
    USHORT              revision;
};

struct assembly_identity
{
    WCHAR                *name;
    WCHAR                *arch;
    WCHAR                *public_key;
    WCHAR                *language;
    WCHAR                *type;
    struct assembly_version version;
    BOOL                  optional;
    BOOL                  delayed;
};

struct strsection_header
{
    DWORD magic;
    ULONG size;
    DWORD unk1[3];
    ULONG count;
    ULONG index_offset;
    DWORD unk2[2];
    ULONG global_offset;
    ULONG global_len;
};

struct string_index
{
    ULONG hash;        /* key string hash */
    ULONG name_offset;
    ULONG name_len;
    ULONG data_offset; /* redirect data offset */
    ULONG data_len;
    ULONG rosterindex;
};

struct guidsection_header
{
    DWORD magic;
    ULONG size;
    DWORD unk[3];
    ULONG count;
    ULONG index_offset;
    DWORD unk2;
    ULONG names_offset;
    ULONG names_len;
};

struct guid_index
{
    GUID  guid;
    ULONG data_offset;
    ULONG data_len;
    ULONG rosterindex;
};

struct wndclass_redirect_data
{
    ULONG size;
    DWORD res;
    ULONG name_len;
    ULONG name_offset;  /* versioned name offset */
    ULONG module_len;
    ULONG module_offset;/* container name offset */
};

struct dllredirect_data
{
    ULONG size;
    ULONG unk;
    DWORD res[3];
};

struct tlibredirect_data
{
    ULONG  size;
    DWORD  res;
    ULONG  name_len;
    ULONG  name_offset;
    LANGID langid;
    WORD   flags;
    ULONG  help_len;
    ULONG  help_offset;
    WORD   major_version;
    WORD   minor_version;
};

enum comclass_threadingmodel
{
    ThreadingModel_Apartment = 1,
    ThreadingModel_Free      = 2,
    ThreadingModel_No        = 3,
    ThreadingModel_Both      = 4,
    ThreadingModel_Neutral   = 5
};

enum comclass_miscfields
{
    MiscStatus          = 1,
    MiscStatusIcon      = 2,
    MiscStatusContent   = 4,
    MiscStatusThumbnail = 8,
    MiscStatusDocPrint  = 16
};

struct comclassredirect_data
{
    ULONG size;
    BYTE  res;
    BYTE  miscmask;
    BYTE  res1[2];
    DWORD model;
    GUID  clsid;
    GUID  alias;
    GUID  clsid2;
    GUID  tlbid;
    ULONG name_len;
    ULONG name_offset;
    ULONG progid_len;
    ULONG progid_offset;
    ULONG clrdata_len;
    ULONG clrdata_offset;
    DWORD miscstatus;
    DWORD miscstatuscontent;
    DWORD miscstatusthumbnail;
    DWORD miscstatusicon;
    DWORD miscstatusdocprint;
};

enum ifaceps_mask
{
    NumMethods = 1,
    BaseIface  = 2
};

struct ifacepsredirect_data
{
    ULONG size;
    DWORD mask;
    GUID  iid;
    ULONG nummethods;
    GUID  tlbid;
    GUID  base;
    ULONG name_len;
    ULONG name_offset;
};

struct clrsurrogate_data
{
    ULONG size;
    DWORD res;
    GUID  clsid;
    ULONG version_offset;
    ULONG version_len;
    ULONG name_offset;
    ULONG name_len;
};

struct clrclass_data
{
    ULONG size;
    DWORD res[2];
    ULONG module_len;
    ULONG module_offset;
    ULONG name_len;
    ULONG name_offset;
    ULONG version_len;
    ULONG version_offset;
    DWORD res2[2];
};

struct progidredirect_data
{
    ULONG size;
    DWORD reserved;
    ULONG clsid_offset;
};

/*

   Sections structure.

   Sections are accessible by string or guid key, that defines two types of sections.
   All sections of each type have same magic value and header structure, index
   data could be of two possible types too. So every string based section uses
   the same index format, same applies to guid sections - they share same guid index
   format.

   - window class redirection section is a plain buffer with following format:

   <section header>
   <index[]>
   <data[]> --- <original name>
                <redirect data>
                <versioned name>
                <module name>

   Header is fixed length structure - struct strsection_header,
   contains redirected classes count;

   Index is an array of fixed length index records, each record is
   struct string_index.

   All strings in data itself are WCHAR, null terminated, 4-bytes aligned.

   Versioned name offset is relative to redirect data structure (struct wndclass_redirect_data),
   others are relative to section itself.

   - dll redirect section format:

   <section header>
   <index[]>
   <data[]> --- <dll name>
                <data>

   This section doesn't seem to carry any payload data except dll names.

   - typelib section format:

   <section header>
   <module names[]>
   <index[]>
   <data[]> --- <data>
                <helpstring>

   Header is fixed length, index is an array of fixed length 'struct guid_index'.
   All strings are WCHAR, null terminated, 4-bytes aligned. Module names part is
   4-bytes aligned as a whole.

   Module name offsets are relative to section, helpstring offset is relative to data
   structure itself.

   - comclass section format:

   <section header>
   <module names[]>
   <index[]>
   <data[]> --- <data>   --- <data>
                <progid>     <clrdata>
                             <name>
                             <version>
                             <progid>

   This section uses two index records per comclass, one entry contains original guid
   as specified by context, another one has a generated guid. Index and strings handling
   is similar to typelib sections.

   For CLR classes additional data is stored after main COM class data, it contains
   class name and runtime version string, see 'struct clrclass_data'.

   Module name offsets are relative to section, progid offset is relative to data
   structure itself.

   - COM interface section format:

   <section header>
   <index[]>
   <data[]> --- <data>
                <name>

   Interface section contains data for proxy/stubs and external proxy/stubs. External
   ones are defined at assembly level, so this section has no module information.
   All records are indexed with 'iid' value from manifest. There an exception for
   external variants - if 'proxyStubClsid32' is specified, it's stored as iid in
   redirect data, but index is still 'iid' from manifest.

   Interface name offset is relative to data structure itself.

   - CLR surrogates section format:

   <section header>
   <index[]>
   <data[]> --- <data>
                <name>
                <version>

    There's nothing special about this section, same way to store strings is used,
    no modules part as it belongs to assembly level, not a file.

   - ProgID section format:

   <section header>
   <guids[]>
   <index[]>
   <data[]> --- <progid>
                <data>

   This sections uses generated alias guids from COM server section. This way
   ProgID -> CLSID mapping returns generated guid, not the real one. ProgID string
   is stored too, aligned.
*/

struct progids
{
    WCHAR        **progids;
    unsigned int   num;
    unsigned int   allocated;
};

struct entity
{
    DWORD kind;
    union
    {
        struct
        {
            WCHAR *tlbid;
            WCHAR *helpdir;
            WORD   flags;
            WORD   major;
            WORD   minor;
	} typelib;
        struct
        {
            WCHAR *clsid;
            WCHAR *tlbid;
            WCHAR *progid;
            WCHAR *name;    /* clrClass: class name */
            WCHAR *version; /* clrClass: CLR runtime version */
            DWORD  model;
            DWORD  miscstatus;
            DWORD  miscstatuscontent;
            DWORD  miscstatusthumbnail;
            DWORD  miscstatusicon;
            DWORD  miscstatusdocprint;
            struct progids progids;
	} comclass;
	struct {
            WCHAR *iid;
            WCHAR *base;
            WCHAR *tlib;
            WCHAR *name;
            WCHAR *ps32; /* only stored for 'comInterfaceExternalProxyStub' */
            DWORD  mask;
            ULONG  nummethods;
	} ifaceps;
        struct
        {
            WCHAR *name;
            BOOL   versioned;
        } class;
        struct
        {
            WCHAR *name;
            WCHAR *clsid;
            WCHAR *version;
        } clrsurrogate;
    } u;
};

struct entity_array
{
    struct entity        *base;
    unsigned int          num;
    unsigned int          allocated;
};

struct dll_redirect
{
    WCHAR                *name;
    WCHAR                *hash;
    struct entity_array   entities;
};

enum assembly_type
{
    APPLICATION_MANIFEST,
    ASSEMBLY_MANIFEST,
    ASSEMBLY_SHARED_MANIFEST,
};

struct assembly
{
    enum assembly_type       type;
    struct assembly_identity id;
    struct file_info         manifest;
    WCHAR                   *directory;
    BOOL                     no_inherit;
    struct dll_redirect     *dlls;
    unsigned int             num_dlls;
    unsigned int             allocated_dlls;
    struct entity_array      entities;
};

enum context_sections
{
    WINDOWCLASS_SECTION    = 1,
    DLLREDIRECT_SECTION    = 2,
    TLIBREDIRECT_SECTION   = 4,
    SERVERREDIRECT_SECTION = 8,
    IFACEREDIRECT_SECTION  = 16,
    CLRSURROGATES_SECTION  = 32,
    PROGIDREDIRECT_SECTION = 64
};

typedef struct _ASSEMBLY_STORAGE_MAP_ENTRY
{
    ULONG Flags;
    UNICODE_STRING DosPath;
    HANDLE Handle;
} ASSEMBLY_STORAGE_MAP_ENTRY, *PASSEMBLY_STORAGE_MAP_ENTRY;

typedef struct _ASSEMBLY_STORAGE_MAP
{
    ULONG Flags;
    ULONG AssemblyCount;
    PASSEMBLY_STORAGE_MAP_ENTRY *AssemblyArray;
} ASSEMBLY_STORAGE_MAP, *PASSEMBLY_STORAGE_MAP;

typedef struct _ACTIVATION_CONTEXT
{
    LONG RefCount;
    ULONG Flags;
    LIST_ENTRY Links;
    PACTIVATION_CONTEXT_DATA ActivationContextData;
    PVOID NotificationRoutine;
    PVOID NotificationContext;
    ULONG SentNotifications[8];
    ULONG DisabledNotifications[8];
    ASSEMBLY_STORAGE_MAP StorageMap;
    PASSEMBLY_STORAGE_MAP_ENTRY InlineStorageMapEntries;
    ULONG StackTraceIndex;
    PVOID StackTraces[4][4];
    struct file_info config;
    struct file_info appdir;
    struct assembly *assemblies;
    unsigned int num_assemblies;
    unsigned int allocated_assemblies;
    /* section data */
    DWORD sections;
    struct strsection_header *wndclass_section;
    struct strsection_header *dllredirect_section;
    struct strsection_header *progid_section;
    struct guidsection_header *tlib_section;
    struct guidsection_header *comserver_section;
    struct guidsection_header *ifaceps_section;
    struct guidsection_header *clrsurrogate_section;
} ACTIVATION_CONTEXT, *PIACTIVATION_CONTEXT;

struct actctx_loader
{
    ACTIVATION_CONTEXT       *actctx;
    struct assembly_identity *dependencies;
    unsigned int              num_dependencies;
    unsigned int              allocated_dependencies;
};

static const WCHAR asmv1W[] = {'a','s','m','v','1',':',0};
static const WCHAR asmv2W[] = {'a','s','m','v','2',':',0};

typedef struct _ACTIVATION_CONTEXT_WRAPPED
{
    PVOID MagicMarker;
    ACTIVATION_CONTEXT ActivationContext;
} ACTIVATION_CONTEXT_WRAPPED, *PACTIVATION_CONTEXT_WRAPPED;

VOID
NTAPI
RtlpSxsBreakOnInvalidMarker(IN PACTIVATION_CONTEXT ActCtx,
                            IN ULONG FailureCode)
{
    EXCEPTION_RECORD ExceptionRecord;

    /* Fatal SxS exception header */
    ExceptionRecord.ExceptionRecord = NULL;
    ExceptionRecord.ExceptionCode = STATUS_SXS_CORRUPTION;
    ExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;

    /* With SxS-specific information plus the context itself */
    ExceptionRecord.ExceptionInformation[0] = 1;
    ExceptionRecord.ExceptionInformation[1] = FailureCode;
    ExceptionRecord.ExceptionInformation[2] = (ULONG_PTR)ActCtx;
    ExceptionRecord.NumberParameters = 3;

    /* Raise it */
    RtlRaiseException(&ExceptionRecord);
}

FORCEINLINE
VOID
RtlpValidateActCtx(IN PACTIVATION_CONTEXT ActCtx)
{
    PACTIVATION_CONTEXT_WRAPPED pActual;

    /* Get the caller-opaque header */
    pActual = CONTAINING_RECORD(ActCtx,
                                ACTIVATION_CONTEXT_WRAPPED,
                                ActivationContext);

    /* Check if the header matches as expected */
    if (pActual->MagicMarker != ACTCTX_MAGIC_MARKER)
    {
        /* Nope, print out a warning, assert, and then throw an exception */
        DbgPrint("%s : Invalid activation context marker %p found in activation context %p\n"
                 "     This means someone stepped on the allocation, or someone is using a\n"
                 "     deallocated activation context\n",
                 __FUNCTION__,
                 pActual->MagicMarker,
                 ActCtx);
        ASSERT(pActual->MagicMarker == ACTCTX_MAGIC_MARKER);
        RtlpSxsBreakOnInvalidMarker(ActCtx, 1);
    }
}

static const WCHAR assemblyW[] = {'a','s','s','e','m','b','l','y',0};
static const WCHAR assemblyIdentityW[] = {'a','s','s','e','m','b','l','y','I','d','e','n','t','i','t','y',0};
static const WCHAR bindingRedirectW[] = {'b','i','n','d','i','n','g','R','e','d','i','r','e','c','t',0};
static const WCHAR clrClassW[] = {'c','l','r','C','l','a','s','s',0};
static const WCHAR clrSurrogateW[] = {'c','l','r','S','u','r','r','o','g','a','t','e',0};
static const WCHAR comClassW[] = {'c','o','m','C','l','a','s','s',0};
static const WCHAR comInterfaceExternalProxyStubW[] = {'c','o','m','I','n','t','e','r','f','a','c','e','E','x','t','e','r','n','a','l','P','r','o','x','y','S','t','u','b',0};
static const WCHAR comInterfaceProxyStubW[] = {'c','o','m','I','n','t','e','r','f','a','c','e','P','r','o','x','y','S','t','u','b',0};
static const WCHAR dependencyW[] = {'d','e','p','e','n','d','e','n','c','y',0};
static const WCHAR dependentAssemblyW[] = {'d','e','p','e','n','d','e','n','t','A','s','s','e','m','b','l','y',0};
static const WCHAR descriptionW[] = {'d','e','s','c','r','i','p','t','i','o','n',0};
static const WCHAR fileW[] = {'f','i','l','e',0};
static const WCHAR hashW[] = {'h','a','s','h',0};
static const WCHAR noInheritW[] = {'n','o','I','n','h','e','r','i','t',0};
static const WCHAR noInheritableW[] = {'n','o','I','n','h','e','r','i','t','a','b','l','e',0};
static const WCHAR typelibW[] = {'t','y','p','e','l','i','b',0};
static const WCHAR windowClassW[] = {'w','i','n','d','o','w','C','l','a','s','s',0};

static const WCHAR clsidW[] = {'c','l','s','i','d',0};
static const WCHAR hashalgW[] = {'h','a','s','h','a','l','g',0};
static const WCHAR helpdirW[] = {'h','e','l','p','d','i','r',0};
static const WCHAR iidW[] = {'i','i','d',0};
static const WCHAR languageW[] = {'l','a','n','g','u','a','g','e',0};
static const WCHAR manifestVersionW[] = {'m','a','n','i','f','e','s','t','V','e','r','s','i','o','n',0};
static const WCHAR g_nameW[] = {'n','a','m','e',0};
static const WCHAR neutralW[] = {'n','e','u','t','r','a','l',0};
static const WCHAR newVersionW[] = {'n','e','w','V','e','r','s','i','o','n',0};
static const WCHAR oldVersionW[] = {'o','l','d','V','e','r','s','i','o','n',0};
static const WCHAR optionalW[] = {'o','p','t','i','o','n','a','l',0};
static const WCHAR processorArchitectureW[] = {'p','r','o','c','e','s','s','o','r','A','r','c','h','i','t','e','c','t','u','r','e',0};
static const WCHAR progidW[] = {'p','r','o','g','i','d',0};
static const WCHAR publicKeyTokenW[] = {'p','u','b','l','i','c','K','e','y','T','o','k','e','n',0};
static const WCHAR threadingmodelW[] = {'t','h','r','e','a','d','i','n','g','M','o','d','e','l',0};
static const WCHAR tlbidW[] = {'t','l','b','i','d',0};
static const WCHAR typeW[] = {'t','y','p','e',0};
static const WCHAR versionW[] = {'v','e','r','s','i','o','n',0};
static const WCHAR xmlnsW[] = {'x','m','l','n','s',0};
static const WCHAR versionedW[] = {'v','e','r','s','i','o','n','e','d',0};
static const WCHAR yesW[] = {'y','e','s',0};
static const WCHAR noW[] = {'n','o',0};
static const WCHAR restrictedW[] = {'R','E','S','T','R','I','C','T','E','D',0};
static const WCHAR controlW[] = {'C','O','N','T','R','O','L',0};
static const WCHAR hiddenW[] = {'H','I','D','D','E','N',0};
static const WCHAR hasdiskimageW[] = {'H','A','S','D','I','S','K','I','M','A','G','E',0};
static const WCHAR flagsW[] = {'f','l','a','g','s',0};
static const WCHAR miscstatusW[] = {'m','i','s','c','S','t','a','t','u','s',0};
static const WCHAR miscstatusiconW[] = {'m','i','s','c','S','t','a','t','u','s','I','c','o','n',0};
static const WCHAR miscstatuscontentW[] = {'m','i','s','c','S','t','a','t','u','s','C','o','n','t','e','n','t',0};
static const WCHAR miscstatusthumbnailW[] = {'m','i','s','c','S','t','a','t','u','s','T','h','u','m','b','n','a','i','l',0};
static const WCHAR miscstatusdocprintW[] = {'m','i','s','c','S','t','a','t','u','s','D','o','c','P','r','i','n','t',0};
static const WCHAR baseInterfaceW[] = {'b','a','s','e','I','n','t','e','r','f','a','c','e',0};
static const WCHAR nummethodsW[] = {'n','u','m','M','e','t','h','o','d','s',0};
static const WCHAR proxyStubClsid32W[] = {'p','r','o','x','y','S','t','u','b','C','l','s','i','d','3','2',0};
static const WCHAR runtimeVersionW[] = {'r','u','n','t','i','m','e','V','e','r','s','i','o','n',0};
static const WCHAR mscoreeW[] = {'M','S','C','O','R','E','E','.','D','L','L',0};
static const WCHAR mscoree2W[] = {'m','s','c','o','r','e','e','.','d','l','l',0};

static const WCHAR activatewhenvisibleW[] = {'a','c','t','i','v','a','t','e','w','h','e','n','v','i','s','i','b','l','e',0};
static const WCHAR actslikebuttonW[] = {'a','c','t','s','l','i','k','e','b','u','t','t','o','n',0};
static const WCHAR actslikelabelW[] = {'a','c','t','s','l','i','k','e','l','a','b','e','l',0};
static const WCHAR alignableW[] = {'a','l','i','g','n','a','b','l','e',0};
static const WCHAR alwaysrunW[] = {'a','l','w','a','y','s','r','u','n',0};
static const WCHAR canlinkbyole1W[] = {'c','a','n','l','i','n','k','b','y','o','l','e','1',0};
static const WCHAR cantlinkinsideW[] = {'c','a','n','t','l','i','n','k','i','n','s','i','d','e',0};
static const WCHAR ignoreactivatewhenvisibleW[] = {'i','g','n','o','r','e','a','c','t','i','v','a','t','e','w','h','e','n','v','i','s','i','b','l','e',0};
static const WCHAR imemodeW[] = {'i','m','e','m','o','d','e',0};
static const WCHAR insertnotreplaceW[] = {'i','n','s','e','r','t','n','o','t','r','e','p','l','a','c','e',0};
static const WCHAR insideoutW[] = {'i','n','s','i','d','e','o','u','t',0};
static const WCHAR invisibleatruntimeW[] = {'i','n','v','i','s','i','b','l','e','a','t','r','u','n','t','i','m','e',0};
static const WCHAR islinkobjectW[] = {'i','s','l','i','n','k','o','b','j','e','c','t',0};
static const WCHAR nouiactivateW[] = {'n','o','u','i','a','c','t','i','v','a','t','e',0};
static const WCHAR onlyiconicW[] = {'o','n','l','y','i','c','o','n','i','c',0};
static const WCHAR recomposeonresizeW[] = {'r','e','c','o','m','p','o','s','e','o','n','r','e','s','i','z','e',0};
static const WCHAR renderingisdeviceindependentW[] = {'r','e','n','d','e','r','i','n','g','i','s','d','e','v','i','c','e','i','n','d','e','p','e','n','d','e','n','t',0};
static const WCHAR setclientsitefirstW[] = {'s','e','t','c','l','i','e','n','t','s','i','t','e','f','i','r','s','t',0};
static const WCHAR simpleframeW[] = {'s','i','m','p','l','e','f','r','a','m','e',0};
static const WCHAR staticW[] = {'s','t','a','t','i','c',0};
static const WCHAR supportsmultilevelundoW[] = {'s','u','p','p','o','r','t','s','m','u','l','t','i','l','e','v','e','l','u','n','d','o',0};
static const WCHAR wantstomenumergeW[] = {'w','a','n','t','s','t','o','m','e','n','u','m','e','r','g','e',0};

struct olemisc_entry
{
    const WCHAR *name;
    OLEMISC value;
};

static const struct olemisc_entry olemisc_values[] =
{
    { activatewhenvisibleW,          OLEMISC_ACTIVATEWHENVISIBLE },
    { actslikebuttonW,               OLEMISC_ACTSLIKEBUTTON },
    { actslikelabelW,                OLEMISC_ACTSLIKELABEL },
    { alignableW,                    OLEMISC_ALIGNABLE },
    { alwaysrunW,                    OLEMISC_ALWAYSRUN },
    { canlinkbyole1W,                OLEMISC_CANLINKBYOLE1 },
    { cantlinkinsideW,               OLEMISC_CANTLINKINSIDE },
    { ignoreactivatewhenvisibleW,    OLEMISC_IGNOREACTIVATEWHENVISIBLE },
    { imemodeW,                      OLEMISC_IMEMODE },
    { insertnotreplaceW,             OLEMISC_INSERTNOTREPLACE },
    { insideoutW,                    OLEMISC_INSIDEOUT },
    { invisibleatruntimeW,           OLEMISC_INVISIBLEATRUNTIME },
    { islinkobjectW,                 OLEMISC_ISLINKOBJECT },
    { nouiactivateW,                 OLEMISC_NOUIACTIVATE },
    { onlyiconicW,                   OLEMISC_ONLYICONIC },
    { recomposeonresizeW,            OLEMISC_RECOMPOSEONRESIZE },
    { renderingisdeviceindependentW, OLEMISC_RENDERINGISDEVICEINDEPENDENT },
    { setclientsitefirstW,           OLEMISC_SETCLIENTSITEFIRST },
    { simpleframeW,                  OLEMISC_SIMPLEFRAME },
    { staticW,                       OLEMISC_STATIC },
    { supportsmultilevelundoW,       OLEMISC_SUPPORTSMULTILEVELUNDO },
    { wantstomenumergeW,             OLEMISC_WANTSTOMENUMERGE }
};

static const WCHAR g_xmlW[] = {'?','x','m','l',0};
static const WCHAR manifestv1W[] = {'u','r','n',':','s','c','h','e','m','a','s','-','m','i','c','r','o','s','o','f','t','-','c','o','m',':','a','s','m','.','v','1',0};
static const WCHAR manifestv2W[] = {'u','r','n',':','s','c','h','e','m','a','s','-','m','i','c','r','o','s','o','f','t','-','c','o','m',':','a','s','m','.','v','2',0};
static const WCHAR manifestv3W[] = {'u','r','n',':','s','c','h','e','m','a','s','-','m','i','c','r','o','s','o','f','t','-','c','o','m',':','a','s','m','.','v','3',0};

static const WCHAR dotManifestW[] = {'.','m','a','n','i','f','e','s','t',0};
static const WCHAR version_formatW[] = {'%','u','.','%','u','.','%','u','.','%','u',0};
static const WCHAR wildcardW[] = {'*',0};

static ACTIVATION_CONTEXT_WRAPPED system_actctx = { ACTCTX_MAGIC_MARKER, { 1 } };
static ACTIVATION_CONTEXT *process_actctx = &system_actctx.ActivationContext;

static WCHAR *strdupW(const WCHAR* str)
{
    WCHAR*      ptr;

    if (!(ptr = RtlAllocateHeap(RtlGetProcessHeap(), 0, (strlenW(str) + 1) * sizeof(WCHAR))))
        return NULL;
    return strcpyW(ptr, str);
}

static WCHAR *xmlstrdupW(const xmlstr_t* str)
{
    WCHAR *strW;

    if ((strW = RtlAllocateHeap(RtlGetProcessHeap(), 0, (str->len + 1) * sizeof(WCHAR))))
    {
        memcpy( strW, str->ptr, str->len * sizeof(WCHAR) );
        strW[str->len] = 0;
    }
    return strW;
}

static inline BOOL xmlstr_cmp(const xmlstr_t* xmlstr, const WCHAR *str)
{
    return !strncmpW(xmlstr->ptr, str, xmlstr->len) && !str[xmlstr->len];
}

static inline BOOL xmlstr_cmpi(const xmlstr_t* xmlstr, const WCHAR *str)
{
    return !strncmpiW(xmlstr->ptr, str, xmlstr->len) && !str[xmlstr->len];
}

static inline BOOL xmlstr_cmp_end(const xmlstr_t* xmlstr, const WCHAR *str)
{
    return (xmlstr->len && xmlstr->ptr[0] == '/' &&
            !strncmpW(xmlstr->ptr + 1, str, xmlstr->len - 1) && !str[xmlstr->len - 1]);
}

static inline BOOL xml_elem_cmp(const xmlstr_t *elem, const WCHAR *str, const WCHAR *namespace)
{
    UINT len = strlenW( namespace );

    if (!strncmpW(elem->ptr, str, elem->len) && !str[elem->len]) return TRUE;
    return (elem->len > len && !strncmpW(elem->ptr, namespace, len) &&
            !strncmpW(elem->ptr + len, str, elem->len - len) && !str[elem->len - len]);
}

static inline BOOL xml_elem_cmp_end(const xmlstr_t *elem, const WCHAR *str, const WCHAR *namespace)
{
    if (elem->len && elem->ptr[0] == '/')
    {
        xmlstr_t elem_end;
        elem_end.ptr = elem->ptr + 1;
        elem_end.len = elem->len - 1;
        return xml_elem_cmp( &elem_end, str, namespace );
    }
    return FALSE;
}

static inline BOOL isxmlspace( WCHAR ch )
{
    return (ch == ' ' || ch == '\r' || ch == '\n' || ch == '\t');
}

static UNICODE_STRING xmlstr2unicode(const xmlstr_t *xmlstr)
{
    UNICODE_STRING res;

    res.Buffer = (PWSTR)xmlstr->ptr;
    res.Length = res.MaximumLength = (USHORT)xmlstr->len * sizeof(WCHAR);

    return res;
}

static struct assembly *add_assembly(ACTIVATION_CONTEXT *actctx, enum assembly_type at)
{
    struct assembly *assembly;

    DPRINT("add_assembly() actctx %p, activeframe ??\n", actctx);

    if (actctx->num_assemblies == actctx->allocated_assemblies)
    {
        void *ptr;
        unsigned int new_count;
        if (actctx->assemblies)
        {
            new_count = actctx->allocated_assemblies * 2;
            ptr = RtlReAllocateHeap( RtlGetProcessHeap(), HEAP_ZERO_MEMORY,
                                     actctx->assemblies, new_count * sizeof(*assembly) );
        }
        else
        {
            new_count = 4;
            ptr = RtlAllocateHeap( RtlGetProcessHeap(), HEAP_ZERO_MEMORY, new_count * sizeof(*assembly) );
        }
        if (!ptr) return NULL;
        actctx->assemblies = ptr;
        actctx->allocated_assemblies = new_count;
    }

    assembly = &actctx->assemblies[actctx->num_assemblies++];
    assembly->type = at;
    return assembly;
}

static struct dll_redirect* add_dll_redirect(struct assembly* assembly)
{
    DPRINT("add_dll_redirect() to assembly %p, num_dlls %d\n", assembly, assembly->allocated_dlls);

    if (assembly->num_dlls == assembly->allocated_dlls)
    {
        void *ptr;
        unsigned int new_count;
        if (assembly->dlls)
        {
            new_count = assembly->allocated_dlls * 2;
            ptr = RtlReAllocateHeap( RtlGetProcessHeap(), HEAP_ZERO_MEMORY,
                                     assembly->dlls, new_count * sizeof(*assembly->dlls) );
        }
        else
        {
            new_count = 4;
            ptr = RtlAllocateHeap( RtlGetProcessHeap(), HEAP_ZERO_MEMORY, new_count * sizeof(*assembly->dlls) );
        }
        if (!ptr) return NULL;
        assembly->dlls = ptr;
        assembly->allocated_dlls = new_count;
    }
    return &assembly->dlls[assembly->num_dlls++];
}

static void free_assembly_identity(struct assembly_identity *ai)
{
    RtlFreeHeap( RtlGetProcessHeap(), 0, ai->name );
    RtlFreeHeap( RtlGetProcessHeap(), 0, ai->arch );
    RtlFreeHeap( RtlGetProcessHeap(), 0, ai->public_key );
    RtlFreeHeap( RtlGetProcessHeap(), 0, ai->language );
    RtlFreeHeap( RtlGetProcessHeap(), 0, ai->type );
}

static struct entity* add_entity(struct entity_array *array, DWORD kind)
{
    struct entity*      entity;

    if (array->num == array->allocated)
    {
        void *ptr;
        unsigned int new_count;
        if (array->base)
        {
            new_count = array->allocated * 2;
            ptr = RtlReAllocateHeap( RtlGetProcessHeap(), HEAP_ZERO_MEMORY,
                                     array->base, new_count * sizeof(*array->base) );
        }
        else
        {
            new_count = 4;
            ptr = RtlAllocateHeap( RtlGetProcessHeap(), HEAP_ZERO_MEMORY, new_count * sizeof(*array->base) );
        }
        if (!ptr) return NULL;
        array->base = ptr;
        array->allocated = new_count;
    }
    entity = &array->base[array->num++];
    entity->kind = kind;
    return entity;
}

static void free_entity_array(struct entity_array *array)
{
    unsigned int i, j;
    for (i = 0; i < array->num; i++)
    {
        struct entity *entity = &array->base[i];
        switch (entity->kind)
        {
        case ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION:
            RtlFreeHeap(RtlGetProcessHeap(), 0, entity->u.comclass.clsid);
            RtlFreeHeap(RtlGetProcessHeap(), 0, entity->u.comclass.tlbid);
            RtlFreeHeap(RtlGetProcessHeap(), 0, entity->u.comclass.progid);
            RtlFreeHeap(RtlGetProcessHeap(), 0, entity->u.comclass.name);
            RtlFreeHeap(RtlGetProcessHeap(), 0, entity->u.comclass.version);
            for (j = 0; j < entity->u.comclass.progids.num; j++)
                RtlFreeHeap(RtlGetProcessHeap(), 0, entity->u.comclass.progids.progids[j]);
            RtlFreeHeap(RtlGetProcessHeap(), 0, entity->u.comclass.progids.progids);
            break;
        case ACTIVATION_CONTEXT_SECTION_COM_INTERFACE_REDIRECTION:
            RtlFreeHeap(RtlGetProcessHeap(), 0, entity->u.ifaceps.iid);
            RtlFreeHeap(RtlGetProcessHeap(), 0, entity->u.ifaceps.base);
            RtlFreeHeap(RtlGetProcessHeap(), 0, entity->u.ifaceps.ps32);
            RtlFreeHeap(RtlGetProcessHeap(), 0, entity->u.ifaceps.name);
            RtlFreeHeap(RtlGetProcessHeap(), 0, entity->u.ifaceps.tlib);
            break;
        case ACTIVATION_CONTEXT_SECTION_COM_TYPE_LIBRARY_REDIRECTION:
            RtlFreeHeap(RtlGetProcessHeap(), 0, entity->u.typelib.tlbid);
            RtlFreeHeap(RtlGetProcessHeap(), 0, entity->u.typelib.helpdir);
            break;
        case ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION:
            RtlFreeHeap(RtlGetProcessHeap(), 0, entity->u.class.name);
            break;
        case ACTIVATION_CONTEXT_SECTION_CLR_SURROGATES:
            RtlFreeHeap(RtlGetProcessHeap(), 0, entity->u.clrsurrogate.name);
            RtlFreeHeap(RtlGetProcessHeap(), 0, entity->u.clrsurrogate.clsid);
            RtlFreeHeap(RtlGetProcessHeap(), 0, entity->u.clrsurrogate.version);
            break;
        default:
            DPRINT1("Unknown entity kind %u\n", entity->kind);
        }
    }
    RtlFreeHeap( RtlGetProcessHeap(), 0, array->base );
}

static BOOL is_matching_string( const WCHAR *str1, const WCHAR *str2 )
{
    if (!str1) return !str2;
    return str2 && !strcmpiW( str1, str2 );
}

static BOOL is_matching_identity( const struct assembly_identity *id1,
                                  const struct assembly_identity *id2 )
{
    if (!is_matching_string( id1->name, id2->name )) return FALSE;
    if (!is_matching_string( id1->arch, id2->arch )) return FALSE;
    if (!is_matching_string( id1->public_key, id2->public_key )) return FALSE;

    if (id1->language && id2->language && strcmpiW( id1->language, id2->language ))
    {
        if (strcmpW( wildcardW, id1->language ) && strcmpW( wildcardW, id2->language ))
            return FALSE;
    }
    if (id1->version.major != id2->version.major) return FALSE;
    if (id1->version.minor != id2->version.minor) return FALSE;
    if (id1->version.build > id2->version.build) return FALSE;
    if (id1->version.build == id2->version.build &&
        id1->version.revision > id2->version.revision) return FALSE;
    return TRUE;
}

static BOOL add_dependent_assembly_id(struct actctx_loader* acl,
                                      struct assembly_identity* ai)
{
    unsigned int i;

    /* check if we already have that assembly */

    for (i = 0; i < acl->actctx->num_assemblies; i++)
        if (is_matching_identity( ai, &acl->actctx->assemblies[i].id ))
        {
            DPRINT( "reusing existing assembly for %S arch %S version %u.%u.%u.%u\n",
                   ai->name, ai->arch, ai->version.major, ai->version.minor,
                   ai->version.build, ai->version.revision );
            return TRUE;
        }

    for (i = 0; i < acl->num_dependencies; i++)
        if (is_matching_identity( ai, &acl->dependencies[i] ))
        {
            DPRINT( "reusing existing dependency for %S arch %S version %u.%u.%u.%u\n",
                   ai->name, ai->arch, ai->version.major, ai->version.minor,
                   ai->version.build, ai->version.revision );
            return TRUE;
        }

    if (acl->num_dependencies == acl->allocated_dependencies)
    {
        void *ptr;
        unsigned int new_count;
        if (acl->dependencies)
        {
            new_count = acl->allocated_dependencies * 2;
            ptr = RtlReAllocateHeap(RtlGetProcessHeap(), 0, acl->dependencies,
                                    new_count * sizeof(acl->dependencies[0]));
        }
        else
        {
            new_count = 4;
            ptr = RtlAllocateHeap(RtlGetProcessHeap(), 0, new_count * sizeof(acl->dependencies[0]));
        }
        if (!ptr) return FALSE;
        acl->dependencies = ptr;
        acl->allocated_dependencies = new_count;
    }
    acl->dependencies[acl->num_dependencies++] = *ai;

    return TRUE;
}

static void free_depend_manifests(struct actctx_loader* acl)
{
    unsigned int i;
    for (i = 0; i < acl->num_dependencies; i++)
        free_assembly_identity(&acl->dependencies[i]);
    RtlFreeHeap(RtlGetProcessHeap(), 0, acl->dependencies);
}

static WCHAR *build_assembly_dir(struct assembly_identity* ai)
{
    static const WCHAR undW[] = {'_',0};
    static const WCHAR noneW[] = {'n','o','n','e',0};
    static const WCHAR mskeyW[] = {'d','e','a','d','b','e','e','f',0};

    const WCHAR *arch = ai->arch ? ai->arch : noneW;
    const WCHAR *key = ai->public_key ? ai->public_key : noneW;
    const WCHAR *lang = ai->language ? ai->language : noneW;
    const WCHAR *name = ai->name ? ai->name : noneW;
    SIZE_T size = (strlenW(arch) + 1 + strlenW(name) + 1 + strlenW(key) + 24 + 1 +
		    strlenW(lang) + 1) * sizeof(WCHAR) + sizeof(mskeyW);
    WCHAR *ret;

    if (!(ret = RtlAllocateHeap( RtlGetProcessHeap(), 0, size ))) return NULL;

    strcpyW( ret, arch );
    strcatW( ret, undW );
    strcatW( ret, name );
    strcatW( ret, undW );
    strcatW( ret, key );
    strcatW( ret, undW );
    sprintfW( ret + strlenW(ret), version_formatW,
              ai->version.major, ai->version.minor, ai->version.build, ai->version.revision );
    strcatW( ret, undW );
    strcatW( ret, lang );
    strcatW( ret, undW );
    strcatW( ret, mskeyW );
    return ret;
}

static inline void append_string( WCHAR *buffer, const WCHAR *prefix, const WCHAR *str )
{
    WCHAR *p = buffer;

    if (!str) return;
    strcatW( buffer, prefix );
    p += strlenW(p);
    *p++ = '"';
    strcpyW( p, str );
    p += strlenW(p);
    *p++ = '"';
    *p = 0;
}

static WCHAR *build_assembly_id( const struct assembly_identity *ai )
{
    static const WCHAR archW[] =
        {',','p','r','o','c','e','s','s','o','r','A','r','c','h','i','t','e','c','t','u','r','e','=',0};
    static const WCHAR public_keyW[] =
        {',','p','u','b','l','i','c','K','e','y','T','o','k','e','n','=',0};
    static const WCHAR typeW2[] =
        {',','t','y','p','e','=',0};
    static const WCHAR versionW2[] =
        {',','v','e','r','s','i','o','n','=',0};

    WCHAR version[64], *ret;
    SIZE_T size = 0;

    sprintfW( version, version_formatW,
              ai->version.major, ai->version.minor, ai->version.build, ai->version.revision );
    if (ai->name) size += strlenW(ai->name) * sizeof(WCHAR);
    if (ai->arch) size += strlenW(archW) + strlenW(ai->arch) + 2;
    if (ai->public_key) size += strlenW(public_keyW) + strlenW(ai->public_key) + 2;
    if (ai->type) size += strlenW(typeW2) + strlenW(ai->type) + 2;
    size += strlenW(versionW2) + strlenW(version) + 2;

    if (!(ret = RtlAllocateHeap( RtlGetProcessHeap(), 0, (size + 1) * sizeof(WCHAR) )))
        return NULL;

    if (ai->name) strcpyW( ret, ai->name );
    else *ret = 0;
    append_string( ret, archW, ai->arch );
    append_string( ret, public_keyW, ai->public_key );
    append_string( ret, typeW2, ai->type );
    append_string( ret, versionW2, version );
    return ret;
}

static ACTIVATION_CONTEXT *check_actctx( HANDLE h )
{
    ACTIVATION_CONTEXT *ret = NULL, *actctx = h;
    PACTIVATION_CONTEXT_WRAPPED pActual;

    if (!h || h == INVALID_HANDLE_VALUE) return NULL;
    _SEH2_TRY
    {
        if (actctx)
        {
            pActual = CONTAINING_RECORD(actctx, ACTIVATION_CONTEXT_WRAPPED, ActivationContext);
            if (pActual->MagicMarker == ACTCTX_MAGIC_MARKER) ret = &pActual->ActivationContext;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT1("Invalid activation context handle!\n");
    }
    _SEH2_END;
    return ret;
}

static inline void actctx_addref( ACTIVATION_CONTEXT *actctx )
{
    InterlockedExchangeAdd( &actctx->RefCount, 1 );
}

static void actctx_release( ACTIVATION_CONTEXT *actctx )
{
    PACTIVATION_CONTEXT_WRAPPED pActual;

    if (InterlockedExchangeAdd(&actctx->RefCount, -1) == 1)
    {
        unsigned int i, j;

        for (i = 0; i < actctx->num_assemblies; i++)
        {
            struct assembly *assembly = &actctx->assemblies[i];
            for (j = 0; j < assembly->num_dlls; j++)
            {
                struct dll_redirect *dll = &assembly->dlls[j];
                free_entity_array( &dll->entities );
                RtlFreeHeap( RtlGetProcessHeap(), 0, dll->name );
                RtlFreeHeap( RtlGetProcessHeap(), 0, dll->hash );
            }
            RtlFreeHeap( RtlGetProcessHeap(), 0, assembly->dlls );
            RtlFreeHeap( RtlGetProcessHeap(), 0, assembly->manifest.info );
            RtlFreeHeap( RtlGetProcessHeap(), 0, assembly->directory );
            free_entity_array( &assembly->entities );
            free_assembly_identity(&assembly->id);
        }
        RtlFreeHeap( RtlGetProcessHeap(), 0, actctx->config.info );
        RtlFreeHeap( RtlGetProcessHeap(), 0, actctx->appdir.info );
        RtlFreeHeap( RtlGetProcessHeap(), 0, actctx->assemblies );
        RtlFreeHeap( RtlGetProcessHeap(), 0, actctx->dllredirect_section );
        RtlFreeHeap( RtlGetProcessHeap(), 0, actctx->wndclass_section );
        RtlFreeHeap( RtlGetProcessHeap(), 0, actctx->tlib_section );
        RtlFreeHeap( RtlGetProcessHeap(), 0, actctx->comserver_section );
        RtlFreeHeap( RtlGetProcessHeap(), 0, actctx->ifaceps_section );
        RtlFreeHeap( RtlGetProcessHeap(), 0, actctx->clrsurrogate_section );
        RtlFreeHeap( RtlGetProcessHeap(), 0, actctx->progid_section );

        pActual = CONTAINING_RECORD(actctx, ACTIVATION_CONTEXT_WRAPPED, ActivationContext);
        pActual->MagicMarker = 0;
        RtlFreeHeap(RtlGetProcessHeap(), 0, pActual);
    }
}

static BOOL next_xml_attr(xmlbuf_t* xmlbuf, xmlstr_t* name, xmlstr_t* value,
                          BOOL* error, BOOL* end)
{
    const WCHAR* ptr;

    *error = TRUE;

    while (xmlbuf->ptr < xmlbuf->end && isxmlspace(*xmlbuf->ptr))
        xmlbuf->ptr++;

    if (xmlbuf->ptr == xmlbuf->end) return FALSE;

    if (*xmlbuf->ptr == '/')
    {
        xmlbuf->ptr++;
        if (xmlbuf->ptr == xmlbuf->end || *xmlbuf->ptr != '>')
            return FALSE;

        xmlbuf->ptr++;
        *end = TRUE;
        *error = FALSE;
        return FALSE;
    }

    if (*xmlbuf->ptr == '>')
    {
        xmlbuf->ptr++;
        *error = FALSE;
        return FALSE;
    }

    ptr = xmlbuf->ptr;
    while (ptr < xmlbuf->end && *ptr != '=' && *ptr != '>' && !isxmlspace(*ptr)) ptr++;

    if (ptr == xmlbuf->end) return FALSE;

    name->ptr = xmlbuf->ptr;
    name->len = ptr-xmlbuf->ptr;
    xmlbuf->ptr = ptr;

    /* skip spaces before '=' */
    while (ptr < xmlbuf->end && *ptr != '=' && isxmlspace(*ptr)) ptr++;
    if (ptr == xmlbuf->end || *ptr != '=') return FALSE;

    /* skip '=' itself */
    ptr++;
    if (ptr == xmlbuf->end) return FALSE;

    /* skip spaces after '=' */
    while (ptr < xmlbuf->end && *ptr != '"' && *ptr != '\'' && isxmlspace(*ptr)) ptr++;

    if (ptr == xmlbuf->end || (*ptr != '"' && *ptr != '\'')) return FALSE;

    value->ptr = ++ptr;
    if (ptr == xmlbuf->end) return FALSE;

    ptr = memchrW(ptr, ptr[-1], xmlbuf->end - ptr);
    if (!ptr)
    {
        xmlbuf->ptr = xmlbuf->end;
        return FALSE;
    }

    value->len = ptr - value->ptr;
    xmlbuf->ptr = ptr + 1;

    if (xmlbuf->ptr == xmlbuf->end) return FALSE;

    *error = FALSE;
    return TRUE;
}

static BOOL next_xml_elem(xmlbuf_t* xmlbuf, xmlstr_t* elem)
{
    const WCHAR* ptr;

    for (;;)
    {
        ptr = memchrW(xmlbuf->ptr, '<', xmlbuf->end - xmlbuf->ptr);
        if (!ptr)
        {
            xmlbuf->ptr = xmlbuf->end;
            return FALSE;
        }
        ptr++;
        if (ptr + 3 < xmlbuf->end && ptr[0] == '!' && ptr[1] == '-' && ptr[2] == '-') /* skip comment */
        {
            for (ptr += 3; ptr + 3 <= xmlbuf->end; ptr++)
                if (ptr[0] == '-' && ptr[1] == '-' && ptr[2] == '>') break;

            if (ptr + 3 > xmlbuf->end)
            {
                xmlbuf->ptr = xmlbuf->end;
                return FALSE;
            }
            xmlbuf->ptr = ptr + 3;
        }
        else break;
    }

    xmlbuf->ptr = ptr;
    while (ptr < xmlbuf->end && !isxmlspace(*ptr) && *ptr != '>' && (*ptr != '/' || ptr == xmlbuf->ptr))
        ptr++;

    elem->ptr = xmlbuf->ptr;
    elem->len = ptr - xmlbuf->ptr;
    xmlbuf->ptr = ptr;
    return xmlbuf->ptr != xmlbuf->end;
}

static BOOL parse_xml_header(xmlbuf_t* xmlbuf)
{
    /* FIXME: parse attributes */
    const WCHAR *ptr;

    for (ptr = xmlbuf->ptr; ptr < xmlbuf->end - 1; ptr++)
    {
        if (ptr[0] == '?' && ptr[1] == '>')
        {
            xmlbuf->ptr = ptr + 2;
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL parse_text_content(xmlbuf_t* xmlbuf, xmlstr_t* content)
{
    const WCHAR *ptr = memchrW(xmlbuf->ptr, '<', xmlbuf->end - xmlbuf->ptr);

    if (!ptr) return FALSE;

    content->ptr = xmlbuf->ptr;
    content->len = ptr - xmlbuf->ptr;
    xmlbuf->ptr = ptr;

    return TRUE;
}

static BOOL parse_version(const xmlstr_t *str, struct assembly_version *version)
{
    unsigned int ver[4];
    unsigned int pos;
    const WCHAR *curr;
    UNICODE_STRING strU;

    /* major.minor.build.revision */
    ver[0] = ver[1] = ver[2] = ver[3] = pos = 0;
    for (curr = str->ptr; curr < str->ptr + str->len; curr++)
    {
        if (*curr >= '0' && *curr <= '9')
        {
            ver[pos] = ver[pos] * 10 + *curr - '0';
            if (ver[pos] >= 0x10000) goto error;
        }
        else if (*curr == '.')
        {
            if (++pos >= 4) goto error;
        }
        else goto error;
    }
    version->major = ver[0];
    version->minor = ver[1];
    version->build = ver[2];
    version->revision = ver[3];
    return TRUE;

error:
    strU = xmlstr2unicode(str);
    DPRINT1( "Wrong version definition in manifest file (%wZ)\n", &strU );
    return FALSE;
}

static BOOL parse_expect_elem(xmlbuf_t* xmlbuf, const WCHAR* name, const WCHAR *namespace)
{
    xmlstr_t    elem;
    UNICODE_STRING elemU;
    if (!next_xml_elem(xmlbuf, &elem)) return FALSE;
    if (xml_elem_cmp(&elem, name, namespace)) return TRUE;
    elemU = xmlstr2unicode(&elem);
    DPRINT1( "unexpected element %wZ\n", &elemU );
    return FALSE;
}

static BOOL parse_expect_no_attr(xmlbuf_t* xmlbuf, BOOL* end)
{
    xmlstr_t    attr_name, attr_value;
    UNICODE_STRING attr_nameU, attr_valueU;
    BOOL        error;

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, end))
    {
        attr_nameU = xmlstr2unicode(&attr_name);
        attr_valueU = xmlstr2unicode(&attr_value);
        DPRINT1( "unexpected attr %wZ=%wZ\n", &attr_nameU,
             &attr_valueU);
    }
    return !error;
}

static BOOL parse_end_element(xmlbuf_t *xmlbuf)
{
    BOOL end = FALSE;
    return parse_expect_no_attr(xmlbuf, &end) && !end;
}

static BOOL parse_expect_end_elem(xmlbuf_t *xmlbuf, const WCHAR *name, const WCHAR *namespace)
{
    xmlstr_t    elem;
    UNICODE_STRING elemU;
    if (!next_xml_elem(xmlbuf, &elem)) return FALSE;
    if (!xml_elem_cmp_end(&elem, name, namespace))
    {
        elemU = xmlstr2unicode(&elem);
        DPRINT1( "unexpected element %wZ\n", &elemU );
        return FALSE;
    }
    return parse_end_element(xmlbuf);
}

static BOOL parse_unknown_elem(xmlbuf_t *xmlbuf, const xmlstr_t *unknown_elem)
{
    xmlstr_t attr_name, attr_value, elem;
    BOOL end = FALSE, error, ret = TRUE;

    while(next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end));
    if(error || end) return end;

    while(ret && (ret = next_xml_elem(xmlbuf, &elem)))
    {
        if(*elem.ptr == '/' && elem.len - 1 == unknown_elem->len &&
           !strncmpW(elem.ptr+1, unknown_elem->ptr, unknown_elem->len))
            break;
        else
            ret = parse_unknown_elem(xmlbuf, &elem);
    }

    return ret && parse_end_element(xmlbuf);
}

static BOOL parse_assembly_identity_elem(xmlbuf_t* xmlbuf, ACTIVATION_CONTEXT* actctx,
                                         struct assembly_identity* ai)
{
    xmlstr_t    attr_name, attr_value;
    BOOL        end = FALSE, error;
    UNICODE_STRING  attr_valueU, attr_nameU;

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        if (xmlstr_cmp(&attr_name, g_nameW))
        {
            if (!(ai->name = xmlstrdupW(&attr_value))) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, typeW))
        {
            if (!(ai->type = xmlstrdupW(&attr_value))) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, versionW))
        {
            if (!parse_version(&attr_value, &ai->version)) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, processorArchitectureW))
        {
            if (!(ai->arch = xmlstrdupW(&attr_value))) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, publicKeyTokenW))
        {
            if (!(ai->public_key = xmlstrdupW(&attr_value))) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, languageW))
        {
            DPRINT1("Unsupported yet language attribute (%S)\n",
                 ai->language);
            if (!(ai->language = xmlstrdupW(&attr_value))) return FALSE;
        }
        else
        {
            attr_nameU = xmlstr2unicode(&attr_name);
            attr_valueU = xmlstr2unicode(&attr_value);
            DPRINT1("unknown attr %wZ=%wZ\n", &attr_nameU, &attr_valueU);
        }
    }

    if (error || end) return end;
    return parse_expect_end_elem(xmlbuf, assemblyIdentityW, asmv1W);
}

static enum comclass_threadingmodel parse_com_class_threadingmodel(xmlstr_t *value)
{
    static const WCHAR apartW[] = {'A','p','a','r','t','m','e','n','t',0};
    static const WCHAR neutralW[] = {'N','e','u','t','r','a','l',0};
    static const WCHAR freeW[] = {'F','r','e','e',0};
    static const WCHAR bothW[] = {'B','o','t','h',0};

    if (value->len == 0) return ThreadingModel_No;
    if (xmlstr_cmp(value, apartW))
        return ThreadingModel_Apartment;
    else if (xmlstr_cmp(value, freeW))
        return ThreadingModel_Free;
    else if (xmlstr_cmp(value, bothW))
        return ThreadingModel_Both;
    else if (xmlstr_cmp(value, neutralW))
        return ThreadingModel_Neutral;
    else
        return ThreadingModel_No;
};

static OLEMISC get_olemisc_value(const WCHAR *str, int len)
{
    int min, max;

    min = 0;
    max = sizeof(olemisc_values)/sizeof(struct olemisc_entry) - 1;

    while (min <= max)
    {
        int n, c;

        n = (min+max)/2;

        c = strncmpW(olemisc_values[n].name, str, len);
        if (!c && !olemisc_values[n].name[len])
            return olemisc_values[n].value;

        if (c >= 0)
            max = n-1;
        else
            min = n+1;
    }

    DPRINT1("unknown flag %S\n", str);
    return 0;
}

static DWORD parse_com_class_misc(const xmlstr_t *value)
{
    const WCHAR *str = value->ptr, *start;
    DWORD flags = 0;
    int i = 0;

    /* it's comma separated list of flags */
    while (i < value->len)
    {
        start = str;
        while (*str != ',' && (i++ < value->len)) str++;

        flags |= get_olemisc_value(start, str-start);

        /* skip separator */
        str++;
        i++;
    }

    return flags;
}

static BOOL com_class_add_progid(const xmlstr_t *progid, struct entity *entity)
{
    struct progids *progids = &entity->u.comclass.progids;

    if (progids->allocated == 0)
    {
        progids->allocated = 4;
        if (!(progids->progids = RtlAllocateHeap(RtlGetProcessHeap(), 0, progids->allocated * sizeof(WCHAR*)))) return FALSE;
    }

    if (progids->allocated == progids->num)
    {
        progids->allocated *= 2;
        progids->progids = RtlReAllocateHeap(RtlGetProcessHeap(), 0, progids->progids, progids->allocated * sizeof(WCHAR*));
    }

    if (!(progids->progids[progids->num] = xmlstrdupW(progid))) return FALSE;
    progids->num++;

    return TRUE;
}

static BOOL parse_com_class_progid(xmlbuf_t* xmlbuf, struct entity *entity)
{
    xmlstr_t content;
    BOOL end = FALSE;

    if (!parse_expect_no_attr(xmlbuf, &end) || end || !parse_text_content(xmlbuf, &content))
        return FALSE;

    if (!com_class_add_progid(&content, entity)) return FALSE;
    return parse_expect_end_elem(xmlbuf, progidW, asmv1W);
}

static BOOL parse_com_class_elem(xmlbuf_t* xmlbuf, struct dll_redirect* dll, struct actctx_loader *acl)
{
    xmlstr_t elem, attr_name, attr_value;
    BOOL ret = TRUE, end = FALSE, error;
    struct entity*      entity;
    UNICODE_STRING  attr_valueU, attr_nameU;

    if (!(entity = add_entity(&dll->entities, ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION)))
        return FALSE;

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        if (xmlstr_cmp(&attr_name, clsidW))
        {
            if (!(entity->u.comclass.clsid = xmlstrdupW(&attr_value))) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, progidW))
        {
            if (!(entity->u.comclass.progid = xmlstrdupW(&attr_value))) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, tlbidW))
        {
            if (!(entity->u.comclass.tlbid = xmlstrdupW(&attr_value))) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, threadingmodelW))
        {
            entity->u.comclass.model = parse_com_class_threadingmodel(&attr_value);
        }
        else if (xmlstr_cmp(&attr_name, miscstatusW))
        {
            entity->u.comclass.miscstatus = parse_com_class_misc(&attr_value);
        }
        else if (xmlstr_cmp(&attr_name, miscstatuscontentW))
        {
            entity->u.comclass.miscstatuscontent = parse_com_class_misc(&attr_value);
        }
        else if (xmlstr_cmp(&attr_name, miscstatusthumbnailW))
        {
            entity->u.comclass.miscstatusthumbnail = parse_com_class_misc(&attr_value);
        }
        else if (xmlstr_cmp(&attr_name, miscstatusiconW))
        {
            entity->u.comclass.miscstatusicon = parse_com_class_misc(&attr_value);
        }
        else if (xmlstr_cmp(&attr_name, miscstatusdocprintW))
        {
            entity->u.comclass.miscstatusdocprint = parse_com_class_misc(&attr_value);
        }
        else if (xmlstr_cmp(&attr_name, descriptionW))
        {
            /* not stored */
        }
        else
        {
            attr_nameU = xmlstr2unicode(&attr_name);
            attr_valueU = xmlstr2unicode(&attr_value);
            DPRINT1("unknown attr %wZ=%wZ\n", &attr_nameU, &attr_valueU);
        }
    }

    if (error) return FALSE;

    acl->actctx->sections |= SERVERREDIRECT_SECTION;
    if (entity->u.comclass.progid)
        acl->actctx->sections |= PROGIDREDIRECT_SECTION;

    if (end) return TRUE;

    while (ret && (ret = next_xml_elem(xmlbuf, &elem)))
    {
        if (xmlstr_cmp_end(&elem, comClassW))
        {
            ret = parse_end_element(xmlbuf);
            break;
        }
        else if (xmlstr_cmp(&elem, progidW))
        {
            ret = parse_com_class_progid(xmlbuf, entity);
        }
        else
        {
            attr_nameU = xmlstr2unicode(&elem);
            DPRINT1("unknown elem %wZ\n", &attr_nameU);
            ret = parse_unknown_elem(xmlbuf, &elem);
        }
    }

    if (entity->u.comclass.progids.num)
        acl->actctx->sections |= PROGIDREDIRECT_SECTION;

    return ret;
}

static BOOL parse_nummethods(const xmlstr_t *str, struct entity *entity)
{
    const WCHAR *curr;
    ULONG num = 0;

    for (curr = str->ptr; curr < str->ptr + str->len; curr++)
    {
        if (*curr >= '0' && *curr <= '9')
            num = num * 10 + *curr - '0';
        else
        {
            UNICODE_STRING strU = xmlstr2unicode(str);
            DPRINT1("wrong numeric value %wZ\n", &strU);
            return FALSE;
        }
    }
    entity->u.ifaceps.nummethods = num;

    return TRUE;
}

static BOOL parse_cominterface_proxy_stub_elem(xmlbuf_t* xmlbuf, struct dll_redirect* dll, struct actctx_loader* acl)
{
    xmlstr_t    attr_name, attr_value;
    BOOL        end = FALSE, error;
    struct entity*      entity;
    UNICODE_STRING  attr_valueU, attr_nameU;

    if (!(entity = add_entity(&dll->entities, ACTIVATION_CONTEXT_SECTION_COM_INTERFACE_REDIRECTION)))
        return FALSE;

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        if (xmlstr_cmp(&attr_name, iidW))
        {
            if (!(entity->u.ifaceps.iid = xmlstrdupW(&attr_value))) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, g_nameW))
        {
            if (!(entity->u.ifaceps.name = xmlstrdupW(&attr_value))) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, baseInterfaceW))
        {
            if (!(entity->u.ifaceps.base = xmlstrdupW(&attr_value))) return FALSE;
            entity->u.ifaceps.mask |= BaseIface;
        }
        else if (xmlstr_cmp(&attr_name, nummethodsW))
        {
            if (!(parse_nummethods(&attr_value, entity))) return FALSE;
            entity->u.ifaceps.mask |= NumMethods;
        }
        else if (xmlstr_cmp(&attr_name, tlbidW))
        {
            if (!(entity->u.ifaceps.tlib = xmlstrdupW(&attr_value))) return FALSE;
        }
        /* not used */
        else if (xmlstr_cmp(&attr_name, proxyStubClsid32W) || xmlstr_cmp(&attr_name, threadingmodelW))
        {
        }
        else
        {
            attr_nameU = xmlstr2unicode(&attr_name);
            attr_valueU = xmlstr2unicode(&attr_value);
            DPRINT1("unknown attr %wZ=%wZ\n", &attr_nameU, &attr_valueU);
        }
    }

    if (error) return FALSE;
    acl->actctx->sections |= IFACEREDIRECT_SECTION;
    if (end) return TRUE;

    return parse_expect_end_elem(xmlbuf, comInterfaceProxyStubW, asmv1W);
}

static BOOL parse_typelib_flags(const xmlstr_t *value, struct entity *entity)
{
    WORD *flags = &entity->u.typelib.flags;
    const WCHAR *str = value->ptr, *start;
    int i = 0;

    *flags = 0;

    /* it's comma separated list of flags */
    while (i < value->len)
    {
        start = str;
        while (*str != ',' && (i++ < value->len)) str++;

        if (!strncmpiW(start, restrictedW, str-start))
            *flags |= LIBFLAG_FRESTRICTED;
        else if (!strncmpiW(start, controlW, str-start))
            *flags |= LIBFLAG_FCONTROL;
        else if (!strncmpiW(start, hiddenW, str-start))
            *flags |= LIBFLAG_FHIDDEN;
        else if (!strncmpiW(start, hasdiskimageW, str-start))
            *flags |= LIBFLAG_FHASDISKIMAGE;
        else
        {
            UNICODE_STRING valueU = xmlstr2unicode(value);
            DPRINT1("unknown flags value %wZ\n", &valueU);
            return FALSE;
        }

        /* skip separator */
        str++;
        i++;
    }

    return TRUE;
}

static BOOL parse_typelib_version(const xmlstr_t *str, struct entity *entity)
{
    unsigned int ver[2];
    unsigned int pos;
    const WCHAR *curr;
    UNICODE_STRING strW;

    /* major.minor */
    ver[0] = ver[1] = pos = 0;
    for (curr = str->ptr; curr < str->ptr + str->len; curr++)
    {
        if (*curr >= '0' && *curr <= '9')
        {
            ver[pos] = ver[pos] * 10 + *curr - '0';
            if (ver[pos] >= 0x10000) goto error;
        }
        else if (*curr == '.')
        {
            if (++pos >= 2) goto error;
        }
        else goto error;
    }
    entity->u.typelib.major = ver[0];
    entity->u.typelib.minor = ver[1];
    return TRUE;

error:
    strW = xmlstr2unicode(str);
    DPRINT1("FIXME: wrong typelib version value (%wZ)\n", &strW);
    return FALSE;
}

static BOOL parse_typelib_elem(xmlbuf_t* xmlbuf, struct dll_redirect* dll, struct actctx_loader* acl)
{
    xmlstr_t    attr_name, attr_value;
    BOOL        end = FALSE, error;
    struct entity*      entity;
    UNICODE_STRING  attr_valueU, attr_nameU;

    if (!(entity = add_entity(&dll->entities, ACTIVATION_CONTEXT_SECTION_COM_TYPE_LIBRARY_REDIRECTION)))
        return FALSE;

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        if (xmlstr_cmp(&attr_name, tlbidW))
        {
            if (!(entity->u.typelib.tlbid = xmlstrdupW(&attr_value))) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, versionW))
        {
            if (!parse_typelib_version(&attr_value, entity)) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, helpdirW))
        {
            if (!(entity->u.typelib.helpdir = xmlstrdupW(&attr_value))) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, flagsW))
        {
            if (!parse_typelib_flags(&attr_value, entity)) return FALSE;
        }
        else
        {
            attr_nameU = xmlstr2unicode(&attr_name);
            attr_valueU = xmlstr2unicode(&attr_value);
            DPRINT1("unknown attr %wZ=%wZ\n", &attr_nameU, &attr_valueU);
        }
    }

    if (error) return FALSE;

    acl->actctx->sections |= TLIBREDIRECT_SECTION;

    if (end) return TRUE;

    return parse_expect_end_elem(xmlbuf, typelibW, asmv1W);
}

static inline int aligned_string_len(int len)
{
    return (len + 3) & ~3;
}

static int get_assembly_version(struct assembly *assembly, WCHAR *ret)
{
    static const WCHAR fmtW[] = {'%','u','.','%','u','.','%','u','.','%','u',0};
    struct assembly_version *ver = &assembly->id.version;
    WCHAR buff[25];

    if (!ret) ret = buff;
    return sprintfW(ret, fmtW, ver->major, ver->minor, ver->build, ver->revision);
}

static BOOL parse_window_class_elem(xmlbuf_t* xmlbuf, struct dll_redirect* dll, struct actctx_loader* acl)
{
    xmlstr_t elem, content, attr_name, attr_value;
    BOOL end = FALSE, ret = TRUE, error;
    struct entity*      entity;
    UNICODE_STRING elemU, attr_nameU, attr_valueU;

    if (!(entity = add_entity(&dll->entities, ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION)))
        return FALSE;

    entity->u.class.versioned = TRUE;
    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        if (xmlstr_cmp(&attr_name, versionedW))
        {
            if (xmlstr_cmpi(&attr_value, noW))
                entity->u.class.versioned = FALSE;
            else if (!xmlstr_cmpi(&attr_value, yesW))
               return FALSE;
        }
        else
        {
            attr_nameU = xmlstr2unicode(&attr_name);
            attr_valueU = xmlstr2unicode(&attr_value);
            DPRINT1("unknown attr %wZ=%wZ\n", &attr_nameU, &attr_valueU);
        }
    }

    if (error || end) return end;

    if (!parse_text_content(xmlbuf, &content)) return FALSE;

    if (!(entity->u.class.name = xmlstrdupW(&content))) return FALSE;

    acl->actctx->sections |= WINDOWCLASS_SECTION;

    while (ret && (ret = next_xml_elem(xmlbuf, &elem)))
    {
        if (xmlstr_cmp_end(&elem, windowClassW))
        {
            ret = parse_end_element(xmlbuf);
            break;
        }
        else
        {
            elemU = xmlstr2unicode(&elem);
            DPRINT1("unknown elem %wZ\n", &elemU);
            ret = parse_unknown_elem(xmlbuf, &elem);
        }
    }

    return ret;
}

static BOOL parse_binding_redirect_elem(xmlbuf_t* xmlbuf)
{
    xmlstr_t    attr_name, attr_value;
    UNICODE_STRING  attr_valueU, attr_nameU;
    BOOL        end = FALSE, error;

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        attr_nameU = xmlstr2unicode(&attr_name);
        attr_valueU = xmlstr2unicode(&attr_value);

        if (xmlstr_cmp(&attr_name, oldVersionW))
        {
            DPRINT1("Not stored yet oldVersion=%wZ\n", &attr_valueU);
        }
        else if (xmlstr_cmp(&attr_name, newVersionW))
        {
            DPRINT1("Not stored yet newVersion=%wZ\n", &attr_valueU);
        }
        else
        {
            DPRINT1("unknown attr %wZ=%wZ\n", &attr_nameU, &attr_valueU);
        }
    }

    if (error || end) return end;
    return parse_expect_end_elem(xmlbuf, bindingRedirectW, asmv1W);
}

static BOOL parse_description_elem(xmlbuf_t* xmlbuf)
{
    xmlstr_t    elem, content, attr_name, attr_value;
    BOOL        end = FALSE, ret = TRUE, error = FALSE;

    UNICODE_STRING elem1U, elem2U;

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        elem1U = xmlstr2unicode(&attr_name);
        elem2U = xmlstr2unicode(&attr_value);
        DPRINT1("unknown attr %s=%s\n", &elem1U, &elem2U);
    }

    if (error) return FALSE;
    if (end) return TRUE;

    if (!parse_text_content(xmlbuf, &content))
        return FALSE;

    elem1U = xmlstr2unicode(&content);
    DPRINT("Got description %wZ\n", &elem1U);

    while (ret && (ret = next_xml_elem(xmlbuf, &elem)))
    {
        if (xmlstr_cmp_end(&elem, descriptionW))
        {
            ret = parse_end_element(xmlbuf);
            break;
        }
        else
        {
            elem1U = xmlstr2unicode(&elem);
            DPRINT1("unknown elem %wZ\n", &elem1U);
            ret = parse_unknown_elem(xmlbuf, &elem);
        }
    }

    return ret;
}

static BOOL parse_com_interface_external_proxy_stub_elem(xmlbuf_t* xmlbuf,
                                                         struct assembly* assembly,
                                                         struct actctx_loader* acl)
{
    xmlstr_t            attr_name, attr_value;
    UNICODE_STRING      attr_nameU, attr_valueU;
    BOOL                end = FALSE, error;
    struct entity*      entity;

    entity = add_entity(&assembly->entities, ACTIVATION_CONTEXT_SECTION_COM_INTERFACE_REDIRECTION);
    if (!entity) return FALSE;

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        if (xmlstr_cmp(&attr_name, iidW))
        {
            if (!(entity->u.ifaceps.iid = xmlstrdupW(&attr_value))) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, g_nameW))
        {
            if (!(entity->u.ifaceps.name = xmlstrdupW(&attr_value))) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, baseInterfaceW))
        {
            if (!(entity->u.ifaceps.base = xmlstrdupW(&attr_value))) return FALSE;
            entity->u.ifaceps.mask |= BaseIface;
        }
        else if (xmlstr_cmp(&attr_name, nummethodsW))
        {
            if (!(parse_nummethods(&attr_value, entity))) return FALSE;
            entity->u.ifaceps.mask |= NumMethods;
        }
        else if (xmlstr_cmp(&attr_name, proxyStubClsid32W))
        {
            if (!(entity->u.ifaceps.ps32 = xmlstrdupW(&attr_value))) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, tlbidW))
        {
            if (!(entity->u.ifaceps.tlib = xmlstrdupW(&attr_value))) return FALSE;
        }
        else
        {
            attr_nameU = xmlstr2unicode(&attr_name);
            attr_valueU = xmlstr2unicode(&attr_value);
            DPRINT1("unknown attr %wZ=%wZ\n", &attr_nameU, &attr_valueU);
        }
    }

    if (error) return FALSE;
    acl->actctx->sections |= IFACEREDIRECT_SECTION;
    if (end) return TRUE;

    return parse_expect_end_elem(xmlbuf, comInterfaceExternalProxyStubW, asmv1W);
}

static BOOL parse_clr_class_elem(xmlbuf_t* xmlbuf, struct assembly* assembly, struct actctx_loader *acl)
{
    xmlstr_t    attr_name, attr_value, elem;
    BOOL        end = FALSE, error, ret = TRUE;
    struct entity*      entity;

    entity = add_entity(&assembly->entities, ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION);
    if (!entity) return FALSE;

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        if (xmlstr_cmp(&attr_name, g_nameW))
        {
            if (!(entity->u.comclass.name = xmlstrdupW(&attr_value))) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, clsidW))
        {
            if (!(entity->u.comclass.clsid = xmlstrdupW(&attr_value))) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, progidW))
        {
            if (!(entity->u.comclass.progid = xmlstrdupW(&attr_value))) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, tlbidW))
        {
            if (!(entity->u.comclass.tlbid = xmlstrdupW(&attr_value))) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, threadingmodelW))
        {
            entity->u.comclass.model = parse_com_class_threadingmodel(&attr_value);
        }
        else if (xmlstr_cmp(&attr_name, runtimeVersionW))
        {
            if (!(entity->u.comclass.version = xmlstrdupW(&attr_value))) return FALSE;
        }
        else
        {
            UNICODE_STRING attr_nameU, attr_valueU;
            attr_nameU = xmlstr2unicode(&attr_name);
            attr_valueU = xmlstr2unicode(&attr_value);
            DPRINT1("unknown attr %wZ=%wZ\n", &attr_nameU, &attr_valueU);
        }
    }

    if (error) return FALSE;
    acl->actctx->sections |= SERVERREDIRECT_SECTION;
    if (entity->u.comclass.progid)
        acl->actctx->sections |= PROGIDREDIRECT_SECTION;
    if (end) return TRUE;

    while (ret && (ret = next_xml_elem(xmlbuf, &elem)))
    {
        if (xmlstr_cmp_end(&elem, clrClassW))
        {
            ret = parse_end_element(xmlbuf);
            break;
        }
        else if (xmlstr_cmp(&elem, progidW))
        {
            ret = parse_com_class_progid(xmlbuf, entity);
        }
        else
        {
            UNICODE_STRING elemU = xmlstr2unicode(&elem);
            DPRINT1("unknown elem %wZ\n", &elemU);
            ret = parse_unknown_elem(xmlbuf, &elem);
        }
    }

    if (entity->u.comclass.progids.num)
        acl->actctx->sections |= PROGIDREDIRECT_SECTION;

    return ret;
}

static BOOL parse_clr_surrogate_elem(xmlbuf_t* xmlbuf, struct assembly* assembly, struct actctx_loader *acl)
{
    xmlstr_t    attr_name, attr_value;
    UNICODE_STRING attr_nameU, attr_valueU;
    BOOL        end = FALSE, error;
    struct entity*      entity;

    entity = add_entity(&assembly->entities, ACTIVATION_CONTEXT_SECTION_CLR_SURROGATES);
    if (!entity) return FALSE;

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        if (xmlstr_cmp(&attr_name, g_nameW))
        {
            if (!(entity->u.clrsurrogate.name = xmlstrdupW(&attr_value))) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, clsidW))
        {
            if (!(entity->u.clrsurrogate.clsid = xmlstrdupW(&attr_value))) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, runtimeVersionW))
        {
            if (!(entity->u.clrsurrogate.version = xmlstrdupW(&attr_value))) return FALSE;
        }
        else
        {
            attr_nameU = xmlstr2unicode(&attr_name);
            attr_valueU = xmlstr2unicode(&attr_value);
            DPRINT1("unknown attr %wZ=%wZ\n", &attr_nameU, &attr_valueU);
        }
    }

    if (error) return FALSE;
    acl->actctx->sections |= CLRSURROGATES_SECTION;
    if (end) return TRUE;

    return parse_expect_end_elem(xmlbuf, clrSurrogateW, asmv1W);
}

static BOOL parse_dependent_assembly_elem(xmlbuf_t* xmlbuf, struct actctx_loader* acl, BOOL optional)
{
    struct assembly_identity    ai;
    xmlstr_t                    elem, attr_name, attr_value;
    BOOL                        end = FALSE, error = FALSE, ret = TRUE, delayed = FALSE;

    UNICODE_STRING elem1U, elem2U;

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        static const WCHAR allowDelayedBindingW[] = {'a','l','l','o','w','D','e','l','a','y','e','d','B','i','n','d','i','n','g',0};
        static const WCHAR trueW[] = {'t','r','u','e',0};

        if (xmlstr_cmp(&attr_name, allowDelayedBindingW))
            delayed = xmlstr_cmp(&attr_value, trueW);
        else
        {
            elem1U = xmlstr2unicode(&attr_name);
            elem2U = xmlstr2unicode(&attr_value);
            DPRINT1("unknown attr %s=%s\n", &elem1U, &elem2U);
        }
    }

    if (error || end) return end;

    memset(&ai, 0, sizeof(ai));
    ai.optional = optional;
    ai.delayed = delayed;

    if (!parse_expect_elem(xmlbuf, assemblyIdentityW, asmv1W) ||
        !parse_assembly_identity_elem(xmlbuf, acl->actctx, &ai))
        return FALSE;

    //TRACE( "adding name=%s version=%s arch=%s\n", debugstr_w(ai.name), debugstr_version(&ai.version), debugstr_w(ai.arch) );

    /* store the newly found identity for later loading */
    if (!add_dependent_assembly_id(acl, &ai)) return FALSE;

    while (ret && (ret = next_xml_elem(xmlbuf, &elem)))
    {
        if (xmlstr_cmp_end(&elem, dependentAssemblyW))
        {
            ret = parse_end_element(xmlbuf);
            break;
        }
        else if (xmlstr_cmp(&elem, bindingRedirectW))
        {
            ret = parse_binding_redirect_elem(xmlbuf);
        }
        else
        {
            DPRINT1("unknown elem %S\n", elem.ptr);
            ret = parse_unknown_elem(xmlbuf, &elem);
        }
    }

    return ret;
}

static BOOL parse_dependency_elem(xmlbuf_t* xmlbuf, struct actctx_loader* acl)
{
    xmlstr_t attr_name, attr_value, elem;
    UNICODE_STRING attr_nameU, attr_valueU;
    BOOL end = FALSE, ret = TRUE, error, optional = FALSE;

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        attr_nameU = xmlstr2unicode(&attr_name);
        attr_valueU = xmlstr2unicode(&attr_value);

        if (xmlstr_cmp(&attr_name, optionalW))
        {
            optional = xmlstr_cmpi( &attr_value, yesW );
            DPRINT1("optional=%wZ\n", &attr_valueU);
        }
        else
        {
            DPRINT1("unknown attr %wZ=%wZ\n", &attr_nameU, &attr_valueU);
        }
    }

    while (ret && (ret = next_xml_elem(xmlbuf, &elem)))
    {
        if (xmlstr_cmp_end(&elem, dependencyW))
        {
            ret = parse_end_element(xmlbuf);
            break;
        }
        else if (xmlstr_cmp(&elem, dependentAssemblyW))
        {
            ret = parse_dependent_assembly_elem(xmlbuf, acl, optional);
        }
        else
        {
            attr_nameU = xmlstr2unicode(&elem);
            DPRINT1("unknown element %wZ\n", &attr_nameU);
            ret = parse_unknown_elem(xmlbuf, &elem);
        }
    }

    return ret;
}

static BOOL parse_noinherit_elem(xmlbuf_t* xmlbuf)
{
    BOOL end = FALSE;

    if (!parse_expect_no_attr(xmlbuf, &end)) return FALSE;
    return end || parse_expect_end_elem(xmlbuf, noInheritW, asmv1W);
}

static BOOL parse_noinheritable_elem(xmlbuf_t* xmlbuf)
{
    BOOL end = FALSE;

    if (!parse_expect_no_attr(xmlbuf, &end)) return FALSE;
    return end || parse_expect_end_elem(xmlbuf, noInheritableW, asmv1W);
}

static BOOL parse_file_elem(xmlbuf_t* xmlbuf, struct assembly* assembly, struct actctx_loader* acl)
{
    xmlstr_t    attr_name, attr_value, elem;
    UNICODE_STRING attr_nameU, attr_valueU;
    BOOL        end = FALSE, error, ret = TRUE;
    struct dll_redirect* dll;

    if (!(dll = add_dll_redirect(assembly))) return FALSE;

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        attr_nameU = xmlstr2unicode(&attr_name);
        attr_valueU = xmlstr2unicode(&attr_value);

        if (xmlstr_cmp(&attr_name, g_nameW))
        {
            if (!(dll->name = xmlstrdupW(&attr_value))) return FALSE;
            DPRINT("name=%wZ\n", &attr_valueU);
        }
        else if (xmlstr_cmp(&attr_name, hashW))
        {
            if (!(dll->hash = xmlstrdupW(&attr_value))) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, hashalgW))
        {
            static const WCHAR sha1W[] = {'S','H','A','1',0};
            if (!xmlstr_cmpi(&attr_value, sha1W))
                DPRINT1("hashalg should be SHA1, got %wZ\n", &attr_valueU);
        }
        else
        {
            DPRINT1("unknown attr %wZ=%wZ\n", &attr_nameU, &attr_valueU);
        }
    }

    if (error || !dll->name) return FALSE;

    acl->actctx->sections |= DLLREDIRECT_SECTION;

    if (end) return TRUE;

    while (ret && (ret = next_xml_elem(xmlbuf, &elem)))
    {
        if (xmlstr_cmp_end(&elem, fileW))
        {
            ret = parse_end_element(xmlbuf);
            break;
        }
        else if (xmlstr_cmp(&elem, comClassW))
        {
            ret = parse_com_class_elem(xmlbuf, dll, acl);
        }
        else if (xmlstr_cmp(&elem, comInterfaceProxyStubW))
        {
            ret = parse_cominterface_proxy_stub_elem(xmlbuf, dll, acl);
        }
        else if (xml_elem_cmp(&elem, hashW, asmv2W))
        {
            DPRINT1("asmv2hash (undocumented) not supported\n");
            ret = parse_unknown_elem(xmlbuf, &elem);
        }
        else if (xmlstr_cmp(&elem, typelibW))
        {
            ret = parse_typelib_elem(xmlbuf, dll, acl);
        }
        else if (xmlstr_cmp(&elem, windowClassW))
        {
            ret = parse_window_class_elem(xmlbuf, dll, acl);
        }
        else
        {
            attr_nameU = xmlstr2unicode(&elem);
            DPRINT1("unknown elem %wZ\n", &attr_nameU);
            ret = parse_unknown_elem( xmlbuf, &elem );
        }
    }

    return ret;
}

static BOOL parse_assembly_elem(xmlbuf_t* xmlbuf, struct actctx_loader* acl,
                                struct assembly* assembly,
                                struct assembly_identity* expected_ai)
{
    xmlstr_t    attr_name, attr_value, elem;
    UNICODE_STRING attr_nameU, attr_valueU;
    BOOL        end = FALSE, error, version = FALSE, xmlns = FALSE, ret = TRUE;

    DPRINT("(%p)\n", xmlbuf);

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        attr_nameU = xmlstr2unicode(&attr_name);
        attr_valueU = xmlstr2unicode(&attr_value);

        if (xmlstr_cmp(&attr_name, manifestVersionW))
        {
            static const WCHAR v10W[] = {'1','.','0',0};
            if (!xmlstr_cmp(&attr_value, v10W))
            {
                DPRINT1("wrong version %wZ\n", &attr_valueU);
                return FALSE;
            }
            version = TRUE;
        }
        else if (xmlstr_cmp(&attr_name, xmlnsW))
        {
            if (!xmlstr_cmp(&attr_value, manifestv1W) &&
                !xmlstr_cmp(&attr_value, manifestv2W) &&
                !xmlstr_cmp(&attr_value, manifestv3W))
            {
                DPRINT1("wrong namespace %wZ\n", &attr_valueU);
                return FALSE;
            }
            xmlns = TRUE;
        }
        else
        {
            DPRINT1("unknown attr %wZ=%wZ\n", &attr_nameU, &attr_valueU);
        }
    }

    if (error || end || !xmlns || !version) return FALSE;
    if (!next_xml_elem(xmlbuf, &elem)) return FALSE;

    if (assembly->type == APPLICATION_MANIFEST && xmlstr_cmp(&elem, noInheritW))
    {
        if (!parse_noinherit_elem(xmlbuf) || !next_xml_elem(xmlbuf, &elem))
            return FALSE;
        assembly->no_inherit = TRUE;
    }

    if (xml_elem_cmp(&elem, noInheritableW, asmv1W))
    {
        if (!parse_noinheritable_elem(xmlbuf) || !next_xml_elem(xmlbuf, &elem))
            return FALSE;
    }
    else if ((assembly->type == ASSEMBLY_MANIFEST || assembly->type == ASSEMBLY_SHARED_MANIFEST) &&
             assembly->no_inherit)
        return FALSE;

    while (ret)
    {
        if (xml_elem_cmp_end(&elem, assemblyW, asmv1W))
        {
            ret = parse_end_element(xmlbuf);
            break;
        }
        else if (xml_elem_cmp(&elem, descriptionW, asmv1W))
        {
            ret = parse_description_elem(xmlbuf);
        }
        else if (xml_elem_cmp(&elem, comInterfaceExternalProxyStubW, asmv1W))
        {
            ret = parse_com_interface_external_proxy_stub_elem(xmlbuf, assembly, acl);
        }
        else if (xml_elem_cmp(&elem, dependencyW, asmv1W))
        {
            ret = parse_dependency_elem(xmlbuf, acl);
        }
        else if (xml_elem_cmp(&elem, fileW, asmv1W))
        {
            ret = parse_file_elem(xmlbuf, assembly, acl);
        }
        else if (xml_elem_cmp(&elem, clrClassW, asmv1W))
        {
            ret = parse_clr_class_elem(xmlbuf, assembly, acl);
        }
        else if (xml_elem_cmp(&elem, clrSurrogateW, asmv1W))
        {
            ret = parse_clr_surrogate_elem(xmlbuf, assembly, acl);
        }
        else if (xml_elem_cmp(&elem, assemblyIdentityW, asmv1W))
        {
            if (!parse_assembly_identity_elem(xmlbuf, acl->actctx, &assembly->id)) return FALSE;

            if (expected_ai)
            {
                /* FIXME: more tests */
                if (assembly->type == ASSEMBLY_MANIFEST &&
                    memcmp(&assembly->id.version, &expected_ai->version, sizeof(assembly->id.version)))
                {
                    DPRINT1("wrong version for assembly manifest: %u.%u.%u.%u / %u.%u.%u.%u\n",
                          expected_ai->version.major, expected_ai->version.minor,
                          expected_ai->version.build, expected_ai->version.revision,
                          assembly->id.version.major, assembly->id.version.minor,
                          assembly->id.version.build, assembly->id.version.revision);
                    ret = FALSE;
                }
                else if (assembly->type == ASSEMBLY_SHARED_MANIFEST &&
                         (assembly->id.version.major != expected_ai->version.major ||
                          assembly->id.version.minor != expected_ai->version.minor ||
                          assembly->id.version.build < expected_ai->version.build ||
                          (assembly->id.version.build == expected_ai->version.build &&
                           assembly->id.version.revision < expected_ai->version.revision)))
                {
                    DPRINT1("wrong version for shared assembly manifest\n");
                    ret = FALSE;
                }
            }
        }
        else
        {
            attr_nameU = xmlstr2unicode(&elem);
            DPRINT1("unknown element %wZ\n", &attr_nameU);
            ret = parse_unknown_elem(xmlbuf, &elem);
        }
        if (ret) ret = next_xml_elem(xmlbuf, &elem);
    }

    return ret;
}

static NTSTATUS parse_manifest_buffer( struct actctx_loader* acl, struct assembly *assembly,
                                       struct assembly_identity* ai, xmlbuf_t *xmlbuf )
{
    xmlstr_t elem;
    UNICODE_STRING elemU;

    if (!next_xml_elem(xmlbuf, &elem)) return STATUS_SXS_CANT_GEN_ACTCTX;

    if (xmlstr_cmp(&elem, g_xmlW) &&
        (!parse_xml_header(xmlbuf) || !next_xml_elem(xmlbuf, &elem)))
        return STATUS_SXS_CANT_GEN_ACTCTX;

    if (!xml_elem_cmp(&elem, assemblyW, asmv1W))
    {
        elemU = xmlstr2unicode(&elem);
        DPRINT1("root element is %wZ, not <assembly>\n", &elemU);
        return STATUS_SXS_CANT_GEN_ACTCTX;
    }

    if (!parse_assembly_elem(xmlbuf, acl, assembly, ai))
    {
        DPRINT1("failed to parse manifest %S\n", assembly->manifest.info );
        return STATUS_SXS_CANT_GEN_ACTCTX;
    }

    if (next_xml_elem(xmlbuf, &elem))
    {
        elemU = xmlstr2unicode(&elem);
        DPRINT1("unexpected element %wZ\n", &elemU);
        return STATUS_SXS_CANT_GEN_ACTCTX;
    }

    if (xmlbuf->ptr != xmlbuf->end)
    {
        DPRINT1("parse error\n");
        return STATUS_SXS_CANT_GEN_ACTCTX;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS parse_manifest( struct actctx_loader* acl, struct assembly_identity* ai,
                                LPCWSTR filename, LPCWSTR directory, BOOL shared,
                                const void *buffer, SIZE_T size )
{
    xmlbuf_t xmlbuf;
    NTSTATUS status;
    struct assembly *assembly;
    int unicode_tests;

    DPRINT( "parsing manifest loaded from %S base dir %S\n", filename, directory );

    if (!(assembly = add_assembly(acl->actctx, shared ? ASSEMBLY_SHARED_MANIFEST : ASSEMBLY_MANIFEST)))
        return STATUS_SXS_CANT_GEN_ACTCTX;

    if (directory && !(assembly->directory = strdupW(directory)))
        return STATUS_NO_MEMORY;

    if (filename) assembly->manifest.info = strdupW( filename + 4 /* skip \??\ prefix */ );
    assembly->manifest.type = assembly->manifest.info ? ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE
                                                      : ACTIVATION_CONTEXT_PATH_TYPE_NONE;

    unicode_tests = IS_TEXT_UNICODE_SIGNATURE | IS_TEXT_UNICODE_REVERSE_SIGNATURE;
    if (RtlIsTextUnicode((PVOID)buffer, (ULONG)size, &unicode_tests ))
    {
        xmlbuf.ptr = buffer;
        xmlbuf.end = xmlbuf.ptr + size / sizeof(WCHAR);
        status = parse_manifest_buffer( acl, assembly, ai, &xmlbuf );
    }
    else if (unicode_tests & IS_TEXT_UNICODE_REVERSE_SIGNATURE)
    {
        const WCHAR *buf = buffer;
        WCHAR *new_buff;
        unsigned int i;

        if (!(new_buff = RtlAllocateHeap( RtlGetProcessHeap(), 0, size )))
            return STATUS_NO_MEMORY;
        for (i = 0; i < size / sizeof(WCHAR); i++)
            new_buff[i] = RtlUshortByteSwap( buf[i] );
        xmlbuf.ptr = new_buff;
        xmlbuf.end = xmlbuf.ptr + size / sizeof(WCHAR);
        status = parse_manifest_buffer( acl, assembly, ai, &xmlbuf );
        RtlFreeHeap( RtlGetProcessHeap(), 0, new_buff );
    }
    else
    {
        /* TODO: this doesn't handle arbitrary encodings */
        WCHAR *new_buff;
        ULONG sizeU;

        status = RtlMultiByteToUnicodeSize(&sizeU, buffer, size);
        if (!NT_SUCCESS(status))
        {
            DPRINT1("RtlMultiByteToUnicodeSize failed with %lx\n", status);
            return STATUS_SXS_CANT_GEN_ACTCTX;
        }

        new_buff = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeU);
        if (!new_buff)
            return STATUS_NO_MEMORY;

        status = RtlMultiByteToUnicodeN(new_buff, sizeU, &sizeU, buffer, size);
        if (!NT_SUCCESS(status))
        {
            DPRINT1("RtlMultiByteToUnicodeN failed with %lx\n", status);
            return STATUS_SXS_CANT_GEN_ACTCTX;
        }

        xmlbuf.ptr = new_buff;
        xmlbuf.end = xmlbuf.ptr + sizeU / sizeof(WCHAR);
        status = parse_manifest_buffer(acl, assembly, ai, &xmlbuf);
        RtlFreeHeap(RtlGetProcessHeap(), 0, new_buff);
    }
    return status;
}

static NTSTATUS open_nt_file( HANDLE *handle, UNICODE_STRING *name )
{
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    return NtOpenFile(handle,
                      GENERIC_READ | SYNCHRONIZE,
                      &attr, &io,
                      FILE_SHARE_READ,
                      FILE_SYNCHRONOUS_IO_ALERT);
}

static NTSTATUS get_module_filename( HMODULE module, UNICODE_STRING *str, USHORT extra_len )
{
    NTSTATUS status;
    ULONG_PTR magic;
    LDR_DATA_TABLE_ENTRY *pldr;

    LdrLockLoaderLock(0, NULL, &magic);
    status = LdrFindEntryForAddress( module, &pldr );
    if (status == STATUS_SUCCESS)
    {
        if ((str->Buffer = RtlAllocateHeap( RtlGetProcessHeap(), 0,
                                            pldr->FullDllName.Length + extra_len + sizeof(WCHAR) )))
        {
            memcpy( str->Buffer, pldr->FullDllName.Buffer, pldr->FullDllName.Length + sizeof(WCHAR) );
            str->Length = pldr->FullDllName.Length;
            str->MaximumLength = pldr->FullDllName.Length + extra_len + sizeof(WCHAR);
        }
        else status = STATUS_NO_MEMORY;
    }
    LdrUnlockLoaderLock(0, magic);
    return status;
}

static NTSTATUS get_manifest_in_module( struct actctx_loader* acl, struct assembly_identity* ai,
                                        LPCWSTR filename, LPCWSTR directory, BOOL shared,
                                        HANDLE hModule, LPCWSTR resname, ULONG lang )
{
    NTSTATUS status;
    UNICODE_STRING nameW;
    LDR_RESOURCE_INFO info;
    IMAGE_RESOURCE_DATA_ENTRY* entry = NULL;
    void *ptr;

    //DPRINT( "looking for res %s in module %p %s\n", resname,
    //                hModule, filename );

#if 0
    if (TRACE_ON(actctx))
    {
        if (!filename && !get_module_filename( hModule, &nameW, 0 ))
        {
            DPRINT( "looking for res %s in module %p %s\n", debugstr_w(resname),
                   hModule, debugstr_w(nameW.Buffer) );
            RtlFreeUnicodeString( &nameW );
        }
        else DPRINT( "looking for res %s in module %p %s\n", debugstr_w(resname),
                    hModule, debugstr_w(filename) );
    }
#endif

    if (!resname) return STATUS_INVALID_PARAMETER;

    info.Type = (ULONG_PTR)RT_MANIFEST;
    info.Language = lang;
    if (!((ULONG_PTR)resname >> 16))
    {
        info.Name = (ULONG_PTR)resname;
        status = LdrFindResource_U(hModule, &info, 3, &entry);
    }
    else if (resname[0] == '#')
    {
        ULONG value;
        RtlInitUnicodeString(&nameW, resname + 1);
        if (RtlUnicodeStringToInteger(&nameW, 10, &value) != STATUS_SUCCESS || HIWORD(value))
            return STATUS_INVALID_PARAMETER;
        info.Name = value;
        status = LdrFindResource_U(hModule, &info, 3, &entry);
    }
    else
    {
        RtlCreateUnicodeString(&nameW, resname);
        RtlUpcaseUnicodeString(&nameW, &nameW, FALSE);
        info.Name = (ULONG_PTR)nameW.Buffer;
        status = LdrFindResource_U(hModule, &info, 3, &entry);
        RtlFreeUnicodeString(&nameW);
    }
    if (status == STATUS_SUCCESS) status = LdrAccessResource(hModule, entry, &ptr, NULL);

    if (status == STATUS_SUCCESS)
        status = parse_manifest(acl, ai, filename, directory, shared, ptr, entry->Size);

    return status;
}

static NTSTATUS get_manifest_in_pe_file( struct actctx_loader* acl, struct assembly_identity* ai,
                                         LPCWSTR filename, LPCWSTR directory, BOOL shared,
                                         HANDLE file, LPCWSTR resname, ULONG lang )
{
    HANDLE              mapping;
    OBJECT_ATTRIBUTES   attr;
    LARGE_INTEGER       size;
    LARGE_INTEGER       offset;
    NTSTATUS            status;
    SIZE_T              count;
    void               *base;

    DPRINT( "looking for res %S in %S\n", resname, filename );

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = NULL;
    attr.Attributes               = OBJ_CASE_INSENSITIVE | OBJ_OPENIF;
    attr.SecurityDescriptor       = NULL;
    attr.SecurityQualityOfService = NULL;

    size.QuadPart = 0;
    status = NtCreateSection( &mapping, STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ,
                              &attr, &size, PAGE_READONLY, SEC_COMMIT, file );
    if (status != STATUS_SUCCESS) return status;

    offset.QuadPart = 0;
    count = 0;
    base = NULL;
    status = NtMapViewOfSection( mapping, NtCurrentProcess(), &base, 0, 0, &offset,
                                 &count, ViewShare, 0, PAGE_READONLY );
    NtClose( mapping );
    if (status != STATUS_SUCCESS) return status;

    if (RtlImageNtHeader(base)) /* we got a PE file */
    {
        HANDLE module = (HMODULE)((ULONG_PTR)base | 1);  /* make it a LOAD_LIBRARY_AS_DATAFILE handle */
        status = get_manifest_in_module( acl, ai, filename, directory, shared, module, resname, lang );
    }
    else status = STATUS_INVALID_IMAGE_FORMAT;

    NtUnmapViewOfSection( NtCurrentProcess(), base );
    return status;
}

static NTSTATUS get_manifest_in_manifest_file( struct actctx_loader* acl, struct assembly_identity* ai,
                                               LPCWSTR filename, LPCWSTR directory, BOOL shared, HANDLE file )
{
    FILE_STANDARD_INFORMATION info;
    IO_STATUS_BLOCK io;
    HANDLE              mapping;
    OBJECT_ATTRIBUTES   attr;
    LARGE_INTEGER       size;
    LARGE_INTEGER       offset;
    NTSTATUS            status;
    SIZE_T              count;
    void               *base;

    DPRINT( "loading manifest file %S\n", filename );

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = NULL;
    attr.Attributes               = OBJ_CASE_INSENSITIVE | OBJ_OPENIF;
    attr.SecurityDescriptor       = NULL;
    attr.SecurityQualityOfService = NULL;

    size.QuadPart = 0;
    status = NtCreateSection( &mapping, STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ,
                              &attr, &size, PAGE_READONLY, SEC_COMMIT, file );

    if (status != STATUS_SUCCESS) return status;

    offset.QuadPart = 0;
    count = 0;
    base = NULL;
    status = NtMapViewOfSection( mapping, NtCurrentProcess(), &base, 0, 0, &offset,
                                 &count, ViewShare, 0, PAGE_READONLY );
    NtClose( mapping );
    if (status != STATUS_SUCCESS) return status;

    /* Fixme: WINE uses FileEndOfFileInformation with NtQueryInformationFile. */
    status = NtQueryInformationFile( file, &io, &info, sizeof(info), FileStandardInformation);

    if (status == STATUS_SUCCESS)
        status = parse_manifest(acl, ai, filename, directory, shared, base, (SIZE_T)info.EndOfFile.QuadPart);

    NtUnmapViewOfSection( NtCurrentProcess(), base );
    return status;
}

/* try to load the .manifest file associated to the file */
static NTSTATUS get_manifest_in_associated_manifest( struct actctx_loader* acl, struct assembly_identity* ai,
                                                     LPCWSTR filename, LPCWSTR directory, HMODULE module, LPCWSTR resname )
{
    static const WCHAR fmtW[] = { '.','%','l','u',0 };
    WCHAR *buffer;
    NTSTATUS status;
    UNICODE_STRING nameW;
    HANDLE file;
    ULONG_PTR resid = (ULONG_PTR)CREATEPROCESS_MANIFEST_RESOURCE_ID;

    if (!((ULONG_PTR)resname >> 16)) resid = (ULONG_PTR)resname & 0xffff;

    DPRINT( "looking for manifest associated with %S id %lu\n", filename, resid );

    if (module) /* use the module filename */
    {
        UNICODE_STRING name;

        if (!(status = get_module_filename( module, &name, sizeof(dotManifestW) + 10*sizeof(WCHAR) )))
        {
            if (resid != 1) sprintfW( name.Buffer + strlenW(name.Buffer), fmtW, resid );
            strcatW( name.Buffer, dotManifestW );
            if (!RtlDosPathNameToNtPathName_U( name.Buffer, &nameW, NULL, NULL ))
                status = STATUS_RESOURCE_DATA_NOT_FOUND;
            RtlFreeUnicodeString( &name );
        }
        if (status) return status;
    }
    else
    {
        if (!(buffer = RtlAllocateHeap( RtlGetProcessHeap(), 0,
                                        (strlenW(filename) + 10) * sizeof(WCHAR) + sizeof(dotManifestW) )))
            return STATUS_NO_MEMORY;
        strcpyW( buffer, filename );
        if (resid != 1) sprintfW( buffer + strlenW(buffer), fmtW, resid );
        strcatW( buffer, dotManifestW );
        RtlInitUnicodeString( &nameW, buffer );
    }

    if (!open_nt_file( &file, &nameW ))
    {
        status = get_manifest_in_manifest_file( acl, ai, nameW.Buffer, directory, FALSE, file );
        NtClose( file );
    }
    else status = STATUS_RESOURCE_TYPE_NOT_FOUND;
    RtlFreeUnicodeString( &nameW );
    return status;
}

static WCHAR *lookup_manifest_file( HANDLE dir, struct assembly_identity *ai )
{
    static const WCHAR lookup_fmtW[] =
        {'%','s','_','%','s','_','%','s','_','%','u','.','%','u','.','*','.','*','_',
         '%','s','_','*','.','m','a','n','i','f','e','s','t',0};
    static const WCHAR wine_trailerW[] = {'d','e','a','d','b','e','e','f','.','m','a','n','i','f','e','s','t'};

    WCHAR *lookup, *ret = NULL;
    UNICODE_STRING lookup_us;
    IO_STATUS_BLOCK io;
    const WCHAR *lang = ai->language;
    unsigned int data_pos = 0, data_len;
    char buffer[8192];

    if (!(lookup = RtlAllocateHeap( RtlGetProcessHeap(), 0,
                                    (strlenW(ai->arch) + strlenW(ai->name)
                                     + strlenW(ai->public_key) + 20) * sizeof(WCHAR)
                                    + sizeof(lookup_fmtW) )))
        return NULL;

    if (!lang || !strcmpiW( lang, neutralW )) lang = wildcardW;
    sprintfW( lookup, lookup_fmtW, ai->arch, ai->name, ai->public_key,
              ai->version.major, ai->version.minor, lang );
    RtlInitUnicodeString( &lookup_us, lookup );

    NtQueryDirectoryFile( dir, 0, NULL, NULL, &io, buffer, sizeof(buffer),
                          FileBothDirectoryInformation, FALSE, &lookup_us, TRUE );
    if (io.Status == STATUS_SUCCESS)
    {
        ULONG min_build = ai->version.build, min_revision = ai->version.revision;
        FILE_BOTH_DIR_INFORMATION *dir_info;
        WCHAR *tmp;
        ULONG build, revision;

        data_len = (ULONG)io.Information;

        for (;;)
        {
            if (data_pos >= data_len)
            {
                NtQueryDirectoryFile( dir, 0, NULL, NULL, &io, buffer, sizeof(buffer),
                                      FileBothDirectoryInformation, FALSE, &lookup_us, FALSE );
                if (io.Status != STATUS_SUCCESS) break;
                data_len = (ULONG)io.Information;
                data_pos = 0;
            }
            dir_info = (FILE_BOTH_DIR_INFORMATION*)(buffer + data_pos);

            if (dir_info->NextEntryOffset) data_pos += dir_info->NextEntryOffset;
            else data_pos = data_len;

            tmp = (WCHAR *)dir_info->FileName + (strchrW(lookup, '*') - lookup);
            build = atoiW(tmp);
            if (build < min_build) continue;
            tmp = strchrW(tmp, '.') + 1;
            revision = atoiW(tmp);
            if (build == min_build && revision < min_revision) continue;
            tmp = strchrW(tmp, '_') + 1;
            tmp = strchrW(tmp, '_') + 1;
            if (dir_info->FileNameLength - (tmp - dir_info->FileName) * sizeof(WCHAR) == sizeof(wine_trailerW) &&
                !memicmpW( tmp, wine_trailerW, sizeof(wine_trailerW) / sizeof(WCHAR) ))
            {
                /* prefer a non-Wine manifest if we already have one */
                /* we'll still load the builtin dll if specified through DllOverrides */
                if (ret) continue;
            }
            else
            {
                min_build = build;
                min_revision = revision;
            }
            ai->version.build = build;
            ai->version.revision = revision;
            RtlFreeHeap( RtlGetProcessHeap(), 0, ret );
            if ((ret = RtlAllocateHeap( RtlGetProcessHeap(), 0, dir_info->FileNameLength + sizeof(WCHAR) )))
            {
                memcpy( ret, dir_info->FileName, dir_info->FileNameLength );
                ret[dir_info->FileNameLength/sizeof(WCHAR)] = 0;
            }
        }
    }
    else DPRINT1("no matching file for %S\n", lookup);
    RtlFreeHeap( RtlGetProcessHeap(), 0, lookup );
    return ret;
}

static NTSTATUS lookup_winsxs(struct actctx_loader* acl, struct assembly_identity* ai)
{
    struct assembly_identity    sxs_ai;
    UNICODE_STRING              path_us;
    OBJECT_ATTRIBUTES           attr;
    IO_STATUS_BLOCK             io;
    WCHAR *path, *file = NULL;
    HANDLE handle;

    static const WCHAR manifest_dirW[] =
        {'\\','w','i','n','s','x','s','\\','m','a','n','i','f','e','s','t','s',0};

    if (!ai->arch || !ai->name || !ai->public_key) return STATUS_NO_SUCH_FILE;

    if (!(path = RtlAllocateHeap( RtlGetProcessHeap(), 0,
                                  ((strlenW(SharedUserData->NtSystemRoot) + 1) *sizeof(WCHAR)) + sizeof(manifest_dirW) )))
        return STATUS_NO_MEMORY;

    memcpy( path, SharedUserData->NtSystemRoot, strlenW(SharedUserData->NtSystemRoot) * sizeof(WCHAR) );
    memcpy( path + strlenW(SharedUserData->NtSystemRoot), manifest_dirW, sizeof(manifest_dirW) );

    if (!RtlDosPathNameToNtPathName_U( path, &path_us, NULL, NULL ))
    {
        RtlFreeHeap( RtlGetProcessHeap(), 0, path );
        return STATUS_NO_SUCH_FILE;
    }
    RtlFreeHeap( RtlGetProcessHeap(), 0, path );

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &path_us;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    if (!NtOpenFile(&handle,
                    GENERIC_READ | SYNCHRONIZE,
                    &attr, &io,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT))
    {
        sxs_ai = *ai;
        file = lookup_manifest_file( handle, &sxs_ai );
        NtClose( handle );
    }
    if (!file)
    {
        RtlFreeUnicodeString( &path_us );
        return STATUS_NO_SUCH_FILE;
    }

    /* append file name to directory path */
    if (!(path = RtlReAllocateHeap( RtlGetProcessHeap(), 0, path_us.Buffer,
                                    path_us.Length + (strlenW(file) + 2) * sizeof(WCHAR) )))
    {
        RtlFreeHeap( RtlGetProcessHeap(), 0, file );
        RtlFreeUnicodeString( &path_us );
        return STATUS_NO_MEMORY;
    }

    path[path_us.Length/sizeof(WCHAR)] = '\\';
    strcpyW( path + path_us.Length/sizeof(WCHAR) + 1, file );
    RtlInitUnicodeString( &path_us, path );
    *strrchrW(file, '.') = 0;  /* remove .manifest extension */

    if (!open_nt_file( &handle, &path_us ))
    {
        io.Status = get_manifest_in_manifest_file(acl, &sxs_ai, path_us.Buffer, file, TRUE, handle);
        NtClose( handle );
    }
    else io.Status = STATUS_NO_SUCH_FILE;

    RtlFreeHeap( RtlGetProcessHeap(), 0, file );
    RtlFreeUnicodeString( &path_us );
    return io.Status;
}

static NTSTATUS lookup_assembly(struct actctx_loader* acl,
                                struct assembly_identity* ai)
{
    static const WCHAR dotDllW[] = {'.','d','l','l',0};
    unsigned int i;
    WCHAR *buffer, *p, *directory;
    NTSTATUS status;
    UNICODE_STRING nameW;
    HANDLE file;
    DWORD len;

    DPRINT( "looking for name=%S version=%u.%u.%u.%u arch=%S\n",
            ai->name, ai->version.major, ai->version.minor, ai->version.build, ai->version.revision, ai->arch );

    if ((status = lookup_winsxs(acl, ai)) != STATUS_NO_SUCH_FILE) return status;

    /* FIXME: add support for language specific lookup */

    len = max(RtlGetFullPathName_U(acl->actctx->assemblies->manifest.info, 0, NULL, NULL) / sizeof(WCHAR),
        strlenW(acl->actctx->appdir.info));

    nameW.Buffer = NULL;
    if (!(buffer = RtlAllocateHeap( RtlGetProcessHeap(), 0,
                                    (len + 2 * strlenW(ai->name) + 2) * sizeof(WCHAR) + sizeof(dotManifestW) )))
        return STATUS_NO_MEMORY;

    if (!(directory = build_assembly_dir( ai )))
    {
        RtlFreeHeap( RtlGetProcessHeap(), 0, buffer );
        return STATUS_NO_MEMORY;
    }

    /* Lookup in <dir>\name.dll
     *           <dir>\name.manifest
     *           <dir>\name\name.dll
     *           <dir>\name\name.manifest
     *
     * First 'appdir' is used as <dir>, if that failed
     * it tries application manifest file path.
     */
    strcpyW( buffer, acl->actctx->appdir.info );
    p = buffer + strlenW(buffer);
    for (i = 0; i < 4; i++)
    {
        if (i == 2)
        {
            struct assembly *assembly = acl->actctx->assemblies;
            if (!RtlGetFullPathName_U(assembly->manifest.info, len * sizeof(WCHAR), buffer, &p)) break;
        }
        else *p++ = '\\';

        strcpyW( p, ai->name );
        p += strlenW(p);

        strcpyW( p, dotDllW );
        if (RtlDosPathNameToNtPathName_U( buffer, &nameW, NULL, NULL ))
        {
            status = open_nt_file( &file, &nameW );
            if (!status)
            {
                status = get_manifest_in_pe_file( acl, ai, nameW.Buffer, directory, FALSE, file,
                                                  (LPCWSTR)CREATEPROCESS_MANIFEST_RESOURCE_ID, 0 );
                NtClose( file );
                break;
            }
            RtlFreeUnicodeString( &nameW );
        }

        strcpyW( p, dotManifestW );
        if (RtlDosPathNameToNtPathName_U( buffer, &nameW, NULL, NULL ))
        {
            status = open_nt_file( &file, &nameW );
            if (!status)
            {
                status = get_manifest_in_manifest_file( acl, ai, nameW.Buffer, directory, FALSE, file );
                NtClose( file );
                break;
            }
            RtlFreeUnicodeString( &nameW );
        }
        status = STATUS_SXS_ASSEMBLY_NOT_FOUND;
    }
    RtlFreeUnicodeString( &nameW );
    RtlFreeHeap( RtlGetProcessHeap(), 0, directory );
    RtlFreeHeap( RtlGetProcessHeap(), 0, buffer );
    return status;
}

static NTSTATUS parse_depend_manifests(struct actctx_loader* acl)
{
    NTSTATUS status = STATUS_SUCCESS;
    unsigned int i;

    for (i = 0; i < acl->num_dependencies; i++)
    {
        if (lookup_assembly(acl, &acl->dependencies[i]) != STATUS_SUCCESS)
        {
            if (!acl->dependencies[i].optional && !acl->dependencies[i].delayed)
            {
                const struct assembly_version *ver = &acl->dependencies[i].version;
                DPRINT1( "Could not find dependent assembly %S (%u.%u.%u.%u)\n",
                    acl->dependencies[i].name,
                    ver->major, ver->minor, ver->build, ver->revision );
                status = STATUS_SXS_CANT_GEN_ACTCTX;
                break;
            }
        }
    }
    /* FIXME should now iterate through all refs */
    return status;
}

/* find the appropriate activation context for RtlQueryInformationActivationContext */
static NTSTATUS find_query_actctx( HANDLE *handle, DWORD flags, ULONG class )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (flags & RTL_QUERY_ACTIVATION_CONTEXT_FLAG_USE_ACTIVE_ACTIVATION_CONTEXT)
    {
        if (*handle) return STATUS_INVALID_PARAMETER;

        if (NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame)
            *handle = NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame->ActivationContext;
    }
    else if (flags & (RTL_QUERY_ACTIVATION_CONTEXT_FLAG_IS_ADDRESS | RTL_QUERY_ACTIVATION_CONTEXT_FLAG_IS_HMODULE))
    {
        ULONG_PTR magic;
        LDR_DATA_TABLE_ENTRY *pldr;

        if (!*handle) return STATUS_INVALID_PARAMETER;

        LdrLockLoaderLock( 0, NULL, &magic );
        if (!LdrFindEntryForAddress( *handle, &pldr ))
        {
            if ((flags & RTL_QUERY_ACTIVATION_CONTEXT_FLAG_IS_HMODULE) && *handle != pldr->DllBase)
                status = STATUS_DLL_NOT_FOUND;
            else
                *handle = pldr->EntryPointActivationContext;
        }
        else status = STATUS_DLL_NOT_FOUND;
        LdrUnlockLoaderLock( 0, magic );
    }
    else if (!*handle && (class != ActivationContextBasicInformation))
        *handle = process_actctx;

    return status;
}

static NTSTATUS build_dllredirect_section(ACTIVATION_CONTEXT* actctx, struct strsection_header **section)
{
    unsigned int i, j, total_len = 0, dll_count = 0;
    struct strsection_header *header;
    struct dllredirect_data *data;
    struct string_index *index;
    ULONG name_offset;

    DPRINT("actctx %p, num_assemblies %d\n", actctx, actctx->num_assemblies);

    /* compute section length */
    for (i = 0; i < actctx->num_assemblies; i++)
    {
        struct assembly *assembly = &actctx->assemblies[i];
        for (j = 0; j < assembly->num_dlls; j++)
        {
            struct dll_redirect *dll = &assembly->dlls[j];

            /* each entry needs index, data and string data */
            total_len += sizeof(*index);
            total_len += sizeof(*data);
            total_len += aligned_string_len((strlenW(dll->name)+1)*sizeof(WCHAR));

            DPRINT("assembly %d, dll %d: dll name %S\n", i, j, dll->name);
        }

        dll_count += assembly->num_dlls;
    }

    total_len += sizeof(*header);

    header = RtlAllocateHeap(RtlGetProcessHeap(), 0, total_len);
    if (!header) return STATUS_NO_MEMORY;

    memset(header, 0, sizeof(*header));
    header->magic = STRSECTION_MAGIC;
    header->size  = sizeof(*header);
    header->count = dll_count;
    header->index_offset = sizeof(*header);
    index = (struct string_index*)((BYTE*)header + header->index_offset);
    name_offset = header->index_offset + header->count*sizeof(*index);

    for (i = 0; i < actctx->num_assemblies; i++)
    {
        struct assembly *assembly = &actctx->assemblies[i];

        DPRINT("assembly->num_dlls %d\n", assembly->num_dlls);

        for (j = 0; j < assembly->num_dlls; j++)
        {
            struct dll_redirect *dll = &assembly->dlls[j];
            UNICODE_STRING str;
            WCHAR *ptrW;

            DPRINT("%d: dll name %S\n", j, dll->name);
            /* setup new index entry */
            str.Buffer = dll->name;
            str.Length = strlenW(dll->name)*sizeof(WCHAR);
            str.MaximumLength = str.Length + sizeof(WCHAR);
            /* hash original class name */
            RtlHashUnicodeString(&str, TRUE, HASH_STRING_ALGORITHM_X65599, &index->hash);

            index->name_offset = name_offset;
            index->name_len = str.Length;
            index->data_offset = index->name_offset + aligned_string_len(str.MaximumLength);
            index->data_len = sizeof(*data);
            index->rosterindex = i + 1;

            /* setup data */
            data = (struct dllredirect_data*)((BYTE*)header + index->data_offset);
            data->size = sizeof(*data);
            data->unk = 2; /* FIXME: seems to be constant */
            memset(data->res, 0, sizeof(data->res));

            /* dll name */
            ptrW = (WCHAR*)((BYTE*)header + index->name_offset);
            memcpy(ptrW, dll->name, index->name_len);
            ptrW[index->name_len/sizeof(WCHAR)] = 0;

            name_offset += sizeof(*data) + aligned_string_len(str.MaximumLength);

            index++;
        }
    }

    *section = header;

    return STATUS_SUCCESS;
}

static struct string_index *find_string_index(const struct strsection_header *section, const UNICODE_STRING *name)
{
    struct string_index *iter, *index = NULL;
    ULONG hash = 0, i;

    DPRINT("section %p, name %wZ\n", section, name);
    RtlHashUnicodeString(name, TRUE, HASH_STRING_ALGORITHM_X65599, &hash);
    iter = (struct string_index*)((BYTE*)section + section->index_offset);

    for (i = 0; i < section->count; i++)
    {
        DPRINT("iter->hash 0x%x ?= 0x%x\n", iter->hash, hash);
        DPRINT("iter->name %S\n", (WCHAR*)((BYTE*)section + iter->name_offset));
        if (iter->hash == hash)
        {
            const WCHAR *nameW = (WCHAR*)((BYTE*)section + iter->name_offset);

            if (!strcmpiW(nameW, name->Buffer))
            {
                index = iter;
                break;
            }
            else
                DPRINT1("hash collision 0x%08x, %wZ, %S\n", hash, name, nameW);
        }
        iter++;
    }

    return index;
}

static struct guid_index *find_guid_index(const struct guidsection_header *section, const GUID *guid)
{
    struct guid_index *iter, *index = NULL;
    ULONG i;

    iter = (struct guid_index*)((BYTE*)section + section->index_offset);

    for (i = 0; i < section->count; i++)
    {
        if (!memcmp(guid, &iter->guid, sizeof(*guid)))
        {
            index = iter;
            break;
        }
        iter++;
    }

    return index;
}

static inline struct dllredirect_data *get_dllredirect_data(ACTIVATION_CONTEXT *ctxt, struct string_index *index)
{
    return (struct dllredirect_data*)((BYTE*)ctxt->dllredirect_section + index->data_offset);
}

static NTSTATUS find_dll_redirection(ACTIVATION_CONTEXT* actctx, const UNICODE_STRING *name,
                                     PACTCTX_SECTION_KEYED_DATA data)
{
    struct dllredirect_data *dll;
    struct string_index *index;

    DPRINT("sections: 0x%08X\n", actctx->sections);
    if (!(actctx->sections & DLLREDIRECT_SECTION)) return STATUS_SXS_KEY_NOT_FOUND;

    DPRINT("actctx->dllredirect_section: %p\n", actctx->dllredirect_section);
    if (!actctx->dllredirect_section)
    {
        struct strsection_header *section;

        NTSTATUS status = build_dllredirect_section(actctx, &section);
        if (status) return status;

        if (InterlockedCompareExchangePointer((void**)&actctx->dllredirect_section, section, NULL))
            RtlFreeHeap(RtlGetProcessHeap(), 0, section);
    }

    index = find_string_index(actctx->dllredirect_section, name);
    DPRINT("index: %d\n", index);
    if (!index) return STATUS_SXS_KEY_NOT_FOUND;

    dll = get_dllredirect_data(actctx, index);

    data->ulDataFormatVersion = 1;
    data->lpData = dll;
    data->ulLength = dll->size;
    data->lpSectionGlobalData = NULL;
    data->ulSectionGlobalDataLength = 0;
    data->lpSectionBase = actctx->dllredirect_section;
    data->ulSectionTotalLength = RtlSizeHeap( RtlGetProcessHeap(), 0, actctx->dllredirect_section );
    data->hActCtx = NULL;

    if (data->cbSize >= FIELD_OFFSET(ACTCTX_SECTION_KEYED_DATA, ulAssemblyRosterIndex) + sizeof(ULONG))
        data->ulAssemblyRosterIndex = index->rosterindex;

    return STATUS_SUCCESS;
}

static inline struct string_index *get_wndclass_first_index(ACTIVATION_CONTEXT *actctx)
{
    return (struct string_index*)((BYTE*)actctx->wndclass_section + actctx->wndclass_section->index_offset);
}

static inline struct wndclass_redirect_data *get_wndclass_data(ACTIVATION_CONTEXT *ctxt, struct string_index *index)
{
    return (struct wndclass_redirect_data*)((BYTE*)ctxt->wndclass_section + index->data_offset);
}

static NTSTATUS build_wndclass_section(ACTIVATION_CONTEXT* actctx, struct strsection_header **section)
{
    unsigned int i, j, k, total_len = 0, class_count = 0;
    struct wndclass_redirect_data *data;
    struct strsection_header *header;
    struct string_index *index;
    ULONG name_offset;

    /* compute section length */
    for (i = 0; i < actctx->num_assemblies; i++)
    {
        struct assembly *assembly = &actctx->assemblies[i];
        for (j = 0; j < assembly->num_dlls; j++)
        {
            struct dll_redirect *dll = &assembly->dlls[j];
            for (k = 0; k < dll->entities.num; k++)
            {
                struct entity *entity = &dll->entities.base[k];
                if (entity->kind == ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION)
                {
                    int class_len = strlenW(entity->u.class.name) + 1;
                    int len;

                    /* each class entry needs index, data and string data */
                    total_len += sizeof(*index);
                    total_len += sizeof(*data);
                    /* original name is stored separately */
                    total_len += aligned_string_len(class_len*sizeof(WCHAR));
                    /* versioned name and module name are stored one after another */
                    if (entity->u.class.versioned)
                        len = get_assembly_version(assembly, NULL) + class_len + 1 /* '!' separator */;
                    else
                        len = class_len;
                    len += strlenW(dll->name) + 1;
                    total_len += aligned_string_len(len*sizeof(WCHAR));

                    class_count++;
                }
            }
        }
    }

    total_len += sizeof(*header);

    header = RtlAllocateHeap(RtlGetProcessHeap(), 0, total_len);
    if (!header) return STATUS_NO_MEMORY;

    memset(header, 0, sizeof(*header));
    header->magic = STRSECTION_MAGIC;
    header->size  = sizeof(*header);
    header->count = class_count;
    header->index_offset = sizeof(*header);
    index = (struct string_index*)((BYTE*)header + header->index_offset);
    name_offset = header->index_offset + header->count*sizeof(*index);

    for (i = 0; i < actctx->num_assemblies; i++)
    {
        struct assembly *assembly = &actctx->assemblies[i];
        for (j = 0; j < assembly->num_dlls; j++)
        {
            struct dll_redirect *dll = &assembly->dlls[j];
            for (k = 0; k < dll->entities.num; k++)
            {
                struct entity *entity = &dll->entities.base[k];
                if (entity->kind == ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION)
                {
                    static const WCHAR exclW[] = {'!',0};
                    ULONG versioned_len, module_len;
                    UNICODE_STRING str;
                    WCHAR *ptrW;

                    /* setup new index entry */
                    str.Buffer = entity->u.class.name;
                    str.Length = strlenW(entity->u.class.name)*sizeof(WCHAR);
                    str.MaximumLength = str.Length + sizeof(WCHAR);
                    /* hash original class name */
                    RtlHashUnicodeString(&str, TRUE, HASH_STRING_ALGORITHM_X65599, &index->hash);

                    /* include '!' separator too */
                    if (entity->u.class.versioned)
                        versioned_len = (get_assembly_version(assembly, NULL) + 1)*sizeof(WCHAR) + str.Length;
                    else
                        versioned_len = str.Length;
                    module_len = strlenW(dll->name)*sizeof(WCHAR);

                    index->name_offset = name_offset;
                    index->name_len = str.Length;
                    index->data_offset = index->name_offset + aligned_string_len(str.MaximumLength);
                    index->data_len = sizeof(*data) + versioned_len + module_len + 2*sizeof(WCHAR) /* two nulls */;
                    index->rosterindex = i + 1;

                    /* setup data */
                    data = (struct wndclass_redirect_data*)((BYTE*)header + index->data_offset);
                    data->size = sizeof(*data);
                    data->res = 0;
                    data->name_len = versioned_len;
                    data->name_offset = sizeof(*data);
                    data->module_len = module_len;
                    data->module_offset = index->data_offset + data->name_offset + data->name_len + sizeof(WCHAR);

                    /* original class name */
                    ptrW = (WCHAR*)((BYTE*)header + index->name_offset);
                    memcpy(ptrW, entity->u.class.name, index->name_len);
                    ptrW[index->name_len/sizeof(WCHAR)] = 0;

                    /* module name */
                    ptrW = (WCHAR*)((BYTE*)header + data->module_offset);
                    memcpy(ptrW, dll->name, data->module_len);
                    ptrW[data->module_len/sizeof(WCHAR)] = 0;

                    /* versioned name */
                    ptrW = (WCHAR*)((BYTE*)data + data->name_offset);
                    if (entity->u.class.versioned)
                    {
                        get_assembly_version(assembly, ptrW);
                        strcatW(ptrW, exclW);
                        strcatW(ptrW, entity->u.class.name);
                    }
                    else
                    {
                        memcpy(ptrW, entity->u.class.name, index->name_len);
                        ptrW[index->name_len/sizeof(WCHAR)] = 0;
                    }

                    name_offset += sizeof(*data);
                    name_offset += aligned_string_len(str.MaximumLength) + aligned_string_len(versioned_len + module_len + 2*sizeof(WCHAR));

                    index++;
                }
            }
        }
    }

    *section = header;

    return STATUS_SUCCESS;
}

static NTSTATUS find_window_class(ACTIVATION_CONTEXT* actctx, const UNICODE_STRING *name,
                                  PACTCTX_SECTION_KEYED_DATA data)
{
    struct string_index *iter, *index = NULL;
    struct wndclass_redirect_data *class;
    ULONG hash;
    int i;

    if (!(actctx->sections & WINDOWCLASS_SECTION)) return STATUS_SXS_KEY_NOT_FOUND;

    if (!actctx->wndclass_section)
    {
        struct strsection_header *section;

        NTSTATUS status = build_wndclass_section(actctx, &section);
        if (status) return status;

        if (InterlockedCompareExchangePointer((void**)&actctx->wndclass_section, section, NULL))
            RtlFreeHeap(RtlGetProcessHeap(), 0, section);
    }

    hash = 0;
    RtlHashUnicodeString(name, TRUE, HASH_STRING_ALGORITHM_X65599, &hash);
    iter = get_wndclass_first_index(actctx);

    for (i = 0; i < actctx->wndclass_section->count; i++)
    {
        if (iter->hash == hash)
        {
            const WCHAR *nameW = (WCHAR*)((BYTE*)actctx->wndclass_section + iter->name_offset);

            if (!strcmpW(nameW, name->Buffer))
            {
                index = iter;
                break;
            }
            else
                DPRINT1("hash collision 0x%08x, %wZ, %S\n", hash, name, nameW);
        }
        iter++;
    }

    if (!index) return STATUS_SXS_KEY_NOT_FOUND;

    class = get_wndclass_data(actctx, index);

    data->ulDataFormatVersion = 1;
    data->lpData = class;
    /* full length includes string length with nulls */
    data->ulLength = class->size + class->name_len + class->module_len + 2*sizeof(WCHAR);
    data->lpSectionGlobalData = NULL;
    data->ulSectionGlobalDataLength = 0;
    data->lpSectionBase = actctx->wndclass_section;
    data->ulSectionTotalLength = RtlSizeHeap( RtlGetProcessHeap(), 0, actctx->wndclass_section );
    data->hActCtx = NULL;

    if (data->cbSize >= FIELD_OFFSET(ACTCTX_SECTION_KEYED_DATA, ulAssemblyRosterIndex) + sizeof(ULONG))
        data->ulAssemblyRosterIndex = index->rosterindex;

    return STATUS_SUCCESS;
}

static NTSTATUS build_tlib_section(ACTIVATION_CONTEXT* actctx, struct guidsection_header **section)
{
    unsigned int i, j, k, total_len = 0, tlib_count = 0, names_len = 0;
    struct guidsection_header *header;
    ULONG module_offset, data_offset;
    struct tlibredirect_data *data;
    struct guid_index *index;

    /* compute section length */
    for (i = 0; i < actctx->num_assemblies; i++)
    {
        struct assembly *assembly = &actctx->assemblies[i];
        for (j = 0; j < assembly->num_dlls; j++)
        {
            struct dll_redirect *dll = &assembly->dlls[j];
            for (k = 0; k < dll->entities.num; k++)
            {
                struct entity *entity = &dll->entities.base[k];
                if (entity->kind == ACTIVATION_CONTEXT_SECTION_COM_TYPE_LIBRARY_REDIRECTION)
                {
                    /* each entry needs index, data and string data for module name and help string */
                    total_len += sizeof(*index);
                    total_len += sizeof(*data);
                    /* help string is stored separately */
                    if (*entity->u.typelib.helpdir)
                        total_len += aligned_string_len((strlenW(entity->u.typelib.helpdir)+1)*sizeof(WCHAR));

                    /* module names are packed one after another */
                    names_len += (strlenW(dll->name)+1)*sizeof(WCHAR);

                    tlib_count++;
                }
            }
        }
    }

    total_len += aligned_string_len(names_len);
    total_len += sizeof(*header);

    header = RtlAllocateHeap(RtlGetProcessHeap(), 0, total_len);
    if (!header) return STATUS_NO_MEMORY;

    memset(header, 0, sizeof(*header));
    header->magic = GUIDSECTION_MAGIC;
    header->size  = sizeof(*header);
    header->count = tlib_count;
    header->index_offset = sizeof(*header) + aligned_string_len(names_len);
    index = (struct guid_index*)((BYTE*)header + header->index_offset);
    module_offset = sizeof(*header);
    data_offset = header->index_offset + tlib_count*sizeof(*index);

    for (i = 0; i < actctx->num_assemblies; i++)
    {
        struct assembly *assembly = &actctx->assemblies[i];
        for (j = 0; j < assembly->num_dlls; j++)
        {
            struct dll_redirect *dll = &assembly->dlls[j];
            for (k = 0; k < dll->entities.num; k++)
            {
                struct entity *entity = &dll->entities.base[k];
                if (entity->kind == ACTIVATION_CONTEXT_SECTION_COM_TYPE_LIBRARY_REDIRECTION)
                {
                    ULONG module_len, help_len;
                    UNICODE_STRING str;
                    WCHAR *ptrW;

                    if (*entity->u.typelib.helpdir)
                        help_len = strlenW(entity->u.typelib.helpdir)*sizeof(WCHAR);
                    else
                        help_len = 0;

                    module_len = strlenW(dll->name)*sizeof(WCHAR);

                    /* setup new index entry */
                    RtlInitUnicodeString(&str, entity->u.typelib.tlbid);
                    RtlGUIDFromString(&str, &index->guid);
                    index->data_offset = data_offset;
                    index->data_len = sizeof(*data) + aligned_string_len(help_len);
                    index->rosterindex = i + 1;

                    /* setup data */
                    data = (struct tlibredirect_data*)((BYTE*)header + index->data_offset);
                    data->size = sizeof(*data);
                    data->res = 0;
                    data->name_len = module_len;
                    data->name_offset = module_offset;
                    /* FIXME: resourceid handling is really weird, and it doesn't seem to be useful */
                    data->langid = 0;
                    data->flags = entity->u.typelib.flags;
                    data->help_len = help_len;
                    data->help_offset = sizeof(*data);
                    data->major_version = entity->u.typelib.major;
                    data->minor_version = entity->u.typelib.minor;

                    /* module name */
                    ptrW = (WCHAR*)((BYTE*)header + data->name_offset);
                    memcpy(ptrW, dll->name, data->name_len);
                    ptrW[data->name_len/sizeof(WCHAR)] = 0;

                    /* help string */
                    if (data->help_len)
                    {
                        ptrW = (WCHAR*)((BYTE*)data + data->help_offset);
                        memcpy(ptrW, entity->u.typelib.helpdir, data->help_len);
                        ptrW[data->help_len/sizeof(WCHAR)] = 0;
                    }

                    data_offset += sizeof(*data);
                    if (help_len)
                        data_offset += aligned_string_len(help_len + sizeof(WCHAR));

                    module_offset += module_len + sizeof(WCHAR);

                    index++;
                }
            }
        }
    }

    *section = header;

    return STATUS_SUCCESS;
}

static inline struct tlibredirect_data *get_tlib_data(ACTIVATION_CONTEXT *actctx, struct guid_index *index)
{
    return (struct tlibredirect_data*)((BYTE*)actctx->tlib_section + index->data_offset);
}

static NTSTATUS find_tlib_redirection(ACTIVATION_CONTEXT* actctx, const GUID *guid, ACTCTX_SECTION_KEYED_DATA* data)
{
    struct guid_index *index = NULL;
    struct tlibredirect_data *tlib;

    if (!(actctx->sections & TLIBREDIRECT_SECTION)) return STATUS_SXS_KEY_NOT_FOUND;

    if (!actctx->tlib_section)
    {
        struct guidsection_header *section;

        NTSTATUS status = build_tlib_section(actctx, &section);
        if (status) return status;

        if (InterlockedCompareExchangePointer((void**)&actctx->tlib_section, section, NULL))
            RtlFreeHeap(RtlGetProcessHeap(), 0, section);
    }

    index = find_guid_index(actctx->tlib_section, guid);
    if (!index) return STATUS_SXS_KEY_NOT_FOUND;

    tlib = get_tlib_data(actctx, index);

    data->ulDataFormatVersion = 1;
    data->lpData = tlib;
    /* full length includes string length with nulls */
    data->ulLength = tlib->size + tlib->help_len + sizeof(WCHAR);
    data->lpSectionGlobalData = (BYTE*)actctx->tlib_section + actctx->tlib_section->names_offset;
    data->ulSectionGlobalDataLength = actctx->tlib_section->names_len;
    data->lpSectionBase = actctx->tlib_section;
    data->ulSectionTotalLength = RtlSizeHeap( RtlGetProcessHeap(), 0, actctx->tlib_section );
    data->hActCtx = NULL;

    if (data->cbSize >= FIELD_OFFSET(ACTCTX_SECTION_KEYED_DATA, ulAssemblyRosterIndex) + sizeof(ULONG))
        data->ulAssemblyRosterIndex = index->rosterindex;

    return STATUS_SUCCESS;
}

static void generate_uuid(ULONG *seed, GUID *guid)
{
    ULONG *ptr = (ULONG*)guid;
    int i;

    /* GUID is 16 bytes long */
    for (i = 0; i < sizeof(GUID)/sizeof(ULONG); i++, ptr++)
        *ptr = RtlUniform(seed);

    guid->Data3 &= 0x0fff;
    guid->Data3 |= (4 << 12);
    guid->Data4[0] &= 0x3f;
    guid->Data4[0] |= 0x80;
}

static void get_comserver_datalen(const struct entity_array *entities, const struct dll_redirect *dll,
    unsigned int *count, unsigned int *len, unsigned int *module_len)
{
    unsigned int i;

    for (i = 0; i < entities->num; i++)
    {
        struct entity *entity = &entities->base[i];
        if (entity->kind == ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION)
        {
            /* each entry needs two index entries, extra one goes for alias GUID */
            *len += 2*sizeof(struct guid_index);
            /* To save some memory we don't allocated two data structures,
               instead alias index and normal index point to the same data structure. */
            *len += sizeof(struct comclassredirect_data);

            /* for clrClass store some more */
            if (entity->u.comclass.name)
            {
                unsigned int str_len;

                /* all string data is stored together in aligned block */
                str_len = strlenW(entity->u.comclass.name)+1;
                if (entity->u.comclass.progid)
                    str_len += strlenW(entity->u.comclass.progid)+1;
                if (entity->u.comclass.version)
                    str_len += strlenW(entity->u.comclass.version)+1;

                *len += sizeof(struct clrclass_data);
                *len += aligned_string_len(str_len*sizeof(WCHAR));

                /* module name is forced to mscoree.dll, and stored two times with different case */
                *module_len += sizeof(mscoreeW) + sizeof(mscoree2W);
            }
            else
            {
                /* progid string is stored separately */
                if (entity->u.comclass.progid)
                    *len += aligned_string_len((strlenW(entity->u.comclass.progid)+1)*sizeof(WCHAR));

                *module_len += (strlenW(dll->name)+1)*sizeof(WCHAR);
            }

            *count += 1;
        }
    }
}

static void add_comserver_record(const struct guidsection_header *section, const struct entity_array *entities,
    const struct dll_redirect *dll, struct guid_index **index, ULONG *data_offset, ULONG *module_offset,
    ULONG *seed, ULONG rosterindex)
{
    unsigned int i;

    for (i = 0; i < entities->num; i++)
    {
        struct entity *entity = &entities->base[i];
        if (entity->kind == ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION)
        {
            ULONG module_len, progid_len, str_len = 0;
            struct comclassredirect_data *data;
            struct guid_index *alias_index;
            struct clrclass_data *clrdata;
            UNICODE_STRING str;
            WCHAR *ptrW;

            if (entity->u.comclass.progid)
                progid_len = strlenW(entity->u.comclass.progid)*sizeof(WCHAR);
            else
                progid_len = 0;

            module_len = dll ? strlenW(dll->name)*sizeof(WCHAR) : strlenW(mscoreeW)*sizeof(WCHAR);

            /* setup new index entry */
            RtlInitUnicodeString(&str, entity->u.comclass.clsid);
            RtlGUIDFromString(&str, &(*index)->guid);

            (*index)->data_offset = *data_offset;
            (*index)->data_len = sizeof(*data); /* additional length added later */
            (*index)->rosterindex = rosterindex;

            /* Setup new index entry for alias guid. Alias index records are placed after
               normal records, so normal guids are hit first on search. Note that class count
               is doubled. */
            alias_index = (*index) + section->count/2;
            generate_uuid(seed, &alias_index->guid);
            alias_index->data_offset = (*index)->data_offset;
            alias_index->data_len = 0;
            alias_index->rosterindex = (*index)->rosterindex;

            /* setup data */
            data = (struct comclassredirect_data*)((BYTE*)section + (*index)->data_offset);
            data->size = sizeof(*data);
            data->res = 0;
            data->res1[0] = 0;
            data->res1[1] = 0;
            data->model = entity->u.comclass.model;
            data->clsid = (*index)->guid;
            data->alias = alias_index->guid;
            data->clsid2 = data->clsid;
            if (entity->u.comclass.tlbid)
            {
                RtlInitUnicodeString(&str, entity->u.comclass.tlbid);
                RtlGUIDFromString(&str, &data->tlbid);
            }
            else
                memset(&data->tlbid, 0, sizeof(data->tlbid));
            data->name_len = module_len;
            data->name_offset = *module_offset;
            data->progid_len = progid_len;
            data->progid_offset = data->progid_len ? data->size : 0; /* in case of clrClass additional offset is added later */
            data->clrdata_len = 0; /* will be set later */
            data->clrdata_offset = entity->u.comclass.name ? sizeof(*data) : 0;
            data->miscstatus = entity->u.comclass.miscstatus;
            data->miscstatuscontent = entity->u.comclass.miscstatuscontent;
            data->miscstatusthumbnail = entity->u.comclass.miscstatusthumbnail;
            data->miscstatusicon = entity->u.comclass.miscstatusicon;
            data->miscstatusdocprint = entity->u.comclass.miscstatusdocprint;

            /* mask describes which misc* data is available */
            data->miscmask = 0;
            if (data->miscstatus)
                data->miscmask |= MiscStatus;
            if (data->miscstatuscontent)
                data->miscmask |= MiscStatusContent;
            if (data->miscstatusthumbnail)
                data->miscmask |= MiscStatusThumbnail;
            if (data->miscstatusicon)
                data->miscmask |= MiscStatusIcon;
            if (data->miscstatusdocprint)
                data->miscmask |= MiscStatusDocPrint;

            if (data->clrdata_offset)
            {
                clrdata = (struct clrclass_data*)((BYTE*)data + data->clrdata_offset);

                clrdata->size = sizeof(*clrdata);
                clrdata->res[0] = 0;
                clrdata->res[1] = 2; /* FIXME: unknown field */
                clrdata->module_len = strlenW(mscoreeW)*sizeof(WCHAR);
                clrdata->module_offset = *module_offset + data->name_len + sizeof(WCHAR);
                clrdata->name_len = strlenW(entity->u.comclass.name)*sizeof(WCHAR);
                clrdata->name_offset = clrdata->size;
                clrdata->version_len = entity->u.comclass.version ? strlenW(entity->u.comclass.version)*sizeof(WCHAR) : 0;
                clrdata->version_offset = clrdata->version_len ? clrdata->name_offset + clrdata->name_len + sizeof(WCHAR) : 0;
                clrdata->res2[0] = 0;
                clrdata->res2[1] = 0;

                data->clrdata_len = clrdata->size + clrdata->name_len + sizeof(WCHAR);

                /* module name */
                ptrW = (WCHAR*)((BYTE*)section + clrdata->module_offset);
                memcpy(ptrW, mscoree2W, clrdata->module_len);
                ptrW[clrdata->module_len/sizeof(WCHAR)] = 0;

                ptrW = (WCHAR*)((BYTE*)section + data->name_offset);
                memcpy(ptrW, mscoreeW, data->name_len);
                ptrW[data->name_len/sizeof(WCHAR)] = 0;

                /* class name */
                ptrW = (WCHAR*)((BYTE*)clrdata + clrdata->name_offset);
                memcpy(ptrW, entity->u.comclass.name, clrdata->name_len);
                ptrW[clrdata->name_len/sizeof(WCHAR)] = 0;

                /* runtime version, optional */
                if (clrdata->version_len)
                {
                    data->clrdata_len += clrdata->version_len + sizeof(WCHAR);

                    ptrW = (WCHAR*)((BYTE*)clrdata + clrdata->version_offset);
                    memcpy(ptrW, entity->u.comclass.version, clrdata->version_len);
                    ptrW[clrdata->version_len/sizeof(WCHAR)] = 0;
                }

                if (data->progid_len)
                    data->progid_offset += data->clrdata_len;
                (*index)->data_len += sizeof(*clrdata);
            }
            else
            {
                clrdata = NULL;

                /* module name */
                ptrW = (WCHAR*)((BYTE*)section + data->name_offset);
                memcpy(ptrW, dll->name, data->name_len);
                ptrW[data->name_len/sizeof(WCHAR)] = 0;
            }

            /* progid string */
            if (data->progid_len)
            {
                ptrW = (WCHAR*)((BYTE*)data + data->progid_offset);
                memcpy(ptrW, entity->u.comclass.progid, data->progid_len);
                ptrW[data->progid_len/sizeof(WCHAR)] = 0;
            }

            /* string block length */
            str_len = 0;
            if (clrdata)
            {
                str_len += clrdata->name_len + sizeof(WCHAR);
                if (clrdata->version_len)
                    str_len += clrdata->version_len + sizeof(WCHAR);
            }
            if (progid_len)
                str_len += progid_len + sizeof(WCHAR);

            (*index)->data_len += aligned_string_len(str_len);
            alias_index->data_len = (*index)->data_len;

            /* move to next data record */
            (*data_offset) += sizeof(*data) + aligned_string_len(str_len);
            (*module_offset) += module_len + sizeof(WCHAR);

            if (clrdata)
            {
                (*data_offset) += sizeof(*clrdata);
                (*module_offset) += clrdata->module_len + sizeof(WCHAR);
            }
            (*index) += 1;
        }
    }
}

static NTSTATUS build_comserver_section(ACTIVATION_CONTEXT* actctx, struct guidsection_header **section)
{
    unsigned int i, j, total_len = 0, class_count = 0, names_len = 0;
    struct guidsection_header *header;
    ULONG module_offset, data_offset;
    struct guid_index *index;
    ULONG seed;

    /* compute section length */
    for (i = 0; i < actctx->num_assemblies; i++)
    {
        struct assembly *assembly = &actctx->assemblies[i];
        get_comserver_datalen(&assembly->entities, NULL, &class_count, &total_len, &names_len);
        for (j = 0; j < assembly->num_dlls; j++)
        {
            struct dll_redirect *dll = &assembly->dlls[j];
            get_comserver_datalen(&dll->entities, dll, &class_count, &total_len, &names_len);
        }
    }

    total_len += aligned_string_len(names_len);
    total_len += sizeof(*header);

    header = RtlAllocateHeap(RtlGetProcessHeap(), 0, total_len);
    if (!header) return STATUS_NO_MEMORY;

    memset(header, 0, sizeof(*header));
    header->magic = GUIDSECTION_MAGIC;
    header->size  = sizeof(*header);
    header->count = 2*class_count;
    header->index_offset = sizeof(*header) + aligned_string_len(names_len);
    index = (struct guid_index*)((BYTE*)header + header->index_offset);
    module_offset = sizeof(*header);
    data_offset = header->index_offset + 2*class_count*sizeof(*index);

    seed = NtGetTickCount();
    for (i = 0; i < actctx->num_assemblies; i++)
    {
        struct assembly *assembly = &actctx->assemblies[i];
        add_comserver_record(header, &assembly->entities, NULL, &index, &data_offset, &module_offset, &seed, i+1);
        for (j = 0; j < assembly->num_dlls; j++)
        {
            struct dll_redirect *dll = &assembly->dlls[j];
            add_comserver_record(header, &dll->entities, dll, &index, &data_offset, &module_offset, &seed, i+1);
        }
    }

    *section = header;

    return STATUS_SUCCESS;
}

static inline struct comclassredirect_data *get_comclass_data(ACTIVATION_CONTEXT *actctx, struct guid_index *index)
{
    return (struct comclassredirect_data*)((BYTE*)actctx->comserver_section + index->data_offset);
}

static NTSTATUS find_comserver_redirection(ACTIVATION_CONTEXT* actctx, const GUID *guid, ACTCTX_SECTION_KEYED_DATA* data)
{
    struct comclassredirect_data *comclass;
    struct guid_index *index = NULL;

    if (!(actctx->sections & SERVERREDIRECT_SECTION)) return STATUS_SXS_KEY_NOT_FOUND;

    if (!actctx->comserver_section)
    {
        struct guidsection_header *section;

        NTSTATUS status = build_comserver_section(actctx, &section);
        if (status) return status;

        if (InterlockedCompareExchangePointer((void**)&actctx->comserver_section, section, NULL))
            RtlFreeHeap(RtlGetProcessHeap(), 0, section);
    }

    index = find_guid_index(actctx->comserver_section, guid);
    if (!index) return STATUS_SXS_KEY_NOT_FOUND;

    comclass = get_comclass_data(actctx, index);

    data->ulDataFormatVersion = 1;
    data->lpData = comclass;
    /* full length includes string length with nulls */
    data->ulLength = comclass->size + comclass->clrdata_len;
    if (comclass->progid_len) data->ulLength += comclass->progid_len + sizeof(WCHAR);
    data->lpSectionGlobalData = (BYTE*)actctx->comserver_section + actctx->comserver_section->names_offset;
    data->ulSectionGlobalDataLength = actctx->comserver_section->names_len;
    data->lpSectionBase = actctx->comserver_section;
    data->ulSectionTotalLength = RtlSizeHeap( RtlGetProcessHeap(), 0, actctx->comserver_section );
    data->hActCtx = NULL;

    if (data->cbSize >= FIELD_OFFSET(ACTCTX_SECTION_KEYED_DATA, ulAssemblyRosterIndex) + sizeof(ULONG))
        data->ulAssemblyRosterIndex = index->rosterindex;

    return STATUS_SUCCESS;
}

static void get_ifaceps_datalen(const struct entity_array *entities, unsigned int *count, unsigned int *len)
{
    unsigned int i;

    for (i = 0; i < entities->num; i++)
    {
        struct entity *entity = &entities->base[i];
        if (entity->kind == ACTIVATION_CONTEXT_SECTION_COM_INTERFACE_REDIRECTION)
        {
            *len += sizeof(struct guid_index) + sizeof(struct ifacepsredirect_data);
            if (entity->u.ifaceps.name)
                *len += aligned_string_len((strlenW(entity->u.ifaceps.name)+1)*sizeof(WCHAR));
            *count += 1;
        }
    }
}

static void add_ifaceps_record(struct guidsection_header *section, struct entity_array *entities,
    struct guid_index **index, ULONG *data_offset, ULONG rosterindex)
{
    unsigned int i;

    for (i = 0; i < entities->num; i++)
    {
        struct entity *entity = &entities->base[i];
        if (entity->kind == ACTIVATION_CONTEXT_SECTION_COM_INTERFACE_REDIRECTION)
        {
            struct ifacepsredirect_data *data = (struct ifacepsredirect_data*)((BYTE*)section + *data_offset);
            UNICODE_STRING str;
            ULONG name_len;

            if (entity->u.ifaceps.name)
                name_len = strlenW(entity->u.ifaceps.name)*sizeof(WCHAR);
            else
                name_len = 0;

            /* setup index */
            RtlInitUnicodeString(&str, entity->u.ifaceps.iid);
            RtlGUIDFromString(&str, &(*index)->guid);
            (*index)->data_offset = *data_offset;
            (*index)->data_len = sizeof(*data) + name_len ? aligned_string_len(name_len + sizeof(WCHAR)) : 0;
            (*index)->rosterindex = rosterindex;

            /* setup data record */
            data->size = sizeof(*data);
            data->mask = entity->u.ifaceps.mask;

            /* proxyStubClsid32 value is only stored for external PS,
               if set it's used as iid, otherwise 'iid' attribute value is used */
            if (entity->u.ifaceps.ps32)
            {
                RtlInitUnicodeString(&str, entity->u.ifaceps.ps32);
                RtlGUIDFromString(&str, &data->iid);
            }
            else
                data->iid = (*index)->guid;

            data->nummethods = entity->u.ifaceps.nummethods;

            if (entity->u.ifaceps.tlib)
            {
                RtlInitUnicodeString(&str, entity->u.ifaceps.tlib);
                RtlGUIDFromString(&str, &data->tlbid);
            }
            else
                memset(&data->tlbid, 0, sizeof(data->tlbid));

            if (entity->u.ifaceps.base)
            {
                RtlInitUnicodeString(&str, entity->u.ifaceps.base);
                RtlGUIDFromString(&str, &data->base);
            }
            else
                memset(&data->base, 0, sizeof(data->base));

            data->name_len = name_len;
            data->name_offset = data->name_len ? sizeof(*data) : 0;

            /* name string */
            if (data->name_len)
            {
                WCHAR *ptrW = (WCHAR*)((BYTE*)data + data->name_offset);
                memcpy(ptrW, entity->u.ifaceps.name, data->name_len);
                ptrW[data->name_len/sizeof(WCHAR)] = 0;
            }

            /* move to next record */
            (*index) += 1;
            *data_offset += sizeof(*data);
            if (data->name_len)
                *data_offset += aligned_string_len(data->name_len + sizeof(WCHAR));
        }
    }
}

static NTSTATUS build_ifaceps_section(ACTIVATION_CONTEXT* actctx, struct guidsection_header **section)
{
    unsigned int i, j, total_len = 0, count = 0;
    struct guidsection_header *header;
    struct guid_index *index;
    ULONG data_offset;

    /* compute section length */
    for (i = 0; i < actctx->num_assemblies; i++)
    {
        struct assembly *assembly = &actctx->assemblies[i];

        get_ifaceps_datalen(&assembly->entities, &count, &total_len);
        for (j = 0; j < assembly->num_dlls; j++)
        {
            struct dll_redirect *dll = &assembly->dlls[j];
            get_ifaceps_datalen(&dll->entities, &count, &total_len);
        }
    }

    total_len += sizeof(*header);

    header = RtlAllocateHeap(RtlGetProcessHeap(), 0, total_len);
    if (!header) return STATUS_NO_MEMORY;

    memset(header, 0, sizeof(*header));
    header->magic = GUIDSECTION_MAGIC;
    header->size  = sizeof(*header);
    header->count = count;
    header->index_offset = sizeof(*header);
    index = (struct guid_index*)((BYTE*)header + header->index_offset);
    data_offset = header->index_offset + count*sizeof(*index);

    for (i = 0; i < actctx->num_assemblies; i++)
    {
        struct assembly *assembly = &actctx->assemblies[i];

        add_ifaceps_record(header, &assembly->entities, &index, &data_offset, i + 1);
        for (j = 0; j < assembly->num_dlls; j++)
        {
            struct dll_redirect *dll = &assembly->dlls[j];
            add_ifaceps_record(header, &dll->entities, &index, &data_offset, i + 1);
        }
    }

    *section = header;

    return STATUS_SUCCESS;
}

static inline struct ifacepsredirect_data *get_ifaceps_data(ACTIVATION_CONTEXT *actctx, struct guid_index *index)
{
    return (struct ifacepsredirect_data*)((BYTE*)actctx->ifaceps_section + index->data_offset);
}

static NTSTATUS find_cominterface_redirection(ACTIVATION_CONTEXT* actctx, const GUID *guid, ACTCTX_SECTION_KEYED_DATA* data)
{
    struct ifacepsredirect_data *iface;
    struct guid_index *index = NULL;

    if (!(actctx->sections & IFACEREDIRECT_SECTION)) return STATUS_SXS_KEY_NOT_FOUND;

    if (!actctx->ifaceps_section)
    {
        struct guidsection_header *section;

        NTSTATUS status = build_ifaceps_section(actctx, &section);
        if (status) return status;

        if (InterlockedCompareExchangePointer((void**)&actctx->ifaceps_section, section, NULL))
            RtlFreeHeap(RtlGetProcessHeap(), 0, section);
    }

    index = find_guid_index(actctx->ifaceps_section, guid);
    if (!index) return STATUS_SXS_KEY_NOT_FOUND;

    iface = get_ifaceps_data(actctx, index);

    data->ulDataFormatVersion = 1;
    data->lpData = iface;
    data->ulLength = iface->size + (iface->name_len ? iface->name_len + sizeof(WCHAR) : 0);
    data->lpSectionGlobalData = NULL;
    data->ulSectionGlobalDataLength = 0;
    data->lpSectionBase = actctx->ifaceps_section;
    data->ulSectionTotalLength = RtlSizeHeap( RtlGetProcessHeap(), 0, actctx->ifaceps_section );
    data->hActCtx = NULL;

    if (data->cbSize >= FIELD_OFFSET(ACTCTX_SECTION_KEYED_DATA, ulAssemblyRosterIndex) + sizeof(ULONG))
        data->ulAssemblyRosterIndex = index->rosterindex;

    return STATUS_SUCCESS;
}

static NTSTATUS build_clr_surrogate_section(ACTIVATION_CONTEXT* actctx, struct guidsection_header **section)
{
    unsigned int i, j, total_len = 0, count = 0;
    struct guidsection_header *header;
    struct clrsurrogate_data *data;
    struct guid_index *index;
    ULONG data_offset;

    /* compute section length */
    for (i = 0; i < actctx->num_assemblies; i++)
    {
        struct assembly *assembly = &actctx->assemblies[i];
        for (j = 0; j < assembly->entities.num; j++)
        {
            struct entity *entity = &assembly->entities.base[j];
            if (entity->kind == ACTIVATION_CONTEXT_SECTION_CLR_SURROGATES)
            {
                ULONG len;

                total_len += sizeof(*index) + sizeof(*data);
                len = strlenW(entity->u.clrsurrogate.name) + 1;
                if (entity->u.clrsurrogate.version)
                   len += strlenW(entity->u.clrsurrogate.version) + 1;
                total_len += aligned_string_len(len*sizeof(WCHAR));

                count++;
            }
        }
    }

    total_len += sizeof(*header);

    header = RtlAllocateHeap(RtlGetProcessHeap(), 0, total_len);
    if (!header) return STATUS_NO_MEMORY;

    memset(header, 0, sizeof(*header));
    header->magic = GUIDSECTION_MAGIC;
    header->size  = sizeof(*header);
    header->count = count;
    header->index_offset = sizeof(*header);
    index = (struct guid_index*)((BYTE*)header + header->index_offset);
    data_offset = header->index_offset + count*sizeof(*index);

    for (i = 0; i < actctx->num_assemblies; i++)
    {
        struct assembly *assembly = &actctx->assemblies[i];
        for (j = 0; j < assembly->entities.num; j++)
        {
            struct entity *entity = &assembly->entities.base[j];
            if (entity->kind == ACTIVATION_CONTEXT_SECTION_CLR_SURROGATES)
            {
                ULONG version_len, name_len;
                UNICODE_STRING str;
                WCHAR *ptrW;

                if (entity->u.clrsurrogate.version)
                    version_len = strlenW(entity->u.clrsurrogate.version)*sizeof(WCHAR);
                else
                    version_len = 0;
                name_len = strlenW(entity->u.clrsurrogate.name)*sizeof(WCHAR);

                /* setup new index entry */
                RtlInitUnicodeString(&str, entity->u.clrsurrogate.clsid);
                RtlGUIDFromString(&str, &index->guid);

                index->data_offset = data_offset;
                index->data_len = sizeof(*data) + aligned_string_len(name_len + sizeof(WCHAR) + (version_len ? version_len + sizeof(WCHAR) : 0));
                index->rosterindex = i + 1;

                /* setup data */
                data = (struct clrsurrogate_data*)((BYTE*)header + index->data_offset);
                data->size = sizeof(*data);
                data->res = 0;
                data->clsid = index->guid;
                data->version_offset = version_len ? data->size : 0;
                data->version_len = version_len;
                data->name_offset = data->size + version_len;
                if (version_len)
                    data->name_offset += sizeof(WCHAR);
                data->name_len = name_len;

                /* surrogate name */
                ptrW = (WCHAR*)((BYTE*)data + data->name_offset);
                memcpy(ptrW, entity->u.clrsurrogate.name, data->name_len);
                ptrW[data->name_len/sizeof(WCHAR)] = 0;

                /* runtime version */
                if (data->version_len)
                {
                    ptrW = (WCHAR*)((BYTE*)data + data->version_offset);
                    memcpy(ptrW, entity->u.clrsurrogate.version, data->version_len);
                    ptrW[data->version_len/sizeof(WCHAR)] = 0;
                }

                data_offset += index->data_offset;
                index++;
            }
        }
    }

    *section = header;

    return STATUS_SUCCESS;
}

static inline struct clrsurrogate_data *get_surrogate_data(ACTIVATION_CONTEXT *actctx, const struct guid_index *index)
{
    return (struct clrsurrogate_data*)((BYTE*)actctx->clrsurrogate_section + index->data_offset);
}

static NTSTATUS find_clr_surrogate(ACTIVATION_CONTEXT* actctx, const GUID *guid, ACTCTX_SECTION_KEYED_DATA* data)
{
    struct clrsurrogate_data *surrogate;
    struct guid_index *index = NULL;

    if (!(actctx->sections & CLRSURROGATES_SECTION)) return STATUS_SXS_KEY_NOT_FOUND;

    if (!actctx->clrsurrogate_section)
    {
        struct guidsection_header *section;

        NTSTATUS status = build_clr_surrogate_section(actctx, &section);
        if (status) return status;

        if (InterlockedCompareExchangePointer((void**)&actctx->clrsurrogate_section, section, NULL))
            RtlFreeHeap(RtlGetProcessHeap(), 0, section);
    }

    index = find_guid_index(actctx->clrsurrogate_section, guid);
    if (!index) return STATUS_SXS_KEY_NOT_FOUND;

    surrogate = get_surrogate_data(actctx, index);

    data->ulDataFormatVersion = 1;
    data->lpData = surrogate;
    /* full length includes string length with nulls */
    data->ulLength = surrogate->size + surrogate->name_len + sizeof(WCHAR);
    if (surrogate->version_len)
        data->ulLength += surrogate->version_len + sizeof(WCHAR);

    data->lpSectionGlobalData = NULL;
    data->ulSectionGlobalDataLength = 0;
    data->lpSectionBase = actctx->clrsurrogate_section;
    data->ulSectionTotalLength = RtlSizeHeap( RtlGetProcessHeap(), 0, actctx->clrsurrogate_section );
    data->hActCtx = NULL;

    if (data->cbSize >= FIELD_OFFSET(ACTCTX_SECTION_KEYED_DATA, ulAssemblyRosterIndex) + sizeof(ULONG))
        data->ulAssemblyRosterIndex = index->rosterindex;

    return STATUS_SUCCESS;
}

static void get_progid_datalen(struct entity_array *entities, unsigned int *count, unsigned int *total_len)
{
    unsigned int i, j, single_len;

    single_len = sizeof(struct progidredirect_data) + sizeof(struct string_index) + sizeof(GUID);
    for (i = 0; i < entities->num; i++)
    {
        struct entity *entity = &entities->base[i];
        if (entity->kind == ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION)
        {
            if (entity->u.comclass.progid)
            {
                *total_len += single_len + aligned_string_len((strlenW(entity->u.comclass.progid)+1)*sizeof(WCHAR));
                *count += 1;
            }

            for (j = 0; j < entity->u.comclass.progids.num; j++)
                *total_len += aligned_string_len((strlenW(entity->u.comclass.progids.progids[j])+1)*sizeof(WCHAR));

            *total_len += single_len*entity->u.comclass.progids.num;
            *count += entity->u.comclass.progids.num;
        }
    }
}

static void write_progid_record(struct strsection_header *section, const WCHAR *progid, const GUID *alias,
    struct string_index **index, ULONG *data_offset, ULONG *global_offset, ULONG rosterindex)
{
    struct progidredirect_data *data;
    UNICODE_STRING str;
    GUID *guid_ptr;
    WCHAR *ptrW;

    /* setup new index entry */

    /* hash progid name */
    RtlInitUnicodeString(&str, progid);
    RtlHashUnicodeString(&str, TRUE, HASH_STRING_ALGORITHM_X65599, &(*index)->hash);

    (*index)->name_offset = *data_offset;
    (*index)->name_len = str.Length;
    (*index)->data_offset = (*index)->name_offset + aligned_string_len(str.MaximumLength);
    (*index)->data_len = sizeof(*data);
    (*index)->rosterindex = rosterindex;

    *data_offset += aligned_string_len(str.MaximumLength);

    /* setup data structure */
    data = (struct progidredirect_data*)((BYTE*)section + *data_offset);
    data->size = sizeof(*data);
    data->reserved = 0;
    data->clsid_offset = *global_offset;

    /* write progid string */
    ptrW = (WCHAR*)((BYTE*)section + (*index)->name_offset);
    memcpy(ptrW, progid, (*index)->name_len);
    ptrW[(*index)->name_len/sizeof(WCHAR)] = 0;

    /* write guid to global area */
    guid_ptr = (GUID*)((BYTE*)section + data->clsid_offset);
    *guid_ptr = *alias;

    /* to next entry */
    *global_offset += sizeof(GUID);
    *data_offset += data->size;
    (*index) += 1;
}

static void add_progid_record(ACTIVATION_CONTEXT* actctx, struct strsection_header *section, const struct entity_array *entities,
    struct string_index **index, ULONG *data_offset, ULONG *global_offset, ULONG rosterindex)
{
    unsigned int i, j;

    for (i = 0; i < entities->num; i++)
    {
        struct entity *entity = &entities->base[i];
        if (entity->kind == ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION)
        {
            const struct progids *progids = &entity->u.comclass.progids;
            struct comclassredirect_data *comclass;
            struct guid_index *guid_index;
            UNICODE_STRING str;
            GUID clsid;

            RtlInitUnicodeString(&str, entity->u.comclass.clsid);
            RtlGUIDFromString(&str, &clsid);

            guid_index = find_guid_index(actctx->comserver_section, &clsid);
            comclass = get_comclass_data(actctx, guid_index);

            if (entity->u.comclass.progid)
                write_progid_record(section, entity->u.comclass.progid, &comclass->alias,
                     index, data_offset, global_offset, rosterindex);

            for (j = 0; j < progids->num; j++)
                write_progid_record(section, progids->progids[j], &comclass->alias,
                     index, data_offset, global_offset, rosterindex);
        }
    }
}

static NTSTATUS build_progid_section(ACTIVATION_CONTEXT* actctx, struct strsection_header **section)
{
    unsigned int i, j, total_len = 0, count = 0;
    struct strsection_header *header;
    ULONG data_offset, global_offset;
    struct string_index *index;

    /* compute section length */
    for (i = 0; i < actctx->num_assemblies; i++)
    {
        struct assembly *assembly = &actctx->assemblies[i];

        get_progid_datalen(&assembly->entities, &count, &total_len);
        for (j = 0; j < assembly->num_dlls; j++)
        {
            struct dll_redirect *dll = &assembly->dlls[j];
            get_progid_datalen(&dll->entities, &count, &total_len);
        }
    }

    total_len += sizeof(*header);

    header = RtlAllocateHeap(RtlGetProcessHeap(), 0, total_len);
    if (!header) return STATUS_NO_MEMORY;

    memset(header, 0, sizeof(*header));
    header->magic = STRSECTION_MAGIC;
    header->size  = sizeof(*header);
    header->count = count;
    header->global_offset = header->size;
    header->global_len = count*sizeof(GUID);
    header->index_offset = header->size + header->global_len;

    index = (struct string_index*)((BYTE*)header + header->index_offset);
    data_offset = header->index_offset + count*sizeof(*index);
    global_offset = header->global_offset;

    for (i = 0; i < actctx->num_assemblies; i++)
    {
        struct assembly *assembly = &actctx->assemblies[i];

        add_progid_record(actctx, header, &assembly->entities, &index, &data_offset, &global_offset, i + 1);
        for (j = 0; j < assembly->num_dlls; j++)
        {
            struct dll_redirect *dll = &assembly->dlls[j];
            add_progid_record(actctx, header, &dll->entities, &index, &data_offset, &global_offset, i + 1);
        }
    }

    *section = header;

    return STATUS_SUCCESS;
}

static inline struct progidredirect_data *get_progid_data(ACTIVATION_CONTEXT *actctx, const struct string_index *index)
{
    return (struct progidredirect_data*)((BYTE*)actctx->progid_section + index->data_offset);
}

static NTSTATUS find_progid_redirection(ACTIVATION_CONTEXT* actctx, const UNICODE_STRING *name,
                                     PACTCTX_SECTION_KEYED_DATA data)
{
    struct progidredirect_data *progid;
    struct string_index *index;

    if (!(actctx->sections & PROGIDREDIRECT_SECTION)) return STATUS_SXS_KEY_NOT_FOUND;

    if (!actctx->comserver_section)
    {
        struct guidsection_header *section;

        NTSTATUS status = build_comserver_section(actctx, &section);
        if (status) return status;

        if (InterlockedCompareExchangePointer((void**)&actctx->comserver_section, section, NULL))
            RtlFreeHeap(RtlGetProcessHeap(), 0, section);
    }

    if (!actctx->progid_section)
    {
        struct strsection_header *section;

        NTSTATUS status = build_progid_section(actctx, &section);
        if (status) return status;

        if (InterlockedCompareExchangePointer((void**)&actctx->progid_section, section, NULL))
            RtlFreeHeap(RtlGetProcessHeap(), 0, section);
    }

    index = find_string_index(actctx->progid_section, name);
    if (!index) return STATUS_SXS_KEY_NOT_FOUND;

    progid = get_progid_data(actctx, index);

    data->ulDataFormatVersion = 1;
    data->lpData = progid;
    data->ulLength = progid->size;
    data->lpSectionGlobalData = (BYTE*)actctx->progid_section + actctx->progid_section->global_offset;
    data->ulSectionGlobalDataLength = actctx->progid_section->global_len;
    data->lpSectionBase = actctx->progid_section;
    data->ulSectionTotalLength = RtlSizeHeap( RtlGetProcessHeap(), 0, actctx->progid_section );
    data->hActCtx = NULL;

    if (data->cbSize >= FIELD_OFFSET(ACTCTX_SECTION_KEYED_DATA, ulAssemblyRosterIndex) + sizeof(ULONG))
        data->ulAssemblyRosterIndex = index->rosterindex;

    return STATUS_SUCCESS;
}

static NTSTATUS find_string(ACTIVATION_CONTEXT* actctx, ULONG section_kind,
                            const UNICODE_STRING *section_name,
                            DWORD flags, PACTCTX_SECTION_KEYED_DATA data)
{
    NTSTATUS status;

    switch (section_kind)
    {
    case ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION:
        status = find_dll_redirection(actctx, section_name, data);
        break;
    case ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION:
        status = find_window_class(actctx, section_name, data);
        break;
    case ACTIVATION_CONTEXT_SECTION_COM_PROGID_REDIRECTION:
        status = find_progid_redirection(actctx, section_name, data);
        break;
    case ACTIVATION_CONTEXT_SECTION_GLOBAL_OBJECT_RENAME_TABLE:
        DPRINT1("Unsupported yet section_kind %x\n", section_kind);
        return STATUS_SXS_SECTION_NOT_FOUND;
    default:
        DPRINT1("Unknown section_kind %x\n", section_kind);
        return STATUS_SXS_SECTION_NOT_FOUND;
    }

    if (status != STATUS_SUCCESS) return status;

    if (flags & FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX)
    {
        actctx_addref(actctx);
        data->hActCtx = actctx;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS find_guid(ACTIVATION_CONTEXT* actctx, ULONG section_kind,
                          const GUID *guid, DWORD flags, PACTCTX_SECTION_KEYED_DATA data)
{
    NTSTATUS status;

    switch (section_kind)
    {
    case ACTIVATION_CONTEXT_SECTION_COM_TYPE_LIBRARY_REDIRECTION:
        status = find_tlib_redirection(actctx, guid, data);
        break;
    case ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION:
        status = find_comserver_redirection(actctx, guid, data);
        break;
    case ACTIVATION_CONTEXT_SECTION_COM_INTERFACE_REDIRECTION:
        status = find_cominterface_redirection(actctx, guid, data);
        break;
    case ACTIVATION_CONTEXT_SECTION_CLR_SURROGATES:
        status = find_clr_surrogate(actctx, guid, data);
        break;
    default:
        DPRINT("Unknown section_kind %x\n", section_kind);
        return STATUS_SXS_SECTION_NOT_FOUND;
    }

    if (status != STATUS_SUCCESS) return status;

    if (flags & FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX)
    {
        actctx_addref(actctx);
        data->hActCtx = actctx;
    }
    return STATUS_SUCCESS;
}

/* initialize the activation context for the current process */
void actctx_init(void)
{
    ACTCTXW ctx;
    HANDLE handle;

    ctx.cbSize   = sizeof(ctx);
    ctx.lpSource = NULL;
    ctx.dwFlags  = ACTCTX_FLAG_RESOURCE_NAME_VALID | ACTCTX_FLAG_HMODULE_VALID;
    ctx.hModule  = NtCurrentTeb()->ProcessEnvironmentBlock->ImageBaseAddress;
    ctx.lpResourceName = (LPCWSTR)CREATEPROCESS_MANIFEST_RESOURCE_ID;

    if (NT_SUCCESS(RtlCreateActivationContext(0, (PVOID)&ctx, 0, NULL, NULL, &handle)))
    {
        process_actctx = check_actctx(handle);
    }
}

/* FUNCTIONS ***************************************************************/

/***********************************************************************
 * RtlCreateActivationContext (NTDLL.@)
 *
 * Create an activation context.
 */
NTSTATUS
NTAPI
RtlCreateActivationContext(IN ULONG Flags,
                           IN PACTIVATION_CONTEXT_DATA ActivationContextData,
                           IN ULONG ExtraBytes,
                           IN PVOID NotificationRoutine,
                           IN PVOID NotificationContext,
                           OUT PACTIVATION_CONTEXT *ActCtx)
{
    const ACTCTXW *pActCtx = (PVOID)ActivationContextData;
    const WCHAR *directory = NULL;
    PACTIVATION_CONTEXT_WRAPPED ActualActCtx;
    ACTIVATION_CONTEXT *actctx;
    UNICODE_STRING nameW;
    ULONG lang = 0;
    NTSTATUS status = STATUS_NO_MEMORY;
    HANDLE file = 0;
    struct actctx_loader acl;

    DPRINT("%p %08x\n", pActCtx, pActCtx ? pActCtx->dwFlags : 0);

    if (!pActCtx || pActCtx->cbSize < sizeof(*pActCtx) ||
        (pActCtx->dwFlags & ~ACTCTX_FLAGS_ALL))
        return STATUS_INVALID_PARAMETER;


    if (!(ActualActCtx = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ActualActCtx))))
        return STATUS_NO_MEMORY;

    ActualActCtx->MagicMarker = ACTCTX_MAGIC_MARKER;

    actctx = &ActualActCtx->ActivationContext;
    actctx->RefCount = 1;
    actctx->config.type = ACTIVATION_CONTEXT_PATH_TYPE_NONE;
    actctx->config.info = NULL;
    actctx->appdir.type = ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE;
    if (pActCtx->dwFlags & ACTCTX_FLAG_APPLICATION_NAME_VALID)
    {
        if (!(actctx->appdir.info = strdupW( pActCtx->lpApplicationName ))) goto error;
    }
    else
    {
        UNICODE_STRING dir;
        WCHAR *p;
        HMODULE module;

        if (pActCtx->dwFlags & ACTCTX_FLAG_HMODULE_VALID) module = pActCtx->hModule;
        else module = NtCurrentTeb()->ProcessEnvironmentBlock->ImageBaseAddress;

        status = get_module_filename( module, &dir, 0 );
        if (!NT_SUCCESS(status)) goto error;
        if ((p = strrchrW( dir.Buffer, '\\' ))) p[1] = 0;
        actctx->appdir.info = dir.Buffer;
    }

    nameW.Buffer = NULL;

    /* open file only if it's going to be used */
    if (pActCtx->lpSource && !((pActCtx->dwFlags & ACTCTX_FLAG_RESOURCE_NAME_VALID) &&
                               (pActCtx->dwFlags & ACTCTX_FLAG_HMODULE_VALID)))
    {
        WCHAR *source = NULL;
        BOOLEAN ret;

        if (pActCtx->dwFlags & ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID &&
            RtlDetermineDosPathNameType_U(pActCtx->lpSource) == RtlPathTypeRelative)
        {
            DWORD dir_len, source_len;

            dir_len = strlenW(pActCtx->lpAssemblyDirectory);
            source_len = strlenW(pActCtx->lpSource);
            if (!(source = RtlAllocateHeap( RtlGetProcessHeap(), 0, (dir_len+source_len+2)*sizeof(WCHAR))))
            {
                status = STATUS_NO_MEMORY;
                goto error;
            }

            memcpy(source, pActCtx->lpAssemblyDirectory, dir_len*sizeof(WCHAR));
            source[dir_len] = '\\';
            memcpy(source+dir_len+1, pActCtx->lpSource, (source_len+1)*sizeof(WCHAR));
        }

        ret = RtlDosPathNameToNtPathName_U(source ? source : pActCtx->lpSource, &nameW, NULL, NULL);
        if (source) RtlFreeHeap( RtlGetProcessHeap(), 0, source );
        if (!ret)
        {
            status = STATUS_NO_SUCH_FILE;
            goto error;
        }
        status = open_nt_file( &file, &nameW );
        if (!NT_SUCCESS(status))
        {
            RtlFreeUnicodeString( &nameW );
            goto error;
        }
    }

    acl.actctx = actctx;
    acl.dependencies = NULL;
    acl.num_dependencies = 0;
    acl.allocated_dependencies = 0;

    if (pActCtx->dwFlags & ACTCTX_FLAG_LANGID_VALID) lang = pActCtx->wLangId;
    if (pActCtx->dwFlags & ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID) directory = pActCtx->lpAssemblyDirectory;

    if (pActCtx->dwFlags & ACTCTX_FLAG_RESOURCE_NAME_VALID)
    {
        /* if we have a resource it's a PE file */
        if (pActCtx->dwFlags & ACTCTX_FLAG_HMODULE_VALID)
        {
            status = get_manifest_in_module( &acl, NULL, NULL, directory, FALSE, pActCtx->hModule,
                                             pActCtx->lpResourceName, lang );
            if (status && status != STATUS_SXS_CANT_GEN_ACTCTX)
                /* FIXME: what to do if pActCtx->lpSource is set */
                status = get_manifest_in_associated_manifest( &acl, NULL, NULL, directory,
                                                              pActCtx->hModule, pActCtx->lpResourceName );
        }
        else if (pActCtx->lpSource)
        {
            status = get_manifest_in_pe_file( &acl, NULL, nameW.Buffer, directory, FALSE,
                                              file, pActCtx->lpResourceName, lang );
            if (status && status != STATUS_SXS_CANT_GEN_ACTCTX)
                status = get_manifest_in_associated_manifest( &acl, NULL, nameW.Buffer, directory,
                                                              NULL, pActCtx->lpResourceName );
        }
        else status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = get_manifest_in_manifest_file( &acl, NULL, nameW.Buffer, directory, FALSE, file );
    }

    if (file) NtClose( file );
    RtlFreeUnicodeString( &nameW );

    if (NT_SUCCESS(status)) status = parse_depend_manifests(&acl);
    free_depend_manifests( &acl );

    if (NT_SUCCESS(status))
        *ActCtx = actctx;
    else actctx_release( actctx );
    return status;

error:
    if (file) NtClose( file );
    actctx_release( actctx );
    return status;
}

#if 0
#define ACT_CTX_VALID(p)    ((((ULONG_PTR)p - 1) | 7) != -1)

VOID
NTAPI
RtlAddRefActivationContext(IN PACTIVATION_CONTEXT Handle)
{
    PIACTIVATION_CONTEXT ActCtx = (PIACTIVATION_CONTEXT)Handle;
    LONG OldRefCount, NewRefCount;

    if ((ActCtx) && (ACT_CTX_VALID(ActCtx)) && (ActCtx->RefCount != LONG_MAX))
    {
        RtlpValidateActCtx(ActCtx);

        while (TRUE)
        {
            OldRefCount = ActCtx->RefCount;
            ASSERT(OldRefCount > 0);

            if (OldRefCount == LONG_MAX) break;

            NewRefCount = OldRefCount + 1;
            if (InterlockedCompareExchange(&ActCtx->RefCount,
                                           NewRefCount,
                                           OldRefCount) == OldRefCount)
            {
                break;
            }
        }

        NewRefCount = LONG_MAX;
        ASSERT(NewRefCount > 0);
    }
}

VOID
NTAPI
RtlReleaseActivationContext( HANDLE handle )
{
    PIACTIVATION_CONTEXT ActCtx = (PIACTIVATION_CONTEXT) Handle;

    if ((ActCtx) && (ACT_CTX_VALID(ActCtx)) && (ActCtx->RefCount != LONG_MAX))
    {
        RtlpValidateActCtx(ActCtx);

        actctx_release(ActCtx);
    }
}
#else

/***********************************************************************
 *		RtlAddRefActivationContext (NTDLL.@)
 */
VOID NTAPI RtlAddRefActivationContext( HANDLE handle )
{
    ACTIVATION_CONTEXT *actctx;

    if ((actctx = check_actctx( handle ))) actctx_addref( actctx );
}


/******************************************************************
 *		RtlReleaseActivationContext (NTDLL.@)
 */
VOID NTAPI RtlReleaseActivationContext( HANDLE handle )
{
    ACTIVATION_CONTEXT *actctx;

    if ((actctx = check_actctx( handle ))) actctx_release( actctx );
}

#endif

/******************************************************************
 *              RtlZombifyActivationContext (NTDLL.@)
 *
 */
NTSTATUS NTAPI RtlZombifyActivationContext(PVOID Context)
{
    UNIMPLEMENTED;

    if (Context == ACTCTX_FAKE_HANDLE)
        return STATUS_SUCCESS;

    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI RtlActivateActivationContextEx( ULONG flags, PTEB tebAddress, HANDLE handle, PULONG_PTR cookie )
{
    RTL_ACTIVATION_CONTEXT_STACK_FRAME *frame;

    if (!(frame = RtlAllocateHeap( RtlGetProcessHeap(), 0, sizeof(*frame) )))
        return STATUS_NO_MEMORY;

    frame->Previous = tebAddress->ActivationContextStackPointer->ActiveFrame;
    frame->ActivationContext = handle;
    frame->Flags = 0;

    tebAddress->ActivationContextStackPointer->ActiveFrame = frame;
    RtlAddRefActivationContext( handle );

    *cookie = (ULONG_PTR)frame;
    DPRINT( "%p cookie=%lx\n", handle, *cookie );
    return STATUS_SUCCESS;
}

/******************************************************************
 *		RtlActivateActivationContext (NTDLL.@)
 */
NTSTATUS NTAPI RtlActivateActivationContext( ULONG flags, HANDLE handle, PULONG_PTR cookie )
{
    return RtlActivateActivationContextEx(flags, NtCurrentTeb(), handle, cookie);
}

/***********************************************************************
 *		RtlDeactivateActivationContext (NTDLL.@)
 */
NTSTATUS NTAPI RtlDeactivateActivationContext( ULONG flags, ULONG_PTR cookie )
{
    RTL_ACTIVATION_CONTEXT_STACK_FRAME *frame, *top;

    DPRINT( "%x cookie=%lx\n", flags, cookie );

    /* find the right frame */
    top = NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame;
    for (frame = top; frame; frame = frame->Previous)
        if ((ULONG_PTR)frame == cookie) break;

    if (!frame)
        RtlRaiseStatus( STATUS_SXS_INVALID_DEACTIVATION );

    if (frame != top && !(flags & RTL_DEACTIVATE_ACTIVATION_CONTEXT_FLAG_FORCE_EARLY_DEACTIVATION))
        RtlRaiseStatus( STATUS_SXS_EARLY_DEACTIVATION );

    /* pop everything up to and including frame */
    NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame = frame->Previous;

    while (top != NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame)
    {
        frame = top->Previous;
        RtlReleaseActivationContext( top->ActivationContext );
        RtlFreeHeap( RtlGetProcessHeap(), 0, top );
        top = frame;
    }

    return STATUS_SUCCESS;
}

VOID
NTAPI
RtlFreeActivationContextStack(IN PACTIVATION_CONTEXT_STACK Stack)
{
    PRTL_ACTIVATION_CONTEXT_STACK_FRAME ActiveFrame, PrevFrame;

    /* Nothing to do if there is no stack */
    if (!Stack) return;

    /* Get the current active frame */
    ActiveFrame = Stack->ActiveFrame;

    /* Go through them in backwards order and release */
    while (ActiveFrame)
    {
        PrevFrame = ActiveFrame->Previous;
        RtlReleaseActivationContext(ActiveFrame->ActivationContext);
        RtlFreeHeap(RtlGetProcessHeap(), 0, ActiveFrame);
        ActiveFrame = PrevFrame;
    }

    /* Zero out the active frame */
    Stack->ActiveFrame = NULL;

    /* TODO: Empty the Frame List Cache */
    ASSERT(IsListEmpty(&Stack->FrameListCache));

    /* Free activation stack memory */
    RtlFreeHeap(RtlGetProcessHeap(), 0, Stack);
}

/******************************************************************
 *		RtlFreeThreadActivationContextStack (NTDLL.@)
 */
VOID NTAPI RtlFreeThreadActivationContextStack(VOID)
{
    RtlFreeActivationContextStack(NtCurrentTeb()->ActivationContextStackPointer);
    NtCurrentTeb()->ActivationContextStackPointer = NULL;
}


/******************************************************************
 *		RtlGetActiveActivationContext (NTDLL.@)
 */
NTSTATUS NTAPI RtlGetActiveActivationContext( HANDLE *handle )
{
    if (NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame)
    {
        *handle = NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame->ActivationContext;
        RtlAddRefActivationContext( *handle );
    }
    else
        *handle = 0;

    return STATUS_SUCCESS;
}


/******************************************************************
 *		RtlIsActivationContextActive (NTDLL.@)
 */
BOOLEAN NTAPI RtlIsActivationContextActive( HANDLE handle )
{
    RTL_ACTIVATION_CONTEXT_STACK_FRAME *frame;

    for (frame = NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame; frame; frame = frame->Previous)
        if (frame->ActivationContext == handle) return TRUE;
    return FALSE;
}


/***********************************************************************
 *		RtlQueryInformationActivationContext (NTDLL.@)
 *
 * Get information about an activation context.
 * FIXME: function signature/prototype may be wrong
 */
NTSTATUS NTAPI RtlQueryInformationActivationContext( ULONG flags, HANDLE handle, PVOID subinst,
                                                     ULONG class, PVOID buffer,
                                                     SIZE_T bufsize, SIZE_T *retlen )
{
    ACTIVATION_CONTEXT *actctx;
    NTSTATUS status;

    DPRINT("%08x %p %p %u %p %Iu %p\n", flags, handle,
          subinst, class, buffer, bufsize, retlen);

    if (retlen) *retlen = 0;
    if ((status = find_query_actctx( &handle, flags, class ))) return status;

    switch (class)
    {
    case ActivationContextBasicInformation:
        {
            ACTIVATION_CONTEXT_BASIC_INFORMATION *info = buffer;

            if (retlen) *retlen = sizeof(*info);
            if (!info || bufsize < sizeof(*info)) return STATUS_BUFFER_TOO_SMALL;

            info->hActCtx = handle;
            info->dwFlags = 0;  /* FIXME */
            if (!(flags & RTL_QUERY_ACTIVATION_CONTEXT_FLAG_NO_ADDREF)) RtlAddRefActivationContext(handle);
        }
        break;

    case ActivationContextDetailedInformation:
        {
            ACTIVATION_CONTEXT_DETAILED_INFORMATION *acdi = buffer;
            struct assembly *assembly = NULL;
            SIZE_T len, manifest_len = 0, config_len = 0, appdir_len = 0;
            LPWSTR ptr;

            if (!(actctx = check_actctx(handle))) return STATUS_INVALID_PARAMETER;

            if (actctx->num_assemblies) assembly = actctx->assemblies;

            if (assembly && assembly->manifest.info)
                manifest_len = strlenW(assembly->manifest.info) + 1;
            if (actctx->config.info) config_len = strlenW(actctx->config.info) + 1;
            if (actctx->appdir.info) appdir_len = strlenW(actctx->appdir.info) + 1;
            len = sizeof(*acdi) + (manifest_len + config_len + appdir_len) * sizeof(WCHAR);

            if (retlen) *retlen = len;
            if (!buffer || bufsize < len) return STATUS_BUFFER_TOO_SMALL;

            acdi->dwFlags = 0;
            acdi->ulFormatVersion = assembly ? 1 : 0; /* FIXME */
            acdi->ulAssemblyCount = actctx->num_assemblies;
            acdi->ulRootManifestPathType = assembly ? assembly->manifest.type : 0 /* FIXME */;
            acdi->ulRootManifestPathChars = assembly && assembly->manifest.info ? (DWORD)manifest_len - 1 : 0;
            acdi->ulRootConfigurationPathType = actctx->config.type;
            acdi->ulRootConfigurationPathChars = actctx->config.info ? (DWORD)config_len - 1 : 0;
            acdi->ulAppDirPathType = actctx->appdir.type;
            acdi->ulAppDirPathChars = actctx->appdir.info ? (DWORD)appdir_len - 1 : 0;
            ptr = (LPWSTR)(acdi + 1);
            if (manifest_len)
            {
                acdi->lpRootManifestPath = ptr;
                memcpy(ptr, assembly->manifest.info, manifest_len * sizeof(WCHAR));
                ptr += manifest_len;
            }
            else acdi->lpRootManifestPath = NULL;
            if (config_len)
            {
                acdi->lpRootConfigurationPath = ptr;
                memcpy(ptr, actctx->config.info, config_len * sizeof(WCHAR));
                ptr += config_len;
            }
            else acdi->lpRootConfigurationPath = NULL;
            if (appdir_len)
            {
                acdi->lpAppDirPath = ptr;
                memcpy(ptr, actctx->appdir.info, appdir_len * sizeof(WCHAR));
            }
            else acdi->lpAppDirPath = NULL;
        }
        break;

    case AssemblyDetailedInformationInActivationContext:
        {
            ACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION *afdi = buffer;
            struct assembly *assembly;
            WCHAR *assembly_id;
            DWORD index;
            SIZE_T len, id_len = 0, ad_len = 0, path_len = 0;
            LPWSTR ptr;

            if (!(actctx = check_actctx(handle))) return STATUS_INVALID_PARAMETER;
            if (!subinst) return STATUS_INVALID_PARAMETER;

            index = *(DWORD*)subinst;
            if (!index || index > actctx->num_assemblies) return STATUS_INVALID_PARAMETER;

            assembly = &actctx->assemblies[index - 1];

            if (!(assembly_id = build_assembly_id( &assembly->id ))) return STATUS_NO_MEMORY;
            id_len = strlenW(assembly_id) + 1;
            if (assembly->directory) ad_len = strlenW(assembly->directory) + 1;

            if (assembly->manifest.info &&
                (assembly->type == ASSEMBLY_MANIFEST || assembly->type == ASSEMBLY_SHARED_MANIFEST))
                path_len  = strlenW(assembly->manifest.info) + 1;

            len = sizeof(*afdi) + (id_len + ad_len + path_len) * sizeof(WCHAR);

            if (retlen) *retlen = len;
            if (!buffer || bufsize < len)
            {
                RtlFreeHeap( RtlGetProcessHeap(), 0, assembly_id );
                return STATUS_BUFFER_TOO_SMALL;
            }

            afdi->ulFlags = 0;  /* FIXME */
            afdi->ulEncodedAssemblyIdentityLength = (DWORD)(id_len - 1) * sizeof(WCHAR);
            afdi->ulManifestPathType = assembly->manifest.type;
            afdi->ulManifestPathLength = assembly->manifest.info ? (DWORD)(path_len - 1) * sizeof(WCHAR) : 0;
            /* FIXME afdi->liManifestLastWriteTime = 0; */
            afdi->ulPolicyPathType = ACTIVATION_CONTEXT_PATH_TYPE_NONE; /* FIXME */
            afdi->ulPolicyPathLength = 0;
            /* FIXME afdi->liPolicyLastWriteTime = 0; */
            afdi->ulMetadataSatelliteRosterIndex = 0; /* FIXME */
            afdi->ulManifestVersionMajor = 1;
            afdi->ulManifestVersionMinor = 0;
            afdi->ulPolicyVersionMajor = 0; /* FIXME */
            afdi->ulPolicyVersionMinor = 0; /* FIXME */
            afdi->ulAssemblyDirectoryNameLength = ad_len ? (DWORD)(ad_len - 1) * sizeof(WCHAR) : 0;
            ptr = (LPWSTR)(afdi + 1);
            afdi->lpAssemblyEncodedAssemblyIdentity = ptr;
            memcpy( ptr, assembly_id, id_len * sizeof(WCHAR) );
            ptr += id_len;
            if (path_len)
            {
                afdi->lpAssemblyManifestPath = ptr;
                memcpy(ptr, assembly->manifest.info, path_len * sizeof(WCHAR));
                ptr += path_len;
            } else afdi->lpAssemblyManifestPath = NULL;
            afdi->lpAssemblyPolicyPath = NULL; /* FIXME */
            if (ad_len)
            {
                afdi->lpAssemblyDirectoryName = ptr;
                memcpy(ptr, assembly->directory, ad_len * sizeof(WCHAR));
            }
            else afdi->lpAssemblyDirectoryName = NULL;
            RtlFreeHeap( RtlGetProcessHeap(), 0, assembly_id );
        }
        break;

    case FileInformationInAssemblyOfAssemblyInActivationContext:
        {
            const ACTIVATION_CONTEXT_QUERY_INDEX *acqi = subinst;
            ASSEMBLY_FILE_DETAILED_INFORMATION *afdi = buffer;
            struct assembly *assembly;
            struct dll_redirect *dll;
            SIZE_T len, dll_len = 0;
            LPWSTR ptr;

            if (!(actctx = check_actctx(handle))) return STATUS_INVALID_PARAMETER;
            if (!acqi) return STATUS_INVALID_PARAMETER;

            if (acqi->ulAssemblyIndex >= actctx->num_assemblies)
                return STATUS_INVALID_PARAMETER;
            assembly = &actctx->assemblies[acqi->ulAssemblyIndex];

            if (acqi->ulFileIndexInAssembly >= assembly->num_dlls)
                return STATUS_INVALID_PARAMETER;
            dll = &assembly->dlls[acqi->ulFileIndexInAssembly];

            if (dll->name) dll_len = strlenW(dll->name) + 1;
            len = sizeof(*afdi) + dll_len * sizeof(WCHAR);

            if (!buffer || bufsize < len)
            {
                if (retlen) *retlen = len;
                return STATUS_BUFFER_TOO_SMALL;
            }
            if (retlen) *retlen = 0; /* yes that's what native does !! */
            afdi->ulFlags = ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION;
            afdi->ulFilenameLength = dll_len ? (DWORD)(dll_len - 1) * sizeof(WCHAR) : 0;
            afdi->ulPathLength = 0; /* FIXME */
            ptr = (LPWSTR)(afdi + 1);
            if (dll_len)
            {
                afdi->lpFileName = ptr;
                memcpy( ptr, dll->name, dll_len * sizeof(WCHAR) );
            } else afdi->lpFileName = NULL;
            afdi->lpFilePath = NULL; /* FIXME */
        }
        break;

    default:
        DPRINT( "class %u not implemented\n", class );
        return STATUS_NOT_IMPLEMENTED;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlQueryInformationActiveActivationContext(ULONG ulInfoClass,
                                           PVOID pvBuffer,
                                           SIZE_T cbBuffer OPTIONAL,
                                           SIZE_T *pcbWrittenOrRequired OPTIONAL)
{
    return RtlQueryInformationActivationContext(RTL_QUERY_ACTIVATION_CONTEXT_FLAG_USE_ACTIVE_ACTIVATION_CONTEXT,
                                                NULL,
                                                NULL,
                                                ulInfoClass,
                                                pvBuffer,
                                                cbBuffer,
                                                pcbWrittenOrRequired);
}

#define FIND_ACTCTX_RETURN_FLAGS 0x00000002
#define FIND_ACTCTX_RETURN_ASSEMBLY_METADATA 0x00000004
#define FIND_ACTCTX_VALID_MASK (FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX | FIND_ACTCTX_RETURN_FLAGS | FIND_ACTCTX_RETURN_ASSEMBLY_METADATA)

NTSTATUS
NTAPI
RtlpFindActivationContextSection_CheckParameters( ULONG flags, const GUID *guid, ULONG section_kind,
                                                  const UNICODE_STRING *section_name, PACTCTX_SECTION_KEYED_DATA data )
{
    /* Check general parameter combinations */
    if (!section_name ||  !section_name->Buffer || 
        (flags & ~FIND_ACTCTX_VALID_MASK) ||
        ((flags & FIND_ACTCTX_VALID_MASK) && !data) ||
        (data && data->cbSize < offsetof(ACTCTX_SECTION_KEYED_DATA, ulAssemblyRosterIndex)))
    {
        DPRINT1("invalid parameter\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* TODO */
    if (flags & FIND_ACTCTX_RETURN_FLAGS ||
        flags & FIND_ACTCTX_RETURN_ASSEMBLY_METADATA)
    {
        DPRINT1("unknown flags %08x\n", flags);
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

/***********************************************************************
 *		RtlFindActivationContextSectionString (NTDLL.@)
 *
 * Find information about a string in an activation context.
 * FIXME: function signature/prototype may be wrong
 */
NTSTATUS NTAPI RtlFindActivationContextSectionString( ULONG flags, const GUID *guid, ULONG section_kind,
                                                      const UNICODE_STRING *section_name, PVOID ptr )
{
    PACTCTX_SECTION_KEYED_DATA data = ptr;
    NTSTATUS status;

    DPRINT("RtlFindActivationContextSectionString(%x %p %x %wZ %p)\n", flags, guid, section_kind, section_name, ptr);
    status = RtlpFindActivationContextSection_CheckParameters(flags, guid, section_kind, section_name, data);
    if (!NT_SUCCESS(status))
    {
        DPRINT1("RtlFindActivationContextSectionString() failed with status %x\n", status);
        return status;
    }

    status = STATUS_SXS_KEY_NOT_FOUND;

    /* if there is no data, but params are valid,
       we return that sxs key is not found to be at least somehow compatible */
    if (!data)
    {
        DPRINT("RtlFindActivationContextSectionString() failed with status %x\n", status);
        return status;
    }

    ASSERT(NtCurrentTeb());
    ASSERT(NtCurrentTeb()->ActivationContextStackPointer);

    DPRINT("ActiveFrame: %p\n",NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame);
    if (NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame)
    {
        ACTIVATION_CONTEXT *actctx = check_actctx(NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame->ActivationContext);
        if (actctx) status = find_string( actctx, section_kind, section_name, flags, data );
    }

    DPRINT("status %x\n", status);
    if (status != STATUS_SUCCESS)
        status = find_string( process_actctx, section_kind, section_name, flags, data );

    DPRINT("RtlFindActivationContextSectionString() returns status %x\n", status);
    return status;
}

/***********************************************************************
 *		RtlFindActivationContextSectionGuid (NTDLL.@)
 *
 * Find information about a GUID in an activation context.
 * FIXME: function signature/prototype may be wrong
 */
NTSTATUS WINAPI RtlFindActivationContextSectionGuid( ULONG flags, const GUID *extguid, ULONG section_kind,
                                                     const GUID *guid, void *ptr )
{
    ACTCTX_SECTION_KEYED_DATA *data = ptr;
    NTSTATUS status = STATUS_SXS_KEY_NOT_FOUND;

    if (extguid)
    {
        DPRINT1("expected extguid == NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (flags & ~FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX)
    {
        DPRINT1("unknown flags %08x\n", flags);
        return STATUS_INVALID_PARAMETER;
    }

    if (!data || data->cbSize < FIELD_OFFSET(ACTCTX_SECTION_KEYED_DATA, ulAssemblyRosterIndex) || !guid)
        return STATUS_INVALID_PARAMETER;

    if (NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame)
    {
        ACTIVATION_CONTEXT *actctx = check_actctx(NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame->ActivationContext);
        if (actctx) status = find_guid( actctx, section_kind, guid, flags, data );
    }

    if (status != STATUS_SUCCESS)
        status = find_guid( process_actctx, section_kind, guid, flags, data );

    return status;
}

/* Stubs */

NTSTATUS
NTAPI
RtlAllocateActivationContextStack(IN PACTIVATION_CONTEXT_STACK *Stack)
{
    PACTIVATION_CONTEXT_STACK ContextStack;

    /* Check if it's already allocated */
    if (*Stack) return STATUS_SUCCESS;

    /* Allocate space for the context stack */
    ContextStack = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ACTIVATION_CONTEXT_STACK));
    if (!ContextStack)
    {
        return STATUS_NO_MEMORY;
    }

    /* Initialize the context stack */
    ContextStack->Flags = 0;
    ContextStack->ActiveFrame = NULL;
    InitializeListHead(&ContextStack->FrameListCache);
    ContextStack->NextCookieSequenceNumber = 1;
    ContextStack->StackId = 1; //TODO: Timer-based

    *Stack = ContextStack;

    return STATUS_SUCCESS;
}

PRTL_ACTIVATION_CONTEXT_STACK_FRAME
FASTCALL
RtlActivateActivationContextUnsafeFast(IN PRTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED Frame,
                                       IN PVOID Context)
{
    RTL_ACTIVATION_CONTEXT_STACK_FRAME *ActiveFrame;

    /* Get the curren active frame */
    ActiveFrame = NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame;

    DPRINT("ActiveSP %p, ActiveFrame %p, &Frame->Frame %p, Context %p\n",
        NtCurrentTeb()->ActivationContextStackPointer, ActiveFrame,
        &Frame->Frame, Context);

    /* Actually activate it */
    Frame->Frame.Previous = ActiveFrame;
    Frame->Frame.ActivationContext = Context;
    Frame->Frame.Flags = 0;

    /* Check if we can activate this context */
    if ((ActiveFrame && (ActiveFrame->ActivationContext != Context)) ||
        Context)
    {
        /* Set new active frame */
        NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame = &Frame->Frame;
        return &Frame->Frame;
    }

    /* We can get here only one way: it was already activated */
    DPRINT("Trying to activate improper activation context\n");

    /* Activate only if we are allowing multiple activation */
    if (!RtlpNotAllowingMultipleActivation)
    {
        NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame = &Frame->Frame;
    }
    else
    {
        /* Set flag */
        Frame->Frame.Flags = 0x30;
    }

    /* Return pointer to the activation frame */
    return &Frame->Frame;
}

PRTL_ACTIVATION_CONTEXT_STACK_FRAME
FASTCALL
RtlDeactivateActivationContextUnsafeFast(IN PRTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED Frame)
{
    RTL_ACTIVATION_CONTEXT_STACK_FRAME *frame;
    //RTL_ACTIVATION_CONTEXT_STACK_FRAME *top;

    /* find the right frame */
    //top = NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame;
    frame = &Frame->Frame;

    if (!frame)
    {
        DPRINT1("No top frame!\n");
        RtlRaiseStatus( STATUS_SXS_INVALID_DEACTIVATION );
    }

    DPRINT("Deactivated actctx %p, active frame %p, new active frame %p\n", NtCurrentTeb()->ActivationContextStackPointer, frame, frame->Previous);
    /* pop everything up to and including frame */
    NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame = frame->Previous;

    return frame;
}
