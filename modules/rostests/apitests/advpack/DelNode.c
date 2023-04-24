/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for advpack.dll DelNode function
 * PROGRAMMERS:     Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */
#include <apitest.h>

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <stdio.h>

/* Flags for DelNode */
#define ADN_DEL_IF_EMPTY            0x00000001
#define ADN_DONT_DEL_SUBDIRS        0x00000002
#define ADN_DONT_DEL_DIR            0x00000004
#define ADN_DEL_UNC_PATHS           0x00000008

/* DelNodeA and DelNodeW in advpack.dll */
typedef HRESULT (WINAPI *DELNODEA)(LPCSTR, DWORD);
typedef HRESULT (WINAPI *DELNODEW)(LPCWSTR, DWORD);

static HINSTANCE s_hAdvPack = NULL;
static DELNODEA s_pDelNodeA = NULL;
static DELNODEW s_pDelNodeW = NULL;

typedef struct {
    const char *item;
    BOOL is_dir;
} NODEA;

static const NODEA s_nodesA[] =
{
    { "dir1", TRUE },   /* 0 */
    { "dir1\\dir2", TRUE }, /* 1 */
    { "dir1\\dir2\\file1", FALSE }, /* 2 */
};

typedef struct {
    const WCHAR *item;
    BOOL is_dir;
} NODEW;

static const NODEW s_nodesW[] =
{
    { L"dir1", TRUE },   /* 0 */
    { L"dir1\\dir2", TRUE }, /* 1 */
    { L"dir1\\dir2\\file1", FALSE }, /* 2 */
};

typedef enum
{
    EAT_CREATE,
    EAT_DELETE,
    EAT_CHECK_EXIST,
    EAT_CHECK_NON_EXIST,
    EAT_CALL
} ENTRY_ACTION_TYPE;

typedef struct {
    int node;
    ENTRY_ACTION_TYPE action_type;
    DWORD flags;
    HRESULT ret;
} ENTRY;

/* TODO: ADN_DEL_UNC_PATHS */
#define FLAGS0  0
#define FLAGS1  ADN_DEL_IF_EMPTY
#define FLAGS2  ADN_DONT_DEL_DIR
#define FLAGS3  ADN_DONT_DEL_SUBDIRS
#define FLAGS4  (ADN_DONT_DEL_DIR | ADN_DONT_DEL_SUBDIRS)
#define FLAGS5  (ADN_DEL_IF_EMPTY | ADN_DONT_DEL_DIR)
#define FLAGS6  (ADN_DEL_IF_EMPTY | ADN_DONT_DEL_SUBDIRS)
#define FLAGS7  (ADN_DEL_IF_EMPTY | ADN_DONT_DEL_DIR | ADN_DONT_DEL_SUBDIRS)
#define FLAGS8  0xFFFFFFFF

