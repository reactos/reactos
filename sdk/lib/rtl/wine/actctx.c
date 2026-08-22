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
 *                  Stefan Ginsberg (stefan.ginsberg@reactos.org)
 *                  Samuel Serapión
 */

/* Based on Wine 3.2-37c98396 */
#ifdef __REACTOS__
#include <stdlib.h>

#include <rtl.h>
#include <ntstrsafe.h>
#include <compat_undoc.h>

#include <wine/unicode.h>
#include "wine/exception.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(actctx);

#define GetProcessHeap() RtlGetProcessHeap()
#define GetCurrentProcess() NtCurrentProcess()
#define DPRINT1 FIXME
#define DPRINT TRACE
#define FILE_END_OF_FILE_INFORMATION FILE_STANDARD_INFORMATION
#define FileEndOfFileInformation FileStandardInformation
#define RELATIVE_PATH RtlPathTypeRelative
#define windows_dir SharedUserData->NtSystemRoot
#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
#define wcsnicmp _wcsnicmp
#define swprintf _snwprintf
#define wcsicmp _wcsicmp
extern LPCSTR debugstr_us( const UNICODE_STRING *str ) DECLSPEC_HIDDEN;

#undef RT_MANIFEST
#undef CREATEPROCESS_MANIFEST_RESOURCE_ID

BOOLEAN RtlpNotAllowingMultipleActivation;

#endif // __REACTOS__

#define ACTCTX_FLAGS_ALL (\
 ACTCTX_FLAG_PROCESSOR_ARCHITECTURE_VALID |\
 ACTCTX_FLAG_LANGID_VALID |\
 ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID |\
 ACTCTX_FLAG_RESOURCE_NAME_VALID |\
 ACTCTX_FLAG_SET_PROCESS_DEFAULT |\
 ACTCTX_FLAG_APPLICATION_NAME_VALID |\
 ACTCTX_FLAG_SOURCE_IS_ASSEMBLYREF |\
 ACTCTX_FLAG_HMODULE_VALID )

#define ACTCTX_MAGIC       0xC07E3E11
#define STRSECTION_MAGIC   0x64487353 /* dHsS */
#define GUIDSECTION_MAGIC  0x64487347 /* dHsG */

#define ACTCTX_FAKE_HANDLE ((HANDLE) 0xf00baa)

/* we don't want to include winuser.h */
#define RT_MANIFEST                        ((ULONG_PTR)24)
#define CREATEPROCESS_MANIFEST_RESOURCE_ID ((ULONG_PTR)1)

#ifndef __REACTOS__ // defined in oaidl.h
/* from oaidl.h */
typedef enum tagLIBFLAGS {
    LIBFLAG_FRESTRICTED   = 0x1,
    LIBFLAG_FCONTROL      = 0x2,
    LIBFLAG_FHIDDEN       = 0x4,
    LIBFLAG_FHASDISKIMAGE = 0x8
} LIBFLAGS;

/* from oleidl.idl */
typedef enum tagOLEMISC
{
    OLEMISC_RECOMPOSEONRESIZE            = 0x1,
    OLEMISC_ONLYICONIC                   = 0x2,
    OLEMISC_INSERTNOTREPLACE             = 0x4,
    OLEMISC_STATIC                       = 0x8,
    OLEMISC_CANTLINKINSIDE               = 0x10,
    OLEMISC_CANLINKBYOLE1                = 0x20,
    OLEMISC_ISLINKOBJECT                 = 0x40,
    OLEMISC_INSIDEOUT                    = 0x80,
    OLEMISC_ACTIVATEWHENVISIBLE          = 0x100,
    OLEMISC_RENDERINGISDEVICEINDEPENDENT = 0x200,
    OLEMISC_INVISIBLEATRUNTIME           = 0x400,
    OLEMISC_ALWAYSRUN                    = 0x800,
    OLEMISC_ACTSLIKEBUTTON               = 0x1000,
    OLEMISC_ACTSLIKELABEL                = 0x2000,
    OLEMISC_NOUIACTIVATE                 = 0x4000,
    OLEMISC_ALIGNABLE                    = 0x8000,
    OLEMISC_SIMPLEFRAME                  = 0x10000,
    OLEMISC_SETCLIENTSITEFIRST           = 0x20000,
    OLEMISC_IMEMODE                      = 0x40000,
    OLEMISC_IGNOREACTIVATEWHENVISIBLE    = 0x80000,
    OLEMISC_WANTSTOMENUMERGE             = 0x100000,
    OLEMISC_SUPPORTSMULTILEVELUNDO       = 0x200000
} OLEMISC;
#endif // !__REACTOS__

#define MAX_NAMESPACES 64

typedef struct
{
    const WCHAR        *ptr;
    unsigned int        len;
} xmlstr_t;

struct xml_elem
{
    xmlstr_t            name;
    xmlstr_t            ns;
    int                 ns_pos;
};

struct xml_attr
{
    xmlstr_t            name;
    xmlstr_t            value;
};

typedef struct
{
    const WCHAR        *ptr;
    const WCHAR        *end;
    struct xml_attr     namespaces[MAX_NAMESPACES];
    int                 ns_pos;
    BOOL                error;
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
    ULONG flags;
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

struct activatable_class_data
{
    ULONG size;
    DWORD unk;
    DWORD module_len;
    DWORD module_offset;
    DWORD threading_model;
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

   - WinRT activatable class section is a plain buffer with following format:

   <section header>
   <module names[]>
   <index[]>
   <data[]> --- <class name>
                <data>

   Header is fixed length structure - struct strsection_header,
   contains classes count;

   Index is an array of fixed length index records, each record is
   struct string_index.

   All strings in data itself are WCHAR, null terminated, 4-bytes aligned.

   All offsets are relative to section itself.

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
        struct
        {
            WCHAR *name;
            WCHAR *value;
            WCHAR *ns;
        } settings;
        struct
        {
            WCHAR *name;
            DWORD  threading_model;
        } activatable_class;
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
    WCHAR                *load_from;
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
    enum assembly_type             type;
    struct assembly_identity       id;
    struct file_info               manifest;
    WCHAR                         *directory;
    BOOL                           no_inherit;
    struct dll_redirect           *dlls;
    unsigned int                   num_dlls;
    unsigned int                   allocated_dlls;
    struct entity_array            entities;
    COMPATIBILITY_CONTEXT_ELEMENT *compat_contexts;
    ULONG                          num_compat_contexts;
    ACTCTX_REQUESTED_RUN_LEVEL     run_level;
    ULONG                          ui_access;
};

enum context_sections
{
    WINDOWCLASS_SECTION    = 1,
    DLLREDIRECT_SECTION    = 2,
    TLIBREDIRECT_SECTION   = 4,
    SERVERREDIRECT_SECTION = 8,
    IFACEREDIRECT_SECTION  = 16,
    CLRSURROGATES_SECTION  = 32,
    PROGIDREDIRECT_SECTION = 64,
    ACTIVATABLE_CLASS_SECTION = 128,
};

#ifdef __REACTOS__
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
#endif // __REACTOS__

typedef struct _ACTIVATION_CONTEXT
{
    ULONG               magic;
    LONG                ref_count;
#ifdef __REACTOS__
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
#endif // __REACTOS__
    struct file_info    config;
    struct file_info    appdir;
    struct assembly    *assemblies;
    unsigned int        num_assemblies;
    unsigned int        allocated_assemblies;
    /* section data */
    DWORD               sections;
    struct strsection_header  *wndclass_section;
    struct strsection_header  *dllredirect_section;
    struct strsection_header  *progid_section;
    struct strsection_header  *activatable_class_section;
    struct guidsection_header *tlib_section;
    struct guidsection_header *comserver_section;
    struct guidsection_header *ifaceps_section;
    struct guidsection_header *clrsurrogate_section;
} ACTIVATION_CONTEXT;

struct actctx_loader
{
    ACTIVATION_CONTEXT       *actctx;
    struct assembly_identity *dependencies;
    unsigned int              num_dependencies;
    unsigned int              allocated_dependencies;
};

static const xmlstr_t empty_xmlstr;

#ifdef __i386__
static const WCHAR current_archW[] = L"x86";
#elif defined __aarch64__ || defined __arm64ec__
static const WCHAR current_archW[] = L"arm64";
#elif defined __x86_64__
static const WCHAR current_archW[] = L"amd64";
#elif defined __arm__
static const WCHAR current_archW[] = L"arm";
#else
static const WCHAR current_archW[] = L"none";
#endif

static const WCHAR asmv1W[] = L"urn:schemas-microsoft-com:asm.v1";
static const WCHAR asmv2W[] = L"urn:schemas-microsoft-com:asm.v2";
static const WCHAR asmv3W[] = L"urn:schemas-microsoft-com:asm.v3";
static const WCHAR winrtv1W[] = L"urn:schemas-microsoft-com:winrt.v1";
static const WCHAR compatibilityNSW[] = L"urn:schemas-microsoft-com:compatibility.v1";
static const WCHAR windowsSettings2005NSW[] = L"http://schemas.microsoft.com/SMI/2005/WindowsSettings";
static const WCHAR windowsSettings2011NSW[] = L"http://schemas.microsoft.com/SMI/2011/WindowsSettings";
static const WCHAR windowsSettings2016NSW[] = L"http://schemas.microsoft.com/SMI/2016/WindowsSettings";
static const WCHAR windowsSettings2017NSW[] = L"http://schemas.microsoft.com/SMI/2017/WindowsSettings";
static const WCHAR windowsSettings2019NSW[] = L"http://schemas.microsoft.com/SMI/2019/WindowsSettings";
static const WCHAR windowsSettings2020NSW[] = L"http://schemas.microsoft.com/SMI/2020/WindowsSettings";

struct olemisc_entry
{
    const WCHAR *name;
    OLEMISC value;
};

static const struct olemisc_entry olemisc_values[] =
{
    { L"activatewhenvisible",          OLEMISC_ACTIVATEWHENVISIBLE },
    { L"actslikebutton",               OLEMISC_ACTSLIKEBUTTON },
    { L"actslikelabel",                OLEMISC_ACTSLIKELABEL },
    { L"alignable",                    OLEMISC_ALIGNABLE },
    { L"alwaysrun",                    OLEMISC_ALWAYSRUN },
    { L"canlinkbyole1",                OLEMISC_CANLINKBYOLE1 },
    { L"cantlinkinside",               OLEMISC_CANTLINKINSIDE },
    { L"ignoreactivatewhenvisible",    OLEMISC_IGNOREACTIVATEWHENVISIBLE },
    { L"imemode",                      OLEMISC_IMEMODE },
    { L"insertnotreplace",             OLEMISC_INSERTNOTREPLACE },
    { L"insideout",                    OLEMISC_INSIDEOUT },
    { L"invisibleatruntime",           OLEMISC_INVISIBLEATRUNTIME },
    { L"islinkobject",                 OLEMISC_ISLINKOBJECT },
    { L"nouiactivate",                 OLEMISC_NOUIACTIVATE },
    { L"onlyiconic",                   OLEMISC_ONLYICONIC },
    { L"recomposeonresize",            OLEMISC_RECOMPOSEONRESIZE },
    { L"renderingisdeviceindependent", OLEMISC_RENDERINGISDEVICEINDEPENDENT },
    { L"setclientsitefirst",           OLEMISC_SETCLIENTSITEFIRST },
    { L"simpleframe",                  OLEMISC_SIMPLEFRAME },
    { L"static",                       OLEMISC_STATIC },
    { L"supportsmultilevelundo",       OLEMISC_SUPPORTSMULTILEVELUNDO },
    { L"wantstomenumerge",             OLEMISC_WANTSTOMENUMERGE }
};

static ACTIVATION_CONTEXT system_actctx = { ACTCTX_MAGIC, 1 };
static ACTIVATION_CONTEXT *process_actctx = &system_actctx;
static ACTIVATION_CONTEXT *implicit_actctx = &system_actctx;

static WCHAR *strdupW(const WCHAR* str)
{
    WCHAR*      ptr;

    if (!(ptr = RtlAllocateHeap(GetProcessHeap(), 0, (wcslen(str) + 1) * sizeof(WCHAR))))
        return NULL;
    return wcscpy(ptr, str);
}

static WCHAR *xmlstrdupW(const xmlstr_t* str)
{
    WCHAR *strW;

    if ((strW = RtlAllocateHeap(GetProcessHeap(), 0, (str->len + 1) * sizeof(WCHAR))))
    {
        memcpy( strW, str->ptr, str->len * sizeof(WCHAR) );
        strW[str->len] = 0;
    }
    return strW;
}

static inline BOOL xmlstr_cmp(const xmlstr_t* xmlstr, const WCHAR *str)
{
    return !wcsncmp(xmlstr->ptr, str, xmlstr->len) && !str[xmlstr->len];
}

static inline BOOL xmlstr_cmpi(const xmlstr_t* xmlstr, const WCHAR *str)
{
    return !wcsnicmp(xmlstr->ptr, str, xmlstr->len) && !str[xmlstr->len];
}

static BOOL xml_attr_cmp( const struct xml_attr *attr, const WCHAR *str )
{
    return xmlstr_cmp( &attr->name, str );
}

static BOOL xml_name_cmp( const struct xml_elem *elem1, const struct xml_elem *elem2 )
{
    return (elem1->name.len == elem2->name.len &&
            elem1->ns.len == elem2->ns.len &&
            !wcsncmp( elem1->name.ptr, elem2->name.ptr, elem1->name.len ) &&
            !wcsncmp( elem1->ns.ptr, elem2->ns.ptr, elem1->ns.len ));
}

static inline BOOL xml_elem_cmp(const struct xml_elem *elem, const WCHAR *str, const WCHAR *namespace)
{
    if (!xmlstr_cmp( &elem->name, str )) return FALSE;
    if (xmlstr_cmp( &elem->ns, namespace )) return TRUE;
    if (!wcscmp( namespace, asmv1W ))
    {
        if (xmlstr_cmp( &elem->ns, asmv2W )) return TRUE;
        if (xmlstr_cmp( &elem->ns, asmv3W )) return TRUE;
    }
    else if (!wcscmp( namespace, asmv2W ))
    {
        if (xmlstr_cmp( &elem->ns, asmv3W )) return TRUE;
    }
    return FALSE;
}

static inline BOOL isxmlspace( WCHAR ch )
{
    return (ch == ' ' || ch == '\r' || ch == '\n' || ch == '\t');
}

static inline const char* debugstr_xmlstr(const xmlstr_t* str)
{
    return debugstr_wn(str->ptr, str->len);
}

static inline const char *debugstr_xml_elem( const struct xml_elem *elem )
{
    return wine_dbg_sprintf( "%s ns %s", debugstr_wn( elem->name.ptr, elem->name.len ),
                             debugstr_wn( elem->ns.ptr, elem->ns.len ));
}

static inline const char *debugstr_xml_attr( const struct xml_attr *attr )
{
    return wine_dbg_sprintf( "%s=%s", debugstr_wn( attr->name.ptr, attr->name.len ),
                             debugstr_wn( attr->value.ptr, attr->value.len ));
}

static inline const char* debugstr_version(const struct assembly_version *ver)
{
    return wine_dbg_sprintf("%u.%u.%u.%u", ver->major, ver->minor, ver->build, ver->revision);
}

static NTSTATUS get_module_filename( HMODULE module, UNICODE_STRING *str, unsigned int extra_len )
{
    NTSTATUS status;
    ULONG_PTR magic;
    LDR_DATA_TABLE_ENTRY *pldr;

    LdrLockLoaderLock(0, NULL, &magic);
    status = LdrFindEntryForAddress( module, &pldr );
    if (status == STATUS_SUCCESS)
    {
        if ((str->Buffer = RtlAllocateHeap( GetProcessHeap(), 0,
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
            ptr = RtlReAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                     actctx->assemblies, new_count * sizeof(*assembly) );
        }
        else
        {
            new_count = 4;
            ptr = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, new_count * sizeof(*assembly) );
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
            ptr = RtlReAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                     assembly->dlls, new_count * sizeof(*assembly->dlls) );
        }
        else
        {
            new_count = 4;
            ptr = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, new_count * sizeof(*assembly->dlls) );
        }
        if (!ptr) return NULL;
        assembly->dlls = ptr;
        assembly->allocated_dlls = new_count;
    }
    return &assembly->dlls[assembly->num_dlls++];
}

static PCOMPATIBILITY_CONTEXT_ELEMENT add_compat_context(struct assembly* assembly)
{
    void *ptr;
    if (assembly->num_compat_contexts)
    {
        unsigned int new_count = assembly->num_compat_contexts + 1;
        ptr = RtlReAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                 assembly->compat_contexts,
                                 new_count * sizeof(COMPATIBILITY_CONTEXT_ELEMENT) );
    }
    else
    {
        ptr = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(COMPATIBILITY_CONTEXT_ELEMENT) );
    }
    if (!ptr) return NULL;
    assembly->compat_contexts = ptr;
    return &assembly->compat_contexts[assembly->num_compat_contexts++];
}

static void free_assembly_identity(struct assembly_identity *ai)
{
    RtlFreeHeap( GetProcessHeap(), 0, ai->name );
    RtlFreeHeap( GetProcessHeap(), 0, ai->arch );
    RtlFreeHeap( GetProcessHeap(), 0, ai->public_key );
    RtlFreeHeap( GetProcessHeap(), 0, ai->language );
    RtlFreeHeap( GetProcessHeap(), 0, ai->type );
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
            ptr = RtlReAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                     array->base, new_count * sizeof(*array->base) );
        }
        else
        {
            new_count = 4;
            ptr = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, new_count * sizeof(*array->base) );
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
            RtlFreeHeap(GetProcessHeap(), 0, entity->u.comclass.clsid);
            RtlFreeHeap(GetProcessHeap(), 0, entity->u.comclass.tlbid);
            RtlFreeHeap(GetProcessHeap(), 0, entity->u.comclass.progid);
            RtlFreeHeap(GetProcessHeap(), 0, entity->u.comclass.name);
            RtlFreeHeap(GetProcessHeap(), 0, entity->u.comclass.version);
            for (j = 0; j < entity->u.comclass.progids.num; j++)
                RtlFreeHeap(GetProcessHeap(), 0, entity->u.comclass.progids.progids[j]);
            RtlFreeHeap(GetProcessHeap(), 0, entity->u.comclass.progids.progids);
            break;
        case ACTIVATION_CONTEXT_SECTION_COM_INTERFACE_REDIRECTION:
            RtlFreeHeap(GetProcessHeap(), 0, entity->u.ifaceps.iid);
            RtlFreeHeap(GetProcessHeap(), 0, entity->u.ifaceps.base);
            RtlFreeHeap(GetProcessHeap(), 0, entity->u.ifaceps.ps32);
            RtlFreeHeap(GetProcessHeap(), 0, entity->u.ifaceps.name);
            RtlFreeHeap(GetProcessHeap(), 0, entity->u.ifaceps.tlib);
            break;
        case ACTIVATION_CONTEXT_SECTION_COM_TYPE_LIBRARY_REDIRECTION:
            RtlFreeHeap(GetProcessHeap(), 0, entity->u.typelib.tlbid);
            RtlFreeHeap(GetProcessHeap(), 0, entity->u.typelib.helpdir);
            break;
        case ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION:
            RtlFreeHeap(GetProcessHeap(), 0, entity->u.class.name);
            break;
        case ACTIVATION_CONTEXT_SECTION_CLR_SURROGATES:
            RtlFreeHeap(GetProcessHeap(), 0, entity->u.clrsurrogate.name);
            RtlFreeHeap(GetProcessHeap(), 0, entity->u.clrsurrogate.clsid);
            RtlFreeHeap(GetProcessHeap(), 0, entity->u.clrsurrogate.version);
            break;
        case ACTIVATION_CONTEXT_SECTION_APPLICATION_SETTINGS:
            RtlFreeHeap(GetProcessHeap(), 0, entity->u.settings.name);
            RtlFreeHeap(GetProcessHeap(), 0, entity->u.settings.value);
            RtlFreeHeap(GetProcessHeap(), 0, entity->u.settings.ns);
            break;
        case ACTIVATION_CONTEXT_SECTION_WINRT_ACTIVATABLE_CLASSES:
            RtlFreeHeap(GetProcessHeap(), 0, entity->u.activatable_class.name);
            break;
        default:
            FIXME("Unknown entity kind %ld\n", entity->kind);
        }
    }
    RtlFreeHeap( GetProcessHeap(), 0, array->base );
}

