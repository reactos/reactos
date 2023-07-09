
PRIVLIB= $(TARGETPATHLIB)\$(TARGET_DIRECTORY)\user32p.lib
$(TARGETPATHLIB)\$(TARGET_DIRECTORY)\user32.lib : $(PRIVLIB)

PRIVLIB= $(TARGETPATHLIB)\$(TARGET_DIRECTORY)\user32p.lib

$(O)\user32.def: ..\user32.def
    @echo Creating $@ from $**
    $(C_PREPROCESSOR) $** > $@

dispcb.c: ..\dispcb.tpl ..\..\inc\$(ALT_PROJECT)\cb.lst
    listmung ..\..\inc\$(ALT_PROJECT)\cb.lst ..\dispcb.tpl >dispcb.c

..\csenda.c: ..\ntcftxt.h
..\csendw.c: ..\ntcftxt.h
..\crecv.c:  ..\..\inc\ntcb.h ..\..\inc\$(ALT_PROJECT)\cb.lst ..\..\inc\messages.h
..\clmsg.c: ..\..\inc\messages.h
..\clienta.c: ..\cltxt.h
..\clientw.c: ..\cltxt.h
..\csrstubs.c: ..\..\inc\csrmsg.h
..\ntstubs.c: ..\..\inc\messages.h
..\precomp.h: ..\usercli.h ..\..\inc\user.h ..\..\inc\ntuser.h ..\..\..\..\..\inc\winuserp.h \
              ..\..\inc\$(ALT_PROJECT)\cscall.h ..\..\..\w32inc\winuserk.h
..\res.rc: ..\rcscale\truetype.bmp  \
           ..\rc96dpi\truetype.bmp  \
           ..\rc120dpi\truetype.bmp  \
           ..\cursicon\windows.ico  \
           ..\cursicon\warning.ico  \
           ..\cursicon\question.ico  \
           ..\cursicon\error.ico  \
           ..\cursicon\info.ico  \
           ..\cursicon\app.ico  \
           ..\cursicon\shtdwn.ico  \
           ..\cursicon\help.bmp  \
           ..\cursicon\arrow.cur  \
           ..\cursicon\ibeam.cur  \
           ..\cursicon\wait.cur  \
           ..\cursicon\cross.cur \
           ..\cursicon\uparrow.cur  \
           ..\cursicon\sizenwse.cur  \
           ..\cursicon\sizenesw.cur  \
           ..\cursicon\sizewe.cur  \
           ..\cursicon\sizens.cur  \
           ..\cursicon\sizeall.cur  \
           ..\cursicon\nodrop.cur  \
           ..\cursicon\appstart.cur  \
           ..\cursicon\help.cur  \
           ..\cursicon\nwpen.cur  \
!ifdef JANUS
           ..\strid.rc  \
!endif
           ..\cursicon\icon.cur



$(PRIVLIB) $(PRIVLIB:.lib=.exp): $(O)\user32p.def $(LIBRARY_OBJS)
   -lib -out:$(PRIVLIB) @<<
$(LIBRARIAN_FLAGS)
-def:$(O)\user32p.def
$(LIBRARY_OBJS)
<<NOKEEP

!IF "$(PRECOMPILED_INCLUDE)" != ""
$(O)\clienta.obj $(O)\csenda.obj :
    nmake PRECOMPILED_INCLUDE= $(O)\$(@F)
!ENDIF

C_DEFINES=$(C_DEFINES) -DPRIVATE=
$(O)\user32p.def: $(O)\user32.def
    $(C_PREPROCESSOR) $** > $@