static const ENTRY s_entries[] =
{
    /* #0 */ { 0, EAT_CREATE },
    /* #1 */ { 1, EAT_CREATE },
    /* #2 */ { 1, EAT_DELETE },
    /* #3 */ { 0, EAT_DELETE },

    /* ------------------------------- */

    /* #4 */ { 0, EAT_CREATE },
    /* #5 */ { 1, EAT_CREATE },
    /* #6 */ { 0, EAT_CALL, FLAGS0, S_OK },
    /* #7 */ { 0, EAT_CHECK_NON_EXIST },

    /* #8 */ { 0, EAT_CREATE },
    /* #9 */ { 1, EAT_CREATE },
    /* #10 */ { 0, EAT_CALL, FLAGS1, E_FAIL },
    /* #11 */ { 0, EAT_CHECK_EXIST },

    /* #12 */ { 0, EAT_CREATE },
    /* #13 */ { 1, EAT_CREATE },
    /* #14 */ { 0, EAT_CALL, FLAGS2, S_OK },
    /* #15 */ { 0, EAT_CHECK_EXIST },

    /* #16 */ { 0, EAT_CREATE },
    /* #17 */ { 1, EAT_CREATE },
    /* #18 */ { 0, EAT_CALL, FLAGS3, S_OK },
    /* #19 */ { 0, EAT_CHECK_EXIST },

    /* #20 */ { 0, EAT_CREATE },
    /* #21 */ { 1, EAT_CREATE },
    /* #22 */ { 0, EAT_CALL, FLAGS4, S_OK },
    /* #23 */ { 0, EAT_CHECK_EXIST },

    /* #24 */ { 0, EAT_CREATE },
    /* #25 */ { 1, EAT_CREATE },
    /* #26 */ { 0, EAT_CALL, FLAGS5, E_FAIL },
    /* #27 */ { 0, EAT_CHECK_EXIST },

    /* #28 */ { 0, EAT_CREATE },
    /* #29 */ { 1, EAT_CREATE },
    /* #30 */ { 0, EAT_CALL, FLAGS6, E_FAIL },
    /* #31 */ { 0, EAT_CHECK_EXIST },

    /* #32 */ { 0, EAT_CREATE },
    /* #33 */ { 1, EAT_CREATE },
    /* #34 */ { 0, EAT_CALL, FLAGS7, E_FAIL },
    /* #35 */ { 0, EAT_CHECK_EXIST },

    /* #36 */ { 0, EAT_CREATE },
    /* #37 */ { 1, EAT_CREATE },
    /* #38 */ { 0, EAT_CALL, FLAGS8, E_FAIL },
    /* #39 */ { 0, EAT_CHECK_EXIST },

    /* ------------------------------- */

    /* #40 */ { 0, EAT_CREATE },
    /* #41 */ { 1, EAT_CREATE },
    /* #42 */ { 2, EAT_CREATE },
    /* #43 */ { 0, EAT_CALL, FLAGS0, S_OK },
    /* #44 */ { 0, EAT_CHECK_NON_EXIST },

    /* #45 */ { 0, EAT_CREATE },
    /* #46 */ { 1, EAT_CREATE },
    /* #47 */ { 2, EAT_CREATE },
    /* #48 */ { 0, EAT_CALL, FLAGS1, E_FAIL },
    /* #49 */ { 0, EAT_CHECK_EXIST },
    /* #50 */ { 1, EAT_CHECK_EXIST },
    /* #51 */ { 2, EAT_CHECK_EXIST },

    /* #52 */ { 0, EAT_CREATE },
    /* #53 */ { 1, EAT_CREATE },
    /* #54 */ { 2, EAT_CREATE },
    /* #55 */ { 0, EAT_CALL, FLAGS2, S_OK },
    /* #56 */ { 0, EAT_CHECK_EXIST },
    /* #57 */ { 1, EAT_CHECK_NON_EXIST },
    /* #58 */ { 2, EAT_CHECK_NON_EXIST },

    /* #59 */ { 0, EAT_CREATE },
    /* #60 */ { 1, EAT_CREATE },
    /* #61 */ { 2, EAT_CREATE },
    /* #62 */ { 0, EAT_CALL, FLAGS3, S_OK },
    /* #63 */ { 0, EAT_CHECK_EXIST },
    /* #64 */ { 1, EAT_CHECK_EXIST },
    /* #65 */ { 2, EAT_CHECK_EXIST },

    /* #66 */ { 0, EAT_CREATE },
    /* #67 */ { 1, EAT_CREATE },
    /* #68 */ { 2, EAT_CREATE },
    /* #69 */ { 0, EAT_CALL, FLAGS4, S_OK },
    /* #70 */ { 0, EAT_CHECK_EXIST },
    /* #71 */ { 1, EAT_CHECK_EXIST },
    /* #72 */ { 2, EAT_CHECK_EXIST },

    /* #73 */ { 0, EAT_CREATE },
    /* #74 */ { 1, EAT_CREATE },
    /* #75 */ { 2, EAT_CREATE },
    /* #76 */ { 0, EAT_CALL, FLAGS5, E_FAIL },
    /* #77 */ { 0, EAT_CHECK_EXIST },
    /* #78 */ { 1, EAT_CHECK_EXIST },
    /* #79 */ { 2, EAT_CHECK_EXIST },

    /* #80 */ { 0, EAT_CREATE },
    /* #81 */ { 1, EAT_CREATE },
    /* #82 */ { 2, EAT_CREATE },
    /* #83 */ { 0, EAT_CALL, FLAGS6, E_FAIL },
    /* #84 */ { 0, EAT_CHECK_EXIST },
    /* #85 */ { 1, EAT_CHECK_EXIST },
    /* #86 */ { 2, EAT_CHECK_EXIST },

    /* #87 */ { 0, EAT_CREATE },
    /* #88 */ { 1, EAT_CREATE },
    /* #89 */ { 2, EAT_CREATE },
    /* #90 */ { 0, EAT_CALL, FLAGS7, E_FAIL },
    /* #91 */ { 0, EAT_CHECK_EXIST },
    /* #92 */ { 1, EAT_CHECK_EXIST },
    /* #93 */ { 2, EAT_CHECK_EXIST },

    /* #94 */ { 0, EAT_CREATE },
    /* #95 */ { 1, EAT_CREATE },
    /* #96 */ { 2, EAT_CREATE },
    /* #97 */ { 0, EAT_CALL, FLAGS8, E_FAIL },
    /* #98 */ { 0, EAT_CHECK_EXIST },
    /* #99 */ { 1, EAT_CHECK_EXIST },
    /* #100 */ { 2, EAT_CHECK_EXIST },

    /* ------------------------------- */

    /* #101 */ { 1, EAT_CREATE },
    /* #102 */ { 2, EAT_CREATE },
    /* #103 */ { 1, EAT_CALL, FLAGS0, S_OK },
    /* #104 */ { 1, EAT_CHECK_NON_EXIST },

    /* #105 */ { 1, EAT_CREATE },
    /* #106 */ { 2, EAT_CREATE },
    /* #107 */ { 1, EAT_CALL, FLAGS1, E_FAIL },
    /* #108 */ { 1, EAT_CHECK_EXIST },
    /* #109 */ { 2, EAT_CHECK_EXIST },

    /* #110 */ { 1, EAT_CREATE },
    /* #111 */ { 2, EAT_CREATE },
    /* #112 */ { 1, EAT_CALL, FLAGS2, S_OK },
    /* #113 */ { 1, EAT_CHECK_EXIST },
    /* #114 */ { 2, EAT_CHECK_NON_EXIST },

    /* #115 */ { 1, EAT_CREATE },
    /* #116 */ { 2, EAT_CREATE },
    /* #117 */ { 1, EAT_CALL, FLAGS3, S_OK },
    /* #118 */ { 1, EAT_CHECK_NON_EXIST },

    /* #119 */ { 1, EAT_CREATE },
    /* #120 */ { 2, EAT_CREATE },
    /* #121 */ { 1, EAT_CALL, FLAGS4, S_OK },
    /* #122 */ { 1, EAT_CHECK_EXIST },
    /* #123 */ { 2, EAT_CHECK_NON_EXIST },

    /* #124 */ { 1, EAT_CREATE },
    /* #125 */ { 2, EAT_CREATE },
    /* #126 */ { 1, EAT_CALL, FLAGS5, E_FAIL },
    /* #127 */ { 1, EAT_CHECK_EXIST },
    /* #128 */ { 2, EAT_CHECK_EXIST },

    /* #129 */ { 1, EAT_CREATE },
    /* #130 */ { 2, EAT_CREATE },
    /* #131 */ { 1, EAT_CALL, FLAGS6, E_FAIL },
    /* #132 */ { 1, EAT_CHECK_EXIST },
    /* #133 */ { 2, EAT_CHECK_EXIST },

    /* #134 */ { 1, EAT_CREATE },
    /* #135 */ { 2, EAT_CREATE },
    /* #136 */ { 1, EAT_CALL, FLAGS7, E_FAIL },
    /* #137 */ { 1, EAT_CHECK_EXIST },
    /* #138 */ { 2, EAT_CHECK_EXIST },

    /* #139 */ { 1, EAT_CREATE },
    /* #140 */ { 2, EAT_CREATE },
    /* #141 */ { 1, EAT_CALL, FLAGS8, E_FAIL },
    /* #142 */ { 1, EAT_CHECK_EXIST },
    /* #143 */ { 2, EAT_CHECK_EXIST },

    /* ------------------------------- */

    /* #144 */ { 2, EAT_DELETE },

    /* #145 */ { 1, EAT_CREATE },
    /* #146 */ { 1, EAT_CALL, FLAGS0, S_OK },
    /* #147 */ { 1, EAT_CHECK_NON_EXIST },

    /* #148 */ { 1, EAT_CREATE },
    /* #149 */ { 1, EAT_CALL, FLAGS1, S_OK },
    /* #150 */ { 1, EAT_CHECK_NON_EXIST },

    /* #151 */ { 1, EAT_CREATE },
    /* #152 */ { 1, EAT_CALL, FLAGS2, S_OK },
    /* #153 */ { 1, EAT_CHECK_EXIST },

    /* #154 */ { 1, EAT_CREATE },
    /* #155 */ { 1, EAT_CALL, FLAGS3, S_OK },
    /* #156 */ { 1, EAT_CHECK_NON_EXIST },

    /* #157 */ { 1, EAT_CREATE },
    /* #158 */ { 1, EAT_CALL, FLAGS4, S_OK },
    /* #159 */ { 1, EAT_CHECK_EXIST },

    /* #160 */ { 1, EAT_CREATE },
    /* #161 */ { 1, EAT_CALL, FLAGS5, S_OK },
    /* #162 */ { 1, EAT_CHECK_NON_EXIST },

    /* #163 */ { 1, EAT_CREATE },
    /* #164 */ { 1, EAT_CALL, FLAGS6, S_OK },
    /* #165 */ { 1, EAT_CHECK_NON_EXIST },

    /* #166 */ { 1, EAT_CREATE },
    /* #167 */ { 1, EAT_CALL, FLAGS7, S_OK },
    /* #168 */ { 1, EAT_CHECK_NON_EXIST },

    /* #169 */ { 1, EAT_CREATE },
    /* #170 */ { 1, EAT_CALL, FLAGS8, S_OK },
    /* #171 */ { 1, EAT_CHECK_NON_EXIST },

    /* ------------------------------- */

    /* #172 */ { 1, EAT_CREATE },

    /* #173 */ { 2, EAT_CREATE },
    /* #174 */ { 2, EAT_CALL, FLAGS0, S_OK },
    /* #175 */ { 2, EAT_CHECK_NON_EXIST },

    /* #176 */ { 2, EAT_CREATE },
    /* #177 */ { 2, EAT_CALL, FLAGS1, S_OK },
    /* #178 */ { 2, EAT_CHECK_NON_EXIST },

    /* #179 */ { 2, EAT_CREATE },
    /* #180 */ { 2, EAT_CALL, FLAGS2, S_OK },
    /* #181 */ { 2, EAT_CHECK_NON_EXIST },

    /* #182 */ { 2, EAT_CREATE },
    /* #183 */ { 2, EAT_CALL, FLAGS3, S_OK },
    /* #184 */ { 2, EAT_CHECK_NON_EXIST },

    /* #185 */ { 2, EAT_CREATE },
    /* #186 */ { 2, EAT_CALL, FLAGS4, S_OK },
    /* #187 */ { 2, EAT_CHECK_NON_EXIST },

    /* #188 */ { 2, EAT_CREATE },
    /* #189 */ { 2, EAT_CALL, FLAGS5, S_OK },
    /* #190 */ { 2, EAT_CHECK_NON_EXIST },

    /* #191 */ { 2, EAT_CREATE },
    /* #192 */ { 2, EAT_CALL, FLAGS6, S_OK },
    /* #193 */ { 2, EAT_CHECK_NON_EXIST },

    /* #194 */ { 2, EAT_CREATE },
    /* #195 */ { 2, EAT_CALL, FLAGS7, S_OK },
    /* #196 */ { 2, EAT_CHECK_NON_EXIST },

    /* #197 */ { 2, EAT_CREATE },
    /* #198 */ { 2, EAT_CALL, FLAGS8, S_OK },
    /* #199 */ { 2, EAT_CHECK_NON_EXIST },

    /* ------------------------------- */

    /* #200 */ { 2, EAT_DELETE },
    /* #201 */ { 1, EAT_DELETE },
    /* #202 */ { 0, EAT_DELETE },

    /* #203 */ { 2, EAT_CALL, FLAGS0, E_FAIL },
    /* #204 */ { 2, EAT_CALL, FLAGS1, E_FAIL },
    /* #205 */ { 2, EAT_CALL, FLAGS2, E_FAIL },
    /* #206 */ { 2, EAT_CALL, FLAGS3, E_FAIL },
    /* #207 */ { 2, EAT_CALL, FLAGS4, E_FAIL },
    /* #208 */ { 2, EAT_CALL, FLAGS5, E_FAIL },
    /* #209 */ { 2, EAT_CALL, FLAGS6, E_FAIL },
    /* #210 */ { 2, EAT_CALL, FLAGS7, E_FAIL },
    /* #211 */ { 2, EAT_CALL, FLAGS8, E_FAIL },

    /* #212 */ { 1, EAT_CALL, FLAGS0, E_FAIL },
    /* #213 */ { 1, EAT_CALL, FLAGS1, E_FAIL },
    /* #214 */ { 1, EAT_CALL, FLAGS2, E_FAIL },
    /* #215 */ { 1, EAT_CALL, FLAGS3, E_FAIL },
    /* #216 */ { 1, EAT_CALL, FLAGS4, E_FAIL },
    /* #217 */ { 1, EAT_CALL, FLAGS5, E_FAIL },
    /* #218 */ { 1, EAT_CALL, FLAGS6, E_FAIL },
    /* #219 */ { 1, EAT_CALL, FLAGS7, E_FAIL },
    /* #220 */ { 1, EAT_CALL, FLAGS8, E_FAIL },
};

