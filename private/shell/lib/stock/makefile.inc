..\shstra.cpp: ..\shstr.cpp
    echo #ifdef UNICODE > $@
    echo #undef UNICODE >> $@
    echo #endif >> $@
    type $** >> $@

..\shstrw.cpp: ..\shstr.cpp
    echo #ifndef UNICODE > $@
    echo #define UNICODE >> $@
    echo #endif >> $@
    type $** >> $@

..\dkaa.cpp: ..\dka.cpp
    echo #ifdef UNICODE > $@
    echo #undef UNICODE >> $@
    echo #endif >> $@
    type $** >> $@

..\dkaw.cpp: ..\dka.cpp
    echo #ifndef UNICODE > $@
    echo #define UNICODE >> $@
    echo #endif >> $@
    type $** >> $@

..\nativea.cpp: ..\native.cpp
    echo #ifdef UNICODE > $@
    echo #undef UNICODE >> $@
    echo #endif >> $@
    type $** >> $@

..\nativew.cpp: ..\native.cpp
    echo #ifndef UNICODE > $@
    echo #define UNICODE >> $@
    echo #endif >> $@
    type $** >> $@

LIBOBJ          = $(CCSHELL_DIR)\lib\$O

clean:
    -del $(NTTARGETFILE0)
    -del $(NTTARGETFILES)

#
# Copy stocklib to shell\lib\obj\*
#
$(LIBOBJ)\stocklib.lib: $(TARGETPATH)\$(TARGET_DIRECTORY)\stocklib.lib
    if not exist $(TARGETPATH)\nul md $(TARGETPATH)
    if not exist $(LIBOBJ)\nul md $(LIBOBJ)
    copy $** $@

## webcheck includes rpc headers. Not here as I dont think they
##  will affect guids that we need.
$(O)\shguid.obj: ..\shguid.c \
        $(BASEDIR)\public\sdk\inc\shlguid.h \
        $(BASEDIR)\public\sdk\inc\isguids.h
    @type <<$(ECHO_RSP)
$(ECHO_MSG_P)
<<NOKEEP
    @$(C_COMPILER_NAME) @<<$(CL_RSP) -Fo$(MAKEDIR)\$@ $(USECXX_FLAG) $(MAKEDIR)\..\shguid.c
$(NP_COMPILER_FLAGS: =
)
<<NOKEEP


$(LIBOBJ)\shguid.lib: $(O)\shguid.obj
    if not exist $(LIBOBJ)\nul md $(LIBOBJ)
    copy $** $@

$(O)\shguidp.obj: ..\shguidp.c \
        $(BASEDIR)\private\inc\shlguidp.h \
        $(CCSHELL_DIR)\inc\shguidp.h \
        $(CCSHELL_DIR)\inc\shdguid.h \
        $(CCSHELL_DIR)\inc\sfview.h \
        $(CCSHELL_DIR)\inc\dvocx.h \
        $(BASEDIR)\public\sdk\inc\adoid.h \
        $(PRIVATE_INC_PATH)\shlobjp.h
    @type <<$(ECHO_RSP)
$(ECHO_MSG_P)
<<NOKEEP
    @$(C_COMPILER_NAME) @<<$(CL_RSP) -Fo$(MAKEDIR)\$@ $(USECXX_FLAG) $(MAKEDIR)\..\shguidp.c
$(NP_COMPILER_FLAGS: =
)
<<NOKEEP

$(LIBOBJ)\shguidp.lib: $(O)\shguidp.obj
    copy $** $@