static BOOL is_matching_string( const WCHAR *str1, const WCHAR *str2 )
{
    if (!str1) return !str2;
    return str2 && !RtlCompareUnicodeStrings( str1, wcslen(str1), str2, wcslen(str2), TRUE );
}

static BOOL is_matching_identity( const struct assembly_identity *id1,
                                  const struct assembly_identity *id2 )
{
    if (!is_matching_string( id1->name, id2->name )) return FALSE;
    if (!is_matching_string( id1->arch, id2->arch )) return FALSE;
    if (!is_matching_string( id1->public_key, id2->public_key )) return FALSE;

    if (id1->language && id2->language && !is_matching_string( id1->language, id2->language ))
    {
        if (wcscmp( L"*", id1->language ) && wcscmp( L"*", id2->language ))
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
            TRACE( "reusing existing assembly for %s arch %s version %u.%u.%u.%u\n",
                   debugstr_w(ai->name), debugstr_w(ai->arch), ai->version.major, ai->version.minor,
                   ai->version.build, ai->version.revision );
            return TRUE;
        }

    for (i = 0; i < acl->num_dependencies; i++)
        if (is_matching_identity( ai, &acl->dependencies[i] ))
        {
            TRACE( "reusing existing dependency for %s arch %s version %u.%u.%u.%u\n",
                   debugstr_w(ai->name), debugstr_w(ai->arch), ai->version.major, ai->version.minor,
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
            ptr = RtlReAllocateHeap(GetProcessHeap(), 0, acl->dependencies,
                                    new_count * sizeof(acl->dependencies[0]));
        }
        else
        {
            new_count = 4;
            ptr = RtlAllocateHeap(GetProcessHeap(), 0, new_count * sizeof(acl->dependencies[0]));
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
    RtlFreeHeap(GetProcessHeap(), 0, acl->dependencies);
}

static WCHAR *build_assembly_dir(struct assembly_identity* ai)
{
    static const WCHAR mskeyW[] = L"deadbeef";
    const WCHAR *arch = ai->arch ? ai->arch : L"none";
    const WCHAR *key = ai->public_key ? ai->public_key : L"none";
    const WCHAR *lang = ai->language ? ai->language : L"none";
    const WCHAR *name = ai->name ? ai->name : L"none";
    SIZE_T size = (wcslen(arch) + 1 + wcslen(name) + 1 + wcslen(key) + 24 + 1 +
		    wcslen(lang) + 1) * sizeof(WCHAR) + sizeof(mskeyW);
    WCHAR *ret;

    if (!(ret = RtlAllocateHeap( GetProcessHeap(), 0, size ))) return NULL;

    wcscpy( ret, arch );
    wcscat( ret, L"_" );
    wcscat( ret, name );
    wcscat( ret, L"_" );
    wcscat( ret, key );
    wcscat( ret, L"_" );
    swprintf( ret + wcslen(ret), size - wcslen(ret), L"%u.%u.%u.%u",
              ai->version.major, ai->version.minor, ai->version.build, ai->version.revision );
    wcscat( ret, L"_" );
    wcscat( ret, lang );
    wcscat( ret, L"_" );
    wcscat( ret, mskeyW );
    return ret;
}

static inline void append_string( WCHAR *buffer, const WCHAR *prefix, const WCHAR *str )
{
    WCHAR *p = buffer;

    if (!str) return;
    wcscat( buffer, prefix );
    p += wcslen(p);
    *p++ = '"';
    wcscpy( p, str );
    p += wcslen(p);
    *p++ = '"';
    *p = 0;
}

static WCHAR *build_assembly_id( const struct assembly_identity *ai )
{
    WCHAR version[64], *ret;
    SIZE_T size = 0;

    swprintf( version, ARRAY_SIZE(version), L"%u.%u.%u.%u",
              ai->version.major, ai->version.minor, ai->version.build, ai->version.revision );
    if (ai->name) size += wcslen(ai->name) * sizeof(WCHAR);
    if (ai->arch) size += wcslen(L",processorArchitecture=") + wcslen(ai->arch) + 2;
    if (ai->public_key) size += wcslen(L",publicKeyToken=") + wcslen(ai->public_key) + 2;
    if (ai->type) size += wcslen(L",type=") + wcslen(ai->type) + 2;
    size += wcslen(L",version=") + wcslen(version) + 2;

    if (!(ret = RtlAllocateHeap( GetProcessHeap(), 0, (size + 1) * sizeof(WCHAR) )))
        return NULL;

    if (ai->name) wcscpy( ret, ai->name );
    else *ret = 0;
    append_string( ret, L",processorArchitecture=", ai->arch );
    append_string( ret, L",publicKeyToken=", ai->public_key );
    append_string( ret, L",type=", ai->type );
    append_string( ret, L",version=", version );
    return ret;
}

static ACTIVATION_CONTEXT *check_actctx( HANDLE h )
{
    ACTIVATION_CONTEXT *ret = NULL, *actctx = h;

    if (!h || h == INVALID_HANDLE_VALUE) return NULL;
    __TRY
    {
        if (actctx->magic == ACTCTX_MAGIC) ret = actctx;
    }
    __EXCEPT_PAGE_FAULT
    {
        DPRINT1("Invalid activation context handle!\n");
    }
    __ENDTRY
    return ret;
}

static inline void actctx_addref( ACTIVATION_CONTEXT *actctx )
{
    InterlockedIncrement( &actctx->ref_count );
}

static void actctx_release( ACTIVATION_CONTEXT *actctx )
{
    if (!InterlockedDecrement( &actctx->ref_count ))
    {
        unsigned int i, j;

        for (i = 0; i < actctx->num_assemblies; i++)
        {
            struct assembly *assembly = &actctx->assemblies[i];
            for (j = 0; j < assembly->num_dlls; j++)
            {
                struct dll_redirect *dll = &assembly->dlls[j];
                free_entity_array( &dll->entities );
                RtlFreeHeap( GetProcessHeap(), 0, dll->name );
                RtlFreeHeap( GetProcessHeap(), 0, dll->load_from );
                RtlFreeHeap( GetProcessHeap(), 0, dll->hash );
            }
            RtlFreeHeap( GetProcessHeap(), 0, assembly->dlls );
            RtlFreeHeap( GetProcessHeap(), 0, assembly->manifest.info );
            RtlFreeHeap( GetProcessHeap(), 0, assembly->directory );
            RtlFreeHeap( GetProcessHeap(), 0, assembly->compat_contexts );
            free_entity_array( &assembly->entities );
            free_assembly_identity(&assembly->id);
        }
        RtlFreeHeap( GetProcessHeap(), 0, actctx->config.info );
        RtlFreeHeap( GetProcessHeap(), 0, actctx->appdir.info );
        RtlFreeHeap( GetProcessHeap(), 0, actctx->assemblies );
        RtlFreeHeap( GetProcessHeap(), 0, actctx->dllredirect_section );
        RtlFreeHeap( GetProcessHeap(), 0, actctx->wndclass_section );
        RtlFreeHeap( GetProcessHeap(), 0, actctx->tlib_section );
        RtlFreeHeap( GetProcessHeap(), 0, actctx->comserver_section );
        RtlFreeHeap( GetProcessHeap(), 0, actctx->ifaceps_section );
        RtlFreeHeap( GetProcessHeap(), 0, actctx->clrsurrogate_section );
        RtlFreeHeap( GetProcessHeap(), 0, actctx->progid_section );
        RtlFreeHeap( GetProcessHeap(), 0, actctx->activatable_class_section );
        actctx->magic = 0;
        RtlFreeHeap( GetProcessHeap(), 0, actctx );
    }
}

static BOOL set_error( xmlbuf_t *xmlbuf )
{
    xmlbuf->error = TRUE;
    return FALSE;
}

static BOOL is_xmlns_attr( const struct xml_attr *attr )
{
    const int len = wcslen( L"xmlns" );
    if (attr->name.len < len) return FALSE;
    if (wcsncmp( attr->name.ptr, L"xmlns", len )) return FALSE;
    return (attr->name.len == len || attr->name.ptr[len] == ':');
}

static void push_xmlns( xmlbuf_t *xmlbuf, const struct xml_attr *attr )
{
    const int len = wcslen( L"xmlns" );
    struct xml_attr *ns;

    if (xmlbuf->ns_pos == MAX_NAMESPACES - 1)
    {
        FIXME( "too many namespaces in manifest\n" );
        set_error( xmlbuf );
        return;
    }
    ns = &xmlbuf->namespaces[xmlbuf->ns_pos++];
    ns->value = attr->value;
    if (attr->name.len > len)
    {
        ns->name.ptr = attr->name.ptr + len + 1;
        ns->name.len = attr->name.len - len - 1;
    }
    else ns->name = empty_xmlstr;
}

static xmlstr_t find_xmlns( xmlbuf_t *xmlbuf, const xmlstr_t *name )
{
    int i;

    for (i = xmlbuf->ns_pos - 1; i >= 0; i--)
    {
        if (xmlbuf->namespaces[i].name.len == name->len &&
            !wcsncmp( xmlbuf->namespaces[i].name.ptr, name->ptr, name->len ))
            return xmlbuf->namespaces[i].value;
    }
    if (xmlbuf->ns_pos) WARN( "namespace %s not found\n", debugstr_xmlstr( name ));
    return empty_xmlstr;
}

static BOOL next_xml_attr(xmlbuf_t *xmlbuf, struct xml_attr *attr, BOOL *end)
{
    const WCHAR* ptr;
    WCHAR quote;

    if (xmlbuf->error) return FALSE;

    while (xmlbuf->ptr < xmlbuf->end && isxmlspace(*xmlbuf->ptr))
        xmlbuf->ptr++;

    if (xmlbuf->ptr == xmlbuf->end) return set_error( xmlbuf );

    if (*xmlbuf->ptr == '/')
    {
        xmlbuf->ptr++;
        if (xmlbuf->ptr == xmlbuf->end || *xmlbuf->ptr != '>')
            return set_error( xmlbuf );

        xmlbuf->ptr++;
        *end = TRUE;
        return FALSE;
    }

    if (*xmlbuf->ptr == '>')
    {
        xmlbuf->ptr++;
        return FALSE;
    }

    ptr = xmlbuf->ptr;
    while (ptr < xmlbuf->end && *ptr != '=' && *ptr != '>' && !isxmlspace(*ptr)) ptr++;

    if (ptr == xmlbuf->end) return set_error( xmlbuf );

    attr->name.ptr = xmlbuf->ptr;
    attr->name.len = ptr-xmlbuf->ptr;
    xmlbuf->ptr = ptr;

    /* skip spaces before '=' */
    while (ptr < xmlbuf->end && *ptr != '=' && isxmlspace(*ptr)) ptr++;
    if (ptr == xmlbuf->end || *ptr != '=') return set_error( xmlbuf );

    /* skip '=' itself */
    ptr++;
    if (ptr == xmlbuf->end) return set_error( xmlbuf );

    /* skip spaces after '=' */
    while (ptr < xmlbuf->end && *ptr != '"' && *ptr != '\'' && isxmlspace(*ptr)) ptr++;

    if (ptr == xmlbuf->end || (*ptr != '"' && *ptr != '\'')) return set_error( xmlbuf );

    quote = *ptr++;
    attr->value.ptr = ptr;
    if (ptr == xmlbuf->end) return set_error( xmlbuf );

    while (ptr < xmlbuf->end && *ptr != quote) ptr++;
    if (ptr == xmlbuf->end)
    {
        xmlbuf->ptr = xmlbuf->end;
        return set_error( xmlbuf );
    }

    attr->value.len = ptr - attr->value.ptr;
    xmlbuf->ptr = ptr + 1;
    if (xmlbuf->ptr != xmlbuf->end) return TRUE;

    return set_error( xmlbuf );
}

static void read_xml_elem( xmlbuf_t *xmlbuf, struct xml_elem *elem )
{
    const WCHAR* ptr = xmlbuf->ptr;

    elem->ns = empty_xmlstr;
    elem->name.ptr = ptr;
    while (ptr < xmlbuf->end && !isxmlspace(*ptr) && *ptr != '>' && *ptr != '/')
    {
        if (*ptr == ':')
        {
            elem->ns.ptr = elem->name.ptr;
            elem->ns.len = ptr - elem->ns.ptr;
            elem->name.ptr = ptr + 1;
        }
        ptr++;
    }
    elem->name.len = ptr - elem->name.ptr;
    xmlbuf->ptr = ptr;
}

static BOOL next_xml_elem( xmlbuf_t *xmlbuf, struct xml_elem *elem, const struct xml_elem *parent )
{
    const WCHAR* ptr;
    struct xml_attr attr;
    xmlbuf_t attr_buf;
    BOOL end = FALSE;

    xmlbuf->ns_pos = parent->ns_pos;  /* restore namespace stack to parent state */

    if (xmlbuf->error) return FALSE;

    for (;;)
    {
        for (ptr = xmlbuf->ptr; ptr < xmlbuf->end; ptr++) if (*ptr == '<') break;
        if (ptr == xmlbuf->end)
        {
            xmlbuf->ptr = xmlbuf->end;
            return set_error( xmlbuf );
        }
        ptr++;
        if (ptr + 3 < xmlbuf->end && ptr[0] == '!' && ptr[1] == '-' && ptr[2] == '-') /* skip comment */
        {
            for (ptr += 3; ptr + 3 <= xmlbuf->end; ptr++)
                if (ptr[0] == '-' && ptr[1] == '-' && ptr[2] == '>') break;

            if (ptr + 3 > xmlbuf->end)
            {
                xmlbuf->ptr = xmlbuf->end;
                return set_error( xmlbuf );
            }
            xmlbuf->ptr = ptr + 3;
        }
        else break;
    }

    xmlbuf->ptr = ptr;
    /* check for element terminating the parent element */
    if (ptr < xmlbuf->end && *ptr == '/')
    {
        xmlbuf->ptr++;
        read_xml_elem( xmlbuf, elem );
        elem->ns = find_xmlns( xmlbuf, &elem->ns );
        if (!xml_name_cmp( elem, parent ))
        {
            ERR( "wrong closing element %s for %s\n",
                 debugstr_xmlstr(&elem->name), debugstr_xmlstr(&parent->name ));
            return set_error( xmlbuf );
        }
        while (xmlbuf->ptr < xmlbuf->end && isxmlspace(*xmlbuf->ptr)) xmlbuf->ptr++;
        if (xmlbuf->ptr == xmlbuf->end || *xmlbuf->ptr++ != '>') return set_error( xmlbuf );
        return FALSE;
    }

    read_xml_elem( xmlbuf, elem );

    /* parse namespace attributes */
    attr_buf = *xmlbuf;
    while (next_xml_attr( &attr_buf, &attr, &end ))
    {
        if (is_xmlns_attr( &attr )) push_xmlns( xmlbuf, &attr );
    }
    elem->ns = find_xmlns( xmlbuf, &elem->ns );
    elem->ns_pos = xmlbuf->ns_pos;

    if (xmlbuf->ptr != xmlbuf->end) return TRUE;

    return set_error( xmlbuf );
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
    const WCHAR *ptr;

    if (xmlbuf->error) return FALSE;

    for (ptr = xmlbuf->ptr; ptr < xmlbuf->end; ptr++) if (*ptr == '<') break;
    if (ptr == xmlbuf->end) return set_error( xmlbuf );

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
    FIXME( "Wrong version definition in manifest file (%s)\n", debugstr_xmlstr(str) );
    return FALSE;
}

static void parse_expect_no_attr(xmlbuf_t* xmlbuf, BOOL* end)
{
    struct xml_attr attr;

    while (next_xml_attr(xmlbuf, &attr, end))
    {
        if (!is_xmlns_attr( &attr )) WARN("unexpected attr %s\n", debugstr_xml_attr(&attr));
    }
}

static void parse_expect_end_elem( xmlbuf_t *xmlbuf, const struct xml_elem *parent )
{
    struct xml_elem elem;

    if (next_xml_elem(xmlbuf, &elem, parent))
    {
        FIXME( "unexpected element %s\n", debugstr_xml_elem(&elem) );
        set_error( xmlbuf );
    }
}

static void parse_unknown_elem(xmlbuf_t *xmlbuf, const struct xml_elem *parent)
{
    struct xml_elem elem;
    struct xml_attr attr;
    BOOL end = FALSE;

    while (next_xml_attr(xmlbuf, &attr, &end));
    if (end) return;

    while (next_xml_elem(xmlbuf, &elem, parent))
        parse_unknown_elem(xmlbuf, &elem);
}

static void parse_assembly_identity_elem(xmlbuf_t *xmlbuf, ACTIVATION_CONTEXT *actctx,
                                         struct assembly_identity* ai, const struct xml_elem *parent)
{
    struct xml_attr attr;
    BOOL end = FALSE;

    while (next_xml_attr(xmlbuf, &attr, &end))
    {
        if (xml_attr_cmp(&attr, L"name"))
        {
            if (!(ai->name = xmlstrdupW(&attr.value))) set_error( xmlbuf );
        }
        else if (xml_attr_cmp(&attr, L"type"))
        {
            if (!(ai->type = xmlstrdupW(&attr.value))) set_error( xmlbuf );
        }
        else if (xml_attr_cmp(&attr, L"version"))
        {
            if (!parse_version(&attr.value, &ai->version)) set_error( xmlbuf );
        }
        else if (xml_attr_cmp(&attr, L"processorArchitecture"))
        {
            if (!(ai->arch = xmlstrdupW(&attr.value))) set_error( xmlbuf );
        }
        else if (xml_attr_cmp(&attr, L"publicKeyToken"))
        {
            if (!(ai->public_key = xmlstrdupW(&attr.value))) set_error( xmlbuf );
        }
        else if (xml_attr_cmp(&attr, L"language"))
        {
            if (!(ai->language = xmlstrdupW(&attr.value))) set_error( xmlbuf );
        }
        else if (!is_xmlns_attr( &attr ))
        {
            WARN("unknown attr %s\n", debugstr_xml_attr(&attr));
        }
    }

    TRACE( "name=%s version=%s arch=%s\n",
           debugstr_w(ai->name), debugstr_version(&ai->version), debugstr_w(ai->arch) );

    if (!end) parse_expect_end_elem(xmlbuf, parent);
}

static enum comclass_threadingmodel parse_com_class_threadingmodel(xmlstr_t *value)
{
    if (value->len == 0) return ThreadingModel_No;
    if (xmlstr_cmp(value, L"Apartment"))
        return ThreadingModel_Apartment;
    else if (xmlstr_cmp(value, L"Free"))
        return ThreadingModel_Free;
    else if (xmlstr_cmp(value, L"Both"))
        return ThreadingModel_Both;
    else if (xmlstr_cmp(value, L"Neutral"))
        return ThreadingModel_Neutral;
    else
        return ThreadingModel_No;
};

static OLEMISC get_olemisc_value(const WCHAR *str, int len)
{
    int min, max;

    min = 0;
    max = ARRAY_SIZE(olemisc_values) - 1;

    while (min <= max)
    {
        int n, c;

        n = (min+max)/2;

        c = wcsncmp(olemisc_values[n].name, str, len);
        if (!c && !olemisc_values[n].name[len])
            return olemisc_values[n].value;

        if (c >= 0)
            max = n-1;
        else
            min = n+1;
    }

    WARN("unknown flag %s\n", debugstr_wn(str, len));
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
        if (!(progids->progids = RtlAllocateHeap(GetProcessHeap(), 0, progids->allocated * sizeof(WCHAR*)))) return FALSE;
    }

    if (progids->allocated == progids->num)
    {
        WCHAR **new_progids = RtlReAllocateHeap(GetProcessHeap(), 0, progids->progids,
                                                2 * progids->allocated * sizeof(WCHAR*));
        if (!new_progids) return FALSE;
        progids->allocated *= 2;
        progids->progids = new_progids;
    }

    if (!(progids->progids[progids->num] = xmlstrdupW(progid))) return FALSE;
    progids->num++;

    return TRUE;
}

static void parse_com_class_progid(xmlbuf_t *xmlbuf, struct entity *entity, const struct xml_elem *parent)
{
    xmlstr_t content;
    BOOL end = FALSE;

    parse_expect_no_attr(xmlbuf, &end);
    if (end) set_error( xmlbuf );
    if (!parse_text_content(xmlbuf, &content)) return;

    if (!com_class_add_progid(&content, entity)) set_error( xmlbuf );
    parse_expect_end_elem(xmlbuf, parent);
}

static void parse_com_class_elem( xmlbuf_t *xmlbuf, struct dll_redirect *dll, struct actctx_loader *acl,
                                  const struct xml_elem *parent )
{
    struct xml_elem elem;
    struct xml_attr attr;
    BOOL end = FALSE;
    struct entity*      entity;

    if (!(entity = add_entity(&dll->entities, ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION)))
    {
        set_error( xmlbuf );
        return;
    }

    while (next_xml_attr(xmlbuf, &attr, &end))
    {
        if (xml_attr_cmp(&attr, L"clsid"))
        {
            if (!(entity->u.comclass.clsid = xmlstrdupW(&attr.value))) set_error( xmlbuf );
        }
        else if (xml_attr_cmp(&attr, L"progid"))
        {
            if (!(entity->u.comclass.progid = xmlstrdupW(&attr.value))) set_error( xmlbuf );
        }
        else if (xml_attr_cmp(&attr, L"tlbid"))
        {
            if (!(entity->u.comclass.tlbid = xmlstrdupW(&attr.value))) set_error( xmlbuf );
        }
        else if (xml_attr_cmp(&attr, L"threadingModel"))
        {
            entity->u.comclass.model = parse_com_class_threadingmodel(&attr.value);
        }
        else if (xml_attr_cmp(&attr, L"miscStatus"))
        {
            entity->u.comclass.miscstatus = parse_com_class_misc(&attr.value);
        }
        else if (xml_attr_cmp(&attr, L"miscStatusContent"))
        {
            entity->u.comclass.miscstatuscontent = parse_com_class_misc(&attr.value);
        }
        else if (xml_attr_cmp(&attr, L"miscStatusThumbnail"))
        {
            entity->u.comclass.miscstatusthumbnail = parse_com_class_misc(&attr.value);
        }
        else if (xml_attr_cmp(&attr, L"miscStatusIcon"))
        {
            entity->u.comclass.miscstatusicon = parse_com_class_misc(&attr.value);
        }
        else if (xml_attr_cmp(&attr, L"miscStatusDocPrint"))
        {
            entity->u.comclass.miscstatusdocprint = parse_com_class_misc(&attr.value);
        }
        else if (xml_attr_cmp(&attr, L"description"))
        {
            /* not stored */
        }
        else if (!is_xmlns_attr( &attr ))
        {
            WARN("unknown attr %s\n", debugstr_xml_attr(&attr));
        }
    }

    acl->actctx->sections |= SERVERREDIRECT_SECTION;
    if (entity->u.comclass.progid)
        acl->actctx->sections |= PROGIDREDIRECT_SECTION;

    if (end) return;

    while (next_xml_elem(xmlbuf, &elem, parent))
    {
        if (xml_elem_cmp(&elem, L"progid", asmv1W))
        {
            parse_com_class_progid(xmlbuf, entity, &elem);
        }
        else
        {
            WARN("unknown elem %s\n", debugstr_xml_elem(&elem));
            parse_unknown_elem(xmlbuf, &elem);
        }
    }

    if (entity->u.comclass.progids.num)
        acl->actctx->sections |= PROGIDREDIRECT_SECTION;
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
            ERR("wrong numeric value %s\n", debugstr_xmlstr(str));
            return FALSE;
        }
    }
    entity->u.ifaceps.nummethods = num;

    return TRUE;
}

