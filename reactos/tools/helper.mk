# $Id: helper.mk,v 1.62 2004/05/17 19:45:10 gvg Exp $
#
# Helper makefile for ReactOS modules
# Variables this makefile accepts:
#   $TARGET_TYPE       = Type of target:
#                        program = User mode program
#                        proglib = Executable program that have exported functions
#                        dynlink = Dynamic Link Library (DLL)
#                        library = Library that will be linked with other code
#                        driver = Kernel mode driver
#                        export_driver = Kernel mode driver that have exported functions
#                        driver_library = Import library for a driver
#                        kmlibrary = Static kernel-mode library
#                        hal = Hardware Abstraction Layer
#                        bootpgm = Boot program
#                        miniport = Kernel mode driver that does not link with ntoskrnl.exe or hal.dll
#                        gdi_driver = Kernel mode graphics driver that link with win32k.sys
#                        subsystem = Kernel subsystem
#                        kmdll = Kernel mode DLL
#                        winedll = DLL imported from wine
#   $TARGET_APPTYPE    = Application type (windows,native,console).
#                        Required only for TARGET_TYPEs program and proglib
#   $TARGET_NAME       = Base name of output file and .rc, .def, and .edf files
#   $TARGET_OBJECTS    = Object files that compose the module
#   $TARGET_CPPAPP     = C++ application (no,yes) (optional)
#   $TARGET_HEADERS    = Header files that the object files depend on (optional)
#   $TARGET_DEFNAME    = Base name of .def and .edf files (optional)
#   $TARGET_BASENAME   = Base name of output file (overrides $TARGET_NAME if it exists) (optional)
#   $TARGET_EXTENSION  = Extension of the output file (optional)
#   $TARGET_DDKLIBS    = DDK libraries that are to be imported by the module (optional)
#   $TARGET_SDKLIBS    = SDK libraries that are to be imported by the module (optional)
#   $TARGET_LIBS       = Other libraries that are to be imported by the module (optional)
#   $TARGET_GCCLIBS    = GCC libraries imported with -l (optional)
#   $TARGET_LFLAGS     = GCC flags when linking (optional)
#   $TARGET_CFLAGS     = GCC flags (optional)
#   $TARGET_CPPFLAGS   = G++ flags (optional)
#   $TARGET_ASFLAGS    = GCC assembler flags (optional)
#   $TARGET_NFLAGS     = NASM flags (optional)
#   $TARGET_RCFLAGS    = Windres flags (optional)
#   $TARGET_CLEAN      = Files that are part of the clean rule (optional)
#   $TARGET_PATH       = Relative path for *.def, *.edf, and *.rc (optional)
#   $TARGET_BASE       = Default base address (optional)
#   $TARGET_ENTRY      = Entry point (optional)
#   $TARGET_DEFONLY    = Use .def instead of .edf extension (no,yes) (optional)
#   $TARGET_NORC       = Do not include standard resource file (no,yes) (optional)
#   $TARGET_LIBPATH    = Destination path for static libraries (optional)
#   $TARGET_IMPLIBPATH = Destination path for import libraries (optional)
#   $TARGET_INSTALLDIR = Destination path when installed (optional)
#   $TARGET_PCH        = Filename of header to use to generate a PCH if supported by the compiler (optional)
#   $TARGET_BOOTSTRAP  = Whether this file is needed to bootstrap the installation (no,yes) (optional)
#   $TARGET_BOOTSTRAP_NAME = Name on the installation medium (optional)
#   $TARGET_REGTESTS   = This module has regression tests (no,yes) (optional)
#   $WINE_MODE         = Compile using WINE headers (no,yes) (optional)
#   $WINE_RC           = Name of .rc file for WINE modules (optional)
#   $SUBDIRS           = Subdirs in which to run make (optional)

include $(PATH_TO_TOP)/config


ifeq ($(TARGET_PATH),)
TARGET_PATH := .
endif

ifeq ($(ARCH),i386)
 MK_ARCH_ID := _M_IX86
endif

ifeq ($(ARCH),alpha)
 MK_ARCH_ID := _M_ALPHA
endif

ifeq ($(ARCH),mips)
 MK_ARCH_ID := _M_MIPS
endif

ifeq ($(ARCH),powerpc)
 MK_ARCH_ID := _M_PPC
endif

# unknown architecture
ifeq ($(MK_ARCH_ID),)
 MK_ARCH_ID := _M_UNKNOWN
endif

#
# VARIABLES IN USE BY VARIOUS TARGETS
#
# MK_BOOTCDDIR     = Directory on the ReactOS ISO CD in which to place the file (subdir of reactos/)
# MK_CFLAGS        = C compiler command-line flags for this target
# MK_CPPFLAGS      = C++ compiler command-line flags for this target
# MK_DDKLIBS       = Import libraries from the ReactOS DDK to link with
# MK_DEFENTRY      = Module entry point: 
#                    _WinMain@16 for windows EXE files that are export libraries
#                    _DriverEntry@8 for .SYS files 
#                    _DllMain@12 for .DLL files
#                    _DrvEnableDriver@12 for GDI drivers
#                    _WinMainCRTStartup for Win32 EXE files 
#                    _NtProcessStartup@4 for Native EXE files
#                    _mainCRTStartup for Console EXE files
# MK_DEFEXT        = Extension to give compiled modules (.EXE, .DLL, .SYS, .a)
# MK_DISTDIR       = (unused?)
# MK_EXETYPE       = Compiler option packages based on type of PE file (exe, dll)
# MK_IMPLIB        = Whether or not to generate a DLL import stub library (yes, no)
# MK_IMPLIB_EXT    = Extension to give import libraries (.a always)
# MK_IMPLIBDEFPATH = Default place to put the import stub library when built
# MK_IMPLIBONLY    = Whether the target is only an import library (yes, no; used only by generic hal)
# MK_INSTALLDIR    = Where "make install" should put the target, relative to reactos/
# MK_MODE          = Mode the target's code is intended to run in
#                    user - User-mode compiler settings
#                    kernel - Kernel-mode compiler settings
#                    static - Static library compiler settings
# MK_RCFLAGS       = Flags to add to resource compiler command line
# MK_RES_BASE      = Base name of resource files
# MK_SDKLIBS       = Default SDK libriaries to link with
#

