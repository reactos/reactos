!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\f3\crt=)
MODULE=crt
INCLUDEDIRS=$(TYPEDIR) $(TOOLSINCLUDE) $(TOOLSINCLUDE)\sys $(TOOLSMISCINCLUDE) $(ROOT)\src\core\include

!include $(ROOT)\src\f3\make\make1.inc

_WARNINGLEVEL=3
LIBTARG=$(BLDDIR)\crt.lib
LIBOBJS= \
         $(BLDDIR)\sbheap.obj  \
         $(BLDDIR)\bsearch.obj  \
         $(BLDDIR)\qsort.obj    \
         $(BLDDIR)\lfind.obj    \
         $(BLDDIR)\wcsrev.obj   \
         $(BLDDIR)\wcspbrk.obj  \
         $(BLDDIR)\wcsrchr.obj  \
         $(BLDDIR)\memmove.obj  \
         $(BLDDIR)\strrchr.obj  \
         $(BLDDIR)\wcstok.obj   \
         $(BLDDIR)\wcsspn.obj   \
         $(BLDDIR)\wcsstr.obj   \
         $(BLDDIR)\wcscmp.obj   \
         $(BLDDIR)\wcscat.obj   \
         $(BLDDIR)\wcslen.obj   \
         $(BLDDIR)\wcschr.obj   \
         $(BLDDIR)\wcsncpy.obj  \
         $(BLDDIR)\strchr.obj   \
         $(BLDDIR)\strncmp.obj  \
         $(BLDDIR)\strtol.obj   \
         $(BLDDIR)\wcstol.obj   \
         $(BLDDIR)\xtow.obj     \
         $(BLDDIR)\wtox.obj     \
         $(BLDDIR)\dllcrt0.obj  \
         $(BLDDIR)\atexit.obj   \
         $(BLDDIR)\crt0dat.obj  \
         $(BLDDIR)\xtoa.obj     \
         $(BLDDIR)\atox.obj     \
         $(BLDDIR)\strutil.obj  \
         $(SRCROOT)\f3\crt\$(_MACHINE)\chkstk.obj     \
         $(SRCROOT)\f3\crt\$(_MACHINE)\exsup.obj      \
!if "$(_MACHINE)" == "x86"
         $(SRCROOT)\f3\crt\$(_MACHINE)\exsup3.obj     \
         $(SRCROOT)\f3\crt\$(_MACHINE)\llmul.obj     \
!endif
!if "$(_DEBUG)" == "1" || "$(_DEBUG)" == "4"
         $(SRCROOT)\f3\crt\$(_MACHINE)\rtti.obj        \
         $(SRCROOT)\f3\crt\$(_MACHINE)\typname.obj     \
         $(SRCROOT)\f3\crt\$(_MACHINE)\undname.obj     \
         $(SRCROOT)\f3\crt\$(_MACHINE)\typinfo.obj     \
         $(SRCROOT)\f3\crt\$(_MACHINE)\stdexcpt.obj    \
         $(BLDDIR)\ti_utils.obj            \
!endif

all: $(LIBTARG)


!include $(MAKE2_INC)


!endif		# NTMAKEENV
