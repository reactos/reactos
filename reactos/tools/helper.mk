# $Id: helper.mk,v 1.24 2003/01/05 19:17:16 robd Exp $
#
# Helper makefile for ReactOS modules
# Variables this makefile accepts:
#   $TARGET_TYPE       = Type of target:
#                        program = User mode program
#                        proglib = Executable program that have exported functions
#                        dynlink = Dynamic Link Library (DLL)
#                        library = Library that will be linked with other code
#                        driver_library = Import library for a driver
#                        driver = Kernel mode driver
#                        export_driver = Kernel mode driver that have exported functions
#                        hal = Hardware Abstraction Layer
#                        bootpgm = Boot program
#                        miniport = Kernel mode driver that does not link with ntoskrnl.exe or hal.dll
#                        gdi_driver = Kernel mode graphics driver that link with win32k.sys
#   $TARGET_APPTYPE    = Application type (windows,native,console)
#   $TARGET_NAME       = Base name of output file and .rc, .def, and .edf files
#   $TARGET_OBJECTS    = Object files that compose the module
#   $TARGET_HEADERS    = Header files that the object files depend on (optional)
#   $TARGET_DEFNAME    = Base name of .def and .edf files (optional)
#   $TARGET_BASENAME   = Base name of output file (overrides $TARGET_NAME if it exists) (optional)
#   $TARGET_EXTENSION  = Extesion of the output file (optional)
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
#   $TARGET_LIBPATH    = Destination path for import libraries (optional)
#   $TARGET_INSTALLDIR = Destination path when installed (optional)
#   $WINE_MODE         = Compile using WINE headers (no,yes) (optional)
#   $WINE_RC           = Name of .rc file for WINE modules (optional)

include $(PATH_TO_TOP)/config


ifeq ($(TARGET_PATH),)
TARGET_PATH := .
endif


ifeq ($(TARGET_TYPE),program)
  MK_MODE := user
  MK_EXETYPE := exe
  MK_DEFEXT := .exe
  MK_DEFENTRY := _DEFINE_TARGET_APPTYPE
  MK_DDKLIBS :=
  MK_SDKLIBS :=
ifneq ($(WINE_MODE),yes)
  MK_CFLAGS := -I./ -I$(SDK_PATH_INC)
  MK_CPPFLAGS := -I./ -I$(SDK_PATH_INC)
  MK_RCFLAGS := --include-dir $(SDK_PATH_INC)
else
  MK_CFLAGS := -I$(PATH_TO_TOP)/include/wine -I./ -I$(WINE_INCLUDE)
  MK_CPPFLAGS := -I$(PATH_TO_TOP)/include/wine -I./ -I$(WINE_INCLUDE)
  MK_RCFLAGS := --include-dir $(PATH_TO_TOP)/include/wine --include-dir $(WINE_INCLUDE)
endif
  MK_IMPLIB := no
  MK_IMPLIBONLY := no
  MK_IMPLIBDEFPATH :=
  MK_IMPLIB_EXT := .a
  MK_INSTALLDIR := bin
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
  MK_CFLAGS := -I./ -I$(SDK_PATH_INC)
  MK_CPPFLAGS := -I./ -I$(SDK_PATH_INC)
  MK_RCFLAGS := --include-dir $(SDK_PATH_INC)
  MK_IMPLIB := yes
  MK_IMPLIBONLY := no
  MK_IMPLIBDEFPATH := $(SDK_PATH_LIB)
  MK_IMPLIB_EXT := .a
  MK_INSTALLDIR := bin
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
  MK_CFLAGS := -I./ -I$(SDK_PATH_INC)
  MK_CPPFLAGS := -I./ -I$(SDK_PATH_INC)
  MK_RCFLAGS := --include-dir $(SDK_PATH_INC)
