PRIVLIB= $(TARGETPATHLIB)\$(TARGET_DIRECTORY)\advapip.lib
$(TARGETPATHLIB)\$(TARGET_DIRECTORY)\advapi32.lib : $(PRIVLIB)

$(PRIVLIB) $(PRIVLIB:.lib=.exp): $(O)\advapip.def $(LIBRARY_OBJS)
   -lib -out:$(PRIVLIB) @<<
$(LIBRARIAN_FLAGS)
-def:$(O)\advapip.def
$(LIBRARY_OBJS)
<<NOKEEP

$(O)\advapip.def: ..\advapi32.def
    $(C_PREPROCESSOR) $** -DPRIVATE= > $@
