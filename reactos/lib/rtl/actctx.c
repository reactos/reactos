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

/* Based on Wine 1.1.26 */

/* INCLUDES *****************************************************************/
#include <rtl.h>

#define NDEBUG
#include <debug.h>

#include <wine/unicode.h>

#define QUERY_ACTCTX_FLAG_ACTIVE (0x00000001)

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
};

struct entity
{
    DWORD kind;
    union
    {
        struct
        {
            WCHAR *tlbid;
            WCHAR *version;
            WCHAR *helpdir;
        } typelib;
        struct
        {
            WCHAR *clsid;
        } comclass;
        struct {
            WCHAR *iid;
            WCHAR *name;
        } proxy;
        struct
        {
            WCHAR *name;
        } class;
        struct
        {
            WCHAR *name;
            WCHAR *clsid;
        } clrclass;
        struct
        {
            WCHAR *name;
            WCHAR *clsid;
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

typedef struct _ACTIVATION_CONTEXT
{
    ULONG               magic;
    long                 ref_count;
    struct file_info    config;
    struct file_info    appdir;
    struct assembly    *assemblies;
    unsigned int        num_assemblies;
    unsigned int        allocated_assemblies;
} ACTIVATION_CONTEXT;

struct actctx_loader
{
    ACTIVATION_CONTEXT       *actctx;
    struct assembly_identity *dependencies;
    unsigned int              num_dependencies;
    unsigned int              allocated_dependencies;
};

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
static const WCHAR asmv2hashW[] = {'a','s','m','v','2',':','h','a','s','h',0};
static const WCHAR noInheritW[] = {'n','o','I','n','h','e','r','i','t',0};
static const WCHAR noInheritableW[] = {'n','o','I','n','h','e','r','i','t','a','b','l','e',0};
static const WCHAR typelibW[] = {'t','y','p','e','l','i','b',0};
static const WCHAR windowClassW[] = {'w','i','n','d','o','w','C','l','a','s','s',0};

static const WCHAR clsidW[] = {'c','l','s','i','d',0};
static const WCHAR hashW[] = {'h','a','s','h',0};
static const WCHAR hashalgW[] = {'h','a','s','h','a','l','g',0};
static const WCHAR helpdirW[] = {'h','e','l','p','d','i','r',0};
static const WCHAR iidW[] = {'i','i','d',0};
static const WCHAR languageW[] = {'l','a','n','g','u','a','g','e',0};
static const WCHAR manifestVersionW[] = {'m','a','n','i','f','e','s','t','V','e','r','s','i','o','n',0};
static const WCHAR nameW[] = {'n','a','m','e',0};
static const WCHAR newVersionW[] = {'n','e','w','V','e','r','s','i','o','n',0};
static const WCHAR oldVersionW[] = {'o','l','d','V','e','r','s','i','o','n',0};
static const WCHAR optionalW[] = {'o','p','t','i','o','n','a','l',0};
static const WCHAR processorArchitectureW[] = {'p','r','o','c','e','s','s','o','r','A','r','c','h','i','t','e','c','t','u','r','e',0};
static const WCHAR publicKeyTokenW[] = {'p','u','b','l','i','c','K','e','y','T','o','k','e','n',0};
static const WCHAR tlbidW[] = {'t','l','b','i','d',0};
static const WCHAR typeW[] = {'t','y','p','e',0};
static const WCHAR versionW[] = {'v','e','r','s','i','o','n',0};
static const WCHAR xmlnsW[] = {'x','m','l','n','s',0};

static const WCHAR xmlW[] = {'?','x','m','l',0};
static const WCHAR manifestv1W[] = {'u','r','n',':','s','c','h','e','m','a','s','-','m','i','c','r','o','s','o','f','t','-','c','o','m',':','a','s','m','.','v','1',0};
static const WCHAR manifestv3W[] = {'u','r','n',':','s','c','h','e','m','a','s','-','m','i','c','r','o','s','o','f','t','-','c','o','m',':','a','s','m','.','v','3',0};

static const WCHAR dotManifestW[] = {'.','m','a','n','i','f','e','s','t',0};
static const WCHAR version_formatW[] = {'%','u','.','%','u','.','%','u','.','%','u',0};

static ACTIVATION_CONTEXT system_actctx = { ACTCTX_MAGIC, 1 };
static ACTIVATION_CONTEXT *process_actctx = &system_actctx;

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

static inline BOOL isxmlspace( WCHAR ch )
{
    return (ch == ' ' || ch == '\r' || ch == '\n' || ch == '\t');
}

static struct assembly *add_assembly(ACTIVATION_CONTEXT *actctx, enum assembly_type at)
{
    struct assembly *assembly;

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
    unsigned int i;
    for (i = 0; i < array->num; i++)
    {
        struct entity *entity = &array->base[i];
        switch (entity->kind)
        {
        case ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION:
            RtlFreeHeap(RtlGetProcessHeap(), 0, entity->u.comclass.clsid);
            break;
        case ACTIVATION_CONTEXT_SECTION_COM_INTERFACE_REDIRECTION:
            RtlFreeHeap(RtlGetProcessHeap(), 0, entity->u.proxy.iid);
            RtlFreeHeap(RtlGetProcessHeap(), 0, entity->u.proxy.name);
            break;
        case ACTIVATION_CONTEXT_SECTION_COM_TYPE_LIBRARY_REDIRECTION:
            RtlFreeHeap(RtlGetProcessHeap(), 0, entity->u.typelib.tlbid);
            RtlFreeHeap(RtlGetProcessHeap(), 0, entity->u.typelib.version);
            RtlFreeHeap(RtlGetProcessHeap(), 0, entity->u.typelib.helpdir);
            break;
        case ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION:
            RtlFreeHeap(RtlGetProcessHeap(), 0, entity->u.class.name);
            break;
        case ACTIVATION_CONTEXT_SECTION_COM_PROGID_REDIRECTION:
            RtlFreeHeap(RtlGetProcessHeap(), 0, entity->u.clrclass.name);
            RtlFreeHeap(RtlGetProcessHeap(), 0, entity->u.clrclass.clsid);
            break;
        case ACTIVATION_CONTEXT_SECTION_CLR_SURROGATES:
            RtlFreeHeap(RtlGetProcessHeap(), 0, entity->u.clrsurrogate.name);
            RtlFreeHeap(RtlGetProcessHeap(), 0, entity->u.clrsurrogate.clsid);
            break;
        default:
            DPRINT1("Unknown entity kind %d\n", entity->kind);
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
        static const WCHAR wildcardW[] = {'*',0};
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
    static const WCHAR typeW[] =
        {',','t','y','p','e','=',0};
    static const WCHAR versionW[] =
        {',','v','e','r','s','i','o','n','=',0};

    WCHAR version[64], *ret;
    SIZE_T size = 0;

    sprintfW( version, version_formatW,
              ai->version.major, ai->version.minor, ai->version.build, ai->version.revision );
    if (ai->name) size += strlenW(ai->name) * sizeof(WCHAR);
    if (ai->arch) size += strlenW(archW) + strlenW(ai->arch) + 2;
    if (ai->public_key) size += strlenW(public_keyW) + strlenW(ai->public_key) + 2;
    if (ai->type) size += strlenW(typeW) + strlenW(ai->type) + 2;
    size += strlenW(versionW) + strlenW(version) + 2;

    if (!(ret = RtlAllocateHeap( RtlGetProcessHeap(), 0, (size + 1) * sizeof(WCHAR) )))
        return NULL;

    if (ai->name) strcpyW( ret, ai->name );
    else *ret = 0;
    append_string( ret, archW, ai->arch );
    append_string( ret, public_keyW, ai->public_key );
    append_string( ret, typeW, ai->type );
    append_string( ret, versionW, version );
    return ret;
}

static ACTIVATION_CONTEXT *check_actctx( HANDLE h )
{
    ACTIVATION_CONTEXT *ret = NULL, *actctx = h;

    if (!h || h == INVALID_HANDLE_VALUE) return NULL;
    _SEH2_TRY
    {
        if (actctx && actctx->magic == ACTCTX_MAGIC) ret = actctx;
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
    _InterlockedExchangeAdd( &actctx->ref_count, 1 );
}

static void actctx_release( ACTIVATION_CONTEXT *actctx )
{
    if (_InterlockedExchangeAdd( &actctx->ref_count, -1 ) == 1)
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
        actctx->magic = 0;
        RtlFreeHeap( RtlGetProcessHeap(), 0, actctx );
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

    if (ptr == xmlbuf->end || *ptr != '=') return FALSE;

    name->ptr = xmlbuf->ptr;
    name->len = ptr-xmlbuf->ptr;
    xmlbuf->ptr = ptr;

    ptr++;
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
    DPRINT1( "Wrong version definition in manifest file (%S)\n", str->ptr );
    return FALSE;
}

static BOOL parse_expect_elem(xmlbuf_t* xmlbuf, const WCHAR* name)
{
    xmlstr_t    elem;
    if (!next_xml_elem(xmlbuf, &elem)) return FALSE;
    if (xmlstr_cmp(&elem, name)) return TRUE;
    DPRINT1( "unexpected element %S\n", elem.ptr );
    return FALSE;
}

static BOOL parse_expect_no_attr(xmlbuf_t* xmlbuf, BOOL* end)
{
    xmlstr_t    attr_name, attr_value;
    BOOL        error;

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, end))
    {
        DPRINT1( "unexpected attr %S=%S\n", attr_name.ptr,
             attr_value.ptr);
    }
    return !error;
}

static BOOL parse_end_element(xmlbuf_t *xmlbuf)
{
    BOOL end = FALSE;
    return parse_expect_no_attr(xmlbuf, &end) && !end;
}

static BOOL parse_expect_end_elem(xmlbuf_t *xmlbuf, const WCHAR *name)
{
    xmlstr_t    elem;
    if (!next_xml_elem(xmlbuf, &elem)) return FALSE;
    if (!xmlstr_cmp_end(&elem, name))
    {
        DPRINT1( "unexpected element %S\n", elem.ptr );
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

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        if (xmlstr_cmp(&attr_name, nameW))
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
                 attr_value.ptr);
            if (!(ai->language = xmlstrdupW(&attr_value))) return FALSE;
        }
        else
        {
            DPRINT1("unknown attr %S=%S\n", attr_name.ptr,
                 attr_value.ptr);
        }
    }