static void parse_add_interface_class( xmlbuf_t *xmlbuf, struct entity_array *entities,
        struct actctx_loader *acl, WCHAR *clsid )
{
    struct entity *entity;
    WCHAR *str;

    if (!clsid) return;

    if (!(str = strdupW(clsid)))
    {
        set_error( xmlbuf );
        return;
    }

    if (!(entity = add_entity(entities, ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION)))
    {
        RtlFreeHeap(GetProcessHeap(), 0, str);
        set_error( xmlbuf );
        return;
    }

    entity->u.comclass.clsid = str;
    entity->u.comclass.model = ThreadingModel_Both;

    acl->actctx->sections |= SERVERREDIRECT_SECTION;
}

static void parse_cominterface_proxy_stub_elem( xmlbuf_t *xmlbuf, struct dll_redirect *dll,
                                                struct actctx_loader *acl, const struct xml_elem *parent )
{
    WCHAR *psclsid = NULL;
    struct entity *entity;
    struct xml_attr attr;
    BOOL end = FALSE;

    if (!(entity = add_entity(&dll->entities, ACTIVATION_CONTEXT_SECTION_COM_INTERFACE_REDIRECTION)))
    {
        set_error( xmlbuf );
        return;
    }

    while (next_xml_attr(xmlbuf, &attr, &end))
    {
        if (xml_attr_cmp(&attr, L"iid"))
        {
            if (!(entity->u.ifaceps.iid = xmlstrdupW(&attr.value))) set_error( xmlbuf );
        }
        else if (xml_attr_cmp(&attr, L"name"))
        {
            if (!(entity->u.ifaceps.name = xmlstrdupW(&attr.value))) set_error( xmlbuf );
        }
        else if (xml_attr_cmp(&attr, L"baseInterface"))
        {
            if (!(entity->u.ifaceps.base = xmlstrdupW(&attr.value))) set_error( xmlbuf );
            entity->u.ifaceps.mask |= BaseIface;
        }
        else if (xml_attr_cmp(&attr, L"numMethods"))
        {
            if (!(parse_nummethods(&attr.value, entity))) set_error( xmlbuf );
            entity->u.ifaceps.mask |= NumMethods;
        }
        else if (xml_attr_cmp(&attr, L"tlbid"))
        {
            if (!(entity->u.ifaceps.tlib = xmlstrdupW(&attr.value))) set_error( xmlbuf );
        }
        else if (xml_attr_cmp(&attr, L"proxyStubClsid32"))
        {
            if (!(psclsid = xmlstrdupW(&attr.value))) set_error( xmlbuf );
        }
        /* not used */
        else if (xml_attr_cmp(&attr, L"threadingModel"))
        {
        }
        else if (!is_xmlns_attr( &attr ))
        {
            WARN("unknown attr %s\n", debugstr_xml_attr(&attr));
        }
    }

    acl->actctx->sections |= IFACEREDIRECT_SECTION;
    if (!end) parse_expect_end_elem(xmlbuf, parent);

    parse_add_interface_class(xmlbuf, &dll->entities, acl, psclsid ? psclsid : entity->u.ifaceps.iid);

    RtlFreeHeap(GetProcessHeap(), 0, psclsid);
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

        if (!wcsnicmp(start, L"RESTRICTED", str-start))
            *flags |= LIBFLAG_FRESTRICTED;
        else if (!wcsnicmp(start, L"CONTROL", str-start))
            *flags |= LIBFLAG_FCONTROL;
        else if (!wcsnicmp(start, L"HIDDEN", str-start))
            *flags |= LIBFLAG_FHIDDEN;
        else if (!wcsnicmp(start, L"HASDISKIMAGE", str-start))
            *flags |= LIBFLAG_FHASDISKIMAGE;
        else
        {
            WARN("unknown flags value %s\n", debugstr_xmlstr(value));
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
    FIXME("wrong typelib version value (%s)\n", debugstr_xmlstr(str));
    return FALSE;
}

static void parse_typelib_elem( xmlbuf_t *xmlbuf, struct dll_redirect *dll,
                                struct actctx_loader *acl, const struct xml_elem *parent )
{
    struct xml_attr attr;
    BOOL end = FALSE;
    struct entity*      entity;

    if (!(entity = add_entity(&dll->entities, ACTIVATION_CONTEXT_SECTION_COM_TYPE_LIBRARY_REDIRECTION)))
    {
        set_error( xmlbuf );
        return;
    }

    while (next_xml_attr(xmlbuf, &attr, &end))
    {
        if (xml_attr_cmp(&attr, L"tlbid"))
        {
            if (!(entity->u.typelib.tlbid = xmlstrdupW(&attr.value))) set_error( xmlbuf );
        }
        else if (xml_attr_cmp(&attr, L"version"))
        {
            if (!parse_typelib_version(&attr.value, entity)) set_error( xmlbuf );
        }
        else if (xml_attr_cmp(&attr, L"helpdir"))
        {
            if (!(entity->u.typelib.helpdir = xmlstrdupW(&attr.value))) set_error( xmlbuf );
        }
        else if (xml_attr_cmp(&attr, L"flags"))
        {
            if (!parse_typelib_flags(&attr.value, entity)) set_error( xmlbuf );
        }
        else if (!is_xmlns_attr( &attr ))
        {
            WARN("unknown attr %s\n", debugstr_xml_attr(&attr));
        }
    }

    acl->actctx->sections |= TLIBREDIRECT_SECTION;
    if (!end) parse_expect_end_elem(xmlbuf, parent);
}

static inline int aligned_string_len(int len)
{
    return (len + 3) & ~3;
}

static int get_assembly_version(struct assembly *assembly, WCHAR *ret)
{
    struct assembly_version *ver = &assembly->id.version;
    WCHAR buff[25];

    if (!ret) ret = buff;
    return swprintf(ret, ARRAY_SIZE(buff), L"%u.%u.%u.%u", ver->major, ver->minor, ver->build, ver->revision);
}

static void parse_window_class_elem( xmlbuf_t *xmlbuf, struct dll_redirect *dll,
                                     struct actctx_loader *acl, const struct xml_elem *parent )
{
    struct xml_elem elem;
    struct xml_attr attr;
    xmlstr_t content;
    BOOL end = FALSE;
    struct entity*      entity;

    if (!(entity = add_entity(&dll->entities, ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION)))
    {
        set_error( xmlbuf );
        return;
    }
    entity->u.class.versioned = TRUE;
    while (next_xml_attr(xmlbuf, &attr, &end))
    {
        if (xml_attr_cmp(&attr, L"versioned"))
        {
            if (xmlstr_cmpi(&attr.value, L"no"))
                entity->u.class.versioned = FALSE;
            else if (!xmlstr_cmpi(&attr.value, L"yes"))
                set_error( xmlbuf );
        }
        else if (!is_xmlns_attr( &attr ))
        {
            WARN("unknown attr %s\n", debugstr_xml_attr(&attr));
        }
    }

    if (end) return;

    if (!parse_text_content(xmlbuf, &content)) return;
    if (!(entity->u.class.name = xmlstrdupW(&content))) set_error( xmlbuf );

    acl->actctx->sections |= WINDOWCLASS_SECTION;

    while (next_xml_elem(xmlbuf, &elem, parent))
    {
        WARN("unknown elem %s\n", debugstr_xml_elem(&elem));
        parse_unknown_elem(xmlbuf, &elem);
    }
}

static void parse_binding_redirect_elem( xmlbuf_t *xmlbuf, const struct xml_elem *parent )
{
    struct xml_attr attr;
    BOOL end = FALSE;

    while (next_xml_attr(xmlbuf, &attr, &end))
    {
        if (xml_attr_cmp(&attr, L"oldVersion"))
        {
            FIXME("Not stored yet %s\n", debugstr_xml_attr(&attr));
        }
        else if (xml_attr_cmp(&attr, L"newVersion"))
        {
            FIXME("Not stored yet %s\n", debugstr_xml_attr(&attr));
        }
        else if (!is_xmlns_attr( &attr ))
        {
            WARN("unknown attr %s\n", debugstr_xml_attr(&attr));
        }
    }

    if (!end) parse_expect_end_elem(xmlbuf, parent);
}

static void parse_description_elem( xmlbuf_t *xmlbuf, const struct xml_elem *parent )
{
    struct xml_elem elem;
    struct xml_attr attr;
    xmlstr_t content;
    BOOL end = FALSE;

    while (next_xml_attr(xmlbuf, &attr, &end))
    {
        if (!is_xmlns_attr( &attr )) WARN("unknown attr %s\n", debugstr_xml_attr(&attr));
    }

    if (end) return;
    if (!parse_text_content(xmlbuf, &content)) return;

    TRACE("Got description %s\n", debugstr_xmlstr(&content));

    while (next_xml_elem(xmlbuf, &elem, parent))
    {
        WARN("unknown elem %s\n", debugstr_xml_elem(&elem));
        parse_unknown_elem(xmlbuf, &elem);
    }
}

static void parse_com_interface_external_proxy_stub_elem(xmlbuf_t *xmlbuf,
                                                         struct assembly* assembly,
                                                         struct actctx_loader* acl,
                                                         const struct xml_elem *parent)
{
    struct xml_attr attr;
    BOOL end = FALSE;
    struct entity*      entity;

    if (!(entity = add_entity(&assembly->entities, ACTIVATION_CONTEXT_SECTION_COM_INTERFACE_REDIRECTION)))
    {
        set_error( xmlbuf );
        return;
    }

    while (next_xml_attr(xmlbuf, &attr, &end))
    {
        if (xml_attr_cmp(&attr, L"iid"))
        {
            if (!(entity->u.ifaceps.iid = xmlstrdupW(&attr.value))) set_error( xmlbuf );
        }
        else if (xml_attr_cmp(&attr, L"name"))
        {
            if (!(entity->u.ifaceps.name = xmlstrdupW(&attr.value))) set_error( xmlbuf );
        }
        else if (xml_attr_cmp(&attr, L"baseInterface"))
        {
            if (!(entity->u.ifaceps.base = xmlstrdupW(&attr.value))) set_error( xmlbuf );
            entity->u.ifaceps.mask |= BaseIface;
        }
        else if (xml_attr_cmp(&attr, L"numMethods"))
        {
            if (!(parse_nummethods(&attr.value, entity))) set_error( xmlbuf );
            entity->u.ifaceps.mask |= NumMethods;
        }
        else if (xml_attr_cmp(&attr, L"proxyStubClsid32"))
        {
            if (!(entity->u.ifaceps.ps32 = xmlstrdupW(&attr.value))) set_error( xmlbuf );
        }
        else if (xml_attr_cmp(&attr, L"tlbid"))
        {
            if (!(entity->u.ifaceps.tlib = xmlstrdupW(&attr.value))) set_error( xmlbuf );
        }
        else if (!is_xmlns_attr( &attr ))
        {
            WARN("unknown attr %s\n", debugstr_xml_attr(&attr));
        }
    }

    acl->actctx->sections |= IFACEREDIRECT_SECTION;
    if (!end) parse_expect_end_elem(xmlbuf, parent);
}

static void parse_clr_class_elem( xmlbuf_t* xmlbuf, struct assembly* assembly,
                                  struct actctx_loader *acl, const struct xml_elem *parent )

{
    struct xml_elem elem;
    struct xml_attr attr;
    BOOL end = FALSE;
    struct entity*      entity;

    if (!(entity = add_entity(&assembly->entities, ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION)))
    {
        set_error( xmlbuf );
        return;
    }

    while (next_xml_attr(xmlbuf, &attr, &end))
    {
        if (xml_attr_cmp(&attr, L"name"))
        {
            if (!(entity->u.comclass.name = xmlstrdupW(&attr.value))) set_error( xmlbuf );
        }
        else if (xml_attr_cmp(&attr, L"clsid"))
        {
            if (!(entity->u.comclass.clsid = xmlstrdupW(&attr.value))) set_error( xmlbuf );
        }
        else if (xml_attr_cmp(&attr, L"progid"))
        {
            if (!(entity->u.comclass.progid = xmlstrdupW(&attr.value))) set_error( xmlbuf );
        }
        else if (xml_attr_cmp(&attr, L"tlbid"))
        {
            if (!(entity->u.comclass.tlbid = xmlstrdupW(&attr.value))) set_error( xmlbuf );
        }
        else if (xml_attr_cmp(&attr, L"threadingModel"))
        {
            entity->u.comclass.model = parse_com_class_threadingmodel(&attr.value);
        }
        else if (xml_attr_cmp(&attr, L"runtimeVersion"))
        {
            if (!(entity->u.comclass.version = xmlstrdupW(&attr.value))) set_error( xmlbuf );
        }
        else if (!is_xmlns_attr( &attr ))
        {
            WARN("unknown attr %s\n", debugstr_xml_attr(&attr));
        }
    }

    acl->actctx->sections |= SERVERREDIRECT_SECTION;
    if (entity->u.comclass.progid)
        acl->actctx->sections |= PROGIDREDIRECT_SECTION;
    if (end) return;

    while (next_xml_elem(xmlbuf, &elem, parent))
    {
        if (xml_elem_cmp(&elem, L"progid", asmv1W))
        {
            parse_com_class_progid(xmlbuf, entity, &elem);
        }
        else
        {
            WARN("unknown elem %s\n", debugstr_xml_elem(&elem));
            parse_unknown_elem(xmlbuf, &elem);
        }
    }

    if (entity->u.comclass.progids.num)
        acl->actctx->sections |= PROGIDREDIRECT_SECTION;
}

static void parse_clr_surrogate_elem( xmlbuf_t *xmlbuf, struct assembly *assembly,
                                      struct actctx_loader *acl, const struct xml_elem *parent )
{
    struct xml_attr attr;
    BOOL end = FALSE;
    struct entity*      entity;

    if (!(entity = add_entity(&assembly->entities, ACTIVATION_CONTEXT_SECTION_CLR_SURROGATES)))
    {
        set_error( xmlbuf );
        return;
    }

    while (next_xml_attr(xmlbuf, &attr, &end))
    {
        if (xml_attr_cmp(&attr, L"name"))
        {
            if (!(entity->u.clrsurrogate.name = xmlstrdupW(&attr.value))) set_error( xmlbuf );
        }
        else if (xml_attr_cmp(&attr, L"clsid"))
        {
            if (!(entity->u.clrsurrogate.clsid = xmlstrdupW(&attr.value))) set_error( xmlbuf );
        }
        else if (xml_attr_cmp(&attr, L"runtimeVersion"))
        {
            if (!(entity->u.clrsurrogate.version = xmlstrdupW(&attr.value))) set_error( xmlbuf );
        }
        else if (!is_xmlns_attr( &attr ))
        {
            WARN("unknown attr %s\n", debugstr_xml_attr(&attr));
        }
    }

    acl->actctx->sections |= CLRSURROGATES_SECTION;
    if (!end) parse_expect_end_elem(xmlbuf, parent);
}

static void parse_dependent_assembly_elem( xmlbuf_t *xmlbuf, struct actctx_loader *acl,
                                           const struct xml_elem *parent, BOOL optional )
{
    struct xml_elem elem;
    struct xml_attr attr;
    struct assembly_identity    ai;
    BOOL end = FALSE;

    memset(&ai, 0, sizeof(ai));
    ai.optional = optional;

    while (next_xml_attr(xmlbuf, &attr, &end))
    {
        if (xml_attr_cmp(&attr, L"allowDelayedBinding"))
            ai.delayed = xmlstr_cmp(&attr.value, L"true");
        else if (!is_xmlns_attr( &attr ))
            WARN("unknown attr %s\n", debugstr_xml_attr(&attr));
    }

    if (end) return;

    while (next_xml_elem(xmlbuf, &elem, parent))
    {
        if (xml_elem_cmp(&elem, L"assemblyIdentity", asmv1W))
        {
            parse_assembly_identity_elem(xmlbuf, acl->actctx, &ai, &elem);
            /* store the newly found identity for later loading */
            if (ai.arch && !wcscmp(ai.arch, L"*"))
            {
                RtlFreeHeap( GetProcessHeap(), 0, ai.arch );
                ai.arch = strdupW( current_archW );
            }
            TRACE( "adding name=%s version=%s arch=%s\n",
                   debugstr_w(ai.name), debugstr_version(&ai.version), debugstr_w(ai.arch) );
            if (!add_dependent_assembly_id(acl, &ai)) set_error( xmlbuf );
        }
        else if (xml_elem_cmp(&elem, L"bindingRedirect", asmv1W))
        {
            parse_binding_redirect_elem(xmlbuf, &elem);
        }
        else
        {
            WARN("unknown elem %s\n", debugstr_xml_elem(&elem));
            parse_unknown_elem(xmlbuf, &elem);
        }
    }
}

static void parse_dependency_elem( xmlbuf_t *xmlbuf, struct actctx_loader *acl,
                                   const struct xml_elem *parent )

{
    struct xml_elem elem;
    struct xml_attr attr;
    BOOL end = FALSE, optional = FALSE;

    while (next_xml_attr(xmlbuf, &attr, &end))
    {
        if (xml_attr_cmp(&attr, L"optional"))
        {
            optional = xmlstr_cmpi( &attr.value, L"yes" );
            TRACE("optional=%s\n", debugstr_xmlstr(&attr.value));
        }
        else if (!is_xmlns_attr( &attr ))
        {
            WARN("unknown attr %s\n", debugstr_xml_attr(&attr));
        }
    }

    if (end) return;

    while (next_xml_elem(xmlbuf, &elem, parent))
    {
        if (xml_elem_cmp(&elem, L"dependentAssembly", asmv1W))
        {
            parse_dependent_assembly_elem(xmlbuf, acl, &elem, optional);
        }
        else
        {
            WARN("unknown element %s\n", debugstr_xml_elem(&elem));
            parse_unknown_elem(xmlbuf, &elem);
        }
    }
}

static void parse_noinherit_elem( xmlbuf_t *xmlbuf, const struct xml_elem *parent )
{
    BOOL end = FALSE;

    parse_expect_no_attr(xmlbuf, &end);
    if (!end) parse_expect_end_elem(xmlbuf, parent);
}

static void parse_noinheritable_elem( xmlbuf_t *xmlbuf, const struct xml_elem *parent )
{
    BOOL end = FALSE;

    parse_expect_no_attr(xmlbuf, &end);
    if (!end) parse_expect_end_elem(xmlbuf, parent);
}

static void parse_activatable_class_elem( xmlbuf_t *xmlbuf, struct dll_redirect *dll,
                                          struct actctx_loader *acl, const struct xml_elem *parent )
{
    struct xml_elem elem;
    struct xml_attr attr;
    BOOL end = FALSE;
    struct entity *entity;

