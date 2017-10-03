/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test to validate section flags in ntoskrnl
 * PROGRAMMER:      Mark Jansen
 */

#include <apitest.h>
#include <strsafe.h>

typedef struct KnownSections
{
    const char* Name;
    DWORD Required;
    DWORD Disallowed;
} KnownSections;

static struct KnownSections g_Sections[] = {
    {
        ".text",
        IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_NOT_PAGED,
        IMAGE_SCN_MEM_DISCARDABLE
    },
    {
        ".data",
        IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_NOT_PAGED,
        IMAGE_SCN_MEM_DISCARDABLE
    },
    {
        ".rsrc",
        IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ,
        IMAGE_SCN_MEM_DISCARDABLE
    },
    {
        ".rdata",
        IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ,
        IMAGE_SCN_MEM_DISCARDABLE
    },
    {
        ".reloc",
        IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_DISCARDABLE | IMAGE_SCN_MEM_READ,
        0
    },
    {
        "INIT",
        IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_DISCARDABLE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ,
        0
    },
    { NULL, 0 },
};

static char* Chr2Str(DWORD value)
{
    static char buf[512];
    buf[0] = '\0';
#define IFX(x)      if( value & IMAGE_SCN_##x )\
                    {\
                        if(buf[0]) { StringCchCatA(buf, _countof(buf), "|" ); }\
                        StringCchCatA(buf, _countof(buf), #x );\
                        value &= ~(IMAGE_SCN_##x);\
                    }
    IFX(TYPE_NO_PAD);
    IFX(CNT_CODE);
    IFX(CNT_INITIALIZED_DATA);
    IFX(CNT_UNINITIALIZED_DATA);
    IFX(LNK_OTHER);
    IFX(LNK_INFO);
    IFX(LNK_REMOVE);
    IFX(LNK_COMDAT);
    //IFX(NO_DEFER_SPEC_EXC);
    //IFX(GPREL);
    IFX(MEM_FARDATA);
    IFX(MEM_PURGEABLE);
    IFX(MEM_16BIT);
    IFX(MEM_LOCKED);
    IFX(MEM_PRELOAD);
    IFX(ALIGN_1BYTES);
    IFX(ALIGN_2BYTES);
    IFX(ALIGN_4BYTES);
    IFX(ALIGN_8BYTES);
    IFX(ALIGN_16BYTES);
    IFX(ALIGN_32BYTES);
    IFX(ALIGN_64BYTES);
    //IFX(ALIGN_128BYTES);
    //IFX(ALIGN_256BYTES);
    //IFX(ALIGN_512BYTES);
    //IFX(ALIGN_1024BYTES);
    //IFX(ALIGN_2048BYTES);
    //IFX(ALIGN_4096BYTES);
    //IFX(ALIGN_8192BYTES);
    IFX(LNK_NRELOC_OVFL);
    IFX(MEM_DISCARDABLE);
    IFX(MEM_NOT_CACHED);
    IFX(MEM_NOT_PAGED);
    IFX(MEM_SHARED);
    IFX(MEM_EXECUTE);
    IFX(MEM_READ);
    IFX(MEM_WRITE);
    if( value )
    {
        StringCchPrintfA(buf + strlen(buf), _countof(buf) - strlen(buf), "|0x%x", value);
    }
    return buf;
}


START_TEST(ntoskrnl_SectionFlags)
{
    char buf[MAX_PATH];
    HMODULE mod;
    GetSystemDirectoryA(buf, _countof(buf));
    StringCchCatA(buf, _countof(buf), "\\ntoskrnl.exe");

    mod = LoadLibraryExA( buf, NULL, LOAD_LIBRARY_AS_DATAFILE );
    if( mod != NULL )
    {
        // we have to take into account that a datafile is not returned at the exact address it's loaded at.
        PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)(((size_t)mod) & (~0xf));
        PIMAGE_NT_HEADERS nt;
        PIMAGE_SECTION_HEADER firstSection;
        WORD numSections, n;

        if( dos->e_magic != IMAGE_DOS_SIGNATURE )
        {
            skip("Couldn't find ntoskrnl.exe dos header\n");
            FreeLibrary(mod);
            return;
        }
        nt = (PIMAGE_NT_HEADERS)( ((PBYTE)dos) + dos->e_lfanew );
        if( nt->Signature != IMAGE_NT_SIGNATURE )
        {
            skip("Couldn't find ntoskrnl.exe nt header\n");
            FreeLibrary(mod);
            return;
        }
        firstSection = IMAGE_FIRST_SECTION(nt);
        numSections = nt->FileHeader.NumberOfSections;
        for( n = 0; n < numSections; ++n )
        {
            PIMAGE_SECTION_HEADER section = firstSection + n;
            char name[9] = {0};
            size_t i;
            StringCchCopyNA(name, _countof(name), (PCSTR)section->Name, 8);

            for( i = 0; g_Sections[i].Name; ++i )
            {
                if( !_strnicmp(name, g_Sections[i].Name, 8) )
                {
                    if( g_Sections[i].Required )
                    {
                        DWORD Flags = g_Sections[i].Required & section->Characteristics;
                        ok(Flags == g_Sections[i].Required,
                            "Missing required Characteristics on %s: %s\n",
                            name, Chr2Str(Flags ^ g_Sections[i].Required));
                    }
                    if( g_Sections[i].Disallowed )
                    {
                        DWORD Flags = g_Sections[i].Disallowed & section->Characteristics;
                        ok(!Flags, "Disallowed section Characteristics on %s: %s\n",
                            name, Chr2Str(section->Characteristics));
                    }
                    break;
                }
            }
        }
        FreeLibrary(mod);
    }
    else
    {
        skip("Couldn't load ntoskrnl.exe as datafile\n");
    }
}