else
  MK_CFLAGS := -I$(PATH_TO_TOP)/include/wine -I./ -I$(WINE_INCLUDE)
  MK_CPPFLAGS := -I$(PATH_TO_TOP)/include/wine -I./ -I$(WINE_INCLUDE)
  MK_RCFLAGS := --include-dir $(PATH_TO_TOP)/include/wine --include-dir $(WINE_INCLUDE)
endif
  MK_IMPLIB := yes
  MK_IMPLIBONLY := no
  MK_IMPLIBDEFPATH := $(SDK_PATH_LIB)
  MK_IMPLIB_EXT := .a
  MK_INSTALLDIR := system32
  MK_DISTDIR := dlls
ifeq ($(WINE_RC),)
  MK_RES_BASE := $(TARGET_NAME)
else
  MK_RES_BASE := $(WINE_RC)
endif
endif

ifeq ($(TARGET_TYPE),library)
  MK_MODE := static
  MK_EXETYPE :=
  MK_DEFEXT := .a
  MK_DEFENTRY :=
  MK_DDKLIBS :=
  MK_SDKLIBS :=
  MK_CFLAGS := -I./ -I$(SDK_PATH_INC)
  MK_CPPFLAGS := -I./ -I$(SDK_PATH_INC)
  MK_RCFLAGS := --include-dir $(SDK_PATH_INC)
  MK_IMPLIB := no
  MK_IMPLIBONLY := no
  MK_IMPLIBDEFPATH :=
  MK_IMPLIB_EXT :=
  MK_INSTALLDIR := system32
  MK_DISTDIR := # FIXME
  MK_RES_BASE :=
endif

ifeq ($(TARGET_TYPE),driver_library)
  MK_MODE := kernel
  MK_EXETYPE := dll
  MK_DEFEXT := .dll
  MK_DEFENTRY :=
  MK_DDKLIBS :=
  MK_SDKLIBS :=
  MK_CFLAGS := -I./ -I$(DDK_PATH_INC)
  MK_CPPFLAGS := -I./ -I$(DDK_PATH_INC)
  MK_RCFLAGS := --include-dir $(SDK_PATH_INC)
  MK_IMPLIB := no
  MK_IMPLIBONLY := yes
  MK_IMPLIBDEFPATH := $(DDK_PATH_LIB)
  MK_IMPLIB_EXT := .a
  MK_INSTALLDIR := $(DDK_PATH_INC)
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
  MK_CFLAGS := -D__NTDRIVER__ -I./ -I$(DDK_PATH_INC)
  MK_CPPFLAGS := -D__NTDRIVER__ -I./ -I$(DDK_PATH_INC)
  MK_RCFLAGS := --include-dir $(SDK_PATH_INC)
  MK_IMPLIB := no
  MK_IMPLIBONLY := no
  MK_IMPLIBDEFPATH :=
  MK_IMPLIB_EXT := .a
  MK_INSTALLDIR := system32/drivers
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
  MK_CFLAGS := -D__NTDRIVER__ -I./ -I$(DDK_PATH_INC)
  MK_CPPFLAGS := -D__NTDRIVER__ -I./ -I$(DDK_PATH_INC)
  MK_RCFLAGS := --include-dir $(SDK_PATH_INC)
  MK_IMPLIB := yes
  MK_IMPLIBONLY := no
  MK_IMPLIBDEFPATH := $(DDK_PATH_LIB)
  MK_IMPLIB_EXT := .a
  MK_INSTALLDIR := system32/drivers
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
  MK_CFLAGS := -D__NTHAL__ -I./ -I$(DDK_PATH_INC)
  MK_CPPFLAGS := -D__NTHAL__ -I./ -I$(DDK_PATH_INC)
  MK_RCFLAGS := --include-dir $(SDK_PATH_INC)
  MK_IMPLIB := yes
  MK_IMPLIBONLY := no
  MK_IMPLIBDEFPATH :=
  MK_IMPLIB_EXT := .a
  MK_INSTALLDIR := system32
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
  MK_CFLAGS := -D__NTDRIVER__ -I./ -I$(DDK_PATH_INC)
  MK_CPPFLAGS := -D__NTDRIVER__ -I./ -I$(DDK_PATH_INC)
  MK_RCFLAGS := --include-dir $(SDK_PATH_INC)
  MK_IMPLIB := no
  MK_IMPLIBONLY := no
  MK_IMPLIBDEFPATH :=
  MK_IMPLIB_EXT := .a
  MK_INSTALLDIR := system32
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
  MK_CFLAGS := -D__NTDRIVER__ -I./ -I$(DDK_PATH_INC)
  MK_CPPFLAGS := -D__NTDRIVER__ -I./ -I$(DDK_PATH_INC)
  MK_RCFLAGS := --include-dir $(SDK_PATH_INC)
  MK_IMPLIB := no
  MK_IMPLIBONLY := no
  MK_IMPLIBDEFPATH :=
  MK_IMPLIB_EXT := .a
  MK_INSTALLDIR := system32/drivers
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
  MK_CFLAGS := -D__NTDRIVER__ -I./ -I$(DDK_PATH_INC)
  MK_CPPFLAGS := -D__NTDRIVER__ -I./ -I$(DDK_PATH_INC)
  MK_RCFLAGS := --include-dir $(SDK_PATH_INC)
  MK_IMPLIB := yes
  MK_IMPLIBONLY := no
  MK_IMPLIBDEFPATH := $(DDK_PATH_LIB)
  MK_IMPLIB_EXT := .a
  MK_INSTALLDIR := system32/drivers
  MK_DISTDIR := drivers
  MK_RES_BASE := $(TARGET_NAME)
