# -------------------------------------------------------------
# Platform and build-type specific defines
# -------------------------------------------------------------

LIBOBJS=$(LIBOBJS) $(LIBOBJS_NOT96) $(LIBOBJS_96P) $(LIBOBJS_NOT97)
EXEOBJS=$(EXEOBJS) $(EXEOBJS_NOT96) $(EXEOBJS_96P) $(EXEOBJS_NOT97)
DLLOBJS=$(DLLOBJS) $(DLLOBJS_NOT96) $(DLLOBJS_96P) $(DLLOBJS_NOT97)
LIBLIBS=$(LIBLIBS) $(LIBLIBS_NOT96) $(LIBLIBS_96P) $(LIBLIBS_NOT97)
EXELIBS=$(EXELIBS) $(EXELIBS_NOT96) $(EXELIBS_96P) $(EXELIBS_NOT97)
DLLLIBS=$(DLLLIBS) $(DLLLIBS_NOT96) $(DLLLIBS_96P) $(DLLLIBS_NOT97)

!if "$(_MACHINE)" == "x86"
LIBOBJS=$(LIBOBJS) $(LIBOBJS_INTEL) $(LIBOBJS_NOTPPCMAC)
DLLOBJS=$(DLLOBJS) $(DLLOBJS_INTEL) $(DLLOBJS_NOTPPCMAC)
DLLLIBS=$(DLLLIBS) $(DLLLIBS_INTEL) $(DLLLIBS_NOTPPCMAC)
DLLXLIBS=$(DLLXLIBS) $(DLLXLIBS_INTEL) $(DLLXLIBS_NOTPPCMAC)
EXEOBJS=$(EXEOBJS) $(EXEOBJS_INTEL) $(EXEOBJS_NOTPPCMAC)
EXELIBS=$(EXELIBS) $(EXELIBS_INTEL) $(EXELIBS_NOTPPCMAC)
EXEXLIBS=$(EXEXLIBS) $(EXEXLIBS_INTEL) $(EXEXLIBS_NOTPPCMAC)

!elseif "$(_MACHINE)" == "ALPHA"
LIBOBJS=$(LIBOBJS) $(LIBOBJS_ALPHA) $(LIBOBJS_NOTPPCMAC)
DLLOBJS=$(DLLOBJS) $(DLLOBJS_ALPHA) $(DLLOBJS_NOTPPCMAC)
DLLLIBS=$(DLLLIBS) $(DLLLIBS_ALPHA) $(DLLLIBS_NOTPPCMAC)
DLLXLIBS=$(DLLXLIBS) $(DLLXLIBS_ALPHA) $(DLLXLIBS_NOTPPCMAC)
EXEOBJS=$(EXEOBJS) $(EXEOBJS_ALPHA) $(EXEOBJS_NOTPPCMAC)
EXELIBS=$(EXELIBS) $(EXELIBS_ALPHA) $(EXELIBS_NOTPPCMAC)
EXEXLIBS=$(EXEXLIBS) $(EXEXLIBS_ALPHA) $(EXEXLIBS_NOTPPCMAC)

!elseif "$(_MACHINE)" == "MIPS"
LIBOBJS=$(LIBOBJS) $(LIBOBJS_MIPS) $(LIBOBJS_NOTPPCMAC)
DLLOBJS=$(DLLOBJS) $(DLLOBJS_MIPS) $(DLLOBJS_NOTPPCMAC)
DLLLIBS=$(DLLLIBS) $(DLLLIBS_MIPS) $(DLLLIBS_NOTPPCMAC)
DLLXLIBS=$(DLLXLIBS) $(DLLXLIBS_MIPS) $(DLLXLIBS_NOTPPCMAC)
EXEOBJS=$(EXEOBJS) $(EXEOBJS_MIPS) $(EXEOBJS_NOTPPCMAC)
EXELIBS=$(EXELIBS) $(EXELIBS_MIPS) $(EXELIBS_NOTPPCMAC)
EXEXLIBS=$(EXEXLIBS) $(EXEXLIBS_MIPS) $(EXEXLIBS_NOTPPCMAC)

