/*
 * C++ exception handling facility
 *
 * Copyright 2000 Francois Gouget.
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
#ifndef __WINE_EH_H
#define __WINE_EH_H

#include <corecrt.h>

#ifndef __cplusplus
#error "eh.h is meant only for C++ applications"
#endif

#include <pshpack8.h>

struct _EXCEPTION_POINTERS;

typedef void (__cdecl *terminate_handler)(void);
typedef void (__cdecl *terminate_function)(void);
typedef void (__cdecl *unexpected_handler)(void);
typedef void (__cdecl *unexpected_function)(void);
typedef void (__cdecl *_se_translator_function)(unsigned int code, struct _EXCEPTION_POINTERS *info);

_ACRTIMP terminate_function __cdecl set_terminate(terminate_function func);
_ACRTIMP unexpected_function __cdecl set_unexpected(unexpected_function func);
_ACRTIMP _se_translator_function __cdecl set_se_translator(_se_translator_function func);

_ACRTIMP DECLSPEC_NORETURN void __cdecl terminate(void);
_ACRTIMP DECLSPEC_NORETURN void __cdecl unexpected(void);

#include <poppack.h>

#endif /* __WINE_EH_H */