    if (error || end) return end;
    return parse_expect_end_elem(xmlbuf, assemblyIdentityW);
}

static BOOL parse_com_class_elem(xmlbuf_t* xmlbuf, struct dll_redirect* dll)
{
    xmlstr_t elem, attr_name, attr_value;
    BOOL ret, end = FALSE, error;
    struct entity*      entity;

    if (!(entity = add_entity(&dll->entities, ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION)))
        return FALSE;

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        if (xmlstr_cmp(&attr_name, clsidW))
        {
            if (!(entity->u.comclass.clsid = xmlstrdupW(&attr_value))) return FALSE;
        }
        else
        {
            DPRINT1("unknown attr %S=%S\n", attr_name.ptr, attr_value.ptr);
        }
    }

    if (error || end) return end;

    while ((ret = next_xml_elem(xmlbuf, &elem)))
    {
        if (xmlstr_cmp_end(&elem, comClassW))
        {
            ret = parse_end_element(xmlbuf);
            break;
        }
        else
        {
            DPRINT1("unknown elem %S\n", elem.ptr);
            ret = parse_unknown_elem(xmlbuf, &elem);
        }
    }
    return ret;
}

static BOOL parse_cominterface_proxy_stub_elem(xmlbuf_t* xmlbuf, struct dll_redirect* dll)
{
    xmlstr_t    attr_name, attr_value;
    BOOL        end = FALSE, error;
    struct entity*      entity;

    if (!(entity = add_entity(&dll->entities, ACTIVATION_CONTEXT_SECTION_COM_INTERFACE_REDIRECTION)))
        return FALSE;

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        if (xmlstr_cmp(&attr_name, iidW))
        {
            if (!(entity->u.proxy.iid = xmlstrdupW(&attr_value))) return FALSE;
        }
        if (xmlstr_cmp(&attr_name, nameW))
        {
            if (!(entity->u.proxy.name = xmlstrdupW(&attr_value))) return FALSE;
        }
        else
        {
            DPRINT1("unknown attr %S=%S\n", attr_name.ptr, attr_value.ptr);
        }
    }

