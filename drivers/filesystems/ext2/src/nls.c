/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             nls.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://www.ext2fsd.com
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"

/* GOBALS** ****************************************************************/

extern struct nls_table *tables;
extern spinlock_t nls_lock;

/* DECLARES ****************************************************************/

#define FULL_CODEPAGES_SUPPORT

#ifdef FULL_CODEPAGES_SUPPORT

DECLARE_INIT(init_nls_ascii);
DECLARE_EXIT(exit_nls_ascii);
DECLARE_INIT(init_nls_cp1250);
DECLARE_EXIT(exit_nls_cp1250);
DECLARE_INIT(init_nls_cp1251);
DECLARE_EXIT(exit_nls_cp1251);
DECLARE_INIT(init_nls_cp1255);
DECLARE_EXIT(exit_nls_cp1255);
DECLARE_INIT(init_nls_cp437);
DECLARE_EXIT(exit_nls_cp437);
DECLARE_INIT(init_nls_cp737);
DECLARE_EXIT(exit_nls_cp737);
DECLARE_INIT(init_nls_cp775);
DECLARE_EXIT(exit_nls_cp775);
DECLARE_INIT(init_nls_cp850);
DECLARE_EXIT(exit_nls_cp850);
DECLARE_INIT(init_nls_cp852);
DECLARE_EXIT(exit_nls_cp852);
DECLARE_INIT(init_nls_cp855);
DECLARE_EXIT(exit_nls_cp855);
DECLARE_INIT(init_nls_cp857);
DECLARE_EXIT(exit_nls_cp857);
DECLARE_INIT(init_nls_cp860);
DECLARE_EXIT(exit_nls_cp860);
DECLARE_INIT(init_nls_cp861);
DECLARE_EXIT(exit_nls_cp861);
DECLARE_INIT(init_nls_cp862);
DECLARE_EXIT(exit_nls_cp862);
DECLARE_INIT(init_nls_cp863);
DECLARE_EXIT(exit_nls_cp863);
DECLARE_INIT(init_nls_cp864);
DECLARE_EXIT(exit_nls_cp864);
DECLARE_INIT(init_nls_cp865);
DECLARE_EXIT(exit_nls_cp865);
DECLARE_INIT(init_nls_cp866);
DECLARE_EXIT(exit_nls_cp866);
DECLARE_INIT(init_nls_cp869);
DECLARE_EXIT(exit_nls_cp869);
DECLARE_INIT(init_nls_cp874);
DECLARE_EXIT(exit_nls_cp874);
DECLARE_INIT(init_nls_cp932);
DECLARE_EXIT(exit_nls_cp932);
DECLARE_INIT(init_nls_cp949);
DECLARE_EXIT(exit_nls_cp949);
DECLARE_INIT(init_nls_euc_jp);
DECLARE_EXIT(exit_nls_euc_jp);
DECLARE_INIT(init_nls_iso8859_1);
DECLARE_EXIT(exit_nls_iso8859_1);
DECLARE_INIT(init_nls_iso8859_13);
DECLARE_EXIT(exit_nls_iso8859_13);
DECLARE_INIT(init_nls_iso8859_14);
DECLARE_EXIT(exit_nls_iso8859_14);
DECLARE_INIT(init_nls_iso8859_15);
DECLARE_EXIT(exit_nls_iso8859_15);
DECLARE_INIT(init_nls_iso8859_2);
DECLARE_EXIT(exit_nls_iso8859_2);
DECLARE_INIT(init_nls_iso8859_3);
DECLARE_EXIT(exit_nls_iso8859_3);
DECLARE_INIT(init_nls_iso8859_4);
DECLARE_EXIT(exit_nls_iso8859_4);
DECLARE_INIT(init_nls_iso8859_5);
DECLARE_EXIT(exit_nls_iso8859_5);
DECLARE_INIT(init_nls_iso8859_6);
DECLARE_EXIT(exit_nls_iso8859_6);
DECLARE_INIT(init_nls_iso8859_7);
DECLARE_EXIT(exit_nls_iso8859_7);
DECLARE_INIT(init_nls_iso8859_9);
DECLARE_EXIT(exit_nls_iso8859_9);
DECLARE_INIT(init_nls_koi8_r);
DECLARE_EXIT(exit_nls_koi8_r);
DECLARE_INIT(init_nls_koi8_ru);
DECLARE_EXIT(exit_nls_koi8_ru);
DECLARE_INIT(init_nls_koi8_u);
DECLARE_EXIT(exit_nls_koi8_u);
#endif //FULL_CODEPAGES_SUPPORT

/* gb2312 */
DECLARE_INIT(init_nls_cp936);
DECLARE_EXIT(exit_nls_cp936);

/* big5 */
DECLARE_INIT(init_nls_cp950);
DECLARE_EXIT(exit_nls_cp950);

/* utf8 */
DECLARE_INIT(init_nls_utf8);
DECLARE_EXIT(exit_nls_utf8);


/* FUNCTIONS ****************************************************************/


