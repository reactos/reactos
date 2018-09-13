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

!IF "$(ALMOSTRETAIL)"!="" && $(FREEBUILD)
C_DEFINES=    \
              $(C_DEFINES)          \
              -DDBGASSERT=1
!ENDIF

USE_MAPSYM=1

# For the Daytona build, we do not want statically linked compiler runtimes,
# so leave this commented out.
#
# USE_LIBCMT=   1
# USE_CRTDLL=1
USE_NOLIBS = 1

MSC_WARNING_LEVEL=/W3 /WX

GPCH_BUILD=daytona
