..\cdfvwlc.rc : ..\res\*.ico
..\cdfvwlc.rc : ..\res\strings.rcs
..\cdfvwlc.rc : ..\res\version.rcv
..\cdfvwlc.rc : ..\res\selfreg_cdfview.inf
..\cdfvwlc.rc : ..\res\menus.rcm
..\cdfvwlc.rc : ..\res\cdfview.dlg
..\cdfvwlc.rc : ..\res\download.avi
..\cdfvwlc.rc : ..\res\*.gif
..\cdfvwlc.rc : ..\res\xmlerror.htm
..\cdfvwlc.rc : ..\res\hrerror.htm
..\cdfvwlc.rc : ..\res\cacheerr.htm

..\res\selfreg_cdfview.inf : ..\res\selfreg.inx
        $(CLEANINF) $** $@