int
Ext2LoadAllNls()
{
    int rc;

    tables = NULL;
    spin_lock_init(&nls_lock);

    /* loading utf8 ... */
    LOAD_NLS(init_nls_utf8);

#ifdef FULL_CODEPAGES_SUPPORT

    /* loading chinese gb2312 and big5... */
    LOAD_NLS(init_nls_cp936);
    LOAD_NLS(init_nls_cp950);

    /* loading all others */

    LOAD_NLS(init_nls_ascii);
    LOAD_NLS(init_nls_cp1250);
    LOAD_NLS(init_nls_cp1251);
    LOAD_NLS(init_nls_cp1255);
    LOAD_NLS(init_nls_cp437);
    LOAD_NLS(init_nls_cp737);
    LOAD_NLS(init_nls_cp775);
    LOAD_NLS(init_nls_cp850);
    LOAD_NLS(init_nls_cp852);
    LOAD_NLS(init_nls_cp855);
    LOAD_NLS(init_nls_cp857);
    LOAD_NLS(init_nls_cp860);
    LOAD_NLS(init_nls_cp861);
    LOAD_NLS(init_nls_cp862);
    LOAD_NLS(init_nls_cp863);
    LOAD_NLS(init_nls_cp864);
    LOAD_NLS(init_nls_cp865);
    LOAD_NLS(init_nls_cp866);
    LOAD_NLS(init_nls_cp869);
    LOAD_NLS(init_nls_cp874);
    LOAD_NLS(init_nls_cp932);
    LOAD_NLS(init_nls_euc_jp);
    LOAD_NLS(init_nls_cp949);
    LOAD_NLS(init_nls_iso8859_1);
    LOAD_NLS(init_nls_iso8859_13);
    LOAD_NLS(init_nls_iso8859_14);
    LOAD_NLS(init_nls_iso8859_15);
    LOAD_NLS(init_nls_iso8859_2);
    LOAD_NLS(init_nls_iso8859_3);
    LOAD_NLS(init_nls_iso8859_4);
    LOAD_NLS(init_nls_iso8859_5);
    LOAD_NLS(init_nls_iso8859_6);
    LOAD_NLS(init_nls_iso8859_7);
    LOAD_NLS(init_nls_iso8859_9);
    LOAD_NLS(init_nls_koi8_r);
    LOAD_NLS(init_nls_koi8_u);
    LOAD_NLS(init_nls_koi8_ru);

#endif //FULL_CODEPAGES_SUPPORT

    return rc;
}


VOID
Ext2UnloadAllNls()
{

#ifdef FULL_CODEPAGES_SUPPORT

    UNLOAD_NLS(init_nls_ascii);
    UNLOAD_NLS(init_nls_cp1250);
    UNLOAD_NLS(exit_nls_cp1251);
    UNLOAD_NLS(exit_nls_cp1255);
    UNLOAD_NLS(exit_nls_cp437);
    UNLOAD_NLS(exit_nls_cp737);
    UNLOAD_NLS(exit_nls_cp775);
    UNLOAD_NLS(exit_nls_cp850);
    UNLOAD_NLS(exit_nls_cp852);
    UNLOAD_NLS(exit_nls_cp855);
    UNLOAD_NLS(exit_nls_cp857);
    UNLOAD_NLS(exit_nls_cp860);
    UNLOAD_NLS(exit_nls_cp861);
    UNLOAD_NLS(exit_nls_cp862);
    UNLOAD_NLS(exit_nls_cp863);
    UNLOAD_NLS(exit_nls_cp864);
    UNLOAD_NLS(exit_nls_cp865);
    UNLOAD_NLS(exit_nls_cp866);
    UNLOAD_NLS(exit_nls_cp869);
    UNLOAD_NLS(exit_nls_cp874);
    UNLOAD_NLS(exit_nls_euc_jp);
    UNLOAD_NLS(exit_nls_cp932);
    UNLOAD_NLS(exit_nls_cp949);
    UNLOAD_NLS(exit_nls_iso8859_1);
    UNLOAD_NLS(exit_nls_iso8859_13);
    UNLOAD_NLS(exit_nls_iso8859_14);
    UNLOAD_NLS(exit_nls_iso8859_15);
    UNLOAD_NLS(exit_nls_iso8859_2);
    UNLOAD_NLS(exit_nls_iso8859_3);
    UNLOAD_NLS(exit_nls_iso8859_4);
    UNLOAD_NLS(exit_nls_iso8859_5);
    UNLOAD_NLS(exit_nls_iso8859_6);
    UNLOAD_NLS(exit_nls_iso8859_7);
    UNLOAD_NLS(exit_nls_iso8859_9);
    UNLOAD_NLS(exit_nls_koi8_ru);
    UNLOAD_NLS(exit_nls_koi8_r);
    UNLOAD_NLS(exit_nls_koi8_u);

    /* unloading chinese codepages */
    UNLOAD_NLS(exit_nls_cp950);
    UNLOAD_NLS(exit_nls_cp936);

#endif //FULL_CODEPAGES_SUPPORT

    /* unloading nls of utf8  */
    UNLOAD_NLS(exit_nls_utf8);
}