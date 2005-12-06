/* $Id$
*/

/*
 * REACTOS ELF LOADER
 *
 * ELF run-time linker, ported from FreeBSD by KJK::Hyperion as part of the ELF
 * support initiative. Original copyright, licensing and disclaimers follow
 */

/*-
 * Copyright 1996, 1997, 1998, 1999, 2000 John D. Polstra.
 * Copyright 2003 Alexander Kabaev <kan@FreeBSD.ORG>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD: src/libexec/rtld-elf/rtld.c,v 1.101 2004/11/02 09:42:21 ssouhlal Exp $
 */

/*
 * Dynamic linker for ELF.
 *
 * John Polstra <jdp@polstra.com>.
 */

#include <ntdll.h>
#define NDEBUG
#include <debug.h>

#if 0

#ifndef __GNUC__
#error "GCC is needed to compile this file"
#endif

#include <sys/param.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <dlfcn.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "debug.h"
#include "rtld.h"
#include "libmap.h"
#include "rtld_tls.h"

#ifndef COMPAT_32BIT
#define PATH_RTLD	"/libexec/ld-elf.so.1"
#else
#define PATH_RTLD	"/libexec/ld-elf32.so.1"
#endif

/* Types. */
typedef void (*func_ptr_type)();
typedef void * (*path_enum_proc) (const char *path, size_t len, void *arg);

/*
 * This structure provides a reentrant way to keep a list of objects and
 * check which ones have already been processed in some way.
 */
typedef struct Struct_DoneList {
    const Obj_Entry **objs;		/* Array of object pointers */
    unsigned int num_alloc;		/* Allocated size of the array */
    unsigned int num_used;		/* Number of array slots used */
} DoneList;

/*
 * Function declarations.
 */
static const char *basename(const char *);
static void die(void);
static void digest_dynamic(Obj_Entry *, int);
static Obj_Entry *digest_phdr(const Elf_Phdr *, int, caddr_t, const char *);
static Obj_Entry *dlcheck(void *);
static int do_search_info(const Obj_Entry *obj, int, struct dl_serinfo *);
static bool donelist_check(DoneList *, const Obj_Entry *);
static void errmsg_restore(char *);
static char *errmsg_save(void);
static void *fill_search_info(const char *, size_t, void *);
static char *find_library(const char *, const Obj_Entry *);
static const char *gethints(void);
static void init_dag(Obj_Entry *);
static void init_dag1(Obj_Entry *root, Obj_Entry *obj, DoneList *);
static void init_rtld(caddr_t);
static void initlist_add_neededs(Needed_Entry *needed, Objlist *list);
static void initlist_add_objects(Obj_Entry *obj, Obj_Entry **tail,
  Objlist *list);
static bool is_exported(const Elf_Sym *);
static void linkmap_add(Obj_Entry *);
static void linkmap_delete(Obj_Entry *);
static int load_needed_objects(Obj_Entry *);
static int load_preload_objects(void);
static Obj_Entry *load_object(char *);
static Obj_Entry *obj_from_addr(const void *);
static void objlist_call_fini(Objlist *);
static void objlist_call_init(Objlist *);
static void objlist_clear(Objlist *);
static Objlist_Entry *objlist_find(Objlist *, const Obj_Entry *);
static void objlist_init(Objlist *);
static void objlist_push_head(Objlist *, Obj_Entry *);
static void objlist_push_tail(Objlist *, Obj_Entry *);
static void objlist_remove(Objlist *, Obj_Entry *);
static void objlist_remove_unref(Objlist *);
static void *path_enumerate(const char *, path_enum_proc, void *);
static int relocate_objects(Obj_Entry *, bool, Obj_Entry *);
static int rtld_dirname(const char *, char *);
static void rtld_exit(void);
static char *search_library_path(const char *, const char *);
static const void **get_program_var_addr(const char *name);
static void set_program_var(const char *, const void *);
static const Elf_Sym *symlook_default(const char *, unsigned long hash,
  const Obj_Entry *refobj, const Obj_Entry **defobj_out, bool in_plt);
static const Elf_Sym *symlook_list(const char *, unsigned long,
  Objlist *, const Obj_Entry **, bool in_plt, DoneList *);
static void trace_loaded_objects(Obj_Entry *obj);
static void unlink_object(Obj_Entry *);
static void unload_object(Obj_Entry *);
static void unref_dag(Obj_Entry *);
static void ref_dag(Obj_Entry *);

void r_debug_state(struct r_debug*, struct link_map*);

/*
 * Data declarations.
 */
static char *error_message;	/* Message for dlerror(), or NULL */
struct r_debug r_debug;		/* for GDB; */
static bool libmap_disable;	/* Disable libmap */
static bool trust;		/* False for setuid and setgid programs */
static char *ld_bind_now;	/* Environment variable for immediate binding */
static char *ld_debug;		/* Environment variable for debugging */
static char *ld_library_path;	/* Environment variable for search path */
static char *ld_preload;	/* Environment variable for libraries to
				   load first */
static char *ld_tracing;	/* Called from ldd to print libs */
static Obj_Entry *obj_list;	/* Head of linked list of shared objects */
static Obj_Entry **obj_tail;	/* Link field of last object in list */
static Obj_Entry *obj_main;	/* The main program shared object */
static Obj_Entry obj_rtld;	/* The dynamic linker shared object */
static unsigned int obj_count;	/* Number of objects in obj_list */

static Objlist list_global =	/* Objects dlopened with RTLD_GLOBAL */
  STAILQ_HEAD_INITIALIZER(list_global);
static Objlist list_main =	/* Objects loaded at program startup */
  STAILQ_HEAD_INITIALIZER(list_main);
static Objlist list_fini =	/* Objects needing fini() calls */
  STAILQ_HEAD_INITIALIZER(list_fini);

static Elf_Sym sym_zero;	/* For resolving undefined weak refs. */

#define GDB_STATE(s,m)	r_debug.r_state = s; r_debug_state(&r_debug,m);

extern Elf_Dyn _DYNAMIC;
#pragma weak _DYNAMIC
#ifndef RTLD_IS_DYNAMIC
#define	RTLD_IS_DYNAMIC()	(&_DYNAMIC != NULL)
#endif

/*
 * These are the functions the dynamic linker exports to application
 * programs.  They are the only symbols the dynamic linker is willing
 * to export from itself.
 */
static func_ptr_type exports[] = {
    (func_ptr_type) &_rtld_error,
    (func_ptr_type) &dlclose,
    (func_ptr_type) &dlerror,
    (func_ptr_type) &dlopen,
    (func_ptr_type) &dlsym,
    (func_ptr_type) &dladdr,
    (func_ptr_type) &dllockinit,
    (func_ptr_type) &dlinfo,
    (func_ptr_type) &_rtld_thread_init,
#ifdef __i386__
    (func_ptr_type) &___tls_get_addr,
#endif
    (func_ptr_type) &__tls_get_addr,
    (func_ptr_type) &_rtld_allocate_tls,
    (func_ptr_type) &_rtld_free_tls,
    NULL
};

/*
 * Global declarations normally provided by crt1.  The dynamic linker is
 * not built with crt1, so we have to provide them ourselves.
 */
char *__progname;
char **environ;

/*
 * Globals to control TLS allocation.
 */
size_t tls_last_offset;		/* Static TLS offset of last module */
size_t tls_last_size;		/* Static TLS size of last module */
size_t tls_static_space;	/* Static TLS space allocated */
int tls_dtv_generation = 1;	/* Used to detect when dtv size changes  */
int tls_max_index = 1;		/* Largest module index allocated */

/*
 * Fill in a DoneList with an allocation large enough to hold all of
 * the currently-loaded objects.  Keep this as a macro since it calls
 * alloca and we want that to occur within the scope of the caller.
 */
#define donelist_init(dlp)					\
    ((dlp)->objs = alloca(obj_count * sizeof (dlp)->objs[0]),	\
    assert((dlp)->objs != NULL),				\
    (dlp)->num_alloc = obj_count,				\
    (dlp)->num_used = 0)

/*
 * Main entry point for dynamic linking.  The first argument is the
 * stack pointer.  The stack is expected to be laid out as described
 * in the SVR4 ABI specification, Intel 386 Processor Supplement.
 * Specifically, the stack pointer points to a word containing
 * ARGC.  Following that in the stack is a null-terminated sequence
 * of pointers to argument strings.  Then comes a null-terminated
 * sequence of pointers to environment strings.  Finally, there is a
 * sequence of "auxiliary vector" entries.
 *
 * The second argument points to a place to store the dynamic linker's
 * exit procedure pointer and the third to a place to store the main
 * program's object.
 *
 * The return value is the main program's entry point.
 */
