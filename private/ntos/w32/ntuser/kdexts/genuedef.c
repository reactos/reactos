
LIBRARY USEREXTS

;
; This file generates userexts.def or userkdx.def depending on the
; state of KERNEL.  This allows one file (exts.h) to
; be used to generate extension exports, entrypoints, and help text.
;
; To add an extension, add the appropriate entry to exts.h and matching
; code to userexts.c
;

EXPORTS
#define DOIT(name, helpstring1, helpstring2, validflags, argtype) name
#include "exts.h"

#ifdef KERNEL
;--------------------------------------------------------------------
;
; these are the extension service functions provided for the debugger
;
;--------------------------------------------------------------------

    CheckVersion
    WinDbgExtensionDllInit
    ExtensionApiVersion
#endif
