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
#        missing makefile-${ROS_ARCH}.auto if needed.
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
#    ROS_ARCH
#        This variable specifies the name of the architecture to build ReactOS for.
#        The variable defaults to i386.
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
#        defaults to .\obj-{ROS_ARCH}.
#
#    ROS_OUTPUT
#        This variable controls where to put output files. Output files are
#        generated files that makes up the result of the build process.
#        Examples of output files include *.exe, *.dll, and *.sys. N.B. Don't
#        put a path separator at the end. The variable defaults to .\output-{ROS_ARCH}.
#
#    ROS_CDOUTPUT
#        This variable controls the name of the ReactOS directory on cdrom.
#        The variable defaults to reactos.
#        Warning: setting this value may lead to a not bootable/installable cdrom.
#
#    ROS_TEMPORARY
#        This variable controls where to put temporary files. Temporary files
#        are (usually small) generated files that are needed to generate the
#        intermediate or final output files. Examples of temporary files include
#        *.rci (preprocessed .rc files for wrc), *.tmp, and *.exp. N.B. Don't put
#        a path separator at the end. The variable defaults to {ROS_INTERMEDIATE}
#        directory.
#
#    ROS_INSTALL
#        This variable controls where to install output files to when using
#        'make install'. N.B. Don't put a path separator at the end. The variable
#        defaults to .\{ROS_CDOUTPUT}.
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
#    ROS_LEAN_AND_MEAN
#        This variable controls if all binaries should be stripped out of useless
#        data added by GCC/LD as well as of RSYM symbol data. Output binary size
#        will go from 80 to 40MB, memory usage from 58 to 38MB and the install CD
#        from 18 to 13MB. The variable defaults to no.
#
#    ROS_RBUILDFLAGS
#        Pass parameters to rbuild.
#            -v           Be verbose.
#            -c           Clean as you go. Delete generated files as soon as they are not needed anymore.
#            -dd          Disable automatic dependencies.
#            -da          Enable automatic dependencies.
#            -df          Enable full dependencies.
#            -dm{module}  Check only automatic dependencies for this module.
#            -hd          Disable precompiled headers.
#            -mi          Let make handle creation of install directories. Rbuild will not generate the directories.
#            -ps          Generate proxy makefiles in source tree instead of the output tree.
#            -ud          Disable compilation units.
#            -r           Input XML
#
#    ROS_AUTOMAKE
#        Alternate name of makefile-${ROS_ARCH}.auto
#
#    ROS_BUILDENGINE
#        The Build engine to be used. The variable defaults to rbuild (RBUILD_TARGET)
#

# check for versions of make that don't have features we need...
# the function "eval" is only available in 3.80+, which happens to be the minimum
# version that has the features we use...
# THIS CHECK IS BORROWED FROM THE "GMSL" PROJECT, AND IS COVERED BY THE GPL LICENSE
# YOU CAN FIND OUT MORE ABOUT GMSL - A VERY COOL PROJECT - AT:
# http://gmsl.sourceforge.net/

__gmsl_have_eval :=
__gmsl_ignore := $(eval __gmsl_have_eval := T)

