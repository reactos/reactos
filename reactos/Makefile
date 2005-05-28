# Accepted environment variables:
#
#    ROS_PREFIX
#        This variable specifies the prefix of the MinGW installation. On Windows
#        a prefix is usually not needed, but on linux it is usually "mingw32". If
#        not present and no executable named "gcc" can be found, then the prefix is
#        assumed to be "mingw32".
#
#    ROS_INTERMEDIATE
#        This variable controls where to put intermediate files. Intermediate
#        files are generated files that are needed to generate the final
#        output files. Examples of intermediate files include *.o, *.a, and
#        *.coff. N.B. Don't put a path separator at the end. The variable
#        defaults to .\obj-i386.
#
#    ROS_OUTPUT
#        This variable controls where to put output files. Output files are
#        generated files that makes up the result of the build process.
#        Examples of output files include *.exe, *.dll, and *.sys. N.B. Don't
#        put a path separator at the end. The variable defaults to .\output-i386.
#
#    ROS_TEMPORARY
#        This variable controls where to put temporary files. Temporary files
#        are (usually small) generated files that are needed to generate the
#        intermediate or final output files. Examples of temporary files include
#        *.rci (preprocessed .rc files for wrc), *.tmp, and *.exp. N.B. Don't put
#        a path separator at the end. The variable defaults to the current
#        directory.
#
#    ROS_INSTALL
#        This variable controls where to install output files to when using
#        'make install'. N.B. Don't put a path separator at the end. The variable
#        defaults to .\reactos.
#
#    ROS_BUILDMAP
#        This variable controls if map files are to be generated for executable
#        output files. Map files have the extension .map. The value can be either
#        full (to build map files with assembly code), yes (to build map files
#        without source code) or no (to not build any map files). The variable
#        defaults to no.
#
#    ROS_BUILDNOSTRIP
#        This variable controls if non-symbol-stripped versions are to be built
#        of executable output files. Non-symbol-stripped executable output files
#        have .nostrip added to the filename just before the extension. The value
#        can be either yes (to build non-symbol-stripped versions of executable
#        output files) or no (to not build non-symbol-stripped versions of
#        executable output files). The variable defaults to no.
#
#    ROS_RBUILDFLAGS
#        Pass parameters to rbuild.

.PHONY: all
.PHONY: clean
all: makefile.auto

.SUFFIXES:

ifeq ($(HOST),)
ifeq ($(word 1,$(shell gcc -dumpmachine)),mingw32)
HOST=mingw32-windows
else
HOST=mingw32-linux
endif
endif

# Default to half-verbose mode
ifeq ($(VERBOSE),no)
  Q = @
  HALFVERBOSEECHO = no
  BUILDNO_QUIET = -q
else
ifeq ($(VERBOSE),full)
  Q =
  HALFVERBOSEECHO = no
  BUILDNO_QUIET =
else
  Q = @
  HALFVERBOSEECHO = yes
  BUILDNO_QUIET = -q
endif
endif
ifeq ($(HALFVERBOSEECHO),yes)
  ECHO_CP      =@echo [COPY]     $@
  ECHO_MKDIR   =@echo [MKDIR]    $@
  ECHO_BUILDNO =@echo [BUILDNO]  $@
  ECHO_INVOKE  =@echo [INVOKE]   $<
  ECHO_PCH     =@echo [PCH]      $@
  ECHO_CC      =@echo [CC]       $<
  ECHO_GAS     =@echo [GAS]      $<
  ECHO_NASM    =@echo [NASM]     $<
  ECHO_AR      =@echo [AR]       $@
  ECHO_WINEBLD =@echo [WINEBLD]  $@
  ECHO_WRC     =@echo [WRC]      $@
  ECHO_WIDL    =@echo [WIDL]     $@
  ECHO_BIN2RES =@echo [BIN2RES]  $<
  ECHO_DLLTOOL =@echo [DLLTOOL]  $@
  ECHO_LD      =@echo [LD]       $@
  ECHO_NM      =@echo [NM]       $@
  ECHO_OBJDUMP =@echo [OBJDUMP]  $@
  ECHO_RBUILD  =@echo [RBUILD]   $@
  ECHO_RSYM    =@echo [RSYM]     $@
  ECHO_WMC     =@echo [WMC]      $@
  ECHO_NCI     =@echo [NCI]      $@
  ECHO_CABMAN  =@echo [CABMAN]   $<
  ECHO_CDMAKE  =@echo [CDMAKE]   $@
  ECHO_MKHIVE  =@echo [MKHIVE]   $@
  ECHO_REGTESTS=@echo [REGTESTS] $@
  ECHO_TEST    =@echo [TEST]     $@
else
  ECHO_CP      =
  ECHO_MKDIR   =
  ECHO_BUILDNO =
  ECHO_INVOKE  =
  ECHO_PCH     =
  ECHO_CC      =
  ECHO_GAS     =
  ECHO_NASM    =
  ECHO_AR      =
  ECHO_WINEBLD =
  ECHO_WRC     =
  ECHO_WIDL    =
  ECHO_BIN2RES =
  ECHO_DLLTOOL =
  ECHO_LD      =
  ECHO_NM      =
  ECHO_OBJDUMP =
  ECHO_RBUILD  =
  ECHO_RSYM    =
  ECHO_WMC     =
  ECHO_NCI     =
  ECHO_CABMAN  =
  ECHO_CDMAKE  =
  ECHO_MKHIVE  =
  ECHO_REGTESTS=
  ECHO_TEST    =
