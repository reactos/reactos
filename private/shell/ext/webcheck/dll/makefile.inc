..\webcheck.rc : $(SDK_LIB_PATH)\webcheck.tlb ..\selfreg_webcheck.inf

..\selfreg_webcheck.inf : ..\selfreg.inx
        $(CLEANINF) $** $@

