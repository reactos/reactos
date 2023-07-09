..\urlmon.rc : ..\selfreg.inf

..\selfreg.inf : ..\selfreg.inx
        $(CLEANINF) $** $@