char  s_cur_dir_A[MAX_PATH] = "";
WCHAR s_cur_dir_W[MAX_PATH] = L"";

static char *GetPathA(const char *item)
{
    static char s_path[MAX_PATH];
    lstrcpyA(s_path, s_cur_dir_A);
    lstrcatA(s_path, "\\");
    lstrcatA(s_path, item);
    return s_path;
}

static WCHAR *GetPathW(const WCHAR *item)
{
    static WCHAR s_path[MAX_PATH];
    lstrcpyW(s_path, s_cur_dir_W);
    lstrcatW(s_path, L"\\");
    lstrcatW(s_path, item);
    return s_path;
}

static void cleanupA(void)
{
    size_t i, k;
    for (k = 0; k < 4; ++k)
    {
        for (i = 0; i < _countof(s_nodesA); ++i)
        {
            const NODEA *node = &s_nodesA[i];
            char *path = GetPathA(node->item);

            DeleteFileA(path);
            RemoveDirectoryA(path);
        }
    }
}

static void Test_DelNodeA(void)
{
    DWORD attr;
    size_t i;

    if (!GetCurrentDirectoryA(_countof(s_cur_dir_A), s_cur_dir_A))
    {
        skip("GetCurrentDirectoryA failed\n");
        return;
    }

    cleanupA();

    for (i = 0; i < _countof(s_entries); ++i)
    {
        const ENTRY *entry = &s_entries[i];
        const NODEA *node = &s_nodesA[entry->node];
        char *path = GetPathA(node->item);
        int ret;

        switch (entry->action_type)
        {
        case EAT_CREATE:
            if (node->is_dir)
            {
                CreateDirectoryA(path, NULL);

                attr = GetFileAttributesA(path);
                ok(attr != INVALID_FILE_ATTRIBUTES, "Entry #%d: path:%s, attr:0x%08lX\n", i, path, attr);
                ok((attr & FILE_ATTRIBUTE_DIRECTORY), "Entry #%d: path:%s, attr:0x%08lX\n", i, path, attr);
            }
            else
            {
                fclose(fopen(path, "w"));

                attr = GetFileAttributesA(path);
                ok(attr != INVALID_FILE_ATTRIBUTES, "Entry #%d: attr was 0x%08lX\n", i, attr);
                ok(!(attr & FILE_ATTRIBUTE_DIRECTORY), "Entry #%d: attr was 0x%08lX\n", i, attr);
            }
            break;
        case EAT_DELETE:
            if (node->is_dir)
            {
                RemoveDirectoryA(path);
            }
            else
            {
                DeleteFileA(path);
            }
            attr = GetFileAttributesA(path);
            ok(attr == INVALID_FILE_ATTRIBUTES, "Entry #%d: cannot delete\n", i);
            break;
        case EAT_CHECK_EXIST:
            attr = GetFileAttributesA(path);
            ok(attr != INVALID_FILE_ATTRIBUTES, "Entry #%d: attr was 0x%08lX\n", i, attr);
            break;
        case EAT_CHECK_NON_EXIST:
            attr = GetFileAttributesA(path);
            ok(attr == INVALID_FILE_ATTRIBUTES, "Entry #%d: attr was 0x%08lX\n", i, attr);
            break;
        case EAT_CALL:
            ret = (*s_pDelNodeA)(path, entry->flags);
            ok(ret == entry->ret, "Entry #%d: ret:%d, path:%s, flags:0x%08lX\n",
               i, ret, path, entry->flags);
            break;
        }
    }

    cleanupA();
}

