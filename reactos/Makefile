ifeq ($(HOST),mingw32-linux)
	EXEPOSTFIX =
	SEP = /
	host_gcc = gcc
	host_gpp = g++
	nmkdir = mkdir -p
else # mingw32-windows
	EXEPOSTFIX = .exe
	SEP = \$(EMPTY_VAR)
	host_gcc = gcc
	host_gpp = g++
	nmkdir = mkdir
endif

default: Makefile.auto

ifneq ($(ROS_INTERMEDIATE),)
$(ROS_INTERMEDIATE)tools: $(ROS_INTERMEDIATE)
else
$(ROS_INTERMEDIATE)tools:
endif
	${nmkdir} $(ROS_INTERMEDIATE)tools

ifneq ($(ROS_INTERMEDIATE),)
$(ROS_INTERMEDIATE):
	${nmkdir} $(ROS_INTERMEDIATE)
endif

Makefile.auto: rmkdir buildno wmc rbuild
	tools$(SEP)rbuild$(SEP)rbuild$(EXEPOSTFIX) mingw
#	$(MAKE) $(MAKECMDGOALS)

init:: \
	include$(SEP)reactos$(SEP)bugcodes.h \
	ntoskrnl$(SEP)bugcodes.rc \
	include$(SEP)reactos$(SEP)errcodes.h \
	lib$(SEP)kernel32$(SEP)errcodes.rc

include$(SEP)reactos$(SEP)bugcodes.h ntoskrnl$(SEP)bugcodes.rc: ntoskrnl$(SEP)ntoskrnl.mc
	$(WMC_TARGET) -H include$(SEP)reactos$(SEP)bugcodes.h -o ntoskrnl$(SEP)bugcodes.rc bugcodes$(SEP)ntoskrnl.mc

include$(SEP)reactos$(SEP)errcodes.h lib$(SEP)kernel32$(SEP)errcodes.rc: lib$(SEP)kernel32$(SEP)kernel32.mc
	$(WMC_TARGET) -H include$(SEP)reactos$(SEP)errcodes.h -o lib$(SEP)kernel32$(SEP)errcodes.rc lib$(SEP)kernel32$(SEP)kernel32.mc

include tools/Makefile
include tools/rbuild/makefile

#-include Makefile.auto
