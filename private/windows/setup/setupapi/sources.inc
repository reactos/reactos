MAJORCOMP=setup

TARGETPATH=obj
TARGETPATHLIB=$(BASEDIR)\public\sdk\lib
TARGETTYPE=DYNLINK

!include $(BASEDIR)\private\windows\setup\sources.inc

INCLUDES=\
    ..;\
    ..\..\inc;\
    $(BASEDIR)\private\windows\inc

C_DEFINES=$(C_DEFINES) -DDBGHEAP_CHECK

DLLDEF=setupapi.def
DLLBASE=0x2000000
DLLENTRY=_DllMainCRTStartup

MSC_WARNING_LEVEL=/W3 /WX

#
# setupapi can be run over the net; this flag forces the file to be copied
# to the local pagefile and protects against inpage i/o errors
#
LINKER_FLAGS=/SWAPRUN:CD /SWAPRUN:NET

SOURCES=\
    ..\backup.c    \
    ..\cntxtlog.c  \
    ..\copy.c      \
    ..\debug.c     \
    ..\decomp.c    \
    ..\devclass.c  \
    ..\devdrv.c    \
    ..\devicon.c   \
    ..\devinfo.c   \
    ..\devinst.c   \
    ..\devinstd.c  \
    ..\devoem.c    \
    ..\devprop.c   \
    ..\devreg.c    \
    ..\devres.c    \
    ..\devres1.c   \
    ..\devres2.c   \
    ..\devwiz.c    \
    ..\diamond.c   \
    ..\diansicv.c  \
    ..\diskspac.c  \
    ..\diutil.c    \
    ..\dll.c       \
    ..\filelog.c   \
    ..\fileq1.c    \
    ..\fileq2.c    \
    ..\fileq3.c    \
    ..\fileq4.c    \
    ..\fileq5.c    \
    ..\fileq6.c    \
    ..\fileqcb.c   \
    ..\fileutil.c  \
    ..\infflist.c  \
    ..\infinst.c   \
    ..\infline.c   \
    ..\infload.c   \
    ..\inflogcf.c  \
    ..\infold.c    \
    ..\infopenv.c  \
    ..\infsdisk.c  \
    ..\infvalue.c  \
    ..\logapi.c    \
    ..\memory.c    \
    ..\miscutil.c  \
    ..\mru.c       \
    ..\prompt.c    \
    ..\registry.c  \
    ..\resource.c  \
    ..\security.c  \
    ..\setupapi.rc \
    ..\strtab.c    \
    ..\stub.c

UMRES=obj\*\setupapi.res
UMTYPE=windows
UMENTRY=winmain

NTTARGETFILE0=msg.h msg.rc

PRECOMPILED_INCLUDE=precomp.h