    if (error || end) return end;
    return parse_expect_end_elem(xmlbuf, comInterfaceProxyStubW);
}

static BOOL parse_typelib_elem(xmlbuf_t* xmlbuf, struct dll_redirect* dll)
{
    xmlstr_t    attr_name, attr_value;
    BOOL        end = FALSE, error;
    struct entity*      entity;

    if (!(entity = add_entity(&dll->entities, ACTIVATION_CONTEXT_SECTION_COM_TYPE_LIBRARY_REDIRECTION)))
        return FALSE;

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        if (xmlstr_cmp(&attr_name, tlbidW))
        {
            if (!(entity->u.typelib.tlbid = xmlstrdupW(&attr_value))) return FALSE;
        }
        if (xmlstr_cmp(&attr_name, versionW))
        {
            if (!(entity->u.typelib.version = xmlstrdupW(&attr_value))) return FALSE;
        }
        if (xmlstr_cmp(&attr_name, helpdirW))
        {
            if (!(entity->u.typelib.helpdir = xmlstrdupW(&attr_value))) return FALSE;
        }
        else
        {
            DPRINT1("unknown attr %S=%S\n", attr_name.ptr , attr_value.ptr);
        }
    }

    if (error || end) return end;
    return parse_expect_end_elem(xmlbuf, typelibW);
}

static BOOL parse_window_class_elem(xmlbuf_t* xmlbuf, struct dll_redirect* dll)
{
    xmlstr_t    elem, content;
    BOOL        end = FALSE, ret = TRUE;
    struct entity*      entity;

    if (!(entity = add_entity(&dll->entities, ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION)))
        return FALSE;

    if (!parse_expect_no_attr(xmlbuf, &end)) return FALSE;
    if (end) return FALSE;

    if (!parse_text_content(xmlbuf, &content)) return FALSE;

    if (!(entity->u.class.name = xmlstrdupW(&content))) return FALSE;

    while (ret && (ret = next_xml_elem(xmlbuf, &elem)))
    {
        if (xmlstr_cmp_end(&elem, windowClassW))
        {
            ret = parse_end_element(xmlbuf);
            break;
        }
        else
        {
            DPRINT1("unknown elem %S\n", elem.ptr);
            ret = parse_unknown_elem(xmlbuf, &elem);
        }
    }

    return ret;
}

static BOOL parse_binding_redirect_elem(xmlbuf_t* xmlbuf)
{
    xmlstr_t    attr_name, attr_value;
    BOOL        end = FALSE, error;

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        if (xmlstr_cmp(&attr_name, oldVersionW))
        {
            DPRINT1("Not stored yet oldVersion=%S\n", attr_value.ptr);
        }
        else if (xmlstr_cmp(&attr_name, newVersionW))
        {
            DPRINT1("Not stored yet newVersion=%S\n", attr_value.ptr);
        }
        else
        {
            DPRINT1("unknown attr %S=%S\n", attr_name.ptr, attr_value.ptr);
        }
    }

    if (error || end) return end;
    return parse_expect_end_elem(xmlbuf, bindingRedirectW);
}

static BOOL parse_description_elem(xmlbuf_t* xmlbuf)
{
    xmlstr_t    elem, content;
    BOOL        end = FALSE, ret = TRUE;

    if (!parse_expect_no_attr(xmlbuf, &end) || end ||
        !parse_text_content(xmlbuf, &content))
        return FALSE;

    DPRINT("Got description %S\n", content.ptr);

    while (ret && (ret = next_xml_elem(xmlbuf, &elem)))
    {
        if (xmlstr_cmp_end(&elem, descriptionW))
        {
            ret = parse_end_element(xmlbuf);
            break;
        }
        else
        {
            DPRINT1("unknown elem %S\n", elem.ptr);
            ret = parse_unknown_elem(xmlbuf, &elem);
        }
    }

    return ret;
}

