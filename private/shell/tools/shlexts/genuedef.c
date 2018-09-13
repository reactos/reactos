
LIBRARY SHLEXTS

; DESCRIPTION is obsolete and gives the IA64 linker the heebie-jeebies
;DESCRIPTION 'SHELL Debugger Extensions'

;
; This file generates shlexts.def. This allows one file (exts.h) to
; be used to generate extension exports, entrypoints, and help text.
;
; To add an extension, add the appropriate entry to exts.h and matching
; code to shlexts.c
;

//#ifdef _FE_
//#define FE_IME 1
//#endif

EXPORTS
#define DOIT(name, helpstring1, helpstring2, validflags, argtype) name
#include "exts.h"

