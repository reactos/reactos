# Default to half-verbose mode
ifeq ($(VERBOSE),no)
  Q = @
else
ifeq ($(VERBOSE),yes)
  Q =
else
  Q = @
endif
endif

export MAKE := @$(MAKE)

ifeq ($(VERBOSE),no)
# Do not print "Entering directory ..."
export MAKEFLAGS += --no-print-directory
# Be silent
export MAKEFLAGS += --silent
endif

# Windows is default host environment
ifeq ($(HOST),)
export HOST = mingw32-windows
endif

# Default to building map files which includes source and asm code
ifeq ($(FULL_MAP),)
export FULL_MAP = yes
endif

# Default to minimal dependencies, making components not
# depend on all import libraries
ifeq ($(MINIMALDEPENDENCIES),)
export MINIMALDEPENDENCIES = yes
endif

# Default to no PCH support
ifeq ($(ROS_USE_PCH),)
export ROS_USE_PCH = no
endif

# uncomment if you use bochs and it displays only 30 rows
# BOCHS_30ROWS = yes

#
# Choose various options
#
ifeq ($(HOST),mingw32-linux)
export NASM_FORMAT = win32
export PREFIX = mingw32-
export EXE_POSTFIX :=
export EXE_PREFIX := ./
export DLLTOOL = $(PREFIX)dlltool --as=$(PREFIX)as
#
# Do not change NASM_CMD to NASM because older versions of 
# nasm doesn't like an environment variable NASM
#
export NASM_CMD = nasm
export DOSCLI =
export FLOPPY_DIR = /mnt/floppy
export SEP := /
export PIPE :=
endif

ifeq ($(HOST),mingw32-windows)
export NASM_FORMAT = win32
export PREFIX =
export EXE_PREFIX :=
export EXE_POSTFIX := .exe
export DLLTOOL = $(Q)$(PREFIX)dlltool --as=$(PREFIX)as
#
# Do not change NASM_CMD to NASM because older versions of 
# nasm doesn't like an environment variable NASM
#
export NASM_CMD = $(Q)nasmw
export DOSCLI = yes
export FLOPPY_DIR = A:
export SEP := \$(EMPTY_VAR)
export PIPE := -pipe
endif

# TOPDIR is used by make bootcd but not defined anywhere.  Usurp pointed out
# that it has the same meaning as PATH_TO_TOP.
export TOPDIR = $(PATH_TO_TOP)

# Directory to build a bootable CD image in
export BOOTCD_DIR=$(TOPDIR)/../bootcd/disk
export LIVECD_DIR=$(TOPDIR)/../livecd/disk

ifeq ($(LIVECD_INSTALL),yes)
export INSTALL_DIR=$(LIVECD_DIR)/reactos
else
# Use environment var ROS_INSTALL to override default install dir
ifeq ($(ROS_INSTALL),)
ifeq ($(HOST),mingw32-windows)
export INSTALL_DIR = C:/reactos
else
export INSTALL_DIR = $(PATH_TO_TOP)/reactos
endif
else
export INSTALL_DIR = $(ROS_INSTALL)
endif
endif


