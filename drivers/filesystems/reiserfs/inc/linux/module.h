/*
 * COPYRIGHT:        GNU GENERAL PUBLIC LICENSE VERSION 2
 * PROJECT:          ReiserFs file system driver for Windows NT/2000/XP/Vista.
 * FILE:             module.h
 * PURPOSE:          Header file: nls structures & linux kernel ...
 * PROGRAMMER:       Mark Piper, Matt Wu, Bo Brantén.
 * HOMEPAGE:         
 * UPDATE HISTORY: 
 */

#ifndef _RFSD_MODULE_HEADER_
#define _RFSD_MODULE_HEADER_

/* INCLUDES *************************************************************/

#include "ntifs.h"
#include "linux/types.h"

/* STRUCTS & S******************************************************/

//
// Linux modules
//

#define __init
#define __exit

#define try_inc_mod_count(x)  TRUE
#define __MOD_DEC_USE_COUNT(x) do {} while(FALSE);
#define EXPORT_SYMBOL(x)
#define THIS_MODULE NULL

#define module_init(X) int  __init module_##X() {return X();}
#define module_exit(X) void __exit module_##X() {X();}

#define MODULE_LICENSE(x)

#define inline __inline

#define DECLARE_INIT(X) extern int  __init  module_##X(void)
#define DECLARE_EXIT(X) extern void __exit  module_##X(void)

#define LOAD_NLS(X) do { rc = module_##X();     \
                         if (rc != 0) {         \
                                goto errorout;  \
                         }                      \
                    } while(0)


#define UNLOAD_NLS(X) do { module_##X(); } while(0)

//
// Spin locks .....
//

typedef struct _spinlock_t {

    KSPIN_LOCK SpinLock;
    KIRQL      Irql;
} spinlock_t;

#define spin_lock_init(lock)    KeInitializeSpinLock(&((lock)->SpinLock))
#define spin_lock(lock)         KeAcquireSpinLock(&((lock)->SpinLock), &((lock)->Irql))
#define spin_unlock(lock)       KeReleaseSpinLock(&((lock)->SpinLock), (lock)->Irql)


//
// unicode character
//

struct nls_table {
        char *charset;
        int (*uni2char) (wchar_t uni, unsigned char *out, int boundlen);
        int (*char2uni) (const  unsigned char *rawstring, int boundlen,
                         wchar_t *uni);
        unsigned char *charset2lower;
        unsigned char *charset2upper;
        struct module *owner;
        struct nls_table *next;
};

/* this value hold the maximum octet of charset */
#define NLS_MAX_CHARSET_SIZE 6 /* for UTF-8 */

/* nls.c */
extern int register_nls(struct nls_table *);
extern int unregister_nls(struct nls_table *);
extern struct nls_table *load_nls(char *);
extern void unload_nls(struct nls_table *);
extern struct nls_table *load_nls_default(void);

extern int utf8_mbtowc(wchar_t *, const __u8 *, int);
extern int utf8_mbstowcs(wchar_t *, const __u8 *, int);
extern int utf8_wctomb(__u8 *, wchar_t, int);
extern int utf8_wcstombs(__u8 *, const wchar_t *, int);

/* FUNCTIONS DECLARATION *****************************************************/

#endif // _RFSD_MODULE_HEADER_
