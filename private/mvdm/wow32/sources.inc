INDENTED_DIRECTIVES=1

DLLORDER=..\wow32.prf



MAJORCOMP=mvdm
MINORCOMP=wow32

TARGETNAME=wow32
TARGETTYPE=DYNLINK

TARGETPATH=obj
TARGETPATHLIB=$(BASEDIR)\public\sdk\lib\$(ALT_PROJECT_TARGET)
TARGETLIBS=$(BASEDIR)\public\sdk\lib\*\kernl32p.lib \
           $(BASEDIR)\public\sdk\lib\*\gdi32p.lib \
           $(BASEDIR)\public\sdk\lib\*\user32p.lib \
           $(BASEDIR)\public\sdk\lib\*\advapi32.lib \
           $(BASEDIR)\public\sdk\lib\*\shell32.lib \
           $(BASEDIR)\public\sdk\lib\*\ntvdm.lib \
           $(BASEDIR)\public\sdk\lib\*\oemuni.lib \
           $(BASEDIR)\public\sdk\lib\*\comdlg32.lib \
           $(BASEDIR)\public\sdk\lib\*\version.lib

DLLENTRY=W32DllInitialize
DLLBASE=0xffb0000
DLLDEF=obj\*\wow32.def

NTTARGETFILES=
NTTARGETFILE0=wowit.h wowit.c

INCLUDES=..;..\..\inc;..\..\..\windows\inc;$(BASEDIR)\private\inc;..\..\..\inc;..\..\vdd\h; \
         $(BASEDIR)\private\ntos\w32\ntgdi\inc

USE_NTDLL = 1

SOURCES= \
    ..\wow32.c     \
    ..\wowtbl.c    \
    ..\intthunk.c  \
    wowit.c        \
    ..\win95.c     \
    ..\mapembed.c  \
    ..\walias.c    \
    ..\wstruc.c    \
    ..\wcall16.c   \
    ..\wcall32.c   \
    ..\wcntl32.c   \
    ..\wdde.c      \
    ..\wres16.c    \
    ..\wres32.c    \
    ..\wmsg16.c    \
    ..\wmdisp32.c  \
    ..\wmsgbm.c    \
    ..\wmsgcb.c    \
    ..\wmsgem.c    \
    ..\wmsglb.c    \
    ..\wmsgsbm.c   \
    ..\wmtbl32.c   \
    ..\wgdi31.c    \
    ..\wgman.c     \
    ..\wgdi.c      \
    ..\wgfont.c    \
    ..\wgmeta.c    \
    ..\wgpal.c     \
    ..\wgprnset.c  \
    ..\wgtext.c    \
    ..\wkbman.c    \
    ..\wkmem.c     \
    ..\wkgthunk.c  \
    ..\wkman.c     \
    ..\wkfileio.c  \
    ..\wkernel.c   \
    ..\wow32fax.c  \
    ..\wsman.c     \
    ..\wshell.c    \
    ..\wuser31.c   \
    ..\wuman.c     \
    ..\wucaret.c   \
    ..\wuclass.c   \
    ..\wuclip.c    \
    ..\wucomm.c    \
    ..\wucursor.c  \
    ..\wudlg.c     \
    ..\wuhook.c    \
    ..\wulang.c    \
    ..\wumenu.c    \
    ..\wumsg.c     \
    ..\wuser.c     \
    ..\wutext.c    \
    ..\wutmr.c     \
    ..\wuwind.c    \
    ..\wowhooks.c  \
    ..\wdos.c      \
    ..\wreldc.c    \
    ..\wcuricon.c  \
    ..\wsubcls.c   \
    ..\wmmedia.c   \
    ..\wmmedia1.c  \
    ..\wmmedia2.c  \
    ..\wmmstruc.c  \
    ..\wmmstru1.c  \
    ..\wmmstru2.c  \
    ..\wmmalias.c  \
    ..\walloc16.c  \
    ..\waccel.c    \
    ..\wsdata.c    \
    ..\wsext.c     \
    ..\wsraw.c     \
    ..\wthman.c    \
    ..\wcommdlg.c  \
    ..\wheap.c     \
    ..\wspool.c    \
    ..\wusercli.c  \
    ..\wdib.c      \
    ..\wole2.c     \
    ..\wparam.c    \
    ..\wow32.rc
!if "$(ALT_PROJECT)" == "CHS" || "$(ALT_PROJECT)" == "CHT" || "$(ALT_PROJECT)" == "CHP" || "$(ALT_PROJECT)" == "KOR" || "$(ALT_PROJECT)" == "JPN" || "$(ALT_PROJECT)" == "NEC_98"
SOURCES=$(SOURCES) \
    ..\wnman.c     \
    ..\wwmman.c
!endif

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

!IF "$(WOWPROFILE)" == ""
C_DEFINES=-DWIN_32
ASM_DEFINES=-DWIN_32
!ELSE
C_DEFINES=-DWIN_32 -DWOWPROFILE
ASM_DEFINES=-DWIN_32 -DWOWPROFILE
!ENDIF


UMTYPE=windows
UMTEST=
UMRES=..\obj\$(ALT_PROJECT_TARGET)\*\wow32.res
UMLIBS=

!IFNDEF MAKE_GENWOWIT
PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj
!ENDIF

#
# Let build.exe know that subwtype.h is included under #ifdef (by d3dtypes.h)
# so it won't complain if it can't be found.
#

CONDITIONAL_INCLUDES=subwtype.h

W32_SB=1

SOURCES_USED=..\sources.inc