endif


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


MK_RESOURCE := $(MK_RES_BASE).coff


ifneq ($(TARGET_INSTALLDIR),)
  MK_INSTALLDIR := $(TARGET_INSTALLDIR)
endif


ifeq ($(TARGET_LIBPATH),)
  MK_IMPLIBPATH := $(MK_IMPLIBDEFPATH)
else
  MK_IMPLIBPATH := $(TARGET_LIBPATH)
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


ifeq ($(TARGET_DEFNAME),)
  MK_DEFBASE := $(TARGET_NAME)
else
  MK_DEFBASE := $(TARGET_DEFNAME)
endif

MK_DEFNAME := $(TARGET_PATH)/$(MK_DEFBASE).def
ifeq ($(TARGET_DEFONLY),yes)
  MK_EDFNAME := $(MK_DEFNAME)
else
  MK_EDFNAME := $(TARGET_PATH)/$(MK_DEFBASE).edf
endif


ifeq ($(MK_MODE),user)
  MK_DEFBASE := 0x400000
  ifneq ($(TARGET_SDKLIBS),)
    MK_LIBS := $(addprefix $(SDK_PATH_LIB)/, $(TARGET_SDKLIBS))
  else
    MK_LIBS := $(addprefix $(SDK_PATH_LIB)/, $(MK_SDKLIBS))
  endif
endif


ifeq ($(MK_MODE),kernel)
  MK_DEFBASE := 0x10000
  MK_LIBS := $(addprefix $(DDK_PATH_LIB)/, $(MK_DDKLIBS) $(TARGET_DDKLIBS))
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


TARGET_CFLAGS += $(MK_CFLAGS)
TARGET_CFLAGS += -pipe -march=$(ARCH)
ifeq ($(DBG),1)
TARGET_ASFLAGS += -g
TARGET_CFLAGS += -g
TARGET_LFLAGS += -g
endif

TARGET_CPPFLAGS += $(MK_CPPFLAGS)
TARGET_CPPFLAGS += -pipe -march=$(ARCH)

TARGET_RCFLAGS += $(MK_RCFLAGS)

TARGET_ASFLAGS += $(MK_ASFLAGS)
TARGET_ASFLAGS += -pipe -march=$(ARCH)

TARGET_NFLAGS += $(MK_NFLAGS)


