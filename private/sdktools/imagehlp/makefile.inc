OBJECTS = $(OBJECTS) $(BASEDIR)\private\ntos\rtl\user\obj\$(TARGET_DIRECTORY)\imagedir.obj

debug.c undname.c: $(BASEDIR)\public\sdk\inc\ntverp.h

obj\$(TARGET_DIRECTORY)\bind.res: bind.rc

obj\$(TARGET_DIRECTORY)\binplace.res: binplace.rc

obj\$(TARGET_DIRECTORY)\checkfix.res: checkfix.rc

obj\$(TARGET_DIRECTORY)\dbgdump.res: dbgdump.rc

obj\$(TARGET_DIRECTORY)\editsym.res: binplace.rc

obj\$(TARGET_DIRECTORY)\imagecfg.res: imagecfg.rc

obj\$(TARGET_DIRECTORY)\imagechk.res: imagechk.rc

obj\$(TARGET_DIRECTORY)\imagehlp.res: imagehlp.rc

obj\$(TARGET_DIRECTORY)\impchk.res: impchk.rc

obj\$(TARGET_DIRECTORY)\rebase.res: rebase.rc

obj\$(TARGET_DIRECTORY)\smashlck.res: smashlck.rc

obj\$(TARGET_DIRECTORY)\splitsym.res: splitsym.rc

obj\$(TARGET_DIRECTORY)\stripcv.res: stripcv.rc

obj\$(TARGET_DIRECTORY)\undname.res: undname.rc

obj\$(TARGET_DIRECTORY)\verfix.res: verfix.rc

obj\$(TARGET_DIRECTORY)\editsym.res: editsym.rc

obj\$(TARGET_DIRECTORY)\upddbg.res: upddbg.rc

obj\$(TARGET_DIRECTORY)\certify.res: certify.rc

obj\$(TARGET_DIRECTORY)\imhlptst.res: imhlptst.rc

$(O)\pimagehlp.def : imagehlp.src
    @type <<$(ECHO_RSP)
$(ECHO_MSG) /EP $<
<<NOKEEP
    @$(C_PREPROCESSOR_NAME) @<<$(CL_RSP) /Tcimagehlp.src > $@
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
