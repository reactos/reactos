
##
## Assume guidlib.exe is in our path (as is idw and mstools)
##

LIBOBJ          = $(CCSHELL_DIR)\lib\$O

clean:
    -del $(NTTARGETFILES)

#
# Copy stocklib to shell\lib\obj\*
#
$(LIBOBJ)\stock5.lib: $(TARGETPATH)\$(TARGET_DIRECTORY)\stock5.lib
    if not exist $(TARGETPATH)\nul md $(TARGETPATH)
    if not exist $(LIBOBJ)\nul md $(LIBOBJ)
    copy $** $@