!elseif "$(_MACHINE)" == "PPC"
LIBOBJS=$(LIBOBJS) $(LIBOBJS_PPC) $(LIBOBJS_NOTPPCMAC)
DLLOBJS=$(DLLOBJS) $(DLLOBJS_PPC) $(DLLOBJS_NOTPPCMAC)
DLLLIBS=$(DLLLIBS) $(DLLLIBS_PPC) $(DLLLIBS_NOTPPCMAC)
DLLXLIBS=$(DLLXLIBS) $(DLLXLIBS_PPC) $(DLLXLIBS_NOTPPCMAC)
EXEOBJS=$(EXEOBJS) $(EXEOBJS_PPC) $(EXEOBJS_NOTPPCMAC)
EXELIBS=$(EXELIBS) $(EXELIBS_PPC) $(EXELIBS_NOTPPCMAC)
EXEXLIBS=$(EXEXLIBS) $(EXEXLIBS_PPC) $(EXEXLIBS_NOTPPCMAC)

!elseif "$(_MACHINE)" == "PPCMAC"
LIBOBJS=$(LIBOBJS) $(LIBOBJS_PPCMAC)
DLLOBJS=$(DLLOBJS) $(DLLOBJS_PPCMAC)
DLLLIBS=$(DLLLIBS) $(DLLLIBS_PPCMAC)
DLLXLIBS=$(DLLXLIBS) $(DLLXLIBS_PPCMAC)
EXEOBJS=$(EXEOBJS) $(EXEOBJS_PPCMAC)
EXELIBS=$(EXELIBS) $(EXELIBS_PPCMAC)
EXEXLIBS=$(EXEXLIBS) $(EXEXLIBS_PPCMAC)

!endif

# --------------------------------------------------------------
# LIBTARG
# --------------------------------------------------------------

!ifdef LIBTARG

!if "$(_NO_INCREMENTAL_LINK)" == "1"

$(LIBTARG) : $(BLDDIR)\c.rsp $(LIBOBJS)
        $(DEL) $@
!ifdef SILENTMODE
        $(ECHO) Linking $@...
!endif
        $(LIBR) $(NOLOGO) -MACHINE:$(LINKMACHINE) @<<
        -OUT:$*.lib
        $(LIBOBJS: = ^
)
<<$(KEEPFLAGS)

!else

#
# For incremental linking we create a .lib file that is just a text file that
# contains a list of all the objs that would have gone into the lib.
#
$(LIBTARG) : $(BLDDIR)\c.rsp $(LIBOBJS)
        $(DEL) $@
        $(ECHO) Incrementally Linking $@...
# The following line creates a file with the name $(LIBTARG) that contains
#  the name of each object file on a separate line. SED is used to remove
#  blank lines.
        $(SED) "/^[ ]*$$/d" << >$@
$(LIBOBJS: = ^
)
<<$(KEEPFLAGS)

!endif

clean::
        $(DEL) $(LIBTARG)

!endif

# -------------------------------------------------------------
# DLLTARG
# -------------------------------------------------------------

!ifdef DLLTARG


# How do we specify a resource-only DLL on the Mac?
!if "$(RESOURCE_ONLY)" == "1"  && "$(_MACHINE)" != "PPCMAC"
DLLENTRY=/NOENTRY
!else
!       if "$(_MACHINE)" == "PPCMAC"

DLLENTRY=$(MACDLLENTRY)
DLLMAC=-mac:NOMFILEPAD
!       else
DLLENTRY=/ENTRY:$(DLLENTRY)
DLLMAC=
!       endif
!endif

$(BLDDIR)\linkdll.rsp : $(DLLLIBS)
        @$(DEL) $(BLDDIR)\linkdll.rsp
        @$(DEL) $(BLDDIR)\linkdll.tmp
!if "$(DLLLIBS: =)" != ""
!  if "$(_NO_INCREMENTAL_LINK)" == "1"
        @!echo $** >> $(BLDDIR)\linkdll.rsp
!  else
        @!type $** >> $(BLDDIR)\linkdll.tmp
        @$(SED) "y/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/" $(BLDDIR)\linkdll.tmp > $(BLDDIR)\linkdll.rsp
        $(DEL) $(BLDDIR)\linkdll.tmp
!  endif
!else
        @echo. > $(BLDDIR)\linkdll.rsp
