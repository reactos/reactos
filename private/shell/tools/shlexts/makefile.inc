HDEPENDENCIES=..\exts.h $(BASEDIR)\private\inc\stdexts.h

GENUEDEFDEFINES=/D_$(ALT_PROJECT)_
!if !$(FREEBUILD)
GENUEDEFDEFINES=$(GENUEDEFDEFINES) /DDEBUGTAGS=1
!endif

precomp.h: ..\precomp.h

shlexts.def: ..\genuedef.c $(HDEPENDENCIES)
        $(CXX_COMPILER_NAME) $(GENUEDEFDEFINES) /EP ..\genuedef.c >shlexts.def

dsotable.c: ..\ntsd.txt ..\dsocode.c
    $(C_PREPROCESSOR) ..\shlkdx.h > shlkdx.pph   
    structo -i h -p -s ..\ntsd.txt -o $@ shlkdx.pph


#    del shlkdx.pph
