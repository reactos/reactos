# Default to half-verbose mode
ifeq ($(VERBOSE),no)
  Q = @
  HALFVERBOSEECHO = @:
else
ifeq ($(VERBOSE),yes)
  Q =
  HALFVERBOSEECHO = @:
else
  Q = @
  HALFVERBOSEECHO = @echo
endif
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

.PHONY: all
.PHONY: clean

all: Makefile.auto

ifneq ($(ROS_INTERMEDIATE),)
$(ROS_INTERMEDIATE)tools: $(ROS_INTERMEDIATE)
	${nmkdir} $(ROS_INTERMEDIATE)tools
endif

ifneq ($(ROS_INTERMEDIATE),)
$(ROS_INTERMEDIATE):
	${nmkdir} $(ROS_INTERMEDIATE)
endif

include tools/Makefile
-include Makefile.auto

PREAUTO := \
	include$(SEP)reactos$(SEP)buildno.h \
	include$(SEP)reactos$(SEP)bugcodes.h \
	ntoskrnl$(SEP)bugcodes.rc \
	include$(SEP)reactos$(SEP)errcodes.h \
	lib$(SEP)kernel32$(SEP)errcodes.rc

Makefile.auto: $(RMKDIR_TARGET) $(BUILDNO_TARGET) $(NCI_SERVICE_FILES) $(WMC_TARGET) $(WRC_TARGET) \
               $(RSYM_TARGET) $(CDMAKE_TARGET) $(RBUILD_TARGET) $(PREAUTO) $(XMLBUILDFILES)
	$(HALFVERBOSEECHO) [RBUILD]  makefile.auto
	$(Q)tools$(SEP)rbuild$(SEP)rbuild$(EXEPOSTFIX) mingw


include$(SEP)reactos$(SEP)bugcodes.h ntoskrnl$(SEP)bugcodes.rc: $(WMC_TARGET) ntoskrnl$(SEP)ntoskrnl.mc
	$(WMC_TARGET) -i -H include$(SEP)reactos$(SEP)bugcodes.h -o ntoskrnl$(SEP)bugcodes.rc ntoskrnl$(SEP)ntoskrnl.mc

include$(SEP)reactos$(SEP)errcodes.h lib$(SEP)kernel32$(SEP)errcodes.rc: $(WMC_TARGET) lib$(SEP)kernel32$(SEP)kernel32.mc
	$(WMC_TARGET) -i -H include$(SEP)reactos$(SEP)errcodes.h -o lib$(SEP)kernel32$(SEP)errcodes.rc lib$(SEP)kernel32$(SEP)kernel32.mc

.PHONY: makefile_auto_clean
makefile_auto_clean:
	-@$(rm) Makefile.auto $(PREAUTO) 2>$(NUL)
clean: makefile_auto_clean