!endif

$(DLLTARG) : $(BLDDIR)\c.rsp $(BLDDIR)\linkdll.rsp $(DLLOBJS) $(DLLXLIBS) $(PCHFILEOBJ) $(DLLDEF)
!ifdef SILENTMODE
!  if "$(_NO_INCREMENTAL_LINK)" == "1"
        $(ECHO) Linking $@...
!  else
        $(ECHO) Incrementally Linking $@...
!  endif
!endif
        $(LINK) $(LFLAGS) $(DLLMAC) /DEF:$(DLLDEF) /DLL /OUT:$(DLLTARG) /IMPLIB:$(SDKDIR)\$(@B).lib $(DLLENTRY) @<< @$(BLDDIR)\linkdll.rsp
$(DLLOBJS: =^
)
$(DLLXLIBS: =^
)
!if "$(_DEBUG)" == "3" || "$(_DEBUG)" == "4"
$(CAPLIB)
!endif
$(PCHFILEOBJ)
<<$(KEEPFLAGS)
!if "$(_MAP)"=="1"
    $(MAPSYM) -o $*.sym $*.map
!endif
     $(SPLIT)   $@

!if "$(_MACHINE)"=="PPCMAC"
$(MACTARG) : $(DLLTARG) $(PEFTARG)
!   if "$(_MACSYMS)"=="1"
       $(DEL) $(@B).sym
       $(MAP2SYM) -n $(@R).sym $(@R).MAP
!       endif
!       if "$(_COPYMAC)"=="1"
#               MFILE requires that mrc.exe be in the executable path
                cd $(TOOLSBIN)
!           if "$(PEFTARG)"==""
                @echo MFILE copy -n  -c $(MACDLLCREATOR) -t shlb $(DLLTARG) \
                        "$(_MACFOLDER):$(MACNAME)"
                $(MFILE) copy -n  -c $(MACDLLCREATOR) -t shlb $(DLLTARG) \
                        "$(_MACFOLDER):$(MACNAME)"
!           else
                @echo MFILE copy -n  -c $(MACDLLCREATOR) -t shlb $(PEFTARG) \
                        "$(_MACFOLDER):$(MACNAME)"
                $(MFILE) copy -n  -c $(MACDLLCREATOR) -t shlb $(PEFTARG) \
                        "$(_MACFOLDER):$(MACNAME)"
                @echo MFILE copy -n  -c $(MACDLLCREATOR) -t shlb -r -b1 $(CFRGTARG) \
                        "$(_MACFOLDER):$(MACNAME)"
                $(MFILE) copy -n  -c $(MACDLLCREATOR) -t shlb -r -b1 $(CFRGTARG) \
                        "$(_MACFOLDER):$(MACNAME)"
!           endif
!           if "$(_MACSYMS)"=="1"
                $(MFILE) copy -n -c $(MACDLLCREATOR) -t TEXT $(@R).sym \
                       "$(_MACFOLDER):$(@B).sym"
!           endif
                cd $(ROOT)
                @echo. > $(MACTARG)
!   endif
!endif


clean::
        $(DEL) $(DLLTARG)
        $(DEL) $(DLLTARG:.dll=.exp)
        $(DEL) $(DLLTARG:.dll=.lib)
        $(DEL) $(DLLTARG:.dll=.map)
        $(DEL) $(DLLTARG:.dll=.ilk)
        $(DEL) $(DLLTARG:.dll=.idb)
        $(DEL) $(DLLTARG:.dll=.dbg)

!endif

# -------------------------------------------------------------
# EXETARG
# -------------------------------------------------------------

!ifdef EXETARG
!ifndef EXETYPE
!error EXETYPE must be console or windows for .exes
!endif

!if "$(_MACHINE)" == "PPCMAC"
EXEMAC=-mac:NOMFILEPAD -mac:bundle -mac:type=APPL -mac:creator=$(MACEXECREATOR)
!else
EXEMAC=
!endif

$(BLDDIR)\linkexe.rsp : $(EXELIBS)
        @$(DEL) $(BLDDIR)\linkexe.rsp
        @$(DEL) $(BLDDIR)\linkexe.tmp
