

MAJORCOMP=shell
MINORCOMP=accesory

TARGETNAME=mspaint
TARGETPATH=obj
TARGETTYPE=PROGRAM

INCLUDES=..;..\res;.

C_DEFINES=-DWIN32 -DNT -D_WINDOWS -DUSE_MIRRORING

MSC_WARNING_LEVEL=/W3 /WX

SOURCES= \
    ..\bar.cpp\
    ..\bmobject.cpp\
    ..\cmpmsg.cpp\
    ..\colorsrc.cpp\
    ..\docking.cpp\
    ..\global.cpp\
    ..\imageatt.cpp\
    ..\imgbrush.cpp\
    ..\imgcolor.cpp\
    ..\imgcpyps.cpp\
    ..\imgdlgs.cpp\
    ..\imgfile.cpp\
    ..\imgsuprt.cpp\
    ..\imgtools.cpp\
    ..\imgwell.cpp\
    ..\imgwnd.cpp\
    ..\ipframe.cpp\
    ..\minifwnd.cpp\
    ..\pbrusdoc.cpp\
    ..\pbrusfrm.cpp\
    ..\pbrush.cpp\
    ..\pbrusvw.cpp\
    ..\pictures.cpp\
    ..\printres.cpp\
    ..\rotate.cpp\
    ..\settings.cpp\
    ..\skew.cpp\
    ..\sprite.cpp\
    ..\srvritem.cpp\
    ..\tedit.cpp\
    ..\tfont.cpp\
    ..\thumnail.cpp\
    ..\toolbox.cpp\
    ..\tracker.cpp\
    ..\t_curve.cpp\
    ..\t_fhsel.cpp\
    ..\t_poly.cpp\
    ..\t_text.cpp\
    ..\undo.cpp\
    ..\mspaint.rc \
    ..\loadimag.cpp\
    ..\saveimag.cpp\

# Add interlac.cpp if the PNG support is ever fixed

USE_NATIVE_EH=1

UMTYPE=windows
PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\stdafx.h
PRECOMPILED_PCH=stdafx.pch
PRECOMPILED_OBJ=stdafx.obj

TARGETLIBS=$(BASEDIR)\public\sdk\lib\*\kernel32.lib \
           $(BASEDIR)\public\sdk\lib\*\gdi32.lib \
           $(BASEDIR)\public\sdk\lib\*\user32.lib \
           $(BASEDIR)\public\sdk\lib\*\advapi32.lib \
           $(BASEDIR)\public\sdk\lib\*\comdlg32.lib\
           $(BASEDIR)\public\sdk\lib\*\ole32.lib\
           $(BASEDIR)\public\sdk\lib\*\winspool.lib\
           $(BASEDIR)\public\sdk\lib\*\uuid.lib \
           $(BASEDIR)\public\sdk\lib\*\oleaut32.lib \
           $(BASEDIR)\public\sdk\lib\*\shell32.lib\
           $(BASEDIR)\public\sdk\lib\*\htmlhelp.lib\
           $(BASEDIR)\public\sdk\lib\*\imm32.lib\
           ..\ofn\obj\*\ofn.lib



!if $(386)
TARGETLIBS=$(TARGETLIBS) \
    ..\image.lib \
    $(BASEDIR)\public\sdk\lib\*\oldnames.lib
!endif
