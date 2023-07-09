PRIVLIB= $(TARGETPATH)\$(TARGET_DIRECTORY)\wmip.lib
$(TARGETPATH)\$(TARGET_DIRECTORY)\wmi.lib : $(PRIVLIB)

$(PRIVLIB) $(PRIVLIB:.lib=.exp): $(O)\wmip.def $(LIBRARY_OBJS)
   -lib -out:$(PRIVLIB) @<<
$(LIBRARIAN_FLAGS)
-def:$(O)\wmip.def
$(LIBRARY_OBJS)
<<NOKEEP

$(O)\wmip.def: ..\advapi32.def
    $(C_PREPROCESSOR) $** -DPRIVATE= -DWMIDEF > $@

$(O)\wmi.def: ..\advapi32.def
    $(C_PREPROCESSOR) $** -DWMIDEF > $@