ifeq ($(TARGET_TYPE),program)
  MK_MODE := user
  MK_EXETYPE := exe
  MK_DEFEXT := .exe
  MK_DEFENTRY := _DEFINE_TARGET_APPTYPE
  MK_DDKLIBS :=
  MK_SDKLIBS :=
ifneq ($(WINE_MODE),yes)
  MK_CFLAGS := -I.
  MK_CPPFLAGS := -I.
else
  MK_CFLAGS := -I$(PATH_TO_TOP)/include/wine
  MK_CPPFLAGS := -I$(PATH_TO_TOP)/include/wine
  MK_RCFLAGS := --include-dir $(PATH_TO_TOP)/include/wine --include-dir $(WINE_INCLUDE)
endif
  MK_IMPLIB := no
  MK_IMPLIBONLY := no
  MK_IMPLIBDEFPATH :=
  MK_IMPLIB_EXT := .a
  MK_INSTALLDIR := bin
  MK_BOOTCDDIR := system32
  MK_DISTDIR := apps
ifeq ($(WINE_RC),)
  MK_RES_BASE := $(TARGET_NAME)
else
  MK_RES_BASE := $(WINE_RC)
endif
endif

ifeq ($(TARGET_TYPE),proglib)
  MK_MODE := user
  MK_EXETYPE := dll
  MK_DEFEXT := .exe
  MK_DEFENTRY := _WinMain@16
  MK_DDKLIBS :=
  MK_SDKLIBS :=
  MK_CFLAGS := -I.
  MK_CPPFLAGS := -I.
  MK_IMPLIB := yes
  MK_IMPLIBONLY := no
  MK_IMPLIBDEFPATH := $(SDK_PATH_LIB)
  MK_IMPLIB_EXT := .a
  MK_INSTALLDIR := bin
  MK_BOOTCDDIR := system32
  MK_DISTDIR := apps
  MK_RES_BASE := $(TARGET_NAME)
endif

ifeq ($(TARGET_TYPE),dynlink)
  MK_MODE := user
  MK_EXETYPE := dll
  MK_DEFEXT := .dll
  MK_DEFENTRY := _DllMain@12
  MK_DDKLIBS :=
  MK_SDKLIBS :=
ifneq ($(WINE_MODE),yes)
  MK_CFLAGS := -I.
  MK_CPPFLAGS := -I.
else
  MK_CFLAGS := -I$(PATH_TO_TOP)/include/wine -I. -I$(WINE_INCLUDE)
  MK_CPPFLAGS := -I$(PATH_TO_TOP)/include/wine -I. -I$(WINE_INCLUDE)
  MK_RCFLAGS := --include-dir $(PATH_TO_TOP)/include/wine --include-dir $(WINE_INCLUDE)
endif
  MK_IMPLIB := yes
  MK_IMPLIBONLY := no
  MK_IMPLIBDEFPATH := $(SDK_PATH_LIB)
  MK_IMPLIB_EXT := .a
  MK_INSTALLDIR := system32
  MK_BOOTCDDIR := system32
  MK_DISTDIR := dlls
ifeq ($(WINE_RC),)
  MK_RES_BASE := $(TARGET_NAME)
else
  MK_RES_BASE := $(WINE_RC)
endif
endif

ifeq ($(TARGET_TYPE),library)
  TARGET_NORC := yes
  MK_MODE := static
  MK_EXETYPE :=
  MK_DEFEXT := .a
  MK_DEFENTRY :=
  MK_DDKLIBS :=
  MK_SDKLIBS :=
  MK_CFLAGS := -I.
  MK_CPPFLAGS := -I.
  MK_IMPLIB := no
  MK_IMPLIBONLY := no
  MK_IMPLIBDEFPATH :=
  MK_IMPLIB_EXT :=
  MK_INSTALLDIR := # none
  MK_BOOTCDDIR := system32
  MK_DISTDIR := # none
  MK_RES_BASE :=
endif

ifeq ($(TARGET_TYPE),kmlibrary)
  TARGET_NORC := yes
  MK_MODE := static
  MK_DEFEXT := .a
  MK_CFLAGS := -I.
  MK_CPPFLAGS := -I.
  MK_IMPLIB := no
  MK_IMPLIBONLY := no
  MK_IMPLIBDEFPATH := $(DDK_PATH_LIB)
  #MK_IMPLIB_EXT :=
endif
ifeq ($(TARGET_TYPE),driver_library)
  MK_MODE := kernel
  MK_EXETYPE := dll
  MK_DEFEXT := .dll
  MK_DEFENTRY :=
  MK_DDKLIBS :=
  MK_SDKLIBS :=
  MK_CFLAGS := -I.
  MK_CPPFLAGS := -I.
  MK_IMPLIB := no
  MK_IMPLIBONLY := yes
  MK_IMPLIBDEFPATH := $(DDK_PATH_LIB)
  MK_IMPLIB_EXT := .a
  MK_INSTALLDIR := $(DDK_PATH_INC)
  MK_BOOTCDDIR := .
  MK_DISTDIR := # FIXME
  MK_RES_BASE :=
endif

ifeq ($(TARGET_TYPE),driver)
  MK_MODE := kernel
  MK_EXETYPE := dll
  MK_DEFEXT := .sys
  MK_DEFENTRY := _DriverEntry@8
  MK_DDKLIBS := ntoskrnl.a hal.a
  MK_SDKLIBS :=
  MK_CFLAGS := -D__NTDRIVER__ -I.
  MK_CPPFLAGS := -D__NTDRIVER__ -I.
  MK_IMPLIB := no
  MK_IMPLIBONLY := no
  MK_IMPLIBDEFPATH :=
  MK_IMPLIB_EXT := .a
  MK_INSTALLDIR := system32/drivers
  MK_BOOTCDDIR := .
  MK_DISTDIR := drivers
  MK_RES_BASE := $(TARGET_NAME)
