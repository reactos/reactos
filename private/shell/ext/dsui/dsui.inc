#
# DSUI project global sources information
#
# This sets the include, librarys and a set of compile time flags
# that are project wide.  A majority of it is to cope with
# the cross platform (win95,98, NT4 and NT5 nature of the product).
#

DSUI_DIR=   $(BASEDIR)\private\shell\ext\dsui

INCLUDES=   ..;                             \
            ..\..\inc;                      \
            $(BASEDIR)\private\inc;         \
            $(BASEDIR)\private\windows\inc; \
            $(BASEDIR)\private\shell\inc;

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

USE_ATL=1
USE_NOLIBS=1
USE_MSVCRT=1
USE_NATIVE_EH=1

UMTYPE=windows
DLLENTRY=_DllMainCRTStartup


#
# Platform specific build features
#

!IF "$(PLATFORM)"=="WINNT"
REGISTER_NSE=1
DELEGATE=0
DOWNLEVEL_SHELL = 0
C_DEFINES=$(C_DEFINES) -DUNICODE
LIBRARY_PLATFORM_PATH=$(BASEDIR)\public\sdk\lib
!ELSE
REGISTER_NSE=0
DELEGATE=0
DOWNLEVEL_SHELL = 1
CHICAGO_PRODUCT = 1
LIBRARY_PLATFORM_PATH = $(BASEDIR)\public\sdk\lib\chicago
ALT_PROJECT_TARGET = win95
SUBSYSTEM_VERSION = 4.00
!ENDIF


#
# We need new comctl32 features, makefile.def doesn't define this
# currently so lets ensure that it is what we want
#

!if !defined(WIN32_WIN95_VERSION)
WIN32_WIN95_VERSION=0x0400
!endif
!if !defined(WIN32_WINNT_VERSION)
WIN32_WINNT_VERSION=0x500
!endif
!if !defined(WIN32_IE_VERSION)
WIN32_IE_VERSION=0x0500
!endif


#
# For the downlevel builds we need to build our private shell functions
# along with linking to different shell32.lib files.
#
# For NT4:      shell32.nt4
# For Win9x:    shell32.w95
#
# When building for NT5 we can just assume the features we want are exported
# from the existing shell32.dll.
#

C_DEFINES=  $(C_DEFINES) -DDOWNLEVEL_SHELL=$(DOWNLEVEL_SHELL) -DDELEGATE=$(DELEGATE) -DREGISTER_NSE=$(REGISTER_NSE)
RC_DEFINES= $(RC_DEFINES) /DDOWNLEVEL_SHELL=$(DOWNLEVEL_SHELL)

!if "$(DOWNLEVEL_SHELL)"=="1"

SHELL_LIBS= $(BASEDIR)\private\lib\chicago\*\SHELL32.W95

!else

SHELL_LIBS= $(LIBRARY_PLATFORM_PATH)\*\shell32p.lib   \
            $(LIBRARY_PLATFORM_PATH)\*\user32p.lib    \

#
# On NT5 we use the linkers DELAYLOAD switch to reduce working set,
# to safely use this we also link to shlwapis delay load helper function
# that handles any failures that may occur during fix up of these exports.
#

#DELAYLOAD=  activeds.dll

!endif