!if "$(EXELIBS: =)" != ""
!  if "$(_NO_INCREMENTAL_LINK)" == "1"
        @!echo $** >> $(BLDDIR)\linkexe.rsp
!  else
        @!type $** >> $(BLDDIR)\linkexe.tmp
        @$(SED) "y/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/" $(BLDDIR)\linkexe.tmp > $(BLDDIR)\linkexe.rsp
        $(DEL) $(BLDDIR)\linkexe.tmp
!  endif
!else
        @echo. > $(BLDDIR)\linkexe.rsp
!endif

$(EXETARG) : $(BLDDIR)\c.rsp $(BLDDIR)\linkexe.rsp $(EXEOBJS) $(EXEXLIBS) $(PCHFILEOBJ)
!ifdef SILENTMODE
!  if "$(_NO_INCREMENTAL_LINK)" == "1"
        $(ECHO) Linking $@...
!  else
        $(ECHO) Incrementally Linking $@...
!  endif
!endif
        $(LINK) $(LFLAGS) $(EXEMAC) -subsystem:$(EXETYPE) /OUT:$(EXETARG) @$(BLDDIR)\linkexe.rsp @<<
$(EXEOBJS: =^
)
$(EXEXLIBS: =^
)
!if "$(_DEBUG)" == "3" || "$(_DEBUG)" == "4"
$(CAPLIB)
!endif
$(PCHFILEOBJ)
<<$(KEEPFLAGS)
        $(SPLIT)   $@

!if "$(_MACHINE)"=="PPCMAC"
$(MACTARG) : $(EXETARG)
!       if "$(_MACSYMS)"=="1"
        $(DEL) $(@B).sym
        $(MAP2SYM) -n $(@R).sym $(@R).MAP
!       endif
!   if "$(_COPYMAC)"=="1"
#       MFILE requires that mrc.exe be in the executable path
#                       (why doesn't PATH work here?)
                cd $(TOOLSBIN)
                @echo MFILE copy -n -c $(MACEXECREATOR) -t APPL $(@R).exe \
                                "$(_MACFOLDER):$(MACNAME)"
                $(MFILE) copy -n -c $(MACEXECREATOR) -t APPL $(@R).exe \
                                "$(_MACFOLDER):$(MACNAME)"
!               if "$(_MACSYMS)"=="1"
                        $(MFILE) copy -n -c $(MACSYMCREATOR) -t TEXT $(@R).sym \
                                        "$(_MACFOLDER):$(@B).sym"
!               endif
                cd $(ROOT)
                @echo. > $(MACTARG)
!       endif
!endif

clean::
        $(DEL) $(EXETARG)
        $(DEL) $(EXETARG:.exe=.map)
        $(DEL) $(EXETARG:.exe=.ilk)
        $(DEL) $(EXETARG:.exe=.dbg)

!endif

# -------------------------------------------------------------
# Browse
# -------------------------------------------------------------

!if defined(EXETARG)
BROWSETARG=$(EXETARG:.exe=.bsc)
!elseif defined(DLLTARG)
BROWSETARG=$(DLLTARG:.dll=.bsc)
!else
BROWSETARG=$(BLDROOT)\browse.bsc
!endif

browse:
        $(ECHO) /o $(BROWSETARG) /n > $(TMPDIR)\bsc1.tmp
        -DIR /s/b $(ROOT)\tools\include\*.h > $(TMPDIR)\bsc2.tmp
        $(SED) -e "s/^/\/Ei/" $(TMPDIR)\bsc2.tmp >> $(TMPDIR)\bsc1.tmp
        -DIR /s/b $(BLDROOT)\*.sbr >> $(TMPDIR)\bsc1.tmp
        $(BSCMAKE) $(NOLOGO) @$(TMPDIR)\bsc1.tmp
        $(DEL) $(TMPDIR)\bsc1.tmp
        $(DEL) $(TMPDIR)\bsc2.tmp

# -------------------------------------------------------------
# Other Targets
# -------------------------------------------------------------

beep:
        @ECHO 

clean::
        $(DELALL) $(BLDDIR)\*.*

fresh: clean depend all