endif

ifeq ($(TARGET_TYPE),export_driver)
  MK_MODE := kernel
  MK_EXETYPE := dll
  MK_DEFEXT := .sys
  MK_DEFENTRY := _DriverEntry@8
  MK_DDKLIBS := ntoskrnl.a hal.a
  MK_SDKLIBS :=
  MK_CFLAGS := -D__NTDRIVER__ -I.
  MK_CPPFLAGS := -D__NTDRIVER__ -I.
  MK_IMPLIB := yes
  MK_IMPLIBONLY := no
  MK_IMPLIBDEFPATH := $(DDK_PATH_LIB)
  MK_IMPLIB_EXT := .a
  MK_INSTALLDIR := system32/drivers
  MK_BOOTCDDIR := .
  MK_DISTDIR := drivers
  MK_RES_BASE := $(TARGET_NAME)
endif

ifeq ($(TARGET_TYPE),hal)
  MK_MODE := kernel
  MK_EXETYPE := dll
  MK_DEFEXT := .dll
  MK_DEFENTRY := _DriverEntry@8
  MK_DDKLIBS := ntoskrnl.a
  MK_SDKLIBS :=
  MK_CFLAGS := -D__NTHAL__ -I.
  MK_CPPFLAGS := -D__NTHAL__ -I.
  MK_IMPLIB := yes
  MK_IMPLIBONLY := no
  MK_IMPLIBDEFPATH :=
  MK_IMPLIB_EXT := .a
  MK_INSTALLDIR := system32
  MK_BOOTCDDIR := .
  MK_DISTDIR := dlls
  MK_RES_BASE := $(TARGET_NAME)
endif

ifeq ($(TARGET_TYPE),bootpgm)
  MK_MODE := kernel
  MK_EXETYPE := exe
  MK_DEFEXT := .exe
  MK_DEFENTRY := _DriverEntry@8
  MK_DDKLIBS :=
  MK_SDKLIBS :=
  MK_CFLAGS := -D__NTDRIVER__ -I.
  MK_CPPFLAGS := -D__NTDRIVER__ -I.
  MK_IMPLIB := no
  MK_IMPLIBONLY := no
  MK_IMPLIBDEFPATH :=
  MK_IMPLIB_EXT := .a
  MK_INSTALLDIR := system32
  MK_BOOTCDDIR := system32
  MK_DISTDIR := # FIXME
  MK_RES_BASE := $(TARGET_NAME)
endif

ifeq ($(TARGET_TYPE),miniport)
  MK_MODE := kernel
  MK_EXETYPE := dll
  MK_DEFEXT := .sys
  MK_DEFENTRY := _DriverEntry@8
  MK_DDKLIBS :=
  MK_SDKLIBS :=
  MK_CFLAGS := -D__NTDRIVER__ -I.
  MK_CPPFLAGS := -D__NTDRIVER__ -I.
  MK_IMPLIB := no
  MK_IMPLIBONLY := no
  MK_IMPLIBDEFPATH :=
  MK_IMPLIB_EXT := .a
  MK_INSTALLDIR := system32/drivers
  MK_BOOTCDDIR := .
  MK_DISTDIR := drivers
  MK_RES_BASE := $(TARGET_NAME)
endif

ifeq ($(TARGET_TYPE),gdi_driver)
  MK_MODE := kernel
  MK_EXETYPE := dll
  MK_DEFEXT := .dll
  MK_DEFENTRY := _DrvEnableDriver@12
  MK_DDKLIBS := ntoskrnl.a hal.a win32k.a
  MK_SDKLIBS :=
  MK_CFLAGS := -D__NTDRIVER__ -I.
  MK_CPPFLAGS := -D__NTDRIVER__ -I.
  MK_IMPLIB := yes
  MK_IMPLIBONLY := no
  MK_IMPLIBDEFPATH := $(DDK_PATH_LIB)
  MK_IMPLIB_EXT := .a
  MK_INSTALLDIR := system32
  MK_BOOTCDDIR := .
  MK_DISTDIR := dlls
  MK_RES_BASE := $(TARGET_NAME)
endif


# can be overidden with $(CXX) for linkage of c++ executables
LD_CC = $(CC)


ifeq ($(TARGET_TYPE),program)
  ifeq ($(TARGET_APPTYPE),windows)
    MK_DEFENTRY := _WinMainCRTStartup
    MK_SDKLIBS := ntdll.a kernel32.a gdi32.a user32.a
    TARGET_LFLAGS += -Wl,--subsystem,windows
  endif

  ifeq ($(TARGET_APPTYPE),native)
    MK_DEFENTRY := _NtProcessStartup@4
    MK_SDKLIBS := ntdll.a
    TARGET_LFLAGS += -Wl,--subsystem,native -nostartfiles
  endif

  ifeq ($(TARGET_APPTYPE),console)
    MK_DEFENTRY := _mainCRTStartup
    MK_SDKLIBS :=
    TARGET_LFLAGS += -Wl,--subsystem,console
  endif
endif

ifeq ($(TARGET_TYPE),proglib)
  ifeq ($(TARGET_APPTYPE),windows)
    MK_DEFENTRY := _WinMainCRTStartup
    MK_SDKLIBS := ntdll.a kernel32.a gdi32.a user32.a
    TARGET_LFLAGS += -Wl,--subsystem,windows
  endif

  ifeq ($(TARGET_APPTYPE),native)
    MK_DEFENTRY := _NtProcessStartup@4
    MK_SDKLIBS := ntdll.a
    TARGET_LFLAGS += -Wl,--subsystem,native -nostartfiles
  endif

  ifeq ($(TARGET_APPTYPE),console)
    MK_DEFENTRY := _mainCRTStartup
    MK_SDKLIBS :=
    TARGET_LFLAGS += -Wl,--subsystem,console
  endif
