#
# Important
#
.EXPORT_ALL_VARIABLES:

ifeq ($(HOST),mingw32-linux)
TOPDIR := $(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi)
endif

#
# Choose various options
#
ifeq ($(HOST),mingw32-linux)
NASM_FORMAT = win32
PREFIX = i586-mingw32-
EXE_POSTFIX = 
EXE_PREFIX = ./
#CP = cp
CP = $(PATH_TO_TOP)/rcopy
DLLTOOL = $(PREFIX)dlltool --as=$(PREFIX)as
NASM_CMD = nasm
KM_SPECS = $(TOPDIR)/specs
FLOPPY_DIR = /a
# DIST_DIR should be relative from the top of the tree
DIST_DIR = dist
endif

ifeq ($(HOST),mingw32-windows)
NASM_FORMAT = win32
PREFIX = 
EXE_POSTFIX = .exe
#CP = copy /B
CP = rcopy
DLLTOOL = $(PREFIX)dlltool --as=$(PREFIX)as
NASM_CMD = nasm
RM = del
RMDIR = rmdir
KM_SPECS = specs
DOSCLI = yes
FLOPPY_DIR = A:
# DIST_DIR should be relative from the top of the tree
DIST_DIR = dist
endif

#
# Create variables for all the compiler tools 
#
ifeq ($(WITH_DEBUGGING),yes)
DEBUGGING_CFLAGS = -g
OPTIMIZATIONS = -O2
else
DEBUGGING_CFLAGS =
OPTIMIZATIONS = -O2
endif

ifeq ($(WARNINGS_ARE_ERRORS),yes)
EXTRA_CFLAGS = -Werror
endif

DEFINES = -DDBG

ifeq ($(WIN32_LEAN_AND_MEAN),yes)
LEAN_AND_MEAN_DEFINE = -DWIN32_LEAN_AND_MEAN
else
LEAN_AND_MEAN_DEFINE = 
endif 

CC = $(PREFIX)gcc
NATIVE_CC = gcc
NATIVE_NM = nm
CFLAGS = $(BASE_CFLAGS) \
	-pipe \
	$(OPTIMIZATIONS) \
	$(LEAN_AND_MEAN_DEFINE)  \
	$(DEFINES) -Wall \
	-Wstrict-prototypes $(DEBUGGING_CFLAGS) \
	$(EXTRA_CFLAGS)
CXXFLAGS = $(CFLAGS)
NFLAGS = -i../../include/ -i../include/ -pinternal/asm.inc -f$(NASM_FORMAT) -d$(NASM_FORMAT)
LD = $(PREFIX)ld
NM = $(PREFIX)nm
OBJCOPY = $(PREFIX)objcopy
STRIP = $(PREFIX)strip
AS_INCLUDES = -I../include
AS = $(PREFIX)gcc -c -x assembler-with-cpp -D__ASM__ $(AS_BASEFLAGS) $(AS_INCLUDES)
CPP = $(PREFIX)cpp
AR = $(PREFIX)ar
RC = $(PREFIX)windres
RCINC = \
	--include-dir ../include	\
	--include-dir ../../include	\
	--include-dir ../../../include	\
	--include-dir ../../../../include

%.o: %.cc
	$(CC) $(CFLAGS) -c $< -o $@
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
%.o: %.asm
	$(NASM_CMD) $(NFLAGS) $< -o $@
%.coff: %.rc
	$(RC) $(RCINC) $< $@

%.sys: %.o
	$(CC) \
		-specs=$(PATH_TO_TOP)/services/svc_specs \
		-mdll \
		-o junk.tmp \
		-Wl,--defsym,_end=end \
		-Wl,--defsym,_edata=__data_end__ \
		-Wl,--defsym,_etext=etext \
		-Wl,--base-file,base.tmp $^
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $@ \
		--base-file base.tmp \
		--output-exp temp.exp \
		--kill-at
	- $(RM) base.tmp
	$(CC) \
		--verbose \
		-Wl,--image-base,0x10000 \
		-Wl,-e,_DriverEntry@8 \
		-Wl,temp.exp \
		-specs=$(PATH_TO_TOP)/services/svc_specs \
		-mdll \
		-o $@.unstripped \
		$^
	- $(RM) temp.exp
	$(STRIP) --strip-debug $<
	$(CC) \
		-specs=$(PATH_TO_TOP)/services/svc_specs \
		-mdll \
		-o junk.tmp \
		-Wl,--defsym,_end=end \
		-Wl,--defsym,_edata=__data_end__ \
		-Wl,--defsym,_etext=etext \
		-Wl,--base-file,base.tmp $^
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $@ \
		--base-file base.tmp \
		--output-exp temp.exp \
		--kill-at
	- $(RM) base.tmp
	$(CC) \
		--verbose \
		-Wl,--image-base,0x10000 \
		-Wl,-e,_DriverEntry@8 \
		-Wl,temp.exp \
		-specs=$(PATH_TO_TOP)/services/svc_specs \
		-mdll \
		-o $@ \
		$^
	- $(RM) temp.exp

RULES_MAK_INCLUDED = 1