    if (!(entity = add_entity(&dll->entities, ACTIVATION_CONTEXT_SECTION_WINRT_ACTIVATABLE_CLASSES)))
    {
        set_error( xmlbuf );
        return;
    }
    while (next_xml_attr(xmlbuf, &attr, &end))
    {
        if (xml_attr_cmp(&attr, L"name"))
        {
            if (!(entity->u.activatable_class.name = xmlstrdupW(&attr.value)))
                set_error( xmlbuf );
        }
        else if (xml_attr_cmp(&attr, L"threadingModel"))
        {
            if (xmlstr_cmpi(&attr.value, L"both"))
                entity->u.activatable_class.threading_model = 0;
            else if (xmlstr_cmpi(&attr.value, L"sta"))
                entity->u.activatable_class.threading_model = 1;
            else if (xmlstr_cmpi(&attr.value, L"mta"))
                entity->u.activatable_class.threading_model = 2;
            else
                set_error( xmlbuf );
        }
        else if (!is_xmlns_attr( &attr ))
        {
            WARN("unknown attr %s\n", debugstr_xml_attr(&attr));
        }
    }

    acl->actctx->sections |= ACTIVATABLE_CLASS_SECTION;

    if (end) return;

    while (next_xml_elem(xmlbuf, &elem, parent))
    {
        WARN("unknown elem %s\n", debugstr_xml_elem(&elem));
        parse_unknown_elem(xmlbuf, &elem);
    }
}

static void parse_file_elem( xmlbuf_t* xmlbuf, struct assembly* assembly,
                             struct actctx_loader* acl, const struct xml_elem *parent )
{
    struct xml_elem elem;
    struct xml_attr attr;
    BOOL end = FALSE;
    struct dll_redirect* dll;

    if (!(dll = add_dll_redirect(assembly)))
    {
        set_error( xmlbuf );
        return;
    }

    while (next_xml_attr(xmlbuf, &attr, &end))
    {
        if (xml_attr_cmp(&attr, L"name"))
        {
            if (!(dll->name = xmlstrdupW(&attr.value))) set_error( xmlbuf );
            TRACE("name=%s\n", debugstr_xmlstr(&attr.value));
        }
        else if (xml_attr_cmp(&attr, L"loadFrom"))
        {
            if (!(dll->load_from = xmlstrdupW(&attr.value))) set_error( xmlbuf );
        }
        else if (xml_attr_cmp(&attr, L"hash"))
        {
            if (!(dll->hash = xmlstrdupW(&attr.value))) set_error( xmlbuf );
        }
        else if (xml_attr_cmp(&attr, L"hashalg"))
        {
            if (!xmlstr_cmpi(&attr.value, L"SHA1"))
                FIXME("hashalg should be SHA1, got %s\n", debugstr_xmlstr(&attr.value));
        }
        else if (!is_xmlns_attr( &attr ))
        {
            WARN("unknown attr %s\n", debugstr_xml_attr(&attr));
        }
    }

    if (!dll->name) set_error( xmlbuf );

    acl->actctx->sections |= DLLREDIRECT_SECTION;

    if (end) return;

    while (next_xml_elem(xmlbuf, &elem, parent))
    {
        if (xml_elem_cmp(&elem, L"comClass", asmv1W))
        {
            parse_com_class_elem(xmlbuf, dll, acl, &elem);
        }
        else if (xml_elem_cmp(&elem, L"comInterfaceProxyStub", asmv1W))
        {
            parse_cominterface_proxy_stub_elem(xmlbuf, dll, acl, &elem);
        }
        else if (xml_elem_cmp(&elem, L"hash", asmv2W))
        {
            WARN("asmv2:hash (undocumented) not supported\n");
            parse_unknown_elem(xmlbuf, &elem);
        }
        else if (xml_elem_cmp(&elem, L"typelib", asmv1W))
        {
            parse_typelib_elem(xmlbuf, dll, acl, &elem);
        }
        else if (xml_elem_cmp(&elem, L"windowClass", asmv1W))
        {
            parse_window_class_elem(xmlbuf, dll, acl, &elem);
        }
        else if (xml_elem_cmp(&elem, L"activatableClass", winrtv1W))
        {
            parse_activatable_class_elem(xmlbuf, dll, acl, &elem);
        }
        else
        {
            WARN("unknown elem %s\n", debugstr_xml_elem(&elem));
            parse_unknown_elem( xmlbuf, &elem );
        }
    }
}

static void parse_supportedos_elem( xmlbuf_t *xmlbuf, struct assembly *assembly,
                                    struct actctx_loader *acl, const struct xml_elem *parent )
{
    struct xml_attr attr;
    BOOL end = FALSE;

    while (next_xml_attr(xmlbuf, &attr, &end))
    {
        if (xml_attr_cmp(&attr, L"Id"))
        {
            COMPATIBILITY_CONTEXT_ELEMENT *compat;
            UNICODE_STRING str;
            GUID compat_id;

            str.Buffer = (PWSTR)attr.value.ptr;
            str.Length = str.MaximumLength = (USHORT)attr.value.len * sizeof(WCHAR);
            if (RtlGUIDFromString(&str, &compat_id) == STATUS_SUCCESS)
            {
                if (!(compat = add_compat_context(assembly)))
                {
                    set_error( xmlbuf );
                    return;
                }
                compat->Type = ACTCTX_COMPATIBILITY_ELEMENT_TYPE_OS;
                compat->Id = compat_id;
            }
            else
            {
                WARN("Invalid guid %s\n", debugstr_xmlstr(&attr.value));
            }
        }
        else if (!is_xmlns_attr( &attr ))
        {
            WARN("unknown attr %s\n", debugstr_xml_attr(&attr));
        }
    }

    if (!end) parse_expect_end_elem(xmlbuf, parent);
}

#if (NTDDI_VERSION >= NTDDI_WIN10_19H1)
static void parse_maxversiontested_elem( xmlbuf_t *xmlbuf, struct assembly *assembly,
                                         struct actctx_loader *acl, const struct xml_elem *parent )
{
    struct xml_attr attr;
    BOOL end = FALSE;

    while (next_xml_attr(xmlbuf, &attr, &end))
    {
        if (xml_attr_cmp(&attr, L"Id"))
        {
            COMPATIBILITY_CONTEXT_ELEMENT *compat;
            struct assembly_version version;

            if (!(compat = add_compat_context(assembly)))
            {
                set_error( xmlbuf );
                return;
            }
            parse_version( &attr.value, &version );
            compat->Type = ACTCTX_COMPATIBILITY_ELEMENT_TYPE_MAXVERSIONTESTED;
            compat->MaxVersionTested = (ULONGLONG)version.major << 48 |
                (ULONGLONG)version.minor << 32 | version.build << 16 | version.revision;
        }
        else if (!is_xmlns_attr( &attr ))
        {
            WARN("unknown attr %s\n", debugstr_xml_attr(&attr));
        }
    }

    if (!end) parse_expect_end_elem(xmlbuf, parent);
}
#endif // NTDDI_VERSION >= NTDDI_WIN10_19H1

static void parse_compatibility_application_elem(xmlbuf_t *xmlbuf, struct assembly *assembly,
                                                 struct actctx_loader* acl, const struct xml_elem *parent)
{
    struct xml_elem elem;

    while (next_xml_elem(xmlbuf, &elem, parent))
    {
        if (xml_elem_cmp(&elem, L"supportedOS", compatibilityNSW))
        {
            parse_supportedos_elem(xmlbuf, assembly, acl, &elem);
        }
#if (NTDDI_VERSION >= NTDDI_WIN10_19H1)
        else if (xml_elem_cmp(&elem, L"maxversiontested", compatibilityNSW))
        {
            parse_maxversiontested_elem(xmlbuf, assembly, acl, &elem);
        }
#endif // NTDDI_VERSION >= NTDDI_WIN10_19H1
        else
        {
            WARN("unknown elem %s\n", debugstr_xml_elem(&elem));
            parse_unknown_elem(xmlbuf, &elem);
        }
    }
}

static void parse_compatibility_elem(xmlbuf_t *xmlbuf, struct assembly *assembly,
                                     struct actctx_loader* acl, const struct xml_elem *parent)
{
    struct xml_elem elem;

    while (next_xml_elem(xmlbuf, &elem, parent))
    {
        if (xml_elem_cmp(&elem, L"application", compatibilityNSW))
        {
            parse_compatibility_application_elem(xmlbuf, assembly, acl, &elem);
        }
        else
        {
            WARN("unknown elem %s\n", debugstr_xml_elem(&elem));
            parse_unknown_elem(xmlbuf, &elem);
        }
    }
}

static void parse_settings_elem( xmlbuf_t *xmlbuf, struct assembly *assembly, struct actctx_loader *acl,
                                 struct xml_elem *parent )
{
    struct xml_elem elem;
    struct xml_attr attr;
    xmlstr_t content;
    BOOL end = FALSE;
    struct entity *entity;

    while (next_xml_attr( xmlbuf, &attr, &end ))
    {
        if (!is_xmlns_attr( &attr )) WARN( "unknown attr %s\n", debugstr_xml_attr(&attr) );
    }

    if (end) return;

    if (!parse_text_content( xmlbuf, &content )) return;
    TRACE( "got %s %s\n", debugstr_xmlstr(&parent->name), debugstr_xmlstr(&content) );

    entity = add_entity( &assembly->entities, ACTIVATION_CONTEXT_SECTION_APPLICATION_SETTINGS );
    if (!entity)
    {
        set_error( xmlbuf );
        return;
    }
    entity->u.settings.name = xmlstrdupW( &parent->name );
    entity->u.settings.value = xmlstrdupW( &content );
    entity->u.settings.ns = xmlstrdupW( &parent->ns );

    while (next_xml_elem(xmlbuf, &elem, parent))
    {
        WARN( "unknown elem %s\n", debugstr_xml_elem(&elem) );
        parse_unknown_elem( xmlbuf, &elem );
    }
}

static void parse_windows_settings_elem( xmlbuf_t *xmlbuf, struct assembly *assembly,
                                         struct actctx_loader *acl, const struct xml_elem *parent )
{
    struct xml_elem elem;

    while (next_xml_elem( xmlbuf, &elem, parent ))
    {
        if (xml_elem_cmp( &elem, L"activeCodePage", windowsSettings2019NSW ) ||
            xml_elem_cmp( &elem, L"autoElevate", windowsSettings2005NSW ) ||
            xml_elem_cmp( &elem, L"disableTheming", windowsSettings2005NSW ) ||
            xml_elem_cmp( &elem, L"disableWindowFiltering", windowsSettings2011NSW ) ||
            xml_elem_cmp( &elem, L"dpiAware", windowsSettings2005NSW ) ||
            xml_elem_cmp( &elem, L"dpiAwareness", windowsSettings2016NSW ) ||
            xml_elem_cmp( &elem, L"gdiScaling", windowsSettings2017NSW ) ||
            xml_elem_cmp( &elem, L"heapType", windowsSettings2020NSW ) ||
            xml_elem_cmp( &elem, L"highResolutionScrollingAware", windowsSettings2017NSW ) ||
            xml_elem_cmp( &elem, L"longPathAware", windowsSettings2016NSW ) ||
            xml_elem_cmp( &elem, L"magicFutureSetting", windowsSettings2017NSW ) ||
            xml_elem_cmp( &elem, L"printerDriverIsolation", windowsSettings2011NSW ) ||
            xml_elem_cmp( &elem, L"ultraHighResolutionScrollingAware", windowsSettings2017NSW ))
        {
            parse_settings_elem( xmlbuf, assembly, acl, &elem );
        }
        else
        {
            WARN( "unknown elem %s\n", debugstr_xml_elem(&elem) );
            parse_unknown_elem( xmlbuf, &elem );
        }
    }
}

static void parse_application_elem( xmlbuf_t *xmlbuf, struct assembly *assembly,
                                    struct actctx_loader *acl, const struct xml_elem *parent )
{
    struct xml_elem elem;
    struct xml_attr attr;
    BOOL end = FALSE;

    while (next_xml_attr(xmlbuf, &attr, &end))
    {
        if (!is_xmlns_attr( &attr )) WARN( "unknown attr %s\n", debugstr_xml_attr(&attr) );
    }

    if (end) return;

    while (next_xml_elem( xmlbuf, &elem, parent ))
    {
        if (xml_elem_cmp( &elem, L"windowsSettings", asmv1W ))
        {
            parse_windows_settings_elem( xmlbuf, assembly, acl, &elem );
        }
        else
        {
            WARN( "unknown elem %s\n", debugstr_xml_elem(&elem) );
            parse_unknown_elem( xmlbuf, &elem );
        }
    }
}

static void parse_requested_execution_level_elem( xmlbuf_t *xmlbuf, struct assembly *assembly,
                                                  struct actctx_loader *acl, const struct xml_elem *parent )
{
    struct xml_elem elem;
    struct xml_attr attr;
    BOOL end = FALSE;

    /* Multiple requestedExecutionLevel elements are not supported. */
    if (assembly->run_level != ACTCTX_RUN_LEVEL_UNSPECIFIED) set_error( xmlbuf );

    while (next_xml_attr(xmlbuf, &attr, &end))
    {
        if (xml_attr_cmp(&attr, L"level"))
        {
            if (xmlstr_cmpi(&attr.value, L"asInvoker"))
                assembly->run_level = ACTCTX_RUN_LEVEL_AS_INVOKER;
            else if (xmlstr_cmpi(&attr.value, L"highestAvailable"))
                assembly->run_level = ACTCTX_RUN_LEVEL_HIGHEST_AVAILABLE;
            else if (xmlstr_cmpi(&attr.value, L"requireAdministrator"))
                assembly->run_level = ACTCTX_RUN_LEVEL_REQUIRE_ADMIN;
            else
                FIXME("unknown execution level: %s\n", debugstr_xmlstr(&attr.value));
        }
        else if (xml_attr_cmp(&attr, L"uiAccess"))
        {
            if (xmlstr_cmpi(&attr.value, L"false"))
                assembly->ui_access = FALSE;
            else if (xmlstr_cmpi(&attr.value, L"true"))
                assembly->ui_access = TRUE;
            else
                FIXME("unknown uiAccess value: %s\n", debugstr_xmlstr(&attr.value));
        }
        else if (!is_xmlns_attr( &attr ))
            WARN("unknown attr %s\n", debugstr_xml_attr(&attr));
    }

    if (end) return;

    while (next_xml_elem(xmlbuf, &elem, parent))
    {
        WARN("unknown element %s\n", debugstr_xml_elem(&elem));
        parse_unknown_elem(xmlbuf, &elem);
    }
}

static void parse_requested_privileges_elem( xmlbuf_t *xmlbuf, struct assembly *assembly,
                                             struct actctx_loader *acl, const struct xml_elem *parent )
{
    struct xml_elem elem;

    while (next_xml_elem(xmlbuf, &elem, parent))
    {
        if (xml_elem_cmp(&elem, L"requestedExecutionLevel", asmv1W))
        {
            parse_requested_execution_level_elem(xmlbuf, assembly, acl, &elem);
        }
        else
        {
            WARN("unknown elem %s\n", debugstr_xml_elem(&elem));
            parse_unknown_elem(xmlbuf, &elem);
        }
    }
}

static void parse_security_elem( xmlbuf_t *xmlbuf, struct assembly *assembly,
                                 struct actctx_loader *acl, const struct xml_elem *parent )
{
    struct xml_elem elem;

    while (next_xml_elem(xmlbuf, &elem, parent))
    {
        if (xml_elem_cmp(&elem, L"requestedPrivileges", asmv1W))
        {
            parse_requested_privileges_elem(xmlbuf, assembly, acl, &elem);
        }
        else
        {
            WARN("unknown elem %s\n", debugstr_xml_elem(&elem));
            parse_unknown_elem(xmlbuf, &elem);
        }
    }
}

static void parse_trust_info_elem( xmlbuf_t *xmlbuf, struct assembly *assembly,
                                   struct actctx_loader *acl, const struct xml_elem *parent )
{
    struct xml_elem elem;

    while (next_xml_elem(xmlbuf, &elem, parent))
    {
        if (xml_elem_cmp(&elem, L"security", asmv1W))
        {
            parse_security_elem(xmlbuf, assembly, acl, &elem);
        }
        else
        {
            WARN("unknown elem %s\n", debugstr_xml_elem(&elem));
            parse_unknown_elem(xmlbuf, &elem);
        }
    }
}

static void parse_assembly_elem( xmlbuf_t *xmlbuf, struct assembly* assembly,
                                 struct actctx_loader* acl, const struct xml_elem *parent,
                                 struct assembly_identity* expected_ai)
{
    struct xml_elem elem;
    struct xml_attr attr;
    BOOL end = FALSE, version = FALSE;

    TRACE("(%p)\n", xmlbuf);

    while (next_xml_attr(xmlbuf, &attr, &end))
    {
        if (xml_attr_cmp(&attr, L"manifestVersion"))
        {
            if (!xmlstr_cmp(&attr.value, L"1.0"))
            {
                FIXME("wrong version %s\n", debugstr_xmlstr(&attr.value));
                break;
            }
            version = TRUE;
        }
        else if (!is_xmlns_attr( &attr ))
        {
            WARN("unknown attr %s\n", debugstr_xml_attr(&attr));
        }
    }

    if (!version)
    {
        set_error( xmlbuf );
        return;
    }

    if (end)
        return;

    while (next_xml_elem(xmlbuf, &elem, parent))
    {
        if (assembly->type == APPLICATION_MANIFEST && xml_elem_cmp(&elem, L"noInherit", asmv1W))
        {
            parse_noinherit_elem(xmlbuf, &elem);
            assembly->no_inherit = TRUE;
        }
        else if (xml_elem_cmp(&elem, L"noInheritable", asmv1W))
        {
            parse_noinheritable_elem(xmlbuf, &elem);
        }
        else if (xml_elem_cmp(&elem, L"description", asmv1W))
        {
            parse_description_elem(xmlbuf, &elem);
        }
        else if (xml_elem_cmp(&elem, L"comInterfaceExternalProxyStub", asmv1W))
        {
            parse_com_interface_external_proxy_stub_elem(xmlbuf, assembly, acl, &elem);
        }
        else if (xml_elem_cmp(&elem, L"dependency", asmv1W))
        {
            parse_dependency_elem(xmlbuf, acl, &elem);
        }
        else if (xml_elem_cmp(&elem, L"file", asmv1W))
        {
            parse_file_elem(xmlbuf, assembly, acl, &elem);
        }
        else if (xml_elem_cmp(&elem, L"clrClass", asmv1W))
        {
            parse_clr_class_elem(xmlbuf, assembly, acl, &elem);
        }
        else if (xml_elem_cmp(&elem, L"clrSurrogate", asmv1W))
        {
            parse_clr_surrogate_elem(xmlbuf, assembly, acl, &elem);
        }
        else if (xml_elem_cmp(&elem, L"trustInfo", asmv1W))
        {
            parse_trust_info_elem(xmlbuf, assembly, acl, &elem);
        }
        else if (xml_elem_cmp(&elem, L"assemblyIdentity", asmv1W))
        {
            parse_assembly_identity_elem(xmlbuf, acl->actctx, &assembly->id, &elem);

            if (!xmlbuf->error && expected_ai)
            {
                /* FIXME: more tests */
                if (assembly->type == ASSEMBLY_MANIFEST &&
                    memcmp(&assembly->id.version, &expected_ai->version, sizeof(assembly->id.version)))
                {
                    FIXME("wrong version for assembly manifest: %u.%u.%u.%u / %u.%u.%u.%u\n",
                          expected_ai->version.major, expected_ai->version.minor,
                          expected_ai->version.build, expected_ai->version.revision,
                          assembly->id.version.major, assembly->id.version.minor,
                          assembly->id.version.build, assembly->id.version.revision);
                    set_error( xmlbuf );
                }
                else if (assembly->type == ASSEMBLY_SHARED_MANIFEST &&
                         (assembly->id.version.major != expected_ai->version.major ||
                          assembly->id.version.minor != expected_ai->version.minor ||
                          assembly->id.version.build < expected_ai->version.build ||
                          (assembly->id.version.build == expected_ai->version.build &&
                           assembly->id.version.revision < expected_ai->version.revision)))
                {
                    FIXME("wrong version for shared assembly manifest\n");
                    set_error( xmlbuf );
                }
            }
        }
        else if (xml_elem_cmp(&elem, L"compatibility", compatibilityNSW))
        {
            parse_compatibility_elem(xmlbuf, assembly, acl, &elem);
        }
        else if (xml_elem_cmp(&elem, L"application", asmv1W))
        {
            parse_application_elem(xmlbuf, assembly, acl, &elem);
        }
        else
        {
            WARN("unknown element %s\n", debugstr_xml_elem(&elem));
            parse_unknown_elem(xmlbuf, &elem);
        }
    }

