host_gcc = gcc
host_gpp = g++
host_ld = ld
host_ar = ar
host_objcopy = objcopy
ifeq ($(HOST),mingw32-linux)
	EXEPREFIX = ./
	EXEPOSTFIX =
	SEP = /
	nmkdir = mkdir -p
	gcc = mingw32-gcc
	gpp = mingw32-g++
	ld = mingw32-ld
	ar = mingw32-ar
	objcopy = mingw32-objcopy
	dlltool = mingw32-dlltool
	windres = mingw32-windres
	rm = rm -f
	cp = cp
	NUL = /dev/null
else # mingw32-windows
	EXEPREFIX =
	EXEPOSTFIX = .exe
	SEP = \$(EMPTY_VAR)
	nmkdir = mkdir
	gcc = gcc
	gpp = g++
	ld = ld
	ar = ar
	objcopy = objcopy
	dlltool = dlltool
	windres = windres
	rm = del /f /q
	cp = copy /y
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
include Makefile.auto

PREAUTO := \
	include$(SEP)reactos$(SEP)buildno.h \
	include$(SEP)reactos$(SEP)bugcodes.h \
	ntoskrnl$(SEP)bugcodes.rc \
	include$(SEP)reactos$(SEP)errcodes.h \
	lib$(SEP)kernel32$(SEP)errcodes.rc

Makefile.auto: $(RMKDIR_TARGET) $(BUILDNO_TARGET) $(NCI_SERVICE_FILES) $(WMC_TARGET) $(CDMAKE_TARGET) $(RBUILD_TARGET) $(PREAUTO)
	tools$(SEP)rbuild$(SEP)rbuild$(EXEPOSTFIX) mingw
#	$(MAKE) $(MAKECMDGOALS)


include$(SEP)reactos$(SEP)bugcodes.h ntoskrnl$(SEP)bugcodes.rc: $(WMC_TARGET) ntoskrnl$(SEP)ntoskrnl.mc
	$(WMC_TARGET) -H include$(SEP)reactos$(SEP)bugcodes.h -o ntoskrnl$(SEP)bugcodes.rc ntoskrnl$(SEP)ntoskrnl.mc

include$(SEP)reactos$(SEP)errcodes.h lib$(SEP)kernel32$(SEP)errcodes.rc: $(WMC_TARGET) lib$(SEP)kernel32$(SEP)kernel32.mc
	$(WMC_TARGET) -H include$(SEP)reactos$(SEP)errcodes.h -o lib$(SEP)kernel32$(SEP)errcodes.rc lib$(SEP)kernel32$(SEP)kernel32.mc

clean::
	-@$(rm) Makefile.auto $(PREAUTO) 2>$(NUL)