static void cleanupW(void)
{
    size_t i, k;
    for (k = 0; k < 4; ++k)
    {
        for (i = 0; i < _countof(s_nodesW); ++i)
        {
            const NODEW *node = &s_nodesW[i];
            WCHAR *path = GetPathW(node->item);

            DeleteFileW(path);
            RemoveDirectoryW(path);
        }
    }
}

static void Test_DelNodeW(void)
{
    DWORD attr;
    size_t i;

    if (!GetCurrentDirectoryW(_countof(s_cur_dir_W), s_cur_dir_W))
    {
        skip("GetCurrentDirectoryA failed\n");
        return;
    }

    cleanupW();

    for (i = 0; i < _countof(s_entries); ++i)
    {
        const ENTRY *entry = &s_entries[i];
        const NODEW *node = &s_nodesW[entry->node];
        WCHAR *path = GetPathW(node->item);
        int ret;

        switch (entry->action_type)
        {
        case EAT_CREATE:
            if (node->is_dir)
            {
                CreateDirectoryW(path, NULL);

                attr = GetFileAttributesW(path);
                ok(attr != INVALID_FILE_ATTRIBUTES, "Entry #%d: path:%S, attr:0x%08lX\n", i, path, attr);
                ok((attr & FILE_ATTRIBUTE_DIRECTORY), "Entry #%d: path:%S, attr:0x%08lX\n", i, path, attr);
            }
            else
            {
                fclose(_wfopen(path, L"w"));

                attr = GetFileAttributesW(path);
                ok(attr != INVALID_FILE_ATTRIBUTES, "Entry #%d: attr was 0x%08lX\n", i, attr);
                ok(!(attr & FILE_ATTRIBUTE_DIRECTORY), "Entry #%d: attr was 0x%08lX\n", i, attr);
            }
            break;
        case EAT_DELETE:
            if (node->is_dir)
            {
                RemoveDirectoryW(path);
            }
            else
            {
                DeleteFileW(path);
            }
            attr = GetFileAttributesW(path);
            ok(attr == INVALID_FILE_ATTRIBUTES, "Entry #%d: cannot delete\n", i);
            break;
        case EAT_CHECK_EXIST:
            attr = GetFileAttributesW(path);
            ok(attr != INVALID_FILE_ATTRIBUTES, "Entry #%d: attr was 0x%08lX\n", i, attr);
            break;
        case EAT_CHECK_NON_EXIST:
            attr = GetFileAttributesW(path);
            ok(attr == INVALID_FILE_ATTRIBUTES, "Entry #%d: attr was 0x%08lX\n", i, attr);
            break;
        case EAT_CALL:
            ret = (*s_pDelNodeW)(path, entry->flags);
            ok(ret == entry->ret, "Entry #%d: ret:%d, path:%S, flags:0x%08lX\n",
               i, ret, path, entry->flags);
            break;
        }
    }

    cleanupW();
}

START_TEST(DelNode)
{
    s_hAdvPack = LoadLibraryA("advpack.dll");
    if (s_hAdvPack == NULL)
    {
        skip("unable to load advpack.dll\n");
        return;
    }

    /* DelNodeA */
    s_pDelNodeA = (DELNODEA)GetProcAddress(s_hAdvPack, "DelNodeA");
    if (s_pDelNodeA)
    {
        Test_DelNodeA();
    }
    else
    {
        skip("DelNodeA not found in advpack.dll\n");
    }

    /* DelNodeW */
    s_pDelNodeW = (DELNODEW)GetProcAddress(s_hAdvPack, "DelNodeW");
    if (s_pDelNodeW)
    {
        Test_DelNodeW();
    }
    else
    {
        skip("DelNodeW not found in advpack.dll\n");
    }

    FreeLibrary(s_hAdvPack);
    s_hAdvPack = NULL;
}