func_ptr_type
_rtld(Elf_Addr *sp, func_ptr_type *exit_proc, Obj_Entry **objp)
{
    Elf_Auxinfo *aux_info[AT_COUNT];
    int i;
    int argc;
    char **argv;
    char **env;
    Elf_Auxinfo *aux;
    Elf_Auxinfo *auxp;
    const char *argv0;
    Objlist_Entry *entry;
    Obj_Entry *obj;
    Obj_Entry **preload_tail;
    Objlist initlist;
    int lockstate;

    /*
     * On entry, the dynamic linker itself has not been relocated yet.
     * Be very careful not to reference any global data until after
     * init_rtld has returned.  It is OK to reference file-scope statics
     * and string constants, and to call static and global functions.
     */

    /* Find the auxiliary vector on the stack. */
    argc = *sp++;
    argv = (char **) sp;
    sp += argc + 1;	/* Skip over arguments and NULL terminator */
    env = (char **) sp;
    while (*sp++ != 0)	/* Skip over environment, and NULL terminator */
	;
    aux = (Elf_Auxinfo *) sp;

    /* Digest the auxiliary vector. */
    for (i = 0;  i < AT_COUNT;  i++)
	aux_info[i] = NULL;
    for (auxp = aux;  auxp->a_type != AT_NULL;  auxp++) {
	if (auxp->a_type < AT_COUNT)
	    aux_info[auxp->a_type] = auxp;
    }

    /* Initialize and relocate ourselves. */
    assert(aux_info[AT_BASE] != NULL);
    init_rtld((caddr_t) aux_info[AT_BASE]->a_un.a_ptr);

    __progname = obj_rtld.path;
    argv0 = argv[0] != NULL ? argv[0] : "(null)";
    environ = env;

    trust = !issetugid();

    ld_bind_now = getenv(LD_ "BIND_NOW");
    if (trust) {
	ld_debug = getenv(LD_ "DEBUG");
	libmap_disable = getenv(LD_ "LIBMAP_DISABLE") != NULL;
	ld_library_path = getenv(LD_ "LIBRARY_PATH");
	ld_preload = getenv(LD_ "PRELOAD");
    }
    ld_tracing = getenv(LD_ "TRACE_LOADED_OBJECTS");

    if (ld_debug != NULL && *ld_debug != '\0')
	debug = 1;
    dbg("%s is initialized, base address = %p", __progname,
	(caddr_t) aux_info[AT_BASE]->a_un.a_ptr);
    dbg("RTLD dynamic = %p", obj_rtld.dynamic);
    dbg("RTLD pltgot  = %p", obj_rtld.pltgot);

    /*
     * Load the main program, or process its program header if it is
     * already loaded.
     */
    if (aux_info[AT_EXECFD] != NULL) {	/* Load the main program. */
	int fd = aux_info[AT_EXECFD]->a_un.a_val;
	dbg("loading main program");
	obj_main = map_object(fd, argv0, NULL);
	close(fd);
	if (obj_main == NULL)
	    die();
    } else {				/* Main program already loaded. */
	const Elf_Phdr *phdr;
	int phnum;
	caddr_t entry;

	dbg("processing main program's program header");
	assert(aux_info[AT_PHDR] != NULL);
	phdr = (const Elf_Phdr *) aux_info[AT_PHDR]->a_un.a_ptr;
	assert(aux_info[AT_PHNUM] != NULL);
	phnum = aux_info[AT_PHNUM]->a_un.a_val;
	assert(aux_info[AT_PHENT] != NULL);
	assert(aux_info[AT_PHENT]->a_un.a_val == sizeof(Elf_Phdr));
	assert(aux_info[AT_ENTRY] != NULL);
	entry = (caddr_t) aux_info[AT_ENTRY]->a_un.a_ptr;
	if ((obj_main = digest_phdr(phdr, phnum, entry, argv0)) == NULL)
	    die();
    }

    obj_main->path = xstrdup(argv0);
    obj_main->mainprog = true;

    /*
     * Get the actual dynamic linker pathname from the executable if
     * possible.  (It should always be possible.)  That ensures that
     * gdb will find the right dynamic linker even if a non-standard
     * one is being used.
     */
    if (obj_main->interp != NULL &&
      strcmp(obj_main->interp, obj_rtld.path) != 0) {
	free(obj_rtld.path);
	obj_rtld.path = xstrdup(obj_main->interp);
        __progname = obj_rtld.path;
    }

    digest_dynamic(obj_main, 0);

    linkmap_add(obj_main);
    linkmap_add(&obj_rtld);

    /* Link the main program into the list of objects. */
    *obj_tail = obj_main;
    obj_tail = &obj_main->next;
    obj_count++;
    /* Make sure we don't call the main program's init and fini functions. */
    obj_main->init = obj_main->fini = (Elf_Addr)NULL;

    /* Initialize a fake symbol for resolving undefined weak references. */
    sym_zero.st_info = ELF_ST_INFO(STB_GLOBAL, STT_NOTYPE);
    sym_zero.st_shndx = SHN_UNDEF;

    if (!libmap_disable)
        libmap_disable = (bool)lm_init();

    dbg("loading LD_PRELOAD libraries");
    if (load_preload_objects() == -1)
	die();
    preload_tail = obj_tail;

    dbg("loading needed objects");
    if (load_needed_objects(obj_main) == -1)
	die();

    /* Make a list of all objects loaded at startup. */
    for (obj = obj_list;  obj != NULL;  obj = obj->next) {
	objlist_push_tail(&list_main, obj);
    	obj->refcount++;
    }

    if (ld_tracing) {		/* We're done */
	trace_loaded_objects(obj_main);
	exit(0);
    }

    if (getenv(LD_ "DUMP_REL_PRE") != NULL) {
       dump_relocations(obj_main);
       exit (0);
    }

    /* setup TLS for main thread */
    dbg("initializing initial thread local storage");
    STAILQ_FOREACH(entry, &list_main, link) {
	/*
	 * Allocate all the initial objects out of the static TLS
	 * block even if they didn't ask for it.
	 */
	allocate_tls_offset(entry->obj);
    }
    allocate_initial_tls(obj_list);

    if (relocate_objects(obj_main,
	ld_bind_now != NULL && *ld_bind_now != '\0', &obj_rtld) == -1)
	die();

    dbg("doing copy relocations");
    if (do_copy_relocations(obj_main) == -1)
	die();

    if (getenv(LD_ "DUMP_REL_POST") != NULL) {
       dump_relocations(obj_main);
       exit (0);
    }

    dbg("initializing key program variables");
    set_program_var("__progname", argv[0] != NULL ? basename(argv[0]) : "");
    set_program_var("environ", env);

    dbg("initializing thread locks");
    lockdflt_init();

    /* Make a list of init functions to call. */
    objlist_init(&initlist);
    initlist_add_objects(obj_list, preload_tail, &initlist);

    r_debug_state(NULL, &obj_main->linkmap); /* say hello to gdb! */

    objlist_call_init(&initlist);
    lockstate = wlock_acquire(rtld_bind_lock);
    objlist_clear(&initlist);
    wlock_release(rtld_bind_lock, lockstate);

    dbg("transferring control to program entry point = %p", obj_main->entry);

    /* Return the exit procedure and the program entry point. */
    *exit_proc = rtld_exit;
    *objp = obj_main;
    return (func_ptr_type) obj_main->entry;
}

#endif /* 0 */

Elf_Addr
_rtld_bind(Obj_Entry *obj, Elf_Word reloff)
{
    const Elf_Rel *rel;
    const Elf_Sym *def;
    const Obj_Entry *defobj;
    Elf_Addr *where;
    Elf_Addr target;
    int lockstate;

    lockstate = rlock_acquire(rtld_bind_lock);
    if (obj->pltrel)
	rel = (const Elf_Rel *) ((caddr_t) obj->pltrel + reloff);
    else
	rel = (const Elf_Rel *) ((caddr_t) obj->pltrela + reloff);

    where = (Elf_Addr *) (obj->relocbase + rel->r_offset);
    def = find_symdef(ELF_R_SYM(rel->r_info), obj, &defobj, true, NULL);
    if (def == NULL)
	die();

    target = (Elf_Addr)(defobj->relocbase + def->st_value);

    dbg("\"%s\" in \"%s\" ==> %p in \"%s\"",
      defobj->strtab + def->st_name, basename(obj->path),
      (void *)target, basename(defobj->path));

    /*
     * Write the new contents for the jmpslot. Note that depending on
     * architecture, the value which we need to return back to the
     * lazy binding trampoline may or may not be the target
     * address. The value returned from reloc_jmpslot() is the value
     * that the trampoline needs.
     */
    target = reloc_jmpslot(where, target, defobj, obj, rel);
    rlock_release(rtld_bind_lock, lockstate);
    return target;
}

#if 0

/*
 * Error reporting function.  Use it like printf.  If formats the message
 * into a buffer, and sets things up so that the next call to dlerror()
 * will return the message.
 */
void
_rtld_error(const char *fmt, ...)
{
    static char buf[512];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    error_message = buf;
    va_end(ap);
}

/*
 * Return a dynamically-allocated copy of the current error message, if any.
 */
static char *
errmsg_save(void)
{
    return error_message == NULL ? NULL : xstrdup(error_message);
}

/*
 * Restore the current error message from a copy which was previously saved
 * by errmsg_save().  The copy is freed.
 */
static void
errmsg_restore(char *saved_msg)
{
    if (saved_msg == NULL)
	error_message = NULL;
    else {
	_rtld_error("%s", saved_msg);
	free(saved_msg);
    }
}

static const char *
basename(const char *name)
{
    const char *p = strrchr(name, '/');
    return p != NULL ? p + 1 : name;
}

static void
die(void)
{
    const char *msg = dlerror();

    if (msg == NULL)
	msg = "Fatal error";
    errx(1, "%s", msg);
}

#endif /* 0 */

/*
 * Process a shared object's DYNAMIC section, and save the important
 * information in its Obj_Entry structure.
 */
