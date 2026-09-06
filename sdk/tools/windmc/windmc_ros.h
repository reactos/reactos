/*
 * PROJECT:     ReactOS host tools
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     Support header for the windmc port
 * COPYRIGHT:   Copyright 2026 Ahmed ARIF <arif193@gmail.com>
 *              Interfaces follow GNU binutils, libiberty and BFD,
 *              portions Copyright (C) Free Software Foundation, Inc.
 */

#ifndef WINDMC_ROS_H
#define WINDMC_ROS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <locale.h>

#if defined(__GNUC__) || defined(__clang__)
#define WINDMC_NORETURN __attribute__ ((__noreturn__))
#elif defined(_MSC_VER)
#define WINDMC_NORETURN __declspec(noreturn)
#else
#define WINDMC_NORETURN
#endif

#define _(String) (String)
#define N_(String) (String)
#define PACKAGE "windmc"
#define LOCALEDIR ""
#define bindtextdomain(Domain, Directory) do { } while (0)
#define textdomain(Domain) do { } while (0)

#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define WORDS_BIGENDIAN 1
#endif

#if defined(_WIN32) && !defined(__CYGWIN__)
static __inline char *
windmc_stpcpy(char *dst, const char *src)
{
    size_t len = strlen(src);
    memcpy(dst, src, len + 1);
    return dst + len;
}
#define stpcpy windmc_stpcpy
#endif

#ifdef _MSC_VER
#define WINBOOL BOOL
#endif

#define ISPRINT(c) ((c) >= 0x20 && (c) < 0x7f)

void *xmalloc (size_t);
void *xrealloc (void *, size_t);
char *xstrdup (const char *);
WINDMC_NORETURN void xexit (int);
void xmalloc_set_program_name (const char *);
void expandargv (int *, char ***);

/* Zeroed, with one slack byte: codepage_from_unicode can produce an
   unterminated string when it hits an unconvertible character.  */
void *windmc_zalloc (size_t);

struct obstack
{
  int unused;
};

#define obstack_init(ob) ((void) (ob))
#define obstack_alloc(ob, size) ((void) (ob), windmc_zalloc (size))

extern char *program_name;

WINDMC_NORETURN void fatal (const char *, ...);
void non_fatal (const char *, ...);
WINDMC_NORETURN void bfd_fatal (const char *);

void set_default_bfd_target (void);
void list_supported_targets (const char *, FILE *);
WINDMC_NORETURN void print_version (const char *);

#define REPORT_BUGS_TO ""

typedef uint64_t bfd_vma;
typedef int64_t bfd_signed_vma;
typedef uint64_t bfd_size_type;
typedef unsigned char bfd_byte;
typedef unsigned int flagword;

#define SEC_HAS_CONTENTS 0x001
#define SEC_ALLOC        0x002
#define SEC_LOAD         0x004
#define SEC_DATA         0x008

typedef struct bfd_section
{
  struct bfd_section *output_section;
  struct bfd *owner;
  bfd_size_type size;
} asection;

typedef struct bfd
{
  FILE *file;
  char *filename;
  unsigned char *data;
  bfd_size_type size;
  asection section;
} bfd;

typedef struct bfd_target
{
  int unused;
} bfd_target;

#define BFD_INIT_MAGIC 0x6d63696eu

unsigned int bfd_init (void);
void bfd_set_error_program_name (const char *);
const bfd_target *bfd_get_target_info (const char *, bfd *, bool *, int *,
				       const char **);

bfd *bfd_openw (const char *, const char *);
asection *bfd_make_section_with_flags (bfd *, const char *, flagword);
bool bfd_set_section_size (asection *, bfd_size_type);
bool bfd_set_section_contents (bfd *, asection *, const void *,
			       bfd_size_type, bfd_size_type);
bool bfd_close (bfd *);

static __inline void
bfd_putl16 (bfd_vma data, void *p)
{
  bfd_byte *addr = (bfd_byte *) p;
  addr[0] = data & 0xff;
  addr[1] = (data >> 8) & 0xff;
}

static __inline void
bfd_putb16 (bfd_vma data, void *p)
{
  bfd_byte *addr = (bfd_byte *) p;
  addr[0] = (data >> 8) & 0xff;
  addr[1] = data & 0xff;
}

static __inline void
bfd_putl32 (bfd_vma data, void *p)
{
  bfd_byte *addr = (bfd_byte *) p;
  addr[0] = data & 0xff;
  addr[1] = (data >> 8) & 0xff;
  addr[2] = (data >> 16) & 0xff;
  addr[3] = (data >> 24) & 0xff;
}

static __inline void
bfd_putb32 (bfd_vma data, void *p)
{
  bfd_byte *addr = (bfd_byte *) p;
  addr[0] = (data >> 24) & 0xff;
  addr[1] = (data >> 16) & 0xff;
  addr[2] = (data >> 8) & 0xff;
  addr[3] = data & 0xff;
}

#endif /* WINDMC_ROS_H */
