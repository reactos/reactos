# Time-stamp: <04/03/31 07:55:19 ptr>
# $Id$

!if "$(TARGET_PROC)" == ""
!error No target processor configured! Please rerun configure.bat!
!endif

!if "$(CC)" == ""
!error CC not set, run the proper WCE*.bat from this shell to set it!
!endif

# All the batchfiles to setup the environment yield different
# compilers which they put into CC.
CXX = $(CC)

DEFS_COMMON = $(DEFS_COMMON) /D _WIN32_WCE=$(CEVERSION) /D UNDER_CE=$(CEVERSION) /D "UNICODE"
LDFLAGS_COMMON = $(LDFLAGS_COMMON) coredll.lib corelibc.lib /nodefaultlib:LIBC.lib /nodefaultlib:OLDNAMES.lib
LDFLAGS_COMMON = $(LDFLAGS_COMMON) /stack:0x10000,0x1000 /subsystem:WINDOWSCE /align:"4096"

# increase compiler memory in order to compile deeply nested template code
OPT_STLDBG = $(OPT_STLDBG) /Zm800
OPT_STATIC_STLDBG = $(OPT_STATIC_STLDBG) /Zm800

# activate global (whole program) optimizations
OPT_REL = $(OPT_REL) /Og
OPT_STATIC_REL = $(OPT_STATIC_REL) /Og

# ARM specific settings
!if "$(TARGET_PROC)" == "arm"
DEFS_COMMON = $(DEFS_COMMON) /D "ARM" /D "_ARM_" /D "ARMV4"
OPT_STATIC_STLDBG = $(OPT_STATIC_STLDBG) /Zm800
OPT_COMMON = $(OPT_COMMON)
# TODO: eVC4 IDE uses ARM for ARMV4 and THUMB for ARMV4I and ARMV4T
LDFLAGS_COMMON = $(LDFLAGS_COMMON) /MACHINE:ARM
# RTTI patch for PPC2003 SDK
!if "$(PLATFORM)" == "POCKET PC 2003"
LDFLAGS_COMMON = $(LDFLAGS_COMMON) ccrtrtti.lib
!endif
!endif

# x86 specific settings
!if "$(TARGET_PROC)" == "x86"
DEFS_COMMON = $(DEFS_COMMON) /D "x86" /D "_X86_" /D "_i386_"
OPT_COMMON = $(OPT_COMMON) /Gs8192
LDFLAGS_COMMON = $(LDFLAGS_COMMON) $(CEx86Corelibc) /MACHINE:X86
!if "$(TARGET_PROC_SUBTYPE)" == "emulator"
DEFS_COMMON = $(DEFS_COMMON) /D "_STLP_WCE_TARGET_PROC_SUBTYPE_EMULATOR"
!endif
!if "$(PLATFORM)" == "POCKET PC 2003"
# RTTI patch for PPC2003 SDK
LDFLAGS_COMMON = $(LDFLAGS_COMMON) ccrtrtti.lib
!endif
!endif

# MIPS specific settings
!if "$(TARGET_PROC)" == "mips"
DEFS_COMMON = $(DEFS_COMMON) /D "_MIPS_" /D "MIPS" /D "$(TARGET_PROC_SUBTYPE)"
OPT_COMMON = $(OPT_COMMON)

# Note: one might think that MIPSII_FP and MIPSIV_FP should use /MACHINE:MIPSFPU
# while MIPSII and MIPSIV should use /MACHINE:MIPS, but this is exactly how the
# eVC4 IDE does it.
!if "$(TARGET_PROC_SUBTYPE)" == ""
!error "MIPS subtype not set"
!elseif "$(TARGET_PROC_SUBTYPE)" == "MIPS16"
LDFLAGS_COMMON = $(LDFLAGS_COMMON) /MACHINE:MIPS
!elseif "$(TARGET_PROC_SUBTYPE)" == "MIPSII"
OPT_COMMON = $(OPT_COMMON) /QMmips2 /QMFPE
LDFLAGS_COMMON = $(LDFLAGS_COMMON) /MACHINE:MIPS
!elseif "$(TARGET_PROC_SUBTYPE)" == "MIPSII_FP"
OPT_COMMON = $(OPT_COMMON) /QMmips2 /QMFPE-
LDFLAGS_COMMON = $(LDFLAGS_COMMON) /MACHINE:MIPS
!elseif "$(TARGET_PROC_SUBTYPE)" == "MIPSIV"
OPT_COMMON = $(OPT_COMMON) /QMmips4 /QMn32 /QMFPE
LDFLAGS_COMMON = $(LDFLAGS_COMMON) /MACHINE:MIPSFPU
!elseif "$(TARGET_PROC_SUBTYPE)" == "MIPSIV_FP"
OPT_COMMON = $(OPT_COMMON) /QMmips4 /QMn32 /QMFPE-
LDFLAGS_COMMON = $(LDFLAGS_COMMON) /MACHINE:MIPSFPU
!else
!error "unknown MIPS subtype"
!endif

!endif

# SH3 specific settings
!if "$(TARGET_PROC)" == "sh3"
DEFS_COMMON = $(DEFS_COMMON) /D "SH3" /D "_SH3_" /D "SHx"
OPT_COMMON = $(OPT_COMMON)
LDFLAGS_COMMON = $(LDFLAGS_COMMON) /MACHINE:SH3
!endif

# SH4 specific settings
!if "$(TARGET_PROC)" == "sh4"
DEFS_COMMON = $(DEFS_COMMON) /D "SH4" /D "_SH4_" /D "SHx"
OPT_COMMON = $(OPT_COMMON) /Qsh4
LDFLAGS_COMMON = $(LDFLAGS_COMMON) /MACHINE:SH4
!endif


# exception handling support
CFLAGS_COMMON = /nologo /TC /W4 /GF /GR /GX
CFLAGS_REL = $(CFLAGS_COMMON) $(OPT_REL)
CFLAGS_STATIC_REL = $(CFLAGS_COMMON) $(OPT_STATIC_REL)
CFLAGS_DBG = $(CFLAGS_COMMON) $(OPT_DBG)
CFLAGS_STATIC_DBG = $(CFLAGS_COMMON) $(OPT_STATIC_DBG)
CFLAGS_STLDBG = $(CFLAGS_COMMON) $(OPT_STLDBG)
CFLAGS_STATIC_STLDBG = $(CFLAGS_COMMON) $(OPT_STATIC_STLDBG)
CXXFLAGS_COMMON = /nologo /TP /W4 /GF /GR /GX
CXXFLAGS_REL = $(CXXFLAGS_COMMON) $(OPT_REL)
CXXFLAGS_STATIC_REL = $(CXXFLAGS_COMMON) $(OPT_STATIC_REL)
CXXFLAGS_DBG = $(CXXFLAGS_COMMON) $(OPT_DBG)
CXXFLAGS_STATIC_DBG = $(CXXFLAGS_COMMON) $(OPT_STATIC_DBG)
CXXFLAGS_STLDBG = $(CXXFLAGS_COMMON) $(OPT_STLDBG)
CXXFLAGS_STATIC_STLDBG = $(CXXFLAGS_COMMON) $(OPT_STATIC_STLDBG)

!include evc-common.mak
