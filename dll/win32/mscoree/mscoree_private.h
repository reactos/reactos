/*
 *
 * Copyright 2008 Alistair Leslie-Hughes
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __MSCOREE_PRIVATE__
#define __MSCOREE_PRIVATE__

extern IUnknown* create_corruntimehost(void);

/* Mono 2.6 embedding */
typedef struct _MonoDomain MonoDomain;
typedef struct _MonoAssembly MonoAssembly;

extern HMODULE mono_handle;

extern void (*mono_config_parse)(const char *filename);
extern MonoAssembly* (*mono_domain_assembly_open) (MonoDomain *domain, const char *name);
extern void (*mono_jit_cleanup)(MonoDomain *domain);
extern int (*mono_jit_exec)(MonoDomain *domain, MonoAssembly *assembly, int argc, char *argv[]);
extern MonoDomain* (*mono_jit_init)(const char *file);
extern int (*mono_jit_set_trace_options)(const char* options);
extern void (*mono_set_dirs)(const char *assembly_dir, const char *config_dir);

#endif   /* __MSCOREE_PRIVATE__ */
