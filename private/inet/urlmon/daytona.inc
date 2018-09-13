# This is the global include file for the daytona version of CairOLE.
# It is included by all project sources files.

C_DEFINES=    \
              $(C_DEFINES)   \
              -DFLAT         \
              -DWIN32=100    \
              -D_NT1X_=100   \
              -DINC_OLE2     \
              -DNOEXCEPTIONS \
	      -DNASHVILLE    \
              $(TRACELOG)


BLDCRT=       1

!IF "$(FREEBUILD)" == "0"
C_DEFINES=    \
              $(C_DEFINES)          \
              -DDBGASSERT=1
!ENDIF

!IFDEF USE_PERFTAGS
!IF $(FREEBUILD)
C_DEFINES = $(C_DEFINES) -DPERFTAGS
!ENDIF
!ENDIF

!if !defined(CLEANINF_FLAGS)
# Cleaninf flags
CLEANINF_FLAGS  = -w
!endif

# Cleaninf utility
CLEANINF        = cleaninf.exe $(CLEANINF_FLAGS)


# Do this to assure that we run on NT 4.0 and Win95, even with
# NT5/Memphis headers.
#
WIN32_WINNT_VERSION=0x0400
WIN32_WIN95_VERSION=0x0400


# For the Daytona build, we do not want statically linked compiler runtimes,
# so leave this commented out.
#
# USE_CRTDLL=1
# USE_NOLIBS= 1
USE_NOLIBS= 1

# Use the stripped down version for the crt
USE_IERT=1

#
# Linker flags
#

!if "$(BUILD_PRODUCT)" == "IE"
USE_WIN98OPT=1
!endif

MSC_WARNING_LEVEL=/W3 /WX

NTLEGO=1

GPCH_BUILD=..\..\mon\daytona

DELAYLOAD=WININET.DLL;SHELL32.DLL;RPCRT4.DLL;MPR.DLL

