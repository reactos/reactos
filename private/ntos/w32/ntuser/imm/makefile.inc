PRIVLIB= $(BASEDIR)\public\sdk\lib\$(ALT_PROJECT_TARGET)\$(TARGET_DIRECTORY)\imm32p.lib

$(PRIVLIB) $(PRIVLIB:.lib=.exp): imm32p.def $(LIBRARY_OBJS)
   -lib -out:$(PRIVLIB) @<<
$(LIBRARIAN_FLAGS)
-def:imm32p.def
$(LIBRARY_OBJS)
<<NOKEEP


C_DEFINES=$(C_DEFINES) -DPRIVATE=
imm32p.def: $(DLLDEF)
    $(C_PREPROCESSOR) $(DLLDEF) > $@