static void
digest_dynamic(Obj_Entry *obj, int early)
{
    const Elf_Dyn *dynp;
    Needed_Entry **needed_tail = &obj->needed;
    const Elf_Dyn *dyn_rpath = NULL;
    int plttype = DT_REL;

    obj->bind_now = false;
    for (dynp = obj->dynamic;  dynp->d_tag != DT_NULL;  dynp++) {
	switch (dynp->d_tag) {

	case DT_REL:
	    obj->rel = (const Elf_Rel *) (obj->relocbase + dynp->d_un.d_ptr);
	    break;

	case DT_RELSZ:
	    obj->relsize = dynp->d_un.d_val;
	    break;

	case DT_RELENT:
	    assert(dynp->d_un.d_val == sizeof(Elf_Rel));
	    break;

	case DT_JMPREL:
	    obj->pltrel = (const Elf_Rel *)
	      (obj->relocbase + dynp->d_un.d_ptr);
	    break;

	case DT_PLTRELSZ:
	    obj->pltrelsize = dynp->d_un.d_val;
	    break;

	case DT_RELA:
	    obj->rela = (const Elf_Rela *) (obj->relocbase + dynp->d_un.d_ptr);
	    break;

	case DT_RELASZ:
	    obj->relasize = dynp->d_un.d_val;
	    break;

	case DT_RELAENT:
	    assert(dynp->d_un.d_val == sizeof(Elf_Rela));
	    break;

	case DT_PLTREL:
	    plttype = dynp->d_un.d_val;
	    assert(dynp->d_un.d_val == DT_REL || plttype == DT_RELA);
	    break;

	case DT_SYMTAB:
	    obj->symtab = (const Elf_Sym *)
	      (obj->relocbase + dynp->d_un.d_ptr);
	    break;

	case DT_SYMENT:
	    assert(dynp->d_un.d_val == sizeof(Elf_Sym));
	    break;

	case DT_STRTAB:
	    obj->strtab = (const char *) (obj->relocbase + dynp->d_un.d_ptr);
	    break;

	case DT_STRSZ:
	    obj->strsize = dynp->d_un.d_val;
	    break;

	case DT_HASH:
	    {
		const Elf_Hashelt *hashtab = (const Elf_Hashelt *)
		  (obj->relocbase + dynp->d_un.d_ptr);
		obj->nbuckets = hashtab[0];
		obj->nchains = hashtab[1];
		obj->buckets = hashtab + 2;
		obj->chains = obj->buckets + obj->nbuckets;
	    }
	    break;

	case DT_NEEDED:
	    if (!obj->rtld) {
		Needed_Entry *nep = NEW(Needed_Entry);
		nep->name = dynp->d_un.d_val;
		nep->obj = NULL;
		nep->next = NULL;

		*needed_tail = nep;
		needed_tail = &nep->next;
	    }
	    break;

	case DT_PLTGOT:
	    obj->pltgot = (Elf_Addr *) (obj->relocbase + dynp->d_un.d_ptr);
	    break;

	case DT_TEXTREL:
	    obj->textrel = true;
	    break;

	case DT_SYMBOLIC:
	    obj->symbolic = true;
	    break;

	case DT_RPATH:
	case DT_RUNPATH:	/* XXX: process separately */
	    /*
	     * We have to wait until later to process this, because we
	     * might not have gotten the address of the string table yet.
	     */
	    dyn_rpath = dynp;
	    break;

	case DT_SONAME:
	    /* Not used by the dynamic linker. */
	    break;

	case DT_INIT:
	    obj->init = (Elf_Addr) (obj->relocbase + dynp->d_un.d_ptr);
	    break;

	case DT_FINI:
	    obj->fini = (Elf_Addr) (obj->relocbase + dynp->d_un.d_ptr);
	    break;

	case DT_DEBUG:
	    /* XXX - not implemented yet */
	    if (!early)
		dbg("Filling in DT_DEBUG entry");
	    ((Elf_Dyn*)dynp)->d_un.d_ptr = (Elf_Addr) &r_debug;
	    break;

	case DT_FLAGS:
		if (dynp->d_un.d_val & DF_ORIGIN) {
		    obj->origin_path = xmalloc(PATH_MAX);
		    if (rtld_dirname(obj->path, obj->origin_path) == -1)
			die();
		}
		if (dynp->d_un.d_val & DF_SYMBOLIC)
		    obj->symbolic = true;
		if (dynp->d_un.d_val & DF_TEXTREL)
		    obj->textrel = true;
		if (dynp->d_un.d_val & DF_BIND_NOW)
		    obj->bind_now = true;
		if (dynp->d_un.d_val & DF_STATIC_TLS)
		    ;
	    break;

	default:
	    if (!early) {
		dbg("Ignoring d_tag %ld = %#lx", (long)dynp->d_tag,
		    (long)dynp->d_tag);
	    }
	    break;
	}
    }

    obj->traced = false;

    if (plttype == DT_RELA) {
	obj->pltrela = (const Elf_Rela *) obj->pltrel;
	obj->pltrel = NULL;
	obj->pltrelasize = obj->pltrelsize;
	obj->pltrelsize = 0;
    }

    if (dyn_rpath != NULL)
	obj->rpath = obj->strtab + dyn_rpath->d_un.d_val;
}

/*
 * Process a shared object's program header.  This is used only for the
 * main program, when the kernel has already loaded the main program
 * into memory before calling the dynamic linker.  It creates and
 * returns an Obj_Entry structure.
 */
static Obj_Entry *
digest_phdr(const Elf_Phdr *phdr, int phnum, caddr_t entry, const char *path)
{
    Obj_Entry *obj;
    const Elf_Phdr *phlimit = phdr + phnum;
    const Elf_Phdr *ph;
    int nsegs = 0;

    obj = obj_new();
    for (ph = phdr;  ph < phlimit;  ph++) {
	switch (ph->p_type) {

	case PT_PHDR:
	    if ((const Elf_Phdr *)ph->p_vaddr != phdr) {
		_rtld_error("%s: invalid PT_PHDR", path);
		return NULL;
	    }
	    obj->phdr = (const Elf_Phdr *) ph->p_vaddr;
	    obj->phsize = ph->p_memsz;
	    break;

	case PT_INTERP:
	    obj->interp = (const char *) ph->p_vaddr;
	    break;

	case PT_LOAD:
	    if (nsegs == 0) {	/* First load segment */
		obj->vaddrbase = trunc_page(ph->p_vaddr);
		obj->mapbase = (caddr_t) obj->vaddrbase;
		obj->relocbase = obj->mapbase - obj->vaddrbase;
		obj->textsize = round_page(ph->p_vaddr + ph->p_memsz) -
		  obj->vaddrbase;
	    } else {		/* Last load segment */
		obj->mapsize = round_page(ph->p_vaddr + ph->p_memsz) -
		  obj->vaddrbase;
	    }
	    nsegs++;
	    break;

	case PT_DYNAMIC:
	    obj->dynamic = (const Elf_Dyn *) ph->p_vaddr;
	    break;

	case PT_TLS:
	    obj->tlsindex = 1;
	    obj->tlssize = ph->p_memsz;
	    obj->tlsalign = ph->p_align;
	    obj->tlsinitsize = ph->p_filesz;
	    obj->tlsinit = (void*) ph->p_vaddr;
	    break;
	}
    }
    if (nsegs < 1) {
	_rtld_error("%s: too few PT_LOAD segments", path);
	return NULL;
    }

    obj->entry = entry;
    return obj;
}

#if 0

static Obj_Entry *
dlcheck(void *handle)
{
    Obj_Entry *obj;

    for (obj = obj_list;  obj != NULL;  obj = obj->next)
	if (obj == (Obj_Entry *) handle)
	    break;

    if (obj == NULL || obj->refcount == 0 || obj->dl_refcount == 0) {
	_rtld_error("Invalid shared object handle %p", handle);
	return NULL;
    }
    return obj;
}

/*
 * If the given object is already in the donelist, return true.  Otherwise
 * add the object to the list and return false.
 */
static bool
donelist_check(DoneList *dlp, const Obj_Entry *obj)
{
    unsigned int i;

    for (i = 0;  i < dlp->num_used;  i++)
	if (dlp->objs[i] == obj)
	    return true;
    /*
     * Our donelist allocation should always be sufficient.  But if
     * our threads locking isn't working properly, more shared objects
     * could have been loaded since we allocated the list.  That should
     * never happen, but we'll handle it properly just in case it does.
     */
    if (dlp->num_used < dlp->num_alloc)
	dlp->objs[dlp->num_used++] = obj;
    return false;
}

#endif /* 0 */

/*
 * Hash function for symbol table lookup.  Don't even think about changing
 * this.  It is specified by the System V ABI.
 */
unsigned long
elf_hash(const char *name)
{
    const unsigned char *p = (const unsigned char *) name;
    unsigned long h = 0;
    unsigned long g;

    while (*p != '\0') {
	h = (h << 4) + *p++;
	if ((g = h & 0xf0000000) != 0)
	    h ^= g >> 24;
	h &= ~g;
    }
    return h;
}

#if 0

/*
 * Find the library with the given name, and return its full pathname.
 * The returned string is dynamically allocated.  Generates an error
 * message and returns NULL if the library cannot be found.
 *
 * If the second argument is non-NULL, then it refers to an already-
 * loaded shared object, whose library search path will be searched.
 *
 * The search order is:
 *   LD_LIBRARY_PATH
 *   rpath in the referencing file
 *   ldconfig hints
 *   /lib:/usr/lib
 */
static char *
find_library(const char *xname, const Obj_Entry *refobj)
{
    char *pathname;
    char *name;

    if (strchr(xname, '/') != NULL) {	/* Hard coded pathname */
	if (xname[0] != '/' && !trust) {
	    _rtld_error("Absolute pathname required for shared object \"%s\"",
	      xname);
	    return NULL;
	}
	return xstrdup(xname);
    }

    if (libmap_disable || (refobj == NULL) ||
	(name = lm_find(refobj->path, xname)) == NULL)
	name = (char *)xname;

    dbg(" Searching for \"%s\"", name);

    if ((pathname = search_library_path(name, ld_library_path)) != NULL ||
      (refobj != NULL &&
      (pathname = search_library_path(name, refobj->rpath)) != NULL) ||
      (pathname = search_library_path(name, gethints())) != NULL ||
      (pathname = search_library_path(name, STANDARD_LIBRARY_PATH)) != NULL)
	return pathname;

    if(refobj != NULL && refobj->path != NULL) {
	_rtld_error("Shared object \"%s\" not found, required by \"%s\"",
	  name, basename(refobj->path));
    } else {
	_rtld_error("Shared object \"%s\" not found", name);
    }
    return NULL;
}

#endif /* 0 */

/*
 * Given a symbol number in a referencing object, find the corresponding
 * definition of the symbol.  Returns a pointer to the symbol, or NULL if
 * no definition was found.  Returns a pointer to the Obj_Entry of the
 * defining object via the reference parameter DEFOBJ_OUT.
 */
const Elf_Sym *
find_symdef(unsigned long symnum, const Obj_Entry *refobj,
    const Obj_Entry **defobj_out, bool in_plt, SymCache *cache)
{
    const Elf_Sym *ref;
    const Elf_Sym *def;
    const Obj_Entry *defobj;
    const char *name;
    unsigned long hash;

    /*
     * If we have already found this symbol, get the information from
     * the cache.
     */
    if (symnum >= refobj->nchains)
	return NULL;	/* Bad object */
    if (cache != NULL && cache[symnum].sym != NULL) {
	*defobj_out = cache[symnum].obj;
	return cache[symnum].sym;
    }

    ref = refobj->symtab + symnum;
    name = refobj->strtab + ref->st_name;
    defobj = NULL;

    /*
     * We don't have to do a full scale lookup if the symbol is local.
     * We know it will bind to the instance in this load module; to
     * which we already have a pointer (ie ref). By not doing a lookup,
     * we not only improve performance, but it also avoids unresolvable
     * symbols when local symbols are not in the hash table. This has
     * been seen with the ia64 toolchain.
     */
    if (ELF_ST_BIND(ref->st_info) != STB_LOCAL) {
	if (ELF_ST_TYPE(ref->st_info) == STT_SECTION) {
	    _rtld_error("%s: Bogus symbol table entry %lu", refobj->path,
		symnum);
	}
	hash = elf_hash(name);
	def = symlook_default(name, hash, refobj, &defobj, in_plt);
    } else {
	def = ref;
	defobj = refobj;
    }

    /*
     * If we found no definition and the reference is weak, treat the
     * symbol as having the value zero.
     */
    if (def == NULL && ELF_ST_BIND(ref->st_info) == STB_WEAK) {
	def = &sym_zero;
	defobj = obj_main;
    }

    if (def != NULL) {
	*defobj_out = defobj;
	/* Record the information in the cache to avoid subsequent lookups. */
	if (cache != NULL) {
	    cache[symnum].sym = def;
	    cache[symnum].obj = defobj;
	}
    } else {
	if (refobj != &obj_rtld)
	    _rtld_error("%s: Undefined symbol \"%s\"", refobj->path, name);
    }
    return def;
}