MK_GCCLIBS := $(addprefix -l, $(TARGET_GCCLIBS))

MK_FULLNAME := $(MK_BASENAME)$(MK_EXT)
MK_IMPLIB_FULLNAME := $(MK_BASENAME)$(MK_IMPLIB_EXT)

MK_NOSTRIPNAME := $(MK_BASENAME).nostrip$(MK_EXT)

# We don't want to link header files
MK_OBJECTS := $(filter-out %.h,$(TARGET_OBJECTS))
MK_STRIPPED_OBJECT := $(MK_BASENAME).stripped.o

ifeq ($(MK_IMPLIBONLY),yes)

TARGET_CLEAN += $(MK_IMPLIBPATH)/$(MK_IMPLIB_FULLNAME)

all: $(MK_IMPLIBPATH)/$(MK_IMPLIB_FULLNAME)

$(MK_IMPLIBPATH)/$(MK_IMPLIB_FULLNAME): $(TARGET_OBJECTS)
	$(DLLTOOL) \
		--dllname $(MK_FULLNAME) \
		--def $(MK_DEFNAME) \
		--output-lib $(MK_IMPLIBPATH)/$(MK_BASENAME).a \
		--kill-at

else # MK_IMPLIBONLY


all: $(MK_FULLNAME) $(MK_NOSTRIPNAME)


ifeq ($(MK_IMPLIB),yes)
  MK_EXTRACMD := --def $(MK_EDFNAME)
else
  MK_EXTRACMD :=
endif

# User mode targets
ifeq ($(MK_MODE),user)

ifeq ($(MK_EXETYPE),dll)
  TARGET_LFLAGS += -mdll -Wl,--image-base,$(TARGET_BASE)
  MK_EXTRACMD2 := -Wl,temp.exp
else
  MK_EXTRACMD2 :=
endif

$(MK_NOSTRIPNAME): $(MK_FULLRES) $(TARGET_OBJECTS) $(MK_LIBS)
ifeq ($(MK_EXETYPE),dll)
	$(CC) -Wl,--base-file,base.tmp \
		-Wl,--entry,$(TARGET_ENTRY) \
		$(TARGET_LFLAGS) \
		-o junk.tmp \
		$(MK_FULLRES) $(MK_OBJECTS) $(MK_LIBS) $(MK_GCCLIBS)
	- $(RM) junk.tmp
	$(DLLTOOL) --dllname $(MK_FULLNAME) \
		--base-file base.tmp \
		--output-exp temp.exp $(MK_EXTRACMD)
	- $(RM) base.tmp
endif
	$(CC) $(TARGET_LFLAGS) \
		-Wl,--entry,$(TARGET_ENTRY) $(MK_EXTRACMD2) \
	  -o $(MK_NOSTRIPNAME) \
	  $(MK_FULLRES) $(MK_OBJECTS) $(MK_LIBS) $(MK_GCCLIBS)
	- $(RM) temp.exp
	- $(RSYM) $(MK_NOSTRIPNAME) $(MK_BASENAME).sym

$(MK_FULLNAME): $(MK_NOSTRIPNAME)
	 $(CP) $(MK_NOSTRIPNAME) $(MK_FULLNAME)
#	 $(STRIP) --strip-debug $(MK_FULLNAME)

endif # KM_MODE

# Kernel mode targets
ifeq ($(MK_MODE),kernel)

ifeq ($(MK_IMPLIB),yes)
  MK_EXTRACMD := --def $(MK_EDFNAME)
else
  MK_EXTRACMD :=
endif

$(MK_NOSTRIPNAME): $(MK_FULLRES) $(TARGET_OBJECTS) $(MK_LIBS)
	$(CC) -Wl,--base-file,base.tmp \
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
	$(CC) $(TARGET_LFLAGS) \
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

