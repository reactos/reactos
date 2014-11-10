# build/Makefiles/nmake/evc9.mak

# Note: _WIN32_WCE is defined as 420 for CE 4.2 but as 0x500 for CE 5.0!
DEFS_COMMON = $(DEFS_COMMON) /D _WIN32_WCE=0x$(CEVERSION) /D UNDER_CE=1 /D "UNICODE" 
LDFLAGS_COMMON = $(LDFLAGS_COMMON) coredll.lib corelibc.lib /nodefaultlib:LIBC.lib /nodefaultlib:OLDNAMES.lib 
!if "$(PLATFORM)" == "POCKET PC 2003"
LDFLAGS_COMMON = $(LDFLAGS_COMMON) /subsystem:windowsce,4.20
!else
# TODO: the subsystem settings will have to be adjusted for CE5.01...
LDFLAGS_COMMON = $(LDFLAGS_COMMON) /subsystem:windowsce,5.00
!endif

!if "$(TARGET_PROC)" == ""
!error No target processor configured! Please rerun configure.bat!
!endif

!if "$(CC)" == ""
CC=cl.exe
!endif

CXX = $(CC)

# activate global optimisations (aka Link Time Code Generation)
OPT_REL = $(OPT_REL) /GL
LDFLAGS_REL = $(LDFLAGS_REL) /LTCG


# make the compiler display absolute paths in diagnostics
# While this is not necessary for STLport in any way, it is convenient when using
# the VC8 IDE for building things because then you can click on diagnostics in
# order to warp to the exact place in the code.
OPT_COMMON = $(OPT_COMMON) /FC


# ARM specific settings
!if "$(TARGET_PROC)" == "arm"
DEFS_COMMON = $(DEFS_COMMON) /D "ARM" /D "_ARM_" /D "$(TARGET_PROC_SUBTYPE)"
OPT_COMMON = $(OPT_COMMON)
!if "$(PLATFORM)" == "POCKET PC 2003"
DEFS_COMMON = $(DEFS_COMMON) /DWIN32_PLATFORM_PSPC
# Pocket PC 2003 doesn't support THUMB.
LDFLAGS_COMMON = $(LDFLAGS_COMMON) ccrtrtti.lib secchk.lib /machine:ARM
!endif
!endif

# x86 specific settings
!if "$(TARGET_PROC)" == "x86"
DEFS_COMMON = $(DEFS_COMMON) /D "x86" /D "_X86_"
OPT_COMMON = $(OPT_COMMON)
!endif

# MIPS specific settings
!if "$(TARGET_PROC)" == "mips"
DEFS_COMMON = $(DEFS_COMMON) /D "MIPS" /D "_MIPS_" /D "$(TARGET_PROC_SUBTYPE)"
!if "$(TARGET_PROC_SUBTYPE)" == ""
!error "MIPS subtype not set"
!elseif "$(TARGET_PROC_SUBTYPE)" == "MIPSII"
OPT_COMMON = $(OPT_COMMON) /QMmips2
!else
!error "unknown MIPS subtype"
!endif
!endif

# SH4 specific settings
!if "$(TARGET_PROC)" == "sh4"
DEFS_COMMON = $(DEFS_COMMON) /D "SHx" /D "_SHX_" /D "SH4"
OPT_COMMON = $(OPT_COMMON)
!endif


# Note: /GX for MSC<14 has been replaced with /EHsc
CFLAGS_COMMON = /nologo /TC /WX /GF /GR /EHsc
CFLAGS_REL = $(CFLAGS_COMMON) $(OPT_REL)
CFLAGS_STATIC_REL = $(CFLAGS_COMMON) $(OPT_STATIC_REL)
CFLAGS_DBG = $(CFLAGS_COMMON) $(OPT_DBG)
CFLAGS_STATIC_DBG = $(CFLAGS_COMMON) $(OPT_STATIC_DBG)
CFLAGS_STLDBG = $(CFLAGS_COMMON) $(OPT_STLDBG)
CFLAGS_STATIC_STLDBG = $(CFLAGS_COMMON) $(OPT_STATIC_STLDBG)
CXXFLAGS_COMMON = /nologo /TP /WX /GF /GR /EHsc
CXXFLAGS_REL = $(CXXFLAGS_COMMON) $(OPT_REL)
CXXFLAGS_STATIC_REL = $(CXXFLAGS_COMMON) $(OPT_STATIC_REL)
CXXFLAGS_DBG = $(CXXFLAGS_COMMON) $(OPT_DBG)
CXXFLAGS_STATIC_DBG = $(CXXFLAGS_COMMON) $(OPT_STATIC_DBG)
CXXFLAGS_STLDBG = $(CXXFLAGS_COMMON) $(OPT_STLDBG)
CXXFLAGS_STATIC_STLDBG = $(CXXFLAGS_COMMON) $(OPT_STATIC_STLDBG)

# setup proper runtime (static/dynamic, debug/release)
!ifdef WITH_STATIC_RTL
OPT_DBG = $(OPT_DBG) /MTd
OPT_STLDBG = $(OPT_STLDBG) /MTd
OPT_REL = $(OPT_REL) /MT
DEFS_REL = $(DEFS_REL) /D_STLP_USE_DYNAMIC_LIB
DEFS_DBG = $(DEFS_DBG) /D_STLP_USE_DYNAMIC_LIB
DEFS_STLDBG = $(DEFS_STLDBG) /D_STLP_USE_DYNAMIC_LIB
!else
OPT_DBG = $(OPT_DBG) /MDd
OPT_STLDBG = $(OPT_STLDBG) /MDd
OPT_REL = $(OPT_REL) /MD
!endif


!include evc-common.mak