    if ((assembly->type == ASSEMBLY_MANIFEST || assembly->type == ASSEMBLY_SHARED_MANIFEST) &&
        assembly->no_inherit)
    {
        set_error( xmlbuf );
    }
}

static NTSTATUS parse_manifest_buffer( struct actctx_loader* acl, struct assembly *assembly,
                                       struct assembly_identity* ai, xmlbuf_t *xmlbuf )
{
    struct xml_elem elem;
    struct xml_elem parent = {0};

    xmlbuf->error = FALSE;
    xmlbuf->ns_pos = 0;

    if (!next_xml_elem(xmlbuf, &elem, &parent)) return STATUS_SXS_CANT_GEN_ACTCTX;

    if (xmlstr_cmp(&elem.name, L"?xml") &&
        (!parse_xml_header(xmlbuf) || !next_xml_elem(xmlbuf, &elem, &parent)))
        return STATUS_SXS_CANT_GEN_ACTCTX;

    if (!xml_elem_cmp(&elem, L"assembly", asmv1W))
    {
        FIXME("root element is %s, not <assembly>\n", debugstr_xml_elem(&elem));
        return STATUS_SXS_CANT_GEN_ACTCTX;
    }

    parse_assembly_elem(xmlbuf, assembly, acl, &elem, ai);
    if (xmlbuf->error)
    {
        FIXME("failed to parse manifest %s\n", debugstr_w(assembly->manifest.info) );
        return STATUS_SXS_CANT_GEN_ACTCTX;
    }

    if (next_xml_elem(xmlbuf, &elem, &parent))
    {
        FIXME("unexpected element %s\n", debugstr_xml_elem(&elem));
        return STATUS_SXS_CANT_GEN_ACTCTX;
    }

    if (xmlbuf->ptr != xmlbuf->end)
    {
        FIXME("parse error\n");
        return STATUS_SXS_CANT_GEN_ACTCTX;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS parse_manifest( struct actctx_loader* acl, struct assembly_identity* ai,
                                LPCWSTR filename, HANDLE module, LPCWSTR directory, BOOL shared,
                                const void *buffer, SIZE_T size )
{
    xmlbuf_t xmlbuf;
    NTSTATUS status;
    struct assembly *assembly;
    int unicode_tests;

    TRACE( "parsing manifest loaded from %s base dir %s\n", debugstr_w(filename), debugstr_w(directory) );

    if (!(assembly = add_assembly(acl->actctx, shared ? ASSEMBLY_SHARED_MANIFEST : ASSEMBLY_MANIFEST)))
        return STATUS_SXS_CANT_GEN_ACTCTX;

    if (directory && !(assembly->directory = strdupW(directory)))
        return STATUS_NO_MEMORY;

    if (!filename)
    {
        UNICODE_STRING module_path;
        if ((status = get_module_filename( module, &module_path, 0 ))) return status;
        assembly->manifest.info = module_path.Buffer;
    }
    else if(!(assembly->manifest.info = strdupW( filename + 4 /* skip \??\ prefix */ ))) return STATUS_NO_MEMORY;

    assembly->manifest.type = assembly->manifest.info ? ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE
                                                      : ACTIVATION_CONTEXT_PATH_TYPE_NONE;

    unicode_tests = IS_TEXT_UNICODE_SIGNATURE | IS_TEXT_UNICODE_REVERSE_SIGNATURE;
    if (RtlIsTextUnicode( buffer, size, &unicode_tests ))
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

        if (!(new_buff = RtlAllocateHeap( GetProcessHeap(), 0, size )))
            return STATUS_NO_MEMORY;
        for (i = 0; i < size / sizeof(WCHAR); i++)
            new_buff[i] = RtlUshortByteSwap( buf[i] );
        xmlbuf.ptr = new_buff;
        xmlbuf.end = xmlbuf.ptr + size / sizeof(WCHAR);
        status = parse_manifest_buffer( acl, assembly, ai, &xmlbuf );
        RtlFreeHeap( GetProcessHeap(), 0, new_buff );
    }
    else
    {
        DWORD len;
        WCHAR *new_buff;

        /* let's assume utf-8 for now */
        status = RtlUTF8ToUnicodeN( NULL, 0, &len, buffer, size );
        if (!NT_SUCCESS(status))
        {
            DPRINT1("RtlMultiByteToUnicodeSize failed with %lx\n", status);
            return STATUS_SXS_CANT_GEN_ACTCTX;
        }

        if (!(new_buff = RtlAllocateHeap( GetProcessHeap(), 0, len ))) return STATUS_NO_MEMORY;
        status = RtlUTF8ToUnicodeN( new_buff, len, &len, buffer, size );
        if (!NT_SUCCESS(status))
        {
            DPRINT1("RtlMultiByteToUnicodeN failed with %lx\n", status);
            return STATUS_SXS_CANT_GEN_ACTCTX;
        }

        xmlbuf.ptr = new_buff;
        xmlbuf.end = xmlbuf.ptr + len / sizeof(WCHAR);
        status = parse_manifest_buffer( acl, assembly, ai, &xmlbuf );
        RtlFreeHeap( GetProcessHeap(), 0, new_buff );
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
    return NtOpenFile( handle, GENERIC_READ | SYNCHRONIZE, &attr, &io,
                       FILE_SHARE_READ | FILE_SHARE_DELETE, FILE_SYNCHRONOUS_IO_ALERT );
}

static NTSTATUS find_first_manifest_resource_in_module( HANDLE hModule, const WCHAR **resname )
{
    static LDR_RESOURCE_INFO manifest_res_info = { RT_MANIFEST };
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *entry_base, *entry;
    IMAGE_RESOURCE_DIRECTORY *resdir;
    NTSTATUS status;

    status = LdrFindResourceDirectory_U( hModule, &manifest_res_info, 1, &resdir );
    if (status != STATUS_SUCCESS) return status;

    if (!resdir->NumberOfIdEntries) return STATUS_RESOURCE_NAME_NOT_FOUND;
    entry_base = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(resdir + 1);
    entry = entry_base + resdir->NumberOfNamedEntries;
    *resname = (const WCHAR *)(ULONG_PTR)entry->Id;

    return STATUS_SUCCESS;
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
    DPRINT("get_manifest_in_module %p\n", hModule);

#if 0
    if (TRACE_ON(actctx))
    {
        if (!filename && !get_module_filename( hModule, &nameW, 0 ))
        {
            TRACE( "looking for res %s in module %p %s\n", debugstr_w(resname),
                   hModule, debugstr_w(nameW.Buffer) );
            RtlFreeUnicodeString( &nameW );
        }
        else TRACE( "looking for res %s in module %p %s\n", debugstr_w(resname),
                    hModule, debugstr_w(filename) );
    }
#endif

    if (!resname)
    {
        status = find_first_manifest_resource_in_module( hModule, &resname );
        if (status != STATUS_SUCCESS) return status;
    }

    info.Type = RT_MANIFEST;
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
        status = parse_manifest(acl, ai, filename, hModule, directory, shared, ptr, entry->Size);

    return status;
}

#ifdef __REACTOS__
IMAGE_RESOURCE_DIRECTORY *find_entry_by_name( IMAGE_RESOURCE_DIRECTORY *dir,
                                             LPCWSTR name, void *root,
                                             int want_dir );

IMAGE_RESOURCE_DIRECTORY *find_first_entry( IMAGE_RESOURCE_DIRECTORY *dir,
                                            void *root, int want_dir );


static IMAGE_RESOURCE_DIRECTORY *find_first_id_entry( IMAGE_RESOURCE_DIRECTORY *dir,
                                           void *root, int want_dir )
{
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *entry = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(dir + 1);
    int pos;

    for (pos = dir->NumberOfNamedEntries; pos < dir->NumberOfNamedEntries + dir->NumberOfIdEntries; pos++)
    {
        if (!entry[pos].DataIsDirectory == !want_dir)
            return (IMAGE_RESOURCE_DIRECTORY *)((char *)root + entry[pos].OffsetToDirectory);
    }
    return NULL;
}


static NTSTATUS search_manifest_in_module( struct actctx_loader* acl, struct assembly_identity* ai,
                                       LPCWSTR filename, LPCWSTR directory, BOOL shared,
                                       HANDLE hModule, ULONG lang )
{
    ULONG size;
    PVOID root, ptr;
    IMAGE_RESOURCE_DIRECTORY *resdirptr;
    IMAGE_RESOURCE_DATA_ENTRY *entry;
    NTSTATUS status;

    root = RtlImageDirectoryEntryToData(hModule, TRUE, IMAGE_DIRECTORY_ENTRY_RESOURCE, &size);
    if (!root) return STATUS_RESOURCE_DATA_NOT_FOUND;
    if (size < sizeof(*resdirptr)) return STATUS_RESOURCE_DATA_NOT_FOUND;
    resdirptr = root;

    if (!(ptr = find_entry_by_name(resdirptr, (LPCWSTR)RT_MANIFEST, root, 1)))
        return STATUS_RESOURCE_TYPE_NOT_FOUND;

    resdirptr = ptr;
    if (!(ptr = find_first_id_entry(resdirptr, root, 1)))
        return STATUS_RESOURCE_TYPE_NOT_FOUND;

    resdirptr = ptr;
    if (!(ptr = find_first_entry(resdirptr, root, 0)))
        return STATUS_RESOURCE_TYPE_NOT_FOUND;

    entry = ptr;
    status = LdrAccessResource(hModule, entry, &ptr, NULL);

    if (status == STATUS_SUCCESS)
        status = parse_manifest(acl, ai, filename, hModule, directory, shared, ptr, entry->Size);

    return status;
}
#endif // __REACTOS__

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
    WCHAR resnameBuf[20];
    LPCWSTR resptr = resname;

    if ((!((ULONG_PTR)resname >> 16)))
    {
        _swprintf(resnameBuf, L"#%u", PtrToUlong(resname));
        resptr = resnameBuf;
    }

    DPRINT( "looking for res %S in %S\n", resptr, filename ? filename : L"<NULL>");

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
    status = NtMapViewOfSection( mapping, GetCurrentProcess(), &base, 0, 0, &offset,
                                 &count, ViewShare, 0, PAGE_READONLY );
    NtClose( mapping );
    if (status != STATUS_SUCCESS) return status;

    if (RtlImageNtHeader(base)) /* we got a PE file */
    {
        HANDLE module = (HMODULE)((ULONG_PTR)base | 1);  /* make it a LOAD_LIBRARY_AS_DATAFILE handle */
        if (resname)
            status = get_manifest_in_module( acl, ai, filename, directory, shared, module, resname, lang );
        else
            status = search_manifest_in_module(acl, ai, filename, directory, shared, module, lang);
    }
    else status = STATUS_INVALID_IMAGE_FORMAT;

    NtUnmapViewOfSection( GetCurrentProcess(), base );
    return status;
}

static NTSTATUS get_manifest_in_manifest_file( struct actctx_loader* acl, struct assembly_identity* ai,
                                               LPCWSTR filename, LPCWSTR directory, BOOL shared, HANDLE file )
{
    FILE_END_OF_FILE_INFORMATION info;
    IO_STATUS_BLOCK io;
    HANDLE              mapping;
    OBJECT_ATTRIBUTES   attr;
    LARGE_INTEGER       size;
    LARGE_INTEGER       offset;
    NTSTATUS            status;
    SIZE_T              count;
    void               *base;

    TRACE( "loading manifest file %s\n", debugstr_w(filename) );

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
    status = NtMapViewOfSection( mapping, GetCurrentProcess(), &base, 0, 0, &offset,
                                 &count, ViewShare, 0, PAGE_READONLY );
    NtClose( mapping );
    if (status != STATUS_SUCCESS) return status;

    status = NtQueryInformationFile( file, &io, &info, sizeof(info), FileEndOfFileInformation );
    if (status == STATUS_SUCCESS)
        status = parse_manifest(acl, ai, filename, NULL, directory, shared, base, info.EndOfFile.QuadPart);

    NtUnmapViewOfSection( GetCurrentProcess(), base );
    return status;
}

/* try to load the .manifest file associated to the file */
static NTSTATUS get_manifest_in_associated_manifest( struct actctx_loader* acl, struct assembly_identity* ai,
                                                     LPCWSTR filename, LPCWSTR directory, HMODULE module, LPCWSTR resname )
{
    WCHAR *buffer;
    NTSTATUS status;
    UNICODE_STRING nameW;
    HANDLE file;
    UINT resid = CREATEPROCESS_MANIFEST_RESOURCE_ID;

    if (!((ULONG_PTR)resname >> 16)) resid = LOWORD(resname);

    TRACE( "looking for manifest associated with %s id %u\n", debugstr_w(filename), resid );

    if (module) /* use the module filename */
    {
        UNICODE_STRING name;

        if (!(status = get_module_filename( module, &name, sizeof(L".manifest") + 10*sizeof(WCHAR) )))
        {
            if (resid != 1) swprintf( name.Buffer + wcslen(name.Buffer), 10, L".%u", resid );
            wcscat( name.Buffer, L".manifest" );
            if (!RtlDosPathNameToNtPathName_U( name.Buffer, &nameW, NULL, NULL ))
                status = STATUS_RESOURCE_DATA_NOT_FOUND;
            RtlFreeUnicodeString( &name );
        }
        if (status) return status;
    }
    else
    {
        if (!(buffer = RtlAllocateHeap( GetProcessHeap(), 0,
                                        (wcslen(filename) + 10) * sizeof(WCHAR) + sizeof(L".manifest") )))
            return STATUS_NO_MEMORY;
        wcscpy( buffer, filename );
        if (resid != 1) swprintf( buffer + wcslen(buffer), 10, L".%u", resid );
        wcscat( buffer, L".manifest" );
        RtlInitUnicodeString( &nameW, buffer );
    }

    if (!open_nt_file( &file, &nameW ))
    {
        status = get_manifest_in_manifest_file( acl, ai, nameW.Buffer, directory, FALSE, file );
        NtClose( file );
    }
    else status = STATUS_RESOURCE_NAME_NOT_FOUND;
    RtlFreeUnicodeString( &nameW );
    return status;
}

static WCHAR *lookup_manifest_file( HANDLE dir, struct assembly_identity *ai )
{
    static const WCHAR lookup_fmtW[] = L"%s_%s_%s_%u.%u.*.*_%s_*.manifest";
    static const WCHAR wine_trailerW[] = {'d','e','a','d','b','e','e','f','.','m','a','n','i','f','e','s','t'};

    WCHAR *lookup, *ret = NULL;
    UNICODE_STRING lookup_us;
    IO_STATUS_BLOCK io;
    const WCHAR *lang = ai->language;
    unsigned int data_pos = 0, data_len, len;
    char buffer[8192];

    if (!lang || !wcsicmp( lang, L"neutral" )) lang = L"*";

    len = wcslen(ai->arch) + wcslen(ai->name) + wcslen(ai->public_key) + wcslen(lang) + 20 + ARRAY_SIZE(lookup_fmtW);
    if (!(lookup = RtlAllocateHeap( GetProcessHeap(), 0, len * sizeof(WCHAR) ))) return NULL;
    swprintf( lookup, len, lookup_fmtW, ai->arch, ai->name, ai->public_key,
              ai->version.major, ai->version.minor, lang );
    RtlInitUnicodeString( &lookup_us, lookup );

#ifdef __arm64ec__
    if (!wcsncmp( lookup, L"amd64_", 6 )) memcpy( lookup, L"a??", 3 * sizeof(WCHAR) );
#endif

    if (!NtQueryDirectoryFile( dir, 0, NULL, NULL, &io, buffer, sizeof(buffer),
                               FileBothDirectoryInformation, FALSE, &lookup_us, TRUE ))
    {
        ULONG min_build = ai->version.build, min_revision = ai->version.revision;
        FILE_BOTH_DIR_INFORMATION *dir_info;
        WCHAR *tmp;
        ULONG build, revision;

        data_len = io.Information;

        for (;;)
        {
            if (data_pos >= data_len)
            {
                if (NtQueryDirectoryFile( dir, 0, NULL, NULL, &io, buffer, sizeof(buffer),
                                          FileBothDirectoryInformation, FALSE, &lookup_us, FALSE ))
                    break;
                data_len = io.Information;
                data_pos = 0;
            }
            dir_info = (FILE_BOTH_DIR_INFORMATION*)(buffer + data_pos);

            if (dir_info->NextEntryOffset) data_pos += dir_info->NextEntryOffset;
            else data_pos = data_len;

            tmp = (WCHAR *)dir_info->FileName + (wcschr(lookup, '*') - lookup);
            build = wcstoul( tmp, NULL, 10 );
            if (build < min_build) continue;
            tmp = wcschr(tmp, '.') + 1;
            revision = wcstoul( tmp, NULL, 10 );
            if (build == min_build && revision < min_revision) continue;
            tmp = wcschr(tmp, '_') + 1;
            tmp = wcschr(tmp, '_') + 1;
            if (dir_info->FileNameLength - (tmp - dir_info->FileName) * sizeof(WCHAR) == sizeof(wine_trailerW) &&
                !wcsnicmp( tmp, wine_trailerW, ARRAY_SIZE( wine_trailerW )))
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
            RtlFreeHeap( GetProcessHeap(), 0, ret );
            if ((ret = RtlAllocateHeap( GetProcessHeap(), 0, dir_info->FileNameLength + sizeof(WCHAR) )))
            {
                memcpy( ret, dir_info->FileName, dir_info->FileNameLength );
                ret[dir_info->FileNameLength/sizeof(WCHAR)] = 0;
            }
        }
    }
    else WARN("no matching file for %s\n", debugstr_w(lookup));
    RtlFreeHeap( GetProcessHeap(), 0, lookup );
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

    if (!ai->arch || !ai->name || !ai->public_key) return STATUS_NO_SUCH_FILE;

    if (!(path = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(L"\\winsxs\\manifests") +
                                  wcslen(windows_dir) * sizeof(WCHAR) )))
        return STATUS_NO_MEMORY;

    wcscpy( path, windows_dir );
    wcscat( path, L"\\winsxs\\manifests" );

    if (!RtlDosPathNameToNtPathName_U( path, &path_us, NULL, NULL ))
    {
        RtlFreeHeap( GetProcessHeap(), 0, path );
        return STATUS_NO_SUCH_FILE;
    }
    RtlFreeHeap( GetProcessHeap(), 0, path );

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &path_us;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    if (!NtOpenFile( &handle, GENERIC_READ | SYNCHRONIZE, &attr, &io, FILE_SHARE_READ | FILE_SHARE_WRITE,
                     FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT ))
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
    if (!(path = RtlReAllocateHeap( GetProcessHeap(), 0, path_us.Buffer,
                                    path_us.Length + (wcslen(file) + 2) * sizeof(WCHAR) )))
    {
        RtlFreeHeap( GetProcessHeap(), 0, file );
        RtlFreeUnicodeString( &path_us );
        return STATUS_NO_MEMORY;
    }