endif


host_gcc = $(Q)gcc
host_gpp = $(Q)g++
host_ld = $(Q)ld
host_ar = $(Q)ar
host_objcopy = $(Q)objcopy
ifeq ($(HOST),mingw32-linux)
	EXEPREFIX = ./
	EXEPOSTFIX =
	SEP = /
	mkdir = -$(Q)mkdir -p
	gcc = $(Q)mingw32-gcc
	gpp = $(Q)mingw32-g++
	ld = $(Q)mingw32-ld
	nm = $(Q)mingw32-nm
	objdump = $(Q)mingw32-objdump
	ar = $(Q)mingw32-ar
	objcopy = $(Q)mingw32-objcopy
	dlltool = $(Q)mingw32-dlltool
	windres = $(Q)mingw32-windres
	rm = $(Q)rm -f
	cp = $(Q)cp
	NUL = /dev/null
else # mingw32-windows
	EXEPREFIX =
	EXEPOSTFIX = .exe
	ROS_EMPTY =
	SEP = \$(ROS_EMPTY)
	mkdir = -$(Q)mkdir
	gcc = $(Q)gcc
	gpp = $(Q)g++
	ld = $(Q)ld
	nm = $(Q)nm
	objdump = $(Q)objdump
	ar = $(Q)ar
	objcopy = $(Q)objcopy
	dlltool = $(Q)dlltool
	windres = $(Q)windres
	rm = $(Q)del /f /q
	cp = $(Q)copy /y
	NUL = NUL
endif

ifneq ($(ROS_INTERMEDIATE),)
  INTERMEDIATE := $(ROS_INTERMEDIATE)
else
  INTERMEDIATE := obj-i386
endif
INTERMEDIATE_ := $(INTERMEDIATE)$(SEP)

ifneq ($(ROS_OUTPUT),)
  OUTPUT := $(ROS_OUTPUT)
else
  OUTPUT := output-i386
endif
OUTPUT_ := $(OUTPUT)$(SEP)

ifneq ($(ROS_TEMPORARY),)
  TEMPORARY := $(ROS_TEMPORARY)
else
  TEMPORARY :=
endif
TEMPORARY_ := $(TEMPORARY)$(SEP)

ifneq ($(ROS_INSTALL),)
  INSTALL := $(ROS_INSTALL)
else
  INSTALL := reactos
endif
INSTALL_ := $(INSTALL)$(SEP)

$(INTERMEDIATE):
	${mkdir} $@

ifneq ($(INTERMEDIATE),$(OUTPUT))
$(OUTPUT):
	${mkdir} $@
endif


NTOSKRNL_MC = ntoskrnl$(SEP)ntoskrnl.mc
KERNEL32_MC = lib$(SEP)kernel32$(SEP)kernel32.mc
BUILDNO_H = include$(SEP)reactos$(SEP)buildno.h
BUGCODES_H = include$(SEP)reactos$(SEP)bugcodes.h
BUGCODES_RC = ntoskrnl$(SEP)bugcodes.rc
ERRCODES_H = include$(SEP)reactos$(SEP)errcodes.h
ERRCODES_RC = lib$(SEP)kernel32$(SEP)errcodes.rc

include lib/lib.mak
include tools/tools.mak
-include makefile.auto

PREAUTO := \
	$(BIN2RES_TARGET) \
	$(BUILDNO_H) \
	$(BUGCODES_H) \
	$(BUGCODES_RC) \
	$(ERRCODES_H) \
	$(ERRCODES_RC) \
	$(NCI_SERVICE_FILES)

makefile.auto: $(RBUILD_TARGET) $(PREAUTO) $(XMLBUILDFILES)
	$(ECHO_RBUILD)
	$(Q)$(RBUILD_TARGET) $(ROS_RBUILDFLAGS) mingw


$(BUGCODES_H) $(BUGCODES_RC): $(WMC_TARGET) $(NTOSKRNL_MC)
	$(ECHO_WMC)
	$(Q)$(WMC_TARGET) -i -H $(BUGCODES_H) -o $(BUGCODES_RC) $(NTOSKRNL_MC)

$(ERRCODES_H) $(ERRCODES_RC): $(WMC_TARGET) $(KERNEL32_MC)
	$(ECHO_WMC)
	$(Q)$(WMC_TARGET) -i -H $(ERRCODES_H) -o $(ERRCODES_RC) $(KERNEL32_MC)

.PHONY: makefile_auto_clean
makefile_auto_clean:
	-@$(rm) makefile.auto $(PREAUTO) 2>$(NUL)

.PHONY: clean
clean: makefile_auto_clean

.PHONY: depends
depends:
	@-$(rm) makefile.auto
	@$(MAKE) $(filter-out depends, $(MAKECMDGOALS))