FULLMAKE=$(MAKE) /$(MAKEFLAGS: =) $(NOLOGO) -f $(SRCROOT)\f3\make\master.mak _RELEASE=$(_RELEASE) SRCROOT=$(SRCROOT) $(MODULE)

fclean:
        $(FULLMAKE) TARGET=clean

fdepend:
        $(FULLMAKE) TARGET=depend

fall:
        $(FULLMAKE) TARGET=all

ffresh: fclean fdepend fall

maccopy::

#!ifdef TYPETARG
#$(TYPETARG): pdlparse
#!endif

# -------------------------------------------------------------
# c.rsp
# -------------------------------------------------------------

$(BLDDIR)\c.rsp : makefile ## $(SRCROOT)\f3\make\make1.inc $(SRCROOT)\f3\make\make2.inc
    $(COPY) << $(BLDDIR)\c.rsp > nul
/I$(INCLUDEPATH:;=
/I)
$(CFLAGS: =
)
/Fo$(BLDDIR)\
/c
<<$(KEEFLAGS)

# Make sure c.rsp gets built before the precompiled header
!ifdef PCHFILE
$(PCHPATH)\$(PCHFILE).pch : $(BLDDIR)\c.rsp
!endif

# -------------------------------------------------------------
# Depend
# -------------------------------------------------------------

depend:: $(TYPETARG)
        $(SET) INCLUDE=
!ifdef PCHFILE
!if exist($(PCHFILE).hxx)
        $(MKDEP) @<< > $(TMPDIR)\depend.tmp
-n
/I $(INCLUDEDIRS: =
/I )
-s.obj
-P $(BLDDIR)\
*.cpp
*.cxx
*.c
$(PCHFILE).hxx
<<$(KEEPFLAGS)
        $(SED) -f << $(TMPDIR)\depend.tmp > $(BLDDIR)\depend.inc
s/$(BLDDIR:\=\\)\\$(PCHFILE)\.obj :/$(PCHPATH:\=\\)\\$(PCHFILE).pch : $(BLDDIR:\=\\)\\c.rsp $(PCHFILE).hxx/
s/\.obj :/.obj : $(BLDDIR:\=\\)\\c.rsp $(PCHPATH:\=\\)\\$(PCHFILE).pch/
<<$(KEEPFLAGS)

!else   # exist(pchfile.hxx)

        $(MKDEP) @<< > $(TMPDIR)\depend.tmp
-d$(PCHFILE).hxx
-n
/I $(INCLUDEDIRS: =
/I )
-s.obj
-P $(BLDDIR)\
*.cpp
*.cxx
*.c
<<$(KEEPFLAGS)
        $(SED) -f << $(TMPDIR)\depend.tmp > $(BLDDIR)\depend.inc
s/\.obj :/.obj : $(BLDDIR:\=\\)\\c.rsp $(PCHPATH:\=\\)\\$(PCHFILE).pch/
<<$(KEEPFLAGS)
!endif  # exist(pchfile.hxx)

        $(DEL) $(TMPDIR)\depend.tmp

!else   # defined(pchfile)

        $(MKDEP) @<< > $(BLDDIR)\depend.inc
-n
/I $(INCLUDEDIRS: =
/I )
-s.obj
-P $(BLDDIR)\
*.cpp
*.cxx
*.c
<<$(KEEPFLAGS)
!endif  # defined(pchfile)

        $(MKDEP) @<< >> $(BLDDIR)\depend.inc
-n
/I $(INCLUDEDIRS: =
/I )
-s.hdl
-P $(TYPEDIR)\
*.pdl
<<$(KEEPFLAGS)

#
# site\types has too many files and we can't just do a del $(TYPETARG) because
#   the command line is too long. Therefore we build a batch file that has
#   all the del commands in it and then call it. The SED command strips out
#   del commands with no arguments.
#
!if "$(TYPETARG)" != ""
clean::
        -$(SED) "/^[ ]*del $$/d" << >$(TMPDIR)\deltypes.bat
@echo off
del $(TYPETARG: =
del )
<<KEEP
        -$(TMPDIR)\deltypes.bat $(EAT_ERRORS)
        $(DEL) $(TMPDIR)\deltypes.bat
!endif

!if exist($(BLDDIR)\depend.inc)
!include "$(BLDDIR)\depend.inc"
!endif
