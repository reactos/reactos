clean:
    -del /f /q $(NTTARGETFILES)

$(BASEDIR)\public\sdk\inc\bugcodes.h : $(O)\bugcodes.h
    copy $** $@