export CC = $(Q)$(PREFIX)gcc
export CXX = $(Q)$(PREFIX)g++
export HOST_CC = $(Q)gcc
export HOST_CXX = $(Q)g++
export HOST_AR = $(Q)ar
export HOST_NM = $(Q)nm
export LD = $(Q)$(PREFIX)ld
export NM = $(Q)$(PREFIX)nm
export OBJCOPY = $(Q)$(PREFIX)objcopy
export STRIP = $(Q)$(PREFIX)strip
export AS = $(Q)$(PREFIX)gcc -c -x assembler-with-cpp
export CPP = $(Q)$(PREFIX)cpp
export AR = $(Q)$(PREFIX)ar
export RC = $(Q)$(PREFIX)windres
export WRC = $(Q)$(WINE_TOP)/tools/wrc/wrc
export OBJCOPY = $(Q)$(PREFIX)objcopy
export OBJDUMP =$(Q)$(PREFIX)objdump
export TOOLS_PATH = $(PATH_TO_TOP)/tools
export W32API_PATH = $(PATH_TO_TOP)/w32api
export CP = $(Q)$(TOOLS_PATH)/rcopy
export RM = $(Q)$(TOOLS_PATH)/rdel
export RLINE = $(Q)$(TOOLS_PATH)/rline
export RMDIR = $(Q)$(TOOLS_PATH)/rrmdir
export RMKDIR = $(Q)$(TOOLS_PATH)/rmkdir
export RSYM = $(Q)$(TOOLS_PATH)/rsym
export RTOUCH = $(Q)$(TOOLS_PATH)/rtouch
export REGTESTS = $(Q)$(TOOLS_PATH)/regtests
export MC = $(Q)$(TOOLS_PATH)/wmc/wmc
export CABMAN = $(Q)$(TOOLS_PATH)/cabman/cabman
export WINEBUILD = $(Q)$(TOOLS_PATH)/winebuild/winebuild
export WINE2ROS = $(Q)$(TOOLS_PATH)/wine2ros/wine2ros
export MKHIVE = $(Q)$(TOOLS_PATH)/mkhive/mkhive
export CDMAKE = $(Q)$(TOOLS_PATH)/cdmake/cdmake
export BIN2RES = $(Q)$(TOOLS_PATH)/bin2res/bin2res
export XSLTPROC = $(Q)xsltproc
export MS2PS = $(Q)$(TOOLS_PATH)/ms2ps/ms2ps

export STD_CFLAGS = -I$(PATH_TO_TOP)/include -I$(W32API_PATH)/include -pipe -march=$(OARCH) -D_M_IX86
export STD_CPPFLAGS = $(STD_CFLAGS)
# Check for 3GB 
ifeq ($(3GB), 1)
export STD_ASFLAGS = -I$(PATH_TO_TOP)/include -I$(W32API_PATH)/include -D__ASM__ -D_M_IX86 -D__3GB__
else
export STD_ASFLAGS = -I$(PATH_TO_TOP)/include -I$(W32API_PATH)/include -D__ASM__ -D_M_IX86
endif
export STD_RCFLAGS = --include-dir $(PATH_TO_TOP)/include --include-dir $(W32API_PATH)/include
export STD_NFLAGS = -f win32

# Developer Kits
export DK_PATH=$(PATH_TO_TOP)/dk
# Native and kernel mode
export DDK_PATH=$(DK_PATH)/nkm
export DDK_PATH_LIB=$(DDK_PATH)/lib
export DDK_PATH_INC=$(PATH_TO_TOP)/include
# Win32
export SDK_PATH=$(DK_PATH)/w32
export SDK_PATH_LIB=$(SDK_PATH)/lib
export SDK_PATH_INC=$(PATH_TO_TOP)/include
# POSIX+
export XDK_PATH=$(DK_PATH)/psx
export XDK_PATH_LIB=$(XDK_PATH)/lib
export XDK_PATH_INC=$(XDK_PATH)/include

# Wine Integration
export WINE_PATH=$(PATH_TO_TOP)/../wine
export WINE_PATH_LIB=$(WINE_PATH)/lib
export WINE_PATH_INC=$(WINE_PATH)/include

# Posix+ Integration
export POSIX_PATH=$(PATH_TO_TOP)/../posix
export POSIX_PATH_LIB=$(POSIX_PATH)/lib
export POSIX_PATH_INC=$(POSIX_PATH)/include

# OS/2 Integration
export OS2_PATH=$(PATH_TO_TOP)/../os2
export OS2_PATH_LIB=$(OS2_PATH)/lib
export OS2_PATH_INC=$(OS2_PATH)/include

# Other systems integration
export REGTESTS_PATH=$(PATH_TO_TOP)/regtests
export REGTESTS_PATH_INC=$(PATH_TO_TOP)/regtests/shared
