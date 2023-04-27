/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for advpack.dll DelNode function
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */
#include <apitest.h>

#include <windef.h>
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

typedef struct
{
    const char *item;
    BOOL is_dir;
} NODEA;

static const NODEA s_nodesA[] =
{
    { "dir1", TRUE },               /* 0 */
    { "dir1\\dir2", TRUE },         /* 1 */
    { "dir1\\dir2\\file1", FALSE }, /* 2 */
};

typedef struct
{
    const WCHAR *item;
    BOOL is_dir;
} NODEW;

static const NODEW s_nodesW[] =
{
    { L"dir1", TRUE },               /* 0 */
    { L"dir1\\dir2", TRUE },         /* 1 */
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

typedef struct
{
    INT lineno;
    INT node;
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

/* The test entries */
static const ENTRY s_entries[] =
{
    { __LINE__, 0, EAT_CREATE },
    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 1, EAT_DELETE },
    { __LINE__, 0, EAT_DELETE },

    { __LINE__, 0, EAT_CREATE },
    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 0, EAT_CALL, FLAGS0, S_OK },
    { __LINE__, 0, EAT_CHECK_NON_EXIST },

    { __LINE__, 0, EAT_CREATE },
    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 0, EAT_CALL, FLAGS1, E_FAIL },
    { __LINE__, 0, EAT_CHECK_EXIST },

    { __LINE__, 0, EAT_CREATE },
    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 0, EAT_CALL, FLAGS2, S_OK },
    { __LINE__, 0, EAT_CHECK_EXIST },

    { __LINE__, 0, EAT_CREATE },
    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 0, EAT_CALL, FLAGS3, S_OK },
    { __LINE__, 0, EAT_CHECK_EXIST },

    { __LINE__, 0, EAT_CREATE },
    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 0, EAT_CALL, FLAGS4, S_OK },
    { __LINE__, 0, EAT_CHECK_EXIST },

    { __LINE__, 0, EAT_CREATE },
    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 0, EAT_CALL, FLAGS5, E_FAIL },
    { __LINE__, 0, EAT_CHECK_EXIST },

    { __LINE__, 0, EAT_CREATE },
    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 0, EAT_CALL, FLAGS6, E_FAIL },
    { __LINE__, 0, EAT_CHECK_EXIST },

    { __LINE__, 0, EAT_CREATE },
    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 0, EAT_CALL, FLAGS7, E_FAIL },
    { __LINE__, 0, EAT_CHECK_EXIST },

    { __LINE__, 0, EAT_CREATE },
    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 0, EAT_CALL, FLAGS8, E_FAIL },
    { __LINE__, 0, EAT_CHECK_EXIST },

    { __LINE__, 0, EAT_CREATE },
    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 2, EAT_CREATE },
    { __LINE__, 0, EAT_CALL, FLAGS0, S_OK },
    { __LINE__, 0, EAT_CHECK_NON_EXIST },

    { __LINE__, 0, EAT_CREATE },
    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 2, EAT_CREATE },
    { __LINE__, 0, EAT_CALL, FLAGS1, E_FAIL },
    { __LINE__, 0, EAT_CHECK_EXIST },
    { __LINE__, 1, EAT_CHECK_EXIST },
    { __LINE__, 2, EAT_CHECK_EXIST },

    { __LINE__, 0, EAT_CREATE },
    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 2, EAT_CREATE },
    { __LINE__, 0, EAT_CALL, FLAGS2, S_OK },
    { __LINE__, 0, EAT_CHECK_EXIST },
    { __LINE__, 1, EAT_CHECK_NON_EXIST },
    { __LINE__, 2, EAT_CHECK_NON_EXIST },

    { __LINE__, 0, EAT_CREATE },
    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 2, EAT_CREATE },
    { __LINE__, 0, EAT_CALL, FLAGS3, S_OK },
    { __LINE__, 0, EAT_CHECK_EXIST },
    { __LINE__, 1, EAT_CHECK_EXIST },
    { __LINE__, 2, EAT_CHECK_EXIST },

    { __LINE__, 0, EAT_CREATE },
    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 2, EAT_CREATE },
    { __LINE__, 0, EAT_CALL, FLAGS4, S_OK },
    { __LINE__, 0, EAT_CHECK_EXIST },
    { __LINE__, 1, EAT_CHECK_EXIST },
    { __LINE__, 2, EAT_CHECK_EXIST },

    { __LINE__, 0, EAT_CREATE },
    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 2, EAT_CREATE },
    { __LINE__, 0, EAT_CALL, FLAGS5, E_FAIL },
    { __LINE__, 0, EAT_CHECK_EXIST },
    { __LINE__, 1, EAT_CHECK_EXIST },
    { __LINE__, 2, EAT_CHECK_EXIST },

    { __LINE__, 0, EAT_CREATE },
    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 2, EAT_CREATE },
    { __LINE__, 0, EAT_CALL, FLAGS6, E_FAIL },
    { __LINE__, 0, EAT_CHECK_EXIST },
    { __LINE__, 1, EAT_CHECK_EXIST },
    { __LINE__, 2, EAT_CHECK_EXIST },

    { __LINE__, 0, EAT_CREATE },
    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 2, EAT_CREATE },
    { __LINE__, 0, EAT_CALL, FLAGS7, E_FAIL },
    { __LINE__, 0, EAT_CHECK_EXIST },
    { __LINE__, 1, EAT_CHECK_EXIST },
    { __LINE__, 2, EAT_CHECK_EXIST },

    { __LINE__, 0, EAT_CREATE },
    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 2, EAT_CREATE },
    { __LINE__, 0, EAT_CALL, FLAGS8, E_FAIL },
    { __LINE__, 0, EAT_CHECK_EXIST },
    { __LINE__, 1, EAT_CHECK_EXIST },
    { __LINE__, 2, EAT_CHECK_EXIST },

    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 2, EAT_CREATE },
    { __LINE__, 1, EAT_CALL, FLAGS0, S_OK },
    { __LINE__, 1, EAT_CHECK_NON_EXIST },

    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 2, EAT_CREATE },
    { __LINE__, 1, EAT_CALL, FLAGS1, E_FAIL },
    { __LINE__, 1, EAT_CHECK_EXIST },
    { __LINE__, 2, EAT_CHECK_EXIST },

    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 2, EAT_CREATE },
    { __LINE__, 1, EAT_CALL, FLAGS2, S_OK },
    { __LINE__, 1, EAT_CHECK_EXIST },
    { __LINE__, 2, EAT_CHECK_NON_EXIST },

    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 2, EAT_CREATE },
    { __LINE__, 1, EAT_CALL, FLAGS3, S_OK },
    { __LINE__, 1, EAT_CHECK_NON_EXIST },

    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 2, EAT_CREATE },
    { __LINE__, 1, EAT_CALL, FLAGS4, S_OK },
    { __LINE__, 1, EAT_CHECK_EXIST },
    { __LINE__, 2, EAT_CHECK_NON_EXIST },

    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 2, EAT_CREATE },
    { __LINE__, 1, EAT_CALL, FLAGS5, E_FAIL },
    { __LINE__, 1, EAT_CHECK_EXIST },
    { __LINE__, 2, EAT_CHECK_EXIST },

    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 2, EAT_CREATE },
    { __LINE__, 1, EAT_CALL, FLAGS6, E_FAIL },
    { __LINE__, 1, EAT_CHECK_EXIST },
    { __LINE__, 2, EAT_CHECK_EXIST },

    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 2, EAT_CREATE },
    { __LINE__, 1, EAT_CALL, FLAGS7, E_FAIL },
    { __LINE__, 1, EAT_CHECK_EXIST },
    { __LINE__, 2, EAT_CHECK_EXIST },

    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 2, EAT_CREATE },
    { __LINE__, 1, EAT_CALL, FLAGS8, E_FAIL },
    { __LINE__, 1, EAT_CHECK_EXIST },
    { __LINE__, 2, EAT_CHECK_EXIST },

    { __LINE__, 2, EAT_DELETE },

    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 1, EAT_CALL, FLAGS0, S_OK },
    { __LINE__, 1, EAT_CHECK_NON_EXIST },

    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 1, EAT_CALL, FLAGS1, S_OK },
    { __LINE__, 1, EAT_CHECK_NON_EXIST },

    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 1, EAT_CALL, FLAGS2, S_OK },
    { __LINE__, 1, EAT_CHECK_EXIST },

    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 1, EAT_CALL, FLAGS3, S_OK },
    { __LINE__, 1, EAT_CHECK_NON_EXIST },

    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 1, EAT_CALL, FLAGS4, S_OK },
    { __LINE__, 1, EAT_CHECK_EXIST },

    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 1, EAT_CALL, FLAGS5, S_OK },
    { __LINE__, 1, EAT_CHECK_NON_EXIST },

    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 1, EAT_CALL, FLAGS6, S_OK },
    { __LINE__, 1, EAT_CHECK_NON_EXIST },

    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 1, EAT_CALL, FLAGS7, S_OK },
    { __LINE__, 1, EAT_CHECK_NON_EXIST },

    { __LINE__, 1, EAT_CREATE },
    { __LINE__, 1, EAT_CALL, FLAGS8, S_OK },
    { __LINE__, 1, EAT_CHECK_NON_EXIST },

    { __LINE__, 1, EAT_CREATE },

    { __LINE__, 2, EAT_CREATE },
    { __LINE__, 2, EAT_CALL, FLAGS0, S_OK },
    { __LINE__, 2, EAT_CHECK_NON_EXIST },

    { __LINE__, 2, EAT_CREATE },
    { __LINE__, 2, EAT_CALL, FLAGS1, S_OK },
    { __LINE__, 2, EAT_CHECK_NON_EXIST },

    { __LINE__, 2, EAT_CREATE },
    { __LINE__, 2, EAT_CALL, FLAGS2, S_OK },
    { __LINE__, 2, EAT_CHECK_NON_EXIST },

    { __LINE__, 2, EAT_CREATE },
    { __LINE__, 2, EAT_CALL, FLAGS3, S_OK },
    { __LINE__, 2, EAT_CHECK_NON_EXIST },

    { __LINE__, 2, EAT_CREATE },
    { __LINE__, 2, EAT_CALL, FLAGS4, S_OK },
    { __LINE__, 2, EAT_CHECK_NON_EXIST },

    { __LINE__, 2, EAT_CREATE },
    { __LINE__, 2, EAT_CALL, FLAGS5, S_OK },
    { __LINE__, 2, EAT_CHECK_NON_EXIST },

    { __LINE__, 2, EAT_CREATE },
    { __LINE__, 2, EAT_CALL, FLAGS6, S_OK },
    { __LINE__, 2, EAT_CHECK_NON_EXIST },

    { __LINE__, 2, EAT_CREATE },
    { __LINE__, 2, EAT_CALL, FLAGS7, S_OK },
    { __LINE__, 2, EAT_CHECK_NON_EXIST },

    { __LINE__, 2, EAT_CREATE },
    { __LINE__, 2, EAT_CALL, FLAGS8, S_OK },
    { __LINE__, 2, EAT_CHECK_NON_EXIST },

    { __LINE__, 2, EAT_DELETE },
    { __LINE__, 1, EAT_DELETE },
    { __LINE__, 0, EAT_DELETE },

    { __LINE__, 2, EAT_CALL, FLAGS0, E_FAIL },
    { __LINE__, 2, EAT_CALL, FLAGS1, E_FAIL },
    { __LINE__, 2, EAT_CALL, FLAGS2, E_FAIL },
    { __LINE__, 2, EAT_CALL, FLAGS3, E_FAIL },
    { __LINE__, 2, EAT_CALL, FLAGS4, E_FAIL },
    { __LINE__, 2, EAT_CALL, FLAGS5, E_FAIL },
    { __LINE__, 2, EAT_CALL, FLAGS6, E_FAIL },
    { __LINE__, 2, EAT_CALL, FLAGS7, E_FAIL },
    { __LINE__, 2, EAT_CALL, FLAGS8, E_FAIL },

    { __LINE__, 1, EAT_CALL, FLAGS0, E_FAIL },
    { __LINE__, 1, EAT_CALL, FLAGS1, E_FAIL },
    { __LINE__, 1, EAT_CALL, FLAGS2, E_FAIL },
    { __LINE__, 1, EAT_CALL, FLAGS3, E_FAIL },
    { __LINE__, 1, EAT_CALL, FLAGS4, E_FAIL },
    { __LINE__, 1, EAT_CALL, FLAGS5, E_FAIL },
    { __LINE__, 1, EAT_CALL, FLAGS6, E_FAIL },
    { __LINE__, 1, EAT_CALL, FLAGS7, E_FAIL },
    { __LINE__, 1, EAT_CALL, FLAGS8, E_FAIL },
};

