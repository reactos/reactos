obj\*\urlhlink.def: urlhlink.src

!IF $(386)
!IF "$(LINKONLY)"!=""
#MAPSYM=$(_WINBASE)\dev\tools\common\mapsym.exe -nologo
MAPSYM=mapsym.exe -nologo

!IF EXIST($(TARGETPATH)\i386\urlhlink.map)
# generate the sym file from the map file
urlhlink.sym:      $(TARGETPATH)\i386\urlhlink.map
        $(MAPSYM) -o $(TARGETPATH)\i386\urlhlink.sym $(TARGETPATH)\i386\urlhlink.map
        binplace -R $(_NTTREE) $(TARGETPATH)\i386\urlhlink.sym
        binplace -R $(_NTTREE) $(TARGETPATH)\i386\urlhlink.map
!else
# generate the sym file from the dll with coff symbols
urlhlink.sym:      $(TARGETPATH)\i386\urlhlink.lib
        $(MAPSYM) -o $(TARGETPATH)\i386\urlhlink.sym $(TARGETPATH)\i386\urlhlink.map
        binplace -R $(_NTTREE) $(TARGETPATH)\i386\urlhlink.sym
        binplace -R $(_NTTREE) $(TARGETPATH)\i386\urlhlink.map
!endif
!else
urlhlink.sym:      $(TARGETPATH)\i386\urlhlink.map
        echo Only build sym file during link phase.
!endif

!endif

