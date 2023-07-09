preprocess: verexe.rc version.h
    cl /nologo /D$(VERSION) /I$(BASEDIR)\public\sdk\inc /Tcverexe.rc /E > ver_exe.i
