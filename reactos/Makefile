.PHONY: all
.PHONY: clean
all: makefile.auto

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
else
ifeq ($(VERBOSE),yes)
  Q =
  HALFVERBOSEECHO = no
else
  Q = @
  HALFVERBOSEECHO = yes
endif
endif
ifeq ($(HALFVERBOSEECHO),yes)
  ECHO_INVOKE =@echo [INVOKE]  $<
  ECHO_PCH    =@echo [PCH]     $@
  ECHO_CC     =@echo [CC]      $<
  ECHO_GAS    =@echo [GAS]     $<
  ECHO_NASM   =@echo [NASM]    $<
  ECHO_AR     =@echo [AR]      $@
  ECHO_WINEBLD=@echo [WINEBLD] $@
  ECHO_WRC    =@echo [WRC]     $@
  ECHO_BIN2RES=@echo [BIN2RES] $<
  ECHO_DLLTOOL=@echo [DLLTOOL] $@
  ECHO_LD     =@echo [LD]      $@
  ECHO_RBUILD =@echo [RBUILD]  $@
  ECHO_RSYM   =@echo [RSYM]    $@
  ECHO_NCI    =@echo [NCI]     $@
  ECHO_CABMAN =@echo [CABMAN]  $<
  ECHO_CDMAKE =@echo [CDMAKE]  $@
  ECHO_TEST   =@echo [TEST]    $@
else
  ECHO_INVOKE =
  ECHO_PCH    =
  ECHO_CC     =
  ECHO_GAS    =
  ECHO_NASM   =
  ECHO_AR     =
  ECHO_WINEBLD=
  ECHO_WRC    =
  ECHO_BIN2RES=
  ECHO_DLLTOOL=
  ECHO_LD     =
  ECHO_RBUILD =
  ECHO_RSYM   =
  ECHO_NCI    =
  ECHO_CABMAN =
  ECHO_CDMAKE =
  ECHO_TEST   =
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
	nmkdir = $(Q)mkdir -p
	gcc = $(Q)mingw32-gcc
	gpp = $(Q)mingw32-g++
	ld = $(Q)mingw32-ld
	ar = $(Q)mingw32-ar
	objcopy = $(Q)mingw32-objcopy
	dlltool = $(Q)mingw32-dlltool
	windres = $(Q)mingw32-windres
	rm = rm -f
	cp = $(Q)cp
	NUL = /dev/null
else # mingw32-windows
	EXEPREFIX =
	EXEPOSTFIX = .exe
	SEP = \$(EMPTY_VAR)
	nmkdir = $(Q)mkdir
	gcc = $(Q)gcc
	gpp = $(Q)g++
	ld = $(Q)ld
	ar = $(Q)ar
	objcopy = $(Q)objcopy
	dlltool = $(Q)dlltool
	windres = $(Q)windres
	rm = del /f /q
	cp = $(Q)copy /y
	NUL = NUL
endif

ifneq ($(ROS_INTERMEDIATE),)
$(ROS_INTERMEDIATE):
	${nmkdir} $(ROS_INTERMEDIATE)
$(ROS_INTERMEDIATE)tools: $(ROS_INTERMEDIATE)
	${nmkdir} $@
$(ROS_INTERMEDIATE)dk: $(ROS_INTERMEDIATE)
	${nkmdir} $@
$(ROS_INTERMEDIATE)dk$(SEP)nkm: $(ROS_INTERMEDIATE)dk
	${nkmdir} $@
endif

$(ROS_INTERMEDIATE)dk$(SEP)nkm$(SEP)lib: $(ROS_INTERMEDIATE)dk$(SEP)nkm
	${nmkdir} $@

NTOSKRNL_MC = .$(SEP)ntoskrnl$(SEP)ntoskrnl.mc
KERNEL32_MC = .$(SEP)lib$(SEP)kernel32$(SEP)kernel32.mc
BUILDNO_H = .$(SEP)include$(SEP)reactos$(SEP)buildno.h
BUGCODES_H = .$(SEP)include$(SEP)reactos$(SEP)bugcodes.h
BUGCODES_RC = .$(SEP)ntoskrnl$(SEP)bugcodes.rc
ERRCODES_H = .$(SEP)include$(SEP)reactos$(SEP)errcodes.h
ERRCODES_RC = .$(SEP)lib$(SEP)kernel32$(SEP)errcodes.rc

include tools/tools.mak
-include makefile.auto

PREAUTO := \
	$(BUILDNO_H) \
	$(BUGCODES_H) \
	$(BUGCODES_RC) \
	$(ERRCODES_H) \
	$(ERRCODES_RC) \
	$(NCI_SERVICE_FILES)

makefile.auto: $(RMKDIR_TARGET) $(RBUILD_TARGET) $(PREAUTO) $(XMLBUILDFILES)
	$(ECHO_RBUILD)
	$(Q)tools$(SEP)rbuild$(SEP)rbuild$(EXEPOSTFIX) mingw


$(BUGCODES_H) $(BUGCODES_RC): $(WMC_TARGET) $(NTOSKRNL_MC)
	$(WMC_TARGET) -i -H $(BUGCODES_H) -o $(BUGCODES_RC) $(NTOSKRNL_MC)

$(ERRCODES_H) $(ERRCODES_RC): $(WMC_TARGET) $(KERNEL32_MC)
	$(WMC_TARGET) -i -H $(ERRCODES_H) -o $(ERRCODES_RC) $(KERNEL32_MC)

.PHONY: makefile_auto_clean
makefile_auto_clean:
	-@$(rm) makefile.auto $(PREAUTO) 2>$(NUL)
clean: makefile_auto_clean