#if 0

/*
 * Return the search path from the ldconfig hints file, reading it if
 * necessary.  Returns NULL if there are problems with the hints file,
 * or if the search path there is empty.
 */
static const char *
gethints(void)
{
    static char *hints;

    if (hints == NULL) {
	int fd;
	struct elfhints_hdr hdr;
	char *p;

	/* Keep from trying again in case the hints file is bad. */
	hints = "";

	if ((fd = open(_PATH_ELF_HINTS, O_RDONLY)) == -1)
	    return NULL;
	if (read(fd, &hdr, sizeof hdr) != sizeof hdr ||
	  hdr.magic != ELFHINTS_MAGIC ||
	  hdr.version != 1) {
	    close(fd);
	    return NULL;
	}
	p = xmalloc(hdr.dirlistlen + 1);
	if (lseek(fd, hdr.strtab + hdr.dirlist, SEEK_SET) == -1 ||
	  read(fd, p, hdr.dirlistlen + 1) != (ssize_t)hdr.dirlistlen + 1) {
	    free(p);
	    close(fd);
	    return NULL;
	}
	hints = p;
	close(fd);
    }
    return hints[0] != '\0' ? hints : NULL;
}

static void
init_dag(Obj_Entry *root)
{
    DoneList donelist;

    donelist_init(&donelist);
    init_dag1(root, root, &donelist);
}

static void
init_dag1(Obj_Entry *root, Obj_Entry *obj, DoneList *dlp)
{
    const Needed_Entry *needed;

    if (donelist_check(dlp, obj))
	return;

    obj->refcount++;
    objlist_push_tail(&obj->dldags, root);
    objlist_push_tail(&root->dagmembers, obj);
    for (needed = obj->needed;  needed != NULL;  needed = needed->next)
	if (needed->obj != NULL)
	    init_dag1(root, needed->obj, dlp);
}

/*
 * Initialize the dynamic linker.  The argument is the address at which
 * the dynamic linker has been mapped into memory.  The primary task of
 * this function is to relocate the dynamic linker.
 */
static void
init_rtld(caddr_t mapbase)
{
    Obj_Entry objtmp;	/* Temporary rtld object */

    /*
     * Conjure up an Obj_Entry structure for the dynamic linker.
     *
     * The "path" member can't be initialized yet because string constatns
     * cannot yet be acessed. Below we will set it correctly.
     */
    memset(&objtmp, 0, sizeof(objtmp));
    objtmp.path = NULL;
    objtmp.rtld = true;
    objtmp.mapbase = mapbase;
#ifdef PIC
    objtmp.relocbase = mapbase;
#endif
    if (RTLD_IS_DYNAMIC()) {
	objtmp.dynamic = rtld_dynamic(&objtmp);
	digest_dynamic(&objtmp, 1);
	assert(objtmp.needed == NULL);
	assert(!objtmp.textrel);

	/*
	 * Temporarily put the dynamic linker entry into the object list, so
	 * that symbols can be found.
	 */

	relocate_objects(&objtmp, true, &objtmp);
    }

    /* Initialize the object list. */
    obj_tail = &obj_list;

    /* Now that non-local variables can be accesses, copy out obj_rtld. */
    memcpy(&obj_rtld, &objtmp, sizeof(obj_rtld));

    /* Replace the path with a dynamically allocated copy. */
    obj_rtld.path = xstrdup(PATH_RTLD);

    r_debug.r_brk = r_debug_state;
    r_debug.r_state = RT_CONSISTENT;
}

/*
 * Add the init functions from a needed object list (and its recursive
 * needed objects) to "list".  This is not used directly; it is a helper
 * function for initlist_add_objects().  The write lock must be held
 * when this function is called.
 */
static void
initlist_add_neededs(Needed_Entry *needed, Objlist *list)
{
    /* Recursively process the successor needed objects. */
    if (needed->next != NULL)
	initlist_add_neededs(needed->next, list);

    /* Process the current needed object. */
    if (needed->obj != NULL)
	initlist_add_objects(needed->obj, &needed->obj->next, list);
}

/*
 * Scan all of the DAGs rooted in the range of objects from "obj" to
 * "tail" and add their init functions to "list".  This recurses over
 * the DAGs and ensure the proper init ordering such that each object's
 * needed libraries are initialized before the object itself.  At the
 * same time, this function adds the objects to the global finalization
 * list "list_fini" in the opposite order.  The write lock must be
 * held when this function is called.
 */
static void
initlist_add_objects(Obj_Entry *obj, Obj_Entry **tail, Objlist *list)
{
    if (obj->init_done)
	return;
    obj->init_done = true;

    /* Recursively process the successor objects. */
    if (&obj->next != tail)
	initlist_add_objects(obj->next, tail, list);

    /* Recursively process the needed objects. */
    if (obj->needed != NULL)
	initlist_add_neededs(obj->needed, list);

    /* Add the object to the init list. */
    if (obj->init != (Elf_Addr)NULL)
	objlist_push_tail(list, obj);

    /* Add the object to the global fini list in the reverse order. */
    if (obj->fini != (Elf_Addr)NULL)
	objlist_push_head(&list_fini, obj);
}

#ifndef FPTR_TARGET
#define FPTR_TARGET(f)	((Elf_Addr) (f))
#endif

static bool
is_exported(const Elf_Sym *def)
{
    Elf_Addr value;
    const func_ptr_type *p;

    value = (Elf_Addr)(obj_rtld.relocbase + def->st_value);
    for (p = exports;  *p != NULL;  p++)
	if (FPTR_TARGET(*p) == value)
	    return true;
    return false;
}

/*
 * Given a shared object, traverse its list of needed objects, and load
 * each of them.  Returns 0 on success.  Generates an error message and
 * returns -1 on failure.
 */
static int
load_needed_objects(Obj_Entry *first)
{
    Obj_Entry *obj;

    for (obj = first;  obj != NULL;  obj = obj->next) {
	Needed_Entry *needed;

	for (needed = obj->needed;  needed != NULL;  needed = needed->next) {
	    const char *name = obj->strtab + needed->name;
	    char *path = find_library(name, obj);

	    needed->obj = NULL;
	    if (path == NULL && !ld_tracing)
		return -1;

	    if (path) {
		needed->obj = load_object(path);
		if (needed->obj == NULL && !ld_tracing)
		    return -1;		/* XXX - cleanup */
	    }
	}
    }

    return 0;
}

static int
load_preload_objects(void)
{
    char *p = ld_preload;
    static const char delim[] = " \t:;";

    if (p == NULL)
	return 0;

    p += strspn(p, delim);
    while (*p != '\0') {
	size_t len = strcspn(p, delim);
	char *path;
	char savech;

	savech = p[len];
	p[len] = '\0';
	if ((path = find_library(p, NULL)) == NULL)
	    return -1;
	if (load_object(path) == NULL)
	    return -1;	/* XXX - cleanup */
	p[len] = savech;
	p += len;
	p += strspn(p, delim);
    }
    return 0;
}

/*
 * Load a shared object into memory, if it is not already loaded.  The
 * argument must be a string allocated on the heap.  This function assumes
 * responsibility for freeing it when necessary.
 *
 * Returns a pointer to the Obj_Entry for the object.  Returns NULL
 * on failure.
 */
static Obj_Entry *
load_object(char *path)
{
    Obj_Entry *obj;
    int fd = -1;
    struct stat sb;

    for (obj = obj_list->next;  obj != NULL;  obj = obj->next)
	if (strcmp(obj->path, path) == 0)
	    break;

    /*
     * If we didn't find a match by pathname, open the file and check
     * again by device and inode.  This avoids false mismatches caused
     * by multiple links or ".." in pathnames.
     *
     * To avoid a race, we open the file and use fstat() rather than
     * using stat().
     */
    if (obj == NULL) {
	if ((fd = open(path, O_RDONLY)) == -1) {
	    _rtld_error("Cannot open \"%s\"", path);
	    return NULL;
	}
	if (fstat(fd, &sb) == -1) {
	    _rtld_error("Cannot fstat \"%s\"", path);
	    close(fd);
	    return NULL;
	}
	for (obj = obj_list->next;  obj != NULL;  obj = obj->next) {
	    if (obj->ino == sb.st_ino && obj->dev == sb.st_dev) {
		close(fd);
		break;
	    }
	}
    }

    if (obj == NULL) {	/* First use of this object, so we must map it in */
	dbg("loading \"%s\"", path);
	obj = map_object(fd, path, &sb);
	close(fd);
	if (obj == NULL) {
	    free(path);
	    return NULL;
	}

	obj->path = path;
	digest_dynamic(obj, 0);

	*obj_tail = obj;
	obj_tail = &obj->next;
	obj_count++;
	linkmap_add(obj);	/* for GDB & dlinfo() */

	dbg("  %p .. %p: %s", obj->mapbase,
	  obj->mapbase + obj->mapsize - 1, obj->path);
	if (obj->textrel)
	    dbg("  WARNING: %s has impure text", obj->path);
    } else
	free(path);

    return obj;
}

static Obj_Entry *
obj_from_addr(const void *addr)
{
    Obj_Entry *obj;

    for (obj = obj_list;  obj != NULL;  obj = obj->next) {
	if (addr < (void *) obj->mapbase)
	    continue;
	if (addr < (void *) (obj->mapbase + obj->mapsize))
	    return obj;
    }
    return NULL;
}

/*
 * Call the finalization functions for each of the objects in "list"
 * which are unreferenced.  All of the objects are expected to have
 * non-NULL fini functions.
 */
static void
objlist_call_fini(Objlist *list)
{
    Objlist_Entry *elm;
    char *saved_msg;

    /*
     * Preserve the current error message since a fini function might
     * call into the dynamic linker and overwrite it.
     */
    saved_msg = errmsg_save();
    STAILQ_FOREACH(elm, list, link) {
	if (elm->obj->refcount == 0) {
	    dbg("calling fini function for %s at %p", elm->obj->path,
	        (void *)elm->obj->fini);
	    call_initfini_pointer(elm->obj, elm->obj->fini);
	}
    }
    errmsg_restore(saved_msg);
}

/*
 * Call the initialization functions for each of the objects in
 * "list".  All of the objects are expected to have non-NULL init
 * functions.
 */
