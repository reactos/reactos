..\precomp.h: ..\consrv.h ..\cmdline.h ..\globals.h ..\menu.h ..\..\inc\conmsg.h \
    ..\..\inc\server.h ..\..\inc\font.h $(BASEDIR)\private\inc\winconp.h $(BASEDIR)\private\inc\winbasep.h \
    ..\..\..\w32inc\winuserk.h $(BASEDIR)\private\inc\winuserp.h \
    $(BASEDIR)\private\ntos\w32\ntuser\server\usersrv.h

#
# From here down is a hack to build conexts.dll
#

EXT_DEF=..\conexts.def

EXT_LIBS=$(BASEDIR)\public\sdk\lib\*\ntdll.lib\
         $(LINKLIBS)

EXT_OBJS=obj\*\conexts.res\
         obj\*\conexts.obj

EXT_TARGET=obj\*\conexts

!IF $(386)
EXT_LIBS=$(EXT_LIBS:*=i386)
EXT_OBJS=$(EXT_OBJS:*=i386)
EXT_TARGET=$(EXT_TARGET:*=i386)
!ELSEIF $(IA64)
EXT_LIBS=$(EXT_LIBS:*=ia64)
EXT_OBJS=$(EXT_OBJS:*=ia64)
EXT_TARGET=$(EXT_TARGET:*=ia64)
!ELSEIF $(ALPHA)
EXT_LIBS=$(EXT_LIBS:*=alpha)
EXT_OBJS=$(EXT_OBJS:*=alpha)
EXT_TARGET=$(EXT_TARGET:*=alpha)
!ELSEIF $(AXP64)
EXT_LIBS=$(EXT_LIBS:*=axp64)
EXT_OBJS=$(EXT_OBJS:*=axp64)
EXT_TARGET=$(EXT_TARGET:*=axp64)
!ENDIF

!IF "$(NOLINK)" == ""
all: $(EXT_TARGET).dll
!ELSE
all: $(EXT_TARGET).lib
!ENDIF

$(EXT_TARGET).lib $(EXT_TARGET).exp: $(EXT_DEF) $(EXT_OBJS)
    -lib -out:$(@R).lib @<<
-machine:$(MACHINE_TYPE)
-def:$(EXT_DEF)
-nodefaultlib
$(EXT_OBJS)
<<
!IFDEF _NTLIBS
    @$(LIB_COPY) $@ $(@B) LIBS >nul
!ENDIF

$(EXT_TARGET).dll: $(EXT_TARGET).exp $(EXT_OBJS) $(EXT_LIBS)
    $(LINKER) @<<
-dll
-machine:$(MACHINE_TYPE)
-noentry -nod
-base:@$(BASEDIR)\public\sdk\lib\coffbase.txt,$(@B)
-subsystem:console$(SUBSYSTEM_CONVER)
$(HEADEROBJNAME)
$**
<<
    $(BINPLACE_CMD)
!IFDEF _NTDUMPAMAP
    link -dump -map $@ | sort /R /+62 > $(_NTDUMPAMAP)\$(@B).srt
!ENDIF
