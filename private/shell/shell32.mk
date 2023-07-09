#
# Usage:        "nmake" for retail build
#               "nmake BUILD=debug" for debug build
#

#===========================================================================
# Pass-1/Pass-2 common
#===========================================================================
!ifndef LANG
LANG=usa
!endif
RESDIR=messages\$(LANG)


!ifndef VERDIR
#===========================================================================
# Pass-1 build rules
#===========================================================================

!if "$(BUILD)" == "debug"
DEFAULTVERDIR=debug32
!else
DEFAULTVERDIR=retail32
!endif

!ifndef COMMONMKFILE
COMMONMKFILE=makefile
!endif

first:
    mkdir $(DEFAULTVERDIR)
    cd $(DEFAULTVERDIR)
    -$(MAKE) /l VERDIR="$(DEFAULTVERDIR)" /f ..\$(COMMONMKFILE)
    cd ..

#
# This block makes it possible to say "nmake foo.obj"
#
.c.obj:
    cd $(DEFAULTVERDIR)
    -$(MAKE) /l VERDIR="$(DEFAULTVERDIR)" /f ..\$(COMMONMKFILE) $@
    cd ..

.c.cod:
    cd $(DEFAULTVERDIR)
    -$(MAKE) /l VERDIR="$(DEFAULTVERDIR)" /f ..\$(COMMONMKFILE) $@
    cd ..

.c._c:
    cd $(DEFAULTVERDIR)
    -$(MAKE) /l VERDIR="$(DEFAULTVERDIR)" /f ..\$(COMMONMKFILE) $@
    cd ..

{$(RESDIR)}.rc.res:
    cd $(DEFAULTVERDIR)
    -$(MAKE) /l VERDIR="$(DEFAULTVERDIR)" /f ..\$(COMMONMKFILE) $@
    cd ..

depend clean deffile:
    cd $(DEFAULTVERDIR)
    -$(MAKE) /l VERDIR="$(DEFAULTVERDIR)" /f ..\$(COMMONMKFILE) $@
    cd ..

!else # VERDIR
#===========================================================================
# Pass-2 build rules
#===========================================================================

#
# We should set NODEBUG before including win32c.mk
#
!if "$(BUILD)" != "debug"
NODEBUG=1
!endif

!ifdef INCLUDE
INCLUDE=..;$(INCLUDE)
!else
INCLUDE=..
!endif

!ifdef LIB
LIB = $(ROOT)\dev\lib;$(LIB)
!else
LIB = $(ROOT)\dev\lib
!endif

#
# We need to include WIN32C.MK with OWNMAKE to customize build rules.
#
OWNMAKE=1
!include $(ROOT)\dev\win32c.mk

#
!ifndef NODEBUG
cdebug = $(cdebug) -DDEBUG
!endif

# Figure out the correct CRT combination to use
!ifdef CRTDLL
targcvars = $(cvarsdll)
targlibs = $(LIBS) $(deflibsdll)
!else
!ifdef MULTITHREADED
targcvars = $(cvarsmt)
targlibs = $(LIBS) $(deflibsmt)
!else
targcvars = $(cvars)
targlibs = $(LIBS) $(deflibs)
!endif
!endif

# Figure out if we're making a DLL or an EXE
!ifdef MAKEDLL
target = $(PROJ).dll
!else
target = $(PROJ).exe
!endif

all: $(target)

!ifndef OBJS
OBJS = $(PROJ).obj
!endif

$(target): $(OBJS)

# Update the help file if necessary
!ifdef MAKEHELP
$(PROJ).hlp : $(PROJ).rtf
    $(hc) -n $(PROJ).hpj
!endif

# Target dependcies
targdepend = $(OBJS) $(PROJ).res
!ifdef MAKEHELP
targdepend = $(targdepend) $(PROJ).hlp
!endif
!ifdef MAKEDLL
targdepend = $(targdepend) $(PROJ).def
!endif


$(PROJ).def deffile: ..\$(PROJ).def
    cl -Tc ..\$(PROJ).def $(targcvars) -EP > $(PROJ).def

# Link the target
$(target): $(targdepend)
	set LIB=$(libpath)
# If this is a DLL, we need to make an import library
!ifdef MAKEDLL
	$(link) -lib @<<
-out:$(PROJ).lib
-def:$(PROJ).def
-merge:.rdata=.text
!ifndef NOMERGEBSS
-merge:.bss=.data
!endif
$(OBJS) pch.obj
<<
!endif
	$(link) -link @<<
$(ldebug)
$(deflflags)
-out:$(target)
-map:$(PROJ).map
!ifdef MAKEDLL
-dll
!ifdef DLLBASE
-base:$(DLLBASE)
!else
-base:0x40000000
!endif
$(PROJ).exp
!else
-base:0x400000
!endif
-merge:.rdata=.text
!ifndef NOMERGEBSS
-merge:.bss=.data
!endif
$(OBJS) pch.obj
$(targlibs)
VERSION.LIB
$(PROJ).res
<<
        $(mapsym) $(msymflags) $(PROJ).map

clean:
   del *.obj
   del *.exe
   del *.res
   del *.map
   del *.sym
   del *.hlp
   del *.pdb

#
# Pre-compled header
#
!ifndef PRIVINC
PRIVINC=$(PROJ)
!endif

$(OBJS): $(PRIVINC).pch pch.obj

pch.c:
     echo #include "$(PRIVINC).h" > $@

$(PRIVINC).pch pch.obj: pch.c ..\$(PRIVINC).h
	set INCLUDE=$(inclpath)
	$(cc) -Yc$(PRIVINC).h $(cflags) $(targcvars) $(cdebug) pch.c

# Object file dependencies
{..\$(RESDIR)}.rc.res:
	set INCLUDE=$(inclpath)
	$(rc) $(rcvars) -I..\$(RESDIR) -r -fo $@ $(cvars) ..\$(RESDIR)\$*.rc

{..}.c.obj:
	set INCLUDE=$(inclpath)
	$(cc) -Yu$(PRIVINC).h $(cflags) $(targcvars) $(cdebug) ..\$*.c

{..}.c.cod:
	set INCLUDE=$(inclpath)
	$(cc) -Yu$(PRIVINC).h $(cflags) $(targcvars) $(cdebug) ..\$*.c -Fc

{..}.c._c:
	set INCLUDE=$(inclpath)
	$(cc) -Yu$(PRIVINC).h $(cflags) $(targcvars) $(cdebug) ..\$*.c -E > $@

depend:
	set INCLUDE=$(inclpath)
        includes -n$(PRIVINC).h -S. -L. -i -e -D$(ROOT)\dev\inc -D$(ROOT)\dev\sdk\inc ..\*.c ..\*.asm >..\depend.new
        includes -S. -L. -i -e -D$(ROOT)\dev\inc -D$(ROOT)\dev\sdk\inc pch.c >>..\depend.new
        erase ..\depend.mk
        ren ..\depend.new *.mk

#===========================================================================
!endif # VERDIR