static void
objlist_call_init(Objlist *list)
{
    Objlist_Entry *elm;
    char *saved_msg;

    /*
     * Preserve the current error message since an init function might
     * call into the dynamic linker and overwrite it.
     */
    saved_msg = errmsg_save();
    STAILQ_FOREACH(elm, list, link) {
	dbg("calling init function for %s at %p", elm->obj->path,
	    (void *)elm->obj->init);
	call_initfini_pointer(elm->obj, elm->obj->init);
    }
    errmsg_restore(saved_msg);
}

static void
objlist_clear(Objlist *list)
{
    Objlist_Entry *elm;

    while (!STAILQ_EMPTY(list)) {
	elm = STAILQ_FIRST(list);
	STAILQ_REMOVE_HEAD(list, link);
	free(elm);
    }
}

static Objlist_Entry *
objlist_find(Objlist *list, const Obj_Entry *obj)
{
    Objlist_Entry *elm;

    STAILQ_FOREACH(elm, list, link)
	if (elm->obj == obj)
	    return elm;
    return NULL;
}

static void
objlist_init(Objlist *list)
{
    STAILQ_INIT(list);
}

static void
objlist_push_head(Objlist *list, Obj_Entry *obj)
{
    Objlist_Entry *elm;

    elm = NEW(Objlist_Entry);
    elm->obj = obj;
    STAILQ_INSERT_HEAD(list, elm, link);
}

static void
objlist_push_tail(Objlist *list, Obj_Entry *obj)
{
    Objlist_Entry *elm;

    elm = NEW(Objlist_Entry);
    elm->obj = obj;
    STAILQ_INSERT_TAIL(list, elm, link);
}

static void
objlist_remove(Objlist *list, Obj_Entry *obj)
{
    Objlist_Entry *elm;

    if ((elm = objlist_find(list, obj)) != NULL) {
	STAILQ_REMOVE(list, elm, Struct_Objlist_Entry, link);
	free(elm);
    }
}

/*
 * Remove all of the unreferenced objects from "list".
 */
static void
objlist_remove_unref(Objlist *list)
{
    Objlist newlist;
    Objlist_Entry *elm;

    STAILQ_INIT(&newlist);
    while (!STAILQ_EMPTY(list)) {
	elm = STAILQ_FIRST(list);
	STAILQ_REMOVE_HEAD(list, link);
	if (elm->obj->refcount == 0)
	    free(elm);
	else
	    STAILQ_INSERT_TAIL(&newlist, elm, link);
    }
    *list = newlist;
}

#endif /* 0 */

/*
 * Relocate newly-loaded shared objects.  The argument is a pointer to
 * the Obj_Entry for the first such object.  All objects from the first
 * to the end of the list of objects are relocated.  Returns 0 on success,
 * or -1 on failure.
 */
static int
relocate_objects(Obj_Entry *first, bool bind_now, Obj_Entry *rtldobj)
{
    Obj_Entry *obj;

    for (obj = first;  obj != NULL;  obj = obj->next) {
	if (obj != rtldobj)
	    dbg("relocating \"%s\"", obj->path);
	if (obj->nbuckets == 0 || obj->nchains == 0 || obj->buckets == NULL ||
	    obj->symtab == NULL || obj->strtab == NULL) {
	    _rtld_error("%s: Shared object has no run-time symbol table",
	      obj->path);
	    return -1;
	}

	if (obj->textrel) {
	    /* There are relocations to the write-protected text segment. */
	    if (mprotect(obj->mapbase, obj->textsize,
	      PROT_READ|PROT_WRITE|PROT_EXEC) == -1) {
		_rtld_error("%s: Cannot write-enable text segment: %s",
		  obj->path, strerror(errno));
		return -1;
	    }
	}

	/* Process the non-PLT relocations. */
	if (reloc_non_plt(obj, rtldobj))
		return -1;

	if (obj->textrel) {	/* Re-protected the text segment. */
	    if (mprotect(obj->mapbase, obj->textsize,
	      PROT_READ|PROT_EXEC) == -1) {
		_rtld_error("%s: Cannot write-protect text segment: %s",
		  obj->path, strerror(errno));
		return -1;
	    }
	}

	/* Process the PLT relocations. */
	if (reloc_plt(obj) == -1)
	    return -1;
	/* Relocate the jump slots if we are doing immediate binding. */
	if (obj->bind_now || bind_now)
	    if (reloc_jmpslots(obj) == -1)
		return -1;


	/*
	 * Set up the magic number and version in the Obj_Entry.  These
	 * were checked in the crt1.o from the original ElfKit, so we
	 * set them for backward compatibility.
	 */
	obj->magic = RTLD_MAGIC;
	obj->version = RTLD_VERSION;

	/* Set the special PLT or GOT entries. */
	init_pltgot(obj);
    }

    return 0;
}

#if 0

/*
 * Cleanup procedure.  It will be called (by the atexit mechanism) just
 * before the process exits.
 */
static void
rtld_exit(void)
{
    Obj_Entry *obj;

    dbg("rtld_exit()");
    /* Clear all the reference counts so the fini functions will be called. */
    for (obj = obj_list;  obj != NULL;  obj = obj->next)
	obj->refcount = 0;
    objlist_call_fini(&list_fini);
    /* No need to remove the items from the list, since we are exiting. */
    if (!libmap_disable)
        lm_fini();
}

static void *
path_enumerate(const char *path, path_enum_proc callback, void *arg)
{
#ifdef COMPAT_32BIT
    const char *trans;
#endif
    if (path == NULL)
	return (NULL);

    path += strspn(path, ":;");
    while (*path != '\0') {
	size_t len;
	char  *res;

	len = strcspn(path, ":;");
#ifdef COMPAT_32BIT
	trans = lm_findn(NULL, path, len);
	if (trans)
	    res = callback(trans, strlen(trans), arg);
	else
#endif
	res = callback(path, len, arg);

	if (res != NULL)
	    return (res);

	path += len;
	path += strspn(path, ":;");
    }

    return (NULL);
}

struct try_library_args {
    const char	*name;
    size_t	 namelen;
    char	*buffer;
    size_t	 buflen;
};

static void *
try_library_path(const char *dir, size_t dirlen, void *param)
{
    struct try_library_args *arg;

    arg = param;
    if (*dir == '/' || trust) {
	char *pathname;

	if (dirlen + 1 + arg->namelen + 1 > arg->buflen)
		return (NULL);

	pathname = arg->buffer;
	strncpy(pathname, dir, dirlen);
	pathname[dirlen] = '/';
	strcpy(pathname + dirlen + 1, arg->name);

	dbg("  Trying \"%s\"", pathname);
	if (access(pathname, F_OK) == 0) {		/* We found it */
	    pathname = xmalloc(dirlen + 1 + arg->namelen + 1);
	    strcpy(pathname, arg->buffer);
	    return (pathname);
	}
    }
    return (NULL);
}

static char *
search_library_path(const char *name, const char *path)
{
    char *p;
    struct try_library_args arg;

    if (path == NULL)
	return NULL;

    arg.name = name;
    arg.namelen = strlen(name);
    arg.buffer = xmalloc(PATH_MAX);
    arg.buflen = PATH_MAX;

    p = path_enumerate(path, try_library_path, &arg);

    free(arg.buffer);

    return (p);
}

int
dlclose(void *handle)
{
    Obj_Entry *root;
    int lockstate;

    lockstate = wlock_acquire(rtld_bind_lock);
    root = dlcheck(handle);
    if (root == NULL) {
	wlock_release(rtld_bind_lock, lockstate);
	return -1;
    }

    /* Unreference the object and its dependencies. */
    root->dl_refcount--;

    unref_dag(root);

    if (root->refcount == 0) {
	/*
	 * The object is no longer referenced, so we must unload it.
	 * First, call the fini functions with no locks held.
	 */
	wlock_release(rtld_bind_lock, lockstate);
	objlist_call_fini(&list_fini);
	lockstate = wlock_acquire(rtld_bind_lock);
	objlist_remove_unref(&list_fini);

	/* Finish cleaning up the newly-unreferenced objects. */
	GDB_STATE(RT_DELETE,&root->linkmap);
	unload_object(root);
	GDB_STATE(RT_CONSISTENT,NULL);
    }
    wlock_release(rtld_bind_lock, lockstate);
    return 0;
}

const char *
dlerror(void)
{
    char *msg = error_message;
    error_message = NULL;
    return msg;
}

/*
 * This function is deprecated and has no effect.
 */
void
dllockinit(void *context,
	   void *(*lock_create)(void *context),
           void (*rlock_acquire)(void *lock),
           void (*wlock_acquire)(void *lock),
           void (*lock_release)(void *lock),
           void (*lock_destroy)(void *lock),
	   void (*context_destroy)(void *context))
{
    static void *cur_context;
    static void (*cur_context_destroy)(void *);

    /* Just destroy the context from the previous call, if necessary. */
    if (cur_context_destroy != NULL)
	cur_context_destroy(cur_context);
    cur_context = context;
    cur_context_destroy = context_destroy;
}

void *
dlopen(const char *name, int mode)
{
    Obj_Entry **old_obj_tail;
    Obj_Entry *obj;
    Objlist initlist;
    int result, lockstate;

    ld_tracing = (mode & RTLD_TRACE) == 0 ? NULL : "1";
    if (ld_tracing != NULL)
	environ = (char **)*get_program_var_addr("environ");

    objlist_init(&initlist);

    lockstate = wlock_acquire(rtld_bind_lock);
    GDB_STATE(RT_ADD,NULL);

    old_obj_tail = obj_tail;
    obj = NULL;
    if (name == NULL) {
	obj = obj_main;
	obj->refcount++;
    } else {
	char *path = find_library(name, obj_main);
	if (path != NULL)
	    obj = load_object(path);
    }

    if (obj) {
	obj->dl_refcount++;
	if (mode & RTLD_GLOBAL && objlist_find(&list_global, obj) == NULL)
	    objlist_push_tail(&list_global, obj);
	mode &= RTLD_MODEMASK;
	if (*old_obj_tail != NULL) {		/* We loaded something new. */
	    assert(*old_obj_tail == obj);

	    result = load_needed_objects(obj);
	    if (result != -1 && ld_tracing)
		goto trace;

	    if (result == -1 ||
	      (init_dag(obj), relocate_objects(obj, mode == RTLD_NOW,
	       &obj_rtld)) == -1) {
		obj->dl_refcount--;
		unref_dag(obj);
		if (obj->refcount == 0)
		    unload_object(obj);
		obj = NULL;
	    } else {
		/* Make list of init functions to call. */
		initlist_add_objects(obj, &obj->next, &initlist);
	    }
	} else {

	    /* Bump the reference counts for objects on this DAG. */
	    ref_dag(obj);

	    if (ld_tracing)
		goto trace;
	}
    }

    GDB_STATE(RT_CONSISTENT,obj ? &obj->linkmap : NULL);

    /* Call the init functions with no locks held. */
    wlock_release(rtld_bind_lock, lockstate);
    objlist_call_init(&initlist);
    lockstate = wlock_acquire(rtld_bind_lock);
    objlist_clear(&initlist);
    wlock_release(rtld_bind_lock, lockstate);
    return obj;
trace:
    trace_loaded_objects(obj);
    wlock_release(rtld_bind_lock, lockstate);
    exit(0);
}

