MAJORCOMP=sdktools
MINORCOMP=imagehlp

TARGETPATH=obj
TARGETPATHLIB=$(BASEDIR)\public\sdk\lib
TARGETTYPE=DYNLINK
#DLLENTRY=ImageHlpDllEntry
DLLENTRY=_DllMainCRTStartup
DLLDEF=$(O)\imagehlp.def

INCLUDES=..; \
         $(BASEDIR)\private\sdktools\vctools\pdb6\langapi\include; \

USE_MSVCRT=1
NO_NTDLL=1

TARGETLIBS=\
    $(BASEDIR)\public\sdk\lib\*\kernel32.lib \
    ..\*\msdbi60L.lib \
    #(BASEDIR)\public\sdk\lib\*\libcmt.lib

MSC_WARNING_LEVEL=/W3 /WX

C_DEFINES=$(C_DEFINES) -DCC_CVTYPE32 -D_VC_VER_INC -DIMAGEHLPSRC

UMTYPE=console
SUBSYSTEM_VERSION=4.00