    path[path_us.Length/sizeof(WCHAR)] = '\\';
    wcscpy( path + path_us.Length/sizeof(WCHAR) + 1, file );
    RtlInitUnicodeString( &path_us, path );
    *wcsrchr(file, '.') = 0;  /* remove .manifest extension */

    if (!open_nt_file( &handle, &path_us ))
    {
        io.Status = get_manifest_in_manifest_file(acl, &sxs_ai, path_us.Buffer, file, TRUE, handle);
        NtClose( handle );
    }
    else io.Status = STATUS_NO_SUCH_FILE;

    RtlFreeHeap( GetProcessHeap(), 0, file );
    RtlFreeUnicodeString( &path_us );
    return io.Status;
}

static NTSTATUS lookup_assembly(struct actctx_loader* acl,
                                struct assembly_identity* ai)
{
    unsigned int i;
    WCHAR *buffer, *p, *directory;
    NTSTATUS status;
    UNICODE_STRING nameW;
    HANDLE file;
    DWORD len;

    TRACE( "looking for name=%s version=%s arch=%s\n",
           debugstr_w(ai->name), debugstr_version(&ai->version), debugstr_w(ai->arch) );

    if ((status = lookup_winsxs(acl, ai)) != STATUS_NO_SUCH_FILE) return status;

    /* FIXME: add support for language specific lookup */

    len = max(RtlGetFullPathName_U(acl->actctx->assemblies->manifest.info, 0, NULL, NULL) / sizeof(WCHAR),
        wcslen(acl->actctx->appdir.info));

    nameW.Buffer = NULL;
    if (!(buffer = RtlAllocateHeap( GetProcessHeap(), 0,
                                    (len + 2 * wcslen(ai->name) + 2) * sizeof(WCHAR) + sizeof(L".manifest") )))
        return STATUS_NO_MEMORY;

    if (!(directory = build_assembly_dir( ai )))
    {
        RtlFreeHeap( GetProcessHeap(), 0, buffer );
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
    wcscpy( buffer, acl->actctx->appdir.info );
    p = buffer + wcslen(buffer);
    for (i = 0; i < 4; i++)
    {
        if (i == 2)
        {
            struct assembly *assembly = acl->actctx->assemblies;
            if (!RtlGetFullPathName_U(assembly->manifest.info, len * sizeof(WCHAR), buffer, &p)) break;
        }
        else *p++ = '\\';

        wcscpy( p, ai->name );
        p += wcslen(p);

        wcscpy( p, L".dll" );
        if (RtlDosPathNameToNtPathName_U( buffer, &nameW, NULL, NULL ))
        {
            status = open_nt_file( &file, &nameW );
            if (!status)
            {
                status = get_manifest_in_pe_file( acl, ai, nameW.Buffer, directory, FALSE, file, NULL, 0 );
                NtClose( file );
                if (status == STATUS_SUCCESS)
                    break;
            }
            RtlFreeUnicodeString( &nameW );
        }

        wcscpy( p, L".manifest" );
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
    RtlFreeHeap( GetProcessHeap(), 0, directory );
    RtlFreeHeap( GetProcessHeap(), 0, buffer );
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
                FIXME( "Could not find dependent assembly %s (%s)\n",
                    debugstr_w(acl->dependencies[i].name),
                    debugstr_version(&acl->dependencies[i].version) );
                status = STATUS_SXS_CANT_GEN_ACTCTX;
                break;
            }
        }
    }
    /* FIXME should now iterate through all refs */
    return status;
}

static HANDLE get_current_actctx_no_addref(void)
{
    ACTIVATION_CONTEXT_STACK *actctx_stack = NtCurrentTeb()->ActivationContextStackPointer;

    if (actctx_stack->ActiveFrame)
        return actctx_stack->ActiveFrame->ActivationContext;

    return NULL;
}

/* find the appropriate activation context for RtlQueryInformationActivationContext */
static NTSTATUS find_query_actctx( HANDLE *handle, DWORD flags, ULONG class )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (flags & RTL_QUERY_ACTIVATION_CONTEXT_FLAG_USE_ACTIVE_ACTIVATION_CONTEXT)
    {
        if (*handle) return STATUS_INVALID_PARAMETER;

        *handle = get_current_actctx_no_addref();
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
    ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION *data;
    ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_SEGMENT *path;
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
            total_len += aligned_string_len((wcslen(dll->name)+1)*sizeof(WCHAR));
            total_len += sizeof(*data);
            if (dll->load_from)
            {
                total_len += sizeof(*path);
                total_len += aligned_string_len( wcslen(dll->load_from) * sizeof(WCHAR) );
            }

            DPRINT("assembly %d (%p), dll %d: dll name %S\n", i, assembly, j, dll->name);
        }

        dll_count += assembly->num_dlls;
    }

    total_len += sizeof(*header);

    header = RtlAllocateHeap(GetProcessHeap(), 0, total_len);
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
            str.Length = wcslen(dll->name)*sizeof(WCHAR);
            str.MaximumLength = str.Length + sizeof(WCHAR);
            /* hash original class name */
            RtlHashUnicodeString(&str, TRUE, HASH_STRING_ALGORITHM_X65599, &index->hash);

            index->name_offset = name_offset;
            index->name_len = str.Length;
            index->data_offset = index->name_offset + aligned_string_len(str.MaximumLength);
            index->data_len = sizeof(*data);
            index->rosterindex = i + 1;

            /* dll name */
            ptrW = (WCHAR*)((BYTE*)header + index->name_offset);
            memcpy(ptrW, dll->name, index->name_len);
            ptrW[index->name_len/sizeof(WCHAR)] = 0;
            name_offset += aligned_string_len(str.MaximumLength);

            /* setup data */
            data = (ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION *)((BYTE *)header + index->data_offset);
            if (dll->load_from)
            {
                ULONG len = wcslen(dll->load_from) * sizeof(WCHAR);
                data->Size = sizeof(*data) + sizeof(*path);
                data->Flags = 0;
                data->TotalPathLength = aligned_string_len( len );
                data->PathSegmentCount = 1;
                data->PathSegmentOffset = index->data_offset + sizeof(*data);
                path = (ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_SEGMENT *)(data + 1);
                path->Offset = index->data_offset + data->Size;
                path->Length = len;
                ptrW = (WCHAR *)((BYTE *)header + path->Offset);
                memcpy( ptrW, dll->load_from, len );
                if (wcschr( dll->load_from, '%' ))
                    data->Flags |= ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_EXPAND;
            }
            else
            {
                data->Size = sizeof(*data);
                data->Flags = ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_OMITS_ASSEMBLY_ROOT;
                data->TotalPathLength = 0;
                data->PathSegmentCount = 0;
                data->PathSegmentOffset = 0;
            }
            name_offset += data->Size + data->TotalPathLength;

            index++;
        }
    }

    *section = header;

    return STATUS_SUCCESS;
}

static struct string_index *find_string_index(const struct strsection_header *section, const UNICODE_STRING *name)
{
    struct string_index *iter, *index = NULL;
    UNICODE_STRING str;
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
            str.Buffer = (WCHAR *)((BYTE *)section + iter->name_offset);
            str.Length = iter->name_len;
            if (RtlEqualUnicodeString( &str, name, TRUE ))
            {
                index = iter;
                break;
            }
            else
                WARN("hash collision 0x%08lx, %s, %s\n", hash, debugstr_us(name), debugstr_us(&str));
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

static inline ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION *get_dllredirect_data(ACTIVATION_CONTEXT *ctxt, struct string_index *index)
{
    return (ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION *)((BYTE *)ctxt->dllredirect_section + index->data_offset);
}

static NTSTATUS find_dll_redirection(ACTIVATION_CONTEXT* actctx, const UNICODE_STRING *name,
                                     PACTCTX_SECTION_KEYED_DATA data)
{
    ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION *dll;
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
            RtlFreeHeap(GetProcessHeap(), 0, section);
    }

    index = find_string_index(actctx->dllredirect_section, name);
    DPRINT("index: %d\n", index);
    if (!index) return STATUS_SXS_KEY_NOT_FOUND;

    if (data)
    {
        dll = get_dllredirect_data(actctx, index);

        data->ulDataFormatVersion = 1;
        data->lpData = dll;
        data->ulLength = dll->Size;
        data->lpSectionGlobalData = NULL;
        data->ulSectionGlobalDataLength = 0;
        data->lpSectionBase = actctx->dllredirect_section;
        data->ulSectionTotalLength = RtlSizeHeap( GetProcessHeap(), 0, actctx->dllredirect_section );
        data->hActCtx = NULL;

        if (data->cbSize >= FIELD_OFFSET(ACTCTX_SECTION_KEYED_DATA, ulAssemblyRosterIndex) + sizeof(ULONG))
            data->ulAssemblyRosterIndex = index->rosterindex;
    }

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
                    int class_len = wcslen(entity->u.class.name) + 1;
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
                    len += wcslen(dll->name) + 1;
                    total_len += aligned_string_len(len*sizeof(WCHAR));

                    class_count++;
                }
            }
        }
    }

    total_len += sizeof(*header);

    header = RtlAllocateHeap(GetProcessHeap(), 0, total_len);
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
                    ULONG versioned_len, module_len;
                    UNICODE_STRING str;
                    WCHAR *ptrW;

                    /* setup new index entry */
                    str.Buffer = entity->u.class.name;
                    str.Length = wcslen(entity->u.class.name)*sizeof(WCHAR);
                    str.MaximumLength = str.Length + sizeof(WCHAR);
                    /* hash original class name */
                    RtlHashUnicodeString(&str, TRUE, HASH_STRING_ALGORITHM_X65599, &index->hash);

                    /* include '!' separator too */
                    if (entity->u.class.versioned)
                        versioned_len = (get_assembly_version(assembly, NULL) + 1)*sizeof(WCHAR) + str.Length;
                    else
                        versioned_len = str.Length;
                    module_len = wcslen(dll->name)*sizeof(WCHAR);

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
                        wcscat(ptrW, L"!");
                        wcscat(ptrW, entity->u.class.name);
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
    UNICODE_STRING str;
    ULONG hash;
    int i;

    if (!(actctx->sections & WINDOWCLASS_SECTION)) return STATUS_SXS_KEY_NOT_FOUND;

    if (!actctx->wndclass_section)
    {
        struct strsection_header *section;

        NTSTATUS status = build_wndclass_section(actctx, &section);
        if (status) return status;

        if (InterlockedCompareExchangePointer((void**)&actctx->wndclass_section, section, NULL))
            RtlFreeHeap(GetProcessHeap(), 0, section);
    }

    hash = 0;
    RtlHashUnicodeString(name, TRUE, HASH_STRING_ALGORITHM_X65599, &hash);
    iter = get_wndclass_first_index(actctx);

    for (i = 0; i < actctx->wndclass_section->count; i++)
    {
        if (iter->hash == hash)
        {
            str.Buffer = (WCHAR *)((BYTE *)actctx->wndclass_section + iter->name_offset);
            str.Length = iter->name_len;
            if (RtlEqualUnicodeString( &str, name, TRUE ))
            {
                index = iter;
                break;
            }
            else
                WARN("hash collision 0x%08lx, %s, %s\n", hash, debugstr_us(name), debugstr_us(&str));
        }
        iter++;
    }

    if (!index) return STATUS_SXS_KEY_NOT_FOUND;

    if (data)
    {
        class = get_wndclass_data(actctx, index);

        data->ulDataFormatVersion = 1;
        data->lpData = class;
        /* full length includes string length with nulls */
        data->ulLength = class->size + class->name_len + class->module_len + 2*sizeof(WCHAR);
        data->lpSectionGlobalData = NULL;
        data->ulSectionGlobalDataLength = 0;
        data->lpSectionBase = actctx->wndclass_section;
        data->ulSectionTotalLength = RtlSizeHeap( GetProcessHeap(), 0, actctx->wndclass_section );
        data->hActCtx = NULL;

        if (data->cbSize >= FIELD_OFFSET(ACTCTX_SECTION_KEYED_DATA, ulAssemblyRosterIndex) + sizeof(ULONG))
            data->ulAssemblyRosterIndex = index->rosterindex;
    }

    return STATUS_SUCCESS;
}

static inline struct string_index *get_activatable_class_first_index(ACTIVATION_CONTEXT *actctx)
{
    return (struct string_index *)((BYTE *)actctx->activatable_class_section + actctx->activatable_class_section->index_offset);
}

static inline struct activatable_class_data *get_activatable_class_data(ACTIVATION_CONTEXT *ctxt, struct string_index *index)
{
    return (struct activatable_class_data *)((BYTE *)ctxt->activatable_class_section + index->data_offset);
}

static NTSTATUS build_activatable_class_section(ACTIVATION_CONTEXT *actctx, struct strsection_header **section)
{
    unsigned int i, j, k, total_len = 0, class_count = 0, global_offset = 0, global_len = 0;
    struct activatable_class_data *data;
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
            BOOL has_class = FALSE;

            for (k = 0; k < dll->entities.num; k++)
            {
                struct entity *entity = &dll->entities.base[k];
                if (entity->kind == ACTIVATION_CONTEXT_SECTION_WINRT_ACTIVATABLE_CLASSES)
                {
                    int class_len = wcslen(entity->u.activatable_class.name) + 1;

                    /* each class entry needs index, data and string data */
                    total_len += sizeof(*index);
                    total_len += aligned_string_len(class_len * sizeof(WCHAR));
                    total_len += sizeof(*data);

                    class_count++;
                    has_class = TRUE;
                }
            }

            if (has_class)
            {
                int module_len = wcslen(dll->name) + 1;
                global_len += aligned_string_len(module_len * sizeof(WCHAR));
            }
        }
    }

    total_len += sizeof(*header) + global_len;

    header = RtlAllocateHeap(GetProcessHeap(), 0, total_len);
    if (!header) return STATUS_NO_MEMORY;

    memset(header, 0, sizeof(*header));
    header->magic = STRSECTION_MAGIC;
    header->size  = sizeof(*header);
    header->count = class_count;
    header->global_offset = header->size;
    header->global_len = global_len;
    header->index_offset = header->global_offset + header->global_len;
    index = (struct string_index *)((BYTE *)header + header->index_offset);
    name_offset = header->index_offset + header->count * sizeof(*index);

    global_offset = header->size;
    for (i = 0; i < actctx->num_assemblies; i++)
    {
        struct assembly *assembly = &actctx->assemblies[i];
        for (j = 0; j < assembly->num_dlls; j++)
        {
            struct dll_redirect *dll = &assembly->dlls[j];
            int module_len = wcslen(dll->name) * sizeof(WCHAR);
            BOOL has_class = FALSE;

            for (k = 0; k < dll->entities.num; k++)
            {
                struct entity *entity = &dll->entities.base[k];

                if (entity->kind == ACTIVATION_CONTEXT_SECTION_WINRT_ACTIVATABLE_CLASSES)
                {
                    UNICODE_STRING str;
                    WCHAR *ptrW;

                    /* setup new index entry */
                    str.Buffer = entity->u.activatable_class.name;
                    str.Length = wcslen(entity->u.activatable_class.name) * sizeof(WCHAR);
                    str.MaximumLength = str.Length + sizeof(WCHAR);
                    /* hash class name */
                    RtlHashUnicodeString(&str, TRUE, HASH_STRING_ALGORITHM_X65599, &index->hash);

                    index->name_offset = name_offset;
                    index->name_len = str.Length;
                    index->data_offset = index->name_offset + aligned_string_len(str.MaximumLength);
                    index->data_len = sizeof(*data);
                    index->rosterindex = i + 1;

                    /* class name */
                    ptrW = (WCHAR *)((BYTE *)header + index->name_offset);
                    memcpy(ptrW, entity->u.activatable_class.name, index->name_len);
                    ptrW[index->name_len / sizeof(WCHAR)] = 0;

                    /* class data */
                    data = (struct activatable_class_data *)((BYTE *)header + index->data_offset);
                    data->size = sizeof(*data);
                    data->threading_model = entity->u.activatable_class.threading_model;
                    data->module_len = module_len;
                    data->module_offset = global_offset;

                    name_offset += aligned_string_len(str.MaximumLength);
                    name_offset += sizeof(*data);

                    index++;
                    has_class = TRUE;
                }
            }

            if (has_class)
            {
                WCHAR *ptrW = (WCHAR *)((BYTE *)header + global_offset);
                memcpy(ptrW, dll->name, module_len);
                ptrW[module_len / sizeof(WCHAR)] = 0;
                global_offset += aligned_string_len(module_len + sizeof(WCHAR));
            }
        }
    }

    *section = header;

    return STATUS_SUCCESS;
}

static NTSTATUS find_activatable_class(ACTIVATION_CONTEXT* actctx, const UNICODE_STRING *name,
                                       PACTCTX_SECTION_KEYED_DATA data)
{
    struct string_index *iter, *index = NULL;
    struct activatable_class_data *class;
    UNICODE_STRING str;
    ULONG hash;
    int i;

    if (!(actctx->sections & ACTIVATABLE_CLASS_SECTION)) return STATUS_SXS_KEY_NOT_FOUND;

    if (!actctx->activatable_class_section)
    {
        struct strsection_header *section;

        NTSTATUS status = build_activatable_class_section(actctx, &section);
        if (status) return status;

        if (InterlockedCompareExchangePointer((void**)&actctx->activatable_class_section, section, NULL))
            RtlFreeHeap(GetProcessHeap(), 0, section);
    }

    hash = 0;
    RtlHashUnicodeString(name, TRUE, HASH_STRING_ALGORITHM_X65599, &hash);
    iter = get_activatable_class_first_index(actctx);

    for (i = 0; i < actctx->activatable_class_section->count; i++)
    {
        if (iter->hash == hash)
        {
            str.Buffer = (WCHAR *)((BYTE *)actctx->activatable_class_section + iter->name_offset);
            str.Length = iter->name_len;
            if (RtlEqualUnicodeString( &str, name, TRUE ))
            {
                index = iter;
                break;
            }
            else
                WARN("hash collision 0x%08lx, %s, %s\n", hash, debugstr_us(name), debugstr_us(&str));
        }
        iter++;
    }

    if (!index) return STATUS_SXS_KEY_NOT_FOUND;

    if (data)
    {
        class = get_activatable_class_data(actctx, index);

        data->ulDataFormatVersion = 1;
        data->lpData = class;
        /* full length includes string length with nulls */
        data->ulLength = class->size + class->module_len + sizeof(WCHAR);
        data->lpSectionGlobalData = (BYTE *)actctx->activatable_class_section + actctx->activatable_class_section->global_offset;
        data->ulSectionGlobalDataLength = actctx->activatable_class_section->global_len;
        data->lpSectionBase = actctx->activatable_class_section;
        data->ulSectionTotalLength = RtlSizeHeap( GetProcessHeap(), 0, actctx->activatable_class_section );
        data->hActCtx = NULL;

        if (data->cbSize >= FIELD_OFFSET(ACTCTX_SECTION_KEYED_DATA, ulAssemblyRosterIndex) + sizeof(ULONG))
            data->ulAssemblyRosterIndex = index->rosterindex;
    }

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
                        total_len += aligned_string_len((wcslen(entity->u.typelib.helpdir)+1)*sizeof(WCHAR));

                    /* module names are packed one after another */
                    names_len += (wcslen(dll->name)+1)*sizeof(WCHAR);

                    tlib_count++;
                }
            }
        }
    }

    total_len += aligned_string_len(names_len);
    total_len += sizeof(*header);

    header = RtlAllocateHeap(GetProcessHeap(), 0, total_len);
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
                    NTSTATUS Status;

                    if (*entity->u.typelib.helpdir)
                        help_len = wcslen(entity->u.typelib.helpdir)*sizeof(WCHAR);
                    else
                        help_len = 0;

                    module_len = wcslen(dll->name)*sizeof(WCHAR);

                    /* setup new index entry */
                    RtlInitUnicodeString(&str, entity->u.typelib.tlbid);
                    Status = RtlGUIDFromString(&str, &index->guid);
                    if (!NT_SUCCESS(Status))
                    {
                        RtlFreeHeap(GetProcessHeap(), 0, header);
                        return Status;
                    }
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
            RtlFreeHeap(GetProcessHeap(), 0, section);
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
    data->ulSectionTotalLength = RtlSizeHeap( GetProcessHeap(), 0, actctx->tlib_section );
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
                str_len = wcslen(entity->u.comclass.name)+1;
                if (entity->u.comclass.progid)
                    str_len += wcslen(entity->u.comclass.progid)+1;
                if (entity->u.comclass.version)
                    str_len += wcslen(entity->u.comclass.version)+1;

                *len += sizeof(struct clrclass_data);
                *len += aligned_string_len(str_len*sizeof(WCHAR));

                /* module name is forced to mscoree.dll, and stored two times with different case */
                *module_len += sizeof(L"MSCOREE.DLL") + sizeof(L"mscoree.dll");
            }
            else
            {
                /* progid string is stored separately */
                if (entity->u.comclass.progid)
                    *len += aligned_string_len((wcslen(entity->u.comclass.progid)+1)*sizeof(WCHAR));

                *module_len += (wcslen(dll->name)+1)*sizeof(WCHAR);
            }

            *count += 1;
        }
    }
}

