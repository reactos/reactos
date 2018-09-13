..\shdoc401.rc : $(SDK_LIB_PATH)\shldisp.tlb ..\selfreg.inf 

..\filetypa.c: ..\filetype.c
    echo #undef UNICODE > $@
    echo #include "priv.h" >> $@
    type $** >> $@

!include $(CCSHELL_DIR)\makefile.inc