ifndef __gmsl_have_eval
$(error ReactOS's makefiles use GNU Make 3.80+ features, you have $(MAKE_VERSION), you MUST UPGRADE in order to build ReactOS - Sorry)
endif
# END of code borrowed from GMSL ( http://gmsl.sourceforge.net/ )

.PHONY: all
.PHONY: clean
.PHONY: world
.PHONY: universe

ifneq ($(ROS_ARCH),)
  ARCH := $(ROS_ARCH)
else
  ARCH := i386
endif

ifeq ($(ROS_AUTOMAKE),)
  ifeq ($(ARCH),i386)
    ROS_AUTOMAKE=makefile.auto
  else
    ROS_AUTOMAKE=makefile-$(ARCH).auto
  endif
endif

all: $(ROS_AUTOMAKE)


.SUFFIXES:

ifeq ($(HOST),)
ifeq ($(word 1,$(shell gcc -dumpmachine)),mingw32)
ifeq ($(findstring msys,$(shell sh --version 2>nul)),msys)
export OSTYPE = msys
HOST=mingw32-linux
HOST_CFLAGS+=-fshort-wchar
HOST_CPPFLAGS+=-fshort-wchar
else
HOST=mingw32-windows
endif
else
HOST=mingw32-linux
HOST_CFLAGS+=-fshort-wchar
HOST_CPPFLAGS+=-fshort-wchar
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
  ECHO_CPP     =@echo $(QUOTE)[CPP]      $@$(QUOTE)
  ECHO_CC      =@echo $(QUOTE)[CC]       $<$(QUOTE)
  ECHO_HOSTCC  =@echo $(QUOTE)[HOST-CC]  $<$(QUOTE)
  ECHO_CL      =@echo $(QUOTE)[CL]       $<$(QUOTE)
  ECHO_AS      =@echo $(QUOTE)[AS]       $<$(QUOTE)
  ECHO_NASM    =@echo $(QUOTE)[NASM]     $<$(QUOTE)
  ECHO_AR      =@echo $(QUOTE)[AR]       $@$(QUOTE)
  ECHO_HOSTAR  =@echo $(QUOTE)[HOST-AR]  $@$(QUOTE)
  ECHO_WINEBLD =@echo $(QUOTE)[WINEBLD]  $@$(QUOTE)
  ECHO_WRC     =@echo $(QUOTE)[WRC]      $@$(QUOTE)
  ECHO_RC      =@echo $(QUOTE)[RC]       $@$(QUOTE)
  ECHO_CVTRES  =@echo $(QUOTE)[CVTRES]   $@$(QUOTE)
  ECHO_WIDL    =@echo $(QUOTE)[WIDL]     $@$(QUOTE)
  ECHO_BIN2RES =@echo $(QUOTE)[BIN2RES]  $<$(QUOTE)
  ECHO_DLLTOOL =@echo $(QUOTE)[DLLTOOL]  $@$(QUOTE)
  ECHO_LD      =@echo $(QUOTE)[LD]       $@$(QUOTE)
  ECHO_HOSTLD  =@echo $(QUOTE)[HOST-LD]  $@$(QUOTE)
  ECHO_LINK    =@echo $(QUOTE)[LINK]     $@$(QUOTE)
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
  ECHO_GENDIB  =@echo $(QUOTE)[GENDIB]   $@$(QUOTE)
  ECHO_STRIP   =@echo $(QUOTE)[STRIP]    $@$(QUOTE)
  ECHO_RGENSTAT=@echo $(QUOTE)[RGENSTAT] $@$(QUOTE)
  ECHO_DEPENDS =@echo $(QUOTE)[DEPENDS]  $<$(QUOTE)
else
  ECHO_CP      =
  ECHO_MKDIR   =
  ECHO_BUILDNO =
  ECHO_INVOKE  =
  ECHO_PCH     =
  ECHO_CPP     =
  ECHO_CC      =
  ECHO_HOSTCC  =
  ECHO_AS      =
  ECHO_NASM    =
  ECHO_AR      =
  ECHO_HOSTAR  =
  ECHO_WINEBLD =
  ECHO_WRC     =
  ECHO_RC      =
  ECHO_CVTRES  =
  ECHO_WIDL    =
  ECHO_BIN2RES =
  ECHO_DLLTOOL =
  ECHO_LD      =
  ECHO_HOSTLD  =
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
  ECHO_GENDIB  =
  ECHO_STRIP   =
  ECHO_RGENSTAT=
  ECHO_DEPENDS =
endif

# Set host compiler/linker
ifeq ($(HOST_CC),)
  HOST_CC = gcc
endif
ifeq ($(HOST_CPP),)
  HOST_CPP = g++
endif
host_gcc = $(Q)$(HOST_CC)
host_gpp = $(Q)$(HOST_CPP)
host_ld = $(Q)ld
host_ar = $(Q)ar
host_objcopy = $(Q)objcopy

# Set target compiler/linker
ifneq ($(ROS_PREFIX),)
  PREFIX_ := $(ROS_PREFIX)-
else
  ifeq ($(HOST),mingw32-linux)
    PREFIX_ := mingw32-
  else
    PREFIX_ :=
  endif
endif
ifeq ($(TARGET_CC),)
  TARGET_CC = $(PREFIX_)gcc
endif
ifeq ($(TARGET_CPP),)
  TARGET_CPP = $(PREFIX_)g++
endif
gcc = $(Q)$(TARGET_CC)
gpp = $(Q)$(TARGET_CPP)
gas = $(Q)$(TARGET_CC) -x assembler-with-cpp
ld = $(Q)$(PREFIX_)ld
nm = $(Q)$(PREFIX_)nm
objdump = $(Q)$(PREFIX_)objdump
ar = $(Q)$(PREFIX_)ar
objcopy = $(Q)$(PREFIX_)objcopy
dlltool = $(Q)$(PREFIX_)dlltool
strip = $(Q)$(PREFIX_)strip
windres = $(Q)$(PREFIX_)windres

# Set utilities
ifeq ($(OSTYPE),msys)
  HOST=mingw32-linux
endif
ifeq ($(HOST),mingw32-linux)
	ifeq ($(OSTYPE),msys)
		export EXEPOSTFIX = .exe
	else
		export EXEPOSTFIX =
	endif
	export SEP = /
	mkdir = -$(Q)mkdir -p
	rm = $(Q)rm -f
	cp = $(Q)cp
	NUL = /dev/null
else # mingw32-windows
	export EXEPOSTFIX = .exe
	ROS_EMPTY =
	export SEP = \$(ROS_EMPTY)
	mkdir = -$(Q)mkdir
	rm = $(Q)del /f /q
	cp = $(Q)copy /y
	NUL = NUL
endif

ifneq ($(ROS_INTERMEDIATE),)
  INTERMEDIATE := $(ROS_INTERMEDIATE)
else
  INTERMEDIATE := obj-$(ARCH)
endif
INTERMEDIATE_ := $(INTERMEDIATE)$(SEP)

ifneq ($(ROS_OUTPUT),)
  OUTPUT := $(ROS_OUTPUT)
else
  OUTPUT := output-$(ARCH)
endif
OUTPUT_ := $(OUTPUT)$(SEP)

ifneq ($(ROS_CDOUTPUT),)
  CDOUTPUT := $(ROS_CDOUTPUT)
else
  CDOUTPUT := reactos
endif
CDOUTPUT_ := $(CDOUTPUT)$(SEP)

ifneq ($(ROS_TEMPORARY),)
  TEMPORARY := $(ROS_TEMPORARY)
else
  TEMPORARY := $(INTERMEDIATE)
endif
TEMPORARY_ := $(TEMPORARY)$(SEP)

ifneq ($(ROS_INSTALL),)
  INSTALL := $(ROS_INSTALL)
else
  INSTALL := $(CDOUTPUT)
endif
INSTALL_ := $(INSTALL)$(SEP)

RBUILD_FLAGS := -rReactOS-$(ARCH).rbuild -DARCH=$(ARCH)

$(INTERMEDIATE):
	$(ECHO_MKDIR)
	${mkdir} $@

ifneq ($(INTERMEDIATE),$(OUTPUT))
$(OUTPUT):
	$(ECHO_MKDIR)
	${mkdir} $@
endif

ifneq ($(TEMPORARY),$(INTERMEDIATE))
ifneq ($(TEMPORARY),$(OUTPUT))
$(TEMPORARY):
	$(ECHO_MKDIR)
	${mkdir} $@
endif
endif

BUILDNO_H = $(INTERMEDIATE_)include$(SEP)reactos$(SEP)buildno.h

include lib/lib.mak
include tools/tools.mak
-include $(ROS_AUTOMAKE)

PREAUTO := \
	$(BIN2C_TARGET) \
	$(BIN2RES_TARGET) \
	$(BUILDNO_H) \
	$(GENDIB_DIB_FILES) \
	$(NCI_SERVICE_FILES)

ifeq ($(ARCH),powerpc)
PREAUTO += $(OFW_INTERFACE_SERVICE_FILES) $(PPCMMU_TARGETS)
endif

ifeq ($(ROS_BUILDENGINE),)
ROS_BUILDENGINE=$(RBUILD_TARGET)
endif

$(ROS_AUTOMAKE): $(ROS_BUILDENGINE) $(XMLBUILDFILES) | $(PREAUTO)
	${mkdir} $(OUTPUT_)media$(SEP)inf 2>$(NUL)
	$(ECHO_RBUILD)
	$(Q)$(ROS_BUILDENGINE) $(RBUILD_FLAGS) $(ROS_RBUILDFLAGS) mingw

world: all bootcd livecd

universe:
	$(MAKE) KDBG=1 DBG=1 \
		ROS_AUTOMAKE=makefile-$(ARCH)-kd.auto \
		ROS_INSTALL=reactos-$(ARCH)-kd \
		ROS_INTERMEDIATE=obj-$(ARCH)-kd \
		ROS_OUTPUT=output-$(ARCH)-kd \
		world
	$(MAKE) KDBG=0 DBG=1 \
		ROS_AUTOMAKE=makefile-$(ARCH)-d.auto \
		ROS_INSTALL=reactos-$(ARCH)-d \
		ROS_INTERMEDIATE=obj-$(ARCH)-d \
		ROS_OUTPUT=output-$(ARCH)-d \
		world
	$(MAKE) KDBG=0 DBG=0 \
		ROS_AUTOMAKE=makefile-$(ARCH)-r.auto \
		ROS_INSTALL=reactos-$(ARCH)-r \
		ROS_INTERMEDIATE=obj-$(ARCH)-r \
		ROS_OUTPUT=output-$(ARCH)-r \
		world

.PHONY: rgenstat
rgenstat: $(RGENSTAT_TARGET)
	$(ECHO_RGENSTAT)
	$(Q)$(RGENSTAT_TARGET) apistatus.lst apistatus.xml

.PHONY: cb
cb: $(ROS_BUILDENGINE)
	$(ECHO_RBUILD)
	$(Q)$(ROS_BUILDENGINE) $(RBUILD_FLAGS) $(ROS_RBUILDFLAGS) cb

.PHONY: msbuild
msbuild: $(ROS_BUILDENGINE)
	$(ECHO_RBUILD)
	$(Q)$(ROS_BUILDENGINE) $(RBUILD_FLAGS) $(ROS_RBUILDFLAGS) msbuild

.PHONY: msbuild_clean
msbuild_clean: $(ROS_BUILDENGINE)
	$(ECHO_RBUILD)
	$(Q)$(ROS_BUILDENGINE) $(RBUILD_FLAGS) $(ROS_RBUILDFLAGS) -c msbuild

.PHONY: depmap
depmap: $(ROS_BUILDENGINE)
	$(ECHO_RBUILD)
	$(Q)$(ROS_BUILDENGINE) $(RBUILD_FLAGS) $(ROS_RBUILDFLAGS) depmap

.PHONY: vreport
vreport:$(ROS_BUILDENGINE)
	$(ECHO_RBUILD)
	$(Q)$(ROS_BUILDENGINE) $(RBUILD_FLAGS) $(ROS_RBUILDFLAGS) vreport

.PHONY: msvc
msvc: $(ROS_BUILDENGINE)
	$(ECHO_RBUILD)
	$(Q)$(ROS_BUILDENGINE) $(RBUILD_FLAGS) $(ROS_RBUILDFLAGS) msvc

.PHONY: msvc6
msvc6: $(ROS_BUILDENGINE)
	$(ECHO_RBUILD)
	$(Q)$(ROS_BUILDENGINE) $(RBUILD_FLAGS) $(ROS_RBUILDFLAGS) -vs6.00 -voversionconfiguration msvc

.PHONY: msvc7
msvc7: $(ROS_BUILDENGINE)
	$(ECHO_RBUILD)
	$(Q)$(ROS_BUILDENGINE) $(RBUILD_FLAGS) $(ROS_RBUILDFLAGS) -vs7.00 -voversionconfiguration msvc

.PHONY: msvc71
msvc71: $(ROS_BUILDENGINE)
	$(ECHO_RBUILD)
	$(Q)$(ROS_BUILDENGINE) $(RBUILD_FLAGS) $(ROS_RBUILDFLAGS) -vs7.10 -voversionconfiguration msvc

.PHONY: msvc8
msvc8: $(ROS_BUILDENGINE)
	$(ECHO_RBUILD)
	$(Q)$(ROS_BUILDENGINE) $(RBUILD_FLAGS) $(ROS_RBUILDFLAGS) -vs8.00 -voversionconfiguration msvc

.PHONY: msvc9
msvc9: $(ROS_BUILDENGINE)
	$(ECHO_RBUILD)
	$(Q)$(ROS_BUILDENGINE) $(RBUILD_FLAGS) $(ROS_RBUILDFLAGS) -vs9.00 -voversionconfiguration msvc

.PHONY: msvc6_clean
msvc6_clean: $(ROS_BUILDENGINE)
	$(ECHO_RBUILD)
	$(Q)$(ROS_BUILDENGINE) $(RBUILD_FLAGS) $(ROS_RBUILDFLAGS) -c -vs6.00 -voversionconfiguration msvc

.PHONY: msvc7_clean
msvc7_clean: $(ROS_BUILDENGINE)
	$(ECHO_RBUILD)
	$(Q)$(ROS_BUILDENGINE) $(RBUILD_FLAGS) $(ROS_RBUILDFLAGS) -c -vs7.00 -voversionconfiguration msvc

.PHONY: msvc71_clean
msvc71_clean: $(ROS_BUILDENGINE)
	$(ECHO_RBUILD)
	$(Q)$(ROS_BUILDENGINE) $(RBUILD_FLAGS) $(ROS_RBUILDFLAGS) -c -vs7.10 -voversionconfiguration msvc

.PHONY: msvc8_clean
msvc8_clean: $(ROS_BUILDENGINE)
	$(ECHO_RBUILD)
	$(Q)$(ROS_BUILDENGINE) $(RBUILD_FLAGS) $(ROS_RBUILDFLAGS) -c -vs8.00 -voversionconfiguration msvc

.PHONY: msvc9_clean
msvc9_clean: $(ROS_BUILDENGINE)
	$(ECHO_RBUILD)
	$(Q)$(ROS_BUILDENGINE) $(RBUILD_FLAGS) $(ROS_RBUILDFLAGS) -c -vs9.00 -voversionconfiguration msvc

.PHONY: msvc_clean
msvc_clean: $(ROS_BUILDENGINE)
	$(ECHO_RBUILD)
	$(Q)$(ROS_BUILDENGINE) $(RBUILD_FLAGS) $(ROS_RBUILDFLAGS) -c msvc

.PHONY: msvc_clean_all
msvc_clean_all: $(ROS_BUILDENGINE)
	$(ECHO_RBUILD)
	$(Q)$(ROS_BUILDENGINE) $(RBUILD_FLAGS) $(ROS_RBUILDFLAGS) -c -vs6.00 -voversionconfiguration msvc
	$(Q)$(ROS_BUILDENGINE) $(RBUILD_FLAGS) $(ROS_RBUILDFLAGS) -c -vs7.00 -voversionconfiguration msvc
	$(Q)$(ROS_BUILDENGINE) $(RBUILD_FLAGS) $(ROS_RBUILDFLAGS) -c -vs7.10 -voversionconfiguration msvc
	$(Q)$(ROS_BUILDENGINE) $(RBUILD_FLAGS) $(ROS_RBUILDFLAGS) -c -vs8.10 -voversionconfiguration msvc

.PHONY: msvc7_install_debug
msvc7_install_debug: $(ROS_BUILDENGINE)
	$(ECHO_RBUILD)
	$(Q)$(ROS_BUILDENGINE) $(RBUILD_FLAGS) $(ROS_RBUILDFLAGS) -vs7.00 -vcdebug -voversionconfiguration msvc

.PHONY: msvc7_install_release
msvc7_install_release: $(ROS_BUILDENGINE)
	$(ECHO_RBUILD)
	$(Q)$(ROS_BUILDENGINE) $(RBUILD_FLAGS) $(ROS_RBUILDFLAGS) -vs7.00 -vcrelease -voversionconfiguration msvc

.PHONY: msvc7_install_speed
msvc7_install_speed: $(ROS_BUILDENGINE)
	$(ECHO_RBUILD)
	$(Q)$(ROS_BUILDENGINE) $(RBUILD_FLAGS) $(ROS_RBUILDFLAGS) -vs7.00 -vcspeed -voversionconfiguration msvc

.PHONY: msvc71_install_debug
msvc71_install_debug: $(ROS_BUILDENGINE)
	$(ECHO_RBUILD)
	$(Q)$(ROS_BUILDENGINE) $(RBUILD_FLAGS) $(ROS_RBUILDFLAGS) -vs7.10 -vcdebug -voversionconfiguration msvc

.PHONY: msvc71_install_release
msvc71_install_release: $(ROS_BUILDENGINE)
	$(ECHO_RBUILD)
	$(Q)$(ROS_BUILDENGINE) $(RBUILD_FLAGS) $(ROS_RBUILDFLAGS) -vs7.10 -vcrelease -voversionconfiguration msvc


.PHONY: msvc71_install_speed
msvc71_install_speed: $(ROS_BUILDENGINE)
	$(ECHO_RBUILD)
	$(Q)$(ROS_BUILDENGINE) $(RBUILD_FLAGS) $(ROS_RBUILDFLAGS) -vs7.10 -vcspeed -voversionconfiguration msvc

.PHONY: msvc8_install_debug
msvc8_install_debug: $(ROS_BUILDENGINE)
	$(ECHO_RBUILD)
	$(Q)$(ROS_BUILDENGINE) $(RBUILD_FLAGS) $(ROS_RBUILDFLAGS) -vs8.00 -vcdebug -voversionconfiguration msvc

.PHONY: msvc8_install_release
msvc8_install_release: $(ROS_BUILDENGINE)
	$(ECHO_RBUILD)
	$(Q)$(ROS_BUILDENGINE) $(RBUILD_FLAGS) $(ROS_RBUILDFLAGS) -vs8.00 -vcrelease -voversionconfiguration msvc

.PHONY: msvc8_install_speed
msvc8_install_speed: $(ROS_BUILDENGINE)
	$(ECHO_RBUILD)
	$(Q)$(ROS_BUILDENGINE) $(RBUILD_FLAGS) $(ROS_RBUILDFLAGS) -vs8.00 -vcspeed -voversionconfiguration msvc

.PHONY: makefile_auto_clean
makefile_auto_clean:
	-@$(rm) $(ROS_AUTOMAKE) $(PREAUTO) 2>$(NUL)

.PHONY: clean
clean: makefile_auto_clean

.PHONY: depends
depends: $(ROS_BUILDENGINE)
	$(ECHO_RBUILD)
	$(Q)$(ROS_BUILDENGINE) $(RBUILD_FLAGS) $(ROS_RBUILDFLAGS) mingw