void *
dlsym(void *handle, const char *name)
{
    const Obj_Entry *obj;
    unsigned long hash;
    const Elf_Sym *def;
    const Obj_Entry *defobj;
    int lockstate;

    hash = elf_hash(name);
    def = NULL;
    defobj = NULL;

    lockstate = rlock_acquire(rtld_bind_lock);
    if (handle == NULL || handle == RTLD_NEXT ||
	handle == RTLD_DEFAULT || handle == RTLD_SELF) {
	void *retaddr;

	retaddr = __builtin_return_address(0);	/* __GNUC__ only */
	if ((obj = obj_from_addr(retaddr)) == NULL) {
	    _rtld_error("Cannot determine caller's shared object");
	    rlock_release(rtld_bind_lock, lockstate);
	    return NULL;
	}
	if (handle == NULL) {	/* Just the caller's shared object. */
	    def = symlook_obj(name, hash, obj, true);
	    defobj = obj;
	} else if (handle == RTLD_NEXT || /* Objects after caller's */
		   handle == RTLD_SELF) { /* ... caller included */
	    if (handle == RTLD_NEXT)
		obj = obj->next;
	    for (; obj != NULL; obj = obj->next) {
		if ((def = symlook_obj(name, hash, obj, true)) != NULL) {
		    defobj = obj;
		    break;
		}
	    }
	} else {
	    assert(handle == RTLD_DEFAULT);
	    def = symlook_default(name, hash, obj, &defobj, true);
	}
    } else {
	if ((obj = dlcheck(handle)) == NULL) {
	    rlock_release(rtld_bind_lock, lockstate);
	    return NULL;
	}

	if (obj->mainprog) {
	    DoneList donelist;

	    /* Search main program and all libraries loaded by it. */
	    donelist_init(&donelist);
	    def = symlook_list(name, hash, &list_main, &defobj, true,
	      &donelist);
	} else {
	    /*
	     * XXX - This isn't correct.  The search should include the whole
	     * DAG rooted at the given object.
	     */
	    def = symlook_obj(name, hash, obj, true);
	    defobj = obj;
	}
    }

    if (def != NULL) {
	rlock_release(rtld_bind_lock, lockstate);

	/*
	 * The value required by the caller is derived from the value
	 * of the symbol. For the ia64 architecture, we need to
	 * construct a function descriptor which the caller can use to
	 * call the function with the right 'gp' value. For other
	 * architectures and for non-functions, the value is simply
	 * the relocated value of the symbol.
	 */
	if (ELF_ST_TYPE(def->st_info) == STT_FUNC)
	    return make_function_pointer(def, defobj);
	else
	    return defobj->relocbase + def->st_value;
    }

    _rtld_error("Undefined symbol \"%s\"", name);
    rlock_release(rtld_bind_lock, lockstate);
    return NULL;
}

int
dladdr(const void *addr, Dl_info *info)
{
    const Obj_Entry *obj;
    const Elf_Sym *def;
    void *symbol_addr;
    unsigned long symoffset;
    int lockstate;

    lockstate = rlock_acquire(rtld_bind_lock);
    obj = obj_from_addr(addr);
    if (obj == NULL) {
        _rtld_error("No shared object contains address");
	rlock_release(rtld_bind_lock, lockstate);
        return 0;
    }
    info->dli_fname = obj->path;
    info->dli_fbase = obj->mapbase;
    info->dli_saddr = (void *)0;
    info->dli_sname = NULL;

    /*
     * Walk the symbol list looking for the symbol whose address is
     * closest to the address sent in.
     */
    for (symoffset = 0; symoffset < obj->nchains; symoffset++) {
        def = obj->symtab + symoffset;

        /*
         * For skip the symbol if st_shndx is either SHN_UNDEF or
         * SHN_COMMON.
         */
        if (def->st_shndx == SHN_UNDEF || def->st_shndx == SHN_COMMON)
            continue;

        /*
         * If the symbol is greater than the specified address, or if it
         * is further away from addr than the current nearest symbol,
         * then reject it.
         */
        symbol_addr = obj->relocbase + def->st_value;
        if (symbol_addr > addr || symbol_addr < info->dli_saddr)
            continue;

        /* Update our idea of the nearest symbol. */
        info->dli_sname = obj->strtab + def->st_name;
        info->dli_saddr = symbol_addr;

        /* Exact match? */
        if (info->dli_saddr == addr)
            break;
    }
    rlock_release(rtld_bind_lock, lockstate);
    return 1;
}

int
dlinfo(void *handle, int request, void *p)
{
    const Obj_Entry *obj;
    int error, lockstate;

    lockstate = rlock_acquire(rtld_bind_lock);

    if (handle == NULL || handle == RTLD_SELF) {
	void *retaddr;

	retaddr = __builtin_return_address(0);	/* __GNUC__ only */
	if ((obj = obj_from_addr(retaddr)) == NULL)
	    _rtld_error("Cannot determine caller's shared object");
    } else
	obj = dlcheck(handle);

    if (obj == NULL) {
	rlock_release(rtld_bind_lock, lockstate);
	return (-1);
    }

    error = 0;
    switch (request) {
    case RTLD_DI_LINKMAP:
	*((struct link_map const **)p) = &obj->linkmap;
	break;
    case RTLD_DI_ORIGIN:
	error = rtld_dirname(obj->path, p);
	break;

    case RTLD_DI_SERINFOSIZE:
    case RTLD_DI_SERINFO:
	error = do_search_info(obj, request, (struct dl_serinfo *)p);
	break;

    default:
	_rtld_error("Invalid request %d passed to dlinfo()", request);
	error = -1;
    }

    rlock_release(rtld_bind_lock, lockstate);

    return (error);
}

struct fill_search_info_args {
    int		 request;
    unsigned int flags;
    Dl_serinfo  *serinfo;
    Dl_serpath  *serpath;
    char	*strspace;
};

static void *
fill_search_info(const char *dir, size_t dirlen, void *param)
{
    struct fill_search_info_args *arg;

    arg = param;

    if (arg->request == RTLD_DI_SERINFOSIZE) {
	arg->serinfo->dls_cnt ++;
	arg->serinfo->dls_size += dirlen + 1;
    } else {
	struct dl_serpath *s_entry;

	s_entry = arg->serpath;
	s_entry->dls_name  = arg->strspace;
	s_entry->dls_flags = arg->flags;

	strncpy(arg->strspace, dir, dirlen);
	arg->strspace[dirlen] = '\0';

	arg->strspace += dirlen + 1;
	arg->serpath++;
    }

    return (NULL);
}

static int
do_search_info(const Obj_Entry *obj, int request, struct dl_serinfo *info)
{
    struct dl_serinfo _info;
    struct fill_search_info_args args;

    args.request = RTLD_DI_SERINFOSIZE;
    args.serinfo = &_info;

    _info.dls_size = __offsetof(struct dl_serinfo, dls_serpath);
    _info.dls_cnt  = 0;

    path_enumerate(ld_library_path, fill_search_info, &args);
    path_enumerate(obj->rpath, fill_search_info, &args);
    path_enumerate(gethints(), fill_search_info, &args);
    path_enumerate(STANDARD_LIBRARY_PATH, fill_search_info, &args);


    if (request == RTLD_DI_SERINFOSIZE) {
	info->dls_size = _info.dls_size;
	info->dls_cnt = _info.dls_cnt;
	return (0);
    }

    if (info->dls_cnt != _info.dls_cnt || info->dls_size != _info.dls_size) {
	_rtld_error("Uninitialized Dl_serinfo struct passed to dlinfo()");
	return (-1);
    }

    args.request  = RTLD_DI_SERINFO;
    args.serinfo  = info;
    args.serpath  = &info->dls_serpath[0];
    args.strspace = (char *)&info->dls_serpath[_info.dls_cnt];

    args.flags = LA_SER_LIBPATH;
    if (path_enumerate(ld_library_path, fill_search_info, &args) != NULL)
	return (-1);

    args.flags = LA_SER_RUNPATH;
    if (path_enumerate(obj->rpath, fill_search_info, &args) != NULL)
	return (-1);

    args.flags = LA_SER_CONFIG;
    if (path_enumerate(gethints(), fill_search_info, &args) != NULL)
	return (-1);

    args.flags = LA_SER_DEFAULT;
    if (path_enumerate(STANDARD_LIBRARY_PATH, fill_search_info, &args) != NULL)
	return (-1);
    return (0);
}

static int
rtld_dirname(const char *path, char *bname)
{
    const char *endp;

    /* Empty or NULL string gets treated as "." */
    if (path == NULL || *path == '\0') {
	bname[0] = '.';
	bname[1] = '\0';
	return (0);
    }

    /* Strip trailing slashes */
    endp = path + strlen(path) - 1;
    while (endp > path && *endp == '/')
	endp--;

    /* Find the start of the dir */
    while (endp > path && *endp != '/')
	endp--;

    /* Either the dir is "/" or there are no slashes */
    if (endp == path) {
	bname[0] = *endp == '/' ? '/' : '.';
	bname[1] = '\0';
	return (0);
    } else {
	do {
	    endp--;
	} while (endp > path && *endp == '/');
    }

    if (endp - path + 2 > PATH_MAX)
    {
	_rtld_error("Filename is too long: %s", path);
	return(-1);
    }

    strncpy(bname, path, endp - path + 1);
    bname[endp - path + 1] = '\0';
    return (0);
}

