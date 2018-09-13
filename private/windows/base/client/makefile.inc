t.c: dllprof.c

winerror.rc: $(BASEDIR)\private\genx\nlsmsg\$(O)\winerror.rc
        copy $** $@

msg00001.bin: $(BASEDIR)\private\genx\nlsmsg\$(O)\msg00001.bin
        copy $** $@

..\base.rc : msg00001.bin winerror.rc ..\..\..\winnls\winnls.rc


#
# Build private kernel32.lib.  It contains all the private api's and
# the Win9x api's.
#

obj\$(TARGET_DIRECTORY)\kernl32p.def: ..\kernel32.src
    @echo Creating $@ from $**
    $(C_PREPROCESSOR_NAME) /DALL_WIN32_EXPORTS /DPRIVATE= $(C_PREPROCESSOR_FLAGS) $** > $@

obj\$(TARGET_DIRECTORY)\kernl32p.lib: obj\$(TARGET_DIRECTORY)\kernl32p.def $(LIBRARY_OBJS)
   -lib -out:$@ @<<
$(LIBRARIAN_FLAGS)
-def:obj\$(TARGET_DIRECTORY)\kernl32p.def
$(LIBRARY_OBJS)
<<NOKEEP

$(BASEDIR)\public\sdk\lib\$(ALT_PROJECT_TARGET)\$(TARGET_DIRECTORY)\kernl32p.lib : obj\$(TARGET_DIRECTORY)\kernl32p.lib
    copy $** $@

#
# Build public kernel32.lib with extra Win9x exports.
#

obj\$(TARGET_DIRECTORY)\w32kernel32.def: ..\kernel32.src
    @echo Creating $@ from $**
    $(C_PREPROCESSOR_NAME) /DALL_WIN32_EXPORTS $(C_PREPROCESSOR_FLAGS) $** > $@

obj\$(TARGET_DIRECTORY)\w32kernel32.lib : obj\$(TARGET_DIRECTORY)\w32kernel32.def $(LIBRARY_OBJS)
   -lib -out:$@ @<<
$(LIBRARIAN_FLAGS)
-def:obj\$(TARGET_DIRECTORY)\w32kernel32.def
$(LIBRARY_OBJS)
<<NOKEEP

$(BASEDIR)\public\sdk\lib\$(ALT_PROJECT_TARGET)\$(TARGET_DIRECTORY)\kernel32.lib : obj\$(TARGET_DIRECTORY)\w32kernel32.lib
    copy $** $@
