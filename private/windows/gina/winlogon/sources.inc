TARGETNAME=winlogon
TARGETPATH=obj
TARGETTYPE=PROGRAM

SOURCES_USED=$(SOURCES_USED) ..\sources.inc

INCLUDES=\
    ..\; \
    ..\$(TARGET_DIRECTORY); \
    $(BASEDIR)\private\windows\inc; \
    $(BASEDIR)\private\inc; \
    $(BASEDIR)\private\windows\screg\winreg; \
    $(BASEDIR)\private\inc\crypto; \
    $(BASEDIR)\public\sdk\inc\hydra

C_DEFINES=-DUNICODE -D_UNICODE

!ifndef MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=-W3 -WX
!endif

BACKGROUND_USE=1
LINKER_STACKCOMMITSIZE=8192

SOURCES= \
         ..\access.c       \
         ..\autoenrl.c     \
         ..\debug.c        \
         ..\callback.c     \
         ..\connect.c      \
         ..\envvar.c       \
         ..\ginamgr.c      \
         ..\logoff.c       \
         ..\misc.c         \
         ..\msgalias.c     \
         ..\provider.c     \
         ..\removabl.c     \
         ..\res.rc         \
         ..\sas.c          \
         ..\scrnsave.c     \
         ..\secutil.c      \
         ..\setup.c        \
         ..\shell.c        \
         ..\sysinit.c      \
         ..\sysshut.c      \
         ..\termutil.c     \
         ..\timeout.c      \
         ..\usrenv.c       \
         ..\usrpro.c       \
         ..\win31mig.c     \
         ..\winlogon.c     \
         ..\winutil.c      \
         ..\wlx.c          \
         ..\rng.c          \
         ..\secboot.c      \
         ..\notify.c       \
         ..\sc.c           \
         ..\wlxutil.c      \
         ..\jobwait.c      \
         ..\samwait.c      \
         ..\shutinit.c     \
         ..\wlevents.mc


#
# Note:  this gets overridden by i386\sources, but in the event that
# we get rid of os2 support, this will keep things working
#

I386_SOURCES=..\nddeagnt.c
ALPHA_SOURCES=..\nddeagnt.c

USE_CRTDLL=1

UMTYPE=windows
UMENTRY=winmain
LINKLIBS= \
         $(BASEDIR)\public\sdk\lib\*\rpcutil.lib \
         $(BASEDIR)\public\sdk\lib\*\rpcrt4.lib \
         $(BASEDIR)\public\sdk\lib\*\rpcndr.lib \
         $(BASEDIR)\public\sdk\lib\*\netapi32.lib \
         $(BASEDIR)\public\sdk\lib\*\lsadll.lib \
         $(BASEDIR)\public\sdk\lib\*\user32p.lib \
         $(BASEDIR)\public\sdk\lib\*\gdi32p.lib \
         $(BASEDIR)\public\sdk\lib\*\kernl32p.lib \
         $(BASEDIR)\public\sdk\lib\*\ntdll.lib \
         $(BASEDIR)\public\sdk\lib\*\winmm.lib \
         $(BASEDIR)\public\sdk\lib\*\userenv.lib \
!if "$(WIN64)" == "0"
         $(BASEDIR)\public\sdk\lib\*\nddeapi.lib \
!endif
         $(BASEDIR)\public\sdk\lib\*\crypt32.lib \
         $(BASEDIR)\public\sdk\lib\*\certcli.lib \
         $(BASEDIR)\public\sdk\lib\*\wintrust.lib \
         $(BASEDIR)\public\sdk\lib\*\sfc.lib \
         $(BASEDIR)\public\sdk\lib\*\setupapi.lib \
         $(BASEDIR)\public\sdk\lib\*\ole32.lib \
         $(BASEDIR)\public\sdk\lib\*\secur32.lib \
         $(BASEDIR)\private\windows\gina\profmap\obj\*\profmap.lib \
         $(BASEDIR)\private\lib\*\rsa32.lib \
         $(BASEDIR)\private\security\wxlpc\obj\*\wxapisrv.lib

OPTIONAL_UMTEST=tnp

DELAYLOAD=winmm.dll;certcli.dll;netapi32.dll;ole32.dll;crypt32.dll
DLOAD_ERROR_HANDLER=kernel32