static void
linkmap_add(Obj_Entry *obj)
{
    struct link_map *l = &obj->linkmap;
    struct link_map *prev;

    obj->linkmap.l_name = obj->path;
    obj->linkmap.l_addr = obj->mapbase;
    obj->linkmap.l_ld = obj->dynamic;
#ifdef __mips__
    /* GDB needs load offset on MIPS to use the symbols */
    obj->linkmap.l_offs = obj->relocbase;
#endif

    if (r_debug.r_map == NULL) {
	r_debug.r_map = l;
	return;
    }

    /*
     * Scan to the end of the list, but not past the entry for the
     * dynamic linker, which we want to keep at the very end.
     */
    for (prev = r_debug.r_map;
      prev->l_next != NULL && prev->l_next != &obj_rtld.linkmap;
      prev = prev->l_next)
	;

    /* Link in the new entry. */
    l->l_prev = prev;
    l->l_next = prev->l_next;
    if (l->l_next != NULL)
	l->l_next->l_prev = l;
    prev->l_next = l;
}

static void
linkmap_delete(Obj_Entry *obj)
{
    struct link_map *l = &obj->linkmap;

    if (l->l_prev == NULL) {
	if ((r_debug.r_map = l->l_next) != NULL)
	    l->l_next->l_prev = NULL;
	return;
    }

    if ((l->l_prev->l_next = l->l_next) != NULL)
	l->l_next->l_prev = l->l_prev;
}

/*
 * Function for the debugger to set a breakpoint on to gain control.
 *
 * The two parameters allow the debugger to easily find and determine
 * what the runtime loader is doing and to whom it is doing it.
 *
 * When the loadhook trap is hit (r_debug_state, set at program
 * initialization), the arguments can be found on the stack:
 *
 *  +8   struct link_map *m
 *  +4   struct r_debug  *rd
 *  +0   RetAddr
 */
void
r_debug_state(struct r_debug* rd, struct link_map *m)
{
}

/*
 * Get address of the pointer variable in the main program.
 */
static const void **
get_program_var_addr(const char *name)
{
    const Obj_Entry *obj;
    unsigned long hash;

    hash = elf_hash(name);
    for (obj = obj_main;  obj != NULL;  obj = obj->next) {
	const Elf_Sym *def;

	if ((def = symlook_obj(name, hash, obj, false)) != NULL) {
	    const void **addr;

	    addr = (const void **)(obj->relocbase + def->st_value);
	    return addr;
	}
    }
    return NULL;
}

/*
 * Set a pointer variable in the main program to the given value.  This
 * is used to set key variables such as "environ" before any of the
 * init functions are called.
 */
static void
set_program_var(const char *name, const void *value)
{
    const void **addr;

    if ((addr = get_program_var_addr(name)) != NULL) {
	dbg("\"%s\": *%p <-- %p", name, addr, value);
	*addr = value;
    }
}

/*
 * Given a symbol name in a referencing object, find the corresponding
 * definition of the symbol.  Returns a pointer to the symbol, or NULL if
 * no definition was found.  Returns a pointer to the Obj_Entry of the
 * defining object via the reference parameter DEFOBJ_OUT.
 */
static const Elf_Sym *
symlook_default(const char *name, unsigned long hash,
    const Obj_Entry *refobj, const Obj_Entry **defobj_out, bool in_plt)
{
    DoneList donelist;
    const Elf_Sym *def;
    const Elf_Sym *symp;
    const Obj_Entry *obj;
    const Obj_Entry *defobj;
    const Objlist_Entry *elm;
    def = NULL;
    defobj = NULL;
    donelist_init(&donelist);

    /* Look first in the referencing object if linked symbolically. */
    if (refobj->symbolic && !donelist_check(&donelist, refobj)) {
	symp = symlook_obj(name, hash, refobj, in_plt);
	if (symp != NULL) {
	    def = symp;
	    defobj = refobj;
	}
    }

    /* Search all objects loaded at program start up. */
    if (def == NULL || ELF_ST_BIND(def->st_info) == STB_WEAK) {
	symp = symlook_list(name, hash, &list_main, &obj, in_plt, &donelist);
	if (symp != NULL &&
	  (def == NULL || ELF_ST_BIND(symp->st_info) != STB_WEAK)) {
	    def = symp;
	    defobj = obj;
	}
    }

    /* Search all DAGs whose roots are RTLD_GLOBAL objects. */
    STAILQ_FOREACH(elm, &list_global, link) {
       if (def != NULL && ELF_ST_BIND(def->st_info) != STB_WEAK)
           break;
       symp = symlook_list(name, hash, &elm->obj->dagmembers, &obj, in_plt,
         &donelist);
	if (symp != NULL &&
	  (def == NULL || ELF_ST_BIND(symp->st_info) != STB_WEAK)) {
	    def = symp;
	    defobj = obj;
	}
    }

    /* Search all dlopened DAGs containing the referencing object. */
    STAILQ_FOREACH(elm, &refobj->dldags, link) {
	if (def != NULL && ELF_ST_BIND(def->st_info) != STB_WEAK)
	    break;
	symp = symlook_list(name, hash, &elm->obj->dagmembers, &obj, in_plt,
	  &donelist);
	if (symp != NULL &&
	  (def == NULL || ELF_ST_BIND(symp->st_info) != STB_WEAK)) {
	    def = symp;
	    defobj = obj;
	}
    }

    /*
     * Search the dynamic linker itself, and possibly resolve the
     * symbol from there.  This is how the application links to
     * dynamic linker services such as dlopen.  Only the values listed
     * in the "exports" array can be resolved from the dynamic linker.
     */
    if (def == NULL || ELF_ST_BIND(def->st_info) == STB_WEAK) {
	symp = symlook_obj(name, hash, &obj_rtld, in_plt);
	if (symp != NULL && is_exported(symp)) {
	    def = symp;
	    defobj = &obj_rtld;
	}
    }

    if (def != NULL)
	*defobj_out = defobj;
    return def;
}

static const Elf_Sym *
symlook_list(const char *name, unsigned long hash, Objlist *objlist,
  const Obj_Entry **defobj_out, bool in_plt, DoneList *dlp)
{
    const Elf_Sym *symp;
    const Elf_Sym *def;
    const Obj_Entry *defobj;
    const Objlist_Entry *elm;

    def = NULL;
    defobj = NULL;
    STAILQ_FOREACH(elm, objlist, link) {
	if (donelist_check(dlp, elm->obj))
	    continue;
	if ((symp = symlook_obj(name, hash, elm->obj, in_plt)) != NULL) {
	    if (def == NULL || ELF_ST_BIND(symp->st_info) != STB_WEAK) {
		def = symp;
		defobj = elm->obj;
		if (ELF_ST_BIND(def->st_info) != STB_WEAK)
		    break;
	    }
	}
    }
    if (def != NULL)
	*defobj_out = defobj;
    return def;
}

#endif /* 0 */

/*
 * Search the symbol table of a single shared object for a symbol of
 * the given name.  Returns a pointer to the symbol, or NULL if no
 * definition was found.
 *
 * The symbol's hash value is passed in for efficiency reasons; that
 * eliminates many recomputations of the hash value.
 */
const Elf_Sym *
symlook_obj(const char *name, unsigned long hash, const Obj_Entry *obj,
  bool in_plt)
{
    if (obj->buckets != NULL) {
	unsigned long symnum = obj->buckets[hash % obj->nbuckets];

	while (symnum != STN_UNDEF) {
	    const Elf_Sym *symp;
	    const char *strp;

	    if (symnum >= obj->nchains)
		return NULL;	/* Bad object */
	    symp = obj->symtab + symnum;
	    strp = obj->strtab + symp->st_name;

	    if (name[0] == strp[0] && strcmp(name, strp) == 0)
		return symp->st_shndx != SHN_UNDEF ||
		  (!in_plt && symp->st_value != 0 &&
		  ELF_ST_TYPE(symp->st_info) == STT_FUNC) ? symp : NULL;

	    symnum = obj->chains[symnum];
	}
    }
    return NULL;
}

#if 0

static void
trace_loaded_objects(Obj_Entry *obj)
{
    char	*fmt1, *fmt2, *fmt, *main_local, *list_containers;
    int		c;

    if ((main_local = getenv(LD_ "TRACE_LOADED_OBJECTS_PROGNAME")) == NULL)
	main_local = "";

    if ((fmt1 = getenv(LD_ "TRACE_LOADED_OBJECTS_FMT1")) == NULL)
	fmt1 = "\t%o => %p (%x)\n";

    if ((fmt2 = getenv(LD_ "TRACE_LOADED_OBJECTS_FMT2")) == NULL)
	fmt2 = "\t%o (%x)\n";

    list_containers = getenv(LD_ "TRACE_LOADED_OBJECTS_ALL");

    for (; obj; obj = obj->next) {
	Needed_Entry		*needed;
	char			*name, *path;
	bool			is_lib;

	if (list_containers && obj->needed != NULL)
	    printf("%s:\n", obj->path);
	for (needed = obj->needed; needed; needed = needed->next) {
	    if (needed->obj != NULL) {
		if (needed->obj->traced && !list_containers)
		    continue;
		needed->obj->traced = true;
		path = needed->obj->path;
	    } else
		path = "not found";

	    name = (char *)obj->strtab + needed->name;
	    is_lib = strncmp(name, "lib", 3) == 0;	/* XXX - bogus */

	    fmt = is_lib ? fmt1 : fmt2;
	    while ((c = *fmt++) != '\0') {
		switch (c) {
		default:
		    putchar(c);
		    continue;
		case '\\':
		    switch (c = *fmt) {
		    case '\0':
			continue;
		    case 'n':
			putchar('\n');
			break;
		    case 't':
			putchar('\t');
			break;
		    }
		    break;
		case '%':
		    switch (c = *fmt) {
		    case '\0':
			continue;
		    case '%':
		    default:
			putchar(c);
			break;
		    case 'A':
			printf("%s", main_local);
			break;
		    case 'a':
			printf("%s", obj_main->path);
			break;
		    case 'o':
			printf("%s", name);
			break;
#if 0
		    case 'm':
			printf("%d", sodp->sod_major);
			break;
		    case 'n':
			printf("%d", sodp->sod_minor);
			break;
#endif
		    case 'p':
			printf("%s", path);
			break;
		    case 'x':
			printf("%p", needed->obj ? needed->obj->mapbase : 0);
			break;
		    }
		    break;
		}
		++fmt;
	    }
	}
    }
}

/*
 * Unload a dlopened object and its dependencies from memory and from
 * our data structures.  It is assumed that the DAG rooted in the
 * object has already been unreferenced, and that the object has a
 * reference count of 0.
 */