endif

ifeq ($(TARGET_TYPE),subsystem)
  MK_MODE := kernel
  MK_EXETYPE := dll
  MK_DEFEXT := .sys
  MK_DEFENTRY := _DriverEntry@8
  MK_DDKLIBS := ntoskrnl.a hal.a
  MK_SDKLIBS :=
  MK_CFLAGS := -D__NTDRIVER__ -I.
  MK_CPPFLAGS := -D__NTDRIVER__ -I.
  MK_IMPLIB := yes
  MK_IMPLIBONLY := no
  MK_IMPLIBDEFPATH := $(DDK_PATH_LIB)
  MK_IMPLIB_EXT := .a
  MK_INSTALLDIR := system32
  MK_DISTDIR := drivers
  MK_RES_BASE := $(TARGET_NAME)
endif

ifeq ($(TARGET_TYPE),kmdll)
  MK_MODE := kernel
  MK_EXETYPE := dll
  MK_DEFEXT := .dll
  MK_DEFENTRY := 0x0
  MK_DDKLIBS := ntoskrnl.a hal.a
  MK_SDKLIBS :=
  MK_CFLAGS := -D__NTDRIVER__ -I.
  MK_CPPFLAGS := -D__NTDRIVER__ -I.
  MK_IMPLIB := yes
  MK_IMPLIBONLY := no
  MK_IMPLIBDEFPATH := $(DDK_PATH_LIB)
  MK_IMPLIB_EXT := .a
  MK_INSTALLDIR := system32
  MK_DISTDIR := drivers
  MK_RES_BASE := $(TARGET_NAME)
endif

ifeq ($(TARGET_TYPE),winedll)
-include Makefile.ros
  MK_GENERATED_MAKEFILE = Makefile.ros
  MK_MODE := user
  MK_EXETYPE := dll
  MK_DEFEXT := .dll
  MK_DEFENTRY := _DllMain@12
  MK_DDKLIBS :=
  MK_SDKLIBS :=
  MK_CFLAGS := -D__USE_W32API -D_WIN32_IE=0x600 -D_WIN32_WINNT=0x501 -DWINVER=0x501 -D_STDDEF_H -DCOBJMACROS -I$(PATH_TO_TOP)/include/wine
  MK_CPPFLAGS := -D__USE_W32API -D_WIN32_IE=0x600 -D_WIN32_WINNT=0x501 -DWINVER=0x501 -D__need_offsetof -DCOBJMACROS -I$(PATH_TO_TOP)/include -I$(PATH_TO_TOP)/include/wine
  MK_RCFLAGS := --define __USE_W32API --include-dir $(PATH_TO_TOP)/include/wine
  MK_IMPLIB := yes
  MK_IMPLIBONLY := no
  MK_IMPLIBDEFPATH := $(SDK_PATH_LIB)
  MK_IMPLIB_EXT := .a
  MK_INSTALLDIR := system32
  MK_BOOTCDDIR := system32
  MK_DISTDIR := dlls
  MK_RES_BASE := $(TARGET_NAME)
ifeq ($(TARGET_DEFNAME),)
  MK_DEFBASENAME := $(TARGET_NAME).spec
  MK_SPECDEF := $(MK_DEFBASENAME).def
else
  MK_DEFBASENAME := $(TARGET_DEFNAME)
endif
  MK_KILLAT := --kill-at
  TARGET_DEFONLY := yes
  MK_RC_BINARIES = $(TARGET_RC_BINARIES)
endif

ifeq ($(TARGET_TYPE),winedrv)
-include Makefile.ros
  MK_GENERATED_MAKEFILE = Makefile.ros
  MK_MODE := user
  MK_EXETYPE := drv
  MK_DEFEXT := .drv
# does this need changing?:
  MK_DEFENTRY := _DllMain@12
  MK_DDKLIBS :=
  MK_SDKLIBS :=
  MK_CFLAGS := -D__USE_W32API -D_WIN32_IE=0x600 -D_WIN32_WINNT=0x501 -DWINVER=0x501 -D__need_offsetof -DCOBJMACROS -I$(PATH_TO_TOP)/include/wine
  MK_CPPFLAGS := -D__USE_W32API -D_WIN32_IE=0x600 -D_WIN32_WINNT=0x501 -DWINVER=0x501 -D__need_offsetof -DCOBJMACROS -I$(PATH_TO_TOP)/include -I$(PATH_TO_TOP)/include/wine
  MK_RCFLAGS := --define __USE_W32API --include-dir $(PATH_TO_TOP)/include/wine
  MK_IMPLIB := yes
  MK_IMPLIBONLY := no
  MK_IMPLIBDEFPATH := $(SDK_PATH_LIB)
  MK_IMPLIB_EXT := .a
  MK_INSTALLDIR := system32
  MK_BOOTCDDIR := system32
  MK_DISTDIR := dlls
  MK_RES_BASE := $(TARGET_NAME)
ifeq ($(TARGET_DEFNAME),)
  MK_DEFBASENAME := $(TARGET_NAME).spec
  MK_SPECDEF := $(MK_DEFBASENAME).def
else
  MK_DEFBASENAME := $(TARGET_DEFNAME)
endif
  MK_KILLAT := --kill-at
  TARGET_DEFONLY := yes
  MK_RC_BINARIES = $(TARGET_RC_BINARIES)
endif

ifeq ($(TARGET_RC_SRCS),)
  MK_RES_SRC := $(TARGET_PATH)/$(MK_RES_BASE).rc
  MK_RESOURCE := $(MK_RES_BASE).coff
else
  MK_RES_SRC := $(TARGET_RC_SRCS)
  MK_RESOURCE := $(TARGET_RC_SRCS:.rc=.coff)
endif

ifneq ($(TARGET_INSTALLDIR),)
  MK_INSTALLDIR := $(TARGET_INSTALLDIR)
