/*
 * PROJECT:     ReactOS SDK
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     dllimport-slot aliases for llvm-mingw runtime references
 * COPYRIGHT:   Copyright 2026 Ahmed ARIF <arif.ing@outlook.com>
 */

/*
 * llvm-mingw's runtime objects reference these CRT functions through dllimport slots. Resolving the slots
 * from the ucrtbase import library pulls import thunks that collide with the static definitions in
 * libmingwex/msvcrtex (lld: "<sym> was replaced"), so bind the slots to the static definitions instead.
 */

#include <imp_alias.h>

IMP_ALIAS_CDECL(btowc);
IMP_ALIAS_CDECL(mbrtowc);
IMP_ALIAS_CDECL(wcrtomb);
IMP_ALIAS_CDECL(wctob);
