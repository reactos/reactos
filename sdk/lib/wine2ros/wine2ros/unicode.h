/*
 * PROJECT:     ReactOS Wine-To-ReactOS
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Reducing dependency on Wine
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#define memicmpW(s1,s2,n) _wcsnicmp((s1),(s2),(n))
#define strlenW(s) wcslen((s))
#define strcpyW(d,s) wcscpy((d),(s))
#define strcatW(d,s) wcscat((d),(s))
#define strcspnW(d,s) wcscspn((d),(s))
#define strstrW(d,s) wcsstr((d),(s))
#define strtolW(s,e,b) wcstol((s),(e),(b))
#define strchrW(s,c) wcschr((s),(c))
#define strrchrW(s,c) wcsrchr((s),(c))
#define strncmpW(s1,s2,n) wcsncmp((s1),(s2),(n))
#define strncpyW(s1,s2,n) wcsncpy((s1),(s2),(n))
#define strcmpW(s1,s2) wcscmp((s1),(s2))
#define strcmpiW(s1,s2) _wcsicmp((s1),(s2))
#define strncmpiW(s1,s2,n) _wcsnicmp((s1),(s2),(n))
#define strtoulW(s1,s2,b) wcstoul((s1),(s2),(b))
#define strspnW(str, accept) wcsspn((str),(accept))
#define strpbrkW(str, accept) wcspbrk((str),(accept))
#define tolowerW(n) towlower((n))
#define toupperW(n) towupper((n))
#define islowerW(n) iswlower((n))
#define isupperW(n) iswupper((n))
#define isalphaW(n) iswalpha((n))
#define isalnumW(n) iswalnum((n))
#define isdigitW(n) iswdigit((n))
#define isxdigitW(n) iswxdigit((n))
#define isspaceW(n) iswspace((n))
#define iscntrlW(n) iswcntrl((n))
#define atoiW(s) _wtoi((s))
#define atolW(s) _wtol((s))
#define strlwrW(s) _wcslwr((s))
#define struprW(s) _wcsupr((s))
#define sprintfW _swprintf
#define vsprintfW _vswprintf
#define snprintfW _snwprintf
#define vsnprintfW _vsnwprintf
#define isprintW iswprint