static BOOL parse_com_interface_external_proxy_stub_elem(xmlbuf_t* xmlbuf,
                                                         struct assembly* assembly)
{
    xmlstr_t            attr_name, attr_value;
    BOOL                end = FALSE, error;
    struct entity*      entity;

    entity = add_entity(&assembly->entities, ACTIVATION_CONTEXT_SECTION_COM_INTERFACE_REDIRECTION);
    if (!entity) return FALSE;

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        if (xmlstr_cmp(&attr_name, iidW))
        {
            if (!(entity->u.proxy.iid = xmlstrdupW(&attr_value))) return FALSE;
        }
        if (xmlstr_cmp(&attr_name, nameW))
        {
            if (!(entity->u.proxy.name = xmlstrdupW(&attr_value))) return FALSE;
        }
        else
        {
            DPRINT1("unknown attr %S=%S\n", attr_name.ptr, attr_value.ptr);
        }
    }

    if (error || end) return end;
    return parse_expect_end_elem(xmlbuf, comInterfaceExternalProxyStubW);
}

static BOOL parse_clr_class_elem(xmlbuf_t* xmlbuf, struct assembly* assembly)
{
    xmlstr_t    attr_name, attr_value;
    BOOL        end = FALSE, error;
    struct entity*      entity;

    entity = add_entity(&assembly->entities, ACTIVATION_CONTEXT_SECTION_COM_PROGID_REDIRECTION);
    if (!entity) return FALSE;

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        if (xmlstr_cmp(&attr_name, nameW))
        {
            if (!(entity->u.clrclass.name = xmlstrdupW(&attr_value))) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, clsidW))
        {
            if (!(entity->u.clrclass.clsid = xmlstrdupW(&attr_value))) return FALSE;
        }
        else
        {
            DPRINT1("unknown attr %S=%S\n", attr_name.ptr, attr_value.ptr);
        }
    }

    if (error || end) return end;
    return parse_expect_end_elem(xmlbuf, clrClassW);
}

static BOOL parse_clr_surrogate_elem(xmlbuf_t* xmlbuf, struct assembly* assembly)
{
    xmlstr_t    attr_name, attr_value;
    BOOL        end = FALSE, error;
    struct entity*      entity;

    entity = add_entity(&assembly->entities, ACTIVATION_CONTEXT_SECTION_CLR_SURROGATES);
    if (!entity) return FALSE;

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        if (xmlstr_cmp(&attr_name, nameW))
        {
            if (!(entity->u.clrsurrogate.name = xmlstrdupW(&attr_value))) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, clsidW))
        {
            if (!(entity->u.clrsurrogate.clsid = xmlstrdupW(&attr_value))) return FALSE;
        }
        else
        {
            DPRINT1("unknown attr %S=%S\n", attr_name.ptr, attr_value.ptr);
        }
    }

    if (error || end) return end;
    return parse_expect_end_elem(xmlbuf, clrSurrogateW);
}

static BOOL parse_dependent_assembly_elem(xmlbuf_t* xmlbuf, struct actctx_loader* acl, BOOL optional)
{
    struct assembly_identity    ai;
    xmlstr_t                    elem;
    BOOL                        end = FALSE, ret = TRUE;

    if (!parse_expect_no_attr(xmlbuf, &end) || end) return end;

    memset(&ai, 0, sizeof(ai));
    ai.optional = optional;

    if (!parse_expect_elem(xmlbuf, assemblyIdentityW) ||
        !parse_assembly_identity_elem(xmlbuf, acl->actctx, &ai))
        return FALSE;

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
    BOOL end = FALSE, ret = TRUE, error, optional = FALSE;

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        if (xmlstr_cmp(&attr_name, optionalW))
        {
            static const WCHAR yesW[] = {'y','e','s',0};
            optional = xmlstr_cmpi( &attr_value, yesW );
            DPRINT1("optional=%S\n", attr_value.ptr);
        }
        else
        {
            DPRINT1("unknown attr %S=%S\n", attr_name.ptr, attr_value.ptr);
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
            DPRINT1("unknown element %S\n", elem.ptr);
            ret = parse_unknown_elem(xmlbuf, &elem);
        }
    }

    return ret;
}

static BOOL parse_noinherit_elem(xmlbuf_t* xmlbuf)
{
    BOOL end = FALSE;

    if (!parse_expect_no_attr(xmlbuf, &end)) return FALSE;
    return end || parse_expect_end_elem(xmlbuf, noInheritW);
}

static BOOL parse_noinheritable_elem(xmlbuf_t* xmlbuf)
{
    BOOL end = FALSE;

    if (!parse_expect_no_attr(xmlbuf, &end)) return FALSE;
    return end || parse_expect_end_elem(xmlbuf, noInheritableW);
}