$(MK_FULLNAME): $(MK_FULLRES) $(TARGET_OBJECTS) $(MK_LIBS) $(MK_NOSTRIPNAME)
	$(LD) -r -o $(MK_STRIPPED_OBJECT) $(MK_OBJECTS)
	$(STRIP) --strip-debug $(MK_STRIPPED_OBJECT)
	$(CC) -Wl,--base-file,base.tmp \
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
	$(CC) $(TARGET_LFLAGS) \
		-Wl,--subsystem,native \
		-Wl,--image-base,$(TARGET_BASE) \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,--entry,$(TARGET_ENTRY) \
		-Wl,temp.exp \
		-mdll -nostartfiles -nostdlib \
		-o $(MK_FULLNAME) \
	  $(MK_FULLRES) $(MK_STRIPPED_OBJECT) $(MK_LIBS) $(MK_GCCLIBS)
	- $(RM) temp.exp

endif # MK_MODE

# Static library target
ifeq ($(MK_MODE),static)

$(MK_FULLNAME): $(TARGET_OBJECTS)
	$(AR) -r $(MK_FULLNAME) $(TARGET_OBJECTS)

endif # MK_MODE

endif # MK_IMPLIBONLY


$(MK_FULLRES): $(PATH_TO_TOP)/include/reactos/buildno.h $(TARGET_PATH)/$(MK_RES_BASE).rc

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
implib:
endif

# Be carefull not to clean non-object files
MK_CLEANFILES := $(filter %.o,$(MK_OBJECTS))

clean:
	- $(RM) *.o $(MK_BASENAME).sym $(MK_BASENAME).a $(TARGET_PATH)/$(MK_RES_BASE).coff \
	  $(MK_FULLNAME) $(MK_NOSTRIPNAME) $(MK_CLEANFILES) \
	  junk.tmp base.tmp temp.exp \
	  $(TARGET_CLEAN)

ifneq ($(TARGET_HEADERS),)
$(TARGET_OBJECTS): $(TARGET_HEADERS)
endif

# install and dist rules

ifeq ($(MK_IMPLIBONLY),yes)

# Don't install import libraries

install:

dist:

else # MK_IMPLIBONLY


install: $(INSTALL_DIR)/$(MK_INSTALLDIR)/$(MK_FULLNAME)

$(INSTALL_DIR)/$(MK_INSTALLDIR)/$(MK_FULLNAME): $(MK_FULLNAME) $(MK_BASENAME).sym
	$(CP) $(MK_FULLNAME) $(INSTALL_DIR)/$(MK_INSTALLDIR)/$(MK_FULLNAME)
	$(CP) $(MK_BASENAME).sym $(INSTALL_DIR)/symbols/$(MK_BASENAME).sym


dist: $(DIST_DIR)/$(MK_DISTDIR)/$(MK_FULLNAME)

$(DIST_DIR)/$(MK_DISTDIR)/$(MK_FULLNAME): $(MK_FULLNAME)
	$(CP) $(MK_FULLNAME) $(DIST_DIR)/$(MK_DISTDIR)/$(MK_FULLNAME)
	$(CP) $(MK_BASENAME).sym $(DIST_DIR)/symbols/$(MK_BASENAME).sym


endif # MK_IMPLIBONLY


.phony: all depends implib clean install dist depends


%.o: %.c
	$(CC) $(TARGET_CFLAGS) -c $< -o $@
%.o: %.cc
	$(CC) $(TARGET_CPPFLAGS) -c $< -o $@
%.o: %.cpp
	$(CC) $(TARGET_CPPFLAGS) -c $< -o $@
%.o: %.S
	$(AS) $(TARGET_ASFLAGS) -c $< -o $@
%.o: %.s
	$(AS) $(TARGET_ASFLAGS) -c $< -o $@
%.o: %.asm
	$(NASM_CMD) $(NFLAGS) $(TARGET_NFLAGS) $< -o $@
%.coff: %.rc
	$(RC) $(TARGET_RCFLAGS) $(RCINC) $< -o $@


# Compatibility
CFLAGS := $(TARGET_CFLAGS)

# EOF
