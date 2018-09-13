..\inetcpl.rc : ..\selfreg_inetcpl.inf
..\inetcpl.rc : ..\inetcpld.rc

..\selfreg_inetcpl.inf : ..\selfreg.inx
        $(CLEANINF) $** $@