static BOOL parse_file_elem(xmlbuf_t* xmlbuf, struct assembly* assembly)
{
    xmlstr_t    attr_name, attr_value, elem;
    BOOL        end = FALSE, error, ret = TRUE;
    struct dll_redirect* dll;

    if (!(dll = add_dll_redirect(assembly))) return FALSE;

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        if (xmlstr_cmp(&attr_name, nameW))
        {
            if (!(dll->name = xmlstrdupW(&attr_value))) return FALSE;
            DPRINT("name=%S\n", attr_value.ptr);
        }
        else if (xmlstr_cmp(&attr_name, hashW))
        {
            if (!(dll->hash = xmlstrdupW(&attr_value))) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, hashalgW))
        {
            static const WCHAR sha1W[] = {'S','H','A','1',0};
            if (!xmlstr_cmpi(&attr_value, sha1W))
                DPRINT1("hashalg should be SHA1, got %S\n", attr_value.ptr);
        }
        else
        {
            DPRINT1("unknown attr %S=%S\n", attr_name.ptr, attr_value.ptr);
        }
    }

    if (error || !dll->name) return FALSE;
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
            ret = parse_com_class_elem(xmlbuf, dll);
        }
        else if (xmlstr_cmp(&elem, comInterfaceProxyStubW))
        {
            ret = parse_cominterface_proxy_stub_elem(xmlbuf, dll);
        }
        else if (xmlstr_cmp(&elem, asmv2hashW))
        {
            DPRINT1("asmv2hash (undocumented) not supported\n");
            ret = parse_unknown_elem(xmlbuf, &elem);
        }
        else if (xmlstr_cmp(&elem, typelibW))
        {
            ret = parse_typelib_elem(xmlbuf, dll);
        }
        else if (xmlstr_cmp(&elem, windowClassW))
        {
            ret = parse_window_class_elem(xmlbuf, dll);
        }
        else
        {
            DPRINT1("unknown elem %S\n", elem.ptr);
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
    BOOL        end = FALSE, error, version = FALSE, xmlns = FALSE, ret = TRUE;

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        if (xmlstr_cmp(&attr_name, manifestVersionW))
        {
            static const WCHAR v10W[] = {'1','.','0',0};
            if (!xmlstr_cmp(&attr_value, v10W))
            {
                DPRINT1("wrong version %S\n", attr_value.ptr);
                return FALSE;
            }
            version = TRUE;
        }
        else if (xmlstr_cmp(&attr_name, xmlnsW))
        {
            if (!xmlstr_cmp(&attr_value, manifestv1W) && !xmlstr_cmp(&attr_value, manifestv3W))
            {
                DPRINT1("wrong namespace %S\n", attr_value.ptr);
                return FALSE;
            }
            xmlns = TRUE;
        }
        else
        {
            DPRINT1("unknown attr %S=%S\n", attr_name.ptr, attr_value.ptr);
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

    if (xmlstr_cmp(&elem, noInheritableW))
    {
        if (!parse_noinheritable_elem(xmlbuf) || !next_xml_elem(xmlbuf, &elem))
            return FALSE;
    }
    else if ((assembly->type == ASSEMBLY_MANIFEST || assembly->type == ASSEMBLY_SHARED_MANIFEST) &&
             assembly->no_inherit)
        return FALSE;

    while (ret)
    {
        if (xmlstr_cmp_end(&elem, assemblyW))
        {
            ret = parse_end_element(xmlbuf);
            break;
        }
        else if (xmlstr_cmp(&elem, descriptionW))
        {
            ret = parse_description_elem(xmlbuf);
        }
        else if (xmlstr_cmp(&elem, comInterfaceExternalProxyStubW))
        {
            ret = parse_com_interface_external_proxy_stub_elem(xmlbuf, assembly);
        }
        else if (xmlstr_cmp(&elem, dependencyW))
        {
            ret = parse_dependency_elem(xmlbuf, acl);
        }
        else if (xmlstr_cmp(&elem, fileW))
        {
            ret = parse_file_elem(xmlbuf, assembly);
        }
        else if (xmlstr_cmp(&elem, clrClassW))
        {
            ret = parse_clr_class_elem(xmlbuf, assembly);
        }
        else if (xmlstr_cmp(&elem, clrSurrogateW))
        {
            ret = parse_clr_surrogate_elem(xmlbuf, assembly);
        }
        else if (xmlstr_cmp(&elem, assemblyIdentityW))
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
            DPRINT1("unknown element %S\n", elem.ptr);
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

    if (!next_xml_elem(xmlbuf, &elem)) return STATUS_SXS_CANT_GEN_ACTCTX;

    if (xmlstr_cmp(&elem, xmlW) &&
        (!parse_xml_header(xmlbuf) || !next_xml_elem(xmlbuf, &elem)))
        return STATUS_SXS_CANT_GEN_ACTCTX;

    if (!xmlstr_cmp(&elem, assemblyW))
    {
        DPRINT1("root element is %S, not <assembly>\n", elem.ptr);
        return STATUS_SXS_CANT_GEN_ACTCTX;
    }

    if (!parse_assembly_elem(xmlbuf, acl, assembly, ai))
    {
        DPRINT1("failed to parse manifest %S\n", assembly->manifest.info );
        return STATUS_SXS_CANT_GEN_ACTCTX;
    }

    if (next_xml_elem(xmlbuf, &elem))
    {
        DPRINT1("unexpected element %S\n", elem.ptr);
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
    if (RtlIsTextUnicode( (PVOID) buffer, size, &unicode_tests ))
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
        /* let's assume utf-8 for now */
        int len;

        _SEH2_TRY
        {
            len = mbstowcs( NULL, buffer, size);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            DPRINT1("Exception accessing buffer\n");
            return STATUS_SXS_CANT_GEN_ACTCTX;
        }
        _SEH2_END;

        DPRINT("len = %x\n", len);
        WCHAR *new_buff;

        if (len == -1)
        {
            DPRINT1( "utf-8 conversion failed\n" );
            return STATUS_SXS_CANT_GEN_ACTCTX;
        }
        if (!(new_buff = RtlAllocateHeap( RtlGetProcessHeap(), HEAP_ZERO_MEMORY, len * sizeof(WCHAR) )))
            return STATUS_NO_MEMORY;

        mbstowcs( new_buff, buffer, len);
        xmlbuf.ptr = new_buff;
        DPRINT("Buffer %S\n", new_buff);
        xmlbuf.end = xmlbuf.ptr + len;
        status = parse_manifest_buffer( acl, assembly, ai, &xmlbuf );

        RtlFreeHeap( RtlGetProcessHeap(), 0, new_buff );
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
    return NtOpenFile( handle, GENERIC_READ, &attr, &io, FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_ALERT );
}

static NTSTATUS get_module_filename( HMODULE module, UNICODE_STRING *str, unsigned int extra_len )
{
    NTSTATUS status;
    ULONG magic;
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

    if (status != STATUS_SUCCESS) return status;

    /* Fixme: WINE uses FileEndOfFileInformation with NtQueryInformationFile. */
    status = NtQueryInformationFile( file, &io, &info, sizeof(info), FileStandardInformation);

    if (status == STATUS_SUCCESS)
        status = parse_manifest(acl, ai, filename, directory, shared, base, info.EndOfFile.QuadPart);

    NtUnmapViewOfSection( NtCurrentProcess(), base );
    NtClose( mapping );
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
    else status = STATUS_RESOURCE_DATA_NOT_FOUND;
    RtlFreeUnicodeString( &nameW );
    return status;
}

static WCHAR *lookup_manifest_file( HANDLE dir, struct assembly_identity *ai )
{
    static const WCHAR lookup_fmtW[] =
        {'%','s','_','%','s','_','%','s','_','%','u','.','%','u','.','*','.','*','_',
         '*', /* FIXME */
         '.','m','a','n','i','f','e','s','t',0};

    WCHAR *lookup, *ret = NULL;
    UNICODE_STRING lookup_us;
    IO_STATUS_BLOCK io;
    unsigned int data_pos = 0, data_len;
    char buffer[8192];

    if (!(lookup = RtlAllocateHeap( RtlGetProcessHeap(), 0,
                                    (strlenW(ai->arch) + strlenW(ai->name)
                                     + strlenW(ai->public_key) + 20) * sizeof(WCHAR)
                                    + sizeof(lookup_fmtW) )))
        return NULL;

    sprintfW( lookup, lookup_fmtW, ai->arch, ai->name, ai->public_key, ai->version.major, ai->version.minor);
    RtlInitUnicodeString( &lookup_us, lookup );

    NtQueryDirectoryFile( dir, 0, NULL, NULL, &io, buffer, sizeof(buffer),
                          FileBothDirectoryInformation, FALSE, &lookup_us, TRUE );
    if (io.Status == STATUS_SUCCESS)
    {
        FILE_BOTH_DIR_INFORMATION *dir_info;
        WCHAR *tmp;
        ULONG build, revision;

        data_len = io.Information;

        for (;;)
        {
            if (data_pos >= data_len)
            {
                NtQueryDirectoryFile( dir, 0, NULL, NULL, &io, buffer, sizeof(buffer),
                                      FileBothDirectoryInformation, FALSE, &lookup_us, FALSE );
                if (io.Status != STATUS_SUCCESS) break;
                data_len = io.Information;
                data_pos = 0;
            }
            dir_info = (FILE_BOTH_DIR_INFORMATION*)(buffer + data_pos);

            if (dir_info->NextEntryOffset) data_pos += dir_info->NextEntryOffset;
            else data_pos = data_len;

            tmp = (WCHAR *)dir_info->FileName + (strchrW(lookup, '*') - lookup);
            build = atoiW(tmp);
            if (build < ai->version.build) continue;
            tmp = strchrW(tmp, '.') + 1;
            revision = atoiW(tmp);
            if (build == ai->version.build && revision < ai->version.revision)
                continue;
            ai->version.build = build;
            ai->version.revision = revision;

            if ((ret = RtlAllocateHeap( RtlGetProcessHeap(), 0, dir_info->FileNameLength * sizeof(WCHAR) )))
            {
                memcpy( ret, dir_info->FileName, dir_info->FileNameLength );
                ret[dir_info->FileNameLength/sizeof(WCHAR)] = 0;
            }
            break;
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

    if (!NtOpenFile( &handle, GENERIC_READ, &attr, &io, FILE_SHARE_READ | FILE_SHARE_WRITE,
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

    DPRINT1( "looking for name=%S version=%u.%u.%u.%u arch=%S\n",
           ai->name, ai->version.major, ai->version.minor, ai->version.build, ai->version.revision, ai->arch );

    if ((status = lookup_winsxs(acl, ai)) != STATUS_NO_SUCH_FILE) return status;

    /* FIXME: add support for language specific lookup */

    nameW.Buffer = NULL;
    if (!(buffer = RtlAllocateHeap( RtlGetProcessHeap(), 0,
                                    (strlenW(acl->actctx->appdir.info) + 2 * strlenW(ai->name) + 2) * sizeof(WCHAR) + sizeof(dotManifestW) )))
        return STATUS_NO_MEMORY;

    if (!(directory = build_assembly_dir( ai )))
    {
        RtlFreeHeap( RtlGetProcessHeap(), 0, buffer );
        return STATUS_NO_MEMORY;
    }

    /* lookup in appdir\name.dll
     *           appdir\name.manifest
     *           appdir\name\name.dll
     *           appdir\name\name.manifest
     */
    strcpyW( buffer, acl->actctx->appdir.info );
    p = buffer + strlenW(buffer);
    for (i = 0; i < 2; i++)
    {
        *p++ = '\\';
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
            if (!acl->dependencies[i].optional)
            {
                DPRINT1( "Could not find dependent assembly %S\n", acl->dependencies[i].name );
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

    if (flags & QUERY_ACTCTX_FLAG_USE_ACTIVE_ACTCTX)
    {
        if (NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame)
            *handle = NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame->ActivationContext;
    }
    else if (flags & (QUERY_ACTCTX_FLAG_ACTCTX_IS_ADDRESS|QUERY_ACTCTX_FLAG_ACTCTX_IS_HMODULE))
    {
        ULONG magic;
        LDR_DATA_TABLE_ENTRY *pldr;

        LdrLockLoaderLock( 0, NULL, &magic );
        if (!LdrFindEntryForAddress( *handle, &pldr ))
        {
            if ((flags & QUERY_ACTCTX_FLAG_ACTCTX_IS_HMODULE) && *handle != pldr->DllBase)
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

static NTSTATUS fill_keyed_data(PACTCTX_SECTION_KEYED_DATA data, PVOID v1, PVOID v2, unsigned int i)
{
    data->ulDataFormatVersion = 1;
    data->lpData = v1;
    data->ulLength = 20; /* FIXME */
    data->lpSectionGlobalData = NULL; /* FIXME */
    data->ulSectionGlobalDataLength = 0; /* FIXME */
    data->lpSectionBase = v2;
    data->ulSectionTotalLength = 0; /* FIXME */
    data->hActCtx = NULL;
    if (data->cbSize >= offsetof(ACTCTX_SECTION_KEYED_DATA, ulAssemblyRosterIndex) + sizeof(ULONG))
        data->ulAssemblyRosterIndex = i + 1;

    return STATUS_SUCCESS;
}

static NTSTATUS find_dll_redirection(ACTIVATION_CONTEXT* actctx, const UNICODE_STRING *section_name,
                                     PACTCTX_SECTION_KEYED_DATA data)
{
    unsigned int i, j, snlen = section_name->Length / sizeof(WCHAR);

    for (i = 0; i < actctx->num_assemblies; i++)
    {
        struct assembly *assembly = &actctx->assemblies[i];
        for (j = 0; j < assembly->num_dlls; j++)
        {
            struct dll_redirect *dll = &assembly->dlls[j];
            if (!strncmpiW(section_name->Buffer, dll->name, snlen) && !dll->name[snlen])
                return fill_keyed_data(data, dll, assembly, i);
        }
    }
    return STATUS_SXS_KEY_NOT_FOUND;
}

static NTSTATUS find_window_class(ACTIVATION_CONTEXT* actctx, const UNICODE_STRING *section_name,
                                  PACTCTX_SECTION_KEYED_DATA data)
{
    unsigned int i, j, k, snlen = section_name->Length / sizeof(WCHAR);

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
                    if (!strncmpiW(section_name->Buffer, entity->u.class.name, snlen) && !entity->u.class.name[snlen])
                        return fill_keyed_data(data, entity, dll, i);
                }
            }
        }
    }
    return STATUS_SXS_KEY_NOT_FOUND;
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
    case ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION:
    case ACTIVATION_CONTEXT_SECTION_COM_INTERFACE_REDIRECTION:
    case ACTIVATION_CONTEXT_SECTION_COM_TYPE_LIBRARY_REDIRECTION:
    case ACTIVATION_CONTEXT_SECTION_COM_PROGID_REDIRECTION:
    case ACTIVATION_CONTEXT_SECTION_GLOBAL_OBJECT_RENAME_TABLE:
    case ACTIVATION_CONTEXT_SECTION_CLR_SURROGATES:
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

    if (!RtlCreateActivationContext( &handle, &ctx )) process_actctx = check_actctx(handle);
}

/* FUNCTIONS ***************************************************************/

NTSTATUS WINAPI RtlCreateActivationContext( HANDLE *handle,  void *ptr )
{
    const ACTCTXW *pActCtx = ptr;
    const WCHAR *directory = NULL;
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


    if (!(actctx = RtlAllocateHeap( RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*actctx) )))
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

        if ((status = get_module_filename( NtCurrentTeb()->ProcessEnvironmentBlock->ImageBaseAddress, &dir, 0 )))
            goto error;

        if ((p = strrchrW( dir.Buffer, '\\' ))) p[1] = 0;
        actctx->appdir.info = dir.Buffer;
    }

    nameW.Buffer = NULL;
    if (pActCtx->lpSource)
    {
        if (!RtlDosPathNameToNtPathName_U(pActCtx->lpSource, &nameW, NULL, NULL))
        {
            status = STATUS_NO_SUCH_FILE;
            goto error;
        }
        status = open_nt_file( &file, &nameW );
        if (status)
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

    if (status == STATUS_SUCCESS) status = parse_depend_manifests(&acl);
    free_depend_manifests( &acl );

    if (status == STATUS_SUCCESS) *handle = actctx;
    else actctx_release( actctx );

    return status;

error:
    if (file) NtClose( file );
    actctx_release( actctx );
    return status;
}

VOID
NTAPI
RtlAddRefActivationContext(HANDLE handle)
{
    ACTIVATION_CONTEXT *actctx;

    if ((actctx = check_actctx( handle ))) actctx_addref( actctx );
}

VOID
NTAPI
RtlReleaseActivationContext( HANDLE handle )
{
    ACTIVATION_CONTEXT *actctx;

    if ((actctx = check_actctx( handle ))) actctx_release( actctx );
}

NTSTATUS
NTAPI RtlActivateActivationContext( ULONG unknown, HANDLE handle, PULONG_PTR cookie )
{
    RTL_ACTIVATION_CONTEXT_STACK_FRAME *frame;

    if (!(frame = RtlAllocateHeap( RtlGetProcessHeap(), 0, sizeof(*frame) )))
        return STATUS_NO_MEMORY;

    frame->Previous = NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame;
    frame->ActivationContext = handle;
    frame->Flags = 0;

    NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame = frame;
    RtlAddRefActivationContext( handle );

    *cookie = (ULONG_PTR)frame;
    DPRINT( "%p cookie=%lx\n", handle, *cookie );
    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
RtlDeactivateActivationContext( ULONG flags, ULONG_PTR cookie )
{
    RTL_ACTIVATION_CONTEXT_STACK_FRAME *frame, *top;

    DPRINT( "%x cookie=%lx\n", flags, cookie );

    /* find the right frame */
    top = NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame;
    for (frame = top; frame; frame = frame->Previous)
        if ((ULONG_PTR)frame == cookie) break;

    if (!frame)
        RtlRaiseStatus( STATUS_SXS_INVALID_DEACTIVATION );

    if (frame != top && !(flags & DEACTIVATE_ACTCTX_FLAG_FORCE_EARLY_DEACTIVATION))
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
NTAPI RtlFreeThreadActivationContextStack(void)
{
    RTL_ACTIVATION_CONTEXT_STACK_FRAME *frame;

    frame = NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame;
    while (frame)
    {
        RTL_ACTIVATION_CONTEXT_STACK_FRAME *prev = frame->Previous;
        RtlReleaseActivationContext( frame->ActivationContext );
        RtlFreeHeap( RtlGetProcessHeap(), 0, frame );
        frame = prev;
    }
    NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame = NULL;
}


NTSTATUS
NTAPI RtlGetActiveActivationContext( HANDLE *handle )
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


BOOLEAN
NTAPI RtlIsActivationContextActive( HANDLE handle )
{
    RTL_ACTIVATION_CONTEXT_STACK_FRAME *frame;

    for (frame = NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame; frame; frame = frame->Previous)
        if (frame->ActivationContext == handle) return TRUE;
    return FALSE;
}

NTSTATUS
NTAPI
RtlQueryInformationActivationContext( ULONG flags, HANDLE handle, PVOID subinst,
                                      ULONG class, PVOID buffer,
                                      SIZE_T bufsize, SIZE_T *retlen )
{
    ACTIVATION_CONTEXT *actctx;
    NTSTATUS status;

    DPRINT("%08x %p %p %u %p %ld %p\n", flags, handle,
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
            if (!(flags & QUERY_ACTCTX_FLAG_NO_ADDREF)) RtlAddRefActivationContext( handle );
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
                ptr += ad_len;
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

    default:
        DPRINT( "class %u not implemented\n", class );
        return STATUS_NOT_IMPLEMENTED;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlFindActivationContextSectionString( ULONG flags, const GUID *guid, ULONG section_kind,
                                       UNICODE_STRING *section_name, PVOID ptr )
{
    PACTCTX_SECTION_KEYED_DATA data = ptr;
    NTSTATUS status = STATUS_SXS_KEY_NOT_FOUND;

    if (guid)
    {
        DPRINT1("expected guid == NULL\n");
        return STATUS_INVALID_PARAMETER;
    }
    if (flags & ~FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX)
    {
        DPRINT1("unknown flags %08x\n", flags);
        return STATUS_INVALID_PARAMETER;
    }
    if (!data || data->cbSize < offsetof(ACTCTX_SECTION_KEYED_DATA, ulAssemblyRosterIndex) ||
        !section_name || !section_name->Buffer)
    {
        DPRINT1("invalid parameter\n");
        return STATUS_INVALID_PARAMETER;
    }

    ASSERT(NtCurrentTeb());
    ASSERT(NtCurrentTeb()->ActivationContextStackPointer);

    if (NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame)
    {
        ACTIVATION_CONTEXT *actctx = check_actctx(NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame->ActivationContext);
        if (actctx) status = find_string( actctx, section_kind, section_name, flags, data );
    }

    if (status != STATUS_SUCCESS)
        status = find_string( process_actctx, section_kind, section_name, flags, data );

    return status;
}

/* Stubs */

NTSTATUS
NTAPI
RtlAllocateActivationContextStack(IN PVOID *Context)
{
    PACTIVATION_CONTEXT_STACK ContextStack;
    ContextStack = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof (ACTIVATION_CONTEXT_STACK) );
    if (!ContextStack)
    {
        return STATUS_NO_MEMORY;
    }

    ContextStack->ActiveFrame = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(RTL_ACTIVATION_CONTEXT_STACK_FRAME));
    if (!ContextStack->ActiveFrame) return STATUS_NO_MEMORY;

    *Context = ContextStack;

    /* FIXME: Documentation on MSDN reads that activation contexts are only created
              for modules that have a valid manifest file or resource */
    actctx_init();

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlActivateActivationContextUnsafeFast(IN PRTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED Frame,
                                       IN PVOID Context)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
RtlDeactivateActivationContextUnsafeFast(IN PRTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED Frame)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
NTAPI
RtlZombifyActivationContext(PVOID Context)
{
    UNIMPLEMENTED;

    if (Context == ACTCTX_FAKE_HANDLE)
        return STATUS_SUCCESS;

    return STATUS_NOT_IMPLEMENTED;
}

