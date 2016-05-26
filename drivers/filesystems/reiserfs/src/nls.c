/*
 * COPYRIGHT:        GNU GENERAL PUBLIC LICENSE VERSION 2
 * PROJECT:          ReiserFs file system driver for Windows NT/2000/XP/Vista.
 * FILE:             nls.c
 * PURPOSE:          
 * PROGRAMMER:       Mark Piper, Matt Wu, Bo Brantén.
 * HOMEPAGE:         
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include "rfsd.h"

/* GOBALS** ****************************************************************/

extern struct nls_table *tables;
extern spinlock_t nls_lock;

/* DECLARES ****************************************************************/

#undef FULL_CODEPAGES_SUPPORT

#ifdef FULL_CODEPAGES_SUPPORT

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
DECLARE_INIT(init_nls_cp950);
DECLARE_EXIT(exit_nls_cp950);
DECLARE_INIT(init_nls_euc_jp);
DECLARE_EXIT(exit_nls_euc_jp);
DECLARE_INIT(init_nls_euc_kr);
DECLARE_EXIT(exit_nls_euc_kr);
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
DECLARE_INIT(init_nls_iso8859_8);
DECLARE_EXIT(exit_nls_iso8859_8);
DECLARE_INIT(init_nls_iso8859_9);
DECLARE_EXIT(exit_nls_iso8859_9);
DECLARE_INIT(init_nls_koi8_r);
DECLARE_EXIT(exit_nls_koi8_r);
DECLARE_INIT(init_nls_koi8_ru);
DECLARE_EXIT(exit_nls_koi8_ru);
DECLARE_INIT(init_nls_koi8_u);
DECLARE_EXIT(exit_nls_koi8_u);
DECLARE_INIT(init_nls_sjis);
DECLARE_EXIT(exit_nls_sjis);
DECLARE_INIT(init_nls_tis_620);
DECLARE_EXIT(exit_nls_tis_620);
DECLARE_INIT(init_nls_big5);
DECLARE_EXIT(exit_nls_big5);
DECLARE_INIT(init_nls_cp936);
DECLARE_EXIT(exit_nls_cp936);
DECLARE_INIT(init_nls_gb2312);
DECLARE_EXIT(exit_nls_gb2312);

#endif //FULL_CODEPAGES_SUPPORT

DECLARE_INIT(init_nls_utf8);
DECLARE_EXIT(exit_nls_utf8);


/* FUNCTIONS ****************************************************************/


int
RfsdLoadAllNls()
{
    int rc;

    tables = NULL;
    spin_lock_init(&nls_lock);

    //
    // Loading nls for utf8 ...
    //

    LOAD_NLS(init_nls_utf8);

#ifdef FULL_CODEPAGES_SUPPORT

    //
    // Loading nls for chinese gb2312 ...
    //

    LOAD_NLS(init_nls_cp936);
    LOAD_NLS(init_nls_gb2312);


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

    //
    // Loading nls for chinese big5
    //

    LOAD_NLS(init_nls_cp950);
    LOAD_NLS(init_nls_big5);

    //
    // Loading jp codepage
    //

    LOAD_NLS(init_nls_cp932);
    LOAD_NLS(init_nls_euc_jp);
    LOAD_NLS(init_nls_sjis);

    //
    // Loading korean nls codepage
    //

    LOAD_NLS(init_nls_cp949);
    LOAD_NLS(init_nls_euc_kr);

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
    LOAD_NLS(init_nls_iso8859_8);
    LOAD_NLS(init_nls_iso8859_9);
    LOAD_NLS(init_nls_koi8_r);
    LOAD_NLS(init_nls_koi8_u);

    LOAD_NLS(init_nls_koi8_ru);
    LOAD_NLS(init_nls_tis_620);

#endif //FULL_CODEPAGES_SUPPORT


    KdPrint(("RfsdLoadAllNls: succeed to load all codepages ...\n"));

errorout:

    return rc;
}


VOID
RfsdUnloadAllNls()
{
#ifdef FULL_CODEPAGES_SUPPORT

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
    UNLOAD_NLS(exit_nls_cp949);
    UNLOAD_NLS(exit_nls_cp950);
    UNLOAD_NLS(exit_nls_cp932);
    UNLOAD_NLS(exit_nls_euc_jp);
    UNLOAD_NLS(exit_nls_big5);
    UNLOAD_NLS(exit_nls_euc_kr);
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
    UNLOAD_NLS(exit_nls_iso8859_8);
    UNLOAD_NLS(exit_nls_iso8859_9);
    UNLOAD_NLS(exit_nls_koi8_r);
    UNLOAD_NLS(exit_nls_koi8_ru);
    UNLOAD_NLS(exit_nls_koi8_u);
    UNLOAD_NLS(exit_nls_sjis);
    UNLOAD_NLS(exit_nls_tis_620);

    //
    // Unloading nls of chinese ...
    //

    UNLOAD_NLS(exit_nls_gb2312);
    UNLOAD_NLS(exit_nls_cp936);

#endif //FULL_CODEPAGES_SUPPORT

    //
    // Unloading nls of utf8 ...
    //

    UNLOAD_NLS(exit_nls_utf8);
}