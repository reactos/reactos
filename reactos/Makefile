# Well-known targets:
#
#    all (default target)
#        This target builds all of ReactOS.
#
#    module
#        These targets builds a single module. Replace module with the name of
#        the module you want to build.
#
#    bootcd
#        This target builds an ISO (ReactOS.iso) from which ReactOS can be booted
#        and installed.
#
#    livecd
#        This target builds an ISO (ReactOS-Live.iso) from which ReactOS can be
#        booted, but not installed.
#
#    install
#        This target installs all of ReactOS to a location specified by the
#        ROS_INSTALL environment variable.
#
#    module_install
#        These targets installs a single module to a location specified by the
#        ROS_INSTALL environment variable. Replace module with the name of the
#        module you want to install.
#
#    clean
#        This target cleans (deletes) all files that are generated when building
#        ReactOS.
#
#    module_clean
#        These targets cleans (deletes) files that are generated when building a
#        single module. Replace module with the name of the module you want to
#        clean.
#
#    depends
#        This target does a complete dependency check of the ReactOS codebase.
#        This can require several minutes to complete. If you only need to check
#        dependencies for a single or few modules then you can use the
#        module_depends targets instead. This target can also repair a damaged or
#        missing makefile.auto if needed.
#
#    module_depends
#        These targets do a dependency check of individual modules. Replace module
#        with the name of the module for which you want to check dependencies.
#        This is faster than the depends target which does a complete dependency
#        check of the ReactOS codebase.
#
#
# Accepted environment variables:
#
#    ROS_PREFIX
#        This variable specifies the prefix of the MinGW installation. On Windows
#        a prefix is usually not needed, but on linux it is usually "mingw32". If
#        not present and no executable named "gcc" can be found, then the prefix is
#        assumed to be "mingw32". If your gcc is named i386-mingw32-gcc then set
#        ROS_PREFIX to i386-mingw32. Don't include the dash (-) before gcc.
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
#            -v           Be verbose.
#            -c           Clean as you go. Delete generated files as soon as they are not needed anymore.
#            -dd          Disable automatic dependencies.
#            -dm{module}  Check only automatic dependencies for this module.
#            -mi          Let make handle creation of install directories. Rbuild will not generate the directories.
#            -ps          Generate proxy makefiles in source tree instead of the output tree.

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
ifeq ($(HOST),mingw32-linux)
  QUOTE = "
else
  QUOTE =
endif
ifeq ($(HALFVERBOSEECHO),yes)
  ECHO_CP      =@echo $(QUOTE)[COPY]     $@$(QUOTE)
  ECHO_MKDIR   =@echo $(QUOTE)[MKDIR]    $@$(QUOTE)
  ECHO_BUILDNO =@echo $(QUOTE)[BUILDNO]  $@$(QUOTE)
  ECHO_INVOKE  =@echo $(QUOTE)[INVOKE]   $<$(QUOTE)
  ECHO_PCH     =@echo $(QUOTE)[PCH]      $@$(QUOTE)
  ECHO_CC      =@echo $(QUOTE)[CC]       $<$(QUOTE)
  ECHO_GAS     =@echo $(QUOTE)[GAS]      $<$(QUOTE)
  ECHO_NASM    =@echo $(QUOTE)[NASM]     $<$(QUOTE)
  ECHO_AR      =@echo $(QUOTE)[AR]       $@$(QUOTE)
  ECHO_WINEBLD =@echo $(QUOTE)[WINEBLD]  $@$(QUOTE)
  ECHO_WRC     =@echo $(QUOTE)[WRC]      $@$(QUOTE)
  ECHO_WIDL    =@echo $(QUOTE)[WIDL]     $@$(QUOTE)
  ECHO_BIN2RES =@echo $(QUOTE)[BIN2RES]  $<$(QUOTE)
  ECHO_DLLTOOL =@echo $(QUOTE)[DLLTOOL]  $@$(QUOTE)
  ECHO_LD      =@echo $(QUOTE)[LD]       $@$(QUOTE)
  ECHO_NM      =@echo $(QUOTE)[NM]       $@$(QUOTE)
  ECHO_OBJDUMP =@echo $(QUOTE)[OBJDUMP]  $@$(QUOTE)
  ECHO_RBUILD  =@echo $(QUOTE)[RBUILD]   $@$(QUOTE)
  ECHO_RSYM    =@echo $(QUOTE)[RSYM]     $@$(QUOTE)
  ECHO_WMC     =@echo $(QUOTE)[WMC]      $@$(QUOTE)
  ECHO_NCI     =@echo $(QUOTE)[NCI]      $@$(QUOTE)
  ECHO_CABMAN  =@echo $(QUOTE)[CABMAN]   $<$(QUOTE)
  ECHO_CDMAKE  =@echo $(QUOTE)[CDMAKE]   $@$(QUOTE)
  ECHO_MKHIVE  =@echo $(QUOTE)[MKHIVE]   $@$(QUOTE)
  ECHO_REGTESTS=@echo $(QUOTE)[REGTESTS] $@$(QUOTE)
  ECHO_TEST    =@echo $(QUOTE)[TEST]     $@$(QUOTE)
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
	gcc = $(Q)$(PREFIX)-gcc
	gpp = $(Q)$(PREFIX)-g++
	ld = $(Q)$(PREFIX)-ld
	nm = $(Q)$(PREFIX)-nm
	objdump = $(Q)$(PREFIX)-objdump
	ar = $(Q)$(PREFIX)-ar
	objcopy = $(Q)$(PREFIX)-objcopy
	dlltool = $(Q)$(PREFIX)-dlltool
	windres = $(Q)$(PREFIX)-windres
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
