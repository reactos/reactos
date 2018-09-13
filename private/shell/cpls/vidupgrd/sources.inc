TARGETNAME=vidupgrd
TARGETPATH=obj
TARGETTYPE=DYNLINK
TARGETEXT=dll

INCLUDES=$(BASEDIR)\private\inc

TARGETLIBS=$(BASEDIR)\public\sdk\lib\*\advapi32.lib     \
           $(BASEDIR)\public\sdk\lib\*\user32.lib       \
           $(BASEDIR)\public\sdk\lib\*\kernel32.lib     \
           $(BASEDIR)\public\sdk\lib\*\gdi32.lib        \
           $(BASEDIR)\public\sdk\lib\*\shell32.lib      

USE_CRTDLL=1

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF

MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX
C_DEFINES=-DWIN32 -DW3 -DUNICODE -D_UNICODE 

SOURCES=..\migrate.cpp 

DLLDEF=$(O)\vidupgrd.def
