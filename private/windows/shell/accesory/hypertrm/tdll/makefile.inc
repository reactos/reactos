
preprocess: ..\term\verdll.rc ..\term\version.h
    cl /nologo /D$(VERSION) /I$(BASEDIR)\public\sdk\inc /Tc..\term\verdll.rc /E > ver_dll.i


copytermrc: ..\term\term.rc
    attrib -r term.rc
    copy ..\term\term.rc term.rc
