#ifndef APPHELP_APITEST_H
#define APPHELP_APITEST_H

#ifdef __cplusplus
extern "C" {
#endif


/* data.c */
void test_create_db_imp(const WCHAR* name, int win10);
DWORD test_get_db_size();
void test_create_exe_imp(const WCHAR* name, int skip_rsrc_exports);
void test_create_file_imp(const WCHAR* name, const char* contents, size_t len);
void test_create_ne_imp(const WCHAR* name, int skip_names);
DWORD get_host_winver(void);
DWORD get_module_version(HMODULE mod);
void silence_debug_output(void);        // Silence output if the environment variable is not set.

#define test_create_db      (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : test_create_db_imp
#define test_create_exe     (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : test_create_exe_imp
#define test_create_file    (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : test_create_file_imp
#define test_create_ne      (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : test_create_ne_imp


static DWORD g_WinVersion;

#define WINVER_ANY     0
#define WINVER_WINXP   0x0501
#define WINVER_2003    0x0502
#define WINVER_VISTA   0x0600
#define WINVER_WIN7    0x0601
#define WINVER_WIN8    0x0602
#define WINVER_WIN10   0x0a00


typedef WORD TAG;
typedef DWORD TAGID;
typedef DWORD TAGREF;
typedef UINT64 QWORD;
typedef VOID* PDB;
typedef VOID* HSDB;
typedef INT PATH_TYPE;



#define SDB_MAX_SDBS 16
#define SDB_MAX_EXES_VISTA 16
#define SDB_MAX_LAYERS 8
#define SHIMREG_DISABLE_LAYER (0x00000020)

#define SDBQUERYRESULT_EXPECTED_SIZE_VISTA    456



typedef struct tagSDBQUERYRESULT_VISTA
{
    TAGREF atrExes[SDB_MAX_EXES_VISTA];
    DWORD  adwExeFlags[SDB_MAX_EXES_VISTA];
    TAGREF atrLayers[SDB_MAX_LAYERS];
    DWORD  dwLayerFlags;
    TAGREF trApphelp;
    DWORD  dwExeCount;
    DWORD  dwLayerCount;
    GUID   guidID;
    DWORD  dwFlags;
    DWORD  dwCustomSDBMap;
    GUID   rgGuidDB[SDB_MAX_SDBS];
} SDBQUERYRESULT_VISTA, *PSDBQUERYRESULT_VISTA;


#define SDBQUERYRESULT_EXPECTED_SIZE_2k3    344

#define SDB_MAX_EXES_2k3    4

typedef struct tagSDBQUERYRESULT_2k3
{
    TAGREF atrExes[SDB_MAX_EXES_2k3];
    TAGREF atrLayers[SDB_MAX_LAYERS];
    DWORD  dwLayerFlags;
    TAGREF trApphelp;       // probably?
    DWORD  dwExeCount;
    DWORD  dwLayerCount;
    GUID   guidID;          // probably?
    DWORD  dwFlags;         // probably?
    DWORD  dwCustomSDBMap;
    GUID   rgGuidDB[SDB_MAX_SDBS];
} SDBQUERYRESULT_2k3, *PSDBQUERYRESULT_2k3;





C_ASSERT(sizeof(SDBQUERYRESULT_VISTA) == SDBQUERYRESULT_EXPECTED_SIZE_VISTA);
C_ASSERT(sizeof(SDBQUERYRESULT_2k3) == SDBQUERYRESULT_EXPECTED_SIZE_2k3);




#ifdef __cplusplus
} // extern "C"
#endif

#endif // APPHELP_APITEST_H
