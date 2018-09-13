# delpathw.c and delnatw.c are temporary files so pathw.c 
# and nativew.cpp are rebuilt correctly since my change
# on 6/11/98.
#

..\delpathw.c: 
    del pathw.c
    type deleteme > $@ 

..\delnatw.c: 
    del nativew.c
    type deleteme > $@ 

..\pathw.c: ..\path.c ..\delpathw.c
    echo // This file expects to be compiled UNICODE > $@
    type $** >> $@

..\nativew.cpp: ..\native.cpp ..\delnatw.c
    echo // This file expects to be compiled UNICODE > $@
    type $** >> $@


