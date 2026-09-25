/*
 * PROJECT:     ReactOS host tools
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     Minimal binutils/libiberty/BFD replacements for the windmc port
 * COPYRIGHT:   Copyright 2026 Ahmed ARIF <arif193@gmail.com>
 *              Portions Copyright (C) Free Software Foundation, Inc.
 */

#include "windmc_ros.h"

#ifndef WINDMC_VERSION
#define WINDMC_VERSION "unknown"
#endif

char *program_name = "windmc";

static void
report (const char *format, va_list args)
{
  fprintf (stderr, "%s: ", program_name);
  vfprintf (stderr, format, args);
  putc ('\n', stderr);
}

void
fatal (const char *format, ...)
{
  va_list args;

  va_start (args, format);
  report (format, args);
  va_end (args);
  xexit (1);
}

void
non_fatal (const char *format, ...)
{
  va_list args;

  va_start (args, format);
  report (format, args);
  va_end (args);
}

void
bfd_fatal (const char *msg)
{
  fatal ("%s: failed", msg);
}

void
print_version (const char *name)
{
  printf ("GNU %s (GNU Binutils) %s [ReactOS host tool]\n",
	  name, WINDMC_VERSION);
  printf ("Copyright (C) 2026 Free Software Foundation, Inc.\n");
  printf ("This program is free software; you may redistribute it under the terms of\n"
	  "the GNU General Public License version 3 or (at your option) any later version.\n"
	  "This program has absolutely no warranty.\n");
  exit (0);
}

void *
xmalloc (size_t size)
{
  void *p = malloc (size != 0 ? size : 1);

  if (p == NULL)
    fatal ("out of memory allocating %lu bytes", (unsigned long) size);
  return p;
}

void *
xrealloc (void *ptr, size_t size)
{
  void *p = realloc (ptr, size != 0 ? size : 1);

  if (p == NULL)
    fatal ("out of memory allocating %lu bytes", (unsigned long) size);
  return p;
}

void *
windmc_zalloc (size_t size)
{
  void *p = xmalloc (size + 1);

  memset (p, 0, size + 1);
  return p;
}

char *
xstrdup (const char *s)
{
  char *p = xmalloc (strlen (s) + 1);

  strcpy (p, s);
  return p;
}

void
xexit (int code)
{
  exit (code);
}

void
xmalloc_set_program_name (const char *name)
{
  (void) name;
}

void
expandargv (int *argcp, char ***argvp)
{
  (void) argcp;
  (void) argvp;
}

unsigned int
bfd_init (void)
{
  return BFD_INIT_MAGIC;
}

void
bfd_set_error_program_name (const char *name)
{
  (void) name;
}

void
set_default_bfd_target (void)
{
}

void
list_supported_targets (const char *name, FILE *f)
{
  fprintf (f, "%s: supported targets: binary (little-endian;"
	   " a -F target name containing \"big\" selects big-endian)\n",
	   name);
}

const bfd_target *
bfd_get_target_info (const char *target, bfd *abfd, bool *is_bigendian,
		     int *underscoring, const char **def_target_arch)
{
  static const bfd_target dummy_target;

  (void) abfd;
  if (underscoring != NULL)
    *underscoring = -1;
  if (is_bigendian != NULL)
    *is_bigendian = (target != NULL && strstr (target, "big") != NULL);
  if (def_target_arch != NULL)
    *def_target_arch = (target != NULL ? target : "binary");
  return &dummy_target;
}

bfd *
bfd_openw (const char *filename, const char *target)
{
  FILE *f;
  bfd *abfd;

  (void) target;
  f = fopen (filename, "wb");
  if (f == NULL)
    return NULL;
  abfd = xmalloc (sizeof (*abfd));
  memset (abfd, 0, sizeof (*abfd));
  abfd->file = f;
  abfd->filename = xstrdup (filename);
  abfd->section.owner = abfd;
  return abfd;
}

asection *
bfd_make_section_with_flags (bfd *abfd, const char *name, flagword flags)
{
  (void) name;
  (void) flags;
  return &abfd->section;
}

bool
bfd_set_section_size (asection *sec, bfd_size_type size)
{
  bfd *abfd = sec->owner;

  free (abfd->data);
  abfd->data = xmalloc (size != 0 ? (size_t) size : 1);
  memset (abfd->data, 0, (size_t) size);
  abfd->size = size;
  sec->size = size;
  return true;
}

bool
bfd_set_section_contents (bfd *abfd, asection *sec, const void *data,
			  bfd_size_type offset, bfd_size_type count)
{
  if (sec != &abfd->section
      || abfd->data == NULL
      || offset > abfd->size
      || count > abfd->size - offset)
    return false;
  memcpy (abfd->data + offset, data, (size_t) count);
  return true;
}

bool
bfd_close (bfd *abfd)
{
  bool ok = true;

  if (abfd->size != 0
      && fwrite (abfd->data, 1, (size_t) abfd->size, abfd->file) != abfd->size)
    ok = false;
  if (fclose (abfd->file) != 0)
    ok = false;
  if (!ok)
    fatal ("error writing `%s'", abfd->filename);
  free (abfd->data);
  free (abfd->filename);
  free (abfd);
  return true;
}
