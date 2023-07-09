..\cdfview.rc : ..\res\*.ico
..\cdfview.rc : ..\res\strings.rcs
..\cdfview.rc : ..\res\version.rcv
..\cdfview.rc : ..\res\selfreg_cdfview.inf
..\cdfview.rc : ..\res\menus.rcm
..\cdfview.rc : ..\res\cdfview.dlg
..\cdfview.rc : ..\res\download.avi
..\cdfview.rc : ..\res\*.gif
..\cdfview.rc : ..\res\xmlerror.htm
..\cdfview.rc : ..\res\hrerror.htm
..\cdfview.rc : ..\res\cacheerr.htm

..\res\selfreg_cdfview.inf : ..\res\selfreg.inx
        $(CLEANINF) $** $@
