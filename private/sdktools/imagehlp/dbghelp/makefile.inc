#OBJECTS = $(OBJECTS) $(BASEDIR)\private\ntos\rtl\user\obj\$(TARGET_DIRECTORY)\imagedir.obj

obj\$(TARGET_DIRECTORY)\dbghelp.res: ..\dbghelp.rc

..\debug.c ..\undname.c: $(BASEDIR)\public\sdk\inc\ntverp.h

$(O)\pdbghelp.def : ..\dbghelp.src
    @type <<$(ECHO_RSP)
$(ECHO_MSG) /EP $<
<<NOKEEP
    @$(C_PREPROCESSOR_NAME) @<<$(CL_RSP) /Tc..\dbghelp.src > $@
$(CPPXX: =
)
-DPRIVATE=
<<NOKEEP

$(O)\pdbghelp.lib : $(O)\pdbghelp.def $(IMPLIB_DEPEND)
    -lib -out:$@ @<<
$(LIBRARIAN_FLAGS: =
)
-def:$(O)\pdbghelp.def
$(LIBRARY_OBJS: =
)
<<NOKEEP