endif


ifneq ($(BOOTCD_INSTALL),)
  MK_INSTALLDIR := .
endif


ifeq ($(TARGET_LIBPATH),)
  MK_LIBPATH := $(SDK_PATH_LIB)
else
  MK_LIBPATH := $(TARGET_LIBPATH)
endif

ifeq ($(TARGET_IMPLIBPATH),)
  MK_IMPLIBPATH := $(MK_IMPLIBDEFPATH)
else
  MK_IMPLIBPATH := $(TARGET_IMPLIBPATH)
endif


ifeq ($(TARGET_BASENAME),)
  MK_BASENAME := $(TARGET_NAME)
else
  MK_BASENAME := $(TARGET_BASENAME)
endif


ifeq ($(TARGET_EXTENSION),)
  MK_EXT := $(MK_DEFEXT)
else
  MK_EXT := $(TARGET_EXTENSION)
endif


ifeq ($(TARGET_NORC),yes)
  MK_FULLRES :=
else
  MK_FULLRES := $(TARGET_PATH)/$(MK_RESOURCE)
endif

ifneq ($(TARGET_TYPE),winedll)
ifeq ($(TARGET_DEFNAME),)
  MK_DEFBASENAME := $(TARGET_NAME)
else
  MK_DEFBASENAME := $(TARGET_DEFNAME)
endif
endif

MK_DEFNAME := $(TARGET_PATH)/$(MK_DEFBASENAME).def
ifeq ($(TARGET_DEFONLY),yes)
  MK_EDFNAME := $(MK_DEFNAME)
else
  MK_EDFNAME := $(TARGET_PATH)/$(MK_DEFBASENAME).edf
endif


ifeq ($(MK_MODE),user)
  ifeq ($(MK_EXETYPE),dll)
    MK_DEFBASE := 0x10000000
  else
    MK_DEFBASE := 0x400000
  endif
  ifneq ($(TARGET_SDKLIBS),)
    MK_LIBS := $(addprefix $(SDK_PATH_LIB)/, $(TARGET_SDKLIBS))
  else
    MK_LIBS := $(addprefix $(SDK_PATH_LIB)/, $(MK_SDKLIBS))
  endif
endif


ifeq ($(MK_MODE),kernel)
  MK_DEFBASE := 0x10000
  MK_LIBS := $(addprefix $(DDK_PATH_LIB)/, $(TARGET_DDKLIBS) $(MK_DDKLIBS))
endif


ifneq ($(TARGET_LIBS),)
  MK_LIBS := $(TARGET_LIBS) $(MK_LIBS)
endif


ifeq ($(TARGET_BASE),)
  TARGET_BASE := $(MK_DEFBASE)
endif


ifeq ($(TARGET_ENTRY),)
  TARGET_ENTRY := $(MK_DEFENTRY)
endif

#
# Include details of the OS configuration
#
include $(PATH_TO_TOP)/config


TARGET_CFLAGS += $(MK_CFLAGS) $(STD_CFLAGS)
ifeq ($(DBG),1)
TARGET_ASFLAGS += -g
TARGET_CFLAGS += -g
TARGET_LFLAGS += -g
endif

TARGET_CPPFLAGS += $(MK_CPPFLAGS) $(STD_CPPFLAGS)

TARGET_RCFLAGS += $(MK_RCFLAGS) $(STD_RCFLAGS)

TARGET_ASFLAGS += $(MK_ASFLAGS) $(STD_ASFLAGS)

TARGET_NFLAGS += $(MK_NFLAGS)


MK_GCCLIBS := $(addprefix -l, $(TARGET_GCCLIBS))

ifeq ($(MK_MODE),static)
  MK_FULLNAME := $(MK_LIBPATH)/$(MK_BASENAME)$(MK_EXT)
else
  MK_FULLNAME := $(MK_BASENAME)$(MK_EXT)
endif

ifeq ($(TARGET_TYPE), kmlibrary)
  MK_FULLNAME := $(DDK_PATH_LIB)/$(MK_BASENAME)$(MK_EXT)
endif

MK_IMPLIB_FULLNAME := $(MK_BASENAME)$(MK_IMPLIB_EXT)

MK_NOSTRIPNAME := $(MK_BASENAME).nostrip$(MK_EXT)

# We don't want to link header files
MK_OBJECTS := $(filter-out %.h,$(TARGET_OBJECTS))

# There is problems with C++ applications and ld -r. Ld can cause errors like:
#   reloc refers to symbol `.text$_ZN9CCABCodecC2Ev' which is not being output
ifeq ($(TARGET_CPPAPP),yes)
  MK_STRIPPED_OBJECT := $(MK_OBJECTS)
else
  MK_STRIPPED_OBJECT := $(MK_BASENAME).stripped.o
endif

ifeq ($(TARGET_REGTESTS),yes)
  REGTEST_TARGETS := tests/_regtests.c tests/Makefile.tests tests/_rtstub.c 
ifeq ($(MK_MODE),user)
    MK_LIBS := $(SDK_PATH_LIB)/rtshared.a $(MK_LIBS)
endif
  MK_REGTESTS_CLEAN := clean_regtests
  MK_OBJECTS += tests/_rtstub.o tests/regtests.a
  TARGET_CFLAGS += -I$(REGTESTS_PATH_INC)
else
  REGTEST_TARGETS :=
  MK_REGTESTS_CLEAN :=
endif

ifeq ($(MK_IMPLIBONLY),yes)

TARGET_CLEAN += $(MK_IMPLIBPATH)/$(MK_IMPLIB_FULLNAME)

all: $(REGTEST_TARGETS) $(MK_IMPLIBPATH)/$(MK_IMPLIB_FULLNAME)

$(MK_IMPLIBPATH)/$(MK_IMPLIB_FULLNAME): $(MK_OBJECTS) $(MK_DEFNAME)
	$(DLLTOOL) \
		--dllname $(MK_FULLNAME) \
		--def $(MK_DEFNAME) \
		--output-lib $(MK_IMPLIBPATH)/$(MK_BASENAME).a \
		--kill-at