static NTSTATUS add_comserver_record(const struct guidsection_header *section, const struct entity_array *entities,
    const struct dll_redirect *dll, struct guid_index **index, ULONG *data_offset, ULONG *module_offset,
    ULONG *seed, ULONG rosterindex)
{
    unsigned int i;
    NTSTATUS Status;

    for (i = 0; i < entities->num; i++)
    {
        struct entity *entity = &entities->base[i];
        if (entity->kind == ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION)
        {
            ULONG module_len, progid_len, str_len = 0, miscmask;
            struct comclassredirect_data *data;
            struct guid_index *alias_index;
            struct clrclass_data *clrdata;
            UNICODE_STRING str;
            WCHAR *ptrW;

            if (entity->u.comclass.progid)
                progid_len = wcslen(entity->u.comclass.progid)*sizeof(WCHAR);
            else
                progid_len = 0;

            module_len = dll ? wcslen(dll->name)*sizeof(WCHAR) : wcslen(L"MSCOREE.DLL")*sizeof(WCHAR);

            /* setup new index entry */
            RtlInitUnicodeString(&str, entity->u.comclass.clsid);
            Status = RtlGUIDFromString(&str, &(*index)->guid);
            if (!NT_SUCCESS(Status))
                return Status;

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
            data->model = entity->u.comclass.model;
            data->clsid = (*index)->guid;
            data->alias = alias_index->guid;
            data->clsid2 = data->clsid;
            if (entity->u.comclass.tlbid)
            {
                RtlInitUnicodeString(&str, entity->u.comclass.tlbid);
                Status = RtlGUIDFromString(&str, &data->tlbid);
                if (!NT_SUCCESS(Status))
                    return Status;
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
            miscmask = 0;
            if (data->miscstatus)
                miscmask |= MiscStatus;
            if (data->miscstatuscontent)
                miscmask |= MiscStatusContent;
            if (data->miscstatusthumbnail)
                miscmask |= MiscStatusThumbnail;
            if (data->miscstatusicon)
                miscmask |= MiscStatusIcon;
            if (data->miscstatusdocprint)
                miscmask |= MiscStatusDocPrint;
            data->flags = miscmask << 8;

            if (data->clrdata_offset)
            {
                clrdata = (struct clrclass_data*)((BYTE*)data + data->clrdata_offset);

                clrdata->size = sizeof(*clrdata);
                clrdata->res[0] = 0;
                clrdata->res[1] = 2; /* FIXME: unknown field */
                clrdata->module_len = wcslen(L"MSCOREE.DLL")*sizeof(WCHAR);
                clrdata->module_offset = *module_offset + data->name_len + sizeof(WCHAR);
                clrdata->name_len = wcslen(entity->u.comclass.name)*sizeof(WCHAR);
                clrdata->name_offset = clrdata->size;
                clrdata->version_len = entity->u.comclass.version ? wcslen(entity->u.comclass.version)*sizeof(WCHAR) : 0;
                clrdata->version_offset = clrdata->version_len ? clrdata->name_offset + clrdata->name_len + sizeof(WCHAR) : 0;
                clrdata->res2[0] = 0;
                clrdata->res2[1] = 0;

                data->clrdata_len = clrdata->size + clrdata->name_len + sizeof(WCHAR);

                /* module name */
                ptrW = (WCHAR*)((BYTE*)section + clrdata->module_offset);
                memcpy(ptrW, L"mscoree.dll", clrdata->module_len);
                ptrW[clrdata->module_len/sizeof(WCHAR)] = 0;

                ptrW = (WCHAR*)((BYTE*)section + data->name_offset);
                memcpy(ptrW, L"MSCOREE.DLL", data->name_len);
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

    return STATUS_SUCCESS;
}

static NTSTATUS build_comserver_section(ACTIVATION_CONTEXT* actctx, struct guidsection_header **section)
{
    unsigned int i, j, total_len = 0, class_count = 0, names_len = 0;
    struct guidsection_header *header;
    ULONG module_offset, data_offset;
    struct guid_index *index;
    ULONG seed;
    NTSTATUS Status;

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

    header = RtlAllocateHeap(GetProcessHeap(), 0, total_len);
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
        Status = add_comserver_record(header, &assembly->entities, NULL, &index, &data_offset, &module_offset, &seed, i+1);
        if (!NT_SUCCESS(Status))
        {
            RtlFreeHeap(GetProcessHeap(), 0, header);
            return Status;
        }
        for (j = 0; j < assembly->num_dlls; j++)
        {
            struct dll_redirect *dll = &assembly->dlls[j];
            Status = add_comserver_record(header, &dll->entities, dll, &index, &data_offset, &module_offset, &seed, i+1);
            if (!NT_SUCCESS(Status))
            {
                RtlFreeHeap(GetProcessHeap(), 0, header);
                return Status;
            }
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
            RtlFreeHeap(GetProcessHeap(), 0, section);
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
    data->ulSectionTotalLength = RtlSizeHeap( GetProcessHeap(), 0, actctx->comserver_section );
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
                *len += aligned_string_len((wcslen(entity->u.ifaceps.name)+1)*sizeof(WCHAR));
            *count += 1;
        }
    }
}

static NTSTATUS add_ifaceps_record(struct guidsection_header *section, struct entity_array *entities,
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
            NTSTATUS Status;

            if (entity->u.ifaceps.name)
                name_len = wcslen(entity->u.ifaceps.name)*sizeof(WCHAR);
            else
                name_len = 0;

            /* setup index */
            RtlInitUnicodeString(&str, entity->u.ifaceps.iid);
            Status = RtlGUIDFromString(&str, &(*index)->guid);
            if (!NT_SUCCESS(Status))
                return Status;
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
                Status = RtlGUIDFromString(&str, &data->iid);
                if (!NT_SUCCESS(Status))
                    return Status;
            }
            else
                data->iid = (*index)->guid;

            data->nummethods = entity->u.ifaceps.nummethods;

            if (entity->u.ifaceps.tlib)
            {
                RtlInitUnicodeString(&str, entity->u.ifaceps.tlib);
                Status = RtlGUIDFromString(&str, &data->tlbid);
                if (!NT_SUCCESS(Status))
                    return Status;
            }
            else
                memset(&data->tlbid, 0, sizeof(data->tlbid));

            if (entity->u.ifaceps.base)
            {
                RtlInitUnicodeString(&str, entity->u.ifaceps.base);
                Status = RtlGUIDFromString(&str, &data->base);
                if (!NT_SUCCESS(Status))
                    return Status;
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

    return STATUS_SUCCESS;
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

    header = RtlAllocateHeap(GetProcessHeap(), 0, total_len);
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
        NTSTATUS Status;

        Status = add_ifaceps_record(header, &assembly->entities, &index, &data_offset, i + 1);
        if (!NT_SUCCESS(Status))
        {
            RtlFreeHeap(GetProcessHeap(), 0, header);
            return Status;
        }

        for (j = 0; j < assembly->num_dlls; j++)
        {
            struct dll_redirect *dll = &assembly->dlls[j];
            Status = add_ifaceps_record(header, &dll->entities, &index, &data_offset, i + 1);
            if (!NT_SUCCESS(Status))
            {
                RtlFreeHeap(GetProcessHeap(), 0, header);
                return Status;
            }
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
            RtlFreeHeap(GetProcessHeap(), 0, section);
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
    data->ulSectionTotalLength = RtlSizeHeap( GetProcessHeap(), 0, actctx->ifaceps_section );
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
                len = wcslen(entity->u.clrsurrogate.name) + 1;
                if (entity->u.clrsurrogate.version)
                   len += wcslen(entity->u.clrsurrogate.version) + 1;
                total_len += aligned_string_len(len*sizeof(WCHAR));

                count++;
            }
        }
    }

    total_len += sizeof(*header);

    header = RtlAllocateHeap(GetProcessHeap(), 0, total_len);
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
                NTSTATUS Status;

                if (entity->u.clrsurrogate.version)
                    version_len = wcslen(entity->u.clrsurrogate.version)*sizeof(WCHAR);
                else
                    version_len = 0;
                name_len = wcslen(entity->u.clrsurrogate.name)*sizeof(WCHAR);

                /* setup new index entry */
                RtlInitUnicodeString(&str, entity->u.clrsurrogate.clsid);
                Status = RtlGUIDFromString(&str, &index->guid);
                if (!NT_SUCCESS(Status))
                {
                    RtlFreeHeap(GetProcessHeap(), 0, header);
                    return Status;
                }

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

                data_offset += index->data_len;
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
            RtlFreeHeap(GetProcessHeap(), 0, section);
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
    data->ulSectionTotalLength = RtlSizeHeap( GetProcessHeap(), 0, actctx->clrsurrogate_section );
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
                *total_len += single_len + aligned_string_len((wcslen(entity->u.comclass.progid)+1)*sizeof(WCHAR));
                *count += 1;
            }

            for (j = 0; j < entity->u.comclass.progids.num; j++)
                *total_len += aligned_string_len((wcslen(entity->u.comclass.progids.progids[j])+1)*sizeof(WCHAR));

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

static NTSTATUS add_progid_record(ACTIVATION_CONTEXT* actctx, struct strsection_header *section, const struct entity_array *entities,
    struct string_index **index, ULONG *data_offset, ULONG *global_offset, ULONG rosterindex)
{
    unsigned int i, j;
    NTSTATUS Status;

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
            Status = RtlGUIDFromString(&str, &clsid);
            if (!NT_SUCCESS(Status))
                return Status;

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
    return Status;
}

static NTSTATUS build_progid_section(ACTIVATION_CONTEXT* actctx, struct strsection_header **section)
{
    unsigned int i, j, total_len = 0, count = 0;
    struct strsection_header *header;
    ULONG data_offset, global_offset;
    struct string_index *index;
    NTSTATUS Status;

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

    header = RtlAllocateHeap(GetProcessHeap(), 0, total_len);
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

        Status = add_progid_record(actctx, header, &assembly->entities, &index, &data_offset, &global_offset, i + 1);
        if (!NT_SUCCESS(Status))
        {
            RtlFreeHeap(GetProcessHeap(), 0, header);
            return Status;
        }

        for (j = 0; j < assembly->num_dlls; j++)
        {
            struct dll_redirect *dll = &assembly->dlls[j];
            Status = add_progid_record(actctx, header, &dll->entities, &index, &data_offset, &global_offset, i + 1);
            if (!NT_SUCCESS(Status))
            {
                RtlFreeHeap(GetProcessHeap(), 0, header);
                return Status;
            }
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
            RtlFreeHeap(GetProcessHeap(), 0, section);
    }

    if (!actctx->progid_section)
    {
        struct strsection_header *section;

        NTSTATUS status = build_progid_section(actctx, &section);
        if (status) return status;

        if (InterlockedCompareExchangePointer((void**)&actctx->progid_section, section, NULL))
            RtlFreeHeap(GetProcessHeap(), 0, section);
    }

    index = find_string_index(actctx->progid_section, name);
    if (!index) return STATUS_SXS_KEY_NOT_FOUND;

    if (data)
    {
        progid = get_progid_data(actctx, index);

        data->ulDataFormatVersion = 1;
        data->lpData = progid;
        data->ulLength = progid->size;
        data->lpSectionGlobalData = (BYTE*)actctx->progid_section + actctx->progid_section->global_offset;
        data->ulSectionGlobalDataLength = actctx->progid_section->global_len;
        data->lpSectionBase = actctx->progid_section;
        data->ulSectionTotalLength = RtlSizeHeap( GetProcessHeap(), 0, actctx->progid_section );
        data->hActCtx = NULL;

        if (data->cbSize >= FIELD_OFFSET(ACTCTX_SECTION_KEYED_DATA, ulAssemblyRosterIndex) + sizeof(ULONG))
            data->ulAssemblyRosterIndex = index->rosterindex;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS find_string(ACTIVATION_CONTEXT* actctx, ULONG section_kind,
                            const UNICODE_STRING *section_name,
                            DWORD flags, PACTCTX_SECTION_KEYED_DATA data)
{
    NTSTATUS status;

    switch (section_kind)
    {
#ifdef __REACTOS__
    case ACTIVATION_CONTEXT_SECTION_ASSEMBLY_INFORMATION:
        DPRINT1("Unsupported yet section_kind %x\n", section_kind);
        return STATUS_SXS_KEY_NOT_FOUND;
#endif // __REACTOS__
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
        FIXME("Unsupported yet section_kind %lx\n", section_kind);
        return STATUS_SXS_SECTION_NOT_FOUND;
    case ACTIVATION_CONTEXT_SECTION_WINRT_ACTIVATABLE_CLASSES:
        status = find_activatable_class(actctx, section_name, data);
        break;
    default:
        WARN("Unknown section_kind %lx\n", section_kind);
        return STATUS_SXS_SECTION_NOT_FOUND;
    }

    if (status != STATUS_SUCCESS) return status;

    if (data && (flags & FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX))
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
        WARN("Unknown section_kind %lx\n", section_kind);
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

static const WCHAR *find_app_settings( ACTIVATION_CONTEXT *actctx, const WCHAR *settings, const WCHAR *ns )
{
    unsigned int i, j;

    for (i = 0; i < actctx->num_assemblies; i++)
    {
        struct assembly *assembly = &actctx->assemblies[i];
        for (j = 0; j < assembly->entities.num; j++)
        {
            struct entity *entity = &assembly->entities.base[j];
            if (entity->kind == ACTIVATION_CONTEXT_SECTION_APPLICATION_SETTINGS &&
                !wcscmp( entity->u.settings.name, settings ) &&
                !wcscmp( entity->u.settings.ns, ns ))
                return entity->u.settings.value;
        }
    }
    return NULL;
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

#ifdef __REACTOS__
    NtCurrentTeb()->ProcessEnvironmentBlock->ActivationContextData = process_actctx->ActivationContextData;
#else
    NtCurrentTeb()->Peb->ActivationContextData = process_actctx;
#endif // __REACTOS__
}


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
    ACTIVATION_CONTEXT *actctx;
    UNICODE_STRING nameW;
    ULONG lang = 0;
    NTSTATUS status = STATUS_NO_MEMORY;
    HANDLE file = 0;
    struct actctx_loader acl;

    TRACE("%p %08lx\n", pActCtx, pActCtx ? pActCtx->dwFlags : 0);

#define CHECK_LIMIT( field ) (pActCtx->cbSize >= RTL_SIZEOF_THROUGH_FIELD( ACTCTXW, field ))
    if (!pActCtx || (pActCtx->dwFlags & ~ACTCTX_FLAGS_ALL) ||
        !CHECK_LIMIT( lpSource ) ||
        ((pActCtx->dwFlags & ACTCTX_FLAG_PROCESSOR_ARCHITECTURE_VALID) && !CHECK_LIMIT( wProcessorArchitecture )) ||
        ((pActCtx->dwFlags & ACTCTX_FLAG_LANGID_VALID) && !CHECK_LIMIT( wLangId )) ||
        ((pActCtx->dwFlags & ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID) && !CHECK_LIMIT( lpAssemblyDirectory )) ||
        ((pActCtx->dwFlags & ACTCTX_FLAG_RESOURCE_NAME_VALID) && !CHECK_LIMIT( lpResourceName )) ||
        ((pActCtx->dwFlags & ACTCTX_FLAG_APPLICATION_NAME_VALID) && !CHECK_LIMIT( lpApplicationName )) ||
        ((pActCtx->dwFlags & ACTCTX_FLAG_HMODULE_VALID) && !CHECK_LIMIT( hModule )))
        return STATUS_INVALID_PARAMETER;
#undef CHECK_LIMIT

    if ((pActCtx->dwFlags & ACTCTX_FLAG_RESOURCE_NAME_VALID) && !pActCtx->lpResourceName)
        return STATUS_INVALID_PARAMETER;

    if (!(actctx = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*actctx) )))
        return STATUS_NO_MEMORY;

    actctx->magic = ACTCTX_MAGIC;
    actctx->ref_count = 1;
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
        if ((p = wcsrchr( dir.Buffer, '\\' ))) p[1] = 0;
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
            RtlDetermineDosPathNameType_U(pActCtx->lpSource) == RELATIVE_PATH)
        {
            DWORD dir_len, source_len;

            dir_len = wcslen(pActCtx->lpAssemblyDirectory);
            source_len = wcslen(pActCtx->lpSource);
            if (!(source = RtlAllocateHeap( GetProcessHeap(), 0, (dir_len+source_len+2)*sizeof(WCHAR))))
            {
                status = STATUS_NO_MEMORY;
                goto error;
            }

            memcpy(source, pActCtx->lpAssemblyDirectory, dir_len*sizeof(WCHAR));
            source[dir_len] = '\\';
            memcpy(source+dir_len+1, pActCtx->lpSource, (source_len+1)*sizeof(WCHAR));
        }

        ret = RtlDosPathNameToNtPathName_U(source ? source : pActCtx->lpSource, &nameW, NULL, NULL);
        RtlFreeHeap( GetProcessHeap(), 0, source );
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
        else if (pActCtx->lpSource && pActCtx->lpResourceName)
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


/***********************************************************************
 *		RtlAddRefActivationContext (NTDLL.@)
 */
void WINAPI RtlAddRefActivationContext( HANDLE handle )
{
    ACTIVATION_CONTEXT *actctx;

    if ((actctx = check_actctx( handle ))) actctx_addref( actctx );
}


/******************************************************************
 *		RtlReleaseActivationContext (NTDLL.@)
 */
void WINAPI RtlReleaseActivationContext( HANDLE handle )
{
    ACTIVATION_CONTEXT *actctx;

    if ((actctx = check_actctx( handle ))) actctx_release( actctx );
}

/******************************************************************
 *              RtlZombifyActivationContext (NTDLL.@)
 *
 * FIXME: function prototype might be wrong
 */
NTSTATUS WINAPI RtlZombifyActivationContext( HANDLE handle )
{
    FIXME("%p: stub\n", handle);

    if (handle == ACTCTX_FAKE_HANDLE)
        return STATUS_SUCCESS;

    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************
 *		RtlActivateActivationContext (NTDLL.@)
 */
NTSTATUS WINAPI RtlActivateActivationContext( ULONG unknown, HANDLE handle, PULONG_PTR cookie )
{
    return RtlActivateActivationContextEx( 0, NtCurrentTeb(), handle, cookie );
}

/******************************************************************
 *		RtlActivateActivationContextEx (NTDLL.@)
 */
NTSTATUS WINAPI RtlActivateActivationContextEx( ULONG flags, TEB *teb, HANDLE handle, ULONG_PTR *cookie )
{
    ACTIVATION_CONTEXT_STACK *actctx_stack = teb->ActivationContextStackPointer;
    RTL_ACTIVATION_CONTEXT_STACK_FRAME *frame;

    frame = RtlAllocateHeap( GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, sizeof(*frame) );
    frame->Previous = actctx_stack->ActiveFrame;
    frame->ActivationContext = handle;
    frame->Flags = 0;

    DPRINT("ActiveSP %p: ACTIVATE (ActiveFrame %p -> NewFrame %p, Context %p)\n",
        teb->ActivationContextStackPointer, teb->ActivationContextStackPointer->ActiveFrame,
        frame, handle);

    actctx_stack->ActiveFrame = frame;
    RtlAddRefActivationContext( handle );

    *cookie = (ULONG_PTR)frame;
    TRACE( "%p cookie=%Ix\n", handle, *cookie );
    return STATUS_SUCCESS;
}

/***********************************************************************
 *		RtlDeactivateActivationContext (NTDLL.@)
 */
NTSTATUS WINAPI RtlDeactivateActivationContext( ULONG flags, ULONG_PTR cookie )
{
    ACTIVATION_CONTEXT_STACK *actctx_stack = NtCurrentTeb()->ActivationContextStackPointer;
    RTL_ACTIVATION_CONTEXT_STACK_FRAME *frame, *top;

    TRACE( "%lx cookie=%Ix\n", flags, cookie );

    /* find the right frame */
    top = actctx_stack->ActiveFrame;
    for (frame = top; frame; frame = frame->Previous)
        if ((ULONG_PTR)frame == cookie) break;

    if (!frame)
        RtlRaiseStatus( STATUS_SXS_INVALID_DEACTIVATION );

    if (frame != top && !(flags & RTL_DEACTIVATE_ACTIVATION_CONTEXT_FLAG_FORCE_EARLY_DEACTIVATION))
        RtlRaiseStatus( STATUS_SXS_EARLY_DEACTIVATION );

    DPRINT("ActiveSP %p: DEACTIVATE (ActiveFrame %p -> PreviousFrame %p)\n",
        NtCurrentTeb()->ActivationContextStackPointer,
        NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame,
        frame->Previous);

    /* pop everything up to and including frame */
    actctx_stack->ActiveFrame = frame->Previous;

    while (top != actctx_stack->ActiveFrame)
    {
        frame = top->Previous;
        RtlReleaseActivationContext( top->ActivationContext );
        RtlFreeHeap( GetProcessHeap(), 0, top );
        top = frame;
    }

    return STATUS_SUCCESS;
}

/******************************************************************
 *		RtlFreeThreadActivationContextStack (NTDLL.@)
 */
void WINAPI RtlFreeThreadActivationContextStack(void)
{
    RtlFreeActivationContextStack( NtCurrentTeb()->ActivationContextStackPointer );
#ifdef __REACTOS__
    NtCurrentTeb()->ActivationContextStackPointer = NULL;
#endif
}


/******************************************************************
 *		RtlFreeActivationContextStack (NTDLL.@)
 */
void WINAPI RtlFreeActivationContextStack( ACTIVATION_CONTEXT_STACK *actctx_stack )
{
    RTL_ACTIVATION_CONTEXT_STACK_FRAME *frame;

#ifdef __REACTOS__
    /* Nothing to do if there is no stack */
    if (!actctx_stack) return;
#endif // __REACTOS_

    frame = actctx_stack->ActiveFrame;
    while (frame)
    {
        RTL_ACTIVATION_CONTEXT_STACK_FRAME *prev = frame->Previous;
        RtlReleaseActivationContext( frame->ActivationContext );
        RtlFreeHeap( GetProcessHeap(), 0, frame );
        frame = prev;
    }
    actctx_stack->ActiveFrame = NULL;

#ifdef __REACTOS__
    /* TODO: Empty the Frame List Cache */
    ASSERT(IsListEmpty(&actctx_stack->FrameListCache));

    /* Free activation stack memory */
    RtlFreeHeap(GetProcessHeap(), 0, actctx_stack);
#endif // __REACTOS__
}


/******************************************************************
 *		RtlGetActiveActivationContext (NTDLL.@)
 */
NTSTATUS WINAPI RtlGetActiveActivationContext( HANDLE *handle )
{
    RtlAddRefActivationContext( *handle = get_current_actctx_no_addref() );
    return STATUS_SUCCESS;
}


/******************************************************************
 *		RtlIsActivationContextActive (NTDLL.@)
 */
BOOLEAN WINAPI RtlIsActivationContextActive( HANDLE handle )
{
    ACTIVATION_CONTEXT_STACK *actctx_stack = NtCurrentTeb()->ActivationContextStackPointer;
    RTL_ACTIVATION_CONTEXT_STACK_FRAME *frame;

    for (frame = actctx_stack->ActiveFrame; frame; frame = frame->Previous)
        if (frame->ActivationContext == handle) return TRUE;
    return FALSE;
}


/***********************************************************************
 *		RtlQueryInformationActivationContext (NTDLL.@)
 *
 * Get information about an activation context.
 * FIXME: function signature/prototype may be wrong
 */
NTSTATUS WINAPI RtlQueryInformationActivationContext( ULONG flags, HANDLE handle, PVOID subinst,
                                                      ULONG class, PVOID buffer,
                                                      SIZE_T bufsize, SIZE_T *retlen )
{
    ACTIVATION_CONTEXT *actctx;
    NTSTATUS status;

    TRACE("%08lx %p %p %lu %p %Id %p\n", flags, handle,
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
                manifest_len = wcslen(assembly->manifest.info) + 1;
            if (actctx->config.info) config_len = wcslen(actctx->config.info) + 1;
            if (actctx->appdir.info) appdir_len = wcslen(actctx->appdir.info) + 1;
            len = sizeof(*acdi) + (manifest_len + config_len + appdir_len) * sizeof(WCHAR);

            if (retlen) *retlen = len;
            if (!buffer || bufsize < len) return STATUS_BUFFER_TOO_SMALL;

            acdi->dwFlags = 0;
            acdi->ulFormatVersion = assembly ? 1 : 0; /* FIXME */
            acdi->ulAssemblyCount = actctx->num_assemblies;
            acdi->ulRootManifestPathType = assembly ? assembly->manifest.type : 0 /* FIXME */;
            acdi->ulRootManifestPathChars = assembly && assembly->manifest.info ? manifest_len - 1 : 0;
            acdi->ulRootConfigurationPathType = actctx->config.type;
            acdi->ulRootConfigurationPathChars = actctx->config.info ? config_len - 1 : 0;
            acdi->ulAppDirPathType = actctx->appdir.type;
            acdi->ulAppDirPathChars = actctx->appdir.info ? appdir_len - 1 : 0;
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
            id_len = wcslen(assembly_id) + 1;
            if (assembly->directory) ad_len = wcslen(assembly->directory) + 1;

            if (assembly->manifest.info &&
                (assembly->type == ASSEMBLY_MANIFEST || assembly->type == ASSEMBLY_SHARED_MANIFEST))
                path_len  = wcslen(assembly->manifest.info) + 1;

            len = sizeof(*afdi) + (id_len + ad_len + path_len) * sizeof(WCHAR);

            if (retlen) *retlen = len;
            if (!buffer || bufsize < len)
            {
                RtlFreeHeap( GetProcessHeap(), 0, assembly_id );
                return STATUS_BUFFER_TOO_SMALL;
            }

            afdi->ulFlags = 0;  /* FIXME */
            afdi->ulEncodedAssemblyIdentityLength = (id_len - 1) * sizeof(WCHAR);
            afdi->ulManifestPathType = assembly->manifest.type;
            afdi->ulManifestPathLength = assembly->manifest.info ? (path_len - 1) * sizeof(WCHAR) : 0;
            /* FIXME afdi->liManifestLastWriteTime = 0; */
            afdi->ulPolicyPathType = ACTIVATION_CONTEXT_PATH_TYPE_NONE; /* FIXME */
            afdi->ulPolicyPathLength = 0;
            /* FIXME afdi->liPolicyLastWriteTime = 0; */
            afdi->ulMetadataSatelliteRosterIndex = 0; /* FIXME */
            afdi->ulManifestVersionMajor = 1;
            afdi->ulManifestVersionMinor = 0;
            afdi->ulPolicyVersionMajor = 0; /* FIXME */
            afdi->ulPolicyVersionMinor = 0; /* FIXME */
            afdi->ulAssemblyDirectoryNameLength = ad_len ? (ad_len - 1) * sizeof(WCHAR) : 0;
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
            RtlFreeHeap( GetProcessHeap(), 0, assembly_id );
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

            if (dll->name) dll_len = wcslen(dll->name) + 1;
            len = sizeof(*afdi) + dll_len * sizeof(WCHAR);

            if (!buffer || bufsize < len)
            {
                if (retlen) *retlen = len;
                return STATUS_BUFFER_TOO_SMALL;
            }
            if (retlen) *retlen = 0; /* yes that's what native does !! */
            afdi->ulFlags = ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION;
            afdi->ulFilenameLength = dll_len ? (dll_len - 1) * sizeof(WCHAR) : 0;
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

    case CompatibilityInformationInActivationContext:
        {
            struct acci
            {
                DWORD ElementCount;
                COMPATIBILITY_CONTEXT_ELEMENT Elements[1];
            } *acci = buffer;
            struct assembly *assembly = NULL;
            ULONG num_compat_contexts = 0, n;
            SIZE_T len;

            if (!(actctx = check_actctx(handle))) return STATUS_INVALID_PARAMETER;

            if (actctx->num_assemblies) assembly = actctx->assemblies;

            if (assembly)
                num_compat_contexts = assembly->num_compat_contexts;
            len = offsetof( struct acci, Elements[num_compat_contexts] );

            if (retlen) *retlen = len;
            if (!buffer || bufsize < len) return STATUS_BUFFER_TOO_SMALL;

            acci->ElementCount = num_compat_contexts;
            for (n = 0; n < num_compat_contexts; ++n)
            {
                acci->Elements[n] = assembly->compat_contexts[n];
            }
        }
        break;

    case RunlevelInformationInActivationContext:
        {
            ACTIVATION_CONTEXT_RUN_LEVEL_INFORMATION *acrli = buffer;
            struct assembly *assembly;
            SIZE_T len;

            if (!(actctx = check_actctx(handle))) return STATUS_INVALID_PARAMETER;

            len = sizeof(*acrli);
            if (retlen) *retlen = len;
            if (!buffer || bufsize < len)
                return STATUS_BUFFER_TOO_SMALL;

            assembly = actctx->assemblies;

            acrli->ulFlags  = 0;
            acrli->RunLevel = assembly ? assembly->run_level : ACTCTX_RUN_LEVEL_UNSPECIFIED;
            acrli->UiAccess = assembly ? assembly->ui_access : 0;
        }
        break;

    default:
        FIXME( "class %lu not implemented\n", class );
        return STATUS_NOT_IMPLEMENTED;
    }
    return STATUS_SUCCESS;
}

#ifdef __REACTOS__
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
#endif // __REACTOS__

/***********************************************************************
 *		RtlFindActivationContextSectionString (NTDLL.@)
 *
 * Find information about a string in an activation context.
 * FIXME: function signature/prototype may be wrong
 */
NTSTATUS WINAPI RtlFindActivationContextSectionString( ULONG flags, const GUID *guid, ULONG section_kind,
                                                       const UNICODE_STRING *section_name, PVOID ptr )
{
    PACTCTX_SECTION_KEYED_DATA data = ptr;
    NTSTATUS status = STATUS_SXS_KEY_NOT_FOUND;
    ACTIVATION_CONTEXT *actctx;

    TRACE("%08lx %s %lu %s %p\n", flags, debugstr_guid(guid), section_kind,
          debugstr_us(section_name), data);

#ifdef __REACTOS__
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
#else
    if (guid)
    {
        FIXME("expected guid == NULL\n");
        return STATUS_INVALID_PARAMETER;
    }
    if (flags & ~FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX)
    {
        FIXME("unknown flags %08lx\n", flags);
        return STATUS_INVALID_PARAMETER;
    }
    if ((data && data->cbSize < offsetof(ACTCTX_SECTION_KEYED_DATA, ulAssemblyRosterIndex)) ||
        !section_name || !section_name->Buffer)
    {
        WARN("invalid parameter\n");
        return STATUS_INVALID_PARAMETER;
    }
#endif // __REACTOS__

    actctx = check_actctx( get_current_actctx_no_addref() );
    if (actctx) status = find_string( actctx, section_kind, section_name, flags, data );

    DPRINT("status %x\n", status);
    if (status != STATUS_SUCCESS)
        status = find_string( process_actctx, section_kind, section_name, flags, data );

    if (status != STATUS_SUCCESS)
        status = find_string( implicit_actctx, section_kind, section_name, flags, data );

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
    ACTIVATION_CONTEXT *actctx;

    TRACE("%08lx %s %lu %s %p\n", flags, debugstr_guid(extguid), section_kind, debugstr_guid(guid), data);

    if (extguid)
    {
        FIXME("expected extguid == NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (flags & ~FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX)
    {
        FIXME("unknown flags %08lx\n", flags);
        return STATUS_INVALID_PARAMETER;
    }

    if (!data || data->cbSize < FIELD_OFFSET(ACTCTX_SECTION_KEYED_DATA, ulAssemblyRosterIndex) || !guid)
        return STATUS_INVALID_PARAMETER;

    actctx = check_actctx( get_current_actctx_no_addref() );
    if (actctx) status = find_guid( actctx, section_kind, guid, flags, data );

    if (status != STATUS_SUCCESS)
        status = find_guid( process_actctx, section_kind, guid, flags, data );

    if (status != STATUS_SUCCESS)
        status = find_guid( implicit_actctx, section_kind, guid, flags, data );

    return status;
}


/***********************************************************************
 *		RtlQueryActivationContextApplicationSettings (NTDLL.@)
 */
NTSTATUS WINAPI RtlQueryActivationContextApplicationSettings( DWORD flags, HANDLE handle, const WCHAR *ns,
                                                              const WCHAR *settings, WCHAR *buffer,
                                                              SIZE_T size, SIZE_T *written )
{
    ACTIVATION_CONTEXT *actctx;
    const WCHAR *res;

    if (flags)
    {
        WARN( "unknown flags %08lx\n", flags );
        return STATUS_INVALID_PARAMETER;
    }

    if (ns)
    {
        if (wcscmp( ns, windowsSettings2005NSW ) &&
            wcscmp( ns, windowsSettings2011NSW ) &&
            wcscmp( ns, windowsSettings2016NSW ) &&
            wcscmp( ns, windowsSettings2017NSW ) &&
            wcscmp( ns, windowsSettings2019NSW ) &&
            wcscmp( ns, windowsSettings2020NSW ))
            return STATUS_INVALID_PARAMETER;
    }
    else ns = windowsSettings2005NSW;

    if (!handle) handle = process_actctx;
    if (!(actctx = check_actctx( handle ))) return STATUS_INVALID_PARAMETER;

    if (!(res = find_app_settings( actctx, settings, ns ))) return STATUS_SXS_KEY_NOT_FOUND;

    if (written) *written = wcslen(res) + 1;
    if (size < wcslen(res)) return STATUS_BUFFER_TOO_SMALL;
    wcscpy( buffer, res );
    return STATUS_SUCCESS;
}

#ifdef __REACTOS__
/* Stubs */

NTSTATUS
NTAPI
RtlAllocateActivationContextStack(IN PACTIVATION_CONTEXT_STACK *Stack)
{
    PACTIVATION_CONTEXT_STACK ContextStack;

    /* Check if it's already allocated */
    if (*Stack) return STATUS_SUCCESS;

    /* Allocate space for the context stack */
    ContextStack = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ACTIVATION_CONTEXT_STACK));
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
    RTL_ACTIVATION_CONTEXT_STACK_FRAME *NewFrame;
    RTL_ACTIVATION_CONTEXT_STACK_FRAME *ActiveFrame;

    /* Get the current active frame */
    ActiveFrame = NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame;

    DPRINT("ActiveSP %p: ACTIVATE (ActiveFrame %p -> NewFrame %p, Context %p)\n",
        NtCurrentTeb()->ActivationContextStackPointer, ActiveFrame,
        &Frame->Frame, Context);

    /* Ensure it's in the right format and at least fits basic info */
    ASSERT(Frame->Format == RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_FORMAT_WHISTLER);
    ASSERT(Frame->Size >= sizeof(RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_BASIC));

    /* Set debug info if size allows*/
    if (Frame->Size >= sizeof(RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED))
    {
        Frame->Extra1 = (PVOID)(~(ULONG_PTR)ActiveFrame);
        Frame->Extra2 = (PVOID)(~(ULONG_PTR)Context);
        //Frame->Extra3 = ...;
    }

    if (ActiveFrame)
    {
        /*ASSERT((ActiveFrame->Flags &
            (RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_ACTIVATED |
             RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_DEACTIVATED |
             RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_NOT_REALLY_ACTIVATED)) == RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_ACTIVATED);*/

        if (!(ActiveFrame->Flags & RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_HEAP_ALLOCATED))
        {
            // TODO: Perform some additional checks if it was not heap allocated
        }
    }

    /* Save pointer to the new activation frame */
    NewFrame = &Frame->Frame;

    /* Actually activate it */
    Frame->Frame.Previous = ActiveFrame;
    Frame->Frame.ActivationContext = Context;
    Frame->Frame.Flags = RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_ACTIVATED;

    /* Check if we can activate this context */
    if ((ActiveFrame && (ActiveFrame->ActivationContext != Context)) ||
        Context)
    {
        /* Set new active frame */
        DPRINT("Setting new active frame %p instead of old %p\n", NewFrame, ActiveFrame);
        NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame = NewFrame;
        return NewFrame;
    }

    /* We can get here only one way: it was already activated */
    DPRINT("Trying to activate already activated activation context\n");

    /* Activate only if we are allowing multiple activation */
#if 0
    if (!RtlpNotAllowingMultipleActivation)
    {
        Frame->Frame.Flags = RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_ACTIVATED | RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_NOT_REALLY_ACTIVATED;
        NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame = NewFrame;
    }
#else
    // Activate it anyway
    NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame = NewFrame;
#endif

    /* Return pointer to the activation frame */
    return NewFrame;
}

PRTL_ACTIVATION_CONTEXT_STACK_FRAME
FASTCALL
RtlDeactivateActivationContextUnsafeFast(IN PRTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED Frame)
{
    PRTL_ACTIVATION_CONTEXT_STACK_FRAME ActiveFrame, NewFrame;

    ActiveFrame = NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame;

    /* Ensure it's in the right format and at least fits basic info */
    ASSERT(Frame->Format == RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_FORMAT_WHISTLER);
    ASSERT(Frame->Size >= sizeof(RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_BASIC));

    /* Make sure it is not deactivated and it is activated */
    ASSERT((Frame->Frame.Flags & RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_DEACTIVATED) == 0);
    ASSERT(Frame->Frame.Flags & RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_ACTIVATED);
    ASSERT((Frame->Frame.Flags & (RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_ACTIVATED | RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_DEACTIVATED)) == RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_ACTIVATED);

    /* Check debug info if it is present */
    if (Frame->Size >= sizeof(RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED))
    {
        ASSERT(Frame->Extra1 == (PVOID)(~(ULONG_PTR)Frame->Frame.Previous));
        ASSERT(Frame->Extra2 == (PVOID)(~(ULONG_PTR)Frame->Frame.ActivationContext));
        //Frame->Extra3 = ...;
    }

    if (ActiveFrame)
    {
        // TODO: Perform some additional checks here
    }

    /* Special handling for not-really-activated */
    if (Frame->Frame.Flags & RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_NOT_REALLY_ACTIVATED)
    {
        DPRINT1("Deactivating not really activated activation context\n");
        Frame->Frame.Flags |= RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_DEACTIVATED;
        return &Frame->Frame;
    }

    /* find the right frame */
    NewFrame = &Frame->Frame;
    if (ActiveFrame != NewFrame)
    {
        DPRINT1("Deactivating wrong active frame: %p != %p\n", ActiveFrame, NewFrame);
    }

    DPRINT("ActiveSP %p: DEACTIVATE (ActiveFrame %p -> PreviousFrame %p)\n",
        NtCurrentTeb()->ActivationContextStackPointer, NewFrame, NewFrame->Previous);

    /* Pop everything up to and including frame */
    NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame = NewFrame->Previous;

    Frame->Frame.Flags |= RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_DEACTIVATED;
    return NewFrame->Previous;
}

NTSTATUS
NTAPI
RtlpInitializeActCtx(PVOID* pOldShimData)
{
    ACTCTXW ctx;
    HANDLE handle;
    WCHAR buffer[1024];
    NTSTATUS Status;

    /* Initialize trace flags to WARN and ERR */
    __wine_dbch_actctx.flags = 0x03;

    actctx_init();

    /* ReactOS specific:
       Now that we have found the process_actctx we can initialize the process compat subsystem */
    LdrpInitializeProcessCompat(process_actctx, pOldShimData);

    ctx.cbSize   = sizeof(ctx);
    ctx.dwFlags  = 0;
    ctx.hModule  = NULL;
    ctx.lpResourceName = NULL;
    ctx.lpSource = buffer;
    RtlStringCchCopyW(buffer, RTL_NUMBER_OF(buffer), SharedUserData->NtSystemRoot);
    RtlStringCchCatW(buffer, RTL_NUMBER_OF(buffer), L"\\winsxs\\manifests\\systemcompatible.manifest");

    Status = RtlCreateActivationContext(0, (PVOID)&ctx, 0, NULL, NULL, &handle);
    if (NT_SUCCESS(Status))
    {
        implicit_actctx = check_actctx(handle);
    }
    else
    {
        DPRINT1("Failed to create the implicit act ctx. Status: 0x%x!!!\n", Status);
    }

    return Status;
}

#endif // __REACTOS__