static void
unload_object(Obj_Entry *root)
{
    Obj_Entry *obj;
    Obj_Entry **linkp;

    assert(root->refcount == 0);

    /*
     * Pass over the DAG removing unreferenced objects from
     * appropriate lists.
     */
    unlink_object(root);

    /* Unmap all objects that are no longer referenced. */
    linkp = &obj_list->next;
    while ((obj = *linkp) != NULL) {
	if (obj->refcount == 0) {
	    dbg("unloading \"%s\"", obj->path);
	    munmap(obj->mapbase, obj->mapsize);
	    linkmap_delete(obj);
	    *linkp = obj->next;
	    obj_count--;
	    obj_free(obj);
	} else
	    linkp = &obj->next;
    }
    obj_tail = linkp;
}

static void
unlink_object(Obj_Entry *root)
{
    Objlist_Entry *elm;

    if (root->refcount == 0) {
	/* Remove the object from the RTLD_GLOBAL list. */
	objlist_remove(&list_global, root);

    	/* Remove the object from all objects' DAG lists. */
    	STAILQ_FOREACH(elm, &root->dagmembers , link) {
	    objlist_remove(&elm->obj->dldags, root);
	    if (elm->obj != root)
		unlink_object(elm->obj);
	}
    }
}

static void
ref_dag(Obj_Entry *root)
{
    Objlist_Entry *elm;

    STAILQ_FOREACH(elm, &root->dagmembers , link)
	elm->obj->refcount++;
}

static void
unref_dag(Obj_Entry *root)
{
    Objlist_Entry *elm;

    STAILQ_FOREACH(elm, &root->dagmembers , link)
	elm->obj->refcount--;
}

/*
 * Common code for MD __tls_get_addr().
 */
void *
tls_get_addr_common(Elf_Addr** dtvp, int index, size_t offset)
{
    Elf_Addr* dtv = *dtvp;

    /* Check dtv generation in case new modules have arrived */
    if (dtv[0] != tls_dtv_generation) {
	Elf_Addr* newdtv;
	int to_copy;

	newdtv = calloc(1, (tls_max_index + 2) * sizeof(Elf_Addr));
	to_copy = dtv[1];
	if (to_copy > tls_max_index)
	    to_copy = tls_max_index;
	memcpy(&newdtv[2], &dtv[2], to_copy * sizeof(Elf_Addr));
	newdtv[0] = tls_dtv_generation;
	newdtv[1] = tls_max_index;
	free(dtv);
	*dtvp = newdtv;
    }

    /* Dynamically allocate module TLS if necessary */
    if (!dtv[index + 1])
	dtv[index + 1] = (Elf_Addr)allocate_module_tls(index);

    return (void*) (dtv[index + 1] + offset);
}

/* XXX not sure what variants to use for arm. */

#if defined(__ia64__) || defined(__alpha__) || defined(__powerpc__)

/*
 * Allocate Static TLS using the Variant I method.
 */
void *
allocate_tls(Obj_Entry *objs, void *oldtls, size_t tcbsize, size_t tcbalign)
{
    Obj_Entry *obj;
    size_t size;
    char *tls;
    Elf_Addr *dtv, *olddtv;
    Elf_Addr addr;
    int i;

    size = tls_static_space;

    tls = malloc(size);
    dtv = malloc((tls_max_index + 2) * sizeof(Elf_Addr));

    *(Elf_Addr**) tls = dtv;

    dtv[0] = tls_dtv_generation;
    dtv[1] = tls_max_index;

    if (oldtls) {
	/*
	 * Copy the static TLS block over whole.
	 */
	memcpy(tls + tcbsize, oldtls + tcbsize, tls_static_space - tcbsize);

	/*
	 * If any dynamic TLS blocks have been created tls_get_addr(),
	 * move them over.
	 */
	olddtv = *(Elf_Addr**) oldtls;
	for (i = 0; i < olddtv[1]; i++) {
	    if (olddtv[i+2] < (Elf_Addr)oldtls ||
		olddtv[i+2] > (Elf_Addr)oldtls + tls_static_space) {
		dtv[i+2] = olddtv[i+2];
		olddtv[i+2] = 0;
	    }
	}

	/*
	 * We assume that all tls blocks are allocated with the same
	 * size and alignment.
	 */
	free_tls(oldtls, tcbsize, tcbalign);
    } else {
	for (obj = objs; obj; obj = obj->next) {
	    if (obj->tlsoffset) {
		addr = (Elf_Addr)tls + obj->tlsoffset;
		memset((void*) (addr + obj->tlsinitsize),
		       0, obj->tlssize - obj->tlsinitsize);
		if (obj->tlsinit)
		    memcpy((void*) addr, obj->tlsinit,
			   obj->tlsinitsize);
		dtv[obj->tlsindex + 1] = addr;
	    } else if (obj->tlsindex) {
		dtv[obj->tlsindex + 1] = 0;
	    }
	}
    }

    return tls;
}

void
free_tls(void *tls, size_t tcbsize, size_t tcbalign)
{
    size_t size;
    Elf_Addr* dtv;
    int dtvsize, i;
    Elf_Addr tlsstart, tlsend;

    /*
     * Figure out the size of the initial TLS block so that we can
     * find stuff which __tls_get_addr() allocated dynamically.
     */
    size = tls_static_space;

    dtv = ((Elf_Addr**)tls)[0];
    dtvsize = dtv[1];
    tlsstart = (Elf_Addr) tls;
    tlsend = tlsstart + size;
    for (i = 0; i < dtvsize; i++) {
	if (dtv[i+2] < tlsstart || dtv[i+2] > tlsend) {
	    free((void*) dtv[i+2]);
	}
    }

    free((void*) tlsstart);
}

#endif

#if defined(__i386__) || defined(__amd64__) || defined(__sparc64__) || \
    defined(__arm__)

/*
 * Allocate Static TLS using the Variant II method.
 */
void *
allocate_tls(Obj_Entry *objs, void *oldtls, size_t tcbsize, size_t tcbalign)
{
    Obj_Entry *obj;
    size_t size;
    char *tls;
    Elf_Addr *dtv, *olddtv;
    Elf_Addr segbase, oldsegbase, addr;
    int i;

    size = round(tls_static_space, tcbalign);

    assert(tcbsize >= 2*sizeof(Elf_Addr));
    tls = malloc(size + tcbsize);
    dtv = malloc((tls_max_index + 2) * sizeof(Elf_Addr));

    segbase = (Elf_Addr)(tls + size);
    ((Elf_Addr*)segbase)[0] = segbase;
    ((Elf_Addr*)segbase)[1] = (Elf_Addr) dtv;

    dtv[0] = tls_dtv_generation;
    dtv[1] = tls_max_index;

    if (oldtls) {
	/*
	 * Copy the static TLS block over whole.
	 */
	oldsegbase = (Elf_Addr) oldtls;
	memcpy((void *)(segbase - tls_static_space),
	       (const void *)(oldsegbase - tls_static_space),
	       tls_static_space);

	/*
	 * If any dynamic TLS blocks have been created tls_get_addr(),
	 * move them over.
	 */
	olddtv = ((Elf_Addr**)oldsegbase)[1];
	for (i = 0; i < olddtv[1]; i++) {
	    if (olddtv[i+2] < oldsegbase - size || olddtv[i+2] > oldsegbase) {
		dtv[i+2] = olddtv[i+2];
		olddtv[i+2] = 0;
	    }
	}

	/*
	 * We assume that this block was the one we created with
	 * allocate_initial_tls().
	 */
	free_tls(oldtls, 2*sizeof(Elf_Addr), sizeof(Elf_Addr));
    } else {
	for (obj = objs; obj; obj = obj->next) {
	    if (obj->tlsoffset) {
		addr = segbase - obj->tlsoffset;
		memset((void*) (addr + obj->tlsinitsize),
		       0, obj->tlssize - obj->tlsinitsize);
		if (obj->tlsinit)
		    memcpy((void*) addr, obj->tlsinit, obj->tlsinitsize);
		dtv[obj->tlsindex + 1] = addr;
	    } else if (obj->tlsindex) {
		dtv[obj->tlsindex + 1] = 0;
	    }
	}
    }

    return (void*) segbase;
}

void
free_tls(void *tls, size_t tcbsize, size_t tcbalign)
{
    size_t size;
    Elf_Addr* dtv;
    int dtvsize, i;
    Elf_Addr tlsstart, tlsend;

    /*
     * Figure out the size of the initial TLS block so that we can
     * find stuff which ___tls_get_addr() allocated dynamically.
     */
    size = round(tls_static_space, tcbalign);

    dtv = ((Elf_Addr**)tls)[1];
    dtvsize = dtv[1];
    tlsend = (Elf_Addr) tls;
    tlsstart = tlsend - size;
    for (i = 0; i < dtvsize; i++) {
	if (dtv[i+2] < tlsstart || dtv[i+2] > tlsend) {
	    free((void*) dtv[i+2]);
	}
    }

    free((void*) tlsstart);
}

#endif

/*
 * Allocate TLS block for module with given index.
 */
void *
allocate_module_tls(int index)
{
    Obj_Entry* obj;
    char* p;

    for (obj = obj_list; obj; obj = obj->next) {
	if (obj->tlsindex == index)
	    break;
    }
    if (!obj) {
	_rtld_error("Can't find module with TLS index %d", index);
	die();
    }

    p = malloc(obj->tlssize);
    memcpy(p, obj->tlsinit, obj->tlsinitsize);
    memset(p + obj->tlsinitsize, 0, obj->tlssize - obj->tlsinitsize);

    return p;
}

bool
allocate_tls_offset(Obj_Entry *obj)
{
    size_t off;

    if (obj->tls_done)
	return true;

    if (obj->tlssize == 0) {
	obj->tls_done = true;
	return true;
    }

    if (obj->tlsindex == 1)
	off = calculate_first_tls_offset(obj->tlssize, obj->tlsalign);
    else
	off = calculate_tls_offset(tls_last_offset, tls_last_size,
				   obj->tlssize, obj->tlsalign);

    /*
     * If we have already fixed the size of the static TLS block, we
     * must stay within that size. When allocating the static TLS, we
     * leave a small amount of space spare to be used for dynamically
     * loading modules which use static TLS.
     */
    if (tls_static_space) {
	if (calculate_tls_end(off, obj->tlssize) > tls_static_space)
	    return false;
    }

    tls_last_offset = obj->tlsoffset = off;
    tls_last_size = obj->tlssize;
    obj->tls_done = true;

    return true;
}

void *
_rtld_allocate_tls(void *oldtls, size_t tcbsize, size_t tcbalign)
{
    return allocate_tls(obj_list, oldtls, tcbsize, tcbalign);
}

void
_rtld_free_tls(void *tcb, size_t tcbsize, size_t tcbalign)
{
    free_tls(tcb, tcbsize, tcbalign);
}

#endif /* 0 */