else # MK_IMPLIBONLY

all: $(REGTEST_TARGETS) $(MK_FULLNAME) $(MK_NOSTRIPNAME) $(SUBDIRS:%=%_all)


ifeq ($(MK_IMPLIB),yes)
  MK_EXTRACMD := --def $(MK_EDFNAME)
else
  MK_EXTRACMD :=
endif

# User mode targets
ifeq ($(MK_MODE),user)

ifeq ($(MK_EXETYPE),dll)
  TARGET_LFLAGS += -mdll -Wl,--image-base,$(TARGET_BASE)
  MK_EXTRADEP := $(MK_EDFNAME)
  MK_EXTRACMD2 := -Wl,temp.exp
else
  MK_EXTRADEP :=
  MK_EXTRACMD2 :=
endif

$(MK_NOSTRIPNAME): $(MK_FULLRES) $(MK_OBJECTS) $(MK_EXTRADEP) $(MK_LIBS)
ifeq ($(MK_EXETYPE),dll)
	$(LD_CC) -Wl,--base-file,base.tmp \
		-Wl,--entry,$(TARGET_ENTRY) \
		$(TARGET_LFLAGS) \
		-o junk.tmp \
		$(MK_FULLRES) $(MK_OBJECTS) $(MK_LIBS) $(MK_GCCLIBS)
	- $(RM) junk.tmp
	$(DLLTOOL) --dllname $(MK_FULLNAME) \
		--base-file base.tmp \
		--output-exp temp.exp $(MK_EXTRACMD)
	- $(RM) base.tmp
	$(LD_CC) -Wl,--base-file,base.tmp \
		-Wl,--entry,$(TARGET_ENTRY) \
		$(TARGET_LFLAGS) \
		temp.exp \
		-o junk.tmp \
		$(MK_FULLRES) $(MK_OBJECTS) $(MK_LIBS) $(MK_GCCLIBS)
	- $(RM) junk.tmp
	$(DLLTOOL) --dllname $(MK_FULLNAME) \
		--base-file base.tmp \
		--output-exp temp.exp $(MK_KILLAT) $(MK_EXTRACMD)
	- $(RM) base.tmp
endif
	$(LD_CC) $(TARGET_LFLAGS) \
		-Wl,--entry,$(TARGET_ENTRY) $(MK_EXTRACMD2) \
	  	-o $(MK_NOSTRIPNAME) \
	  	$(MK_FULLRES) $(MK_OBJECTS) $(MK_LIBS) $(MK_GCCLIBS)
	- $(RM) temp.exp
	- $(RSYM) $(MK_NOSTRIPNAME) $(MK_BASENAME).sym
ifeq ($(FULL_MAP),yes)
	$(OBJDUMP) -d -S $(MK_NOSTRIPNAME) > $(MK_BASENAME).map
else
	$(NM) --numeric-sort $(MK_NOSTRIPNAME) > $(MK_BASENAME).map
endif

$(MK_FULLNAME): $(MK_NOSTRIPNAME) $(MK_EXTRADEP)
	-
ifneq ($(TARGET_CPPAPP),yes)
	$(LD) -r -o $(MK_STRIPPED_OBJECT) $(MK_OBJECTS)
	$(STRIP) --strip-debug $(MK_STRIPPED_OBJECT)
endif
ifeq ($(MK_EXETYPE),dll)
	$(LD_CC) -Wl,--base-file,base.tmp \
		-Wl,--entry,$(TARGET_ENTRY) \
		-Wl,--strip-debug \
		$(TARGET_LFLAGS) \
		-o junk.tmp \
		$(MK_FULLRES) $(MK_STRIPPED_OBJECT) $(MK_LIBS) $(MK_GCCLIBS)
	- $(RM) junk.tmp
	$(DLLTOOL) --dllname $(MK_FULLNAME) \
		--base-file base.tmp \
		--output-exp temp.exp $(MK_EXTRACMD)
	- $(RM) base.tmp
	$(LD_CC) -Wl,--base-file,base.tmp \
		-Wl,--entry,$(TARGET_ENTRY) \
		-Wl,--strip-debug \
		$(TARGET_LFLAGS) \
		temp.exp \
		-o junk.tmp \
		$(MK_FULLRES) $(MK_STRIPPED_OBJECT) $(MK_LIBS) $(MK_GCCLIBS)
	- $(RM) junk.tmp
	$(DLLTOOL) --dllname $(MK_FULLNAME) \
		--base-file base.tmp \
		--output-exp temp.exp $(MK_KILLAT) $(MK_EXTRACMD)
	- $(RM) base.tmp
endif
	$(LD_CC) $(TARGET_LFLAGS) \
		-Wl,--entry,$(TARGET_ENTRY) \
		-Wl,--strip-debug \
		$(MK_EXTRACMD2) \
	  	-o $(MK_FULLNAME) \
	  	$(MK_FULLRES) $(MK_STRIPPED_OBJECT) $(MK_LIBS) $(MK_GCCLIBS)
ifneq ($(TARGET_CPPAPP),yes)
	- $(RM) temp.exp $(MK_STRIPPED_OBJECT)
else
	- $(RM) temp.exp
endif

endif # KM_MODE

# Kernel mode targets
ifeq ($(MK_MODE),kernel)

ifeq ($(MK_IMPLIB),yes)
  MK_EXTRACMD := --def $(MK_EDFNAME)
  MK_EXTRADEP := $(MK_EDFNAME)
else
  MK_EXTRACMD :=
  MK_EXTRADEP :=
endif

