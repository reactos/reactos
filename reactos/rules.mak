#
# Important
#
.EXPORT_ALL_VARIABLES:

#HOST = mingw32-windows

# uncomment if you use bochs and it displays only 30 rows
# BOCHS_30ROWS = yes

ifeq ($(HOST),mingw32-linux)
TOPDIR := $(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi)
endif

#
# Choose various options
#
ifeq ($(HOST),mingw32-linux)
NASM_FORMAT = win32
PREFIX = i586-mingw32-
#PREFIX = /usr/mingw32-cvs-000207/bin/mingw32-cvs-000207-
EXE_POSTFIX = 
EXE_PREFIX = ./
#CP = cp
CP = $(PATH_TO_TOP)/rcopy
DLLTOOL = $(PREFIX)dlltool --as=$(PREFIX)as
NASM_CMD = nasm
#KM_SPECS = $(TOPDIR)/specs
FLOPPY_DIR = /a
# DIST_DIR should be relative from the top of the tree
DIST_DIR = dist
endif

ifeq ($(HOST),mingw32-windows)
NASM_FORMAT = win32
PREFIX = 
EXE_POSTFIX = .exe
#CP = copy /B
CP = $(PATH_TO_TOP)/rcopy
DLLTOOL = $(PREFIX)dlltool --as=$(PREFIX)as
NASM_CMD = nasm
RM = del
RMDIR = rmdir
#KM_SPECS = specs
DOSCLI = yes
FLOPPY_DIR = A:
# DIST_DIR should be relative from the top of the tree
DIST_DIR = dist
endif

CC = $(PREFIX)gcc
CXX = $(PREFIX)g++
HOST_CC = gcc
HOST_NM = nm
CFLAGS := $(CFLAGS) -I$(PATH_TO_TOP)/include -pipe
CXXFLAGS = $(CFLAGS)
NFLAGS = -i$(PATH_TO_TOP)/include/ -f$(NASM_FORMAT) -d$(NASM_FORMAT)
LD = $(PREFIX)ld
NM = $(PREFIX)nm
OBJCOPY = $(PREFIX)objcopy
STRIP = $(PREFIX)strip
ASFLAGS := $(ASFLAGS) -I$(PATH_TO_TOP)/include -D__ASM__
AS = $(PREFIX)gcc -c -x assembler-with-cpp 
CPP = $(PREFIX)cpp
AR = $(PREFIX)ar
RC = $(PREFIX)windres
RCINC = --include-dir $(PATH_TO_TOP)/include
OBJCOPY = $(PREFIX)objcopy

%.o: %.cc
	$(CC) $(CFLAGS) -c $< -o $@
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
%.o: %.S
	$(AS) $(ASFLAGS) -c $< -o $@
%.o: %.s
	$(AS) $(ASFLAGS) -c $< -o $@	
%.o: %.asm
	$(NASM_CMD) $(NFLAGS) $< -o $@
%.coff: %.rc
	$(RC) $(RCINC) $< $@

%.sys: %.o
	$(CC) \
		-nostartfiles -nostdlib -e _DriverEntry@8\
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
		-Wl,--subsystem,native \
		-Wl,--image-base,0x10000 \
		-Wl,-e,_DriverEntry@8 \
		-Wl,temp.exp \
		-nostartfiles -nostdlib -e _DriverEntry@8 \
		-mdll \
		-o $@.unstripped \
		$^
	- $(RM) temp.exp
	$(STRIP) --strip-debug $<
	$(CC) \
		-nostartfiles -nostdlib -e _DriverEntry@8 \
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
		-Wl,--subsystem,native \
		-Wl,--image-base,0x10000 \
		-Wl,-e,_DriverEntry@8 \
		-Wl,temp.exp \
		-nostartfiles -nostdlib -e _DriverEntry@8 \
		-mdll \
		-o $@ \
		$^
	- $(RM) temp.exp

RULES_MAK_INCLUDED = 1