static char  s_cur_dir_A[MAX_PATH] = "";
static WCHAR s_cur_dir_W[MAX_PATH] = L"";

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
        INT lineno = entry->lineno;
        const NODEA *node = &s_nodesA[entry->node];
        char *path = GetPathA(node->item);
        INT ret;

        switch (entry->action_type)
        {
            case EAT_CREATE:
                if (node->is_dir)
                {
                    CreateDirectoryA(path, NULL);

                    attr = GetFileAttributesA(path);
                    ok(attr != INVALID_FILE_ATTRIBUTES, "Line %d: path:%s, attr:0x%08lX\n", lineno, path, attr);
                    ok((attr & FILE_ATTRIBUTE_DIRECTORY), "Line %d: path:%s, attr:0x%08lX\n", lineno, path, attr);
                }
                else
                {
                    fclose(fopen(path, "w"));

                    attr = GetFileAttributesA(path);
                    ok(attr != INVALID_FILE_ATTRIBUTES, "Line %d: attr was 0x%08lX\n", lineno, attr);
                    ok(!(attr & FILE_ATTRIBUTE_DIRECTORY), "Line %d: attr was 0x%08lX\n", lineno, attr);
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
                ok(attr == INVALID_FILE_ATTRIBUTES, "Line %d: cannot delete\n", lineno);
                break;
            case EAT_CHECK_EXIST:
                attr = GetFileAttributesA(path);
                ok(attr != INVALID_FILE_ATTRIBUTES, "Line %d: attr was 0x%08lX\n", lineno, attr);
                break;
            case EAT_CHECK_NON_EXIST:
                attr = GetFileAttributesA(path);
                ok(attr == INVALID_FILE_ATTRIBUTES, "Line %d: attr was 0x%08lX\n", lineno, attr);
                break;
            case EAT_CALL:
                ret = (*s_pDelNodeA)(path, entry->flags);
                ok(ret == entry->ret, "Line %d: ret:%d, path:%s, flags:0x%08lX\n",
                   lineno, ret, path, entry->flags);
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
        INT lineno = entry->lineno;
        const NODEW *node = &s_nodesW[entry->node];
        WCHAR *path = GetPathW(node->item);
        INT ret;

        switch (entry->action_type)
        {
            case EAT_CREATE:
                if (node->is_dir)
                {
                    CreateDirectoryW(path, NULL);

                    attr = GetFileAttributesW(path);
                    ok(attr != INVALID_FILE_ATTRIBUTES, "Line %d: path:%S, attr:0x%08lX\n", lineno, path, attr);
                    ok((attr & FILE_ATTRIBUTE_DIRECTORY), "Line %d: path:%S, attr:0x%08lX\n", lineno, path, attr);
                }
                else
                {
                    fclose(_wfopen(path, L"w"));

                    attr = GetFileAttributesW(path);
                    ok(attr != INVALID_FILE_ATTRIBUTES, "Line %d: attr was 0x%08lX\n", lineno, attr);
                    ok(!(attr & FILE_ATTRIBUTE_DIRECTORY), "Line %d: attr was 0x%08lX\n", lineno, attr);
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
                ok(attr == INVALID_FILE_ATTRIBUTES, "Line %d: cannot delete\n", lineno);
                break;
            case EAT_CHECK_EXIST:
                attr = GetFileAttributesW(path);
                ok(attr != INVALID_FILE_ATTRIBUTES, "Line %d: attr was 0x%08lX\n", lineno, attr);
                break;
            case EAT_CHECK_NON_EXIST:
                attr = GetFileAttributesW(path);
                ok(attr == INVALID_FILE_ATTRIBUTES, "Line %d: attr was 0x%08lX\n", lineno, attr);
                break;
            case EAT_CALL:
                ret = (*s_pDelNodeW)(path, entry->flags);
                ok(ret == entry->ret, "Line %d: ret:%d, path:%S, flags:0x%08lX\n",
                   lineno, ret, path, entry->flags);
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