$(MK_NOSTRIPNAME): $(MK_FULLRES) $(MK_OBJECTS) $(MK_EXTRADEP) $(MK_LIBS)
	$(LD_CC) -Wl,--base-file,base.tmp \
		-Wl,--entry,$(TARGET_ENTRY) \
		$(TARGET_LFLAGS) \
		-nostartfiles -nostdlib \
		-o junk.tmp \
		$(MK_FULLRES) $(MK_OBJECTS) $(MK_LIBS) $(MK_GCCLIBS)
	- $(RM) junk.tmp
	$(DLLTOOL) --dllname $(MK_FULLNAME) \
		--base-file base.tmp \
		--output-exp temp.exp $(MK_EXTRACMD)
	- $(RM) base.tmp
	$(LD_CC) $(TARGET_LFLAGS) \
		-Wl,--subsystem,native \
		-Wl,--image-base,$(TARGET_BASE) \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,--entry,$(TARGET_ENTRY) \
		-Wl,temp.exp \
		-mdll -nostartfiles -nostdlib \
		-o $(MK_NOSTRIPNAME) \
	  	$(MK_FULLRES) $(MK_OBJECTS) $(MK_LIBS) $(MK_GCCLIBS)
	- $(RM) temp.exp
	$(RSYM) $(MK_NOSTRIPNAME) $(MK_BASENAME).sym
ifeq ($(FULL_MAP),yes)
	$(OBJDUMP) -d -S $(MK_NOSTRIPNAME) > $(MK_BASENAME).map
else
	$(NM) --numeric-sort $(MK_NOSTRIPNAME) > $(MK_BASENAME).map
endif

$(MK_FULLNAME): $(MK_FULLRES) $(MK_OBJECTS) $(MK_EXTRADEP) $(MK_LIBS) $(MK_NOSTRIPNAME)
	-
ifneq ($(TARGET_CPPAPP),yes)
	$(LD) -r -o $(MK_STRIPPED_OBJECT) $(MK_OBJECTS)
	$(STRIP) --strip-debug $(MK_STRIPPED_OBJECT)
endif
	$(LD_CC) -Wl,--base-file,base.tmp \
		-Wl,--entry,$(TARGET_ENTRY) \
		$(TARGET_LFLAGS) \
		-nostartfiles -nostdlib \
		-o junk.tmp \
		$(MK_FULLRES) $(MK_STRIPPED_OBJECT) $(MK_LIBS) $(MK_GCCLIBS)
	- $(RM) junk.tmp
	$(DLLTOOL) --dllname $(MK_FULLNAME) \
		--base-file base.tmp \
		--output-exp temp.exp $(MK_EXTRACMD)
	- $(RM) base.tmp
	$(LD_CC) $(TARGET_LFLAGS) \
		-Wl,--subsystem,native \
		-Wl,--image-base,$(TARGET_BASE) \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,--entry,$(TARGET_ENTRY) \
		-Wl,temp.exp \
		-mdll -nostartfiles -nostdlib \
		-o $(MK_FULLNAME) \
	  	$(MK_FULLRES) $(MK_STRIPPED_OBJECT) $(MK_LIBS) $(MK_GCCLIBS)
ifneq ($(TARGET_CPPAPP),yes)
	- $(RM) temp.exp $(MK_STRIPPED_OBJECT)
else
	- $(RM) temp.exp
endif

endif # MK_MODE

# Static library target
ifeq ($(MK_MODE),static)

$(MK_FULLNAME): $(TARGET_OBJECTS)
	$(AR) -r $(MK_FULLNAME) $(TARGET_OBJECTS)

# Static libraries dont have a nostrip version
$(MK_NOSTRIPNAME):
	-

.PHONY: $(MK_NOSTRIPNAME)

endif # MK_MODE

endif # MK_IMPLIBONLY


$(MK_FULLRES): $(PATH_TO_TOP)/include/reactos/buildno.h $(MK_RES_SRC)

ifeq ($(MK_DEPENDS),yes)
depends:
else
depends:
endif

ifeq ($(MK_IMPLIB),yes)
$(MK_IMPLIBPATH)/$(MK_BASENAME).a: $(MK_DEFNAME)
	$(DLLTOOL) --dllname $(MK_FULLNAME) \
		--def $(MK_DEFNAME) \
		--output-lib $(MK_IMPLIBPATH)/$(MK_BASENAME).a \
		--kill-at

implib: $(MK_IMPLIBPATH)/$(MK_BASENAME).a
else
implib: $(SUBDIRS:%=%_implib)
endif

# Be carefull not to clean non-object files
MK_CLEANFILES := $(filter %.o,$(MK_OBJECTS))
MK_CLEANFILTERED := $(MK_OBJECTS:.o=.d)
MK_CLEANDEPS := $(join $(dir $(MK_CLEANFILTERED)), $(addprefix ., $(notdir $(MK_CLEANFILTERED))))

clean: $(MK_REGTESTS_CLEAN) $(SUBDIRS:%=%_clean)
	- $(RM) *.o depend.d *.pch $(MK_BASENAME).sym $(MK_BASENAME).a $(MK_RESOURCE) \
	  $(MK_FULLNAME) $(MK_NOSTRIPNAME) $(MK_CLEANFILES) $(MK_CLEANDEPS) $(MK_BASENAME).map \
	  junk.tmp base.tmp temp.exp $(MK_RC_BINARIES) $(MK_SPECDEF) $(MK_GENERATED_MAKEFILE) \
	  $(TARGET_CLEAN)

ifneq ($(TARGET_HEADERS),)
$(TARGET_OBJECTS): $(TARGET_HEADERS)
endif

# install and bootcd rules

ifeq ($(MK_IMPLIBONLY),yes)

# Don't install import libraries

install:

bootcd:

else # MK_IMPLIBONLY


# Don't install static libraries
ifeq ($(MK_MODE),static)

install:
	
bootcd:	

else # MK_MODE

ifeq ($(INSTALL_SYMBOLS),yes)

install: $(SUBDIRS:%=%_install) $(MK_FULLNAME) $(MK_BASENAME).sym
	-$(CP) $(MK_FULLNAME) $(INSTALL_DIR)/$(MK_INSTALLDIR)/$(MK_FULLNAME)
	-$(CP) $(MK_BASENAME).sym $(INSTALL_DIR)/symbols/$(MK_BASENAME).sym

