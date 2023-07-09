#OBJECTS = $(OBJECTS) $(BASEDIR)\private\ntos\rtl\user\obj\$(TARGET_DIRECTORY)\imagedir.obj

obj\$(TARGET_DIRECTORY)\imagehlp.res: ..\imagehlp.rc

..\debug.c ..\undname.c: $(BASEDIR)\public\sdk\inc\ntverp.h

$(O)\pimagehlp.def : ..\imagehlp.src
    @type <<$(ECHO_RSP)
$(ECHO_MSG) /EP $<
<<NOKEEP
    @$(C_PREPROCESSOR_NAME) @<<$(CL_RSP) /Tc..\imagehlp.src > $@
$(CPPXX: =
)
-DPRIVATE=
<<NOKEEP

$(O)\pimagehlp.lib : $(O)\pimagehlp.def $(IMPLIB_DEPEND)
    -lib -out:$@ @<<
$(LIBRARIAN_FLAGS: =
)
-def:$(O)\pimagehlp.def
$(LIBRARY_OBJS: =
)
<<NOKEEP
