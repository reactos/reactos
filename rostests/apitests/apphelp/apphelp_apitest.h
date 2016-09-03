#ifndef APPHELP_APITEST_H
#define APPHELP_APITEST_H


/* data.c */
void test_create_db_imp(const char* name);
DWORD test_get_db_size();
void test_create_exe_imp(const char* name, int skip_rsrc_exports);
void test_create_file_imp(const char* name, const char* contents, size_t len);
void test_create_ne_imp(const char* name, int skip_names);
DWORD get_host_winver(void);
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


#endif // APPHELP_APITEST_H