else # INSTALL_SYMBOLS

install: $(SUBDIRS:%=%_install) $(MK_FULLNAME)
	-$(CP) $(MK_FULLNAME) $(INSTALL_DIR)/$(MK_INSTALLDIR)/$(MK_FULLNAME)

endif # INSTALL_SYMBOLS


# Bootstrap files for the bootable CD
ifeq ($(TARGET_BOOTSTRAP),yes)

ifneq ($(TARGET_BOOTSTRAP_NAME),)
MK_BOOTSTRAP_NAME := $(TARGET_BOOTSTRAP_NAME)
else # TARGET_BOOTSTRAP_NAME
MK_BOOTSTRAP_NAME := $(MK_FULLNAME)
endif # TARGET_BOOTSTRAP_NAME

bootcd: $(SUBDIRS:%=%_bootcd)
	- $(CP) $(MK_FULLNAME) $(BOOTCD_DIR)/reactos/$(MK_BOOTCDDIR)/$(MK_BOOTSTRAP_NAME)

else # TARGET_BOOTSTRAP

bootcd:
	
endif # TARGET_BOOTSTRAP
    
endif # MK_MODE     

endif # MK_IMPLIBONLY

ifeq ($(TARGET_TYPE),winedll)
Makefile.ros: Makefile.in Makefile.ros-template
	$(TOOLS_PATH)/wine2ros/wine2ros Makefile.in Makefile.ros-template Makefile.ros

$(MK_RC_BINARIES): $(TARGET_RC_BINSRC)
	$(TOOLS_PATH)/bin2res/bin2res -f -o $@ $(TARGET_RC_BINSRC)

$(MK_RESOURCE): $(MK_RC_BINARIES)
endif

REGTEST_TESTS = $(wildcard tests/tests/*.c)

$(REGTEST_TARGETS): $(REGTEST_TESTS)
ifeq ($(MK_MODE),user)
	$(REGTESTS) ./tests/tests ./tests/_regtests.c ./tests/Makefile.tests -u ./tests/_rtstub.c
	$(MAKE) -C tests TARGET_REGTESTS=no all
else
ifeq ($(MK_MODE),kernel)
	$(REGTESTS) ./tests/tests ./tests/_regtests.c ./tests/Makefile.tests -k ./tests/_rtstub.c
	$(MAKE) -C tests TARGET_REGTESTS=no all
endif
endif

clean_regtests:
	$(MAKE) -C tests TARGET_REGTESTS=no clean
	$(RM) ./tests/_rtstub.c ./tests/_regtests.c ./tests/Makefile.tests

.PHONY: all depends implib clean install dist bootcd depends gen_regtests clean_regtests

ifneq ($(SUBDIRS),)
$(SUBDIRS:%=%_all): %_all:
	$(MAKE) -C $* SUBDIRS= all

$(SUBDIRS:%=%_implib): %_implib:
	$(MAKE) -C $* SUBDIRS= implib

$(SUBDIRS:%=%_clean): %_clean:
	$(MAKE) -C $* SUBDIRS= clean

$(SUBDIRS:%=%_install): %_install:
	$(MAKE) -C $* SUBDIRS= install

$(SUBDIRS:%=%_dist): %_dist:
	$(MAKE) -C $* SUBDIRS= dist

$(SUBDIRS:%=%_bootcd): %_bootcd:
	$(MAKE) -C $* SUBDIRS= bootcd

.PHONY: $(SUBDIRS:%=%_all) $(SUBDIRS:%=%_implib) $(SUBDIRS:%=%_clean) \
        $(SUBDIRS:%=%_install) $(SUBDIRS:%=%_dist) $(SUBDIRS:%=%_bootcd)
endif

# Precompiled header support
# When using PCHs, use dependency tracking to keep the .pch files up-to-date.

MK_PCHNAME =
ifeq ($(ROS_USE_PCH),yes)
ifneq ($(TARGET_PCH),)
MK_PCHNAME = $(TARGET_PCH).pch

# GCC generates wrong dependencies for header files.
MK_PCHFAKE = $(TARGET_PCH:.h=.o)
$(MK_PCHFAKE):
	- $(RTOUCH) $(MK_PCHFAKE)

$(MK_PCHNAME): depend.d
	- $(RTOUCH) $(MK_PCHNAME)
	- $(CC) $(TARGET_CFLAGS) $(TARGET_PCH)

depend.d: $(MK_PCHFAKE)
	- $(RTOUCH) depend.d
	- $(CC) $(TARGET_CFLAGS) $(TARGET_PCH) -M -MF depend.d

include depend.d

endif # TARGET_PCH
endif # ROS_USE_PCH


%.o: %.c $(MK_PCHNAME)
	$(CC) $(TARGET_CFLAGS) -c $< -o $@
%.o: %.cc
	$(CXX) $(TARGET_CPPFLAGS) -c $< -o $@
%.o: %.cxx
	$(CXX) $(TARGET_CPPFLAGS) -c $< -o $@
%.o: %.cpp
	$(CXX) $(TARGET_CPPFLAGS) -c $< -o $@
%.o: %.S
	$(AS) $(TARGET_ASFLAGS) -c $< -o $@
%.o: %.s
	$(AS) $(TARGET_ASFLAGS) -c $< -o $@
%.o: %.asm
	$(NASM_CMD) $(NFLAGS) $(TARGET_NFLAGS) $< -o $@
%.coff: %.rc
	$(RC) $(TARGET_RCFLAGS) $< -o $@
%.spec.def: %.spec
	$(WINEBUILD) $(DEFS) -o $@ --def $<
%.drv.spec.def: %.spec
	$(WINEBUILD) $(DEFS) -o $@ --def $<
# Kill implicit rule
.o:;

# Compatibility
CFLAGS := $(TARGET_CFLAGS)

# EOF
