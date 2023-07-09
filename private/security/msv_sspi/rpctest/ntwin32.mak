# Win32 NMAKE definitions


!IF "$(CPU)" == "i386"

# Debug switches are default for current release
#
# These switches allow for source level debugging
# with NTSD for local and global variables.


CPUTYPE=1
cdebug = -Zd -Od -Oy-

cc = cl
cflags	= -c -G3d -W3 -Di386=1 $(cdebug)

!ENDIF

!IF "$(CPU)" == "MIPS"
#declarations for use on self hosted MIPS box.

CPUTYPE=2
cc = cl
cflags	= -c -W3 -DMIPS=1
!ENDIF

!IF "$(CPU)" == "PPC"
#declarations for use on self hosted PPC box.

CPUTYPE=3
cc = mcl
cflags	= -c -G3d -W3 -DPPC=1
!ENDIF

!IFNDEF CPUTYPE
!ERROR  Must specify CPU Environment Variable (i386 or MIPS or PPC )
!ENDIF


#Universal declaration

cvars = -DWIN32
linkdebug = -debug:full -debugtype:coff
link = link $(linkdebug)

# link flags - must be specified after $(link)
#
# conflags : creating a character based console application
# guiflags : creating a GUI based "Windows" application

conflags =  -subsystem:console -entry:mainCRTStartup
guiflags =  -subsystem:windows -entry:WinMainCRTStartup

# Link libraries - system import and C runtime libraries
#
# conlibs : libraries to link with for a console application
# guilibs : libraries to link with for a "Windows" application
#
# note : $(LIB) is set in environment variables

conlibs = $(LIB)\libcmt.lib $(LIB)\*.lib

guilibs = $(LIB)\libcmt.lib $(LIB)\*.lib
